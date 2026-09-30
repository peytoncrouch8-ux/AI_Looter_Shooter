"""The tutorial island's macro color map, T_TutorialIslandMacro_BC.png: 4096 px over the map square (5 cm per pixel),
painted from the built island: meadow greens with sun-dried patches and clover, the forest floor, the farmyard, the
village square, the orchard rows, mowed stripes on the target meadow, dirt roads (ragged edges, wheel ruts, a grassy
middle, pebbles), footpaths, mud and sand at the water, rock on cliffs and steep banks, the drier plateau.

RGB is the ground's average color (sRGB): M_Terrain only modulates its brightness with the detail textures.
Alpha picks the detail texture: 0 grass and soil, 1 rock (roads about 0.4, footpaths 0.25, scree 0.6). It never
reaches exactly 0 (the lowest value is 1/255), so an importer that treats zero-alpha pixels as empty (Unreal's PNG
infill) keeps the color. The image has north up; UV 0 of the terrain tiles maps it: U = (Y + 10240) / 20480,
V = (X + 10240) / 20480 (Unreal cm).
"""
import math
import os

import numpy as np

import island_computed
from island_math import MAP_HALF, Grid, blur, fbm, fbm_raster, polyline_field, resize, signed_distance
from island_mesh import extend_nan
from island_shape import to_m

SIZE = 4096


def _c(value):
    return np.array([(value >> 16) & 255, (value >> 8) & 255, value & 255], dtype=np.float32) / 255.0


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

BUILDINGS = {  # kind: (length along its front, width) in meters, for the worn ground around and in front of it
    'Farmhouse': (9.0, 11.0), 'Barn': (12.0, 9.0), 'LogCabin': (6.5, 8.0), 'Cottage': (6.5, 8.0),
    'Outhouse': (1.3, 1.3), 'Well': (1.8, 1.8), 'Windmill': (3.0, 3.0), 'LookoutTower': (4.0, 4.0),
    'GunRack': (0.6, 2.0),
}
TRAILS = [  # worn trails (painted only), between placements (id) or points (layout meters)
    ('farmhouse', 'well'), ('barn', 'well'), ('log_cabin', 'outhouse'),
    ('cottage', (0.0, 0.0)), ('log_cabin', (0.0, 0.0)), ('gun_rack', (0.0, 0.0)),
]


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


def _place(island, ref):
    if isinstance(ref, tuple):
        return np.array(ref, dtype=np.float64)
    p = next(p for p in island.layout['placements'] if p['id'] == ref)
    return np.asarray(p['location'], dtype=np.float64) / 100.0


def paint(island, out_path, size=SIZE, log=print):
    grid = Grid(size)
    n = size
    log('macro: fields')
    h = resize(island.h, n)
    gx, gy = np.gradient(island.h.astype(np.float32), island.grid.px)
    slope = resize(np.degrees(np.arctan(np.hypot(gx, gy))).astype(np.float32), n)
    del gx, gy
    # Convexity at the scale of hills (about 4 m of blur), so small bumps don't count: > 0 on crowns and ridges.
    h1k = blur(resize(island.h, 1024), 12)
    lap = np.gradient(np.gradient(h1k, axis=0), axis=0) + np.gradient(np.gradient(h1k, axis=1), axis=1)
    convex = resize(-lap / (2.0 * MAP_HALF / 1024) ** 2, n)
    ao = resize(island.ao, n) if getattr(island, 'ao', None) is not None else np.ones((n, n), np.float32)
    edge = resize(island.edge, n)
    surface = island.water_surface()
    wet_1k = resize(np.where(np.isfinite(surface) & (surface > island.h), 1.0, 0.0).astype(np.float32), 1024)
    near_water = resize(np.clip(blur(wet_1k, 12) * 3.0, 0.0, 1.0), n)  # about 5 m around the water
    water = resize(np.nan_to_num(extend_nan(surface, 40), nan=-100.0), n)
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
    rim_dry = (1.0 - _ss(1.0, 7.0, edge)) * _ss(-0.3, 0.4, n_mid + 0.5 * n_patch)
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

    _forest(island, grid, canvas, n_mid, n_fine)
    _plateau_top(island, grid, canvas, h, n_fine)
    _range(island, grid, canvas)
    _orchard(island, grid, canvas)
    _yards(island, grid, canvas, n_fine)
    log('macro: roads')
    _roads(island, grid, canvas, h, n_fine, n_grain)
    log('macro: water and rock')
    _water(canvas, depth, near_water, n_mid, n_fine)
    road_near = 1.0 - _ss(1.5, 4.0, resize(np.minimum(island.road_gap, 50.0), n))
    _rock(island, grid, canvas, slope, ao, edge, depth, n_fine, n_grain, np.maximum(near_water, road_near))

    # Grain everywhere: a little brightness noise at 35 cm and 1 m, so no area is flat.
    canvas.rgb *= (1.0 + 0.05 * n_grain + 0.05 * n_fine)[..., None]
    rgba = np.empty((n, n, 4), dtype=np.float32)
    rgba[..., :3] = np.clip(canvas.rgb, 0.0, 1.0)
    rgba[..., 3] = np.clip(canvas.alpha, 1.0 / 255.0, 1.0)
    _save(rgba, out_path)
    preview = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
        out_path))))), 'Saved', 'ArtPreviews', 'Terrain', 'macro.png')
    small = rgba.reshape(1024, n // 1024, 1024, n // 1024, 4).mean(axis=(1, 3))
    small[..., 3] = 1.0
    os.makedirs(os.path.dirname(preview), exist_ok=True)
    _save(small, preview)
    log(f'macro: wrote {out_path} and {preview}')


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


def _zone(island, zone_id):
    z = next((z for z in island.layout['zones'] if z['id'] == zone_id), None)
    return None if z is None else to_m(z['polygon'])


def _zone_weight(island, grid, zone_id, soft, wobble):
    poly = _zone(island, zone_id)
    if poly is None:
        return None
    from island_math import catmull_rom
    sd = resize(signed_distance(island.grid, catmull_rom(poly, closed=True, step=0.5), soft + 8.0), grid.n)
    return 1.0 - _ss(-soft, soft, sd + wobble)


def _forest(island, grid, canvas, n_mid, n_fine):
    """The grove's floor: darker olive, leaf litter and moss in patches, clearings between."""
    wobble = 4.0 * fbm_raster(grid, 12.0, seed=111, octaves=2)
    w = _zone_weight(island, grid, 'forest', 6.0, wobble)
    if w is None:
        return
    litter = fbm_raster(grid, 3.5, seed=112, octaves=2)
    floor = _mix(FOREST_FLOOR, LITTER, 0.45 + 0.9 * litter)
    cover = w * (0.35 + 0.3 * _ss(-0.3, 0.4, n_mid))
    canvas.paint(floor, cover * _ss(-0.5, 0.2, litter + 0.3 * n_fine))
    canvas.paint(MOSS, 0.35 * w * _ss(0.35, 0.6, fbm_raster(grid, 2.0, seed=113, octaves=2)))


def _plateau_top(island, grid, canvas, h, n_fine):
    """Drier grass on the plateau, thin soil and rock breaking through."""
    p = island.plateau
    if p is None:
        return
    top = resize(p['rise'], grid.n)
    canvas.paint(PLATEAU_GRASS, 0.4 * _ss(0.85, 0.99, top))
    thin = _ss(0.25, 0.5, fbm_raster(grid, 5.0, seed=121, octaves=2) + 0.3 * n_fine) * _ss(0.9, 0.99, top)
    canvas.paint(SOIL, 0.55 * thin)
    outcrop = _ss(0.42, 0.52, fbm_raster(grid, 6.5, seed=122, octaves=3)) * _ss(0.9, 0.99, top)
    rock = _mix(ROCK, ROCK_LIGHT, 0.5 + n_fine)
    canvas.paint(rock, outcrop, alpha=1.0)


def _range(island, grid, canvas):
    """Mowed stripes across the target meadow (the player shoots along them, northward)."""
    w = _zone_weight(island, grid, 'range', 1.5, 0.0)
    if w is None:
        return
    x, _ = grid.mesh()
    stripes = 0.5 + 0.5 * np.sin(x * (2.0 * math.pi / 6.0)).astype(np.float32)
    canvas.paint(GREEN_WARM * 1.05, 0.16 * w * _ss(0.3, 0.7, stripes))
    canvas.paint(GREEN_MID * 0.96, 0.12 * w * (1.0 - _ss(0.3, 0.7, stripes)))
    for p in island.layout['placements']:
        if p['kind'] == 'TargetDummy':
            _disc(grid, canvas, np.asarray(p['location']) / 100.0, 1.3, SOIL, 0.8, 0.1)


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


def _orchard(island, grid, canvas):
    """Along each tree row: a darker mowed strip and bare soil around every trunk."""
    for row in island_computed.orchard_rows(island):
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


def _yards(island, grid, canvas, n_fine):
    """Worn ground around buildings: trampled grass all around, bare dirt in front, the farmyard and the village
    square, and trails between doors."""
    for p in island.layout['placements']:
        if p['kind'] not in BUILDINGS:
            continue
        length, width = BUILDINGS[p['kind']]
        c = np.asarray(p['location']) / 100.0
        yaw = math.radians(p.get('yaw', 0.0))
        fwd = np.array([math.cos(yaw), math.sin(yaw)])
        reach = max(length, width) * 0.5 + 3.0
        _disc(grid, canvas, c, reach, DRY, 0.45, None, soft=2.5)
        _disc(grid, canvas, c + fwd * (length * 0.5 + 1.4), max(1.2, width * 0.3), SOIL, 0.8, 0.12, soft=1.2)
    farm = np.mean([_place(island, k) for k in ('farmhouse', 'barn', 'well', 'spawn')], axis=0)
    _disc(grid, canvas, farm, 9.0, SOIL, 0.55, 0.15, soft=4.0)
    _disc(grid, canvas, farm, 6.0, DIRT, 0.5, 0.2, soft=3.0)
    _disc(grid, canvas, np.zeros(2), 10.0, DRY, 0.5, None, soft=4.0)
    _disc(grid, canvas, np.zeros(2), 7.5, DIRT_LIGHT, 0.75, 0.35, soft=2.5)
    for a, b in TRAILS:
        pts = np.array([_place(island, a), _place(island, b)])
        dist, _, _ = polyline_field(grid, pts, 2.0)
        near = np.isfinite(dist)
        w = np.zeros(dist.shape, np.float32)
        w[near] = 1.0 - _ss(0.25, 0.75, dist[near] + 0.2 * n_fine[near])
        canvas.paint(SOIL, 0.6 * w, alpha=0.1)


def _roads(island, grid, canvas, h, n_fine, n_grain):
    edge_noise = 0.35 * fbm_raster(grid, 2.5, seed=141, octaves=2)
    pebbles = fbm_raster(grid, 0.22, seed=142, octaves=1)
    tufts = fbm_raster(grid, 0.6, seed=143, octaves=2)
    curves = [dict(r) for r in island.roads]
    if island.ramp is not None:
        curves.append(dict(island.ramp, kind='ramp', id='ramp'))
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
    """The pond and creek: a dark bed under water, wet mud at the waterline, sand on some shores."""
    under = _ss(0.0, 0.08, depth)
    canvas.paint(_mix(MUD, POND_BED, _ss(0.1, 0.8, depth)), under, alpha=0.08)
    line = (1.0 - _ss(0.05, 0.35, -depth)) * (1.0 - under)
    sandy = _ss(0.1, 0.5, n_mid + 0.3 * n_fine)
    canvas.paint(_mix(MUD_WET, SAND, sandy), 0.9 * line, alpha=0.15)
    damp = (1.0 - _ss(0.3, 0.9, -depth)) * (1.0 - line) * (1.0 - under)
    canvas.paint(MUD, 0.35 * damp * (1.0 - sandy))


def _rock(island, grid, canvas, slope, ao, edge, depth, n_fine, n_grain, earthy):
    """Rock on cliffs and steep banks (by slope, with a ragged boundary), scree below the plateau's cliffs, and
    the island's edge where the turf rolls over into the rock wall."""
    jitter = 7.0 * fbm_raster(grid, 3.0, seed=151, octaves=2)
    # Beside water and roads, steep ground is earth (cut banks), not bare rock.
    rockw = _ss(34.0, 46.0, slope + jitter) * (1.0 - 0.85 * earthy)
    # Steep banks of soil first (the creek's ravine), rock on the steepest.
    bank = _mix(SOIL_DARK, SOIL, 0.55 + n_fine) * (1.0 - 0.12 * earthy[..., None])
    canvas.paint(bank, 0.8 * _ss(24.0, 34.0, slope + jitter) * _ss(-0.2, 0.3, -depth),
                 alpha=0.2)
    p = island.plateau
    if p is not None:
        sd = resize(p['sd'], grid.n)
        width = p['width']
        scree = (1.0 - _ss(width * 0.5, width * 0.5 + 3.5, sd + 1.2 * n_fine)) * _ss(width * 0.2, width * 0.5, sd)
        canvas.paint(_mix(SCREE, ROCK_DARK, 0.4 + 0.6 * n_grain), 0.85 * scree, alpha=0.65)
        # The cliff's lip: a band of bare rock just inside the top edge (inside = sd below -width / 2).
        inset = -(sd + width * 0.5) + 0.5 * n_fine
        lip = _ss(-0.4, 0.1, inset) * (1.0 - _ss(0.9, 1.8, inset))
        rockw = np.maximum(rockw, lip * _ss(0.2, 0.6, 0.5 + n_fine))
    rim = 1.0 - _ss(0.15, 1.1, edge + 0.4 * n_fine)
    canvas.paint(_mix(SOIL_DARK, ROCK_DARK, 0.5 + n_fine), rim, alpha=0.7)
    # Rock itself: lighter on crowns and faces, darker in crevices (the occlusion), a little lichen.
    shade = np.clip(0.55 + 0.9 * (ao - 0.8) + 0.35 * n_fine + 0.25 * n_grain, 0.0, 1.0)
    rock = _mix(ROCK_DARK, ROCK_LIGHT, shade)
    canvas.paint(rock, rockw, alpha=1.0)
    canvas.paint(LICHEN, 0.35 * rockw * _ss(0.45, 0.6, fbm_raster(grid, 1.6, seed=152, octaves=2)))
