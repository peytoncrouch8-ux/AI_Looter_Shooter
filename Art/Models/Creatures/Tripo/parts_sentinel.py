"""SK_StoneSentinel from the user's Tripo model (2026-10-29): what StoneSentinel.py builds when LOOTER_SENTINEL_TRIPO is
on (the default; 0 builds the script stand-in). Not a model: Tools\\models.ps1 skips parts_*.py.

    build() -> (rig, body)   the shape (parts_sentinel_cut.py), its look (parts_sentinel_look.py), the rig and hit hulls
    previews(rig, body, plinth, out_dir)   the sheet and one render per game pose (parts_sentinel_poses.py)

Kept from the stand-in, as lane D's code reads them: the asset name, the 17 bones and the SOCKET_Head / FistL / FistR
bones, the rest pose (the dormant statue: hood bowed, fists clasped before the belt), about 2.37 m, a hull per bone,
LODs 50% and 25%, no Nanite. Each piece is weighted wholly to its bone (rigid stone). The bones sit on the joints'
pivots (the cuts' balls and drums are centred on them), so the game's turns slide the pieces on their seams.
"""
import math
import os
import runpy

import bpy
from mathutils import Quaternion, Vector

import looter_barrow as lb
import looter_model as lm
import looter_textures as lt

HERE = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Tripo')
CUT = runpy.run_path(os.path.join(HERE, 'parts_sentinel_cut.py'))
LOOK = runpy.run_path(os.path.join(HERE, 'parts_sentinel_look.py'))
POSES = runpy.run_path(os.path.join(HERE, 'parts_sentinel_poses.py'))
SOURCES = [CUT['GLB']] + [os.path.join(HERE, f) for f in ('parts_sentinel.py', 'parts_sentinel_cut.py',
                                                         'parts_sentinel_look.py')]
FACE = Vector((0.0, -0.24, 1.93))          # the face in the hood (Tripo's stance): SOCKET_Head, the crit
log = CUT['log']


def mirror(v, s):
    return Vector((v[0] * s, v[1], v[2]))


def bones(xf, joints):
    """(name, head, tail, parent) in Blender m, the pose's pivots."""
    C = CUT
    neck = C['NECK']
    up = xf['head'].to_quaternion() @ Vector((0.0, -0.25, 0.97))
    out = [
        ('pelvis', Vector((0.0, 0.0, 1.10)), C['WAIST'].copy(), None),
        ('spine_01', C['WAIST'].copy(), C['CHEST'].copy(), 'pelvis'),
        ('spine_02', C['CHEST'].copy(), neck.copy(), 'spine_01'),
        ('head', neck.copy(), neck + up * 0.32, 'spine_02'),
    ]
    for s, sfx in ((1.0, 'l'), (-1.0, 'r')):
        hip = Vector((0.14 * s, C['HIP_Y'], C['HIP_Z']))
        knee = Vector((0.27 * s, C['KNEE_Y'], C['KNEE_Z']))
        ankle = mirror(C['ANKLE'], s)
        out += [
            (f'upperarm_{sfx}', joints[f'shoulder_{sfx}'], joints[f'elbow_{sfx}'], 'spine_02'),
            (f'lowerarm_{sfx}', joints[f'elbow_{sfx}'], joints[f'wrist_{sfx}'], f'upperarm_{sfx}'),
            (f'hand_{sfx}', joints[f'wrist_{sfx}'], joints[f'fist_{sfx}'], f'lowerarm_{sfx}'),
            (f'thigh_{sfx}', hip, knee, 'pelvis'),
            (f'calf_{sfx}', knee, ankle, f'thigh_{sfx}'),
            (f'foot_{sfx}', ankle, Vector((0.42 * s, -0.40, 0.03)), f'calf_{sfx}'),
        ]
    face = xf['head'] @ FACE
    fwd = (xf['head'].to_quaternion() @ Vector((0.0, -1.0, 0.0))) * 0.05
    out += [('SOCKET_Head', face, face + fwd, 'head')]
    for s, sfx, name in ((1.0, 'l', 'SOCKET_FistL'), (-1.0, 'r', 'SOCKET_FistR')):
        fist = joints[f'fist_{sfx}']
        out.append((name, fist, fist + Vector((0.0, -0.05, 0.0)), f'hand_{sfx}'))
    return out


def build():
    C, L = CUT, LOOK
    src, maps = C['load']()
    whole = C['retopo'](src)
    high = C['cut'](src, 'P')
    low = C['cut'](whole, 'L')
    C['tidy_caps'](low)
    L['staples'](low)
    xf, joints = C['rest_pose']()
    C['pose'](high, xf)
    C['pose'](low, xf)
    specs = bones(xf, joints)
    rig = lm.armature()
    lm.bones(rig, [(n, h.copy(), t.copy(), p) for n, h, t, p in specs])
    for bone in C['BONES']:
        lm.hit_hull(rig, bone, [low[bone]])
    body = lm.skin(rig, 'StoneSentinel', {bone: [low[bone]] for bone in C['BONES']})
    L['record_staples'](body)
    digest = L['unwrap'](body)
    if L['cached'](digest, SOURCES):
        log('look: the baked set fits this atlas (uv.digest) and is newer than its sources: kept')
        mat = lt.material(L['SET'], name=L['MATERIAL'], master='World')
    else:
        baked = L['bake'](body, list(high.values()), maps)
        look = L['compose'](baked, L['granite'](body))
        mat = L['write'](look, digest)
    L['finish'](body, mat)
    for p in body.data.polygons:
        p.use_smooth = True
    body.data.set_sharp_from_angle(angle=math.radians(60.0))
    lt.bake_vertex_ao(body, samples=24, distance=0.45)
    for obj in high.values():
        me = obj.data
        bpy.data.objects.remove(obj)
        if me and not me.users:
            bpy.data.meshes.remove(me)
    rig['LODs'] = '50,25'
    rig['LODScreens'] = '0.35,0.15'
    report(rig, body, specs)
    return rig, body


def report(rig, body, specs):
    tris = sum(len(p.vertices) - 2 for p in body.data.polygons)
    lo, hi = lt._bounds([body])
    hulls = [o for o in rig.children if o.name.startswith('UCX_')]
    log(f'SK_StoneSentinel (Tripo): {tris} triangles, {len(rig.data.bones)} bones, {len(hulls)} hulls, height '
        f'{hi.z:.2f} m, width {hi.x - lo.x:.2f} m, depth {hi.y - lo.y:.2f} m; materials '
        f'{", ".join(m.name for m in body.data.materials)}')
    for name, h, t, parent in specs:
        # Blender m to Unreal component cm (x forward, y right).
        log(f'bone {name} (under {parent or "root"}) at ({-h.y * 100:.1f}, {-h.x * 100:.1f}, {h.z * 100:.1f}) cm')


# --- Check renders ---

def previews(rig, body, plinth, out_dir, top):
    """The sheet (front, three-quarter, side, back, close, distance) on the plinth, and one render per game pose."""
    os.makedirs(out_dir, exist_ok=True)
    rig.location = (0.0, 0.0, top)
    bpy.context.view_layer.update()
    scale = lm.cylinder('_Figure', 0.16, 1.8, center=(1.35, 0.4, 0.9), segments=16,
                        material=lm.material('_FigureGrey', 0x8a8f96))
    shown = [body, plinth]
    target = (0.0, 0.0, 1.45)
    sun = (-0.55, -0.75, 0.62)
    shots = []
    for key, cam, aim, lens, objs in (
            ('front', (0.0, -7.0, 1.7), target, 62.0, shown),
            ('three_quarter', (-4.6, -5.4, 2.0), target, 62.0, shown + [scale]),
            ('side', (-7.0, 0.0, 1.7), target, 62.0, shown),
            ('back', (2.0, 6.8, 2.2), target, 62.0, shown),
            ('close', (0.9, -2.4, 2.35), (0.0, -0.15, 1.75), 45.0, shown),
            ('distance', (-6.0, -16.0, 1.7), (0.0, 0.0, 1.3), 50.0, shown + [scale])):
        shots.append(lb.render(objs, os.path.join(out_dir, f'StoneSentinel_{key}.png'), cam, aim, lens=lens,
                               size=(900, 1000), ground=0x8f8a72, sun_dir=sun))
    lb.sheet(shots, os.path.join(out_dir, 'StoneSentinel_sheet.png'), cols=3)
    rig.location = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    # The game's poses, off the plinth (it rises out of it awake), from the front three-quarter and the side.
    specs = [(b.name, b.parent.name if b.parent else None, b.head_local.copy()) for b in rig.data.bones]
    shots = []
    for name, (weight, walking, phase, arm_pose, clock) in POSES['POSES'].items():
        mats = POSES['pose'](specs, weight, walking, phase, arm_pose, clock)
        posed = POSES['posed_copy'](body, specs, mats, f'_Pose_{name}')
        for key, cam in (('q', (-2.9, -4.2, 1.9)), ('side', (-5.0, -0.3, 1.5))):
            shots.append(lb.render([posed], os.path.join(out_dir, f'StoneSentinel_pose_{name}_{key}.png'), cam,
                                   (0.0, -0.1, 1.25), lens=50.0, size=(700, 800), ground=0x8f8a72, sun_dir=sun))
        me = posed.data
        bpy.data.objects.remove(posed)
        bpy.data.meshes.remove(me)
    lb.sheet(shots, os.path.join(out_dir, 'StoneSentinel_poses.png'), cols=4)
