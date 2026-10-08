"""GLB reading, merging and checking in plain Python (no glTF library): the packer joins the per-model GLBs into
packs of at most 14 MB, one top-level node per model, materials shared by name."""
import json
import struct

import numpy as np

COMPONENT = {5120: np.int8, 5121: np.uint8, 5122: np.int16, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
COUNT = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT2': 4, 'MAT3': 9, 'MAT4': 16}


def read(path):
    with open(path, 'rb') as f:
        data = f.read()
    magic, version, length = struct.unpack_from('<III', data, 0)
    if magic != 0x46546C67:
        raise ValueError(f'{path}: not a GLB')
    offset = 12
    gltf, binary = None, b''
    while offset < length:
        chunk_len, chunk_type = struct.unpack_from('<II', data, offset)
        chunk = data[offset + 8:offset + 8 + chunk_len]
        if chunk_type == 0x4E4F534A:
            gltf = json.loads(chunk.decode('utf-8'))
        elif chunk_type == 0x004E4942:
            binary = chunk
        offset += 8 + chunk_len
    return gltf, binary


def write(path, gltf, binary):
    text = json.dumps(gltf, separators=(',', ':')).encode('utf-8')
    text += b' ' * ((4 - len(text) % 4) % 4)
    binary = bytes(binary) + b'\0' * ((4 - len(binary) % 4) % 4)
    total = 12 + 8 + len(text) + (8 + len(binary) if binary else 0)
    with open(path, 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, total))
        f.write(struct.pack('<II', len(text), 0x4E4F534A))
        f.write(text)
        if binary:
            f.write(struct.pack('<II', len(binary), 0x004E4942))
            f.write(binary)


def accessor(gltf, binary, index):
    acc = gltf['accessors'][index]
    view = gltf['bufferViews'][acc['bufferView']]
    dtype = COMPONENT[acc['componentType']]
    n = COUNT[acc['type']]
    start = view.get('byteOffset', 0) + acc.get('byteOffset', 0)
    stride = view.get('byteStride')
    item = np.dtype(dtype).itemsize * n
    if stride and stride != item:
        raw = np.frombuffer(binary, np.uint8, count=stride * acc['count'], offset=start).reshape(acc['count'], stride)
        out = raw[:, :item].copy().view(dtype).reshape(acc['count'], n)
    else:
        out = np.frombuffer(binary, dtype, count=acc['count'] * n, offset=start).reshape(acc['count'], n)
    if acc.get('normalized'):
        out = out.astype(np.float32) / np.iinfo(dtype).max
    return out


class Merger:
    """Appends whole GLBs into one: every glTF array offset, one buffer, materials deduplicated by name, each source's
    scene roots become top-level nodes of the one scene."""

    ARRAYS = ('accessors', 'bufferViews', 'meshes', 'materials', 'nodes', 'skins', 'textures', 'images', 'samplers')

    def __init__(self):
        self.gltf = {'asset': {'version': '2.0', 'generator': 'Style Lab exporter (Tools/StyleLab/export)'},
                     'scene': 0, 'scenes': [{'nodes': []}], 'buffers': [{'byteLength': 0}]}
        for key in self.ARRAYS:
            self.gltf[key] = []
        self.binary = bytearray()
        self.material_by_name = {}
        self.extensions = set()

    def add(self, path):
        src, binary = read(path)
        base = len(self.binary)
        pad = (4 - base % 4) % 4
        self.binary += b'\0' * pad
        base += pad
        self.binary += binary
        off = {key: len(self.gltf[key]) for key in self.ARRAYS}
        # Materials by name.
        mat_map = {}
        for i, mat in enumerate(src.get('materials', [])):
            name = mat.get('name', f'mat{i}')
            if name not in self.material_by_name:
                self.material_by_name[name] = len(self.gltf['materials'])
                self.gltf['materials'].append(mat)
            mat_map[i] = self.material_by_name[name]
        for view in src.get('bufferViews', []):
            view = dict(view)
            view['buffer'] = 0
            view['byteOffset'] = view.get('byteOffset', 0) + base
            self.gltf['bufferViews'].append(view)
        for acc in src.get('accessors', []):
            acc = dict(acc)
            if 'bufferView' in acc:
                acc['bufferView'] += off['bufferViews']
            self.gltf['accessors'].append(acc)
        for mesh in src.get('meshes', []):
            mesh = json.loads(json.dumps(mesh))
            for prim in mesh['primitives']:
                prim['attributes'] = {k: v + off['accessors'] for k, v in prim['attributes'].items()}
                if 'indices' in prim:
                    prim['indices'] += off['accessors']
                if 'material' in prim:
                    prim['material'] = mat_map[prim['material']]
                for target in prim.get('targets', []):
                    for k in target:
                        target[k] += off['accessors']
            self.gltf['meshes'].append(mesh)
        for node in src.get('nodes', []):
            node = dict(node)
            if 'mesh' in node:
                node['mesh'] += off['meshes']
            if 'skin' in node:
                node['skin'] += off['skins']
            if 'children' in node:
                node['children'] = [c + off['nodes'] for c in node['children']]
            self.gltf['nodes'].append(node)
        for skin in src.get('skins', []):
            skin = dict(skin)
            skin['joints'] = [j + off['nodes'] for j in skin['joints']]
            if 'skeleton' in skin:
                skin['skeleton'] += off['nodes']
            if 'inverseBindMatrices' in skin:
                skin['inverseBindMatrices'] += off['accessors']
            self.gltf['skins'].append(skin)
        scene = src['scenes'][src.get('scene', 0)]
        roots = [n + off['nodes'] for n in scene['nodes']]
        self.gltf['scenes'][0]['nodes'].extend(roots)
        self.extensions.update(src.get('extensionsUsed', []))
        return [self.gltf['nodes'][n].get('name') for n in roots]

    def size(self):
        return len(self.binary) + len(json.dumps(self.gltf))

    def save(self, path):
        self.gltf['buffers'][0]['byteLength'] = len(self.binary) + (4 - len(self.binary) % 4) % 4
        for key in self.ARRAYS:
            if not self.gltf[key]:
                del self.gltf[key]
        if self.extensions:
            self.gltf['extensionsUsed'] = sorted(self.extensions)
        write(path, self.gltf, self.binary)


def summary(path):
    """Top-level node names, material names and per-node triangle counts of a GLB."""
    gltf, binary = read(path)
    scene = gltf['scenes'][gltf.get('scene', 0)]
    materials = [m.get('name') for m in gltf.get('materials', [])]

    def tris_under(n):
        node = gltf['nodes'][n]
        t = 0
        if 'mesh' in node:
            for prim in gltf['meshes'][node['mesh']]['primitives']:
                if 'indices' in prim:
                    t += gltf['accessors'][prim['indices']]['count'] // 3
                else:
                    t += gltf['accessors'][prim['attributes']['POSITION']]['count'] // 3
        for c in node.get('children', []):
            t += tris_under(c)
        return t
    roots = {gltf['nodes'][n].get('name'): tris_under(n) for n in scene['nodes']}
    return roots, materials, gltf, binary
