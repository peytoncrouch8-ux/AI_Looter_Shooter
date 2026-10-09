"""An area's macro color map, T_<Area>Macro_BC.png: the layout's macro cells over the map square (4096 px at 5 cm on
the tutorial island), painted from the built area: meadow greens with sun-dried patches and clover, the forest floor,
farmyards and squares, the orchard rows, mowed stripes on the target meadow, dirt roads (ragged edges, wheel ruts, a
grassy middle, pebbles), footpaths, mud and sand at the water, rock on cliffs and steep banks, the drier plateaus.

RGB is the ground's average color (sRGB): M_Terrain only modulates its brightness with the detail textures.
Alpha picks the detail texture: 0 grass and soil, 1 rock (roads about 0.4, footpaths 0.25, scree 0.6). It never
reaches exactly 0 (the lowest value is 1/255), so an importer that treats zero-alpha pixels as empty (Unreal's PNG
infill) keeps the color. The image has north up; UV 0 of the terrain tiles maps the map square onto it (on the tutorial
island U = (Y + 10240) / 20480, V = (X + 10240) / 20480, Unreal cm; layout_computed.json macroMap).

A grounded area's core treats the escarpment's lip as the island treats its rim (sun-dried turf, a band of rock at the
edge), and its colors blend into the surround ring's map across the seam band. paint_ring() paints that map,
T_<Area>RingMacro_BC.png, over the ring's square from the same palette and noise: ridges with dark pine floors and
rock bands, the canyon's ochre floor with its river, the plains past the far wall. Pits, gullies and knobs get their
own ground (_features). A layout's "macro": {"grade": ...} turns both maps' greens to another season (GRADES).
"""
import math
import os

import numpy as np

import area_computed
import area_faces
from area_math import Grid, blur, catmull_rom, cells, fbm, fbm_raster, polyline_field, resize, sample, signed_distance
from area_mesh import extend_nan
from area_shape import BUILDINGS, to_m


def _c(value):
    return np.array([(value >> 16) & 255, (value >> 8) & 255, value & 255], dtype=np.float32) / 255.0


# Grades a layout can ask for (layout.json "macro": {"grade": name}): the palette's greens turned toward another
# season, applied to both maps after painting, so the core and the ring still agree at the seam. "golden" is late
# summer (Ransom's Rest): the meadow's greens become golden grass, toward hue 42 degrees, a little lighter and paler;
# straw, soil, rock and water keep their colors.
GRADES = {'golden': dict(hue=42.0, keep=0.1, lift=0.28, fade=0.18)}


def _grade(rgb, name):
    """Grades an (n, n, 3) sRGB array in place, a band of rows at a time."""
    spec = GRADES[name]
    for a in range(0, rgb.shape[0], 256):
        block = rgb[a:a + 256]
        r, g, b = block[..., 0], block[..., 1], block[..., 2]
        mx = np.maximum(np.maximum(r, g), b)
        delta = mx - np.minimum(np.minimum(r, g), b)
        safe = np.where(delta > 1e-6, delta, 1.0)
        hue = np.where(mx == r, ((g - b) / safe) % 6.0, np.where(mx == g, (b - r) / safe + 2.0, (r - g) / safe + 4.0))
        hue = np.where(delta > 1e-6, hue * 60.0, 0.0)
        sat = np.where(mx > 1e-6, delta / np.maximum(mx, 1e-6), 0.0)
        w = (_ss(spec['hue'] - 10.0, spec['hue'], hue) * (1.0 - _ss(150.0, 170.0, hue)) * _ss(0.06, 0.16, sat))
        green = _ss(50.0, 75.0, hue) * w
        val = mx * (1.0 + spec['lift'] * green)
        sat = sat * (1.0 - spec['fade'] * green)
        target = spec['hue'] + (np.maximum(hue, spec['hue']) - spec['hue']) * spec['keep']
        h6 = (hue + (target - hue) * w) / 60.0
        c = val * sat
        x = c * (1.0 - np.abs(h6 % 2.0 - 1.0))
        k = np.floor(h6).astype(np.int64) % 6
        zero = np.zeros_like(c)
        for i, (rr, gg, bb) in enumerate(((c, x, zero), (x, c, zero), (zero, c, x), (zero, x, c), (x, zero, c),
                                          (c, zero, x))):
            sel = k == i
            block[..., 0] = np.where(sel, rr + val - c, block[..., 0])
            block[..., 1] = np.where(sel, gg + val - c, block[..., 1])
            block[..., 2] = np.where(sel, bb + val - c, block[..., 2])
        np.clip(block, 0.0, 1.0, out=block)


# The palette (sRGB): warm, slightly desaturated meadow greens, straw, soil, stone.
GREEN_DEEP, GREEN_MID, GREEN_WARM = _c(0x506530), _c(0x68773b), _c(0x818746)
GREEN_LUSH, CLOVER = _c(0x4b6431), _c(0x43582d)
DRY, STRAW = _c(0x958c56), _c(0xab9d68)
SOIL, SOIL_DARK = _c(0x6c5840), _c(0x564634)
FOREST_FLOOR, LITTER, MOSS = _c(0x4e5832), _c(0x6b5d3f), _c(0x55632f)
DIRT, DIRT_LIGHT, RUT = _c(0x8b7658), _c(0x9e8a69), _c(0x6f5d46)
PEBBLE_LIGHT, PEBBLE_DARK = _c(0xa89c86), _c(0x5c4f3e)
MUD, MUD_WET, SAND, POND_BED = _c(0x54463a), _c(0x3d3329), _c(0xa3936f), _c(0x3b3a2b)
ROCK, ROCK_LIGHT, ROCK_DARK, LICHEN, SCREE = _c(0x88806f), _c(0xa39b8b), _c(0x5d564b), _c(0x8e8b5a), _c(0x8f8878)
PLATEAU_GRASS = _c(0x7f834a)

# The worn discs of a yard (layout.json "yards"), by its kind: (radius, color, strength, alpha, soft edge) in meters,
# painted in order.
YARDS = {
    'farmyard': ((9.0, SOIL, 0.55, 0.15, 4.0), (6.0, DIRT, 0.5, 0.2, 3.0)),
    'square': ((10.0, DRY, 0.5, None, 4.0), (7.5, DIRT_LIGHT, 0.75, 0.35, 2.5)),
    # A town's square (Skyreach's Crossroads Town): its buildings' fronts stand about 14 m out, so the trampled ground
    # reaches them and the bare middle is wide enough to cross and fight in.
    'townSquare': ((15.0, DRY, 0.5, None, 4.0), (11.5, DIRT_LIGHT, 0.75, 0.35, 3.0)),
}


class Canvas:
    """RGB + alpha over the grid; paint() blends a color (or per-pixel colors) in with a weight."""

    def __init__(self, n):
        self.rgb = np.zeros((n, n, 3), dtype=np.float32)
        self.alpha = np.zeros((n, n), dtype=np.float32)

    def paint(self, color, weight, alpha=None, where=None):
        w = np.clip(weight, 0.0, 1.0).astype(np.float32)
        if where is not None:
            w = w * where
        color = np.asarray(color, dtype=np.float32)
        if color.ndim == 1:
            self.rgb += (color[None, None, :] - self.rgb) * w[..., None]
        else:
            self.rgb += (color - self.rgb) * w[..., None]
        if alpha is not None:
            self.alpha += (np.float32(alpha) - self.alpha) * w


def _ss(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return (t * t * (3.0 - 2.0 * t)).astype(np.float32)


def _mix(a, b, t):
    """Per-pixel colors between a and b (t of any shape; the result has a trailing RGB axis)."""
    return (a + (b - a) * np.clip(t, 0.0, 1.0)[..., None]).astype(np.float32)


def paint(area, out_path, preview_dir=None, log=print):
    n = area.sizes['macro']
    grid = Grid(n, area.half)
    log('macro: fields')
    h = area.resized(area.h, n)
    gx, gy = np.gradient(area.h.astype(np.float32), area.grid.px)
    slope = area.resized(np.degrees(np.arctan(np.hypot(gx, gy))).astype(np.float32), n)
    del gx, gy
    # Convexity at the scale of hills (about 4 m of blur), so small bumps don't count: > 0 on crowns and ridges. It
    # and the wetness are worked out on the curvature raster (20 cm on the tutorial island).
    nf = area.sizes['curvature']
    pf = 2.0 * area.half / nf
    h_fields = blur(area.resized(area.h, nf), cells(2.4, pf))
    lap = np.gradient(np.gradient(h_fields, axis=0), axis=0) + np.gradient(np.gradient(h_fields, axis=1), axis=1)
    convex = area.resized(-lap / pf ** 2, n)
    ao = area.resized(area.ao, n) if getattr(area, 'ao', None) is not None else np.ones((n, n), np.float32)
    if area.setting == 'grounded':
        # The escarpment's lip stands in for the island's rim (and there's none without an escarpment).
        edge = area.resized(area.dl, n) if area.dl is not None else np.full((n, n), 1e3, np.float32)
    else:
        edge = area.resized(area.edge, n)
    surface = area.water_surface()
    wet_fields = area.resized(np.where(np.isfinite(surface) & (surface > area.h), 1.0, 0.0).astype(np.float32), nf)
    near_water = area.resized(np.clip(blur(wet_fields, cells(2.4, pf)) * 3.0, 0.0, 1.0), n)  # about 5 m around water
    water = area.resized(np.nan_to_num(extend_nan(surface, cells(4.0, area.grid.px)), nan=-100.0), n)
    depth = water - h  # > 0 under water

    canvas = Canvas(n)
    log('macro: meadow')
    n_large = fbm_raster(grid, 38.0, seed=101, octaves=2)
    n_mid = fbm_raster(grid, 13.0, seed=102, octaves=2)
    n_patch = fbm_raster(grid, 7.0, seed=103, octaves=3)
    n_fine = fbm_raster(grid, 1.1, seed=104, octaves=2)
    n_grain = fbm_raster(grid, 0.35, seed=105, octaves=1)
    canvas.rgb[:] = _mix(GREEN_DEEP, GREEN_MID, 0.5 + 0.8 * n_large)
    canvas.paint(GREEN_WARM, 0.55 * _ss(-0.1, 0.6, n_mid))
    # Sun-dried: crowns, the rim, dry patches; wetter ground (hollows, near water) stays lush.
    n_dry = fbm_raster(grid, 4.0, seed=108, octaves=2)
    rim_dry = (1.0 - _ss(1.0, 7.0, edge)) * _ss(-0.3, 0.4, n_mid + 0.5 * n_patch)  # island setting
    dryness = (0.6 * _ss(0.004, 0.045, convex) + 0.4 * _ss(0.1, 0.7, n_patch) + 0.35 * rim_dry
               - 0.6 * near_water - 0.4 * (1.0 - _ss(0.72, 0.95, ao)) + 0.15 * n_dry + 0.04 * n_fine)
    dryness = np.clip(dryness, 0.0, 1.0)
    canvas.paint(DRY, 0.8 * _ss(0.35, 0.85, dryness))
    canvas.paint(STRAW, 0.55 * _ss(0.75, 1.0, dryness))
    canvas.paint(GREEN_LUSH, 0.45 * near_water)
    clover = fbm_raster(grid, 2.4, seed=106, octaves=2)
    canvas.paint(CLOVER, 0.5 * _ss(0.35, 0.6, clover) * (1.0 - _ss(0.4, 0.8, dryness)))
    soil_spots = _ss(0.6, 0.78, fbm_raster(grid, 3.2, seed=107, octaves=2)) * _ss(0.5, 0.95, dryness)
    canvas.paint(SOIL, 0.6 * soil_spots)
    del dryness, clover, soil_spots, n_patch

    _forest(area, grid, canvas, n_mid, n_fine)
    _plateau_tops(area, grid, canvas, n_fine)
    _features(area, grid, canvas, n_fine, n_grain)
    _range(area, grid, canvas)
    _orchard(area, grid, canvas)
    _yards(area, grid, canvas, n_fine)
    log('macro: roads')
    _roads(area, grid, canvas, h, n_fine, n_grain)
    log('macro: water and rock')
    _water(canvas, depth, near_water, n_mid, n_fine)
    road_near = 1.0 - _ss(1.5, 4.0, area.resized(np.minimum(area.road_gap, 50.0), n))
    _rock(area, grid, canvas, slope, ao, edge, depth, n_fine, n_grain, np.maximum(near_water, road_near))

    # Grain everywhere: a little brightness noise at 35 cm and 1 m, so no area is flat.
    canvas.rgb *= (1.0 + 0.05 * n_grain + 0.05 * n_fine)[..., None]
    rgba = np.empty((n, n, 4), dtype=np.float32)
    rgba[..., :3] = np.clip(canvas.rgb, 0.0, 1.0)
    rgba[..., 3] = np.clip(canvas.alpha, 1.0 / 255.0, 1.0)
    grade = area.layout.get('macro', {}).get('grade')
    if grade:
        _grade(rgba[..., :3], grade)
    del canvas
    if area.setting == 'grounded':
        # The ridges' big faces (area_faces.py), after the grade: kept off what the core itself shapes and paints
        # there (water, roads, its features, the rock raised past the boundary's closed edges).
        faces = area_faces.fields(area, area.grid, area.h)
        if faces is not None:
            log('macro: ridge faces')
            shaped = _ss(0.5, 1.2, area.resized(np.abs(area.h - area.h_region), n))
            keep = np.maximum.reduce([near_water, road_near, shaped, _ss(0.0, 0.08, depth)])
            del shaped
            area_faces.paint(rgba, faces, keep=keep, fine=n_fine, grain=n_grain)
            del faces, keep
    if getattr(area, 'ring_rgba', None) is not None:
        # Across the seam band the core's colors fade into the ring's, which they meet exactly at the square's edge.
        import area_region
        x, y = grid.mesh()
        w = area_region.seam_weight(area, x, y).astype(np.float32)
        for c in range(4):
            ring = sample(area.ring_rgba[..., c], x, y, area.region.half).astype(np.float32)
            rgba[..., c] = ring + (rgba[..., c] - ring) * w
        del x, y, w
    _save(rgba, out_path)
    written = out_path
    if preview_dir:
        # A preview for people, at most 1024 px (the map is a power of two, so it scales down evenly).
        preview = os.path.join(preview_dir, 'macro.png')
        p = min(n, 1024)
        small = rgba.reshape(p, n // p, p, n // p, 4).mean(axis=(1, 3))
        small[..., 3] = 1.0
        os.makedirs(os.path.dirname(preview), exist_ok=True)
        _save(small, preview)
        written += ' and ' + preview
    log(f'macro: wrote {written}')


def _save(rgba, path):
    """Saves an (n, n, 4) sRGB array (row 0 = south, the bottom of the image) as a PNG through Blender."""
    import bpy
    n = rgba.shape[0]
    image = bpy.data.images.new('_macro_save', rgba.shape[1], n, alpha=True)
    image.colorspace_settings.name = 'sRGB'
    image.alpha_mode = 'STRAIGHT'
    image.pixels.foreach_set(np.ascontiguousarray(rgba, dtype=np.float32).ravel())
    os.makedirs(os.path.dirname(path), exist_ok=True)
    image.filepath_raw = path
    image.file_format = 'PNG'
    image.save()
    bpy.data.images.remove(image)
    # A material may already show this file: reload it.
    target = os.path.normcase(os.path.abspath(path))
    for other in list(bpy.data.images):
        if other.filepath and os.path.normcase(os.path.abspath(bpy.path.abspath(other.filepath))) == target:
            other.reload()


def _zones_weight(area, grid, kind, soft, wobble):
    """1 inside the zones of a kind, fading out over soft meters across their edges (pushed about by wobble); None
    when the area has no such zone."""
    weights = []
    for zone in area.zones_of(kind):
        poly = to_m(zone['polygon'])
        sd = area.resized(signed_distance(area.grid, catmull_rom(poly, closed=True, step=0.5), soft + 8.0), grid.n)
        weights.append(1.0 - _ss(-soft, soft, sd + wobble))
    return np.maximum.reduce(weights) if weights else None


def _forest(area, grid, canvas, n_mid, n_fine):
    """The groves' floor: darker olive, leaf litter and moss in patches, clearings between."""
    wobble = 4.0 * fbm_raster(grid, 12.0, seed=111, octaves=2)
    w = _zones_weight(area, grid, 'forest', 6.0, wobble)
    if w is None:
        return
    litter = fbm_raster(grid, 3.5, seed=112, octaves=2)
    floor = _mix(FOREST_FLOOR, LITTER, 0.45 + 0.9 * litter)
    cover = w * (0.35 + 0.3 * _ss(-0.3, 0.4, n_mid))
    canvas.paint(floor, cover * _ss(-0.5, 0.2, litter + 0.3 * n_fine))
    canvas.paint(MOSS, 0.35 * w * _ss(0.35, 0.6, fbm_raster(grid, 2.0, seed=113, octaves=2)))


def _features(area, grid, canvas, n_fine, n_grain):
    """The ground of step 3b's features: a pit's damp, dark floor and the scree at its wall's foot, a gully's gravel
    bed and earthen banks, the trampled soil a knob's rock stands on."""
    for p in getattr(area, 'pits', []):
        sink = area.resized(p['rise'], grid.n)
        canvas.paint(_mix(SOIL_DARK, LITTER, 0.5 + 0.8 * n_fine), 0.75 * _ss(0.9, 0.99, sink), alpha=0.15)
        foot = _ss(0.55, 0.85, sink) * (1.0 - _ss(0.95, 0.995, sink))
        canvas.paint(_mix(SCREE, ROCK_DARK, 0.4 + 0.6 * n_grain), 0.8 * foot, alpha=0.65)
        # Away from the walls the floor is bare grit and dust (it grows little: area_scatter.py).
        from_wall = -area.resized(p['sd'], grid.n) - p['width'] * 0.5
        grit = _ss(0.9, 0.99, sink) * _ss(2.0, 6.0, from_wall + 1.5 * n_fine)
        del from_wall
        canvas.paint(_mix(DIRT, PEBBLE_LIGHT, 0.4 + 0.8 * n_grain), 0.5 * grit, alpha=0.3)
        canvas.paint(PEBBLE_DARK, 0.45 * grit * _ss(0.5, 0.7, n_grain), alpha=0.5)
    for g in getattr(area, 'gullies', []):
        reach = g['half'] + (g['depth'] + 3.0) / g['bank'] + 1.0
        dist, along, _ = polyline_field(grid, g['pts'], reach)
        near = np.isfinite(dist)
        if not near.any():
            continue
        d = np.where(near, dist, 1e3) + 0.3 * n_fine
        bed = 1.0 - _ss(g['half'] - 0.4, g['half'] + 0.3, d)
        bank = (1.0 - _ss(reach - 2.0, reach, d)) * (1.0 - bed)
        canvas.paint(_mix(SOIL, SOIL_DARK, 0.5 + n_fine), 0.45 * bank, alpha=0.2)
        canvas.paint(_mix(SAND, PEBBLE_LIGHT, 0.5 + 0.9 * n_grain), 0.85 * bed, alpha=0.55)
        canvas.paint(PEBBLE_DARK, 0.5 * bed * _ss(0.55, 0.7, n_grain), alpha=0.6)
    for k in getattr(area, 'knobs', []):
        _disc(grid, canvas, np.asarray(k['center']), max(k['radius'] * 0.8, 1.2), SOIL, 0.7, 0.2, soft=1.0)


def _plateau_tops(area, grid, canvas, n_fine):
    """Drier grass on each plateau, thin soil and rock breaking through."""
    for p in area.plateaus:
        top = area.resized(p['rise'], grid.n)
        canvas.paint(PLATEAU_GRASS, 0.4 * _ss(0.85, 0.99, top))
        thin = _ss(0.25, 0.5, fbm_raster(grid, 5.0, seed=121, octaves=2) + 0.3 * n_fine) * _ss(0.9, 0.99, top)
        canvas.paint(SOIL, 0.55 * thin)
        outcrop = _ss(0.42, 0.52, fbm_raster(grid, 6.5, seed=122, octaves=3)) * _ss(0.9, 0.99, top)
        rock = _mix(ROCK, ROCK_LIGHT, 0.5 + n_fine)
        canvas.paint(rock, outcrop, alpha=1.0)


def _range(area, grid, canvas):
    """Mowed stripes across the target meadow (the player shoots along them, northward), and worn ground under
    the target dummies."""
    w = _zones_weight(area, grid, 'range', 1.5, 0.0)
    if w is None:
        return
    x, _ = grid.mesh()
    stripes = 0.5 + 0.5 * np.sin(x * (2.0 * math.pi / 6.0)).astype(np.float32)
    canvas.paint(GREEN_WARM * 1.05, 0.16 * w * _ss(0.3, 0.7, stripes))
    canvas.paint(GREEN_MID * 0.96, 0.12 * w * (1.0 - _ss(0.3, 0.7, stripes)))
    for p in area.layout['placements']:
        if p['kind'] == 'TargetDummy':
            _disc(grid, canvas, np.asarray(p['location'][:2]) / 100.0, 1.3, SOIL, 0.8, 0.1)


def _disc(grid, canvas, center, radius, color, strength, alpha, soft=0.8):
    """A worn patch: a disc whose edge wanders (by a fifth of its radius) and frays."""
    reach = radius * 1.25 + soft + 1.0
    i0, i1 = grid.index_range(center[0] - reach, center[0] + reach)
    j0, j1 = grid.index_range(center[1] - reach, center[1] + reach)
    x, y = grid.mesh(i0, i1, j0, j1)
    d = (np.hypot(x - center[0], y - center[1]) + 0.2 * radius * fbm(x, y, max(2.0, radius * 0.7), seed=130)
         + 0.35 * fbm(x, y, 1.2, seed=131))
    w = strength * (1.0 - _ss(radius - soft, radius, d))
    sub = Canvas(1)
    sub.rgb = canvas.rgb[i0:i1, j0:j1]
    sub.alpha = canvas.alpha[i0:i1, j0:j1]
    sub.paint(color, w, alpha=alpha)


def _orchard(area, grid, canvas):
    """Along each tree row: a darker mowed strip and bare soil around every trunk."""
    for row in area_computed.orchard_rows(area):
        pts = np.array([t[:2] for t in row['trees']]) / 100.0
        if len(pts) < 2:
            continue
        dist, _, _ = polyline_field(grid, pts, 3.0)
        near = np.isfinite(dist)
        strip = np.zeros(dist.shape, np.float32)
        strip[near] = 1.0 - _ss(0.6, 1.4, dist[near])
        canvas.paint(GREEN_DEEP, 0.45 * strip)
        for t in pts:
            _disc(grid, canvas, t, 1.0, SOIL_DARK, 0.75, 0.05)


def _yards(area, grid, canvas, n_fine):
    """Worn ground around buildings: trampled grass all around, bare dirt in front; the layout's yards (a farmyard,
    a village square); and its trails between doors (layout.json "trails": placement ids or [X, Y] in cm)."""
    for p in area.layout['placements']:
        if p['kind'] not in BUILDINGS:
            continue
        length, width = BUILDINGS[p['kind']]
        c = np.asarray(p['location'][:2]) / 100.0
        yaw = math.radians(p.get('yaw', 0.0))
        fwd = np.array([math.cos(yaw), math.sin(yaw)])
        reach = max(length, width) * 0.5 + 3.0
        _disc(grid, canvas, c, reach, DRY, 0.45, None, soft=2.5)
        _disc(grid, canvas, c + fwd * (length * 0.5 + 1.4), max(1.2, width * 0.3), SOIL, 0.8, 0.12, soft=1.2)
    for yard in area.layout.get('yards', []):
        center = area.yard_center(yard)
        if center is None:
            continue
        for radius, color, strength, alpha, soft in YARDS[yard['kind']]:
            _disc(grid, canvas, center, radius, color, strength, alpha, soft=soft)
    for a, b in area.layout.get('trails', []):
        pts = np.array([area.point(a), area.point(b)])
        dist, _, _ = polyline_field(grid, pts, 2.0)
        near = np.isfinite(dist)
        w = np.zeros(dist.shape, np.float32)
        w[near] = 1.0 - _ss(0.25, 0.75, dist[near] + 0.2 * n_fine[near])
        canvas.paint(SOIL, 0.6 * w, alpha=0.1)


def _roads(area, grid, canvas, h, n_fine, n_grain):
    edge_noise = 0.35 * fbm_raster(grid, 2.5, seed=141, octaves=2)
    pebbles = fbm_raster(grid, 0.22, seed=142, octaves=1)
    tufts = fbm_raster(grid, 0.6, seed=143, octaves=2)
    curves = [dict(r) for r in area.roads] + [dict(r, kind='ramp') for r in area.ramps]
    for road in curves:
        half = road['width'] * 0.5
        dist, along, side = polyline_field(grid, road['pts'], half + 3.0)
        near = np.isfinite(dist)
        if not near.any():
            continue
        idx = np.nonzero(near)
        d = dist[near]
        signed = d * side[near]
        en = edge_noise[near] + 0.15 * n_fine[near]
        # Not where the ground drops away below the road (the creek under the bridge).
        z = np.interp(along[near], road['s'], road['z'])
        onground = _ss(-0.5, -0.25, h[near] - z)
        sub_rgb = canvas.rgb[idx]
        sub_a = canvas.alpha[idx]
        if road['kind'] == 'dirt':
            core = (1.0 - _ss(half - 0.6, half + 0.05, d + en)) * onground
            worn = (1.0 - _ss(half, half + 1.3, d + en)) * onground
            color = _mix(DIRT, DIRT_LIGHT, 0.5 + 1.2 * n_fine[near])
            # Two wheel ruts, a grassy hump between them, grass creeping in from the edges.
            gauge = min(0.85, half * 0.42)
            wob = 0.06 * np.sin(along[near] * 0.9) + 0.04 * n_fine[near]
            rut = np.maximum(1.0 - _ss(0.12, 0.3, np.abs(signed - gauge + wob)),
                             1.0 - _ss(0.12, 0.3, np.abs(signed + gauge + wob)))
            hump = (1.0 - _ss(0.2, 0.45, np.abs(signed + wob))) * _ss(0.0, 0.5, tufts[near] + 0.3)
            creep = _ss(half - 0.9, half - 0.1, d + en) * _ss(0.1, 0.4, tufts[near])
            sub_rgb, sub_a = _blend(sub_rgb, sub_a, DRY, 0.5 * worn)
            sub_rgb, sub_a = _blend(sub_rgb, sub_a, color, core, 0.42)
            sub_rgb, sub_a = _blend(sub_rgb, sub_a, RUT, 0.55 * rut * core, 0.35)
            sub_rgb, sub_a = _blend(sub_rgb, sub_a, GREEN_WARM * 0.92, 0.7 * hump * core, 0.1)
            sub_rgb, sub_a = _blend(sub_rgb, sub_a, DRY, 0.8 * creep * core, 0.12)
        else:
            width = half * (0.55 if road['kind'] == 'path' else 0.75)
            core = (1.0 - _ss(width - 0.45, width + 0.1, d + 0.7 * en)) * onground
            worn = (1.0 - _ss(width, width + 1.0, d + en)) * onground
            color = _mix(SOIL, DIRT, 0.55 + 0.9 * n_fine[near])
            creep = _ss(0.25, 0.55, tufts[near]) * _ss(width - 0.7, width, d + en)
            sub_rgb, sub_a = _blend(sub_rgb, sub_a, DRY, 0.5 * worn)
            sub_rgb, sub_a = _blend(sub_rgb, sub_a, color, core, 0.25 if road['kind'] == 'path' else 0.35)
            sub_rgb, sub_a = _blend(sub_rgb, sub_a, DRY, 0.75 * creep * core, 0.1)
        # Pebbles: light and dark specks on the bare dirt.
        peb = core * _ss(0.55, 0.7, pebbles[near])
        sub_rgb, sub_a = _blend(sub_rgb, sub_a, PEBBLE_LIGHT, 0.7 * peb, 0.6)
        sub_rgb, sub_a = _blend(sub_rgb, sub_a, PEBBLE_DARK, 0.6 * core * _ss(0.55, 0.7, -pebbles[near]), 0.5)
        canvas.rgb[idx] = sub_rgb
        canvas.alpha[idx] = sub_a


def _blend(rgb, alpha, color, weight, a=None):
    w = np.clip(weight, 0.0, 1.0).astype(np.float32)
    color = np.asarray(color, dtype=np.float32)
    rgb = rgb + (color - rgb) * w[:, None] if color.ndim == 2 else rgb + (color[None, :] - rgb) * w[:, None]
    if a is not None:
        alpha = alpha + (np.float32(a) - alpha) * w
    return rgb, alpha


def _water(canvas, depth, near_water, n_mid, n_fine):
    """The ponds and creeks: a dark bed under water, wet mud at the waterline, sand on some shores."""
    under = _ss(0.0, 0.08, depth)
    canvas.paint(_mix(MUD, POND_BED, _ss(0.1, 0.8, depth)), under, alpha=0.08)
    line = (1.0 - _ss(0.05, 0.35, -depth)) * (1.0 - under)
    sandy = _ss(0.1, 0.5, n_mid + 0.3 * n_fine)
    canvas.paint(_mix(MUD_WET, SAND, sandy), 0.9 * line, alpha=0.15)
    damp = (1.0 - _ss(0.3, 0.9, -depth)) * (1.0 - line) * (1.0 - under)
    canvas.paint(MUD, 0.35 * damp * (1.0 - sandy))


def _rock(area, grid, canvas, slope, ao, edge, depth, n_fine, n_grain, earthy):
    """Rock on cliffs and steep banks (by slope, with a ragged boundary), scree below each plateau's cliffs, and
    the island's edge where the turf rolls over into the rock wall."""
    jitter = 7.0 * fbm_raster(grid, 3.0, seed=151, octaves=2)
    # Beside water and roads, steep ground is earth (cut banks), not bare rock.
    rockw = _ss(34.0, 46.0, slope + jitter) * (1.0 - 0.85 * earthy)
    # Steep banks of soil first (the creek's ravine), rock on the steepest.
    bank = _mix(SOIL_DARK, SOIL, 0.55 + n_fine) * (1.0 - 0.12 * earthy[..., None])
    canvas.paint(bank, 0.8 * _ss(24.0, 34.0, slope + jitter) * _ss(-0.2, 0.3, -depth),
                 alpha=0.2)
    for p in area.plateaus:
        sd = area.resized(p['sd'], grid.n)
        width = p['width']
        scree = (1.0 - _ss(width * 0.5, width * 0.5 + 3.5, sd + 1.2 * n_fine)) * _ss(width * 0.2, width * 0.5, sd)
        canvas.paint(_mix(SCREE, ROCK_DARK, 0.4 + 0.6 * n_grain), 0.85 * scree, alpha=0.65)
        # The cliff's lip: a band of bare rock just inside the top edge (inside = sd below -width / 2).
        inset = -(sd + width * 0.5) + 0.5 * n_fine
        lip = _ss(-0.4, 0.1, inset) * (1.0 - _ss(0.9, 1.8, inset))
        rockw = np.maximum(rockw, lip * _ss(0.2, 0.6, 0.5 + n_fine))
    # The island's rim: turf rolling over into the rock wall (island setting).
    rim = 1.0 - _ss(0.15, 1.1, edge + 0.4 * n_fine)
    canvas.paint(_mix(SOIL_DARK, ROCK_DARK, 0.5 + n_fine), rim, alpha=0.7)
    # Rock itself: lighter on crowns and faces, darker in crevices (the occlusion), a little lichen.
    shade = np.clip(0.55 + 0.9 * (ao - 0.8) + 0.35 * n_fine + 0.25 * n_grain, 0.0, 1.0)
    rock = _mix(ROCK_DARK, ROCK_LIGHT, shade)
    canvas.paint(rock, rockw, alpha=1.0)
    canvas.paint(LICHEN, 0.35 * rockw * _ss(0.45, 0.6, fbm_raster(grid, 1.6, seed=152, octaves=2)))


# --- The surround ring's map (grounded) ---

def paint_ring(area, out_path, preview_dir=None, log=print):
    """T_<Area>RingMacro_BC.png over the ring's square (layout.json region.ring.macro px, 2048 by default), from the
    regional field on the ring's raster: the same meadow as the core's (the same noise, so they agree at the seam),
    pines darkening the ridges' slopes, rock where it's steep, the canyon's ochre floor with its river, and the dry
    plains past the far wall. Keeps the map on the area (ring_rgba) for the core's seam blend."""
    region = area.region
    n = area.layout['region'].get('ring', {}).get('macro', 2048)
    grid = Grid(n, region.half)
    log(f'macro: the ring, {n} px over {2.0 * region.half:g} m')
    src_px = area.ring_grid.px
    h = resize(area.ring_h, n, region.half)
    gx, gy = np.gradient(area.ring_h.astype(np.float32), src_px)
    slope = resize(np.degrees(np.arctan(np.hypot(gx, gy))).astype(np.float32), n, region.half)
    del gx, gy
    h_fields = blur(area.ring_h, cells(2.4, src_px))
    lap = np.gradient(np.gradient(h_fields, axis=0), axis=0) + np.gradient(np.gradient(h_fields, axis=1), axis=1)
    convex = resize((-lap / src_px ** 2).astype(np.float32), n, region.half)
    ao = resize(area.ring_ao, n, region.half)
    dl = resize(area.ring_dl, n, region.half) if area.ring_dl is not None else np.full((n, n), 1e3, np.float32)
    canvas = Canvas(n)
    n_large = fbm_raster(grid, 38.0, seed=101, octaves=2)
    n_mid = fbm_raster(grid, 13.0, seed=102, octaves=2)
    n_patch = fbm_raster(grid, 7.0, seed=103, octaves=3)
    n_fine = fbm_raster(grid, 2.2, seed=104, octaves=2)
    canvas.rgb[:] = _mix(GREEN_DEEP, GREEN_MID, 0.5 + 0.8 * n_large)
    canvas.paint(GREEN_WARM, 0.55 * _ss(-0.1, 0.6, n_mid))
    rim_dry = (1.0 - _ss(1.0, 7.0, dl)) * _ss(-0.3, 0.4, n_mid + 0.5 * n_patch)
    dryness = np.clip(0.6 * _ss(0.004, 0.045, convex) + 0.4 * _ss(0.1, 0.7, n_patch) + 0.35 * rim_dry
                      - 0.4 * (1.0 - _ss(0.72, 0.95, ao)) + 0.15 * fbm_raster(grid, 4.0, seed=108, octaves=2), 0.0, 1.0)
    canvas.paint(DRY, 0.8 * _ss(0.35, 0.85, dryness))
    canvas.paint(STRAW, 0.55 * _ss(0.75, 1.0, dryness))
    del dryness, rim_dry
    # The ridges' slopes: a pine floor, thickest on the steeper ground well up from the valley.
    ridge = _ss(12.0, 24.0, slope) * _ss(3.0, 8.0, h) * _ss(0.0, 2.0, dl)
    canvas.paint(_mix(FOREST_FLOOR, MOSS, 0.5 + n_mid), 0.75 * ridge)
    canvas.paint(LITTER, 0.3 * ridge * _ss(0.2, 0.6, n_patch))
    if region.lip is not None:
        # The canyon: ochre soil and dry grass on its floor, sand and the river's water along it, the plains beyond.
        floor = 1.0 - _ss(-region.wall - 2.0, -region.wall + 2.0, dl)
        canvas.paint(_mix(DRY, SOIL, 0.45 + 0.7 * n_mid), 0.8 * floor)
        canvas.paint(STRAW, 0.35 * floor * _ss(0.0, 0.6, n_patch))
        canvas.paint(GREEN_DEEP, 0.35 * floor * _ss(0.3, 0.6, n_patch))  # scrub
        river = resize(region.river_distance, n, region.half)
        half = region.river_width * 0.5
        canvas.paint(_mix(GREEN_MID, GREEN_LUSH, 0.5 + n_mid), 0.7 * floor * (1.0 - _ss(half + 3.0, half + 22.0, river)))
        canvas.paint(_mix(SAND, PEBBLE_LIGHT, 0.5 + n_fine), floor * (1.0 - _ss(half, half + 4.0, river)), alpha=0.5)
        canvas.paint(_mix(POND_BED, _c(0x506a72), 0.6), floor * (1.0 - _ss(half - 1.0, half, river)), alpha=0.08)
    # Rock where it's steep: the ridges' bands, the escarpment's wall, the far wall; the lip's own edge.
    jitter = 7.0 * fbm_raster(grid, 3.0, seed=151, octaves=2)
    rockw = _ss(34.0, 46.0, slope + jitter)
    wall = getattr(region, 'wall', 12.0)
    lip = (1.0 - _ss(0.15, 1.1, dl + 0.4 * n_fine)) * _ss(-wall - 2.0, -wall, dl)  # not the canyon past the wall
    canvas.paint(_mix(SOIL_DARK, ROCK_DARK, 0.5 + n_fine), lip, alpha=0.7)
    shade = np.clip(0.55 + 0.9 * (ao - 0.8) + 0.35 * n_fine, 0.0, 1.0)
    canvas.paint(_mix(ROCK_DARK, ROCK_LIGHT, shade), rockw, alpha=1.0)
    canvas.paint(LICHEN, 0.35 * rockw * _ss(0.45, 0.6, fbm_raster(grid, 1.6, seed=152, octaves=2)))
    canvas.rgb *= (1.0 + 0.05 * n_fine)[..., None]
    rgba = np.empty((n, n, 4), dtype=np.float32)
    rgba[..., :3] = np.clip(canvas.rgb, 0.0, 1.0)
    rgba[..., 3] = np.clip(canvas.alpha, 1.0 / 255.0, 1.0)
    grade = area.layout.get('macro', {}).get('grade')
    if grade:
        _grade(rgba[..., :3], grade)
    # The ridges' faces past the core, as the core paints its own (area_faces.py), so they agree across the seam.
    faces = area_faces.ring_fields(area)
    if faces is not None:
        area_faces.paint(rgba, faces, fine=n_fine)
    area.ring_faces = faces = None
    area.ring_rgba = rgba
    _save(rgba, out_path)
    written = out_path
    if preview_dir:
        preview = os.path.join(preview_dir, 'ring_macro.png')
        p = min(n, 1024)
        small = rgba.reshape(p, n // p, p, n // p, 4).mean(axis=(1, 3))
        small[..., 3] = 1.0
        os.makedirs(os.path.dirname(preview), exist_ok=True)
        _save(small, preview)
        written += ' and ' + preview
    log(f'macro: wrote {written}')
