"""What lies past a grounded area's core (step 3c; numpy, plus Blender's mathutils for the triangulations). area_model.py
makes Blender objects of them; area_region.py has the regional field R they're all cut from.

- ring(): the surround ring, R from the core square out to the ring's square (layout.json region.ring), with vertices
  2.5-10 m apart, closer where the ground bends and next to the core:
  - the upland: its inner edge reuses the core's seam vertices exactly (no T-junctions, no crack) and takes their
    normals from R's gradient, as the core does; its edge along the escarpment's lip is the canyon wall's top;
  - the canyon: the floor and the far wall, from just under the canyon wall's foot out.
  Split by angle into sectors: the upland's (the ridges' outer slopes: collision) and the canyon's (no collision:
  nothing past the lip has any).
- canyon_wall(): an open strip hanging from the whole lip (the core's lip vertices and the ring's) down past the canyon
  floor, in sectors along the lip: a slight batter, a ledge at the foot of each 6 m layer, level strata UVs; the
  generated part of the escarpment below the cliff kit's courses.
- backdrop(): unlit silhouettes on circles of about 3, 6 and 12 km (layout.json region.backdrop.layers), each a curtain
  from far below the horizon up to its skyline: the far ranges and hills, the plains beyond the canyon with their
  rolling relief and gaps, and the landforms on them (mesas, buttes, stepped and broken mesas, spires). Each vertex
  also carries its depth below the skyline, for the material's haze.
- seam(): the gap and the normal difference where the core meets the ring.
Every piece returns its vertices in layout meters (x north, y east, z up), triangles, and per-vertex extras.
"""
import math

import numpy as np
from mathutils import Vector
from mathutils.geometry import delaunay_2d_cdt

from area_math import Grid, blur, cells, gradient_noise, points_in_polygon, raster_size, resample, sample, smoothstep
from area_mesh import orient_outward, to_blender, vertex_normals

RING_LEVELS = (10.0, 5.0, 2.5)  # the ring's vertex spacings (m): a quadtree on a lattice of the finest
RING_FINE_NEAR = 14.0           # meters past the core square (and along the lip) where the ring keeps its finest spacing
RING_SEAM_NEAR = 3.0            # meters past the core square where it's half that again
RING_TRIANGLES = 38000
WALL_ROWS = ((0.03, 1.0), (0.08, 1.4), (0.15, 1.9), (0.24, 2.4), (0.35, 3.0), (0.48, 3.6), (0.62, 4.3), (0.76, 5.0),
             (0.89, 5.8), (1.0, 6.5))  # the canyon wall's rows: the share of the drop each reaches, its spacing (m)
WALL_BURY = 2.0                 # meters the wall reaches below the canyon floor
WALL_TOP_STEP = 2.5             # meters between the lip's vertices on the ring
BACKDROP_TRIANGLES = 6000


# --- Triangulation ---

def _cdt(points, edges, keep):
    """Constrained Delaunay triangulation of points (N, 2) with constraint edges (index pairs), keeping the triangles
    whose centroid passes keep(cx, cy). Returns the output points, the triangles and each input's output index."""
    verts = [Vector((float(-y), float(-x))) for x, y in points]  # Blender's plane: x_b = -y, y_b = -x
    out_verts, _, out_faces, orig_verts, _, _ = delaunay_2d_cdt(verts, [tuple(e) for e in edges], [], 0, 1e-6, True)
    coords = np.array([(v.x, v.y) for v in out_verts])
    pts = np.column_stack([-coords[:, 1], -coords[:, 0]])
    tris = np.array([f for f in out_faces if len(f) == 3], dtype=np.int64)
    index_of = np.full(len(points), -1, dtype=np.int64)
    for k, originals in enumerate(orig_verts):
        for o in originals:
            index_of[o] = k
    centroid = pts[tris].mean(axis=1)
    return pts, tris[keep(centroid[:, 0], centroid[:, 1])], index_of


def _chain_edges(start, count, closed=False):
    """Constraint edges along a run of consecutive point indices."""
    edges = [(start + i, start + i + 1) for i in range(count - 1)]
    if closed:
        edges.append((start + count - 1, start))
    return edges


def _lattice_quadtree(spacing_at, half, levels, keep, rng):
    """Points on a quadtree over the square of half side half: a cell of a level is a leaf when the spacing wanted
    anywhere in it (spacing_at, sampled at the finest lattice's cell corners and centers) is at least its size.
    keep(x, y, clearance) filters the points."""
    finest = levels[-1]
    m = int(math.ceil(2.0 * half / finest))
    m = int(math.ceil(m / 2 ** (len(levels) - 1))) * 2 ** (len(levels) - 1)
    edge = -m * finest * 0.5
    c = edge + (np.arange(m) + 0.5) * finest
    x, y = np.meshgrid(c, c, indexing='ij')
    wanted = spacing_at(x, y)
    for dx in (-0.45, 0.45):
        for dy in (-0.45, 0.45):
            wanted = np.minimum(wanted, spacing_at(x + dx * finest, y + dy * finest))
    taken = np.zeros((m, m), dtype=bool)
    points = []
    for level, size in enumerate(levels):
        ratio = int(round(size / finest))
        cells_ = m // ratio
        pooled = wanted.reshape(cells_, ratio, cells_, ratio).min(axis=(1, 3))
        covered = taken.reshape(cells_, ratio, cells_, ratio).any(axis=(1, 3))
        leaf = ~covered & ((pooled >= size - 1e-6) | (level == len(levels) - 1))
        ii, jj = np.nonzero(leaf)
        jitter = rng.uniform(-0.18, 0.18, (len(ii), 2)) * size
        px = edge + (ii + 0.5) * size + jitter[:, 0]
        py = edge + (jj + 0.5) * size + jitter[:, 1]
        ok = keep(px, py, 0.45 * size)
        points.append(np.column_stack([px[ok], py[ok]]))
        taken |= np.repeat(np.repeat(leaf, ratio, axis=0), ratio, axis=1)
    return np.vstack(points)


# --- The ring ---

def ring_raster(area, log=print):
    """R over the ring's square on a 1 m raster (area.ring_grid, area.ring_h), for the ring's spacing, its occlusion
    and its macro map."""
    region = area.region
    n = raster_size(2.0 * region.half, area.layout['region'].get('ring', {}).get('cell', 100) / 100.0, 4096)
    grid = Grid(n, region.half)
    area.ring_grid = grid
    area.ring_h = region.raster(grid)
    area.ring_dl = region.lip_distance(grid) if region.lip is not None else None
    log(f'ring: R on {n} x {n} cells ({grid.px:.2f} m)')
    return area.ring_h


def _curvature(h, px):
    gx, gy = np.gradient(h, px)
    gxx, gxy = np.gradient(gx, px)
    _, gyy = np.gradient(gy, px)
    return blur(np.sqrt(gxx * gxx + 2.0 * gxy * gxy + gyy * gyy), cells(2.0, px))


def _lip_runs(area):
    """The lip outside the core square, as the ring sees it: its south and north runs (each from the ring's edge to
    the core's corner where the lip meets the square, WALL_TOP_STEP apart), both running along the lip's direction."""
    region = area.region
    lip = region.lip
    half, outer = area.half, region.half
    inside_core = (np.abs(lip[:, 0]) < half) & (np.abs(lip[:, 1]) < half)
    in_ring = (np.abs(lip[:, 0]) < outer) & (np.abs(lip[:, 1]) < outer)
    idx = np.nonzero(inside_core)[0]
    i0, i1 = int(idx[0]), int(idx[-1])
    ring_idx = np.nonzero(in_ring)[0]
    r0, r1 = int(ring_idx[0]), int(ring_idx[-1])
    first = np.vstack([_crossing(lip[r0 - 1], lip[r0], outer), lip[r0:i0]])
    second = np.vstack([lip[i1 + 1:r1 + 1], _crossing(lip[r1 + 1], lip[r1], outer)])
    return first, second


def _crossing(outside, inside, h):
    """Where a segment from outside a square (half side h) to inside crosses its edge (bisection), on the edge."""
    lo, hi = 0.0, 1.0
    for _ in range(60):
        mid = 0.5 * (lo + hi)
        p = outside + (inside - outside) * mid
        if max(abs(p[0]), abs(p[1])) < h:
            hi = mid
        else:
            lo = mid
    p = outside + (inside - outside) * (0.5 * (lo + hi))
    axis = 0 if abs(abs(p[0]) - h) < abs(abs(p[1]) - h) else 1
    p[axis] = math.copysign(h, p[axis])
    return p[None, :]


def _square_run(a, b, h, step, upland):
    """The square's edge (half side h) from point a to point b (both on it), the way round that keeps to the
    upland side, every step meters at most, corners kept, both ends left out."""
    corners = np.array([[-h, -h], [h, -h], [h, h], [-h, h]], dtype=np.float64)
    perimeter = 8.0 * h

    def t_of(p):
        x, y = p
        if abs(y + h) < 1e-6:
            return x + h
        if abs(x - h) < 1e-6:
            return 2.0 * h + (y + h)
        if abs(y - h) < 1e-6:
            return 4.0 * h + (h - x)
        return 6.0 * h + (h - y)
    ta, tb = t_of(a), t_of(b)
    for direction in (1.0, -1.0):
        span = ((tb - ta) * direction) % perimeter
        between = sorted((((2.0 * h * k - ta) * direction) % perimeter, k) for k in range(4))
        path = [a] + [corners[k] for off, k in between if 1e-9 < off < span - 1e-9] + [b]
        mid = (path[0] + path[1]) * 0.5 if len(path) > 2 else (a + b) * 0.5
        probe = mid * 0.98  # a point just inside the square's edge, beside the run's first stretch
        if upland(np.array([probe[0]]), np.array([probe[1]]))[0]:
            break
    out = []
    for p, q in zip(path[:-1], path[1:]):
        count = max(1, int(math.ceil(np.linalg.norm(q - p) / step - 1e-9)))
        out.append(p + (q - p) * np.linspace(0.0, 1.0, count + 1)[:-1, None])
    return np.vstack(out)[1:]


def ring(area, core_xyz, core_ring, core_kinds, core_normals, core_ao, budget, log=print):
    """The surround ring: (upland, canyon, seam). The upland and the canyon are dicts of vertices (N, 3), triangles,
    normals (N, 3, Blender's axes) and occlusion (N,), the canyon None without an escarpment. seam holds the core's
    seam vertex ids (core_ids) and the upland's for each (in its "seam"), and the upland's lip vertices (south_ids and
    north_ids, along the lip). core_* are the core's top mesh: its vertices, its boundary loop's vertex indices, their
    kinds (0 lip, 1 seam, 2 both), and its normals (Blender axes) and occlusion per vertex."""
    region = area.region
    outer = region.half
    half = area.half
    spec = area.layout['region'].get('ring', {})
    levels = tuple(v / 100.0 for v in spec.get('levels', [lv * 100.0 for lv in RING_LEVELS]))
    has_lip = region.lip is not None
    rng = np.random.default_rng(7)
    wall_reach = region.wall - 1.5 if has_lip else 0.0  # the canyon's floor starts just under the wall's foot

    def dl_at(x, y):
        return sample(area.ring_dl, x, y, outer) if has_lip else np.full(np.shape(x), 1e3)

    def in_upland(x, y, clear=0.0):
        far = np.maximum(np.abs(x), np.abs(y))
        return (far > half + clear) & (far < outer - clear) & (dl_at(x, y) > clear)

    def in_canyon(x, y, clear=0.0):
        far = np.maximum(np.abs(x), np.abs(y))
        return (far < outer - clear) & (dl_at(x, y) < -wall_reach - clear)

    # Vertex spacing from R's curvature: finest next to the core and along the lip.
    k = _curvature(area.ring_h, area.ring_grid.px)

    # Right at the seam a finer level still, half the finest: the ring's triangles there follow the field as
    # closely as the core's do on the other side.
    quad_levels = levels + (levels[-1] * 0.5,)

    def spacing_for(eps):
        def at(x, y):
            s = np.clip(np.sqrt(8.0 * eps / np.maximum(sample(k, x, y, outer), 1e-6)), levels[-1], levels[0])
            dl = dl_at(x, y)
            # How far past the core square's edge, on the upland: the seam (the canyon there meets no core mesh).
            past = np.where(dl > 0.0, np.maximum(np.abs(x), np.abs(y)) - half, 1e3)
            fine = (past < RING_FINE_NEAR) | (np.abs(dl) < 6.0)
            return np.where(past < RING_SEAM_NEAR, quad_levels[-1], np.where(fine, levels[-1], s))
        return at

    def interior(eps):
        up = _lattice_quadtree(spacing_for(eps), outer, quad_levels, lambda x, y, c: in_upland(x, y, c), rng)
        down = _lattice_quadtree(spacing_for(eps), outer, quad_levels, lambda x, y, c: in_canyon(x, y, c), rng)
        return up, down

    # The upland's edge, one closed polygon: with a lip, from the ring's edge along the lip's south run to the core,
    # round the core's seam (its own vertices, the core's way round reversed), along the lip's north run to the
    # ring's edge, and back round the ring's edge on the upland side. Without one, the core's seam and the ring's edge
    # are two loops.
    loop_xy = core_xyz[core_ring][:, :2]
    if has_lip:
        south, north = _lip_runs(area)
        south, north = resample(south, WALL_TOP_STEP), resample(north, WALL_TOP_STEP)
        corner = np.nonzero(core_kinds == 2)[0]  # where the lip enters the core square, then where it leaves
        entry, exit_ = int(corner[0]), int(corner[-1])
        count = len(loop_xy)
        seam_ids = np.array([exit_] + [(exit_ + 1 + i) % count for i in range((entry - exit_ - 1) % count)]
                            + [entry])
        inner = np.vstack([south[:-1], loop_xy[seam_ids[::-1]], north[1:]])
        outer_run = _square_run(north[-1], south[0], outer, levels[0], lambda x, y: in_upland(x, y))
        polygon = np.vstack([inner, outer_run])
        seam_slots = len(south) - 1 + np.arange(len(seam_ids))[::-1]
        south_slots = np.arange(len(south) - 1)
        north_slots = np.arange(len(south) - 1 + len(seam_ids), len(inner))
        loops = [polygon]
    else:
        seam_ids = np.arange(len(loop_xy))
        seam_slots = np.arange(len(loop_xy))
        south_slots = north_slots = np.zeros(0, dtype=np.int64)
        corners = np.array([[-outer, -outer], [outer, -outer], [outer, outer], [-outer, outer], [-outer, -outer]])
        edge = np.vstack([a + (b - a) * np.linspace(0.0, 1.0, int(2.0 * outer / levels[0]) + 1)[:-1, None]
                          for a, b in zip(corners[:-1], corners[1:])])
        loops = [loop_xy, edge]
    boundary = np.vstack(loops)

    # Fill the budget: about two triangles per interior point, plus one per boundary point.
    canyon_edge = _offset_lip(area, wall_reach) if has_lip else None
    fixed = len(boundary) + (len(canyon_edge) + int(8.0 * outer / levels[0]) if has_lip else 0)
    lo, hi = math.log(1e-4), math.log(8.0)
    for _ in range(14):
        mid = 0.5 * (lo + hi)
        up, down = interior(math.exp(mid))
        if 2 * (len(up) + len(down)) + fixed > budget:
            lo = mid
        else:
            hi = mid
    eps = math.exp(hi)
    up, down = interior(eps)
    log(f'ring: height tolerance {eps * 100.0:.0f} cm, {len(up)} upland and {len(down)} canyon points inside')

    # The upland: the triangles inside its polygon (outside the core and inside the ring's edge, without a lip).
    edges, start = [], 0
    for loop in loops:
        edges += _chain_edges(start, len(loop), closed=True)
        start += len(loop)
    if has_lip:
        def keep(x, y):
            return points_in_polygon(x, y, polygon)
    else:
        def keep(x, y):
            far = np.maximum(np.abs(x), np.abs(y))
            return (far > half) & (far < outer)
    out_pts, tris, index_of = _cdt(np.vstack([boundary, up]), edges, keep)
    verts = np.column_stack([out_pts, region.height(out_pts[:, 0], out_pts[:, 1])])
    seam_out = index_of[seam_slots]
    verts[seam_out] = core_xyz[core_ring[seam_ids]]  # the core's own vertices, exactly
    upland = _finish(area, verts, tris, seam_out, core_normals[core_ring[seam_ids]], core_ao[core_ring[seam_ids]],
                     marks={'south': index_of[south_slots], 'north': index_of[north_slots]})

    # The canyon: its floor and far wall, from just under the canyon wall's foot to the ring's edge.
    canyon = None
    if has_lip:
        far_run = _square_run(canyon_edge[-1], canyon_edge[0], outer, levels[0], lambda x, y: dl_at(x, y) < 0.0)
        cpoly = np.vstack([canyon_edge, far_run])
        c_out, c_tris, _ = _cdt(np.vstack([cpoly, down]), _chain_edges(0, len(cpoly), closed=True),
                                lambda x, y: points_in_polygon(x, y, cpoly))
        cverts = np.column_stack([c_out, region.height(c_out[:, 0], c_out[:, 1])])
        canyon = _finish(area, cverts, c_tris, np.zeros(0, np.int64), None, None)
    seam = dict(core_ids=core_ring[seam_ids], south_ids=upland['marks']['south'],
                north_ids=upland['marks']['north'])
    return upland, canyon, seam


def _offset_lip(area, distance):
    """The lip moved distance meters toward the canyon (the canyon floor's inner edge), cut to the ring's square."""
    region = area.region
    lip = resample(region.lip_coarse, WALL_TOP_STEP * 2.0)
    d = np.gradient(lip, axis=0)
    d /= np.maximum(np.linalg.norm(d, axis=1, keepdims=True), 1e-9)
    out = np.column_stack([-d[:, 1], d[:, 0]]) * -region.lip_up  # the right normal is the side +1; the canyon is -up
    moved = lip + out * distance
    inside = np.nonzero((np.abs(moved[:, 0]) < region.half) & (np.abs(moved[:, 1]) < region.half))[0]
    a, b = int(inside[0]), int(inside[-1])
    return np.vstack([_crossing(moved[a - 1], moved[a], region.half), moved[a:b + 1],
                      _crossing(moved[b + 1], moved[b], region.half)])


def _finish(area, verts, tris, seam_out, seam_normals, seam_ao, marks=None):
    """Drops unused vertices, faces the triangles up, and works out normals (Blender axes; the seam's from the
    core) and occlusion per vertex. Returns a dict; marks (named index arrays) come back renumbered."""
    used, inverse = np.unique(tris.ravel(), return_inverse=True)
    remap = np.full(len(verts), -1, dtype=np.int64)
    remap[used] = np.arange(len(used))
    verts, tris = verts[used], inverse.reshape(-1, 3)
    vb = to_blender(verts)
    if np.mean(np.cross(vb[tris[:, 1]] - vb[tris[:, 0]], vb[tris[:, 2]] - vb[tris[:, 0]])[:, 2]) < 0.0:
        tris = tris[:, [0, 2, 1]]
    normals = vertex_normals(vb, tris)
    ao = sample(area.ring_ao, verts[:, 0], verts[:, 1], area.region.half)
    seam = remap[seam_out] if len(seam_out) else np.zeros(0, dtype=np.int64)
    geometric = normals[seam].copy() if len(seam) else np.zeros((0, 3))
    if seam_normals is not None and len(seam):
        normals[seam] = seam_normals
        ao[seam] = seam_ao
    marks = {name: remap[ids] for name, ids in (marks or {}).items()}
    return dict(verts=verts, tris=tris, normals=normals, ao=np.clip(ao, 0.0, 1.0), seam=seam, geometric=geometric,
                marks=marks)


def ring_ao(area, reach=18.0, directions=16):
    """Occlusion over the ring's raster, as terrain_ao() works it out for the core (the same reach, so the two agree
    at the seam), padded with the edge's heights."""
    h = area.ring_h.astype(np.float64)
    n = h.shape[0]
    px = area.ring_grid.px
    pad = int(math.ceil(reach / px)) + 2
    hp = np.pad(h, pad, mode='edge')
    distances = []
    r = px * 1.5
    while r < reach:
        distances.append(r)
        r *= 1.22
    total = np.zeros_like(h)
    for d in range(directions):
        angle = 2.0 * math.pi * (d + 0.5) / directions
        best = np.full_like(h, -1.0)
        for r in distances:
            di, dj = int(round(r * math.cos(angle) / px)), int(round(r * math.sin(angle) / px))
            rise = (hp[pad + di:pad + di + n, pad + dj:pad + dj + n] - h - 0.04) / r
            np.maximum(best, rise, out=best)
        total += np.clip(best / np.sqrt(1.0 + best * best), 0.0, 1.0)
    ao = 1.0 - total / directions
    area.ring_ao = np.clip(1.0 - 1.25 * (1.0 - ao), 0.3, 1.0).astype(np.float32)
    return area.ring_ao


def sectors(verts, tris, count, start_angle=None):
    """Splits triangles by the angle of their centroid around the origin into count sectors (equal angles, from
    start_angle, else over the angles they cover). Returns a list of triangle index arrays."""
    centroid = verts[tris].mean(axis=1)
    angle = np.degrees(np.arctan2(centroid[:, 1], centroid[:, 0]))  # 0 north, 90 east
    if start_angle is None:
        # Start at the widest gap between the triangles' angles, so a sector never straddles it.
        a = np.sort(angle)
        gaps = np.diff(np.concatenate([a, [a[0] + 360.0]]))
        g = int(np.argmax(gaps))
        start_angle = a[(g + 1) % len(a)]
        span = 360.0 - gaps[g]
    else:
        span = 360.0
    rel = (angle - start_angle) % 360.0
    index = np.minimum((rel / max(span, 1e-6) * count).astype(int), count - 1)
    return [np.nonzero(index == s)[0] for s in range(count)]


# --- The canyon wall ---

def canyon_wall(area, lip_xyz, sector_count=4, seed=71):
    """The strip under the lip: lip_xyz is the whole lip along the ring and the core, in order (its vertices shared
    with the core and the ring). Returns per sector (vertices (N, 3), triangles, extra (N, 5): place along the lip
    (0..1) and the lip's length (m), for the strata UVs, then the normal in Blender's axes, from the whole strip)."""
    region = area.region
    top = lip_xyz
    seg = np.linalg.norm(np.diff(top[:, :2], axis=0), axis=1)
    s_top = np.concatenate([[0.0], np.cumsum(seg)])
    length = s_top[-1]
    u_top = s_top / length
    d = np.gradient(top[:, :2], axis=0)
    d /= np.maximum(np.linalg.norm(d, axis=1, keepdims=True), 1e-9)
    out = np.column_stack([-d[:, 1], d[:, 0]]) * -region.lip_up  # toward the canyon
    rows = [(u_top, top.copy())]
    batter = region.wall / region.drop
    for frac, step in WALL_ROWS:
        count = max(2, int(round(length / step)) + 1)
        u = np.linspace(0.0, 1.0, count)
        base = np.column_stack([np.interp(u, u_top, top[:, c]) for c in range(3)])
        o = np.column_stack([np.interp(u, u_top, out[:, c]) for c in range(2)])
        o /= np.maximum(np.linalg.norm(o, axis=1, keepdims=True), 1e-9)
        arc = u * length
        # How far down the canyon's floor is here (measured past the wall's foot), plus the bit buried in it.
        foot = base[:, :2] + o * (region.wall + 1.0)
        bottom = region.height(foot[:, 0], foot[:, 1]) - WALL_BURY
        drop = np.maximum(base[:, 2] - bottom, 1.0)
        if frac < 1.0:
            f = frac * (1.0 + 0.1 * gradient_noise(arc / 9.0, frac * 4.0, seed))
            z = base[:, 2] - drop * np.minimum(f, 0.98)
        else:
            z = bottom
        z = np.minimum(z, base[:, 2] - 0.3)
        depth_below = base[:, 2] - z
        # Ledges: each 6 m layer juts out at its foot; ribs and gullies run down the face.
        layer = (-z / 6.0 + 0.35 * gradient_noise(arc / 30.0, 0.5, seed + 6)) % 1.0
        push = (batter * depth_below + 0.9 * layer ** 3 + 0.8 * gradient_noise(arc / 7.0, z / 9.0, seed + 2)
                + 0.5 * gradient_noise(arc / 2.7, z / 4.0, seed + 3)) * min(1.0, frac * 6.0)
        rows.append((u, np.column_stack([base[:, :2] + o * push[:, None], z])))
    verts, tris, starts, wrap = [], [], [], []
    count = 0
    for u, xyz in rows:
        starts.append(count)
        verts.append(xyz)
        wrap.append(np.column_stack([u, np.full(len(u), length)]))
        count += len(xyz)
    for r in range(len(rows) - 1):
        tris += _zipper_open(starts[r], rows[r][0], starts[r + 1], rows[r + 1][0])
    verts, wrap = np.vstack(verts), np.vstack(wrap)
    tris = np.array(tris, dtype=np.int64)
    # Face the canyon: judged against a point far back on the upland.
    mid = top[len(top) // 2]
    tris = orient_outward(verts, tris, (mid[0] - out[len(top) // 2][0] * 400.0,
                                        mid[1] - out[len(top) // 2][1] * 400.0, mid[2] - 30.0))
    extra = np.column_stack([wrap, vertex_normals(to_blender(verts), tris)])
    u_tri = wrap[tris][:, :, 0].mean(axis=1)
    index = np.minimum((u_tri * sector_count).astype(int), sector_count - 1)
    pieces = []
    for sct in range(sector_count):
        chosen = tris[index == sct]
        used, inverse = np.unique(chosen.ravel(), return_inverse=True)
        pieces.append((verts[used], inverse.reshape(-1, 3), extra[used]))
    return pieces


def _zipper_open(a_start, a_u, b_start, b_u):
    """Triangles joining two open rows of vertices (parameters a_u and b_u from 0 to 1, both running the same way)."""
    tris = []
    i = j = 0
    na, nb = len(a_u), len(b_u)
    while i < na - 1 or j < nb - 1:
        if j >= nb - 1 or (i < na - 1 and a_u[i + 1] <= b_u[j + 1]):
            tris.append((a_start + i, a_start + i + 1, b_start + j))
            i += 1
        else:
            tris.append((a_start + i, b_start + j + 1, b_start + j))
            j += 1
    return tris


# --- The backdrop ---

DIRECTIONS = ('north', 'east', 'south', 'west')
# The landforms standing on a backdrop layer's skyline besides today's mesa (a smooth step: _shape_profile), each a
# profile across its width: knots (the offset from its azimuth as a share of its width, -0.5 to 0.5; the height as a
# share of its own) joined by straight runs. The curtain gets a column at every knot, so it draws them exactly: a thin
# spire still reads, and no two are the same stamp. Their sides are steep but never sheer, and all but the mesa stand
# on a talus apron, as buttes and spires do.
BACKDROP_SHAPES = {
    # Narrow and tall: a flat cap a third of the base wide on cliffs, then the talus.
    'butte': ((-0.5, 0.0), (-0.36, 0.16), (-0.27, 0.42), (-0.215, 0.9), (-0.19, 0.985), (-0.16, 1.0), (0.16, 1.0),
              (0.19, 0.985), (0.215, 0.9), (0.27, 0.42), (0.36, 0.16), (0.5, 0.0)),
    # A mesa with a lower shelf (about half its height) on its far side: toward larger azimuths, or smaller with side -1.
    'stepped': ((-0.5, 0.0), (-0.42, 0.14), (-0.36, 0.4), (-0.325, 0.92), (-0.305, 1.0), (0.04, 1.0), (0.06, 0.95),
                (0.085, 0.6), (0.1, 0.56), (0.3, 0.53), (0.32, 0.49), (0.355, 0.24), (0.42, 0.09), (0.5, 0.0)),
    # A mesa whose top a notch splits into two blocks, the smaller one a little lower.
    'broken': ((-0.5, 0.0), (-0.42, 0.14), (-0.36, 0.4), (-0.325, 0.92), (-0.305, 1.0), (0.03, 1.0), (0.05, 0.94),
               (0.075, 0.66), (0.1, 0.6), (0.125, 0.64), (0.15, 0.93), (0.17, 0.985), (0.3, 0.965), (0.32, 0.9),
               (0.355, 0.38), (0.42, 0.13), (0.5, 0.0)),
    # A thin pinnacle tapering to a point off a talus cone, leaning a little toward its far side.
    'spire': ((-0.5, 0.0), (-0.3, 0.05), (-0.15, 0.17), (-0.085, 0.45), (-0.05, 0.8), (-0.028, 0.95), (-0.01, 1.0),
              (0.01, 0.99), (0.032, 0.93), (0.055, 0.76), (0.095, 0.42), (0.16, 0.16), (0.31, 0.04), (0.5, 0.0)),
}
MESA_SIDES = (0.35, 0.3875, 0.425, 0.4625, 0.5)  # where a mesa's smooth sides get columns (share of its width)
SHAPE_COVER = 0.2               # a landform keeps its outline where it stands this share of its height or more
NOTCH_SAMPLES = (0.0, 0.25, 0.45, 0.6, 0.72, 0.86, 1.0)  # and a notch's walls (share of its half width), both sides
RELIEF_WAVELENGTH = 110000      # cm of arc between a layer's relief swells, unless its relief gives a wavelength


def _eased(theta, values):
    """Per-direction values (north, east, south, west) at azimuths theta (degrees, 0 north, 90 east), eased between
    the four directions."""
    t = (np.asarray(theta) % 360.0) / 90.0
    k = np.floor(t).astype(int) % 4
    f = t - np.floor(t)
    f = f * f * (3.0 - 2.0 * f)
    nxt = (k + 1) % 4
    return values[k] + (values[nxt] - values[k]) * f


def _direction_range(theta, spec):
    """A layer's skyline range [low, high] (m) at azimuths theta, eased between the four directions' ranges."""
    lows = np.array([spec[n][0] for n in DIRECTIONS], dtype=np.float64) / 100.0
    highs = np.array([spec[n][1] for n in DIRECTIONS], dtype=np.float64) / 100.0
    return _eased(theta, lows), _eased(theta, highs)


def _backdrop_shapes(layer):
    """A layer's landforms (its "mesas"), as dicts: azimuth and width (degrees), height (m over the layer's low), kind,
    side (1, or -1 to mirror the kind) and tilt; plain marks today's three-number form."""
    shapes = []
    for entry in layer.get('mesas', []):
        if isinstance(entry, dict):
            azimuth, width, height = entry['azimuth'], entry['width'], entry['height']
            kind, side, tilt = entry.get('kind', 'mesa'), entry.get('side', 1), entry.get('tilt', 0.0)
        else:
            azimuth, width, height = entry[:3]
            kind, side, tilt = (entry[3] if len(entry) > 3 else 'mesa'), 1, 0.0
        if kind != 'mesa' and kind not in BACKDROP_SHAPES:
            raise ValueError(f"backdrop: a shape's kind is {kind!r}, not one of mesa, {', '.join(BACKDROP_SHAPES)}")
        shapes.append(dict(azimuth=azimuth, width=width, height=height / 100.0, kind=kind,
                           side=-1.0 if side < 0 else 1.0, tilt=float(tilt),
                           plain=not isinstance(entry, dict) and len(entry) == 3))
    return shapes


def _shape_profile(shape, off):
    """A landform's height as a share of its own (0 to 1) at offsets off (degrees from its azimuth)."""
    width = shape['width']
    if shape['kind'] == 'mesa':
        f = 1.0 - smoothstep(width * 0.35, width * 0.5, np.abs(off))  # today's mesa, to the bit
    else:
        knots = np.asarray(BACKDROP_SHAPES[shape['kind']])
        f = np.interp(off / width * shape['side'], knots[:, 0], knots[:, 1], left=0.0, right=0.0)
    if shape['tilt']:
        f = f * (1.0 + 2.0 * shape['tilt'] * np.clip(off / width, -0.5, 0.5))
    return f


def _shape_columns(shape):
    """The azimuths (degrees) where a landform's outline turns: its knots, or a mesa's smooth sides."""
    if shape['kind'] == 'mesa':
        s = np.concatenate([-np.array(MESA_SIDES[::-1]), MESA_SIDES])
    else:
        s = np.asarray(BACKDROP_SHAPES[shape['kind']])[:, 0] * shape['side']
    return shape['azimuth'] + s * shape['width']


def _notch_columns(notch):
    """The azimuths (degrees) of a notch's columns: its floor and both walls."""
    u = np.array(NOTCH_SAMPLES)
    return notch[0] + np.concatenate([-u[:0:-1], u]) * 0.5 * notch[1]


def _notch_cut(theta, notches):
    """How far (m) a layer's notches cut its skyline down at azimuths theta: each a soft U (a side canyon's gap), its
    floor rounded and its rims rolling over, steepest about two thirds of the way out. (A flat floor read as a box cut
    out of the skyline.) Overlapping notches don't add up: the deeper one wins."""
    cut = np.zeros(np.shape(theta))
    for azimuth, width, depth in notches:
        off = ((theta - azimuth + 180.0) % 360.0) - 180.0
        u = np.minimum(np.abs(off) / (0.5 * width), 1.0)
        cut = np.maximum(cut, depth / 100.0 * (1.0 - u * u) ** 1.5)
    return cut


def _relief(theta, layer, li, radius, seed):
    """A layer's rolling relief (m) at azimuths theta: low swells that hold level for a stretch and break softly to the
    next (smooth noise through a tanh), not peaks, about as high either way as the direction's amplitude, eased between
    the directions like the ranges; the swells are the relief's wavelength apart along the circle."""
    relief = layer['relief']
    amplitude = _eased(theta, np.array([relief.get(n, 0) for n in DIRECTIONS], dtype=np.float64) / 100.0)
    wave = relief.get('wavelength', RELIEF_WAVELENGTH) / 100.0
    a = np.radians(theta)
    u, v = np.cos(a) * radius / wave, np.sin(a) * radius / wave
    swell = (0.8 * gradient_noise(u + 47.0 * li, v, seed + li + 41)
             + 0.2 * gradient_noise(2.3 * u, 2.3 * v + 3.0, seed + li + 43))
    return amplitude * np.tanh(2.0 * swell)


def _backdrop_columns(segments, sector_count, shapes, notches):
    """A layer's column azimuths (degrees, 0 to 360) and the index each sector starts at (and the last's end). A layer
    of plain mesas only has them evenly spaced, as ever. Once a layer has a landform of a kind or a notch, every
    landform and notch also gets columns where its outline turns, and the even ones thin out to keep the count: those
    right beside a turn make way (no slivers), except where a sector starts."""
    if not notches and all(s['plain'] for s in shapes):
        return np.linspace(0.0, 360.0, segments + 1), [s * segments // sector_count for s in range(sector_count + 1)]
    turns = np.sort(np.concatenate([_shape_columns(s) for s in shapes] + [_notch_columns(n) for n in notches])
                    % 360.0)
    turns = turns[np.diff(np.concatenate([[-1.0], turns])) > 1e-4]  # overlapping outlines: one column each

    def merged(count):
        even = np.linspace(0.0, 360.0, count + 1)
        starts = np.zeros(count + 1, dtype=bool)
        starts[::count // sector_count] = True
        gap = np.min(np.abs(even[:, None] - turns[None, :]), axis=1, initial=1e9)
        kept = (gap > 0.3 * 360.0 / count) | starts
        clear = np.min(np.abs(turns[:, None] - even[starts][None, :]), axis=1) > 1e-4
        theta = np.concatenate([even[kept], turns[clear]])
        first = np.concatenate([starts[kept], np.zeros(int(clear.sum()), dtype=bool)])
        order = np.argsort(theta, kind='stable')
        return theta[order], list(np.nonzero(first[order])[0])
    count = max(sector_count, (segments - len(turns)) // sector_count * sector_count)
    theta, starts = merged(count)
    # The even columns that made way leave some of the count unspent: more even ones, while it lasts.
    while len(theta) - 1 < segments:
        more = merged(count + sector_count)
        if len(more[0]) - 1 > segments:
            break
        count += sector_count
        theta, starts = more
    return theta, starts


def backdrop(area, budget=None, seed=91):
    """The backdrop's layers: a list per layer of (layer index, sector, vertices (N, 3), triangles, extra (N, 3): the
    height share up the curtain, 0 at its foot and 1 on the skyline; the azimuth share round the circle; and the depth
    below the skyline in meters, 0 on it and the skyline less the base at the foot, for the material's haze).

    Each layer (layout.json region.backdrop.layers, lengths in cm) is a curtain at its distance from its base (or
    lower: see below) up to a skyline over azimuth (degrees, 0 north, 90 east), within each direction's [low, high]:
    - north, east, south, west: the skyline's range, eased between the directions; a ridgeline of a few octaves of
      noise (rougher on the higher ranges) runs within it.
    - mesas (optional): the landforms standing on it, each over the layer's low there: [azimuth, width, height] is
      today's flat-topped mesa; [azimuth, width, height, kind] or {"azimuth", "width", "height", "kind", "side",
      "tilt"} picks a kind: 'mesa', 'butte' (narrow, tall, steep), 'stepped' (a lower shelf on one side), 'broken' (a
      notch in its top) or 'spire' (a thin pinnacle), all but the mesa on a talus apron (BACKDROP_SHAPES); side -1
      mirrors it, and tilt (a share of its height) lifts its far side and drops its near one.
    - relief (optional): {"west": 1500, "east": 2000, "wavelength": 110000}: rolling swells, about that high either way
      per direction (eased; directions left out have none), the wavelength apart along the circle.
    - notches (optional): [[azimuth, width (degrees), depth], ...]: the skyline cut down in a soft U, a side canyon's
      gap.
    The relief and the notches shape the skyline around the landforms, not through them, easing in across their
    talus, and may take it under the range's low. A layer of plain mesas only keeps its columns evenly spaced, as it
    always was; one with kinds or notches also has columns at their corners (_backdrop_columns), within the same
    count. Each layer's share of the backdrop's triangles (its "triangles") makes two per column."""
    spec = area.layout['region'].get('backdrop', {})
    layers = spec.get('layers', [])
    if not layers:
        return []
    budget = budget or spec.get('triangles', BACKDROP_TRIANGLES)
    sector_count = spec.get('sectors', 4)
    per_layer = budget // len(layers)
    segments = max(sector_count, (per_layer // 2) // sector_count * sector_count)
    # Each curtain reaches below the steepest look past the ring's edge (from the core's highest ground to the edge's
    # lowest), so no view sees the gap between the ring and the backdrop.
    ring_h = area.ring_h
    edge_low = float(min(ring_h[0].min(), ring_h[-1].min(), ring_h[:, 0].min(), ring_h[:, -1].min()))
    steepest = math.atan2(float(np.max(area.h)) - edge_low, area.region.half - area.half) + math.radians(2.0)
    pieces = []
    for li, layer in enumerate(layers):
        radius = layer['distance'] / 100.0
        base = min(layer.get('base', -50000) / 100.0, -radius * math.tan(steepest))
        shapes = _backdrop_shapes(layer)
        notches = layer.get('notches', [])
        theta, starts = _backdrop_columns(segments, sector_count, shapes, notches)
        lo, hi = _direction_range(theta, layer)
        # The skyline: a ridgeline of a few octaves (rougher on the higher ranges) and the landforms standing on it,
        # then its relief and notches around them.
        a = np.radians(theta)
        ridge = (0.55 * gradient_noise(np.cos(a) * radius / 900.0 + 31.0 * li, np.sin(a) * radius / 900.0, seed + li)
                 + 0.3 * gradient_noise(np.cos(a) * radius / 300.0, np.sin(a) * radius / 300.0 + 7.0, seed + li + 9)
                 + 0.15 * gradient_noise(np.cos(a) * radius / 110.0, np.sin(a) * radius / 110.0, seed + li + 17))
        sky = lo + (hi - lo) * np.clip(0.5 + 0.8 * ridge, 0.0, 1.0)
        cover = np.zeros(len(theta))
        for shape in shapes:
            off = ((theta - shape['azimuth'] + 180.0) % 360.0) - 180.0
            f = _shape_profile(shape, off)
            sky = np.maximum(sky, lo + shape['height'] * f)
            cover = np.maximum(cover, np.minimum(f / SHAPE_COVER, 1.0))
        # The relief and the notches shape the skyline around the landforms, never through them: they ease in across
        # each one's talus. (Applied before, they took the skyline under the range's low, and every landform's foot,
        # at the low, lifted it back.)
        if layer.get('relief') or notches:
            shift = _relief(theta, layer, li, radius, seed) if layer.get('relief') else np.zeros(len(theta))
            if notches:
                shift = shift - _notch_cut(theta, notches)
            sky = sky + shift * (1.0 - cover)
        sky[-1] = sky[0]
        x, y = radius * np.cos(a), radius * np.sin(a)
        for sct in range(sector_count):
            idx = np.arange(starts[sct], starts[sct + 1] + 1)
            bottom = np.column_stack([x[idx], y[idx], np.full(len(idx), base)])
            top = np.column_stack([x[idx], y[idx], sky[idx]])
            verts = np.vstack([bottom, top])
            m = len(idx)
            tris = []
            for k in range(m - 1):
                tris += [(k, k + 1, m + k + 1), (k, m + k + 1, m + k)]
            tris = np.array(tris, dtype=np.int64)
            # Face the center: flip if the triangles face away from it.
            vb = to_blender(verts)
            normal = np.cross(vb[tris[:, 1]] - vb[tris[:, 0]], vb[tris[:, 2]] - vb[tris[:, 0]])
            toward = -vb[tris].mean(axis=1)
            toward[:, 2] = 0.0
            if np.sum(np.einsum('ij,ij->i', normal, toward) > 0.0) < 0.5 * len(tris):
                tris = tris[:, [0, 2, 1]]
            extra = np.column_stack([np.concatenate([np.zeros(m), np.ones(m)]),
                                     np.concatenate([theta[idx], theta[idx]]) / 360.0,
                                     np.concatenate([sky[idx] - base, np.zeros(m)])])
            pieces.append((li, sct, verts, tris, extra))
    return pieces


# --- The seam ---

def seam(core_xyz, core_tris, core_normals, core_geometric, seam, ring_piece, region):
    """The seam's numbers: the largest gap between the core's seam vertices and the ring's (mm), whether every seam
    edge is an edge of both meshes, and the largest normal differences (degrees): the meshes' own normals across the
    seam, and each side's geometric normal against R's gradient."""
    core_ids, ring_ids = seam['core_ids'], ring_piece['seam']
    gap = float(np.max(np.linalg.norm(core_xyz[core_ids] - ring_piece['verts'][ring_ids], axis=1))) * 1000.0

    def edge_set(tris):
        e = np.vstack([tris[:, [0, 1]], tris[:, [1, 2]], tris[:, [2, 0]]])
        return set(map(tuple, np.sort(e, axis=1).tolist()))
    core_edges, ring_edges = edge_set(core_tris), edge_set(ring_piece['tris'])
    pairs = list(zip(range(len(core_ids) - 1), range(1, len(core_ids))))
    missing = 0
    for a, b in pairs:
        if tuple(sorted((int(core_ids[a]), int(core_ids[b])))) not in core_edges:
            missing += 1
        if tuple(sorted((int(ring_ids[a]), int(ring_ids[b])))) not in ring_edges:
            missing += 1

    def angle(a, b):
        cos = np.clip(np.einsum('ij,ij->i', a, b) / (np.linalg.norm(a, axis=1) * np.linalg.norm(b, axis=1)), -1, 1)
        return np.degrees(np.arccos(cos))
    shading = float(np.max(angle(core_normals[core_ids], ring_piece['normals'][ring_ids])))
    # (to_blender() turns a normal as it turns a position: it swaps and negates the plan's axes.)
    field = to_blender(region.gradient_normals(core_xyz[core_ids, 0], core_xyz[core_ids, 1]))
    core_dev = angle(core_geometric[core_ids], field)
    ring_dev = angle(ring_piece['geometric'], field)
    worst = core_xyz[core_ids[int(np.argmax(np.maximum(core_dev, ring_dev)))]]
    return dict(vertices=len(core_ids), gap_mm=gap, missing_edges=missing, normal_deg=shading,
                core_geometric_deg=float(np.max(core_dev)), ring_geometric_deg=float(np.max(ring_dev)),
                core_geometric_mean=float(np.mean(core_dev)), ring_geometric_mean=float(np.mean(ring_dev)),
                core_geometric_p95=float(np.percentile(core_dev, 95)),
                ring_geometric_p95=float(np.percentile(ring_dev, 95)), worst=worst.tolist())
