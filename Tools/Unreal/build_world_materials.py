"""Builds the master materials of the textured art style (Docs/TutorialIsland.md) in /Game/Art/Materials/Masters:

  M_World         opaque: BaseColorMap, NormalMap, ORMMap (ambient occlusion, roughness, metallic), Tint, UVScale.
                  The vertex color alpha is baked ambient occlusion (SSAO is off on Medium), and DiffuseAO lets some of
                  it darken the base color too, so contact shading shows in direct light. MossAmount (0 = off) grows
                  moss on upward faces, in MossColor, above the MossThreshold slope.
  M_WorldFoliage  masked, two-sided (back faces keep the front's normal), the same maps (opacity from the base
                  color's alpha) plus wind: vertex color R is how far a vertex sways, G offsets its phase;
                  WindStrength (cm), WindSpeed, WindDirection.
  M_Terrain       MacroMap on UV 0 covers the whole island (its alpha picks the detail: 0 grass/soil, 1 rock); the
                  Grass* and Rock* maps tile on UV 1 (meters) and only modulate the macro color's brightness.
  M_Water         opaque, cheap: a color, glossy, procedural ripples.

The model importer (FModelImporter) makes MI_<material> instances of these from the Blender materials. Re-running this
keeps each material asset (so instances stay linked) and rebuilds its graph. Run in the open editor:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_world_materials.py"
"""
import unreal

FOLDER = '/Game/Art/Materials/Masters'
MEL = unreal.MaterialEditingLibrary
WHITE = '/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture'
FLAT_NORMAL = '/Engine/EngineMaterials/DefaultNormal.DefaultNormal'
DEFAULT_ORM_FILE = 'C:/Dev/AI_Looter_Shooter/Art/Textures/Default/T_DefaultORM.png'
DEFAULT_ORM = '/Game/Art/Textures/Default/T_DefaultORM'


def default_orm():
    """A 4x4 neutral ORM (occlusion 1, roughness 0.8, metallic 0) for masters whose ORM map isn't set."""
    if not unreal.EditorAssetLibrary.does_asset_exist(DEFAULT_ORM):
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', DEFAULT_ORM_FILE)
        task.set_editor_property('destination_path', '/Game/Art/Textures/Default')
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(DEFAULT_ORM)
    texture.set_editor_property('srgb', False)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def material(name):
    path = f'{FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mat = unreal.load_asset(path)
        MEL.delete_all_material_expressions(mat)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    return mat


class Graph:
    def __init__(self, mat):
        self.mat = mat

    def node(self, cls, x, y, **props):
        n = MEL.create_material_expression(self.mat, cls, x, y)
        for key, value in props.items():
            n.set_editor_property(key, value)
        return n

    def link(self, a, a_out, b, b_in):
        if not MEL.connect_material_expressions(a, a_out, b, b_in):
            raise RuntimeError(f'could not connect {a.get_name()}.{a_out} -> {b.get_name()}.{b_in}')

    def out(self, a, a_out, prop):
        if not MEL.connect_material_property(a, a_out, prop):
            raise RuntimeError(f'could not connect {a.get_name()}.{a_out} -> {prop}')

    def scalar(self, name, value, x, y):
        return self.node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)

    def vector(self, name, color, x, y):
        return self.node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name, default_value=unreal.LinearColor(*color))

    def texture(self, name, sampler, default, uv, x, y):
        t = self.node(unreal.MaterialExpressionTextureSampleParameter2D, x, y, parameter_name=name, sampler_type=sampler,
                      texture=unreal.load_asset(default))
        if uv is not None:
            self.link(uv, '', t, 'UVs')
        return t

    def mul(self, a, a_out, b, b_out, x, y):
        m = self.node(unreal.MaterialExpressionMultiply, x, y)
        self.link(a, a_out, m, 'A')
        self.link(b, b_out, m, 'B')
        return m

    def custom(self, code, inputs, out_type, x, y, desc):
        pins = []
        for name, _, _ in inputs:
            pin = unreal.CustomInput()
            pin.set_editor_property('input_name', name)
            pins.append(pin)
        c = self.node(unreal.MaterialExpressionCustom, x, y, code=code, output_type=out_type, description=desc, inputs=pins)
        for name, source, source_out in inputs:
            self.link(source, source_out, c, name)
        return c


def finish(mat, usages):
    for usage in usages:
        MEL.set_base_material_usage(mat, usage)
    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)


def textured_inputs(g, orm_default):
    """UV (UV 0 x UVScale), the three maps, vertex color, and the combined occlusion (ORM.R x vertex alpha)."""
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1600, 0, coordinate_index=0)
    uv = g.mul(coords, '', g.scalar('UVScale', 1.0, -1600, 120), '', -1400, 40)
    bc = g.texture('BaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, uv, -1100, -300)
    nrm = g.texture('NormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, FLAT_NORMAL, uv, -1100, 0)
    orm = g.texture('ORMMap', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, orm_default, uv, -1100, 300)
    vc = g.node(unreal.MaterialExpressionVertexColor, -1100, 600)
    ao = g.mul(orm, 'R', vc, 'A', -800, 400)
    return bc, nrm, orm, vc, ao


def shaded_color(g, bc, ao):
    """Base color x Tint, darkened by DiffuseAO of the occlusion."""
    tint = g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1100, -500)
    tinted = g.mul(bc, 'RGB', tint, '', -800, -400)
    lerp = g.node(unreal.MaterialExpressionLinearInterpolate, -800, -150, const_a=1.0)
    g.link(ao, '', lerp, 'B')
    g.link(g.scalar('DiffuseAO', 0.4, -1100, -150), '', lerp, 'Alpha')
    return g.mul(tinted, '', lerp, '', -500, -300)


MOSS_MASK = """float Luma = dot(Color, float3(0.299, 0.587, 0.114));
float Up = saturate((Normal.z - Threshold + (Luma - 0.25) * 0.8) / 0.2);
return Up * Amount;"""


def moss(g, bc, color, roughness):
    """Moss and grass settling on upward faces (rock ledges, boulder tops, old roofs): MossAmount 0 (off) to 1, above
    the MossThreshold slope (the vertex normal's up component), broken up by the texture's light and dark."""
    mask = g.custom(MOSS_MASK, [
        ('Color', bc, 'RGB'),
        ('Normal', g.node(unreal.MaterialExpressionVertexNormalWS, -800, -700), ''),
        ('Threshold', g.scalar('MossThreshold', 0.55, -800, -800), ''),
        ('Amount', g.scalar('MossAmount', 0.0, -800, -900), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -500, -700, 'Moss mask')
    moss_color = g.mul(g.vector('MossColor', (0.11, 0.19, 0.035, 1.0), -500, -900), '', bc, 'RGB', -300, -850)
    # The moss color carries the texture's light and dark (x2: the maps average about half brightness).
    moss_color = g.mul(moss_color, '', g.node(unreal.MaterialExpressionConstant, -300, -950, r=2.0), '', -150, -850)
    blended = g.node(unreal.MaterialExpressionLinearInterpolate, 0, -600)
    g.link(color, '', blended, 'A')
    g.link(moss_color, '', blended, 'B')
    g.link(mask, '', blended, 'Alpha')
    rough = g.node(unreal.MaterialExpressionLinearInterpolate, 0, -400, const_b=0.9)
    g.link(roughness, 'G', rough, 'A')
    g.link(mask, '', rough, 'Alpha')
    return blended, rough


def build_world(orm_default):
    mat = material('M_World')
    g = Graph(mat)
    bc, nrm, orm, vc, ao = textured_inputs(g, orm_default)
    color, rough = moss(g, bc, shaded_color(g, bc, ao), orm)
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(nrm, 'RGB', unreal.MaterialProperty.MP_NORMAL)
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(orm, 'B', unreal.MaterialProperty.MP_METALLIC)
    g.out(ao, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    finish(mat, [unreal.MaterialUsage.MATUSAGE_NANITE, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,
                 unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH])
    return mat


WIND_CODE = """float Phase = VertexColor.g * 6.2831 + InstanceRandom * 6.2831;
float Sway = sin(Time * Speed + Phase) * 0.7 + sin(Time * Speed * 2.37 + Phase * 1.7) * 0.3;
float2 Dir = normalize(Direction.xy + float2(0.0001, 0.0));
return float3(Dir * Sway * Strength * VertexColor.r, 0.0);"""


def build_foliage(orm_default):
    mat = material('M_WorldFoliage')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided', True)
    g = Graph(mat)
    bc, nrm, orm, vc, ao = textured_inputs(g, orm_default)
    g.out(shaded_color(g, bc, ao), '', unreal.MaterialProperty.MP_BASE_COLOR)
    # A two-sided material turns the normal around on back faces. Leaf cards and blades carry normals that point out of
    # the crown or up from the ground, which both sides should keep, or half the cards shade dark: undo the turn.
    # The engine multiplies the whole world normal by TwoSidedSign, so the tangent normal is multiplied by it first.
    facing = g.custom('return Normal * Sign;', [
        ('Normal', nrm, 'RGB'),
        ('Sign', g.node(unreal.MaterialExpressionTwoSidedSign, -800, 100), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 50, 'Keep back faces on the authored normal')
    g.out(facing, '', unreal.MaterialProperty.MP_NORMAL)
    g.out(orm, 'G', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(ao, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    g.out(bc, 'A', unreal.MaterialProperty.MP_OPACITY_MASK)
    wind = g.custom(WIND_CODE, [
        ('Time', g.node(unreal.MaterialExpressionTime, -800, 800), ''),
        ('VertexColor', vc, ''),
        ('InstanceRandom', g.node(unreal.MaterialExpressionPerInstanceRandom, -800, 900), ''),
        ('Direction', g.vector('WindDirection', (1.0, 0.35, 0.0, 0.0), -800, 1000), ''),
        ('Strength', g.scalar('WindStrength', 6.0, -800, 1100), ''),
        ('Speed', g.scalar('WindSpeed', 1.3, -800, 1200), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, 900, 'Wind')
    g.out(wind, '', unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    finish(mat, [unreal.MaterialUsage.MATUSAGE_NANITE, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES])
    return mat


TERRAIN_CODE = """float3 Detail = lerp(Grass, Rock, Select);
float Luma = dot(Detail, float3(0.299, 0.587, 0.114));
float Mean = lerp(GrassMean, RockMean, Select);
float3 Color = Macro * lerp(1.0, Luma / max(Mean, 0.05), Strength);
return Color * lerp(1.0, Occlusion, DiffuseAO);"""


def build_terrain():
    mat = material('M_Terrain')
    g = Graph(mat)
    macro_uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1800, -300, coordinate_index=0)
    macro = g.texture('MacroMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, macro_uv, -1400, -300)
    detail_coords = g.node(unreal.MaterialExpressionTextureCoordinate, -2000, 200, coordinate_index=1)
    grass_uv = g.mul(detail_coords, '', g.scalar('GrassScale', 0.5, -2000, 320), '', -1800, 200)
    rock_uv = g.mul(detail_coords, '', g.scalar('RockScale', 0.25, -2000, 520), '', -1800, 450)
    grass = g.texture('GrassBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, grass_uv, -1400, 0)
    grass_n = g.texture('GrassNormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, FLAT_NORMAL, grass_uv, -1400, 250)
    rock = g.texture('RockBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, rock_uv, -1400, 500)
    rock_n = g.texture('RockNormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, FLAT_NORMAL, rock_uv, -1400, 750)
    vc = g.node(unreal.MaterialExpressionVertexColor, -1400, 1000)
    color = g.custom(TERRAIN_CODE, [
        ('Macro', macro, 'RGB'), ('Grass', grass, 'RGB'), ('Rock', rock, 'RGB'), ('Select', macro, 'A'),
        ('GrassMean', g.scalar('GrassMeanLuma', 0.3, -1000, 900), ''),
        ('RockMean', g.scalar('RockMeanLuma', 0.35, -1000, 1000), ''),
        ('Strength', g.scalar('DetailStrength', 0.6, -1000, 1100), ''),
        ('Occlusion', vc, 'A'),
        ('DiffuseAO', g.scalar('DiffuseAO', 0.45, -1000, 1200), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -600, 0, 'TerrainColor')
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    normal = g.node(unreal.MaterialExpressionLinearInterpolate, -900, 400)
    g.link(grass_n, 'RGB', normal, 'A')
    g.link(rock_n, 'RGB', normal, 'B')
    g.link(macro, 'A', normal, 'Alpha')
    flatten = g.node(unreal.MaterialExpressionLinearInterpolate, -600, 400)
    g.link(g.node(unreal.MaterialExpressionConstant3Vector, -900, 600, constant=unreal.LinearColor(0.0, 0.0, 1.0, 1.0)), '', flatten, 'A')
    g.link(normal, '', flatten, 'B')
    g.link(g.scalar('NormalStrength', 0.8, -900, 700), '', flatten, 'Alpha')
    g.out(flatten, '', unreal.MaterialProperty.MP_NORMAL)
    rough = g.node(unreal.MaterialExpressionLinearInterpolate, -600, 700, const_a=0.92, const_b=0.82)
    g.link(macro, 'A', rough, 'Alpha')
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(vc, 'A', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    finish(mat, [unreal.MaterialUsage.MATUSAGE_NANITE])
    return mat


WATER_CODE = """float2 P = UV * 6.2831;
float2 G = 0.10 * float2(cos(P.x * 1.7 + Time * 1.1), cos(P.y * 1.3 - Time * 0.9))
         + 0.06 * float2(cos((P.x + P.y) * 3.1 + Time * 1.7), cos((P.x - P.y) * 2.7 - Time * 1.4));
return normalize(float3(-G.x, -G.y, 1.0));"""


def build_water():
    mat = material('M_Water')
    g = Graph(mat)
    g.out(g.vector('WaterColor', (0.05, 0.12, 0.11, 1.0), -600, -200), '', unreal.MaterialProperty.MP_BASE_COLOR)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1000, 200, coordinate_index=0)
    uv = g.mul(coords, '', g.scalar('RippleScale', 1.0, -1000, 320), '', -800, 200)
    ripples = g.custom(WATER_CODE, [('UV', uv, ''), ('Time', g.node(unreal.MaterialExpressionTime, -800, 400), '')],
                       unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 200, 'Ripples')
    g.out(ripples, '', unreal.MaterialProperty.MP_NORMAL)
    g.out(g.scalar('Roughness', 0.06, -600, 500), '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(g.scalar('Specular', 0.6, -600, 600), '', unreal.MaterialProperty.MP_SPECULAR)
    finish(mat, [unreal.MaterialUsage.MATUSAGE_NANITE])
    return mat


orm = default_orm()
built = [build_world(DEFAULT_ORM), build_foliage(DEFAULT_ORM), build_terrain(), build_water()]
unreal.log('LOOTER world materials: ' + ', '.join(m.get_path_name() for m in built))
