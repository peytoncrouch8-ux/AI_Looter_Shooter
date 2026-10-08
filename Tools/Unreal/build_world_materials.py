"""Builds the master materials of the textured art style (Docs/TutorialIsland.md) in /Game/Art/Materials/Masters:

  M_World         opaque: BaseColorMap, NormalMap, ORMMap (ambient occlusion, roughness, metallic), Tint, UVScale.
                  The vertex color alpha is baked ambient occlusion (SSAO is off on Medium), and DiffuseAO lets some of
                  it darken the base color too, so contact shading shows in direct light. MossAmount (0 = off) grows
                  moss on upward faces, in MossColor, above the MossThreshold slope. RoughnessOffset (added to the
                  map's) and Specular (0.5 is Unreal's 4%) are for cloth and feathers: black wool reflects so little.
  M_Gun           M_World's maps for gun parts, plus per-gun wear: Wear (0 fresh to 1 battered) comes from each part's
                  custom primitive data 0 (set by UWeaponModelComponent, no material copies per gun): scuffs where the
                  finish is rubbed through to a paler layer, grime in the creases, a duller finish. No moss.
  M_WorldFoliage  masked, two-sided (back faces keep the front's normal), the same maps (opacity from the base
                  color's alpha) plus wind: vertex color R is how far a vertex sways, G offsets its phase;
                  WindStrength (cm), WindSpeed, WindDirection.
  M_Terrain       MacroMap on UV 0 covers the whole island (its alpha picks the detail: 0 grass/soil, 1 rock); the
                  Grass* and Rock* maps tile on UV 1 (meters) and only modulate the macro color's brightness. Faces
                  steeper than SteepStart degrees (fully past SteepFull) take the rock map laid on from the side as
                  their detail, with no detail normal: maps laid on from above smear down a cliff. The face keeps the
                  macro map's color with the rock's light and dark on it (SteepDetail), darkened by SteepTint, some
                  bands darker or redder (SteepVariation), and the rock's layers rise and fall along it (StrataWarp
                  meters over StrataWave), so a long bluff wears the area's colors and no ruled stripes. Its relief comes from the rock's normal map laid on from the same sides, at
                  SteepNormalStrength (0: none, as on the island; SteepNormalGreen flips its green if the cracks read
                  inverted).
  M_SkyClouds     unlit, translucent: painted clouds on a sky dome (Coverage, Softness, Scale, wind), each lit from
                  the sun's side (the sky atmosphere's sun): a warm LitColor where it thins toward the sun, a cool
                  ShadeColor where it thickens, darker cores against the sun and thin edges glowing near it.
  M_Waterfall,    unlit, translucent effects scrolling the macro noise: a falling water sheet's foam streaks, and
  M_Smoke         chimney smoke. Vertex color A is opacity (R foam on the waterfall).
  M_GravewindWisp, unlit, translucent, two-sided: the Gravewind's cards at Gravewind Point (Art/Models/Props/
  M_CanyonFog     Gravewind.py): world-scale noise streaks (the wisps off the Rim) or billows (the fog banks in the
                  canyon) panned along UV1, fading edge-on, against what's behind, near the camera and (wisps) far off;
                  lilac turning warm toward the sun, times MPC_Lighting's CloudTint; a sway from vertex G.
  M_Water         opaque, cheap: a dark color (the sky's reflection does the rest), glossy, procedural ripples on
                  UV 0 in meters.
  M_Gel           lit translucent gel (the slime): Tint, mostly clear facing the eye (Opacity) and denser at grazing
                  edges (EdgeOpacity), a faint inner glow, and a sun highlight worked out in the material (the sky
                  atmosphere's sun direction), so the cheapest translucent lighting does. Casts a solid shadow.
  M_Glass         unlit, translucent: lenses and sight windows. Mostly clear (Opacity) facing the eye, so a sight can
                  be aimed through, tinted (Tint) and brighter and denser toward grazing edges (RimBrightness,
                  EdgeOpacity), which reads as glass without reflections. A screen door's wire mesh sets MeshMaskStrength:
                  its weave (MeshMask, MeshTiling repeats a meter of UV 0) then takes over the opacity.
  M_Backdrop      unlit, opaque, one-sided: the far silhouettes past a grounded area (Art/Levels/area_beyond.py), a flat
                  Tint times Brightness (about what sunlit ground of that color shows) times the lighting state's
                  BackdropTint from MPC_Lighting (white by day; lighting_collection.py makes the collection first),
                  darkening toward the sun's bearing to their shaded sides (Backlit, the sky atmosphere's sun); the
                  height fog and the atmosphere haze them, the farther layers more. Also the cold open's black
                  silhouettes: the gang and Abel (skinned, so it's set for skeletal meshes) and Sexton on the rail.

  M_PaintedPost   post process (before depth of field, so TAA smooths it): Painted Frontier's screen pass, the one part of
                  the look that has to be screen-space: convex edges catch a warm band and creases a violet line, from
                  the normal and depth buffers (the Style Lab's painted_edges.js), then the grade's vibrance and its
                  violet vignette. Only Lvl_TutorialIsland's PaintedPost volume uses it (build_island_painted.py).

Painted Frontier (Style Lab style 3, Docs/Art/StyleLab/Briefs.md): M_World, M_WorldFoliage, M_Terrain and M_SkyClouds
carry a static switch, Painted, off by default, so every instance and level keeps exactly the look above (the switch's
off side is the old graph, with a constant 0 emissive where there was none). An instance that turns it on gets the
lab's painted light (lib/painted_shaders.js paintHook, moved before the lighting: it multiplies the base colour, which
is the same for diffuse light) from its Paint* parameters (group Painted; painted_look.py has the lab's numbers):
  - the texture maps read softer (PaintBlur, PaintNormalBlur: mip bias), the albedo regraded (PaintAdjust: saturation,
    value, contrast, hue; PaintTint), its values grouped into a few soft painted bands (PaintBands), brush-sized
    blotches of value and hue in world space, and three-tone brush strokes on the plane a face mostly faces
    (PaintStrokes), from the macro noise (PaintNoise);
  - tops lifted and warmed, undersides darkened and cooled (PaintWarmTop, PaintCoolUnder), the normal map's relief lit
    from above as painted bevels, dark feet to light heads up each object from its pivot (PaintFeet: the art's pivots
    are at the ground), shade facing away from the sun tinted blue-violet (PaintShade), and the baked occlusion both
    grey (DiffuseAO) and violet (PaintAO, the lab's AO pass: SSAO is off on Medium);
  - emissive: a warm rim (PaintRim, PaintRimPower) and a blue fill in shade (PaintFill), in Unreal's light units
    (PaintEmissiveScale); rougher, less specular (PaintRoughness, PaintSpecular).
M_Terrain's painted ground recolours the macro map's kinds (PaintGrass, PaintDirt, PaintRock by its alpha and hue,
keeping some of its light and dark: PaintGround) with big brushed patches and strokes, warm in sun and cool in shade.
M_SkyClouds' paints the lab's sky on the dome: a three-stop gradient, towering cumulus round the horizon and floating
clusters built from lit puffs with brushed shading, the haze from MPC_Lighting's fog colour, a sun disc and glow.

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
SCREEN_MESH_FILE = 'C:/Dev/AI_Looter_Shooter/Art/Textures/ScreenMesh/T_ScreenMesh.png'
SCREEN_MESH = '/Game/Art/Textures/ScreenMesh/T_ScreenMesh'


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


def clear_graph(mat):
    """Empties a master's graph to build it again in place. The engine's delete marks each node as garbage, which crashes
    the editor (Assertion failed: !IsRooted()) on nodes loaded as it started, when a class's defaults reach the master
    (the Unpaid's model reaches M_Ghost): LooterEditor's ClearMaterialGraph moves those out of the material instead."""
    tools = getattr(unreal, 'LooterMaterialGraphTools', None)
    if tools is None:
        MEL.delete_all_material_expressions(mat)
        return
    removed, moved_out = tools.clear_material_graph(mat)
    if moved_out:
        unreal.log(f'{mat.get_name()}: {moved_out} of its {removed} nodes were loaded as the editor started; moved out of it')


def material(name):
    path = f'{FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mat = unreal.load_asset(path)
        clear_graph(mat)
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
    # The painted side samples the same maps at a softer mip from the same UVs.
    g.uv = uv
    return bc, nrm, orm, vc, ao


def shaded_color(g, bc, ao, tint=None):
    """Base color x Tint (or the given tint node), darkened by DiffuseAO of the occlusion."""
    tint = tint or g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1100, -500)
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


# --- Painted Frontier (the Painted switch; see the docstring) ---

PAINT_GROUP = 'Painted'
# Each Paint* vector packs four numbers (painted_look.py names them); Custom nodes take it whole.
PAINT_SURFACE_VECTORS = ('PaintAdjust', 'PaintTint', 'PaintBands', 'PaintStrokes', 'PaintWarmTop', 'PaintCoolUnder',
                         'PaintFeet', 'PaintShade', 'PaintFill', 'PaintAO', 'PaintRim')

# The lab's paintHook (lib/painted_shaders.js) on the base colour, in metres; Unreal's Z is the lab's Y. Its noise is
# the macro noise: soft fbm, evenly spread from 0 to 1, so the lab's fbm (b1) and value noise (b2) are its deviation
# from 0.5 scaled to their spread (0.45 and 0.7).
PAINT_SURFACE = """const float NUV = 0.0685;
const float3 Wl = float3(0.2126, 0.7152, 0.0722);
const float3 Gy = float3(0.57735, 0.57735, 0.57735);
float3 c = max(Color, 0.0);
// The albedo regraded (the lab's slAdjust): hue turned, saturation about its luma, contrast about 0.18, value, and a
// tint that keeps the luma.
float ha = Adjust.w * 0.0174533;
c = c * cos(ha) + cross(Gy, c) * sin(ha) + Gy * dot(Gy, c) * (1.0 - cos(ha));
float l0 = dot(c, Wl);
c = max(lerp(float3(l0, l0, l0), c, Adjust.x), 0.0);
c = pow(max(c, 1e-6) / 0.18, Adjust.z) * 0.18;
c *= Adjust.y;
c = saturate(lerp(c, TintC.rgb * (dot(c, Wl) / max(dot(TintC.rgb, Wl), 0.001)), TintC.a));
// Brush-sized blotches: value and hue wander at the scale of a brush dab.
float3 Pm = WorldPos * 0.01;
float3 bp = Pm * Bands.w;
float b1 = 0.5 + (Texture2DSample(Noise, NoiseSampler, (bp.xy + bp.z * 0.73 + float2(bp.z * 0.31, 0.0)) * NUV).r - 0.5) * 0.45;
float b2 = 0.5 + (Texture2DSample(Noise, NoiseSampler, (float2(bp.y, bp.z) * 1.9 + bp.x * 0.4 + 9.0) * NUV).r - 0.5) * 0.7;
float3 alb = max(c, 0.004);
// The values regrouped into a few painted bands, soft-edged, their edges wandering with the brush.
float bs = max(Bands.x, 1.0) * 0.25;
float lA = dot(alb, Wl);
float lb = log2(lA + 0.003) * bs + (b1 - 0.5) * 0.6;
float lq = floor(lb) + smoothstep(0.35, 0.65, frac(lb));
float lt = exp2(lq / bs) - 0.003;
alb *= lerp(1.0, lt / max(lA, 0.003), Bands.y * step(0.5, Bands.x));
alb *= 1.0 + (b1 - 0.5) * Bands.z;
// Brush strokes in three tones (cool dark, base, warm light) along a slowly turning direction, on the plane the face
// mostly faces, fading out by Strokes.w metres.
float3 An = abs(N);
float2 sp = An.z > max(An.x, An.y) ? Pm.xy : (An.x > An.y ? Pm.yz : Pm.xz);
float sFade = 1.0 - smoothstep(Strokes.w * 0.4, Strokes.w, length(WorldPos - CameraPos) * 0.01);
float sa = (Texture2DSample(Noise, NoiseSampler, (sp * 0.12 + 1.7) * NUV).r - 0.5) * 2.4;
float2 sq = float2(cos(sa) * sp.x - sin(sa) * sp.y, sin(sa) * sp.x + cos(sa) * sp.y);
float st = Texture2DSample(Noise, NoiseSampler, (sq / Strokes.yz) * NUV).r * 0.65
         + Texture2DSample(Noise, NoiseSampler, (sq / (Strokes.yz * 0.5) + 5.3) * NUV).r * 0.35;
float stroke = (smoothstep(0.36, 0.4, st) + smoothstep(0.62, 0.66, st) - 1.0) * sFade;
alb *= 1.0 + stroke * Strokes.x;
alb *= lerp(float3(1.0, 1.0, 1.0), stroke > 0.0 ? float3(1.04, 1.0, 0.93) : float3(0.95, 0.97, 1.06), abs(stroke));
float hb = (b2 - 0.5) * 0.035 * 6.28318;
alb = alb * cos(hb) + cross(Gy, alb) * sin(hb) + Gy * dot(Gy, alb) * (1.0 - cos(hb));
float lB = dot(alb, Wl);
alb = max(lerp(float3(lB, lB, lB), alb, 1.0 + (b2 - 0.5) * 0.3), 0.0);
// The light painted in: tops lifted and warmed, undersides darkened and cooled.
float3 m = lerp(float3(1.0, 1.0, 1.0), WarmTop.rgb, saturate(N.z) * 0.55)
         * lerp(float3(1.0, 1.0, 1.0), CoolUnder.rgb, saturate(-N.z) * 0.8);
// Painted bevels: the normal map's relief catches the light on its upper edges and goes dark under them.
float3 ng = normalize(Ng);
float3 dn = N - ng * dot(N, ng);
float3 Ls = normalize(SunDir + 1e-5);
m *= 1.0 + clamp((dn.z * 1.8 + dot(dn, Ls) * 0.9) * Feet.z, -0.45, 0.6);
// Dark feet, light heads: up each object from its pivot (the art's pivots are at the ground); more than a metre under
// the pivot (a sky island hangs from its top) there is no ground to darken toward.
float hh = Local.z * 0.01;
float feet = lerp(Feet.x, 1.06, smoothstep(-0.05, Feet.y, hh));
feet = lerp(feet, 1.0, saturate(-hh - 1.0));
m *= lerp(1.0, feet, step(0.5, Feet.w));
// Shade (facing away from the sun) goes blue-violet; cast shadows are the post volume's grade's.
float lit = smoothstep(-0.05, 0.3, dot(N, Ls));
m *= lerp(float3(1.0, 1.0, 1.0), Shade.rgb, (1.0 - lit) * Shade.a);
// Contact shade from the baked occlusion: grey (DiffuseAO, as unpainted) and violet (the lab's AO pass).
float occ = saturate(Occlusion);
m *= lerp(1.0, occ, DiffuseAO);
m *= lerp(AO.rgb, float3(1.0, 1.0, 1.0), lerp(1.0, pow(max(occ, 1e-4), 1.5), AO.a));
return alb * m;"""

# What the lab adds to the lit colour, as emissive: a warm rim (stronger on the sun's side) and the painted sky fill in
# shade, in Unreal's light units (Scale).
PAINT_EMISSIVE = """float3 Ls = normalize(SunDir + 1e-5);
float ndl = dot(N, Ls);
float rim = pow(1.0 - saturate(dot(N, normalize(V))), RimPower) * Rim.a * (0.35 + 0.65 * saturate(ndl));
float lit = smoothstep(-0.05, 0.3, ndl);
float3 fill = Albedo * Fill.rgb * Fill.a * (1.0 - lit) * saturate(Occlusion);
return (Rim.rgb * rim + fill) * Scale;"""

# The painted ground (groundRecolor and paintGroundHook): the macro map's kinds recoloured, keeping some of its light and
# dark, under big brushed patches, strokes, and a warm/cool split by the sun.
PAINT_GROUND = """const float NUV = 0.0685;
const float3 Wl = float3(0.2126, 0.7152, 0.0722);
const float3 Gy = float3(0.57735, 0.57735, 0.57735);
float3 alb = max(Color, 0.004);
float hueDirt = smoothstep(Hue.x, Hue.y, (Macro.r - Macro.g) / max(Macro.g, 0.02));
float wD = max(smoothstep(Split.x, Split.y, Select), hueDirt);
float wR = max(smoothstep(Split.z, Split.w, Select), Steep);
float3 kind = lerp(lerp(GrassC.rgb, DirtC.rgb, wD), RockC.rgb, wR);
float3 col = kind * clamp(pow(max(dot(alb, Wl) / Ground.y, 1e-4), Ground.x), Ground.z, Ground.w);
float3 Pm = WorldPos * 0.01;
float dist = length(WorldPos - CameraPos) * 0.01;
float b1 = 0.5 + (Texture2DSample(Noise, NoiseSampler, (Pm.xy * 0.16) * NUV).r - 0.5) * 0.45;
float b2 = 0.5 + (Texture2DSample(Noise, NoiseSampler, (Pm.xy * 0.9 + 7.0) * NUV).r - 0.5) * 0.45;
float b3 = 0.5 + (Texture2DSample(Noise, NoiseSampler, (Pm.xy * float2(2.8, 0.9) + 3.0) * NUV).r - 0.5) * 0.7;
col *= 1.0 + (b1 - 0.5) * Brush.x + (b2 - 0.5) * Brush.y * (1.0 - smoothstep(30.0, 120.0, dist)) + (b3 - 0.5) * Brush.z;
float hb = (b1 - 0.5) * Brush.w * 6.28318;
col = max(col * cos(hb) + cross(Gy, col) * sin(hb) + Gy * dot(Gy, col) * (1.0 - cos(hb)), 0.0);
float3 An = abs(N);
float2 sp = An.z > max(An.x, An.y) ? Pm.xy : (An.x > An.y ? Pm.yz : Pm.xz);
float sFade = 1.0 - smoothstep(Strokes.w * 0.4, Strokes.w, dist);
float sa = (Texture2DSample(Noise, NoiseSampler, (sp * 0.12 + 1.7) * NUV).r - 0.5) * 2.4;
float2 sq = float2(cos(sa) * sp.x - sin(sa) * sp.y, sin(sa) * sp.x + cos(sa) * sp.y);
float st = Texture2DSample(Noise, NoiseSampler, (sq / Strokes.yz) * NUV).r * 0.65
         + Texture2DSample(Noise, NoiseSampler, (sq / (Strokes.yz * 0.5) + 5.3) * NUV).r * 0.35;
float stroke = (smoothstep(0.36, 0.4, st) + smoothstep(0.62, 0.66, st) - 1.0) * sFade;
col *= 1.0 + stroke * Strokes.x;
col *= lerp(float3(1.0, 1.0, 1.0), stroke > 0.0 ? float3(1.05, 1.0, 0.9) : float3(0.93, 0.96, 1.08), abs(stroke));
float lit = smoothstep(-0.05, 0.3, dot(N, normalize(SunDir + 1e-5)));
return col * lerp(ShadeC.rgb, LitC.rgb, lit);"""


def look():
    """painted_look.py, the Painted Frontier numbers, beside this script. Imported when a master is built, not with the
    module: build_creature_materials.py and build_decal_materials.py run this module's imports without its body. Reloaded,
    as the editor keeps modules between runs."""
    import importlib
    import os
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    import painted_look
    return importlib.reload(painted_look)


def pscalar(g, name, value, x, y):
    node = g.scalar(name, float(value), x, y)
    node.set_editor_property('group', PAINT_GROUP)
    return node


def pvector(g, name, value, x, y):
    values = [float(v) for v in value][:4]
    node = g.vector(name, tuple(values + [1.0] * (4 - len(values))), x, y)
    node.set_editor_property('group', PAINT_GROUP)
    return node


def painted(g, on, off, x, y, on_out='', off_out=''):
    """The Painted switch: on when an instance turns it on, else off (the unpainted graph, unchanged)."""
    switch = g.node(unreal.MaterialExpressionStaticSwitchParameter, x, y, parameter_name='Painted', default_value=False)
    switch.set_editor_property('group', PAINT_GROUP)
    g.link(on, on_out, switch, 'True')
    g.link(off, off_out, switch, 'False')
    return switch


def painted_map(g, name, sampler, default, uv, bias_name, bias, x, y):
    """The same texture parameter read at a softer mip (the lab's blur): only the painted side samples it."""
    mode = getattr(unreal.TextureMipValueMode, 'TMVM_MIP_BIAS')
    t = g.node(unreal.MaterialExpressionTextureSampleParameter2D, x, y, parameter_name=name, sampler_type=sampler,
               texture=unreal.load_asset(default), mip_value_mode=mode)
    g.link(uv, '', t, 'UVs')
    # The pin is "MipBias" in bias mode, which the material graph shortens to "Bias".
    amount = pscalar(g, bias_name, bias, x - 300, y + 120)
    if not any(MEL.connect_material_expressions(amount, '', t, pin) for pin in ('Bias', 'MipBias')):
        raise RuntimeError(f'could not connect {bias_name} to {name}\'s mip bias pin')
    return t


def paint_noise(g, x, y):
    node = g.node(unreal.MaterialExpressionTextureObjectParameter, x, y, parameter_name='PaintNoise',
                  texture=import_mask(MACRO_NOISE_FILE, MACRO_NOISE))
    node.set_editor_property('group', PAINT_GROUP)
    return node


def painted_surface(g, color, color_out, ao, defaults, x, y):
    """The painted base colour and its emissive (PAINT_SURFACE, PAINT_EMISSIVE) from a colour (map x tint) and the
    baked occlusion, with defaults (painted_look.SURFACE_DEFAULTS or FOLIAGE_DEFAULTS) for the Paint* parameters."""
    vectors, scalars = defaults['vectors'], defaults['scalars']
    v = {name: pvector(g, name, vectors[name], x - 700, y + 60 * i) for i, name in enumerate(PAINT_SURFACE_VECTORS)}
    normal = g.node(unreal.MaterialExpressionPixelNormalWS, x - 400, y - 300)
    sun = g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection, x - 400, y - 200)
    base = g.custom(PAINT_SURFACE, [
        ('Color', color, color_out), ('Occlusion', ao, ''),
        ('DiffuseAO', g.scalar('DiffuseAO', 0.4, x - 400, y - 400), ''),
        ('WorldPos', g.node(unreal.MaterialExpressionWorldPosition, x - 400, y - 600), ''),
        ('CameraPos', g.node(unreal.MaterialExpressionCameraPositionWS, x - 400, y - 500), ''),
        ('N', normal, ''), ('Ng', g.node(unreal.MaterialExpressionVertexNormalWS, x - 400, y - 100), ''),
        ('SunDir', sun, ''), ('Local', g.node(unreal.MaterialExpressionLocalPosition, x - 400, y), ''),
        ('Noise', paint_noise(g, x - 400, y + 100), ''),
        ('Adjust', v['PaintAdjust'], 'RGBA'), ('TintC', v['PaintTint'], 'RGBA'), ('Bands', v['PaintBands'], 'RGBA'),
        ('Strokes', v['PaintStrokes'], 'RGBA'), ('WarmTop', v['PaintWarmTop'], 'RGBA'),
        ('CoolUnder', v['PaintCoolUnder'], 'RGBA'), ('Feet', v['PaintFeet'], 'RGBA'), ('Shade', v['PaintShade'], 'RGBA'),
        ('AO', v['PaintAO'], 'RGBA'),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, x, y, 'Painted Frontier surface')
    emissive = g.custom(PAINT_EMISSIVE, [
        ('Albedo', base, ''), ('Occlusion', ao, ''), ('N', normal, ''),
        ('V', g.node(unreal.MaterialExpressionCameraVectorWS, x - 400, y + 300), ''), ('SunDir', sun, ''),
        ('Rim', v['PaintRim'], 'RGBA'), ('RimPower', pscalar(g, 'PaintRimPower', scalars['PaintRimPower'], x - 700, y + 800), ''),
        ('Fill', v['PaintFill'], 'RGBA'),
        ('Scale', pscalar(g, 'PaintEmissiveScale', scalars['PaintEmissiveScale'], x - 700, y + 860), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, x, y + 400, 'Painted Frontier rim and fill')
    return base, emissive


def build_world(orm_default):
    mat = material('M_World')
    g = Graph(mat)
    bc, nrm, orm, vc, ao = textured_inputs(g, orm_default)
    color, rough = moss(g, bc, shaded_color(g, bc, ao), orm)
    # RoughnessOffset raises the map's roughness for cloth and feathers, and Specular (0.5 is Unreal's 4% reflection)
    # lowers how much a dark dielectric reflects: on black wool the bright sky's 4% outweighs the diffuse, and it reads
    # like pale denim. Both default to no change.
    rough = g.custom('return saturate(Rough + Offset);', [
        ('Rough', rough, ''),
        ('Offset', g.scalar('RoughnessOffset', 0.0, 0, -250), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, 300, -400, 'Roughness')
    specular = g.scalar('Specular', 0.5, 300, -150)
    # Painted Frontier (the Painted switch, off by default): the maps read softer, moss as unpainted, then the painted
    # light on the colour; rougher and less specular; a rim and a fill in shade as emissive.
    d = look().SURFACE_DEFAULTS
    bc_p = painted_map(g, 'BaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, g.uv, 'PaintBlur',
                       d['scalars']['PaintBlur'], -1100, 2000)
    nrm_p = painted_map(g, 'NormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, FLAT_NORMAL, g.uv,
                        'PaintNormalBlur', d['scalars']['PaintNormalBlur'], -1100, 2300)
    tinted_p = g.mul(bc_p, 'RGB', g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1100, 1800), '', -800, 1900)
    color_p, rough_p = moss(g, bc_p, tinted_p, orm)
    base_p, emissive_p = painted_surface(g, color_p, '', ao, d, 400, 2400)
    rough_p = g.custom('return saturate(Rough * Scale + Offset);', [
        ('Rough', rough_p, ''), ('Scale', pscalar(g, 'PaintRoughness', d['scalars']['PaintRoughness'], 0, 1600), ''),
        ('Offset', g.scalar('RoughnessOffset', 0.0, 0, 1700), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, 300, 1600, 'Painted roughness')
    specular_p = g.mul(specular, '', pscalar(g, 'PaintSpecular', d['scalars']['PaintSpecular'], 300, 1450), '', 500, 1450)
    g.out(painted(g, base_p, color, 900, -600), '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(painted(g, nrm_p, nrm, 900, -450, 'RGB', 'RGB'), '', unreal.MaterialProperty.MP_NORMAL)
    g.out(painted(g, rough_p, rough, 900, -300), '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(painted(g, specular_p, specular, 900, -150), '', unreal.MaterialProperty.MP_SPECULAR)
    g.out(orm, 'B', unreal.MaterialProperty.MP_METALLIC)
    g.out(ao, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    g.out(painted(g, emissive_p, g.node(unreal.MaterialExpressionConstant3Vector, 600, 0,
                                        constant=unreal.LinearColor(0.0, 0.0, 0.0, 1.0)), 900, 0),
          '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    # Spline meshes too: the jetty's mooring lines bend MI_Canvas along a spline. Without the flag the editor sets it on
    # the fly (and a cooked game draws the lines with the default material).
    finish(mat, [unreal.MaterialUsage.MATUSAGE_NANITE, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,
                 unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH, unreal.MaterialUsage.MATUSAGE_SPLINE_MESH])
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


# A share of the plants (VariationAmount; 0 for all but an area's own instances) wear TintVariation instead of Tint: the
# yellowing crowns of a dry summer. Picked by a hash of the plant's position, as a placed actor's per-instance random is
# always 0 and every one would turn.
FOLIAGE_TINT = """float Pick = frac(sin(dot(floor(Position.xy / 50.0), float2(12.9898, 78.233))) * 43758.5453);
return Pick < Amount ? Variation : Tint;"""


def build_foliage(orm_default):
    mat = material('M_WorldFoliage')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided', True)
    g = Graph(mat)
    bc, nrm, orm, vc, ao = textured_inputs(g, orm_default)
    tint = g.custom(FOLIAGE_TINT, [
        ('Tint', g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1400, -600), ''),
        ('Variation', g.vector('TintVariation', (1.0, 1.0, 1.0, 1.0), -1400, -500), ''),
        ('Amount', g.scalar('VariationAmount', 0.0, -1400, -400), ''),
        ('Position', g.node(unreal.MaterialExpressionObjectPositionWS, -1400, -300), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1100, -500, 'Tint, or TintVariation for a share of the plants')
    color = shaded_color(g, bc, ao, tint)
    # A two-sided material turns the normal around on back faces. Leaf cards and blades carry normals that point out of
    # the crown or up from the ground, which both sides should keep, or half the cards shade dark: undo the turn.
    # The engine multiplies the whole world normal by TwoSidedSign, so the tangent normal is multiplied by it first.
    sign = g.node(unreal.MaterialExpressionTwoSidedSign, -800, 100)
    facing = g.custom('return Normal * Sign;', [
        ('Normal', nrm, 'RGB'),
        ('Sign', sign, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 50, 'Keep back faces on the authored normal')
    # Painted Frontier (the Painted switch, off by default): the colour map read softer (the cut-outs stay on the sharp
    # map's alpha), the painted light on it, rougher and less specular, a rim and a fill in shade as emissive.
    d = look().FOLIAGE_DEFAULTS
    bc_p = painted_map(g, 'BaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, g.uv, 'PaintBlur',
                       d['scalars']['PaintBlur'], -1100, 2000)
    nrm_p = painted_map(g, 'NormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, FLAT_NORMAL, g.uv,
                        'PaintNormalBlur', d['scalars']['PaintNormalBlur'], -1100, 2300)
    base_p, emissive_p = painted_surface(g, g.mul(bc_p, 'RGB', tint, '', -800, 1900), '', ao, d, 400, 2400)
    facing_p = g.custom('return Normal * Sign;', [('Normal', nrm_p, 'RGB'), ('Sign', sign, '')],
                        unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 2300, 'Painted normal, back faces kept')
    rough_p = g.custom('return saturate(Rough * Scale);', [
        ('Rough', orm, 'G'), ('Scale', pscalar(g, 'PaintRoughness', d['scalars']['PaintRoughness'], 0, 1600), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, 300, 1600, 'Painted roughness')
    # Unconnected, Specular is 0.5: the unpainted side keeps exactly that.
    specular = g.node(unreal.MaterialExpressionConstant, 300, -150, r=0.5)
    specular_p = g.mul(specular, '', pscalar(g, 'PaintSpecular', d['scalars']['PaintSpecular'], 300, 1450), '', 500, 1450)
    g.out(painted(g, base_p, color, 900, -600), '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(painted(g, facing_p, facing, 900, -450), '', unreal.MaterialProperty.MP_NORMAL)
    g.out(painted(g, rough_p, orm, 900, -300, '', 'G'), '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(painted(g, specular_p, specular, 900, -150), '', unreal.MaterialProperty.MP_SPECULAR)
    g.out(painted(g, emissive_p, g.node(unreal.MaterialExpressionConstant3Vector, 600, 0,
                                        constant=unreal.LinearColor(0.0, 0.0, 0.0, 1.0)), 900, 0),
          '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
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
// A steep face takes the rock's own color, laid on from the side it faces (the X and Y planes, blended by how far it
// faces each): the macro map and the detail maps are laid on from above, so down a cliff they smear into streaks.
float2 Facing = pow(abs(normalize(Normal).xy), 4.0);
Facing /= max(Facing.x + Facing.y, 1e-4);
float3 Side = RockX * Facing.x + RockY * Facing.y;
// The face takes the macro map's color with the side-laid rock's light and dark on it (SteepDetail), as gentler ground
// takes the detail maps': the rock map's own pale cream at full strength stood out of the area's palette, and its
// layers, every four meters, ruled stripes down a long bluff.
float SideLuma = dot(Side, float3(0.299, 0.587, 0.114));
// SteepTint (white unless an area sets it) darkens and weathers the faces under the dusty slopes, and SteepVariation
// (0 unless an area sets it) makes some bands darker and others redder, by a large noise read mostly by height.
float B = (Band - 0.5) * 2.0 * SteepVariation;
float3 Banding = (1.0 - 0.5 * max(-B, 0.0)) * lerp(float3(1.0, 1.0, 1.0), float3(1.1, 0.9, 0.78), max(B, 0.0));
Color = lerp(Color, Macro * SteepTint * Banding * lerp(1.0, SideLuma / max(RockMean, 0.05), SteepDetail), Steep);
return Color * lerp(1.0, Occlusion, DiffuseAO);"""
# The rock's relief on a steep face, from the same two side-laid projections as its color (the rock normal map sampled
# with the X- and the Y-side UVs): each sample turned into the world (its red along the projection's U axis, its green up
# the face by Green's sign, its blue out of the face), blended by the same side weights. The caller turns it back into
# the tangent frame.
SIDE_NORMAL_CODE = """float3 N = normalize(Normal);
float2 Facing = pow(abs(N.xy), 4.0);
Facing /= max(Facing.x + Facing.y, 1e-4);
float SX = N.x >= 0.0 ? 1.0 : -1.0;
float SY = N.y >= 0.0 ? 1.0 : -1.0;
float3 FromX = float3(NX.z * SX, NX.x, NX.y * Green);
float3 FromY = float3(NY.x, NY.z * SY, NY.y * Green);
return normalize(FromX * Facing.x + FromY * Facing.y);"""
# The rock map laid on from one side (A: world X or Y, across the face) in world meters times Scale, its layers lifted
# and dropped a little along the face (two long, low waves, and a slight lean), so they never run dead level for long.
# StrataWarp (meters; 0 on the island) bends them further by the macro noise over StrataWave meters, so the layers
# undulate, pinch and thicken instead of ruling a whole face.
SIDE_UV_CODE = """float W = sin(A * 0.0011 + 1.3) * 0.14 + sin(A * 0.0037 + P.z * 0.0007) * 0.05 + A * 0.00002;
float N = Texture2DSampleLevel(Noise, NoiseSampler, float2(A, P.z * 2.0) * 0.01 / max(Wave, 1.0), 0).r;
W += (N - 0.5) * 2.0 * Warp * Scale;
return float2(A * Scale * 0.01, -P.z * Scale * 0.01 + W);"""
# How steep the ground is, from 0 (gentler than SteepStart degrees) to 1 (steeper than SteepFull).
STEEP_CODE = 'return 1.0 - smoothstep(cos(radians(Full)), cos(radians(Start)), normalize(Normal).z);'


def build_terrain():
    mat = material('M_Terrain')
    g = Graph(mat)
    macro_uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1800, -300, coordinate_index=0)
    macro = g.texture('MacroMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, macro_uv, -1400, -300)
    detail_coords = g.node(unreal.MaterialExpressionTextureCoordinate, -2000, 200, coordinate_index=1)
    grass_uv = g.mul(detail_coords, '', g.scalar('GrassScale', 0.5, -2000, 320), '', -1800, 200)
    rock_scale = g.scalar('RockScale', 0.25, -2000, 520)
    rock_uv = g.mul(detail_coords, '', rock_scale, '', -1800, 450)
    grass = g.texture('GrassBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, grass_uv, -1400, 0)
    grass_n = g.texture('GrassNormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, FLAT_NORMAL, grass_uv, -1400, 250)
    rock = g.texture('RockBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, rock_uv, -1400, 500)
    rock_n = g.texture('RockNormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, FLAT_NORMAL, rock_uv, -1400, 750)
    # Steep faces: the rock map again, laid on from the X and the Y side (world meters x RockScale, as on UV 1).
    world = g.node(unreal.MaterialExpressionWorldPosition, -2400, 1300)
    vertex_normal = g.node(unreal.MaterialExpressionVertexNormalWS, -2400, 1500)
    noise = g.node(unreal.MaterialExpressionTextureObjectParameter, -2400, 1100, parameter_name='StrataNoise',
                   texture=import_mask(MACRO_NOISE_FILE, MACRO_NOISE))
    warp, wave = g.scalar('StrataWarp', 0.0, -2400, 1000), g.scalar('StrataWave', 45.0, -2400, 900)
    side_uvs = [g.custom(f'float A = {axis};\n' + SIDE_UV_CODE,
                         [('P', world, ''), ('Scale', rock_scale, ''), ('Noise', noise, ''), ('Warp', warp, ''),
                          ('Wave', wave, '')],
                         unreal.CustomMaterialOutputType.CMOT_FLOAT2, -2000, y, f'Rock UV from the {name} side')
                for axis, name, y in (('P.y', 'X', 1300), ('P.x', 'Y', 1450))]
    # The bands' darkness: the macro noise over about 120 m along the ground and 25 m up, so it changes layer to layer.
    band = g.custom('return Texture2DSampleLevel(Noise, NoiseSampler, float2((P.x + P.y) / 12000.0, P.z / 2500.0), 0).r;',
                    [('P', world, ''), ('Noise', noise, '')],
                    unreal.CustomMaterialOutputType.CMOT_FLOAT1, -2000, 1600, 'Band darkness noise')
    rock_x = g.texture('RockBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, side_uvs[0], -1400, 1300)
    rock_y = g.texture('RockBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, side_uvs[1], -1400, 1550)
    steep = g.custom(STEEP_CODE, [
        ('Normal', vertex_normal, ''),
        ('Start', g.scalar('SteepStart', 50.0, -2000, 1650), ''),
        ('Full', g.scalar('SteepFull', 65.0, -2000, 1750), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1400, 1800, 'Steepness')
    vc = g.node(unreal.MaterialExpressionVertexColor, -1400, 1000)
    color_inputs = [
        ('Macro', macro, 'RGB'), ('Grass', grass, 'RGB'), ('Rock', rock, 'RGB'), ('Select', macro, 'A'),
        ('GrassMean', g.scalar('GrassMeanLuma', 0.3, -1000, 900), ''),
        ('RockMean', g.scalar('RockMeanLuma', 0.35, -1000, 1000), ''),
        ('Strength', g.scalar('DetailStrength', 0.6, -1000, 1100), ''),
        ('Occlusion', vc, 'A'),
        ('DiffuseAO', g.scalar('DiffuseAO', 0.45, -1000, 1200), ''),
        ('RockX', rock_x, 'RGB'), ('RockY', rock_y, 'RGB'), ('Normal', vertex_normal, ''), ('Steep', steep, ''),
        ('SteepDetail', g.scalar('SteepDetail', 0.75, -1000, 1300), ''),
        ('SteepTint', g.vector('SteepTint', (1.0, 1.0, 1.0, 1.0), -1000, 1400), ''),
        ('Band', band, ''), ('SteepVariation', g.scalar('SteepVariation', 0.0, -1000, 1500), ''),
    ]
    color = g.custom(TERRAIN_CODE, color_inputs, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -600, 0, 'TerrainColor')
    # Painted Frontier (the Painted switch, off by default): the detail maps read softer, then the painted ground.
    d = look().GROUND_DEFAULTS
    grass_p = painted_map(g, 'GrassBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, grass_uv,
                          'PaintBlur', d['scalars']['PaintBlur'], -1400, 2600)
    rock_p = painted_map(g, 'RockBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, rock_uv,
                         'PaintBlur', d['scalars']['PaintBlur'], -1400, 2900)
    swapped = {'Grass': grass_p, 'Rock': rock_p}
    color_p = g.custom(TERRAIN_CODE, [(name, swapped.get(name, source), out) for name, source, out in color_inputs],
                       unreal.CustomMaterialOutputType.CMOT_FLOAT3, -600, 2600, 'TerrainColor, painted maps')
    v = {name: pvector(g, name, value, -1000, 3200 + 60 * i) for i, (name, value) in enumerate(d['vectors'].items())}
    ground_p = g.custom(PAINT_GROUND, [
        ('Color', color_p, ''), ('Macro', macro, 'RGB'), ('Select', macro, 'A'), ('Steep', steep, ''),
        ('WorldPos', world, ''), ('CameraPos', g.node(unreal.MaterialExpressionCameraPositionWS, -600, 3000), ''),
        ('N', g.node(unreal.MaterialExpressionPixelNormalWS, -600, 3100), ''),
        ('SunDir', g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection, -600, 3200), ''),
        ('Noise', paint_noise(g, -600, 3300), ''),
        ('GrassC', v['PaintGrass'], 'RGBA'), ('DirtC', v['PaintDirt'], 'RGBA'), ('RockC', v['PaintRock'], 'RGBA'),
        ('Ground', v['PaintGround'], 'RGBA'), ('Split', v['PaintGroundSplit'], 'RGBA'),
        ('Hue', v['PaintGroundHue'], 'RGBA'), ('Brush', v['PaintGroundBrush'], 'RGBA'),
        ('Strokes', v['PaintStrokes'], 'RGBA'), ('LitC', v['PaintGroundLit'], 'RGBA'),
        ('ShadeC', v['PaintGroundShade'], 'RGBA'),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -300, 2600, 'Painted Frontier ground')
    g.out(painted(g, ground_p, color, 0, -200), '', unreal.MaterialProperty.MP_BASE_COLOR)
    normal = g.node(unreal.MaterialExpressionLinearInterpolate, -900, 400)
    g.link(grass_n, 'RGB', normal, 'A')
    g.link(rock_n, 'RGB', normal, 'B')
    g.link(macro, 'A', normal, 'Alpha')
    flatten = g.node(unreal.MaterialExpressionLinearInterpolate, -600, 400)
    g.link(g.node(unreal.MaterialExpressionConstant3Vector, -900, 600, constant=unreal.LinearColor(0.0, 0.0, 1.0, 1.0)), '', flatten, 'A')
    g.link(normal, '', flatten, 'B')
    # A steep face's detail normal would be the stretched one from above: it goes flat there, and the side-laid color
    # carries the rock's cracks.
    strength = g.custom('return Strength * (1.0 - Steep);', [
        ('Strength', g.scalar('NormalStrength', 0.8, -900, 700), ''), ('Steep', steep, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -900, 800, 'Normal strength')
    g.link(strength, '', flatten, 'Alpha')
    # The steep faces' own relief, at SteepNormalStrength (0 unless an area sets it, so the island's faces stay flat).
    rock_nx = g.texture('RockNormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, FLAT_NORMAL, side_uvs[0], -1400, 1900)
    rock_ny = g.texture('RockNormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, FLAT_NORMAL, side_uvs[1], -1400, 2150)
    side_world = g.custom(SIDE_NORMAL_CODE, [
        ('NX', rock_nx, 'RGB'), ('NY', rock_ny, 'RGB'), ('Normal', vertex_normal, ''),
        ('Green', g.scalar('SteepNormalGreen', 1.0, -1000, 2300), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1000, 2000, 'Side-laid rock normal (world)')
    side_tangent = g.node(unreal.MaterialExpressionTransform, -700, 2000,
                          transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD,
                          transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
    g.link(side_world, '', side_tangent, '')
    relief = g.node(unreal.MaterialExpressionLinearInterpolate, -400, 600)
    g.link(flatten, '', relief, 'A')
    g.link(side_tangent, '', relief, 'B')
    g.link(g.custom('return Steep * Strength;', [
        ('Steep', steep, ''), ('Strength', g.scalar('SteepNormalStrength', 0.0, -700, 2200), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -700, 2300, 'Steep relief'), '', relief, 'Alpha')
    g.out(relief, '', unreal.MaterialProperty.MP_NORMAL)
    rough = g.node(unreal.MaterialExpressionLinearInterpolate, -600, 700, const_a=0.92, const_b=0.82)
    g.link(g.custom('return max(Select, Steep);', [('Select', macro, 'A'), ('Steep', steep, '')],
                    unreal.CustomMaterialOutputType.CMOT_FLOAT1, -900, 950, 'Rock amount'), '', rough, 'Alpha')
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
CLOUD_COLOR = CLOUD_DENSITY + """// Each cloud lit from the sun's side, like a bump map of its density: where it thins toward the sun the face is lit
// (the warm LitColor), where it thickens toward the sun it's in shade (the cool ShadeColor), and thick cores darken.
// The fine octave is the same at both points, so it drops out of the difference.
float2 Q = P + normalize(SunDirection.xy + 1e-4) * ShadowStep;
float Ahead = Texture2DSample(Noise, NoiseSampler, Q + Drift).r * 0.6
            + Texture2DSample(Noise, NoiseSampler, Q * 2.31 + float2(0.37, 0.71) + Drift * 1.6).r * 0.3
            + C * 0.1 - Coverage;
float Lit = saturate(0.5 + (Density - Ahead) * Contrast) * (1.0 - 0.5 * saturate(Density / (Softness * 3.0)));
// Seen against the sun a cloud shows its shaded side: the thick parts darken (Backlight), while thin cloud glows with
// the light scattered forward through it (SunGlow), its rims brightest close to the sun.
float Toward = saturate(dot(D, normalize(SunDirection + 1e-5)));
float Thin = 1.0 - saturate(Density / (Softness * 2.0));
Lit *= 1.0 - Backlight * Toward * Toward * (1.0 - Thin);
return lerp(ShadeColor, LitColor, Lit) + LitColor * (SunGlow * pow(Toward, 8.0) * Thin);"""
CLOUD_OPACITY = CLOUD_DENSITY + """// Soft edges, and the layer fades into the haze toward the horizon.
return saturate(Density / Softness) * saturate((Up - 0.04) * 3.0) * Opacity;"""


# Painted Frontier's sky (the Style Lab's makeToonSky, lib/anime_sky.js, as 03_painted.js sets it): a three-stop
# gradient; towering cumulus standing round the horizon and floating fair-weather clusters, each built from lit spheres
# ("puffs", the front-most one under a direction wins) so every cloud has a rounded form the sun models, softly shaded
# and brushed; the haze at the horizon in the fog's colour; the sun's glow and disc. In azimuth and elevation (radians),
# Unreal's Z up. Returns the colour and the opacity: Misc2.z 1 paints the whole sky over the atmosphere, 0 only clouds.
PAINT_SKY = """const float SkyPi = 3.14159265;
const float NUV = 0.0685;
float3 d = normalize(-CameraVector);
float3 S = normalize(SunDir + 1e-5);
float h = d.z;
float t = pow(smoothstep(0.0, 1.0, max(h, 0.0)), Grad.y);
float mA = Grad.x;
float3 col = t < mA ? lerp(Horizon.rgb, Mid.rgb, smoothstep(0.0, 1.0, t / max(mA, 0.001)))
                    : lerp(Mid.rgb, Zenith.rgb, smoothstep(0.0, 1.0, (t - mA) / max(1.0 - mA, 0.001)));
col = lerp(col, Ground.rgb, smoothstep(0.0, -0.06, h));
float cs = dot(d, S);
float2 p = float2(atan2(d.y, d.x), asin(clamp(h, -1.0, 1.0)));
p.x += Time * Misc.x;
// The edges a little lumpy (the domain warped), read at a fixed mip: the azimuth jumps round the back of the sky.
float2 wq = p * float2(9.0, 14.0);
p += (float2(Texture2DSampleLevel(Noise, NoiseSampler, wq * NUV, 1.0).r,
             Texture2DSampleLevel(Noise, NoiseSampler, (wq + 5.2) * NUV, 1.0).r) - 0.5) * 0.45 * Look.z;
float3 gN = float3(0.0, 0.0, 1.0);
float3 gM = gN;
float3 gMass = gN;
float gZ = -1e9;
float gCov = 0.0;
float gLvl = 1.0;
float soft = Misc.w;
// Towers round the horizon: a broad shelf of puffs at the base, a heaped body and a cauliflower crown.
if (Tow.x > 0.5 && p.y < 0.75)
{
    int NT = (int)(Tow.x + 0.5);
    [loop] for (int i = 0; i < 12; i++)
    {
        if (i >= NT) break;
        float fi = float(i) * 1.731 + Tow.w * 17.0;
        float ca = (float(i) + 0.15 + 0.7 * frac(sin((fi + 0.1) * 127.1 + 311.7) * 43758.5453)) * 2.0 * SkyPi / float(NT);
        float TW = (0.10 + 0.12 * frac(sin((fi + 0.2) * 127.1 + 311.7) * 43758.5453)) * Tow.z;
        float TH = (0.06 + 0.17 * frac(sin((fi + 0.3) * 127.1 + 311.7) * 43758.5453)) * Tow.y;
        float dx = p.x - ca;
        dx -= 2.0 * SkyPi * floor((dx + SkyPi) / (2.0 * SkyPi));
        if (abs(dx) > TW * 1.9 || p.y > TH + TW * 0.8) continue;
        float2 mq = float2(dx / (TW * 1.3), (p.y - TH * 0.35) / (TH * 0.75 + TW * 0.5));
        gMass = normalize(float3(mq, sqrt(max(1.0 - dot(mq, mq), 0.04))));
        [loop] for (int k = 0; k < 20; k++)
        {
            float fk = float(k);
            float r1 = frac(sin((fi * 7.0 + fk * 1.3) * 127.1 + 311.7) * 43758.5453);
            float r2 = frac(sin((fi * 5.0 + fk * 2.7) * 127.1 + 311.7) * 43758.5453);
            float r3 = frac(sin((fi * 3.0 + fk * 4.1) * 127.1 + 311.7) * 43758.5453);
            float tt = 0.06 * r3;
            float px = (fk / 5.0 - 0.5) * 1.8 * TW + (r1 - 0.5) * TW * 0.25;
            float pr = TW * (0.28 + 0.14 * r2);
            if (k >= 6)
            {
                tt = pow(max((fk - 6.0) / 13.0, 1e-5), 0.8);
                px = (r1 - 0.5) * 2.0 * TW * (0.9 - 0.45 * tt);
                pr = TW * (0.36 - 0.10 * tt) * (0.65 + 0.7 * r2);
            }
            float py = -0.03 + TH * pow(max(tt, 1e-5), 0.85);
            float2 q = float2(dx - px, p.y - py);
            float dd = length(q) / pr;
            float cov = (1.0 - smoothstep(1.0 - soft, 1.0 + soft * 0.4, dd)) * smoothstep(-0.003, 0.003, p.y + 0.03);
            if (cov > 0.0)
            {
                float z = sqrt(max(1.0 - min(dd * dd, 1.0), 0.0)) * pr + r3 * pr * 0.6 + tt * TW * 0.12;
                gCov = max(gCov, cov);
                if (z > gZ) { gZ = z; gN = float3(q / pr, sqrt(max(1.0 - min(dd * dd, 1.0), 0.0))); gLvl = tt; gM = gMass; }
            }
        }
    }
}
// Floating clusters on a grid of cells over the sky, each a row of puffs on a flat floor.
if (Puff.x > 0.0 && p.y > Puff.z - 0.2)
{
    const float NC = 15.0;
    const float ch = 0.17;
    float cw = 2.0 * SkyPi / NC;
    float2 gi = floor(float2(p.x / cw, (p.y - Puff.z) / ch));
    [loop] for (int j = -1; j <= 1; j++)
    {
        [loop] for (int i2 = -1; i2 <= 1; i2++)
        {
            float2 cell = gi + float2(i2, j);
            float cy0 = Puff.z + cell.y * ch;
            if (cell.y < 0.0 || cy0 > Puff.w) continue;
            float2 key = float2(cell.x - NC * floor(cell.x / NC), cell.y) + Tow.w * 3.7;
            if (frac(sin(dot(key, float2(12.9898, 78.233))) * 43758.5453) > Puff.x) continue;
            float2 cc0 = float2((cell.x + 0.2 + 0.6 * frac(sin(dot(key + 3.1, float2(12.9898, 78.233))) * 43758.5453)) * cw,
                                cy0 + (0.25 + 0.5 * frac(sin(dot(key + 5.3, float2(12.9898, 78.233))) * 43758.5453)) * ch);
            float sz = Puff.y * (0.45 + 0.75 * sin(cc0.y)) * (0.7 + 0.6 * frac(sin(dot(key + 7.9, float2(12.9898, 78.233))) * 43758.5453));
            float dxc = p.x - cc0.x;
            dxc -= 2.0 * SkyPi * floor((dxc + SkyPi) / (2.0 * SkyPi));
            float2 q0 = float2(dxc * cos(cc0.y), p.y - cc0.y);
            if (abs(q0.x) > sz * 2.2 || abs(q0.y) > sz * 1.3) continue;
            float2 mq = float2(q0.x / (sz * 1.6), (q0.y - sz * 0.1) / (sz * 0.9));
            gMass = normalize(float3(mq, sqrt(max(1.0 - dot(mq, mq), 0.04))));
            float floorY = -sz * 0.32;
            [loop] for (int k2 = 0; k2 < 7; k2++)
            {
                float fk = float(k2);
                float r1 = frac(sin(dot(key + fk * 1.7, float2(12.9898, 78.233))) * 43758.5453);
                float r2 = frac(sin(dot(key + fk * 2.9 + 1.0, float2(12.9898, 78.233))) * 43758.5453);
                float r3 = frac(sin(dot(key + fk * 4.3 + 2.0, float2(12.9898, 78.233))) * 43758.5453);
                float px = (fk / 6.0 - 0.5) * 2.4 * sz + (r1 - 0.5) * sz * 0.4;
                float pr = sz * (0.38 + 0.32 * r2) * (1.0 - 0.45 * abs(fk / 6.0 - 0.5) * 2.0);
                float py = floorY + pr * (0.55 + 0.35 * r3);
                float2 q = q0 - float2(px, py);
                float dd = length(q) / pr;
                float cov = (1.0 - smoothstep(1.0 - soft, 1.0 + soft * 0.4, dd)) * smoothstep(-0.003, 0.003, q0.y - floorY);
                if (cov > 0.0)
                {
                    float nz = sqrt(max(1.0 - min(dd * dd, 1.0), 0.0));
                    float z = nz * pr + r3 * pr * 0.4;
                    gCov = max(gCov, cov);
                    if (z > gZ) { gZ = z; gN = float3(q / pr, nz); gLvl = 0.55 + 0.45 * (py - floorY) / sz; gM = gMass; }
                }
            }
        }
    }
}
float cloudCov = (h > -0.06) ? gCov : 0.0;
float3 cc = float3(0.0, 0.0, 0.0);
if (cloudCov > 0.0)
{
    // The sun's side lit, biased to the tops (light from the sky above), so the forms read as heaped domes.
    float3 T = normalize(float3(-d.y, d.x, 0.0) + 1e-5);
    float3 U = normalize(float3(0.0, 0.0, 1.0) - d * h + 1e-5);
    float3 nl = normalize(lerp(gN, gM, Misc2.y));
    float3 Nc = normalize(T * nl.x + U * nl.y - d * nl.z);
    float lw = (lerp(dot(Nc, S), Nc.z * 0.9 + 0.1, Misc2.x) + Cel.z) / (1.0 + Cel.z);
    float cel = smoothstep(Cel.x - Cel.y, Cel.x + Cel.y, lw);
    cc = lerp(CloudShade.rgb, CloudLit.rgb, cel);
    // lower in a tower, deeper in its own shade; a bright rim with the sun behind, a warm bleed near it
    cc = lerp(lerp(CloudDark.rgb, cc, 0.35), cc, lerp(1.0, smoothstep(0.0, 0.55, gLvl), Misc.y));
    cc += CloudRim.rgb * pow(1.0 - saturate(gN.z), 3.0) * Cel.w * (0.25 + 1.5 * pow(max(cs, 0.0), 4.0));
    cc += SunColor.rgb * pow(max(cs, 0.0), 6.0) * Misc.z * (1.0 - cel * 0.5);
    // brush strokes: long dabs following the cloud's curve
    float ba = atan2(gN.y, gN.x);
    float2 bp = p * 60.0;
    float b = Texture2DSampleLevel(Noise, NoiseSampler, float2(bp.x * cos(ba) + bp.y * sin(ba), (-bp.x * sin(ba) + bp.y * cos(ba)) * 3.0) * NUV, 1.0).r;
    cc *= 1.0 + (b - 0.5) * 0.45 * Look.y;
}
col = lerp(col, cc, cloudCov);
// the haze at the horizon is the fog's own colour, so the far edge melts into it
col = lerp(col, Haze, Grad.z * (1.0 - smoothstep(-0.02, 0.2, h)));
col *= Grad.w;
float sunGlow = pow(max(cs, 0.0), 8.0) * 0.12 + pow(max(cs, 0.0), 80.0) * 0.35 + pow(max(cs, 0.0), 1200.0) * 1.5;
float disc = smoothstep(cos(SunP.x) - 0.00004, cos(SunP.x) + 0.00004, cs) * (1.0 - cloudCov);
float3 sun = SunColor.rgb * (sunGlow * SunP.z * (1.0 - cloudCov * 0.6) + disc * SunP.y);
float3 outColor = Misc2.z > 0.5 ? col + sun : cc * Grad.w + sun * Misc2.z;
return float4(outColor, saturate(max(Misc2.z, cloudCov)));"""

PAINT_SKY_VECTORS = ('PaintZenith', 'PaintMid', 'PaintHorizon', 'PaintGround', 'PaintSunColor', 'PaintCloudLit',
                     'PaintCloudShade', 'PaintCloudDark', 'PaintCloudRim', 'PaintGrad', 'PaintSun', 'PaintTowers',
                     'PaintFloaters', 'PaintCel', 'PaintLook', 'PaintMisc', 'PaintMisc2')


def import_mask(file, path, compression=unreal.TextureCompressionSettings.TC_MASKS):
    """A linear single-channel texture from the library (the macro noise for the clouds; the screen door's wire weave, kept
    uncompressed as Grayscale, since block compression smears a weave only a few pixels wide)."""
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', file)
        task.set_editor_property('destination_path', path.rsplit('/', 1)[0])
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(path)
    texture.set_editor_property('srgb', False)
    texture.set_editor_property('compression_settings', compression)
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
    # The sun's direction comes from the sky atmosphere (its sun light), so the clouds turn with a lighting state's sun.
    color = g.custom(CLOUD_COLOR, inputs + [
        ('SunDirection', g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection, -1200, 750), ''),
        ('LitColor', g.vector('LitColor', (3.0, 2.35, 1.6, 1.0), -900, 750), ''),
        ('ShadeColor', g.vector('ShadeColor', (0.9, 1.02, 1.32, 1.0), -900, 850), ''),
        ('ShadowStep', g.scalar('ShadowStep', 0.03, -1200, 850), ''),
        ('Contrast', g.scalar('Contrast', 6.0, -1200, 950), ''),
        ('SunGlow', g.scalar('SunGlow', 0.6, -1200, 1050), ''),
        ('Backlight', g.scalar('Backlight', 0.6, -1200, 1150), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, -100, 'Cloud color')
    opacity = g.custom(CLOUD_OPACITY, inputs + [
        ('Opacity', g.scalar('Opacity', 0.9, -900, 950), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -400, 200, 'Cloud opacity')
    # The lighting state's cloud tint (white by day) dims and warms them at dusk.
    tint = lighting_tint(g, 'CloudTint', -900, 1050)
    emissive = g.mul(color, '', tint, '', -100, -100)
    # Painted Frontier (the Painted switch, off by default): the lab's painted sky on the dome (PAINT_SKY).
    d = look().SKY_DEFAULTS
    v = {name: pvector(g, name, d['vectors'][name], -1600, 1400 + 60 * i) for i, name in enumerate(PAINT_SKY_VECTORS)}
    haze = g.mul(lighting_tint(g, 'FogInscattering', -1600, 2500), '',
                 pscalar(g, 'PaintHazeBrightness', d['scalars']['PaintHazeBrightness'], -1600, 2600), '', -1200, 2500)
    sky = g.custom(PAINT_SKY, [
        ('CameraVector', inputs[0][1], ''), ('SunDir', g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection,
                                                              -1200, 1300), ''),
        ('Time', inputs[2][1], ''), ('Noise', noise, ''), ('Haze', haze, ''),
        ('Zenith', v['PaintZenith'], 'RGBA'), ('Mid', v['PaintMid'], 'RGBA'), ('Horizon', v['PaintHorizon'], 'RGBA'),
        ('Ground', v['PaintGround'], 'RGBA'), ('SunColor', v['PaintSunColor'], 'RGBA'),
        ('CloudLit', v['PaintCloudLit'], 'RGBA'), ('CloudShade', v['PaintCloudShade'], 'RGBA'),
        ('CloudDark', v['PaintCloudDark'], 'RGBA'), ('CloudRim', v['PaintCloudRim'], 'RGBA'),
        ('Grad', v['PaintGrad'], 'RGBA'), ('SunP', v['PaintSun'], 'RGBA'), ('Tow', v['PaintTowers'], 'RGBA'),
        ('Puff', v['PaintFloaters'], 'RGBA'), ('Cel', v['PaintCel'], 'RGBA'), ('Look', v['PaintLook'], 'RGBA'),
        ('Misc', v['PaintMisc'], 'RGBA'), ('Misc2', v['PaintMisc2'], 'RGBA'),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT4, -800, 1400, 'Painted Frontier sky')
    sky_rgb = g.node(unreal.MaterialExpressionComponentMask, -500, 1400, r=True, g=True, b=True, a=False)
    g.link(sky, '', sky_rgb, '')
    sky_alpha = g.node(unreal.MaterialExpressionComponentMask, -500, 1550, r=False, g=False, b=False, a=True)
    g.link(sky, '', sky_alpha, '')
    g.out(painted(g, g.mul(sky_rgb, '', tint, '', -300, 1400), emissive, 100, -100), '',
          unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(painted(g, sky_alpha, opacity, 100, 200), '', unreal.MaterialProperty.MP_OPACITY)
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
    # A wire mesh (a screen door's) instead of a pane: the weave (R: 1 wire, 0 gap) tiled MeshTiling times a meter of UV 0
    # takes over the opacity by MeshMaskStrength. 0, as every lens and window has it, leaves the glass as it was.
    import_mask(SCREEN_MESH_FILE, SCREEN_MESH, unreal.TextureCompressionSettings.TC_GRAYSCALE)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1500, 600, coordinate_index=0)
    weave = g.texture('MeshMask', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE, SCREEN_MESH,
                      g.mul(coords, '', g.scalar('MeshTiling', 32.0, -1500, 700), '', -1250, 650), -1000, 600)
    meshed = g.node(unreal.MaterialExpressionLinearInterpolate, -350, 400)
    g.link(opacity, '', meshed, 'A')
    g.link(weave, 'R', meshed, 'B')
    g.link(g.scalar('MeshMaskStrength', 0.0, -600, 700), '', meshed, 'Alpha')
    g.out(meshed, '', unreal.MaterialProperty.MP_OPACITY)
    finish(mat, [])
    return mat


BACKDROP_COLOR = """// Seen against the sun a range shows the sides it doesn't reach: toward the sun's bearing the layers
// darken and cool (Backlit at the sun itself), so the haze in front of them, thicker on the farther ones, sets them apart.
float3 D = -CameraVector;
float Toward = saturate(dot(D, normalize(SunDirection + 1e-5)));
float3 Lit = Color * lerp(float3(1.0, 1.0, 1.0), Backlit, Toward * Toward) * BackdropTint;
// Land fading into air: the further a point lies below its layer's skyline (UV 1's U, in meters), the more air hangs in
// front of it, so each range grades from hazy at its foot to crisp on the skyline. The haze takes the lighting state's
// own fog color (MPC_Lighting) but not its glow toward the sun, so at dusk the ranges against the sun stay silhouettes.
float3 Air = FogInscattering * HazeBrightness;
return lerp(Lit, Air, MaxHaze * saturate(Depth.x / HazeDepth));"""


def build_backdrop():
    """M_Backdrop: a few cheap draws for the ranges and plains kilometers out. Unlit, so their color doesn't depend on
    how the low sun happens to strike them; opaque and one-sided, since they're only ever seen from inside. Being unlit
    they never see the sun go down either, so the lighting state's tint (MPC_Lighting's BackdropTint, white by day)
    multiplies every layer's own: dusk darkens and warms the ranges without touching the instances. Toward the sun
    (the sky atmosphere's) they darken to their shaded sides, and each grades into the haze toward its foot (UV 1:
    meters below the skyline, which Art/Levels/area_beyond.py writes)."""
    mat = material('M_Backdrop')
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    g = Graph(mat)
    # A lit surface of albedo A shows about 2.5 A in the island's sun and sky (the unlit clouds' white is about 3).
    color = g.mul(g.vector('Tint', (0.11, 0.15, 0.11, 1.0), -600, -100), '', g.scalar('Brightness', 2.5, -600, 50), '',
                  -300, -50)
    shaded = g.custom(BACKDROP_COLOR, [
        ('Color', color, ''),
        ('CameraVector', g.node(unreal.MaterialExpressionCameraVectorWS, -900, 350), ''),
        ('SunDirection', g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection, -900, 450), ''),
        ('Backlit', g.vector('Backlit', (0.22, 0.27, 0.38, 1.0), -900, 550), ''),
        ('BackdropTint', lighting_tint(g, 'BackdropTint', -1200, 200), ''),
        ('FogInscattering', lighting_tint(g, 'FogInscattering', -1200, 650), ''),
        ('Depth', g.node(unreal.MaterialExpressionTextureCoordinate, -900, 950, coordinate_index=1), ''),
        # About what a view sees of the nearest layer below its skyline (meters): the grade runs over that.
        ('HazeDepth', g.scalar('HazeDepth', 220.0, -900, 1050), ''),
        ('MaxHaze', g.scalar('MaxHaze', 0.35, -900, 1150), ''),
        ('HazeBrightness', g.scalar('HazeBrightness', 1.2, -900, 1250), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -300, 300, 'Backdrop color')
    g.out(shaded, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    # The cold open draws its silhouettes with it too (AColdOpenCast: the gang as black mannequins against the sunset),
    # and they're skinned meshes.
    finish(mat, [unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH])
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
    """M_Smoke: soft chimney smoke drifting up its cards. A light grey rather than white, and fainter: against a deep
    afternoon sky a white plume over 1 read as bright streaks, not smoke. Unlit, so it follows the clouds' lighting tint
    (MPC_Lighting's CloudTint, white by day) and dims at dusk with them."""
    return effect('M_Smoke',
                  [('Speed', lambda g: g.scalar('Speed', 0.12, -900, 200))],
                  'return SmokeColor * CloudTint;',
                  [('SmokeColor', lambda g: g.vector('SmokeColor', (0.78, 0.79, 0.82, 1.0), -900, 300)),
                   ('CloudTint', lambda g: lighting_tint(g, 'CloudTint', -900, 500))],
                  SMOKE_OPACITY,
                  [('Opacity', lambda g: g.scalar('Opacity', 0.28, -900, 400))])


# The Gravewind's cards (Art/Models/Props/Gravewind.py): UV1 in meters (X across, Y along: root to tail, bottom to top),
# vertex alpha the opacity, R a phase per strand or card, G how far it sways. Two octaves of the macro noise at world
# scale, panned along the strands, make the streaks or billows; they fade where a card turns edge-on, where it meets
# what's behind it, near the camera and (the wisps) far off; the colour is the sky's cool lilac turning warm toward the
# sun, times the clouds' lighting tint, so dusk reaches them.
GRAVEWIND_NOISE = """float2 P = float2(UV1.x * ScaleAcross, (UV1.y - Time * Pan) * ScaleAlong) + Phase * 7.0;
P += 0.18 * float2(sin(P.y * 2.1 + Time * 0.2), sin(P.x * 1.7 - Time * 0.15)) * Distort;
// Three octaves, the first warped by the second (the billows'), shaped soft: a hard cut of one bilinear octave broke into
// stair-stepped blotches.
float N2 = Texture2DSample(Noise, NoiseSampler, P * 2.3 + 0.37).r;
float N1 = Texture2DSample(Noise, NoiseSampler, P + (N2 - 0.5) * 0.15 * Distort).r;
float N3 = Texture2DSample(Noise, NoiseSampler, P * 5.0 + 0.71).r;
float Shape = smoothstep(ShapeLow, ShapeHigh, N1 * Octave1 + N2 * Octave2 + N3 * Octave3);
float EdgeOn = saturate(abs(dot(normalize(Normal), CameraVector)) * 2.5);
float Dist = length(WorldPos - CameraPos);
float Near = saturate((Dist - NearStart) / max(NearEnd - NearStart, 1.0));
float Far = 1.0 - saturate((Dist - FarStart) / max(FarEnd - FarStart, 1.0));
"""
# Inside a bank the fog thins to BillowFloor but never opens onto the canyon floor; its card's own edges still fade it.
GRAVEWIND_OPACITY = GRAVEWIND_NOISE + """return saturate(Alpha * lerp(BillowFloor, 1.0, Shape) * EdgeOn * Near * Far * Opacity) * Soft;"""
GRAVEWIND_COLOR = """float Toward = pow(saturate(dot(-CameraVector, normalize(SunDirection + 1e-5))), SunPower);
float Rise = lerp(BaseShade, 1.0, saturate((WorldPos.z - ObjectPos.z) / 1000.0));
return lerp(Cool, Warm, Toward) * Rise * Brightness * CloudTint;"""
GRAVEWIND_SWAY = """return normalize(Normal) * Sway * Amount * sin(Time * Speed + Phase * 6.28318 + UV1.y * Along);"""


def gravewind(name, settings):
    """An unlit, translucent, two-sided master for the Gravewind's cards, by the art session's spec: settings holds each
    parameter's default (the wisps' and the fog's differ)."""
    mat = material(name)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    g = Graph(mat)
    vertex = g.node(unreal.MaterialExpressionVertexColor, -1300, -100)
    uv1 = g.node(unreal.MaterialExpressionTextureCoordinate, -1300, -250, coordinate_index=1)
    normal = g.node(unreal.MaterialExpressionVertexNormalWS, -1300, 50)
    camera = g.node(unreal.MaterialExpressionCameraVectorWS, -1300, 150)
    world = g.node(unreal.MaterialExpressionWorldPosition, -1300, 250)
    time = g.node(unreal.MaterialExpressionTime, -1300, 350)
    params = {key: g.scalar(key, value, -1700, 40 * i) for i, (key, value) in enumerate(settings['scalars'].items())}
    soft = g.node(unreal.MaterialExpressionDepthFade, -1000, 600)
    g.link(params['DepthFade'], '', soft, 'FadeDistance')
    opacity = g.custom(GRAVEWIND_OPACITY, [
        ('UV1', uv1, ''), ('Alpha', vertex, 'A'), ('Phase', vertex, 'R'),
        ('Noise', g.node(unreal.MaterialExpressionTextureObjectParameter, -1300, 450, parameter_name='Noise',
                         texture=import_mask(MACRO_NOISE_FILE, MACRO_NOISE)), ''),
        ('Time', time, ''), ('Normal', normal, ''), ('CameraVector', camera, ''), ('WorldPos', world, ''),
        ('CameraPos', g.node(unreal.MaterialExpressionCameraPositionWS, -1300, 550), ''), ('Soft', soft, ''),
    ] + [(key, params[key], '') for key in ('ScaleAcross', 'ScaleAlong', 'Pan', 'Distort', 'ShapeLow', 'ShapeHigh',
                                           'Octave1', 'Octave2', 'Octave3', 'BillowFloor', 'NearStart', 'NearEnd',
                                           'FarStart', 'FarEnd', 'Opacity')],
        unreal.CustomMaterialOutputType.CMOT_FLOAT1, -600, 200, f'{name} opacity')
    color = g.custom(GRAVEWIND_COLOR, [
        ('CameraVector', camera, ''), ('SunDirection', g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection,
                                                               -1300, 650), ''),
        ('WorldPos', world, ''), ('ObjectPos', g.node(unreal.MaterialExpressionObjectPositionWS, -1300, 750), ''),
        ('Cool', g.vector('Cool', settings['cool'], -1700, 600), ''),
        ('Warm', g.vector('Warm', settings['warm'], -1700, 700), ''),
        ('CloudTint', lighting_tint(g, 'CloudTint', -1700, 800), ''),
    ] + [(key, params[key], '') for key in ('SunPower', 'BaseShade', 'Brightness')],
        unreal.CustomMaterialOutputType.CMOT_FLOAT3, -600, -100, f'{name} color')
    sway = g.custom(GRAVEWIND_SWAY, [
        ('Normal', normal, ''), ('Sway', vertex, 'G'), ('Phase', vertex, 'R'), ('UV1', uv1, ''), ('Time', time, ''),
    ] + [(key, params[key], '') for key in ('Amount', 'Speed', 'Along')],
        unreal.CustomMaterialOutputType.CMOT_FLOAT3, -600, 500, f'{name} sway')
    g.out(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    g.out(sway, '', unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    # Drawn as instances (ADuskScenery): without the usage saved, the game falls back to the default material.
    finish(mat, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES])
    return mat


# Brightness is on the painted clouds' scale (their LitColor about 3, before the lighting state's CloudTint, a quarter of
# it at dusk), and Warm keeps their lit sides' ratio: the banks sit just lighter than the canyon haze they hang in, as fog
# must; the wisps a barely-there cold drift, gone by 45 m so the far side's don't sit on the rim as tufts (the art
# session's values from the game's dusk, 2026-10-07).
def build_gravewind_wisp():
    """M_GravewindWisp: fine streaming streaks off the Rim at dusk (lengths in cm, Pan in m a second along UV1's V)."""
    return gravewind('M_GravewindWisp', {
        'cool': (0.434, 0.468, 0.658, 1.0), 'warm': (1.0, 0.79, 0.53, 1.0),
        'scalars': dict(ScaleAcross=1.6, ScaleAlong=0.12, Pan=2.5, Distort=0.0, ShapeLow=0.25, ShapeHigh=0.8,
                        Octave1=0.6, Octave2=0.3, Octave3=0.1, BillowFloor=0.0, NearStart=50.0, NearEnd=200.0, FarStart=1500.0, FarEnd=4500.0, Opacity=0.35, DepthFade=50.0,
                        SunPower=6.0, BaseShade=1.0, Brightness=2.5, Amount=15.0, Speed=1.3, Along=0.8)})


def build_canyon_fog():
    """M_CanyonFog: slow billowing banks rising out of the canyon at the deck, darker low down (no far fade: they're
    the view): broad soft rolls about 25 m across, never opening onto the floor inside a bank."""
    return gravewind('M_CanyonFog', {
        'cool': (0.503, 0.527, 0.701, 1.0), 'warm': (1.0, 0.79, 0.53, 1.0),
        'scalars': dict(ScaleAcross=0.04, ScaleAlong=0.04, Pan=0.3, Distort=0.4, ShapeLow=0.05, ShapeHigh=0.75,
                        Octave1=0.75, Octave2=0.25, Octave3=0.0, BillowFloor=0.45,
                        NearStart=400.0, NearEnd=1200.0, FarStart=1.0e6, FarEnd=2.0e6, Opacity=0.7, DepthFade=300.0,
                        SunPower=3.0, BaseShade=0.85, Brightness=3.0, Amount=40.0, Speed=0.785, Along=0.0)})


# Painted Frontier's screen pass (the Style Lab's painted_edges.js edgePaint, then its grade's vibrance and its violet
# vignette), on the pre-exposed scene colour before depth of field, so TAA smooths the bands. Convex edges catch a warm
# light band (brighter on the sun's side, a fixed size in the world), creases take a soft violet line, from the
# neighbours' normals in view space; a neighbour across a depth jump is left out, so silhouettes don't light up, and
# nothing within EdgeK.w metres (the first-person gun) or past Edge.w is touched.
PAINT_POST = """float3 c = Color.rgb;
float z = Depth.r * 0.01;
if (Edge.x > 0.0 && z > EdgeK.w && z < Edge.w)
{
    float2 uv = GetDefaultSceneTextureUV(Parameters, 1);
    float wpx = clamp(Edge.x / max(z, 0.1), Edge.y, Edge.z) * (View.ViewSizeAndInvSize.y / 1080.0);
    float2 o = wpx * View.BufferSizeAndInvSize.zw;
    float2 uR = uv + float2(o.x, 0.0);
    float2 uL = uv - float2(o.x, 0.0);
    float2 uU = uv - float2(0.0, o.y);
    float2 uD = uv + float2(0.0, o.y);
    float tol = z * 0.035 + 0.04;
    float wx = step(abs(SceneTextureLookup(uR, 1, false).r * 0.01 - z), tol) * step(abs(SceneTextureLookup(uL, 1, false).r * 0.01 - z), tol);
    float wy = step(abs(SceneTextureLookup(uU, 1, false).r * 0.01 - z), tol) * step(abs(SceneTextureLookup(uD, 1, false).r * 0.01 - z), tol);
    float3x3 toView = (float3x3)View.TranslatedWorldToView;
    float3 nr = mul(SceneTextureLookup(uR, 8, false).rgb, toView);
    float3 nl = mul(SceneTextureLookup(uL, 8, false).rgb, toView);
    float3 nu = mul(SceneTextureLookup(uU, 8, false).rgb, toView);
    float3 nd = mul(SceneTextureLookup(uD, 8, false).rgb, toView);
    float curv = ((nr.x - nl.x) * wx + (nu.y - nd.y) * wy) * 0.5;
    float fade = (1.0 - smoothstep(Edge.w * 0.5, Edge.w, z)) * smoothstep(EdgeK.w, EdgeK.w * 1.5, z);
    float vex = smoothstep(0.05, 0.6, curv) * fade;
    float cav = smoothstep(0.05, 0.6, -curv) * fade;
    float lit = saturate(dot(Normal.rgb, normalize(SunDir + 1e-5)) * 0.5 + 0.5);
    float l = dot(c, float3(0.2126, 0.7152, 0.0722));
    c += EdgeTint.rgb * vex * EdgeK.x * (EdgeK.z + lit) * max(l, 0.05) * 1.5;
    c = lerp(c, c * CreaseTint.rgb * 2.0, cav * EdgeK.y);
}
// vibrance: the less saturated a colour, the more it's pushed
float lv = dot(c, float3(0.2126, 0.7152, 0.0722));
float hi = max(c.r, max(c.g, c.b));
float sat = (hi - min(c.r, min(c.g, c.b))) / max(hi, 1e-4);
c = max(lerp(float3(lv, lv, lv), c, 1.0 + Vibrance * (1.0 - sat)), 0.0);
// a soft violet vignette
float2 vd = (ViewportUV - 0.5) * float2(lerp(View.ViewSizeAndInvSize.x * View.ViewSizeAndInvSize.w, 1.0, Vignette.z), 1.0);
float vv = smoothstep(1.0 - Vignette.y, 1.0 + Vignette.y * 0.2, length(vd) * 1.4142);
return lerp(c, VignetteColor.rgb, vv * Vignette.x);"""


def scene_texture(g, name, x, y):
    """A SceneTexture node (its id by the enum's Python name, whose spelling has varied)."""
    ids = unreal.SceneTextureId
    wanted = {'PostProcessInput0': ('PPI_POST_PROCESS_INPUT0', 'PPI_POSTPROCESSINPUT0'),
              'SceneDepth': ('PPI_SCENE_DEPTH', 'PPI_SCENEDEPTH'), 'WorldNormal': ('PPI_WORLD_NORMAL', 'PPI_WORLDNORMAL')}
    for spelling in wanted[name]:
        if hasattr(ids, spelling):
            return g.node(unreal.MaterialExpressionSceneTexture, x, y, scene_texture_id=getattr(ids, spelling))
    raise RuntimeError(f'no SceneTextureId for {name} (tried {wanted[name]})')


def build_painted_post():
    """M_PaintedPost: Painted Frontier's screen pass (PAINT_POST). Lvl_TutorialIsland's PaintedPost volume wears an
    instance of it (Tools/Unreal/build_island_painted.py); nothing else does."""
    mat = material('M_PaintedPost')
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
    mat.set_editor_property('blendable_location', unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_DOF)
    g = Graph(mat)
    d = look().POST_DEFAULTS
    v = {name: pvector(g, name, value, -1000, 400 + 60 * i) for i, (name, value) in enumerate(d['vectors'].items())}
    color = g.custom(PAINT_POST, [
        ('Color', scene_texture(g, 'PostProcessInput0', -1000, -300), 'Color'),
        ('Depth', scene_texture(g, 'SceneDepth', -1000, -150), 'Color'),
        ('Normal', scene_texture(g, 'WorldNormal', -1000, 0), 'Color'),
        ('ViewportUV', g.node(unreal.MaterialExpressionScreenPosition, -1000, 150), 'ViewportUV'),
        ('SunDir', g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection, -1000, 250), ''),
        ('Edge', v['PaintEdge'], 'RGBA'), ('EdgeK', v['PaintEdgeK'], 'RGBA'), ('EdgeTint', v['PaintEdgeTint'], 'RGBA'),
        ('CreaseTint', v['PaintCreaseTint'], 'RGBA'), ('Vignette', v['PaintVignette'], 'RGBA'),
        ('VignetteColor', v['PaintVignetteColor'], 'RGBA'),
        ('Vibrance', pscalar(g, 'PaintVibrance', d['scalars']['PaintVibrance'], -1000, 1000), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 0, 'Painted Frontier screen pass')
    g.out(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(mat, [])
    return mat


BUILDERS = {'M_World': lambda: build_world(DEFAULT_ORM), 'M_Gun': lambda: build_gun(DEFAULT_ORM),
            'M_WorldFoliage': lambda: build_foliage(DEFAULT_ORM),
            'M_Terrain': build_terrain, 'M_Water': build_water, 'M_SkyClouds': build_sky_clouds,
            'M_Waterfall': build_waterfall, 'M_Smoke': build_smoke, 'M_Glass': build_glass, 'M_Gel': build_gel,
            'M_Backdrop': build_backdrop, 'M_GravewindWisp': build_gravewind_wisp, 'M_CanyonFog': build_canyon_fog,
            'M_PaintedPost': build_painted_post}
wanted = [name for name in sys.argv[1:] if name in BUILDERS] or list(BUILDERS)
orm = default_orm()
built = [BUILDERS[name]() for name in wanted]
unreal.log('LOOTER world materials: ' + ', '.join(m.get_path_name() for m in built))
