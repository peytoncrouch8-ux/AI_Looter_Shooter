"""The tutorial island's shape, computed from layout.json (numpy only; see island_math.py for the conventions).

Island().build() fills one height raster (10 cm cells over the map square) in this order, each step working on the
result of the one before: meadow noise, hills, the plateau and its cliffs, pads, building footprints, roads, the plateau
ramp, the pond, the creek, and the rim rolling over the island's edge. It keeps what the macro painter and the mesh
builder need: the heights, the water surface, the island outline, the curves of the roads, the creek and the ramp.
"""
import json
import math
import os

import numpy as np

from island_math import (Grid, arc_length, blur, catmull_rom, fbm, fbm_raster, gauss_smooth_1d, normals_of,
                         points_in_polygon, polygon_area, polyline_field, resample, sample, signed_distance, smax, smin,
                         smoothstep)

HERE = os.path.dirname(os.path.abspath(__file__))
LAYOUT_PATH = os.path.join(HERE, 'layout.json')

# Flat footprints under buildings (radius, blend) in meters, by placement kind. The ground is flattened to its own
# height at the building's location, so a building on a slope or at a pad's edge doesn't float or sink.
FOOTPRINTS = {
    'Farmhouse': (7.0, 4.0), 'Barn': (8.0, 4.0), 'LogCabin': (5.5, 3.5), 'Cottage': (5.5, 3.5),
    'Outhouse': (2.0, 3.0), 'Well': (2.5, 2.5), 'Windmill': (3.0, 4.5), 'LookoutTower': (4.0, 3.0),
    'GunRack': (2.0, 2.0),
}
ROAD_STYLE = {  # how deep a road is carved and how wide its soft shoulders are (m), by kind
    'dirt': dict(carve=0.12, shoulder=2.2),
    'path': dict(carve=0.07, shoulder=2.0),
}
RIM_ROLL = 3.2          # the meadow rolls over the island's edge within this distance of the rim...
RIM_ROLL_DROP = 1.2     # ...dropping this far before the rock wall (the underside's rim) starts
CREEK_DEPTH = 0.45      # water depth in the middle of the creek
CREEK_WATER_HALF = 1.2  # half the creek's water width
POND_SHORE_SLOPE = 0.16


def to_m(points):
    """Layout centimeters [[X, Y], ...] to layout meters (N, 2)."""
    return np.asarray(points, dtype=np.float64) / 100.0


def load_layout(path=LAYOUT_PATH):
    with open(path, encoding='utf-8') as file:
        return json.load(file)


class Island:
    def __init__(self, layout=None, raster=2048):
        self.layout = layout or load_layout()
        self.grid = Grid(raster)
        self.features = {f['id']: f for f in self.layout['features']}
        self.island = self.layout['island']

    # --- The steps ---

    def build(self, log=print):
        grid = self.grid
        x, y = grid.mesh()
        self.x, self.y = x, y
        self._outline()
        log('terrain: meadow and hills')
        h = self._meadow(x, y)
        self.h_natural = h.astype(np.float32)
        log('terrain: plateau')
        h = self._plateau(x, y, h)
        log('terrain: pads and footprints')
        h = self._pads(x, y, h)
        h = self._footprints(h)
        # The pond and the creek's valley come before the roads (a road passing the pond keeps its shoulders, and
        # the forest road follows the valley down to the bridge); the creek's channel after them (it runs under the
        # bridge).
        log('terrain: pond, roads, ramp and creek')
        h = self._pond(x, y, h)
        h = self._creek_valley(x, y, h)
        h = self._roads(h)
        h = self._ramp(h)
        h = self._creek(x, y, h)
        h = self._relax(h)
        log('terrain: rim')
        h = self._rim(x, y, h)
        self.h = h.astype(np.float32)
        del self.x, self.y
        return self

    def _relax(self, h, limit=30.0, passes=4):
        """Eases banks steeper than about limit degrees that no feature asked for (where a pad, a footprint and a
        path's cut meet on a hillside) toward a blurred copy of the ground. Designed steep ground is left alone:
        the plateau's cliffs, the ramp's walls, the creek's channel, road surfaces and the rim."""
        keep = np.zeros(h.shape, dtype=bool)
        if self.plateau is not None:
            keep |= (self.plateau['rise'] > 0.01) & (self.plateau['rise'] < 0.995)
            keep |= self.plateau['sd'] < self.plateau['width'] * 0.5 + 3.0
        if self.ramp is not None:
            dist, _, _ = polyline_field(self.grid, self.ramp['pts'], 12.0)
            keep |= np.isfinite(dist)
        if self.creek is not None:
            keep |= np.isfinite(self.creek['dist']) & (self.creek['dist'] < 3.5)
        keep |= self.road_gap < 0.3
        keep |= self.edge < RIM_ROLL + 1.0
        free = 1.0 - blur(keep.astype(np.float32), 5)  # fades in over half a meter from protected ground
        for _ in range(passes):
            gx, gy = np.gradient(h, self.grid.px)
            slope = np.degrees(np.arctan(np.hypot(gx, gy)))
            w = smoothstep(limit - 4.0, limit + 6.0, blur(slope.astype(np.float32), 4)) * free
            h = h + (blur(h, 8) - h) * w
        return h

    def _outline(self):
        """The island's edge: the layout's outline as a smooth curve, pushed in and out a little by noise."""
        pts = catmull_rom(to_m(self.layout['outline']), closed=True, step=0.5)
        if polygon_area(pts) < 0.0:
            pts = pts[::-1].copy()
        inward = normals_of(pts, closed=True)  # the left normal points inside for a counter-clockwise (x, y) loop
        px, py = pts[:, 0], pts[:, 1]
        wobble = 1.4 * fbm(px, py, 18.0, seed=41, octaves=2) + 0.45 * fbm(px, py, 5.0, seed=42)
        pts = pts - inward * wobble[:, None]
        self.outline = resample(pts, 0.5, closed=True)
        self.edge = -signed_distance(self.grid, self.outline, 14.0)  # meters inside the rim (negative outside)
        self.inside = self.edge > 0.0

    def _meadow(self, x, y):
        base, micro = self.island['baseNoise'], self.island['microNoise']
        # Each noise is the layout's amplitude at its wavelength (the base with a faint second octave).
        h = base['amplitude'] / 100.0 * fbm_raster(self.grid, base['wavelength'] / 100.0, seed=1, octaves=2, gain=0.35)
        h += micro['amplitude'] / 100.0 * fbm_raster(self.grid, micro['wavelength'] / 100.0, seed=2, octaves=1)
        # Hills, their outlines bent by a warp so none is a perfect dome. The warp fades out toward the top, so the
        # summit stays at the hill's center (the windmill stands there).
        wx = 4.0 * fbm_raster(self.grid, 24.0, seed=7, octaves=1)
        wy = 4.0 * fbm_raster(self.grid, 24.0, seed=8, octaves=1)
        for f in self.layout['features']:
            if f['type'] != 'hill':
                continue
            cx, cy = np.asarray(f['center']) / 100.0
            radius = f['radius'] / 100.0
            fade = smoothstep(0.1, 0.55, np.hypot(x - cx, y - cy) / radius)
            r = np.hypot(x + wx * fade - cx, y + wy * fade - cy) / radius
            h += f['height'] / 100.0 * (0.5 + 0.5 * np.cos(np.pi * np.minimum(r, 1.0)))
        return h

    def _plateau(self, x, y, h):
        f = self.features.get('plateau')
        self.plateau = None
        if f is None:
            return h
        top = f['height'] / 100.0
        width = f.get('cliffWidth', 350) / 100.0
        poly = catmull_rom(to_m(f['polygon']), closed=True, step=0.5)
        sd = signed_distance(self.grid, poly, 14.0)
        # The cliff line wanders: large bays and spurs plus a jagged top edge.
        sd = sd + 1.4 * fbm_raster(self.grid, 11.0, seed=11, octaves=2) + 0.35 * fbm_raster(self.grid, 2.6, seed=12,
                                                                                              octaves=1)
        t = np.clip((width * 0.5 - sd) / width, 0.0, 1.0)
        k = 4.2  # steep in the middle, a rounded lip at the top and a foot at the bottom
        rise = 0.5 + 0.5 * np.tanh(k * (t - 0.5)) / math.tanh(k * 0.5)
        top_h = top + 0.45 * fbm_raster(self.grid, 19.0, seed=13, octaves=2)
        top_h += 0.12 * fbm_raster(self.grid, 3.0, seed=14)
        talus = 0.9 * np.exp(-np.maximum(sd - width * 0.5, 0.0) / 2.2)
        foot = h + talus
        self.plateau = dict(poly=poly, sd=sd.astype(np.float32), rise=rise.astype(np.float32), top=top, width=width)
        return foot + (top_h - foot) * rise

    def _pads(self, x, y, h):
        self.pad_weight = np.zeros(h.shape, dtype=np.float32)
        wobble = 0.9 * fbm_raster(self.grid, 9.0, seed=15)
        micro = 0.05 * fbm_raster(self.grid, 5.0, seed=16)
        for f in self.layout['features']:
            if f['type'] != 'pad':
                continue
            cx, cy = np.asarray(f['center']) / 100.0
            sx, sy = np.asarray(f['size']) / 200.0
            yaw = math.radians(f.get('yaw', 0.0))
            lx = (x - cx) * math.cos(yaw) + (y - cy) * math.sin(yaw)
            ly = -(x - cx) * math.sin(yaw) + (y - cy) * math.cos(yaw)
            corner = min(sx, sy) * 0.35
            qx, qy = np.abs(lx) - (sx - corner), np.abs(ly) - (sy - corner)
            sd = np.hypot(np.maximum(qx, 0.0), np.maximum(qy, 0.0)) + np.minimum(np.maximum(qx, qy), 0.0) - corner
            blend = 3.0 if min(sx, sy) < 6.0 else 12.0
            w = 1.0 - smoothstep(-1.0, blend, sd + wobble)
            h = h + (f['height'] / 100.0 + micro - h) * w
            self.pad_weight = np.maximum(self.pad_weight, w)
        return h

    def _footprints(self, h):
        self.footprints = []
        for p in self.layout['placements']:
            if p['kind'] not in FOOTPRINTS:
                continue
            radius, blend = FOOTPRINTS[p['kind']]
            cx, cy = np.asarray(p['location']) / 100.0
            target = float(sample(h, cx, cy))
            i0, i1 = self.grid.index_range(cx - radius - blend, cx + radius + blend)
            j0, j1 = self.grid.index_range(cy - radius - blend, cy + radius + blend)
            x, y = self.grid.mesh(i0, i1, j0, j1)
            w = 1.0 - smoothstep(radius, radius + blend, np.hypot(x - cx, y - cy))
            h[i0:i1, j0:j1] += (target - h[i0:i1, j0:j1]) * w
            self.footprints.append(dict(id=p['id'], center=(cx, cy), radius=radius, blend=blend, z=target))
        return h

    def _roads(self, h):
        """Roads follow the ground, smoothed along their length, flat across and carved a little."""
        self.roads = []
        self.road_gap = np.full(h.shape, np.inf, dtype=np.float32)  # meters past the nearest road's edge
        for road in self.layout['roads']:
            style = ROAD_STYLE.get(road.get('kind', 'dirt'), ROAD_STYLE['dirt'])
            pts = catmull_rom(to_m(road['path']), step=0.5)
            s = arc_length(pts)
            ground = sample(h, pts[:, 0], pts[:, 1])
            z = gauss_smooth_1d(ground, 10.0) - style['carve']
            half = road['width'] / 200.0
            margin = half + style['shoulder'] + 0.5
            dist, along, _ = polyline_field(self.grid, pts, margin)
            near = np.isfinite(dist)
            wobble = 0.25 * fbm(self.x[near], self.y[near], 3.0, seed=17)
            w = 1.0 - smoothstep(half - 0.2, half + style['shoulder'], dist[near] + wobble)
            target = np.interp(along[near], s, z)
            h[near] += (target - h[near]) * w
            self.road_gap[near] = np.minimum(self.road_gap[near], dist[near] - half)
            self.roads.append(dict(id=road['id'], kind=road.get('kind', 'dirt'), width=road['width'] / 100.0,
                                   pts=pts, s=s, z=z, shoulder=style['shoulder']))
        return h

    def _ramp(self, h):
        """The plateau's ramp: a path climbing at an even grade, on an embankment outside the cliff and in a cut
        through it (steep rock walls on both sides)."""
        f = self.features.get('plateau')
        self.ramp = None
        if f is None or 'ramp' not in f:
            return h
        ramp = f['ramp']
        pts = catmull_rom(to_m(ramp['path']), step=0.5)
        s = arc_length(pts)
        z0 = float(sample(h, *pts[0]))
        # The top end: the plateau's height a little past the end of the path.
        end_dir = (pts[-1] - pts[-4]) / max(np.linalg.norm(pts[-1] - pts[-4]), 1e-6)
        z1 = float(sample(h, *(pts[-1] + end_dir * 3.0)))
        t = s / s[-1]
        z = z0 + (z1 - z0) * (0.5 * t + 0.5 * t * t * (3.0 - 2.0 * t))
        half = ramp['width'] / 200.0
        dist, along, _ = polyline_field(self.grid, pts, 26.0)
        near = np.isfinite(dist)
        d = dist[near]
        zr = np.interp(along[near], s, z)
        e = np.maximum(d - half, 0.0)
        rough = 0.35 * fbm(self.x[near], self.y[near], 2.5, seed=18)
        hn = h[near]
        hn = smin(hn, zr + 2.0 * e + np.maximum(rough, 0.0) * np.minimum(e, 1.0), 0.35)  # the cut's walls
        hn = smax(hn, zr - 0.62 * e, 0.5)                                                 # the embankment's sides
        w = 1.0 - smoothstep(half - 0.3, half + 0.4, d)
        hn = hn + (zr - hn) * w
        h[near] = hn
        self.ramp = dict(pts=pts, s=s, z=z, width=ramp['width'] / 100.0)
        return h

    def _pond(self, x, y, h):
        f = self.features.get('pond')
        self.pond = None
        if f is None:
            return h
        cx, cy = np.asarray(f['center']) / 100.0
        rx, ry = np.asarray(f['radii']) / 100.0
        level = f['waterLevel'] / 100.0
        bottom = -f['depth'] / 100.0
        dx, dy = x - cx, y - cy
        rho = np.hypot(dx / rx, dy / ry) * (1.0 + 0.07 * fbm_raster(self.grid, 7.0, seed=21))
        phi = np.arctan2(dy, dx)
        radius = 1.0 / np.sqrt((np.cos(phi) / rx) ** 2 + (np.sin(phi) / ry) ** 2)
        beyond = (rho - 1.0) * radius  # meters past the shoreline (negative in the water)
        depth_curve = smoothstep(0.0, 0.6, 1.0 - rho)
        # The shore: a muddy shelf in some places, a steeper bank in others, always rising fast far out (so the
        # basin never cuts the meadow beyond it).
        slope = POND_SHORE_SLOPE + 0.14 * (0.5 + 0.5 * fbm_raster(self.grid, 14.0, seed=22, octaves=1))
        out = np.maximum(beyond, 0.0)
        basin = np.where(beyond > 0.0, level + slope * out + 0.08 * np.maximum(out - 4.0, 0.0) ** 2,
                         level - (level - bottom) * depth_curve)
        h = smin(h, basin, 0.3)
        # Where the meadow around the pond lies low, it's lifted gently (fading out over 12 m), so the shore always
        # stands above the water.
        lift = np.maximum(level + 0.35 - h, 0.0) * smoothstep(0.0, 1.5, beyond) * (1.0 - smoothstep(2.0, 12.0, beyond))
        h = h + lift
        self.pond = dict(center=(cx, cy), radii=(rx, ry), level=level, bottom=bottom,
                         rho=rho.astype(np.float32), beyond=beyond.astype(np.float32))
        return h

    def _creek_valley(self, x, y, h):
        """The creek's course (where it leaves the pond, its water level, its lip at the rim) and its valley: higher
        ground eases down toward the banks over about 12 m, so the creek runs in a fold of the meadow rather than a
        canyon."""
        f = self.features.get('creek')
        self.creek = None
        if f is None:
            return h
        pts = catmull_rom(to_m(f['path']), step=0.5)
        s = arc_length(pts)
        level = self.pond['level'] if self.pond else 0.0
        # Where the creek leaves the pond and where it runs off the island (its lip).
        in_pond = np.zeros(len(pts), dtype=bool)
        if self.pond:
            (cx, cy), (rx, ry) = self.pond['center'], self.pond['radii']
            in_pond = np.hypot((pts[:, 0] - cx) / rx, (pts[:, 1] - cy) / ry) < 1.0
        s_exit = float(s[np.argmax(~in_pond)]) if in_pond.any() else 0.0
        outside = ~points_in_polygon(pts[:, 0], pts[:, 1], self.outline)
        k_lip = int(np.argmax(outside)) if (f.get('waterfallAtEnd') and outside.any()) else len(pts) - 1
        s_lip = float(s[k_lip])
        run = np.maximum(s - s_exit, 0.0)
        water = level - 0.011 * run - 0.035 * np.maximum(s - (s_lip - 9.0), 0.0)
        dist, along, side = polyline_field(self.grid, pts, 16.0)
        near = np.isfinite(dist)
        d = dist[near] + 0.25 * fbm(x[near], y[near], 3.0, seed=31)
        bank = np.interp(along[near], s, water) + 0.8
        valley = (1.0 - smoothstep(2.5, 14.0, d)) * 0.85
        hn = h[near]
        h[near] = np.where(hn > bank, hn - (hn - bank) * valley, hn)
        self.creek = dict(pts=pts, s=s, water=water, s_exit=s_exit, s_lip=s_lip, k_lip=k_lip,
                          width=f['width'] / 100.0, dist=dist, along=along, side=side, wander=d - dist[near])
        return h

    def _creek(self, x, y, h):
        """The creek's channel, cut after the roads (it runs under the bridge): a rounded bed, banks at about 45
        degrees up to the bank level, gentler above."""
        c = self.creek
        if c is None:
            return h
        near = np.isfinite(c['dist'])
        d = c['dist'][near] + c.pop('wander')
        w = np.interp(c['along'][near], c['s'], c['water'])
        half = CREEK_WATER_HALF
        channel = np.where(d <= half, w - CREEK_DEPTH + CREEK_DEPTH * (d / half) ** 2,
                           np.where(d <= half + 0.8, w + 1.0 * (d - half), w + 0.8 + 0.7 * (d - half - 0.8)))
        hn = smin(h[near], channel, 0.25)
        # Banks stay above the water (except in the pond): low ground beside the creek is lifted gently.
        lift = np.maximum(w + 0.3 - hn, 0.0) * smoothstep(half + 0.2, half + 0.9, d) * (1.0 - smoothstep(2.5, 9.0, d))
        h[near] = hn + lift * (c['along'][near] > c['s_exit'] + 1.0)
        return h

    def _rim(self, x, y, h):
        """The meadow rolls over the edge; the rock wall below belongs to the underside."""
        e = self.edge
        roll = np.clip(1.0 - e / RIM_ROLL, 0.0, 1.0)
        amount = RIM_ROLL_DROP * (1.0 + 0.35 * fbm_raster(self.grid, 4.0, seed=51))
        if self.creek is not None:
            amount = amount * smoothstep(2.5, 6.0, self.creek['dist'])  # the creek keeps its banks to the lip
        if self.plateau is not None:
            amount = amount * (1.0 - 0.5 * self.plateau['rise'])       # rock doesn't roll as far as turf
        return h - amount * roll * roll

    # --- Queries ---

    def height(self, x, y):
        """Terrain height (m) at layout-meter points."""
        return sample(self.h, x, y)

    def water_surface(self):
        """The water surface height (m) per raster cell, NaN where there's no water: the pond at its level, the
        creek along its course down to the lip."""
        w = np.full(self.h.shape, np.nan, dtype=np.float32)
        if self.pond:
            wet = self.pond['rho'] < 1.35
            w[wet] = self.pond['level']
        if self.creek:
            c = self.creek
            reach = np.isfinite(c['dist']) & (c['dist'] < 3.2) & (c['along'] <= c['s_lip'] + 1.0)
            w[reach] = np.interp(c['along'][reach], c['s'], c['water'])
        return w
