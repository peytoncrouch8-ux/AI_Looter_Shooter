"""Mesh geometry for the tutorial island (numpy, plus Blender's mathutils for the triangulation). It returns plain
arrays in layout meters (x north, y east, z up); Art/Models/Terrain/TutorialIsland.py turns them into Blender objects.

- top_surface(): the walkable top as one triangulation: vertices packed where the ground bends (cliff lips, road
  shoulders, the rim) and spread about a meter apart elsewhere, sized to a triangle budget.
- terrain_ao(): ambient occlusion from the height raster (valleys, cliff feet and the creek's insides go dark).
- underside(): the rock below the rim: a wall under the top's edge, a tapering mass and hanging spires.
- water(): the pond and the creek's surface.
"""
import math

import numpy as np
from mathutils import Vector
from mathutils.geometry import delaunay_2d_cdt

from island_math import MAP_HALF, Grid, blur, gradient_noise, resize, resample, sample, points_in_polygon

LEVELS = (1.2, 0.6, 0.3)  # quadtree cell sizes (m): the spacing of the top's vertices


def _spacing(island, eps):
    """Target vertex spacing (m) per 10 cm cell for a height error eps (m): a linear triangle of size s over ground
    of curvature k misses it by about k s^2 / 8."""
    spacing = np.clip(np.sqrt(8.0 * eps / np.maximum(island._curvature, 1e-6)), LEVELS[-1], LEVELS[0])
    # Waterlines stay crisp: where the water meets a gentle shore, a centimeter of height moves it a hand's width.
    return np.where(island._waterline, np.minimum(spacing, LEVELS[-1]), spacing).astype(np.float32)


def _curvature(island):
    """|Hessian| of the heights, measured over 20 cm and blurred a little (on the 10 cm raster)."""
    h = resize(island.h, 1024)
    px = 2.0 * MAP_HALF / 1024
    gx, gy = np.gradient(h, px)
    gxx, gxy = np.gradient(gx, px)
    _, gyy = np.gradient(gy, px)
    k = np.sqrt(gxx * gxx + 2.0 * gxy * gxy + gyy * gyy)
    return resize(blur(k, 2), island.grid.n)


def _quadtree(spacing, edge):
    """Jittered points on the quadtree leaves: a cell is a leaf when the spacing wanted anywhere in it is at least
    its size."""
    n = spacing.shape[0]
    px = 2.0 * MAP_HALF / n
    steps = [int(round(c / px)) for c in LEVELS]  # 12, 6, 3 cells
    size = int(math.ceil(n / steps[0]) * steps[0])
    padded = np.pad(spacing, ((0, size - n), (0, size - n)), mode='edge')
    rng = np.random.default_rng(5)
    points = []
    taken = np.zeros((size // steps[-1],) * 2, dtype=bool)  # finest-level cells already covered
    for level, (cell, step) in enumerate(zip(LEVELS, steps)):
        m = size // step
        pooled = padded.reshape(m, step, m, step).min(axis=(1, 3))
        ratio = step // steps[-1]
        covered = taken.reshape(m, ratio, m, ratio).any(axis=(1, 3))
        leaf = ~covered & ((pooled >= cell - 1e-4) | (level == len(LEVELS) - 1))
        ii, jj = np.nonzero(leaf)
        jitter = rng.uniform(-0.18, 0.18, (len(ii), 2)) * cell
        x = -MAP_HALF + (ii + 0.5) * cell + jitter[:, 0]
        y = -MAP_HALF + (jj + 0.5) * cell + jitter[:, 1]
        # Keep clear of the rim (the boundary has its own points).
        keep = sample(edge, x, y) > 0.45 * cell
        points.append(np.column_stack([x[keep], y[keep]]))
        taken |= np.repeat(np.repeat(leaf, ratio, axis=0), ratio, axis=1)
    return np.vstack(points)


def waterline_points(island, surface, step=0.25):
    """Points on the waterlines (where the ground meets the water surface), about step meters apart: marching squares
    on the height raster, one crossing kept per step-sized cell."""
    level = extend_nan(surface, 6)
    f = island.h - np.nan_to_num(level, nan=1e3)
    c = island.grid.c
    points = []
    for axis in (0, 1):
        a = f[:-1, :] if axis == 0 else f[:, :-1]
        b = f[1:, :] if axis == 0 else f[:, 1:]
        ii, jj = np.nonzero((np.sign(a) != np.sign(b)) & (np.abs(a) < 5.0) & (np.abs(b) < 5.0))
        t = a[ii, jj] / (a[ii, jj] - b[ii, jj])
        x = c[ii] + (t * island.grid.px if axis == 0 else 0.0)
        y = c[jj] + (t * island.grid.px if axis == 1 else 0.0)
        points.append(np.column_stack([x, y]))
    pts = np.vstack(points)
    pts = pts[sample(island.edge, pts[:, 0], pts[:, 1]) > 1.0]
    _, first = np.unique(np.floor(pts / step).astype(np.int64), axis=0, return_index=True)
    return pts[np.sort(first)]


def _cell_keys(points, cell):
    """One integer per cell of a square grid (cell meters) that each point falls in."""
    cells = np.floor(points / cell).astype(np.int64)
    return cells[:, 0] * 100003 + cells[:, 1]


def top_surface(island, max_triangles=118000, boundary_step=0.7, log=print):
    """The top's vertices (N, 3) and triangles (T, 3), counter-clockwise seen from above in Blender's axes, and the
    boundary loop's vertex indices (in order around the island)."""
    island._curvature = _curvature(island)
    surface = island.water_surface()
    gap = island.h - np.nan_to_num(surface, nan=-100.0)
    island._waterline = np.isfinite(surface) & (gap > -0.2) & (gap < 0.4)
    boundary = resample(island.outline, boundary_step, closed=True)
    shore = waterline_points(island, surface)
    # Pick the height error that fills the budget (triangles ~ 2 x interior points + boundary points).
    lo, hi = math.log(1e-4), math.log(1.0)
    for _ in range(14):
        mid = 0.5 * (lo + hi)
        count = len(_quadtree(_spacing(island, math.exp(mid)), island.edge))
        if 2 * (count + len(shore)) + len(boundary) > max_triangles:
            lo = mid
        else:
            hi = mid
    eps = math.exp(hi)
    interior = _quadtree(_spacing(island, eps), island.edge)
    # Vertices exactly on the waterlines, so the water meets the shore along a smooth line (on a gentle shore a
    # centimeter between triangles moves the waterline a hand's width). Interior points too close to them go.
    if len(shore):
        cell = 0.2
        near = set()
        for di in (-1, 0, 1):
            for dj in (-1, 0, 1):
                near.update(_cell_keys(shore + np.array([di * cell, dj * cell]), cell).tolist())
        interior = interior[~np.isin(_cell_keys(interior, cell), np.array(sorted(near)))]
        interior = np.vstack([interior, shore])
    log(f'terrain: top surface, height tolerance {eps * 100:.1f} cm, {len(interior)} interior points '
        f'({len(shore)} on waterlines)')

    # Triangulate in Blender's plane (x_b = -y, y_b = -x), the boundary as the constraint.
    pts2 = np.vstack([boundary, interior])
    bx, by = -pts2[:, 1], -pts2[:, 0]
    loop = list(range(len(boundary)))
    lx, ly = bx[:len(boundary)], by[:len(boundary)]
    if np.sum(lx * np.roll(ly, -1) - np.roll(lx, -1) * ly) < 0.0:  # the constraint loop must run counter-clockwise
        loop.reverse()
    verts = [Vector((float(a), float(b))) for a, b in zip(bx, by)]
    out_verts, _, out_faces, orig_verts, _, _ = delaunay_2d_cdt(verts, [], [loop], 1, 1e-6, True)
    coords = np.array([(v.x, v.y) for v in out_verts])
    tris = np.array([f for f in out_faces if len(f) == 3], dtype=np.int64)
    # Back to layout meters, and the boundary's vertex indices in the output.
    x, y = -coords[:, 1], -coords[:, 0]
    index_of = {}
    for out_index, originals in enumerate(orig_verts):
        for o in originals:
            index_of[o] = out_index
    ring = np.array([index_of[k] for k in range(len(boundary))], dtype=np.int64)
    z = island.height(x, y)
    return np.column_stack([x, y, z]), tris, ring


def vertex_normals(verts_b, tris):
    """Area-weighted vertex normals (Blender axes)."""
    a, b, c = verts_b[tris[:, 0]], verts_b[tris[:, 1]], verts_b[tris[:, 2]]
    face = np.cross(b - a, c - a)
    normals = np.zeros_like(verts_b)
    for k in range(3):
        np.add.at(normals, tris[:, k], face)
    return normals / np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-12)


def terrain_ao(island, n=1024, directions=16, reach=18.0):
    """Ambient occlusion per raster cell (1 open, 0 closed) from the heights: the average sine of the horizon over
    directions. Beyond the rim is open sky."""
    h = resize(island.h, n).astype(np.float64)
    inside = resize(island.edge, n) > -0.5
    h[~inside] = -500.0
    px = 2.0 * MAP_HALF / n
    distances = []
    r = px * 1.5
    while r < reach:
        distances.append(r)
        r *= 1.22
    pad = int(math.ceil(reach / px)) + 2
    hp = np.pad(h, pad, mode='constant', constant_values=-500.0)
    total = np.zeros_like(h)
    for d in range(directions):
        angle = 2.0 * math.pi * (d + 0.5) / directions
        best = np.full_like(h, -1.0)
        for r in distances:
            di, dj = int(round(r * math.cos(angle) / px)), int(round(r * math.sin(angle) / px))
            shifted = hp[pad + di:pad + di + n, pad + dj:pad + dj + n]
            rise = (shifted - h - 0.04) / r
            np.maximum(best, rise, out=best)
        total += np.clip(best / np.sqrt(1.0 + best * best), 0.0, 1.0)  # sin(atan(rise))
    ao = 1.0 - total / directions
    return np.clip(1.0 - 1.25 * (1.0 - ao), 0.3, 1.0).astype(np.float32)


# --- The underside ---

def _zipper(a_start, a_count, a_u, b_start, b_count, b_u):
    """Triangles joining two closed loops of vertices (indices start..start+count, parameter u in 0..1 around the
    loop, both running the same way)."""
    tris = []
    i = j = 0
    while i < a_count or j < b_count:
        ua = a_u[i % a_count] + (i // a_count)
        ub = b_u[j % b_count] + (j // b_count)
        a0, a1 = a_start + i % a_count, a_start + (i + 1) % a_count
        b0, b1 = b_start + j % b_count, b_start + (j + 1) % b_count
        next_a = a_u[(i + 1) % a_count] + ((i + 1) // a_count)
        next_b = b_u[(j + 1) % b_count] + ((j + 1) // b_count)
        if j >= b_count or (i < a_count and next_a <= next_b):
            tris.append((a0, a1, b0))
            i += 1
        else:
            tris.append((a0, b1, b0))
            j += 1
    return tris


def underside(island, ring_xyz, rim_drop, depth, sectors=4, seed=61):
    """The rock under the island. ring_xyz is the top's boundary (layout meters, in order). Returns a list of
    (vertices (N, 3), triangles, extra (N, 5)) per sector, with the spires hanging under each sector merged in. extra
    holds each vertex's place around its loop (0..1) and that loop's length in meters (for a cylindrical mapping that
    keeps the rock's strata level), then its normal in Blender's axes (from the whole mass, so sectors match)."""
    rng = np.random.default_rng(seed)
    k = len(ring_xyz)
    center = ring_xyz[:, :2].mean(axis=0)
    seg = np.linalg.norm(np.diff(np.vstack([ring_xyz[:, :2], ring_xyz[:1, :2]]), axis=0), axis=1)
    u_top = np.concatenate([[0.0], np.cumsum(seg)[:-1]]) / seg.sum()
    perimeter = seg.sum()
    outward = ring_xyz[:, :2] - center
    outward /= np.linalg.norm(outward, axis=1)[:, None]

    rows = []  # (u values, xyz, loop length) from the top down
    rows.append((u_top, ring_xyz.copy(), perimeter))
    # The rim wall: straight down under the edge, leaning in a little, rough.
    for frac, count in ((0.35, k), (0.7, k // 2), (1.0, k // 3)):
        u = np.linspace(0.0, 1.0, count, endpoint=False)
        base = _loop_at(ring_xyz, u_top, u)
        arc = u * perimeter
        z = base[:, 2] - rim_drop * frac * (1.0 + 0.12 * gradient_noise(arc / 7.0, frac * 3.0, seed))
        rough = 0.45 * gradient_noise(arc / 3.0, z / 2.5, seed + 1) + 0.35 * frac
        xy = base[:, :2] - _loop_at_dir(outward, u_top, u) * rough[:, None]
        rows.append((u, np.column_stack([xy, z]), perimeter))
    # The mass: a bulb under the rim that tapers into a keel, in lobes (some hanging lower), ribs running down.
    wall_bottom = rows[-1][1]
    profile_d = np.array([0.0, 7.0, 16.0, 27.0, 39.0, 50.0, 58.0, 63.0]) / 64.0
    profile_r = np.array([1.0, 0.95, 0.83, 0.64, 0.44, 0.26, 0.13, 0.05])
    m = k // 3
    below = depth - rim_drop
    u_base = np.linspace(0.0, 1.0, m, endpoint=False)
    mean_top = float(ring_xyz[:, 2].mean())
    for t in np.linspace(0.0, 1.0, 41)[1:]:
        r = float(np.interp(t, profile_d, profile_r))
        count = max(18, int(m * 0.8 * r ** 0.7))
        u = np.linspace(0.0, 1.0, count, endpoint=False)
        top = _loop_at(wall_bottom, u_base, u)
        arc = u * perimeter
        c = np.array([center[0], center[1]])
        xy = c + (top[:, :2] - c) * r
        z = top[:, 2] * (1.0 - t) + (mean_top - rim_drop) * t - below * t
        belly = math.sin(math.pi * min(t * 1.25, 1.0)) ** 0.7
        lobes = 13.0 * gradient_noise(arc / 45.0, t * 1.4, seed + 5) * belly
        ribs = 2.6 * gradient_noise(arc / 7.5, t * 1.1, seed + 2) + 1.6 * gradient_noise(arc / 3.2, t * 2.5, seed + 3)
        # Ledges: each 6 m layer of rock juts out at its foot, a little differently around the island.
        layer = (-z / 6.0 + 0.35 * gradient_noise(arc / 30.0, 0.5, seed + 6)) % 1.0
        ledges = 1.6 * layer ** 3
        push = ((lobes + ribs + ledges) * min(1.0, r * 2.2))[:, None] * _loop_at_dir(outward, u_top, u)
        z = z - 11.0 * np.maximum(gradient_noise(arc / 36.0 + 9.0, t * 1.5, seed + 4), 0.0) * t * (1.0 - t) * 2.0
        rows.append((u, np.column_stack([xy + push, z]), perimeter * r))

    # Stitch the rows, then cap the keel.
    verts, tris, starts, wrap = [], [], [], []
    count = 0
    for u, xyz, length in rows:
        starts.append(count)
        verts.append(xyz)
        wrap.append(np.column_stack([u, np.full(len(u), length)]))
        count += len(xyz)
    for r in range(len(rows) - 1):
        tris += _zipper(starts[r], len(rows[r][0]), rows[r][0], starts[r + 1], len(rows[r + 1][0]), rows[r + 1][0])
    last = rows[-1][1]
    tip = np.array([[last[:, 0].mean(), last[:, 1].mean(), last[:, 2].min() - 6.0]])
    verts.append(tip)
    wrap.append(np.array([[0.5, rows[-1][2]]]))
    tip_index = count
    n_last = len(last)
    tris += [(starts[-1] + i, starts[-1] + (i + 1) % n_last, tip_index) for i in range(n_last)]
    verts, wrap = np.vstack(verts), np.vstack(wrap)
    tris = np.array(tris, dtype=np.int64)
    tris = orient_outward(verts, tris, (center[0], center[1], mean_top - 0.3 * depth))

    # Split by sector (the angle of each triangle around the center), keeping each sector's own vertices.
    centroid = verts[tris].mean(axis=1)
    angle = (np.arctan2(centroid[:, 1] - center[1], centroid[:, 0] - center[0]) + math.pi) / (2.0 * math.pi)
    sector = np.minimum((angle * sectors).astype(int), sectors - 1)
    # The keel's cap (under every sector) goes with sector 0.
    sector[np.isin(tris, [tip_index]).any(axis=1)] = 0
    # Normals of the whole mass (Blender axes), so the sectors shade as one piece across their seams.
    extra = np.column_stack([wrap, vertex_normals(to_blender(verts), tris)])
    pieces = [_submesh(verts, tris[sector == s], extra) for s in range(sectors)]

    # Hanging spires under the mass, each merged into the sector above it.
    for sv, st, sw in _spires(verts, tris, rng, seed):
        sw = np.column_stack([sw, vertex_normals(to_blender(sv), st)])
        c = sv[:, :2].mean(axis=0)
        a = (math.atan2(c[1] - center[1], c[0] - center[0]) + math.pi) / (2.0 * math.pi)
        s = min(int(a * sectors), sectors - 1)
        pv, pt, pw = pieces[s]
        pieces[s] = (np.vstack([pv, sv]), np.vstack([pt, st + len(pv)]), np.vstack([pw, sw]))
    return pieces


def _loop_at(loop, u_loop, u):
    """Points of a closed loop (parameter u_loop) at parameters u (linear between points)."""
    uu = np.concatenate([u_loop, [1.0]])
    ext = np.vstack([loop, loop[:1]])
    return np.column_stack([np.interp(u, uu, ext[:, c]) for c in range(loop.shape[1])])


def _loop_at_dir(dirs, u_loop, u):
    d = _loop_at(dirs, u_loop, u)
    return d / np.maximum(np.linalg.norm(d, axis=1, keepdims=True), 1e-9)


def _submesh(verts, tris, extra):
    used, inverse = np.unique(tris.ravel(), return_inverse=True)
    return verts[used], inverse.reshape(-1, 3), extra[used]


def _spires(verts, tris, rng, seed, count=7):
    """Downward rock spires: rough cones rooted in the underside's lower half."""
    centroid = verts[tris].mean(axis=1)
    zmin, zmax = centroid[:, 2].min(), centroid[:, 2].max()
    band = (centroid[:, 2] < zmax - 0.35 * (zmax - zmin)) & (centroid[:, 2] > zmin + 0.15 * (zmax - zmin))
    candidates = centroid[band]
    chosen = []
    for _ in range(400):
        p = candidates[rng.integers(len(candidates))]
        if all(np.linalg.norm(p[:2] - q[:2]) > 16.0 for q in chosen):
            chosen.append(p)
        if len(chosen) == count:
            break
    out = []
    sides, rings = 10, 11
    for n, root in enumerate(chosen):
        length = rng.uniform(12.0, 26.0)
        width = rng.uniform(6.0, 10.0)
        lean = rng.normal(0.0, 0.1, 2)
        bend = rng.normal(0.0, 0.08, 2)
        v, t, w = [], [], []
        for r in range(rings):
            f = r / (rings - 1)
            # A fat root narrowing fast, then a drip; lumps where each rock layer ends.
            rad = width * ((1.0 - f) ** 1.8 * 0.85 + 0.15 * (1.0 - f)) * (1.0 + 0.18 * math.sin(f * 9.0 + n)) + 0.1
            z = root[2] + 5.0 - (length + 5.0) * f
            cx = root[0] + (lean[0] * f + bend[0] * f * f) * length
            cy = root[1] + (lean[1] * f + bend[1] * f * f) * length
            for s in range(sides):
                a = 2.0 * math.pi * s / sides
                wob = 1.0 + 0.35 * float(gradient_noise(a * 1.1 + n * 7.1, f * 2.6, seed + 9))
                v.append((cx + math.cos(a) * rad * wob, cy + math.sin(a) * rad * wob, z))
                w.append((s / sides, 2.0 * math.pi * width))
        for r in range(rings - 1):
            for s in range(sides):
                a0, a1 = r * sides + s, r * sides + (s + 1) % sides
                b0, b1 = a0 + sides, a1 + sides
                t += [(a0, b1, a1), (a0, b0, b1)]
        tip = len(v)
        v.append((cx, cy, root[2] + 5.0 - length - 1.2))
        w.append((0.5, 2.0 * math.pi * width))
        base = (rings - 1) * sides
        t += [(base + s, tip, base + (s + 1) % sides) for s in range(sides)]
        v, t = np.array(v), np.array(t, dtype=np.int64)
        # Judge the facing against the spire's own axis at mid-length.
        axis = (root[0] + lean[0] * length * 0.5, root[1] + lean[1] * length * 0.5, root[2] - length * 0.45)
        out.append((v, orient_outward(v, t, axis), np.array(w)))
    return out


# --- Water ---

def water(island, spacing=0.6):
    """The water surface (layout meters): the pond and the creek down to its lip, reaching a little under the banks.
    Returns vertices (N, 3) and triangles."""
    surface = island.water_surface()
    grid = island.grid
    wet = np.isfinite(surface) & (surface > island.h - 0.35) & island.inside
    # Points on a jittered lattice where it's wet, plus the lip across the creek at the rim.
    n = int(2.0 * MAP_HALF / spacing)
    lattice = Grid(n)
    x, y = lattice.mesh()
    rng = np.random.default_rng(9)
    x = x + rng.uniform(-0.12, 0.12, x.shape) * spacing
    y = y + rng.uniform(-0.12, 0.12, y.shape) * spacing
    keep = sample(wet.astype(np.float32), x, y) > 0.2
    pts = np.column_stack([x[keep], y[keep]])
    verts = [Vector((float(-b), float(-a))) for a, b in pts]
    out_verts, _, out_faces, _, _, _ = delaunay_2d_cdt(verts, [], [], 0, 1e-6, False)
    coords = np.array([(v.x, v.y) for v in out_verts])
    px, py = -coords[:, 1], -coords[:, 0]
    tris = np.array([f for f in out_faces if len(f) == 3], dtype=np.int64)
    # Drop long triangles bridging separate wet areas, and anything outside the island.
    a = np.column_stack([px, py])[tris]
    longest = np.max(np.linalg.norm(a - np.roll(a, 1, axis=1), axis=2), axis=1)
    cx, cy = a[:, :, 0].mean(axis=1), a[:, :, 1].mean(axis=1)
    inside = points_in_polygon(cx, cy, island.outline)
    tris = tris[(longest < spacing * 2.2) & inside]
    z = sample(extend_nan(surface, 20), px, py)
    used, inverse = np.unique(tris.ravel(), return_inverse=True)
    return np.column_stack([px, py, np.nan_to_num(z, nan=0.0)])[used], inverse.reshape(-1, 3)


def extend_nan(raster, steps):
    """A copy with the NaN cells next to real values filled from their neighbors, steps cells out (so a bilinear
    sample near the edge of the valid area never mixes in a NaN)."""
    out = raster.copy()
    for _ in range(steps):
        empty = np.isnan(out)
        if not empty.any():
            break
        padded = np.pad(out, 1, constant_values=np.nan)
        stack = np.stack([padded[:-2, 1:-1], padded[2:, 1:-1], padded[1:-1, :-2], padded[1:-1, 2:]])
        valid = ~np.isnan(stack)
        total = np.where(valid, stack, 0.0).sum(axis=0)
        count = valid.sum(axis=0)
        fill = np.where(count > 0, total / np.maximum(count, 1), np.nan)
        out[empty] = fill[empty]
    return out


def to_blender(verts):
    """Layout meters (x north, y east, z up) to Blender (x_b = -y, y_b = -x)."""
    return np.column_stack([-verts[:, 1], -verts[:, 0], verts[:, 2]])


def orient_outward(verts, tris, center):
    """Flips the triangles so that most face away from center (layout meters), judged in Blender's axes."""
    vb = to_blender(verts)
    cb = to_blender(np.asarray(center, dtype=np.float64).reshape(1, 3))[0]
    a, b, c = vb[tris[:, 0]], vb[tris[:, 1]], vb[tris[:, 2]]
    normal = np.cross(b - a, c - a)
    away = (a + b + c) / 3.0 - cb
    if np.sum(np.einsum('ij,ij->i', normal, away) > 0.0) < 0.5 * len(tris):
        tris = tris[:, [0, 2, 1]]
    return tris
