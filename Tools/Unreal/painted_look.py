"""Painted Frontier (Style Lab style 3: Tools/StyleLab/web/styles/03_painted.js with lib/painted_shaders.js,
lib/painted_edges.js and lib/anime_sky.js) in Unreal's terms: the lab's numbers, the roles it paints by, and the colour
conversions. Tools/Unreal/build_world_materials.py takes the masters' Painted defaults from here, and
Tools/Unreal/build_island_painted.py each tutorial island instance's values. Plain Python with no unreal import, so a
dry run can load it anywhere.

Units. The lab's lights are three.js numbers (a 4.3 sun) seen through its exposure of 1.06. The island's are Unreal's (a
7 lux sun) seen through an exposure that stays pinned at its post volume's top (auto exposure's max brightness, EV100
1.0) and a bias. So:
- a colour the lab draws unlit (its sky, its clouds), given as it should look on screen, becomes an emissive here
  through the inverse of the filmic curve and the exposure (screen_to_emissive);
- a radiance the lab adds to a lit colour (the warm rim, the blue fill in shade) is scaled by LAB_TO_UE_EMISSIVE, so it
  keeps its share of a sunlit white surface (0.4 of the lab's 1.37 there, 0.72 of Unreal's 2.2).
"""

# Where the island's auto exposure sits: its post volume clamps EV100 to 0.3..1.0, and the sunlit island always asks for
# more than 1.0, so the exposure is 1 / (1.2 * 2^1) times 2^bias.
EV100 = 1.0
LAB_EXPOSURE = 1.06
LAB_TO_UE_EMISSIVE = 1.8
# The bias the masters' defaults are worked out for; the island's own comes from Art/Levels/TutorialIsland/painted.json.
DEFAULT_EXPOSURE_BIAS = 0.6
# A display value the inverse curve may start from: the filmic curve reaches 1.0 only at about 7, which would turn a
# white cloud into a bloom source; 0.92 keeps it near the painted clouds' old brightness (LitColor 3).
SCREEN_CEILING = 0.92
# The macro noise (T_MacroNoise_M) has about 14.6 soft features across its 1024 px; the lab's noise has one per unit.
NOISE_UV = 1.0 / 14.6


def srgb_to_linear(value):
    return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4


def rgb(color):
    """A '#rrggbb' sRGB colour as linear (r, g, b); a list or tuple is taken as linear already."""
    if isinstance(color, str):
        h = color.lstrip('#')
        return tuple(srgb_to_linear(int(h[i:i + 2], 16) / 255.0) for i in (0, 2, 4))
    return tuple(float(c) for c in color[:3])


def filmic(x):
    """The ACES fit Unreal's default filmic tone curve follows closely (Narkowicz)."""
    return x * (2.51 * x + 0.03) / (x * (2.43 * x + 0.59) + 0.14)


def inverse_filmic(y):
    y = min(max(y, 0.0), 0.999)
    lo, hi = 0.0, 64.0
    for _ in range(60):
        mid = 0.5 * (lo + hi)
        if filmic(mid) < y:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


def exposure(bias):
    return 2.0 ** bias / (1.2 * 2.0 ** EV100)


def screen_to_buffer(color, ceiling=SCREEN_CEILING):
    """What the pre-exposed scene colour must hold to show color on screen (a post-process material's units)."""
    return tuple(inverse_filmic(min(c, ceiling)) for c in rgb(color))


def screen_to_emissive(color, bias, ceiling=SCREEN_CEILING):
    """The emissive (unlit) colour that shows color on screen at the island's exposure with this bias."""
    scale = 1.0 / exposure(bias)
    return tuple(c * scale for c in screen_to_buffer(color, ceiling))


def lab_radiance_to_emissive(color, bias):
    """A radiance the lab draws through its own exposure (its sun's colour on the sky), at Unreal's exposure."""
    scale = LAB_EXPOSURE / exposure(bias)
    return tuple(c * scale for c in rgb(color))


def vec4(values, w=1.0):
    values = list(values)
    return tuple(float(v) for v in (values + [w] * (4 - len(values)))[:4])


# --- Roles: what a material is, as the lab's export decides it (Tools/StyleLab/export/lab_export.py role_of) ---

ROLE_BY_SET = {'HouseTrim': 'wood', 'WoodPlanks': 'wood', 'WoodEndGrain': 'wood', 'GunWood': 'wood', 'Hay': 'grass',
               'MetalRust': 'metal', 'MetalWorn': 'metal', 'PaintWorn': 'metal', 'ScreenMesh': 'metal',
               'StoneWall': 'stone', 'RockGranite': 'stone', 'RockCliff': 'rock', 'GroundGrass': 'grass',
               'GroundDirt': 'ground', 'BarkOak': 'bark', 'BarkBirch': 'bark', 'BarkPine': 'bark',
               'LeavesOak': 'foliage', 'LeavesBirch': 'foliage', 'NeedlesPine': 'foliage', 'FoliagePalette': 'foliage',
               'SpiderBody': 'creature', 'SpiderBody_Pale': 'creature', 'Webs': 'fabric', 'Posters': 'paper',
               'Polymer': 'gun'}
ROLE_WORDS = (('glow', ('Glow', 'Flame', 'Ember', 'Reticle')), ('glass', ('Glass', 'Lens', 'Window')),
              ('bone', ('Bone', 'Skull', 'Rib', 'Antler', 'Horn', 'Tooth', 'Teeth')),
              ('skin', ('Skin', 'Flesh', 'Face', 'Hand', 'Lip', 'Eye')),
              ('fabric', ('Cloth', 'Coat', 'Crepe', 'Fabric', 'Hat', 'Wool', 'Silk', 'Web', 'Ribbon', 'Rope', 'Leather',
                          'Felt', 'Shirt', 'Scarf', 'Veil', 'Canvas', 'Sack', 'Burlap', 'Vest', 'Trouser', 'Boot',
                          'Strap', 'Belt', 'Bandana', 'Feather', 'Cocoon', 'Shroud')),
              ('paper', ('Paper', 'Poster', 'Page', 'Ledger', 'Card', 'Book', 'Letter')),
              ('metal', ('Iron', 'Metal', 'Steel', 'Brass', 'Tin', 'Rust', 'Gun', 'Bell', 'Nail', 'Chain', 'Silver',
                         'Gold', 'Copper', 'Lantern')),
              ('wood', ('Wood', 'Plank', 'Board', 'Post', 'Coffin', 'Barrel', 'Crate', 'Log', 'Shake', 'Beam',
                        'Walnut', 'Oak', 'Pine')),
              ('stone', ('Stone', 'Granite', 'Brick', 'Marble', 'Plaster', 'Mortar')),
              ('rock', ('Rock', 'Cliff', 'Boulder', 'Pebble')),
              ('foliage', ('Leaf', 'Leaves', 'Needle', 'Moss', 'Bush', 'Fern', 'Sage', 'Juniper', 'Flower', 'Petal')),
              ('grass', ('Grass', 'Hay', 'Straw', 'Tuft')),
              ('water', ('Water',)), ('smoke', ('Smoke', 'Fog', 'Mist', 'Wisp')))
# The texture sets with cut-outs (their base colour PNGs carry alpha): the lab keeps those crisp (a softer blur).
ALPHA_SETS = ('LeavesBirch', 'LeavesOak', 'NeedlesPine', 'Posters', 'Webs')
# The masters, by what the island script does with an instance of each.
MASTERS = {'M_World': 'surface', 'M_WorldFoliage': 'surface', 'M_Terrain': 'terrain', 'M_SkyClouds': 'sky',
           'M_Water': 'water', 'M_Smoke': 'smoke', 'M_Waterfall': 'waterfall'}
# Roles the lab paints without the painted hook (flat glow, glass, creatures with their own look, guns at runtime).
UNPAINTED_ROLES = ('glow', 'glass', 'creature', 'gun', 'water', 'smoke')


def role_of(name, set_name, master):
    """A surface's role: by its texture set, else by words in its material's name, else foliage or other."""
    if set_name in ROLE_BY_SET:
        return ROLE_BY_SET[set_name]
    lowered = name.lower()
    for role, words in ROLE_WORDS:
        if any(w.lower() in lowered for w in words):
            return role
    return 'foliage' if master == 'M_WorldFoliage' else 'other'


def is_alpha_masked(set_name, master):
    return set_name in ALPHA_SETS and (master == 'M_WorldFoliage' or set_name in ('Webs', 'Posters'))


# --- Surfaces: 03_painted.js material() and paintHook()'s defaults ---

WARM_TOP = (1.16, 1.06, 0.88)
COOL_UNDER = (0.62, 0.66, 0.92)
FOLIAGE_WARM = (1.22, 1.14, 0.78)
FOLIAGE_COOL = (0.55, 0.65, 0.95)
SATURATION = {'wood': 1.45, 'rock': 1.35, 'stone': 1.2, 'metal': 1.25, 'foliage': 1.4, 'grass': 1.35, 'bark': 1.3,
              'creature': 1.0}
RIM = ('#ffd9a8', 3.0, 0.4)           # colour, power, strength
SHADE = (0.80, 0.74, 1.28, 0.55)      # paintHook's shadeTint and shadeAmount
FILL = (0.30, 0.30, 0.52, 0.55)       # its sky fill in shade, and how much
AO = ('#4a3070', 0.85)                # the post AO's colour and intensity (kit.post.ao)
STROKES = (0.1, 0.45, 0.07, 45.0)     # amount, stroke length and width (m), fade distance (m)
BLOTCH_SCALE = 1.4                    # brush blotches per metre
NORMAL_BLUR = 1.5                     # biasedNormalChunk(THREE, 1.5)


def lab_surface(role, set_name, alpha_mask):
    """The lab's options for one material (03_painted.js material()), or None for a role it doesn't paint."""
    if role in UNPAINTED_ROLES:
        return None
    o = {'blur': 1.6,
         'saturation': 1.6 if set_name == 'HouseTrim' else 1.25 if role == 'wood' else SATURATION.get(role, 1.3),
         'contrast': 1.1, 'value': 1.75 if role == 'wood' else 1.2, 'hue': 0.0,
         'tint': ('#b8783e', 0.3) if role == 'wood' and set_name != 'HouseTrim' else None,
         'roughness': 1.25, 'specular': 0.35, 'rim': RIM,
         'hook': dict(heights=True, bands=4, bandAmount=0.7, blotch=0.26, feet=0.74, feetHeight=3.2,
                      warm=WARM_TOP, cool=COOL_UNDER)}
    if alpha_mask:
        # leaves, webs, posters: the cut-outs kept crisp, the colour painted
        o['blur'] = 0.6
        o['hook'] = dict(heights=True, bands=0, bandAmount=0.7, blotch=0.3, feet=0.75, feetHeight=6.0, warm=WARM_TOP,
                         cool=COOL_UNDER)
    if role in ('foliage', 'grass'):
        o['hue'] = 4.0
        o['hook'] = dict(heights=True, bands=3, bandAmount=0.7, blotch=0.32, feet=0.7, feetHeight=5.0,
                         warm=FOLIAGE_WARM, cool=FOLIAGE_COOL)
    if role == 'metal':
        o.update(roughness=0.85, specular=0.7)
    if role == 'rock':
        # sandstone painted in warm reds and ochres, the shade going violet
        o.update(tint=('#cf7a4c', 0.35), value=1.3, saturation=1.3)
    return o


def surface_params(o):
    """An instance's Paint* values (M_World, M_WorldFoliage) from lab_surface() options."""
    hook = o['hook']
    tint = vec4(rgb(o['tint'][0]), o['tint'][1]) if o['tint'] else (1.0, 1.0, 1.0, 0.0)
    rim_color, rim_power, rim_strength = o['rim']
    return {
        'scalars': {'PaintBlur': o['blur'], 'PaintNormalBlur': NORMAL_BLUR, 'PaintRimPower': rim_power,
                    'PaintRoughness': o['roughness'], 'PaintSpecular': o['specular'],
                    'PaintEmissiveScale': LAB_TO_UE_EMISSIVE},
        'vectors': {'PaintAdjust': (o['saturation'], o['value'], o['contrast'], o['hue']),
                    'PaintTint': tint,
                    'PaintBands': (hook['bands'], hook['bandAmount'], hook['blotch'], BLOTCH_SCALE),
                    'PaintStrokes': STROKES,
                    'PaintWarmTop': vec4(hook['warm']), 'PaintCoolUnder': vec4(hook['cool']),
                    'PaintFeet': (hook['feet'], hook['feetHeight'], 1.0, 1.0 if hook['heights'] else 0.0),
                    'PaintShade': SHADE, 'PaintFill': FILL,
                    'PaintAO': vec4(rgb(AO[0]), AO[1]),
                    'PaintRim': vec4(rgb(rim_color), rim_strength)},
    }


SURFACE_DEFAULTS = surface_params(lab_surface('other', None, False))
FOLIAGE_DEFAULTS = surface_params(lab_surface('foliage', 'LeavesOak', True))


# --- The ground: 03_painted.js terrain(), groundRecolor() and paintGroundHook() ---

def ground_params():
    """M_Terrain's Paint* values. The island's macro map's alpha picks the ground's kind (0 grass and soil, about 0.25
    footpaths, 0.4 dirt roads, 0.65 scree, 1 rock) and its hue finds bare soil, where the lab had blend weights."""
    return {
        'scalars': {'PaintBlur': 2.0, 'DetailStrength': 0.35, 'NormalStrength': 0.35},
        'vectors': {'PaintDirt': vec4(rgb('#d9b98c')), 'PaintGrass': vec4(rgb('#7fa848')),
                    'PaintRock': vec4(rgb('#cf7d50')),
                    'PaintGround': (0.6, 0.15, 0.45, 1.7),          # keep, reference luma, least and most variation
                    'PaintGroundSplit': (0.08, 0.35, 0.5, 0.9),     # alpha for dirt (from, to), for rock (from, to)
                    'PaintGroundHue': (-0.05, 0.08, 0.0, 0.0),      # (red - green) / green for bare soil (from, to)
                    'PaintGroundBrush': (0.30, 0.16, 0.08, 0.05),   # big, mid and fine patches, hue wander (turns)
                    'PaintStrokes': (0.1, 1.1, 0.16, 90.0),
                    'PaintGroundLit': (1.07, 1.0, 0.9, 1.0), 'PaintGroundShade': (0.82, 0.82, 1.2, 1.0)},
    }


GROUND_DEFAULTS = ground_params()


# --- The sky: 03_painted.js sky() through makeToonSky() ---

SKY = {'zenith': '#2f74cc', 'mid': '#62a6dc', 'midAt': 0.3, 'power': 0.7, 'horizon': '#d2e8e2', 'ground': '#a08460',
       'haze': 0.45, 'brightness': 1.0,
       'sunColor': '#ffe0ae', 'sunIntensity': 4.3,
       'sun': {'size': 1.6, 'disc': 18.0, 'glow': 0.9, 'glare': 0.0},
       'towers': {'count': 8, 'height': 1.1, 'width': 1.1, 'seed': 9},
       'floaters': {'amount': 0.48, 'size': 0.1, 'low': 13.0, 'high': 52.0},
       'cel': {'edge': 0.0, 'softness': 0.22, 'wrap': 0.4, 'rim': 0.3}, 'topLight': 0.35, 'mass': 0.65,
       'cloud': {'lit': '#fff4e2', 'shade': '#b8b0d6', 'dark': '#a08cb8', 'rim': '#ffe6c0'},
       'felt': 0.0, 'brush': 0.16, 'warp': 0.012, 'drift': 0.0018, 'bottomDark': 0.6, 'sunBleed': 0.35,
       'edgeSoft': 0.08,
       # 1 paints the whole sky on the dome over the atmosphere (the lab's look); 0 lays only the clouds over it.
       'skyOpacity': 1.0, 'hazeBrightness': 1.0}


def merged(base, changes):
    out = dict(base)
    for key, value in (changes or {}).items():
        out[key] = merged(base[key], value) if isinstance(value, dict) and isinstance(base.get(key), dict) else value
    return out


def sky_params(bias, changes=None):
    """M_SkyClouds' Paint* values at the island's exposure, with changes over the lab's numbers."""
    s = merged(SKY, changes)
    e = lambda color: vec4(screen_to_emissive(color, bias))  # noqa: E731
    deg = 3.14159265 / 180.0
    sun_color = tuple(c * max(0.2, s['sunIntensity']) * 0.3 for c in lab_radiance_to_emissive(s['sunColor'], bias))
    sun, tw, fl, cel, cl = s['sun'], s['towers'], s['floaters'], s['cel'], s['cloud']
    return {
        'scalars': {'PaintHazeBrightness': s['hazeBrightness']},
        'vectors': {'PaintZenith': e(s['zenith']), 'PaintMid': e(s['mid']), 'PaintHorizon': e(s['horizon']),
                    'PaintGround': e(s['ground']), 'PaintSunColor': vec4(sun_color),
                    'PaintCloudLit': e(cl['lit']), 'PaintCloudShade': e(cl['shade']), 'PaintCloudDark': e(cl['dark']),
                    'PaintCloudRim': e(cl['rim']),
                    'PaintGrad': (s['midAt'], s['power'], s['haze'], s['brightness']),
                    'PaintSun': (sun['size'] * 0.5 * deg, sun['disc'], sun['glow'], sun['glare']),
                    'PaintTowers': (tw['count'], tw['height'], tw['width'], tw['seed']),
                    'PaintFloaters': (fl['amount'], fl['size'], fl['low'] * deg, fl['high'] * deg),
                    'PaintCel': (cel['edge'], cel['softness'], cel['wrap'], cel['rim']),
                    'PaintLook': (s['felt'], s['brush'], s['warp'], 0.0),
                    'PaintMisc': (s['drift'], s['bottomDark'], s['sunBleed'], s['edgeSoft']),
                    'PaintMisc2': (s['topLight'], s['mass'], s['skyOpacity'], 0.0)},
    }


SKY_DEFAULTS = sky_params(DEFAULT_EXPOSURE_BIAS)


# --- The screen pass: painted_edges.js edgePaint() and the grade's vibrance and vignette ---

POST = {'edges': {'worldWidth': 6.0, 'minWidth': 1.0, 'maxWidth': 3.0, 'fadeFar': 110.0, 'convex': 1.0,
                  'concave': 0.45, 'litBias': 0.25, 'nearCut': 1.0, 'tint': '#ffe9c4', 'creaseTint': '#3c2a5c'},
        'vibrance': 0.3,
        'vignette': {'amount': 0.18, 'softness': 0.65, 'roundness': 0.7, 'color': '#1c1428'}}


def post_params(changes=None):
    """M_PaintedPost's values. nearCut (m) keeps the edges off the first-person gun, as the lab did."""
    p = merged(POST, changes)
    ed, vg = p['edges'], p['vignette']
    return {
        'scalars': {'PaintVibrance': p['vibrance']},
        'vectors': {'PaintEdge': (ed['worldWidth'], ed['minWidth'], ed['maxWidth'], ed['fadeFar']),
                    'PaintEdgeK': (ed['convex'], ed['concave'], ed['litBias'], ed['nearCut']),
                    'PaintEdgeTint': vec4(rgb(ed['tint'])), 'PaintCreaseTint': vec4(rgb(ed['creaseTint'])),
                    'PaintVignette': (vg['amount'], vg['softness'], vg['roundness'], 0.0),
                    'PaintVignetteColor': vec4(screen_to_buffer(vg['color']))},
    }


POST_DEFAULTS = post_params()


def adjusted(color, saturation, value):
    """The lab's slAdjust on a linear colour (saturation about its luma, then value): the smoke and the waterfall."""
    r, g, b = color[:3]
    luma = 0.2126 * r + 0.7152 * g + 0.0722 * b
    return tuple(max(luma + (c - luma) * saturation, 0.0) * value for c in (r, g, b))
