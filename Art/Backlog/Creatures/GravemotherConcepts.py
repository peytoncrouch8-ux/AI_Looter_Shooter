"""Three looks for the Gravemother, Ransom's Rest's Legendary monster (Docs/Areas/RansomsRest.md: Side 3, the creature
table's Gravemother and Spiderling rows, the Sink, the art needs). She is the game's spider at 1.8x, on its rig and mesh
unchanged (Art/Models/Creatures/Spider.py, the "Meadow Wolf"), in a pale hide. She lives in the den under Den Rock on the
Sink's east wall, eats what the Unpaid leave behind, and calls spiderlings: the same spider at 0.45x in its normal hide.
Concept art kept in Art/Backlog (see its README), the record of the three looks: the user picked A, Bone (2026-10-07),
which now lives in Spider.py (its BONE hide, painted into Art/Textures/SpiderBody_Pale/T_SpiderBody_Pale_BC.png). The
game's build never reads this file.

Why a second color map: the spider has one material, MI_SpiderBody on the World master, and the master's only color
control is a Tint that multiplies the base color. Tinting the brown atlas can only darken or shift it, never make it
pale. So each look is a second base-color map in the same atlas layout, painted by the same painter call
(looter_creatures.paint_atlas with Spider.py's ATLAS, size, pixel size and normal strength) through a painter that takes
roughness, height and occlusion from Spider.py's own painter and only paints the color with another palette. Its normal
and ORM maps therefore come out byte for byte the same as Art/Textures/SpiderBody's, and the game reuses those (this
script checks it every run). A is painted by Spider.py's own painter with its BONE hide (and checked against the game's
T_SpiderBody_Pale_BC when that exists); B and C by this file's copy of the painter's masks, which the brown palette
('Check') proves still match the shipped color map.

  A  Bone       a chalky bone-white hide whose markings have faded to grey ghost-lines (the heart, the chevrons, the
                carapace bands), dark rings kept on the legs; old pale-edged scars and grave dust on her back. The pick.
  B  Ash        grey-white with charcoal: charcoal chevrons and carapace bands, a soot-black heart, charcoal leg rings,
                and feet blackened as if she walks in a fire's leavings.
  C  Ivory      a clean warm ivory with one strong tell: a dark oxblood hourglass on her back (her abdomen crit) and
                oxblood joints, palps and fangs; everything else almost unmarked.

Nothing is a rarity color in a big area (she wears the game's orange Legendary tag already).

Test maps go to Intermediate/GravemotherConcepts/<Look>/ (never Art/Textures). Renders go to
Saved/ArtPreviews/RansomsRest/Concepts/Gravemother/:
  Gravemother_<X>_Golden.png   on the Sink floor in the level's golden late afternoon (sun at 247.5 degrees, 15 up),
                               a 1.8 m figure beside her and a spiderling at her feet
  Gravemother_<X>_Shade.png    at the den's mouth in the Sink's shade (the west rim takes the sun)
  Gravemother_<X>_Close.png    her head and abdomen markings up close
  Gravemother_<X>_Game.png     the den from 16 m at a player's eye height with the game's 90 degree view, 1920 x 1080
  Gravemother_Lineup.png       A, B and C at 1.8x beside the Meadow Wolf at 1.0x, a spiderling and the 1.8 m figure
  Gravemother_Atlas.png        the 1024 atlas at 1.8x from 1.6 m (Bone left, brown Meadow Wolf at 1.0x right)
Gravemother_options.png, the comparison sheet, is assembled from these renders afterwards (System.Drawing, outside
Blender). Gravemother_Final.png ('final') is the pick as the game builds it: Spider.py's mesh with the
T_SpiderBody_Pale_BC Spider.py wrote and SpiderBody's normal and ORM maps, on the Sink floor and at the den's mouth.

The den is a stand-in sized like the reworked one (a mouth 7 m wide and about 4.8 m high, 9 m deep, under a brow), in
a ring of wall 18 m round like the Sink's. In the shade shots an unseen plane takes the sun, as the Sink's west rim
does: 12 m up and 36 m off, it shades the whole floor at a 15 degree sun.

    powershell -NoProfile -File <artrun.ps1> -Script Art\\Backlog\\Creatures\\GravemotherConcepts.py
    ... -ScriptArgs maps                          paint and check the maps only
    ... -ScriptArgs A,golden,shade,close,game     some looks and shots (none named: all of them, plus lineup, atlas)
    ... -ScriptArgs A,golden,fast                 half size, few samples
    ... -ScriptArgs final                         Gravemother_Final.png (run Spider.py first, so its maps exist)
"""
import hashlib
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector, noise

import looter_creatures as lc
import looter_textures as lt

REPO = lt.REPO
SPIDER_PY = os.path.join(REPO, 'Art', 'Models', 'Creatures', 'Spider.py')
MAP_DIR = os.path.join(REPO, 'Intermediate', 'GravemotherConcepts')
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'Gravemother')
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
FAST = 'fast' in ARGV
LOOK_KEYS = ('A', 'B', 'C')
SHOT_KEYS = ('golden', 'shade', 'close', 'game')
TITLES = {'A': 'A  Bone', 'B': 'B  Ash', 'C': 'C  Ivory'}
SCALE, SPIDERLING = 1.8, 0.45
# Spider.py's paint_atlas call: SIZE comes from its namespace; these two are written into its call.
PX_M, NORMAL_STRENGTH = 0.003, 1.2
SUN_AZ, SUN_EL = 247.5, 15.0
WALL_X = 8.0                      # the den's face (the Sink's east wall), facing west


def log(*a):
    print('GRAVEMOTHER:', *a, flush=True)


# --- The spider, built by Spider.py itself with its texture painter switched off ---

def build_spider():
    """Runs Spider.py in a namespace of its own. paint_atlas is switched off while it runs, so it reads
    Art/Textures/SpiderBody but never writes it. Returns the namespace (ATLAS, SIZE, paint, spider, rig)."""
    keep_paint, keep_argv = lc.paint_atlas, sys.argv[:]
    lc.paint_atlas = lambda *a, **k: None
    sys.argv = [keep_argv[0], '--']               # no --preview: Spider.py renders nothing
    before = set(bpy.data.objects)
    ns = {'__name__': 'spider_for_gravemother', '__file__': SPIDER_PY}
    try:
        exec(compile(open(SPIDER_PY, encoding='utf-8').read(), SPIDER_PY, 'exec'), ns)
    finally:
        lc.paint_atlas, sys.argv = keep_paint, keep_argv
    for o in bpy.data.objects:
        if o not in before:
            o.hide_render = True                    # the rig, its hit hulls and the source mesh; the shots use copies
    return ns


# --- The pale color maps ---

def c(value):
    return lt.rgb(value)


# Each look is a palette for Spider.py's painter structure (base, light, belly and the dark markings) plus a few
# extras. The keys with _k are how strongly each mask mixes in. 'Check' is the shipped brown palette, which must give
# back T_SpiderBody_BC exactly. A (Bone) is Spider.py's BONE hide, painted by Spider.py's own painter.
BROWN, BROWN_DARK, BROWN_LIGHT, BROWN_BELLY = 0x6b4a2e, 0x2c1c0f, 0xb89466, 0xa08055
LOOKS = {
    'Check': dict(
        base=c(BROWN), light=c(BROWN_LIGHT), belly=c(BROWN_BELLY),
        field=c(BROWN_DARK), field_k=0.8, heart=c(BROWN_LIGHT) * 0.95, heart_k=1.0, heart_shape='lens',
        chevron=c(BROWN_LIGHT), chevron_k=0.85, speck=c(BROWN_LIGHT) * 0.9, speck_k=0.6,
        lateral=c(BROWN_DARK), lateral_k=0.85, stripe=c(BROWN_LIGHT), stripe_k=(0.9, 0.6),
        margin=c(BROWN_LIGHT) * 0.95, margin_k=0.7,
        ring=c(BROWN_DARK), ring_k=0.62, palp=c(BROWN_DARK), palp_k=0.45, joint=c(BROWN_DARK), joint_k=0.3,
        solid={'eye': 0x070606, 'bristle': 0x24170c, 'fang': 0x1a120c, 'misc': 0x4a3220}),
    # B: ash. Grey-white with charcoal markings: the brown spider's light marks turn dark, a soot-black heart.
    'B': dict(
        base=c(0xa9a59d), light=c(0xc2beb6), belly=c(0x84807a),
        field=c(0x6c6864), field_k=0.5, heart=c(0x1d1a18), heart_k=1.0, heart_shape='lens_wide',
        chevron=c(0x2a2724), chevron_k=0.9, speck=c(0x34302d), speck_k=0.45,
        lateral=c(0x37332f), lateral_k=0.8, stripe=c(0xcfcbc3), stripe_k=(0.85, 0.6),
        margin=c(0xbbb7af), margin_k=0.6, face=c(0x2c2926), face_k=0.75,
        ring=c(0x24211f), ring_k=0.82, palp=c(0x34302d), palp_k=0.72, joint=c(0x3c3834), joint_k=0.5,
        solid={'eye': 0x070606, 'bristle': 0x2a2725, 'fang': 0x141210, 'misc': 0x332f2b},
        tibia_dark=c(0x34302d), tibia_dark_k=0.85, tibia_ring=c(0xb3afa7),
        soot=c(0x151312), soot_k=0.9),
    # C: ivory with one tell. The marks of the brown spider all but gone; an oxblood hourglass on the abdomen's back
    # (her abdomen crit) and oxblood joints, palps and fangs. Oxblood, not red-orange: orange is the Legendary color.
    'C': dict(
        base=c(0xc8bb9d), light=c(0xd9cfb7), belly=c(0xa89b7f),
        field=c(0xb3a585), field_k=0.25, heart=c(0x531b13), heart_k=1.0, heart_shape='hourglass',
        chevron=c(0xae9e80), chevron_k=0.0, speck=c(0xb3a585), speck_k=0.0,
        lateral=c(0xb1a385), lateral_k=0.35, stripe=c(0xddd3bc), stripe_k=(0.4, 0.3),
        margin=c(0xd2c7ae), margin_k=0.3, face=c(0x988a6d), face_k=0.45,
        ring=c(0xa09074), ring_k=0.25, palp=c(0x4c1d17), palp_k=0.88, joint=c(0x45201a), joint_k=0.82,
        solid={'eye': 0x070606, 'bristle': 0x978869, 'fang': 0x35130d, 'misc': 0x4a201a},
        knee=c(0x45201a), knee_k=0.8),
}


def heart_mask(shape_kind, s, V, d, n):
    """The mark on the abdomen's back, in its atlas axes (s across, V along: 1 = the front; n the region's noise).
    'lens' is Spider.py's."""
    if shape_kind == 'lens':
        return ((np.abs(s) < 0.11 * np.clip(1 - ((V - 0.78) / 0.2) ** 2, 0, 1)) & (d > 0)).astype(np.float32)
    if shape_kind == 'lens_wide':
        return ((np.abs(s) < 0.15 * np.clip(1 - ((V - 0.76) / 0.23) ** 2, 0, 1)) & (d > 0)).astype(np.float32)
    # An hourglass: two triangles meeting at a narrow waist. s is about 64 cm per unit across her back, V about
    # 160 cm per unit along it (at 1x), so widths are scaled to stay in proportion.
    # Centred on the top of her back: V is latitude, and past about 0.85 the surface turns down toward the waist.
    waist, top, bottom = 0.57, 0.82, 0.31
    half = np.where(V > waist, (V - waist) / (top - waist), (waist - V) / (waist - bottom))
    width = (0.035 + 0.4 * np.clip(half, 0, 1) ** 1.1) * (1.0 + 0.08 * n)      # an uneven, grown edge
    inside = (np.abs(s) < width) & (V < top + 0.012 * n) & (V > bottom + 0.012 * n) & (d > 0)
    return inside.astype(np.float32)


def paint_color(region, U, V, seed, P):
    """Spider.py's painter, color only, with the palette P: the same masks in the same order (so the Check palette
    gives back T_SpiderBody_BC), then B's and C's extras. (A's extras, the ghost outline, the scars and the dust, went
    into Spider.py with its palette.)"""
    shape = U.shape
    base, light, belly = P['base'], P['light'], P['belly']
    n = lt.noise(shape, seed, 12.0, 12.0, octaves=3)
    if region in ('eye', 'bristle', 'fang', 'misc'):
        return np.broadcast_to(lt.rgb(P['solid'][region]), shape + (3,)).copy()
    if region in ('abdomen', 'carapace', 'head'):
        d, s = lc.body_axes(U, V)
        fur = lt.noise(shape, seed + 11, 0.7, 5.0)
        col = lt.mix(base, light, np.clip(0.25 + 0.2 * n, 0, 1))
        col = lt.mix(col, belly, lt.smooth(-0.1, -0.5, d))
        if region == 'abdomen':
            field = lt.smooth(0.62, 0.42, np.abs(s) + 0.05 * n) * lt.smooth(-0.15, 0.25, d) * lt.smooth(0.02, 0.15, V)
            col = lt.mix(col, P['field'], field * P['field_k'])
            heart = heart_mask(P['heart_shape'], s, V, d, n)
            col = lt.mix(col, P['heart'], lt.blur(heart, 1.5) * P['heart_k'])
            for k in range(4):
                vk = 0.5 - k * 0.11
                line = 1 - lt.smooth(0.012, 0.03, np.abs(V - (vk - 0.22 * np.abs(s))))
                col = lt.mix(col, P['chevron'], line * (np.abs(s) < 0.42) * (d > 0) * P['chevron_k'])
            col = lt.mix(col, P['speck'], lt.specks(shape, seed + 3, 0.04, 1.0) * P['speck_k'] * (d > -0.2))
        else:
            stripe = lt.smooth(0.15, 0.08, np.abs(s)) * (d > 0)
            lateral = lt.smooth(0.12, 0.2, np.abs(s)) * lt.smooth(0.72, 0.55, np.abs(s)) * (d > 0)
            col = lt.mix(col, P['lateral'], lateral * P['lateral_k'])
            col = lt.mix(col, P['stripe'], stripe * (P['stripe_k'][0] if region == 'carapace' else P['stripe_k'][1]))
            col = lt.mix(col, P['margin'], lt.smooth(0.75, 0.92, np.abs(s)) * (d > -0.3) * P['margin_k'])
            if 'face' in P and region == 'head':
                # A darker face round the eyes: on a pale head, black eyes alone read as a toy's.
                mask = lt.smooth(0.5, 0.74, V) * lt.smooth(-0.75, -0.25, d) * np.clip(0.85 + 0.25 * n, 0, 1)
                col = lt.mix(col, P['face'], mask * P['face_k'])
        col = col * (1.0 + 0.09 * fur)[..., None]
        return col * (1.0 + 0.05 * lt.noise(shape, seed + 5, 3.0, 3.0))[..., None]
    along, top = lc.limb_axes(U, V)
    fur = lt.noise(shape, seed + 13, 5.0, 0.7)
    col = lt.mix(base, light, np.clip(0.3 + 0.2 * n, 0, 1))
    col = lt.mix(col, belly * 1.05, lt.smooth(0.0, -0.6, top))
    if region == 'femur':
        col = lt.mix(col, P['ring'], lc.bands(along, (0.42, 0.78), 0.07) * P['ring_k'])
        if 'knee' in P:
            # The knee's colour runs back into the femur, so the joint reads as stained rather than a ball.
            col = lt.mix(col, P['knee'], lt.smooth(0.8, 0.99, along) ** 1.5 * P['knee_k'])
    elif region == 'tibia':
        if 'tibia_dark' in P:
            # The lower legs turned: charcoal with ash rings.
            col = lt.mix(col, P['tibia_dark'], P['tibia_dark_k'])
            col = lt.mix(col, P['tibia_ring'], lc.bands(along, (0.18, 0.46, 0.72), 0.05) * P['ring_k'])
        else:
            col = lt.mix(col, P['ring'], lc.bands(along, (0.18, 0.46, 0.72, 0.94), 0.05) * P['ring_k'])
        if 'knee' in P:
            col = lt.mix(col, P['knee'], lt.smooth(0.12, 0.0, along) ** 1.5 * P['knee_k'])
        if 'soot' in P:
            # Soot-black feet, smudged up the leg.
            smudge = np.clip(0.85 + 0.3 * lt.noise(shape, seed + 31, 12.0, 1.5), 0, 1)
            reach = 0.06 * lt.noise(shape, seed + 33, 1.0, 3.0)        # a ragged edge, streaking up the leg
            col = lt.mix(col, P['soot'], lt.smooth(0.62, 0.9, along + reach) * smudge * P['soot_k'])
    elif region in ('palp', 'chel'):
        col = lt.mix(col, P['palp'], P['palp_k'])
    else:
        col = lt.mix(col, P['joint'], P['joint_k'])
    col = col * (1.0 + 0.09 * fur)[..., None]
    return col


def md5(path):
    with open(path, 'rb') as f:
        return hashlib.md5(f.read()).hexdigest()


def paint_maps(ns, keys):
    """Paints each look's set with paint_atlas into MAP_DIR/<key>/T_SpiderBody_<key>_*.png and checks the result:
    its N and ORM must equal the shipped ones (so the game reuses those), the Check palette's BC must equal the shipped
    BC (so the color masks are Spider.py's), and A's BC, painted by Spider.py's own painter with its BONE hide, must
    equal the game's T_SpiderBody_Pale_BC when Spider.py has written it."""
    original = ns['paint']
    shipped = {kind: md5(lt.texture_path('SpiderBody', kind)) for kind in ('BC', 'N', 'ORM')}
    pale = os.path.join(lt.TEXTURE_DIR, 'SpiderBody_Pale', 'T_SpiderBody_Pale_BC.png')

    def painter_for(key):
        if key == 'A':
            return lambda region, U, V, k: original(region, U, V, k, ns['BONE'])

        def painter(region, U, V, k):
            _, rough, height, occl = original(region, U, V, k)
            return paint_color(region, U, V, k, LOOKS[key]), rough, height, occl
        return painter

    paths = {}
    for key in keys:
        out = os.path.join(MAP_DIR, key)
        name = f'SpiderBody_{key}'
        lc.paint_atlas(ns['ATLAS'], painter_for(key), ns['SIZE'], PX_M, out, name, normal_strength=NORMAL_STRENGTH)
        files = {kind: os.path.join(out, f'T_{name}_{kind}.png') for kind in ('BC', 'N', 'ORM')}
        same = {kind: md5(files[kind]) == shipped[kind] for kind in files}
        log(f'maps {key}: N same as shipped {same["N"]}, ORM same {same["ORM"]}, BC same {same["BC"]}')
        if not (same['N'] and same['ORM']):
            raise RuntimeError(f'{key}: the normal or ORM map differs from Art/Textures/SpiderBody')
        if key == 'Check' and not same['BC']:
            raise RuntimeError('Check: the brown palette no longer reproduces T_SpiderBody_BC')
        if key == 'A' and os.path.exists(pale):
            log(f'maps A: same as the game\'s T_SpiderBody_Pale_BC {md5(files["BC"]) == md5(pale)}')
        paths[key] = files['BC']
    return paths


def hide_material(key, bc_path):
    """The look as the game would draw it: MI_SpiderBody's material with only its base-color map swapped."""
    mat = lt.material('SpiderBody', name=f'SpiderBody_{key}')
    for node in mat.node_tree.nodes:
        if node.type == 'TEX_IMAGE' and node.image and os.path.basename(node.image.filepath) == 'T_SpiderBody_BC.png':
            image = bpy.data.images.load(bc_path, check_existing=True)
            image.colorspace_settings.name = 'sRGB'
            node.image = image
    return mat


# --- Scene helpers ---

SCENE = bpy.context.scene
PLACED = []


def white_col(mesh):
    col = mesh.color_attributes.get('Col')
    if col is None:
        col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
        col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))
    mesh.color_attributes.active_color = col


def mesh_obj(name, verts, faces, mat=None, smooth=True):
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], faces)
    me.update()
    o = bpy.data.objects.new(name, me)
    SCENE.collection.objects.link(o)
    if mat is not None:
        me.materials.append(mat)
    me.uv_layers.new(name='UVMap')
    white_col(me)
    for p in me.polygons:
        p.use_smooth = smooth
    return o


def heading_to(src, dst):
    """The Z turn that points a model's front (Blender -Y) from src toward dst."""
    dx, dy = dst[0] - src[0], dst[1] - src[1]
    return math.atan2(dx, -dy)


def place(src, location, toward, scale=1.0, mat=None, name=None):
    """A copy of src standing at location, its front toward a point, sharing src's mesh."""
    o = src.copy()
    o.name = name or ('_P_' + src.name)
    SCENE.collection.objects.link(o)
    o.parent = None
    o.modifiers.clear()
    o.location = Vector((location[0], location[1], 0.0 if len(location) < 3 else location[2]))
    o.rotation_euler = (0.0, 0.0, heading_to(location, toward))
    o.scale = (scale, scale, scale)
    if mat is not None:
        o.material_slots[0].link = 'OBJECT'
        o.material_slots[0].material = mat
    o.hide_render = False
    PLACED.append(o)
    return o


def local(o, p):
    """A point given in a placed model's own frame, in world metres (not scaled with it): x her left, y her back
    (the spider faces -Y), z up from the ground."""
    return o.location.copy() + Matrix.Rotation(o.rotation_euler.z, 4, 'Z') @ Vector(p)


def show_only(objs):
    keep = set(objs)
    for o in PLACED:
        o.hide_render = o not in keep


# --- The 1.8 m figure ---

def mannequin():
    """A plain clay figure 1.8 m tall, built in creature centimetres (x forward, y left)."""
    m = lc.Mesh({'clay': (0.0, 1.0, 0.0, 1.0)})
    e = lc.ellipsoid
    e(m, (1.0, 0.0, 168.0), (11.5, 9.0, 12.0), 'clay', fwd=(0, 0, 1), up=(1, 0, 0), segs=18, rings=12, patch=True)
    lc.tube(m, [(0.0, 0.0, 148.0), (0.5, 0.0, 160.0)], [5.8, 5.2], 'clay', sides=12, patch=True)
    e(m, (0.0, 0.0, 129.0), (22.0, 19.5, 12.0), 'clay', fwd=(0, 0, 1), up=(1, 0, 0), segs=20, rings=12, patch=True)
    e(m, (0.0, 0.0, 101.0), (14.0, 17.0, 11.0), 'clay', fwd=(0, 0, 1), up=(1, 0, 0), segs=20, rings=10, patch=True)
    for side in (1.0, -1.0):
        e(m, (0.0, side * 19.0, 144.0), (7.5, 7.0, 7.0), 'clay', segs=12, rings=8, patch=True)
        lc.tube(m, [(0.0, side * 21.0, 144.0), (-2.0, side * 24.0, 117.0), (3.0, side * 25.0, 90.0)], [5.0, 4.0, 3.3],
                'clay', sides=12, patch=True)
        e(m, (4.0, side * 25.0, 82.0), (8.0, 3.2, 4.4), 'clay', fwd=(0, 0, 1), up=(1, 0, 0), segs=10, rings=6,
          patch=True)
        lc.tube(m, [(0.0, side * 9.0, 100.0), (1.5, side * 10.0, 53.0), (0.0, side * 10.5, 9.0)], [8.0, 5.6, 4.4],
                'clay', sides=14, patch=True)
        e(m, (6.0, side * 10.5, 4.5), (13.0, 5.0, 4.5), 'clay', segs=12, rings=8, patch=True)
    clay = bpy.data.materials.new('_Clay')
    clay.use_nodes = True
    bsdf = next(n for n in clay.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x6e665d)
    bsdf.inputs['Roughness'].default_value = 0.8
    obj = m.finish('_Figure', clay)
    white_col(obj.data)
    obj.hide_render = True
    return obj


# --- The set: the Sink's floor and the den's stand-in ---

def fbm(p, octaves=4):
    total, amp, freq = 0.0, 1.0, 1.0
    for _ in range(octaves):
        total += amp * noise.noise(p * freq)
        amp, freq = amp * 0.5, freq * 2.1
    return total


PIT_R = 18.0                        # the Sink: a round pit, its east wall through the den's face
PIT_C = Vector((WALL_X - PIT_R, 0.0, 0.0))


def wall_point(y, inset=0.0):
    """The point on the Sink's ring at arc length y from the den's line (positive north), inset toward the middle."""
    a = y / PIT_R
    return PIT_C + Vector((math.cos(a), math.sin(a), 0.0)) * (PIT_R - inset)


def den_wall():
    """The Sink's wall with the den: a rock face round the pit's ring (its depth a function of the arc and the
    height, in beds, with a brow over the mouth) closed behind into a solid, and the cave cut out of it: lobes melted
    into one ellipsoid, 7 m across at the face and about 4.8 m high."""
    rock = lt.material('RockCliff', MossAmount=0.6)
    ys = np.linspace(-PIT_R * 1.45, PIT_R * 1.45, 181)
    zs = np.linspace(-0.6, 13.0, 47)

    def face_in(y, z):
        """How far the face stands in from the ring (metres, toward the pit's middle)."""
        p = Vector((y * 0.16, z * 0.16, 1.7))
        x = 0.7 * fbm(p, 3)
        bed = z / 2.4 + 0.3 * noise.noise(Vector((y * 0.09, 3.0, 0.0)))
        frac = bed - math.floor(bed)
        x -= 0.45 * lc.smoothstep(0.62, 0.9, frac) - 0.2       # each bed's top juts, its foot is undercut
        brow = lc.smoothstep(7.5, 3.5, abs(y)) * lc.smoothstep(4.0, 5.2, z) * lc.smoothstep(7.8, 6.0, z)
        x -= 1.1 * brow
        x += 0.25 * lc.smoothstep(2.0, 0.0, z)                 # a worn foot where it meets the floor
        return -x

    verts, faces = [], []
    ny, nz = len(ys), len(zs)
    for y in ys:
        for z in zs:
            verts.append(tuple(wall_point(y, face_in(y, z)) + Vector((0.0, 0.0, z))))
    for y in ys:
        for z in zs:
            verts.append(tuple(wall_point(y, -14.0) + Vector((0.0, 0.0, z))))
    off = ny * nz

    def vi(i, j, b=0):
        return b + i * nz + j
    for i in range(ny - 1):
        for j in range(nz - 1):
            faces.append((vi(i, j), vi(i, j + 1), vi(i + 1, j + 1), vi(i + 1, j)))          # the face, toward -X
            faces.append((vi(i, j, off), vi(i + 1, j, off), vi(i + 1, j + 1, off), vi(i, j + 1, off)))
    for i in range(ny - 1):            # bottom and top strips
        faces.append((vi(i, 0), vi(i + 1, 0), vi(i + 1, 0, off), vi(i, 0, off)))
        faces.append((vi(i, nz - 1), vi(i, nz - 1, off), vi(i + 1, nz - 1, off), vi(i + 1, nz - 1)))
    for j in range(nz - 1):            # the two ends
        faces.append((vi(0, j), vi(0, j, off), vi(0, j + 1, off), vi(0, j + 1)))
        faces.append((vi(ny - 1, j), vi(ny - 1, j + 1), vi(ny - 1, j + 1, off), vi(ny - 1, j, off)))
    wall = mesh_obj('_DenWall', verts, faces, rock)

    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=64, v_segments=36, radius=1.0)
    center, radii = Vector((WALL_X + 2.9, 0.0, 0.3)), Vector((6.6, 3.95, 4.85))
    for v in bm.verts:
        d = v.co.copy()
        r = 1.0 + 0.09 * fbm(d * 2.2 + Vector((5.0, 1.0, 2.0)), 3)
        v.co = center + Vector((d.x * radii.x, d.y * radii.y, d.z * radii.z)) * r
    cave_me = bpy.data.meshes.new('_Cave')
    bm.to_mesh(cave_me)
    bm.free()
    cave = bpy.data.objects.new('_Cave', cave_me)
    SCENE.collection.objects.link(cave)
    cut = wall.modifiers.new('Cave', 'BOOLEAN')
    cut.operation, cut.solver, cut.object = 'DIFFERENCE', 'EXACT', cave
    dg = bpy.context.evaluated_depsgraph_get()
    carved = bpy.data.meshes.new_from_object(wall.evaluated_get(dg))
    wall.modifiers.clear()
    old = wall.data
    wall.data = carved
    bpy.data.meshes.remove(old)
    bpy.data.objects.remove(cave)
    bpy.data.meshes.remove(cave_me)
    if not carved.materials:
        carved.materials.append(rock)
    for p in carved.polygons:
        p.use_smooth = True
    carved.set_sharp_from_angle(angle=math.radians(55.0))
    if not carved.uv_layers:
        carved.uv_layers.new(name='UVMap')
    lt.box_uv(wall, 'RockCliff')
    white_col(carved)
    return wall


def boulder(name, location, size, seed, mat):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=3, radius=1.0)
    off = Vector((seed * 3.1, seed * 1.7, seed * 0.9))
    for v in bm.verts:
        d = v.co.copy()
        v.co = Vector((d.x * size[0], d.y * size[1], d.z * size[2])) * (1.0 + 0.22 * fbm(d * 1.4 + off, 3))
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new(name, me)
    SCENE.collection.objects.link(o)
    me.materials.append(mat)
    for p in me.polygons:
        p.use_smooth = True
    me.uv_layers.new(name='UVMap')
    o.location = location
    o.rotation_euler = (0.0, 0.0, seed * 0.7)
    bpy.context.view_layer.update()
    lt.box_uv(o, 'RockCliff')
    white_col(me)
    return o


SET = []


def hide_set(hidden):
    """The den's wall and rocks off for the shots staged on open ground (the lineup, the atlas check)."""
    for o in SET:
        o.hide_render = hidden


def build_set():
    soil = lt.material('GroundDirt', name='_SinkFloor')
    ground = mesh_obj('_Floor', [(-300, -300, 0), (300, -300, 0), (300, 300, 0), (-300, 300, 0)], [(0, 1, 2, 3)], soil)
    lt.box_uv(ground, 'GroundDirt')
    wall = den_wall()
    rock = lt.material('RockCliff', MossAmount=0.6)
    rocks = [boulder('_RockA', wall_point(-6.6, 0.9) + Vector((0, 0, 0.2)), (1.5, 1.2, 1.0), 1, rock),
             boulder('_RockB', wall_point(-8.8, 1.9) + Vector((0, 0, 0.1)), (0.8, 0.7, 0.55), 2, rock),
             boulder('_RockC', wall_point(7.0, 0.8) + Vector((0, 0, 0.25)), (1.3, 1.6, 1.1), 3, rock),
             boulder('_RockD', wall_point(5.6, 2.8) + Vector((0, 0, 0.05)), (0.55, 0.5, 0.35), 4, rock),
             boulder('_RockE', wall_point(-15.0, 1.6) + Vector((0, 0, 0.1)), (1.6, 1.3, 1.0), 5, rock),
             boulder('_RockF', wall_point(14.5, 1.4) + Vector((0, 0, 0.1)), (1.2, 1.4, 0.9), 6, rock),
             boulder('_RockG', (-6.0, -6.5, 0.05), (0.6, 0.5, 0.35), 7, rock)]
    SET.extend([wall] + rocks)
    return [ground, wall] + rocks


# --- Light ---

def to_sun(az=SUN_AZ, el=SUN_EL):
    a, e = math.radians(az), math.radians(el)
    return Vector((math.sin(a) * math.cos(e), math.cos(a) * math.cos(e), math.sin(e)))


def golden_world(sun_dir):
    """The Sink stage's sky (Sink/stage.py): warm haze at the horizon, blue above, a glow round the low sun."""
    world = bpy.data.worlds.new('_GoldenSky')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -1.0
    links.new(split.outputs['Z'], remap.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    stops = [(0.0, 0x5c5242), (0.47, 0x8a7d68), (0.5, 0xe6cda0), (0.53, 0xd6d2bf), (0.62, 0xa7bccf), (1.0, 0x5a80b2)]
    el = ramp.color_ramp.elements
    el[0].position, el[0].color = stops[0][0], lt.hex_color(stops[0][1])
    el[1].position, el[1].color = stops[-1][0], lt.hex_color(stops[-1][1])
    for p, col in stops[1:-1]:
        el.new(p).color = lt.hex_color(col)
    color = ramp.outputs['Color']
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(coords.outputs['Generated'], dot.inputs[0])
    dot.inputs[1].default_value = sun_dir
    clamp = nodes.new('ShaderNodeMath')
    clamp.operation = 'MAXIMUM'
    links.new(dot.outputs['Value'], clamp.inputs[0])
    for glow, power, strength in ((0xffd9a0, 20.0, 0.9), (0xfff0cc, 500.0, 6.0)):
        pw = nodes.new('ShaderNodeMath')
        pw.operation = 'POWER'
        links.new(clamp.outputs['Value'], pw.inputs[0])
        pw.inputs[1].default_value = power
        mul = nodes.new('ShaderNodeMath')
        mul.operation = 'MULTIPLY'
        links.new(pw.outputs['Value'], mul.inputs[0])
        mul.inputs[1].default_value = strength
        add = nodes.new('ShaderNodeMix')
        add.data_type = 'RGBA'
        add.blend_type = 'ADD'
        links.new(mul.outputs['Value'], add.inputs['Factor'])
        links.new(color, add.inputs['A'])
        add.inputs['B'].default_value = lt.hex_color(glow)
        color = add.outputs['Result']
    links.new(color, bg.inputs['Color'])
    bg.inputs['Strength'].default_value = 1.0
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    return world


def build_light():
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy, sun.data.color, sun.data.angle = 4.6, (1.0, 0.78, 0.55), math.radians(1.5)
    sun.rotation_euler = (-to_sun()).to_track_quat('-Z', 'Y').to_euler()
    SCENE.collection.objects.link(sun)
    SCENE.world = golden_world(to_sun())
    # The Sink's west rim: 12 m up and 36 m off, it shades the whole floor at a 15 degree sun. Unseen by the camera.
    rim = mesh_obj('_WestRim', [(-60, -60, 0), (60, -60, 0), (60, 60, 0), (-60, 60, 0)], [(0, 1, 2, 3)])
    rim.location = Vector((0.0, 0.0, 2.0)) + to_sun() * 45.0
    rim.rotation_euler = (-to_sun()).to_track_quat('Z', 'Y').to_euler()
    rim.visible_camera = False
    rim.visible_glossy = False
    rim.hide_render = True
    return sun, rim


def setup_render(samples):
    SCENE.render.engine = 'BLENDER_EEVEE_NEXT'
    ee = SCENE.eevee
    ee.taa_render_samples = 16 if FAST else samples
    ee.use_shadows = True
    if hasattr(ee, 'use_raytracing'):
        ee.use_raytracing = True
        ee.ray_tracing_method = 'SCREEN'
    if hasattr(ee, 'use_fast_gi'):
        ee.use_fast_gi = True
    if hasattr(ee, 'shadow_resolution_scale'):
        ee.shadow_resolution_scale = 1.0
    SCENE.render.film_transparent = False
    SCENE.render.image_settings.file_format = 'PNG'
    SCENE.render.image_settings.color_mode = 'RGB'
    SCENE.view_settings.view_transform = 'AgX'
    SCENE.view_settings.look = 'AgX - Medium High Contrast'
    SCENE.view_settings.exposure = 0.0


def camera(location, target, lens=32.0):
    cam = bpy.data.objects.get('_Cam') or bpy.data.objects.new('_Cam', bpy.data.cameras.new('_Cam'))
    if cam.name not in SCENE.collection.objects:
        SCENE.collection.objects.link(cam)
    cam.location = Vector(location)
    cam.rotation_euler = (Vector(target) - Vector(location)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens = lens
    cam.data.sensor_fit = 'HORIZONTAL'
    cam.data.sensor_width = 36.0
    cam.data.clip_start, cam.data.clip_end = 0.05, 3000.0
    SCENE.camera = cam
    return cam


def render(name, size=(1600, 1000)):
    path = os.path.join(OUT, name)
    SCENE.render.resolution_x, SCENE.render.resolution_y = size
    SCENE.render.resolution_percentage = 50 if FAST else 100
    SCENE.render.filepath = path
    bpy.ops.render.render(write_still=True)
    log('rendered', path)


def label(text, location, size, toward):
    curve = bpy.data.curves.new('_Label', 'FONT')
    curve.body = text
    curve.size = size
    curve.align_x = 'CENTER'
    curve.extrude = size * 0.03
    obj = bpy.data.objects.new('_Label', curve)
    obj.location = location
    obj.rotation_euler = (math.radians(90.0), 0.0, heading_to(location, toward))
    mat = bpy.data.materials.get('_LabelInk') or bpy.data.materials.new('_LabelInk')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0xece6da)
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(0xece6da)
    bsdf.inputs['Emission Strength'].default_value = 0.6
    curve.materials.append(mat)
    SCENE.collection.objects.link(obj)
    PLACED.append(obj)
    return obj


# --- The shots ---

def build_all(keys):
    ns = build_spider()
    painted = paint_maps(ns, ['Check'] + list(keys))
    mats = {k: hide_material(k, painted[k]) for k in keys}
    mats['Brown'] = ns['MATERIAL']
    return ns, mats


def vignette(ns, mats, key, gm_at, gm_toward, figure_local, ling_local, ling_toward=None):
    """She, the figure and a spiderling, those two placed in her frame (see local)."""
    src, fig = ns['spider'], FIGURE
    gm = place(src, gm_at, gm_toward, SCALE, mats[key], name=f'_GM_{key}')
    f_at = local(gm, figure_local)
    figure = place(fig, f_at, ling_toward or gm_toward, 1.0)
    l_at = local(gm, ling_local)
    ling = place(src, l_at, ling_toward or gm_toward, SPIDERLING, mats['Brown'], name='_Spiderling')
    return [gm, figure, ling]


GOLDEN_AT, GOLDEN_TOWARD = (-2.0, 0.6), (-12.0, -1.6)
SHADE_AT, SHADE_TOWARD = (4.6, 0.3), (-12.0, -3.0)


def shot_golden(ns, mats, key, set_objs, rim):
    rim.hide_render = True
    stuff = vignette(ns, mats, key, GOLDEN_AT, GOLDEN_TOWARD, (-4.7, 0.6, 0.0), (-0.9, -3.7, 0.0), (-13.0, 5.0))
    show_only(stuff)
    setup_render(80)
    camera((-11.8, 5.6, 2.2), (-2.4, 1.0, 1.2), lens=30.0)
    render(f'Gravemother_{key}_Golden.png')


def shot_shade(ns, mats, key, set_objs, rim):
    rim.hide_render = False
    stuff = vignette(ns, mats, key, SHADE_AT, SHADE_TOWARD, (-4.4, -1.6, 0.0), (0.9, -3.9, 0.0), (-14.0, 4.0))
    show_only(stuff)
    setup_render(80)
    camera((-8.2, -5.6, 1.8), (4.4, 0.4, 1.45), lens=32.0)
    render(f'Gravemother_{key}_Shade.png')
    rim.hide_render = True


def shot_close(ns, mats, key, set_objs, rim):
    rim.hide_render = True
    gm = place(ns['spider'], GOLDEN_AT, GOLDEN_TOWARD, SCALE, mats[key], name=f'_GM_{key}')
    show_only([gm])
    setup_render(80)
    # From high on her left (the sun's side), looking down: the face and eyes in profile, the carapace's stripe and
    # the whole of the abdomen's back.
    camera(local(gm, (3.3, 1.6, 5.4)), local(gm, (0.0, 0.85, 1.0)), lens=32.0)
    render(f'Gravemother_{key}_Close.png')


def shot_game(ns, mats, key, set_objs, rim):
    """What a player sees: the den from 16 m, eye height 1.7 m, a 90 degree horizontal view, 1080p."""
    rim.hide_render = False
    gm = place(ns['spider'], (WALL_X - 2.6, 0.2), (-10.0, -1.0), SCALE, mats[key], name=f'_GM_{key}')
    ling1 = place(ns['spider'], (WALL_X - 6.3, -2.2), (-10.0, -3.0), SPIDERLING, mats['Brown'], name='_Ling1')
    ling2 = place(ns['spider'], (WALL_X - 5.6, 2.8), (-10.0, 4.0), SPIDERLING, mats['Brown'], name='_Ling2')
    show_only([gm, ling1, ling2])
    setup_render(64)
    camera((WALL_X - 2.6 - 16.0, 1.2, 1.7), (WALL_X - 2.6, 0.2, 1.2), lens=18.0)
    render(f'Gravemother_{key}_Game.png', size=(1920, 1080))
    rim.hide_render = True


def shot_lineup(ns, mats, keys, rim):
    """A, B and C at 1.8x, the Meadow Wolf at 1.0x and a spiderling, the 1.8 m figure: one light, one scale. On the
    open floor far from the Sink, seen from the south."""
    rim.hide_render = True
    y0, cx = -80.0, -24.6
    cam = Vector((cx, y0 - 34.0, 2.2))
    xs = {'A': -34.0, 'B': -27.0, 'C': -20.0, 'Brown': -14.0}
    names = dict(TITLES, Brown='Meadow Wolf 1.0x')
    stuff = []
    for k in list(keys) + ['Brown']:
        x = xs[k]
        scale = SCALE if k != 'Brown' else 1.0
        stuff.append(place(ns['spider'], (x, y0), (x - 9.0, y0 - 22.0), scale, mats[k], name=f'_Line_{k}'))
        stuff.append(label(names[k], (x, y0 - 4.0, 0.1), 0.6, (cam.x, cam.y)))
    stuff.append(place(FIGURE, (-39.4, y0 - 0.6), (-44.0, y0 - 20.0), 1.0))
    stuff.append(label('1.8 m', (-39.4, y0 - 2.8, 0.1), 0.45, (cam.x, cam.y)))
    stuff.append(place(ns['spider'], (-10.4, y0 - 1.0), (-15.0, y0 - 20.0), SPIDERLING, mats['Brown'], name='_LineLing'))
    stuff.append(label('spiderling 0.45x', (-13.6, y0 - 9.0, 0.1), 0.32, (cam.x, cam.y)))
    show_only(stuff)
    hide_set(True)
    setup_render(80)
    camera(cam, (cx, y0, 0.9), lens=36.0)
    render('Gravemother_Lineup.png', size=(2200, 760))
    hide_set(False)


def read_png(path):
    """A rendered PNG as uint8 rows, top row first."""
    image = bpy.data.images.load(path)
    w, h = image.size
    px = np.empty(w * h * 4, np.float32)
    image.pixels.foreach_get(px)
    bpy.data.images.remove(image)
    return lt.to8(px.reshape(h, w, 4)[::-1, :, :3])


def shot_atlas(ns, mats, rim):
    """How the 1024 atlas holds at 1.8x up close. Left: a pale look's abdomen and legs from her left side, the camera
    1.6 m off her abdomen at eye height (1.7 m). Right: the brown Meadow Wolf at 1.0x seen the same way from 1/1.8 the
    distance, so it fills the same part of the screen. Each half is half of a 1080p frame at the game's 90 degree view
    (a 36 mm lens on 960 x 1080), so the texel sharpness is what a player gets."""
    rim.hide_render = True
    key = 'A' if 'A' in mats else next(k for k in mats if k != 'Brown')
    # Facing north-west, so the sun comes from just behind the camera onto her left side: the legs' shadows fall away
    # behind them instead of across the abdomen.
    gm = place(ns['spider'], (-40.0, 90.0), (-45.4, 98.4), SCALE, mats[key], name='_AtlasGM')
    wolf = place(ns['spider'], (-140.0, 220.0), (-145.4, 228.4), 1.0, mats['Brown'], name='_AtlasWolf')
    show_only([gm, wolf])
    hide_set(True)
    setup_render(80)
    halves = []
    for o, s, tag in ((gm, SCALE, key), (wolf, 1.0, 'Brown')):
        # The abdomen's centre is 72 cm behind the thorax and 70 cm up at 1x; its half width is 44 cm.
        target = local(o, (0.0, 0.80 * s, 0.62 * s))
        eye = local(o, ((0.44 + 1.6 / SCALE) * s, 0.62 * s, 1.7 * s / SCALE))
        camera(eye, target, lens=36.0)
        name = f'_atlas_{tag}.png'
        render(name, size=(960, 1080))
        halves.append(read_png(os.path.join(OUT, name)))
        os.remove(os.path.join(OUT, name))
    hide_set(False)
    both = np.concatenate([halves[0], np.full((halves[0].shape[0], 4, 3), 20, np.uint8), halves[1]], axis=1)
    lt.write_png(os.path.join(OUT, 'Gravemother_Atlas.png'), both)
    log('rendered', os.path.join(OUT, 'Gravemother_Atlas.png'))


def shot_final(ns, rim):
    """The pick as the game builds it, Gravemother_Final.png: Spider.py's mesh with the T_SpiderBody_Pale_BC it wrote
    (and SpiderBody's normal and ORM maps, as MI_SpiderBody_Pale has them), on the Sink floor in the golden light and at
    the den's mouth in the shade, side by side."""
    pale = os.path.join(lt.TEXTURE_DIR, 'SpiderBody_Pale', 'T_SpiderBody_Pale_BC.png')
    if not os.path.exists(pale):
        raise RuntimeError(f'{pale} is missing: run Art/Models/Creatures/Spider.py first')
    log(f'final: {pale} md5 {md5(pale)}')
    mats = {'Pale': hide_material('Pale', pale), 'Brown': ns['MATERIAL']}
    shot_golden(ns, mats, 'Pale', SET, rim)
    shot_shade(ns, mats, 'Pale', SET, rim)
    halves = []
    for name in ('Gravemother_Pale_Golden.png', 'Gravemother_Pale_Shade.png'):
        halves.append(read_png(os.path.join(OUT, name)))
        os.remove(os.path.join(OUT, name))
    both = np.concatenate([halves[0], np.full((halves[0].shape[0], 6, 3), 20, np.uint8), halves[1]], axis=1)
    lt.write_png(os.path.join(OUT, 'Gravemother_Final.png'), both)
    log('rendered', os.path.join(OUT, 'Gravemother_Final.png'))


FIGURE = None


def main():
    global FIGURE
    keys = [k for k in LOOK_KEYS if k in ARGV] or list(LOOK_KEYS)
    os.makedirs(OUT, exist_ok=True)
    if 'final' in ARGV:
        ns = build_spider()
        FIGURE = mannequin()
        build_set()
        _, rim = build_light()
        shot_final(ns, rim)
        return
    if 'maps' in ARGV:
        ns = build_spider()
        paint_maps(ns, ['Check'] + keys)
        return
    ns, mats = build_all(keys)
    FIGURE = mannequin()
    set_objs = build_set()
    _, rim = build_light()
    shots = [s for s in SHOT_KEYS if s in ARGV]
    extra = [s for s in ('lineup', 'atlas') if s in ARGV]
    if not shots and not extra:
        shots, extra = list(SHOT_KEYS), ['lineup', 'atlas']
    table = {'golden': shot_golden, 'shade': shot_shade, 'close': shot_close, 'game': shot_game}
    for key in keys:
        for s in shots:
            table[s](ns, mats, key, set_objs, rim)
    if 'lineup' in extra:
        shot_lineup(ns, mats, keys, rim)
    if 'atlas' in extra:
        shot_atlas(ns, mats, rim)


main()
