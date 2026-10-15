"""Bakes procedural looks (looter_looks.py) into a texture set the game's textured masters read: T_<Set>_BC.png (sRGB
colour), T_<Set>_N.png (tangent-space normal, DirectX: green down, as Unreal expects) and T_<Set>_ORM.png (linear: R
ambient occlusion, G roughness, B metallic), in Art/Textures/<Set>/ (looter_textures' convention).

    pack(objects)                                  # one atlas: every object's 'UVMap' islands, even density, packed
    maps = bake_set('EllisGear', objects, 2048)   # bakes every material on them (each must be a look)

Each pass swaps each look's output for an emission of one of its OUT_ reroutes (exact values, no lighting), except the
normal (Cycles' normal bake of the look's own shading normal) and the occlusion (Cycles' AO bake, which sees every
object in the scene, so a belt shades the coat under it). Bakes are deterministic for a given Blender build.
"""
import hashlib
import os
import time

import bpy
import numpy as np

import looter_textures as lt

PASSES = (('COLOR', 'OUT_COLOR', 'sRGB'), ('ROUGH', 'OUT_ROUGH', 'Non-Color'), ('METAL', 'OUT_METAL', 'Non-Color'))


def log(message):
    print(f'BAKE {message}', flush=True)


def select(objects, active=None):
    bpy.ops.object.mode_set(mode='OBJECT') if bpy.context.object and bpy.context.object.mode != 'OBJECT' else None
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objects:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = active or objects[0]


def smart_uv(ob, angle=60.0, margin=0.01):
    """A fresh 'UVMap' by smart projection (for parts made without UVs, or with overlapping ones)."""
    me = ob.data
    if 'UVMap' not in me.uv_layers:
        me.uv_layers.new(name='UVMap')
    me.uv_layers.active = me.uv_layers['UVMap']
    select([ob])
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=np.radians(angle), island_margin=margin)
    bpy.ops.object.mode_set(mode='OBJECT')


def pack(objects, margin=0.004, weights=None):
    """Packs the 'UVMap' islands of all objects into one 0..1 atlas at an even texel density. weights {object: k}
    scales an object's islands before packing (k > 1 gives it more texels)."""
    for ob in objects:
        ob.data.uv_layers.active = ob.data.uv_layers['UVMap']
    select(objects)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.select_all(action='SELECT')
    bpy.ops.uv.average_islands_scale()
    bpy.ops.object.mode_set(mode='OBJECT')
    if weights:
        for ob, k in weights.items():
            layer = ob.data.uv_layers['UVMap']
            uv = np.empty(len(layer.data) * 2, dtype=np.float32)
            layer.data.foreach_get('uv', uv)
            layer.data.foreach_set('uv', uv * k)
    select(objects)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.select_all(action='SELECT')
    try:
        bpy.ops.uv.pack_islands(margin=margin, rotate=True)
    except TypeError:
        bpy.ops.uv.pack_islands(margin=margin)
    bpy.ops.object.mode_set(mode='OBJECT')


def _image(name, size, colorspace, float_buffer=False):
    img = bpy.data.images.get(name)
    if img is not None:
        bpy.data.images.remove(img)
    img = bpy.data.images.new(name, size, size, alpha=False, float_buffer=float_buffer)
    img.colorspace_settings.name = colorspace
    return img


def _materials(objects):
    mats = []
    for ob in objects:
        for slot in ob.material_slots:
            if slot.material is not None and slot.material not in mats:
                mats.append(slot.material)
    return mats


def _target(mat, img):
    nt = mat.node_tree
    nd = nt.nodes.get('BAKE_TARGET') or nt.nodes.new('ShaderNodeTexImage')
    nd.name = 'BAKE_TARGET'
    nd.image = img
    nt.nodes.active = nd
    nd.select = True


def _output(mat):
    return next(n for n in mat.node_tree.nodes if n.type == 'OUTPUT_MATERIAL')


def _swap_emission(mat, reroute):
    """Wires reroute (an OUT_ node) through an Emission into the output; returns the link to restore."""
    nt = mat.node_tree
    out = _output(mat)
    old = out.inputs['Surface'].links[0].from_socket if out.inputs['Surface'].links else None
    em = nt.nodes.get('BAKE_EMIT') or nt.nodes.new('ShaderNodeEmission')
    em.name = 'BAKE_EMIT'
    em.inputs['Strength'].default_value = 1.0
    for ln in list(em.inputs['Color'].links):
        nt.links.remove(ln)
    nt.links.new(nt.nodes[reroute].outputs[0], em.inputs['Color'])
    nt.links.new(em.outputs[0], out.inputs['Surface'])
    return old


def _restore(mat, old):
    if old is not None:
        mat.node_tree.links.new(old, _output(mat).inputs['Surface'])


def _pixels(img):
    a = np.empty(img.size[0] * img.size[1] * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    return a.reshape(img.size[1], img.size[0], 4)


def bake_set(set_name, objects, size=2048, samples=1, ao_samples=48, ao_distance=0.1, margin=12, out_dir=None):
    """Bakes the looks on objects into the set's three PNGs. Returns {'BC', 'N', 'ORM': path, 'images': {...}} with
    the baked Blender images kept (COLOR, ROUGH, METAL, NORMAL as OpenGL, AO) for previews and glTF."""
    out_dir = out_dir or os.path.join(lt.TEXTURE_DIR, set_name)
    os.makedirs(out_dir, exist_ok=True)
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    bk = sc.render.bake
    bk.margin = margin
    bk.use_clear = True
    try:
        bk.target = 'IMAGE_TEXTURES'
    except Exception:
        pass
    mats = _materials(objects)
    for ob in objects:
        ob.data.uv_layers.active = ob.data.uv_layers['UVMap']
    images = {}
    # occlusion first: the looks' grime reads it instead of tracing its own rays
    img = _image(f'{set_name}_AO', size, 'Non-Color')
    for m in mats:
        _target(m, img)
    sc.cycles.samples = ao_samples
    if sc.world is None:
        sc.world = bpy.data.worlds.new('BakeWorld')
    sc.world.light_settings.distance = ao_distance
    select(objects)
    t0 = time.time()
    bpy.ops.object.bake(type='AO', margin=margin, use_clear=True)
    images['AO'] = img
    for m in mats:
        _use_baked_ao(m, img)
    log(f'{set_name}: occlusion ({time.time() - t0:.0f} s)')
    for key, reroute, cs in PASSES:
        img = _image(f'{set_name}_{key}', size, cs)
        olds = {}
        for m in mats:
            _target(m, img)
            olds[m.name] = _swap_emission(m, reroute)
        sc.cycles.samples = samples
        select(objects)
        t0 = time.time()
        bpy.ops.object.bake(type='EMIT', margin=margin, use_clear=True)
        for m in mats:
            _restore(m, olds[m.name])
        images[key] = img
        log(f'{set_name}: {key.lower()} ({time.time() - t0:.0f} s)')
    img = _image(f'{set_name}_NORMAL', size, 'Non-Color')
    for m in mats:
        _target(m, img)
    sc.cycles.samples = 1
    select(objects)
    t0 = time.time()
    bpy.ops.object.bake(type='NORMAL', normal_space='TANGENT', margin=margin, use_clear=True)
    images['NORMAL'] = img
    log(f'{set_name}: normal ({time.time() - t0:.0f} s)')
    for m in mats:
        nd = m.node_tree.nodes.get('BAKE_TARGET')
        if nd is not None:
            m.node_tree.nodes.remove(nd)
        em = m.node_tree.nodes.get('BAKE_EMIT')
        if em is not None:
            m.node_tree.nodes.remove(em)
    # write the set: colour as it is; normal with green flipped (DirectX); ORM packed
    paths = {}
    bc = _pixels(images['COLOR'])
    paths['BC'] = lt.texture_path(set_name, 'BC')
    _save(bc, paths['BC'], 'sRGB')
    n = _pixels(images['NORMAL']).copy()
    n[..., 1] = 1.0 - n[..., 1]
    paths['N'] = lt.texture_path(set_name, 'N')
    _save(n, paths['N'], 'Non-Color')
    orm = np.stack([_pixels(images['AO'])[..., 0], _pixels(images['ROUGH'])[..., 0], _pixels(images['METAL'])[..., 0],
                    np.ones((size, size), dtype=np.float32)], -1)
    paths['ORM'] = lt.texture_path(set_name, 'ORM')
    _save(orm, paths['ORM'], 'Non-Color')
    images['ORM'] = _from_pixels(f'{set_name}_ORMpacked', orm, 'Non-Color')
    paths['images'] = images
    log(f'{set_name}: written to {os.path.dirname(paths["BC"])}')
    return paths


def load_set(set_name):
    """A set baked before, read back from its PNGs into the images bake_set returns (the normal turned back to
    OpenGL), for previews and glTF when the bake is skipped. None if a file is missing."""
    paths = {k: lt.texture_path(set_name, k) for k in ('BC', 'N', 'ORM')}
    if not all(os.path.exists(p) for p in paths.values()):
        return None
    images = {}
    images['COLOR'] = bpy.data.images.load(paths['BC'], check_existing=False)
    images['COLOR'].colorspace_settings.name = 'sRGB'
    orm = bpy.data.images.load(paths['ORM'], check_existing=False)
    orm.colorspace_settings.name = 'Non-Color'
    images['ORM'] = orm
    n = bpy.data.images.load(paths['N'], check_existing=False)
    n.colorspace_settings.name = 'Non-Color'
    px = _pixels(n).copy()
    px[..., 1] = 1.0 - px[..., 1]
    images['NORMAL'] = _from_pixels(f'{set_name}_NORMAL', px, 'Non-Color')
    out = dict(paths)
    out['images'] = images
    return out


def digest(paths, extra=''):
    """A fingerprint of the files a bake depends on (their bytes) and any extra settings."""
    h = hashlib.sha1(extra.encode())
    for p in sorted(paths):
        with open(p, 'rb') as f:
            h.update(f.read())
    return h.hexdigest()


def _use_baked_ao(mat, img):
    """Swaps the look's ambient-occlusion nodes for the baked occlusion map (read through 'UVMap')."""
    nt = mat.node_tree
    for nd in [n for n in nt.nodes if n.bl_idname == 'ShaderNodeAmbientOcclusion']:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = img
        uv = nt.nodes.new('ShaderNodeUVMap')
        uv.uv_map = 'UVMap'
        nt.links.new(uv.outputs['UV'], tex.inputs['Vector'])
        for out in nd.outputs:
            for ln in list(out.links):
                nt.links.new(tex.outputs['Color'], ln.to_socket)
        nt.nodes.remove(nd)


def _from_pixels(name, px, colorspace):
    h, w = px.shape[:2]
    img = _image(name, w, colorspace)
    img.pixels.foreach_set(np.ascontiguousarray(px, dtype=np.float32).ravel())
    return img


def _save(px, path, colorspace):
    """Writes pixels (as Blender holds them: bottom row first, RGBA 0..1) to an 8-bit PNG."""
    img = _from_pixels('_save', px, colorspace)
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)
