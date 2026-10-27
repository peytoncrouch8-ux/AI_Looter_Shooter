"""Builds what the underwater view uses (Player/UnderwaterView: UUnderwaterViewComponent), script-made (no textures, nothing
to paint or import):

  M_UnderwaterPost  /Game/Art/Materials/Masters: a post-process material after the tonemapper (display colours, so the
                    water's tints read the same in any exposure), on the view only while it's under water (an unbound
                    post-process component of the player's own; nothing at all above water). UNDERWATER_POST: the water's
                    fog by distance (Fog.x per metre, from Fog.y metres, at most Fog.z) toward its tint (ShallowTint near the
                    surface, DeepTint Fog.w metres down), the fog's light following what's behind it by Look.x (dusk water
                    darker, the surface's light brighter); a caustic shimmer on what lies under the surface near it
                    (Caustic: x strength, y scale per metre, z speed, w the depth it fades out by, metres), two crossed
                    families of wavy lines drifting; a soft dark edge (Look.y amount, Look.z softness). Parameters: Surface
                    (the water's height, cm; the code sets it), ShallowTint, DeepTint, Fog, Caustic, Look.
  SMX_Underwater    /Game/Audio/Mix: the sound mix pushed while the view is under (the world muffled): SC_Effects and
                    SC_Ambience through a LOW_PASS Hz low-pass at EFFECTS_VOLUME, SC_Music lightly (MUSIC_LOW_PASS); the
                    interface's sounds stay clear (menus, the HUD's). Fades in over FADE_IN s, out over FADE_OUT s. Needs the
                    classes build_sound_bank.py makes (SC_<Class>).

It reuses build_world_materials.py's graph helpers (without running that module's build), as build_vfx_materials.py does, and
never touches another asset: the material's graph is rebuilt in place; an asset at either path of another kind stops it.
Run it in the open editor (no C++ needed):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_underwater_assets.py"
It prints UNDERWATER lines and "UNDERWATER done".
"""
import ast
import os
import sys
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))

MIX_FOLDER = '/Game/Audio/Mix'
MIX_NAME = 'SMX_Underwater'
# The world under water: a low-pass this low (Hz) on the effects and the ambience, a little quieter; the music only dulled.
LOW_PASS = 900.0
EFFECTS_VOLUME = 0.8
MUSIC_LOW_PASS = 2600.0
FADE_IN = 0.12
FADE_OUT = 0.3

# The material's defaults: the water concept's teal (2026-10-14: shallow #43918A, deep #184A54) as display colours after
# the tonemapper, a touch darker for the murk.
DEFAULTS = {
    'ShallowTint': (0.22, 0.50, 0.48, 1.0),
    'DeepTint': (0.07, 0.24, 0.28, 1.0),
    # Fog: per metre, start (m), most, deep range (m).
    'Fog': (0.2, 0.4, 0.94, 4.0),
    # Caustic: strength, scale (per metre), speed, fades out by (m under the surface).
    'Caustic': (0.45, 0.85, 0.7, 3.5),
    # Look: the fog's light following what's behind it, the edge's darkness, its softness.
    'Look': (0.45, 0.35, 0.5, 0.0),
}

UNDERWATER_POST = """float3 c = Color.rgb;
float3 d = (WorldPos - CameraPos) * 0.01;
float dist = length(d);
float eyeDepth = max(Surface - CameraPos.z, 0.0) * 0.01;
float3 tint = lerp(ShallowTint.rgb, DeepTint.rgb, saturate(eyeDepth / max(Fog.w, 0.01)));
const float3 Wl = float3(0.299, 0.587, 0.114);
float luma = dot(c, Wl);
float tintLuma = max(dot(tint, Wl), 1e-3);
float3 fogColor = tint * lerp(1.0, clamp(luma / tintLuma, 0.25, 1.6), Look.x);
float amount = (1.0 - exp(-max(dist - Fog.y, 0.0) * Fog.x)) * Fog.z;
float under = (Surface - WorldPos.z) * 0.01;
float2 q = WorldPos.xy * 0.01 * Caustic.y;
float t = Clock * Caustic.z;
float a = sin(q.x * 1.7 + sin(q.y * 1.3 + t) * 1.4 + t * 0.6) + sin(q.y * 2.1 - sin(q.x * 0.9 - t * 0.8) * 1.2 - t * 0.5);
float b = sin(q.x * 2.3 - q.y * 0.7 + sin(q.y * 1.9 - t * 0.7) + 1.3) + sin(q.y * 1.1 + q.x * 1.6 - t * 0.9 + 2.1);
float web = exp(-a * a * 6.0) + exp(-b * b * 6.0);
float reach = saturate(1.0 - under / max(Caustic.w, 0.01)) * step(0.0, under) * (1.0 - amount);
c += c * web * Caustic.x * reach;
c = lerp(c, fogColor, amount);
float edge = smoothstep(0.35, 0.35 + max(Look.z, 0.01), length(ViewportUV - 0.5));
return c * (1.0 - edge * Look.y);"""


def log(message):
    unreal.log(f'UNDERWATER {message}')


def world_helpers():
    """build_world_materials.py's helpers (Graph, material, finish, scene_texture, its paths), without its build."""
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


def after_tonemapping():
    """The blendable location after the tonemapper (its Python name has varied)."""
    for name in ('BL_SCENE_COLOR_AFTER_TONEMAPPING', 'BL_AFTER_TONEMAPPING'):
        if hasattr(unreal.BlendableLocation, name):
            return getattr(unreal.BlendableLocation, name)
    raise RuntimeError('no BlendableLocation after the tonemapper in this engine')


def build_post():
    path = f'{W.FOLDER}/M_UnderwaterPost'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Material):
        raise RuntimeError(f'{path} exists and is not a material: nothing was changed')
    mat = W.material('M_UnderwaterPost')
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
    mat.set_editor_property('blendable_location', after_tonemapping())
    g = W.Graph(mat)
    vectors = {name: g.vector(name, value, -1000, 500 + 80 * i)
               for i, (name, value) in enumerate(DEFAULTS.items())}
    color = g.custom(UNDERWATER_POST, [
        ('Color', W.scene_texture(g, 'PostProcessInput0', -1000, -300), 'Color'),
        ('WorldPos', g.node(unreal.MaterialExpressionWorldPosition, -1000, -100), ''),
        ('CameraPos', g.node(unreal.MaterialExpressionCameraPositionWS, -1000, 0), ''),
        ('ViewportUV', g.node(unreal.MaterialExpressionScreenPosition, -1000, 100), 'ViewportUV'),
        ('Clock', g.node(unreal.MaterialExpressionTime, -1000, 200), ''),
        ('Surface', g.scalar('Surface', 0.0, -1000, 300), ''),
        ('ShallowTint', vectors['ShallowTint'], 'RGBA'), ('DeepTint', vectors['DeepTint'], 'RGBA'),
        ('Fog', vectors['Fog'], 'RGBA'), ('Caustic', vectors['Caustic'], 'RGBA'), ('Look', vectors['Look'], 'RGBA'),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 0, 'Under water')
    g.out(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    W.finish(mat, [])
    log(f'{mat.get_path_name()} built (post process, after the tonemapper)')
    return mat


def adjuster(sound_class, volume, low_pass):
    a = unreal.SoundClassAdjuster()
    a.set_editor_property('sound_class_object', sound_class)
    a.set_editor_property('volume_adjuster', volume)
    a.set_editor_property('low_pass_filter_frequency', low_pass)
    a.set_editor_property('apply_to_children', False)
    return a


def build_mix():
    path = f'{MIX_FOLDER}/{MIX_NAME}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mix = unreal.load_asset(path)
        if not isinstance(mix, unreal.SoundMix):
            raise RuntimeError(f'{path} exists and is not a sound mix: nothing was changed')
    else:
        factory_class = getattr(unreal, 'SoundMixFactory', None)
        mix = unreal.AssetToolsHelpers.get_asset_tools().create_asset(MIX_NAME, MIX_FOLDER, unreal.SoundMix,
                                                                      factory_class() if factory_class else None)
        if mix is None:
            raise RuntimeError(f'{path} could not be made')
        log(f'{path} made')
    classes = {}
    for name in ('Effects', 'Ambience', 'Music'):
        sound_class = unreal.load_asset(f'{MIX_FOLDER}/SC_{name}') if unreal.EditorAssetLibrary.does_asset_exist(f'{MIX_FOLDER}/SC_{name}') else None
        if sound_class is None:
            unreal.log_warning(f'UNDERWATER warning: no {MIX_FOLDER}/SC_{name} (build_sound_bank.py makes the classes): left out of the mix')
            continue
        classes[name] = sound_class
    effects = []
    for name in ('Effects', 'Ambience'):
        if name in classes:
            effects.append(adjuster(classes[name], EFFECTS_VOLUME, LOW_PASS))
    if 'Music' in classes:
        effects.append(adjuster(classes['Music'], 1.0, MUSIC_LOW_PASS))
    mix.set_editor_property('sound_class_effects', effects)
    mix.set_editor_property('fade_in_time', FADE_IN)
    mix.set_editor_property('fade_out_time', FADE_OUT)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mix, only_if_is_dirty=False):
        raise RuntimeError(f'{path} could not be saved')
    log(f'{path}: {", ".join(classes)} muffled (low-pass {LOW_PASS:.0f} Hz on effects and ambience, music {MUSIC_LOW_PASS:.0f} Hz)')
    return mix


def run(only=None):
    if only is None or 'post' in only:
        build_post()
    if only is None or 'mix' in only:
        build_mix()
    log('done')


# Arguments name what to build ("post", "mix"); none, both.
run(set(a.lower() for a in sys.argv[1:]) if len(sys.argv) > 1 else None)
