"""An area's wanted posters and Calder's note (AWantedPoster: Ransom's Rest's Side 1), from layout.json gameplay.posters,
for build_area.py's gameplay() (importlib.reload(build_area_posters).place(self)), or on their own in the open level:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_posters.py RansomsRest"
which replaces only the posters a build placed before, and saves the level.

Each entry names its host, a placement key of layout.json (build_area.py labels the building, the depot's station or
the notice board with it), and where on it:
  {"id": "posterSheriff", "host": "sheriff", "at": [500, 96, 188], "yaw": 0}
      at: a point on the host's face in its own frame (cm: +X its front, +Y its right as Unreal has it, +Z up from its
      pivot); yaw: which way the paper faces, from the host's front (0: the way the host faces, 90: toward its +Y).
      The paper is snapped onto the surface there: a line from 40 cm in front of the point back through it, against the
      host's own meshes only (complex collision), and it faces out of what it meets. Missing, it stays at the point.
  {"id": "calderNote", "host": "noticeBoard", "socket": "Decal", "offset": [0, -40, 9], "variant": "CalderNote"}
      socket: on that socket of the host's mesh (the board's SOCKET_Decal sits 1 mm off its face, facing out), moved
      by offset along the socket's axes (cm: +Y to the left of whoever reads it, +Z up).
variant: Wanted (the default) or CalderNote. scale: the paper's size against the atlas's (default 1). Flat surfaces only
(planks, boards, plaster, stone): the decal stretches over round logs.
"""
import importlib
import math
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
CLASSES = '/Script/AI_Looter_Shooter.'
# The snapping line: from this far in front of the point to this far behind it (cm).
SNAP_OUT = 40.0
SNAP_IN = 40.0
# A face whose normal climbs more than this is no wall (a sill, a roof).
WALL_STEEPNESS = 0.5

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def poster_class():
    return unreal.load_class(None, CLASSES + 'WantedPoster')


def find_hosts(build):
    """The level's actors this build placed, by label (their placement keys)."""
    hosts = {}
    for actor in actors.get_all_level_actors():
        if unreal.Name(build.tag) in actor.tags:
            hosts.setdefault(str(actor.get_actor_label()), actor)
    return hosts


def trace_onto(host, start, end):
    """Where the line from start to end first meets one of the host's own meshes, and that surface's normal; None when
    it meets none."""
    best = None
    for mesh in host.get_components_by_class(unreal.StaticMeshComponent):
        hit = mesh.line_trace_component(start, end, True, False, False)
        if not hit:
            continue
        if isinstance(hit, tuple):
            # (location, normal, bone, hit result): K2_LineTraceComponent's outputs.
            location, normal = hit[0], hit[1]
        else:
            fields = hit.to_tuple()
            location, normal = fields[5], fields[7]
        distance = (location - start).length()
        if best is None or distance < best[0]:
            best = (distance, location, normal)
    return None if best is None else best[1:]


def on_face(build, host, spec):
    """A point on the host's face, snapped onto it: (location, yaw), or the point itself with a warning."""
    point = host.get_actor_transform().transform_location(unreal.Vector(*spec['at']))
    yaw = host.get_actor_rotation().yaw + spec.get('yaw', 0.0)
    out = unreal.Vector(math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0.0)
    hit = trace_onto(host, point + out * SNAP_OUT, point - out * SNAP_IN)
    if hit is None:
        build.warn(f"poster {spec['id']}: nothing of {spec['host']} at {spec['at']}, so it hangs there unsnapped")
        return point, yaw
    location, normal = hit
    if abs(normal.z) > WALL_STEEPNESS:
        build.warn(f"poster {spec['id']}: {spec['host']} at {spec['at']} is no wall (its normal climbs {normal.z:.2f}), "
                   f"so it hangs there unsnapped")
        return point, yaw
    return location, math.degrees(math.atan2(normal.y, normal.x))


def on_socket(build, host, spec):
    """On a socket of the host's mesh, moved by the offset along its axes: (location, yaw), or None."""
    name = spec['socket']
    for mesh in host.get_components_by_class(unreal.StaticMeshComponent):
        static = mesh.get_editor_property('static_mesh')
        if static is not None and static.find_socket(name) is not None:
            socket = mesh.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)
            location = socket.transform_location(unreal.Vector(*spec.get('offset', (0.0, 0.0, 0.0))))
            return location, socket.rotation.rotator().yaw
    build.warn(f"poster {spec['id']}: {spec['host']} has no {name} socket (its model isn't imported yet?): left out")
    return None


def place(build):
    """Every poster of the layout's gameplay.posters, in the Gameplay folder (a gameplay build clears it first)."""
    specs = build.settings.get('posters', [])
    if not specs:
        return
    cls = poster_class()
    if cls is None:
        build.warn('no WantedPoster class (build the game module first): the posters are left out')
        return
    hosts = find_hosts(build)
    placed = 0
    for spec in specs:
        host = hosts.get(spec['host'])
        if host is None:
            build.warn(f"poster {spec['id']}: no {spec['host']} in the level (build_area.py places it first): left out")
            continue
        where = on_socket(build, host, spec) if 'socket' in spec else on_face(build, host, spec)
        if where is None:
            continue
        location, yaw = where
        poster = build.place(cls, (location.x, location.y, location.z), yaw, label=spec['id'], folder='Gameplay',
                             tags=('Obstacle',))
        if spec.get('variant', 'Wanted') == 'CalderNote':
            poster.set_editor_property('variant', unreal.WantedPosterVariant.CALDER_NOTE)
        if 'scale' in spec:
            poster.set_editor_property('scale', float(spec['scale']))
        placed += 1
    build.log(f'placed {placed} of {len(specs)} posters')


def run(area):
    """The posters again in the area's open level, on their own: the ones a build placed before go first."""
    sys.path.append(HERE)
    import build_area
    build = importlib.reload(build_area).AreaBuild(area)
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world.get_path_name().split('.')[0] != build.level:
        raise RuntimeError(f'open {build.level} first (the editor has {world.get_path_name()}): nothing was placed')
    cls = poster_class()
    if cls is None:
        raise RuntimeError('no WantedPoster class: build the game module first')
    old = [a for a in actors.get_all_level_actors()
           if a.get_class() == cls and unreal.Name(build.tag) in a.tags]
    if old:
        actors.destroy_actors(old)
        build.log(f'removed the {len(old)} posters placed before')
    place(build)
    levels.save_current_level()


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_posters.py <Area> (a folder under Art/Levels whose layout has gameplay.posters)')
    run(sys.argv[1])
