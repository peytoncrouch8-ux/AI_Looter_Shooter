"""The PCG scatter mask, T_TutorialIslandScatter_BC.png: 512 px over the same square and UV mapping as the macro map
(U = (Y + 10240) / 20480, V = (X + 10240) / 20480, north up; 40 cm per pixel). Linear values (no sRGB curve), each
channel a density from 0 to 255, painted from the built island so it agrees with the terrain:

  R  trees: high in the forest grove, a few lone trees in the open meadow, none on roads and paths (plus 1.5 m),
     water, rock, building footprints, the village square, the farmyard, the target meadow and the orchard (its trees
     stand on the rows in layout_computed.json).
  G  grass: 1 in the meadows, 0 on road and path surfaces (soft 1 m edge), water, steep rock, footprints and bare
     dirt; about 0.4 under the forest canopy.
  B  flowers: a few big drifts across the meadows, never where there's no grass, sparse in the forest.
  A  pebbles and small rocks: a thin band just outside road edges, the feet of cliffs, the island's rim, the creek's
     banks.
Edges are softened by a few pixels, so density filters give natural transitions.
"""
import os

import numpy as np

from island_math import Grid, blur, catmull_rom, fbm_raster, polyline_field, resize, signed_distance
from island_shape import to_m

SIZE = 512


def _ss(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def _zone(island, grid, zone_id, soft=2.0):
    z = next((z for z in island.layout['zones'] if z['id'] == zone_id), None)
    if z is None:
        return np.zeros((grid.n, grid.n), np.float32)
    sd = resize(signed_distance(island.grid, catmull_rom(to_m(z['polygon']), closed=True, step=0.5), soft + 6.0),
                grid.n)
    return 1.0 - _ss(-soft, soft, sd)


def _disc(grid, center, radius, soft):
    x, y = grid.mesh()
    return 1.0 - _ss(radius, radius + soft, np.hypot(x - center[0], y - center[1]))


def paint(island, out_path, log=print):
    grid = Grid(SIZE)
    n = SIZE
    inside = resize(island.edge, n)
    land = _ss(0.0, 0.8, inside)
    gx, gy = np.gradient(island.h.astype(np.float32), island.grid.px)
    slope = resize(np.degrees(np.arctan(np.hypot(gx, gy))).astype(np.float32), n)
    h = resize(island.h, n)
    surface = island.water_surface()
    water = resize(np.nan_to_num(surface, nan=-100.0), n)
    wet = _ss(-0.25, 0.05, water - h)  # 1 in the water and on the waterline

    # Roads, paths and the ramp: the surface (for grass), the surface plus 1.5 m (for trees), the band beside it.
    surface_w = np.zeros((n, n), np.float32)
    clear_w = np.zeros((n, n), np.float32)
    shoulder = np.zeros((n, n), np.float32)
    curves = list(island.roads) + ([dict(island.ramp, kind='ramp')] if island.ramp else [])
    for road in curves:
        half = road['width'] * 0.5
        dist, _, _ = polyline_field(grid, road['pts'], half + 4.0)
        d = np.where(np.isfinite(dist), dist, 1e3)
        surface_w = np.maximum(surface_w, 1.0 - _ss(half - 0.5, half + 0.5, d))
        clear_w = np.maximum(clear_w, 1.0 - _ss(half + 1.5, half + 2.3, d))
        shoulder = np.maximum(shoulder, _ss(half - 0.2, half + 0.2, d) * (1.0 - _ss(half + 0.8, half + 1.4, d)))

    # Built-up ground: footprints (flat radius plus blend), the village square, the farmyard's bare dirt.
    built = np.zeros((n, n), np.float32)
    for f in island.footprints:
        built = np.maximum(built, _disc(grid, f['center'], f['radius'] + f['blend'] * 0.5, f['blend'] * 0.5))
    places = {p['id']: np.asarray(p['location']) / 100.0 for p in island.layout['placements']}
    farm = np.mean([places[k] for k in ('farmhouse', 'barn', 'well', 'spawn') if k in places], axis=0)
    yard = _disc(grid, farm, 6.5, 2.5)
    square = _disc(grid, np.zeros(2), 7.5, 2.0)
    bare = np.maximum.reduce([built, yard, square])

    # Rock: steep ground, the plateau's cliff faces and outcrops.
    rock = _ss(34.0, 42.0, slope)
    scree = np.zeros((n, n), np.float32)
    if island.plateau is not None:
        p = island.plateau
        sd = resize(p['sd'], n)
        w = p['width']
        rock = np.maximum(rock, _ss(-w * 0.5, -w * 0.3, sd) * (1.0 - _ss(w * 0.3, w * 0.5, sd)))
        scree = _ss(w * 0.2, w * 0.5, sd) * (1.0 - _ss(w * 0.5 + 2.0, w * 0.5 + 4.0, sd))

    forest = _zone(island, grid, 'forest', 4.0)
    orchard = _zone(island, grid, 'orchard', 1.5)
    range_zone = _zone(island, grid, 'range', 1.0)
    variety = 0.5 + 0.5 * fbm_raster(grid, 9.0, seed=201, octaves=2)
    lone = _ss(0.55, 0.8, 0.5 + 0.5 * fbm_raster(grid, 14.0, seed=202, octaves=2))

    trees = forest * (0.55 + 0.45 * variety) + (1.0 - forest) * 0.12 * lone
    if island.plateau is not None:
        trees = np.where(resize(island.plateau['rise'], n) > 0.9, trees * 0.6, trees)
    trees *= land * (1.0 - clear_w) * (1.0 - wet) * (1.0 - rock) * (1.0 - bare) * (1.0 - orchard) * (1.0 - range_zone)
    trees *= 1.0 - _ss(0.0, 1.0, _disc(grid, farm, 10.0, 4.0) + _disc(grid, np.zeros(2), 12.0, 4.0))

    grass = land * (1.0 - surface_w) * (1.0 - wet) * (1.0 - rock) * (1.0 - bare)
    grass *= 1.0 - 0.6 * forest * (0.6 + 0.4 * variety)
    grass *= 1.0 - 0.7 * scree

    drifts = _ss(0.3, 0.65, 0.5 + 0.5 * fbm_raster(grid, 26.0, seed=203, octaves=3))
    flowers = drifts * _ss(0.35, 0.8, grass) * (1.0 - 0.8 * forest)

    rim = (1.0 - _ss(0.5, 2.5, inside)) * land
    creek_bank = np.zeros((n, n), np.float32)
    if island.creek is not None:
        c = island.creek
        cd = resize(np.where(np.isfinite(c['dist']), c['dist'], 99.0).astype(np.float32), n)
        creek_bank = _ss(0.8, 1.4, cd) * (1.0 - _ss(3.5, 5.0, cd)) * (1.0 - wet)
    pebbles = np.maximum.reduce([0.7 * shoulder * (1.0 - bare * 0.5), scree, rim, 0.8 * creek_bank])
    pebbles *= land

    rgba = np.stack([trees, grass, flowers, pebbles], axis=-1)
    for k in range(4):
        rgba[..., k] = blur(np.clip(rgba[..., k], 0.0, 1.0), 1)
    rgba = np.clip(rgba, 0.0, 1.0)
    _save(rgba, out_path)
    # A preview for people: the four channels side by side (trees, grass / flowers, pebbles), north up.
    top = np.concatenate([rgba[..., 2], rgba[..., 3]], axis=1)
    bottom = np.concatenate([rgba[..., 0], rgba[..., 1]], axis=1)
    sheet = np.concatenate([bottom, top], axis=0)  # row 0 is the bottom of the image
    gray = np.stack([sheet, sheet, sheet, np.ones_like(sheet)], axis=-1)
    preview = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
        out_path))))), 'Saved', 'ArtPreviews', 'Terrain', 'scatter.png')
    _save(gray, preview)
    log(f'scatter: wrote {out_path} and {preview}')


def _save(rgba, path):
    """Linear bytes: the image is Non-Color, so Blender writes the values as they are."""
    import bpy
    n = rgba.shape[0]
    image = bpy.data.images.new('_scatter_save', rgba.shape[1], n, alpha=True)
    image.colorspace_settings.name = 'Non-Color'
    image.alpha_mode = 'STRAIGHT'
    image.pixels.foreach_set(np.ascontiguousarray(rgba, dtype=np.float32).ravel())
    os.makedirs(os.path.dirname(path), exist_ok=True)
    image.filepath_raw = path
    image.file_format = 'PNG'
    image.save()
    bpy.data.images.remove(image)
