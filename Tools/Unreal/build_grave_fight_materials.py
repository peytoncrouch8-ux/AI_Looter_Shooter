"""Builds the Gravemother's Barrow fight materials (Saved/Gravemother/CONTRACT.md, BARROW-ART; Saved/Gravemother/PLAN.md
6 and 7 read them) beside the world's masters:

  Textures   /Game/Art/Effects/GraveFight/T_GraveFight_*: Art/Textures/GraveFight/make_grave_fight.py's PNGs, imported
             again whenever a PNG changes (build_world_materials.py's import_texture keeps its MD5 on the asset). Cards and
             decal colour: sRGB, BC7 (soft alpha). Normal maps: normal maps. ORM: masks. Clamped, but the two strips that
             tile along U (HeaveCrack, Furrow) wrap in U.
  M_GraveDecal      /Game/Art/Materials/Masters: a deferred decal, translucent (DBuffer colour, normal, roughness: lit as
                    the ground it lies on, on Medium without Lumen as on High). DecalColor (its colour, alpha the opacity),
                    DecalNormal, DecalORM; Tiles (U and V repeats, U and V offsets) on the decal's own UV (U along the
                    strip). Opacity = alpha x Opacity (the code drives it through a MID) x Reveal's front (the part of U
                    up to Reveal shows, its front soft over RevealSoft; 1 shows it all: the heave's crawl) x the ends'
                    fades (FadeIn from U = 0, FadeOut to U = 1, shares of U; 0 none: a strip's first and last tiles) x
                    AlphaScale (the picture's alpha scaled: the landing shadow's 1.8) x the decal's lifetime fade x its
                    component's DecalColor alpha, only on surfaces facing back up its axis
                    (never smeared down a wall or a mound's steep side). Tint, NormalStrength.
  M_FX_GraveCard    /Game/Art/Materials/Masters: unlit, translucent, two-sided: a dust card (the engine's Plane, UVs 0-1)
                    drawn as instanced quads the code turns to the camera. Per-instance custom data (the code's layout):
                    0-2 the card's colour (linear; its tint, the light already in it: the material is unlit), 3 its
                    opacity, 4 its atlas cell (row by row from the top left; Columns x Rows set per instance of the
                    master). Colour = Sprite's shade x the colour x Tint (white); opacity = Sprite's alpha x 3 x Opacity,
                    soft where it meets what's behind it (SoftDepth cm). No lighting cost.
  M_FX_GraveGlow    /Game/Art/Materials/Masters: unlit, additive, two-sided: a spark's streak or a flash (as M_FX_Glow).
                    Custom data 0-2 its colour, 3 its strength; Sprite's colour x its alpha x those x Glow.
  MI_Decal_HeaveCrack, MI_Decal_HeaveCrackEnd, MI_Decal_Furrow, MI_Decal_ImpactScar, MI_Decal_LandingShadow,
  MI_Decal_MoundCracks         /Game/Art/Effects/GraveFight: M_GraveDecal with their maps
  MI_FX_GraveDust (the 2x2 earth puffs), MI_FX_FloorDust (the 2x2 floor puffs), MI_FX_GraveDustJet (two jets, 2 x 1)
                               /Game/Art/Effects/GraveFight: M_FX_GraveCard; their colours come from the code (#806B55
                               earth dust, #DCC9A4 floor dust, #D2CDC3 stone dust on GraveDust)
  MI_FX_StoneSpark, MI_FX_GraveFlash   /Game/Art/Effects/GraveFight: M_FX_GraveGlow (the streak's head at U = 1)
  MI_GravemotherBare           /Game/Art/Materials: M_World with the GravemotherBare maps (Art/Models/Creatures/
                               Gravemother.py paints them): SK_Gravemother's GravemotherAbdomen slot once the stone is off,
                               her abdomen all pale Bone (optional: SetMaterialByName("GravemotherAbdomen", it)).

It never touches another asset: the masters' graphs are rebuilt in place (their instances stay linked), and an asset at
one of these paths that isn't what this makes is left alone. It reuses build_world_materials.py's graph helpers (without
running that module's build), as build_stain_decal.py does. Run it in the open editor (no C++ needed), after
make_grave_fight.py and Gravemother.py have painted their textures:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_grave_fight_materials.py"
It prints GRAVEFIGHTMATS lines and "GRAVEFIGHTMATS done".
"""
import ast
import os
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE = os.path.join(PROJECT, 'Art', 'Textures', 'GraveFight')
FOLDER = '/Game/Art/Effects/GraveFight'
BARE_SET = 'GravemotherBare'
BARE_INSTANCE = ('/Game/Art/Materials', 'MI_GravemotherBare')
WORLD_MASTER = '/Game/Art/Materials/Masters/M_World'


def log(message):
    unreal.log(f'GRAVEFIGHTMATS {message}')


def world_helpers():
    """build_world_materials.py's helpers (Graph, material, finish, import_texture, its paths), without its build: that
    module builds its masters as it's run, so only its imports, definitions and constants are run here."""
    path = os.path.join(HERE, 'build_world_materials.py')
    with open(path, encoding='utf-8') as source:
        tree = ast.parse(source.read(), path)
    keep = []
    for node in tree.body:
        if isinstance(node, (ast.Import, ast.ImportFrom, ast.FunctionDef, ast.ClassDef)):
            keep.append(node)
        elif isinstance(node, ast.Assign) and all(isinstance(t, ast.Name) and t.id.isupper() for t in node.targets):
            keep.append(node)
    module = types.ModuleType('build_world_materials_helpers')
    module.__file__ = path
    exec(compile(ast.Module(body=keep, type_ignores=[]), path, 'exec'), module.__dict__)
    return module


W = world_helpers()
MEL = W.MEL

# --- Textures ---

CARDS = ('DustPuffs', 'FloorPuffs', 'DustJet', 'SparkStreak', 'Flash')
DECALS = ('HeaveCrack', 'HeaveCrackEnd', 'Furrow', 'ImpactScar', 'LandingShadow', 'MoundCracks')
WRAP_U = ('HeaveCrack', 'Furrow')
# Per decal, its instance's own numbers: the clod's landing shadow (its picture's alpha at most 0.5) darker, so it reads
# through the grass as the telegraph it is (2026-10-20: at the old 0.85 opacity it was hardly seen in the west field).
DECAL_SCALARS = {'LandingShadow': {'AlphaScale': 1.8}}


def texture(file_name, kind, wrap_u=False):
    """One of make_grave_fight.py's PNGs at FOLDER (its path), or None while the PNG is missing."""
    file = os.path.join(SOURCE, file_name + '.png')
    if not os.path.isfile(file):
        unreal.log_error(f'GRAVEFIGHTMATS {file} is missing: run Tools\\artrun.ps1 -Script '
                         f'Art\\Textures\\GraveFight\\make_grave_fight.py first')
        return None
    path = f'{FOLDER}/{file_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Texture2D):
        raise RuntimeError(f'{path} exists and is not a texture: nothing was changed')
    tex = W.import_texture(file.replace('\\', '/'), path, {'BC': 'BC', 'N': 'N'}.get(kind, 'M'))
    if kind == 'BC':
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_BC7)
    elif kind == 'ORM':
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
    tex.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP if wrap_u else unreal.TextureAddress.TA_CLAMP)
    tex.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
    unreal.EditorAssetLibrary.save_loaded_asset(tex, only_if_is_dirty=False)
    return path


def parameter_names(mat):
    scalars = sorted(str(n) for n in MEL.get_scalar_parameter_names(mat))
    vectors = sorted(str(n) for n in MEL.get_vector_parameter_names(mat))
    textures = sorted(str(n) for n in MEL.get_texture_parameter_names(mat))
    return f'scalars {", ".join(scalars)}; colors {", ".join(vectors)}; textures {", ".join(textures)}'


def master(name):
    """A master by that name, its graph emptied to build again; refuses an asset there that isn't a material."""
    path = f'{W.FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Material):
        raise RuntimeError(f'{path} exists and is not a material: nothing was changed')
    return W.material(name)


# --- M_GraveDecal ---

DECAL_UV = 'return UV * Tiles.xy + Tiles.zw;'
DECAL_FACING = """float3 N = normalize(cross(ddx(P), ddy(P)));
N *= dot(N, V) < 0.0 ? -1.0 : 1.0;
return smoothstep(0.2, 0.5, dot(N, -normalize(Axis)));"""
DECAL_OPACITY = """// The part of the strip up to Reveal shows (its front soft over RevealSoft of U); at 1 all of it.
float Front = saturate((Reveal - UV.x) / max(RevealSoft, 1e-3) + 1.0);
// A strip's first and last tiles fade in from U = 0 over FadeIn and out to U = 1 over FadeOut (0: cut square, where one
// tile meets the next), so the strip doesn't end in a straight edge.
float Ends = (FadeIn > 0.0 ? smoothstep(0.0, FadeIn, UV.x) : 1.0) * (FadeOut > 0.0 ? smoothstep(0.0, FadeOut, 1.0 - UV.x) : 1.0);
return saturate(Alpha * AlphaScale * Strength * Front * Ends * Facing * Life * Code.a);"""
DECAL_NORMAL = 'return normalize(float3(N.xy * Strength, N.z));'


def build_decal(defaults):
    mat = master('M_GraveDecal')
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1900, 0, coordinate_index=0)
    tiles = g.vector('Tiles', (1.0, 1.0, 0.0, 0.0), -1900, 120)
    # Its RGBA output: the first (unnamed) one is RGB only, and the offsets are in .zw (a float3 failed to compile).
    uv = g.custom(DECAL_UV, [('UV', coords, ''), ('Tiles', tiles, 'RGBA')], unreal.CustomMaterialOutputType.CMOT_FLOAT2,
                  -1600, 50, 'Tiles')
    bc = g.texture('DecalColor', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, defaults['BC'], uv, -1200, -250)
    nrm = g.texture('DecalNormal', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, defaults['N'], uv, -1200, 50)
    orm = g.texture('DecalORM', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, defaults['ORM'], uv, -1200, 350)
    world = g.node(unreal.MaterialExpressionWorldPosition, -1600, 650)
    try:
        world.set_editor_property('world_position_shader_offset', unreal.WorldPositionIncludedOffsets.WPT_CAMERA_RELATIVE)
    except Exception as error:  # noqa: BLE001 - the absolute position works too, a little less precisely far off
        unreal.log_warning(f'GRAVEFIGHTMATS M_GraveDecal: camera-relative world position not set ({error})')
    facing = g.custom(DECAL_FACING, [
        ('P', world, ''), ('V', g.node(unreal.MaterialExpressionCameraVectorWS, -1600, 750), ''),
        ('Axis', g.node(unreal.MaterialExpressionObjectOrientation, -1600, 850), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1200, 700, 'Facing')
    code = g.node(unreal.MaterialExpressionDecalColor, -1600, 950)
    opacity = g.custom(DECAL_OPACITY, [
        ('UV', coords, ''), ('Alpha', bc, 'A'), ('AlphaScale', g.scalar('AlphaScale', 1.0, -1200, 1500), ''),
        ('Strength', g.scalar('Opacity', 1.0, -1200, 900), ''),
        ('Reveal', g.scalar('Reveal', 1.0, -1200, 1000), ''), ('RevealSoft', g.scalar('RevealSoft', 0.03, -1200, 1100), ''),
        ('FadeIn', g.scalar('FadeIn', 0.0, -1200, 1300), ''), ('FadeOut', g.scalar('FadeOut', 0.0, -1200, 1400), ''),
        ('Facing', facing, ''), ('Life', g.node(unreal.MaterialExpressionDecalLifetimeOpacity, -1200, 1200), ''),
        ('Code', code, 'RGBA'),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -700, 700, 'Opacity')
    tint = g.mul(g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1200, -450), '', code, '', -900, -450)
    color = g.mul(bc, 'RGB', tint, '', -700, -300)
    normal = g.custom(DECAL_NORMAL, [('N', nrm, 'RGB'), ('Strength', g.scalar('NormalStrength', 1.0, -1200, 200), '')],
                      unreal.CustomMaterialOutputType.CMOT_FLOAT3, -700, 50, 'Normal strength')
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    g.out(normal, '', unreal.MaterialProperty.MP_NORMAL)
    g.out(orm, 'G', unreal.MaterialProperty.MP_ROUGHNESS)
    W.finish(mat, [])
    log(f'{mat.get_path_name()} built (deferred decal, translucent): {parameter_names(mat)}')
    return mat


# --- The cards ---

CARD_UV = """// The card's own 0-1 into its cell of the sheet (Grid.x columns, Grid.y rows; cells counted row by row from the
// top left, as the sheet is painted).
float Count = max(Grid.x * Grid.y, 1.0);
float Cell = clamp(floor(CellIn + 0.5), 0.0, Count - 1.0);
float2 Corner = float2(fmod(Cell, Grid.x), floor(Cell / Grid.x));
return (Corner + saturate(UV)) / Grid.xy;"""


def custom_data(g, index, default, x, y):
    return g.node(unreal.MaterialExpressionPerInstanceCustomData, x, y, data_index=index, const_default_value=default)


def card_inputs(g, defaults):
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1900, 0, coordinate_index=0)
    grid = g.vector('Grid', (2.0, 2.0, 0.0, 0.0), -1900, 120)
    uv = g.custom(CARD_UV, [('UV', coords, ''), ('Grid', grid, ''), ('CellIn', custom_data(g, 4, 0.0, -1900, 250), '')],
                  unreal.CustomMaterialOutputType.CMOT_FLOAT2, -1600, 50, 'Atlas cell')
    sprite = g.texture('Sprite', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, defaults, uv, -1200, 0)
    rgb = g.node(unreal.MaterialExpressionAppendVector, -1500, 400)
    rg = g.node(unreal.MaterialExpressionAppendVector, -1700, 400)
    g.link(custom_data(g, 0, 1.0, -1900, 380), '', rg, 'A')
    g.link(custom_data(g, 1, 1.0, -1900, 460), '', rg, 'B')
    g.link(rg, '', rgb, 'A')
    g.link(custom_data(g, 2, 1.0, -1900, 540), '', rgb, 'B')
    return sprite, rgb


def build_card(defaults):
    mat = master('M_FX_GraveCard')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)
    sprite, rgb = card_inputs(g, defaults)
    tint = g.mul(rgb, '', g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1500, 600), '', -1200, 450)
    color = g.mul(sprite, 'RGB', tint, '', -800, 100)
    alpha = g.mul(sprite, 'A', custom_data(g, 3, 1.0, -1500, 750), '', -1200, 750)
    alpha = g.mul(alpha, '', g.scalar('Opacity', 1.0, -1500, 850), '', -1000, 750)
    fade = W.depth_fade(g, g.scalar('SoftDepth', 40.0, -1200, 950), -900, 950)
    g.link(alpha, '', fade, 'Opacity')
    g.out(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(fade, '', unreal.MaterialProperty.MP_OPACITY)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES])
    log(f'{mat.get_path_name()} built (unlit, translucent, two-sided): {parameter_names(mat)}; per-instance custom data '
        f'0-2 colour, 3 opacity, 4 cell')
    return mat


def build_glow(defaults):
    mat = master('M_FX_GraveGlow')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)
    sprite, rgb = card_inputs(g, defaults)
    lit = g.mul(sprite, 'RGB', sprite, 'A', -900, 0)
    strength = g.mul(custom_data(g, 3, 1.0, -1500, 750), '', g.scalar('Glow', 1.0, -1500, 850), '', -1200, 750)
    color = g.mul(g.mul(lit, '', rgb, '', -700, 100), '', strength, '', -500, 200)
    g.out(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES])
    log(f'{mat.get_path_name()} built (unlit, additive, two-sided): {parameter_names(mat)}; per-instance custom data '
        f'0-2 colour, 3 strength')
    return mat


# --- The instances ---

def instance(folder, name, parent, textures=None, scalars=None, vectors=None):
    """MI_<name> of parent with its settings, from scratch each time; an asset there that isn't an instance of parent
    is left alone."""
    path = f'{folder}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        made = unreal.load_asset(path)
        current = made.get_editor_property('parent') if isinstance(made, unreal.MaterialInstanceConstant) else None
        if current is None or current.get_path_name() != parent.get_path_name():
            log(f'{path} left alone: it is not an instance of {parent.get_name()}')
            return None
        verb = 'updated'
    else:
        made = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        if made is None:
            raise RuntimeError(f'{path} could not be created')
        MEL.set_material_instance_parent(made, parent)
        verb = 'made'
    MEL.clear_all_material_instance_parameters(made)
    for key, value in (textures or {}).items():
        MEL.set_material_instance_texture_parameter_value(made, key, unreal.load_asset(value))
    for key, value in (scalars or {}).items():
        MEL.set_material_instance_scalar_parameter_value(made, key, value)
    for key, value in (vectors or {}).items():
        MEL.set_material_instance_vector_parameter_value(made, key, unreal.LinearColor(*value))
    MEL.update_material_instance(made)
    unreal.EditorAssetLibrary.save_loaded_asset(made, only_if_is_dirty=False)
    log(f'{path} {verb}: {parent.get_name()}')
    return made


def bare_abdomen():
    """MI_GravemotherBare: M_World with GravemotherBare's three maps, set as the model importer sets a set's (colour
    sRGB, the normal map, the ORM as masks), Tint white, UVScale 1."""
    folder = os.path.join(PROJECT, 'Art', 'Textures', BARE_SET)
    maps = {}
    for param, kind in (('BaseColorMap', 'BC'), ('NormalMap', 'N'), ('ORMMap', 'ORM')):
        file = os.path.join(folder, f'T_{BARE_SET}_{kind}.png')
        if not os.path.isfile(file):
            unreal.log_error(f'GRAVEFIGHTMATS {file} is missing: run Tools\\artrun.ps1 -Script '
                             f'Art\\Models\\Creatures\\Gravemother.py first; MI_GravemotherBare not made')
            return
        path = f'/Game/Art/Textures/{BARE_SET}/T_{BARE_SET}_{kind}'
        tex = W.import_texture(file.replace('\\', '/'), path, {'BC': 'BC', 'N': 'N'}.get(kind, 'M'))
        if kind == 'ORM':
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
            unreal.EditorAssetLibrary.save_loaded_asset(tex, only_if_is_dirty=False)
        maps[param] = path
    instance(BARE_INSTANCE[0], BARE_INSTANCE[1], unreal.load_asset(WORLD_MASTER), textures=maps,
             scalars={'UVScale': 1.0}, vectors={'Tint': (1.0, 1.0, 1.0, 1.0)})


def run():
    unreal.EditorAssetLibrary.make_directory(FOLDER)
    cards = {name: texture(f'T_GraveFight_{name}', 'BC') for name in CARDS}
    decals = {name: {kind: texture(f'T_GraveFight_{name}_{kind}', kind, wrap_u=name in WRAP_U)
                     for kind in ('BC', 'N', 'ORM')} for name in DECALS}
    W.default_orm()
    decal_master = build_decal({'BC': W.WHITE, 'N': W.FLAT_NORMAL, 'ORM': W.DEFAULT_ORM})
    card_master = build_card(W.WHITE)
    glow_master = build_glow(W.WHITE)
    for name in DECALS:
        maps = decals[name]
        if None in maps.values():
            unreal.log_error(f'GRAVEFIGHTMATS MI_Decal_{name} not made: its maps are missing')
            continue
        instance(FOLDER, f'MI_Decal_{name}', decal_master,
                 textures={'DecalColor': maps['BC'], 'DecalNormal': maps['N'], 'DecalORM': maps['ORM']},
                 scalars=DECAL_SCALARS.get(name))
    for mi, sheet, grid, master_mat in (('MI_FX_GraveDust', 'DustPuffs', (2.0, 2.0), card_master),
                                        ('MI_FX_FloorDust', 'FloorPuffs', (2.0, 2.0), card_master),
                                        ('MI_FX_GraveDustJet', 'DustJet', (2.0, 1.0), card_master),
                                        ('MI_FX_StoneSpark', 'SparkStreak', (1.0, 1.0), glow_master),
                                        ('MI_FX_GraveFlash', 'Flash', (1.0, 1.0), glow_master)):
        if cards[sheet] is None:
            unreal.log_error(f'GRAVEFIGHTMATS {mi} not made: T_GraveFight_{sheet} is missing')
            continue
        instance(FOLDER, mi, master_mat, textures={'Sprite': cards[sheet]},
                 vectors={'Grid': (grid[0], grid[1], 0.0, 0.0)})
    bare_abdomen()
    log('done')


run()
