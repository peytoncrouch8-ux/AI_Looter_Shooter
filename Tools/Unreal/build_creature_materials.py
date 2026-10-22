"""Builds the creatures' materials (Docs/Areas/RansomsRest.md, step 16) beside the world's masters:

  M_Ghost       /Game/Art/Materials/Masters: the Unpaid (and Hob's ash later), masked and two-sided, its opacity mask
                dithered (DitherTemporalAA), never translucent, so a dozen in a fight stay cheap on Medium. Per vertex:
                R picks one of four tint zones at 0, 1/3, 2/3 and 1 (Zone1Color, skin and shroud, to Zone4Color, the
                accents), G darkens the cavities, B is the ember edge round the coal's hole, A fades the body into the
                shroud (1 solid, 0 gone). The fade is broken up by the macro noise at two scales drifting up the strips
                (UV 0's V runs down them: NoiseScale, NoiseSpeed; FadeSoftness the phase-step's edge). The Polymer grain
                (BaseColorMap on UV 0 times UVScale) runs through the cloth, and Tint multiplies it all. Emissive: a pale
                rim (RimColor, RimStrength, and RimPower: how tightly it hugs the silhouette), a faint glow of its own
                (GlowStrength) and the ember edge in the rank's color (EmberStrength).
                The painted ghost (CHAR-ART item 1, the user's OK 2026-10-22, sheet Saved/ArtPreviews/Characters/
                Concepts/CharArt_1): light painted in, so every ghost reads under any lighting, Medium included. Each
                new parameter's default is the look and no instance sets them, so every M_Ghost user has it with no
                re-import (PAINTED below):
                  - cloth (ClothMap: T_GhostCloth_M from Art/Textures/GhostCloth/make_ghost_cloth.py, on UV 0 times
                    ClothScale): the weave and seams (ClothWeave; SkinCloth of it on zone 1, the skin and shroud),
                    hanging folds (ClothFolds) and grave-dirt stains (ClothStains toward StainTint) multiplying the tint;
                  - top light by height on the body (its pre-skinned Z, cm): TopLightLow at TopLightFrom (the waist)
                    to TopLightHigh at TopLightTo (the head); a small prop on the slot (the hat: its bounds' height under
                    PropSize cm)
                    takes PropTopLight;
                  - the cavities harder (vertex G to the CavityPower, times CavityStrength);
                  - the rim leaning cold (ColdRim of the way to ColdRimColor; the instances' RimColor stays the base);
                  - inner glow: the rank's color lighting the body round the coal (InnerGlow, falling off over
                    InnerGlowRadius cm from CoalAt, the coal's rest place in the mesh's own space), with the coal's heat;
                  - the fade stretched into streaks along the strips (StreakAcross x StreakAlong) over a wider dithered
                    band (FadeBand), so the shroud melts into the air instead of ending in shards.
                One texture sample more than before (the cloth); the pre-skinned position is one interpolator.
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
  MI_SpiderBody_Pale  the Gravemother's bone-pale hide (Side 3): M_World with Spider.py's pale color map
                (T_SpiderBody_Pale_BC, imported here whenever the PNG changes) over the brown spider's normal and ORM.

It never touches another asset: M_Ghost's graph is rebuilt in place (its instances stay linked), and an asset at one of
these paths that isn't what this makes is left alone. It reuses build_world_materials.py's graph helpers (without running
that module's build). Run it in the open editor (it needs no C++):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_creature_materials.py [M_Ghost MI_SpiderBody_Pale]"
It prints a CREATUREMATS line per asset and "CREATUREMATS done". M_Ghost imports T_GhostCloth_M again whenever its PNG
changes.
"""
import ast
import hashlib
import os
import sys
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
INSTANCE_FOLDER = '/Game/Art/Materials'
POLYMER = '/Game/Art/Textures/Polymer/T_Polymer_BC'
DITHER = '/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA'
# The Gravemother's pale hide: the color map Spider.py paints, on M_World with the brown spider's other maps.
PALE_HIDE = 'MI_SpiderBody_Pale'
PALE_FILE = os.path.normpath(os.path.join(HERE, '..', '..', 'Art', 'Textures', 'SpiderBody_Pale', 'T_SpiderBody_Pale_BC.png'))
PALE_MAP = '/Game/Art/Textures/SpiderBody_Pale/T_SpiderBody_Pale_BC'
SPIDER_MAPS = '/Game/Art/Textures/SpiderBody/T_SpiderBody'
WORLD_MASTER = '/Game/Art/Materials/Masters/M_World'
# The painted ghost's cloth (linear data: R weave and seams, G folds, B stains).
CLOTH_FILE = os.path.normpath(os.path.join(HERE, '..', '..', 'Art', 'Textures', 'GhostCloth', 'T_GhostCloth_M.png'))
CLOTH_MAP = '/Game/Art/Textures/GhostCloth/T_GhostCloth_M'

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
COLD_RIM = 0xa4e0ee       # the painted ghost's cold rim (sheet 1): a pale cyan, far from the rare blue
STAIN = 0x9c8c78          # grave dirt: what a stain multiplies the cloth by at full strength (sRGB)
BASIC_COAL = 0xb02a18     # a Basic coal's dull red: what a mesh no creature drives shows
COAL_CRUST = 0x1d110c

# The scalars on M_Ghost and their defaults: an instance that sets none of them looks right.
DEFAULTS = dict(RimStrength=1.5, RimPower=2.8, GlowStrength=0.1, EmberStrength=3.0, NoiseScale=1.5, NoiseSpeed=0.06,
                FadeSoftness=0.07, Coal=0.0, UVScale=1.0)
# The painted ghost's (sheet 1). Heights are the mesh's own (pre-skinned) cm: every ghost is built standing on its origin,
# the pelvis (the float height) at 100, the head at about 170 (Abel and the Foreman are scaled by their actors, not built
# bigger). COAL_AT: the coal's middle at rest (Unpaid.py, DeadMiner.py, Abel.py, Amos.py, Foreman.py: 1.32-1.33 m up,
# 0.24-0.3 rad round to his left from the front), within a few cm on every ghost; SK_Unpaid's coal bone, read back
# from its FBX (Unreal's x forward, y right).
PAINTED = dict(ClothScale=1.25, ClothWeave=0.18, SkinCloth=0.35, ClothFolds=0.40, ClothStains=0.45,
               TopLightLow=0.72, TopLightHigh=1.18, TopLightFrom=90.0, TopLightTo=165.0, PropSize=60.0, PropTopLight=1.10,
               CavityStrength=0.95, CavityPower=0.6, ColdRim=0.7, InnerGlow=1.0, InnerGlowRadius=26.0,
               StreakAcross=3.0, StreakAlong=0.35, FadeBand=0.28)
COAL_AT = (10.9, -5.9, 131.3)
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
// The Polymer grain runs through it by its brightness (about 0.8 on average, so the zone keeps its value).
float Grain = clamp(dot(GrainColor, float3(0.299, 0.587, 0.114)) * 1.25, 0.5, 1.5);
// The painted cloth (T_GhostCloth_M): the weave and its seams mostly on the clothes (zone 1 is skin and shroud), the
// hanging folds and the grave-dirt stains on everything. Each averages 1, so a zone keeps its value from afar.
float OnCloth = ZoneIndex < 0.5 ? SkinCloth : 1.0;
float Weave = 1.0 + ClothWeave * OnCloth * (Painted.r - 0.5) * 2.0;
float Fold = 1.0 + ClothFolds * (Painted.g - 0.5) * 2.0;
float3 Stain = lerp(float3(1.0, 1.0, 1.0), StainTint, saturate(Painted.b * ClothStains));
// Top light painted in by height on the body (its own rest-pose cm): head and shoulders lit, darker toward the waist
// and down the shroud. A small prop on the slot (the hat) has a small space of its own: it takes PropTopLight.
float Up = smoothstep(TopFrom, TopTo, LocalPos.z);
float Top = Bounds.z > PropSize ? lerp(TopLow, TopHigh, Up) : PropTop;
// The cavities (vertex G) pushed harder than a straight darkening: sockets, mouth, under the vest.
float Cavity = 1.0 - CavityStrength * pow(max(VertexColor.g, 1e-4), CavityPower);
float3 Body = Cloth * Grain * Tint * Weave * Fold * Stain * (Top * Cavity);
// The coal is a dark crust under its glow.
return lerp(Body, Crust, saturate(CoalAmount));"""

GHOST_GLOW = """// The rank's color, which the creature sets in its custom primitive data (alpha: how strongly it glows). A mesh no
// creature drives (the bestiary's stand) has none: a Basic coal's dull red.
float3 RankGlow = RankAlpha > 0.0 ? RankColor * RankAlpha : BasicColor * 1.8;
float Burn = max(1.0 + Heat, 0.0);
float Facing = abs(dot(normalize(Normal), normalize(CameraVector)));
// RimPower keeps the rim to the silhouette: a low one covers a thin limb's whole face in daylight. The rim leans cold
// (ColdRim of the way to ColdRimColor): the light round a ghost against any ground.
float RimAmount = pow(saturate(1.0 - Facing), RimPower);
float Dark = 1.0 - CavityStrength * pow(max(VertexColor.g, 1e-4), CavityPower);
float3 Rim = lerp(RimColor, ColdRimColor, saturate(ColdRim));
// The coal lights the body round it from inside, in the rank's color and as hot as it burns (a sunk ember: none).
float Near = length(LocalPos - CoalAt) / max(InnerRadius, 1.0);
float Inner = InnerGlow * exp(-Near * Near) * Burn;
// The body: the rim, a faint glow of its own (a ghost never goes fully dark), the inner glow and the ember edge round
// the coal's hole (vertex B).
float3 BodyGlow = Rim * (RimStrength * RimAmount * Dark) + BaseColor * GlowStrength
    + RankGlow * (VertexColor.b * EmberStrength * Burn + Inner * Dark);
// The coal: glowing cracks in a dark crust, hottest in the middle where it faces the eye.
float Crack = Texture2DSample(Noise, NoiseSampler, UV * 3.0 + float2(0.0, Time * 0.02)).r;
float Cracks = lerp(0.25, 1.0, 1.0 - saturate(abs(Crack - 0.5) * 7.0));
float3 CoalGlow = RankGlow * (EmberStrength * 1.6 * Burn * Cracks * lerp(0.45, 1.0, Facing));
return lerp(BodyGlow, CoalGlow, saturate(CoalAmount));"""

GHOST_OPACITY = """// Two scales of the noise drifting up the strips (UV 0's V runs down them), stretched along them into streaks, so the
// shroud thins into the air in wisps.
float2 Drift = float2(0.0, Time * NoiseSpeed);
float2 Streak = float2(StreakAcross, StreakAlong);
float N = Texture2DSample(Noise, NoiseSampler, (UV + Drift) * NoiseScale * Streak).r * 0.62
        + Texture2DSample(Noise, NoiseSampler, (UV + Drift * 1.6) * (NoiseScale * 3.07) * Streak + float2(0.37, 0.11)).r * 0.38;
float Steep = 1.0 / max(FadeSoftness, 0.005);
// The fade into the shroud (vertex A: 1 solid, 0 gone), broken up by the streaks over a wide dithered band (FadeBand):
// the strips melt away instead of ending in hard shards. The coal never fades this way.
float Shroud = lerp(saturate((Fade * 1.3 - 0.15 - N) / max(FadeBand, 0.005) + 0.5), 1.0, saturate(CoalAmount));
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


def cloth_texture():
    """T_GhostCloth_M: linear data (sRGB off, Masks compression), imported again whenever its PNG changes (its MD5 kept on
    the texture as metadata). Its path, or a white stand-in (with a warning) until the PNG is made."""
    if not os.path.exists(CLOTH_FILE):
        unreal.log_warning(f'CREATUREMATS {CLOTH_FILE} is missing (run Art/Textures/GhostCloth/make_ghost_cloth.py): '
                           'M_Ghost paints no cloth until it is made')
        return W.WHITE
    with open(CLOTH_FILE, 'rb') as f:
        md5 = hashlib.md5(f.read()).hexdigest()
    if unreal.EditorAssetLibrary.does_asset_exist(CLOTH_MAP):
        if unreal.EditorAssetLibrary.get_metadata_tag(unreal.load_asset(CLOTH_MAP), 'SourceMD5') == md5:
            return CLOTH_MAP
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', CLOTH_FILE)
    task.set_editor_property('destination_path', CLOTH_MAP.rsplit('/', 1)[0])
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(CLOTH_MAP)
    texture.set_editor_property('srgb', False)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD_SPECULAR)
    unreal.EditorAssetLibrary.set_metadata_tag(texture, 'SourceMD5', md5)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    log(f'{CLOTH_MAP} imported (source {md5[:8]})')
    return CLOTH_MAP


def local_position(g, x, y):
    """The mesh's own rest-pose position (pre-skinned, cm) handed from the vertex shader to the pixels, as M_Gun's notches
    and M_CreatureTier's core read it: the top light and the coal's inner glow ride the body as it moves."""
    local = g.node(unreal.MaterialExpressionVertexInterpolator, x, y)
    g.link(g.node(unreal.MaterialExpressionPreSkinnedPosition, x - 300, y), '', local, 'VS')
    return local


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
    cloth_map = cloth_texture()
    mat = W.material('M_Ghost')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)

    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1800, 200, coordinate_index=0)
    grain_uv = g.mul(coords, '', g.scalar('UVScale', DEFAULTS['UVScale'], -1800, 320), '', -1600, 260)
    grain = g.texture('BaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, grain_texture(), grain_uv, -1400, 200)
    cloth_uv = g.mul(coords, '', g.scalar('ClothScale', PAINTED['ClothScale'], -1800, 420), '', -1600, 400)
    painted = g.texture('ClothMap', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, cloth_map, cloth_uv, -1400, 320)
    vertex = g.node(unreal.MaterialExpressionVertexColor, -1400, 500)
    noise = g.node(unreal.MaterialExpressionTextureObjectParameter, -1400, 700, parameter_name='NoiseMap', texture=noise_texture())
    time = g.node(unreal.MaterialExpressionTime, -1400, 850)
    coal = g.scalar('Coal', DEFAULTS['Coal'], -1400, 950)
    zones = [g.vector(name, linear(color), -1400, -600 + 100 * index) for index, (name, color) in enumerate(zip(ZONES, CLOTHES['A']))]
    local = local_position(g, -1400, 1650)
    bounds = g.node(unreal.MaterialExpressionObjectBounds, -1400, 1750)
    p = {key: g.scalar(key, value, -2200, -900 + 60 * i) for i, (key, value) in enumerate(PAINTED.items()) if key != 'ClothScale'}
    cavity = [('CavityStrength', p['CavityStrength'], ''), ('CavityPower', p['CavityPower'], '')]

    color = g.custom(GHOST_COLOR, [
        ('VertexColor', vertex, ''),
        ('GrainColor', grain, 'RGB'),
        ('Painted', painted, 'RGB'),
        ('Zone1', zones[0], ''), ('Zone2', zones[1], ''), ('Zone3', zones[2], ''), ('Zone4', zones[3], ''),
        ('Tint', g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1400, -200), ''),
        ('Crust', g.node(unreal.MaterialExpressionConstant3Vector, -1400, -100, constant=unreal.LinearColor(*linear(COAL_CRUST))), ''),
        ('CoalAmount', coal, ''),
        ('SkinCloth', p['SkinCloth'], ''), ('ClothWeave', p['ClothWeave'], ''), ('ClothFolds', p['ClothFolds'], ''),
        ('ClothStains', p['ClothStains'], ''),
        ('StainTint', g.vector('StainTint', linear(STAIN), -2200, 400), 'RGB'),
        ('LocalPos', local, ''), ('Bounds', bounds, ''),
        ('TopLow', p['TopLightLow'], ''), ('TopHigh', p['TopLightHigh'], ''), ('TopFrom', p['TopLightFrom'], ''),
        ('TopTo', p['TopLightTo'], ''), ('PropSize', p['PropSize'], ''), ('PropTop', p['PropTopLight'], ''),
    ] + cavity, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -900, -300, 'Ghost color')

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
        ('RimColor', g.vector('RimColor', linear(RIM), -1100, 600), 'RGB'),
        ('ColdRimColor', g.vector('ColdRimColor', linear(COLD_RIM), -2200, 500), 'RGB'),
        ('ColdRim', p['ColdRim'], ''),
        ('RimStrength', g.scalar('RimStrength', DEFAULTS['RimStrength'], -1100, 700), ''),
        ('RimPower', g.scalar('RimPower', DEFAULTS['RimPower'], -1100, 750), ''),
        ('GlowStrength', g.scalar('GlowStrength', DEFAULTS['GlowStrength'], -1100, 800), ''),
        ('EmberStrength', g.scalar('EmberStrength', DEFAULTS['EmberStrength'], -1100, 900), ''),
        ('CoalAmount', coal, ''),
        ('Noise', noise, ''),
        ('UV', coords, ''),
        ('Time', time, ''),
        ('LocalPos', local, ''),
        ('CoalAt', g.vector('CoalAt', COAL_AT + (0.0,), -2200, 600), 'RGB'),
        ('InnerGlow', p['InnerGlow'], ''), ('InnerRadius', p['InnerGlowRadius'], ''),
    ] + cavity, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 300, 'Ghost glow')

    opacity = g.custom(GHOST_OPACITY, [
        ('Fade', vertex, 'A'),
        ('Noise', noise, ''),
        ('UV', coords, ''),
        ('Time', time, ''),
        ('NoiseScale', g.scalar('NoiseScale', DEFAULTS['NoiseScale'], -900, 1100), ''),
        ('NoiseSpeed', g.scalar('NoiseSpeed', DEFAULTS['NoiseSpeed'], -900, 1200), ''),
        ('FadeSoftness', g.scalar('FadeSoftness', DEFAULTS['FadeSoftness'], -900, 1300), ''),
        ('StreakAcross', p['StreakAcross'], ''), ('StreakAlong', p['StreakAlong'], ''), ('FadeBand', p['FadeBand'], ''),
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


def import_color_map(file, path):
    """A color map as the model importer brings one in (sRGB, default compression, the World group), imported again
    whenever the PNG changes (its MD5 kept on the texture as metadata)."""
    with open(file, 'rb') as f:
        md5 = hashlib.md5(f.read()).hexdigest()
    exists = unreal.EditorAssetLibrary.does_asset_exist(path)
    if exists and unreal.EditorAssetLibrary.get_metadata_tag(unreal.load_asset(path), 'SourceMD5') == md5:
        return unreal.load_asset(path), False
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', file)
    task.set_editor_property('destination_path', path.rsplit('/', 1)[0])
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(path)
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
    unreal.EditorAssetLibrary.set_metadata_tag(texture, 'SourceMD5', md5)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return texture, True


def pale_hide():
    """MI_SpiderBody_Pale, the Gravemother's bone-pale hide (Side 3; look A, Bone, the user's pick): M_World with the pale
    color map Art/Models/Creatures/Spider.py paints (its BONE palette) over the brown spider's own normal and ORM maps.
    M_World's Tint only multiplies, so the brown map can't be tinted pale. AGravemotherCreature wears it on SK_Spider's
    slot 0 (restart the editor after making it: the class finds it as its defaults are built)."""
    texture, imported = import_color_map(PALE_FILE, PALE_MAP)
    path = f'{INSTANCE_FOLDER}/{PALE_HIDE}'
    master = unreal.load_asset(WORLD_MASTER)
    textures = {'BaseColorMap': texture, 'NormalMap': unreal.load_asset(f'{SPIDER_MAPS}_N'),
                'ORMMap': unreal.load_asset(f'{SPIDER_MAPS}_ORM')}
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        instance = unreal.load_asset(path)
        if not isinstance(instance, unreal.MaterialInstanceConstant):
            log(f'{path} left alone: it is not a material instance')
            return
        made = 'updated'
    else:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            PALE_HIDE, INSTANCE_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        made = 'made'
    worn = {parameter_name(v): v.get_editor_property('parameter_value')
            for v in instance.get_editor_property('texture_parameter_values')}
    if (made == 'updated' and not imported and instance.get_editor_property('parent') == master
            and all(worn.get(k) == v for k, v in textures.items())):
        log(f'{path} unchanged')
        return
    MEL.set_material_instance_parent(instance, master)
    MEL.clear_all_material_instance_parameters(instance)
    for key, value in textures.items():
        MEL.set_material_instance_texture_parameter_value(instance, key, value)
    MEL.set_material_instance_vector_parameter_value(instance, 'Tint', unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    MEL.set_material_instance_scalar_parameter_value(instance, 'UVScale', 1.0)
    MEL.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    log(f'{path} {made}: {PALE_MAP} on the spider\'s own normal and ORM maps')


def run(wanted):
    """Everything, or only the named assets (M_Ghost with its tints MI_Ghost_B and _C, or MI_SpiderBody_Pale)."""
    if not wanted or {'M_Ghost', 'MI_Ghost_B', 'MI_Ghost_C'} & wanted:
        master = build_ghost()
        shared = first_tint_settings()
        for key in ('B', 'C'):
            clothes(f'MI_Ghost_{key}', CLOTHES[key], master, shared)
    if not wanted or PALE_HIDE in wanted:
        pale_hide()
    log('done')


run(set(sys.argv[1:]))
