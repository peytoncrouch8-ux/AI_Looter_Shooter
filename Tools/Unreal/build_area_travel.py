"""Where an area's trips start and end, for build_area.py: the stations players leave from and the landings they arrive at
(Areas/AreaLandings.h: a landing is a player start named for it, or an actor or component tagged with its name).

- The skiff jetty (layout.json gameplay.jetty: at [X, Y] on the rim where the drop starts, its yaw out over the drop):
  ASkiffJetty moors its skiff and carries its own landing (Landing_Jetty).
- The depot (a Depot placement) stands as ATrainStation: the depot with its departures board, carrying the landing its
  LandingName names (Landing_Depot), set from the Landing placement keyed by that name: on the platform's boards,
  facing the way the placement does.
- The spawn's landing (gameplay.spawnLanding): a player start named for the landing a first arrival comes to, beside the
  level's own (unnamed) start.
- Any other Landing placement: a target point tagged with its key.
A class the game module hasn't built yet is left out with a warning; the depot then stands as a plain mesh.
"""
import math

import unreal

CLASSES = '/Script/AI_Looter_Shooter.'


def trace_down(x, y, top=20000.0, bottom=-20000.0):
    """The first surface under (x, y) between top and bottom, or None."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, top), unreal.Vector(x, y, bottom),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    return None if hit is None else hit.to_tuple()[5].z


def place_jetty(build):
    """The skiff jetty, its origin on the ground where the drop starts (in the Gameplay folder)."""
    spot = build.settings.get('jetty')
    if not spot:
        return
    cls = unreal.load_class(None, CLASSES + 'SkiffJetty')
    if cls is None:
        build.warn('no SkiffJetty class (build the game module first): the jetty is left out')
        return
    x, y = spot['at']
    out = math.radians(spot['yaw'])
    # The ground a metre inland too: right at the rim the trace can slip past the edge and find the cliff below.
    heights = [h for h in (trace_down(x, y), trace_down(x - 100.0 * math.cos(out), y - 100.0 * math.sin(out)))
               if h is not None]
    if not heights:
        build.warn(f'no ground under the jetty at ({x}, {y})')
        return
    build.place(cls, (x, y, max(heights)), spot['yaw'], label='SkiffJetty', folder='Gameplay')


def station_class():
    return unreal.load_class(None, CLASSES + 'TrainStation')


def station_landings(build):
    """The landings the area's stations carry: their LandingName for each Depot placement, when the class exists."""
    cls = station_class()
    if cls is None or not any(s['kind'] == 'Depot' for s in build.layout['placements'].values()):
        return set()
    return {str(unreal.get_default_object(cls).get_editor_property('landing_name'))}


def place_station(build, key, spot, mesh):
    """The depot as the area's station (in the Buildings folder). Returns None when the game module has no ATrainStation
    yet. Its landing waits for land_station, once the platform it lies on stands too."""
    cls = station_class()
    if cls is None:
        build.warn('no TrainStation class (build the game module first): the depot stands as a plain mesh')
        return None
    station = build.place(cls, spot['location'], spot['yaw'], label=key, folder='Buildings', tags=('Obstacle',))
    station.set_editor_property('building_mesh', mesh)
    return station


def land_station(build, station, key, spot):
    """Sets a placed station's landing from the Landing placement its LandingName names."""
    name = str(station.get_editor_property('landing_name'))
    landing = build.layout['placements'].get(name)
    if landing is None:
        build.warn(f'{key}: no {name} placement, so trips arrive at the station itself')
        return
    lx, ly, lz = landing['location']
    # The spot is where feet stand (the travel adds the capsule's half height): on the platform's boards. A car's door
    # opens at the platform's edge, so it steps onto the boards the way it faces until a probe finds them.
    face = math.radians(landing.get('yaw', 0.0))
    for step in (0.0, 30.0, 60.0, 90.0, 120.0):
        px, py = lx + step * math.cos(face), ly + step * math.sin(face)
        boards = trace_down(px, py, lz + 150.0, lz - 100.0)
        if boards is not None and boards > lz + 10.0:
            lx, ly, lz = px, py, boards
            break
    else:
        build.warn(f'{key}: no platform at {name}, so it stays on the ground')
    sx, sy, sz = spot['location']
    yaw = math.radians(spot['yaw'])
    dx, dy = lx - sx, ly - sy
    local = unreal.Vector(dx * math.cos(yaw) + dy * math.sin(yaw), -dx * math.sin(yaw) + dy * math.cos(yaw), lz - sz)
    turn = unreal.Rotator(roll=0.0, pitch=0.0, yaw=landing.get('yaw', 0.0) - spot['yaw'])
    station.set_editor_property('landing_transform', unreal.Transform(local, turn, unreal.Vector(1.0, 1.0, 1.0)))
    build.log(f'{key}: the station, its {name} at ({lx:.0f}, {ly:.0f}, {lz:.0f})')


def place_spawn_landing(build, location, yaw):
    """gameplay.spawnLanding: a player start of its own named for that landing, where the level's start is (in the
    Gameplay folder). The level's start stays unnamed: a death with no grave open wakes there, never at a landing."""
    landing = build.settings.get('spawnLanding')
    if landing:
        start = build.place(unreal.PlayerStart, location, yaw, label=landing, folder='Gameplay')
        start.set_editor_property('player_start_tag', unreal.Name(landing))


def place_landings(build):
    """A target point (in the Gameplay folder) for each Landing placement no station carries, tagged with its key."""
    carried = station_landings(build)
    for key, spot in build.layout['placements'].items():
        if spot['kind'] == 'Landing' and key not in carried:
            build.place(unreal.TargetPoint, spot['location'], spot['yaw'], label=key, folder='Gameplay', tags=(key,))
