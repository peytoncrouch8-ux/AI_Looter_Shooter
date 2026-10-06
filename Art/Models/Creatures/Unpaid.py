"""The Unpaid (AUnpaidCreature): the restless dead of Ransom's Rest as the user picked them on 2026-10-06, option D of
Art/Backlog/Creatures/UnpaidConcepts.py. A homesteader wasted to the bone in the clothes he died in: slouch hat, wool
vest, bandana, rolled sleeves; a hungry face with sunk eyes and a slack, toothed jaw; his shirt giving way below the
waist to torn shroud strips that fade; a coal burning through the vest where his heart was. He floats: no legs. A rig
the game poses in code, like the spider; there are no animations.

Models:
  SK_Unpaid     the creature, about 6.5k triangles, LODs at 50% and 25% (no Nanite), skinned smoothly (up to four bones a
                vertex) to the bones below.
  SM_UnpaidHat  his hat, a static mesh (no collision) whose pivot is the 'hat' bone's head: attach it there. Abel and Amos
                get their own hats later.

Bones (Blender names, as AUnpaidCreature reads them; the armature is Unreal's root), in the bind pose: the game's idle,
which the code poses from and the bestiary's stand shows unposed (facing -Y, Unreal's +X; the origin on the ground;
upright, a slight hunch baked into the upper back, head level, the jaw slack and open 16 degrees, the arms hanging with
the forearms bent 12 degrees and the palms in, the hands as the D concept holds them, the shroud down and back). The code
was written against an older rest without those three; see OLD_JAW, IDLE_JAW, IDLE_BEND, code_finger_pose and bake_idle.
  pelvis                         the float height (1.0 m over the origin); parent of the spine and the shroud
  spine_01, spine_02             spine_02 is the chest; it carries the arms, the neck and the coal
  coal                           the coal, rigid; its head is the coal's middle, the crit point
  neck, head, jaw, hat           jaw: the lower face, lower lip and lower teeth, opened by turning it down about its
                                 head (the shriek's 30 degrees more make 46 in all); hat: no skin, the hat's attach point
  upperarm_l, lowerarm_l, hand_l and its fingers thumb_01_l, index_01_l, middle_01_l, ring_01_l, pinky_01_l (one bone a
                                 finger, at the knuckle); the same with _r
  tail_01 .. tail_05             the shroud, down its middle from the pelvis
  tail_l_01, tail_l_02, tail_r_01, tail_r_02
                                 side chains off tail_02 that the strips on each side follow as well
Smooth weights: the cloth blends across the spine and shoulders, the sleeves into the elbows, the forearms into the
wrists, the neck between the chest and the head, the cheeks between head and jaw; the shroud's weights run along the
tail chains by how far down the shroud a vertex is, its outer strips sharing the side chains.

Vertex colors ('Col', what M_Ghost reads): R the tint zone (0 skin and shroud, 1/3 shirt, 2/3 vest and hat, 1 bandana
and hat band; Zone1Color..Zone4Color), G cavity darkening (sockets, mouth, the coal's crater), B the ember edge round the
coal (glows in the rank color), A the fade (1 solid, 0 gone; the shroud's strips). The rig's FBX carries colors as
sRGB, so the body stores them as bytes with exactly these values; the hat (a textured static mesh, exported linear)
stores the same values as floats. UV 0 is a world-scale box projection for the Polymer set (1 UV unit per meter), but
on the shroud: U the meters round it, V minus the meters down it, so that Unreal's V (flipped on import) runs down the
strips toward their ends, the way M_Ghost drifts its noise up them.

Materials: Ghost_A (M_Ghost: the four zone tints of tint 1, the rim, the glow and the ember) on everything but the coal,
the hat too; GhostCoal (M_Ghost with Coal = 1) on the coal.

Hit zones (PA_Unpaid): a convex hull on pelvis, spine_01, spine_02, neck, head (not the hat), jaw, each upper arm,
forearm and hand (its fingers included, at rest and as the code curls them idle and lunging), tail_01 and tail_02, round
every face with a corner those bones carry most (so neighbors overlap by a face and no band between them lets a shot
through); and a 7 cm sphere on coal. The build checks that every face but the fading strips' lies inside a zone. The
strips past tail_02 have none: shots pass through them.

    blender -b --factory-startup --python Art/Models/Creatures/Unpaid.py -- --preview
Previews go to Saved/ArtPreviews/RansomsRest/Unpaid/ (the stage comes from the concept script).
"""
import math
import os
import sys
from collections import Counter

import bpy
import numpy as np
from mathutils import Matrix, Quaternion, Vector, kdtree, noise

import looter_model as lm
import looter_textures as lt

SIDES = (1.0, -1.0)                     # +1 the left (+X), -1 the right
SUFFIX = {1.0: 'l', -1.0: 'r'}
HUNCH = math.radians(9.0)               # the upper back's curve, baked into the bind pose
# The rest pose is the game's idle, so the bestiary's stand and the editor, which show the mesh unposed, show him as he
# hangs in the game. AUnpaidCreature's numbers were first written against an older rest (straight forearms, the fingers
# turned back from the idle hand, the jaw open 10 degrees); its idle over that rest (UnpaidCreatureRig.cpp: PoseTargets
# and the arms) is baked in here, and the code takes these amounts off at the bones, so its poses stay where they were.
OLD_JAW = 10.0                          # the older rest's jaw, degrees open
IDLE_JAW = 6.0                          # the code's idle jaw over it (Goal.Jaw = 6; Own = PitchBy(Jaw))
IDLE_BEND = 12.0                        # the code's idle elbow (Bend = 12 + ...; the forearm's Own = PitchBy(-Bend))
JAW_BIND = math.radians(OLD_JAW + IDLE_JAW)     # the rest jaw: 16 degrees open, slack
SKIN, SHIRT, VEST, ACCENT = 0, 1, 2, 3  # tint zones (vertex color R = zone / 3)
# Tint 1: pale skin and shroud, faded chambray, wool, red. The skin and the shirt are about 20% darker (in linear) than
# the concept's #D2D8D5 and #A5B2BA, so that in daylight sunlit skin reads pale grey rather than white (tried in the game,
# approved 2026-10-06).
ZONE_COLORS = ('#BAC4C6', '#93A5B2', '#6C5D50', '#A65E4C')
RIM_COLOR = '#DCECEE'
TRIANGLES = dict(shirt=700, sleeve=230, vest=820, arm=560, head=1050)   # the parts that are reduced to a budget
OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Unpaid')
WORK = os.path.join(lt.REPO, 'Intermediate', 'UnpaidModel')


def log(message):
    print(f'UNPAID: {message}', flush=True)


def finger_bone(finger, sfx):
    """A finger's bone, as AUnpaidCreature names them: thumb_01_l .. pinky_01_r."""
    return f'{finger}_01_{sfx}'


# --- Math ---

def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=float) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def gauss(x, s):
    return np.exp(-(np.asarray(x, float) / s) ** 2)


def unit(v):
    v = np.asarray(v, dtype=float)
    return v / np.linalg.norm(v, axis=-1, keepdims=True)


def rotation(axis, angle):
    x, y, z = unit(axis)
    c, s = math.cos(angle), math.sin(angle)
    C = 1.0 - c
    return np.array([[c + x * x * C, x * y * C - z * s, x * z * C + y * s],
                     [y * x * C + z * s, c + y * y * C, y * z * C - x * s],
                     [z * x * C - y * s, z * y * C + x * s, c + z * z * C]])


def pnoise(points, scale=1.0, seed=0):
    off = Vector((seed * 13.17 + 3.1, seed * 7.31 + 1.7, seed * 5.13 + 4.3))
    return np.array([noise.noise(Vector(p) * scale + off) for p in np.asarray(points, float).reshape(-1, 3)])


def wrap(a):
    return (np.asarray(a, float) + np.pi) % (2.0 * np.pi) - np.pi


def turn_matrix(axis, degrees):
    """The rotation by degrees about axis (right-handed in the axes given)."""
    a = unit(axis)
    k = np.array([[0.0, -a[2], a[1]], [a[2], 0.0, -a[0]], [-a[1], a[0], 0.0]])
    r = math.radians(degrees)
    return np.eye(3) + math.sin(r) * k + (1.0 - math.cos(r)) * (k @ k)


# AUnpaidCreature's turns (UnpaidCreatureRig.cpp), in Blender's axes. Its PitchBy(a) is a turn about Blender's +X by a
# (a hanging limb swings back), as jaw_turn() turns the jaw; the hands' turns are below.
UE_FROM_BLENDER = np.array([[0.0, -1.0, 0.0], [-1.0, 0.0, 0.0], [0.0, 0.0, 1.0]])   # the rig's export; its own inverse
FINGERS = ('thumb', 'index', 'middle', 'ring', 'pinky')       # the code's order: they fan out round the middle one
IDLE_CURL, IDLE_SPLAY = 25.0, 4.0                             # its idle Curl and Splay (PoseTargets)


def code_finger_turn(finger, knuckle, wrist, side, curl, splay):
    """The code's turn of a finger for its Curl and Splay, about the knuckle: fanned about the side axis by its place
    from the middle finger, then curled about the axis square to the way it points from the wrist and to the arm's side
    (side +1 left): TurnBy(Axis, -Curl) * PitchBy(Fan * Splay). Over the older rest, that was the finger's whole pose."""
    M = UE_FROM_BLENDER
    pointing = unit(M @ (np.asarray(knuckle, float) - np.asarray(wrist, float)))
    curl_turn = turn_matrix(np.cross(pointing, (0.0, -side, 0.0)), -curl)
    fan = turn_matrix((0.0, 1.0, 0.0), (FINGERS.index(finger) - 2.0) * splay)
    return M @ curl_turn @ fan @ M


def code_finger_pose(finger, knuckle, wrist, side, curl, splay):
    """A finger's pose over this rest, the idle hand: the code's turn less its idle one (TurnBy(Axis, -Curl) *
    PitchBy(Fan * Splay) * IdleTurn.Inverse()), with the axes from this rest. The forearm's bend turns about the same axis
    as the side, the splay and the hand, so these axes are the older rest's turned with it."""
    return (code_finger_turn(finger, knuckle, wrist, side, curl, splay)
            @ code_finger_turn(finger, knuckle, wrist, side, IDLE_CURL, IDLE_SPLAY).T)


def bent(s):
    """The forearm as the rest pose holds it, bent at the elbow by the code's idle (PitchBy(-12), the forearm swinging
    forward): the elbow, the wrist, the way the hand points and the back of the hand."""
    R = turn_matrix((1.0, 0.0, 0.0), -IDLE_BEND)
    return ELBOW[s], ELBOW[s] + R @ (WRIST[s] - ELBOW[s]), R @ HAND_DIR[s], R @ HAND_UP[s]


def spline(points, n):
    """n points along a Catmull-Rom curve through points, evenly spaced by arc length."""
    P = np.asarray(points, dtype=float)
    dense = []
    for i in range(len(P) - 1):
        p0, p1, p2, p3 = P[max(i - 1, 0)], P[i], P[i + 1], P[min(i + 2, len(P) - 1)]
        for t in np.linspace(0.0, 1.0, 24, endpoint=False):
            dense.append(0.5 * (2 * p1 + (p2 - p0) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t
                                + (3 * p1 - p0 - 3 * p2 + p3) * t ** 3))
    dense.append(P[-1])
    dense = np.array(dense)
    s = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(dense, axis=0), axis=1))])
    target = np.linspace(0.0, s[-1], n)
    return np.stack([np.interp(target, s, dense[:, k]) for k in range(3)], axis=1)


def frames(C, up):
    """Rotation-minimizing tangents, normals and binormals along a polyline, the normal starting toward up."""
    T = unit(np.gradient(C, axis=0))
    N = np.zeros_like(C)
    n = np.asarray(up, float) - T[0] * np.dot(up, T[0])
    N[0] = n / np.linalg.norm(n)
    for i in range(1, len(C)):
        n = N[i - 1] - T[i] * np.dot(N[i - 1], T[i])
        N[i] = n / np.linalg.norm(n)
    return T, N, np.cross(T, N)


def table(rows, x):
    rows = np.asarray(rows, float)
    return [np.interp(x, rows[:, 0], rows[:, k]) for k in range(1, rows.shape[1])]


def sample(arr, tq):
    """Rows of arr read at fractions tq of its length."""
    n = len(arr)
    x = np.clip(np.asarray(tq, float) * (n - 1), 0.0, n - 1.0001)
    i = x.astype(int)
    f = (x - i)[..., None]
    return arr[i] * (1.0 - f) + arr[i + 1] * f


# --- Mesh pieces: (vertices, [face arrays]) ---

def grid_faces(rows, cols, closed=True):
    i = np.arange(rows - 1)[:, None]
    j = np.arange(cols if closed else cols - 1)[None, :]
    j1 = (j + 1) % cols
    return np.stack(np.broadcast_arrays(i * cols + j, i * cols + j1, (i + 1) * cols + j1, (i + 1) * cols + j),
                    axis=-1).reshape(-1, 4)


def fan(center, ring, top=False):
    ring = np.asarray(ring)
    tris = np.stack([np.full(len(ring), center), np.roll(ring, -1), ring], axis=1)
    return tris[:, ::-1] if top else tris


def ellipsoid(center, radii, axes=None, segs=32, rings=18):
    axes = np.eye(3) if axes is None else np.asarray(axes, float)
    th = np.linspace(0.0, np.pi, rings + 1)[1:-1]
    ph = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    z = np.repeat(-np.cos(th), segs)
    r = np.repeat(np.sin(th), segs)
    x, y = r * np.tile(np.cos(ph), len(th)), r * np.tile(np.sin(ph), len(th))
    pts = np.vstack([np.stack([x, y, z], 1), [[0.0, 0.0, -1.0], [0.0, 0.0, 1.0]]])
    V = np.asarray(center, float) + (pts * np.asarray(radii, float)) @ axes
    n = (rings - 1) * segs
    return V, [grid_faces(rings - 1, segs), fan(n, np.arange(segs)), fan(n + 1, np.arange(segs) + n - segs, top=True)]


def tube(C, rx, ry=None, segs=16, up=(0.0, 0.0, 1.0), domes=4, pointed=0.0):
    """A closed tube along C with a radius per point (ry along the binormal), domed ends (pointed: claws)."""
    C = np.asarray(C, float)
    T, N, B = frames(C, up)
    rx = np.broadcast_to(np.asarray(rx, float), (len(C),)).copy()
    ry = rx.copy() if ry is None else np.broadcast_to(np.asarray(ry, float), (len(C),)).copy()
    a = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    rings = []

    def ring(c, n, b, r1, r2):
        return c + np.outer(np.cos(a), n) * r1 + np.outer(np.sin(a), b) * r2

    for k in range(domes, 0, -1):
        phi = 0.5 * np.pi * k / (domes + 1)
        rings.append(ring(C[0] - T[0] * rx[0] * math.sin(phi), N[0], B[0], rx[0] * math.cos(phi), ry[0] * math.cos(phi)))
    for i in range(len(C)):
        rings.append(ring(C[i], N[i], B[i], rx[i], ry[i]))
    tip = rx[-1] * (1.0 + pointed * 3.0)
    for k in range(1, domes + 1):
        phi = 0.5 * np.pi * k / (domes + 1)
        rings.append(ring(C[-1] + T[-1] * tip * math.sin(phi), N[-1], B[-1], rx[-1] * math.cos(phi) ** (1 + pointed),
                          ry[-1] * math.cos(phi) ** (1 + pointed)))
    R = len(rings)
    V = np.vstack(rings + [C[0] - T[0] * rx[0], C[-1] + T[-1] * tip])
    return V, [grid_faces(R, segs), fan(R * segs, np.arange(segs)), fan(R * segs + 1, np.arange(segs) + (R - 1) * segs, top=True)]


def tube_fn(C, rfn, segs=24, up=(0.0, 0.0, 1.0), caps=True):
    """A tube along C whose radius rfn(s, a) varies along (s 0..1) and around (a); flat fans close its ends."""
    C = np.asarray(C, float)
    T, N, B = frames(C, up)
    s = np.linspace(0.0, 1.0, len(C))
    a = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    S, A = np.meshgrid(s, a, indexing='ij')
    R = rfn(S, A)
    P = C[:, None, :] + (N[:, None, :] * np.cos(A)[..., None] + B[:, None, :] * np.sin(A)[..., None]) * R[..., None]
    n = len(C) * segs
    if not caps:
        return orient_open(P.reshape(-1, 3), [grid_faces(len(C), segs)], C, s)
    V = np.vstack([P.reshape(-1, 3), C[0], C[-1]])
    return orient((V, [grid_faces(len(C), segs), fan(n, np.arange(segs)), fan(n + 1, np.arange(segs) + n - segs, top=True)]))


def orient_open(V, faces, C, s):
    """An open tube turned so its faces point away from its axis."""
    F = faces[0]
    centers = V[F].mean(1)
    k = np.clip(np.round(np.interp(np.arange(len(F)), [0, len(F) - 1], [0, len(C) - 1])).astype(int), 0, len(C) - 1)
    normal = np.cross(V[F[:, 1]] - V[F[:, 0]], V[F[:, 3]] - V[F[:, 0]])
    outward = np.sum(normal * (centers - C[k]), axis=1)
    return (V, faces) if np.sum(outward) > 0 else (V, [F[:, ::-1]])


def torus(center, axis, major, minor, segs=36, sides=12, squash=1.0):
    axis = unit(axis)
    a = unit(np.cross(axis, [0.31, 0.17, 0.93] if abs(axis[2]) < 0.9 else [1.0, 0.0, 0.0]))
    b = np.cross(axis, a)
    u = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    v = np.linspace(0.0, 2.0 * np.pi, sides, endpoint=False)
    radial = np.outer(np.cos(u), a) + np.outer(np.sin(u), b)
    V = (np.asarray(center, float) + radial[:, None, :] * major + radial[:, None, :] * (np.cos(v) * minor)[None, :, None]
         + axis[None, None, :] * (np.sin(v) * minor * squash)[None, :, None]).reshape(-1, 3)
    i, j = np.meshgrid(np.arange(segs), np.arange(sides), indexing='ij')
    i1, j1 = (i + 1) % segs, (j + 1) % sides
    return V, [np.stack([i * sides + j, i1 * sides + j, i1 * sides + j1, i * sides + j1], -1).reshape(-1, 4)]


def closed_loft(P):
    R, C = P.shape[:2]
    V = np.vstack([P.reshape(-1, 3), P[0].mean(0), P[-1].mean(0)])
    return V, [grid_faces(R, C), fan(R * C, np.arange(C)), fan(R * C + 1, np.arange(C) + (R - 1) * C, top=True)]


def merge(pieces):
    Vs, Fs, n = [], [], 0
    for V, faces in pieces:
        Vs.append(np.asarray(V, float).reshape(-1, 3))
        Fs += [np.asarray(F) + n for F in faces]
        n += len(Vs[-1])
    return np.concatenate(Vs), Fs


def transform(piece, R=None, t=(0.0, 0.0, 0.0), pivot=(0.0, 0.0, 0.0)):
    V, faces = piece
    V = np.asarray(V, float)
    if R is not None:
        V = (V - pivot) @ np.asarray(R).T + pivot
    return V + np.asarray(t, float), faces


def orient(piece):
    """A closed piece with its faces turned outward (by the sign of its volume)."""
    V, faces = piece
    vol = 0.0
    for F in faces:
        for k in range(1, F.shape[1] - 1):
            vol += np.sum(V[F[:, 0]] * np.cross(V[F[:, k]], V[F[:, k + 1]]))
    return (V, faces) if vol > 0 else (V, [F[:, ::-1] for F in faces])


def to_mesh(name, V, faces):
    mesh = bpy.data.meshes.new(name)
    flat = []
    for F in faces:
        flat += np.asarray(F).tolist()
    mesh.from_pydata(np.asarray(V, float).tolist(), [], flat)
    return mesh


def arrays(mesh):
    co = np.empty(len(mesh.vertices) * 3)
    mesh.vertices.foreach_get('co', co)
    tot = np.empty(len(mesh.polygons), dtype=np.int64)
    mesh.polygons.foreach_get('loop_total', tot)
    idx = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', idx)
    starts = np.concatenate([[0], np.cumsum(tot)[:-1]]).astype(np.int64)
    return co.reshape(-1, 3), [idx[starts[tot == k][:, None] + np.arange(k)[None, :]] for k in np.unique(tot)]


def _evaluated(piece, modifier):
    V, faces = piece
    mesh = to_mesh('_eval', V, faces)
    obj = bpy.data.objects.new('_eval', mesh)
    bpy.context.scene.collection.objects.link(obj)
    modifier(obj)
    depsgraph = bpy.context.evaluated_depsgraph_get()
    result = bpy.data.meshes.new_from_object(obj.evaluated_get(depsgraph))
    out = arrays(result)
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    bpy.data.meshes.remove(result)
    return out


def union(pieces, voxel, smooth=4):
    """Closed pieces fused by a voxel remesh, then smoothed where they met."""
    def mods(obj):
        remesh = obj.modifiers.new('Remesh', 'REMESH')
        remesh.mode = 'VOXEL'
        remesh.voxel_size = voxel
        remesh.adaptivity = 0.0
        sm = obj.modifiers.new('Smooth', 'SMOOTH')
        sm.factor = 0.5
        sm.iterations = smooth
    return _evaluated(merge(pieces), mods)


def reduce(piece, triangles):
    """A piece decimated (edge collapse) to about this many triangles, without slivers."""
    count = sum(len(F) * (F.shape[1] - 2) for F in piece[1])
    if count <= triangles:
        return clean(piece)

    def mods(obj):
        mod = obj.modifiers.new('Decimate', 'DECIMATE')
        mod.decimate_type = 'COLLAPSE'
        mod.ratio = triangles / count
        mod.use_collapse_triangulate = True
    return clean(_evaluated(piece, mods))


def clean(piece, min_area=2e-8):
    """Drops faces with almost no area and the vertices nothing uses: they would leave Unreal's tangents zero."""
    V, faces = piece
    kept = []
    for F in faces:
        area = np.zeros(len(F))
        for k in range(1, F.shape[1] - 1):
            area += 0.5 * np.linalg.norm(np.cross(V[F[:, k]] - V[F[:, 0]], V[F[:, k + 1]] - V[F[:, 0]]), axis=1)
        kept.append(F[area > min_area])
    used = np.unique(np.concatenate([F.ravel() for F in kept]))
    remap = -np.ones(len(V), dtype=np.int64)
    remap[used] = np.arange(len(used))
    return V[used], [remap[F] for F in kept if len(F)]


def normals(V, faces):
    N = np.zeros_like(V)
    for F in faces:
        if F.shape[1] == 3:
            fn = np.cross(V[F[:, 1]] - V[F[:, 0]], V[F[:, 2]] - V[F[:, 0]])
        else:
            fn = np.cross(V[F[:, 2]] - V[F[:, 0]], V[F[:, 3]] - V[F[:, 1]])
        for k in range(F.shape[1]):
            np.add.at(N, F[:, k], fn)
    return N / (np.linalg.norm(N, axis=1, keepdims=True) + 1e-12)


def falloff(V, center, radius, axes=None):
    d = np.asarray(V, float) - np.asarray(center, float)
    if axes is not None:
        d = d @ np.asarray(axes, float).T
    return smoothstep(1.0, 0.0, np.linalg.norm(d / np.asarray(radius, float), axis=1))


def push(piece, center, radius, amount):
    V, faces = piece
    return V + normals(V, faces) * (amount * falloff(V, center, radius))[:, None], faces


def cut(P, sd, closed=True):
    """The grid P (R, C, 3) cut to where sd <= 0, its outside corners slid onto the edge. Returns (V, faces, used)."""
    R, C = sd.shape
    F = grid_faces(R, C, closed)
    S = sd.ravel()
    inside = S <= 0.0
    F = F[inside[F].sum(1) >= 2]
    flat = P.reshape(-1, 3)
    V = flat.copy()
    used = np.unique(F)
    for v in used[~inside[used]]:
        i, j = divmod(int(v), C)
        hits = []
        for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            ii, jj = i + di, (j + dj) % C if closed else j + dj
            if 0 <= ii < R and 0 <= jj < C and inside[ii * C + jj]:
                w = ii * C + jj
                t = S[v] / (S[v] - S[w])
                hits.append(flat[v] + (flat[w] - flat[v]) * t)
        if hits:
            V[v] = np.mean(hits, axis=0)
    remap = -np.ones(len(V), dtype=np.int64)
    remap[used] = np.arange(len(used))
    return V[used], [remap[F]], used


def nearest(src, dst):
    """For each point of dst, the index of the nearest point of src."""
    tree = kdtree.KDTree(len(src))
    for i, p in enumerate(src):
        tree.insert(p, i)
    tree.balance()
    return np.array([tree.find(p)[1] for p in dst], dtype=np.int64)


def polyline_distance(V, pts):
    """Distance from each point of V to a polyline, and how far along it (0..1) the nearest point lies."""
    pts = np.asarray(pts, float)
    seg = np.diff(pts, axis=0)
    lengths = np.linalg.norm(seg, axis=1)
    starts = np.concatenate([[0.0], np.cumsum(lengths)[:-1]]) / lengths.sum()
    best = np.full(len(V), np.inf)
    along = np.zeros(len(V))
    for k in range(len(seg)):
        t = np.clip(((V - pts[k]) @ seg[k]) / (lengths[k] ** 2), 0.0, 1.0)
        d = np.linalg.norm(V - (pts[k] + t[:, None] * seg[k]), axis=1)
        better = d < best
        best[better] = d[better]
        along[better] = starts[k] + t[better] * lengths[k] / lengths.sum()
    return best, along


# --- The body's shapes (option D) ---

# Wasted to the bone in his own clothes: (z, half width, half depth, y of the middle, superellipse power).
TORSO_D = [(0.90, 0.150, 0.100, 0.005, 2.2), (0.98, 0.142, 0.094, 0.005, 2.2), (1.06, 0.132, 0.088, 0.0, 2.2),
           (1.16, 0.140, 0.094, -0.008, 2.3), (1.26, 0.155, 0.104, -0.014, 2.4), (1.34, 0.166, 0.108, -0.02, 2.6),
           (1.40, 0.175, 0.102, -0.028, 2.8), (1.45, 0.174, 0.094, -0.034, 3.2), (1.475, 0.162, 0.087, -0.038, 3.4),
           (1.50, 0.135, 0.079, -0.042, 3.0), (1.52, 0.098, 0.068, -0.045, 2.6), (1.54, 0.066, 0.06, -0.047, 2.2)]
HEAD_D = dict(width=0.14, front=0.092, back=0.104, top=0.114, bottom=0.138, mouth=-0.072, brow=0.009, brow_shelf=0.004,
              socket=0.017, eyeball=0.0075, nose=0.02, cheek=0.007, zyg=0.004, hollow=0.011, nasolabial=0.0028,
              lips=0.002, lips_thin=0.0015, chin=0.007, jaw_angle=0.005, nape=0.24, jaw_taper=0.12, socket_dark=0.95,
              temple=0.009, asym=0.12)
HINGE = np.array([0.0, 0.006, -0.03])    # the jaw's hinge in head space (head space: origin mid-head at eye height)
# The arms in the bind pose before the hunch: shoulder, elbow, wrist, and the hands (fingers' direction, back of hand).
SHOULDER = {s: np.array([s * 0.16, -0.022, 1.428]) for s in SIDES}
ELBOW = {1.0: np.array([0.205, -0.075, 1.12]), -1.0: np.array([-0.2, -0.04, 1.115])}
WRIST = {1.0: np.array([0.205, -0.2, 0.865]), -1.0: np.array([-0.225, -0.13, 0.84])}
HAND_DIR = {1.0: unit([0.05, -0.55, -1.0]), -1.0: unit([-0.06, -0.35, -1.0])}
HAND_UP = {1.0: unit([1.0, -0.25, 0.1]), -1.0: unit([-1.0, -0.15, 0.1])}
CURLS = {1.0: (0.35, 0.5, 0.62, 0.75), -1.0: (0.2, 0.32, 0.42, 0.55)}
SPREAD = {1.0: 0.1, -1.0: 0.14}
SHROUD_PATH = [(0.0, 0.005, 1.04), (0.0, 0.04, 0.84), (0.0, 0.17, 0.62), (0.0, 0.42, 0.45), (0.0, 0.75, 0.33),
               (0.0, 1.1, 0.27)]
SHROUD_SIZE = [(0.0, 0.135, 0.092), (0.12, 0.142, 0.104), (0.3, 0.118, 0.1), (0.5, 0.088, 0.08), (0.75, 0.056, 0.052),
               (1.0, 0.028, 0.028)]
TAIL_STOPS = (0.0, 0.2, 0.4, 0.6, 0.8, 1.0)


def hunch(V):
    """The bind pose's curve of the upper back: everything above the chest turned forward about the X axis."""
    V = np.atleast_2d(np.asarray(V, float))
    a = HUNCH * smoothstep(1.25, 1.41, V[:, 2])
    y, z = V[:, 1], V[:, 2] - 1.33
    out = V.copy()
    out[:, 1] = y * np.cos(a) - z * np.sin(a)
    out[:, 2] = 1.33 + y * np.sin(a) + z * np.cos(a)
    return out


def hunched(p):
    return hunch(np.asarray(p, float)[None])[0]


def torso_points(profile, z, theta, grow=0.0):
    z = np.asarray(z, float)
    w, d, yc, p = table(profile, z)
    c, s = np.cos(theta)[None, :], np.sin(theta)[None, :]
    e = (2.0 / p)[:, None]
    cf, sl = np.sign(c) * np.abs(c) ** e, np.sign(s) * np.abs(s) ** e
    x = (w[:, None] + grow) * sl
    y = yc[:, None] - (d[:, None] + grow) * cf
    return np.stack([x, y, np.broadcast_to(z[:, None], x.shape)], axis=-1)


def torso_normal(theta):
    return np.array([math.sin(theta), -math.cos(theta), 0.0])


def head_surface(spec, rows=110, cols=112):
    """The hungry head in head space (front -Y, origin mid-head at eye height): one smooth surface, a radius for every
    direction, shaped by brow, sockets with sunk eyes, nose, cheekbones and their arches, hollow cheeks and temples, the
    folds from nose to mouth, thin lips, chin and jaw. The lips are parted by a slit (the mouth) that the jaw opens.
    Returns ((V, faces), cavity, jaw weight)."""
    g = gauss
    phi = np.linspace(0.0, np.pi, rows + 2)[1:-1]
    th = np.linspace(-np.pi, np.pi, cols, endpoint=False)
    PH, TH = np.meshgrid(phi, th, indexing='ij')
    d = np.stack([np.sin(PH) * np.sin(TH), -np.sin(PH) * np.cos(TH), np.cos(PH)], -1)
    a = spec['width'] / 2.0
    b = np.where(d[..., 1] < 0, spec['front'], spec['back'])
    c = np.where(d[..., 2] > 0, spec['top'], spec['bottom'])
    r = 1.0 / np.sqrt((d[..., 0] / a) ** 2 + (d[..., 1] / b) ** 2 + (d[..., 2] / c) ** 2)
    z = d[..., 2] * r
    at = np.abs(TH)
    zm = spec['mouth']
    r = r * (1.0 - spec['nape'] * smoothstep(-0.03, -0.11, z) * smoothstep(1.9, 2.8, at))
    r = r * (1.0 - spec['jaw_taper'] * smoothstep(zm + 0.01, -spec['bottom'], z) * smoothstep(2.2, 1.2, at)
             * np.abs(np.sin(TH)))
    nose = (spec['nose'] * smoothstep(0.016, -0.038, z) * smoothstep(-0.058, -0.04, z)
            * g(TH, 0.07 + 0.09 * smoothstep(-0.01, -0.045, z)))
    lop = 1.0 + spec['asym'] * (TH < 0)
    nl = 0.2 + 0.22 * np.clip((-0.045 - z) / 0.035, 0.0, 1.0)
    r = r + (spec['brow'] * g(TH, 0.62) * g(z - 0.022, 0.011)
             - spec['socket'] * lop * (g(TH - 0.36, 0.16) + g(TH + 0.36, 0.16)) * g(z - 0.002 + 0.004 * (lop - 1.0), 0.016)
             + nose + 0.004 * g(at - 0.17, 0.06) * g(z + 0.043, 0.008)
             + spec['cheek'] * g(at - 0.68, 0.22) * g(z + 0.02, 0.015)
             - spec['hollow'] * g(at - 0.62, 0.25) * g(z + 0.055, 0.022)
             + spec['lips'] * g(TH, 0.42) * g(z - zm, 0.022)
             + spec['chin'] * g(TH, 0.32) * g(z + spec['bottom'] - 0.016, 0.013)
             - spec['temple'] * g(at - 1.05, 0.22) * g(z - 0.04, 0.025)
             + spec['brow_shelf'] * (g(TH - 0.36, 0.2) + g(TH + 0.36, 0.2)) * g(z - 0.017, 0.006)
             + spec['eyeball'] * lop * (g(TH - 0.36, 0.085) + g(TH + 0.36, 0.085)) * g(z + 0.002, 0.009)
             + spec['zyg'] * g(z + 0.018, 0.009) * smoothstep(0.55, 0.75, at) * smoothstep(1.55, 1.3, at)
             + spec['jaw_angle'] * g(at - 1.22, 0.2) * g(z + 0.085, 0.02)
             - spec['nasolabial'] * g(at - nl, 0.045) * smoothstep(-0.04, -0.05, z) * smoothstep(-0.088, -0.078, z)
             + spec['lips_thin'] * g(TH, 0.3) * (g(z - zm - 0.007, 0.0035) + g(z - zm + 0.008, 0.0035)))
    P = d * r[..., None]
    zz = P[..., 2]
    cav = np.clip(1.6 * (g(TH - 0.36, 0.12) + g(TH + 0.36, 0.12)) * g(zz, 0.012), 0.0, 1.0) * spec['socket_dark']
    ball = np.clip(1.6 * (g(TH - 0.36, 0.06) + g(TH + 0.36, 0.06)) * g(zz + 0.002, 0.007), 0.0, 1.0)
    cav = cav * (1.0 - 0.45 * ball)
    cav = np.maximum(cav, 0.35 * spec['nasolabial'] / 0.004 * g(at - nl, 0.03)
                     * smoothstep(-0.04, -0.05, zz) * smoothstep(-0.088, -0.078, zz))
    # The mouth: a slit along the row nearest the lip line, across the mouth's width; the lower lip, the chin and the
    # jaw below it turn with the jaw, the cheeks beyond the corners stretch.
    mouth_half = 0.42
    centre_col = cols // 2
    i_m = int(np.argmin(np.abs(zz[:, centre_col] - zm)))
    slit = np.where(np.abs(th) < mouth_half)[0]
    j0, j1 = slit.min(), slit.max()
    inner = np.arange(j0 + 1, j1)                 # the slit's own columns; its ends stay joined (the corners)
    inside = smoothstep(mouth_half + 0.16, mouth_half, at)
    sharp = (np.arange(rows)[:, None] > i_m).astype(float) * np.ones_like(at)
    soft = smoothstep(zm + 0.004, zm - 0.03, zz) * smoothstep(2.3, 1.5, at)
    jw = inside * sharp + (1.0 - inside) * soft
    cav = np.maximum(cav, 0.45 * g(TH, 0.3) * g(zz - zm, 0.004))
    V = P.reshape(-1, 3)
    n = rows * cols
    dup = n + np.arange(len(inner))               # the lower lip's copies of the slit's vertices
    V = np.vstack([V, P[i_m, inner], P[0].mean(0) + (0.0, 0.0, 0.002), P[-1].mean(0) - (0.0, 0.0, 0.002)])
    jw_v = np.concatenate([jw.ravel(), np.ones(len(inner)), [0.0, 1.0]])
    jw_v[i_m * cols + inner] = 0.0
    cav_v = np.concatenate([cav.ravel(), np.full(len(inner), 0.5), [0.0, 0.0]])
    cav_v[i_m * cols + inner] = 0.5
    F = grid_faces(rows, cols)
    lower = (F[:, 0] // cols == i_m)              # the quads just under the slit's row
    swap = {int(i_m * cols + j): int(k) for j, k in zip(inner, dup)}
    for f in np.where(lower)[0]:
        F[f] = [swap.get(int(v), int(v)) if v // cols == i_m else v for v in F[f]]
    top, bottom = n + len(inner), n + len(inner) + 1
    piece = orient((V, [F, fan(top, np.arange(cols)), fan(bottom, np.arange(cols) + n - cols)]))
    return piece, cav_v, jw_v


def jaw_turn(V, jw, angle):
    """Head-space points turned about the jaw's hinge by angle, as much as their jaw weight."""
    a = angle * np.asarray(jw, float)
    y, z = V[:, 1] - HINGE[1], V[:, 2] - HINGE[2]
    out = V.copy()
    out[:, 1] = HINGE[1] + y * np.cos(a) - z * np.sin(a)
    out[:, 2] = HINGE[2] + y * np.sin(a) + z * np.cos(a)
    return out


def hand_pieces(W, fwd, up, side, curl, spread, thumb=0.35, length=1.26, thin=0.82, claw=0.3, knuckle=1.3, scale=1.02,
                rest_turn=None):
    """A long-fingered hand from the wrist W (fwd along the fingers, up out of its back; side +1 left): closed pieces for
    a union, and each finger's line (name, points, radius) for its weights. rest_turn(finger, knuckle), if given, is a
    rotation each finger is turned by about its knuckle (where its bone starts) after it's shaped."""
    def turned(name, pts, at, lu):
        if rest_turn is None:
            return pts, lu
        R = rest_turn(name, pts[at])
        return pts[:at + 1] + [pts[at] + R @ (p - pts[at]) for p in pts[at + 1:]], R @ lu

    f = unit(fwd)
    u = unit(np.asarray(up, float) - f * np.dot(up, f))
    t = np.cross(f, u) * side
    frame = np.array([f, np.cross(u, f), u])
    s = scale
    W = np.asarray(W, float)
    pieces = [ellipsoid(W + f * 0.047 * s * length ** 0.3, (0.047 * s * length ** 0.3, 0.041 * s, 0.0145 * s * thin),
                        frame, 24, 14),
              tube([W - f * 0.04 * s, W + f * 0.02 * s], [0.019 * s * thin, 0.016 * s * thin],
                   [0.026 * s * thin, 0.022 * s * thin], segs=16, up=u)]
    lines = []
    offsets = (0.026, 0.0085, -0.0095, -0.026)
    lengths = (0.074, 0.083, 0.078, 0.062)
    arch = (0.003, 0.005, 0.003, -0.001)
    fans = (1.4, 0.45, -0.5, -1.5)
    for k, name in enumerate(('index', 'middle', 'ring', 'pinky')):
        base = W + f * 0.088 * s * length ** 0.3 + t * offsets[k] * s + u * arch[k] * s
        a = spread * fans[k]
        d = f * math.cos(a) + t * math.sin(a)
        lu = u.copy()
        pts = [base]
        L = lengths[k] * length * s
        for share, bendk in zip((0.45, 0.31, 0.24), (0.8, 1.05, 0.75)):
            b = curl[k] * bendk
            d, lu = d * math.cos(b) - lu * math.sin(b), lu * math.cos(b) + d * math.sin(b)
            pts.append(pts[-1] + d * L * share)
        pts, lu = turned(name, pts, 0, lu)
        r0 = 0.0089 * s * thin
        radii = [r0, r0 * 0.93, r0 * 0.84, r0 * (0.7 - 0.35 * claw)]
        C = spline(pts, 14)
        rr = np.interp(np.linspace(0, 1, 14), [0, 0.45, 0.76, 1.0], radii)
        pieces.append(tube(C, rr, segs=12, up=lu, domes=3, pointed=claw))
        pieces.append(ellipsoid(base + u * 0.003 * s, (0.0105 * s * knuckle, 0.0098 * s * knuckle, 0.0085 * s * knuckle),
                                frame, 12, 8))
        lines.append((name, np.array(pts), r0))
    tb = W + f * 0.018 * s + t * 0.022 * s - u * 0.006 * s
    d = unit(f * 0.55 + t * 0.75 - u * 0.35)
    lu = unit(np.cross(d, f) * side + u * 0.5)
    pts = [tb, tb + d * 0.042 * s]
    for share in (0.036, 0.03):
        towards = unit(f - d * np.dot(f, d))
        d = unit(d * math.cos(thumb) + towards * math.sin(thumb) - u * 0.25 * math.sin(thumb))
        pts.append(pts[-1] + d * share * s * length ** 0.5)
    pts, lu = turned('thumb', pts, 1, lu)
    C = spline(pts, 14)
    rr = np.interp(np.linspace(0, 1, 14), [0, 0.4, 0.75, 1.0],
                   [0.0145 * s * thin, 0.0105 * s * thin, 0.0092 * s * thin, 0.0072 * s * thin * (1 - 0.4 * claw)])
    pieces.append(tube(C, rr, segs=12, up=lu, domes=3, pointed=claw))
    lines.append(('thumb', np.array(pts[1:]), 0.0125 * s * thin))
    return pieces, lines


def sleeve(S, E, W, r0, r1, seed):
    """A loose shirt sleeve from the shoulder to just past the elbow, with three folds winding down from the armpit and
    a pile of small folds above the rolled cuff."""
    rng = np.random.default_rng(seed)
    d = unit(E - S)
    C = spline([S, (S + E) * 0.5 + np.cross(d, [0.0, 0.0, 1.0]) * 0.004, E + unit(W - E) * 0.03], 28)
    starts = rng.uniform(0.0, 2.0 * np.pi, 3)
    twists = rng.uniform(0.8, 1.6, 3) * rng.choice((-1.0, 1.0), 3)
    amps = rng.uniform(0.1, 0.17, 3)

    def rfn(s, a):
        base = r0 + (r1 - r0) * s
        f = sum(k * np.exp(-(wrap(a - p - t * s) / 0.38) ** 2) for k, p, t in zip(amps, starts, twists))
        f = f * smoothstep(0.04, 0.25, s) * smoothstep(0.96, 0.7, s)
        pile = sum(0.07 * np.exp(-((s - sk) / 0.028) ** 2) * (0.6 + 0.4 * np.cos(a + ph))
                   for sk, ph in ((0.76, 0.5), (0.84, 2.4), (0.91, 4.1)))
        return base * (1.0 + f + pile - 0.04)
    return tube_fn(C, rfn, segs=28), C


def ribbon(path, width, n=8, cols=3, out=(0.0, 0.0, 1.0), curl=0.25, twist=0.0):
    """A strip of cloth along path: width a number or rows of (t, width); its face turned toward out (a direction, or
    a function of the points)."""
    C = spline(path, n)
    T = unit(np.gradient(C, axis=0))
    O = np.asarray(out(C), float) if callable(out) else np.broadcast_to(np.asarray(out, float), C.shape).copy()
    O = unit(O - T * np.sum(O * T, axis=1, keepdims=True))
    A = np.cross(T, O)
    t = np.linspace(0.0, 1.0, n)
    if twist:
        a = 2.0 * np.pi * twist * t
        O, A = O * np.cos(a)[:, None] + A * np.sin(a)[:, None], A * np.cos(a)[:, None] - O * np.sin(a)[:, None]
    w = np.interp(t, [r[0] for r in width], [r[1] for r in width]) if isinstance(width, (list, tuple)) else np.full(n, width)
    across = np.linspace(-1.0, 1.0, cols)
    P = C[:, None, :] + A[:, None, :] * (w[:, None] * across[None, :])[..., None] \
        + O[:, None, :] * (curl * w[:, None] * (1.0 - across ** 2)[None, :])[..., None]
    return P.reshape(-1, 3), [grid_faces(n, cols, closed=False)]


def shroud(rows=13, cols=24, strips=7, split=0.16, ends=(0.74, 1.0), spread=0.1, seed=4, fade_from=0.22, folds=0.34,
           narrow=0.55, wave=0.035, harmonics=(5, 7, 9, 12), twist=0.8):
    """The shroud at game density: a tube from the waist along SHROUD_PATH tearing into strips past split. Returns the
    piece and, per vertex, how far down the shroud it is (t), its fade, how far down its strip (q), its strip's side
    (+1 left, -1 right, 0 the middle) and its UV makings (the angle round the shroud from the front, its radius there,
    and V: minus the meters down it, so Unreal's V, flipped on import, runs down the strips as M_Ghost wants)."""
    rng = np.random.default_rng(seed)
    C = spline(SHROUD_PATH, 300)
    length = float(np.linalg.norm(np.diff(C, axis=0), axis=1).sum())
    T, N, B = frames(C, (0.0, -1.0, 0.0))
    amps = rng.uniform(0.55, 1.0, len(harmonics))
    phases = rng.uniform(0.0, 2.0 * np.pi, len(harmonics))
    drift = rng.uniform(-1.0, 1.0, len(harmonics)) * twist

    def at(t):
        return sample(C, t), sample(N, t), sample(B, t)

    def point(t, theta, out=0.0):
        t = np.asarray(t, float)
        c, n, b = at(t)
        w, d = table(SHROUD_SIZE, t)
        f = sum(a * np.cos(k * theta + p + dr * t * 6.0) for a, k, p, dr in zip(amps, harmonics, phases, drift)) / amps.sum()
        r = 1.0 + folds * smoothstep(0.04, 0.4, t) * (f - 0.35 * np.abs(f))
        return (c + n * (d * r * np.cos(theta) + out * np.cos(theta))[..., None]
                + b * (w * r * np.sin(theta) + out * np.sin(theta))[..., None])

    theta = np.linspace(0.0, 2.0 * np.pi, cols, endpoint=False)
    mrows = 3
    tt, th = np.meshgrid(np.linspace(0.0, split, mrows), theta, indexing='ij')
    pieces = [(point(tt, th).reshape(-1, 3), [grid_faces(mrows, cols)])]
    ts, fades, qs, sides = [tt.ravel()], [(1.0 - smoothstep(fade_from, 1.0, tt)).ravel()], [np.zeros(tt.size)], [np.zeros(tt.size)]
    angles = [wrap(th).ravel()]           # from the front, its seam at the back
    bounds = np.unique(np.sort((np.arange(strips) + rng.uniform(-0.25, 0.25, strips)) * cols / strips).astype(int) % cols)
    srows = rows - mrows + 1
    for k in range(len(bounds)):
        j0, j1 = bounds[k], bounds[(k + 1) % len(bounds)]
        if j1 <= j0:
            j1 += cols
        js = np.arange(j0, j1 + 1)
        th_j = theta[js % cols] + 2.0 * np.pi * (js // cols)
        tc = th_j.mean()
        end = rng.uniform(*ends)
        jag = end - 0.05 * np.abs(np.sin(js * 1.7 + k)) - 0.06 * smoothstep(0.0, 1.0, np.abs(js - js.mean()) / max(len(js) / 2.0, 1.0)) ** 2
        q = np.linspace(0.0, 1.0, srows)[:, None]
        tq = split + (jag[None, :] - split) * q
        ang = tc + (th_j[None, :] - tc) * (1.0 - narrow * q)
        lift = spread * q ** 1.6 * (1.0 + 0.5 * rng.uniform(-1, 1))
        Q = point(tq, ang, out=lift)
        c, n, b = at(split + (np.mean(jag) - split) * q[:, 0])
        side_dir = -math.sin(tc) * n + math.cos(tc) * b
        Q = Q + (side_dir * (wave * q * np.sin(q * 7.0 + k * 1.3)))[:, None, :]
        pieces.append((Q.reshape(-1, 3), [grid_faces(srows, len(js), closed=False)]))
        ts.append(tq.ravel())
        fades.append((1.0 - smoothstep(fade_from, jag[None, :], tq)).ravel())
        qs.append(np.broadcast_to(q, tq.shape).ravel())
        angles.append((wrap(tc) + ang - tc).ravel())      # unbroken across a strip, even one at the back
        mid_x = Q[srows // 2, :, 0].mean()
        sides.append(np.full(tq.size, 1.0 if mid_x > 0.045 else (-1.0 if mid_x < -0.045 else 0.0)))
    V, F = merge(pieces)
    t = np.concatenate(ts)
    w, d = table(SHROUD_SIZE, t)
    radius = (3.0 * (w + d) - np.sqrt((3.0 * w + d) * (w + 3.0 * d))) / 2.0      # its perimeter there (Ramanujan) / 2 pi
    uv = np.column_stack([np.concatenate(angles), radius, -t * length])
    return (V, F), t, np.concatenate(fades), np.concatenate(qs), np.concatenate(sides), uv


def hat_pieces():
    """The slouch hat in hat space (origin at the brim's middle, front -Y): felt (brim with a rolled edge, a crown
    pinched in front and creased on top) and band. Game density; the brim is a thin solid."""
    theta = np.linspace(0.0, 2.0 * np.pi, 32, endpoint=False)
    q = np.linspace(0.0, 1.0, 5)[:, None]
    r = 0.084 + 0.108 * q * (1.0 + 0.04 * np.sin(3 * theta + 0.6))
    droop = -0.034 * q ** 1.7 * (0.55 + 0.45 * np.cos(2 * theta)) + 0.012 * q ** 2 * np.sin(theta) ** 2
    droop = droop - 0.01 * q ** 2 * np.cos(theta) + 0.004 * q * np.sin(5 * theta + 1.0)
    top = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.06, droop], -1)
    under = top - np.array([0.0, 0.0, 0.009])
    # The brim: top and underside, closed round the outer edge (its inner edge sits inside the crown).
    R = 5
    V = np.vstack([top.reshape(-1, 3), under.reshape(-1, 3)])
    # Faces: the top facing up, the underside down, the rolled outer edge outward.
    F = [grid_faces(R, 32)[:, ::-1], grid_faces(R, 32) + R * 32]
    outer = np.arange(32)
    edge = np.stack([(R - 1) * 32 + outer, (R - 1) * 32 + (outer + 1) % 32, R * 32 + (R - 1) * 32 + (outer + 1) % 32,
                     R * 32 + (R - 1) * 32 + outer], 1)
    F.append(edge[:, ::-1])
    brim = (V, F)
    zc = np.array([-0.004, 0.025, 0.05, 0.07, 0.084, 0.091])
    rc = np.array([0.088, 0.086, 0.082, 0.076, 0.066, 0.05])
    rings = []
    tc = np.linspace(0.0, 2.0 * np.pi, 24, endpoint=False)     # the crown is smaller: fewer columns round it
    pinch = 1.0 - 0.1 * np.exp(-((np.abs(wrap(tc)) - 0.55) / 0.3) ** 2)
    for z, rr in zip(zc, rc):
        k = 1.0 + (pinch - 1.0) * (max(z, 0.0) / 0.091)
        rings.append(np.stack([rr * k * np.sin(tc), -rr * k * np.cos(tc) * 1.1, np.full_like(tc, z)], -1))
    # The top closes over a crease: a fan from the last ring down to a dip in the middle.
    P = np.array(rings)
    n = len(rings) * 24
    crown = (np.vstack([P.reshape(-1, 3), [[0.0, 0.0, 0.074]]]),
             [grid_faces(len(rings), 24), fan(n, np.arange(24) + n - 24, top=True)])
    band = np.array([np.stack([0.0905 * np.sin(theta), -0.0905 * np.cos(theta) * 1.1, np.full_like(theta, z)], -1)
                     for z in (0.003, 0.016, 0.03)])
    band = (band.reshape(-1, 3), [grid_faces(3, 32)])
    return merge([brim, crown]), band


# --- Building the body, part by part ---

class Part:
    """Geometry with what the material reads per vertex and the weights it is skinned with."""

    def __init__(self, name, piece, zone, slot=0, cavity=0.0, ember=0.0, fade=1.0, strip_uv=None):
        V, faces = piece
        self.name, self.zone, self.slot = name, zone, slot
        # Its own UVs instead of the box projection (the shroud): per vertex, the angle round it, meters per radian and V.
        self.strip_uv = strip_uv
        self.V = np.asarray(V, float)
        self.F = [np.asarray(F) for F in faces if len(F)]
        n = len(self.V)
        self.cavity = np.broadcast_to(np.asarray(cavity, float), (n,)).copy()
        self.ember = np.broadcast_to(np.asarray(ember, float), (n,)).copy()
        self.fade = np.broadcast_to(np.asarray(fade, float), (n,)).copy()
        self.weights = {}

    def weigh(self, bone, w):
        w = np.broadcast_to(np.asarray(w, float), (len(self.V),))
        self.weights[bone] = self.weights.get(bone, np.zeros(len(self.V))) + w
        return self

    def triangles(self):
        return sum(len(F) * (F.shape[1] - 2) for F in self.F)


def chain(x, stops, bones):
    """Weights over a chain of bones by a coordinate x: bones[k] whole at stops[k], shared linearly between."""
    x = np.clip(np.asarray(x, float), stops[0], stops[-1])
    out = {b: np.zeros_like(x) for b in bones}
    k = np.clip(np.searchsorted(stops, x, side='right') - 1, 0, len(stops) - 2)
    f = (x - np.asarray(stops)[k]) / (np.asarray(stops)[k + 1] - np.asarray(stops)[k])
    for i, b in enumerate(bones):
        out[b] += np.where(k == i, 1.0 - f, 0.0) + np.where(k == i - 1, f, 0.0)
    return out


def torso_weights(part, z, x=None, shoulders=True):
    """The spine's share of torso cloth by height (before the hunch; each bone whole at its middle), the neck's only
    close round the neck (the shoulders stay with the chest when he turns his head), and the shoulders' share to the
    upper arms."""
    w = chain(z, (1.04, 1.2, 1.38, 1.58), ('pelvis', 'spine_01', 'spine_02', 'neck'))
    if x is not None:
        away = smoothstep(0.06, 0.11, np.abs(x))
        w['spine_02'] = w['spine_02'] + w['neck'] * away
        w['neck'] = w['neck'] * (1.0 - away)
    arm = np.zeros(len(z))
    if shoulders and x is not None:
        arm = 0.35 * smoothstep(0.12, 0.17, np.abs(x)) * smoothstep(1.36, 1.43, z)
    for bone, value in w.items():
        part.weigh(bone, value * (1.0 - arm))
    if shoulders and x is not None:
        for s in SIDES:
            part.weigh(f'upperarm_{SUFFIX[s]}', arm * (np.sign(x) == s))
    return part


def build_torso():
    """Shirt (torso with the coal's crater, collar, sleeves, rolled cuffs), vest with buttons, bandana, coal."""
    parts = []
    S = {s: hunched(SHOULDER[s]) for s in SIDES}
    # The shirt's torso, loose on him: folds round the tails, the hollow chest, the crater.
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    z = np.linspace(0.90, 1.54, 40)
    P = torso_points(TORSO_D, z, theta)
    tails = smoothstep(1.0, 0.9, z)[:, None]
    folds = (np.cos(5 * theta + 0.7) * 0.6 + np.cos(9 * theta + 2.1) * 0.4)[None, :]
    P[..., 0] *= 1.0 + 0.06 * tails * folds
    P[..., 1] *= 1.0 + 0.06 * tails * folds
    P[0, :, 2] += 0.016 * np.sin(3 * theta + 1.0) + 0.009 * np.sin(7 * theta)
    dense = closed_loft(P)
    dense = push(dense, (0.0, -0.11, 1.4), (0.03, 0.04, 0.09), -0.012)
    for s in SIDES:
        dense = push(dense, (s * 0.06, -0.09, 1.46), 0.032, -0.008)
    tc, zc = 0.3, 1.32
    pc = torso_points(TORSO_D, [zc], np.array([tc]))[0, 0]
    nc = torso_normal(tc)
    dense = push(dense, pc, 0.068, -0.02)
    dist = np.linalg.norm(dense[0] - pc, axis=1)
    jitter = 0.5 + 0.5 * pnoise(dense[0], 60.0, 5)
    d_cav = smoothstep(0.06, 0.035, dist) * 0.9
    d_ember = smoothstep(0.03, 0.05, dist) * smoothstep(0.085, 0.062, dist) * (0.4 + 0.6 * jitter)
    low = reduce(dense, TRIANGLES['shirt'])
    near = nearest(dense[0], low[0])
    shirt = Part('Shirt', (hunch(low[0]), low[1]), SHIRT, cavity=d_cav[near], ember=d_ember[near])
    torso_weights(shirt, low[0][:, 2], low[0][:, 0])
    parts.append(shirt)
    coal_center = hunched(pc - nc * 0.012)
    coal_normal = unit(hunched(pc + nc * 0.05) - hunched(pc))
    # The collar, too big now for the neck.
    collar = np.array([np.stack([0.05 * np.sin(theta[::3]), -0.043 - 0.047 * np.cos(theta[::3]),
                                 np.full(32, zz)], -1) for zz in (1.52, 1.55, 1.578)])
    part = Part('Collar', (hunch(collar.reshape(-1, 3)), [grid_faces(3, 32)]), SHIRT)
    parts.append(part.weigh('neck', 0.5).weigh('spine_02', 0.5))
    # Sleeves and cuffs, along the bind pose's arms: the cuffs rolled round the forearms where they bend.
    for k, s in enumerate(SIDES):
        sfx = SUFFIX[s]
        E, W = bent(s)[:2]
        piece, line = sleeve(S[s], E, W, 0.056, 0.05, seed=31 + k)
        low = reduce(piece, TRIANGLES['sleeve'])
        part = Part(f'Sleeve_{sfx}', low, SHIRT)
        d, along = polyline_distance(low[0], line)
        part.weigh('spine_02', 0.45 * smoothstep(0.16, 0.0, along))
        part.weigh(f'lowerarm_{sfx}', 0.5 * smoothstep(0.84, 1.0, along))
        part.weigh(f'upperarm_{sfx}', 1.0 - 0.45 * smoothstep(0.16, 0.0, along) - 0.5 * smoothstep(0.84, 1.0, along))
        parts.append(part)
        dv = unit(W - E)
        cuff = torus(E + dv * 0.04, W - E, 0.046, 0.015, segs=14, sides=6, squash=0.85)
        parts.append(Part(f'Cuff_{sfx}', cuff, SHIRT).weigh(f'upperarm_{sfx}', 0.5).weigh(f'lowerarm_{sfx}', 0.5))

    # The vest: it hangs off him, stands clear of the hollow chest, folds slack down the front, pulls at the buttons.
    zv = np.linspace(0.92, 1.545, 120)
    tv = np.linspace(0.0, 2.0 * np.pi, 220, endpoint=False)
    th, zz = np.meshgrid(wrap(tv), zv)
    bridge = 0.009 * gauss(th, 0.5) * gauss(zz - 1.35, 0.08)
    fold = (sum(0.0055 * gauss(th - k, 0.07) for k in (0.42, 0.86, -0.45, -0.88)) * smoothstep(1.33, 1.05, zz)
            + sum(0.005 * gauss(th - k, 0.085) for k in (np.pi - 0.35, -np.pi + 0.35, np.pi - 0.8)) * smoothstep(1.42, 1.1, zz)
            + 0.005 * gauss(np.abs(th) - 1.57, 0.1) * smoothstep(1.32, 1.1, zz))
    buttons = (1.005, 1.07, 1.135, 1.2)
    for zb in buttons:
        for s in SIDES:
            reach = s * (th - 0.03)
            fold = fold + 0.0035 * gauss(zz - zb - 0.06 * reach, 0.006) * smoothstep(0.02, 0.1, reach) * smoothstep(0.5, 0.22, reach)
    P = np.array([torso_points(TORSO_D, [zrow], tv)[0] for zrow in zv])
    out = np.stack([np.sin(tv), -np.cos(tv), np.zeros_like(tv)], -1)[None, :, :]
    P = P + out * (0.016 + bridge + fold)[..., None]
    hem = 0.98 - 0.05 * np.exp(-((np.abs(th) - 0.2) / 0.13) ** 2) + 0.008 * np.sin(5 * th + 0.4)
    sd = np.maximum(hem - zz, np.where(np.abs(th) < 1.2, zz - (1.24 + 0.36 * np.abs(th)), -1.0))
    armhole = np.sqrt(((np.abs(th) - np.pi / 2) / 0.66) ** 2 + ((zz - 1.41) / 0.1) ** 2)
    sd = np.maximum(sd, (1.0 - armhole) * 0.06)
    sd = np.maximum(sd, zz - 1.53)
    ang = np.arctan2(zz - zc, (th - tc) * 0.17)
    rh = 0.07 * (1.0 + 0.12 * np.sin(ang * 5 + 1.0) + 0.08 * np.sin(ang * 11.0))
    hole = np.hypot((th - tc) * 0.17, zz - zc)
    sd = np.maximum(sd, rh - hole)
    V, F, used = cut(P, sd)
    d_ember = np.clip(smoothstep(0.03, 0.0, hole.ravel()[used] - rh.ravel()[used]) * (0.5 + 0.5 * pnoise(V, 50.0, 2)), 0, 1)
    low = reduce((V, F), TRIANGLES['vest'])
    near = nearest(V, low[0])
    vest = Part('Vest', (hunch(low[0]), low[1]), VEST, ember=d_ember[near])
    torso_weights(vest, low[0][:, 2], low[0][:, 0], shoulders=False)
    parts.append(vest)
    n = torso_normal(0.03)
    axes = np.array([[0.0, 0.0, 1.0], np.cross(n, [0.0, 0.0, 1.0]), n])
    for zb in buttons:
        p = torso_points(TORSO_D, [zb], np.array([0.03]), grow=0.022)[0, 0]
        button = ellipsoid(p, (0.0075, 0.0075, 0.0045), axes, 6, 3)
        part = Part('Button', (hunch(button[0]), button[1]), VEST, cavity=0.45)
        parts.append(torso_weights(part, button[0][:, 2], shoulders=False))

    # The bandana, loose round a thin neck: the rolled knot, the triangle over the chest, the knot behind.
    knot = torus((0.0, -0.046, 1.566), (0.0, -0.3, 1.0), 0.047, 0.011, segs=14, sides=5, squash=0.75)
    parts.append(Part('BandanaKnot', (hunch(knot[0]), knot[1]), ACCENT).weigh('neck', 0.6).weigh('spine_02', 0.4))
    rows, cols = 6, 9
    zb = np.linspace(1.545, 1.43, rows)
    width = np.linspace(0.9, 0.06, rows)
    flap = np.array([torso_points(TORSO_D, [zr], np.linspace(-w, w, cols), grow=0.03)[0] for zr, w in zip(zb, width)])
    flap[..., 2] += 0.005 * np.sin(np.linspace(0, 3 * np.pi, cols))[None, :] * (1 - width[:, None])
    flat = flap.reshape(-1, 3)
    part = Part('BandanaFlap', (hunch(flat), [grid_faces(rows, cols, closed=False)]), ACCENT)
    parts.append(part.weigh('spine_02', 1.0 - 0.4 * smoothstep(1.5, 1.545, flat[:, 2]))
                 .weigh('neck', 0.4 * smoothstep(1.5, 1.545, flat[:, 2])))
    back = ellipsoid((0.0, 0.0, 1.562), (0.018, 0.013, 0.015), segs=8, rings=5)
    parts.append(Part('BandanaBack', (hunch(back[0]), back[1]), ACCENT).weigh('neck', 1.0))

    # The coal, burning where the heart was.
    V, F = ellipsoid(coal_center, (0.056, 0.056, 0.052), segs=12, rings=8)
    d = V - coal_center
    V = V + d * (0.16 * pnoise(V, 30.0, 3))[:, None]
    parts.append(Part('Coal', (V, F), SKIN, slot=1).weigh('coal', 1.0))
    return parts, dict(coal=(coal_center, coal_normal), shoulders=S)


def build_arms():
    """Forearms and hands, fused per side and reduced; fingers on their own bones past the knuckles. Built in the rest
    pose, the idle: the forearm bent at the elbow, the hand the D concept's as modeled. Also returns the older rest's
    finger lines (straight forearm, fingers turned back by the code's idle), which the skeleton is laid out on first."""
    parts, fingers, old_fingers = [], {}, {}
    for s in SIDES:
        sfx = SUFFIX[s]
        E, W, fwd, up = bent(s)
        dv = unit(W - E)
        pieces = [tube([E + dv * 0.02, E + dv * 0.12, (E + W) * 0.5, W - dv * 0.01], [0.033, 0.031, 0.026, 0.017],
                       [0.036, 0.034, 0.03, 0.024], segs=20, up=up),
                  ellipsoid(W - dv * 0.012 + unit(np.cross(dv, up)) * s * -0.018, (0.008, 0.008, 0.008))]
        hand, lines = hand_pieces(W, fwd, up, s, CURLS[s], SPREAD[s])
        old_fingers[s] = hand_pieces(WRIST[s], HAND_DIR[s], HAND_UP[s], s, CURLS[s], SPREAD[s],
                                     rest_turn=lambda name, knuckle, s=s:
                                     code_finger_turn(name, knuckle, WRIST[s], s, IDLE_CURL, IDLE_SPLAY).T)[1]
        low = reduce(union(pieces + hand, 0.0024, smooth=3), TRIANGLES['arm'])
        part = Part(f'Arm_{sfx}', low, SKIN)
        V = low[0]
        u = ((V - E) @ dv) / np.linalg.norm(W - E)
        hand_share = smoothstep(0.88, 1.04, u)
        best = np.full(len(V), np.inf)
        owner = np.full(len(V), -1)
        along = np.zeros(len(V))
        for k, (name, pts, radius) in enumerate(lines):
            d, t = polyline_distance(V, pts)
            better = (d < radius + 0.007) & (d < best)
            best[better], owner[better], along[better] = d[better], k, t[better]
        finger_share = np.where(owner >= 0, smoothstep(0.06, 0.2, along), 0.0)
        part.weigh(f'lowerarm_{sfx}', 1.0 - hand_share)
        part.weigh(f'hand_{sfx}', hand_share * (1.0 - finger_share))
        for k, (name, pts, radius) in enumerate(lines):
            part.weigh(finger_bone(name, sfx), hand_share * finger_share * (owner == k))
        parts.append(part)
        fingers[s] = lines
    return parts, fingers, old_fingers


def build_head():
    """The head (with its slit mouth, sunk eyes, ears, mouth and teeth), the neck, and the hair under the hat brim."""
    parts = []
    pivot = hunched((0.0, -0.06, 1.615))
    center = pivot + np.array([0.0, -0.012, 0.09])      # the hunch turned the head level: no rotation left
    spec = HEAD_D
    zm = spec['mouth']
    (V, F), cav, jw = head_surface(spec)
    jw[-1] = 0.5                                        # the pole under the chin, hidden in the neck
    V = jaw_turn(V, jw, JAW_BIND)
    low = reduce((V, F), TRIANGLES['head'])
    near = nearest(V, low[0])
    head = Part('Head', (low[0] + center, low[1]), SKIN, cavity=cav[near])
    parts.append(head.weigh('head', 1.0 - jw[near]).weigh('jaw', jw[near]))
    for s in SIDES:
        ear = ellipsoid((s * (spec['width'] / 2.0 - 0.003), 0.014, -0.006), (0.008, 0.017, 0.027), segs=8, rings=5)
        parts.append(Part('Ear', transform(ear, None, center), SKIN).weigh('head', 1.0))
    # The mouth behind the lips (dark), stretching with the jaw.
    sock = ellipsoid((0.0, -0.035, zm - 0.008), (0.03, 0.038, 0.024), segs=10, rings=6)
    sw = smoothstep(zm + 0.006, zm - 0.012, sock[0][:, 2])
    parts.append(Part('Mouth', (jaw_turn(sock[0], sw, JAW_BIND) + center, sock[1]), SKIN, cavity=1.0)
                 .weigh('head', 1.0 - sw).weigh('jaw', sw))
    # Teeth: eight above on the head, eight below on the jaw.
    for k in range(8):
        a = (k - 3.5) * 0.17
        for upper in (True, False):
            if upper:
                base = np.array([spec['width'] * 0.36 * math.sin(a), -spec['front'] * 0.86 * math.cos(a), zm + 0.009])
                tip_dir = np.array([0.0, -0.15, -1.0])
            else:
                base = np.array([spec['width'] * 0.33 * math.sin(a), -spec['front'] * 0.8 * math.cos(a), zm - 0.011])
                tip_dir = np.array([0.0, -0.15, 1.0])
            tooth = tooth_piece(base, unit(tip_dir), 0.011 if upper else 0.009, 0.0042)
            V = tooth[0]
            if not upper:
                V = jaw_turn(V, np.ones(len(V)), JAW_BIND)
            part = Part('Tooth', (V + center, tooth[1]), SKIN, cavity=0.15)
            parts.append(part.weigh('head' if upper else 'jaw', 1.0))
    # The neck: thin, the cords standing in it, the Adam's apple; up into the head from inside the collar.
    neck_pts = np.vstack([hunch(np.array([[0.0, -0.03, 1.49], [0.0, -0.048, 1.56]])), [center + (0.0, 0.018, -0.07)]])
    C = spline(neck_pts, 9)

    def rfn(s, a):
        cords = 0.1 * (gauss(wrap(a - 0.95), 0.25) + gauss(wrap(a + 0.95), 0.25)) * smoothstep(0.1, 0.4, s)
        apple = 0.2 * gauss(wrap(a), 0.35) * gauss(s - 0.55, 0.12)
        return np.interp(s, [0.0, 0.5, 1.0], [0.037, 0.034, 0.033]) * (1.0 + cords + apple)
    neck = tube_fn(C, rfn, segs=12, up=(0.0, -1.0, 0.0))
    s_neck = polyline_distance(neck[0], C)[1]
    part = Part('Neck', neck, SKIN)
    part.weigh('spine_02', 0.6 * smoothstep(0.25, 0.0, s_neck))
    part.weigh('head', smoothstep(0.6, 1.0, s_neck))
    part.weigh('neck', 1.0 - 0.6 * smoothstep(0.25, 0.0, s_neck) - smoothstep(0.6, 1.0, s_neck))
    parts.append(part)
    # Thin grey hair under the brim: uneven locks, some lifting off the collar.
    rng = np.random.default_rng(17)
    for k in range(16):
        a = math.pi * rng.uniform(0.4, 1.0) * (1 if k % 2 else -1)
        root = np.array([0.07 * math.sin(a) * 1.02, -0.09 * math.cos(a) * 1.08 + 0.006, rng.uniform(0.03, 0.06)])
        out_dir = np.array([math.sin(a), -math.cos(a), 0.0])
        drop = rng.uniform(0.05, 0.15)
        flick = rng.uniform(-0.004, 0.016)
        side = np.array([math.cos(a), math.sin(a), 0.0]) * rng.uniform(-0.012, 0.012)
        path = [root, root + out_dir * 0.01 + (0.0, 0.0, -drop * 0.35),
                root + out_dir * 0.014 + side * 0.5 + (0.0, 0.006, -drop * 0.7),
                root + out_dir * (0.014 + flick) + side + (0.0, 0.012, -drop)]
        w0 = rng.uniform(0.01, 0.02)
        lock = ribbon([center + p for p in path], [(0.0, w0), (0.6, w0 * 0.7), (1.0, 0.002)], n=6, cols=3,
                      out=lambda C, c0=center: unit(C - c0), curl=0.35, twist=0.25 * rng.uniform(-1, 1))
        parts.append(Part('Hair', lock, SKIN, cavity=0.3).weigh('head', 1.0))
    hat_pivot = pivot + np.array([0.0, 0.0, 0.148])
    return parts, dict(pivot=pivot, center=center, hat=hat_pivot)


def tooth_piece(base, direction, length, width):
    """A tooth: a small blunt spike from base toward direction (head space)."""
    d = unit(direction)
    a = unit(np.cross(d, [1.0, 0.0, 0.0] if abs(d[0]) < 0.9 else [0.0, 1.0, 0.0]))
    b = np.cross(d, a)
    ring = [base + (a * math.cos(t) + b * math.sin(t)) * width * (1.0 if k % 2 == 0 else 0.75)
            for k, t in enumerate(np.linspace(0.0, 2.0 * np.pi, 4, endpoint=False))]
    V = np.array(ring + [base + d * length, base - d * 0.002])
    F = [np.array([[0, 1, 4], [1, 2, 4], [2, 3, 4], [3, 0, 4], [1, 0, 5], [2, 1, 5], [3, 2, 5], [0, 3, 5]])]
    return orient((V, F))


def build_shroud():
    piece, t, fade, q, side, uv = shroud()
    part = Part('Shroud', piece, SKIN, fade=fade, strip_uv=uv)
    # Each tail bone is whole at the middle of its span, shared with its neighbours between.
    mids = [(a + b) / 2 for a, b in zip(TAIL_STOPS[:-1], TAIL_STOPS[1:])]
    main = chain(t, mids, ('tail_01', 'tail_02', 'tail_03', 'tail_04', 'tail_05'))
    top = smoothstep(0.06, 0.0, t)          # where the shroud starts, inside the shirt, it moves with the pelvis
    side_share = 0.6 * smoothstep(0.1, 0.6, q) * (side != 0)
    for bone, w in main.items():
        part.weigh(bone, w * (1.0 - top) * (1.0 - side_share))
    part.weigh('pelvis', top)
    for s in SIDES:
        sfx = SUFFIX[s]
        on_side = side_share * (side == s) * (1.0 - top)
        sw = chain(t, (0.46, 0.78), (f'tail_{sfx}_01', f'tail_{sfx}_02'))
        for bone, w in sw.items():
            part.weigh(bone, w * on_side)
    C = spline(SHROUD_PATH, 300)
    sides = {}
    for s in SIDES:
        sel = side == s
        pts = []
        for t0 in (0.3, 0.62, 0.95):
            near = sel & (np.abs(t - t0) < 0.08)
            pts.append(piece[0][near].mean(0) if near.any() else sample(C, t0))
        sides[s] = pts
    return [part], dict(path=C, sides=sides)


# --- The rig ---

def bone_specs(torso, arms, head, tail):
    """(name, head, tail, parent) for every bone, Blender meters, from where the parts were built; the arms, hands,
    fingers and jaw as the older rest had them (bake_idle then turns them into this rest)."""
    specs = [('pelvis', (0.0, 0.005, 1.0), (0.0, 0.0, 1.12), None),
             ('spine_01', (0.0, 0.0, 1.12), hunched((0.0, -0.012, 1.27)), 'pelvis'),
             ('spine_02', hunched((0.0, -0.012, 1.27)), hunched((0.0, -0.04, 1.48)), 'spine_01')]
    center, normal = torso['coal']
    specs.append(('coal', center, center + normal * 0.04, 'spine_02'))
    specs.append(('neck', hunched((0.0, -0.042, 1.5)), head['pivot'], 'spine_02'))
    specs.append(('head', head['pivot'], head['pivot'] + np.array([0.0, 0.0, 0.17]), 'neck'))
    specs.append(('jaw', head['center'] + HINGE, head['center'] + np.array([0.0, -0.08, -0.11]), 'head'))
    specs.append(('hat', head['hat'], head['hat'] + np.array([0.0, 0.0, 0.1]), 'head'))
    for s in SIDES:
        sfx = SUFFIX[s]
        S, E, W = torso['shoulders'][s], ELBOW[s], WRIST[s]
        specs.append((f'upperarm_{sfx}', S, E, 'spine_02'))
        specs.append((f'lowerarm_{sfx}', E, W, f'upperarm_{sfx}'))
        specs.append((f'hand_{sfx}', W, W + HAND_DIR[s] * 0.08, f'lowerarm_{sfx}'))
        for name, pts, radius in arms[s]:
            specs.append((finger_bone(name, sfx), pts[0], pts[1], f'hand_{sfx}'))
    C = tail['path']
    prev = 'pelvis'
    for k in range(5):
        a, b = sample(C, TAIL_STOPS[k]), sample(C, TAIL_STOPS[k + 1])
        specs.append((f'tail_{k + 1:02d}', a, b, prev))
        prev = f'tail_{k + 1:02d}'
    for s in SIDES:
        sfx = SUFFIX[s]
        p = tail['sides'][s]
        specs.append((f'tail_{sfx}_01', p[0], p[1], 'tail_02'))
        specs.append((f'tail_{sfx}_02', p[1], p[2], f'tail_{sfx}_01'))
    return [(name, tuple(map(float, h)), tuple(map(float, t)), parent) for name, h, t, parent in specs]


def materials():
    """Ghost_A and GhostCoal: M_Ghost's slots (their properties are the material instances' parameters; the importer
    resets each instance and sets only these). The values give the D concept's look through M_Ghost's own formulas
    (Tools/Unreal/build_creature_materials.py), where the code's rank strength (a Basic coal's 1.8) multiplies the
    rank color first; the skin, the shirt, the rim and the glow as tried in the game's daylight and approved. NoiseSpeed
    is left to the master: the concept didn't move."""
    ghost = bpy.data.materials.get('Ghost_A') or bpy.data.materials.new('Ghost_A')
    ghost.use_nodes = True
    bsdf = next(n for n in ghost.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(int(ZONE_COLORS[0][1:], 16))    # the skin, in Blender's view
    for key in [k for k in ghost.keys() if not k.startswith('_')]:
        del ghost[key]
    ghost['Master'] = 'Ghost'
    ghost['TextureSet'] = 'Polymer'
    for k, color in enumerate(ZONE_COLORS):
        ghost[f'Zone{k + 1}Color'] = color
    ghost['RimColor'] = RIM_COLOR
    ghost['RimPower'] = 4.0          # a thinner rim than the master's 2.8, (1 - facing) ^ 4, so thin limbs keep their
                                     # shading in daylight instead of going flat white
    ghost['RimStrength'] = 2.2       # raised with the thinner rim, so the edge still reads
    ghost['GlowStrength'] = 0.12     # the body's own faint glow (its base color times this), raised so the darker skin
                                     # keeps the dusk look
    ghost['EmberStrength'] = 2.0     # the ember edge: the concept's 3.6 times the rank color, over the code's 1.8
    ghost['NoiseScale'] = 1.4        # the fade's noise at 1.4 a meter (UVs are in meters), as the concept's
    ghost['FadeSoftness'] = 0.07     # the concept's steepness, 14
    ghost['UVScale'] = 3.0           # the concept's Polymer grain: three times a meter
    coal = bpy.data.materials.get('GhostCoal') or bpy.data.materials.new('GhostCoal')
    coal.use_nodes = True
    bsdf = next(n for n in coal.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x1d110c)
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(0xb02a18)
    bsdf.inputs['Emission Strength'].default_value = 6.0
    for key in [k for k in coal.keys() if not k.startswith('_')]:
        del coal[key]
    coal['Master'] = 'Ghost'
    coal['Coal'] = 1.0               # the crust and roughness are the master's; its rim and glow strengths don't apply
    coal['EmberStrength'] = 2.1      # its cracks: the concept's 6.0 over the code's 1.8 and the master's 1.6
    coal['FadeSoftness'] = 0.07      # dissolves on a phase-step as the body does
    return ghost, coal


def assemble(parts):
    """All parts as one mesh's arrays: vertices, faces, per-vertex color (zone, cavity, ember, fade), per-face slot,
    and weights per bone (up to four a vertex, normalized)."""
    Vs, Fs, cols, slots, bones = [], [], [], [], set()
    n = 0
    for p in parts:
        Vs.append(p.V)
        for F in p.F:
            Fs.append(F + n)
            slots.append(np.full(len(F), p.slot))
        cols.append(np.column_stack([np.full(len(p.V), p.zone / 3.0), np.clip(p.cavity, 0, 1), np.clip(p.ember, 0, 1),
                                     np.clip(p.fade, 0, 1)]))
        bones |= set(p.weights)
        n += len(p.V)
    V = np.concatenate(Vs)
    names = sorted(bones)
    W = np.zeros((len(V), len(names)))
    n = 0
    for p in parts:
        for bone, w in p.weights.items():
            W[n:n + len(p.V), names.index(bone)] += np.clip(w, 0.0, None)
        n += len(p.V)
    # Up to four bones a vertex, normalized.
    order = np.argsort(-W, axis=1)
    keep = np.zeros_like(W, dtype=bool)
    np.put_along_axis(keep, order[:, :4], True, axis=1)
    W = np.where(keep, W, 0.0)
    W[W < 0.01] = 0.0
    W = W / np.maximum(W.sum(1, keepdims=True), 1e-9)
    return V, Fs, np.concatenate(cols), np.concatenate(slots), names, W


def bake_idle(rig, fingers):
    """Turns the skeleton from the older rest into this one the way AUnpaidCreature turned it at idle: each finger about
    its knuckle by the code's idle turn (Curl 25, Splay 4), then each forearm with its hand and fingers about the elbow
    by PitchBy(-12), and the jaw about its hinge by PitchBy(6). Whole bone matrices turn, rolls too, so the code's poses
    over this rest (less those amounts) put every bone exactly where its poses over the older rest did. Only the wrists
    and knuckles move, with their forearms; then they're checked against the parts, which were built in this pose."""
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = rig.data.edit_bones

    def turn(bone, pivot, R):
        pivot = Vector(pivot)
        bone.matrix = Matrix.Translation(pivot) @ Matrix(R.tolist()).to_4x4() @ Matrix.Translation(-pivot) @ bone.matrix

    for s in SIDES:
        sfx = SUFFIX[s]
        wrist = np.array(edit[f'hand_{sfx}'].head)
        for finger in FINGERS:
            bone = edit[finger_bone(finger, sfx)]
            turn(bone, bone.head.copy(), code_finger_turn(finger, bone.head, wrist, s, IDLE_CURL, IDLE_SPLAY))
        elbow = edit[f'lowerarm_{sfx}'].head.copy()
        for name in [f'lowerarm_{sfx}', f'hand_{sfx}'] + [finger_bone(f, sfx) for f in FINGERS]:
            turn(edit[name], elbow, turn_matrix((1.0, 0.0, 0.0), -IDLE_BEND))
    turn(edit['jaw'], edit['jaw'].head.copy(), turn_matrix((1.0, 0.0, 0.0), IDLE_JAW))
    bpy.ops.object.mode_set(mode='OBJECT')
    worst = 0.0
    for s in SIDES:
        sfx = SUFFIX[s]
        E, W, fwd, up = bent(s)
        bones = rig.data.bones
        for point, want in ((bones[f'lowerarm_{sfx}'].tail_local, W), (bones[f'hand_{sfx}'].head_local, W),
                            (bones[f'hand_{sfx}'].tail_local, W + fwd * 0.08)):
            worst = max(worst, float(np.linalg.norm(np.array(point) - want)))
        for name, pts, radius in fingers[s]:
            bone = bones[finger_bone(name, sfx)]
            worst = max(worst, float(np.linalg.norm(np.array(bone.head_local) - pts[0])),
                        float(np.linalg.norm(np.array(bone.tail_local) - pts[1])))
    if worst > 1e-5:
        raise RuntimeError(f'the idle skeleton is {1000.0 * worst:.3f} mm off the parts built in it')
    log(f'rest pose: the idle baked in (fingers Curl {IDLE_CURL:g} Splay {IDLE_SPLAY:g}, elbows {IDLE_BEND:g}, jaw '
        f'{IDLE_JAW:g} more); its wrists and knuckles sit on the parts to {1000.0 * worst:.4f} mm')


def strip_uvs(mesh, parts):
    """The parts with UVs of their own (the shroud) take them over the box projection: U the meters round the shroud,
    V as given. A face across the seam at the back takes its corners round the same way."""
    data = np.full((len(mesh.vertices), 3), np.nan)
    first = 0
    for p in parts:
        if p.strip_uv is not None:
            data[first:first + len(p.V)] = p.strip_uv
        first += len(p.V)
    loops = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loops)
    layer = mesh.uv_layers.active.data
    coords = np.empty(len(mesh.loops) * 2)
    layer.foreach_get('uv', coords)
    coords = coords.reshape(-1, 2)
    for poly in mesh.polygons:
        at = slice(poly.loop_start, poly.loop_start + poly.loop_total)
        here = data[loops[at]]
        if np.isnan(here).any():
            continue
        angle = here[:, 0]
        if angle.max() - angle.min() > math.pi:
            angle = np.where(angle < 0.0, angle + 2.0 * math.pi, angle)
        coords[at] = np.column_stack([angle * here[:, 1], here[:, 2]])
    layer.foreach_set('uv', coords.ravel())


def build_rig():
    torso_parts, torso = build_torso()
    arm_parts, fingers, old_fingers = build_arms()
    head_parts, head = build_head()
    shroud_parts, tail = build_shroud()
    parts = torso_parts + arm_parts + head_parts + shroud_parts
    for p in parts:
        if not p.weights:
            raise RuntimeError(f'{p.name} has no weights')
    rig = lm.armature()
    lm.bones(rig, bone_specs(torso, old_fingers, head, tail))
    bake_idle(rig, fingers)
    V, faces, colors, slots, names, W = assemble(parts)
    mesh = to_mesh('Unpaid', V, faces)
    ghost, coal = materials()
    mesh.materials.append(ghost)
    mesh.materials.append(coal)
    mesh.polygons.foreach_set('material_index', slots.astype(np.int32))      # faces are in the parts' order
    mesh.validate()
    if len(mesh.vertices) != len(V):
        raise RuntimeError('validate() changed the vertices')
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(70.0))
    body = bpy.data.objects.new('Unpaid', mesh)
    bpy.context.scene.collection.objects.link(body)
    lt.box_uv(body, 'Polymer')
    strip_uvs(mesh, parts)
    # Vertex colors: bytes holding exactly these values (the rig's FBX writes colors as sRGB, unconverted).
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    loops = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loops)
    col.data.foreach_set('color_srgb', colors[loops].astype(np.float32).ravel())
    mesh.color_attributes.active_color = col
    mesh.color_attributes.render_color_index = 0
    for b, name in enumerate(names):
        group = body.vertex_groups.new(name=name)
        w = W[:, b]
        for value in np.unique(np.round(w[w > 0.0], 3)):
            group.add(np.where(np.abs(np.round(w, 3) - value) < 1e-9)[0].tolist(), float(value), 'REPLACE')
    body.parent = rig
    body.modifiers.new('Armature', 'ARMATURE').object = rig
    hulls = hit_zones(rig, V, faces, W, names)
    rig['LODs'] = '50,25'
    rig['LODScreens'] = '0.3,0.12'
    rig['Nanite'] = 0
    tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
    log(f'SK_Unpaid: {tris} triangles, {len(mesh.vertices)} vertices, {len(rig.data.bones)} bones, {hulls} hit zones')
    totals = {}
    for p in parts:
        totals[p.name] = totals.get(p.name, 0) + p.triangles()
    log('  ' + ', '.join(f'{name} {count}' for name, count in totals.items()))
    return rig, body, head, torso, colors


# Hit zones: what each bone's hull is made of. Fingers fold into their hand; the shroud's tips past tail_02 have none.
HULL_BONES = ('pelvis', 'spine_01', 'spine_02', 'neck', 'head', 'jaw', 'upperarm_l', 'upperarm_r', 'lowerarm_l',
              'lowerarm_r', 'hand_l', 'hand_r', 'tail_01', 'tail_02')
NO_HULL = ('tail_03', 'tail_04', 'tail_05', 'tail_l_01', 'tail_l_02', 'tail_r_01', 'tail_r_02', 'hat')


def hull_owner(bone):
    for finger in ('thumb', 'index', 'middle', 'ring', 'pinky'):
        if bone.startswith(finger + '_'):
            return 'hand_' + bone[-1]
    return bone


COAL_REACH = 0.07     # the coal's hit sphere radius
HITS = {}             # the zones as planes, for checking what they cover: filled by hit_zones()
HAND_HULL_POSES = ((0.0, 0.0), (48.0, 12.0))     # besides the rest (the idle hand), the hand hulls wrap the fingers
                                                 # opened to the older rest's (Curl 0, Splay 0) and the lunge's claw


def curled(rig, P, W, names, sfx, curl, splay):
    """Points P (with their weights W) skinned to one hand's fingers as AUnpaidCreature turns them for this Curl and
    Splay over this rest (code_finger_pose); all else at rest."""
    side = 1.0 if sfx == 'l' else -1.0
    wrist = np.array(rig.data.bones[f'hand_{sfx}'].head_local)
    out = P.copy()
    for finger in FINGERS:
        name = finger_bone(finger, sfx)
        if name in names:
            knuckle = np.array(rig.data.bones[name].head_local)
            R = code_finger_pose(finger, knuckle, wrist, side, curl, splay)
            out += W[:, names.index(name)][:, None] * ((P - knuckle) @ R.T + knuckle - P)
    return out


def convex_planes(obj):
    """A convex hull object's face planes (outward unit normals and offsets) in rest-pose space."""
    M = np.array(obj.matrix_world)
    co = np.array([v.co[:] for v in obj.data.vertices]) @ M[:3, :3].T + M[:3, 3]
    tri = np.array([p.vertices[:3] for p in obj.data.polygons])
    n = np.cross(co[tri[:, 1]] - co[tri[:, 0]], co[tri[:, 2]] - co[tri[:, 0]])
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
    d = (n * co[tri[:, 0]]).sum(1)
    flip = n @ co.mean(0) - d > 0.0       # the hull's middle is inside, so every normal points away from it
    n[flip] *= -1.0
    d[flip] *= -1.0
    return n, d


def face_samples(P, faces, keep, owner):
    """Corners, edge middles and middles of the kept faces, with the bone that carries each face's first corner."""
    pts, who = [], []
    for F, k in zip(faces, keep):
        F = F[k]
        if not len(F):
            continue
        C = P[F]
        for S in (C, (C + np.roll(C, -1, axis=1)) / 2.0, C.mean(1, keepdims=True)):
            pts.append(S.reshape(-1, 3))
            who.append(np.repeat(owner[F[:, 0]], S.shape[1]))
    return np.vstack(pts), np.concatenate(who)


def outside_hits(P, transforms=None, tol=0.002):
    """Which points lie outside every hit zone. transforms maps each zone's bone from rest to pose (None: the rest pose).
    The tolerance allows for Blender's hull, which can drop a point lying a millimeter or so outside it."""
    inside = np.zeros(len(P), dtype=bool)
    for bone, (n, d) in HITS['planes'].items():
        Q = P
        if transforms is not None:
            T = np.linalg.inv(transforms[bone])
            Q = P @ T[:3, :3].T + T[:3, 3]
        inside |= (Q @ n.T - d <= tol).all(1)
    c = HITS['coal']
    if transforms is not None:
        c = transforms['coal'][:3, :3] @ c + transforms['coal'][:3, 3]
    inside |= np.linalg.norm(P - c, axis=1) <= COAL_REACH + tol
    return ~inside


def hit_zones(rig, V, faces, W, names):
    """A convex hull per bone round the faces it carries most, and the coal's 7 cm sphere. A face counts for every bone
    that carries one of its corners most, so the hulls overlap a face's width where two bones meet: round the bones'
    vertices alone they'd leave a band between them that a shot passes through."""
    owner = np.array([hull_owner(names[i]) for i in np.argmax(W, axis=1)])
    made = 0
    HITS['planes'] = {}
    for bone in HULL_BONES:
        mine = np.zeros(len(V), dtype=bool)
        for F in faces:
            mine[F[(owner[F] == bone).any(1)].ravel()] = True
        pts = V[mine]
        if len(pts) < 4:
            raise RuntimeError(f'no vertices for the hit zone on {bone}')
        if bone.startswith('hand_'):
            pts = np.vstack([pts] + [curled(rig, V[mine], W[mine], names, bone[-1], c, s) for c, s in HAND_HULL_POSES])
        log(f'  hull {bone}: {len(pts)} vertices, {np.round(pts.min(0), 2)} .. {np.round(pts.max(0), 2)}')
        cloud = bpy.data.meshes.new('_cloud')
        cloud.from_pydata(pts.tolist(), [], [])
        obj = bpy.data.objects.new('_cloud', cloud)
        hull = lm.hit_hull(rig, bone, [obj])
        bpy.data.objects.remove(obj)
        bpy.data.meshes.remove(cloud)
        HITS['planes'][bone] = convex_planes(hull)
        made += 1
    coal = V[owner == 'coal']
    center = np.array(rig.data.bones['coal'].head_local)
    reach = np.linalg.norm(coal - center, axis=1).max()
    if reach > COAL_REACH:
        raise RuntimeError(f'the coal reaches {reach:.3f} m from its middle, past its 7 cm hit sphere')
    lm.hit_sphere(rig, 'coal', tuple(center), COAL_REACH)
    HITS['coal'] = center
    uncovered = ~np.isin(owner, HULL_BONES + ('coal',))
    others = set(owner[uncovered]) - set(NO_HULL)
    if others:
        raise RuntimeError(f'vertices on {sorted(others)} have no hit zone')
    # Every face with a corner in a zone lies inside one: its corners, edge middles and middle.
    HITS['faces'] = faces
    HITS['keep'] = [(~uncovered[F]).any(1) for F in faces]
    HITS['owner'] = owner
    S, _ = face_samples(V, faces, HITS['keep'], owner)
    gaps = int(outside_hits(S).sum())
    if gaps:
        raise RuntimeError(f'{gaps} of {len(S)} points on faces with a hit zone lie outside every zone')
    log(f'hit zones: {made} hulls and the coal sphere hold every face but the fading strips\' '
        f'({int(uncovered.sum())} vertices left out)')
    return made + 1


def build_hat(head):
    """SM_UnpaidHat: its origin is the 'hat' bone's head; tilted back a little and to one side, as he wears it."""
    felt, band = hat_pieces()
    R = rotation((1, 0, 0), math.radians(-4)) @ rotation((0, 1, 0), math.radians(5))
    felt = transform(felt, R)
    band = transform(band, R)
    V, faces = merge([felt, band])
    zone = np.concatenate([np.full(len(felt[0]), VEST / 3.0), np.full(len(band[0]), ACCENT / 3.0)])
    mesh = to_mesh('UnpaidHat', V, faces)
    mesh.validate()
    mesh.materials.append(bpy.data.materials['Ghost_A'])
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(70.0))
    hat = bpy.data.objects.new('UnpaidHat', mesh)
    bpy.context.scene.collection.objects.link(hat)
    lt.box_uv(hat, 'Polymer')
    # A textured static mesh exports its colors linear: floats holding the values.
    col = mesh.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    loops = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loops)
    rgba = np.column_stack([zone, np.zeros(len(V)), np.zeros(len(V)), np.ones(len(V))])
    col.data.foreach_set('color', rgba[loops].astype(np.float32).ravel())
    hat['Nanite'] = 0
    hat['LODs'] = '50,25'
    hat['LODScreens'] = '0.25,0.1'
    hat['Collision'] = 'None'
    hat.location = tuple(head['hat'])        # where it sits on the bind pose (the exporter takes its origin as pivot)
    log(f'SM_UnpaidHat: {sum(len(p.vertices) - 2 for p in mesh.polygons)} triangles')
    return hat


rig, body, head, torso, raw_colors = build_rig()
hat = build_hat(head)


# --- Previews (Saved/ArtPreviews/RansomsRest/Unpaid): the stage and the ghost look come from the concept script ---

def want_preview():
    argv = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',')]
    return '--preview' in argv or lt.want_preview()


def stage(name):
    c = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(c)
    return c


def strike(c):
    for o in list(c.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.collections.remove(c)


def keep(obj, where):
    for c in list(obj.users_collection):
        c.objects.unlink(obj)
    where.objects.link(obj)
    return obj


def pose(rotations):
    """Bones turned about axes of the rest pose (degrees), each relative to its parent: {bone: [(axis, degrees)]}."""
    for pb in rig.pose.bones:
        pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    for name, turns in rotations.items():
        M = rig.data.bones[name].matrix_local.to_3x3()
        q = Quaternion()
        for axis, degrees in turns:
            q = Quaternion(Vector(axis), math.radians(degrees)) @ q
        rig.pose.bones[name].rotation_quaternion = (M.inverted() @ q.to_matrix() @ M).to_quaternion()
    bpy.context.view_layer.update()
    # The hat rides the 'hat' bone as the game will attach it.
    rest = rig.data.bones['hat'].matrix_local
    hat.matrix_world = rig.matrix_world @ rig.pose.bones['hat'].matrix @ rest.inverted() @ Matrix.Translation(Vector(head['hat']))
    bpy.context.view_layer.update()


def posed_bounds(objects):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    pts = []
    for obj in objects:
        ev = obj.evaluated_get(depsgraph)
        m = ev.to_mesh()
        co = np.empty(len(m.vertices) * 3)
        m.vertices.foreach_get('co', co)
        pts.append(co.reshape(-1, 3) @ np.array(obj.matrix_world.to_3x3()).T + np.array(obj.matrix_world.translation))
        ev.to_mesh_clear()
    pts = np.vstack(pts)
    return pts.min(0), pts.max(0)


def pose_gaps(name):
    """How much of the posed surface the hit zones (rigid on their bones, as in the physics asset) miss, and where."""
    ev = body.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m = ev.to_mesh()
    P = np.empty(len(m.vertices) * 3)
    m.vertices.foreach_get('co', P)
    ev.to_mesh_clear()
    T = {b: np.array(rig.matrix_world @ rig.pose.bones[b].matrix @ rig.data.bones[b].matrix_local.inverted())
         for b in list(HITS['planes']) + ['coal']}
    S, who = face_samples(P.reshape(-1, 3), HITS['faces'], HITS['keep'], HITS['owner'])
    gaps = outside_hits(S, T)
    where = ', '.join(f'{b} {n}' for b, n in Counter(who[gaps].tolist()).most_common(4))
    log(f'{name}: {100.0 * gaps.mean():.1f}% of the surface outside the hit zones' + (f' ({where})' if where else ''))


def hand_pose(curl, splay):
    """The hands as AUnpaidCreature turns them over this rest for these Curl and Splay channels, as pose() takes them."""
    turns = {}
    for s in SIDES:
        wrist = rig.data.bones[f'hand_{SUFFIX[s]}'].head_local
        for finger in FINGERS:
            name = finger_bone(finger, SUFFIX[s])
            R = Matrix(code_finger_pose(finger, rig.data.bones[name].head_local, wrist, s, curl, splay).tolist())
            axis, angle = R.to_quaternion().to_axis_angle()
            turns[name] = [(tuple(axis), math.degrees(angle))]
    return turns


# The previews' poses as offsets from this rest, the idle itself: the code's jaw over its idle, its forearms' bend over
# the idle's (a straight forearm is +IDLE_BEND), its hands' Curl and Splay through code_finger_pose.
IDLE = {}
LUNGE = {'pelvis': [((1, 0, 0), 20)], 'spine_01': [((1, 0, 0), 8)], 'spine_02': [((1, 0, 0), 6)],
         'neck': [((1, 0, 0), -14)], 'head': [((1, 0, 0), -24)], 'jaw': [((1, 0, 0), 27 - IDLE_JAW)],
         'upperarm_l': [((1, 0, 0), -105)], 'lowerarm_l': [((1, 0, 0), -12 + IDLE_BEND)], 'hand_l': [((1, 0, 0), -15)],
         'upperarm_r': [((0, 1, 0), 50), ((1, 0, 0), -70)], 'lowerarm_r': [((1, 0, 0), -25 + IDLE_BEND)],
         'tail_01': [((1, 0, 0), 12)], 'tail_02': [((1, 0, 0), 6)], 'tail_l_01': [((1, 0, 0), 4)],
         'tail_r_01': [((1, 0, 0), 4)], **hand_pose(48.0, 12.0)}
SHRIEK = {'spine_02': [((1, 0, 0), -6)], 'neck': [((1, 0, 0), -14)], 'head': [((1, 0, 0), -30)],
          'jaw': [((1, 0, 0), 36 - IDLE_JAW)],
          'upperarm_l': [((0, 1, 0), -48), ((1, 0, 0), -30)], 'lowerarm_l': [((1, 0, 0), -35 + IDLE_BEND)],
          'upperarm_r': [((0, 1, 0), 48), ((1, 0, 0), -30)], 'lowerarm_r': [((1, 0, 0), -35 + IDLE_BEND)],
          'tail_01': [((1, 0, 0), 8)], 'tail_02': [((0, 0, 1), 8)], **hand_pose(-5.0, 18.0)}
TRAIL = {'pelvis': [((1, 0, 0), 10)], 'tail_01': [((1, 0, 0), 22)], 'tail_02': [((1, 0, 0), 16), ((0, 0, 1), 14)],
         'tail_03': [((1, 0, 0), 8), ((0, 0, 1), -18)], 'tail_04': [((0, 0, 1), -16)], 'tail_05': [((0, 0, 1), 14)],
         'tail_l_01': [((0, 0, 1), 18)], 'tail_l_02': [((0, 0, 1), -12)], 'tail_r_01': [((0, 0, 1), -16)],
         'tail_r_02': [((0, 0, 1), 14)], 'head': [((1, 0, 0), 6)],
         'lowerarm_l': [((1, 0, 0), IDLE_BEND)], 'lowerarm_r': [((1, 0, 0), IDLE_BEND)]}


def previews():
    sys.path.insert(0, os.path.join(lt.REPO, 'Art', 'Backlog', 'Creatures'))
    import UnpaidConcepts as uc
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(WORK, exist_ok=True)
    hulls = [o for o in rig.children if o.name.startswith(('UCX_', 'USP_'))]
    for o in hulls:
        o.hide_render = True
    # The ghost look: the concept's preview material reads a float copy of the vertex colors.
    for obj, rgba in ((body, raw_colors), (hat, None)):
        attr = obj.data.color_attributes.new('Ghost', 'FLOAT_COLOR', 'POINT')
        if rgba is None:
            src = obj.data.color_attributes['Col']
            loops = np.empty(len(obj.data.loops), dtype=np.int64)
            obj.data.loops.foreach_get('vertex_index', loops)
            per_loop = np.empty(len(obj.data.loops) * 4, np.float32)
            src.data.foreach_get('color', per_loop)
            rgba = np.zeros((len(obj.data.vertices), 4), np.float32)
            rgba[loops] = per_loop.reshape(-1, 4)
        attr.data.foreach_set('color', np.asarray(rgba, np.float32).ravel())
    ghost = uc.ghost_material(tuple(int(c[1:], 16) for c in ZONE_COLORS), uc.RANKS[0][1:])    # the slot's zone colors
    body.data.materials[0] = ghost
    body.data.materials[1] = uc.coal_material(uc.RANKS[0][1:])
    hat.data.materials[0] = ghost

    def outdoors(where, seed=5):
        uc.ground(where)
        uc.grass(where, seed=seed)
        uc.headboard(where, (-1.5, 2.9, 0.0), turn=0.25, tilt=0.07)
        uc.headboard(where, (1.6, 3.6, 0.0), turn=-0.2, tilt=-0.05, height=0.7)
        uc.sky()
        uc.sun((-0.62, -0.72, 0.34), where=where)

    def shoot(where, direction, path, resolution, lens=50.0, distance=None, lift=0.05, fit=2.3):
        lo, hi = posed_bounds([body, hat])
        target = Vector(((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2 - 0.1, (lo[2] + hi[2]) / 2 + lift))
        size = max(hi - lo)
        dist = distance or size * fit * 50.0 / lens + 0.6
        keep(uc.camera(target + Vector(direction).normalized() * dist, target, lens=lens), where)
        uc.render(path, resolution)

    # At rest, which is the idle, in the boot hill light of the concepts; and the same view beside the concept.
    pose(IDLE)
    where = stage('Rest')
    outdoors(where)
    shoot(where, (0.78, -1.0, 0.13), os.path.join(OUT, 'Unpaid_rest.png'), (1600, 1000), distance=4.9)
    strike(where)
    concept = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'Unpaid', 'Unpaid_D_hero.png')
    if os.path.exists(concept):
        uc.compose([concept, os.path.join(OUT, 'Unpaid_rest.png')], os.path.join(OUT, 'Unpaid_vs_concept.png'))

    # Posed by its bones: the lunge, the shriek, the shroud trailing.
    panels = []
    for name, rotations, direction in (('lunge', LUNGE, (0.95, -0.75, 0.14)), ('shriek', SHRIEK, (0.35, -1.0, 0.1)),
                                       ('trail', TRAIL, (1.0, -0.35, 0.12))):
        pose(rotations)
        pose_gaps(name)
        where = stage('Pose')
        uc.ground(where, 0xbdb4a4, plain=True)
        uc.sky(0.8)
        uc.sun((-0.45, -0.85, 0.42), strength=3.8, where=where)
        path = os.path.join(WORK, f'Unpaid_pose_{name}.png')
        shoot(where, direction, path, (900, 900), lens=50.0, fit=1.7)
        panels.append(path)
        strike(where)
    uc.compose(panels, os.path.join(OUT, 'Unpaid_poses.png'))
    pose({})

    # The LODs the importer will make (50% and 25%), reduced here the same way, with their counts.
    where = stage('LODs')
    uc.ground(where, 0xbdb4a4, plain=True)
    uc.sky(0.8)
    uc.sun((-0.45, -0.85, 0.42), strength=3.8, where=where)
    copies = []
    depsgraph = bpy.context.evaluated_depsgraph_get()
    base = sum(len(p.vertices) - 2 for p in body.data.polygons)
    for k, share in enumerate((1.0, 0.5, 0.25)):
        copy = body.copy()
        where.objects.link(copy)
        if share < 1.0:
            mod = copy.modifiers.new('LOD', 'DECIMATE')
            mod.decimate_type = 'COLLAPSE'
            mod.ratio = share
        copy.location.x += (k - 1) * 1.1
        copies.append(copy)
    body.hide_render = hat.hide_render = True
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    for k, copy in enumerate(copies):
        m = copy.evaluated_get(depsgraph).to_mesh()
        count = sum(len(p.vertices) - 2 for p in m.polygons)
        copy.evaluated_get(depsgraph).to_mesh_clear()
        uc.label(f'LOD{k}  {count} triangles', ((k - 1) * 1.1, -0.9, 0.05), 0.075, where=where)
        log(f'LOD{k}: {count} triangles' + (' (LOD0)' if k == 0 else f' ({100 * count / base:.0f}%)'))
    keep(uc.camera((0.0, -20.0, 1.05 + 20.0 * math.tan(math.radians(8.0))), (0.0, 0.0, 1.05), ortho=3.9), where)
    uc.render(os.path.join(OUT, 'Unpaid_lods.png'), (1800, 1100), samples=48)
    strike(where)
    body.hide_render = hat.hide_render = False

    # The hit zones over the mesh: a hull per bone and the coal's sphere, front and side.
    where = stage('Hits')
    uc.ground(where, 0xbdb4a4, plain=True)
    uc.sky(0.8)
    uc.sun((-0.45, -0.85, 0.42), strength=3.8, where=where)
    matte = uc.flat_material('HitMatte', 0x8e8a84, 0.9)
    saved = list(body.data.materials)
    body.data.materials[0] = matte
    body.data.materials[1] = matte
    hat.hide_render = True
    palette = (0xe6553a, 0xf0a030, 0xd6d040, 0x6cc070, 0x40b8c8, 0x5a7ee0, 0xa060d8, 0xe060a0)
    for k, o in enumerate(sorted(hulls, key=lambda h: h.name)):
        o.hide_render = False
        wire = o.modifiers.new('Wire', 'WIREFRAME')
        wire.thickness = 0.004
        wire.use_even_offset = False      # even thickness spikes at the hulls' sharp corners
        color = 0xff2a10 if o.name.startswith('USP_') else palette[k % len(palette)]
        o.data.materials.clear()
        o.data.materials.append(uc.flat_material(f'Hit{color:06x}', color, 0.5, 2.0))
    shots = []
    for name, direction in (('front', (0.0, -1.0, 0.08)), ('side', (1.0, 0.0, 0.08))):
        path = os.path.join(WORK, f'Unpaid_hits_{name}.png')
        lo, hi = posed_bounds([body])
        target = Vector(((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2))
        cam = keep(uc.camera(target + Vector(direction).normalized() * 20.0, target, ortho=2.25), where)
        uc.render(path, (900, 1000), samples=32)
        bpy.data.objects.remove(cam)
        shots.append(path)
    uc.compose(shots, os.path.join(OUT, 'Unpaid_hitzones.png'))
    strike(where)
    for o in hulls:
        o.hide_render = True
    for k, m in enumerate(saved):
        body.data.materials[k] = m
    hat.hide_render = False


if want_preview():
    previews()
