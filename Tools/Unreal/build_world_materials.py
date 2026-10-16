"""Builds the master materials of the textured art style (Docs/TutorialIsland.md) in /Game/Art/Materials/Masters:

  M_World         opaque: BaseColorMap, NormalMap, ORMMap (ambient occlusion, roughness, metallic), Tint, UVScale.
                  The vertex color alpha is baked ambient occlusion (SSAO is off on Medium), and DiffuseAO lets some of
                  it darken the base color too, so contact shading shows in direct light. MossAmount (0 = off) grows
                  moss on upward faces, in MossColor, above the MossThreshold slope. RoughnessOffset (added to the
                  map's) and Specular (0.5 is Unreal's 4%) are for cloth and feathers: black wool reflects so little.
  M_Gun           M_World's maps for gun parts, plus per-gun wear: Wear (0 fresh to 1 battered) comes from each part's
                  custom primitive data 0 (set by UWeaponModelComponent, no material copies per gun): scuffs where the
                  finish is rubbed through to a paler layer, grime in the creases, a duller finish. No moss. Its notches
                  from the same data: the tally (NotchMarks 1, NotchRow 2-5, NotchHeight 6, NotchDepth 7) cut dark into
                  the sides of one part along a row in the part's own space (its pre-skinned position, so the
                  first-person view's own field of view can't move it), and a soul-forged gun's faint rim glow in its
                  rarity's color (SoulLight 8-11, emissive).
  M_WorldFoliage  masked, two-sided (back faces keep the front's normal), the same maps (opacity from the base
                  color's alpha) plus wind: vertex color R is how far a vertex sways, G offsets its phase;
                  WindStrength (cm), WindSpeed, WindDirection.
  M_Terrain       MacroMap on UV 0 covers the whole island (its alpha picks the detail: 0 grass/soil, 1 rock); the
                  Grass* and Rock* maps tile on UV 1 (meters) and only modulate the macro color's brightness. The
                  ground between (alpha under 0.55) shares three patterns, picked per pixel from the macro map's own
                  color and alpha, the terrain's baked occlusion and GroundNoise (group Ground; GROUND_WEIGHTS): the
                  grass on green ground, matted thatch with grass-root clumps (GroundThatch) on gold and dry ground,
                  stony ground (GroundStony: cracked crust, gravel, stones; leaf litter under trees) on bare soil,
                  roads, footpaths, scree and bare spots, and damp soil in the hollows; each normalized by its own mean
                  (T_<Set>_stats.json), so from afar the macro map's color and brightness are what they were. Faces
                  steeper than SteepStart degrees (fully past SteepFull) take the rock map laid on from the side as
                  their detail, with no detail normal: maps laid on from above smear down a cliff. The face keeps the
                  macro map's color with the rock's light and dark on it (SteepDetail), darkened by SteepTint, some
                  bands darker or redder (SteepVariation), and the rock's layers rise and fall along it (StrataWarp
                  meters over StrataWave), so a long bluff wears the area's colors and no ruled stripes. Its relief comes from the rock's normal map laid on from the same sides, at
                  SteepNormalStrength (0: none, as on the island; SteepNormalGreen flips its green if the cracks read
                  inverted). Ransom's Rest's (2026-10-14; each off by default): SteepRock makes the faces the cliff
                  kit's own rock (the side-laid map's colour in SteepRockTint, a little macro light and dark, beds by height:
                  SteepBeds), SteepAOLift lifts their baked occlusion, SteepSharp, SteepBreakup and SteepPull move
                  their foot off the terrain's triangles onto the rock's layers and the macro's painted rock, and
                  StrataSmooth and StrataFlat keep the layers' bend long and low. No texture samples beyond the old.
  M_SkyClouds     unlit, translucent: painted clouds on a sky dome (Coverage, Softness, Scale, wind), each lit from
                  the sun's side (the sky atmosphere's sun): a warm LitColor where it thins toward the sun, a cool
                  ShadeColor where it thickens, darker cores against the sun and thin edges glowing near it.
  M_Smoke         unlit, translucent effect scrolling the macro noise: chimney smoke. Vertex color A is opacity.
  M_Waterfall     unlit, translucent, two-sided falling water (the water contract of 2026-10-14): three copies of
                  T_WaterStreaks falling at different scales and speeds in world meters (UV 0: U across a ribbon, V meters
                  along the flow; the level's stretch in Z taken back out), see-through gaps and ragged edges cut from
                  them by the vertex alpha (A), a darker body (WaterColor) white only where vertex R says it breaks
                  (FoamColor), DepthFade where it meets the pool or the rock, times MPC_Lighting's CloudTint.
  M_WaterMist     unlit, translucent, two-sided spray at a fall's foot and lip: the macro noise billowing outward from
                  BillowOrigin on each card's UV 0, inside the card's soft vertex alpha (A), fading edge-on, near the
                  camera and against what's behind it, brighter toward the sun, times CloudTint.
  M_GravewindWisp, unlit, translucent, two-sided: the Gravewind's cards at Gravewind Point (Art/Models/Props/
  M_CanyonFog     Gravewind.py): world-scale noise streaks (the wisps off the Rim) or billows (the fog banks in the
                  canyon) panned along UV1, fading edge-on, against what's behind, near the camera and (wisps) far off;
                  lilac turning warm toward the sun, times MPC_Lighting's CloudTint; a sway from vertex G.
  M_Water         opaque, lit, cheap (two T_WaterNormal taps, one T_WaterFoam tap): the creeks and ponds, from the water
                  mesh's vertex colors (R shore closeness, G depth share, B and A the flow packed 0..1) and UV 0 in
                  meters: teal shallows (ShallowColor) deepening to blue-green (WaterColor), ripples flowing downstream
                  (a two-phase flow map; still water drifts), the sky's tint toward grazing angles (SkyColor), a broken,
                  lapping foam line at the shore (FoamColor); the sun's glints from its low roughness. Lit like the
                  bank, so dusk dims it. A mesh without vertex colors reads as still water with no shore.
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

  M_BulletMark    a deferred decal, translucent (DBuffer colour, normal, roughness and metallic: drawn before the lighting,
                  so it's lit as its surface is, on Medium's Nanite fallback without Lumen as on High): the bullet marks
                  (Source/AI_Looter_Shooter/Weapons/BulletMarks). The atlas T_BulletMarks_BC (colour, alpha the opacity),
                  _N and _ORM (G roughness, B metallic) is script-made by Art/Textures/BulletMarks/make_bullet_marks.py: 8
                  cells across and 4 down, a style's two variants side by side. Each decal's colour picks its cell (R: the
                  style, (style + 0.5) / 16; G: the seed, past 0.5 the second variant; A: its strength). The opacity fades
                  with the decal's life (DecalLifetimeOpacity: FBulletMarks sets its hold and fade) and where the surface
                  doesn't face back up the decal's axis (the back of a thin board in its box, a face it would smear along).
                  Opacity and NormalStrength scale the whole look; Tint its colour. It isn't built until the atlas exists.

  M_SunbleachedPost  post process (before depth of field): Sunbleached's screen pass, the lab's luminous cream haze over
                  everything solid (its height fog, thick with distance and glowing toward the sun) and its lavender
                  vignette. Only the tutorial island's style volume uses it (build_island_style.py sunbleached).

Island styles: Style Lab looks tried on the tutorial island alone (Tools/Unreal/build_island_style.py). Each adds a
static switch named for it to the masters it changes, off by default, so every instance and level keeps exactly the
look above: the switch's off side is the unstyled graph, with a constant 0 emissive where there was none. An
island-only instance turns it on. A second style's switch wraps the first's off side the same way (styled()).

Sunbleached (Style Lab style 6; numbers in style_sunbleached.py, parameters Bleach* in group Sunbleached) on M_World,
M_WorldFoliage and M_Terrain (06_bleached.js's hooks, run on the lit colour in the lab, moved here: a multiplier of
the base colour is the same for diffuse light, and what the lab adds to the lit colour is emissive):
  - base colour: the albedo graded (BleachAdjust: saturation, value, contrast, hue; BleachTint: toward sandstone
    cream, keeping its luma), shade facing away from the sun shifted lavender (BleachShade; cast shadows take it from
    the sky light's filter and the grade), the baked occlusion tinted blue-violet (BleachAO, the lab's AO pass);
  - emissive, in the sun's own light (SkyAtmosphereLightIlluminance, as the lab's slSunCol): a warm rim, blazing when
    a surface stands between the eye and the sun (BleachRim, BleachRimPower; on foliage from the crown's vertex normal,
    so leaf cards don't speckle), a warm bounce lifting the shade (BleachBounce), a glow from within past the
    terminator (BleachGlow), glitter: sparse world-space facets that flash when they mirror the sun, the cells
    doubling in size with distance (BleachGlitter, BleachGlitter2.x; rock, stone and the ground), and the sand's broad
    sheen toward the sun (BleachGlitter2.y; the ground). The lab's constant terms scale with the sun's brightness over
    its own (BleachGlitter2.z, 4.6).
M_SkyClouds' paints the lab's sky on the dome (bleached_sky.js): white-gold horizon to pale turquoise, a lavender band
opposite the sun, sheared cirrus from the macro noise, the sun's huge glow, the 22-degree ice halo and a white-hot disc,
the haze at the horizon in the lab's cream fog.

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


def wind_param(g, name, x, y):
    """A gust parameter UGustSubsystem writes (MPC_Wind: GustStrength, 0 between gusts; GustDirection, its way in xy).
    wind_collection.py makes the collection first, as lighting_tint's does."""
    import importlib
    import os
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    import wind_collection
    collection = importlib.reload(wind_collection).collection()
    return g.node(unreal.MaterialExpressionCollectionParameter, x, y, collection=collection, parameter_name=name)


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


# --- Island styles (Tools/Unreal/build_island_style.py; see the docstring) ---

SUNBLEACHED = 'Sunbleached'
# Each Bleach* vector packs four numbers (style_sunbleached.py names them); Custom nodes take it whole.
BLEACH_SURFACE_VECTORS = ('BleachAdjust', 'BleachTint', 'BleachShade', 'BleachAO')
BLEACH_LIGHT_VECTORS = ('BleachRim', 'BleachBounce', 'BleachGlow', 'BleachGlitter', 'BleachGlitter2')

# Sunbleached on the base colour (06_bleached.js material(): kit.pbr's albedo grade, then shadeHook and the AO pass),
# from the unstyled colour (map x tint, moss, DiffuseAO already on it).
BLEACH_SURFACE = """const float3 Wl = float3(0.2126, 0.7152, 0.0722);
const float3 Gy = float3(0.57735, 0.57735, 0.57735);
float3 c = max(Color, 0.0);
// The albedo graded (the lab's slAdjust): hue turned, saturation about its luma, contrast about 0.18, value, and a
// tint toward sandstone cream that keeps the luma.
float ha = Adjust.w * 0.0174533;
c = c * cos(ha) + cross(Gy, c) * sin(ha) + Gy * dot(Gy, c) * (1.0 - cos(ha));
float l0 = dot(c, Wl);
c = max(lerp(float3(l0, l0, l0), c, Adjust.x), 0.0);
c = pow(max(c, 1e-6) / 0.18, Adjust.z) * 0.18;
c *= Adjust.y;
c = saturate(lerp(c, TintC.rgb * (dot(c, Wl) / max(dot(TintC.rgb, Wl), 0.001)), TintC.a));
// Lavender shade: what faces away from the sun shifts lavender (blue up, green down), never grey. Cast shadows take it
// from the sky light's filter and the grade's shadows, which the base pass can't see.
float lit = smoothstep(-0.05, 0.3, dot(N, normalize(SunDir + 1e-5)));
c *= lerp(float3(1.0, 1.0, 1.0), Shade.rgb, (1.0 - lit) * Shade.a);
// Contact shade from the baked occlusion, tinted blue-violet (the lab's AO pass; Medium has no SSAO).
c *= lerp(AO.rgb, float3(1.0, 1.0, 1.0), lerp(1.0, pow(max(saturate(Occlusion), 1e-4), 1.6), AO.a));
return c;"""

# 06_bleached.js's hooks that add light (lib/bleached_glsl.js), as emissive, in the sun's own light: SunLight is its
# illuminance (lux, SkyAtmosphereLightIlluminance), as the lab's slSunCol, so these keep their share of the diffuse light
# and follow the sun. The lab's constant terms (rim, bounce) were set against its own sun (Glitter2.z, 4.6) and scale
# with this one's brightness. The rim reads RimN, the crown's smooth vertex normal on foliage (the pixel normal elsewhere),
# taken either way round, so leaf cards' edges don't speckle.
BLEACH_LIGHT = """const float3 Wl = float3(0.2126, 0.7152, 0.0722);
float3 Ls = normalize(SunDir + 1e-5);
float3 Vv = normalize(V);
float3 sun = max(SunLight, 0.0);
float sunScale = dot(sun, Wl) / max(Glitter2.z, 0.001);
float ndl = dot(N, Ls);
float occ = saturate(Occlusion);
float3 e = float3(0.0, 0.0, 0.0);
// a warm rim, blazing on silhouettes that stand between the eye and the sun
float toward = pow(saturate(dot(-Vv, Ls)), 2.0);
e += Rim.rgb * pow(saturate(1.0 - abs(dot(normalize(RimN), Vv))), RimPower) * (0.22 + 1.1 * toward) * (0.45 + 0.55 * occ) * Rim.a * sunScale;
// the shade lifted by light bounced off the sunlit ground, from below and the sides
e += Albedo * Bounce.rgb * (1.0 - saturate(ndl)) * (0.55 - 0.45 * N.z) * occ * Bounce.a * sunScale;
// glow from within: sunlight carried a little past the terminator and warmed, as through sandstone and dry timber
float wrapL = saturate((ndl + 0.35) / 1.35);
e += Albedo * sun * Glow.rgb * wrapL * (1.0 - smoothstep(0.3, 0.9, ndl)) * Glow.a;
// the sand's broad sheen toward the sun at grazing angles (Journey's "ocean specular")
float3 sR = reflect(-Vv, N);
e += sun * pow(saturate(dot(sR, Ls)), 10.0) * (0.25 + 0.75 * pow(1.0 - saturate(dot(N, Vv)), 2.0)) * Glitter2.y;
// glitter: sparse facets in world-space cells (Glitter.y metres across at 2 m, doubling with each doubling of distance,
// so a glint stays about two pixels wide) that flash when they mirror the sun into the eye, twinkling slowly; Glitter.x
// of the cells can glint, fading out by Glitter.w metres; Glitter2.x is how far the facets tilt from the surface
if (Glitter.z > 0.0)
{
    float gd = length(WorldPos - CameraPos) * 0.01;
    float gcs = Glitter.y * exp2(floor(log2(max(gd, 2.0) / 2.0)));
    float3 gcp = floor(WorldPos * 0.01 / gcs);
    const float offs[5] = { 0.0, 11.7, 23.1, 37.3, 5.0 };
    float hv[5];
    [unroll] for (int i = 0; i < 5; i++)
    {
        float3 q = frac((gcp + offs[i]) * 0.1031);
        q += dot(q, q.zyx + 31.32);
        hv[i] = frac((q.x + q.y) * q.z);
    }
    float3 gN = normalize(N + (float3(hv[0], hv[1], hv[2]) * 2.0 - 1.0) * Glitter2.x);
    float glint = smoothstep(0.972, 0.994, dot(gN, normalize(Ls + Vv)));
    float keep = step(1.0 - Glitter.x, hv[3]);
    float fade = 1.0 - smoothstep(Glitter.w * 0.45, Glitter.w, gd);
    float twinkle = 0.75 + 0.25 * sin(Time * 3.0 + hv[4] * 40.0);
    e += sun * glint * keep * fade * twinkle * step(0.0, ndl) * Glitter.z;
}
return e;"""


def style_numbers(name):
    """Tools/Unreal/style_<name>.py, an island style's numbers, beside this script. Imported when a master is built, not
    with this module: build_creature_materials.py and build_decal_materials.py run its imports without its body.
    Reloaded (with style_common.py), as the editor keeps modules between runs."""
    import importlib
    import os
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    importlib.reload(importlib.import_module('style_common'))
    return importlib.reload(importlib.import_module(f'style_{name}'))


def sparam(g, group, name, value, x, y):
    node = g.scalar(name, float(value), x, y)
    node.set_editor_property('group', group)
    return node


def svector(g, group, name, value, x, y):
    values = [float(v) for v in value][:4]
    node = g.vector(name, tuple(values + [1.0] * (4 - len(values))), x, y)
    node.set_editor_property('group', group)
    return node


def styled(g, style, on, off, x, y, on_out='', off_out=''):
    """A style's static switch (named for it, off by default): on when an island instance turns it on, else off (the
    unstyled graph, or a style switched before it, unchanged)."""
    switch = g.node(unreal.MaterialExpressionStaticSwitchParameter, x, y, parameter_name=style, default_value=False)
    switch.set_editor_property('group', style)
    g.link(on, on_out, switch, 'True')
    g.link(off, off_out, switch, 'False')
    return switch


def black(g, x, y):
    """The unstyled side of an emissive a style adds: what an unconnected emissive is."""
    return g.node(unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(0.0, 0.0, 0.0, 1.0))


def bleached(g, color, color_out, ao, ao_out, defaults, x, y, rim_normal=None):
    """Sunbleached's base colour and emissive for a surface (BLEACH_SURFACE, BLEACH_LIGHT) from its unstyled colour and
    baked occlusion, with defaults (style_sunbleached.SURFACE_, FOLIAGE_ or GROUND_DEFAULTS) for the Bleach* parameters.
    The rim reads rim_normal when given (foliage: the crown's vertex normal), else the pixel normal."""
    group = SUNBLEACHED
    v = {name: svector(g, group, name, defaults['vectors'][name], x - 700, y + 60 * i)
         for i, name in enumerate(BLEACH_SURFACE_VECTORS + BLEACH_LIGHT_VECTORS)}
    normal = g.node(unreal.MaterialExpressionPixelNormalWS, x - 400, y - 300)
    sun = g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection, x - 400, y - 200)
    base = g.custom(BLEACH_SURFACE, [
        ('Color', color, color_out), ('Occlusion', ao, ao_out), ('N', normal, ''), ('SunDir', sun, ''),
        ('Adjust', v['BleachAdjust'], 'RGBA'), ('TintC', v['BleachTint'], 'RGBA'), ('Shade', v['BleachShade'], 'RGBA'),
        ('AO', v['BleachAO'], 'RGBA'),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, x, y, 'Sunbleached surface')
    light = g.custom(BLEACH_LIGHT, [
        ('Albedo', base, ''), ('Occlusion', ao, ao_out), ('N', normal, ''), ('RimN', rim_normal or normal, ''),
        ('V', g.node(unreal.MaterialExpressionCameraVectorWS, x - 400, y + 300), ''), ('SunDir', sun, ''),
        ('SunLight', g.node(unreal.MaterialExpressionSkyAtmosphereLightIlluminance, x - 400, y + 400), ''),
        ('WorldPos', g.node(unreal.MaterialExpressionWorldPosition, x - 400, y + 500), ''),
        ('CameraPos', g.node(unreal.MaterialExpressionCameraPositionWS, x - 400, y + 600), ''),
        ('Time', g.node(unreal.MaterialExpressionTime, x - 400, y + 700), ''),
        ('Rim', v['BleachRim'], 'RGBA'),
        ('RimPower', sparam(g, group, 'BleachRimPower', defaults['scalars']['BleachRimPower'], x - 700, y + 600), ''),
        ('Bounce', v['BleachBounce'], 'RGBA'), ('Glow', v['BleachGlow'], 'RGBA'),
        ('Glitter', v['BleachGlitter'], 'RGBA'), ('Glitter2', v['BleachGlitter2'], 'RGBA'),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, x, y + 400, 'Sunbleached light')
    return base, light


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
    # Sunbleached (its switch, off by default): the graded, lavender-shaded colour, and its light as emissive.
    base_b, light_b = bleached(g, color, '', ao, '', style_numbers('sunbleached').SURFACE_DEFAULTS, 400, 1600)
    g.out(styled(g, SUNBLEACHED, base_b, color, 900, -600), '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(nrm, 'RGB', unreal.MaterialProperty.MP_NORMAL)
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(g.scalar('Specular', 0.5, 300, -150), '', unreal.MaterialProperty.MP_SPECULAR)
    g.out(orm, 'B', unreal.MaterialProperty.MP_METALLIC)
    g.out(ao, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    g.out(styled(g, SUNBLEACHED, light_b, black(g, 600, 0), 900, 0), '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
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


# The tally cut into a gun's stock (UWeaponModelComponent::ShowNotches; Source/AI_Looter_Shooter/Weapons/
# WeaponModelNotches.cpp places it on each part). Every edge is filtered to the pixel (the overlap of a pixel-wide box
# with the cut), so far off the marks fade to a dark smudge instead of shimmering.
TALLY_CODE = """// Marks cuts, four upright and a fifth slashed across them in each group of five, along the row from Row.xy to Row.zw in
// the part's own space (cm: X along the gun, Z up), Height tall, cut only into the outside of its sides (a sideways
// surface at least Depth from the part's middle). Returns the cut (x) and a pale lip of chipped finish beside it (y).
float2 Result = float2(0.0, 0.0);
float2 Delta = Row.zw - Row.xy;
float Len = max(length(Delta), 0.01);
float2 Along = Delta / Len;
float2 Across = float2(-Along.y, Along.x);
float S = dot(Local.xz - Row.xy, Along);
float T = dot(Local.xz - Row.xy, Across);
// The pixel's footprint, taken before any branch (gradients need every pixel of the quad).
float Px = max(max(fwidth(S), fwidth(T)), 0.001);
if (Marks > 0.5)
{
    float Side = saturate((abs(normalize(LocalNormal).y) - 0.55) * 5.0) * step(Depth, abs(Local.y));
    float Group = Len / 5.0;
    float Pitch = Group / 5.5;
    float HalfW = Pitch * 0.22;
    float HalfH = Height * 0.5;
    float G = floor(S / Group);
    float X = S - G * Group;
    float Cut = 0.0;
    float Lip = 0.0;
    if (Side > 0.0 && G >= 0.0 && G < 5.0)
    {
        // The nearest upright, if it's been cut yet; each leans and runs a little its own way, as if cut by hand.
        float K = clamp(floor(X / Pitch), 0.0, 3.0);
        float Hand = frac(sin((G * 4.0 + K + 1.0) * 78.233) * 43758.5453);
        float D = X - (K + 0.5) * Pitch - T * (Hand - 0.5) * 0.14;
        float Reach = HalfH * (0.88 + 0.16 * Hand);
        float AD = abs(D);
        float AT = abs(T);
        float Upright = saturate((min(HalfW, AD + Px * 0.5) - max(-HalfW, AD - Px * 0.5)) / Px)
                      * saturate((min(Reach, AT + Px * 0.5) - max(-Reach, AT - Px * 0.5)) / Px);
        float LD = abs(D - HalfW * 1.9);
        float LipW = HalfW * 0.45;
        float LipCover = saturate((min(LipW, LD + Px * 0.5) - max(-LipW, LD - Px * 0.5)) / Px) * step(AT, Reach);
        float IsCut = step(G * 5.0 + K + 0.5, Marks);
        Cut = Upright * IsCut;
        Lip = LipCover * IsCut;
        // The fifth slashes across the four once the group is full.
        if (G * 5.0 + 4.5 < Marks)
        {
            float2 A = float2(-0.15 * Pitch, -HalfH * 0.8);
            float2 AB = float2(4.3 * Pitch, HalfH * 1.6);
            float2 Q = float2(X, T) - A;
            float SD = length(Q - AB * saturate(dot(Q, AB) / dot(AB, AB)));
            float SW = HalfW * 1.1;
            Cut = max(Cut, saturate((min(SW, SD + Px * 0.5) - max(-SW, SD - Px * 0.5)) / Px));
        }
    }
    Result = float2(Cut * Side, Lip * Side * (1.0 - Cut));
}
return Result;"""

# The cut takes the finish off and lies in shadow; beside it the finish is chipped paler.
CARVE_CODE = """float3 C = Color * (1.0 - 0.8 * Tally.x);
return lerp(C, C * 1.3 + 0.025, Tally.y * 0.6);"""

# A soul-forged gun's soul-light: faint over it, gathering at its edges as they turn away, breathing slowly. Soul.a is
# its strength, 0 on every other gun.
SOUL_CODE = """float Facing = saturate(dot(normalize(N), normalize(V)));
float Rim = pow(1.0 - Facing, 3.0);
float Breath = 0.8 + 0.2 * sin(Time * 1.6);
return Soul.rgb * Soul.a * (0.08 + Rim) * Breath;"""


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

    # Notches (UWeaponModelComponent::ShowNotches sets the custom primitive data per part; 0 on parts without them).
    def cpd_scalar(name, index, x, y):
        return g.node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=0.0,
                      use_custom_primitive_data=True, primitive_data_index=index)

    def cpd_vector(name, index, x, y):
        return g.node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                      default_value=unreal.LinearColor(0.0, 0.0, 0.0, 0.0), use_custom_primitive_data=True,
                      primitive_data_index=index)

    # The part's own position and normal from the vertex shader (pre-skinned: the mesh's own space, which neither the
    # gun's place nor the first-person view's field of view and scale can move), handed on to the pixels.
    local = g.node(unreal.MaterialExpressionVertexInterpolator, -800, 1200)
    g.link(g.node(unreal.MaterialExpressionPreSkinnedPosition, -1100, 1200), '', local, 'VS')
    local_normal = g.node(unreal.MaterialExpressionVertexInterpolator, -800, 1350)
    g.link(g.node(unreal.MaterialExpressionPreSkinnedNormal, -1100, 1350), '', local_normal, 'VS')
    tally = g.custom(TALLY_CODE, [
        ('Local', local, ''),
        ('LocalNormal', local_normal, ''),
        ('Marks', cpd_scalar('NotchMarks', 1, -800, 1500), ''),
        ('Row', cpd_vector('NotchRow', 2, -800, 1600), 'RGBA'),
        ('Height', cpd_scalar('NotchHeight', 6, -800, 1700), ''),
        ('Depth', cpd_scalar('NotchDepth', 7, -800, 1800), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT2, -300, 1300, 'Notch tally')
    carved = g.custom(CARVE_CODE, [
        ('Color', rgb, ''), ('Tally', tally, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, 200, -300, 'Carved tally')
    rough = g.custom('return lerp(saturate(Roughness + Wear * 0.12 + Scuff * 0.15), 0.95, Tally.x);', [
        ('Roughness', orm, 'G'), ('Wear', wear, ''), ('Scuff', scuff, ''), ('Tally', tally, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, 200, 100, 'Worn roughness')
    occlusion = g.custom('return Occlusion * (1.0 - 0.6 * Tally.x);', [
        ('Occlusion', ao, ''), ('Tally', tally, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, 200, 300, 'Tally occlusion')
    soul = g.custom(SOUL_CODE, [
        ('Soul', cpd_vector('SoulLight', 8, -300, 1900), 'RGBA'),
        ('N', g.node(unreal.MaterialExpressionPixelNormalWS, -300, 2000), ''),
        ('V', g.node(unreal.MaterialExpressionCameraVectorWS, -300, 2100), ''),
        ('Time', g.node(unreal.MaterialExpressionTime, -300, 2200), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, 200, 600, 'Soul-light')
    g.out(carved, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(nrm, 'RGB', unreal.MaterialProperty.MP_NORMAL)
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(orm, 'B', unreal.MaterialProperty.MP_METALLIC)
    g.out(occlusion, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    g.out(soul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(mat, [])
    return mat


WIND_CODE = """float Phase = VertexColor.g * 6.2831 + InstanceRandom * 6.2831;
float Sway = sin(Time * Speed + Phase) * 0.7 + sin(Time * Speed * 2.37 + Phase * 1.7) * 0.3;
float2 Dir = normalize(Direction.xy + float2(0.0001, 0.0));
// A gust (UGustSubsystem, MPC_Wind; 0 between gusts): the plants bow downwind and sway harder as its front rolls over
// them, a band about 25 m long passing at 5 m/s.
float2 GDir = normalize(GustDirection.xy + float2(0.0001, 0.0));
float Front = 0.5 + 0.5 * sin(dot(WorldPos.xy, GDir) * 0.0025 - Time * 1.25);
float G = GustStrength * lerp(0.55, 1.0, Front);
float Flutter = sin(Time * Speed * 3.1 + Phase * 2.3);
float2 Push = Dir * Sway * (1.0 + G * GustSway) + GDir * G * (GustBow + Flutter * GustFlutter);
return float3(Push * Strength * VertexColor.r, 0.0);"""


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
    # Sunbleached (its switch, off by default): the graded, lavender-shaded colour, and its rim as emissive, read from
    # the crown's own vertex normals (which point out of the crown or up from the ground): the cards' normal-mapped
    # pixel normals turn edge-on all over a crown and lit its leaf cards' edges in pale specks (style 3's lesson).
    base_b, light_b = bleached(g, color, '', ao, '', style_numbers('sunbleached').FOLIAGE_DEFAULTS, 400, 1600,
                               rim_normal=g.node(unreal.MaterialExpressionVertexNormalWS, 0, 1500))
    g.out(styled(g, SUNBLEACHED, base_b, color, 900, -600), '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(styled(g, SUNBLEACHED, light_b, black(g, 600, 0), 900, 0), '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
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
        ('GustStrength', wind_param(g, 'GustStrength', -800, 1300), ''),
        ('GustDirection', wind_param(g, 'GustDirection', -800, 1400), ''),
        ('WorldPos', g.node(unreal.MaterialExpressionWorldPosition, -800, 1500,
                            world_position_shader_offset=unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS), ''),
        ('GustBow', g.scalar('GustBow', 1.6, -800, 1600), ''),
        ('GustSway', g.scalar('GustSway', 1.5, -800, 1700), ''),
        ('GustFlutter', g.scalar('GustFlutter', 0.8, -800, 1800), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, 900, 'Wind')
    g.out(wind, '', unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    # Foliage is never Nanite (it has LODs instead), and the Nanite permutation of this material doesn't compile on SM6.
    # Skeletal too: creatures (the slime's core and pebbles) take their colors from the foliage palette.
    finish(mat, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH])
    return mat


# The ground layers (2026-10-12, the user: "The ground should be slightly more textured, its literally just dirt right
# now"): between the grass blades the ground was one fine stubble tile, which a few meters off mips to flat dirt. Now
# three patterns share it, chosen per pixel from what the macro map already says, so they follow any regenerated map:
# the grass (GroundGrass, the instance's Grass maps) on green ground, matted thatch with grass-root clumps
# (GroundThatch) on gold and dry ground, and stony ground (GroundStony: cracked crust, gravel, small stones, leaf litter
# in its alpha) on bare soil, roads, footpaths, scree, bare spots and the dark floor under trees; damp, darker, glossier
# soil in the hollows (the terrain's baked occlusion). GroundNoise (four soft noises over 51.2 m) makes the borders wander
# in patches, opens the bare spots, varies the damp and slides the layers' repeats apart. Every layer is normalized by
# its own mean (T_<Set>_stats.json), so from afar the ground has exactly the brightness and the color it had: the macro
# map stays in charge, and the layers' contrast fades with distance (GroundFade). Rock (the macro alpha past 0.55) and
# steep faces keep the rock detail as before.
# GROUND_WEIGHTS is shared by the color, normal and roughness nodes (the shader compiler merges the repeats): it sets
# Wt (the grass, thatch and stony weights), Damp, Leaf, RockAmt and Fd (the distance fade).
GROUND_WEIGHTS = """const float3 Wl = float3(0.299, 0.587, 0.114);
// The layers' contrast fades from GroundFade.x to .y meters away to .z of it: the macro map carries the distance.
float Fd = lerp(1.0, Fade.z, smoothstep(Fade.x * 100.0, Fade.y * 100.0, Depth));
// The macro map's cues, from its linear color: how warm it is (red over green: bare soil, then gold, then green), how
// dark and how saturated.
float Lm = dot(Macro, Wl);
float Warm = (Macro.r - Macro.g) / max(Macro.r + Macro.g, 0.002);
float Hi = max(Macro.r, max(Macro.g, Macro.b));
float Sat = (Hi - min(Macro.r, min(Macro.g, Macro.b))) / max(Hi, 0.002);
// Where one kind of ground gives way to another wanders in patches (Noise.r), so it never traces the macro map's own
// contours.
float W = Warm + (Noise.r - 0.5) * Cues.z;
float Lush = 1.0 - smoothstep(Cues.x - 0.05, Cues.x + 0.05, W);
float Soil = smoothstep(Cues.y - 0.04, Cues.y + 0.04, W);
// Stony ground: bare soil, roads, footpaths and scree (the macro alpha between grass and rock), bare spots in the dry
// grass (Noise.g's highest), and the dark, dull ground under trees (Shade), which takes the leaf litter.
float Road = smoothstep(0.1, 0.2, Select);
float Bare = smoothstep(1.0 - Cues.w, 1.0 - Cues.w + 0.05, Noise.g) * (1.0 - 0.7 * Lush);
float Shade = (1.0 - smoothstep(Cues2.y * 0.65, Cues2.y, Lm)) * (1.0 - smoothstep(0.66, 0.76, Sat));
float S = saturate(max(max(Soil, Road), max(Bare, Shade * 0.85)));
// Green ground shares itself between the grass and the thatch (matted under a green macro color, it reads as green
// grass laid flat), in patches (Noise.a).
float G = (1.0 - S) * Lush * saturate(Cues2.x * (0.4 + 1.2 * Noise.a));
float T = max(1.0 - S - G, 0.0);
// Height blend: each layer's own height (the thatch's alpha; the grass's and the stony ground's light and dark) decides
// which shows where two meet, so stones and tussocks stand out of the next layer instead of fading into it.
float HG = saturate(dot(Grass, Wl) / GrassStats.a * 0.5);
float HT = Thatch.a;
float HS = saturate(dot(Stony.rgb, Wl) / StonyStats.a * 0.5);
float3 Bl = float3(G, T, S) + float3(HG, HT, HS) * Cues2.w * saturate(float3(G, T, S) * 4.0);
float3 Wt = max(Bl - (max(Bl.x, max(Bl.y, Bl.z)) - 0.2), 0.0);
Wt /= max(Wt.x + Wt.y + Wt.z, 1e-4);
// Damp: the hollows (the terrain's baked occlusion) darker and glossier, in patches (Noise.b), drying off the tops of
// the layers' relief; and dark, warm low ground (mud by the water).
float H = dot(Wt, float3(HG, HT, HS));
float Damp = (1.0 - smoothstep(DampP.y, DampP.x, Occlusion)) * saturate(0.3 + 1.4 * Noise.b);
Damp = max(Damp, Shade * Soil * 0.6);
Damp = saturate(Damp * (1.25 - 0.6 * H)) * DampP.w * Fd;
// Leaf litter on the stony ground: thick where it's shaded (under trees), a few leaves anywhere it's bare.
float Leaf = saturate((Stony.a - 0.01) * 1.65) * Wt.z * saturate(Shade * Cues2.z + 0.12) * Fd;
// Rock and the steeper scree keep the rock detail.
float RockAmt = smoothstep(0.55, 0.9, Select);
"""

TERRAIN_CODE = GROUND_WEIGHTS + """// From afar the ground keeps the brightness it always had: the old grass and rock detail at their true means
// (GrassStats.a, RockTrue: measured from the textures) over the means it was set against (GrassMean, RockMean).
float K = lerp(1.0, lerp(GrassStats.a, RockTrue, Select) / max(lerp(GrassMean, RockMean, Select), 0.05), Strength);
// Each layer's light and dark (Layers.xyz) and a little of its own color (Layers.w), around 1 from its own mean: the
// macro map keeps the color, the layer brings the pattern. <Layer>Stats: rgb its mean luma over each channel's mean
// (so its average color comes out white), a its mean luma.
float LG = max(dot(Grass, Wl), 0.004);
float LT = max(dot(Thatch.rgb, Wl), 0.004);
float LS = max(dot(Stony.rgb, Wl), 0.004);
const float3 One = float3(1.0, 1.0, 1.0);
float3 PG = lerp(1.0, LG / GrassStats.a, Layers.x) * lerp(One, clamp(Grass / LG * GrassStats.rgb, 0.6, 1.5), Layers.w);
float3 PT = lerp(1.0, LT / ThatchStats.a, Layers.y) * lerp(One, clamp(Thatch.rgb / LT * ThatchStats.rgb, 0.6, 1.5), Layers.w);
float3 PS = lerp(1.0, LS / StonyStats.a, Layers.z) * lerp(One, clamp(Stony.rgb / LS * StonyStats.rgb, 0.6, 1.5), Layers.w);
float3 Pattern = Wt.x * PG + Wt.y * PT + Wt.z * PS;
// The leaves: LeafTint (a fallen leaf's color over the ground's), lit by their own light and dark in the alpha.
Pattern = lerp(Pattern, LeafTint * lerp(0.75, 1.15, saturate((Stony.a - 0.62) / 0.38)), Leaf);
float3 Ground = Macro * K * lerp(One, Pattern, Fd) * lerp(1.0, DampP.z, Damp);
// Rock: the macro map with the rock detail's light and dark, as before.
float3 Old = lerp(Grass, Rock, Select);
float3 RockColor = Macro * lerp(1.0, dot(Old, Wl) / max(lerp(GrassMean, RockMean, Select), 0.05), Strength);
float3 Color = lerp(Ground, RockColor, RockAmt);
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
float3 OldSteep = Macro * SteepTint * Banding * lerp(1.0, SideLuma / max(RockMean, 0.05), SteepDetail);
// SteepRock (0 unless an area sets it; Ransom's Rest, 2026-10-14): the face is the cliff kit's own rock instead, so a
// wall the kit leaves bare reads as the same sandstone as the pieces on it rather than dark mud: the side-laid rock
// map's own colour in the area's cliff tint (SteepRockTint: the kit's MI Tint), keeping a little of the macro map's light and
// dark (SteepMacroKeep, around SteepMacroMean, the macro's mean on its rock), in beds by height (SteepBeds: x a bed's
// height in metres, y how much lighter or darker each is, z a soft parting's darkening, w its width as a share of the
// bed), counted up the face from the side-laid V, so they bend with the rock map's own layers.
float2 BedV = float2(SideUVX.y, SideUVY.y) / (max(RockScale, 0.01) * max(SteepBeds.x, 0.1));
float2 BedI = floor(BedV);
float2 BedT = BedV - BedI;
float2 H0 = frac(sin(BedI * 12.9898 + 4.1) * 43758.5453);
float2 H1 = frac(sin((BedI + 1.0) * 12.9898 + 4.1) * 43758.5453);
float2 Bs = lerp(H0, H1, smoothstep(0.85, 1.0, BedT));
float2 Parting = (1.0 - smoothstep(0.0, max(SteepBeds.w, 0.01), BedT)) * step(0.45, H0);
float2 BedShade = (1.0 + (Bs - 0.5) * 2.0 * SteepBeds.y) * (1.0 - SteepBeds.z * Parting);
float BedS = Bs.x * Facing.x + Bs.y * Facing.y;
float3 Beds = (BedShade.x * Facing.x + BedShade.y * Facing.y) * (1.0 + (BedS - 0.5) * float3(0.06, 0.0, -0.06));
float MacroVar = lerp(1.0, clamp(Lm / max(SteepMacroMean, 0.01), 0.7, 1.35), SteepMacroKeep);
float3 KitSteep = Side * SteepRockTint * MacroVar * Banding * Beds;
Color = lerp(Color, lerp(OldSteep, KitSteep, SteepRock), Steep);
// SteepAOLift (0 unless an area sets it): a steep face's baked occlusion counts the sky its own angle hides, which the
// sky light's shading already leaves out, so shaded walls went black; on the faces it's lifted toward 1 (the material's
// ambient occlusion output takes the same lift).
float Occ = lerp(Occlusion, 1.0, Steep * SteepAOLift);
return Color * lerp(1.0, Occ, DiffuseAO);"""
# The edge between the ground and a steep face (SteepEdge): the steepness from the vertex normals, sharpened (Sharp)
# and moved by the side-laid rock's light and dark (Breakup) and by the macro map's alpha, painted from the true slope
# (Pull), so the face's foot follows the rock's layers and the painted rock instead of the terrain's triangles (the
# vertex normals made a saw-toothed foot under every bluff). Sharp 1, Breakup 0 and Pull 0 (the master's defaults) give
# the steepness unchanged.
STEEP_EDGE_CODE = """float2 Facing = pow(abs(normalize(Normal).xy), 4.0);
Facing /= max(Facing.x + Facing.y, 1e-4);
float H = saturate(dot(RockX * Facing.x + RockY * Facing.y, float3(0.299, 0.587, 0.114)) / max(RockTrue, 0.05) * 0.5);
return saturate((Steep - 0.5) * Sharp + 0.5 + (H - 0.5) * Breakup + (Select - 0.5) * Pull);"""
# The ground's detail normal: the layers' normals by the same weights, flattened under leaves and where it's damp (wet
# soil is smooth), and the rock's where it's rock. The caller fades it on steep faces, as before.
GROUND_NORMAL_CODE = GROUND_WEIGHTS + """float3 N = Wt.x * GrassN + Wt.y * ThatchN + Wt.z * StonyN;
N = lerp(N, float3(0.0, 0.0, 1.0), saturate(Leaf * 0.8 + Damp * 0.5));
return lerp(N, RockN, RockAmt);"""
# Roughness: dry grass and soil matte, stones a little smoother, damp ground glossier (GroundFade.w), rock and steep
# faces 0.82 as before.
GROUND_ROUGHNESS_CODE = GROUND_WEIGHTS + """float R = dot(Wt, float3(0.93, 0.92, lerp(0.95, 0.8, saturate(HS * 2.0 - 1.0))));
R = lerp(R, Fade.w, Damp);
return lerp(R, 0.82, max(RockAmt, Steep));"""
# The layers' UVs on UV 1 (meters) at their own repeats, slid by GroundNoise's slow drift (A, up to Warp meters, the
# two layers opposite ways), so neither repeat lines up for long.
LAYER_UV_CODE = 'return UV * Scale + (Noise.a - 0.5) * 2.0 * Warp * Scale * {sign} + {offset};'
LAYER_UV_SLIDE = {'Thatch': ('1.0', 'float2(0.0, 0.0)'), 'Stony': ('-1.0', 'float2(0.37, 0.61)')}
# The ground layers' parameters (group Ground) as the master sets them; an area's instance may change any.
# Tools/Blender/ground_preview.py reads this table too.
GROUND_DEFAULTS = {
    'GroundNoiseTile': 51.2,             # meters a repeat of GroundNoise (Tools/Blender/looter_textures.py)
    'GroundWarp': 0.5,                   # meters the layers' repeats slide
    'ThatchScale': 1.0 / 3.2,            # repeats a meter (looter_textures THATCH_TILE_M)
    'StonyScale': 1.0 / 2.8,             # (STONY_TILE_M)
    # x where green ends (warmth: red over green), y where bare soil starts, z how far the borders wander, w the share
    # of the dry grass opened into bare spots
    'GroundCues': (-0.05, 0.13, 0.14, 0.1),
    # x the grass's share of green ground (the rest is thatch), y how dark the ground under trees is (linear luma),
    # z its leaf litter, w the height blend
    'GroundCues2': (0.4, 0.15, 0.9, 0.5),
    # x, y the baked occlusion where the damp starts and where it's full, z its darkening, w how much of it
    'GroundDamp': (0.9, 0.62, 0.75, 1.0),
    # x, y where the layers' contrast starts and ends fading (m), z the share left past it, w damp ground's roughness
    'GroundFade': (20.0, 80.0, 0.5, 0.62),
    # the grass's, the thatch's and the stony ground's light and dark, w how much of their own color they bring
    'GroundLayers': (0.45, 0.6, 0.7, 0.25),
    'LeafTint': (1.18, 0.96, 0.78),      # a fallen leaf's color over the ground's (luma about 1)
}
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
# undulate, pinch and thicken instead of ruling a whole face. StrataSmooth (a mip of the noise; 0 by default) and
# StrataFlat (the share of its change up the face taken out; 0 by default) keep that bend long and low: from the noise's
# finest mip, at twice its rate up the face, a 1.5 m bend turned the layers round the noise's blobs, and Ransom's Rest's
# bare walls read as wood grain (2026-10-14).
SIDE_UV_CODE = """float W = sin(A * 0.0011 + 1.3) * 0.14 + sin(A * 0.0037 + P.z * 0.0007) * 0.05 + A * 0.00002;
float N = Texture2DSampleLevel(Noise, NoiseSampler, float2(A, P.z * 2.0 * (1.0 - Flat)) * 0.01 / max(Wave, 1.0),
                               Smooth).r;
W += (N - 0.5) * 2.0 * Warp * Scale;
return float2(A * Scale * 0.01, -P.z * Scale * 0.01 + W);"""
# How steep the ground is, from 0 (gentler than SteepStart degrees) to 1 (steeper than SteepFull).
STEEP_CODE = 'return 1.0 - smoothstep(cos(radians(Full)), cos(radians(Start)), normalize(Normal).z);'
# The steep faces' parameters as the master sets them (TERRAIN_CODE, STEEP_EDGE_CODE and SIDE_UV_CODE say what each
# is); an area's instance may change any. Tools/Blender/ground_preview.py reads this table too (its walls), and takes
# STEEP_ADDED at these values for the look before 2026-10-14.
STEEP_DEFAULTS = {
    'RockScale': 0.25, 'RockMeanLuma': 0.35, 'DiffuseAO': 0.45,
    'SteepStart': 50.0, 'SteepFull': 65.0, 'SteepDetail': 0.75, 'SteepTint': (1.0, 1.0, 1.0),
    'SteepVariation': 0.0, 'SteepNormalStrength': 0.0, 'SteepNormalGreen': 1.0,
    'StrataWarp': 0.0, 'StrataWave': 45.0, 'StrataSmooth': 0.0, 'StrataFlat': 0.0,
    'SteepRock': 0.0, 'SteepRockTint': (1.0, 1.0, 1.0), 'SteepMacroKeep': 0.0, 'SteepMacroMean': 0.125,
    'SteepBeds': (1.7, 0.0, 0.0, 0.07), 'SteepAOLift': 0.0, 'SteepSharp': 1.0, 'SteepBreakup': 0.0, 'SteepPull': 0.0,
}
STEEP_ADDED = ('StrataSmooth', 'StrataFlat', 'SteepRock', 'SteepRockTint', 'SteepMacroKeep', 'SteepMacroMean',
               'SteepBeds', 'SteepAOLift', 'SteepSharp', 'SteepBreakup', 'SteepPull')

TEXTURES = 'C:/Dev/AI_Looter_Shooter/Art/Textures'
# GroundGrass's and RockCliff's mean linear color and luma, measured from their color maps (2026-10-12): every terrain
# instance's Grass and Rock maps (Art/Levels/area_model.py DETAIL_SETS). The ground's old brightness from afar comes from
# these against GrassMeanLuma and RockMeanLuma, so the layers keep it.
GRASS_MEAN = (0.0987, 0.1359, 0.0286, 0.1125)
ROCK_TRUE_LUMA = 0.2976


def import_texture(file, path, kind):
    """A texture from the library with the settings the model importer gives a set's maps (kind 'BC': color, sRGB; 'N':
    normal map; 'M': a linear mask, kept uncompressed), imported again whenever the PNG changes (its MD5 is kept on the
    asset as SourceMD5): importing only when the asset was missing once left a regenerated mask out of the scatter."""
    import hashlib
    with open(file, 'rb') as source:
        digest = hashlib.md5(source.read()).hexdigest()
    library = unreal.EditorAssetLibrary
    texture = unreal.load_asset(path) if library.does_asset_exist(path) else None
    if texture is None or library.get_metadata_tag(texture, 'SourceMD5') != digest:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', file)
        task.set_editor_property('destination_path', path.rsplit('/', 1)[0])
        task.set_editor_property('destination_name', path.rsplit('/', 1)[1])
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture = unreal.load_asset(path)
        unreal.log(f'{path} imported from {file} (MD5 {digest})')
    if kind == 'N':
        texture.set_editor_property('srgb', False)
        texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
        texture.set_editor_property('flip_green_channel', False)
    elif kind == 'M':
        texture.set_editor_property('srgb', False)
        texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
        texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
    else:
        texture.set_editor_property('srgb', True)
        texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
        texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
    library.set_metadata_tag(texture, 'SourceMD5', digest)
    library.save_loaded_asset(texture)
    return texture


def ground_texture(set_name, kind):
    """One of M_Terrain's ground layers' maps (Tools/Blender/looter_textures.py makes them), imported to
    /Game/Art/Textures/<Set>/T_<Set>_<kind>, where the model importer would put it; returns its path."""
    path = f'/Game/Art/Textures/{set_name}/T_{set_name}_{kind}'
    import_texture(f'{TEXTURES}/{set_name}/T_{set_name}_{kind}.png', path, kind)
    return path


def layer_stats(set_name=None, mean=None):
    """A ground layer's <Layer>Stats for M_Terrain from its mean linear color and luma (T_<Set>_stats.json, written with
    its textures, or mean as (r, g, b, luma)): the luma over each channel's mean (the shader multiplies a texel's color
    over its luma by these, so the layer's average color comes out white), and the luma."""
    if mean is None:
        import json
        with open(f'{TEXTURES}/{set_name}/T_{set_name}_stats.json') as source:
            stats = json.load(source)
        mean = tuple(stats['meanLinear']) + (stats['meanLuma'],)
    luma = mean[3]
    return tuple(luma / max(c, 0.004) for c in mean[:3]) + (luma,)


def shared_wrap(sample):
    """A texture sample on the shared wrap sampler (the world group's filtering), which takes no sampler slot."""
    sample.set_editor_property('sampler_source', unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
    return sample


def steep_vector(name):
    """A vector of STEEP_DEFAULTS as a vector parameter's four numbers."""
    value = tuple(float(v) for v in STEEP_DEFAULTS[name])
    return value + (1.0,) * (4 - len(value))


def build_terrain():
    mat = material('M_Terrain')
    g = Graph(mat)
    macro_uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1800, -300, coordinate_index=0)
    macro = g.texture('MacroMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, macro_uv, -1400, -300)
    detail_coords = g.node(unreal.MaterialExpressionTextureCoordinate, -2000, 200, coordinate_index=1)
    grass_uv = g.mul(detail_coords, '', g.scalar('GrassScale', 0.5, -2000, 320), '', -1800, 200)
    rock_scale = g.scalar('RockScale', STEEP_DEFAULTS['RockScale'], -2000, 520)
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
    warp = g.scalar('StrataWarp', STEEP_DEFAULTS['StrataWarp'], -2400, 1000)
    wave = g.scalar('StrataWave', STEEP_DEFAULTS['StrataWave'], -2400, 900)
    smooth = g.scalar('StrataSmooth', STEEP_DEFAULTS['StrataSmooth'], -2600, 1000)
    flat = g.scalar('StrataFlat', STEEP_DEFAULTS['StrataFlat'], -2600, 900)
    side_uvs = [g.custom(f'float A = {axis};\n' + SIDE_UV_CODE,
                         [('P', world, ''), ('Scale', rock_scale, ''), ('Noise', noise, ''), ('Warp', warp, ''),
                          ('Wave', wave, ''), ('Smooth', smooth, ''), ('Flat', flat, '')],
                         unreal.CustomMaterialOutputType.CMOT_FLOAT2, -2000, y, f'Rock UV from the {name} side')
                for axis, name, y in (('P.y', 'X', 1300), ('P.x', 'Y', 1450))]
    # The bands' darkness: the macro noise over about 120 m along the ground and 25 m up, so it changes layer to layer.
    band = g.custom('return Texture2DSampleLevel(Noise, NoiseSampler, float2((P.x + P.y) / 12000.0, P.z / 2500.0), 0).r;',
                    [('P', world, ''), ('Noise', noise, '')],
                    unreal.CustomMaterialOutputType.CMOT_FLOAT1, -2000, 1600, 'Band darkness noise')
    rock_x = g.texture('RockBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, side_uvs[0], -1400, 1300)
    rock_y = g.texture('RockBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, WHITE, side_uvs[1], -1400, 1550)
    steep_raw = g.custom(STEEP_CODE, [
        ('Normal', vertex_normal, ''),
        ('Start', g.scalar('SteepStart', STEEP_DEFAULTS['SteepStart'], -2000, 1650), ''),
        ('Full', g.scalar('SteepFull', STEEP_DEFAULTS['SteepFull'], -2000, 1750), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1400, 1800, 'Steepness')
    # The face's edge (STEEP_EDGE_CODE): what every steep blend below uses; with the master's defaults it is the
    # steepness unchanged.
    rock_true = sparam(g, 'Ground', 'RockTrueLuma', ROCK_TRUE_LUMA, -1300, -940)
    steep = g.custom(STEEP_EDGE_CODE, [
        ('Steep', steep_raw, ''), ('Normal', vertex_normal, ''), ('RockX', rock_x, 'RGB'), ('RockY', rock_y, 'RGB'),
        ('RockTrue', rock_true, ''), ('Select', macro, 'A'),
        ('Sharp', g.scalar('SteepSharp', STEEP_DEFAULTS['SteepSharp'], -1700, 1950), ''),
        ('Breakup', g.scalar('SteepBreakup', STEEP_DEFAULTS['SteepBreakup'], -1700, 2000), ''),
        ('Pull', g.scalar('SteepPull', STEEP_DEFAULTS['SteepPull'], -1700, 2050), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1200, 1850, 'Steep face edge')
    vc = g.node(unreal.MaterialExpressionVertexColor, -1400, 1000)
    # The ground layers (GROUND_WEIGHTS): GroundNoise over GroundNoiseTile meters, the thatch and the stony ground on
    # UV 1 at their own repeats, slid apart by the noise's drift. They're the master's own textures (no instance sets
    # them), on the shared wrap sampler, so they take no sampler slots of their own.
    ground = 'Ground'
    noise_uv = g.custom('return UV / Tile;', [
        ('UV', detail_coords, ''),
        ('Tile', sparam(g, ground, 'GroundNoiseTile', GROUND_DEFAULTS['GroundNoiseTile'], -2600, -900), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT2, -2300, -900, 'Ground noise UV')
    ground_noise = shared_wrap(g.texture('GroundNoiseMap', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR,
                                         ground_texture('GroundNoise', 'M'), noise_uv, -2000, -900))
    layer_warp = sparam(g, ground, 'GroundWarp', GROUND_DEFAULTS['GroundWarp'], -2600, -700)
    layer_uvs = {}
    for name, y in (('Thatch', -700), ('Stony', -500)):
        sign, offset = LAYER_UV_SLIDE[name]
        layer_uvs[name] = g.custom(LAYER_UV_CODE.format(sign=sign, offset=offset), [
            ('UV', detail_coords, ''),
            ('Scale', sparam(g, ground, f'{name}Scale', GROUND_DEFAULTS[f'{name}Scale'], -2600, y), ''),
            ('Noise', ground_noise, 'RGBA'), ('Warp', layer_warp, ''),
        ], unreal.CustomMaterialOutputType.CMOT_FLOAT2, -2300, y, f'{name} UV')
    thatch = shared_wrap(g.texture('ThatchBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
                                   ground_texture('GroundThatch', 'BC'), layer_uvs['Thatch'], -2000, -700))
    thatch_n = shared_wrap(g.texture('ThatchNormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
                                     ground_texture('GroundThatch', 'N'), layer_uvs['Thatch'], -2000, -600))
    stony = shared_wrap(g.texture('StonyBaseColorMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
                                  ground_texture('GroundStony', 'BC'), layer_uvs['Stony'], -2000, -500))
    stony_n = shared_wrap(g.texture('StonyNormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
                                    ground_texture('GroundStony', 'N'), layer_uvs['Stony'], -2000, -400))
    # The Ground group's parameters: GROUND_DEFAULTS says what each number is.
    grass_mean = g.scalar('GrassMeanLuma', 0.3, -1000, 900)
    rock_mean = g.scalar('RockMeanLuma', STEEP_DEFAULTS['RockMeanLuma'], -1000, 1000)
    ao_lift = g.scalar('SteepAOLift', STEEP_DEFAULTS['SteepAOLift'], -1000, 1850)
    weights = [
        ('Macro', macro, 'RGB'), ('Select', macro, 'A'), ('Noise', ground_noise, 'RGBA'), ('Occlusion', vc, 'A'),
        ('Depth', g.node(unreal.MaterialExpressionPixelDepth, -1300, -1100), ''),
        ('Grass', grass, 'RGB'), ('Thatch', thatch, 'RGBA'), ('Stony', stony, 'RGBA'),
        ('Cues', svector(g, ground, 'GroundCues', GROUND_DEFAULTS['GroundCues'], -1300, -1300), 'RGBA'),
        ('Cues2', svector(g, ground, 'GroundCues2', GROUND_DEFAULTS['GroundCues2'], -1300, -1240), 'RGBA'),
        ('DampP', svector(g, ground, 'GroundDamp', GROUND_DEFAULTS['GroundDamp'], -1300, -1180), 'RGBA'),
        ('Fade', svector(g, ground, 'GroundFade', GROUND_DEFAULTS['GroundFade'], -1300, -1120), 'RGBA'),
        ('GrassStats', svector(g, ground, 'GrassStats', layer_stats(mean=GRASS_MEAN), -1300, -1060), 'RGBA'),
        ('StonyStats', svector(g, ground, 'StonyStats', layer_stats('GroundStony'), -1300, -1000), 'RGBA'),
    ]
    color = g.custom(TERRAIN_CODE, weights + [
        ('Rock', rock, 'RGB'), ('GrassMean', grass_mean, ''), ('RockMean', rock_mean, ''),
        ('Strength', g.scalar('DetailStrength', 0.6, -1000, 1100), ''),
        ('RockTrue', rock_true, ''),
        ('ThatchStats', svector(g, ground, 'ThatchStats', layer_stats('GroundThatch'), -1300, -880), 'RGBA'),
        ('Layers', svector(g, ground, 'GroundLayers', GROUND_DEFAULTS['GroundLayers'], -1300, -820), 'RGBA'),
        ('LeafTint', svector(g, ground, 'LeafTint', GROUND_DEFAULTS['LeafTint'], -1300, -760), ''),
        ('DiffuseAO', g.scalar('DiffuseAO', STEEP_DEFAULTS['DiffuseAO'], -1000, 1200), ''),
        ('RockX', rock_x, 'RGB'), ('RockY', rock_y, 'RGB'), ('Normal', vertex_normal, ''), ('Steep', steep, ''),
        ('SteepDetail', g.scalar('SteepDetail', STEEP_DEFAULTS['SteepDetail'], -1000, 1300), ''),
        ('SteepTint', g.vector('SteepTint', steep_vector('SteepTint'), -1000, 1400), ''),
        ('Band', band, ''),
        ('SteepVariation', g.scalar('SteepVariation', STEEP_DEFAULTS['SteepVariation'], -1000, 1500), ''),
        # The cliff kit's rock on steep faces (SteepRock; TERRAIN_CODE says what each is), off by default.
        ('SideUVX', side_uvs[0], ''), ('SideUVY', side_uvs[1], ''), ('RockScale', rock_scale, ''),
        ('SteepRock', g.scalar('SteepRock', STEEP_DEFAULTS['SteepRock'], -1000, 1600), ''),
        ('SteepRockTint', g.vector('SteepRockTint', steep_vector('SteepRockTint'), -1000, 1650), ''),
        ('SteepMacroKeep', g.scalar('SteepMacroKeep', STEEP_DEFAULTS['SteepMacroKeep'], -1000, 1700), ''),
        ('SteepMacroMean', g.scalar('SteepMacroMean', STEEP_DEFAULTS['SteepMacroMean'], -1000, 1750), ''),
        ('SteepBeds', g.vector('SteepBeds', steep_vector('SteepBeds'), -1000, 1800), 'RGBA'),
        ('SteepAOLift', ao_lift, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -600, 0, 'TerrainColor')
    # Sunbleached (its switch, off by default): the ground graded toward sun-baked sand and shaded lavender, and its
    # light as emissive: the sheen toward the sun and the glitter, a faint rim, bounce and glow.
    base_b, light_b = bleached(g, color, '', vc, 'A', style_numbers('sunbleached').GROUND_DEFAULTS, -300, 2600)
    g.out(styled(g, SUNBLEACHED, base_b, color, 0, -200), '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(styled(g, SUNBLEACHED, light_b, black(g, 0, -400), 200, -400), '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    normal = g.custom(GROUND_NORMAL_CODE, weights + [
        ('GrassN', grass_n, 'RGB'), ('ThatchN', thatch_n, 'RGB'), ('StonyN', stony_n, 'RGB'), ('RockN', rock_n, 'RGB'),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -900, 400, 'Ground normal')
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
        ('Green', g.scalar('SteepNormalGreen', STEEP_DEFAULTS['SteepNormalGreen'], -1000, 2300), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1000, 2000, 'Side-laid rock normal (world)')
    side_tangent = g.node(unreal.MaterialExpressionTransform, -700, 2000,
                          transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD,
                          transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
    g.link(side_world, '', side_tangent, '')
    relief = g.node(unreal.MaterialExpressionLinearInterpolate, -400, 600)
    g.link(flatten, '', relief, 'A')
    g.link(side_tangent, '', relief, 'B')
    g.link(g.custom('return Steep * Strength;', [
        ('Steep', steep, ''),
        ('Strength', g.scalar('SteepNormalStrength', STEEP_DEFAULTS['SteepNormalStrength'], -700, 2200), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -700, 2300, 'Steep relief'), '', relief, 'Alpha')
    g.out(relief, '', unreal.MaterialProperty.MP_NORMAL)
    rough = g.custom(GROUND_ROUGHNESS_CODE, weights + [('Steep', steep, '')],
                     unreal.CustomMaterialOutputType.CMOT_FLOAT1, -900, 950, 'Ground roughness')
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    # The baked occlusion, lifted on steep faces by SteepAOLift as TERRAIN_CODE lifts it (0 by default: unchanged).
    ambient = g.custom('return lerp(Occlusion, 1.0, Steep * Lift);', [
        ('Occlusion', vc, 'A'), ('Steep', steep, ''), ('Lift', ao_lift, ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -600, 1100, 'Ambient occlusion')
    g.out(ambient, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    finish(mat, [unreal.MaterialUsage.MATUSAGE_NANITE])
    return mat


# --- Water (the shared water contract, 2026-10-14: clear teal water, falling water, spray) ---

WATER_TEXTURES = 'C:/Dev/AI_Looter_Shooter/Art/Textures/Water'


def srgb(value):
    """0xRRGGBB (the contract's palette, sRGB) as a linear color parameter's four numbers."""
    def channel(byte):
        c = byte / 255.0
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    return (channel((value >> 16) & 255), channel((value >> 8) & 255), channel(value & 255), 1.0)


# The contract's palette: shallow teal, deep blue-green, foam, the falling water's body; and the sky the water tints
# toward at grazing angles (a pale blue, lit as the water is).
WATER_SHALLOW, WATER_DEEP, WATER_FOAM, FALL_BODY, WATER_SKY = 0x43918A, 0x184A54, 0xEEF5F2, 0x6F9FA3, 0xA8CCD8
# The water mesh's flow (vertex B, A unpacked to -1..1) into UV 0's axes: x' = dot(flow, (X, Y)), y' = dot(flow, (Z, W)).
# The mesh packs the flow in layout meters (x north, y east; Art/Levels/area_mesh.py); its UVMap is (east, north) and the
# FBX import flips V, so UV 0 is (east, -north): x' = y, y' = -x.
WATER_FLOW_AXES = (0.0, 1.0, -1.0, 0.0)


def water_texture(name):
    """One of the water's textures (Art/Textures/Water/make_water_textures.py writes them) at /Game/Art/Textures/Water,
    imported again whenever its PNG changes: T_WaterNormal a normal map; T_WaterFoam and T_WaterStreaks linear greyscale
    kept uncompressed (a byte a texel: block compression would band the soft thresholds the materials cut). They tile, so
    they wrap. None while the PNG is missing."""
    import os
    file = f'{WATER_TEXTURES}/{name}.png'
    if not os.path.isfile(file):
        return None
    path = f'/Game/Art/Textures/Water/{name}'
    texture = import_texture(file, path, 'N' if name == 'T_WaterNormal' else 'M')
    if name != 'T_WaterNormal':
        texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_GRAYSCALE)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def water_maps(owner, names):
    """The water textures a master needs, or None (logged) while any is missing: a stand-in would draw flat water."""
    maps = {name: water_texture(name) for name in names}
    missing = [name for name, texture in maps.items() if texture is None]
    if missing:
        unreal.log_error(f'{owner} not built: {", ".join(missing)} missing from {WATER_TEXTURES}; run Tools\\artrun.ps1 '
                         '-Script Art\\Textures\\Water\\make_water_textures.py first')
        return None
    return maps


def map_object(g, name, texture, sampler, x, y):
    """A texture object parameter for a Custom node to sample (the node gets <pin> and <pin>Sampler)."""
    return g.node(unreal.MaterialExpressionTextureObjectParameter, x, y, parameter_name=name, texture=texture,
                  sampler_type=sampler)


def tangent_to_world(g, source, x, y):
    """A tangent-space vector in world space (the Transform node), or the vector as it is should the node's settings not
    take: the water is level, so its tangent-space normal is close to the world's anyway."""
    try:
        node = g.node(unreal.MaterialExpressionTransform, x, y,
                      transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_TANGENT,
                      transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    except Exception as error:  # noqa: BLE001 - the fallback is close enough for a level surface
        unreal.log_warning(f'tangent-to-world transform not made ({error}); the Fresnel reads the tangent-space normal')
        return source
    g.link(source, '', node, '')
    return node


# The water mesh's vertex colours, shared by M_Water's foam and ripples (see M_Water in the docstring).
WATER_COMMON = """// A mesh without colours reads white: R and G both 1, which a real water mesh never has (G is 0 at the waterline), is
// taken as still water with no shore.
float Legacy = step(1.98, Col.r + Col.g);
float Shore = Col.r * (1.0 - Legacy);
// The flow, unpacked and turned into UV 0's axes (FlowAxes), past a dead zone for 8-bit stillness (0.5 is stored 0.502).
float2 Packed = float2(Col.b, FlowY) * 2.0 - 1.0;
float2 Flow = float2(dot(Packed, FlowAxes.xy), dot(Packed, FlowAxes.zw)) * (1.0 - Legacy);
Flow *= saturate(length(Flow) * 20.0 - 0.5);
// A slow phase across the surface (UV 0 is meters), so the flow's cycles and the foam's lapping never beat in step.
float Jitter = frac(0.5 + 0.25 * sin(UV.x * 0.71 + 1.7 * sin(UV.y * 0.53)) + 0.25 * sin(UV.y * 0.83 + 1.9 * sin(UV.x * 0.47)));
"""
# The shore's foam: a band FoamReach of the shore ramp wide (R runs from 1 at the waterline to 0 1.5 m in), breathing
# in and out as if lapping, broken up by T_WaterFoam (flattened: a cut c keeps 1 - c of it): fullest at the waterline,
# flecks at its inner edge, drifting and wobbling a little.
WATER_FOAM_CODE = WATER_COMMON + """float Lap = sin((Time * LapSpeed + Jitter) * 6.28318);
float Reach = FoamReach * (1.0 + 0.2 * Lap);
float T = saturate((Shore - 1.0 + Reach) / max(Reach, 0.01));
float2 Drift = frac(float2(0.6, -0.8) * (Time * FoamDrift * FoamScale));
float2 Wobble = 0.05 * float2(sin(Time * 0.7 + UV.y * 1.3), sin(Time * 0.53 + UV.x * 1.1));
float N = Texture2DSample(Tex, TexSampler, UV * FoamScale + Drift + Wobble).r;
float Cut = 1.0 - T * FoamCover;
return smoothstep(Cut, Cut + FoamSoft, N) * (0.4 + 0.6 * T) * FoamOpacity * step(0.001, T);"""
# Two taps of T_WaterNormal (one tile on 1 / RippleScale meters). Flowing water: both slide downstream (FlowSpeed m/s
# at full flow), each over one cycle (FlowCycle a second), half a cycle apart, each faded out as it jumps back: a flow
# map, so the ripples flow on forever without a creek's bends stretching them. Still water: both stay, drifting slowly
# against each other (IdleSpeed m/s). The second tap is turned 37 degrees and 1.37 times finer, so the two never line
# up; its normal is turned back. The blend keeps their contrast (an even mix of two unrelated maps is flatter than
# either); running water ripples a little more, foam flattens them.
WATER_RIPPLES = WATER_COMMON + """float PhaseA = frac(Time * FlowCycle + Jitter);
float PhaseB = frac(PhaseA + 0.5);
float Moving = saturate(length(Flow) * 3.0);
float W = lerp(0.5, 1.0 - abs(1.0 - 2.0 * PhaseA), Moving);
float Period = 1.0 / max(FlowCycle, 0.01);
float2 QA = UV - Flow * (FlowSpeed * Period * (PhaseA - 0.5));
float2 QB = UV - Flow * (FlowSpeed * Period * (PhaseB - 0.5));
float2 IdleA = frac(float2(0.8, 0.6) * (Time * IdleSpeed * RippleScale));
float2 IdleB = frac(float2(-0.6, -0.8) * (Time * IdleSpeed * RippleScale * 1.37));
float2 RB = float2(0.8 * QB.x - 0.6 * QB.y, 0.6 * QB.x + 0.8 * QB.y);
float2 NA = Texture2DSample(Tex, TexSampler, QA * RippleScale + IdleA).rg * 2.0 - 1.0;
float2 NR = Texture2DSample(Tex, TexSampler, RB * (RippleScale * 1.37) + float2(0.37, 0.61) + IdleB).rg * 2.0 - 1.0;
float2 NB = float2(0.8 * NR.x + 0.6 * NR.y, -0.6 * NR.x + 0.8 * NR.y);
float2 XY = (NA * W + NB * (1.0 - W)) * rsqrt(W * W + (1.0 - W) * (1.0 - W));
XY *= RippleStrength * (1.0 + 0.5 * Moving) * (1.0 - 0.7 * Foam);
return normalize(float3(XY, 1.0));"""
# Teal shallows deepening to blue-green (G, eased by DepthPower), the sky's tint growing toward grazing angles (Schlick's
# Fresnel for water, from the rippled normal: SkyTint of it; the sky light's own reflection adds the rest), foam over it.
# A lit base colour: the sun, the shadows and dusk light it as they light the bank.
WATER_COLOR = """float Legacy = step(1.98, Col.r + Col.g);
float Depth = lerp(Col.g, 1.0, Legacy);
float3 C = lerp(ShallowColor, WaterColor, pow(saturate(Depth), DepthPower));
float F = 0.02 + 0.98 * pow(1.0 - saturate(dot(normalize(Normal), CameraVector)), 5.0);
C = lerp(C, SkyColor, saturate(F * SkyTint));
return lerp(C, FoamColor, Foam);"""


def build_water():
    """M_Water: the creeks and ponds (see the docstring). Opaque and lit; three texture taps and a little arithmetic, so
    a view full of water costs Medium about what the same ground would. The sun's glints are the lighting's own specular
    off the ripples (Roughness), which shadows put out as they should."""
    maps = water_maps('M_Water', ('T_WaterNormal', 'T_WaterFoam'))
    if maps is None:
        return None
    mat = material('M_Water')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    g = Graph(mat)
    vertex = g.node(unreal.MaterialExpressionVertexColor, -1700, -100)
    common = [
        ('UV', g.node(unreal.MaterialExpressionTextureCoordinate, -1700, -250, coordinate_index=0), ''),
        ('Time', g.node(unreal.MaterialExpressionTime, -1700, -175), ''),
        # The vertex color's plain output is RGB only: its alpha comes in on a pin of its own.
        ('Col', vertex, ''), ('FlowY', vertex, 'A'),
        ('FlowAxes', g.vector('FlowAxes', WATER_FLOW_AXES, -1700, 50), 'RGBA'),
    ]

    def scalars(values, x, y):
        return [(name, g.scalar(name, value, x, y + 60 * i), '') for i, (name, value) in enumerate(values)]
    foam = g.custom(WATER_FOAM_CODE, common + [
        ('Tex', map_object(g, 'FoamMap', maps['T_WaterFoam'], unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE,
                           -1700, 200), ''),
    ] + scalars((('FoamScale', 0.4), ('FoamReach', 0.35), ('FoamCover', 0.7), ('FoamSoft', 0.25),
                 ('FoamOpacity', 0.9), ('LapSpeed', 0.12), ('FoamDrift', 0.03)), -2000, 200),
        unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1200, 200, 'Shore foam')
    ripples = g.custom(WATER_RIPPLES, common + [
        ('Tex', map_object(g, 'RippleMap', maps['T_WaterNormal'], unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
                           -1700, 700), ''),
        ('Foam', foam, ''),
    ] + scalars((('RippleScale', 0.2), ('RippleStrength', 0.3), ('FlowSpeed', 0.8), ('FlowCycle', 0.5),
                 ('IdleSpeed', 0.05)), -2000, 700),
        unreal.CustomMaterialOutputType.CMOT_FLOAT3, -900, 600, 'Ripples')
    g.out(ripples, '', unreal.MaterialProperty.MP_NORMAL)
    color = g.custom(WATER_COLOR, [
        ('Col', vertex, ''), ('Foam', foam, ''),
        ('Normal', tangent_to_world(g, ripples, -600, 450), ''),
        ('CameraVector', g.node(unreal.MaterialExpressionCameraVectorWS, -600, 350), ''),
        ('ShallowColor', g.vector('ShallowColor', srgb(WATER_SHALLOW), -900, -500), ''),
        ('WaterColor', g.vector('WaterColor', srgb(WATER_DEEP), -900, -420), ''),
        ('FoamColor', g.vector('FoamColor', srgb(WATER_FOAM), -900, -340), ''),
        ('SkyColor', g.vector('SkyColor', srgb(WATER_SKY), -900, -260), ''),
        ('SkyTint', g.scalar('SkyTint', 0.35, -900, -180), ''),
        ('DepthPower', g.scalar('DepthPower', 0.6, -900, -120), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -300, -200, 'Water color')
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = g.node(unreal.MaterialExpressionLinearInterpolate, -300, 300)
    g.link(g.scalar('Roughness', 0.12, -600, 250), '', rough, 'A')
    g.link(g.scalar('FoamRoughness', 0.55, -600, 300), '', rough, 'B')
    g.link(foam, '', rough, 'Alpha')
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(g.scalar('Specular', 0.5, -600, 550), '', unreal.MaterialProperty.MP_SPECULAR)
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


# Sunbleached's sky (the Style Lab's bleachedSky, lib/bleached_sky.js): bleached white-gold at the horizon to a pale
# turquoise overhead, a lavender band low on the horizon opposite the sun, the lab's cream haze at the horizon (warmer
# toward the sun), thin sheared cirrus on a high layer lit gold toward the sun (the macro noise, scaled to the lab's fbm
# spread), the sun's huge soft glow, the 22-degree ice halo (a thin ring, reddish inside and bluish outside), and the
# white-hot disc. The sun is the sky atmosphere's, so it all stands round the island's real sun. Returns the colour and
# the opacity: Opacity 1 paints the whole sky over the atmosphere (the lab's look).
BLEACH_SKY = """const float NUV = 0.0685;
float3 d = normalize(-CameraVector);
float3 L = normalize(SunDir + 1e-5);
float h = d.z;
float cs = dot(d, L);
float up = max(h, 0.0);
float3 col = lerp(Horizon.rgb, Mid.rgb, smoothstep(0.0, 0.22, up));
col = lerp(col, Zenith.rgb, smoothstep(0.18, 0.8, up));
// opposite the sun, a lavender band low on the horizon
float2 dh = normalize(d.xy + 1e-5);
float2 lh = normalize(L.xy + 1e-5);
float away = pow(saturate(-dot(dh, lh) * 0.5 + 0.5), 2.0);
col = lerp(col, Anti.rgb, away * (1.0 - smoothstep(0.0, 0.2, up)) * 0.75);
col = lerp(col, Ground.rgb, smoothstep(0.0, -0.06, h));
float3 fogC = lerp(FogColor.rgb, FogSun.rgb, saturate(pow(max(cs, 0.0), FogScatter.y) * FogScatter.x));
col = lerp(col, fogC, Sky.z * (1.0 - smoothstep(-0.02, 0.1, h)));
// cirrus: long sheared wisps on a high layer
float2 cp = d.xy / (up + 0.12);
cp = float2(0.8 * cp.x - 0.6 * cp.y, 0.6 * cp.x + 0.8 * cp.y);
float2 drift = float2(Time * 0.004, 0.0);
float n = 0.5 + (Texture2DSample(Noise, NoiseSampler, (float2(cp.x * 0.35, cp.y * 2.6) + drift) * NUV).r - 0.5) * 0.45;
float n2 = 0.5 + (Texture2DSample(Noise, NoiseSampler, (float2(cp.x * 1.3, cp.y * 7.0) + 3.0 + drift * 2.0) * NUV).r - 0.5) * 0.45;
float dens = smoothstep(0.52, 0.86, n * 0.72 + n2 * 0.42) * smoothstep(0.03, 0.22, h) * Sky.y;
float3 cc = lerp(CirrusShade.rgb, Cirrus.rgb, smoothstep(0.3, 0.8, n2)) + SunGlow.rgb * pow(max(cs, 0.0), 6.0) * 0.25;
col = lerp(col, cc, dens);
// the sun's glow: a huge soft bloom, a brighter middle, a tight core (the angle from the cross product near the sun,
// where acos loses the small angles)
float ang = cs > 0.7 ? asin(min(length(cross(d, L)), 1.0)) : acos(clamp(cs, -1.0, 1.0));
col += SunGlow.rgb * Sky.w * (exp(-ang * 2.4) * 0.06 + exp(-ang * 10.0) * 0.16 + exp(-ang * 50.0) * 1.2);
// the 22-degree halo, fading toward the ground
float hr = (ang - 0.3840) / 0.011;
float ring = exp(-hr * hr) + 0.3 * exp(-hr * hr / 9.0);
float3 rc = lerp(float3(1.0, 0.72, 0.5), float3(0.78, 0.88, 1.0), smoothstep(-1.2, 2.0, hr));
col += HaloC.rgb * rc * ring * Sky.x * smoothstep(-0.04, 0.12, h);
// the disc, white-hot
col += SunDisc.rgb * (1.0 - smoothstep(0.012, 0.0145, ang)) * step(0.0, cs);
return float4(col, saturate(max(Opacity, dens)));"""

# The sky's Custom node inputs and the Bleach* vectors that feed them.
BLEACH_SKY_INPUTS = (('Zenith', 'BleachZenith'), ('Mid', 'BleachMid'), ('Horizon', 'BleachHorizon'),
                     ('Ground', 'BleachGround'), ('Anti', 'BleachAnti'), ('Cirrus', 'BleachCirrus'),
                     ('CirrusShade', 'BleachCirrusShade'), ('FogColor', 'BleachFogColor'), ('FogSun', 'BleachFogSun'),
                     ('SunDisc', 'BleachSunDisc'), ('SunGlow', 'BleachSunGlow'), ('HaloC', 'BleachHalo'),
                     ('Sky', 'BleachSky'), ('FogScatter', 'BleachFogScatter'))


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
    # Sunbleached (its switch, off by default): the lab's bleached sky on the dome (BLEACH_SKY), times the lighting
    # state's cloud tint as the clouds are.
    d = style_numbers('sunbleached').SKY_DEFAULTS
    v = {name: svector(g, SUNBLEACHED, name, value, -1600, 1400 + 60 * i)
         for i, (name, value) in enumerate(d['vectors'].items())}
    sky = g.custom(BLEACH_SKY, [
        ('CameraVector', inputs[0][1], ''),
        ('SunDir', g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection, -1200, 1300), ''),
        ('Time', inputs[2][1], ''), ('Noise', noise, ''),
        ('Opacity', sparam(g, SUNBLEACHED, 'BleachSkyOpacity', d['scalars']['BleachSkyOpacity'], -1600, 1300), ''),
    ] + [(pin, v[name], 'RGBA') for pin, name in BLEACH_SKY_INPUTS],
        unreal.CustomMaterialOutputType.CMOT_FLOAT4, -800, 1400, 'Sunbleached sky')
    sky_rgb = g.node(unreal.MaterialExpressionComponentMask, -500, 1400, r=True, g=True, b=True, a=False)
    g.link(sky, '', sky_rgb, '')
    sky_alpha = g.node(unreal.MaterialExpressionComponentMask, -500, 1550, r=False, g=False, b=False, a=True)
    g.link(sky, '', sky_alpha, '')
    g.out(styled(g, SUNBLEACHED, g.mul(sky_rgb, '', tint, '', -300, 1400), emissive, 100, -100), '',
          unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(styled(g, SUNBLEACHED, sky_alpha, opacity, 100, 200), '', unreal.MaterialProperty.MP_OPACITY)
    finish(mat, [])
    return mat


# Effects: unlit and translucent, scrolling the macro noise. Their models carry the shape in vertex colors (A opacity)
# and UV 0 (0..1 up the cards for smoke).
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


# Falling water (M_Waterfall), shared by its colour and opacity. Meters along the flow in the world: UV 0's V is the
# mesh's own meters along it, and the level stretches a fall in Z (a short fall's model squashed to its drop, a tall
# one to the canyon's), so that stretch is taken back out of the falling part: DepthW is how far below the lip (the
# pivot) a point is in the world, DepthW / ScaleZ in the mesh. Three copies of T_WaterStreaks fall at different scales
# and speeds (Speed m/s for the broadest, Length meters a repeat, Across repeats over a ribbon), so the sheet never
# repeats in step: S1 the ropes of water, S2 and S3 the finer threads. Where the vertex foam (R) is low, smooth water
# rolling over the lip, the streaks stay faint and whole.
FALL_LAYERS = """float ScaleZ = max(length(ZAxis), 0.05);
float DepthW = (ObjectPos.z - WorldPos.z) * 0.01;
float Along = UV.y + DepthW * (1.0 - 1.0 / ScaleZ);
float L2 = Length * 0.61;
float L3 = Length * 0.37;
float S1 = Texture2DSample(Tex, TexSampler, float2(UV.x * Across, Along / Length - frac(Time * Speed / Length))).r;
float S2 = Texture2DSample(Tex, TexSampler, float2(UV.x * Across * 1.73 + 0.37, Along / L2 - frac(Time * Speed * 1.35 / L2))).r;
float S3 = Texture2DSample(Tex, TexSampler, float2(UV.x * Across * 2.91 + 0.71, Along / L3 - frac(Time * Speed * 1.8 / L3))).r;
float Break = smoothstep(0.25, 0.75, Foam);
float Streak = 0.5 + (S1 * 0.5 + S2 * 0.3 + S3 * 0.2 - 0.5) * (0.45 + 0.55 * Break);
"""
# A darker body, lighter in its ropes, white only where it breaks: R lowers the cut on the fine threads (FoamAmount of
# it), so a breaking strand whitens in streaks, never all at once. Unlit, at the brightness a lit surface of its colour
# shows in the sun (Brightness), dimmed at dusk by the lighting state's cloud tint as the smoke is.
FALL_COLOR = FALL_LAYERS + """float Fine = S2 * 0.6 + S3 * 0.4;
float WhiteCut = 1.0 - Foam * FoamAmount;
float White = smoothstep(WhiteCut, WhiteCut + FoamSoft, Fine);
float3 Water = WaterColor * (0.8 + 0.4 * S1);
return lerp(Water, FoamColor, White) * Brightness * CloudTint;"""
# See-through gaps (a cut on the streaks: GapCalm of the sheet open on the lip, GapBreak once it breaks) and ragged
# edges: where the mesh thins the sheet (alpha toward its edges and its frayed foot) the cut rises toward CutMax, so
# only the densest streaks stay. It fades where a ribbon turns edge-on (crossed ribbons would show as lines; EdgeFade)
# and where it meets what's behind it (Fade, the depth fade: the pool, the rock, the creek under the lip).
FALL_OPACITY = FALL_LAYERS + """float Gap = lerp(GapCalm, GapBreak, Break);
float Cut = lerp(Gap, CutMax, 1.0 - saturate(Alpha));
float Keep = smoothstep(Cut - Soft, Cut + Soft, Streak);
float Dense = Keep * lerp(Body, 1.0, Streak);
float EdgeOn = lerp(1.0 - EdgeFade, 1.0, saturate(abs(dot(normalize(Normal), CameraVector)) * 3.0));
return saturate(Dense * saturate(Alpha * 3.0) * EdgeOn * Opacity) * Fade;"""


def water_effect(name):
    """An unlit, translucent, two-sided master, emptied for its graph."""
    mat = material(name)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    return mat


def object_z_axis(g, x, y):
    """The object's own up axis in the world (its length is the level's Z scale), or straight up (no stretch known)
    should the Transform node's settings not take."""
    up = g.node(unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    try:
        node = g.node(unreal.MaterialExpressionTransform, x + 250, y,
                      transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,
                      transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    except Exception as error:  # noqa: BLE001 - unstretched falls look the same without it
        unreal.log_warning(f'local-to-world transform not made ({error}); M_Waterfall ignores the level\'s Z stretch')
        return up
    g.link(up, '', node, '')
    return node


def depth_fade(g, distance, x, y):
    """0 where the surface touches what's behind it, 1 from distance cm on."""
    fade = g.node(unreal.MaterialExpressionDepthFade, x, y)
    g.link(distance, '', fade, 'FadeDistance')
    return fade


def build_waterfall():
    """M_Waterfall: falling water (FALL_LAYERS). Three streak taps on each covered pixel; the sheets are thin, so even a
    fall filling the view stays inside the contract's budget."""
    maps = water_maps('M_Waterfall', ('T_WaterStreaks',))
    if maps is None:
        return None
    mat = water_effect('M_Waterfall')
    g = Graph(mat)
    vertex = g.node(unreal.MaterialExpressionVertexColor, -1500, -100)
    shared = [
        ('UV', g.node(unreal.MaterialExpressionTextureCoordinate, -1500, -250, coordinate_index=0), ''),
        ('Foam', vertex, 'R'),
        ('Tex', map_object(g, 'StreakMap', maps['T_WaterStreaks'],
                           unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE, -1500, 50), ''),
        ('Time', g.node(unreal.MaterialExpressionTime, -1500, 200), ''),
        ('WorldPos', g.node(unreal.MaterialExpressionWorldPosition, -1500, 300,
                            world_position_shader_offset=unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS), ''),
        ('ObjectPos', g.node(unreal.MaterialExpressionObjectPositionWS, -1500, 400), ''),
        ('ZAxis', object_z_axis(g, -1800, 500), ''),
        ('Speed', g.scalar('Speed', 6.0, -1500, 600), ''),
        ('Length', g.scalar('Length', 12.0, -1500, 660), ''),
        ('Across', g.scalar('Across', 1.0, -1500, 720), ''),
    ]
    color = g.custom(FALL_COLOR, shared + [
        ('WaterColor', g.vector('WaterColor', srgb(FALL_BODY), -1100, -500), ''),
        ('FoamColor', g.vector('FoamColor', srgb(WATER_FOAM), -1100, -420), ''),
        ('FoamAmount', g.scalar('FoamAmount', 0.5, -1100, -340), ''),
        ('FoamSoft', g.scalar('FoamSoft', 0.15, -1100, -280), ''),
        ('Brightness', g.scalar('Brightness', 2.5, -1100, -220), ''),
        ('CloudTint', lighting_tint(g, 'CloudTint', -1500, -400), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -600, -200, 'M_Waterfall color')
    opacity = g.custom(FALL_OPACITY, shared + [
        ('Alpha', vertex, 'A'),
        ('Normal', g.node(unreal.MaterialExpressionVertexNormalWS, -1100, 250), ''),
        ('CameraVector', g.node(unreal.MaterialExpressionCameraVectorWS, -1100, 320), ''),
        ('Fade', depth_fade(g, g.scalar('DepthFadeDistance', 60.0, -1400, 900), -1100, 900), ''),
    ] + [(name, g.scalar(name, value, -1100, 400 + 60 * i), '') for i, (name, value) in enumerate((
        ('GapCalm', 0.12), ('GapBreak', 0.3), ('CutMax', 0.82), ('Soft', 0.08), ('Body', 0.55), ('EdgeFade', 0.6),
        ('Opacity', 0.9)))],
        unreal.CustomMaterialOutputType.CMOT_FLOAT1, -600, 300, 'M_Waterfall opacity')
    g.out(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    finish(mat, [])
    return mat


# Spray (M_WaterMist): billows rolling outward from BillowOrigin (UV 0, 0..1 over each card): two copies of the macro
# noise growing outward, each over one cycle (BillowRate a second), half a cycle apart, faded out as it starts again
# from a new part of the noise, so the spray billows out forever without stretching or repeating in step. The blend
# keeps the contrast of either copy.
MIST_OPACITY = """float2 C = (UV - BillowOrigin.xy) * BillowScale;
float PA = frac(Time * BillowRate);
float PB = frac(PA + 0.5);
float W = 1.0 - abs(1.0 - 2.0 * PA);
float2 SeedA = frac(floor(Time * BillowRate) * float2(0.37, 0.61));
float2 SeedB = frac(floor(Time * BillowRate + 0.5) * float2(0.53, 0.19));
float NA = Texture2DSample(Noise, NoiseSampler, C / (1.0 + PA * Grow) + SeedA).r;
float NB = Texture2DSample(Noise, NoiseSampler, C / (1.0 + PB * Grow) + SeedB).r;
float Puff = 0.5 + (NA * W + NB * (1.0 - W) - 0.5) * rsqrt(W * W + (1.0 - W) * (1.0 - W));
// Soft billows inside the card's own soft alpha (vertex A), never quite empty (Floor); fading where a card turns
// edge-on, near the camera (walking through spray doesn't white out the view) and where it meets what's behind it.
float Shape = smoothstep(PuffLow, PuffHigh, Puff);
float EdgeOn = saturate(abs(dot(normalize(Normal), CameraVector)) * 2.5);
float Near = saturate((length(WorldPos - CameraPos) - NearStart) / max(NearEnd - NearStart, 1.0));
return saturate(Alpha * lerp(Floor, 1.0, Shape) * EdgeOn * Near * Opacity) * Fade;"""
# Spray white, much brighter toward the sun (light scattered forward through it), unlit at the scene's brightness and
# dimmed at dusk by the cloud tint.
MIST_COLOR = """float Toward = pow(saturate(dot(-CameraVector, normalize(SunDirection + 1e-5))), 4.0);
return MistColor * Brightness * (1.0 + SunGlow * Toward) * CloudTint;"""


def build_water_mist():
    """M_WaterMist: the spray at a fall's foot and lip (MIST_OPACITY): two macro-noise taps a pixel, soft and faint, so
    it billows outward rather than falling like the old cards. Instanced usage too, so pooled cards (FX) may wear it."""
    mat = water_effect('M_WaterMist')
    g = Graph(mat)
    vertex = g.node(unreal.MaterialExpressionVertexColor, -1500, -100)
    camera = g.node(unreal.MaterialExpressionCameraVectorWS, -1500, 0)
    opacity = g.custom(MIST_OPACITY, [
        ('UV', g.node(unreal.MaterialExpressionTextureCoordinate, -1500, -250, coordinate_index=0), ''),
        ('Alpha', vertex, 'A'),
        ('Noise', g.node(unreal.MaterialExpressionTextureObjectParameter, -1500, 100, parameter_name='Noise',
                         texture=import_mask(MACRO_NOISE_FILE, MACRO_NOISE)), ''),
        ('Time', g.node(unreal.MaterialExpressionTime, -1500, 200), ''),
        ('Normal', g.node(unreal.MaterialExpressionVertexNormalWS, -1500, 300), ''),
        ('CameraVector', camera, ''),
        ('WorldPos', g.node(unreal.MaterialExpressionWorldPosition, -1500, 400), ''),
        ('CameraPos', g.node(unreal.MaterialExpressionCameraPositionWS, -1500, 500), ''),
        ('BillowOrigin', g.vector('BillowOrigin', (0.5, 0.5, 0.0, 0.0), -1100, 0), ''),
        ('Fade', depth_fade(g, g.scalar('DepthFadeDistance', 150.0, -1400, 1100), -1100, 1100), ''),
    ] + [(name, g.scalar(name, value, -1100, 100 + 60 * i), '') for i, (name, value) in enumerate((
        ('BillowScale', 0.8), ('BillowRate', 0.15), ('Grow', 1.0), ('PuffLow', 0.25), ('PuffHigh', 0.85),
        ('Floor', 0.2), ('NearStart', 60.0), ('NearEnd', 300.0), ('Opacity', 0.35)))],
        unreal.CustomMaterialOutputType.CMOT_FLOAT1, -600, 300, 'M_WaterMist opacity')
    color = g.custom(MIST_COLOR, [
        ('CameraVector', camera, ''),
        ('SunDirection', g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection, -1100, -400), ''),
        ('MistColor', g.vector('MistColor', srgb(WATER_FOAM), -1100, -300), ''),
        ('Brightness', g.scalar('Brightness', 1.6, -1100, -240), ''),
        ('SunGlow', g.scalar('SunGlow', 0.8, -1100, -180), ''),
        ('CloudTint', lighting_tint(g, 'CloudTint', -1500, -400), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -600, -200, 'M_WaterMist color')
    g.out(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    finish(mat, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES])
    return mat


# A gust leans a plume downwind, more the higher up it (MPC_Wind; the object's height rather than UV.y, whose direction on
# the cards is ambiguous). The dust devils share M_Smoke and lean too.
SMOKE_LEAN = """float H = saturate((WorldPos.z - ObjectPos.z) / max(Height, 1.0));
float2 GDir = normalize(GustDirection.xy + float2(0.0001, 0.0));
float Bend = GustStrength * GustLean * H * H;
return float3(GDir * Bend, -Bend * 0.25);"""


def build_smoke():
    """M_Smoke: soft chimney smoke drifting up its cards. A light grey rather than white, and fainter: against a deep
    afternoon sky a white plume over 1 read as bright streaks, not smoke. Unlit, so it follows the clouds' lighting tint
    (MPC_Lighting's CloudTint, white by day) and dims at dusk with them; a gust leans it (SMOKE_LEAN)."""
    mat = effect('M_Smoke',
                  [('Speed', lambda g: g.scalar('Speed', 0.12, -900, 200))],
                  'return SmokeColor * CloudTint;',
                  [('SmokeColor', lambda g: g.vector('SmokeColor', (0.78, 0.79, 0.82, 1.0), -900, 300)),
                   ('CloudTint', lambda g: lighting_tint(g, 'CloudTint', -900, 500))],
                  SMOKE_OPACITY,
                  [('Opacity', lambda g: g.scalar('Opacity', 0.28, -900, 400))])
    g = Graph(mat)
    lean = g.custom(SMOKE_LEAN, [
        ('WorldPos', g.node(unreal.MaterialExpressionWorldPosition, -900, 700,
                            world_position_shader_offset=unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS), ''),
        ('ObjectPos', g.node(unreal.MaterialExpressionObjectPositionWS, -900, 800), ''),
        ('GustStrength', wind_param(g, 'GustStrength', -900, 900), ''),
        ('GustDirection', wind_param(g, 'GustDirection', -900, 1000), ''),
        ('Height', g.scalar('PlumeHeight', 800.0, -900, 1100), ''),
        ('GustLean', g.scalar('GustLean', 250.0, -900, 1200), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, 800, 'Gust lean')
    g.out(lean, '', unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    finish(mat, [])
    return mat


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


# Sunbleached's screen pass, on the pre-exposed scene colour before depth of field: the lab's height fog over everything
# solid (its slFogAmount, in metres: Haze.x density per metre at Haze.z metres up, thinning by Haze.y per metre of
# height, from Haze.w metres out), cream and warming toward the sun, so the air itself glows; then its lavender
# vignette. The sky (far past anything solid) keeps the painted sky's own horizon haze.
BLEACH_POST = """float3 c = Color.rgb;
if (Depth.r < 1.0e6)
{
    float3 d = (WorldPos - CameraPos) * 0.01;
    float dist = length(d);
    float far = max(dist - Haze.w, 0.0);
    float k = Haze.y;
    float kdy = k * d.z * (far / max(dist, 1e-4));
    float lineInt = abs(kdy) > 1e-4 ? (1.0 - exp(-kdy)) / kdy : 1.0;
    float od = Haze.x * exp(-k * (CameraPos.z * 0.01 - Haze.z)) * far * lineInt;
    float amount = min(1.0 - exp(-max(od, 0.0)), HazeSun.z);
    float s = pow(saturate(dot(d / max(dist, 1e-4), normalize(SunDir + 1e-5))), HazeSun.y);
    c = lerp(c, lerp(HazeColor.rgb, HazeSunColor.rgb, saturate(s * HazeSun.x)), amount);
}
float2 vd = (ViewportUV - 0.5) * float2(lerp(View.ViewSizeAndInvSize.x * View.ViewSizeAndInvSize.w, 1.0, Vignette.z), 1.0);
float vv = smoothstep(1.0 - Vignette.y, 1.0 + Vignette.y * 0.2, length(vd) * 1.4142);
return lerp(c, VignetteColor.rgb, vv * Vignette.x);"""


def scene_texture(g, name, x, y):
    """A SceneTexture node (its id by the enum's Python name, whose spelling has varied)."""
    ids = unreal.SceneTextureId
    wanted = {'PostProcessInput0': ('PPI_POST_PROCESS_INPUT0', 'PPI_POSTPROCESSINPUT0'),
              'SceneDepth': ('PPI_SCENE_DEPTH', 'PPI_SCENEDEPTH')}
    for spelling in wanted[name]:
        if hasattr(ids, spelling):
            return g.node(unreal.MaterialExpressionSceneTexture, x, y, scene_texture_id=getattr(ids, spelling))
    raise RuntimeError(f'no SceneTextureId for {name} (tried {wanted[name]})')


def build_sunbleached_post():
    """M_SunbleachedPost: Sunbleached's screen pass (BLEACH_POST). Only the tutorial island's style volume wears an
    instance of it (Tools/Unreal/build_island_style.py sunbleached)."""
    mat = material('M_SunbleachedPost')
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
    mat.set_editor_property('blendable_location', unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_DOF)
    g = Graph(mat)
    d = style_numbers('sunbleached').POST_DEFAULTS
    v = {name: svector(g, SUNBLEACHED, name, value, -1000, 400 + 60 * i)
         for i, (name, value) in enumerate(d['vectors'].items())}
    color = g.custom(BLEACH_POST, [
        ('Color', scene_texture(g, 'PostProcessInput0', -1000, -300), 'Color'),
        ('Depth', scene_texture(g, 'SceneDepth', -1000, -150), 'Color'),
        ('WorldPos', g.node(unreal.MaterialExpressionWorldPosition, -1000, 0), ''),
        ('CameraPos', g.node(unreal.MaterialExpressionCameraPositionWS, -1000, 75), ''),
        ('ViewportUV', g.node(unreal.MaterialExpressionScreenPosition, -1000, 150), 'ViewportUV'),
        ('SunDir', g.node(unreal.MaterialExpressionSkyAtmosphereLightDirection, -1000, 250), ''),
        ('Haze', v['BleachHaze'], 'RGBA'), ('HazeSun', v['BleachHazeSun'], 'RGBA'),
        ('HazeColor', v['BleachHazeColor'], 'RGBA'), ('HazeSunColor', v['BleachHazeSunColor'], 'RGBA'),
        ('Vignette', v['BleachVignette'], 'RGBA'), ('VignetteColor', v['BleachVignetteColor'], 'RGBA'),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 0, 'Sunbleached screen pass')
    g.out(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(mat, [])
    return mat


MARK_TEXTURES = 'C:/Dev/AI_Looter_Shooter/Art/Textures/BulletMarks'
# The decal's colour picks the mark's cell in the atlas (make_bullet_marks.py: COLUMNS x ROWS cells, VARIANTS per style
# side by side): R carries the style ((style + 0.5) / 16, FBulletMarks::Code), G the seed.
MARK_CELL_UV = """float Style = floor(Code.r * 16.0);
float Cell = Style * 2.0 + (Code.g >= 0.5 ? 1.0 : 0.0);
float2 Corner = float2(fmod(Cell, 8.0), floor(Cell / 8.0));
return (Corner + saturate(UV)) / float2(8.0, 4.0);"""
# Only on surfaces facing back up the decal's way (Axis, its X, points into the surface): never the back of a thin board
# inside its box, nor a face the mark would only smear along. The surface's facing comes from the depth buffer's own
# positions (camera-relative, for their precision), turned toward the eye.
MARK_FACING = """float3 N = normalize(cross(ddx(P), ddy(P)));
N *= dot(N, V) < 0.0 ? -1.0 : 1.0;
return smoothstep(0.2, 0.5, dot(N, -normalize(Axis)));"""
MARK_OPACITY = 'return saturate(Alpha * Facing * Life * Code.a * Strength);'
MARK_NORMAL = 'return normalize(float3(N.xy * Strength, N.z));'


def mark_texture(kind):
    """One of the marks' atlas maps (make_bullet_marks.py writes them) at /Game/Art/Textures/BulletMarks: the colour (sRGB,
    its alpha the opacity), the normal map, or the ORM (linear masks); clamped at the atlas's edges. None while the PNG is
    missing."""
    import os
    file = f'{MARK_TEXTURES}/T_BulletMarks_{kind}.png'
    if not os.path.isfile(file):
        return None
    path = f'/Game/Art/Textures/BulletMarks/T_BulletMarks_{kind}'
    texture = import_texture(file, path, {'BC': 'BC', 'N': 'N'}.get(kind, 'M'))
    if kind == 'ORM':
        texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
    texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return path


def build_bullet_mark():
    maps = {kind: mark_texture(kind) for kind in ('BC', 'N', 'ORM')}
    missing = [kind for kind, path in maps.items() if path is None]
    if missing:
        # A stand-in would print white squares on every wall: no material, and the game leaves no marks until there is one.
        unreal.log_error(f'M_BulletMark not built: the atlas ({", ".join(missing)}) is missing from {MARK_TEXTURES}; run '
                         'Tools\\artrun.ps1 -Script Art\\Textures\\BulletMarks\\make_bullet_marks.py first')
        return None
    mat = material('M_BulletMark')
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    g = Graph(mat)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1800, 0, coordinate_index=0)
    # Its RGBA output: the first (unnamed) one is RGB only, and the opacity reads the decal colour's alpha (Code.a).
    code = g.node(unreal.MaterialExpressionDecalColor, -1800, 200)
    uv = g.custom(MARK_CELL_UV, [('UV', coords, ''), ('Code', code, 'RGBA')],
                  unreal.CustomMaterialOutputType.CMOT_FLOAT2, -1500, 100, 'Mark cell')
    bc = g.texture('MarkColor', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, maps['BC'], uv, -1100, -200)
    nrm = g.texture('MarkNormal', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, maps['N'], uv, -1100, 100)
    orm = g.texture('MarkORM', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, maps['ORM'], uv, -1100, 400)
    world = g.node(unreal.MaterialExpressionWorldPosition, -1500, 650)
    try:
        world.set_editor_property('world_position_shader_offset', unreal.WorldPositionIncludedOffsets.WPT_CAMERA_RELATIVE)
    except Exception as error:  # noqa: BLE001 - the absolute position works too, a little less precisely far from the origin
        unreal.log_warning(f'M_BulletMark: camera-relative world position not set ({error}); using the absolute one')
    facing = g.custom(MARK_FACING, [
        ('P', world, ''), ('V', g.node(unreal.MaterialExpressionCameraVectorWS, -1500, 750), ''),
        ('Axis', g.node(unreal.MaterialExpressionObjectOrientation, -1500, 850), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1100, 700, 'Facing')
    life = g.node(unreal.MaterialExpressionDecalLifetimeOpacity, -1100, 850)
    opacity = g.custom(MARK_OPACITY, [
        ('Alpha', bc, 'A'), ('Facing', facing, ''), ('Life', life, ''), ('Code', code, 'RGBA'),
        ('Strength', g.scalar('Opacity', 1.0, -1100, 950), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -700, 600, 'Opacity')
    color = g.mul(bc, 'RGB', g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1100, -400), '', -700, -300)
    normal = g.custom(MARK_NORMAL, [('N', nrm, 'RGB'), ('Strength', g.scalar('NormalStrength', 1.0, -1100, 250), '')],
                      unreal.CustomMaterialOutputType.CMOT_FLOAT3, -700, 100, 'Normal strength')
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    g.out(normal, '', unreal.MaterialProperty.MP_NORMAL)
    g.out(orm, 'G', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(orm, 'B', unreal.MaterialProperty.MP_METALLIC)
    finish(mat, [])
    return mat


# --- Characters (the player's Ellis, 2026-10-15: the user found him "plastic" on M_World) ---
# One baked set feeds several slots: each part kind's slot names its master (Art/Models/Characters/Ellis.py's
# GAME_MATERIALS). The masters share M_World's maps; what they add:
#   M_Character       leather, metal, hair, wood: the maps' roughness scaled and offset (RoughnessScale, RoughnessOffset),
#                     broken up by a fine noise (RoughnessBreakup: no smooth plastic sheen across a whole part),
#                     NormalStrength, Specular, DiffuseAO (stronger than the world's: the bake's folds read).
#   M_CharacterCloth  the engine's Cloth shading: a fuzz of fibres lit at grazing angles (its fuzz colour: the base
#                     colour lightened toward FuzzTint by FuzzLift, times FuzzStrength, 0 none), the same roughness
#                     controls. (The Cloth amount, Custom Data 0, can't be wired from Python: unwired it's 1, all cloth.)
#   M_CharacterSkin   Subsurface Profile shading (SP_Skin, the engine's skin defaults): light scattering under the skin
#                     (SubsurfaceAmount), skin's 2.8% reflectance (Specular 0.35).
#   M_CharacterHair   hair cards (Ellis round 6; the atlas from Tools/Blender/ellis_hair.py): masked, two-sided, the
#                     engine's Hair shading (strands lit along their tangent: a highlight band across the hair, a warm
#                     second one through it). The colour map's alpha is the strands' coverage, raised as the map mips
#                     down (MipBoost per level: a coarser level averages strands with their gaps, and the mask thinned
#                     until the hair went bald a few metres off) and dithered (DitherTemporalAA, clip 0.5: once TAA
#                     blends, a texel covers as much as its alpha; a hard clip aliased into stair-stepped strands). The
#                     ORM's blue is each strand's random id, NOT metal: it tips the strand's tangent (TangentJitter) so
#                     the highlight breaks along the strands. Scatter (the Hair model reads it from the Metallic pin),
#                     Specular, RoughnessScale/Offset, DiffuseAO, Tint. Backlit (Custom Data 0) can't be wired from
#                     Python: unwired it is 1. HAIR_SHADING = MSM_DEFAULT_LIT is the fallback if the Hair model looks
#                     wrong on Medium: the strands' normal map then lights it, back faces on the authored normal.

CHARACTER_BREAKUP = """// A fine, two-scale noise in UV space: the roughness wanders a little, as real wear and handling leave it.
float N = Texture2DSample(Noise, NoiseSampler, UV * 7.0).r * 0.6 + Texture2DSample(Noise, NoiseSampler, UV * 23.0 + 0.41).r * 0.4;
return saturate(Rough * Scale + Offset + (N - 0.5) * Breakup);"""

CHARACTER_NORMAL = """// The map's normal toward flat by Strength (0 flat, 1 as baked, above 1 deeper).
float3 N = float3(Map.xy * Strength, Map.z);
return normalize(N);"""

CLOTH_FUZZ = """// Fibres catch the light paler and less saturated than the dyed cloth under them.
float Luma = dot(Color, float3(0.299, 0.587, 0.114));
float3 Pale = lerp(Color, float3(Luma, Luma, Luma), 0.35);
return saturate(lerp(Pale, Tint, Lift) * Strength);"""


def macro_noise():
    """The shared macro noise (T_MacroNoise_M), imported once."""
    if not unreal.EditorAssetLibrary.does_asset_exist(MACRO_NOISE):
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', MACRO_NOISE_FILE)
        task.set_editor_property('destination_path', '/Game/Art/Textures/MacroNoise')
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return MACRO_NOISE


def character_surface(g, orm_default, uv_x=-1600):
    """The maps and the shared outputs of the character masters: base colour (tinted, DiffuseAO), normal (strength),
    roughness (scale, offset, breakup), metallic, occlusion. Returns (color, bc)."""
    bc, nrm, orm, vc, ao = textured_inputs(g, orm_default)
    tint = g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1100, -500)
    tinted = g.mul(bc, 'RGB', tint, '', -800, -400)
    lerp = g.node(unreal.MaterialExpressionLinearInterpolate, -800, -150, const_a=1.0)
    g.link(ao, '', lerp, 'B')
    g.link(g.scalar('DiffuseAO', 0.7, -1100, -150), '', lerp, 'Alpha')
    color = g.mul(tinted, '', lerp, '', -500, -300)
    noise = g.node(unreal.MaterialExpressionTextureObject, -1100, 900, texture=unreal.load_asset(macro_noise()))
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1100, 1000, coordinate_index=0)
    rough = g.custom(CHARACTER_BREAKUP, [
        ('Rough', orm, 'G'), ('Noise', noise, ''), ('UV', coords, ''),
        ('Scale', g.scalar('RoughnessScale', 1.0, -800, 700), ''),
        ('Offset', g.scalar('RoughnessOffset', 0.0, -800, 800), ''),
        ('Breakup', g.scalar('RoughnessBreakup', 0.12, -800, 900), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -400, 600, 'Roughness')
    normal = g.custom(CHARACTER_NORMAL, [('Map', nrm, 'RGB'), ('Strength', g.scalar('NormalStrength', 1.0, -800, 100), '')],
                      unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, 50, 'Normal strength')
    g.out(normal, '', unreal.MaterialProperty.MP_NORMAL)
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(orm, 'B', unreal.MaterialProperty.MP_METALLIC)
    g.out(ao, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    return color, bc


CHARACTER_USAGES = [unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH]


def build_character(orm_default):
    mat = material('M_Character')
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    g = Graph(mat)
    color, _ = character_surface(g, orm_default)
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(g.scalar('Specular', 0.5, -400, -150), '', unreal.MaterialProperty.MP_SPECULAR)
    finish(mat, CHARACTER_USAGES)
    return mat


def build_character_cloth(orm_default):
    mat = material('M_CharacterCloth')
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_CLOTH)
    g = Graph(mat)
    color, bc = character_surface(g, orm_default)
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(g.scalar('Specular', 0.35, -400, -150), '', unreal.MaterialProperty.MP_SPECULAR)
    fuzz = g.custom(CLOTH_FUZZ, [
        ('Color', color, ''),
        ('Tint', g.vector('FuzzTint', (0.62, 0.58, 0.52, 1.0), -800, -800), ''),
        ('Lift', g.scalar('FuzzLift', 0.25, -800, -700), ''),
        ('Strength', g.scalar('FuzzStrength', 1.2, -800, -600), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, -700, 'Fuzz colour')
    # The Cloth shading model reads its fuzz colour from Subsurface Color.
    g.out(fuzz, '', unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    finish(mat, CHARACTER_USAGES)
    return mat


SKIN_PROFILE = '/Game/Art/Materials/Masters/SP_Skin'


def skin_profile():
    """The engine's skin scattering (its profile's defaults are tuned for skin), made once."""
    if unreal.EditorAssetLibrary.does_asset_exist(SKIN_PROFILE):
        return unreal.load_asset(SKIN_PROFILE)
    profile = unreal.AssetToolsHelpers.get_asset_tools().create_asset('SP_Skin', FOLDER, unreal.SubsurfaceProfile,
                                                                      unreal.SubsurfaceProfileFactory())
    unreal.EditorAssetLibrary.save_loaded_asset(profile)
    return profile


def build_character_skin(orm_default):
    mat = material('M_CharacterSkin')
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_SUBSURFACE_PROFILE)
    mat.set_editor_property('subsurface_profile', skin_profile())
    g = Graph(mat)
    color, _ = character_surface(g, orm_default)
    g.out(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(g.scalar('Specular', 0.35, -400, -150), '', unreal.MaterialProperty.MP_SPECULAR)
    # Subsurface Profile reads how much light scatters under the skin from Opacity.
    g.out(g.scalar('SubsurfaceAmount', 1.0, -400, -500), '', unreal.MaterialProperty.MP_OPACITY)
    finish(mat, CHARACTER_USAGES)
    return mat


HAIR_SHADING = unreal.MaterialShadingModel.MSM_HAIR

HAIR_COVERAGE = """// The strands' coverage (the colour map's alpha), raised by the mip level the pixel samples: a coarser level averages
// strands with the gaps between them, so the mask thinned with distance and the hair went bald a few metres off.
float2 T = UV * Size;
float2 Dx = ddx(T);
float2 Dy = ddy(T);
float Level = max(0.0, 0.5 * log2(max(max(dot(Dx, Dx), dot(Dy, Dy)), 1e-8)));
return saturate(Alpha * (1.0 + Level * Boost));"""

HAIR_TANGENT = """// Along the strands (the cards' V runs from root to tip), each strand tipped toward or away from the card by its random
// id (the ORM's blue): the highlight breaks along the strands instead of one smooth band across a card.
return normalize(float3(0.0, 1.0, (Id - 0.5) * Jitter));"""

HAIR_ROUGH = 'return saturate(Rough * Scale + Offset);'


def build_character_hair(orm_default):
    mat = material('M_CharacterHair')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided', True)
    mat.set_editor_property('opacity_mask_clip_value', 0.5)
    mat.set_editor_property('shading_model', HAIR_SHADING)
    g = Graph(mat)
    bc, nrm, orm, vc, ao = textured_inputs(g, orm_default)
    tinted = g.mul(bc, 'RGB', g.vector('Tint', (1.0, 1.0, 1.0, 1.0), -1100, -500), '', -800, -400)
    lerp = g.node(unreal.MaterialExpressionLinearInterpolate, -800, -150, const_a=1.0)
    g.link(ao, '', lerp, 'B')
    g.link(g.scalar('DiffuseAO', 0.7, -1100, -150), '', lerp, 'Alpha')
    g.out(g.mul(tinted, '', lerp, '', -500, -300), '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = g.custom(HAIR_ROUGH, [
        ('Rough', orm, 'G'),
        ('Scale', g.scalar('RoughnessScale', 1.0, -800, 700), ''),
        ('Offset', g.scalar('RoughnessOffset', 0.0, -800, 800), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -400, 600, 'Roughness')
    g.out(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(g.scalar('Specular', 0.45, -400, -150), '', unreal.MaterialProperty.MP_SPECULAR)
    g.out(ao, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    coverage = g.custom(HAIR_COVERAGE, [
        ('Alpha', bc, 'A'),
        ('UV', g.node(unreal.MaterialExpressionTextureCoordinate, -800, 1100, coordinate_index=0), ''),
        ('Size', g.scalar('AtlasSize', 2048.0, -800, 1200), ''),
        ('Boost', g.scalar('MipBoost', 0.35, -800, 1300), ''),
    ], unreal.CustomMaterialOutputType.CMOT_FLOAT1, -400, 1100, 'Coverage kept as it mips')
    # DitherTemporalAA is an engine material function (no expression class of its own): the mask's soft edge dithers over
    # frames instead of aliasing at the clip. Without it (an engine without the function) the coverage clips as it is.
    dither_function = unreal.load_asset('/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA')
    mask = coverage
    if dither_function:
        dither = g.node(unreal.MaterialExpressionMaterialFunctionCall, -100, 1100)
        dither.set_editor_property('material_function', dither_function)
        if any(MEL.connect_material_expressions(coverage, '', dither, pin) for pin in ('AlphaThreshold', 'Alpha Threshold', '')):
            mask = dither
        else:
            unreal.log_warning('M_CharacterHair: could not connect the coverage to DitherTemporalAA; the mask clips undithered')
    else:
        unreal.log_warning('M_CharacterHair: no DitherTemporalAA function in this engine; the mask clips undithered')
    g.out(mask, '', unreal.MaterialProperty.MP_OPACITY_MASK)
    if HAIR_SHADING == unreal.MaterialShadingModel.MSM_HAIR:
        # the Hair model's Scatter is the Metallic pin; its strand direction the Tangent pin
        g.out(g.scalar('Scatter', 0.2, -400, 300), '', unreal.MaterialProperty.MP_METALLIC)
        tangent = g.custom(HAIR_TANGENT, [('Id', orm, 'B'), ('Jitter', g.scalar('TangentJitter', 0.5, -800, 200), '')],
                           unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, 150, 'Strand tangent')
        g.out(tangent, '', unreal.MaterialProperty.MP_TANGENT)
    else:
        # lit cards: the strands' normal map, back faces kept on the authored normal (as M_WorldFoliage's leaves)
        normal = g.custom('return normalize(float3(Map.xy * Strength, Map.z)) * Sign;', [
            ('Map', nrm, 'RGB'), ('Strength', g.scalar('NormalStrength', 1.0, -800, 100), ''),
            ('Sign', g.node(unreal.MaterialExpressionTwoSidedSign, -800, 0), ''),
        ], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, 50, 'Normal, both sides')
        g.out(normal, '', unreal.MaterialProperty.MP_NORMAL)
    finish(mat, CHARACTER_USAGES)
    return mat


BUILDERS = {'M_World': lambda: build_world(DEFAULT_ORM), 'M_Gun': lambda: build_gun(DEFAULT_ORM),
            'M_Character': lambda: build_character(DEFAULT_ORM), 'M_CharacterCloth': lambda: build_character_cloth(DEFAULT_ORM),
            'M_CharacterSkin': lambda: build_character_skin(DEFAULT_ORM),
            'M_CharacterHair': lambda: build_character_hair(DEFAULT_ORM),
            'M_WorldFoliage': lambda: build_foliage(DEFAULT_ORM),
            'M_Terrain': build_terrain, 'M_Water': build_water, 'M_SkyClouds': build_sky_clouds,
            'M_Waterfall': build_waterfall, 'M_WaterMist': build_water_mist, 'M_Smoke': build_smoke,
            'M_Glass': build_glass, 'M_Gel': build_gel,
            'M_Backdrop': build_backdrop, 'M_GravewindWisp': build_gravewind_wisp, 'M_CanyonFog': build_canyon_fog,
            'M_SunbleachedPost': build_sunbleached_post, 'M_BulletMark': build_bullet_mark}
wanted = [name for name in sys.argv[1:] if name in BUILDERS] or list(BUILDERS)
orm = default_orm()
built = [made for made in (BUILDERS[name]() for name in wanted) if made is not None]
unreal.log('LOOTER world materials: ' + ', '.join(m.get_path_name() for m in built))
