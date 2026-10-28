"""The heroes' clipping and fit audit (2026-10-27, the user: our script-built heroes perfected, Tripo clipped and
z-fought): a hero's model script built and rigged WITHOUT its bake (Hero.bake_all stubbed: the texture sets on disk
are never touched), then in the game's poses (looter_audit.POSES and body_poses: idle, run, crouch, slide, aim, jump,
fall, the hero's own idle on the select stand, a wide hip turn each way) every layer checked against the ones it meets,
into Saved/Concepts/FitAudit/<Hero>/<tag>/findings.json and the log:

    Tools\\artrun.ps1 -Script Tools\\Blender\\fit_audit.py -ScriptArgs '<Hero>,--tag,<tag>[,--poses,idle+slide][,--renders]'

Checks (cm; the hero's own audit module gives the pairs: <hero>_audit.PAIRS (inner, outer), SOLIDS, GROUNDED):
- through: points of the inner layer out through the outer (looter_audit.poke_through);
- cross: faces of the two that cut each other where both face out (the outer surfaces: a thickened part's inner shell
  is hidden), BVH overlap;
- inside: a part's points inside a solid part; under: below the ground;
- zfight (at rest and the idle): points of one part within ZFIGHT_GAP of another part's surface facing the same way
  (coplanar layers flicker in game); every pair of parts, not only PAIRS;
- Gauge (hard surface): gauge_audit's check, what a pose pushes inside another part beyond the rest's overlap.
--renders: clay views (back faces culled: the game's one-sided materials) of each pose, front, back, both sides.
Not in any bake's digest."""
import importlib
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
for p in (HERE, os.path.join(REPO, 'Art', 'Models', 'Characters')):
    if p not in sys.path:
        sys.path.insert(0, p)

import looter_audit as la  # noqa: E402

ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
HERO = next(a for a in ARGV if not a.startswith('--'))
AUDITS = {'Ellis': 'ellis_audit', 'Odessa': 'odessa_audit', 'Crane': 'crane_audit', 'Gauge': 'gauge_audit',
          'Pike': 'pike_audit'}
POSES = ('rest', 'idle', 'stand', 'run', 'stride', 'crouch', 'slide', 'aim', 'jump', 'fall', 'hip_turn_l',
         'hip_turn_r', 'grip_a', 'carry', 'crouch_aim')
# pairs the hero's own audit lacks, checked here too: (part, solid) as SOLIDS, (inner, outer) as PAIRS
EXTRA_SOLIDS = {'Ellis': (('Holster', 'Duster'), ('Revolver', 'Duster'), ('HolsterSkirt', 'Duster'))}
EXTRA_PAIRS = {}
INSIDE_REACH = 0.12        # m: how deep inside a solid a point is still found (looter_audit.inside's 5 cm missed a
                           # holster pouch 6-8 cm inside a sprinting thigh)
THROUGH_DEPTH = 0.03       # m: how far out through the layer over it a point is still found (deeper read the waistcoat
                           # in the coat's open front as through it)
ZFIGHT_GAP = 0.0006          # m: two surfaces facing the same way nearer than this fight for depth at range
ZFIGHT_DOT = 0.9
DETAIL = '--detail' in ARGV  # each finding's points grouped by side and height in the log


def arg_value(name, default=None):
    return ARGV[ARGV.index(name) + 1] if name in ARGV and ARGV.index(name) + 1 < len(ARGV) else default


def log(message):
    print(f'FIT {message}', flush=True)


# --------------------------------------------------------------------------------------------------- the hero idles
def hero_idle(arm, hero):
    """The hero's own idle on the select stand (FHeroIdle per create_ember_assets.py HERO_IDLE; the stance layer's
    LooterStancePoseIdle.cpp): posture, lean, hip cock, stance width, head, and where each hand rests."""
    b = arm.data.bones
    hd = lambda n: b[n].head_local.copy()
    X, Y, Z = la.X, la.Y, la.Z
    pel, chest = hd('pelvis'), hd('spine_05')
    ua_l, ua_r = hd('upperarm_l'), hd('upperarm_r')
    hang_l, hang_r = la._arms_down()[:2], la._arms_down()[2:]
    t = lambda v: tuple(v)
    if hero == 'Ellis':
        # the weary revenant: chest 4, neck 7 down, head 2 up; right thumb hooked in the belt's front
        return hang_l + [('turn', 'spine_03', X, 4.0), ('turn', 'neck_01', X, 7.0), ('turn', 'head', X, -2.0),
                         ('ik', 'arm_r', t(pel + Vector((-0.10, -0.13, 0.03))), t(ua_r + Vector((-0.35, 0.15, -0.25))))]
    if hero == 'Odessa':
        # hip cocked over the right leg, chin up; right hand on the hip (elbow out), left raised before the shoulder
        return [('move', 'pelvis', (-0.045, 0.0, 0.0)), ('turn', 'pelvis', Y, 4.0), ('turn', 'spine_01', Y, -6.0),
                ('turn', 'head', X, -5.0),
                ('ik', 'leg_l', t(hd('foot_l')), t(hd('calf_l') + Vector((0.0, -0.5, 0.0)))),
                ('ik', 'leg_r', t(hd('foot_r')), t(hd('calf_r') + Vector((0.0, -0.5, 0.0)))),
                ('ik', 'arm_r', t(pel + Vector((-0.235, 0.0, 0.10))), t(ua_r + Vector((-0.40, 0.20, -0.20)))),
                ('ik', 'arm_l', t(ua_l + Vector((-0.04, -0.28, -0.14))), t(ua_l + Vector((0.15, 0.0, -0.40))))]
    if hero == 'Crane':
        # stooped (lean 8, chest 8), head low and forward (neck 18); left hand closed on the chain at his chest
        return hang_r + [('turn', 'spine_01', X, 4.0), ('turn', 'spine_03', X, 12.0), ('turn', 'neck_01', X, 18.0),
                         ('turn', 'head', X, -5.0),
                         ('ik', 'arm_l', t(chest + Vector((0.04, -0.19, -0.08))), t(ua_l + Vector((0.30, 0.0, -0.40))))]
    if hero == 'Gauge':
        # wide (each foot 7 out), hips settling, hands hung clear of the boiler
        return la._arms_down(22.0) + [
            ('move', 'pelvis', (0.0, 0.0, -0.02)),
            ('ik', 'leg_l', t(hd('foot_l') + Vector((0.07, 0.0, 0.0))), t(hd('calf_l') + Vector((0.1, -0.5, 0.0)))),
            ('ik', 'leg_r', t(hd('foot_r') + Vector((-0.07, 0.0, 0.0))), t(hd('calf_r') + Vector((-0.1, -0.5, 0.0))))]
    if hero == 'Pike':
        # upright, weight back (hips 3 back, lean -3); thumbs in both pack straps at the upper chest; head 28 right
        return [('move', 'pelvis', (0.0, 0.03, 0.0)), ('turn', 'spine_01', X, -3.0),
                ('ik', 'leg_l', t(hd('foot_l')), t(hd('calf_l') + Vector((0.0, -0.5, 0.0)))),
                ('ik', 'leg_r', t(hd('foot_r')), t(hd('calf_r') + Vector((0.0, -0.5, 0.0)))),
                ('ik', 'arm_l', t(chest + Vector((0.11, -0.16, 0.0))), t(ua_l + Vector((0.30, 0.05, -0.40)))),
                ('ik', 'arm_r', t(chest + Vector((-0.11, -0.16, 0.0))), t(ua_r + Vector((-0.30, 0.05, -0.40)))),
                ('turn', 'head', Z, -28.0), ('turn', 'head', X, -3.0)]
    return la.POSES['idle']


# ---------------------------------------------------------------------------------------------------------- checks
def face_normals(co, faces):
    n = np.array([np.cross(co[f[1]] - co[f[0]], co[f[-1]] - co[f[0]]) for f in faces])
    return n / np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)


def crossings_out(a, b, bones):
    """Face pairs of a and b that cut each other where both faces look out of the body (an outer surface: a
    thickened part's inner shell is hidden), and where (cm)."""
    ta = BVHTree.FromPolygons([tuple(map(float, p)) for p in a[0]], a[1])
    tb = BVHTree.FromPolygons([tuple(map(float, p)) for p in b[0]], b[1])
    pairs = ta.overlap(tb)
    if not pairs:
        return 0, None, None
    ia = sorted({i for i, _ in pairs})
    ib = sorted({j for _, j in pairs})
    ca = np.array([a[0][a[1][i]].mean(0) for i in ia])
    cb = np.array([b[0][b[1][j]].mean(0) for j in ib])
    na = face_normals(a[0], [a[1][i] for i in ia])
    nb = face_normals(b[0], [b[1][j] for j in ib])
    oa = ((na * la.away_from_bones(ca, bones)).sum(1) > 0.0)
    ob = ((nb * la.away_from_bones(cb, bones)).sum(1) > 0.0)
    out_a = dict(zip(ia, oa))
    out_b = dict(zip(ib, ob))
    keep = [(i, j) for i, j in pairs if out_a[i] and out_b[j]]
    if not keep:
        return 0, None, None
    cen = np.array([a[0][a[1][i]].mean(0) for i, _ in keep])
    return len(keep), [round(float(v) * 100, 1) for v in cen.mean(0)], cen


def zfight(parts):
    """{(a, b): (points, where cm)}: points of part a within ZFIGHT_GAP of part b's surface, facing the same way."""
    names = sorted(parts)
    trees, boxes, nrm = {}, {}, {}
    for n in names:
        co, faces = parts[n]
        trees[n] = BVHTree.FromPolygons([tuple(map(float, p)) for p in co], faces)
        boxes[n] = (co.min(0) - ZFIGHT_GAP, co.max(0) + ZFIGHT_GAP)
        nrm[n] = la.normals(co, faces)
    found = {}
    for a in names:
        co = parts[a][0]
        for b in names:
            if b == a:
                continue
            lo, hi = boxes[b]
            cand = np.nonzero(((co >= lo) & (co <= hi)).all(1))[0]
            hits = []
            for i in cand:
                loc, n, _f, d = trees[b].find_nearest(Vector(tuple(co[i])), ZFIGHT_GAP)
                if loc is not None and float(np.dot(nrm[a][i], tuple(n))) > ZFIGHT_DOT:
                    hits.append(i)
            if len(hits) >= 3:
                found[(a, b)] = (len(hits), [round(float(v) * 100, 1) for v in co[hits].mean(0)])
                if DETAIL:
                    log(f'  zfight {a} on {b}: {groups(co[hits])}')
    return found


BONES = ('pelvis', 'spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05', 'neck_01', 'neck_02', 'head') + tuple(
    f'{b}_{s}' for b in ('clavicle', 'upperarm', 'lowerarm', 'hand', 'thigh', 'calf', 'foot') for s in 'lr')


def family_parts(fig, mats):
    """{part name: (posed points, faces)} for every part of the rig, by its fig.rest name."""
    out = {}
    for n, v in fig.rest.items():
        co, faces, W = v[0], v[1], v[2]
        out[n] = (la.deform(co, W, mats) if mats is not None else co, faces)
    return out


def groups(points, deep=None):
    """Points grouped by side (L +x, R -x, M the middle 4 cm) and 5 cm of height: 'L z130: 6 pts 2.1 cm (x y z)'."""
    out = {}
    for k, p in enumerate(points):
        side = 'M' if abs(p[0]) < 0.02 else ('L' if p[0] > 0 else 'R')
        key = (side, int(math.floor(p[2] / 0.05)) * 5)
        out.setdefault(key, []).append(k)
    rows = []
    for key in sorted(out):
        ii = out[key]
        d = f' {deep[ii].max() * 100:.1f} cm' if deep is not None else ''
        rows.append(f'{key[0]} z{key[1]}: {len(ii)}{d} {np.round(points[ii].mean(0) * 100, 1).tolist()}')
    return '; '.join(rows)


def check_layers(A, fig, arm, pose):
    """The layered heroes (Ellis, Odessa, Crane, Pike): through, cross, inside, under in one pose."""
    get = A.posed_parts(fig, arm, pose)
    pb = arm.pose.bones
    bones = (np.array([tuple(pb[b].head) for b in BONES]), np.array([tuple(pb[b].tail) for b in BONES]))
    found = []
    for inner, outer in tuple(A.PAIRS) + EXTRA_PAIRS.get(HERO, ()):
        a, b = get(inner), get(outer)
        if a is None or b is None:
            continue
        idx, deep = la.poke_through(a, b, depth=THROUGH_DEPTH, outward=la.away_from_bones(a[0], bones))
        if len(idx):
            found.append(('through', inner, outer, int(len(idx)), round(float(deep.max() * 100), 2),
                          [round(float(v) * 100, 1) for v in a[0][idx].mean(0)]))
            if DETAIL:
                log(f'  {pose} {inner} through {outer}: {groups(a[0][idx], deep)}')
        n, where, cen = crossings_out(a, b, bones)
        if n:
            found.append(('cross', inner, outer, n, 0.0, where))
            if DETAIL:
                log(f'  {pose} {inner} cross {outer}: {groups(cen)}')
    for part, solid in tuple(A.SOLIDS) + EXTRA_SOLIDS.get(HERO, ()):
        a, b = get(part), get(solid)
        if a is None or b is None:
            continue
        idx, deep = la.inside(a[0], b, INSIDE_REACH)
        if len(idx):
            found.append(('inside', part, solid, int(len(idx)), round(float(deep.max() * 100), 2),
                          [round(float(v) * 100, 1) for v in a[0][idx].mean(0)]))
            if DETAIL:
                log(f'  {pose} {part} inside {solid}: {groups(a[0][idx], deep)}')
    for part in A.GROUNDED:
        a = get(part)
        if a is None:
            continue
        idx = np.nonzero(a[0][:, 2] < 0.0)[0]
        if len(idx):
            found.append(('under', part, 'the ground', int(len(idx)), round(float(-a[0][idx, 2].min() * 100), 2),
                          [round(float(v) * 100, 1) for v in a[0][idx].mean(0)]))
    return found


def tint(name):
    """A steady colour per part family (its name without numbers and sides), spread round the hue circle."""
    import colorsys
    import zlib
    fam = name.split('.')[0].rstrip('_lr').rstrip('_')
    h = (zlib.crc32(fam.encode()) % 997) / 997.0
    return colorsys.hsv_to_rgb(h, 0.55, 0.85)


def renders(fig, arm, skin, A, poses, out):
    """Clay of each pose, each part its own colour, back faces culled on a magenta sky (a hole shows magenta): the
    whole figure from four sides and closer views of the hips and shoulders (FIT_VIEWS env: 'hips+chest' etc.)."""
    la.clay()
    sc = bpy.context.scene
    sc.display.shading.show_backface_culling = True
    sc.world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    sc.world.color = (0.85, 0.15, 0.75)
    skin.hide_render = True
    for ob in arm.children:
        if ob.name.startswith(('USP_', 'UCP_', 'UCX_')):
            ob.hide_render = True
    obs = {}
    for name, v in fig.rest.items():
        me = bpy.data.meshes.new('_Fit_' + name)
        me.from_pydata([tuple(map(float, p)) for p in v[0]], [], [tuple(f) for f in v[1]])
        me.update()
        ob = bpy.data.objects.new('_Fit_' + name, me)
        sc.collection.objects.link(ob)
        ob.color = (*tint(name), 1.0)
        obs[name] = ob
    whole = {'front': (0.0, -3.6, 0.3), 'back': (0.0, 3.6, 0.3), 'left': (3.6, -0.4, 0.3), 'right': (-3.6, -0.4, 0.3)}
    for pose in poses:
        la.pose(arm, la.POSES[pose])
        if fig_has_swing(fig):
            la.settle_swing(arm, A.chains(fig), la.POSES[pose])
        mats = la.skinning(arm, fig.rig_names)
        for name, ob in obs.items():
            co = la.deform(fig.rest[name][0], fig.rest[name][2], mats)
            ob.data.vertices.foreach_set('co', co.astype(np.float32).ravel())
            ob.data.update()
        pb = arm.pose.bones
        hips = np.array(pb['pelvis'].head)
        chest = np.array(pb['spine_05'].head)
        z = float(hips[2] + chest[2]) * 0.5
        for v, off in whole.items():
            la.shoot(os.path.join(out, f'{pose}_{v}.png'), (float(hips[0]), float(hips[1]), z), off, 85.0, (700, 900))
        close = {'hips_l': (hips, (1.1, -0.9, 0.15)), 'hips_r': (hips, (-1.1, -0.9, 0.15)),
                 'chest_l': (chest, (0.9, -0.9, 0.1)), 'chest_r': (chest, (-0.9, -0.9, 0.1)),
                 'back_neck': (chest + np.array((0.0, 0.0, 0.1)), (0.0, 1.0, 0.35)),
                 'waist': (hips + np.array((0.0, -0.1, 0.06)), (0.25, -0.75, 0.05)),
                 'knee_l': (np.array(pb['calf_l'].head), (0.45, -0.6, 0.25))}
        for v, (t, off) in close.items():
            if os.environ.get('FIT_VIEWS') and v not in os.environ['FIT_VIEWS'].split('+'):
                continue
            la.shoot(os.path.join(out, f'{pose}_{v}.png'), tuple(map(float, t)), off, 85.0, (700, 700))
    for ob in obs.values():
        me = ob.data
        bpy.data.objects.remove(ob)
        bpy.data.meshes.remove(me)
    skin.hide_render = False
    la.reset(arm)


def fig_has_swing(fig):
    return bool(getattr(fig, 'swing', None))


def run_fit(fig, arm, skin):
    tag = arg_value('--tag', 'fit')
    out = os.path.join(REPO, 'Saved', 'Concepts', 'FitAudit', HERO, tag)
    os.makedirs(out, exist_ok=True)
    A = importlib.import_module(AUDITS[HERO])
    la.POSES.update(la.body_poses(arm))
    la.POSES['stand'] = hero_idle(arm, HERO)
    want = arg_value('--poses')
    poses = [p for p in POSES if not want or p in want.split('+')]
    log(f'{HERO}: parts {sorted(fig.rest)}')
    if '--renders-only' in ARGV:
        renders(fig, arm, skin, A, [p for p in poses if p != 'rest'], out)
        log(f'renders in {out}')
        return
    report = {}
    if HERO == 'Gauge':
        la.reset(arm)
        rest_found = A.crossings(A.posed(fig, arm))
        for pose in poses:
            if pose == 'rest':
                continue
            rows = A.check(fig, arm, rest_found, pose)
            report[pose] = [('inside', a, b, n, round(deep, 2), w) for a, b, n, n0, deep, deep0, w in rows]
    else:
        for pose in poses:
            report[pose] = check_layers(A, fig, arm, pose)
    # coplanar layers at rest and the idle
    for pose in ('rest', 'idle'):
        if want and pose not in want.split('+'):
            continue
        la.pose(arm, la.POSES[pose])
        if fig_has_swing(fig):
            la.settle_swing(arm, A.chains(fig), la.POSES[pose])
        mats = la.skinning(arm, fig.rig_names)
        for (a, b), (n, where) in zfight(family_parts(fig, mats)).items():
            report.setdefault(pose, []).append(('zfight', a, b, n, 0.0, where))
    totals = {}
    for pose, rows in report.items():
        for kind, a, b, n, deep, where in rows:
            log(f'{pose}: {a} {kind} {b}: {n}' + (f', {deep:.1f} cm deep' if deep else '') + f', about {where} cm')
            key = f'{a} {kind} {b}'
            totals[key] = totals.get(key, 0) + n
        if not rows:
            log(f'{pose}: clear')
    with open(os.path.join(out, 'findings.json'), 'w') as f:
        json.dump({'poses': report, 'totals': totals}, f, indent=1)
    log('totals: ' + '; '.join(f'{k} {v}' for k, v in sorted(totals.items(), key=lambda kv: -kv[1])))
    if '--renders' in ARGV:
        renders(fig, arm, skin, A, [p for p in poses if p != 'rest'], out)
    la.reset(arm)
    log(f'findings in {out}')


def main():
    if '--audit' not in sys.argv:
        sys.argv += ['--audit', arg_value('--tag', 'fit')]
    M = importlib.import_module(HERO)
    M.bake_all = lambda *a, **k: None          # never bake: the texture sets on disk stay as they are
    A = importlib.import_module(AUDITS[HERO])

    def wrapped(*args, **kw):
        fig = next(a for a in args if hasattr(a, 'rest'))
        arm = next(a for a in args if isinstance(a, bpy.types.Object) and a.type == 'ARMATURE')
        skin = next(a for a in args if isinstance(a, bpy.types.Object) and a.type == 'MESH')
        run_fit(fig, arm, skin)
    A.run = wrapped
    M.main()


if __name__ == '__main__':
    main()
