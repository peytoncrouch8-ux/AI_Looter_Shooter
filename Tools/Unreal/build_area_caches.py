"""Step 26, the Ranger caches (Docs/Areas/RansomsRest.md: Missions, "Optional: Ranger Caches"; "Loot and chests"), for
build_area_story.py: Ruth Calder's three Supply Crates and the gang's Strongbox (AChest, Source/.../Loot/Chest.h),
placed from the layout (the windmill's placement, the Sink, the bluff path), the placed sheriff's office's socket and the
terrain's tiles, so they follow the level whenever it's rebuilt. build_area_story's place() calls place() here (reloaded
each run, as the editor keeps modules between runs); it uses build_area_story's helpers. Everything goes in the area's
Gameplay folder, so a "gameplay" build places it all again; a kind whose models aren't imported yet (/Game/Art/Loot, from
Art/Models/Loot/Chests.py) is left out with a warning, and so is everything while the Chest class isn't built.

- Ruth's three Supply Crates (kind SupplyCrate: one gun at Luck 0.5 and two ammo pickups of 36 rounds), the caches her
  note on the Rim Rangers' board names ("UNDER THE WINDMILL - SINK RIM - BLUFF PATH"), tagged Chest, RangerCache and
  Obstacle, each with a stable id (its label too):
  - RangerCache_Windmill, under the windmill: at its tower's foot behind its legs, under the tail vane, so the tower
    stands between it and the barn yard's gate; its front toward the yard's south fence.
  - RangerCache_SinkRim, on the Sink's rim: on the east rim where the south warning fence ends against Den Rock's west
    flank, its back to the rock, across the pit from the ramp head and well off the Sink road.
  - RangerCache_BluffPath, on the bluff path: where the path tops out on Ransom's Point, tucked against the east lip a
    few metres north of it, behind the right shoulder of a player coming up. The path itself climbs about 1 in 4, too
    steep for a crate to stand level on.
  Each has a few spots in order of preference (CACHES). The first where the crate stands level (the terrain under its
  corners within LEVEL cm, no drop within a hand of it) and clear wins: nothing solid in its footprint or a hand round
  it (traced: a rock, a tree, a fence, a cliff piece, the scatter), none of the dressing's pieces
  (build_area_dressing.footprints()), clear of the roads, of the encounters' spots and of the playable boundary's edge.
  Without such a spot, the level spot with the fewest problems is taken, else the flattest, with a warning naming them.
  It stands level, its pivot on the terrain (the tiles alone, never a rock or the scatter).
- The gang's Strongbox (kind Strongbox: two guns at Luck 1.0; id GangStrongbox, tagged Chest, GangStrongbox and
  Obstacle) on the sheriff's office's floor at the placed FalseFront_Sheriff's SOCKET_Strongbox (FalseFronts.py's
  walk-in front office), facing as the socket faces (the open door). Without that socket, none: no spot outside stands
  in for the office.
They open from the start, with no story condition: the doc gives none, and Calder's note only says where they are.
"""
import importlib
import math

import unreal

import build_area
import build_area_story as story

LOOT = '/Game/Art/Loot/'
# Each kind's models (Chests.py): the body, the lid on its SOCKET_Lid, the wheel on its SOCKET_Wheel.
MODELS = {
    'SupplyCrate': ('SM_SupplyCrate', 'SM_SupplyCrate_Lid'),
    'Strongbox': ('SM_Strongbox', 'SM_Strongbox_Lid', 'SM_Strongbox_Wheel'),
}
PARTS = ('body', 'lid', 'wheel')
KIND_ENUM = {'SupplyCrate': 'SUPPLY_CRATE', 'Strongbox': 'STRONGBOX'}

# The tags and ids, as the C++ (AChest::ChestTag) and the tests (RangerCachesTests.cpp) name them.
CHEST_TAG = 'Chest'
CACHE_TAG = 'RangerCache'
STRONGBOX_ID = 'GangStrongbox'
OBSTACLE_TAG = 'Obstacle'

# What they stand on or by: the placements' kinds (layout.json) and the office's floor socket.
WINDMILL = 'Windmill'
SHERIFF = 'FalseFront_Sheriff'
STRONGBOX_SOCKET = 'Strongbox'

# A Supply Crate's footprint, halves in cm (Chests.py: 0.52 m deep along its X, its front +X; 1.15 m along its Y).
CRATE_HALF = (26.0, 57.5)
# Kept clear round it: a hand's width past its sides, which also takes the lid's swing (21 cm past its hinge, behind).
MARGIN = 30.0
# Level: its corners' terrain within this (cm); a drop: the terrain this far under it within the margin (a lip, the pit).
LEVEL = 8.0
DROP = 40.0
# Something solid: a probe's first hit this far over the terrain (cm). Probes start this far up: under tree crowns, the
# windmill's platform and head, so what hangs well over a crate doesn't count.
SOLID_OVER = 6.0
PROBE_FROM = 250.0
# Clear of a road's edge, an encounter's spot, the playable boundary's edge (cm).
ROAD_CLEAR = 150.0
SPOT_CLEAR = 250.0
EDGE_CLEAR = 300.0

# Ruth's caches: (id, where in words, the placement kind its spots are in the frame of (None: the level's own, layout cm),
# spots in order of preference as (x, y, yaw): yaw is where the crate's front faces, its back the way it's tucked).
CACHES = (
    ('RangerCache_Windmill', 'under the windmill', WINDMILL, (
        # Behind the tower (its -X, under the tail vane): the tower stands between it and the yard's gate; 1.9 m out,
        # clear of the stone footings (1.53 m) and of the leg hull with the lid open.
        (-190.0, 0.0, 180.0),
        # Its right side, toward the yard's south-east corner.
        (0.0, 190.0, 90.0),
        # Its left side, toward the barn.
        (0.0, -190.0, -90.0),
    )),
    ('RangerCache_SinkRim', "on the Sink's rim", None, (
        # The east rim where the south warning fence ends (2 m off) against Den Rock's west flank (3.4 m), its back to the
        # rock, 4.6 m back from the pit's edge.
        (3600.0, 8600.0, -135.0),
        # A little further along the rock's flank.
        (3550.0, 8750.0, -150.0),
        # The north-east rim between Den Rock and the Webwood, past the north fence's end.
        (6000.0, 8550.0, -125.0),
    )),
    ('RangerCache_BluffPath', 'on the bluff path', None, (
        # Where the path tops out (-7882, -7208): 5 m north of it on the top, 2.4 m in from the east lip, facing in.
        (-7450.0, -7380.0, -95.0),
        # A little further north along the lip.
        (-7250.0, -7360.0, -95.0),
        # South of where it tops out, by the lip.
        (-8450.0, -7420.0, -120.0),
        # The path's foot on the farm terrace, against the Rimrock's south piece.
        (-5271.0, -10970.0, 20.0),
    )),
)


# ---------------------------------------------------------------------------
# Plain geometry
# ---------------------------------------------------------------------------

def frame(x, y, yaw, along_x, along_y):
    """A point along_x out of the front and along_y to the right of something at (x, y) turned yaw."""
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return x + c * along_x - s * along_y, y + s * along_x + c * along_y


def corners(x, y, yaw, half_x, half_y):
    return [frame(x, y, yaw, sx * half_x, sy * half_y) for sx, sy in ((1, 1), (-1, 1), (-1, -1), (1, -1))]


def overlap(a, b):
    """Two convex quads (corner lists) overlap, seen from above (separating axes)."""
    for quad in (a, b):
        for i in range(len(quad)):
            (x1, y1), (x2, y2) = quad[i], quad[(i + 1) % len(quad)]
            nx, ny = y2 - y1, x1 - x2
            pa = [nx * px + ny * py for px, py in a]
            pb = [nx * px + ny * py for px, py in b]
            if max(pa) < min(pb) or max(pb) < min(pa):
                return False
    return True


def segment_distance(p, a, b):
    dx, dy = b[0] - a[0], b[1] - a[1]
    length = dx * dx + dy * dy
    t = 0.0 if length == 0.0 else max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / length))
    return math.hypot(p[0] - (a[0] + t * dx), p[1] - (a[1] + t * dy))


def line_distance(p, points, closed=False):
    pairs = list(zip(points, points[1:])) + ([(points[-1], points[0])] if closed else [])
    return min(segment_distance(p, a, b) for a, b in pairs) if pairs else float('inf')


# ---------------------------------------------------------------------------
# In the editor
# ---------------------------------------------------------------------------

class Site:
    """What a cache's spot is judged against: the terrain's tiles, the dressing's pieces, the roads, the encounters' spots
    and the playable boundary."""

    def __init__(self, build):
        self.build = build
        self.tiles = build_area.terrain_tiles(build.tag)
        if not self.tiles:
            build.warn('no terrain tiles tagged Ground (build the whole level first): the caches can\'t be seated')
        # Reloaded, as the editor keeps modules between runs: a gameplay build may come after the dressing's tables changed.
        dressing = importlib.reload(importlib.import_module('build_area_dressing'))
        self.pieces = dressing.footprints(build.source, build.layout['placements'])
        self.roads = [([p[:2] for p in road['points']], road['width']) for road in build.layout.get('roads', {}).values()]
        self.boundary = [c[:2] for c in build.layout.get('boundary', {}).get('corners', [])]
        self.spots = encounter_spots()

    def terrain(self, x, y):
        """The terrain's height under (x, y) (its tiles alone), or None off them."""
        hit = build_area.terrain_hit(self.tiles, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -20000.0))
        return None if hit is None else hit.z

    def judge(self, x, y, yaw):
        """(the terrain's height at its pivot, how far its corners' ground differs, the problems in words) for a crate at
        (x, y) facing yaw. The height is None off the terrain."""
        hx, hy = CRATE_HALF
        under = [self.terrain(*frame(x, y, yaw, fx * hx, fy * hy)) for fx in (-1, 0, 1) for fy in (-1, 0, 1)]
        if any(z is None for z in under):
            return None, float('inf'), ['not on the terrain']
        z = self.terrain(x, y)
        spread = max(under) - min(under)
        problems = []
        if spread > LEVEL:
            problems.append(f'not level ({spread:.0f} cm across it)')
        world = unreal.EditorLevelLibrary.get_editor_world()
        solid = drops = 0
        for fx in (-1.0, 0.0, 1.0):
            for fy in (-1.0, -0.5, 0.0, 0.5, 1.0):
                px, py = frame(x, y, yaw, fx * (hx + MARGIN), fy * (hy + MARGIN))
                ground = self.terrain(px, py)
                if ground is None or ground < z - DROP:
                    drops += 1
                    continue
                hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(px, py, ground + PROBE_FROM),
                                                             unreal.Vector(px, py, ground - 50.0),
                                                             unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
                                                             unreal.DrawDebugTrace.NONE, True)
                if hit is not None and hit.to_tuple()[5].z > ground + SOLID_OVER:
                    solid += 1
        if drops:
            problems.append(f'a drop within {MARGIN:.0f} cm of it')
        if solid:
            problems.append(f'something solid in its footprint or round it ({solid} of 15 probes)')
        mine = corners(x, y, yaw, hx + MARGIN, hy + MARGIN)
        pieces = [p for p in self.pieces if math.hypot(p[0] - x, p[1] - y) < 2000.0
                  and overlap(mine, corners(p[0], p[1], p[2], p[4], p[3]))]
        if pieces:
            problems.append(f'on {len(pieces)} of the dressing\'s pieces')
        near_road = min((line_distance((x, y), points) - width * 0.5 for points, width in self.roads), default=float('inf'))
        if near_road < ROAD_CLEAR + hy:
            problems.append(f'{max(near_road, 0.0):.0f} cm from a road')
        near_spot = min((math.hypot(sx - x, sy - y) for sx, sy in self.spots), default=float('inf'))
        if near_spot < SPOT_CLEAR:
            problems.append(f'{near_spot:.0f} cm from an encounter\'s spot')
        if self.boundary:
            if not build_area.inside((x, y), self.boundary):
                problems.append('outside the playable boundary')
            elif line_distance((x, y), self.boundary, closed=True) < EDGE_CLEAR:
                problems.append('by the playable boundary\'s edge')
        return z, spread, problems


def encounter_spots():
    """Where the level's encounters bring their creatures (layout cm): the spawners placed so far (their own spots and
    middles), and Side 2's hired hands, whose spawner build_area_whitlock.py places after this."""
    spots = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if actor.get_class().get_name() != 'EncounterSpawner':
            continue
        at = actor.get_actor_location()
        spots.append((at.x, at.y))
        transform = actor.get_actor_transform()
        for point in actor.get_editor_property('spawn_points'):
            world = transform.transform_location(point)
            spots.append((world.x, world.y))
    try:
        whitlock = importlib.import_module('build_area_whitlock')
        spots += [(x, y) for x, y in getattr(whitlock, 'HANDS_SPOTS', ())]
        if hasattr(whitlock, 'HANDS_AT'):
            spots.append(tuple(whitlock.HANDS_AT))
    except ImportError:
        pass
    return spots


def models(build, kind):
    """The kind's models (body, lid and wheel), or None with a warning while any isn't imported."""
    paths = [LOOT + name for name in MODELS[kind]]
    missing = [path for path in paths if not unreal.EditorAssetLibrary.does_asset_exist(path)]
    if missing:
        build.warn(f'no {", ".join(missing)} yet (import Art/Models/Loot/Chests.py with Tools/models.ps1): no {kind}')
        return None
    return [unreal.load_asset(path) for path in paths]


def place_chest(build, cls, kind, meshes, chest_id, location, yaw, tags):
    """A chest of kind at location facing yaw, standing level, with its models and id."""
    chest = build.place(cls, location, yaw, label=chest_id, folder='Gameplay',
                        tags=(CHEST_TAG,) + tuple(tags) + (OBSTACLE_TAG,))
    for part, mesh in zip(PARTS, meshes):
        chest.get_editor_property(part).set_static_mesh(mesh)
    chest.set_editor_property('kind', getattr(unreal.ChestKind, KIND_ENUM[kind]))
    # Set last, so the editor builds it again with its models and puts the lid and wheel on their sockets.
    chest.set_editor_property('chest_id', unreal.Name(chest_id))
    return chest


def anchor(build, kind):
    """(x, y, yaw) of the placed model of a placement kind (labelled with its key), else of its layout placement, or None."""
    actor = story.placed(build, kind)
    if actor is not None:
        at = actor.get_actor_location()
        return at.x, at.y, actor.get_actor_rotation().yaw
    spot = next((s for s in build.layout['placements'].values() if s['kind'] == kind), None)
    return None if spot is None else (spot['location'][0], spot['location'][1], spot['yaw'])


def choose(site, spots):
    """The first spot that's level and clear, else the level one with the fewest problems, else the flattest: (index,
    x, y, z, yaw, spread, problems), or None when none is on the terrain."""
    judged = []
    for i, (x, y, yaw) in enumerate(spots):
        z, spread, problems = site.judge(x, y, yaw)
        if z is None:
            judged.append((i, x, y, z, yaw, spread, problems))
            continue
        if not problems:
            return i, x, y, z, yaw, spread, problems
        judged.append((i, x, y, z, yaw, spread, problems))
    seated = [j for j in judged if j[3] is not None]
    if not seated:
        return None
    level = [j for j in seated if j[5] <= LEVEL]
    if level:
        return min(level, key=lambda j: (len(j[6]), j[0]))
    return min(seated, key=lambda j: j[5])


def place_caches(build, cls, site):
    meshes = models(build, 'SupplyCrate')
    if meshes is None:
        return
    for cache_id, words, relative_to, spots in CACHES:
        if relative_to:
            base = anchor(build, relative_to)
            if base is None:
                build.warn(f'the layout has no {relative_to}: no cache {words}')
                continue
            bx, by, byaw = base
            spots = [frame(bx, by, byaw, lx, ly) + (byaw + lyaw,) for lx, ly, lyaw in spots]
        chosen = choose(site, spots)
        if chosen is None:
            build.warn(f'no terrain under any of {cache_id}\'s spots: no cache {words}')
            continue
        i, x, y, z, yaw, spread, problems = chosen
        place_chest(build, cls, 'SupplyCrate', meshes, cache_id, (x, y, z), yaw, (CACHE_TAG,))
        where = f'{cache_id} {words} at ({x:.0f}, {y:.0f}, {z:.0f}) facing {yaw % 360.0:.0f}, spot {i + 1} of {len(spots)}'
        if problems:
            build.warn(f'{where}: {"; ".join(problems)} (no spot was level and clear: look at it, and retune CACHES)')
        else:
            build.log(f'{where}, level within {spread:.0f} cm')


def place_strongbox(build, cls):
    office = story.placed(build, SHERIFF)
    floor = story.socket(office, STRONGBOX_SOCKET) if office is not None else None
    if floor is None:
        if office is not None:
            build.warn(f'{office.get_actor_label()} has no SOCKET_{STRONGBOX_SOCKET} yet (FalseFronts.py\'s walk-in front '
                       'office): no Strongbox')
        return
    meshes = models(build, 'Strongbox')
    if meshes is None:
        return
    at = floor.translation
    yaw = floor.rotation.rotator().yaw
    place_chest(build, cls, 'Strongbox', meshes, STRONGBOX_ID, (at.x, at.y, at.z), yaw, (STRONGBOX_ID,))
    build.log(f'the gang\'s Strongbox ({STRONGBOX_ID}) on the sheriff\'s office\'s floor at ({at.x:.0f}, {at.y:.0f}, '
              f'{at.z:.0f}) facing {yaw % 360.0:.0f}')


def place(build):
    """Ruth's caches and the gang's Strongbox, in the Gameplay folder (build_area_story's place() calls this, after the
    den). Only a level whose layout has the sheriff's office (Ransom's Rest) gets them."""
    if not any(spot['kind'] == SHERIFF for spot in build.layout['placements'].values()):
        return
    cls = story.actor_class('Chest')
    if cls is None:
        build.warn('no Chest class (build the game module first): no Ranger caches, no Strongbox')
        return
    place_caches(build, cls, Site(build))
    place_strongbox(build, cls)
