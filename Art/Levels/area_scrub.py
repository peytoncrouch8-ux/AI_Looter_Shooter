"""A grounded area's dry scrub (numpy only): where the art session's scrub kit (Art/Models/Vegetation/Scrub.py:
sagebrush, rabbitbrush, junipers, dry bunchgrass) grows, for the PCG scatter graph
(Tools/Unreal/build_island_scatter.py, whose mesh tables name the kit). Densities per square meter are the art
session's:

  slopes      sagebrush 0.15-0.25 (thinning up the faces), dry tufts 0.25-0.4 in the gaps (out to TUFT_REACH m past
              the boundary: they're culled at 40 m), rabbitbrush 0.01-0.02 at the faces' toes, junipers 0.02-0.04
              only in the creases (area_faces.py), at least 4 m apart
  flats       on the margins only (the roadsides, the slopes' toes, the boundary's edge, the gullies' banks, round the
              ruins; never in the fields, the town, the yards, the zones of EXCLUDED kinds or ground left open on
              purpose): sagebrush 0.03-0.08, dry tufts 0.15-0.3 where the meadow thins, rabbitbrush 0.01-0.02 on
              disturbed ground (the roadsides and road cuts, round the ruins)
  pit floors  (the Sink) dry tufts 0.3-0.5 in patches, small sage 0.02-0.05 near the walls, no rabbitbrush, a juniper
              or two on the rim

paint() writes the scrub mask, T_<Area>Scrub_BC.png beside the scatter mask (the same square and size, linear bytes,
north up), one layer per channel (LAYERS): R sagebrush, G dry tufts, B rabbitbrush, A creases' junipers. The graph
draws a layer's candidates every `cell` cm (jittered by `jitter` of a cell) and keeps one where mask x random >= keep,
so the mask holds keep / (1 - chance): a candidate is kept with that chance, and the layer's density is chance / cell².
points() gives what a mask can't place exactly, for layout_computed.json's "scrub": a wind-sheared juniper every
CREST_STEP m along the ridges' crests, each pit floor's tufts and small sage, and a juniper or two on each pit's rim.
Both are deterministic: noise and seeded draws only.
"""
import math

import numpy as np

from area_math import blur, catmull_rom, cells, fbm, fbm_raster, polyline_field, resample, signed_distance
from area_shape import to_m

# The mask's layers: channel, candidate spacing (cm), the graph's keep, jitter (share of a cell either way).
LAYERS = {
    'sage': {'channel': 'R', 'cell': 190.0, 'keep': 0.1, 'jitter': 0.5},
    'tufts': {'channel': 'G', 'cell': 150.0, 'keep': 0.1, 'jitter': 0.5},
    'rabbitbrush': {'channel': 'B', 'cell': 500.0, 'keep': 0.1, 'jitter': 0.5},
    # 5 m apart, at most 0.5 m off: never closer than 4 m.
    'junipers': {'channel': 'A', 'cell': 500.0, 'keep': 0.1, 'jitter': 0.1},
}
CHANNELS = ('sage', 'tufts', 'rabbitbrush', 'junipers')
# The graph's scrub layers stand on ground up to this steep (degrees; flatness 0.64): the faces' steepest rock is bare.
STEEPEST = 50.2
TUFT_REACH = 40.0       # meters past the boundary the dry tufts reach
RABBIT_REACH = 80.0     # and the rabbitbrush
BOUNDARY_ROCK = 5.0     # meters past the boundary kept bare (the rock raised past its closed edges)
SEAM_FADE = (4.0, 20.0)  # the scrub fades out between these distances (m) inside the core square's edge
# Zone kinds the flats' scrub keeps out of (the fields, the town, the yards, where people live and are buried, and
# Ransom's Point's top, the lookout the whole valley is seen from).
EXCLUDED = ('town', 'fields', 'orchard', 'home', 'graveyard', 'chapel', 'travel', 'boss_arena', 'lookout', 'pit',
            'forest', 'range', 'village')
CREST_STEP = (10.0, 20.0)  # meters between the crests' junipers
CREST_YAW = 20.0        # degrees either side of 0: their swept crowns (-Y) point west, downwind
RIM_JUNIPERS = 2        # per pit, on its rim
PIT_TUFT_SPACING = 1.0  # meters between the pit floors' tuft candidates
PIT_SAGE_SPACING = 3.0  # and their sage's


def _ss(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=np.float32) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def _zones(area, grid, kinds, soft=2.0):
    """1 inside the zones of the kinds (a zone's kind, else its id), fading over soft meters across their edges."""
    out = np.zeros((grid.n, grid.n), np.float32)
    for zone in area.layout.get('zones', []):
        if zone.get('kind', zone['id']) not in kinds:
            continue
        sd = signed_distance(grid, catmull_rom(to_m(zone['polygon']), closed=True, step=0.5), soft + 6.0)
        out = np.maximum(out, 1.0 - _ss(-soft, soft, sd))
    return out


def _polygons(grid, polygons, soft=2.0):
    """1 inside any of the polygons (cm), fading over soft meters."""
    out = np.zeros((grid.n, grid.n), np.float32)
    for poly in polygons:
        sd = signed_distance(grid, to_m(poly), soft + 6.0)
        out = np.maximum(out, 1.0 - _ss(-soft, soft, sd))
    return out


def densities(area, grid, faces, ctx):
    """The four layers' densities per square meter on the scatter grid (dict by LAYERS name). ctx is the scatter
    mask's own rasters on that grid: slope, the roads' surface (and the 1.5 m past it), water, bare ground, the gullies'
    gravel, and past (meters past the playable boundary, negative inside)."""
    import area_faces
    n = grid.n
    x, y = grid.mesh()
    slope, past = ctx['slope'], ctx['past']
    face, crease, rel = (area_faces.at(faces, k, n) for k in ('face', 'crease', 'rel'))
    band, fan = (area_faces.at(faces, k, n) for k in ('band', 'fan'))
    stands = _ss(-0.35, 0.35, fbm_raster(grid, 11.0, seed=701, octaves=2) + 0.4 * fbm_raster(grid, 3.0, seed=702))
    gaps = 1.0 - stands
    up = _ss(16.0, 50.0, rel)

    # Where nothing grows: roads and the ground just past them, water, bare ground, gravel, the rock past the closed
    # edges, the steepest faces, the core's last meters (the ring has no scatter: fade, so no line shows there).
    ok = (1.0 - ctx['surface']) * (1.0 - ctx['wet']) * (1.0 - ctx['bare']) * (1.0 - ctx['gravel'])
    ok *= 1.0 - _ss(-0.5, 0.5, past) * (1.0 - _ss(BOUNDARY_ROCK, BOUNDARY_ROCK + 2.0, past))
    ok *= 1.0 - _ss(STEEPEST - 3.0, STEEPEST, slope)
    ok *= _ss(*SEAM_FADE, area.half - np.maximum(np.abs(x), np.abs(y)))
    for p in getattr(area, 'pits', []):
        ok *= 1.0 - _ss(0.02, 0.2, area.resized(p['rise'], n))  # the pit floors' scrub is points() (and its walls bare)

    # The faces: sage thickest at the foot, thinning up them, off the rock bands and thin on the scree; tufts in the
    # gaps between the sage; rabbitbrush at their toes; junipers in the creases.
    on_face = face * (1.0 - band) * (1.0 - 0.6 * fan)
    sloped = _ss(16.0, 26.0, slope)
    sage = on_face * (0.06 + (0.09 + 0.10 * stands) * sloped) * (1.0 - 0.4 * up)
    tufts = on_face * (0.25 + 0.15 * gaps) * _ss(10.0, 20.0, slope) * (1.0 - _ss(TUFT_REACH - 6.0, TUFT_REACH, past))
    toe = face * (1.0 - _ss(6.0, 14.0, rel))
    rabbit = 0.015 * toe * (0.5 + stands) * (1.0 - band)
    groups = _ss(-0.1, 0.3, fbm_raster(grid, 38.0, seed=633, octaves=2))
    junipers = face * _ss(0.3, 0.7, crease) * (0.02 + 0.02 * groups) * _ss(20.0, 26.0, slope)
    junipers *= (1.0 - _ss(46.0, 49.0, slope)) * (1.0 - band)

    # The flats' margins: the roadsides (from 1.5 m past a road's edge), the slopes' toes, the boundary's edge, the
    # gullies' banks and round the ruins; never in the excluded zones or the ground left open on purpose.
    gap = area.resized(np.minimum(area.road_gap, 50.0), n)
    roadside = _ss(1.2, 2.0, gap) * (1.0 - _ss(4.0, 7.0, gap))
    steep_near = blur(_ss(27.0, 36.0, slope), cells(4.0, grid.px))
    toes = _ss(0.08, 0.35, steep_near) * (1.0 - _ss(22.0, 28.0, slope))
    edge = (1.0 - _ss(8.0, 18.0, -past)) * _ss(0.0, 1.0, -past)
    banks = np.zeros((n, n), np.float32)
    for g in getattr(area, 'gullies', []):
        reach = g['half'] + (g['depth'] + 3.0) / g['bank'] + 2.0
        dist, _, _ = polyline_field(grid, g['pts'], reach + 3.0)
        d = np.where(np.isfinite(dist), dist, 1e3)
        banks = np.maximum(banks, _ss(g['half'] + 0.3, g['half'] + 1.0, d) * (1.0 - _ss(reach, reach + 2.5, d)))
    ruins = [ob['polygon'] for ob in area.layout.get('obstacles', []) if ob.get('kind') == 'ruin' and 'polygon' in ob]
    around_ruins = blur(_polygons(grid, ruins, 1.0), cells(3.0, grid.px)) if ruins else np.zeros((n, n), np.float32)
    around_ruins = _ss(0.05, 0.3, around_ruins)
    margin = np.maximum.reduce([roadside, toes, edge, banks, around_ruins])
    opened = [o['polygon'] for o in area.layout.get('openGround', {}).get('open', [])]
    flats = (1.0 - face) * (1.0 - _zones(area, grid, EXCLUDED)) * (1.0 - _polygons(grid, opened))
    flats *= _ss(-0.1, 0.4, fbm_raster(grid, 7.0, seed=703, octaves=2) + 0.5 * margin) * (past < 10.0)
    sage = sage + flats * margin * (0.03 + 0.05 * stands)
    tufts = tufts + flats * margin * (0.15 + 0.15 * gaps)
    cuts = _ss(0.05, 0.3, steep_near)  # a road cut: steep ground beside the road
    disturbed = np.maximum.reduce([roadside * (0.4 + 0.6 * cuts), around_ruins, banks, toes])
    rabbit = rabbit + 0.015 * flats * disturbed * (0.5 + stands)
    rabbit *= 1.0 - _ss(RABBIT_REACH - 8.0, RABBIT_REACH, past)
    out = {'sage': sage, 'tufts': tufts, 'rabbitbrush': rabbit, 'junipers': junipers}
    return {k: np.clip(v * ok, 0.0, None).astype(np.float32) for k, v in out.items()}


def paint(area, grid, faces, ctx, out_path, save, preview_dir=None):
    """Writes the scrub mask (and, with preview_dir, scrub.png: its four layers side by side, sage and tufts below,
    rabbitbrush and junipers above, as the chance per candidate). Returns a note of about how many each layer places."""
    dens = densities(area, grid, faces, ctx)
    soften = cells(0.4, grid.px)
    rgba = np.zeros((grid.n, grid.n, 4), np.float32)
    chances, notes = [], []
    for k, name in enumerate(CHANNELS):
        spec = LAYERS[name]
        cell = spec['cell'] / 100.0
        chance = np.clip(blur(dens[name], soften) * cell * cell, 0.0, 1.0 - spec['keep'] - 0.01)
        rgba[..., k] = np.where(chance > 0.002, spec['keep'] / (1.0 - chance), 0.0)
        chances.append(chance)
        notes.append(f"{float(dens[name].sum()) * grid.px * grid.px:.0f} {name}")
    save(rgba, out_path)
    if preview_dir:
        import os
        top = np.concatenate([chances[2], chances[3]], axis=1)
        bottom = np.concatenate([chances[0], chances[1]], axis=1)
        sheet = np.concatenate([bottom, top], axis=0)
        save(np.stack([sheet, sheet, sheet, np.ones_like(sheet)], axis=-1), os.path.join(preview_dir, 'scrub.png'))
    return 'scrub: about ' + ', '.join(notes)


# --- Points ---

def _rng_points(rng, lo, hi, spacing):
    """A jittered grid of candidates (N, 2) over a box, about spacing meters apart."""
    xs = np.arange(lo[0] + spacing * 0.5, hi[0], spacing)
    ys = np.arange(lo[1] + spacing * 0.5, hi[1], spacing)
    gx, gy = np.meshgrid(xs, ys, indexing='ij')
    pts = np.column_stack([gx.ravel(), gy.ravel()])
    return pts + rng.uniform(-0.45, 0.45, pts.shape) * spacing


def _polyline_distance(pts, line):
    best = np.full(len(pts), np.inf)
    for a, b in zip(line[:-1], line[1:]):
        d = b - a
        t = np.clip(((pts - a) @ d) / max(float(d @ d), 1e-12), 0.0, 1.0)
        best = np.minimum(best, np.linalg.norm(pts - (a + t[:, None] * d), axis=1))
    return best


def _road_distance(area, pts):
    """Meters from each point to the nearest road's or ramp's edge (negative on it)."""
    best = np.full(len(pts), np.inf)
    for road in list(area.roads) + list(area.ramps):
        best = np.minimum(best, _polyline_distance(pts, road['pts']) - road['width'] * 0.5)
    return best


def _slope_at(area, pts, e=0.6):
    x, y = pts[:, 0], pts[:, 1]
    gx = (area.height(x + e, y) - area.height(x - e, y)) / (2.0 * e)
    gy = (area.height(x, y + e) - area.height(x, y - e)) / (2.0 * e)
    return np.degrees(np.arctan(np.hypot(gx, gy)))


def _cm(pts):
    return [[int(round(float(x) * 100.0)), int(round(float(y) * 100.0))] for x, y in pts]


def crest_junipers(area, rng):
    """[x, y, yaw] (m, degrees): a single wind-sheared juniper every CREST_STEP m along each regional ridge's crest
    inside the core, on the crest's highest ground across the ridge, its swept crown west (yaw near 0)."""
    region = area.region
    found = []
    limit = area.half - 6.0
    for r in region.ridges:
        foot = r['foot']
        s = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(foot, axis=0), axis=1))])
        d = np.gradient(foot, axis=0)
        d /= np.maximum(np.linalg.norm(d, axis=1, keepdims=True), 1e-9)
        into = np.column_stack([-d[:, 1], d[:, 0]]) * r['side']  # toward the ridge (area_region's polygon side)
        across = np.arange(-12.0, 12.01, 0.5)
        t = float(rng.uniform(*CREST_STEP)) * 0.5
        while t < s[-1]:
            k = min(int(np.searchsorted(s, t)), len(foot) - 1)
            t += float(rng.uniform(*CREST_STEP))
            line = foot[k] + into[k] * (r['depth'] + across)[:, None]
            inside = np.maximum(np.abs(line[:, 0]), np.abs(line[:, 1])) < limit
            if not inside.all():
                continue
            if region.lip is not None and float(np.min(area.at(area.dl, line[:, 0], line[:, 1]))) < 4.0:
                continue
            z = area.height(line[:, 0], line[:, 1])
            found.append(line[int(np.argmax(z))])
    if not found:
        return []
    pts = np.array(found)
    keep = (_slope_at(area, pts) < 26.0) & (_road_distance(area, pts) > 4.0)
    kept = []
    for p in pts[keep]:
        if all(np.hypot(*(p - q)) >= CREST_STEP[0] * 0.8 for q in kept):
            kept.append(p)
    return [[x, y, int(round(rng.uniform(-CREST_YAW, CREST_YAW)))] for x, y in _cm(kept)]


def _pit_floor(area, p, pts):
    """For points: whether each is on the pit's floor, and its distance (m) in from the wall's foot."""
    rise = area.at(p['rise'], pts[:, 0], pts[:, 1])
    from_wall = -area.at(p['sd'], pts[:, 0], pts[:, 1]) - p['width'] * 0.5
    return rise > 0.97, from_wall


def pit_scrub(area, rng):
    """Each pit's floor: dry tufts in patches (0.3-0.5 per m² in them, a few between), small sage near the walls,
    none on its ramps or within 0.7 m of a sage; and a juniper or two on its rim, clear of its ramp's head, the rock
    models in its wall's gaps, the fences and obstacles round it and the roads. Returns (tufts, sage, rim junipers)."""
    tufts, sage, rim = [], [], []
    for p in getattr(area, 'pits', []):
        poly = p['poly']
        lo, hi = poly.min(axis=0) - 1.0, poly.max(axis=0) + 1.0
        ramps = [r for r in area.ramps if r.get('plateau') == p['id']]

        def off_ramps(pts, extra):
            ok = np.ones(len(pts), dtype=bool)
            for r in ramps:
                ok &= _polyline_distance(pts, r['pts']) > r['width'] * 0.5 + extra
            return ok
        cand = _rng_points(rng, lo, hi, PIT_SAGE_SPACING)
        floor, from_wall = _pit_floor(area, p, cand)
        ok = floor & (from_wall > 0.6) & (from_wall < 4.5) & off_ramps(cand, 1.5)
        ok &= rng.random(len(cand)) < 0.035 * PIT_SAGE_SPACING ** 2
        here_sage = cand[ok]
        cand = _rng_points(rng, lo, hi, PIT_TUFT_SPACING)
        floor, from_wall = _pit_floor(area, p, cand)
        patch = _ss(-0.05, 0.25, fbm(cand[:, 0], cand[:, 1], 6.0, seed=711, octaves=2))
        chance = (0.06 + 0.42 * patch) * PIT_TUFT_SPACING ** 2
        ok = floor & (from_wall > 0.2) & off_ramps(cand, 0.8) & (rng.random(len(cand)) < chance)
        for s_pt in here_sage:
            ok &= np.hypot(cand[:, 0] - s_pt[0], cand[:, 1] - s_pt[1]) > 0.7
        tufts += _cm(cand[ok])
        sage += _cm(here_sage)
        rim += _rim_junipers(area, p, ramps)
    return tufts, sage, rim


def _rim_junipers(area, p, ramps):
    """Up to RIM_JUNIPERS spots a few meters out from a pit's rim, the clearest first, then the clearest at least
    20 m from those already chosen."""
    poly = resample(p['poly'], 2.0, closed=True)
    gx, gy = np.gradient(p['sd'])  # the signed distance grows outward
    out = np.column_stack([area.at(gx, poly[:, 0], poly[:, 1]), area.at(gy, poly[:, 0], poly[:, 1])])
    out /= np.maximum(np.linalg.norm(out, axis=1, keepdims=True), 1e-9)
    cand = poly + out * (p['width'] * 0.5 + 3.5)
    clear = np.full(len(cand), 30.0)
    for r in ramps:
        clear = np.minimum(clear, np.hypot(*(cand - r['pts'][0]).T) - 10.0)
        clear = np.minimum(clear, _polyline_distance(cand, r['pts']) - r['width'] * 0.5 - 2.0)
    for gap in p['feature'].get('cliffGaps', []):
        clear = np.minimum(clear, np.hypot(*(cand - to_m([gap['center']])[0]).T) - gap['radius'] / 100.0 - 3.0)
    for ob in area.layout.get('obstacles', []):
        line = to_m(ob.get('polygon') or ob.get('path') or [])
        if len(line) < 2:
            continue
        if 'polygon' in ob:
            line = np.vstack([line, line[:1]])
        clear = np.minimum(clear, _polyline_distance(cand, line) - 2.5)
    clear = np.minimum(clear, _road_distance(area, cand) - 4.0)
    if area.dl is not None:
        clear = np.minimum(clear, area.at(area.dl, cand[:, 0], cand[:, 1]) - 2.0)
    clear = np.where(_slope_at(area, cand) < 22.0, clear, -1.0)
    chosen = []
    for _ in range(RIM_JUNIPERS):
        score = clear.copy()
        for c in chosen:
            score = np.where(np.hypot(*(cand - c).T) < 20.0, -1.0, score)
        k = int(np.argmax(score))
        if score[k] <= 0.0:
            break
        chosen.append(cand[k])
    return [[x, y, 0] for x, y in _cm(chosen)]


def points(area):
    """layout_computed.json's "scrub" lists (cm; yaw in degrees), or None for an area without regional ridges."""
    region = getattr(area, 'region', None)
    if region is None or not region.ridges:
        return None
    rng = np.random.default_rng(29)
    crests = crest_junipers(area, rng)
    tufts, sage, rim = pit_scrub(area, rng)
    return {'crestJunipers': crests, 'pitTufts': tufts, 'pitSage': sage, 'rimJunipers': rim}
