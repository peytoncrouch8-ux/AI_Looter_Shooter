"""What happens between the story's fights (Docs/Polish/BorderlandsComparison.md, item 7), for build_area.py's gameplay
build: the layout's gameplay.encounters (Art/Levels/<Area>/layout.json) as encounters in the level, so the walks between
the scripted fights aren't empty. Each waits for the story (its 'after': the missions turned in before it's on), keeps
off the safe zones, the arrivals and the scripted fights' grounds (Looter.Encounters.Camps.Placement checks the layout;
this checks the level as built), and, like every encounter, starts over when the level loads. It uses build_area_story's
helpers and build_area_caches's crate placing. Everything goes in the area's Gameplay folder, so a "gameplay" build
places it all again; a piece whose class or model isn't there yet is left out with a warning.

- Camps (gameplay.encounters.camps): an AEncounterSpawner Camp_<id> on the camp's middle, its creatures standing about
  at its spots (relative to it) until they notice the player: a few Basic and a Restless leader, tagged Camp_<id>,
  hunting on the camp's ground (a radius round it or a polygon, its height band). Then its Supply Crate (AChest, kind
  SupplyCrate, id CampCache_<id>, tagged Chest, CampCache and Obstacle): the first of its spots that stands level and
  clear, as Ruth's caches are judged (build_area_caches.Site: clear of the dressing, the roads, every encounter's spots
  placed so far, the boundary's edge); a tucked spot stands its back against a wall.
- Patrols (gameplay.encounters.patrols): an APatrolSpawner Patrol_<id> on the route's middle, its route relative to it
  (each point on the ground), there and back (or round, 'loop'), resting 'pause' seconds at each end; its ground the
  road's corridor, 'corridor' cm either side of the route and past its ends.
- Ambushes (gameplay.encounters.ambushes): an AAmbushSpawner Ambush_<id> at the middle of its spots, sprung as the player
  walks onto its trigger (an obstacle's polygon: the churchyard's fence, the Webwood), its creatures arriving as its
  'entrance' says (Rise: the dead fade up out of the ground; Drop: from 'dropHeight' over their feet) and coming at once.

Main runs it from build_area.py's gameplay(), after the story's pieces (the Webwood's trees and the caches stand first):
    importlib.reload(importlib.import_module('build_area_camps')).place(self)
"""
import importlib
import math

import unreal

import build_area
import build_area_story as story

# The kinds of encounter, and the tags and ids they carry (the tests and the console find them by these).
SPAWNER = 'EncounterSpawner'
PATROL = 'PatrolSpawner'
AMBUSH = 'AmbushSpawner'
CRATE_TAG = 'CampCache'
CRATE_KIND = 'SupplyCrate'

RANKS = {'Basic': 'BASIC', 'Rare': 'RARE', 'Epic': 'EPIC'}
ENTRANCES = {'Appear': 'APPEAR', 'Rise': 'RISE', 'Drop': 'DROP'}

# A spot whose ground is more than this (cm) above or below its encounter's middle is warned of: the spawner skips it
# in play (its max_ground_step, 4 m unless the layout says).
DEFAULT_STEP = 400.0
# Where the build looks for the ground under a spot: this far over and under its encounter's middle's ground (cm), so a
# knoll's slope is found and the canyon under the Rim isn't.
LOOK_REACH = 1500.0


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def group(cls, count, rank):
    """FEncounterGroup: count of cls, Fixed at a rank ('Basic', 'Rare'), or the area's promotions ('Area')."""
    made = unreal.EncounterGroup()
    made.set_editor_property('creature_class', cls)
    made.set_editor_property('count', count)
    if rank == 'Area':
        made.set_editor_property('rank_roll', unreal.EncounterRankRoll.AREA)
    else:
        made.set_editor_property('rank_roll', unreal.EncounterRankRoll.FIXED)
        made.set_editor_property('rank', getattr(unreal.CreatureRank, RANKS[rank]))
    return made


def groups(build, entry):
    """The entry's groups, or None (with a warning) when a class isn't built."""
    made = []
    for each in entry['groups']:
        cls = story.actor_class(each['class'])
        if cls is None:
            build.warn(f'no {each["class"]} class (build the game module first): no {entry["id"]}')
            return None
        made.append(group(cls, each['count'], each['rank']))
    return made


# The level's terrain tiles (build_area.terrain_tiles), found once a build: the ground is traced on them alone, so a dead
# tree's limb over a Webwood spot or the fold's wall top never stands in for it.
_TILES = {}


def tiles(build):
    if build.tag not in _TILES:
        _TILES[build.tag] = build_area.terrain_tiles(build.tag)
        if not _TILES[build.tag]:
            build.warn('no terrain tiles tagged Ground (build the whole level first): the encounters stand on whatever traces find')
    return _TILES[build.tag]


def ground_z(build, x, y, near=None):
    """The terrain's height under (x, y): within LOOK_REACH over and under near (its encounter's middle's ground) when
    given, so a knoll's slope is found and the canyon under the Rim isn't; or None when there's none there. Without
    terrain tiles, the first thing a trace meets."""
    top = 20000.0 if near is None else near + LOOK_REACH
    bottom = -20000.0 if near is None else near - LOOK_REACH
    if tiles(build):
        hit = build_area.terrain_hit(tiles(build), unreal.Vector(x, y, top), unreal.Vector(x, y, bottom))
        return None if hit is None else hit.z
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, top), unreal.Vector(x, y, bottom),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    return None if hit is None else hit.to_tuple()[5].z


def polygon_of(build, ground):
    """A ground's or trigger's polygon (layout cm): its own, or an obstacle's or zone's by id. None for a radius."""
    if 'polygon' in ground:
        return ground['polygon']
    for section, key in (('obstacles', 'obstacle'), ('zones', 'zone')):
        if key in ground:
            entry = story.layout_entry(build, section, ground[key])
            return entry['polygon'] if entry else None
    return None


def set_ground(build, spawner, ground, z):
    """The encounter's hunting ground: a polygon (its own, or an obstacle's), or a radius round it; its height band, its
    margin past the polygon's edge, how far a spot may be above or below its middle."""
    corners = polygon_of(build, ground)
    if corners:
        spawner.set_editor_property('ground_corners', [unreal.Vector(x, y, z) for x, y in corners])
    if 'radius' in ground:
        spawner.set_editor_property('give_up_radius', float(ground['radius']))
    if 'margin' in ground:
        spawner.set_editor_property('ground_margin', float(ground['margin']))
    if 'maxRise' in ground:
        spawner.set_editor_property('ground_max_rise', float(ground['maxRise']))
    if 'maxStep' in ground:
        spawner.set_editor_property('max_ground_step', float(ground['maxStep']))


def check_levels(build, label, middle, spots, step):
    """Warns of spots whose ground is off the middle's level (the spawner skips them in play) or missing."""
    mx, my, mz = middle
    off = []
    for x, y in spots:
        z = ground_z(build, x, y, mz)
        if z is None:
            off.append(f'({x:.0f}, {y:.0f}) no ground')
        elif abs(z - mz) > step:
            off.append(f'({x:.0f}, {y:.0f}) {(z - mz) / 100.0:+.1f} m')
    if off:
        build.warn(f'{label}: spots off its level of ground (more than {step / 100.0:.1f} m from its middle): {", ".join(off)}')


def spawner(build, kind, label, x, y, entry, tag):
    """An encounter of kind at (x, y) on the ground: its id, groups, tag, story. Returns (actor, z), or (None, z)."""
    cls = story.actor_class(kind)
    made_groups = groups(build, entry)
    z = ground_z(build, x, y)
    if z is None:
        build.warn(f'no terrain under {label}\'s middle ({x:.0f}, {y:.0f}): it stands at 0 (is the terrain placed?)')
        z = 0.0
    if cls is None or made_groups is None:
        if cls is None:
            build.warn(f'no {kind} class (build the game module first): no {label}')
        return None, z
    made = build.place(cls, (x, y, z + 50.0), 0.0, label=f'Encounter_{label}', folder='Gameplay')
    made.set_editor_property('spawner_id', unreal.Name(label))
    made.set_editor_property('groups', made_groups)
    made.set_editor_property('creature_tags', [unreal.Name(tag)])
    made.set_editor_property('active_when', story.condition(after=entry.get('after', ())))
    return made, z


def relative(build, points, x, y, z):
    """Points (layout cm) relative to an encounter whose ground is at (x, y, z), each on its own ground (level with the
    encounter's where none is found). A spawner's spots need only their x and y; a patrol's route walks at its heights."""
    made = []
    for px, py in points:
        pz = ground_z(build, px, py, z)
        made.append(unreal.Vector(px - x, py - y, (z if pz is None else pz) - z))
    return made


def corridor(route, width):
    """A polygon width cm either side of a route (and width past its ends), corners in order round it."""
    left, right = [], []
    count = len(route)
    for i, (px, py) in enumerate(route):
        a, b = (route[i], route[i + 1]) if i < count - 1 else (route[i - 1], route[i])
        dx, dy = b[0] - a[0], b[1] - a[1]
        length = math.hypot(dx, dy) or 1.0
        nx, ny = -dy / length * width, dx / length * width
        ex = ey = 0.0
        if i == 0:
            ex, ey = -dx / length * width, -dy / length * width
        elif i == count - 1:
            ex, ey = dx / length * width, dy / length * width
        left.append((px + nx + ex, py + ny + ey))
        right.insert(0, (px - nx + ex, py - ny + ey))
    return left + right


def route_middle(route):
    """The point half way along a route (layout cm)."""
    legs = [math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(route, route[1:])]
    left = sum(legs) * 0.5
    for (a, b), length in zip(zip(route, route[1:]), legs):
        if left <= length and length > 0.0:
            t = left / length
            return a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
        left -= length
    return tuple(route[-1])


# ---------------------------------------------------------------------------
# Camps, patrols, ambushes
# ---------------------------------------------------------------------------

def place_camp(build, camp):
    """A camp's encounter: its creatures standing about at its spots until they notice the player. Returns its crate's
    spots, for after every encounter is placed (the crates keep clear of their spots)."""
    label = f'Camp_{camp["id"]}'
    x, y = camp['center']
    made, z = spawner(build, SPAWNER, label, x, y, camp, label)
    if made is None:
        return None
    made.set_editor_property('spawn_points', relative(build, camp["spots"], x, y, z))
    made.set_editor_property('hunt_on_spawn', False)
    set_ground(build, made, camp['ground'], z)
    step = float(camp['ground'].get('maxStep', DEFAULT_STEP))
    check_levels(build, label, (x, y, z), camp['spots'], step)
    build.log(f'{label} ({camp["where"]}): {", ".join(str(g["count"]) + " " + g["rank"] + " " + g["class"] for g in camp["groups"])}'
              f' at ({x:.0f}, {y:.0f}), after {", ".join(camp.get("after", ())) or "the start"}')
    return label, camp


def place_patrol(build, patrol):
    """A patrol's encounter on the middle of its route: its route relative to it, its ground the road's corridor."""
    label = f'Patrol_{patrol["id"]}'
    route = patrol['route']
    x, y = route_middle(route)
    made, z = spawner(build, PATROL, label, x, y, patrol, label)
    if made is None:
        return
    made.set_editor_property('patrol_route', relative(build, route, x, y, z))
    made.set_editor_property('loop', bool(patrol.get('loop', False)))
    made.set_editor_property('pause_seconds', float(patrol.get('pause', 3.0)))
    set_ground(build, made, {'polygon': corridor(route, float(patrol['corridor'])), 'maxRise': patrol.get('maxRise', 800.0)}, z)
    check_levels(build, label, (x, y, z), route, float(patrol.get('maxRise', 800.0)))
    length = sum(math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(route, route[1:]))
    build.log(f'{label} ({patrol["where"]}): {length / 100.0:.0f} m of road, {"round" if patrol.get("loop") else "there and back"}, '
              f'after {", ".join(patrol.get("after", ())) or "the start"}')


def place_ambush(build, ambush):
    """An ambush's encounter at the middle of its spots, sprung by the player walking onto its trigger's ground."""
    label = f'Ambush_{ambush["id"]}'
    spots = ambush['spots']
    x = sum(p[0] for p in spots) / len(spots)
    y = sum(p[1] for p in spots) / len(spots)
    made, z = spawner(build, AMBUSH, label, x, y, ambush, label)
    if made is None:
        return
    made.set_editor_property('spawn_points', relative(build, spots, x, y, z))
    trigger = polygon_of(build, ambush['trigger'])
    if not trigger:
        build.warn(f'{label}: no trigger polygon ({ambush["trigger"]}): it springs on approach instead')
    else:
        made.set_editor_property('ambush_corners', [unreal.Vector(px, py, z) for px, py in trigger])
    if 'maxRise' in ambush['trigger']:
        made.set_editor_property('ambush_max_rise', float(ambush['trigger']['maxRise']))
    made.set_editor_property('entrance', getattr(unreal.AmbushEntrance, ENTRANCES[ambush.get('entrance', 'Appear')]))
    if 'dropHeight' in ambush:
        made.set_editor_property('drop_height', float(ambush['dropHeight']))
    set_ground(build, made, ambush['ground'], z)
    check_levels(build, label, (x, y, z), spots, float(ambush['ground'].get('maxStep', DEFAULT_STEP)))
    build.log(f'{label} ({ambush["where"]}): {ambush.get("entrance", "Appear")}, sprung on its ground, after '
              f'{", ".join(ambush.get("after", ())) or "the start"}')


def place_crates(build, camps):
    """Each camp's Supply Crate, judged as Ruth's caches are (after every encounter is placed: clear of their spots)."""
    caches = importlib.reload(importlib.import_module('build_area_caches'))
    cls = story.actor_class('Chest')
    if cls is None:
        build.warn('no Chest class (build the game module first): the camps have no crates')
        return
    meshes = caches.models(build, CRATE_KIND)
    if meshes is None:
        return
    site = caches.Site(build)
    for label, camp in camps:
        crate_id = f'CampCache_{camp["id"]}'
        chosen = caches.choose(site, [(sx, sy, syaw, bool(tucked)) for sx, sy, syaw, tucked in camp['crate']])
        if chosen is None:
            build.warn(f'no terrain under any of {crate_id}\'s spots: {label} has no crate')
            continue
        i, x, y, z, yaw, spread, problems = chosen
        caches.place_chest(build, cls, CRATE_KIND, meshes, crate_id, (x, y, z), yaw, (CRATE_TAG,))
        where = f'{crate_id} at ({x:.0f}, {y:.0f}, {z:.0f}) facing {yaw % 360.0:.0f}, spot {i + 1} of {len(camp["crate"])}'
        if problems:
            build.warn(f'{where}: {"; ".join(problems)} (no spot was level and clear: look at it, and retune its crate spots)')
        else:
            build.log(f'{where}, level within {spread:.0f} cm')


def place(build):
    """Every camp, patrol and ambush of the layout's gameplay.encounters, then the camps' crates. A layout without any
    places nothing."""
    encounters = build.settings.get('encounters')
    if not encounters:
        return
    # The terrain as it stands now (a rebuilt level has new tiles).
    _TILES.clear()
    camps = [made for made in (place_camp(build, camp) for camp in encounters.get('camps', [])) if made]
    for patrol in encounters.get('patrols', []):
        place_patrol(build, patrol)
    for ambush in encounters.get('ambushes', []):
        place_ambush(build, ambush)
    place_crates(build, camps)
    build.log(f'between the fights: {len(encounters.get("camps", []))} camps, {len(encounters.get("patrols", []))} patrols, '
              f'{len(encounters.get("ambushes", []))} ambushes')
