"""Builds the hero page (HANDOFF.md, "Five heroes"). Exports each hero script in Art/Backlog/Characters as a skinned GLB,
with its bones' rest points and sockets in a JSON beside it, then assembles the page from ui.html, the concept viewer's
engine (Tools/ConceptViewer/web) and heroes.js.
  pip install "bpy==4.5.*" numpy                              # once
  python3 Docs/HeroSelect/build.py [out folder] [Name ...]    # default Saved/HeroSelect, all five heroes

Writes index.html (the page to publish, with the files in models/ beside it) and test.html (the same page with a
document skeleton, for testing locally). The exporter runs under plain Python with the bpy module, not inside Blender.
"""
import base64
import json
import os
import runpy
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Tools', 'Blender'))
OUT = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(REPO, 'Saved', 'HeroSelect')
NAMES = sys.argv[2:] or ['Ellis', 'Odessa', 'Crane', 'Gauge', 'Pike']
MODELS = os.path.join(OUT, 'models')
os.makedirs(MODELS, exist_ok=True)

import bpy  # noqa: E402


def export(name):
    t0 = time.time()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    runpy.run_path(os.path.join(REPO, 'Art', 'Backlog', 'Characters', name + '.py'), run_name='__main__')
    arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE' and o.get('Hero'))
    body = next(c for c in arm.children if c.type == 'MESH')
    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True)
    body.select_set(True)
    bpy.context.view_layer.objects.active = arm
    path = os.path.join(MODELS, f'Hero_{arm["Hero"]}.glb')
    # No materials: the page draws the print palette from the 'Flat' vertex color the hero kit bakes.
    bpy.ops.export_scene.gltf(filepath=path, export_format='GLB', use_selection=True, export_apply=False,
                              export_materials='NONE', export_vertex_color='ACTIVE',
                              export_active_vertex_color_when_no_material=True, export_all_vertex_colors=False,
                              export_normals=True, export_texcoords=False, export_skins=True, export_animations=False,
                              export_yup=True, export_extras=False, export_def_bones=False)
    # The artifact host serves JSON but not .glb, so the model ships as a base64 string in a .json.
    with open(path, 'rb') as f:
        data = base64.b64encode(f.read()).decode('ascii')
    with open(path[:-4] + '.json', 'w') as f:
        f.write('"' + data + '"')
    # Bone rest points and sockets in the page's frame (glTF y-up: x, z, -y). The page poses bones about their rest
    # directions and carries props on the sockets.
    conv = lambda p: [round(p[0], 4), round(p[2], 4), round(-p[1], 4)]  # noqa: E731
    info = {'bones': {b.name: [conv(b.head_local), conv(b.tail_local)] for b in arm.data.bones},
            'sockets': {}, 'tris': sum(len(p.vertices) - 2 for p in body.data.polygons),
            'verts': len(body.data.vertices), 'materials': [m.name for m in body.data.materials]}
    for o in bpy.data.objects:
        if o.type == 'EMPTY' and o.name.startswith('SOCKET_'):
            m = o.matrix_world
            info['sockets'][o.name[7:]] = {'bone': o.get('Bone'), 'pos': conv(m.translation),
                                           'x': conv(m.col[0][:3]), 'y': conv(m.col[1][:3])}
    with open(path[:-4] + '.info.json', 'w') as f:
        json.dump(info, f)
    print(f'{arm["Hero"]}: {info["tris"]} triangles, {info["verts"]} vertices, {len(info["materials"])} materials, '
          f'{len(arm.data.bones)} bones, {os.path.getsize(path) // 1024} KB, {time.time() - t0:.1f} s', flush=True)
    return arm['Hero']


def assemble():
    # The engine reads the island's layout when it loads (concepts.js), but the page draws no island model, so the
    # manifest stays empty.
    viewer = os.path.join(REPO, 'Tools', 'ConceptViewer', 'web')
    layout = json.load(open(os.path.join(REPO, 'Art/Levels/TutorialIsland/layout.json')))
    computed = json.load(open(os.path.join(REPO, 'Art/Levels/TutorialIsland/layout_computed.json')))
    keep = ('cliffs', 'bridge', 'waterfall', 'pond', 'creek', 'ramp', 'orchardRows', 'footprints', 'placements')
    data = dict(layout=layout, computed={k: computed[k] for k in keep if k in computed},
                manifest={'models': {}, 'terrain': {}})
    code = '\n'.join(open(os.path.join(viewer, f)).read() for f in ('engine.js', 'kit.js', 'concepts.js', 'app.js'))
    code += '\n' + open(os.path.join(HERE, 'heroes.js')).read()
    three = 'https://cdn.jsdelivr.net/npm/three@0.160.0/'
    html = ''.join([
        open(os.path.join(HERE, 'ui.html')).read(),
        '<script>window.__DATA__ = ' + json.dumps(data, separators=(',', ':')) + ';</script>\n',
        '<script type="importmap">{"imports":{"three":"' + three + 'build/three.module.js","three/addons/":"' + three
        + 'examples/jsm/"}}</script>\n',
        '<script type="module">\n' + code + '\n</script>\n'])
    # index.html is what gets published (the artifact service adds the document skeleton); test.html carries it.
    open(os.path.join(OUT, 'index.html'), 'w').write(html)
    open(os.path.join(OUT, 'test.html'), 'w').write(
        '<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, '
        'initial-scale=1"></head><body>' + html + '</body></html>')
    print(f'index.html {len(html) / 1024:.0f} KB in {OUT}')


built = [export(n) for n in NAMES]
assemble()
print('Publish index.html with models/Hero_<Name>.json and .info.json for ' + ', '.join(built))
