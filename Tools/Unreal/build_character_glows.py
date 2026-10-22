"""Moves the heroes' glowing eyes and Gauge's lamp onto M_CharacterGlow (CHAR-ART items 4 and 11, 2026-10-22: a hot core,
an iris ring and a soft falloff instead of the flat white ovals M_StylizedGlow burned them to), without re-importing the
heroes: each MI_<slot> in Tools/Blender/looter_export.py's CHARACTER_GLOWS (read from that file, so the two never
disagree) takes M_CharacterGlow as its parent and keeps its Color; its Glow is the one it had (times the table's
GlowScale) unless the table sets Glow; the table's other numbers are set on it. A hero's next import does the same from
its script (looter_export maps those Glow slots to the master), so this is only for heroes not re-imported yet.

Run after build_world_materials.py M_CharacterGlow, in the open editor:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_character_glows.py"
It prints a CHARGLOWS line per instance and "CHARGLOWS done". An instance already on M_CharacterGlow is set again the
same way (its Color and Glow kept); a missing one is skipped.
"""
import ast
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
EXPORT = os.path.normpath(os.path.join(HERE, '..', 'Blender', 'looter_export.py'))
MASTER = '/Game/Art/Materials/Masters/M_CharacterGlow'
FOLDERS = ('/Game/Art/Materials',)            
MEL = unreal.MaterialEditingLibrary


def log(message):
    unreal.log(f'CHARGLOWS {message}')


def table():
    """CHARACTER_GLOWS from looter_export.py (a plain literal), without running that Blender module."""
    with open(EXPORT, encoding='utf-8') as source:
        tree = ast.parse(source.read(), EXPORT)
    for node in tree.body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'CHARACTER_GLOWS' for t in node.targets):
            return ast.literal_eval(node.value)
    raise RuntimeError(f'{EXPORT} has no CHARACTER_GLOWS')


def find(name):
    for folder in FOLDERS:
        path = f'{folder}/MI_{name}'
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.load_asset(path)
    return None


def value(instance, kind, name):
    values = instance.get_editor_property(f'{kind}_parameter_values')
    for v in values:
        if str(v.get_editor_property('parameter_info').get_editor_property('name')) == name:
            return v.get_editor_property('parameter_value')
    return None


def run():
    master = unreal.load_asset(MASTER)
    if master is None:
        raise RuntimeError(f'{MASTER} is missing: run build_world_materials.py M_CharacterGlow first')
    for name, params in table().items():
        instance = find(name)
        if not isinstance(instance, unreal.MaterialInstanceConstant):
            log(f'MI_{name} not found: skipped')
            continue
        color = value(instance, 'vector', 'Color') or unreal.LinearColor(0.1, 0.7, 1.0, 1.0)
        glow = value(instance, 'scalar', 'Glow')
        glow = 4.0 if glow is None else glow
        params = dict(params)
        scale = float(params.pop('GlowScale', 1.0))
        was = instance.get_editor_property('parent')
        on_master = was is not None and was.get_path_name() == master.get_path_name()
        if on_master and 'Glow' not in params:
            # already moved: its Glow is the scaled one, keep it as it is
            scale = 1.0
        glow = float(params.pop('Glow', glow * scale))
        MEL.set_material_instance_parent(instance, master)
        MEL.clear_all_material_instance_parameters(instance)
        MEL.set_material_instance_vector_parameter_value(instance, 'Color', color)
        MEL.set_material_instance_scalar_parameter_value(instance, 'Glow', glow)
        for key, number in params.items():
            MEL.set_material_instance_scalar_parameter_value(instance, key, float(number))
        MEL.update_material_instance(instance)
        unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
        log(f'{instance.get_path_name()}: {"" if on_master else "moved "}on M_CharacterGlow, Glow {glow:g}'
            f'{", " + ", ".join(f"{k} {v:g}" for k, v in params.items()) if params else ""}')
    log('done')


run()
