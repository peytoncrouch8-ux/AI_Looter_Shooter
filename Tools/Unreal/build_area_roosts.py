"""Hob's roosts (World/RespawnMarker: ARespawnMarker), from the layout's gameplay.roosts, for build_area.py's gameplay pass
(build_area_story.py calls it, where the family plot's grave marker was placed), or on their own in the open level:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_roosts.py RansomsRest"
which replaces every roost in the level (the build's and any placed by hand; a trip's landing marker stays) with the
layout's, and saves the level: run twice, it leaves the same ones.

The user's call (2026-10-13): Hob's crow roosts replace the respawn graves as the one save, respawn and fast-travel point on
every map, at landmarks and roads (the graves were "in odd locations"); the roost is the Crow's Mast (look sheet 41, design
B: SM_HobRoost, its bell SM_HobRoostBell). The user's call (2026-10-14): two a map at most (Ransom's Rest's the Depot by the
train station and Calder's Post up the mountain), the others removed, not kept for old saves (the game reads a save
naming a gone roost as its newest find still standing: ARespawnMarker::FindLastFound); and (2026-10-23, "Town roost", as
deaths in town woke at the farm) a third in town by the sheriff's office (SheriffsOffice): three a map now (MOST_PER_MAP),
a new id so no old save finds it (MainStreet was a removed one). A death wakes the
player at the roost they last found, before any at the level's own start. Each roost comes from one entry of
gameplay.roosts (level cm, X north, Y east, as every layout point is):
  { "id": "Depot", "name": "The Depot", "at": [750, 10250], "yaw": 135, "onRoad": false,
    "openAfter": "Main4", "markAfter": "Main3", "startOpen": false, "note": "..." }
  - id: its MarkerId, the save data's name; never change an id once a session may have saved it (the roosts that
    replaced graves kept the graves' ids, so old saves' marked graves were found roosts).
  - name: what the map and the ROOST FOUND banner call it.
  - at: where the mast stands; yaw: the way the player faces on waking there (degrees: 0 north, 90 east). The player's spot
    (the actor) is the model's Wake socket from the mast (FALLBACK_WAKE before the model is imported, the same numbers as
    World/RoostKit.h): 1.7 m out in front of the mast, facing away from it, so face the yaw out over the land.
  - openAfter (ActiveAfterMission), markAfter (MarkAfterMission), startOpen (bStartActive): optional; without them a roost
    opens when it's found, any time.
  - fastTravel: false makes it a save point and death-respawn only, no fast-travel destination (default true; the Depot
    and Calder's Post are the stations, the user's call 2026-10-27).
  - onRoad: the mast may stand in a road (as Main Street's did, beside the town memorial); every other keeps off roads.
Each spot is checked: inside the playable boundary; the mast and the player's spot on walkable ground (tagged Ground) level
enough under the cairn and the feet; clear of the layout's roads, obstacles, placements, the stand-ins' boxes, pits and rock
knobs; nothing solid already standing at either (the dressing's fences and props, the story's pieces). A spot that fails is
moved to the nearest that passes, in rings round it up to MOVE_MOST (logged with how far); one with nowhere to go is left out
with a warning. Each is tagged Obstacle (the scatter keeps off it: run build_island_scatter.py after it; the minimap draws its
mast) and Roost, in the area's Gameplay/Roosts folder.

Hook for other areas (Ransom's Rest's runs from build_area_story.py): in build_area.py's gameplay(), after
self.story.place(self):
    importlib.reload(importlib.import_module('build_area_roosts')).place(self)
"""
import importlib
import json
import math
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
CLASSES = '/Script/AI_Looter_Shooter.'
FOLDER = 'Gameplay/Roosts'
ROOST_TAG = 'Roost'
OBSTACLE_TAG = 'Obstacle'

# The user's rules: two roosts a map at most (2026-10-14), and a town roost on top (2026-10-23): three; a layout with more
# places its first three and warns.
MOST_PER_MAP = 3

# The models (Art/Models/Props/HobRoost.py: the Crow's Mast and its bell); a roost without them stands as the C++ greybox.
ROOST_MESH = '/Game/Art/Props/SM_HobRoost'
BELL_MESH = '/Game/Art/Props/SM_HobRoostBell'
# The model's Wake socket before it's imported (World/RoostKit.h FallbackWake: change both together): where the player stands
# in the mast's frame (cm) and the way they face there (degrees).
FALLBACK_WAKE = (170.0, 0.0, 0.0)

# How far (cm) a roost may be moved from its layout spot to a spot that passes, in rings this far apart.
MOVE_MOST = 600.0
MOVE_STEP = 50.0
MOVE_TURN = 15.0

# The cairn's half width and the player's feet (cm), and the most their ground may differ under them (cm).
CAIRN_HALF = 65.0
FEET_HALF = 40.0
CAIRN_SPREAD = 45.0
FEET_SPREAD = 35.0
# Nothing solid within these (cm) at the mast (over the cairn, at chest height) and at the player's spot.
MAST_CLEAR = 55.0
FEET_CLEAR = 40.0

# Clearances (cm) for the mast and, after the slash, the player's spot: from a road's edge (the spot may stand on a road),
# an obstacle's line or outline, a placed model's pivot, a stand-in's box, a pit's rim, a rock knob, the boundary.
ROAD_CLEAR = 100.0
OBSTACLE_CLEAR = (150.0, 110.0)
PLACEMENT_CLEAR = (330.0, 220.0)
STANDIN_CLEAR = (250.0, 150.0)
PIT_CLEAR = 450.0
ROCK_CLEAR = 150.0
BOUNDARY_CLEAR = 300.0

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


# ---------------------------------------------------------------------------
# Plain geometry (level cm, X north, Y east)
# ---------------------------------------------------------------------------

def segment_distance(p, a, b):
    ax, ay = b[0] - a[0], b[1] - a[1]
    length2 = ax * ax + ay * ay
    t = 0.0 if length2 < 1e-6 else max(0.0, min(1.0, ((p[0] - a[0]) * ax + (p[1] - a[1]) * ay) / length2))
    return math.hypot(p[0] - (a[0] + ax * t), p[1] - (a[1] + ay * t))


def path_distance(p, path):
    if len(path) == 1:
        return math.hypot(p[0] - path[0][0], p[1] - path[0][1])
    return min(segment_distance(p, a, b) for a, b in zip(path, path[1:]))


def inside(p, polygon):
    """Whether p is inside the polygon (even-odd)."""
    result = False
    for (x0, y0), (x1, y1) in zip(polygon, list(polygon[1:]) + list(polygon[:1])):
        if (y0 > p[1]) != (y1 > p[1]) and p[0] < x0 + (p[1] - y0) * (x1 - x0) / (y1 - y0):
            result = not result
    return result


def outline_distance(p, polygon):
    return path_distance(p, list(polygon) + [polygon[0]])


def turned(x, y, yaw):
    """(x, y) turned yaw degrees about the vertical."""
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return x * c - y * s, x * s + y * c


# ---------------------------------------------------------------------------
# The models
# ---------------------------------------------------------------------------

def load_if_made(path):
    return unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None


def wake_socket(roost_mesh):
    """Where the player wakes in the mast's frame: (x, y, yaw), from the model's Wake socket or FALLBACK_WAKE."""
    socket = roost_mesh.find_socket('Wake') if roost_mesh is not None else None
    if socket is None:
        return FALLBACK_WAKE
    at = socket.get_editor_property('relative_location')
    turn = socket.get_editor_property('relative_rotation')
    return at.x, at.y, turn.yaw


# ---------------------------------------------------------------------------
# The level
# ---------------------------------------------------------------------------

def ground(x, y):
    """The walkable terrain (tagged Ground) under (x, y), or None: a fence rail, a rock or a roof never counts."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, 30000.0), unreal.Vector(x, y, -30000.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    for hit in hits or []:
        t = hit.to_tuple()
        if t[9] is not None and unreal.Name('Ground') in t[9].tags:
            return t[4].z
    return None


def footing(x, y, half):
    """The ground under (x, y) and four points half out round it: (the middle's height, the spread), or None off the ground."""
    spots = [(x, y), (x + half, y), (x - half, y), (x, y + half), (x, y - half)]
    zs = [ground(sx, sy) for sx, sy in spots]
    if any(z is None for z in zs):
        return None
    return zs[0], max(zs) - min(zs)


def solid_at(x, y, z, radius):
    """Something solid stands at (x, y, z) within radius: the dressing's fences and props, a placed model, the story's
    pieces. The terrain (tagged Ground) and the scatter's volume don't count (the scatter keeps off the roost once it's run
    again)."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    try:
        found = unreal.SystemLibrary.sphere_overlap_actors(world, unreal.Vector(x, y, z), radius,
                                                           [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], None, [])
    except Exception as error:  # a failed check never stops the build: the layout's checks still hold
        unreal.log_warning(f'roosts: the spot ({x:.0f}, {y:.0f}) could not be checked for solids ({error})')
        return None
    hits = found[1] if isinstance(found, tuple) else found
    for actor in hits or []:
        if actor is None or unreal.Name('Ground') in actor.tags or isinstance(actor, unreal.PCGVolume):
            continue
        if actor.get_class().get_name() in ('RespawnMarker', 'RoostCrows'):
            continue
        return actor.get_actor_label()
    return None


# ---------------------------------------------------------------------------
# What a roost keeps clear of, from the layout
# ---------------------------------------------------------------------------

def blockers(build):
    """(kind, data, clearance for the mast, for the player's spot, is a road) from the layout's roads, ramps, pits, rock
    knobs, obstacles, placements and stand-ins."""
    found = []
    for road in build.source.get('roads', []):
        if road.get('path'):
            found.append(('path', road['path'], road.get('width', 300.0) * 0.5 + ROAD_CLEAR, 0.0, True))
    for feature in build.source.get('features', []):
        ramp = feature.get('ramp')
        if ramp and ramp.get('path'):
            found.append(('path', ramp['path'], ramp.get('width', 400.0) * 0.5 + ROAD_CLEAR, 0.0, True))
        if feature.get('polygon') and feature.get('type') == 'pit':
            found.append(('area', feature['polygon'], PIT_CLEAR, PIT_CLEAR, False))
        if feature.get('center') and feature.get('radius') and feature.get('rock'):
            found.append(('point', [feature['center']], feature['radius'] + ROCK_CLEAR, feature['radius'] + ROCK_CLEAR * 0.5, False))
    for obstacle in build.source.get('obstacles', []):
        if obstacle.get('path'):
            found.append(('path', obstacle['path'], OBSTACLE_CLEAR[0], OBSTACLE_CLEAR[1], False))
        elif obstacle.get('polygon'):
            found.append(('area', obstacle['polygon'], OBSTACLE_CLEAR[0], OBSTACLE_CLEAR[1], False))
    for spot in build.layout.get('placements', {}).values():
        found.append(('point', [spot['location'][:2]], PLACEMENT_CLEAR[0], PLACEMENT_CLEAR[1], False))
    for stand_in in build.source.get('preview', {}).get('standIns', []):
        (sx, sy), (bx, by) = stand_in['location'][:2], stand_in['box'][:2]
        # The stand-ins' boxes are metres across, about their pivot.
        hx, hy = bx * 50.0, by * 50.0
        found.append(('area', [(sx - hx, sy - hy), (sx + hx, sy - hy), (sx + hx, sy + hy), (sx - hx, sy + hy)],
                      STANDIN_CLEAR[0], STANDIN_CLEAR[1], False))
    return found


def blocked_by(p, found, mast, on_road):
    """The first blocker p is too near (a word for the log), or None. mast: the mast's clearances, else the player's spot's."""
    for kind, data, mast_clear, feet_clear, road in found:
        if road and (on_road or not mast):
            continue
        clearance = mast_clear if mast else feet_clear
        if kind == 'area':
            if inside(p, data) or outline_distance(p, data) < clearance:
                return 'too near something the layout has'
        elif path_distance(p, data) < clearance:
            return 'a road' if road else 'too near something the layout has'
    return None


def in_bounds(build, p):
    corners = build.layout.get('boundary', {}).get('corners', [])
    if len(corners) < 3:
        return True
    points = [c[:2] for c in corners]
    return inside(p, points) and outline_distance(p, points) >= BOUNDARY_CLEAR


# ---------------------------------------------------------------------------
# Choosing each roost's spot
# ---------------------------------------------------------------------------

def judge(build, found, mast, feet, on_road):
    """Why the mast at mast (x, y) and the player at feet (x, y) won't do, or None and their ground (mast z, feet z)."""
    for p, is_mast in ((mast, True), (feet, False)):
        if not in_bounds(build, p):
            return 'outside the playable area', None
        why = blocked_by(p, found, is_mast, on_road)
        if why:
            return why, None
    under_mast = footing(mast[0], mast[1], CAIRN_HALF)
    under_feet = footing(feet[0], feet[1], FEET_HALF)
    if under_mast is None or under_feet is None:
        return 'no walkable ground', None
    if under_mast[1] > CAIRN_SPREAD or under_feet[1] > FEET_SPREAD:
        return 'too steep', None
    solid = solid_at(mast[0], mast[1], under_mast[0] + 150.0, MAST_CLEAR) or solid_at(feet[0], feet[1], under_feet[0] + 100.0, FEET_CLEAR)
    if solid:
        return f'something solid there ({solid})', None
    return None, (under_mast[0], under_feet[0])


def choose(build, found, entry, wake):
    """The spot for a roost: (mast x, y, z, the player's x, y, z, how far it moved), or None with a warning."""
    ax, ay = entry['at'][:2]
    face = float(entry.get('yaw', 0.0))
    wx, wy, wyaw = wake
    # The mast turned so its Wake socket faces the layout's yaw; the player's spot is that socket from the mast.
    mast_yaw = face - wyaw
    ox, oy = turned(wx, wy, mast_yaw)
    on_road = bool(entry.get('onRoad', False))
    reasons = {}
    rings = [0.0]
    step = MOVE_STEP
    while step <= MOVE_MOST + 1e-3:
        rings.append(step)
        step += MOVE_STEP
    for radius in rings:
        turns = [0.0] if radius == 0.0 else [i * MOVE_TURN for i in range(int(360.0 / MOVE_TURN))]
        for turn in turns:
            mx = ax + math.cos(math.radians(turn)) * radius
            my = ay + math.sin(math.radians(turn)) * radius
            feet = (mx + ox, my + oy)
            why, heights = judge(build, found, (mx, my), feet, on_road)
            if why:
                reasons[why] = reasons.get(why, 0) + 1
                continue
            return mx, my, heights[0], feet[0], feet[1], heights[1], radius, mast_yaw
    build.warn(f"roost {entry['id']}: no spot within {MOVE_MOST / 100.0:.0f} m of ({ax:.0f}, {ay:.0f}) will do ({reasons}): left out")
    return None


# ---------------------------------------------------------------------------
# Placing them
# ---------------------------------------------------------------------------

def place(build):
    """Every roost of the layout's gameplay.roosts, in the Gameplay/Roosts folder (a gameplay build clears it first); three
    at most (MOST_PER_MAP)."""
    entries = build.settings.get('roosts', [])
    if not entries:
        return
    if len(entries) > MOST_PER_MAP:
        left_out = ', '.join(entry.get('id', '?') for entry in entries[MOST_PER_MAP:])
        build.warn(f'gameplay.roosts has {len(entries)} roosts; the user allows {MOST_PER_MAP} a map: {left_out} left out')
        entries = entries[:MOST_PER_MAP]
    cls = unreal.load_class(None, CLASSES + 'RespawnMarker')
    if cls is None:
        build.warn('no RespawnMarker class (build the game module first): the roosts are left out')
        return
    roost_mesh = load_if_made(ROOST_MESH)
    bell_mesh = load_if_made(BELL_MESH)
    if roost_mesh is None:
        build.warn(f'no {ROOST_MESH} yet: the roosts stand as their greybox')
    wake = wake_socket(roost_mesh)
    found = blockers(build)
    ids = set()
    placed = 0
    for entry in entries:
        roost_id = entry['id']
        if roost_id in ids:
            build.warn(f'roost {roost_id} is in the layout twice: the second is left out (ids are save data)')
            continue
        ids.add(roost_id)
        spot = choose(build, found, entry, wake)
        if spot is None:
            continue
        mx, my, mz, fx, fy, fz, moved, mast_yaw = spot
        roost = build.place(cls, (fx, fy, fz), float(entry.get('yaw', 0.0)), label=f'Roost_{roost_id}', folder=FOLDER,
                            tags=(OBSTACLE_TAG, ROOST_TAG))
        roost.set_editor_property('marker_id', unreal.Name(roost_id))
        roost.set_editor_property('display_name', unreal.Text(entry.get('name', roost_id)))
        roost.set_editor_property('start_active', bool(entry.get('startOpen', False)))
        roost.set_editor_property('active_after_mission', unreal.Name(entry.get('openAfter', 'None')))
        roost.set_editor_property('mark_after_mission', unreal.Name(entry.get('markAfter', 'None')))
        # fastTravel false: a death-respawn and save point only (the town roost; ARespawnMarker.bFastTravelStation).
        roost.set_editor_property('fast_travel_station', bool(entry.get('fastTravel', True)))
        if roost_mesh is not None:
            roost.set_editor_property('roost_mesh', roost_mesh)
        if bell_mesh is not None:
            roost.set_editor_property('bell_mesh', bell_mesh)
        # The mast's foot on its own ground, a little sunk so its cairn never shows a gap.
        roost.set_editor_property('roost_lift', mz - fz - 5.0)
        placed += 1
        gates = ', '.join(f'{key} {entry[key]}' for key in ('openAfter', 'markAfter') if entry.get(key))
        build.log(f"roost {roost_id} ({entry.get('name', roost_id)}): the mast at ({mx:.0f}, {my:.0f}, {mz:.0f}), the player's spot at "
                  f"({fx:.0f}, {fy:.0f}, {fz:.0f}) facing {float(entry.get('yaw', 0.0)):.0f}"
                  f"{f', moved {moved / 100.0:.1f} m from the layout' if moved > 0.0 else ''}{f' ({gates})' if gates else ''}")
    build.log(f'{placed} of {len(entries)} roosts placed')


def run(area):
    """The roosts again in the area's open level, on their own: every roost there before goes first."""
    sys.path.append(HERE)
    import build_area
    build = importlib.reload(build_area).AreaBuild(area)
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world.get_path_name().split('.')[0] != build.level:
        raise RuntimeError(f'open {build.level} first (the editor has {world.get_path_name()}): nothing was placed')
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    cls = unreal.load_class(None, CLASSES + 'RespawnMarker')
    if cls is None:
        raise RuntimeError('no RespawnMarker class: build the game module first')
    # Every roost in the level goes, the build's (and the graves' markers before them, the same class) and any placed by
    # hand, wherever they stood; the layout's are placed again below. Run twice, it leaves the same ones (the cut to two a
    # map, 2026-10-14, removes the rest on its first run). A trip's landing tagged on a marker (Landing_...) is the travel's,
    # never a roost: it stays.
    old = [a for a in actors.get_all_level_actors()
           if a.get_class() == cls and not any(str(tag).startswith('Landing_') for tag in a.tags)]
    if old:
        names = ', '.join(sorted(str(a.get_editor_property('marker_id')) for a in old))
        actors.destroy_actors(old)
        build.log(f'removed the {len(old)} roosts placed before ({names})')
    place(build)
    levels.save_current_level()


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_roosts.py <Area> (a folder under Art/Levels whose layout has gameplay.roosts)')
    run(sys.argv[1])
