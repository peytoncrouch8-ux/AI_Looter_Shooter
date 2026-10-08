"""Opens a map clean from disk: collects garbage first (a test's loaded copy of it goes), then loads it. Never saves."""
import sys
import unreal

path = sys.argv[1] if len(sys.argv) > 1 else '/Game/Maps/Lvl_RansomsRest'
unreal.SystemLibrary.collect_garbage()
before = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
unreal.log(f'OPENCLEAN dirty maps after gc: {before}')
if path in before:
    unreal.log_warning(f'OPENCLEAN {path} is still loaded and dirty: not opening it')
else:
    unreal.EditorLoadingAndSavingUtils.load_map(path)
    unreal.log('OPENCLEAN opened %s; dirty maps now %s' % (
        unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name(),
        [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]))
