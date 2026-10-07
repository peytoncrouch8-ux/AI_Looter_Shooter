"""Three concepts for Amos Whitlock, the friendly ghost of Side 2, "Unfinished Business" (Docs/Areas/RansomsRest.md:
the Side 2 mission, the NPC table and the art needs; Docs/Story.md). Amos died last harvest with his hay half in. He was
still on the Sundown Road when the saint went dark, and he drifted home to find it rotting in the field. He isn't angry
yet. Ellis talks to him at his fence in Whitlock Fields, by the gate where the fields path passes (only Ellis can see
him), and at the end he sits on his fence to wait for the saint to come back. Concept art kept in Art/Backlog (see its
README): nothing in the game uses it. The pick becomes a game model on the Unpaid's rig, as Abel did.

  A  The hayman: caught in the middle of the work. A wide straw hat with a ragged, unbound brim and a stalk in its band,
     a collarless linen shirt with the sleeves rolled past the elbows and chaff stuck to his sweat, one brace slipped off
     his shoulder for the heat, duck trousers patched with flour sack, his hay fork; a straw between his teeth. About
     fifty, a sun-creased face with a few days' stubble and sun-bleached hair.
  B  The rancher: the man who hired the hands. A creased felt hat, a hip-length duck chore coat worn open, its corduroy
     collar turned down, over a bleached shirt and a dark bandana; his work gloves stuffed in the coat's pocket, a tally
     book and a pencil stub in the other. Nearer sixty, broad and steady, sandy mutton-chop whiskers joined to his
     moustache.
  C  The old hand: the oldest Amos. A flat cap, washed denim bib overalls over an oat henley, the stoneware water jug he
     carried out to the field, a watch chain and a pencil on his bib, a red kerchief in his back pocket. About seventy,
     stooped, with a white chin-curtain beard and the kindest face of the three.

Every option is the Unpaid's kin, as the doc fixes him ("the Unpaid rig in work clothes, his coal banked low"): the
masked, dithered ghost material of the Unpaid concepts (UnpaidConcepts.py, whose toolkit this script borrows; the four
tint zones in vertex R, cavity in G, the coal's ember in B, the fade in A), the coal on the left of his chest, and below
the knees, where his trousers fray away, the Unpaid's pale shroud: lacy strips that trail back off him as a tail. But
his face is a living man's, tired and patient, the eyes creased, the mouth closed in a half smile, and his coal is
banked: a small coal under grey ash with a dull, low ember in its cracks, where the Unpaid's burns an angry red. It stays
his crit spot only if he ever turns. Every farmer's neck is creased and leathered at the back by the sun.

Renders go to Saved/ArtPreviews/RansomsRest/Concepts/Amos/, in the level's golden late afternoon (the sun at 247.5
degrees, 15 up, as the Farmhouse previews light it): per option leaning his forearms on the top rail of the game's fence
(Art/Models/Props/Fences.py's FenceRail) with his half-in hay behind him (Amos_A_lean.png), sitting on the fence
(_sit), front, side and back beside an Unpaid and a 1.8 m post (_front) and a face close-up (_face); and
Amos_options.png, the three side by side at the same scale in the same light, beside an Unpaid.

    powershell -NoProfile -File <artrun.ps1> -Script Art\\Backlog\\Creatures\\AmosConcepts.py -ScriptArgs A,lean
    ... -ScriptArgs A,B,C,lean,sit,front,face,options      (no shot named: all of them)
    ... -ScriptArgs A,debug,work=<folder>                  quick checks into work (default Intermediate/AmosConcepts)
    ... -ScriptArgs A,lean,fast,work=<folder>              half-size, few samples, into work
"""
import math
import os
import sys
import time

import bpy
import numpy as np
from mathutils import Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import UnpaidConcepts as uc  # noqa: E402  (its toolkit, its ghost and the Unpaid for scale; importing renders nothing)
import looter_textures as lt  # noqa: E402

OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'Amos')
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
WORK = next((a[5:] for a in ARGV if a.startswith('work=')), os.path.join(lt.REPO, 'Intermediate', 'AmosConcepts'))
OPTIONS = ('A', 'B', 'C')
SHOTS = ('lean', 'sit', 'front', 'face', 'options')
TITLES = {'A': 'A  The hayman', 'B': 'B  The rancher', 'C': 'C  The old hand'}
FAST = 'fast' in ARGV
SIDES = (1.0, -1.0)                     # +1 his left (+X), -1 his right
SKIN, CLOTH1, CLOTH2, ACCENT = 0, 1, 2, 3
smoothstep, unit, rotation, wrap, spline = uc.smoothstep, uc.unit, uc.rotation, uc.wrap, uc.spline

# The zones (skin and shroud, cloth 1, cloth 2, accent), pale and faded as the Unpaid's: none is a rarity color, and
# none repeats the Unpaid's three tints (chambray, Sunday cream and charcoal, harvest brown and drab) or Abel's black.
SKIN_TINT = 0xbac4c6                    # the Unpaid's approved ghost skin (Unpaid.py's ZONE_COLORS)
TINTS = {'A': (SKIN_TINT, 0xd3c8ad, 0x85725c, 0x4b3f37),   # unbleached linen (and his sun-bleached hair), brown duck, dark braces
         'B': (SKIN_TINT, 0xd5d2c9, 0x9f8e6e, 0x4a4038),   # bleached work shirt, duck canvas (and his sandy whiskers), dark brown
         'C': (0xc0c8c8, 0xd2cab6, 0x7d888f, 0x8d5f51)}    # paler skin (and his white hair), oat henley, washed denim, faded red
# The hats are their own meshes (as SM_UnpaidHat) with their own instance; A's straw takes the Hay set's grain.
HAT_LOOK = {'A': ((0xd9c891, 0xd9c891, 0x4b3f37, 0x4b3f37), 'Hay', 3.2),
            'B': ((0x5d544b, 0x5d544b, 0x3b342f, 0x3b342f), 'Polymer', 3.0),
            'C': ((0x7a7262, 0x7a7262, 0x575046, 0x575046), 'Polymer', 5.0)}
FORK_TINT = (0xa89777, 0xa89777, 0x8e959a, 0x8e959a)       # ash handle, iron
JUG_TINT = (0xd6cdb7, 0xd6cdb7, 0x6f5341, 0x8e765a)        # cream glaze, the brown-dipped neck, the cork
# The coal banked low: a small coal under grey ash, a dull brick-red ember in its cracks (the Unpaid's Basic coal is
# #B02A18 at 6, an angry red), and a narrow scorch round its hole that barely glows.
EMBER = (0x96421f, 0.8)
ASH = 0x6d6761
COAL_RADIUS = 0.038
HOLE_RADIUS = 0.046
COAL_AT = (0.25, 1.33)                  # the coal: angle round the chest from the front, height (the rig's coal bone)


def log(message):
    print(f'AMOS: {message}', flush=True)


def g(x, s):
    return np.exp(-(np.asarray(x, float) / s) ** 2)


def rot_x(degrees):
    """A turn about +X: an upright part leans forward (its front, -Y, goes down) for positive degrees."""
    return rotation((1.0, 0.0, 0.0), math.radians(degrees))


def rot_y(degrees):
    return rotation((0.0, 1.0, 0.0), math.radians(degrees))


def rot_z(degrees):
    """A turn about +Z: the face (-Y) swings toward +X, his left, for positive degrees."""
    return rotation((0.0, 0.0, 1.0), math.radians(degrees))


def outward(theta):
    return np.stack([np.sin(theta), -np.cos(theta), np.zeros_like(theta)], -1)


def surface(profile, z, theta, grow=0.0):
    return uc.torso_points(profile, z, theta, grow)


def tube(C, rx, ry=None, segs=16, up=None, domes=4, pointed=0.0):
    """uc.tube, its frames started square to the tube's first step even when that runs up (a fork's shaft, a cork)."""
    C = np.asarray(C, float)
    if up is None:
        t0 = unit(C[min(1, len(C) - 1)] - C[0])
        up = (0.0, 0.0, 1.0) if abs(t0[2]) < 0.9 else (0.0, 1.0, 0.0)
    return uc.tube(C, rx, ry, segs=segs, up=up, domes=domes, pointed=pointed)


def frame_matrix(origin, x, y, z):
    M = np.eye(4)
    M[:3, 0], M[:3, 1], M[:3, 2], M[:3, 3] = x, y, z, origin
    return M


def apply(M, V):
    V = np.asarray(V, float)
    return V @ M[:3, :3].T + M[:3, 3]


# --- Parts with a zone per vertex (the face's hair and beard, the jug's dipped neck) ---

class ZPart(uc.Part):
    """uc.Part with an optional tint zone per vertex: the head is one surface whose hair and beard take a cloth zone."""

    def __init__(self, name, zone=0):
        super().__init__(name, zone)
        self.zones = []

    def add(self, piece, fade=1.0, cavity=0.0, ember=0.0, zone=None):
        super().add(piece, fade, cavity, ember)
        k = len(self.V[-1])
        self.zones.append(np.broadcast_to(np.asarray(self.zone if zone is None else zone, float), (k,)).copy())
        return self


class Amos(uc.Figure):
    def __init__(self, key, pose):
        super().__init__(key, pose)
        self.marks = {}          # named points (the seat, the knees, the hands' grips), for staging


def build_part(part, material, where, solidify=0.0, location=(0.0, 0.0, 0.0), turn=0.0, shadow=True, **_):
    """uc.build, reading a zone per vertex where the part has them."""
    V = np.concatenate(part.V)
    mesh = uc.to_mesh(part.name, V, part.F)
    mesh.validate()
    attr = np.concatenate(part.attr)
    zones = np.concatenate(part.zones) if getattr(part, 'zones', None) else np.full(len(V), float(part.zone))
    rgba = np.column_stack([zones / 3.0, attr[:, 1], attr[:, 2], attr[:, 0]])
    col = mesh.color_attributes.new('Ghost', 'FLOAT_COLOR', 'POINT')
    col.data.foreach_set('color', rgba.astype(np.float32).ravel())
    mesh.polygons.foreach_set('use_smooth', np.ones(len(mesh.polygons), dtype=bool))
    mesh.materials.append(material)
    obj = bpy.data.objects.new(part.name, mesh)
    where.objects.link(obj)
    if solidify:
        mod = obj.modifiers.new('Thickness', 'SOLIDIFY')
        mod.thickness = solidify
        mod.offset = -1.0
        mod.use_even_offset = False
        mod.use_rim = True
    obj.location = location
    obj.rotation_euler = (0.0, 0.0, turn)
    obj.visible_shadow = shadow
    return obj


# --- Materials: the Unpaid's ghost, with the grain set, the rim and the ember edge as parameters ---

_materials = {}


def ghost_material(name, colors, ember=EMBER, grain='Polymer', grain_scale=3.0, rim=2.0, glow=0.09, edge=0.6, power=2.8):
    """UnpaidConcepts.ghost_material (the game's M_Ghost) with its grain set (BaseColorMap: Polymer, or Hay for the
    straw hat), the ember edge's strength (EmberStrength: low, the coal is banked) and the rim as parameters."""
    key = (name, tuple(colors), ember, grain, grain_scale, rim, glow, edge, power)
    if key in _materials:
        return _materials[key]
    nodes_mat = bpy.data.materials.new(name)
    mat = nodes_mat
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    attr = nodes.new('ShaderNodeAttribute')
    attr.attribute_name = 'Ghost'
    sep = nodes.new('ShaderNodeSeparateColor')
    links.new(attr.outputs['Color'], sep.inputs['Color'])
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.interpolation = 'CONSTANT'
    elements = ramp.color_ramp.elements
    elements[0].position, elements[0].color = 0.0, lt.hex_color(colors[0])
    elements[1].position, elements[1].color = 0.17, lt.hex_color(colors[1])
    for position, color in ((0.5, colors[2]), (0.83, colors[3])):
        elements.new(position).color = lt.hex_color(color)
    links.new(sep.outputs['Red'], ramp.inputs['Fac'])
    coords = nodes.new('ShaderNodeTexCoord')
    cloth = nodes.new('ShaderNodeMapping')
    cloth.inputs['Scale'].default_value = (grain_scale,) * 3
    links.new(coords.outputs['Object'], cloth.inputs['Vector'])
    tex = uc._image(nodes, links, lt.texture_path(grain, 'BC'), 'sRGB', cloth.outputs['Vector'])
    # M_Ghost reads the grain by its brightness (clamped 0.5..1.5 round the zone's own value), so a straw set's strands
    # show on the hat without its color.
    gray = nodes.new('ShaderNodeRGBToBW')
    links.new(tex.outputs['Color'], gray.inputs['Color'])
    level = uc._math(nodes, links, 'MULTIPLY', gray.outputs['Val'], 1.25 if grain == 'Polymer' else 1.7)
    level = uc._math(nodes, links, 'MAXIMUM', level, 0.5)
    level = uc._math(nodes, links, 'MINIMUM', level, 1.5)
    base = uc._vscale(nodes, links, ramp.outputs['Color'], level)
    dark = uc._math(nodes, links, 'MULTIPLY_ADD', sep.outputs['Green'], -0.93)
    dark.node.inputs[2].default_value = 1.0
    base = uc._vscale(nodes, links, base, dark)
    links.new(base, bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = 0.86
    bump = nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.12 if grain == 'Polymer' else 0.3
    bump.inputs['Distance'].default_value = 0.002
    links.new(gray.outputs['Val'], bump.inputs['Height'])
    links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    facing = nodes.new('ShaderNodeLayerWeight')
    facing.inputs['Blend'].default_value = 0.35
    rim_k = uc._math(nodes, links, 'POWER', facing.outputs['Facing'], power)
    rim_k = uc._math(nodes, links, 'MULTIPLY', rim_k, dark)
    rim_k = uc._math(nodes, links, 'MULTIPLY', rim_k, rim)
    rim_c = uc._vscale(nodes, links, uc._color(nodes, uc.RIM), rim_k)
    glow_c = uc._vscale(nodes, links, base, glow)
    ember_c = uc._vscale(nodes, links, uc._color(nodes, ember[0]),
                         uc._math(nodes, links, 'MULTIPLY', sep.outputs['Blue'], ember[1] * edge))
    links.new(uc._vadd(nodes, links, uc._vadd(nodes, links, rim_c, glow_c), ember_c), bsdf.inputs['Emission Color'])
    bsdf.inputs['Emission Strength'].default_value = 1.0
    fade_map = nodes.new('ShaderNodeMapping')
    fade_map.inputs['Scale'].default_value = (1.4, 1.4, 1.4)
    links.new(coords.outputs['Object'], fade_map.inputs['Vector'])
    fine_map = nodes.new('ShaderNodeMapping')
    fine_map.inputs['Scale'].default_value = (4.3, 4.3, 4.3)
    fine_map.inputs['Location'].default_value = (0.37, 0.11, 0.53)
    links.new(coords.outputs['Object'], fine_map.inputs['Vector'])
    path = os.path.join(lt.TEXTURE_DIR, 'MacroNoise', 'T_MacroNoise_M.png')
    n1 = uc._image(nodes, links, path, 'Non-Color', fade_map.outputs['Vector'])
    n2 = uc._image(nodes, links, path, 'Non-Color', fine_map.outputs['Vector'])
    n = uc._math(nodes, links, 'ADD', uc._math(nodes, links, 'MULTIPLY', n1.outputs['Color'], 0.62),
                 uc._math(nodes, links, 'MULTIPLY', n2.outputs['Color'], 0.38))
    fade = uc._math(nodes, links, 'MULTIPLY_ADD', attr.outputs['Alpha'], 1.3)
    fade.node.inputs[2].default_value = -0.15
    alpha = uc._math(nodes, links, 'SUBTRACT', fade, n)
    alpha = uc._math(nodes, links, 'MULTIPLY_ADD', alpha, 14.0, clamp=True)
    alpha.node.inputs[2].default_value = 0.5
    links.new(alpha, bsdf.inputs['Alpha'])
    mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = False
    mat.use_transparent_shadow = True
    _materials[key] = mat
    return mat


def banked_coal_material():
    """The coal banked low: grey ash over it, thin cracks with a dull ember in them, warmest where it faces the eye.
    (In the game M_Ghost's coal crust is a fixed near-black; an ash crust needs it as a parameter.)"""
    if 'coal' in _materials:
        return _materials['coal']
    color, strength = EMBER
    mat = bpy.data.materials.new('BankedCoal')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(ASH)
    bsdf.inputs['Roughness'].default_value = 0.95
    coords = nodes.new('ShaderNodeTexCoord')
    vor = nodes.new('ShaderNodeTexVoronoi')
    vor.feature = 'DISTANCE_TO_EDGE'
    vor.inputs['Scale'].default_value = 34.0
    links.new(coords.outputs['Object'], vor.inputs['Vector'])
    crack = nodes.new('ShaderNodeMapRange')
    crack.inputs['From Min'].default_value = 0.0
    crack.inputs['From Max'].default_value = 0.035
    crack.inputs['To Min'].default_value = 1.0
    crack.inputs['To Max'].default_value = 0.0
    links.new(vor.outputs['Distance'], crack.inputs['Value'])
    facing = nodes.new('ShaderNodeLayerWeight')
    facing.inputs['Blend'].default_value = 0.5
    hot = uc._math(nodes, links, 'MULTIPLY_ADD', facing.outputs['Facing'], -0.7)
    hot.node.inputs[2].default_value = 1.0
    k = uc._math(nodes, links, 'MULTIPLY', crack.outputs['Result'], hot)
    k = uc._math(nodes, links, 'ADD', uc._math(nodes, links, 'MULTIPLY', k, strength * 2.2), 0.12)
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(color)
    links.new(k, bsdf.inputs['Emission Strength'])
    _materials['coal'] = mat
    return mat


# --- The body under the clothes ---

# Torso profiles (z, half width, half depth, y of its middle, superellipse power), in the Unpaid's frame: front -Y,
# origin on the ground, the pelvis (the float height) at 1 m. A: a lean, wiry homesteader (UnpaidConcepts' A, the
# Unpaid before the hunger took him); B: broad and solid; C: an old man, narrower in the shoulder, a little belly.
TORSO = {
    'A': [(0.90, 0.168, 0.112, 0.000, 2.2), (0.98, 0.161, 0.108, 0.000, 2.2), (1.06, 0.151, 0.104, -0.004, 2.2),
          (1.16, 0.157, 0.110, -0.010, 2.3), (1.26, 0.171, 0.119, -0.012, 2.4), (1.34, 0.183, 0.121, -0.008, 2.6),
          (1.40, 0.190, 0.114, -0.002, 2.8), (1.45, 0.188, 0.104, 0.004, 3.2), (1.475, 0.176, 0.096, 0.008, 3.4),
          (1.50, 0.149, 0.088, 0.010, 3.0), (1.52, 0.110, 0.078, 0.010, 2.6), (1.54, 0.075, 0.068, 0.010, 2.2)],
    'B': [(0.90, 0.172, 0.118, 0.000, 2.2), (0.98, 0.168, 0.116, -0.004, 2.2), (1.06, 0.164, 0.114, -0.008, 2.2),
          (1.16, 0.168, 0.116, -0.010, 2.3), (1.26, 0.180, 0.122, -0.010, 2.4), (1.34, 0.192, 0.125, -0.010, 2.6),
          (1.40, 0.199, 0.119, -0.006, 2.8), (1.45, 0.197, 0.107, -0.002, 3.2), (1.475, 0.185, 0.099, 0.000, 3.4),
          (1.50, 0.156, 0.091, 0.000, 3.0), (1.52, 0.116, 0.081, -0.002, 2.6), (1.54, 0.079, 0.071, -0.004, 2.2)],
    'C': [(0.90, 0.166, 0.118, -0.006, 2.2), (0.98, 0.165, 0.124, -0.014, 2.2), (1.06, 0.161, 0.126, -0.018, 2.2),
          (1.16, 0.158, 0.118, -0.012, 2.3), (1.26, 0.162, 0.111, -0.004, 2.4), (1.34, 0.169, 0.107, 0.004, 2.6),
          (1.40, 0.173, 0.101, 0.010, 2.8), (1.45, 0.170, 0.095, 0.014, 3.2), (1.475, 0.159, 0.089, 0.016, 3.4),
          (1.50, 0.133, 0.082, 0.016, 3.0), (1.52, 0.097, 0.072, 0.014, 2.6), (1.54, 0.066, 0.062, 0.012, 2.2)],
}
SHOULDER = {'A': {s: np.array([s * 0.17, 0.004, 1.43]) for s in SIDES},
            'B': {s: np.array([s * 0.178, -0.004, 1.444]) for s in SIDES},
            'C': {s: np.array([s * 0.162, 0.008, 1.42]) for s in SIDES}}
PIVOT = {'A': np.array([0.0, -0.02, 1.615]), 'B': np.array([0.0, -0.026, 1.622]), 'C': np.array([0.0, -0.012, 1.6])}
HIP = 0.9                               # the hip joints' height: the thighs swing from here


def torso_core(fig, profile):
    """The body under the clothes, in the skin: it shows in the shirt's open neck and through the coal's hole, where it
    sinks into a shallow crater round the coal, scorched and faintly warm at its rim."""
    theta = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    z = np.linspace(0.92, 1.55, 40)
    piece = uc.closed_loft(surface(profile, z, theta))
    tc, zc = COAL_AT
    pc = surface(profile, [zc], np.array([tc]))[0, 0]
    nc = uc.torso_normal(tc)
    piece = uc.push(piece, pc, COAL_RADIUS * 1.35, -0.016)
    dist = np.linalg.norm(piece[0] - pc, axis=1)
    cavity = smoothstep(COAL_RADIUS * 1.3, COAL_RADIUS * 0.7, dist) * 0.85
    ember = (smoothstep(COAL_RADIUS * 0.8, COAL_RADIUS * 1.1, dist) * smoothstep(COAL_RADIUS * 1.5, COAL_RADIUS * 1.2, dist)
             * (0.3 + 0.7 * (0.5 + 0.5 * uc.pnoise(piece[0], 60.0, 5))))
    fig.add(ZPart('Body', SKIN).add(piece, cavity=cavity, ember=ember))
    fig.coal = (pc - nc * 0.006, COAL_RADIUS)


def shell(profile, z_lo, z_hi, sd_fn, folds_fn, grow=0.0, rows=150, cols=240, hole=HOLE_RADIUS):
    """A garment round the body: the profile grown by grow, folded by folds_fn(theta, z), cut where sd_fn > 0 and
    through at the coal (a ragged hole with a scorched edge). Returns (V, faces), each vertex's theta, z and its
    distance from the hole's edge."""
    theta = np.linspace(-np.pi, np.pi, cols, endpoint=False)
    z = np.linspace(z_lo, z_hi, rows)
    th, zz = np.meshgrid(theta, z)
    P = surface(profile, z, theta, grow) + outward(theta)[None, :, :] * folds_fn(th, zz)[..., None]
    sd = sd_fn(th, zz)
    tc, zc = COAL_AT
    ang = np.arctan2(zz - zc, (th - tc) * 0.17)
    rh = hole * (1.0 + 0.14 * np.sin(ang * 5 + 1.0) + 0.09 * np.sin(ang * 11.0))
    gap = np.hypot((th - tc) * 0.17, zz - zc)
    if hole:
        sd = np.maximum(sd, rh - gap)
    V, F, used = uc.cut(P, sd)
    return (V, F), th.ravel()[used], zz.ravel()[used], (gap - rh).ravel()[used]


def scorch(edge, V, seed=2):
    """The ember edge round the coal's hole: narrow, and only warm in patches (banked)."""
    return np.clip(smoothstep(0.016, 0.0, edge) * (0.25 + 0.75 * (0.5 + 0.5 * uc.pnoise(V, 50.0, seed))), 0.0, 1.0)


def scorch_cavity(edge):
    return 0.55 * smoothstep(0.02, 0.0, edge)


def ribbon_on(part, profile, keys, width, grow, n=40, cols=4, curl=0.05, fade=1.0):
    """A strap or band lying on a body surface: keys are (theta, z) points it passes through, width a number or rows
    of (t, width)."""
    pts = [surface(profile, [z], np.array([t]), grow)[0, 0] for t, z in keys]
    uc.ribbon(part, pts, width, n=n, cols=cols, out=lambda C: outward(np.arctan2(C[:, 0], -C[:, 1])), curl=curl)
    return part


def buttons(part, profile, points, grow, size=0.0068):
    for t, z in points:
        p = surface(profile, [z], np.array([t]), grow)[0, 0]
        n = uc.torso_normal(t)
        part.add(uc.ellipsoid(p, (size, size, size * 0.6), np.array([[0, 0, 1.0], np.cross(n, (0, 0, 1.0)), n]), 12, 8),
                 cavity=0.4)


def patch(part, profile, t0, t1, z0, z1, grow, cavity_edge=0.5, rows=8, cols=10):
    """A sewn-on patch or pocket: a little panel standing off the cloth, its edge darkened like stitching."""
    ts = np.linspace(t0, t1, cols)
    zs = np.linspace(z0, z1, rows)
    P = surface(profile, zs, ts, grow)
    T, Z = np.meshgrid(np.linspace(0, 1, cols), np.linspace(0, 1, rows))
    edge = np.minimum(np.minimum(T, 1 - T) * (t1 - t0) * 0.17, np.minimum(Z, 1 - Z) * (z1 - z0))
    part.add((P.reshape(-1, 3), [uc.grid_faces(rows, cols, closed=False)]), cavity=(cavity_edge * smoothstep(0.006, 0.0, edge)).ravel())


def chaff(part, V, count, seed, region, length=(0.012, 0.035)):
    """Bits of straw caught on the cloth: short thin stalks lying on the surface near the vertices V where region(V)."""
    rng = np.random.default_rng(seed)
    pts = V[region(V)]
    if not len(pts):
        return
    for p in pts[rng.choice(len(pts), size=min(count, len(pts)), replace=False)]:
        out = unit(np.array([p[0], p[1] + 0.01, 0.0]) + np.array([0.0, 0.0, 0.4]) * rng.uniform(0, 1))
        d = unit(np.cross(out, rng.normal(size=3)))
        L = rng.uniform(*length)
        base = p + out * 0.004
        part.add(tube([base - d * L / 2, base + d * L / 2 + out * rng.uniform(-0.002, 0.004)], [0.0011, 0.0008],
                         segs=6, domes=1), cavity=0.08)


# --- The face: a living man's, tired and patient, never the Unpaid's hunger ---

ET = 0.36                               # the eyes, either side of the front (angle round the head)
EYE_W = 0.0145                          # half an eye's width
FACE_R = 0.086                          # meters of face per radian round it at the eyes

FACES = {
    # A, about fifty: lean, a sun-creased face, a patient half smile, a few days' stubble, shaggy sun-bleached hair.
    'A': dict(width=0.146, front=0.097, back=0.106, top=0.118, bottom=0.13, mouth=-0.07, brow=0.011, bags=0.0012,
              nose=0.027, cheek=0.0065, hollow=0.0024, jaw=0.006, chin=0.0085, taper=0.09, naso=0.0015, smile=0.55,
              worry=0.35, squint=0.55, heavy=0.55, age=0.6, jowl=0.0, brow_hair=0.0032, droop=0.45, lips=1.0,
              beard='stubble', hair='shaggy', hair_zone=CLOTH1),
    # B, nearer sixty: broad, heavy-browed, steady; sandy mutton chops going grey, joined to a full moustache.
    'B': dict(width=0.152, front=0.099, back=0.108, top=0.12, bottom=0.135, mouth=-0.071, brow=0.0135, bags=0.0016,
              nose=0.029, cheek=0.007, hollow=0.0018, jaw=0.0072, chin=0.0095, taper=0.075, naso=0.002, smile=0.45,
              worry=0.25, squint=0.5, heavy=0.6, age=0.8, jowl=0.0016, brow_hair=0.0042, droop=0.5, lips=0.9,
              beard='chops', hair='short', hair_zone=CLOTH2),
    # C, about seventy: a long nose, hollow cheeks and jowls, deep smile lines, a white chin-curtain beard.
    'C': dict(width=0.144, front=0.098, back=0.106, top=0.116, bottom=0.132, mouth=-0.072, brow=0.0115, bags=0.002,
              nose=0.031, cheek=0.006, hollow=0.0036, jaw=0.005, chin=0.0075, taper=0.095, naso=0.0024, smile=0.8,
              worry=0.3, squint=0.75, heavy=0.65, age=1.0, jowl=0.0032, brow_hair=0.0046, droop=0.6, lips=0.75,
              beard='curtain', hair='thin', hair_zone=SKIN),
}


def face_radius(d, F):
    """The head's radius along unit directions d (head space: origin mid-head at eye height, front -Y), after
    Art/Models/Creatures/Abel.py's face: a skull flatter in front than an egg, the nape tucked in, the jaw narrowing to
    a broad chin; the brow ridge and the orbits; a straight nose with its bridge, tip and wings; cheekbones and their
    arches, a little hollow under them, temples; the mouth's arch with the philtrum and both lips, the hollow under the
    lower lip; the jaw's angle, jowls with age; the folds from the nose past the corners of the mouth."""
    TH = np.arctan2(d[..., 0], -d[..., 1])
    a = F['width'] / 2.0
    b = np.where(d[..., 1] < 0, F['front'], F['back'])
    c = np.where(d[..., 2] > 0, F['top'], F['bottom'])
    p = np.where(d[..., 1] < 0, 2.35, 2.0)
    r = 1.0 / ((np.abs(d[..., 0]) / a) ** p + (np.abs(d[..., 1]) / b) ** p + (np.abs(d[..., 2]) / c) ** p) ** (1.0 / p)
    z = d[..., 2] * r
    at = np.abs(TH)
    zm = F['mouth']
    r = r * (1.0 - 0.2 * smoothstep(-0.03, -0.11, z) * smoothstep(1.9, 2.8, at))
    # The face narrower than the skull below the cheekbones, the jaw a U to a broad chin (not a V).
    r = r * (1.0 - F['taper'] * smoothstep(zm + 0.02, -F['bottom'], z) * smoothstep(2.2, 1.0, at) * np.abs(np.sin(TH)) ** 1.6)
    r = r * (1.0 - 0.05 * smoothstep(-0.005, -0.05, z) * smoothstep(0.35, 1.0, at) * smoothstep(2.0, 1.3, at))
    brow_z = 0.021 + 0.004 * F['worry'] * g(TH, 0.32)
    R = F['brow'] * g(TH, 0.72) * g(z - brow_z, 0.012)
    R = R - 0.0065 * (g(TH - ET, 0.17) + g(TH + ET, 0.17)) * g(z - 0.002, 0.016)        # the orbits
    R = R + F['bags'] * (g(TH - ET - 0.03, 0.12) + g(TH + ET + 0.03, 0.12)) * g(z + 0.0165, 0.0045)
    nose = (F['nose'] * smoothstep(0.014, -0.034, z) * smoothstep(-0.056, -0.042, z)
            * g(TH, 0.06 + 0.065 * smoothstep(-0.01, -0.046, z)))
    R = R + nose + 0.0016 * g(TH, 0.05) * g(z + 0.011, 0.007) + 0.0034 * g(TH, 0.085) * g(z + 0.04, 0.0065)
    R = R + 0.0042 * (g(TH - 0.095, 0.036) + g(TH + 0.095, 0.036)) * g(z + 0.0435, 0.0058)    # the wings
    R = R + (F['cheek'] * g(at - 0.66, 0.22) * g(z + 0.02, 0.016)
             + 0.003 * g(z + 0.018, 0.009) * smoothstep(0.6, 0.8, at) * smoothstep(1.5, 1.25, at)
             - F['hollow'] * g(at - 0.68, 0.2) * g(z + 0.058, 0.018)
             - 0.005 * g(at - 1.05, 0.22) * g(z - 0.04, 0.025)
             + 0.0068 * g(TH, 0.5) * g(z + 0.068, 0.028)
             + F['jaw'] * g(at - 1.2, 0.22) * g(z + 0.09, 0.024)
             + F['chin'] * g(TH, 0.46) * g(z + F['bottom'] - 0.02, 0.017)
             + 0.0012 * F['smile'] * g(at - 0.5, 0.18) * g(z + 0.032, 0.014)
             + F['jowl'] * g(at - 0.85, 0.28) * g(z + 0.1, 0.02))
    nl = 0.21 + 0.22 * np.clip((-0.045 - z) / 0.04, 0.0, 1.0)
    window = smoothstep(-0.041, -0.049, z) * smoothstep(-0.088, -0.074, z)
    R = R - F['naso'] * g(at - nl, 0.032) * window + 0.0012 * g(at - nl - 0.07, 0.05) * window
    lips = F['lips']
    philtrum = 0.0005 * (g(TH - 0.034, 0.012) + g(TH + 0.034, 0.012)) * smoothstep(-0.05, -0.054, z) * smoothstep(zm + 0.008, zm + 0.013, z)
    R = R + (0.0014 * g(TH, 0.42) * g(z - zm, 0.02)
             - 0.0018 * g(TH, 0.27) * g(z - zm, 0.0022)                                 # the mouth's line
             + 0.0008 * lips * g(TH, 0.25) * g(z - zm - 0.0045, 0.0042)                  # upper lip
             + 0.0014 * lips * g(TH, 0.23) * g(z - zm + 0.0058, 0.0038)                  # lower lip, fuller
             - 0.0013 * g(TH, 0.2) * g(z - zm + 0.0165, 0.0038)                          # the hollow under it
             + philtrum)
    return r + R


def face_grid(rows, cols):
    """Directions over the head, crowded at the face (where the eyes and the mouth need them) and sparse at the back."""
    u = np.linspace(-1.0, 1.0, cols, endpoint=False) + 1.0 / cols
    th = np.pi * (0.36 * u + 0.64 * u ** 3)
    w = np.linspace(-1.0, 1.0, rows + 2)[1:-1]
    phi = 0.5 * np.pi + 0.5 * np.pi * (0.42 * w + 0.58 * w ** 3) + 0.06 * (1.0 - w ** 2)      # densest just under the eyes
    PH, TH = np.meshgrid(phi, th, indexing='ij')
    return PH, TH


def eye_fields(F, TH, z):
    """The eyes carved in the face: for each, an almond opening tilted a little down at its outer corner (tired, kind),
    the eyeball rounding inside it and looking a little down, a rolled upper lid that the years have made heavy and its
    crease, a lower lid lifted by the squint of a smile. Returns (displacement, cavity, opening)."""
    disp = np.zeros_like(TH)
    cav = np.zeros_like(TH)
    opening = np.zeros_like(TH)
    for s in SIDES:
        u = (TH - s * ET) * s * FACE_R                  # meters along the face, toward the outer corner
        v = z
        un = u / EYE_W
        shape = np.clip(1.0 - un ** 2, 0.0, None)
        tilt = -0.07 * u
        up_line = (0.0047 - 0.0014 * F['heavy']) * shape ** 0.6 * (1.0 + 0.12 * np.clip(-un, 0, 1)) + tilt + 0.0004
        lo_line = -(0.0041 - 0.0012 * F['squint']) * shape ** 0.8 * (1.0 + 0.15 * np.clip(un, 0, 1)) + tilt
        inside = np.minimum(np.minimum(up_line - v, v - lo_line), (1.0 - np.abs(un)) * 0.006)
        m = smoothstep(-0.0003, 0.0007, inside)
        near = smoothstep(1.45, 1.0, np.abs(un))
        ball = -0.0021 + 0.0024 * np.clip(1.0 - (u ** 2 + (v + 0.0006) ** 2) / EYE_W ** 2, 0.0, 1.0)
        crease = up_line + 0.0052 + 0.0012 * F['heavy']
        lid = (0.00105 * g(v - up_line - 0.0009, 0.0011) * near                     # the upper lid's rolled margin
               + 0.0005 * smoothstep(up_line, up_line + 0.002, v) * smoothstep(crease + 0.0005, crease - 0.0015, v) * near
               - 0.001 * g(v - crease, 0.001) * smoothstep(1.25, 0.8, np.abs(un))     # its crease
               + 0.0006 * F['heavy'] * g(v - crease - 0.0018, 0.0016) * near          # the heavy fold over it
               + 0.0007 * g(v - lo_line + 0.0008, 0.0009) * near                      # the lower lid's margin
               - 0.0006 * g(v - lo_line + 0.0045, 0.0014) * near)                     # the line under it
        disp = disp + m * ball + (1.0 - m) * lid
        iris = g(np.hypot(u - 0.0004 * s, v + 0.0012), 0.0046)
        eye_cav = np.clip(0.48 + 0.46 * iris, 0.0, 0.93)
        lash = 0.8 * g(v - up_line - 0.0002, 0.0008) * smoothstep(1.08, 0.75, np.abs(un))
        lower = 0.28 * g(v - lo_line + 0.0003, 0.0006) * smoothstep(1.0, 0.6, np.abs(un))
        cav = np.maximum(cav, np.maximum(m * eye_cav, np.maximum(lash, lower)))
        cav = np.maximum(cav, 0.32 * g(v - crease, 0.0009) * smoothstep(1.2, 0.8, np.abs(un)))
        cav = np.maximum(cav, 0.16 * g(v - crease - 0.004, 0.004) * smoothstep(1.4, 0.7, np.abs(un)))   # under the brow
        cav = np.maximum(cav, 0.2 * np.exp(-(u / 0.021) ** 2 - ((v - 0.0015) / 0.011) ** 2))           # the orbit's shade
        opening = np.maximum(opening, m)
    return disp, cav, opening


def head_surface(F, rows=210, cols=236):
    """The head as one surface: the anatomy (face_radius) and the eyes carved in it (eye_fields); lines of age across
    the forehead, deep crow's feet from a life of squinting into the sun, the folds from the nose; brows relaxed, their
    outer ends drooping a little (tired, kind); the corners of the mouth turned up a hair, a patient half smile; the
    whiskers and the hair under the hat, which take a cloth zone. Returns the piece, cavity, the zone per vertex and the
    grid (for the locks)."""
    PH, TH = face_grid(rows, cols)
    d = np.stack([np.sin(PH) * np.sin(TH), -np.sin(PH) * np.cos(TH), np.cos(PH)], -1)
    r = face_radius(d, F)
    z = d[..., 2] * r
    at = np.abs(TH)
    zm = F['mouth']
    eyes, eye_cav, opening = eye_fields(F, TH, z)
    creases = sum(g(z - zk, 0.0028) * (1.0 + 0.35 * np.sin(TH * 9.0 + k * 1.7))
                  for k, zk in enumerate((0.05, 0.062, 0.074))) * g(TH, 0.6)
    R = eyes - 0.0011 * F['age'] * creases
    crow = np.zeros_like(TH)
    for sgn in (1.0, -1.0):
        s_arc = (sgn * TH - 0.53) * FACE_R
        for ang in (-0.6, -0.2, 0.2, 0.55):
            along = math.cos(ang) * s_arc + math.sin(ang) * z
            across = -math.sin(ang) * s_arc + math.cos(ang) * z
            crow = crow + g(across, 0.0016) * smoothstep(0.003, 0.009, along) * smoothstep(0.034, 0.016, along)
    R = R - (0.0007 + 0.0007 * F['squint']) * crow
    line = (0.026 + 0.004 * F['worry'] * g(at - 0.12, 0.11) - 0.011 * ((at - 0.4) / 0.32) ** 2
            - 0.003 * F['droop'] * smoothstep(0.45, 0.75, at))
    brows = g(z - line, 0.0055) * smoothstep(0.07, 0.14, at) * smoothstep(0.8, 0.68, at)
    R = R + F['brow_hair'] * brows * (1.0 + 0.12 * np.sin(TH * 34.0 + z * 160.0))
    m, t, comb = beard_mask(F['beard'], TH, z, zm)
    R = R + m * t * (1.0 + 0.13 * comb)
    hm, ht, comb2 = hair_mask(F['hair'], TH, z)
    hm = hm * (1.0 - m)
    R = R + hm * ht * (1.0 + 0.12 * comb2)
    P = d * (r + R)[..., None]
    P[..., 2] += 0.0022 * F['smile'] * smoothstep(0.12, 0.34, at) * g(z - zm, 0.014) * smoothstep(1.0, 0.5, at)
    cav = eye_cav
    cav = np.maximum(cav, 0.42 * smoothstep(0.15, 0.6, brows))
    cav = np.maximum(cav, 0.7 * (g(TH - 0.06, 0.02) + g(TH + 0.06, 0.02)) * g(z + 0.0485, 0.0042))     # nostrils
    cav = np.maximum(cav, 0.6 * g(TH, 0.27) * g(z - zm, 0.0024) * (1.0 - smoothstep(0.27, 0.36, at)))   # the mouth's line
    cav = np.maximum(cav, 0.18 * g(TH, 0.2) * g(z - zm + 0.0165, 0.003))
    cav = np.maximum(cav, 0.24 * (g(TH - 0.33, 0.028) + g(TH + 0.33, 0.028)) * g(z - zm - 0.001, 0.0035) * F['smile'])
    nl = 0.21 + 0.22 * np.clip((-0.045 - z) / 0.04, 0.0, 1.0)
    window = smoothstep(-0.041, -0.049, z) * smoothstep(-0.088, -0.074, z)
    cav = np.maximum(cav, 0.26 * g(at - nl, 0.028) * window)
    cav = np.maximum(cav, 0.22 * np.clip(crow, 0.0, 1.0))
    cav = np.maximum(cav, 0.13 * F['age'] * np.clip(creases, 0.0, 1.0))
    if F['beard'] == 'stubble':
        # A few days' growth: a shadow over the jaw, the chin and the upper lip, speckled.
        lb = beard_line(at)
        lip_band = g(TH, 0.36) * smoothstep(zm + 0.0095, zm + 0.004, z) * smoothstep(zm - 0.012, zm - 0.005, z)
        grow = (smoothstep(lb + 0.008, lb - 0.01, z) * smoothstep(1.7, 1.5, at)
                + g(TH, 0.38) * smoothstep(zm + 0.026, zm + 0.016, z) * smoothstep(zm + 0.003, zm + 0.0095, z))
        speck = 0.5 + 0.5 * np.sin(TH * 210.0 + z * 530.0) * np.sin(TH * 97.0 - z * 830.0)
        cav = np.maximum(cav, np.clip(grow, 0.0, 1.0) * (1.0 - lip_band) * (0.11 + 0.08 * speck))
    # The hair's edge in tufts, not along the grid.
    tufts = 0.3 * np.sin(TH * 95.0 + z * 310.0) * np.sin(TH * 41.0 - z * 170.0)
    hairy = np.maximum(m, hm) + tufts * smoothstep(0.05, 0.45, np.maximum(m, hm)) * smoothstep(0.98, 0.6, np.maximum(m, hm))
    cav = np.where(hairy > 0.5, 0.1 + 0.05 * (1.0 - np.where(m > hm, comb, comb2)), cav)
    zone = np.where(hairy > 0.5, F['hair_zone'], SKIN).astype(float)
    V = np.vstack([P.reshape(-1, 3), P[0].mean(0) + (0.0, 0.0, 0.002), P[-1].mean(0) - (0.0, 0.0, 0.002)])
    n = rows * cols
    faces = [uc.grid_faces(rows, cols), uc.fan(n, np.arange(cols)), uc.fan(n + 1, np.arange(cols) + n - cols)]
    piece = uc.orient((V, faces))
    return (piece, np.concatenate([cav.ravel(), [0.0, 0.0]]), np.concatenate([zone.ravel(), [SKIN, SKIN]]),
            (P, d, TH, z))


def beard_line(at):
    """Where whiskers start on the face: just under the cheekbones, up the jaw to the sideburns."""
    return -0.034 - 0.012 * g(at - 0.8, 0.3) + 0.06 * smoothstep(1.15, 1.5, at)


def beard_mask(style, TH, z, zm):
    """The whiskers as (mask, thickness, combing): stubble has none (it is shading only), the chops run down the
    cheeks into a full moustache with the chin shaved, the chin curtain follows the jaw with the lips clear."""
    at = np.abs(TH)
    lips = g(TH, 0.36) * smoothstep(zm + 0.0105, zm + 0.0045, z) * smoothstep(zm - 0.014, zm - 0.006, z)
    nose = g(TH, 0.13) * smoothstep(-0.051, -0.043, z)
    comb = (np.sin(TH * 30.0 + 2.0 * np.sin(z * 60.0) + 1.3 * np.sin(TH * 7.0)) * 0.5
            + np.sin(TH * 13.0 - z * 45.0) * 0.3 + np.sin(TH * 47.0 + z * 90.0) * 0.2)
    if style == 'chops':
        lb = beard_line(at)
        cheeks = smoothstep(lb + 0.005, lb - 0.009, z) * smoothstep(1.72, 1.58, at) * smoothstep(0.42, 0.6, at)
        tache = g(TH, 0.5) * smoothstep(zm + 0.028, zm + 0.017, z) * smoothstep(zm + 0.0015, zm + 0.0085, z)
        # The moustache's ends sweep down past the corners of the mouth into the chops.
        join = (smoothstep(0.32, 0.46, at) * smoothstep(0.68, 0.55, at) * smoothstep(zm + 0.022, zm + 0.008, z)
                * smoothstep(zm - 0.04, zm - 0.018, z))
        m = np.maximum(np.maximum(cheeks, tache), join)
        t = 0.0075 + 0.0035 * smoothstep(lb, lb - 0.035, z)
        t = np.maximum(t, 0.0082 * tache)
    elif style == 'curtain':
        line = zm - 0.017 - 0.012 * smoothstep(0.3, 1.0, at) + 0.068 * smoothstep(1.1, 1.52, at)
        m = smoothstep(line + 0.003, line - 0.006, z) * smoothstep(1.72, 1.58, at)
        t = 0.0075 + 0.02 * g(TH, 0.62) * smoothstep(zm - 0.025, zm - 0.1, z)
    else:
        return np.zeros_like(TH), np.zeros_like(TH), comb
    return m * (1.0 - lips) * (1.0 - nose), t, comb


def hair_mask(style, TH, z):
    at = np.abs(TH)
    low = -0.04 + 0.03 * smoothstep(2.4, 1.3, at) - (0.026 if style == 'shaggy' else 0.0) * smoothstep(1.7, 2.7, at)
    if style == 'shaggy':
        low = low + 0.007 * np.sin(TH * 23.0) * np.sin(TH * 9.0 + 1.0)        # a ragged edge over the collar
    front = 0.95 + 0.3 * smoothstep(0.045, -0.02, z)
    m = smoothstep(front - 0.16, front + 0.08, at) * smoothstep(low - 0.006, low + 0.008, z)
    if style == 'thin':
        m = m * smoothstep(0.075, 0.045, z)          # a fringe round the back and sides: bald under the cap
    t = {'shaggy': 0.009, 'short': 0.005, 'thin': 0.0042}[style]
    return m, t, np.sin(TH * 24.0 + 1.6 * np.sin(z * 60.0))


def ear_piece(s, size=1.0, side_x=0.07):
    """An ear (Abel.py's): the rim curling round, its bowl, the lobe, the flap before the opening."""
    rim = spline([(0.0, -0.007, 0.011), (0.0, -0.003, 0.025), (0.0, 0.008, 0.031), (0.0, 0.017, 0.024),
                  (0.0, 0.021, 0.008), (0.0, 0.017, -0.009), (0.0, 0.009, -0.02)], 24)
    rr = np.interp(np.linspace(0.0, 1.0, 24), [0.0, 0.3, 0.7, 1.0], [0.003, 0.0046, 0.0046, 0.0038])
    pieces = [tube(rim, rr, segs=10, up=(1.0, 0.0, 0.0), domes=2),
              uc.ellipsoid((0.0, 0.007, 0.004), (0.0058, 0.0135, 0.023), segs=14, rings=9),
              uc.ellipsoid((0.0, 0.006, -0.021), (0.0055, 0.008, 0.009 * size), segs=10, rings=7),
              uc.ellipsoid((0.0018, -0.0075, -0.004), (0.003, 0.0035, 0.005), segs=8, rings=5)]
    V, F = uc.union(pieces, 0.0011, smooth=3)
    cav = 0.48 * g(V[:, 1] - 0.006, 0.006) * g(V[:, 2] + 0.001, 0.011) * smoothstep(0.0012, -0.001, V[:, 0])
    V = V * size
    R = rot_z(-s * 16.0) @ rot_x(-14.0)
    V = V * np.array([s, 1.0, 1.0])
    V = V @ R.T + np.array([s * side_x, 0.012, -0.014])
    if s < 0:
        F = [f[:, ::-1] for f in F]
    return (V, F), cav


def head_parts(fig, key, R, pivot, neck_points, neck_radius=(0.05, 0.047, 0.045), straw=False):
    """The head turned by R about the neck's top (pivot), on its neck rising out of the collar (neck_points, already
    where the body put them), with its eyes and ears; the back of the neck creased and leathered by the sun, as a
    farmer's is. Returns the head's middle."""
    F = FACES[key]
    center = pivot + R @ np.array([0.0, -0.012, 0.09])
    piece, cav, zone, grid = head_surface(F)
    head = ZPart('Head', SKIN)
    head.add(uc.transform(piece, R, center), cavity=cav, zone=zone)
    for s in SIDES:
        p, c = ear_piece(s, 1.2 if key == 'C' else 1.12, F['width'] / 2.0 - 0.003)
        head.add(uc.transform(p, R, center), cavity=c)
    # The neck: cords, the Adam's apple, and the back of it creased in a criss-cross by the sun.
    C = spline(list(neck_points) + [center + R @ np.array([0.0, 0.018, -0.07])], 24)
    a_ = np.linspace(0.0, 2.0 * np.pi, 28, endpoint=False)

    def rfn(s, a):
        cords = 0.07 * (g(wrap(a - 0.95), 0.25) + g(wrap(a + 0.95), 0.25)) * smoothstep(0.25, 0.55, s)
        apple = 0.09 * g(wrap(a), 0.22) * g(s - 0.55, 0.12) * (0.4 if key == 'C' else 1.0)
        return np.interp(s, [0.0, 0.5, 1.0], neck_radius) * (1.0 + cords + apple)
    V, faces = uc.tube_fn(C, rfn, segs=28, up=(0.0, -1.0, 0.0))
    s_ = np.clip(np.linspace(0.0, 1.0, len(C)), 0, 1)
    S, A = np.meshgrid(s_, a_, indexing='ij')
    nape = g(wrap(A - np.pi), 0.9)                                      # the back of the neck
    lines = np.maximum(smoothstep(0.82, 0.97, np.sin(A * 9.0 + S * 26.0)), smoothstep(0.82, 0.97, np.sin(A * 9.0 - S * 26.0)))
    neck_cav = np.concatenate([(0.3 * nape * lines * smoothstep(0.15, 0.4, S) * smoothstep(0.95, 0.75, S)).ravel(), [0.0, 0.0]])
    head.add((V, faces), cavity=neck_cav)
    fig.add(head)
    if straw:
        # A straw between his lips at the corner of the mouth, tipping down and out.
        zm = F['mouth']
        dirn = unit(np.array([math.sin(0.3), -math.cos(0.3), zm / 0.09]))
        a0 = dirn * float(face_radius(dirn[None], F)[0])
        path = [a0 + np.array([-0.014, 0.012, 0.0]), a0 + np.array([0.035, -0.022, -0.018]),
                a0 + np.array([0.078, -0.04, -0.045])]
        sp = ZPart('Straw', CLOTH1)
        sp.add(tube([center + R @ p for p in path], [0.0017, 0.0015, 0.001], segs=6, domes=1), cavity=0.05)
        fig.add(sp)
    return center


# --- Hats (hat space: origin at the brim's middle, where the 'hat' bone's head is; front -Y) ---

def hat_straw():
    """A wide harvest straw: a round crown, a broad floppy brim that waves and droops, its edge unbound and fraying
    (the fade eats it, as M_Ghost would), loose straws standing out of it; a dark cloth band with a stalk tucked in.
    Returns pieces as (V, faces, fade, zone[, cavity])."""
    rng = np.random.default_rng(23)
    theta = np.linspace(0.0, 2.0 * np.pi, 128, endpoint=False)
    q = np.linspace(0.0, 1.0, 16)[:, None]
    # The unbound edge: ragged, a few bites out of it, worn thinner toward the front where his hand takes it off.
    ragged = (0.006 * np.sin(theta * 29.0) * np.sin(theta * 11.0 + 1.0) + 0.004 * np.sin(theta * 53.0 + 2.0)
              - 0.008 * np.exp(-((wrap(theta - 2.2)) / 0.1) ** 2) - 0.006 * np.exp(-((wrap(theta + 0.9)) / 0.08) ** 2))
    r = 0.087 + (0.129 + ragged) * q * (1.0 + 0.025 * np.sin(3 * theta + 0.4) + 0.015 * np.sin(7 * theta + 1.3))
    droop = (-0.03 * q ** 1.6 * (0.6 + 0.4 * np.cos(2 * theta)) - 0.012 * q ** 2.2 * np.cos(theta)
             + 0.007 * q ** 2 * np.sin(5 * theta + 0.7) + 0.003 * q * np.sin(11 * theta))
    brim = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.04, droop + 0.0 * theta], -1)
    worn = np.broadcast_to(0.18 * smoothstep(0.88, 1.0, q), brim.shape[:2]).ravel()
    out = [(brim.reshape(-1, 3), [uc.grid_faces(16, 128)], np.ones(16 * 128), CLOTH1, worn)]
    zc = [0.0, 0.025, 0.05, 0.072, 0.09, 0.103, 0.11]
    rc = [0.088, 0.088, 0.086, 0.081, 0.071, 0.054, 0.03]
    crown = np.array([np.stack([rr * np.sin(theta), -rr * np.cos(theta) * 1.08, np.full_like(theta, zz)], -1)
                      for zz, rr in zip(zc, rc)])
    crown[..., 2] -= 0.006 * np.exp(-(crown[..., 0] / 0.03) ** 2) * (np.array(zc)[:, None] > 0.08)     # a soft crease
    V = np.vstack([crown.reshape(-1, 3), [[0.0, 0.0, 0.106]]])
    out.append((V, [uc.grid_faces(len(zc), 128), uc.fan(len(zc) * 128, np.arange(128) + (len(zc) - 1) * 128, top=True)],
                np.ones(len(V)), CLOTH1))
    band = np.array([np.stack([0.0895 * np.sin(theta), -0.0895 * np.cos(theta) * 1.08, np.full_like(theta, z)], -1)
                     for z in (0.002, 0.015, 0.028)])
    out.append((band.reshape(-1, 3), [uc.grid_faces(3, 128)], np.ones(band.size // 3), ACCENT))
    # Loose straws out of the fraying edge, and a stalk tucked in the band on his left.
    straws = []
    for k in range(18):
        a = rng.uniform(0.0, 2.0 * np.pi)
        rr = 0.205 + rng.uniform(-0.01, 0.01)
        base = np.array([rr * math.sin(a), -rr * math.cos(a) * 1.04,
                         -0.03 * (0.6 + 0.4 * math.cos(2 * a)) - 0.012 * math.cos(a)])
        b = a + rng.uniform(-0.6, 0.6)
        d = unit(np.array([math.sin(b), -math.cos(b), rng.uniform(-0.4, 0.2)]))
        L = rng.uniform(0.018, 0.05)
        straws.append(tube([base - d * 0.012, base + d * L], [0.0012, 0.0008], segs=5, domes=1))
    tuck = [np.array([0.088, 0.02, 0.012]), np.array([0.094, 0.0, 0.06]), np.array([0.1, -0.02, 0.105])]
    straws.append(tube(spline(tuck, 8), np.linspace(0.0018, 0.0008, 8), segs=6, domes=1))
    V, F = uc.merge(straws)
    out.append((V, F, np.ones(len(V)), CLOTH1))
    return out


def hat_rancher():
    """A creased felt hat: a tall crown pinched in front with a dent down the middle, a wide brim curled up at the
    sides and dipping front and back; sweat has darkened the felt above the band."""
    theta = np.linspace(0.0, 2.0 * np.pi, 120, endpoint=False)
    q = np.linspace(0.0, 1.0, 14)[:, None]
    r = 0.087 + 0.117 * q * (1.0 + 0.02 * np.sin(3 * theta + 0.6))
    lift = 0.034 * q ** 1.8 * np.sin(theta) ** 2 - 0.01 * q ** 2 * np.cos(theta) ** 2 + 0.003 * q * np.sin(5 * theta)
    brim = np.stack([r * np.sin(theta), -r * np.cos(theta) * 1.07, lift], -1)
    out = [(brim.reshape(-1, 3), [uc.grid_faces(14, 120)], np.ones(14 * 120), CLOTH1)]
    zc = np.array([0.0, 0.035, 0.07, 0.098, 0.12, 0.131])
    rc = np.array([0.089, 0.088, 0.085, 0.079, 0.069, 0.054])
    pinch = 1.0 - 0.17 * np.exp(-((np.abs(wrap(theta)) - 0.48) / 0.3) ** 2)
    rings = []
    for z, rr in zip(zc, rc):
        k = 1.0 + (pinch - 1.0) * (z / 0.131)
        rings.append(np.stack([rr * k * np.sin(theta), -rr * k * np.cos(theta) * 1.12, np.full_like(theta, z)], -1))
    for s, z in ((0.62, 0.129), (0.3, 0.115)):
        x, y = 0.054 * s * np.sin(theta), -0.054 * s * np.cos(theta) * 1.12
        rings.append(np.stack([x, y, z - 0.024 * np.exp(-(x / 0.022) ** 2)], -1))
    P = np.array(rings)
    V = np.vstack([P.reshape(-1, 3), [[0.0, 0.0, 0.088]]])
    nr = len(rings)
    stain = np.concatenate([np.where((P[..., 2] > 0.03) & (P[..., 2] < 0.058), 0.2, 0.0).ravel(), [0.0]])
    out.append((V, [uc.grid_faces(nr, 120), uc.fan(nr * 120, np.arange(120) + (nr - 1) * 120, top=True)], np.ones(len(V)),
                CLOTH1, stain))
    band = np.array([np.stack([0.0898 * np.sin(theta), -0.0898 * np.cos(theta) * 1.12, np.full_like(theta, z)], -1)
                     for z in (0.003, 0.016, 0.029)])
    out.append((band.reshape(-1, 3), [uc.grid_faces(3, 120)], np.ones(band.size // 3), ACCENT))
    return out


def cap_flat():
    """A flat cap: a soft, flat crown overhanging the head, pulled forward and down onto a short stiff peak; a button
    on top."""
    # The crown: one soft mass (a flattened, forward-tilted dome over the headband), fused smooth.
    crown = uc.ellipsoid((0.0, -0.016, 0.036), (0.109, 0.124, 0.046), np.array(
        [[1.0, 0.0, 0.0], rot_x(9.0) @ np.array([0.0, 1.0, 0.0]), rot_x(9.0) @ np.array([0.0, 0.0, 1.0])]), 48, 24)
    band = uc.torus((0.0, 0.0, 0.012), (0.0, 0.0, 1.0), 0.088, 0.013, squash=1.0)
    V, F = uc.union([crown, band], 0.003, smooth=4)
    V = V[:, :] * 1.0
    seam = 0.25 * g(np.hypot(V[:, 0], V[:, 1] + 0.016) - 0.075, 0.0025) * (V[:, 2] > 0.06)
    out = [(V, F, np.ones(len(V)), CLOTH1, seam)]
    # The peak: a short stiff brim round the front, curving down.
    a = np.linspace(-1.15, 1.15, 40)
    q = np.linspace(0.0, 1.0, 7)[:, None]
    r = 0.088 + 0.055 * q * np.cos(a * 0.9) ** 0.7
    peak = np.stack([r * np.sin(a), -r * np.cos(a) * 1.06, 0.004 - 0.022 * q ** 1.4 + 0.0 * a], -1)
    out.append((peak.reshape(-1, 3), [uc.grid_faces(7, 40, closed=False)], np.ones(7 * 40), ACCENT))
    V, F = uc.ellipsoid((0.0, -0.02, 0.058), (0.008, 0.008, 0.004), segs=12, rings=6)
    out.append((V, F, np.ones(len(V)), ACCENT))
    return out


HAT_FUNCS = {'A': hat_straw, 'B': hat_rancher, 'C': cap_flat}
HAT_SEAT = {'A': 0.153, 'B': 0.153, 'C': 0.137}     # how far over the neck's top the hat sits (the cap sits lower)


def add_hat(fig, key, R, pivot):
    Rh = R @ rot_x(-4.0) @ rot_y(3.0) if key != 'C' else R @ rot_x(9.0) @ rot_y(-3.0)
    at = pivot + R @ np.array([0.0, 0.0, HAT_SEAT[key]])
    part = ZPart('Hat', CLOTH1)
    for item in HAT_FUNCS[key]():
        V, F, fade, zone = item[:4]
        cav = item[4] if len(item) > 4 else 0.0
        part.add((np.asarray(V) @ Rh.T + at, F), fade=fade, zone=zone, cavity=cav)
    fig.add(part, solidify=0.006, material='hat')
    fig.marks['hat'] = at


# --- Props: the hay fork and the water jug (ghost twins, like Abel's lantern and pump) ---

def fork_parts():
    """A hay fork in fork space: the butt at the origin, the ash shaft up +Z, an iron ferrule and three long tines
    curving to its front (-Y)."""
    shaft = ZPart('ForkShaft', CLOTH1)
    shaft.add(tube([(0.0, 0.0, 0.0), (0.0, 0.004, 0.7), (0.0, 0.0, 1.33)], [0.0152, 0.015, 0.0143], segs=14))
    iron = ZPart('ForkIron', CLOTH2)
    iron.add(tube([(0.0, 0.0, 1.31), (0.0, 0.0, 1.385)], [0.0168, 0.012], segs=14))
    iron.add(tube(spline([(-0.042, 0.0, 1.392), (0.0, 0.0, 1.383), (0.042, 0.0, 1.392)], 9), 0.0068, segs=10))
    for x in (-0.042, 0.0, 0.042):
        path = [(x, 0.0, 1.39), (x * 1.06, -0.006, 1.48), (x * 1.16, -0.03, 1.58), (x * 1.24, -0.066, 1.665)]
        iron.add(tube(spline(path, 14), np.linspace(0.0062, 0.0026, 14), segs=10, domes=3, pointed=0.9), cavity=0.1)
    return [shaft, iron]


JUG_PROFILE = [(0.074, 0.0), (0.086, 0.012), (0.096, 0.05), (0.1, 0.1), (0.097, 0.15), (0.085, 0.19), (0.062, 0.224),
               (0.04, 0.25), (0.031, 0.27), (0.029, 0.288), (0.034, 0.297), (0.031, 0.307)]


def jug_parts():
    """The harvest water jug in jug space: the base's middle at the origin; stoneware in a cream glaze, its shoulder and
    neck dipped brown, a strap handle on +X and a cork."""
    theta = np.linspace(0.0, 2.0 * np.pi, 48, endpoint=False)
    rings = np.array([np.stack([r * np.cos(theta), r * np.sin(theta), np.full_like(theta, z)], -1) for r, z in JUG_PROFILE])
    V, F = uc.orient(uc.closed_loft(rings))
    zone = np.where(V[:, 2] > 0.205 + 0.006 * np.sin(np.arctan2(V[:, 1], V[:, 0]) * 3.0), CLOTH2, CLOTH1).astype(float)
    body = ZPart('Jug', CLOTH1).add((V, F), zone=zone, cavity=0.25 * smoothstep(0.012, 0.0, V[:, 2]))
    handle = [(0.088, 0.0, 0.19), (0.128, 0.0, 0.215), (0.13, 0.0, 0.252), (0.1, 0.0, 0.276), (0.034, 0.0, 0.279)]
    body.add(tube(spline(handle, 18), 0.0118, 0.0058, segs=12, up=(0.0, 1.0, 0.0)), zone=CLOTH2)
    body.add(tube([(0.0, 0.0, 0.296), (0.0, 0.0, 0.33)], [0.022, 0.0245], segs=16), zone=ACCENT, cavity=0.1)
    return [body]


def add_prop(fig, kind, M):
    """A prop's parts, carried by M (4x4, prop space to figure space), drawn with its own instance."""
    for part in {'fork': fork_parts, 'jug': jug_parts}[kind]():
        part.V = [apply(M, V) for V in part.V]
        fig.add(part, material=kind)
    fig.marks[kind] = M


def grip_point(W, fwd, up, side, scale=1.1):
    """Where a fist's grip sits (inside the curled fingers), and the hand's axes: along the fingers, the back of the
    hand, and the grip's own axis (the way the thumb closes)."""
    f = unit(fwd)
    u = unit(np.asarray(up, float) - f * np.dot(up, f))
    return W + f * 0.075 * scale - u * 0.03 * scale, f, u, np.cross(f, u) * side


# --- Arms and hands ---

def hand_part(fig, name, W, fwd, up, side, curl, spread=0.06, thumb=0.35, forearm=None, scale=1.1, zone=SKIN):
    """A big working hand from the wrist W (UnpaidConcepts.hand filled out: no claws, thick fingers), with the bare
    forearm from forearm (its elbow end, inside a rolled sleeve) when given."""
    dv = unit(W - forearm) if forearm is not None else unit(fwd)
    pieces = []
    if forearm is not None:
        L = np.linalg.norm(W - forearm)
        C = [forearm, forearm + dv * L * 0.3, forearm + dv * L * 0.65, W - dv * 0.01]
        pieces.append(tube(C, [0.039, 0.041, 0.033, 0.026], [0.041, 0.043, 0.036, 0.031], segs=22, up=up))
    else:
        pieces.append(tube([W - dv * 0.08, W - dv * 0.01], [0.031, 0.026], [0.035, 0.031], segs=18, up=up))
    pieces += uc.hand(W, fwd, up, side, curl=curl, spread=spread, thumb=thumb, length=1.02, thin=1.12, claw=0.0,
                      knuckle=1.2, scale=scale)
    fig.add(ZPart(name, zone).add(uc.union(pieces, 0.0024, smooth=3)))


def rolled_sleeve(S, E, r0, r1, seed, roll=0.017):
    """A loose shirt sleeve from the shoulder, rolled to just above the elbow: three folds winding down from the armpit,
    a pile of small folds above the roll."""
    rng = np.random.default_rng(seed)
    d = unit(E - S)
    end = E - d * 0.03
    C = spline([S, (S + end) * 0.5 + np.cross(d, [0.0, 0.0, 1.0]) * 0.004, end], 26)
    starts = rng.uniform(0.0, 2.0 * np.pi, 3)
    twists = rng.uniform(0.8, 1.6, 3) * rng.choice((-1.0, 1.0), 3)
    amps = rng.uniform(0.09, 0.15, 3)

    def rfn(s, a):
        base = r0 + (r1 - r0) * s
        f = sum(k * np.exp(-(wrap(a - p - t * s) / 0.38) ** 2) for k, p, t in zip(amps, starts, twists))
        f = f * smoothstep(0.04, 0.25, s) * smoothstep(0.96, 0.7, s)
        pile = sum(0.06 * np.exp(-((s - sk) / 0.03) ** 2) * (0.6 + 0.4 * np.cos(a + ph)) for sk, ph in ((0.8, 0.5), (0.9, 2.4)))
        return base * (1.0 + f + pile - 0.03)
    return [uc.tube_fn(C, rfn, segs=28), uc.torus(end, d, r1 - 0.002, roll, squash=0.9)]


def full_sleeve(S, E, W, radii, seed, cuff=0.012, folds=0.09):
    """A sleeve from the shoulder to the wrist: folds winding round it, bunching at the elbow, a cuff."""
    rng = np.random.default_rng(seed)
    d = unit(E - S)
    C = spline([S, (S + E) * 0.5 + np.cross(d, [0.0, 0.0, 1.0]) * 0.006, E, (E + W) * 0.5, W - unit(W - E) * 0.02], 40)
    starts = rng.uniform(0.0, 2.0 * np.pi, 4)
    twists = rng.uniform(0.8, 1.8, 4) * rng.choice((-1.0, 1.0), 4)

    def rfn(t, a):
        base = np.interp(t, [0.0, 0.5, 1.0], radii)
        f = sum(folds * np.exp(-(wrap(a - p - tw * t) / 0.35) ** 2) for p, tw in zip(starts, twists))
        elbow = 0.07 * np.exp(-((t - 0.5) / 0.08) ** 2) * (0.6 + 0.4 * np.cos(3 * a))
        return base * (1.0 + f * smoothstep(0.05, 0.3, t) + elbow)
    pieces = [uc.tube_fn(C, rfn, segs=28)]
    if cuff:
        dv = unit(W - E)
        pieces.append(uc.torus(W - dv * 0.03, dv, radii[2] - 0.004, cuff, squash=1.4))
    return pieces


# --- Below the waist: trousers to the knee, then the pale shroud ---

def knees(legs):
    """The knees' middles for the legs' pose: hanging ('down') or sitting with the thighs forward ('sit')."""
    if legs == 'sit':
        return {s: np.array([s * 0.108, -0.43, HIP - 0.016]) for s in SIDES}
    return {s: np.array([s * 0.1, -0.03, 0.5]) for s in SIDES}


def trousers(fig, profile, legs, zone, top=1.05, patch_on=None):
    """Work trousers from the waist to the knees: the hips and seat round the torso's bottom, two loose thighs; the
    cloth thins and frays at the knees, where the shroud takes over. Returns the knees."""
    K = knees(legs)
    theta = np.linspace(0.0, 2.0 * np.pi, 72, endpoint=False)
    z = np.linspace(0.93, top, 10)
    pieces = [uc.closed_loft(surface(profile, z, theta, grow=0.018))]
    if legs == 'sit':
        pieces.append(uc.ellipsoid((0.0, 0.025, 0.875), (0.178, 0.12, 0.075)))      # the seat, pressed on the rail
    else:
        pieces.append(uc.ellipsoid((0.0, 0.01, 0.9), (0.178, 0.118, 0.1)))
    hips = {s: np.array([s * 0.09, 0.0, HIP + 0.01]) for s in SIDES}
    for s in SIDES:
        mid = (hips[s] + K[s]) * 0.5 + (np.array([s * 0.008, -0.012, 0.0]) if legs != 'sit' else np.array([s * 0.006, 0.0, 0.01]))
        pieces.append(tube(spline([hips[s], mid, K[s]], 12), np.linspace(0.087, 0.06, 12), segs=24))
    V, F = uc.union(pieces, 0.005, smooth=4)
    # The fade: solid to above the knee, then thinning and fraying over the last few centimeters.
    reach = np.full(len(V), -1.0)
    for s in SIDES:
        d = unit(K[s] - hips[s])
        t = (V - hips[s]) @ d / np.linalg.norm(K[s] - hips[s])
        side = (V[:, 0] * s) > 0.0
        reach = np.where(side, np.maximum(reach, t), reach)
    fray = 0.04 * np.sin(np.arctan2(V[:, 1], V[:, 0]) * 7.0 + V[:, 2] * 40.0)
    fade = 1.0 - 0.72 * smoothstep(0.78 + fray, 1.02, reach)
    # The fly, a crease down each thigh's front, and creases behind the knees.
    cav = 0.3 * g(V[:, 0], 0.004) * (V[:, 1] < 0.0) * smoothstep(0.76, 0.86, V[:, 2]) * smoothstep(1.0, 0.94, V[:, 2])
    cav = np.maximum(cav, 0.12 * smoothstep(0.82, 0.95, reach) * (0.5 + 0.5 * np.sin(V[:, 0] * 120.0 + V[:, 2] * 60.0)))
    part = ZPart('Trousers', zone).add((V, F), fade=fade, cavity=cav)
    if patch_on is not None:
        # A patch sewn on his right thigh: flour sack, in the shirt's linen.
        s = -1.0
        c = (hips[s] + K[s]) * 0.5 + np.array([0.0, 0.0, 0.03])
        d = unit(K[s] - hips[s])
        front = unit(np.cross(d, (1.0, 0.0, 0.0))) if legs != 'sit' else np.array([0.0, 0.0, 1.0])
        if front[1] > 0 and legs != 'sit':
            front = -front
        side_v = unit(np.cross(front, d))
        rows, cols = 7, 8
        uu, vv = np.meshgrid(np.linspace(-1, 1, cols), np.linspace(-1, 1, rows))
        r_out = 0.08
        P = (c + d[None, None, :] * (vv[..., None] * 0.055) + side_v[None, None, :] * (uu[..., None] * 0.045)
             + front[None, None, :] * (r_out + 0.003 - 0.004 * uu[..., None] ** 2))
        edge = np.minimum(1 - np.abs(uu), 1 - np.abs(vv))
        part.add((P.reshape(-1, 3), [uc.grid_faces(rows, cols, closed=False)]), zone=patch_on,
                 cavity=(0.5 * smoothstep(0.18, 0.0, edge)).ravel())
    fig.add(part)
    fig.marks['knees'] = K
    return K


def leg_wisps(fig, K, legs, seed=31):
    """Below the knees the trousers give way to the Unpaid's pale shroud: one sheet round both legs, tearing into
    strips that sweep back off him as a tail and fade long before the ground (Abel's coat ends the same way)."""
    c = (K[1.0] + K[-1.0]) * 0.5
    if legs == 'sit':
        # Sitting on the rail: the shroud hangs from his knees and drifts back in under him.
        path = [c + (0.0, 0.0, 0.04), c + (0.0, 0.03, -0.1), c + (0.0, 0.13, -0.24), c + (0.0, 0.3, -0.34),
                c + (0.0, 0.52, -0.4), c + (0.0, 0.78, -0.43)]
        size = [(0.0, 0.15, 0.075), (0.2, 0.14, 0.07), (0.45, 0.1, 0.055), (0.7, 0.06, 0.04), (1.0, 0.025, 0.022)]
    else:
        path = [c + (0.0, 0.0, 0.07), c + (0.0, 0.03, -0.08), c + (0.0, 0.13, -0.2), c + (0.0, 0.34, -0.3),
                c + (0.0, 0.64, -0.36), c + (0.0, 0.98, -0.39)]
        size = [(0.0, 0.165, 0.085), (0.2, 0.155, 0.08), (0.45, 0.11, 0.065), (0.7, 0.07, 0.045), (1.0, 0.028, 0.024)]
    part = uc.shroud(ZPart('Shroud', SKIN), path, size, rows=64, cols=72, strips=8, split=0.12, ends=(0.6, 1.0),
                     spread=0.09, seed=seed, fade_from=0.1, folds=0.38, narrow=0.62, wave=0.035)
    # Lace rather than a sheet: never quite solid, so M_Ghost's noise opens holes all through it, and its top gathers
    # out of the frayed trouser legs as mist.
    top = path[0][2]
    for V, a in zip(part.V, part.attr):
        a[:, 0] = np.minimum(a[:, 0], 0.64) * (0.2 + 0.8 * smoothstep(top - 0.005, top - 0.1, V[:, 2]))
    fig.add(part, solidify=0.005)


# --- Option A: the hayman ---

def collar_band(profile, z0=1.522, height=0.03, gap=0.24, r=(0.058, 0.056, 0.055), y0=-0.008):
    """A shirt's low band collar round the neck, open at the throat."""
    theta = np.linspace(-np.pi, np.pi, 72, endpoint=False)
    rings = np.array([np.stack([rr * np.sin(theta), y0 - rr * 0.92 * np.cos(theta), np.full_like(theta, z0 + height * k / 2)], -1)
                      for k, rr in enumerate(r)])
    sd = np.broadcast_to((gap - np.abs(theta))[None, :], rings.shape[:2]) * 0.1
    V, F, used = uc.cut(rings, sd)
    return V, F


def garments_A(fig, P, legs):
    torso_core(fig, P)

    def folds(th, zz):
        # Bloused a little over the waistband, tucked in tight under it (the trousers cover it there).
        blouse = 0.01 * g(zz - 1.085, 0.035)
        f = (0.0045 * np.cos(7 * th + 0.5) + 0.003 * np.cos(12 * th + 2.0)) * smoothstep(1.28, 1.06, zz)
        back = 0.003 * np.cos(9 * th + 1.0) * smoothstep(1.45, 1.25, zz) * smoothstep(1.4, 2.2, np.abs(th))
        return 0.003 + (0.005 + blouse + f + back) * smoothstep(1.03, 1.075, zz)

    def sd(th, zz):
        at = np.abs(th)
        out = 0.95 - zz
        out = np.maximum(out, np.where(zz > 1.43, (0.045 + 1.25 * (zz - 1.43)) - at, -1.0))      # two buttons open
        arm = np.sqrt(((at - np.pi / 2 - 0.03) / 0.22) ** 2 + ((zz - 1.372) / 0.05) ** 2)
        out = np.maximum(out, (1.0 - arm) * 0.06)
        return np.maximum(out, zz - 1.548)
    (V, F), th, zz, edge = shell(P, 0.95, 1.548, sd, folds)
    placket = 0.32 * g(th, 0.011) * (zz < 1.432)
    shirt = ZPart('Shirt', CLOTH1).add((V, F), ember=scorch(edge, V), cavity=np.maximum(placket, scorch_cavity(edge)))
    buttons(shirt, P, [(0.0, z) for z in (1.405, 1.235, 1.15, 1.065)], grow=0.012)
    patch(shirt, P, -0.6, -0.26, 1.27, 1.37, grow=0.0125)
    fig.add(shirt, solidify=0.006)
    fig.add(ZPart('Collar', CLOTH1).add(collar_band(P)), solidify=0.005)
    # A straw stalk in the pocket, and chaff stuck to his sweat on the shoulders and back.
    bits = ZPart('Chaff', CLOTH1)
    p0 = surface(P, [1.355], np.array([-0.43]), 0.018)[0, 0]
    bits.add(tube([p0 - (0.0, 0.0, 0.03), p0 + (0.012, -0.01, 0.07), p0 + (0.03, -0.02, 0.12)], [0.0016, 0.0014, 0.001],
                     segs=6, domes=1))
    fig.add(bits)
    # The braces: the right one up over his shoulder and straight down his back; the left slipped off for the heat, its
    # loop hanging by his hip.
    braces = ZPart('Braces', ACCENT)
    keys = [(-0.64, 1.04), (-0.6, 1.16), (-0.53, 1.29), (-0.52, 1.41), (-0.8, 1.5), (-1.5708, 1.535), (-2.33, 1.5),
            (-2.62, 1.41), (-2.66, 1.28), (-2.66, 1.15), (-2.64, 1.04)]
    ribbon_on(braces, P, keys, 0.031, 0.021, n=70)
    front_btn = surface(P, [1.04], np.array([0.64]), 0.026)[0, 0]
    back_btn = surface(P, [1.04], np.array([2.64]), 0.026)[0, 0]
    if legs == 'sit':
        loop = [front_btn, (0.2, -0.085, 0.96), (0.216, -0.02, 0.88), (0.205, 0.06, 0.95), back_btn]
    else:
        loop = [front_btn, (0.19, -0.075, 0.88), (0.205, -0.01, 0.77), (0.196, 0.07, 0.87), back_btn]
    uc.ribbon(braces, [np.asarray(p, float) for p in loop], 0.03, n=40, cols=4,
              out=lambda C: unit(np.column_stack([C[:, 0], C[:, 1] * 0.3, np.zeros(len(C))])), curl=0.05)
    for t in (-0.64, 0.64, 2.64, -2.64):
        p = surface(P, [1.04], np.array([t]), 0.028)[0, 0]
        braces.add(uc.ellipsoid(p, (0.007, 0.007, 0.0045), np.array([[0, 0, 1.0], np.cross(uc.torso_normal(t), (0, 0, 1.0)),
                                                                      uc.torso_normal(t)]), 12, 8), cavity=0.4)
    fig.add(braces, solidify=0.004)
    chaff(bits, np.vstack(shirt.V), 18, 9, lambda V: (V[:, 2] > 1.44) & (np.abs(V[:, 0]) > 0.07))
    chaff(bits, np.vstack(braces.V), 8, 11, lambda V: V[:, 2] > 1.1)
    K = trousers(fig, P, legs, CLOTH2, patch_on=CLOTH1)
    return K


# --- Option B: the rancher ---

COAT_B = [(0.82, 0.208, 0.152, 0.012, 2.4), (0.90, 0.204, 0.148, 0.01, 2.4), (1.0, 0.2, 0.143, 0.004, 2.4),
          (1.1, 0.196, 0.137, 0.0, 2.4), (1.2, 0.198, 0.137, -0.004, 2.5), (1.3, 0.202, 0.137, -0.006, 2.6),
          (1.38, 0.207, 0.132, -0.004, 2.8), (1.44, 0.206, 0.122, -0.002, 3.2), (1.47, 0.195, 0.113, 0.0, 3.4),
          (1.5, 0.174, 0.103, 0.0, 3.0), (1.52, 0.131, 0.093, -0.002, 2.6), (1.545, 0.09, 0.083, -0.004, 2.2)]


def opening_B(zz):
    """The open coat's front edges (angle either side of the middle): parted from the collar, wider toward the hem."""
    return 0.3 + 0.24 * smoothstep(1.35, 0.85, zz) - 0.07 * smoothstep(1.42, 1.53, zz)


def garments_B(fig, P, legs):
    torso_core(fig, P)
    # The shirt: buttoned up, the collar's points turned down, a bandana knotted over the top button.
    def sfolds(th, zz):
        return 0.006 + 0.003 * np.cos(9 * th + 0.4) * smoothstep(1.2, 1.0, zz)

    def ssd(th, zz):
        at = np.abs(th)
        arm = np.sqrt(((at - np.pi / 2 - 0.03) / 0.22) ** 2 + ((zz - 1.372) / 0.05) ** 2)
        return np.maximum(np.maximum(0.95 - zz, (1.0 - arm) * 0.06), zz - 1.548)
    (V, F), th, zz, edge = shell(P, 0.95, 1.548, ssd, sfolds)
    shirt = ZPart('Shirt', CLOTH1).add((V, F), ember=scorch(edge, V),
                                        cavity=np.maximum(0.3 * g(th, 0.01), scorch_cavity(edge)))
    buttons(shirt, P, [(0.0, z) for z in (1.42, 1.24, 1.16, 1.08)], grow=0.01, size=0.006)
    fig.add(shirt, solidify=0.005)
    collar = ZPart('ShirtCollar', CLOTH1)
    collar.add(collar_band(P, z0=1.52, height=0.028, gap=0.05, r=(0.059, 0.057, 0.056)))
    for s in SIDES:
        path = [(s * 0.03, -0.062, 1.556), (s * 0.06, -0.068, 1.535), (s * 0.075, -0.07, 1.505)]
        uc.ribbon(collar, path, [(0.0, 0.012), (0.6, 0.017), (1.0, 0.006)], n=14, cols=4,
                  out=lambda C: outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.1)
    fig.add(collar, solidify=0.004)
    band = ZPart('Bandana', ACCENT)
    band.add(uc.torus((0.0, -0.004, 1.548), (0.0, -0.25, 1.0), 0.066, 0.011, squash=0.8))
    band.add(uc.ellipsoid((0.0, -0.075, 1.53), (0.017, 0.012, 0.014)))
    for s in SIDES:
        tail = [(0.0, -0.08, 1.525), (s * 0.012, -0.09, 1.49), (s * 0.02, -0.094, 1.455)]
        uc.ribbon(band, tail, [(0.0, 0.016), (1.0, 0.024)], n=14, cols=4, out=(0.0, -1.0, 0.1), curl=0.15, twist=0.1 * s)
    fig.add(band, solidify=0.004)

    # The chore coat: duck canvas to the hip, hanging open, boxy; stiff folds; a corduroy collar turned down.
    rng = np.random.default_rng(21)
    amps, phases = rng.uniform(0.5, 1.0, 4), rng.uniform(0.0, 2.0 * np.pi, 4)

    def folds(th, zz):
        f = sum(a * np.cos(k * th + p) for a, k, p in zip(amps, (5, 7, 11, 15), phases)) / amps.sum()
        sag = 0.006 * smoothstep(1.3, 0.9, zz) * (1.0 - smoothstep(0.0, 0.9, np.abs(th)))      # the fronts hang forward
        return 0.012 + 0.012 * smoothstep(1.15, 0.85, zz) * (f - 0.3 * np.abs(f)) + sag

    def sd(th, zz):
        at = np.abs(th)
        hem = 0.83 + 0.008 * np.sin(4 * th + 1.0) + 0.006 * np.sin(11 * th)
        out = np.maximum(hem - zz, opening_B(zz) - at)
        arm = np.sqrt(((at - np.pi / 2 - 0.04) / 0.24) ** 2 + ((zz - 1.368) / 0.055) ** 2)
        out = np.maximum(out, (1.0 - arm) * 0.06)
        return np.maximum(out, zz - 1.546)
    (V, F), th, zz, edge = shell(COAT_B, 0.83, 1.546, sd, folds, hole=HOLE_RADIUS + 0.004)
    # The coal's hole would sit in the open front: only where the coat covers it is it burnt.
    coat = ZPart('Coat', CLOTH2).add((V, F), ember=scorch(edge, V), cavity=scorch_cavity(edge))
    # Patch pockets: two on the chest with flaps, two low.
    for s in SIDES:
        t0, t1 = sorted((s * (opening_B(1.3) + 0.06), s * (opening_B(1.3) + 0.42)))
        patch(coat, COAT_B, t0, t1, 1.24, 1.34, grow=0.016, cavity_edge=0.45)
        patch(coat, COAT_B, t0 - 0.01, t1 + 0.01, 1.335, 1.36, grow=0.02, cavity_edge=0.55)
        u0, u1 = sorted((s * (opening_B(0.95) + 0.08), s * (opening_B(0.95) + 0.5)))
        patch(coat, COAT_B, u0, u1, 0.88, 1.0, grow=0.018, cavity_edge=0.45)
    fig.add(coat, solidify=0.008)
    # The tally book and a pencil stub in his left chest pocket.
    book = ZPart('Tally', CLOTH1)
    t_mid = opening_B(1.3) + 0.24
    p = surface(COAT_B, [1.37], np.array([t_mid]), 0.024)[0, 0]
    n = uc.torso_normal(t_mid)
    book.add(uc.ellipsoid(p + (0.0, 0.0, -0.004), (0.028, 0.004, 0.022), np.array([np.cross(n, (0, 0, 1.0)) * -1, n, (0, 0, 1.0)])),
             cavity=0.15)
    book.add(tube([p + (0.03, -0.004, -0.01), p + (0.034, -0.006, 0.04)], [0.0035, 0.003], segs=8), zone=ACCENT)
    fig.add(book)
    # The corduroy collar, turned down onto the shoulders, its points on the chest.
    theta = np.linspace(-np.pi, np.pi, 96, endpoint=False)
    rows = [(1.552, 0.093, 0.0), (1.535, 0.108, 0.0), (1.505, 0.128, 0.012), (1.47, 0.145, 0.03)]
    rings = []
    for z0, r0, drop in rows:
        front = np.cos(theta).clip(0.0, None) ** 2
        rings.append(np.stack([r0 * np.sin(theta), 0.006 - r0 * 0.96 * np.cos(theta), z0 - drop * 2.2 * front], -1))
    rings = np.array(rings)
    sdc = np.broadcast_to((0.42 - np.abs(theta))[None, :], rings.shape[:2]) * 0.1
    Vc, Fc, used = uc.cut(rings, sdc)
    fig.add(ZPart('CoatCollar', ACCENT).add((Vc, Fc), cavity=0.08), solidify=0.008)
    # The belt and its buckle (in the open coat).
    belt = ZPart('Belt', ACCENT)
    ribbon_on(belt, P, [(t, 1.0) for t in np.linspace(-np.pi, np.pi, 25)], 0.034, 0.03, n=90, curl=0.0)
    b0 = surface(P, [1.0], np.array([0.0]), 0.034)[0, 0]
    belt.add(uc.box_piece(b0, (0.024, 0.005, 0.02)), cavity=0.25)
    fig.add(belt, solidify=0.003)
    # His work gloves stuffed in the lower pocket, folded over its lip: two flat leather bundles, the fingers together
    # (lines in the cavity), the cuffs down in the pocket.
    gp = ZPart('Gloves', ACCENT)
    for k, (t0, lean_out, drop) in enumerate(((0.74, 0.42, 0.14), (0.88, 0.58, 0.12))):
        root = surface(COAT_B, [0.995], np.array([t0]), 0.02)[0, 0]
        n = uc.torso_normal(t0)
        side = unit(np.cross((0.0, 0.0, 1.0), n))
        down = unit(np.array([0.0, 0.0, -1.0]) + n * lean_out)
        rows, cols = 12, 9
        q = np.linspace(0.0, 1.0, rows)[:, None]
        w = np.linspace(-1.0, 1.0, cols)[None, :]
        width = 0.024 * (1.0 - 0.25 * q ** 2)
        # Over the pocket's lip and down its face, the bundle bowed across and thick in the middle.
        bend = n[None, None, :] * (0.012 * np.sin(np.pi * np.clip(q, 0, 0.35) / 0.35)[..., None] - 0.003 * q[..., None])
        tips = 1.0 - 0.16 * (0.5 - 0.5 * np.cos(w * 4.0 * np.pi)) * smoothstep(0.7, 1.0, q)     # the four finger ends
        Q = (root[None, None, :] + down[None, None, :] * (q * tips * drop - 0.02)[..., None]
             + side[None, None, :] * (w * width)[..., None] + bend + n[None, None, :] * (0.006 * (1 - w ** 2))[..., None])
        fingers = 0.5 * smoothstep(0.45, 0.6, q) * (np.abs(np.sin(w * np.pi * 2.0)) < 0.3)
        piece = (Q.reshape(-1, 3), [uc.grid_faces(rows, cols, closed=False)])
        gp.add(piece, cavity=np.broadcast_to(fingers, (rows, cols)).ravel().astype(float))
    fig.add(gp, solidify=0.008)
    chaff(book, np.vstack(coat.V), 14, 13, lambda V: (V[:, 2] > 1.43) & (np.abs(V[:, 0]) > 0.08))
    return trousers(fig, P, legs, ACCENT)


# --- Option C: the old hand ---

def garments_C(fig, P, legs):
    torso_core(fig, P)

    def sfolds(th, zz):
        return 0.007 + 0.003 * np.cos(8 * th + 0.4) * smoothstep(1.3, 1.0, zz)

    def ssd(th, zz):
        at = np.abs(th)
        out = 0.95 - zz
        out = np.maximum(out, np.where(zz > 1.47, (0.03 + 0.6 * (zz - 1.47)) - at, -1.0))      # the henley's placket open
        arm = np.sqrt(((at - np.pi / 2 - 0.03) / 0.22) ** 2 + ((zz - 1.372) / 0.05) ** 2)
        out = np.maximum(out, (1.0 - arm) * 0.06)
        return np.maximum(out, zz - 1.548)
    (V, F), th, zz, edge = shell(P, 0.95, 1.548, ssd, sfolds, hole=0.0)
    shirt = ZPart('Shirt', CLOTH1).add((V, F), cavity=0.3 * g(th, 0.01) * (zz > 1.38))
    buttons(shirt, P, [(0.0, z) for z in (1.405, 1.44, 1.475)], grow=0.01, size=0.0055)
    fig.add(shirt, solidify=0.005)
    fig.add(ZPart('Collar', CLOTH1).add(collar_band(P, z0=1.52, height=0.024, gap=0.18, r=(0.056, 0.054, 0.053))),
            solidify=0.004)
    # The overalls: washed denim, the bib up to mid-chest, straps over the shoulders crossing his back, buckles at the
    # bib's corners, a bib pocket with a pencil, a watch pocket with its chain; the coal has burnt through the bib.
    def bfolds(th, zz):
        return 0.016 + 0.003 * np.cos(6 * th + 0.3) * smoothstep(1.25, 1.02, zz)

    def bsd(th, zz):
        at = np.abs(th)
        half = 0.56 + 0.14 * smoothstep(1.3, 1.02, zz)
        out = np.maximum(at - half, zz - (1.37 - 0.01 * (at / 0.56) ** 2))
        return np.maximum(out, 0.98 - zz)
    (V, F), th, zz, edge = shell(P, 0.98, 1.38, bsd, bfolds)
    bib = ZPart('Bib', CLOTH2).add((V, F), ember=scorch(edge, V),
                                    cavity=np.maximum(scorch_cavity(edge), 0.35 * smoothstep(0.012, 0.0, 1.37 - zz)))
    patch(bib, P, -0.42, -0.08, 1.2, 1.31, grow=0.021, cavity_edge=0.5)
    fig.add(bib, solidify=0.006)
    straps = ZPart('Straps', CLOTH2)
    for s in SIDES:
        keys = [(s * 0.48, 1.36), (s * 0.5, 1.42), (s * 0.85, 1.5), (s * 1.5708, 1.535), (s * 2.3, 1.5), (s * 2.7, 1.41),
                (s * 2.98, 1.3), (s * 3.28, 1.18), (s * 3.5, 1.08), (s * 3.6, 1.0)]
        ribbon_on(straps, P, keys, 0.038, 0.021, n=60)
    fig.add(straps, solidify=0.005)
    metal = ZPart('Buckles', CLOTH1)
    for s in SIDES:
        p = surface(P, [1.36], np.array([s * 0.48]), 0.026)[0, 0]
        n = uc.torso_normal(s * 0.48)
        metal.add(uc.torus(p, n, 0.016, 0.003, squash=0.6), cavity=0.2)
        metal.add(uc.ellipsoid(surface(P, [1.33], np.array([s * 0.5]), 0.024)[0, 0], (0.008, 0.008, 0.004),
                               np.array([[0, 0, 1.0], np.cross(n, (0, 0, 1.0)), n])), cavity=0.3)
        metal.add(uc.ellipsoid(surface(P, [1.0], np.array([s * 1.45]), 0.03)[0, 0], (0.0085, 0.0085, 0.0045)), cavity=0.3)
    # The watch chain from the bib's top button loop across to the watch pocket (on his left, under the coal).
    c0 = surface(P, [1.355], np.array([0.42]), 0.024)[0, 0]
    c1 = surface(P, [1.24], np.array([0.5]), 0.024)[0, 0]
    mid = (c0 + c1) * 0.5 + np.array([0.012, -0.014, -0.04])
    links_ = spline([c0, mid, c1], 22)
    for k in range(len(links_) - 1):
        metal.add(uc.ellipsoid((links_[k] + links_[k + 1]) * 0.5, (0.0035, 0.0018, 0.0018),
                               np.array([unit(links_[k + 1] - links_[k]), (0, 0, 1.0), np.cross(unit(links_[k + 1] - links_[k]), (0, 0, 1.0))]),
                               8, 5), cavity=0.15)
    fig.add(metal)
    pencil = ZPart('Pencil', ACCENT)
    p = surface(P, [1.31], np.array([-0.3]), 0.025)[0, 0]
    pencil.add(tube([p - (0.0, 0.0, 0.04), p + (0.004, -0.004, 0.035)], [0.0042, 0.0036], segs=8))
    fig.add(pencil)
    # The red kerchief hanging out of his back pocket, and chaff on the bib and straps.
    kerchief = ZPart('Kerchief', ACCENT)
    k0 = np.array([-0.08, 0.135, 0.9])
    uc.ribbon(kerchief, [k0, k0 + (-0.01, 0.012, -0.06), k0 + (-0.02, 0.016, -0.13)], [(0.0, 0.03), (0.6, 0.036), (1.0, 0.012)],
              n=14, cols=5, out=(0.0, 1.0, 0.1), curl=0.2, twist=0.08)
    fig.add(kerchief, solidify=0.004)
    bits = ZPart('Chaff', CLOTH1)
    chaff(bits, np.vstack(bib.V), 10, 17, lambda V: (V[:, 2] > 1.05))
    chaff(bits, np.vstack(straps.V), 8, 19, lambda V: (V[:, 2] > 1.4))
    fig.add(bits)
    return trousers(fig, P, legs, CLOTH2, top=1.05)


GARMENTS = {'A': garments_A, 'B': garments_B, 'C': garments_C}


# --- Poses ---
#
# A pose bends the body at the waist and the upper back (the rig's pelvis, spine_01 and spine_02: the garments are
# built upright and warped), then places the arms, the head and the legs where they go. The fence in the story poses:
# the top rail's middle 35 cm before him (y -0.35) with its top at 1.035 m (Fences.py's FenceRail: rails at 0.98 m,
# 11 cm deep); sitting, his seat rests on that top.

RAIL_Y, RAIL_TOP = -0.35, 1.035
SEAT_Z = 0.79                           # the seat's underside in the sitting figure's own frame (on the rail's top)
FIST = (1.12, 1.18, 1.18, 1.12)
LOOSE = (0.22, 0.32, 0.42, 0.52)


def arm(E, W, fwd, up, curl=LOOSE, spread=0.07, thumb=0.35, rel=False):
    return dict(E=np.asarray(E, float), W=np.asarray(W, float), fwd=unit(fwd), up=unit(up), curl=curl, spread=spread,
                thumb=thumb, rel=rel)


def pose_spec(key, pose):
    """How the figure stands (or leans, or sits) for a shot."""
    if pose == 'lean':
        # His forearms on the top rail, the hands folded, right over left; bent at the hips over the fence, his head
        # lifted to the one he's talking to.
        spec = dict(lean=38.0, hunch=6.0, legs='down', head=rot_z(14.0) @ rot_x(-3.0), props=[])
        spec['arms'] = {1.0: arm((0.25, -0.335, 1.07), (0.02, -0.365, 1.07), (-1.0, -0.35, -0.45), (0.0, -0.3, 1.0),
                                 (0.55, 0.6, 0.65, 0.7), 0.05, 0.3),
                        -1.0: arm((-0.25, -0.335, 1.075), (-0.03, -0.38, 1.115), (1.0, -0.25, -0.3), (0.0, -0.15, 1.0),
                                  (0.4, 0.45, 0.5, 0.55), 0.05, 0.3)}
        if key == 'C':
            spec.update(lean=33.0, hunch=13.0, head=rot_z(14.0) @ rot_x(-7.0))
        if key == 'A':
            spec['props'] = [('fork_lean', None)]
        if key == 'C':
            spec['props'] = [('jug_post', None)]
        return spec
    if pose == 'sit':
        spec = dict(lean=6.0, lean_z=1.07, lean_w=0.05, hunch=5.0, legs='sit', head=rot_z(-20.0) @ rot_x(-6.0), props=[])
        knee_hand = arm((0.25, -0.17, 1.12), (0.13, -0.38, 0.985), (0.02, -0.75, -0.65), (0.1, -0.35, 1.0),
                        (0.5, 0.55, 0.62, 0.68), 0.08, 0.3)
        if key == 'A':
            spec['arms'] = {1.0: knee_hand,
                            -1.0: arm((-0.26, -0.13, 1.16), (-0.245, -0.37, 1.04), (0.05, -1.0, 0.1), (-1.0, 0.05, 0.1),
                                      FIST, 0.04, 0.8)}
            spec['props'] = [('fork_sit', None)]
        elif key == 'B':
            spec['arms'] = {s: arm((s * 0.24, 0.03, 1.15), (s * 0.255, -0.005, 0.875), (0.0, -0.85, -0.55),
                                   (s * 0.25, 0.25, 1.0), (0.95, 1.0, 1.0, 0.95), 0.05, 0.5) for s in SIDES}
            spec['head'] = rot_z(-18.0) @ rot_x(-7.0)
        else:
            spec.update(hunch=12.0, head=rot_z(-12.0) @ rot_x(-5.0))
            # The jug in his lap on his right thigh, his hand round its neck.
            spec['arms'] = {1.0: knee_hand,
                            -1.0: arm((-0.3, 0.02, 1.2), (-0.144, -0.154, 1.235), (0.3, -0.95, 0.0), (-0.95, -0.3, 0.0),
                                      (1.0, 1.1, 1.1, 1.05), 0.04, 0.7)}
            spec['props'] = [('jug_knee', None)]
        return spec
    # The idle: floating at ease, as he'd hang by his fence.
    if key == 'A':
        spec = dict(lean=2.0, hunch=4.0, legs='down', head=rot_z(5.0) @ rot_x(-3.0), props=[('fork_hand', None)])
        spec['arms'] = {-1.0: arm((-0.065, 0.03, -0.28), (-0.035, -0.15, -0.215), (0.05, -1.0, 0.15), (-1.0, 0.0, 0.1),
                                  FIST, 0.04, 0.8, rel=True),
                        1.0: arm((0.04, -0.055, -0.3), (0.012, -0.08, -0.255), (0.04, -0.22, -1.0), (1.0, -0.12, 0.05),
                                 LOOSE, 0.1, 0.35, rel=True)}
    elif key == 'B':
        # Thumbs hooked in his belt, the coat pushed back.
        spec = dict(lean=1.0, hunch=2.0, legs='down', head=rot_z(-6.0) @ rot_x(-3.0), props=[])
        spec['arms'] = {s: arm((s * 0.092, 0.034, -0.264), (s * -0.14, -0.175, -0.128), (s * -0.2, -0.3, -1.0),
                               (s * 0.5, -0.85, 0.05), (0.25, 0.3, 0.36, 0.42), 0.04, 0.0, rel=True) for s in SIDES}
    else:
        # Stooped, the jug hanging from his right hand.
        spec = dict(lean=4.0, hunch=17.0, hunch_z=1.36, legs='down', head=rot_z(4.0) @ rot_x(-9.0), props=[('jug_hand', None)])
        spec['arms'] = {-1.0: arm((-0.05, 0.0, -0.29), (-0.02, -0.07, -0.255), (0.0, -0.55, -1.0), (-1.0, -0.1, 0.05),
                                  (1.0, 1.1, 1.1, 1.05), 0.04, 0.7, rel=True),
                        1.0: arm((0.035, -0.04, -0.29), (0.015, -0.085, -0.25), (0.04, -0.22, -1.0), (1.0, -0.12, 0.05),
                                 LOOSE, 0.1, 0.35, rel=True)}
    return spec


def warp_of(spec):
    hunch = uc.lean(math.radians(spec.get('hunch', 0.0)), pivot_z=spec.get('hunch_z', 1.34), width=0.08)
    lean = uc.lean(math.radians(spec.get('lean', 0.0)), pivot_z=spec.get('lean_z', 1.0), width=spec.get('lean_w', 0.1))

    def fn(V):
        return lean(hunch(np.asarray(V, float)))
    return fn


def at(fn, p):
    return fn(np.atleast_2d(np.asarray(p, float)))[0]


def arms_for(fig, key, S, spec):
    """Sleeves from the shoulders (where the body's bend put them) to the elbows and wrists, and the hands."""
    hands = {}
    for k, s in enumerate(SIDES):
        a = spec['arms'][s]
        E = S[s] + a['E'] if a['rel'] else a['E']
        W = E + a['W'] if a['rel'] else a['W']
        S0 = S[s] + np.array([-s * 0.012, 0.0, 0.008])
        if key == 'A':
            sleeve = ZPart(f'Sleeve{s:+.0f}', CLOTH1)
            for piece in rolled_sleeve(S0, E, 0.063, 0.056, 31 + k):
                sleeve.add(piece)
            fig.add(sleeve)
            hand_part(fig, f'Hand{s:+.0f}', W, a['fwd'], a['up'], s, a['curl'], a['spread'], a['thumb'],
                      forearm=E - unit(E - S[s]) * 0.012)
        else:
            radii = (0.069, 0.063, 0.057) if key == 'B' else (0.064, 0.057, 0.05)
            sleeve = ZPart(f'Sleeve{s:+.0f}', CLOTH2 if key == 'B' else CLOTH1)
            for piece in full_sleeve(S0, E, W, radii, 41 + k, cuff=0.012 if key == 'B' else 0.009,
                                     folds=0.08 if key == 'B' else 0.1):
                sleeve.add(piece)
            fig.add(sleeve)
            hand_part(fig, f'Hand{s:+.0f}', W, a['fwd'], a['up'], s, a['curl'], a['spread'], a['thumb'],
                      scale=1.12 if key == 'B' else 1.06)
        hands[s] = (W, a['fwd'], a['up'])
    return hands


def place_props(fig, key, spec, hands, origin_z=0.0):
    """The props where the pose puts them: the fork in his fist or leaning on the fence post, the jug in his hand, on
    the post's top or on his knee. origin_z: where the ground is in the figure's frame (sitting, below its feet)."""
    for kind, _ in spec['props']:
        if kind in ('fork_hand', 'fork_sit'):
            W, f, u = hands[-1.0]
            G, f, u, t = grip_point(W, f, u, -1.0)
            if kind == 'fork_hand':
                butt = np.array([G[0] - 0.02, G[1] - 0.02, 0.0])
            else:
                butt = np.array([G[0] - 0.03, G[1] - 0.16, origin_z])
            z = unit(G - butt)
            y = unit(np.array([0.0, 1.0, 0.0]) - z * z[1])
            add_prop(fig, 'fork', frame_matrix(butt, np.cross(y, z), y, z))
        elif kind == 'fork_lean':
            butt = np.array([-0.6, -0.02, 0.0])
            touch = np.array([-0.69, RAIL_Y + 0.085, RAIL_TOP + 0.12])
            z = unit(touch - butt)
            y = unit(np.array([0.0, 1.0, 0.0]) - z * z[1])
            add_prop(fig, 'fork', frame_matrix(butt, np.cross(y, z), y, z))
        elif kind == 'jug_hand':
            W, f, u = hands[-1.0]
            G, f, u, t = grip_point(W, f, u, -1.0, scale=1.06)
            x = unit(np.array([t[0], t[1], 0.0]))
            zz = np.array([0.0, 0.0, 1.0])
            M = frame_matrix(np.zeros(3), x, np.cross(zz, x), zz)
            M[:3, 3] = G - M[:3, :3] @ np.array([0.082, 0.0, 0.277])
            add_prop(fig, 'jug', M)
        elif kind == 'jug_post':
            add_prop(fig, 'jug', frame_matrix(np.array([0.75, RAIL_Y, 1.2]), np.array([0.6, 0.8, 0.0]),
                                              np.array([-0.8, 0.6, 0.0]), np.array([0.0, 0.0, 1.0])))
        elif kind == 'jug_knee':
            add_prop(fig, 'jug', frame_matrix(np.array([-0.09, -0.22, 0.965]), np.array([-0.6, 0.8, 0.0]),
                                              np.array([-0.8, -0.6, 0.0]), np.array([0.0, 0.0, 1.0])))


# --- The figure ---

_cache = {}


def figure(key, pose='idle'):
    if (key, pose) in _cache:
        return _cache[(key, pose)]
    t0 = time.time()
    fig = Amos(key, pose)
    spec = pose_spec(key, pose)
    P = TORSO[key]
    K = GARMENTS[key](fig, P, spec['legs'])
    warp = warp_of(spec)
    fig.warp(warp)
    S = {s: at(warp, SHOULDER[key][s]) for s in SIDES}
    hands = arms_for(fig, key, S, spec)
    pivot = at(warp, PIVOT[key])
    neck = [at(warp, (0.0, PIVOT[key][1] + 0.01, 1.47)), at(warp, (0.0, PIVOT[key][1] - 0.002, 1.55))]
    head_parts(fig, key, spec['head'], pivot, neck, straw=(key == 'A'),
               neck_radius=(0.064, 0.05, 0.046) if key != 'C' else (0.06, 0.047, 0.043))
    add_hat(fig, key, spec['head'], pivot)
    leg_wisps(fig, K, spec['legs'])
    place_props(fig, key, spec, hands, origin_z=-(RAIL_TOP - SEAT_Z) if pose == 'sit' else 0.0)
    fig.add(ZPart('Coal', SKIN).add(uc.coal_piece(*fig.coal)), material='coal')
    fig.marks['head'] = pivot + spec['head'] @ np.array([0.0, -0.012, 0.09])
    _cache[(key, pose)] = fig
    log(f'{key} {pose}: {fig.triangles()} triangles (concept density), built in {time.time() - t0:.1f} s')
    return fig


def materials(key):
    colors, grain, scale = HAT_LOOK[key]
    return {'body': ghost_material(f'Amos{key}', TINTS[key]),
            'hat': ghost_material(f'Amos{key}Hat', colors, grain=grain, grain_scale=scale, rim=0.7, power=4.5),
            'fork': ghost_material('AmosFork', FORK_TINT, rim=1.4, glow=0.08),
            'jug': ghost_material('AmosJug', JUG_TINT, rim=1.4, glow=0.08),
            'coal': banked_coal_material()}


def instantiate(fig, where, location=(0.0, 0.0, 0.0), turn=0.0):
    mats = materials(fig.key)
    objs = []
    for part, options in fig.parts:
        mat = mats[options.get('material', 'body')]
        objs.append(build_part(part, mat, where, location=location, turn=turn,
                               **{k: v for k, v in options.items() if k != 'material'}))
    return objs


def to_world(p, location, turn):
    return rotation((0.0, 0.0, 1.0), turn) @ np.asarray(p, float) + np.asarray(location, float)


# --- The stage: Amos's fence in Whitlock Fields, the golden late afternoon ---
#
# The world here: +X east, +Y north (the plan's axes). The fence runs east-west along y = 0 with its posts every 1.5 m;
# Amos's place is between the posts at x = 0 and 1.5. His field lies on both sides of it (the fence splits the
# hayfields): stubble in drill rows, the cut hay rotting in windrows and bales behind him, the uncut hay standing past
# them, and the ridges in the haze.

AMOS_X = 0.75


def bearing(azimuth, elevation):
    """Toward a sun at a compass bearing (degrees from north toward east) and elevation, in this world (x east, y north)."""
    a, e = math.radians(azimuth), math.radians(elevation)
    return (math.sin(a) * math.cos(e), math.cos(a) * math.cos(e), math.sin(e))


# The level's light (Art/Levels/RansomsRest/layout.json's golden afternoon, as Farmhouse.py's previews paint it).
SUN = bearing(247.5, 15.0)
SUN_COLOR, SUN_STRENGTH = (1.0, 0.8, 0.56), 4.4
GOLDEN_SKY = [(0.0, 0x6b6050), (0.47, 0x8e8270), (0.5, 0xe9d2a6), (0.53, 0xd9d6c4), (0.62, 0xa9bfd2), (1.0, 0x5d84b6)]
GOLDEN_GLOWS = [(0xffe0a8, 24.0, 0.9), (0xfff2d0, 600.0, 6.0)]


def golden_sky(strength=1.0):
    world = bpy.data.worlds.get('AmosGolden') or bpy.data.worlds.new('AmosGolden')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    background = nodes.new('ShaderNodeBackground')
    background.inputs['Strength'].default_value = strength
    coords = nodes.new('ShaderNodeTexCoord')
    norm = nodes.new('ShaderNodeVectorMath')
    norm.operation = 'NORMALIZE'
    links.new(coords.outputs['Generated'], norm.inputs[0])
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(norm.outputs['Vector'], split.inputs['Vector'])
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -1.0
    links.new(split.outputs['Z'], remap.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    elements = ramp.color_ramp.elements
    elements[0].position, elements[0].color = GOLDEN_SKY[0][0], lt.hex_color(GOLDEN_SKY[0][1])
    elements[1].position, elements[1].color = GOLDEN_SKY[-1][0], lt.hex_color(GOLDEN_SKY[-1][1])
    for position, color in GOLDEN_SKY[1:-1]:
        elements.new(position).color = lt.hex_color(color)
    color = ramp.outputs['Color']
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(norm.outputs['Vector'], dot.inputs[0])
    dot.inputs[1].default_value = Vector(SUN).normalized()
    clamp = uc._math(nodes, links, 'MAXIMUM', dot.outputs['Value'], 0.0)
    for glow, power, k in GOLDEN_GLOWS:
        lift = uc._math(nodes, links, 'MULTIPLY', uc._math(nodes, links, 'POWER', clamp, power), k)
        add = nodes.new('ShaderNodeMix')
        add.data_type = 'RGBA'
        add.blend_type = 'ADD'
        links.new(lift, add.inputs['Factor'])
        links.new(color, add.inputs['A'])
        add.inputs['B'].default_value = lt.hex_color(glow)
        color = add.outputs['Result']
    links.new(color, background.inputs['Color'])
    links.new(background.outputs['Background'], out.inputs['Surface'])
    bpy.context.scene.world = world


def model_source(path, functions):
    """A model script's builders, without building its models: its source up to its models list, run in a namespace."""
    source = open(path, encoding='utf-8').read()
    source = source[:source.index('\nmodels = [')]
    ns = {'__name__': 'amos_stage', '__file__': path}
    exec(compile(source, path, 'exec'), ns)
    return [ns[f] for f in functions]


def fence_models():
    """Fences.py's FenceRail and FencePost (the game's own pieces, same seeds), hidden: the stage places copies."""
    fence_rail, fence_post = model_source(os.path.join(lt.REPO, 'Art', 'Models', 'Props', 'Fences.py'),
                                          ('fence_rail', 'fence_post'))
    rail, post = fence_rail('FenceRail', 10), fence_post('FencePost', 20)
    for obj in (rail, post):
        for c in obj.children:
            c.hide_render = True
        obj.hide_render = True
    return rail, post


def bale_model():
    hay_square, = model_source(os.path.join(lt.REPO, 'Art', 'Models', 'Props', 'FarmProps.py'), ('hay_square',))
    bale = hay_square('HayBale_Square', 40)
    for c in bale.children:
        c.hide_render = True
    bale.hide_render = True
    return bale


def place_copy(src, where, location, turn=0.0, tilt=(0.0, 0.0), material=None):
    obj = src.copy()
    if material is not None:
        obj.data = src.data.copy()
        obj.data.materials.clear()
        obj.data.materials.append(material)
    where.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (tilt[0], tilt[1], turn)
    obj.hide_render = False
    return obj


def palette_v(name, across):
    rows = len(lt.PALETTE)
    return (lt.PALETTE.index(name) + 0.15 + 0.7 * np.clip(across, 0.0, 1.0)) / rows


def stalks(where, name, roots, heights, widths, lean, swatches, rng, segments=1, bend=0.0):
    """Upright blades or stalks on FoliagePalette swatches (one quad strip each, facing a random way)."""
    n = len(roots)
    heading = rng.uniform(0.0, 2.0 * np.pi, n)
    side = np.stack([np.cos(heading), np.sin(heading), np.zeros(n)], 1)
    tip_dir = np.stack([-np.sin(heading), np.cos(heading), np.zeros(n)], 1)
    names = np.asarray(swatches)[rng.integers(len(swatches), size=n)]
    V, UV, AO = [], [], []
    for k in range(segments + 1):
        q = k / segments
        p = roots + np.stack([np.zeros(n), np.zeros(n), heights * q], 1) + tip_dir * (lean * heights * (q ** 2 if bend else q))[:, None]
        w = widths * (1.0 - (0.85 if segments > 1 else 0.15) * q)
        V += [p - side * w[:, None], p + side * w[:, None]]
        u = np.full(n, min(max(q, 0.02), 0.98))
        UV += [np.stack([u, [palette_v(s, 0.0) for s in names]], 1), np.stack([u, [palette_v(s, 1.0) for s in names]], 1)]
        AO += [np.full(n, 0.55 + 0.45 * q)] * 2
    Vs = np.stack(V, 1).reshape(-1, 3)                  # per blade: (segments + 1) x 2 vertices
    UVs = np.stack(UV, 1).reshape(-1, 2)
    AOs = np.stack(AO, 1).reshape(-1)
    per = 2 * (segments + 1)
    F = []
    for k in range(segments):
        b = np.arange(n) * per + 2 * k
        F.append(np.stack([b, b + 1, b + 3, b + 2], 1))
    return uc.mesh_object(name, Vs, [np.vstack(F)], lt.material('FoliagePalette'), where, uvs=UVs, ao=AOs)


def field(where, seed=4):
    """The hayfield round Amos's fence."""
    rng = np.random.default_rng(seed)
    # The ground: the soil under a litter of cut straw.
    a = np.linspace(0.0, 2.0 * np.pi, 96, endpoint=False)
    rr = np.array([0.0, 2.0, 5.0, 10.0, 20.0, 40.0, 90.0, 900.0])
    V = [(AMOS_X, 4.0, 0.0)] + [(AMOS_X + r * math.cos(t), 4.0 + r * math.sin(t), 0.0) for r in rr[1:] for t in a]
    V = np.array(V)
    faces = [uc.fan(0, np.arange(96) + 1, top=True), uc.grid_faces(len(rr) - 1, 96)[:, ::-1] + 1]
    uc.mesh_object('Ground', V, faces, lt.material('Hay', name='AmosStubbleGround', tint=0xc7ae86), where,
                   uvs=V[:, :2] / 1.4)
    # Stubble in drill rows along the fence, a narrow verge of taller grass along the fence line itself.
    roots = []
    for y in np.arange(-6.0, 15.0, 0.16):
        if abs(y) < 0.32:
            continue
        x = np.sort(rng.uniform(-9.0, 11.0, int(20.0 * 16)))
        clump = np.repeat(x, 3) + rng.normal(0.0, 0.012, len(x) * 3)
        roots.append(np.stack([clump, np.full(len(clump), y) + rng.normal(0.0, 0.02, len(clump)), np.full(len(clump), -0.004)], 1))
    roots = np.vstack(roots)
    stalks(where, 'Stubble', roots, rng.uniform(0.05, 0.13, len(roots)), rng.uniform(0.0016, 0.0026, len(roots)),
           rng.uniform(0.0, 0.18, len(roots)), ('Straw', 'DryTan', 'Straw', 'GrassDry'), rng)
    verge = np.stack([rng.uniform(-9.0, 11.0, 4200), rng.normal(0.0, 0.14, 4200), np.full(4200, -0.01)], 1)
    stalks(where, 'Verge', verge, rng.uniform(0.18, 0.5, len(verge)), rng.uniform(0.004, 0.007, len(verge)),
           rng.uniform(0.15, 0.45, len(verge)), ('GrassDry', 'Straw', 'GrassYellow', 'DryTan', 'GrassOlive'), rng,
           segments=3, bend=1.0)
    # The uncut half: hay standing gone to seed past the windrows.
    tall = np.stack([rng.uniform(-30.0, 32.0, 52000), rng.uniform(15.5, 34.0, 52000), np.full(52000, -0.01)], 1)
    stalks(where, 'Uncut', tall, rng.uniform(0.45, 0.85, len(tall)), rng.uniform(0.005, 0.009, len(tall)),
           rng.uniform(0.1, 0.35, len(tall)), ('GrassYellow', 'Straw', 'GrassDry', 'DryTan', 'Straw'), rng, segments=3, bend=1.0)
    # The cut hay left lying: windrows, grey with rot, behind him.
    rotten = lt.material('Hay', name='AmosHayRotting', tint=0xb0a58a)
    for k, y in enumerate((3.4, 7.1, 10.9)):
        xs = np.linspace(-11.0, 13.0, 160)
        theta = np.linspace(0.0, np.pi, 12)
        lump = 1.0 + 0.18 * np.sin(xs * 1.7 + k) + 0.1 * np.sin(xs * 4.1 + 2 * k)
        P = np.array([[(x, y - 0.36 * l * math.cos(t), 0.2 * l * math.sin(t) ** 0.8 - 0.01) for t in theta]
                      for x, l in zip(xs, lump)])
        P[..., 1] += 0.15 * np.sin(xs * 0.35 + k)[:, None]
        Vw = P.reshape(-1, 3)
        Fw = uc.grid_faces(len(xs), len(theta), closed=False)
        uv = np.column_stack([Vw[:, 0] / 1.2, np.tile(np.linspace(0, 0.8, len(theta)), len(xs))])
        uc.mesh_object('Windrow', Vw, [Fw], rotten, where, uvs=uv,
                       ao=np.tile(0.6 + 0.4 * np.sin(theta), len(xs)))
    bale = bale_model()
    for x, y, t, tilt in ((-2.7, 5.0, 15.0, 0.0), (3.3, 5.5, -28.0, 0.0), (5.9, 9.1, 82.0, 0.0), (-5.8, 8.8, 5.0, 0.0),
                          (1.2, 12.6, 40.0, 0.0), (-1.1, 5.35, 70.0, 90.0)):
        place_copy(bale, where, (x, y, 0.0 if not tilt else 0.23), math.radians(t), (math.radians(tilt), 0.0), rotten)
    # The fence: Fences.py's own segments, the posts at x = 0 and 1.5 either side of him.
    rail, post = fence_models()
    for x0 in np.arange(-10.5, 12.0, 3.0):
        place_copy(rail, where, (x0, 0.0, 0.0))
    place_copy(post, where, (13.5, 0.0, 0.0))
    ridges(where)


def ridges(where, seed=9):
    """The far side of the valley in the haze: Larkspur Ridge's dark pines, the higher ridges beyond, each paler."""
    rng = np.random.default_rng(seed)
    for k, (dist, top, color) in enumerate(((330.0, 34.0, 0x6d6a55), (620.0, 70.0, 0x8b8a7c), (1150.0, 120.0, 0xa7aca8))):
        mat = uc.flat_material(f'AmosRidge{k}', color, 1.0)
        xs = np.linspace(-2500.0, 2500.0, 400)
        h = top * (0.55 + 0.2 * np.sin(xs / 210.0 + rng.uniform(0, 6)) + 0.14 * np.sin(xs / 70.0 + rng.uniform(0, 6))
                   + 0.06 * np.sin(xs / 19.0 + rng.uniform(0, 6)))
        h += rng.uniform(-1.0, 1.0, len(xs)) * (2.5 if k == 0 else 1.0)
        V = np.vstack([np.stack([xs, np.full_like(xs, dist), np.full_like(xs, -60.0)], -1),
                       np.stack([xs, np.full_like(xs, dist), h], -1)])
        n = len(xs)
        F = np.array([(i, n + i, n + i + 1, i + 1) for i in range(n - 1)])
        obj = uc.mesh_object('Ridge', V, [F], mat, where)
        obj.data.polygons.foreach_set('use_smooth', np.zeros(len(obj.data.polygons), dtype=bool))
        obj.visible_shadow = False


def golden_light(where):
    golden_sky()
    sun = uc.sun(SUN, strength=SUN_STRENGTH, color=SUN_COLOR, where=where)
    sun.data.angle = math.radians(2.5)            # a touch softer, so shadow edges on the faces don't step
    if hasattr(sun.data, 'use_shadow_jitter'):
        sun.data.use_shadow_jitter = True
    return sun


# --- Rendering ---

def render(path, resolution, samples=64):
    if FAST:
        path = os.path.join(WORK, 'fast_' + os.path.basename(path))
        resolution, samples = (resolution[0] // 2, resolution[1] // 2), 12
    os.makedirs(os.path.dirname(path), exist_ok=True)
    uc.render(path, resolution, samples=samples)
    return path


def lens_shot(path, eye, target, lens, resolution, samples=64, focus=None, fstop=None):
    cam = uc.camera(Vector(eye), Vector(target), lens=lens)
    cam.data.clip_end = 5000.0
    if focus is not None:
        cam.data.dof.use_dof = True
        cam.data.dof.focus_distance = focus
        cam.data.dof.aperture_fstop = fstop
    render(path, resolution, samples)
    bpy.data.objects.remove(cam)


def shot_path(key, name):
    return os.path.join(OUT, f'Amos_{key}_{name}.png')


SIT_TURN = math.radians(-25.0)


def sit_location(fig, turn=SIT_TURN):
    """Where the sitting figure goes so that its seat rests on the top rail between the posts."""
    seat_local = np.array([0.0, 0.025, SEAT_Z])
    seat_world = np.array([AMOS_X, 0.0, RAIL_TOP])
    return tuple(seat_world - rotation((0.0, 0.0, 1.0), turn) @ seat_local)


def shot_lean(key):
    """Leaning his forearms on the top rail, talking to Ellis on the far side; the half-in hay behind him."""
    uc.reset()
    where = uc.collection('Lean')
    field(where)
    golden_light(where)
    fig = figure(key, 'lean')
    loc = (AMOS_X, -RAIL_Y, 0.0)
    instantiate(fig, where, loc)
    target = Vector((AMOS_X + 0.05, 0.0, 1.2))
    lens_shot(shot_path(key, 'lean'), (AMOS_X + 1.0, -3.3, 1.52), target, 40.0, (1600, 1000), samples=128,
              focus=(Vector((AMOS_X + 1.0, -3.3, 1.52)) - Vector((AMOS_X, -0.1, 1.45))).length, fstop=4.0)


def shot_sit(key):
    """Sitting on his fence to wait for the saint to come back, looking down the Sundown Road into the sun."""
    uc.reset()
    where = uc.collection('Sit')
    field(where)
    golden_light(where)
    fig = figure(key, 'sit')
    loc = sit_location(fig)
    instantiate(fig, where, loc, SIT_TURN)
    target = Vector((AMOS_X - 0.05, -0.15, 1.3))
    eye = Vector((AMOS_X + 0.6, -3.4, 1.48))
    lens_shot(shot_path(key, 'sit'), eye, target, 40.0, (1600, 1000), samples=128,
              focus=(eye - Vector((AMOS_X, -0.2, 1.5))).length, fstop=4.0)


def shot_face(key):
    """His face as Ellis sees it across the fence: leaning on the rail, in the low sun."""
    uc.reset()
    where = uc.collection('Face')
    field(where)
    golden_light(where)
    fig = figure(key, 'lean')
    loc = (AMOS_X, -RAIL_Y, 0.0)
    instantiate(fig, where, loc)
    head = Vector(to_world(fig.marks['head'], loc, 0.0))
    eye = head + Vector((0.3, -1.0, -0.02)).normalized() * 1.15
    lens_shot(shot_path(key, 'face'), eye, head + Vector((0.0, 0.0, 0.02)), 85.0, (1200, 1200), samples=128,
              focus=(head - eye).length - 0.08, fstop=5.6)


def studio(where, floor=0xc4b59c):
    uc.ground(where, floor, plain=True)
    golden_sky(0.85)


def shot_front(key):
    """Front, side and back beside an Unpaid and a 1.8 m post, orthographic, in the golden light from the front left."""
    uc.reset()
    where = uc.collection('Front')
    fig = figure(key, 'idle')
    turns = (0.0, math.pi / 2.0, math.pi)
    spans = [(fig.bounds(t)[0][0], fig.bounds(t)[1][0]) for t in turns]
    unpaid = uc.figure('D', 'idle')
    ulo, uhi = unpaid.bounds(0.0)
    xs, total = uc.lineup([(-0.08, 0.08), (ulo[0], uhi[0])] + spans, gap=0.6)
    uc.post(where, (xs[0], 0.0, 0.0))
    uc.label('1.8 m', (xs[0] + 0.1, -0.05, 1.78), 0.075, align='LEFT')
    uc.instantiate(unpaid, where, location=(xs[1], 0.0, 0.0), turn=0.0)
    uc.label('an Unpaid', (xs[1] + (ulo[0] + uhi[0]) / 2, -0.6, 0.06), 0.1)
    for x, t, name, (lo, hi) in zip(xs[2:], turns, ('front', 'side', 'back'), spans):
        instantiate(fig, where, location=(x, 0.0, 0.0), turn=t)
        uc.label(name, (x + (lo + hi) / 2, -0.6, 0.06), 0.1)
    uc.label(TITLES[key], (xs[0] - 0.1, -1.5, 2.3), 0.12, align='LEFT')
    studio(where)
    uc.sun((-0.42, -0.85, 0.36), strength=4.0, color=SUN_COLOR)
    width = total + 0.7
    height = 2.55
    cam = uc.camera((0.0, -30.0, 1.12 + 30.0 * math.tan(math.radians(7.0))), (0.0, 0.0, 1.12), ortho=width)
    render(shot_path(key, 'front'), (2400, int(2400 * height / width)), 96)


def shot_options():
    """The three side by side in the same light at the same scale, an Unpaid and a 1.8 m post beside them."""
    uc.reset()
    where = uc.collection('Options')
    turn = math.radians(-26.0)
    figs = [figure(k, 'idle') for k in OPTIONS]
    unpaid = uc.figure('D', 'idle')
    ulo, uhi = unpaid.bounds(turn)
    items = [(-0.08, 0.08), (ulo[0], uhi[0])]
    for fig in figs:
        lo, hi = fig.bounds(turn)
        items.append((lo[0], hi[0]))
    xs, total = uc.lineup(items, gap=0.62)
    uc.post(where, (xs[0], 0.0, 0.0))
    uc.label('1.8 m', (xs[0] + 0.1, -0.05, 1.78), 0.075, align='LEFT')
    uc.instantiate(unpaid, where, location=(xs[1], 0.0, 0.0), turn=turn)
    uc.label('an Unpaid', (xs[1] + (ulo[0] + uhi[0]) / 2, -0.75, 0.05), 0.09)
    for x, fig, (lo, hi) in zip(xs[2:], figs, items[2:]):
        instantiate(fig, where, location=(x, 0.0, 0.0), turn=turn)
        uc.label(TITLES[fig.key], (x + (lo + hi) / 2, -0.75, 0.05), 0.09)
    studio(where)
    uc.sun((-0.45, -0.85, 0.36), strength=4.0, color=SUN_COLOR)
    width = total + 0.6
    cam = uc.camera((0.0, -30.0, 1.1 + 30.0 * math.tan(math.radians(7.0))), (0.0, 0.0, 1.1), ortho=width)
    render(os.path.join(OUT, 'Amos_options.png'), (2600, int(2600 * 2.45 / width)), 128)


def shot_debug(key, pose='idle'):
    """Quick checks of a figure (not deliverables), into WORK."""
    uc.reset()
    where = uc.collection('Debug')
    fig = figure(key, pose)
    instantiate(fig, where)
    studio(where, 0x9d9484)
    uc.sun((-0.45, -0.8, 0.45), strength=3.6, color=SUN_COLOR)
    c = Vector((0.0, -0.1, 1.05))
    paths = []
    for name, d in (('front', (0.0, -1.0, 0.05)), ('three', (0.8, -1.0, 0.1)), ('side', (1.0, 0.0, 0.05)),
                    ('back', (0.3, 1.0, 0.1))):
        cam = uc.camera(c + Vector(d).normalized() * 3.6, c, lens=50.0)
        paths.append(os.path.join(WORK, f'debug_{key}_{pose}_{name}.png'))
        uc.render(paths[-1], (600, 900), samples=12)
        bpy.data.objects.remove(cam)
    uc.compose(paths, os.path.join(WORK, f'debug_{key}_{pose}.png'))


def shot_headlab(key, hat=False):
    """The bare head (and the hat if asked) in a soft key light, front, three-quarter and side: for shaping the face."""
    uc.reset()
    where = uc.collection('HeadLab')
    fig = figure(key, 'idle')
    mats = materials(key)
    for part, options in fig.parts:
        if part.name in ('Head', 'Hair', 'Straw') or (hat and part.name == 'Hat'):
            build_part(part, mats[options.get('material', 'body')], where,
                       **{k: v for k, v in options.items() if k != 'material'})
    studio(where, 0x6f6a62)
    uc.sun((-0.5, -0.85, 0.25), strength=3.4, color=SUN_COLOR)
    head = Vector(fig.marks['head'])
    paths = []
    for name, d in (('front', (0.0, -1.0, 0.0)), ('three', (0.55, -1.0, 0.04)), ('side', (1.0, -0.12, 0.02))):
        cam = uc.camera(head + Vector(d).normalized() * 0.62, head + Vector((0.0, 0.0, -0.01)), lens=85.0)
        path = os.path.join(WORK, f'headlab_{key}_{name}.png')
        uc.render(path, (700, 760), samples=24)
        paths.append(path)
        bpy.data.objects.remove(cam)
    uc.compose(paths, os.path.join(WORK, f'headlab_{key}.png'))


def main():
    keys = [a for a in ARGV if a in OPTIONS] or list(OPTIONS)
    if 'headlab' in ARGV:
        for key in keys:
            shot_headlab(key, hat='hat' in ARGV)
        return
    if 'debug' in ARGV:
        poses = [p for p in ('idle', 'lean', 'sit') if p in ARGV] or ['idle']
        for key in keys:
            for pose in poses:
                shot_debug(key, pose)
        return
    if 'build' in ARGV:
        for key in keys:
            for pose in ('idle', 'lean', 'sit'):
                figure(key, pose)
        return
    shots = [a for a in ARGV if a in SHOTS] or list(SHOTS)
    funcs = {'lean': shot_lean, 'sit': shot_sit, 'front': shot_front, 'face': shot_face}
    for key in keys:
        for shot in shots:
            if shot in funcs:
                funcs[shot](key)
    if 'options' in shots:
        shot_options()


if __name__ == '__main__':
    main()
