"""What every island style (Style Lab looks tried on the tutorial island alone: Tools/Unreal/build_island_style.py) shares:
the roles a material is painted by, and the colour conversions between the lab's numbers and Unreal's. Each style's own
numbers are in Tools/Unreal/style_<name>.py. Plain Python with no unreal import, so a dry run can load it anywhere.

Units. The lab's lights are three.js numbers seen through its own exposure; the island's are Unreal's (a 7 lux sun) seen
through an exposure that stays pinned at its post volume's top (auto exposure's max brightness, EV100 1.0; the project
extends the luminance range, so that is EV100) and a bias. So:
- a colour the lab draws unlit (its sky, its fog), given as it should look on screen, becomes an emissive here through
  the inverse of the filmic curve and the exposure (screen_to_emissive), or, in a post-process material that works on
  the pre-exposed scene colour, through the inverse curve alone (screen_to_buffer);
- a radiance the lab adds to a lit colour in its own units is scaled by the island's sun over the lab's (7 / its sun),
  so it keeps its share of a sunlit white surface; a term the lab writes in its sun's light (slSunCol) reads Unreal's
  sun directly (SkyAtmosphereLightIlluminance) and needs no scale.
"""

# The island's exposure: 1 / (1.2 * 2^EV100) times 2^bias, the sunlit island always asking for more than EV100 1.0.
EV100 = 1.0
ISLAND_SUN = 7.0
# A display value the inverse curve may start from: the filmic curve reaches 1.0 only at about 7, which would turn a
# white sky or cloud into a bloom source.
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


def vec4(values, w=1.0):
    values = list(values)
    return tuple(float(v) for v in (values + [w] * (4 - len(values)))[:4])


def scaled(values, k):
    return tuple(float(v) * k for v in values)


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
    return scaled(screen_to_buffer(color, ceiling), 1.0 / exposure(bias))


def lab_radiance_to_emissive(color, lab_exposure, bias, k=1.0):
    """A radiance the lab draws through its own exposure (a sun disc on its sky, k times color), at Unreal's."""
    return scaled(rgb(color), k * lab_exposure / exposure(bias))


def merged(base, changes):
    out = dict(base)
    for key, value in (changes or {}).items():
        out[key] = merged(base[key], value) if isinstance(value, dict) and isinstance(base.get(key), dict) else value
    return out


def adjusted(color, saturation, value):
    """The lab's slAdjust on a linear colour (saturation about its luma, then value): smoke, waterfalls, water."""
    r, g, b = color[:3]
    luma = 0.2126 * r + 0.7152 * g + 0.0722 * b
    return tuple(max(luma + (c - luma) * saturation, 0.0) * value for c in (r, g, b))


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
# The texture sets with cut-outs (their base colour PNGs carry alpha).
ALPHA_SETS = ('LeavesBirch', 'LeavesOak', 'NeedlesPine', 'Posters', 'Webs')


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
