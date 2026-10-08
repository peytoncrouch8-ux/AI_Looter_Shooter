"""Sunbleached (Style Lab style 6: Tools/StyleLab/web/styles/06_bleached.js with lib/bleached_glsl.js and
lib/bleached_sky.js; Docs/Art/StyleLab/Catalog.md section 6) in Unreal's terms: the numbers its static switch's
parameters take on the masters (Tools/Unreal/build_world_materials.py) and on the tutorial island's instances
(Tools/Unreal/build_island_style.py sunbleached). Plain Python with no unreal import.

The look: a high-key, luminous frontier. Albedo lifted and warmed toward sandstone cream; whatever the sun doesn't
reach shifts lavender; a warm rim blazes on silhouettes between the eye and the sun; a warm bounce lifts the shade;
sandstone and timber glow a little past the terminator; rock and sand glitter, the sand with a broad sheen toward the
sun; a white-gold sky with sheared cirrus, a huge sun glow and the 22-degree ice halo; luminous cream haze; a gentle,
wide bloom. Every parameter is in group Sunbleached and starts with Bleach.
"""
import style_common as sc

NAME = 'Sunbleached'
# The masters this style changes, by what the island script does with an instance of each, and those with its switch.
MASTERS = {'M_World': 'surface', 'M_WorldFoliage': 'surface', 'M_Terrain': 'terrain', 'M_SkyClouds': 'sky',
           'M_Water': 'water', 'M_Smoke': 'smoke', 'M_Waterfall': 'waterfall'}
SWITCHED = ('M_World', 'M_WorldFoliage', 'M_Terrain', 'M_SkyClouds')
POST_MASTER = 'M_SunbleachedPost'
# Roles the lab leaves to their own look here (glow and gun as they are; creatures dark by their own masters, which
# this doesn't reach).
UNSTYLED_ROLES = ('glow', 'creature', 'gun')
LAB_SUN = 4.6
LAB_EXPOSURE = 1.05
# The island's exposure bias the masters' defaults are worked out for (the island's own: style_sunbleached.json).
DEFAULT_EXPOSURE_BIAS = 0.7

SHADE = (0.9, 0.84, 1.22)        # lavender: blue up, green down
RIM = (1.0, 0.82, 0.58)
BOUNCE = (0.42, 0.32, 0.22)
GLOW = (1.0, 0.7, 0.42)
AO = ('#3a3480', 0.5)            # the post AO's colour and intensity
NO_GLITTER = (0.0, 0.012, 0.0, 50.0)


def lab_surface(role):
    """06_bleached.js material() for a role, or None for one it leaves."""
    if role in UNSTYLED_ROLES:
        return None
    if role in ('foliage', 'grass'):
        # shadeHook(SHADE, 0.8) + rimHook(RIM, 0.45): no bounce, glow or glitter
        return dict(saturation=0.85, value=1.12, hue=6.0, tint=None, shade=0.8, rim=0.45, bounce=0.0, glow=0.0,
                    glitter=None, sheen=0.0)
    if role in ('glass', 'water', 'smoke'):
        return dict(saturation=1.0, value=1.05, hue=0.0, tint=None, shade=0.0, rim=0.0, bounce=0.0, glow=0.0,
                    glitter=None, sheen=0.0)
    o = dict(saturation=0.95, value=1.1, hue=0.0, tint=('#eec79a', 0.35), shade=1.0, rim=0.5, bounce=0.15, glow=0.3,
             glitter=None, sheen=0.0)
    if role in ('rock', 'stone'):
        o['glitter'] = dict(density=0.06, size=0.012, strength=10.0, far=50.0, spread=0.55)
    return o


def lab_ground():
    """06_bleached.js terrain(): its grade, then shade, sheen, glitter, rim, bounce and glow."""
    return dict(saturation=1.0, value=1.1, hue=0.0, tint=('#ecbf88', 0.45), shade=1.0, rim=0.15, bounce=0.1,
                glow=0.2, glitter=dict(density=0.05, size=0.01, strength=16.0, far=60.0, spread=0.45), sheen=0.4)


def surface_params(o):
    """The Bleach* values of M_World, M_WorldFoliage and M_Terrain from lab options."""
    tint = sc.vec4(sc.rgb(o['tint'][0]), o['tint'][1]) if o['tint'] else (1.0, 1.0, 1.0, 0.0)
    g = o['glitter']
    return {
        'scalars': {'BleachRimPower': o.get('rimPower', 3.0)},
        'vectors': {'BleachAdjust': (o['saturation'], o['value'], 1.0, o['hue']),
                    'BleachTint': tint,
                    'BleachShade': sc.vec4(SHADE, o['shade']),
                    'BleachAO': sc.vec4(sc.rgb(AO[0]), AO[1]),
                    'BleachRim': sc.vec4(RIM, o['rim']),
                    'BleachBounce': sc.vec4(BOUNCE, o['bounce']),
                    'BleachGlow': sc.vec4(GLOW, o['glow']),
                    'BleachGlitter': (g['density'], g['size'], g['strength'], g['far']) if g else NO_GLITTER,
                    'BleachGlitter2': (g['spread'] if g else 0.55, o['sheen'], LAB_SUN, 0.0)},
    }


SURFACE_DEFAULTS = surface_params(lab_surface('other'))
FOLIAGE_DEFAULTS = surface_params(lab_surface('foliage'))
GROUND_DEFAULTS = surface_params(lab_ground())


# --- The sky: lib/bleached_sky.js with 06_bleached.js's SKY and fog ---

SKY = {'zenith': '#2f90c2', 'mid': '#7fbfd4', 'horizon': '#fbeed6', 'ground': '#e8d6bc', 'anti': '#c8bce8',
       'cirrus': '#efebe4', 'cirrusShade': '#d2cde4', 'sun': '#fff6e6', 'sunBright': 16.0, 'glow': '#ffe2a8',
       'glowAmount': 1.0, 'halo': '#fff0d8', 'haloAmount': 0.6, 'cirrusAmount': 0.75, 'horizonFog': 0.6,
       'fog': '#f6e8d4', 'fogSun': '#fff7e4', 'fogScatter': 0.9, 'fogExponent': 6.0,
       # 1 paints the whole sky on the dome over the engine's atmosphere (the lab's look)
       'opacity': 1.0}


def sky_params(bias, changes=None):
    """M_SkyClouds' Bleach* values at the island's exposure: colours given as they should look on screen are turned
    into emissives; the sun's disc, glow and halo are the lab's scene-referred radiance at Unreal's exposure."""
    s = sc.merged(SKY, changes)
    e = lambda color: sc.vec4(sc.screen_to_emissive(color, bias))  # noqa: E731
    r = lambda color, k=1.0: sc.vec4(sc.lab_radiance_to_emissive(color, LAB_EXPOSURE, bias, k))  # noqa: E731
    return {
        'scalars': {'BleachSkyOpacity': s['opacity']},
        'vectors': {'BleachZenith': e(s['zenith']), 'BleachMid': e(s['mid']), 'BleachHorizon': e(s['horizon']),
                    'BleachGround': e(s['ground']), 'BleachAnti': e(s['anti']), 'BleachCirrus': e(s['cirrus']),
                    'BleachCirrusShade': e(s['cirrusShade']), 'BleachFogColor': e(s['fog']),
                    'BleachFogSun': e(s['fogSun']),
                    'BleachSunDisc': r(s['sun'], s['sunBright']), 'BleachSunGlow': r(s['glow']),
                    'BleachHalo': r(s['halo']),
                    'BleachSky': (s['haloAmount'], s['cirrusAmount'], s['horizonFog'], s['glowAmount']),
                    'BleachFogScatter': (s['fogScatter'], s['fogExponent'], 0.0, 0.0)},
    }


SKY_DEFAULTS = sky_params(DEFAULT_EXPOSURE_BIAS)


# --- The screen pass: the lab's height fog (luminous cream haze) and its lavender vignette ---

POST = {'haze': {'color': '#f6e8d4', 'sunColor': '#fff7e4', 'density': 0.0035, 'falloff': 0.025, 'height': 0.0,
                 'start': 25.0, 'scatter': 0.9, 'exponent': 6.0, 'maxOpacity': 1.0},
        'vignette': {'amount': 0.12, 'softness': 0.6, 'roundness': 0.6, 'color': '#4a3e6a'}}


def post_params(changes=None, bias=DEFAULT_EXPOSURE_BIAS):
    """M_SunbleachedPost's values (it works on the pre-exposed scene colour, so its colours are the inverse curve's
    alone). Haze lengths in metres, density per metre (the lab's: 8, 23 and 62 % at 50, 100 and 300 m)."""
    p = sc.merged(POST, changes)
    hz, vg = p['haze'], p['vignette']
    return {
        'scalars': {},
        'vectors': {'BleachHaze': (hz['density'], hz['falloff'], hz['height'], hz['start']),
                    'BleachHazeSun': (hz['scatter'], hz['exponent'], hz['maxOpacity'], 0.0),
                    'BleachHazeColor': sc.vec4(sc.screen_to_buffer(hz['color'])),
                    'BleachHazeSunColor': sc.vec4(sc.screen_to_buffer(hz['sunColor'])),
                    'BleachVignette': (vg['amount'], vg['softness'], vg['roundness'], 0.0),
                    'BleachVignetteColor': sc.vec4(sc.screen_to_buffer(vg['color']))},
    }


POST_DEFAULTS = post_params()


# --- The island's instances ---

def with_changes(params, changes):
    out = {'scalars': dict(params.get('scalars', {})), 'vectors': dict(params.get('vectors', {}))}
    for kind in ('scalars', 'vectors'):
        out[kind].update((changes or {}).get(kind, {}))
    return out


def instance_values(kind, name, set_name, master, vector_of, settings, bias):
    """(role, values) for the island's instance of a material (kind from MASTERS; vector_of(name) reads the material's
    own vector parameter), or (role, None) for one this style leaves. settings is style_sunbleached.json."""
    if kind == 'surface':
        role = sc.role_of(name, set_name, master)
        options = lab_surface(role)
        if options is None:
            return role, None
        options.update(settings.get('roles', {}).get(role, {}))
        params = surface_params(options)
    elif kind == 'terrain':
        role, params = 'ground', with_changes(surface_params(lab_ground()), settings.get('terrain'))
    elif kind == 'sky':
        role, params = 'sky', sky_params(bias, settings.get('sky'))
    else:
        # water, smoke and waterfalls: the lab only lifts them a little (value 1.05)
        role = kind
        value = settings.get('effects', {}).get('value', 1.05)
        names = {'water': ('WaterColor',), 'smoke': ('SmokeColor',), 'waterfall': ('WaterColor', 'FoamColor')}[kind]
        params = {'scalars': {}, 'vectors': {n: sc.vec4(sc.adjusted(vector_of(n), 1.0, value)) for n in names}}
    return role, with_changes(params, settings.get('materials', {}).get(name))
