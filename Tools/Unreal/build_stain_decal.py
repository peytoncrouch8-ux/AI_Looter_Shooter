"""Builds the old stain on the lookout's deck boards (Main 2's optional, the LookoutStains readable: "the boards where it
happened") beside the world's masters:

  T_LookoutStain_BC,  /Game/Art/Textures/LookoutStain: the stain's maps, script-made by
  _N, _ORM            Art/Textures/LookoutStain/make_lookout_stain.py (colour with its opacity in alpha; DirectX normal map;
                      R occlusion, G roughness, B metallic). Imported again whenever a PNG changes (build_world_materials.py's
                      import_texture: its MD5 is kept on the asset), clamped at the edges.
  M_StainDecal        /Game/Art/Materials/Masters: a deferred decal, translucent (DBuffer colour, normal, roughness and
                      metallic: drawn before the lighting, so it's lit as its boards are, on Medium's Nanite fallback without
                      Lumen as on High), the stain soaked into what it's laid on. StainColor's colour times Tint, its alpha
                      times Opacity the opacity, only on surfaces facing back up the decal's axis (the boards' tops, never
                      the sides of their gaps or a post it would smear down), StainNormal at NormalStrength, StainORM's
                      roughness and metallic. Any script-made stain set can be an instance of it.
  MI_LookoutStain     /Game/Art/Materials: the instance with the lookout stain's maps.

The decal itself is placed by Tools/Unreal/build_area_missionprops_valley.py (place_lookout_stains): an ADecalActor on
the deck boards, pitched straight down, 1.5 m square as the maps are painted (make_lookout_stain.py's SPAN_M; the decal's
size is half extents, 75 cm), its U (local Y) along the boards.

It never touches another asset: the master's graph is rebuilt in place (the instance stays linked), and an asset at one of
these paths that isn't what this makes is left alone. It reuses build_world_materials.py's graph helpers (without running
that module's build), as build_decal_materials.py does. Run it in the open editor (it needs no C++), after the maps are
made:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_stain_decal.py"
It prints STAINDECAL lines and "STAINDECAL done".
"""
import ast
import os
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
INSTANCE_FOLDER = '/Game/Art/Materials'
SET = 'LookoutStain'
MAPS = {'BC': 'StainColor', 'N': 'StainNormal', 'ORM': 'StainORM'}


def log(message):
    unreal.log(f'STAINDECAL {message}')


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


# --- The maps ---

def import_maps():
    """The stain's three maps at /Game/Art/Textures/LookoutStain, clamped; their paths by kind, or None while a PNG is
    missing (make_lookout_stain.py hasn't been run): no material then, since a stand-in would lay a white square on the
    deck."""
    folder = os.path.join(PROJECT, 'Art', 'Textures', SET)
    files = {kind: os.path.join(folder, f'T_{SET}_{kind}.png') for kind in MAPS}
    missing = [kind for kind, file in files.items() if not os.path.isfile(file)]
    if missing:
        unreal.log_error(f'STAINDECAL not built: {", ".join(missing)} missing from {folder}; run '
                         f'Tools\\artrun.ps1 -Script Art\\Textures\\{SET}\\make_lookout_stain.py first')
        return None
    paths = {}
    for kind, file in files.items():
        path = f'/Game/Art/Textures/{SET}/T_{SET}_{kind}'
        if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Texture2D):
            raise RuntimeError(f'{path} exists and is not a texture: nothing was changed')
        texture = W.import_texture(file.replace('\\', '/'), path, {'BC': 'BC', 'N': 'N'}.get(kind, 'M'))
        if kind == 'ORM':
            texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
        # Clamped: the stain fades out well inside its square, and its edge never picks up the opposite one.
        texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
        texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
        unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
        log(f'{path}: {texture.blueprint_get_size_x()} x {texture.blueprint_get_size_y()}, clamped')
        paths[kind] = path
    return paths


# --- M_StainDecal ---

# Only on surfaces facing back up the decal's way (Axis, its X, points into the surface): never the side of a board's gap
# or a post inside its box, which the stain would only smear down. The surface's facing comes from the depth buffer's own
# positions (camera-relative, for their precision), turned toward the eye, as M_BulletMark's.
FACING = """float3 N = normalize(cross(ddx(P), ddy(P)));
N *= dot(N, V) < 0.0 ? -1.0 : 1.0;
return smoothstep(0.2, 0.5, dot(N, -normalize(Axis)));"""
OPACITY = 'return saturate(Alpha * Facing * Strength);'
NORMAL = 'return normalize(float3(N.xy * Strength, N.z));'


def master():
    """M_StainDecal, its graph emptied to build again; refuses an asset at its path that isn't a material."""
    path = f'{W.FOLDER}/M_StainDecal'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Material):
        raise RuntimeError(f'{path} exists and is not a material: nothing was changed')
    return W.material('M_StainDecal')


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
    uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1500, 0, coordinate_index=0)
    color = g.texture('StainColor', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, maps['BC'], uv, -1100, -200)
    normal = g.texture('StainNormal', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, maps['N'], uv, -1100, 100)
    orm = g.texture('StainORM', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, maps['ORM'], uv, -1100, 400)
    world = g.node(unreal.MaterialExpressionWorldPosition, -1500, 650)
    try:
        world.set_editor_property('world_position_shader_offset', unreal.WorldPositionIncludedOffsets.WPT_CAMERA_RELATIVE)
    except Exception as error:  # noqa: BLE001 - the absolute position works too, a little less precisely far from the origin
        unreal.log_warning(f'STAINDECAL M_StainDecal: camera-relative world position not set ({error}); using the absolute one')
    facing = g.custom(FACING, [
        ('P', world, ''), ('V', g.node(unreal.MaterialExpressionCameraVectorWS, -1500, 750), ''),
        ('Axis', g.node(unreal.MaterialExpressionObjectOrientation, -1500, 850), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1100, 700, 'Facing')
    opacity = g.custom(OPACITY, [
        ('Alpha', color, 'A'), ('Facing', facing, ''), ('Strength', g.scalar('Opacity', 1.0, -1100, 900), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -700, 600, 'Opacity')
    tinted = g.mul(color, 'RGB', g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1100, -400), '', -700, -300)
    bent = g.custom(NORMAL, [('N', normal, 'RGB'), ('Strength', g.scalar('NormalStrength', 1.0, -1100, 250), '')],
                    unreal.CustomMaterialOutputType.CMOT_FLOAT3, -700, 100, 'Normal strength')
    g.out(tinted, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    g.out(bent, '', unreal.MaterialProperty.MP_NORMAL)
    g.out(orm, 'G', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(orm, 'B', unreal.MaterialProperty.MP_METALLIC)
    W.finish(mat, [])
    log(f'{mat.get_path_name()} built (deferred decal, translucent): {parameter_names(mat)}')
    return mat


# --- MI_LookoutStain ---

def instance(master_material, maps):
    """MI_LookoutStain of the master with the stain's maps; an asset there that isn't an instance of it is left alone."""
    name = f'MI_{SET}'
    path = f'{INSTANCE_FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        made = unreal.load_asset(path)
        parent = made.get_editor_property('parent') if isinstance(made, unreal.MaterialInstanceConstant) else None
        if parent is None or parent.get_path_name() != master_material.get_path_name():
            log(f'{path} left alone: it is not an instance of {master_material.get_name()}')
            return None
        verb = 'updated'
    else:
        made = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, INSTANCE_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        if made is None:
            raise RuntimeError(f'{path} could not be created')
        MEL.set_material_instance_parent(made, master_material)
        verb = 'made'
    # From scratch each time: the master's defaults, and the maps.
    MEL.clear_all_material_instance_parameters(made)
    for kind, parameter in MAPS.items():
        MEL.set_material_instance_texture_parameter_value(made, parameter, unreal.load_asset(maps[kind]))
    MEL.update_material_instance(made)
    unreal.EditorAssetLibrary.save_loaded_asset(made, only_if_is_dirty=False)
    log(f'{path} {verb}: {master_material.get_name()} with the {SET} maps')
    return made


def run():
    maps = import_maps()
    if maps is None:
        return
    instance(build_master(maps), maps)
    log('done')


run()
