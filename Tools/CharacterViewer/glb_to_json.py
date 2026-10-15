"""A .glb as a glTF JSON (geometry embedded as base64) and one image file per texture (.webp as exported), for hosts
that serve no binary model files: artifacts serve .json and .webp but not .glb. The spin viewer (index.html beside this)
loads <name>.json.

    python Tools/CharacterViewer/glb_to_json.py <in.glb> <out dir> <name>
"""
import base64, json, os, struct, sys
src, out_dir, name = sys.argv[1], sys.argv[2], sys.argv[3]
data = open(src, 'rb').read()
assert data[:4] == b'glTF'
off, js, binc = 12, None, None
while off < len(data):
    ln, kind = struct.unpack_from('<II', data, off)
    chunk = data[off + 8: off + 8 + ln]
    if kind == 0x4E4F534A: js = json.loads(chunk)
    elif kind == 0x004E4942: binc = chunk
    off += 8 + ln
ext = {'image/webp': '.webp', 'image/png': '.png', 'image/jpeg': '.jpg'}
image_views = set()
for i, img in enumerate(js.get('images', [])):
    bv = js['bufferViews'][img['bufferView']]
    raw = binc[bv.get('byteOffset', 0): bv.get('byteOffset', 0) + bv['byteLength']]
    fn = f"{name}_{i}{ext.get(img.get('mimeType'), '.bin')}"
    open(os.path.join(out_dir, fn), 'wb').write(raw)
    image_views.add(img['bufferView'])
    del img['bufferView']; img.pop('mimeType', None); img['uri'] = fn
# rebuild the buffer from the views that are left, renumbering them
new_bin, remap, views = bytearray(), {}, []
for i, bv in enumerate(js['bufferViews']):
    if i in image_views: continue
    raw = binc[bv.get('byteOffset', 0): bv.get('byteOffset', 0) + bv['byteLength']]
    while len(new_bin) % 4: new_bin.append(0)
    bv = dict(bv); bv['byteOffset'] = len(new_bin); new_bin += raw
    remap[i] = len(views); views.append(bv)
js['bufferViews'] = views
for acc in js.get('accessors', []):
    if 'bufferView' in acc: acc['bufferView'] = remap[acc['bufferView']]
js['buffers'] = [{'byteLength': len(new_bin), 'uri': 'data:application/octet-stream;base64,' + base64.b64encode(bytes(new_bin)).decode()}]
json.dump(js, open(os.path.join(out_dir, name + '.json'), 'w'), separators=(',', ':'))
print('images', len(image_views), 'json', os.path.getsize(os.path.join(out_dir, name + '.json')) // 1024, 'KB')
