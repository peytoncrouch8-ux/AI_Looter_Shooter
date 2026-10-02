"""The feature types that came with Ransom's Rest (step 3b), as steps of Area.build() (area_shape.py runs them; numpy
only, area_math.py has the conventions). Each type is a list on the area, in layout order, and each feature names its
cliff group in layout_computed.json (its cliffGroup, else its id); area_cliffs.py writes the groups. Lengths in
layout.json are centimeters; "left" and "right" are as seen on the plan (north up), looking along a path.

  ridge  A raised line (a spine), its crest along "path" at "height" (or "heights", one per path point) over the ground:
         "cliffs" "left", "right", "both" or "none" says which sides drop as a cliff ("cliffWidth" wide, from a flat
         top "crestWidth" across); a side without one slopes down over "width". "taper" [start, end] eases the crest
         down to the ground over that length at each end.
  scarp  A one-sided step of 2-6 m ("height") along "path": "side" is the high side; its face is "cliffWidth" wide,
         and the high ground slopes back down over "back" behind it (a cuesta), so it rejoins the ground. Both ends
         ease out over "taper".
  pit    The plateau turned upside down (the Sink): the ground inside "polygon" sinks "depth" below the ground along its
         rim (or to "floor"), with an inward-facing cliff ring "cliffWidth" wide and a talus apron at its foot. Its
         "ramp" ({path, width, grade}) runs from the ground outside down to the floor; Area._ramps() cuts it.
  mesa   A plateau with a cliff all round and no ramp (Coffin Rock): Area._plateaus() builds it like a plateau.
  knob   A small footprint that seats a freestanding rock: the ground within "radius" of "center" is made level,
         "height" above the ground at the center, blending out over "blend". Its "rock" ({piece, height, yaw}) names the
         outcrop kit's piece that stands on it.
  gully  A dry creek bed (the Dry Wash): a floor "width" wide, "depth" below the ground along "path", always running
         down toward the path's end, with banks rising "bank" meters per meter (sloped), or near-vertical walls
         ("walls": "cliff").
  creek  ... with "falls": {"drop", "gorge": {path, width, fall}}: the creek ends in a falls "drop" high into a slot
         gorge with cliff walls, which starts at the creek's end and whose floor falls "fall" more along its path. A
         shallow stream runs on in the gorge.
Plateaus, pits and their ramps, and the creeks, are in area_shape.py.
"""
import math

import numpy as np

from area_math import (arc_length, catmull_rom, fbm, fbm_raster, gauss_smooth_1d, polyline_field, sample,
                       signed_distance, smin, smoothstep)

K = 4.2              # the cliff profile's steepness: steep in the middle, a rounded lip at the top, a foot below
GORGE_WALL = 5.0     # a cliff-walled channel's walls rise this many meters per meter (79 degrees)
GORGE_STREAM = 0.25  # the stream in a gorge stands this deep on its floor


def to_m(points):
    return np.asarray(points, dtype=np.float64) / 100.0


def rise(t):
    """0 at t = 0 (a cliff's foot) to 1 at t = 1 (its top): the plateaus' cliff profile."""
    return 0.5 + 0.5 * np.tanh(K * (np.clip(t, 0.0, 1.0) - 0.5)) / math.tanh(K * 0.5)


def right_normal(a, b):
    """The unit normal on the right of travel from a to b, as seen on the plan (polyline_field()'s side +1)."""
    d = np.asarray(b, dtype=np.float64) - np.asarray(a, dtype=np.float64)
    n = np.array([-d[1], d[0]])
    return n / max(np.linalg.norm(n), 1e-12)


def control_arcs(pts, s, ctrl):
    """The arc length along a curve (pts, s) at each of its control points."""
    return np.array([s[int(np.argmin(np.linalg.norm(pts - c, axis=1)))] for c in ctrl])


DROP_SIDES = {'left': [-1], 'right': [1], 'both': [-1, 1]}


def ramp_drop_sides(ramp):
    """The sides of a ramp whose embankment drops as a cliff (its "drop": left, right or both), as polyline_field()'s
    side values (+1 on the right, as seen on the plan); empty when it has none."""
    drop = ramp.get('drop')
    if drop is None:
        return []
    if drop not in DROP_SIDES:
        raise ValueError(f"a ramp's drop must be left, right or both, not {drop!r}")
    return DROP_SIDES[drop]


class FeatureSteps:
    """The new feature types' steps of Area.build(), mixed into Area. Each fills its list on the area and marks the
    steep ground it designs in area.designed, which _relax() leaves alone."""

    def _cliff_side(self, d, top, width, crest, wander):
        """A side dropping as a cliff: flat out to top meters from the line, then down over width meters."""
        r = rise((top + width - (d + wander)) / width)
        return crest * r + 0.1 * crest * np.exp(-np.maximum(d - top - width, 0.0) / 2.2) * (1.0 - r)

    def _spines(self, h):
        """Each ridge feature: a crest raised over the ground, with a cliff or a slope on each side."""
        self.spines = []
        for f in self.by_type['ridge']:
            ctrl = to_m(f['path'])
            pts = catmull_rom(ctrl, step=0.5)
            s = arc_length(pts)
            length = s[-1]
            heights = np.asarray(f.get('heights', [f.get('height', 600)] * len(ctrl)), dtype=np.float64) / 100.0
            s_ctrl = control_arcs(pts, s, ctrl)
            width = f.get('width', 1200) / 100.0
            cw = f.get('cliffWidth', 250) / 100.0
            top = f.get('crestWidth', 200) / 200.0
            cliffs = f.get('cliffs', 'both')
            if cliffs not in ('left', 'right', 'both', 'none'):
                raise ValueError(f"ridge {f['id']}: cliffs must be left, right, both or none")
            taper = np.asarray(f.get('taper', [0, 0]), dtype=np.float64) / 100.0
            reach = max(width, top + cw + 5.0) + 2.0
            dist, along, side = polyline_field(self.grid, pts, reach)
            near = np.isfinite(dist)
            d, a = dist[near].astype(np.float64), along[near].astype(np.float64)
            x, y = self.x[near], self.y[near]
            crest = np.interp(a, s_ctrl, heights) * (1.0 + 0.05 * fbm(x, y, 19.0, seed=71, octaves=2))
            for k, t in enumerate(taper):
                if t > 0.0:
                    crest = crest * smoothstep(0.0, t, a if k == 0 else length - a)
            # The signed offset across the line (+ on the right); past an end, across the end's own axis, so the two
            # sides' profiles meet without a step there.
            lateral = d * side[near]
            for cap, end, n in ((a <= 1e-6, pts[0], right_normal(pts[0], pts[1])),
                                (a >= length - 1e-6, pts[-1], right_normal(pts[-2], pts[-1]))):
                if cap.any():
                    lateral[cap] = (x[cap] - end[0]) * n[0] + (y[cap] - end[1]) * n[1]
            wander = 0.8 * fbm(x, y, 11.0, seed=72, octaves=2) + 0.25 * fbm(x, y, 2.6, seed=73)
            profiles = []
            for which in ('left', 'right'):
                if cliffs in (which, 'both'):
                    profiles.append(self._cliff_side(d, top, cw, crest, wander))
                else:
                    profiles.append(crest * (0.5 + 0.5 * np.cos(np.pi * np.minimum(d / width, 1.0))))
            w_right = smoothstep(-0.4, 0.4, lateral)
            h[near] += profiles[0] * (1.0 - w_right) + profiles[1] * w_right
            if cliffs != 'none':
                self.designed[near] |= (d > top - 1.0) & (d < top + cw + 2.0)
            self.spines.append(dict(id=f['id'], feature=f, kind='ridge', cliff_group=f.get('cliffGroup', f['id']),
                                    pts=pts, s=s, s_ctrl=s_ctrl, heights=heights, top=top, cliff_width=cw,
                                    width=width, cliffs=cliffs))
        return h

    def _scarps(self, h):
        """Each scarp: the high side raised along the line, its face a cliff, sloping back down behind."""
        self.scarps = []
        for f in self.by_type['scarp']:
            pts = catmull_rom(to_m(f['path']), step=0.5)
            s = arc_length(pts)
            height = f['height'] / 100.0
            cw = f.get('cliffWidth', 150) / 100.0
            back = f.get('back', 2500) / 100.0
            taper = f.get('taper', 600) / 100.0
            high = 1.0 if f.get('side', 'left') == 'right' else -1.0
            dist, along, side = polyline_field(self.grid, pts, back + cw + 6.0)
            near = np.isfinite(dist)
            x, y = self.x[near], self.y[near]
            a = along[near].astype(np.float64)
            off = (dist[near] * side[near] * high + 0.6 * fbm(x, y, 9.0, seed=81, octaves=2)
                   + 0.15 * fbm(x, y, 2.5, seed=82))  # meters toward the high side
            r = rise((off + cw * 0.5) / cw)
            behind = 1.0 - smoothstep(0.0, 1.0, np.clip((off - cw * 0.5) / back, 0.0, 1.0))
            ends = smoothstep(0.0, taper, a) * smoothstep(0.0, taper, s[-1] - a)
            hh = height * (1.0 + 0.08 * fbm(x, y, 15.0, seed=83, octaves=2)) * ends
            talus = 0.12 * hh * np.exp(-np.maximum(-off - cw * 0.5, 0.0) / 1.8) * (1.0 - r)
            h[near] += hh * r * behind + talus
            self.designed[near] |= (np.abs(off) < cw * 0.5 + 1.5) & (ends > 0.05)
            self.scarps.append(dict(id=f['id'], feature=f, kind='scarp', cliff_group=f.get('cliffGroup', f['id']),
                                    pts=pts, s=s, height=height, cliff_width=cw, high=high, taper=taper))
        return h

    def _pits(self, h):
        """Each pit: the ground inside its rim sinks to its floor, inside an inward-facing cliff."""
        self.pits = []
        for f in self.by_type['pit']:
            width = f.get('cliffWidth', 300) / 100.0
            poly = catmull_rom(to_m(f['polygon']), closed=True, step=0.5)
            rim = float(np.mean(sample(h, poly[:, 0], poly[:, 1], self.half)))
            floor = f['floor'] / 100.0 if 'floor' in f else rim - f['depth'] / 100.0
            sd = signed_distance(self.grid, poly, 14.0)
            sd = (sd + 1.4 * fbm_raster(self.grid, 11.0, seed=91, octaves=2)
                  + 0.35 * fbm_raster(self.grid, 2.6, seed=92, octaves=1))
            sink = rise((width * 0.5 - sd) / width)
            bottom = (floor + 0.3 * fbm_raster(self.grid, 15.0, seed=93, octaves=2)
                      + 1.1 * np.exp(-np.maximum(-sd - width * 0.5, 0.0) / 2.4))  # a talus apron at the wall's foot
            h = h + (bottom - h) * sink
            self.designed |= (sink > 0.01) & (sink < 0.995)
            self.pits.append(dict(id=f['id'], feature=f, kind='pit', cliff_group=f.get('cliffGroup', f['id']),
                                  poly=poly, sd=sd.astype(np.float32), rise=sink.astype(np.float32), floor=floor,
                                  rim=rim, width=width))
        return h

    def _knobs(self, h):
        """Each knob: a level seat for a freestanding rock, a little above the ground around it."""
        self.knobs = []
        for f in self.by_type['knob']:
            cx, cy = to_m(f['center'])
            radius = f.get('radius', 400) / 100.0
            blend = f.get('blend', max(200.0, 80.0 * radius)) / 100.0
            target = float(sample(h, cx, cy, self.half)) + f.get('height', 50) / 100.0
            i0, i1 = self.grid.index_range(cx - radius - blend, cx + radius + blend)
            j0, j1 = self.grid.index_range(cy - radius - blend, cy + radius + blend)
            x, y = self.grid.mesh(i0, i1, j0, j1)
            w = 1.0 - smoothstep(radius, radius + blend, np.hypot(x - cx, y - cy))
            h[i0:i1, j0:j1] += (target - h[i0:i1, j0:j1]) * w
            self.knobs.append(dict(id=f['id'], feature=f, kind='knob', cliff_group=f.get('cliffGroup', f['id']),
                                   center=(cx, cy), radius=radius, blend=blend, z=target, rock=f.get('rock', {})))
        return h

    def _channel(self, h, pts, s, bed, half, bank, seed, depth=None):
        """Carves a channel along a curve: a floor half meters either side of the line at the bed's height (m, along
        s), rounded a little, and banks rising bank meters per meter up to the ground. Cliff walls (a gorge) reach up
        to whatever ground is beside them; sloped banks only about depth + 3 m, fading out past that, so a gully's
        rounded head doesn't bite into a ridge beside it."""
        top = float(np.max(sample(h, pts[:, 0], pts[:, 1], self.half) - bed)) + 3.0  # the banks' greatest height
        if depth is not None:
            top = min(top, depth + 3.0)
        reach = half + top / bank + 3.0
        dist, along, _ = polyline_field(self.grid, pts, reach)
        near = np.isfinite(dist)
        d = dist[near] + 0.4 * fbm(self.x[near], self.y[near], 4.0, seed=seed, octaves=2)
        z = np.interp(along[near], s, bed)
        channel = np.where(d <= half, z + 0.25 * (np.maximum(d, 0.0) / half) ** 2, z + 0.25 + bank * (d - half))
        cut = smin(h[near], channel, 0.35)
        if depth is not None:
            cut = h[near] + (cut - h[near]) * (1.0 - smoothstep(reach - 3.0, reach, dist[near]))
        h[near] = cut
        if bank >= 2.0:
            self.designed[near] |= (d > half - 1.0) & (d < half + top / bank + 2.0)
        return h

    def _gullies(self, h):
        """Each dry gully: a bed below the ground along its path, always running down to its end, cut with its
        banks (after the roads, so a road crossing one keeps its deck height for a bridge)."""
        self.gullies = []
        for f in self.by_type['gully']:
            pts = catmull_rom(to_m(f['path']), step=0.5)
            s = arc_length(pts)
            half = f.get('width', 800) / 200.0
            depth = f['depth'] / 100.0
            walls = f.get('walls', 'sloped')
            bank = GORGE_WALL if walls == 'cliff' else f.get('bank', 0.7)
            ground = sample(h, pts[:, 0], pts[:, 1], self.half)
            # Past the core's edge (a wash spilling over the escarpment's lip) the bed holds its level to the end:
            # the drop beyond mustn't pull it down before the lip.
            on_core = sample(self.edge, pts[:, 0], pts[:, 1], self.half) > 0.5
            if on_core.any() and not on_core.all():
                last = int(np.nonzero(on_core)[0][-1])
                ground[last + 1:] = ground[last]
            bed = np.minimum.accumulate(gauss_smooth_1d(ground, 16.0) - depth)
            h = self._channel(h, pts, s, bed, half, bank, seed=101, depth=None if walls == 'cliff' else depth)
            self.gullies.append(dict(id=f['id'], feature=f, kind='gully', cliff_group=f.get('cliffGroup', f['id']),
                                     pts=pts, s=s, bed=bed, half=half, bank=bank, depth=depth, walls=walls))
        return h

    def _gorges(self, h):
        """Each creek's falls: a slot gorge with cliff walls from the creek's end, its floor the falls' drop below the
        water at the lip and falling on along its path. Its head wall is the falls' face, at the lip."""
        self.gorges = []
        for c in self.creeks:
            falls = c['feature'].get('falls')
            if not falls:
                continue
            spec = falls['gorge']
            pts = catmull_rom(to_m(spec['path']), step=0.5)
            s = arc_length(pts)
            half = spec.get('width', 700) / 200.0
            lip = c['pts'][c['k_lip']]
            lip_water = float(np.interp(c['s_lip'], c['s'], c['water']))
            floor0 = lip_water - falls['drop'] / 100.0
            floor = floor0 - spec.get('fall', 200) / 100.0 * s / max(s[-1], 1e-6)
            # The carve's curve starts half a width in, plus the wall's lean, so the top of its rounded head wall
            # (the falls' face) stands at the lip.
            start = int(np.searchsorted(s, half + (lip_water - floor0) / GORGE_WALL))
            h = self._channel(h, pts[start:], s[start:], floor[start:], half, GORGE_WALL, seed=111)
            gorge = dict(id=spec.get('id', c['id'] + 'Gorge'), creek=c['id'], kind='gorge', pts=pts, s=s,
                         floor=floor, half=half, lip=lip, lip_water=lip_water, drop=falls['drop'] / 100.0,
                         head=float(s[start]),  # meters along it to the carve's start: the falls' face is before it
                         direction=pts[min(8, len(pts) - 1)] - pts[0])  # the way the water leaves the lip
            gorge['cliff_group'] = spec.get('cliffGroup', gorge['id'])
            c['gorge'] = gorge
            self.gorges.append(gorge)
        return h


# --- Where features reach (the seam band check) ---

def _circle(center, radius, count=24):
    a = np.linspace(0.0, 2.0 * math.pi, count, endpoint=False)
    return np.column_stack([center[0] + radius * np.cos(a), center[1] + radius * np.sin(a)])


def feature_extents(layout):
    """Every feature, road, ramp and footprint as (label, points (N, 2) in meters, margin in meters): what it changes
    lies within margin of those points."""
    from area_shape import FOOTPRINTS, ROAD_STYLE  # area_shape imports this module
    out = []
    for f in layout.get('features', []):
        kind, label = f['type'], f"{f['type']} {f['id']}"
        if kind == 'hill':
            out.append((label, _circle(to_m(f['center']), f['radius'] / 100.0), 0.0))
        elif kind in ('plateau', 'mesa', 'pit'):
            poly = catmull_rom(to_m(f['polygon']), closed=True, step=1.0)
            out.append((label, poly, f.get('cliffWidth', 350 if kind != 'pit' else 300) / 200.0 + 4.5))
        elif kind == 'pad':
            sx, sy = np.asarray(f['size'], dtype=np.float64) / 200.0
            yaw = math.radians(f.get('yaw', 0.0))
            corners = np.array([[sx, sy], [-sx, sy], [-sx, -sy], [sx, -sy]])
            rot = np.array([[math.cos(yaw), -math.sin(yaw)], [math.sin(yaw), math.cos(yaw)]])
            out.append((label, to_m(f['center']) + corners @ rot.T, 3.0 if min(sx, sy) < 6.0 else 12.0))
        elif kind == 'pond':
            rx, ry = np.asarray(f['radii'], dtype=np.float64) / 100.0
            a = np.linspace(0.0, 2.0 * math.pi, 32, endpoint=False)
            c = to_m(f['center'])
            out.append((label, np.column_stack([c[0] + 1.35 * rx * np.cos(a), c[1] + 1.35 * ry * np.sin(a)]), 12.0))
        elif kind == 'creek':
            out.append((label, catmull_rom(to_m(f['path']), step=1.0), 16.0))
            gorge = f.get('falls', {}).get('gorge')
            if gorge:
                out.append((label + ' gorge', catmull_rom(to_m(gorge['path']), step=1.0),
                            gorge.get('width', 700) / 200.0 + 3.0))
        elif kind == 'gully':
            bank = GORGE_WALL if f.get('walls') == 'cliff' else f.get('bank', 0.7)
            out.append((label, catmull_rom(to_m(f['path']), step=1.0),
                        f.get('width', 800) / 200.0 + (f['depth'] / 100.0 + 3.0) / bank + 3.0))
        elif kind == 'ridge':
            reach = max(f.get('width', 1200) / 100.0, f.get('crestWidth', 200) / 200.0 + f.get('cliffWidth', 250)
                        / 100.0)
            out.append((label, catmull_rom(to_m(f['path']), step=1.0), reach + 3.0))
        elif kind == 'scarp':
            out.append((label, catmull_rom(to_m(f['path']), step=1.0),
                        f.get('back', 2500) / 100.0 + f.get('cliffWidth', 150) / 100.0 + 2.0))
        elif kind == 'knob':
            radius = f.get('radius', 400) / 100.0
            out.append((label, to_m([f['center']]), radius + f.get('blend', max(200.0, 80.0 * radius)) / 100.0))
        for ramp in [f['ramp']] if 'ramp' in f else []:
            out.append((label + ' ramp', catmull_rom(to_m(ramp['path']), step=1.0), ramp['width'] / 200.0 + 8.0))
    for road in layout.get('roads', []):
        style = ROAD_STYLE.get(road.get('kind', 'dirt'), ROAD_STYLE['dirt'])
        out.append((f"road {road['id']}", catmull_rom(to_m(road['path']), step=1.0),
                    road['width'] / 200.0 + style['shoulder'] + 0.5))
    for p in layout.get('placements', []):
        if p['kind'] in FOOTPRINTS:
            radius, blend = FOOTPRINTS[p['kind']]
            out.append((f"placement {p['id']}", to_m([p['location'][:2]]), radius + blend))
    return out


def seam_conflicts(area):
    """What reaches into a grounded area's seam band (or past the core square), on the lip's upland side, as
    messages: features, roads, ramps, footprints and the playable boundary's corners."""
    region = area.region
    inner = area.half - region.seam
    extents = feature_extents(area.layout)
    if area.layout.get('boundary'):
        extents.append(('the playable boundary', to_m(area.layout['boundary']['polygon']), 0.0))
    found = []
    ring = np.vstack([[0.0, 0.0], _circle((0.0, 0.0), 1.0, 16)])
    for label, pts, margin in extents:
        # Each point's reach: its margin all round, where that's on the upland (over the canyon it doesn't count).
        pts = (np.atleast_2d(pts)[:, None, :] + ring[None, :, :] * margin).reshape(-1, 2)
        upland = region.upland(pts[:, 0], pts[:, 1])
        if not upland.any():
            continue
        reach = float(np.max(np.maximum(np.abs(pts[upland, 0]), np.abs(pts[upland, 1]))))
        if reach > inner + 1e-6:
            found.append(f'{label} reaches {reach - inner:.1f} m into the seam band (the core\'s outer '
                         f'{region.seam:g} m; keep it within {inner:g} m of the center on both axes)')
    return found
