"""Shape work on a mesh as arrays (V: n x 3, faces: index arrays), for looter_tripo_rig's AI-made creatures: thin cloth
thickened before a remesh (so it isn't broken into shards), QuadriFlow's odd needles pulled in, colour borders cut along
a field's contour, open loops capped, ambient occlusion per vertex. Import looter_tripo_rig (it re-exports it all)."""
import math

import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

from looter_tripo_rig import edges_of, neighbours, smooth_values, triangles, vertex_normals


def despike(V, faces, limit=1.5, passes=3):
    """Vertices standing far off their neighbours' middle (QuadriFlow's odd needle, a pinched pole) pulled back onto it:
    a vertex further off than limit times its mean edge is moved to the middle."""
    V = np.asarray(V, float).copy()
    nbr = neighbours(len(V), faces)
    moved = 0
    for _ in range(passes):
        for i, ns in enumerate(nbr):
            if len(ns) < 2:
                continue
            P = V[ns]
            mid = P.mean(0)
            edge = np.linalg.norm(P - V[i], axis=1).mean()
            if np.linalg.norm(V[i] - mid) > limit * max(np.linalg.norm(P - mid, axis=1).mean(), 1e-6) and \
                    np.linalg.norm(V[i] - mid) > 0.5 * edge:
                V[i] = mid
                moved += 1
    print(f'despike: {moved} vertices pulled in')
    return V



def thickness(V, faces, normals=None, reach=0.05):
    """How thick the mesh is under each vertex: the distance to the far side along minus its normal (reach if none)."""
    V = np.asarray(V, float)
    T, _ = triangles(faces)
    bvh = BVHTree.FromPolygons(V.tolist(), T.tolist(), all_triangles=True)
    N = vertex_normals(V, faces) if normals is None else normals
    out = np.full(len(V), reach)
    for i, (p, n) in enumerate(zip(V, N)):
        hit = bvh.ray_cast(Vector(p - n * 1e-4), Vector(-n), reach)
        if hit[0] is not None:
            out[i] = hit[3]
    return out


def thicken(V, faces, minimum, where=None, passes=2):
    """Thin parts (cloth strips, cords) pushed out along their normals until they are at least minimum thick, so a voxel
    remesh and QuadriFlow keep them whole instead of breaking them into shards. where: the vertices allowed to move."""
    V = np.asarray(V, float).copy()
    for _ in range(passes):
        N = vertex_normals(V, faces)
        t = thickness(V, faces, N, reach=minimum * 2.0)
        push = np.clip(minimum - t, 0.0, None) * 0.5
        push = smooth_values(push, faces, passes=2)
        if where is not None:
            push = push * where
        V = V + N * push[:, None]
    return V


def cut_field(V, faces, f, level=0.5, snap=0.22):
    """Cuts the faces along the contour where a per-vertex field crosses level (a clean colour border instead of a
    stair-step of whole faces). Where the contour passes close to a vertex (within snap of an edge) the vertex is slid
    onto it instead of a sliver being cut. Returns (V, faces, parents): parents[k] = (a, b, t) for each vertex k from
    len(V) on (its attributes are a's and b's mixed by t); moved vertices keep their own."""
    V = np.asarray(V, float).copy()
    f = np.asarray(f, float) - level
    E = edges_of(faces)
    on = np.zeros(len(V), dtype=bool)
    fa, fb = f[E[:, 0]], f[E[:, 1]]
    cross = fa * fb < 0.0
    t = np.where(cross, fa / np.where(cross, fa - fb, 1.0), 0.5)
    # Slide vertices onto the contour where it nearly meets them (nearest crossing first, each vertex once).
    near = np.nonzero(cross & ((t < snap) | (t > 1.0 - snap)))[0]
    for e in near[np.argsort(np.minimum(t[near], 1.0 - t[near]))]:
        a, b = E[e]
        v, w, s = (a, b, t[e]) if t[e] < 0.5 else (b, a, 1.0 - t[e])
        if on[v] or on[w]:
            continue
        V[v] = V[v] + s * (V[w] - V[v])
        on[v] = True
    f[on] = 0.0
    fa, fb = f[E[:, 0]], f[E[:, 1]]
    cross = (fa * fb < 0.0)
    new_index, parents, extra = {}, [], []
    for e in np.nonzero(cross)[0]:
        a, b = E[e]
        s = fa[e] / (fa[e] - fb[e])
        new_index[(a, b)] = len(V) + len(extra)
        extra.append(V[a] + s * (V[b] - V[a]))
        parents.append((int(a), int(b), float(s)))
    out = []
    for face in faces:
        ring, cuts = [], []
        n = len(face)
        for k in range(n):
            a, b = int(face[k]), int(face[(k + 1) % n])
            ring.append(a)
            if on[a]:
                cuts.append(len(ring) - 1)
            key = (min(a, b), max(a, b))
            if key in new_index:
                ring.append(new_index[key])
                cuts.append(len(ring) - 1)
        m = len(ring)
        if len(cuts) == 2 and (cuts[1] - cuts[0]) % m not in (1, m - 1):
            i, j = cuts
            out.append(np.array(ring[i:j + 1]))
            out.append(np.array(ring[j:] + ring[:i + 1]))
        else:
            out.append(np.array(ring))
    V = np.vstack([V] + ([np.array(extra)] if extra else []))
    return V, out, parents


def carry(values, parents, n_old):
    """A per-vertex array grown to the vertices cut_field added (each mixed from its two parents)."""
    values = np.asarray(values, float)
    if not parents:
        return values
    a = np.array([p[0] for p in parents])
    b = np.array([p[1] for p in parents])
    t = np.array([p[2] for p in parents]).reshape((-1,) + (1,) * (values.ndim - 1))
    return np.concatenate([values, values[a] * (1.0 - t) + values[b] * t])


# --- Cuts and caps ---

def boundary_loops(faces):
    """The open edge loops of a mesh, each an ordered vertex list."""
    count = {}
    for f in faces:
        for k in range(len(f)):
            a, b = int(f[k]), int(f[(k + 1) % len(f)])
            count[(a, b)] = count.get((a, b), 0) + 1
    nxt = {}
    for (a, b) in count:
        if (b, a) not in count:
            nxt[a] = b
    loops, seen = [], set()
    for start in list(nxt):
        if start in seen:
            continue
        loop, v = [], start
        while v not in seen and v in nxt:
            seen.add(v)
            loop.append(v)
            v = nxt[v]
        if len(loop) > 2:
            loops.append(loop)
    return loops


def cap(V, loop, up, height, rings=6):
    """New vertices and faces closing an open loop: rings drawn in toward its middle and raised along up by height at
    the middle (a dome; negative height dips). The loop's order makes the faces' winding (reverse it to flip them)."""
    V = np.asarray(V, float)
    P = V[loop]
    middle = P.mean(0)
    up = np.asarray(up, float) / np.linalg.norm(up)
    new_V, new_F = [], []
    base = len(V)
    prev = list(loop)
    for k in range(1, rings):
        f = k / rings
        ring = middle + (P - middle) * (1.0 - f) + up * height * math.sqrt(max(1.0 - (1.0 - f) ** 2, 0.0))
        ids = list(range(base + len(new_V), base + len(new_V) + len(P)))
        new_V.extend(ring)
        n = len(P)
        for i in range(n):
            new_F.append([prev[i], prev[(i + 1) % n], ids[(i + 1) % n], ids[i]])
        prev = ids
    top = base + len(new_V)
    new_V.append(middle + up * height)
    n = len(P)
    for i in range(n):
        new_F.append([prev[i], prev[(i + 1) % n], top])
    return np.array(new_V), new_F


# --- Occlusion ---

def ao(V, faces, normals=None, rays=24, reach=0.06, seed=1):
    """Ambient occlusion per vertex (1 open, 0 shut in) from rays over the hemisphere round each vertex's normal."""
    V = np.asarray(V, float)
    T, _ = triangles(faces)
    bvh = BVHTree.FromPolygons(V.tolist(), T.tolist(), all_triangles=True)
    if normals is None:
        normals = vertex_normals(V, faces)
    rng = np.random.default_rng(seed)
    D = rng.normal(size=(rays, 3))
    D /= np.linalg.norm(D, axis=1, keepdims=True)
    out = np.ones(len(V))
    for i, (p, n) in enumerate(zip(V, normals)):
        dirs = np.where((D @ n)[:, None] < 0.0, -D, D)
        start = Vector(p + n * 0.0015)
        hits = 0.0
        for d in dirs:
            hit = bvh.ray_cast(start, Vector(d), reach)
            if hit[0] is not None:
                hits += 1.0 - hit[3] / reach
        out[i] = 1.0 - hits / rays
    return out




def tidy_polygons(V, faces, flat=1e-10):
    """Polygons a contour cut left with a corner on a straight edge (or more than four corners) fanned from the corner
    that gives the fattest triangles, slivers of no area dropped: Unreal's own fan triangulation of them made triangles
    of no area (zero tangents). Clean quads and triangles stay as they are. Returns the faces."""
    V = np.asarray(V, float)
    out = []

    def area(a, b, c):
        return 0.5 * np.linalg.norm(np.cross(V[b] - V[a], V[c] - V[a]))

    for f in faces:
        f = [int(i) for i in f]
        n = len(f)
        if n == 3:
            if area(*f) > flat:
                out.append(np.array(f))
            continue
        fans = []
        for s in range(n):
            ring = f[s:] + f[:s]
            tris = [(ring[0], ring[k], ring[k + 1]) for k in range(1, n - 1)]
            fans.append((min(area(*t) for t in tris), tris))
        if n == 4 and min(fans[0][0], fans[1][0]) > flat * 100:
            out.append(np.array(f))
            continue
        best = max(fans, key=lambda t: t[0])[1]
        out.extend(np.array(t) for t in best if area(*t) > flat)
    return out
