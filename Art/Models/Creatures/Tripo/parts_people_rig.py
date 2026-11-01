"""A story person's Tripo body on its script rig: the skeleton laid on the model in its own pose (upright space, see
parts_people_src), the skin weights made there, the maps that carry each bone onto the rig's rest, the parted lips.
The recipe is parts_unpaid_rig.py's (the Tripo Unpaid on Unpaid.py's rig), made general over the six rigs."""
import math

import bmesh
import bpy
import numpy as np

import looter_tripo_rig as tr
import parts_people_cfg as cfg
import parts_people_src as ps

SIDES = ((1.0, 'l'), (-1.0, 'r'))
FINGERS = ('thumb', 'index', 'middle', 'ring', 'pinky')
X = np.array([1.0, 0.0, 0.0])
SHOULDER_BLEND = (0.12, 0.05)    # m from the shoulder joint: the upper arm's weight smoothed fully within the second
FINGER_HOLD = (-0.004, 0.012)    # m along a finger past its knuckle: its bone's share 0 .. 1


def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, float) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def chain(x, stops, bones):
    x = np.clip(np.asarray(x, float), stops[0], stops[-1])
    out = {b: np.zeros_like(x) for b in bones}
    k = np.clip(np.searchsorted(stops, x, side='right') - 1, 0, len(stops) - 2)
    f = (x - np.asarray(stops)[k]) / (np.asarray(stops)[k + 1] - np.asarray(stops)[k])
    for i, b in enumerate(bones):
        out[b] += np.where(k == i, 1.0 - f, 0.0) + np.where(k == i - 1, f, 0.0)
    return out


def unit(v):
    v = np.asarray(v, float)
    return v / max(np.linalg.norm(v), 1e-12)


def _bone(rig, name):
    b = rig.data.bones[name]
    return np.array(b.head_local), np.array(b.tail_local)


def knuckle_centre(rig, sfx):
    return np.mean([_bone(rig, f'{f}_01_{sfx}')[0] for f in ('index', 'middle', 'ring', 'pinky')], axis=0)


def _row(rig, sfx):
    return _bone(rig, f'pinky_01_{sfx}')[0] - _bone(rig, f'index_01_{sfx}')[0]


# --- The skeleton in the model's own pose (upright space) ---

def skeleton(name, rig, J):
    """{bone: (head, tail, side)} for the bones laid on the model: the spine straight up from the pelvis (the rig's
    lengths), the neck, head and jaw, arms and hands on the measured joints, and cfg 'upright' bones (a seated rig's
    thighs, standing)."""
    c = cfg.PEOPLE[name]
    S = {}
    h = np.array([0.0, 0.0, ps.HIP_Z + 0.09])
    for b in ('pelvis', 'spine_01', 'spine_02'):
        a, t = _bone(rig, b)
        n = h + np.array([0.0, 0.0, np.linalg.norm(t - a)])
        S[b] = (h, n, X)
        h = n
    S['neck'] = (J['neck'], J['head'], X)
    S['head'] = (J['head'], J['head'] + np.array([0.0, 0.0, 0.17]), X)
    S['jaw'] = (J['hinge'], J['chin'], X)
    for s, sfx in SIDES:
        S[f'upperarm_{sfx}'] = (J[f'shoulder_{sfx}'], J[f'elbow_{sfx}'], X)
        S[f'lowerarm_{sfx}'] = (J[f'elbow_{sfx}'], J[f'wrist_{sfx}'], X)
        row = np.asarray(c.get('row_' + sfx, c.get('row', (0.0, 1.0, 0.0))), float)
        S[f'hand_{sfx}'] = (J[f'wrist_{sfx}'], J[f'knuckles_{sfx}'], row)
    if c.get('seated'):        # a seated rig's thighs, laid standing on the model: hips to knees, then the shin's top
        hips, knee = np.array([0.0, 0.0, ps.HIP_Z]), np.array([0.0, J['knee'][1], J['knee'][2]])
        S['skirt_f_01'] = (hips, knee, X)
        S['skirt_f_02'] = (knee, knee - np.array([0.0, 0.0, 0.14]), X)
    return S


def bone_maps(name, rig, S, J):
    """Each rig bone's map (4x4) from upright space to the rig's rest. Bones not laid on the model ride an ancestor's."""
    c = cfg.PEOPLE[name]
    F = {b: tr.frame(*S[b]) for b in S}
    R = {b.name: tr.frame(b.head_local, b.tail_local, X) for b in rig.data.bones}
    M = {}
    for b in ('pelvis', 'spine_01', 'spine_02', 'neck'):
        M[b] = tr.bone_map(F[b], R[b])
    # The head as one piece, scaled so the script's hat sits on it: the model's chin-to-crown onto the script head's.
    k = c.get('head_scale') or head_scale(name, rig, J)
    Rh, hh, lh = R['head']
    M['head'] = tr.bone_map(F['head'], (Rh, hh, lh), along=k, across=k)
    ps.log(f'{name}: the head scaled {k:.3f}')
    for b in ('jaw', 'hat'):
        if b in rig.data.bones:
            M[b] = M['head']
    for s, sfx in SIDES:
        M[f'upperarm_{sfx}'] = tr.bone_map(F[f'upperarm_{sfx}'], R[f'upperarm_{sfx}'], across=c.get('arm_across', 1.0))
        M[f'lowerarm_{sfx}'] = tr.bone_map(F[f'lowerarm_{sfx}'], R[f'lowerarm_{sfx}'], across=c.get('arm_across', 1.0))
        # The hand whole: wrist to knuckles onto the rig's wrist to knuckles, scaled evenly (its own proportions).
        wrist = _bone(rig, f'hand_{sfx}')[0]
        target = tr.frame(wrist, knuckle_centre(rig, sfx), _row(rig, sfx))
        kh = c.get('hand_scale', target[2] / F[f'hand_{sfx}'][2])
        M[f'hand_{sfx}'] = tr.bone_map(F[f'hand_{sfx}'], target, along=kh, across=kh)
        ps.log(f'{name}: hand_{sfx} scaled {kh:.3f}')
    for b in ('skirt_f_01', 'skirt_f_02'):
        if b in S and b in R:
            M[b] = tr.bone_map(F[b], R[b], across=1.0)
    for bone in rig.data.bones:          # the rest ride their nearest mapped ancestor (fingers and prop bones the hand)
        if bone.name in M:
            continue
        p = bone.parent
        while p is not None and p.name not in M:
            p = p.parent
        M[bone.name] = M[p.name] if p is not None else M['pelvis']
    if 'coal' in M:
        M['coal'] = M['spine_02']
    return M


def head_scale(name, rig, J):
    """The script head's chin-to-crown over the model's (cfg PEOPLE[name]['script_head']: (chin z, crown z) of the
    script body at rest, measured from its head and jaw vertices)."""
    chin, crown = cfg.PEOPLE[name]['script_head']
    return (crown - chin) / (J['crown'][2] - J['chin'][2])


# --- Weights ---

def arm_bones(sfx):
    return (f'upperarm_{sfx}', f'lowerarm_{sfx}', f'hand_{sfx}')


ARM_BONES = arm_bones('l') + arm_bones('r')


def _connected(mask, faces, seeds):
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


def skin_weights(name, rig, S, J, V, faces, lip=None):
    """{bone: weights} in upright space: bone heat splits the arms off, the torso by height, the head and jaw, the
    lower body by cfg 'lower' ('skirts': Abel's long coat; 'thighs': trousers to the knee; 'pelvis')."""
    c = cfg.PEOPLE[name]
    n = len(V)
    parents = dict(pelvis=None, spine_01='pelvis', spine_02='spine_01', neck='spine_02', head='neck')
    for _, sfx in SIDES:
        parents.update({f'upperarm_{sfx}': 'spine_02', f'lowerarm_{sfx}': f'upperarm_{sfx}',
                        f'hand_{sfx}': f'lowerarm_{sfx}'})
    specs = [(b, tuple(map(float, S[b][0])), tuple(map(float, S[b][1])), p) for b, p in parents.items()]
    body_faces = [f for f in faces if lip is None or not (np.abs(lip[f]) > 0).all()] if lip is not None else faces
    heat = tr.heat_weights(V, body_faces, specs, list(parents))
    if not heat:
        ps.log(f'{name}: bone heat found nothing: envelope weights instead')
        heat = envelope_weights(V, faces, S, list(parents))
    ps.log(f'{name}: bone heat: ' + ', '.join(f'{b} {heat[b].sum():.0f}' if b in heat else f'{b} NONE' for b in parents))
    W = {b: heat.get(b, np.zeros(n)) for b in ARM_BONES}
    for s, sfx in SIDES:
        mine = arm_bones(sfx)
        share = sum(W[b] for b in mine)
        seeds = np.argsort(np.linalg.norm(V - J[f'wrist_{sfx}'], axis=1))[:20].tolist() + \
            np.argsort(np.linalg.norm(V - J[f'elbow_{sfx}'], axis=1))[:20].tolist()
        keep = _connected(share > 0.2, faces, seeds) | (share <= 0.2)
        stray = (share > 0.0) & ~_connected(share > 0.0, faces, seeds)
        for b in mine:
            W[b] = np.where(~keep | stray, 0.0, W[b])
        # The shoulder blended wide between the upper arm and the chest (the T-pose's arm comes down 80 degrees).
        d = np.linalg.norm(V - J[f'shoulder_{sfx}'], axis=1)
        reach = smoothstep(SHOULDER_BLEND[0], SHOULDER_BLEND[1], d)
        upper = W[f'upperarm_{sfx}']
        smooth = tr.smooth_values(upper, faces, passes=c.get('shoulder_passes', 10), amount=0.5)
        W[f'upperarm_{sfx}'] = upper + (smooth - upper) * reach
        # The forearm and hand wholly the arm's.
        low = W[f'lowerarm_{sfx}'] + W[f'hand_{sfx}']
        arm_sum = sum(W[b] for b in mine)
        whole = smoothstep(0.1, 0.4, low) * (arm_sum > 1e-6)
        scale = 1.0 + (1.0 / np.maximum(arm_sum, 1e-6) - 1.0) * whole
        for b in mine:
            W[b] = W[b] * scale
    arm = np.clip(sum(W[b] for b in ARM_BONES), 0.0, 1.0)

    # The torso by height (each bone whole at its middle), the neck only round the neck, the head above its pivot and
    # the face and beard in front of the collar.
    z, x, y = V[:, 2], V[:, 0], V[:, 1]
    neck_z, head_z = J['neck'][2], J['head'][2]
    stops = (1.04, 1.2, min(1.38, neck_z - 0.05), 0.5 * (neck_z + head_z))
    body = chain(z, stops, ('pelvis', 'spine_01', 'spine_02', 'neck'))
    away = smoothstep(0.06, 0.1, np.abs(x))
    body['spine_02'] = body['spine_02'] + body['neck'] * away
    body['neck'] = body['neck'] * (1.0 - away)
    chin = J['chin']
    face = smoothstep(J['head'][1] - 0.04, J['head'][1] - 0.075, y) * smoothstep(chin[2] - 0.03, chin[2] + 0.0, z) * \
        smoothstep(0.12, 0.09, np.abs(x))
    head = np.maximum(smoothstep(head_z - 0.012, head_z + 0.022, z) * smoothstep(0.13, 0.1, np.abs(x)), face)
    for b in body:
        W[b] = body[b] * (1.0 - head) * (1.0 - arm)
    W['head'] = head * (1.0 - arm)

    lower = c.get('lower', 'pelvis')
    if lower in ('skirts', 'thighs'):
        # Below the waist (Abel.py's body_weights: from 1.06 m down) the lower body's own weights.
        L = (skirt_weights if lower == 'skirts' else thigh_weights)(name, J, V)
        low = smoothstep(1.06, 0.98, z)
        for b in set(W) | set(L):
            if b in ARM_BONES:
                continue
            W[b] = W.get(b, np.zeros(n)) * (1.0 - low) + np.asarray(L.get(b, 0.0)) * low * (1.0 - arm)

    # The jaw: the lower face below the lips, in front of the hinge, out of what the head carries.
    m = c['mouth_u']
    hinge = J['hinge']
    below = smoothstep(m['z'] + 0.004, m['z'] - 0.008, z)
    side = smoothstep(-0.003, 0.03, np.abs(x - m['x']) - m['half_width'])
    cheek = smoothstep(m['z'] + 0.03, m['z'] - 0.045, z)
    below = below * (1.0 - side) + cheek * side * 0.75
    front = smoothstep(hinge[1] + 0.01, hinge[1] - 0.04, y)
    jaw = below * front * W['head']
    if lip is not None:
        jaw = np.where(lip < 0, W['head'], np.where(lip > 0, 0.0, jaw))
        near = (np.abs(x - m['x']) < m['half_width'] + 0.07) & (np.abs(z - m['z']) < 0.07) & (lip == 0)
        soft = tr.smooth_values(jaw, faces, passes=4, amount=0.5)
        jaw = np.where(near, np.minimum(soft, W['head']), jaw)
    W['jaw'] = jaw
    W['head'] = W['head'] - jaw
    W = {b: w for b, w in W.items() if b in rig.data.bones}
    return tr.normalize(W, n)


def envelope_weights(V, faces, S, bones, power=6.0):
    """Weights by the nearest bones' segments (1 / d^power), smoothed over the mesh: bone heat's stand-in."""
    D = []
    for b in bones:
        h, t, _ = S[b]
        d = t - h
        u = np.clip(((V - h) @ d) / max(d @ d, 1e-12), 0.0, 1.0)
        D.append(np.linalg.norm(V - (h + u[:, None] * d), axis=1))
    D = np.maximum(np.column_stack(D), 1e-4)
    w = D ** -power
    w /= w.sum(1, keepdims=True)
    w = tr.smooth_values(w, faces, passes=3, amount=0.5)
    return {b: w[:, k] for k, b in enumerate(bones)}


def skirt_weights(name, J, V):
    """A long coat below the hips (Abel.py's skirt_weights): front and sides on the skirt chain, the back on tail_01,
    the sides above the knee on skirt_l_01 / skirt_r_01."""
    theta = np.arctan2(V[:, 0], -V[:, 1])
    z = V[:, 2]
    knee_z = J['knee'][2]
    down = smoothstep(1.0, 0.86, z)
    front = smoothstep(2.3, 1.95, np.abs(theta))
    sides = smoothstep(math.radians(70.0), math.radians(95.0), np.abs(theta))
    knee = np.maximum(smoothstep(knee_z + 0.07, knee_z - 0.03, z), sides * smoothstep(knee_z + 0.27, knee_z + 0.13, z))
    side = smoothstep(math.radians(70.0), math.radians(100.0), np.abs(theta))
    thigh = down * front * (1.0 - knee)
    return {'pelvis': 1.0 - down, 'skirt_f_01': thigh * (1.0 - side), 'skirt_l_01': thigh * side * (V[:, 0] >= 0.0),
            'skirt_r_01': thigh * side * (V[:, 0] < 0.0), 'skirt_f_02': down * front * knee,
            'tail_01': down * (1.0 - front)}


def thigh_weights(name, J, V):
    """Trousers to the knee (Amos.py's thigh_weights): the waist on the pelvis, the thighs on skirt_f_01, the knees on
    skirt_f_02, the seat on the pelvis."""
    hips = np.array([0.09, 0.0, ps.HIP_Z])
    knee = np.array([0.1, J['knee'][1], J['knee'][2]])
    side = np.where(V[:, 0] >= 0.0, 1.0, -1.0)
    H = hips * np.column_stack([side, np.ones_like(side), np.ones_like(side)])
    K = knee * np.column_stack([side, np.ones_like(side), np.ones_like(side)])
    d = K - H
    t = ((V - H) * d).sum(1) / (d * d).sum(1)
    z = V[:, 2]
    leg = smoothstep(0.02, 0.28, t) * smoothstep(0.98, 0.9, z)
    leg = leg * (1.0 - smoothstep(0.02, 0.085, V[:, 1] - J['hips'][1]) * smoothstep(0.88, 0.78, z) *
                 smoothstep(0.45, 0.2, t))
    kn = smoothstep(0.84, 0.99, t) * leg
    return {'pelvis': 1.0 - leg, 'skirt_f_01': leg - kn, 'skirt_f_02': kn}


def finger_weights(name, rig, Vr, W):
    """At rest (after the warp), an open hand's fingers onto the rig's finger bones (nearest, past the knuckle);
    a fist keeps them on the hand (cfg 'fists': 'l', 'r', 'lr')."""
    fists = cfg.PEOPLE[name].get('fists', '')
    for s, sfx in SIDES:
        if sfx in fists:
            continue
        hand = W.get(f'hand_{sfx}')
        if hand is None:
            continue
        region = hand > 0.05
        names = [f'{f}_01_{sfx}' for f in FINGERS]
        dist, along = [], []
        for b in names:
            h, t = _bone(rig, b)
            d = unit(t - h)
            seg_t = np.clip((Vr - h) @ d, 0.0, 0.08)
            dist.append(np.linalg.norm(Vr - (h + seg_t[:, None] * d), axis=1))
            along.append((Vr - h) @ d)
        dist, along = np.column_stack(dist), np.column_stack(along)
        near = np.argmin(dist, axis=1)
        a = along[np.arange(len(Vr)), near]
        hold = smoothstep(FINGER_HOLD[0], FINGER_HOLD[1], a) * region
        for k, b in enumerate(names):
            W[b] = W.get(b, np.zeros(len(Vr))) + hand * hold * (near == k)
        W[f'hand_{sfx}'] = hand * (1.0 - hold)
    return W


# --- The mouth: the lips parted along the painted mouth, a pocket behind them ---

def cut_mouth(name, V, faces):
    """parts_unpaid_rig.cut_mouth on this person's mouth (cfg mouth_u, upright). Returns (V, faces, lip, pocket)."""
    m = cfg.PEOPLE[name]['mouth_u']
    zm, xm, hw, front = m['z'], m['x'], m['half_width'], m['y'] + 0.025
    obj = tr.new_object('_mouth', V, faces)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    region = [f for f in bm.faces if all(abs(v.co.x - xm) < hw + 0.012 and abs(v.co.z - zm) < 0.02 and v.co.y < front
                                         for v in f.verts)]
    geom = list({e for f in region for e in f.edges}) + list({v for f in region for v in f.verts}) + region
    bmesh.ops.bisect_plane(bm, geom=geom, plane_co=(0.0, 0.0, zm), plane_no=(0.0, 0.0, 1.0))
    lips = [e for e in bm.edges
            if all(abs(v.co.z - zm) < 2e-5 and abs(v.co.x - xm) < hw and v.co.y < front for v in e.verts)]
    bmesh.ops.split_edges(bm, edges=lips)
    bm.to_mesh(obj.data)
    bm.free()
    V, faces = tr.mesh_arrays(obj.data)
    bpy.data.objects.remove(obj)
    ps.log(f'{name}: mouth: {len(region)} faces round the lips, {len(lips)} edges parted')
    return _pocket(V, faces, zm, hw, front, xm)


def _pocket(V, faces, zm, hw, front, xm, depth=0.024, rings=5):
    on = (np.abs(V[:, 2] - zm) < 2e-5) & (np.abs(V[:, 0] - xm) < hw + 1e-4) & (V[:, 1] < front)
    lip = np.zeros(len(V))
    above = np.zeros(len(V))
    for f in faces:
        np.add.at(above, f, V[f, 2].mean() - zm)
    lip[on] = np.sign(above[on])
    loops = [lp for lp in tr.boundary_loops(faces) if on[lp].mean() > 0.5]
    if not loops:
        ps.log('mouth: no parted lips found (check cfg mouth)')
        return V, faces, lip, np.zeros(len(V))
    loop = max(loops, key=len)
    P = V[loop]
    middle = P.mean(0)
    side = lip[loop]
    new_V, new_F, new_lip = [], [], []
    base, prev, n = len(V), list(loop), len(loop)
    for k in range(1, rings):
        f = k / rings
        ring = middle + (P - middle) * (1.0 - f) + np.array([0.0, depth, 0.0]) * math.sqrt(1.0 - (1.0 - f) ** 2)
        ring[:, 2] += side * 0.016 * f * (1.0 - f)
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
    a, b, c = (V2[new_F[0][j]] for j in range(3))
    if np.cross(b - a, c - a)[1] > 0.0:
        new_F = [f[::-1] for f in new_F]
    pocket = np.concatenate([np.zeros(len(V)), np.ones(len(new_V))])
    return V2, list(faces) + [np.array(f) for f in new_F], np.concatenate([lip, new_lip]), pocket


def teeth(name, U):
    """Eight teeth on the head and eight on the jaw behind the lips (Unpaid.py's tooth_piece): [(V, faces, bone)]."""
    m = cfg.PEOPLE[name]['mouth_u']
    s = 0.0182          # the Unpaid's GLB unit, which its teeth were sized in
    zm, hw = m['z'], m['half_width']
    pieces = []
    for k in range(8):
        u = (k - 3.5) / 3.5
        for upper in (True, False):
            x = m['x'] + u * hw * 0.72
            y = m['y'] + (0.75 + 0.9 * u * u) * s
            if upper:
                base, tip, length = (x, y, zm + 0.28 * s), (0.0, -0.15, -1.0), 0.6 * s
            else:
                base, tip, length = (x, y + 0.1 * s, zm - 0.3 * s), (0.0, -0.15, 1.0), 0.5 * s
            Vt, Ft = U['tooth_piece'](np.array(base), U['unit'](np.array(tip)), length, 0.23 * s)
            pieces.append((Vt, Ft, 'head' if upper else 'jaw'))
    return pieces
