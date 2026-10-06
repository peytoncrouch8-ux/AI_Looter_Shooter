"""The grounded setting (layout.json "setting": "grounded"; numpy only, area_math.py has the conventions): the regional
height field the core square sits in, the core's edge, the playable boundary and the seam band.

A grounded area is not an island: it has no rim, no underside and no outline. Its map square is the core, and a regional
height field R (layout.json "region") is defined all around it, out to the surround ring's square:
- a valley floor: large-scale noise on a gentle tilt, raised or lowered by benches (a terrace, a lowland past a ridge:
  "benches", each a polygon whose ground stands "height" above or below the floor, easing out over "blend" outside it);
- ridges, each rising from its foot (a polyline, on the side away from the area's center): a cliff band at the foot,
  a slope up to the crest, and an outer slope back down; a saddle ("saddles": a center, a radius and the crest's
  "height" there) brings the crest down near a pass without lowering the band;
- an escarpment: a lip (a polyline) past which the ground drops into a canyon, with the canyon's floor, its river and
  its far wall below. The lip wanders a meter or so, except in its "calm" stretches (a center and a radius), where a
  model is fitted to it (Ransom's Rest's burial deck).
R is worked out on one raster over the ring's square (the region grid, 1 m cells): its smooth parts (noise, the canyon
floor) as heights and its sharp parts (the ridges' feet, the lip) as distance fields. heights() turns sampled fields
into R, so the core's raster, the ring's raster and single points (the seam's vertices) all see the same field, each at
its own resolution: a 10 m cliff band stays sharp on the core's 10 cm raster.

The core is the square on the upland side of the lip: its mesh's edge is the square on the ridge sides and the lip on
the canyon side. The square's outer seamBand (25 m) is the seam band: every core feature keeps clear of it (Area.build()
stops with an error otherwise), and there the core's own noise fades out, so at the square's edge the core is R exactly
and meets the surround ring without a crack (area_beyond.py builds the ring, the canyon wall and the backdrop).

The playable boundary (layout.json "boundary") is a polygon with its open edges, where a drop is part of play (the lip,
a falls); layout_computed.json hands its corners and open edges to the game's APlayableArea.
"""
import math

import numpy as np

from area_math import (Grid, catmull_rom, fbm, normals_of, points_in_polygon, polygon_mask, polyline_field,
                       raster_size, resample, sample, signed_distance, smoothstep)

LIP_STEP = 0.5        # meters between the lip's points
LIP_NEAR = 40.0       # how far (m) the lip's exact distance field reaches; past it a coarse one is enough
ROLL = (1.6, 0.3)     # the ground rounds over the lip: within this distance (m) of it, dropping this far (m)
WALL_TOP = 0.3        # the drop starts this far (m) past the lip, so the lip's own vertices stay on the upland
WALL_BATTER = 0.17    # the canyon wall leans out this far (m) per meter of drop: steep, never overhanging
SEAM_STEP = 1.2       # meters between the core's edge vertices on the square (the seam); the lip's are 0.7 m
BAND_FADE = (12.0, 4.0)  # a ridge's cliff band fades out between these distances (m) inside the core square's edge


def _to_m(points):
    return np.asarray(points, dtype=np.float64) / 100.0


def _extend(poly, length):
    """A polyline with a point added beyond each end, length meters out along the end's direction."""
    a = poly[0] - poly[1]
    b = poly[-1] - poly[-2]
    a /= max(np.linalg.norm(a), 1e-9)
    b /= max(np.linalg.norm(b), 1e-9)
    return np.vstack([poly[:1] + a * length, poly, poly[-1:] + b * length])


def side_at(poly, point):
    """The side (+1 or -1, as polyline_field() gives it) of a point against a polyline: its closest segment's."""
    best, sign = np.inf, 1
    for a, b in zip(poly[:-1], poly[1:]):
        d = b - a
        t = np.clip(((point[0] - a[0]) * d[0] + (point[1] - a[1]) * d[1]) / max(d @ d, 1e-12), 0.0, 1.0)
        dist = math.hypot(point[0] - a[0] - t * d[0], point[1] - a[1] - t * d[1])
        if dist < best:
            best = dist
            sign = 1 if d[0] * (point[1] - a[1]) - d[1] * (point[0] - a[0]) >= 0.0 else -1
    return sign


def _soft_max(a, b, k):
    """max(a, b), rounded within k where both are up (ridges meeting at a corner); exact where either is 0."""
    k = k * smoothstep(0.0, k, np.minimum(a, b)) + 1e-6
    h = np.clip(0.5 + 0.5 * (a - b) / k, 0.0, 1.0)
    return b + (a - b) * h + k * h * (1.0 - h)


class Region:
    """A grounded area's regional height field (layout.json "region"). The constructor reads the geometry (cheap: the
    layout checker uses it); build_fields() works out the region grid's rasters, which heights() needs."""

    def __init__(self, area):
        spec = area.layout['region']
        self.spec = spec
        self.core_half = area.half
        ring = spec.get('ring', {})
        self.half = ring.get('half', 4.0 * area.half_cm) / 100.0
        if self.half < area.half + 20.0:
            raise ValueError(f'{area.path}: region.ring.half must reach well past the core square')
        self.seam = spec.get('seamBand', 2500) / 100.0
        self.ridges = [self._ridge(r) for r in spec.get('ridges', [])]
        self.benches = [self._bench(b) for b in spec.get('benches', [])]
        self.escarpment = spec.get('escarpment')
        self.lip = self.lip_coarse = self.canyon_polygon = self.river = None
        if self.escarpment:
            self._lip(area)
        self.fields = None

    # --- Geometry ---

    def _ridge(self, spec):
        foot = _extend(catmull_rom(_to_m(spec['foot']), step=2.0), 3.0 * self.half)
        ridge = dict(id=spec['id'], spec=spec, foot=resample(foot, 4.0), height=spec['height'] / 100.0,
                     depth=spec.get('crest', 3000) / 100.0, back=spec.get('back', 6000) / 100.0,
                     band=spec.get('band', 0) / 100.0)
        ridge['band_width'] = spec.get('bandWidth', max(150.0, 25.0 * ridge['band'])) / 100.0
        if ridge['band'] > ridge['height'] or ridge['band_width'] >= ridge['depth']:
            raise ValueError(f"ridge {spec['id']}: its band must be lower than the ridge and narrower than its crest")
        # The ridge rises on the side of its foot away from the area's center.
        ridge['side'] = -side_at(ridge['foot'], (0.0, 0.0))
        # A polygon round the ridge's side (the foot closed far out that way): the side of points past the foot's
        # distance field, so the field never jumps from one side's value to the other's there.
        out = normals_of(ridge['foot']) * ridge['side']
        far = 12.0 * self.half
        ridge['polygon'] = np.vstack([ridge['foot'], ridge['foot'][-1:] + out[-1:] * far,
                                      ridge['foot'][:1] + out[:1] * far])
        # Saddles: (center, radius, the crest's height at the center), meters.
        ridge['saddles'] = [(_to_m(s['center']), s['radius'] / 100.0, s['height'] / 100.0)
                            for s in spec.get('saddles', [])]
        for _, radius, height in ridge['saddles']:
            if height < 1.3 * ridge['band'] or height > ridge['height'] or radius <= 0.0:
                raise ValueError(f"ridge {spec['id']}: a saddle's height must lie between 1.3 times the band and the "
                                 'ridge\'s height, and its radius be positive')
        return ridge

    def _bench(self, spec):
        """A bench: the valley floor raised (a terrace) or lowered (a lowland) inside a polygon, easing out over its
        blend outside it."""
        poly = catmull_rom(_to_m(spec['polygon']), closed=True, step=2.0)
        bench = dict(id=spec['id'], polygon=poly, height=spec['height'] / 100.0, blend=spec.get('blend', 1000) / 100.0)
        if bench['blend'] <= 0.0:
            raise ValueError(f"bench {spec['id']}: its blend must be positive")
        return bench

    def _lip(self, area):
        esc = self.escarpment
        ctrl = _extend(catmull_rom(_to_m(esc['lip']), step=2.0), 3.0 * self.half)
        dense = resample(ctrl, LIP_STEP)
        # The lip wanders a little, like the island's outline: bays and spurs of a meter or so.
        normal = normals_of(dense)
        wobble = (1.4 * fbm(dense[:, 0], dense[:, 1], 18.0, seed=311, octaves=2)
                  + 0.45 * fbm(dense[:, 0], dense[:, 1], 5.0, seed=312))
        for spot in esc.get('calm', []):
            # Calm stretches (a center and a radius, cm): the lip runs where the layout draws it, for a model fitted to
            # it (the burial deck's bearers on the rim); the wobble fades back in toward the radius.
            center, radius = _to_m(spot['center']), spot['radius'] / 100.0
            wobble = wobble * smoothstep(0.7 * radius, radius, np.hypot(dense[:, 0] - center[0],
                                                                       dense[:, 1] - center[1]))
        self.lip = resample(dense + normal * wobble[:, None], LIP_STEP)
        self.lip_coarse = resample(ctrl, 8.0)
        self.lip_up = side_at(self.lip_coarse, (0.0, 0.0))  # the upland's side: the area's center is on it
        # A polygon around the canyon's side (the lip closed far out that way), for the side of far points.
        out = normals_of(self.lip_coarse) * -self.lip_up
        far = 12.0 * self.half
        self.canyon_polygon = np.vstack([self.lip_coarse, self.lip_coarse[-1:] + out[-1:] * far,
                                         self.lip_coarse[:1] + out[:1] * far])
        self.drop = esc['drop'] / 100.0
        self.wall = esc.get('wall', self.drop * WALL_BATTER * 100.0) / 100.0
        self.kit = esc.get('kit', 2000) / 100.0
        far_wall = esc.get('farWall', {})
        self.far = dict(distance=far_wall.get('distance', 25000) / 100.0, height=far_wall.get('height', 3000) / 100.0,
                        width=far_wall.get('width', 4000) / 100.0)
        # The river: along the canyon, about two fifths of the way to the far wall, meandering.
        river = esc.get('river', {})
        if 'path' in river:
            self.river = catmull_rom(_to_m(river['path']), step=2.0)
        else:
            offset = river.get('offset', 0.4 * self.far['distance'] * 100.0) / 100.0
            base = resample(ctrl, 6.0)
            s = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(base, axis=0), axis=1))])
            meander = 9.0 * np.sin(s / 70.0) + 5.0 * fbm(s / 40.0, 0.5, 1.0, seed=313, octaves=2)
            self.river = base + (normals_of(base) * -self.lip_up) * (offset + meander)[:, None]
        self.river_width = river.get('width', 600) / 100.0

    def upland(self, x, y):
        """Whether points are on the lip's upland side (all of them when there's no escarpment)."""
        if self.lip is None:
            return np.ones(np.shape(x), dtype=bool)
        return ~points_in_polygon(np.asarray(x, np.float64), np.asarray(y, np.float64), self.canyon_polygon)

    def core_outline(self):
        """The core mesh's edge, counter-clockwise in (x, y): the square on the upland side of the lip. Returns the
        polygon (N, 2) and, for each point, whether it lies on the lip (else on the square: the seam)."""
        h = self.core_half
        # The square's corners in order around it (south-west, north-west, north-east, south-east).
        corners = np.array([[-h, -h], [h, -h], [h, h], [-h, h]], dtype=np.float64)
        if self.lip is None:
            return corners, np.zeros(4, dtype=bool)
        lip = self.lip
        inside = (np.abs(lip[:, 0]) < h) & (np.abs(lip[:, 1]) < h)
        if not inside.any():
            return corners, np.zeros(4, dtype=bool)
        idx = np.nonzero(inside)[0]
        i0, i1 = int(idx[0]), int(idx[-1])
        if not inside[i0:i1 + 1].all() or i0 == 0 or i1 == len(lip) - 1:
            raise ValueError('the escarpment lip must cross the core square once, entering and leaving through its '
                             'edges')
        a = _square_crossing(lip[i0 - 1], lip[i0], h)
        b = _square_crossing(lip[i1 + 1], lip[i1], h)
        part = np.vstack([a, lip[i0:i1 + 1], b])
        perimeter = 8.0 * h
        ta, tb = _perimeter_t(a, h), _perimeter_t(b, h)
        best = None
        for direction in (1.0, -1.0):
            # Walk the square from the lip's exit (b) back to its entry (a), one way round or the other, taking the
            # corners on the way (corner k is 2 k h meters round from the south-west one).
            span = ((ta - tb) * direction) % perimeter
            between = sorted((((2.0 * h * k - tb) * direction) % perimeter, k) for k in range(4))
            path = [corners[k] for offset, k in between if 1e-9 < offset < span - 1e-9]
            poly = np.vstack([part] + ([np.array(path)] if path else []))
            if points_in_polygon(np.array([0.0]), np.array([0.0]), poly)[0]:
                best = poly
                on_lip = np.zeros(len(poly), dtype=bool)
                on_lip[:len(part)] = True
                break
        if best is None:
            raise ValueError("the area's center must be on the lip's upland side")
        if _area(best) < 0.0:
            best, on_lip = best[::-1].copy(), on_lip[::-1].copy()
        return best, on_lip

    # --- The fields ---

    def build_fields(self, log=print):
        """The region grid's rasters: the noise, each ridge's signed distance from its foot, the lip's signed distance
        and the canyon's floor."""
        n = raster_size(2.0 * self.half, self.spec.get('cell', 100) / 100.0, 4096)
        self.grid = Grid(n, self.half)
        log(f'region: {n} x {n} cells over a {2.0 * self.half:g} m square')
        g = self.grid
        x, y = g.mesh()
        noise = self.spec.get('noise', {})
        tilt = self.spec.get('tilt', [0.0, 0.0])
        f = {}
        f['base'] = (noise.get('amplitude', 0.0) / 100.0 * fbm(x, y, noise.get('wavelength', 6000) / 100.0, seed=301,
                                                                octaves=2, gain=0.35)
                     + tilt[0] * x + tilt[1] * y)
        for bench in self.benches:
            # Inside the polygon the bench's full height; outside it, easing out over the blend.
            outside = np.maximum(signed_distance(g, bench['polygon'], bench['blend'] + 2.0), 0.0)
            f['base'] += bench['height'] * (1.0 - smoothstep(0.0, bench['blend'], outside))
        f['var'] = fbm(x, y, 45.0, seed=302, octaves=2)
        for i, r in enumerate(self.ridges):
            # Meters past the foot, into the ridge (negative on the valley's side), kept to the range the profile
            # changes over: a sample between two cells never sweeps through the crest's height.
            reach = r['depth'] + r['back'] + 12.0
            dist, _, side = polyline_field(g, r['foot'], reach)
            near = np.isfinite(dist)
            sign = np.where(near, np.where(side == r['side'], 1.0, -1.0),
                            np.where(polygon_mask(g, r['polygon']), 1.0, -1.0))
            s = np.where(near, dist, reach) * sign
            f[f'ridge{i}'] = np.clip(s, -4.0, r['depth'] + r['back'] + 2.0)
        if self.lip is not None:
            f['dl'] = self._lip_distance(g)
            f['canyon'] = self._canyon(g, x, y, f['dl'])
        self.fields = {k: v.astype(np.float32) for k, v in f.items()}

    def _lip_distance(self, g):
        """Signed distance (m) from the lip, positive on the upland: exact near the lip, from the coarse lip farther
        out, and only its side past the far wall."""
        near, _, side = polyline_field(g, self.lip, LIP_NEAR)
        reach = self.far['distance'] + self.far['width'] + 80.0
        far, _, _ = polyline_field(g, self.lip_coarse, reach)
        sign = np.where(polygon_mask(g, self.canyon_polygon), -1.0, 1.0)
        d = np.where(np.isfinite(far), far, reach) * sign
        return np.where(np.isfinite(near), near * np.where(side == self.lip_up, 1.0, -1.0), d)

    def _canyon(self, g, x, y, dl):
        """The canyon's floor (m): rolling ground at the drop's depth, the river's channel and the far wall."""
        esc = self.escarpment
        floor_noise = esc.get('floorNoise', {})
        c = (-self.drop + floor_noise.get('amplitude', 250) / 100.0
             * fbm(x, y, floor_noise.get('wavelength', 12000) / 100.0, seed=321, octaves=3, gain=0.45))
        dist, _, _ = polyline_field(g, self.river, self.river_width * 0.5 + 12.0)
        dist = np.where(np.isfinite(dist), dist, 1e3)
        c -= 1.1 * (1.0 - smoothstep(self.river_width * 0.5, self.river_width * 0.5 + 6.0, dist))
        self.river_distance = dist.astype(np.float32)
        # The far wall: its foot wanders by a couple of dozen meters; its face steps in ledges.
        far = self.far
        u = (-dl - far['distance'] - 22.0 * fbm(x, y, 110.0, seed=322, octaves=2)) / far['width']
        u = np.clip(u, 0.0, 1.0)
        steps = u + 0.06 * np.sin(u * 6.0 * math.pi)
        c += far['height'] * (smoothstep(0.0, 1.0, steps) + 0.04 * fbm(x, y, 30.0, seed=323))
        return c

    def at(self, x, y):
        """The fields sampled (bilinear) at points."""
        return {k: sample(v, x, y, self.half) for k, v in self.fields.items()}

    def heights(self, f, x, y):
        """R (m) from fields sampled at points (x, y)."""
        x, y = np.broadcast_arrays(np.asarray(x, dtype=np.float64), np.asarray(y, dtype=np.float64))
        z = np.zeros(x.shape)
        if self.ridges:
            fade = 1.0 - smoothstep(BAND_FADE[0], BAND_FADE[1], self.core_half - np.maximum(np.abs(x), np.abs(y)))
            # Short noise, worked out at the points themselves (a 1 m raster would facet it), only near the feet: the
            # feet wander a meter or so, so no band runs ruler-straight, and the bands' height varies.
            active = np.zeros(x.shape, dtype=bool)
            for i, r in enumerate(self.ridges):
                active |= (f[f'ridge{i}'] > -4.0) & (f[f'ridge{i}'] < r['depth'] + 2.0)
            fine, wob = np.zeros(x.shape), np.zeros(x.shape)
            if active.any():
                xa, ya = x[active], y[active]
                fine[active] = fbm(xa, ya, 14.0, seed=303, octaves=2)
                wob[active] = 1.2 * fbm(xa, ya, 16.0, seed=304, octaves=2) + 0.3 * fbm(xa, ya, 4.0, seed=305)
            for i, r in enumerate(self.ridges):
                s = f[f'ridge{i}']
                zi = self._ridge_height(r, s + wob * (1.0 - smoothstep(r['depth'] - 6.0, r['depth'], s)), f['var'],
                                        fine, fade, self._saddle_top(r, x, y))
                z = zi if i == 0 else _soft_max(z, zi, 3.0)
        upland = f['base'] + z
        if self.lip is None:
            return upland
        dl = f['dl']
        upland = upland - ROLL[1] * (1.0 - smoothstep(0.0, ROLL[0], dl))
        u = np.clip((-dl - WALL_TOP) / self.wall, 0.0, 1.0)
        drop = 1.0 - (1.0 - u) ** 2.2  # most of the drop at once, then a foot that flares out
        return upland - (upland - f['canyon']) * drop

    @staticmethod
    def _saddle_top(r, x, y):
        """A ridge's crest height (m) at points, lowered toward each saddle's center; None without saddles."""
        if not r['saddles']:
            return None
        top = np.full(np.shape(x), r['height'])
        for (cx, cy), radius, height in r['saddles']:
            w = 1.0 - smoothstep(0.3 * radius, radius, np.hypot(x - cx, y - cy))
            top = top + (height - top) * w
        return top

    @staticmethod
    def _ridge_height(r, s, var, fine, fade, top=None):
        """A ridge's height (m) at s meters past its foot (negative on the valley's side): a cliff band at the foot,
        then a slope that eases out of the band's top and over the crest (no crease where the band fades out near the
        core square's edge: the slope then starts at the foot), then the outer slope down to the valley's level. top
        is the crest's height per point where a saddle lowers it (else the ridge's own)."""
        height = (r['height'] if top is None else top) * (1.0 + 0.08 * var)
        band = r['band'] * (1.0 + 0.2 * fine) * fade
        depth = r['depth']
        wb = r['band_width'] * fade
        k = 4.2
        t = np.clip(s / np.maximum(wb, 1e-3), 0.0, 1.0)
        z = band * (0.5 + 0.5 * np.tanh(k * (t - 0.5)) / math.tanh(k * 0.5))
        u = np.clip((s - wb) / (depth - wb), 0.0, 1.0)
        z = z + (height - band) * (0.5 - 0.5 * np.cos(np.pi * u))
        v = np.clip((s - depth) / r['back'], 0.0, 1.0)
        return z - height * smoothstep(0.0, 1.0, v)

    def height(self, x, y):
        """R (m) at points (arrays of any shape, or single numbers)."""
        x, y = np.broadcast_arrays(np.asarray(x, dtype=np.float64), np.asarray(y, dtype=np.float64))
        flat_x, flat_y = x.reshape(-1), y.reshape(-1)
        return self.heights(self.at(flat_x, flat_y), flat_x, flat_y).reshape(x.shape)

    @staticmethod
    def wobble(x, y):
        """How far (m) the ridges' feet are pushed into the ridge at a point (the band's foot is that far back)."""
        return 1.2 * fbm(x, y, 16.0, seed=304, octaves=2) + 0.3 * fbm(x, y, 4.0, seed=305)

    def band_height(self, r, x, y):
        """A ridge's cliff band's height (m) at a point on its foot, as heights() builds it."""
        fade = 1.0 - smoothstep(BAND_FADE[0], BAND_FADE[1], self.core_half - max(abs(x), abs(y)))
        return float(r['band'] * (1.0 + 0.2 * fbm(x, y, 14.0, seed=303, octaves=2)) * fade)

    def raster(self, grid, chunk=256):
        """R over another grid (the core's), row block by row block."""
        out = np.empty((grid.n, grid.n), dtype=np.float32)
        for a in range(0, grid.n, chunk):
            x, y = grid.mesh(a, min(grid.n, a + chunk))
            out[a:a + chunk] = self.height(x, y)
        return out

    def lip_distance(self, grid, chunk=256):
        """The lip's signed distance over another grid (m, positive on the upland)."""
        out = np.empty((grid.n, grid.n), dtype=np.float32)
        for a in range(0, grid.n, chunk):
            x, y = grid.mesh(a, min(grid.n, a + chunk))
            out[a:a + chunk] = sample(self.fields['dl'], x, y, self.half)
        return out

    def gradient_normals(self, x, y, step=0.25):
        """Unit normals (layout axes: x north, y east, z up) of R at points, from its gradient."""
        gx = (self.height(x + step, y) - self.height(x - step, y)) / (2.0 * step)
        gy = (self.height(x, y + step) - self.height(x, y - step)) / (2.0 * step)
        n = np.column_stack([-gx, -gy, np.ones_like(gx)])
        return n / np.linalg.norm(n, axis=1, keepdims=True)


def _square_crossing(outside, inside, h):
    """Where the segment from a point outside the square of half side h to one inside it crosses the square's edge
    (bisection: the square is convex, so there is one crossing), snapped onto the edge."""
    lo, hi = 0.0, 1.0  # outside + (inside - outside) * lo stays outside, * hi inside
    for _ in range(60):
        mid = 0.5 * (lo + hi)
        p = outside + (inside - outside) * mid
        if max(abs(p[0]), abs(p[1])) < h:
            hi = mid
        else:
            lo = mid
    p = outside + (inside - outside) * (0.5 * (lo + hi))
    axis = 0 if abs(abs(p[0]) - h) < abs(abs(p[1]) - h) else 1
    p[axis] = math.copysign(h, p[axis])
    return p


def _perimeter_t(p, h):
    """A point on the square's edge as meters around it from the south-west corner, by way of the west edge."""
    x, y = p
    if abs(y + h) < 1e-6:
        return x + h                       # the west edge, northward
    if abs(x - h) < 1e-6:
        return 2.0 * h + (y + h)           # the north edge, eastward
    if abs(y - h) < 1e-6:
        return 4.0 * h + (h - x)           # the east edge, southward
    return 6.0 * h + (h - y)               # the south edge, westward


def _area(poly):
    x, y = poly[:, 0], poly[:, 1]
    return 0.5 * float(np.sum(x * np.roll(y, -1) - np.roll(x, -1) * y))


def seam_weight(area, x, y):
    """1 inside the seam band's inner edge, fading to 0 at the core square's edge."""
    return smoothstep(0.0, area.region.seam, area.half - np.maximum(np.abs(x), np.abs(y)))


def boundary(area):
    """The playable boundary (layout.json "boundary"): its corners (N, 2, meters) and one flag per edge (edge i runs
    from corner i to corner i + 1; True where it's open). "open" lists runs of open edges by corner: from corner a to
    corner b (b may be the corner count, meaning back to corner 0)."""
    spec = area.layout.get('boundary')
    if not spec:
        return None, None
    corners = _to_m(spec['polygon'])
    count = len(corners)
    if count < 3:
        raise ValueError(f'{area.path}: the boundary needs at least three corners')
    flags = [False] * count
    for run in spec.get('open', []):
        a, b = int(run['from']), int(run['to'])
        if not 0 <= a < b <= count:
            raise ValueError(f'{area.path}: boundary.open run {run} is outside corners 0..{count}')
        for edge in range(a, b):
            flags[edge] = True
    return corners, flags


def edges_cross(corners):
    """Pairs of the polygon's edges that cross (it must be simple)."""
    count = len(corners)
    bad = []
    for i in range(count):
        a, b = corners[i], corners[(i + 1) % count]
        for j in range(i + 2, count):
            if i == 0 and j == count - 1:
                continue
            c, d = corners[j], corners[(j + 1) % count]
            r, s = b - a, d - c
            denom = r[0] * s[1] - r[1] * s[0]
            if abs(denom) < 1e-12:
                continue
            t = ((c[0] - a[0]) * s[1] - (c[1] - a[1]) * s[0]) / denom
            u = ((c[0] - a[0]) * r[1] - (c[1] - a[1]) * r[0]) / denom
            if 0.0 < t < 1.0 and 0.0 < u < 1.0:
                bad.append((i, j))
    return bad
