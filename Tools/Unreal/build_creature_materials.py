"""Builds the creatures' materials (Docs/Areas/RansomsRest.md, step 16) beside the world's masters:

  M_Ghost       /Game/Art/Materials/Masters: the Unpaid (and Hob's ash later), masked and two-sided, its opacity mask
                dithered (DitherTemporalAA), never translucent, so a dozen in a fight stay cheap on Medium. Per vertex:
                R picks one of four tint zones at 0, 1/3, 2/3 and 1 (Zone1Color, skin and shroud, to Zone4Color, the
                accents), G darkens the cavities, B is the ember edge round the coal's hole, A fades the body into the
                shroud (1 solid, 0 gone). The fade is broken up by the macro noise at two scales drifting up the strips
                (UV 0's V runs down them: NoiseScale, NoiseSpeed, FadeSoftness). The Polymer grain (BaseColorMap on UV 0
                times UVScale) runs through the cloth, and Tint multiplies it all. Emissive: a pale rim (RimColor,
                RimStrength, and RimPower: how tightly it hugs the silhouette), a faint glow of its own (GlowStrength) and
                the ember edge in the rank's color (EmberStrength).
                Coal = 1 makes a slot the coal instead: a dark crust with glowing cracks in the rank's color, no grain and
                no fade. Phase dissolves it for the phase-step: the solid body goes first and the shroud's ends last, and
                coming back the ends come first; the coal goes with the body.
                The creature drives its own look through its mesh's custom primitive data, with no material instance per
                creature (AUnpaidCreature, UnpaidLook): RankColor (0-3: the rank's color, alpha its strength), Phase (4)
                and Heat (5: the coal's flare). A mesh no creature drives (the bestiary's stand) shows a Basic coal's dull
                red, solid. Setting Phase on an instance does nothing: it's the creature's.
  MI_Ghost_B,   the second and third clothing tints (Sunday best; harvest clothes). The first, MI_Ghost_A, and the coal's
  MI_Ghost_C    MI_GhostCoal are the model importer's, made from SK_Unpaid's own slots (Ghost_A, GhostCoal): this never
                touches them. B and C take MI_Ghost_A's settings (all but its zone colors) when it exists, so run this
                again after importing SK_Unpaid.

It never touches another asset: M_Ghost's graph is rebuilt in place (its instances stay linked), and an asset at one of
these paths that isn't what this makes is left alone. It reuses build_world_materials.py's graph helpers (without running
that module's build). Run it in the open editor (it needs no C++):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_creature_materials.py"
It prints a CREATUREMATS line per asset and "CREATUREMATS done".
"""
import ast
import os
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
INSTANCE_FOLDER = '/Game/Art/Materials'
POLYMER = '/Game/Art/Textures/Polymer/T_Polymer_BC'
DITHER = '/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA'

# The clothes they died in (Art/Backlog/Creatures/UnpaidConcepts.py's option A, the user's pick, 2026-10-06): skin and
# shroud, then the shirt, the vest and hat, and the accents (bandana, hat band). A's are M_Ghost's defaults; MI_Ghost_A
# itself is the importer's, from the model's script.
# Pale enough to read as a ghost, dark enough that direct sun still shades it (at #D2D8D5 a sunlit limb went flat white).
SKIN = 0xbac4c6
CLOTHES = {
    'A': (SKIN, 0x93a5b2, 0x6c5d50, 0xa65e4c),   # faded chambray shirt, brown wool vest, red bandana
    'B': (SKIN, 0xc6beae, 0x4d4e51, 0x8f8b7e),   # Sunday: cream shirt, charcoal vest, grey
    'C': (SKIN, 0xa38f7e, 0x7b7f6e, 0xcfc095),   # harvest: dusty brown shirt (a rust one read as the legendary orange in sun), drab vest, straw
}
RIM = 0xdcecee            # pale and only barely cool, so no rarity color covers the body
BASIC_COAL = 0xb02a18     # a Basic coal's dull red: what a mesh no creature drives shows
COAL_CRUST = 0x1d110c

# The scalars on M_Ghost and their defaults: an instance that sets none of them looks right.
DEFAULTS = dict(RimStrength=1.5, RimPower=2.8, GlowStrength=0.1, EmberStrength=3.0, NoiseScale=1.5, NoiseSpeed=0.06,
                FadeSoftness=0.07, Coal=0.0, UVScale=1.0)
# Read from the creature's custom primitive data (UnpaidLook in Creatures/UnpaidCreature.h): never an instance's.
PRIMITIVE_DATA = {'RankColor': 0, 'Phase': 4, 'Heat': 5}
ZONES = ('Zone1Color', 'Zone2Color', 'Zone3Color', 'Zone4Color')


def log(message):
    unreal.log(f'CREATUREMATS {message}')


def linear(hex_value, alpha=1.0):
    """'#RRGGBB' as an sRGB color, made linear (what Unreal's colors are)."""
    def channel(byte):
        c = byte / 255.0
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    return (channel((hex_value >> 16) & 0xff), channel((hex_value >> 8) & 0xff), channel(hex_value & 0xff), alpha)


def world_helpers():
    """build_world_materials.py's helpers (Graph, material, finish, import_mask, its paths), without its build: that
    module builds its masters as it's run, every one of them when our arguments name none of its, so only its imports,
    definitions and constants are run here."""
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


# --- M_Ghost ---

GHOST_COLOR = """// Vertex R picks the tint zone: 0, 1/3, 2/3 and 1.
float ZoneIndex = VertexColor.r * 3.0;
float3 Cloth = ZoneIndex < 0.5 ? Zone1 : (ZoneIndex < 1.5 ? Zone2 : (ZoneIndex < 2.5 ? Zone3 : Zone4));
// The Polymer grain runs through it by its brightness (about 0.8 on average, so the zone keeps its value), and the
// cavities darken it (vertex G).
float Grain = clamp(dot(GrainColor, float3(0.299, 0.587, 0.114)) * 1.25, 0.5, 1.5);
float3 Body = Cloth * Grain * Tint * (1.0 - 0.93 * VertexColor.g);
// The coal is a dark crust under its glow.
return lerp(Body, Crust, saturate(CoalAmount));"""

GHOST_GLOW = """// The rank's color, which the creature sets in its custom primitive data (alpha: how strongly it glows). A mesh no
// creature drives (the bestiary's stand) has none: a Basic coal's dull red.
float3 RankGlow = RankAlpha > 0.0 ? RankColor * RankAlpha : BasicColor * 1.8;
float Burn = max(1.0 + Heat, 0.0);
float Facing = abs(dot(normalize(Normal), normalize(CameraVector)));
// RimPower keeps the rim to the silhouette: a low one covers a thin limb's whole face in daylight.
float RimAmount = pow(saturate(1.0 - Facing), RimPower);
float Dark = 1.0 - 0.93 * VertexColor.g;
// The body: a pale rim, a faint glow of its own (a ghost never goes fully dark) and the ember edge round the coal's hole
// (vertex B).
float3 BodyGlow = RimColor * (RimStrength * RimAmount * Dark) + BaseColor * GlowStrength
    + RankGlow * (VertexColor.b * EmberStrength * Burn);
// The coal: glowing cracks in a dark crust, hottest in the middle where it faces the eye.
float Crack = Texture2DSample(Noise, NoiseSampler, UV * 3.0 + float2(0.0, Time * 0.02)).r;
float Cracks = lerp(0.25, 1.0, 1.0 - saturate(abs(Crack - 0.5) * 7.0));
float3 CoalGlow = RankGlow * (EmberStrength * 1.6 * Burn * Cracks * lerp(0.45, 1.0, Facing));
return lerp(BodyGlow, CoalGlow, saturate(CoalAmount));"""

GHOST_OPACITY = """// Two scales of the noise drifting up the strips (UV 0's V runs down them).
float2 Drift = float2(0.0, Time * NoiseSpeed);
float N = Texture2DSample(Noise, NoiseSampler, (UV + Drift) * NoiseScale).r * 0.62
        + Texture2DSample(Noise, NoiseSampler, (UV + Drift * 1.6) * (NoiseScale * 3.07) + float2(0.37, 0.11)).r * 0.38;
float Steep = 1.0 / max(FadeSoftness, 0.005);
// The fade into the shroud (vertex A: 1 solid, 0 gone), broken up by the noise: mostly there or gone, with a thin
// dithered band between. The coal never fades this way.
float Shroud = lerp(saturate((Fade * 1.3 - 0.15 - N) * Steep + 0.5), 1.0, saturate(CoalAmount));
// The phase-step's dissolve, ordered by the same fade: the solid body goes first and the shroud's ends last, and coming
// back the ends come first. The coal goes with the body.
float Order = lerp(Fade * 0.75 + N * 0.25, 0.85, saturate(CoalAmount));
float Phased = saturate((1.05 - 1.1 * saturate(Phase) - Order) * Steep + 0.5);
return Shroud * Phased;"""


def noise_texture():
    """The macro noise, as the world's masters use it; imported only if it isn't yet (never re-saved)."""
    if unreal.EditorAssetLibrary.does_asset_exist(W.MACRO_NOISE):
        return unreal.load_asset(W.MACRO_NOISE)
    return W.import_mask(W.MACRO_NOISE_FILE, W.MACRO_NOISE)


def grain_texture():
    if unreal.EditorAssetLibrary.does_asset_exist(POLYMER):
        return POLYMER
    unreal.log_warning(f'CREATUREMATS {POLYMER} is missing: M_Ghost takes a plain white grain until a model brings the Polymer set')
    return W.WHITE


def dithered(g, alpha, x, y):
    """The opacity through DitherTemporalAA (TAA smooths the dither into a soft fade); the plain opacity if the function
    can't be wired."""
    try:
        call = g.node(unreal.MaterialExpressionMaterialFunctionCall, x, y)
        call.set_editor_property('material_function', unreal.load_asset(DITHER))
        g.link(alpha, '', call, 'Alpha Threshold')
        return call, 'Result'
    except Exception as error:   # noqa: BLE001 - the material still works undithered; say so
        unreal.log_warning(f'CREATUREMATS M_Ghost: DitherTemporalAA could not be wired ({error}); its opacity mask is undithered')
        return alpha, ''


def build_ghost():
    path = f'{W.FOLDER}/M_Ghost'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Material):
        raise RuntimeError(f'{path} exists and is not a material: nothing was changed')
    mat = W.material('M_Ghost')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)

    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1800, 200, coordinate_index=0)
    grain_uv = g.mul(coords, '', g.scalar('UVScale', DEFAULTS['UVScale'], -1800, 320), '', -1600, 260)
    grain = g.texture('BaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, grain_texture(), grain_uv, -1400, 200)
    vertex = g.node(unreal.MaterialExpressionVertexColor, -1400, 500)
    noise = g.node(unreal.MaterialExpressionTextureObjectParameter, -1400, 700, parameter_name='NoiseMap', texture=noise_texture())
    time = g.node(unreal.MaterialExpressionTime, -1400, 850)
    coal = g.scalar('Coal', DEFAULTS['Coal'], -1400, 950)
    zones = [g.vector(name, linear(color), -1400, -600 + 100 * index) for index, (name, color) in enumerate(zip(ZONES, CLOTHES['A']))]

    color = g.custom(GHOST_COLOR, [
        ('VertexColor', vertex, ''),
        ('GrainColor', grain, 'RGB'),
        ('Zone1', zones[0], ''), ('Zone2', zones[1], ''), ('Zone3', zones[2], ''), ('Zone4', zones[3], ''),
        ('Tint', g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1400, -200), ''),
        ('Crust', g.node(unreal.MaterialExpressionConstant3Vector, -1400, -100, constant=unreal.LinearColor(*linear(COAL_CRUST))), ''),
        ('CoalAmount', coal, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -900, -300, 'Ghost color')

    rank = g.node(unreal.MaterialExpressionVectorParameter, -1100, 1100, parameter_name='RankColor',
                  default_value=unreal.LinearColor(*linear(BASIC_COAL, 1.8)), use_custom_primitive_data=True,
                  primitive_data_index=PRIMITIVE_DATA['RankColor'])
    heat = g.node(unreal.MaterialExpressionScalarParameter, -1100, 1250, parameter_name='Heat', default_value=0.0,
                  use_custom_primitive_data=True, primitive_data_index=PRIMITIVE_DATA['Heat'])
    phase = g.node(unreal.MaterialExpressionScalarParameter, -1100, 1350, parameter_name='Phase', default_value=0.0,
                   use_custom_primitive_data=True, primitive_data_index=PRIMITIVE_DATA['Phase'])

    glow = g.custom(GHOST_GLOW, [
        ('VertexColor', vertex, ''),
        ('BaseColor', color, ''),
        ('Normal', g.node(unreal.MaterialExpressionPixelNormalWS, -1100, 400), ''),
        ('CameraVector', g.node(unreal.MaterialExpressionCameraVectorWS, -1100, 500), ''),
        ('RankColor', rank, ''),
        ('RankAlpha', rank, 'A'),
        ('BasicColor', g.node(unreal.MaterialExpressionConstant3Vector, -1100, 1500, constant=unreal.LinearColor(*linear(BASIC_COAL))), ''),
        ('Heat', heat, ''),
        ('RimColor', g.vector('RimColor', linear(RIM), -1100, 600), ''),
        ('RimStrength', g.scalar('RimStrength', DEFAULTS['RimStrength'], -1100, 700), ''),
        ('RimPower', g.scalar('RimPower', DEFAULTS['RimPower'], -1100, 750), ''),
        ('GlowStrength', g.scalar('GlowStrength', DEFAULTS['GlowStrength'], -1100, 800), ''),
        ('EmberStrength', g.scalar('EmberStrength', DEFAULTS['EmberStrength'], -1100, 900), ''),
        ('CoalAmount', coal, ''),
        ('Noise', noise, ''),
        ('UV', coords, ''),
        ('Time', time, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 300, 'Ghost glow')

    opacity = g.custom(GHOST_OPACITY, [
        ('Fade', vertex, 'A'),
        ('Noise', noise, ''),
        ('UV', coords, ''),
        ('Time', time, ''),
        ('NoiseScale', g.scalar('NoiseScale', DEFAULTS['NoiseScale'], -900, 1100), ''),
        ('NoiseSpeed', g.scalar('NoiseSpeed', DEFAULTS['NoiseSpeed'], -900, 1200), ''),
        ('FadeSoftness', g.scalar('FadeSoftness', DEFAULTS['FadeSoftness'], -900, 1300), ''),
        ('Phase', phase, ''),
        ('CoalAmount', coal, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -500, 900, 'Ghost opacity')
    mask, mask_out = dithered(g, opacity, -200, 900)

    rough = g.node(unreal.MaterialExpressionLinearInterpolate, -500, 650, const_a=0.86, const_b=0.6)
    g.link(coal, '', rough, 'Alpha')
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(glow, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(mask, mask_out, unreal.MaterialProperty.MP_OPACITY_MASK)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH])

    scalars = sorted(str(n) for n in MEL.get_scalar_parameter_names(mat))
    vectors = sorted(str(n) for n in MEL.get_vector_parameter_names(mat))
    textures = sorted(str(n) for n in MEL.get_texture_parameter_names(mat))
    log(f'{mat.get_path_name()} built: scalars {", ".join(scalars)}; colors {", ".join(vectors)}; textures {", ".join(textures)}; '
        f'custom primitive data {PRIMITIVE_DATA}')
    return mat


# --- The clothes ---

def parameter_name(value):
    return str(value.get_editor_property('parameter_info').get_editor_property('name'))


def first_tint_settings():
    """MI_Ghost_A's own settings but its zone colors (the importer writes them from the model's script), or None before
    SK_Unpaid is imported."""
    path = f'{INSTANCE_FOLDER}/MI_Ghost_A'
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        return None
    first = unreal.load_asset(path)
    if not isinstance(first, unreal.MaterialInstanceConstant):
        return None
    skip = set(ZONES) | set(PRIMITIVE_DATA)
    scalars = {parameter_name(v): v.get_editor_property('parameter_value') for v in first.get_editor_property('scalar_parameter_values')}
    vectors = {parameter_name(v): v.get_editor_property('parameter_value') for v in first.get_editor_property('vector_parameter_values')}
    textures = {parameter_name(v): v.get_editor_property('parameter_value') for v in first.get_editor_property('texture_parameter_values')}
    return ({k: v for k, v in scalars.items() if k not in skip}, {k: v for k, v in vectors.items() if k not in skip},
            {k: v for k, v in textures.items() if k not in skip and v is not None})


def clothes(name, zones, master, shared):
    path = f'{INSTANCE_FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        instance = unreal.load_asset(path)
        parent = instance.get_editor_property('parent') if isinstance(instance, unreal.MaterialInstanceConstant) else None
        if parent is None or parent.get_path_name() != master.get_path_name():
            log(f'{path} left alone: it is not an instance of M_Ghost')
            return
        made = 'updated'
    else:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, INSTANCE_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        if instance is None:
            raise RuntimeError(f'{path} could not be created')
        MEL.set_material_instance_parent(instance, master)
        made = 'made'
    # From scratch each time: MI_Ghost_A's settings (when it exists), then this tint's zones.
    MEL.clear_all_material_instance_parameters(instance)
    if shared:
        scalars, vectors, textures = shared
        for key, value in scalars.items():
            MEL.set_material_instance_scalar_parameter_value(instance, key, value)
        for key, value in vectors.items():
            MEL.set_material_instance_vector_parameter_value(instance, key, value)
        for key, value in textures.items():
            MEL.set_material_instance_texture_parameter_value(instance, key, value)
    for key, color in zip(ZONES, zones):
        MEL.set_material_instance_vector_parameter_value(instance, key, unreal.LinearColor(*linear(color)))
    MEL.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    source = (f'MI_Ghost_A\'s other settings ({sum(len(part) for part in shared)})' if shared
              else "M_Ghost's defaults for the rest (MI_Ghost_A isn't imported yet: run this again after SK_Unpaid)")
    log(f'{path} {made}: zones {", ".join(f"#{c:06x}" for c in zones)}; {source}')


def run():
    master = build_ghost()
    shared = first_tint_settings()
    for key in ('B', 'C'):
        clothes(f'MI_Ghost_{key}', CLOTHES[key], master, shared)
    log('done')


run()
