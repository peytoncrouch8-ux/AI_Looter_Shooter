"""Hollis Crane's audit (Crane.py --audit <tag>), as ellis_audit.py's: every layer checked against the one over it in
the game's poses (looter_audit.POSES: idle, aim, stride, crouch, the grips), the swing chains where the game's springs
would rest them (looter_audit.settle_swing); the poses in clay; the baked look as a turntable at the idle with
close-ups; into Saved/Concepts/CraneAudit/<tag>/ (findings.json). Not in the bake's digest."""
import json
import math
import os

import bpy
import numpy as np
from mathutils import Vector

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


# One-sided close-ups (back faces culled, as Unreal draws them), each part its own colour (crane_rig.part_tints), of
# what the polish round fixes: (name, pose, target, camera offset, lens)
FIX_VIEWS = (
    ('neck_back', 'idle', (0.0, 0.02, 1.66), (0.30, 0.85, 0.30), 70.0),
    ('neck_back_aim', 'aim', (0.0, 0.02, 1.64), (0.35, 0.85, 0.35), 70.0),
    ('armpit_l', 'idle', (0.16, -0.04, 1.32), (0.75, -0.75, 0.05), 70.0),
    ('armpit_r', 'idle', (-0.16, -0.04, 1.32), (-0.75, -0.75, 0.05), 70.0),
    ('waist_l', 'idle', (0.21, 0.0, 1.15), (1.0, -0.35, 0.05), 70.0),
    ('aim_side', 'aim', (0.0, -0.05, 1.35), (1.3, -0.6, 0.15), 60.0),
    ('chain', 'idle', (0.02, -0.14, 1.38), (0.25, -0.80, 0.05), 70.0),
    ('chain_back', 'idle', (0.05, 0.13, 1.30), (0.30, 0.85, 0.05), 70.0),
    ('back_seat', 'idle', (0.0, 0.12, 0.85), (0.0, 1.9, 0.10), 60.0),
    ('hem_crouch', 'crouch', (0.0, 0.0, 0.18), (1.1, -1.4, 0.25), 60.0),
    ('neck_top', 'idle', (0.0, 0.0, 1.66), (0.05, 0.45, 0.55), 70.0),
    ('armpit_l_back', 'idle', (0.17, 0.02, 1.30), (0.70, 0.75, -0.10), 70.0),
    ('armpit_l_aim', 'aim', (0.17, -0.02, 1.30), (0.85, 0.25, -0.35), 70.0),
    ('armpit_r_aim', 'aim', (-0.17, -0.02, 1.30), (-0.85, 0.25, -0.35), 70.0),
    ('armpit_l_below_aim', 'aim', (0.15, -0.08, 1.40), (0.30, -0.25, -0.60), 70.0),
    ('armpit_r_below_aim', 'aim', (-0.15, -0.08, 1.40), (-0.30, -0.25, -0.60), 70.0),
    ('hip_l_front', 'idle', (0.19, -0.10, 1.08), (0.35, -0.85, 0.05), 70.0),
)


# the same, the hair taken away
NO_HAIR_VIEWS = (
    ('neck_back_nohair', 'idle', (0.0, 0.0, 1.66), (0.0, 0.55, 0.25), 70.0),
    ('neck_top_nohair', 'idle', (0.0, 0.0, 1.66), (0.0, 0.35, 0.55), 70.0),
    ('neck_back_aim_nohair', 'aim', (0.0, 0.0, 1.64), (0.15, 0.55, 0.30), 70.0),
)


DIAG_PAIRS = (('through', 'Coat', 'Cape'), ('through', 'Coat', 'Collar'), ('through', 'Coat', 'Sleeves'),
              ('inside', 'Chain', 'Coat'), ('inside', 'Star', 'Coat'), ('under', 'Coat', None))


def exposed(trees, points, n=48):
    """How many of points a look from outside meets first: rays from n directions round them (none from under the
    ground), each seeing a point when nothing lies on its way in to within 4 mm of it."""
    k = np.arange(n) + 0.5
    z = 1.0 - 1.6 * k / n                       # down to 37 degrees under level
    r = np.sqrt(np.maximum(1.0 - z * z, 0.0))
    a = k * math.pi * (3.0 - math.sqrt(5.0))
    dirs = np.stack([r * np.cos(a), r * np.sin(a), z], 1)
    seen, best = 0, 0
    for p in points:
        count = 0
        for d in dirs:
            o = Vector(tuple(p + d * 1.5))
            dv = Vector(tuple(-d))
            if all(t.ray_cast(o, dv, 1.5 - 0.004)[0] is None for t in trees):
                count += 1
        seen += count > 0
        best = max(best, count)
    return f'; seen from outside: {seen} of {len(points)} (from up to {best} of {n} directions)'


def neck_radii(fig, log):
    """(diag) Round the back of the neck at the bind pose: how far out from the neck's axis each layer first meets a
    level ray (cm), by height."""
    parts = {}
    for key in ('Crane', 'ShirtBand', 'Coat', 'Cape', 'Collar'):
        p = parts_named(fig, key)
        if p is not None:
            parts[key] = la.tree(p[0], p[1])
    neck = np.array(fig.neck)
    for z in (0.020, 0.010, 0.002, -0.006, -0.012, -0.018, -0.022, -0.030, -0.040, -0.055):
        row = []
        for deg in (90, 120, 150, 180, 210, 240, 270):
            a = math.radians(deg)
            d = Vector((math.sin(a), -math.cos(a), 0.0))
            o = Vector((neck[0], neck[1], neck[2] + z))
            cells = []
            for key, t in parts.items():
                hit = t.ray_cast(o, d, 0.4)
                cells.append(f'{key[:2]}{hit[3] * 100:4.1f}' if hit[0] is not None else f'{key[:2]}  - ')
            row.append(f'{deg:3d}: ' + ' '.join(cells))
        log(f'neck radii z{z * 100:+.1f}: ' + ' | '.join(row))


def diag(fig, arm, poses, log, out=None):
    """Where the polish round's leftovers are: each pair's offending points grouped by side and 4 cm of height, with
    their posed and rest places (cm), into the log; with out, a one-sided close-up of each group over 5 mm deep, looking
    at it from outside (diag_<pose>_<part>_<side><z>.png)."""
    pb = arm.pose.bones
    log(f'diag: neck {np.round(np.array(fig.neck) * 100, 1)} shoulder_z {fig.shoulder_z * 100:.1f} '
        f'hip_z {fig.hip_z * 100:.1f} centre {np.round(np.array(fig.centre) * 100, 1)}')
    for pose in poses:
        shots = []
        get = posed_parts(fig, arm, pose)
        bones = (np.array([tuple(pb[b].head) for b in BONES]), np.array([tuple(pb[b].tail) for b in BONES]))
        mats = la.skinning(arm, fig.rig_names)
        everything = [la.tree(la.deform(c, w, mats), f) for c, f, w in fig.rest.values()]
        for kind, a_name, b_name in DIAG_PAIRS:
            a = get(a_name)
            if a is None:
                continue
            if kind == 'under':
                idx = np.nonzero(a[0][:, 2] < 0.0)[0]
                deep = -a[0][idx, 2]
            else:
                b = get(b_name)
                if b is None:
                    continue
                if kind == 'through':
                    idx, deep = la.poke_through(a, b, outward=la.away_from_bones(a[0], bones))
                else:
                    idx, deep = la.inside(a[0], b)
            if not len(idx):
                log(f'diag {pose}: {a_name} {kind} {b_name}: none')
                continue
            rest = parts_named(fig, a_name)[0]
            groups = {}
            for i, d in zip(idx, deep):
                p = a[0][i]
                key = ('L' if p[0] >= 0 else 'R', int(math.floor(p[2] / 0.04)))
                groups.setdefault(key, []).append((i, d))
            for key in sorted(groups):
                g = groups[key]
                ii = np.array([i for i, _ in g])
                dd = np.array([d for _, d in g])
                log(f'diag {pose}: {a_name} {kind} {b_name} {key[0]} z~{key[1] * 4}cm: {len(g)} pts, '
                    f'{dd.max() * 100:.1f} cm deep, posed {np.round(a[0][ii].mean(0) * 100, 1)}, '
                    f'rest {np.round(rest[ii].mean(0) * 100, 1)} (rest z {rest[ii, 2].min() * 100:.1f}-'
                    f'{rest[ii, 2].max() * 100:.1f})' + (exposed(everything, a[0][ii]) if kind == 'through' else ''))
                if out and kind == 'through' and dd.max() > 0.005:
                    shots.append((f'diag_{pose}_{a_name}{b_name}_{key[0]}{key[1] * 4}', a[0][ii].mean(0)))
        if out and shots:
            la.pose(arm, la.POSES[pose])
            la.settle_swing(arm, chains(fig), la.POSES[pose])
            axis = np.array([fig.centre[0], fig.centre[1]])
            for name, p in shots:
                d = np.array([p[0] - axis[0], p[1] - axis[1], 0.0])
                d = d / max(np.linalg.norm(d), 1e-6)
                la.shoot(os.path.join(out, name + '.png'), tuple(p), tuple(d * 0.42 + np.array([0.0, 0.0, 0.12])),
                         70.0, (700, 700))
    la.reset(arm)


def fix_views(fig, arm, skin, tag, log, repo):
    """The FIX_VIEWS into Saved/Concepts/CraneFix/<tag>/: Workbench, back faces culled, the parts' tints."""
    out = os.path.join(repo, 'Saved', 'Concepts', 'CraneFix', tag)
    os.makedirs(out, exist_ok=True)
    la.clay()
    sc = bpy.context.scene
    sh = sc.display.shading
    sh.color_type = 'VERTEX'
    sh.show_backface_culling = True
    skin.data.color_attributes.active_color = skin.data.color_attributes['PartTint']
    world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    world.color = (0.85, 0.15, 0.75)            # magenta: a hole through a one-sided surface shows
    sc.world = world
    for ob in arm.children:
        if ob.name.startswith(('USP_', 'UCP_')):
            ob.hide_render = True
    if os.environ.get('FIX_RADII'):
        neck_radii(fig, log)
    if os.environ.get('FIX_DIAG'):
        diag(fig, arm, os.environ['FIX_DIAG'].split('+'), log, out if os.environ.get('FIX_DIAG_SHOTS') else None)
    last = None
    for name, pose, target, off, lens in FIX_VIEWS:
        if pose != last:
            la.pose(arm, la.POSES[pose])
            la.settle_swing(arm, chains(fig), la.POSES[pose])
            last = pose
        la.shoot(os.path.join(out, name + '.png'), target, off, lens, (900, 900))
    # the back of the neck again without the hair over it (its tint picks its points; the last renders of the run)
    import bmesh
    tint = np.empty(len(skin.data.vertices) * 4, dtype=np.float32)
    skin.data.color_attributes['PartTint'].data.foreach_get('color', tint)
    hair = np.nonzero(np.abs(tint.reshape(-1, 4)[:, 0] - 0.95) < 1e-3)[0]
    bm = bmesh.new()
    bm.from_mesh(skin.data)
    bm.verts.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[bm.verts[i] for i in hair], context='VERTS')
    bm.to_mesh(skin.data)
    bm.free()
    for name, pose, target, off, lens in NO_HAIR_VIEWS:
        la.pose(arm, la.POSES[pose])
        la.settle_swing(arm, chains(fig), la.POSES[pose])
        la.shoot(os.path.join(out, name + '.png'), target, off, lens, (900, 900))
    la.reset(arm)
    log(f'fix views: {len(FIX_VIEWS) + len(NO_HAIR_VIEWS)} one-sided renders in {out}')
