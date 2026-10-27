"""Builds the main menu's backdrop materials, Teropa from orbit (Docs/Story.md; the scene: build_menu_teropa.py), in
/Game/Art/Materials/Menu, and imports their maps (Art/Textures/MenuTeropa/make_teropa_maps.py makes them) into
/Game/Art/Textures/MenuTeropa:

  M_Planet     unlit, translucent, on a proxy sphere a little bigger than the planet (the engine's sphere: its facets
               never show, as the planet is drawn by the pixel, not the mesh). Each pixel casts the eye's ray at a true
               sphere of PlanetRadius round the actor's centre: a clean round limb at any size, antialiased over one
               pixel. On the disc: the ground map (SurfaceMap: colour, the sea's glint in alpha) turning about the
               planet's North axis at Spin.x degrees a second (Spin.z where it starts), relief from ReliefMap's slopes
               (R east, G north; ReliefStrength), lit by the sun (SunDirection, SunColor) with a soft terminator warmed
               toward sunset (TerminatorColor), the sea's sun glint (SeaLook), cloud cover (CloudMap) on its own shell
               a little higher (CloudHeight: parallax at the limb), turning at its own speed (Spin.y, Spin.w) and
               shadowing the ground toward the sun, the night side's lights (ReliefMap B, LightsColor) dimmed under the
               clouds, and the atmosphere: a blue rim haze, brighter on the lit side and toward the sun. Off the disc:
               the atmosphere's halo, a thin glowing shell (AtmosphereHeight) lit on the sun's side and blazing when the
               sun stands behind it (forward scattering). Both carry the lens's wide glare toward the sun (Glare, as
               M_Starfield, so the sky and the planet agree where they meet). Its pixel derivatives are taken from the
               unwrapped map position, so neither the date line nor the turning map's wrap shows a seam.
               The moon wears it too (MI_Planet_Moon: no clouds, lights, glint or air).
  M_Starfield  unlit, opaque, two-sided, on a big sphere round the camera: three layers of stars hashed from the view
               direction (about a pixel wide at any resolution, warm and cool, a few twinkling), denser along the
               galaxy's band (NebulaMap, in the sky's own frame: SkyNorth, SkyEast), the sun's disc, glow and glare.

Instances (the look; build_menu_teropa.py sets the scene's own numbers on them: PlanetRadius, SunDirection, North,
East, Facing, SkyNorth, SkyEast): MI_Planet_Teropa, MI_Planet_Moon, MI_Starfield_Teropa. Packed vectors, as named:
  Shape (CloudHeight, AtmosphereHeight: shares of the radius; ReliefStrength; unused)
  Spin (ground and cloud degrees a second; ground and cloud start, degrees: the start puts the Reaches at the terminator)
  AtmosphereLook (RimStrength, RimPower, HaloBrightness, HaloOpacity)
  AtmosphereShape (HaloFalloff, ForwardStrength, ForwardPower, DayHaze)
  Glare (Strength, Power), NightLook (LightsIntensity, TerminatorWidth, TerminatorWarmth)
  CloudLook (Opacity, ShadowStrength, ShadowShift, Brightness), SeaLook (GlintStrength, GlintPower)
  Stars (Scale: cells per unit of the first layer, Share of cells with a star, BandBoost, MagnitudePower)
  StarLook (NebulaBrightness, StarBrightness, Twinkle), Sun (Radius rad, DiscBrightness, GlowWidth rad, GlowBrightness)

Cost (1080p, the planet about half the screen): one Custom node a pixel, four texture reads on the planet, one on the
sky; no lights, shadows or Lumen: well under the menu's 4 ms with the post (bloom, TSR).

Re-running keeps every asset (instances stay linked) and rebuilds the graphs; a texture is reimported in place. Run in
the open editor (nothing playing), after make_teropa_maps.py:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_menu_materials.py"
"""
import os

import unreal

FOLDER = '/Game/Art/Materials/Menu'
TEXTURE_FOLDER = '/Game/Art/Textures/MenuTeropa'
SOURCE = 'C:/Dev/AI_Looter_Shooter/Art/Textures/MenuTeropa'
MEL = unreal.MaterialEditingLibrary
CMOT = unreal.CustomMaterialOutputType

# name -> (sRGB, compression). The ground's and the moon's maps keep their fine detail in BC7; the clouds one grey
# channel, uncompressed (soft gradients band in block compression).
TEXTURES = {
    'T_Teropa_BC': (True, unreal.TextureCompressionSettings.TC_BC7),
    'T_Teropa_M': (False, unreal.TextureCompressionSettings.TC_BC7),
    'T_TeropaClouds_M': (False, unreal.TextureCompressionSettings.TC_GRAYSCALE),
    'T_TeropaMoon_BC': (True, unreal.TextureCompressionSettings.TC_BC7),
    'T_TeropaMoon_M': (False, unreal.TextureCompressionSettings.TC_BC7),
    'T_MenuNebula_BC': (True, unreal.TextureCompressionSettings.TC_BC7),
}


def log(message):
    unreal.log(f'Menu materials: {message}')


def import_textures():
    """Imports (or reimports in place) the maps. Equirectangular: they wrap east-west and clamp at the poles. Never
    streamed: the menu shows them whole the moment it opens, and they are only the menu's (about 30 MB)."""
    tasks = []
    for name in TEXTURES:
        source = f'{SOURCE}/{name}.png'
        if not os.path.exists(source):
            raise RuntimeError(f'{source} is missing: run Art/Textures/MenuTeropa/make_teropa_maps.py first')
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', source)
        task.set_editor_property('destination_path', TEXTURE_FOLDER)
        task.set_editor_property('destination_name', name)
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    textures = {}
    for name, (srgb, compression) in TEXTURES.items():
        texture = unreal.load_asset(f'{TEXTURE_FOLDER}/{name}')
        if texture is None:
            raise RuntimeError(f'{TEXTURE_FOLDER}/{name} did not import')
        texture.set_editor_property('srgb', srgb)
        texture.set_editor_property('compression_settings', compression)
        texture.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP)
        texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
        texture.set_editor_property('never_stream', True)
        unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
        textures[name] = texture
    log(f'{len(textures)} maps imported into {TEXTURE_FOLDER}')
    return textures


# --- Graph helpers (the style of build_world_materials.py's) ---

def clear_graph(mat):
    """Empties a master's graph to build it again in place (LooterEditor's ClearMaterialGraph when there, as
    build_world_materials.py does: the engine's delete can crash on nodes loaded as the editor started)."""
    tools = getattr(unreal, 'LooterMaterialGraphTools', None)
    if tools is None:
        MEL.delete_all_material_expressions(mat)
        return
    tools.clear_material_graph(mat)


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

    def scalar(self, name, value, x, y, group):
        n = self.node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=float(value))
        n.set_editor_property('group', group)
        return n

    def vector(self, name, value, x, y, group):
        values = [float(v) for v in value][:4]
        values += [1.0] * (4 - len(values))
        n = self.node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                      default_value=unreal.LinearColor(*values))
        n.set_editor_property('group', group)
        return n

    def texture_object(self, name, texture, x, y, group):
        n = self.node(unreal.MaterialExpressionTextureObjectParameter, x, y, parameter_name=name, texture=texture)
        n.set_editor_property('group', group)
        return n

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


def finish(mat):
    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)


def time_node(g, x, y):
    """Game time that runs on while paused: the planet keeps turning behind any pause the menu might take."""
    t = g.node(unreal.MaterialExpressionTime, x, y)
    t.set_editor_property('ignore_pause', True)
    return t


# --- M_Planet ---

def lat_long(vec, spin, tag):
    """HLSL: the map position of a direction on the planet (its own frame: Fr toward the camera's meridian, Rt east,
    Up north), the map slid west by spin (turns), and its screen derivatives with the wrap taken out (the date line
    behind the planet and the turning map's wrap would otherwise pick the smallest mip in a one-pixel seam)."""
    return f"""
float lon{tag} = atan2(dot({vec}, Rt), dot({vec}, Fr));
float lat{tag} = asin(clamp(dot({vec}, Up), -1.0, 1.0));
float2 uv{tag} = float2(lon{tag} / (2.0 * Pi) + 0.5 - {spin}, 0.5 - lat{tag} / Pi);
float2 dx{tag} = ddx(uv{tag}); dx{tag}.x -= round(dx{tag}.x);
float2 dy{tag} = ddy(uv{tag}); dy{tag}.x -= round(dy{tag}.x);"""


PLANET_CODE = """const float Pi = 3.14159265;
float R = max(PlanetRadius, 1.0);
float3 O = CameraPos - Center;
float3 D = normalize(WorldPos - CameraPos);
float3 L = normalize(SunDirection);
float3 Up = normalize(North);
float3 Rt = normalize(East - Up * dot(East, Up));
float3 Fr = normalize(Facing - Up * dot(Facing, Up) - Rt * dot(Facing, Rt));

// The eye's ray against the sphere: P its closest approach to the centre, d how close. On the disc n is where it
// meets the ground; off it, the limb's normal (so the edge pixels shade as the limb does).
float b = dot(O, D);
float3 P = O - D * b;
float d = length(P);
float cover = saturate((R - d) / max(fwidth(d), 0.001) + 0.5);
float3 n = normalize(O + D * (-b - sqrt(max(R * R - d * d, 0.0))));
float Rc = R * (1.0 + Shape.x);
float3 nc = normalize(O + D * (-b - sqrt(max(Rc * Rc - d * d, 0.0))));
float3 ns = normalize(n + L * CloudLook.z);
float spinGround = frac((Time * Spin.x + Spin.z) / 360.0);
float spinCloud = frac((Time * Spin.y + Spin.w) / 360.0);
""" + lat_long('n', 'spinGround', 'G') + lat_long('nc', 'spinCloud', 'C') + lat_long('ns', 'spinCloud', 'S') + """
float4 ground = Texture2DSampleGrad(SurfaceMap, SurfaceMapSampler, uvG, dxG, dyG);
float4 relief = Texture2DSampleGrad(ReliefMap, ReliefMapSampler, uvG, dxG, dyG);
float cloud = Texture2DSampleGrad(CloudMap, CloudMapSampler, uvC, dxC, dyC).r;
float shadow = Texture2DSampleGrad(CloudMap, CloudMapSampler, uvS, dxS, dyS).r;

// Relief: the map's slopes east and north tip the ground's normal (the escarpments and ranges catch the low sun).
float3 across = cos(lonG) * Fr + sin(lonG) * Rt;
float3 eastT = -sin(lonG) * Fr + cos(lonG) * Rt;
float3 northT = Up * cos(latG) - across * sin(latG);
float2 slope = relief.rg * 2.0 - 1.0;
float3 nr = normalize(n - (eastT * slope.x + northT * slope.y) * Shape.z);

// The sun on the ground: a soft terminator, warm toward sunset, the clouds' shadows, the sea's glint.
float ndl = dot(n, L);
float day = smoothstep(-0.04, 0.14, ndl);
float warm = exp(-(ndl * ndl) / max(NightLook.y * NightLook.y, 0.0001));
float3 sun = SunColor * lerp(float3(1.0, 1.0, 1.0), TerminatorColor, warm * NightLook.z);
float shade = 1.0 - shadow * CloudLook.y * day;
float3 col = ground.rgb * (sun * saturate(dot(nr, L)) * day * shade + AmbientColor);
col += sun * pow(saturate(dot(reflect(D, n), L)), max(SeaLook.y, 1.0)) * SeaLook.x * ground.a * day * shade;

// Clouds on their own shell, lit a little round the terminator.
float ndlC = dot(nc, L);
float cloudA = saturate(cloud * CloudLook.x);
float3 cloudCol = CloudLook.w * sun * saturate(ndlC * 0.85 + 0.15) * smoothstep(-0.12, 0.1, ndlC) + AmbientColor * 0.8;
col = lerp(col, cloudCol, cloudA);

// The night side's lights, through thin cloud only.
float night = smoothstep(0.06, -0.14, ndl);
col += LightsColor * (relief.b * relief.b * NightLook.x * night * (1.0 - cloudA * 0.8));

// The air: a blue haze thickening to the rim, lit on the sun's side and toward the sun; a little haze by day.
float rim = pow(1.0 - saturate(dot(n, -D)), max(AtmosphereLook.y, 0.1)) * AtmosphereLook.x;
float airLit = smoothstep(-0.3, 0.35, ndl);
float forward = pow(saturate(dot(D, L)), max(AtmosphereShape.z, 0.1)) * AtmosphereShape.y;
float3 air = Atmosphere * SunColor;
col = lerp(col, air * airLit * 0.6, saturate(rim * 0.5));
col += air * (rim * (airLit + forward) + AtmosphereShape.w * day);

// Off the disc: the halo, a thin shell fading outward, lit on the sun's side, blazing with the sun behind it.
float h = (d - R) / max(R * Shape.y, 1.0);
float dens = exp(-max(h, 0.0) * AtmosphereShape.x) * saturate(1.0 - h);
float haloLit = smoothstep(-0.35, 0.3, dot(P / max(d, 0.001), L));
float3 haloCol = air * (haloLit + forward) * AtmosphereLook.z;
float haloA = saturate(dens * saturate(haloLit + forward) * AtmosphereLook.w);

// The lens's glare toward the sun over both (M_Starfield adds the same behind, so a half-covered pixel sums right).
float3 glare = SunColor * pow(saturate(dot(D, L)), max(Glare.y, 0.1)) * Glare.x;
float A = cover + haloA * (1.0 - cover);
float3 C = ((col + glare) * cover + (haloCol + glare) * haloA * (1.0 - cover)) / max(A, 0.0001);
return float4(C, A);"""

# The Teropa look (Docs/Story.md: an earth-like world; dusty air pales its seas and warms its sunsets).
PLANET_LOOK = dict(
    scalars={'PlanetRadius': 20000.0},
    vectors={
        'Shape': (0.006, 0.035, 0.5, 0.0),
        'Spin': (1.0, 1.18, -20.0, 10.0),
        'SunColor': (1.35, 1.28, 1.15),
        'TerminatorColor': (1.4, 0.7, 0.4),
        'NightLook': (2.2, 0.16, 0.55, 0.0),
        'LightsColor': (1.0, 0.64, 0.3),
        'AmbientColor': (0.008, 0.01, 0.016),
        'CloudLook': (0.95, 0.45, 0.004, 1.05),
        'SeaLook': (0.9, 80.0, 0.0, 0.0),
        'Atmosphere': (0.3, 0.56, 1.0),
        'AtmosphereLook': (1.6, 3.0, 1.4, 0.9),
        'AtmosphereShape': (4.0, 1.2, 6.0, 0.03),
        'Glare': (0.9, 4.0, 0.0, 0.0),
        # The scene's (build_menu_teropa.py sets them): defaults for a planet ahead of an unturned camera.
        'SunDirection': (0.2, 0.9, 0.38),
        'North': (0.0, 0.0, 1.0),
        'East': (0.0, 1.0, 0.0),
        'Facing': (-1.0, 0.0, 0.0),
    },
)
# The moon: grey, airless, cloudless, dark at night (a faint earthshine), turning slowly.
MOON_LOOK = dict(
    vectors={
        'Shape': (0.0, 0.01, 0.8, 0.0),
        'Spin': (0.15, 0.0, 40.0, 0.0),
        'NightLook': (0.0, 0.1, 0.25, 0.0),
        'AmbientColor': (0.006, 0.007, 0.009),
        'CloudLook': (0.0, 0.0, 0.0, 0.0),
        'SeaLook': (0.0, 1.0, 0.0, 0.0),
        'AtmosphereLook': (0.0, 1.0, 0.0, 0.0),
        'AtmosphereShape': (4.0, 0.0, 6.0, 0.0),
    },
)


def build_planet(textures):
    mat = material('M_Planet')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', False)
    g = Graph(mat)
    v = PLANET_LOOK['vectors']
    groups = {'Shape': 'Planet', 'Spin': 'Planet', 'SunDirection': 'Scene', 'North': 'Scene', 'East': 'Scene',
              'Facing': 'Scene', 'SunColor': 'Light', 'TerminatorColor': 'Light', 'NightLook': 'Light',
              'LightsColor': 'Light', 'AmbientColor': 'Light', 'Glare': 'Light', 'CloudLook': 'Clouds',
              'SeaLook': 'Planet', 'Atmosphere': 'Air', 'AtmosphereLook': 'Air', 'AtmosphereShape': 'Air'}
    inputs = [
        ('WorldPos', g.node(unreal.MaterialExpressionWorldPosition, -1400, -700,
                            world_position_shader_offset=unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS), ''),
        ('CameraPos', g.node(unreal.MaterialExpressionCameraPositionWS, -1400, -600), ''),
        ('Center', g.node(unreal.MaterialExpressionObjectPositionWS, -1400, -500), ''),
        ('Time', time_node(g, -1400, -400), ''),
        ('PlanetRadius', g.scalar('PlanetRadius', PLANET_LOOK['scalars']['PlanetRadius'], -1400, -300, 'Scene'), ''),
        ('SurfaceMap', g.texture_object('SurfaceMap', textures['T_Teropa_BC'], -1400, -200, 'Maps'), ''),
        ('ReliefMap', g.texture_object('ReliefMap', textures['T_Teropa_M'], -1400, -50, 'Maps'), ''),
        ('CloudMap', g.texture_object('CloudMap', textures['T_TeropaClouds_M'], -1400, 100, 'Maps'), ''),
    ]
    y = 250
    for name, value in v.items():
        pin_out = 'RGBA' if name in ('Shape', 'Spin', 'NightLook', 'CloudLook', 'SeaLook', 'AtmosphereLook',
                                     'AtmosphereShape', 'Glare') else ''
        inputs.append((name, g.vector(name, value, -1400, y, groups[name]), pin_out))
        y += 100
    planet = g.custom(PLANET_CODE, inputs, CMOT.CMOT_FLOAT4, -700, 0, 'Teropa from orbit')
    rgb = g.node(unreal.MaterialExpressionComponentMask, -300, -50, r=True, g=True, b=True, a=False)
    g.link(planet, '', rgb, '')
    alpha = g.node(unreal.MaterialExpressionComponentMask, -300, 100, r=False, g=False, b=False, a=True)
    g.link(planet, '', alpha, '')
    g.out(rgb, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(alpha, '', unreal.MaterialProperty.MP_OPACITY)
    finish(mat)
    return mat


# --- M_Starfield ---

STARS_CODE = """const float Pi = 3.14159265;
float3 d = normalize(-CameraVector);
// A pixel's width as an angle: stars stay about a pixel or two wide at any resolution and field of view.
float pix = max(length(fwidth(d)), 0.00001);
float3 Up = normalize(SkyNorth);
float3 Rt = normalize(SkyEast - Up * dot(SkyEast, Up));
float3 Fr = cross(Up, Rt);
float lon = atan2(dot(d, Rt), dot(d, Fr));
float lat = asin(clamp(dot(d, Up), -1.0, 1.0));
float2 uv = float2(lon / (2.0 * Pi) + 0.5, 0.5 - lat / Pi);
float2 dx = ddx(uv); dx.x -= round(dx.x);
float2 dy = ddy(uv); dy.x -= round(dy.x);
float3 col = Texture2DSampleGrad(NebulaMap, NebulaMapSampler, uv, dx, dy).rgb * StarLook.x;
float band = exp(-(lat * lat) / 0.06);

// Three layers of cells round the eye, each holding at most one star well inside it (so no star is cut at a cell's
// edge): few bright ones, more faint ones, a dust of the faintest.
[unroll] for (int i = 0; i < 3; i++)
{
    float scale = Stars.x * (i == 0 ? 1.0 : (i == 1 ? 2.6 : 6.5));
    float share = Stars.y * (i == 0 ? 1.0 : (i == 1 ? 0.7 : 0.4));
    float bright = (i == 0 ? 1.0 : (i == 1 ? 0.6 : 0.35));
    float3 cell = floor(d * scale);
    uint3 q = uint3(int3(cell) + 32768) * 1664525u + 1013904223u + uint(i) * 2891336453u;
    q.x += q.y * q.z; q.y += q.z * q.x; q.z += q.x * q.y;
    q ^= q >> 16u;
    q.x += q.y * q.z; q.y += q.z * q.x; q.z += q.x * q.y;
    float3 h = float3(q) * (1.0 / 4294967296.0);
    uint3 r = q * 747796405u + 2891336453u;
    r.x += r.y * r.z; r.y += r.z * r.x; r.z += r.x * r.y;
    r ^= r >> 16u;
    float3 h2 = float3(r) * (1.0 / 4294967296.0);
    float3 star = normalize(cell + 0.3 + 0.4 * h);
    float a = length(d - star);
    float present = step(h2.x, share * (1.0 + Stars.z * band));
    float mag = pow(h2.y, max(Stars.w, 0.1)) * bright;
    float size = pix * (0.55 + 1.1 * mag) * (i == 2 ? 0.8 : 1.0);
    float twinkle = 1.0 + StarLook.z * sin(Time * (1.3 + 2.5 * h2.z) + h.x * 40.0) * step(0.5, mag);
    float3 tint = lerp(float3(1.0, 0.78, 0.58), float3(0.7, 0.82, 1.0), h2.z);
    col += tint * (exp(-(a * a) / (size * size)) * mag * present * twinkle * StarLook.y);
}

// The sun: a white disc (usually out of frame), its glow, and the lens's wide glare (as M_Planet's).
float cs = dot(d, normalize(SunDirection));
float ang = acos(clamp(cs, -1.0, 1.0));
col += SunColor * (smoothstep(Sun.x * 1.2, Sun.x * 0.8, ang) * Sun.y + exp(-ang / max(Sun.z, 0.0001)) * Sun.w
    + pow(saturate(cs), max(Glare.y, 0.1)) * Glare.x);
return col;"""

STARS_LOOK = dict(
    vectors={
        'Stars': (70.0, 0.45, 1.5, 6.0),
        'StarLook': (0.4, 2.2, 0.2, 0.0),
        'Sun': (0.0093, 40.0, 0.05, 0.6),
        'SunColor': PLANET_LOOK['vectors']['SunColor'],
        'Glare': PLANET_LOOK['vectors']['Glare'],
        # The scene's (build_menu_teropa.py).
        'SunDirection': PLANET_LOOK['vectors']['SunDirection'],
        'SkyNorth': (0.3, -0.45, 0.84),
        'SkyEast': (0.0, 1.0, 0.0),
    },
)


def build_starfield(textures):
    mat = material('M_Starfield')
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    # Seen from inside its sphere.
    mat.set_editor_property('two_sided', True)
    g = Graph(mat)
    inputs = [
        ('CameraVector', g.node(unreal.MaterialExpressionCameraVectorWS, -1400, -500), ''),
        ('Time', time_node(g, -1400, -400), ''),
        ('NebulaMap', g.texture_object('NebulaMap', textures['T_MenuNebula_BC'], -1400, -300, 'Maps'), ''),
    ]
    y = -150
    for name, value in STARS_LOOK['vectors'].items():
        group = 'Scene' if name in ('SunDirection', 'SkyNorth', 'SkyEast') else 'Sky'
        inputs.append((name, g.vector(name, value, -1400, y, group), 'RGBA' if name in ('Stars', 'StarLook', 'Sun', 'Glare') else ''))
        y += 100
    stars = g.custom(STARS_CODE, inputs, CMOT.CMOT_FLOAT3, -700, 0, 'Stars, the galaxy and the sun')
    g.out(stars, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(mat)
    return mat


# --- Instances ---

def instance(name, parent, look, textures=None):
    """MI_<name> of parent with the look's values (made, or updated in place so placed meshes keep it)."""
    path = f'{FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    for key, value in look.get('scalars', {}).items():
        MEL.set_material_instance_scalar_parameter_value(mi, key, float(value))
    for key, value in look.get('vectors', {}).items():
        values = [float(x) for x in value][:4]
        MEL.set_material_instance_vector_parameter_value(mi, key, unreal.LinearColor(*(values + [1.0] * (4 - len(values)))))
    for key, texture in (textures or {}).items():
        MEL.set_material_instance_texture_parameter_value(mi, key, texture)
    MEL.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi, only_if_is_dirty=False)
    return mi


def main():
    if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor():
        raise RuntimeError('a play session is running: stop it first')
    textures = import_textures()
    planet = build_planet(textures)
    stars = build_starfield(textures)
    instance('MI_Planet_Teropa', planet, PLANET_LOOK)
    instance('MI_Planet_Moon', planet, MOON_LOOK,
             {'SurfaceMap': textures['T_TeropaMoon_BC'], 'ReliefMap': textures['T_TeropaMoon_M']})
    instance('MI_Starfield_Teropa', stars, STARS_LOOK)
    log(f'built {planet.get_path_name()}, {stars.get_path_name()} and MI_Planet_Teropa, MI_Planet_Moon, MI_Starfield_Teropa')


main()
