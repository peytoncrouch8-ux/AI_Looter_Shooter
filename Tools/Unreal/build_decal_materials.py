"""Builds the decals' materials (Docs/Areas/RansomsRest.md, Side 1: the wanted posters) beside the world's masters:

  T_Posters_BC    /Game/Art/Textures/Posters: the posters' atlas, the art session's (Tools/Blender/looter_posters.py), a
                  decal-only set (no _N or _ORM). 1024 px square at 1024 px per metre, so a cell's UV size is its size in
                  metres. Imported from Art/Textures/Posters/T_Posters_BC.png, or in a checkout without it from the
                  stand-in Saved/ArtPreviews/RansomsRest/Posters/T_Posters_BC_B.png (the same layout), again on every run
                  (in place: what uses it stays linked). sRGB, its alpha kept, BC7 (colour with alpha: the note's
                  handwriting stays crisp), mips from the World group, clamped at the edges.
  M_PosterDecal   /Game/Art/Materials/Masters: a deferred decal, translucent. Atlas shows its Cell (U0, V0, U1, V1 of the
                  atlas, V = 0 at the top): the colour times Tint (0.9: paper in full sun reads light; calm it here, not in
                  the art), the alpha cut hard at 0.5 (the art's clip, no soft edge), and Roughness (0.8, matte paper). No
                  normal: the wall's own boards and stones still shade the paper on them. A deferred decal goes into the
                  DBuffer (colour and roughness) before the lighting, so it's lit as its wall is, on Medium without Lumen
                  as on High with it.
  M_PosterScrap   the scrap torn off a poster as it falls: masked and two-sided, its fade dithered (as M_Ghost's, never
                  translucent). Its card takes Cell, BackCell (the scrap's back, mirrored, for back faces) and Fade from
                  its custom primitive data (0-3, 4-7 and 8, as AWantedPoster sets them), so there's no material instance
                  per scrap. The card is square at the cell's longer side: the cell shows in its middle at its own shape,
                  nothing past the cell shows, and it doesn't matter which way the card's UVs run.
  MI_PosterDecal, /Game/Art/Materials: the instances AWantedPoster uses, with the atlas set.
  MI_PosterScrap

It never touches another asset: the masters' graphs are rebuilt in place (their instances stay linked), and an asset at
one of these paths that isn't what this makes is left alone. It reuses build_world_materials.py's graph helpers (without
running that module's build). Run it in the open editor (it needs no C++):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_decal_materials.py"
It prints a DECALMATS line per asset and "DECALMATS done".
"""
import ast
import os
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
INSTANCE_FOLDER = '/Game/Art/Materials'
ATLAS = '/Game/Art/Textures/Posters/T_Posters_BC'
# The installed atlas first; its stand-in only for a checkout without it.
ATLAS_SOURCES = (os.path.join(PROJECT, 'Art', 'Textures', 'Posters', 'T_Posters_BC.png'),
                 os.path.join(PROJECT, 'Saved', 'ArtPreviews', 'RansomsRest', 'Posters', 'T_Posters_BC_B.png'))
DITHER = '/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA'

# The art's cells (AWantedPoster's FWantedPosterAtlas has them all): the masters' defaults show the poster and the scrap.
POSTER_CELL = (0.0, 0.0, 0.5, 0.703125)
SCRAP_CELL = (0.25, 0.703125, 0.5, 0.890625)
SCRAP_BACK_CELL = (0.5, 0.703125, 0.75, 0.890625)
TINT = 0.9
ROUGHNESS = 0.8
# Read from the scrap card's custom primitive data (World/WantedPosterTear.cpp): never an instance's.
PRIMITIVE_DATA = {'Cell': 0, 'BackCell': 4, 'Fade': 8}


def log(message):
    unreal.log(f'DECALMATS {message}')


def world_helpers():
    """build_world_materials.py's helpers (Graph, material, finish, its paths), without its build: that module builds its
    masters as it's run, every one of them when our arguments name none of its, so only its imports, definitions and
    constants are run here."""
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


# --- The atlas ---

def import_atlas():
    """T_Posters_BC from the installed PNG (else its stand-in), with the atlas's settings; its path, or None with neither
    a source nor the asset (the masters then default to white, and the posters show blank paper)."""
    source = next((path for path in ATLAS_SOURCES if os.path.isfile(path)), None)
    if source is not None:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', source)
        task.set_editor_property('destination_path', ATLAS.rsplit('/', 1)[0])
        task.set_editor_property('destination_name', ATLAS.rsplit('/', 1)[1])
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    elif not unreal.EditorAssetLibrary.does_asset_exist(ATLAS):
        unreal.log_warning(f'DECALMATS no atlas: neither {ATLAS_SOURCES[0]} nor its stand-in {ATLAS_SOURCES[1]} exists, '
                           f'and {ATLAS} was never imported. The masters take a plain white texture until it is.')
        return None
    texture = unreal.load_asset(ATLAS)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f'{ATLAS} is not a texture: nothing was changed')
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_BC7)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    # Clamped: the cells on the atlas's edges don't pick up the opposite edge as they're filtered.
    texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    origin = os.path.relpath(source, PROJECT).replace('\\', '/') if source else 'the asset as it was (no source found)'
    log(f'{ATLAS} from {origin}: {texture.blueprint_get_size_x()} x {texture.blueprint_get_size_y()}, sRGB, BC7, clamped')
    return ATLAS


# --- M_PosterDecal ---

DECAL_UV = """// The decal's own 0-1 across the cell's rectangle (U0, V0, U1, V1; V = 0 at the top).
return lerp(Cell.xy, float2(Cell.z, CellV1), UV);"""

HARD_EDGE = """// The art's clip: paper where the alpha is 0.5 or more, nothing under it (no soft edge).
return Alpha >= 0.5 ? 1.0 : 0.0;"""


def master(name):
    """The master by that name, its graph emptied to build again; refuses an asset there that isn't a material."""
    path = f'{W.FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Material):
        raise RuntimeError(f'{path} exists and is not a material: nothing was changed')
    return W.material(name)


def parameter_names(mat):
    scalars = sorted(str(n) for n in MEL.get_scalar_parameter_names(mat))
    vectors = sorted(str(n) for n in MEL.get_vector_parameter_names(mat))
    textures = sorted(str(n) for n in MEL.get_texture_parameter_names(mat))
    return f'scalars {", ".join(scalars)}; colors {", ".join(vectors)}; textures {", ".join(textures)}'


def build_decal(atlas):
    mat = master('M_PosterDecal')
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1400, 0, coordinate_index=0)
    cell = g.vector('Cell', POSTER_CELL, -1400, 150)
    uv = g.custom(DECAL_UV, [('UV', coords, ''), ('Cell', cell, ''), ('CellV1', cell, 'A')],
                  unreal.CustomMaterialOutputType.CMOT_FLOAT2, -1100, 50, 'Atlas cell')
    paper = g.texture('Atlas', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, atlas or W.WHITE, uv, -800, 0)
    color = g.mul(paper, 'RGB', g.vector('Tint', (TINT, TINT, TINT, 1.0), -800, -250), '', -450, -100)
    opacity = g.custom(HARD_EDGE, [('Alpha', paper, 'A')], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -450, 150,
                       'Hard edge')
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    g.out(g.scalar('Roughness', ROUGHNESS, -450, 300), '', unreal.MaterialProperty.MP_ROUGHNESS)
    W.finish(mat, [])
    log(f'{mat.get_path_name()} built (deferred decal, translucent): {parameter_names(mat)}')
    return mat


# --- M_PosterScrap ---

SCRAP_RECT = """// The front shows Cell and the back BackCell; the card is square at the cell's longer side, the cell in its middle.
float4 Rect = TwoSidedSign > 0.0 ? float4(Cell, CellV1) : float4(BackCell, BackCellV1);
float Side = max(Rect.z - Rect.x, Rect.w - Rect.y);
float2 P = (Rect.xy + Rect.zw) * 0.5 + (UV - 0.5) * Side;
"""
SCRAP_UV = SCRAP_RECT + "return P;"
SCRAP_OPACITY = SCRAP_RECT + """// Only the cell shows (the square card reaches past its short sides), cut hard at the art's 0.5, faded as it falls.
float Inside = step(Rect.x, P.x) * step(P.x, Rect.z) * step(Rect.y, P.y) * step(P.y, Rect.w);
return (Alpha >= 0.5 ? 1.0 : 0.0) * Inside * saturate(Fade);"""


def primitive_vector(g, name, default, x, y):
    return g.node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                  default_value=unreal.LinearColor(*default), use_custom_primitive_data=True,
                  primitive_data_index=PRIMITIVE_DATA[name])


def dithered(g, alpha, x, y):
    """The opacity through DitherTemporalAA (TAA smooths the dither into a soft fade), as M_Ghost has it; the plain opacity
    if the function can't be wired."""
    try:
        call = g.node(unreal.MaterialExpressionMaterialFunctionCall, x, y)
        call.set_editor_property('material_function', unreal.load_asset(DITHER))
        g.link(alpha, '', call, 'Alpha Threshold')
        return call, 'Result'
    except Exception as error:   # noqa: BLE001 - the material still works undithered; say so
        unreal.log_warning(f'DECALMATS M_PosterScrap: DitherTemporalAA could not be wired ({error}); its fade is undithered')
        return alpha, ''


def build_scrap(atlas):
    mat = master('M_PosterScrap')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1500, 0, coordinate_index=0)
    side = g.node(unreal.MaterialExpressionTwoSidedSign, -1500, 100)
    cell = primitive_vector(g, 'Cell', SCRAP_CELL, -1500, 200)
    back = primitive_vector(g, 'BackCell', SCRAP_BACK_CELL, -1500, 350)
    fade = g.node(unreal.MaterialExpressionScalarParameter, -1500, 500, parameter_name='Fade', default_value=1.0,
                  use_custom_primitive_data=True, primitive_data_index=PRIMITIVE_DATA['Fade'])
    rect_inputs = [('UV', coords, ''), ('TwoSidedSign', side, ''), ('Cell', cell, ''), ('CellV1', cell, 'A'),
                   ('BackCell', back, ''), ('BackCellV1', back, 'A')]
    uv = g.custom(SCRAP_UV, rect_inputs, unreal.CustomMaterialOutputType.CMOT_FLOAT2, -1150, 100, 'Scrap cell')
    paper = g.texture('Atlas', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, atlas or W.WHITE, uv, -850, 0)
    color = g.mul(paper, 'RGB', g.vector('Tint', (TINT, TINT, TINT, 1.0), -850, -250), '', -500, -100)
    opacity = g.custom(SCRAP_OPACITY, rect_inputs + [('Alpha', paper, 'A'), ('Fade', fade, '')],
                       unreal.CustomMaterialOutputType.CMOT_FLOAT1, -500, 250, 'Scrap opacity')
    mask, mask_out = dithered(g, opacity, -200, 250)
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(g.scalar('Roughness', ROUGHNESS, -500, 450), '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(mask, mask_out, unreal.MaterialProperty.MP_OPACITY_MASK)
    W.finish(mat, [])
    log(f'{mat.get_path_name()} built (masked, two-sided, dithered): {parameter_names(mat)}; '
        f'custom primitive data {PRIMITIVE_DATA}')
    return mat


# --- The instances ---

def instance(name, master_material, atlas):
    """MI_<name> of the master with the atlas set; an asset there that isn't an instance of it is left alone."""
    path = f'{INSTANCE_FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        made = unreal.load_asset(path)
        parent = made.get_editor_property('parent') if isinstance(made, unreal.MaterialInstanceConstant) else None
        if parent is None or parent.get_path_name() != master_material.get_path_name():
            log(f'{path} left alone: it is not an instance of {master_material.get_name()}')
            return
        verb = 'updated'
    else:
        made = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, INSTANCE_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        if made is None:
            raise RuntimeError(f'{path} could not be created')
        MEL.set_material_instance_parent(made, master_material)
        verb = 'made'
    # From scratch each time: the master's defaults, and the atlas.
    MEL.clear_all_material_instance_parameters(made)
    if atlas:
        MEL.set_material_instance_texture_parameter_value(made, 'Atlas', unreal.load_asset(atlas))
    MEL.update_material_instance(made)
    unreal.EditorAssetLibrary.save_loaded_asset(made, only_if_is_dirty=False)
    log(f'{path} {verb}: {master_material.get_name()} with {"the atlas" if atlas else "no atlas yet (white)"}')


def run():
    atlas = import_atlas()
    decal = build_decal(atlas)
    scrap = build_scrap(atlas)
    instance('MI_PosterDecal', decal, atlas)
    instance('MI_PosterScrap', scrap, atlas)
    log('done')


run()
