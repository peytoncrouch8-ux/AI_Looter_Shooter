"""Builds the VFX pass's three masters (Saved/Vfx/PLAN.md, 2026-10-21 night) beside the world's masters, procedural (no
textures, so nothing to paint or import):

  M_FX_Shape       /Game/Art/Materials/Masters: unlit, additive, two-sided, for instanced static meshes (FWeaponFX's "Shapes"
                   component on the engine's Plane). Per-instance custom data (WeaponFXDraw.cpp's layout, ShapeFloats 7):
                   0-2 colour (linear), 3 strength, 4 shape (EVfxShape: 0 ring, 1 fireball, 2 starburst), 5 param (a ring's
                   thickness as a share of its radius; a fireball's heat, 1 white-hot .. 0 a dull ember edge; a starburst's
                   spikes, 3-16), 6 seed (0..1). Emissive = shape's colour x mask x strength x Glow (1). Fireballs and
                   starbursts are softened where they meet what's behind them (SoftDepth cm); rings are not (they lie on the
                   ground, a few cm over it).
  M_FX_Puff        /Game/Art/Materials/Masters: unlit, translucent, two-sided, instanced static meshes (FWeaponFX's "Smoke"
                   once it exists: PuffFloats 6). Custom data 0-2 colour (the light already in it: unlit), 3 opacity, 4 seed
                   (below 0: a plain soft disc, the water rings' dashes), 5 erosion (0 fresh .. 1 thinned away: the noise
                   eats it into wisps as it ages). Two octaves of value noise drift with the erosion (the billow turning
                   over); denser parts a touch darker. Soft where it meets what's behind it (SoftDepth cm).
  M_FX_Fire        /Game/Art/Materials/Masters (VFX-2, 2026-10-22): unlit, translucent (alpha-blended), two-sided,
                   instanced static meshes (FWeaponFX's "Fire": FireFloats 10). Custom data 0-2 fire colour, 3-5 smoke colour
                   (the light in it), 6 opacity, 7 heat (1 white-hot .. 0 smoke), 8 seed, 9 erosion. Fire rolling into soot
                   that keeps its colour in full sun (additive fire washed out to a pale puff). Glow scales the fire.
  M_FX_SplatDecal  /Game/Art/Materials/Masters: a deferred decal, translucent (DBuffer colour and roughness: lit as the ground
                   it lies on, on Medium without Lumen as on High). A blob with a noisy edge out to Spread (0..1 of the
                   decal's radius; the code spreads it), a scatter of droplets past the edge, its colour and opacity the
                   decal's colour (UDecalComponent::DecalColor, RGB and A), Wet (0 matte .. 1 glossy: lower roughness),
                   Seed (0..1, its shape), faded by the decal's lifetime (the pool's hold and fade).
  M_FX_Muzzle      /Game/Art/Materials/Masters (the muzzle rework, 2026-10-22): unlit, additive, two-sided, instanced static
                   meshes (AWeaponBase's MuzzleFlash on the engine Plane; the old M_FX_Glow's flat-filled streaks washed to
                   flat yellow cards). Custom data (WeaponBaseMuzzle.cpp, 8 floats): 0-2 colour, 3 strength, 4 shape
                   (EVfxFlashShape, M_FX_Glow's numbers: 0 streak, 1 round, 2 star), 5 param (a streak's length share this
                   shot; a round's softness, 0 tight white-hot core .. 1 wide bloom; a star's spikes 3-16), 6 seed (each
                   shot's), 7 the crosshair's hole (a share of the screen's width round its middle where nothing shows; 0
                   none: third person). Every shape falls off softly (gaussian), whitens toward its heart, never shows the
                   quad's edge, and fades as the quad turns edge-on (never a hard line). Shapes drawn by LocalPosition (the
                   Plane is -50..50; its UVs aren't the obvious axes). No depth fade (the first-person flash is drawn pulled
                   in toward the eye).

FWeaponFX (WeaponFX.cpp) loads the first two at the start of a level's effects and draws today's stand-ins until they exist;
FDeathSplats (DeathSplats.cpp) lays no splats until the third exists; guns flash on M_FX_Glow until M_FX_Muzzle exists
(its shape numbers match). It never touches another asset: the masters' graphs
are rebuilt in place, and an asset at one of these paths that isn't a material is left alone. It reuses
build_world_materials.py's graph helpers (without running that module's build), as build_grave_fight_materials.py does.
Run it in the open editor (no C++ needed):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_vfx_materials.py"
It prints VFXMATS lines and "VFXMATS done".
"""
import ast
import os
import sys
import types

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))


def log(message):
    unreal.log(f'VFXMATS {message}')


def world_helpers():
    """build_world_materials.py's helpers (Graph, material, finish, depth_fade, its paths), without its build."""
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


def master(name):
    """A master by that name, its graph emptied to build again; refuses an asset there that isn't a material."""
    path = f'{W.FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not isinstance(unreal.load_asset(path), unreal.Material):
        raise RuntimeError(f'{path} exists and is not a material: nothing was changed')
    return W.material(name)


def custom_data(g, index, default, x, y):
    return g.node(unreal.MaterialExpressionPerInstanceCustomData, x, y, data_index=index, const_default_value=default)


def parameter_names(mat):
    scalars = sorted(str(n) for n in MEL.get_scalar_parameter_names(mat))
    vectors = sorted(str(n) for n in MEL.get_vector_parameter_names(mat))
    return f'scalars {", ".join(scalars)}; colors {", ".join(vectors)}'


def instance_color(g, x, y):
    """Custom data 0-2 as one float3 (the colour the code wrote)."""
    rg = g.node(unreal.MaterialExpressionAppendVector, x + 200, y)
    rgb = g.node(unreal.MaterialExpressionAppendVector, x + 400, y)
    g.link(custom_data(g, 0, 1.0, x, y - 60), '', rg, 'A')
    g.link(custom_data(g, 1, 1.0, x, y + 20), '', rg, 'B')
    g.link(rg, '', rgb, 'A')
    g.link(custom_data(g, 2, 1.0, x, y + 100), '', rgb, 'B')
    return rgb


# Two octaves of value noise at Q into N (0..1), written out (a Custom node's code is one function body: no helpers).
NOISE = """float N = 0.0;
float Amp = 0.62;
float2 QQ = Q;
for (int Octave = 0; Octave < 2; ++Octave)
{
    float2 I = floor(QQ);
    float2 F = frac(QQ);
    F = F * F * (3.0 - 2.0 * F);
    float H00 = frac(sin(dot(I, float2(127.1, 311.7))) * 43758.5453);
    float H10 = frac(sin(dot(I + float2(1.0, 0.0), float2(127.1, 311.7))) * 43758.5453);
    float H01 = frac(sin(dot(I + float2(0.0, 1.0), float2(127.1, 311.7))) * 43758.5453);
    float H11 = frac(sin(dot(I + float2(1.0, 1.0), float2(127.1, 311.7))) * 43758.5453);
    N += lerp(lerp(H00, H10, F.x), lerp(H01, H11, F.x), F.y) * Amp;
    QQ = QQ * 2.07 + float2(3.1, 1.7);
    Amp *= 0.5;
}
N = saturate(N / 0.93);
"""

SHAPE_CODE = """float2 P = UV * 2.0 - 1.0;
float R = length(P);
float A = atan2(P.y, P.x);
float S = floor(Shape + 0.5);
float Seed01 = frac(Seed);
float3 C = Color;
float M = 0.0;
float Soften = 1.0;
if (S < 0.5)
{
    // A ring: a soft band of Param's share of the radius, brightest at its leading (outer) edge, a little broken.
    float2 Q = float2(A * 2.2 + Seed01 * 17.0, Seed01 * 5.0);
""" + NOISE + """    float T = clamp(Param, 0.02, 1.0);
    float Mid = 1.0 - T * 0.5;
    float Band = saturate(1.0 - abs(R - Mid) / (T * 0.5));
    Band = Band * Band * (3.0 - 2.0 * Band);
    float Lead = smoothstep(1.0 - T, 1.0, R);
    M = Band * lerp(0.55, 1.0, Lead) * lerp(0.6, 1.0, N);
}
else if (S < 1.5)
{
    // A fireball: a noisy ball, white-hot at the heart while it's hot, its colour as it cools, then a dull ember edge the
    // noise eats away.
    float2 Q = P * 2.3 + float2(Seed01 * 13.1, Seed01 * 7.7);
""" + NOISE + """    float H = saturate(Param);
    float Body = saturate(((1.0 - R) + (N - 0.5) * 0.6) * 1.6);
    Body = saturate(Body - (1.0 - H) * (1.0 - N) * 0.9);
    float Core = saturate(1.0 - R / max(0.2 + 0.5 * H, 0.05)) * H;
    float3 Hot = float3(1.0, 0.95, 0.85);
    float3 Ember = float3(0.35, 0.06, 0.02);
    // White only at the very start: fire reads by its colour in daylight (additive white-hot washed out to a white cloud).
    float3 Heat = H > 0.75 ? lerp(Color, Hot, (H - 0.75) * 4.0) : lerp(Ember, Color, saturate(H / 0.75));
    C = lerp(Heat, Hot, Core * saturate((H - 0.6) * 2.5));
    M = Body * (0.6 + 0.4 * N);
    Soften = SoftDepth;
}
else
{
    // A starburst: Param uneven spikes round a hot core, turned by its seed.
    float K = clamp(floor(Param + 0.5), 3.0, 16.0);
    float Turn = Seed01 * 6.2831853;
    float Sector = floor(frac((A + Turn) / 6.2831853 + 1.0) * K);
    float Jitter = frac(sin(Sector * 12.9898 + Seed01 * 78.233) * 43758.5453);
    float Length = lerp(0.5, 1.0, Jitter);
    float Spike = pow(abs(cos((A + Turn) * K * 0.5)), 16.0);
    float Fall = saturate(1.0 - R / Length);
    float Core = pow(saturate(1.0 - R * 2.6), 2.0);
    M = Spike * Fall * Fall + Core * 1.2;
    C = lerp(Color, float3(1.0, 0.97, 0.9), saturate(1.0 - R * 3.0));
    Soften = SoftDepth;
}
// Never a quad's edge.
M *= saturate((1.0 - R) * 8.0);
return C * M * Strength * Glow * Soften;"""

PUFF_CODE = """float2 P = UV * 2.0 - 1.0;
float R = length(P);
float Disc = saturate(1.0 - R);
Disc = Disc * Disc * (3.0 - 2.0 * Disc);
if (Seed < 0.0)
{
    // A plain soft disc (the water rings' dashes).
    return float4(Color, Disc * Opacity);
}
// The billow: noise drifting as it ages, eating it into wisps as it thins.
float2 Q = P * 1.8 + float2(Seed * 37.1, Seed * 11.7) + float2(0.0, Erode * 0.7);
""" + NOISE + """float Dens = saturate(Disc * (0.45 + 0.9 * N) - Erode * 0.42);
Dens = saturate(Dens * 1.35);
float Shade = lerp(0.78, 1.06, N);
return float4(Color * Shade, Dens * Opacity);"""

# M_FX_Fire (VFX-2, 2026-10-22): a puff of fire rolling into soot, alpha-blended so its colour holds in full sun. Its noise
# (one look, two octaves: a big blast costs its overdraw, so the shader stays light) eats it into wisps as it ages; it burns
# hottest at the heart and in the thick of the billow and cools from the outside in: soot, deep red, its colour, white-yellow.
FIRE_CODE = """float2 P = UV * 2.0 - 1.0;
float R = length(P);
float Seed01 = frac(Seed);
float2 Q = P * 2.2 + float2(Seed01 * 37.1, Seed01 * 11.7) + float2(0.0, Erode * 0.9);
float N1 = 0.0;
{
""" + NOISE + """N1 = N;
}
float Bill = N1;
float Disc = saturate(1.0 - R);
Disc = Disc * Disc * (3.0 - 2.0 * Disc);
float Dens = saturate(Disc * (0.65 + Bill) - Erode * 0.42);
Dens = saturate(Dens * 2.2);
// Hottest at the heart, the billow's thick parts hotter; the rim burns dark first (a sooty edge round the fire).
float Core = saturate(1.0 - R / 0.75);
float T = Heat * (0.35 + 0.75 * Core) + (Bill - 0.5) * 0.5 * Heat - smoothstep(0.5, 0.95, R) * 0.35;
T = saturate(T);
float3 Soot = Smoke * lerp(0.5, 1.4, Bill);
float3 Ember = Fire * float3(0.7, 0.22, 0.12) * 1.2;
float3 Flame = Fire * 1.9 * Glow;
float3 Hot = float3(1.0, 0.8, 0.45) * 4.0 * Glow;
float3 C = lerp(Soot, Ember, smoothstep(0.06, 0.28, T));
C = lerp(C, Flame, smoothstep(0.3, 0.6, T));
C = lerp(C, Hot, smoothstep(0.78, 1.0, T));
// Never a quad's edge.
float A = saturate(Dens * Opacity * (1.0 + T)) * saturate((1.0 - R) * 6.0);
return float4(C, A);"""

SPLAT_OPACITY = """float2 P = UV * 2.0 - 1.0;
float R = length(P);
float A = atan2(P.y, P.x);
float Seed01 = frac(Seed);
// The pool's edge: out to Spread, wobbling with its noise.
float2 Q = float2(A * 1.6 + Seed01 * 31.0, Seed01 * 7.0);
""" + NOISE + """float Edge = Spread * (0.6 + 0.3 * N);
float Blob = 1.0 - smoothstep(Edge - 0.06, Edge, R);
// Droplets thrown past the edge (within the spread): a few cells of a grid lit at random.
float2 G = P * 6.0 + Seed01 * 13.0;
float2 Cell = floor(G);
float Lit = frac(sin(dot(Cell, float2(12.9898, 78.233))) * 43758.5453);
float Drop = Lit > 0.78 ? 1.0 - smoothstep(0.12, 0.22, length(frac(G) - 0.5)) : 0.0;
Drop *= step(Edge, R) * step(R, min(Spread, 0.98));
return saturate(Blob + Drop) * saturate(Code.a) * Life;"""

# M_FX_Muzzle (the muzzle rework, 2026-10-22): soft light, never a card. The core burns white; the thin spikes and tongues
# saturate only at their hearts, their soft edges keeping the colour (strengths in VfxRulesMuzzle.cpp).
MUZZLE_CODE = """float2 P = Local.xy / 50.0;
float R = length(P);
float S = floor(Shape + 0.5);
float Seed01 = frac(Seed);
float3 Hot = float3(1.0, 0.95, 0.85);
float3 C = Color;
float M = 0.0;
float Facing = abs(dot(normalize(Normal), normalize(ToCamera)));
float Turn = 1.0;
if (S < 0.5)
{
    // A tapered tongue of flame: its hot head at +X (the muzzle end), thinning to a soft point Param of the way along.
    float Len = clamp(Param, 0.2, 1.0);
    float T = saturate((1.0 - P.x) / (2.0 * Len));
    float Half = lerp(0.85, 0.06, T);
    float Across = abs(P.y) / Half;
    float Body = exp(-Across * Across * 2.5);
    float Along = pow(1.0 - T, 1.6) * saturate((1.0 - P.x) * 8.0);
    M = Body * Along;
    C = lerp(Color, Hot, saturate(Body * Body * (1.0 - T) * (1.0 - T)));
    // Edge-on a streak is a hard line: gone as it turns away.
    Turn = smoothstep(0.08, 0.45, Facing);
}
else if (S < 1.5)
{
    // A round glow: Param 0 a tight white-hot core, 1 a wide soft bloom.
    float K = lerp(14.0, 3.2, saturate(Param));
    M = exp(-R * R * K);
    float Heart = exp(-R * R * K * 4.0);
    C = lerp(Color, Hot, Heart * (1.0 - saturate(Param)));
    Turn = smoothstep(0.05, 0.35, Facing);
}
else
{
    // Thin tapered spikes round a hot point, each its own length (and a little its own strength) by the seed, turned by it.
    float K = clamp(floor(Param + 0.5), 3.0, 16.0);
    float A = atan2(P.y, P.x) + Seed01 * 6.2831853;
    float Step = 6.2831853 / K;
    float N = floor(A / Step + 0.5);
    float D = A - N * Step;
    float Jitter = frac(sin(N * 12.9898 + Seed01 * 78.233) * 43758.5453);
    float Len = lerp(0.5, 1.0, Jitter);
    float Run = saturate(R / Len);
    float Perp = R * abs(sin(D)) / lerp(0.09, 0.006, Run);
    float Spike = exp(-Perp * Perp) * pow(1.0 - Run, 1.5) * lerp(0.7, 1.0, Jitter);
    float Core = exp(-R * R * 60.0);
    float Halo = exp(-R * R * 10.0) * 0.25;
    M = Spike + Core * 1.5 + Halo;
    C = lerp(Color, Hot, saturate(Core + Spike * (1.0 - Run) * 0.5));
    Turn = smoothstep(0.1, 0.45, Facing);
}
// Never a quad's edge.
M *= saturate((1.0 - max(abs(P.x), abs(P.y))) * 10.0);
// The crosshair's hole: nothing within Clear (a share of the screen's width) of the screen's middle.
float Hole = 1.0;
if (Clear > 0.0)
{
    float2 Off = (Screen - 0.5) * float2(1.0, ViewSize.y / max(ViewSize.x, 1.0));
    Hole = smoothstep(Clear * 0.7, Clear, length(Off));
}
return C * M * Strength * Glow * Turn * Hole;"""

SPLAT_COLOR = """float2 P = UV * 2.0 - 1.0;
// A little deeper toward its middle, where it's thickest.
return Code.rgb * (0.85 + 0.3 * saturate(1.0 - length(P)));"""


def build_shape():
    mat = master('M_FX_Shape')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1900, 0, coordinate_index=0)
    soft = W.depth_fade(g, g.scalar('SoftDepth', 12.0, -1900, 900), -1600, 900)
    code = g.custom(SHAPE_CODE, [
        ('UV', coords, ''), ('Color', instance_color(g, -1900, 200), ''),
        ('Strength', custom_data(g, 3, 1.0, -1900, 450), ''), ('Shape', custom_data(g, 4, 0.0, -1900, 550), ''),
        ('Param', custom_data(g, 5, 0.2, -1900, 650), ''), ('Seed', custom_data(g, 6, 0.0, -1900, 750), ''),
        ('Glow', g.scalar('Glow', 1.0, -1900, 850), ''), ('SoftDepth', soft, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1000, 300, 'Shapes')
    g.out(code, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES])
    log(f'{mat.get_path_name()} built (unlit, additive, two-sided): {parameter_names(mat)}; custom data 0-2 colour, '
        f'3 strength, 4 shape, 5 param, 6 seed')
    return mat


def build_puff():
    mat = master('M_FX_Puff')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1900, 0, coordinate_index=0)
    code = g.custom(PUFF_CODE, [
        ('UV', coords, ''), ('Color', instance_color(g, -1900, 200), ''),
        ('Opacity', custom_data(g, 3, 1.0, -1900, 450), ''), ('Seed', custom_data(g, 4, -1.0, -1900, 550), ''),
        ('Erode', custom_data(g, 5, 0.0, -1900, 650), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT4, -1100, 300, 'Puff')
    rgb = g.node(unreal.MaterialExpressionComponentMask, -800, 200, r=True, g=True, b=True, a=False)
    g.link(code, '', rgb, '')
    alpha = g.node(unreal.MaterialExpressionComponentMask, -800, 400, r=False, g=False, b=False, a=True)
    g.link(code, '', alpha, '')
    fade = W.depth_fade(g, g.scalar('SoftDepth', 30.0, -1100, 700), -600, 500)
    g.link(alpha, '', fade, 'Opacity')
    g.out(rgb, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(fade, '', unreal.MaterialProperty.MP_OPACITY)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES])
    log(f'{mat.get_path_name()} built (unlit, translucent, two-sided): {parameter_names(mat)}; custom data 0-2 colour, '
        f'3 opacity, 4 seed (<0 plain), 5 erosion')
    return mat


def build_fire():
    mat = master('M_FX_Fire')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1900, 0, coordinate_index=0)

    def rgb_at(first, x, y):
        rg = g.node(unreal.MaterialExpressionAppendVector, x + 200, y)
        rgb = g.node(unreal.MaterialExpressionAppendVector, x + 400, y)
        g.link(custom_data(g, first, 1.0, x, y - 60), '', rg, 'A')
        g.link(custom_data(g, first + 1, 0.5, x, y + 20), '', rg, 'B')
        g.link(rg, '', rgb, 'A')
        g.link(custom_data(g, first + 2, 0.1, x, y + 100), '', rgb, 'B')
        return rgb

    code = g.custom(FIRE_CODE, [
        ('UV', coords, ''), ('Fire', rgb_at(0, -1900, 200), ''), ('Smoke', rgb_at(3, -1900, 450), ''),
        ('Opacity', custom_data(g, 6, 1.0, -1900, 650), ''), ('Heat', custom_data(g, 7, 0.0, -1900, 750), ''),
        ('Seed', custom_data(g, 8, 0.0, -1900, 850), ''), ('Erode', custom_data(g, 9, 0.0, -1900, 950), ''),
        ('Glow', g.scalar('Glow', 1.0, -1900, 1050), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT4, -1100, 300, 'Fire')
    rgb = g.node(unreal.MaterialExpressionComponentMask, -800, 200, r=True, g=True, b=True, a=False)
    g.link(code, '', rgb, '')
    alpha = g.node(unreal.MaterialExpressionComponentMask, -800, 400, r=False, g=False, b=False, a=True)
    g.link(code, '', alpha, '')
    fade = W.depth_fade(g, g.scalar('SoftDepth', 40.0, -1100, 700), -600, 500)
    g.link(alpha, '', fade, 'Opacity')
    g.out(rgb, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(fade, '', unreal.MaterialProperty.MP_OPACITY)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES])
    log(f'{mat.get_path_name()} built (unlit, translucent, two-sided): {parameter_names(mat)}; custom data 0-2 fire, 3-5 smoke, '
        f'6 opacity, 7 heat, 8 seed, 9 erosion')
    return mat


def build_splat():
    mat = master('M_FX_SplatDecal')
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    g = W.Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1900, 0, coordinate_index=0)
    code = g.node(unreal.MaterialExpressionDecalColor, -1900, 200)
    life = g.node(unreal.MaterialExpressionDecalLifetimeOpacity, -1900, 300)
    opacity = g.custom(SPLAT_OPACITY, [
        ('UV', coords, ''), ('Code', code, 'RGBA'), ('Life', life, ''),
        ('Spread', g.scalar('Spread', 1.0, -1900, 400), ''), ('Seed', g.scalar('Seed', 0.0, -1900, 500), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1100, 300, 'Splat')
    color = g.custom(SPLAT_COLOR, [('UV', coords, ''), ('Code', code, 'RGBA')], unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                     -1100, 0, 'Splat colour')
    rough = g.node(unreal.MaterialExpressionLinearInterpolate, -800, 600)
    g.link(g.node(unreal.MaterialExpressionConstant, -1100, 550, r=0.85), '', rough, 'A')
    g.link(g.node(unreal.MaterialExpressionConstant, -1100, 650, r=0.12), '', rough, 'B')
    g.link(g.scalar('Wet', 0.0, -1100, 750), '', rough, 'Alpha')
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    W.finish(mat, [])
    log(f'{mat.get_path_name()} built (deferred decal, translucent): {parameter_names(mat)}')
    return mat


def build_muzzle():
    mat = master('M_FX_Muzzle')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = W.Graph(mat)
    local = g.node(unreal.MaterialExpressionLocalPosition, -1900, 0)
    screen = g.node(unreal.MaterialExpressionScreenPosition, -1900, 1000)
    code = g.custom(MUZZLE_CODE, [
        ('Local', local, ''), ('Color', instance_color(g, -1900, 200), ''),
        ('Strength', custom_data(g, 3, 1.0, -1900, 450), ''), ('Shape', custom_data(g, 4, 1.0, -1900, 550), ''),
        ('Param', custom_data(g, 5, 0.0, -1900, 650), ''), ('Seed', custom_data(g, 6, 0.0, -1900, 750), ''),
        ('Clear', custom_data(g, 7, 0.0, -1900, 850), ''),
        ('Normal', g.node(unreal.MaterialExpressionVertexNormalWS, -1900, 920), ''),
        ('ToCamera', g.node(unreal.MaterialExpressionCameraVectorWS, -1900, 960), ''),
        ('Screen', screen, 'ViewportUV'), ('ViewSize', g.node(unreal.MaterialExpressionViewSize, -1900, 1100), ''),
        ('Glow', g.scalar('Glow', 1.0, -1900, 1200), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1000, 300, 'Muzzle')
    g.out(code, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    W.finish(mat, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES])
    log(f'{mat.get_path_name()} built (unlit, additive, two-sided): {parameter_names(mat)}; custom data 0-2 colour, '
        f'3 strength, 4 shape, 5 param, 6 seed, 7 clear')
    return mat


def run(only=None):
    builders = {'shape': build_shape, 'puff': build_puff, 'fire': build_fire, 'splat': build_splat, 'muzzle': build_muzzle}
    for name, build in builders.items():
        if only is None or name in only:
            build()
    log('done')


# Arguments name the masters to build ("py .../build_vfx_materials.py muzzle"); none, all five.
run(set(a.lower() for a in sys.argv[1:]) if len(sys.argv) > 1 else None)
