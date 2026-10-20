"""Builds the enemy tiers' shell look (Creatures/CreatureTierLookComponent.h) beside the world's masters:

  M_CreatureTier  /Game/Art/Materials/Masters: what a creature above Regular wears on its tier shell (the body's mesh drawn
                  once more on a follower of its pose; never the body's overlay slot, which the hit flash and the ember
                  powers' tints use). Unlit, translucent, one-sided, no textures, so a horde of Ethereals costs one cheap
                  translucent draw each on Medium. Three masks, each switched on by the code's shared instance for a kind
                  and tier (UCreatureTierFXSubsystem::GetShellLook), all in the tier's Color:
                    eyes (EyeOn): UV 0 inside EyeMin..EyeMax (the spider atlas's 'eye' patch) burns at Glow;
                    core (CoreOn): the inside parts (vertex colour red under a half: the slime's core, pebbles and seeds;
                      the gel is white) within CoreRadius cm of CoreCenter in the mesh's own rest pose (its pre-skinned
                      position, so the slime's squash and hops never move the mask off the core) burns at Glow;
                    the ghost-rim (RimOn, the Ethereal's): a thin Fresnel line round the outline (RimPower, RimOpacity,
                      RimGlow), shimmering in soft bands that crawl up the body (ShimmerScale per cm, ShimmerSpeed,
                      ShimmerDepth) with a quick waver, lifted off the body a little more than the rest (RimPush cm,
                      breathing) so it reads as a faint aura, not paint.
                  Push (cm) lifts it all off the surface along the normal, so it never loses the depth test to its own body.
                  Per creature, through the shell's custom primitive data (never an instance's): Flare (0, CreatureTierLook::
                  FlareIndex) brightens it all (FlareGain; the notice sting's flare, the death's), Fade (1, FadeIndex: 1
                  shown, 0 gone) fades it with the body (a death, an Unpaid's phase-step). ShroudFade 1 (an Unpaid's shell)
                  fades it with vertex alpha too, as the shroud fades (M_Ghost: 1 solid, 0 gone); on every other model that
                  alpha is baked occlusion (M_World's), so it's off there.

The code sets Color, Glow, EyeOn, EyeMin, EyeMax, CoreOn, CoreCenter, CoreRadius, RimOn and ShroudFade; the rest are the
look's tuning, at these defaults. Used on skeletal meshes only.

It never touches another asset: the master's graph is rebuilt in place (instances stay linked), and an asset at its path
that isn't a material is left alone. It reuses build_world_materials.py's graph helpers (without running that module's
build). Run it in the open editor (it needs no C++):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_tier_material.py"
It prints TIERMAT lines and "TIERMAT done".
"""
import ast
import os
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
NAME = 'M_CreatureTier'

# The Ethereal's red (#FF3B3B, LooterUI::Color::TierEthereal) in linear: a default the code overwrites per tier.
ETHEREAL = (1.0, 0.0437, 0.0437, 1.0)
DEFAULTS = {
    'Glow': 8.0,            # the eyes' and core's emissive multiple of Color (the code sets a kind's and tier's)
    'EyeOn': 0.0,
    'CoreOn': 0.0,
    'CoreRadius': 15.0,     # cm, the mesh at rest
    'InsideOnly': 1.0,      # the core takes only the inside parts (vertex red under a half)
    'RimOn': 0.0,
    'ShroudFade': 0.0,      # 1: fades with vertex alpha (the Unpaid's shroud); elsewhere that alpha is baked occlusion
    'RimGlow': 2.4,        # the ghost-rim's emissive multiple of Color: faint, but red over the body's own colours
    'RimOpacity': 0.55,
    'RimPower': 3.0,        # how tight it hugs the outline (higher: thinner)
    'ShimmerScale': 0.035,  # bands per cm of height (about one every 30 cm)
    'ShimmerSpeed': 0.6,    # bands a second crawling up
    'ShimmerDepth': 0.6,    # how far a band dims it (0: steady)
    'FlareGain': 2.0,       # Flare 1 makes it this much brighter again
    'Push': 0.4,            # cm along the normal
    'RimPush': 1.2,         # cm more for the ghost-rim, breathing
}
# Per creature, from the shell's custom primitive data (CreatureTierLook::FlareIndex, FadeIndex): never an instance's.
PRIMITIVE_DATA = {'Flare': (0, 0.0), 'Fade': (1, 1.0)}

SHARED = """// The eyes: a rectangle of UV 0 (the spider atlas's 'eye' patch).
float2 Uv = UV;
float Eye = EyeOn * step(EyeMin.x, Uv.x) * step(Uv.x, EyeMax.x) * step(EyeMin.y, Uv.y) * step(Uv.y, EyeMax.y);
// The core: the inside parts (vertex red under a half) near the core's middle in the mesh's rest pose.
float Inside = lerp(1.0, step(VertexColor.r, 0.5), InsideOnly);
float CoreDistance = length(LocalPos - CoreCenter);
float Core = CoreOn * Inside * (1.0 - smoothstep(CoreRadius * 0.6, CoreRadius, CoreDistance));
// The ghost-rim: a Fresnel line round the outline, in soft bands crawling up the body, wavering.
float Facing = saturate(dot(normalize(Normal), normalize(CameraVector)));
float RimAmount = pow(1.0 - Facing, RimPower);
float Bands = 0.5 + 0.5 * sin((WorldPos.z * ShimmerScale - Time * ShimmerSpeed) * 6.2831853
    + 2.0 * sin(WorldPos.x * 0.021 + WorldPos.y * 0.017 + Time * 0.7));
float Waver = 0.85 + 0.15 * sin(Time * 9.0 + WorldPos.z * 0.05);
float Rim = RimOn * RimAmount * lerp(1.0, Bands, ShimmerDepth) * Waver;
// Shown as the body is: the code's fade (death, a phase-step) and, on an Unpaid (ShroudFade), its shroud's (vertex alpha;
// on every other model that alpha is baked occlusion, M_World's, and must not put the glow out in its creases).
float Shown = saturate(Fade) * lerp(1.0, smoothstep(0.2, 0.7, VertexAlpha), ShroudFade);
float Lift = 1.0 + max(Flare, 0.0) * FlareGain;
"""
OPACITY = SHARED + 'return saturate(max(Eye, Core) + Rim * RimOpacity * Lift) * Shown;'
COLOR = SHARED + 'return Color * ((Eye + Core) * Glow + Rim * RimGlow) * Lift;'
PUSH = """// Off its own surface, the ghost-rim a little farther and breathing, so it sits round the body as an aura.
return Push + RimOn * RimPush * (0.6 + 0.4 * sin(Time * 2.1));"""


def log(message):
    unreal.log(f'TIERMAT {message}')


def world_helpers():
    """build_world_materials.py's helpers (Graph, material, finish, its paths), without its build: that module builds its
    masters as it's run, so only its imports, definitions and constants are run here (as build_highlight_material.py does)."""
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


def master():
    """The master, its graph emptied to build again; refuses an asset at its path that isn't a material."""
    path = f'{W.FOLDER}/{NAME}'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Material):
        raise RuntimeError(f'{path} exists and is not a material: nothing was changed')
    return W.material(NAME)


def local_position(g, center):
    """The mesh's own rest-pose position handed from the vertex shader to the pixels (as M_Gun's notches read it). If the
    editor can't wire it, the core's centre stands in: the core mask then takes every inside part (pebbles and seeds too)."""
    try:
        local = g.node(unreal.MaterialExpressionVertexInterpolator, -1500, 900)
        g.link(g.node(unreal.MaterialExpressionPreSkinnedPosition, -1800, 900), '', local, 'VS')
        return local
    except Exception as error:   # noqa: BLE001 - the look still works with a looser core mask; say so
        unreal.log_warning(f'TIERMAT the pre-skinned position could not be wired ({error}): the core glows on every inside part')
        return center


def build():
    mat = master()
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', False)
    g = W.Graph(mat)
    uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1800, -500, coordinate_index=0)
    vertex = g.node(unreal.MaterialExpressionVertexColor, -1800, -400)
    normal = g.node(unreal.MaterialExpressionVertexNormalWS, -1800, -300)
    camera = g.node(unreal.MaterialExpressionCameraVectorWS, -1800, -200)
    world = g.node(unreal.MaterialExpressionWorldPosition, -1800, -100)
    time = g.node(unreal.MaterialExpressionTime, -1800, 0)
    color = g.vector('Color', ETHEREAL, -2200, -700)
    eye_min = g.vector('EyeMin', (0.752, 0.877, 0.0, 0.0), -2200, -600)
    eye_max = g.vector('EyeMax', (0.8105, 0.998, 0.0, 0.0), -2200, -500)
    center = g.vector('CoreCenter', (0.0, 0.0, 30.0, 0.0), -2200, -400)
    s = {key: g.scalar(key, value, -2200, -300 + 60 * i) for i, (key, value) in enumerate(DEFAULTS.items())}
    cpd = {key: g.node(unreal.MaterialExpressionScalarParameter, -2200, 700 + 80 * i, parameter_name=key, default_value=default,
                       use_custom_primitive_data=True, primitive_data_index=index)
           for i, (key, (index, default)) in enumerate(PRIMITIVE_DATA.items())}
    local = local_position(g, center)

    shared = [('UV', uv, ''), ('VertexColor', vertex, ''), ('VertexAlpha', vertex, 'A'), ('Normal', normal, ''),
              ('CameraVector', camera, ''), ('WorldPos', world, ''), ('Time', time, ''), ('LocalPos', local, ''),
              ('EyeMin', eye_min, ''), ('EyeMax', eye_max, ''), ('CoreCenter', center, ''),
              ('Flare', cpd['Flare'], ''), ('Fade', cpd['Fade'], '')]
    shared += [(key, s[key], '') for key in ('EyeOn', 'CoreOn', 'CoreRadius', 'InsideOnly', 'RimOn', 'ShroudFade', 'RimPower',
                                             'ShimmerScale', 'ShimmerSpeed', 'ShimmerDepth', 'FlareGain')]
    opacity = g.custom(OPACITY, shared + [('RimOpacity', s['RimOpacity'], '')],
                       unreal.CustomMaterialOutputType.CMOT_FLOAT1, -900, 300, 'Tier opacity')
    emissive = g.custom(COLOR, shared + [('Color', color, ''), ('Glow', s['Glow'], ''), ('RimGlow', s['RimGlow'], '')],
                        unreal.CustomMaterialOutputType.CMOT_FLOAT3, -900, -300, 'Tier glow')
    push = g.custom(PUSH, [('Push', s['Push'], ''), ('RimOn', s['RimOn'], ''), ('RimPush', s['RimPush'], ''), ('Time', time, '')],
                    unreal.CustomMaterialOutputType.CMOT_FLOAT1, -900, 700, 'Tier push')
    offset = g.mul(normal, '', push, '', -500, 700)
    g.out(emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    g.out(offset, '', unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH])

    scalars = sorted(str(n) for n in MEL.get_scalar_parameter_names(mat))
    vectors = sorted(str(n) for n in MEL.get_vector_parameter_names(mat))
    for needed in ('Glow', 'EyeOn', 'CoreOn', 'CoreRadius', 'RimOn', 'ShroudFade', 'Flare', 'Fade'):
        if needed not in scalars:
            raise RuntimeError(f'{NAME} has no scalar {needed}: the code could not drive it')
    for needed in ('Color', 'EyeMin', 'EyeMax', 'CoreCenter'):
        if needed not in vectors:
            raise RuntimeError(f'{NAME} has no colour {needed}: the code could not drive it')
    log(f'{mat.get_path_name()} built (unlit, translucent, one-sided): scalars {", ".join(scalars)}; colors {", ".join(vectors)}; '
        f'custom primitive data {PRIMITIVE_DATA}')
    return mat


build()
log('done')
