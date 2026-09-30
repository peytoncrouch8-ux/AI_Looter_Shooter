"""Values the placement script needs, measured on the built island: Art/Levels/TutorialIsland/layout_computed.json.
Everything in it is Unreal world centimeters (X north, Y east, Z up) and yaw in degrees (0 = +X, 90 = +Y), like
layout.json. compute() returns the dictionary, write() saves it.
"""
import json
import math
import os

import numpy as np

from island_math import MAP_HALF, arc_length, points_in_polygon, sample
from island_shape import CREEK_WATER_HALF, HERE, to_m

COMPUTED_PATH = os.path.join(HERE, 'layout_computed.json')
CLIFF_STEP = 10.0     # meters between cliff dressing points
ORCHARD_SPACING = 4.5


def _cm(v):
    return round(float(v) * 100.0, 1)


def _xyz(island, x, y, z=None):
    return [_cm(x), _cm(y), _cm(island.height(x, y) if z is None else z)]


def _yaw(dx, dy):
    return round(math.degrees(math.atan2(dy, dx)), 1)


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


def _road(island, road_id):
    return next((r for r in island.roads if r['id'] == road_id), None)


def bridge(island):
    road = _road(island, 'forest')
    creek = island.creek
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
    # The span: out along the road until the ground is back up to the deck on both sides, plus a meter each way.
    reach = []
    for sign in (-1.0, 1.0):
        d = 0.5
        while d < 15.0 and island.height(*(point + sign * direction * d)) < deck - 0.12:
            d += 0.1
        reach.append(d)
    water = float(np.interp(_along(creek, point), creek['s'], creek['water']))
    center = point + direction * (reach[1] - reach[0]) * 0.5
    return {
        'id': 'creek_bridge', 'road': 'forest', 'creek': 'creek',
        'location': [_cm(center[0]), _cm(center[1]), _cm(deck)],
        'yaw': _yaw(*direction), 'span': _cm(sum(reach) + 2.0), 'width': _cm(road['width']),
        'waterZ': _cm(water), 'bedZ': _cm(island.height(*point)),
        'note': 'location is the middle of the span at deck height (the road surface on both banks); '
                'yaw runs along the road; span is bank to bank plus a meter each side',
    }


def _along(curve, point):
    d = np.linalg.norm(curve['pts'] - point, axis=1)
    return float(curve['s'][int(np.argmin(d))])


def waterfall(island):
    creek = island.creek
    if creek is None:
        return None
    k = creek['k_lip']
    seg = creek['pts'][max(k - 1, 0):k + 1]
    found = _intersect(seg, np.vstack([island.outline, island.outline[:1]]))
    point = found[0] if found else creek['pts'][k]
    s = _along(creek, point)
    direction = creek['pts'][min(k + 1, len(creek['pts']) - 1)] - creek['pts'][max(k - 3, 0)]
    water = float(np.interp(s, creek['s'], creek['water']))
    return {
        'location': [_cm(point[0]), _cm(point[1]), _cm(water)], 'yaw': _yaw(*direction),
        'width': _cm(2.0 * CREEK_WATER_HALF), 'bedZ': _cm(island.height(*point)),
        'dropTo': _cm(water - island.layout['island']['undersideDepth'] / 100.0),
        'note': 'the lip: where the creek runs off the rim, at the water surface; yaw is the flow direction',
    }


def plateau_cliffs(island):
    """Points along the plateau's cliff (every CLIFF_STEP m): where the cliff face meets the ground below, its top,
    and the outward direction. None on the rim (that's the underside's) or across the ramp."""
    p = island.plateau
    if p is None:
        return []
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
        if sample(p['sd'], *(a + n)) < sample(p['sd'], *(a - n)):
            n = -n  # point outward (the signed distance grows outward)
        offsets = np.arange(-6.0, 6.0, 0.1)
        line = a[None, :] + offsets[:, None] * n[None, :]
        sd = sample(p['sd'], line[:, 0], line[:, 1])
        cross = np.nonzero(np.diff(np.sign(sd)) != 0)[0]
        if not len(cross):
            continue
        c = line[cross[np.argmin(np.abs(offsets[cross]))]]
        if sample(island.edge, *c) < 4.0 or _near_ramp(island, c, 3.0):
            continue
        gx = sample(normal_field[0], *c)
        gy = sample(normal_field[1], *c)
        out = np.array([gx, gy]) / max(math.hypot(gx, gy), 1e-9)
        base = c + out * (p['width'] * 0.5 + 1.2)
        top = c - out * (p['width'] * 0.5 + 1.0)
        zb, zt = float(island.height(*base)), float(island.height(*top))
        points.append({'location': [_cm(c[0]), _cm(c[1]), _cm(zb)], 'top': _cm(zt), 'height': _cm(zt - zb),
                       'yaw': _yaw(*out)})
    return points


def _near_ramp(island, point, margin):
    if island.ramp is None:
        return False
    d = np.min(np.linalg.norm(island.ramp['pts'] - point, axis=1))
    return d < island.ramp['width'] * 0.5 + margin


def ramp_walls(island, step=5.0):
    """Both walls of the ramp's cut through the cliff, where they're more than 1.5 m high: the wall's foot, its
    height and the direction it faces (toward the path)."""
    r = island.ramp
    if r is None:
        return []
    walls = []
    pts, s = r['pts'], r['s']
    half = r['width'] * 0.5
    for target in np.arange(step * 0.5, s[-1], step):
        k = int(np.searchsorted(s, target))
        tangent = pts[min(k + 1, len(pts) - 1)] - pts[max(k - 1, 0)]
        tangent /= np.linalg.norm(tangent)
        side = np.array([-tangent[1], tangent[0]])
        zr = float(np.interp(target, s, r['z']))
        for sign in (-1.0, 1.0):
            d = np.arange(half, half + 9.0, 0.2)
            line = pts[k][None, :] + sign * d[:, None] * side[None, :]
            rise = island.height(line[:, 0], line[:, 1]) - zr
            if rise.max() < 1.5:
                continue
            foot = line[int(np.argmax(rise > 0.25))]
            walls.append({'location': [_cm(foot[0]), _cm(foot[1]), _cm(zr)], 'height': _cm(rise.max()),
                          'yaw': _yaw(*(-sign * side))})
    return walls


def rim_points(island):
    """The island's edge every CLIFF_STEP m: the top of the rock wall under it, how far it drops before the
    underside tapers in, and the outward direction. The creek's lip is left out."""
    outline = island.outline
    s = arc_length(outline, closed=True)
    lip = waterfall(island)
    lip_xy = np.array(lip['location'][:2]) / 100.0 if lip else None
    drop = island.layout['island']['rimDrop'] / 100.0
    points = []
    for target in np.arange(0.0, s[-1], CLIFF_STEP):
        k = min(int(np.searchsorted(s, target)), len(outline) - 1)
        a = outline[k]
        if lip_xy is not None and np.linalg.norm(a - lip_xy) < 5.0:
            continue
        tangent = outline[(k + 1) % len(outline)] - outline[k - 1]
        out = np.array([tangent[1], -tangent[0]]) / max(np.linalg.norm(tangent), 1e-9)
        if sample(island.edge, *(a + out)) > sample(island.edge, *(a - out)):
            out = -out
        z = float(island.height(*a))
        points.append({'location': [_cm(a[0]), _cm(a[1]), _cm(z)], 'drop': _cm(drop), 'yaw': _yaw(*out),
                       'plateau': bool(z > 5.0)})
    return points


def orchard_rows(island):
    """Apple tree rows across the orchard zone: east-west rows ORCHARD_SPACING apart, a tree every ORCHARD_SPACING,
    kept clear of the zone's edge, the orchard path (a lane through the rows) and the island's rim."""
    zone = next((z for z in island.layout['zones'] if z['id'] == 'orchard'), None)
    if zone is None:
        return []
    poly = to_m(zone['polygon'])
    lanes = [r['pts'] for r in island.roads]
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
                keep &= sample(island.edge, cand[:, 0], cand[:, 1]) > 3.5
                if keep.sum() >= 2:
                    rows.append(cand[keep])
            if sum(len(r) for r in rows) > sum(len(r) for r in best):
                best = rows
    return [{'start': _xyz(island, *trees[0]), 'end': _xyz(island, *trees[-1]),
             'trees': [_xyz(island, *t) for t in trees]} for trees in best]


def compute(island):
    data = {
        'about': 'Computed by Art/Models/Terrain/TutorialIsland.py from layout.json (run it with --computed). Unreal '
                 'world centimeters (X north, Y east, Z up); yaw in degrees (0 = +X, 90 = +Y). Terrain heights are '
                 'the built terrain\'s, with the island placed at the origin.',
        'macroMap': {
            'texture': 'Art/Textures/TutorialIslandMacro/T_TutorialIslandMacro_BC.png',
            'covers': [[-MAP_HALF * 100.0, -MAP_HALF * 100.0], [MAP_HALF * 100.0, MAP_HALF * 100.0]],
            'uv0': 'U = (Y + 10240) / 20480, V = (X + 10240) / 20480 (V up from the bottom of the image, as in '
                   'Blender; north is up in the image)',
            'alpha': 'detail selector: about 0 grass and soil, 1 rock; dirt roads about 0.4, footpaths 0.25, '
                     'scree 0.65 (never exactly 0: the lowest value is 1/255)',
            'scatterMap': {
                'texture': 'Art/Textures/TutorialIslandMacro/T_TutorialIslandScatter_BC.png',
                'size': 512, 'uv0': 'same square and mapping as the macro map', 'values': 'linear (no sRGB), 0..255',
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
                        'flatRadius': _cm(f['radius']), 'blend': _cm(f['blend'])} for f in island.footprints],
    }
    for p in island.layout['placements']:
        x, y = np.asarray(p['location']) / 100.0
        data['placements'][p['id']] = {'kind': p['kind'], 'location': _xyz(island, x, y), 'yaw': p.get('yaw', 0.0)}
    data['bridge'] = bridge(island)
    data['waterfall'] = waterfall(island)
    if island.pond:
        cx, cy = island.pond['center']
        data['pond'] = {'center': [_cm(cx), _cm(cy)], 'radii': [_cm(r) for r in island.pond['radii']],
                        'waterZ': _cm(island.pond['level']), 'bottomZ': _cm(island.pond['bottom'])}
    if island.creek:
        c = island.creek
        keep = c['s'] <= c['s_lip']
        pts = c['pts'][keep][::4]
        data['creek'] = {'points': [[_cm(x), _cm(y), _cm(z)] for (x, y), z in
                                    zip(pts, np.interp(c['s'][keep][::4], c['s'], c['water']))],
                         'waterWidth': _cm(2.0 * CREEK_WATER_HALF)}
    data['cliffs'] = {'plateau': plateau_cliffs(island), 'rampWalls': ramp_walls(island), 'rim': rim_points(island)}
    data['roads'] = {}
    for r in island.roads:
        idx = np.arange(0, len(r['pts']), 4)
        data['roads'][r['id']] = {'kind': r['kind'], 'width': _cm(r['width']),
                                  'points': [[_cm(r['pts'][i][0]), _cm(r['pts'][i][1]), _cm(r['z'][i])] for i in idx]}
    if island.ramp:
        r = island.ramp
        idx = np.arange(0, len(r['pts']), 4)
        data['ramp'] = {'width': _cm(r['width']),
                        'points': [[_cm(r['pts'][i][0]), _cm(r['pts'][i][1]), _cm(r['z'][i])] for i in idx]}
    data['orchardRows'] = orchard_rows(island)
    return data


def write(island, path=COMPUTED_PATH, log=print):
    data = compute(island)
    with open(path, 'w', encoding='utf-8') as file:
        json.dump(data, file, indent=1)
    log(f"terrain: wrote {path} ({len(data['cliffs']['plateau'])} plateau cliff points, "
        f"{len(data['cliffs']['rim'])} rim points)")
    return data
