"""An area's terrain shape, computed from its layout.json (numpy only; see area_math.py for the conventions).

Area(path).build() fills one height raster over the area's map square (the layout's "map": half the square's side and
the cell size of each raster, 10 cm for the heights on the tutorial island) in this order, each step working on the
result of the one before: base noise and hills, ridges and scarps, plateaus and mesas and their cliffs, pits, pads,
building footprints, knobs, ponds, creek valleys, roads, ramps, flights of steps, creek channels, dry gullies, the
gorges below falls, easing unplanned banks, and last the rim rolling over the island's edge (or, grounded, the rock past
the playable boundary's closed edges (area_boundary.py) and the seam band). It keeps what the macro painter and the mesh
builder need: the heights, the water surface, the outline, and the curves of the roads, creeks, gullies and ramps.

A map may grow (map.grownFrom: the half side, cm, of the square it grew from, on the same shape cells): the noise
rasters and the core's outline along the escarpment's lip are then worked out as the old square had them, so the
ground inside the old square comes out as it was (Tools/terrain_compare.py proves it). map.caps raises a raster's
largest size; map.macroSplit cuts the macro map into windows, each its own texture (macro_windows()).

Features are lists by type (FEATURE_TYPES), in layout order, each with its id: area.plateaus (mesas too), area.pits,
area.ramps, area.ponds, area.creeks, area.spines (ridge features), area.scarps, area.knobs, area.gullies,
area.gorges; hills and pads only change the heights. A feature with cliffs names its cliff group in
layout_computed.json (its cliffGroup, else its id; a ramp's walls are its own group). area_features.py has the types
that came with step 3b (ridges, scarps, pits, mesas, knobs, gullies, falls into gorges).

The setting is layout.json's "setting": "island" (the default) is the floating island with a rim, an outline and an
underside; everything that assumes the rim is marked "island setting" here and in the other area_*.py modules, and the
island keeps that code path exactly. "grounded" (area_region.py) sets the core square in a regional height field: no
rim or underside, the core's edge is the square and the escarpment's lip, a playable boundary with open edges, and a
seam band where the core fades into the surround ring.
"""
import hashlib
import json
import math
import os

import numpy as np

import area_boundary
from area_features import (GORGE_STREAM, GORGE_WALL, PAD_BANK_REACH, FeatureSteps, ramp_drop_sides,
                           seam_conflicts)
from area_math import (Grid, Window, arc_length, blur, catmull_rom, cells, fbm, fbm_raster, gauss_smooth_1d,
                       normals_of, points_in_polygon, polygon_area, polyline_field, raster_size, resample, resize,
                       sample, signed_distance, smax, smin, smoothstep)
from area_region import SEAM_STEP, Region, boundary, seam_weight

LEVELS = os.path.dirname(os.path.abspath(__file__))
FEATURE_TYPES = ('hill', 'plateau', 'pad', 'pond', 'creek', 'mesa', 'pit', 'ridge', 'scarp', 'knob', 'gully',
                 'steps')
SETTINGS = ('island', 'grounded')
# The rasters, by the name layout.json's map.cells uses: the cell size (cm) when the layout gives none, the largest
# size (cells across), and whether it's a texture (a power of two, so Unreal can mip and stream it). A 400 m square
# keeps 12.5 cm heights; the macro map stops at 4096 px (about 10 cm per pixel there).
RASTERS = {
    'shape': (10.0, 4096, False),      # the heights and every field built with them
    'curvature': (20.0, 2048, False),  # the top's vertex density, and the macro map's convexity and wetness
    'ao': (20.0, 2048, False),         # the top's baked ambient occlusion
    'scatter': (40.0, 1024, True),     # the PCG scatter mask
    'macro': (5.0, 4096, True),        # the macro color map
}

# Flat footprints under buildings (radius, blend) in meters, by placement kind. The ground is flattened to its own
# height at the building's location, so a building on a slope or at a pad's edge doesn't float or sink.
FOOTPRINTS = {
    'Farmhouse': (7.0, 4.0), 'Barn': (8.0, 4.0), 'LogCabin': (5.5, 3.5), 'Cottage': (5.5, 3.5),
    'Outhouse': (2.0, 3.0), 'Well': (2.5, 2.5), 'Windmill': (3.0, 4.5), 'LookoutTower': (4.0, 3.0),
    'GunRack': (2.0, 2.0),
    # Ransom's Rest's buildings (Docs/Areas/RansomsRest.md).
    'Lookout': (4.0, 3.0), 'Chapel': (8.5, 4.0), 'WaterTower': (3.0, 2.0), 'CoffinShed': (2.5, 2.0),
    'FalseFront_Undertaker': (6.5, 3.0), 'FalseFront_Saloon': (7.0, 3.0), 'FalseFront_Store': (7.0, 3.0),
    'FalseFront_Sheriff': (5.5, 3.0),
    # Ransom's Rest's mountain (Docs/Areas/RansomsRest_Expansion.md, section 6): each stands on a level pad of the
    # layout's (the quarry's floor, Calder's Post's yard, the Diggings), wider than this, so placing it there changes
    # nothing. The lift's tower and the mine's portal stand on a slope and in a band: none. Nor the lift head: its
    # circle levelled the end of the lift's cutting back up to the shoulder, and the cage ran 2 m into it (2026-10-30).
    'RR_LiftHouse': (5.5, 3.0), 'RR_Watchtower': (2.8, 2.0), 'RR_RangerCabin': (4.5, 2.5),
    'RR_LeanTo': (3.0, 2.0), 'RR_OreBin': (4.5, 2.5), 'RR_PowderHouse': (3.0, 2.0), 'RR_Smithy': (4.5, 2.5),
    # Ransom's Rest's own homes and fronts (section 9), centred on their pivots (the front doors), each about two thirds
    # of the way to its farthest back corner. The hotel, the workshop and Amos's barn stand on pads instead; the dugout
    # levels only its doorstep (its mound is dug into the slope).
    'RR_Home_A': (6.0, 3.0), 'RR_Home_B': (7.0, 3.0), 'RR_Home_C': (5.0, 3.0), 'RR_Home_D': (6.0, 3.0),
    'RR_Home_E': (2.5, 2.0), 'RR_Home_F': (6.0, 3.0), 'RR_Home_G': (5.5, 3.0), 'RR_Home_H': (6.5, 3.0),
    'RR_Home_I': (6.5, 3.0), 'RR_Home_J': (4.0, 2.5), 'RR_Front_Assay': (8.0, 3.0), 'RR_Front_Barber': (8.0, 3.0),
    'RR_CoalStage': (3.5, 2.0), 'RR_Well': (2.5, 2.5), 'RR_Outhouse': (2.0, 3.0),
}
# Buildings' sizes (length along the front, width) in meters: the worn ground around and in front of each (the macro
# map) and the breaks they make in open ground (area_open.py).
BUILDINGS = {
    'Farmhouse': (9.0, 11.0), 'Barn': (12.0, 9.0), 'LogCabin': (6.5, 8.0), 'Cottage': (6.5, 8.0),
    'Outhouse': (1.3, 1.3), 'Well': (1.8, 1.8), 'Windmill': (3.0, 3.0), 'LookoutTower': (4.0, 4.0),
    'GunRack': (0.6, 2.0),
    'Lookout': (6.6, 6.6), 'Chapel': (14.4, 9.6), 'Depot': (8.0, 14.0), 'WaterTower': (4.0, 4.0),
    'CoffinShed': (2.4, 3.2), 'FalseFront_Undertaker': (12.0, 7.0), 'FalseFront_Saloon': (14.0, 8.4),
    'FalseFront_Store': (13.0, 9.4), 'FalseFront_Sheriff': (10.0, 6.6),
    'Locomotive_B': (8.8, 2.8), 'PassengerCar': (12.3, 2.9), 'HearseCar': (10.3, 2.9),
    'RR_LiftHouse': (8.0, 10.0), 'RR_LiftTower': (3.9, 3.4), 'RR_LiftHead': (4.0, 5.7), 'RR_Watchtower': (4.0, 4.0),
    'RR_RangerCabin': (6.0, 8.0), 'RR_LeanTo': (4.0, 6.0), 'RR_MinePortal': (3.0, 7.0), 'RR_OreBin': (6.0, 8.0),
    'RR_PowderHouse': (4.0, 5.0), 'RR_Smithy': (6.0, 8.0),
    # The Rest's own (their pivots are their front doors, so the trampled ground lies in front of the door).
    'RR_Home_A': (6.4, 7.2), 'RR_Home_B': (7.1, 11.4), 'RR_Home_C': (11.4, 3.9), 'RR_Home_D': (7.4, 8.0),
    'RR_Home_E': (8.6, 12.0), 'RR_Home_F': (5.8, 8.8), 'RR_Home_G': (4.4, 7.3), 'RR_Home_H': (8.8, 8.8),
    'RR_Home_I': (7.8, 8.0), 'RR_Home_J': (4.9, 4.2), 'RR_Front_Assay': (9.6, 7.2), 'RR_Front_Hotel': (15.7, 10.3),
    'RR_Front_Barber': (9.3, 6.1), 'RR_Workshop_Bright': (8.2, 12.5), 'RR_Barn_Whitlock': (9.5, 15.8),
    'RR_CoalStage': (5.4, 5.1), 'RR_Well': (1.8, 1.8), 'RR_Outhouse': (1.4, 1.5),
}
ROAD_STYLE = {  # how deep a road is carved and how wide its soft shoulders are (m), by kind
    'dirt': dict(carve=0.12, shoulder=2.2),
    'path': dict(carve=0.07, shoulder=2.0),
    'rail': dict(carve=0.0, shoulder=1.5),   # a rail bed: graded to its "level" (cm), not carved
}
RIM_ROLL = 3.2          # the meadow rolls over the island's edge within this distance of the rim...
RIM_ROLL_DROP = 1.2     # ...dropping this far before the rock wall (the underside's rim) starts
CREEK_DEPTH = 0.45      # water depth in the middle of the creek
CREEK_WATER_HALF = 1.2  # half the creek's water width
POND_SHORE_SLOPE = 0.16


def to_m(points):
    """Layout centimeters [[X, Y], ...] to layout meters (N, 2)."""
    return np.asarray(points, dtype=np.float64) / 100.0


def _inner_run(loop, half):
    """A closed loop's one run of points strictly inside the square of half side half (the lip's stretch of an outline
    whose other points lie on the square's edge), in loop order; None when there's no single such run."""
    inside = np.max(np.abs(loop), axis=1) < half - 1e-6
    if not inside.any() or inside.all():
        return None
    start = int(np.nonzero(~inside)[0][-1]) + 1
    loop, inside = np.roll(loop, -start, axis=0), np.roll(inside, -start)
    k0 = int(np.argmax(inside))
    k1 = k0 + int(np.argmax(~inside[k0:])) if (~inside[k0:]).any() else len(loop)
    if inside[k1:].any():
        return None
    return loop[k0:k1]


def layout_path(name):
    """An area's layout: Art/Levels/<name>/layout.json."""
    return os.path.join(LEVELS, name, 'layout.json')


def load_layout(path):
    with open(path, encoding='utf-8') as file:
        return json.load(file)


# The blocks of layout.json the generator never reads: how Unreal builds the level (its map, lighting, sky islands,
# scatter graph) and its gameplay (creature groups and the like).
BUILD_ONLY = ('level', 'gameplay')


def layout_sha1(path):
    """What layout_computed.json's layoutSha1 records, and terrain_check.py compares: the SHA-1 of the layout the
    generator reads, parsed, without its BUILD_ONLY blocks and written as canonical JSON (sorted keys, no spaces).
    Editing those blocks, or only the file's formatting, then never makes a computed layout look stale."""
    read = {key: value for key, value in load_layout(path).items() if key not in BUILD_ONLY}
    text = json.dumps(read, sort_keys=True, separators=(',', ':'), ensure_ascii=False)
    return hashlib.sha1(text.encode('utf-8')).hexdigest()


class Area(FeatureSteps):
    def __init__(self, path, layout=None):
        self.path = os.path.abspath(path)
        self.folder = os.path.dirname(self.path)
        self.layout = layout or load_layout(self.path)
        self.name = self.layout.get('name') or os.path.basename(self.folder)
        self.setting = self.layout.get('setting', 'island')
        if self.setting not in SETTINGS:
            raise ValueError(f"{self.path}: setting {self.setting!r} isn't one of {', '.join(SETTINGS)}")
        self.island = self.layout.get('island', {})
        self._map_square()
        self.grid = Grid(self.sizes['shape'], self.half)
        if self.grown_from is not None:
            # The noise rasters take their points from the square the map grew from (area_math._fbm_on_lattice).
            self.grid.lattice = (self.grown_from['half'], self.grown_from['shape'])
        unknown = [f['id'] for f in self.layout['features'] if f['type'] not in FEATURE_TYPES]
        if unknown:
            raise ValueError(f"{self.path}: features of no known type ({', '.join(FEATURE_TYPES)}): "
                             f"{', '.join(unknown)}")
        self.by_type = {t: [f for f in self.layout['features'] if f['type'] == t] for t in FEATURE_TYPES}
        # The regional field's geometry (its rasters come with build()); the island setting has none.
        self.region = Region(self) if self.setting == 'grounded' else None
        self.boundary = self.open_edges = None

    def _map_square(self):
        """The map square and each raster's size, from the layout's map (in cm: the sizes come out exact, as 20480 cm
        over 10 cm cells is 2048)."""
        square = self.layout.get('map')
        if not square or 'half' not in square:
            raise ValueError(f'{self.path}: no map.half (half the side of the square the terrain covers, in cm)')
        self.half_cm = square['half']
        self.half = self.half_cm / 100.0
        wanted = square.get('cells', {})
        # A layout may raise a raster's cap ("caps": {raster: cells}): a grown square keeps its 20 cm curvature and
        # occlusion on more cells than the default 2048.
        caps = square.get('caps', {})
        unknown = sorted(set(caps) - set(RASTERS))
        if unknown:
            raise ValueError(f"{self.path}: map.caps names no raster: {', '.join(unknown)}")
        self.sizes = {}
        for raster, (cell, cap, texture) in RASTERS.items():
            self.sizes[raster] = raster_size(2.0 * self.half_cm, float(wanted.get(raster, cell)),
                                             int(caps.get(raster, cap)), texture)
        self.grown_from = self._grown_from(square, wanted)

    def _grown_from(self, square, wanted):
        """A grown map square (layout.json map.grownFrom: the half side, cm, of the square it grew from, the same cells
        on the same grid): that square's half side (m) and shape raster size, which the noise rasters and the core's
        outline along the escarpment's lip are worked out on, so everything inside the old square comes out as it did.
        None for a map that never grew. The heights' cells must line up exactly."""
        if 'grownFrom' not in square:
            return None
        ref_cm = float(square['grownFrom'])
        cell = float(wanted.get('shape', RASTERS['shape'][0]))
        ref_n = raster_size(2.0 * ref_cm, cell, RASTERS['shape'][1], RASTERS['shape'][2])
        px, ref_px = 2.0 * self.half_cm / self.sizes['shape'], 2.0 * ref_cm / ref_n
        offset = (self.half_cm - ref_cm) / px
        if ref_cm >= self.half_cm or abs(px - ref_px) > 1e-9 or abs(offset - round(offset)) > 1e-9:
            raise ValueError(f'{self.path}: map.grownFrom {ref_cm:g} cm must be a smaller square on the same shape '
                             f'cells ({ref_px:g} cm there against {px:g} cm here, {offset:g} cells apart)')
        return {'half': ref_cm / 100.0, 'shape': ref_n}

    def cell(self, raster):
        """The size (m) of one cell of a raster over the map square."""
        return 2.0 * self.half / self.sizes[raster]

    # --- Paths and names ---

    @property
    def computed_path(self):
        return os.path.join(self.folder, 'layout_computed.json')

    @property
    def macro_texture(self):
        """The macro color map, relative to the repository (Art/Textures/<Area>Macro, its texture set)."""
        return f'Art/Textures/{self.name}Macro/T_{self.name}Macro_BC.png'

    def macro_texture_of(self, suffix):
        """A macro map's texture, relative to the repository: the whole map's (suffix '') or a split map's window's
        (its own texture set, <Area>Macro<suffix>)."""
        name = f'{self.name}Macro{suffix}'
        return f'Art/Textures/{name}/T_{name}_BC.png'

    def macro_windows(self):
        """The macro map's pieces, [(suffix, grid)]: the map square at the macro size ('', a Grid), unless the layout
        splits it (map.macroSplit: s cuts it into s x s windows of the macro size / s cells across, area_math.Window,
        each its own texture set for the tiles over it; s = 2 names them SW, SE, NW, NE). A split map keeps fine
        pixels over a large square at the size one texture may have."""
        split = int(self.layout.get('map', {}).get('macroSplit', 1))
        n = self.sizes['macro']
        if split <= 1:
            return [('', Grid(n, self.half))]
        if n % split:
            raise ValueError(f'{self.path}: map.macroSplit {split} must divide the macro map\'s {n} cells')
        m, px = n // split, 2.0 * self.half / n
        names = {(0, 0): 'SW', (0, 1): 'SE', (1, 0): 'NW', (1, 1): 'NE'} if split == 2 else {}
        return [(names.get((i, j), f'{i}{j}'),
                 Window(m, px, -self.half + i * m * px, -self.half + j * m * px, (self.half, n)))
                for i in range(split) for j in range(split)]

    @property
    def scatter_texture(self):
        return f'Art/Textures/{self.name}Macro/T_{self.name}Scatter_BC.png'

    @property
    def scrub_texture(self):
        """A grounded area's scrub mask (area_scrub.py), beside the scatter mask."""
        return f'Art/Textures/{self.name}Macro/T_{self.name}Scrub_BC.png'

    @property
    def ring_macro_texture(self):
        """A grounded area's surround ring's macro color map (its own texture set, Art/Textures/<Area>RingMacro)."""
        return f'Art/Textures/{self.name}RingMacro/T_{self.name}RingMacro_BC.png'

    def zones_of(self, kind):
        """The zones of a kind (a zone's kind, else its id): forest, orchard, range, village, ..."""
        return [z for z in self.layout['zones'] if z.get('kind', z['id']) == kind]

    def placement(self, placement_id):
        return next((p for p in self.layout['placements'] if p['id'] == placement_id), None)

    def point(self, ref):
        """A point in layout meters: a placement's id, or [X, Y] in layout centimeters."""
        if isinstance(ref, str):
            return np.asarray(self.placement(ref)['location'][:2], dtype=np.float64) / 100.0
        return np.asarray(ref, dtype=np.float64) / 100.0

    def yard_center(self, yard):
        """Where a yard (layout.json "yards": a farmyard, a village square; the painters keep it bare) is: its
        center, or the middle of the placements it's around (those that exist). None when none does."""
        if 'center' in yard:
            return self.point(yard['center'])
        found = [self.point(k) for k in yard.get('around', []) if self.placement(k) is not None]
        return np.mean(found, axis=0) if found else None

    # --- The steps ---

    def build(self, log=print):
        grid = self.grid
        x, y = grid.mesh()
        self.x, self.y = x, y
        # Steep ground the features design (cliffs, walls): _relax() leaves it alone.
        self.designed = np.zeros((grid.n, grid.n), dtype=bool)
        if self.setting == 'grounded':
            self._grounded(log)
        else:
            self._outline()
        log('terrain: meadow and hills')
        h = self._meadow(x, y)
        self.h_natural = h.astype(np.float32)
        h = self._spines(h)
        h = self._scarps(h)
        log('terrain: plateaus')
        h = self._plateaus(h)
        h = self._pits(h)
        log('terrain: pads and footprints')
        h = self._pads(x, y, h)
        h = self._footprints(h)
        h = self._knobs(h)
        # The ponds and the creeks' valleys come before the roads (a road passing a pond keeps its shoulders, and
        # the forest road follows the valley down to the bridge); the creeks' channels after them (they run under
        # bridges), and the dry gullies and gorges with them.
        log('terrain: ponds, roads, ramps and creeks')
        h = self._ponds(x, y, h)
        h = self._creek_valleys(x, y, h)
        h = self._roads(h)
        h = self._ramps(h)
        h = self._steps(h)
        h = self._creeks(h)
        h = self._gullies(h)
        h = self._gorges(h)
        h = self._relax(h)
        if self.setting == 'grounded':
            # The rock past the playable boundary's closed edges (boundary.foot), the last of the shaping.
            h = area_boundary.foot(self, h)
            log('terrain: seam band')
            h = self._seam(h)
        else:
            log('terrain: rim')
            h = self._rim(h)  # island setting
        self.h = h.astype(np.float32)
        del self.x, self.y, self.designed
        return self

    def _grounded(self, log):
        """The grounded setting's frame: the regional field, the core's edge (the square on the upland side of the
        lip) and the playable boundary. Stops on any feature in the seam band."""
        conflicts = seam_conflicts(self)
        if conflicts:
            raise ValueError(f'{self.path}: features in the seam band:\n  ' + '\n  '.join(conflicts))
        self.region.build_fields(log)
        self.core_polygon, self.core_on_lip = self.region.core_outline()
        self.outline = resample(self.core_polygon, 0.5, closed=True)
        if self.grown_from is not None and self.core_on_lip.any():
            self.outline = self._grown_outline()
        self.edge = -signed_distance(self.grid, self.outline, 14.0)  # meters inside the core's edge
        self.inside = self.edge > 0.0
        self.boundary, self.open_edges = boundary(self)
        if self.boundary is not None:
            self.play_edge = -signed_distance(self.grid, self.boundary, 30.0)  # meters inside the playable boundary
        else:
            self.play_edge = self.edge
        self.h_region = self.region.raster(self.grid)
        self.dl = self.region.lip_distance(self.grid) if self.region.lip is not None else None

    def _grown_outline(self):
        """The core's outline on a grown map square (map.grownFrom): where it runs along the escarpment's lip inside the
        old square, the very points the old square's outline had there (its 0.5 m resampling started where the lip
        crossed the old square's edge, so the new square's would fall elsewhere along the lip and move the distance
        inside the edge, area.edge, by a centimeter or two); the rest as the new square's own."""
        ref = self.grown_from['half']
        ref_poly, _ = self.region.core_outline(ref)
        old = _inner_run(resample(ref_poly, 0.5, closed=True), ref)
        loop = self.outline
        inside = np.max(np.abs(loop), axis=1) < ref - 1e-6
        if old is None or not inside.any() or inside.all():
            return loop
        # Turn the loop so the run inside the old square doesn't wrap past its start, then splice the old points in.
        start = int(np.nonzero(~inside)[0][-1]) + 1
        loop, inside = np.roll(loop, -start, axis=0), np.roll(inside, -start)
        k0 = int(np.argmax(inside))
        k1 = k0 + int(np.argmax(~inside[k0:])) if (~inside[k0:]).any() else len(loop)
        if np.linalg.norm(old[0] - loop[k0]) > np.linalg.norm(old[-1] - loop[k0]):
            old = old[::-1]
        return np.vstack([loop[:k0], old, loop[k1:]])

    def _seam(self, h):
        """The seam band: the core's own shape fades into the regional field toward the square's edge, where the core
        is the field exactly."""
        return self.h_region + (h - self.h_region) * seam_weight(self, self.x, self.y)

    def mesh_boundary(self, step=0.7):
        """The top mesh's edge loop (N, 2) and what each point is (None on an island: the rim all round; grounded: 0
        on the lip, 1 on the square's edge, the seam, and 2 where the lip meets the square). The lip's points are step
        meters apart and the seam's SEAM_STEP, with the square's corners kept."""
        if self.setting != 'grounded':
            return resample(self.outline, step, closed=True), None
        poly, on_lip = self.core_polygon, self.core_on_lip

        def edges(run):
            """A run of the polygon resampled edge by edge (corners kept), at most SEAM_STEP apart."""
            out = [run[:1]]
            for a, b in zip(run[:-1], run[1:]):
                count = max(1, int(math.ceil(np.linalg.norm(b - a) / SEAM_STEP - 1e-9)))
                t = np.linspace(0.0, 1.0, count + 1)[1:, None]
                out.append(a + (b - a) * t)
            return np.vstack(out)
        if not on_lip.any():
            loop = edges(np.vstack([poly, poly[:1]]))[:-1]
            return loop, np.ones(len(loop), dtype=np.int8)
        k = int(next(i for i in range(len(poly)) if on_lip[i] and not on_lip[i - 1]))
        poly, on_lip = np.roll(poly, -k, axis=0), np.roll(on_lip, -k)
        m = int(on_lip.sum())
        lip = resample(poly[:m], step)
        square = edges(np.vstack([poly[m - 1:], poly[:1]]))
        loop = np.vstack([lip, square[1:-1]])
        kinds = np.concatenate([np.zeros(len(lip), np.int8), np.ones(len(square) - 2, np.int8)])
        kinds[0] = kinds[len(lip) - 1] = 2
        return loop, kinds

    def _relax(self, h, limit=30.0, passes=4):
        """Eases banks steeper than about limit degrees that no feature asked for (where a pad, a footprint and a
        path's cut meet on a hillside) toward a blurred copy of the ground. Designed steep ground is left alone:
        the plateaus' cliffs, the ramps' walls, the creeks' channels, road surfaces, the features' cliffs and walls,
        and the rim (or, grounded, everything outside the playable boundary)."""
        keep = self.designed.copy()
        for p in self.plateaus:
            keep |= (p['rise'] > 0.01) & (p['rise'] < 0.995)
            keep |= p['sd'] < p['width'] * 0.5 + 3.0
        for r in self.ramps:
            dist, _, _ = polyline_field(self.grid, r['pts'], 12.0)
            keep |= np.isfinite(dist)
        for c in self.creeks:
            keep |= np.isfinite(c['dist']) & (c['dist'] < 3.5)
        keep |= self.road_gap < 0.3
        if self.setting == 'grounded':
            keep |= self.play_edge < 0.5  # the ridges' bands and slopes past the boundary
            if self.dl is not None:
                keep |= self.dl < 1.5     # the escarpment's lip and drop
        else:
            keep |= self.edge < RIM_ROLL + 1.0  # island setting
        px = self.grid.px
        free = 1.0 - blur(keep.astype(np.float32), cells(0.5, px))  # fades in over half a meter from protected ground
        for _ in range(passes):
            gx, gy = np.gradient(h, px)
            slope = np.degrees(np.arctan(np.hypot(gx, gy)))
            w = smoothstep(limit - 4.0, limit + 6.0, blur(slope.astype(np.float32), cells(0.4, px))) * free
            h = h + (blur(h, cells(0.8, px)) - h) * w
        return h

    def _outline(self):
        """The island's edge (island setting): the layout's outline as a smooth curve, pushed in and out a little by
        noise."""
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
        self.play_edge = self.edge  # the rim is the island's playable edge

    def _meadow(self, x, y):
        if self.setting == 'grounded':
            return self._valley(x, y)
        base, micro = self.island['baseNoise'], self.island['microNoise']
        # Each noise is the layout's amplitude at its wavelength (the base with a faint second octave).
        h = base['amplitude'] / 100.0 * fbm_raster(self.grid, base['wavelength'] / 100.0, seed=1, octaves=2, gain=0.35)
        h += micro['amplitude'] / 100.0 * fbm_raster(self.grid, micro['wavelength'] / 100.0, seed=2, octaves=1)
        return self._hills(x, y, h)

    def _hills(self, x, y, h):
        # Hills, their outlines bent by a warp so none is a perfect dome. The warp fades out toward the top, so the
        # summit stays at the hill's center (the windmill stands there).
        wx = 4.0 * fbm_raster(self.grid, 24.0, seed=7, octaves=1)
        wy = 4.0 * fbm_raster(self.grid, 24.0, seed=8, octaves=1)
        for f in self.by_type['hill']:
            cx, cy = np.asarray(f['center']) / 100.0
            radius = f['radius'] / 100.0
            fade = smoothstep(0.1, 0.55, np.hypot(x - cx, y - cy) / radius)
            r = np.hypot(x + wx * fade - cx, y + wy * fade - cy) / radius
            h += f['height'] / 100.0 * (0.5 + 0.5 * np.cos(np.pi * np.minimum(r, 1.0)))
        return h

    def _valley(self, x, y):
        """The grounded core's ground: the regional field, with the core's own small noise and its hills on the
        upland (they fade out across the seam band at the end of build())."""
        micro = self.region.spec.get('microNoise', {'amplitude': 25, 'wavelength': 600})
        own = micro['amplitude'] / 100.0 * fbm_raster(self.grid, micro['wavelength'] / 100.0, seed=2, octaves=1)
        own = self._hills(x, y, own)
        if self.dl is not None:
            own = own * smoothstep(0.0, 2.0, self.dl)
        return self.h_region + own

    def _plateaus(self, h):
        """Each plateau and mesa: a top raised over the ground inside its polygon, with a cliff around it."""
        self.plateaus = []
        for f in self.by_type['plateau'] + self.by_type['mesa']:
            top = f['height'] / 100.0
            width = f.get('cliffWidth', 350) / 100.0
            poly = catmull_rom(to_m(f['polygon']), closed=True, step=0.5)
            sd = signed_distance(self.grid, poly, 14.0)
            # The cliff line wanders: large bays and spurs plus a jagged top edge.
            sd = (sd + 1.4 * fbm_raster(self.grid, 11.0, seed=11, octaves=2)
                  + 0.35 * fbm_raster(self.grid, 2.6, seed=12, octaves=1))
            t = np.clip((width * 0.5 - sd) / width, 0.0, 1.0)
            k = 4.2  # steep in the middle, a rounded lip at the top and a foot at the bottom
            rise = 0.5 + 0.5 * np.tanh(k * (t - 0.5)) / math.tanh(k * 0.5)
            top_h = top + 0.45 * fbm_raster(self.grid, 19.0, seed=13, octaves=2)
            top_h += 0.12 * fbm_raster(self.grid, 3.0, seed=14)
            talus = 0.9 * np.exp(-np.maximum(sd - width * 0.5, 0.0) / 2.2)
            foot = h + talus
            self.plateaus.append(dict(id=f['id'], feature=f, kind=f['type'], cliff_group=f.get('cliffGroup', f['id']),
                                      poly=poly, sd=sd.astype(np.float32), rise=rise.astype(np.float32), top=top,
                                      width=width))
            h = foot + (top_h - foot) * rise
        return h

    def _pads(self, x, y, h):
        """Each pad: a rounded rectangle ("center", "size", "yaw") made level at "height" (cm, absolute), blending
        into the ground around it over 3 m (a small pad) or 12 m. Optional: "corner", the corners' radius as a share of
        the shorter half side (0.35); "grade" [north, east], a gentle tilt (m of rise per m; a hillside graveyard); and
        "bank" {slope, round, toe, reach}, a graded edge instead of the blend (_pad_bank); "exact": its edge where the
        layout draws it and its top flat, without the wander and the micro relief every other pad has (a mine's portal
        is fitted to it)."""
        self.pad_weight = np.zeros(h.shape, dtype=np.float32)
        wobble = 0.9 * fbm_raster(self.grid, 9.0, seed=15)
        micro = 0.05 * fbm_raster(self.grid, 5.0, seed=16)
        for f in self.by_type['pad']:
            cx, cy = np.asarray(f['center']) / 100.0
            sx, sy = np.asarray(f['size']) / 200.0
            yaw = math.radians(f.get('yaw', 0.0))
            lx = (x - cx) * math.cos(yaw) + (y - cy) * math.sin(yaw)
            ly = -(x - cx) * math.sin(yaw) + (y - cy) * math.cos(yaw)
            corner = min(sx, sy) * f.get('corner', 0.35)
            qx, qy = np.abs(lx) - (sx - corner), np.abs(ly) - (sy - corner)
            sd = np.hypot(np.maximum(qx, 0.0), np.maximum(qy, 0.0)) + np.minimum(np.maximum(qx, qy), 0.0) - corner
            # "exact" takes the micro relief off too: a model's floor meets the pad's top to the centimeter.
            top = f['height'] / 100.0 + (0.0 if f.get('exact') else micro)
            if 'grade' in f:
                gx, gy = f['grade']
                top = top + gx * (x - cx) + gy * (y - cy)
            # "exact": the edge where the layout draws it, without the wander (a model fitted to it: a mine's portal).
            edge = sd if f.get('exact') else sd + wobble
            if 'bank' in f:
                h, w = self._pad_bank(f['bank'], h, edge, top)
            else:
                blend = 3.0 if min(sx, sy) < 6.0 else 12.0
                w = 1.0 - smoothstep(-1.0, blend, edge)
                h = h + (top - h) * w
            self.pad_weight = np.maximum(self.pad_weight, w)
        return h

    def _pad_bank(self, bank, h, d, top):
        """A pad's graded edge, the way a yard is levelled on a hillside: inside (d, meters past the pad's edge, <= 0)
        the ground is the pad's top exactly; outside, it's kept within a batter of bank "slope" degrees from the edge:
        filled up to the line falling away from the top, cut down to the line rising from it, and left alone where the
        ground already lies between (a bank never steeper than the slope, unless the ground was). The batter starts
        level and turns down over "round" cm (a rounded crest), and its foot meets the ground over "toe" cm (a smooth
        minimum, not a crease). It works only within "reach" cm of the edge (PAD_BANK_REACH at most), letting go over
        its last 3 m: the batter's lines run on forever, and unbounded they filled a canyon 70 m down a hundred meters
        off and cut a ridge's foot (Ransom's Rest's churchyard, 2026-10-12). The pad's flat is left out of _relax().
        Returns the heights and the pad's weight."""
        t = math.tan(math.radians(bank['slope']))
        rounding = max(bank.get('round', 200) / 100.0, 0.01)
        toe = bank.get('toe', 150) / 100.0
        limit = min(bank.get('reach', 1200) / 100.0, PAD_BANK_REACH)
        out = np.maximum(d, 0.0)
        reach = np.where(out < rounding, out * out / (2.0 * rounding), out - 0.5 * rounding)
        low, high = top - t * reach, top + t * reach
        # The toe's blend grows in over the crest, so the edge itself is exact.
        k = np.maximum(toe * smoothstep(0.0, rounding, d), 1e-4)
        graded = smin(smax(h, low, k), high, k)
        graded = h + (graded - h) * (1.0 - smoothstep(limit - min(3.0, 0.5 * limit), limit, d))
        flat = d <= 0.0
        self.designed |= flat
        return np.where(flat, top, graded), (1.0 - smoothstep(0.0, rounding, d)).astype(np.float32)

    def _footprints(self, h):
        self.footprints = []
        for p in self.layout['placements']:
            if p['kind'] not in FOOTPRINTS:
                continue
            radius, blend = FOOTPRINTS[p['kind']]
            cx, cy = np.asarray(p['location'][:2]) / 100.0
            target = float(sample(h, cx, cy, self.half))
            i0, i1 = self.grid.index_range(cx - radius - blend, cx + radius + blend)
            j0, j1 = self.grid.index_range(cy - radius - blend, cy + radius + blend)
            x, y = self.grid.mesh(i0, i1, j0, j1)
            w = 1.0 - smoothstep(radius, radius + blend, np.hypot(x - cx, y - cy))
            h[i0:i1, j0:j1] += (target - h[i0:i1, j0:j1]) * w
            self.footprints.append(dict(id=p['id'], center=(cx, cy), radius=radius, blend=blend, z=target))
        return h

    def _roads(self, h):
        """Roads follow the ground, smoothed along their length, flat across and carved a little. A road with a
        "level" (cm; a rail bed) is graded flat at that height instead."""
        self.roads = []
        self.road_gap = np.full(h.shape, np.inf, dtype=np.float32)  # meters past the nearest road's edge
        for road in self.layout['roads']:
            style = ROAD_STYLE.get(road.get('kind', 'dirt'), ROAD_STYLE['dirt'])
            pts = catmull_rom(to_m(road['path']), step=0.5)
            s = arc_length(pts)
            ground = sample(h, pts[:, 0], pts[:, 1], self.half)
            if 'level' in road:
                z = np.full(len(pts), road['level'] / 100.0 - style['carve'])
            else:
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

    def _ramps(self, h):
        """Each plateau's ramp, and each pit's: a path climbing (or descending) along its length, on an embankment
        where the ground is lower and in a cut where it's higher (steep rock walls on both sides). Its grade eases in
        and out; "grade": "even" keeps it constant but for 3 m at each end. "drop": "left", "right" or "both" makes
        that side's embankment a cliff (a ledge along a face: the bluff path, the Sink's ramp), as steep as a gorge's
        walls, dressed with the ramp's own cliff group."""
        self.ramps = []
        for p in self.plateaus + self.pits:
            ramp = p['feature'].get('ramp')
            if ramp is None:
                continue
            pts = catmull_rom(to_m(ramp['path']), step=0.5)
            s = arc_length(pts)
            z0 = float(sample(h, *pts[0], self.half))
            # The top end: the plateau's height a little past the end of the path (a pit's floor).
            end_dir = (pts[-1] - pts[-4]) / max(np.linalg.norm(pts[-1] - pts[-4]), 1e-6)
            z1 = float(sample(h, *(pts[-1] + end_dir * 3.0), self.half))
            t = s / s[-1]
            if ramp.get('grade') == 'even':
                ease = min(3.0, 0.2 * s[-1])
                weight = smoothstep(0.0, ease, s) * smoothstep(0.0, ease, s[-1] - s) + 1e-6
                climb = np.concatenate([[0.0], np.cumsum(0.5 * (weight[1:] + weight[:-1]) * np.diff(s))])
                z = z0 + (z1 - z0) * climb / climb[-1]
            else:
                z = z0 + (z1 - z0) * (0.5 * t + 0.5 * t * t * (3.0 - 2.0 * t))
            half = ramp['width'] / 200.0
            dist, along, side = polyline_field(self.grid, pts, 26.0)
            near = np.isfinite(dist)
            d = dist[near]
            zr = np.interp(along[near], s, z)
            e = np.maximum(d - half, 0.0)
            rough = 0.35 * fbm(self.x[near], self.y[near], 2.5, seed=18)
            hn = h[near]
            hn = smin(hn, zr + 2.0 * e + np.maximum(rough, 0.0) * np.minimum(e, 1.0), 0.35)  # the cut's walls
            drop = ramp_drop_sides(ramp)
            if drop:
                # A side that drops as a cliff: polyline_field's side is +1 on the right (as seen on the plan).
                steep = np.isin(side[near], drop)
                hn = smax(hn, zr - np.where(steep, GORGE_WALL, 0.62) * e, 0.5)
            else:
                hn = smax(hn, zr - 0.62 * e, 0.5)                                             # the embankment's sides
            w = 1.0 - smoothstep(half - 0.3, half + 0.4, d)
            hn = hn + (zr - hn) * w
            if self.setting == 'grounded':
                # The cut and the embankment stay near the ramp: their cones past its ends would otherwise reach a
                # ridge or the escarpment's drop.
                reach = 1.0 - smoothstep(half + 9.0, half + 12.0, d)
                if self.dl is not None:
                    reach = reach * smoothstep(0.0, 1.0, self.dl[near])
                hn = h[near] + (hn - h[near]) * reach
            h[near] = hn
            ramp_id = ramp.get('id', p['id'] + '_ramp')
            self.ramps.append(dict(id=ramp_id, plateau=p['id'], cliff_group=ramp.get('cliffGroup', ramp_id), pts=pts,
                                   s=s, z=z, width=ramp['width'] / 100.0, drop=drop))
        return h

    def _ponds(self, x, y, h):
        self.ponds = []
        for f in self.by_type['pond']:
            cx, cy = np.asarray(f['center']) / 100.0
            rx, ry = np.asarray(f['radii']) / 100.0
            level = f['waterLevel'] / 100.0
            # Its bed's lowest point: "bottom" (cm, a height like waterLevel: a tarn up on a hanging valley), else
            # "depth" below the island's meadow level (0).
            bottom = f['bottom'] / 100.0 if 'bottom' in f else -f['depth'] / 100.0
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
            # Where the meadow around the pond lies low, it's lifted gently (fading out over 12 m), so the shore
            # always stands above the water.
            lift = (np.maximum(level + 0.35 - h, 0.0) * smoothstep(0.0, 1.5, beyond)
                    * (1.0 - smoothstep(2.0, 12.0, beyond)))
            h = h + lift
            self.ponds.append(dict(id=f['id'], center=(cx, cy), radii=(rx, ry), level=level, bottom=bottom,
                                   rho=rho.astype(np.float32), beyond=beyond.astype(np.float32)))
        return h

    def _pond_at(self, point):
        """The pond whose shoreline ellipse holds a point, if any."""
        for p in self.ponds:
            (cx, cy), (rx, ry) = p['center'], p['radii']
            if math.hypot((point[0] - cx) / rx, (point[1] - cy) / ry) < 1.0:
                return p
        return None

    def _creek_valleys(self, x, y, h):
        """Each creek's course (where it leaves its pond, its water level, its lip at the rim) and its valley: higher
        ground eases down toward the banks over about 12 m, so the creek runs in a fold of the meadow rather than a
        canyon."""
        self.creeks = []
        for f in self.by_type['creek']:
            pts = catmull_rom(to_m(f['path']), step=0.5)
            s = arc_length(pts)
            bottom = f.get('bottom')
            if bottom:
                # A flat bottom the creek runs in (Mill Creek's, where the slimes live), cut first with sloped banks,
                # always falling toward the creek's end: the water then follows its floor.
                depth = bottom['depth'] / 100.0
                ground = sample(h, pts[:, 0], pts[:, 1], self.half)
                bed = np.minimum.accumulate(gauss_smooth_1d(ground, 16.0) - depth)
                h = self._channel(h, pts, s, bed, bottom['width'] / 200.0, bottom.get('bank', 0.8), seed=121,
                                  depth=depth)
            # The pond the creek starts in sets its water level.
            pond = self._pond_at(pts[0])
            level = pond['level'] if pond else 0.0
            # Where the creek leaves the pond and where it runs off the island (its lip; island setting).
            in_pond = np.zeros(len(pts), dtype=bool)
            if pond:
                (cx, cy), (rx, ry) = pond['center'], pond['radii']
                in_pond = np.hypot((pts[:, 0] - cx) / rx, (pts[:, 1] - cy) / ry) < 1.0
            s_exit = float(s[np.argmax(~in_pond)]) if in_pond.any() else 0.0
            outside = ~points_in_polygon(pts[:, 0], pts[:, 1], self.outline)
            k_lip = int(np.argmax(outside)) if (f.get('waterfallAtEnd') and outside.any()) else len(pts) - 1
            if f.get('falls'):
                k_lip = len(pts) - 1  # a falls is at the creek's end, where its gorge starts
            s_lip = float(s[k_lip])
            run = np.maximum(s - s_exit, 0.0)
            if pond is None and self.setting == 'grounded':
                # A spring: the water follows the ground down from where it rises, always falling.
                ground = sample(h, pts[:, 0], pts[:, 1], self.half)
                if f.get('waterfallAtEnd'):
                    # Past the core's edge (a stream running over the escarpment's lip) the ground holds its level to
                    # the end, as a gully's bed does: the canyon's drop beyond mustn't pull the water down before it.
                    on_core = sample(self.edge, pts[:, 0], pts[:, 1], self.half) > 0.5
                    if on_core.any() and not on_core.all():
                        last = int(np.nonzero(on_core)[0][-1])
                        ground = ground.copy()
                        ground[last + 1:] = ground[last]
                water = np.minimum.accumulate(gauss_smooth_1d(ground, 12.0) - 0.55 - 0.004 * s)
                water = water - 0.035 * np.maximum(s - (s_lip - 9.0), 0.0)
            else:
                water = level - 0.011 * run - 0.035 * np.maximum(s - (s_lip - 9.0), 0.0)
            dist, along, side = polyline_field(self.grid, pts, 16.0)
            near = np.isfinite(dist)
            d = dist[near] + 0.25 * fbm(x[near], y[near], 3.0, seed=31)
            if not bottom:  # a creek with a bottom keeps its banks
                bank = np.interp(along[near], s, water) + 0.8
                valley = (1.0 - smoothstep(2.5, 14.0, d)) * 0.85
                hn = h[near]
                h[near] = np.where(hn > bank, hn - (hn - bank) * valley, hn)
            self.creeks.append(dict(id=f['id'], feature=f, pts=pts, s=s, water=water, s_exit=s_exit, s_lip=s_lip,
                                    k_lip=k_lip, width=f['width'] / 100.0, dist=dist, along=along, side=side,
                                    wander=d - dist[near]))
        return h

    def _creeks(self, h):
        """Each creek's channel, cut after the roads (it runs under bridges): a rounded bed, banks at about 45
        degrees up to the bank level, gentler above."""
        for c in self.creeks:
            near = np.isfinite(c['dist'])
            d = c['dist'][near] + c.pop('wander')
            w = np.interp(c['along'][near], c['s'], c['water'])
            half = CREEK_WATER_HALF
            channel = np.where(d <= half, w - CREEK_DEPTH + CREEK_DEPTH * (d / half) ** 2,
                               np.where(d <= half + 0.8, w + 1.0 * (d - half), w + 0.8 + 0.7 * (d - half - 0.8)))
            hn = smin(h[near], channel, 0.25)
            # Banks stay above the water (except in the pond): low ground beside the creek is lifted gently.
            lift = (np.maximum(w + 0.3 - hn, 0.0) * smoothstep(half + 0.2, half + 0.9, d)
                    * (1.0 - smoothstep(2.5, 9.0, d)))
            h[near] = hn + lift * (c['along'][near] > c['s_exit'] + 1.0)
        return h

    def _rim(self, h):
        """The meadow rolls over the edge; the rock wall below belongs to the underside (island setting)."""
        e = self.edge
        roll = np.clip(1.0 - e / RIM_ROLL, 0.0, 1.0)
        amount = RIM_ROLL_DROP * (1.0 + 0.35 * fbm_raster(self.grid, 4.0, seed=51))
        for c in self.creeks:
            amount = amount * smoothstep(2.5, 6.0, c['dist'])  # a creek keeps its banks to the lip
        for p in self.plateaus:
            amount = amount * (1.0 - 0.5 * p['rise'])           # rock doesn't roll as far as turf
        return h - amount * roll * roll

    # --- Queries ---

    def at(self, raster, x, y):
        """Bilinear sample (at layout-meter points) of any raster over the map square."""
        return sample(raster, x, y, self.half)

    def resized(self, raster, n):
        """A raster over the map square resampled to n x n."""
        return resize(raster, n, self.half)

    def onto(self, raster, grid):
        """A raster over the map square resampled onto a grid: the square at its size (as resized()), or a window of
        it (area_math.Window: a split macro map's quarter)."""
        if not getattr(grid, 'window', False):
            return resize(raster, grid.n, self.half)
        out = np.empty((grid.n, grid.n), dtype=np.float32)
        for a in range(0, grid.n, 256):
            x, y = grid.mesh(a, min(grid.n, a + 256))
            out[a:a + 256] = sample(raster, x, y, self.half)
        return out

    def height(self, x, y):
        """Terrain height (m) at layout-meter points."""
        return sample(self.h, x, y, self.half)

    def water_surface(self):
        """The water surface height (m) per raster cell, NaN where there's no water: each pond at its level, each
        creek along its course down to its lip."""
        w = np.full(self.h.shape, np.nan, dtype=np.float32)
        for p in self.ponds:
            wet = p['rho'] < 1.35
            w[wet] = p['level']
        for c in self.creeks:
            reach = np.isfinite(c['dist']) & (c['dist'] < 3.2) & (c['along'] <= c['s_lip'] + 1.0)
            if c.get('gorge'):
                # The water stops at the falls' lip: none hangs over the gorge past the creek's end.
                ii, jj = np.nonzero(reach & (c['along'] >= c['s_lip'] - 1e-3))
                end, direction = c['pts'][-1], c['gorge']['direction']
                past = ((self.grid.c[ii] - end[0]) * direction[0] + (self.grid.c[jj] - end[1]) * direction[1]) > 0.0
                reach[ii[past], jj[past]] = False
            w[reach] = np.interp(c['along'][reach], c['s'], c['water'])
        for g in getattr(self, 'gorges', []):
            # A shallow stream on the gorge's floor, from the falls' foot on.
            dist, along, _ = polyline_field(self.grid, g['pts'], g['half'])
            ii, jj = np.nonzero(np.isfinite(dist) & (dist < g['half'] - 0.3))
            lip, direction = g['lip'], g['direction']
            past = ((self.grid.c[ii] - lip[0]) * direction[0] + (self.grid.c[jj] - lip[1]) * direction[1]) > 0.2
            ii, jj = ii[past], jj[past]
            w[ii, jj] = np.interp(along[ii, jj], g['s'], g['floor']) + GORGE_STREAM
        return w
