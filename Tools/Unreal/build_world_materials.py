"""Builds the master materials of the textured art style (Docs/TutorialIsland.md) in /Game/Art/Materials/Masters:

  M_World         opaque: BaseColorMap, NormalMap, ORMMap (ambient occlusion, roughness, metallic), Tint, UVScale.
                  The vertex color alpha is baked ambient occlusion (SSAO is off on Medium), and DiffuseAO lets some of
                  it darken the base color too, so contact shading shows in direct light. MossAmount (0 = off) grows
                  moss on upward faces, in MossColor, above the MossThreshold slope.
  M_Gun           M_World's maps for gun parts, plus per-gun wear: Wear (0 fresh to 1 battered) comes from each part's
                  custom primitive data 0 (set by UWeaponModelComponent, no material copies per gun): scuffs where the
                  finish is rubbed through to a paler layer, grime in the creases, a duller finish. No moss.
  M_WorldFoliage  masked, two-sided (back faces keep the front's normal), the same maps (opacity from the base
                  color's alpha) plus wind: vertex color R is how far a vertex sways, G offsets its phase;
                  WindStrength (cm), WindSpeed, WindDirection.
  M_Terrain       MacroMap on UV 0 covers the whole island (its alpha picks the detail: 0 grass/soil, 1 rock); the
                  Grass* and Rock* maps tile on UV 1 (meters) and only modulate the macro color's brightness.
  M_SkyClouds     unlit, translucent: painted clouds on a sky dome (Coverage, Softness, Scale, wind, colors).
  M_Waterfall,    unlit, translucent effects scrolling the macro noise: a falling water sheet's foam streaks, and
  M_Smoke         chimney smoke. Vertex color A is opacity (R foam on the waterfall).
  M_Water         opaque, cheap: a dark color (the sky's reflection does the rest), glossy, procedural ripples on
                  UV 0 in meters.
  M_Gel           lit translucent gel (the slime): Tint, mostly clear facing the eye (Opacity) and denser at grazing
                  edges (EdgeOpacity), a faint inner glow, and a sun highlight worked out in the material (the sky
                  atmosphere's sun direction), so the cheapest translucent lighting does. Casts a solid shadow.
  M_Glass         unlit, translucent: lenses and sight windows. Mostly clear (Opacity) facing the eye, so a sight can
                  be aimed through, tinted (Tint) and brighter and denser toward grazing edges (RimBrightness,
                  EdgeOpacity), which reads as glass without reflections.
  M_Backdrop      unlit, opaque, one-sided: the far silhouettes past a grounded area (Art/Levels/area_beyond.py), a flat
                  Tint times Brightness (about what sunlit ground of that color shows) times the lighting state's
                  BackdropTint from MPC_Lighting (white by day; lighting_collection.py makes the collection first); the
                  height fog hazes them.

The model importer (FModelImporter) makes MI_<material> instances of these from the Blender materials. Re-running this
keeps each material asset (so instances stay linked) and rebuilds its graph. Run in the open editor, optionally with the
names of the masters to build (the rest are left alone):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_world_materials.py [M_Glass ...]"
"""
import sys

import unreal

FOLDER = '/Game/Art/Materials/Masters'
MEL = unreal.MaterialEditingLibrary
WHITE = '/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture'
FLAT_NORMAL = '/Engine/EngineMaterials/DefaultNormal.DefaultNormal'
DEFAULT_ORM_FILE = 'C:/Dev/AI_Looter_Shooter/Art/Textures/Default/T_DefaultORM.png'
DEFAULT_ORM = '/Game/Art/Textures/Default/T_DefaultORM'
MACRO_NOISE_FILE = 'C:/Dev/AI_Looter_Shooter/Art/Textures/MacroNoise/T_MacroNoise_M.png'
MACRO_NOISE = '/Game/Art/Textures/MacroNoise/T_MacroNoise_M'


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


def lighting_tint(g, name, x, y):
    """A tint the lighting states set (MPC_Lighting; white by day), as its RGB. Unlit materials never see the sun go
    down, so dusk reaches them this way. lighting_collection.py makes the collection first (a material can't read a
    parameter that isn't there yet); it's reloaded, as the editor keeps modules between runs."""
    import importlib
    import os
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    import lighting_collection
    collection = importlib.reload(lighting_collection).collection()
    # The collection is set before the name: naming the parameter looks up its id in the collection.
    state = g.node(unreal.MaterialExpressionCollectionParameter, x, y, collection=collection, parameter_name=name)
    rgb = g.node(unreal.MaterialExpressionComponentMask, x + 300, y, r=True, g=True, b=True, a=False)
    g.link(state, '', rgb, '')
    return rgb


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
    # Moss keeps its own brightness and takes only a little of the texture's light and dark (0.7 to 1.3 of MossColor):
    # multiplying by the rock's color as well made moss on dark stone nearly black.
    moss_color = g.custom('return Moss * (0.7 + 0.6 * saturate(dot(Color, float3(0.299, 0.587, 0.114)) * 2.0));', [
        ('Moss', g.vector('MossColor', (0.13, 0.21, 0.05, 1.0), -500, -900), ''),
        ('Color', bc, 'RGB'),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -250, -850, 'Moss color')
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


WEAR_CODE = """// Scuffs: small specks from two fine scales of the macro noise, where the finish is rubbed through to bare grey metal
// (darker on light paint, lighter on dark); the more worn the gun, the more of them.
float N = Texture2DSample(Noise, NoiseSampler, UV * 23.0).r * 0.6 + Texture2DSample(Noise, NoiseSampler, UV * 61.0 + 0.37).r * 0.4;
float Scuff = saturate((N - (1.0 - Wear * 0.26)) * 12.0);
float3 C = lerp(Color, float3(0.42, 0.42, 0.44), Scuff * 0.45);
// Grime settles in the creases, and the whole finish dulls a little.
C *= lerp(1.0, 0.7, Wear * saturate(1.0 - Occlusion));
C *= 1.0 - Wear * 0.08;
return float4(C, Scuff);"""


def build_gun(orm_default):
    mat = material('M_Gun')
    g = Graph(mat)
    bc, nrm, orm, vc, ao = textured_inputs(g, orm_default)
    color = shaded_color(g, bc, ao)
    wear = g.node(unreal.MaterialExpressionScalarParameter, -800, 600, parameter_name='Wear', default_value=0.0,
                  use_custom_primitive_data=True, primitive_data_index=0)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -800, 800, coordinate_index=0)
    worn = g.custom(WEAR_CODE, [
        ('Color', color, ''),
        ('Occlusion', ao, ''),
        ('Wear', wear, ''),
        ('UV', coords, ''),
        ('Noise', g.node(unreal.MaterialExpressionTextureObjectParameter, -800, 900, parameter_name='WearNoise',
                         texture=import_mask(MACRO_NOISE_FILE, MACRO_NOISE)), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT4, -300, -200, 'Wear')
    rgb = g.node(unreal.MaterialExpressionComponentMask, 0, -300, r=True, g=True, b=True, a=False)
    g.link(worn, '', rgb, '')
    scuff = g.node(unreal.MaterialExpressionComponentMask, 0, 0, r=False, g=False, b=False, a=True)
    g.link(worn, '', scuff, '')
    rough = g.custom('return saturate(Roughness + Wear * 0.12 + Scuff * 0.15);', [
        ('Roughness', orm, 'G'), ('Wear', wear, ''), ('Scuff', scuff, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, 200, 100, 'Worn roughness')
    g.out(rgb, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(nrm, 'RGB', unreal.MaterialProperty.MP_NORMAL)
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(orm, 'B', unreal.MaterialProperty.MP_METALLIC)
    g.out(ao, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    finish(mat, [])
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
    # Foliage is never Nanite (it has LODs instead), and the Nanite permutation of this material doesn't compile on SM6.
    # Skeletal too: creatures (the slime's core and pebbles) take their colors from the foliage palette.
    finish(mat, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH])
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


WATER_CODE = """// Four waves at odd angles and lengths (meters), over a slowly warped surface, so no grid lines up.
float2 Q = UV + 0.35 * float2(sin(UV.y * 0.9 + Time * 0.30), sin(UV.x * 0.7 - Time * 0.25));
float2 D1 = float2(0.80, 0.60), D2 = float2(-0.45, 0.89), D3 = float2(0.95, -0.31), D4 = float2(-0.71, -0.71);
float2 G = D1 * cos(dot(Q, D1) * 2.73 + Time * 1.3) * 0.050
         + D2 * cos(dot(Q, D2) * 4.49 + Time * 1.7) * 0.035
         + D3 * cos(dot(Q, D3) * 6.98 + Time * 2.2) * 0.025
         + D4 * cos(dot(Q, D4) * 11.4 + Time * 2.9) * 0.015;
return normalize(float3(-G.x, -G.y, 1.0));"""


def build_water():
    mat = material('M_Water')
    g = Graph(mat)
    g.out(g.vector('WaterColor', (0.012, 0.032, 0.03, 1.0), -600, -200), '', unreal.MaterialProperty.MP_BASE_COLOR)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1000, 200, coordinate_index=0)
    uv = g.mul(coords, '', g.scalar('RippleScale', 1.0, -1000, 320), '', -800, 200)
    ripples = g.custom(WATER_CODE, [('UV', uv, ''), ('Time', g.node(unreal.MaterialExpressionTime, -800, 400), '')],
                       unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 200, 'Ripples')
    g.out(ripples, '', unreal.MaterialProperty.MP_NORMAL)
    g.out(g.scalar('Roughness', 0.1, -600, 500), '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(g.scalar('Specular', 0.5, -600, 600), '', unreal.MaterialProperty.MP_SPECULAR)
    # Water meshes aren't Nanite, and the Nanite permutation of this material doesn't compile on SM6.
    finish(mat, [])
    return mat


# The cloud layer, shared by the color and the opacity: an overhead plane seen through the dome, so far clouds crowd
# toward the horizon as a real layer does; three octaves of the macro noise drifting with the wind.
CLOUD_DENSITY = """float3 D = -CameraVector;
float Up = D.z;
float2 P = D.xy / (max(Up, 0.0) + 0.3) * Scale;
float2 Drift = WindDirection.xy * Time * WindSpeed;
float A = Texture2DSample(Noise, NoiseSampler, P + Drift).r;
float B = Texture2DSample(Noise, NoiseSampler, P * 2.31 + float2(0.37, 0.71) + Drift * 1.6).r;
float C = Texture2DSample(Noise, NoiseSampler, P * 5.13 + float2(0.11, 0.53) + Drift * 2.4).r;
float Density = A * 0.6 + B * 0.3 + C * 0.1 - Coverage;
"""
CLOUD_COLOR = CLOUD_DENSITY + """// Lit tops, greyer bellies where the cloud is thick.
return lerp(LitColor, ShadeColor, saturate(Density / (Softness * 3.0)) * 0.7);"""
CLOUD_OPACITY = CLOUD_DENSITY + """// Soft edges, and the layer fades into the haze toward the horizon.
return saturate(Density / Softness) * saturate((Up - 0.04) * 3.0) * Opacity;"""


def import_mask(file, path):
    """A linear single-channel texture from the library (here the macro noise for the clouds)."""
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', file)
        task.set_editor_property('destination_path', path.rsplit('/', 1)[0])
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(path)
    texture.set_editor_property('srgb', False)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def build_sky_clouds():
    """M_SkyClouds: painted clouds on a sky dome around the level. Unlit and translucent over the sky atmosphere; one
    cheap draw instead of volumetric clouds (which cost Medium 2 ms or more looking up through their layer)."""
    mat = material('M_SkyClouds')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = Graph(mat)
    noise = g.node(unreal.MaterialExpressionTextureObjectParameter, -900, 0, parameter_name='Noise',
                   texture=import_mask(MACRO_NOISE_FILE, MACRO_NOISE))
    # One set of inputs feeds both nodes.
    inputs = [
        ('CameraVector', g.node(unreal.MaterialExpressionCameraVectorWS, -900, -200), ''),
        ('Noise', noise, ''),
        ('Time', g.node(unreal.MaterialExpressionTime, -900, 150), ''),
        ('WindDirection', g.vector('WindDirection', (1.0, 0.4, 0.0, 0.0), -900, 250), ''),
        ('WindSpeed', g.scalar('WindSpeed', 0.004, -900, 350), ''),
        ('Scale', g.scalar('Scale', 0.45, -900, 450), ''),
        ('Coverage', g.scalar('Coverage', 0.58, -900, 550), ''),
        ('Softness', g.scalar('Softness', 0.22, -900, 650), ''),
    ]
    color = g.custom(CLOUD_COLOR, inputs + [
        ('LitColor', g.vector('LitColor', (3.0, 2.95, 2.85, 1.0), -900, 750), ''),
        ('ShadeColor', g.vector('ShadeColor', (1.35, 1.45, 1.65, 1.0), -900, 850), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, -100, 'Cloud color')
    opacity = g.custom(CLOUD_OPACITY, inputs + [
        ('Opacity', g.scalar('Opacity', 0.9, -900, 950), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -400, 200, 'Cloud opacity')
    # The lighting state's cloud tint (white by day) dims and warms them at dusk.
    g.out(g.mul(color, '', lighting_tint(g, 'CloudTint', -900, 1050), '', -100, -100), '',
          unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    finish(mat, [])
    return mat


# Effects: unlit and translucent, scrolling the macro noise. Their models carry the shape in vertex colors (A opacity,
# R foam) and UV 0 (V along the flow, in meters for the waterfall, 0..1 up the cards for smoke).
FALL_STREAKS = """float2 P = float2(UV.x * 4.0, UV.y * 0.06 - Time * Speed);
float Streak = Texture2DSample(Noise, NoiseSampler, P).r * 0.65
             + Texture2DSample(Noise, NoiseSampler, P * float2(2.7, 1.9) + 0.31).r * 0.35;
"""
FALL_COLOR = FALL_STREAKS + """float Foam = saturate(VertexColor.r + (Streak - 0.45) * 1.4);
return lerp(WaterColor, FoamColor, Foam);"""
FALL_OPACITY = FALL_STREAKS + """return saturate(Alpha * (0.35 + Streak * 1.1)) * Opacity;"""

SMOKE_NOISE = """float2 P = float2(UV.x * 0.8, UV.y * 0.5 - Time * Speed);
float Puff = Texture2DSample(Noise, NoiseSampler, P).r * 0.7
           + Texture2DSample(Noise, NoiseSampler, P * 2.3 + 0.57).r * 0.3;
"""
SMOKE_OPACITY = SMOKE_NOISE + """// Soft card edges, thinning as it rises.
float Edge = sin(saturate(UV.x) * 3.14159);
return saturate((Puff - 0.42) * 2.6) * Edge * Edge * Alpha * (1.0 - UV.y * 0.6) * Opacity;"""


SUN_HIGHLIGHT = """float3 R = reflect(-CameraVector, Normal);
return pow(saturate(dot(R, normalize(LightDirection + 1e-5))), Sharpness) * Strength;"""


def build_gel():
    mat = material('M_Gel')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)
    # A solid shadow under it (its opacity is over the clip value everywhere) keeps it on the ground.
    mat.set_editor_property('cast_dynamic_shadow_as_masked', True)
    g = Graph(mat)
    tint = g.vector('Tint', (0.48, 0.87, 0.25, 1.0), -900, -300)
    g.out(tint, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(g.scalar('Roughness', 0.15, -600, 0), '', unreal.MaterialProperty.MP_ROUGHNESS)
    fresnel = g.node(unreal.MaterialExpressionFresnel, -900, 300, exponent=2.5, base_reflect_fraction=0.0)
    highlight = g.custom(SUN_HIGHLIGHT, [
        ('Normal', g.node(unreal.MaterialExpressionPixelNormalWS, -900, 500), ''),
        ('CameraVector', g.node(unreal.MaterialExpressionCameraVectorWS, -900, 600), ''),
        ('LightDirection', g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection, -900, 700), ''),
        ('Sharpness', g.scalar('HighlightSharpness', 60.0, -900, 800), ''),
        ('Strength', g.scalar('HighlightStrength', 2.0, -900, 900), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -500, 600, 'Sun highlight')
    # Emissive: a faint glow of its own color (light passing through gel) plus the highlight.
    glow = g.mul(tint, '', g.scalar('InnerGlow', 0.12, -900, -150), '', -600, -200)
    emissive = g.node(unreal.MaterialExpressionAdd, -300, 0)
    g.link(glow, '', emissive, 'A')
    g.link(highlight, '', emissive, 'B')
    g.out(emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity = g.node(unreal.MaterialExpressionLinearInterpolate, -600, 300)
    g.link(g.scalar('Opacity', 0.35, -900, 150), '', opacity, 'A')
    g.link(g.scalar('EdgeOpacity', 0.85, -900, 220), '', opacity, 'B')
    g.link(fresnel, '', opacity, 'Alpha')
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    finish(mat, [unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH])
    return mat


def build_glass():
    mat = material('M_Glass')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    g = Graph(mat)
    fresnel = g.node(unreal.MaterialExpressionFresnel, -900, 200, exponent=3.0, base_reflect_fraction=0.0)
    brightness = g.node(unreal.MaterialExpressionLinearInterpolate, -600, -100, const_a=1.0)
    g.link(g.scalar('RimBrightness', 4.0, -900, 0), '', brightness, 'B')
    g.link(fresnel, '', brightness, 'Alpha')
    g.out(g.mul(g.vector('Tint', (0.05, 0.09, 0.12, 1.0), -900, -200), '', brightness, '', -300, -150), '',
          unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity = g.node(unreal.MaterialExpressionLinearInterpolate, -600, 300)
    g.link(g.scalar('Opacity', 0.15, -900, 350), '', opacity, 'A')
    g.link(g.scalar('EdgeOpacity', 0.6, -900, 450), '', opacity, 'B')
    g.link(fresnel, '', opacity, 'Alpha')
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    finish(mat, [])
    return mat


def build_backdrop():
    """M_Backdrop: a few cheap draws for the ranges and plains kilometers out. Unlit, so their color doesn't depend on
    how the low sun happens to strike them; opaque and one-sided, since they're only ever seen from inside. Being unlit
    they never see the sun go down either, so the lighting state's tint (MPC_Lighting's BackdropTint, white by day)
    multiplies every layer's own: dusk darkens and warms the ranges without touching the instances."""
    mat = material('M_Backdrop')
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    g = Graph(mat)
    # A lit surface of albedo A shows about 2.5 A in the island's sun and sky (the unlit clouds' white is about 3).
    color = g.mul(g.vector('Tint', (0.11, 0.15, 0.11, 1.0), -600, -100), '', g.scalar('Brightness', 2.5, -600, 50), '',
                  -300, -50)
    g.out(g.mul(color, '', lighting_tint(g, 'BackdropTint', -600, 200), '', 0, 0), '',
          unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(mat, [])
    return mat


def effect(name, inputs_extra, color_code, color_inputs, opacity_code, opacity_inputs):
    """An unlit, translucent, two-sided master whose color and opacity come from two Custom nodes over shared inputs."""
    mat = material(name)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = Graph(mat)
    # The vertex color's plain output is RGB only: its alpha comes in on a pin of its own.
    vertex = g.node(unreal.MaterialExpressionVertexColor, -900, -100)
    shared = [
        ('UV', g.node(unreal.MaterialExpressionTextureCoordinate, -900, -200, coordinate_index=0), ''),
        ('VertexColor', vertex, ''),
        ('Alpha', vertex, 'A'),
        ('Noise', g.node(unreal.MaterialExpressionTextureObjectParameter, -900, 0, parameter_name='Noise',
                         texture=import_mask(MACRO_NOISE_FILE, MACRO_NOISE)), ''),
        ('Time', g.node(unreal.MaterialExpressionTime, -900, 100), ''),
    ] + [(n, make(g), '') for n, make in inputs_extra]
    color = g.custom(color_code, shared + [(n, make(g), '') for n, make in color_inputs],
                     unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, -100, f'{name} color')
    opacity = g.custom(opacity_code, shared + [(n, make(g), '') for n, make in opacity_inputs],
                       unreal.CustomMaterialOutputType.CMOT_FLOAT1, -400, 200, f'{name} opacity')
    g.out(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    finish(mat, [])
    return mat


def build_waterfall():
    """M_Waterfall: foam streaks falling down a water sheet (Speed in sheet-V units a second)."""
    return effect('M_Waterfall',
                  [('Speed', lambda g: g.scalar('Speed', 0.5, -900, 200))],
                  FALL_COLOR,
                  # Unlit, so as bright as the sunlit scene around it (the clouds are about 3).
                  [('WaterColor', lambda g: g.vector('WaterColor', (1.1, 1.45, 1.6, 1.0), -900, 300)),
                   ('FoamColor', lambda g: g.vector('FoamColor', (3.2, 3.3, 3.4, 1.0), -900, 400))],
                  FALL_OPACITY,
                  [('Opacity', lambda g: g.scalar('Opacity', 0.85, -900, 500))])


def build_smoke():
    """M_Smoke: soft chimney smoke drifting up its cards."""
    return effect('M_Smoke',
                  [('Speed', lambda g: g.scalar('Speed', 0.12, -900, 200))],
                  'return SmokeColor;',
                  [('SmokeColor', lambda g: g.vector('SmokeColor', (1.25, 1.27, 1.33, 1.0), -900, 300))],
                  SMOKE_OPACITY,
                  [('Opacity', lambda g: g.scalar('Opacity', 0.4, -900, 400))])


BUILDERS = {'M_World': lambda: build_world(DEFAULT_ORM), 'M_Gun': lambda: build_gun(DEFAULT_ORM),
            'M_WorldFoliage': lambda: build_foliage(DEFAULT_ORM),
            'M_Terrain': build_terrain, 'M_Water': build_water, 'M_SkyClouds': build_sky_clouds,
            'M_Waterfall': build_waterfall, 'M_Smoke': build_smoke, 'M_Glass': build_glass, 'M_Gel': build_gel,
            'M_Backdrop': build_backdrop}
wanted = [name for name in sys.argv[1:] if name in BUILDERS] or list(BUILDERS)
orm = default_orm()
built = [BUILDERS[name]() for name in wanted]
unreal.log('LOOTER world materials: ' + ', '.join(m.get_path_name() for m in built))
