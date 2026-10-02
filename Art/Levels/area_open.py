"""The open-ground metric (numpy only; Docs/Areas/RansomsRest.md, step 3b): how far every walkable meter inside an area's
playable boundary (an island's rim) is from its nearest break, so long stretches of empty ground show up before
anything is built. layout_computed.json gets the largest distance and the worst spots ("openGround"), and the plan
preview paints an overlay of it (area_preview.py).

Breaks are what a player can take cover behind or must go around:
- the terrain: a cliff (ground steeper than BREAK_SLOPE), or a rise of 1.5 m within 6 m (BREAK_RISE);
- buildings and the like (the placements area_shape.BUILDINGS sizes);
- layout.json "obstacles": walls, fences, outcrops, boulder groups, tree stands, ruins, as a "path" (a line) or a
  "polygon" (an area). Fences, low walls and knee-high cairns count: they are cover, even though a player sees over
  them;
- tree stands drawn as zones (kind forest or orchard), and the knobs' rocks.
Walkable ground is inside the boundary, gentler than WALKABLE_SLOPE and not under water. Ground marked open on purpose
(layout.json openGround.open: polygons with ids) isn't measured and breaks nothing. openGround.target (cm, 10 m by
default) is the most any walkable meter should be from a break.
"""
import math

import numpy as np

from area_math import Grid, catmull_rom, points_in_polygon, polygon_mask, polyline_field, raster_size, resize, sample

CELL = 1.0             # meters: the metric's raster
BREAK_RISE = (1.5, 6.0)
BREAK_SLOPE = 45.0     # degrees
WALKABLE_SLOPE = 40.0  # degrees
LINE_REACH = 0.6       # meters either side of an obstacle's line that count as the obstacle
TARGET = 10.0          # meters, unless openGround.target says otherwise
WORST = 8              # how many of the worst spots to report


def _to_m(points):
    return np.asarray(points, dtype=np.float64) / 100.0


def distance_to(mask, px):
    """The exact Euclidean distance (m) from every cell to the nearest True cell (separable: each column's nearest
    first, then each row's best combination)."""
    n0, n1 = mask.shape
    far = float(n0 + n1)
    g = np.where(mask, 0.0, far)
    for i in range(1, n0):
        np.minimum(g[i], g[i - 1] + 1.0, out=g[i])
    for i in range(n0 - 2, -1, -1):
        np.minimum(g[i], g[i + 1] + 1.0, out=g[i])
    j = np.arange(n1, dtype=np.float64)
    across = (j[:, None] - j[None, :]) ** 2
    out = np.empty((n0, n1))
    rows = max(1, int(4_000_000 // (n1 * n1)))
    for a in range(0, n0, rows):
        block = g[a:a + rows] ** 2
        out[a:a + rows] = np.min(block[:, None, :] + across[None, :, :], axis=2)
    return np.sqrt(out) * px


def _rise(h, px):
    """Where the ground within BREAK_RISE[1] m of a cell stands BREAK_RISE[0] m or more above it."""
    r = int(round(BREAK_RISE[1] / px))
    n0, n1 = h.shape
    padded = np.pad(h, r, mode='edge')
    top = h.copy()
    for di in range(-r, r + 1):
        for dj in range(-r, r + 1):
            if di * di + dj * dj <= r * r and (di or dj):
                np.maximum(top, padded[r + di:r + di + n0, r + dj:r + dj + n1], out=top)
    return top - h >= BREAK_RISE[0]


def _rectangle(center, yaw_deg, length, width):
    yaw = math.radians(yaw_deg)
    fwd, side = np.array([math.cos(yaw), math.sin(yaw)]), np.array([-math.sin(yaw), math.cos(yaw)])
    return np.array([center + fwd * a * length * 0.5 + side * b * width * 0.5
                     for a, b in ((1, 1), (-1, 1), (-1, -1), (1, -1))])


def obstacle_mask(area, grid):
    """Every break that isn't the terrain itself, on the metric's grid."""
    from area_shape import BUILDINGS  # area_shape imports nothing of this module's
    mask = np.zeros((grid.n, grid.n), dtype=bool)
    for ob in area.layout.get('obstacles', []):
        if 'polygon' in ob:
            poly = _to_m(ob['polygon'])
            mask |= polygon_mask(grid, poly)
            dist, _, _ = polyline_field(grid, poly, LINE_REACH, closed=True)
        else:
            dist, _, _ = polyline_field(grid, _to_m(ob['path']), LINE_REACH)
        mask |= np.isfinite(dist)
    for p in area.layout.get('placements', []):
        if p['kind'] in BUILDINGS:
            length, width = BUILDINGS[p['kind']]
            mask |= polygon_mask(grid, _rectangle(_to_m(p['location'][:2]), p.get('yaw', 0.0), length, width))
    for zone in area.layout.get('zones', []):
        if zone.get('kind', zone['id']) in ('forest', 'orchard'):
            mask |= polygon_mask(grid, catmull_rom(_to_m(zone['polygon']), closed=True, step=1.0))
    x, y = grid.mesh()
    for k in getattr(area, 'knobs', []):
        cx, cy = k['center']
        mask |= np.hypot(x - cx, y - cy) <= max(k['radius'] * 0.6, 1.5)
    return mask


def marked_open(area, grid):
    """Ground left open on purpose (openGround.open), and the ids of its polygons."""
    mask = np.zeros((grid.n, grid.n), dtype=bool)
    ids = []
    for spot in area.layout.get('openGround', {}).get('open', []):
        mask |= polygon_mask(grid, _to_m(spot['polygon']))
        ids.append(spot['id'])
    return mask, ids


def measure(area, log=print):
    """The metric on a CELL-meter raster over the map square: the result for layout_computed.json, and the distance
    raster (meters; NaN off walkable ground) for the plan's overlay. Kept on the area, so asking again is free."""
    if getattr(area, 'open_ground', None) is not None:
        return area.open_ground
    n = raster_size(2.0 * area.half, CELL, 2048)
    grid = Grid(n, area.half)
    px = grid.px
    h = area.resized(area.h, n)
    gx, gy = np.gradient(area.h.astype(np.float32), area.grid.px)
    # The steepest slope within each metric cell (a thin cliff mustn't average away).
    slope_fine = np.degrees(np.arctan(np.hypot(gx, gy))).astype(np.float32)
    del gx, gy
    step = area.grid.n / n
    if abs(step - round(step)) < 1e-9 and step >= 1:
        k = int(round(step))
        slope = slope_fine[:n * k, :n * k].reshape(n, k, n, k).max(axis=(1, 3))
    else:
        slope = resize(slope_fine, n, area.half)
    del slope_fine
    surface = area.water_surface()
    depth = resize(np.nan_to_num(surface - area.h, nan=-10.0).astype(np.float32), n, area.half)
    inside = resize(area.play_edge.astype(np.float32), n, area.half) > 0.0
    open_mask, open_ids = marked_open(area, grid)
    walkable = inside & (slope < WALKABLE_SLOPE) & (depth < 0.3) & ~open_mask
    breaks = (_rise(h, px) | (slope >= BREAK_SLOPE) | obstacle_mask(area, grid)) & ~open_mask
    dist = distance_to(breaks, px) if breaks.any() else np.full((n, n), np.inf)
    shown = np.where(walkable, dist, np.nan).astype(np.float32)
    target = area.layout.get('openGround', {}).get('target', TARGET * 100.0) / 100.0
    result = {'target': round(target * 100.0, 1), 'cell': round(px * 100.0, 1)}
    if walkable.any():
        values = dist[walkable]
        result['largest'] = round(float(values.max()) * 100.0, 1)
        result['overTarget'] = round(float((values > target).mean()), 4)
        result['walkable'] = round(float(walkable.sum()) * px * px)
        result['worst'] = _worst(area, grid, np.where(walkable, dist, -1.0), target)
    result['markedOpen'] = open_ids
    result['note'] = ('distances (cm) from walkable ground inside the boundary to its nearest break: largest is the '
                      'widest empty circle\'s radius; worst lists the centers of the emptiest stretches (one per '
                      'stretch); overTarget is the share of walkable ground farther than target from a break')
    area.open_ground = (result, grid, shown)
    log(f"open ground: the largest distance to a break {result.get('largest', 0) / 100.0:.1f} m "
        f"(target {target:g} m), {100.0 * result.get('overTarget', 0.0):.1f}% of {result.get('walkable', 0)} m² over it")
    return area.open_ground


def _worst(area, grid, dist, target):
    """The centers of the emptiest stretches, worst first: the farthest cell, then the farthest outside its empty
    circle, and so on, down to half the target."""
    spots = []
    work = dist.copy()
    x, y = grid.mesh()
    for _ in range(WORST):
        k = int(np.argmax(work))
        radius = float(work.flat[k])
        if radius < 0.5 * target:
            break
        cx, cy = float(x.flat[k]), float(y.flat[k])
        spots.append({'location': [round(cx * 100.0, 1), round(cy * 100.0, 1),
                                   round(float(area.height(cx, cy)) * 100.0, 1)], 'distance': round(radius * 100.0, 1)})
        work[np.hypot(x - cx, y - cy) < max(radius, 2.0)] = -1.0
    return spots
