"""Landfall's material instances (Docs/Work/2026-10-31-Wardens/CONTRACT.md, lane WORLD), made or updated in the open
editor before Tools/Unreal/build_area_landfall.py places what wears them. Each is an instance of a material the game has
already (no new master, no new shader to compile beyond the instances' own permutations), in /Game/Art/Landfall/Materials:

  MI_Landfall_AbelTrail      M_GravewindWisp (unlit, translucent ribbons): Abel's trail, warm gold-white instead of the
                             Gravewind's cold lilac, brighter, panning slower, seen from farther (the Point down the path).
  MI_Landfall_SoulSmog       M_Smoke: soul smog, black-violet and denser than chimney smoke, slower, leaning with the gust
                             over a plume four times the chimney's height.
  MI_Landfall_SoulSmogGlint  M_Smoke: a faint cyan glint in the smog (one thinner plume inside the black).
  MI_Landfall_BargeIron      MI_IronBlack (the World master's worn metal): the barge's riveted iron, darker and rustier.
  MI_Landfall_HoseRubber     MI_PaintBlack: the dredge hose's black rubber.

A parameter an instance names that its parent doesn't have is reported and skipped (set_*_parameter_value returns
False). Saved only when something changed. In the open editor:
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_landfall_materials.py"
"""
import unreal

FOLDER = '/Game/Art/Landfall/Materials'
MASTERS = '/Game/Art/Materials/Masters/'
SHARED = '/Game/Art/Materials/'

# name: (parent, vectors, scalars). Colours linear.
INSTANCES = {
    'MI_Landfall_AbelTrail': (MASTERS + 'M_GravewindWisp', {
        'Cool': (1.0, 0.80, 0.48, 1.0),
        'Warm': (1.0, 0.93, 0.74, 1.0),
    }, {
        'Brightness': 5.0, 'Opacity': 0.55, 'Pan': 1.1, 'ScaleAlong': 0.2,
        'NearStart': 40.0, 'NearEnd': 160.0, 'FarStart': 3500.0, 'FarEnd': 9000.0,
        'Amount': 8.0, 'Speed': 0.9,
    }),
    'MI_Landfall_SoulSmog': (MASTERS + 'M_Smoke', {
        'SmokeColor': (0.05, 0.035, 0.07, 1.0),
    }, {
        'Opacity': 0.6, 'Speed': 0.06, 'PlumeHeight': 3200.0, 'GustLean': 600.0,
    }),
    'MI_Landfall_SoulSmogGlint': (MASTERS + 'M_Smoke', {
        'SmokeColor': (0.22, 0.62, 0.66, 1.0),
    }, {
        'Opacity': 0.1, 'Speed': 0.1, 'PlumeHeight': 2400.0, 'GustLean': 500.0,
    }),
    'MI_Landfall_BargeIron': (SHARED + 'MI_IronBlack', {
        'Tint': (0.30, 0.25, 0.22, 1.0),
    }, {}),
    'MI_Landfall_HoseRubber': (SHARED + 'MI_PaintBlack', {
        'Tint': (0.10, 0.10, 0.11, 1.0),
    }, {}),
}


def make(name, parent_path, vectors, scalars):
    mel = unreal.MaterialEditingLibrary
    parent = unreal.load_asset(parent_path) if unreal.EditorAssetLibrary.does_asset_exist(parent_path) else None
    if parent is None:
        unreal.log_warning(f'landfall materials: no {parent_path}: no {name}')
        return None
    path = f'{FOLDER}/{name}'
    instance = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if instance is None:
        # (A just-deleted asset can still read as existing while it no longer loads: make it again.)
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    changed = instance.get_editor_property('parent') != parent
    if changed:
        mel.set_material_instance_parent(instance, parent)
        # The instance learns its parent's parameters only once refreshed: setting values before that fails quietly.
        mel.update_material_instance(instance)
    for key, value in vectors.items():
        color = unreal.LinearColor(*value)
        old = mel.get_material_instance_vector_parameter_value(instance, key)
        if changed or any(abs(a - b) > 1e-4 for a, b in zip(old.to_tuple(), color.to_tuple())):
            # The setter's own result reads false in 5.8 even when the value took: check by reading it back.
            mel.set_material_instance_vector_parameter_value(instance, key, color)
            back = mel.get_material_instance_vector_parameter_value(instance, key).to_tuple()
            if any(abs(a - b) > 1e-3 for a, b in zip(back, color.to_tuple())):
                unreal.log_warning(f'landfall materials: {parent_path} has no vector {key} ({name})')
                continue
            changed = True
    for key, value in scalars.items():
        if changed or abs(mel.get_material_instance_scalar_parameter_value(instance, key) - value) > 1e-4:
            mel.set_material_instance_scalar_parameter_value(instance, key, value)
            if abs(mel.get_material_instance_scalar_parameter_value(instance, key) - value) > 1e-3:
                unreal.log_warning(f'landfall materials: {parent_path} has no scalar {key} ({name})')
                continue
            changed = True
    if changed:
        mel.update_material_instance(instance)
        unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
        unreal.log(f'landfall materials: {path} saved')
    else:
        unreal.log(f'landfall materials: {path} unchanged')
    return instance


def run():
    made = [name for name, (parent, vectors, scalars) in INSTANCES.items() if make(name, parent, vectors, scalars)]
    unreal.log(f'landfall materials: {len(made)} of {len(INSTANCES)} instances ready ({", ".join(made)})')


if __name__ == '__main__':
    run()
