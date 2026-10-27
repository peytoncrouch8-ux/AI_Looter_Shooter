"""Builds the game's Niagara effects (round 5, 2026-10-27: "try Niagara effects"; Docs/Work/2026-10-27-Playtest/NIAGARA.md)
in /Game/Art/Effects/Niagara, by script: no hand-made particle assets. The code-drawn looks stay beside them; the game picks
with Looter.Fx.Niagara 1|0 (Source/.../Weapons/NiagaraFX), and a system not built yet falls back to the code's on its own.

  NS_CinderBed    the cinder jar's bed (AGraveSaltCinderBed, 2.6 m, burning 5 s; looping until the code deactivates it a
                  second before its end, then its last flames and smoke finish on their own): Heat (big flat glows lying on
                  the ground, the coals drawn into them by the material, pulsing), Flames (puffs of fire rolling into soot),
                  Tongues (tall licks of fire, drawn along their rise), Embers (sparks rising), Smoke (soot rolling up).
  NS_CinderBurst  the cinder jar's own layer on its burst (GraveSaltBurst::SpawnEffects; the salt jar's blast under it stays
                  the code's): Sparks (a fountain of embers falling back), Flare (fire puffs), Plume (soot, a moment later),
                  HeatFlash (one flat glow on the ground, quick).

Materials (here, not in Masters: they're this effect's), procedural, every one for Niagara sprites:
  M_NFX_Fire      unlit, translucent (alpha-blended: the colour holds in sun), two-sided. M_FX_Fire's own shader
                  (build_vfx_materials.py FIRE_CODE, read from that file, not copied) fed from the particle: colour = the
                  fire's (Particles.Color RGB), alpha = its opacity, the heat falls from 1 to 0 over Burn of its life (x
                  HeatScale), its age erodes it, its MaterialRandom is the seed; SmokeColor the soot. FadeIn/FadeOut (shares
                  of its life), Glow, SoftDepth (cm).
    MI_NFX_Tongue   M_NFX_Fire burning longer (Burn 0.85), faster in.
    MI_NFX_Smoke    M_NFX_Fire with no heat (HeatScale 0): soot only.
  M_NFX_Ember     unlit, additive: a soft hot dot (a streak once drawn along its velocity), flickering, cooling redder
                  with age. Glow.
  M_NFX_Heat      unlit, additive, flat on the ground: a soft pool of the particle's colour with coals in it (a grid of
                  world-space cells, CoalScale cm, three in four holding a coal pulsing at its own pace; CoalAmount),
                  fading in and out over its life (FadeIn, FadeOut). Glow.
    MI_NFX_HeatFlash  quick in, slow out, no coals (the burst's flash of heat).

How: the Cascade To Niagara Converter plugin's scripting API (unreal.FXConverterUtilitiesLibrary and its system/emitter
conversion contexts) is the engine's only Python route to author Niagara emitters and modules (NiagaraEditor exposes no
authoring functions). The plugin is editor-only (it ships nothing) and off by default: enable it in the .uproject
({"Name": "CascadeToNiagaraConverter", "Enabled": true, "TargetAllowList": ["Editor"]}) and restart the editor once. Only
its C++ library is used; every module is the engine's own (/Niagara/Modules/...).

It never touches an asset outside /Game/Art/Effects/Niagara. A system is rebuilt by deleting it and making it again (the
conversion context only adds emitters); it refuses an asset at one of its paths that isn't what it makes. Not during PIE.
Run it in the open editor, optionally naming what to build (materials, bed, burst; none = all), or "check" to see the
plugin and every module and enum it uses are there without building anything:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_niagara_fx.py [check | materials bed burst]"
It prints NIAGARAFX lines and "NIAGARAFX done".
"""
import ast
import os
import sys
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
FOLDER = '/Game/Art/Effects/Niagara'
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

# --- The engine's Niagara scripts and enums this uses (the converter's Paths.py names the same ones) ---
INITIALIZE = '/Niagara/Modules/Spawn/Initialization/V2/InitializeParticle.InitializeParticle'
SHAPE = '/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation'
ADD_VELOCITY = '/Niagara/Modules/Spawn/Velocity/AddVelocity.AddVelocity'
COLOR = '/Niagara/Modules/Update/Color/Color.Color'
EMITTER_STATE = '/Niagara/Modules/Emitter/EmitterState.EmitterState'
SPAWN_RATE = '/Niagara/Modules/Emitter/SpawnRate.SpawnRate'
SPAWN_BURST = '/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous'
PARTICLE_STATE = '/Niagara/Modules/Update/Lifetime/ParticleState.ParticleState'
ACCELERATION = '/Niagara/Modules/Update/Forces/AccelerationForce.AccelerationForce'
DRAG = '/Niagara/Modules/Update/Forces/Drag.Drag'
CURL_NOISE = '/Niagara/Modules/Update/Forces/CurlNoiseForce.CurlNoiseForce'
SOLVE = '/Niagara/Modules/Solvers/SolveForcesAndVelocity.SolveForcesAndVelocity'
SCALE_SPRITE = '/Niagara/Modules/Update/Size/ScaleSpriteSize.ScaleSpriteSize'
RANDOM_FLOAT = '/Niagara/DynamicInputs/UniformRange/V2/RandomRangeFloat.RandomRangeFloat'
RANDOM_VEC2 = '/Niagara/DynamicInputs/UniformRange/V2/RandomRangeVector2D.RandomRangeVector2D'
RANDOM_VECTOR = '/Niagara/DynamicInputs/UniformRange/V2/RandomRangeVector.RandomRangeVector'
LERP_FLOAT = '/Niagara/DynamicInputs/Lerp/Lerp_Float.Lerp_Float'
VEC2_FROM_FLOAT = '/Niagara/DynamicInputs/TypeConversions/Vector2DFromFloat.Vector2DFromFloat'
COLOR_FROM_VECTOR = '/Niagara/DynamicInputs/LinearColor/MakeLinearColorFromVectorAndFloat.MakeLinearColorFromVectorAndFloat'
ENUM_SHAPE = '/Niagara/Enums/Location/ENiagara_LocationShapes.ENiagara_LocationShapes'
ENUM_SPACE = '/Niagara/Enums/ENiagaraCoordinateSpace.ENiagaraCoordinateSpace'
ENUM_LIFE_CYCLE = '/Niagara/Enums/ENiagaraEmitterLifeCycleMode.ENiagaraEmitterLifeCycleMode'
ENUM_INACTIVE = '/Niagara/Enums/ENiagaraInactiveMode.ENiagaraInactiveMode'
ENUM_LOOP = '/Niagara/Enums/ENiagara_EmitterStateOptions.ENiagara_EmitterStateOptions'
ENUM_RANDOMNESS = '/Niagara/Enums/ENiagaraRandomnessEvaluation.ENiagaraRandomnessEvaluation'
ENUM_SIZE_MODE = '/Niagara/Enums/ENiagara_SizeScaleMode.ENiagara_SizeScaleMode'
ENUM_SCALE_SPRITE = '/Niagara/Enums/SpriteRenderer/ENiagara_ScaleSpriteSize.ENiagara_ScaleSpriteSize'
ENUM_ROTATION_MODE = '/Niagara/Enums/ENiagara_SpriteRotationMode.ENiagara_SpriteRotationMode'
SCRIPTS = (INITIALIZE, SHAPE, ADD_VELOCITY, COLOR, EMITTER_STATE, SPAWN_RATE, SPAWN_BURST, PARTICLE_STATE, ACCELERATION, DRAG,
           CURL_NOISE, SOLVE, SCALE_SPRITE, RANDOM_FLOAT, RANDOM_VEC2, RANDOM_VECTOR, LERP_FLOAT, VEC2_FROM_FLOAT, COLOR_FROM_VECTOR,
           ENUM_SHAPE, ENUM_SPACE, ENUM_LIFE_CYCLE, ENUM_INACTIVE, ENUM_LOOP, ENUM_RANDOMNESS, ENUM_SIZE_MODE, ENUM_SCALE_SPRITE,
           ENUM_ROTATION_MODE)

# --- The looks (every number of the systems is here; cm, seconds, per second). The code-drawn bed's for comparison:
# 14 flames a second over 2.6 m (22-38 cm growing to 60-100, rising 70 cm/s, 0.8-1.2 s, burning 55% of it), 4 ember puffs a
# second, the edge ring and the light (both kept in either look: they're the code's). ---
CINDER_FIRE = (1.0, 0.36, 0.08)   # GrenadeKinds.cpp CinderFire
BED_FIRE = (1.0, 0.4, 0.1)        # GraveSaltCinderBed.cpp FlameFire
EMBER = (1.0, 0.45, 0.12)
SOOT = (0.075, 0.065, 0.058)
HEAT = (0.6, 0.17, 0.04)

BED = {
    'heat': dict(rate=3.0, life=(1.4, 1.9), size=(520.0, 600.0), alpha=0.5),
    'flames': dict(rate=14.0, life=(0.8, 1.2), radius=240.0, size=(22.0, 38.0), grow=2.6, rise=(45.0, 90.0), spread=8.0,
                   lift=290.0, drag=2.0, alpha=0.85),
    'tongues': dict(rate=9.0, life=(0.4, 0.65), radius=200.0, size=((18.0, 50.0), (30.0, 85.0)), shrink=0.45,
                    rise=(150.0, 240.0), spread=15.0, lift=250.0, drag=1.0, alpha=0.9),
    'embers': dict(rate=26.0, life=(1.3, 2.2), radius=230.0, height=30.0, size=((2.2, 5.0), (4.0, 10.0)), rise=(110.0, 240.0),
                   spread=35.0, lift=40.0, drag=0.4, curl=(120.0, 50.0), glow=6.0),
    'smoke': dict(rate=5.0, life=(2.2, 3.2), radius=170.0, height=60.0, size=(70.0, 110.0), grow=3.0, rise=(60.0, 110.0),
                  spread=20.0, lift=25.0, drag=0.4, alpha=0.45),
}
BURST = {
    'duration': 0.6,
    'sparks': dict(count=70, life=(0.9, 1.7), radius=25.0, velocity=((-650.0, -650.0, 280.0), (650.0, 650.0, 1100.0)),
                   gravity=-750.0, drag=0.7, size=((3.0, 8.0), (5.0, 16.0)), glow=8.0),
    'flare': dict(count=8, life=(0.5, 0.85), radius=70.0, velocity=((-140.0, -140.0, 120.0), (140.0, 140.0, 420.0)), lift=220.0,
                  drag=2.2, size=(80.0, 140.0), grow=2.2, alpha=0.9),
    'plume': dict(count=6, time=0.12, life=(2.4, 3.4), radius=90.0, velocity=((-90.0, -90.0, 90.0), (90.0, 90.0, 190.0)),
                  lift=30.0, drag=0.5, size=(120.0, 180.0), grow=2.6, alpha=0.55),
    'flash': dict(count=1, life=(0.9, 0.9), size=(540.0, 580.0), alpha=1.0),
}


def log(message):
    unreal.log(f'NIAGARAFX {message}')


def warn(message):
    unreal.log_warning(f'NIAGARAFX {message}')


# --- Material graph helpers: build_world_materials.py's (Graph, finish, depth_fade, clear_graph) without its build, and
# build_vfx_materials.py's M_FX_Fire shader (NOISE, FIRE_CODE) without its build ---

def read_module(file_name, keep_names=None):
    """A file's imports, functions, classes and upper-case constants (or only keep_names' constants), run without the
    rest (its build at the bottom)."""
    path = os.path.join(HERE, file_name)
    with open(path, encoding='utf-8') as source:
        tree = ast.parse(source.read(), path)
    keep = []
    for node in tree.body:
        is_constant = isinstance(node, ast.Assign) and all(isinstance(t, ast.Name) and t.id.isupper() for t in node.targets)
        if keep_names is None:
            if isinstance(node, (ast.Import, ast.ImportFrom, ast.FunctionDef, ast.ClassDef)) or is_constant:
                keep.append(node)
        elif is_constant and all(t.id in keep_names for t in node.targets):
            keep.append(node)
    module = types.ModuleType(os.path.splitext(file_name)[0] + '_parts')
    module.__file__ = path
    exec(compile(ast.Module(body=keep, type_ignores=[]), path, 'exec'), module.__dict__)
    return module


W = read_module('build_world_materials.py')
VFX = read_module('build_vfx_materials.py', {'NOISE', 'FIRE_CODE'})


def own_asset(name, cls):
    """The asset at FOLDER/name if it's a cls (None when there's none); refuses anything else there."""
    path = f'{FOLDER}/{name}'
    if not EAL.does_asset_exist(path):
        return None
    asset = unreal.load_asset(path)
    if not isinstance(asset, cls):
        raise RuntimeError(f'{path} exists and is not a {cls.__name__}: nothing was changed')
    return asset


def material(name):
    mat = own_asset(name, unreal.Material)
    if mat is not None:
        W.clear_graph(mat)
        return mat
    return TOOLS.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


def instance(name, parent, scalars=None, vectors=None):
    mi = own_asset(name, unreal.MaterialInstanceConstant)
    if mi is None:
        mi = TOOLS.create_asset(name, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    for key, value in (scalars or {}).items():
        MEL.set_material_instance_scalar_parameter_value(mi, key, value)
    for key, value in (vectors or {}).items():
        MEL.set_material_instance_vector_parameter_value(mi, key, unreal.LinearColor(*value, 1.0))
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    log(f'{mi.get_path_name()} -> {parent.get_name()} {scalars or {}} {vectors or {}}')
    return mi


def particle_inputs(g, x, y):
    """The particle's colour (RGB, A), age share of its life (Particles.NormalizedAge) and random (Particles.MaterialRandom)."""
    color = g.node(unreal.MaterialExpressionParticleColor, x, y)
    age = g.node(unreal.MaterialExpressionParticleRelativeTime, x, y + 200)
    seed = g.node(unreal.MaterialExpressionParticleRandom, x, y + 280)
    return color, age, seed


# M_NFX_Fire: the particle's life drives M_FX_Fire's inputs (FIRE_CODE reads Heat, Erode, Opacity, Seed, UV, Fire, Smoke,
# Glow): the heat burns down over Burn of its life, its age erodes it, and it fades in and out at its ends.
FIRE_PARTICLE = """float Age01 = saturate(Age);
float Heat = saturate(1.0 - Age01 / max(Burn, 0.001)) * HeatScale;
float Erode = Age01;
float Opacity = Alpha * smoothstep(0.0, max(FadeIn, 0.001), Age01) * (1.0 - smoothstep(1.0 - max(FadeOut, 0.001), 1.0, Age01));
"""

# M_NFX_Ember: a soft hot dot, whiter at its heart, flickering at its own pace, cooling redder and dimmer as it ages.
EMBER_CODE = """float2 P = UV * 2.0 - 1.0;
float R = length(P);
float Dot = saturate(1.0 - R);
Dot = Dot * Dot;
float Core = saturate(1.0 - R * 2.5);
float Age01 = saturate(Age);
float Seed01 = frac(Seed);
float Flick = 0.65 + 0.35 * sin(Now * (14.0 + Seed01 * 18.0) + Seed01 * 40.0);
float Life = smoothstep(0.0, 0.05, Age01) * (1.0 - smoothstep(0.55, 1.0, Age01));
float Peak = max(max(Color.r, Color.g), Color.b);
float3 C = lerp(Color, float3(1.0, 0.85, 0.55) * Peak, Core * 0.6);
C *= lerp(1.0, 0.45, Age01) * float3(1.0, lerp(1.0, 0.55, Age01), lerp(1.0, 0.4, Age01));
return C * Dot * Alpha * Flick * Life * Glow;"""

# M_NFX_Heat: a pool of heat on the ground with coals in it. The coals are a world-space grid (one cell each pixel reads,
# so a few overlapping pools draw the same coals: the bed reads as one bed, breathing).
HEAT_CODE = """float2 P = UV * 2.0 - 1.0;
float R = length(P);
float Pool = saturate(1.0 - R);
Pool = Pool * Pool * (3.0 - 2.0 * Pool);
float Age01 = saturate(Age);
float Life = smoothstep(0.0, max(FadeIn, 0.001), Age01) * (1.0 - smoothstep(1.0 - max(FadeOut, 0.001), 1.0, Age01));
float2 G = World / max(CoalScale, 1.0);
float2 Cell = floor(G);
float H = frac(sin(dot(Cell, float2(127.1, 311.7))) * 43758.5453);
float H2 = frac(sin(dot(Cell, float2(269.5, 183.3))) * 43758.5453);
float2 Spot = float2(H, H2) * 0.5 + 0.25;
float Size = lerp(0.12, 0.24, H2);
float Lit = 1.0 - smoothstep(Size * 0.4, Size, length(frac(G) - Spot));
float Pulse = 0.55 + 0.45 * sin(Now * (1.5 + H * 2.5) + H * 6.2831);
float Coal = Lit * Pulse * step(0.25, H) * CoalAmount * saturate(Pool * 1.6);
float3 C = Color * (0.35 + 0.65 * Pool) * Pool + Color * float3(1.0, 0.7, 0.45) * Coal * 2.5;
return C * Alpha * Life * Glow;"""


def build_fire():
    mat = material('M_NFX_Fire')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1900, -100, coordinate_index=0)
    color, age, seed = particle_inputs(g, -1900, 50)
    code = g.custom(FIRE_PARTICLE + VFX.FIRE_CODE, [
        ('UV', coords, ''), ('Fire', color, 'RGB'), ('Alpha', color, 'A'), ('Age', age, ''), ('Seed', seed, ''),
        ('Smoke', g.vector('SmokeColor', SOOT + (1.0,), -1900, 450), ''),
        ('Burn', g.scalar('Burn', 0.55, -1900, 600), ''), ('HeatScale', g.scalar('HeatScale', 1.0, -1900, 680), ''),
        ('FadeIn', g.scalar('FadeIn', 0.08, -1900, 760), ''), ('FadeOut', g.scalar('FadeOut', 0.3, -1900, 840), ''),
        ('Glow', g.scalar('Glow', 1.0, -1900, 920), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT4, -1100, 300, 'Niagara fire')
    rgb = g.node(unreal.MaterialExpressionComponentMask, -800, 200, r=True, g=True, b=True, a=False)
    g.link(code, '', rgb, '')
    alpha = g.node(unreal.MaterialExpressionComponentMask, -800, 400, r=False, g=False, b=False, a=True)
    g.link(code, '', alpha, '')
    fade = W.depth_fade(g, g.scalar('SoftDepth', 40.0, -1100, 700), -600, 500)
    g.link(alpha, '', fade, 'Opacity')
    g.out(rgb, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(fade, '', unreal.MaterialProperty.MP_OPACITY)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_NIAGARA_SPRITES])
    log(f'{mat.get_path_name()} built (unlit, translucent, two-sided, Niagara sprites)')
    return mat


def build_additive(name, code_text, extra, desc):
    mat = material(name)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1900, -100, coordinate_index=0)
    color, age, seed = particle_inputs(g, -1900, 50)
    inputs = [('UV', coords, ''), ('Color', color, 'RGB'), ('Alpha', color, 'A'), ('Age', age, ''), ('Seed', seed, ''),
              ('Now', g.node(unreal.MaterialExpressionTime, -1900, 400), ''),
              ('Glow', g.scalar('Glow', 1.0, -1900, 480), '')]
    y = 560
    for pin, (kind, default) in extra.items():
        if kind == 'world':
            inputs.append((pin, g.node(unreal.MaterialExpressionWorldPosition, -1900, y), 'XY'))
        else:
            inputs.append((pin, g.scalar(pin, default, -1900, y), ''))
        y += 80
    code = g.custom(code_text, inputs, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1000, 300, desc)
    g.out(code, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_NIAGARA_SPRITES])
    log(f'{mat.get_path_name()} built (unlit, additive, two-sided, Niagara sprites)')
    return mat


def build_materials():
    fire = build_fire()
    ember = build_additive('M_NFX_Ember', EMBER_CODE, {}, 'Niagara ember')
    heat = build_additive('M_NFX_Heat', HEAT_CODE, {'World': ('world', 0.0), 'CoalScale': ('scalar', 22.0),
                                                     'CoalAmount': ('scalar', 1.0), 'FadeIn': ('scalar', 0.3),
                                                     'FadeOut': ('scalar', 0.45)}, 'Niagara heat')
    tongue = instance('MI_NFX_Tongue', fire, {'Burn': 0.85, 'FadeIn': 0.12, 'FadeOut': 0.35})
    smoke = instance('MI_NFX_Smoke', fire, {'HeatScale': 0.0, 'FadeIn': 0.15, 'FadeOut': 0.45, 'SoftDepth': 60.0})
    flash = instance('MI_NFX_HeatFlash', heat, {'FadeIn': 0.04, 'FadeOut': 0.8, 'CoalAmount': 0.0})
    return {'fire': fire, 'ember': ember, 'heat': heat, 'tongue': tongue, 'smoke': smoke, 'flash': flash}


def load_materials():
    names = {'fire': 'M_NFX_Fire', 'ember': 'M_NFX_Ember', 'heat': 'M_NFX_Heat', 'tongue': 'MI_NFX_Tongue', 'smoke': 'MI_NFX_Smoke',
             'flash': 'MI_NFX_HeatFlash'}
    found = {}
    for key, name in names.items():
        path = f'{FOLDER}/{name}'
        if not EAL.does_asset_exist(path):
            return None
        found[key] = unreal.load_asset(path)
    return found


# --- Niagara authoring through the converter's contexts ---

FX = getattr(unreal, 'FXConverterUtilitiesLibrary', None)
SPAWN = getattr(unreal, 'ScriptExecutionCategory', None)
TYPE = getattr(unreal, 'NiagaraScriptInputType', None)


def script(path, version=None):
    asset = FX.create_asset_data(path)
    return unreal.CreateScriptContextArgs(asset, version) if version else unreal.CreateScriptContextArgs(asset)


def dynamic(path, inputs, out_type, version=(1, 0)):
    """A dynamic input script (a random range, a lerp...) with its inputs set, as one input."""
    context = FX.create_script_context(script(path, list(version)))
    for name, value in inputs.items():
        context.set_parameter(name, value)
    return FX.create_script_input_dynamic(context, out_type)


def f(value):
    return FX.create_script_input_float(float(value))


def vec(x, y, z):
    return FX.create_script_input_vector(unreal.Vector(x, y, z))


def vec2(x, y):
    return FX.create_script_input_vec2(unreal.Vector2D(x, y))


def enum(path, name):
    return FX.create_script_input_enum(path, name)


def linked(name, kind):
    return FX.create_script_input_linked_parameter(name, kind)


def random_float(low, high):
    if low == high:
        return f(low)
    return dynamic(RANDOM_FLOAT, {'Minimum': f(low), 'Maximum': f(high), 'Evaluation Type': enum(ENUM_RANDOMNESS, 'Spawn Only')},
                   TYPE.FLOAT)


def random_vector(low, high):
    return dynamic(RANDOM_VECTOR, {'Minimum': vec(*low), 'Maximum': vec(*high), 'Evaluation Type': enum(ENUM_RANDOMNESS, 'Spawn Only')},
                   TYPE.VEC3)


def random_vec2(low, high):
    return dynamic(RANDOM_VEC2, {'Minimum': vec2(*low), 'Maximum': vec2(*high), 'Evaluation Type': enum(ENUM_RANDOMNESS, 'Spawn Only')},
                   TYPE.VEC2)


def square(size):
    """A sprite size from one number (a float input) as Vector2D."""
    return dynamic(VEC2_FROM_FLOAT, {'Value': size}, TYPE.VEC2)


def over_life(start, end):
    """A float from start to end over the particle's life (Particles.NormalizedAge)."""
    return dynamic(LERP_FLOAT, {'A': f(start), 'B': f(end), 'Alpha': linked('Particles.NormalizedAge', TYPE.FLOAT)}, TYPE.FLOAT)


def colour(rgb, alpha):
    return dynamic(COLOR_FROM_VECTOR, {'Vector (RGB)': vec(*rgb), 'Float (Alpha)': f(alpha)}, TYPE.LINEAR_COLOR)


class Emitter:
    """One emitter, its modules added in stack order (each category in the order called)."""

    def __init__(self, system, name, looping, duration):
        self.name = name
        self.e = system.add_empty_emitter(name)
        self.e.set_sim_target(unreal.NiagaraSimTarget.CPU_SIM)
        self.e.set_local_space(False)
        state = self.e.find_or_add_module_script('EmitterState', script(EMITTER_STATE, [1, 0]), SPAWN.EMITTER_UPDATE)
        state.set_parameter('Life Cycle Mode', enum(ENUM_LIFE_CYCLE, 'Self'))
        state.set_parameter('Inactive Response', enum(ENUM_INACTIVE, 'Complete (Let Particles Finish then Kill Emitter)'))
        state.set_parameter('Loop Behavior', enum(ENUM_LOOP, 'Infinite' if looping else 'Once'))
        state.set_parameter('Loop Duration', f(duration))

    def rate(self, per_second):
        spawn = self.e.find_or_add_module_script('SpawnRate', script(SPAWN_RATE, [1, 0]), SPAWN.EMITTER_UPDATE)
        spawn.set_parameter('SpawnRate', f(per_second))

    def burst(self, count, time=0.0):
        spawn = self.e.find_or_add_module_script('SpawnBurst', script(SPAWN_BURST, [1, 1]), SPAWN.EMITTER_UPDATE)
        spawn.set_parameter('Spawn Count', FX.create_script_input_int(int(count)))
        spawn.set_parameter('Spawn Time', f(time))

    def initialize(self, life, size, spin=True):
        """Its life (low, high), its sprite size (a Vector2D input) and a random turn; its MaterialRandom (the shaders' seed)."""
        init = self.e.find_or_add_module_script('InitializeParticle', script(INITIALIZE, [1, 0]), SPAWN.PARTICLE_SPAWN)
        init.set_parameter('Lifetime', random_float(*life))
        init.set_parameter('Sprite Size Mode', enum(ENUM_SIZE_MODE, 'Non-Uniform'))
        init.set_parameter('Sprite Size', size, True, True)
        if spin:
            init.set_parameter('Sprite Rotation Mode', enum(ENUM_ROTATION_MODE, 'Direct Normalized Angle (0-1)'))
            init.set_parameter('Sprite Rotation Angle', random_float(0.0, 1.0))
        self.e.set_parameter_directly('Particles.MaterialRandom', random_float(0.0, 1.0), SPAWN.PARTICLE_SPAWN)

    def disc(self, radius, height=0.0):
        """Spawned over a disc of radius lying on the system's plane (the ground, its up the system's Z), up to height over it;
        scaled with the component (a bigger bed spreads wider)."""
        shape = self.e.find_or_add_module_script('ShapeLocation', script(SHAPE), SPAWN.PARTICLE_SPAWN)
        shape.set_parameter('Shape Primitive', enum(ENUM_SHAPE, 'Cylinder'))
        shape.set_parameter('Cylinder Radius', f(radius))
        shape.set_parameter('Cylinder Height', f(max(height, 0.1)))
        shape.set_parameter('Cylinder Height Midpoint', f(0.0))
        shape.set_parameter('Apply Owner Scale', FX.create_script_input_bool(True))

    def ball(self, radius):
        shape = self.e.find_or_add_module_script('ShapeLocation', script(SHAPE), SPAWN.PARTICLE_SPAWN)
        shape.set_parameter('Shape Primitive', enum(ENUM_SHAPE, 'Sphere'))
        shape.set_parameter('Sphere Radius', f(radius))
        shape.set_parameter('Apply Owner Scale', FX.create_script_input_bool(True))

    def velocity(self, low, high):
        """A world-space velocity at random between low and high (fire rises up the world's Z, whatever slope it's on)."""
        add = self.e.find_or_add_module_script('AddVelocity', script(ADD_VELOCITY, [1, 2]), SPAWN.PARTICLE_SPAWN)
        add.set_parameter('Velocity', random_vector(low, high))
        add.set_parameter('Rotation Coordinate Space', enum(ENUM_SPACE, 'World'))

    def color(self, rgb, alpha):
        tint = self.e.find_or_add_module_script('InitialColor', script(COLOR), SPAWN.PARTICLE_SPAWN)
        tint.set_parameter('Color', colour(rgb, alpha))

    def flat(self):
        """Lying on the ground: facing along the system's up (the ground's normal)."""
        self.e.set_parameter_directly('Particles.SpriteFacing', linked('Engine.Owner.SystemZAxis', TYPE.VEC3), SPAWN.PARTICLE_SPAWN)

    def update(self, lift=0.0, drag=0.0, curl=None, scale=None):
        """Its age (ParticleState), a steady push up the world's Z (lift; negative falls), drag, curl noise (strength,
        frequency), the forces solved, then its sprite scaled over its life (start, end) from its first size."""
        self.e.find_or_add_module_script('ParticleState', script(PARTICLE_STATE, [1, 1]), SPAWN.PARTICLE_UPDATE)
        if lift:
            push = self.e.find_or_add_module_script('Acceleration', script(ACCELERATION), SPAWN.PARTICLE_UPDATE)
            push.set_parameter('Acceleration', vec(0.0, 0.0, lift))
        if drag:
            slow = self.e.find_or_add_module_script('Drag', script(DRAG, [1, 1]), SPAWN.PARTICLE_UPDATE)
            slow.set_parameter('Drag', f(drag))
        if curl:
            noise = self.e.find_or_add_module_script('CurlNoise', script(CURL_NOISE), SPAWN.PARTICLE_UPDATE)
            noise.set_parameter('Noise Strength', f(curl[0]))
            noise.set_parameter('Noise Frequency', f(curl[1]))
        # Always solved: the spawn velocities move it even with no force on it.
        self.e.find_or_add_module_script('SolveForcesAndVelocity', script(SOLVE), SPAWN.PARTICLE_UPDATE)
        if scale:
            size = self.e.find_or_add_module_script('ScaleSprite', script(SCALE_SPRITE, [1, 1]), SPAWN.PARTICLE_UPDATE)
            size.set_parameter('Scale Sprite Size Mode', enum(ENUM_SCALE_SPRITE, 'Uniform'))
            size.set_parameter('Uniform Scale Factor', over_life(*scale))

    def sprites(self, material, facing='camera', sort=True):
        props = unreal.NiagaraSpriteRendererProperties()
        props.set_editor_property('Material', material)
        if facing == 'velocity':
            props.set_editor_property('Alignment', unreal.NiagaraSpriteAlignment.VELOCITY_ALIGNED)
            props.set_editor_property('FacingMode', unreal.NiagaraSpriteFacingMode.FACE_CAMERA)
        elif facing == 'ground':
            props.set_editor_property('Alignment', unreal.NiagaraSpriteAlignment.UNALIGNED)
            props.set_editor_property('FacingMode', unreal.NiagaraSpriteFacingMode.CUSTOM_FACING_VECTOR)
        else:
            props.set_editor_property('Alignment', unreal.NiagaraSpriteAlignment.UNALIGNED)
            props.set_editor_property('FacingMode', unreal.NiagaraSpriteFacingMode.FACE_CAMERA)
        props.set_editor_property('SortMode', unreal.NiagaraSortMode.VIEW_DEPTH if sort else unreal.NiagaraSortMode.NONE)
        self.e.add_renderer('SpriteRenderer', props)


def new_system(name):
    """An empty system at FOLDER/name; one there already (only a Niagara system) is deleted first: the conversion context
    only adds emitters, so a rebuild starts clean."""
    path = f'{FOLDER}/{name}'
    if own_asset(name, unreal.NiagaraSystem) is not None:
        if not EAL.delete_asset(path):
            raise RuntimeError(f'{path} could not be deleted (open in PIE?): nothing was changed')
    system = TOOLS.create_asset(name, FOLDER, unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if system is None:
        raise RuntimeError(f'{path} could not be made')
    return system, FX.create_system_conversion_context(system)


def save_system(system, context, emitters):
    context.finalize()
    EAL.save_loaded_asset(system)
    log(f'{system.get_path_name()} built: {", ".join(emitters)}')


def build_bed(m):
    system, context = new_system('NS_CinderBed')
    b = BED

    heat = Emitter(context, 'Heat', True, 2.0)
    heat.rate(b['heat']['rate'])
    heat.initialize(b['heat']['life'], square(random_float(*b['heat']['size'])))
    heat.disc(40.0, 10.0)
    heat.color(HEAT, b['heat']['alpha'])
    heat.flat()
    heat.update()
    heat.sprites(m['heat'], facing='ground', sort=False)

    flames = Emitter(context, 'Flames', True, 2.0)
    n = b['flames']
    flames.rate(n['rate'])
    flames.initialize(n['life'], square(random_float(*n['size'])))
    flames.disc(n['radius'], 6.0)
    flames.velocity((-n['spread'], -n['spread'], n['rise'][0]), (n['spread'], n['spread'], n['rise'][1]))
    flames.color(BED_FIRE, n['alpha'])
    flames.update(lift=n['lift'], drag=n['drag'], scale=(1.0, n['grow']))
    flames.sprites(m['fire'])

    tongues = Emitter(context, 'Tongues', True, 2.0)
    n = b['tongues']
    tongues.rate(n['rate'])
    tongues.initialize(n['life'], random_vec2(*n['size']), spin=False)
    tongues.disc(n['radius'], 6.0)
    tongues.velocity((-n['spread'], -n['spread'], n['rise'][0]), (n['spread'], n['spread'], n['rise'][1]))
    tongues.color(BED_FIRE, n['alpha'])
    tongues.update(lift=n['lift'], drag=n['drag'], scale=(1.0, n['shrink']))
    tongues.sprites(m['tongue'], facing='velocity')

    embers = Emitter(context, 'Embers', True, 2.0)
    n = b['embers']
    embers.rate(n['rate'])
    embers.initialize(n['life'], random_vec2(*n['size']), spin=False)
    embers.disc(n['radius'], n['height'])
    embers.velocity((-n['spread'], -n['spread'], n['rise'][0]), (n['spread'], n['spread'], n['rise'][1]))
    embers.color(tuple(c * n['glow'] for c in EMBER), 1.0)
    embers.update(lift=n['lift'], drag=n['drag'], curl=n['curl'])
    embers.sprites(m['ember'], facing='velocity', sort=False)

    smoke = Emitter(context, 'Smoke', True, 2.0)
    n = b['smoke']
    smoke.rate(n['rate'])
    smoke.initialize(n['life'], square(random_float(*n['size'])))
    smoke.disc(n['radius'], n['height'])
    smoke.velocity((-n['spread'], -n['spread'], n['rise'][0]), (n['spread'], n['spread'], n['rise'][1]))
    smoke.color((1.0, 1.0, 1.0), n['alpha'])
    smoke.update(lift=n['lift'], drag=n['drag'], scale=(1.0, n['grow']))
    smoke.sprites(m['smoke'])

    save_system(system, context, ['Heat', 'Flames', 'Tongues', 'Embers', 'Smoke'])


def build_burst(m):
    system, context = new_system('NS_CinderBurst')
    b = BURST

    flash = Emitter(context, 'HeatFlash', False, b['duration'])
    n = b['flash']
    flash.burst(n['count'])
    flash.initialize(n['life'], square(random_float(*n['size'])))
    flash.disc(1.0, 10.0)
    flash.color(HEAT, n['alpha'])
    flash.flat()
    flash.update()
    flash.sprites(m['flash'], facing='ground', sort=False)

    sparks = Emitter(context, 'Sparks', False, b['duration'])
    n = b['sparks']
    sparks.burst(n['count'])
    sparks.initialize(n['life'], random_vec2(*n['size']), spin=False)
    sparks.ball(n['radius'])
    sparks.velocity(*n['velocity'])
    sparks.color(tuple(c * n['glow'] for c in EMBER), 1.0)
    sparks.update(lift=n['gravity'], drag=n['drag'])
    sparks.sprites(m['ember'], facing='velocity', sort=False)

    flare = Emitter(context, 'Flare', False, b['duration'])
    n = b['flare']
    flare.burst(n['count'])
    flare.initialize(n['life'], square(random_float(*n['size'])))
    flare.ball(n['radius'])
    flare.velocity(*n['velocity'])
    flare.color(CINDER_FIRE, n['alpha'])
    flare.update(lift=n['lift'], drag=n['drag'], scale=(1.0, n['grow']))
    flare.sprites(m['fire'])

    plume = Emitter(context, 'Plume', False, b['duration'])
    n = b['plume']
    plume.burst(n['count'], n['time'])
    plume.initialize(n['life'], square(random_float(*n['size'])))
    plume.ball(n['radius'])
    plume.velocity(*n['velocity'])
    plume.color((1.0, 1.0, 1.0), n['alpha'])
    plume.update(lift=n['lift'], drag=n['drag'], scale=(1.0, n['grow']))
    plume.sprites(m['smoke'])

    save_system(system, context, ['HeatFlash', 'Sparks', 'Flare', 'Plume'])


def check():
    """Whether it can run: the converter's library and every engine script and enum it names."""
    good = True
    if FX is None:
        warn('unreal.FXConverterUtilitiesLibrary is missing: enable the "Cascade To Niagara Converter" plugin (editor only) '
             'in AI_Looter_Shooter.uproject and restart the editor')
        good = False
    for path in SCRIPTS:
        package = path.split('.')[0]
        if not EAL.does_asset_exist(package):
            warn(f'missing engine asset {package}')
            good = False
    log('check: ready' if good else 'check: NOT ready (see the warnings above)')
    return good


def run(only):
    if 'check' in only:
        check()
        log('done')
        return
    if not check():
        log('done (nothing built)')
        return
    build_all = not only
    mats = build_materials() if build_all or 'materials' in only else load_materials()
    if mats is None:
        warn('the materials are not built yet: run with "materials" first (or no arguments)')
        log('done (nothing built)')
        return
    if build_all or 'bed' in only:
        build_bed(mats)
    if build_all or 'burst' in only:
        build_burst(mats)
    log('done')


# Arguments name what to build ("py .../build_niagara_fx.py bed"); none, all of it; "check" builds nothing.
run(set(a.lower() for a in sys.argv[1:]))
