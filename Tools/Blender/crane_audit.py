"""Hollis Crane's audit (Crane.py --audit <tag>), as ellis_audit.py's: every layer checked against the one over it in
the game's poses (looter_audit.POSES: idle, aim, stride, crouch, the grips), the swing chains where the game's springs
would rest them (looter_audit.settle_swing); the poses in clay; the baked look as a turntable at the idle with
close-ups; into Saved/Concepts/CraneAudit/<tag>/ (findings.json). Not in the bake's digest."""
import json
import math
import os

import bpy
import numpy as np

import looter_audit as la

# (inner, outer): the inner layer must stay under the outer one
PAIRS = (('Shirt', 'Coat'), ('Trousers', 'Coat'), ('Belt', 'Coat'), ('Boots', 'Coat'), ('Trousers', 'Boots'),
         ('Trousers', 'Belt'), ('Shirt', 'Belt'), ('Sleeves', 'Cuff'), ('Crane', 'Cuff'), ('Sleeves', 'Cape'),
         ('Coat', 'Cape'), ('Chain', 'Cape'), ('Coat', 'Collar'), ('Crane', 'ShirtBand'), ('ShirtBand', 'Collar'),
         ('Collar', 'Hair'), ('Coat', 'Sleeves'))
# (part, solid): the part must not be inside the solid
SOLIDS = (('Hook', 'Coat'), ('Hook', 'Shirt'), ('Hook', 'Trousers'), ('Chain', 'Coat'), ('Chain', 'Shirt'),
          ('Star', 'Coat'), ('Coin', 'Crane'))
GROUNDED = ('Coat', 'Cape', 'Hook')
POSES = ('rest', 'idle', 'aim', 'stride', 'crouch', 'grip_a', 'grip_b')


def parts_named(fig, prefix):
    names = ['Crane'] if prefix == 'Crane' else [n for n in fig.rest if n.split('.')[0].startswith(prefix)]
    if prefix == 'Hook':
        names = [n for n in fig.rest if n.startswith(('Hook', 'HookLinks', 'HookEye'))]
    co, faces, W, off = [], [], [], 0
    for n in names:
        c, f, w = fig.rest[n]
        co.append(c)
        faces += [[i + off for i in p] for p in f]
        W.append(w)
        off += len(c)
    if not co:
        return None
    return np.concatenate(co), faces, np.concatenate(W)


def chains(fig):
    return {c['name']: [f'{c["name"]}_{k + 1:02d}' for k in range(len(c['joints']) - 1)] for c in fig.swing}


def posed_parts(fig, arm, pose):
    la.pose(arm, la.POSES[pose])
    la.settle_swing(arm, chains(fig), la.POSES[pose])
    mats = la.skinning(arm, fig.rig_names)
    cache = {}

    def get(prefix):
        if prefix not in cache:
            p = parts_named(fig, prefix)
            cache[prefix] = None if p is None else (la.deform(p[0], p[2], mats), p[1])
        return cache[prefix]
    return get


BONES = ('pelvis', 'spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05', 'neck_01', 'neck_02', 'head') + tuple(
    f'{b}_{s}' for b in ('clavicle', 'upperarm', 'lowerarm', 'hand', 'thigh', 'calf', 'foot') for s in 'lr')


def check(fig, arm, pose):
    get = posed_parts(fig, arm, pose)
    pb = arm.pose.bones
    bones = (np.array([tuple(pb[b].head) for b in BONES]), np.array([tuple(pb[b].tail) for b in BONES]))
    found = []
    for inner, outer in PAIRS:
        a, b = get(inner), get(outer)
        if a is None or b is None:
            continue
        idx, deep = la.poke_through(a, b, outward=la.away_from_bones(a[0], bones))
        if len(idx):
            found.append(('through', inner, outer, int(len(idx)), float(deep.max() * 100),
                          [round(float(v) * 100, 1) for v in a[0][idx].mean(0)]))
    for part, solid in SOLIDS:
        a, b = get(part), get(solid)
        if a is None or b is None:
            continue
        idx, deep = la.inside(a[0], b)
        if len(idx):
            found.append(('inside', part, solid, int(len(idx)), float(deep.max() * 100),
                          [round(float(v) * 100, 1) for v in a[0][idx].mean(0)]))
    for part in GROUNDED:
        a = get(part)
        if a is None:
            continue
        idx = np.nonzero(a[0][:, 2] < 0.0)[0]
        if len(idx):
            found.append(('under', part, 'the ground', int(len(idx)), float(-a[0][idx, 2].min() * 100),
                          [round(float(v) * 100, 1) for v in a[0][idx].mean(0)]))
    return found


def run(fig, arm, skin, viewer_materials, tag, log, repo):
    out = os.path.join(repo, 'Saved', 'Concepts', 'CraneAudit', tag)
    os.makedirs(out, exist_ok=True)
    path = lambda n: os.path.join(out, n + '.png')
    report = {}
    poses = [p for p in POSES if not os.environ.get('AUDIT_POSES') or p in os.environ['AUDIT_POSES'].split('+')]
    for pose in poses:
        report[pose] = check(fig, arm, pose)
        for kind, a, b, n, deep, where in report[pose]:
            log(f'audit {pose}: {a} {kind} {b}: {n} points, {deep:.1f} cm deep at most, about {where} cm')
        if not report[pose]:
            log(f'audit {pose}: nothing through anything')
    with open(os.path.join(out, 'findings.json'), 'w') as f:
        json.dump(report, f, indent=1)
    if os.environ.get('AUDIT_NO_RENDERS'):
        la.reset(arm)
        return
    la.clay()
    bpy.context.scene.world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    bpy.context.scene.world.color = (0.30, 0.33, 0.38)
    skin.color = (0.62, 0.58, 0.53, 1.0)
    for ob in arm.children:
        if ob.name.startswith(('USP_', 'UCP_')):
            ob.hide_render = True
    views = {'front': (0.0, -4.4, 0.3), 'side': (4.4, 0.0, 0.3), 'back': (0.0, 4.4, 0.3), 'three': (2.7, -3.5, 0.5)}
    for pose in [p for p in poses if p != 'rest']:
        la.pose(arm, la.POSES[pose])
        la.settle_swing(arm, chains(fig), la.POSES[pose])
        for v, off in views.items():
            la.shoot(path(f'clay_{pose}_{v}'), (0.0, 0.0, 0.95 if pose != 'crouch' else 0.72), off, 85.0, (900, 1200))
        up = (0.0, -0.05, 1.45 if pose != 'crouch' else 1.15)
        la.shoot(path(f'clay_{pose}_chest'), up, (0.9, -1.4, 0.15), 85.0, (900, 900))
        la.shoot(path(f'clay_{pose}_shoulders_back'), up, (-0.6, 1.5, 0.35), 85.0, (900, 900))
    if os.environ.get('AUDIT_CLAY_ONLY'):
        la.reset(arm)
        return
    viewer_materials()
    la.cycles(int(os.environ.get('AUDIT_SAMPLES', 40)))
    bpy.context.scene.cycles.transparent_max_bounces = 48
    la.light('day')
    la.pose(arm, la.POSES['idle'])
    la.settle_swing(arm, chains(fig), la.POSES['idle'])
    for k in range(8):
        a = math.radians(45.0 * k)
        la.shoot(path(f'turn_{k * 45:03d}'), (0.0, 0.0, 1.0), (math.sin(a) * 6.5, -math.cos(a) * 6.5, 0.35), 85.0)
    pb = arm.pose.bones
    eye = (fig.eyes[0] + fig.eyes[1]) * 0.5
    shots = {
        'face_front': (eye, (0.0, -0.95, 0.0), 100.0, (1200, 1200)),
        'face_three': (eye, (0.6, -0.75, 0.0), 100.0, (1200, 1200)),
        'face_below_brim': (eye, (0.0, -0.8, -0.32), 100.0, (1200, 1200)),
        'hand_l_front': (tuple(pb['hand_l'].tail), (0.0, -0.75, 0.05), 100.0, (1200, 1200)),
        'hand_r_out': (tuple(pb['hand_r'].tail), (-0.75, -0.1, 0.05), 100.0, (1200, 1200)),
        'chest_chain_star': ((0.0, -0.1, 1.45), (0.25, -1.3, 0.05), 85.0, (1200, 1200)),
        'belt_hips': ((0.0, -0.1, 1.05), (0.0, -1.6, 0.15), 85.0, (1600, 1200)),
        'boots': ((0.0, -0.05, 0.28), (0.55, -1.5, 0.2), 85.0, (1600, 1200)),
        'back_coat_cape': ((0.0, 0.12, 1.05), (0.0, 3.4, 0.3), 85.0, (1200, 1600)),
        'hair_side': ((0.0, 0.0, 1.70), (1.1, 0.0, 0.0), 100.0, (1200, 1200)),
        'hair_back': ((0.0, 0.04, 1.65), (0.0, 1.2, 0.1), 100.0, (1200, 1200)),
    }
    for name, (target, off, lens, size) in shots.items():
        la.shoot(path(name), target, off, lens, size)
    la.light('dusk')
    la.shoot(path('dusk_three'), (0.0, 0.0, 1.0), (3.9, -5.1, 0.4), 85.0)
    la.reset(arm)
    log(f'audit: renders and findings in {out}')
