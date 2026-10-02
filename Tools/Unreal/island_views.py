"""An area's viewpoints: where a player looks from in each part of it, for screenshots and performance checks.

  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/island_views.py [Area]"                   the cameras
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/island_views.py [Area] shoot Spawn Medium"  one shot
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/island_views.py [Area] clear"             remove them

The area defaults to TutorialIsland, and its viewpoints are Art/Levels/<Area>/views.json's. The cameras are editor-only
helpers (tagged IslandView, never saved: clear them, or reload the level without saving). The editor only renders
while it is not minimized and not behind other windows; for shots of the running game use Tools/tour.ps1. Shots land
in Saved/Screenshots/<the area's outliner folder>/<view>_<quality>.png (Saved/Screenshots/Island on the tutorial
island). The editor stays on the quality of the last shot until review_stage.py restore (or a restart) puts it back
on Epic.
"""
import json
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area  # noqa: E402
import review_stage  # noqa: E402

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
TAG = 'IslandView'

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def views(area):
    """The area's viewpoints, shared with the game's Looter.Tour (Tools/tour.ps1): name, x, y, height above the
    ground, yaw, pitch and field of view."""
    with open(os.path.join(PROJECT, 'Art', 'Levels', area, 'views.json')) as file:
        return {v['name']: (v['x'], v['y'], v['height'], v['yaw'], v['pitch'], v.get('fov', 80.0))
                for v in json.load(file)['views']}


def ground(x, y):
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 30000.0), unreal.Vector(x, y, -30000.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    return hit.to_tuple()[5].z if hit else 0.0


def cameras():
    return {a.get_actor_label(): a for a in actors.get_all_level_actors() if unreal.Name(TAG) in a.tags}


def place(area):
    level = build_area.AreaBuild(area).level
    if unreal.EditorLevelLibrary.get_editor_world().get_path_name().split('.')[0] != level:
        unreal.EditorLoadingAndSavingUtils.load_map(level)
    clear()
    placed = views(area)
    for name, (x, y, height, yaw, pitch, fov) in placed.items():
        camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(x, y, ground(x, y) + height),
                                               unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
        camera.set_actor_label(name)
        camera.set_editor_property('tags', [unreal.Name(TAG)])
        camera.camera_component.set_editor_property('field_of_view', fov)
    return placed


def clear():
    stale = list(cameras().values())
    if stale:
        actors.destroy_actors(stale)


def shoot(area, name, quality):
    # Selection outlines would show in the shot.
    actors.select_nothing()
    level, variables = review_stage.QUALITY[quality]
    review_stage.console(f'scalability {level}')
    review_stage.console('r.ScreenPercentage 100')
    for key, value in variables.items():
        review_stage.console(f'{key} {value}')
    out = os.path.join(PROJECT, 'Saved', 'Screenshots', build_area.AreaBuild(area).folder)
    os.makedirs(out, exist_ok=True)
    unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, os.path.join(out, f'{name}_{quality}.png'),
                                                      cameras()[name])
    # The editor stops redrawing while it's behind other windows: ask for a redraw, which takes the shot.
    unreal.EditorLevelLibrary.editor_invalidate_viewports()


if __name__ == '__main__':
    args = sys.argv[1:]
    # The area comes first when given; older commands start with the action (the tutorial island).
    area = args.pop(0) if args and args[0] not in ('shoot', 'clear') else 'TutorialIsland'
    if not args:
        unreal.log(f'Island views: placed {", ".join(place(area))}')
    elif args[0] == 'shoot':
        shoot(area, args[1], args[2] if len(args) > 2 else 'Medium')
        unreal.log(f'Island views: {args[1]} {args[2] if len(args) > 2 else "Medium"} requested')
    elif args[0] == 'clear':
        clear()
        unreal.log('Island views: cleared')
