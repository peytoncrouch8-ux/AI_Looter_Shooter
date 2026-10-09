"""An area's ambient life (Docs/Polish/BorderlandsComparison.md, item 5; Source/AI_Looter_Shooter/World/Fauna*): where the
birds perch and fly, where the insects keep, where tumbleweeds roll and dust devils rise, and the washing on the lines.

    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_fauna.py RansomsRest"

places only this again (the area's Fauna folder; the level is saved), and build_area.py's full build can call place(build)
once the level stands (after gameplay(), since Hob's perches are kept clear). Everything it places is tagged with the
area's build tag and Fauna and sits in <area>/Fauna, so building again replaces only what this placed.

What it reads (all of it from what the level already has, nothing hand-placed):
- Perches, found by traces against the placed pieces themselves: the tops of fence rails and posts, stone walls,
  headboards and crosses, cairns, hitch rails, troughs, hay bales, crates and barrels (the dressing's AInstancedProps,
  PERCH_MESHES), the ridges and flat tops of the buildings (BUILDING_MESHES), and the limbs of dead trees (a grid of
  traces from above; a tree whose limbs have no collision gives its trunk's top). Each perch is kept off the story:
  none within HOB_CLEARANCE of one of Hob's perches (AHobBird's, and every SOCKET_Perch* in the level), so a plain crow
  is never seen where the story's one-eyed crow is; none outside the playable area.
- Flocks: the perches gathered into clusters (the densest first, graves and dead trees counting a little more), each at
  least its species' FLOCK_SPACING from the last, with open ground beside it for the birds to walk and peck on (traced:
  terrain tagged Ground, gentle, dry, not under anything). Ransom's Rest gets crows, Skyreach sparrows; both get a pair
  of hawks over the land, and Skyreach swallows over the pond and the Wallow.
- Insects: butterflies along the larkspur (the salt line, the field wall) and the orchard rows on Ransom's Rest, over the
  town's gardens and the pasture on Skyreach; dragonflies along the creeks and over the ponds; fireflies at dusk in Mill
  Creek's bottom, the orchard and the farm yard; flies over the outhouses and the Gravemother's larder.
- Wind (Ransom's Rest): tumbleweed lanes, open ground running downwind (WIND_YAW, the way the smoke leans) from the
  roads and the open zones, each checked clear for at least 20 m; dust devil spots on the roads and yards, checked open.
- Washing: three pieces on the empty back line of every laundry line in the level.

Every choice comes from seeded streams and sorted lists, so a rebuild places the same. Run plan_* anywhere (plain
Python and plain data); place() needs the editor.
"""
import json
import math
import os
import random
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area  # noqa: E402

CLASSES = '/Script/AI_Looter_Shooter.'
FOLDER = 'Fauna'
TAG = 'Fauna'

# The way the wind blows (degrees): build_area.py's, the foliage master's sway and the chimney smoke's lean.
WIND_YAW = build_area.WIND_YAW

# No bird perches nearer one of Hob's perches than this (cm).
HOB_CLEARANCE = 1500.0

# Perches on the dressing's pieces, by the start of the mesh's name: how a bird stands there.
PERCH_MESHES = (
    ('FenceRail', 'Rail'), ('FenceBroken', 'Rail'), ('FencePost', 'Post'), ('Fence_PicketSection', 'Rail'),
    ('Fence_PicketPost', 'Post'), ('Fence_IronSection', 'Rail'), ('Fence_IronPost', 'Post'), ('Fence_IronGate', 'Rail'),
    ('StoneWall_Corner', 'Post'), ('StoneWallEnd', 'Post'), ('StoneWall', 'Rail'), ('Grave_Headboard', 'Rail'),
    ('Grave_Cross', 'Post'), ('Cairn_', 'Post'), ('HitchRail', 'Rail'), ('WaterTrough', 'Rail'), ('HayBale_Round', 'Post'),
    ('HayBale_Square', 'Rail'), ('Crate_', 'Post'), ('Barrel_', 'Post'), ('FirewoodStack', 'Rail'), ('Cart', 'Rail'),
    ('LaundryLine', 'Rail'), ('Signpost', 'Post'), ('LampPost', 'Post'), ('NoticeBoard', 'Rail'), ('Woodshed', 'Roof'),
    ('DeadTree', 'Branch'),
)
# Buildings whose ridges and tops birds sit on, by the start of the mesh's name.
BUILDING_MESHES = ('Barn', 'Farmhouse', 'Cottage', 'LogCabin', 'FalseFront', 'Chapel', 'Depot', 'CoffinShed', 'Outhouse',
                   'Well', 'WaterTower', 'Woodshed', 'Lookout')
# How much each kind of perch draws a flock (crows on graves and dead trees, as the valley is in mourning).
KIND_WEIGHT = {'Rail': 1.0, 'Post': 1.0, 'Roof': 1.1, 'Branch': 1.4, 'Ground': 0.0}
GRAVE_WEIGHT = 1.4

# Along a rail, a perch every RAIL_SPACING (cm); a roof's ridge every ROOF_SPACING; a dead tree's crown on a grid.
RAIL_SPACING = 45.0
ROOF_SPACING = 70.0
BRANCH_GRID = 7
BRANCH_MIN_HEIGHT = 250.0     # cm over the ground: limbs, not roots
BRANCHES_PER_TREE = 10
PERCH_MIN_APART = 35.0        # no two perches nearer than this (cm)

# Each species: its models (SM_<name> under /Game/Art/Creatures) and its AFaunaFlock settings.
SPECIES = {
    'crow': dict(
        meshes=dict(perched_mesh='Crow_Perched', head_mesh='Crow_Head', flying_mesh='Crow_Flying',
                    wing_mesh_l='Crow_WingInnerL', wing_mesh_r='Crow_WingInnerR', outer_wing_mesh_l='Crow_WingOuterL',
                    outer_wing_mesh_r='Crow_WingOuterR'),
        flocks=8, birds=(4, 8), spacing=2200.0, reach=800.0, ground=(3, 6),
        settings=dict(flee_radius=1200.0, gunfire_radius=5000.0, impact_radius=1500.0, flight_speed=900.0, circle_radius=1600.0,
                      circle_height=1400.0, calm_seconds_min=8.0, calm_seconds_max=16.0, leave_after_seconds=30.0,
                      hop_seconds_min=14.0, hop_seconds_max=32.0, hops_on_ground=False, ground_speed=45.0, head_seconds_min=0.6,
                      head_seconds_max=2.2, wingbeats_per_second=4.5, wingbeat_degrees=38.0, glide_degrees=5.0, glide_share=0.35,
                      call_cue='World.Fauna.Crow.Caw', take_off_cue='World.Fauna.Crow.TakeOff', call_range=5000.0,
                      call_seconds_min=7.0, call_seconds_max=20.0, cull_distance=9000.0, bird_scale=1.0, scale_jitter=0.08)),
    'sparrow': dict(
        meshes=dict(perched_mesh='Sparrow_Perched', head_mesh='Sparrow_Head', flying_mesh='Sparrow_Flying',
                    wing_mesh_l='Sparrow_WingL', wing_mesh_r='Sparrow_WingR'),
        flocks=9, birds=(5, 9), spacing=1600.0, reach=600.0, ground=(3, 6),
        settings=dict(flee_radius=700.0, gunfire_radius=3500.0, impact_radius=1000.0, flight_speed=1000.0, circle_radius=900.0,
                      circle_height=700.0, calm_seconds_min=5.0, calm_seconds_max=10.0, leave_after_seconds=25.0,
                      hop_seconds_min=6.0, hop_seconds_max=14.0, hops_on_ground=True, ground_speed=70.0, head_seconds_min=0.25,
                      head_seconds_max=0.9, wingbeats_per_second=13.0, wingbeat_degrees=55.0, glide_degrees=0.0, glide_share=0.25,
                      call_cue='World.Fauna.Sparrow.Chirp', take_off_cue='World.Fauna.Sparrow.TakeOff', call_range=2500.0,
                      call_seconds_min=3.0, call_seconds_max=8.0, cull_distance=6000.0, bird_scale=1.0, scale_jitter=0.1)),
    'swallow': dict(
        meshes=dict(flying_mesh='Swallow_Flying', wing_mesh_l='Swallow_WingL', wing_mesh_r='Swallow_WingR'),
        settings=dict(flight_speed=1100.0, wingbeats_per_second=7.0, wingbeat_degrees=45.0, glide_degrees=2.0, glide_share=0.4,
                      aerial_figure=2, call_cue='World.Fauna.Swallow.Twitter', call_range=3000.0, call_seconds_min=6.0,
                      call_seconds_max=15.0, cull_distance=7000.0, bird_scale=1.0, scale_jitter=0.06)),
    'hawk': dict(
        meshes=dict(flying_mesh='Hawk_Flying', wing_mesh_l='Hawk_WingL', wing_mesh_r='Hawk_WingR'),
        settings=dict(flight_speed=1000.0, wingbeats_per_second=2.6, wingbeat_degrees=24.0, glide_degrees=9.0, glide_share=0.85,
                      aerial_figure=1, call_cue='World.Fauna.Hawk.Cry', call_range=15000.0, call_seconds_min=25.0,
                      call_seconds_max=70.0, cull_distance=0.0, bird_scale=1.0, scale_jitter=0.05)),
}

# Each insect: its models and AFaunaSwarm settings.
INSECTS = {
    'butterfly': dict(kind='BUTTERFLY', speed=120.0, flee_radius=250.0, insect_scale=1.0, max_active=10, active_radius=3500.0,
                      cull_distance=4500.0, shown_in='Day'),
    'dragonfly': dict(kind='DRAGONFLY', body='Dragonfly', speed=600.0, flee_radius=250.0, insect_scale=1.0, max_active=10,
                      active_radius=3000.0, cull_distance=3500.0, shown_in='Day', sound_cue='World.Fauna.Dragonfly.Buzz', sound_range=600.0),
    'firefly': dict(kind='FIREFLY', body='Firefly', speed=40.0, flee_radius=150.0, insect_scale=1.6, max_active=40,
                    active_radius=4000.0, cull_distance=6000.0, shown_in='Dusk'),
    'fly': dict(kind='FLY', body='Fly', speed=280.0, flee_radius=0.0, insect_scale=1.4, max_active=14, active_radius=1500.0,
                cull_distance=2500.0, sound_cue='World.Fauna.Flies', sound_range=800.0),
}

# What each area has. Ransom's Rest: late summer, mourning, crows; Skyreach: a green island, sparrows and swallows.
AREAS = {
    'RansomsRest': dict(
        species='crow', hawks=dict(count=2, radius=3500.0, low=4500.0, high=7500.0),
        swallows=(), butterflies=('Sulphur', 'Copper'),
        flower_lines=('saltLine', 'fieldWall'), flower_zones=(), orchard=True, gardens=False,
        fireflies=True, firefly_zones=('farm',), flies_labels=('Den_Bones_PileB', 'Den_CattleSkull', 'Den_Ribcage'),
        tumbleweeds=True, lane_roads=('mainStreet', 'farmRoad', 'westRoad', 'northRoad', 'fieldsPath', 'yardRoad'),
        lane_zones=('whitlock', 'farm', 'bootHill'), dust_roads=('mainStreet', 'farmRoad', 'westRoad', 'fieldsPath', 'yardRoad'),
        dust_zones=('farm', 'whitlock', 'depot', 'mainStreet'), avoid_zones=('gravewindPoint',)),
    'TutorialIsland': dict(
        species='sparrow', hawks=dict(count=2, radius=3000.0, low=3500.0, high=6000.0),
        swallows=('pond', 'wallow'), butterflies=('Sulphur', 'White'),
        flower_lines=(), flower_zones=('pasture', 'orchard', 'farmstead'), orchard=True, gardens=True,
        fireflies=False, firefly_zones=(), flies_labels=(), tumbleweeds=False, lane_roads=(), lane_zones=(),
        dust_roads=(), dust_zones=(), avoid_zones=()),
}

# Tumbleweed lanes: their length (cm), the shortest clear run that counts, how far apart, how many.
LANE_LENGTH = 4500.0
LANE_MIN_CLEAR = 2000.0
LANE_SPACING = 2500.0
LANES_MAX = 10
# Dust devil spots: how open (cm clear all round), how far apart, how many.
DUST_CLEAR = 600.0
DUST_SPACING = 2000.0
DUST_MAX = 10

# The laundry line (Art/Models/Props/VillageProps.py): its back line, in the line's own frame (cm; Unreal's X forward, Y
# along the line), its height at a point along it, and where three pieces hang on it.
LAUNDRY_BACK_X = -28.0
LAUNDRY_SPAN = 360.0
LAUNDRY_TOP = 200.0
LAUNDRY_SAG = 16.0
LAUNDRY_PIECES = ((-115.0, 0), (-10.0, 1), (95.0, 2))   # (along the line, which cloth: sheet, shirt, towel)
CLOTH_MESHES = ('WindCloth_Sheet', 'WindCloth_Shirt', 'WindCloth_Towel')


# ---------------------------------------------------------------------------
# Plain Python: planning from plain data
# ---------------------------------------------------------------------------

def inside(point, polygon):
    return build_area.inside(point, polygon)


def dist2(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def polyline_samples(points, step):
    """Points every step (cm) along a polyline [(x, y[, z]), ...], with the direction there (unit XY)."""
    out = []
    carry = 0.0
    for a, b in zip(points, points[1:]):
        length = dist2(a, b)
        if length < 1e-3:
            continue
        ux, uy = (b[0] - a[0]) / length, (b[1] - a[1]) / length
        d = carry
        while d <= length:
            t = d / length
            z = a[2] + (b[2] - a[2]) * t if len(a) > 2 and len(b) > 2 else 0.0
            out.append(((a[0] + ux * d, a[1] + uy * d, z), (ux, uy)))
            d += step
        carry = d - length
    return out


def polygon_centroid(polygon):
    xs, ys = [p[0] for p in polygon], [p[1] for p in polygon]
    return sum(xs) / len(xs), sum(ys) / len(ys)


def polygon_grid(polygon, step, limit, seed):
    """Up to limit points inside a polygon on a grid of step (cm), chosen by the seed."""
    xs, ys = [p[0] for p in polygon], [p[1] for p in polygon]
    points = []
    x = min(xs) + step * 0.5
    while x < max(xs):
        y = min(ys) + step * 0.5
        while y < max(ys):
            if inside((x, y), polygon):
                points.append((x, y))
            y += step
        x += step
    rng = random.Random(seed)
    rng.shuffle(points)
    return sorted(points[:limit])


class Grid:
    """Items bucketed by where they stand (XY, cells of size cm), to find the near ones without looking at them all.
    Buckets fill in the order items come, so every walk over them is the same on every run."""

    def __init__(self, cell):
        self.cell = cell
        self.cells = {}

    def key(self, p):
        return int(math.floor(p[0] / self.cell)), int(math.floor(p[1] / self.cell))

    def add(self, p, item):
        self.cells.setdefault(self.key(p), []).append(item)

    def near(self, p, radius):
        reach = int(math.ceil(radius / self.cell))
        kx, ky = self.key(p)
        for dx in range(-reach, reach + 1):
            for dy in range(-reach, reach + 1):
                yield from self.cells.get((kx + dx, ky + dy), ())


def thin(points, apart, key=lambda p: p):
    """Points no nearer than apart (cm) to one another (XY), keeping the earlier."""
    grid = Grid(max(apart, 1.0))
    kept = []
    for p in points:
        at = key(p)
        if all(dist2(at, key(q)) >= apart for q in grid.near(at, apart)):
            kept.append(p)
            grid.add(at, p)
    return kept


def near_hob(point, hob_perches, clearance=HOB_CLEARANCE):
    return any(math.dist(point, h) < clearance for h in hob_perches)


def plan_flocks(candidates, species, hob_perches, seed):
    """Flocks from perch candidates [(x, y, z, yaw, kind, weight)]: the densest cluster first (weighted), each its
    species' spacing from the last, every perch clear of Hob's. Returns [(centre (x, y, z), [perches])]."""
    spec = SPECIES[species]
    reach = spec['reach']
    usable = sorted(c for c in candidates if not near_hob(c[:3], hob_perches) and c[4] != 'Ground')
    grid = Grid(reach)
    for i, c in enumerate(usable):
        grid.add(c, i)
    neighbours = [sorted(j for j in grid.near(c, reach) if dist2(c, usable[j]) < reach) for c in usable]
    flocks = []
    used = [False] * len(usable)
    while len(flocks) < spec['flocks']:
        best, best_score = None, 0.0
        for i, c in enumerate(usable):
            # Its own middle may draw a reach toward the last flock's: the seeds keep the spacing and a reach more.
            if used[i] or any(dist2(c, f[0]) < spec['spacing'] + reach for f in flocks):
                continue
            score = sum(usable[j][5] for j in neighbours[i] if not used[j])
            if score > best_score + 1e-6:
                best, best_score = i, score
        if best is None or best_score < 4.0:
            break
        centre = usable[best]
        # The nearest dozen and a half round the densest point, each one taken out of what later flocks may use.
        members = sorted((j for j in neighbours[best] if not used[j]), key=lambda j: (dist2(centre, usable[j]), usable[j][:2]))[:18]
        for j in members:
            used[j] = True
        perches = [usable[j] for j in members]
        cx = sum(p[0] for p in perches) / len(perches)
        cy = sum(p[1] for p in perches) / len(perches)
        cz = sum(p[2] for p in perches) / len(perches)
        flocks.append(((cx, cy, cz), perches))
    return flocks


def flower_zones(source, computed, area, seed):
    """Where butterflies keep: along the area's flower lines (larkspur by fences and walls), its flower zones, the
    orchard rows and the town's gardens. [(x, y, radius, count)] (heights come from the ground)."""
    spec = AREAS[area]
    zones = []
    for obstacle in source.get('obstacles', []):
        if obstacle['id'] in spec['flower_lines']:
            path = obstacle.get('path') or obstacle.get('polygon') or []
            for (x, y, _), _ in polyline_samples([tuple(p) + (0.0,) for p in path], 1800.0):
                zones.append((x, y, 600.0, 2))
    polygons = {z['id']: z['polygon'] for z in source.get('zones', [])}
    for k, zone_id in enumerate(spec['flower_zones']):
        if zone_id in polygons:
            for x, y in polygon_grid(polygons[zone_id], 2000.0, 6, seed + k):
                zones.append((x, y, 700.0, 2))
    if spec['orchard']:
        for row in computed.get('orchardRows', []):
            a, b = row['start'], row['end']
            zones.append(((a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5, 700.0, 2))
    if spec['gardens']:
        for line in load_town_lines(source, area):
            if line.get('kit') == 'picket' and line.get('closed') and len(line['points']) >= 3:
                cx, cy = polygon_centroid(line['points'])
                xs, ys = [p[0] for p in line['points']], [p[1] for p in line['points']]
                radius = min(700.0, 0.5 * min(max(xs) - min(xs), max(ys) - min(ys)) + 100.0)
                zones.append((cx, cy, radius, 3))
    return thin(zones, 900.0)


def load_town_lines(source, area):
    town = source.get('level', {}).get('town')
    if not town:
        return []
    path = os.path.join(build_area.PROJECT, 'Art', 'Levels', area, town)
    if not os.path.exists(path):
        return []
    with open(path) as f:
        return json.load(f).get('lines', [])


def water_zones(computed):
    """Where dragonflies keep: along every creek (at the water, its width and a little) and over every pond.
    [(x, y, z, radius, count)]."""
    zones = []
    for creek in computed.get('creeks', {}).values():
        width = float(creek.get('waterWidth', 240.0))
        for (x, y, z), _ in polyline_samples([tuple(p) for p in creek['points']], 1400.0):
            zones.append((x, y, z, width * 0.5 + 150.0, 2))
    for key, pond in sorted(computed.get('ponds', {}).items()):
        (cx, cy), (rx, ry) = pond['center'], pond['radii']
        zones.append((cx, cy, pond['waterZ'], min(rx, ry) * 0.8, 3 if max(rx, ry) > 800.0 else 2))
    return zones


def firefly_zones(source, computed, area):
    """Fireflies at dusk: along the creek's bottom, in the orchard, over the farm yard. [(x, y, z or None, radius, count)]."""
    spec = AREAS[area]
    zones = []
    for creek in computed.get('creeks', {}).values():
        for (x, y, z), _ in polyline_samples([tuple(p) for p in creek['points']], 1600.0):
            zones.append((x, y, z, 700.0, 4))
    for row in computed.get('orchardRows', []):
        a, b = row['start'], row['end']
        zones.append(((a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5, None, 700.0, 3))
    polygons = {z['id']: z['polygon'] for z in source.get('zones', [])}
    for zone_id in spec['firefly_zones']:
        if zone_id in polygons:
            cx, cy = polygon_centroid(polygons[zone_id])
            zones.append((cx, cy, None, 1200.0, 6))
    return zones


def lane_starts(source, computed, area, seed):
    """Candidate starts for tumbleweed lanes: along the area's lane roads (every 15 m) and on a grid in its open zones."""
    spec = AREAS[area]
    starts = []
    roads = computed.get('roads', {})
    for road_id in spec['lane_roads']:
        if road_id in roads:
            for (x, y, _), _ in polyline_samples([tuple(p) for p in roads[road_id]['points']], 1500.0):
                starts.append((x, y))
    polygons = {z['id']: z['polygon'] for z in source.get('zones', [])}
    for k, zone_id in enumerate(spec['lane_zones']):
        if zone_id in polygons:
            starts.extend(polygon_grid(polygons[zone_id], 2000.0, 12, seed + 31 + k))
    return sorted(set((round(x, 1), round(y, 1)) for x, y in starts))


def dust_starts(source, computed, area):
    spec = AREAS[area]
    starts = []
    roads = computed.get('roads', {})
    for road_id in spec['dust_roads']:
        if road_id in roads:
            for (x, y, _), _ in polyline_samples([tuple(p) for p in roads[road_id]['points']], 3000.0):
                starts.append((x, y))
    polygons = {z['id']: z['polygon'] for z in source.get('zones', [])}
    for zone_id in spec['dust_zones']:
        if zone_id in polygons:
            starts.append(polygon_centroid(polygons[zone_id]))
    return sorted(set((round(x, 1), round(y, 1)) for x, y in starts))


def laundry_points(transform_location, yaw):
    """Where three pieces hang on a laundry line's back line: (world point, cloth index, yaw), given the line's
    transform_location(local (x, y, z)) and its yaw."""
    out = []
    for along, cloth in LAUNDRY_PIECES:
        t = along / LAUNDRY_SPAN
        z = LAUNDRY_TOP - LAUNDRY_SAG * (1.0 - 4.0 * t * t) - 1.0
        out.append((transform_location((LAUNDRY_BACK_X, along, z)), cloth, yaw))
    return out


# ---------------------------------------------------------------------------
# In the editor
# ---------------------------------------------------------------------------

def vec(v):
    return unreal.Vector(float(v[0]), float(v[1]), float(v[2]))


def trace_component(component, start, end):
    """Where a line first meets a component (complex collision): (point, normal) or None."""
    hit = component.line_trace_component(vec(start), vec(end), True, False, False)
    if not hit:
        return None
    if isinstance(hit, tuple):
        return hit[0], hit[1]
    parts = hit.to_tuple()
    return parts[5], parts[7]


class Ground:
    """The terrain under a point (its tiles alone): (point, normal) or None."""

    def __init__(self, build):
        self.tiles = build_area.terrain_tiles(build.tag)

    def __call__(self, x, y, top=20000.0, bottom=-20000.0):
        best = None
        for mesh, x0, x1, y0, y1 in self.tiles:
            if not (x0 <= x <= x1 and y0 <= y <= y1):
                continue
            hit = trace_component(mesh, (x, y, top), (x, y, bottom))
            if hit and (best is None or hit[0].z > best[0].z):
                best = hit
        return best


def perch_kind(mesh_name):
    for prefix, kind in PERCH_MESHES:
        if mesh_name.startswith(prefix):
            return kind
    return None


def local_axes(transform):
    """A transform's local X and Y as world unit vectors (XY)."""
    rot = transform.rotation.rotator()
    yaw = math.radians(rot.yaw)
    return (math.cos(yaw), math.sin(yaw)), (-math.sin(yaw), math.cos(yaw)), rot.yaw


def rail_perches(component, transform, box, kind, out):
    """Perches along a piece's top: along its longer side, a trace down at each step; a post's or a cairn's top at its
    middle. A bird on a rail faces across it."""
    size_x, size_y = box.max.x - box.min.x, box.max.y - box.min.y
    along_x = size_x > size_y
    cx, cy = (box.min.x + box.max.x) * 0.5, (box.min.y + box.max.y) * 0.5
    top = box.max.z
    if kind == 'Post':
        steps = [(cx, cy)]
    else:
        lo, hi = (box.min.x, box.max.x) if along_x else (box.min.y, box.max.y)
        count = max(1, int((hi - lo - 20.0) // RAIL_SPACING))
        steps = [((lo + 10.0 + (hi - lo - 20.0) * (k + 0.5) / count), cy) if along_x else
                 (cx, lo + 10.0 + (hi - lo - 20.0) * (k + 0.5) / count) for k in range(count)]
    _, _, yaw = local_axes(transform)
    face = yaw + (0.0 if along_x else 90.0) + 90.0
    for lx, ly in steps:
        start = transform.transform_location(unreal.Vector(lx, ly, top + 30.0))
        end = transform.transform_location(unreal.Vector(lx, ly, top - 60.0))
        hit = trace_component(component, (start.x, start.y, start.z), (end.x, end.y, end.z))
        if hit and hit[1].z > 0.5:
            out.append((hit[0].x, hit[0].y, hit[0].z + 0.5, face, kind))


def roof_perches(component, transform, box, out):
    """Perches along a building's ridge: its long axis's middle line traced from above, kept where nothing beside it
    stands higher (a ridge or a flat top)."""
    size_x, size_y = box.max.x - box.min.x, box.max.y - box.min.y
    along_x = size_x >= size_y
    cx, cy = (box.min.x + box.max.x) * 0.5, (box.min.y + box.max.y) * 0.5
    lo, hi = (box.min.x, box.max.x) if along_x else (box.min.y, box.max.y)
    span = hi - lo
    count = max(1, int(span * 0.7 // ROOF_SPACING))
    _, _, yaw = local_axes(transform)
    face = yaw + (0.0 if along_x else 90.0) + 90.0
    top = box.max.z + 100.0

    def height_at(lx, ly):
        start = transform.transform_location(unreal.Vector(lx, ly, top))
        end = transform.transform_location(unreal.Vector(lx, ly, box.min.z))
        return trace_component(component, (start.x, start.y, start.z), (end.x, end.y, end.z))
    for k in range(count):
        t = lo + span * (0.15 + 0.7 * (k + 0.5) / count)
        lx, ly = (t, cy) if along_x else (cx, t)
        hit = height_at(lx, ly)
        if not hit or hit[1].z < 0.3:
            continue
        beside = [height_at(lx, ly + d) if along_x else height_at(lx + d, ly) for d in (-40.0, 40.0)]
        if any(b and b[0].z > hit[0].z + 5.0 for b in beside):
            continue
        out.append((hit[0].x, hit[0].y, hit[0].z + 0.5, face, 'Roof'))


def branch_perches(component, transform, box, ground, out):
    """Perches on a dead tree's limbs: a grid of traces down over its crown; without limbs to meet, its trunk's top."""
    found = []
    for i in range(BRANCH_GRID):
        for j in range(BRANCH_GRID):
            lx = box.min.x + (box.max.x - box.min.x) * (i + 0.5) / BRANCH_GRID
            ly = box.min.y + (box.max.y - box.min.y) * (j + 0.5) / BRANCH_GRID
            start = transform.transform_location(unreal.Vector(lx, ly, box.max.z + 50.0))
            end = transform.transform_location(unreal.Vector(lx, ly, box.min.z))
            hit = trace_component(component, (start.x, start.y, start.z), (end.x, end.y, end.z))
            if not hit or hit[1].z < 0.35:
                continue
            floor = ground(hit[0].x, hit[0].y)
            if floor and hit[0].z - floor[0].z >= BRANCH_MIN_HEIGHT:
                found.append((hit[0].x, hit[0].y, hit[0].z + 0.5))
    found = thin(sorted(found, key=lambda p: -p[2]), 50.0)[:BRANCHES_PER_TREE]
    rng = random.Random(f'{round(transform.translation.x)} {round(transform.translation.y)}')
    for x, y, z in found:
        out.append((x, y, z, rng.uniform(-180.0, 180.0), 'Branch'))
    return len(found)


def gather_perches(build, ground, log):
    """Every perch candidate in the level: [(x, y, z, yaw, kind, weight)]."""
    candidates = []
    counts = {}
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    for actor in sorted(actors, key=lambda a: str(a.get_actor_label())):
        if unreal.Name(build.tag) not in actor.tags or unreal.Name(TAG) in actor.tags:
            continue
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh = component.static_mesh
            if mesh is None:
                continue
            name = mesh.get_name()[3:] if mesh.get_name().startswith('SM_') else mesh.get_name()
            kind = perch_kind(name)
            building = name.startswith(BUILDING_MESHES) and not isinstance(component, unreal.InstancedStaticMeshComponent)
            if kind is None and not building:
                continue
            box = mesh.get_bounding_box()
            found = []
            if isinstance(component, unreal.InstancedStaticMeshComponent):
                for i in range(component.get_instance_count()):
                    transform = component.get_instance_transform(i, True)
                    if isinstance(transform, tuple):
                        transform = transform[1] if transform[0] else None
                    if transform is None:
                        continue
                    if kind == 'Branch':
                        branch_perches(component, transform, box, ground, found)
                    elif kind == 'Roof':
                        roof_perches(component, transform, box, found)
                    else:
                        rail_perches(component, transform, box, kind, found)
            else:
                transform = component.get_world_transform()
                if kind == 'Branch':
                    branch_perches(component, transform, box, ground, found)
                elif building or kind == 'Roof':
                    roof_perches(component, transform, box, found)
                else:
                    rail_perches(component, transform, box, kind, found)
            weight_scale = GRAVE_WEIGHT if name.startswith(('Grave_', 'Cairn_')) else 1.0
            for x, y, z, yaw, k in found:
                candidates.append((round(x, 1), round(y, 1), round(z, 1), round(yaw, 1), k, KIND_WEIGHT[k] * weight_scale))
            counts[name] = counts.get(name, 0) + len(found)
    log(f'fauna: perch candidates {len(candidates)}: ' + ', '.join(f'{n} {c}' for n, c in sorted(counts.items()) if c))
    candidates = thin(sorted(candidates), PERCH_MIN_APART)
    return candidates


def hob_perches(build):
    """Every place Hob perches: his actor's perches, and every SOCKET_Perch* on what the level places."""
    found = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if actor.get_class().get_name() == 'HobBird':
            for perch in actor.get_editor_property('perches'):
                loc = perch.get_editor_property('location')
                found.append((loc.x, loc.y, loc.z))
            continue
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh = component.static_mesh
            if mesh is None or isinstance(component, unreal.InstancedStaticMeshComponent):
                continue
            try:
                sockets = mesh.get_editor_property('sockets') or []
            except Exception:
                sockets = []
            for socket in sockets:
                if str(socket.get_editor_property('socket_name')).startswith('Perch'):
                    where = component.get_socket_location(socket.get_editor_property('socket_name'))
                    found.append((where.x, where.y, where.z))
    return found


def playable_polygon():
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if actor.get_class().get_name() == 'PlayableArea':
            return [(c.x, c.y) for c in actor.get_editor_property('corners')]
    return None


def building_boxes(build):
    """The placed buildings' boxes (XY, with their top), so nothing on the ground stands inside one."""
    boxes = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if unreal.Name(build.tag) not in actor.tags:
            continue
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh = component.static_mesh
            if mesh is None or isinstance(component, unreal.InstancedStaticMeshComponent):
                continue
            name = mesh.get_name()[3:]
            if name.startswith(BUILDING_MESHES):
                origin, extent = actor.get_actor_bounds(False)
                boxes.append((origin.x - extent.x, origin.x + extent.x, origin.y - extent.y, origin.y + extent.y))
    return boxes


def volumes():
    """Every volume in the level (the PCG volume, trigger boxes): world traces look through them."""
    return [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a, unreal.Volume)]


def in_box(point, boxes, margin=0.0):
    return any(b[0] - margin < point[0] < b[1] + margin and b[2] - margin < point[1] < b[3] + margin for b in boxes)


def in_creek(point, computed, margin=100.0):
    for creek in computed.get('creeks', {}).values():
        half = float(creek.get('waterWidth', 240.0)) * 0.5 + margin
        pts = creek['points']
        for a, b in zip(pts, pts[1:]):
            ax, ay, bx, by = a[0], a[1], b[0], b[1]
            dx, dy = bx - ax, by - ay
            length2 = dx * dx + dy * dy
            t = 0.0 if length2 < 1e-6 else max(0.0, min(1.0, ((point[0] - ax) * dx + (point[1] - ay) * dy) / length2))
            if math.hypot(point[0] - (ax + dx * t), point[1] - (ay + dy * t)) < half:
                return True
    return False


def open_ground(build, ground, x, y, boxes, ignore, wander=80.0):
    """A spot of open, gentle, dry ground at (x, y), with room to walk about it: (point, normal, wander) or None. ignore:
    the actors a world trace looks through (the volumes)."""
    hit = ground(x, y)
    if not hit or hit[1].z < 0.9:
        return None
    if build.in_water((x, y)) or in_creek((x, y), build.layout) or in_box((x, y), boxes, 50.0):
        return None
    # Nothing over it (a roof, a tree's crown, a rock's overhang): a trace down from above meets the ground first.
    world = unreal.EditorLevelLibrary.get_editor_world()
    above = unreal.SystemLibrary.line_trace_single(world, vec((x, y, hit[0].z + 3000.0)), vec((x, y, hit[0].z - 10.0)),
                                                   unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, ignore, unreal.DrawDebugTrace.NONE, True)
    if above is not None:
        parts = above.to_tuple()
        actor = parts[9]
        if actor is not None and unreal.Name('Ground') not in actor.tags and abs(parts[5].z - hit[0].z) > 20.0:
            return None
    # Room to walk: the ground round it level with it.
    for dx, dy in ((wander, 0.0), (-wander, 0.0), (0.0, wander), (0.0, -wander)):
        side = ground(x + dx, y + dy)
        if not side or abs(side[0].z - hit[0].z) > 25.0:
            wander = 0.0
            break
    return hit[0], hit[1], wander


def struct(cls_name, **values):
    made = getattr(unreal, cls_name)()
    for key, value in values.items():
        made.set_editor_property(key, value)
    return made


def mesh_asset(meshes, name, build, missing):
    if name not in meshes:
        missing.add(name)
        return None
    return unreal.load_asset(meshes[name])


def place_flock(build, cls, label, species, perches, count, seed, meshes, missing, mode='PERCHING', aerial=None):
    spec = SPECIES[species]
    loaded = {prop: mesh_asset(meshes, name, build, missing) for prop, name in spec['meshes'].items()}
    if any(v is None for v in loaded.values()):
        return None
    centre = aerial['center'] if aerial else perches[0].get_editor_property('location')
    actor = build.place(cls, (centre.x, centre.y, centre.z), label=label, folder=FOLDER, tags=(TAG,))
    actor.set_editor_property('mode', getattr(unreal.FaunaFlockMode, mode))
    for prop, mesh in loaded.items():
        actor.set_editor_property(prop, mesh)
    for prop, value in spec['settings'].items():
        actor.set_editor_property(prop, value)
    actor.set_editor_property('bird_count', count)
    actor.set_editor_property('seed', seed)
    if aerial:
        actor.set_editor_property('aerial_area', aerial['zone'])
    else:
        actor.set_editor_property('perches', perches)
    return actor


def place(build):
    """The area's ambient life (see the module's docstring), in its Fauna folder."""
    area = build.name
    if area not in AREAS:
        build.log(f'fauna: no ambient life planned for {area}')
        return
    spec = AREAS[area]
    flock_cls = unreal.load_class(None, CLASSES + 'FaunaFlock')
    if flock_cls is None:
        build.warn('fauna: no FaunaFlock class (build the game module first): no ambient life placed')
        return
    swarm_cls = unreal.load_class(None, CLASSES + 'FaunaSwarm')
    tumble_cls = unreal.load_class(None, CLASSES + 'FaunaTumbleweeds')
    dust_cls = unreal.load_class(None, CLASSES + 'FaunaDustDevils')
    cloth_cls = unreal.load_class(None, CLASSES + 'FaunaCloth')
    meshes = build_area.mesh_index()
    missing = set()
    ground = Ground(build)
    if not ground.tiles:
        build.warn('fauna: no terrain tiles tagged Ground (build the whole level first): nothing placed')
        return
    boundary = playable_polygon()
    hob = hob_perches(build)
    boxes = building_boxes(build)
    ignore = volumes()
    avoid = [z['polygon'] for z in build.source.get('zones', []) if z['id'] in spec['avoid_zones']]
    seed = sum(ord(c) for c in area)

    def allowed(point):
        return (boundary is None or inside(point[:2], boundary)) and not any(inside(point[:2], p) for p in avoid)

    # --- Perching flocks ---
    candidates = [c for c in gather_perches(build, ground, build.log) if allowed(c)]
    flocks = plan_flocks(candidates, spec['species'], hob, seed)
    placed_birds = 0
    for n, (centre, members) in enumerate(flocks):
        rng = random.Random(f'{area} flock {n}')
        perches = [struct('FaunaPerch', location=vec(m[:3]), yaw=float(m[3]), kind=getattr(unreal.FaunaPerchKind, m[4].upper()))
                   for m in members]
        # Open ground beside it, to walk and peck on.
        low, high = SPECIES[spec['species']]['ground']
        wanted = rng.randint(low, high)
        tries = 0
        while wanted > 0 and tries < 40:
            tries += 1
            angle = rng.uniform(0.0, 2.0 * math.pi)
            distance = rng.uniform(250.0, 900.0)
            x, y = centre[0] + math.cos(angle) * distance, centre[1] + math.sin(angle) * distance
            spot = open_ground(build, ground, x, y, boxes, ignore) if allowed((x, y)) else None
            if spot and not near_hob((spot[0].x, spot[0].y, spot[0].z), hob):
                point, normal, wander = spot
                perches.append(struct('FaunaPerch', location=unreal.Vector(point.x, point.y, point.z + 0.5), yaw=0.0,
                                      kind=unreal.FaunaPerchKind.GROUND, normal=normal, wander=float(wander)))
                wanted -= 1
        birds_low, birds_high = SPECIES[spec['species']]['birds']
        count = min(len(perches) - 1, rng.randint(birds_low, birds_high))
        if count < 2:
            continue
        actor = place_flock(build, flock_cls, f'Fauna_{spec["species"].title()}s_{n + 1:02d}', spec['species'], perches, count,
                            seed + 101 * n, meshes, missing)
        if actor:
            placed_birds += count
    build.log(f'fauna: {len(flocks)} {spec["species"]} flocks, {placed_birds} birds')

    # --- Aerial: hawks over the land, swallows over the water ---
    if boundary:
        cx, cy = polygon_centroid(boundary)
    else:
        xs = [t[1] for t in ground.tiles] + [t[2] for t in ground.tiles]
        ys = [t[3] for t in ground.tiles] + [t[4] for t in ground.tiles]
        cx, cy = (min(xs) + max(xs)) * 0.5, (min(ys) + max(ys)) * 0.5
    hawks = spec['hawks']
    floor = ground(cx, cy)
    base = floor[0].z if floor else 0.0
    zone = struct('FaunaZone', center=unreal.Vector(cx, cy, base), radius=hawks['radius'], min_height=hawks['low'],
                  max_height=hawks['high'], count=hawks['count'])
    place_flock(build, flock_cls, 'Fauna_Hawks', 'hawk', [], hawks['count'], seed + 7, meshes, missing, mode='AERIAL',
                aerial=dict(center=unreal.Vector(cx, cy, base + hawks['low']), zone=zone))
    ponds = build.layout.get('ponds', {})
    for n, which in enumerate(spec['swallows']):
        if which == 'pond' and 'pond' in ponds:
            pond = ponds['pond']
            (px, py), (rx, ry) = pond['center'], pond['radii']
            centre, radius, z, count = (px, py), max(rx, ry) * 0.9, pond['waterZ'], 5
        elif which == 'wallow':
            pools = [p for k, p in sorted(ponds.items()) if k.startswith('wallow')]
            if not pools:
                continue
            px = sum(p['center'][0] for p in pools) / len(pools)
            py = sum(p['center'][1] for p in pools) / len(pools)
            centre, radius, z, count = (px, py), 900.0, max(p['waterZ'] for p in pools), 3
        else:
            continue
        zone = struct('FaunaZone', center=unreal.Vector(centre[0], centre[1], z), radius=radius, min_height=120.0,
                      max_height=450.0, count=count)
        place_flock(build, flock_cls, f'Fauna_Swallows_{n + 1:02d}', 'swallow', [], count, seed + 13 + n, meshes, missing,
                    mode='AERIAL', aerial=dict(center=unreal.Vector(centre[0], centre[1], z + 200.0), zone=zone))

    # --- Insects ---
    def swarm(label, insect, zones, body, wings=None, swarm_seed=0):
        if swarm_cls is None or not zones:
            return None
        settings = INSECTS[insect]
        body_mesh = mesh_asset(meshes, body, build, missing)
        wing_meshes = [mesh_asset(meshes, w, build, missing) for w in wings] if wings else []
        if body_mesh is None or any(w is None for w in wing_meshes):
            return None
        first = zones[0].get_editor_property('center')
        actor = build.place(swarm_cls, (first.x, first.y, first.z), label=label, folder=FOLDER, tags=(TAG,))
        actor.set_editor_property('kind', getattr(unreal.FaunaInsect, settings['kind']))
        actor.set_editor_property('body_mesh', body_mesh)
        if wing_meshes:
            actor.set_editor_property('wing_mesh_l', wing_meshes[0])
            actor.set_editor_property('wing_mesh_r', wing_meshes[1])
        for key in ('speed', 'flee_radius', 'insect_scale', 'max_active', 'active_radius', 'cull_distance'):
            actor.set_editor_property(key, settings[key])
        if settings.get('shown_in'):
            actor.set_editor_property('shown_in', settings['shown_in'])
        if settings.get('sound_cue'):
            actor.set_editor_property('sound_cue', settings['sound_cue'])
            actor.set_editor_property('sound_range', settings['sound_range'])
        actor.set_editor_property('zones', zones)
        actor.set_editor_property('seed', swarm_seed)
        return actor

    def zone_at(x, y, z, radius, low, high, count):
        if z is None:
            hit = ground(x, y)
            if not hit:
                return None
            z = hit[0].z
        if not allowed((x, y)):
            return None
        return struct('FaunaZone', center=unreal.Vector(x, y, z), radius=radius, min_height=low, max_height=high, count=count)

    flowers = [zone_at(x, y, None, r, 30.0, 170.0, c) for x, y, r, c in flower_zones(build.source, build.layout, area, seed)]
    flowers = [z for z in flowers if z]
    for n, look in enumerate(spec['butterflies']):
        swarm(f'Fauna_Butterflies_{look}', 'butterfly', flowers, 'Butterfly_Body',
              (f'ButterflyWing_{look}L', f'ButterflyWing_{look}R'), seed + 201 + n)
    water = [zone_at(x, y, z, r, 40.0, 140.0, c) for x, y, z, r, c in water_zones(build.layout)]
    water = [z for z in water if z]
    swarm('Fauna_Dragonflies', 'dragonfly', water, 'Dragonfly', swarm_seed=seed + 301)
    if spec['fireflies']:
        glow = [zone_at(x, y, z, r, 40.0, 260.0, c) for x, y, z, r, c in firefly_zones(build.source, build.layout, area)]
        swarm('Fauna_Fireflies', 'firefly', [z for z in glow if z], 'Firefly', swarm_seed=seed + 401)
    # As tuples: the layout's locations are lists, the actors' below tuples, and sorting mixes them.
    fly_spots = [tuple(spot['location']) for key, spot in sorted(build.layout.get('placements', {}).items())
                 if spot['kind'] == 'Outhouse']
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        label = str(actor.get_actor_label())
        if unreal.Name(build.tag) in actor.tags and label.startswith(spec['flies_labels'] or ('\0',)):
            where = actor.get_actor_location()
            fly_spots.append((where.x, where.y, where.z))
    flies = [zone_at(x, y, z, 120.0, 40.0, 180.0, 6) for x, y, z in sorted(fly_spots)]
    swarm('Fauna_Flies', 'fly', [z for z in flies if z], 'Fly', swarm_seed=seed + 501)

    # --- Wind: tumbleweed lanes and dust devils (Ransom's Rest) ---
    world = unreal.EditorLevelLibrary.get_editor_world()
    wind = (math.cos(math.radians(WIND_YAW)), math.sin(math.radians(WIND_YAW)))

    def blocked(a, b, height):
        """Whether anything solid but the ground stands across the line from a to b at height over the ground."""
        hit = unreal.SystemLibrary.line_trace_single(world, vec((a[0], a[1], a[2] + height)), vec((b[0], b[1], b[2] + height)),
                                                     unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignore, unreal.DrawDebugTrace.NONE, True)
        if hit is None:
            return False
        actor = hit.to_tuple()[9]
        return actor is not None and unreal.Name('Ground') not in actor.tags

    if spec['tumbleweeds'] and tumble_cls is not None:
        lanes = []
        for x, y in lane_starts(build.source, build.layout, area, seed):
            if not allowed((x, y)) or in_box((x, y), boxes, 100.0):
                continue
            start = ground(x, y)
            if not start or start[1].z < 0.85:
                continue
            points = [(x, y, start[0].z)]
            clear = 0.0
            step = 300.0
            while clear < LANE_LENGTH:
                nx, ny = points[-1][0] + wind[0] * step, points[-1][1] + wind[1] * step
                hit = ground(nx, ny)
                if not hit or not allowed((nx, ny)) or blocked(points[-1], (nx, ny, hit[0].z), 50.0) or build.in_water((nx, ny)):
                    break
                points.append((nx, ny, hit[0].z))
                clear += step
            if clear >= LANE_MIN_CLEAR:
                lanes.append((clear, points[0], points[-1]))
        lanes.sort(key=lambda lane: (-lane[0], lane[1][:2]))
        lanes = thin(lanes, LANE_SPACING, key=lambda lane: lane[1])[:LANES_MAX]
        mesh = mesh_asset(meshes, 'Tumbleweed', build, missing)
        if lanes and mesh:
            first = lanes[0][1]
            actor = build.place(tumble_cls, first, label='Fauna_Tumbleweeds', folder=FOLDER, tags=(TAG,))
            actor.set_editor_property('mesh', mesh)
            actor.set_editor_property('lanes', [struct('FaunaLane', start=vec(a), end=vec(b)) for _, a, b in lanes])
            actor.set_editor_property('bounce_cue', 'World.Fauna.Tumbleweed.Bounce')
            actor.set_editor_property('seed', seed + 601)
        build.log(f'fauna: {len(lanes)} tumbleweed lanes')

    if spec['dust_roads'] and dust_cls is not None:
        spots = []
        for x, y in dust_starts(build.source, build.layout, area):
            if not allowed((x, y)) or in_box((x, y), boxes, 300.0):
                continue
            hit = ground(x, y)
            if not hit or hit[1].z < 0.9:
                continue
            centre = (x, y, hit[0].z)
            ring = [(x + math.cos(a) * DUST_CLEAR, y + math.sin(a) * DUST_CLEAR) for a in (k * math.pi / 4.0 for k in range(8))]
            if any(blocked(centre, (rx, ry, hit[0].z), 100.0) for rx, ry in ring):
                continue
            spots.append(centre)
        spots = thin(spots, DUST_SPACING)[:DUST_MAX]
        mesh = mesh_asset(meshes, 'DustDevil', build, missing)
        if spots and mesh:
            actor = build.place(dust_cls, spots[0], label='Fauna_DustDevils', folder=FOLDER, tags=(TAG,))
            actor.set_editor_property('mesh', mesh)
            actor.set_editor_property('spots', [struct('FaunaZone', center=vec(s), radius=800.0, min_height=0.0, max_height=0.0, count=1)
                                                for s in spots])
            actor.set_editor_property('whirl_cue', 'World.Fauna.DustDevil')
            actor.set_editor_property('wind_yaw', WIND_YAW)
            actor.set_editor_property('seed', seed + 701)
        build.log(f'fauna: {len(spots)} dust devil spots')

    # --- Washing on the laundry lines ---
    pieces = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if unreal.Name(build.tag) not in actor.tags:
            continue
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh = component.static_mesh
            if mesh is None or not mesh.get_name().startswith('SM_LaundryLine'):
                continue
            transforms = []
            if isinstance(component, unreal.InstancedStaticMeshComponent):
                for i in range(component.get_instance_count()):
                    t = component.get_instance_transform(i, True)
                    t = (t[1] if t[0] else None) if isinstance(t, tuple) else t
                    if t is not None:
                        transforms.append(t)
            else:
                transforms.append(component.get_world_transform())
            for t in transforms:
                yaw = t.rotation.rotator().yaw

                def to_world(local, t=t):
                    w = t.transform_location(unreal.Vector(*local))
                    return (w.x, w.y, w.z)
                pieces.extend(laundry_points(to_world, yaw))
    cloth_meshes = [mesh_asset(meshes, name, build, missing) for name in CLOTH_MESHES]
    if pieces and cloth_cls is not None and all(cloth_meshes):
        actor = build.place(cloth_cls, pieces[0][0], label='Fauna_Washing', folder=FOLDER, tags=(TAG,))
        actor.set_editor_property('meshes', cloth_meshes)
        actor.set_editor_property('pieces', [struct('FaunaClothPiece', mesh=cloth, location=vec(where), yaw=float(yaw))
                                             for where, cloth, yaw in pieces])
        actor.set_editor_property('wind_yaw', WIND_YAW)
        actor.set_editor_property('flap_cue', 'World.Fauna.Cloth.Flap')
        actor.set_editor_property('seed', seed + 801)
    build.log(f'fauna: {len(pieces)} pieces of washing on {len(pieces) // max(1, len(LAUNDRY_PIECES))} lines')
    if missing:
        build.warn(f'fauna: no SM_{", SM_".join(sorted(missing))} yet (import Art/Models/Creatures/AmbientFauna.py): those left out')
    build.log(f'fauna: {len(hob)} of Hob\'s perches kept clear by {HOB_CLEARANCE / 100.0:.0f} m')


def run(name):
    """Places only the fauna again: the area's Fauna folder cleared, placed, the level saved."""
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    build_area.levels.save_current_level()
    build.log('fauna placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_fauna.py <Area> (RansomsRest or TutorialIsland)')
    run(sys.argv[1])
