"""The gun makers' assets (Docs/Work/2026-10-22-Next/MAKERS.md). The makers themselves are C++ (Affixes/WeaponMakers:
their keys, names, paints and marks; their numbers are Looter.Makers.* cvars), so this only brings in what the gun
master needs and checks the rest:
  T_GunMakerMarks   the marks' mask atlas (2x2: Roundhouse, Saint Maud, Windfall, Meridian), from
                    Art/Textures/GunMakerMarks/T_GunMakerMarks.png (python Art/Icons/MakerMarks.py makes it), into
                    /Game/Art/Weapons/Textures, linear and uncompressed (block compression smears a mark 4 cm across).
Then it reports whether the definitions' parts carry their mark places (setup_gun_parts.py's MARKS: run that again after
the C++ build) and whether M_Gun reads the mark (MakerMarks parameter: build_world_materials.py M_Gun).

Run it in the open editor after the C++ with the makers is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_manufacturers.py"
then setup_gun_parts.py, then build_world_materials.py M_Gun. It prints MAKERS lines and "MAKERS done"; running it
again re-imports the atlas over itself.
"""
import os

import unreal

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ATLAS_FILE = os.path.join(REPO, 'Art', 'Textures', 'GunMakerMarks', 'T_GunMakerMarks.png')
ATLAS = '/Game/Art/Weapons/Textures/T_GunMakerMarks'
DATA = '/Game/Weapons/Data'
GUNS = ('DA_AssaultRifle', 'DA_PumpShotgun', 'DA_Revolver', 'DA_SMG', 'DA_LMG', 'DA_Sniper', 'DA_Launcher')
GUN_MASTER = '/Game/Art/Materials/Masters/M_Gun'


def import_atlas():
    if not os.path.exists(ATLAS_FILE):
        raise RuntimeError(f'{ATLAS_FILE} is missing: run python Art/Icons/MakerMarks.py first')
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', ATLAS_FILE)
    task.set_editor_property('destination_path', ATLAS.rsplit('/', 1)[0])
    task.set_editor_property('destination_name', ATLAS.rsplit('/', 1)[1])
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(ATLAS)
    if texture is None:
        raise RuntimeError(f'{ATLAS} did not import')
    texture.set_editor_property('srgb', False)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_GRAYSCALE)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    unreal.log(f'MAKERS atlas {ATLAS} ({texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()})')


def check_marks():
    if not hasattr(unreal, 'WeaponMarkPlace'):
        unreal.log_warning('MAKERS the makers C++ is not built yet (no WeaponMarkPlace)')
        return
    for asset in GUNS:
        definition = unreal.load_asset(f'{DATA}/{asset}')
        if definition is None:
            unreal.log_warning(f'MAKERS {asset} missing')
            continue
        marked = [slot.get_editor_property('name') for slot in definition.get_editor_property('parts')
                  if any(option.get_editor_property('mark_place').get_editor_property('size') > 0.0
                         for option in slot.get_editor_property('options'))]
        if marked:
            unreal.log(f'MAKERS {asset}: marked on {", ".join(str(n) for n in marked)}')
        else:
            unreal.log_warning(f'MAKERS {asset}: no part carries the mark: run setup_gun_parts.py')


def check_master():
    master = unreal.load_asset(GUN_MASTER)
    names = [str(n) for n in unreal.MaterialEditingLibrary.get_texture_parameter_names(master)] if master else []
    if 'MakerMarks' in names:
        unreal.log('MAKERS M_Gun paints the mark')
    else:
        unreal.log_warning('MAKERS M_Gun has no MakerMarks parameter yet: build_world_materials.py M_Gun (with the makers patch)')


import_atlas()
check_marks()
check_master()
unreal.log('MAKERS done')
