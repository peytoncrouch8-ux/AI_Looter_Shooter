"""Builds the cliff stains' decal material (the "make it alive" polish, concept B: varnish streaks, seeps and whitewash down
Ransom's Rest's sandstone faces) beside the world's masters:

  T_CliffStains_BC, _ORM  /Game/Art/Textures/CliffStains: the atlas, script-made by
                  Art/Textures/CliffStains/make_cliff_stains.py (8 cells side by side, each 256 x 1024 px, top row = the
                  streak's top; BC sRGB with the opacity in alpha; ORM linear: R occlusion (unused), G roughness, B
                  metallic). Imported again whenever a PNG changes (build_world_materials.py's import_texture: its MD5 is
                  kept on the asset), the BC as colour (sRGB, its alpha kept), the ORM as masks (TC_MASKS), both clamped
                  at the atlas's edges.
  M_CliffStain    /Game/Art/Materials/Masters: a deferred decal, translucent (DBuffer colour and roughness: drawn before
                  the lighting, so it's lit as the rock under it, on Medium's Nanite fallback without Lumen as on High).
                  No normal: varnish is a film and the bloom a crust, so the face's own relief shades them. The decal's
                  colour picks the cell, R = (cell + 0.5) / 8 (floor(R x 8)), and its A scales the opacity (each stain's
                  strength). Base colour StainColor's RGB x Tint; opacity its alpha x A x Opacity, only on surfaces
                  facing back up the decal's axis (M_BulletMark's MARK_FACING: never the top of a ledge or a lip inside
                  the box, never a face it would only smear along); roughness StainORM's G.
                  The placer (build_area_cliff_life.py) turns each decal X into the face, Y up: Unreal's decal UVs are
                  U = local Z, V = -local Y, so V runs down the face and the cell's top row is the decal's top. Should
                  the stains show sideways or upside down in the level, set SwapUV, FlipU or FlipV (0 or 1) on
                  MI_CliffStain rather than placing them again (a rebuild here keeps them).
  MI_CliffStain   /Game/Art/Materials: the instance the decals wear, with the atlas set.

It never touches another asset: the master's graph is rebuilt in place (the instance stays linked), and an asset at one of
these paths that isn't what this makes is left alone. It reuses build_world_materials.py's graph helpers and MARK_FACING
(without running that module's build), as build_stain_decal.py does. Run it in the open editor (it needs no C++), after
the atlas is made (Tools\\artrun.ps1 -Script Art\\Textures\\CliffStains\\make_cliff_stains.py):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_cliff_stain_material.py" -Until "CLIFFSTAIN done"
It prints CLIFFSTAIN lines and "CLIFFSTAIN done".
"""
import ast
import os
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
INSTANCE_FOLDER = '/Game/Art/Materials'
SET = 'CliffStains'
MAPS = {'BC': 'StainColor', 'ORM': 'StainORM'}
CELLS = 8

# The cell's UV in the atlas: the decal's own 0-1 (turned or flipped when asked), squeezed into its cell's eighth across.
CELL_UV = f"""float2 T = saturate(Swap > 0.5 ? UV.yx : UV);
T.x = FlipU > 0.5 ? 1.0 - T.x : T.x;
T.y = FlipV > 0.5 ? 1.0 - T.y : T.y;
float Cell = clamp(floor(Code.r * {CELLS:.1f}), 0.0, {CELLS - 1:.1f});
return float2((Cell + T.x) / {CELLS:.1f}, T.y);"""
OPACITY = 'return saturate(Alpha * Facing * Code.a * Strength);'


def log(message):
    unreal.log(f'CLIFFSTAIN {message}')


def world_helpers():
    """build_world_materials.py's helpers (Graph, material, finish, import_texture, MARK_FACING, its paths), without its
    build: that module builds its masters as it's run, so only its imports, definitions and constants are run here."""
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

def import_maps():
    """The atlas's two maps at /Game/Art/Textures/CliffStains, clamped; their paths by kind, or None while a PNG is
    missing (make_cliff_stains.py hasn't been run): no material then, since a stand-in would lay white boxes on the rock."""
    folder = os.path.join(PROJECT, 'Art', 'Textures', SET)
    files = {kind: os.path.join(folder, f'T_{SET}_{kind}.png') for kind in MAPS}
    missing = [kind for kind, file in files.items() if not os.path.isfile(file)]
    if missing:
        unreal.log_error(f'CLIFFSTAIN not built: {", ".join(missing)} missing from {folder}; run '
                         f'Tools\\artrun.ps1 -Script Art\\Textures\\{SET}\\make_cliff_stains.py first')
        return None
    paths = {}
    for kind, file in files.items():
        path = f'/Game/Art/Textures/{SET}/T_{SET}_{kind}'
        if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Texture2D):
            raise RuntimeError(f'{path} exists and is not a texture: nothing was changed')
        # 'BC': colour, sRGB, its alpha kept (TC_DEFAULT); 'M': a linear mask, made TC_MASKS below.
        texture = W.import_texture(file.replace('\\', '/'), path, 'BC' if kind == 'BC' else 'M')
        if kind == 'ORM':
            texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
        # Clamped: the end cells never pick up the atlas's other end as they're filtered.
        texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
        texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
        unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
        log(f'{path}: {texture.blueprint_get_size_x()} x {texture.blueprint_get_size_y()}, '
            f'{"sRGB colour with alpha" if kind == "BC" else "linear masks"}, clamped')
        paths[kind] = path
    return paths


# --- M_CliffStain ---

def master():
    """M_CliffStain, its graph emptied to build again; refuses an asset at its path that isn't a material."""
    path = f'{W.FOLDER}/M_CliffStain'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Material):
        raise RuntimeError(f'{path} exists and is not a material: nothing was changed')
    return W.material('M_CliffStain')


def parameter_names(mat):
    scalars = sorted(str(n) for n in MEL.get_scalar_parameter_names(mat))
    vectors = sorted(str(n) for n in MEL.get_vector_parameter_names(mat))
    textures = sorted(str(n) for n in MEL.get_texture_parameter_names(mat))
    return f'scalars {", ".join(scalars)}; colors {", ".join(vectors)}; textures {", ".join(textures)}'


def build_master(maps):
    mat = master()
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1900, 0, coordinate_index=0)
    # Its RGBA output: the first (unnamed) one is RGB only, and the opacity reads the decal colour's alpha (Code.a).
    code = g.node(unreal.MaterialExpressionDecalColor, -1900, 200)
    uv = g.custom(CELL_UV, [
        ('UV', coords, ''), ('Code', code, 'RGBA'),
        ('Swap', g.scalar('SwapUV', 0.0, -1900, 400), ''), ('FlipU', g.scalar('FlipU', 0.0, -1900, 500), ''),
        ('FlipV', g.scalar('FlipV', 0.0, -1900, 600), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT2, -1500, 100, 'Stain cell')
    color = g.texture('StainColor', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, maps['BC'], uv, -1100, -200)
    orm = g.texture('StainORM', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, maps['ORM'], uv, -1100, 200)
    world = g.node(unreal.MaterialExpressionWorldPosition, -1500, 700)
    try:
        world.set_editor_property('world_position_shader_offset', unreal.WorldPositionIncludedOffsets.WPT_CAMERA_RELATIVE)
    except Exception as error:  # noqa: BLE001 - the absolute position works too, less precisely far from the origin
        unreal.log_warning(f'CLIFFSTAIN M_CliffStain: camera-relative world position not set ({error}); '
                           'using the absolute one')
    facing = g.custom(W.MARK_FACING, [
        ('P', world, ''), ('V', g.node(unreal.MaterialExpressionCameraVectorWS, -1500, 800), ''),
        ('Axis', g.node(unreal.MaterialExpressionObjectOrientation, -1500, 900), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1100, 700, 'Facing')
    opacity = g.custom(OPACITY, [
        ('Alpha', color, 'A'), ('Facing', facing, ''), ('Code', code, 'RGBA'),
        ('Strength', g.scalar('Opacity', 1.0, -1100, 950), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -700, 600, 'Opacity')
    tinted = g.mul(color, 'RGB', g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1100, -400), '', -700, -300)
    g.out(tinted, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    g.out(orm, 'G', unreal.MaterialProperty.MP_ROUGHNESS)
    W.finish(mat, [])
    log(f'{mat.get_path_name()} built (deferred decal, translucent; colour, opacity, roughness): {parameter_names(mat)}')
    return mat


# --- MI_CliffStain ---

def instance(master_material, maps):
    """MI_CliffStain of the master with the atlas's maps; an asset there that isn't an instance of it is left alone."""
    name = 'MI_CliffStain'
    path = f'{INSTANCE_FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        made = unreal.load_asset(path)
        parent = made.get_editor_property('parent') if isinstance(made, unreal.MaterialInstanceConstant) else None
        if parent is None or parent.get_path_name() != master_material.get_path_name():
            log(f'{path} left alone: it is not an instance of {master_material.get_name()}')
            return None
        verb = 'updated'
        # Keep the orientation fixes someone set on it (SwapUV, FlipU, FlipV), as they were.
        kept = {key: MEL.get_material_instance_scalar_parameter_value(made, key) for key in ('SwapUV', 'FlipU', 'FlipV')}
    else:
        made = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, INSTANCE_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        if made is None:
            raise RuntimeError(f'{path} could not be created')
        MEL.set_material_instance_parent(made, master_material)
        verb = 'made'
        kept = {}
    # From scratch each time: the master's defaults, the maps, and any orientation fix it had.
    MEL.clear_all_material_instance_parameters(made)
    for kind, parameter in MAPS.items():
        MEL.set_material_instance_texture_parameter_value(made, parameter, unreal.load_asset(maps[kind]))
    for key, value in kept.items():
        if value > 0.5:
            MEL.set_material_instance_scalar_parameter_value(made, key, 1.0)
    MEL.update_material_instance(made)
    unreal.EditorAssetLibrary.save_loaded_asset(made, only_if_is_dirty=False)
    fixes = ', '.join(key for key, value in kept.items() if value > 0.5)
    log(f'{path} {verb}: {master_material.get_name()} with the {SET} maps' + (f' ({fixes} kept on)' if fixes else ''))
    return made


def run():
    maps = import_maps()
    if maps is None:
        return
    instance(build_master(maps), maps)
    log('done')


run()
