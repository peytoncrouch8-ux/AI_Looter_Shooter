"""Building blocks for the ghosts on the Unpaid's rig (Art/Models/Creatures/Unpaid.py's bones): the Providence's dead
miners (DeadMiner.py), the Foreman (Foreman.py), Hiram Coyle (Hiram.py) and Thaddeus Bright (Thaddeus.py). The toolkit
Unpaid.py, Amos.py and Abel.py each carry, gathered here so the four share one copy:

  - numpy mesh pieces (tubes, lofts, ellipsoids, ribbons, garment shells cut from a body profile), the voxel union and
    the decimation to a triangle budget;
  - Part: geometry with what M_Ghost reads per vertex (vertex color R the tint zone, G cavity, B the coal's ember edge,
    A the fade) and the bones it is skinned with; the skinned mesh built from parts (sRGB byte colors, the Polymer box
    projection, the shroud's own UVs, up to four weights a vertex);
  - static props on a bone (their pivot at the bone's head, modeled where they sit at rest; float colors; sockets);
  - M_Ghost slots (their custom properties are the material instance's parameters) and the shared GhostCoal;
  - hit zones: a convex hull per bone round the faces it carries most and the coal's sphere, checked to hold every face;
  - a living face (Amos.py's, with the whiskers and hair as options) and big working hands;
  - the pose solver and pose tables (Amos.py's): a pose turns bones in the rest pose's axes, each under its parent's
    (AUnpaidCreature's Turned = Above * Own), and can shift a joint; the arms are laid by where the fist holds or where
    the elbow and wrist go; the table is Unreal's component space (cm; x forward, y right, z up) and FQuats;
  - the previews' stand-in for M_Ghost in Eevee (Amos.py's preview_material).

Units are Blender meters; the model's front is -Y, the origin on the ground under him, the pelvis (the float height) at
1 m, as Art/README.md's rigged models are.
"""
import json
import math
import os
from collections import Counter

import bpy
import numpy as np
from mathutils import Matrix, Vector, kdtree, noise

import looter_model as lm
import looter_textures as lt

SIDES = (1.0, -1.0)                     # +1 the left (+X), -1 the right
SUFFIX = {1.0: 'l', -1.0: 'r'}
FINGERS = ('thumb', 'index', 'middle', 'ring', 'pinky')
SKIN, ZONE1, ZONE2, ZONE3 = 0, 1, 2, 3  # tint zones: vertex color R = zone / 3
RIM_COLOR = '#DCECEE'
# The Unpaid's approved look (Unpaid.py's Ghost_A, as tried in the game's daylight): a thin rim that still reads, the
# body's own faint glow, the ember edge, the fade's noise and steepness, the Polymer grain three times a meter.
GHOST_LOOK = dict(RimColor=RIM_COLOR, RimPower=4.0, RimStrength=2.2, GlowStrength=0.12, EmberStrength=2.0,
                  NoiseScale=1.4, FadeSoftness=0.07, UVScale=3.0)
# A prop's look (Abel.py's lantern, Amos.py's fork): the same ghost, a rim that reads on thin things, no ember.
PROP_LOOK = dict(RimColor=RIM_COLOR, RimPower=4.0, RimStrength=1.6, GlowStrength=0.1, EmberStrength=0.0,
                 NoiseScale=1.4, FadeSoftness=0.07, UVScale=3.0)


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


def rot_x(degrees):
    """A turn about Blender's +X: an upright bone leans forward, a hanging one swings back (AUnpaidCreature's PitchBy)."""
    return rotation((1.0, 0.0, 0.0), math.radians(degrees))


def rot_y(degrees):
    return rotation((0.0, 1.0, 0.0), math.radians(degrees))


def rot_z(degrees):
    """A turn about +Z: the front (-Y) swings toward +X, his left, for positive degrees."""
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


def perp(v, axis):
    """v with its part along axis taken out."""
    a = unit(axis)
    v = np.asarray(v, float)
    return v - a * np.dot(v, a)


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


def lean(angle, pivot_z=1.0, width=0.1):
    """A warp leaning everything above the pivot height forward by angle (radians) about the X axis, blended over width,
    as the spine bones would (UnpaidConcepts' lean)."""
    def fn(V):
        V = np.atleast_2d(np.asarray(V, float))
        a = angle * smoothstep(pivot_z - width, pivot_z + width, V[:, 2])
        y, z = V[:, 1], V[:, 2] - pivot_z
        out = V.copy()
        out[:, 1] = y * np.cos(a) - z * np.sin(a)
        out[:, 2] = pivot_z + y * np.sin(a) + z * np.cos(a)
        return out
    return fn


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


def tube(C, rx, ry=None, segs=16, up=None, domes=4, pointed=0.0):
    """A closed tube along C with a radius per point (ry along the binormal), domed ends; up (the first normal's way)
    defaults to a direction square to the tube's first step."""
    C = np.asarray(C, float)
    if up is None:
        t0 = unit(C[min(1, len(C) - 1)] - C[0])
        up = (0.0, 0.0, 1.0) if abs(t0[2]) < 0.9 else (0.0, 1.0, 0.0)
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


def lathe(profile, origin=(0.0, 0.0, 0.0), axis=(0.0, 0.0, 1.0), segs=24, squash=1.0):
    """A closed surface of revolution: profile [(height along axis, radius)], a radius of 0 at either end a pole."""
    o, k = np.asarray(origin, float), unit(axis)
    a = unit(np.cross(k, (0.31, 0.17, 0.93) if abs(k[2]) < 0.9 else (1.0, 0.0, 0.0)))
    b = np.cross(k, a)
    ang = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    rad = np.outer(np.cos(ang), a) + np.outer(np.sin(ang), b) * squash
    rings = [o + k * h + rad * r for h, r in profile if r > 1e-7]
    V = list(np.vstack(rings))
    R = len(rings)
    faces = [grid_faces(R, segs)]
    first, last = profile[0], profile[-1]
    V.append(o + k * first[0] if first[1] <= 1e-7 else rings[0].mean(0))
    faces.append(fan(len(V) - 1, np.arange(segs)))
    V.append(o + k * last[0] if last[1] <= 1e-7 else rings[-1].mean(0))
    faces.append(fan(len(V) - 1, np.arange(segs) + (R - 1) * segs, top=True))
    return orient((np.array(V), faces))


def box_piece(center, half, R=None):
    """A box (half sizes) as a closed piece, turned by R (its columns the box's axes) about its center."""
    c = np.asarray(center, float)
    V = np.array([[sx, sy, sz] for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)], float) * np.asarray(half, float)
    if R is not None:
        V = V @ np.asarray(R).T
    return orient((V + c, [np.array([(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)])]))


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


# --- The body: profiles (rows of z, half width, half depth, y of the middle, superellipse power) and garments on them ---

def surface(profile, z, theta, grow=0.0):
    """Points round a profile at heights z and angles theta (from the front, toward the left): (len z, len theta, 3)."""
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


def shell(profile, z_lo, z_hi, sd_fn, folds_fn, grow=0.0, rows=120, cols=200, hole=0.0, coal_at=(0.25, 1.33)):
    """A garment round the body: the profile grown by grow, folded by folds_fn(theta, z), cut where sd_fn > 0 and, with
    hole, burnt through at the coal (a ragged hole). Returns (V, faces), each vertex's theta and z, and its distance from
    the hole's edge."""
    theta = np.linspace(-np.pi, np.pi, cols, endpoint=False)
    z = np.linspace(z_lo, z_hi, rows)
    th, zz = np.meshgrid(theta, z)
    P = surface(profile, z, theta, grow) + outward(theta)[None, :, :] * folds_fn(th, zz)[..., None]
    sd = sd_fn(th, zz)
    tc, zc = coal_at
    ang = np.arctan2(zz - zc, (th - tc) * 0.17)
    rh = hole * (1.0 + 0.14 * np.sin(ang * 5 + 1.0) + 0.09 * np.sin(ang * 11.0))
    gap = np.hypot((th - tc) * 0.17, zz - zc)
    if hole:
        sd = np.maximum(sd, rh - gap)
    V, F, used = cut(P, sd)
    return (V, F), th.ravel()[used], zz.ravel()[used], (gap - rh).ravel()[used]


def armhole(th, zz, z0=1.372, w=0.22, h=0.05):
    """A garment's signed distance for an armhole either side (positive inside the hole)."""
    at = np.abs(th)
    return (1.0 - np.sqrt(((at - np.pi / 2 - 0.03) / w) ** 2 + ((zz - z0) / h) ** 2)) * 0.06


def scorch(edge, V, width=0.03, heat=1.0):
    """The ember edge round the coal's burnt hole (vertex B)."""
    return np.clip(smoothstep(width, 0.0, edge) * (0.5 + 0.5 * pnoise(V, 50.0, 2)) * heat, 0.0, 1.0)


# --- Parts: geometry with what M_Ghost reads per vertex and its skin weights ---

class Part:
    """A piece of the skin: its tint zone (per vertex or one for all), cavity, ember and fade per vertex, its material
    slot, its own UV makings (the shroud) and its weights per bone."""

    def __init__(self, name, piece, zone, slot=0, cavity=0.0, ember=0.0, fade=1.0, strip_uv=None):
        V, faces = piece
        self.name, self.slot = name, slot
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


def chain(x, stops, bones):
    """Weights over a chain of bones by a coordinate x: bones[k] whole at stops[k], shared linearly between."""
    x = np.clip(np.asarray(x, float), stops[0], stops[-1])
    out = {b: np.zeros_like(x) for b in bones}
    k = np.clip(np.searchsorted(stops, x, side='right') - 1, 0, len(stops) - 2)
    f = (x - np.asarray(stops)[k]) / (np.asarray(stops)[k + 1] - np.asarray(stops)[k])
    for i, b in enumerate(bones):
        out[b] += np.where(k == i, 1.0 - f, 0.0) + np.where(k == i - 1, f, 0.0)
    return out


def torso_weights(part, z, x=None, shoulders=True, neck=True, scale=1.0, stops=(1.04, 1.2, 1.38, 1.58),
                  shoulder_x=(0.14, 0.2)):
    """The spine's share of torso cloth by height (each bone whole at its middle; z in the upright frame the cloth was
    built in), the neck's only close round the neck, and the shoulders' share to the upper arms."""
    bones = ('pelvis', 'spine_01', 'spine_02', 'neck') if neck else ('pelvis', 'spine_01', 'spine_02')
    w = chain(z, stops if neck else stops[:3], bones)
    if x is not None and neck:
        away = smoothstep(0.06, 0.11, np.abs(x))
        w['spine_02'] = w['spine_02'] + w['neck'] * away
        w['neck'] = w['neck'] * (1.0 - away)
    arm = np.zeros(len(np.atleast_1d(z)))
    if shoulders and x is not None:
        arm = 0.35 * smoothstep(shoulder_x[0], shoulder_x[1], np.abs(x)) * smoothstep(1.36, 1.44, z)
    for bone, value in w.items():
        part.weigh(bone, value * (1.0 - arm) * scale)
    if shoulders and x is not None:
        for s in SIDES:
            part.weigh(f'upperarm_{SUFFIX[s]}', arm * (np.sign(x) == s) * scale)
    return part


# --- Arms and hands ---

def two_bone(S, W, L1, L2, pole):
    """The elbow for a shoulder S and wrist W with these segment lengths, bent toward pole; the wrist pulled in if out of
    reach. Returns (E, W, reached)."""
    S, W = np.asarray(S, float), np.asarray(W, float)
    d = W - S
    dist = float(np.linalg.norm(d))
    x = d / dist
    reach = min(max(dist, abs(L1 - L2) + 1e-4), L1 + L2 - 1e-4)
    W = S + x * reach
    a = (L1 * L1 - L2 * L2 + reach * reach) / (2.0 * reach)
    h = math.sqrt(max(L1 * L1 - a * a, 0.0))
    y = unit(np.asarray(pole, float) - x * np.dot(pole, x))
    return S + x * a + y * h, W, abs(reach - dist) < 1e-3


def grip_of(W, f, u, scale=1.1):
    """Where a fist (wrist W, fingers' way f, back of the hand u) holds a rod: inside the curled fingers."""
    return np.asarray(W, float) + unit(f) * 0.075 * scale - unit(u) * 0.03 * scale


def wrist_for(G, f, u, scale=1.1):
    return np.asarray(G, float) - unit(f) * 0.075 * scale + unit(u) * 0.03 * scale


def rod_hand(rod, fingers, side):
    """A fist round a rod running rod (the thumb's way), the fingers pointing fingers' way across it: (f, back of the
    hand). The fist's tunnel (f x u, by side) runs along the rod."""
    r = unit(rod)
    f = unit(np.asarray(fingers, float) - r * np.dot(fingers, r))
    u = np.cross(f, r) * side
    return f, u


def hand_pieces(W, fwd, up, side, curl, spread, thumb=0.35, length=1.0, thin=1.0, claw=0.0, knuckle=1.0, scale=1.0):
    """A hand from the wrist W (fwd along the fingers, up out of its back; side +1 left): closed pieces for a union,
    and each finger's line (name, points, radius) for its weights (Unpaid.py's hand, as Abel.py and Amos.py build it)."""
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


def weigh_hand(part, V, E, W, lines, sfx, elbow_from=None):
    """A forearm-and-hand part's weights: the forearm on lowerarm (the elbow's end shared with the upper arm when
    elbow_from is given: the arm's shoulder), the hand on hand, each finger past its knuckle on its own bone."""
    dv = unit(W - E)
    u_ = ((V - E) @ dv) / np.linalg.norm(W - E)
    hand_share = smoothstep(0.9, 1.02, u_)
    elbow = 0.5 * smoothstep(0.1, -0.02, u_) if elbow_from is not None else np.zeros(len(V))
    best = np.full(len(V), np.inf)
    owner = np.full(len(V), -1)
    along = np.zeros(len(V))
    for j, (name, pts, radius) in enumerate(lines):
        dd, tt = polyline_distance(V, pts)
        better = (dd < radius + 0.007) & (dd < best)
        best[better], owner[better], along[better] = dd[better], j, tt[better]
    finger_share = np.where(owner >= 0, smoothstep(0.06, 0.2, along), 0.0)
    if elbow_from is not None:
        part.weigh(f'upperarm_{sfx}', elbow * (1.0 - hand_share))
    part.weigh(f'lowerarm_{sfx}', (1.0 - elbow) * (1.0 - hand_share))
    part.weigh(f'hand_{sfx}', hand_share * (1.0 - finger_share))
    for j, (name, pts, radius) in enumerate(lines):
        part.weigh(finger_bone(name, sfx), hand_share * finger_share * (owner == j))
    return part


def sleeve_piece(S, E, W, radii, seed, folds=0.09, end=0.02):
    """A sleeve from the shoulder to the wrist (W None: to just past the elbow), folds winding round it and bunching at
    the elbow. Returns (piece, its line, how far along the line the elbow is)."""
    rng = np.random.default_rng(seed)
    d = unit(E - S)
    pts = [S, (S + E) * 0.5 + np.cross(d, [0.0, 0.0, 1.0]) * 0.006, E]
    if W is not None:
        pts += [(E + W) * 0.5, W - unit(W - E) * end]
    C = spline(pts, 30 if W is not None else 18)
    L = np.cumsum(np.concatenate([[0.0], np.linalg.norm(np.diff(C, axis=0), axis=1)]))
    at_elbow = float(L[int(np.argmin(np.linalg.norm(C - E, axis=1)))] / L[-1])
    starts = rng.uniform(0.0, 2.0 * np.pi, 4)
    twists = rng.uniform(0.8, 1.8, 4) * rng.choice((-1.0, 1.0), 4)

    def rfn(t, a):
        base = np.interp(t, [0.0, at_elbow, 1.0], radii)
        f = sum(folds * np.exp(-(wrap(a - p - tw * t) / 0.35) ** 2) for p, tw in zip(starts, twists))
        bunch = 0.07 * np.exp(-((t - at_elbow) / 0.08) ** 2) * (0.6 + 0.4 * np.cos(3 * a))
        return base * (1.0 + f * smoothstep(0.05, 0.3, t) + bunch)
    return tube_fn(C, rfn, segs=20), C, at_elbow


def weigh_sleeve(part, V, line, at_elbow, sfx, top=0.14):
    """A sleeve's weights: its top shared with the chest, the upper arm, past the elbow the forearm."""
    d, along = polyline_distance(V, line)
    head = smoothstep(top, 0.0, along)
    elbow = smoothstep(at_elbow - 0.08, at_elbow + 0.08, along)
    part.weigh('spine_02', 0.45 * head)
    part.weigh(f'upperarm_{sfx}', (1.0 - 0.45 * head) * (1.0 - elbow))
    part.weigh(f'lowerarm_{sfx}', (1.0 - 0.45 * head) * elbow)
    return part


# --- Materials: M_Ghost slots (their custom properties are the material instance's parameters) ---

def ghost_slot(name, colors, texture_set='Polymer', **scalars):
    """An M_Ghost slot: the importer resets the instance MI_<name> and sets only these. Its Blender color is the first
    zone's, for the viewport."""
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


def coal_slot():
    """GhostCoal, shared with SK_Unpaid, SK_Abel and SK_Amos: exactly their values (the code sets its rank's color)."""
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
    return coal


# --- The skinned mesh ---

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
    """Parts with UVs of their own (the shroud) take them over the box projection: U the meters round it, V minus the
    meters down it (Unreal's V, flipped on import, runs down the strips as M_Ghost drifts its noise up them)."""
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


def skinned(rig, name, parts, slot_materials, soft=('Head', 'Ear', 'Neck', 'Mouth', 'Teeth')):
    """The skin under the rig: one mesh of every part (its slots in this order), sRGB byte colors with exactly the
    values M_Ghost reads, the Polymer box projection and the shroud's own UVs, smooth but for creases sharper than 70
    degrees (never on the face), the weights as vertex groups, an Armature modifier. Returns the object, the arrays
    (V, faces, colors, W, names) and where each part's vertices are."""
    for p in parts:
        if not p.weights:
            raise RuntimeError(f'{p.name} has no weights')
    V, faces, colors, slots, names, W = assemble(parts)
    missing = sorted(set(names) - set(b.name for b in rig.data.bones))
    if missing:
        raise RuntimeError(f'weights on bones the rig has not: {missing}')
    mesh = to_mesh(name, V, faces)
    for mat in slot_materials:
        mesh.materials.append(mat)
    mesh.polygons.foreach_set('material_index', slots.astype(np.int32))
    mesh.validate()
    if len(mesh.vertices) != len(V):
        raise RuntimeError('validate() changed the vertices')
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(70.0))
    soft_v = np.concatenate([np.full(len(p.V), p.name in soft) for p in parts])
    if 'sharp_edge' in mesh.attributes:
        ev = np.empty(len(mesh.edges) * 2, dtype=np.int64)
        mesh.edges.foreach_get('vertices', ev)
        ev = ev.reshape(-1, 2)
        sharp = np.empty(len(mesh.edges), dtype=bool)
        mesh.attributes['sharp_edge'].data.foreach_get('value', sharp)
        sharp &= ~(soft_v[ev[:, 0]] & soft_v[ev[:, 1]])
        mesh.attributes['sharp_edge'].data.foreach_set('value', sharp)
    body = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(body)
    lt.box_uv(body, 'Polymer')
    strip_uvs(mesh, parts)
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    loops = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loops)
    col.data.foreach_set('color_srgb', colors[loops].astype(np.float32).ravel())
    mesh.color_attributes.active_color = col
    mesh.color_attributes.render_color_index = 0
    for b, bone in enumerate(names):
        group = body.vertex_groups.new(name=bone)
        w = W[:, b]
        for value in np.unique(np.round(w[w > 0.0], 3)):
            group.add(np.where(np.abs(np.round(w, 3) - value) < 1e-9)[0].tolist(), float(value), 'REPLACE')
    body.parent = rig
    body.modifiers.new('Armature', 'ARMATURE').object = rig
    layout = {}
    first = 0
    for p in parts:
        layout.setdefault(p.name, []).append((first, first + len(p.V)))
        first += len(p.V)
    rig['LODs'] = '50,25'
    rig['LODScreens'] = '0.3,0.12'
    rig['Nanite'] = 0
    return body, (V, faces, colors, W, names), layout


def triangle_report(log, name, body, parts):
    tris = sum(len(p.vertices) - 2 for p in body.data.polygons)
    rig = body.parent
    log(f'SK_{name}: {tris} triangles, {len(body.data.vertices)} vertices, {len(rig.data.bones)} bones')
    totals = {}
    for p in parts:
        totals[p.name] = totals.get(p.name, 0) + p.triangles()
    log('  ' + ', '.join(f'{k} {v}' for k, v in totals.items()))
    return tris


# --- Static props: modeled where they sit at rest, their pivots at their bones' heads ---

def static_colors(mesh, rgba_per_vertex):
    """A textured static mesh's colors: floats holding the values (exported linear)."""
    col = mesh.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    loops = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loops)
    col.data.foreach_set('color', np.asarray(rgba_per_vertex, np.float32)[loops].ravel())
    mesh.color_attributes.active_color = col
    mesh.color_attributes.render_color_index = 0


def static_prop(name, parts, materials_, pivot, sharp=60.0):
    """A static prop from parts (Part: their zones, cavity, fade, slot), in rig space, its origin moved to pivot and left
    standing there: no Nanite, LODs at 50% and 25%, no collision."""
    Vs, Fs, cols, slots = [], [], [], []
    n = 0
    for p in parts:
        Vs.append(p.V)
        for F in p.F:
            Fs.append(F + n)
            slots.append(np.full(len(F), p.slot))
        cols.append(np.column_stack([p.zones / 3.0, np.clip(p.cavity, 0, 1), np.clip(p.ember, 0, 1),
                                     np.clip(p.fade, 0, 1)]))
        n += len(p.V)
    V = np.concatenate(Vs)
    mesh = to_mesh(name, V - np.asarray(pivot, float), Fs)
    for mat in materials_:
        mesh.materials.append(mat)
    mesh.polygons.foreach_set('material_index', np.concatenate(slots).astype(np.int32))
    mesh.validate()
    if len(mesh.vertices) != len(V):
        raise RuntimeError(f'{name}: validate() changed the vertices')
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.set_sharp_from_angle(angle=math.radians(sharp))
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    lt.box_uv(obj, 'Polymer')
    static_colors(mesh, np.concatenate(cols))
    obj['Nanite'] = 0
    obj['LODs'] = '50,25'
    obj['LODScreens'] = '0.25,0.1'
    obj['Collision'] = 'None'
    obj.location = tuple(np.asarray(pivot, float))
    return obj


def socket(obj, name, point, R=None):
    """SOCKET_<name> on a static prop at point (rig space at rest), its frame R (columns: the socket's x, y, z in rig
    space; its front is its -Y, its top +Z, as a model's)."""
    local = np.asarray(point, float) - np.array(obj.location)
    rot = (0.0, 0.0, 0.0)
    if R is not None:
        e = Matrix(np.asarray(R).tolist()).to_euler('XYZ')
        rot = tuple(math.degrees(a) for a in e)
    return lm.socket(obj, name, tuple(local), rot)


def triangles_of(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


# --- Hit zones (PA_<Name>): a convex hull per bone round the faces it carries most, the coal's sphere ---

class HitZones:
    """The hulls and the coal's sphere, and what they cover: every face but the fading shroud strips' must lie inside
    one (corners, edge middles and middles checked), at rest and as measured in poses."""

    def __init__(self, hull_bones, no_hull, coal_reach, tol=0.004):
        self.hull_bones, self.no_hull, self.coal_reach, self.tol = tuple(hull_bones), tuple(no_hull), coal_reach, tol
        self.planes = {}

    @staticmethod
    def owner_of(bone):
        for finger in FINGERS:
            if bone.startswith(finger + '_'):
                return 'hand_' + bone[-1]
        return bone

    @staticmethod
    def convex_planes(obj):
        M = np.array(obj.matrix_world)
        co = np.array([v.co[:] for v in obj.data.vertices]) @ M[:3, :3].T + M[:3, 3]
        tri = np.array([p.vertices[:3] for p in obj.data.polygons])
        n = np.cross(co[tri[:, 1]] - co[tri[:, 0]], co[tri[:, 2]] - co[tri[:, 0]])
        keep = np.linalg.norm(n, axis=1) > 2e-7
        tri, n = tri[keep], n[keep]
        n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
        d = (n * co[tri[:, 0]]).sum(1)
        flip = n @ co.mean(0) - d > 0.0
        n[flip] *= -1.0
        d[flip] *= -1.0
        return n, d

    @staticmethod
    def face_samples(P, faces, keep, owner):
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

    def outside(self, P, transforms=None):
        inside = np.zeros(len(P), dtype=bool)
        for bone, (n, d) in self.planes.items():
            Q = P
            if transforms is not None:
                T = np.linalg.inv(transforms[bone])
                Q = P @ T[:3, :3].T + T[:3, 3]
            inside |= (Q @ n.T - d <= self.tol).all(1)
        c = self.coal
        if transforms is not None:
            c = transforms['coal'][:3, :3] @ c + transforms['coal'][:3, 3]
        inside |= np.linalg.norm(P - c, axis=1) <= self.coal_reach + self.tol
        return ~inside

    def build(self, rig, V, faces, W, names, log, extra=None):
        """extra: {bone: points} added to a bone's hull (a hand's fingers as the code curls them)."""
        owner = np.array([self.owner_of(names[i]) for i in np.argmax(W, axis=1)])
        for bone in self.hull_bones:
            mine = np.zeros(len(V), dtype=bool)
            for F in faces:
                mine[F[(owner[F] == bone).any(1)].ravel()] = True
            pts = V[mine]
            if extra and bone in extra:
                pts = np.vstack([pts, extra[bone]])
            if len(pts) < 4:
                raise RuntimeError(f'no vertices for the hit zone on {bone}')
            cloud = bpy.data.meshes.new('_cloud')
            cloud.from_pydata(pts.tolist(), [], [])
            obj = bpy.data.objects.new('_cloud', cloud)
            hull = lm.hit_hull(rig, bone, [obj])
            bpy.data.objects.remove(obj)
            bpy.data.meshes.remove(cloud)
            self.planes[bone] = self.convex_planes(hull)
        coal = V[owner == 'coal']
        center = np.array(rig.data.bones['coal'].head_local)
        reach = np.linalg.norm(coal - center, axis=1).max()
        if reach > self.coal_reach:
            raise RuntimeError(f'the coal reaches {reach:.3f} m from its middle, past its hit sphere')
        lm.hit_sphere(rig, 'coal', tuple(center), self.coal_reach)
        self.coal = center
        uncovered = ~np.isin(owner, self.hull_bones + ('coal',))
        others = sorted(set(owner[uncovered].tolist()) - set(self.no_hull))
        if others:
            raise RuntimeError(f'vertices on {others} have no hit zone')
        self.faces, self.owner = faces, owner
        self.keep = [(~uncovered[F]).any(1) for F in faces]
        S, who = self.face_samples(V, faces, self.keep, owner)
        out = self.outside(S)
        if out.any():
            where = ', '.join(f'{b} {n}' for b, n in Counter(who[out].tolist()).most_common(6))
            raise RuntimeError(f'{int(out.sum())} of {len(S)} points on faces with a hit zone lie outside every zone ({where})')
        log(f'hit zones: {len(self.hull_bones)} hulls and the coal sphere ({100 * self.coal_reach:.1f} cm); every face but '
            f'the shroud strips\' lies inside one ({int(uncovered.sum())} vertices left out)')
        return len(self.hull_bones) + 1

    def gaps(self, rig, P):
        """How much of the posed surface (P: the skin's vertices as the rig poses them) the zones, rigid on their bones,
        miss; and where."""
        T = {b.name: np.array(rig.pose.bones[b.name].matrix @ b.matrix_local.inverted()) for b in rig.data.bones}
        S, who = self.face_samples(P, self.faces, self.keep, self.owner)
        out = self.outside(S, T)
        where = ', '.join(f'{b} {n}' for b, n in Counter(who[out].tolist()).most_common(3))
        return 100.0 * out.mean(), where


# --- Poses: what the game's code makes of the rig, solved here bone by bone (Amos.py's) ---

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


def quat_mul(a, b):
    """FQuat's A * B (B first, then A), (X, Y, Z, W)."""
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return np.array([aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
                     aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz])


def turn_between(a, b):
    """The least turn taking direction a to direction b."""
    a, b = unit(a), unit(b)
    axis = np.cross(a, b)
    s = np.linalg.norm(axis)
    if s < 1e-9:
        return np.eye(3)
    return rotation(axis, math.atan2(s, float(np.dot(a, b))))


class Poser:
    """Solves poses on a rig and writes them as a table. arms: the rest arms per side (S, E, W, f along the fingers,
    u the back of the hand, and grip_scale); props: {bone: (object, pivot)} static props riding bones.

    A pose spec: own {bone: 3x3} (the turn under its parent's), total {bone: 3x3} (the whole turn), shift {bone: vector}
    (its joint off where its parent carries it), aim {bone: direction} (its length turned that way), follow {bone:
    other} (carried by another bone than its parent), arms {side: dict(grip, f, u, pole) or dict(E, W, f, u)}, roll
    {bone: degrees about its length}, place {bone: (head, whole turn)} (a prop's bone set down)."""

    def __init__(self, rig, arms, props=None):
        self.rig, self.arms, self.props = rig, arms, props or {}
        self.rest = {b.name: (np.array(b.head_local), np.array(b.matrix_local.to_3x3()), b.parent.name if b.parent else None)
                     for b in rig.data.bones}
        names = [b.name for b in rig.data.bones]
        depth = {}
        for n in names:
            d, p = 0, self.rest[n][2]
            while p:
                d, p = d + 1, self.rest[p][2]
            depth[n] = d
        # Parents before children, and the skirt bones before the shroud that can follow them.
        self.order = sorted(names, key=lambda n: (n.startswith('tail_'), depth[n], names.index(n)))

    def tip(self, name):
        return np.array(self.rig.data.bones[name].tail_local) - self.rest[name][0]

    def arm_turns(self, side, S1, arm):
        """An arm's whole turns: to a grip (f, u: the hand's frame; pole: the elbow's way), to points E and W (with f, u),
        or laid along directions upper and fore as the code lays its arms (the upper arm's least turn from its rest way,
        the forearm's whole turn its own least turn, the hand with it, bent wrist degrees about the arm's side)."""
        A = self.arms[side]
        L1, L2 = np.linalg.norm(A['E'] - A['S']), np.linalg.norm(A['W'] - A['E'])
        if 'upper' in arm:
            R_ua = turn_between(A['E'] - A['S'], arm['upper'])
            R_la = turn_between(A['W'] - A['E'], arm['fore'])
            side_axis = unit(np.cross(R_la @ (A['W'] - A['E']), (0.0, 0.0, 1.0)))
            R_h = rotation(side_axis, -math.radians(arm.get('wrist', 0.0))) @ R_la
            return R_ua, R_la, R_h
        f1, u1 = unit(arm['f']), unit(arm['u'])
        u1 = unit(u1 - f1 * np.dot(u1, f1))
        if 'E' in arm:
            E1 = S1 + unit(np.asarray(arm['E'], float) - S1) * L1
            W1 = E1 + unit(np.asarray(arm['W'], float) - E1) * L2
        else:
            W1 = wrist_for(arm['grip'], f1, u1, A.get('grip_scale', 1.1))
            E1, W1, reached = two_bone(S1, W1, L1, L2, arm['pole'])
        n0 = unit(np.cross(A['E'] - A['S'], A['W'] - A['E']))
        n1 = np.cross(E1 - S1, W1 - E1)
        n1 = unit(n1) if np.linalg.norm(n1) > 1e-6 else n0

        def bone_frame(v, n):
            return frame(v, np.cross(n, unit(v)))
        R_ua = bone_frame(E1 - S1, n1) @ bone_frame(A['E'] - A['S'], n0).T
        R_la = bone_frame(W1 - E1, n1) @ bone_frame(A['W'] - A['E'], n0).T
        R_h = frame(f1, u1) @ frame(A['f'], A['u']).T
        return R_ua, R_la, R_h

    def solve(self, spec):
        """Every bone's whole turn (3x3, Blender axes) and joint (its head) for a pose."""
        own, totals, shift = spec.get('own', {}), dict(spec.get('total', {})), spec.get('shift', {})
        aims, follow = spec.get('aim', {}), spec.get('follow', {})
        T, J = {}, {}
        for name in self.order:
            head, _, parent = self.rest[name]
            if parent is not None:
                above = T[parent]
                joint = J[parent] + above @ (head - self.rest[parent][0])
            else:
                above = np.eye(3)
                joint = head.copy()
            if name in follow:
                other = follow[name]
                joint = J[other] + T[other] @ (head - self.rest[other][0])
                above = T[other]
            joint = joint + np.asarray(shift.get(name, (0.0, 0.0, 0.0)), float)
            side = 1.0 if name.endswith('_l') else -1.0
            if name.startswith('upperarm_') and side in spec.get('arms', {}):
                R_ua, R_la, R_h = self.arm_turns(side, joint, spec['arms'][side])
                totals[name], totals[f'lowerarm_{SUFFIX[side]}'], totals[f'hand_{SUFFIX[side]}'] = R_ua, R_la, R_h
            turned = totals[name] if name in totals else above @ own.get(name, np.eye(3))
            if name in aims:
                turned = turn_between(turned @ self.tip(name), aims[name]) @ turned
            if name in spec.get('roll', {}):
                turned = rotation(unit(turned @ self.tip(name)), math.radians(spec['roll'][name])) @ turned
            if name in spec.get('place', {}):
                joint, turned = (np.asarray(v, float) for v in spec['place'][name])
            T[name], J[name] = turned, joint
        return T, J

    def apply(self, spec):
        """Poses the Blender rig as solve() does, and moves the props with their bones."""
        T, J = self.solve(spec)
        posed = {}
        rig = self.rig
        for bone in rig.data.bones:
            rest = bone.matrix_local
            world = Matrix.Translation(Vector(J[bone.name])) @ (Matrix(T[bone.name].tolist()) @ rest.to_3x3()).to_4x4()
            posed[bone.name] = world
            if bone.parent:
                basis = (posed[bone.parent.name] @ bone.parent.matrix_local.inverted() @ rest).inverted() @ world
            else:
                basis = rest.inverted() @ world
            pb = rig.pose.bones[bone.name]
            pb.rotation_mode = 'QUATERNION'
            pb.matrix_basis = basis
        bpy.context.view_layer.update()
        self.carry()
        return T, J

    def carry(self):
        """The props where their bones are now (the rig's world matrix included)."""
        rig = self.rig
        for name, (obj, pivot) in self.props.items():
            rest = rig.data.bones[name].matrix_local
            obj.matrix_world = rig.matrix_world @ rig.pose.bones[name].matrix @ rest.inverted() @ Matrix.Translation(Vector(pivot))
        bpy.context.view_layer.update()

    def place(self, location=(0.0, 0.0, 0.0), turn=0.0, scale=1.0):
        self.rig.matrix_world = (Matrix.Translation(Vector(location)) @ Matrix.Rotation(math.radians(turn), 4, 'Z')
                                 @ Matrix.Scale(scale, 4))
        bpy.context.view_layer.update()
        self.carry()

    def table(self, poses, path, header, log, verbose=False):
        """Every pose's bones in Unreal's component space: rest head, posed head (cm at size 1), the whole turn from rest
        and the bone's own turn under its parent's, as FQuat (X, Y, Z, W); a bone not listed stays at rest under its
        parent. Written as JSON (Amos_poses.json's layout) and checked."""
        out = {}
        for name, spec in poses.items():
            T, J = self.solve(spec)
            bones = {}
            for bone in self.order:
                parent = self.rest[bone][2]
                turn = T[bone]
                own = (T[parent].T @ turn) if parent else turn
                if name != 'idle' and np.allclose(turn, np.eye(3), atol=1e-6) and np.allclose(own, np.eye(3), atol=1e-6) \
                        and np.allclose(J[bone], self.rest[bone][0], atol=1e-6):
                    continue
                qt, qo = quat_ue(turn), quat_ue(own)
                bones[bone] = dict(rest=(np.round(to_ue(self.rest[bone][0]), 2) + 0.0).tolist(),
                                   head=(np.round(to_ue(J[bone]), 2) + 0.0).tolist(),
                                   turn=(np.round(qt, 5) + 0.0).tolist(), own=(np.round(qo, 5) + 0.0).tolist())
                if verbose and name != 'idle':
                    axis, angle = axis_angle(qt)
                    log(f'POSE {name} {bone}: {tuple(np.round(to_ue(J[bone]), 1))}, turn {angle:.1f} deg about '
                        f'{tuple(np.round(axis, 3))}')
            out[name] = bones
        os.makedirs(os.path.dirname(path), exist_ok=True)
        lines = ['{'] + [f' {json.dumps(k)}: {json.dumps(v)},' for k, v in header.items()] + [' "poses": {']
        for k, (name, bones) in enumerate(out.items()):
            lines.append(f'  {json.dumps(name)}: {{')
            lines.append(',\n'.join(f'   {json.dumps(b)}: {json.dumps(v)}' for b, v in bones.items()))
            lines.append('  }' + (',' if k < len(out) - 1 else ''))
        lines += [' }', '}']
        with open(path, 'w', encoding='utf-8', newline='\n') as fh:
            fh.write('\n'.join(lines) + '\n')
        log(f'pose table: {len(out)} poses ({", ".join(out)}) written to {os.path.relpath(path, lt.REPO)}')
        self.check(path, log)
        return out

    def check(self, path, log):
        """The written table read back: unit quaternions; every bone's whole turn its parent's whole turn times its own;
        every listed bone's rest head where the rig has it."""
        data = json.load(open(path, encoding='utf-8'))
        worst_len, worst_chain, worst_rest, count = 0.0, 0.0, 0.0, 0
        for name, bones in data['poses'].items():
            def whole(bone):
                return np.array(bones[bone]['turn']) if bone in bones else np.array([0.0, 0.0, 0.0, 1.0])
            for bone, row in bones.items():
                for key in ('turn', 'own'):
                    worst_len = max(worst_len, abs(np.linalg.norm(row[key]) - 1.0))
                parent = self.rest[bone][2]
                above = whole(parent) if parent else np.array([0.0, 0.0, 0.0, 1.0])
                got = quat_mul(above, np.array(row['own']))
                want = np.array(row['turn'])
                worst_chain = max(worst_chain, min(np.abs(got - want).max(), np.abs(got + want).max()))
                worst_rest = max(worst_rest, float(np.abs(np.array(row['rest']) - to_ue(self.rest[bone][0])).max()))
                count += 1
        ok = worst_len < 2e-5 and worst_chain < 5e-5 and worst_rest < 0.006
        log(f'pose table check: {count} rows; quaternion lengths off 1 by at most {worst_len:.1e}; whole = parent x own to '
            f'{worst_chain:.1e}; rest heads to {worst_rest:.3f} cm' + ('' if ok else '  <-- FAILED'))
        if not ok:
            raise RuntimeError('the pose table does not check out')


def posed_vertices(body):
    ev = body.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m = ev.to_mesh()
    P = np.empty(len(m.vertices) * 3)
    m.vertices.foreach_get('co', P)
    ev.to_mesh_clear()
    return P.reshape(-1, 3)


# --- The previews: M_Ghost in Eevee (Amos.py's preview_material) ---

def preview_material(name, look, colors, slot_coal=False, rank=0xb02a18, rank_alpha=1.8, heat=0.0, grain='Polymer'):
    """M_Ghost as Tools/Unreal/build_creature_materials.py builds it, for Eevee: the zone color by vertex R, the grain
    on UV 0 x UVScale, cavity G; emissive the rim (1 - facing) ^ RimPower, the glow and the ember edge in the rank's
    color times Burn (1 + Heat); the fade's two noises on UV 0 x NoiseScale against vertex A, dithered. Coal: the crust
    and its cracks in the rank's color, times Burn."""
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
    layer.inputs['Blend'].default_value = 0.5
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


def ghost_colors(obj, raw=None):
    """A float copy of an object's vertex colors as 'Ghost' (what the preview material reads)."""
    if 'Ghost' in obj.data.color_attributes:
        return
    attr = obj.data.color_attributes.new('Ghost', 'FLOAT_COLOR', 'POINT')
    if raw is None:
        src = obj.data.color_attributes['Col']
        loops = np.empty(len(obj.data.loops), dtype=np.int64)
        obj.data.loops.foreach_get('vertex_index', loops)
        per_loop = np.empty(len(obj.data.loops) * 4, np.float32)
        src.data.foreach_get('color', per_loop)
        raw = np.zeros((len(obj.data.vertices), 4), np.float32)
        raw[loops] = per_loop.reshape(-1, 4)
    attr.data.foreach_set('color', np.asarray(raw, np.float32).ravel())


def slot_look(mat):
    """A slot's look (its scalars) and zone colors, as its custom properties hold them."""
    look = {k: mat[k] for k in ('RimColor', 'RimPower', 'RimStrength', 'GlowStrength', 'EmberStrength', 'NoiseScale',
                                'FadeSoftness', 'UVScale') if k in mat.keys()}
    for k, v in GHOST_LOOK.items():
        look.setdefault(k, v)
    colors = tuple(mat.get(f'Zone{k + 1}Color', '#BAC4C6') for k in range(4))
    return look, colors, mat.get('TextureSet', 'Polymer')
