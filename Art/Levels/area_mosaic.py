"""A grounded area's late-summer ground (numpy only): the meadow of its macro maps (area_macro.py) painted as the mosaic
dry country really is, in place of the island's even green meadow graded gold, for a layout whose "macro" block asks
for it ("mosaic": "lateSummer"; Ransom's Rest). The island's meadow, graded gold, read as one flat ochre-brown from the
lookout and on foot (the user, 2026-10-08): real late-summer land is a patchwork of grass tones keyed to water and
ground, which is what gives it form from far off.

What it paints, each from world position and the heights only, so the core's map and the ring's agree at the seam:
- moisture: hollows and the low ground wetter, rises and crowns drier (the heights against their surroundings at two
  scales), slopes facing north and east (away from the afternoon sun) wetter, the high ground drier, the lowlands past
  the ridges wetter still; the water's banks, the dry gullies' beds, under living trees, and the layout's zones (a
  farm a little greener, the town and the depot dusty) shift it; large and middle noise breaks it into patches;
- the grass's color by moisture, from bleached straw on the driest rises through gold and olive gold to olive and the
  green of the creek bottoms; sage grey-green and coppery patches beside them at the same moisture (different grasses),
  so the bands of moisture don't read as contours;
- soft light and dark over a hundred meters and more, like cloud shadow, and a little lighter on rises and darker in
  hollows, for the land's form from the lookout;
- wildflower drifts as specks of color (larkspur blue, purple, white, yellow) where the scatter's flowers drift
  (area_scatter.py's own drift noise), bare dry soil in patches on the driest ground, and darker, greener shade under
  the living tree stands;
- the hayfields (layout openGround polygons named in "hayfields"): pale stubble mowed in swaths, raked windrows, an
  uncut headland round each field;
- on the ring: a darker forest floor under the far trees (area_fartrees.py), so the woods read as masses from afar.

Colors are the map's own (no season's grade follows them) and chosen for the golden hour's warm light, which pushes
every albedo toward orange: the golden grass is painted a little yellower than it is meant to look, and the greens keep
more green than red so they still read green in that light.
"""
import math

import numpy as np

from area_math import Grid, blur, catmull_rom, cells, fbm_raster, polyline_field, resize, sample, signed_distance
from area_shape import to_m


def _c(value):
    return np.array([(value >> 16) & 255, (value >> 8) & 255, value & 255], dtype=np.float32) / 255.0


def _ss(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=np.float32) - e0) / (e1 - e0), 0.0, 1.0)
    return (t * t * (3.0 - 2.0 * t)).astype(np.float32)


# The colors are albedo for the game's golden hour, measured on the tour's shots (2026-10-08, Medium): sunlit ground
# shows about (0.72, 0.47, 0.24) of its albedo in linear red, green and blue (a white wall reads #d2aa6d, the old
# golden meadow #908155 reads #7c5a29). So an albedo with red and green about equal shows gold, and only one with far
# more green than red shows olive or green: each color below is noted with what it shows in that light.
# The grass by moisture, driest first: bleached straw on the rises, gold over most of the open ground, olive gold and
# olive where it holds water, the green of the creek bottoms.
STRAW_PALE = _c(0xcdc9a0)    # shows #b08d51, light tan gold
STRAW = _c(0xc0bc88)         # #a3843f
GOLD = _c(0xb2b27a)          # #9c8041, gold
OLIVE_GOLD = _c(0x9ea66a)    # #897536
OLIVE = _c(0x86965f)         # #736a2f, olive
GREEN = _c(0x6c8a50)         # #5c5f25
GREEN_WET = _c(0x577d45)     # #4a571e, green
MOISTURE = ((0.0, STRAW_PALE), (0.2, STRAW), (0.38, GOLD), (0.55, OLIVE_GOLD), (0.7, OLIVE), (0.85, GREEN),
            (1.0, GREEN_WET))
SAGE = _c(0x8f9e8c)          # grey-olive (#7a7350): sage and blue bunchgrass among the meadow
COPPER = _c(0xb09a72)        # copper (#966a3c): late summer's reddening grass on dry ground
SOIL_DRY = _c(0xa89070)      # tan-brown (#8a6232): bare dry soil
SHADE = _c(0x5f7149)         # dark olive (#4f4d22): the ground under living trees
FOREST_FLOOR = _c(0x4a573a)  # near black-olive (#3d3918): the ring's woods, needles and shade
STUBBLE, WINDROW = _c(0xd0c9a0), _c(0xada171)  # the hayfields: pale mowed stubble (#b5915a), raked rows (#957238)
# The flowers' colors by their kind noise: white, larkspur blue, purple, yellow (blue shows only as a cool violet-grey
# in that light, so it's painted strong).
FLOWERS = ((-1.0, _c(0xf0eee2)), (-0.3, _c(0xf0eee2)), (-0.1, _c(0x7d8fd8)), (0.18, _c(0x7d8fd8)),
           (0.3, _c(0xb08ad0)), (0.42, _c(0xf0e070)), (1.0, _c(0xf0e070)))

# The ridges' faces (area_faces.py's palette) in the same light: grass in patches, gold on the slopes facing the sun
# and olive on those turned from it, ochre and rusty soil, banded rock in cream, grey, rust and dark layers, sage
# scrub, dark needle floors in the creases where the pines stand.
FACES = dict(
    grass=_c(0xb2ad78), straw=_c(0xc4bd8c), grass_cool=_c(0x8f9a64),        # #9a7c40, #a8854a, olive #776b30
    soil=_c(0xa8957a), soil_dark=_c(0x85765f), soil_red=_c(0xa8806c),       # ochre #8d663a, #6e4e2c, rust #8e5838
    rock=_c(0x9f9888), rock_light=_c(0xbdb6a5), rock_cool=_c(0x9a9da0), rock_dark=_c(0x6c675e),
    ledge=_c(0x3f3b35), scree=_c(0xb0a998), scree_dark=_c(0x8f887a),
    scrub=_c(0x8a977e), scrub_dark=_c(0x55613f), crease=_c(0x5f6745),      # sage #737048, juniper #474520
    pines=_c(0x3f4c33),                                                      # the far faces' pine stands, #312e14
    cover=True,
    strata=((-1.0, _c(0xbab29f)), (-0.35, _c(0xbab29f)), (-0.12, _c(0x9f9888)), (0.1, _c(0xb08a76)),
            (0.25, _c(0x8f8a8f)), (0.4, _c(0x6c675e)), (1.0, _c(0x6c675e))))  # cream, rock, rust, grey, dark

WORK_PX = 0.4           # meters per cell the broad fields are worked out at (or coarser on a coarse map)
PAD = 70.0              # meters past the core square the core's broad fields look (the ring's heights there)
TPI = (35.0, 9.0)       # meters: the two scales a point's height is weighed against its surroundings
FLOWER_DRIFT = (26.0, 203)  # area_scatter.py's drift noise (wavelength, seed): the macro's flowers drift with it
CROWN = {'SM_FarPine_A': 2.3, 'SM_FarPine_B': 2.5, 'SM_FarBroadleaf': 3.4}  # the far trees' crowns' radius (m)
# What each term adds to the moisture (0 dry, 1 wet), and where it starts.
# Tuned so the valley's open meadow comes out about a sixth straw, a quarter gold, a third olive gold, a sixth olive and
# the rest green (the log prints the shares): mostly golden, with the moisture's patchwork plain from the lookout.
WET = dict(start=0.34, hollow=0.17, rise=-0.15, cool=0.15, high=-0.2, low=0.18, large=0.68, mid=0.27, water=0.4,
           gully=0.2, shade=0.22, woods=0.22)
SWATH, WINDROW_STEP = 3.6, 7.2  # meters: a mower's swath, and the raked rows' spacing
HEADLAND = 3.0          # meters of uncut grass round a hayfield's edge
FOOT = 3.5              # meters out from a rock's foot its contact shadow reaches
GRASS_THIN = 0.3        # the grass mask on the driest ground (the graph keeps a patch where mask x random >= 0.12)


def spec(area):
    """The layout's late-summer settings (its "macro" block), or None when it doesn't ask for the mosaic."""
    macro = area.layout.get('macro', {})
    if macro.get('mosaic') != 'lateSummer':
        return None
    return macro


def _ramp(t, stops):
    xs = np.array([s for s, _ in stops], dtype=np.float32)
    cs = np.array([c for _, c in stops], dtype=np.float32)
    out = np.empty(t.shape + (3,), np.float32)
    for k in range(3):
        out[..., k] = np.interp(t, xs, cs[:, k])
    return out


def _mix(rgb, color, w):
    w = np.clip(w, 0.0, 1.0).astype(np.float32)[..., None]
    color = np.asarray(color, dtype=np.float32)
    rgb += (color - rgb) * w


def _terrain(grid, h, base):
    """The ground's own terms on a working grid: hollows and rises at two scales, how much a slope faces away from the
    afternoon sun (north and east), the height over the valley's floor (base) and the lowlands (the floor below 0)."""
    px = grid.px
    hb = blur(h, cells(2.0, px))
    gx, gy = np.gradient(hb, px)  # layout axes: x north, y east
    g = np.hypot(gx, gy)
    slope = np.degrees(np.arctan(g)).astype(np.float32)
    cool = ((-0.8 * gx - 0.45 * gy) / np.maximum(g, 1e-4) * _ss(4.0, 18.0, slope)).astype(np.float32)
    del hb, gx, gy, g
    # A point against the gentle ground round it only: a cliff or a ridge's face beside the valley floor would make the
    # whole floor a hollow.
    gentle = 1.0 - _ss(16.0, 28.0, slope)
    big = h - _gentle_mean(h, gentle, cells(TPI[0], px))
    small = h - _gentle_mean(h, gentle, cells(TPI[1], px))
    hollow = 0.7 * _ss(0.2, -2.0, big) + 0.3 * _ss(0.1, -0.7, small)
    rise = 0.7 * _ss(0.2, 2.0, big) + 0.3 * _ss(0.1, 0.7, small)
    del big, small, gentle
    high = _ss(6.0, 30.0, h - base)
    low = _ss(-6.0, -25.0, base)
    return dict(hollow=hollow, rise=rise, cool=cool, high=high, low=low, slope=slope)


def _gentle_mean(h, weight, radius):
    """The heights' mean round each cell (a blur of radius cells) over the weighted ground only."""
    return blur(h * weight, radius) / np.maximum(blur(weight, radius), 1e-3)


def _noise(grid):
    """The broad noises (fractal noise: mostly within -0.4..0.4, about 0.2 either way): moisture's large and middle
    patches, the grasses' kinds (sage, copper), the soft light and dark, the flowers' drifts and kinds, the bare soil's
    patches and the tussocks."""
    return dict(large=fbm_raster(grid, 110.0, seed=801, octaves=3), mid=fbm_raster(grid, 32.0, seed=802, octaves=2),
                sage=fbm_raster(grid, 48.0, seed=811, octaves=2) + 0.4 * fbm_raster(grid, 14.0, seed=812, octaves=2),
                copper=fbm_raster(grid, 65.0, seed=813, octaves=2),
                value=0.35 * fbm_raster(grid, 170.0, seed=821, octaves=2) + 0.2 * fbm_raster(grid, 55.0, seed=822,
                                                                                         octaves=2),
                drift=_ss(0.3, 0.65, 0.5 + 0.5 * fbm_raster(grid, FLOWER_DRIFT[0], seed=FLOWER_DRIFT[1], octaves=3)),
                kind=fbm_raster(grid, 70.0, seed=831, octaves=2),
                bare=fbm_raster(grid, 5.0, seed=841, octaves=2),
                tussock=fbm_raster(grid, 6.0, seed=842, octaves=2))


def _moisture(t, nz, extra=0.0):
    w = WET
    m = (w['start'] + w['hollow'] * t['hollow'] + w['rise'] * t['rise'] + w['cool'] * t['cool'] + w['high'] * t['high']
         + w['low'] * t['low'] + w['large'] * nz['large'] + w['mid'] * nz['mid'] + extra)
    return m.astype(np.float32)


def _broad(m, t, nz):
    """The broad weights from moisture and the noises, on the working grid: sage and copper's shares, the light, the
    flowers' and the bare soil's (before their speckle)."""
    mid_wet = _ss(0.4, 0.55, m) * (1.0 - _ss(0.8, 0.95, m))
    sage = 0.6 * _ss(0.05, 0.3, nz['sage']) * mid_wet
    dryish = 1.0 - _ss(0.42, 0.55, m)
    copper = 0.45 * _ss(0.05, 0.3, nz['copper']) * dryish
    value = 1.0 + nz['value'] + 0.05 * t['rise'] - 0.06 * t['hollow']
    flowers = nz['drift'] * _ss(0.1, 0.3, m)
    bare = _ss(0.2, 0.4, nz['bare']) * _ss(0.22, 0.02, m)
    return dict(sage=sage, copper=copper, value=value, flowers=flowers, bare=bare, kind=nz['kind'],
                tussock=0.35 * nz['tussock'])


def _colors(m, b, fine, grain):
    """The ground's color and alpha on the map's grid from the moisture and the broad weights (all on it), and that
    map's own fine noise (fine about a meter, grain finer; None on a coarse map)."""
    jag = 0.0 if fine is None else fine
    speck = 0.0 if grain is None else grain
    # Tussocks and swales a few meters across: the grass a shade wetter or drier, so no stretch of one tone is flat.
    rgb = _ramp(np.clip(m + b['tussock'] + 0.06 * jag + 0.03 * speck, 0.0, 1.0), MOISTURE)
    _mix(rgb, SAGE, b['sage'] * (0.8 + 0.2 * jag))
    _mix(rgb, COPPER, b['copper'])
    alpha = np.zeros(m.shape, np.float32)
    bare = b['bare'] * _ss(-0.2, 0.3, jag + 0.5 * speck)
    _mix(rgb, SOIL_DRY, 0.55 * bare)
    alpha += (0.12 - alpha) * 0.55 * bare
    # The flowers: specks (a few tens of centimeters), so close up they're dots of color in the grass and far off a
    # faint tint.
    flecks = _ss(0.15, 0.45, speck + 0.25 * jag) if grain is not None else 0.35
    _mix(rgb, _ramp(b['kind'], FLOWERS), 0.22 * b['flowers'] * flecks)
    rgb *= b['value'][..., None]
    return rgb, alpha


def _up(field, grid, work):
    return field if field.shape[0] == grid.n else resize(field, grid.n, work.half).astype(np.float32)


def _zones(area, grid, settings):
    """The layout's zone shifts (settings "zones": {kind: moisture added}), softened over about 12 m and frayed, so no
    zone's outline shows in the grass."""
    total = np.zeros((grid.n, grid.n), np.float32)
    shifts = settings.get('zones', {})
    fray = None
    for zone in area.layout.get('zones', []):
        add = shifts.get(zone.get('kind', zone['id']))
        if not add:
            continue
        if fray is None:
            fray = 6.0 * fbm_raster(grid, 20.0, seed=861, octaves=2)
        sd = signed_distance(grid, catmull_rom(to_m(zone['polygon']), closed=True, step=0.5), 24.0)
        total += add * (1.0 - _ss(-10.0, 12.0, sd + fray))
    return total


def _feet(area, grid):
    """1 at the foot of each outcrop (layout obstacles), fading over FOOT m out from it; among a boulder group's
    stones, half that in patches (the group's outline is only where they lie, not a stone)."""
    out = np.zeros((grid.n, grid.n), np.float32)
    patches = None
    for ob in area.layout.get('obstacles', []):
        kind = ob.get('kind')
        if kind not in ('outcrop', 'boulders') or 'polygon' not in ob:
            continue
        sd = signed_distance(grid, to_m(ob['polygon']), FOOT + 2.0)
        if kind == 'outcrop':
            out = np.maximum(out, 1.0 - _ss(-0.5, FOOT, sd))
        else:
            if patches is None:
                patches = _ss(-0.1, 0.4, fbm_raster(grid, 4.0, seed=871, octaves=2))
            out = np.maximum(out, 0.5 * patches * (1.0 - _ss(-1.0, 1.5, sd)))
    return out


def grass_density(area, grid):
    """The scatter's grass (area_scatter.py) by the mosaic's moisture on a grid over the map square: thin on the
    bleached rises (about two thirds of the patches there), full from the gold on; None without a mosaic."""
    if spec(area) is None:
        return None
    if getattr(area, 'mosaic_moisture', None) is None:
        raise RuntimeError('the scatter mask follows the macro map: paint it first (area_macro.paint)')
    m, half = area.mosaic_moisture
    x, y = grid.mesh()
    return (GRASS_THIN + (1.0 - GRASS_THIN) * _ss(0.08, 0.38, sample(m, x, y, half))).astype(np.float32)


def _shade(area, grid, settings):
    """Under the living tree stands (settings "shade": obstacle ids, and every forest and orchard zone): 1 under the
    crowns, easing out 4 m past them; and how far under (for the darker color)."""
    near = np.zeros((grid.n, grid.n), np.float32)
    under = np.zeros((grid.n, grid.n), np.float32)
    polygons = [to_m(ob['polygon']) for ob in area.layout.get('obstacles', [])
                if ob['id'] in settings.get('shade', []) and 'polygon' in ob]
    polygons += [catmull_rom(to_m(z['polygon']), closed=True, step=0.5) for z in area.layout.get('zones', [])
                 if z.get('kind') in ('forest', 'orchard')]
    for poly in polygons:
        sd = signed_distance(grid, poly, 10.0)
        near = np.maximum(near, 1.0 - _ss(-2.0, 4.0, sd))
        under = np.maximum(under, 1.0 - _ss(-3.0, 1.0, sd))
    return near, under


def _onto(field, half, grid):
    """A field over a square of half side half (m) sampled onto a grid over a smaller square, row block by row block."""
    out = np.empty((grid.n, grid.n), dtype=np.float32)
    for a in range(0, grid.n, 256):
        x, y = grid.mesh(a, min(grid.n, a + 256))
        out[a:a + 256] = sample(field, x, y, half)
    return out


def core(area, grid, fine, grain, log=print):
    """The core map's ground (rgb, alpha) on its grid; fine and grain are that map's own noise (about 1 m and 35 cm)."""
    settings = spec(area)
    f = core_fields(area, log)
    m, b, under, half = f['m'], f['broad'], f['under'], f['half']
    del f
    # Onto the map's grid.
    up = {k: _onto(v, half, grid) for k, v in b.items()}
    m_up = _onto(m, half, grid)
    under_up = _onto(under, half, grid)
    del b, m, under
    rgb, alpha = _colors(m_up, up, fine, grain)
    del up, m_up
    _mix(rgb, SHADE, 0.5 * under_up * (0.8 + 0.2 * fine))
    rgb *= (1.0 - 0.1 * under_up)[..., None]
    del under_up
    _hayfields(area, grid, rgb, alpha, settings, fine)
    return rgb, alpha


def core_fields(area, log=print):
    """The core's broad fields on its working grid (a little wider than the map square): the moisture (m), the broad
    weights (broad), the shade under the living trees (under), the grid (work) and its half side (half)."""
    settings = spec(area)
    half = area.half + PAD
    n = int(round(2.0 * half / WORK_PX / 8.0)) * 8
    work = Grid(n, half)
    x, y = work.mesh()
    # The heights past the square are the ring's (the regional field), so the broad terms see over the core's edge.
    hw = np.asarray(sample(area.ring_h, x, y, area.region.half) if getattr(area, 'ring_h', None) is not None
                    else sample(area.h, x, y, area.half), dtype=np.float32)
    inside = (np.abs(x) < area.half) & (np.abs(y) < area.half)
    hw[inside] = sample(area.h, x[inside], y[inside], area.half)
    base = area.region.at(x.ravel(), y.ravel())['base'].reshape(x.shape).astype(np.float32)
    t = _terrain(work, hw, base)
    del base
    # The water's banks (about 8 m round it), the dry gullies' beds and banks, the trees' shade, the zones.
    surface = area.water_surface()
    wet = np.where(np.isfinite(surface) & (surface > area.h), 1.0, 0.0).astype(np.float32)
    wet = np.where(inside, sample(wet, x, y, area.half), 0.0).astype(np.float32)
    water = np.clip(blur(wet, cells(6.0, work.px)) * 5.0, 0.0, 1.0)
    del wet, surface
    gully = np.zeros((n, n), np.float32)
    for g in getattr(area, 'gullies', []):
        dist, _, _ = polyline_field(work, g['pts'], g['half'] + 14.0)
        gully = np.maximum(gully, 1.0 - _ss(g['half'] + 2.0, g['half'] + 12.0, np.where(np.isfinite(dist), dist, 1e3)))
    near, under = _shade(area, work, settings)
    nz = _noise(work)
    m = _moisture(t, nz, WET['water'] * water + WET['gully'] * gully + WET['shade'] * near
                  + _zones(area, work, settings))
    del water, gully
    b = _broad(m, t, nz)
    # The shares over the open ground a player sees as meadow: inside the boundary, gentle.
    meadow = inside & (t['slope'] < 20.0)
    if area.boundary is not None:
        from area_math import points_in_polygon
        meadow &= points_in_polygon(x.ravel(), y.ravel(), area.boundary).reshape(x.shape)
    log(f'macro: late summer, moisture {_shares(m[meadow])}')
    del nz, t, inside, x, y, meadow
    area.mosaic_moisture = (m, half)  # the scatter's grass follows it (grass_density())
    # Contact shadow: the ground darker round the feet of the rocks (the outcrops and boulder groups), so they sit in
    # it rather than on it.
    b['value'] = b['value'] * (1.0 - 0.22 * _feet(area, work))
    return dict(m=m, broad=b, under=under, work=work, half=half)


def _shares(m):
    """The ground's share in each band of moisture, for the log."""
    names = ('straw', 'gold', 'olive gold', 'olive', 'green', 'wet')
    bins = [s for s, _ in MOISTURE[:-1]] + [1.0001]
    counts = np.histogram(np.clip(m, 0.0, 1.0), bins=bins)[0]
    total = max(1, int(counts.sum()))
    return ', '.join(f'{name} {100.0 * c / total:.0f}%' for name, c in zip(names, counts))


def _hayfields(area, grid, rgb, alpha, settings, fine):
    """Each hayfield (settings "hayfields": [{"id": an openGround polygon, "rows": the mowing's heading in degrees, 0 =
    north}]): pale stubble in swaths a little lighter and darker in turn, raked windrows, an uncut headland."""
    polygons = {o['id']: o['polygon'] for o in area.layout.get('openGround', {}).get('open', [])}
    regrown = None
    for field in settings.get('hayfields', []):
        poly = to_m(polygons[field['id']])
        lo, hi = poly.min(axis=0) - 2.0, poly.max(axis=0) + 2.0
        i0, i1 = grid.index_range(lo[0], hi[0])
        j0, j1 = grid.index_range(lo[1], hi[1])
        x, y = grid.mesh(i0, i1, j0, j1)
        sd = _window_sd(poly, x, y)
        jag = fine[i0:i1, j0:j1]
        inside = 1.0 - _ss(-0.4, 0.4, sd + 0.25 * jag)
        cut = inside * _ss(HEADLAND - 0.6, HEADLAND + 0.4, -sd + 0.5 * jag)
        a = math.radians(field.get('rows', 0.0))
        u = -x * math.sin(a) + y * math.cos(a)  # across the rows
        swath = 0.5 + 0.5 * np.sin(u * (2.0 * math.pi / SWATH))
        row = np.abs((u / WINDROW_STEP) % 1.0 - 0.5) * WINDROW_STEP
        windrow = 1.0 - _ss(0.35, 0.8, row + 0.15 * jag)
        if regrown is None:
            regrown = fbm_raster(grid, 9.0, seed=851, octaves=2)  # green aftergrowth in patches
        regrowth = _ss(0.15, 0.6, regrown[i0:i1, j0:j1]) * 0.3
        stubble = np.empty(x.shape + (3,), np.float32)
        stubble[:] = STUBBLE
        _mix(stubble, OLIVE_GOLD, regrowth)
        _mix(stubble, WINDROW, 0.6 * windrow)
        stubble *= (0.95 + 0.08 * swath)[..., None]
        window = rgb[i0:i1, j0:j1]
        _mix(window, stubble, cut)
        alpha[i0:i1, j0:j1] *= 1.0 - cut


def _window_sd(poly, x, y):
    """Signed distance (m, negative inside) from a closed polygon at points."""
    from area_math import points_in_polygon
    best = np.full(x.shape, np.inf)
    ends = np.vstack([poly, poly[:1]])
    for p, q in zip(ends[:-1], ends[1:]):
        d = q - p
        tt = np.clip(((x - p[0]) * d[0] + (y - p[1]) * d[1]) / max(float(d @ d), 1e-12), 0.0, 1.0)
        best = np.minimum(best, np.hypot(x - p[0] - tt * d[0], y - p[1] - tt * d[1]))
    inside = points_in_polygon(x.ravel(), y.ravel(), poly).reshape(x.shape)
    return np.where(inside, -best, best).astype(np.float32)


def canopy(area, grid):
    """The far trees' crowns on a grid (area_fartrees.py places them; None without any): 1 under a crown."""
    trees = getattr(area, 'far_trees', None)
    if trees is None:
        import area_fartrees
        trees = area_fartrees.place(area)
    if not trees:
        return None
    out = np.zeros((grid.n, grid.n), np.float32)
    for name, items in trees['meshes'].items():
        reach = CROWN.get(name, 2.5)
        for x, y, _, _, s in items:
            cx, cy, r = x / 100.0, y / 100.0, reach * s
            i0, i1 = grid.index_range(cx - r, cx + r)
            j0, j1 = grid.index_range(cy - r, cy + r)
            if i1 <= i0 or j1 <= j0:
                continue
            xx, yy = grid.mesh(i0, i1, j0, j1)
            np.maximum(out[i0:i1, j0:j1], 1.0 - _ss(0.6 * r, r, np.hypot(xx - cx, yy - cy)), out=out[i0:i1, j0:j1])
    return out


def ring(area, grid, log=print):
    """The ring map's ground (rgb, alpha) on its grid, from the regional heights, with the woods' floor under the far
    trees."""
    region = area.region
    n = min(grid.n, max(256, int(round(2.0 * region.half / max(WORK_PX, 2.0 * grid.px) / 8.0)) * 8))
    work = Grid(n, region.half)
    x, y = work.mesh()
    hw = resize(area.ring_h, n, region.half)
    base = region.at(x.ravel(), y.ravel())['base'].reshape(x.shape).astype(np.float32)
    t = _terrain(work, hw, base)
    del base, hw
    crowns = canopy(area, grid)
    woods = np.zeros((n, n), np.float32)
    if crowns is not None:
        woods = np.clip(resize(blur(crowns, cells(10.0, grid.px)), n, region.half) * 3.0, 0.0, 1.0)
    nz = _noise(work)
    m = _moisture(t, nz, WET['woods'] * woods)
    b = _broad(m, t, nz)
    del nz, t, woods
    up = {k: _up(v, grid, work) for k, v in b.items()}
    m_up = _up(m, grid, work)
    del b, m
    fine = fbm_raster(grid, 2.2, seed=104, octaves=2)
    rgb, alpha = _colors(m_up, up, fine, None)
    if crowns is not None:
        # The woods' floor: dark under each crown, and the whole stand darker and greener between them, so a wood
        # reads as one mass from afar even where its trees stand apart.
        stand = np.clip(blur(crowns, cells(9.0, grid.px)) * 2.5, 0.0, 1.0)
        _mix(rgb, SHADE, 0.45 * stand)
        floor = blur(crowns, cells(2.5, grid.px))
        _mix(rgb, FOREST_FLOOR, 0.75 * np.clip(floor * 1.6, 0.0, 1.0))
        del stand, floor
    log(f'macro: late summer on the ring, moisture {_shares(m_up)}')
    return rgb, alpha
