"""Values the placement scripts need, measured on the built area: Art/Levels/<Area>/layout_computed.json. Everything in
it is Unreal world centimeters (X north, Y east, Z up) and yaw in degrees (0 = +X, 90 = +Y), like layout.json.
compute() returns the dictionary, write() saves it.

The squares the macro map and the scatter mask cover are macroMap.covers and macroMap.scatterMap.covers
(Tools/Unreal/build_island_scatter.py reads the mask's); a grounded area's ring has its own map, ringMap. Cliff dressing
comes in groups, one per feature with cliffs (a plateau's or mesa's cliff, a pit's wall, a ramp's walls, a ridge's or
scarp's face, a knob's rock, a gully's banks, a gorge's walls, a regional ridge's band, the rock raised past the
playable boundary, the escarpment's lip) plus the island's rim; area_cliffs.py has the points' format (stacked courses
above 12 m) and Tools/Unreal/build_area.py dresses every group. ponds, creeks, ramps, bridges, gullies and waterfalls
hold every one by id; the singular pond, creek, ramp, bridge and waterfall describe the first of each, as before. A
grounded area's playable boundary is "boundary" (corners and open edges, as APlayableArea takes them, and "rise": how
the ground climbs past its closed edges, area_boundary.py), and every area gets the open-ground metric, "openGround"
(area_open.py).
"""
import json
import math

import numpy as np

import area_boundary
import area_cliffs
from area_math import arc_length, points_in_polygon
from area_shape import CREEK_WATER_HALF, layout_sha1, to_m

CLIFF_STEP = 10.0     # meters between cliff dressing points
ORCHARD_SPACING = 4.5


def _cm(v):
    return round(float(v) * 100.0, 1)


def _xyz(area, x, y, z=None):
    return [_cm(x), _cm(y), _cm(area.height(x, y) if z is None else z)]


def _yaw(dx, dy):
    return round(math.degrees(math.atan2(dy, dx)), 1)


def _number(value):
    """A layout number as people write it: 10240, not 10240.0."""
    value = float(value)
    return str(int(value)) if value.is_integer() else repr(value)


def _intersect(p, q):
    """The first crossing of polylines p and q: (point, index into p, fraction, index into q, fraction) or None."""
    for i in range(len(p) - 1):
        a, b = p[i], p[i + 1]
        c, d = q[:-1], q[1:]
        r, s = b - a, d - c
        denom = r[0] * s[:, 1] - r[1] * s[:, 0]
        ok = np.abs(denom) > 1e-12
        t = np.where(ok, ((c[:, 0] - a[0]) * s[:, 1] - (c[:, 1] - a[1]) * s[:, 0]) / np.where(ok, denom, 1.0), -1.0)
        u = np.where(ok, ((c[:, 0] - a[0]) * r[1] - (c[:, 1] - a[1]) * r[0]) / np.where(ok, denom, 1.0), -1.0)
        hit = np.nonzero((t >= 0.0) & (t <= 1.0) & (u >= 0.0) & (u <= 1.0))[0]
        if len(hit):
            j = int(hit[0])
            return a + r * t[j], i, float(t[j]), j, float(u[j])
    return None


def _road(area, road_id):
    return next((r for r in area.roads if r['id'] == road_id), None)


def _creek(area, creek_id):
    found = next((c for c in area.creeks if c['id'] == creek_id), None)
    if found is None:
        # A gully (a dry creek): its bed stands in for the water.
        gully = next((g for g in getattr(area, 'gullies', []) if g['id'] == creek_id), None)
        if gully is not None:
            found = dict(gully, water=gully['bed'])
    return found


def bridge(area):
    """The layout's first bridge (layout.json "bridges": where a road crosses a creek or a gully)."""
    spec = next(iter(area.layout.get('bridges', [])), None)
    return bridge_of(area, spec) if spec is not None else None


def bridge_of(area, spec):
    road = _road(area, spec['road'])
    creek = _creek(area, spec['creek'])
    if road is None or creek is None:
        return None
    found = _intersect(road['pts'], creek['pts'])
    if found is None:
        return None
    point, i, t, _, _ = found
    s_road = road['s'][i] + t * (road['s'][i + 1] - road['s'][i])
    deck = float(np.interp(s_road, road['s'], road['z']))
    direction = road['pts'][min(i + 2, len(road['pts']) - 1)] - road['pts'][max(i - 2, 0)]
    direction /= np.linalg.norm(direction)
    # The span: out along the road until the ground is back up to the deck on both sides, plus a meter each way. Over
    # a dry gully it ends at the top of the gully's banks too, where the road runs on lower than the deck (down off a
    # terrace, as the keeper's path does over the Dry Wash).
    channel = None
    if 'bank' in creek:
        bed = float(np.interp(_along(creek, point), creek['s'], creek['bed']))
        channel = creek['half'] + max(deck - bed, 0.0) / creek['bank'] + 0.5
    reach = []
    for sign in (-1.0, 1.0):
        d = 0.5
        while d < 15.0 and area.height(*(point + sign * direction * d)) < deck - 0.12:
            if channel is not None and np.min(np.linalg.norm(creek['pts'] - (point + sign * direction * d),
                                                             axis=1)) > channel:
                break
            d += 0.1
        reach.append(d)
    water = float(np.interp(_along(creek, point), creek['s'], creek['water']))
    center = point + direction * (reach[1] - reach[0]) * 0.5
    return {
        'id': spec['id'], 'road': spec['road'], 'creek': spec['creek'],
        'location': [_cm(center[0]), _cm(center[1]), _cm(deck)],
        'yaw': _yaw(*direction), 'span': _cm(sum(reach) + 2.0), 'width': _cm(road['width']),
        'waterZ': _cm(water), 'bedZ': _cm(area.height(*point)),
        'note': 'location is the middle of the span at deck height (the road surface on both banks); '
                'yaw runs along the road; span is bank to bank plus a meter each side',
    }


def _along(curve, point):
    d = np.linalg.norm(curve['pts'] - point, axis=1)
    return float(curve['s'][int(np.argmin(d))])


def waterfall(area):
    """Where the first creek that ends in a waterfall runs off the rim (island setting: it falls to the underside's
    depth), or else the first falls into a gorge."""
    if area.setting == 'grounded':
        return next(iter(waterfalls(area).values()), None)
    creek = next((c for c in area.creeks if c['feature'].get('waterfallAtEnd')), None)
    if creek is None:
        return None
    k = creek['k_lip']
    seg = creek['pts'][max(k - 1, 0):k + 1]
    found = _intersect(seg, np.vstack([area.outline, area.outline[:1]]))
    point = found[0] if found else creek['pts'][k]
    s = _along(creek, point)
    direction = creek['pts'][min(k + 1, len(creek['pts']) - 1)] - creek['pts'][max(k - 3, 0)]
    water = float(np.interp(s, creek['s'], creek['water']))
    return {
        'location': [_cm(point[0]), _cm(point[1]), _cm(water)], 'yaw': _yaw(*direction),
        'width': _cm(2.0 * CREEK_WATER_HALF), 'bedZ': _cm(area.height(*point)),
        'dropTo': _cm(water - area.island['undersideDepth'] / 100.0),
        'note': 'the lip: where the creek runs off the rim, at the water surface; yaw is the flow direction',
    }


def waterfalls(area):
    """Every creek's falls into its gorge, by the creek's id: the lip at the water surface, the way the water goes,
    and dropTo, the stream on the gorge's floor below."""
    found = {}
    for g in getattr(area, 'gorges', []):
        lip, direction = g['lip'], g['direction']
        found[g['creek']] = {
            'location': [_cm(lip[0]), _cm(lip[1]), _cm(g['lip_water'])], 'yaw': _yaw(*direction),
            'width': _cm(2.0 * CREEK_WATER_HALF), 'bedZ': _cm(area.height(*lip)),
            'dropTo': _cm(g['floor'][0] + 0.25), 'gorge': g['id'],
            'note': 'the lip: where the creek falls into its gorge, at the water surface; yaw is the flow direction',
        }
    return found


def plateau_cliffs(area, p):
    """Points along a plateau's cliff (every CLIFF_STEP m): where the cliff face meets the ground below, its top,
    and the outward direction. None on the rim (that's the underside's) or across a ramp."""
    poly = p['poly']
    s = arc_length(poly, closed=True)
    normal_field = np.gradient(p['sd'])
    points = []
    for target in np.arange(0.0, s[-1], CLIFF_STEP):
        k = int(np.searchsorted(s, target))
        k = min(k, len(poly) - 1)
        a = poly[k]
        tangent = poly[(k + 1) % len(poly)] - poly[k - 1]
        n = np.array([tangent[1], -tangent[0]]) / max(np.linalg.norm(tangent), 1e-9)
        if area.at(p['sd'], *(a + n)) < area.at(p['sd'], *(a - n)):
            n = -n  # point outward (the signed distance grows outward)
        offsets = np.arange(-6.0, 6.0, 0.1)
        line = a[None, :] + offsets[:, None] * n[None, :]
        sd = area.at(p['sd'], line[:, 0], line[:, 1])
        cross = np.nonzero(np.diff(np.sign(sd)) != 0)[0]
        if not len(cross):
            continue
        c = line[cross[np.argmin(np.abs(offsets[cross]))]]
        if area.at(area.edge, *c) < 4.0 or _near_ramp(area, c, 3.0):  # island setting: the rim
            continue
        gx = area.at(normal_field[0], *c)
        gy = area.at(normal_field[1], *c)
        out = np.array([gx, gy]) / max(math.hypot(gx, gy), 1e-9)
        base = c + out * (p['width'] * 0.5 + 1.2)
        top = c - out * (p['width'] * 0.5 + 1.0)
        zb, zt = float(area.height(*base)), float(area.height(*top))
        points.append(area_cliffs.plateau_courses(area, p, {
            'location': [_cm(c[0]), _cm(c[1]), _cm(zb)], 'top': _cm(zt), 'height': _cm(zt - zb), 'yaw': _yaw(*out)}))
    return points


def _near_ramp(area, point, margin):
    for r in area.ramps:
        d = np.min(np.linalg.norm(r['pts'] - point, axis=1))
        if d < r['width'] * 0.5 + margin:
            return True
    return False


def ramp_walls(area, r, step=5.0):
    """Both walls of a ramp's cut through the cliff, where they're more than 1.5 m high: the wall's foot, its
    height and the direction it faces (toward the path). A ramp whose side drops as a cliff ("drop") gets that face
    too, after the cut's walls: a standing face from the ground below up to the ramp's edge, facing away from it."""
    walls, drops = [], []
    pts, s = r['pts'], r['s']
    half = r['width'] * 0.5
    for target in np.arange(step * 0.5, s[-1], step):
        k = int(np.searchsorted(s, target))
        tangent = pts[min(k + 1, len(pts) - 1)] - pts[max(k - 1, 0)]
        tangent /= np.linalg.norm(tangent)
        side = np.array([-tangent[1], tangent[0]])  # the right of travel, as seen on the plan
        zr = float(np.interp(target, s, r['z']))
        for sign in (-1.0, 1.0):
            d = np.arange(half, half + 9.0, 0.2)
            line = pts[k][None, :] + sign * d[:, None] * side[None, :]
            ground = area.height(line[:, 0], line[:, 1])
            rise = ground - zr
            if sign in r.get('drop', ()):
                point = _ramp_drop(area, line, ground, sign * side)
                if point:
                    drops.append(point)
            if rise.max() < 1.5:
                continue
            foot = line[int(np.argmax(rise > 0.25))]
            walls.append({'location': [_cm(foot[0]), _cm(foot[1]), _cm(zr)], 'height': _cm(rise.max()),
                          'yaw': _yaw(*(-sign * side))})
    return walls + drops


def _ramp_drop(area, line, ground, out):
    """The face where a ramp's side drops as a cliff, from the samples of a line running out from its edge: from
    where the ground starts falling steeply to where it stops. None when it's lower than a face needs."""
    steep = np.gradient(ground, 0.2) < -1.5
    if not steep.any():
        return None
    first = int(np.argmax(steep))
    last = first + int(np.argmax(~steep[first:])) if (~steep[first:]).any() else len(line) - 1
    top, foot = line[first], line[last]
    reach = float(np.linalg.norm(foot - top)) * 0.5 + 0.3
    return area_cliffs.face(area, (top + foot) * 0.5, out, reach, reach)


def rim_points(area):
    """The island's edge every CLIFF_STEP m (island setting): the top of the rock wall under it, how far it drops
    before the underside tapers in, and the outward direction. The creek's lip is left out."""
    outline = area.outline
    s = arc_length(outline, closed=True)
    lip = waterfall(area)
    lip_xy = np.array(lip['location'][:2]) / 100.0 if lip else None
    drop = area.island['rimDrop'] / 100.0
    points = []
    for target in np.arange(0.0, s[-1], CLIFF_STEP):
        k = min(int(np.searchsorted(s, target)), len(outline) - 1)
        a = outline[k]
        if lip_xy is not None and np.linalg.norm(a - lip_xy) < 5.0:
            continue
        tangent = outline[(k + 1) % len(outline)] - outline[k - 1]
        out = np.array([tangent[1], -tangent[0]]) / max(np.linalg.norm(tangent), 1e-9)
        if area.at(area.edge, *(a + out)) > area.at(area.edge, *(a - out)):
            out = -out
        z = float(area.height(*a))
        points.append({'location': [_cm(a[0]), _cm(a[1]), _cm(z)], 'drop': _cm(drop), 'yaw': _yaw(*out),
                       'plateau': bool(z > 5.0)})
    return points


def cliff_groups(area):
    """Every cliff group in order: each plateau's (and mesa's) cliff and then its ramp's walls, each pit's wall and its
    ramp's, the ridges' and scarps' faces, the knobs' rocks, the gullies' banks, the gorges' walls, then the rim
    (island setting) or the regional ridges' bands, the rock wall past the boundary's closed edges ("boundaryFoot",
    with boundary.foot, which takes over the bands' points there) and the escarpment's lip (grounded). A group is named
    by its feature's cliffGroup (the feature's id, or the ramp's own id, when not given)."""
    groups = {}

    def add(name, points):
        if name in groups:
            raise ValueError(f'{area.path}: two features share the cliff group {name!r}; give one a cliffGroup')
        groups[name] = points
    for p in area.plateaus:
        add(p['cliff_group'], plateau_cliffs(area, p))
        for r in area.ramps:
            if r['plateau'] == p['id']:
                add(r['cliff_group'], ramp_walls(area, r))
    for p in getattr(area, 'pits', []):
        add(p['cliff_group'], area_cliffs.pit_cliffs(area, p, _near_ramp))
        for r in area.ramps:
            if r['plateau'] == p['id']:
                add(r['cliff_group'], ramp_walls(area, r))
    for sp in getattr(area, 'spines', []):
        add(sp['cliff_group'], area_cliffs.spine_cliffs(area, sp))
    for sc in getattr(area, 'scarps', []):
        add(sc['cliff_group'], area_cliffs.scarp_cliffs(area, sc))
    for kn in getattr(area, 'knobs', []):
        add(kn['cliff_group'], area_cliffs.knob_points(area, kn))
    for g in getattr(area, 'gullies', []):
        add(g['cliff_group'], area_cliffs.gully_walls(area, g))
    for g in getattr(area, 'gorges', []):
        add(g['cliff_group'], area_cliffs.gorge_walls(area, g))
    if area.setting == 'grounded':
        for r in area.region.ridges:
            # Where boundary.foot's rock stands, its own dressing covers the band from the wall's foot.
            add(r['id'], area_boundary.clear_of_rock(area, area_cliffs.ridge_bands(area, r)))
        if area_boundary.foot_spec(area) is not None:
            add('boundaryFoot', area_boundary.rock_faces(area))
        if area.region.lip is not None:
            loop, kinds = area.mesh_boundary()
            add('escarpment', area_cliffs.escarpment_points(area, loop[kinds != 1]))
    else:
        add('rim', rim_points(area))
    return groups


def orchard_rows(area):
    """Apple tree rows across each orchard zone: east-west rows ORCHARD_SPACING apart, a tree every ORCHARD_SPACING,
    kept clear of the zone's edge, roads and paths (a lane through the rows) and the island's rim."""
    rows = []
    for zone in area.zones_of('orchard'):
        rows += _orchard_rows(area, to_m(zone['polygon']))
    return rows


def _orchard_rows(area, poly):
    lanes = [r['pts'] for r in area.roads]
    lo, hi = poly.min(axis=0), poly.max(axis=0)
    best = []
    # The grid's offset that fits the most trees.
    for ox in np.arange(0.0, ORCHARD_SPACING, 1.25):
        for oy in np.arange(0.0, ORCHARD_SPACING, 1.25):
            rows = []
            for x in np.arange(hi[0] - 1.5 - ox, lo[0], -ORCHARD_SPACING):
                y = np.arange(hi[1] - 1.5 - oy, lo[1], -ORCHARD_SPACING)
                cand = np.column_stack([np.full(len(y), x), y])
                keep = points_in_polygon(cand[:, 0], cand[:, 1], poly)
                for dx, dy in ((1.5, 0.0), (-1.5, 0.0), (0.0, 1.5), (0.0, -1.5)):
                    keep &= points_in_polygon(cand[:, 0] + dx, cand[:, 1] + dy, poly)
                for lane in lanes:
                    keep &= np.min(np.linalg.norm(cand[:, None, :] - lane[None, :, :], axis=2), axis=1) > 3.2
                keep &= area.at(area.play_edge, cand[:, 0], cand[:, 1]) > 3.5  # clear of the rim (or boundary)
                if keep.sum() >= 2:
                    rows.append(cand[keep])
            if sum(len(r) for r in rows) > sum(len(r) for r in best):
                best = rows
    return [{'start': _xyz(area, *trees[0]), 'end': _xyz(area, *trees[-1]),
             'trees': [_xyz(area, *t) for t in trees]} for trees in best]


def compute(area):
    half, side = _number(area.half_cm), _number(2 * area.half_cm)
    covers = [[-area.half * 100.0, -area.half * 100.0], [area.half * 100.0, area.half * 100.0]]
    data = {
        'about': f'Computed by Art/Models/Terrain/{area.name}.py from layout.json (run it with --computed). Unreal '
                 'world centimeters (X north, Y east, Z up); yaw in degrees (0 = +X, 90 = +Y). Terrain heights are '
                 f'the built terrain\'s, with the {"island" if area.setting == "island" else "area"} placed at the '
                 'origin.',
        'macroMap': {
            'texture': area.macro_texture,
            'covers': covers,
            'uv0': f'U = (Y + {half}) / {side}, V = (X + {half}) / {side} (V up from the bottom of the image, as in '
                   'Blender; north is up in the image)',
            'alpha': 'detail selector: about 0 grass and soil, 1 rock; dirt roads about 0.4, footpaths 0.25, '
                     'scree 0.65 (never exactly 0: the lowest value is 1/255)',
            'scatterMap': {
                'texture': area.scatter_texture,
                'size': area.sizes['scatter'], 'covers': covers,
                'uv0': 'same square and mapping as the macro map', 'values': 'linear (no sRGB), 0..255',
                'R': 'tree density: high in the forest grove, a few lone trees in open meadow; 0 on roads and paths '
                     '(+1.5 m), water, rock, building footprints, the village square, the farmyard, the target '
                     'meadow and the orchard (its trees go on orchardRows)',
                'G': 'grass density: 1 in meadows; 0 on road and path surfaces (soft 1 m edge), water, steep rock, '
                     'footprints and bare dirt; about 0.4 under the forest canopy',
                'B': 'flower density: big drifts in the meadows, 0 where G is 0, sparse in the forest',
                'A': 'pebbles and small rocks: a thin band just outside road edges, cliff feet (scree), the rim, '
                     'the creek banks; 0 elsewhere',
            },
        },
        'placements': {},
        'footprints': [{'id': f['id'], 'center': [_cm(f['center'][0]), _cm(f['center'][1]), _cm(f['z'])],
                        'flatRadius': _cm(f['radius']), 'blend': _cm(f['blend'])} for f in area.footprints],
    }
    for p in area.layout['placements']:
        # [X, Y] stands on the terrain; [X, Y, Z] keeps its own height (a model seated on something the heights
        # don't show: Den Rock's pivot on the Sink's rim, over the pocket its den goes into).
        x, y = np.asarray(p['location'][:2]) / 100.0
        z = p['location'][2] / 100.0 if len(p['location']) > 2 else None
        data['placements'][p['id']] = {'kind': p['kind'], 'location': _xyz(area, x, y, z), 'yaw': p.get('yaw', 0.0)}
    data['bridge'] = bridge(area)
    data['waterfall'] = waterfall(area)
    if area.ponds:
        pond = area.ponds[0]
        cx, cy = pond['center']
        data['pond'] = {'center': [_cm(cx), _cm(cy)], 'radii': [_cm(r) for r in pond['radii']],
                        'waterZ': _cm(pond['level']), 'bottomZ': _cm(pond['bottom'])}
    if area.creeks:
        c = area.creeks[0]
        keep = c['s'] <= c['s_lip']
        pts = c['pts'][keep][::4]
        data['creek'] = {'points': [[_cm(x), _cm(y), _cm(z)] for (x, y), z in
                                    zip(pts, np.interp(c['s'][keep][::4], c['s'], c['water']))],
                         'waterWidth': _cm(2.0 * CREEK_WATER_HALF)}
    data['cliffs'] = cliff_groups(area)
    data['roads'] = {}
    for r in area.roads:
        idx = np.arange(0, len(r['pts']), 4)
        data['roads'][r['id']] = {'kind': r['kind'], 'width': _cm(r['width']),
                                  'points': [[_cm(r['pts'][i][0]), _cm(r['pts'][i][1]), _cm(r['z'][i])] for i in idx]}
    if area.ramps:
        r = area.ramps[0]
        idx = np.arange(0, len(r['pts']), 4)
        data['ramp'] = {'width': _cm(r['width']),
                        'points': [[_cm(r['pts'][i][0]), _cm(r['pts'][i][1]), _cm(r['z'][i])] for i in idx]}
    data['orchardRows'] = orchard_rows(area)
    # The top mesh's lowest and highest ground (the scatter's rays start above the one and reach below the other).
    on_top = area.h[area.inside]
    data['heightRange'] = [_cm(on_top.min()), _cm(on_top.max())]
    _all_of_each(area, data)
    if area.setting == 'grounded':
        _grounded(area, data)
    import area_open
    data['openGround'] = area_open.measure(area)[0]
    # The far trees on the ring (region.farTrees), on the meshes the terrain model has just built.
    import area_fartrees
    trees = area_fartrees.place(area)
    if trees is not None:
        data['farTrees'] = trees
    # Which layout it came from, leaving out the blocks the generator never reads (area_shape.layout_sha1).
    data['layoutSha1'] = layout_sha1(area.path)
    return data


def _all_of_each(area, data):
    """Every pond, creek, ramp, bridge, gully and waterfall by id (the singular keys keep the first of each)."""
    data['ponds'] = {}
    for p in area.ponds:
        cx, cy = p['center']
        data['ponds'][p['id']] = {'center': [_cm(cx), _cm(cy)], 'radii': [_cm(r) for r in p['radii']],
                                  'waterZ': _cm(p['level']), 'bottomZ': _cm(p['bottom'])}
    data['creeks'] = {}
    for c in area.creeks:
        keep = c['s'] <= c['s_lip']
        pts = c['pts'][keep][::4]
        data['creeks'][c['id']] = {
            'points': [[_cm(x), _cm(y), _cm(z)] for (x, y), z in
                       zip(pts, np.interp(c['s'][keep][::4], c['s'], c['water']))],
            'waterWidth': _cm(2.0 * CREEK_WATER_HALF)}
    data['ramps'] = {}
    for r in area.ramps:
        idx = np.arange(0, len(r['pts']), 4)
        data['ramps'][r['id']] = {'of': r['plateau'], 'width': _cm(r['width']),
                                  'points': [[_cm(r['pts'][i][0]), _cm(r['pts'][i][1]), _cm(r['z'][i])] for i in idx]}
    data['bridges'] = {}
    for spec in area.layout.get('bridges', []):
        found = bridge_of(area, spec)
        if found:
            data['bridges'][spec['id']] = found
    data['gullies'] = {}
    for g in getattr(area, 'gullies', []):
        idx = np.arange(0, len(g['pts']), 4)
        data['gullies'][g['id']] = {'width': _cm(2.0 * g['half']), 'depth': _cm(g['depth']), 'walls': g['walls'],
                                    'points': [[_cm(g['pts'][i][0]), _cm(g['pts'][i][1]), _cm(g['bed'][i])]
                                               for i in idx],
                                    'note': 'points run along the bed (z is the bed), down to its end'}
    if area.setting == 'grounded':
        data['waterfalls'] = waterfalls(area)
    else:
        creek = next((c for c in area.creeks if c['feature'].get('waterfallAtEnd')), None)
        data['waterfalls'] = {creek['id']: data['waterfall']} if creek and data.get('waterfall') else {}


def _grounded(area, data):
    """A grounded area's extra entries: the ring's macro map, the seam band and the playable boundary."""
    region = area.region
    # The scatter mask's creases' pines and the scrub (area_scatter.py, area_scrub.py, with regional ridges);
    # Tools/Unreal/build_island_scatter.py adds their graph layers when these are there.
    if region.ridges:
        data['macroMap']['scatterMap']['steep'] = (
            'R over 37 degrees is pines in the ridges\' faces\' creases (area_faces.py), which past the boundary also '
            'carries the creases\' trees on gentler ground')
        import area_scrub
        data['scrub'] = dict(
            {'texture': area.scrub_texture, 'covers': data['macroMap']['covers'],
             'note': 'the dry scrub (area_scrub.py): the mask\'s layers (a channel each, candidates every cell cm '
                     'jittered by jitter of a cell, kept where mask x random >= keep; the graph names the meshes), '
                     'and points in cm: crestJunipers [x, y, yaw], pitTufts and pitSage [x, y], '
                     'rimJunipers [x, y, yaw]',
             'layers': area_scrub.LAYERS},
            **area_scrub.points(area))
    ring = [[-region.half * 100.0, -region.half * 100.0], [region.half * 100.0, region.half * 100.0]]
    half, side = _number(region.half * 100.0), _number(region.half * 200.0)
    data['ringMap'] = {'texture': area.ring_macro_texture, 'covers': ring,
                       'uv0': f'U = (Y + {half}) / {side}, V = (X + {half}) / {side}, like the macro map\'s',
                       'note': "the surround ring's macro color map (its meshes' UV 0); the core's blends into it "
                               'across the seam band'}
    data['region'] = {'seamBand': _cm(region.seam), 'core': data['macroMap']['covers'], 'ring': ring,
                      'note': "the core is the map square; its outer seamBand fades into the regional field, so the "
                              'core meets the surround ring there without a crack'}
    if area.boundary is not None:
        flags = area.open_edges
        corners = [[_cm(x), _cm(y), _cm(_ground_near(area, x, y, flags[i] or flags[i - 1]))]
                   for i, (x, y) in enumerate(area.boundary)]
        data['boundary'] = {
            'corners': corners, 'openEdges': [bool(f) for f in area.open_edges],
            'note': "APlayableArea's corners (world cm; Z is the ground at the corner, or the nearest ground on the "
                    "core within 2 m for a corner on an open edge, which stands out over a drop) and open_edges (edge i runs from corner i to corner "
                    "i + 1, the last back to corner 0; true where a drop is part of play)"}
        rise = area_boundary.summary(area)
        if rise is not None:
            data['boundary']['rise'] = rise


def _ground_near(area, x, y, open_edge):
    """The ground's height at a boundary corner. A corner on an open edge stands just past a drop (the lip, a gorge's
    rim), so it takes the highest core ground within 2 m: the edge it looks over."""
    if not open_edge and area.at(area.edge, x, y) > 0.3:
        return float(area.height(x, y))
    gx, gy = np.meshgrid(np.linspace(x - 2.0, x + 2.0, 9), np.linspace(y - 2.0, y + 2.0, 9), indexing='ij')
    on_core = area.at(area.edge, gx, gy) > 0.2
    if on_core.any():
        return float(np.max(area.height(gx[on_core], gy[on_core])))
    pts = area.outline
    k = int(np.argmin(np.hypot(pts[:, 0] - x, pts[:, 1] - y)))
    inward = -pts[k] / max(float(np.linalg.norm(pts[k])), 1e-9)
    return float(area.height(*(pts[k] + inward * 0.5)))


def _dumps(data):
    """The file's text: indented as ever, but the far trees one per line ([x, y, z, yaw, scale]), which keeps a few
    thousand of them compact; the layout's hash stays last."""
    rest = {k: v for k, v in data.items() if k not in ('farTrees', 'layoutSha1')}
    text = json.dumps(rest, indent=1)
    if 'farTrees' in data:
        trees = data['farTrees']
        rows = []
        for name, items in trees['meshes'].items():
            body = ',\n'.join('    ' + json.dumps(t, separators=(',', ':')) for t in items)
            rows.append(f'   {json.dumps(name)}: [\n{body}\n   ]' if items else f'   {json.dumps(name)}: []')
        text = (text[:-2] + ',\n "farTrees": {\n  "note": ' + json.dumps(trees['note']) + ',\n  "meshes": {\n'
                + ',\n'.join(rows) + '\n  }\n }\n}')
    if 'layoutSha1' in data:
        text = text[:-2] + ',\n "layoutSha1": ' + json.dumps(data['layoutSha1']) + '\n}'
    return text


def write(area, path=None, log=print):
    path = path or area.computed_path
    data = compute(area)
    with open(path, 'w', encoding='utf-8') as file:
        if 'farTrees' in data:
            file.write(_dumps(data))
        else:
            json.dump(data, file, indent=1)
    counts = ', '.join(f'{len(points)} {group}' for group, points in data['cliffs'].items())
    trees = data.get('farTrees')
    far = ('; far trees: ' + ', '.join(f'{len(v)} {k[3:]}' for k, v in trees['meshes'].items())) if trees else ''
    log(f'terrain: wrote {path} (cliff points: {counts}){far}')
    return data
