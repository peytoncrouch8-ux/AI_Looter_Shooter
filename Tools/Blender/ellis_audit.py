"""Ellis's audit (Ellis.py --audit <tag>; the user's "the model has to be perfect", 2026-10-15): the baked figure in
daylight as a turntable and close-ups (and one dusk shot), the game's poses in clay, and every layer checked against
the one over it in each pose, into Saved/Concepts/EllisAudit/<tag>/ (findings.txt lists what pokes through where).
The swing chains rest where the game's springs would (looter_audit.settle_swing). Not in the bake's digest."""
import json
import math
import os

import bpy
import numpy as np

import looter_audit as la

# (inner, outer): the inner layer must stay under the outer one
PAIRS = (('Shirt', 'Vest'), ('Shirt', 'Bandana'), ('Vest', 'Bandana'), ('Shirt', 'Duster'), ('Vest', 'Duster'),
         ('Belt', 'Duster'), ('Trousers', 'Duster'), ('Trousers', 'Boots'), ('Boots', 'Duster'), ('Trousers', 'Belt'),
         ('Vest', 'Belt'), ('Sleeves', 'Gauntlet'), ('Gloves', 'Gauntlet'), ('Sleeves', 'Cape'), ('Duster', 'Cape'),
         ('Duster', 'Collar'), ('Cape', 'Collar'), ('Ellis', 'Collar'), ('Ellis', 'Bandana'), ('Bandana', 'Collar'),
         ('Hair', 'Collar'), ('Trousers', 'HolsterSkirt'), ('Duster', 'Sleeves'))
# (part, solid): the part must not be inside the solid
SOLIDS = (('Lantern', 'Trousers'), ('Lantern', 'Duster'), ('Holster', 'Trousers'), ('Revolver', 'Trousers'),
          ('Gloves', 'Trousers'), ('Gloves', 'Duster'))
# parts that must stay over the ground (z 0: his soles), as BodySwing's floor keeps the swing points (crouched, the
# ankle-length hem went 11 cm under it before the floor, 2026-10-15)
GROUNDED = ('Duster', 'Cape', 'Lantern')
POSES = ('rest', 'idle', 'aim', 'stride', 'crouch')


def parts_named(fig, prefix):
    """(points, faces, weights) of every part whose name starts with prefix, joined."""
    names = ['Ellis'] if prefix == 'Ellis' else [n for n in fig.rest if n.split('.')[0].startswith(prefix)]
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
    out = {}
    for c in fig.swing:
        out[c['name']] = [f'{c["name"]}_{k + 1:02d}' for k in range(len(c['joints']) - 1)]
    return out


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
    """Every PAIRS and SOLIDS finding in a pose: [(kind, a, b, points, deepest cm, where cm)]."""
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


def option_views(fig, arm, skin, repo, name, log):
    """A silhouette option (ellis_options) in clay at the game's idle, front, back, side and three-quarter (the game's
    view is from behind): Saved/Concepts/EllisOptions/<name>_<view>.png."""
    out = os.path.join(repo, 'Saved', 'Concepts', 'EllisOptions')
    os.makedirs(out, exist_ok=True)
    la.clay()
    bpy.context.scene.world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    bpy.context.scene.world.color = (0.30, 0.33, 0.38)
    skin.color = (0.62, 0.58, 0.53, 1.0)
    for ob in arm.children:
        if ob.name.startswith(('USP_', 'UCP_')):
            ob.hide_render = True
    la.pose(arm, la.POSES['idle'])
    la.settle_swing(arm, chains(fig), la.POSES['idle'])
    views = {'front': (0.0, -4.4, 0.3), 'back': (0.0, 4.4, 0.4), 'side': (4.4, 0.0, 0.3), 'three': (3.0, 3.2, 0.5)}
    for v, off in views.items():
        la.shoot(os.path.join(out, f'{name}_{v}.png'), (0.0, 0.0, 0.92), off, 85.0, (1200, 1600))
    tris = sum(len(p.vertices) - 2 for p in skin.data.polygons)
    log(f'option {name}: {tris} triangles; renders in {out}')
    la.reset(arm)


def run(fig, arm, skin, baked, tag, viewer_materials, log, repo):
    out = os.path.join(repo, 'Saved', 'Concepts', 'EllisAudit', tag)
    os.makedirs(out, exist_ok=True)
    path = lambda n: os.path.join(out, n + '.png')
    sk = fig.sk
    # --- the clearance checks, every pose
    report = {}
    for pose in POSES:
        report[pose] = check(fig, arm, pose)
        for kind, a, b, n, deep, where in report[pose]:
            log(f'audit {pose}: {a} {kind} {b}: {n} points, {deep:.1f} cm deep at most, about {where} cm')
    with open(os.path.join(out, 'findings.json'), 'w') as f:
        json.dump(report, f, indent=1)
    # --- the game's poses in clay
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
        # the hips close: holster, lantern, the hands by the coat
        hips = (0.0, -0.05, 0.85 if pose != 'crouch' else 0.62)
        la.shoot(path(f'clay_{pose}_hips_right'), hips, (-1.6, -0.6, 0.1), 85.0, (900, 900))
        la.shoot(path(f'clay_{pose}_hips_left'), hips, (1.6, -0.6, 0.1), 85.0, (900, 900))
    if os.environ.get('AUDIT_CLAY_ONLY'):
        la.reset(arm)
        return
    # --- the baked look: the turntable at the game's idle, close-ups, dusk
    viewer_materials(skin, baked)
    la.cycles(int(os.environ.get('AUDIT_SAMPLES', 40)))
    la.light('day')
    la.pose(arm, la.POSES['idle'])
    la.settle_swing(arm, chains(fig), la.POSES['idle'])
    for k in range(8):
        a = math.radians(45.0 * k)
        la.shoot(path(f'turn_{k * 45:03d}'), (0.0, 0.0, 0.93), (math.sin(a) * 6.2, -math.cos(a) * 6.2, 0.35), 85.0)
    pb = arm.pose.bones
    eye = (fig.eyes[0] + fig.eyes[1]) * 0.5
    shots = {
        'face_front': (eye, (0.0, -0.95, 0.0), 100.0, (1200, 1200)),
        'face_below_brim': (eye, (0.0, -0.8, -0.32), 100.0, (1200, 1200)),
        'hand_l_front': (tuple(pb['hand_l'].tail), (0.0, -0.75, 0.05), 100.0, (1200, 1200)),
        'hand_l_out': (tuple(pb['hand_l'].tail), (0.75, -0.1, 0.05), 100.0, (1200, 1200)),
        'hand_r_front': (tuple(pb['hand_r'].tail), (0.0, -0.75, 0.05), 100.0, (1200, 1200)),
        'hand_r_out': (tuple(pb['hand_r'].tail), (-0.75, -0.1, 0.05), 100.0, (1200, 1200)),
        'belt_holster_lantern': ((0.0, -0.1, 0.9), (0.0, -1.6, 0.15), 85.0, (1600, 1200)),
        'boots': ((0.0, -0.05, 0.26), (0.55, -1.5, 0.2), 85.0, (1600, 1200)),
        'boots_side': ((0.12, -0.08, 0.15), (1.1, -0.25, 0.1), 85.0, (1600, 1200)),
        'waistcoat': ((0.0, -0.13, 1.22), (0.12, -0.85, 0.04), 85.0, (1200, 1200)),
        'back_vent_cape': ((0.0, 0.12, 1.0), (0.0, 3.2, 0.3), 85.0, (1200, 1600)),
        'collar_pin': ((0.06, -0.08, 1.56), (0.25, -0.85, 0.05), 100.0, (1200, 1200)),
        'hair_side': ((0.0, 0.0, 1.66), (1.0, 0.0, 0.0), 100.0, (1200, 1200)),
        'hair_back': ((0.0, 0.03, 1.66), (0.0, 1.0, 0.05), 100.0, (1200, 1200)),
    }
    for name, (target, off, lens, size) in shots.items():
        la.shoot(path(name), target, off, lens, size)
    la.light('dusk')
    la.shoot(path('dusk_three'), (0.0, 0.0, 0.93), (3.8, -4.9, 0.4), 85.0)
    la.reset(arm)
    log(f'audit: renders and findings in {out}')
