"""The Meadow Wolf from the user's Tripo model (Art/Models/Creatures/Tripo/MeadowWolf.glb, copied from
Art/Concepts/Creatures/Meadow Spider/Meadow Wolf.glb; the paid plan): reading its shape. Not a model: Tools\models.ps1
skips parts_*.py; Spider.py runs parts_spider_build.py (LOOTER_SPIDER_TRIPO, default on), which reads this.

The GLB is one watertight shell (14.9k vertices, 29.7k triangles, Tripo's units: 100 across the legs), its front -Y,
standing nose-down: the plane through its eight hips leans 23 degrees (its back legs off the ground, its abdomen high).
It has exactly eight legs (each a branch off the thick body, counted below), two chelicerae with their fangs, two short
palps, eight eyes (two big ovals over a row of four, two on top), a domed carapace and a big egg of an abdomen.
- Source: the mesh in creature axes (x forward, y left, z up; Tripo's units), its thickness at every vertex (a ray in
  along the normal), each triangle's UVs and Tripo's colour map (the eyes are found by its black: colour_at()).
- segment(): labels: the legs are the branches that reach more than BRANCH units (along the surface) off the thick body
  (thicker than THICK), grown back down to it; the chelicerae and palps the thin pieces left at the front.
  split_abdomen(): the abdomen, inside the egg fitted to the body's rear.
- level(): the turn that lays the hips' plane flat and squares the hips' left-right pairs across, then the scale and
  shift that put the hips on the rig's (least squares): Tripo's units to creature centimetres.
- leg_line(): a limb's centre line from its hip to its tip (slices along the surface from the tip, then re-centred on
  rays round each station).
- sample_shell(), fill_grid(): a round part's latitude-longitude grid of rays, its holes bridged.
Everything is numpy; surfaces are mathutils BVH trees over a label's own triangles."""
import heapq
import math
import os

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_textures as lt
import looter_tripo as tripo

GLB = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Tripo', 'MeadowWolf.glb')
THICK = 12.0          # Tripo units: thicker than this (a ray in along the normal) is the body
BRANCH = 10.0         # a leg reaches past this along the surface from the body ...
LEG_VERTICES = 300    # ... with more vertices than this (the palps reach 10-12 with a hundred)
RAYS = 16             # rays round a leg's station
# Labels.
LEGS = range(8)       # 0..7: the legs, numbered front to back, left then right (2 * pair + side, side 0 left)
CHEL_L, CHEL_R, PALP_L, PALP_R, CARAPACE, ABDOMEN = 10, 11, 12, 13, 21, 22


def log(message):
    print(f'SPIDER TRIPO: {message}', flush=True)


def _neighbours(V, F):
    edges = set()
    for a, b, c in F:
        for p, q in ((a, b), (b, c), (c, a)):
            edges.add((min(p, q), max(p, q)))
    nb = [[] for _ in range(len(V))]
    for a, b in sorted(edges):
        length = float(np.linalg.norm(V[a] - V[b]))
        nb[a].append((b, length))
        nb[b].append((a, length))
    return nb


def dijkstra(nb, sources, allowed):
    """Distances along the surface from sources [(vertex, label)] through allowed vertices, and the label each took."""
    n = len(nb)
    dist = np.full(n, np.inf)
    lab = -np.ones(n, int)
    heap = []
    for s, label in sources:
        dist[s], lab[s] = 0.0, label
        heap.append((0.0, int(s)))
    heapq.heapify(heap)
    while heap:
        d, v = heapq.heappop(heap)
        if d > dist[v]:
            continue
        for w, length in nb[v]:
            if allowed[w] and d + length < dist[w]:
                dist[w], lab[w] = d + length, lab[v]
                heapq.heappush(heap, (d + length, w))
    return dist, lab


def _components(nb, keep):
    comp = -np.ones(len(nb), int)
    count = 0
    for s in range(len(nb)):
        if not keep[s] or comp[s] >= 0:
            continue
        stack = [s]
        comp[s] = count
        while stack:
            v = stack.pop()
            for w, _ in nb[v]:
                if keep[w] and comp[w] < 0:
                    comp[w] = count
                    stack.append(w)
        count += 1
    return comp, count


def vertex_normals(V, F):
    N = np.zeros_like(V)
    n = np.cross(V[F[:, 1]] - V[F[:, 0]], V[F[:, 2]] - V[F[:, 0]])
    for k in range(3):
        np.add.at(N, F[:, k], n)
    return N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)


class Source:
    """The Tripo model in creature axes: V (n x 3), F (m x 3), N, thickness T, UV (m x 3 x 2), px (its colour map, sRGB,
    1024 px), nb (each vertex's neighbours and the edges' lengths)."""

    def __init__(self):
        obj, maps = tripo.load(GLB)
        me = obj.data
        G = np.empty(len(me.vertices) * 3)
        me.vertices.foreach_get('co', G)
        G = G.reshape(-1, 3)
        self.V = np.column_stack([-G[:, 1], G[:, 0], G[:, 2]])          # Blender (left +X, front -Y) to creature axes
        if any(p.loop_total != 3 for p in me.polygons):
            raise ValueError(f'{GLB}: expected triangles only')
        F = np.empty(len(me.loops), dtype=np.int64)
        me.loops.foreach_get('vertex_index', F)
        self.F = F.reshape(-1, 3)
        uv = np.empty(len(me.loops) * 2)
        me.uv_layers.active.data.foreach_get('uv', uv)
        uv = uv.reshape(-1, 2)
        self.UV = uv.reshape(-1, 3, 2)                                 # each triangle's corners' UVs
        self.px = tripo.pixels(maps['BC'], 1024)[..., :3]
        self.maps = maps            # Tripo's colour, roughness-metal and normal maps (parts_spider_look.py reads them)
        self.N = vertex_normals(self.V, self.F)
        tree = BVHTree.FromPolygons(self.V.tolist(), self.F.tolist(), all_triangles=True)
        self.T = np.full(len(self.V), 60.0)
        for i, (p, n) in enumerate(zip(self.V, self.N)):
            hit = tree.ray_cast(Vector(p - n * 1e-3), Vector(-n), 60.0)
            if hit[0] is not None:
                self.T[i] = hit[3]
        self.nb = _neighbours(self.V, self.F)
        bpy.data.objects.remove(obj)
        log(f'{len(self.V)} vertices, {len(self.F)} triangles')


def segment(src):
    """Labels every vertex (see the module's labels); returns (labels, leg count found)."""
    V, nb = src.V, src.nb
    thick = src.T >= THICK
    geo, _ = dijkstra(nb, [(int(i), 0) for i in np.nonzero(thick)[0]], np.ones(len(V), bool))
    src.geo = geo
    comp, count = _components(nb, geo > BRANCH)
    sizes = np.bincount(comp[comp >= 0], minlength=count)
    branches = [k for k in range(count) if sizes[k] > LEG_VERTICES]
    sources = [(int(i), int(comp[i])) for i in np.nonzero(np.isin(comp, branches))[0]]
    _, grown = dijkstra(nb, sources, geo > 0.0)
    labels = np.full(len(V), CARAPACE)
    found = len(branches)
    # The legs, numbered by their hips (the vertices next to the body): front to back, left (+y) then right.
    hips = {}
    for k in branches:
        ring = [i for i in np.nonzero(grown == k)[0] if any(geo[w] == 0.0 for w, _ in nb[i])]
        hips[k] = V[ring].mean(0)
    order = {}
    for side, sign in ((0, 1.0), (1, -1.0)):
        mine = sorted([k for k in branches if np.sign(hips[k][1]) == sign], key=lambda k: -hips[k][0])
        for pair, k in enumerate(mine):
            order[k] = 2 * pair + side
    for k, leg in order.items():
        labels[grown == k] = leg
    # The thin pieces left at the front: the chelicerae (the bigger pair) and the palps.
    rest = (labels == CARAPACE) & ~thick
    pieces, n = _components(nb, rest)
    found_pieces = sorted([(int((pieces == k).sum()), k) for k in range(n)], reverse=True)
    big = [k for size, k in found_pieces if size > 150][:4]
    big.sort(key=lambda k: -(pieces == k).sum())
    for idx, k in enumerate(big):
        left = V[pieces == k][:, 1].mean() > 0.0
        labels[pieces == k] = (CHEL_L if left else CHEL_R) if idx < 2 else (PALP_L if left else PALP_R)
    log(f'{found} legs; chelicerae and palps of {[int((pieces == k).sum()) for k in big]} vertices')
    return labels, found


def _rotation_between(a, b):
    a, b = a / np.linalg.norm(a), b / np.linalg.norm(b)
    v = np.cross(a, b)
    c = float(a @ b)
    if np.linalg.norm(v) < 1e-9:
        return np.eye(3)
    K = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + K + K @ K * (1.0 / (1.0 + c))


def hip_points(src, labels):
    """Each leg's hip: the middle of its vertices that touch another label's (where it leaves the body)."""
    hips = {}
    for leg in LEGS:
        ring = [i for i in np.nonzero(labels == leg)[0] if any(labels[w] != leg for w, _ in src.nb[i])]
        hips[leg] = src.V[ring].mean(0)
    return hips


def level(hips, rig_hips):
    """The map from Tripo's creature axes to creature centimetres: a turn (the hips' plane flat, the pairs square
    across), then a uniform scale and a shift onto the rig's hips (least squares). Returns (R, s, t, residuals cm)."""
    H = np.array([hips[k] for k in LEGS])
    c = H.mean(0)
    _, _, vt = np.linalg.svd(H - c)
    n = vt[2] * (1.0 if vt[2][2] > 0 else -1.0)
    R = _rotation_between(n, np.array([0.0, 0.0, 1.0]))
    # Yaw: the left-right pairs' mean across-way onto +y.
    across = np.mean([(R @ (hips[2 * p] - hips[2 * p + 1])) for p in range(4)], axis=0)
    yaw = math.atan2(across[0], across[1])
    cz, sz = math.cos(yaw), math.sin(yaw)
    R = np.array([[cz, -sz, 0.0], [sz, cz, 0.0], [0.0, 0.0, 1.0]]) @ R
    P = (R @ H.T).T
    Q = np.array([rig_hips[k] for k in LEGS])
    pm, qm = P.mean(0), Q.mean(0)
    s = float(((P - pm) * (Q - qm)).sum() / ((P - pm) ** 2).sum())
    t = qm - s * pm
    res = np.linalg.norm(s * P + t - Q, axis=1)
    tilt = math.degrees(math.acos(min(1.0, n[2])))
    pitch, roll = math.degrees(math.atan2(n[0], n[2])), math.degrees(math.atan2(n[1], n[2]))
    log(f'levelled: the hips leaned {tilt:.1f} deg (pitch {pitch:.1f}, roll {roll:.1f}), yaw {math.degrees(yaw):.1f}; '
        f'scale {s:.3f} cm per unit; hip residuals {np.round(res, 1).tolist()} cm')
    return R, s, t, res


def surface(V, F, mask):
    """A BVH over the triangles whose corners all carry mask."""
    tris = F[mask[F].all(1)]
    return BVHTree.FromPolygons(V.tolist(), tris.tolist(), all_triangles=True)


def frame_at(tangent, up):
    """(tangent, top, side): top is up made square to the tangent."""
    t = tangent / np.linalg.norm(tangent)
    top = up - t * (up @ t)
    if np.linalg.norm(top) < 1e-6:
        top = np.array([1.0, 0.0, 0.0]) - t * t[0]
    top /= np.linalg.norm(top)
    return t, top, np.cross(t, top)


def ring_radii(tree, centre, t, top, side, reach, rays=RAYS):
    """Radii round a station (rays from centre square to t, from top round past side); nan where a ray misses."""
    out = np.full(rays, np.nan)
    for j in range(rays):
        a = 2.0 * math.pi * j / rays
        d = top * math.cos(a) + side * math.sin(a)
        hit = tree.ray_cast(Vector(centre), Vector(d), reach)
        if hit[0] is not None:
            out[j] = hit[3]
    return out


def leg_line(src, V, labels, leg, scale, step=3.0, tree=None):
    """A leg's centre line from its hip to its tip (V: the vertices levelled and scaled to cm; scale: cm per Tripo
    unit): stations every step cm from slices along the surface from the tip (the leg's vertex farthest from the
    body), smoothed, then each re-centred on its ring of rays (tree: the leg's own surface). Returns the points, hip
    first."""
    members = np.nonzero(labels == leg)[0]
    tip = members[np.argmax(src.geo[members])]
    d, _ = dijkstra(src.nb, [(int(tip), 0)], labels == leg)
    dm = d[members] * scale
    pts = []
    for a in np.arange(0.0, dm.max() + step, step):
        sel = members[(dm >= a) & (dm < a + step)]
        if len(sel) >= 4:
            pts.append(V[sel].mean(0))
    P = np.array(pts)[::-1]
    P = _resample(_smooth_line(P, 2), step)
    if tree is not None:
        for _ in range(2):
            T = tangents(P)
            Q = P.copy()
            for i in range(1, len(P) - 1):
                t, top, side = frame_at(T[i], np.array([0.0, 0.0, 1.0]))
                r = ring_radii(tree, P[i], t, top, side, 40.0)
                ok = ~np.isnan(r)
                if ok.sum() >= RAYS * 0.75:
                    a = 2.0 * np.pi * np.arange(RAYS) / RAYS
                    off = (r[ok, None] * (np.cos(a[ok])[:, None] * top + np.sin(a[ok])[:, None] * side)).mean(0)
                    Q[i] = P[i] + off
            P = _resample(_smooth_line(Q, 1), step)
    return P


def _smooth_line(P, passes):
    P = P.copy()
    for _ in range(passes):
        Q = P.copy()
        Q[1:-1] = (P[:-2] + 2.0 * P[1:-1] + P[2:]) / 4.0
        P = Q
    return P


def _resample(P, step):
    seg = np.linalg.norm(np.diff(P, axis=0), axis=1)
    s = np.concatenate([[0.0], np.cumsum(seg)])
    n = max(int(round(s[-1] / step)), 2)
    at = np.linspace(0.0, s[-1], n + 1)
    return np.column_stack([np.interp(at, s, P[:, k]) for k in range(3)])


def tangents(P):
    T = np.gradient(P, axis=0)
    return T / np.maximum(np.linalg.norm(T, axis=1, keepdims=True), 1e-9)


def colour_at(src, tri, point, V):
    """Tripo's colour (sRGB) at a point on triangle tri (V: the levelled vertices the point is in)."""
    A, B, C = V[src.F[tri]]
    v0, v1, v2 = B - A, C - A, point - A
    d00, d01, d11, d20, d21 = v0 @ v0, v0 @ v1, v1 @ v1, v2 @ v0, v2 @ v1
    den = d00 * d11 - d01 * d01
    b = (d11 * d20 - d01 * d21) / den if abs(den) > 1e-12 else 0.0
    c = (d00 * d21 - d01 * d20) / den if abs(den) > 1e-12 else 0.0
    uv = src.UV[tri, 0] * (1.0 - b - c) + src.UV[tri, 1] * b + src.UV[tri, 2] * c
    h, w = src.px.shape[:2]
    return src.px[min(h - 1, max(0, int((1.0 - uv[1] % 1.0) * h))), min(w - 1, max(0, int((uv[0] % 1.0) * w)))]


def fit_quadric(P):
    """An ellipsoid through points P (least squares, any turn): (centre, A) with (p - c)^T A (p - c) = 1 on it."""
    x, y, z = P[:, 0], P[:, 1], P[:, 2]
    D = np.column_stack([x * x, y * y, z * z, 2 * x * y, 2 * x * z, 2 * y * z, 2 * x, 2 * y, 2 * z])
    p, *_ = np.linalg.lstsq(D, np.ones(len(P)), rcond=None)
    A = np.array([[p[0], p[3], p[4]], [p[3], p[1], p[5]], [p[4], p[5], p[2]]])
    b = p[6:9]
    c = -np.linalg.solve(A, b)
    k = 1.0 + c @ A @ c
    return c, A / k


def split_abdomen(V, labels, behind=-40.0, grow=1.08, front=-8.0):
    """Moves the abdomen's vertices from CARAPACE to ABDOMEN: an ellipsoid fitted to the body behind x = behind (cm,
    all abdomen); every carapace vertex inside it grown by grow (and behind x = front) is the abdomen's. The egg's own
    overhang above the carapace's rear comes with it; the carapace's rear, lower and narrower, stays. Returns the
    fit (centre, A)."""
    body = labels == CARAPACE
    c, A = fit_quadric(V[body & (V[:, 0] < behind)])
    d = V - c
    f = np.einsum('ij,jk,ik->i', d, A, d)
    labels[body & (f < grow * grow) & (V[:, 0] < front)] = ABDOMEN
    radii = 1.0 / np.sqrt(np.linalg.eigvalsh(A))
    log(f'abdomen: centre {np.round(c, 1).tolist()} cm, radii {np.round(radii, 1).tolist()}, '
        f'{int((labels == ABDOMEN).sum())} vertices')
    return c, A


def sample_shell(V, F, mask, centre, fwd, up, radii, rings, segs):
    """A latitude-longitude grid of rays from centre against the faces carrying mask, the directions laid on an
    ellipsoid of radii (along fwd, left, up) so the grid comes out even. Returns (r: rings-1 x segs distances, nan
    where a ray missed; hit triangle indices, -1 where missed; the unit directions; the poles' distances; the frame).
    Ring i goes from the rear pole (i 0) to the front; seg j round from the belly (phi -pi) past the left side to the
    top (phi 0), as lc.ellipsoid lays its rings."""
    idx = np.nonzero(mask[F].all(1))[0]
    tree = BVHTree.FromPolygons(V.tolist(), F[idx].tolist(), all_triangles=True)
    fwd = np.asarray(fwd, float) / np.linalg.norm(fwd)
    up = np.asarray(up, float) - fwd * (np.asarray(up, float) @ fwd)
    up /= np.linalg.norm(up)
    left = np.cross(up, fwd)
    r = np.full((rings - 1, segs), np.nan)
    tri = -np.ones((rings - 1, segs), int)
    dirs = np.zeros((rings - 1, segs, 3))
    for i in range(1, rings):
        theta = math.pi * i / rings
        for j in range(segs):
            phi = -math.pi + 2.0 * math.pi * j / segs
            a, rr = -math.cos(theta), math.sin(theta)
            d = fwd * (a * radii[0]) + left * (rr * math.sin(phi) * radii[1]) + up * (rr * math.cos(phi) * radii[2])
            d /= np.linalg.norm(d)
            dirs[i - 1, j] = d
            hit = tree.ray_cast(Vector(centre), Vector(d), 1000.0)
            if hit[0] is not None:
                r[i - 1, j] = hit[3]
                tri[i - 1, j] = idx[hit[2]]
    poles = []
    for a in (-1.0, 1.0):
        hit = tree.ray_cast(Vector(centre), Vector(fwd * a), 1000.0)
        poles.append(hit[3] if hit[0] is not None else np.nan)
    return r, tri, dirs, poles, (fwd, left, up)


def fill_grid(r, passes=400):
    """Fills a grid's nan cells from their neighbours (rings up and down, segments round and wrapping), keeping the
    known cells: a smooth membrane across the holes."""
    r = r.copy()
    known = ~np.isnan(r)
    if not known.any():
        raise ValueError('fill_grid: no ray hit')
    r[~known] = np.nanmean(r)
    for _ in range(passes):
        up = np.vstack([r[:1], r[:-1]])
        down = np.vstack([r[1:], r[-1:]])
        avg = (up + down + np.roll(r, 1, 1) + np.roll(r, -1, 1)) * 0.25
        r = np.where(known, r, avg)
    return r
