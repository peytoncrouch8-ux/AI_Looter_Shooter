"""Checks the Style Lab export (Saved/StyleLab/export) against the contract: every pack loads (GLB parse) and is at most
14 MB, its top-level nodes are exactly the manifest's models in it, every material name in any GLB has a manifest record,
every texture path exists, every scene model is in the manifest, the heights file has the right size, and Main
Street's buildings line both sides of the street. Prints the town's bounding boxes in three metres. Exit code 1 on a
failure.

    /home/user/bpyenv/bin/python Tools/StyleLab/export/check.py [--blender]   (--blender also imports each pack with
                                                                               Blender's glTF importer)
"""
import json
import math
import os
import sys

import numpy as np

sys.dont_write_bytecode = True
import labcommon as lc  # noqa: E402
import glbtool

OUT = lc.OUT
failures = []


def fail(message):
    failures.append(message)
    print(f'CHECK FAIL: {message}', flush=True)


def check_textures(manifest):
    """The maps a browser would spoil: no terrain map has an alpha channel (premultiplied decoding loses the colour
    under alpha 0); every cut-out set's BC decodes to the source's colour under alpha 0 (within 3 levels, RGB and alpha
    resized apart), so mip-mapped alpha tests don't fringe dark; the macro map's colour under the source's alpha 0
    matches the source."""
    from PIL import Image
    tex = os.path.join(lc.REPO, 'Art', 'Textures')
    for rel in sorted(f for f in os.listdir(os.path.join(OUT, 'tex')) if f.startswith(('Terrain_', 'MacroNoise'))):
        mode = Image.open(os.path.join(OUT, 'tex', rel)).mode
        if mode not in ('RGB', 'L'):
            fail(f'tex/{rel} is {mode}: terrain maps carry no alpha')
    for name, s in sorted(manifest['textureSets'].items()):
        if not s.get('alpha'):
            continue
        src = Image.open(os.path.join(tex, name, f'T_{name}_BC.png')).convert('RGBA')
        out = Image.open(os.path.join(OUT, s['bc']))
        if out.mode != 'RGBA':
            fail(f'{s["bc"]} lost its alpha ({out.mode})')
            continue
        size = out.size
        rgb = np.asarray(src.convert('RGB').resize(size, Image.LANCZOS), np.float32)
        alpha = np.asarray(src.getchannel('A').resize(size, Image.LANCZOS))
        got = np.asarray(out, np.float32)
        clear = alpha < 8
        if clear.any():
            diff = np.abs(got[..., :3][clear] - rgb[clear]).mean()
            print(f'CHECK: {s["bc"]}: colour under alpha 0 off the source by {diff:.2f} levels '
                  f'(source mean {rgb[clear].mean(0).round(0)}, decoded {got[..., :3][clear].mean(0).round(0)})',
                  flush=True)
            if diff > 3.0:
                fail(f'{s["bc"]}: the colour under alpha 0 is {diff:.1f} levels off the source')
    src = Image.open(os.path.join(tex, 'RansomsRestMacro', 'T_RansomsRestMacro_BC.png')).convert('RGBA')
    out = np.asarray(Image.open(os.path.join(OUT, manifest['terrain']['macro'])), np.float32)[::-1, ::-1]
    n = out.shape[0]
    rgb = np.asarray(src.convert('RGB').resize((n, n), Image.LANCZOS), np.float32)
    alpha = np.asarray(src.getchannel('A').resize((n, n), Image.LANCZOS))
    clear = alpha < 8
    diff = np.abs(out[clear] - rgb[clear]).mean(0)
    print(f'CHECK: Terrain_Macro under the source\'s alpha 0: mean {out[clear].mean(0).round(0)} vs source '
          f'{rgb[clear].mean(0).round(0)}', flush=True)
    if diff.max() > 6.0:
        fail(f'Terrain_Macro\'s colour under alpha 0 is off the source by {diff.round(1)}')


def main():
    manifest = json.load(open(os.path.join(OUT, 'manifest.json')))
    scene = json.load(open(os.path.join(OUT, 'scene.json')))
    mats = set(manifest['materials'])
    glb_files = list(manifest['packs']) + [manifest['terrain']['near']] + list(manifest['terrain']['far'])
    in_pack = {}
    for i, rel in enumerate(glb_files):
        path = os.path.join(OUT, rel)
        size = os.path.getsize(path)
        if size > 14 * 2 ** 20:
            fail(f'{rel} is {size / 2 ** 20:.1f} MB')
        roots, materials, gltf, binary = glbtool.summary(path)
        for m in materials:
            if m not in mats:
                fail(f'{rel}: material {m} has no manifest record')
        for acc in gltf['accessors']:
            view = gltf['bufferViews'][acc['bufferView']]
            if view.get('byteOffset', 0) + view['byteLength'] > len(binary) + 3:
                fail(f'{rel}: a buffer view runs past the binary chunk')
                break
        if rel in manifest['packs']:
            for name in roots:
                in_pack[name] = i
        print(f'CHECK: {rel}: {len(roots)} nodes, {len(materials)} materials, {size / 2 ** 20:.2f} MB', flush=True)
    for name, model in manifest['models'].items():
        if in_pack.get(name) != model['pack']:
            fail(f'model {name}: manifest says pack {model["pack"]}, found in {in_pack.get(name)}')
    for name in in_pack:
        if name not in manifest['models']:
            fail(f'{name} is in a pack but not in the manifest')
    for name, m in manifest['materials'].items():
        if m.get('set') and m['set'] not in manifest['textureSets']:
            fail(f'material {name}: set {m["set"]} not in textureSets')
    for name, s in manifest['textureSets'].items():
        for kind in ('bc', 'n', 'orm'):
            if kind in s and not os.path.exists(os.path.join(OUT, s[kind])):
                fail(f'texture {s[kind]} missing')
    t = manifest['terrain']
    for key in ('macro', 'macroSelect', 'masks', 'masksA', 'ringMacro', 'ringMacroSelect'):
        if not os.path.exists(os.path.join(OUT, t[key])):
            fail(f'terrain {key} {t[key]} missing')
    check_textures(manifest)
    h = t['heights']
    if os.path.getsize(os.path.join(OUT, h['file'])) != h['w'] * h['h'] * 4:
        fail('heights.bin has the wrong size')
    names = set(manifest['models'])
    for inst in scene['instances']:
        if inst['model'] not in names:
            fail(f'scene instance {inst["id"]}: model {inst["model"]} not in the manifest')
    for c in scene['creatures']:
        if c['model'] not in names:
            fail(f'creature {c}: model not in the manifest')
    for rule in scene['scatter']:
        for m in rule['models']:
            if m not in names:
                fail(f'scatter {rule["name"]}: model {m} not in the manifest')
    files = sum(len(fs) for _, _, fs in os.walk(OUT))
    print(f'CHECK: {files} files under export/', flush=True)

    # Main Street: the town's buildings, bounding boxes in three metres.
    town = ('cabinNorthWest', 'sheriff', 'cottageNorth', 'saloon', 'cabinNorth', 'cottageNorthEast',
            'cottageSouthWest', 'cabinSouth', 'store', 'cottageSouth', 'undertaker', 'chapel')
    for inst in scene['instances']:
        if inst['id'] not in town:
            continue
        b = np.array(manifest['models'][inst['model']]['bounds'])
        corners = np.array([[x, y, z] for x in b[:, 0] for y in b[:, 1] for z in b[:, 2]]) * np.array(inst['s'])
        ry = inst['r'][1]
        c, s = math.cos(ry), math.sin(ry)
        rot = np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])
        w = corners @ rot.T + np.array(inst['p'])
        lo, hi = w.min(0), w.max(0)
        print(f'CHECK: {inst["id"]:18s} {inst["model"]:22s} x {lo[0]:7.2f}..{hi[0]:7.2f}  y {lo[1]:6.2f}..{hi[1]:6.2f}  '
              f'z {lo[2]:7.2f}..{hi[2]:7.2f}', flush=True)
        if inst['id'] != 'chapel':
            side = 1 if inst['p'][2] > 0 else -1
            if not (-75.0 <= inst['p'][0] <= 1.0 and 10.0 <= abs(inst['p'][2]) <= 16.0):
                fail(f'{inst["id"]} is not on Main Street ({inst["p"]})')
            # Its street face toward the road (the near edge of its box within a few metres of the street's middle).
            near = lo[2] if side > 0 else -hi[2]
            if not 3.0 <= near <= 12.0:
                fail(f'{inst["id"]}: its street face is {near:.1f} m from the street\'s middle')
    chapel = next(i for i in scene['instances'] if i['id'] == 'chapel')
    if not (abs(chapel['p'][0] - 19.0) < 0.01 and abs(chapel['p'][2] - 66.0) < 0.01
            and abs(abs(chapel['r'][1]) - math.pi) < 1e-3):
        fail(f'the chapel is at {chapel["p"]} {chapel["r"]}, not x 19, z 66, rotation.y -pi')
    if '--blender' in sys.argv:
        import bpy
        for rel in glb_files:
            bpy.ops.wm.read_factory_settings(use_empty=True)
            bpy.ops.import_scene.gltf(filepath=os.path.join(OUT, rel))
            # (The importer adds an Icosphere per rig as its bones' display shape: not part of the file.)
            tops = sorted(o.name for o in bpy.context.scene.objects if o.parent is None
                          and not o.name.startswith('Icosphere'))
            print(f'CHECK: Blender imported {rel}: {len(tops)} top-level objects', flush=True)
            if rel in manifest['packs']:
                want = sorted(n for n, m in manifest['models'].items() if manifest['packs'][m['pack']] == rel)
                if tops != want:
                    fail(f'{rel}: Blender sees {len(tops)} top-level objects, the manifest {len(want)}: '
                         f'{sorted(set(tops) ^ set(want))[:10]}')
    print(f'CHECK: {"PASS" if not failures else f"{len(failures)} FAILURES"}', flush=True)
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
