"""Imports the heroes' painted portraits (the user's picks, 2026-10-19) as UI textures for the hero select and the session
cards: Art/Portraits/Picks/<Hero>.png -> /Game/UI/Portraits/T_Portrait_<Id> (sRGB, BC7, UI group, never streamed, clamped).
They're paintings, not mockup vector art, so textures are right for them (CLAUDE.md's UI rules keep vector art in code).

The picks, copied in from Saved/ArtPreviews/Portraits/<Hero>/<Hero><key>.png: Ellis A, Crane A, Pike A, Gauge C, Odessa A
(the busts the user chose; their painted frame and name plate are part of the picture). A new pick: copy its PNG over
the hero's and run this again (it replaces the texture in place, so the hero definitions keep pointing at it).

Run it in the open editor, then Tools/Unreal/create_ember_assets.py (which points each DA_Hero_<Id> at its portrait):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/import_hero_portraits.py"
It prints a PORTRAITS line per texture and "PORTRAITS done".
"""
import os

import unreal

PROJECT = os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
SOURCE = os.path.join(PROJECT, 'Art', 'Portraits', 'Picks')
FOLDER = '/Game/UI/Portraits'
HEROES = ['Ellis', 'Odessa', 'Crane', 'Gauge', 'Pike']

# Mipmaps: the contract asked for none (a UI texture); the session cards draw the face about a tenth of its painted size,
# where a picture without mips sparkles. MIPS True gives the small draws a filtered copy at the cost of a third more memory.
MIPS = False


def log(message):
    unreal.log(f'PORTRAITS {message}')


def import_portrait(hero):
    source = os.path.join(SOURCE, f'{hero}.png')
    name = f'T_Portrait_{hero}'
    path = f'{FOLDER}/{name}'
    if not os.path.isfile(source):
        unreal.log_warning(f'PORTRAITS {hero}: {source} is missing; {path} is left as it is')
        return False
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', source)
    task.set_editor_property('destination_path', FOLDER)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f'{path} is not a texture after its import: nothing more was changed')
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_BC7)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings',
                                unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE if MIPS else unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('never_stream', True)
    # Clamped: a cut of the picture (the roster's band, a card's face) never picks up the opposite edge as it's filtered.
    texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError(f'{path} could not be saved')
    log(f'{path} from Art/Portraits/Picks/{hero}.png: {texture.blueprint_get_size_x()} x {texture.blueprint_get_size_y()}, '
        f'sRGB, BC7, UI group, {"mips" if MIPS else "no mips"}')
    return True


def run():
    made = sum(1 for hero in HEROES if import_portrait(hero))
    log(f'done ({made} of {len(HEROES)})')


run()
