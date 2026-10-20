"""Odessa's audit (Odessa.py --audit <tag>; ellis_audit.py's for her layers): every layer checked against the one over it
in the game's poses (looter_audit.POSES: idle, aim, stride, crouch, the grips), the tails' chains resting where the
game's springs would (looter_audit.settle_swing); clay renders of the poses and the baked look in daylight (a
turntable at the idle, close-ups, dusk), into Saved/Concepts/OdessaAudit/<tag>/ with findings.json. Not in the bake's
digest."""
import json
import math
import os

import bpy
import numpy as np

import looter_audit as la

# (inner, outer): the inner layer must stay under the outer one
PAIRS = (('Shirt', 'Vest'), ('Shirt', 'CravatFall'), ('Vest', 'CravatFall'), ('Shirt', 'Coat'), ('Vest', 'Coat'),
         ('Belt', 'CoatTails'), ('Trousers', 'CoatTails'), ('Trousers', 'Boots'), ('Boots', 'CoatTails'),
         ('CoatTails', 'Coat'), ('Trousers', 'Belt'), ('Vest', 'Belt'), ('Coat', 'CoatCuff'), ('ShirtCuff', 'Coat'),
         ('Gauntlet', 'ShirtCuff'), ('Gloves', 'Gauntlet'), ('Odessa', 'Collar'), ('Collar', 'CravatBand'),
         ('Collar', 'CoatCollar'), ('Coat', 'Lapel'), ('Coat', 'CoatCollar'), ('Odessa', 'Hair'),
         ('Hair', 'HatBandInner'), ('Trousers', 'CardCase'))
# (part, solid): the part must not be inside the solid
SOLIDS = (('CardCase', 'Trousers'), ('Gloves', 'Trousers'), ('Gloves', 'CoatTails'), ('Gloves', 'CardCase'),
          ('Knot', 'Collar'), ('Knot', 'CoatCollar'), ('EarDrop', 'Collar'))
GROUNDED = ('CoatTails',)
POSES = ('rest', 'idle', 'aim', 'stride', 'crouch', 'grip_a', 'grip_b')


def parts_named(fig, prefix):
    names = ['Odessa'] if prefix == 'Odessa' else [n for n in fig.rest if n.split('.')[0].startswith(prefix)]
    if prefix == 'Coat':
        names = [n for n in names if n.split('.')[0] == 'Coat']
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


def run(fig, arm, skin, baked, tag, viewer_materials, log, repo):
    out = os.path.join(repo, 'Saved', 'Concepts', 'OdessaAudit', tag)
    os.makedirs(out, exist_ok=True)
    path = lambda n: os.path.join(out, n + '.png')
    report = {}
    for pose in POSES:
        report[pose] = check(fig, arm, pose)
        for kind, a, b, n, deep, where in report[pose]:
            log(f'audit {pose}: {a} {kind} {b}: {n} points, {deep:.1f} cm deep at most, about {where} cm')
    with open(os.path.join(out, 'findings.json'), 'w') as f:
        json.dump(report, f, indent=1)
    la.clay()
    bpy.context.scene.world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    bpy.context.scene.world.color = (0.30, 0.33, 0.38)
    skin.color = (0.62, 0.58, 0.53, 1.0)
    for ob in arm.children:
        if ob.name.startswith(('USP_', 'UCP_')):
            ob.hide_render = True
    views = {'front': (0.0, -4.2, 0.3), 'side': (4.2, 0.0, 0.3), 'back': (0.0, 4.2, 0.3), 'three': (2.6, -3.3, 0.5)}
    for pose in POSES[1:]:
        la.pose(arm, la.POSES[pose])
        la.settle_swing(arm, chains(fig), la.POSES[pose])
        for v, off in views.items():
            la.shoot(path(f'clay_{pose}_{v}'), (0.0, 0.0, 0.85 if pose != 'crouch' else 0.65), off, 85.0, (900, 1200))
        hips = (0.0, -0.05, 0.85 if pose != 'crouch' else 0.62)
        la.shoot(path(f'clay_{pose}_hips_right'), hips, (-1.6, -0.6, 0.1), 85.0, (900, 900))
    if os.environ.get('AUDIT_CLAY_ONLY') or baked is None:
        la.reset(arm)
        return
    viewer_materials(skin, baked)
    la.cycles(int(os.environ.get('AUDIT_SAMPLES', 40)))
    la.light('day')
    la.pose(arm, la.POSES['idle'])
    la.settle_swing(arm, chains(fig), la.POSES['idle'])
    for k in range(8):
        a = math.radians(45.0 * k)
        la.shoot(path(f'turn_{k * 45:03d}'), (0.0, 0.0, 0.92), (math.sin(a) * 6.0, -math.cos(a) * 6.0, 0.35), 85.0)
    pb = arm.pose.bones
    eye = (fig.eyes[0] + fig.eyes[1]) * 0.5
    shots = {
        'face_front': (eye, (0.0, -0.95, 0.0), 100.0, (1200, 1200)),
        'face_three': (eye, (0.45, -0.85, 0.05), 100.0, (1200, 1200)),
        'face_below_brim': (eye, (0.0, -0.8, -0.32), 100.0, (1200, 1200)),
        'hand_l_out': (tuple(pb['hand_l'].tail), (0.75, -0.1, 0.05), 100.0, (1200, 1200)),
        'hand_r_front': (tuple(pb['hand_r'].tail), (0.0, -0.75, 0.05), 100.0, (1200, 1200)),
        'waistcoat_cravat': ((0.0, -0.12, 1.30), (0.12, -0.85, 0.04), 85.0, (1200, 1200)),
        'hips_case': ((-0.08, -0.08, 0.92), (-0.4, -1.5, 0.15), 85.0, (1600, 1200)),
        'boots': ((0.0, -0.05, 0.28), (0.55, -1.5, 0.2), 85.0, (1600, 1200)),
        'back_tails': ((0.0, 0.12, 0.85), (0.0, 3.0, 0.3), 85.0, (1200, 1600)),
        'hat_back_knot': ((0.0, 0.05, 1.62), (0.4, 1.0, 0.12), 100.0, (1200, 1200)),
        'hat_side_card': ((0.05, 0.0, 1.70), (1.0, -0.2, 0.08), 100.0, (1200, 1200)),
    }
    for name, (target, off, lens, size) in shots.items():
        la.shoot(path(name), target, off, lens, size)
    la.light('dusk')
    la.shoot(path('dusk_three'), (0.0, 0.0, 0.92), (3.8, -4.9, 0.4), 85.0)
    la.reset(arm)
    log(f'audit: renders and findings in {out}')
