"""The soul flask (Docs/Work/2026-10-30-SoulFlask/CONTRACT.md): the small stoppered glass flask of soul-light every hero
carries from the start, drunk on a key (H) to heal mid-fight, filled again by the soul of kills. Our own design: a worn,
hand-blown glass flask, its neck a little crooked, carried in a leather and tin harness with a cork in it. The player
holds it in the off hand in first person (UPlayerFlaskComponent, FirstPersonHandsFlask.cpp). A scripted model file
(Art/README.md), two models on one pivot:

  SoulFlask           the flask, corked. A squat round belly (10.4 cm across, a little flattened front to back, a hair
                      lopsided as blown glass is), a short shoulder into a neck that leans about 10 degrees off true,
                      a rolled lip. The glass is clear and faintly green; inside, the soul-light fills it to the shoulder
                      (an emissive surface in the soul-motes' heal green). A tin cup holds its foot; four leather straps
                      run up from the cup over the belly to a tin collar round the neck's foot, held by a leather band
                      round its waist, riveted where they cross; a D-ring hangs off the collar (where it clips to a
                      belt). Waxed cord is wound three times under the lip. The cork stands 1.6 cm proud. 16 cm tall.
  SoulFlaskUncorked   the same flask without its cork (same pivot, same socket), for the drink: the code may swap the
                      mesh in at the uncork (one SetStaticMesh), so the cork reads as out.

Pivot: the middle of its foot. SOCKET_Mouth at the neck's opening on the bent neck's axis, +Z out of the mouth (its
front, -Y, as the model's). No collision, no Nanite (small, held close), no LODs.

Materials (5 slots; the textured sets the game already loads, tinted, shared by name with the salt grenade where alike):
  SoulFlaskGlass    the Glass master (unlit, translucent, rim-bright), a pale green-grey tint, opacity 0.12
  SoulFlaskLiquid   the stylized Surface material, opaque, emissive: Color #6DFF7A (LooterUI::Color::Heal), Glow 2.0.
                    MI_SoulFlaskLiquid on M_StylizedSurface: the code may drive its "Glow" scalar (and "Color") on a
                    dynamic instance (dim it at 0 charges; brighten it at the uncork)
  SoulFlaskLeather  Polymer, dark oiled tan leather
  SoulFlaskTin      MetalWorn, dull tin
  SaltCork          Polymer, cork tan (the salt grenade's)    SaltFuse   Polymer, waxed brown cord (the salt grenade's)

    Tools\\artrun.ps1 -Script Art\\Models\\Items\\SoulFlask.py -Preview
renders Saved/ArtPreviews/Items/SoulFlask.png (three-quarter front), SoulFlask_Side.png (the crooked neck side on),
SoulFlask_Top.png (from above: cork, collar, ring) and SoulFlask_Pair.png (corked and uncorked side by side).
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_props as lp
import looter_textures as lt

SEGMENTS = 28

# --- The glass's outside, foot to lip (radius, height in metres), before the warp (bent neck, flattening, lopsidedness) ---
GLASS = [(0.0, 0.0), (0.035, 0.0), (0.0385, 0.0018), (0.040, 0.006), (0.0462, 0.014), (0.0502, 0.026), (0.0520, 0.040),
         (0.0516, 0.054), (0.0492, 0.067), (0.0432, 0.080), (0.0342, 0.090), (0.0242, 0.097), (0.0168, 0.1035),
         (0.0142, 0.110), (0.0136, 0.128), (0.0146, 0.135), (0.0172, 0.1372), (0.0174, 0.1408), (0.0152, 0.1424),
         (0.0118, 0.1418), (0.0112, 0.130), (0.0112, 0.105)]
LIP = 0.1424             # the lip's top
WALL = 0.0026            # the glass between the outside and the soul-light
FILL = 0.073             # the soul-light's level (just under the shoulder)
NECK_FOOT = 0.095        # where the neck starts to lean
LEAN = 2.0               # how far it leans: dx = LEAN * (z - NECK_FOOT)^2 (4.5 mm and about 10 degrees at the lip)
SQUASH = 0.86            # the belly flattened front to back (a flask carried against the hip)

GLASS_MAT = None         # made in materials()
HEAL = 0x6dff7a          # LooterUI::Color::Heal: the soul-motes' green


def glass(name, tint, opacity):
    """See-through glass on the Glass master (as Tools/Blender/looter_guns.py's lenses), alpha-blended in the previews."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    rgba = lm.hex_color(tint)
    bsdf = next(node for node in mat.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = rgba
    bsdf.inputs['Alpha'].default_value = opacity
    bsdf.inputs['Roughness'].default_value = 0.05
    mat.diffuse_color = rgba[:3] + [opacity]
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    for key in [k for k in mat.keys() if not k.startswith('_')]:
        del mat[key]
    mat['Master'] = 'Glass'
    mat['Tint'] = f'#{tint:06x}'
    mat['Opacity'] = opacity
    return mat


def glow(name, color, strength):
    """An opaque emissive surface (the stylized Surface material's Glow; Nanite can't draw the additive kind, and this
    is no worse off the fallback). Emission set in Blender too, only so the previews show it (at half: the preview's
    tonemapping burns a full-strength glow to a pale mint; the exporter reads the Glow property, not this)."""
    mat = lm.material(name, color, Glow=strength, Variation=0.0)
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Emission Color'].default_value = lm.hex_color(color)
    bsdf.inputs['Emission Strength'].default_value = strength * 0.5
    return mat


GLASS_MAT = glass('SoulFlaskGlass', 0x3f5f55, 0.12)
LIQUID = glow('SoulFlaskLiquid', HEAL, 2.0)
LEATHER = lt.material('Polymer', name='SoulFlaskLeather', tint=0x6b4528)
TIN = lt.material('MetalWorn', name='SoulFlaskTin', tint=0xb5b0a3)
CORK = lt.material('Polymer', name='SaltCork', tint=0xb8915f)
CORD = lt.material('Polymer', name='SaltFuse', tint=0x6e5536)


# --- The shape's warp: every part is built round a straight, round flask and then bent with it, so they all fit ---

def _belly(z):
    """1 over the belly, 0 from the shoulder up (smooth)."""
    u = min(max((z - 0.082) / 0.02, 0.0), 1.0)
    return 1.0 - u * u * (3.0 - 2.0 * u)


def bend(z):
    """How far the neck has leant over (+X) at height z: curving from its foot to the lip, straight on above it (the cork
    is stiff: it carries on along the neck's way out, it doesn't bend)."""
    if z > LIP:
        return LEAN * (LIP - NECK_FOOT) ** 2 + 2.0 * LEAN * (LIP - NECK_FOOT) * (z - LIP)
    return LEAN * max(z - NECK_FOOT, 0.0) ** 2


def warp(obj):
    """Blown glass is never true: the belly a hair lopsided (two slow waves round it) and flattened front to back, the
    neck leaning off to one side."""
    for v in obj.data.vertices:
        x, y, z = v.co
        b = _belly(z)
        a = math.atan2(y, x)
        s = 1.0 + b * (0.012 * math.sin(2.0 * a + 0.7) + 0.007 * math.sin(3.0 * a + 2.1))
        x, y = x * s, y * s * (1.0 - (1.0 - SQUASH) * b)
        v.co = (x + bend(z), y, z)
    obj.data.update()
    return obj


def glass_r(z):
    """The glass's outside radius at height z (the belly and neck, before the warp)."""
    pts = GLASS[1:16]
    for (r0, z0), (r1, z1) in zip(pts, pts[1:]):
        if z0 <= z <= z1:
            u = (z - z0) / max(z1 - z0, 1e-9)
            return r0 + (r1 - r0) * u
    return pts[-1][0] if z > pts[-1][1] else pts[0][0]


def lathed(profile, mat, set_name, segments=SEGMENTS, closed=False):
    obj = lp.lathe(profile, segments=segments, closed=closed)[0]
    lt.assign(obj, mat)
    lp.lathe_uv(obj, set_name)
    return obj


# --- The parts ---

def glass_shell():
    """The glass: its outside down to the foot, the rolled lip, and the neck's inside down to the shoulder (open there:
    what's seen through it is the soul-light and the cork's foot)."""
    return lathed(GLASS, GLASS_MAT, 'Polymer')


def soul_light():
    """The soul-light inside, a glass wall in from the outside, up to its level, a shallow meniscus on top."""
    zs = [0.0035, 0.008, 0.015, 0.026, 0.040, 0.054, 0.064, FILL]
    profile = [(0.0, 0.0035)]
    for z in zs:
        profile.append((max(glass_r(max(z, 0.006)) - WALL - (0.003 if z < 0.006 else 0.0), 0.004), z))
    top = profile[-1][0]
    profile += [(top * 0.94, FILL + 0.0009), (top * 0.5, FILL + 0.0013), (0.0, FILL + 0.0014)]
    return lathed(profile, LIQUID, 'Polymer')


def foot_cup():
    """The tin cup the foot sits in: its wall hugging the glass up to a rolled bead, a plain bottom under it."""
    outer = [(0.0, -0.0014), (0.0372, -0.0014), (0.0402, 0.0012)]
    for z in (0.006, 0.011, 0.0155):
        outer.append((glass_r(z) + 0.0016, z))
    top = glass_r(0.0175) + 0.0026
    profile = outer + [(top, 0.0168), (top, 0.0188), (glass_r(0.019) + 0.0004, 0.0192)]
    for z in (0.014, 0.008):
        profile.append((glass_r(z) + 0.0004, z))
    profile.append((0.0, 0.0018))
    return lathed(profile, TIN, 'MetalWorn')


BAND = (0.034, 0.047)    # the leather band round the waist, bottom to top
THICK = 0.0026           # the leather's thickness


def band():
    """The leather band round the belly's widest, its edges rounded a little."""
    lo, hi = BAND
    r = lambda z: glass_r(z) + 0.0003
    profile = [(r(lo), lo), (r(lo) + THICK * 0.7, lo + 0.0006), (r(lo + 0.002) + THICK, lo + 0.0022),
               (r(hi - 0.002) + THICK, hi - 0.0022), (r(hi) + THICK * 0.7, hi - 0.0006), (r(hi), hi)]
    return lathed(profile, LEATHER, 'Polymer', closed=True)


STRAPS = (45.0, 135.0, 225.0, 315.0)   # round the flask (0 = +X); the front (-Y, 270) left open to show the light
STRAP_W = 0.0085


def strap(angle):
    """One strap from the cup's bead up over the belly to the collar: a flat leather strip laid on the glass, its edges
    eased."""
    a = math.radians(angle)
    out = Vector((math.cos(a), math.sin(a), 0.0))
    zs = [0.0175 + (0.100 - 0.0175) * k / 14 for k in range(15)]
    pts = [out * (glass_r(z) + 0.0004) + Vector((0.0, 0.0, z)) for z in zs]
    w = STRAP_W * 0.5
    t = THICK * 0.8
    profile = [(-w, 0.0), (w, 0.0), (w, t * 0.7), (w * 0.8, t), (-w * 0.8, t), (-w, t * 0.7)]
    obj = lp.sweep(pts, profile, up=tuple(out))
    lt.assign(obj, LEATHER)
    lt.box_uv(obj, 'Polymer')
    return obj


def collar():
    """The tin collar round the neck's foot where the straps end, two beads, and its D-ring off the -X side."""
    profile = []
    for z, extra in ((0.0975, 0.0012), (0.099, 0.0034), (0.1035, 0.0028), (0.106, 0.0034), (0.1075, 0.0012)):
        profile.append((glass_r(z) + extra, z))
    profile += [(glass_r(0.1075) + 0.0002, 0.1075), (glass_r(0.0975) + 0.0002, 0.0975)]
    ring = lathed(profile, TIN, 'MetalWorn', closed=True)
    parts = [ring]
    # The D-ring: a tin loop hanging off a little lug, where a belt clip takes it.
    lug_at = Vector((-(glass_r(0.102) + 0.0042), 0.0, 0.102))
    lug = lp.block((0.005, 0.006, 0.0075), lug_at, bevel=0.0012)
    lt.assign(lug, TIN)
    lt.box_uv(lug, 'MetalWorn')
    parts.append(lug)
    centre = lug_at + Vector((-0.0062, 0.0, -0.0035))
    loop = [centre + Vector((math.cos(t) * 0.0062, 0.0, math.sin(t) * 0.0085))
            for t in [math.radians(90.0 + 360.0 * k / 16) for k in range(17)]]
    wire = [(math.cos(2.0 * math.pi * k / 6) * 0.0011, math.sin(2.0 * math.pi * k / 6) * 0.0011) for k in range(6)]
    d_ring = lp.sweep(loop, wire, up=(0.0, 1.0, 0.0))
    lt.assign(d_ring, TIN)
    lt.box_uv(d_ring, 'MetalWorn')
    parts.append(d_ring)
    return parts


def rivets():
    """Tin rivets where each strap crosses the band and meets the cup and the collar."""
    parts = []
    for angle in STRAPS:
        a = math.radians(angle)
        out = Vector((math.cos(a), math.sin(a), 0.0))
        for z, lift in ((sum(BAND) * 0.5, THICK), (0.0215, THICK * 0.8), (0.0935, THICK * 0.8)):
            bm = lp.new_bmesh()
            lm.uv_sphere(bm, 10, 6, 0.0021)
            obj = lp.mesh_object(bm, '_rivet')
            # Flattened onto the strap: squashed along its outward axis.
            squash = Matrix.Identity(4)
            for i in range(3):
                for j in range(3):
                    squash[i][j] = (1.0 if i == j else 0.0) - 0.55 * out[i] * out[j]
            obj.data.transform(Matrix.Translation(out * (glass_r(z) + 0.0004 + lift) + Vector((0.0, 0.0, z))) @ squash)
            obj.data.update()
            lt.assign(obj, TIN)
            lt.box_uv(obj, 'MetalWorn')
            parts.append(obj)
    return parts


def cord():
    """Waxed cord wound three times round the neck under the lip, its loose end tucked down under the turns."""
    parts = []
    wire = [(math.cos(2.0 * math.pi * k / 6) * 0.0011, math.sin(2.0 * math.pi * k / 6) * 0.0011) for k in range(6)]
    for turn, z in enumerate((0.1265, 0.1288, 0.1311)):
        r = glass_r(z) + 0.0010
        ring = lp.lathe([(r - 0.0011, z), (r, z - 0.0011), (r + 0.0011, z), (r, z + 0.0011)], segments=20, closed=True)[0]
        lt.assign(ring, CORD)
        lp.lathe_uv(ring, 'Polymer')
        parts.append(ring)
    end = [Vector((0.0, -(glass_r(0.125) + 0.0012), 0.125)), Vector((0.0015, -(glass_r(0.121) + 0.0015), 0.121)),
           Vector((0.0022, -(glass_r(0.117) + 0.0013), 0.1172))]
    tail = lp.sweep(end, wire, up=(0.0, -1.0, 0.0), scales=[1.0, 0.95, 0.8])
    lt.assign(tail, CORD)
    lt.box_uv(tail, 'Polymer')
    parts.append(tail)
    return parts


CORK_TOP = LIP + 0.016


def cork():
    """The cork pushed into the neck, wider at its top, its top edge worn round."""
    profile = [(0.0, LIP - 0.010), (0.0104, LIP - 0.010), (0.0112, LIP - 0.002), (0.0126, LIP + 0.004),
               (0.0136, CORK_TOP - 0.0025), (0.0128, CORK_TOP - 0.0004), (0.0108, CORK_TOP), (0.0, CORK_TOP)]
    return lathed(profile, CORK, 'Polymer', 18)


def mouth():
    """SOCKET_Mouth on the bent neck's axis at the lip, and the neck's way out there (unit)."""
    slope = 2.0 * LEAN * (LIP - NECK_FOOT)
    out = Vector((slope, 0.0, 1.0)).normalized()
    return Vector((bend(LIP), 0.0, LIP)), out


def build(name, with_cork):
    parts = [glass_shell(), soul_light(), foot_cup(), band(), *[strap(a) for a in STRAPS], *collar(), *rivets(), *cord()]
    if with_cork:
        parts.append(cork())
    for part in parts:
        warp(part)
    model = lp.join(name, parts)
    lp.finish(model, ao=0.03, nanite=False, smooth=40.0)
    model['Collision'] = 'None'
    at, out = mouth()
    # +Z out of the mouth: turned about Y by the neck's lean (the socket's front stays the model's, -Y).
    lm.socket(model, 'Mouth', at, (0.0, math.degrees(math.atan2(out.x, out.z)), 0.0))
    return model


for _obj in list(bpy.data.objects):   # the startup scene's cube, light and camera
    bpy.data.objects.remove(_obj)
flask = build('SoulFlask', True)
open_flask = build('SoulFlaskUncorked', False)
for model in (flask, open_flask):
    lt._log(f'{model.name}: {lp.tri_count(model)} triangles, materials {", ".join(m.name for m in model.data.materials)}')

if lt.want_preview():
    # The uncorked one moved aside for the pictures only (an object's place doesn't reach the export).
    open_flask.location = (0.3, 0.0, 0.0)
    bpy.context.view_layer.update()
    lt.preview([flask], lt.preview_path('Items', 'SoulFlask'), view=(-0.55, -1.4, 0.45), fit=1.2)
    lt.preview([flask], lt.preview_path('Items', 'SoulFlask_Side'), view=(0.0, -1.5, 0.15), fit=1.2)
    lt.preview([flask], lt.preview_path('Items', 'SoulFlask_Top'), view=(0.35, -0.5, 1.4), fit=1.2)
    lt.preview([flask, open_flask], lt.preview_path('Items', 'SoulFlask_Pair'), view=(-0.3, -1.5, 0.5), fit=1.15)
