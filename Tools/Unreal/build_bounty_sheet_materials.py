"""Imports the bounty board's paper and makes its decal instances (Tools/Blender/looter_bounty_sheets.py draws it):

  T_BountySheets_BC  /Game/Art/Textures/Bounty, from Art/Textures/Bounty/T_BountySheets_BC.png (2048 x 2048: the four
                     kinds' sheets). Imported with the posters' settings: sRGB, alpha kept, BC7, World mips, clamped.
  T_BountyNames_BC   the same for the 32 printed names (2048 x 512).
  MI_BountySheetDecal, MI_BountyNameDecal   /Game/Art/Materials: instances of M_PosterDecal (the posters' deferred
                     decal master, build_decal_materials.py) with Atlas set to those textures. ABountyBoard makes a
                     dynamic instance of each per sheet and sets "Cell" to the sheet's or the name's rectangle.

Run it in the open editor after build_decal_materials.py has made M_PosterDecal:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_bounty_sheet_materials.py"
It prints a BOUNTYSHEETS line per asset and "BOUNTYSHEETS done". Nothing else is touched; assets already there are
re-imported or re-pointed in place (what uses them stays linked).
"""
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
TEXTURE_FOLDER = '/Game/Art/Textures/Bounty'
MATERIAL_FOLDER = '/Game/Art/Materials'
PARENT = '/Game/Art/Materials/Masters/M_PosterDecal'

TEXTURES = {
    'T_BountySheets_BC': os.path.join(PROJECT, 'Art', 'Textures', 'Bounty', 'T_BountySheets_BC.png'),
    'T_BountyNames_BC': os.path.join(PROJECT, 'Art', 'Textures', 'Bounty', 'T_BountyNames_BC.png'),
}
INSTANCES = {'MI_BountySheetDecal': 'T_BountySheets_BC', 'MI_BountyNameDecal': 'T_BountyNames_BC'}


def log(message):
    unreal.log(f'BOUNTYSHEETS {message}')


def import_texture(name, source):
    asset = f'{TEXTURE_FOLDER}/{name}'
    if os.path.isfile(source):
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', source)
        task.set_editor_property('destination_path', TEXTURE_FOLDER)
        task.set_editor_property('destination_name', name)
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    elif not unreal.EditorAssetLibrary.does_asset_exist(asset):
        raise RuntimeError(f'{source} is missing and {asset} was never imported: run looter_bounty_sheets.py first')
    texture = unreal.load_asset(asset)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f'{asset} is not a texture: nothing was changed')
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_BC7)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    log(f'{asset}: {texture.blueprint_get_size_x()} x {texture.blueprint_get_size_y()}, sRGB, BC7, clamped')
    return texture


def make_instance(name, texture):
    path = f'{MATERIAL_FOLDER}/{name}'
    parent = unreal.load_asset(PARENT)
    if not isinstance(parent, unreal.MaterialInterface):
        raise RuntimeError(f'{PARENT} is missing: run build_decal_materials.py first')
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        instance = unreal.load_asset(path)
        if not isinstance(instance, unreal.MaterialInstanceConstant):
            raise RuntimeError(f'{path} exists and is not a material instance: nothing was changed')
    else:
        factory = unreal.MaterialInstanceConstantFactoryNew()
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MATERIAL_FOLDER,
                                                                           unreal.MaterialInstanceConstant, factory)
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(instance, 'Atlas', texture)
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    log(f'{path} -> Atlas {texture.get_path_name()}')


def main():
    textures = {name: import_texture(name, source) for name, source in TEXTURES.items()}
    for name, texture_name in INSTANCES.items():
        make_instance(name, textures[texture_name])
    log('done')


main()
