"""The big slopes of a grounded area's ridges (numpy only): the regional ridges' faces from their cliff band's top
over the crest and down their backs, which would otherwise be one smooth ochre sweep each. fields() works out what
breaks them up, from the heights of any raster over them, and the macro maps (area_macro.py, the core's and the
ring's), the scatter mask (area_scatter.py) and the far trees (area_fartrees.py) all read it, so they agree where the
core's map fades into the ring's:
- rock bands at irregular heights: strata that follow the contours but wander, step at faults, thin out and pinch out
  in lenses, each a darker outcrop with a lit lip along its top and a shadowed ledge under it, on the steeper ground;
- scree fans below some of the lenses, rayed down the fall line;
- creases: the concave lines down the slopes (where the contours curve round a hollow, from the height raster's
  curvature, drawn out along the fall line), where scrub gathers and dark pines stand in groups;
- scrub and dry grass climbing the gentler ground and the creases in tongues, densest at the face's foot, thinning
  upward;
- tone variation: large patches lighter and darker, warmer and greyer, and streaks down the fall line.
Everything is a function of world position (layout meters) and the heights, worked out on a raster of about WORK_PX
(or the raster's own cells, if coarser), so the core's 10 cm map and the ring's paint the same bands.

paint() lays the faces over a macro map after its season's grade (their colors are the ones the faces show), with
the macro alpha (the detail selector) following them: rock on the bands and the bare faces, scree on the fans, soil
between, grass detail where the grass climbs. A late-summer area (area_mosaic.py) paints them in its own palette
(palette()), with grass holding on the steep faces in patches and tongues by aspect, rusty soil, bands of several
colors and, on the far faces nobody walks near, painted pine stands. pines() gives the scatter mask's creases' pines
their density, and area_scrub.py reads the fields for the scrub's.
"""
import numpy as np

import area_mosaic
from area_math import Grid, blur, cells, fbm, gradient_noise, raster_size, resize, sample, signed_distance, smoothstep

WORK_PX = 0.4            # meters per cell the fields are worked out at (or the raster's own, if coarser)
CREASE_BLUR = 4.0        # meters: the scale of the hollows a crease follows
CREASE_CURVE = (0.008, 0.035)  # the contours' curvature (1/m) over which a hollow becomes a crease
CREASE_REACH = 12.0      # meters up and down the fall line a crease is drawn out over
STREAK_REACH = 5.0       # meters up and down the fall line the streaks are drawn out over
TONGUE_REACH = 10.0      # meters up and down the fall line the grass tongues are drawn out over
BAND_SPACING = 8.5       # meters of height between band candidates (irregular: noise along the height)
LEDGE = 1.8              # meters down the fall line a band's ledge shadows the ground under it
CAP = 0.5                # meters of height under a band's top that catch the light (its lip)
FAN_REACH = 9.0          # meters down the fall line a band's scree fan reaches
LIC_SPREAD = 0.09        # the usual spread of noise drawn out along the fall line: scaled up to about -1..1
WOODS_PAST = (50.0, 90.0)  # meters past the playable boundary a late-summer area's painted pine woods fade in over

# Colors (sRGB), as the faces show them: laid on after the macro map's grade, so they sit among its golden meadow
# (about #908155), its banks (#6b6041) and its rock (#756f61).
FACE_GRASS, FACE_STRAW = np.array([0x8a, 0x7b, 0x50]) / 255.0, np.array([0xa6, 0x95, 0x63]) / 255.0
FACE_SOIL, FACE_SOIL_DARK = np.array([0x8c, 0x77, 0x56]) / 255.0, np.array([0x6c, 0x5b, 0x44]) / 255.0
FACE_ROCK, FACE_ROCK_LIGHT = np.array([0x7e, 0x74, 0x63]) / 255.0, np.array([0x98, 0x8b, 0x74]) / 255.0
FACE_ROCK_COOL, FACE_ROCK_DARK = np.array([0x78, 0x76, 0x70]) / 255.0, np.array([0x55, 0x4f, 0x46]) / 255.0
FACE_LEDGE = np.array([0x3a, 0x34, 0x2c]) / 255.0
FACE_SCREE, FACE_SCREE_DARK = np.array([0x99, 0x90, 0x7f]) / 255.0, np.array([0x76, 0x6e, 0x60]) / 255.0
FACE_SCRUB, FACE_SCRUB_DARK = np.array([0x55, 0x51, 0x34]) / 255.0, np.array([0x3c, 0x3e, 0x28]) / 255.0
FACE_CREASE = np.array([0x4e, 0x4b, 0x32]) / 255.0


def _ss(e0, e1, x):
    return smoothstep(e0, e1, x).astype(np.float32)


def _steps(n, count=2.5):
    """A smooth staircase of noise: flat treads with quick risers, so what it offsets jumps at "faults"."""
    k = n * count
    f = np.floor(k)
    return (f + _ss(0.78, 1.0, k - f)) / count


def _fall_line(raster, x, y, ux, uy, half, reach, step):
    """A raster averaged along the fall line (line integral convolution): through each point, from reach meters down
    the slope to reach meters up it."""
    count = max(1, int(round(reach / step)))
    total = np.zeros(x.shape, np.float32)
    for k in range(-count, count + 1):
        total += sample(raster, x + ux * k * step, y + uy * k * step, half).astype(np.float32)
    return total / (2 * count + 1)


def _uphill_max(raster, x, y, ux, uy, half, reach, step, fade):
    """The most of a raster found up the slope from each point, within reach meters, faded by fade(d): what lies
    above a point (a band over a ledge or a scree fan)."""
    best = np.zeros(x.shape, np.float32)
    for k in range(1, max(1, int(round(reach / step))) + 1):
        d = k * step
        np.maximum(best, fade(d) * sample(raster, x + ux * d, y + uy * d, half).astype(np.float32), out=best)
    return best


def fields(area, grid, h):
    """The faces' fields over a raster grid (Grid) with heights h on it (m), worked out at WORK_PX or the grid's own
    cells: a dict of rasters on that working grid ("grid" is it). None for an area without regional ridges."""
    region = getattr(area, 'region', None)
    if region is None or not region.ridges or region.fields is None:
        return None
    n = grid.n if grid.px >= 0.9 * WORK_PX else raster_size(2.0 * grid.half, WORK_PX, 1 << 15)
    work = grid if n == grid.n else Grid(n, grid.half)
    half = grid.half
    hw = (h if n == grid.n else resize(h, n, half)).astype(np.float32)
    px = work.px
    x, y = work.mesh()
    gx, gy = np.gradient(hw, px)
    slope = np.degrees(np.arctan(np.hypot(gx, gy))).astype(np.float32)
    # Up the slope (unit, layout axes), from slightly smoothed heights.
    sx, sy = np.gradient(blur(hw, cells(1.0, px)), px)
    norm = np.maximum(np.hypot(sx, sy), 1e-4)
    ux, uy = (sx / norm).astype(np.float32), (sy / norm).astype(np.float32)
    del sx, sy, norm, gx, gy

    # Which ground is a ridge's face: past its band's top, out to its back's foot, on the upland.
    f = region.at(x.ravel(), y.ravel())
    on = np.zeros(x.size, np.float32)
    for i, r in enumerate(region.ridges):
        s = f[f'ridge{i}']
        on = np.maximum(on, _ss(r['band_width'] + 0.3, r['band_width'] + 2.5, s)
                        * (1.0 - _ss(r['depth'] + r['back'] - 8.0, r['depth'] + r['back'], s)))
    if region.lip is not None:
        on *= _ss(0.5, 3.0, f['dl'])
    face = on.reshape(n, n)
    rel = (hw - f['base'].reshape(n, n)).astype(np.float32)  # meters over the valley's floor
    del f, on

    # Creases: hollows (contours curving round them: positive plan curvature) at the scale of CREASE_BLUR, drawn out
    # up and down the fall line, on the faces' slopes.
    hb = blur(hw, cells(CREASE_BLUR, px))
    fx, fy = np.gradient(hb, px)
    fxx, fxy = np.gradient(fx, px)
    _, fyy = np.gradient(fy, px)
    p = fx * fx + fy * fy
    curve = (fxx * fy * fy - 2.0 * fxy * fx * fy + fyy * fx * fx) / (p ** 1.5 + 1e-6)
    hollow = (_ss(*CREASE_CURVE, curve) * _ss(0.03, 0.15, p)).astype(np.float32)
    del hb, fx, fy, fxx, fxy, fyy, p, curve
    crease = _fall_line(hollow, x, y, ux, uy, half, CREASE_REACH, max(px, 1.0))
    crease = _ss(0.2, 0.6, crease) * _ss(18.0, 26.0, slope) * face
    del hollow

    # Streaks down the fall line (fine noise drawn out along it), and tongues (coarser, drawn out farther). Noise
    # finer than a few cells would alias on a coarse raster (the ring's), so none is.
    finest = 2.5 * px
    grain = fbm(x, y, max(1.6, finest), seed=611, octaves=1).astype(np.float32)
    streak = _fall_line(grain, x, y, ux, uy, half, STREAK_REACH, max(px, 0.6))
    streak = np.clip(streak * (0.5 / LIC_SPREAD), -1.0, 1.0).astype(np.float32)
    lumps = fbm(x, y, 7.0, seed=612, octaves=2).astype(np.float32)
    tongue = _fall_line(lumps, x, y, ux, uy, half, TONGUE_REACH, max(px, 1.0))
    tongue = np.clip(tongue * (0.5 / LIC_SPREAD), -1.0, 1.0).astype(np.float32)
    del grain, lumps

    # Rock bands: noise along the height (its sequence drifting slowly across the map, so each face has its own),
    # the band height wandering a few meters over tens of meters and stepping at faults; thinned or gone where the
    # threshold is high; broken into lenses; only on the steeper ground, and fewer on the steepest walls (their own
    # strata band them already).
    lift = (3.2 * fbm(x, y, 55.0, seed=613, octaves=2) + 1.0 * fbm(x, y, 17.0, seed=614, octaves=2)
            + 1.8 * _steps(fbm(x, y, 34.0, seed=615, octaves=2))).astype(np.float32)
    hband = hw + lift
    drift = (x * 0.37 + y * 0.61) / 340.0

    # The finer strata fade where a cell spans more height than they'd show (the steepest walls on a coarse raster).
    fine_strata = 0.35 * (1.0 - _ss(0.6, 1.2, np.tan(np.radians(np.minimum(slope, 85.0))) * px))

    def strata_at(hb):
        return gradient_noise(hb / BAND_SPACING, drift, 621) + fine_strata * gradient_noise(hb / 3.4, drift + 7.3, 622)
    strata = strata_at(hband).astype(np.float32)
    above = strata_at(hband + CAP).astype(np.float32)
    # A late-summer area's bands each take a color of their own (area_mosaic.FACES), by a noise along the same height.
    band_hue = (gradient_noise(hband / BAND_SPACING, drift + 3.1, 628).astype(np.float32)
                if area_mosaic.spec(area) is not None else None)
    del hband, lift, drift, fine_strata
    thr = (0.2 + 0.14 * fbm(x, y, 28.0, seed=623, octaves=2)).astype(np.float32)
    # A lens pinches out toward its ends rather than stopping square: the threshold rises there.
    lens = _ss(-0.1, 0.25, fbm(x, y, 30.0, seed=624, octaves=2) + 0.3 * fbm(x, y, 9.0, seed=625, octaves=2))
    thr = thr + 0.3 * (1.0 - lens)
    rocky = _ss(26.0, 36.0, slope + 6.0 * fbm(x, y, 9.0, seed=626, octaves=2))
    rocky *= (1.0 - 0.6 * _ss(56.0, 64.0, slope)) * face
    band_v = (strata - thr).astype(np.float32)
    band = _ss(0.0, 0.06, band_v) * rocky
    cap = band * (1.0 - _ss(0.0, 0.06, above - thr))  # the band's top lip, catching the light
    del strata, above, thr
    # The ledge's shadow just under each band, and scree fans under some lenses, wider and fainter farther down,
    # rayed by the streaks.
    step = max(px, 0.35)
    ledge = _uphill_max(band, x, y, ux, uy, half, LEDGE, step, lambda d: 1.0 - 0.6 * d / LEDGE) * (1.0 - band)
    sheds = band * _ss(0.0, 0.35, fbm(x, y, 22.0, seed=627, octaves=2))
    fan = _uphill_max(blur(sheds, cells(1.2, px)), x, y, ux, uy, half, FAN_REACH, max(px, 0.6),
                      lambda d: 1.0 - d / (FAN_REACH + 1.0))
    fan = blur(fan, cells(0.8, px)) * (1.0 - band) * (0.45 + 0.55 * _ss(-0.4, 0.4, streak)) * _ss(18.0, 28.0, slope)
    del sheds

    # Scrub: at the face's foot and in the creases, on gentler ground, thinning upward, in patches.
    foot = 1.0 - _ss(9.0, 22.0, rel)
    up = _ss(16.0, 50.0, rel)
    gentle = 1.0 - _ss(30.0, 44.0, slope)
    patch = _ss(-0.3, 0.3, fbm(x, y, 9.0, seed=631, octaves=2) + 0.5 * fbm(x, y, 3.5, seed=632, octaves=2)
                + 0.3 * tongue)
    scrub = np.clip(0.15 + 0.5 * foot + 0.75 * crease + 0.3 * gentle, 0.0, 1.0)
    scrub = scrub * (1.0 - 0.6 * up * (1.0 - crease)) * patch * face * (1.0 - band) * (1.0 - 0.6 * fan)
    # Pines: few, in groups along the creases.
    groups = _ss(0.0, 0.35, fbm(x, y, 38.0, seed=633, octaves=2))
    pines = np.clip(2.2 * crease * (0.25 + 0.75 * groups), 0.0, 1.0) * (1.0 - 0.4 * up)
    pines *= _ss(20.0, 28.0, slope) * (1.0 - _ss(52.0, 58.0, slope))

    clumps = fbm(x, y, max(1.5, finest), seed=634, octaves=2).astype(np.float32)  # scrub's clumps, a meter or two
    tone = fbm(x, y, 42.0, seed=641, octaves=2).astype(np.float32)
    warm = fbm(x, y, 70.0, seed=642, octaves=2).astype(np.float32)
    f32 = np.float32
    out = dict(grid=work, face=face, slope=slope, rel=rel, crease=crease.astype(f32), streak=streak,
               tongue=tongue, band_v=band_v, banded=rocky.astype(f32), band=band.astype(f32),
               cap=cap.astype(f32), ledge=ledge.astype(f32), fan=fan.astype(f32), scrub=scrub.astype(f32),
               pines=pines.astype(f32), tone=tone, warm=warm, patch=patch.astype(f32), clumps=clumps)
    if area_mosaic.spec(area) is not None:
        # A late-summer area's faces (area_mosaic.FACES) wear grass in big patches and tongues, more of it on the
        # slopes turned from the afternoon sun (north and east), and their bands' rock in several colors: how far a
        # slope faces away from the sun, the grass cover's noise, and each band's color by its height.
        ax, ay = np.gradient(blur(hw, cells(3.0, px)), px)
        g = np.maximum(np.hypot(ax, ay), 1e-4)
        out['cool'] = ((-0.8 * ax - 0.45 * ay) / g).astype(f32)
        del ax, ay, g
        out['cover'] = (0.8 * fbm(x, y, 55.0, seed=651, octaves=2) + 0.45 * fbm(x, y, 18.0, seed=652, octaves=2)
                        ).astype(f32)
        out['band_hue'] = band_hue
        # Pine woods on the far faces nobody walks near (WOODS_PAST m and more past the playable boundary): the higher
        # forested ridges' stands, painted, where the slope is too steep for the far trees.
        if getattr(area, 'boundary', None) is not None:
            # Stands run down the slopes in tongues and up the creases (noise drawn out along the fall line), as trees
            # on a steep face follow its gullies, rather than sitting on it as round blots.
            stands = 0.45 * groups + 0.4 * tongue + 0.9 * crease + 0.25 * out['cover'] - 0.2
            out['woods'] = (_ss(*WOODS_PAST, signed_distance(work, area.boundary, WOODS_PAST[1] + 5.0))
                            * _ss(0.0, 0.25, stands) * _ss(30.0, 40.0, slope)
                            * (1.0 - _ss(62.0, 70.0, slope))).astype(f32)
            del stands
    return out


def ring_fields(area):
    """The faces' fields over the surround ring's square at its macro map's size (layout.json region.ring.macro),
    worked out once per area: the far trees and the ring's macro map both read them (area_macro.paint_ring() lets
    them go after)."""
    if getattr(area, 'ring_faces', None) is None:
        n = area.layout['region'].get('ring', {}).get('macro', 2048)
        half = area.region.half
        area.ring_faces = fields(area, Grid(n, half), resize(area.ring_h, n, half))
    return area.ring_faces


def at(faces, name, n):
    """One of the fields on an n x n raster over the same square."""
    raster = faces[name]
    return raster if raster.shape[0] == n else resize(raster, n, faces['grid'].half)


def pines(faces, n):
    """The creases' pines' density (0..1) on an n x n raster, for area_scatter.py: few, grouped in the creases, never
    on a band, thin on a fan."""
    return at(faces, 'pines', n) * (1.0 - at(faces, 'band', n)) * (1.0 - 0.7 * at(faces, 'fan', n))


def _mix(a, b, t):
    return (a + (b - a) * np.clip(t, 0.0, 1.0)[..., None]).astype(np.float32)


def _paint(rgb, alpha, color, weight, a=None):
    w = np.clip(weight, 0.0, 1.0).astype(np.float32)
    color = np.asarray(color, dtype=np.float32)
    if color.ndim == 1:
        rgb += (color[None, None, :] - rgb) * w[..., None]
    else:
        rgb += (color - rgb) * w[..., None]
    if a is not None:
        alpha += (np.asarray(a, dtype=np.float32) - alpha) * w


# The faces' colors (PALETTE), by name. A late-summer area brings its own (area_mosaic.FACES), with the extras
# _paint_rows() reads when they're there: cover (grass in patches and tongues by the faces' cover noise and aspect),
# grass_cool (the grass on slopes turned from the sun), soil_red (rusty soil in large patches), strata (each band's
# color by its height: stops of a ramp over the band noise), pines (the far faces' painted stands; with cover).
PALETTE = dict(grass=FACE_GRASS, straw=FACE_STRAW, soil=FACE_SOIL, soil_dark=FACE_SOIL_DARK, rock=FACE_ROCK,
               rock_light=FACE_ROCK_LIGHT, rock_cool=FACE_ROCK_COOL, rock_dark=FACE_ROCK_DARK, ledge=FACE_LEDGE,
               scree=FACE_SCREE, scree_dark=FACE_SCREE_DARK, scrub=FACE_SCRUB, scrub_dark=FACE_SCRUB_DARK,
               crease=FACE_CREASE)


def palette(area):
    """The faces' colors for an area: its late summer's, else PALETTE."""
    return area_mosaic.FACES if area_mosaic.spec(area) is not None else PALETTE


def paint(rgba, faces, keep=None, fine=None, grain=None, block=512, colors=None):
    """Lays the faces over an (n, n, 4) macro map (sRGB, alpha the detail selector), after its grade, block rows at a
    time. keep (n x n, 0..1) holds what the map already paints (roads, water, the core's features) back from them;
    fine and grain are that map's own noise (-1..1, about 1 m and finer), for frayed edges and speckle (None on a
    coarse map); colors is the palette (PALETTE by default)."""
    colors = PALETTE if colors is None else colors
    n = rgba.shape[0]
    work, half = faces['grid'], faces['grid'].half
    grid = Grid(n, half)
    for a in range(0, n, block):
        b = min(n, a + block)
        if n == work.n:
            def field(name, a=a, b=b):
                return faces[name][a:b]
        else:
            x, y = grid.mesh(a, b)

            def field(name, x=x, y=y):
                return sample(faces[name], x, y, half).astype(np.float32)
        w = field('face')
        if keep is not None:
            w = w * (1.0 - keep[a:b])
        if w.any():
            _paint_rows(rgba[a:b], field, w, None if fine is None else fine[a:b],
                        None if grain is None else grain[a:b], colors)


def _ramp(t, stops):
    xs = np.array([s for s, _ in stops], dtype=np.float32)
    cs = np.array([c for _, c in stops], dtype=np.float32)
    out = np.empty(np.shape(t) + (3,), np.float32)
    for k in range(3):
        out[..., k] = np.interp(t, xs, cs[:, k])
    return out


def _paint_rows(rgba, field, w, fine, grain, pal=PALETTE):
    rgb, alpha = rgba[..., :3], rgba[..., 3]
    slope, tone, warm, streak, tongue, strata = (field(k) for k in ('slope', 'tone', 'warm', 'streak', 'tongue',
                                                                     'band_v'))
    jag = 0.0 if fine is None else fine
    # The ground: ochre stony soil on the steep faces, bare rock only on the steepest (stonier along the strata, the
    # ground near a band, so it varies along the contours too), dry grass on the gentler slopes climbing up them in
    # tongues; patches lighter and darker, warmer and greyer; streaks down the steeper parts.
    rocky = _ss(50.0, 62.0, slope + 5.0 * tone + 6.0 * _ss(-0.25, 0.2, strata) + 3.0 * jag)
    grassy = (1.0 - _ss(32.0, 46.0, slope + 6.0 * tone - 7.0 * tongue + 3.0 * jag)) * (1.0 - rocky)
    cool = 0.0
    if pal.get('cover'):
        # Late summer: grass holds on the steep faces too, in big patches and tongues down them, a little more on the
        # slopes turned from the afternoon sun and in the creases, giving way to bare soil and rock toward the
        # steepest; so a long face isn't one sweep of ochre.
        cool = field('cool')
        cover = _ss(-0.25, 0.25, field('cover') + 0.25 * cool + 0.45 * tongue + 0.5 * field('crease') + 0.1 * jag
                    - 0.8 * _ss(42.0, 60.0, slope))
        grassy = np.maximum(grassy, cover * (1.0 - rocky))
        del cover
    # The rock face itself is banded too: lighter and darker strata along the contours.
    rock = _mix(_mix(pal['rock_cool'], pal['rock'], 0.5 + warm), pal['rock_dark'], 0.3 - 0.4 * tone - 0.4 * strata)
    if 'strata' in pal:
        # Late summer's walls: the layers' own colors across all the bare rock, not just the lenses (a tall wall reads
        # as cream, rust and grey courses), and darker runoff stains down it.
        rock = _mix(rock, _ramp(field('band_hue') + 0.05 * jag, pal['strata']), 0.55)
        rock *= (1.0 - 0.2 * _ss(0.1, 0.6, streak))[..., None]
    soil = _mix(pal['soil_dark'], pal['soil'], 0.5 + 0.6 * tone + 0.3 * streak)
    if 'soil_red' in pal:
        soil = _mix(soil, pal['soil_red'], 0.45 * _ss(0.0, 0.45, warm + 0.3 * streak))  # rusty soil in large patches
    soil = _mix(soil, pal['rock'], 0.25 + 0.3 * jag)  # stony
    grass = _mix(pal['grass'], pal['straw'], 0.4 + 0.7 * warm + 0.35 * tongue)
    if 'grass_cool' in pal:
        grass = _mix(grass, pal['grass_cool'], 0.35 + 0.65 * cool)  # olive where it's turned from the sun
    ground = _mix(_mix(soil, rock, rocky), grass, grassy)
    ground *= (1.0 + 0.1 * tone + 0.04 * tongue + 0.05 * streak * (1.0 - grassy))[..., None]
    _paint(rgb, alpha, ground, w, 0.45 + 0.4 * rocky - 0.37 * grassy)
    del ground, rock, soil, grass, rocky, grassy
    # Scrub: dark clumps in the creases and at the foot, sparser up the face; the creases darker under them.
    crease = field('crease')
    speck = 0.0 if grain is None else grain
    threshold = 0.45 - 0.5 * field('scrub')
    spots = _ss(threshold - 0.05, threshold + 0.05, field('clumps') + 0.15 * speck + 0.08 * jag)
    _paint(rgb, alpha, pal['crease'], 0.45 * w * crease)
    _paint(rgb, alpha, _mix(pal['scrub'], pal['scrub_dark'], crease + 0.3 * speck), 0.8 * w * spots, 0.12)
    if pal.get('cover'):
        # Late summer: dark scrub clinging to the steep rock here and there, most on the ledges' tops; and on the far
        # faces, stands of pine (their dark needle floor and crowns, with ragged edges).
        clinging = _ss(0.5, 0.6, field('clumps') + 0.35 * field('cap') + 0.1 * speck) * _ss(44.0, 54.0, slope)
        _paint(rgb, alpha, pal['scrub_dark'], 0.7 * w * clinging)
        del clinging
        try:
            woods = field('woods')
        except KeyError:
            woods = None
        if woods is not None:
            ragged = _ss(0.25, 0.7, woods + 0.25 * field('clumps') + 0.1 * jag)
            _paint(rgb, alpha, _mix(pal['pines'], pal['scrub_dark'], 0.3 + 0.4 * field('clumps')), 0.85 * w * ragged,
                   0.1)
            del woods, ragged
    del spots, crease
    # Scree fans under some lenses, then the bands (their edges frayed) and the shadow under their ledges.
    _paint(rgb, alpha, _mix(pal['scree_dark'], pal['scree'], 0.5 + 0.6 * streak + 0.3 * jag), 0.8 * w * field('fan'),
           0.6)
    band = _ss(0.0, 0.06, strata + 0.04 * jag) * field('banded')
    # A band is a darker, greyer outcrop with a lighter lip along its top (late summer: each band its own color).
    shade = 0.45 + 0.25 * tone + 0.3 * jag + 0.2 * streak
    if 'strata' in pal:
        layer = _ramp(field('band_hue') + 0.05 * jag, pal['strata'])
        outcrop = layer * (0.78 + 0.3 * np.clip(shade, 0.0, 1.0))[..., None]
        del layer
    else:
        outcrop = _mix(pal['rock_dark'], pal['rock'], shade)
    _paint(rgb, alpha, outcrop, w * band, 1.0)
    del outcrop, shade
    _paint(rgb, alpha, _mix(pal['rock'], pal['rock_light'], 0.85 + 0.3 * jag), w * field('cap'))
    _paint(rgb, alpha, pal['ledge'], 0.85 * w * field('ledge') * (1.0 - band), 0.9)
    np.clip(rgb, 0.0, 1.0, out=rgb)
