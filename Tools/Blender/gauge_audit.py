"""Gauge's checks (Art/Models/Characters/Gauge.py --rig-check and --audit <tag>), on looter_audit's poses and skinning.
Not in the bake's digest.

--rig-check (no bake): the weights in clay (each part coloured by its bone), the influences and sums, and the head
turned, pitched and rolled alone (the game's look and aim) against the collar, pauldrons, stacks, drum and whistle,
into Saved/Concepts/GaugeAudit/rig/.

--audit <tag>: every part against every other part in the game's poses (looter_audit.POSES): the points of one part
that go inside another part in a pose and weren't inside it at rest (hard-surface parts overlap at their joints by
design: only what a pose pushes through counts), with how deep and where; the poses in clay; the baked look as a
turntable at the idle and close-ups; into Saved/Concepts/GaugeAudit/<tag>/ (findings.json).
"""
import json
import math
import os

import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_audit as la

POSES = ('idle', 'aim', 'stride', 'crouch', 'grip_a', 'grip_b', 'aim_high', 'aim_low', 'crouch_aim', 'swing', 'carry',
         'look_l', 'look_r', 'look_up', 'look_down')
CLAY_POSES = ('idle', 'aim', 'stride', 'crouch', 'aim_high', 'crouch_aim')
GROUP_COLOURS = {'Head': (0.85, 0.7, 0.3), 'Torso': (0.55, 0.6, 0.66), 'Belly': (0.5, 0.55, 0.6), 'Waist': (0.4, 0.42, 0.45),
                 'Hips': (0.6, 0.55, 0.5), 'Tasset': (0.7, 0.5, 0.45), 'Pauldron': (0.75, 0.62, 0.45),
                 'UpperArm': (0.5, 0.65, 0.55), 'Forearm': (0.45, 0.6, 0.7), 'Hand': (0.8, 0.55, 0.6),
                 'Thigh': (0.55, 0.62, 0.48), 'Shin': (0.5, 0.58, 0.65), 'Foot': (0.7, 0.6, 0.5)}
REACH = 0.08


def group_base(g):
    return g[:-2] if g.endswith(('_l', '_r')) else g


def posed(fig, arm):
    """{part: (points, faces, group)} posed by the armature as it stands (blend skinning, looter_audit.deform)."""
    mats = la.skinning(arm, fig.rig_names)
    return {n: (la.deform(co, W, mats), faces, g) for n, (co, faces, W, g) in fig.rest.items()}


def tree(co, faces):
    return BVHTree.FromPolygons([tuple(map(float, p)) for p in co], faces)


RAYS = (Vector((0.5773, 0.5774, 0.5773)), Vector((-0.6, 0.3, 0.7416)))


def inside_set(points, solid):
    """{index: depth} of points inside a closed solid (points, faces): an odd number of crossings along each of two
    rays (the nearest face's normal misreads points round a thin shell's edge), depth the distance to its surface."""
    co, faces = solid
    lo, hi = co.min(0) - 1e-4, co.max(0) + 1e-4
    cand = np.nonzero(((points >= lo) & (points <= hi)).all(1))[0]
    if not len(cand):
        return {}
    t = tree(co, faces)
    out = {}
    for i in cand:
        p = Vector(tuple(points[i]))
        odd = True
        for d in RAYS:
            n, o = 0, p
            while n < 64:
                hit = t.ray_cast(o, d, 3.0)
                if hit[0] is None:
                    break
                n += 1
                o = hit[0] + d * 1e-5
            if n % 2 == 0:
                odd = False
                break
        if odd:
            out[int(i)] = t.find_nearest(p)[3]
    return out


def group_parts(parts):
    out = {}
    for n, (co, faces, g) in parts.items():
        out.setdefault(g, []).append(n)
    return out


def joined(parts, names):
    co, faces, off = [], [], 0
    for n in names:
        c, f, _g = parts[n]
        co.append(c)
        faces += [[i + off for i in p] for p in f]
        off += len(c)
    return np.concatenate(co), faces


def crossings(parts):
    """{(part a, part b): {a's vertex: depth}} for the parts of different groups whose surfaces cross: a's points
    inside b."""
    groups = group_parts(parts)
    trees = {g: tree(*joined(parts, ns)) for g, ns in groups.items()}
    names = sorted(groups)
    found = {}
    for i, ga in enumerate(names):
        for gb in names[i + 1:]:
            if not trees[ga].overlap(trees[gb]):
                continue
            for a in groups[ga]:
                for b in groups[gb]:
                    for x, y in ((a, b), (b, a)):
                        hit = inside_set(parts[x][0], parts[y][:2])
                        if hit:
                            found[(x, y)] = hit
    return found


def check(fig, arm, rest_found, pose):
    """What a pose pushes into what: (a, b, points of a inside b, at rest, deepest cm, at rest cm, where cm), where
    the pose has more of a's points inside b than the rest does (by 3), or deeper (by 5 mm). (By count, not by which
    points: a ball turning on its own centre changes which of its points are inside, not how many.)"""
    la.pose(arm, la.POSES[pose])
    parts = posed(fig, arm)
    out = []
    for (a, b), hit in crossings(parts).items():
        r = rest_found.get((a, b), {})
        deep, deep0 = max(hit.values()), max(r.values()) if r else 0.0
        if len(hit) >= len(r) + 3 or deep > deep0 + 0.005:
            new = [i for i in hit if i not in r] or list(hit)
            where = parts[a][0][new].mean(0)
            out.append((a, b, len(hit), len(r), deep * 100.0, deep0 * 100.0, [round(float(v) * 100, 1) for v in where]))
    out.sort(key=lambda r_: -r_[4])
    return out


def clay(skin, fig):
    la.clay()
    sc = bpy.context.scene
    sc.world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    sc.world.color = (0.30, 0.33, 0.38)
    # each vertex coloured by its part's group (found by its rest place: the join reorders the parts)
    from mathutils import kdtree
    pts = [(co, g) for n, (co, faces, W, g) in fig.rest.items()]
    kd = kdtree.KDTree(sum(len(c) for c, g in pts))
    look = []
    for co, g in pts:
        for p in co:
            kd.insert(tuple(p), len(look))
            look.append(GROUP_COLOURS.get(group_base(g), (0.6, 0.6, 0.6)))
    kd.balance()
    rgb = np.array([look[kd.find(v.co)[1]] for v in skin.data.vertices])
    attr = skin.data.color_attributes.get('Groups') or skin.data.color_attributes.new('Groups', 'FLOAT_COLOR', 'POINT')
    attr.data.foreach_set('color', np.concatenate([rgb, np.ones((len(rgb), 1))], 1).astype(np.float32).ravel())
    skin.data.color_attributes.active_color = attr
    sc.display.shading.color_type = 'VERTEX'
    for ob in skin.parent.children if skin.parent else []:
        if ob.name.startswith(('UCX_', 'USP_', 'UCP_')):
            ob.hide_render = True


def rig_check(fig, arm, skin, log, repo):
    out = os.path.join(repo, 'Saved', 'Concepts', 'GaugeAudit', 'rig')
    os.makedirs(out, exist_ok=True)
    names = [g.name for g in skin.vertex_groups]
    W = np.zeros((len(skin.data.vertices), len(names)))
    for v in skin.data.vertices:
        for g in v.groups:
            W[v.index, g.group] = g.weight
    log(f'rig check: {len(arm.data.bones)} bones, {len(names)} weighted; at most {int((W > 0).sum(1).max())} influences '
        f'a vertex, sums {W.sum(1).min():.3f}..{W.sum(1).max():.3f}; unweighted: '
        f'{", ".join(b.name for b in arm.data.bones if b.name not in names)}')
    clay(skin, fig)
    shoot = lambda tag, target, off, lens=85.0, size=(900, 1200): la.shoot(os.path.join(out, tag + '.png'), target, off, lens, size)
    hz = fig.sk['head'][0][2]
    for v, off in {'front': (0.0, -4.4, 0.3), 'side': (4.4, 0.0, 0.3), 'back': (0.0, 4.4, 0.4)}.items():
        shoot(f'rest_{v}', (0.0, 0.0, 0.96), off)
    # the head alone: turned 35 each way, pitched 25 up and down, rolled 20 each way
    rest = posed(fig, arm)
    base = crossings(rest)
    tests = (('yaw_l', (0, 0, 1), 35.0), ('yaw_r', (0, 0, 1), -35.0), ('up', (1, 0, 0), -25.0), ('down', (1, 0, 0), 25.0),
             ('roll_l', (0, 1, 0), 20.0), ('roll_r', (0, 1, 0), -20.0))
    for tag, axis, deg in tests:
        la.pose(arm, [('turn', 'head', axis, deg)])
        parts = posed(fig, arm)
        hits = [(a, b, len(h), max(h.values()) * 100) for (a, b), h in crossings(parts).items()
                if (parts[a][2] == 'Head' or parts[b][2] == 'Head') and
                (len(h) >= len(base.get((a, b), {})) + 3 or max(h.values()) > max(base.get((a, b), {0: 0.0}).values()) + 0.005)]
        log(f'rig check: head {tag} {deg:+.0f}: ' + (', '.join(f'{a} in {b} {n} points {d:.1f} cm' for a, b, n, d in hits)
                                                    or 'clear of everything'))
        shoot(f'head_{tag}_front', (0.0, -0.03, hz + 0.08), (0.0, -1.6, 0.25), 85.0, (900, 900))
        shoot(f'head_{tag}_side', (0.0, -0.03, hz + 0.08), (1.6, -0.3, 0.25), 85.0, (900, 900))
    la.reset(arm)
    log(f'rig check: renders in {out}')


def run(fig, arm, skin, baked, tag, viewer_materials, log, repo):
    out = os.path.join(repo, 'Saved', 'Concepts', 'GaugeAudit', tag)
    os.makedirs(out, exist_ok=True)
    path = lambda n: os.path.join(out, n + '.png')
    la.reset(arm)
    rest_found = crossings(posed(fig, arm))
    report = {}
    for pose in os.environ.get('AUDIT_POSES', ','.join(POSES)).split(','):
        report[pose] = check(fig, arm, rest_found, pose)
        for a, b, n, n0, deep, deep0, where in report[pose]:
            log(f'audit {pose}: {a} into {b}: {n} points (rest {n0}), {deep:.1f} cm deep (rest {deep0:.1f}), '
                f'about {where} cm')
        if not report[pose]:
            log(f'audit {pose}: nothing through anything')
    with open(os.path.join(out, 'findings.json'), 'w') as f:
        json.dump(report, f, indent=1)
    clay(skin, fig)
    views = {'front': (0.0, -4.6, 0.3), 'side': (4.6, 0.0, 0.3), 'back': (0.0, 4.6, 0.3), 'three': (2.9, -3.6, 0.6)}
    hz = fig.sk['head'][0][2]
    for pose in CLAY_POSES:
        la.pose(arm, la.POSES[pose])
        low = pose.startswith('crouch')
        for v, off in views.items():
            la.shoot(path(f'clay_{pose}_{v}'), (0.0, 0.0, 0.66 if low else 0.96), off, 85.0, (900, 1200))
        sh = (0.0, -0.02, hz - (0.37 if low else 0.05))
        la.shoot(path(f'clay_{pose}_shoulders'), sh, (1.2, -1.9, 0.5), 85.0, (1200, 900))
        la.shoot(path(f'clay_{pose}_shoulders_r'), sh, (-1.2, -1.9, 0.5), 85.0, (1200, 900))
        la.shoot(path(f'clay_{pose}_hips'), (0.0, -0.05, 0.62 if low else 0.95), (1.4, -1.6, 0.2), 85.0, (1200, 900))
    if os.environ.get('AUDIT_CLAY_ONLY') or baked is None:
        la.reset(arm)
        log(f'audit: clay and findings in {out}')
        return
    viewer_materials(skin, baked)
    la.cycles(int(os.environ.get('AUDIT_SAMPLES', 40)))
    la.light('day')
    la.pose(arm, la.POSES['idle'])
    for k in range(8):
        a = math.radians(45.0 * k)
        la.shoot(path(f'turn_{k * 45:03d}'), (0.0, 0.0, 0.98), (math.sin(a) * 6.4, -math.cos(a) * 6.4, 0.4), 85.0)
    pb = arm.pose.bones
    shots = {
        'face_front': ((0.0, -0.1, hz + 0.19), (0.0, -1.0, 0.02), 100.0, (1200, 1200)),
        'face_three': ((0.0, -0.1, hz + 0.19), (0.6, -0.8, 0.05), 100.0, (1200, 1200)),
        'chest': ((0.0, -0.15, 1.45), (0.25, -1.3, 0.05), 85.0, (1200, 1200)),
        'firebox': ((0.0, -0.18, 1.27), (0.15, -0.9, 0.05), 85.0, (1200, 1200)),
        'hand_l': (tuple(pb['hand_l'].tail), (0.45, -0.75, 0.1), 100.0, (1200, 1200)),
        'hand_r': (tuple(pb['hand_r'].tail), (-0.45, -0.75, 0.1), 100.0, (1200, 1200)),
        'feet': ((0.0, -0.08, 0.15), (0.5, -1.5, 0.25), 85.0, (1600, 1200)),
        'back_stacks': ((0.0, 0.15, 1.6), (0.6, 2.2, 0.4), 85.0, (1200, 1200)),
        'shoulder_l': ((0.3, -0.02, 1.5), (1.4, -1.0, 0.3), 85.0, (1200, 1200)),
    }
    for name, (target, off, lens, size) in shots.items():
        la.shoot(path(name), target, off, lens, size)
    la.light('dusk')
    la.shoot(path('dusk_three'), (0.0, 0.0, 0.98), (3.9, -5.0, 0.5), 85.0)
    la.reset(arm)
    log(f'audit: renders and findings in {out}')
