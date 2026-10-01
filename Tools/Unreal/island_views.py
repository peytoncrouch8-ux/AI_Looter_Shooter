"""The tutorial island's viewpoints: where a player looks from in each area, for screenshots and performance checks.

  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/island_views.py"                   place the cameras
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/island_views.py shoot Spawn Medium"  one screenshot
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/island_views.py clear"             remove the cameras

The viewpoints are Art/Levels/TutorialIsland/views.json's. The cameras are editor-only helpers (tagged IslandView,
never saved: clear them, or reload the level without saving). The editor only renders while it is not minimized and
not behind other windows; for shots of the running game use Tools/tour.ps1. Shots land in
Saved/Screenshots/Island/<view>_<quality>.png. The editor stays on the quality of the last shot until
review_stage.py restore (or a restart) puts it back on Epic.
"""
import json
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import review_stage  # noqa: E402

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OUT = os.path.join(PROJECT, 'Saved/Screenshots/Island')
TAG = 'IslandView'

# The viewpoints are shared with the game's Looter.Tour (Tools/tour.ps1): name, x, y, height above the ground, yaw,
# pitch and field of view.
with open(os.path.join(PROJECT, 'Art/Levels/TutorialIsland/views.json')) as _file:
    VIEWS = {v['name']: (v['x'], v['y'], v['height'], v['yaw'], v['pitch'], v.get('fov', 80.0))
             for v in json.load(_file)['views']}

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def ground(x, y):
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 30000.0), unreal.Vector(x, y, -30000.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    return hit.to_tuple()[5].z if hit else 0.0


def cameras():
    return {a.get_actor_label(): a for a in actors.get_all_level_actors() if unreal.Name(TAG) in a.tags}


def place():
    if unreal.EditorLevelLibrary.get_editor_world().get_path_name().split('.')[0] != review_stage.island.LEVEL:
        unreal.EditorLoadingAndSavingUtils.load_map(review_stage.island.LEVEL)
    clear()
    for name, (x, y, height, yaw, pitch, fov) in VIEWS.items():
        camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(x, y, ground(x, y) + height),
                                               unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
        camera.set_actor_label(name)
        camera.set_editor_property('tags', [unreal.Name(TAG)])
        camera.camera_component.set_editor_property('field_of_view', fov)


def clear():
    stale = list(cameras().values())
    if stale:
        actors.destroy_actors(stale)


def shoot(name, quality):
    # Selection outlines would show in the shot.
    actors.select_nothing()
    level, variables = review_stage.QUALITY[quality]
    review_stage.console(f'scalability {level}')
    review_stage.console('r.ScreenPercentage 100')
    for key, value in variables.items():
        review_stage.console(f'{key} {value}')
    os.makedirs(OUT, exist_ok=True)
    unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, os.path.join(OUT, f'{name}_{quality}.png'),
                                                      cameras()[name])
    # The editor stops redrawing while it's behind other windows: ask for a redraw, which takes the shot.
    unreal.EditorLevelLibrary.editor_invalidate_viewports()


if __name__ == '__main__':
    args = sys.argv[1:]
    if not args:
        place()
        unreal.log(f'Island views: placed {", ".join(VIEWS)}')
    elif args[0] == 'shoot':
        shoot(args[1], args[2] if len(args) > 2 else 'Medium')
        unreal.log(f'Island views: {args[1]} {args[2] if len(args) > 2 else "Medium"} requested')
    elif args[0] == 'clear':
        clear()
        unreal.log('Island views: cleared')
