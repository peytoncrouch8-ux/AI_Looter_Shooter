"""Abel Ransom, the Keeper (Docs/Areas/RansomsRest.md, "The boss"; Docs/Story.md): Ellis's Pa, the area's boss and then
a friend, as the user picked him on 2026-10-06, option A of Art/Backlog/Creatures/AbelConcepts.py, "the Sunday keeper":
the black frock coat he wore to walk the dead to the boards, buttoned to the throat, its skirts closed to the knee; a
white shirt and black string tie; the keeper's bone-white stole with the sun-ring worked at its ends; a flat-crowned,
wide-brimmed hat; a weathered face of about fifty-five, deep-set eyes under heavy brows, a trimmed grey beard. Grief and
dignity, never a monster's hunger. Below the coat a pale shroud sweeps back off him in five wide strips that taper to soft
points: no legs. A rig the game poses in code (on the Unpaid's bones), at 1.3 times: he is built here at 1 and the actor
scales him.

Models:
  SK_Abel         about 17k triangles, LODs at 50% and 25% (no Nanite), skinned smoothly (up to four bones a vertex).
  SM_AbelHat      his hat. Its pivot is the 'hat' bone's head.
  SM_AbelLantern  his ghost lantern (the Keeper's Lantern of Art/Models/Props/BurialDeck.py gone to spirit), hanging from
                  his left fist. Its pivot is the 'lantern' bone's head, the bail's grip; SOCKET_Light at the globe's
                  middle, for the shadowless point light.
  SM_AbelPump     the spectral twin of his Ranchhand pump (Heirloom), in his right fist. Its pivot is the 'gun' bone's
                  head, the stock's wrist; SOCKET_Muzzle at the muzzle, its +X (Unreal) along the bore, where the
                  spectral pellets leave.
Each prop is modeled where he holds it in the rest pose and has no collision: attach it to its bone as the Unpaid's hat
is attached (at the bone, turned by the inverse of the bone's rest rotation), and it rides the bone from there.

Bones (as AUnpaidCreature names them; the armature is Unreal's root). The bind pose is the fight's idle (facing -Y,
Unreal's +X; the origin on the ground; upright; the left forearm across the chest before the coal, the lantern hanging
from the fist; the pump low in the right hand, pointing ahead and down; the shroud down and back):
  pelvis                         the float height (1.0 m); parent of the spine, the shroud and the skirt bones
  spine_01, spine_02             spine_02 is the chest: it carries the arms, the neck and the coal
  coal                           the coal, rigid; its head is the coal's middle (the crit point), its tail the way it faces
  neck, head, jaw, hat           jaw: the lower lip, chin and chin beard, opened by turning it down about its head (at
                                 rest it is closed, the lips meeting); hat: no skin, the hat's attach point
  upperarm_l, lowerarm_l, hand_l and its fingers thumb_01_l, index_01_l, middle_01_l, ring_01_l, pinky_01_l (one bone a
                                 finger, at the knuckle); the same with _r. The hands are fists round the props.
  lantern                        (new) under hand_l: the lantern's attach point at the bail's grip, pointing down
  gun                            (new) under hand_r: the pump's attach point at the stock's wrist, pointing along the bore
  tail_01 .. tail_05             the shroud, down its middle from the pelvis; tail_01 also carries the back of the skirts
  tail_l_01, tail_l_02, tail_r_01, tail_r_02    side chains off tail_02 for the shroud's side strips (two a side)
  skirt_f_01, skirt_f_02         (new) under the pelvis: the front of the coat's skirts from the hips to the knees, and
                                 the front and sides from the knees to the hem. Swung forward they make his lap, so he
                                 kneels and sits on the boards instead of sinking into them.
  skirt_l_01, skirt_r_01         (new) under the pelvis: the sides of the skirts from the hips to the knees, so knelt and
                                 sat the coat's sides lie down on the boards beside the lap. Only those two poses move
                                 them.
The pose table (logged as POSE lines, and written to Intermediate/AbelModel/Abel_poses.json) gives, for the fight idle,
the lantern flare, the shot, the stock lunge, the turn to the sunset, the kneel and the sit on his bier, where each bone's
head goes and its turn from rest, in Unreal's component space (cm at size 1; x forward, y right, z up), both as the whole
turn and as the bone's own turn under its parent's (AUnpaidCreature's Turned = Above * Own).

Vertex colors ('Col', what M_Ghost reads): R the tint zone (0 skin and shroud, 1/3 shirt and his grey beard and hair, 2/3
coat, hat and tie, 1 the stole), G cavity (only where a face has it: the eye sockets and under the brows, the brows, the
nostrils, the mouth's line, the folds from the nose, a few fine lines; the beard's and hair's combing; the coal's
crater), B the ember edge round the coal, A the fade (the shroud's strips: solid down to their tips, cut in their last
6 mm). The rig's FBX carries them as sRGB, so the body stores bytes with these values; the props (textured static
meshes, exported linear) store floats. UV 0: a world-scale box projection for the Polymer set
(1 unit a meter), but on the shroud U the meters round it and V minus the meters down it (Unreal's V runs down the strips).

Materials (M_Ghost's slots; the importer resets each instance and sets these):
  GhostAbel         the body and hat: his four zone colors and the Unpaid's approved rim, glow, ember and fade values
  GhostCoal         the coal, shared with SK_Unpaid (the same values); its color is the Boss rank's gold, set by the code
  GhostAbelLantern  the lantern's ghost brass, iron and grip      GhostAbelFlame  its globe, a pale flame
  GhostAbelPump     the pump's steel, walnut, dark steel and bead

Hit zones (PA_Abel), on the Unpaid's scheme: a convex hull on pelvis, spine_01, spine_02, neck, head, jaw, each upper arm,
forearm and hand (the fist), tail_01, tail_02, skirt_f_01, skirt_f_02, skirt_l_01 and skirt_r_01, round every face with a
corner those bones carry most; a sphere on coal; and a hull on lantern round the lantern. In the idle the left forearm's, fist's and lantern's
hulls stand in front of the coal, so a shot at it hits the arm first (no crit) until he lowers the lantern. The build
checks that every face but the shroud strips' (past tail_02: shots pass through them) lies inside a zone, and measures
how much of the coal the arm covers.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Abel.py -Preview
    ... -ScriptArgs --preview,dusk,face        only some: compare dusk face kneel close sit poses turn hits lods
    ... -ScriptArgs debug                      quick checks into Intermediate/AbelModel
Previews go to Saved/ArtPreviews/RansomsRest/Abel/ (the deck and the stage come from the concept script).
"""
import json
import math
import os
import sys
from collections import Counter

import bpy
import numpy as np
from mathutils import Matrix, Quaternion, Vector, kdtree, noise

import looter_model as lm
import looter_textures as lt

SCALE = 1.3                             # the actor's size: he is built at 1
SIDES = (1.0, -1.0)                     # +1 the left (+X), -1 the right
SUFFIX = {1.0: 'l', -1.0: 'r'}
SKIN, SHIRT, COAT, STOLE = 0, 1, 2, 3   # tint zones (vertex color R = zone / 3)
# Option A's tints: pale skin and shroud (the Unpaid's approved skin), white shirt, black coat, bone stole. The coat is the
# darkest zone and the stole the lightest cloth: a black silhouette with a white band down it.
ZONE_COLORS = ('#BAC4C6', '#DCD8CC', '#3C3B40', '#D3C9AE')
RIM_COLOR = '#DCECEE'
LANTERN_COLORS = ('#CABF9F', '#7D8285', '#9A8774', '#CABF9F')   # ghost brass, iron, the grip's wood
FLAME_COLOR = '#FFF0D6'                                         # a pale, warm white: no rarity color
PUMP_COLORS = ('#B4BCC0', '#A08A72', '#8F979B', '#CFC4AD')      # steel, walnut, dark steel, the bead
BOSS_COAL = 0xffcc00                    # the Boss rank's color (CreatureRankSettings), for the previews only
JAW_REST = 0.0                          # degrees the jaw is open at rest: the lips closed, the mouth a line
TRIANGLES = dict(bodice=1500, sleeve=480, hand=760, head=5600)     # the parts reduced to a budget
OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Abel')
WORK = os.path.join(lt.REPO, 'Intermediate', 'AbelModel')
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]


def log(message):
    print(f'ABEL: {message}', flush=True)


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


# --- The body: a farmer of about 1.85 m, upright, broad in the shoulder (option A's measures) ---

# (z, half width, half depth, y of the middle, superellipse power)
TORSO = [(0.90, 0.165, 0.110, 0.000, 2.2), (0.98, 0.160, 0.106, 0.000, 2.2), (1.06, 0.155, 0.104, -0.004, 2.2),
         (1.16, 0.163, 0.110, -0.008, 2.3), (1.26, 0.178, 0.120, -0.010, 2.4), (1.34, 0.190, 0.124, -0.010, 2.6),
         (1.40, 0.198, 0.118, -0.006, 2.8), (1.45, 0.196, 0.106, -0.002, 3.2), (1.475, 0.184, 0.098, 0.000, 3.4),
         (1.50, 0.155, 0.090, 0.000, 3.0), (1.52, 0.115, 0.080, -0.002, 2.6), (1.54, 0.078, 0.070, -0.004, 2.2)]
# The frock coat: fitted to the waist, its skirts flaring to the knee, the back fuller than the front.
COAT_A = [(0.42, 0.236, 0.186, 0.048, 2.0), (0.55, 0.222, 0.168, 0.036, 2.0), (0.70, 0.205, 0.148, 0.022, 2.1),
          (0.85, 0.184, 0.126, 0.01, 2.2), (0.98, 0.168, 0.113, 0.002, 2.2), (1.06, 0.160, 0.108, -0.004, 2.2)] + \
         [row for row in TORSO if row[0] > 1.06]
SHOULDER = {s: np.array([s * 0.172, -0.006, 1.44]) for s in SIDES}
PIVOT = np.array([0.0, -0.03, 1.62])                      # the top of the neck, where the head turns
HEAD_CENTER = PIVOT + np.array([0.0, -0.012, 0.09])       # head space's origin: mid-head at eye height
HAT_PIVOT = PIVOT + np.array([0.0, 0.0, 0.153])           # the brim's middle: the 'hat' bone's head
COAL_AT = (0.24, 1.33)                                    # the coal: angle round the chest from the front, height
COAL_RADIUS = 0.05
SKIRT_HINGE = np.array([0.0, -0.06, 0.95])                # skirt_f_01's head: the hips, where the lap folds
SKIRT_KNEE = np.array([0.0, -0.13, 0.55])                 # skirt_f_02's head: the knees
SKIRT_HEM = np.array([0.0, -0.15, 0.42])
# skirt_l_01's head and tail (skirt_r_01's mirrored): the side of the hips, down the coat's side to the knee.
SKIRT_SIDE_HIP = np.array([0.17, 0.004, 0.95])
SKIRT_SIDE_KNEE = np.array([0.2, 0.03, 0.55])
SIDE_PIVOTS = {}                                          # each side's pivot for the poses (side_skirt), set by build_coat
SHROUD_PATH = [(0.0, 0.0, 0.92), (0.0, 0.06, 0.6), (0.0, 0.2, 0.36), (0.0, 0.48, 0.2), (0.0, 0.85, 0.12),
               (0.0, 1.25, 0.08)]
SHROUD_SIZE = [(0.0, 0.17, 0.12), (0.25, 0.15, 0.12), (0.5, 0.095, 0.085), (0.75, 0.055, 0.05), (1.0, 0.025, 0.025)]
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
    return pc - nc * 0.01, nc, pc


# --- The arms in the rest pose, built round what the hands hold ---
# Left: the guard. The forearm crosses the chest right before the coal; the fist, at the right of the chest, holds the
# lantern's bail with the grip running front to back (level), the lantern hanging plumb before the belly.
# Right: the pump low at the hip, pointing ahead and down, the stock tucked back under the forearm.

HAND_SCALE = 1.14                         # big working hands
GRIP_ALONG, GRIP_UNDER = 0.075 * HAND_SCALE, 0.03 * HAND_SCALE    # where a fist's grip sits from the wrist


def grip_of(W, f, u):
    """Where a fist (wrist W, fingers' way f, back of the hand u) holds a rod: inside the curled fingers."""
    return np.asarray(W, float) + unit(f) * GRIP_ALONG - unit(u) * GRIP_UNDER


def wrist_for(G, f, u):
    return np.asarray(G, float) - unit(f) * GRIP_ALONG + unit(u) * GRIP_UNDER


def lantern_hand(f, rod_hint):
    """A left fist pointing f with the bail's grip level through it (the lantern hangs plumb), the thumb toward
    rod_hint: (f, back of the hand, the rod's way)."""
    f = unit(f)
    t = unit(np.cross(f, (0.0, 0.0, 1.0)))
    if np.dot(t, rod_hint) < 0.0:
        t = -t
    return f, np.cross(t, f), t


def gun_hand(bore, fingers):
    """A right fist round the pump's wrist, the gun pointing bore, the fingers wrapping from fingers' side: (f, back of
    the hand, bore). The thumb lies toward the muzzle."""
    b = unit(bore)
    f = unit(np.asarray(fingers, float) - b * np.dot(fingers, b))
    return f, np.cross(f, b), b


def rest_arms():
    """Shoulder, elbow, wrist, the hand's frame and the grip, per side, in the rest pose."""
    arms = {}
    # The guard: elbow forward and out, forearm across the coal, rising a little to the fist.
    E, W = np.array([0.265, -0.19, 1.24]), np.array([0.02, -0.27, 1.36])
    f, u, t = lantern_hand(W - E, (0.0, 1.0, 0.0))
    arms[1.0] = dict(S=SHOULDER[1.0], E=E, W=W, f=f, u=u, G=grip_of(W, f, u), rod=t,
                     curls=(1.1, 1.15, 1.15, 1.1))
    # The pump: the bore ahead and 21 degrees down, the fist round its wrist, the wrist cocked down from the forearm.
    bore = unit([0.06, -1.0, -0.4])
    f, u, b = gun_hand(bore, (0.0, 0.0, -1.0))
    E = np.array([-0.28, -0.06, 1.17])
    W = E + 0.272 * unit(f * math.cos(math.radians(52.0)) + b * math.sin(math.radians(52.0)))
    arms[-1.0] = dict(S=SHOULDER[-1.0], E=E, W=W, f=f, u=u, G=grip_of(W, f, u), bore=b,
                      curls=(0.95, 1.0, 1.05, 1.1))
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


def torso_weights(part, z, x=None, shoulders=True, neck=True):
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
        part.weigh(bone, value * (1.0 - arm))
    if shoulders and x is not None:
        for s in SIDES:
            part.weigh(f'upperarm_{SUFFIX[s]}', arm * (np.sign(x) == s))
    return part


def skirt_weights(part, V):
    """The coat below the hips: the front and sides on the skirt chain (thighs to the knee, then to the hem), the back on
    tail_01 with the shroud, all of it on the pelvis at the waist. The sides above the knee are on their own bones
    (skirt_l_01, skirt_r_01), and they bend at the knee from higher up than the front: knelt or sat, their lower part hangs
    from the knee instead of passing under the lap into the boards."""
    theta = np.arctan2(V[:, 0], -V[:, 1])
    z = V[:, 2]
    down = smoothstep(1.0, 0.86, z)
    front = smoothstep(2.3, 1.95, np.abs(theta))
    sides = smoothstep(math.radians(70.0), math.radians(95.0), np.abs(theta))
    knee = np.maximum(smoothstep(0.6, 0.5, z), sides * smoothstep(0.8, 0.66, z))
    side = smoothstep(math.radians(70.0), math.radians(100.0), np.abs(theta))
    thigh = down * front * (1.0 - knee)
    part.weigh('pelvis', 1.0 - down)
    part.weigh('skirt_f_01', thigh * (1.0 - side))
    part.weigh('skirt_l_01', thigh * side * (V[:, 0] >= 0.0))
    part.weigh('skirt_r_01', thigh * side * (V[:, 0] < 0.0))
    part.weigh('skirt_f_02', down * front * knee)
    part.weigh('tail_01', down * (1.0 - front))
    return part


def body_weights(part, V, x=True, shoulders=True):
    """Cloth that runs from the chest down over the skirts (the stole): the torso's weights above the hips, the skirt's
    below."""
    z = V[:, 2]
    below = smoothstep(1.06, 0.98, z)
    upper = Part('_', (V, []), 0)
    torso_weights(upper, z, V[:, 0] if x else None, shoulders=shoulders)
    lower = Part('_', (V, []), 0)
    skirt_weights(lower, V)
    for bone, w in upper.weights.items():
        part.weigh(bone, w * (1.0 - below))
    for bone, w in lower.weights.items():
        part.weigh(bone, w * below)
    return part


def build_coat():
    """The frock coat: the bodice (buttoned to the chest, the V filled by the shirt and tie, the coal burning through
    its left breast), its skirts closed to the knee all round but for the back vent, lapels and collar."""
    parts = []
    center, nc, pc = coal_center()
    tc, zc = COAL_AT
    rng = np.random.default_rng(11)

    # The bodice, from just over the waist seam to the collar, cut for the V, the armholes and the coal's hole.
    theta = np.linspace(-np.pi, np.pi, 200, endpoint=False)
    z = np.linspace(1.025, 1.548, 90)
    th, zz = np.meshgrid(theta, z)
    P = np.array([surface(COAT_A, [zr], theta)[0] for zr in z])
    folds = 0.016 + 0.002 * np.cos(11 * th) * smoothstep(1.1, 1.3, zz) + 0.003 * smoothstep(1.08, 1.03, zz)
    P = P + outward(theta)[None, :, :] * folds[..., None]
    at = np.abs(th)
    sd = np.where(zz > 1.38, (0.1 + 1.7 * (zz - 1.38)) - at, -1.0)                     # the V
    arm = np.sqrt(((at - np.pi / 2 - 0.04) / 0.44) ** 2 + ((zz - 1.405) / 0.1) ** 2)
    sd = np.maximum(sd, (1.0 - arm) * 0.06)                                          # armholes
    sd = np.maximum(sd, zz - 1.548)
    ang = np.arctan2(zz - zc, (th - tc) * 0.17)
    rh = 0.062 * (1.0 + 0.12 * np.sin(ang * 5 + 1.0) + 0.08 * np.sin(ang * 11.0))
    hole = np.hypot((th - tc) * 0.17, zz - zc)
    sd = np.maximum(sd, rh - hole)                                                   # the coal's hole, burnt ragged
    V, F, used = cut(P, sd)
    edge = (hole - rh).ravel()[used]
    ember = np.clip(smoothstep(0.03, 0.0, edge) * (0.5 + 0.5 * pnoise(V, 50.0, 2)), 0.0, 1.0)
    low = reduce((V, F), TRIANGLES['bodice'])
    near = nearest(V, low[0])
    bodice = Part('Bodice', low, COAT, ember=ember[near])
    parts.append(torso_weights(bodice, low[0][:, 2], low[0][:, 0]))

    # The skirts: one panel round from the waist (under the bodice's edge) to the hem, closed in front, the left front
    # edge lapped over the right; open only at the back vent below the seat. Rows are even so the lap folds cleanly.
    rows = 17
    base = np.linspace(-np.pi, np.pi, 65)
    extra = np.array([-0.035, -0.012, 0.004])                 # the front edge: a crisp lap and its shadow
    cols_th = np.unique(np.round(np.concatenate([base, extra]), 6))
    amps, phases = rng.uniform(0.5, 1.0, 5), rng.uniform(0.0, 2.0 * np.pi, 5)
    s = np.linspace(0.0, 1.0, rows)[:, None]
    hem = 0.44 + 0.006 * np.sin(5 * cols_th + 0.4) + 0.008 * np.cos(cols_th)
    zs = 1.065 + (hem[None, :] - 1.065) * s
    # The back vent: its two edges part below the seat, a little more toward the hem.
    gap = np.clip(0.012 + 0.07 * (0.82 - zs), 0.0, None) * (zs < 0.82)
    span = np.pi - gap
    TH = cols_th[None, :] / np.pi * span
    P = np.zeros(zs.shape + (3,))
    for i in range(rows):
        P[i] = surface(COAT_A, zs[i], TH[i])[np.arange(len(cols_th)), np.arange(len(cols_th))]
    f = sum(a * np.cos(k * TH + p) for a, k, p in zip(amps, (5, 7, 9, 11, 13), phases)) / amps.sum()
    deep = smoothstep(1.0, 0.55, zs)
    front_still = smoothstep(0.05, 0.3, np.abs(TH))           # the closed front hangs straight
    lap = 0.003 * (TH >= 0.0) * smoothstep(1.03, 0.98, zs)
    grow = 0.010 + 0.016 * deep * (f - 0.3 * np.abs(f)) * front_still + lap
    P = P + outward(TH) * grow[..., None]
    nrow, ncol = P.shape[:2]
    F = grid_faces(nrow, ncol, closed=False)
    # The hem turned up inside: a narrow band so the skirt's edge has a thickness.
    turn = P[-1] - outward(TH[-1]) * 0.008 + np.array([0.0, 0.0, 0.016])
    Vs = np.vstack([P.reshape(-1, 3), turn])
    band = np.stack([(nrow - 1) * ncol + np.arange(ncol - 1), (nrow - 1) * ncol + np.arange(1, ncol),
                     nrow * ncol + np.arange(1, ncol), nrow * ncol + np.arange(ncol - 1)], 1)
    cav = np.concatenate([(0.42 * gauss(TH + 0.02, 0.012) * smoothstep(1.03, 0.98, zs)).ravel(), np.full(ncol, 0.3)])
    skirt = Part('Skirt', (Vs, [F, band]), COAT, cavity=cav)
    parts.append(skirt_weights(skirt, Vs))
    # Where each side turns in the kneel and the sit (side_skirt): the skirt's point at the coat's side, mid-thigh.
    at = np.degrees(np.arctan2(Vs[:, 0], -Vs[:, 1]))
    for s in SIDES:
        SIDE_PIVOTS[s] = Vs[int(np.argmin(((at - s * 90.0) / 30.0) ** 2 + ((Vs[:, 2] - 0.75) / 0.05) ** 2))].copy()

    # Buttons down the closed front and the two at the back of the waist.
    buttons = Part('Buttons', merge([ellipsoid(surface(COAT_A, [zb], np.array([0.035]), grow=0.02)[0, 0],
                                               (0.0075, 0.0075, 0.0045), np.array([[0, 0, 1.0], [1.0, 0, 0], [0, -1.0, 0]]),
                                               8, 4) for zb in (1.36, 1.29, 1.22, 1.15, 1.08)]
                                    + [ellipsoid(surface(COAT_A, [1.05], np.array([np.pi - s * 0.2]), grow=0.02)[0, 0],
                                                 (0.008, 0.0045, 0.008), segs=8, rings=4) for s in SIDES]),
                   COAT, cavity=0.35)
    parts.append(torso_weights(buttons, buttons.V[:, 2], buttons.V[:, 0]))

    # Lapels laid back along the V, and the turned-down collar.
    lapel_pieces = []
    for s in SIDES:
        zl = np.linspace(1.38, 1.53, 10)
        path = [surface(COAT_A, [zr], np.array([s * (0.1 + 1.7 * (zr - 1.38)) + s * 0.05]), grow=0.024)[0, 0]
                for zr in zl]
        lapel_pieces.append(ribbon(path, [(0.0, 0.006), (0.5, 0.03), (1.0, 0.024)], n=12, cols=3,
                                   out=lambda C: outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.1))
    lapels = Part('Lapels', merge(lapel_pieces), COAT)
    parts.append(torso_weights(lapels, lapels.V[:, 2], lapels.V[:, 0]))
    th = np.linspace(0.0, 2.0 * np.pi, 40, endpoint=False)
    collar = np.array([np.stack([rr * np.sin(th), -0.012 - rr * 0.92 * np.cos(th), np.full_like(th, zr)], -1)
                       for zr, rr in ((1.53, 0.086), (1.556, 0.075), (1.576, 0.066))])
    part = Part('Collar', closed_loft(collar), COAT)
    parts.append(part.weigh('spine_02', 0.7).weigh('neck', 0.3))
    return parts


def build_shirt():
    """The shirt in the V (its placket), its stand collar, the black string tie; the burnt shirt in the coal's hole."""
    parts = []
    th = np.linspace(-0.46, 0.46, 15)
    z = np.linspace(1.36, 1.56, 9)
    P = np.array([surface(TORSO, [zr], th, grow=0.008)[0] for zr in z])
    tz, thz = np.meshgrid(z, th, indexing='ij')
    cav = 0.35 * gauss(thz, 0.03)
    part = Part('ShirtFront', (P.reshape(-1, 3), [grid_faces(len(z), len(th), closed=False)]), SHIRT, cavity=cav.ravel())
    parts.append(torso_weights(part, part.V[:, 2], part.V[:, 0], shoulders=False))
    a = np.linspace(0.0, 2.0 * np.pi, 32, endpoint=False)
    sc = np.array([np.stack([rr * np.sin(a), -0.016 - rr * 0.9 * np.cos(a), np.full_like(a, zr)], -1)
                   for zr, rr in ((1.545, 0.058), (1.565, 0.056), (1.585, 0.055))])
    part = Part('ShirtCollar', closed_loft(sc), SHIRT)
    parts.append(part.weigh('neck', 0.5).weigh('spine_02', 0.5))
    # The string tie: a small bow at the throat, its two ends down the shirt front, all lying on the shirt.
    knot = surface(TORSO, [1.522], np.array([0.0]), grow=0.014)[0, 0]
    tie = [ellipsoid(knot, (0.02, 0.009, 0.014), segs=10, rings=6)]
    for s in SIDES:
        wing = surface(TORSO, [1.52], np.array([s * 0.14]), grow=0.013)[0, 0]
        tie.append(ellipsoid(wing, (0.021, 0.007, 0.012), rotation((0, 1, 0), s * 0.35), 10, 6))
        ends = [surface(TORSO, [zr], np.array([s * th0]), grow=0.012)[0, 0]
                for zr, th0 in ((1.508, 0.02), (1.46, 0.05), (1.41, 0.07))]
        tie.append(tube(ends, [0.007, 0.006, 0.005], [0.003, 0.0026, 0.0024], segs=6, up=(0.0, -1.0, 0.0), domes=1))
    part = Part('Tie', merge(tie), COAT, cavity=0.15)
    parts.append(part.weigh('spine_02', 0.8).weigh('neck', 0.2))
    # The burnt shirt round the coal: a cup sunk where the heart was, its rim glowing.
    center, nc, pc = coal_center()
    tc, zc = COAL_AT
    th = np.linspace(tc - 0.6, tc + 0.6, 17)
    z = np.linspace(zc - 0.11, zc + 0.11, 13)
    P = np.array([surface(TORSO, [zr], th, grow=0.004)[0] for zr in z])
    piece = (P.reshape(-1, 3), [grid_faces(len(z), len(th), closed=False)])
    piece = push(piece, pc, 0.062, -0.022)
    dist = np.linalg.norm(piece[0] - pc, axis=1)
    cavity = smoothstep(0.055, 0.03, dist) * 0.9
    ember = smoothstep(0.03, 0.05, dist) * smoothstep(0.08, 0.058, dist) * (0.4 + 0.6 * (0.5 + 0.5 * pnoise(piece[0], 60.0, 5)))
    part = Part('Crater', piece, SHIRT, cavity=cavity, ember=ember)
    parts.append(part.weigh('spine_02', 1.0))
    V, F = ellipsoid(center, (COAL_RADIUS, COAL_RADIUS, COAL_RADIUS * 0.92), segs=14, rings=9)
    V = V + (V - center) * (0.16 * pnoise(V, 30.0, 3))[:, None]
    parts.append(Part('Coal', (V, F), SKIN, slot=1).weigh('coal', 1.0))
    return parts


def build_stole():
    """The keeper's stole: a bone-white band round his neck and down both sides of the coat front to the knee, the
    sun-ring worked in black thread near each end, the ends fringed."""
    parts = []
    marks, fringe = [], []
    for s in SIDES:
        top = [(0.0, 0.07, 1.552), (s * 0.06, 0.03, 1.575), (s * 0.095, -0.03, 1.55)]
        down = []
        for zr in (1.48, 1.38, 1.26, 1.14, 1.02, 0.9, 0.78, 0.66):
            th0 = s * (0.62 - 0.08 * smoothstep(1.48, 1.0, zr))
            p = surface(COAT_A, [zr], np.array([th0]), grow=0.03)[0, 0]
            down.append(p + np.array([0.0, -0.012 * smoothstep(1.06, 0.7, zr), 0.0]))
        path = top + down
        piece = ribbon(path, [(0.0, 0.03), (0.25, 0.042), (0.8, 0.05), (1.0, 0.056)], n=34, cols=3,
                       out=lambda C: outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.08)
        part = Part('Stole', piece, STOLE)
        V = piece[0]
        neck = smoothstep(1.5, 1.55, V[:, 2])
        body_weights(part, V, shoulders=False)
        for bone in list(part.weights):
            part.weights[bone] = part.weights[bone] * (1.0 - neck)
        part.weigh('spine_02', 0.6 * neck).weigh('neck', 0.4 * neck)
        parts.append(part)
        # The sun-ring near the end, a ring with short rays laid flat on the band; the fringe below.
        end = np.array(down[-2]) + (np.array(down[-1]) - np.array(down[-2])) * 0.35
        n = outward(math.atan2(end[0], -end[1]))
        c = end + n * 0.0035
        up = np.array([0.0, 0.0, 1.0])
        v = np.cross(n, up)
        ring = []
        a = np.linspace(0.0, 2.0 * np.pi, 20, endpoint=False)
        for r0, r1 in ((0.0155, 0.0205),):
            inner = c + np.outer(np.cos(a), up) * r0 + np.outer(np.sin(a), v) * r0
            outer = c + np.outer(np.cos(a), up) * r1 + np.outer(np.sin(a), v) * r1
            ring.append((np.vstack([inner, outer]), [np.stack([np.arange(20), (np.arange(20) + 1) % 20,
                                                               20 + (np.arange(20) + 1) % 20, 20 + np.arange(20)], 1)]))
        for k in range(10):
            ang = 2.0 * np.pi * k / 10
            d = up * math.cos(ang) + v * math.sin(ang)
            side = np.cross(n, d)
            p0, p1 = c + d * 0.024, c + d * 0.034
            ring.append((np.array([p0 - side * 0.0022, p0 + side * 0.0022, p1 + side * 0.0012, p1 - side * 0.0012]),
                         [np.array([[0, 1, 2, 3]])]))
        marks.append(merge(ring))
        # The fringe: a comb of short tongues below the end.
        last, prev = np.array(down[-1]), np.array(down[-2])
        along = unit(last - prev)
        across = unit(np.cross(along, n))
        w = 0.056
        teeth = []
        for k in range(9):
            x = (k - 4) / 4.0 * (w - 0.006)
            p = last + across * x
            teeth.append((np.array([p - across * 0.0035, p + across * 0.0035, p + across * 0.0025 + along * 0.04,
                                    p - across * 0.0025 + along * 0.04]), [np.array([[0, 1, 2, 3]])]))
        fringe.append(merge(teeth))
    part = Part('StoleMarks', merge(marks), COAT, cavity=0.2)
    parts.append(body_weights(part, part.V, shoulders=False))
    part = Part('Fringe', merge(fringe), STOLE, cavity=0.15)
    parts.append(body_weights(part, part.V, shoulders=False))
    return parts


def sleeve(S, E, W, radii, seed):
    """A coat sleeve from the shoulder to the wrist: folds winding down it and bunched at the elbow."""
    rng = np.random.default_rng(seed)
    side = 1.0 if S[0] > 0 else -1.0
    d = unit(E - S)
    C = spline([S + np.array([-side * 0.012, 0.0, 0.01]), (S + E) * 0.5 + np.cross(d, [0.0, 0.0, 1.0]) * 0.006,
                E, (E + W) * 0.5, W - unit(W - E) * 0.02], 30)
    starts = rng.uniform(0.0, 2.0 * np.pi, 4)
    twists = rng.uniform(0.8, 1.8, 4) * rng.choice((-1.0, 1.0), 4)
    lengths = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(C, axis=0), axis=1))])
    at_elbow = float(lengths[np.argmin(np.linalg.norm(C - E, axis=1))] / lengths[-1])

    def rfn(t, a):
        base = np.interp(t, [0.0, at_elbow, 1.0], radii)
        f = sum(0.09 * np.exp(-(wrap(a - p - tw * t) / 0.35) ** 2) for p, tw in zip(starts, twists))
        elbow = 0.07 * np.exp(-((t - at_elbow) / 0.08) ** 2) * (0.6 + 0.4 * np.cos(3 * a))
        return base * (1.0 + f * smoothstep(0.05, 0.3, t) + elbow)
    return tube_fn(C, rfn, segs=20), C, at_elbow


def hand_pieces(W, fwd, up, side, curl, spread, thumb=0.35, length=1.0, thin=1.0, claw=0.0, knuckle=1.0, scale=1.0):
    """A hand from the wrist W (fwd along the fingers, up out of its back; side +1 left): closed pieces for a union,
    and each finger's line (name, points, radius) for its weights (Unpaid.py's hand)."""
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


def build_arms():
    """Coat sleeves with turned-back cuffs, the wrists and the big fists round the props; fingers on their own bones."""
    parts, fingers = [], {}
    for k, s in enumerate(SIDES):
        sfx = SUFFIX[s]
        A = ARMS[s]
        S, E, W = A['S'], A['E'], A['W']
        piece, line, at_elbow = sleeve(S, E, W, (0.064, 0.058, 0.05), seed=41 + k)
        low = reduce(piece, TRIANGLES['sleeve'])
        part = Part(f'Sleeve_{sfx}', low, COAT)
        d, along = polyline_distance(low[0], line)
        top = smoothstep(0.14, 0.0, along)
        elbow = smoothstep(at_elbow - 0.08, at_elbow + 0.08, along)
        part.weigh('spine_02', 0.45 * top)
        part.weigh(f'upperarm_{sfx}', (1.0 - 0.45 * top) * (1.0 - elbow))
        part.weigh(f'lowerarm_{sfx}', (1.0 - 0.45 * top) * elbow)
        parts.append(part)
        dv = unit(W - E)
        cuff = torus(W - dv * 0.035, dv, 0.046, 0.012, segs=16, sides=6, squash=1.4)
        parts.append(Part(f'Cuff_{sfx}', cuff, COAT, cavity=0.12).weigh(f'lowerarm_{sfx}', 1.0))
        limb = [tube([W - dv * 0.09, W - dv * 0.01], [0.03, 0.025], [0.034, 0.03], segs=16, up=A['u'])]
        hand, lines = hand_pieces(W, A['f'], A['u'], s, A['curls'], 0.04, thumb=0.75, length=1.05, thin=1.15,
                                  claw=0.0, knuckle=1.25, scale=HAND_SCALE)
        low = reduce(union(limb + hand, 0.0024, smooth=3), TRIANGLES['hand'])
        part = Part(f'Hand_{sfx}', low, SKIN)
        V = low[0]
        u_ = ((V - E) @ dv) / np.linalg.norm(W - E)
        hand_share = smoothstep(0.9, 1.02, u_)
        best = np.full(len(V), np.inf)
        owner = np.full(len(V), -1)
        along = np.zeros(len(V))
        for j, (name, pts, radius) in enumerate(lines):
            dd, tt = polyline_distance(V, pts)
            better = (dd < radius + 0.007) & (dd < best)
            best[better], owner[better], along[better] = dd[better], j, tt[better]
        finger_share = np.where(owner >= 0, smoothstep(0.06, 0.2, along), 0.0)
        part.weigh(f'lowerarm_{sfx}', 1.0 - hand_share)
        part.weigh(f'hand_{sfx}', hand_share * (1.0 - finger_share))
        for j, (name, pts, radius) in enumerate(lines):
            part.weigh(finger_bone(name, sfx), hand_share * finger_share * (owner == j))
        parts.append(part)
        fingers[s] = lines
    return parts, fingers


# --- The head: a weathered man of about fifty-five in grief, a face that carries a scene at arm's length ---
# Head space: origin mid-head at eye height (HEAD_CENTER in the rig), front -Y, up +Z, meters. The head is one radial
# surface (a radius for every direction from its middle: face_radius), sunk deep where the eyes sit; creases, brows,
# beard and hair are grown out of it; the eyes and their lids, and the ears, are pieces of their own.

MOUTH_Z = -0.071                            # the lips' meeting line
HINGE = np.array([0.0, 0.008, -0.032])      # the jaw's hinge
MOUTH_HALF = 0.36                           # the mouth's slit, radians either side of the middle
JAW_BUILD = 14.0                            # the jaw is opened this far while the head is reduced (the lips apart, so
                                            # each keeps its own side), then closed to JAW_REST
EYE_RADIUS = 0.0122
LID_RADIUS = 0.0152                         # the lids lie over the eyeball this far from its middle
EYE_DOWN = 6.0                              # degrees the eyes look down


def upper_margin(x):
    """The upper lid's line (height over the eye's middle, m) across the eye (x, m): heavy, tired, arched."""
    return 0.0042 - 0.0046 * (x / 0.0125) ** 2


def lower_margin(x):
    return -0.0058 + 0.0024 * (x / 0.0125) ** 2


HEAD = dict(width=0.156, front=0.097, back=0.108, top=0.12, bottom=0.13)
EYE_TH = 0.36                               # the eyes: angle either side of the front


def face_radius(d, sunk=True):
    """The head's radius along unit directions d (head space): a skull whose front is flatter than an egg, its nape
    tucked in and the jaw narrowing to the chin; a heavy brow ridge knitted in the middle, hollows round the eyes and
    tired bags under them; a long straight nose with a bump on the bridge, its tip and wings; full cheekbones and their
    arches, a little hollow under them; the temples; the mouth's arch standing forward, the jaw's angle, the chin; the
    folds from the nose past the mouth; the lips."""
    g = gauss
    TH = np.arctan2(d[..., 0], -d[..., 1])
    a = HEAD['width'] / 2.0
    b = np.where(d[..., 1] < 0, HEAD['front'], HEAD['back'])
    c = np.where(d[..., 2] > 0, HEAD['top'], HEAD['bottom'])
    p = np.where(d[..., 1] < 0, 2.35, 2.0)
    r = 1.0 / ((np.abs(d[..., 0]) / a) ** p + (np.abs(d[..., 1]) / b) ** p + (np.abs(d[..., 2]) / c) ** p) ** (1.0 / p)
    z = d[..., 2] * r
    at = np.abs(TH)
    zm = MOUTH_Z
    r = r * (1.0 - 0.2 * smoothstep(-0.03, -0.11, z) * smoothstep(1.9, 2.8, at))
    r = r * (1.0 - 0.07 * smoothstep(zm + 0.01, -HEAD['bottom'], z) * smoothstep(2.2, 1.2, at) * np.abs(np.sin(TH)))
    eye = lambda w, zc, s: (g(TH - EYE_TH, w) + g(TH + EYE_TH, w)) * g(z - zc, s)    # noqa: E731
    brow_z = 0.021 + 0.007 * g(TH, 0.32)
    R = 0.0145 * g(TH, 0.72) * g(z - brow_z, 0.012) + 0.0028 * g(TH, 0.12) * g(z - 0.03, 0.009)
    R = R - 0.006 * (g(TH - EYE_TH, 0.17) + g(TH + EYE_TH, 0.17)) * g(z - 0.001, 0.017)
    if sunk:
        # Where the eye sits the face sinks well behind it, so the lids meet it smoothly instead of a cut edge.
        R = R - 0.011 * (g(TH - EYE_TH, 0.12) + g(TH + EYE_TH, 0.12)) * g(z + 0.0008, 0.0105)
    R = R + 0.0014 * eye(0.12, -0.0175, 0.0045)
    nose = (0.03 * smoothstep(0.016, -0.036, z) * smoothstep(-0.057, -0.041, z)
            * g(TH, 0.065 + 0.075 * smoothstep(-0.01, -0.046, z)))
    R = R + nose + 0.0022 * g(TH, 0.055) * g(z + 0.011, 0.008) + 0.0035 * g(TH, 0.085) * g(z + 0.038, 0.007)
    R = R + 0.0058 * (g(TH - 0.1, 0.042) + g(TH + 0.1, 0.042)) * g(z + 0.043, 0.0062)
    R = R + (0.0085 * g(at - 0.7, 0.24) * g(z + 0.02, 0.017)
             + 0.005 * g(z + 0.018, 0.009) * smoothstep(0.55, 0.75, at) * smoothstep(1.55, 1.3, at)
             - 0.0035 * g(at - 0.66, 0.22) * g(z + 0.057, 0.02)
             - 0.0055 * g(at - 1.05, 0.22) * g(z - 0.04, 0.025)
             + 0.0075 * g(TH, 0.52) * g(z + 0.07, 0.028)
             + 0.006 * g(at - 1.2, 0.2) * g(z + 0.088, 0.02)
             + 0.008 * g(TH, 0.32) * g(z + HEAD['bottom'] - 0.017, 0.014))
    nl = 0.21 + 0.24 * np.clip((-0.045 - z) / 0.042, 0.0, 1.0)
    window = smoothstep(-0.04, -0.05, z) * smoothstep(-0.1, -0.088, z)
    R = R - 0.0028 * g(at - nl, 0.036) * window + 0.0018 * g(at - nl - 0.07, 0.05) * window
    R = R + (0.0016 * g(TH, 0.4) * g(z - zm, 0.02) - 0.0024 * g(TH, 0.3) * g(z - zm, 0.0028)
             + 0.0006 * g(TH, 0.3) * g(z - zm + 0.008, 0.0042) + 0.0004 * g(TH, 0.3) * g(z - zm - 0.007, 0.0035))
    return r + R


def eye_center(s):
    """An eyeball's middle: its front 2.5 mm behind the face where it looks out, under the brow."""
    dirn = np.array([s * math.sin(EYE_TH), -math.cos(EYE_TH), 0.0])
    surf = dirn * float(face_radius(dirn[None], sunk=False)[0])
    look = rot_x(-EYE_DOWN) @ unit(np.array([0.05 * s, -1.0, 0.0]))
    return surf - look * (0.0025 + EYE_RADIUS)


EYE_C = eye_center(1.0)                     # the left eyeball's middle (the right's is mirrored)


def beard_line(at):
    """Where the beard starts on the face: just under the cheekbones, up the jaw to the sideburns."""
    return -0.034 - 0.012 * gauss(at - 0.8, 0.3) + 0.06 * smoothstep(1.15, 1.5, at)


def head_surface(rows=190, cols=200):
    """The head as one surface: its anatomy (face_radius), the brow's creases and the frown between the brows, bags,
    crow's feet, the folds from the nose past the mouth, whose corners are pulled down (grief); bushy brows raised at
    their inner ends; a trimmed grey beard following the jaw in soft combed lobes, the moustache clear of the lips; grey
    hair cropped under the hat. The lips are parted by a slit the jaw opens. Returns ((V, faces), cavity, jaw weight,
    keep, tint zone: the beard and hair Zone2, the rest Zone1)."""
    g = gauss
    phi = np.linspace(0.0, np.pi, rows + 2)[1:-1]
    th = np.linspace(-np.pi, np.pi, cols, endpoint=False)
    PH, TH = np.meshgrid(phi, th, indexing='ij')
    d = np.stack([np.sin(PH) * np.sin(TH), -np.sin(PH) * np.cos(TH), np.cos(PH)], -1)
    r = face_radius(d)
    z = d[..., 2] * r
    at = np.abs(TH)
    zm = MOUTH_Z
    eye = lambda w, zc, s: (g(TH - EYE_TH, w) + g(TH + EYE_TH, w)) * g(z - zc, s)    # noqa: E731
    # The forehead creased by the raised inner brows, the frown's two lines between them.
    creases = sum(g(z - zk, 0.0032) for zk in (0.05, 0.064, 0.078)) * g(TH, 0.34)
    frown = (g(TH - 0.06, 0.018) + g(TH + 0.06, 0.018)) * g(z - 0.033, 0.009)
    R = -0.0015 * creases - 0.0012 * frown
    # The crease under the bags, and crow's feet fanning from the outer corners of the eyes.
    R = R - 0.001 * eye(0.13, -0.0235, 0.0018)
    crow = np.zeros_like(TH)
    for sgn in (1.0, -1.0):
        s_arc = (sgn * TH - 0.53) * 0.085
        for ang in (-0.55, -0.1, 0.4):
            along = math.cos(ang) * s_arc + math.sin(ang) * z
            across = -math.sin(ang) * s_arc + math.cos(ang) * z
            crow = crow + g(across, 0.0019) * smoothstep(0.002, 0.008, along) * smoothstep(0.03, 0.015, along)
    R = R - 0.001 * crow
    # The folds from the nose's wings down past the corners of the mouth.
    nl = 0.21 + 0.24 * np.clip((-0.045 - z) / 0.042, 0.0, 1.0)
    window = smoothstep(-0.04, -0.05, z) * smoothstep(-0.1, -0.088, z)
    naso = g(at - nl, 0.032) * window
    R = R - 0.0026 * naso
    # The brows: bushy grey hair on the ridge, their inner ends raised and drawn up toward the middle.
    line = 0.027 + 0.0095 * g(at - 0.12, 0.11) - 0.011 * ((at - 0.4) / 0.32) ** 2
    brows = g(z - line, 0.006) * smoothstep(0.07, 0.14, at) * smoothstep(0.78, 0.66, at)
    tufts = np.sin(TH * 34.0 + z * 160.0)
    R = R + 0.0038 * brows * (1.0 + 0.12 * tufts)
    # The beard: trimmed close on the cheeks, fuller at the chin, following the jaw to the sideburns, in soft combed
    # lobes; the moustache over the upper lip; the lips and the nose clear.
    lb = beard_line(at)
    m = smoothstep(lb + 0.003, lb - 0.005, z) * smoothstep(1.72, 1.58, at)
    chin = smoothstep(zm - 0.02, zm - 0.07, z)
    t = 0.006 + 0.004 * smoothstep(lb, lb - 0.03, z) + 0.006 * g(TH, 0.45) * chin
    tache = g(TH, 0.4) * smoothstep(zm + 0.026, zm + 0.016, z) * smoothstep(zm + 0.003, zm + 0.0095, z)
    t = np.maximum(t, 0.0072 * tache)
    m = np.maximum(m, tache)
    lips = g(TH, 0.36) * smoothstep(zm + 0.0105, zm + 0.0045, z) * smoothstep(zm - 0.014, zm - 0.006, z)
    m = m * (1.0 - lips) * (1.0 - g(TH, 0.13) * smoothstep(-0.051, -0.043, z))
    comb = (np.sin(TH * 30.0 + 2.0 * np.sin(z * 60.0) + 1.3 * np.sin(TH * 7.0)) * 0.5
            + np.sin(TH * 13.0 - z * 45.0) * 0.3 + np.sin(TH * 47.0 + z * 90.0) * 0.2)
    R = R + m * t * (1.0 + 0.13 * comb)
    # The hair under the hat: cropped grey, down to the nape and the sideburns.
    low = -0.04 + 0.03 * smoothstep(2.4, 1.3, at)
    front = 0.95 + 0.3 * smoothstep(0.045, -0.02, z)
    hair = smoothstep(front - 0.16, front + 0.08, at) * smoothstep(low - 0.012, low + 0.012, z) * (1.0 - m)
    comb2 = np.sin(TH * 24.0 + 1.6 * np.sin(z * 60.0))
    R = R + 0.0048 * hair * (1.0 + 0.12 * comb2)
    P = d * (r + R)[..., None]
    # The corners of the mouth pulled down, the cheeks round them with them.
    P[..., 2] -= 0.0038 * smoothstep(0.1, 0.36, at) * g(z - zm, 0.018) * smoothstep(1.2, 0.6, at)

    # Cavity, only where a face has it: the eye sockets and under the brows, the brows themselves a little, the
    # nostrils, the line of the mouth, the folds from the nose; the crow's feet, the frown and the crease under the
    # bags as thin lines. The broad planes (forehead, cheeks, temples) keep the skin's own pale value, as the Unpaid's.
    # The beard and the hair take the shirt's light zone (Zone2) with a little combing in the cavity, so they read
    # light grey against the face.
    zz = P[..., 2]
    sock = np.clip(1.6 * (g(TH - EYE_TH, 0.12) + g(TH + EYE_TH, 0.12)) * g(z - 0.002, 0.014), 0.0, 1.0)
    cav = 0.6 * sock
    cav = np.maximum(cav, 0.25 * np.clip(eye(0.13, -0.0235, 0.0018) * 1.4, 0.0, 1.0))
    cav = np.maximum(cav, 0.35 * smoothstep(0.15, 0.6, brows))
    cav = np.maximum(cav, 0.7 * (g(TH - 0.06, 0.022) + g(TH + 0.06, 0.022)) * g(z + 0.048, 0.0045))
    cav = np.maximum(cav, 0.82 * g(TH, 0.3) * g(z - zm, 0.0045) * (1.0 - smoothstep(0.3, 0.42, at)))
    cav = np.maximum(cav, 0.5 * naso)
    cav = np.maximum(cav, 0.16 * frown)
    cav = np.maximum(cav, 0.2 * np.clip(crow, 0.0, 1.0))
    hairy = np.maximum(m, hair)
    cav = np.where(hairy > 0.5, 0.11 + 0.05 * (1.0 - np.where(m > hair, comb, comb2)), cav)
    # Where the reduction keeps detail: the face, the eyes and the mouth most.
    face = np.maximum(smoothstep(1.35, 0.95, at), 0.4 * smoothstep(1.9, 1.5, at)) * smoothstep(-0.16, -0.12, z) * smoothstep(0.085, 0.06, z)
    keep = face * (0.4 + 0.6 * np.clip(1.6 * (g(TH - EYE_TH, 0.22) + g(TH + EYE_TH, 0.22)) * g(z, 0.025)
                                       + g(TH, 0.5) * g(z - zm, 0.022) + g(TH, 0.12) * g(z + 0.03, 0.03), 0.0, 1.0))

    # The mouth: a slit along the row nearest the lip line, across the mouth; the lower lip, the chin and the beard
    # under it turn with the jaw, the cheeks beyond the corners stretch. The band round the slit is kept whole, so the
    # two lips still meet along it when the jaw closes.
    centre_col = cols // 2
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
    zone_v = np.concatenate([np.where(hairy > 0.5, SHIRT, SKIN).ravel(), np.zeros(len(inner)), [0.0, 0.0]])
    F = grid_faces(rows, cols)
    lower = (F[:, 0] // cols == i_m)
    swap = {int(i_m * cols + j): int(k) for j, k in zip(inner, dup)}
    for f in np.where(lower)[0]:
        F[f] = [swap.get(int(v), int(v)) if v // cols == i_m else v for v in F[f]]
    top, bottom = n + len(inner), n + len(inner) + 1
    piece = orient((V, [F, fan(top, np.arange(cols)), fan(bottom, np.arange(cols) + n - cols)]))
    return piece, cav_v, jw_v, keep_v, zone_v


def jaw_turn(V, jw, degrees):
    """Head-space points turned about the jaw's hinge by degrees (down), as much as their jaw weight."""
    a = math.radians(degrees) * np.asarray(jw, float)
    y, z = V[:, 1] - HINGE[1], V[:, 2] - HINGE[2]
    out = V.copy()
    out[:, 1] = HINGE[1] + y * np.cos(a) - z * np.sin(a)
    out[:, 2] = HINGE[2] + y * np.sin(a) + z * np.cos(a)
    return out


def eye_frame(s):
    """An eye's middle and axes (rows: across, back into the head, up; right-handed on both sides, so both eyes' pieces
    face out), looking ahead, a little out and down."""
    center = EYE_C * np.array([s, 1.0, 1.0])
    look = rot_x(-EYE_DOWN) @ unit(np.array([0.05 * s, -1.0, 0.0]))
    upward = unit(np.array([0.0, 0.0, 1.0]) - look * look[2])
    across = np.cross(-look, upward)
    return center, np.array([across, -look, upward])


def lid_patch(center, axes, rows):
    """A lid as a grid over the eye (columns across, rows from its margin out): rows of (height over the eye's middle,
    distance from it) per column; the first rows roll in from the margin to the eyeball, so the lid has a rounded edge."""
    P = []
    for zr, radius in rows:
        xs = LID_XS
        yr = -np.sqrt(np.clip(radius ** 2 - xs ** 2 - zr ** 2, 0.0, None))
        P.append(np.stack([xs, yr, zr], -1))
    P = np.array(P)
    return center + P.reshape(-1, 3) @ axes, [grid_faces(len(rows), len(LID_XS), closed=False)]


LID_XS = np.linspace(-0.0128, 0.0128, 13)


def eye_pieces():
    """Each eye (head space): a dark eyeball set in its orbit and two lids over it, the upper heavy and the lower tired,
    each lid rolling in at its margin to meet the eyeball; light at their margins and going into the orbit's shadow.
    Returns (piece, cavity) pairs."""
    pieces = []
    x = LID_XS
    top = np.sqrt(np.clip(LID_RADIUS ** 2 - x ** 2, 0.0, None)) * 0.94
    inner, mid = EYE_RADIUS + 0.0004, (EYE_RADIUS + LID_RADIUS) / 2.0 + 0.0006
    um, lm_ = upper_margin(x), lower_margin(x)
    # The upper lid: from the eyeball up round the margin, then out over the eye into the orbit.
    up_rows = [(um - 0.0011, inner), (um - 0.0006, mid), (um, LID_RADIUS)]
    up_rows += [(um + (top - um) * q, LID_RADIUS) for q in (0.3, 0.62, 1.0)]
    low_rows = [(-top, LID_RADIUS * 0.98), (lm_ + (-top - lm_) * 0.45, LID_RADIUS * 0.98), (lm_, LID_RADIUS * 0.98),
                (lm_ + 0.0005, mid), (lm_ + 0.0009, inner)]
    shade_up = np.repeat([0.08, 0.1, 0.14, 0.26, 0.4, 0.54], len(x))
    shade_low = np.repeat([0.52, 0.34, 0.18, 0.12, 0.1], len(x))
    for s in SIDES:
        center, axes = eye_frame(s)
        pieces.append((ellipsoid(center, (EYE_RADIUS,) * 3, axes, 12, 7), 0.9))
        pieces.append((lid_patch(center, axes, up_rows), shade_up))
        pieces.append((lid_patch(center, axes, low_rows), shade_low))
    return pieces


def ear_piece(s):
    """An ear (head space, side s): the rim curling from above the opening round the back to the lobe, the bowl inside
    it, the lobe and the flap before the opening; standing off the head a little at the back. Returns the piece and its
    cavity."""
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
    V = V * np.array([s, 1.0, 1.0])
    # On the side of the head, between the cheekbone's arch and the jaw's hinge.
    V = V @ R.T + np.array([s * 0.07, 0.012, -0.014])
    if s < 0:
        F = [f[:, ::-1] for f in F]
    return (V, F), cav


def build_head():
    """The head (face, brows, beard and hair in one surface), the eyes and their lids, the ears, the mouth behind the
    lips and the teeth, the neck."""
    parts = []
    center = HEAD_CENTER
    (V, F), cav, jw, keep, zone = head_surface()
    jw[-1] = 0.5                                       # the pole under the chin, hidden in the neck
    opened = jaw_turn(V, jw, JAW_BUILD)
    low = reduce((opened, F), TRIANGLES['head'], keep=keep, keep_factor=3.0)
    near = nearest(opened, low[0])
    jw_low = jw[near]
    Vr = jaw_turn(low[0], jw_low, JAW_REST - JAW_BUILD)
    head = Part('Head', (Vr + center, low[1]), zone[near], cavity=cav[near])
    parts.append(head.weigh('head', 1.0 - jw_low).weigh('jaw', jw_low))
    for piece, c in eye_pieces():
        parts.append(Part('Eye', transform(piece, None, center), SKIN, cavity=c).weigh('head', 1.0))
    for s in SIDES:
        piece, c = ear_piece(s)
        parts.append(Part('Ear', transform(piece, None, center), SKIN, cavity=c).weigh('head', 1.0))
    zm = MOUTH_Z
    # The mouth behind the lips (dark), stretching with the jaw.
    sock = ellipsoid((0.0, -0.06, zm - 0.006), (0.024, 0.026, 0.017), segs=12, rings=7)
    sw = smoothstep(zm + 0.004, zm - 0.01, sock[0][:, 2])
    parts.append(Part('Mouth', (jaw_turn(sock[0], sw, JAW_REST) + center, sock[1]), SKIN, cavity=1.0)
                 .weigh('head', 1.0 - sw).weigh('jaw', sw))
    # Teeth: a row behind each lip, worn and even.
    for upper in (True, False):
        a = np.linspace(-0.55, 0.55, 9)
        zt = (zm + 0.0045, zm - 0.0005) if upper else (zm - 0.0015, zm - 0.0065)
        rr = 0.03 if upper else 0.028
        pts = np.array([[[rr * math.sin(x), -0.072 - 0.012 * math.cos(x) - (0.0015 if upper else 0.0), zz]
                         for x in a] for zz in zt])
        piece = (pts.reshape(-1, 3), [grid_faces(2, len(a), closed=False)])
        Vt = piece[0] if upper else jaw_turn(piece[0], np.ones(len(piece[0])), JAW_REST)
        parts.append(Part('Teeth', (Vt + center, piece[1]), SKIN, cavity=0.2).weigh('head' if upper else 'jaw', 1.0))
    # The neck: up into the head from inside the collar.
    C = spline([(0.0, -0.012, 1.48), (0.0, -0.022, 1.55), center + (0.0, 0.018, -0.07)], 9)

    def rfn(s, a):
        cords = 0.08 * (gauss(wrap(a - 0.95), 0.25) + gauss(wrap(a + 0.95), 0.25)) * smoothstep(0.2, 0.5, s)
        return np.interp(s, [0.0, 0.5, 1.0], [0.05, 0.047, 0.046]) * (1.0 + cords)
    neck = tube_fn(C, rfn, segs=14, up=(0.0, -1.0, 0.0))
    s_neck = polyline_distance(neck[0], C)[1]
    part = Part('Neck', neck, SKIN)
    part.weigh('spine_02', 0.6 * smoothstep(0.25, 0.0, s_neck))
    part.weigh('head', smoothstep(0.6, 1.0, s_neck))
    part.weigh('neck', 1.0 - 0.6 * smoothstep(0.25, 0.0, s_neck) - smoothstep(0.6, 1.0, s_neck))
    parts.append(part)
    return parts


# The shroud's side chains (tail_l_01 .. tail_r_02) at their rest points as the rig was handed over (2026-10-07): fixed
# here, so the strips can change without moving a bone. Each list is the chain's first head, its joint and its end.
SIDE_CHAINS = {1.0: ((0.10417993505684862, 0.12726328779342894, 0.422535517385642),
                     (0.11105428042612653, 0.5932806867991746, 0.12405007538871289),
                     (0.11643843393333371, 1.0771138296696836, 0.12087758387707037)),
               -1.0: ((-0.1325481430089557, 0.12654919646544996, 0.4304879693247792),
                      (-0.12898045512836412, 0.6066128487443253, 0.15670934534553485),
                      (-0.10854571420903397, 1.1041164245552966, 0.08328022629969395))}
SHROUD_SPLIT = 0.38         # where the shroud tears into strips: just under the coat's hem
STRIP_CENTERS = (12.0, 84.0, 156.0, -132.0, -60.0)  # degrees round the shroud from the front (+ toward his right): five
                                                    # wide strips, set a column off true, so no two hang as a pair
STRIP_SWEEP = 0.1           # how far the strips' tips blow to his left (m): the Gravewind off the canyon, as a tail
STRIP_SOLID = 0.92          # the fade at a strip's last solid row: 1.3 x 0.92 - 0.15 > 1, above every value of M_Ghost's
                            # noise, so no hole opens in a strip and nothing breaks off it
TIP_CUT = 0.006             # the tip's alpha cut: the last row (fade 0) this far past the last solid one, in meters


def build_shroud():
    """The shroud below the coat at game density: a pale tube from inside the skirts down and back, whole to just under
    the hem, then five wide strips sweeping back behind him and tapering to soft points. One strip is in front, centered,
    and swings back under him, so nothing hangs in front like a pair of legs; those at the sides and back trail long.

    The strips don't fade along their length: M_Ghost cuts its fade against a macro noise that runs 0 to 1 in sharp
    blobs (up to about 100 a meter along V), so any gradual fade breaks into holes and floating bits. Each strip stays
    solid (its fade 1 down to 0.92, which the noise never reaches) and its tip is cut over its last 6 mm, so it ends as
    the soft point its taper draws, without fragments. Returns the part and the tail chains' lines."""
    rows, cols, mrows = 17, 30, 6
    narrow, spread, wave, folds, harmonics, twist, seed = 0.55, 0.05, 0.03, 0.3, (5, 7, 9, 12), 0.8, 5
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
    # The tube, whole from the waist to just under the hem.
    tt, th = np.meshgrid(np.linspace(0.0, SHROUD_SPLIT, mrows), theta, indexing='ij')
    pieces = [(point(tt, th).reshape(-1, 3), [grid_faces(mrows, cols)])]
    blocks, start = [], mrows * cols                         # each strip's first vertex, rows and columns
    ts, fades, qs, sides = [tt.ravel()], [np.ones(tt.size)], [np.zeros(tt.size)], [np.zeros(tt.size)]
    angles = [wrap(th).ravel()]
    srows = rows - mrows + 1
    # The strips' rows, the last one just past the one before (the tip's cut).
    span = 1.0 - TIP_CUT / ((0.85 - SHROUD_SPLIT) * length)
    q = np.concatenate([np.linspace(0.0, span, srows - 1), [1.0]])[:, None]
    step = cols // len(STRIP_CENTERS)
    for k, center in enumerate(STRIP_CENTERS):
        jc = int(round(center / 360.0 * cols))
        js = np.arange(jc - step // 2, jc - step // 2 + step + 1)
        th_j = 2.0 * np.pi * js / cols
        tc = math.radians(center)
        front = max(0.0, math.cos(tc)) ** 2
        # The front strip is the shortest (it swings back under him); those at the back trail longest.
        end = 0.72 + 0.24 * (1.0 - math.cos(tc)) / 2.0 + rng.uniform(-0.015, 0.015)
        across = (js - js.mean()) / (len(js) / 2.0)
        jag = end - 0.06 * np.abs(across) ** 1.5           # a soft point, longest in the middle
        tq = SHROUD_SPLIT + (jag[None, :] - SHROUD_SPLIT) * q
        ang = tc + (th_j[None, :] - tc) * (1.0 - narrow * q ** 1.8)
        lift = spread * q ** 1.6
        Q = point(tq, ang, out=lift)
        c, n, b = at(SHROUD_SPLIT + (np.mean(jag) - SHROUD_SPLIT) * q[:, 0])
        side_dir = -math.sin(tc) * n + math.cos(tc) * b
        Q = Q + (side_dir * (wave * q * np.sin(q * 7.0 + k * 1.3)))[:, None, :]
        Q = Q + np.array([0.0, 0.2, 0.05])[None, None, :] * (front * q ** 1.4)[..., None]
        Q[..., 0] += STRIP_SWEEP * q ** 1.5
        pieces.append((Q.reshape(-1, 3), [grid_faces(srows, len(js), closed=False)]))
        blocks.append((start, srows, len(js)))
        start += srows * len(js)
        ts.append(tq.ravel())
        fade = np.broadcast_to(1.0 - (1.0 - STRIP_SOLID) * q / span, tq.shape).copy()
        fade[-1] = 0.0
        fades.append(fade.ravel())
        qs.append(np.broadcast_to(q, tq.shape).ravel())
        angles.append((wrap(tc) + ang - tc).ravel())
        # The side the strip hangs on (the shroud's frame has its binormal on his right): +1 left, -1 right, 0 the
        # middle, for the side chains.
        sides.append(np.full(tq.size, 0.0 if abs(math.sin(tc)) < 0.3 else -math.copysign(1.0, math.sin(tc))))
    V, F = merge(pieces)
    t = np.concatenate(ts)
    fade = np.concatenate(fades)
    q = np.concatenate(qs)
    side = np.concatenate(sides)
    w, dd = table(SHROUD_SIZE, t)
    radius = (3.0 * (w + dd) - np.sqrt((3.0 * w + dd) * (w + 3.0 * dd))) / 2.0
    uv = np.column_stack([np.concatenate(angles), radius, -t * length])
    part = Part('Shroud', (V, F), SKIN, fade=fade, strip_uv=uv)
    mids = [(a + b) / 2 for a, b in zip(TAIL_STOPS[:-1], TAIL_STOPS[1:])]
    main = chain(t, mids, ('tail_01', 'tail_02', 'tail_03', 'tail_04', 'tail_05'))
    top = smoothstep(0.06, 0.0, t)
    side_share = 0.6 * smoothstep(0.1, 0.6, q) * (side != 0)
    for bone, wgt in main.items():
        part.weigh(bone, wgt * (1.0 - top) * (1.0 - side_share))
    part.weigh('pelvis', top)
    for s in SIDES:
        sfx = SUFFIX[s]
        on_side = side_share * (side == s) * (1.0 - top)
        sw = chain(t, (0.46, 0.78), (f'tail_{sfx}_01', f'tail_{sfx}_02'))
        for bone, wgt in sw.items():
            part.weigh(bone, wgt * on_side)
    lines = {s: [np.array(p) for p in SIDE_CHAINS[s]] for s in SIDES}
    return [part], dict(path=C, sides=lines, strips=dict(V=V, fade=fade, uv=uv, blocks=blocks))


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


# --- The rig ---

def bone_specs(fingers, tail):
    """(name, head, tail, parent) for every bone, Blender meters, laid out on the rest pose the parts were built in."""
    center, nc, pc = coal_center()
    specs = [('pelvis', (0.0, 0.0, 1.0), (0.0, 0.0, 1.12), None),
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
        if s > 0:
            specs.append(('lantern', A['G'], A['G'] + np.array([0.0, 0.0, -0.12]), 'hand_l'))
        else:
            specs.append(('gun', A['G'], A['G'] + A['bore'] * 0.1, 'hand_r'))
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
    specs.append(('skirt_f_02', SKIRT_KNEE, SKIRT_HEM, 'skirt_f_01'))
    for s in SIDES:
        mirror = np.array([s, 1.0, 1.0])
        specs.append((f'skirt_{SUFFIX[s]}_01', SKIRT_SIDE_HIP * mirror, SKIRT_SIDE_KNEE * mirror, 'pelvis'))
    return [(name, tuple(map(float, h)), tuple(map(float, t)), parent) for name, h, t, parent in specs]


def ghost_slot(name, colors, **scalars):
    """An M_Ghost slot: its properties are the material instance's parameters (the importer resets the instance and
    sets only these). Its Blender color is the first zone's, for the viewport."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(int(colors[0][1:], 16))
    for key in [k for k in mat.keys() if not k.startswith('_')]:
        del mat[key]
    mat['Master'] = 'Ghost'
    mat['TextureSet'] = 'Polymer'
    for k, color in enumerate(colors):
        mat[f'Zone{k + 1}Color'] = color
    for key, value in scalars.items():
        mat[key] = value
    return mat


# The Unpaid's approved look (Unpaid.py's Ghost_A, tried in the game's daylight): a thin rim that still reads, the body's
# own faint glow, the ember edge, the fade's noise and steepness, the Polymer grain three times a meter.
GHOST_LOOK = dict(RimColor=RIM_COLOR, RimPower=4.0, RimStrength=2.2, GlowStrength=0.12, EmberStrength=2.0,
                  NoiseScale=1.4, FadeSoftness=0.07, UVScale=3.0)


def materials():
    """GhostAbel (body and hat), GhostCoal (shared with SK_Unpaid: exactly its values), and the props' slots."""
    body = ghost_slot('GhostAbel', ZONE_COLORS, **GHOST_LOOK)
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
    lantern = ghost_slot('GhostAbelLantern', LANTERN_COLORS, **GHOST_LOOK)
    # The globe: the same ghost, glowing of itself. UVScale nearly nothing keeps the grain out of the light.
    flame = ghost_slot('GhostAbelFlame', (FLAME_COLOR,) * 4, RimColor=RIM_COLOR, RimPower=4.0, RimStrength=0.6,
                       GlowStrength=5.0, EmberStrength=0.0, NoiseScale=1.4, FadeSoftness=0.07, UVScale=0.02)
    pump = ghost_slot('GhostAbelPump', PUMP_COLORS, **GHOST_LOOK)
    return dict(body=body, coal=coal, lantern=lantern, flame=flame, pump=pump)


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


LAYOUT = {}       # part name: its vertex ranges in SK_Abel's mesh; 'head_faces': the face's faces (filled by build_rig)


def build_rig():
    coat_parts = build_coat()
    shirt_parts = build_shirt()
    stole_parts = build_stole()
    arm_parts, fingers = build_arms()
    head_parts = build_head()
    shroud_parts, tail = build_shroud()
    LAYOUT['strips'] = tail['strips']
    parts = coat_parts + shirt_parts + stole_parts + arm_parts + head_parts + shroud_parts
    for p in parts:
        if not p.weights:
            raise RuntimeError(f'{p.name} has no weights')
    rig = lm.armature()
    lm.bones(rig, bone_specs(fingers, tail))
    V, faces, colors, slots, names, W = assemble(parts)
    mesh = to_mesh('Abel', V, faces)
    mats = materials()
    mesh.materials.append(mats['body'])
    mesh.materials.append(mats['coal'])
    mesh.polygons.foreach_set('material_index', slots.astype(np.int32))
    mesh.validate()
    if len(mesh.vertices) != len(V):
        raise RuntimeError('validate() changed the vertices')
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(70.0))
    # The face, eyes, ears and neck are never hard-edged: their folds and lids are drawn by shading alone.
    soft = np.concatenate([np.full(len(p.V), p.name in ('Head', 'Eye', 'Ear', 'Neck', 'Mouth', 'Teeth')) for p in parts])
    if 'sharp_edge' in mesh.attributes:
        ev = np.empty(len(mesh.edges) * 2, dtype=np.int64)
        mesh.edges.foreach_get('vertices', ev)
        ev = ev.reshape(-1, 2)
        sharp = np.empty(len(mesh.edges), dtype=bool)
        mesh.attributes['sharp_edge'].data.foreach_get('value', sharp)
        sharp &= ~(soft[ev[:, 0]] & soft[ev[:, 1]])
        mesh.attributes['sharp_edge'].data.foreach_set('value', sharp)
    body = bpy.data.objects.new('Abel', mesh)
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
    # Where each part's vertices are in the mesh, and the face's faces: for checking a pose's hands against his face.
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
    log(f'SK_Abel: {tris} triangles, {len(mesh.vertices)} vertices, {len(rig.data.bones)} bones')
    totals = {}
    for p in parts:
        totals[p.name] = totals.get(p.name, 0) + p.triangles()
    log('  ' + ', '.join(f'{name} {count}' for name, count in totals.items()))
    return rig, body, mats, colors, (V, faces, W, names)


# --- The props: hat, lantern, pump (static meshes modeled where he holds them at rest, their pivots at their bones) ---

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


def hat_pieces():
    """The keeper's Sunday hat in hat space (origin at the brim's middle, front -Y): a low flat crown and a wide flat
    brim, a thin solid with its edge rolled up a little, and the band."""
    segs = 48
    theta = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    q = np.linspace(0.0, 1.0, 6)[:, None]
    r = 0.084 + 0.128 * q
    lift = 0.011 * smoothstep(0.7, 1.0, q) ** 2 + 0.003 * q * np.sin(2 * theta)
    top = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.05, lift + 0.0 * theta], -1)
    under = top - np.array([0.0, 0.0, 0.007])
    R = len(q)
    V = np.vstack([top.reshape(-1, 3), under.reshape(-1, 3)])
    F = [grid_faces(R, segs)[:, ::-1], grid_faces(R, segs) + R * segs]
    outer = np.arange(segs)
    edge = np.stack([(R - 1) * segs + outer, (R - 1) * segs + (outer + 1) % segs,
                     R * segs + (R - 1) * segs + (outer + 1) % segs, R * segs + (R - 1) * segs + outer], 1)
    F.append(edge[:, ::-1])
    brim = (V, F)
    zc = [-0.006, 0.03, 0.06, 0.082, 0.09, 0.093]
    rc = [0.087, 0.086, 0.085, 0.083, 0.077, 0.065]
    crown = np.array([np.stack([rr * np.sin(theta), -rr * np.cos(theta) * 1.1, np.full_like(theta, zz)], -1)
                      for zz, rr in zip(zc, rc)])
    n = len(zc) * segs
    crown = (np.vstack([crown.reshape(-1, 3), [[0.0, 0.0, 0.094]]]),
             [grid_faces(len(zc), segs), fan(n, np.arange(segs) + n - segs, top=True)])
    band = np.array([np.stack([0.0885 * np.sin(theta), -0.0885 * np.cos(theta) * 1.1, np.full_like(theta, z)], -1)
                     for z in (0.002, 0.014, 0.026)])
    return merge([brim, crown]), (band.reshape(-1, 3), [grid_faces(3, segs)])


def build_hat(mats):
    """SM_AbelHat: worn level, tipped back a touch; its origin is the 'hat' bone's head."""
    felt, band = hat_pieces()
    R = rot_x(-2.0)
    felt = transform(felt, R, HAT_PIVOT)
    band = transform(band, R, HAT_PIVOT)
    V, faces = merge([felt, band])
    n_felt = len(felt[0])
    rgba = np.column_stack([np.full(len(V), COAT / 3.0), np.where(np.arange(len(V)) < n_felt, 0.0, 0.4),
                            np.zeros(len(V)), np.ones(len(V))])
    slots = np.zeros(sum(len(F) for F in faces), np.int32)
    hat = static_object('AbelHat', V, faces, slots, [mats['body']], HAT_PIVOT)
    static_colors(hat.data, rgba_per_vertex=rgba)
    log(f'SM_AbelHat: {sum(len(p.vertices) - 2 for p in hat.data.polygons)} triangles')
    return hat


LANTERN_GRIP = 0.427          # the Keeper's Lantern's SOCKET_Grip height over its base
LANTERN_GLOBE = 0.134         # the globe's middle (it runs from 0.058 to 0.21)


def keepers_lantern():
    """The Keeper's Lantern's mesh as Art/Models/Props/BurialDeck.py builds it (no AO): (V, faces, the source material
    per face). Its own objects are taken out of the scene again."""
    path = os.path.join(lt.REPO, 'Art', 'Models', 'Props', 'BurialDeck.py')
    saved = sys.argv
    sys.argv = [saved[0], '--', '--only=KeepersLantern', '--no-ao']
    before = set(o.name for o in bpy.data.objects)
    try:
        ns = {'__name__': 'burial_deck', '__file__': path}
        exec(compile(open(path, encoding='utf-8').read(), path, 'exec'), ns)
    finally:
        sys.argv = saved
    made = sorted((o for o in bpy.data.objects if o.name not in before), key=lambda o: o.name)
    src = next(o for o in made if o.type == 'MESH' and o.parent is None)
    me = src.data
    V, faces = arrays(me)
    names = [m.name.split('.')[0] for m in me.materials]
    mat_idx = np.empty(len(me.polygons), dtype=np.int64)
    me.polygons.foreach_get('material_index', mat_idx)
    tot = np.empty(len(me.polygons), dtype=np.int64)
    me.polygons.foreach_get('loop_total', tot)
    # arrays() groups faces by corner count; keep each group's materials in the same order.
    per_group = [mat_idx[tot == k] for k in np.unique(tot)]
    for o in made:
        bpy.data.objects.remove(o, do_unlink=True)
    return V, faces, [np.array([names[i] for i in g]) for g in per_group]


def build_lantern(mats):
    """SM_AbelLantern: the Keeper's Lantern gone to spirit, hanging plumb from his left fist, its bail's grip through the
    fist (its origin, the 'lantern' bone's head); SOCKET_Light at the globe's middle."""
    V, faces, names = keepers_lantern()
    A = ARMS[1.0]
    rod = A['rod']
    yaw = math.atan2(rod[1], rod[0])                       # the lantern's grip runs along its X
    R = rot_z(math.degrees(yaw))
    V = (V - np.array([0.0, 0.0, LANTERN_GRIP])) @ R.T + A['G']
    zone = {'BrassWorn': 0.0, 'IronBlack': 1.0, 'HouseTrim': 2.0, 'LanternGlow': 0.0}
    rgba, slots = [], []
    for F, nm in zip(faces, names):
        for name in nm:
            rgba.append((zone.get(name, 0.0) / 3.0, 0.0, 0.0, 1.0))
            slots.append(1 if name == 'LanternGlow' else 0)
    lantern = static_object('AbelLantern', V, faces, slots, [mats['lantern'], mats['flame']], A['G'])
    static_colors(lantern.data, rgba_per_face=np.array(rgba, np.float32))
    sock = lm.socket(lantern, 'Light', (0.0, 0.0, LANTERN_GLOBE - LANTERN_GRIP))
    log(f'SM_AbelLantern: {sum(len(p.vertices) - 2 for p in lantern.data.polygons)} triangles')
    return lantern


def pump_pieces():
    """His Ranchhand pump (Heirloom) in gun space: X along the bore from the stock's wrist (where the fist holds it), Z
    up. A classic walnut pump in the Ranchhand's proportions: a 20 cm receiver, a 56 cm barrel. Pieces by zone: 0 steel,
    1 walnut, 2 dark steel, 3 the bead."""
    pieces = {0: [], 1: [], 2: [], 3: []}

    def loft(sections, segs=16):
        """Rounded sections along x: (x, half width, half height, z middle, power)."""
        a = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
        rings = []
        for x, w, h, zc, p in sections:
            c, s = np.cos(a), np.sin(a)
            e = 2.0 / p
            rings.append(np.stack([np.full_like(a, x), w * np.sign(s) * np.abs(s) ** e,
                                   zc + h * np.sign(c) * np.abs(c) ** e], -1))
        return orient(closed_loft(np.array(rings)))
    pieces[1].append(loft([(0.05, 0.016, 0.022, -0.004, 2.4), (0.0, 0.017, 0.021, -0.008, 2.4),
                           (-0.06, 0.018, 0.026, -0.016, 2.4), (-0.14, 0.02, 0.038, -0.03, 2.6),
                           (-0.24, 0.021, 0.05, -0.044, 2.8), (-0.33, 0.022, 0.06, -0.054, 3.0),
                           (-0.345, 0.021, 0.059, -0.055, 3.0)]))
    pieces[2].append(loft([(-0.343, 0.022, 0.06, -0.055, 3.0), (-0.36, 0.022, 0.06, -0.055, 3.0)]))
    pieces[0].append(loft([(0.04, 0.019, 0.03, 0.0, 3.0), (0.06, 0.022, 0.034, 0.002, 3.4),
                           (0.22, 0.022, 0.034, 0.002, 3.4), (0.245, 0.02, 0.03, 0.004, 3.0)]))
    pieces[0].append(torus((0.085, 0.0, -0.045), (0.0, 1.0, 0.0), 0.022, 0.0035, segs=14, sides=5))
    pieces[2].append(tube([(0.09, 0.0, -0.03), (0.092, 0.0, -0.05)], [0.003, 0.0025], segs=6, domes=1))
    pieces[0].append(tube([(0.24, 0.0, 0.014), (0.8, 0.0, 0.014)], 0.0115, segs=14, domes=1))
    pieces[2].append(tube([(0.24, 0.0, -0.014), (0.66, 0.0, -0.014)], 0.0105, segs=12, domes=1))
    pieces[3].append(ellipsoid((0.795, 0.0, 0.0265), (0.003, 0.003, 0.003), segs=6, rings=4))
    xs = np.linspace(0.34, 0.54, 24)
    C = np.stack([xs, np.zeros_like(xs), np.full_like(xs, -0.014)], -1)

    def grooves(t, a):
        return 0.022 * (1.0 - 0.07 * (np.cos(2.0 * np.pi * t * 9.0) > 0.3) * smoothstep(0.05, 0.12, t)
                        * smoothstep(0.95, 0.88, t))
    pieces[1].append(tube_fn(C, grooves, segs=14, up=(0.0, 0.0, 1.0)))
    return pieces


MUZZLE = np.array([0.8115, 0.0, 0.014])     # where the bore ends, in gun space


def gun_frame(G, bore, up=(0.0, 0.0, 1.0)):
    """Gun space to rig space: X along the bore, Z its top (as near up as the bore allows)."""
    b = unit(bore)
    z = unit(np.asarray(up, float) - b * np.dot(up, b))
    return np.column_stack([b, np.cross(z, b), z]), np.asarray(G, float)


def build_pump(mats):
    """SM_AbelPump in his right fist at rest, its origin the stock's wrist (the 'gun' bone's head); SOCKET_Muzzle at the
    muzzle with its front (Blender's -Y, Unreal's +X) along the bore."""
    A = ARMS[-1.0]
    R, G = gun_frame(A['G'], A['bore'])
    Vs, Fs, zones = [], [], []
    n = 0
    for zone, pieces in pump_pieces().items():
        for V, faces in pieces:
            Vs.append(V @ R.T + G)
            Fs += [np.asarray(F) + n for F in faces]
            zones.append(np.full(len(V), zone / 3.0))
            n += len(V)
    V = np.concatenate(Vs)
    rgba = np.column_stack([np.concatenate(zones), np.zeros(n), np.zeros(n), np.ones(n)])
    slots = np.zeros(sum(len(F) for F in Fs), np.int32)
    pump = static_object('AbelPump', V, Fs, slots, [mats['pump']], G)
    static_colors(pump.data, rgba_per_vertex=rgba)
    b = R[:, 0]
    sock = lm.socket(pump, 'Muzzle', tuple(R @ MUZZLE))
    # The socket's front (-Y) along the bore, its top (+Z) the gun's top; set in the pump's own space (its object's
    # matrix isn't updated yet).
    sock.matrix_basis = Matrix.Translation(Vector(R @ MUZZLE)) @ Matrix(frame(np.cross(-b, R[:, 2]), -b).tolist()).to_4x4()
    log(f'SM_AbelPump: {sum(len(p.vertices) - 2 for p in pump.data.polygons)} triangles')
    return pump


# --- Hit zones ---

HULL_BONES = ('pelvis', 'spine_01', 'spine_02', 'neck', 'head', 'jaw', 'upperarm_l', 'upperarm_r', 'lowerarm_l',
              'lowerarm_r', 'hand_l', 'hand_r', 'tail_01', 'tail_02', 'skirt_f_01', 'skirt_f_02', 'skirt_l_01', 'skirt_r_01')
NO_HULL = ('tail_03', 'tail_04', 'tail_05', 'tail_l_01', 'tail_l_02', 'tail_r_01', 'tail_r_02', 'hat', 'gun', 'lantern')
GUARD_BONES = ('upperarm_l', 'lowerarm_l', 'hand_l', 'lantern')
COAL_REACH = 0.062                                       # the coal's hit sphere
HITS = {}


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


def hit_zones(rig, V, faces, W, names, lantern):
    """A convex hull per bone round the faces it carries most, the coal's sphere, and a hull round the lantern. A face
    counts for every bone that carries one of its corners most, so neighbors overlap a face's width."""
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
    # The lantern's vertices where it hangs at rest (its object stands at its pivot; the matrix isn't updated yet).
    lv = np.array([v.co[:] for v in lantern.data.vertices]) + np.array(lantern.location)
    hull = make_hull(rig, 'lantern', lv)
    HITS['planes']['lantern'] = convex_planes(hull)
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
        far = np.min([np.max(P @ n.T - d, axis=1) for b, (n, d) in HITS['planes'].items() if b != 'lantern'], axis=0)
        raise RuntimeError(f'{gaps} of {len(S)} points on faces with a hit zone lie outside every zone ({where}), up '
                           f'to {1000.0 * far.max():.1f} mm, e.g. at {np.round(P[np.argmax(far)], 3)}')
    log(f'hit zones: {made} hulls and the coal sphere ({100 * COAL_REACH:.1f} cm); every face but the shroud strips\' '
        f'lies inside one ({int(uncovered.sum())} vertices left out)')
    return made + 1


def ray_hull(o, d, n, dd):
    """Where a ray (origin o, unit direction d; arrays of rays) enters and leaves a convex hull (planes n.x <= dd)."""
    nd = d @ n.T
    no = o @ n.T
    t = (dd[None, :] - no) / np.where(np.abs(nd) < 1e-12, 1e-12, nd)
    enter = np.where(nd < 0.0, t, -np.inf).max(1)
    leave = np.where(nd > 0.0, t, np.inf).min(1)
    parallel_out = ((np.abs(nd) < 1e-12) & (no > dd[None, :])).any(1)
    hit = (enter <= leave) & (leave > 0.0) & ~parallel_out
    return np.where(hit, np.maximum(enter, 0.0), np.inf)


def ray_sphere(o, d, c, r):
    oc = o - c
    b = (oc * d).sum(1)
    q = (oc * oc).sum(1) - r * r
    disc = b * b - q
    t = -b - np.sqrt(np.maximum(disc, 0.0))
    return np.where(disc >= 0.0, np.where(t > 0.0, t, np.inf), np.inf)


def coal_guard(transforms=None, radii=(0.05, 0.07, 0.12), yaws=(0, -15, 15, -30, 30), pitches=(0, -10, 10)):
    """How much of the coal the guard covers: shots from the front (yaw, pitch degrees off the coal's way, which faces
    ahead) whose line passes within each radius of the coal's middle; the share whose first hit is the lantern arm or
    the lantern rather than the chest, spine or coal (AUnpaidCreature's crit test). Returns {radius: {(yaw, pitch):
    share}} over the shots that hit him at all."""
    planes = HITS['planes']
    c = HITS['coal']
    T = transforms or {}
    if 'coal' in T:
        c = T['coal'][:3, :3] @ c + T['coal'][:3, 3]
    rng = np.random.default_rng(7)
    out = {}
    for radius in radii:
        k = 900
        rr = radius * np.sqrt(rng.uniform(0.0, 1.0, k))
        ph = rng.uniform(0.0, 2.0 * np.pi, k)
        res = {}
        for yaw in yaws:
            for pitch in pitches:
                # The shot travels toward +Y (into his front), turned by yaw and pitch.
                d = rot_z(yaw) @ rot_x(-pitch) @ np.array([0.0, 1.0, 0.0])
                a = unit(np.cross(d, (0.0, 0.0, 1.0)))
                b = np.cross(a, d)
                pts = c + np.outer(rr * np.cos(ph), a) + np.outer(rr * np.sin(ph), b)
                o = pts - d * 3.0
                D = np.broadcast_to(d, o.shape)
                best = np.full(k, np.inf)
                who = np.array([''] * k, dtype=object)
                for bone, (n, dd) in planes.items():
                    oo, DD = o, D
                    if bone in T:
                        inv = np.linalg.inv(T[bone])
                        oo = o @ inv[:3, :3].T + inv[:3, 3]
                        DD = D @ inv[:3, :3].T
                    t = ray_hull(oo, DD, n, dd)
                    better = t < best
                    best[better] = t[better]
                    who[better] = bone
                t = ray_sphere(o, D, c, COAL_REACH)
                better = t < best
                best[better] = t[better]
                who[better] = 'coal'
                hit = np.isfinite(best)
                guard = np.isin(who, GUARD_BONES) & hit
                res[(yaw, pitch)] = guard.sum() / max(hit.sum(), 1)
        out[radius] = res
    return out


def log_guard(name, transforms=None):
    g = coal_guard(transforms)
    for radius, res in g.items():
        front = res[(0, 0)]
        sides = min(res[(y, 0)] for y in (-15, 15))
        wide = min(res[(y, 0)] for y in (-30, 30))
        tilted = min(res[(0, p)] for p in (-10, 10))
        log(f'{name}: shots within {100 * radius:.0f} cm of the coal blocked by the lantern arm: {100 * front:.0f}% '
            f'straight on, {100 * sides:.0f}% at 15 deg to a side, {100 * wide:.0f}% at 30, {100 * tilted:.0f}% 10 deg '
            f'above or below')
    return g


def build():
    rig, body, mats, raw_colors, (V, faces, W, names) = build_rig()
    hat = build_hat(mats)
    lantern = build_lantern(mats)
    pump = build_pump(mats)
    hit_zones(rig, V, faces, W, names, lantern)
    log_guard('idle')
    islands, holes, largest = shroud_islands(LAYOUT['strips'])
    log(f'shroud: over a whole period of the fade noise\'s drift, {islands} floating bits and {holes} holes or bites in '
        f'the strips' + (f' (the largest {1000.0 * largest:.0f} mm across)' if islands or holes else ''))
    return rig, body, hat, lantern, pump, raw_colors


RIG, BODY, HAT, LANTERN, PUMP, RAW_COLORS = build()
PROPS = {'hat': (HAT, HAT_PIVOT), 'lantern': (LANTERN, ARMS[1.0]['G']), 'gun': (PUMP, ARMS[-1.0]['G'])}


# --- Poses: what the game's code makes of the rig, solved here bone by bone ---
# A pose turns bones about their joints in the rest pose's axes, each under its parent's turn (AUnpaidCreature's
# Turned = Above * Own), and may shift the pelvis. The arms are placed by where the fist holds its prop (grip), how the
# fist is turned (f along the fingers, u its back) and which way the elbow points (pole), the elbow found as the game's
# two-bone solve finds a knee. The lantern hangs plumb from the fist (it only turns about the vertical); the pump stays
# in the fist unless a pose turns it in the hand, or lets it go (the kneel places its bone on the boards, off the hand).
# Fingers turn about their knuckles, under their hand. In the kneel and the sit the shroud's links swing back to lie on
# the boards (or the bier) instead of going through them.

REST = {b.name: (np.array(b.head_local), np.array(b.matrix_local.to_3x3()), b.parent.name if b.parent else None)
        for b in RIG.data.bones}
ORDER = [b.name for b in RIG.data.bones]
SHROUD_LINKS = ('tail_01', 'tail_02', 'tail_03', 'tail_04', 'tail_05', 'tail_l_01', 'tail_l_02', 'tail_r_01', 'tail_r_02')
# The shroud lying on the boards (the kneel, the sit): one veil, not a fan. Its links drift to his left, with the canyon
# wind, by LIE_DRIFT degrees at the tips (less the nearer the pelvis; on the bier only SIT_DRIFT, which keeps the tips on
# its board at his 1.3), and the side chains turn LIE_GATHER degrees in toward the middle, so the side strips lie along
# the middle one and the strips part only near their tips.
LIE_DRIFT, SIT_DRIFT, LIE_GATHER = 18.0, 8.0, 9.0
LIE_ALONG = dict(tail_01=0.0, tail_02=0.25, tail_03=0.5, tail_04=0.75, tail_05=1.0, tail_l_01=0.5, tail_l_02=0.85,
                 tail_r_01=0.5, tail_r_02=0.85)
# How high each link's tip lies over the boards (m, at size 1), fitted to the mesh link by link so that the shroud's
# underside rests 5 mm over them all along: the tube is 24 cm through by the pelvis, and the strips' folds hang lowest
# where they part (so tail_02's tip lies highest). The side strips' tips are carried partly by the middle chain, so
# tail_r_02's own tip lies under the boards while its strip rests on them. On the bier (SIT_REST) the same, fitted at
# his 1.3 over the bier's two lashings and, at the tips, its head block.
LIE_REST = dict(tail_01=0.125, tail_02=0.164, tail_03=0.08, tail_04=0.038, tail_05=0.03, tail_l_01=0.055,
                tail_l_02=0.038, tail_r_01=0.065, tail_r_02=-0.031)
SIT_REST = dict(tail_01=0.144, tail_02=0.15, tail_03=0.083, tail_04=0.037, tail_05=0.03, tail_l_01=0.066,
                tail_l_02=0.024, tail_r_01=0.09, tail_r_02=0.056)
SIDE_SKIRTS = ('skirt_l_01', 'skirt_r_01')


def side_skirt(sides, name, T, J):
    """A side of the skirts in a pose (its whole turn and its head): pitched as skirt_f_01 is or to its own pitch (a turn
    about +X, as the lap's), rolled about its length by roll degrees (negative: what would hang under the lap turns in under
    it, onto the boards), its pivot (the coat's side at mid-thigh) where skirt_f_01 carries it, lowered by drop (m)."""
    s = 1.0 if name == 'skirt_l_01' else -1.0
    Rf, Jf, Hf = T['skirt_f_01'], J['skirt_f_01'], REST['skirt_f_01'][0]
    Rb = rot_x(sides['pitch']) if 'pitch' in sides else Rf
    u = Rb @ np.array([0.0, 0.0, -1.0])
    roll = sides.get('roll', 0.0)
    R = rotation(u, math.radians(abs(roll)))
    if roll and (R @ Rb @ np.array([0.0, 1.0, 0.0]))[0] * s * math.copysign(1.0, roll) < 0.0:
        R = rotation(u, -math.radians(abs(roll)))
    R = R @ Rb
    Q = SIDE_PIVOTS[s]
    target = Jf + Rf @ (Q - Hf) - np.array([0.0, 0.0, sides.get('drop', 0.0)])
    return R, target - R @ (Q - REST[name][0])


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


def arm_turns(side, S1, arm):
    """The whole turns of the upper arm, forearm and hand that put this side's fist on its grip."""
    A = ARMS[side]
    f1, u1 = unit(arm['f']), unit(arm['u'])
    W1 = wrist_for(arm['grip'], f1, u1)
    L1, L2 = np.linalg.norm(A['E'] - A['S']), np.linalg.norm(A['W'] - A['E'])
    E1, W1, reached = two_bone(S1, W1, L1, L2, arm['pole'])
    if not reached:
        log(f'  the {SUFFIX[side]} fist falls {1000 * np.linalg.norm(W1 - wrist_for(arm["grip"], f1, u1)):.0f} mm short')
    n0 = unit(np.cross(A['E'] - A['S'], A['W'] - A['E']))
    n1 = unit(np.cross(E1 - S1, W1 - E1))

    def bone_frame(v, n):
        return frame(v, np.cross(n, unit(v)))
    R_ua = bone_frame(E1 - S1, n1) @ bone_frame(A['E'] - A['S'], n0).T
    R_la = bone_frame(W1 - E1, n1) @ bone_frame(A['W'] - A['E'], n0).T
    R_h = frame(f1, u1) @ frame(A['f'], A['u']).T
    return R_ua, R_la, R_h


def solve(spec):
    """Every bone's whole turn (3x3, Blender axes) and joint (its head) for a pose."""
    own, totals, shift = spec.get('own', {}), dict(spec.get('total', {})), spec.get('shift', {})
    floor = spec.get('floor')
    T, J = {}, {}
    for name in ORDER:
        head, _, parent = REST[name]
        if parent is not None:
            above = T[parent]
            joint = J[parent] + above @ (head - REST[parent][0])
        else:
            above = np.eye(3)
            joint = head.copy()
        joint = joint + np.asarray(shift.get(name, (0.0, 0.0, 0.0)), float)
        side = 1.0 if name.endswith('_l') else -1.0
        if name.startswith('upperarm_') and side in spec.get('arms', {}):
            arm = spec['arms'][side]
            if arm == 'guard':
                # The guard held where it was before the coal, wherever the chest carries the coal, still facing ahead.
                A = ARMS[side]
                x = unit(A['W'] - A['S'])
                arm = dict(grip=J['coal'] + (A['G'] - REST['coal'][0]), f=A['f'], u=A['u'],
                           pole=(A['E'] - A['S']) - x * np.dot(A['E'] - A['S'], x))
            R_ua, R_la, R_h = arm_turns(side, joint, arm)
            totals[name], totals[f'lowerarm_{SUFFIX[side]}'], totals[f'hand_{SUFFIX[side]}'] = R_ua, R_la, R_h
        if name == 'lantern' and name not in totals:
            # Plumb: the lantern turns only about the vertical, as far as the fist turns its bail's grip.
            rod0 = ARMS[1.0]['rod']
            rod1 = T['hand_l'] @ rod0
            yaw = math.atan2(rod1[1], rod1[0]) - math.atan2(rod0[1], rod0[0])
            totals[name] = rot_z(math.degrees(yaw))
        turned = totals[name] if name in totals else above @ own.get(name, np.eye(3))
        if name in spec.get('place', {}):
            # Let go of: a prop's bone set down where the pose puts it (its head and whole turn), off its hand.
            joint, turned = (np.asarray(v, float) for v in spec['place'][name])
        if name in SIDE_SKIRTS and 'sides' in spec:
            turned, joint = side_skirt(spec['sides'], name, T, J)
        if floor is not None and name in SHROUD_LINKS and name not in totals:
            # The shroud lies along the boards like cloth: each link keeps the way it trails at rest, seen from above,
            # and tilts so its tip lies at its height over the boards (down to them, or up over the cloth bunched
            # before it).
            tip = np.array(RIG.data.bones[name].tail_local) - head
            length = float(np.linalg.norm(tip))
            d = turned @ (tip / length)
            # Lying, the strips gather into one veil drifting to his left with the canyon wind: each link turns about
            # the vertical by the drift as far down the shroud as it is, and the side chains turn in toward the middle.
            lie = spec['lie']
            side = 1.0 if '_l_' in name else (-1.0 if '_r_' in name else 0.0)
            want = rot_z(-lie['drift'] * LIE_ALONG[name] + side * lie['gather']) @ (tip / length)
            h = np.array([want[0], want[1], 0.0])
            h = unit(h) if np.linalg.norm(h) > 1e-6 else np.array([0.0, 1.0, 0.0])
            rise = float(np.clip((floor + lie['rest'][name] - joint[2]) / length, -1.0, 1.0))
            want = h * math.sqrt(1.0 - rise * rise) + np.array([0.0, 0.0, rise])
            axis = np.cross(d, want)
            angle = math.atan2(np.linalg.norm(axis), float(np.dot(d, want)))
            if np.linalg.norm(axis) > 1e-9:
                turned = rotation(axis, angle) @ turned
        T[name], J[name] = turned, joint
    return T, J


# The kneel at zero, as the user approved it in the concept: beaten, his right hand risen to his mouth and beard, the
# fingers loosely curled; the pump dropped on the boards before him; the lantern standing by his left knee under his
# hand.
# The right hand: a loose fist risen to his mouth, the back of the hand up and toward Ellis, the fingers curling down
# over his beard, the curled index finger's side at his lips. Its knuckles' middle from his lips, in his head's axes
# (m), and the way the fingers point from the wrist (across his mouth toward his left, a little up).
KNEEL_HAND = dict(ahead=0.09, up=-0.03, right=0.015, across=1.0, rise=0.25, back_up=0.8, back_ahead=0.6)
KNEEL_UNCURL = (('index', 18.0), ('middle', 20.0), ('ring', 24.0), ('pinky', 28.0))   # degrees each finger opens
PUMP_DOWN = dict(at=(-0.3, -0.7), bore=(0.95, -0.3))     # where the dropped pump's wrist lies, and the way its bore lies


def pump_lying():
    """The pump dropped on the boards before his knees, lying on its right side, its bore across in front of him: the
    'gun' bone's head (the stock's wrist) and its whole turn from rest, in the rig's space."""
    lowest = min(float(np.min(V[:, 1])) for pieces in pump_pieces().values() for V, F in pieces)
    b = unit(np.array([*PUMP_DOWN['bore'], 0.0]))
    up = np.array([0.0, 0.0, 1.0])
    lying = np.column_stack([b, up, np.cross(b, up)])      # gun space: x the bore, y its left side (up), z its top
    rest, _ = gun_frame(ARMS[-1.0]['G'], ARMS[-1.0]['bore'])
    return np.array([*PUMP_DOWN['at'], 0.001 - lowest]), lying @ rest.T


def kneel_spec():
    """At zero: on his knees on the boards, sat back on his heels, the coat's front a lap over his thighs and its hem on
    the boards before his knees; the lantern set down beside his left knee, his hand still on its bail; his right hand
    risen to his mouth, the fingers loosely curled over his beard; the pump dropped on the boards before him; the coal
    open, an ember. His body bows, but his head lifts to whoever stands before him ("...El? You came home.")."""
    f, u, t = lantern_hand((0.08, -0.2, -1.0), (0.0, 1.0, 0.0))
    # Sat back on his heels, his hips 30 cm over the boards: the thighs slope from them to his knees on the boards (the
    # lap 28 degrees down), the coat's front falls straight from his knees to the boards, and its sides lie a little
    # flatter and lower than the lap, down onto the boards. The shroud's top joint drops 8.6 cm, so its tube comes down
    # from under the coat to lie on the boards right behind him.
    spec = dict(shift={'pelvis': (0.0, 0.03, -0.7), 'tail_01': (0.0, 0.0, -0.086)}, floor=0.0,
                lie=dict(drift=LIE_DRIFT, gather=LIE_GATHER, rest=LIE_REST),
                own={'pelvis': rot_x(6.0), 'spine_01': rot_x(8.0), 'spine_02': rot_x(10.0),
                     'neck': rot_x(-12.0), 'head': rot_x(-22.0)},
                total={'skirt_f_01': rot_x(-62.0), 'skirt_f_02': rot_x(6.0)},
                sides=dict(pitch=-72.0, drop=0.04),
                arms={1.0: dict(grip=(0.33, -0.27, 0.427), f=f, u=u, pole=(0.6, 0.6, -0.4))})
    # Where his lips are, knelt (the right arm doesn't carry the head): the hand is set by them.
    T, J = solve(spec)
    head, rest = T['head'], REST['head'][0]
    d = unit(np.array([0.0, -1.0, MOUTH_Z / 0.095]))
    lips = J['head'] + head @ (HEAD_CENTER + d * float(face_radius(d[None])[0]) - rest)
    ahead, up, right = head @ np.array([0.0, -1.0, 0.0]), head @ np.array([0.0, 0.0, 1.0]), head @ np.array([-1.0, 0.0, 0.0])
    H = KNEEL_HAND
    knuckles = lips + ahead * H['ahead'] + up * H['up'] + right * H['right']
    fr = unit(-right * H['across'] + up * H['rise'])
    back = up * H['back_up'] + ahead * H['back_ahead']
    ur = unit(back - fr * np.dot(back, fr))
    W = knuckles - fr * 0.088 * HAND_SCALE * 1.05 ** 0.3
    spec['arms'][-1.0] = dict(grip=grip_of(W, fr, ur), f=fr, u=ur, pole=(-0.8, -0.2, -1.0))
    # The fist round the pump's wrist opens: each finger turns back about its knuckle, the way it curled.
    A = ARMS[-1.0]
    curl_axis = np.cross(A['u'], A['f'])
    for finger, degrees in KNEEL_UNCURL:
        spec['own'][finger_bone(finger, 'r')] = rotation(curl_axis, -math.radians(degrees))
    spec['place'] = {'gun': pump_lying()}
    return spec


def pose_specs():
    """The poses the boss and the friend need, as solve() takes them."""
    up = np.array([0.0, 0.0, 1.0])
    poses = {'idle': {}}
    # The lantern flare before the buckshot (a one-second tell): he thrusts the lantern up and out before his chest, the
    # forearm standing over the coal and the lantern hanging in front of it (the coal stays covered: he is fighting),
    # the fist under his chin, clear of his face; the pump comes up level at his hip.
    f, u, t = lantern_hand((0.0, -0.3, 1.0), (1.0, 0.0, 0.0))
    fr, ur, b = gun_hand((0.02, -1.0, -0.05), (0.0, 0.0, -1.0))
    poses['flare'] = dict(own={'spine_02': rot_x(-3.0), 'neck': rot_x(-3.0), 'head': rot_x(-5.0)},
                          arms={1.0: dict(grip=(0.05, -0.4, 1.53), f=f, u=u, pole=(0.6, -0.1, -1.0)),
                                -1.0: dict(grip=(-0.25, -0.33, 1.0), f=fr, u=ur, pole=(-0.6, 0.5, -0.4))})
    # The shot: the pump thrust out level at the target from the hip, the lantern still guarding the coal.
    fr, ur, b = gun_hand((0.0, -1.0, 0.0), (0.0, 0.0, -1.0))
    poses['fire'] = dict(own={'spine_02': rot_x(-2.0)},
                         arms={-1.0: dict(grip=(-0.2, -0.47, 1.14), f=fr, u=ur, pole=(-0.7, 0.3, -0.5))})
    # The stock lunge: thrown forward, the right shoulder driving, the pump flipped in his fist so the butt leads at the
    # target's chest and the barrel trails back along his forearm.
    bore = unit([-0.18, 0.95, -0.22])
    fr, ur, _ = gun_hand(-bore, (0.0, 0.0, -1.0))
    gun_total = frame(bore, up - bore * np.dot(up, bore))
    gun_rest = frame(ARMS[-1.0]['bore'], up - ARMS[-1.0]['bore'] * np.dot(up, ARMS[-1.0]['bore']))
    poses['lunge'] = dict(own={'pelvis': rot_x(10.0), 'spine_01': rot_x(5.0), 'spine_02': rot_z(14.0) @ rot_x(6.0),
                               'neck': rot_z(-8.0) @ rot_x(-7.0), 'head': rot_z(-6.0) @ rot_x(-10.0),
                               'tail_01': rot_x(15.0), 'tail_02': rot_x(6.0)},
                          shift={'pelvis': (0.0, -0.08, -0.03)},
                          total={'gun': gun_total @ gun_rest.T},
                          arms={1.0: 'guard', -1.0: dict(grip=(-0.06, -0.62, 1.22), f=fr, u=ur, pole=(-0.9, 0.2, -0.4))})
    # Toward the sunset (every 12 s, and stunned by the saint's pull): the lantern lowered to his side, the pump down,
    # his head lifted to the light. The coal is open.
    f, u, t = lantern_hand((0.02, -0.12, -1.0), (0.0, 1.0, 0.0))
    fr, ur, b = gun_hand((0.05, -0.5, -0.86), (0.0, 1.0, -0.2))
    poses['sunset'] = dict(own={'spine_02': rot_x(-4.0), 'neck': rot_x(-4.0), 'head': rot_x(-9.0)},
                           arms={1.0: dict(grip=(0.23, -0.06, 0.83), f=f, u=u, pole=(0.15, 1.0, 0.0)),
                                 -1.0: dict(grip=(-0.27, -0.14, 0.93), f=fr, u=ur, pole=(-1.0, 0.4, 0.0))})
    poses['kneel'] = kneel_spec()
    # After the fight, on his own bier facing the sunset (the actor's origin at the bier's SOCKET_Sit, on the board):
    # his lap on the board, the knees over its foot, the shroud lying along the board behind him; the pump across his
    # lap, the lantern hanging from his left fist past his knee. He sits on the shroud's top: its first joint lifts
    # 11 cm, so the tube lies on the board instead of through it, and is pressed 8 cm forward under him, so that at his
    # 1.3 the shroud ends on the board (1.99 m behind the seat; the board ends at 2.0 m), lying over the head block.
    f, u, t = lantern_hand((0.0, -0.6, -0.8), (1.0, 0.0, 0.0))
    fr, ur, b = gun_hand((1.0, 0.0, 0.02), (0.0, -0.8, -0.6))
    # The coat's sides by his hips lie on the board beside him: level, rolled in under the lap, 4 cm higher than the lap
    # would carry them.
    poses['sit'] = dict(shift={'pelvis': (0.0, 0.03, -0.9), 'tail_01': (0.0, -0.08, 0.11)}, floor=0.0,
                        lie=dict(drift=SIT_DRIFT, gather=LIE_GATHER, rest=SIT_REST),
                        own={'pelvis': rot_x(-3.0), 'spine_01': rot_x(4.0), 'spine_02': rot_x(5.0), 'neck': rot_x(2.0),
                             'head': rot_x(-3.0)},
                        total={'skirt_f_01': rot_x(-88.0), 'skirt_f_02': rot_x(-2.0)},
                        sides=dict(pitch=-92.0, roll=-60.0, drop=-0.04),
                        arms={1.0: dict(grip=(0.13, -0.5, 0.2), f=f, u=u, pole=(1.0, 0.3, 0.0)),
                              -1.0: dict(grip=(-0.12, -0.3, 0.16), f=fr, u=ur, pole=(-1.0, 0.3, 0.0))})
    return poses


POSES = pose_specs()


def to_ue(p):
    """A Blender rig-space point (m) in Unreal's component space (cm)."""
    return np.array([-p[1], -p[0], p[2]]) * 100.0


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


def pose_table():
    """Every pose's bones in Unreal's component space: rest head, posed head (cm at size 1), the whole turn from rest
    and the bone's own turn under its parent's, as FQuat (X, Y, Z, W) and axis-angle. Logged and written as JSON."""
    out = {}
    for name, spec in POSES.items():
        T, J = solve(spec)
        bones = {}
        for bone in ORDER:
            parent = REST[bone][2]
            turn = T[bone]
            own = (T[parent].T @ turn) if parent else turn
            if name != 'idle' and np.allclose(turn, np.eye(3), atol=1e-6) and np.allclose(J[bone], REST[bone][0], atol=1e-6):
                continue
            qt, qo = quat_ue(turn), quat_ue(own)
            bones[bone] = dict(rest=(np.round(to_ue(REST[bone][0]), 2) + 0.0).tolist(),
                               head=(np.round(to_ue(J[bone]), 2) + 0.0).tolist(),
                               turn=(np.round(qt, 5) + 0.0).tolist(), own=(np.round(qo, 5) + 0.0).tolist())
            if name != 'idle':
                axis, angle = axis_angle(qt)
                oaxis, oangle = axis_angle(qo)
                log(f'POSE {name} {bone}: {tuple(np.round(to_ue(REST[bone][0]), 1))} -> {tuple(np.round(to_ue(J[bone]), 1))}'
                    f', turn {angle:.1f} deg about {tuple(np.round(axis, 3))}, own {oangle:.1f} deg about '
                    f'{tuple(np.round(oaxis, 3))}')
        out[name] = bones
    os.makedirs(WORK, exist_ok=True)
    path = os.path.join(WORK, 'Abel_poses.json')
    head = dict(units='Unreal component space at size 1 (the actor scales it 1.3): cm; x forward, y right, z up',
                quaternions='FQuat (X, Y, Z, W); turn = the whole turn from rest; own = the turn under the parent\'s '
                            '(Turned = Above * Own); in a pose, a bone not listed stays at rest under its parent',
                shift='a bone whose head is off where its parent carries it is shifted (Shift = its head here minus '
                      'that): the pelvis in the lunge, the kneel and the sit, the shroud\'s tail_01 in the kneel '
                      '(0, 0, -8.6) and the sit (8, 0, 11), and the coat\'s side bones skirt_l_01 and skirt_r_01 in the '
                      'kneel and the sit; in both, the shroud\'s links lie as listed (its swing, FShroudChain, must not '
                      'drive them)',
                sides='skirt_l_01 and skirt_r_01 (under the pelvis) carry the coat\'s sides above the knee; only the '
                      'kneel and the sit move them (elsewhere they stay at rest under the pelvis)',
                sit='the actor stands at the bier\'s SOCKET_Sit (on the board, facing as the socket faces); the '
                    'lying shroud is fitted to the bier at the actor\'s 1.3',
                kneel='the pump is let go: the gun bone\'s head and turn are where it lies on the boards (set the bone, '
                      'or the detached SM_AbelPump, to them); its own turn under hand_r means nothing there')
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
    return out


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


def pose_gaps():
    """How much of the posed surface the hit zones (rigid on their bones, as in the physics asset) miss, and where."""
    ev = BODY.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m = ev.to_mesh()
    P = np.empty(len(m.vertices) * 3)
    m.vertices.foreach_get('co', P)
    ev.to_mesh_clear()
    S, who = face_samples(P.reshape(-1, 3), HITS['faces'], HITS['keep'], HITS['owner'])
    gaps = outside_hits(S, bone_transforms())
    where = ', '.join(f'{b} {n}' for b, n in Counter(who[gaps].tolist()).most_common(3))
    return 100.0 * gaps.mean(), where


def hand_clearance(sfx):
    """How near the posed hand comes to his face (beard and moustache included) and to the hat: the least signed
    distance of its vertices from the face's surface (negative: inside it), how many are inside, and the least distance
    from the hat (m)."""
    from mathutils.bvhtree import BVHTree
    ev = BODY.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m = ev.to_mesh()
    P = np.empty(len(m.vertices) * 3)
    m.vertices.foreach_get('co', P)
    ev.to_mesh_clear()
    P = P.reshape(-1, 3)
    hand = np.concatenate([np.arange(a, b) for a, b in LAYOUT[f'Hand_{sfx}']])
    face = BVHTree.FromPolygons([tuple(p) for p in P], [list(f) for F in LAYOUT['head_faces'] for f in F.tolist()])
    worst, inside = np.inf, 0
    for p in P[hand]:
        loc, normal, _, _ = face.find_nearest(Vector(p))
        signed = (Vector(p) - loc).dot(normal)
        worst = min(worst, signed)
        inside += signed < 0.0
    world = HAT.matrix_world
    hat = BVHTree.FromPolygons([world @ v.co for v in HAT.data.vertices], [list(pl.vertices) for pl in HAT.data.polygons])
    near_hat = min(hat.find_nearest(Vector(p))[3] for p in P[hand])
    return worst, inside, near_hat


def lying_shroud():
    """The posed shroud's lowest point over the floor, how far back it reaches and how far out to a side (m, size 1)."""
    ev = BODY.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m = ev.to_mesh()
    P = np.empty(len(m.vertices) * 3)
    m.vertices.foreach_get('co', P)
    ev.to_mesh_clear()
    P = P.reshape(-1, 3)[np.concatenate([np.arange(a, b) for a, b in LAYOUT['Shroud']])]
    return float(P[:, 2].min()), float(P[:, 1].max()), float(np.abs(P[:, 0]).max())


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


def log_pump_down():
    """The dropped pump in the kneel, two ways for the code: the 'gun' bone's head and whole turn in component space
    (override the bone), and the same as the prop's own transform (detach it: SM_AbelPump is modeled in its rest grip, so
    its component-space transform is exactly the bone's)."""
    head, turn = POSES['kneel']['place']['gun']
    q = quat_ue(turn)
    pitch, yaw, roll = rotator_ue(q)
    bore = to_ue(turn @ ARMS[-1.0]['bore']) / 100.0
    log(f'kneel: the pump on the boards: gun bone head {tuple(np.round(to_ue(head), 2))} cm (component space, size 1), '
        f'whole turn FQuat {tuple(np.round(q, 5))} = FRotator(P {pitch:.2f}, Y {yaw:.2f}, R {roll:.2f}); its bore '
        f'{tuple(np.round(bore, 3))}')


TABLE = pose_table()
for pose_name in POSES:
    if pose_name != 'idle':
        apply(POSES[pose_name])
        g = coal_guard(bone_transforms(), radii=(0.05,), yaws=(0, -15, 15), pitches=(0,))[0.05]
        share, where = pose_gaps()
        log(f'{pose_name}: shots within 5 cm of the coal blocked by the lantern arm: {100 * g[(0, 0)]:.0f}% straight on, '
            f'{100 * min(g[(-15, 0)], g[(15, 0)]):.0f}% at 15 deg; {share:.1f}% of the surface outside the hit zones'
            + (f' ({where})' if where else ''))
        if 'floor' in POSES[pose_name]:
            low, back, wide = lying_shroud()
            log(f'{pose_name}: the lying shroud {1000.0 * low:.0f} mm over the boards at its lowest; at his {SCALE} it '
                f'reaches {SCALE * back:.2f} m back and {SCALE * wide:.2f} m out to a side'
                + (' (the bier\'s board: 2.0 m back, 0.40 m out)' if pose_name == 'sit' else ''))
        if pose_name == 'kneel':
            worst, inside, near_hat = hand_clearance('r')
            log(f'kneel: the right hand at his mouth: {inside} of its vertices inside his face or beard, the nearest '
                f'{1000.0 * worst:.1f} mm off it; {1000.0 * near_hat:.0f} mm from the hat')
            log_pump_down()
apply(POSES['idle'])


# --- Previews (Saved/ArtPreviews/RansomsRest/Abel) ---

def want_preview():
    return '--preview' in ARGV or lt.want_preview()


def preview_material(name, look, colors, slot_coal=False, rank=BOSS_COAL, rank_alpha=1.0):
    """M_Ghost as Tools/Unreal/build_creature_materials.py builds it, for Eevee: the zone color by vertex R, the Polymer
    grain on UV 0 x UVScale, cavity G; emissive the rim (1 - facing) ^ RimPower, the glow and the ember edge in the rank's
    color; the fade's two noises on UV 0 x NoiseScale against vertex A, hashed (dithered). Coal: the crust and its cracks
    in the rank's color."""
    mat = bpy.data.materials.new(f'Preview{name}')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])

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

    def rgb(hex_text):
        return tuple(lt.hex_color(int(hex_text.lstrip('#'), 16))[:3]) if isinstance(hex_text, str) else \
            tuple(lt.hex_color(hex_text)[:3])

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
    rank_rgb = vscale(vmath('ADD', rgb(rank), 0.0), rank_alpha)
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
    grain_c = image(lt.texture_path('Polymer', 'BC'), 'sRGB', look.get('UVScale', 1.0))
    luma = nodes.new('ShaderNodeRGBToBW')
    links.new(grain_c, luma.inputs['Color'])
    grain = math_('MINIMUM', math_('MAXIMUM', math_('MULTIPLY', luma.outputs['Val'], 1.25), 0.5), 1.5)
    dark = math_('SUBTRACT', 1.0, math_('MULTIPLY', sep.outputs['Green'], 0.93))
    body = vscale(vscale(ramp.outputs['Color'], grain), dark)
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
    for obj in (BODY, HAT, LANTERN, PUMP):
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
    look = dict(GHOST_LOOK)
    body = preview_material('Body', look, ZONE_COLORS)
    BODY.data.materials[0] = body
    BODY.data.materials[1] = preview_material('Coal', dict(EmberStrength=2.1), ZONE_COLORS, slot_coal=True)
    HAT.data.materials[0] = body
    LANTERN.data.materials[0] = preview_material('Lantern', look, LANTERN_COLORS)
    flame = dict(RimColor=RIM_COLOR, RimPower=4.0, RimStrength=0.6, GlowStrength=5.0, EmberStrength=0.0, NoiseScale=1.4,
                 FadeSoftness=0.07, UVScale=0.02)
    LANTERN.data.materials[1] = preview_material('Flame', flame, (FLAME_COLOR,) * 4)
    PUMP.data.materials[0] = preview_material('Pump', look, PUMP_COLORS)


def place(location=(0.0, 0.0, 0.0), turn=0.0, scale=SCALE):
    """Stands the rig (and so the props, which follow its bones) at location, turned about Z, at the actor's size."""
    RIG.matrix_world = Matrix.Translation(Vector(location)) @ Matrix.Rotation(turn, 4, 'Z') @ Matrix.Scale(scale, 4)
    bpy.context.view_layer.update()
    for name, (obj, pivot) in PROPS.items():
        rest = RIG.data.bones[name].matrix_local
        obj.matrix_world = RIG.matrix_world @ RIG.pose.bones[name].matrix @ rest.inverted() @ Matrix.Translation(Vector(pivot))
    bpy.context.view_layer.update()


def world_point(p):
    return np.array(RIG.matrix_world @ Vector(p))


def ghost_light(where, energy=12.0):
    """The lantern's light, as the level adds it: a shadowless point light at SOCKET_Light."""
    sock = next(c for c in LANTERN.children if c.name.startswith('SOCKET_Light'))
    lamp = bpy.data.objects.new('GhostLight', bpy.data.lights.new('GhostLight', 'POINT'))
    lamp.data.energy = energy * SCALE * SCALE
    lamp.data.color = lt.hex_color(int(FLAME_COLOR[1:], 16))[:3]
    lamp.data.shadow_soft_size = 0.06
    lamp.data.use_shadow = False
    lamp.location = sock.matrix_world.translation
    where.objects.link(lamp)
    return lamp


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
    import AbelConcepts as ac
    return ac


def render(path, resolution, samples=64):
    ac = concepts()
    os.makedirs(os.path.dirname(path), exist_ok=True)
    ac.uc.render(path, resolution, samples=samples)
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
    render(path, resolution, samples)
    bpy.data.objects.remove(cam)


def deck_stage(mode, where):
    """The burial deck on Gravewind Point (the concept's stage: deck, biers and lantern posts at their sockets, the sky,
    the sun, the canyon's ridges and fog)."""
    ac = concepts()
    ac.stage(mode, where)
    return ac


def studio_stage(where, sun=(-0.45, -0.85, 0.42)):
    ac = concepts()
    ac.uc.ground(where, 0xbdb4a4, plain=True)
    ac.uc.sky(0.8)
    ac.uc.sun(sun, strength=3.8, where=where)
    return ac


def shot_debug():
    """Quick checks into Intermediate/AbelModel: the rest pose front, three-quarter, side and back; the face."""
    where = stage_collection('Debug')
    studio_stage(where)
    apply(POSES['idle'])
    place()
    c = Vector((0.0, 0.0, 1.05 * SCALE))
    paths = []
    for name, d in (('front', (0.0, -1.0, 0.05)), ('three', (0.8, -1.0, 0.1)), ('side', (1.0, 0.0, 0.05)),
                    ('back', (0.3, 1.0, 0.1))):
        path = os.path.join(WORK, f'debug_{name}.png')
        lens_shot(where, path, c + Vector(d).normalized() * 8.0, c, 50.0, (700, 900), samples=16)
        paths.append(path)
    concepts().uc.compose(paths, os.path.join(WORK, 'debug_strip.png'))
    head = Vector(world_point(HEAD_CENTER))
    face = []
    for name, d in (('face_front', (0.0, -1.0, 0.0)), ('face_three', (0.6, -1.0, 0.05)), ('face_side', (1.0, -0.15, 0.0))):
        path = os.path.join(WORK, f'debug_{name}.png')
        lens_shot(where, path, head + Vector(d).normalized() * 0.75, head + Vector((0.0, 0.0, -0.03)), 85.0, (800, 800),
                  samples=24)
        face.append(path)
    concepts().uc.compose(face, os.path.join(WORK, 'debug_face.png'))
    strike(where)


AT_EDGE = (-5.3, -10.5)          # the concept's hero spot, two meters in from the deck's open edge


def pose_jaw(spec, degrees):
    """A pose with the jaw opened a little more (speaking)."""
    spec = dict(spec)
    own = dict(spec.get('own', {}))
    own['jaw'] = rot_x(degrees) @ own.get('jaw', np.eye(3))
    spec['own'] = own
    return spec


def snapshot(where):
    """Copies of the figure as it stands now (the posed skin baked into a plain mesh, the props), for scenes that show it
    more than once."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(BODY.evaluated_get(depsgraph))
    body = bpy.data.objects.new('AbelCopy', mesh)
    body.matrix_world = BODY.matrix_world.copy()
    where.objects.link(body)
    objs = [body]
    for obj in (HAT, LANTERN, PUMP):
        copy = obj.copy()
        copy.matrix_world = obj.matrix_world.copy()
        where.objects.link(copy)
        objs.append(copy)
    return objs


def hide_figure(hidden=True):
    for obj in (BODY, HAT, LANTERN, PUMP):
        obj.hide_render = hidden


def shot_dusk():
    """On the deck's open edge at dusk, the sun setting behind him: the concept's hero view, and beside the concept."""
    where = stage_collection('Dusk')
    deck_stage('dusk', where)
    apply(POSES['idle'])
    place((*AT_EDGE, 0.0), math.pi)
    ghost_light(where)
    target = Vector((AT_EDGE[0], AT_EDGE[1], 1.5))
    path = os.path.join(OUT, 'Abel_dusk.png')
    lens_shot(where, path, target + Vector((2.2, 4.3, -0.4)), target, 33.0, (1600, 1000), samples=64)
    concept = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'Abel', 'Abel_A_dusk.png')
    if os.path.exists(concept):
        concepts().uc.compose([concept, path], os.path.join(OUT, 'Abel_vs_concept.png'))
    # The same light from the other side: what the player walking up the deck sees, his silhouette against the sun.
    lens_shot(where, os.path.join(OUT, 'Abel_dusk_back.png'), target + Vector((-3.0, 7.5, 0.2)), target, 35.0,
              (1600, 1000), samples=64)
    strike(where)


def shot_face():
    """His face at the scene's distance (1.2 m from the player's eyes, a normal lens): knelt at zero, the lantern on the
    boards lighting him from below, the sunset behind; the same as he speaks; and standing in the fight."""
    where = stage_collection('Face')
    deck_stage('dusk', where)
    spot = (AT_EDGE[0], AT_EDGE[1] + 1.0)
    paths = []
    for name, spec, eye_height in (('kneel', POSES['kneel'], 1.65), ('speak', pose_jaw(POSES['kneel'], 9.0), 1.65),
                                   ('stand', POSES['idle'], 1.65)):
        apply(spec)
        place((*spot, 0.0), math.pi)
        lamp = ghost_light(where)
        head = Vector(world_point(HEAD_CENTER)) if name == 'stand' else \
            Vector(np.array(RIG.matrix_world @ RIG.pose.bones['head'].matrix @ RIG.data.bones['head'].matrix_local.inverted()
                            @ Vector(HEAD_CENTER)))
        flat = Vector((0.0, 1.0, 0.0))
        eye = head + flat * math.sqrt(max(1.2 ** 2 - (eye_height - head.z) ** 2, 0.25))
        eye.z = eye_height
        path = os.path.join(WORK, f'face_{name}.png')
        lens_shot(where, path, eye, head + Vector((0.0, 0.0, -0.02)), 50.0, (900, 1000), samples=64)
        paths.append(path)
        bpy.data.objects.remove(lamp)
    concepts().uc.compose(paths, os.path.join(OUT, 'Abel_face.png'))
    strike(where)


def shot_kneel():
    """At zero: knelt on the boards, the lantern set down by his knee, the pump across his lap, the coal open."""
    where = stage_collection('Kneel')
    deck_stage('dusk', where)
    spot = (1.4, -9.6)
    apply(POSES['kneel'])
    place((*spot, 0.0), math.pi + math.radians(14.0))
    ghost_light(where)
    target = Vector((spot[0], spot[1], 0.55))
    # From a standing player's eyes (1.65 m), three and a half meters off, looking down at him; and from his side.
    paths = [os.path.join(WORK, 'kneel_player.png'), os.path.join(WORK, 'kneel_side.png')]
    lens_shot(where, paths[0], target + Vector((-1.2, 3.5, 1.1)), target, 35.0, (1100, 1000), samples=64)
    side = Matrix.Rotation(math.pi + math.radians(14.0), 3, 'Z') @ Vector((-1.0, 0.0, 0.0))
    lens_shot(where, paths[1], target + side * 3.6 + Vector((0.0, 0.0, 0.5)), target, 35.0, (1100, 1000), samples=64)
    concepts().uc.compose(paths, os.path.join(OUT, 'Abel_kneel.png'))
    strike(where)


def posed_point(bone, p):
    """A rest-pose point p (rig space) where the posed bone carries it, in the world."""
    return (RIG.matrix_world @ RIG.pose.bones[bone].matrix @ RIG.data.bones[bone].matrix_local.inverted()
            @ Vector(tuple(p)))


def shot_kneel_close():
    """The line "...El? You came home.", as the player sees it: knelt on the boards at dusk, seen three-quarter from his
    left (the lantern's side) from a standing player's eye height, 1.62 m, about 2 m off, looking down at him."""
    where = stage_collection('KneelClose')
    deck_stage('dusk', where)
    spot = (1.4, -9.6)
    turn = math.pi + math.radians(14.0)
    apply(POSES['kneel'])
    place((*spot, 0.0), turn)
    ghost_light(where)
    head = posed_point('head', HEAD_CENTER)
    eye = Vector((head.x, head.y, 0.0)) + Matrix.Rotation(turn + math.radians(35.0), 3, 'Z') @ Vector((0.0, -2.0, 0.0))
    eye.z = 1.62
    lens_shot(where, os.path.join(OUT, 'Abel_kneel_close.png'), eye, head + Vector((0.0, 0.0, -0.26)), 35.0,
              (1600, 1000), samples=64)
    strike(where)


def shot_sit():
    """After the fight: on his own bier facing the sunset (the actor at the bier's SOCKET_Sit), seen from the sunset's
    side and from behind against it."""
    where = stage_collection('Sit')
    ac = deck_stage('dusk', where)
    deck, sockets = ac.deck_models()['BurialDeck']
    bier, bier_sockets = ac.deck_models()['Bier']
    loc, rz = sockets['Bier_2']
    sit_loc, _ = bier_sockets['Sit']
    seat = Vector(loc) + Vector((0.0, 0.0, -0.4)) + Matrix.Rotation(rz, 3, 'Z') @ Vector(sit_loc)
    apply(POSES['sit'])
    place(tuple(seat), rz)
    ghost_light(where)
    target = seat + Vector((0.0, 0.0, 0.75))
    front = Matrix.Rotation(rz, 3, 'Z') @ Vector((0.0, -1.0, 0.0))
    side = Matrix.Rotation(rz, 3, 'Z') @ Vector((1.0, 0.0, 0.0))
    paths = []
    for name, d, lens in (('front', front * 3.4 + side * 1.6 + Vector((0.0, 0.0, 0.4)), 35.0),
                          ('back', -front * 3.6 + side * 2.0 + Vector((0.0, 0.0, 0.6)), 35.0)):
        path = os.path.join(WORK, f'sit_{name}.png')
        lens_shot(where, path, target + d, target, lens, (1100, 1000), samples=64)
        paths.append(path)
    ac.uc.compose(paths, os.path.join(OUT, 'Abel_sit.png'))
    strike(where)


def shot_poses():
    """The fight's poses in the studio light: the idle, the lantern flare, the shot, the stock lunge, the turn to the
    sunset (the coal open)."""
    paths = []
    for name in ('idle', 'flare', 'fire', 'lunge', 'sunset'):
        where = stage_collection('Pose')
        studio_stage(where)
        apply(POSES[name])
        place((0.0, 0.0, 0.0), math.radians(-28.0))
        ghost_light(where, energy=12.0)
        c = Vector((0.0, -0.2, 1.15 * SCALE))
        path = os.path.join(WORK, f'pose_{name}.png')
        lens_shot(where, path, c + Vector((0.15, -1.0, 0.08)).normalized() * 6.5, c, 50.0, (700, 1000), samples=32)
        paths.append(path)
        strike(where)
    concepts().uc.compose(paths, os.path.join(OUT, 'Abel_poses.png'))


UNPAID_ZONES = ('#BAC4C6', '#93A5B2', '#6C5D50', '#A65E4C')   # SK_Unpaid's Ghost_A, as approved in the game


def unpaid_stand_in(where, location, turn):
    """An Unpaid to stand beside him: the approved concept's figure (option D), drawn as the game draws SK_Unpaid, with
    its Ghost_A colors, the same M_Ghost values as Abel's and a Basic coal, so their faces compare in the same light."""
    ac = concepts()
    body = preview_material('Unpaid', dict(GHOST_LOOK), UNPAID_ZONES, rank=0xb02a18, rank_alpha=1.8)
    coal = preview_material('UnpaidCoal', dict(EmberStrength=2.1), UNPAID_ZONES, slot_coal=True, rank=0xb02a18,
                            rank_alpha=1.8)
    for part, options in ac.uc.figure('D', 'idle').parts:
        obj = ac.uc.build(part, coal if options.get('material') == 'coal' else body, where, location=location,
                          turn=turn, **{k: v for k, v in options.items() if k != 'material'})
        lt.box_uv(obj, 'Polymer')


def shot_turnaround():
    """Front, side and back beside a 1.8 m post and an Unpaid (the concept's), orthographic, at the actor's size."""
    ac = concepts()
    where = stage_collection('Turn')
    apply(POSES['idle'])
    xs = (-0.9, 1.15, 2.95, 4.75)
    ac.uc.post(where, (-2.3, 0.0, 0.0))
    ac.uc.label('1.8 m', (-2.2, -0.05, 1.78), 0.075, align='LEFT', where=where)
    unpaid_stand_in(where, (xs[0], 0.0, 0.0), math.radians(-25.0))
    for x, turn, name in zip(xs[1:], (0.0, math.pi / 2.0, math.pi), ('front', 'side', 'back')):
        place((x, 0.0, 0.0), turn)
        snapshot(where)
        ac.uc.label(name, (x, -0.8, 0.05), 0.1, where=where)
    hide_figure(True)
    ac.uc.label('an Unpaid', (xs[0], -0.8, 0.05), 0.1, where=where)
    ac.uc.studio(where)
    ac.uc.sun((-0.4, -0.85, 0.45), strength=3.8, where=where)
    cam = keep(ac.uc.camera((1.2, -30.0, 1.45 + 30.0 * math.tan(math.radians(7.0))), (1.2, 0.0, 1.45), ortho=8.4), where)
    render(os.path.join(OUT, 'Abel_turnaround.png'), (2400, 1000), 48)
    bpy.data.objects.remove(cam)
    hide_figure(False)
    strike(where)


def shot_hits():
    """The hit zones over the mesh: a hull per bone, the lantern's and the coal's sphere; front and side at rest, and
    the front in the lantern flare."""
    ac = concepts()
    hulls = [o for o in RIG.children if o.name.startswith(('UCX_', 'USP_'))]
    rest = {o.name: o.matrix_basis.copy() for o in hulls}      # in the rig's space (they're its children)
    matte = ac.uc.flat_material('HitMatte', 0x8e8a84, 0.9)
    saved = {obj: list(obj.data.materials) for obj in (BODY, HAT, LANTERN, PUMP)}
    for obj in saved:
        for k in range(len(obj.data.materials)):
            obj.data.materials[k] = matte
    HAT.hide_render = True
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
                                  ('flare', POSES['flare'], (0.0, -1.0, 0.08))):
        where = stage_collection('Hits')
        studio_stage(where)
        apply(spec)
        place((0.0, 0.0, 0.0), 0.0, scale=1.0)
        # The hulls ride their bones (in Unreal each is a body on its bone); here they're the rig's children, so they
        # are moved by hand.
        moves = bone_transforms()
        for o in hulls:
            o.matrix_basis = Matrix(moves[o['Bone']].tolist()) @ rest[o.name]
        target = Vector((0.0, -0.05, 1.05))
        cam = keep(ac.uc.camera(target + Vector(direction).normalized() * 20.0, target, ortho=2.3), where)
        path = os.path.join(WORK, f'hits_{name}.png')
        render(path, (800, 1000), 32)
        bpy.data.objects.remove(cam)
        paths.append(path)
        strike(where)
    ac.uc.compose(paths, os.path.join(OUT, 'Abel_hitzones.png'))
    for o in hulls:
        o.hide_render = True
        o.matrix_basis = rest[o.name]
    for obj, mats in saved.items():
        for k, m in enumerate(mats):
            obj.data.materials[k] = m
    HAT.hide_render = False


def shot_lods():
    """The LODs the importer will make (50% and 25%), reduced here the same way, with their counts."""
    ac = concepts()
    where = stage_collection('LODs')
    studio_stage(where)
    apply(POSES['idle'])
    place((0.0, 0.0, 0.0), 0.0, scale=1.0)
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
    keep(ac.uc.camera((0.0, -20.0, 1.0 + 20.0 * math.tan(math.radians(8.0))), (0.0, 0.0, 1.0), ortho=4.2), where)
    render(os.path.join(OUT, 'Abel_lods.png'), (1800, 1100), 48)
    strike(where)
    hide_figure(False)


SHOTS = {'compare': shot_dusk, 'dusk': shot_dusk, 'face': shot_face, 'kneel': shot_kneel, 'close': shot_kneel_close,
         'sit': shot_sit,
         'poses': shot_poses, 'turn': shot_turnaround, 'hits': shot_hits, 'lods': shot_lods}


def previews():
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(WORK, exist_ok=True)
    for o in RIG.children:
        if o.name.startswith(('UCX_', 'USP_')):
            o.hide_render = True
    preview_look()
    if 'debug' in ARGV:
        shot_debug()
        return
    wanted = [a for a in ARGV if a in SHOTS] or ['dusk', 'face', 'kneel', 'close', 'sit', 'poses', 'turn', 'hits', 'lods']
    done = set()
    for name in wanted:
        if SHOTS[name] not in done:
            SHOTS[name]()
            done.add(SHOTS[name])
    apply(POSES['idle'])
    place((0.0, 0.0, 0.0), 0.0, scale=1.0)


if want_preview() or 'debug' in ARGV:
    previews()
