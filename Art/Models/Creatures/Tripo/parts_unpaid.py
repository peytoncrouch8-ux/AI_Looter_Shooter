"""SK_Unpaid's body and SM_UnpaidHat from the user's Tripo model (Art/Models/Creatures/Tripo/Unpaid.glb, copied from
Art/Concepts/Creatures/Unpaid; the paid plan), on Unpaid.py's own rig: Unpaid.py builds its script model and its
skeleton first (its library users stop before that), then swap() puts this body in the script body's place
(LOOTER_UNPAID_TRIPO=0 keeps the script body). Kept exactly: every bone, its place and the rest pose (the game's idle);
the slots Ghost_A and GhostCoal; the coal (the script's own coal on the 'coal' bone, so the crit sphere and M_Ghost's
CoalAt still fit); the vertex colours M_Ghost reads (zones, cavity, ember, fade) and the shroud's strip UVs; the hit
hulls from Unpaid.py's hit_zones() (so PA_Unpaid and the arm guard read the new shape); the hat as its own mesh on the
'hat' bone.

How (looter_tripo_rig): the GLB in "Tripo space" (its units to meters, uniform: SCALE, LIFT, DEPTH; it faces -Y),
the hat cut off at the brim (HAT_CUT) and the head closed under it, thin strips thickened, then even quads by QuadriFlow
with the hands, face and tie blown up first (more, smaller quads there); the mouth cut open with a dark pocket and teeth
behind it; skin weights made in the model's own pose (bone heat on a skeleton laid on its joints, TRIPO_JOINTS; the robe
along the shroud chain by height); then every bone's map from its Tripo joint to the rig's (bone_maps) carries the mesh
into the rest pose: the chest hunched as the rig is, the arms in to the rig's shoulders, the hands halved (the model's
hands are twice a man's: the finger bones sit where a hand's knuckles are), the robe swept back along the shroud chain.
The look (zones from Tripo's paint, cavity, ember, fade) is parts_unpaid_look.py's; Tripo's paint itself rides on UV 1
(parts_unpaid_paint.py, 2026-10-31), drawn by M_Ghost with PaintAmount 1."""
import hashlib
import math
import os

import bmesh
import bpy
import numpy as np

import looter_textures as lt
import looter_tripo as tripo
import looter_tripo_rig as tr

GLB = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Tripo', 'Unpaid.glb')
CACHE = os.path.join(lt.REPO, 'Intermediate', 'UnpaidModel', 'TripoQuads')

# Tripo space: the GLB's units (its hat top at 99.7) to meters, so its shoulders, coal, chin and waistcoat's hem land at
# the rig's heights (s from the shoulder-to-head-top and coal-to-hem spans, which agree to 1%).
SCALE = 0.0182
LIFT = 0.11                  # m: z of the GLB's 0 (its chest's hole at the coal's height under the rig's hunch)
DEPTH = 1.1                   # GLB units: its body's middle front to back (y), put on the rig's spine (its chest's
                              # hole then near the coal)
HAT_CUT = 91.3                # GLB z: just under the brim where it meets the head (the brim dips lower further out)
QUADS = 13500                 # QuadriFlow's target over the blown-up surface (about 11.5k quads come back)
THIN = 0.009                  # m: strips thinner than this (the robe's tatters, the tie's ends) are thickened to it
HAT_QUADS = 1500


def T(p):
    """GLB units to Tripo space (meters, the rig's frame but the model's own pose)."""
    p = np.asarray(p, float)
    q = np.stack([p[..., 0] * SCALE, (p[..., 1] - DEPTH) * SCALE, p[..., 2] * SCALE + LIFT], -1)
    return q


def log(message):
    print(f'UNPAID TRIPO: {message}')


# --- The model's own joints (GLB units, measured from slices and ortho views; its left, +X; the right mirrors) ---

TRIPO_JOINTS = dict(
    shoulder=(11.0, 2.5, 71.5), elbow=(16.6, 3.2, 57.0), wrist=(22.3, -0.7, 41.5),
    # knuckle and the first joint along each finger (a finger bone's line); the thumb from its root
    index=((21.5, -4.6, 31.0), (20.6, -4.7, 27.5)), middle=((22.9, -3.0, 31.2), (22.2, -3.0, 27.5)),
    ring=((23.6, -1.3, 31.0), (22.9, -1.0, 27.5)), pinky=((24.4, 0.4, 30.6), (23.9, 1.0, 28.0)),
    thumb=((18.8, -3.6, 35.5), (17.2, -4.1, 31.0)),
    neck=((0.0, 1.0, 76.7), (0.0, 0.6, 83.0)), head=(0.0, 0.6, 83.0),
    hinge=(0.0, 0.3, 86.3), chin=(0.0, -6.8, 82.2),      # hinge: where the rig's jaw bone turns, in the model
)
# Each finger's centre line to its tip (GLB units, the left hand), for keeping a finger's weight on its own bone.
FINGER_LINES = dict(
    index=[(21.5, -4.6, 31.0), (21.0, -4.7, 28.5), (20.1, -4.5, 26.5), (18.8, -3.7, 24.6), (17.8, -3.3, 23.5)],
    middle=[(22.9, -3.0, 31.2), (22.4, -3.0, 28.5), (21.0, -2.6, 25.7), (19.4, -1.7, 24.4), (18.0, -0.9, 23.5)],
    ring=[(23.6, -1.3, 31.0), (23.2, -1.1, 28.5), (22.0, -0.5, 26.5), (20.4, 0.2, 25.2), (19.9, 0.3, 24.5)],
    pinky=[(24.4, 0.4, 30.6), (24.1, 0.8, 29.0), (23.4, 1.3, 27.7), (21.8, 1.9, 27.0), (20.4, 2.2, 27.5)],
    thumb=[(18.8, -3.6, 35.5), (17.6, -4.0, 32.5), (17.1, -4.0, 29.5), (17.4, -3.4, 26.0), (17.5, -3.2, 23.2)],
)
FINGERS = ('thumb', 'index', 'middle', 'ring', 'pinky')
# The hand (palm and fingers as one) onto the rig's: its knuckles 0.10 m from the wrist, the model's 0.19.
HAND_ALONG, HAND_ACROSS = 0.6, 0.7
FOREARM_ACROSS = 0.92
# Where the robe starts (Tripo-space z; the waistcoat's hem is at 0.957) and its middle line front to back.
ROBE_TOP, ROBE_FULL = 1.00, 0.95
# GLB units: the painted frown's middle (x, z), its front (y), half its width.
MOUTH = dict(x=-0.85, z=85.35, y=-7.5, half_width=1.9)
HAND_CENTRES = ((21.5, -2.0, 32.0), (-21.5, -2.0, 32.0))
BLOWN_UP = [((21.5, -2.0, 32.0), 2.0, 0.11, 0.24), ((-21.5, -2.0, 32.0), 2.0, 0.11, 0.24),
            ((-0.5, -3.5, 86.5), 1.9, 0.12, 0.22), ((0.0, -5.0, 78.0), 1.5, 0.05, 0.12)]   # (GLB centre, x, m, m)


def mirror(p):
    p = np.array(p, float)
    p[..., 0] *= -1.0
    return p


def side_joints(s):
    """The model's joints on one side (s = +1 left, -1 right) in Tripo space."""
    J = {}
    for key in ('shoulder', 'elbow', 'wrist'):
        p = np.array(TRIPO_JOINTS[key], float)
        J[key] = T(p if s > 0 else mirror(p))
    for f in FINGERS:
        a, b = (np.array(q, float) for q in TRIPO_JOINTS[f])
        J[f] = (T(a if s > 0 else mirror(a)), T(b if s > 0 else mirror(b)))
        line = np.array(FINGER_LINES[f], float)
        J[f + '_line'] = T(line if s > 0 else mirror(line))
    return J


# --- The source ---

def load_source():
    """The GLB in Tripo space: vertices, faces, per-face corner UVs and its colour map (rows bottom first)."""
    obj, maps = tripo.load(GLB)
    V, F = tr.mesh_arrays(obj.data)
    uv = tr.corner_uvs(obj.data, F)
    pixels = tripo.pixels(maps['BC'], 2048)[::-1].copy()
    bpy.data.objects.remove(obj)
    for image in [m for m in maps.values() if m]:
        bpy.data.images.remove(image)
    return dict(V=T(V), F=F, uv=uv, pixels=pixels)


def _components(F, n):
    parent = np.arange(n)

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    for f in F:
        r = find(f[0])
        for v in f[1:]:
            q = find(v)
            if q != r:
                parent[q] = r
    return np.array([find(f[0]) for f in F])


def _compact(V, F, keep):
    Fs = [f for f, k in zip(F, keep) if k]
    used = np.unique(np.concatenate(Fs))
    remap = -np.ones(len(V), int)
    remap[used] = np.arange(len(used))
    return V[used], [remap[f] for f in Fs]


def split_hat(V, F):
    """The hat off the head: the mesh cut along the plane at HAT_CUT where it crosses the head only (the brim, dipping
    under the plane further out, stays whole on the hat), the head's top closed in a dome, the hat's opening in a raised
    lining. Returns ((V, F) body, (V, F) hat), both closed."""
    cut = HAT_CUT * SCALE + LIFT
    obj = tr.new_object('_tripo_cut', V, F)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.holes_fill(bm, edges=bm.edges, sides=12)          # Tripo's few pinholes
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=(0, 0, cut), plane_no=(0, 0, 1))
    on_plane = [e for e in bm.edges if all(abs(v.co.z - cut) < 1e-6 for v in e.verts)]     # float32 coordinates
    # Faces apart along the plane: the body is the largest piece below it.
    bmesh.ops.split_edges(bm, edges=on_plane)
    bm.faces.ensure_lookup_table()
    bm.verts.index_update()
    Vc = np.array([v.co[:] for v in bm.verts])
    Fc = [np.array([v.index for v in f.verts]) for f in bm.faces]
    below = np.array([f.calc_center_median().z <= cut for f in bm.faces])
    bm.free()
    bpy.data.objects.remove(obj)
    roots = _components(Fc, len(Vc))
    ids, counts = np.unique(roots[below], return_counts=True)
    body = roots == ids[np.argmax(counts)]
    # The hat: everything else, its pieces welded back along the plane (only the head's ring stays open).
    hat_V, hat_F = tr.mesh_bmesh_clean(*_compact(Vc, Fc, ~body), merge=1e-6)
    body_V, body_F = _compact(Vc, Fc, body)
    body_V, body_F = _close(body_V, body_F, 0.06, up=True)
    hat_V, hat_F = _close(hat_V, hat_F, 0.07, up=True)
    return (body_V, body_F), (hat_V, hat_F)


def _close(V, F, height, up=True):
    """Closes every open loop: the largest with a dome (height up), the rest flat; normals made consistent outward."""
    loops = sorted(tr.boundary_loops(F), key=len, reverse=True)
    V = np.asarray(V, float)
    F = list(F)
    for k, loop in enumerate(loops):
        new_V, new_F = tr.cap(V, loop, (0.0, 0.0, 1.0), height if k == 0 else 0.0, rings=6 if k == 0 else 2)
        F += [np.array(f) for f in new_F]
        V = np.vstack([V, new_V])
    obj = tr.new_object('_closed', V, F)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()
    V, F = tr.mesh_arrays(obj.data)
    bpy.data.objects.remove(obj)
    return V, F


# --- Even quads (cached: QuadriFlow takes a minute) ---

def _key(*parts):
    h = hashlib.sha1()
    with open(GLB, 'rb') as f:
        h.update(f.read())
    h.update(repr(parts).encode())
    for helper in (tr.__file__, os.path.join(os.path.dirname(tr.__file__), 'looter_tripo_shape.py')):
        h.update(open(helper, 'rb').read())
    h.update(open(__file__, 'rb').read().split(b'# --- Even quads')[0])
    return h.hexdigest()[:16]


def _cached(name, key, make):
    path = os.path.join(CACHE, f'{name}_{key}.npz')
    if os.path.exists(path):
        data = np.load(path)
        sizes = data['sizes']
        flat = data['flat']
        F, at = [], 0
        for n in sizes:
            F.append(flat[at:at + n])
            at += n
        return data['V'], F
    V, F = make()
    os.makedirs(CACHE, exist_ok=True)
    np.savez(path, V=V, flat=np.concatenate(F), sizes=np.array([len(f) for f in F]))
    return V, F


def drop_specks(V, F, share=0.01):
    """Pieces under share of the biggest (a fingertip's voxel crumb, a strip's shard) go."""
    roots = _components(F, len(V))
    ids, counts = np.unique(roots, return_counts=True)
    keep = np.isin(roots, ids[counts >= counts.max() * share])
    return _compact(V, F, keep)


def remesh_body(V, F):
    def make():
        hands = [T(c) for c in HAND_CENTRES]
        free = np.ones(len(V))
        for c in hands:        # the fingers keep their own thickness: only cloth is thickened
            free *= np.clip((np.linalg.norm(V - c, axis=1) - 0.15) / 0.05, 0.0, 1.0)
        Vt = tr.thicken(V, F, THIN, where=free)
        regions = [(T(c), f, a, b) for c, f, a, b in BLOWN_UP]
        Vq, Fq = drop_specks(*tr.quad_remesh(Vt, F, QUADS, regions, voxel=0.004, name="_unpaid_quads"))
        return tr.despike(Vq, Fq), Fq
    return _cached('body', _key('body', QUADS, THIN, BLOWN_UP, HAT_CUT, len(V)), make)


def remesh_hat(V, F):
    def make():
        Vt = tr.thicken(V, F, 0.012)
        return drop_specks(*tr.quad_remesh(Vt, F, HAT_QUADS, (), voxel=0.003, name='_unpaid_hat_quads'))
    return _cached('hat', _key('hat', HAT_QUADS, HAT_CUT, len(V)), make)


# --- Into the rig ---

def _flat(faces):
    return [np.asarray(f) for F in faces for f in np.atleast_2d(F)]


def _remove(obj):
    if obj is None:
        return
    data = obj.data
    bpy.data.objects.remove(obj, do_unlink=True)
    if data is not None and not data.users:
        if isinstance(data, bpy.types.Mesh):
            bpy.data.meshes.remove(data)


def swap(U):
    """Replaces Unpaid.py's body (its hulls with it) and hat with the Tripo ones on the same rig. U: Unpaid.py's
    globals after its build. Returns (body, hat)."""
    import parts_unpaid_look as pl
    import parts_unpaid_rig as pr
    rig = U['rig']
    for o in list(rig.children):
        if o.name.startswith(('UCX_', 'USP_')) or o is U['body']:
            _remove(o)
    _remove(U['hat'])
    src = load_source()
    (BV, BF), (HV, HF) = split_hat(src['V'], src['F'])
    V, F = remesh_body(BV, BF)
    HVq, HFq = remesh_hat(HV, HF)
    log(f'quads: body {len(F)} faces ({sum(len(f) == 4 for f in F)} quads), hat {len(HFq)}')
    V, F, lip, pocket = pr.cut_mouth(V, F)
    S = pr.tripo_skeleton(rig)
    M = pr.bone_maps(rig, S)
    W = pr.skin_weights(rig, S, V, F, lip)
    arm = sum(W.get(b, 0.0) for b in pr.ARM_BONES)
    away = np.clip(1.0 - np.asarray(arm) * 3.0, 0.0, 1.0)
    robe = pr.smoothstep(ROBE_TOP, ROBE_FULL, V[:, 2]) * away
    # The robe's colour starts under the waistcoat's lowest point (0.957), its weights and fade a little higher.
    below_hem = pr.smoothstep(0.965, 0.935, V[:, 2]) * away
    carried = {"robe": robe, "lip": lip, "pocket": pocket, **{"W_" + b: w for b, w in W.items()}}
    # Bare skin only on the head and neck, the forearms and hands (the paint's grey shadows on a sleeve are shirt).
    bare = sum(W.get(b, 0.0) for b in ("head", "jaw", "neck") + pr.ARM_BONES if not b.startswith("upperarm"))
    bare = pr.smoothstep(0.3, 0.7, np.asarray(bare))
    hand = sum(W.get(b, 0.0) for b in pr.ARM_BONES if not b.startswith(("upperarm", "lowerarm")))
    hands = pr.smoothstep(0.3, 0.7, np.asarray(hand))
    face = pr.smoothstep(0.3, 0.7, np.asarray(W.get("head", 0.0) + W.get("jaw", 0.0)))
    V, F, zone, eye, carried = pl.zone_mesh(src, V, F, below_hem, carried, bare, hands, face)
    robe, lip, pocket = carried["robe"], carried["lip"], carried["pocket"]
    W = {b[2:]: w for b, w in carried.items() if b.startswith("W_")}
    fade = pl.robe_fade(V, robe)
    strip = pl.robe_uv(V, robe)
    n_body = len(V)

    # The mouth's pocket and teeth, then everything to the rig's rest.
    Vs, Fs, zs, cav, fades, strips = [V], list(F), list(zone), [np.clip(pocket, 0.0, 1.0)], [fade], [strip]
    n_main = len(Fs)          # the body's own faces; the mouth's pieces and the coal after them take no paint
    Ws = {b: [w] for b, w in W.items()}
    count = len(V)
    for Vp, Fp, wp, cavity in pr.mouth_pieces(U):
        Vs.append(Vp)
        flat = _flat(Fp)
        Fs += [f + count for f in flat]
        zs += [pl.SKIN] * len(flat)
        cav.append(np.full(len(Vp), cavity))
        fades.append(np.ones(len(Vp)))
        strips.append(np.full((len(Vp), 3), np.nan))
        for b in set(Ws) | set(wp):
            Ws.setdefault(b, [np.zeros(count)] if count else [])
        for b in Ws:
            Ws[b].append(np.broadcast_to(np.asarray(wp.get(b, 0.0), float), (len(Vp),)).copy())
        count += len(Vp)
    Vt = np.vstack(Vs)
    Wt = {b: np.concatenate(ws) for b, ws in Ws.items()}
    Vr = tr.warp(Vt, Wt, M)
    Vr, center = pl.seat_bowl(Vr, M, rig, U)

    # The coal on its bone, in its own slot.
    Vc, Fc = pl.coal_piece(U, center)
    off = np.linalg.norm(center - np.array(rig.data.bones["coal"].head_local))
    log(f"the coal sits {100 * off:.1f} cm off its bone")
    coal_faces = [f + count for f in _flat(Fc)]
    slots = np.zeros(len(Fs) + len(coal_faces), dtype=np.int32)
    slots[len(Fs):] = 1
    Fs += coal_faces
    zs += [pl.SKIN] * len(coal_faces)
    Vr = np.vstack([Vr, Vc])
    cav.append(np.zeros(len(Vc)))
    fades.append(np.ones(len(Vc)))
    strips.append(np.full((len(Vc), 3), np.nan))
    for b in Wt:
        Wt[b] = np.concatenate([Wt[b], np.zeros(len(Vc))])
    Wt['coal'] = np.concatenate([np.zeros(count), np.ones(len(Vc))])
    count += len(Vc)
    W = tr.normalize(Wt, count)
    # Faces of no area (a contour cut grazing a corner) go: Unreal warns of their tangents.
    area = np.array([np.linalg.norm(sum(np.cross(Vr[f[k]] - Vr[f[0]], Vr[f[k + 1]] - Vr[f[0]])
                                        for k in range(1, len(f) - 1))) for f in Fs]) * 0.5
    keep = area > 1e-9
    solid = (np.arange(len(keep)) >= n_main)[keep]
    Fs, slots, zs = [f for f, k in zip(Fs, keep) if k], slots[keep], [z for z, k in zip(zs, keep) if k]
    log(f"{int((~keep).sum())} faces of no area dropped")
    names = sorted(W)
    Wm = np.column_stack([W[b] for b in names])

    # Vertex colours: R the zone (per face corner), G cavity, B the ember edge round the coal and the eyes, A the fade.
    occ = tr.ao(Vr, Fs, rays=20, reach=0.045)
    # Only real creases: M_Ghost darkens by 0.95 G^0.6, so a little G goes a long way (0.2 already halves it).
    cavity = np.maximum(np.concatenate(cav), pr.smoothstep(0.9, 0.45, occ) * 0.45)
    d = np.linalg.norm(Vr - center, axis=1) - COAL_REACH_IN
    jitter = 0.5 + 0.5 * U['pnoise'](Vr, 50.0, 2)
    ember = pr.smoothstep(0.03, 0.006, d) * np.clip(jitter, 0.0, 1.0)
    cavity = np.maximum(cavity, pr.smoothstep(0.05, 0.0, d) * 0.55)
    ember[count - len(Vc):] = 0.0
    cavity[count - len(Vc):] = 0.0
    ember[:len(eye)] = np.maximum(ember[:len(eye)], np.clip(eye * 1.5, 0.0, 1.0))
    log(f"eyes: {int((eye > 0.3).sum())} vertices glow")
    alpha = np.concatenate(fades)
    zone_face = np.array(zs)

    mesh = tr.new_mesh('Unpaid', Vr, Fs)
    if len(mesh.polygons) != len(Fs) or len(mesh.vertices) != len(Vr):
        raise RuntimeError('validate() changed the Tripo body')
    ghost, coal = bpy.data.materials['Ghost_A'], bpy.data.materials['GhostCoal']
    mesh.materials.append(ghost)
    mesh.materials.append(coal)
    mesh.polygons.foreach_set('material_index', slots)
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(70.0))
    body = bpy.data.objects.new('Unpaid', mesh)
    bpy.context.scene.collection.objects.link(body)
    lt.box_uv(body, 'Polymer')

    class Strips:
        pass
    part = Strips()
    part.V, part.strip_uv = Vr, np.vstack(strips)
    U['strip_uvs'](mesh, [part])
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    loops = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loops)
    face_of = np.repeat(np.arange(len(Fs)), [len(f) for f in Fs])
    rgba = np.column_stack([zone_face[face_of] / 3.0, np.clip(cavity[loops], 0, 1), np.clip(ember[loops], 0, 1),
                            np.clip(alpha[loops], 0, 1)])
    col.data.foreach_set('color_srgb', rgba.astype(np.float32).ravel())
    mesh.color_attributes.active_color = col
    mesh.color_attributes.render_color_index = 0
    for b, name in enumerate(names):
        group = body.vertex_groups.new(name=name)
        w = Wm[:, b]
        for value in np.unique(np.round(w[w > 0.0], 3)):
            group.add(np.where(np.abs(np.round(w, 3) - value) < 1e-9)[0].tolist(), float(value), 'REPLACE')
    body.parent = rig
    body.modifiers.new('Armature', 'ARMATURE').object = rig
    U['convex_planes'] = _supporting_planes
    try:
        hulls = U['hit_zones'](rig, Vr, tr.by_size(Fs), Wm, names)
    except RuntimeError:
        H = U['HITS']
        P, who = U['face_samples'](Vr, H['faces'], H['keep'], H['owner'])
        gaps = U['outside_hits'](P)
        log(f'outside every hit zone: {np.round(P[gaps][:8], 3).tolist()} on {who[gaps][:8].tolist()}')
        raise
    tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
    quads = sum(len(p.vertices) == 4 for p in mesh.polygons)
    log(f'SK_Unpaid (Tripo): {tris} triangles ({quads} of {len(mesh.polygons)} faces quads), {len(mesh.vertices)} '
        f'vertices, {len(rig.data.bones)} bones, {hulls} hit zones, slots {[m.name for m in mesh.materials]}')
    lo, hi = Vr[:n_body].min(0), Vr[:n_body].max(0)
    log(f'bounds {np.round(lo, 3)} .. {np.round(hi, 3)}')
    hat = pl.finish_hat(U, HVq, HFq, M, src)
    # Tripo's own paint on UV 1 (parts_unpaid_paint.py), and Ghost_A set to draw it.
    import parts_unpaid_paint as pp
    hat_world = np.array([v.co[:] for v in hat.data.vertices]) + np.array(hat.location)
    m = M['hat']
    hat_tripo = (hat_world - m[:3, 3]) @ np.linalg.inv(m[:3, :3]).T
    body_tripo = np.vstack([Vt, Vr[len(Vt):]])
    colour = pp.paint(body, body_tripo, hat, hat_tripo, solid)
    pl.paint_look(bpy.data.materials['Ghost_A'], pp.zone_means(colour, [body, hat]))
    U['raw_colors'] = None
    return body, hat


COAL_REACH_IN = 0.05      # m: the coal's mean radius with its lumps; the ember edge is measured from there


def _supporting_planes(obj):
    """Unpaid.py's convex_planes for the hit-zone check, without the planes of the hull's sliver triangles: Blender's
    hull of the dense Tripo skin has a few near-zero triangles (6e-9 m2) whose normals are noise, so their planes cut
    through the hull and read its own points as outside. The hull itself (exported, rebuilt by Unreal) is whole."""
    M = np.array(obj.matrix_world)
    co = np.array([v.co[:] for v in obj.data.vertices]) @ M[:3, :3].T + M[:3, 3]
    tri = np.array([p.vertices[:3] for p in obj.data.polygons])
    n = np.cross(co[tri[:, 1]] - co[tri[:, 0]], co[tri[:, 2]] - co[tri[:, 0]])
    area = np.linalg.norm(n, axis=1) * 0.5
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
    d = (n * co[tri[:, 0]]).sum(1)
    flip = n @ co.mean(0) - d > 0.0
    n[flip] *= -1.0
    d[flip] *= -1.0
    # A real face of the hull has every hull point behind it; a sliver's noisy plane does not.
    good = (area > 1e-7) | ((co @ n.T - d) <= 1e-4).all(0)
    return n[good], d[good]
