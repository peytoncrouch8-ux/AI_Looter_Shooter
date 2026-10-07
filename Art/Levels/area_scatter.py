"""An area's PCG scatter mask, T_<Area>Scatter_BC.png: the layout's scatter cells over the same square and UV mapping as
the macro map (512 px at 40 cm on the tutorial island; layout_computed.json macroMap.scatterMap gives its size and the
square it covers), north up. Linear values (no sRGB curve), each channel a density from 0 to 255, painted from the
built area so it agrees with the terrain:

  R  trees: high in the forest groves, a few lone trees in the open meadow, none on roads and paths (plus 1.5 m),
     water, rock, building footprints, yards (the village square, the farmyard), the target meadow and the orchards
     (their trees stand on the rows in layout_computed.json).
  G  grass: 1 in the meadows, 0 on road and path surfaces (soft 1 m edge), water, steep rock, footprints and bare
     dirt; about 0.4 under the forest canopy.
  B  flowers: a few big drifts across the meadows, never where there's no grass, sparse in the forest.
  A  pebbles and small rocks: a thin band just outside road edges, the feet of cliffs, the island's rim, the creeks'
     banks.
Edges are softened by a few pixels, so density filters give natural transitions.

A grounded area has no rim: things grow on the core up to 10 m past the playable boundary (the macro map paints their
color beyond), with no pebble band along an edge; a gully's bed is gravel (pebbles, no grass or trees), and a pit's
floor grows little (sparse tufts at its walls' feet, bare grit in the middle, no flowers). Its ground steeper than any
flat layer reaches (FLAT_LIMIT degrees) carries steep layers in G, B and R instead (_steep: tufts and low bushes
climbing the regional ridges' faces, pines in their creases, area_faces.py), out past the boundary.

The scatter graph (Tools/Unreal/build_island_scatter.py) draws pebbles, rocks and boulders all from A: rocks and
boulders where it's over ROCK_KEEP, pebbles over 0.1. A layout's "scatter": {"roadside": "pebbles"} keeps the stones
off its roads: A is 0 on every road's surface (another road's band at a junction too), and within ROADSIDE_CLEAR m of
a road's edge it stays under ROCK_KEEP, so only pebbles line the roads and no rock or boulder stands in a street, on the
rail bed or across a junction. Without it ("stones", the default) rocks and boulders line the roads too. "steep": "bare"
keeps every stone off ground steeper than STEEP_BARE degrees (a cliff's face, the rock past a grounded area's
boundary), where one would hang on the slope; the graph's own slope filter judges by the triangle it lands on, which a
face's ledges can fool. Without it ("stones", the default) only that filter does.
"""
import os

import numpy as np

from area_math import Grid, blur, catmull_rom, cells, fbm_raster, polyline_field, signed_distance
from area_shape import to_m

# The scatter graph's Rocks and Boulders layers keep A over this (build_island_scatter.py); pebbles keep it over 0.1.
ROCK_KEEP = 0.15
PEBBLES_ONLY = ROCK_KEEP - 0.01  # A's most near a road with "roadside": "pebbles": pebbles but no rocks
ROADSIDE_CLEAR = 2.5    # meters past a road's edge that big stones keep clear of (a boulder's reach)
ROADSIDE = ('stones', 'pebbles')
STEEP_BARE = 44.0       # degrees: with "steep": "bare", ground steeper than this (no walking up it) gets no stones
STEEP = ('stones', 'bare')

# A pit's floor (the Sink): grass density at the walls' feet (in patches; the graph's grass keeps G over 0.12, so a
# sparse tuft) and in the middle (none), and the pebbles' density on it (grit; under ROCK_KEEP, so no rocks).
PIT_TUFTS, PIT_BARE, PIT_GRIT = 0.1, 0.04, 0.13

# Grounded areas' steep layers (_steep): the slopes (degrees) the graph's flat layers stop at (cos 0.85: grass,
# flowers, bushes) and its tree stands stop at (cos 0.8), and the range its steep layers take (cos 0.85 to 0.64).
FLAT_LIMIT, TREE_LIMIT, STEEP_TOP = 31.8, 36.9, 50.2
BOUNDARY_ROCK = 5.0     # meters past the boundary kept bare (the rock raised past its closed edges, area_boundary.py)
TUFT_REACH = 40.0       # meters past the boundary the slope tufts reach (they're culled at 45-55 m)
BUSH_REACH = 80.0       # and the low bushes (culled at 90 m); the creases' pines go out to the core's edge
# The steep layers in build_island_scatter.py (cell in cm, keep), for the counts the log gives.
STEEP_LAYERS = {'slope tufts': ('G', 170.0, 0.15), 'slope bushes': ('B', 350.0, 0.15),
                'crease pines': ('R', 700.0, 0.15)}

# What a yard (layout.json "yards") keeps clear, by its kind: bare ground (radius, soft edge) and no trees (radius,
# soft edge), in meters.
YARDS = {
    'farmyard': ((6.5, 2.5), (10.0, 4.0)),
    'square': ((7.5, 2.0), (12.0, 4.0)),
}


def _ss(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def _zones(area, grid, kind, soft=2.0):
    """1 inside the zones of a kind, fading out over soft meters across their edges; 0 everywhere without one."""
    weights = []
    for zone in area.zones_of(kind):
        sd = area.resized(signed_distance(area.grid, catmull_rom(to_m(zone['polygon']), closed=True, step=0.5),
                                          soft + 6.0), grid.n)
        weights.append(1.0 - _ss(-soft, soft, sd))
    return np.maximum.reduce(weights) if weights else np.zeros((grid.n, grid.n), np.float32)


def _disc(grid, center, radius, soft):
    x, y = grid.mesh()
    return 1.0 - _ss(radius, radius + soft, np.hypot(x - center[0], y - center[1]))


def _union(current, more):
    """The larger of two densities, where the first may be nothing yet."""
    return more if current is None else np.maximum(current, more)


def paint(area, out_path, preview_dir=None, log=print):
    n = area.sizes['scatter']
    grid = Grid(n, area.half)
    inside = area.resized(area.edge, n)
    land = _ss(0.0, 0.8, inside)  # island setting: nothing grows past the rim
    if area.setting == 'grounded':
        land = land * _ss(-10.5, -9.5, area.resized(area.play_edge, n))  # nor 10 m past the boundary
    gx, gy = np.gradient(area.h.astype(np.float32), area.grid.px)
    slope = area.resized(np.degrees(np.arctan(np.hypot(gx, gy))).astype(np.float32), n)
    h = area.resized(area.h, n)
    surface = area.water_surface()
    water = area.resized(np.nan_to_num(surface, nan=-100.0), n)
    wet = _ss(-0.25, 0.05, water - h)  # 1 in the water and on the waterline

    # Roads, paths and ramps: the surface (for grass), the surface plus 1.5 m (for trees), the band beside it, and
    # (roadside pebbles) the ground near enough for a boulder to reach the road.
    roadside = area.layout.get('scatter', {}).get('roadside', 'stones')
    if roadside not in ROADSIDE:
        raise ValueError(f"{area.path}: scatter.roadside must be {' or '.join(ROADSIDE)}, not {roadside!r}")
    steep = area.layout.get('scatter', {}).get('steep', 'stones')
    if steep not in STEEP:
        raise ValueError(f"{area.path}: scatter.steep must be {' or '.join(STEEP)}, not {steep!r}")
    surface_w = np.zeros((n, n), np.float32)
    clear_w = np.zeros((n, n), np.float32)
    shoulder = np.zeros((n, n), np.float32)
    near_road = np.zeros((n, n), np.float32)
    curves = list(area.roads) + [dict(r, kind='ramp') for r in area.ramps]
    for road in curves:
        half = road['width'] * 0.5
        dist, _, _ = polyline_field(grid, road['pts'], half + 4.0)
        d = np.where(np.isfinite(dist), dist, 1e3)
        surface_w = np.maximum(surface_w, 1.0 - _ss(half - 0.5, half + 0.5, d))
        clear_w = np.maximum(clear_w, 1.0 - _ss(half + 1.5, half + 2.3, d))
        shoulder = np.maximum(shoulder, _ss(half - 0.2, half + 0.2, d) * (1.0 - _ss(half + 0.8, half + 1.4, d)))
        near_road = np.maximum(near_road, 1.0 - _ss(half + ROADSIDE_CLEAR, half + ROADSIDE_CLEAR + 0.4, d))

    # Built-up ground: footprints (flat radius plus blend) and the yards' bare dirt (a farmyard, a village square).
    built = np.zeros((n, n), np.float32)
    for f in area.footprints:
        built = np.maximum(built, _disc(grid, f['center'], f['radius'] + f['blend'] * 0.5, f['blend'] * 0.5))
    bare_yards, tree_clear = [], None
    for yard in area.layout.get('yards', []):
        center = area.yard_center(yard)
        if center is None:
            continue
        (bare_r, bare_soft), (clear_r, clear_soft) = YARDS[yard['kind']]
        bare_yards.append(_disc(grid, center, bare_r, bare_soft))
        clear = _disc(grid, center, clear_r, clear_soft)
        tree_clear = clear if tree_clear is None else tree_clear + clear
    bare = np.maximum.reduce([built] + bare_yards)

    # Rock: steep ground, the plateaus' cliff faces and outcrops.
    rock = _ss(34.0, 42.0, slope)
    scree = None
    for p in area.plateaus:
        sd = area.resized(p['sd'], n)
        w = p['width']
        rock = np.maximum(rock, _ss(-w * 0.5, -w * 0.3, sd) * (1.0 - _ss(w * 0.3, w * 0.5, sd)))
        scree = _union(scree, _ss(w * 0.2, w * 0.5, sd) * (1.0 - _ss(w * 0.5 + 2.0, w * 0.5 + 4.0, sd)))
    if scree is None:
        scree = np.zeros((n, n), np.float32)

    forest = _zones(area, grid, 'forest', 4.0)
    orchard = _zones(area, grid, 'orchard', 1.5)
    range_zone = _zones(area, grid, 'range', 1.0)
    variety = 0.5 + 0.5 * fbm_raster(grid, 9.0, seed=201, octaves=2)
    lone = _ss(0.55, 0.8, 0.5 + 0.5 * fbm_raster(grid, 14.0, seed=202, octaves=2))

    trees = forest * (0.55 + 0.45 * variety) + (1.0 - forest) * 0.12 * lone
    for p in area.plateaus:
        trees = np.where(area.resized(p['rise'], n) > 0.9, trees * 0.6, trees)
    trees *= land * (1.0 - clear_w) * (1.0 - wet) * (1.0 - rock) * (1.0 - bare) * (1.0 - orchard) * (1.0 - range_zone)
    if tree_clear is not None:
        trees *= 1.0 - _ss(0.0, 1.0, tree_clear)

    grass = land * (1.0 - surface_w) * (1.0 - wet) * (1.0 - rock) * (1.0 - bare)
    grass *= 1.0 - 0.6 * forest * (0.6 + 0.4 * variety)
    grass *= 1.0 - 0.7 * scree

    drifts = _ss(0.3, 0.65, 0.5 + 0.5 * fbm_raster(grid, 26.0, seed=203, octaves=3))
    flowers = drifts * _ss(0.35, 0.8, grass) * (1.0 - 0.8 * forest)

    rim = (1.0 - _ss(0.5, 2.5, inside)) * land  # island setting
    if area.setting == 'grounded':
        rim = np.zeros((n, n), np.float32)
    gravel = np.zeros((n, n), np.float32)
    for g in getattr(area, 'gullies', []):
        dist, _, _ = polyline_field(grid, g['pts'], g['half'] + 2.0)
        gravel = np.maximum(gravel, 1.0 - _ss(g['half'] - 0.5, g['half'] + 0.5, np.where(np.isfinite(dist), dist,
                                                                                            1e3)))
    pit_grit = np.zeros((n, n), np.float32)
    for p in getattr(area, 'pits', []):
        # A pit's floor (the Sink: a spider pit of webs, blocks and coffins) grows little: sparse tufts in patches at
        # the walls' feet, bare grit in the middle (pebbles only), no flowers on the floor or the walls.
        rise = area.resized(p['rise'], n)
        floor = _ss(0.9, 0.99, rise)
        from_wall = -area.resized(p['sd'], n) - p['width'] * 0.5  # meters in from the wall's foot
        at_foot = (1.0 - _ss(1.5, 4.0, from_wall)) * _ss(0.45, 0.7, variety)
        grass = grass * (1.0 - floor) + floor * np.minimum(grass, PIT_TUFTS * at_foot + PIT_BARE)
        flowers = flowers * (1.0 - _ss(0.4, 0.8, rise))
        trees = trees * (1.0 - floor)
        pit_grit = np.maximum(pit_grit, PIT_GRIT * floor)
    grass = grass * (1.0 - gravel)
    flowers = flowers * (1.0 - gravel)
    trees = trees * (1.0 - gravel)
    creek_bank = None
    for c in area.creeks:
        cd = area.resized(np.where(np.isfinite(c['dist']), c['dist'], 99.0).astype(np.float32), n)
        creek_bank = _union(creek_bank, _ss(0.8, 1.4, cd) * (1.0 - _ss(3.5, 5.0, cd)) * (1.0 - wet))
    if creek_bank is None:
        creek_bank = np.zeros((n, n), np.float32)
    pebbles = np.maximum.reduce([0.7 * shoulder * (1.0 - bare * 0.5), scree, rim, 0.8 * creek_bank, 0.9 * gravel,
                                 pit_grit])
    pebbles *= land

    steep_note = None
    if area.setting == 'grounded':
        trees, grass, flowers, steep_note = _steep(area, grid, h, slope, trees, grass, flowers,
                                                   (1.0 - surface_w) * (1.0 - clear_w * 0.5) * (1.0 - wet)
                                                   * (1.0 - bare) * (1.0 - gravel))
    rgba = np.stack([trees, grass, flowers, pebbles], axis=-1)
    soften = cells(0.4, grid.px)
    for k in range(4):
        rgba[..., k] = blur(np.clip(rgba[..., k], 0.0, 1.0), soften)
    rgba = np.clip(rgba, 0.0, 1.0)
    if roadside == 'pebbles':
        # After the softening, so no edge creeps back over the line: nothing on the roads, pebbles only beside them.
        cap = 1.0 - (1.0 - PEBBLES_ONLY) * near_road
        rgba[..., 3] = np.minimum(rgba[..., 3], cap) * (1.0 - _ss(0.0, 0.5, surface_w))
    if steep == 'bare':
        # The steepest the ground gets within a cell and its neighbors (a thin face mustn't average away).
        steepest = np.maximum.reduce([np.roll(np.roll(slope, di, 0), dj, 1) for di in (-1, 0, 1) for dj in (-1, 0, 1)])
        rgba[..., 3] *= 1.0 - _ss(STEEP_BARE - 6.0, STEEP_BARE, steepest)
    _save(rgba, out_path)
    written = out_path
    if preview_dir:
        # A preview for people: the four channels side by side (trees, grass / flowers, pebbles), north up.
        top = np.concatenate([rgba[..., 2], rgba[..., 3]], axis=1)
        bottom = np.concatenate([rgba[..., 0], rgba[..., 1]], axis=1)
        sheet = np.concatenate([bottom, top], axis=0)  # row 0 is the bottom of the image
        gray = np.stack([sheet, sheet, sheet, np.ones_like(sheet)], axis=-1)
        preview = os.path.join(preview_dir, 'scatter.png')
        _save(gray, preview)
        written += ' and ' + preview
    log(f'scatter: wrote {written}' + (f'; {steep_note}' if steep_note else ''))


def _steep(area, grid, h, slope, trees, grass, flowers, clear):
    """A grounded area's steep layers. Ground steeper than FLAT_LIMIT, which none of the graph's flat layers reach,
    carries them in the same channels: G slope tufts (dry grass climbing the ridges' faces, sparse on other steep
    banks), B low bushes, and R, over TREE_LIMIT (past the tree stands' reach), pines in the faces' creases
    (area_faces.py: densest at a face's foot and in its creases, thinning upward, never on its rock bands). Past the
    boundary R carries the creases' trees on gentler ground too, where the graph's own stands grow them. Never on
    roads, water, bare ground, the rock past the closed edges, cliff faces (over STEEP_TOP) or the core's last meters.
    Returns the channels and a note of about how many each steep layer places."""
    import area_faces
    n = grid.n
    faces = area_faces.fields(area, grid, h)
    if faces is None:
        return trees, grass, flowers, None
    tufts, bushes, pines = area_faces.scatter(faces, n)
    del faces
    past = signed_distance(grid, area.boundary, TUFT_REACH + BUSH_REACH) if area.boundary is not None else \
        np.full((n, n), -1e3, np.float32)  # meters past the playable boundary (negative inside)
    keep = clear * (1.0 - _ss(STEEP_TOP - 3.0, STEEP_TOP, slope)) * _ss(1.0, 5.0, area.resized(area.edge, n))
    keep *= 1.0 - _ss(-0.5, 0.5, past) * (1.0 - _ss(BOUNDARY_ROCK, BOUNDARY_ROCK + 2.0, past))
    steep = _ss(FLAT_LIMIT - 1.0, FLAT_LIMIT + 1.0, slope)
    tufts = steep * keep * tufts * (1.0 - _ss(TUFT_REACH - 6.0, TUFT_REACH, past))
    bushes = steep * keep * bushes * (1.0 - _ss(BUSH_REACH - 8.0, BUSH_REACH, past))
    pines = keep * pines * _ss(1.0, 3.0, past)
    grass = grass * (1.0 - steep) + tufts
    flowers = flowers * (1.0 - steep) + bushes
    trees = np.maximum(trees * (1.0 - _ss(TREE_LIMIT - 1.0, TREE_LIMIT + 1.5, slope)), pines)
    # About how many each places: one draw per cell, kept with chance 1 - keep / density, on ground in its range.
    cell_area = grid.px * grid.px
    counts = []
    for (name, (_, cell, cut)), density, lo in zip(STEEP_LAYERS.items(), (tufts, bushes, pines),
                                                   (FLAT_LIMIT, FLAT_LIMIT, TREE_LIMIT)):
        chance = np.clip(1.0 - cut / np.maximum(density, 1e-6), 0.0, 1.0) * ((slope > lo) & (slope < STEEP_TOP))
        counts.append(f'{float(chance.sum()) * cell_area / (cell / 100.0) ** 2:.0f} {name}')
    # The creases' trees on gentler ground past the boundary come from the graph's stands (cells of 6 m, keep 0.12).
    gentler = np.clip(1.0 - 0.12 / np.maximum(pines, 1e-6), 0.0, 1.0) * (slope <= TREE_LIMIT)
    counts.append(f'{float(gentler.sum()) * cell_area / 36.0:.0f} crease trees in the stands')
    return trees, grass, flowers, 'steep layers: about ' + ', '.join(counts)


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
