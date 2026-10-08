"""Says whether a play session is running in the editor ("PIECHECK PLAYING <world>" or "PIECHECK idle"). Check it, and
for the launcher's standalone game (UnrealEditor.exe ... -game), before any editor work: the user may be playing.
Usage: Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/pie_check.py" -Until PIECHECK"""
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
unreal.log(f'PIECHECK {"PLAYING " + world.get_path_name() if world else "idle"}')
