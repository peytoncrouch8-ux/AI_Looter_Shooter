"""Takes the gunsmith's bench out of Ransom's Rest (the user, 2026-10-28: "Gun parts bench shouldn't be unlocked until the
lily", "Lock it now": part swapping first comes with Ozias after the Gilded Lily). layout.json gameplay.benches no longer
lists Bench_Farm, so a gameplay build won't place it again; this removes the one already saved in the level without a
rebuild. The tutorial island's Bench_Village went too (a session's fallback map): pass its map,
  ... remove_rr_bench.py /Game/Maps/Lvl_TutorialIsland

Run with the editor free (no PIE): Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/remove_rr_bench.py"
-Until "REMOVE_BENCH (done|stop)". It opens the level clean from disk first (open_clean.py: a test's dirty copy goes,
nothing saved), destroys every AGunsmithBench and every actor tagged GunsmithBench, and saves the level.
"""
import os
import runpy
import sys

import unreal

LEVEL = sys.argv[1] if len(sys.argv) > 1 else '/Game/Maps/Lvl_RansomsRest'
TAG = unreal.Name('GunsmithBench')
OPEN_CLEAN = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'open_clean.py')


def say(text):
    unreal.log(f'REMOVE_BENCH {text}')


def stop(text):
    unreal.log_warning(f'REMOVE_BENCH stop: {text}')


def main():
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if editor.get_game_world() is not None:
        return stop('PIE or simulate is running: stop it first')
    # Another map's unsaved edits would go with the load: leave them to Main (untitled /Temp maps never count).
    others = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
              if p.get_name() != LEVEL and not p.get_name().startswith('/Temp/')]
    if others:
        return stop(f'unsaved changes in {others}: save or discard them first')
    argv = sys.argv
    sys.argv = [OPEN_CLEAN, LEVEL]
    try:
        runpy.run_path(OPEN_CLEAN, run_name='__main__')
    finally:
        sys.argv = argv
    world = editor.get_editor_world()
    if world is None or world.get_path_name().split('.')[0] != LEVEL:
        return stop(f'{LEVEL} did not open clean (the editor world is {world.get_path_name() if world else None})')

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    found = [a for a in actors.get_all_level_actors()
             if a.get_class().get_name() == 'GunsmithBench' or TAG in a.tags]
    for a in found:
        at = a.get_actor_location()
        say(f'removing {a.get_actor_label()} ({a.get_class().get_name()}) at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}) '
            f'in {a.get_folder_path()}')
    if not found:
        say('no gunsmith bench in the level: nothing to remove, nothing saved')
        say('done')
        return
    actors.destroy_actors(found)
    left = [a for a in actors.get_all_level_actors() if a.get_class().get_name() == 'GunsmithBench' or TAG in a.tags]
    if left:
        return stop(f'{len(left)} still in the level after destroying: not saved')
    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    say(f'removed {len(found)}; level saved: {saved}')
    say('done')


main()
