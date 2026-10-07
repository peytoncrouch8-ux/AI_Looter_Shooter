"""Amos Whitlock, the friendly ghost of Side 2, "Unfinished Business" (Docs/Areas/RansomsRest.md: the Side 2 mission, the
NPC table, the art needs; Docs/Story.md), as the user picked him on 2026-10-07: option A of
Art/Backlog/Creatures/AmosConcepts.py, "the hayman". A farmer of about fifty who died in the middle of the harvest: a wide
straw hat with a ragged, unbound brim and a stalk in its band; a collarless linen shirt with the sleeves rolled past the
elbows and chaff stuck to it; one brace up over his right shoulder, the left slipped off for the heat and hanging in a
loop by his hip; brown duck trousers patched with flour sack; his hay fork; a straw at the corner of his mouth. A
sun-creased face with a few days' stubble, sun-bleached hair, the back of his neck leathered by the sun. Tired and
patient, never the Unpaid's hunger. His trousers end at the knees in ragged hems, and below them the Unpaid's pale
shroud gathers and sweeps back off him in strips: no legs. His coal is banked low: small, dull, under ash.

A rig the game poses in code, on the Unpaid's bones (Art/Models/Creatures/Unpaid.py's names and hierarchy, as
Art/Models/Creatures/Abel.py built them), at size 1.

Models:
  SK_Amos       about 13k triangles, LODs at 50% and 25% (no Nanite), skinned smoothly (up to four bones a vertex).
  SM_AmosHat    his straw hat. Its pivot is the 'hat' bone's head. It carries SOCKET_Speaker at his mouth (facing his
                front): the exporter gives a skeletal mesh no sockets, and the hat rides his head, so his speaker point
                attaches here.
  SM_AmosFork   his hay fork, in his right fist in the rest pose. Its pivot is the 'fork' bone's head, where the fist
                holds the shaft (0.95 m above the butt). In the lean it is set down against the fence post: the pose
                table places the bone there, and a placed copy of the same mesh can stand there instead. In the sit
                the table places it too: it stands on the ground, his fist round it higher up the shaft.
Each prop is modeled where it is in the rest pose and has no collision: attach it to its bone as the Unpaid's hat is
attached (at the bone, turned by the inverse of the bone's rest rotation), and it rides the bone from there.

Bones (as AUnpaidCreature names them; the armature is Unreal's root). The bind pose is the idle (facing -Y, Unreal's +X;
the origin on the ground; upright; the fork upright in his right fist, its butt on the ground; the left arm hanging
loose; the head level, the jaw closed; the shroud down and back):
  pelvis                         the float height (1.0 m); parent of the spine, the thighs and the shroud
  spine_01, spine_02             spine_02 is the chest: it carries the arms, the neck and the coal
  coal                           the coal, rigid; its head is the coal's middle (the crit point, if he ever turns)
  neck, head, jaw, hat           jaw: the lower lip and chin, opened by turning it down about its head; hat: no skin,
                                 the hat's attach point
  upperarm_l, lowerarm_l, hand_l and its fingers thumb_01_l, index_01_l, middle_01_l, ring_01_l, pinky_01_l (one bone a
                                 finger, at the knuckle); the same with _r
  fork                           (new) under hand_r: the fork's attach point where the fist holds the shaft, pointing up
                                 the shaft
  skirt_f_01, skirt_f_02         (Abel's front chain) under the pelvis: the thighs from the hips to the knees, then the
                                 knees and the shroud's top. Swung forward they make his lap, so he sits on the rail.
  tail_01 .. tail_05             the shroud, down its middle from the knees (tail_01's head is at the knees, under the
                                 pelvis: in the lean and the sit the pose table shifts it to where skirt_f_02 carries
                                 the knees)
  tail_l_01, tail_l_02, tail_r_01, tail_r_02    side chains off tail_02 for the shroud's side strips
The pose table (written to Intermediate/AmosModel/Amos_poses.json; logged as POSE lines with -ScriptArgs POSE) gives,
for the idle, the
lean on the fence, the sit on the fence and the talk, where each bone's head goes and its turn from rest, in Unreal's
component space (cm at size 1; x forward, y right, z up), both as the whole turn and as the bone's own turn under its
parent's (AUnpaidCreature's Turned = Above * Own).

Vertex colors ('Col', what M_Ghost reads): R the tint zone (0 skin and shroud, 1/3 shirt, his hair and the flour-sack
patch, 2/3 trousers, 1 the braces), G cavity, B the ember edge round the coal (narrow and weak: banked), A the fade (as
the Unpaid's shroud: whole at the hems, fading down the tube and the strips to nothing at their tips; up inside the trouser
legs, where the tube runs on to a cap, it fades in from the cap). The rig's FBX carries them as sRGB, so the body
stores bytes; the props (textured static meshes, exported linear) store floats. UV 0: a world-scale box projection,
1 unit a meter, but on the shroud U the meters round it and V minus the meters down it.

Materials (M_Ghost's slots; the importer resets each instance and sets these):
  GhostAmos      the body: his four zone colors and the Unpaid's approved rim, glow and fade values, a weak ember edge
  GhostCoal      the coal, shared with SK_Unpaid and SK_Abel (the same values). Banked low by the code: see the report
  GhostAmosHat   the hat: TextureSet Hay, so the importer sets M_Ghost's BaseColorMap to T_Hay_BC (the straw's strands)
  GhostAmosFork  the fork's ash and iron

Hit zones (PA_Amos), on the Unpaid's scheme: a convex hull on pelvis, spine_01, spine_02, neck, head, jaw, each upper arm,
forearm and hand, tail_01, tail_02, skirt_f_01 and skirt_f_02, round every face with a corner those bones carry most; a
sphere on coal. The build checks that every face but the shroud strips' lies inside a zone.

    powershell -NoProfile -File <artrun.ps1> -Script Art\\Models\\Creatures\\Amos.py -Preview
    ... -ScriptArgs --preview,lean,face        only some: lean (and compare) sit face turn lods hits poses game
Previews go to Saved/ArtPreviews/RansomsRest/Amos/ (the field, the fence and the light come from the concept script).
"""
import json
import math
import os
import sys
from collections import Counter

import bpy
import numpy as np
from mathutils import Matrix, Vector, kdtree, noise

import looter_model as lm
import looter_textures as lt

SIDES = (1.0, -1.0)                     # +1 the left (+X), -1 the right
SUFFIX = {1.0: 'l', -1.0: 'r'}
SKIN, SHIRT, TROUSERS, BRACES = 0, 1, 2, 3   # tint zones (vertex color R = zone / 3)
# The hayman's tints, as the concept: pale skin and shroud (the Unpaid's approved skin), unbleached linen (his shirt, his
# sun-bleached hair and the flour-sack patch), brown duck trousers, the dark leather of his braces.
ZONE_COLORS = ('#BAC4C6', '#D3C8AD', '#85725C', '#4B3F37')
RIM_COLOR = '#DCECEE'
# The hat: straw (zones 0-1) and its dark band (2-3). M_Ghost reads its grain by brightness (x 1.25, clamped 0.5..1.5),
# and the Hay set's straw averages darker than Polymer's grain, so the straw's zone color is set lighter to land on the
# concept's #D9C891: HAT_GAIN is that ratio, measured on T_Hay_BC and T_Polymer_BC by hat_gain() and logged.
HAT_COLORS = ('#F2E2AA', '#F2E2AA', '#4B3F37', '#4B3F37')
FORK_COLORS = ('#A89777', '#A89777', '#8E959A', '#8E959A')       # the ash shaft; the iron ferrule and tines
EMBER_COLOR = 0x96421f                  # the banked coal's ember, as the code would set RankColor (previews only)
EMBER_ALPHA = 0.35                      # and its strength (RankColor's alpha): a Basic coal's is 1.8
EMBER_HEAT = -0.5                       # and the code's Heat (Burn = 1 + Heat): the coal banked low
JAW_REST = 0.0                          # degrees the jaw is open at rest: the lips closed, the mouth a line
TALK_JAW = 8.0                          # degrees the jaw opens to speak (PitchBy on jaw)
TRIANGLES = dict(shirt=1350, sleeve=260, hand=900, head=4700, trousers=1450)    # the parts reduced to a budget
OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Amos')
WORK = os.path.join(lt.REPO, 'Intermediate', 'AmosModel')
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]

# The fence he leans on and sits on: Art/Models/Props/Fences.py's FenceRail (seed 10), measured at the middle of the
# span between its posts (1.5 m apart), where he stands. The build makes the real fence again (fence_probe), logs what
# it measures and fits the lean and the sit to its surface. The fence's line is the top rail's middle.
RAIL_TOP = 1.008                        # the top rail's top there (m over the ground): where the fits start from
LEAN_BACK = 0.35                        # leaning, his origin stands this far behind the fence's line
POST_X = 0.75                           # the posts either side of him, this far to his left and right


def log(message):
    print(f'AMOS: {message}', flush=True)


def finger_bone(finger, sfx):
    return f'{finger}_01_{sfx}'


FINGERS = ('thumb', 'index', 'middle', 'ring', 'pinky')


# --- Math (as Unpaid.py's) ---

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


def rot_x(degrees):
    """A turn about Blender's +X: an upright bone leans forward, a hanging one swings back (AUnpaidCreature's PitchBy)."""
    return rotation((1.0, 0.0, 0.0), math.radians(degrees))


def rot_y(degrees):
    return rotation((0.0, 1.0, 0.0), math.radians(degrees))


def rot_z(degrees):
    return rotation((0.0, 0.0, 1.0), math.radians(degrees))


def pnoise(points, scale=1.0, seed=0):
    off = Vector((seed * 13.17 + 3.1, seed * 7.31 + 1.7, seed * 5.13 + 4.3))
    return np.array([noise.noise(Vector(p) * scale + off) for p in np.asarray(points, float).reshape(-1, 3)])


def wrap(a):
    return (np.asarray(a, float) + np.pi) % (2.0 * np.pi) - np.pi


def frame(x, y):
    """The rotation whose columns are x, y (made square to x) and x cross y."""
    x = unit(x)
    y = unit(np.asarray(y, float) - x * np.dot(y, x))
    return np.column_stack([x, y, np.cross(x, y)])


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


# --- Mesh pieces: (vertices, [face arrays]) (as Unpaid.py's) ---

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
    """A closed tube along C with a radius per point (ry along the binormal), domed ends."""
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
        return P.reshape(-1, 3), [grid_faces(len(C), segs)]
    V = np.vstack([P.reshape(-1, 3), C[0], C[-1]])
    return orient((V, [grid_faces(len(C), segs), fan(n, np.arange(segs)), fan(n + 1, np.arange(segs) + n - segs, top=True)]))


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


def _evaluated(piece, modifier, keep=None):
    V, faces = piece
    mesh = to_mesh('_eval', V, faces)
    obj = bpy.data.objects.new('_eval', mesh)
    bpy.context.scene.collection.objects.link(obj)
    if keep is not None:
        # Per-vertex weights the decimation spares (1 keeps its detail): Decimate collapses where its group's weight is
        # high, so the group is inverted.
        group = obj.vertex_groups.new(name='keep')
        w = np.round(np.clip(keep, 0.0, 1.0), 2)
        for value in np.unique(w[w > 0.0]):
            group.add(np.where(w == value)[0].tolist(), float(value), 'REPLACE')
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


def reduce(piece, triangles, keep=None, keep_factor=4.0):
    """A piece decimated (edge collapse) to about this many triangles, without slivers; keep (per vertex, 0..1) spares
    detail where it is high."""
    count = sum(len(F) * (F.shape[1] - 2) for F in piece[1])
    if count <= triangles:
        return clean(piece)

    def mods(obj):
        mod = obj.modifiers.new('Decimate', 'DECIMATE')
        mod.decimate_type = 'COLLAPSE'
        mod.ratio = triangles / count
        mod.use_collapse_triangulate = True
        if keep is not None:
            mod.vertex_group = 'keep'
            mod.vertex_group_factor = keep_factor
            mod.invert_vertex_group = True
    return clean(_evaluated(piece, mods, keep))


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


def falloff(V, center, radius):
    d = np.asarray(V, float) - np.asarray(center, float)
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


def chain(x, stops, bones):
    """Weights over a chain of bones by a coordinate x: bones[k] whole at stops[k], shared linearly between."""
    x = np.clip(np.asarray(x, float), stops[0], stops[-1])
    out = {b: np.zeros_like(x) for b in bones}
    k = np.clip(np.searchsorted(stops, x, side='right') - 1, 0, len(stops) - 2)
    f = (x - np.asarray(stops)[k]) / (np.asarray(stops)[k + 1] - np.asarray(stops)[k])
    for i, b in enumerate(bones):
        out[b] += np.where(k == i, 1.0 - f, 0.0) + np.where(k == i - 1, f, 0.0)
    return out



# --- The body: a lean, wiry farmer of about 1.8 m (the concept's measures, the Unpaid before the hunger took him) ---

# (z, half width, half depth, y of the middle, superellipse power)
TORSO = [(0.90, 0.168, 0.112, 0.000, 2.2), (0.98, 0.161, 0.108, 0.000, 2.2), (1.06, 0.151, 0.104, -0.004, 2.2),
         (1.16, 0.157, 0.110, -0.010, 2.3), (1.26, 0.171, 0.119, -0.012, 2.4), (1.34, 0.183, 0.121, -0.008, 2.6),
         (1.40, 0.190, 0.114, -0.002, 2.8), (1.45, 0.188, 0.104, 0.004, 3.2), (1.475, 0.176, 0.096, 0.008, 3.4),
         (1.50, 0.149, 0.088, 0.010, 3.0), (1.52, 0.110, 0.078, 0.010, 2.6), (1.54, 0.075, 0.068, 0.010, 2.2)]
SHOULDER = {s: np.array([s * 0.17, 0.004, 1.43]) for s in SIDES}
PIVOT = np.array([0.0, -0.02, 1.615])                     # the top of the neck, where the head turns
HEAD_CENTER = PIVOT + np.array([0.0, -0.012, 0.09])       # head space's origin: mid-head at eye height
HAT_PIVOT = PIVOT + np.array([0.0, 0.0, 0.153])           # the brim's middle: the 'hat' bone's head
COAL_AT = (0.25, 1.33)                                    # the coal: angle round the chest from the front, height
COAL_RADIUS = 0.038                                       # banked low: smaller than the Unpaid's 5 cm
HOLE_RADIUS = 0.046                                       # the hole it has burnt through his shirt
COAL_REACH = 0.05                                         # its hit sphere
HIPS = {s: np.array([s * 0.09, 0.0, 0.91]) for s in SIDES}        # where each thigh swings from
KNEES = {s: np.array([s * 0.1, -0.03, 0.5]) for s in SIDES}       # the knees' middles at rest
SKIRT_HINGE = np.array([0.0, -0.005, 0.91])               # skirt_f_01's head: the hips, where the lap folds
SKIRT_KNEE = np.array([0.0, -0.03, 0.5])                  # skirt_f_02's head: the knees
SKIRT_SHIN = np.array([0.0, -0.02, 0.36])
# The shroud from the knees down and back (it starts just above the trousers' hems, between and under them).
SHROUD_PATH = [(0.0, -0.035, 0.54), (0.0, -0.01, 0.4), (0.0, 0.09, 0.27), (0.0, 0.3, 0.17), (0.0, 0.6, 0.11),
               (0.0, 0.95, 0.09)]
SHROUD_SIZE = [(0.0, 0.15, 0.072), (0.2, 0.148, 0.075), (0.45, 0.11, 0.07), (0.7, 0.07, 0.048), (1.0, 0.026, 0.024)]
TAIL_STOPS = (0.0, 0.2, 0.4, 0.6, 0.8, 1.0)


def surface(profile, z, theta, grow=0.0):
    """Points round a profile (rows of z, half width, half depth, y middle, power) at heights z and angles theta (from
    the front, toward the left)."""
    z = np.asarray(z, float)
    w, d, yc, p = table(profile, z)
    c, s = np.cos(theta)[None, :], np.sin(theta)[None, :]
    e = (2.0 / p)[:, None]
    cf, sl = np.sign(c) * np.abs(c) ** e, np.sign(s) * np.abs(s) ** e
    x = (w[:, None] + grow) * sl
    y = yc[:, None] - (d[:, None] + grow) * cf
    return np.stack([x, y, np.broadcast_to(z[:, None], x.shape)], axis=-1)


def outward(theta):
    theta = np.asarray(theta, float)
    return np.stack([np.sin(theta), -np.cos(theta), np.zeros_like(theta)], -1)


def coal_center():
    tc, zc = COAL_AT
    pc = surface(TORSO, [zc], np.array([tc]))[0, 0]
    nc = outward(tc)
    return pc - nc * 0.006, nc, pc


# --- The arms in the rest pose ---
# Right: the fork held upright beside his right thigh, its butt on the ground, his fist round the shaft at 0.95 m.
# Left: hanging loose, the elbow a little bent, the fingers relaxed.

HAND_SCALE = 1.1                          # big working hands
GRIP_ALONG, GRIP_UNDER = 0.075 * HAND_SCALE, 0.03 * HAND_SCALE    # where a fist's grip sits from the wrist
UPPER_ARM, FOREARM = 0.29, 0.265          # the arms' lengths (shoulder to elbow, elbow to wrist)
FORK_GRIP = 0.95                          # where on the shaft his fist holds the fork (m up from its butt)
FORK_AT = np.array([-0.255, -0.2])        # where the fork stands at rest (x, y): beside his right thigh, a little ahead
LOOSE = (0.22, 0.32, 0.42, 0.52)
FIST = (1.12, 1.18, 1.18, 1.12)


def grip_of(W, f, u):
    """Where a fist (wrist W, fingers' way f, back of the hand u) holds a rod: inside the curled fingers."""
    return np.asarray(W, float) + unit(f) * GRIP_ALONG - unit(u) * GRIP_UNDER


def wrist_for(G, f, u):
    return np.asarray(G, float) - unit(f) * GRIP_ALONG + unit(u) * GRIP_UNDER


def rod_hand(rod, fingers, side):
    """A fist round a rod running rod (the thumb's way), the fingers pointing fingers' way across it: (f, back of the
    hand). The fist's tunnel (f x u, by side) runs along the rod."""
    r = unit(rod)
    f = unit(np.asarray(fingers, float) - r * np.dot(fingers, r))
    u = np.cross(f, r) * side
    return f, u


def two_bone(S, W, L1, L2, pole):
    """The elbow for a shoulder S and wrist W with these segment lengths, bent toward pole; the wrist pulled in if out of
    reach. Returns (E, W, reached)."""
    d = W - S
    dist = float(np.linalg.norm(d))
    x = d / dist
    reach = min(max(dist, abs(L1 - L2) + 1e-4), L1 + L2 - 1e-4)
    W = S + x * reach
    a = (L1 * L1 - L2 * L2 + reach * reach) / (2.0 * reach)
    h = math.sqrt(max(L1 * L1 - a * a, 0.0))
    y = unit(np.asarray(pole, float) - x * np.dot(pole, x))
    return S + x * a + y * h, W, abs(reach - dist) < 1e-3


def rest_arms():
    """Shoulder, elbow, wrist, the hand's frame and the grip, per side, in the rest pose."""
    arms = {}
    f, u = rod_hand((0.0, 0.0, 1.0), (0.1, -1.0, 0.0), -1.0)
    G = np.array([FORK_AT[0], FORK_AT[1], FORK_GRIP])
    W = wrist_for(G, f, u)
    E, W, reached = two_bone(SHOULDER[-1.0], W, UPPER_ARM, FOREARM, (-0.6, 0.6, -0.2))
    arms[-1.0] = dict(S=SHOULDER[-1.0], E=E, W=W, f=f, u=u, G=grip_of(W, f, u), rod=np.array([0.0, 0.0, 1.0]),
                      curls=FIST, thumb=0.8, spread=0.04)
    f, u = unit(np.array([0.04, -0.22, -1.0])), unit(np.array([1.0, -0.12, 0.05]))
    u = unit(u - f * np.dot(u, f))
    W = np.array([0.212, -0.13, 0.875])
    E, W, reached = two_bone(SHOULDER[1.0], W, UPPER_ARM, FOREARM, (0.35, 0.6, 0.0))
    arms[1.0] = dict(S=SHOULDER[1.0], E=E, W=W, f=f, u=u, G=grip_of(W, f, u), curls=LOOSE, thumb=0.35, spread=0.1)
    return arms


ARMS = rest_arms()


# --- Parts ---

class Part:
    """Geometry with what the material reads per vertex and the weights it is skinned with."""

    def __init__(self, name, piece, zone, slot=0, cavity=0.0, ember=0.0, fade=1.0, strip_uv=None):
        V, faces = piece
        self.name, self.zone, self.slot = name, zone, slot
        self.strip_uv = strip_uv
        self.V = np.asarray(V, float)
        self.F = [np.asarray(F) for F in faces if len(F)]
        n = len(self.V)
        self.zones = np.broadcast_to(np.asarray(zone, float), (n,)).copy()
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


def torso_weights(part, z, x=None, shoulders=True, neck=True, scale=1.0):
    """The spine's share of torso cloth by height (each bone whole at its middle), the neck's only close round the
    neck, and the shoulders' share to the upper arms."""
    stops, bones = ((1.04, 1.2, 1.38, 1.58), ('pelvis', 'spine_01', 'spine_02', 'neck')) if neck else \
        ((1.04, 1.2, 1.38), ('pelvis', 'spine_01', 'spine_02'))
    w = chain(z, stops, bones)
    if x is not None and neck:
        away = smoothstep(0.06, 0.11, np.abs(x))
        w['spine_02'] = w['spine_02'] + w['neck'] * away
        w['neck'] = w['neck'] * (1.0 - away)
    arm = np.zeros(len(z))
    if shoulders and x is not None:
        arm = 0.35 * smoothstep(0.14, 0.2, np.abs(x)) * smoothstep(1.36, 1.44, z)
    for bone, value in w.items():
        part.weigh(bone, value * (1.0 - arm) * scale)
    if shoulders and x is not None:
        for s in SIDES:
            part.weigh(f'upperarm_{SUFFIX[s]}', arm * (np.sign(x) == s) * scale)
    return part


def leg_param(V):
    """For each point, its leg (+1 left, -1 right) and how far down that thigh it lies (0 at the hip, 1 at the knee)."""
    side = np.where(V[:, 0] >= 0.0, 1.0, -1.0)
    H = np.where(side[:, None] > 0, HIPS[1.0], HIPS[-1.0])
    K = np.where(side[:, None] > 0, KNEES[1.0], KNEES[-1.0])
    d = K - H
    t = ((V - H) * d).sum(1) / (d * d).sum(1)
    return side, t


def thigh_weights(part, V, seat=True):
    """Trouser cloth: the waist on the pelvis, the thighs on skirt_f_01 (whole from a hand's width below the hip), the
    knees on skirt_f_02; the seat stays on the pelvis, so it rests where it sits."""
    side, t = leg_param(V)
    z = V[:, 2]
    leg = smoothstep(0.02, 0.28, t) * smoothstep(0.98, 0.9, z)
    if seat:
        leg = leg * (1.0 - smoothstep(0.02, 0.085, V[:, 1]) * smoothstep(0.88, 0.78, z) * smoothstep(0.45, 0.2, t))
    knee = smoothstep(0.84, 0.99, t) * leg
    upper = Part('_', (V, []), 0)
    torso_weights(upper, np.maximum(z, 0.9), V[:, 0], shoulders=False, neck=False)
    for bone, w in upper.weights.items():
        part.weigh(bone, w * (1.0 - leg))
    part.weigh('skirt_f_01', leg - knee)
    part.weigh('skirt_f_02', knee)
    return part


# --- Garments ---

def shell(profile, z_lo, z_hi, sd_fn, folds_fn, grow=0.0, rows=120, cols=200, hole=HOLE_RADIUS):
    """A garment round the body: the profile grown by grow, folded by folds_fn(theta, z), cut where sd_fn > 0 and
    through at the coal (a ragged hole). Returns (V, faces), each vertex's theta and z, and its distance from the hole's
    edge."""
    theta = np.linspace(-np.pi, np.pi, cols, endpoint=False)
    z = np.linspace(z_lo, z_hi, rows)
    th, zz = np.meshgrid(theta, z)
    P = surface(profile, z, theta, grow) + outward(theta)[None, :, :] * folds_fn(th, zz)[..., None]
    sd = sd_fn(th, zz)
    tc, zc = COAL_AT
    ang = np.arctan2(zz - zc, (th - tc) * 0.17)
    rh = hole * (1.0 + 0.14 * np.sin(ang * 5 + 1.0) + 0.09 * np.sin(ang * 11.0))
    gap = np.hypot((th - tc) * 0.17, zz - zc)
    if hole:
        sd = np.maximum(sd, rh - gap)
    V, F, used = cut(P, sd)
    return (V, F), th.ravel()[used], zz.ravel()[used], (gap - rh).ravel()[used]


def build_shirt():
    """The collarless linen shirt: bloused a little over the waistband and tucked in under it, two buttons open at the
    throat (his chest in the V), a breast pocket on his right with a straw stalk in it, the coal burnt through its left
    breast (a narrow, scorched edge); the low band collar; and the coal itself, in its crater."""
    parts = []

    def folds(th, zz):
        blouse = 0.01 * gauss(zz - 1.085, 0.035)
        f = (0.0045 * np.cos(7 * th + 0.5) + 0.003 * np.cos(12 * th + 2.0)) * smoothstep(1.28, 1.06, zz)
        back = 0.003 * np.cos(9 * th + 1.0) * smoothstep(1.45, 1.25, zz) * smoothstep(1.4, 2.2, np.abs(th))
        return 0.003 + (0.005 + blouse + f + back) * smoothstep(1.03, 1.075, zz)

    def sd(th, zz):
        at = np.abs(th)
        out = 0.95 - zz
        out = np.maximum(out, np.where(zz > 1.43, (0.045 + 1.25 * (zz - 1.43)) - at, -1.0))
        arm = np.sqrt(((at - np.pi / 2 - 0.03) / 0.22) ** 2 + ((zz - 1.372) / 0.05) ** 2)
        out = np.maximum(out, (1.0 - arm) * 0.06)
        return np.maximum(out, zz - 1.548)
    (V, F), th, zz, edge = shell(TORSO, 0.95, 1.548, sd, folds)
    ember = np.clip(smoothstep(0.016, 0.0, edge) * (0.25 + 0.75 * (0.5 + 0.5 * pnoise(V, 50.0, 2))), 0.0, 1.0)
    cavity = np.maximum(0.32 * gauss(th, 0.011) * (zz < 1.432), 0.55 * smoothstep(0.02, 0.0, edge))
    low = reduce((V, F), TRIANGLES['shirt'])
    near = nearest(V, low[0])
    shirt = Part('Shirt', low, SHIRT, ember=ember[near], cavity=cavity[near])
    parts.append(torso_weights(shirt, low[0][:, 2], low[0][:, 0]))
    # His chest in the open throat: the skin under the shirt there.
    ths = np.linspace(-0.38, 0.38, 13)
    zs = np.linspace(1.4, 1.56, 7)
    P = surface(TORSO, zs, ths, grow=-0.002)
    part = Part('Chest', (P.reshape(-1, 3), [grid_faces(len(zs), len(ths), closed=False)]), SKIN)
    parts.append(torso_weights(part, part.V[:, 2], part.V[:, 0], shoulders=False))
    # Buttons down the placket under the opening, and the breast pocket on his right.
    pieces = []
    for zb in (1.405, 1.235, 1.15, 1.065):
        p = surface(TORSO, [zb], np.array([0.0]), grow=0.012)[0, 0]
        pieces.append(ellipsoid(p, (0.0065, 0.0065, 0.004), np.array([[0, 0, 1.0], [1.0, 0, 0], [0, -1.0, 0]]), 8, 4))
    part = Part('Buttons', merge(pieces), SHIRT, cavity=0.4)
    parts.append(torso_weights(part, part.V[:, 2], part.V[:, 0], shoulders=False))
    ts, zp = np.linspace(-0.6, -0.26, 6), np.linspace(1.27, 1.37, 5)
    P = surface(TORSO, zp, ts, grow=0.0125)
    T, Z = np.meshgrid(np.linspace(0, 1, len(ts)), np.linspace(0, 1, len(zp)))
    rim = smoothstep(0.22, 0.0, np.minimum(np.minimum(T, 1 - T), np.minimum(Z, 1 - Z)))
    part = Part('Pocket', (P.reshape(-1, 3), [grid_faces(len(zp), len(ts), closed=False)]), SHIRT, cavity=(0.5 * rim).ravel())
    parts.append(torso_weights(part, part.V[:, 2], part.V[:, 0], shoulders=False))
    # The low band collar round his neck, open at the throat.
    a = np.linspace(-np.pi, np.pi, 40, endpoint=False)
    rings = np.array([np.stack([rr * np.sin(a), -0.008 - rr * 0.92 * np.cos(a), np.full_like(a, z0)], -1)
                      for z0, rr in ((1.522, 0.058), (1.537, 0.056), (1.552, 0.055))])
    Vc, Fc, used = cut(rings, np.broadcast_to((0.24 - np.abs(a))[None, :], rings.shape[:2]) * 0.1)
    part = Part('Collar', (Vc, Fc), SHIRT)
    parts.append(part.weigh('spine_02', 0.6).weigh('neck', 0.4))
    # The skin round the coal: a crater sunk where the heart was, scorched at its rim and only faintly warm.
    center, nc, pc = coal_center()
    tc, zc = COAL_AT
    th = np.linspace(tc - 0.5, tc + 0.5, 15)
    z = np.linspace(zc - 0.085, zc + 0.085, 11)
    P = surface(TORSO, z, th, grow=0.0)
    piece = (P.reshape(-1, 3), [grid_faces(len(z), len(th), closed=False)])
    piece = push(piece, pc, COAL_RADIUS * 1.35, -0.016)
    dist = np.linalg.norm(piece[0] - pc, axis=1)
    cavity = smoothstep(COAL_RADIUS * 1.3, COAL_RADIUS * 0.7, dist) * 0.85
    ember = (smoothstep(COAL_RADIUS * 0.8, COAL_RADIUS * 1.1, dist) * smoothstep(COAL_RADIUS * 1.5, COAL_RADIUS * 1.2, dist)
             * (0.3 + 0.7 * (0.5 + 0.5 * pnoise(piece[0], 60.0, 5))))
    parts.append(Part('Crater', piece, SKIN, cavity=cavity, ember=ember).weigh('spine_02', 1.0))
    # The coal: lumpy, small, banked.
    V, F = ellipsoid(center, (COAL_RADIUS, COAL_RADIUS, COAL_RADIUS * 0.92), segs=14, rings=9)
    V = V + (V - center) * (0.16 * pnoise(V, 34.0, 3))[:, None]
    parts.append(Part('Coal', (V, F), SKIN, slot=1).weigh('coal', 1.0))
    # A straw stalk standing out of the pocket.
    p0 = surface(TORSO, [1.355], np.array([-0.43]), 0.018)[0, 0]
    stalk = tube([p0 - (0.0, 0.0, 0.03), p0 + (0.012, -0.01, 0.07), p0 + (0.03, -0.02, 0.12)], [0.0017, 0.0015, 0.0011],
                 segs=5, up=(0.0, -1.0, 0.0), domes=1)
    parts.append(torso_weights(Part('Stalk', stalk, SHIRT, cavity=0.05), stalk[0][:, 2], stalk[0][:, 0], shoulders=False))
    return parts


def build_braces():
    """His braces: the right one up over his shoulder and straight down his back; the left slipped off for the heat, its
    loop hanging by his left hip; their buttons at the waistband."""
    parts = []

    def on_body(keys, grow):
        return [surface(TORSO, [z], np.array([t]), grow)[0, 0] for t, z in keys]
    keys = [(-0.64, 1.04), (-0.6, 1.16), (-0.53, 1.29), (-0.52, 1.41), (-0.8, 1.5), (-1.5708, 1.535), (-2.33, 1.5),
            (-2.62, 1.41), (-2.66, 1.28), (-2.66, 1.15), (-2.64, 1.04)]
    piece = ribbon(on_body(keys, 0.021), 0.031, n=36, cols=3, out=lambda C: outward(np.arctan2(C[:, 0], -C[:, 1])),
                   curl=0.05)
    part = Part('Brace', piece, BRACES)
    parts.append(torso_weights(part, part.V[:, 2], part.V[:, 0]))
    front, back = on_body([(0.64, 1.04)], 0.026)[0], on_body([(2.64, 1.04)], 0.026)[0]
    loop = [front, np.array([0.19, -0.075, 0.88]), np.array([0.205, -0.01, 0.77]), np.array([0.196, 0.07, 0.87]), back]
    piece = ribbon(loop, 0.03, n=20, cols=3, out=lambda C: unit(np.column_stack([C[:, 0], C[:, 1], np.zeros(len(C))])),
                   curl=0.05)
    part = Part('BraceLoop', piece, BRACES)
    hang = smoothstep(1.0, 0.86, part.V[:, 2])
    parts.append(part.weigh('pelvis', 1.0 - 0.6 * hang).weigh('skirt_f_01', 0.6 * hang))
    pieces = []
    for t in (-0.64, 0.64, 2.64, -2.64):
        p = surface(TORSO, [1.04], np.array([t]), 0.028)[0, 0]
        n = outward(t)
        pieces.append(ellipsoid(p, (0.007, 0.007, 0.0045), np.array([[0, 0, 1.0], np.cross(n, (0, 0, 1.0)), n]), 8, 4))
    parts.append(Part('BraceButtons', merge(pieces), BRACES, cavity=0.4).weigh('pelvis', 1.0))
    return parts


def build_trousers():
    """Brown duck trousers from the waist to the knees: the hips and seat round the torso's bottom, two loose thighs,
    each leg ending at the knee in a ragged, torn hem (a cut edge, not a fade: M_Ghost's noise would break a fade into
    holes); a flour-sack patch on the right thigh."""
    theta = np.linspace(0.0, 2.0 * np.pi, 72, endpoint=False)
    z = np.linspace(0.93, 1.05, 8)
    pieces = [closed_loft(surface(TORSO, z, theta, grow=0.018)), ellipsoid((0.0, 0.01, 0.9), (0.178, 0.118, 0.1))]
    for s in SIDES:
        H, K = HIPS[s] + np.array([0.0, 0.0, 0.01]), KNEES[s] - np.array([0.0, 0.0, 0.05])
        mid = (H + K) * 0.5 + np.array([s * 0.008, -0.012, 0.0])
        pieces.append(tube(spline([H, mid, K], 12), np.linspace(0.087, 0.062, 12), segs=24))
    V, F = union(pieces, 0.006, smooth=4)
    # The hems: a ragged line round each knee; the faces below it go.
    side, t = leg_param(V)
    ang = np.arctan2(V[:, 1] - np.where(side > 0, KNEES[1.0][1], KNEES[-1.0][1]),
                     V[:, 0] - np.where(side > 0, KNEES[1.0][0], KNEES[-1.0][0]))
    line = (1.0 - 0.07 * (0.5 + 0.5 * np.sin(5.0 * ang + side)) - 0.05 * np.sin(11.0 * ang + 2.0 * side) ** 2
            - 0.03 * np.sin(23.0 * ang) ** 2)
    below = (t > line) & (V[:, 2] < 0.7)
    F = [f[~below[f].any(1)] for f in F]
    V, F = clean((V, F))
    low = reduce((V, F), TRIANGLES['trousers'])
    Vl = low[0]
    side, t = leg_param(Vl)
    cav = 0.3 * gauss(Vl[:, 0], 0.004) * (Vl[:, 1] < 0.0) * smoothstep(0.76, 0.86, Vl[:, 2]) * smoothstep(1.0, 0.94, Vl[:, 2])
    cav = np.maximum(cav, 0.22 * smoothstep(0.9, 0.99, t) * (Vl[:, 2] < 0.7))          # the hem's frayed edge
    part = Part('Trousers', low, TROUSERS, cavity=cav)
    parts = [thigh_weights(part, Vl)]
    # The flour-sack patch on his right thigh's front, stitched round.
    s = -1.0
    H, K = HIPS[s], KNEES[s]
    d = unit(K - H)
    front = unit(np.cross(d, (1.0, 0.0, 0.0)))
    front = -front if front[1] > 0 else front
    side_v = unit(np.cross(front, d))
    c = (H + K) * 0.5 + np.array([0.0, 0.0, 0.03])
    uu, vv = np.meshgrid(np.linspace(-1, 1, 6), np.linspace(-1, 1, 6))
    P = (c + d[None, None, :] * (vv[..., None] * 0.055) + side_v[None, None, :] * (uu[..., None] * 0.045)
         + front[None, None, :] * (0.083 - 0.004 * uu[..., None] ** 2))
    edge = np.minimum(1 - np.abs(uu), 1 - np.abs(vv))
    patch = Part('Patch', (P.reshape(-1, 3), [grid_faces(6, 6, closed=False)]), SHIRT,
                 cavity=(0.5 * smoothstep(0.25, 0.0, edge)).ravel())
    parts.append(patch.weigh('skirt_f_01', 1.0))
    return parts


def rolled_sleeve(S, E, r0, r1, seed, roll=0.017):
    """A loose shirt sleeve from the shoulder, rolled to just above the elbow: folds winding down from the armpit, a pile
    of small folds above the roll. Returns the piece (closed), the line it runs along."""
    rng = np.random.default_rng(seed)
    side = 1.0 if S[0] > 0 else -1.0
    d = unit(E - S)
    end = E - d * 0.03
    C = spline([S + np.array([-side * 0.012, 0.0, 0.008]), (S + end) * 0.5 + np.cross(d, [0.0, 0.0, 1.0]) * 0.004, end], 16)
    starts = rng.uniform(0.0, 2.0 * np.pi, 3)
    twists = rng.uniform(0.8, 1.6, 3) * rng.choice((-1.0, 1.0), 3)
    amps = rng.uniform(0.09, 0.15, 3)

    def rfn(s, a):
        base = r0 + (r1 - r0) * s
        f = sum(k * np.exp(-(wrap(a - p - t * s) / 0.38) ** 2) for k, p, t in zip(amps, starts, twists))
        f = f * smoothstep(0.04, 0.25, s) * smoothstep(0.96, 0.7, s)
        pile = sum(0.06 * np.exp(-((s - sk) / 0.05) ** 2) * (0.6 + 0.4 * np.cos(a + ph)) for sk, ph in ((0.8, 0.5), (0.9, 2.4)))
        return base * (1.0 + f + pile - 0.03)
    return merge([tube_fn(C, rfn, segs=16), torus(end, d, r1 - 0.002, roll, segs=16, sides=6, squash=0.9)]), C


def build_arms():
    """Shirt sleeves rolled past the elbows, his bare forearms and big hands (the right fist round the fork's shaft, the
    left hanging loose); fingers on their own bones."""
    parts, fingers = [], {}
    for k, s in enumerate(SIDES):
        sfx = SUFFIX[s]
        A = ARMS[s]
        S, E, W = A['S'], A['E'], A['W']
        piece, line = rolled_sleeve(S, E, 0.063, 0.056, 31 + k)
        low = reduce(piece, TRIANGLES['sleeve'])
        part = Part(f'Sleeve_{sfx}', low, SHIRT)
        d, along = polyline_distance(low[0], line)
        top = smoothstep(0.25, 0.0, along)
        part.weigh('spine_02', 0.45 * top)
        part.weigh(f'upperarm_{sfx}', 1.0 - 0.45 * top)
        parts.append(part)
        dv = unit(W - E)
        start = E - unit(E - S) * 0.012
        L = np.linalg.norm(W - start)
        C = [start, start + dv * L * 0.3, start + dv * L * 0.65, W - dv * 0.01]
        limb = [tube(C, [0.039, 0.041, 0.033, 0.026], [0.041, 0.043, 0.036, 0.031], segs=18, up=A['u'])]
        hand, lines = hand_pieces(W, A['f'], A['u'], s, A['curls'], A['spread'], thumb=A['thumb'], length=1.02, thin=1.12,
                                  claw=0.0, knuckle=1.2, scale=HAND_SCALE)
        low = reduce(union(limb + hand, 0.0024, smooth=3), TRIANGLES['hand'])
        part = Part(f'Hand_{sfx}', low, SKIN)
        V = low[0]
        u_ = ((V - E) @ dv) / np.linalg.norm(W - E)
        hand_share = smoothstep(0.9, 1.02, u_)
        elbow = 0.5 * smoothstep(0.1, -0.02, u_)
        best = np.full(len(V), np.inf)
        owner = np.full(len(V), -1)
        along = np.zeros(len(V))
        for j, (name, pts, radius) in enumerate(lines):
            dd, tt = polyline_distance(V, pts)
            better = (dd < radius + 0.007) & (dd < best)
            best[better], owner[better], along[better] = dd[better], j, tt[better]
        finger_share = np.where(owner >= 0, smoothstep(0.06, 0.2, along), 0.0)
        part.weigh(f'upperarm_{sfx}', elbow * (1.0 - hand_share))
        part.weigh(f'lowerarm_{sfx}', (1.0 - elbow) * (1.0 - hand_share))
        part.weigh(f'hand_{sfx}', hand_share * (1.0 - finger_share))
        for j, (name, pts, radius) in enumerate(lines):
            part.weigh(finger_bone(name, sfx), hand_share * finger_share * (owner == j))
        parts.append(part)
        fingers[s] = lines
    return parts, fingers


def hand_pieces(W, fwd, up, side, curl, spread, thumb=0.35, length=1.0, thin=1.0, claw=0.0, knuckle=1.0, scale=1.0):
    """A hand from the wrist W (fwd along the fingers, up out of its back; side +1 left): closed pieces for a union,
    and each finger's line (name, points, radius) for its weights (Unpaid.py's hand, as Abel.py builds it)."""
    f = unit(fwd)
    u = unit(np.asarray(up, float) - f * np.dot(up, f))
    t = np.cross(f, u) * side
    frame_ = np.array([f, np.cross(u, f), u])
    s = scale
    W = np.asarray(W, float)
    pieces = [ellipsoid(W + f * 0.047 * s * length ** 0.3, (0.047 * s * length ** 0.3, 0.041 * s, 0.0145 * s * thin),
                        frame_, 24, 14),
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
        r0 = 0.0089 * s * thin
        radii = [r0, r0 * 0.93, r0 * 0.84, r0 * (0.7 - 0.35 * claw)]
        C = spline(pts, 14)
        rr = np.interp(np.linspace(0, 1, 14), [0, 0.45, 0.76, 1.0], radii)
        pieces.append(tube(C, rr, segs=12, up=lu, domes=3, pointed=claw))
        pieces.append(ellipsoid(base + u * 0.003 * s, (0.0105 * s * knuckle, 0.0098 * s * knuckle, 0.0085 * s * knuckle),
                                frame_, 12, 8))
        lines.append((name, np.array(pts), r0))
    tb = W + f * 0.018 * s + t * 0.022 * s - u * 0.006 * s
    d = unit(f * 0.55 + t * 0.75 - u * 0.35)
    pts = [tb, tb + d * 0.042 * s]
    lu = unit(np.cross(d, f) * side + u * 0.5)
    for share in (0.036, 0.03):
        towards = unit(f - d * np.dot(f, d))
        d = unit(d * math.cos(thumb) + towards * math.sin(thumb) - u * 0.25 * math.sin(thumb))
        pts.append(pts[-1] + d * share * s * length ** 0.5)
    C = spline(pts, 14)
    rr = np.interp(np.linspace(0, 1, 14), [0, 0.4, 0.75, 1.0],
                   [0.0145 * s * thin, 0.0105 * s * thin, 0.0092 * s * thin, 0.0072 * s * thin * (1 - 0.4 * claw)])
    pieces.append(tube(C, rr, segs=12, up=lu, domes=3, pointed=claw))
    lines.append(('thumb', np.array(pts[1:]), 0.0125 * s * thin))
    return pieces, lines


# --- The head: a sun-creased farmer of about fifty, tired and patient ---
# Head space: origin mid-head at eye height (HEAD_CENTER in the rig), front -Y, up +Z, meters. One radial surface (a
# radius for every direction from its middle: face_radius), crowded at the face; the eyes are carved in it (eye_fields),
# the brows and the hair grown out of it, the stubble only shading. The lips are parted by a slit the jaw opens.

FACE = dict(width=0.146, front=0.097, back=0.106, top=0.118, bottom=0.13, mouth=-0.07, brow=0.011, bags=0.0012,
            nose=0.027, cheek=0.0065, hollow=0.0024, jaw=0.006, chin=0.0085, taper=0.09, naso=0.0015, smile=0.55,
            worry=0.35, squint=0.55, heavy=0.55, age=0.6, brow_hair=0.0032, droop=0.45, lips=1.0)
MOUTH_Z = FACE['mouth']                     # the lips' meeting line
HINGE = np.array([0.0, 0.008, -0.032])      # the jaw's hinge
MOUTH_HALF = 0.31                           # the mouth's slit, radians either side of the middle
JAW_BUILD = 14.0                            # the jaw is opened this far while the head is reduced (the lips apart, so
                                            # each keeps its own side), then closed to JAW_REST
ET = 0.36                                   # the eyes, either side of the front (angle round the head)
EYE_W = 0.0145                              # half an eye's width
FACE_R = 0.086                              # meters of face per radian round it at the eyes


def face_radius(d):
    """The head's radius along unit directions d (head space): a skull flatter in front than an egg, the nape tucked in,
    the face narrower than the skull below the cheekbones, the jaw a U to a broad chin; the brow ridge and the orbits;
    a straight nose with its bridge, tip and wings; cheekbones lifted a little by the smile, hollows under them,
    temples; the mouth's arch with the philtrum and both lips, the hollow under the lower lip; the jaw's angle; the
    folds from the nose past the corners of the mouth."""
    F = FACE
    TH = np.arctan2(d[..., 0], -d[..., 1])
    a = F['width'] / 2.0
    b = np.where(d[..., 1] < 0, F['front'], F['back'])
    c = np.where(d[..., 2] > 0, F['top'], F['bottom'])
    p = np.where(d[..., 1] < 0, 2.35, 2.0)
    r = 1.0 / ((np.abs(d[..., 0]) / a) ** p + (np.abs(d[..., 1]) / b) ** p + (np.abs(d[..., 2]) / c) ** p) ** (1.0 / p)
    z = d[..., 2] * r
    at = np.abs(TH)
    zm = F['mouth']
    r = r * (1.0 - 0.2 * smoothstep(-0.03, -0.11, z) * smoothstep(1.9, 2.8, at))
    r = r * (1.0 - F['taper'] * smoothstep(zm + 0.02, -F['bottom'], z) * smoothstep(2.2, 1.0, at) * np.abs(np.sin(TH)) ** 1.6)
    r = r * (1.0 - 0.05 * smoothstep(-0.005, -0.05, z) * smoothstep(0.35, 1.0, at) * smoothstep(2.0, 1.3, at))
    brow_z = 0.021 + 0.004 * F['worry'] * gauss(TH, 0.32)
    R = F['brow'] * gauss(TH, 0.72) * gauss(z - brow_z, 0.012)
    R = R - 0.0065 * (gauss(TH - ET, 0.17) + gauss(TH + ET, 0.17)) * gauss(z - 0.002, 0.016)
    R = R + F['bags'] * (gauss(TH - ET - 0.03, 0.12) + gauss(TH + ET + 0.03, 0.12)) * gauss(z + 0.0165, 0.0045)
    nose = (F['nose'] * smoothstep(0.014, -0.034, z) * smoothstep(-0.056, -0.042, z)
            * gauss(TH, 0.06 + 0.065 * smoothstep(-0.01, -0.046, z)))
    R = R + nose + 0.0016 * gauss(TH, 0.05) * gauss(z + 0.011, 0.007) + 0.0034 * gauss(TH, 0.085) * gauss(z + 0.04, 0.0065)
    R = R + 0.0042 * (gauss(TH - 0.095, 0.036) + gauss(TH + 0.095, 0.036)) * gauss(z + 0.0435, 0.0058)
    R = R + (F['cheek'] * gauss(at - 0.66, 0.22) * gauss(z + 0.02, 0.016)
             + 0.003 * gauss(z + 0.018, 0.009) * smoothstep(0.6, 0.8, at) * smoothstep(1.5, 1.25, at)
             - F['hollow'] * gauss(at - 0.68, 0.2) * gauss(z + 0.058, 0.018)
             - 0.005 * gauss(at - 1.05, 0.22) * gauss(z - 0.04, 0.025)
             + 0.0068 * gauss(TH, 0.5) * gauss(z + 0.068, 0.028)
             + F['jaw'] * gauss(at - 1.2, 0.22) * gauss(z + 0.09, 0.024)
             + F['chin'] * gauss(TH, 0.46) * gauss(z + F['bottom'] - 0.02, 0.017)
             + 0.0012 * F['smile'] * gauss(at - 0.5, 0.18) * gauss(z + 0.032, 0.014))
    nl = 0.21 + 0.22 * np.clip((-0.045 - z) / 0.04, 0.0, 1.0)
    window = smoothstep(-0.041, -0.049, z) * smoothstep(-0.088, -0.074, z)
    R = R - F['naso'] * gauss(at - nl, 0.032) * window + 0.0012 * gauss(at - nl - 0.07, 0.05) * window
    lips = F['lips']
    philtrum = (0.0005 * (gauss(TH - 0.034, 0.012) + gauss(TH + 0.034, 0.012)) * smoothstep(-0.05, -0.054, z)
                * smoothstep(zm + 0.008, zm + 0.013, z))
    R = R + (0.0014 * gauss(TH, 0.42) * gauss(z - zm, 0.02)
             - 0.0018 * gauss(TH, 0.27) * gauss(z - zm, 0.0022)
             + 0.0008 * lips * gauss(TH, 0.25) * gauss(z - zm - 0.0045, 0.0042)
             + 0.0014 * lips * gauss(TH, 0.23) * gauss(z - zm + 0.0058, 0.0038)
             - 0.0013 * gauss(TH, 0.2) * gauss(z - zm + 0.0165, 0.0038)
             + philtrum)
    return r + R


def face_grid(rows, cols):
    """Directions over the head, crowded at the face and sparse at the back: (phi, theta) per row and column."""
    u = np.linspace(-1.0, 1.0, cols, endpoint=False) + 1.0 / cols
    th = np.pi * (0.36 * u + 0.64 * u ** 3)
    w = np.linspace(-1.0, 1.0, rows + 2)[1:-1]
    phi = 0.5 * np.pi + 0.5 * np.pi * (0.42 * w + 0.58 * w ** 3) + 0.06 * (1.0 - w ** 2)
    return phi, th


def eye_fields(TH, z):
    """The eyes carved in the face: an almond opening tilted a little down at its outer corner (tired, kind), the eyeball
    rounding inside it and looking a little down, a rolled upper lid made heavy by the years and its crease, a lower lid
    lifted by the squint of a smile. Returns (displacement, cavity)."""
    F = FACE
    disp = np.zeros_like(TH)
    cav = np.zeros_like(TH)
    for s in SIDES:
        u = (TH - s * ET) * s * FACE_R
        v = z
        un = u / EYE_W
        shape = np.clip(1.0 - un ** 2, 0.0, None)
        tilt = -0.07 * u
        up_line = (0.0047 - 0.0014 * F['heavy']) * shape ** 0.6 * (1.0 + 0.12 * np.clip(-un, 0, 1)) + tilt + 0.0004
        lo_line = -(0.0041 - 0.0012 * F['squint']) * shape ** 0.8 * (1.0 + 0.15 * np.clip(un, 0, 1)) + tilt
        inside = np.minimum(np.minimum(up_line - v, v - lo_line), (1.0 - np.abs(un)) * 0.006)
        m = smoothstep(-0.0003, 0.0007, inside)
        near = smoothstep(1.45, 1.0, np.abs(un))
        ball = -0.0021 + 0.0024 * np.clip(1.0 - (u ** 2 + (v + 0.0006) ** 2) / EYE_W ** 2, 0.0, 1.0)
        crease = up_line + 0.0052 + 0.0012 * F['heavy']
        lid = (0.00105 * gauss(v - up_line - 0.0009, 0.0011) * near
               + 0.0005 * smoothstep(up_line, up_line + 0.002, v) * smoothstep(crease + 0.0005, crease - 0.0015, v) * near
               - 0.001 * gauss(v - crease, 0.001) * smoothstep(1.25, 0.8, np.abs(un))
               + 0.0006 * F['heavy'] * gauss(v - crease - 0.0018, 0.0016) * near
               + 0.0007 * gauss(v - lo_line + 0.0008, 0.0009) * near
               - 0.0006 * gauss(v - lo_line + 0.0045, 0.0014) * near)
        disp = disp + m * ball + (1.0 - m) * lid
        iris = gauss(np.hypot(u - 0.0004 * s, v + 0.0012), 0.0046)
        eye_cav = np.clip(0.48 + 0.46 * iris, 0.0, 0.93)
        lash = 0.8 * gauss(v - up_line - 0.0002, 0.0008) * smoothstep(1.08, 0.75, np.abs(un))
        lower = 0.28 * gauss(v - lo_line + 0.0003, 0.0006) * smoothstep(1.0, 0.6, np.abs(un))
        cav = np.maximum(cav, np.maximum(m * eye_cav, np.maximum(lash, lower)))
        cav = np.maximum(cav, 0.32 * gauss(v - crease, 0.0009) * smoothstep(1.2, 0.8, np.abs(un)))
        cav = np.maximum(cav, 0.16 * gauss(v - crease - 0.004, 0.004) * smoothstep(1.4, 0.7, np.abs(un)))
        cav = np.maximum(cav, 0.2 * np.exp(-(u / 0.021) ** 2 - ((v - 0.0015) / 0.011) ** 2))
    return disp, cav


def beard_line(at):
    """Where his stubble starts on the face: just under the cheekbones, up the jaw to the sideburns."""
    return -0.034 - 0.012 * gauss(at - 0.8, 0.3) + 0.06 * smoothstep(1.15, 1.5, at)


def head_surface(rows=150, cols=168):
    """The head as one surface: the anatomy (face_radius) and the eyes carved in it; lines of age across the forehead,
    deep crow's feet from a life of squinting into the sun, the folds from the nose; brows relaxed, their outer ends
    drooping a little; the corners of the mouth turned up a hair; a few days' stubble (shading only); sun-bleached hair
    under the hat, ragged over the collar (Zone2). The lips are parted by a slit the jaw opens. Returns ((V, faces),
    cavity, jaw weight, keep, tint zone)."""
    F = FACE
    phi, th = face_grid(rows, cols)
    PH, TH = np.meshgrid(phi, th, indexing='ij')
    d = np.stack([np.sin(PH) * np.sin(TH), -np.sin(PH) * np.cos(TH), np.cos(PH)], -1)
    r = face_radius(d)
    z = d[..., 2] * r
    at = np.abs(TH)
    zm = MOUTH_Z
    eyes, eye_cav = eye_fields(TH, z)
    creases = sum(gauss(z - zk, 0.0028) * (1.0 + 0.35 * np.sin(TH * 9.0 + k * 1.7))
                  for k, zk in enumerate((0.05, 0.062, 0.074))) * gauss(TH, 0.6)
    R = eyes - 0.0011 * F['age'] * creases
    crow = np.zeros_like(TH)
    for sgn in (1.0, -1.0):
        s_arc = (sgn * TH - 0.53) * FACE_R
        for ang in (-0.6, -0.2, 0.2, 0.55):
            along = math.cos(ang) * s_arc + math.sin(ang) * z
            across = -math.sin(ang) * s_arc + math.cos(ang) * z
            crow = crow + gauss(across, 0.0016) * smoothstep(0.003, 0.009, along) * smoothstep(0.034, 0.016, along)
    R = R - (0.0007 + 0.0007 * F['squint']) * crow
    line = (0.026 + 0.004 * F['worry'] * gauss(at - 0.12, 0.11) - 0.011 * ((at - 0.4) / 0.32) ** 2
            - 0.003 * F['droop'] * smoothstep(0.45, 0.75, at))
    brows = gauss(z - line, 0.0055) * smoothstep(0.07, 0.14, at) * smoothstep(0.8, 0.68, at)
    R = R + F['brow_hair'] * brows * (1.0 + 0.12 * np.sin(TH * 34.0 + z * 160.0))
    # The hair under the hat: shaggy, longer at the nape, its lower edge ragged over the collar.
    low = (-0.04 + 0.03 * smoothstep(2.4, 1.3, at) - 0.026 * smoothstep(1.7, 2.7, at)
           + 0.007 * np.sin(TH * 23.0) * np.sin(TH * 9.0 + 1.0))
    front = 0.95 + 0.3 * smoothstep(0.045, -0.02, z)
    hm = smoothstep(front - 0.16, front + 0.08, at) * smoothstep(low - 0.006, low + 0.008, z)
    comb2 = np.sin(TH * 24.0 + 1.6 * np.sin(z * 60.0))
    R = R + hm * 0.009 * (1.0 + 0.12 * comb2)
    P = d * (r + R)[..., None]
    P[..., 2] += 0.0022 * F['smile'] * smoothstep(0.12, 0.34, at) * gauss(z - zm, 0.014) * smoothstep(1.0, 0.5, at)
    zz = P[..., 2]
    cav = eye_cav
    cav = np.maximum(cav, 0.42 * smoothstep(0.15, 0.6, brows))
    cav = np.maximum(cav, 0.7 * (gauss(TH - 0.06, 0.02) + gauss(TH + 0.06, 0.02)) * gauss(z + 0.0485, 0.0042))
    cav = np.maximum(cav, 0.6 * gauss(TH, 0.27) * gauss(z - zm, 0.0024) * (1.0 - smoothstep(0.27, 0.36, at)))
    cav = np.maximum(cav, 0.18 * gauss(TH, 0.2) * gauss(z - zm + 0.0165, 0.003))
    cav = np.maximum(cav, 0.24 * (gauss(TH - 0.33, 0.028) + gauss(TH + 0.33, 0.028)) * gauss(z - zm - 0.001, 0.0035) * F['smile'])
    nl = 0.21 + 0.22 * np.clip((-0.045 - z) / 0.04, 0.0, 1.0)
    window = smoothstep(-0.041, -0.049, z) * smoothstep(-0.088, -0.074, z)
    cav = np.maximum(cav, 0.26 * gauss(at - nl, 0.028) * window)
    cav = np.maximum(cav, 0.22 * np.clip(crow, 0.0, 1.0))
    cav = np.maximum(cav, 0.13 * F['age'] * np.clip(creases, 0.0, 1.0))
    # A few days' growth: a shadow over the jaw, the chin and the upper lip, speckled.
    lb = beard_line(at)
    lip_band = gauss(TH, 0.36) * smoothstep(zm + 0.0095, zm + 0.004, z) * smoothstep(zm - 0.012, zm - 0.005, z)
    grow = (smoothstep(lb + 0.008, lb - 0.01, z) * smoothstep(1.7, 1.5, at)
            + gauss(TH, 0.38) * smoothstep(zm + 0.026, zm + 0.016, z) * smoothstep(zm + 0.003, zm + 0.0095, z))
    speck = 0.5 + 0.5 * np.sin(TH * 210.0 + z * 530.0) * np.sin(TH * 97.0 - z * 830.0)
    cav = np.maximum(cav, np.clip(grow, 0.0, 1.0) * (1.0 - lip_band) * (0.11 + 0.08 * speck))
    tufts = 0.3 * np.sin(TH * 95.0 + z * 310.0) * np.sin(TH * 41.0 - z * 170.0)
    hairy = hm + tufts * smoothstep(0.05, 0.45, hm) * smoothstep(0.98, 0.6, hm)
    cav = np.where(hairy > 0.5, 0.1 + 0.05 * (1.0 - comb2), cav)
    zone = np.where(hairy > 0.5, SHIRT, SKIN).astype(float)
    # Where the reduction keeps detail: the face, the eyes and the mouth most.
    face = np.maximum(smoothstep(1.35, 0.95, at), 0.4 * smoothstep(1.9, 1.5, at)) * smoothstep(-0.16, -0.12, z) * smoothstep(0.085, 0.06, z)
    keep = face * (0.4 + 0.6 * np.clip(1.6 * (gauss(TH - ET, 0.22) + gauss(TH + ET, 0.22)) * gauss(z, 0.025)
                                       + gauss(TH, 0.5) * gauss(z - zm, 0.022) + gauss(TH, 0.12) * gauss(z + 0.03, 0.03), 0.0, 1.0))
    # The mouth: a slit along the row nearest the lip line, across the mouth (Abel.py's): the lower lip, the chin and
    # the stubble under it turn with the jaw, the cheeks beyond the corners stretch.
    centre_col = int(np.argmin(np.abs(th)))
    i_m = int(np.argmin(np.abs(zz[:, centre_col] - zm)))
    slit = np.where(np.abs(th) < MOUTH_HALF)[0]
    j0, j1 = slit.min(), slit.max()
    inner = np.arange(j0 + 1, j1)
    inside = smoothstep(MOUTH_HALF + 0.16, MOUTH_HALF, at)
    sharp = (np.arange(rows)[:, None] > i_m).astype(float) * np.ones_like(at)
    soft = smoothstep(zm + 0.004, zm - 0.03, zz) * smoothstep(2.3, 1.5, at)
    jw = inside * sharp + (1.0 - inside) * soft
    band = (np.abs(np.arange(rows)[:, None] - i_m) <= 2) & (at <= MOUTH_HALF + 0.06)
    keep = np.where(band, 1.0, keep)
    V = P.reshape(-1, 3)
    n = rows * cols
    dup = n + np.arange(len(inner))
    V = np.vstack([V, P[i_m, inner], P[0].mean(0) + (0.0, 0.0, 0.002), P[-1].mean(0) - (0.0, 0.0, 0.002)])
    jw_v = np.concatenate([jw.ravel(), np.ones(len(inner)), [0.0, 1.0]])
    jw_v[i_m * cols + inner] = 0.0
    cav_v = np.concatenate([cav.ravel(), np.full(len(inner), 0.6), [0.0, 0.0]])
    cav_v[i_m * cols + inner] = 0.6
    keep_v = np.concatenate([keep.ravel(), np.ones(len(inner)), [0.0, 0.0]])
    zone_v = np.concatenate([zone.ravel(), np.zeros(len(inner)), [0.0, 0.0]])
    Fg = grid_faces(rows, cols)
    lower = (Fg[:, 0] // cols == i_m)
    swap = {int(i_m * cols + j): int(k) for j, k in zip(inner, dup)}
    for f in np.where(lower)[0]:
        Fg[f] = [swap.get(int(v), int(v)) if v // cols == i_m else v for v in Fg[f]]
    top, bottom = n + len(inner), n + len(inner) + 1
    piece = orient((V, [Fg, fan(top, np.arange(cols)), fan(bottom, np.arange(cols) + n - cols)]))
    return piece, cav_v, jw_v, keep_v, zone_v


def jaw_turn(V, jw, degrees):
    """Head-space points turned about the jaw's hinge by degrees (down), as much as their jaw weight."""
    a = math.radians(degrees) * np.asarray(jw, float)
    y, z = V[:, 1] - HINGE[1], V[:, 2] - HINGE[2]
    out = V.copy()
    out[:, 1] = HINGE[1] + y * np.cos(a) - z * np.sin(a)
    out[:, 2] = HINGE[2] + y * np.sin(a) + z * np.cos(a)
    return out


def ear_piece(s, size=1.12):
    """An ear (head space, side s), as Abel.py's: the rim curling round, its bowl, the lobe, the flap before the opening.
    Returns the piece and its cavity."""
    rim = spline([(0.0, -0.007, 0.011), (0.0, -0.003, 0.025), (0.0, 0.008, 0.031), (0.0, 0.017, 0.024),
                  (0.0, 0.021, 0.008), (0.0, 0.017, -0.009), (0.0, 0.009, -0.02)], 24)
    rr = np.interp(np.linspace(0.0, 1.0, 24), [0.0, 0.3, 0.7, 1.0], [0.003, 0.0046, 0.0046, 0.0038])
    pieces = [tube(rim, rr, segs=10, up=(1.0, 0.0, 0.0), domes=2),
              ellipsoid((0.0, 0.007, 0.004), (0.0058, 0.0135, 0.023), segs=14, rings=9),
              ellipsoid((0.0, 0.006, -0.021), (0.0055, 0.008, 0.009), segs=10, rings=7),
              ellipsoid((0.0018, -0.0075, -0.004), (0.003, 0.0035, 0.005), segs=8, rings=5)]
    V, F = reduce(union(pieces, 0.0011, smooth=3), 190)
    cav = 0.48 * gauss(V[:, 1] - 0.006, 0.006) * gauss(V[:, 2] + 0.001, 0.011) * smoothstep(0.0012, -0.001, V[:, 0])
    R = rot_z(-s * 16.0) @ rot_x(-14.0)
    V = V * size * np.array([s, 1.0, 1.0])
    V = V @ R.T + np.array([s * (FACE['width'] / 2.0 - 0.003), 0.012, -0.014])
    if s < 0:
        F = [f[:, ::-1] for f in F]
    return (V, F), cav


def build_head():
    """The head (face, brows and hair in one surface), the ears, the mouth behind the lips and the teeth, the neck with
    the back of it creased by the sun, and the straw at the corner of his mouth."""
    parts = []
    center = HEAD_CENTER
    (V, F), cav, jw, keep, zone = head_surface()
    opened = jaw_turn(V, jw, JAW_BUILD)
    low = reduce((opened, F), TRIANGLES['head'], keep=keep, keep_factor=3.0)
    near = nearest(opened, low[0])
    jw_low = jw[near]
    Vr = jaw_turn(low[0], jw_low, JAW_REST - JAW_BUILD)
    head = Part('Head', (Vr + center, low[1]), zone[near], cavity=cav[near])
    parts.append(head.weigh('head', 1.0 - jw_low).weigh('jaw', jw_low))
    for s in SIDES:
        piece, c = ear_piece(s)
        parts.append(Part('Ear', transform(piece, None, center), SKIN, cavity=c).weigh('head', 1.0))
    zm = MOUTH_Z
    sock = ellipsoid((0.0, -0.06, zm - 0.006), (0.024, 0.026, 0.017), segs=12, rings=7)
    sw = smoothstep(zm + 0.004, zm - 0.01, sock[0][:, 2])
    parts.append(Part('Mouth', (jaw_turn(sock[0], sw, JAW_REST) + center, sock[1]), SKIN, cavity=1.0)
                 .weigh('head', 1.0 - sw).weigh('jaw', sw))
    for upper in (True, False):
        a = np.linspace(-0.55, 0.55, 9)
        zt = (zm + 0.0045, zm - 0.0005) if upper else (zm - 0.0015, zm - 0.0065)
        rr = 0.03 if upper else 0.028
        pts = np.array([[[rr * math.sin(x), -0.072 - 0.012 * math.cos(x) - (0.0015 if upper else 0.0), zt_]
                         for x in a] for zt_ in zt])
        piece = (pts.reshape(-1, 3), [grid_faces(2, len(a), closed=False)])
        Vt = piece[0] if upper else jaw_turn(piece[0], np.ones(len(piece[0])), JAW_REST)
        parts.append(Part('Teeth', (Vt + center, piece[1]), SKIN, cavity=0.2).weigh('head' if upper else 'jaw', 1.0))
    # The neck: up into the head from inside the collar, thick at its root; the back of it creased in a criss-cross by
    # a life in the sun.
    C = spline([(0.0, -0.01, 1.47), (0.0, -0.022, 1.55), center + (0.0, 0.018, -0.07)], 10)

    def rfn(s, a):
        cords = 0.07 * (gauss(wrap(a - 0.95), 0.25) + gauss(wrap(a + 0.95), 0.25)) * smoothstep(0.25, 0.55, s)
        apple = 0.09 * gauss(wrap(a), 0.22) * gauss(s - 0.55, 0.12)
        return np.interp(s, [0.0, 0.45, 1.0], [0.062, 0.05, 0.046]) * (1.0 + cords + apple)
    neck = tube_fn(C, rfn, segs=16, up=(0.0, -1.0, 0.0))
    s_neck = polyline_distance(neck[0], C)[1]
    A = np.arctan2(neck[0][:, 0], -(neck[0][:, 1] - np.interp(s_neck, np.linspace(0, 1, len(C)), C[:, 1])))
    nape = gauss(wrap(A - np.pi), 0.9)
    lines = np.maximum(smoothstep(0.82, 0.97, np.sin(A * 9.0 + s_neck * 26.0)),
                       smoothstep(0.82, 0.97, np.sin(A * 9.0 - s_neck * 26.0)))
    part = Part('Neck', neck, SKIN, cavity=0.3 * nape * lines * smoothstep(0.15, 0.4, s_neck) * smoothstep(0.95, 0.75, s_neck))
    part.weigh('spine_02', 0.6 * smoothstep(0.25, 0.0, s_neck))
    part.weigh('head', smoothstep(0.6, 1.0, s_neck))
    part.weigh('neck', 1.0 - 0.6 * smoothstep(0.25, 0.0, s_neck) - smoothstep(0.6, 1.0, s_neck))
    parts.append(part)
    # The straw between his lips at the corner of his mouth, tipping down and out (on the head: it stays put as he talks).
    d = unit(np.array([math.sin(0.3), -math.cos(0.3), zm / 0.09]))
    a0 = d * float(face_radius(d[None])[0])
    path = [a0 + np.array([-0.014, 0.012, 0.0]), a0 + np.array([0.035, -0.022, -0.018]), a0 + np.array([0.078, -0.04, -0.045])]
    straw = tube([p + center for p in path], [0.0017, 0.0015, 0.001], segs=5, up=(0.0, 0.0, 1.0), domes=1)
    parts.append(Part('Straw', straw, SHIRT, cavity=0.05).weigh('head', 1.0))
    return parts


# --- The shroud below the knees ---

SHROUD_SPLIT = 0.3          # where the shroud tears into strips
STRIP_CENTERS = (12.0, 84.0, 156.0, -132.0, -60.0)  # degrees round the shroud from the front (+ toward his right): five
                                                    # wide strips, set a column off true, so no two hang as a pair
STRIP_SWEEP = 0.04          # how far the strips' tips drift to his left (m): a breath of wind, no gale
TIP_CUT = 0.006             # the strips' last row stands this far (m) past where their spacing would put it (their tips)
TOP_RISE = 0.07             # the tube runs on up inside the trouser legs this far over its old top (of the shroud's length,
                            # about 8 cm: past the hems' highest notches), closed there by a cap, so no open end shows
# M_Ghost's fade (vertex A) down the shroud, pulled in hard below the hems: the game's dithered fade draws a vertex A of
# 0.5 nearly solid, so the shroud is a veil the rail shows through within a hand's width of the knees. Down the tube it
# falls from FADE_HEM at the knees by a factor e every FADE_DECAY meters (about 0.75 at the hems' lowest teeth, 2 cm
# down; 0.4 at 10 cm; 0.18 at 20 cm; 0.05 where the strips tear off), then down each strip to nothing by STRIP_GONE of
# its length; up inside the legs it fades in from FADE_TOP at the cap, so its top edge never shows between his knees.
FADE_HEM = 0.88
FADE_DECAY = 0.127
STRIP_GONE = 0.5
FADE_TOP = 0.25
SHROUD_LENGTH = float(np.linalg.norm(np.diff(spline(SHROUD_PATH, 300), axis=0), axis=1).sum())


def shroud_fade(t, q=None):
    """The shroud's fade (vertex A) at t along it (0 its old top at the knees, negative up inside the legs) and, on a
    strip, q of the way from its root to its tip."""
    t = np.asarray(t, float)
    top = FADE_TOP + (1.0 - FADE_TOP) * smoothstep(-TOP_RISE, -0.01, t)
    down = np.clip(np.minimum(t, SHROUD_SPLIT), 0.0, None) * SHROUD_LENGTH
    fade = top * FADE_HEM * np.exp(-down / FADE_DECAY)
    if q is not None:
        fade = fade * (1.0 - smoothstep(0.0, STRIP_GONE, np.asarray(q, float)))
    return fade


def trouser_weights(trousers, P, k=4):
    """The trousers' skin weights where points P lie (their k nearest vertices, nearer counting more), so the shroud's
    top folds with the trousers round it at the knees and stays inside them."""
    names = sorted(trousers.weights)
    W = np.stack([trousers.weights[n] for n in names], 1)
    W = W / np.maximum(W.sum(1, keepdims=True), 1e-9)
    TV = trousers.V
    out = np.zeros((len(P), len(names)))
    for i, p in enumerate(P):
        d = np.linalg.norm(TV - p, axis=1)
        near = np.argsort(d, kind='stable')[:k]
        share = 1.0 / np.maximum(d[near], 1e-4)
        out[i] = (W[near] * share[:, None]).sum(0) / share.sum()
    return {n: out[:, j] for j, n in enumerate(names)}


def build_shroud(trousers):
    """The shroud at game density (Abel.py's, from the knees): a pale tube gathering out of the trousers' hems, whole for
    a hand's width, then five wide strips sweeping back behind him and tapering to soft points. The one in front is the
    shortest and swings back under him, so nothing hangs in front like a pair of shins. The tube runs on up inside the
    trouser legs, closed by a cap, and its top is skinned as the trousers round it are: when his knees bend in the sit it
    folds with them and no open end shows. It fades as the Unpaid's shroud does (shroud_fade). Returns the part and the
    tail chains' lines."""
    rows, cols, mrows = 17, 30, 6
    narrow, spread, wave, folds, harmonics, twist, seed = 0.55, 0.05, 0.03, 0.3, (5, 7, 9, 12), 0.8, 7
    rng = np.random.default_rng(seed)
    C = spline(SHROUD_PATH, 300)
    length = float(np.linalg.norm(np.diff(C, axis=0), axis=1).sum())
    T, N, B = frames(C, (0.0, -1.0, 0.0))
    amps = rng.uniform(0.55, 1.0, len(harmonics))
    phases = rng.uniform(0.0, 2.0 * np.pi, len(harmonics))
    drift = rng.uniform(-1.0, 1.0, len(harmonics)) * twist

    def at(t):
        # Above its old top (t < 0) the tube runs straight on up the path's first way.
        t = np.asarray(t, float)
        return (sample(C, t) + T[0] * (np.minimum(t, 0.0) * length)[..., None], sample(N, t), sample(B, t))

    def point(t, theta, out=0.0):
        t = np.asarray(t, float)
        c, n, b = at(t)
        w, d = table(SHROUD_SIZE, t)
        f = sum(a * np.cos(k * theta + p + dr * t * 6.0) for a, k, p, dr in zip(amps, harmonics, phases, drift)) / amps.sum()
        r = 1.0 + folds * smoothstep(0.04, 0.4, t) * (f - 0.35 * np.abs(f))
        return (c + n * (d * r * np.cos(theta) + out * np.cos(theta))[..., None]
                + b * (w * r * np.sin(theta) + out * np.sin(theta))[..., None])

    theta = np.linspace(0.0, 2.0 * np.pi, cols, endpoint=False)
    t_rows = np.concatenate([[-TOP_RISE, -TOP_RISE / 2.0], np.linspace(0.0, SHROUD_SPLIT, mrows)])
    trows = len(t_rows)
    tt, th = np.meshgrid(t_rows, theta, indexing='ij')
    ring = point(tt, th).reshape(-1, 3)
    # The cap over the tube's top, a little domed, inside the legs.
    cap_t = -TOP_RISE - 0.012
    cap = at(np.array([cap_t]))[0][0]
    cap_faces = fan(trows * cols, np.arange(cols), top=True)
    tube = np.vstack([ring, cap[None]])
    a, b, c = tube[cap_faces[0]]
    if np.dot(np.cross(b - a, c - a), -T[0]) < 0.0:          # the cap faces up, out of the tube
        cap_faces = cap_faces[:, ::-1]
    pieces = [(tube, [grid_faces(trows, cols), cap_faces])]
    blocks, start = [], trows * cols + 1
    ts = [tt.ravel(), [cap_t]]
    qs, sides = [np.zeros(tt.size + 1)], [np.zeros(tt.size + 1)]
    angles = [wrap(th).ravel(), [np.nan]]
    srows = rows - mrows + 1
    span = 1.0 - TIP_CUT / ((0.85 - SHROUD_SPLIT) * length)
    q = np.concatenate([np.linspace(0.0, span, srows - 1), [1.0]])[:, None]
    step = cols // len(STRIP_CENTERS)
    for k, center in enumerate(STRIP_CENTERS):
        jc = int(round(center / 360.0 * cols))
        js = np.arange(jc - step // 2, jc - step // 2 + step + 1)
        th_j = 2.0 * np.pi * js / cols
        tc = math.radians(center)
        front = max(0.0, math.cos(tc)) ** 2
        end = 0.72 + 0.24 * (1.0 - math.cos(tc)) / 2.0 + rng.uniform(-0.015, 0.015)
        across = (js - js.mean()) / (len(js) / 2.0)
        jag = end - 0.06 * np.abs(across) ** 1.5
        tq = SHROUD_SPLIT + (jag[None, :] - SHROUD_SPLIT) * q
        ang = tc + (th_j[None, :] - tc) * (1.0 - narrow * q ** 1.8)
        lift = spread * q ** 1.6
        Q = point(tq, ang, out=lift)
        c, n, b = at(SHROUD_SPLIT + (np.mean(jag) - SHROUD_SPLIT) * q[:, 0])
        side_dir = -math.sin(tc) * n + math.cos(tc) * b
        Q = Q + (side_dir * (wave * q * np.sin(q * 7.0 + k * 1.3)))[:, None, :]
        Q = Q + np.array([0.0, 0.16, 0.04])[None, None, :] * (front * q ** 1.4)[..., None]
        Q[..., 0] += STRIP_SWEEP * q ** 1.5
        pieces.append((Q.reshape(-1, 3), [grid_faces(srows, len(js), closed=False)]))
        blocks.append((start, srows, len(js)))
        start += srows * len(js)
        ts.append(tq.ravel())
        qs.append(np.broadcast_to(q, tq.shape).ravel())
        angles.append((wrap(tc) + ang - tc).ravel())
        sides.append(np.full(tq.size, 0.0 if abs(math.sin(tc)) < 0.3 else -math.copysign(1.0, math.sin(tc))))
    V, F = merge(pieces)
    t = np.concatenate(ts)
    q = np.concatenate(qs)
    strip = np.zeros(len(V), dtype=bool)
    strip[trows * cols + 1:] = True
    fade = np.where(strip, shroud_fade(t, q), shroud_fade(t))
    side = np.concatenate(sides)
    w, dd = table(SHROUD_SIZE, t)
    radius = (3.0 * (w + dd) - np.sqrt((3.0 * w + dd) * (w + 3.0 * dd))) / 2.0
    uv = np.column_stack([np.concatenate(angles), radius, -t * length])      # the cap keeps the box projection
    part = Part('Shroud', (V, F), SKIN, fade=fade, strip_uv=uv)
    mids = [(a + b) / 2 for a, b in zip(TAIL_STOPS[:-1], TAIL_STOPS[1:])]
    main = chain(t, mids, ('tail_01', 'tail_02', 'tail_03', 'tail_04', 'tail_05'))
    top = smoothstep(0.06, 0.0, t)
    side_share = 0.6 * smoothstep(0.1, 0.6, q) * (side != 0)
    for bone, wgt in main.items():
        part.weigh(bone, wgt * (1.0 - top) * (1.0 - side_share))
    # The top as the trousers round it are skinned (whole at the knees' joint and above, blending into the chain by
    # t 0.06), so it bends with them.
    tops = top > 0.0
    for bone, wgt in trouser_weights(trousers, V[tops]).items():
        full = np.zeros(len(V))
        full[tops] = wgt * top[tops]
        part.weigh(bone, full)
    for s in SIDES:
        sfx = SUFFIX[s]
        on_side = side_share * (side == s) * (1.0 - top)
        sw = chain(t, (0.46, 0.78), (f'tail_{sfx}_01', f'tail_{sfx}_02'))
        for bone, wgt in sw.items():
            part.weigh(bone, wgt * on_side)
    # The side chains run down the side strips: each side's two strips' middle (the binormal is on his right).
    lines = {}
    for s in SIDES:
        tc = math.radians(-96.0 if s > 0 else 120.0)
        lines[s] = [point(np.array([tt_]), np.array([tc]))[0] + np.array([s * STRIP_SWEEP * 0.3, 0.0, 0.0]) * k
                    for k, tt_ in enumerate((0.38, 0.62, 0.86))]
    return [part], dict(path=C, sides=lines, strips=dict(V=V, fade=fade, uv=uv, blocks=blocks, t=t, q=q, strip=strip))


# --- The rig ---

def bone_specs(fingers, tail):
    """(name, head, tail, parent) for every bone, Blender meters, laid out on the rest pose the parts were built in."""
    center, nc, pc = coal_center()
    specs = [('pelvis', (0.0, 0.005, 1.0), (0.0, 0.0, 1.12), None),
             ('spine_01', (0.0, 0.0, 1.12), (0.0, -0.005, 1.27), 'pelvis'),
             ('spine_02', (0.0, -0.005, 1.27), (0.0, -0.01, 1.48), 'spine_01'),
             ('coal', center, center + nc * 0.04, 'spine_02'),
             ('neck', (0.0, -0.02, 1.5), PIVOT, 'spine_02'),
             ('head', PIVOT, PIVOT + np.array([0.0, 0.0, 0.17]), 'neck'),
             ('jaw', HEAD_CENTER + HINGE, HEAD_CENTER + np.array([0.0, -0.08, -0.11]), 'head'),
             ('hat', HAT_PIVOT, HAT_PIVOT + np.array([0.0, 0.0, 0.1]), 'head')]
    for s in SIDES:
        sfx = SUFFIX[s]
        A = ARMS[s]
        specs.append((f'upperarm_{sfx}', A['S'], A['E'], 'spine_02'))
        specs.append((f'lowerarm_{sfx}', A['E'], A['W'], f'upperarm_{sfx}'))
        specs.append((f'hand_{sfx}', A['W'], A['W'] + A['f'] * 0.08, f'lowerarm_{sfx}'))
        for name, pts, radius in fingers[s]:
            specs.append((finger_bone(name, sfx), pts[0], pts[1], f'hand_{sfx}'))
        if s < 0:
            specs.append(('fork', A['G'], A['G'] + A['rod'] * 0.1, 'hand_r'))
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
    specs.append(('skirt_f_01', SKIRT_HINGE, SKIRT_KNEE, 'pelvis'))
    specs.append(('skirt_f_02', SKIRT_KNEE, SKIRT_SHIN, 'skirt_f_01'))
    return [(name, tuple(map(float, h)), tuple(map(float, t)), parent) for name, h, t, parent in specs]


def ghost_slot(name, colors, texture_set='Polymer', **scalars):
    """An M_Ghost slot: its properties are the material instance's parameters (the importer resets the instance and
    sets only these). Its Blender color is the first zone's, for the viewport."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(int(colors[0][1:], 16))
    for key in [k for k in mat.keys() if not k.startswith('_')]:
        del mat[key]
    mat['Master'] = 'Ghost'
    mat['TextureSet'] = texture_set
    for k, color in enumerate(colors):
        mat[f'Zone{k + 1}Color'] = color
    for key, value in scalars.items():
        mat[key] = value
    return mat


# The Unpaid's approved look (Unpaid.py's Ghost_A, as Abel's GhostAbel): a thin rim that still reads, the body's own
# faint glow, the fade's noise and steepness, the Polymer grain three times a meter; the ember edge round his coal's hole
# weak (banked).
GHOST_LOOK = dict(RimColor=RIM_COLOR, RimPower=4.0, RimStrength=2.2, GlowStrength=0.12, EmberStrength=0.8,
                  NoiseScale=1.4, FadeSoftness=0.07, UVScale=3.0)
# The hat: the Hay set's strands as its grain (BaseColorMap = T_Hay_BC), four times a meter; a soft, tight rim so its
# brim doesn't glow white underneath.
HAT_LOOK = dict(RimColor=RIM_COLOR, RimPower=6.0, RimStrength=0.3, GlowStrength=0.1, EmberStrength=0.0, NoiseScale=1.4,
                FadeSoftness=0.07, UVScale=4.0)
FORK_LOOK = dict(RimColor=RIM_COLOR, RimPower=4.0, RimStrength=1.6, GlowStrength=0.1, EmberStrength=0.0, NoiseScale=1.4,
                 FadeSoftness=0.07, UVScale=3.0)


def materials():
    """GhostAmos (the body), GhostCoal (shared with SK_Unpaid and SK_Abel: exactly their values), the props' slots."""
    body = ghost_slot('GhostAmos', ZONE_COLORS, **GHOST_LOOK)
    coal = bpy.data.materials.get('GhostCoal') or bpy.data.materials.new('GhostCoal')
    coal.use_nodes = True
    bsdf = next(n for n in coal.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x1d110c)
    for key in [k for k in coal.keys() if not k.startswith('_')]:
        del coal[key]
    coal['Master'] = 'Ghost'
    coal['Coal'] = 1.0               # the crust and roughness are the master's
    coal['EmberStrength'] = 2.1      # its cracks (SK_Unpaid's value: the slot is shared)
    coal['FadeSoftness'] = 0.07
    hat = ghost_slot('GhostAmosHat', HAT_COLORS, texture_set='Hay', **HAT_LOOK)
    fork = ghost_slot('GhostAmosFork', FORK_COLORS, **FORK_LOOK)
    return dict(body=body, coal=coal, hat=hat, fork=fork)


def noise_field():
    """M_Ghost's fade noise as the master samples it (Tools/Unreal/build_creature_materials.py, GHOST_OPACITY): the macro
    noise at UV x NoiseScale and, finer, at UV x NoiseScale x 3.07 + (0.37, 0.11), weighted 0.62 and 0.38, drifting along
    V. Returns N(u, v, drift) for UV as Unreal reads them (V flipped on import)."""
    img = bpy.data.images.load(os.path.join(lt.TEXTURE_DIR, 'MacroNoise', 'T_MacroNoise_M.png'), check_existing=True)
    img.colorspace_settings.name = 'Non-Color'
    W, H = img.size
    px = np.empty(W * H * 4, np.float32)
    img.pixels.foreach_get(px)
    R = px.reshape(H, W, 4)[::-1, :, 0].astype(float)      # row 0 the texture's top, as Unreal reads it

    def tex(x, y):
        x, y = (x % 1.0) * W - 0.5, (y % 1.0) * H - 0.5
        x0, y0 = np.floor(x).astype(int), np.floor(y).astype(int)
        fx, fy = x - x0, y - y0
        x0, y0 = x0 % W, y0 % H
        x1, y1 = (x0 + 1) % W, (y0 + 1) % H
        return (R[y0, x0] * (1 - fx) * (1 - fy) + R[y0, x1] * fx * (1 - fy) + R[y1, x0] * (1 - fx) * fy
                + R[y1, x1] * fx * fy)
    scale = GHOST_LOOK['NoiseScale']

    def N(u, v, drift):
        return (0.62 * tex(u * scale, (v + drift) * scale)
                + 0.38 * tex(u * scale * 3.07 + 0.37, (v + drift * 1.6) * scale * 3.07 + 0.11))
    return N


def components(mask):
    """4-connected regions of a boolean grid: a label per cell (0 outside) and how many."""
    lab = np.zeros(mask.shape, dtype=np.int32)
    rows, cols = mask.shape
    n = 0
    for r0 in range(rows):
        for c0 in range(cols):
            if mask[r0, c0] and not lab[r0, c0]:
                n += 1
                lab[r0, c0] = n
                stack = [(r0, c0)]
                while stack:
                    r, c = stack.pop()
                    for rr, cc in ((r + 1, c), (r - 1, c), (r, c + 1), (r, c - 1)):
                        if 0 <= rr < rows and 0 <= cc < cols and mask[rr, cc] and not lab[rr, cc]:
                            lab[rr, cc] = n
                            stack.append((rr, cc))
    return lab, n


def shroud_islands(strips, steps=18, density=8):
    """Where the strips show, as M_Ghost draws them (visible where 1.3 x fade - 0.15 > N), sampled densely over each
    strip while the drifting noise passes through a whole period: counts the bits that show without joining their strip
    (floating islands) and the holes and bites in a strip's body, with the largest (m across). The UVs and the fade are
    read as the rasterizer interpolates them across each quad."""
    N = noise_field()
    V, fade, uv = strips['V'], strips['fade'], strips['uv']
    U = uv[:, 0] * uv[:, 1]                                 # Unreal's UV 0: U the meters round the shroud,
    Vv = 1.0 - uv[:, 2]                                     # V flipped on import
    islands = holes = 0
    largest = 0.0
    period = 1.0 / GHOST_LOOK['NoiseScale']
    for start, nrow, ncol in strips['blocks']:
        idx = start + np.arange(nrow * ncol).reshape(nrow, ncol)
        r = np.linspace(0.0, nrow - 1.0, (nrow - 1) * density + 1)
        cc = np.linspace(0.0, ncol - 1.0, (ncol - 1) * density + 1)
        R0 = np.clip(np.floor(r).astype(int), 0, nrow - 2)
        C0 = np.clip(np.floor(cc).astype(int), 0, ncol - 2)
        fr, fc = (r - R0)[:, None], (cc - C0)[None, :]

        def lerp(values):
            return (values[idx[R0][:, C0]] * (1 - fr) * (1 - fc) + values[idx[R0][:, C0 + 1]] * (1 - fr) * fc
                    + values[idx[R0 + 1][:, C0]] * fr * (1 - fc) + values[idx[R0 + 1][:, C0 + 1]] * fr * fc)
        Fs, Us, Vs = lerp(fade), lerp(U), lerp(Vv)
        P = np.stack([lerp(V[:, k]) for k in range(3)], -1)
        along = float(np.linalg.norm(np.diff(P, axis=0), axis=-1).mean())
        across = float(np.linalg.norm(np.diff(P, axis=1), axis=-1).mean())
        for drift in np.linspace(0.0, period, steps, endpoint=False):
            shown = 1.3 * Fs - 0.15 - N(Us, Vs, drift) > 0.0
            lab, n = components(shown)
            attached = set(lab[0][lab[0] > 0].tolist())
            for i in range(1, n + 1):
                if i not in attached:
                    islands += 1
                    largest = max(largest, float(np.sqrt((lab == i).sum() * along * across)))
            # Holes and bites: what doesn't show anywhere but past the tip's cut (the last rows).
            body = ~shown
            body[-density:] = False
            lab, n = components(body)
            holes += n
            for i in range(1, n + 1):
                largest = max(largest, float(np.sqrt((lab == i).sum() * along * across)))
    return islands, holes, largest


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
        cols.append(np.column_stack([p.zones / 3.0, np.clip(p.cavity, 0, 1), np.clip(p.ember, 0, 1),
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
    order = np.argsort(-W, axis=1, kind='stable')
    keep = np.zeros_like(W, dtype=bool)
    np.put_along_axis(keep, order[:, :4], True, axis=1)
    W = np.where(keep, W, 0.0)
    W[W < 0.01] = 0.0
    W = W / np.maximum(W.sum(1, keepdims=True), 1e-9)
    return V, Fs, np.concatenate(cols), np.concatenate(slots), names, W


def strip_uvs(mesh, parts):
    """The shroud takes its own UVs over the box projection: U the meters round it, V minus the meters down it."""
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


LAYOUT = {}       # part name: its vertex ranges in SK_Amos's mesh; 'head_faces': the face's faces (filled by build_rig)


def build_rig():
    shirt_parts = build_shirt()
    brace_parts = build_braces()
    trouser_parts = build_trousers()
    arm_parts, fingers = build_arms()
    head_parts = build_head()
    shroud_parts, tail = build_shroud(trouser_parts[0])
    LAYOUT['strips'] = tail['strips']
    parts = shirt_parts + brace_parts + trouser_parts + arm_parts + head_parts + shroud_parts
    for p in parts:
        if not p.weights:
            raise RuntimeError(f'{p.name} has no weights')
    rig = lm.armature()
    lm.bones(rig, bone_specs(fingers, tail))
    V, faces, colors, slots, names, W = assemble(parts)
    mesh = to_mesh('Amos', V, faces)
    mats = materials()
    mesh.materials.append(mats['body'])
    mesh.materials.append(mats['coal'])
    mesh.polygons.foreach_set('material_index', slots.astype(np.int32))
    mesh.validate()
    if len(mesh.vertices) != len(V):
        raise RuntimeError('validate() changed the vertices')
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(70.0))
    # The face, ears and neck are never hard-edged: their folds and lids are drawn by shading alone.
    soft = np.concatenate([np.full(len(p.V), p.name in ('Head', 'Ear', 'Neck', 'Mouth', 'Teeth')) for p in parts])
    if 'sharp_edge' in mesh.attributes:
        ev = np.empty(len(mesh.edges) * 2, dtype=np.int64)
        mesh.edges.foreach_get('vertices', ev)
        ev = ev.reshape(-1, 2)
        sharp = np.empty(len(mesh.edges), dtype=bool)
        mesh.attributes['sharp_edge'].data.foreach_get('value', sharp)
        sharp &= ~(soft[ev[:, 0]] & soft[ev[:, 1]])
        mesh.attributes['sharp_edge'].data.foreach_set('value', sharp)
    body = bpy.data.objects.new('Amos', mesh)
    bpy.context.scene.collection.objects.link(body)
    lt.box_uv(body, 'Polymer')
    strip_uvs(mesh, parts)
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
    first = 0
    for p in parts:
        LAYOUT.setdefault(p.name, []).append((first, first + len(p.V)))
        if p.name == 'Head':
            LAYOUT['head_faces'] = [F + first for F in p.F]
        first += len(p.V)
    rig['LODs'] = '50,25'
    rig['LODScreens'] = '0.3,0.12'
    rig['Nanite'] = 0
    tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
    log(f'SK_Amos: {tris} triangles, {len(mesh.vertices)} vertices, {len(rig.data.bones)} bones')
    totals = {}
    for p in parts:
        totals[p.name] = totals.get(p.name, 0) + p.triangles()
    log('  ' + ', '.join(f'{name} {count}' for name, count in totals.items()))
    return rig, body, mats, colors, (V, faces, W, names)


# --- The props: the hat and the fork (static meshes modeled where they are at rest, their pivots at their bones) ---

def hat_pieces():
    """The harvest straw in hat space (origin at the brim's middle, front -Y): a round crown with a soft crease, a broad
    brim that droops and waves, a thin solid whose unbound edge is ragged (bites out of it, worn thinner where his hand
    takes it off), a dark cloth band with a stalk tucked in on his left, a few loose straws standing out of the edge.
    Returns the straw's pieces and the band's."""
    rng = np.random.default_rng(23)
    segs = 48
    theta = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    q = np.linspace(0.0, 1.0, 5)[:, None]
    # The ragged edge in waves the ring's 48 steps can draw (finer ones would only alias into a saw).
    ragged = (0.006 * np.sin(theta * 13.0) * np.sin(theta * 5.0 + 1.0) + 0.004 * np.sin(theta * 17.0 + 2.0)
              + 0.0025 * np.sin(theta * 23.0 + 0.6)
              - 0.009 * np.exp(-((wrap(theta - 2.2)) / 0.13) ** 2) - 0.007 * np.exp(-((wrap(theta + 0.9)) / 0.11) ** 2))
    r = 0.087 + (0.129 + ragged) * q * (1.0 + 0.025 * np.sin(3 * theta + 0.4) + 0.015 * np.sin(7 * theta + 1.3))
    droop = (-0.03 * q ** 1.6 * (0.6 + 0.4 * np.cos(2 * theta)) - 0.012 * q ** 2.2 * np.cos(theta)
             + 0.007 * q ** 2 * np.sin(5 * theta + 0.7) + 0.003 * q * np.sin(11 * theta))
    top = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.04, droop + 0.0 * theta], -1)
    thick = 0.006 - 0.002 * q                                  # thinner toward the frayed edge
    under = top - np.stack([np.zeros_like(top[..., 0]), np.zeros_like(top[..., 0]), np.broadcast_to(thick, top.shape[:2])], -1)
    R = len(q)
    V = np.vstack([top.reshape(-1, 3), under.reshape(-1, 3)])
    F = [grid_faces(R, segs)[:, ::-1], grid_faces(R, segs) + R * segs]
    outer = np.arange(segs)
    edge = np.stack([(R - 1) * segs + outer, (R - 1) * segs + (outer + 1) % segs,
                     R * segs + (R - 1) * segs + (outer + 1) % segs, R * segs + (R - 1) * segs + outer], 1)
    F.append(edge[:, ::-1])
    brim = (V, F)
    zc = [-0.006, 0.03, 0.064, 0.089, 0.104, 0.11]
    rc = [0.088, 0.087, 0.082, 0.07, 0.051, 0.028]
    crown = np.array([np.stack([rr * np.sin(theta), -rr * np.cos(theta) * 1.08, np.full_like(theta, zz)], -1)
                      for zz, rr in zip(zc, rc)])
    crown[..., 2] -= 0.006 * np.exp(-(crown[..., 0] / 0.03) ** 2) * (np.array(zc)[:, None] > 0.08)
    n = len(zc) * segs
    crown = (np.vstack([crown.reshape(-1, 3), [[0.0, 0.0, 0.106]]]),
             [grid_faces(len(zc), segs), fan(n, np.arange(segs) + n - segs, top=True)])
    straws = []
    for k in range(7):
        a = rng.uniform(0.0, 2.0 * np.pi)
        rr = 0.205 + rng.uniform(-0.008, 0.004)
        base = np.array([rr * math.sin(a), -rr * math.cos(a) * 1.04,
                         -0.03 * (0.6 + 0.4 * math.cos(2 * a)) - 0.012 * math.cos(a) - 0.002])
        b = a + rng.uniform(-0.5, 0.5)
        d = unit(np.array([math.sin(b), -math.cos(b), rng.uniform(-0.35, 0.15)]))
        L = rng.uniform(0.02, 0.045)
        straws.append(tube([base - d * 0.012, base + d * L], [0.0013, 0.0009], segs=3, up=(0.0, 0.0, 1.0), domes=1))
    tuck = spline([(0.088, 0.02, 0.012), (0.096, 0.0, 0.05), (0.112, -0.022, 0.082)], 5)
    straws.append(tube(tuck, np.linspace(0.0018, 0.0009, 5), segs=4, up=(1.0, 0.0, 0.0), domes=1))
    band = np.array([np.stack([0.0895 * np.sin(theta), -0.0895 * np.cos(theta) * 1.08, np.full_like(theta, z)], -1)
                     for z in (0.002, 0.028)])
    return merge([brim, crown] + straws), (band.reshape(-1, 3), [grid_faces(2, segs)])


def build_hat(mats):
    """SM_AmosHat: worn level, tipped back a touch; its origin is the 'hat' bone's head. SOCKET_Speaker at his mouth,
    facing his front."""
    straw, band = hat_pieces()
    R = rot_x(-4.0) @ rot_y(3.0)
    straw = transform(straw, R, (0.0, 0.0, 0.0))
    band = transform(band, R, (0.0, 0.0, 0.0))
    straw = (straw[0] + HAT_PIVOT, straw[1])
    band = (band[0] + HAT_PIVOT, band[1])
    V, faces = merge([straw, band])
    n_straw = len(straw[0])
    zone = np.where(np.arange(len(V)) < n_straw, SHIRT, BRACES) / 3.0
    # The brim's edge a little darker (worn), the band's shadow on the crown.
    r_xy = np.hypot(V[:, 0] - HAT_PIVOT[0], V[:, 1] - HAT_PIVOT[1])
    cav = np.where(np.arange(len(V)) < n_straw, 0.4 * smoothstep(0.17, 0.215, r_xy), 0.3)
    rgba = np.column_stack([zone, cav, np.zeros(len(V)), np.ones(len(V))])
    slots = np.zeros(sum(len(F) for F in faces), np.int32)
    hat = static_object('AmosHat', V, faces, slots, [mats['hat']], HAT_PIVOT)
    static_colors(hat.data, rgba_per_vertex=rgba)
    d = unit(np.array([0.0, -1.0, MOUTH_Z / 0.095]))
    mouth = HEAD_CENTER + d * (float(face_radius(d[None])[0]) + 0.01)
    lm.socket(hat, 'Speaker', tuple(mouth - HAT_PIVOT))
    log(f'SM_AmosHat: {sum(len(p.vertices) - 2 for p in hat.data.polygons)} triangles; SOCKET_Speaker at '
        f'{tuple(np.round(to_ue(mouth - HAT_PIVOT), 2))} cm from its pivot')
    return hat


def fork_pieces():
    """His hay fork in fork space: the butt at the origin, the ash shaft up +Z, the iron ferrule, a cross bar and three
    long tines curving to its front (-Y). Pieces by zone: 0 the ash, 2 the iron."""
    wood = [tube([(0.0, 0.0, 0.0), (0.0, 0.004, 0.7), (0.0, 0.0, 1.33)], [0.0152, 0.015, 0.0143], segs=10,
                 up=(0.0, -1.0, 0.0), domes=2)]
    iron = [tube([(0.0, 0.0, 1.31), (0.0, 0.0, 1.385)], [0.0168, 0.012], segs=10, up=(0.0, -1.0, 0.0), domes=1),
            tube(spline([(-0.042, 0.0, 1.392), (0.0, 0.0, 1.383), (0.042, 0.0, 1.392)], 6), 0.0068, segs=8,
                 up=(0.0, 0.0, 1.0), domes=2)]
    for x in (-0.042, 0.0, 0.042):
        path = [(x, 0.0, 1.39), (x * 1.06, -0.006, 1.48), (x * 1.16, -0.03, 1.58), (x * 1.24, -0.066, 1.665)]
        iron.append(tube(spline(path, 9), np.linspace(0.0062, 0.0026, 9), segs=7, up=(0.0, -1.0, 0.0), domes=2,
                         pointed=0.9))
    return {0: wood, 2: iron}


def fork_frame(G, rod, front=(0.0, -1.0, 0.0)):
    """Fork space to rig space for a fork whose shaft runs rod, held at G (FORK_GRIP up the shaft from its butt), its
    tines curving toward front: (3x3, the butt)."""
    z = unit(rod)
    y = -unit(np.asarray(front, float) - z * np.dot(front, z))
    R = np.column_stack([np.cross(y, z), y, z])
    return R, np.asarray(G, float) - z * FORK_GRIP


def build_fork(mats):
    """SM_AmosFork upright in his right fist at rest, its butt on the ground; its origin the 'fork' bone's head, where
    the fist holds the shaft."""
    A = ARMS[-1.0]
    R, butt = fork_frame(A['G'], A['rod'])
    Vs, Fs, zones = [], [], []
    n = 0
    for zone, pieces in fork_pieces().items():
        for V, faces in pieces:
            Vs.append(V @ R.T + butt)
            Fs += [np.asarray(F) + n for F in faces]
            zones.append(np.full(len(V), zone / 3.0))
            n += len(V)
    V = np.concatenate(Vs)
    rgba = np.column_stack([np.concatenate(zones), np.zeros(n), np.zeros(n), np.ones(n)])
    slots = np.zeros(sum(len(F) for F in Fs), np.int32)
    fork = static_object('AmosFork', V, Fs, slots, [mats['fork']], A['G'])
    static_colors(fork.data, rgba_per_vertex=rgba)
    log(f'SM_AmosFork: {sum(len(p.vertices) - 2 for p in fork.data.polygons)} triangles')
    return fork


# --- Hit zones ---

HULL_BONES = ('pelvis', 'spine_01', 'spine_02', 'neck', 'head', 'jaw', 'upperarm_l', 'upperarm_r', 'lowerarm_l',
              'lowerarm_r', 'hand_l', 'hand_r', 'tail_01', 'tail_02', 'skirt_f_01', 'skirt_f_02')
NO_HULL = ('tail_03', 'tail_04', 'tail_05', 'tail_l_01', 'tail_l_02', 'tail_r_01', 'tail_r_02', 'hat', 'fork')
HITS = {}


def static_colors(mesh, rgba_per_vertex=None, rgba_per_face=None):
    """A textured static mesh's colors: floats holding the values (exported linear)."""
    col = mesh.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    loops = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loops)
    if rgba_per_face is not None:
        per_loop = np.zeros((len(mesh.loops), 4), np.float32)
        for poly in mesh.polygons:
            per_loop[poly.loop_start:poly.loop_start + poly.loop_total] = rgba_per_face[poly.index]
    else:
        per_loop = rgba_per_vertex[loops]
    col.data.foreach_set('color', np.asarray(per_loop, np.float32).ravel())
    mesh.color_attributes.active_color = col
    mesh.color_attributes.render_color_index = 0


def static_object(name, V, faces, slots, materials_, pivot):
    """A static prop: its mesh in rig space moved so its origin is the pivot, left standing there."""
    mesh = to_mesh(name, np.asarray(V) - pivot, faces)
    for mat in materials_:
        mesh.materials.append(mat)
    mesh.polygons.foreach_set('material_index', np.asarray(slots, np.int32))
    mesh.validate()
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(60.0))
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    lt.box_uv(obj, 'Polymer')
    obj['Nanite'] = 0
    obj['LODs'] = '50,25'
    obj['LODScreens'] = '0.25,0.1'
    obj['Collision'] = 'None'
    obj.location = tuple(pivot)
    return obj


def hull_owner(bone):
    for finger in FINGERS:
        if bone.startswith(finger + '_'):
            return 'hand_' + bone[-1]
    return bone


def convex_planes(obj):
    """A convex hull object's face planes (outward unit normals and offsets) in rest-pose space."""
    M = np.array(obj.matrix_world)
    co = np.array([v.co[:] for v in obj.data.vertices]) @ M[:3, :3].T + M[:3, 3]
    tri = np.array([p.vertices[:3] for p in obj.data.polygons])
    n = np.cross(co[tri[:, 1]] - co[tri[:, 0]], co[tri[:, 2]] - co[tri[:, 0]])
    # A sliver (a face of almost no area) has no trustworthy normal: its plane could cut through the hull.
    keep = np.linalg.norm(n, axis=1) > 2e-7
    tri, n = tri[keep], n[keep]
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
    d = (n * co[tri[:, 0]]).sum(1)
    flip = n @ co.mean(0) - d > 0.0
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


def outside_hits(P, transforms=None, tol=0.004):
    """Which points lie outside every hit zone (on the body's bones). transforms maps each zone's bone from rest to pose
    (None: the rest pose). The tolerance allows for Blender's hull, which can drop a point lying a few millimeters
    outside it (3 mm, at the skirt's hem)."""
    inside = np.zeros(len(P), dtype=bool)
    for bone, (n, d) in HITS['planes'].items():
        if bone == 'lantern':
            continue
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


def make_hull(rig, bone, pts):
    cloud = bpy.data.meshes.new('_cloud')
    cloud.from_pydata(np.asarray(pts, float).tolist(), [], [])
    obj = bpy.data.objects.new('_cloud', cloud)
    hull = lm.hit_hull(rig, bone, [obj])
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(cloud)
    return hull



def to_ue(p):
    """A Blender rig-space point (m) in Unreal's component space (cm)."""
    return np.array([-p[1], -p[0], p[2]]) * 100.0


def hit_zones(rig, V, faces, W, names):
    """A convex hull per bone round the faces it carries most, and the coal's sphere. A face counts for every bone that
    carries one of its corners most, so neighbors overlap a face's width."""
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
        hull = make_hull(rig, bone, pts)
        HITS['planes'][bone] = convex_planes(hull)
        made += 1
    coal = V[owner == 'coal']
    center = np.array(rig.data.bones['coal'].head_local)
    reach = np.linalg.norm(coal - center, axis=1).max()
    if reach > COAL_REACH:
        raise RuntimeError(f'the coal reaches {reach:.3f} m from its middle, past its hit sphere')
    lm.hit_sphere(rig, 'coal', tuple(center), COAL_REACH)
    HITS['coal'] = center
    uncovered = ~np.isin(owner, HULL_BONES + ('coal',))
    others = sorted(set(owner[uncovered].tolist()) - set(NO_HULL))
    if others:
        raise RuntimeError(f'vertices on {others} have no hit zone')
    HITS['faces'] = faces
    HITS['keep'] = [(~uncovered[F]).any(1) for F in faces]
    HITS['owner'] = owner
    S, who = face_samples(V, faces, HITS['keep'], owner)
    out = outside_hits(S)
    gaps = int(out.sum())
    if gaps:
        where = ', '.join(f'{b} {n}' for b, n in Counter(who[out].tolist()).most_common(6))
        P = S[out]
        far = np.min([np.max(P @ n.T - d, axis=1) for b, (n, d) in HITS['planes'].items()], axis=0)
        idx = nearest(V, P[np.argsort(-far)[:5]])
        names_at = [next((k for k, ranges in LAYOUT.items() if isinstance(ranges, list) and k not in ('head_faces',)
                          and any(a <= i < b for a, b in ranges)), '?') for i in idx]
        log(f'  outside: nearest parts {names_at}')
        worst = P[np.argmax(far)]
        per = sorted((float(np.max(worst @ n.T - d)), b) for b, (n, d) in HITS['planes'].items())[:3]
        n_, d_ = HITS['planes']['skirt_f_01']
        viol = worst @ n_.T - d_
        log(f'  worst {np.round(worst, 4)}: nearest hulls {per}; skirt_f_01 planes violated: {int((viol > 0.004).sum())} of '
            f'{len(d_)}, normals there {np.round(n_[viol > 0.004][:3], 3)}')
        raise RuntimeError(f'{gaps} of {len(S)} points on faces with a hit zone lie outside every zone ({where}), up '
                           f'to {1000.0 * far.max():.1f} mm, e.g. at {np.round(P[np.argmax(far)], 3)}')
    log(f'hit zones: {made} hulls and the coal sphere ({100 * COAL_REACH:.1f} cm); every face but the shroud strips\' '
        f'lies inside one ({int(uncovered.sum())} vertices left out)')
    return made + 1


def hat_gain():
    """How much darker the straw's grain (T_Hay_BC) draws a zone than the body's (T_Polymer_BC) under M_Ghost's grain
    (its brightness x 1.25, clamped 0.5..1.5), averaged over each texture: the hat's zone colors are set lighter by it."""
    out = []
    for name in ('Hay', 'Polymer'):
        img = bpy.data.images.load(lt.texture_path(name, 'BC'), check_existing=True)
        px = np.empty(img.size[0] * img.size[1] * 4, np.float32)
        img.pixels.foreach_get(px)
        px = px.reshape(-1, 4)
        luma = px[:, 0] * 0.299 + px[:, 1] * 0.587 + px[:, 2] * 0.114
        out.append(float(np.clip(luma * 1.25, 0.5, 1.5).mean()))
    return out[0] / out[1]


def build():
    rig, body, mats, raw_colors, (V, faces, W, names) = build_rig()
    hat = build_hat(mats)
    fork = build_fork(mats)
    hit_zones(rig, V, faces, W, names)
    islands, holes, largest = shroud_islands(LAYOUT['strips'])
    # The strips fade as the Unpaid's do, so M_Ghost's noise breaks them up toward their tips: a measure, not a fault.
    log(f'shroud: over a whole period of the fade noise\'s drift, {islands} bits break off and {holes} holes open in the '
        f'fading strips' + (f' (the largest {1000.0 * largest:.0f} mm across)' if islands or holes else ''))
    log('shroud fade (vertex A) down the tube: ' + ', '.join(
        f'{float(shroud_fade(cm / 100.0 / SHROUD_LENGTH)):.2f} at {cm} cm'
        for cm in (0, 2, 10, 20, 25, int(100 * SHROUD_SPLIT * SHROUD_LENGTH)))
        + f', then down each strip to 0 by {100 * STRIP_GONE:.0f}% of its length')
    log(f'hat: the Hay grain draws {hat_gain():.3f} as bright as the Polymer grain under M_Ghost')
    return rig, body, hat, fork, raw_colors


RIG, BODY, HAT, FORK, RAW_COLORS = build()
PROPS = {'hat': (HAT, HAT_PIVOT), 'fork': (FORK, ARMS[-1.0]['G'])}


# --- The fence, as the game builds it: what the lean and the sit are fitted to ---

def model_source(path, functions):
    """A model script's builders, without building its models: its source up to its models list, run in a namespace."""
    source = open(path, encoding='utf-8').read()
    source = source[:source.index('\nmodels = [')]
    ns = {'__name__': 'amos_fence', '__file__': path}
    exec(compile(source, path, 'exec'), ns)
    return [ns[f] for f in functions]


def fence_probe():
    """Fences.py's FenceRail (seed 10), built as the game's model is, as a BVH tree in its own space (the rails along +X
    from its post at 0 to its posts at 1.5 and 3, the fence's line y = 0); the built objects are deleted again, so the
    export never sees them. Logs how the measured fence compares with the constants above."""
    from mathutils.bvhtree import BVHTree
    fence_rail, = model_source(os.path.join(lt.REPO, 'Art', 'Models', 'Props', 'Fences.py'), ('fence_rail',))
    obj = fence_rail('_AmosFenceProbe', 10)
    V = np.array([v.co[:] for v in obj.data.vertices])
    polys = [tuple(p.vertices) for p in obj.data.polygons]
    for o in list(obj.children_recursive) + [obj]:
        data = o.data
        bpy.data.objects.remove(o, do_unlink=True)
        if isinstance(data, bpy.types.Mesh) and data.users == 0:
            bpy.data.meshes.remove(data)
    bvh = BVHTree.FromPolygons([tuple(v) for v in V], polys)

    def ray(origin, direction):
        hit = bvh.ray_cast(Vector(origin), Vector(direction), 5.0)
        return np.array(hit[0]) if hit[0] is not None else np.full(3, np.nan)
    top = ray((POST_X, 0.0, 2.0), (0.0, 0.0, -1.0))[2]
    under = ray((POST_X, 0.0, 0.0), (0.0, 0.0, 1.0))[2]
    front = ray((POST_X, -1.0, top - 0.03), (0.0, 1.0, 0.0))[1]
    back = ray((POST_X, 1.0, top - 0.03), (0.0, -1.0, 0.0))[1]
    post = ray((0.0, 0.0, 3.0), (0.0, 0.0, -1.0))[2]
    # The post on his right when he leans (at x = 0): its face toward him (+y) a little under its top, and its top there.
    post_back = ray((0.04, 1.0, post - 0.06), (0.0, -1.0, 0.0))[1]
    post_edge = ray((0.04, post_back - 0.008, 3.0), (0.0, 0.0, -1.0))[2]
    log(f'the fence (FenceRail, seed 10) at the middle of its span: the top rail\'s top {top:.3f} m, {back - front:.3f} '
        f'deep 3 cm under it (its middle at y {0.5 * (front + back):+.3f}); the bottom rail\'s underside {under:.3f}; the '
        f'post {post:.3f} high, its back face at y {post_back:+.3f}, {post_edge:.3f} high there (the poses take the top '
        f'rail at {RAIL_TOP})')
    return bvh, dict(post_back=float(post_back), post_edge=float(post_edge))


FENCE, FENCE_AT = fence_probe()


def fence_distance(P, frame):
    """Signed distances (m) of rig-space points from the fence's surface (negative inside it), the actor standing at
    frame = (turn about Z in degrees, location in the fence's space); farther than 0.3 m counts as 0.3."""
    turn, at = frame
    W = np.asarray(P, float) @ rot_z(turn).T + np.asarray(at, float)
    out = np.full(len(W), 0.3)
    for i, w in enumerate(W):
        hit = FENCE.find_nearest(Vector(w), 0.3)
        if hit[0] is not None:
            out[i] = hit[3] if (Vector(w) - hit[0]).dot(hit[1]) >= 0.0 else -hit[3]
    return out


# Where the actor stands for each fence pose, in the fence's space (the posts at x = 0 and 1.5 either side of him):
# leaning, square to the fence, LEAN_BACK behind its line; sitting, on its line, turned to his right (toward the sun
# going down the Sundown Road, as the concept sat him).
SIT_TURN = -25.0
LEAN_FRAME = (0.0, (POST_X, LEAN_BACK, 0.0))
SIT_FRAME = (SIT_TURN, (POST_X, 0.0, 0.0))


# --- Poses: what the game's code makes of the rig, solved here bone by bone ---
# A pose turns bones about their joints in the rest pose's axes, each under its parent's turn (AUnpaidCreature's
# Turned = Above * Own), and may shift a bone's head off where its parent carries it. The arms are placed either by
# where the fist holds (the elbow found as the game's two-bone solve finds a knee) or by where the elbow and the wrist
# go (lying on the rail). The shroud's chain can follow the knees (skirt_f_02) where the thighs swing, and its links can
# be aimed. The fork rides the right fist unless a pose sets it down (place).

REST = {b.name: (np.array(b.head_local), np.array(b.matrix_local.to_3x3()), b.parent.name if b.parent else None)
        for b in RIG.data.bones}
ORDER_RIG = [b.name for b in RIG.data.bones]
# Parents before children, and the knees (skirt_f_01, skirt_f_02) before the shroud that can follow them.
ORDER = sorted((b.name for b in RIG.data.bones), key=lambda n: (n.startswith('tail_'), ORDER_RIG.index(n)))


def bone_tip(name):
    return np.array(RIG.data.bones[name].tail_local) - REST[name][0]


def turn_between(a, b):
    """The least turn taking direction a to direction b."""
    a, b = unit(a), unit(b)
    axis = np.cross(a, b)
    s = np.linalg.norm(axis)
    if s < 1e-9:
        return np.eye(3)
    return rotation(axis, math.atan2(s, float(np.dot(a, b))))


def arm_turns(side, S1, arm):
    """The whole turns of the upper arm, forearm and hand: the fist on its grip (the elbow by the two-bone solve toward
    the pole), or the elbow and the wrist where the pose puts them (kept at the arm's lengths)."""
    A = ARMS[side]
    f1, u1 = unit(arm['f']), unit(arm['u'])
    u1 = unit(u1 - f1 * np.dot(u1, f1))
    L1, L2 = np.linalg.norm(A['E'] - A['S']), np.linalg.norm(A['W'] - A['E'])
    if 'E' in arm:
        E1 = S1 + unit(np.asarray(arm['E'], float) - S1) * L1
        W1 = E1 + unit(np.asarray(arm['W'], float) - E1) * L2
    else:
        W1 = wrist_for(arm['grip'], f1, u1)
        E1, W1, reached = two_bone(S1, W1, L1, L2, arm['pole'])
        if not reached:
            log(f'  the {SUFFIX[side]} fist falls {1000 * np.linalg.norm(W1 - wrist_for(arm["grip"], f1, u1)):.0f} mm short')
    n0 = unit(np.cross(A['E'] - A['S'], A['W'] - A['E']))
    n1 = np.cross(E1 - S1, W1 - E1)
    n1 = unit(n1) if np.linalg.norm(n1) > 1e-6 else n0

    def bone_frame(v, n):
        return frame(v, np.cross(n, unit(v)))
    R_ua = bone_frame(E1 - S1, n1) @ bone_frame(A['E'] - A['S'], n0).T
    R_la = bone_frame(W1 - E1, n1) @ bone_frame(A['W'] - A['E'], n0).T
    R_h = frame(f1, u1) @ frame(A['f'], A['u']).T
    return R_ua, R_la, R_h


def solve(spec):
    """Every bone's whole turn (3x3, Blender axes) and joint (its head) for a pose."""
    own, totals, shift = spec.get('own', {}), dict(spec.get('total', {})), spec.get('shift', {})
    aims, follow = spec.get('aim', {}), spec.get('follow', {})
    T, J = {}, {}
    for name in ORDER:
        head, _, parent = REST[name]
        if parent is not None:
            above = T[parent]
            joint = J[parent] + above @ (head - REST[parent][0])
        else:
            above = np.eye(3)
            joint = head.copy()
        if name in follow:
            # Carried by another bone than its parent (the shroud by the knees): its head where that bone carries it,
            # turning as that bone turns. In the table this is a shift and a turn like any other.
            other = follow[name]
            joint = J[other] + T[other] @ (head - REST[other][0])
            above = T[other]
        joint = joint + np.asarray(shift.get(name, (0.0, 0.0, 0.0)), float)
        side = 1.0 if name.endswith('_l') else -1.0
        if name.startswith('upperarm_') and side in spec.get('arms', {}):
            R_ua, R_la, R_h = arm_turns(side, joint, spec['arms'][side])
            totals[name], totals[f'lowerarm_{SUFFIX[side]}'], totals[f'hand_{SUFFIX[side]}'] = R_ua, R_la, R_h
        turned = totals[name] if name in totals else above @ own.get(name, np.eye(3))
        if name in aims:
            turned = turn_between(turned @ bone_tip(name), aims[name]) @ turned
        if name in spec.get('roll', {}):
            # Rolled about its own length (degrees): a shroud link turning its flat side away.
            turned = rotation(unit(turned @ bone_tip(name)), math.radians(spec['roll'][name])) @ turned
        if name in spec.get('place', {}):
            # Let go of: a prop's bone set down where the pose puts it (its head and whole turn), off its hand.
            joint, turned = (np.asarray(v, float) for v in spec['place'][name])
        T[name], J[name] = turned, joint
    return T, J


def fork_place(butt, toward, front=(0.0, -1.0, 0.0)):
    """The fork stood with its butt at butt and its shaft running toward a point: the 'fork' bone's head (FORK_GRIP up
    the shaft) and its whole turn from rest."""
    A = ARMS[-1.0]
    R_rest, _ = fork_frame(A['G'], A['rod'])
    butt = np.asarray(butt, float)
    rod = unit(np.asarray(toward, float) - butt)
    R_new, _ = fork_frame(butt + rod * FORK_GRIP, rod, front)
    return butt + rod * FORK_GRIP, R_new @ R_rest.T


# The lean on the fence: he stands behind it (his origin LEAN_BACK behind its line, the posts POST_X to his left and
# right), bent at the hips over the top rail, his arms folded on it, right over left: both forearms lie on the rail;
# the left hand rests on it, the fingers curled over its far edge, and the right forearm crosses over the left wrist
# toward the far side, rising from its elbow no more than it must (at most max_rise, then the elbow comes off the rail),
# its loose fist hanging over the far edge beside the left hand. His head lifts to Ellis on the far side. The thighs
# stay upright under the bent body (skirt_f_01 turns back as far as the pelvis turns forward), and the shroud hangs
# from the knees as at rest. The fork leans on the post on his right.
LEAN = dict(pelvis=32.0, spine_01=12.0, spine_02=10.0, neck=-28.0, head=-32.0)
FOREARM_UNDER = 0.043                    # the forearm's skin under its bone at the elbow
# Each arm: the elbow's place behind the fence's line (m), the forearm's way from the elbow (x toward his middle, then
# y; the right one's rise is fitted, up to max_rise m a meter), the hand's frame (fingers f, back of the hand u). The
# fit sets each elbow's height.
LEFT_ARM = dict(back=0.03, way=(1.0, -0.08), f=(-0.7, -0.55, -0.35), u=(0.0, -0.35, 0.94))
RIGHT_ARM = dict(back=0.04, way=(1.0, -0.5), f=(0.7, -0.6, -0.4), u=(0.45, -0.75, 0.45), max_rise=0.25)
FORK_BUTT = (-POST_X + 0.12, -LEAN_BACK + 0.24)                        # the leaning fork's butt on the ground
FORK_TOUCH = (-POST_X + 0.04, FENCE_AT['post_edge'] - 0.006)          # where its shaft rests on the post's back edge
BUTT_DOME = 0.015                        # the shaft's rounded butt reaches this far under its end


def lean_spec(fit=(0.0, 0.0, 0.2), fork_off=0.0):
    """The lean: fit lifts the left and right elbows (m) to lay the forearms on the rail and gives the right forearm's
    rise over the left hand (m a meter), as the fit measured them; fork_off moves the fork's shaft off the post (m)."""
    p = LEAN
    spec = dict(own={'pelvis': rot_x(p['pelvis']), 'spine_01': rot_x(p['spine_01']), 'spine_02': rot_x(p['spine_02']),
                     'skirt_f_01': rot_x(-p['pelvis']), 'neck': rot_x(p['neck']), 'head': rot_x(p['head'])},
                follow={'tail_01': 'skirt_f_02'})
    T, J = solve(spec)
    y0 = -LEAN_BACK
    arms = {}
    for k, s in enumerate(SIDES):
        S = J[f'upperarm_{SUFFIX[s]}']
        a = LEFT_ARM if s > 0 else RIGHT_ARM
        ye, ze = y0 + a['back'], RAIL_TOP + FOREARM_UNDER + fit[k]
        # The elbow out from the shoulder, at the upper arm's length from it (as low as the arm reaches, if the fit asks
        # for lower: the report then shows the forearm off the rail).
        rest = max(UPPER_ARM ** 2 - (ye - S[1]) ** 2 - (ze - S[2]) ** 2, 1e-8)
        E = np.array([S[0] + s * math.sqrt(rest), ye, ze])
        way = np.array([-s * a['way'][0], a['way'][1], 0.0 if s > 0 else fit[2]])
        arms[s] = dict(E=E, W=E + unit(way) * FOREARM, f=a['f'], u=a['u'])
    spec['arms'] = arms
    butt = np.array([FORK_BUTT[0], FORK_BUTT[1], BUTT_DOME])
    touch = np.array([FORK_TOUCH[0], FENCE_AT['post_back'] - LEAN_BACK + 0.015 + fork_off, FORK_TOUCH[1]])
    spec['place'] = {'fork': fork_place(butt, touch)}
    return spec


# The sit on the fence: his origin on the ground on the fence's line, turned to his right (SIT_FRAME); his seat on the
# top rail (the pelvis lifted until the trousers rest on it), the thighs forward as in Abel's sit; the fork upright in
# his right fist by his knee, its butt on the ground; his left hand on his thigh; his head turned on to the sun, low
# down the road. The shroud is a trail the evening air draws behind him, not a curtain: the knees bend back, the shroud
# twists narrow under them (its links rolled, so its flat width turns away from the front), falls back under his
# thighs and passes between the rails, its tips behind the fence; the side strips drawn in toward the middle, the left
# curling down shorter, the right trailing longer.


def pitched(back, side=0.0):
    """A shroud link's way: pitched back from straight down by back degrees (toward +Y, behind him), leaning side
    (x a meter, + to his left)."""
    b = math.radians(back)
    return (side, math.sin(b), -math.cos(b))


SIT = dict(spine_01=4.0, spine_02=4.0, neck_yaw=-15.0, head_yaw=-22.0, head_pitch=-3.0, thigh=-88.0, knee=40.0)
SIT_AIMS = {'tail_01': pitched(46.0), 'tail_02': pitched(42.0), 'tail_03': pitched(60.0), 'tail_04': pitched(62.0),
            'tail_05': pitched(45.0), 'tail_l_01': pitched(56.0, -0.4), 'tail_l_02': pitched(32.0, -0.2),
            'tail_r_01': pitched(60.0, 0.35), 'tail_r_02': pitched(80.0, 0.12)}
SIT_ROLL = {'tail_01': -45.0, 'tail_02': -25.0}
SIT_GRIP = (-0.27, -0.33, -0.43)          # the right fist round the fork: x, y, and its height under the shoulder
THIGH_HAND = dict(f=(0.05, -0.8, -0.6), u=(0.15, -0.3, 1.0), back=0.15, up=0.075)     # from the left knee's middle


def sit_spec(lift=0.21, back=0.0, palm=0.0):
    """The sit: lift raises the pelvis (m) until the seat rests on the rail, back moves it along his front (m) to put
    the seat's middle over the rail, palm lowers the left hand onto the thigh (m), as the fit measured them."""
    p = SIT
    spec = dict(shift={'pelvis': (0.0, back, lift)},
                own={'spine_01': rot_x(p['spine_01']), 'spine_02': rot_x(p['spine_02']), 'neck': rot_z(p['neck_yaw']),
                     'head': rot_z(p['head_yaw']) @ rot_x(p['head_pitch'])},
                total={'skirt_f_01': rot_x(p['thigh']), 'skirt_f_02': rot_x(p['knee'])},
                follow={'tail_01': 'skirt_f_02'}, aim=dict(SIT_AIMS), roll=dict(SIT_ROLL))
    T, J = solve(spec)
    S = J['upperarm_r']
    G = np.array([SIT_GRIP[0], SIT_GRIP[1], S[2] + SIT_GRIP[2]])
    butt = np.array([G[0] - 0.02, G[1] - 0.12, BUTT_DOME])
    rod = unit(G - butt)
    f, u = rod_hand(rod, (0.15, -1.0, 0.0), -1.0)
    knee = J['skirt_f_02'] + T['skirt_f_02'] @ np.array([0.1, 0.0, 0.0])
    fl, ul = unit(np.array(THIGH_HAND['f'])), unit(np.array(THIGH_HAND['u']))
    Wl = knee + np.array([0.0, THIGH_HAND['back'], THIGH_HAND['up'] - palm])
    spec['arms'] = {-1.0: dict(grip=G, f=f, u=u, pole=(-1.0, 0.4, -0.4)),
                    1.0: dict(grip=grip_of(Wl, fl, ul), f=fl, u=ul, pole=(1.0, 0.3, -0.2))}
    spec['place'] = {'fork': fork_place(butt, G)}
    return spec


def quat_ue(R):
    """A Blender-axes rotation as Unreal's FQuat (X, Y, Z, W)."""
    q = Matrix(np.asarray(R).tolist()).to_quaternion()
    return np.array([q.y, q.x, -q.z, q.w])


def axis_angle(q):
    x, y, z, w = q
    angle = 2.0 * math.degrees(math.acos(max(-1.0, min(1.0, w))))
    s = math.sqrt(max(1.0 - w * w, 0.0))
    axis = np.array([x, y, z]) / s if s > 1e-6 else np.array([0.0, 0.0, 1.0])
    return axis, angle


def apply(spec):
    """Poses the Blender rig as solve() does, and moves the props with their bones."""
    T, J = solve(spec)
    posed = {}
    for bone in RIG.data.bones:
        rest = bone.matrix_local
        world = Matrix.Translation(Vector(J[bone.name])) @ (Matrix(T[bone.name].tolist()) @ rest.to_3x3()).to_4x4()
        posed[bone.name] = world
        if bone.parent:
            basis = (posed[bone.parent.name] @ bone.parent.matrix_local.inverted() @ rest).inverted() @ world
        else:
            basis = rest.inverted() @ world
        pb = RIG.pose.bones[bone.name]
        pb.rotation_mode = 'QUATERNION'
        pb.matrix_basis = basis
    bpy.context.view_layer.update()
    for name, (obj, pivot) in PROPS.items():
        rest = RIG.data.bones[name].matrix_local
        obj.matrix_world = RIG.matrix_world @ RIG.pose.bones[name].matrix @ rest.inverted() @ Matrix.Translation(Vector(pivot))
    bpy.context.view_layer.update()
    return T, J


def bone_transforms():
    """Each bone's rest-to-pose transform (4x4, rig space) as the rig stands now."""
    return {b.name: np.array(RIG.pose.bones[b.name].matrix @ b.matrix_local.inverted()) for b in RIG.data.bones}


def posed_vertices():
    """SK_Amos's vertices as the rig poses them now (rig space)."""
    ev = BODY.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m = ev.to_mesh()
    P = np.empty(len(m.vertices) * 3)
    m.vertices.foreach_get('co', P)
    ev.to_mesh_clear()
    return P.reshape(-1, 3)


def part_index(*names):
    return np.concatenate([np.arange(a, b) for name in names for a, b in LAYOUT[name]])


def part_bvh(P, *names):
    """A BVH tree of the posed faces of these parts."""
    from mathutils.bvhtree import BVHTree
    keep = np.zeros(len(P), dtype=bool)
    keep[part_index(*names)] = True
    polys = [tuple(p.vertices) for p in BODY.data.polygons if keep[p.vertices[0]]]
    return BVHTree.FromPolygons([tuple(v) for v in P], polys)


def surface_distance(bvh, Q, reach=0.2):
    """Signed distances (m) of points from a posed surface (negative inside it); farther than reach counts as reach."""
    out = np.full(len(Q), reach)
    for i, q in enumerate(Q):
        hit = bvh.find_nearest(Vector(q), reach)
        if hit[0] is not None:
            out[i] = hit[3] if (Vector(q) - hit[0]).dot(hit[1]) >= 0.0 else -hit[3]
    return out


def fork_samples(step=0.01):
    """SM_AmosFork's surface as points at rest (rig space): its vertices and points along its edges a centimeter apart
    (the shaft's edges run 0.6 m between rings, so its vertices alone would miss the post)."""
    V = np.array([(FORK.matrix_world @ v.co)[:] for v in FORK.data.vertices])
    E = np.array([tuple(e.vertices) for e in FORK.data.edges])
    out = [V]
    for a, b in E:
        n = int(np.linalg.norm(V[b] - V[a]) / step)
        if n > 1:
            t = np.arange(1, n)[:, None] / n
            out.append(V[a] + (V[b] - V[a]) * t)
    return np.vstack(out)


FORK_REST = fork_samples()


def fork_vertices(T, J):
    """SM_AmosFork's surface points where the pose puts its bone (rig space)."""
    return (FORK_REST - REST['fork'][0]) @ T['fork'].T + J['fork']


def arm_split():
    """Per side, SK_Amos's vertices of the Hand part (the bare forearm and the hand): the forearm's (before the wrist)
    and the hand's."""
    V = np.array([v.co[:] for v in BODY.data.vertices])
    out = {}
    for s in SIDES:
        idx = part_index(f'Hand_{SUFFIX[s]}')
        A = ARMS[s]
        along = (V[idx] - A['W']) @ unit(A['W'] - A['E'])
        out[s] = (idx[along < -0.01], idx[along >= -0.01])
    return out


ARM_SPLIT = arm_split()


def fit_lean():
    """Lays both forearms on the rail (each arm's lowest skin 2 mm over the rail's surface, nowhere into it), raises the
    right forearm over the left wrist just enough to keep 2 mm between them (past max_rise, by lifting its elbow off
    the rail instead), and rests the fork's shaft on the post (1.5 mm off it); measured on the posed mesh against the
    real fence."""
    fit = [0.0, 0.0, 0.2]
    cap = RIGHT_ARM['max_rise']
    for _ in range(3):
        for _ in range(3):
            apply(lean_spec(tuple(fit)))
            P = posed_vertices()
            for k, s in enumerate(SIDES):
                rail = fence_distance(P[part_index(f'Hand_{SUFFIX[s]}')], LEAN_FRAME).min()
                if s < 0 and fit[2] >= cap - 1e-6:
                    # Risen as far as it may: the right elbow comes off the rail instead, onto the left arm.
                    rail = min(rail, surface_distance(part_bvh(P, 'Hand_l'), P[part_index('Hand_r')], reach=0.03).min())
                fit[k] += float(np.clip(0.002 - rail, -0.04, 0.04))
        lo, hi = -0.1, cap
        for _ in range(12):
            mid = 0.5 * (lo + hi)
            apply(lean_spec((fit[0], fit[1], mid)))
            P = posed_vertices()
            if surface_distance(part_bvh(P, 'Hand_l'), P[part_index('Hand_r')], reach=0.03).min() < 0.002:
                lo = mid
            else:
                hi = mid
        fit[2] = hi
    lo, hi = -0.06, 0.12
    for _ in range(22):
        mid = 0.5 * (lo + hi)
        T, J = solve(lean_spec(tuple(fit), mid))
        if fence_distance(fork_vertices(T, J), LEAN_FRAME).min() < 0.0015:
            lo = mid
        else:
            hi = mid
    return tuple(round(v, 4) for v in fit), round(hi, 4)


def seat_points(P):
    """The trousers' seat: the posed trousers' vertices within 2 cm of their lowest near his middle."""
    Q = P[part_index('Trousers')]
    near = Q[np.abs(Q[:, 0]) < 0.16]
    return near[near[:, 2] < near[:, 2].min() + 0.02]


def fit_sit():
    """Seats him on the rail: the seat's middle over the rail's line and its lowest skin 3 mm over the rail; the left
    hand on the thigh (2 mm off the trousers)."""
    lift, back, palm = 0.21, 0.0, 0.0
    c, s = math.cos(math.radians(SIT_TURN)), math.sin(math.radians(SIT_TURN))
    for _ in range(5):
        apply(sit_spec(lift, back, palm))
        P = posed_vertices()
        seat = seat_points(P)
        # The seat's middle across the rail: in the fence's space its y; the pelvis moves along his own y.
        fy = float(np.mean(seat[:, 0] * s + seat[:, 1] * c))
        back -= fy / c
        d = fence_distance(P[part_index('Trousers')], SIT_FRAME)
        lift += 0.003 - float(d.min())
        hand = P[ARM_SPLIT[1.0][1]]
        palm += float(surface_distance(part_bvh(P, 'Trousers'), hand).min()) - 0.002
    return round(lift, 4), round(back, 4), round(palm, 4)


LEAN_FIT, FORK_OFF = fit_lean()
SIT_FIT = fit_sit()
log(f'fitted: the lean\'s elbows lifted {1000 * LEAN_FIT[0]:+.0f} / {1000 * LEAN_FIT[1]:+.0f} mm (left / right), the right '
    f'forearm rising {LEAN_FIT[2]:.3f} a meter, the fork {1000 * FORK_OFF:+.0f} mm off the post; the sit\'s pelvis lifted {1000 * SIT_FIT[0]:.0f} mm and moved '
    f'{1000 * SIT_FIT[1]:+.0f} mm back, the left palm {1000 * SIT_FIT[2]:+.0f} mm down')
POSES = {'idle': {}, 'lean': lean_spec(LEAN_FIT, FORK_OFF), 'sit': sit_spec(*SIT_FIT),
         'talk': dict(own={'jaw': rot_x(TALK_JAW)})}


def rotator_ue(q):
    """FQuat (X, Y, Z, W) as FRotator (Pitch, Yaw, Roll), as FQuat::Rotator works it out."""
    x, y, z, w = q
    test = z * x - w * y
    yaw = math.degrees(math.atan2(2.0 * (w * z + x * y), 1.0 - 2.0 * (y * y + z * z)))
    if abs(test) > 0.4999995:
        pitch = math.copysign(90.0, test)
        roll = (math.copysign(1.0, test) * yaw - 2.0 * math.degrees(math.atan2(x, w)) + 180.0) % 360.0 - 180.0
    else:
        pitch = math.degrees(math.asin(2.0 * test))
        roll = math.degrees(math.atan2(-2.0 * (w * x + y * z), 1.0 - 2.0 * (x * x + y * y)))
    return pitch, yaw, roll


def quat_mul(a, b):
    """FQuat's A * B (B first, then A), (X, Y, Z, W)."""
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return np.array([aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
                     aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz])


def speaker_point():
    """SOCKET_Speaker in the rest pose: rig space (m), and from the head bone's head in Unreal's axes (cm). Read off the
    hat's own space (the hat sits unturned at HAT_PIVOT in the rest pose), whatever pose the rig was left in."""
    sock = next(c for c in HAT.children if c.name.startswith('SOCKET_Speaker'))
    p = HAT_PIVOT + np.array(sock.matrix_local.translation)
    return p, to_ue(p - REST['head'][0])


def pose_table():
    """Every pose's bones in Unreal's component space: rest head, posed head (cm at size 1), the whole turn from rest
    and the bone's own turn under its parent's, as FQuat (X, Y, Z, W). Logged and written as JSON."""
    out = {}
    for name, spec in POSES.items():
        T, J = solve(spec)
        bones = {}
        for bone in ORDER:
            parent = REST[bone][2]
            turn = T[bone]
            own = (T[parent].T @ turn) if parent else turn
            if name != 'idle' and np.allclose(turn, np.eye(3), atol=1e-6) and np.allclose(own, np.eye(3), atol=1e-6) \
                    and np.allclose(J[bone], REST[bone][0], atol=1e-6):
                continue
            qt, qo = quat_ue(turn), quat_ue(own)
            bones[bone] = dict(rest=(np.round(to_ue(REST[bone][0]), 2) + 0.0).tolist(),
                               head=(np.round(to_ue(J[bone]), 2) + 0.0).tolist(),
                               turn=(np.round(qt, 5) + 0.0).tolist(), own=(np.round(qo, 5) + 0.0).tolist())
            if name != 'idle' and 'POSE' in ARGV:
                axis, angle = axis_angle(qt)
                oaxis, oangle = axis_angle(qo)
                log(f'POSE {name} {bone}: {tuple(np.round(to_ue(REST[bone][0]), 1))} -> {tuple(np.round(to_ue(J[bone]), 1))}'
                    f', turn {angle:.1f} deg about {tuple(np.round(axis, 3))}, own {oangle:.1f} deg about '
                    f'{tuple(np.round(oaxis, 3))}')
        out[name] = bones
    os.makedirs(WORK, exist_ok=True)
    path = os.path.join(WORK, 'Amos_poses.json')
    sock, from_head = speaker_point()
    lean_at = to_ue(np.array([0.0, -LEAN_BACK, 0.0]))
    head = dict(units='Unreal component space at size 1 (Amos stands at 1.0): cm; x forward, y right, z up',
                quaternions='FQuat (X, Y, Z, W); turn = the whole turn from rest; own = the turn under the parent\'s '
                            '(Turned = Above * Own); in a pose, a bone not listed stays at rest under its parent',
                shift='a bone whose head is off where its parent carries it is shifted (Shift = its head here minus '
                      'that): the pelvis in the sit (lifted onto the rail), and the shroud\'s tail_01 in the lean and '
                      'the sit, which rides the knees (skirt_f_02) there instead of the pelvis',
                fence=f'Fences.py\'s FenceRail (seed 10): the poses are fitted to it at the middle of a span between '
                      f'two posts (1.5 m apart), the top rail\'s top {100 * RAIL_TOP:.1f} cm up; the fence\'s line is '
                      f'the top rail\'s middle',
                lean=f'the actor stands square to the fence facing it, its origin on the ground {100 * LEAN_BACK:.0f} '
                     f'cm behind the fence\'s line (the line at component x {lean_at[0]:.0f}), midway between the '
                     f'posts ({100 * POST_X:.0f} cm to either side); the forearms lie on the top rail, folded right '
                     f'over left; the fork is let go: the fork bone\'s head and whole turn are where it leans on the '
                     f'post on his right (set the bone, or stand a copy of SM_AmosFork there and hide his)',
                sit=f'the actor\'s origin on the ground on the fence\'s line midway between the posts, facing away '
                    f'from the fence and turned {abs(SIT_TURN):.0f} degrees to his right (yaw {SIT_TURN:+.0f}) from '
                    f'square, so his head turned further right looks into the low sun; his seat rests on the top rail; '
                    f'the knees bend back and the shroud\'s links, rolled and pitched as listed, trail back under his '
                    f'thighs and between the rails, the tips behind the fence (its swing, FShroudChain, must not drive '
                    f'them; tail_01 and tail_02 carry a roll about their length); the fork bone is set (it stands on '
                    f'the ground, his '
                    f'fist round it higher up the shaft than at rest)',
                talk=f'the idle with the jaw opened {TALK_JAW:.0f} degrees: RestJaw {JAW_REST:.0f}, speaking '
                     f'PitchBy(0..{TALK_JAW:.0f}) on jaw (the same turn as own here); it layers on any pose',
                speaker=f'SOCKET_Speaker is on SM_AmosHat (the hat rides the head): in the rest pose '
                        f'{tuple(np.round(to_ue(sock), 1))} cm in component space, '
                        f'{tuple(np.round(from_head, 1))} cm from the head bone\'s head')
    lines = ['{'] + [f' {json.dumps(k)}: {json.dumps(v)},' for k, v in head.items()] + [' "poses": {']
    for k, (name, bones) in enumerate(out.items()):
        lines.append(f'  {json.dumps(name)}: {{')
        rows = [f'   {json.dumps(b)}: {json.dumps(v)}' for b, v in bones.items()]
        lines.append(',\n'.join(rows))
        lines.append('  }' + (',' if k < len(out) - 1 else ''))
    lines += [' }', '}']
    with open(path, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('\n'.join(lines) + '\n')
    log(f'pose table: {len(out)} poses written to {os.path.relpath(path, lt.REPO)}')
    return out, path


def check_table(path):
    """The written table read back: every quaternion of unit length; every bone's whole turn its parent's whole turn
    times its own; every listed bone's rest head where the rig has it."""
    data = json.load(open(path, encoding='utf-8'))
    worst_len, worst_chain, worst_rest, count = 0.0, 0.0, 0.0, 0
    for name, bones in data['poses'].items():
        def whole(bone):
            # A bone the table leaves out is at rest: no turn, under a parent with none.
            return np.array(bones[bone]['turn']) if bone in bones else np.array([0.0, 0.0, 0.0, 1.0])
        for bone, row in bones.items():
            for key in ('turn', 'own'):
                worst_len = max(worst_len, abs(np.linalg.norm(row[key]) - 1.0))
            parent = REST[bone][2]
            above = whole(parent) if parent else np.array([0.0, 0.0, 0.0, 1.0])
            got = quat_mul(above, np.array(row['own']))
            want = np.array(row['turn'])
            worst_chain = max(worst_chain, min(np.abs(got - want).max(), np.abs(got + want).max()))
            worst_rest = max(worst_rest, float(np.abs(np.array(row['rest']) - to_ue(REST[bone][0])).max()))
            count += 1
    ok = worst_len < 2e-5 and worst_chain < 5e-5 and worst_rest < 0.006
    log(f'pose table check: {count} rows; quaternion lengths off 1 by at most {worst_len:.1e}; whole = parent x own to '
        f'{worst_chain:.1e}; rest heads to {worst_rest:.3f} cm' + ('' if ok else '  <-- FAILED'))
    if not ok:
        raise RuntimeError('the pose table does not check out')


def pose_gaps(P):
    """How much of the posed surface the hit zones (rigid on their bones, as in the physics asset) miss, and where."""
    S, who = face_samples(P, HITS['faces'], HITS['keep'], HITS['owner'])
    gaps = outside_hits(S, bone_transforms())
    where = ', '.join(f'{b} {n}' for b, n in Counter(who[gaps].tolist()).most_common(3))
    return 100.0 * gaps.mean(), where


def fence_report(name, P, T, J, frame):
    """How he meets the fence in a pose: the forearms or the seat on the rail, every other part clear of it (the
    nearest and the deepest, by part), the shroud clear of the fence and the ground, the fork."""
    d = fence_distance(P, frame)
    by_part = {}
    for part, ranges in LAYOUT.items():
        if part in ('strips', 'head_faces'):
            continue
        idx = np.concatenate([np.arange(a, b) for a, b in ranges])
        by_part[part] = float(d[idx].min())
    touching = ('Hand_l', 'Hand_r') if name == 'lean' else ('Trousers',)
    rest = {k: v for k, v in by_part.items() if k not in touching}
    nearest = min(rest, key=rest.get)
    fork = fence_distance(fork_vertices(T, J), frame)
    shroud = P[part_index('Shroud')]
    fv = fork_vertices(T, J)
    text = ', '.join(f'{k} {1000 * by_part[k]:+.1f} mm' for k in touching)
    log(f'{name}: on the rail: {text}; the nearest other part {nearest} {1000 * rest[nearest]:+.0f} mm off the fence '
        f'(the shroud {1000 * by_part["Shroud"]:+.0f}); the shroud\'s lowest {1000 * shroud[:, 2].min():.0f} mm over the '
        f'ground; the fork {1000 * fork.min():+.1f} mm off the fence, its butt {1000 * fv[:, 2].min():+.1f} mm off the '
        f'ground')
    clipping = {k: v for k, v in by_part.items() if v < -0.0005}
    if clipping:
        log(f'  {name}: INTO THE FENCE: ' + ', '.join(f'{k} {1000 * v:.1f} mm' for k, v in clipping.items()))
    return by_part


def hands_report(name, P, frame):
    """Leaning: each bare forearm's skin over the rail (its hand left out), and how the right arm lies over the left
    (the least signed distance; negative: into it)."""
    gaps = [float(fence_distance(P[ARM_SPLIT[s][0]], frame).min()) for s in SIDES]
    right = P[part_index('Hand_r')]
    d = surface_distance(part_bvh(P, 'Hand_l'), right, reach=0.03)
    log(f'{name}: the bare forearms {1000 * gaps[0]:+.1f} / {1000 * gaps[1]:+.1f} mm over the rail (left / right); the '
        f'right arm over the left {1000 * d.min():+.1f} mm at the nearest, {int((d < 0).sum())} of its vertices inside')


def thigh_report(name, P):
    hand = P[ARM_SPLIT[1.0][1]]
    d = surface_distance(part_bvh(P, 'Trousers'), hand, reach=0.1)
    log(f'{name}: the left hand on the thigh: {1000 * d.min():+.1f} mm at the nearest, {int((d < 0).sum())} of its '
        f'vertices inside')


def shroud_report(name, P):
    """The shroud against his trousers (its top starts inside their hems, so it always dips in a little: compare with
    the idle) and how wide it is, its tube and its strips, against the trousers at the knees."""
    first = LAYOUT['Shroud'][0][0]
    blocks = LAYOUT['strips']['blocks']
    strips = P[np.concatenate([np.arange(first + s, first + s + r * c) for s, r, c in blocks])]
    tube = P[first:first + blocks[0][0]]
    trousers = P[part_index('Trousers')]
    knees = trousers[trousers[:, 2] < trousers[:, 2].min() + 0.25]
    d = surface_distance(part_bvh(P, 'Trousers'), P[part_index('Shroud')], reach=0.03)
    log(f'{name}: the shroud against the trousers {1000 * d.min():+.1f} mm ({int((d < 0).sum())} vertices in); across, '
        f'the tube {tube[:, 0].min():+.3f}..{tube[:, 0].max():+.3f} m and the strips {strips[:, 0].min():+.3f}..'
        f'{strips[:, 0].max():+.3f}, the trousers at the knees {knees[:, 0].min():+.3f}..{knees[:, 0].max():+.3f}')


def log_fork(name):
    head, turn = POSES[name]['place']['fork']
    q = quat_ue(turn)
    pitch, yaw, roll = rotator_ue(q)
    log(f'{name}: the fork bone set at {tuple(np.round(to_ue(head), 2))} cm (component space, size 1), whole turn FQuat '
        f'{tuple(np.round(q, 5))} = FRotator(P {pitch:.2f}, Y {yaw:.2f}, R {roll:.2f})')


TABLE, TABLE_PATH = pose_table()
check_table(TABLE_PATH)
for pose_name, frame_ in (('idle', None), ('lean', LEAN_FRAME), ('sit', SIT_FRAME), ('talk', None)):
    T_, J_ = apply(POSES[pose_name])
    P_ = posed_vertices()
    share, where = pose_gaps(P_)
    log(f'{pose_name}: {share:.1f}% of the surface outside the hit zones' + (f' ({where})' if where else ''))
    if frame_ is not None:
        fence_report(pose_name, P_, T_, J_, frame_)
        log_fork(pose_name)
    if pose_name == 'lean':
        hands_report(pose_name, P_, frame_)
    if pose_name == 'sit':
        thigh_report(pose_name, P_)
    if pose_name in ('idle', 'sit'):
        shroud_report(pose_name, P_)
apply(POSES['idle'])


# --- Previews (Saved/ArtPreviews/RansomsRest/Amos) ---

def want_preview():
    return '--preview' in ARGV or lt.want_preview()


def preview_material(name, look, colors, slot_coal=False, rank=EMBER_COLOR, rank_alpha=EMBER_ALPHA, heat=EMBER_HEAT,
                     grain='Polymer'):
    """M_Ghost as Tools/Unreal/build_creature_materials.py builds it, for Eevee: the zone color by vertex R, the grain
    (BaseColorMap: T_Polymer_BC, or T_Hay_BC on the hat) on UV 0 x UVScale, cavity G; emissive the rim
    (1 - facing) ^ RimPower, the glow and the ember edge in the rank's color times Burn (1 + Heat); the fade's two noises
    on UV 0 x NoiseScale against vertex A, dithered. Coal: the crust and its cracks in the rank's color, times Burn. The
    rank color, strength and heat are what the code would set on the actor (banked low)."""
    mat = bpy.data.materials.new(f'Preview{name}')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    burn = max(1.0 + heat, 0.0)

    def math_(op, a, b=None, clamp=False):
        node = nodes.new('ShaderNodeMath')
        node.operation = op
        node.use_clamp = clamp
        for k, v in enumerate((a, b)):
            if v is None:
                continue
            if isinstance(v, (int, float)):
                node.inputs[k].default_value = v
            else:
                links.new(v, node.inputs[k])
        return node.outputs[0]

    def vmath(op, a, b):
        node = nodes.new('ShaderNodeVectorMath')
        node.operation = op
        for k, v in enumerate((a, b)):
            if isinstance(v, (int, float)):
                node.inputs[k].default_value = (v, v, v)
            elif isinstance(v, tuple):
                node.inputs[k].default_value = v[:3]
            else:
                links.new(v, node.inputs[k])
        return node.outputs[0]

    def vscale(v, s):
        node = nodes.new('ShaderNodeVectorMath')
        node.operation = 'SCALE'
        links.new(v, node.inputs[0])
        if isinstance(s, (int, float)):
            node.inputs[3].default_value = s
        else:
            links.new(s, node.inputs[3])
        return node.outputs[0]

    def rgb(color):
        return tuple(lt.hex_color(int(color.lstrip('#'), 16))[:3]) if isinstance(color, str) else \
            tuple(lt.hex_color(color)[:3])

    def image(path, colorspace, scale, offset=(0.0, 0.0)):
        uv = nodes.new('ShaderNodeUVMap')
        mapping = nodes.new('ShaderNodeMapping')
        mapping.inputs['Scale'].default_value = (scale, scale, 1.0)
        mapping.inputs['Location'].default_value = (offset[0], offset[1], 0.0)
        links.new(uv.outputs['UV'], mapping.inputs['Vector'])
        tex = nodes.new('ShaderNodeTexImage')
        tex.image = bpy.data.images.load(path, check_existing=True)
        tex.image.colorspace_settings.name = colorspace
        links.new(mapping.outputs['Vector'], tex.inputs['Vector'])
        return tex.outputs['Color']

    attr = nodes.new('ShaderNodeAttribute')
    attr.attribute_name = 'Ghost'
    sep = nodes.new('ShaderNodeSeparateColor')
    links.new(attr.outputs['Color'], sep.inputs['Color'])
    noise_path = os.path.join(lt.TEXTURE_DIR, 'MacroNoise', 'T_MacroNoise_M.png')
    layer = nodes.new('ShaderNodeLayerWeight')
    layer.inputs['Blend'].default_value = 0.5              # Facing out = 1 - |N.V|
    rank_rgb = vscale(vmath('ADD', rgb(rank), 0.0), rank_alpha * burn)
    if slot_coal:
        bsdf.inputs['Base Color'].default_value = lt.hex_color(0x1d110c)
        bsdf.inputs['Roughness'].default_value = 0.6
        crack = image(noise_path, 'Non-Color', 3.0)
        sep_noise = nodes.new('ShaderNodeSeparateColor')
        links.new(crack, sep_noise.inputs['Color'])
        dev = math_('ABSOLUTE', math_('SUBTRACT', sep_noise.outputs['Red'], 0.5))
        cracks = math_('ADD', 0.25, math_('MULTIPLY', 0.75, math_('SUBTRACT', 1.0, math_('MULTIPLY', dev, 7.0, clamp=True))))
        facing = math_('SUBTRACT', 1.0, layer.outputs['Facing'])
        hot = math_('ADD', 0.45, math_('MULTIPLY', 0.55, facing))
        k = math_('MULTIPLY', math_('MULTIPLY', cracks, hot), look['EmberStrength'] * 1.6)
        links.new(vscale(rank_rgb, k), bsdf.inputs['Emission Color'])
        bsdf.inputs['Emission Strength'].default_value = 1.0
        return mat
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.interpolation = 'CONSTANT'
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.0, (*rgb(colors[0]), 1.0)
    els[1].position, els[1].color = 0.1667, (*rgb(colors[1]), 1.0)
    for position, color in ((0.5, colors[2]), (0.8333, colors[3])):
        e = els.new(position)
        e.color = (*rgb(color), 1.0)
    links.new(sep.outputs['Red'], ramp.inputs['Fac'])
    grain_c = image(lt.texture_path(grain, 'BC'), 'sRGB', look.get('UVScale', 1.0))
    luma = nodes.new('ShaderNodeRGBToBW')
    links.new(grain_c, luma.inputs['Color'])
    grain_v = math_('MINIMUM', math_('MAXIMUM', math_('MULTIPLY', luma.outputs['Val'], 1.25), 0.5), 1.5)
    dark = math_('SUBTRACT', 1.0, math_('MULTIPLY', sep.outputs['Green'], 0.93))
    body = vscale(vscale(ramp.outputs['Color'], grain_v), dark)
    links.new(body, bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = 0.86
    rim = math_('MULTIPLY', math_('POWER', layer.outputs['Facing'], look['RimPower']), look['RimStrength'])
    emit = vscale(vmath('ADD', rgb(look['RimColor']), 0.0), math_('MULTIPLY', rim, dark))
    emit = vmath('ADD', emit, vscale(body, look['GlowStrength']))
    emit = vmath('ADD', emit, vscale(rank_rgb, math_('MULTIPLY', sep.outputs['Blue'], look['EmberStrength'])))
    links.new(emit, bsdf.inputs['Emission Color'])
    bsdf.inputs['Emission Strength'].default_value = 1.0
    n1 = nodes.new('ShaderNodeSeparateColor')
    links.new(image(noise_path, 'Non-Color', look['NoiseScale']), n1.inputs['Color'])
    n2 = nodes.new('ShaderNodeSeparateColor')
    links.new(image(noise_path, 'Non-Color', look['NoiseScale'] * 3.07, (0.37, 0.11)), n2.inputs['Color'])
    N = math_('ADD', math_('MULTIPLY', n1.outputs['Red'], 0.62), math_('MULTIPLY', n2.outputs['Red'], 0.38))
    fade = math_('SUBTRACT', math_('SUBTRACT', math_('MULTIPLY', attr.outputs['Alpha'], 1.3), 0.15), N)
    alpha = math_('ADD', math_('MULTIPLY', fade, 1.0 / look['FadeSoftness']), 0.5, clamp=True)
    links.new(alpha, bsdf.inputs['Alpha'])
    mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = False
    return mat


def preview_look():
    """Swaps the slots for their M_Ghost previews (reading a float copy of the vertex colors)."""
    for obj in (BODY, HAT, FORK):
        if 'Ghost' in obj.data.color_attributes:
            continue
        attr = obj.data.color_attributes.new('Ghost', 'FLOAT_COLOR', 'POINT')
        if obj is BODY:
            rgba = RAW_COLORS
        else:
            src = obj.data.color_attributes['Col']
            loops = np.empty(len(obj.data.loops), dtype=np.int64)
            obj.data.loops.foreach_get('vertex_index', loops)
            per_loop = np.empty(len(obj.data.loops) * 4, np.float32)
            src.data.foreach_get('color', per_loop)
            rgba = np.zeros((len(obj.data.vertices), 4), np.float32)
            rgba[loops] = per_loop.reshape(-1, 4)
        attr.data.foreach_set('color', np.asarray(rgba, np.float32).ravel())
    BODY.data.materials[0] = preview_material('Body', GHOST_LOOK, ZONE_COLORS)
    BODY.data.materials[1] = preview_material('Coal', dict(EmberStrength=2.1), ZONE_COLORS, slot_coal=True)
    HAT.data.materials[0] = preview_material('Hat', HAT_LOOK, HAT_COLORS, grain='Hay')
    FORK.data.materials[0] = preview_material('Fork', FORK_LOOK, FORK_COLORS)


def place(location=(0.0, 0.0, 0.0), turn=0.0):
    """Stands the rig (and so the props, which follow its bones) at location, turned about Z (degrees), at size 1."""
    RIG.matrix_world = Matrix.Translation(Vector(location)) @ Matrix.Rotation(math.radians(turn), 4, 'Z')
    bpy.context.view_layer.update()
    for name, (obj, pivot) in PROPS.items():
        rest = RIG.data.bones[name].matrix_local
        obj.matrix_world = RIG.matrix_world @ RIG.pose.bones[name].matrix @ rest.inverted() @ Matrix.Translation(Vector(pivot))
    bpy.context.view_layer.update()


def place_frame(frame):
    """Stands him for a fence pose in the concept's field: the fence along y = 0 with its posts at x = 0 and 1.5."""
    turn, at = frame
    place(at, turn)


def posed_point(bone, p):
    """A rest-pose point p (rig space) where the posed bone carries it, in the world."""
    return (RIG.matrix_world @ RIG.pose.bones[bone].matrix @ RIG.data.bones[bone].matrix_local.inverted()
            @ Vector(tuple(p)))


def stage_collection(name):
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


def concepts():
    sys.path.insert(0, os.path.join(lt.REPO, 'Art', 'Backlog', 'Creatures'))
    import AmosConcepts as ac
    return ac


def render(path, resolution, samples=64):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    concepts().uc.render(path, resolution, samples=samples)
    return path


def lens_shot(where, path, eye, target, lens, resolution, samples=64, focus=None, fstop=None):
    ac = concepts()
    cam = keep(ac.uc.camera(Vector(eye), Vector(target), lens=lens), where)
    cam.data.clip_end = 8000.0
    cam.data.clip_start = 0.02
    if focus is not None:
        cam.data.dof.use_dof = True
        cam.data.dof.focus_distance = focus
        cam.data.dof.aperture_fstop = fstop
    bpy.context.scene.camera = cam
    render(path, resolution, samples)
    bpy.data.objects.remove(cam)


def grid(paths, out, cols):
    """Panels in a grid (each row as tall as its first panel; panels scaled to the first one's width)."""
    images = [bpy.data.images.load(p, check_existing=False) for p in paths]
    w, h = images[0].size
    rows = (len(images) + cols - 1) // cols
    sheet = np.full((rows * h, cols * w, 4), (0.08, 0.075, 0.07, 1.0), np.float32)
    for k, img in enumerate(images):
        if tuple(img.size) != (w, h):
            img.scale(w, h)
        px = np.empty(w * h * 4, np.float32)
        img.pixels.foreach_get(px)
        r, c = rows - 1 - k // cols, k % cols
        sheet[r * h:(r + 1) * h, c * w:(c + 1) * w] = px.reshape(h, w, 4)
    out_img = bpy.data.images.new('Sheet', cols * w, rows * h, alpha=False)
    out_img.pixels.foreach_set(sheet.ravel())
    out_img.filepath_raw = out
    out_img.file_format = 'PNG'
    out_img.save()
    for img in images + [out_img]:
        bpy.data.images.remove(img)


FIELD = {}


def field_stage():
    """The concept's stage, built once: Whitlock Fields round his fence (Fences.py's FenceRail and FencePost), the
    stubble, the rotting windrows and bales, the uncut hay and the ridges; the golden late afternoon (the sun at 247.5
    degrees, 15 up)."""
    if 'where' not in FIELD:
        ac = concepts()
        where = stage_collection('Field')
        ac.field(where)
        ac.golden_light(where)
        FIELD['where'] = where
    concepts().golden_sky()          # the studio sheets set their own sky
    return FIELD['where']


def shot_lean():
    """Leaning on his fence in the golden afternoon: as Ellis sees him from the path (the concept's camera; beside the
    concept), and from his side and behind, the forearms on the rail."""
    ac = concepts()
    where = field_stage()
    apply(POSES['lean'])
    place_frame(LEAN_FRAME)
    X = ac.AMOS_X
    eye = Vector((X + 1.0, -3.3, 1.52))
    front = os.path.join(WORK, 'lean_front.png')
    lens_shot(where, front, eye, Vector((X + 0.05, 0.0, 1.2)), 40.0, (1600, 1000), samples=96,
              focus=(eye - Vector((X, -0.1, 1.45))).length, fstop=4.0)
    concept = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'Amos', 'Amos_A_lean.png')
    if os.path.exists(concept):
        ac.uc.compose([concept, front], os.path.join(OUT, 'Amos_vs_concept.png'))
    close = os.path.join(WORK, 'lean_close.png')
    eye = Vector((X + 0.45, -1.35, 1.42))
    lens_shot(where, close, eye, Vector((X, -0.36, 1.08)), 40.0, (1600, 1000), samples=96)
    side = os.path.join(WORK, 'lean_side.png')
    eye = Vector((X + 2.7, -0.9, 1.35))
    lens_shot(where, side, eye, Vector((X, -0.2, 1.0)), 40.0, (1600, 1000), samples=96)
    back = os.path.join(WORK, 'lean_back.png')
    eye = Vector((X - 1.3, 2.4, 1.6))
    lens_shot(where, back, eye, Vector((X, -0.15, 1.0)), 40.0, (1600, 1000), samples=96)
    grid([front, close, side, back], os.path.join(OUT, 'Amos_lean.png'), 2)


def shot_sit():
    """Sitting on his fence to wait for the saint, his head turned into the low sun down the Sundown Road: the concept's
    camera, from his left, and from the field behind (the shroud under the bottom rail)."""
    ac = concepts()
    where = field_stage()
    apply(POSES['sit'])
    place_frame(SIT_FRAME)
    X = ac.AMOS_X
    paths = [os.path.join(WORK, f'sit_{k}.png') for k in range(4)]
    eye = Vector((X + 0.6, -3.4, 1.48))
    lens_shot(where, paths[0], eye, Vector((X - 0.05, -0.15, 1.3)), 40.0, (1600, 1000), samples=96,
              focus=(eye - Vector((X, -0.2, 1.5))).length, fstop=4.0)
    eye = Vector((X - 2.2, -2.6, 1.3))
    lens_shot(where, paths[1], eye, Vector((X, -0.15, 1.05)), 40.0, (1600, 1000), samples=96)
    eye = Vector((X + 3.0, -1.2, 1.15))
    lens_shot(where, paths[2], eye, Vector((X, -0.1, 0.85)), 40.0, (1600, 1000), samples=96)
    eye = Vector((X - 1.6, 2.6, 1.45))
    lens_shot(where, paths[3], eye, Vector((X, 0.0, 0.8)), 40.0, (1600, 1000), samples=96)
    grid(paths, os.path.join(OUT, 'Amos_sit.png'), 2)


def pose_jaw(spec, degrees):
    """A pose with the jaw opened (speaking)."""
    spec = dict(spec)
    own = dict(spec.get('own', {}))
    own['jaw'] = rot_x(degrees) @ own.get('jaw', np.eye(3))
    spec['own'] = own
    return spec


def shot_face():
    """His face as Ellis sees it across the fence, leaning on the rail in the low sun (the concept's camera): at rest
    and speaking (the jaw open TALK_JAW)."""
    where = field_stage()
    paths = []
    for name, spec in (('rest', POSES['lean']), ('talk', pose_jaw(POSES['lean'], TALK_JAW))):
        apply(spec)
        place_frame(LEAN_FRAME)
        head = posed_point('head', HEAD_CENTER)
        eye = head + Vector((0.3, -1.0, -0.02)).normalized() * 1.15
        path = os.path.join(WORK, f'face_{name}.png')
        lens_shot(where, path, eye, head + Vector((0.0, 0.0, 0.02)), 85.0, (1000, 1000), samples=96,
                  focus=(head - eye).length - 0.08, fstop=5.6)
        paths.append(path)
    concepts().uc.compose(paths, os.path.join(OUT, 'Amos_face.png'))


UNPAID_ZONES = ('#BAC4C6', '#93A5B2', '#6C5D50', '#A65E4C')   # SK_Unpaid's Ghost_A, as approved in the game


def unpaid_stand_in(where, location, turn):
    """An Unpaid to stand beside him: the approved concept's figure (option D), drawn as the game draws SK_Unpaid, with
    its Ghost_A colors, the same M_Ghost values and a Basic coal."""
    ac = concepts()
    body = preview_material('Unpaid', dict(GHOST_LOOK, EmberStrength=3.0), UNPAID_ZONES, rank=0xb02a18, rank_alpha=1.8,
                            heat=0.0)
    coal = preview_material('UnpaidCoal', dict(EmberStrength=2.1), UNPAID_ZONES, slot_coal=True, rank=0xb02a18,
                            rank_alpha=1.8, heat=0.0)
    for part, options in ac.uc.figure('D', 'idle').parts:
        obj = ac.uc.build(part, coal if options.get('material') == 'coal' else body, where, location=location,
                          turn=turn, **{k: v for k, v in options.items() if k != 'material'})
        lt.box_uv(obj, 'Polymer')


def snapshot(where):
    """Copies of the figure as it stands now (the posed skin baked into a plain mesh, the props)."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(BODY.evaluated_get(depsgraph))
    body = bpy.data.objects.new('AmosCopy', mesh)
    body.matrix_world = BODY.matrix_world.copy()
    where.objects.link(body)
    for obj in (HAT, FORK):
        copy = obj.copy()
        copy.matrix_world = obj.matrix_world.copy()
        where.objects.link(copy)


def hide_figure(hidden=True):
    for obj in (BODY, HAT, FORK):
        obj.hide_render = hidden


def hide_field(hidden=True):
    if 'where' in FIELD:
        FIELD['where'].hide_render = hidden
        bpy.context.view_layer.layer_collection.children[FIELD['where'].name].exclude = hidden


def shot_turnaround():
    """Front, side and back beside a 1.8 m post and an Unpaid (the concept's), orthographic, at size 1, in the golden
    light from the front left."""
    ac = concepts()
    hide_field(True)
    where = stage_collection('Turn')
    apply(POSES['idle'])
    xs = (-0.9, 1.0, 2.7, 4.4)
    ac.uc.post(where, (-2.3, 0.0, 0.0))
    ac.uc.label('1.8 m', (-2.2, -0.05, 1.78), 0.075, align='LEFT', where=where)
    unpaid_stand_in(where, (xs[0], 0.0, 0.0), math.radians(-25.0))
    for x, turn, name in zip(xs[1:], (0.0, 90.0, 180.0), ('front', 'side', 'back')):
        place((x, 0.0, 0.0), turn)
        snapshot(where)
        ac.uc.label(name, (x, -0.8, 0.05), 0.1, where=where)
    hide_figure(True)
    ac.uc.label('an Unpaid', (xs[0], -0.8, 0.05), 0.1, where=where)
    ac.uc.studio(where)
    ac.golden_sky(0.85)
    ac.uc.sun((-0.42, -0.85, 0.36), strength=4.0, color=ac.SUN_COLOR, where=where)
    cam = keep(ac.uc.camera((1.0, -30.0, 1.25 + 30.0 * math.tan(math.radians(7.0))), (1.0, 0.0, 1.25), ortho=8.2), where)
    bpy.context.scene.camera = cam
    render(os.path.join(OUT, 'Amos_turnaround.png'), (2400, 1000), 64)
    bpy.data.objects.remove(cam)
    hide_figure(False)
    strike(where)
    hide_field(False)


def shot_hits():
    """The hit zones over the mesh: a hull per bone and the coal's sphere; front and side at rest, the lean and the
    sit."""
    ac = concepts()
    hide_field(True)
    hulls = [o for o in RIG.children if o.name.startswith(('UCX_', 'USP_'))]
    rest = {o.name: o.matrix_basis.copy() for o in hulls}
    matte = ac.uc.flat_material('HitMatte', 0x8e8a84, 0.9)
    saved = {obj: list(obj.data.materials) for obj in (BODY, HAT, FORK)}
    for obj in saved:
        for k in range(len(obj.data.materials)):
            obj.data.materials[k] = matte
    HAT.hide_render = True
    FORK.hide_render = True
    palette = (0xe6553a, 0xf0a030, 0xd6d040, 0x6cc070, 0x40b8c8, 0x5a7ee0, 0xa060d8, 0xe060a0)
    for k, o in enumerate(sorted(hulls, key=lambda h: h.name)):
        o.hide_render = False
        if 'Wire' not in o.modifiers:
            wire = o.modifiers.new('Wire', 'WIREFRAME')
            wire.thickness = 0.004
            wire.use_even_offset = False
        color = 0xff2a10 if o.name.startswith('USP_') else palette[k % len(palette)]
        o.data.materials.clear()
        o.data.materials.append(ac.uc.flat_material(f'Hit{color:06x}', color, 0.5, 2.0))
    paths = []
    for name, spec, direction in (('front', POSES['idle'], (0.0, -1.0, 0.08)), ('side', POSES['idle'], (1.0, 0.0, 0.08)),
                                  ('lean', POSES['lean'], (0.6, -1.0, 0.2)), ('sit', POSES['sit'], (1.0, -0.4, 0.1))):
        where = stage_collection('Hits')
        ac.uc.ground(where, 0xbdb4a4, plain=True)
        ac.uc.sky(0.8)
        ac.uc.sun((-0.45, -0.85, 0.42), strength=3.8, where=where)
        apply(spec)
        place()
        moves = bone_transforms()
        for o in hulls:
            o.matrix_basis = Matrix(moves[o['Bone']].tolist()) @ rest[o.name]
        target = Vector((0.0, -0.05, 1.05))
        cam = keep(ac.uc.camera(target + Vector(direction).normalized() * 20.0, target, ortho=2.4), where)
        bpy.context.scene.camera = cam
        path = os.path.join(WORK, f'hits_{name}.png')
        render(path, (800, 1000), 32)
        bpy.data.objects.remove(cam)
        paths.append(path)
        strike(where)
    ac.uc.compose(paths, os.path.join(OUT, 'Amos_hitzones.png'))
    for o in hulls:
        o.hide_render = True
        o.matrix_basis = rest[o.name]
    for obj, mats in saved.items():
        for k, m in enumerate(mats):
            obj.data.materials[k] = m
    HAT.hide_render = False
    FORK.hide_render = False
    hide_field(False)


def shot_lods():
    """The LODs the importer will make (50% and 25%), reduced here the same way, with their counts."""
    ac = concepts()
    hide_field(True)
    where = stage_collection('LODs')
    ac.uc.ground(where, 0xbdb4a4, plain=True)
    ac.golden_sky(0.85)
    ac.uc.sun((-0.45, -0.85, 0.42), strength=3.8, color=ac.SUN_COLOR, where=where)
    apply(POSES['idle'])
    place()
    base = sum(len(p.vertices) - 2 for p in BODY.data.polygons)
    copies = []
    for k, share in enumerate((1.0, 0.5, 0.25)):
        copy = BODY.copy()
        where.objects.link(copy)
        if share < 1.0:
            mod = copy.modifiers.new('LOD', 'DECIMATE')
            mod.decimate_type = 'COLLAPSE'
            mod.ratio = share
        copy.location.x += (k - 1) * 1.2
        copies.append(copy)
    hide_figure(True)
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    for k, copy in enumerate(copies):
        m = copy.evaluated_get(depsgraph).to_mesh()
        count = sum(len(p.vertices) - 2 for p in m.polygons)
        copy.evaluated_get(depsgraph).to_mesh_clear()
        ac.uc.label(f'LOD{k}  {count} triangles', ((k - 1) * 1.2, -0.9, 0.05), 0.075, where=where)
        log(f'LOD{k}: {count} triangles' + (' (LOD0)' if k == 0 else f' ({100 * count / base:.0f}%)'))
    cam = keep(ac.uc.camera((0.0, -20.0, 1.0 + 20.0 * math.tan(math.radians(8.0))), (0.0, 0.0, 1.0), ortho=4.2), where)
    bpy.context.scene.camera = cam
    render(os.path.join(OUT, 'Amos_lods.png'), (1800, 1100), 48)
    strike(where)
    hide_figure(False)
    hide_field(False)


def shot_poses():
    """The four poses in the studio light at size 1: the idle, the lean and the sit (fence-less: the fence is in the
    other sheets) and the talk."""
    ac = concepts()
    hide_field(True)
    paths = []
    for name in ('idle', 'lean', 'sit', 'talk'):
        where = stage_collection('Pose')
        ac.uc.ground(where, 0xbdb4a4, plain=True)
        ac.golden_sky(0.85)
        ac.uc.sun((-0.45, -0.85, 0.42), strength=3.8, color=ac.SUN_COLOR, where=where)
        apply(POSES[name])
        place((0.0, 0.0, 0.0), -28.0)
        c = Vector((0.0, -0.2, 1.1))
        path = os.path.join(WORK, f'pose_{name}.png')
        lens_shot(where, path, c + Vector((0.15, -1.0, 0.08)).normalized() * 5.5, c, 50.0, (700, 1000), samples=32)
        paths.append(path)
        strike(where)
    ac.uc.compose(paths, os.path.join(OUT, 'Amos_poses.png'))
    hide_field(False)


# The tour's camera on him (Saved/Screenshots/Tour/AmosSit_Medium.png and AmosLean_Medium.png): on the fields path about
# 1.6 m from the fence, 1.2 m up, level, 90 degrees across; in the concept's field (the fence along y = 0).
GAME_VIEW = ((0.78, -1.57, 1.22), (0.74, 1.0, 1.2), 18.0)
FENCE_VIEW = ((0.75 + 3.2, -0.9, 1.0), (0.75, -0.1, 0.85), 30.0)        # along the fence from his left, past the post


def lod_preview(share):
    """SK_Amos as a reduced LOD draws it (share of its triangles; 1: LOD0): collapsed before the armature, so the reduced
    mesh is skinned as Unreal skins its LOD (Medium draws LOD1, the 50% one)."""
    for mod in [m for m in BODY.modifiers if m.name == 'PreviewLOD']:
        BODY.modifiers.remove(mod)
    if share < 1.0:
        mod = BODY.modifiers.new('PreviewLOD', 'DECIMATE')
        mod.decimate_type = 'COLLAPSE'
        mod.ratio = share
        BODY.modifiers.move(len(BODY.modifiers) - 1, 0)
    bpy.context.view_layer.update()


def shot_game():
    """The sit and the lean from the tour's camera, at LOD0 and at LOD1 (what Medium draws), and the sit along the
    fence."""
    where = field_stage()
    for name, frame in (('sit', SIT_FRAME), ('lean', LEAN_FRAME)):
        apply(POSES[name])
        place_frame(frame)
        paths = []
        for share, lod in ((1.0, 'lod0'), (0.5, 'lod1')):
            lod_preview(share)
            paths.append(os.path.join(WORK, f'game_{name}_{lod}.png'))
            lens_shot(where, paths[-1], *GAME_VIEW, (1280, 720), samples=64)
        if name == 'sit':
            paths.append(os.path.join(WORK, 'game_sit_fence.png'))
            lens_shot(where, paths[-1], *FENCE_VIEW, (1280, 720), samples=64)
            lod_preview(0.5)
            paths.append(os.path.join(WORK, 'game_sit_fence_lod1.png'))
            lens_shot(where, paths[-1], *FENCE_VIEW, (1280, 720), samples=64)
        lod_preview(1.0)
        grid(paths, os.path.join(OUT, f'Amos_game_{name}.png'), 2)


SHOTS = {'compare': shot_lean, 'lean': shot_lean, 'sit': shot_sit, 'face': shot_face, 'turn': shot_turnaround,
         'hits': shot_hits, 'lods': shot_lods, 'poses': shot_poses, 'game': shot_game}


def previews():
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(WORK, exist_ok=True)
    for o in RIG.children:
        if o.name.startswith(('UCX_', 'USP_')):
            o.hide_render = True
    preview_look()
    wanted = [a for a in ARGV if a in SHOTS] or ['turn', 'lods', 'hits', 'poses', 'lean', 'sit', 'face', 'game']
    done = set()
    for name in wanted:
        if SHOTS[name] not in done:
            SHOTS[name]()
            done.add(SHOTS[name])
    apply(POSES['idle'])
    place()


if want_preview():
    previews()
