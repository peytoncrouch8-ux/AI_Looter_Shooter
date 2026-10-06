"""Three concepts for the Unpaid, the restless dead of Ransom's Rest (Docs/Story.md; Docs/Areas/RansomsRest.md,
"Enemies by rank" and build step 16), for the user to choose from (2026-10-06). Concept art kept in Art/Backlog (see its
README): nothing in the game uses it. The pick becomes Art/Models/Creatures/Unpaid.py, a small rig posed by code.

  A  The clothes they died in: a homesteader in his slouch hat, vest, bandana and rolled shirt sleeves, human and sad,
     his shirt giving way below the waist to a pale shroud that tears into strips.
  B  The shrouded: a burial sheet hanging from the head and arms like a hood, the face a dark hollow, bound at the
     waist with winding bands; only the hands and the coal show.
  C  The hungry dead: gaunt and stretched, a long neck and long reaching arms, the jaw slack for the shriek, a torn
     shirt hanging off a frame of ribs, trailing into smoky tatters.

All three float with no legs and carry the coal (the crit spot) on the left of the chest. They are drawn the way the
game would draw them: one masked, dithered ghost material (Eevee's Dithered mode is hashed alpha, never blending), a
fresnel rim, the coal emissive. Per vertex the ghost reads what vertex colors would carry in Unreal: R the tint zone
(0 skin and shroud; 1/3, 2/3 and 1 the clothing zones), G cavity darkening, B the ember edge around the coal (in the
rank's color), A the fade into the shroud, broken up by the MacroNoise texture. The cloth grain is the Polymer set.
The three clothing tints are three sets of zone colors (material instances in the game); the coal takes the rank's
color: dull red Basic, blue Restless, purple Gravebound (the rank tag colors of CreatureRankSettings).

Renders go to Saved/ArtPreviews/RansomsRest/Concepts/Unpaid/: per option a hero view (Unpaid_A_hero.png), the
turnaround (_turnaround), the attack pose (_attack), the clothing tints (_tints) and the coal in the rank colors
(_coal); and Unpaid_compare.png, the three side by side beside a 1.8 m figure.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Backlog\\Creatures\\UnpaidConcepts.py -Preview
    ... -ScriptArgs --preview,A,hero       only some: options A B C, shots hero turn attack tints coal compare
    ... -ScriptArgs --preview,C,debug      close-ups for checking the models, into Intermediate/UnpaidConcepts

Without --preview it builds every option and prints the triangle counts of these (dense, unreduced) concept meshes
('bounds' adds each part's extent).
"""
import math
import os
import sys
import time

import bpy
import numpy as np
from mathutils import Vector, noise

import looter_textures as lt
from looter_plants import swatch

OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'Unpaid')
WORK = os.path.join(lt.REPO, 'Intermediate', 'UnpaidConcepts')     # the coal strip's panels and close-up checks
# Arguments after '--'; artrun.ps1 may pass "--preview,A,hero" as one word, so commas split too.
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
OPTIONS = ('A', 'B', 'C', 'D')
SHOTS = ('hero', 'turn', 'attack', 'tints', 'coal', 'compare', 'group')
TITLES = {'A': 'A  The clothes they died in', 'B': 'B  The shrouded', 'C': 'C  The hungry dead',
          'D': 'D  The clothes they died in, hungry'}

# The coal in each rank's color (name, color, emission): dull red for Basic, which is no rarity color, and the rank
# tags' blue and purple (Source/AI_Looter_Shooter/Creatures/CreatureRankSettings.cpp).
RANKS = (('Basic', 0xb02a18, 6.0), ('Restless', 0x7caaff, 3.0), ('Gravebound', 0xcc7cf4, 3.2))
RIM = 0xdcecee          # the fresnel rim: pale and only barely cool, so no rarity color covers the body
SKIN = 0xd2d8d5         # ghost skin and shroud
# Zone colors per option and tint: (skin and shroud, cloth 1, cloth 2, accent). Pale and faded: they are ghosts.
TINTS = {
    'A': ((SKIN, 0xa5b2ba, 0x6c5d50, 0xa65e4c),     # faded chambray shirt, brown wool vest, red bandana
          (SKIN, 0xd6cebd, 0x4d4e51, 0x8f8b7e),     # Sunday: cream shirt, charcoal vest, grey
          (SKIN, 0xb8946f, 0x7b7f6e, 0xcfc095)),    # harvest: rust shirt, drab vest, straw
    'B': ((SKIN, 0xd9d2c1, 0xa49884, 0x4b4541),     # unbleached muslin, stained winding bands, dark cords
          (SKIN, 0xadb5b8, 0x8c8f88, 0x5b4b40),     # grey calico
          (SKIN, 0x8e8478, 0xcabd9f, 0x3c3a3a)),    # mourning-dyed sheet, pale bands
    'C': ((0xc4c9c2, 0xb3a68c, 0x5f5349, 0x2f2b2a),  # grey skin; old linen shirt, suspenders, string tie
          (0xc4c9c2, 0x8c9ba6, 0x705b49, 0x4c3d34),
          (0xc4c9c2, 0xa27d62, 0x4e5249, 0x6b3a30)),
}
TINTS['D'] = TINTS['A']     # the same clothes


def log(message):
    print(f'UNPAID: {message}', flush=True)


# --- Math ---

def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=float) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def unit(v):
    v = np.asarray(v, dtype=float)
    return v / np.linalg.norm(v, axis=-1, keepdims=True)


def rotation(axis, angle):
    """A 3x3 rotation (numpy, applied as R @ v) about axis by angle in radians."""
    x, y, z = unit(axis)
    c, s = math.cos(angle), math.sin(angle)
    C = 1.0 - c
    return np.array([[c + x * x * C, x * y * C - z * s, x * z * C + y * s],
                     [y * x * C + z * s, c + y * y * C, y * z * C - x * s],
                     [z * x * C - y * s, z * y * C + x * s, c + z * z * C]])


def pnoise(points, scale=1.0, seed=0):
    """Perlin noise (about -1..1) at points (n, 3). mathutils' noise() is fixed, so this repeats exactly."""
    off = Vector((seed * 13.17 + 3.1, seed * 7.31 + 1.7, seed * 5.13 + 4.3))
    return np.array([noise.noise(Vector(p) * scale + off) for p in np.asarray(points, float).reshape(-1, 3)])


def wrap(a):
    return (np.asarray(a, float) + np.pi) % (2.0 * np.pi) - np.pi


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
    """Tangents, normals and binormals along a polyline: rotation-minimizing, the normal starting toward up."""
    T = unit(np.gradient(C, axis=0))
    N = np.zeros_like(C)
    n = np.asarray(up, float) - T[0] * np.dot(up, T[0])
    N[0] = n / np.linalg.norm(n)
    for i in range(1, len(C)):
        n = N[i - 1] - T[i] * np.dot(N[i - 1], T[i])
        N[i] = n / np.linalg.norm(n)
    return T, N, np.cross(T, N)


def table(rows, x):
    """A profile table [(x, a, b, ...)] read at x by linear interpolation: one array per column after the first."""
    rows = np.asarray(rows, float)
    return [np.interp(x, rows[:, 0], rows[:, k]) for k in range(1, rows.shape[1])]


# --- Geometry: closed pieces (for union) and open cloth ---

def grid_faces(rows, cols, closed=True):
    """Quads over a rows x cols grid of vertices (row-major), wrapping around the columns when closed."""
    i = np.arange(rows - 1)[:, None]
    j = np.arange(cols if closed else cols - 1)[None, :]
    j1 = (j + 1) % cols
    return np.stack(np.broadcast_arrays(i * cols + j, i * cols + j1, (i + 1) * cols + j1, (i + 1) * cols + j),
                    axis=-1).reshape(-1, 4)


def fan(center, ring, top=False):
    """Triangles from a center vertex to a ring that runs counterclockwise seen from the top side."""
    ring = np.asarray(ring)
    tris = np.stack([np.full(len(ring), center), np.roll(ring, -1), ring], axis=1)
    return tris[:, ::-1] if top else tris


def ellipsoid(center, radii, axes=None, segs=32, rings=18):
    """A closed ellipsoid, radii along the rows of axes (a right-handed frame; default x, y, z)."""
    axes = np.eye(3) if axes is None else np.asarray(axes, float)
    th = np.linspace(0.0, np.pi, rings + 1)[1:-1]
    ph = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    z = np.repeat(-np.cos(th), segs)
    r = np.repeat(np.sin(th), segs)
    x, y = r * np.tile(np.cos(ph), len(th)), r * np.tile(np.sin(ph), len(th))
    unit_pts = np.vstack([np.stack([x, y, z], 1), [[0.0, 0.0, -1.0], [0.0, 0.0, 1.0]]])
    V = np.asarray(center, float) + (unit_pts * np.asarray(radii, float)) @ axes
    n = (rings - 1) * segs
    return V, [grid_faces(rings - 1, segs), fan(n, np.arange(segs)), fan(n + 1, np.arange(segs) + n - segs, top=True)]


def tube(C, rx, ry=None, segs=16, up=(0.0, 0.0, 1.0), domes=4, pointed=0.0):
    """A closed tube along the points C with a radius per point (ry: a second radius, along the binormal), its ends
    domed (pointed stretches the end dome into a cone: claws)."""
    C = np.asarray(C, float)
    T, N, B = frames(C, up)
    rx = np.broadcast_to(np.asarray(rx, float), (len(C),)).copy()
    ry = rx.copy() if ry is None else np.broadcast_to(np.asarray(ry, float), (len(C),)).copy()
    a = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    rings, centers = [], []

    def ring(c, n, b, r1, r2):
        return c + np.outer(np.cos(a), n) * r1 + np.outer(np.sin(a), b) * r2

    for k in range(domes, 0, -1):                        # the start dome, from near its pole to the tube
        phi = 0.5 * np.pi * k / (domes + 1)
        rings.append(ring(C[0] - T[0] * rx[0] * math.sin(phi), N[0], B[0], rx[0] * math.cos(phi), ry[0] * math.cos(phi)))
    for i in range(len(C)):
        rings.append(ring(C[i], N[i], B[i], rx[i], ry[i]))
    tip = rx[-1] * (1.0 + pointed * 3.0)
    for k in range(1, domes + 1):                        # the end dome
        phi = 0.5 * np.pi * k / (domes + 1)
        rings.append(ring(C[-1] + T[-1] * tip * math.sin(phi), N[-1], B[-1], rx[-1] * math.cos(phi) ** (1 + pointed),
                          ry[-1] * math.cos(phi) ** (1 + pointed)))
    R = len(rings)
    V = np.vstack(rings + [C[0] - T[0] * rx[0], C[-1] + T[-1] * tip])
    return V, [grid_faces(R, segs), fan(R * segs, np.arange(segs)), fan(R * segs + 1, np.arange(segs) + (R - 1) * segs, top=True)]


def torus(center, axis, major, minor, segs=36, sides=12, squash=1.0, minor_out=None):
    """A closed ring (a roll of cloth): major radius around axis, a tube of minor radius (squashed along the axis)."""
    axis = unit(axis)
    a = unit(np.cross(axis, [0.31, 0.17, 0.93] if abs(axis[2]) < 0.9 else [1.0, 0.0, 0.0]))
    b = np.cross(axis, a)
    u = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    v = np.linspace(0.0, 2.0 * np.pi, sides, endpoint=False)
    radial = np.outer(np.cos(u), a) + np.outer(np.sin(u), b)
    out = minor if minor_out is None else minor_out
    V = (np.asarray(center, float) + radial[:, None, :] * major
         + radial[:, None, :] * (np.cos(v) * np.where(np.cos(v) > 0, out, minor))[None, :, None]
         + axis[None, None, :] * (np.sin(v) * minor * squash)[None, :, None]).reshape(-1, 3)
    i, j = np.meshgrid(np.arange(segs), np.arange(sides), indexing='ij')
    i1, j1 = (i + 1) % segs, (j + 1) % sides
    F = np.stack([i * sides + j, i1 * sides + j, i1 * sides + j1, i * sides + j1], -1).reshape(-1, 4)
    return V, [F]


def closed_loft(P):
    """A closed solid from rings P (R, C, 3), bottom to top, each counterclockwise seen from above; capped."""
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
    """A piece (V, faces) rotated by R about pivot, then moved by t."""
    V, faces = piece
    V = np.asarray(V, float)
    if R is not None:
        V = (V - pivot) @ np.asarray(R).T + pivot
    return V + np.asarray(t, float), faces


def to_mesh(name, V, faces):
    mesh = bpy.data.meshes.new(name)
    flat = []
    for F in faces:
        flat += np.asarray(F).tolist()
    mesh.from_pydata(np.asarray(V, float).tolist(), [], flat)
    return mesh


def arrays(mesh):
    """A Blender mesh as (vertices, [face arrays grouped by corner count])."""
    co = np.empty(len(mesh.vertices) * 3)
    mesh.vertices.foreach_get('co', co)
    tot = np.empty(len(mesh.polygons), dtype=np.int64)
    mesh.polygons.foreach_get('loop_total', tot)
    idx = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', idx)
    starts = np.concatenate([[0], np.cumsum(tot)[:-1]]).astype(np.int64)
    return co.reshape(-1, 3), [idx[starts[tot == k][:, None] + np.arange(k)[None, :]] for k in np.unique(tot)]


def union(pieces, voxel, smooth=4, factor=0.5):
    """Closed pieces fused into one smooth surface: a voxel remesh (a union, like a sculptor's remesh) and a few
    smoothing passes that round the creases where the pieces met."""
    V, F = merge(pieces)
    mesh = to_mesh('_union', V, F)
    obj = bpy.data.objects.new('_union', mesh)
    bpy.context.scene.collection.objects.link(obj)
    remesh = obj.modifiers.new('Remesh', 'REMESH')
    remesh.mode = 'VOXEL'
    remesh.voxel_size = voxel
    remesh.adaptivity = 0.0
    if smooth:
        sm = obj.modifiers.new('Smooth', 'SMOOTH')
        sm.factor = factor
        sm.iterations = smooth
    depsgraph = bpy.context.evaluated_depsgraph_get()
    result = bpy.data.meshes.new_from_object(obj.evaluated_get(depsgraph))
    out = arrays(result)
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    bpy.data.meshes.remove(result)
    return out


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
    """1 at center, 0 from radius out (a scalar, or radii along the rows of axes), smooth between."""
    d = np.asarray(V, float) - np.asarray(center, float)
    if axes is not None:
        d = d @ np.asarray(axes, float).T
    return smoothstep(1.0, 0.0, np.linalg.norm(d / np.asarray(radius, float), axis=1))


def push(piece, center, radius, amount, axes=None):
    """A sculpt stroke: vertices move along their normals by amount (negative: in) with a smooth falloff."""
    V, faces = piece
    w = falloff(V, center, radius, axes)
    return V + normals(V, faces) * (amount * w)[:, None], faces


def cut(P, sd, closed=True):
    """The grid P (R, C, 3) cut to where sd (R, C) <= 0, the edge kept clean: faces with two or more corners inside
    stay, and their outside corners slide along the grid to where sd crosses zero. Returns (V, faces, used): used
    indexes the flattened grid, to carry per-vertex values over."""
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


# --- Parts and figures ---

class Part:
    """The geometry of one render object and what the ghost material reads per vertex: fade (1 solid, 0 gone),
    cavity (darkening) and ember (the coal's glowing edge). zone is the tint zone (0..3) of the whole part."""

    def __init__(self, name, zone=0):
        self.name, self.zone = name, zone
        self.V, self.F, self.attr = [], [], []
        self.n = 0

    def add(self, piece, fade=1.0, cavity=0.0, ember=0.0):
        V, faces = piece
        V = np.asarray(V, float).reshape(-1, 3)
        k = len(V)
        self.V.append(V)
        self.F += [np.asarray(F) + self.n for F in faces]
        self.attr.append(np.stack([np.broadcast_to(np.asarray(a, float), (k,)) for a in (fade, cavity, ember)], 1))
        self.n += k
        return self

    def triangles(self):
        return sum(len(F) * (F.shape[1] - 2) for F in self.F)


class Figure:
    """One concept in one pose: its parts (with how each is drawn) and where its coal sits."""

    def __init__(self, key, pose):
        self.key, self.pose = key, pose
        self.parts = []          # (Part, options: material 'ghost' or 'coal', solidify, subsurf)
        self.coal = None         # (center, radius)

    def add(self, part, **options):
        self.parts.append((part, options))
        return part

    def warp(self, fn):
        for part, _ in self.parts:
            part.V = [fn(V) for V in part.V]
        if self.coal is not None:
            self.coal = (fn(np.asarray([self.coal[0]]))[0], self.coal[1])

    def bounds(self, turn=0.0):
        R = rotation((0, 0, 1), turn)
        pts = np.vstack([V for part, _ in self.parts for V in part.V]) @ R.T
        return pts.min(0), pts.max(0)

    def triangles(self):
        return sum(part.triangles() for part, _ in self.parts)


def lean(angle, pivot_z=1.0, width=0.1):
    """A warp leaning everything above the pivot (the waist) forward by angle about the X axis, blended over width,
    as the spine bones would."""
    def fn(V):
        a = angle * smoothstep(pivot_z - width, pivot_z + width, V[:, 2])
        y, z = V[:, 1], V[:, 2] - pivot_z
        out = V.copy()
        out[:, 1] = y * np.cos(a) - z * np.sin(a)
        out[:, 2] = pivot_z + y * np.sin(a) + z * np.cos(a)
        return out
    return fn


def coal_piece(center, radius, seed=3):
    """The coal: a lumpy ember."""
    V, F = ellipsoid(center, (radius, radius, radius * 0.92), segs=28, rings=16)
    d = V - center
    V = V + d * (0.16 * pnoise(V, 28.0 / radius * 0.01 * 6.0, seed))[:, None]
    return V, F


# --- Shared anatomy: hands, faces, the shroud tail ---

def hand(W, fwd, up, side, scale=1.0, curl=(0.45, 0.5, 0.55, 0.6), thumb=0.35, spread=0.08, length=1.0, thin=1.0,
         claw=0.0, knuckle=1.0):
    """The closed pieces of a hand from the wrist W: palm, knuckles, four fingers and a thumb. fwd runs along the
    fingers, up out of the back of the hand; side is +1 for the left hand, -1 for the right."""
    f = unit(fwd)
    u = unit(np.asarray(up, float) - f * np.dot(up, f))
    t = np.cross(f, u) * side                     # toward the thumb
    frame = np.array([f, np.cross(u, f), u])      # right-handed, for the ellipsoids
    s = scale
    W = np.asarray(W, float)
    pieces = [ellipsoid(W + f * 0.047 * s * length ** 0.3, (0.047 * s * length ** 0.3, 0.041 * s, 0.0145 * s * thin),
                        frame, 24, 14),
              tube([W - f * 0.04 * s, W + f * 0.02 * s], [0.019 * s * thin, 0.016 * s * thin],
                   [0.026 * s * thin, 0.022 * s * thin], segs=16, up=u)]
    offsets = (0.026, 0.0085, -0.0095, -0.026)
    lengths = (0.074, 0.083, 0.078, 0.062)
    arch = (0.003, 0.005, 0.003, -0.001)
    fans = (1.4, 0.45, -0.5, -1.5)
    for k in range(4):
        base = W + f * 0.088 * s * length ** 0.3 + t * offsets[k] * s + u * arch[k] * s
        a = spread * fans[k]
        d = f * math.cos(a) + t * math.sin(a)
        lu = u.copy()
        pts = [base]
        L = lengths[k] * length * s
        for seg, (share, bendk) in enumerate(zip((0.45, 0.31, 0.24), (0.8, 1.05, 0.75))):
            b = curl[k] * bendk
            d, lu = d * math.cos(b) - lu * math.sin(b), lu * math.cos(b) + d * math.sin(b)
            pts.append(pts[-1] + d * L * share)
        r0 = 0.0089 * s * thin
        radii = [r0, r0 * 0.93, r0 * 0.84, r0 * (0.7 - 0.35 * claw)]
        C = spline(pts, 14)
        rr = np.interp(np.linspace(0, 1, 14), [0, 0.45, 0.76, 1.0], radii)
        pieces.append(tube(C, rr, segs=12, up=lu, domes=3, pointed=claw))
        pieces.append(ellipsoid(base + u * 0.003 * s, (0.0105 * s * knuckle, 0.0098 * s * knuckle, 0.0085 * s * knuckle),
                                frame, 12, 8))
    # The thumb: its metacarpal (the thenar mass) and two joints curling toward the index finger.
    tb = W + f * 0.018 * s + t * 0.022 * s - u * 0.006 * s
    d = unit(f * 0.55 + t * 0.75 - u * 0.35)
    lu = unit(np.cross(d, f) * side + u * 0.5)
    pts = [tb, tb + d * 0.042 * s]
    for share in (0.036, 0.03):
        b = thumb
        towards = unit(f - d * np.dot(f, d))
        d = unit(d * math.cos(b) + towards * math.sin(b) - u * 0.25 * math.sin(b))
        pts.append(pts[-1] + d * share * s * length ** 0.5)
    C = spline(pts, 14)
    rr = np.interp(np.linspace(0, 1, 14), [0, 0.4, 0.75, 1.0],
                   [0.0145 * s * thin, 0.0105 * s * thin, 0.0092 * s * thin, 0.0072 * s * thin * (1 - 0.4 * claw)])
    pieces.append(tube(C, rr, segs=12, up=lu, domes=3, pointed=claw))
    return pieces


def shroud(part, path, size, rows=64, cols=72, folds=0.3, strips=5, split=0.5, ends=(0.8, 1.0), spread=0.1,
           seed=1, front=(0.0, -1.0, 0.0), fade_from=0.3, twist=0.8, harmonics=(5, 7, 9, 12), narrow=0.6,
           wave=0.0, zone_fade=True):
    """A shroud tail into part: a tube along path whose cross-section (size: rows of t, half width, half depth)
    gathers folds as it goes and tears into strips past split. Each strip narrows, fans out, ends raggedly, and fades."""
    rng = np.random.default_rng(seed)
    C = spline(path, 300)
    T, N, B = frames(C, front)
    amps = rng.uniform(0.55, 1.0, len(harmonics))
    phases = rng.uniform(0.0, 2.0 * np.pi, len(harmonics))
    drift = rng.uniform(-1.0, 1.0, len(harmonics)) * twist

    def at(t):
        x = np.clip(np.asarray(t, float) * (len(C) - 1), 0.0, len(C) - 1.0001)
        i = x.astype(int)
        f = (x - i)[..., None]
        return (C[i] * (1 - f) + C[i + 1] * f, N[i] * (1 - f) + N[i + 1] * f, B[i] * (1 - f) + B[i + 1] * f)

    def point(t, theta, out=0.0):
        t = np.asarray(t, float)
        c, n, b = at(t)
        w, d = table(size, t)
        f = sum(a * np.cos(k * theta + p + dr * t * 6.0) for a, k, p, dr in zip(amps, harmonics, phases, drift))
        f = f / amps.sum()
        r = 1.0 + folds * smoothstep(0.04, 0.4, t) * (f - 0.35 * np.abs(f))
        return c + n * (d * r * np.cos(theta) + out * np.cos(theta))[..., None] + b * (w * r * np.sin(theta)
                                                                                       + out * np.sin(theta))[..., None]

    theta = np.linspace(0.0, 2.0 * np.pi, cols, endpoint=False)
    mrows = max(int(rows * split), 3)
    t = np.linspace(0.0, split, mrows)
    tt, th = np.meshgrid(t, theta, indexing='ij')
    P = point(tt, th)
    fade = 1.0 - smoothstep(fade_from, 1.0, tt)
    part.add((P.reshape(-1, 3), [grid_faces(mrows, cols)]), fade=fade.ravel() if zone_fade else 1.0)
    # The strips: sectors of the last ring, each with its own length, a ragged end and a lean away from the rest.
    bounds = np.sort((np.arange(strips) + rng.uniform(-0.25, 0.25, strips)) * cols / strips).astype(int) % cols
    bounds = np.unique(bounds)
    srows = rows - mrows + 6
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
        if wave:
            # The whole strip sways sideways together (per-column sways would fold it over itself).
            c, n, b = at(split + (np.mean(jag) - split) * q[:, 0])
            side = -math.sin(tc) * n + math.cos(tc) * b
            Q = Q + (side * (wave * q * np.sin(q * 7.0 + k * 1.3)))[:, None, :]
        sf = 1.0 - smoothstep(fade_from, jag[None, :], tq)
        part.add((Q.reshape(-1, 3), [grid_faces(srows, len(js), closed=False)]), fade=sf.ravel() if zone_fade else 1.0)
    return part


# --- Option A: the clothes they died in ---

# The torso (z, half width, half depth, y of its middle, superellipse power): a homesteader of about 1.8 m.
TORSO_A = [(0.90, 0.170, 0.112, 0.000, 2.2), (0.98, 0.163, 0.108, 0.000, 2.2), (1.06, 0.152, 0.104, -0.004, 2.2),
           (1.16, 0.158, 0.110, -0.010, 2.3), (1.26, 0.172, 0.120, -0.012, 2.4), (1.34, 0.184, 0.122, -0.008, 2.6),
           (1.40, 0.192, 0.114, -0.002, 2.8), (1.45, 0.190, 0.104, 0.004, 3.2), (1.475, 0.178, 0.096, 0.008, 3.4),
           (1.50, 0.150, 0.088, 0.010, 3.0), (1.52, 0.110, 0.078, 0.010, 2.6), (1.54, 0.075, 0.068, 0.010, 2.2)]


def torso_points(profile, z, theta, grow=0.0):
    """Points on a torso profile at heights z (R,) and angles theta (C,) from the front toward the character's left
    (+X), counterclockwise seen from above: (R, C, 3)."""
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


# Heads: one smooth surface each, a radius for every direction from the head's middle (at eye height), shaped like a
# sculptor's planes: brow, sockets, nose, cheekbones, hollows, mouth, chin. Sizes in meters.
HEAD_A = dict(width=0.146, front=0.092, back=0.104, top=0.118, bottom=0.128, mouth=-0.07, brow=0.008, socket=0.012,
              nose=0.024, cheek=0.005, hollow=0.005, lips=0.004, chin=0.007, nape=0.22, socket_dark=0.92)
HEAD_C = dict(width=0.136, front=0.09, back=0.102, top=0.112, bottom=0.142, mouth=-0.072, brow=0.009, socket=0.017,
              nose=0.011, cheek=0.007, hollow=0.013, lips=0.002, chin=0.005, nape=0.26, jaw_taper=0.2, socket_dark=0.97,
              mouth_depth=0.045, temple=0.009)


def orient(piece):
    """A closed piece with its faces turned outward (by the sign of its volume)."""
    V, faces = piece
    vol = 0.0
    for F in faces:
        for k in range(1, F.shape[1] - 1):
            a, b, c = V[F[:, 0]], V[F[:, k]], V[F[:, k + 1]]
            vol += np.sum(a * np.cross(b, c))
    return (V, faces) if vol > 0 else (V, [F[:, ::-1] for F in faces])


def head_shape(spec, jaw_open, rows=128, cols=128):
    """A head in head space (front -Y, origin at its middle at eye height). The jaw opens by turning the lower face about
    its hinge; the lips stretch into a mouth that is pushed in and dark. Returns ((V, faces), cavity)."""
    g = lambda x, s: np.exp(-(np.asarray(x) / s) ** 2)
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
    r = r * (1.0 - spec.get('nape', 0.22) * smoothstep(-0.03, -0.11, z) * smoothstep(1.9, 2.8, at))
    r = r * (1.0 - spec.get('jaw_taper', 0.0) * smoothstep(zm + 0.01, -spec['bottom'], z) * smoothstep(2.2, 1.2, at)
             * np.abs(np.sin(TH)))
    nose = (spec['nose'] * smoothstep(0.016, -0.038, z) * smoothstep(-0.058, -0.04, z)
            * g(TH, 0.07 + 0.09 * smoothstep(-0.01, -0.045, z)))
    # The right side sits a touch lower and deeper (asym): no face is symmetrical.
    lop = 1.0 + spec.get('asym', 0.0) * (TH < 0)
    r = r + (spec['brow'] * g(TH, 0.62) * g(z - 0.022, 0.011)
             - spec['socket'] * lop * (g(TH - 0.36, 0.16) + g(TH + 0.36, 0.16)) * g(z - 0.002 + 0.004 * (lop - 1.0), 0.016)
             + nose + 0.004 * g(at - 0.17, 0.06) * g(z + 0.043, 0.008)
             + spec['cheek'] * g(at - 0.68, 0.22) * g(z + 0.02, 0.015)
             - spec['hollow'] * g(at - 0.62, 0.25) * g(z + 0.055, 0.022)
             + spec['lips'] * g(TH, 0.42) * g(z - zm, 0.022)
             - 0.0035 * g(TH, 0.28) * g(z - zm, 0.003)
             + spec['chin'] * g(TH, 0.32) * g(z + spec['bottom'] - 0.016, 0.013)
             - spec.get('temple', 0.004) * g(at - 1.05, 0.22) * g(z - 0.04, 0.025))
    # Finer planes (D's head): the brow's shelf over the sockets, eyeballs sunk in them, the cheekbone's arch running
    # back to the ear, the jaw's angle, the folds from nose to mouth, and thin lips.
    nl = 0.2 + 0.22 * np.clip((-0.045 - z) / 0.035, 0.0, 1.0)
    r = r + (spec.get('brow_shelf', 0.0) * (g(TH - 0.36, 0.2) + g(TH + 0.36, 0.2)) * g(z - 0.017, 0.006)
             + spec.get('eyeball', 0.0) * lop * (g(TH - 0.36, 0.085) + g(TH + 0.36, 0.085)) * g(z + 0.002, 0.009)
             + spec.get('zyg', 0.0) * g(z + 0.018, 0.009) * smoothstep(0.55, 0.75, at) * smoothstep(1.55, 1.3, at)
             + spec.get('jaw_angle', 0.0) * g(at - 1.22, 0.2) * g(z + 0.085, 0.02)
             - spec.get('nasolabial', 0.0) * g(at - nl, 0.045) * smoothstep(-0.04, -0.05, z) * smoothstep(-0.088, -0.078, z)
             + spec.get('lips_thin', 0.0) * g(TH, 0.3) * (g(z - zm - 0.007, 0.0035) + g(z - zm + 0.008, 0.0035)))
    P = d * r[..., None]
    cav = np.clip(1.6 * (g(TH - 0.36, 0.12) + g(TH + 0.36, 0.12)) * g(z, 0.012), 0.0, 1.0) * spec.get('socket_dark', 0.9)
    if spec.get('eyeball', 0.0):
        # The sunk eyes themselves catch a little light; the dark is the ring of socket round them.
        ball = np.clip(1.6 * (g(TH - 0.36, 0.06) + g(TH + 0.36, 0.06)) * g(z + 0.002, 0.007), 0.0, 1.0)
        cav = cav * (1.0 - 0.45 * ball)
    cav = np.maximum(cav, 0.5 * g(TH, 0.26) * g(z - zm, 0.0035))
    cav = np.maximum(cav, 0.35 * spec.get('nasolabial', 0.0) / 0.004 * g(at - nl, 0.03)
                     * smoothstep(-0.04, -0.05, z) * smoothstep(-0.088, -0.078, z))
    if jaw_open > 1e-3:
        w = smoothstep(zm + 0.003, zm - 0.012, P[..., 2]) * smoothstep(2.4, 1.6, at)
        hinge = np.array([0.0, 0.006, -0.03])
        ang = jaw_open * w
        y, zz = P[..., 1] - hinge[1], P[..., 2] - hinge[2]
        P = P.copy()
        P[..., 1] = hinge[1] + y * np.cos(ang) - zz * np.sin(ang)
        P[..., 2] = hinge[2] + y * np.sin(ang) + zz * np.cos(ang)
        band = np.clip(4.0 * w * (1.0 - w), 0.0, 1.0) * smoothstep(0.62, 0.26, at)    # the mouth, not the cheeks
        P = P - d * (band * spec.get('mouth_depth', 0.03) * min(jaw_open / 0.4, 1.0))[..., None]
        cav = np.maximum(cav, smoothstep(0.15, 0.6, band) * 0.97)
    rings = P.shape[0]
    V = np.vstack([P.reshape(-1, 3), P[0].mean(0) + (0.0, 0.0, 0.002), P[-1].mean(0) - (0.0, 0.0, 0.002)])
    n = rings * cols
    faces = [grid_faces(rings, cols), fan(n, np.arange(cols)), fan(n + 1, np.arange(cols) + n - cols)]
    piece = orient((V, faces))
    return piece, np.concatenate([cav.ravel(), [0.0, 0.0]])


def head_parts(part, spec, R, pivot, jaw_open, neck, neck_radius=(0.05, 0.047, 0.046), teeth=False):
    """Adds a head to part: the smooth head (head_shape), ears, the neck rising into it from neck (points), a dark
    mouth behind the lips, and teeth if asked. R turns the head; pivot is the top of the neck."""
    center = pivot + R @ np.array([0.0, -0.012, 0.09])
    piece, cav = head_shape(spec, jaw_open)
    part.add(transform(piece, R, center), cavity=cav)
    for s in (1.0, -1.0):
        ear = ellipsoid((s * (spec['width'] / 2.0 - 0.003), 0.014, -0.006), (0.008, 0.017, 0.027), segs=16, rings=10)
        part.add(transform(ear, R, center))
    part.add(tube(list(neck) + [center + R @ np.array([0.0, 0.018, -0.07])], list(neck_radius), segs=20))
    zm = spec['mouth']
    part.add(transform(ellipsoid((0.0, -0.035, zm - 0.012), (0.032, 0.04, 0.026), segs=20, rings=12), R, center),
             cavity=1.0)
    if teeth:
        hinge = np.array([0.0, 0.006, -0.03])
        Rj = rotation((1.0, 0.0, 0.0), jaw_open)
        for k in range(8):
            a = (k - 3.5) * 0.17
            p = np.array([spec['width'] * 0.36 * math.sin(a), -spec['front'] * 0.86 * math.cos(a), zm + 0.004])
            part.add(transform(ellipsoid(p, (0.0042, 0.0036, 0.0078), segs=8, rings=6), R, center), cavity=0.15)
            q = np.array([spec['width'] * 0.33 * math.sin(a), -spec['front'] * 0.8 * math.cos(a), zm - 0.006])
            # Turned as far as the lip they sit behind (head_shape's jaw weight at their height).
            wq = float(smoothstep(zm + 0.003, zm - 0.012, q[2]))
            q = rotation((1.0, 0.0, 0.0), jaw_open * wq) @ (q - hinge) + hinge
            part.add(transform(ellipsoid(q, (0.004, 0.0034, 0.0072), segs=8, rings=6), R, center), cavity=0.15)


def hat_A():
    """The slouch hat in hat space (origin at the brim's middle): (felt pieces, band piece). The brim droops front and
    back; the crown has a crease down its middle."""
    felt = []
    theta = np.linspace(0.0, 2.0 * np.pi, 72, endpoint=False)
    q = np.linspace(0.0, 1.0, 10)[:, None]
    r = 0.084 + 0.108 * q * (1.0 + 0.04 * np.sin(3 * theta + 0.6))
    droop = -0.034 * q ** 1.7 * (0.55 + 0.45 * np.cos(2 * theta)) + 0.012 * q ** 2 * np.sin(theta) ** 2
    droop = droop - 0.01 * q ** 2 * np.cos(theta) + 0.004 * q * np.sin(5 * theta + 1.0)   # lower in front, worn
    P = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.06, droop], -1)
    felt.append(('brim', P.reshape(-1, 3), [grid_faces(10, 72)]))
    zc = np.array([0.0, 0.025, 0.05, 0.07, 0.084, 0.091])
    rc = np.array([0.088, 0.086, 0.082, 0.076, 0.066, 0.05])
    rings = []
    pinch = 1.0 - 0.1 * np.exp(-((np.abs(wrap(theta)) - 0.55) / 0.3) ** 2)     # pinched in at the front
    for z, rr in zip(zc, rc):
        k = 1.0 + (pinch - 1.0) * (z / 0.091)
        rings.append(np.stack([rr * k * np.sin(theta), -rr * k * np.cos(theta) * 1.1, np.full_like(theta, z)], -1))
    # The top: rings closing toward a crease that runs front to back.
    for s, z in ((0.62, 0.088), (0.3, 0.078)):
        x, y = 0.05 * s * np.sin(theta), -0.05 * s * np.cos(theta) * 1.1
        crease = -0.014 * np.exp(-(x / 0.02) ** 2)
        rings.append(np.stack([x, y, z + crease], -1))
    P = np.array(rings)
    R = len(rings)
    V = np.vstack([P.reshape(-1, 3), [[0.0, 0.0, 0.062]]])
    felt.append(('crown', V, [grid_faces(R, 72), fan(R * 72, np.arange(72) + (R - 1) * 72, top=True)]))
    band = np.array([np.stack([0.0905 * np.sin(theta), -0.0905 * np.cos(theta) * 1.1, np.full_like(theta, z)], -1)
                     for z in (0.003, 0.016, 0.03)])
    return felt, (band.reshape(-1, 3), [grid_faces(3, 72)])


def figure_A(pose):
    attack = pose == 'attack'
    fig = Figure('A', pose)
    sides = (1.0, -1.0)
    # The arms: shoulder, elbow, wrist per side (+1 left), and the hands' directions.
    S = {s: np.array([s * 0.172, 0.004, 1.43]) for s in sides}
    if attack:
        E = {s: np.array([s * 0.30, -0.21, 1.37]) for s in sides}
        Wr = {s: np.array([s * 0.40, -0.46, 1.47]) for s in sides}
        hand_dir = {s: unit([s * 0.25, -1.0, 0.12]) for s in sides}
        hand_up = {s: unit([s * 0.1, 0.35, 1.0]) for s in sides}
        curls = (0.12, 0.15, 0.2, 0.25)
    else:
        E = {s: np.array([s * 0.212, -0.03, 1.158]) for s in sides}
        Wr = {s: np.array([s * 0.222, -0.105, 0.938]) for s in sides}
        hand_dir = {s: unit([s * 0.02, -0.45, -1.0]) for s in sides}
        hand_up = {s: unit([s * 1.0, 0.1, 0.0]) for s in sides}
        curls = (0.35, 0.45, 0.55, 0.62)

    # The shirt: torso, collar and sleeves fused, loose and folded below the vest, its tails hanging out.
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    z = np.linspace(0.90, 1.54, 40)
    P = torso_points(TORSO_A, z, theta)
    tails = smoothstep(1.0, 0.9, z)[:, None]
    folds = (np.cos(5 * theta + 0.7) * 0.6 + np.cos(9 * theta + 2.1) * 0.4)[None, :]
    P[..., 0] *= 1.0 + 0.05 * tails * folds
    P[..., 1] *= 1.0 + 0.05 * tails * folds
    P[0, :, 2] += 0.014 * np.sin(3 * theta + 1.0) + 0.008 * np.sin(7 * theta)
    pieces = [closed_loft(P)]
    collar = np.array([np.stack([0.064 * np.sin(theta), 0.01 - 0.06 * np.cos(theta), np.full_like(theta, zz)], -1)
                       for zz in (1.53, 1.56, 1.588)])
    pieces.append(closed_loft(collar))
    for s in sides:
        d = unit(E[s] - S[s])
        mid = (S[s] + E[s]) * 0.5 + np.array([s * 0.006, 0.0, 0.0])
        pieces.append(tube(spline([S[s], mid, E[s] + unit(Wr[s] - E[s]) * 0.03], 14),
                           np.linspace(0.06, 0.052, 14), segs=20))
        pieces.append(torus(E[s] + unit(Wr[s] - E[s]) * 0.035, Wr[s] - E[s], 0.05, 0.016, squash=0.9))
    shirt = union(pieces, 0.0045, smooth=5)
    # The coal sits where the heart should be, in a crater burnt through the shirt and vest.
    tc, zc = 0.3, 1.33
    pc = torso_points(TORSO_A, [zc], np.array([tc]))[0, 0]
    nc = torso_normal(tc)
    shirt = push(shirt, pc, 0.07, -0.02)
    dist = np.linalg.norm(shirt[0] - pc, axis=1)
    jitter = 0.5 + 0.5 * pnoise(shirt[0], 60.0, 5)
    cavity = smoothstep(0.06, 0.035, dist) * 0.9
    ember = smoothstep(0.03, 0.05, dist) * smoothstep(0.085, 0.062, dist) * (0.4 + 0.6 * jitter)
    fig.add(Part('Shirt', 1).add(shirt, cavity=cavity, ember=ember))
    fig.coal = (pc - nc * 0.022, 0.056)

    # The vest: cut from a shell over the torso (V-neck, armholes, two points at the hem) and through at the coal.
    zv = np.linspace(0.925, 1.545, 110)
    tv = np.linspace(0.0, 2.0 * np.pi, 200, endpoint=False)
    P = torso_points(TORSO_A, zv, tv, grow=0.013)
    th, zz = np.meshgrid(wrap(tv), zv)
    hem = 0.98 - 0.05 * np.exp(-((np.abs(th) - 0.2) / 0.13) ** 2)
    sd = np.maximum(hem - zz, np.where(np.abs(th) < 1.2, zz - (1.24 + 0.36 * np.abs(th)), -1.0))
    armhole = np.sqrt(((np.abs(th) - np.pi / 2) / 0.6) ** 2 + ((zz - 1.418) / 0.09) ** 2)
    sd = np.maximum(sd, (1.0 - armhole) * 0.06)
    sd = np.maximum(sd, zz - 1.53)
    ang = np.arctan2(zz - zc, (th - tc) * 0.18)
    rh = 0.072 * (1.0 + 0.12 * np.sin(ang * 5 + 1.0) + 0.08 * np.sin(ang * 11.0))
    hole = np.hypot((th - tc) * 0.18, zz - zc)
    sd = np.maximum(sd, rh - hole)
    V, F, used = cut(P, sd)
    vest_ember = (smoothstep(0.03, 0.0, hole.ravel()[used] - rh.ravel()[used])
                  * (0.5 + 0.5 * pnoise(V, 50.0, 2)))
    vest = Part('Vest', 2).add((V, F), ember=np.clip(vest_ember, 0, 1))
    n = torso_normal(0.03)
    button_axes = np.array([[0.0, 0.0, 1.0], np.cross(n, [0.0, 0.0, 1.0]), n])
    for zb in (1.005, 1.07, 1.135, 1.2):
        p = torso_points(TORSO_A, [zb], np.array([0.03]), grow=0.019)[0, 0]
        vest.add(ellipsoid(p, (0.0078, 0.0078, 0.0045), button_axes, 12, 8), cavity=0.45)
    fig.add(vest, solidify=0.007)

    # The bandana: a rolled knot round the neck and a triangle hanging over the top of the chest.
    band = Part('Bandana', 3)
    band.add(torus((0.0, 0.008, 1.562), (0.0, -0.2, 1.0), 0.064, 0.0135, squash=0.8))
    rows, cols = 14, 25
    zb = np.linspace(1.55, 1.425, rows)
    width = np.linspace(0.95, 0.03, rows)
    flap = np.array([torso_points(TORSO_A, [zr], np.linspace(-w, w, cols), grow=0.024)[0] for zr, w in zip(zb, width)])
    flap[..., 2] += 0.004 * np.sin(np.linspace(0, 3 * np.pi, cols))[None, :] * (1 - width[:, None])
    band.add((flap.reshape(-1, 3), [grid_faces(rows, cols, closed=False)]))
    band.add(ellipsoid((0.0, 0.078, 1.558), (0.022, 0.016, 0.018)))
    fig.add(band, solidify=0.005)

    # Forearms and hands, the skin of the dead: pale.
    for s in sides:
        dirv = unit(Wr[s] - E[s])
        limb = [tube([E[s] + dirv * 0.02, (E[s] + Wr[s]) * 0.5, Wr[s] - dirv * 0.01], [0.036, 0.032, 0.02],
                     [0.04, 0.036, 0.027], segs=20, up=hand_up[s])]
        limb += hand(Wr[s], hand_dir[s], hand_up[s], s, curl=curls, spread=0.16 if attack else 0.07, thumb=0.3)
        fig.add(Part(f'Arm{s:+.0f}', 0).add(union(limb, 0.0026, smooth=3)))

    # The head, bowed (or thrown back to shriek), the neck under it, and the hat.
    pivot = np.array([0.0, 0.004, 1.615])
    if attack:
        R = rotation((1, 0, 0), math.radians(-34))
        jaw_open = math.radians(30)
    else:
        R = rotation((0, 1, 0), math.radians(-6)) @ rotation((1, 0, 0), math.radians(12))
        jaw_open = math.radians(7)
    face = Part('Head', 0)
    head_parts(face, HEAD_A, R, pivot, jaw_open, neck=[(0.0, 0.012, 1.48), (0.0, 0.006, 1.56)])
    fig.add(face)
    felt, band_piece = hat_A()
    Rh = R @ rotation((1, 0, 0), math.radians(-5)) @ rotation((0, 1, 0), math.radians(4))
    at = pivot + R @ np.array([0.0, 0.004, 0.155])
    hat = Part('Hat', 2)
    for _, V, F in felt:
        hat.add((V @ Rh.T + at, F))
    fig.add(hat, solidify=0.007)
    fig.add(Part('HatBand', 3).add((band_piece[0] @ Rh.T + at, band_piece[1])), solidify=0.004)

    # Below the waist: the pale shroud, tearing into strips.
    if attack:
        path = [(0.0, 0.01, 1.04), (0.0, 0.1, 0.88), (0.0, 0.36, 0.72), (0.0, 0.72, 0.6), (0.0, 1.1, 0.53),
                (0.0, 1.45, 0.5)]
    else:
        path = [(0.0, 0.005, 1.04), (0.0, 0.04, 0.84), (0.0, 0.17, 0.62), (0.0, 0.42, 0.45), (0.0, 0.75, 0.33),
                (0.0, 1.1, 0.27)]
    size = [(0.0, 0.15, 0.1), (0.12, 0.158, 0.118), (0.3, 0.13, 0.11), (0.5, 0.095, 0.085), (0.75, 0.06, 0.055),
            (1.0, 0.03, 0.03)]
    fig.add(shroud(Part('Shroud', 0), path, size, strips=7, split=0.16, ends=(0.74, 1.0), spread=0.1, seed=4,
                   fade_from=0.22, folds=0.32, narrow=0.55, wave=0.03), solidify=0.006)
    fig.add(Part('Coal', 0).add(coal_piece(*fig.coal)), material='coal')
    fig.warp(lean(math.radians(30 if attack else 7)))
    return fig


def _sample(arr, tq):
    """Rows of arr (n, k) read at fractions tq (any shape) of its length, linearly."""
    n = len(arr)
    x = np.clip(np.asarray(tq, float) * (n - 1), 0.0, n - 1.0001)
    i = x.astype(int)
    f = (x - i)[..., None]
    return arr[i] * (1.0 - f) + arr[i + 1] * f


def ribbon(part, path, width, n=60, cols=5, out=(0.0, 0.0, 1.0), axis_center=(0.0, 0.0, 0.0), curl=0.25, twist=0.0,
           wave=0.0, ragged=0.0, fade_from=None, seed=0):
    """A strip of cloth along path into part: width a number or rows of (t, width); its face turned toward out (a
    direction, or 'radial': away from a vertical axis through axis_center), bowed across by curl, turned by twist
    (turns), waving sideways by wave, its end torn by ragged (a share of its length), faded past fade_from."""
    rng = np.random.default_rng(seed)
    C = spline(path, n)
    T = unit(np.gradient(C, axis=0))
    if callable(out):
        O = np.asarray(out(C), float)
    elif isinstance(out, str):
        O = C - np.asarray(axis_center, float)
        O[:, 2] = 0.0
    else:
        O = np.broadcast_to(np.asarray(out, float), C.shape).copy()
    O = unit(O - T * np.sum(O * T, axis=1, keepdims=True))
    A = np.cross(T, O)
    t = np.linspace(0.0, 1.0, n)
    if twist:
        a = 2.0 * np.pi * twist * t
        O, A = O * np.cos(a)[:, None] + A * np.sin(a)[:, None], A * np.cos(a)[:, None] - O * np.sin(a)[:, None]
    if isinstance(width, (list, tuple)):
        w = np.interp(t, [r[0] for r in width], [r[1] for r in width])
    else:
        w = np.full(n, float(width))
    if wave:
        C = C + A * (wave * np.sin(t * 8.0 + seed) * t)[:, None]
    across = np.linspace(-1.0, 1.0, cols)
    ends = 1.0 - ragged * rng.uniform(0.0, 1.0, cols) if ragged else np.ones(cols)
    tq = t[:, None] * ends[None, :]
    Cq, Oq, Aq = _sample(C, tq), _sample(O, tq), _sample(A, tq)
    wq = np.interp(tq, t, w)
    P = Cq + Aq * (wq * across[None, :])[..., None] + Oq * (curl * wq * (1.0 - across ** 2)[None, :])[..., None]
    fade = 1.0 if fade_from is None else (1.0 - smoothstep(fade_from, 1.0, tq)).ravel()
    part.add((P.reshape(-1, 3), [grid_faces(n, cols, closed=False)]), fade=fade)
    return part


def periodic(keys, theta):
    """Keyframes [(angle 0..pi, values...)] on the left side, mirrored to the right, read at theta."""
    keys = np.asarray(keys, float)
    a = np.abs(wrap(theta))
    return [np.interp(a, keys[:, 0], keys[:, k]) for k in range(1, keys.shape[1])]


def drape(keys, rows=64, cols=192, folds=0.03, harmonics=(9, 13, 17, 23), seed=3, jag=0.06, y0=0.0):
    """A sheet hanging around a figure: for each angle (0 front, counterclockwise from above), keys give the top of
    the cloth (under the hood), where it rests over the arms, and its hem: [(angle, top r, top z, rest r, rest z,
    hem r, hem z)], mirrored left to right. Each column runs smoothly over its rest point to the hem and gathers
    folds below it; the hem tears unevenly. Returns the grid (rows, cols, 3) and each vertex's fold depth (0..1)."""
    rng = np.random.default_rng(seed)
    theta = np.linspace(0.0, 2.0 * np.pi, cols, endpoint=False)
    tr, tz, sr, sz, hr, hz = periodic(keys, theta)
    s, c = np.sin(theta), -np.cos(theta)
    amps = rng.uniform(0.5, 1.0, len(harmonics))
    phases = rng.uniform(0.0, 2.0 * np.pi, len(harmonics))
    f = sum(a * np.cos(k * theta + p) for a, k, p in zip(amps, harmonics, phases)) / amps.sum()
    f = f - 0.35 * np.abs(f)
    torn = jag * (0.5 + 0.5 * np.sin(theta * 23.0 + 1.0) * np.sin(theta * 7.0)) + jag * 0.6 * rng.uniform(0, 1, cols)
    P = np.zeros((rows, cols, 3))
    depth = np.zeros((rows, cols))
    q = np.linspace(0.0, 1.0, rows)
    for j in range(cols):
        ctrl = [(tr[j] * s[j], y0 + tr[j] * c[j], tz[j]), (sr[j] * s[j], y0 + sr[j] * c[j], sz[j]),
                (hr[j] * s[j], y0 + hr[j] * c[j], hz[j])]
        line = spline(ctrl, 200)
        line = line[: max(int(200 * (1.0 - torn[j])), 40)]
        P[:, j] = _sample(line, q)
        depth[:, j] = smoothstep(sz[j] + 0.02, hz[j], P[:, j, 2])
    # Folds hang down the columns, raised off the cloth along its own normal (so they show on wings as on a skirt).
    N = np.cross(np.gradient(P, axis=1), np.gradient(P, axis=0))
    N = unit(N + 1e-9)
    outward = np.stack([np.broadcast_to(s, (rows, cols)), np.broadcast_to(c, (rows, cols)), np.zeros((rows, cols))], -1)
    N = N * np.sign(np.sum(N * outward, axis=-1, keepdims=True) + 1e-9)
    amp = folds * depth * f[None, :] * (1.0 + 0.4 * np.sin(q[:, None] * 3.0 + np.arange(cols)[None, :] * 0.05))
    return P + N * amp[..., None], depth


def figure_B(pose):
    attack = pose == 'attack'
    fig = Figure('B', pose)
    sides = (1.0, -1.0)
    if attack:
        keys = [(0.0, 0.13, 1.43, 0.17, 1.35, 0.13, 1.0), (0.45, 0.18, 1.45, 0.32, 1.52, 0.24, 0.95),
                (0.8, 0.2, 1.46, 0.5, 1.69, 0.42, 0.86), (1.12, 0.21, 1.47, 0.585, 1.8, 0.5, 0.82),
                (1.45, 0.21, 1.47, 0.43, 1.66, 0.42, 0.86), (1.8, 0.2, 1.46, 0.31, 1.53, 0.25, 0.96),
                (2.3, 0.17, 1.45, 0.19, 1.4, 0.14, 1.0), (np.pi, 0.16, 1.44, 0.165, 1.36, 0.125, 1.0)]
        W = {s: np.array([s * 0.53, -0.26, 1.765]) for s in sides}
        hdir = {s: unit([s * 0.45, -0.75, 0.45]) for s in sides}
        hup = {s: unit([s * 0.2, 0.6, 0.75]) for s in sides}
        curls, spread = (0.3, 0.35, 0.4, 0.45), 0.22
    else:
        keys = [(0.0, 0.135, 1.43, 0.15, 1.3, 0.13, 1.0), (0.45, 0.165, 1.42, 0.22, 1.2, 0.17, 0.97),
                (0.83, 0.19, 1.42, 0.315, 1.11, 0.27, 0.76), (1.2, 0.2, 1.43, 0.29, 1.15, 0.28, 0.72),
                (1.6, 0.2, 1.44, 0.255, 1.2, 0.25, 0.78), (2.0, 0.185, 1.44, 0.2, 1.3, 0.17, 0.97),
                (2.5, 0.17, 1.44, 0.17, 1.32, 0.13, 1.0), (np.pi, 0.16, 1.44, 0.165, 1.32, 0.125, 1.0)]
        W = {s: np.array([s * 0.235, -0.215, 1.08]) for s in sides}
        hdir = {s: unit([s * 0.08, -1.0, -0.35]) for s in sides}
        hup = {s: unit([s * 0.2, -0.15, 1.0]) for s in sides}
        curls, spread = (0.75, 0.85, 0.9, 0.95), 0.1

    # The sheet over the shoulders and arms, torn open over the coal.
    P, depth = drape(keys, folds=0.06 if attack else 0.05, seed=7, harmonics=(7, 11, 16))
    rows, cols = P.shape[:2]
    cc = np.array([0.058, -0.17, 1.315])
    dx, dz = (P[..., 0] - cc[0]) / 0.062, (P[..., 2] - cc[2]) / 0.092
    ang = np.arctan2(dz, dx)
    rr = 1.0 + 0.22 * np.sin(ang * 3.0 + 0.5) + 0.12 * np.sin(ang * 7.0 + 2.0)
    front = P[..., 1] < -0.05
    sd = np.where(front, (rr - np.hypot(dx, dz)) * 0.06, -1.0)
    V, F, used = cut(P, sd, closed=True)
    ember = np.where(front.ravel()[used], smoothstep(1.45, 1.0, np.hypot(dx, dz).ravel()[used] / rr.ravel()[used]), 0.0)
    ember = ember * (0.45 + 0.55 * (0.5 + 0.5 * pnoise(V, 45.0, 9)))
    sheet = Part('Sheet', 1).add((V, F), ember=np.clip(ember, 0, 1))
    fig.add(sheet, solidify=0.008)
    # Behind the tear: darkness, and the coal.
    fig.add(Part('Hollow', 0).add(ellipsoid((0.045, -0.1, 1.3), (0.13, 0.075, 0.15)), cavity=1.0))
    fig.coal = (np.array([0.058, -0.152, 1.312]), 0.052)

    # The hood: the sheet drawn over the head into a deep, rounded cowl that overhangs the face, the corner of the
    # sheet hanging down the back; the face a dark hollow.
    hood_keys = [(1.34, 0.01, 0.27, 0.205, 0.0), (1.41, 0.012, 0.235, 0.178, 0.0), (1.48, 0.016, 0.185, 0.155, 0.0),
                 (1.53, 0.018, 0.15, 0.142, 0.008), (1.58, 0.012, 0.148, 0.15, 0.035), (1.65, 0.006, 0.156, 0.158, 0.058),
                 (1.72, 0.01, 0.158, 0.16, 0.07), (1.775, 0.02, 0.15, 0.154, 0.062), (1.815, 0.032, 0.128, 0.136, 0.04),
                 (1.845, 0.044, 0.096, 0.106, 0.015), (1.863, 0.05, 0.058, 0.066, 0.0), (1.871, 0.052, 0.016, 0.018, 0.0)]
    hk = np.asarray(hood_keys)
    rng = np.random.default_rng(11)
    hem_noise = rng.uniform(0, 1, 120)

    def hood_xyz(th, zh):
        """The hood's surface at angles th and heights zh (arrays of one shape)."""
        yc, w, d, fx = (np.interp(zh, hk[:, 0], hk[:, k]) for k in range(1, 5))
        capelet = smoothstep(1.5, 1.35, zh)
        hf = np.cos(9 * th + 0.4) * 0.6 + np.cos(14 * th + 2.0) * 0.4
        rad = 1.0 + capelet * 0.08 * (hf - 0.3 * np.abs(hf)) + 0.015 * np.cos(4 * th + 1.0) * (1 - capelet)
        X = w * np.sin(th) * rad
        Y = yc - d * np.cos(th) * rad - fx * np.clip(np.cos(th), 0, 1) ** 3
        # The cowl's rim sags: its top edge droops forward and down over the face.
        Z = zh - 0.03 * smoothstep(1.66, 1.8, zh) * np.clip(np.cos(th), 0, 1) ** 6
        hem = 0.035 * (0.5 + 0.5 * np.sin(th * 13.0) * np.sin(th * 5.0 + 1.0))
        Z = Z + hem * smoothstep(1.42, 1.34, zh)
        return np.stack([X, Y, Z], -1)

    zh = np.concatenate([np.linspace(1.34, 1.53, 22)[:-1], np.linspace(1.53, 1.871, 40)])
    th = np.linspace(0.0, 2.0 * np.pi, 120, endpoint=False)
    TH, ZH = np.meshgrid(th, zh)
    P = hood_xyz(TH, ZH)
    P[..., 2] += (0.02 * hem_noise)[None, :] * smoothstep(1.42, 1.34, ZH)
    sd = (1.0 - np.hypot(wrap(TH) / 0.62, (ZH - 1.66) / 0.105)) * 0.06
    V, F, used = cut(P, sd)
    hood = Part('Hood', 1).add((V, F))
    top = P[-1]
    hood.add((np.vstack([top, top.mean(0) + (0.0, 0.0, 0.004)]), [fan(len(top), np.arange(len(top)), top=True)]))
    fig.add(hood, solidify=0.008)
    # The cowl's edge, rolled and a little thick, round the hollow.
    a = np.linspace(0.0, 2.0 * np.pi, 64, endpoint=False)
    edge = hood_xyz(0.62 * np.cos(a), 1.66 + 0.105 * np.sin(a))
    rim = tube(np.vstack([edge, edge[:3]]), 0.011, segs=10, domes=1)
    fig.add(Part('HoodRim', 1).add(rim))
    fig.add(Part('Face', 0).add(ellipsoid((0.0, -0.0, 1.665), (0.112, 0.12, 0.13)), cavity=1.0))
    cords = Part('Cords', 3)
    cords.add(torus((0.0, 0.012, 1.523), (0.0, 0.0, 1.0), 0.136, 0.0075, squash=1.0))
    for s in sides:
        cords.add(tube(spline([(s * 0.012, -0.142, 1.52), (s * 0.02, -0.15, 1.47), (s * 0.016, -0.152, 1.41)], 10),
                       0.0055, segs=8))
        cords.add(ellipsoid((s * 0.016, -0.152, 1.408), (0.009, 0.009, 0.011), segs=10, rings=6))
    fig.add(cords)

    # The hands: bony, long-fingered, curled into claws; a cord bound round each wrist.
    wrists = fig.add(Part('WristCords', 3))
    for s in sides:
        f = hdir[s]
        limb = [tube([W[s] - f * 0.1, W[s] - f * 0.03, W[s] + f * 0.01], [0.026, 0.022, 0.019], [0.03, 0.026, 0.024],
                     segs=16, up=hup[s])]
        limb += hand(W[s], f, hup[s], s, curl=curls, spread=spread, thumb=0.45, length=1.18, thin=0.82, claw=0.55,
                     knuckle=1.25)
        fig.add(Part(f'Hand{s:+.0f}', 0).add(union(limb, 0.0024, smooth=3)))
        wrists.add(torus(W[s] - f * 0.045, f, 0.027, 0.0055, squash=1.0))

    # Bound at the waist: two turns of winding band, knotted, the ends trailing.
    bands = Part('Bands', 2)
    for k, (z0, z1, a0) in enumerate(((0.975, 1.03, 0.4), (1.04, 0.99, 2.2))):
        a = np.linspace(a0, a0 + 2.0 * np.pi * 1.04, 90)
        zb = np.linspace(z0, z1, 90)
        path = np.stack([0.148 * np.sin(a), 0.008 - 0.124 * np.cos(a), zb], 1)
        ribbon(bands, path, 0.045, n=150, cols=5, out='radial', axis_center=(0.0, 0.008, 0.0), curl=0.15, seed=k)
    if attack:
        ends = ([(0.07, -0.135, 1.0), (0.1, -0.08, 0.92), (0.12, 0.2, 0.86), (0.13, 0.55, 0.82), (0.14, 0.85, 0.8)],
                [(0.05, -0.14, 0.99), (0.04, -0.06, 0.9), (0.03, 0.25, 0.8), (0.02, 0.62, 0.72)])
    else:
        ends = ([(0.07, -0.135, 1.0), (0.1, -0.12, 0.86), (0.13, -0.02, 0.68), (0.15, 0.2, 0.52), (0.16, 0.42, 0.42)],
                [(0.05, -0.14, 0.99), (0.05, -0.12, 0.84), (0.04, -0.04, 0.7), (0.03, 0.12, 0.6)])
    for k, path in enumerate(ends):
        ribbon(bands, path, [(0.0, 0.04), (1.0, 0.026)], n=70, cols=5, out=(0.0, -1.0, 0.2), curl=0.2, twist=0.35,
               wave=0.03, ragged=0.12, fade_from=0.45, seed=5 + k)
    bands.add(ellipsoid((0.062, -0.145, 1.0), (0.03, 0.022, 0.026)))
    fig.add(bands, solidify=0.005)

    # Below the band, the sheet runs on into the tail and tears.
    if attack:
        path = [(0.0, 0.0, 1.03), (0.0, 0.1, 0.88), (0.0, 0.36, 0.74), (0.0, 0.72, 0.64), (0.0, 1.1, 0.58),
                (0.0, 1.46, 0.56)]
    else:
        path = [(0.0, 0.0, 1.03), (0.0, 0.03, 0.86), (0.0, 0.14, 0.66), (0.0, 0.38, 0.5), (0.0, 0.7, 0.38),
                (0.0, 1.05, 0.32)]
    size = [(0.0, 0.135, 0.11), (0.15, 0.142, 0.118), (0.35, 0.118, 0.104), (0.55, 0.088, 0.082), (0.8, 0.052, 0.05),
            (1.0, 0.026, 0.026)]
    fig.add(shroud(Part('Tail', 1), path, size, strips=4, split=0.5, ends=(0.8, 1.0), spread=0.12, seed=8,
                   fade_from=0.25, folds=0.42, narrow=0.6, harmonics=(4, 6, 9, 11)), solidify=0.007)
    fig.add(Part('Coal', 0).add(coal_piece(*fig.coal)), material='coal')
    if attack:
        # The hooded head thrown back to shriek, against the lean.
        def lift(V):
            a = math.radians(-18.0) * smoothstep(1.5, 1.6, V[:, 2])
            pivot_y, pivot_z = 0.01, 1.52
            y, z = V[:, 1] - pivot_y, V[:, 2] - pivot_z
            out = V.copy()
            out[:, 1] = pivot_y + y * np.cos(a) - z * np.sin(a)
            out[:, 2] = pivot_z + y * np.sin(a) + z * np.cos(a)
            return out
        for part, _ in fig.parts:
            if part.name in ('Hood', 'HoodRim', 'Face', 'Cords'):
                part.V = [lift(V) for V in part.V]
    fig.warp(lean(math.radians(22 if attack else 4)))
    return fig


# --- Option C: the hungry dead ---

# A frame too thin for its clothes: (z, half width, half depth, y of its middle, power), hunched forward at the top.
TORSO_C = [(0.96, 0.112, 0.08, 0.02, 2.2), (1.04, 0.108, 0.078, 0.02, 2.2), (1.12, 0.126, 0.092, 0.012, 2.2),
           (1.2, 0.145, 0.108, 0.0, 2.3), (1.28, 0.156, 0.117, -0.01, 2.4), (1.35, 0.16, 0.113, -0.024, 2.5),
           (1.41, 0.163, 0.1, -0.044, 2.8), (1.45, 0.14, 0.086, -0.058, 3.0), (1.48, 0.09, 0.066, -0.07, 2.6),
           (1.5, 0.055, 0.05, -0.078, 2.2)]


def rib_strokes(V, ribs, radius=0.013, amount=0.007):
    """How far each vertex rises over the nearest rib: ribs are polylines on the torso."""
    rise = np.zeros(len(V))
    for line in ribs:
        d = np.min(np.linalg.norm(V[:, None, :] - line[None, :, :], axis=2), axis=1)
        rise = np.maximum(rise, smoothstep(radius, 0.0, d))
    return rise * amount


def figure_C(pose):
    attack = pose == 'attack'
    fig = Figure('C', pose)
    sides = (1.0, -1.0)
    S = {s: np.array([s * 0.165, -0.05, 1.425]) for s in sides}
    if attack:
        E = {s: np.array([s * 0.33, -0.36, 1.38]) for s in sides}
        Wr = {s: np.array([s * 0.47, -0.66, 1.42]) for s in sides}
        hdir = {s: unit([s * 0.35, -1.0, 0.05]) for s in sides}
        hup = {s: unit([s * 0.1, 0.25, 1.0]) for s in sides}
        curls, spread = (0.22, 0.28, 0.34, 0.42), 0.3
    else:
        E = {s: np.array([s * 0.235, -0.165, 1.14]) for s in sides}
        Wr = {s: np.array([s * 0.225, -0.3, 0.86]) for s in sides}
        hdir = {s: unit([s * 0.05, -0.4, -1.0]) for s in sides}
        hup = {s: unit([s * 1.0, -0.25, 0.0]) for s in sides}
        curls, spread = (0.42, 0.52, 0.6, 0.7), 0.1
    pivot = np.array([0.0, -0.245, 1.565])
    if attack:
        R = rotation((1, 0, 0), math.radians(-36))
        jaw_open = math.radians(46)
    else:
        R = rotation((0, 1, 0), math.radians(7)) @ rotation((1, 0, 0), math.radians(-6))
        jaw_open = math.radians(26)

    # The body: a ribcage on a waist too thin for it, bony shoulders and collarbones, a long neck thrust forward.
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    z = np.linspace(0.96, 1.5, 44)
    pieces = [closed_loft(torso_points(TORSO_C, z, theta))]
    for s in sides:
        pieces.append(tube([(s * 0.02, -0.11, 1.445), (s * 0.09, -0.1, 1.462), (s * 0.165, -0.065, 1.47)], 0.0115,
                           segs=12))
        pieces.append(ellipsoid(S[s] + np.array([-s * 0.004, 0.0, 0.004]), (0.036, 0.042, 0.04)))
        pieces.append(ellipsoid((s * 0.085, 0.085, 1.36), (0.055, 0.022, 0.07),
                                rotation((0, 0, 1), s * 0.3).T, 20, 12))
    for k in range(10):
        zz = 1.0 + k * 0.047
        yb = np.interp(zz, [p[0] for p in TORSO_C], [p[3] + p[2] for p in TORSO_C])
        pieces.append(ellipsoid((0.0, yb - 0.004, zz), (0.012, 0.012, 0.015), segs=10, rings=6))
    neck = [(0.0, -0.065, 1.46), (0.0, -0.15, 1.525), pivot + R @ np.array([0.0, 0.0, 0.03])]
    pieces.append(tube(spline(neck, 12), np.linspace(0.036, 0.031, 12), segs=16))
    for s in sides:
        pieces.append(tube(spline([(s * 0.022, -0.112, 1.45), (s * 0.03, -0.175, 1.52),
                                   pivot + R @ np.array([s * 0.042, 0.0, 0.045])], 10), 0.0085, segs=10))
    pieces.append(ellipsoid((0.0, -0.17, 1.505), (0.011, 0.012, 0.016)))
    body = union(pieces, 0.0035, smooth=4)
    V = body[0]
    # Ribs: six a side, sloping down to the front, standing out of a sunken chest; a sunken belly under them.
    ribs = []
    for k in range(6):
        zb = 1.37 - k * 0.043
        for s in sides:
            a = np.linspace(2.55, 0.42 + k * 0.03, 60)
            zr = zb - 0.075 * (1.0 - (a - a[-1]) / (a[0] - a[-1]))
            pts = np.array([torso_points(TORSO_C, [zz], np.array([s * aa]), grow=0.0)[0, 0] for aa, zz in zip(a, zr)])
            ribs.append(pts)
    rise = rib_strokes(V, ribs)
    Nn = normals(*body)
    V = V + Nn * (rise - 0.003)[:, None]
    body = (V, body[1])
    body = push(body, (0.0, -0.075, 1.07), (0.085, 0.05, 0.06), -0.018)
    tc, zc = 0.32, 1.285
    pc = torso_points(TORSO_C, [zc], np.array([tc]))[0, 0]
    nc = torso_normal(tc)
    body = push(body, pc, (0.075, 0.075, 0.085), -0.04)
    dist = np.linalg.norm(body[0] - pc, axis=1)
    jitter = 0.5 + 0.5 * pnoise(body[0], 55.0, 3)
    cav = smoothstep(0.065, 0.035, dist) * 0.95
    ember = smoothstep(0.035, 0.06, dist) * smoothstep(0.095, 0.07, dist) * (0.35 + 0.65 * jitter)
    fig.add(Part('Body', 0).add(body, cavity=cav, ember=ember))
    fig.coal = (pc - nc * 0.03, 0.05)
    # Two ribs still cross in front of the coal: a cage.
    cage = []
    for zr, a0, a1 in ((1.305, 0.08, 0.62), (1.255, 0.1, 0.6)):
        a = np.linspace(a0, a1, 16)
        pts = torso_points(TORSO_C, np.full(16, zr) - 0.02 * (a - a0), a, grow=-0.004)
        pts = np.array([pts[i, i] for i in range(16)])
        cage.append(tube(pts, 0.0072, segs=10))
    fig.add(Part('Cage', 0).add(union(cage, 0.0022, smooth=2)))

    # The head, thrust forward on the neck, looking up from under its brow, the jaw hanging.
    face = Part('Head', 0)
    head_parts(face, HEAD_C, R, pivot, jaw_open, neck=[(0.0, -0.12, 1.5), (0.0, -0.19, 1.54)],
               neck_radius=(0.031, 0.03, 0.03), teeth=True)
    fig.add(face)

    # Long arms and big hands, curled hooks of fingers.
    for s in sides:
        dv = unit(Wr[s] - E[s])
        limb = [tube(spline([S[s], (S[s] + E[s]) * 0.5 + np.array([s * 0.006, 0.0, 0.0]), E[s]], 12),
                     np.interp(np.linspace(0, 1, 12), [0, 0.5, 1], [0.036, 0.027, 0.025]), segs=16),
                ellipsoid(E[s] - dv * 0.004, (0.027, 0.027, 0.03)),
                tube([E[s], (E[s] + Wr[s]) * 0.5, Wr[s]], [0.026, 0.022, 0.016], [0.028, 0.026, 0.021], segs=16,
                     up=hup[s])]
        limb += hand(Wr[s], hdir[s], hup[s], s, curl=curls, spread=spread, thumb=0.4, length=1.38, thin=0.78, claw=0.65,
                     knuckle=1.35, scale=1.05)
        fig.add(Part(f'Arm{s:+.0f}', 0).add(union(limb, 0.0024, smooth=3)))

    # The shirt hangs off the frame: open over the ribs, the right sleeve gone, the left torn at the elbow.
    zs = np.linspace(0.93, 1.5, 96)
    ts = np.linspace(0.0, 2.0 * np.pi, 180, endpoint=False)
    P = torso_points(TORSO_C, zs, ts, grow=0.026)
    tw, zz = np.meshgrid(wrap(ts), zs)
    hang = smoothstep(1.3, 0.95, zz)
    fold = np.cos(9 * tw + 0.5) * 0.6 + np.cos(15 * tw + 1.7) * 0.4
    P[..., 0] += (np.sin(tw) * 0.012 * hang * (fold - 0.3 * np.abs(fold)))
    P[..., 1] += (-np.cos(tw) * 0.012 * hang * (fold - 0.3 * np.abs(fold)))
    rng = np.random.default_rng(5)
    hem = 0.95 + 0.05 * (0.5 + 0.5 * np.sin(tw * 11.0 + 0.3) * np.sin(tw * 4.0)) + 0.03 * rng.uniform(0, 1, tw.shape[1])
    sd = hem - zz
    opening = 0.58 - 0.18 * smoothstep(1.42, 1.5, zz)
    sd = np.maximum(sd, (opening - np.abs(tw)) * 0.15)
    right_hole = np.hypot((tw + np.pi / 2) / 0.9, (zz - 1.47) / 0.15)
    left_hole = np.hypot((tw - np.pi / 2) / 0.55, (zz - 1.415) / 0.085)
    sd = np.maximum(sd, (1.0 - right_hole) * 0.05)
    sd = np.maximum(sd, (1.0 - left_hole) * 0.05)
    sd = np.maximum(sd, np.minimum(zz - 1.38, (0.38 - np.abs(tw + 0.68)) * 0.15))   # the right front torn away up top
    sd = np.maximum(sd, zz - 1.462)     # slipped off the bony shoulders: no fins standing up beside the neck
    for cx, cz, r in ((2.4, 1.12, 0.05), (-2.0, 1.25, 0.035), (1.2, 1.02, 0.04)):
        sd = np.maximum(sd, r - np.hypot((tw - cx) * 0.15, zz - cz))
    V, F, used = cut(P, sd)
    shirt = Part('Shirt', 1).add((V, F))
    s = 1.0
    d = unit(E[s] - S[s])
    sl = np.linspace(0.0, 1.0, 30)
    # The sleeve starts a little down the arm, inside the armhole, so its top edge never stands up off the shoulder.
    C = np.array([S[s] + d * 0.035 + (E[s] + d * 0.02 - S[s] - d * 0.035) * q for q in sl])
    T, N, B = frames(C, (0.0, 0.0, 1.0))
    a = np.linspace(0.0, 2.0 * np.pi, 40, endpoint=False)
    rad = np.interp(sl, [0, 1], [0.05, 0.045])[:, None] * (1.0 + 0.08 * np.cos(5 * a + 1.0)[None, :] * sl[:, None])
    P = C[:, None, :] + (N[:, None, :] * np.cos(a)[None, :, None] + B[:, None, :] * np.sin(a)[None, :, None]) * rad[..., None]
    cuff = 0.82 + 0.1 * np.sin(a * 4.0 + 0.5) + 0.05 * np.sin(a * 9.0)
    sdl = (sl[:, None] - cuff[None, :]) * 1.0
    V, F, used = cut(P, sdl)
    shirt.add((V, F))
    collar = []
    for zz_, r_ in ((1.475, 0.072), (1.5, 0.064)):
        collar.append(np.stack([r_ * np.sin(ts), -0.074 - r_ * 0.85 * np.cos(ts), np.full_like(ts, zz_)], -1))
    P = np.array(collar)
    sdc = np.broadcast_to((0.35 - np.abs(wrap(ts)))[None, :] * 0.1, P.shape[:2])
    V, F, used = cut(P, sdc)
    shirt.add((V, F))
    fig.add(shirt, solidify=0.005)
    # Suspenders (one still on its shoulder, the other fallen), a trouser band, a string tie.
    gear = Part('Suspenders', 2)
    lp = [(0.075, -0.115, 0.99), (0.1, -0.135, 1.16), (0.115, -0.13, 1.33), (0.12, -0.075, 1.465),
          (0.1, 0.03, 1.47), (0.075, 0.095, 1.36), (0.06, 0.12, 1.15), (0.06, 0.115, 0.99)]
    def lie_flat(C):
        """Away from the body, turning to face up where the strap crosses the top of the shoulder."""
        O = C * np.array([1.0, 1.0, 0.0])
        O = unit(O + 1e-9)
        O[:, 2] += 2.5 * smoothstep(1.4, 1.46, C[:, 2])
        return O
    ribbon(gear, lp, 0.026, n=90, cols=4, out=lie_flat, curl=0.1, seed=2)
    # The fallen strap hangs from its back button down the hip.
    rp = [(-0.06, 0.115, 0.99), (-0.1, 0.1, 0.9), (-0.13, 0.06, 0.8), (-0.135, 0.04, 0.72)]
    ribbon(gear, rp, 0.026, n=50, cols=4, out='radial', axis_center=(0.0, 0.0, 0.0), curl=0.1, twist=0.15, seed=3)
    band = np.array([np.stack([(0.125) * np.sin(ts), 0.02 - 0.095 * np.cos(ts), np.full_like(ts, z_)], -1)
                     for z_ in (0.955, 0.975, 0.995)])
    gear.add((band.reshape(-1, 3), [grid_faces(3, len(ts))]))
    fig.add(gear, solidify=0.006)
    tie = Part('Tie', 3)
    knot = np.array([0.0, -0.152, 1.47])
    tie.add(ellipsoid(knot, (0.012, 0.008, 0.009)))
    for s in sides:
        # Short ends, hanging off to the side: nothing crosses the coal.
        ribbon(tie, [knot, knot + (s * 0.02, -0.008, -0.012), knot + (-0.03 + s * 0.012, -0.014, -0.06),
                     knot + (-0.045 + s * 0.01, -0.02, -0.095)], [(0.0, 0.006), (1.0, 0.004)], n=24, cols=3,
               out=(0.0, -1.0, 0.0), curl=0.0, twist=0.2, seed=40 + int(s))
    fig.add(tie, solidify=0.003)

    # Below the waist: smoke-like tatters.
    if attack:
        path = [(0.0, 0.02, 1.02), (0.0, 0.14, 0.88), (0.0, 0.45, 0.78), (0.0, 0.82, 0.72), (0.0, 1.2, 0.7),
                (0.0, 1.58, 0.7)]
    else:
        path = [(0.0, 0.02, 1.02), (0.0, 0.07, 0.84), (0.0, 0.24, 0.64), (0.0, 0.52, 0.5), (0.0, 0.86, 0.42),
                (0.0, 1.24, 0.38)]
    size = [(0.0, 0.118, 0.085), (0.12, 0.122, 0.092), (0.35, 0.095, 0.08), (0.6, 0.065, 0.06), (0.85, 0.04, 0.04),
            (1.0, 0.02, 0.02)]
    fig.add(shroud(Part('Tatters', 0), path, size, strips=11, split=0.2, ends=(0.72, 1.0), spread=0.16, seed=12,
                   fade_from=0.14, folds=0.45, narrow=0.8, wave=0.07, harmonics=(5, 8, 11, 14)), solidify=0.004)
    fig.add(Part('Coal', 0).add(coal_piece(*fig.coal)), material='coal')
    fig.warp(lean(math.radians(36 if attack else 14)))
    return fig


# --- Option D: the user's pick (2026-10-06), A's homesteader made gaunt, with C's hungry face and C's lunge ---

# Wasted to the bone in his own clothes: a thin waist, narrow shoulders rolled forward, a hollow chest.
TORSO_D = [(0.90, 0.150, 0.100, 0.005, 2.2), (0.98, 0.142, 0.094, 0.005, 2.2), (1.06, 0.132, 0.088, 0.0, 2.2),
           (1.16, 0.140, 0.094, -0.008, 2.3), (1.26, 0.155, 0.104, -0.014, 2.4), (1.34, 0.166, 0.108, -0.02, 2.6),
           (1.40, 0.175, 0.102, -0.028, 2.8), (1.45, 0.174, 0.094, -0.034, 3.2), (1.475, 0.162, 0.087, -0.038, 3.4),
           (1.50, 0.135, 0.079, -0.042, 3.0), (1.52, 0.098, 0.068, -0.045, 2.6), (1.54, 0.066, 0.06, -0.047, 2.2)]
# C's hungry face, sculpted further and a little more human: it has to sit under a hat (Abel's and Amos's later).
HEAD_D = dict(width=0.14, front=0.092, back=0.104, top=0.114, bottom=0.138, mouth=-0.072, brow=0.009, brow_shelf=0.004,
              socket=0.017, eyeball=0.0075, nose=0.02, cheek=0.007, zyg=0.004, hollow=0.011, nasolabial=0.0028,
              lips=0.002, lips_thin=0.0015, chin=0.007, jaw_angle=0.005, nape=0.24, jaw_taper=0.12, socket_dark=0.95,
              mouth_depth=0.045, temple=0.009, asym=0.12)


def tube_fn(C, rfn, segs=24, up=(0.0, 0.0, 1.0)):
    """A closed tube along C whose radius rfn(s, a) varies along it (s 0..1) and around it (a, radians): cloth with
    folds. Its ends are closed with flat fans (they sit inside other pieces)."""
    C = np.asarray(C, float)
    T, N, B = frames(C, up)
    s = np.linspace(0.0, 1.0, len(C))
    a = np.linspace(0.0, 2.0 * np.pi, segs, endpoint=False)
    S, A = np.meshgrid(s, a, indexing='ij')
    R = rfn(S, A)
    P = (C[:, None, :] + (N[:, None, :] * np.cos(A)[..., None] + B[:, None, :] * np.sin(A)[..., None]) * R[..., None])
    n = len(C) * segs
    V = np.vstack([P.reshape(-1, 3), C[0], C[-1]])
    return orient((V, [grid_faces(len(C), segs), fan(n, np.arange(segs)), fan(n + 1, np.arange(segs) + n - segs, top=True)]))


def sleeve(S, E, W, r0, r1, seed):
    """A loose shirt sleeve from the shoulder to just past the elbow: three folds winding down from the armpit, a pile of
    small folds above the rolled cuff, and the cloth sagging away from a thin arm."""
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
    return tube_fn(C, rfn, segs=28)


def figure_D(pose):
    attack, drift = pose == 'attack', pose == 'drift'
    fig = Figure('D', pose)
    sides = (1.0, -1.0)
    S = {s: np.array([s * 0.16, -0.022, 1.428]) for s in sides}
    if attack:
        # C's lunge: thrown forward, the left arm reaching long, the right clawing wide, the jaw dropped to shriek.
        E = {1.0: np.array([0.25, -0.33, 1.44]), -1.0: np.array([-0.3, -0.24, 1.33])}
        Wr = {1.0: np.array([0.31, -0.63, 1.53]), -1.0: np.array([-0.44, -0.47, 1.28])}
        hdir = {1.0: unit([0.15, -1.0, 0.15]), -1.0: unit([-0.45, -1.0, -0.1])}
        hup = {1.0: unit([0.05, 0.2, 1.0]), -1.0: unit([-0.2, 0.25, 1.0])}
        curls = {1.0: (0.15, 0.2, 0.28, 0.36), -1.0: (0.3, 0.4, 0.5, 0.62)}
        spread = {1.0: 0.28, -1.0: 0.32}
        lean_deg, hunch_deg, R, jaw = 34, 6, rotation((1, 0, 0), math.radians(-42)), math.radians(46)
    elif drift:
        E = {1.0: np.array([0.22, -0.2, 1.25]), -1.0: np.array([-0.22, -0.12, 1.18])}
        Wr = {1.0: np.array([0.25, -0.44, 1.18]), -1.0: np.array([-0.27, -0.34, 1.05])}
        hdir = {1.0: unit([0.1, -1.0, -0.25]), -1.0: unit([-0.15, -1.0, -0.4])}
        hup = {1.0: unit([0.3, 0.1, 1.0]), -1.0: unit([-0.5, 0.0, 1.0])}
        curls = {1.0: (0.3, 0.4, 0.5, 0.6), -1.0: (0.45, 0.55, 0.65, 0.75)}
        spread = {1.0: 0.18, -1.0: 0.12}
        lean_deg, hunch_deg, R, jaw = 16, 8, rotation((1, 0, 0), math.radians(-24)), math.radians(28)
    else:
        # Hanging, but not stiff: the elbows bend, one hand drifts ahead of the other, fingers curl unevenly.
        E = {1.0: np.array([0.205, -0.075, 1.12]), -1.0: np.array([-0.2, -0.04, 1.115])}
        Wr = {1.0: np.array([0.205, -0.2, 0.865]), -1.0: np.array([-0.225, -0.13, 0.84])}
        hdir = {1.0: unit([0.05, -0.55, -1.0]), -1.0: unit([-0.06, -0.35, -1.0])}
        hup = {1.0: unit([1.0, -0.25, 0.1]), -1.0: unit([-1.0, -0.15, 0.1])}
        curls = {1.0: (0.35, 0.5, 0.62, 0.75), -1.0: (0.2, 0.32, 0.42, 0.55)}
        spread = {1.0: 0.1, -1.0: 0.14}
        lean_deg, hunch_deg = 8, 9
        R, jaw = rotation((0, 1, 0), math.radians(-5)) @ rotation((1, 0, 0), math.radians(-14)), math.radians(18)

    # The shirt, loose on him now: torso, a collar too big for the neck, sleeves with folds, rolled cuffs.
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    z = np.linspace(0.90, 1.54, 40)
    P = torso_points(TORSO_D, z, theta)
    tails = smoothstep(1.0, 0.9, z)[:, None]
    folds = (np.cos(5 * theta + 0.7) * 0.6 + np.cos(9 * theta + 2.1) * 0.4)[None, :]
    P[..., 0] *= 1.0 + 0.06 * tails * folds
    P[..., 1] *= 1.0 + 0.06 * tails * folds
    P[0, :, 2] += 0.016 * np.sin(3 * theta + 1.0) + 0.009 * np.sin(7 * theta)
    pieces = [closed_loft(P)]
    collar = np.array([np.stack([0.05 * np.sin(theta), -0.043 - 0.047 * np.cos(theta), np.full_like(theta, zz)], -1)
                       for zz in (1.52, 1.55, 1.578)])
    pieces.append(closed_loft(collar))
    for k, s in enumerate(sides):
        pieces.append(sleeve(S[s], E[s], Wr[s], 0.056, 0.05, seed=31 + k))
        pieces.append(torus(E[s] + unit(Wr[s] - E[s]) * 0.04, Wr[s] - E[s], 0.046, 0.015, squash=0.85))
    shirt = union(pieces, 0.004, smooth=4)
    # The hollow chest: the breastbone sunk between the ribs, hollows under the collarbones (in the vest's V).
    shirt = push(shirt, (0.0, -0.11, 1.4), (0.03, 0.04, 0.09), -0.012)
    for s in sides:
        shirt = push(shirt, (s * 0.06, -0.09, 1.46), 0.032, -0.008)
    tc, zc = 0.3, 1.32
    pc = torso_points(TORSO_D, [zc], np.array([tc]))[0, 0]
    nc = torso_normal(tc)
    shirt = push(shirt, pc, 0.068, -0.02)
    dist = np.linalg.norm(shirt[0] - pc, axis=1)
    jitter = 0.5 + 0.5 * pnoise(shirt[0], 60.0, 5)
    cavity = smoothstep(0.06, 0.035, dist) * 0.9
    ember = smoothstep(0.03, 0.05, dist) * smoothstep(0.085, 0.062, dist) * (0.4 + 0.6 * jitter)
    fig.add(Part('Shirt', 1).add(shirt, cavity=cavity, ember=ember))
    fig.coal = (pc - nc * 0.012, 0.056)

    # The vest hangs off him: it stands clear of the hollow chest, slack folds run down the front panels, the cloth
    # pulls in short creases from each button, a few folds hang at the back and under the arms; the hem waves.
    zv = np.linspace(0.92, 1.545, 120)
    tv = np.linspace(0.0, 2.0 * np.pi, 220, endpoint=False)
    th, zz = np.meshgrid(wrap(tv), zv)
    g = lambda x, s: np.exp(-(x / s) ** 2)
    bridge = 0.009 * g(th, 0.5) * g(zz - 1.35, 0.08)
    fold = (sum(0.0055 * g(th - k, 0.07) for k in (0.42, 0.86, -0.45, -0.88)) * smoothstep(1.33, 1.05, zz)
            + sum(0.005 * g(th - k, 0.085) for k in (np.pi - 0.35, -np.pi + 0.35, np.pi - 0.8)) * smoothstep(1.42, 1.1, zz)
            + 0.005 * g(np.abs(th) - 1.57, 0.1) * smoothstep(1.32, 1.1, zz))
    buttons = (1.005, 1.07, 1.135, 1.2)
    for zb in buttons:
        for s in (1.0, -1.0):
            reach = s * (th - 0.03)
            fold = fold + (0.0035 * g(zz - zb - 0.06 * reach, 0.006) * smoothstep(0.02, 0.1, reach)
                           * smoothstep(0.5, 0.22, reach))
    P = np.zeros(th.shape + (3,))
    for i, zrow in enumerate(zv):
        P[i] = torso_points(TORSO_D, [zrow], tv, grow=0.0)[0]
    out = np.stack([np.sin(tv), -np.cos(tv), np.zeros_like(tv)], -1)[None, :, :]
    P = P + out * (0.016 + bridge + fold)[..., None]
    hem = 0.98 - 0.05 * np.exp(-((np.abs(th) - 0.2) / 0.13) ** 2) + 0.008 * np.sin(5 * th + 0.4)
    sd = np.maximum(hem - zz, np.where(np.abs(th) < 1.2, zz - (1.24 + 0.36 * np.abs(th)), -1.0))
    armhole = np.sqrt(((np.abs(th) - np.pi / 2) / 0.66) ** 2 + ((zz - 1.41) / 0.1) ** 2)    # gaping now
    sd = np.maximum(sd, (1.0 - armhole) * 0.06)
    sd = np.maximum(sd, zz - 1.53)
    ang = np.arctan2(zz - zc, (th - tc) * 0.17)
    rh = 0.07 * (1.0 + 0.12 * np.sin(ang * 5 + 1.0) + 0.08 * np.sin(ang * 11.0))
    hole = np.hypot((th - tc) * 0.17, zz - zc)
    sd = np.maximum(sd, rh - hole)
    V, F, used = cut(P, sd)
    vest_ember = smoothstep(0.03, 0.0, hole.ravel()[used] - rh.ravel()[used]) * (0.5 + 0.5 * pnoise(V, 50.0, 2))
    vest = Part('Vest', 2).add((V, F), ember=np.clip(vest_ember, 0, 1))
    n = torso_normal(0.03)
    button_axes = np.array([[0.0, 0.0, 1.0], np.cross(n, [0.0, 0.0, 1.0]), n])
    for zb in buttons:
        p = torso_points(TORSO_D, [zb], np.array([0.03]), grow=0.022)[0, 0]
        vest.add(ellipsoid(p, (0.0075, 0.0075, 0.0045), button_axes, 12, 8), cavity=0.45)
    fig.add(vest, solidify=0.007)

    # The bandana, loose round a thin neck.
    band = Part('Bandana', 3)
    band.add(torus((0.0, -0.046, 1.566), (0.0, -0.3, 1.0), 0.047, 0.011, squash=0.75))
    rows, cols = 14, 25
    zb = np.linspace(1.545, 1.43, rows)
    width = np.linspace(0.9, 0.03, rows)
    flap = np.array([torso_points(TORSO_D, [zr], np.linspace(-w, w, cols), grow=0.03)[0] for zr, w in zip(zb, width)])
    flap[..., 2] += 0.005 * np.sin(np.linspace(0, 3 * np.pi, cols))[None, :] * (1 - width[:, None])
    band.add((flap.reshape(-1, 3), [grid_faces(rows, cols, closed=False)]))
    band.add(ellipsoid((0.0, 0.0, 1.562), (0.018, 0.013, 0.015)))
    fig.add(band, solidify=0.005)

    # Forearms and hands: bony wrists, long fingers with long nails, knuckles standing out.
    for s in sides:
        dv = unit(Wr[s] - E[s])
        limb = [tube([E[s] + dv * 0.02, E[s] + dv * 0.12, (E[s] + Wr[s]) * 0.5, Wr[s] - dv * 0.01],
                     [0.033, 0.031, 0.026, 0.017], [0.036, 0.034, 0.03, 0.024], segs=20, up=hup[s]),
                ellipsoid(Wr[s] - dv * 0.012 + unit(np.cross(dv, hup[s])) * s * -0.018, (0.008, 0.008, 0.008))]
        limb += hand(Wr[s], hdir[s], hup[s], s, curl=curls[s], spread=spread[s], thumb=0.35, length=1.26, thin=0.82,
                     claw=0.3, knuckle=1.3, scale=1.02)
        fig.add(Part(f'Arm{s:+.0f}', 0).add(union(limb, 0.0024, smooth=3)))

    # The head on a thin neck thrust a little forward, cords standing in it, a hat on top and thin hair below it.
    pivot = np.array([0.0, -0.06, 1.615])
    face = Part('Head', 0)
    head_parts(face, HEAD_D, R, pivot, jaw, neck=[(0.0, -0.03, 1.49), (0.0, -0.048, 1.56)],
               neck_radius=(0.037, 0.035, 0.034), teeth=True)
    center = pivot + R @ np.array([0.0, -0.012, 0.09])
    for s in sides:
        face.add(tube(spline([(s * 0.018, -0.07, 1.53), (s * 0.026, -0.072, 1.575),
                              center + R @ np.array([s * 0.05, 0.006, -0.05])], 10), 0.0075, segs=10))
    face.add(ellipsoid(pivot + np.array([0.0, -0.03, -0.03]), (0.007, 0.009, 0.012)))
    fig.add(face)
    # Thin grey hair under the brim: uneven locks, clumped, some lifting off the collar.
    hair = Part('Hair', 0)
    rng = np.random.default_rng(17)
    for k in range(24):
        a = math.pi * rng.uniform(0.4, 1.0) * (1 if k % 2 else -1)
        root = np.array([0.07 * math.sin(a) * 1.02, -0.09 * math.cos(a) * 1.08 + 0.006, rng.uniform(0.03, 0.06)])
        out_dir = np.array([math.sin(a), -math.cos(a), 0.0])
        drop = rng.uniform(0.05, 0.15)
        flick = rng.uniform(-0.004, 0.016)
        side = np.array([math.cos(a), math.sin(a), 0.0]) * rng.uniform(-0.012, 0.012)
        path = [root, root + out_dir * 0.01 + (0.0, 0.0, -drop * 0.35),
                root + out_dir * 0.014 + side * 0.5 + (0.0, 0.006, -drop * 0.7),
                root + out_dir * (0.014 + flick) + side + (0.0, 0.012, -drop)]
        path = [center + R @ p for p in path]
        w0 = rng.uniform(0.008, 0.02)
        ribbon(hair, path, [(0.0, w0), (0.6, w0 * 0.7), (1.0, 0.002)], n=20, cols=4,
               out=lambda C, c0=center: unit(C - c0), curl=0.35, twist=0.25 * rng.uniform(-1, 1), seed=60 + k)
    for values in hair.attr:
        values[:, 1] = 0.3          # a little darker than the skin: grey hair
    fig.add(hair, solidify=0.003)
    felt, band_piece = hat_A()
    Rh = R @ rotation((1, 0, 0), math.radians(-4)) @ rotation((0, 1, 0), math.radians(5))
    at = pivot + R @ np.array([0.0, 0.0, 0.148])
    hat = Part('Hat', 2)
    for _, V, F in felt:
        hat.add((V @ Rh.T + at, F))
    fig.add(hat, solidify=0.007)
    fig.add(Part('HatBand', 3).add((band_piece[0] @ Rh.T + at, band_piece[1])), solidify=0.004)

    # Below the waist the shirt gives way to the shroud's strips; in the lunge they snap straight back.
    if attack:
        path = [(0.0, 0.01, 1.04), (0.0, 0.16, 0.92), (0.0, 0.5, 0.82), (0.0, 0.9, 0.78), (0.0, 1.3, 0.76),
                (0.0, 1.7, 0.76)]
    elif drift:
        path = [(0.0, 0.01, 1.04), (0.0, 0.08, 0.86), (0.0, 0.3, 0.68), (0.0, 0.6, 0.56), (0.0, 0.95, 0.5),
                (0.0, 1.3, 0.46)]
    else:
        path = [(0.0, 0.005, 1.04), (0.0, 0.04, 0.84), (0.0, 0.17, 0.62), (0.0, 0.42, 0.45), (0.0, 0.75, 0.33),
                (0.0, 1.1, 0.27)]
    size = [(0.0, 0.135, 0.092), (0.12, 0.142, 0.104), (0.3, 0.118, 0.1), (0.5, 0.088, 0.08), (0.75, 0.056, 0.052),
            (1.0, 0.028, 0.028)]
    fig.add(shroud(Part('Shroud', 0), path, size, strips=7, split=0.16, ends=(0.74, 1.0), spread=0.1, seed=4,
                   fade_from=0.22, folds=0.34, narrow=0.55, wave=0.035), solidify=0.006)
    fig.add(Part('Coal', 0).add(coal_piece(*fig.coal)), material='coal')
    fig.warp(lean(math.radians(hunch_deg), pivot_z=1.33, width=0.08))
    fig.warp(lean(math.radians(lean_deg)))
    return fig


FIGURES = {'A': figure_A, 'B': figure_B, 'C': figure_C, 'D': figure_D}
_cache = {}


def figure(key, pose='idle'):
    if (key, pose) not in _cache:
        t0 = time.time()
        _cache[(key, pose)] = FIGURES[key](pose)
        log(f'{key} {pose}: {_cache[(key, pose)].triangles()} triangles (concept density), built in {time.time() - t0:.1f} s')
        if 'bounds' in ARGV:
            for part, _ in _cache[(key, pose)].parts:
                V = np.vstack(part.V)
                log(f'  {part.name}: {np.round(V.min(0), 2)} .. {np.round(V.max(0), 2)}')
    return _cache[(key, pose)]


# --- Materials ---

_materials = {}


def _image(nodes, links, path, colorspace, vector, blend=0.3):
    node = nodes.new('ShaderNodeTexImage')
    node.image = bpy.data.images.load(path, check_existing=True)
    node.image.colorspace_settings.name = colorspace
    node.projection = 'BOX'
    node.projection_blend = blend
    links.new(vector, node.inputs['Vector'])
    return node


def _math(nodes, links, op, a, b=None, clamp=False):
    node = nodes.new('ShaderNodeMath')
    node.operation = op
    node.use_clamp = clamp
    for socket, value in zip(node.inputs, (a, b)):
        if value is None:
            continue
        if isinstance(value, (int, float)):
            socket.default_value = value
        else:
            links.new(value, socket)
    return node.outputs[0]


def _vadd(nodes, links, a, b):
    node = nodes.new('ShaderNodeVectorMath')
    node.operation = 'ADD'
    links.new(a, node.inputs[0])
    links.new(b, node.inputs[1])
    return node.outputs[0]


def _vscale(nodes, links, v, s):
    node = nodes.new('ShaderNodeVectorMath')
    node.operation = 'SCALE'
    links.new(v, node.inputs[0])
    if isinstance(s, (int, float)):
        node.inputs['Scale'].default_value = s
    else:
        links.new(s, node.inputs['Scale'])
    return node.outputs[0]


def _vmul(nodes, links, a, b):
    node = nodes.new('ShaderNodeVectorMath')
    node.operation = 'MULTIPLY'
    links.new(a, node.inputs[0])
    if isinstance(b, (list, tuple)):
        node.inputs[1].default_value = b[:3]
    else:
        links.new(b, node.inputs[1])
    return node.outputs[0]


def ghost_material(colors, coal, rim=RIM):
    """The ghost as the game would draw it (a masked, dithered master): see the module's docstring."""
    key = ('ghost', tuple(colors), coal)
    if key in _materials:
        return _materials[key]
    mat = bpy.data.materials.new('Ghost')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    attr = nodes.new('ShaderNodeAttribute')
    attr.attribute_name = 'Ghost'
    sep = nodes.new('ShaderNodeSeparateColor')
    links.new(attr.outputs['Color'], sep.inputs['Color'])
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.interpolation = 'CONSTANT'
    elements = ramp.color_ramp.elements
    elements[0].position, elements[0].color = 0.0, lt.hex_color(colors[0])
    elements[1].position, elements[1].color = 0.17, lt.hex_color(colors[1])
    for position, color in ((0.5, colors[2]), (0.83, colors[3])):
        e = elements.new(position)
        e.color = lt.hex_color(color)
    links.new(sep.outputs['Red'], ramp.inputs['Fac'])
    coords = nodes.new('ShaderNodeTexCoord')
    cloth = nodes.new('ShaderNodeMapping')
    cloth.inputs['Scale'].default_value = (3.0, 3.0, 3.0)
    links.new(coords.outputs['Object'], cloth.inputs['Vector'])
    grain = _image(nodes, links, lt.texture_path('Polymer', 'BC'), 'sRGB', cloth.outputs['Vector'])
    # The Polymer grain over the zone color (normalized to its own mid grey, so the tint keeps its value).
    base = _vmul(nodes, links, ramp.outputs['Color'], _vscale(nodes, links, grain.outputs['Color'], 1.25))
    dark = _math(nodes, links, 'MULTIPLY_ADD', sep.outputs['Green'], -0.93)
    dark.node.inputs[2].default_value = 1.0
    base = _vscale(nodes, links, base, dark)
    links.new(base, bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = 0.86
    bump = nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.12
    bump.inputs['Distance'].default_value = 0.002
    links.new(grain.outputs['Color'], bump.inputs['Height'])
    links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    # Emission: the fresnel rim, a faint glow of its own (ghosts never go fully dark) and the ember edge.
    facing = nodes.new('ShaderNodeLayerWeight')
    facing.inputs['Blend'].default_value = 0.35
    rim_k = _math(nodes, links, 'POWER', facing.outputs['Facing'], 2.8)
    rim_k = _math(nodes, links, 'MULTIPLY', rim_k, dark)
    rim_k = _math(nodes, links, 'MULTIPLY', rim_k, 2.0)
    rim_c = _vscale(nodes, links, _color(nodes, rim), rim_k)
    glow = _vscale(nodes, links, base, 0.09)
    ember = _vscale(nodes, links, _color(nodes, coal[0]), _math(nodes, links, 'MULTIPLY', sep.outputs['Blue'],
                                                                coal[1] * 0.6))
    links.new(_vadd(nodes, links, _vadd(nodes, links, rim_c, glow), ember), bsdf.inputs['Emission Color'])
    bsdf.inputs['Emission Strength'].default_value = 1.0
    # The fade: MacroNoise at two scales against the fade value; hashed, so a pixel is either drawn or not.
    fade_map = nodes.new('ShaderNodeMapping')
    fade_map.inputs['Scale'].default_value = (1.4, 1.4, 1.4)
    links.new(coords.outputs['Object'], fade_map.inputs['Vector'])
    fine_map = nodes.new('ShaderNodeMapping')
    fine_map.inputs['Scale'].default_value = (4.3, 4.3, 4.3)
    fine_map.inputs['Location'].default_value = (0.37, 0.11, 0.53)
    links.new(coords.outputs['Object'], fine_map.inputs['Vector'])
    path = os.path.join(lt.TEXTURE_DIR, 'MacroNoise', 'T_MacroNoise_M.png')
    n1 = _image(nodes, links, path, 'Non-Color', fade_map.outputs['Vector'])
    n2 = _image(nodes, links, path, 'Non-Color', fine_map.outputs['Vector'])
    n = _math(nodes, links, 'ADD', _math(nodes, links, 'MULTIPLY', n1.outputs['Color'], 0.62),
              _math(nodes, links, 'MULTIPLY', n2.outputs['Color'], 0.38))
    fade = _math(nodes, links, 'MULTIPLY_ADD', attr.outputs['Alpha'], 1.3)
    fade.node.inputs[2].default_value = -0.15
    alpha = _math(nodes, links, 'SUBTRACT', fade, n)
    # Steep: most of the shroud is either there or gone (holes and wisps), only a thin dithered band between.
    alpha = _math(nodes, links, 'MULTIPLY_ADD', alpha, 14.0, clamp=True)
    alpha.node.inputs[2].default_value = 0.5
    links.new(alpha, bsdf.inputs['Alpha'])
    mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = False
    mat.use_transparent_shadow = True
    _materials[key] = mat
    return mat


def _color(nodes, hex_value):
    node = nodes.new('ShaderNodeRGB')
    node.outputs[0].default_value = lt.hex_color(hex_value)
    return node.outputs[0]


def coal_material(coal):
    """The coal: a dark crust split by glowing cracks, hottest in the middle, all of it in the rank's color."""
    key = ('coal', coal)
    if key in _materials:
        return _materials[key]
    color, strength = coal
    mat = bpy.data.materials.new('Coal')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x1d110c)
    bsdf.inputs['Roughness'].default_value = 0.6
    coords = nodes.new('ShaderNodeTexCoord')
    vor = nodes.new('ShaderNodeTexVoronoi')
    vor.feature = 'DISTANCE_TO_EDGE'
    vor.inputs['Scale'].default_value = 26.0
    links.new(coords.outputs['Object'], vor.inputs['Vector'])
    crack = nodes.new('ShaderNodeMapRange')
    crack.inputs['From Min'].default_value = 0.0
    crack.inputs['From Max'].default_value = 0.08
    crack.inputs['To Min'].default_value = 1.0
    crack.inputs['To Max'].default_value = 0.25
    links.new(vor.outputs['Distance'], crack.inputs['Value'])
    facing = nodes.new('ShaderNodeLayerWeight')
    facing.inputs['Blend'].default_value = 0.5
    hot = _math(nodes, links, 'MULTIPLY_ADD', facing.outputs['Facing'], -0.55)
    hot.node.inputs[2].default_value = 1.0
    k = _math(nodes, links, 'MULTIPLY', crack.outputs['Result'], hot)
    k = _math(nodes, links, 'MULTIPLY', k, strength)
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(color)
    links.new(k, bsdf.inputs['Emission Strength'])
    _materials[key] = mat
    return mat


def flat_material(name, color, rough=0.9, emit=0.0):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = rough
    if emit:
        bsdf.inputs['Emission Color'].default_value = lt.hex_color(color)
        bsdf.inputs['Emission Strength'].default_value = emit
    return mat


# --- Scene ---

def reset():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for block in (bpy.data.meshes, bpy.data.curves, bpy.data.cameras, bpy.data.lights):
        for item in list(block):
            if item.users == 0:
                block.remove(item)
    for c in list(bpy.data.collections):
        bpy.data.collections.remove(c)


def collection(name):
    c = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(c)
    return c


def build(part, material, where, solidify=0.0, subsurf=0, location=(0.0, 0.0, 0.0), turn=0.0, shadow=True, **_):
    V = np.concatenate(part.V)
    mesh = to_mesh(part.name, V, part.F)
    mesh.validate()
    attr = np.concatenate(part.attr)
    rgba = np.column_stack([np.full(len(V), part.zone / 3.0), attr[:, 1], attr[:, 2], attr[:, 0]])
    col = mesh.color_attributes.new('Ghost', 'FLOAT_COLOR', 'POINT')
    col.data.foreach_set('color', rgba.astype(np.float32).ravel())
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.materials.append(material)
    obj = bpy.data.objects.new(part.name, mesh)
    where.objects.link(obj)
    if solidify:
        mod = obj.modifiers.new('Thickness', 'SOLIDIFY')
        mod.thickness = solidify
        mod.offset = -1.0
        mod.use_even_offset = False      # even thickness spikes where thin strips twist
        mod.use_rim = True
    if subsurf:
        mod = obj.modifiers.new('Subdivision', 'SUBSURF')
        mod.levels = mod.render_levels = subsurf
    obj.location = location
    obj.rotation_euler = (0.0, 0.0, turn)
    obj.visible_shadow = shadow
    return obj


def instantiate(fig, where, tint=0, coal=RANKS[0][1:], location=(0.0, 0.0, 0.0), turn=0.0):
    ghost = ghost_material(TINTS[fig.key][tint], coal)
    ember = coal_material(coal)
    return [build(part, ember if options.get('material') == 'coal' else ghost, where, location=location, turn=turn,
                  **{k: v for k, v in options.items() if k != 'material'}) for part, options in fig.parts]


def sky(strength=0.9):
    world = bpy.data.worlds.get('UnpaidSky') or bpy.data.worlds.new('UnpaidSky')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    bg.inputs['Strength'].default_value = strength
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    # The view direction's height, -1..1, to 0..1 (0.5 the horizon).
    height = _math(nodes, links, 'MULTIPLY_ADD', split.outputs['Z'], 0.5)
    height.node.inputs[2].default_value = 0.5
    links.new(height, ramp.inputs['Fac'])
    # Late summer, an hour before sunset: a warm haze at the horizon, a soft blue above, dusty ground bounce.
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.44, lt.hex_color(0x6a5642)
    els[1].position, els[1].color = 0.88, lt.hex_color(0x7894bb)
    for position, color in ((0.5, 0xf0c48c), (0.58, 0xd5c3a3)):
        e = els.new(position)
        e.color = lt.hex_color(color)
    links.new(ramp.outputs['Color'], bg.inputs['Color'])
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    bpy.context.scene.world = world


def sun(toward, strength=4.2, color=(1.0, 0.77, 0.52), where=None):
    light = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN'))
    light.data.energy = strength
    light.data.color = color
    light.data.angle = math.radians(1.5)
    light.rotation_euler = (-Vector(toward).normalized()).to_track_quat('-Z', 'Y').to_euler()
    (where or bpy.context.scene.collection).objects.link(light)
    return light


def camera(location, target, lens=50.0, ortho=None):
    cam = bpy.data.objects.new('Camera', bpy.data.cameras.new('Camera'))
    cam.location = location
    cam.rotation_euler = (Vector(target) - Vector(location)).to_track_quat('-Z', 'Y').to_euler()
    if ortho:
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = ortho
    else:
        cam.data.lens = lens
    cam.data.clip_start = 0.05
    cam.data.clip_end = 400.0
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    return cam


def label(text, location, size=0.12, rotation_euler=(math.radians(90.0), 0.0, 0.0), align='CENTER', color=0x2a2622,
          where=None):
    curve = bpy.data.curves.new('Label', 'FONT')
    curve.body = text
    curve.size = size
    curve.align_x = align
    obj = bpy.data.objects.new('Label', curve)
    obj.location = location
    obj.rotation_euler = rotation_euler
    curve.materials.append(flat_material(f'Ink{color:06x}', color, 1.0, 0.0))
    obj.visible_shadow = False
    (where or bpy.context.scene.collection).objects.link(obj)
    return obj


def mesh_object(name, V, faces, material, where, uvs=None, ao=None):
    mesh = to_mesh(name, V, faces)
    if uvs is not None:
        layer = mesh.uv_layers.new(name='UVMap')
        loops = np.empty(len(mesh.loops), dtype=np.int64)
        mesh.loops.foreach_get('vertex_index', loops)
        layer.data.foreach_set('uv', np.asarray(uvs, float)[loops].ravel())
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'POINT')
    a = np.ones(len(V)) if ao is None else np.asarray(ao, float)
    col.data.foreach_set('color', np.column_stack([np.zeros(len(V)), np.full(len(V), 0.5), np.zeros(len(V)), a])
                         .astype(np.float32).ravel())
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    where.objects.link(obj)
    return obj


def ground(where, color=0xf4e0b4, radius=60.0, plain=False):
    """The ground: the dirt set tinted to Ransom's Rest's ochre soil (or a plain studio floor)."""
    a = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    rr = np.array([0.0, 2.0, 5.0, 10.0, 20.0, radius])
    V = [(0.0, 0.0, 0.0)]
    for r in rr[1:]:
        V += [(r * math.cos(t), r * math.sin(t), 0.0) for t in a]
    V = np.array(V)
    faces = [fan(0, np.arange(96) + 1, top=True)]
    faces.append(grid_faces(len(rr) - 1, 96)[:, ::-1] + 1)
    if plain:
        mat = flat_material('Floor', color, 1.0)
    else:
        mat = lt.material('GroundDirt', name='UnpaidGround', tint=color)
    return mesh_object('Ground', V, faces, mat, where, uvs=V[:, :2] / 4.0)


def grass(where, center=(0.0, 0.3), radius=8.5, count=2600, seed=5, keep=None):
    """Dry late-summer grass in tufts on the foliage palette's straw and dry swatches (keep(x, y): where tufts may go)."""
    rng = np.random.default_rng(seed)
    colors = ('GrassDry', 'Straw', 'GrassYellow', 'DryTan', 'GrassDry', 'GrassOlive')
    V, F, UV, AO = [], [], [], []
    for _ in range(count):
        r = radius * math.sqrt(rng.uniform(0.0, 1.0))
        a = rng.uniform(0.0, 2.0 * np.pi)
        cx, cy = center[0] + r * math.cos(a), center[1] + r * math.sin(a)
        if keep is not None and not keep(cx, cy):
            continue
        tall = rng.uniform(0.6, 1.0) * (1.25 if rng.uniform() < 0.2 else 1.0)
        color = colors[rng.integers(len(colors))]
        for _ in range(rng.integers(5, 10)):
            root = np.array([cx + rng.normal(0, 0.03), cy + rng.normal(0, 0.03), -0.01])
            h = rng.uniform(0.12, 0.36) * tall
            heading = rng.uniform(0.0, 2.0 * np.pi)
            hd = np.array([math.cos(heading), math.sin(heading), 0.0])
            side = np.array([-hd[1], hd[0], 0.0])
            lean_k = rng.uniform(0.2, 0.55)
            w = rng.uniform(0.005, 0.009)
            n0 = len(V)
            for k in range(4):
                q = k / 3.0
                p = root + np.array([0.0, 0.0, h * q]) + hd * lean_k * h * q * q
                width = w * (1.0 - 0.85 * q)
                V += [p - side * width, p + side * width]
                UV += [swatch(color, q, 0.0), swatch(color, q, 1.0)]
                AO += [0.6 + 0.4 * q] * 2
            for k in range(3):
                b = n0 + 2 * k
                F.append((b, b + 1, b + 3, b + 2))
    return mesh_object('Grass', np.array(V), [np.array(F)], lt.material('FoliagePalette'), where, uvs=np.array(UV),
                       ao=np.array(AO))


def headboard(where, location, turn=0.0, tilt=0.0, height=0.8):
    """An old wooden grave headboard, for the boot hill around the hero views."""
    w, d = 0.38, 0.045
    pts = []
    prof = [(-w / 2, 0.0), (w / 2, 0.0), (w / 2, height * 0.86), (w * 0.3, height * 0.97), (0.0, height),
            (-w * 0.3, height * 0.97), (-w / 2, height * 0.86)]
    for y in (-d / 2, d / 2):
        pts += [(x, y, z) for x, z in prof]
    V = np.array(pts)
    n = len(prof)
    faces = [np.array([list(range(n))[::-1]]), np.array([list(range(n, 2 * n))])]
    sidesf = [(k, (k + 1) % n, n + (k + 1) % n, n + k) for k in range(n)]
    faces.append(np.array(sidesf))
    R = rotation((0, 0, 1), turn) @ rotation((1, 0, 0), tilt)
    V = V @ R.T + np.asarray(location, float)
    uv = np.column_stack([V[:, 0] + V[:, 1], V[:, 2]]) * 0.8
    mat = lt.material('WoodPlanks', name='UnpaidBoard', tint=0xcfc2ac)
    obj = mesh_object('Headboard', V, faces, mat, where, uvs=uv)
    obj.data.polygons.foreach_set('use_smooth', np.zeros(len(obj.data.polygons), dtype=bool))
    return obj


def post(where, location, height=1.8):
    """A 1.8 m measuring post with a tick every half meter."""
    mat = flat_material('Post', 0x3b3631, 0.8)
    tick = flat_material('Tick', 0xe9e2d2, 0.8)
    x, y, z = location

    def box(c, s):
        V = np.array([[c[0] + sx * s[0], c[1] + sy * s[1], c[2] + sz * s[2]]
                      for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)])
        return V, [np.array([(0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4), (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)])]

    objs = [mesh_object('Post', *box((x, y, z + height / 2), (0.03, 0.03, height / 2)), mat, where)]
    for k in range(1, int(height / 0.5) + 1):
        objs.append(mesh_object('Tick', *box((x + 0.05, y, z + k * 0.5), (0.05, 0.035, 0.006)), tick, where))
    objs.append(mesh_object('Tick', *box((x, y, z + height), (0.07, 0.04, 0.008)), tick, where))
    return objs


def setup_render(resolution, samples=64):
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = samples
    scene.eevee.use_shadows = True
    scene.eevee.shadow_ray_count = 2
    scene.eevee.shadow_step_count = 8
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    # Bloom on what glows (the coal), as the game's post-process would.
    scene.use_nodes = True
    tree = scene.node_tree
    tree.nodes.clear()
    layers = tree.nodes.new('CompositorNodeRLayers')
    glare = tree.nodes.new('CompositorNodeGlare')
    glare.glare_type = 'BLOOM'
    for name, value in (('Threshold', 1.6), ('Strength', 0.55), ('Size', 0.55), ('Smoothness', 0.3)):
        if name in glare.inputs:
            glare.inputs[name].default_value = value
    composite = tree.nodes.new('CompositorNodeComposite')
    tree.links.new(layers.outputs['Image'], glare.inputs['Image'])
    tree.links.new(glare.outputs['Image'], composite.inputs['Image'])


def render(path, resolution, samples=64):
    setup_render(resolution, samples)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    scene = bpy.context.scene
    scene.render.filepath = path
    t0 = time.time()
    bpy.ops.render.render(write_still=True)
    log(f'rendered {os.path.relpath(path, lt.REPO)} in {time.time() - t0:.0f} s')
    return path


def shot_path(key, name):
    return os.path.join(OUT, f'Unpaid_{key}_{name}.png')


# --- Shots ---

def shot_hero(key):
    """Three-quarter view, drifting low over the dry grass of boot hill in the late gold light."""
    reset()
    where = collection('Hero')
    fig = figure(key, 'idle')
    instantiate(fig, where)
    ground(where)
    grass(where, seed=5)
    headboard(where, (-1.5, 2.9, 0.0), turn=0.25, tilt=0.07)
    headboard(where, (1.6, 3.6, 0.0), turn=-0.2, tilt=-0.05, height=0.7)
    sky()
    sun((-0.62, -0.72, 0.34))
    lo, hi = fig.bounds()
    target = Vector(((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2 - 0.1, (lo[2] + hi[2]) / 2 + 0.05))
    direction = Vector((0.78, -1.0, 0.13)).normalized()
    camera(target + direction * 4.9, target, lens=50.0)
    render(shot_path(key, 'hero'), (1600, 1000))


def lineup(items, gap=0.45):
    """x offsets laying items (objects' bounds along x: (lo, hi)) left to right with a gap, centered on 0."""
    widths = [hi - lo for lo, hi in items]
    total = sum(widths) + gap * (len(items) - 1)
    x, out = -total / 2.0, []
    for (lo, hi), w in zip(items, widths):
        out.append(x - lo)
        x += w + gap
    return out, total


def shot_turnaround(key):
    """Front, side and back, orthographic, beside a 1.8 m post."""
    reset()
    where = collection('Turn')
    fig = figure(key, 'idle')
    turns = (0.0, math.pi / 2.0, math.pi)
    spans = [(fig.bounds(t)[0][0], fig.bounds(t)[1][0]) for t in turns]
    xs, total = lineup([(-0.08, 0.08)] + spans, gap=0.55)
    post(where, (xs[0], 0.0, 0.0))
    label('1.8 m', (xs[0] + 0.1, -0.05, 1.78), 0.075, align='LEFT')
    for x, t, name in zip(xs[1:], turns, ('front', 'side', 'back')):
        instantiate(fig, where, location=(x, 0.0, 0.0), turn=t)
        label(name, (x + (fig.bounds(t)[0][0] + fig.bounds(t)[1][0]) / 2, -0.6, 0.06), 0.1)
    label(TITLES[key], (xs[1] - 0.3, -1.5, 2.12), 0.12, align='LEFT')
    studio(where)
    sun((-0.4, -0.85, 0.45), strength=3.8)
    width = total + 0.6
    height = 2.45
    # Looking down a little (8 degrees), so the floor and the shadows show under the floating figures.
    camera((0.0, -20.0, 1.07 + 20.0 * math.tan(math.radians(8.0))), (0.0, 0.0, 1.07), ortho=width)
    res = (2100, int(2100 * height / width))
    render(shot_path(key, 'turnaround'), res, samples=48)


def shot_attack(key):
    """The attack: lunging in to shriek, arms out, the tail streaming behind."""
    reset()
    where = collection('Attack')
    fig = figure(key, 'attack')
    instantiate(fig, where)
    ground(where)
    grass(where, center=(0.0, 0.5), seed=9)
    headboard(where, (1.3, 3.4, 0.0), turn=-0.3, tilt=0.06)
    sky()
    sun((-0.5, -0.8, 0.33))
    lo, hi = fig.bounds()
    target = Vector(((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2 - 0.15, (lo[2] + hi[2]) / 2 + 0.02))
    # From the front-right for D: its long reach is the left arm, which would hide the coal from the left.
    side = {'B': 0.22, 'D': -0.5}.get(key, 0.55)
    direction = Vector((side, -1.0, 0.12)).normalized()
    camera(target + direction * (4.3 if key == 'D' else 5.0), target, lens=45.0)
    render(shot_path(key, 'attack'), (1600, 1000))


TINT_NAMES = {'A': ('faded chambray', 'Sunday black', 'harvest rust'),
              'B': ('unbleached muslin', 'grey calico', 'mourning dye'),
              'C': ('linen', 'washed blue', 'rust')}
TINT_NAMES['D'] = TINT_NAMES['A']


def studio(where, floor=0xbdb4a4):
    ground(where, floor, plain=True)
    sky(0.8)


def shot_tints(key):
    """The three clothing tints (material instances in the game) side by side, three-quarter view."""
    reset()
    where = collection('Tints')
    fig = figure(key, 'idle')
    turn = math.radians(-32.0)
    lo, hi = fig.bounds(turn)
    span = hi[0] - lo[0]
    for k in range(3):
        x = (k - 1) * (span + 0.35) - (lo[0] + hi[0]) / 2
        instantiate(fig, where, tint=k, location=(x, 0.0, 0.0), turn=turn)
        label(f'{k + 1}: {TINT_NAMES[key][k]}', (x + (lo[0] + hi[0]) / 2, -0.4, 0.06), 0.085)
    label(TITLES[key] + ': clothing tints', (-1.5 * (span + 0.35), -1.6, 2.1), 0.1, align='LEFT')
    studio(where)
    sun((-0.45, -0.85, 0.42), strength=3.8)
    width = 3 * span + 2 * 0.35 + 0.5
    camera((0.0, -20.0, 1.05 + 20.0 * math.tan(math.radians(8.0))), (0.0, 0.0, 1.05), ortho=width)
    render(shot_path(key, 'tints'), (1800, int(1800 * 2.35 / width)), samples=48)


def text_material(color=0xf2ece0, strength=2.0):
    mat = bpy.data.materials.get('LabelGlow') or bpy.data.materials.new('LabelGlow')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Emission Strength'].default_value = strength
    return mat


def compose(paths, out_path):
    """Panels side by side (same height), as one PNG."""
    images = [bpy.data.images.load(p, check_existing=False) for p in paths]
    w = sum(i.size[0] for i in images)
    h = images[0].size[1]
    strip = np.zeros((h, w, 4), np.float32)
    x = 0
    for i in images:
        px = np.empty(i.size[0] * i.size[1] * 4, np.float32)
        i.pixels.foreach_get(px)
        strip[:, x:x + i.size[0]] = px.reshape(i.size[1], i.size[0], 4)
        strip[:, x:x + 2] = (0.08, 0.075, 0.07, 1.0) if x else strip[:, x:x + 2]
        x += i.size[0]
    out = bpy.data.images.new('Strip', w, h, alpha=False)
    out.pixels.foreach_set(strip.ravel())
    out.filepath_raw = out_path
    out.file_format = 'PNG'
    out.save()
    for i in images + [out]:
        bpy.data.images.remove(i)
    log(f'composed {os.path.relpath(out_path, lt.REPO)}')


def shot_coal(key):
    """The coal in the three rank colors, close on the chest from the front."""
    paths = []
    for name, color, strength in RANKS:
        reset()
        where = collection('Coal')
        fig = figure(key, 'idle')
        instantiate(fig, where, coal=(color, strength))
        studio(where, 0x9d9484)
        sun((-0.45, -0.8, 0.45), strength=3.6)
        c = Vector(fig.coal[0])
        cam = camera(c + Vector((0.22, -1.25, 0.12)), c + Vector((0.0, 0.0, 0.02)), lens=85.0)
        fwd = (c - cam.location).normalized()
        right = fwd.cross(Vector((0.0, 0.0, 1.0))).normalized()
        up = right.cross(fwd)
        tag = label(name, cam.location + fwd * 0.6 - up * 0.08, 0.016, cam.rotation_euler[:], color=0xf2ece0)
        tag.data.materials[0] = text_material()
        path = os.path.join(WORK, f'Unpaid_{key}_coal_{name}.png')
        render(path, (640, 640), samples=48)
        paths.append(path)
    compose(paths, shot_path(key, 'coal'))


def mannequin():
    """A plain 1.8 m figure for scale."""
    pieces = [ellipsoid((0.0, -0.01, 1.685), (0.075, 0.094, 0.108)),
              tube([(0.0, 0.0, 1.5), (0.0, -0.005, 1.6)], 0.052, segs=16),
              ellipsoid((0.0, 0.0, 1.36), (0.175, 0.11, 0.17)), ellipsoid((0.0, 0.005, 1.13), (0.15, 0.1, 0.15)),
              ellipsoid((0.0, 0.01, 0.95), (0.17, 0.11, 0.11))]
    for s in (1.0, -1.0):
        pieces.append(tube(spline([(s * 0.19, 0.0, 1.44), (s * 0.235, 0.01, 1.15), (s * 0.255, -0.02, 0.88)], 16),
                           np.linspace(0.047, 0.031, 16), segs=16))
        pieces.append(ellipsoid((s * 0.262, -0.025, 0.8), (0.026, 0.042, 0.07)))
        pieces.append(tube(spline([(s * 0.09, 0.01, 0.94), (s * 0.1, 0.0, 0.5), (s * 0.1, 0.02, 0.08)], 16),
                           np.linspace(0.075, 0.042, 16), segs=16))
        pieces.append(ellipsoid((s * 0.1, -0.05, 0.04), (0.045, 0.12, 0.04)))
    return union(pieces, 0.008, smooth=4)


def shot_compare():
    """The three side by side in the same light and at the same scale, beside a 1.8 m figure."""
    reset()
    where = collection('Compare')
    turn = math.radians(-30.0)
    items = []
    man = mannequin()
    R = rotation((0, 0, 1), turn)
    mv = man[0] @ R.T
    items.append((mv[:, 0].min(), mv[:, 0].max()))
    figs = [figure(k, 'idle') for k in ('A', 'B', 'C')]
    for fig in figs:
        lo, hi = fig.bounds(turn)
        items.append((lo[0], hi[0]))
    xs, total = lineup(items, gap=0.5)
    mobj = mesh_object('Figure', man[0], man[1], flat_material('Mannequin', 0x7b8590, 0.7), where)
    mobj.location = (xs[0], 0.0, 0.0)
    mobj.rotation_euler = (0.0, 0.0, turn)
    label('1.8 m figure', (xs[0] + (items[0][0] + items[0][1]) / 2, -0.75, 0.05), 0.085)
    for x, fig, (lo, hi) in zip(xs[1:], figs, items[1:]):
        instantiate(fig, where, location=(x, 0.0, 0.0), turn=turn)
        label(TITLES[fig.key], (x + (lo + hi) / 2, -0.75, 0.05), 0.085)
    studio(where)
    sun((-0.45, -0.85, 0.42), strength=3.8)
    width = total + 0.5
    camera((0.0, -20.0, 1.05 + 20.0 * math.tan(math.radians(8.0))), (0.0, 0.0, 1.05), ortho=width)
    render(os.path.join(OUT, 'Unpaid_compare.png'), (2400, int(2400 * 2.3 / width)), samples=64)


def box_piece(center, size, R=None):
    """A box (half sizes) as a closed piece, turned by R about its center."""
    c = np.asarray(center, float)
    V = np.array([[sx, sy, sz] for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)], float) * np.asarray(size, float)
    if R is not None:
        V = V @ np.asarray(R).T
    return V + c, [np.array([(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)])]


def town_gate(where, seed=8):
    """The road into Ransom's Rest at the town gate: a dirt track between dry grass and split-rail fences, the gate's
    posts and sign 26 m out, Main Street's false fronts behind it (simple blocks: context only)."""
    rng = np.random.default_rng(seed)
    ground(where, radius=250.0)
    # Dry grass either side of the track (the road itself stays bare).
    grass(where, center=(0.0, 14.0), radius=24.0, count=7000, seed=seed,
          keep=lambda x, y: abs(x - 0.15 * math.sin(0.2 * y)) > 2.6)
    wood =lt.material('WoodPlanks', name='GateWood', tint=0xb9a88e)
    dark = lt.material('WoodPlanks', name='TownWood', tint=0x8a7a66)
    pieces = []
    for s in (-1.0, 1.0):
        pieces.append(box_piece((s * 3.7, 26.0, 2.3), (0.13, 0.13, 2.3)))
        for k, y in enumerate(np.arange(3.5, 24.0, 2.8)):
            lean_r = rotation((0, 1, 0), rng.uniform(-0.08, 0.08)) @ rotation((1, 0, 0), rng.uniform(-0.05, 0.05))
            pieces.append(box_piece((s * 3.5, y, 0.62), (0.06, 0.06, 0.66), lean_r))
            if rng.uniform() > 0.15:
                for zr in (0.55, 1.0):
                    pieces.append(box_piece((s * 3.5, y + 1.4, zr + rng.uniform(-0.04, 0.04)), (0.035, 1.42, 0.04),
                                            rotation((1, 0, 0), rng.uniform(-0.04, 0.04))))
    pieces.append(box_piece((0.0, 26.0, 4.45), (4.2, 0.14, 0.12)))
    pieces.append(box_piece((0.0, 25.92, 3.8), (1.45, 0.03, 0.3)))
    for s in (-1.0, 1.0):
        pieces.append(box_piece((s * 1.1, 25.95, 4.15), (0.012, 0.012, 0.22)))
    V, F = merge(pieces)
    obj = mesh_object('Gate', V, F, wood, where, uvs=np.column_stack([V[:, 0] + V[:, 1], V[:, 2]]) * 0.6)
    obj.data.polygons.foreach_set('use_smooth', np.zeros(len(obj.data.polygons), dtype=bool))
    sign = label("RANSOM'S REST", (0.0, 25.87, 3.72), 0.26, color=0x2a2420)
    sign.data.extrude = 0.004
    # Main Street beyond the gate: false fronts, a stepped top on each.
    fronts = []
    for x, w, h, y in ((-17.0, 4.5, 5.4, 70.0), (-10.5, 4.8, 6.6, 73.0), (9.0, 4.6, 7.2, 72.0), (15.5, 5.0, 5.6, 69.0),
                       (22.0, 4.2, 5.0, 74.0)):
        fronts.append(box_piece((x, y + 4.0, h / 2), (w / 2, 4.0, h / 2)))
        fronts.append(box_piece((x, y, h + 0.55), (w / 2 * 0.8, 0.12, 0.55)))
        fronts.append(box_piece((x, y - 1.2, 2.9), (w / 2, 1.1, 0.08)))       # the boardwalk awning
    V, F = merge(fronts)
    obj = mesh_object('Town', V, F, dark, where, uvs=np.column_stack([V[:, 0] + V[:, 1], V[:, 2]]) * 0.4)
    obj.data.polygons.foreach_set('use_smooth', np.zeros(len(obj.data.polygons), dtype=bool))


def shot_group(key):
    """Four of them drifting in on the player at the town gate, 10-20 m out, in the three tints; one is Restless (Main 3:
    four at the gate, one of them Restless). Eye height, a shooter's field of view."""
    reset()
    where = collection('Group')
    town_gate(where)
    cam_xy = np.array([0.4, -0.5])
    group = ((-1.9, 10.5, 'attack', 0, RANKS[0]), (1.7, 13.0, 'drift', 1, RANKS[1]), (-0.4, 16.5, 'idle', 2, RANKS[0]),
             (2.9, 19.0, 'drift', 0, RANKS[0]))
    for x, y, pose, tint, rank in group:
        fig = figure(key, pose)
        d = cam_xy - np.array([x, y])
        turn = math.atan2(d[0], -d[1]) + math.radians(np.random.default_rng(int(y * 10)).uniform(-10, 10))
        instantiate(fig, where, tint=tint, coal=rank[1:], location=(x, y, 0.0), turn=turn)
    sky()
    sun((-0.62, -0.62, 0.3), strength=4.2)
    camera((cam_xy[0], cam_xy[1], 1.65), (0.0, 16.0, 1.25), lens=22.0)
    render(shot_path(key, 'group'), (1920, 1080), samples=64)


def shot_debug(key, pose='idle', what='head'):
    """A close look for checking the model (not one of the deliverables), into Intermediate/UnpaidConcepts."""
    reset()
    where = collection('Debug')
    fig = figure(key, pose)
    instantiate(fig, where)
    studio(where, 0x9d9484)
    sun((-0.45, -0.8, 0.45), strength=3.6)
    lo, hi = fig.bounds()
    if what == 'head':
        top = np.vstack([V for part, _ in fig.parts if part.name in ('Head', 'Hood', 'Face') for V in part.V])
        c = Vector(top.mean(0))
        camera(c + Vector((0.35, -1.0, 0.05)).normalized() * 0.9, c, lens=85.0)
    elif what == 'back':
        c = Vector(fig.coal[0]) + Vector((0.0, 0.1, 0.12))
        camera(c + Vector((0.5, 1.0, 0.3)).normalized() * 1.3, c, lens=60.0)
    else:
        c = Vector(fig.coal[0])
        camera(c + Vector((0.6, -1.0, 0.15)).normalized() * 1.9, c, lens=50.0)
    render(os.path.join(WORK, f'debug_{key}_{pose}_{what}.png'), (900, 900), samples=32)


SHOT_FUNCS = {'hero': shot_hero, 'turn': shot_turnaround, 'attack': shot_attack, 'tints': shot_tints,
              'coal': shot_coal, 'compare': shot_compare, 'group': shot_group}


def main():
    keys = [a for a in ARGV if a in OPTIONS] or [k for k in OPTIONS if k in FIGURES]
    shots = [a for a in ARGV if a in SHOTS] or list(SHOTS)
    if '--preview' not in ARGV and not lt.want_preview():
        for key in keys:
            figure(key, 'idle')
            figure(key, 'attack')
        return
    if 'debug' in ARGV:
        for key in keys:
            for pose in ('idle', 'attack'):
                shot_debug(key, pose, 'head')
                shot_debug(key, pose, 'body')
            shot_debug(key, 'idle', 'back')
        if not [a for a in ARGV if a in SHOTS]:
            return
    for key in keys:
        for shot in shots:
            # The comparison is the first round's (A, B, C); the town-gate group is the pick's (D).
            if shot == 'compare' or (shot == 'group' and key != 'D'):
                continue
            SHOT_FUNCS[shot](key)
    if 'compare' in shots and [k for k in keys if k in 'ABC']:
        SHOT_FUNCS['compare']()


# Run as a script (artrun.ps1); imported (Art/Models/Creatures/Unpaid.py's previews borrow its stage), it does nothing.
if __name__ == '__main__':
    main()
