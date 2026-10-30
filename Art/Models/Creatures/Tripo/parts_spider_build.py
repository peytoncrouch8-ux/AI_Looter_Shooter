"""SK_Spider from the user's Tripo Meadow Wolf (parts_spider_shape.py reads it): Spider.py runs build() below its rig
line (LOOTER_SPIDER_TRIPO, default on; 0 = the scripted spider), after its parts code has made the bones, so the bones,
their names, the rest pose (the standing pose the game's IK solves, knees toward knee_pole), the atlas and MI_SpiderBody
are exactly the scripted spider's; only the parts (one mesh per bone, each moving wholly with it) are Tripo's.

- Space: Tripo's model levelled (its hips' plane flat: it stood 23 degrees nose-down), then SCALE cm per unit about the
  hips' middle, which lands on the rig's (the body's heights then match the scripted spider's). The cephalothorax
  (carapace, head, eyes; the chelicerae and palps ride along unstretched) is stretched KX along x: the rig's hips, the
  fangs' pivots and the head's hit zone lie farther apart front to back than Tripo's round carapace has them; the
  abdomen moves back by what that adds behind (its egg unchanged).
- Shells (parts_spider_shells.py): the carapace and head, the abdomen.
- Limbs (parts_spider_limbs.py): each leg's femur (coxa and femur) and tibia (patella to claw) rebuilt as rings round
  Tripo's own centre line, carried from Tripo's joint frames into the rig's: the claw tip on the foot bone, the knee on
  the knee. Tripo's legs are 114-159 cm at this scale and the rig's 230-238, so they stretch along (the coxa and patella
  only a little) and a little across, and each segment takes Spider.py's chitin plate (neck, swell, rim) so the joints
  read. The chelicerae (with their fangs) and the palps: rings round their own lines, unstretched.
- Bristles (parts_spider_hair.py) as the scripted spider's."""
import math
import os

import numpy as np

import parts_spider_hair as hair
import parts_spider_limbs as sl
import parts_spider_shape as ss
import parts_spider_shells as sh

SCALE = 2.45            # cm per Tripo unit
KX = 1.3                # the cephalothorax's stretch along x
STEP = 2.0              # cm between a limb's stations, in the model's pose
HIP_IN = 6.0            # cm: the femur's tube starts this far inside the body, before Tripo's hip ring
COXA_STRETCH, PATELLA_STRETCH = 1.1, 1.3
TROCHANTER = 5.0        # cm past the coxa's narrow point, the femur's own region starts
ACROSS = (1.0, 1.4)     # the legs' scale across: the square root of their stretch, within these
STATION = 3.5           # cm between a leg's rings along it, in the model's pose ...
JOINT_STATIONS = (-2.4, -1.2, -0.5, 0.0, 0.6, 1.5)   # ... and these round each joint (cm)
PLATE = 0.7             # how much of Spider.py's chitin plate (neck, swell, rim) each segment takes on Tripo's radius
MIN_GAP = 0.4           # cm: the least gap between a leg's rings


def log(message):
    ss.log(message)


def _vec(p):
    return np.array([p[0], p[1], p[2]], float)


class Space:
    """The maps from Tripo's creature axes (units) to cm: uniform (U) and the cephalothorax's (C, stretched along x)."""

    def __init__(self, src, labels, rig_hips):
        hips = ss.hip_points(src, labels)
        self.R, s_fit, _, _ = ss.level(hips, rig_hips)
        self.cT = np.mean([self.R @ hips[k] for k in ss.LEGS], axis=0)
        self.cR = np.mean([rig_hips[k] for k in ss.LEGS], axis=0)
        log(f'placed at {SCALE} cm per unit (the hips alone would give {s_fit:.2f}); the cephalothorax {KX} along')

    def U(self, P):
        return self.cR + SCALE * ((self.R @ np.atleast_2d(P).T).T - self.cT)

    def stretch(self, Q):
        Q = np.array(Q, float)
        Q[..., 0] = self.cR[0] + KX * (Q[..., 0] - self.cR[0])
        return Q


def _extend(P, length, step):
    """P with stations added before its start along its first direction (length cm, every step)."""
    t = P[0] - P[1]
    t /= np.linalg.norm(t)
    n = int(round(length / step))
    return np.vstack([P[0] + t * step * k for k in range(n, 0, -1)] + [P])


def _at(P, s, at):
    return np.array([np.interp(at, s, P[:, k]) for k in range(3)])


def plate_profile(s, bounds, plate):
    """A factor on a leg's radius at arc lengths s: each segment between bounds (its joints) shaped as Spider.py's
    chitin plate (plate(t): a narrow neck out of the joint, a swell, a hard rim before the next), PLATE of it, about
    1 on the whole: Tripo's legs are smooth tubes with soft dips at their joints; the plates make them read jointed."""
    mean = float(np.mean([plate(t) for t in np.linspace(0.0, 1.0, 101)]))
    out = np.ones(len(s))
    for a, b in zip(bounds[:-1], bounds[1:]):
        inside = (s >= a) & (s <= b)
        out[inside] = [1.0 + PLATE * (plate((x - a) / (b - a)) / mean - 1.0) for x in s[inside]]
    return out


def build_leg(m_femur, m_tibia, src, V, labels, leg, rig, pole_r, ns_plate):
    """One leg's femur and tibia meshes. rig: (hip, knee, foot) cm; pole_r: the rig's knee pole for it; ns_plate:
    Spider.py's plate()."""
    tree = ss.surface(V, src.F, labels == leg)
    P = ss.leg_line(src, V, labels, leg, SCALE, STEP, tree)
    members = np.nonzero(labels == leg)[0]
    tip = V[members[np.argmax(src.geo[members])]]
    P = _extend(P, HIP_IN, STEP)
    s = sl.arc(P)
    start = s[int(round(HIP_IN / STEP))]
    outward = np.array([P[0][0], P[0][1], 0.0])
    outward /= np.linalg.norm(outward)
    pole_t = np.array([0.0, 0.0, 1.0]) + outward * 0.4
    # The leg's plane: first its best-fit plane, then (once the knee is known) the hip, knee and claw's.
    _, _, vt = np.linalg.svd(P - P.mean(0))
    normal = vt[2]
    for _ in range(2):
        T, top, side = sl.frames(P, normal, pole_t)
        R = sl.smooth_radii(sl.sample(tree, P, T, top, side))
        J = sl.joints(R, s, start)
        H, K = _at(P, s, start), _at(P, s, J['knee'])
        normal = np.cross(K - H, tip - K)
        normal /= np.linalg.norm(normal)
    T, top, side = sl.frames(P, normal, pole_t)
    R = sl.smooth_radii(sl.sample(tree, P, T, top, side))
    if os.environ.get('LOOTER_SPIDER_DEBUG'):
        log(f'leg {leg} radius every 4 cm from its ring: ' + ' '.join(
            f'{np.interp(a, s, R.mean(1)):.1f}' for a in np.arange(start, s[-1], 4.0)))
    hip_r, knee_r, foot_r = rig
    normal_r = np.cross(knee_r - hip_r, foot_r - knee_r)
    normal_r /= np.linalg.norm(normal_r)
    total = np.linalg.norm(knee_r - hip_r) + np.linalg.norm(foot_r - knee_r)
    across = float(np.clip(math.sqrt(total / (J['tip'] - start)), *ACROSS))

    def frame(head, tail, normal, pole):
        y = (tail - head) / np.linalg.norm(tail - head)
        x = np.cross(normal, y)
        x *= 1.0 if x @ pole > 0 else -1.0
        return head, x, y, np.cross(x, y)

    # The femur: Tripo's hip ring onto the rig's hip, the coxa a little longer, the femur the rest, the knee on the knee.
    fs, ft = frame(H, K, normal, pole_t), frame(hip_r, knee_r, normal_r, pole_r)
    a_c = (_at(P, s, J['coxa']) - H) @ fs[2]
    a_k = (K - H) @ fs[2]
    L_f = np.linalg.norm(knee_r - hip_r)
    g_f = sl.piecewise([(0.0, 0.0), (a_c, a_c * COXA_STRETCH), (a_k, L_f)])
    # The stations the rings stand at: every STATION cm, closer round each joint (its rim and neck), one exactly at the
    # knee (the femur ends there and the tibia starts there, each rounded off: Tripo's knee is the leg's narrow point,
    # so the two domes are small and meet whatever the bend).
    marks = [J['coxa'], J['knee'], J['patella'], J['meta'], J['tarsus']]
    s_new = np.concatenate([np.arange(0.0, s[-1], STATION), [s[-1]], [j + d for j in marks for d in JOINT_STATIONS]])
    s_new = np.unique(s_new[(s_new >= 0.0) & (s_new <= s[-1])])
    # A station closer than MIN_GAP to the one before is dropped (rings on top of each other make slivers); a joint's
    # own station (or the tip's) takes the place of a near one.
    kept = []
    for x in s_new:
        own = any(abs(x - j) < 1e-6 for j in marks + [s[-1]])
        if kept and x - kept[-1][0] < MIN_GAP:
            if own and not kept[-1][1]:
                kept[-1] = (x, True)
            continue
        kept.append((x, own))
    s_new = np.array([x for x, _ in kept])
    P = np.column_stack([np.interp(s_new, s, P[:, k]) for k in range(3)])
    R = np.column_stack([np.interp(s_new, s, R[:, j]) for j in range(R.shape[1])])
    s = s_new
    R = R * plate_profile(s, [start] + marks + [J['tip']], ns_plate)[:, None]
    keep = s <= J['knee'] + 1e-6
    C = sl.carry(P[keep], fs, ft, g_f, across)
    _, top_r, side_r = sl.frames(C, normal_r, pole_r)
    troch = J['coxa'] + TROCHANTER
    sk = s[keep]
    # The coxa's region up to the trochanter (its ring at 0.7 on Tripo's narrow point), the femur's from there: a UV
    # seam at the first station past it.
    u_coxa = np.interp(sk, [0.0, J['coxa'], troch], [0.0, 0.7, 1.0])
    u_femur = np.interp(sk, [troch, J['knee']], [0.0, 1.0])
    seam = int(np.searchsorted(sk, troch))
    sl.tube(m_femur, C, top_r, side_r, R[keep] * across, lambda k, reg: u_coxa[k] if reg == 'coxa' else u_femur[k],
            'coxa', start='hub', end='round', region_at=lambda k: 'coxa' if k < seam else 'femur')
    femur = dict(C=C, top=top_r, side=side_r, R=R[keep] * across, U=u_femur)
    # The tibia: from the knee to the claw, the patella a little longer, the claw's tip on the foot bone.
    ts, tt = frame(K, tip, normal, pole_t), frame(knee_r, foot_r, normal_r, pole_r)
    a_p = (_at(P, s, J['patella']) - K) @ ts[2]
    a_f = (tip - K) @ ts[2]
    L_t = np.linalg.norm(foot_r - knee_r)
    g_t = sl.piecewise([(0.0, 0.0), (a_p, a_p * PATELLA_STRETCH), (a_f, L_t)])
    keep = s >= J['knee'] - 1e-6
    C = sl.carry(P[keep], ts, tt, g_t, across)
    _, top_r, side_r = sl.frames(C, normal_r, pole_r)
    sk = s[keep]
    U = np.interp(sk, [J['knee'], J['patella'], J['meta'], J['tarsus'], J['tip']], [0.0, 0.13, 0.51, 0.86, 0.985])
    claw = sl.carry(tip[None], ts, tt, g_t, across)[0]
    sl.tube(m_tibia, C[:-1], top_r[:-1], side_r[:-1], R[keep][:-1] * across, U[:-1], 'tibia', start='round', end='tip',
            tip=claw)
    tibia = dict(C=C, top=top_r, side=side_r, R=R[keep] * across, U=U)
    log(f'leg {leg}: {J["tip"] - start:.0f} cm along (femur {a_k:.0f} -> {L_f:.0f}, tibia {a_f:.0f} -> {L_t:.0f}), '
        f'across {across:.2f}; joints at ' + ', '.join(f'{k} {v - start:.0f}' for k, v in J.items()) +
        f'; the claw {np.linalg.norm(claw - foot_r):.2f} cm off the foot')
    return femur, tibia


def build_limb(m, src, V, labels, label, step=1.5, shift=None):
    """A chelicera or palp: rings round its own line from where it meets the face to its tip (the fang's point), in
    the model's own shape (unstretched; shift moves it with the stretched face). Region chel or palp along."""
    tree = ss.surface(V, src.F, labels == label)
    P = ss.leg_line(src, V, labels, label, SCALE, step, tree)
    members = np.nonzero(labels == label)[0]
    tip = V[members[np.argmax(src.geo[members])]]
    P = _extend(P, 3.0, step)
    _, _, vt = np.linalg.svd(P - P.mean(0))
    T, top, side = sl.frames(P, vt[2], np.array([0.0, 0.0, 1.0]))
    # Smoother than a leg: the chelicera bulges where its fang leaves it, and its rays come back uneven there.
    R = sl.smooth_radii(sl.sample(tree, P, T, top, side), along=2, around=2)
    s = sl.arc(P)
    region = 'chel' if label in (ss.CHEL_L, ss.CHEL_R) else 'palp'
    d = np.zeros(3) if shift is None else shift
    sl.tube(m, P[:-1] + d, top[:-1], side[:-1], R[:-1], s[:-1] / s[-1], region, start='hub', end='tip', tip=tip + d)
    return P + d, tip + d


def build(ns):
    """The parts and bristles ({bone: lc.Mesh}) for Spider.py's rig, from its namespace (bones, ATLAS, lc, knee_pole,
    plate, the hair numbers)."""
    lc, ATLAS, knee_pole = ns['lc'], ns['ATLAS'], ns['knee_pole']
    bones = {name: (_vec(head), _vec(tail)) for name, head, tail, _ in ns['bones']}
    rig_hips, legs = {}, {}
    for pair in range(4):
        for side, suffix in ((0, 'l'), (1, 'r')):
            hip, knee = bones[f'femur_{pair}_{suffix}']
            foot = bones[f'tibia_{pair}_{suffix}'][1]
            rig_hips[2 * pair + side] = hip
            pole = knee_pole(ns['Vector'](hip.tolist()))
            legs[2 * pair + side] = (f'{pair}_{suffix}', (hip, knee, foot), _vec(pole))
    src = ss.Source()
    labels, found = ss.segment(src)
    if found != 8:
        raise ValueError(f'the Tripo spider has {found} legs; the rig needs 8')
    space = Space(src, labels, rig_hips)
    Vu = space.U(src.V)
    abdomen_fit = ss.split_abdomen(Vu, labels)
    parts, limbs = {}, {}
    for leg, (suffix, rig, pole) in legs.items():
        parts[f'femur_{suffix}'] = lc.Mesh(ATLAS)
        parts[f'tibia_{suffix}'] = lc.Mesh(ATLAS)
        limbs[f'femur_{suffix}'], limbs[f'tibia_{suffix}'] = build_leg(
            parts[f'femur_{suffix}'], parts[f'tibia_{suffix}'], src, Vu, labels, leg, rig, pole, ns['plate'])
    Vc = space.stretch(Vu)
    parts['body'], parts['head'] = lc.Mesh(ATLAS), lc.Mesh(ATLAS)
    face = sh.cephalothorax(parts['body'], parts['head'], src, Vc, labels)
    parts['abdomen'] = lc.Mesh(ATLAS)
    belly = sh.abdomen(parts['abdomen'], src, Vu, labels, abdomen_fit, space)
    for label, bone in ((ss.CHEL_L, 'fang_l'), (ss.CHEL_R, 'fang_r'), (ss.PALP_L, 'palp_l'), (ss.PALP_R, 'palp_r')):
        members = labels == label
        root = Vu[members].mean(0)
        shift = space.stretch(root) - root
        parts[bone] = lc.Mesh(ATLAS)
        P, tip = build_limb(parts[bone], src, Vu, labels, label, shift=shift)
        if bone.startswith('fang'):
            log(f'{bone}: its top {np.round(P[0], 1).tolist()}, its pivot {np.round(bones[bone][0], 1).tolist()}, '
                f'its fang tip {np.round(tip, 1).tolist()}')
    sh.eyes(parts['head'], face)
    # The bristles, as the scripted spider's: on the legs, combed back over the carapace (the body's part of it) and the
    # abdomen's upper side.
    hairs = {}
    hair.legs(hairs, limbs, ns)
    ks = face['ks'] / face['rings']
    hairs['body'] = lc.Mesh(ATLAS)
    hair.shell(hairs['body'], face, ns['BODY_HAIRS'][0][1], lambda d, s, v: d > 0.1 and v < ks - 0.04, ns, 31)
    hairs['abdomen'] = lc.Mesh(ATLAS)
    hair.shell(hairs['abdomen'], belly, ns['BODY_HAIRS'][1][1], lambda d, s, v: d > -0.4 and v < 0.9, ns, 37)
    return parts, hairs


def seam_normals(obj, tolerance=1e-5):
    """Shades across the seams where two parts share a ring (the body's and the head's): every corner at a place more
    than one vertex shares takes the area-weighted normal of all the faces round that place, as one welded surface
    would; every other corner keeps its own. Set as custom normals (the FBX carries them; the importer reads them)."""
    me = obj.data
    n = len(me.vertices)
    co = np.empty(n * 3)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    keys = np.round(co / tolerance).astype(np.int64)
    _, group, counts = np.unique(keys, axis=0, return_inverse=True, return_counts=True)
    group = group.ravel()
    shared = counts[group] > 1
    if not shared.any():
        return 0
    acc = np.zeros((len(counts), 3))
    for poly in me.polygons:
        area_normal = np.array(poly.normal) * poly.area
        for v in poly.vertices:
            if shared[v]:
                acc[group[v]] += area_normal
    acc /= np.maximum(np.linalg.norm(acc, axis=1, keepdims=True), 1e-12)
    loops = np.empty(len(me.loops) * 3)
    me.corner_normals.foreach_get('vector', loops)
    loops = loops.reshape(-1, 3)
    vidx = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get('vertex_index', vidx)
    on = shared[vidx]
    loops[on] = acc[group[vidx[on]]]
    me.normals_split_custom_set(loops.tolist())
    log(f'seam normals: {int(shared.sum())} vertices on shared seams')
    return int(shared.sum())
