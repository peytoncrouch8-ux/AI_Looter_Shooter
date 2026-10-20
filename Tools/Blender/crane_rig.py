"""Hollis Crane's rig (Art/Models/Characters/Crane.py's rig step), as Ellis.py's: the UE5 mannequin's 63 deform bones on
MakeHuman's joints (looter_human, posed by crane_common's stoop), every part weighted by what it is, the forearms' twist
bones, the swing chains the game's springs move (Player/Animation/BodySwing.cpp: skirt_, cape_, hair_, lantern_), the
hat's bone, the cape's idle arms, the hit zones; one joined mesh 'Crane'. Then the game's materials, the viewer's, the
turn to face like the mannequin, and the rig check (--rig-check: the swing bones at rest and bent, in clay).

Not in the bake's digest: editing the rig re-bakes nothing.
"""
import math
import os

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Quaternion, Vector

import crane_clothes as cw
import crane_common as cc
import crane_hair as chh
import looter_bake as bk
import looter_garments as gm
import looter_human as hu
import looter_model as lm
import looter_textures as lt
from crane_common import FWD, HEM, log, round_angle

TWIST = (('lowerarm_twist_01', 0.5), ('lowerarm_twist_02', 0.9))
WRIST_BEND = (0.78, 1.25)
END_BONE = 0.02


def forearm_share(fig, s, P):
    e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
    length = np.linalg.norm(w - e)
    return ((P - e) @ ((w - e) / length)) / length


def rigid_w(fig, n, parts):
    return hu.blend(fig.names, n, parts)


def torso_faces(fig):
    if getattr(fig, '_torso_faces', None) is None:
        # (not the hands: in the bind pose they hang by the coat's sides, which took their weights)
        fig._torso_faces = [f for f, k in zip(fig.faces, fig.face_mask((fig.w_arm < 0.3) & (fig.w_hand < 0.1) &
                                                                      (fig.w_head < 0.3))) if k]
    return fig._torso_faces


def skirt_top(fig):
    return fig.hip_z - 0.07


def per_link(fig, P, n_link, faces):
    """Weights for a part made of rigid pieces of n_link points each (chain links): each piece wholly on the weights
    at its centre, from the body's faces under it."""
    k = len(P) // n_link
    centres = P[:k * n_link].reshape(k, n_link, 3).mean(1)
    W = fig.b.weights_at(centres, faces)[1]
    return np.repeat(W, n_link, axis=0)


def hair_weights(fig, P):
    """The hair: the head down to below the jaw, then (lying on the collar, the cape's shoulders and the chest) the
    neck's and the chest's: the swing chains take the long ends over (swing_weights)."""
    t = gm.smoothstep(fig.eye_z - 0.07, fig.eye_z - 0.20, P[:, 2])
    return hu.top4(rigid_w(fig, len(P), {'head': 1.0 - t, 'neck_01': 0.5 * t, 'spine_05': 0.5 * t}))


def weights(fig, ob):
    """The mannequin bone weights of ob's vertices, by what the part is."""
    me = ob.data
    P = np.array([v.co[:] for v in me.vertices])
    n = len(P)
    name = ob.name
    sk = fig.sk
    if name == 'Crane':
        return fig.b.weights_at(P)[1]
    if name.startswith(('Eye_', 'Hat', 'Coin', 'Mustache', 'Brows')):
        return rigid_w(fig, n, {'head': 1.0})
    if name.startswith('Hair'):
        return hair_weights(fig, P)
    if name.startswith('ShirtBand'):
        return fig.b.weights_at(P)[1]
    if name.startswith(('Shirt', 'Ember')):
        return fig.b.weights_at(P, torso_faces(fig))[1]
    if name.startswith('Collar'):
        t = gm.smoothstep(fig.neck[2] - 0.03, fig.neck[2] + 0.06, P[:, 2])
        return rigid_w(fig, n, {'spine_05': 1.0 - 0.5 * t, 'neck_01': 0.5 * t})
    if name.startswith('Cape'):
        return cw.cape_weights(fig, ob)
    if name.startswith('Coat'):
        names, near = fig.b.weights_at(P, torso_faces(fig))
        hip, knee = fig.hip_z, sk['thigh_l'][1][2]
        t = np.clip((hip - P[:, 2]) / (hip - knee), 0.0, 1.0)
        leg = 0.75 * t ** 0.8
        side = gm.smoothstep(0.0, 1.0, 0.5 + P[:, 0] / 0.36)
        skirt = hu.blend(fig.names, n, {'pelvis': 1.0 - leg, 'thigh_l': leg * side, 'thigh_r': leg * (1.0 - side)})
        k = gm.smoothstep(hip + 0.06, hip - 0.06, P[:, 2])[:, None]
        return hu.top4((1.0 - k) * near + k * skirt)
    if name.startswith(('Belt', 'Buckle')):
        return rigid_w(fig, n, {'pelvis': 0.85, 'spine_01': 0.15})
    if name.startswith('Chain'):
        return per_link(fig, P, ob['LinkPoints'], torso_faces(fig))
    if name.startswith(('Hook', 'HookLinks', 'HookEye')):
        return np.zeros((n, len(fig.names)))           # all lantern_01 (swing_weights)
    if name.startswith('Soles'):
        out = np.zeros((n, len(fig.names)))
        for s in 'lr':
            ankle, ball = sk[f'foot_{s}'][0], sk[f'ball_{s}'][0]
            f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
            sel = (P[:, 0] * ball[0]) > 0
            t = gm.smoothstep(0.10, 0.15, (P[sel] - ankle) @ f)
            out[sel] = hu.blend(fig.names, int(sel.sum()), {f'foot_{s}': 1.0 - t, f'ball_{s}': t})
        return out
    if name.startswith('Cuff'):
        s = name.split('_')[1][0]
        return fig.b.weights_at(P, [f for f, k in zip(fig.faces, fig.face_mask(
            fig.Wv[:, fig.col[f'lowerarm_{s}']] + fig.Wv[:, fig.col[f'hand_{s}']] > 0.3)) if k])[1]
    return fig.b.weights_at(P)[1]


# ------------------------------------------------------------------------------------------------ the swing chains

SKIRT_CHAINS = (('f', 1 / 6), ('s', 1 / 2), ('b', None), ('bi', 'inner'))
INNER_AT = 0.2
CAPE_CHAINS = (('f', 0.09, 'surface'), ('a', 0.24, 'joint'), ('s', 'arm', 'joint'), ('b', 0.64, 'joint'))
HAIR_PARENT = 'neck_01'      # the hair's chains hang from the lower neck: the long ends lie on the collar, the cape and
                             # the chest, which turn with the chest, not the head
HOOK_PARENT = 'spine_05'


def on_surface(tree, centre, ang, z):
    ex, ey = gm.radial(np.float64(ang), FWD)
    for d in np.radians((0, 2, -2, 4, -4, 6, -6, 9, -9, 12, -12, 16, -16, 20, -20)):
        dx, dy = gm.radial(np.float64(ang + d), FWD)
        hit = tree.ray_cast(Vector((centre[0] + dx, centre[1] + dy, z)), Vector((-dx, -dy, 0.0)), 1.0)
        if hit[0] is not None:
            r = math.hypot(hit[0].x - centre[0], hit[0].y - centre[1])
            return np.array([centre[0] + ex * r, centre[1] + ey * r, hit[0].z])
    raise ValueError(f'no garment at {math.degrees(ang):.0f} degrees, {z:.3f} m')


def front_edge(co, edges, centre, z, side):
    a, b = co[edges[:, 0]], co[edges[:, 1]]
    da, db = a[:, 2] - z, b[:, 2] - z
    cut = (da * db <= 0.0) & (da != db)
    p = a[cut] + (b[cut] - a[cut]) * (da[cut] / (da[cut] - db[cut]))[:, None]
    ang = round_angle(p, centre) * side
    return float(ang[ang > 0].min())


def hem_point(co, centre, ang, width=math.radians(6)):
    near = co[np.abs((round_angle(co, centre) - ang + math.pi) % (2 * math.pi) - math.pi) < width]
    p = near[np.argmin(near[:, 2])]
    r = math.hypot(p[0] - centre[0], p[1] - centre[1])
    ex, ey = gm.radial(np.float64(ang), FWD)
    return np.array([centre[0] + ex * r, centre[1] + ey * r, p[2]])


def mesh_arrays(ob):
    ed = np.empty(len(ob.data.edges) * 2, dtype=np.int64)
    ob.data.edges.foreach_get('vertices', ed)
    return gm._object_coords(ob), ed.reshape(-1, 2)


def hair_line(co, centre, ang, width, zs):
    """The hair's middle depth at each height along angle ang round centre (the cards' median distance out within
    width of it): a chain's joints."""
    a = round_angle(co, centre)
    sel = co[np.abs((a - ang + math.pi) % (2 * math.pi) - math.pi) < width]
    r_all = np.hypot(sel[:, 0] - centre[0], sel[:, 1] - centre[1])
    d = np.array(gm.radial(np.float64(ang), FWD))
    out = []
    for z in zs:
        near = np.abs(sel[:, 2] - z) < 0.015
        r = float(np.median(r_all[near])) if near.any() else float(np.median(r_all))
        out.append(np.array([centre[0] + d[0] * r, centre[1] + d[1] * r, z]))
    return out


def swing_layout(fig, parts):
    """The swing chains placed on the built game parts: [{name, parent, part, side, centre, joints}]."""
    by = {o.name.split('.')[0]: o for o in parts}
    sk = fig.sk
    chains = []

    def chain(name, parent, part, side, centre, joints):
        chains.append(dict(name=name, parent=parent, part=part, side=side, centre=np.asarray(centre)[:2],
                           joints=np.array(joints)))

    # the coat's skirt round its own axis: from the belt line, mid-thigh and the knee down to the hem
    co, ed = mesh_arrays(by['Coat'])
    tree = gm.bvh_objects([by['Coat']])
    knee = (sk['calf_l'][0][2] + sk['calf_r'][0][2]) * 0.5
    mid_thigh = ((sk['thigh_l'][0][2] + sk['thigh_r'][0][2]) * 0.5 + knee) * 0.5
    belt = fig.waist - 0.012
    for side, s in ((1, 'l'), (-1, 'r')):
        heights = (belt, mid_thigh, knee, HEM + 0.10)
        edge = [front_edge(co, ed, fig.centre, z, side) for z in heights]
        behind = float(round_angle(np.array([sk[f'calf_{s}'][0][0], fig.centre[1] + 0.18]), fig.centre)[0])
        for key, u in SKIRT_CHAINS:
            if u == 'inner':
                a = [behind + (side * math.pi - behind) * INNER_AT] * 4
            else:
                a = [side * (e + u * (math.pi - e)) for e in edge] if u is not None else [behind] * 4
            joints = [on_surface(tree, fig.centre, a[0], belt)]
            joints += [on_surface(tree, fig.centre, a[k], heights[k]) for k in (1, 2)]
            joints.append(hem_point(co, fig.centre, a[3]))
            chain(f'skirt_{key[0]}{s}{key[1:]}', 'pelvis', 'Coat', side, fig.centre, joints)
    joints = [on_surface(tree, fig.centre, math.pi, z) for z in (belt, mid_thigh, knee)]
    chain('skirt_bm', 'pelvis', 'Coat', 0, fig.centre, joints + [hem_point(co, fig.centre, math.pi)])
    # the cape round the neck: from the shoulder line down to its hem
    co, ed = mesh_arrays(by['Cape'])
    tree = gm.bvh_objects([by['Cape']])
    centre = fig.neck[:2]
    for side, s in ((1, 'l'), (-1, 'r')):
        e = front_edge(co, ed, centre, fig.shoulder_z, side)
        for key, u, root in CAPE_CHAINS:
            a = float(round_angle(fig.sk[f'upperarm_{s}'][0], centre)[0]) if u == 'arm' else side * (e + u * (math.pi - e))
            hem = hem_point(co, centre, a)
            cloth = [on_surface(tree, centre, a, z) for z in (fig.shoulder_z, (fig.shoulder_z + hem[2]) * 0.5)] + [hem]
            if root == 'joint':
                chain(f'cape_{key}{s}', f'clavicle_{s}', 'Cape', side, centre, [fig.sk[f'upperarm_{s}'][0]] + cloth[1:])
            else:
                chain(f'cape_{key}{s}', 'spine_05', 'Cape', side, centre, cloth)
            chains[-1]['ring'] = np.array(cloth)
    hem = hem_point(co, centre, math.pi)
    chain('cape_bm', 'spine_05', 'Cape', 0, centre,
          [on_surface(tree, centre, math.pi, z) for z in (fig.shoulder_z, (fig.shoulder_z + hem[2]) * 0.5)] + [hem])
    # the hair: down the middle of the back from the nape (as it leaves the collar) to where most of it ends; and the
    # two front locks, in front of each shoulder, from the jaw's line down to their ends
    co, _ = mesh_arrays(by['Hair'])
    hc = fig.hair_centre[:2]
    a = round_angle(co, hc)
    back = co[np.abs(np.abs(a) - math.pi) < math.radians(25)]
    top, end = fig.neck[2] + 0.03, float(np.percentile(back[:, 2], 6))
    chain('hair', HAIR_PARENT, 'Hair', 0, hc, hair_line(co, hc, math.pi, math.radians(25), (top, (top + end) * 0.5, end)))
    for side, s in ((1, 'l'), (-1, 'r')):
        lock = co[(a * side > math.radians(40)) & (a * side < math.radians(110))]
        top, end = fig.eye_z - 0.13, float(np.percentile(lock[:, 2], 6))
        ang = side * math.radians(70)
        chain(f'hair_f{s}', HAIR_PARENT, 'Hair', side, hc,
              hair_line(co, hc, ang, math.radians(30), (top, (top + end) * 0.5, end)))
    # the hook swings from its links on the chain
    chain('lantern', HOOK_PARENT, 'Hook', 0, fig.centre, list(fig.hook_line))
    for c in chains:
        j = c['joints']
        log(f'swing {c["name"]}: ' + ' > '.join(f'({p[0] * 100:.1f}, {p[1] * 100:.1f}, {p[2] * 100:.1f})' for p in j) + ' cm')
    return chains


def swing_bones(chains):
    out = []
    for c in chains:
        j = c['joints']
        for k in range(len(j) - 1):
            name = f'{c["name"]}_{k + 1:02d}'
            out.append((name, j[k], j[k + 1], c['parent'] if k == 0 else f'{c["name"]}_{k:02d}', c))
    return out


def add_swing_bones(arm, chains):
    bk.select([arm], arm)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones
    bones = swing_bones(chains)
    ends = [(f'{c["name"]}_end', c['joints'][-1], c['joints'][-1] + gm.unit(c['joints'][-1] - c['joints'][-2]) * END_BONE,
             f'{c["name"]}_{len(c["joints"]) - 1:02d}', c) for c in chains]
    for name, h, t, parent, c in bones + ends:
        e = edit.new(name)
        e.head, e.tail = Vector(h), Vector(t)
        out = np.array([h[0] - c['centre'][0], h[1] - c['centre'][1], 0.0])
        if np.linalg.norm(out) < 1e-6:
            out = np.array([0.0, -1.0, 0.0])
        e.align_roll(Vector(tuple(gm.unit(out))))
        e.use_connect = False
        e.use_deform = True
        e.parent = edit[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    return [b[0] for b in bones]


def soft_top4(W):
    W = np.asarray(W, dtype=np.float64).copy()
    if W.shape[1] > 4:
        fifth = np.sort(W, axis=1)[:, -5][:, None]
        W = np.where(W > fifth, W - fifth, 0.0)
    s = W.sum(axis=1, keepdims=True)
    return np.where(s > 1e-9, W / np.maximum(s, 1e-9), 0.0)


def along(z, zs):
    out = np.zeros((len(z), len(zs) - 1))
    out[:, 0] = 1.0
    for k in range(1, len(zs) - 1):
        moved = out[:, k - 1] * gm.smoothstep(zs[k], zs[k + 1], z)
        out[:, k - 1] -= moved
        out[:, k] += moved
    return out


def ring_weights(P, chains):
    n = len(P)
    out = np.zeros((n, len(chains)))
    centre = chains[0]['centre']
    ang = round_angle(P, centre)
    side = np.where(ang >= 0.0, 1.0, -1.0)
    mid = [i for i, c in enumerate(chains) if c['side'] == 0]
    for s in (1.0, -1.0):
        sel = side == s
        if not sel.any():
            continue
        own = [i for i, c in enumerate(chains) if c['side'] == s]
        last = mid[-1] if mid else [i for i, c in enumerate(chains) if c['side'] == -s][-1]
        th = ang[sel] * s
        th = np.where(th < 0.0, th + 2 * math.pi, th)
        z = P[sel, 2]

        def at(i):
            j = chains[i].get('ring', chains[i]['joints'])
            a = round_angle(j, centre)
            a = np.abs(a) if chains[i]['side'] == 0 else a * chains[i]['side']
            return np.interp(z, j[::-1, 2], a[::-1])
        knots = np.stack([at(i) for i in own] + [at(last) if mid else 2 * math.pi - at(last)], 1)
        w = np.zeros((int(sel.sum()), knots.shape[1]))
        w[:, 0] = th <= knots[:, 0]
        for i in range(knots.shape[1] - 1):
            lo, hi = knots[:, i], knots[:, i + 1]
            inside = (th > lo) & (th <= hi)
            t = np.clip((th - lo) / np.maximum(hi - lo, 1e-6), 0.0, 1.0)
            w[inside, i] += 1.0 - t[inside]
            w[inside, i + 1] += t[inside]
        w[:, -1] += th > knots[:, -1]
        sub = np.zeros((int(sel.sum()), len(chains)))
        for k, i in enumerate(own + [last]):
            sub[:, i] += w[:, k]
        out[sel] = sub
    return out


def swing_weights(fig, ob, W):
    """ob's weights with the swing bones' columns after the mannequin's: the part's own weights give way to its
    chains down the cloth (Ellis.py's); the hair to its back chain behind and its locks' in front; the hook and its
    links all on lantern_01."""
    chains = fig.swing
    bones = swing_bones(chains)
    col = {b[0]: i for i, b in enumerate(bones)}
    P = np.array([v.co[:] for v in ob.data.vertices])
    n = len(P)
    C = np.zeros((n, len(bones)))
    f = np.zeros(n)
    name = ob.name

    def spread(share, c, top):
        j = c['joints']
        a = along(P[:, 2], j[:, 2])
        for k in range(a.shape[1]):
            C[:, col[f'{c["name"]}_{k + 1:02d}']] += share * a[:, k]
        return share * gm.smoothstep(top, j[1, 2], P[:, 2])

    if name.startswith(('Hook', 'HookLinks', 'HookEye')):
        C[:, col['lantern_01']] = 1.0
        f[:] = 1.0
    elif name.startswith(('Coat', 'Cape')):
        part = [c for c in chains if c['part'] == name.split('.')[0].split('_')[0]]
        share = ring_weights(P, part)
        for i, c in enumerate(part):
            f += spread(share[:, i], c, skirt_top(fig) if name.startswith('Coat') else c['joints'][0, 2])
        if 'CapeChain' in ob.data.attributes:
            f = np.empty(n, dtype=np.float32)
            ob.data.attributes['CapeChain'].data.foreach_get('value', f)
            f = np.clip(f.astype(np.float64), 0.0, 1.0)
    elif name.startswith('Hair'):
        hc = fig.hair_centre[:2]
        ang = round_angle(P, hc)
        by = {c['name']: c for c in chains}
        back = gm.smoothstep(math.radians(105), math.radians(135), np.abs(ang))
        for cname, share in (('hair', back), ('hair_fl', (1.0 - back) * (ang > 0)), ('hair_fr', (1.0 - back) * (ang <= 0))):
            f += spread(share, by[cname], by[cname]['joints'][0, 2])
    if not f.any():
        return np.concatenate([W, C], 1)
    C = C / np.maximum(C.sum(1, keepdims=True), 1e-9) * f[:, None]
    return soft_top4(np.concatenate([W * (1.0 - f)[:, None], C], 1))


# ------------------------------------------------------------------------------------- the other added bones

def add_hat_bone(arm, head, tail):
    bk.select([arm], arm)
    bpy.ops.object.mode_set(mode='EDIT')
    e = arm.data.edit_bones.new('hat')
    e.head, e.tail = Vector(head), Vector(tail)
    e.align_roll(Vector((0.0, 0.0, 1.0)))
    e.use_connect, e.use_deform = False, True
    e.parent = arm.data.edit_bones['head']
    bpy.ops.object.mode_set(mode='OBJECT')
    return 'hat'


def add_twist_bones(arm):
    bk.select([arm], arm)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones
    names = []
    for s in 'lr':
        lo = edit[f'lowerarm_{s}']
        h, t = lo.head.copy(), lo.tail.copy()
        for name, f in TWIST:
            e = edit.new(f'{name}_{s}')
            e.head, e.tail = h + (t - h) * f, h + (t - h) * min(f + 0.1, 1.0)
            e.roll = lo.roll
            e.use_connect, e.use_deform = False, True
            e.parent = lo
            names.append(e.name)
    bpy.ops.object.mode_set(mode='OBJECT')
    return names


def add_cape_arm_bones(fig, arm):
    bk.select([arm], arm)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones
    for sx, (h, R) in cw.arm_turns(fig).items():
        s = 'l' if sx > 0 else 'r'
        up = edit[f'upperarm_{s}']
        e = edit.new(f'cape_arm_{s}')
        turn = Matrix.Translation(Vector(tuple(h))) @ Matrix(R.tolist()).to_4x4() @ Matrix.Translation(-Vector(tuple(h)))
        e.head, e.tail = up.head.copy(), up.tail.copy()
        e.matrix = turn @ up.matrix
        e.length = up.length
        e.use_connect, e.use_deform = False, True
        e.parent = edit[f'clavicle_{s}']
    bpy.ops.object.mode_set(mode='OBJECT')


def twist_weights(fig, ob, W, names):
    """The forearm's turn spread down it (Ellis.py's): on the sleeves, the cuffs and the bare forearm and hand each
    vertex's lowerarm and hand weight shared by how far down the forearm it is."""
    if not ob.name.startswith(('Sleeves', 'Cuff', 'Crane')):
        return W
    P = gm._object_coords(ob)
    col = {n: i for i, n in enumerate(names)}
    knots = np.array([0.0] + [f for _, f in TWIST])
    for s in 'lr':
        cols = [col[f'lowerarm_{s}']] + [col[f'{n}_{s}'] for n, _ in TWIST] + [col[f'hand_{s}']]
        total = W[:, cols[0]] + W[:, cols[-1]]
        t = forearm_share(fig, s, P)
        sel = (total > 1e-4) & (t < 1.2) & (t > -0.2)
        if not sel.any():
            continue
        hand = gm.smoothstep(WRIST_BEND[0], WRIST_BEND[1], t[sel])
        tt = np.clip(t[sel], 0.0, knots[-1])
        armw = np.stack([np.interp(tt, knots, np.eye(len(knots))[k]) for k in range(len(knots))], 1)
        share = np.concatenate([armw * (1.0 - hand)[:, None], hand[:, None]], 1)
        W[np.ix_(np.nonzero(sel)[0], cols)] = total[sel, None] * share
    return soft_top4(W)


def hit_zones(fig, arm):
    """Hit zones (the physics asset's bodies): the head with its hat, the trunk in its coat and cape, the coat's skirt
    round the hips, each limb, the hands, the feet."""
    sk = fig.sk
    mid = lambda b, t=0.5: sk[b][0] + (sk[b][1] - sk[b][0]) * t
    lm.hit_sphere(arm, 'head', tuple(mid('head', 0.55)), 0.16)
    cy = fig.centre[1]
    lm.hit_capsule(arm, 'spine_03', (0.0, cy, fig.waist), (0.0, cy - 0.01, fig.shoulder_z - 0.12), 0.21)
    lm.hit_capsule(arm, 'pelvis', (0.0, cy, fig.hip_z - 0.40), (0.0, cy, fig.hip_z - 0.08), 0.24)
    for s in 'lr':
        for bone, r in (('upperarm', 0.075), ('lowerarm', 0.06), ('thigh', 0.11), ('calf', 0.085)):
            a, b = sk[f'{bone}_{s}'][0], sk[f'{bone}_{s}'][1]
            lm.hit_capsule(arm, f'{bone}_{s}', tuple(a + (b - a) * 0.15), tuple(a + (b - a) * 0.85), r)
        lm.hit_sphere(arm, f'hand_{s}', tuple(mid(f'hand_{s}', 0.6)), 0.065)
        lm.hit_capsule(arm, f'foot_{s}', tuple(sk[f'foot_{s}'][0] + np.array([0, 0.04, -0.03])),
                       tuple(sk[f'ball_{s}'][0] + np.array([0, -0.035, -0.02])), 0.06)
    for ob in arm.children:
        if ob.name.startswith(('USP_', 'UCP_')):
            ob.hide_render = True


def ride(under, co, n_piece, over=None, axis=None):
    """Weights for rigid pieces of n_piece points each (the chain's links, the star) lying on a part (under: its points
    and its finished weights, the coat's): each piece wholly on the weights of the part's point nearest its middle, so it
    moves with the cloth under it. (From the body's faces, the links at the hip took the thigh's and rose 4 cm into the
    coat crouching.) With over (the cape's points and weights): a piece the cape covers (within 4 cm, the cape further
    out from the axis than it) rides the cape instead, so the cape lifting with a raised arm carries it rather than
    coming down through it (the links over the right shoulder came 3 cm through the cape aiming, audit r2)."""
    from mathutils import kdtree

    def kd(pts):
        t = kdtree.KDTree(len(pts))
        for i, p in enumerate(pts):
            t.insert(p, i)
        t.balance()
        return t
    pts, W = under
    tree = kd(pts)
    k = max(1, len(co) // n_piece)
    centres = co[:k * n_piece].reshape(k, n_piece, 3).mean(1)
    rows = np.array([W[tree.find(c)[1]] for c in centres])
    if over is not None:
        cp, cw_ = over
        ctree = kd(cp)
        low = axis_z(cp)
        moved = 0
        for i, c in enumerate(centres):
            _p, j, d = ctree.find(c)
            if d > 0.05 or c[2] < low:
                continue
            # outward from the body's axis, turning upward over the shoulders' top (there the cape is above the link)
            out = np.array([c[0] - axis[0], c[1] - axis[1], 0.0])
            out = gm.unit(gm.unit(out) + np.array([0.0, 0.0, 1.5]) * float(gm.smoothstep(low + 0.10, low + 0.20, c[2])))
            if (cp[j] - c) @ out > 0.0:
                rows[i] = cw_[j]
                moved += 1
        log(f'chain: {moved} of {k} links ride the cape over them')
    return np.repeat(rows, n_piece, axis=0)


def axis_z(cape_pts):
    """Near the cape's lowest point (its hem): links under the cape are above it."""
    return float(np.percentile(cape_pts[:, 2], 2))


def drop_hidden_links(fig, objects):
    """The chain's links over the top of the right shoulder go from the game mesh: they lie under the cape's top and the
    collar, always hidden, and their edge-on links stood out through the cape by the back of the neck (audit r2/r3).
    Every link shares one UV island, so the bake is untouched."""
    for ob in objects:
        if not ob.name.startswith('Chain') or 'LinkPoints' not in ob:
            continue
        n = int(ob['LinkPoints'])
        co = gm._object_coords(ob)
        k = len(co) // n
        centres = co[:k * n].reshape(k, n, 3).mean(1)
        drop = np.nonzero(centres[:, 2] > fig.shoulder_z + 0.005)[0]
        if not len(drop):
            continue
        bm = bmesh.new()
        bm.from_mesh(ob.data)
        bm.verts.ensure_lookup_table()
        bmesh.ops.delete(bm, geom=[bm.verts[i * n + j] for i in drop for j in range(n)], context='VERTS')
        bm.to_mesh(ob.data)
        bm.free()
        ob.data.update()
        log(f'chain: {len(drop)} links over the top of the shoulder dropped (hidden under the cape and collar)')


def rig(fig, objects):
    """Weights every part, joins them into one mesh 'Crane' (SK_Crane) under the mannequin armature with the forearms'
    twist bones, the swing bones, the hat's bone and the cape's idle arms."""
    drop_hidden_links(fig, objects)
    arm = fig.b.armature('Crane_Rig')
    fig.swing = swing_layout(fig, objects)
    fig.hat = cc.HAT['pivot']
    log(f'hat bone: ({fig.hat[0][0] * 100:.1f}, {fig.hat[0][1] * 100:.1f}, {fig.hat[0][2] * 100:.1f}) > '
        f'({fig.hat[1][0] * 100:.1f}, {fig.hat[1][1] * 100:.1f}, {fig.hat[1][2] * 100:.1f}) cm')
    names = fig.names + add_swing_bones(arm, fig.swing) + [add_hat_bone(arm, *fig.hat)] + add_twist_bones(arm)
    add_cape_arm_bones(fig, arm)
    fig.rig_names = names
    fig.rest = {}
    coat = cape = None
    first = lambda o: 0 if o.name.startswith('Coat') else 1 if o.name.startswith('Cape') else 2
    for ob in sorted(objects, key=first):
        co = gm._object_coords(ob)
        if ob.name.startswith(('Chain', 'Star')) and coat is not None:
            W = ride(coat, co, ob.get('LinkPoints', len(co)), cape if ob.name.startswith('Chain') else None,
                     fig.neck[:2])
        else:
            W = swing_weights(fig, ob, weights(fig, ob))
            hat = np.full((len(W), 1), 1.0 if ob.name.startswith('Hat') else 0.0)
            W = np.concatenate([W * (1.0 - hat), hat, np.zeros((len(W), len(names) - W.shape[1] - 1))], 1)
            W = twist_weights(fig, ob, W, names)
        if ob.name.startswith('Coat'):
            coat = (co, W)
        if ob.name.startswith('Cape'):
            cape = (co, W)
        fig.rest[ob.name] = (co, [list(p.vertices) for p in ob.data.polygons], W)
        for j, bone in enumerate(names):
            c = W[:, j]
            nz = np.nonzero(c > 1e-4)[0]
            if not len(nz):
                continue
            g = ob.vertex_groups.get(bone) or ob.vertex_groups.new(name=bone)
            for i in nz:
                g.add([int(i)], float(c[i]), 'REPLACE')
    skin = next(o for o in objects if o.name == 'Crane')
    for ob in objects:
        for attr in ('mh_index', 'CapeChain', 'Col', 'Row', 'Tear') + tuple('CapeW_' + b for b in cw.CAPE_SKIN + ('spine_05',)):
            if attr in ob.data.attributes:
                ob.data.attributes.remove(ob.data.attributes[attr])
    bk.select(objects, skin)
    bpy.ops.object.join()
    skin = bpy.context.view_layer.objects.active
    skin.name = skin.data.name = 'Crane'
    skin.parent = arm
    mod = skin.modifiers.new('Armature', 'ARMATURE')
    mod.object = arm
    arm['LODs'] = '50,25'
    arm['LODScreens'] = '0.5,0.2'
    hit_zones(fig, arm)
    log(f'rigged: {len(skin.data.polygons)} faces, {cc.tris_of(skin)} triangles, {len(skin.vertex_groups)} weighted bones, '
        f'{len(arm.data.bones)} bones')
    return arm, skin


# ------------------------------------------------------------------------------------------------- materials

def game_materials(skin, sets, made, game, glows):
    """The looks swapped for the game's (Ellis.py's): a textured material per GAME_MATERIALS slot, Glow for the glows."""
    swap = {}
    for slot_name, (master, keys, params) in game.items():
        found = {s for s, ks in {**sets, **made}.items() if set(keys) & set(ks)}
        assert len(found) == 1, f'{slot_name}: its looks must bake into one set, not {found}'
        mat = lt.material(found.pop(), name=slot_name, master=master, **params)
        for k in keys:
            swap[f'Crane_{k}'] = mat
    for k, (hex_value, strength) in glows.items():
        # CraneEyeGlow, CraneEmberGlow
        swap[f'Crane_{k}'] = lm.material(f'Crane{k}' if k.endswith('Glow') else f'Crane{k}Glow', hex_value, 'Glow',
                                         Glow=strength)
    for i, s_ in enumerate(skin.material_slots):
        if s_.material is not None and s_.material.name in swap:
            skin.material_slots[i].material = swap[s_.material.name]
    bk.select([skin])
    bpy.ops.object.material_slot_remove_unused()


def viewer_materials(skin, baked, sets, made, glows):
    """The baked maps on a Principled BSDF for previews (base colour, normal, roughness and metal from the ORM),
    emission for the glows; the hair's atlas as its own."""
    mats = {}
    for set_name, keys in sets.items():
        imgs = baked[set_name]['images']
        mat = bpy.data.materials.new(f'View_{set_name}')
        mat.use_nodes = True
        nt = mat.node_tree
        nt.nodes.clear()
        out = nt.nodes.new('ShaderNodeOutputMaterial')
        b = nt.nodes.new('ShaderNodeBsdfPrincipled')
        nt.links.new(b.outputs[0], out.inputs['Surface'])
        col = nt.nodes.new('ShaderNodeTexImage')
        col.image = imgs['COLOR']
        orm = nt.nodes.new('ShaderNodeTexImage')
        orm.image = imgs['ORM']
        sep = nt.nodes.new('ShaderNodeSeparateColor')
        nt.links.new(orm.outputs['Color'], sep.inputs[0])
        # the occlusion multiplied in (the game's DiffuseAO)
        mul = nt.nodes.new('ShaderNodeMix')
        mul.data_type, mul.blend_type = 'RGBA', 'MULTIPLY'
        mul.inputs[0].default_value = 0.7
        nt.links.new(col.outputs['Color'], mul.inputs[6])
        nt.links.new(sep.outputs[0], mul.inputs[7])
        nt.links.new(mul.outputs[2], b.inputs['Base Color'])
        nt.links.new(sep.outputs[1], b.inputs['Roughness'])
        nt.links.new(sep.outputs[2], b.inputs['Metallic'])
        nrm = nt.nodes.new('ShaderNodeTexImage')
        nrm.image = imgs['NORMAL']
        nm = nt.nodes.new('ShaderNodeNormalMap')
        nt.links.new(nrm.outputs['Color'], nm.inputs['Color'])
        nt.links.new(nm.outputs['Normal'], b.inputs['Normal'])
        if set_name.endswith('Skin'):
            b.inputs['Subsurface Weight'].default_value = 0.15
            b.inputs['Subsurface Radius'].default_value = (1.0, 0.5, 0.35)
            b.inputs['Subsurface Scale'].default_value = 0.004
        for k in keys:
            mats[f'Crane_{k}'] = mat
    for set_name, keys in made.items():
        mat = chh.material(f'View_{set_name}')
        for k in keys:
            mats[f'Crane_{k}'] = mat
    for k in glows:
        mats[f'Crane_{k}'] = bpy.data.materials.get(f'Crane_{k}')
    for i, s_ in enumerate(skin.material_slots):
        if s_.material is not None and s_.material.name in mats:
            skin.material_slots[i].material = mats[s_.material.name]


VIEW_POSE = {'upperarm_l': ((0.0, 1.0, 0.0), 34.0), 'upperarm_r': ((0.0, 1.0, 0.0), -34.0),
             'lowerarm_l': ((1.0, 0.0, 0.0), 14.0), 'lowerarm_r': ((1.0, 0.0, 0.0), 14.0)}


def pose_for_view(arm):
    """A relaxed stance for the previews: the arms let down from the bind pose's A toward the sides."""
    for bone, (axis, deg) in VIEW_POSE.items():
        pb = arm.pose.bones[bone]
        local = pb.bone.matrix_local.to_3x3().inverted() @ Vector(axis)
        pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = Quaternion(local.normalized(), math.radians(deg))
    bpy.context.view_layer.update()


def face_like_mannequin(arm, skin):
    """The finished figure turned a quarter about Z into its data, the last step before the export: in Unreal SK_Crane
    faces +Y with his left at +X, as SKM_Manny_Simple (Ellis.py's)."""
    turn = Matrix.Rotation(math.radians(-90.0), 4, 'Z')
    bk.select([arm], arm)
    arm.data.transform(turn)
    skin.data.transform(turn)
    for ob in arm.children:
        if ob is not skin:
            ob.matrix_world = turn @ ob.matrix_world
    bpy.context.view_layer.update()
    toe = arm.data.bones['ball_l'].head_local - arm.data.bones['foot_l'].head_local
    log(f'turned to face -X here (+Y in Unreal): foot_l to ball_l {tuple(round(c, 3) for c in toe.normalized())}')


# --------------------------------------------------------------------------------------------- the rig check

FWD3, BACK3, LEFT3, UP3 = (0.0, -1.0, 0.0), (0.0, 1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0)
HAT_TIP = 20.0
SWING_TESTS = (
    ('skirt_fl', (('skirt_fl', FWD3, 35.0),), 'skirt', ('front', 'side')),
    ('skirt_bl', (('skirt_bl', BACK3, 35.0),), 'skirt', ('front', 'side', 'back')),
    ('cape_bl', (('cape_bl', BACK3, 25.0),), 'upper', ('front', 'side', 'back')),
    ('hair', (('hair', BACK3, 30.0),), 'upper', ('side', 'back')),
    ('hair_fl', (('hair_fl', FWD3, 30.0),), 'upper', ('front', 'side')),
    ('lantern', (('lantern', LEFT3, 30.0),), 'upper', ('front', 'side')),
    ('hat_tip', (('hat', UP3, HAT_TIP),), 'head', ('front', 'side', 'back')),
)
CHECK_SHOTS = {'full': (0.98, 4.6, 0.25), 'upper': (1.50, 1.9, 0.08), 'skirt': (0.66, 2.8, 0.15), 'head': (1.78, 1.2, 0.10)}
CHECK_VIEWS = {'front': 0.0, 'side': 90.0, 'back': 180.0, 'rside': -90.0}
CHAIN_COLOURS = {'skirt_fl': (1.0, 0.45, 0.05), 'skirt_sl': (0.95, 0.85, 0.1), 'skirt_bl': (0.9, 0.15, 0.1),
                 'skirt_fr': (0.1, 0.55, 1.0), 'skirt_sr': (0.1, 0.85, 0.75), 'skirt_br': (0.45, 0.25, 0.95),
                 'skirt_bm': (0.85, 0.85, 0.85), 'skirt_bli': (0.75, 0.3, 0.4), 'skirt_bri': (0.3, 0.4, 0.8),
                 'cape_fl': (1.0, 0.45, 0.05), 'cape_bl': (0.9, 0.15, 0.1), 'cape_fr': (0.1, 0.55, 1.0),
                 'cape_br': (0.45, 0.25, 0.95), 'cape_sl': (0.95, 0.85, 0.1), 'cape_sr': (0.1, 0.85, 0.75),
                 'cape_al': (0.55, 0.9, 0.2), 'cape_ar': (0.2, 0.35, 0.9), 'cape_bm': (0.85, 0.85, 0.85),
                 'hair': (0.2, 0.9, 0.3), 'hair_fl': (0.9, 0.6, 0.2), 'hair_fr': (0.2, 0.6, 0.9),
                 'lantern': (0.95, 0.85, 0.1)}


def swing_markers(arm, chains):
    obs = []
    for name, h, t, parent, c in swing_bones(chains):
        bm = bmesh.new()
        bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=0.009, matrix=Matrix.Translation(Vector(h)))
        d = Vector(t) - Vector(h)
        rod = Matrix.LocRotScale((Vector(h) + Vector(t)) * 0.5, Vector((0.0, 0.0, 1.0)).rotation_difference(d), None)
        bmesh.ops.create_cone(bm, cap_ends=True, segments=8, radius1=0.004, radius2=0.002, depth=d.length, matrix=rod)
        me = bpy.data.meshes.new('_Swing_' + name)
        bm.to_mesh(me)
        bm.free()
        ob = bpy.data.objects.new('_Swing_' + name, me)
        bpy.context.scene.collection.objects.link(ob)
        ob.vertex_groups.new(name=name).add(list(range(len(me.vertices))), 1.0, 'REPLACE')
        ob.modifiers.new('Armature', 'ARMATURE').object = arm
        ob.color = (*CHAIN_COLOURS[c['name']], 1.0)
        obs.append(ob)
    return obs


def swing_pose(arm, chains, moves):
    for pb in arm.pose.bones:
        pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    for chain_name, toward, deg in moves:
        names = [b[0] for b in swing_bones(chains) if b[4]['name'] == chain_name] or [chain_name]
        for k, name in enumerate(names):
            b = arm.data.bones[name]
            axis = (b.tail_local - b.head_local).normalized().cross(Vector(toward))
            if axis.length < 1e-6:
                axis = Vector((1.0, 0.0, 0.0))
            local = b.matrix_local.to_3x3().inverted() @ axis
            arm.pose.bones[name].rotation_quaternion = Quaternion(local.normalized(), math.radians(deg if k == 0 else deg / 3))
    bpy.context.view_layer.update()


def hat_contact(fig, deg):
    """How deep the brim, the hat tipped deg on its bone (front up), reaches into the hair and the collar (Ellis.py's)."""
    from mathutils.bvhtree import BVHTree
    co, faces, _ = fig.rest['HatBrim']
    h, t = fig.hat
    axis = gm.unit(np.cross(gm.unit(t - h), (0.0, 0.0, 1.0)))
    R = np.array(Matrix.Rotation(math.radians(deg), 3, Vector(tuple(axis))))
    trees = [BVHTree.FromPolygons([tuple(p) for p in c], faces) for c in (co, (co - h) @ R.T + h)]

    def hits(tree, p, d):
        out, o, d = [], Vector(tuple(p)), Vector(d)
        for _ in range(8):
            loc = tree.ray_cast(o, d, 0.3)[0]
            if loc is None:
                break
            out.append(loc.z)
            o = loc + d * 1e-5
        return out
    report = []
    for part in ('Hair', 'Collar'):
        deep = []
        for p in fig.rest[part][0]:
            up0, up1 = hits(trees[0], p, (0, 0, 1)), hits(trees[1], p, (0, 0, 1))
            down = hits(trees[1], p, (0, 0, -1))
            if len(up1) % 2 == 1 and down:
                deep.append(p[2] - down[0])
            elif up0 and not up1 and len(down) >= 2:
                deep.append(p[2] - down[1])
        report.append(f'{part.lower()}: {len(deep)} of {len(fig.rest[part][0])} points, deepest '
                      f'{max(deep, default=0.0) * 100:.1f} cm')
    return '; '.join(report)


def rig_check(fig, arm, skin, repo):
    """The swing bones on the rigged game mesh, without the bake: rest (bones drawn, then each vertex coloured by its
    chains' shares) and the SWING_TESTS bends, into Saved/Concepts/CraneSwing."""
    out = os.path.join(repo, 'Saved', 'Concepts', 'CraneSwing')
    os.makedirs(out, exist_ok=True)
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.render.resolution_x, sc.render.resolution_y = 720, 960
    sc.display.render_aa = '8'
    sh = sc.display.shading
    sh.light = 'STUDIO'
    sh.color_type = 'OBJECT'
    sh.show_cavity = True
    sh.cavity_type = 'WORLD'
    sh.show_object_outline = False
    sh.show_specular_highlight = False
    world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    world.color = (0.30, 0.33, 0.38)
    sc.world = world
    skin.color = (0.62, 0.58, 0.53, 1.0)
    markers = swing_markers(arm, fig.swing)
    cam = bpy.data.objects.new('CheckCam', bpy.data.cameras.new('CheckCam'))
    cam.data.lens = 70
    sc.collection.objects.link(cam)
    sc.camera = cam
    written = []

    def shoot(tag, aim, views):
        for v in views:
            a = math.radians(CHECK_VIEWS[v])
            target = Vector((0.0, 0.0, aim[0]))
            cam.location = target + Vector((math.sin(a) * aim[1], -math.cos(a) * aim[1], aim[2]))
            cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
            sc.render.filepath = os.path.join(out, f'{tag}_{v}.png')
            bpy.ops.render.render(write_still=True)
            written.append(sc.render.filepath)

    swing_pose(arm, fig.swing, ())
    shoot('rest', CHECK_SHOTS['full'], ('front', 'side', 'back'))
    shoot('rest_upper', CHECK_SHOTS['upper'], ('front', 'side', 'back'))
    names = [g.name for g in skin.vertex_groups]
    W = np.zeros((len(skin.data.vertices), len(names)))
    for v in skin.data.vertices:
        for g in v.groups:
            W[v.index, g.group] = g.weight
    tint = np.zeros((len(names), 3))
    for j, nm in enumerate(names):
        chain = nm[:-3] if nm[-3:-2] == '_' and nm[-2:].isdigit() and nm[:-3] in CHAIN_COLOURS else None
        tint[j] = np.array(CHAIN_COLOURS[chain]) * (1.0 - 0.25 * (int(nm[-2:]) - 1)) if chain else (0.6, 0.6, 0.6)
    swing_cols = [j for j, nm in enumerate(names) if nm not in fig.names]
    ed = np.empty(len(skin.data.edges) * 2, dtype=np.int64)
    skin.data.edges.foreach_get('vertices', ed)
    ed = ed.reshape(-1, 2)
    co = gm._object_coords(skin)
    step = np.abs(W[ed[:, 0]][:, swing_cols] - W[ed[:, 1]][:, swing_cols]).max(1)
    length = np.linalg.norm(co[ed[:, 0]] - co[ed[:, 1]], axis=1)
    worst = np.argsort(-step / np.maximum(length, 0.01))[:3]
    log(f'rig check: at most {int((W > 0).sum(1).max())} influences a vertex, sums {W.sum(1).min():.3f}..{W.sum(1).max():.3f}; '
        f'steepest swing weight steps ' + ', '.join(f'{step[e]:.2f} over {length[e] * 100:.1f} cm at '
                                                     f'{tuple(np.round(co[ed[e, 0]] * 100, 1))}' for e in worst))
    attr = skin.data.color_attributes.new('SwingShares', 'FLOAT_COLOR', 'POINT')
    rgb = W @ tint / np.maximum(W.sum(1, keepdims=True), 1e-9)
    attr.data.foreach_set('color', np.concatenate([rgb, np.ones((len(rgb), 1))], 1).astype(np.float32).ravel())
    skin.data.color_attributes.active_color = attr
    sh.color_type = 'VERTEX'
    for m in markers:
        m.hide_render = True
    shoot('shares', CHECK_SHOTS['full'], ('front', 'side', 'back'))
    shoot('shares_upper', CHECK_SHOTS['upper'], ('front', 'side', 'back'))
    sh.color_type = 'OBJECT'
    for m in markers:
        m.hide_render = False
    for tag, moves, shot, views in SWING_TESTS:
        swing_pose(arm, fig.swing, moves)
        shoot(tag, CHECK_SHOTS[shot], views)
    swing_pose(arm, fig.swing, ())
    log(f'rig check: the hat tipped {HAT_TIP:.0f} degrees, the brim reaches into the {hat_contact(fig, HAT_TIP)}')
    log(f'rig check: {len(written)} renders in {out}')
