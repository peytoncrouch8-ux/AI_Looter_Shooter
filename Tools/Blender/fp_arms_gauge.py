"""Gauge's first-person arms (Tools/Blender/fp_arms_heroes.py): his third-person arms (Tools/Blender/gauge_parts.py's
build_arm and build_hand) remade small enough for the first-person view on Ellis's bones, with gauge_geo's kit: black
enamel casings over the forearm (two, a brass band over their seam, a brass wrist cuff stopping 3 cm short of the wrist
so a bent hand clears it, a piston and rod on the back, rings of rivets), a brass elbow ball between iron hinge caps,
the upper arm's casing with its band and piston to the cut; a boxy enamel palm with a brass knuckle bar, an iron wrist
ball and three riveted fingers of block segments on brass hinge pins: the thumb on thumb_*, the two fingers on index_*
and middle_* (spread a little apart, along their curl axes, so the code's curls still bend them), ring and pinky bones
present and unweighted.

Sizes: the forearm casings 3.9-4.45 cm in radius, about Ellis's duster sleeve (his third-person 7.8 cm would fill the
screen); the hand his third-person one brought to Ellis's bone lengths, a little leaner (PALM, FINGER below); the
palm's underside on Ellis's, 1.58 cm under the bones' plane, so FPArms_rig.json's GripPoint holds. Each finger block's
palm side lies on Ellis's gloved finger's (FPArms.py's capsules, flattened 0.9, the thumb's 0.88), its extra thickness
toward the back: the game closes every hand by Ellis's curls (FHandFingers::Of), so his fingers rest on a grip, not in
it. Every part rides one bone rigidly (as his third-person parts).

Triangles (2026-10-20, ~9.5k for both arms; SK_FP_Arms ~8k): spent on the hand (bevelled blocks, pins, the wrist ball,
the palm's and the fingers' rivets) and the wrist cuff. The casings' rivet rings are not modelled but painted into the
texture from their places (fp_arms_paint.paint_rivets: a dome in the normal map, polished brass; a detail copy cast
onto the game parts, as his third-person model bakes, left garbage normals across the bevels' sharp edges at this
size), and the upper arm (under the frame but for its elbow end) is coarser.
Looks: gauge_looks.make('ArmsEnamel' / 'ArmsBrass' / 'ArmsIron'), baked into one set, FPArmsGauge.
"""
import math

import numpy as np

import bpy
import looter_bake as bk
from fp_arms_base import A, C, log, smoothstep, unit, v3
import gauge_geo as gg

RIVET = 0.42                         # a rivet head (cm)
# His third-person hand (gauge_parts.build_hand, on a hand 1.28 a man's) brought to Ellis's bone lengths (x 0.78):
# the palm 8.1 x 4.8 cm there, the fingers 3.2 x 2.8. Here a little leaner, the palm's underside kept on Ellis's (the
# grip point) and each finger's palm side on his (PALM_FLAT).
SPREAD = {'index': 0.6, 'middle': -0.7}       # the two fingers moved apart along their curl axes (cm): 0.8 cm between
FINGER = {'index': (2.8, 2.5), 'middle': (2.8, 2.5), 'thumb': (2.9, 2.6)}   # width, thickness (cm)
PALM = (0.3, 9.0, -1.55, 5.65, -1.58, 2.35)   # the palm box: x from, to; y from, to; z under, over (hand frame, cm)
PALM_FLAT = {'thumb': 0.88}                   # FPArms.py's capsules' flattening toward the palm (the fingers' 0.9)


def _cm(shape):
    return gg.Shape(shape.V * C, shape.F)


def _np(m):
    return np.array([[m[i][j] for j in range(4)] for i in range(4)])


def rivet(r):
    """A cheap round-headed rivet on +z (34 triangles): the head r across, 0.6 r high, a short shank."""
    return gg.lathe([(r * 0.9, -0.4 * r), (r, 0.0), (r * 0.72, 0.42 * r), (0.0, 0.6 * r)], 6)


def rivets(points, normals, r):
    mats = [gg.frame(p, nm, (1.0, 0.0, 0.0) if abs(unit(nm)[0]) < 0.9 else (0.0, 1.0, 0.0))
            for p, nm in zip(points, normals)]
    return gg.instances(rivet(r), mats)


def finger_sides(ch, k, thick):
    """A finger block's reach from its bone's line toward the palm (on Ellis's gloved finger there: his capsule's radius
    flattened as FPArms.py flattens it, the smaller of the segment's two ends) and toward the back (the rest; the
    thumb's only 3 mm past his: it wraps the grip's other side, where a thick block's corner went into it), cm."""
    palm_side = PALM_FLAT.get(ch.name, 0.9) * min(ch.radii[k], ch.radii[k + 1]) / C
    if ch.name == 'thumb':
        return palm_side, palm_side + 0.3
    return palm_side, thick - palm_side


BLOCK_BEVEL = (0.75, 0.85)                    # a block's edges rounded (cm): the others, the tip's


def finger_width(ch, k):
    """A finger block's width (cm): Ellis's gloved finger's there (his capsule's diameter, the smaller end), a little
    over (the thumb's just its), at most FINGER's; the square block's corners then stay about where his round finger's
    sides are."""
    return min(FINGER[ch.name][0], (2.05 if ch.name == 'thumb' else 2.3) * min(ch.radii[k], ch.radii[k + 1]) / C)


def block_bevel(ch, k, tip):
    """How round a block's edges are (cm): BLOCK_BEVEL, the thumb's nearly round (a slim block's corners are what
    reached into the grip)."""
    if ch.name == 'thumb':
        palm_side, back_side = finger_sides(ch, k, 0.0)
        return 0.92 * min(finger_width(ch, k) / 2, (palm_side + back_side) / 2)
    return BLOCK_BEVEL[tip]


def build(rest, b):
    import os
    import fp_arms_heroes as fh
    gl = fh._module('gauge_looks')
    b.sources += [os.path.join(fh.BLENDER_DIR, 'gauge_looks.py'), os.path.join(fh.BLENDER_DIR, 'gauge_geo.py')]
    looks = {k: gl.make(f'Arms{k}', f'FPArmsGauge_{k}Look', {}) for k in ('Enamel', 'Brass', 'Iron')}
    parts = {}                                   # (bone, look) -> [Shape (m, Blender space)]: the game mesh
    painted = []                                 # rivets painted on the casings: (centre, normal, radius), m

    def add(bone, look, shape):
        parts.setdefault((bone, look), []).append(shape)

    def paint(frame, points, normals):
        for p, n in zip(points, normals):
            painted.append((frame[:3, :3] @ (np.asarray(p) * C) + frame[:3, 3], frame[:3, :3] @ np.asarray(n), RIVET * C))

    M_hand = _np(rest.matrix())                  # hand frame (m) -> Blender
    chs = A['CHAINS']                            # Ellis's: the bones, and his gloved fingers' radii

    def in_hand(shape_cm):
        return _cm(shape_cm).at(M_hand)

    # --- the hand (hand frame, cm)
    x0, x1, y0, y1, zb, zt = PALM
    half = ((x1 - x0) / 2, (y1 - y0) / 2, (zt - zb) / 2)
    palm = gg.box(half, b=0.75, seg=2).at(gg.T(((x1 + x0) / 2, (y1 + y0) / 2, (zt + zb) / 2)))
    V = palm.V.copy()
    yc = (y1 + y0) / 2
    narrow = 1.0 - 0.16 * (1.0 - smoothstep(1.5, 5.0, V[:, 0]))     # narrower toward the wrist
    V[:, 1] = yc + (V[:, 1] - yc) * narrow
    V[:, 2] = np.where(V[:, 2] > 0, V[:, 2] - 0.35 * (1.0 - smoothstep(1.0, 6.0, V[:, 0])), V[:, 2])
    add('hand', 'Enamel', in_hand(gg.Shape(V, palm.F)))
    add('hand', 'Brass', in_hand(gg.cyl(1.0, y1 - y0 + 0.3, 16, b=0.22, seg=1).at(
        gg.frame((x1 - 0.35, y0 - 0.15, zt - 0.75), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)))))
    add('hand', 'Iron', in_hand(gg.sphere(2.75, 20, 10).at(gg.T((-0.4, 0.4, 0.0)))))
    pts = [(x, y, zt + 0.02) for x in (2.8, 6.4) for y in (y0 + 1.25, y1 - 1.25)]
    add('hand', 'Brass', in_hand(rivets(pts, [(0, 0, 1.0)] * len(pts), RIVET * 1.05)))
    # the fingers: block segments along the bones (palm side on Ellis's finger: finger_sides), hinge pins at the joints
    # (on the segment before)
    for ch in chs:
        if ch.name not in ('thumb', 'index', 'middle'):
            continue
        width, thick = FINGER[ch.name]
        first = 0 if ch.name == 'thumb' else 1
        off = ch.lateral * SPREAD.get(ch.name, 0.0)
        bones = ch.bones
        for k in range(first, len(ch.joints) - 1):
            bone = bones[k]
            h, t = ch.joints[k] / C + off, ch.joints[k + 1] / C + off
            ax = unit(t - h)
            nb = unit(ch.dorsal(k) - ax * (ch.dorsal(k) @ ax))
            lb = np.cross(ax, nb)
            L = float(np.linalg.norm(t - h))
            tip = k == len(ch.joints) - 2
            palm_side, back_side = finger_sides(ch, k, thick)
            wk = finger_width(ch, k)
            F = np.eye(4)
            F[:3, 0], F[:3, 1], F[:3, 2], F[:3, 3] = lb, ax, nb, h
            start = 0.35 if (ch.name == 'thumb' and k == 0) else 0.0
            seg = gg.box((wk / 2, (L - start) / 2 + (0.45 if tip else 0.3), (palm_side + back_side) / 2),
                         b=block_bevel(ch, k, tip), seg=2).at(
                F @ gg.T((0.0, start + (L - start) / 2 + (0.25 if tip else 0.0), (back_side - palm_side) / 2)))
            add(bone, 'Enamel', in_hand(seg))
            add(bone, 'Brass', in_hand(rivets([h + ax * L * 0.5 + nb * (back_side + 0.01)], [nb], RIVET * 0.9)))
            if k > first:
                prev = bones[k - 1]
                pin = gg.cyl(0.56, wk + 0.36, 10, b=0.12, seg=1).at(gg.frame(h - lb * (wk + 0.36) / 2, lb, nb))
                add(prev, 'Brass', in_hand(pin))
        if ch.name != 'thumb':
            # the knuckle's pin, on the hand (under the knuckle bar's ends)
            h = ch.joints[1] / C + off
            ax = unit(ch.joints[2] - ch.joints[1])
            nb = unit(ch.dorsal(1) - ax * (ch.dorsal(1) @ ax))
            lb = np.cross(ax, nb)
            wk = finger_width(ch, 1)
            add('hand', 'Iron', in_hand(gg.cyl(0.62, wk + 0.2, 10, b=0.12, seg=1).at(
                gg.frame(h - lb * (wk + 0.2) / 2, lb, nb))))

    # --- the forearm (Blender space, m): frames from the elbow toward the wrist
    E, W, S = rest.elbow, rest.wrist, rest.shoulder
    dl = unit(W - E)
    L2 = float(np.linalg.norm(W - E)) / C
    back = rest.R[:, 2]                          # out of the back of the hand: the forearm's outside here
    Fl = gg.frame(E, dl, back)

    def fore(shape_cm, at=np.eye(4)):
        return _cm(shape_cm.at(at)).at(Fl)

    add('lowerarm', 'Brass', _cm(gg.sphere(3.6, 16, 8)).at(gg.T(E)))
    fa = [(3.5, 4.05), (L2 * 0.3, 4.45), (L2 * 0.47, 4.3)]
    fb = [(L2 * 0.5, 4.2), (L2 - 4.4, 3.9)]
    add('lowerarm', 'Enamel', fore(gg.housing(fa, 24, 0.5, 2)))
    add('lowerarm', 'Brass', fore(gg.cyl(4.33, 1.1, 24, b=0.2, seg=1), gg.T((0, 0, 5.4))))
    add('lowerarm', 'Brass', fore(gg.cyl(4.62, 1.6, 24, b=0.25, seg=1), gg.T((0, 0, L2 * 0.47 - 0.35))))
    add('lowerarm', 'Enamel', fore(gg.housing(fb, 24, 0.42, 2)))
    # the wrist cuff: the arm's brass nearest the eye on a six-gun, rounder than the bands
    add('lowerarm', 'Brass', fore(gg.cyl(4.18, 1.7, 32, b=0.28, seg=2), gg.T((0, 0, L2 - 4.65))))
    # the piston on the outside (its frame's x is toward the back of the hand) and its rod
    px = 4.62
    add('lowerarm', 'Iron', fore(gg.cyl(0.66, 7.0, 10, b=0.15, seg=1), gg.T((px, 0.0, 4.2))))
    add('lowerarm', 'Brass', fore(gg.cyl(0.3, L2 * 0.42, 8), gg.T((px - 0.05, 0.0, 10.8))))
    add('lowerarm', 'Iron', fore(gg.cyl(0.55, 0.9, 10, b=0.12, seg=1), gg.T((px - 0.05, 0.0, 10.8 + L2 * 0.42 - 0.4))))
    rp, rn = [], []
    for z, r, cnt in ((4.6, None, 10), (L2 * 0.43, None, 10), (L2 * 0.58, 4.17, 8), (L2 * 0.8, 3.99, 8)):
        rr = float(np.interp(z, *np.array(fa).T)) if r is None else r
        for a in np.linspace(0.0, 2 * math.pi, cnt, endpoint=False) + 0.3:
            if abs(math.atan2(math.sin(a), math.cos(a))) < 0.25:
                continue                          # the piston's line
            rp.append((rr * math.cos(a), rr * math.sin(a), z))
            rn.append((math.cos(a), math.sin(a), 0.0))
    paint(Fl, rp, rn)

    # --- the elbow's hinge caps and the upper arm (frames from the elbow up)
    hinge = unit(np.cross(E - S, W - E))
    for sgn in (-1.0, 1.0):
        add('upperarm', 'Iron', _cm(gg.cyl(2.05, 1.0, 16, b=0.25, seg=1)).at(gg.frame(E + hinge * sgn * 0.031,
                                                                                     hinge * sgn, dl)))
    du = unit(S - E)
    Fu = gg.frame(E, du, back - du * (back @ du))
    # (the cut end, 25 cm up, is always under the frame: fewer segments up there; its rivets baked)
    ua = [(4.4, 3.8), (12.0, 4.05), (25.0, 4.2)]
    add('upperarm', 'Enamel', _cm(gg.housing(ua, 18, 0.5, 2)).at(Fu))
    add('upperarm', 'Brass', _cm(gg.cyl(4.3, 1.5, 18, b=0.22, seg=1).at(gg.T((0, 0, 6.2)))).at(Fu))
    add('upperarm', 'Iron', _cm(gg.cyl(0.66, 6.0, 8, b=0.15, seg=1).at(gg.T((4.3, 0.0, 12.5)))).at(Fu))
    add('upperarm', 'Brass', _cm(gg.cyl(0.3, 5.5, 6).at(gg.T((4.25, 0.0, 7.6)))).at(Fu))
    rp, rn = [], []
    for z in (8.4,):
        rr = float(np.interp(z, *np.array(ua).T))
        for a in np.linspace(0.0, 2 * math.pi, 8, endpoint=False) + 0.35:
            rp.append((rr * math.cos(a), rr * math.sin(a), z))
            rn.append((math.cos(a), math.sin(a), 0.0))
    paint(Fu, rp, rn)

    # --- into objects: one per bone and look, weighted wholly to its bone, UVs smart-projected
    objs = []
    for (bone, look), shapes in sorted(parts.items()):
        ob = gg.make_object(f'Gauge_{bone}_{look}_r', gg.join(shapes), looks[look], sharp=40.0)
        g = ob.vertex_groups.new(name=f'{bone}_r')
        g.add(list(range(len(ob.data.vertices))), 1.0, 'REPLACE')
        bk.smart_uv(ob, angle=50.0, margin=0.004)
        objs.append(ob)
        log(f'  {ob.name}: {sum(len(p.vertices) - 2 for p in ob.data.polygons)} triangles')
    tris = sum(len(p.vertices) - 2 for o in objs for p in o.data.polygons)
    log(f'Gauge: {len(objs)} parts by bone and look, {tris} triangles a side; {len(painted)} casing rivets painted')
    casings = [o for o in objs if o.name in ('Gauge_lowerarm_Enamel_r', 'Gauge_upperarm_Enamel_r')]
    # fix_ao: the many small parts' occlusion margins painted black squares over each other's islands (bands, cuff)
    b.add_set('FPArmsGauge', objs, 'Character', 'FPArmsGauge_Iron', rivets=(casings, painted), fix_ao=True,
              RoughnessBreakup=0.06)
