"""The far trees past a grounded area's playable boundary (layout.json region.farTrees; Docs/Areas/RansomsRest.md: Art
needs, "Far trees and rocks", and the performance plan's "No masked foliage past the boundary"): the art session's
opaque low-poly trees (Art/Models/Vegetation/FarTrees.py: SM_FarPine_A, SM_FarPine_B, SM_FarBroadleaf), placed by
Tools/Unreal/build_area.py as one instanced component per mesh, tagged Beyond, with no collision and no shadows.

Where they stand, out from "near" (cm) past the boundary (the regular scatter, which stops 10 m past it, is the only
tree layer closer in) to the ring's edge, never on the backdrop:
- candidates on a jittered SPACING m grid, kept off ground steeper than "steep" degrees (the ridges' bands, the
  escarpment's face, the far wall), the canyon's river, every road and the "clear" corridors (the line out of Stage Gap
  and its trestle);
- groves from noise, densest in the "woods" (polygons, each with a density and its share of pines; elsewhere BASE),
  in tree lines along the contours on slopes; on the canyon's floor along the river, and only a few on the plains past
  the far wall;
- thinned where nobody sees them: a tree that no standing spot inside the boundary sees (eye height on a VIEW_STEP m
  grid, and the "lookouts") keeps UNSEEN of its chance, so the "count" goes where it shows;
- pines higher up (by each wood's share), broadleaf low and by water; random yaw, a modest scale;
- and, by their own draws, dark pines in groups in the creases of the ring's ridge faces (area_faces.py), from the
  same "near" out, on steeper ground than the rest (the core's creases are the scatter's).
Each tree stands on the meshes the level gets (the core's top and the ring, through a BVH: their triangles differ from
the field by up to the ring's height tolerance), at the lowest point under its trunk and a little below: no trunk
floats, and the trunk's buried 0.6 m covers the rest. layout_computed.json's "farTrees" lists them per mesh as
[x, y, z, yaw, scale] (cm, whole; degrees, whole; scale, two decimals).

Numpy, and Blender's mathutils for the BVH: area_computed.py asks for them only once the terrain model has built the
meshes (area.top and area.ring_mesh); without them there are none.
"""
import math

import numpy as np

from area_math import fbm, points_in_polygon, sample, smoothstep

# The trees (Art/Models/Vegetation/FarTrees.py): the mesh, its kind, its trunk's radius at the foot and its height (m).
TREES = (('SM_FarPine_A', 'pine', 0.30, 13.0), ('SM_FarPine_B', 'pine', 0.36, 12.5),
         ('SM_FarBroadleaf', 'broadleaf', 0.42, 10.7))
SCALE = {'pine': (0.85, 1.2), 'broadleaf': (0.85, 1.15)}
SPACING = 7.0         # meters between candidates
MARGIN = 3.0          # meters inside the ring's edge
BASE = (0.22, 0.7)    # density and share of pines outside every wood
UNSEEN = 0.3          # of its chance a tree keeps where no standing spot sees it
RIVERSIDE = 0.6       # the chance along the canyon's river (cottonwoods), against a wood's density
VIEW_STEP = 25.0      # meters between the standing spots the views are taken from
EYE = 1.7             # meters
SEEN_AT = 0.55        # of a tree's height: it counts as seen when the line to there is clear
RIVER_CLEAR = 4.0     # meters past the river's edge
ROAD_CLEAR = 4.0      # meters past a road's edge
SINK = 0.05           # meters below the lowest ground under the trunk
# Pines in the ring's ridge creases (_crease_pines; the core's are the scatter's, area_scatter.py): the steepest ground
# (degrees) they stand on, meters between candidates, and their chance per unit of the creases' pine density
# (area_faces.py).
CREASE_STEEP = 46.0
CREASE_SPACING = 6.0
CREASE_CHANCE = 1.5


def _segments_distance(pts, line, closed=False):
    """The distance (m) from points (N, 2) to a polyline (or a closed polygon)."""
    best = np.full(len(pts), np.inf)
    ends = np.vstack([line, line[:1]]) if closed else line
    for a, b in zip(ends[:-1], ends[1:]):
        d = b - a
        t = np.clip(((pts - a) @ d) / max(float(d @ d), 1e-12), 0.0, 1.0)
        best = np.minimum(best, np.linalg.norm(pts - (a + t[:, None] * d), axis=1))
    return best


def _to_m(points):
    return np.asarray(points, dtype=np.float64) / 100.0


def _ground(area):
    """Heights (m) at points: the core's raster inside the map square, the ring's (R) outside it."""
    region = area.region

    def at(x, y):
        x, y = np.asarray(x, dtype=np.float64), np.asarray(y, dtype=np.float64)
        core = (np.abs(x) < area.half) & (np.abs(y) < area.half)
        out = sample(area.ring_h, x, y, region.half)
        out[core] = area.height(x[core], y[core])
        return out
    return at


def _seen(area, ground, pts, top, lookouts):
    """Whether each point at height top (m) shows from some standing spot inside the boundary: the eye on a VIEW_STEP m
    grid over the boundary's inside, and at the lookouts ([x, y, eye height over the ground], m). Each spot's horizon
    is marched out along 360 bearings, 3 m at a time; a point shows when it stands over its bearing's horizon there."""
    corners = area.boundary
    lo, hi = corners.min(axis=0), corners.max(axis=0)
    gx, gy = np.meshgrid(np.arange(lo[0] + VIEW_STEP * 0.5, hi[0], VIEW_STEP),
                         np.arange(lo[1] + VIEW_STEP * 0.5, hi[1], VIEW_STEP), indexing='ij')
    spots = np.column_stack([gx.ravel(), gy.ravel()])
    spots = spots[points_in_polygon(spots[:, 0], spots[:, 1], corners)]
    eyes = np.column_stack([spots, ground(spots[:, 0], spots[:, 1]) + EYE])
    if lookouts:
        extra = np.array(lookouts, dtype=np.float64)
        eyes = np.vstack([eyes, np.column_stack([extra[:, :2], ground(extra[:, 0], extra[:, 1]) + extra[:, 2]])])
    bins = 360
    step = 3.0
    reach = float(np.max(np.hypot(pts[:, 0, None] - eyes[None, :, 0], pts[:, 1, None] - eyes[None, :, 1]))) + step
    d = np.arange(step, reach, step)
    angles = np.radians(np.arange(bins) + 0.5)
    seen = np.zeros(len(pts), dtype=bool)
    for ex, ey, ez in eyes:
        # The horizon along each bearing: the steepest look up at the ground so far, as a tangent.
        rx = ex + np.cos(angles)[:, None] * d[None, :]
        ry = ey + np.sin(angles)[:, None] * d[None, :]
        rise = (ground(rx.ravel(), ry.ravel()).reshape(rx.shape) - ez) / d[None, :]
        horizon = np.maximum.accumulate(rise, axis=1)
        dx, dy = pts[:, 0] - ex, pts[:, 1] - ey
        dist = np.hypot(dx, dy)
        b = (np.degrees(np.arctan2(dy, dx)) % 360.0).astype(np.int64) % bins
        k = np.clip(((dist - step) / step).astype(np.int64) - 1, 0, len(d) - 1)  # the horizon short of the point
        seen |= (top - ez) / np.maximum(dist, 1e-6) >= horizon[b, k]
    return seen


def _mesh_heights(area):
    """A function giving the meshes' own heights (m) at points: rays down onto the core's top and the ring, through a
    BVH (NaN where none is hit). None when they aren't built."""
    pieces = []
    if getattr(area, 'top', None) is not None:
        pieces.append((area.top['verts'], area.top['tris']))
    for piece in getattr(area, 'ring_mesh', None) or []:
        pieces.append((piece['verts'], piece['tris']))
    if len(pieces) < 2:
        return None
    try:
        from mathutils import Vector
        from mathutils.bvhtree import BVHTree
    except ImportError:
        return None
    verts, tris, base = [], [], 0
    for v, t in pieces:
        verts.append(v)
        tris.append(t + base)
        base += len(v)
    verts, tris = np.vstack(verts), np.vstack(tris)
    tree = BVHTree.FromPolygons([tuple(map(float, p)) for p in verts], [tuple(map(int, t)) for t in tris],
                                all_triangles=True)
    top = float(verts[:, 2].max()) + 10.0
    down = Vector((0.0, 0.0, -1.0))

    def at(x, y):
        out = np.full(len(x), np.nan)
        for i, (px, py) in enumerate(zip(x, y)):
            hit = tree.ray_cast(Vector((float(px), float(py), top)), down)
            if hit[0] is not None:
                out[i] = hit[0].z
        return out
    return at


def place(area):
    """The far trees ({'note', 'meshes': {mesh: [[x, y, z, yaw, scale], ...]}}), or None without region.farTrees or
    the built meshes. Worked out once per area."""
    if getattr(area, 'far_trees', None) is not None:
        return area.far_trees
    spec = area.layout.get('region', {}).get('farTrees')
    if not spec or area.setting != 'grounded' or area.boundary is None or getattr(area, 'ring_h', None) is None:
        return None
    mesh_height = _mesh_heights(area)
    if mesh_height is None:
        return None
    region = area.region
    rng = np.random.default_rng(spec.get('seed', 23))
    ground = _ground(area)
    near = spec.get('near', 15000) / 100.0
    edge = region.half - MARGIN

    # Candidates, and where they may stand.
    c = np.arange(-region.half + SPACING * 0.5, region.half, SPACING)
    cx, cy = np.meshgrid(c, c, indexing='ij')
    pts = np.column_stack([cx.ravel(), cy.ravel()]) + rng.uniform(-0.3, 0.3, (cx.size, 2)) * SPACING
    x, y = pts[:, 0], pts[:, 1]
    ok = (np.abs(x) < edge) & (np.abs(y) < edge) & ~points_in_polygon(x, y, area.boundary)
    ok &= _segments_distance(pts, area.boundary, closed=True) >= near
    pts, x, y = pts[ok], x[ok], y[ok]
    z = ground(x, y)
    e = 0.75  # meters: the slope from heights either side, so a band's face isn't missed between samples
    slope = np.degrees(np.arctan(np.hypot(ground(x + e, y) - ground(x - e, y), ground(x, y + e) - ground(x, y - e))
                                 / (2.0 * e)))
    ok = slope <= spec.get('steep', 35.0)
    river = (_segments_distance(pts, region.river) if region.lip is not None and region.river is not None
             else np.full(len(pts), 1e3))
    half_river = region.river_width * 0.5 if region.lip is not None else 0.0
    ok &= river > half_river + RIVER_CLEAR
    for road in area.layout.get('roads', []):
        ok &= _segments_distance(pts, _to_m(road['path'])) > road['width'] / 200.0 + ROAD_CLEAR
    for corridor in spec.get('clear', []):
        ok &= _segments_distance(pts, _to_m(corridor['path'])) > corridor['width'] / 200.0
    water = area.water_surface()
    ok &= ~(sample(np.nan_to_num(water, nan=-1e3), x, y, area.half) > z - 0.3)
    pts, x, y, z, slope, river = pts[ok], x[ok], y[ok], z[ok], slope[ok], river[ok]

    # How dense: the woods, groves from noise, tree lines on the slopes; the canyon by its river; the plains.
    wood = np.full(len(pts), BASE[0])
    pines = np.full(len(pts), BASE[1])
    for w in spec.get('woods', []):
        poly = _to_m(w['polygon'])
        inside = points_in_polygon(x, y, poly)
        soft = np.minimum(1.0, _segments_distance(pts, poly, closed=True) / 30.0)
        soft = np.where(inside, 0.5 + 0.5 * soft, 0.5 - 0.5 * soft)
        wood = np.maximum(wood, w.get('density', 1.0) * soft)
        pines = np.where(soft > 0.5, w.get('pines', BASE[1]), pines)
    grove = smoothstep(-0.2, 0.25, fbm(x, y, 95.0, seed=401, octaves=3) + 0.45 * fbm(x, y, 32.0, seed=402, octaves=2))
    lines = smoothstep(0.3, 0.7, 0.5 + 0.5 * np.sin(2.0 * math.pi * z / 11.0 + 2.5 * fbm(x, y, 70.0, seed=403)))
    sloped = smoothstep(9.0, 16.0, slope)
    chance = wood * grove * (1.0 - sloped + sloped * (0.3 + 0.7 * lines))
    by_water = 1.0 - smoothstep(half_river + 6.0, half_river + 45.0, river)
    if region.lip is not None:
        canyon = sample(area.ring_dl, x, y, region.half) < -(region.wall + 1.0)
        plains = canyon & (z > -region.drop + region.far['height'] * 0.6)
        floor = canyon & ~plains
        chance = np.where(floor, np.maximum(RIVERSIDE * by_water * (0.35 + 0.65 * grove), 0.1 * grove), chance)
        chance = np.where(plains, 0.05 * grove, chance)
    else:
        canyon = np.zeros(len(pts), dtype=bool)
    tree_top = np.full(len(pts), SEEN_AT * 12.0)
    lookouts = [[p[0] / 100.0, p[1] / 100.0, p[2] / 100.0] for p in spec.get('lookouts', [])]
    seen = _seen(area, ground, pts, z + tree_top, lookouts)
    chance *= UNSEEN + (1.0 - UNSEEN) * seen

    # As many as the count asks for: the chances scaled to add up to it.
    target = float(spec.get('count', 2500))
    lo, hi = 0.0, 1e3
    for _ in range(60):
        k = 0.5 * (lo + hi)
        lo, hi = (k, hi) if np.minimum(1.0, k * chance).sum() < target else (lo, k)
    chosen = rng.random(len(pts)) < np.minimum(1.0, k * chance)

    # What each is: pines higher up, broadleaf low and by the water.
    low = smoothstep(-6.0, -18.0, z) * ~canyon
    high = smoothstep(14.0, 30.0, z) * ~canyon
    pine_p = np.clip(pines * (1.0 - 0.85 * by_water) + 0.6 * high * (1.0 - pines) - 0.15 * low, 0.0, 1.0)
    is_pine = rng.random(len(pts)) < pine_p
    stand_a = fbm(x, y, 45.0, seed=404, octaves=2) > -0.05      # pines of a kind stand together
    which = np.where(is_pine, np.where(stand_a, 0, 1), 2)
    yaw = rng.integers(0, 360, len(pts))
    unit = rng.random(len(pts))

    meshes = {name: [] for name, _, _, _ in TREES}
    ring_angles = np.radians(np.arange(8) * 45.0)
    for i in np.nonzero(chosen)[0]:
        name, kind, trunk, _ = TREES[which[i]]
        lo_s, hi_s = SCALE[kind]
        scale = round(lo_s + (hi_s - lo_s) * float(unit[i]), 2)
        r = trunk * scale * 1.15 + 0.1
        sx = np.concatenate([[x[i]], x[i] + np.cos(ring_angles) * r])
        sy = np.concatenate([[y[i]], y[i] + np.sin(ring_angles) * r])
        h = mesh_height(sx, sy)
        if np.isnan(h).any():
            continue
        meshes[name].append([int(round(x[i] * 100.0)), int(round(y[i] * 100.0)),
                             int(round((float(h.min()) - SINK) * 100.0)), int(yaw[i]), scale])
    shown = int((chosen & seen).sum())
    creases = _crease_pines(area, spec, near, ground, mesh_height, lookouts, meshes)
    area.far_trees = {
        'note': f'far trees past the boundary (Art/Levels/area_fartrees.py), one instanced component per mesh: '
                f'[x, y, z, yaw, scale] in cm and degrees, the pivot (the trunk\'s foot) at the lowest ground under '
                f'the trunk; {sum(len(v) for v in meshes.values())} in all, {shown} seen from inside the boundary'
                + (f', {creases} of them pines in the ridges\' creases' if creases else ''),
        'meshes': meshes}
    return area.far_trees


def _crease_pines(area, spec, near, ground, mesh_height, lookouts, meshes):
    """Dark pines in the creases of the ring's ridge faces (area_faces.py), few and in groups, appended to the pines'
    lists: on the ring (the core's scatter has the core's creases), from near m past the boundary like the rest, on
    slopes up to CREASE_STEEP degrees, only where some standing spot inside the boundary sees them. Their own random
    draws, so the trees above are the same with or without them. Returns how many."""
    import area_faces
    faces = area_faces.ring_fields(area)
    if faces is None:
        return 0
    region = area.region
    rng = np.random.default_rng(spec.get('seed', 23) + 1)
    edge = region.half - MARGIN
    c = np.arange(-region.half + CREASE_SPACING * 0.5, region.half, CREASE_SPACING)
    cx, cy = np.meshgrid(c, c, indexing='ij')
    pts = np.column_stack([cx.ravel(), cy.ravel()]) + rng.uniform(-0.4, 0.4, (cx.size, 2)) * CREASE_SPACING
    x, y = pts[:, 0], pts[:, 1]
    density = sample(faces['pines'], x, y, region.half) * sample(faces['face'], x, y, region.half)
    slope = sample(faces['slope'], x, y, region.half)
    ok = ((np.maximum(np.abs(x), np.abs(y)) > area.half + 1.0) & (np.abs(x) < edge) & (np.abs(y) < edge)
          & (density > 0.05) & (slope <= CREASE_STEEP))
    pts, x, y, density = pts[ok], x[ok], y[ok], density[ok]
    ok = _segments_distance(pts, area.boundary, closed=True) >= near
    for corridor in spec.get('clear', []):
        ok &= _segments_distance(pts, _to_m(corridor['path'])) > corridor['width'] / 200.0
    pts, x, y, density = pts[ok], x[ok], y[ok], density[ok]
    if not len(pts):
        return 0
    z = ground(x, y)
    seen = _seen(area, ground, pts, z + SEEN_AT * 12.0, lookouts)
    chosen = seen & (rng.random(len(pts)) < np.minimum(1.0, CREASE_CHANCE * density))
    stand_a = fbm(x, y, 45.0, seed=404, octaves=2) > -0.05  # the same stands as the trees above
    yaw = rng.integers(0, 360, len(pts))
    unit = rng.random(len(pts))
    ring_angles = np.radians(np.arange(8) * 45.0)
    count = 0
    for i in np.nonzero(chosen)[0]:
        name, kind, trunk, _ = TREES[0 if stand_a[i] else 1]
        lo_s, hi_s = SCALE[kind]
        scale = round(lo_s + (hi_s - lo_s) * float(unit[i]), 2)
        r = trunk * scale * 1.15 + 0.1
        h = mesh_height(np.concatenate([[x[i]], x[i] + np.cos(ring_angles) * r]),
                        np.concatenate([[y[i]], y[i] + np.sin(ring_angles) * r]))
        if np.isnan(h).any():
            continue
        meshes[name].append([int(round(x[i] * 100.0)), int(round(y[i] * 100.0)),
                             int(round((float(h.min()) - SINK) * 100.0)), int(yaw[i]), scale])
        count += 1
    return count
