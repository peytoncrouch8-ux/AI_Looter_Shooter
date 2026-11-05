"""The erosion's masks (area_erosion.py; numpy only): the ground erosion must leave as it is, from the area's own data,
and the protection weight built from them. Every kept cell keeps its height bit for bit (the weight is exactly 0
there); outside them the weight eases from 0 to 1 over erosion.feather (cm, an 8-neighbour chamfer distance on
half-meter blocks), and fades to 0 across a grounded core's seam band.

The masks (each a name in MASKS, its margin in cm erosion.masks.<name> unless said):
- roads: every carved road and its shoulders (area.road_gap under the widest shoulder + roads); tracks: the paint-only
  roads ("paint": true, which the shape leaves out), their half width + roads + 1 m;
- ramps (the paths up cliffs with their cut and fill), steps (the Goat Steps' flight), trails (layout trails);
- pads: every pad with its blend or bank (area.pad_weight); footprints: buildings' flat footprints + footprints;
- placements: every placement (half its BUILDINGS diagonal, else 1.4 m, + placements) and level.props;
- town: town.json's pieces, deferred pieces, picket lines, no-tree boxes and spots (+ town);
- obstacles: fences, walls, graves, cairns, biers, ruins, props, outcrops and boulder groups (+ obstacles); tree
  stands ("trees") only inside the playable boundary (past it they're the scatter's woods on the ridges' faces,
  which re-trace the ground: erosion carves there);
- knobs: the outcrops' level seats with their blend (+ knobs);
- zones: every zone but forest and alpine kinds (FREE_ZONES), + zones; encounters: the encounters' grounds, spots,
  crates, triggers, routes and lurks' perches (gameplay.encounters; "frame": "mine" ones are inside the mine), the
  keepOut zones; spots: roosts, boards, finds, the foreman's stone, the mine's mouth (+ spots, more for the last two);
- openGround: the ground kept open on purpose (+ zones); towns: erosion.keep polygons (+ towns) and erosion.townZones
  with masks.townSkirt (30 m: no gully across a road near town, none under Main Street's walks; the Towns plan);
- holes: mesh.holes and boundary.interiors (+ holes); water: every wet cell grown by water, the ponds' shores and
  the creeks' banks to water past them, the gorges;
- cliffs: the designed steep ground (area.designed: the features' cliffs and walls, steps, channel walls, pad flats)
  and the plateaus' and pits' rims; designed: that grown by designed; gullies: the dry gullies' beds and banks;
- bands: the regional ridges' cliff bands, a strip from 4 m out past the foot to bands past the band's top;
- lip: the escarpment's lip (and everything past it); boundary: the playable boundary's line, from boundaryOut past
  it to boundaryIn inside it (the rock past closed edges is gameplay: terrain_check's 44-degree rise); foot: where
  boundary.foot lifted the ground (area.foot_lift), grown 2 m.
"""
import json
import math
import os

import numpy as np

from area_math import catmull_rom, resize, sample, smoothstep

# Zone kinds that don't protect their ground: woods and the Bench's meadow (no site of their own; their sites are).
FREE_ZONES = ('forest', 'alpine')
# The 8 neighbours (di, dj) of a cell; di along x (north), dj along y (east).
OFFSETS = ((-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1))
# Mask names, in label-bit order (area.erosion['labels'] keeps one bit per mask on the working grid).
MASKS = ('roads', 'ramps', 'steps', 'trails', 'pads', 'footprints', 'placements', 'town', 'obstacles', 'knobs',
         'zones', 'encounters', 'spots', 'openGround', 'holes', 'water', 'designed', 'cliffs', 'gullies', 'bands',
         'lip', 'boundary', 'foot', 'towns', 'tracks')


class _Keep:
    """The cells (full raster) that keep their heights, and a label bit per mask on the working grid."""

    def __init__(self, area):
        self.area, self.grid = area, area.grid
        self.keep = np.zeros((area.grid.n, area.grid.n), dtype=bool)
        self.parts = {}

    def _part(self, name):
        if name not in self.parts:
            self.parts[name] = np.zeros_like(self.keep)
        return self.parts[name]

    def add(self, name, mask, window=None):
        """Keeps the cells of a mask (a full raster, or the block of rows i0:i1, columns j0:j1 in window)."""
        part = self._part(name)
        if window is None:
            self.keep |= mask
            part |= mask
        else:
            i0, i1, j0, j1 = window
            self.keep[i0:i1, j0:j1] |= mask
            part[i0:i1, j0:j1] |= mask

    def _window(self, lo, hi):
        g = self.grid
        i0, i1 = g.index_range(lo[0], hi[0])
        j0, j1 = g.index_range(lo[1], hi[1])
        return (i0, i1, j0, j1) if i0 < i1 and j0 < j1 else None

    def disc(self, name, center, radius):
        cx, cy = float(center[0]), float(center[1])
        window = self._window((cx - radius, cy - radius), (cx + radius, cy + radius))
        if window is None:
            return
        x, y = self.grid.mesh(*window)
        self.add(name, np.hypot(x - cx, y - cy) <= radius, window)

    def path(self, name, pts, radius, closed=False):
        """The cells within radius (m) of a polyline, segment by segment in each one's own window."""
        pts = np.atleast_2d(np.asarray(pts, dtype=np.float64))
        if len(pts) == 1:
            return self.disc(name, pts[0], radius)
        if closed:
            pts = np.vstack([pts, pts[:1]])
        for a, b in zip(pts[:-1], pts[1:]):
            window = self._window(np.minimum(a, b) - radius, np.maximum(a, b) + radius)
            if window is None:
                continue
            x, y = self.grid.mesh(*window)
            d = b - a
            t = np.clip(((x - a[0]) * d[0] + (y - a[1]) * d[1]) / max(float(d @ d), 1e-12), 0.0, 1.0)
            self.add(name, np.hypot(x - a[0] - t * d[0], y - a[1] - t * d[1]) <= radius, window)

    def polygon(self, name, poly, margin):
        poly = np.asarray(poly, dtype=np.float64)
        lo, hi = poly.min(axis=0), poly.max(axis=0)
        window = self._window(lo - 1.0, hi + 1.0)
        if window is not None:
            i0, i1, j0, j1 = window
            px = self.grid.px
            y0 = -self.grid.half + j0 * px  # the window's west edge
            inside = np.zeros((i1 - i0, j1 - j0), dtype=bool)
            a, b = poly, np.roll(poly, -1, axis=0)
            for r, x in enumerate(self.grid.c[i0:i1]):
                # Even-odd scanline fill, as area_math.polygon_mask does it.
                cross = (a[:, 0] <= x) != (b[:, 0] <= x)
                if not cross.any():
                    continue
                ea, eb = a[cross], b[cross]
                ys = np.sort(ea[:, 1] + (x - ea[:, 0]) / (eb[:, 0] - ea[:, 0]) * (eb[:, 1] - ea[:, 1]))
                for k in range(0, len(ys) - 1, 2):
                    c0 = int(max(0, math.ceil((ys[k] - y0) / px - 0.5)))
                    c1 = int(min(j1 - j0 - 1, math.floor((ys[k + 1] - y0) / px - 0.5)))
                    if c1 >= c0:
                        inside[r, c0:c1 + 1] = True
            self.add(name, inside, window)
        if margin > 0.0:
            self.path(name, poly, margin, closed=True)


def _m(cfg, key):
    return cfg['masks'][key] / 100.0


def _pts(points):
    return np.asarray([p[:2] for p in points], dtype=np.float64) / 100.0


def protection(area, cfg, m, log=print):
    """The protection weight on the height raster (float32: 0 inside every mask, easing to 1 over the feather, fading
    across the seam band) and the masks' labels on an m x m grid over the map square (uint32, a bit per name in
    MASKS)."""
    keep = _Keep(area)
    layout = area.layout
    # Roads (dirt, paths, rail beds) and their shoulders: road_gap is meters past the nearest road's edge.
    widest = max([r['shoulder'] for r in area.roads] + [0.0])
    keep.add('roads', area.road_gap < widest + _m(cfg, 'roads'))
    for r in area.ramps:
        keep.path('ramps', r['pts'], r['width'] * 0.5 + _m(cfg, 'ramps'))
    for st in getattr(area, 'steps', []):
        keep.path('steps', st['pts'], st['width'] * 0.5 + _m(cfg, 'steps'))
    for a, b in layout.get('trails', []):
        keep.path('trails', np.array([area.point(a), area.point(b)]), _m(cfg, 'trails'))
    keep.add('pads', area.pad_weight > 1e-3)
    for f in area.footprints:
        keep.disc('footprints', f['center'], f['radius'] + f['blend'] + _m(cfg, 'footprints'))
    from area_shape import BUILDINGS
    for p in layout['placements']:
        length, width = BUILDINGS.get(p['kind'], (2.0, 2.0))
        keep.disc('placements', np.asarray(p['location'][:2]) / 100.0,
                  0.5 * math.hypot(length, width) + _m(cfg, 'placements'))
    for prop in layout.get('level', {}).get('props', []):
        keep.disc('placements', np.asarray(prop['location'][:2]) / 100.0, 3.0 + _m(cfg, 'placements'))
    _town(area, keep, cfg)
    woods = []
    for o in layout.get('obstacles', []):
        if o['kind'] == 'trees':
            woods.append(o)  # below, only inside the playable boundary
        elif 'polygon' in o:
            keep.polygon('obstacles', catmull_rom(_pts(o['polygon']), closed=True, step=0.5)
                         if len(o['polygon']) > 2 else _pts(o['polygon']), _m(cfg, 'obstacles'))
        elif 'path' in o:
            keep.path('obstacles', _pts(o['path']), _m(cfg, 'obstacles'))
    if woods:
        stands = _Keep(area)
        for o in woods:
            if 'polygon' in o:
                stands.polygon('trees', catmull_rom(_pts(o['polygon']), closed=True, step=0.5), _m(cfg, 'obstacles'))
            else:
                stands.path('trees', _pts(o['path']), _m(cfg, 'obstacles'))
        inside = area.play_edge > 0.0 if area.setting == 'grounded' else np.ones_like(stands.keep)
        keep.add('obstacles', stands.keep & inside)
        del stands
    for kn in getattr(area, 'knobs', []):
        keep.disc('knobs', kn['center'], kn['radius'] + kn['blend'] + _m(cfg, 'knobs'))
    zones = {z['id']: z for z in layout.get('zones', [])}
    for z in zones.values():
        if z.get('kind', z['id']) not in FREE_ZONES:
            keep.polygon('zones', catmull_rom(_pts(z['polygon']), closed=True, step=0.5), _m(cfg, 'zones'))
    _encounters(area, keep, cfg, zones)
    for spot in layout.get('openGround', {}).get('open', []):
        keep.polygon('openGround', _pts(spot['polygon']), _m(cfg, 'zones'))
    # The towns lanes' ground (2026-11-04 Towns plan, section 6): erosion.keep polygons (lots, back-lot boxes), the
    # town's zones with a skirt (no gully across a road near town, none under Main Street's walks) and the paint-only
    # tracks (roads with "paint": true, which the shape leaves out).
    for spot in cfg.get('keep', []):
        keep.polygon('towns', _pts(spot['polygon']), _m(cfg, 'towns'))
    for zid in cfg.get('townZones', []):
        if zid in zones:
            keep.polygon('towns', catmull_rom(_pts(zones[zid]['polygon']), closed=True, step=0.5),
                         _m(cfg, 'townSkirt'))
    from area_shape import paint_only
    for road in layout.get('roads', []):
        if paint_only(road):
            keep.path('tracks', catmull_rom(_pts(road['path']), step=0.5),
                      road['width'] / 200.0 + _m(cfg, 'roads') + 1.0)
    for hole in layout.get('mesh', {}).get('holes', []):
        keep.polygon('holes', _pts(hole['polygon']), _m(cfg, 'holes'))
    for interior in layout.get('boundary', {}).get('interiors', []):
        keep.polygon('holes', _pts(interior['polygon']), _m(cfg, 'holes'))
    _water(area, keep, cfg)
    # Designed steep ground (the features' cliffs and walls, steps, channel walls, pad flats) and a strip round it.
    designed = area.designed.copy()
    for p in area.plateaus + area.pits:
        designed |= (p['rise'] > 0.01) & (p['rise'] < 0.995)
        designed |= np.abs(p['sd']) < p['width'] * 0.5 + 3.0
    keep.add('cliffs', designed)
    keep.add('designed', _grow(designed, area.grid.px, _m(cfg, 'designed')))
    for g in getattr(area, 'gullies', []):
        keep.path('gullies', g['pts'], g['half'] + (g['depth'] + 3.0) / g['bank'] + _m(cfg, 'gullies'))
    _regional(area, keep, cfg)
    core = area.edge > 0.0  # the core's own ground (past the escarpment's lip is the canyon, never meshed)
    log('erosion: masks (share of the core) ' + ', '.join(f'{name} {100.0 * float(np.mean(mask[core])):.1f}%'
                                                          for name, mask in keep.parts.items())
        + f'; all {100.0 * float(np.mean(keep.keep[core])):.1f}%')
    # The feather: the distance (m) from the kept cells, on a coarse grid, eased over erosion.feather.
    feather = cfg['feather'] / 100.0
    weight = smoothstep(0.0, feather, _distance(keep.keep, area.grid.px, feather)).astype(np.float32)
    weight[keep.keep] = 0.0
    if area.setting == 'grounded':
        x, y = area.grid.mesh()
        inside = area.half - np.maximum(np.abs(x), np.abs(y))
        weight *= smoothstep(0.0, area.region.seam, inside).astype(np.float32)
        del x, y, inside
    labels = _labels(keep, area.grid.n, m)
    return weight, labels


def _labels(keep, n, m):
    """A bit per mask (MASKS order) on the working grid: set where any of its raster cells is in that mask."""
    out = np.zeros((m, m), dtype=np.uint32)
    f = n // m if n % m == 0 else None
    for bit, name in enumerate(MASKS):
        mask = keep.parts.get(name)
        if mask is None:
            continue
        if f:
            coarse = mask.reshape(m, f, m, f).any(axis=(1, 3))
        else:
            coarse = resize(mask.astype(np.float32), m, keep.area.half) > 0.0
        out |= coarse.astype(np.uint32) << np.uint32(bit)
    return out


def _coarse_distance(keep, px, reach, coarse=0.5):
    """The distance (m) from the kept cells out to reach, on blocks of about coarse m (a block holding any kept cell
    is at 0; an 8-neighbour chamfer), and the block's size in cells."""
    n = keep.shape[0]
    f = max(1, int(round(coarse / px)))
    while n % f:
        f -= 1
    m = n // f
    c = f * px
    d = np.where(keep.reshape(m, f, m, f).any(axis=(1, 3)), 0.0, np.inf).astype(np.float32)
    for _ in range(int(math.ceil(reach / c)) + 1):
        p = np.pad(d, 1, mode='edge')
        for di, dj in OFFSETS:
            np.minimum(d, p[1 + di:1 + di + m, 1 + dj:1 + dj + m] + np.float32(c * math.hypot(di, dj)), out=d)
    return np.minimum(d, np.float32(reach + c)), f


def _grow(mask, px, meters):
    """A boolean raster grown by meters (measured on half-meter blocks: up to a block more)."""
    if meters <= 0.0:
        return mask
    d, f = _coarse_distance(mask, px, meters)
    grown = d <= meters
    return (np.repeat(np.repeat(grown, f, axis=0), f, axis=1) if f > 1 else grown) | mask


def _distance(keep, px, reach):
    """The distance (m) from the kept cells out to reach, resampled bilinear onto the raster."""
    d, f = _coarse_distance(keep, px, reach)
    return upsample(d, f)


def upsample(coarse, f):
    """A raster on blocks of f x f cells resampled bilinear onto the cells (area_math.sample's arithmetic, done
    separably)."""
    if f == 1:
        return coarse
    m = coarse.shape[0]
    u = (np.arange(m * f) + 0.5) / f - 0.5
    u = np.clip(u, 0.0, m - 1.000001)
    i0 = np.floor(u).astype(np.int64)
    t = (u - i0).astype(np.float32)
    i1 = np.minimum(i0 + 1, m - 1)
    rows = coarse[i0] * (1.0 - t)[:, None] + coarse[i1] * t[:, None]
    return (rows[:, i0] * (1.0 - t)[None, :] + rows[:, i1] * t[None, :]).astype(np.float32)


def _town(area, keep, cfg):
    """The town's dressing (layout.json level.town: a file beside the layout): its pieces, picket lines, no-tree
    boxes and spots."""
    name = area.layout.get('level', {}).get('town')
    if not name:
        return
    path = os.path.join(area.folder, name)
    if not os.path.exists(path):
        return
    with open(path, encoding='utf-8') as file:
        town = json.load(file)
    r = _m(cfg, 'town')
    for piece in town.get('pieces', []) + town.get('deferredPieces', []):
        if 'at' in piece:
            keep.disc('town', np.asarray(piece['at'][:2]) / 100.0, r)
    for line in town.get('lines', []):
        keep.path('town', _pts(line['points']), r, closed=bool(line.get('closed')))
    for box in town.get('noTrees', []):
        keep.polygon('town', _pts(box['polygon']), r)
    for spot in town.get('spots', []):
        if 'polygon' in spot:
            keep.polygon('town', _pts(spot['polygon']), r)
        elif 'at' in spot:
            keep.disc('town', np.asarray(spot['at'][:2]) / 100.0, r)


def _ground(area, keep, cfg, zones, ground, around=None, name='encounters'):
    """An encounter's ground ({polygon} | {zone} | {obstacle, margin} | {radius} round around, cm)."""
    if not ground:
        return
    r = _m(cfg, 'encounters')
    obstacles = {o['id']: o for o in area.layout.get('obstacles', [])}
    if 'polygon' in ground:
        keep.polygon(name, _pts(ground['polygon']), r)
    elif 'zone' in ground and ground['zone'] in zones:
        keep.polygon(name, catmull_rom(_pts(zones[ground['zone']]['polygon']), closed=True, step=0.5), r)
    elif 'obstacle' in ground and ground['obstacle'] in obstacles:
        o = obstacles[ground['obstacle']]
        margin = ground.get('margin', 0) / 100.0 + r
        if 'polygon' in o:
            keep.polygon(name, _pts(o['polygon']), margin)
        else:
            keep.path(name, _pts(o['path']), margin)
    elif 'radius' in ground and around is not None:
        keep.disc(name, around, ground['radius'] / 100.0 + r)


def _encounters(area, keep, cfg, zones):
    """Where the encounters play: layout.json gameplay.encounters (frame "mine" ones are inside the mine, in its own
    coordinates: the mine's mouth and the boundary's interiors cover them), the roosts, boards, finds and the
    mountain's spots."""
    gameplay = area.layout.get('gameplay', {})
    enc = gameplay.get('encounters', {})
    r = _m(cfg, 'encounters')
    obstacles = {o['id']: o for o in area.layout.get('obstacles', [])}
    for zone in enc.get('keepOut', []):
        if zone.get('zone') in zones:
            keep.polygon('encounters', catmull_rom(_pts(zones[zone['zone']]['polygon']), closed=True, step=0.5), r)
    for f in enc.get('storyFights', []):
        if 'polygon' in f:
            keep.polygon('encounters', _pts(f['polygon']), r)
        if 'zone' in f or 'obstacle' in f:
            _ground(area, keep, cfg, zones, {k: f[k] for k in ('zone', 'obstacle') if k in f})
        if 'at' in f:
            keep.disc('encounters', np.asarray(f['at'][:2]) / 100.0, f.get('radius', 1500) / 100.0 + r)
        if 'between' in f:
            ends = [obstacles[b] for b in f['between'] if b in obstacles]
            if ends:
                center = np.mean(np.vstack([_pts(o.get('polygon', o.get('path'))) for o in ends]), axis=0)
                keep.disc('encounters', center, f.get('radius', 1500) / 100.0 + r)
    for group in ('camps', 'ambushes', 'fights'):
        for e in enc.get(group, []):
            if e.get('frame') == 'mine':
                continue
            center = np.asarray(e['center'][:2]) / 100.0 if 'center' in e else None
            spots = [s for s in e.get('spots', []) if isinstance(s, (list, tuple))] if isinstance(
                e.get('spots'), list) else []
            if center is None and spots:
                center = _pts(spots).mean(axis=0)
            _ground(area, keep, cfg, zones, e.get('ground'), center)
            for key in ('trigger',):
                if isinstance(e.get(key), dict):
                    _ground(area, keep, cfg, zones, e[key], center)
            for s in spots:
                keep.disc('encounters', np.asarray(s[:2]) / 100.0, e.get('riseRadius', 300) / 100.0 + r)
            crates = e.get('crate')
            if isinstance(crates, list):
                for c in crates:
                    keep.disc('encounters', np.asarray(c[:2]) / 100.0, r + 1.0)
            if 'route' in e:
                keep.path('encounters', _pts(e['route']), e.get('corridor', 600) / 200.0 + r)
    for e in enc.get('patrols', []):
        keep.path('encounters', _pts(e['route']), e.get('corridor', 600) / 200.0 + r)
    for e in enc.get('lurks', []):
        perches = e.get('perches') if isinstance(e.get('perches'), list) else []
        for perch in perches:
            keep.disc('encounters', np.asarray(perch['at'][:2]) / 100.0,
                      e.get('ground', {}).get('radius', 600) / 100.0 + r)
        if isinstance(e.get('ground'), dict) and 'radius' not in e['ground']:
            _ground(area, keep, cfg, zones, e['ground'])
    spot = _m(cfg, 'spots')
    for roost in gameplay.get('roosts', []):
        keep.disc('spots', np.asarray(roost['at'][:2]) / 100.0, spot)
    for board in gameplay.get('boards', []):
        keep.disc('spots', np.asarray(board['at'][:2]) / 100.0, spot)
    for find in gameplay.get('finds', {}).get('spots', []):
        for key in ('at', 'near'):
            if isinstance(find.get(key), list):
                keep.disc('spots', np.asarray(find[key][:2]) / 100.0, spot)
        for sub in find.values():
            if isinstance(sub, dict) and isinstance(sub.get('near'), list):
                keep.disc('spots', np.asarray(sub['near'][:2]) / 100.0, spot + 4.0)
    mountain = gameplay.get('mountain', {})
    if isinstance(mountain.get('foremanStone'), list):
        keep.disc('spots', np.asarray(mountain['foremanStone'][:2]) / 100.0, spot + 2.0)
    if isinstance(mountain.get('mine', {}).get('origin'), list):
        keep.disc('spots', np.asarray(mountain['mine']['origin'][:2]) / 100.0, spot + 6.0)


def _water(area, keep, cfg):
    """Ponds, creeks (their banks and valleys), the gorges and every wet cell, with a margin."""
    r = _m(cfg, 'water')
    surface = area.water_surface()
    wet = np.isfinite(surface)
    keep.add('water', _grow(wet, area.grid.px, r))
    for p in area.ponds:
        keep.add('water', p['beyond'] < r)
    for c in area.creeks:
        keep.add('water', np.isfinite(c['dist']) & (c['dist'] < 3.2 + r))
    for g in getattr(area, 'gorges', []):
        keep.path('water', g['pts'], g['half'] + r)


def _regional(area, keep, cfg):
    """Grounded: the ridges' cliff bands (their dressed rock: a strip from the foot to past the band's top), the
    escarpment's lip, the playable boundary's line both sides, and where boundary.foot lifted the ground."""
    if area.setting != 'grounded':
        return
    region = area.region
    grid = area.grid
    if region.ridges:
        band = np.zeros(keep.keep.shape, dtype=bool)
        r = _m(cfg, 'bands')
        for a in range(0, grid.n, 256):
            x, y = grid.mesh(a, min(grid.n, a + 256))
            for i, ridge in enumerate(region.ridges):
                if ridge['band'] <= 0.0:
                    continue
                # The field is clipped at 4 m out on the valley's side (area_region.build_fields), so the strip
                # reaches at most that far out past the foot (its wobble is about 1.5 m).
                s = sample(region.fields[f'ridge{i}'], x, y, region.half)
                band[a:a + 256] |= (s > -3.99) & (s < ridge['band_width'] + 1.5 + r)
        keep.add('bands', band)
    if area.dl is not None:
        keep.add('lip', area.dl < _m(cfg, 'lip'))
    if area.boundary is not None:
        keep.add('boundary', (area.play_edge > -_m(cfg, 'boundaryOut')) & (area.play_edge < _m(cfg, 'boundaryIn')))
    lift = getattr(area, 'foot_lift', None)
    if lift is not None:
        keep.add('foot', _grow(lift > 0.01, grid.px, 2.0))


