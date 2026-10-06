"""Cliff dressing for the feature types that came with step 3b and the grounded setting, and stacked courses (numpy
only): area_computed.py writes the groups into layout_computed.json's "cliffs", one per feature, and
Tools/Unreal/build_area.py dresses every group. Units are Unreal centimeters, as in the rest of that file.

A point is where a wall meets the ground below it ("location", its "height", "top" and the direction it faces, "yaw"),
as the plateaus' points are; or, for a hanging cliff (the island's rim, the escarpment's lip), the wall's top with the
"drop" below it. The cliff kit's tallest piece is 12 m (CliffFace_D), so a wall taller than MAX_COURSE is dressed in
stacked courses: its point carries "courses", bottom first, each a piece's foot ("location": where the wall is at that
course's bottom) and "height", none over MAX_COURSE, together covering the wall (a hanging cliff's courses cover its
drop). Points that aren't faces carry a "kind": "outcrop" (a knob's freestanding rock: the outcrop kit's "piece", its
"height" and "yaw", standing on the knob's level seat) or "bank" (a gully's sloped bank, too gentle for a face; boulders
can go there).
"""
import math

import numpy as np

from area_features import GORGE_WALL, right_normal
from area_math import arc_length

MAX_COURSE = 12.0   # meters: the cliff kit's tallest piece (CliffFace_D)
CLIFF_STEP = 10.0   # meters between dressing points along a cliff
WALL_STEP = 5.0     # meters between dressing points along a channel's walls
MIN_FACE = 1.5      # meters: lower walls get no face


def _cm(v):
    return round(float(v) * 100.0, 1)


def _yaw(dx, dy):
    return round(math.degrees(math.atan2(dy, dx)), 1)


def courses(area, foot, top, z_foot, z_top):
    """A standing wall from foot (its bottom, z_foot) up to top (z_top) as stacked courses of equal height, none over
    MAX_COURSE, bottom first: each course's foot is where the ground on the line from foot to top reaches its bottom.
    None when one piece covers it."""
    height = z_top - z_foot
    count = int(math.ceil(height / MAX_COURSE - 1e-9))
    if count <= 1:
        return None
    step = height / count
    line = foot[None, :] + (top - foot)[None, :] * np.linspace(0.0, 1.0, 121)[:, None]
    z = area.height(line[:, 0], line[:, 1])
    out = []
    for k in range(count):
        bottom = z_foot + k * step
        p = line[int(np.argmax(z >= bottom - 1e-6))]
        out.append({'location': [_cm(p[0]), _cm(p[1]), _cm(bottom)], 'height': _cm(step)})
    return out


def hanging_courses(top_xy, out, z_top, drop, batter):
    """A hanging wall's courses, bottom first: drop meters below z_top, each course's foot set out by the wall's
    batter (meters out per meter down). None when one piece covers it."""
    count = int(math.ceil(drop / MAX_COURSE - 1e-9))
    if count <= 1:
        return None
    step = drop / count
    result = []
    for k in range(count):
        bottom = z_top - drop + k * step
        p = top_xy + out * batter * (z_top - bottom)
        result.append({'location': [_cm(p[0]), _cm(p[1]), _cm(bottom)], 'height': _cm(step)})
    return result


def face(area, c, out, foot_out, top_in, extra=None):
    """A standing face whose middle is at c, facing out: its foot foot_out meters out, its top top_in meters in.
    None when it's lower than MIN_FACE."""
    foot, top = c + out * foot_out, c - out * top_in
    zb, zt = float(area.height(*foot)), float(area.height(*top))
    if zt - zb < MIN_FACE:
        return None
    point = {'location': [_cm(c[0]), _cm(c[1]), _cm(zb)], 'top': _cm(zt), 'height': _cm(zt - zb), 'yaw': _yaw(*out)}
    stacked = courses(area, foot, top, zb, zt)
    if stacked:
        point['courses'] = stacked
    if extra:
        point.update(extra)
    return point


def _inside(area, p, margin=4.0):
    """Whether a point is on the top mesh, margin meters in from its edge (the rim or the lip)."""
    return area.at(area.edge, *p) > margin


def _along(pts, s, step, start=0.0):
    """Indices of a curve's points every step meters from start."""
    return [min(int(np.searchsorted(s, t)), len(pts) - 1) for t in np.arange(start, s[-1] + 1e-9, step)]


def _tangent(pts, k):
    t = pts[min(k + 1, len(pts) - 1)] - pts[max(k - 1, 0)]
    return t / max(np.linalg.norm(t), 1e-9)


def spine_cliffs(area, sp):
    """A ridge feature's cliff faces, on each side that has one: the left side along the crest, then the right side
    back, so neighbors in the list stand side by side."""
    points = []
    for which, sign in (('left', -1.0), ('right', 1.0)):
        if sp['cliffs'] not in (which, 'both'):
            continue
        side = []
        for k in _along(sp['pts'], sp['s'], CLIFF_STEP, CLIFF_STEP * 0.5):
            t = _tangent(sp['pts'], k)
            out = sign * np.array([-t[1], t[0]])
            c = sp['pts'][k] + out * (sp['top'] + sp['cliff_width'] * 0.5)
            if not _inside(area, c):
                continue
            point = face(area, c, out, sp['cliff_width'] * 0.5 + 1.2, sp['cliff_width'] * 0.5 + 1.0)
            if point:
                side.append(point)
        points += side if which == 'left' else side[::-1]
    return points


def scarp_cliffs(area, sc):
    """A scarp's face, facing its low side."""
    points = []
    for k in _along(sc['pts'], sc['s'], CLIFF_STEP, CLIFF_STEP * 0.5):
        t = _tangent(sc['pts'], k)
        out = -sc['high'] * np.array([-t[1], t[0]])  # toward the low side
        c = sc['pts'][k]
        if not _inside(area, c):
            continue
        point = face(area, c, out, sc['cliff_width'] * 0.5 + 1.2, sc['cliff_width'] * 0.5 + 1.0)
        if point:
            points.append(point)
    return points


def pit_cliffs(area, p, near_ramp):
    """A pit's wall, every CLIFF_STEP m round its rim, facing in: the wall's foot is on the floor, its top on the rim.
    Left out across its ramp, and within each of its "cliffGaps" ({center, radius} in cm: where a rock model stands
    in for the wall, as Den Rock does over the Gravemother's den)."""
    poly = p['poly']
    s = arc_length(poly, closed=True)
    gradient = np.gradient(p['sd'])
    gaps = [(np.asarray(g['center'], dtype=np.float64) / 100.0, g['radius'] / 100.0)
            for g in p['feature'].get('cliffGaps', [])]
    points = []
    for target in np.arange(0.0, s[-1], CLIFF_STEP):
        k = min(int(np.searchsorted(s, target)), len(poly) - 1)
        a = poly[k]
        tangent = poly[(k + 1) % len(poly)] - poly[k - 1]
        n = np.array([tangent[1], -tangent[0]]) / max(np.linalg.norm(tangent), 1e-9)
        offsets = np.arange(-6.0, 6.0, 0.1)
        line = a[None, :] + offsets[:, None] * n[None, :]
        sd = area.at(p['sd'], line[:, 0], line[:, 1])
        cross = np.nonzero(np.diff(np.sign(sd)) != 0)[0]
        if not len(cross):
            continue
        c = line[cross[np.argmin(np.abs(offsets[cross]))]]
        if not _inside(area, c) or near_ramp(area, c, 3.0):
            continue
        if any(np.linalg.norm(c - center) < radius for center, radius in gaps):
            continue
        g = np.array([area.at(gradient[0], *c), area.at(gradient[1], *c)])
        outward = g / max(np.linalg.norm(g), 1e-9)  # the signed distance grows outward, away from the floor
        point = face(area, c, -outward, p['width'] * 0.5 + 1.2, p['width'] * 0.5 + 1.0)
        if point:
            points.append(point)
    return points


def knob_points(area, kn):
    """A knob's freestanding rock: the outcrop kit's piece on the knob's level seat."""
    rock = kn['rock']
    cx, cy = kn['center']
    return [{'kind': 'outcrop', 'location': [_cm(cx), _cm(cy), _cm(kn['z'])], 'piece': rock.get('piece', 'TorA'),
             'height': _cm(rock.get('height', 500) / 100.0), 'yaw': float(rock.get('yaw', 0.0)),
             'radius': _cm(kn['radius'])}]


def channel_walls(area, pts, s, floor, half, cliff, start=0.0, reach=8.0):
    """Both walls of a channel (a gully or a gorge) every WALL_STEP m from start along it, where they're more than
    MIN_FACE high: each wall's foot on the floor, its top where the ground stops rising (looking at most reach meters
    past the floor, so a ridge's slope beyond isn't taken for the wall), facing the channel. Cliff walls are faces
    (stacked when tall); sloped ones are banks. Each side runs along the channel; the far side comes back, so
    neighbors in the list stand side by side."""
    sides = ([], [])
    for k in _along(pts, s, WALL_STEP, start):
        t = _tangent(pts, k)
        across = np.array([-t[1], t[0]])
        z_floor = float(np.interp(s[k], s, floor))
        for n, sign in enumerate((-1.0, 1.0)):
            d = np.arange(max(half - 1.0, 0.0), half + reach, 0.2)
            line = pts[k][None, :] + sign * d[:, None] * across[None, :]
            z = area.height(line[:, 0], line[:, 1])
            rise = z - z_floor
            # The wall's top: where the ground stops climbing steeply (or the line's end).
            steep = np.gradient(z, 0.2) > (1.5 if cliff else 0.35)
            if not steep.any():
                continue
            first = int(np.argmax(steep))
            last = first + int(np.argmax(~steep[first:])) if (~steep[first:]).any() else len(d) - 1
            foot, top = line[first], line[last]
            height = float(rise[last] - max(rise[first], 0.0))
            if height < MIN_FACE or not _inside(area, foot, 1.0):
                continue
            facing = -sign * across
            if cliff:
                point = face(area, (foot + top) * 0.5, facing, np.linalg.norm(top - foot) * 0.5 + 0.3,
                             np.linalg.norm(top - foot) * 0.5 + 0.3)
            else:
                zb = float(area.height(*foot))
                point = {'kind': 'bank', 'location': [_cm(foot[0]), _cm(foot[1]), _cm(zb)],
                         'top': _cm(zb + height), 'height': _cm(height), 'yaw': _yaw(*facing)}
            if point:
                sides[n].append(point)
    return sides[0] + sides[1][::-1]


def gully_walls(area, g):
    cliff = g['walls'] == 'cliff'
    return channel_walls(area, g['pts'], g['s'], g['bed'], g['half'], cliff,
                         reach=8.0 if cliff else (g['depth'] + 3.0) / g['bank'] + 1.0)


def gorge_walls(area, g):
    """A gorge's walls, from just past its head (the falls' face, where the waterfall goes) to its end."""
    return channel_walls(area, g['pts'], g['s'], g['floor'], g['half'], True, start=g['head'] + 2.0)


def ridge_bands(area, r):
    """A regional ridge's cliff band along its foot, where the foot is inside the core and the band stands, facing
    the valley. The band's height is the one the regional field gives it (the slope above it is steep too, but bare),
    and its foot is where the field's wobble puts it."""
    region = area.region
    pts = r['foot']
    s = arc_length(pts)
    points = []
    for k in _along(pts, s, CLIFF_STEP):
        p = pts[k]
        if max(abs(p[0]), abs(p[1])) > area.half - 6.0 or not _inside(area, p, 2.0):
            continue
        t = _tangent(pts, k)
        into = r['side'] * np.array([-t[1], t[0]])  # toward the ridge
        foot = p - into * float(region.wobble(*p))
        band = region.band_height(r, *foot)
        if band < MIN_FACE:
            continue
        zb = float(area.height(*(foot - into * 0.6)))
        c = foot + into * r['band_width'] * 0.5
        point = {'location': [_cm(c[0]), _cm(c[1]), _cm(zb)], 'top': _cm(zb + band), 'height': _cm(band),
                 'yaw': _yaw(*-into)}
        stacked = courses(area, foot - into * 0.6, foot + into * (r['band_width'] + 0.6), zb, zb + band)
        if stacked:
            point['courses'] = stacked
        points.append(point)
    return points


def escarpment_points(area, lip_points):
    """The escarpment's lip every CLIFF_STEP m along the core's edge: hanging cliffs, the kit's courses covering the
    top of the drop (region.escarpment.kit), the generated canyon wall below them."""
    region = area.region
    pts = lip_points
    s = arc_length(pts)
    points = []
    for k in _along(pts, s, CLIFF_STEP, CLIFF_STEP * 0.5):
        a = pts[k]
        t = _tangent(pts, k)
        out = np.array([-t[1], t[0]])
        if region.height(*(a + out * 4.0)) > region.height(*(a - out * 4.0)):
            out = -out  # toward the canyon: the side the ground falls away to
        z = float(area.height(*a))
        drop = min(region.kit, z - float(region.height(*(a + out * (region.wall + 2.0)))))
        if drop < MIN_FACE:
            continue
        point = {'location': [_cm(a[0]), _cm(a[1]), _cm(z)], 'drop': _cm(drop), 'yaw': _yaw(*out), 'plateau': False}
        stacked = hanging_courses(a, out, z, drop, region.wall / region.drop)
        if stacked:
            point['courses'] = stacked
        points.append(point)
    return points


def plateau_courses(area, p, point):
    """Stacked courses for a plateau's (or mesa's) cliff point taller than MAX_COURSE (the island's are never)."""
    if point['height'] <= MAX_COURSE * 100.0:
        return point
    c = np.array(point['location'][:2]) / 100.0
    a = math.radians(point['yaw'])
    out = np.array([math.cos(a), math.sin(a)])
    stacked = courses(area, c + out * (p['width'] * 0.5 + 1.2), c - out * (p['width'] * 0.5 + 1.0),
                      point['location'][2] / 100.0, point['top'] / 100.0)
    if stacked:
        point['courses'] = stacked
    return point
