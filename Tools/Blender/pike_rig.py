"""Pike's rig (Art/Models/Characters/Pike.py calls rig() and rig_check() with M, the model's module), as Ellis.py's:
the UE5 mannequin's 63 deform bones on MakeHuman's joints (looter_human), every part weighted from MakeHuman's own skin
weights or rigid on the bones it rides; the forearms' lowerarm_twist_01/02 (the game turns them by the hand's twist);
the swing chains the game's springs move (Player/Animation/BodySwing: names by prefix): the coat's skirt
(skirt_{fl,sl,bl,bm,br,sr,fr}, each two bones from the belt line to the hem), the neckerchief's point (bandana_01..02 on
neck_01) and the spyglass in its sling (lantern_01 on pelvis), each ending in an unweighted <chain>_end leaf; `hat` on
head (tipped back while aiming). Parts lying on another (pockets, lapels, straps, the bandolier) take the weights of
the part under them where they lie, so they move as one with it. Hit zones as Ellis's, the pack's a hull.
"""
import math
import os

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Quaternion, Vector
from mathutils.bvhtree import BVHTree

import looter_garments as gm
import looter_human as hu
import looter_model as lm
from pike_clothes import round_angle


def family(name):
    return name.split('.')[0].split('_')[0]


HAT_PARTS = ('HatCrown', 'HatBrim', 'HatBand', 'HatBandInner', 'BrimRoll', 'Goggle', 'GoggleCup', 'GoggleLens', 'GoggleRim',
             'GoggleBridge', 'GoggleStrap')
HEAD_PARTS = ('Eye', 'Hair', 'Whiskers', 'Brows', 'SpecRim', 'SpecTemple', 'SpecBridge')
PACK_PARTS = ('Rucksack', 'RuckLid', 'RuckBelt', 'RuckBuckle', 'Bedroll', 'BedrollStrap', 'Tripod', 'TripodShoe',
              'TripodBand', 'TripodHead', 'TripodStrap', 'MapTube', 'MapCap', 'MapBand', 'MapLoop')
PACK_BONES = {'spine_05': 0.55, 'spine_04': 0.45}
# The pack's hard pieces move whole: the weights of the back at one point of each (a function of its points)
PACK_RIGID = {
    'Tripod': lambda fig, P: P[np.argmin(P[:, 2])] * np.array([1.0, 1.0, 0.0]) + np.array([0.0, 0.0, 1.20]),
    'TripodShoe': lambda fig, P: np.array([-0.2, 0.0, 1.20]), 'TripodBand': lambda fig, P: np.array([-0.2, 0.0, 1.20]),
    'TripodHead': lambda fig, P: np.array([-0.2, 0.0, 1.20]), 'TripodStrap': lambda fig, P: np.array([-0.2, 0.0, 1.20]),
    'MapTube': lambda fig, P: np.array([-0.07, 0.0, 1.20]), 'MapCap': lambda fig, P: np.array([-0.07, 0.0, 1.20]),
    'MapBand': lambda fig, P: np.array([-0.07, 0.0, 1.20]), 'MapLoop': lambda fig, P: np.array([-0.07, 0.0, 1.20]),
    'Bedroll': lambda fig, P: np.array([0.0, 0.0, 1.03]), 'BedrollStrap': lambda fig, P: np.array([0.0, 0.0, 1.03]),
    'RuckBuckle': lambda fig, P: P.mean(0),
}
PACK_RIGID['Tripod'] = PACK_RIGID['TripodShoe']
BELT_PARTS = ('Belt', 'Buckle', 'Pouch', 'PouchFlap')
SPY_PARTS = ('Spyglass', 'SpyglassBrass', 'SpyglassGrip', 'FrogStrap', 'Frog')
# parts lying on another take its weights where they lie: (family, host families)
ON_COAT = ('Lapel', 'Collar', 'Cuff', 'CuffTab', 'Pencil', 'Strap', 'StrapBuckle', 'Bandolier', 'Cartridges', 'Bullets')
ON_CHEST = ('Lapel', 'Pencil', 'Strap', 'StrapBuckle', 'Bandolier', 'Cartridges', 'Bullets', 'Pocket', 'Flap')


def host_of(ob):
    """The family a part rides on (its weights are the host's where it lies), or None."""
    name = ob.name.split('.')[0]
    fam = family(name)
    if fam in ('Pocket', 'Flap', 'CoatButton'):
        return 'Skirt' if ('hip' in name or 'skirt' in name) else 'Coat'
    if fam in SPY_PARTS:
        return 'Skirt'
    if fam in ON_COAT:
        return 'Coat'
    return None


# ------------------------------------------------------------------------------------------------------- weights

def rigid_w(fig, n, parts):
    return hu.blend(fig.names, n, parts)


def body_faces(fig, mask):
    return [f for f, k in zip(fig.faces, fig.face_mask(mask)) if k]


def weights(M, fig, ob):
    """The mannequin bone weights of ob's vertices (fig.names' columns), by what the part is."""
    me = ob.data
    P = np.array([v.co[:] for v in me.vertices])
    n = len(P)
    fam = family(ob.name)
    sk = fig.sk
    if ob.name == 'Pike':
        return fig.b.weights_at(P)[1]
    if fam in HEAD_PARTS:
        return rigid_w(fig, n, {'head': 1.0})
    if fam in HAT_PARTS:
        return rigid_w(fig, n, {'head': 1.0})          # all on `hat` (rig() moves them there)
    if fam in PACK_PARTS:
        # the pack rides the back it lies on: each point the weights of the back straight in front of it (the torso's,
        # not the arms'), so it bends as the back bends; its hard pieces whole (PACK_RIGID). (Rigid on the upper spine,
        # an aim up swung the bedroll at its foot 3.5 cm into the coat's skirt: it hangs below the chest's pivot.)
        torso = body_faces(fig, (fig.w_arm < 0.25) & (fig.w_head < 0.3) & (fig.w_hand < 0.1) & (fig.w_leg < 0.3))
        Q = P.copy() if fam not in PACK_RIGID else np.repeat(PACK_RIGID[fam](fig, P)[None], n, 0)
        Q[:, 1] = fig.centre[1] + 0.05
        return fig.b.weights_at(Q, torso)[1]
    if fam in BELT_PARTS:
        return rigid_w(fig, n, {'pelvis': 0.85, 'spine_01': 0.15})
    if fam in ('Coat', 'Waistcoat', 'Shirt', 'VestButton'):
        # the body's own where it lies (the coat is the body's shell), not the head's nor the hands'
        mask = (fig.w_head < 0.5) & (fig.w_hand < 0.3) & (fig.w_leg < 0.6)
        return fig.b.weights_at(P, body_faces(fig, mask))[1]
    if fam in ('Kerchief', 'KerchiefKnot', 'KerchiefTail'):
        mask = (fig.w_hand < 0.3) & (fig.Wv[:, fig.col['head']] < 0.5)
        return fig.b.weights_at(P, body_faces(fig, mask))[1]
    if fam == 'Skirt':
        # Ellis.py's duster: above the hips the torso's; its skirt from the pelvis to the thighs by side (its chains
        # take it from mid-thigh: swing_weights)
        torso = body_faces(fig, (fig.w_arm < 0.3) & (fig.w_head < 0.3))
        names, near = fig.b.weights_at(P, torso)
        hip, knee = M.BELT - 0.02, sk['thigh_l'][1][2]
        t = np.clip((hip - P[:, 2]) / (hip - knee), 0.0, 1.0)
        leg = 0.75 * t ** 0.8
        side = gm.smoothstep(0.0, 1.0, 0.5 + P[:, 0] / 0.36)
        skirt = hu.blend(fig.names, n, {'pelvis': 1.0 - leg, 'thigh_l': leg * side, 'thigh_r': leg * (1.0 - side)})
        k = gm.smoothstep(M.BELT + 0.02, M.BELT - 0.04, P[:, 2])[:, None]
        return hu.top4((1.0 - k) * near + k * skirt)
    if fam in ('Trousers', 'Puttee'):
        mask = (fig.w_arm < 0.2) & (fig.w_hand < 0.1)
        return fig.b.weights_at(P, body_faces(fig, mask))[1]
    if fam == 'Soles':
        out = np.zeros((n, len(fig.names)))
        for s in 'lr':
            ankle, ball = sk[f'foot_{s}'][0], sk[f'ball_{s}'][0]
            f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
            sel = (P[:, 0] * ball[0]) > 0
            t = gm.smoothstep(0.09, 0.14, (P[sel] - ankle) @ f)
            out[sel] = hu.blend(fig.names, int(sel.sum()), {f'foot_{s}': 1.0 - t, f'ball_{s}': t})
        return out
    if fam in ('Boots', 'Laces', 'Eyelets'):
        mask = fig.w_leg > 0.3
        return fig.b.weights_at(P, body_faces(fig, mask))[1]
    return fig.b.weights_at(P)[1]


def follow(P, host):
    """Weights (full rows) for points from a host part's (co, faces, W): blended across its nearest face."""
    co, faces, W = host
    tris = [(f[0], f[k], f[k + 1]) for f in faces for k in range(1, len(f) - 1)]
    tree = BVHTree.FromPolygons([tuple(map(float, p)) for p in co], tris, all_triangles=True)
    out = np.zeros((len(P), W.shape[1]))
    for i, p in enumerate(P):
        loc, _n, fi, _d = tree.find_nearest(Vector(tuple(p)))
        if loc is None:
            continue
        t = tris[fi]
        bc = hu.barycentric(np.array(loc), *(co[j] for j in t))
        out[i] = sum(bc[k] * W[t[k]] for k in range(3))
    return out


# ---------------------------------------------------------------------------------------------- swing bones

# The skirt's chains each side at their share of the arc from the front (0) round to the back's middle (1); 'behind'
# straight behind the thigh's middle (a stride's back leg pushes it back: Ellis.py's lesson); skirt_bm down the middle
# of the back. Joints: the belt line, halfway, the hem.
SKIRT_CHAINS = (('f', 1 / 6), ('s', 1 / 2), ('b', 'behind'))
BANDANA_PARENT = 'neck_01'
END_BONE = 0.02


def on_surface(tree, centre, ang, z):
    """A garment's outermost point at angle ang round centre and height z (rays from outside; Ellis.py's)."""
    ex, ey = gm.radial(np.float64(ang), np.array([0.0, -1.0]))
    for d in np.radians((0, 2, -2, 4, -4, 6, -6, 9, -9, 12, -12, 16, -16, 20, -20)):
        dx, dy = gm.radial(np.float64(ang + d), np.array([0.0, -1.0]))
        hit = tree.ray_cast(Vector((centre[0] + dx, centre[1] + dy, z)), Vector((-dx, -dy, 0.0)), 1.0)
        if hit[0] is not None:
            r = math.hypot(hit[0].x - centre[0], hit[0].y - centre[1])
            return np.array([centre[0] + ex * r, centre[1] + ey * r, hit[0].z])
    raise ValueError(f'no garment at {math.degrees(ang):.0f} degrees, {z:.3f} m')


def hem_point(co, centre, ang, width=math.radians(6)):
    near = co[np.abs((round_angle(co, centre) - ang + math.pi) % (2 * math.pi) - math.pi) < width]
    p = near[np.argmin(near[:, 2])]
    r = math.hypot(p[0] - centre[0], p[1] - centre[1])
    ex, ey = gm.radial(np.float64(ang), np.array([0.0, -1.0]))
    return np.array([centre[0] + ex * r, centre[1] + ey * r, p[2]])


def swing_layout(M, fig, parts):
    """The swing chains placed on the built game parts: [{name, parent, part, side, centre, joints}]."""
    by = {o.name.split('.')[0]: o for o in parts}
    sk = fig.sk
    chains = []

    def chain(name, parent, part, side, centre, joints):
        chains.append(dict(name=name, parent=parent, part=part, side=side, centre=np.asarray(centre)[:2],
                           joints=np.array(joints)))
    skirt = by['Skirt']
    co = gm._object_coords(skirt)
    tree = gm.bvh_objects([skirt])
    top = M.BELT - 0.025
    mid = (top + M.HEM) * 0.5 - 0.02
    for side, s in ((1, 'l'), (-1, 'r')):
        th = sk[f'thigh_{s}']
        tm = (th[0] + th[1]) * 0.5
        behind = float(round_angle(np.array([tm[0], fig.centre[1] + 0.2]), fig.centre)[0])
        for key, u in SKIRT_CHAINS:
            a = behind if u == 'behind' else side * u * math.pi
            joints = [on_surface(tree, fig.centre, a, top), on_surface(tree, fig.centre, a, mid), hem_point(co, fig.centre, a)]
            chain(f'skirt_{key}{s}', 'pelvis', 'Skirt', side, fig.centre, joints)
    joints = [on_surface(tree, fig.centre, math.pi, z) for z in (top, mid)] + [hem_point(co, fig.centre, math.pi)]
    chain('skirt_bm', 'pelvis', 'Skirt', 0, fig.centre, joints)
    # the neckerchief's point: from under the knot down the middle of the bib to its point
    kco = gm._object_coords(by['Kerchief'])
    ktree = gm.bvh_objects([by['Kerchief']])
    centre = fig.neck[:2] + np.array([0.0, -0.012])
    front = kco[np.abs(round_angle(kco, centre)) < 0.25]
    tip = front[np.argmin(front[:, 2])]
    z0 = front[:, 2].max() - 0.025
    joints = [on_surface(ktree, centre, 0.0, z) for z in (z0, (z0 + tip[2]) * 0.5)] + [tip]
    chain('bandana', BANDANA_PARENT, 'Kerchief', 0, centre, joints)
    for c in chains:
        j = c['joints']
        M.log(f'swing {c["name"]}: ' + ' > '.join(f'({p[0] * 100:.1f}, {p[1] * 100:.1f}, {p[2] * 100:.1f})' for p in j) + ' cm')
    return chains


def swing_bones(chains):
    out = []
    for c in chains:
        j = c['joints']
        for k in range(len(j) - 1):
            out.append((f'{c["name"]}_{k + 1:02d}', j[k], j[k + 1], c['parent'] if k == 0 else f'{c["name"]}_{k:02d}', c))
    return out


def select(objects, active=None):
    import looter_bake as bk
    bk.select(objects, active)


def add_swing_bones(arm, chains):
    """The swing bones (deforming, unconnected, Z out of the cloth) and each chain's unweighted <chain>_end leaf."""
    select([arm], arm)
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
    """Four influences a vertex without seams (Ellis.py's): the fifth-largest weight comes off the top four."""
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
    """Each vertex's shares of the chains round a garment (Ellis.py's): between the two nearest at its height, the
    front edge's way all to the first; across the back each side's last hands over to the middle chain."""
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
        last = mid[-1]
        th = ang[sel] * s
        th = np.where(th < 0.0, th + 2 * math.pi, th)
        z = P[sel, 2]

        def at(i):
            j = chains[i]['joints']
            a = round_angle(j, centre)
            a = np.abs(a) if chains[i]['side'] == 0 else a * chains[i]['side']
            return np.interp(z, j[::-1, 2], a[::-1])
        knots = np.stack([at(i) for i in own] + [at(last)], 1)
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


SKIRT_TOP = None         # (set in swing_weights: the belt line) the skirt's own weights above, its chains below


def swing_weights(M, fig, ob, W):
    """ob's weights with the swing bones' columns after the mannequin's (Ellis.py's): the skirt's chains from the belt
    line, all of them from halfway down; the neckerchief's bib onto its chain from under the knot; the spyglass rigid
    on lantern_01."""
    chains = fig.swing
    bones = swing_bones(chains)
    col = {b[0]: i for i, b in enumerate(bones)}
    P = np.array([v.co[:] for v in ob.data.vertices])
    n = len(P)
    C = np.zeros((n, len(bones)))
    f = np.zeros(n)
    fam = family(ob.name)

    def spread(share, c, top):
        j = c['joints']
        a = along(P[:, 2], j[:, 2])
        for k in range(a.shape[1]):
            C[:, col[f'{c["name"]}_{k + 1:02d}']] += share * a[:, k]
        return share * gm.smoothstep(top, j[1, 2], P[:, 2])

    if fam == 'Skirt':
        part = [c for c in chains if c['part'] == 'Skirt']
        share = ring_weights(P, part)
        for i, c in enumerate(part):
            f += spread(share[:, i], c, M.BELT - 0.03)
    elif fam in ('Kerchief', 'KerchiefKnot', 'KerchiefTail'):
        c = next(c for c in chains if c['name'] == 'bandana')
        ang = np.abs(round_angle(P, c['centre']))
        share = gm.smoothstep(math.radians(55), math.radians(25), ang)
        if fam == 'KerchiefKnot':
            share = share * 0.0
        f = spread(share, c, c['joints'][0, 2] + 0.01)
    if not f.any():
        return np.concatenate([W, C], 1)
    C = C / np.maximum(C.sum(1, keepdims=True), 1e-9) * f[:, None]
    return soft_top4(np.concatenate([W * (1.0 - f)[:, None], C], 1))


# ------------------------------------------------------------------------------------------- hat and twist bones

def add_hat_bone(arm, head, tail):
    select([arm], arm)
    bpy.ops.object.mode_set(mode='EDIT')
    e = arm.data.edit_bones.new('hat')
    e.head, e.tail = Vector(head), Vector(tail)
    e.align_roll(Vector((0.0, 0.0, 1.0)))              # Z up, X the head's left-right axis (+X tips the front up)
    e.use_connect, e.use_deform = False, True
    e.parent = arm.data.edit_bones['head']
    bpy.ops.object.mode_set(mode='OBJECT')
    return 'hat'


TWIST = (('lowerarm_twist_01', 0.5), ('lowerarm_twist_02', 0.9))
WRIST_BEND = (0.78, 1.25)
TWISTED = ('Coat', 'Cuff', 'CuffTab', 'CoatButton', 'Pike')


def add_twist_bones(arm):
    select([arm], arm)
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


def twist_weights(fig, ob, W, names):
    """The forearm's turn spread down it (Ellis.py's): on the sleeves, the cuffs and the skin of the wrist each
    vertex's lowerarm and hand weight shared by how far down the forearm it is, the hand's share coming in over
    WRIST_BEND, the rest over lowerarm and the twist bones (0, 0.5, 0.9 of the hand's twist)."""
    if family(ob.name) not in TWISTED:
        return W
    P = gm._object_coords(ob)
    col = {n: i for i, n in enumerate(names)}
    knots = np.array([0.0] + [f for _, f in TWIST])
    for s in 'lr':
        cols = [col[f'lowerarm_{s}']] + [col[f'{n}_{s}'] for n, _ in TWIST] + [col[f'hand_{s}']]
        total = W[:, cols[0]] + W[:, cols[-1]]
        t = fig.forearm_share(s, P)
        sel = (total > 1e-4) & (t < 1.2)
        if ob.name == 'Pike':
            sel &= t > 0.6                              # the skin's wrist inside the sleeve; the hand keeps its own
        if not sel.any():
            continue
        hand = gm.smoothstep(WRIST_BEND[0], WRIST_BEND[1], t[sel])
        tt = np.clip(t[sel], 0.0, knots[-1])
        arm_ = np.stack([np.interp(tt, knots, np.eye(len(knots))[k]) for k in range(len(knots))], 1)
        share = np.concatenate([arm_ * (1.0 - hand)[:, None], hand[:, None]], 1)
        W[np.ix_(np.nonzero(sel)[0], cols)] = total[sel, None] * share
    return soft_top4(W)


# ----------------------------------------------------------------------------------------------------------- rig

def full_weights(M, fig, ob, names, n_swing):
    """Every column of the rig (mannequin, swing, hat, twist) for one part, before any follower reads it."""
    W = weights(M, fig, ob)
    W = swing_weights(M, fig, ob, W)
    hat = np.full((len(W), 1), 1.0 if family(ob.name) in HAT_PARTS else 0.0)
    W = np.concatenate([W * (1.0 - hat), hat, np.zeros((len(W), len(names) - W.shape[1] - 1))], 1)
    return twist_weights(fig, ob, W, names)


def rig(M, fig, objects):
    """Weights every part, joins them into one mesh 'Pike' (SK_Pike in Unreal) under the mannequin armature, with the
    twist bones, the swing bones and the hat's bone; the hit zones."""
    arm = fig.b.armature('Pike_Rig')
    fig.swing = swing_layout(M, fig, objects)
    fig.hat = M.HAT['pivot']
    M.log(f'hat bone: {np.round(np.asarray(fig.hat[0]) * 100, 1)} > {np.round(np.asarray(fig.hat[1]) * 100, 1)} cm')
    swing_names = add_swing_bones(arm, fig.swing)
    names = fig.names + swing_names + [add_hat_bone(arm, *fig.hat)] + add_twist_bones(arm)
    fig.rig_names = names
    fig.rest = {}
    coat = next((o for o in objects if family(o.name) == 'Coat'), None)
    coat_arm = None
    if coat is not None and 'ArmShare' in coat.data.attributes:
        coat_arm = np.empty(len(coat.data.vertices), dtype=np.float32)
        coat.data.attributes['ArmShare'].data.foreach_get('value', coat_arm)
    # the hosts first, then the parts riding them
    order = sorted(objects, key=lambda o: 1 if host_of(o) else 0)
    for ob in order:
        host = host_of(ob)
        if host:
            hosts = [fig.rest[o.name] for o in objects if family(o.name) == host]
            co_h = np.concatenate([h[0] for h in hosts])
            off, faces_h = 0, []
            for h in hosts:
                faces_h += [[i + off for i in f] for f in h[1]]
                off += len(h[0])
            if host == 'Coat' and family(ob.name) in ON_CHEST and coat_arm is not None:
                # what lies on the chest follows the coat's body, not its sleeves (by the shoulder the nearest coat was
                # the sleeve's top, and a raised arm pulled the strap through the coat)
                faces_h = [f for f in faces_h if coat_arm[f].max() < 0.35] or faces_h
            P = gm._object_coords(ob)
            if family(ob.name) in SPY_PARTS:
                # rigid: the skirt's weights under the frog (per point, the skirt's springs bent the spyglass in a stride)
                ring, top, bottom = fig.spyglass
                at = top + (bottom - top) * 0.15
                P = np.repeat(at[None], len(P), 0)
            W = follow(P, (co_h, faces_h, np.concatenate([h[2] for h in hosts])))
            W = twist_weights(fig, ob, soft_top4(W), names)
        else:
            W = full_weights(M, fig, ob, names, len(swing_names))
        fig.rest[ob.name] = (gm._object_coords(ob), [list(p.vertices) for p in ob.data.polygons], W)
        for j, bone in enumerate(names):
            c = W[:, j]
            nz = np.nonzero(c > 1e-4)[0]
            if not len(nz):
                continue
            g = ob.vertex_groups.get(bone) or ob.vertex_groups.new(name=bone)
            for i in nz:
                g.add([int(i)], float(c[i]), 'REPLACE')
    pack_pts = np.concatenate([gm._object_coords(o) for o in objects if family(o.name) in PACK_PARTS])
    skin = next(o for o in objects if o.name == 'Pike')
    for ob in objects:
        for attr in ('mh_index', 'ArmShare', 'SeamDist'):
            if attr in ob.data.attributes:
                ob.data.attributes.remove(ob.data.attributes[attr])
    select(objects, skin)
    bpy.ops.object.join()
    skin = bpy.context.view_layer.objects.active
    skin.name = skin.data.name = 'Pike'
    skin.parent = arm
    mod = skin.modifiers.new('Armature', 'ARMATURE')
    mod.object = arm
    arm['LODs'] = '50,25'
    arm['LODScreens'] = '0.5,0.2'
    hit_zones(M, fig, arm, pack_pts, skin)
    M.log(f'rigged: {len(skin.data.polygons)} faces, {sum(len(p.vertices) - 2 for p in skin.data.polygons)} triangles, '
          f'{len(skin.vertex_groups)} weighted bones, {len(arm.data.bones)} bones')
    return arm, skin


def hit_zones(M, fig, arm, pack_pts, skin):
    """Hit zones (the physics asset's bodies): the head with its hat, the trunk in its coat, the skirt round the hips,
    each limb, the hands and feet (as Ellis.py's), the pack a hull on spine_05; checked to hold every point at rest."""
    sk = fig.sk
    mid = lambda b, t=0.5: sk[b][0] + (sk[b][1] - sk[b][0]) * t
    zones = []
    zones.append(lm.hit_sphere(arm, 'head', tuple(mid('head', 0.45)), 0.17))
    zones.append(lm.hit_capsule(arm, 'spine_03', (0.0, fig.centre[1], 1.08), (0.0, fig.centre[1], 1.36), 0.24))
    # the skirt to its flared hem, the spyglass lying on it behind the left hip
    zones.append(lm.hit_capsule(arm, 'pelvis', (0.0, fig.centre[1], 0.60), (0.0, fig.centre[1], 0.92), 0.31))
    for s in 'lr':
        for bone, r, a0, a1 in (('upperarm', 0.09, 0.0, 1.0), ('lowerarm', 0.075, 0.05, 0.95), ('thigh', 0.12, 0.1, 0.9),
                                ('calf', 0.097, 0.05, 0.92)):
            a, b = sk[f'{bone}_{s}'][0], sk[f'{bone}_{s}'][1]
            zones.append(lm.hit_capsule(arm, f'{bone}_{s}', tuple(a + (b - a) * a0), tuple(a + (b - a) * a1), r))
        # the hand to the fingertips
        h0, h1 = sk[f'hand_{s}'][0], sk[f'middle_03_{s}'][1]
        zones.append(lm.hit_capsule(arm, f'hand_{s}', tuple(h0), tuple(h1 + gm.unit(h1 - h0) * 0.01), 0.085))
        # the boot from the heel to past the toe, down to the sole
        ankle, ball = sk[f'foot_{s}'][0], sk[f'ball_{s}'][0]
        f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
        zones.append(lm.hit_capsule(arm, f'foot_{s}', tuple(np.array([ankle[0], ankle[1], 0.046]) - f * 0.035),
                                    tuple(np.array([ball[0], ball[1], 0.036]) + f * 0.050), 0.09))
    # the pack: a convex hull round its parts
    bm = bmesh.new()
    for p in pack_pts:
        bm.verts.new(Vector(tuple(p)))
    hull = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in hull['geom_interior'] + hull['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    zones.append(lm._hit_shape('UCX_', arm, 'spine_05', bm, Matrix.Identity(4)))
    for ob in zones:
        ob.hide_render = True
    # every point of him inside one at rest, counted by part
    shapes = []
    for z in zones:
        zc = gm.object_points(z)
        if z.name.startswith('USP_'):
            c = zc.mean(0)
            shapes.append(('sphere', c, np.linalg.norm(zc - c, axis=1).max()))
        else:
            shapes.append(('hull', BVHTree.FromPolygons([tuple(p) for p in zc], [list(p.vertices) for p in z.data.polygons]), None))
    report = {}
    for name, (co, _f, _w) in fig.rest.items():
        outside = np.ones(len(co), dtype=bool)
        for kind, a, r in shapes:
            if kind == 'sphere':
                outside &= np.linalg.norm(co - a, axis=1) > r
            else:
                for i in np.nonzero(outside)[0]:
                    loc, nrm, _i, _d = a.find_nearest(Vector(tuple(co[i])))
                    if loc is not None and (Vector(tuple(co[i])) - loc).dot(nrm) < 1e-4:
                        outside[i] = False
        if outside.any():
            fam = family(name)
            n0, worst = report.get(fam, (0, None))
            report[fam] = (n0 + int(outside.sum()), np.round(co[outside].mean(0) * 100, 1).tolist())
    if report:
        M.log('hit zones: points outside every zone: ' + '; '.join(f'{k} {v[0]} about {v[1]} cm' for k, v in report.items()))
    else:
        M.log('hit zones: every point inside one')


# ------------------------------------------------------------------------------------------------- the rig check

FWD3, BACK3, LEFT3, UP3 = (0.0, -1.0, 0.0), (0.0, 1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0)
HAT_TIP = 20.0
SWING_TESTS = (
    ('skirt_fl', (('skirt_fl', FWD3, 35.0),), 'skirt', ('front', 'side')),
    ('skirt_bl', (('skirt_bl', BACK3, 35.0),), 'skirt', ('front', 'side', 'back')),
    ('bandana', (('bandana', LEFT3, 30.0),), 'upper', ('front', 'side')),
    ('hat_tip', (('hat', UP3, HAT_TIP),), 'head', ('front', 'side')),
)
CHECK_SHOTS = {'full': (0.86, 4.0, 0.25), 'upper': (1.36, 1.6, 0.08), 'skirt': (0.80, 2.4, 0.15), 'head': (1.62, 1.1, 0.10),
               'hips': (0.85, 1.6, 0.1), 'neck': (1.48, 1.1, 0.05)}
CHECK_VIEWS = {'front': 0.0, 'side': 90.0, 'back': 180.0, 'rside': -90.0}
COLOURS = {'skirt_fl': (1.0, 0.45, 0.05), 'skirt_sl': (0.95, 0.85, 0.1), 'skirt_bl': (0.9, 0.15, 0.1),
           'skirt_fr': (0.1, 0.55, 1.0), 'skirt_sr': (0.1, 0.85, 0.75), 'skirt_br': (0.45, 0.25, 0.95),
           'skirt_bm': (0.85, 0.85, 0.85), 'bandana': (0.95, 0.85, 0.1), 'lantern': (0.2, 0.9, 0.3)}


def swing_pose(arm, chains, moves):
    for pb in arm.pose.bones:
        pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    for chain_name, toward, deg in moves:
        names = [b[0] for b in swing_bones(chains) if b[4]['name'] == chain_name] or [chain_name]
        for k, name in enumerate(names):
            b = arm.data.bones[name]
            axis = (b.tail_local - b.head_local).normalized().cross(Vector(toward))
            local = b.matrix_local.to_3x3().inverted() @ axis
            arm.pose.bones[name].rotation_quaternion = Quaternion(local.normalized(), math.radians(deg if k == 0 else deg / 3))
    bpy.context.view_layer.update()


def posed(fig, name, R, pivot, sub):
    co, faces, W = fig.rest[name]
    w = W[:, sub].sum(1)[:, None]
    return co + w * ((co - pivot) @ R.T + pivot - co), faces


def hat_contact(fig, deg):
    """How many points of the hair, the collar and the pack the brim, tipped deg on its bone (front up), comes down
    into or through, and how deep (cm)."""
    names = [n for n in fig.rest if family(n) in ('HatBrim',)]
    co, faces, _ = fig.rest[names[0]]
    h, t = (np.asarray(x) for x in fig.hat)
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
    for fam in ('Hair', 'Collar', 'MapTube', 'Tripod', 'Whiskers'):
        pts = np.concatenate([fig.rest[n][0] for n in fig.rest if family(n) == fam]) if any(
            family(n) == fam for n in fig.rest) else np.zeros((0, 3))
        deep = []
        for p in pts:
            up0, up1 = hits(trees[0], p, (0, 0, 1)), hits(trees[1], p, (0, 0, 1))
            down = hits(trees[1], p, (0, 0, -1))
            if len(up1) % 2 == 1 and down:
                deep.append(p[2] - down[0])
            elif up0 and not up1 and len(down) >= 2:
                deep.append(p[2] - down[1])
        report.append(f'{fam.lower()} {len(deep)} of {len(pts)} ({max(deep, default=0.0) * 100:.1f} cm)')
    return ', '.join(report)


HEAD_TESTS = (('yaw_l', (0.0, 0.0, 1.0), 35.0), ('yaw_r', (0.0, 0.0, 1.0), -35.0), ('roll', (0.0, 1.0, 0.0), -20.0),
              ('pitch_up', (1.0, 0.0, 0.0), -25.0), ('pitch_down', (1.0, 0.0, 0.0), 25.0))


def head_clearance(fig, arm, axis, deg):
    """With the head turned deg about axis (and every bone under it): the brim's and the whiskers' nearest approach to
    the neckerchief, the collar and the pack (cm; negative through), the head's parts as the armature's blend skinning
    moves them."""
    sub = np.array([n in ('head', 'hat') or any(p.name == 'head' for p in arm.data.bones[n].parent_recursive)
                    for n in fig.rig_names])
    R = np.array(Matrix.Rotation(math.radians(deg), 3, Vector(axis)))
    pivot = fig.sk['head'][0]
    moving = [n for n in fig.rest if family(n) in ('HatBrim', 'HatCrown', 'Whiskers', 'Hair', 'BrimRoll')]
    still = [n for n in fig.rest if family(n) in ('Kerchief', 'Collar', 'MapTube', 'Tripod', 'Rucksack', 'Strap',
                                                  'Bandolier', 'KerchiefKnot')]
    co, fc = [], []
    for n in still:
        c, f, _ = fig.rest[n]
        fc += [[i + len(co) for i in p] for p in f]
        co += [tuple(p) for p in c]
    tree = BVHTree.FromPolygons(co, fc)
    worst = {}
    for n in moving:
        P, _ = posed(fig, n, R, pivot, sub)
        for p in P[::3]:
            loc, nrm, _i, d = tree.find_nearest(Vector(tuple(p)), 0.05)
            if loc is None:
                continue
            s = (Vector(tuple(p)) - loc).dot(nrm)
            fam = family(n)
            worst[fam] = min(worst.get(fam, 1.0), s)
    return ', '.join(f'{k} {v * 100:.1f}' for k, v in worst.items()) or 'nothing within 5 cm'


def rig_check(M, fig, arm, skin):
    """The swing bones on the rigged game mesh, without the bake (Workbench clay): rest (bones drawn, then each vertex
    coloured by its chains' shares), the SWING_TESTS bends; and logs the influences, the tipped brim's contacts, the
    head's turns against the neckerchief, the collar and the pack."""
    out = os.path.join(M.REPO, 'Saved', 'Concepts', 'PikeSwing')
    os.makedirs(out, exist_ok=True)
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.render.resolution_x, sc.render.resolution_y = 720, 960
    sc.display.render_aa = '8'
    sh = sc.display.shading
    sh.light, sh.color_type, sh.show_cavity, sh.cavity_type = 'STUDIO', 'OBJECT', True, 'WORLD'
    sh.show_specular_highlight = False
    world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    world.color = (0.30, 0.33, 0.38)
    sc.world = world
    skin.color = (0.62, 0.58, 0.53, 1.0)
    markers = []
    for name, h, t, parent, c in swing_bones(fig.swing):
        bm = bmesh.new()
        bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=0.009, matrix=Matrix.Translation(Vector(h)))
        d = Vector(t) - Vector(h)
        rod = Matrix.LocRotScale((Vector(h) + Vector(t)) * 0.5, Vector((0.0, 0.0, 1.0)).rotation_difference(d), None)
        bmesh.ops.create_cone(bm, cap_ends=True, segments=8, radius1=0.004, radius2=0.002, depth=d.length, matrix=rod)
        me = bpy.data.meshes.new('_Swing_' + name)
        bm.to_mesh(me)
        bm.free()
        ob = bpy.data.objects.new('_Swing_' + name, me)
        sc.collection.objects.link(ob)
        ob.vertex_groups.new(name=name).add(list(range(len(me.vertices))), 1.0, 'REPLACE')
        ob.modifiers.new('Armature', 'ARMATURE').object = arm
        ob.color = (*COLOURS.get(c['name'], (1, 1, 1)), 1.0)
        markers.append(ob)
    cam = bpy.data.objects.new('CheckCam', bpy.data.cameras.new('CheckCam'))
    cam.data.lens = 70
    sc.collection.objects.link(cam)
    sc.camera = cam

    def shoot(tag, aim, views):
        for v in views:
            a = math.radians(CHECK_VIEWS[v])
            target = Vector((0.0, 0.0, aim[0]))
            cam.location = target + Vector((math.sin(a) * aim[1], -math.cos(a) * aim[1], aim[2]))
            cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
            sc.render.filepath = os.path.join(out, f'{tag}_{v}.png')
            bpy.ops.render.render(write_still=True)
    swing_pose(arm, fig.swing, ())
    shoot('rest', CHECK_SHOTS['full'], ('front', 'side', 'back'))
    names = [g.name for g in skin.vertex_groups]
    W = np.zeros((len(skin.data.vertices), len(names)))
    for v in skin.data.vertices:
        for g in v.groups:
            W[v.index, g.group] = g.weight
    tint = np.zeros((len(names), 3))
    for j, nm in enumerate(names):
        chain = nm[:-3] if nm[-3:-2] == '_' and nm[-2:].isdigit() and nm[:-3] in COLOURS else None
        tint[j] = np.array(COLOURS[chain]) * (1.0 - 0.25 * (int(nm[-2:]) - 1)) if chain else (0.6, 0.6, 0.6)
    M.log(f'rig check: at most {int((W > 0).sum(1).max())} influences a vertex, sums {W.sum(1).min():.3f}..{W.sum(1).max():.3f}')
    attr = skin.data.color_attributes.new('SwingShares', 'FLOAT_COLOR', 'POINT')
    rgb = W @ tint / np.maximum(W.sum(1, keepdims=True), 1e-9)
    attr.data.foreach_set('color', np.concatenate([rgb, np.ones((len(rgb), 1))], 1).astype(np.float32).ravel())
    skin.data.color_attributes.active_color = attr
    sh.color_type = 'VERTEX'
    for m in markers:
        m.hide_render = True
    shoot('shares', CHECK_SHOTS['full'], ('front', 'side', 'back'))
    sh.color_type = 'OBJECT'
    for m in markers:
        m.hide_render = False
    for tag, moves, shot, views in SWING_TESTS:
        swing_pose(arm, fig.swing, moves)
        shoot(tag, CHECK_SHOTS[shot], views)
    swing_pose(arm, fig.swing, ())
    M.log(f'rig check: the hat tipped {HAT_TIP:.0f} degrees, the brim comes into: {hat_contact(fig, HAT_TIP)}')
    for tag, axis, deg in (('rest', (0.0, 0.0, 1.0), 0.0),) + HEAD_TESTS:
        M.log(f'rig check: head {tag}: nearest to the neckerchief, collar and pack (cm): {head_clearance(fig, arm, axis, deg)}')
    M.log(f'rig check: renders in {out}')
