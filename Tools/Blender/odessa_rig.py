"""Odessa's rig (Art/Models/Characters/Odessa.py calls rig() and rig_check()): the UE5 mannequin's 63 deform bones on
MakeHuman's joints, every part weighted by what it is (weights()), the forearms' twist bones, the tailcoat's tails on six
swing chains the game's springs move (Player/Animation/BodySwing; names from its skirt ring: each tail's outer edge
skirt_sl/sr, middle skirt_bl/br and inner edge by the split skirt_bli/bri, three bones each and an unweighted
<chain>_end leaf where the cloth ends), the `hat` bone the game tips back while aiming, and the hit zones. Adapted from
Ellis.py's rig (its lessons in Docs/Pipeline.md "Character models")."""
import math
import os

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Quaternion, Vector, kdtree

import looter_bake as bk
import looter_garments as gm
import looter_human as hu
import looter_model as lm
import odessa_clothes as cl
import odessa_coat as cm
import odessa_head as head
import odessa_kit as kit
from odessa_kit import FWD, log

# ------------------------------------------------------------------------------------------------- skin weights


def rigid_w(fig, n, parts):
    return hu.blend(fig.names, n, parts)


def base_of(name):
    """A part's name without Blender's .001 and the _l/_r of a side."""
    key = name.split('.')[0]
    return key[:-2] if key.endswith(('_l', '_r')) else key


def neck_only(fig, W):
    """Weights with the head's share moved down onto neck_02: a stiff collar turns with the neck, the head turns
    inside it."""
    W = W.copy()
    h, n2 = fig.col['head'], fig.col['neck_02']
    W[:, n2] += W[:, h]
    W[:, h] = 0.0
    return W


def weights(fig, ob):
    """The mannequin bone weights of ob's vertices, by what the part is."""
    me = ob.data
    P = np.array([v.co[:] for v in me.vertices])
    n = len(P)
    name = base_of(ob.name)
    sk = fig.sk
    torso_faces = lambda: [f for f, k in zip(fig.faces, fig.face_mask((fig.w_arm < 0.3) & (fig.w_head < 0.3))) if k]
    if name == 'Odessa':
        return fig.b.weights_at(P)[1]
    if name.startswith(('Eye', 'Hat', 'Hair', 'Knot', 'Earring', 'EarDrop')):
        return rigid_w(fig, n, {'head': 1.0})
    if name.startswith(('Collar', 'CravatBand', 'CravatKnot')):
        # the skin's own weights where it lies round the neck, the head's share on neck_02 (the stiff collar and the
        # cravat round it turn with the neck; the head turns inside them)
        return neck_only(fig, fig.b.weights_at(P)[1])
    if name.startswith(('Gauntlet', 'ShirtCuff', 'CoatCuff')):
        s = ob.name.split('.')[0][-1]
        e, w = sk[f'lowerarm_{s}'][0], sk[f'lowerarm_{s}'][1]
        t = np.clip(((P - e) @ gm.unit(w - e)) / np.linalg.norm(w - e), 0, 1)
        k = gm.smoothstep(0.75, 1.05, t)
        return rigid_w(fig, n, {f'lowerarm_{s}': 1.0 - 0.5 * k, f'hand_{s}': 0.5 * k})
    if name == 'Lapel':
        # sewn to the coat: each vertex the weights of the coat's nearest (on the torso's alone the coat's chest under
        # them, which the clavicles carry, came out through them in the aim; on the skin's own, a lapel 1-4 mm over
        # the coat drew 7 % on the upper arm where the coat had the neck: the same)
        return coat_weights_at(fig, P)
    if name.startswith(('CravatFall', 'StickPin', 'CoatButton', 'Vest', 'Shirt', 'WatchChain', 'WatchFob')):
        return fig.b.weights_at(P, torso_faces())[1]
    if name == 'CoatCollar':
        # its foot moves with the coat's neckline it lies on (the skin's weights, as the coat's), the stand above
        # with the upper spine and the neck
        t = gm.smoothstep(fig.neck[2] - 0.03, fig.neck[2] + 0.06, P[:, 2])
        stand = rigid_w(fig, n, {'spine_05': 1.0 - 0.6 * t, 'neck_01': 0.6 * t})
        k = gm.smoothstep(fig.neck[2] - 0.004, fig.neck[2] + 0.02, P[:, 2])[:, None]
        return hu.top4((1.0 - k) * fig.b.weights_at(P)[1] + k * stand)
    if name == 'CoatTails':
        # above the hips the torso under it; the tails from the pelvis to the thighs (side by x), as Ellis's skirt,
        # before their chains (swing_weights)
        names, near = fig.b.weights_at(P, torso_faces())
        hip, knee = fig.hip + 0.02, sk['thigh_l'][1][2]
        t = np.clip((hip - P[:, 2]) / (hip - knee), 0.0, 1.0)
        leg = 0.75 * t ** 0.8
        side = gm.smoothstep(0.0, 1.0, 0.5 + P[:, 0] / 0.36)
        skirt = hu.blend(fig.names, n, {'pelvis': 1.0 - leg, 'thigh_l': leg * side, 'thigh_r': leg * (1.0 - side)})
        k = gm.smoothstep(hip + 0.06, hip - 0.04, P[:, 2])[:, None]
        return hu.top4((1.0 - k) * near + k * skirt)
    if name == 'Gloves':
        hand_faces = [f for f, k in zip(fig.faces, fig.face_mask(fig.w_hand > 0.3)) if k]
        return fig.b.weights_at(P, hand_faces)[1]
    if name == 'Soles':
        out = np.zeros((n, len(fig.names)))
        for s in 'lr':
            ankle, ball = sk[f'foot_{s}'][0], sk[f'ball_{s}'][0]
            f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
            sel = (P[:, 0] * ball[0]) > 0
            t = gm.smoothstep(0.75 * fig.ball_s, 1.1 * fig.ball_s, (P[sel] - ankle) @ f)
            out[sel] = hu.blend(fig.names, int(sel.sum()), {f'foot_{s}': 1.0 - t, f'ball_{s}': t})
        return out
    if name.startswith(('Belt', 'Buckle')):
        return rigid_w(fig, n, {'pelvis': 0.85, 'spine_01': 0.15})
    if name.startswith('CardCase'):
        # the loop over the belt too (on the belt's weights a crouch's trousers rose 2.5 cm over it)
        return case_w(fig, n)
    return fig.b.weights_at(P)[1]


def coat_weights_at(fig, P):
    """The coat's weights (the default: the skin's) at its vertex nearest each point."""
    if getattr(fig, 'coat_w', None) is None:
        coat = next(o for o in bpy.data.objects if o.type == 'MESH' and base_of(o.name) == 'Coat')
        C = gm._object_coords(coat)
        tree = kdtree.KDTree(len(C))
        for i, c in enumerate(C):
            tree.insert(Vector(tuple(c)), i)
        tree.balance()
        fig.coat_w = (tree, fig.b.weights_at(C)[1])
    tree, W = fig.coat_w
    return W[[tree.find(Vector(tuple(p)))[1] for p in P]]


def case_w(fig, n):
    """The card case, rigid, on the skin's weights where its back lies against the trousers: it moves as the cloth
    under it does (on the thigh alone, or the hips alone, a crouch's hip crease folded 5 cm of trousers over it)."""
    c, nrm, up = fig.case
    back = np.asarray(c) - np.asarray(nrm) * 0.016
    w = fig.b.weights_at(back[None, :])[1]
    return np.repeat(hu.top4(w), n, axis=0)


# ------------------------------------------------------------------------------------- swing bones (the tails)

# Each tail's chains, at their share of its width from its outer edge (0) to its inner edge by the split (1)
TAIL_CHAINS = (('s', 0.10), ('b', 0.50), ('bi', 0.92))
SKIRT_TOP_DROP = 0.045  # the tails' own weights hold down to the hips less this; their chains from below it


def hem_point(co, centre, ang, width=math.radians(5)):
    near = co[np.abs((kit.round_angle(co, centre) - ang + math.pi) % (2 * math.pi) - math.pi) < width]
    p = near[np.argmin(near[:, 2])]
    r = math.hypot(p[0] - centre[0], p[1] - centre[1])
    ex, ey = gm.radial(np.float64(ang), FWD)
    return np.array([centre[0] + ex * r, centre[1] + ey * r, p[2]])


def swing_layout(fig, parts):
    """The tails' swing chains on the built game coat: [{name, parent, part, side, centre, joints}]: joints at the hips,
    a third and two thirds of the way down, and the hem."""
    by = {o.name.split('.')[0]: o for o in parts}
    coat = by['CoatTails']
    co, ed = kit.mesh_arrays(coat)
    tree = gm.bvh_objects([coat])
    top = fig.hip - 0.01
    hem_z = float(co[:, 2].min())
    heights = (top, top + (hem_z - top) / 3.0, top + (hem_z - top) * 2.0 / 3.0, hem_z + 0.02)
    chains = []
    for side, s in ((1, 'l'), (-1, 'r')):
        edges = [cm.edge_angles(co, ed, fig.centre, z, side) for z in heights]
        for key, u in TAIL_CHAINS:
            a = [side * (e0 + u * (e1 - e0)) for e0, e1 in edges]
            joints = [kit.on_surface(tree, fig.centre, a[k], heights[k], spread=(0, 1, -1, 2, -2, 3, -3))
                      for k in range(3)]
            joints.append(hem_point(co, fig.centre, a[3]))
            chains.append(dict(name=f'skirt_{key[0]}{s}{key[1:]}', parent='pelvis', part='CoatTails', side=side,
                               centre=np.asarray(fig.centre), joints=np.array(joints), u=u))
    for c in chains:
        j = c['joints']
        log(f'swing {c["name"]}: ' + ' > '.join(f'({p[0] * 100:.1f}, {p[1] * 100:.1f}, {p[2] * 100:.1f})' for p in j) + ' cm')
    return chains


def swing_bones(chains):
    out = []
    for c in chains:
        j = c['joints']
        for k in range(len(j) - 1):
            out.append((f'{c["name"]}_{k + 1:02d}', j[k], j[k + 1], c['parent'] if k == 0 else f'{c["name"]}_{k:02d}', c))
    return out


END_BONE = 0.02


def add_swing_bones(arm, chains):
    """The swing bones on the armature (deforming, unconnected, Z out of the cloth) and each chain's <chain>_end leaf at
    the cloth's end (deforming so the FBX keeps it, weighted nowhere). Returns the weighted bones' names."""
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


def tail_shares(P, chains):
    """Each vertex's shares of the tails' chains (n, chains): on its own tail (the side of the split it's on) between
    the two chains either side of it at its height, all to the outermost or innermost beyond them."""
    n = len(P)
    out = np.zeros((n, len(chains)))
    centre = chains[0]['centre']
    ang = kit.round_angle(P, centre)
    for s in (1, -1):
        sel = (ang * s) > 0
        if not sel.any():
            continue
        own = [i for i, c in enumerate(chains) if c['side'] == s]
        th = ang[sel] * s
        z = P[sel, 2]

        def at(i):
            j = chains[i]['joints']
            return np.interp(z, j[::-1, 2], (kit.round_angle(j, centre) * s)[::-1])
        knots = np.stack([at(i) for i in own], 1)
        w = np.zeros((int(sel.sum()), len(own)))
        w[:, 0] = th <= knots[:, 0]
        for i in range(len(own) - 1):
            lo, hi = knots[:, i], knots[:, i + 1]
            inside = (th > lo) & (th <= hi)
            t = np.clip((th - lo) / np.maximum(hi - lo, 1e-6), 0.0, 1.0)
            w[inside, i] += 1.0 - t[inside]
            w[inside, i + 1] += t[inside]
        w[:, -1] += th > knots[:, -1]
        sub = np.zeros((int(sel.sum()), len(chains)))
        for k, i in enumerate(own):
            sub[:, i] += w[:, k]
        out[sel] = sub
    return out


def swing_weights(fig, ob, W):
    """ob's weights with the swing bones' columns after the mannequin's: the tails' own weights give way to their
    chains below the hips, each chain's bones handing over in turn down it."""
    chains = fig.swing
    bones = swing_bones(chains)
    col = {b[0]: i for i, b in enumerate(bones)}
    P = np.array([v.co[:] for v in ob.data.vertices])
    n = len(P)
    C = np.zeros((n, len(bones)))
    f = np.zeros(n)
    if base_of(ob.name) == 'CoatTails':
        share = tail_shares(P, chains)
        top = fig.hip - SKIRT_TOP_DROP
        for i, c in enumerate(chains):
            j = c['joints']
            a = along(P[:, 2], j[:, 2])
            for k in range(a.shape[1]):
                C[:, col[f'{c["name"]}_{k + 1:02d}']] += share[:, i] * a[:, k]
            f += share[:, i] * gm.smoothstep(top, j[1, 2], P[:, 2])
    if not f.any():
        return np.concatenate([W, C], 1)
    C = C / np.maximum(C.sum(1, keepdims=True), 1e-9) * f[:, None]
    return soft_top4(np.concatenate([W * (1.0 - f)[:, None], C], 1))


# --------------------------------------------------------------------------------------------- hat, twist bones

def add_hat_bone(arm, head_pt, tail_pt):
    bk.select([arm], arm)
    bpy.ops.object.mode_set(mode='EDIT')
    e = arm.data.edit_bones.new('hat')
    e.head, e.tail = Vector(head_pt), Vector(tail_pt)
    e.align_roll(Vector((0.0, 0.0, 1.0)))              # Z up, X the head's left-right axis (+X tips the front up)
    e.use_connect, e.use_deform = False, True
    e.parent = arm.data.edit_bones['head']
    bpy.ops.object.mode_set(mode='OBJECT')
    return 'hat'


TWIST = (('lowerarm_twist_01', 0.5), ('lowerarm_twist_02', 0.9))
WRIST_BEND = (0.78, 1.25)
TWISTED = ('Coat', 'Gauntlet', 'Gloves', 'ShirtCuff', 'CoatCuff')   # (the coat: its sleeves' forearms)


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


def twist_weights(fig, ob, W, names):
    """The forearm's turn spread down it (Ellis.py's): on the sleeves, the cuffs and the glove's cuff each vertex's
    lowerarm and hand weight shared by how far down the forearm it is, the hand's share coming in over WRIST_BEND, the
    rest over lowerarm and the twist bones; past the wrist the forearm's share is the hand's."""
    if not ob.name.startswith(TWISTED):
        return W
    P = gm._object_coords(ob)
    col = {n: i for i, n in enumerate(names)}
    knots = np.array([0.0] + [f for _, f in TWIST])
    for s in 'lr':
        cols = [col[f'lowerarm_{s}']] + [col[f'{n}_{s}'] for n, _ in TWIST] + [col[f'hand_{s}']]
        total = W[:, cols[0]] + W[:, cols[-1]]
        t = cl.forearm_share(fig, s, P)
        sel = (total > 1e-4) & (t < 1.2)
        if not sel.any():
            continue
        hand = gm.smoothstep(WRIST_BEND[0], WRIST_BEND[1], t[sel])
        tt = np.clip(t[sel], 0.0, knots[-1])
        arm = np.stack([np.interp(tt, knots, np.eye(len(knots))[k]) for k in range(len(knots))], 1)
        share = np.concatenate([arm * (1.0 - hand)[:, None], hand[:, None]], 1)
        W[np.ix_(np.nonzero(sel)[0], cols)] = total[sel, None] * share
    return soft_top4(W)


# ---------------------------------------------------------------------------------------------------- the rig

def rig(fig, objects):
    """Weights every part, joins them into one mesh 'Odessa' (SK_Odessa in Unreal) under the mannequin armature with
    the forearms' twist bones, the tails' swing bones and the hat's bone."""
    arm = fig.b.armature('Odessa_Rig')
    fig.swing = swing_layout(fig, objects)
    fig.hat = fig.hat_pivot
    log(f'hat bone: ({fig.hat[0][0] * 100:.1f}, {fig.hat[0][1] * 100:.1f}, {fig.hat[0][2] * 100:.1f}) > '
        f'({fig.hat[1][0] * 100:.1f}, {fig.hat[1][1] * 100:.1f}, {fig.hat[1][2] * 100:.1f}) cm')
    names = fig.names + add_swing_bones(arm, fig.swing) + [add_hat_bone(arm, *fig.hat)] + add_twist_bones(arm)
    fig.rig_names = names
    fig.rest = {}
    for ob in objects:
        W = swing_weights(fig, ob, weights(fig, ob))
        hat = np.full((len(W), 1), 1.0 if ob.name.startswith('Hat') else 0.0)
        W = np.concatenate([W * (1.0 - hat), hat, np.zeros((len(W), len(names) - W.shape[1] - 1))], 1)
        W = twist_weights(fig, ob, W, names)
        fig.rest[ob.name] = (gm._object_coords(ob), [list(p.vertices) for p in ob.data.polygons], W)
        for j, bone in enumerate(names):
            c = W[:, j]
            nz = np.nonzero(c > 1e-4)[0]
            if not len(nz):
                continue
            g = ob.vertex_groups.get(bone) or ob.vertex_groups.new(name=bone)
            for i in nz:
                g.add([int(i)], float(c[i]), 'REPLACE')
    skin = next(o for o in objects if o.name == 'Odessa')
    # the hat's hull from its parts as they are, before they join the one mesh
    import bmesh as _bm
    hat_parts = []
    for ob in objects:
        if ob.name.startswith('Hat'):
            c = bpy.data.objects.new('_hull_' + ob.name, ob.data.copy())
            bpy.context.scene.collection.objects.link(c)
            hat_parts.append(c)
    for ob in objects:
        for attr in ('mh_index', 'zones'):
            if attr in ob.data.attributes:
                ob.data.attributes.remove(ob.data.attributes[attr])
    bk.select(objects, skin)
    bpy.ops.object.join()
    skin = bpy.context.view_layer.objects.active
    skin.name = skin.data.name = 'Odessa'
    if 'Strand' in skin.data.uv_layers:
        skin.data.uv_layers.remove(skin.data.uv_layers['Strand'])
    skin.parent = arm
    mod = skin.modifiers.new('Armature', 'ARMATURE')
    mod.object = arm
    arm['LODs'] = '50,25'
    arm['LODScreens'] = '0.5,0.2'
    hit_zones(fig, arm, hat_parts)
    for c in hat_parts:
        me = c.data
        bpy.data.objects.remove(c)
        bpy.data.meshes.remove(me)
    log(f'rigged: {len(skin.data.polygons)} faces, {sum(len(p.vertices) - 2 for p in skin.data.polygons)} triangles, '
        f'{len(arm.data.bones)} bones, {len(skin.vertex_groups)} weighted')
    return arm, skin


def hit_zones(fig, arm, hat_parts=()):
    """Hit zones (the physics asset's bodies), every visible part inside one: the head a capsule from under the chin
    (the earrings) to the crown, the hat a hull on its own bone (it tips with it), the trunk, the hips with the tails,
    each limb's whole bone, the hands to the fingertips, the feet with the boots' soles. (Ellis's spheres and 70 %
    capsules left the chin, the brim, fingers and soles out.)"""
    sk = fig.sk
    cy = fig.centre[1]
    hy = sk['head'][0][1]
    lm.hit_capsule(arm, 'head', (0.0, hy, fig.eye_z - 0.075), (0.0, hy, fig.eye_z + 0.05), 0.13)
    if hat_parts:
        lm.hit_hull(arm, 'hat', list(hat_parts))
    # (the trunk's up over the shoulders, the coat collar's foot on them, and out to the shoulder blades; the hips'
    # down to the tails' hem behind the knees)
    lm.hit_capsule(arm, 'spine_03', (0.0, cy, fig.waist + 0.04), (0.0, cy, fig.shoulder_z + 0.03), 0.19)
    lm.hit_capsule(arm, 'pelvis', (0.0, cy + 0.03, 0.58), (0.0, cy, fig.waist - 0.03), 0.22)
    for s in 'lr':
        for bone, r in (('upperarm', 0.075), ('lowerarm', 0.062), ('thigh', 0.105), ('calf', 0.10)):
            a, b = sk[f'{bone}_{s}'][0], sk[f'{bone}_{s}'][1]
            lm.hit_capsule(arm, f'{bone}_{s}', tuple(a), tuple(b), r)
        # the hands to the middle finger's tip and the thumbs their own (the hand's alone left them out; one aimed
        # between the two tips left the fingers out); the feet low, wide and long enough for the heeled soles
        lm.hit_capsule(arm, f'hand_{s}', tuple(sk[f'hand_{s}'][0]), tuple(sk[f'middle_03_{s}'][1]), 0.07)
        lm.hit_capsule(arm, f'thumb_01_{s}', tuple(sk[f'thumb_01_{s}'][0]), tuple(sk[f'thumb_03_{s}'][1]), 0.035)
        ankle, ball = sk[f'foot_{s}'][0], sk[f'ball_{s}'][0]
        lm.hit_capsule(arm, f'foot_{s}', (ankle[0], ankle[1] + 0.05, 0.035), (ball[0], ball[1] - 0.085, 0.035), 0.075)
    for ob in arm.children:
        if ob.name.startswith(('USP_', 'UCP_', 'UCX_')):
            ob.hide_render = True


def zone_cover(fig, arm, skin):
    """The share of each part's vertices inside a hit zone at rest (logged): 'every visible part inside one'."""
    zones = []
    for ob in arm.children:
        if ob.name.startswith('USP_'):
            zones.append(('s', np.array(ob.matrix_world.translation), max(ob.dimensions) * 0.5, None))
        elif ob.name.startswith('UCP_'):
            m = ob.matrix_world
            r = max(ob.dimensions.x, ob.dimensions.y) * 0.5
            ax = np.array((m.to_3x3() @ Vector((0, 0, 1))).normalized())
            half = max(ob.dimensions.z * 0.5 - r, 0.0)
            c = np.array(m.translation)
            zones.append(('c', c - ax * half, r, c + ax * half))
        elif ob.name.startswith('UCX_'):
            from mathutils.bvhtree import BVHTree
            m = ob.matrix_world
            pts = [tuple(m @ v.co) for v in ob.data.vertices]
            zones.append(('h', BVHTree.FromPolygons(pts, [tuple(p.vertices) for p in ob.data.polygons]), 0.0, None))
    out = []
    for name, (co, _f, _w) in fig.rest.items():
        inside = np.zeros(len(co), dtype=bool)
        for kind, a, r, b in zones:
            if kind == 'h':
                for i in np.nonzero(~inside)[0]:
                    loc, nrm, _f, _d = a.find_nearest(Vector(tuple(co[i])), 0.5)
                    if loc is not None and (Vector(tuple(co[i])) - loc).dot(nrm) <= 0.002:
                        inside[i] = True
                continue
            if kind == 's':
                inside |= np.linalg.norm(co - a, axis=1) <= r
            else:
                ab = b - a
                t = np.clip(((co - a) @ ab) / max(ab @ ab, 1e-12), 0, 1)
                inside |= np.linalg.norm(co - (a + np.outer(t, ab)), axis=1) <= r
        share = inside.mean()
        if share < 0.995:
            lo, hi = co[~inside].min(0) * 100, co[~inside].max(0) * 100
            out.append(f'{name} {share * 100:.0f}% (out: x {lo[0]:.0f}..{hi[0]:.0f}, y {lo[1]:.0f}..{hi[1]:.0f}, '
                       f'z {lo[2]:.0f}..{hi[2]:.0f} cm)')
    log('hit zones: every part inside one' if not out else 'hit zones: parts not wholly inside one: ' + ', '.join(out))


# ------------------------------------------------------------------------------------------------- --rig-check

FWD3, BACK3, LEFT3, UP3 = (0.0, -1.0, 0.0), (0.0, 1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0)
HAT_TIP = 20.0
SWING_TESTS = (
    ('skirt_bl', (('skirt_bl', BACK3, 35.0), ('skirt_bli', BACK3, 35.0), ('skirt_sl', BACK3, 35.0)), 'skirt',
     ('side', 'back')),
    ('skirt_sr', (('skirt_sr', (-1.0, 0.0, 0.0), 30.0),), 'skirt', ('back', 'rside')),
    ('hat_tip', (('hat', UP3, HAT_TIP),), 'head', ('front', 'side', 'back')),
)
CHECK_SHOTS = {'full': (0.90, 4.2, 0.25), 'upper': (1.40, 1.7, 0.06), 'skirt': (0.75, 2.6, 0.15),
               'head': (1.66, 1.1, 0.08), 'neck': (1.52, 1.1, 0.05)}
CHECK_VIEWS = {'front': 0.0, 'side': 90.0, 'back': 180.0, 'rside': -90.0}
CHAIN_COLOURS = {'skirt_sl': (0.95, 0.85, 0.1), 'skirt_bl': (0.9, 0.15, 0.1), 'skirt_bli': (0.75, 0.3, 0.4),
                 'skirt_bri': (0.3, 0.4, 0.8), 'skirt_br': (0.45, 0.25, 0.95), 'skirt_sr': (0.1, 0.85, 0.75)}


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
            local = b.matrix_local.to_3x3().inverted() @ axis
            arm.pose.bones[name].rotation_quaternion = Quaternion(local.normalized(), math.radians(deg if k == 0 else deg / 3))
    bpy.context.view_layer.update()


def hat_contact(fig, deg):
    """How deep the brim, the hat tipped deg on its bone (front up), reaches into the hair, the knot and the collars:
    their points that end up inside the brim, or above it from under it (cm)."""
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
    for part in ('Hair', 'Knot', 'Collar', 'CoatCollar'):
        if part not in fig.rest:
            continue
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


HEAD_TESTS = (('yaw_l', (0.0, 0.0, 1.0), 40.0), ('yaw_r', (0.0, 0.0, 1.0), -40.0), ('roll', (0.0, 1.0, 0.0), -20.0),
              ('pitch_down', (1.0, 0.0, 0.0), 25.0), ('pitch_up', (1.0, 0.0, 0.0), -25.0))


def collar_fit(fig, arm, axis, deg):
    """The head turned deg about axis at its joint (only the head bone and what hangs on it): how many of the skin's
    points (the jaw, the chin) end up inside the high collar's ring, and how deep (cm)."""
    names = fig.rig_names
    sub = np.array([n == 'head' or n == 'hat' for n in names])
    R = np.array(Matrix.Rotation(math.radians(deg), 3, Vector(axis)))
    pivot = fig.sk['head'][0]
    co, faces, W = fig.rest['Odessa']
    w = W[:, sub].sum(1)[:, None]
    posed = co + w * ((co - pivot) @ R.T + pivot - co)
    ccol, _cf, _cw = fig.rest['Collar']
    lo, hi = ccol[:, 2].min(), ccol[:, 2].max()
    nc = fig.neck[:2]
    inside = []
    for p in posed[(posed[:, 2] > lo) & (posed[:, 2] < hi) & (w[:, 0] > 0.5)]:
        a = math.atan2(p[0] - nc[0], -(p[1] - nc[1]))
        if p[2] > fig.collar_top(a % (2 * math.pi)) or p[2] < fig.collar_base:
            continue                             # over the collar's top edge there (or under it): not through it
        r = math.hypot(p[0] - nc[0], p[1] - nc[1])
        wall = fig.collar_r(a % (2 * math.pi), p[2])
        if r > wall - 0.004:                     # at the collar's wall or out past it: the jaw through the collar
            inside.append(r - (wall - 0.004))
    return len(inside), max(inside, default=0.0) * 100


def rig_check(fig, arm, skin):
    """The swing bones on the rigged game mesh without the bake: rest (bones drawn, then each vertex coloured by its
    chains' shares), the SWING_TESTS bends, the hat tipped, the head turned in its collar; Workbench clay into
    Saved/Concepts/OdessaSwing."""
    out = os.path.join(kit.REPO, 'Saved', 'Concepts', 'OdessaSwing')
    os.makedirs(out, exist_ok=True)
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.render.resolution_x, sc.render.resolution_y = 720, 960
    sc.display.render_aa = '8'
    sh = sc.display.shading
    sh.light, sh.color_type, sh.show_cavity, sh.cavity_type = 'STUDIO', 'OBJECT', True, 'WORLD'
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
    step = np.abs(W[ed[:, 0]][:, swing_cols] - W[ed[:, 1]][:, swing_cols]).max(1) if swing_cols else np.zeros(len(ed))
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
    shoot('shares', CHECK_SHOTS['full'], ('side', 'back'))
    sh.color_type = 'OBJECT'
    for m in markers:
        m.hide_render = False
    for tag, moves, shot, views in SWING_TESTS:
        swing_pose(arm, fig.swing, moves)
        shoot(tag, CHECK_SHOTS[shot], views)
    swing_pose(arm, fig.swing, ())
    log(f'rig check: the hat tipped {HAT_TIP:.0f} degrees, the brim reaches into the {hat_contact(fig, HAT_TIP)}')
    for tag, axis, deg in (('rest', (0.0, 0.0, 1.0), 0.0),) + HEAD_TESTS:
        n, deep = collar_fit(fig, arm, axis, deg)
        log(f'rig check: head {tag}: {n} skin points inside the high collar ({deep:.1f} cm at most)')
        if deg:
            from mathutils import Quaternion as Q
            for pb in arm.pose.bones:
                pb.rotation_mode = 'QUATERNION'
                pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
            b = arm.data.bones['head']
            local = b.matrix_local.to_3x3().inverted() @ Vector(axis)
            arm.pose.bones['head'].rotation_quaternion = Q(local.normalized(), math.radians(deg))
            bpy.context.view_layer.update()
            shoot(f'head_{tag}', CHECK_SHOTS['neck'], ('front', 'side'))
    for pb in arm.pose.bones:
        pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    zone_cover(fig, arm, skin)
    log(f'rig check: {len(written)} renders in {out}')
