"""The Tripo Unpaid's skeleton in its own pose, its skin weights, its mouth, and the maps that carry it onto Unpaid.py's
rig (parts_unpaid.py's docstring has the whole of it). All in Tripo space (parts_unpaid.T) until the warp."""
import math

import bmesh
import bpy
import numpy as np

import looter_tripo_rig as tr
import parts_unpaid as pu

SIDES = ((1.0, 'l'), (-1.0, 'r'))
ROBE_STRETCH = 1.15          # the robe runs this much longer along the shroud chain than it hangs (it streams back)
FINGER_HOLD = (-0.004, 0.012)   # m along a finger from its knuckle (Tripo space): its bone's share 0 .. 1 (2026-10-31)
SHOULDER_BLEND = (0.11, 0.05)   # m from the shoulder joint: the upper arm's weight smoothed fully within the second
SHOULDER_PASSES = 8
JAW_CORNER = (-0.2, 1.6)        # GLB units past the mouth's corners: the cheeks' jaw weight eases over this ...
JAW_CHEEK = (1.6, -2.4)         # ... from this high over the mouth's line to this low (it was a 7 mm step: torn)
JAW_PASSES = 4


def _bone(rig, name):
    b = rig.data.bones[name]
    return np.array(b.head_local), np.array(b.tail_local)


def _row(rig, sfx):
    """A hand's knuckle row, index to pinky (the side a hand's frame is squared to)."""
    return _bone(rig, f'pinky_01_{sfx}')[0] - _bone(rig, f'index_01_{sfx}')[0]


# --- The skeleton in the model's own pose ---

def tripo_skeleton(rig):
    """{bone: (head, tail, side)} in Tripo space: the spine and the shroud chain as the rig's but straight up and down
    (the model stands straight, its robe hangs), the arms, hands, neck and head on the model's own joints."""
    S = {}
    X = np.array([1.0, 0.0, 0.0])
    # The spine unbent: the rig's lengths, straight up from its pelvis.
    h = _bone(rig, 'pelvis')[0]
    for name in ('pelvis', 'spine_01', 'spine_02'):
        a, b = _bone(rig, name)
        t = h + np.array([0.0, 0.0, np.linalg.norm(b - a)])
        S[name] = (h, t, X)
        h = t
    J = pu.TRIPO_JOINTS
    S['neck'] = (pu.T(J['neck'][0]), pu.T(J['neck'][1]), X)
    head = pu.T(J['head'])
    S['head'] = (head, head + np.array([0.0, 0.0, 0.17]), X)
    S['jaw'] = (pu.T(J['hinge']), pu.T(J['chin']), X)
    S['hat'] = (head + np.array([0.0, 0.0, 0.148]), head + np.array([0.0, 0.0, 0.248]), X)
    for s, sfx in SIDES:
        P = pu.side_joints(s)
        S[f'upperarm_{sfx}'] = (P['shoulder'], P['elbow'], X)
        S[f'lowerarm_{sfx}'] = (P['elbow'], P['wrist'], X)
        knuckles = np.mean([P[f][0] for f in ('index', 'middle', 'ring', 'pinky')], axis=0)
        row = P['pinky'][0] - P['index'][0]
        length = np.linalg.norm(np.subtract(*_bone(rig, f'hand_{sfx}')[::-1]))
        S[f'hand_{sfx}'] = (P['wrist'], P['wrist'] + tr.frame(P['wrist'], knuckles, row)[0][:, 1] * length, row)
        for f in pu.FINGERS:
            S[f'{f}_01_{sfx}'] = (P[f][0], P[f][1], row)
    # The shroud: straight down from the rig's tail_01, each link the rig's length over the stretch.
    h = _bone(rig, 'tail_01')[0]
    for k in range(1, 6):
        a, b = _bone(rig, f'tail_{k:02d}')
        t = h - np.array([0.0, 0.0, np.linalg.norm(b - a) / ROBE_STRETCH])
        S[f'tail_{k:02d}'] = (h, t, X)
        h = t
    return S


def rig_frames(rig):
    """The rig's bones as frames, squared to the same sides as tripo_skeleton's."""
    X = np.array([1.0, 0.0, 0.0])
    out = {}
    for b in rig.data.bones:
        side = X
        for sfx in 'lr':
            if b.name == f'hand_{sfx}' or any(b.name == f'{f}_01_{sfx}' for f in pu.FINGERS):
                side = _row(rig, sfx)
        out[b.name] = tr.frame(b.head_local, b.tail_local, side)
    return out


def bone_maps(rig, S):
    """Each bone's map from the model's pose to the rig's rest (looter_tripo_rig.bone_map), and the side chains of the
    shroud laid out in the model's pose through tail_02's map (so they hang where the robe's sides do)."""
    R = rig_frames(rig)
    F = {name: tr.frame(*S[name]) for name in S}
    M = {}
    for name in ('pelvis', 'spine_01', 'spine_02', 'neck', 'head'):
        M[name] = tr.bone_map(F[name], R[name], along=1.0)
    M['jaw'] = M['head']             # at rest the jaw is where the head carries it
    M['hat'] = M['head']
    M['coal'] = M['spine_02']
    for s, sfx in SIDES:
        M[f'upperarm_{sfx}'] = tr.bone_map(F[f'upperarm_{sfx}'], R[f'upperarm_{sfx}'])
        M[f'lowerarm_{sfx}'] = tr.bone_map(F[f'lowerarm_{sfx}'], R[f'lowerarm_{sfx}'], across=pu.FOREARM_ACROSS)
        hand = tr.bone_map(F[f'hand_{sfx}'], R[f'hand_{sfx}'], along=pu.HAND_ALONG, across=pu.HAND_ACROSS)
        M[f'hand_{sfx}'] = hand
        for f in pu.FINGERS:
            name = f'{f}_01_{sfx}'
            # The fingers ride the palm's own map at rest (as modelled, curled): turned on maps of their own they
            # kinked and lumped at the knuckles where the blends met.
            knuckle = S[name][0]
            to = hand[:3, :3] @ knuckle + hand[:3, 3]
            M[name] = hand
            pu.log(f'{name}: its knuckle lands {100 * np.linalg.norm(to - R[name][1]):.1f} cm from the bone\'s head')
    for k in range(1, 6):
        name = f'tail_{k:02d}'
        M[name] = tr.bone_map(F[name], R[name])
    inverse = np.linalg.inv(M['tail_02'])
    for sfx in 'lr':
        h = None
        for k in (1, 2):
            name = f'tail_{sfx}_{k:02d}'
            a, b = _bone(rig, name)
            if h is None:
                h = inverse[:3, :3] @ a + inverse[:3, 3]
            t = h - np.array([0.0, 0.0, np.linalg.norm(b - a) / ROBE_STRETCH])
            S[name] = (h, t, np.array([1.0, 0.0, 0.0]))
            M[name] = tr.bone_map(tr.frame(*S[name]), R[name])
            h = t
    return M


# --- Weights ---

ARM_BONES = tuple(f'{p}_{sfx}' for sfx in 'lr' for p in ('upperarm', 'lowerarm', 'hand')) + \
    tuple(f'{f}_01_{sfx}' for sfx in 'lr' for f in pu.FINGERS)


def _connected(mask, faces, seeds):
    """The vertices of mask joined (along edges inside mask) to any seed."""
    nbr = tr.neighbours(len(mask), faces)
    keep = np.zeros(len(mask), dtype=bool)
    stack = [int(v) for v in seeds if mask[v]]
    for v in stack:
        keep[v] = True
    while stack:
        v = stack.pop()
        for w in nbr[v]:
            if mask[w] and not keep[w]:
                keep[w] = True
                stack.append(w)
    return keep


def skin_weights(rig, S, V, faces, lip=None):
    """{bone: weights} in the model's pose: bone heat for the body and arms, cleaned (a robe flap beside a hand keeps no
    hand weight, each finger only its own bone), the jaw by the lower face, the robe along the shroud chain."""
    specs = []
    order = [b.name for b in rig.data.bones]
    parents = {b.name: (b.parent.name if b.parent else None) for b in rig.data.bones}
    for name in order:
        if name in S:
            h, t, _ = S[name]
        else:
            h, t = _bone(rig, name)
        specs.append((name, tuple(map(float, h)), tuple(map(float, t)), parents[name]))
    # Bone heat only splits the arms off the body (the torso bones compete for it, then give their share back).
    deform = ["pelvis", "spine_01", "spine_02", "neck", "head"] + list(ARM_BONES)
    heat = tr.heat_weights(V, faces, specs, deform)
    n = len(V)
    W = {b: heat.get(b, np.zeros(n)) for b in ARM_BONES}

    # The arms: only what joins the hand and the elbow through arm-weighted skin keeps arm weight.
    for s, sfx in SIDES:
        mine = [b for b in ARM_BONES if b.endswith('_' + sfx)]
        share = sum(W[b] for b in mine)
        P = pu.side_joints(s)
        seeds = np.argsort(np.linalg.norm(V - P['wrist'], axis=1))[:20].tolist() + \
            np.argsort(np.linalg.norm(V - P['elbow'], axis=1))[:20].tolist()
        keep = _connected(share > 0.2, faces, seeds) | (share <= 0.2)
        stray = (share > 0.0) & ~_connected(share > 0.0, faces, seeds)
        drop = ~keep | stray
        for b in mine:
            W[b] = np.where(drop, 0.0, W[b])
        # Each finger's weight onto the finger whose line it's nearest.
        fingers = [f'{f}_01_{sfx}' for f in pu.FINGERS]
        total = sum(W[b] for b in fingers)
        dist = np.column_stack([tr_polyline_distance(V, P[f + '_line']) for f in pu.FINGERS])
        nearest = np.argmin(dist, axis=1)
        # Along each finger its own bone takes it whole a little past the knuckle (FINGER_HOLD), the hand before it.
        # (Bone heat left a fingertip part hand, part finger: curled for the lunge and the rake, a finger stretched
        # 3-6 cm along itself, 2026-10-31.) The hand's and the fingers' share together stays what the heat gave.
        hand = f'hand_{sfx}'
        both = total + W[hand]
        region = total > 0.02
        along = np.column_stack([_along(V, P[f + '_line']) for f in pu.FINGERS])[np.arange(n), nearest]
        hold = smoothstep(FINGER_HOLD[0], FINGER_HOLD[1], along)
        for k, b in enumerate(fingers):
            W[b] = np.where(region, both * hold * (nearest == k), total * (nearest == k))
        W[hand] = np.where(region, both * (1.0 - hold), W[hand])
        # The shoulder blended wider between the upper arm and the chest: raised 100 degrees out (the spread, the
        # cocked rake) its top folded 3 cm into 0.7 over a narrow seam.
        d = np.linalg.norm(V - P['shoulder'], axis=1)
        reach = smoothstep(SHOULDER_BLEND[0], SHOULDER_BLEND[1], d)
        upper = W[f'upperarm_{sfx}']
        smooth = tr.smooth_values(upper, faces, passes=SHOULDER_PASSES, amount=0.5)
        W[f'upperarm_{sfx}'] = upper + (smooth - upper) * reach
        # The forearm, hand and fingers wholly the arm's: bone heat gave a hand hanging by the hip a share of the pelvis
        # and the waist, so a raised arm left a thumb 3-6 cm behind (stretched along itself), 2026-10-31.
        low = W[f'lowerarm_{sfx}'] + W[hand] + sum(W[b] for b in fingers)
        arm_sum = sum(W[b] for b in mine)
        whole = smoothstep(0.1, 0.4, low) * (arm_sum > 1e-6)
        scale = 1.0 + (1.0 / np.maximum(arm_sum, 1e-6) - 1.0) * whole
        for b in mine:
            W[b] = W[b] * scale

    # The body as Unpaid.py weighs its torso: the spine by height (each bone whole at its middle, the rig's unbent
    # heights), the neck only close round it, the head above its pivot and the face jutting in front of the collar.
    arm = np.clip(sum(W[b] for b in ARM_BONES), 0.0, 1.0)
    z, x, y = V[:, 2], V[:, 0], V[:, 1]
    body = chain(z, (1.04, 1.2, 1.38, 1.58), ("pelvis", "spine_01", "spine_02", "neck"))
    away = smoothstep(0.05, 0.09, np.abs(x))
    body["spine_02"] = body["spine_02"] + body["neck"] * away
    body["neck"] = body["neck"] * (1.0 - away)
    head_z = S["head"][0][2]
    head = np.maximum(smoothstep(head_z - 0.012, head_z + 0.022, z),
                      smoothstep(-0.10, -0.12, y) * smoothstep(1.588, 1.604, z))
    for b in body:
        W[b] = body[b] * (1.0 - head) * (1.0 - arm)
    W["head"] = head * (1.0 - arm)

    # The robe: along the shroud chain by height (it hangs straight in this pose), its sides onto the side chains,
    # its top with the pelvis; arms excluded.
    arm = sum(W[b] for b in ARM_BONES)
    z, x = V[:, 2], V[:, 0]
    robe = smoothstep(pu.ROBE_TOP, pu.ROBE_FULL, z) * np.clip(1.0 - arm * 3.0, 0.0, 1.0)
    heads = [S[f"tail_{k:02d}"][0][2] for k in range(1, 6)] + [S["tail_05"][1][2]]
    stops = [-(a + b) / 2.0 for a, b in zip(heads[:-1], heads[1:])]     # each link whole at its middle
    main = chain(-z, stops, tuple(f'tail_{k:02d}' for k in range(1, 6)))
    side_top = S['tail_l_01'][0][2]
    side_share = 0.6 * smoothstep(0.05, 0.13, np.abs(x)) * smoothstep(side_top + 0.03, side_top - 0.08, z)
    pelvis = smoothstep(pu.ROBE_FULL - 0.02, pu.ROBE_TOP, z)
    R = {b: w * (1.0 - side_share) * (1.0 - pelvis) for b, w in main.items()}
    for s, sfx in SIDES:
        a, b = S[f'tail_{sfx}_01'][0][2], S[f'tail_{sfx}_02'][0][2]
        lo = S[f'tail_{sfx}_02'][1][2]
        sw = chain(-z, (-(a + b) / 2, -(b + lo) / 2), (f'tail_{sfx}_01', f'tail_{sfx}_02'))
        on = side_share * (np.sign(x) == s) * (1.0 - pelvis)
        for bone, w in sw.items():
            R[bone] = R.get(bone, 0.0) + w * on
    R["pelvis"] = pelvis
    for bone in set(W) | set(R):
        W[bone] = W.get(bone, np.zeros(n)) * (1.0 - robe) + np.asarray(R.get(bone, 0.0)) * robe

    # The jaw: the lower face, below the lips and in front of the hinge, from what the head carries.
    G = np.column_stack([V[:, 0] / pu.SCALE, V[:, 1] / pu.SCALE + pu.DEPTH, (V[:, 2] - pu.LIFT) / pu.SCALE])
    hinge = pu.TRIPO_JOINTS['hinge']
    below = smoothstep(pu.MOUTH['z'] + 0.05, pu.MOUTH['z'] - 0.35, G[:, 2])
    # Past the mouth's corners the cheeks ease from the head to the jaw over a few cm (the lips' step there tore the
    # corners open 4-7 cm when the jaw dropped for the lunge and the shriek, 2026-10-31), and follow it less.
    side = smoothstep(JAW_CORNER[0], JAW_CORNER[1], np.abs(G[:, 0] - pu.MOUTH['x']) - pu.MOUTH['half_width'])
    cheek = smoothstep(pu.MOUTH['z'] + JAW_CHEEK[0], pu.MOUTH['z'] + JAW_CHEEK[1], G[:, 2])
    below = below * (1.0 - side) + cheek * side * 0.75
    front = smoothstep(hinge[1] + 0.5, hinge[1] - 2.5, G[:, 1])
    jaw = below * front * W["head"]
    if lip is not None:      # the parted lips: below the cut all jaw, above none
        jaw = np.where(lip < 0, W["head"], np.where(lip > 0, 0.0, jaw))
        # Round the corners (not across the parted lips), the weights smoothed so no edge carries the whole step.
        near = (np.abs(G[:, 0] - pu.MOUTH['x']) < pu.MOUTH['half_width'] + 4.0) & \
            (np.abs(G[:, 2] - pu.MOUTH['z']) < 4.0) & (lip == 0)
        soft = tr.smooth_values(jaw, faces, passes=JAW_PASSES, amount=0.5)
        jaw = np.where(near, np.minimum(soft, W["head"]), jaw)
    W['jaw'] = jaw
    W['head'] = W['head'] - jaw
    for b in list(W):
        W[b] = tr.smooth_values(W[b], faces, passes=1, amount=0.3) if b.startswith('tail') else W[b]
    return tr.normalize(W, n)


def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, float) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def chain(x, stops, bones):
    """Weights over a chain of bones by a coordinate x (Unpaid.py's chain): bones[k] whole at stops[k]."""
    x = np.clip(np.asarray(x, float), stops[0], stops[-1])
    out = {b: np.zeros_like(x) for b in bones}
    k = np.clip(np.searchsorted(stops, x, side='right') - 1, 0, len(stops) - 2)
    f = (x - np.asarray(stops)[k]) / (np.asarray(stops)[k + 1] - np.asarray(stops)[k])
    for i, b in enumerate(bones):
        out[b] += np.where(k == i, 1.0 - f, 0.0) + np.where(k == i - 1, f, 0.0)
    return out


def _along(V, pts):
    """How far along a polyline (from its first point, meters) each point's nearest place on it lies; before its start,
    the distance back along its first segment (negative)."""
    pts = np.asarray(pts, float)
    best = np.full(len(V), np.inf)
    out = np.zeros(len(V))
    start = 0.0
    for k, (a, b) in enumerate(zip(pts[:-1], pts[1:])):
        d = b - a
        L = max(float(np.linalg.norm(d)), 1e-12)
        t = ((V - a) @ d) / (L * L)
        tc = np.clip(t, 0.0 if k else -np.inf, 1.0)
        dist = np.linalg.norm(V - (a + tc[:, None] * d), axis=1)
        better = dist < best
        best = np.where(better, dist, best)
        out = np.where(better, start + tc * L, out)
        start += L
    return out


def tr_polyline_distance(V, pts):
    best = np.full(len(V), np.inf)
    for a, b in zip(pts[:-1], pts[1:]):
        d = b - a
        t = np.clip(((V - a) @ d) / max(d @ d, 1e-12), 0.0, 1.0)
        best = np.minimum(best, np.linalg.norm(V - (a + t[:, None] * d), axis=1))
    return best


# --- The mouth ---

def cut_mouth(V, faces):
    """The lips parted: the faces across the mouth's line cut along it and its edges split between the corners, so the
    jaw opens a mouth (the model's is painted on). Returns (V, faces)."""
    m = pu.MOUTH
    zm = m['z'] * pu.SCALE + pu.LIFT
    xm = m["x"] * pu.SCALE
    hw = m['half_width'] * pu.SCALE
    front = (m['y'] + 2.0 - pu.DEPTH) * pu.SCALE
    obj = tr.new_object('_mouth', V, faces)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    region = [f for f in bm.faces if all(abs(v.co.x - xm) < hw + 0.012 and abs(v.co.z - zm) < 0.02 and v.co.y < front
                                         for v in f.verts)]
    pu.log(f"mouth: {len(region)} faces round the lips")
    near = (np.abs(V[:, 0] - xm) < 0.01) & (np.abs(V[:, 2] - zm) < 0.01)
    if near.any():
        pu.log(f"mouth: the face at the lips is at y {V[near, 1].min():.4f} (front {front:.4f}), z {zm:.4f}")
    geom = list({e for f in region for e in f.edges}) + list({v for f in region for v in f.verts}) + region
    bmesh.ops.bisect_plane(bm, geom=geom, plane_co=(0.0, 0.0, zm), plane_no=(0.0, 0.0, 1.0))
    lips = [e for e in bm.edges
            if all(abs(v.co.z - zm) < 2e-5 and abs(v.co.x - xm) < hw and v.co.y < front for v in e.verts)]
    bmesh.ops.split_edges(bm, edges=lips)
    bm.to_mesh(obj.data)
    bm.free()
    V, faces = tr.mesh_arrays(obj.data)
    bpy.data.objects.remove(obj)
    pu.log(f"mouth: {len(lips)} edges parted along the lips")
    return _pocket(V, faces, zm, hw, front, xm)


def _pocket(V, faces, zm, hw, front, xm, depth=0.024, rings=5):
    """The mouth's inside: the parted lips' hole closed by a pocket drawn back into the head (its rings toward the
    hole's middle and back), the upper half on the lips above, the lower on the lips below (so it opens with the jaw).
    Returns (V, faces, lip, pocket): lip +1 on the upper lip and its half of the pocket, -1 below, 0 elsewhere (the
    pocket's deepest point: half and half); pocket 1 on the pocket."""
    on = (np.abs(V[:, 2] - zm) < 2e-5) & (np.abs(V[:, 0] - xm) < hw + 1e-4) & (V[:, 1] < front)
    lip = np.zeros(len(V))
    above = np.zeros(len(V))
    count = np.zeros(len(V))
    for f in faces:
        c = V[f, 2].mean() - zm
        np.add.at(above, f, c)
        np.add.at(count, f, 1.0)
    lip[on] = np.sign(above[on])
    loops = [l for l in tr.boundary_loops(faces) if on[l].mean() > 0.5]
    if not loops:
        return V, faces, lip, np.zeros(len(V))
    loop = max(loops, key=len)
    P = V[loop]
    middle = P.mean(0)
    side = lip[loop]
    new_V, new_F, new_lip = [], [], []
    base, prev = len(V), list(loop)
    n = len(loop)
    for k in range(1, rings):
        f = k / rings
        ring = middle + (P - middle) * (1.0 - f) + np.array([0.0, depth, 0.0]) * math.sqrt(1.0 - (1.0 - f) ** 2)
        ring[:, 2] += side * 0.016 * f * (1.0 - f)      # a little open inside: its halves never lie on each other
        ids = list(range(base + len(new_V), base + len(new_V) + n))
        new_V.extend(ring)
        new_lip.extend(side)
        for i in range(n):
            new_F.append([prev[i], prev[(i + 1) % n], ids[(i + 1) % n], ids[i]])
        prev = ids
    top = base + len(new_V)
    new_V.append(middle + np.array([0.0, depth, 0.0]))
    new_lip.append(0.0)
    for i in range(n):
        new_F.append([prev[i], prev[(i + 1) % n], top])
    V2 = np.vstack([V, new_V])
    # Wound as the lips are: its faces look out of the mouth.
    a, b, c = (V2[new_F[0][j]] for j in range(3))
    if np.cross(b - a, c - a)[1] > 0.0:
        new_F = [f[::-1] for f in new_F]
    pu.log(f"mouth: a pocket of {len(new_F)} faces behind {n} lip vertices")
    pocket = np.concatenate([np.zeros(len(V)), np.ones(len(new_V))])
    return V2, list(faces) + [np.array(f) for f in new_F], np.concatenate([lip, new_lip]), pocket


def mouth_pieces(U):
    """The teeth behind the lips (Unpaid.py's tooth_piece), eight on the head and eight on the jaw: [(V, faces, weights,
    cavity)] in Tripo space."""
    m = pu.MOUTH
    s = pu.SCALE
    zm = m['z'] * s + pu.LIFT
    y_lip = (m['y'] - pu.DEPTH) * s
    hw = m['half_width'] * s
    pieces = []
    for k in range(8):
        u = (k - 3.5) / 3.5
        for upper in (True, False):
            x = m["x"] * s + u * hw * 0.75
            y = y_lip + (0.75 + 0.9 * u * u) * s
            if upper:
                base, tip, length = (x, y, zm + 0.28 * s), (0.0, -0.15, -1.0), 0.6 * s
            else:
                base, tip, length = (x, y + 0.1 * s, zm - 0.3 * s), (0.0, -0.15, 1.0), 0.5 * s
            Vt, Ft = U['tooth_piece'](np.array(base), U['unit'](np.array(tip)), length, 0.23 * s)
            pieces.append((Vt, Ft, {'head' if upper else 'jaw': 1.0}, 0.15))
    return pieces
