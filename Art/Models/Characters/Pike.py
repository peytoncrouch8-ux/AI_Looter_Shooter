"""SK_Pike: Wendell Pike, the Surveyor (Docs/Story.md "The heroes"), the railroad's old surveyor, built from the portrait
the user picked on 2026-10-19: option A, The Surveyor, of Art/Portraits/Pike.py (its OPTIONS["A"], FACE and MACRO are
the spec; the painted bust Saved/ArtPreviews/Portraits/Pike/PikeA.png). The lower half from the old concept model's
layout (Art/Backlog/Characters/Pike.py: the coat to mid-thigh, puttees, ankle boots, the pack's full shape). The build
contract: Saved/Heroes/CONTRACT.md. The worked example this follows: Art/Models/Characters/Ellis.py. Boots to hat:
  - the body: MakeHuman's CC0 base shaped as the portrait's stocky man of fifty-five, 1.68 m barefoot, 2.5 cm up on boot
    heels; bare weathered hands; the eyes narrowed, burning Revenant cyan behind round brass spectacles
  - brown leather ankle boots laced over hooks, olive wool puttees wound from the knee to the boot, olive breeches
  - a khaki drill field coat to mid-thigh, worn open over a brown wool waistcoat (brass buttons) and a linen shirt:
    lapels and a low rolled collar, buttoned cuffs, patch pockets with buttoned flaps on the chest and the skirt (a
    pencil in the left one); a leather belt over it with two pouches and a frog at the left hip, where a brass spyglass
    hangs in its sling
  - a cartridge bandolier from the right shoulder to the left hip; the pack's shoulder straps down to the belt
  - the pack on his back: a canvas rucksack with a buckled lid, the bedroll strapped under it, the surveyor's tripod
    folded (wooden legs, brass-shod) and the map tube riding up its right side. No rifle (the game's guns are his).
  - a teal neckerchief knotted at the throat, its point down the chest
  - grey mutton chops joined to a big mustache, bushy brows, short grey hair under a brown bowler, brass goggles up on
    the bowler's crown
Rig (Tools/Blender/pike_rig.py): the UE5 mannequin's 63 deform bones on MakeHuman's joints (looter_human.py), the
forearms' lowerarm_twist_01/02, swing chains for the game's springs (the coat's skirt, the neckerchief's point, the
spyglass), `hat` (tipped back while aiming). Built facing -Y, turned last to face -X here (+Y in Unreal).

    artrun.ps1 -Script Art\\Models\\Characters\\Pike.py -Threads 6 -ScriptArgs <flags>
    --shape [--shots full+upper+face+feet+back]   the build in clay, no bake (Saved/ArtPreviews/Characters/Pike)
    --stats        the game mesh's triangles per part, no bake
    --rig-check    the rig in clay, no bake (Saved/Concepts/PikeSwing)
    --preview      the baked look, arms down: front, three-quarter, side, back and the face
    --audit <tag>  the game's poses, every layer checked (Saved/Concepts/PikeAudit/<tag>)
    --rebake       bake even when the scripts haven't changed; --size N the sets' size (2048)
"""
import importlib.util
import math
import os
import sys
import time

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
for p in (os.path.join(REPO, 'Tools', 'Blender'), os.path.join(REPO, 'Art', 'Portraits')):
    if p not in sys.path:
        sys.path.insert(0, p)
import bust  # noqa: E402
import looter_bake as bk  # noqa: E402
import looter_garments as gm  # noqa: E402
import looter_human as hu  # noqa: E402
import looter_looks as lk  # noqa: E402
import looter_model as lm  # noqa: E402
import looter_textures as lt  # noqa: E402
import mh  # noqa: E402


def _portrait():
    """Art/Portraits/Pike.py, the portrait's module, under a name of its own (this file is Pike.py too)."""
    name = 'PikePortrait'
    if name in sys.modules:
        return sys.modules[name]
    spec = importlib.util.spec_from_file_location(name, os.path.join(REPO, 'Art', 'Portraits', 'Pike.py'))
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod


pp = _portrait()
import pike_clothes as pc  # noqa: E402
import pike_gear as pg  # noqa: E402
import pike_hair as ph  # noqa: E402
import pike_looks as pl  # noqa: E402

M = sys.modules[__name__]
ARGS = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
T0 = time.time()
OPT = pp.OPTIONS['A']
MACRO = pp.MACRO
FACE = pp.FACE
HEIGHT = 1.68            # barefoot, the top of the head (m)
LIFT = 0.025             # the boots' heels and soles raise the body this much
BELT = 0.995             # the waist belt's middle (m), over the coat
HEM = 0.655              # the coat's hem: mid-thigh (the old concept's knee + 16 cm)
PUTTEE_TOP = 0.462       # the puttees' top, just under the knee; the breeches bloused over it
BOOT_TOP = 0.165         # the ankle boots' tops
FWD = np.array([0.0, -1.0])
SIDE_SEAM = 1.83         # the coat's skirt side seams, radians round from the front


def log(message):
    print(f'PIKE [{time.time() - T0:6.1f}s] {message}', flush=True)


def arg_value(name, default=None):
    return ARGS[ARGS.index(name) + 1] if name in ARGS and ARGS.index(name) + 1 < len(ARGS) else default


# ------------------------------------------------------------------------------------------------------------ the body

class Figure:
    """The shaped body and everything measured from it that the clothes are fitted to."""

    def __init__(self):
        self.b = hu.Body(MACRO, FACE, HEIGHT, LIFT)
        h = self.b.h
        self.balls = bust.close_lids(h, OPT['lids'])      # narrowed eyes: the lids slide over the balls (MakeHuman space)
        self.hf = bust.head_frame(h)
        self.wv = self.b.world_verts()
        self.names, self.Wv = self.b.vertex_weights()
        self.col = {n: i for i, n in enumerate(self.names)}
        W = lambda *bones: sum(self.b.mh_weight(x) for x in bones)
        sides = ('L', 'R')
        self.w_arm = sum(W(f'shoulder01.{s}', f'upperarm01.{s}', f'upperarm02.{s}', f'lowerarm01.{s}', f'lowerarm02.{s}')
                         for s in sides)
        self.w_hand = sum(W(f'wrist.{s}', *[f'finger{k}-{i}.{s}' for k in range(1, 6) for i in (1, 2, 3)],
                            *[f'metacarpal{k}.{s}' for k in range(1, 5)]) for s in sides)
        self.w_head = self.Wv[:, self.col['head']] + self.Wv[:, self.col['neck_02']] + 0.6 * self.Wv[:, self.col['neck_01']]
        self.w_leg = sum(self.Wv[:, self.col[f'{p}_{s}']] for p in ('thigh', 'calf', 'foot', 'ball') for s in 'lr')
        self.w_foot = sum(self.Wv[:, self.col[f'{p}_{s}']] for p in ('foot', 'ball') for s in 'lr')
        self.faces = self.b.body_faces
        self.fuv = [self.b.h['uv'][fu] for fu, g in zip(self.b.h['fuv'], self.b.h['groups']) if g and g.startswith('body')]
        sk = self.b.skeleton()
        self.sk = sk
        self.neck = sk['neck_01'][0]
        self.shoulder_z = sk['clavicle_l'][1][2]
        used = self.b.used
        torso = used[(self.w_arm[used] < 0.3) & (self.w_hand[used] < 0.1) & (self.w_head[used] < 0.3)]
        tz = self.wv[torso]
        chest = tz[(tz[:, 2] > 1.15) & (tz[:, 2] < 1.33)]
        self.centre = np.array([0.0, (chest[:, 1].min() + chest[:, 1].max()) * 0.5])
        self.torso_idx = torso
        g = self.head_point
        self.eyes = [g(e) for e in self.hf['eyes']]
        self.eye_z = (self.eyes[0][2] + self.eyes[1][2]) * 0.5
        head = self.wv[used][self.w_head[used] > 0.5]
        front = head[head[:, 1] < np.percentile(head[:, 1], 25)]
        self.chin_z = float(front[:, 2].min())
        feet = self.wv[used][self.w_foot[used] > 0.3]
        self.toe_y = float(feet[:, 1].min()) - 0.025          # the boot's toe, past the toes (pike_clothes.boot_last)
        self.beard = beard_mask(self)
        log(f'body: {len(used)} vertices, scale {self.b.scale:.3f}, eyes at {self.eye_z:.3f} m, chin {self.chin_z:.3f}, '
            f'neck {self.neck[2]:.3f}, shoulders {self.shoulder_z:.3f}, chest centre y {self.centre[1]:.3f}')

    def head_point(self, p):
        return self.b.to_world(np.asarray(p, dtype=np.float64)[None])[0]

    def face_mask(self, vert_mask):
        """Faces of the body whose every vertex is in vert_mask (bool per MakeHuman vertex)."""
        return [bool(vert_mask[f].all()) for f in self.faces]

    def to_portrait(self, ob):
        """Moves an object the portrait's builders made (MakeHuman metres, no size or lift) into this figure's space."""
        s = self.b.scale
        ob.matrix_world = Matrix.Translation((0.0, 0.0, -s * 0.1 * self.b.floor + LIFT)) @ Matrix.Scale(s, 4) @ ob.matrix_world

    def forearm_share(self, s, P):
        """How far down the forearm on side s points are (0 the elbow, 1 the wrist; past it, more)."""
        e, w = self.sk[f'lowerarm_{s}'][0], self.sk[f'lowerarm_{s}'][1]
        length = np.linalg.norm(w - e)
        return ((P - e) @ ((w - e) / length)) / length


def beard_mask(fig):
    """Per MakeHuman vertex, how much the mutton chops, the mustache and the corners joining them cover it (0..1): the
    portrait's regions (Art/Portraits/Pike.py build_hair), in the head's rest space (decimetres)."""
    v = fig.b.h['verts']
    ey = fig.hf['eye_y']
    ax = np.abs(v[:, 0])
    ang = np.degrees(np.arctan2(ax, v[:, 2] - 0.45))
    top = ey - 0.64 + 0.82 * np.clip((ang - 35) / 65, 0, 1)
    chops = (ax > 0.42) & (ang > 30) & (ang < 104) & (v[:, 1] > ey - np.where(ang < 60, 0.8, 1.2)) & (v[:, 1] < top)
    corner = (ax > 0.30) & (ax < 0.55) & (v[:, 1] > ey - 0.72) & (v[:, 1] < ey - 0.52) & (v[:, 2] > 0.9)
    mous = (ax < 0.42) & (v[:, 1] > ey - 0.63) & (v[:, 1] < ey - 0.47) & (v[:, 2] > 1.1)
    head = fig.w_head > 0.6
    # no lower than the chops' cards reach (pike_hair: the jaw, higher toward the ear): below it the grey stubble showed
    # as a dark patch on the neck
    jaw = fig.wv[:, 2] > fig.chin_z + 0.010 + 0.016 * np.clip((ang - 55.0) / 30.0, 0.0, 1.0)
    return ((chops | corner | mous) & head & jaw).astype(np.float64)


# ------------------------------------------------------------------------------------------------------------ looks

SLOTS = {}
HAT = {}                             # the hat band's height (the felt's sweat stain) and the hat bone's pivot


def slot(key):
    """The look (looter_looks / pike_looks) for a part of the costume, one material per look; the bake gathers them
    into the texture sets of TEXTURE_SETS."""
    if key not in SLOTS:
        SLOTS[key] = LOOKS[key](f'Pike_{key}')
        SLOTS[key]['LookKey'] = key
    return SLOTS[key]


LOOKS = {
    'Skin': lambda n: lk.skin(n, OPT['skin']),
    'Scalp': lambda n: lk.scalp(n, (0.34, 0.32, 0.30)),              # grey stubble under the whiskers and the hair
    'EyeGlow': lambda n: lk.glow(n, OPT['glow'], 6.0),
    'Felt': lambda n: lk.felt(n, (0.10, 0.058, 0.032), (0.30, 0.18, 0.10), band_z=HAT.get('band_z')),
    'HatBand': lambda n: pl.grosgrain(n, (0.06, 0.042, 0.032)),
    'Hair': lambda n: ph.material(n),                                 # the cards' strand atlas (script-made, not baked)
    'Kerchief': lambda n: lk.bandana(n, (0.03, 0.22, 0.24), (0.10, 0.42, 0.42), figure=(0.36, 0.60, 0.56)),
    'Coat': lambda n: pl.drill(n, (0.40, 0.355, 0.235), (0.58, 0.52, 0.36)),
    'Waistcoat': lambda n: pl.tweed(n, (0.16, 0.095, 0.055), (0.30, 0.19, 0.11)),
    'Shirt': lambda n: lk.linen(n, (0.70, 0.66, 0.57)),
    'Trousers': lambda n: lk.wool(n, (0.33, 0.34, 0.22), dust_amt=0.6),
    'Puttees': lambda n: pl.puttee(n, (0.50, 0.47, 0.36)),
    'Canvas': lambda n: pl.drill(n, (0.27, 0.25, 0.17), (0.40, 0.37, 0.26), dust_amt=0.6),
    'Bedroll': lambda n: pl.blanket(n, (0.42, 0.22, 0.17), (0.62, 0.52, 0.38)),
    # leather grain under the bake's limit (Ellis.py: 900-1100/m aliased into a fuzz that read as suede)
    'Boots': lambda n: lk.leather(n, (0.30, 0.18, 0.11), grain=280.0, dust_amt=0.45, dust_height=0.12, mud=0.4,
                                  boot=(FIG_TOE[0] + 0.05, 0.0215)),
    'Soles': lambda n: lk.stacked(n, (0.12, 0.08, 0.055)),
    'Lace': lambda n: lk.leather(n, (0.22, 0.15, 0.10), grain=280.0, dust_amt=0.3, dust_height=0.15),
    'Leather': lambda n: lk.leather(n, (0.27, 0.16, 0.09), grain=280.0, dust_amt=0.3, dust_height=0.001),
    'TubeLeather': lambda n: lk.leather(n, (0.50, 0.33, 0.17), grain=280.0, dust_amt=0.3, dust_height=0.001,
                                        scuff=(0.70, 0.55, 0.38)),
    'Brass': lambda n: lk.metal(n, (0.58, 0.42, 0.19), (0.86, 0.68, 0.36)),
    'Lead': lambda n: lk.metal(n, (0.38, 0.37, 0.36), (0.58, 0.56, 0.54), rough=(0.4, 0.6), tarnish=(0.2, 0.2, 0.2)),
    'Iron': lambda n: lk.metal(n, (0.05, 0.055, 0.065), (0.17, 0.18, 0.20), rough=(0.3, 0.55), tarnish=(0.05, 0.04, 0.03)),
    'Wood': lambda n: pl.wood(n, (0.34, 0.20, 0.10), (0.60, 0.40, 0.21)),
    'Glass': lambda n: pl.glass(n, (0.08, 0.12, 0.10)),
}
FIG_TOE = [-0.20]                    # the boot's toe tip (y), set by Figure before the boots' look is made

# The texture sets the looks bake into (Art/Textures/<Set>/), and the glowing looks the game draws as Glow materials.
TEXTURE_SETS = {
    'PikeSkin': ('Skin', 'Scalp'),
    'PikeCoat': ('Coat',),
    'PikeGarb': ('Felt', 'HatBand', 'Kerchief', 'Waistcoat', 'Shirt', 'Trousers', 'Puttees', 'Canvas', 'Bedroll'),
    'PikeGear': ('Boots', 'Soles', 'Lace', 'Leather', 'TubeLeather', 'Brass', 'Lead', 'Iron', 'Wood', 'Glass'),
}
# Sets made by a script, not baked: the hair cards' strand atlas (Tools/Blender/pike_hair.py; its UVs are the cards' own)
MADE_SETS = {'PikeHair': ('Hair',)}
GLOWS = {'EyeGlow': (0x5AD8FF, 4.0)}

# The game's material slots, each kind of surface on the master made for it (as Ellis.py's): (master, looks, params).
GAME_MATERIALS = {
    'PikeSkin': ('CharacterSkin', ('Skin', 'Scalp'), {}),
    'PikeCoat': ('CharacterCloth', ('Coat',), {'FuzzStrength': 0.25, 'FuzzLift': 0.1}),
    'PikeGarbCloth': ('CharacterCloth', ('Felt', 'HatBand', 'Kerchief', 'Waistcoat', 'Shirt', 'Trousers', 'Puttees',
                                         'Canvas', 'Bedroll'), {'FuzzStrength': 0.3, 'FuzzLift': 0.12}),
    # grey hair: the atlas is light already; Ellis's 0.55 tint darkened his dark hair in the golden backlight
    'PikeHair': ('CharacterHair', ('Hair',), {'tint': '#D8D8D8'}),
    'PikeGear': ('Character', ('Boots', 'Soles', 'Lace', 'Leather', 'TubeLeather', 'Brass', 'Lead', 'Iron', 'Wood',
                               'Glass'), {}),
}


# --------------------------------------------------------------------------------------------------------- the skin

SKIN_CUFF = 0.07         # the bare skin runs this far up the forearm inside the sleeve (m): no gap at the cuff


def build_skin(fig):
    """The visible skin: the head and the neck down under the collar, and the hands to SKIN_CUFF inside the sleeves.
    Under the hat and the whiskers it takes the grey Scalp look, so nothing pink shows between the cards."""
    neck_w = fig.Wv[:, fig.col['head']] + fig.Wv[:, fig.col['neck_02']] + fig.Wv[:, fig.col['neck_01']]
    keep = (fig.w_head > 0.45) | (fig.w_hand > 0.45) | ((neck_w > 0.5) & (fig.wv[:, 2] > fig.neck[2] - 0.03))
    for s in 'lr':
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        fore = fig.Wv[:, fig.col[f'lowerarm_{s}']] + fig.Wv[:, fig.col[f'hand_{s}']]
        t = fig.forearm_share(s, fig.wv)
        keep |= (fore > 0.5) & (t > 1.0 - SKIN_CUFF / np.linalg.norm(w - e)) & (t < 1.3)
    ob = fig.b.mesh('Pike', keep=lambda i, f: bool(keep[f].all()))
    ob.data.materials.append(slot('Skin'))
    ob.data.materials.append(slot('Scalp'))
    hv = fig.b.h['verts']
    hf = fig.hf
    cz = (hf['verts'][:, 2].max() + hf['verts'][:, 2].min()) * 0.5 - 0.1
    az = np.degrees(np.abs(np.arctan2(hv[:, 0], hv[:, 2] - cz)))
    # under the bowler: the band crosses the forehead ~4 cm over the eyes; the short hair round the back to the nape
    line = np.interp(az, [0, 46, 70, 100, 125, 180], [0.40, 0.38, 0.22, 0.05, -0.55, -0.72]) + hf['eye_y']
    scalp = (hv[:, 1] > line) & (fig.w_head > 0.5)
    beard = fig.beard > 0.5
    idx = np.array([d.value for d in ob.data.attributes['mh_index'].data])
    for p in ob.data.polygons:
        v = idx[list(p.vertices)]
        if scalp[v].all() or beard[v].sum() >= len(v) - 1:
            p.material_index = 1
    # the skin's painted zones (looter_looks.skin multiplies its colour by them): a weathered man of fifty-five
    attr = ob.data.color_attributes.new('zones', 'FLOAT_COLOR', 'POINT')
    attr.data.foreach_set('color', skin_zones(fig, idx).astype(np.float32).ravel())
    eyes = []
    for side in ('l', 'r'):
        c, r = fig.balls[side]
        c = fig.head_point(c)
        bm = bmesh.new()
        bmesh.ops.create_uvsphere(bm, u_segments=20, v_segments=12, radius=r * 0.1 * fig.b.scale)
        me = bpy.data.meshes.new(f'Eye_{side}')
        bm.to_mesh(me)
        bm.free()
        me.shade_smooth()
        eo = bpy.data.objects.new(f'Eye_{side}', me)
        bpy.context.scene.collection.objects.link(eo)
        eo.location = Vector(c)
        eo.data.materials.append(slot('EyeGlow'))
        eyes.append(eo)
    log(f'skin: {len(ob.data.polygons)} faces')
    return ob, eyes


def skin_zones(fig, idx):
    """The skin's colour zones (RGBA per vertex of the skin, multiplied into its colour): the nose and the cheeks
    weathered red, the ears flushed, the eye sockets a little sunk, the lips, the knuckles reddened; 1 elsewhere. In the
    head's rest space (MakeHuman decimetres), as bust.zones."""
    v = fig.b.h['verts'][idx]
    c = np.ones((len(v), 4))
    hw = fig.w_head[idx]
    eyes = fig.hf['eyes']
    for e in eyes:
        d = np.linalg.norm((v - e) * np.array([1.0, 1.35, 1.0]), axis=1)
        k = np.clip(1.0 - (d - 0.10) / 0.30, 0.0, 1.0) ** 1.5 * (hw > 0.5)
        c[:, :3] *= 1.0 - k[:, None] * np.array([0.22, 0.26, 0.22])
    front = fig.b.h['verts'][fig.w_head > 0.5]
    nose = front[np.argmax(front[:, 2])]
    d = np.linalg.norm(v - nose, axis=1)
    k = np.clip(1.0 - d / 0.38, 0.0, 1.0) * (hw > 0.5)
    c[:, :3] *= 1.0 - k[:, None] * np.array([-0.04, 0.16, 0.16])           # a drinker's red nose
    lip = nose + np.array([0.0, -0.48, -0.10])
    d = np.linalg.norm((v - lip) * np.array([0.8, 2.2, 1.0]), axis=1)
    k = np.clip(1.0 - d / 0.3, 0.0, 1.0) ** 0.7 * (hw > 0.5)
    c[:, :3] *= 1.0 - k[:, None] * np.array([0.10, 0.22, 0.18])
    for sx in (-1.0, 1.0):
        cheek = np.array([0.40 * sx, nose[1] + 0.05, nose[2] - 0.50])
        d = np.linalg.norm(v - cheek, axis=1)
        k = np.clip(1.0 - d / 0.42, 0.0, 1.0) * (hw > 0.5)
        c[:, :3] *= 1.0 - k[:, None] * np.array([-0.02, 0.12, 0.12])
        ear = np.array([0.80 * sx, nose[1] + 0.25, nose[2] - 1.15])
        d = np.linalg.norm(v - ear, axis=1)
        k = np.clip(1.0 - d / 0.45, 0.0, 1.0) * (hw > 0.5)
        c[:, :3] *= 1.0 - k[:, None] * np.array([0.0, 0.14, 0.12])
    # the hands: the knuckles and the backs reddened and darker from the sun
    hand = fig.w_hand[idx] > 0.3
    c[hand, :3] *= np.array([0.96, 0.86, 0.80])
    return c


# ------------------------------------------------------------------------------------------------------ the previews

SHOTS = {'full': (0.86, 4.0, 0.25), 'upper': (1.36, 1.6, 0.08), 'face': (1.58, 0.85, 0.02), 'feet': (0.22, 1.5, 0.35),
         'hips': (0.92, 1.8, 0.1)}


def preview(path, views=(('front', 0.0), ('three', 35.0), ('side', 90.0), ('back', 180.0)), size=900, samples=24,
            aim=SHOTS['full']):
    """Quick Cycles renders round the figure (shape checks; with --shape in clay)."""
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = samples
    sc.cycles.use_denoising = True
    sc.render.resolution_x = int(size * 0.75)
    sc.render.resolution_y = size
    sc.render.film_transparent = False
    try:
        sc.view_settings.view_transform = 'AgX'
    except Exception:
        pass
    world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    sc.world = world
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.32, 0.36, 0.42, 1.0)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.6
    if 'Sun' not in bpy.data.objects:
        ld = bpy.data.lights.new('Sun', 'SUN')
        ld.energy = 3.5
        ld.angle = math.radians(8)
        lo = bpy.data.objects.new('Sun', ld)
        sc.collection.objects.link(lo)
        lo.rotation_euler = (math.radians(50), 0.0, math.radians(-35))
    if '--shape' in ARGS:
        # shape checks in clay (the looks unbaked are minutes a frame); the hair cards keep their atlas, its alpha
        clay = bpy.data.materials.get('_Clay')
        if clay is None:
            clay = bpy.data.materials.new('_Clay')
            clay.use_nodes = True
            b = clay.node_tree.nodes['Principled BSDF']
            b.inputs['Base Color'].default_value = (0.55, 0.52, 0.48, 1.0)
            b.inputs['Roughness'].default_value = 0.7
        for ob in bpy.context.scene.objects:
            if ob.type == 'MESH' and family(ob.name) not in CARDS:
                for i in range(len(ob.material_slots)):
                    ob.material_slots[i].material = clay
    cd = bpy.data.cameras.new('PreviewCam')
    cd.lens = 70
    cam = bpy.data.objects.new('PreviewCam', cd)
    sc.collection.objects.link(cam)
    sc.camera = cam
    out = []
    for name, yaw in views:
        a = math.radians(yaw)
        target = Vector((0.0, 0.0, aim[0]))
        cam.location = target + Vector((math.sin(a) * aim[1], -math.cos(a) * aim[1], aim[2]))
        cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = f'{path}_{name}.png'
        bpy.ops.render.render(write_still=True)
        out.append(sc.render.filepath)
    bpy.data.objects.remove(cam)
    return out


# ------------------------------------------------------------------------------------------- UVs, bake, game mesh

# Parts made without usable UVs: smart projection (the bake repacks every island anyway).
SMART_UV = ('Eye', 'Soles', 'Boots', 'HatCrown', 'HatBrim', 'HatBand', 'HatBandInner', 'BrimRoll', 'Goggle', 'GoggleCup',
            'GoggleLens', 'GoggleRim', 'GoggleBridge', 'Spec', 'SpecRim', 'SpecTemple', 'SpecBridge', 'VestButton',
            'CoatButton', 'Buckle', 'Cartridges', 'Bullets', 'Pouch', 'PouchFlap', 'Frog', 'Rucksack', 'RuckBuckle',
            'Bedroll', 'BedrollStrap', 'Tripod', 'TripodShoe', 'TripodBand', 'TripodHead', 'TripodStrap', 'MapTube',
            'MapCap', 'MapBand', 'MapLoop', 'Spyglass', 'SpyglassBrass', 'SpyglassGrip', 'SpyRing', 'KerchiefKnot',
            'Eyelets', 'Laces', 'StrapBuckle', 'Pencil', 'Cuff', 'CuffTab')
# Sheets given thickness after the bake (m), and which way: 1 out along the normals, -1 in, 0 both. What lies on
# something grows inward, so the layers cleared over it (straps over the coat) stay clear; the brim both ways.
THICK = {'Skirt': (0.004, -1.0), 'Collar': (0.003, -1.0), 'Lapel': (0.002, -1.0), 'Pocket': (0.002, -1.0),
         'Flap': (0.002, -1.0), 'Kerchief': (0.0015, -1.0), 'KerchiefTail': (0.0015, 0.0), 'HatBrim': (0.005, 0.0),
         'HatCrown': (0.004, -1.0), 'HatBand': (0.0015, 1.0), 'Belt': (0.005, -1.0), 'GoggleStrap': (0.002, 1.0),
         'RuckLid': (0.003, -1.0), 'RuckBelt': (0.003, 1.0), 'FrogStrap': (0.003, 0.0)}
# Not thickened: the straps and the bandolier (they lie on the coat: no inside shows), the coat's body and sleeves (its inside never shows: the waistcoat fills the opening, the cuffs close
# the sleeves, the belt hides its foot; the lapels give its front edge a thickness), the waistcoat (inside the coat).

# The game mesh (up to 35k triangles, the user's budget for Ellis): each detailed part's share kept by Decimate; 1 keeps
# a part whole (small hard parts: reduced, they shred). Keyed by family (the name before '_' or '.').
LOW = {'Pike': 1.0, 'Eye': 0.35, 'Trousers': 0.3, 'Puttee': 0.06, 'Boots': 0.13, 'Soles': 0.5, 'Laces': 0.35,
       'Eyelets': 0.35, 'Waistcoat': 0.17, 'Shirt': 0.4, 'VestButton': 0.35, 'Coat': 0.055, 'Skirt': 0.13, 'Collar': 0.3,
       'Lapel': 0.6, 'Pocket': 0.5, 'Flap': 0.6, 'CoatButton': 0.35, 'Pencil': 1.0, 'Cuff': 0.5, 'CuffTab': 1.0,
       'Kerchief': 0.065, 'KerchiefKnot': 0.6, 'KerchiefTail': 1.0, 'Belt': 0.4, 'Buckle': 1.0, 'Pouch': 0.6,
       'PouchFlap': 0.6, 'Frog': 1.0, 'FrogStrap': 1.0, 'Bandolier': 0.28, 'Cartridges': 0.45, 'Bullets': 0.25,
       'Strap': 0.35, 'StrapBuckle': 1.0, 'Rucksack': 0.35, 'RuckLid': 0.3, 'RuckBelt': 0.8, 'RuckBuckle': 1.0,
       'Bedroll': 0.8, 'BedrollStrap': 0.8, 'Tripod': 1.0, 'TripodShoe': 1.0, 'TripodBand': 1.0, 'TripodHead': 1.0,
       'TripodStrap': 1.0, 'MapTube': 1.0, 'MapCap': 1.0, 'MapBand': 1.0, 'MapLoop': 1.0, 'Spyglass': 1.0,
       'SpyglassBrass': 1.0, 'SpyglassGrip': 1.0, 'SpyRing': 0.6, 'HatCrown': 0.075, 'HatBrim': 0.12, 'HatBand': 0.5,
       'HatBandInner': 1.0, 'BrimRoll': 0.25, 'Goggle': 1.0, 'GoggleCup': 0.7, 'GoggleLens': 0.7, 'GoggleRim': 0.3,
       'GoggleBridge': 1.0, 'GoggleStrap': 0.4, 'SpecRim': 0.3, 'SpecTemple': 0.5, 'SpecBridge': 1.0}
# Cloth whose open edges Decimate must not chew: a band this wide along them is spared
# (Not the coat's body: its edges lie under the collar, the lapels, the cuffs and the belt, and on its fine mesh a 1 cm
# band kept 5k triangles. Bands a few rows wide on the brim and the neckerchief kept them from reducing at all.)
EDGE_KEEP = {'Skirt': 0.010, 'Collar': 0.006, 'Kerchief': 0.003, 'HatBrim': 0.003, 'Lapel': 0.004}
# The hair cards are the game's hair as built (their UVs are the atlas's): copied, not reduced
CARDS = ('Hair', 'Whiskers', 'Brows')


def family(name):
    return name.split('.')[0].split('_')[0]


def low_ratio(name):
    return LOW.get(family(name.replace('_low', '')), 1.0)


def near_open_edge(ob, co, band):
    """Which vertices lie within band (m) of the mesh's open edges."""
    from mathutils import kdtree
    count = {}
    for p in ob.data.polygons:
        vs = list(p.vertices)
        for k in range(len(vs)):
            e = (min(vs[k], vs[(k + 1) % len(vs)]), max(vs[k], vs[(k + 1) % len(vs)]))
            count[e] = count.get(e, 0) + 1
    rim = sorted({v for e, c in count.items() if c == 1 for v in e})
    if not rim:
        return np.zeros(len(co), dtype=bool)
    tree = kdtree.KDTree(len(rim))
    for i, v in enumerate(rim):
        tree.insert(co[v], i)
    tree.balance()
    return np.array([tree.find(p)[2] < band for p in co])


def unflip(low, high):
    """The faces Decimate turned over turned back to face as the detailed part does there, and its slivers dissolved
    (Unreal draws a face from its front only, so each was a hole; Ellis.py's lesson)."""
    tree = gm.bvh_objects([high])
    bm = bmesh.new()
    bm.from_mesh(low.data)
    bmesh.ops.dissolve_degenerate(bm, dist=1e-6, edges=bm.edges[:])
    bm.normal_update()
    bad = []
    for f in bm.faces:
        loc, nrm, _i, _d = tree.find_nearest(f.calc_center_median(), 0.05)
        if loc is None or f.normal.dot(nrm) >= 0.0:
            continue
        around = Vector()
        for e in f.edges:
            for g in e.link_faces:
                if g is not f:
                    around += g.normal * g.calc_area()
        if around.length == 0.0 or f.normal.dot(around) < 0.0:
            bad.append(f)
    if bad:
        bmesh.ops.reverse_faces(bm, faces=bad)
    bm.to_mesh(low.data)
    bm.free()
    low.data.update()


# The skin's reduction by region (Ellis's ellis_hat.decimate_skin, Pike's regions: his face shows): the lids whole, the
# skull under the crown, the skin under the whiskers (the cards cover it), the neck under the neckerchief and collar,
# the hands (the fingers keep their joints), and the face that shows (nose, cheeks, brow, lips, ears)
SKIN_LOW = {'eyes': 1.0, 'skull': 0.12, 'beard': 0.18, 'neck': 0.08, 'hands': 0.24, 'face': 0.28}
EYE_KEEP = 0.017


def decimate_skin(fig, low):
    import ellis_hat
    s = ellis_hat.seat(fig)
    beard = fig.beard > 0.5
    hand = fig.w_hand > 0.3

    def regions(co, idx):
        d = np.minimum(np.linalg.norm(co - fig.eyes[0], axis=1), np.linalg.norm(co - fig.eyes[1], axis=1))
        eyes = d < EYE_KEEP
        hands = hand[idx] & ~eyes
        skull = ((co - s['O']) @ s['u'] > 0.012) & ~eyes & ~hands
        bd = beard[idx] & ~eyes & ~skull & ~hands
        neck = (co[:, 2] < fig.chin_z - 0.012) & ~eyes & ~skull & ~hands & ~bd
        return {'eyes': eyes, 'skull': skull, 'beard': bd, 'neck': neck, 'hands': hands,
                'face': ~(eyes | skull | bd | neck | hands)}

    before = sum(len(p.vertices) - 2 for p in low.data.polygons)
    for name in ('skull', 'neck', 'beard', 'hands', 'face'):
        share = SKIN_LOW[name]
        if share >= 1.0:
            continue
        co = gm._object_coords(low)
        idx = np.array([d.value for d in low.data.attributes['mh_index'].data])
        reg = regions(co, idx)[name]
        tris = sum(len(p.vertices) - 2 for p in low.data.polygons)
        mine = sum(len(p.vertices) - 2 for p in low.data.polygons if reg[list(p.vertices)].all())
        if not mine:
            continue
        m = low.modifiers.new('decimate', 'DECIMATE')
        m.ratio = (tris - mine * (1.0 - share)) / tris
        m.use_symmetry = True
        m.symmetry_axis = 'X'
        g = low.vertex_groups.new(name='keep')
        g.add([int(i) for i in np.nonzero(~reg)[0]], 1.0, 'REPLACE')
        m.vertex_group = 'keep'
        m.invert_vertex_group = True
        m.vertex_group_factor = 1.0
        gm.apply_modifiers(low, keep=())
        low.vertex_groups.remove(low.vertex_groups['keep'])
    return before, sum(len(p.vertices) - 2 for p in low.data.polygons)


# (outer families, the families under them): reduced layers can cut through one another where the detailed ones only
# just cleared, so each outer part is pushed back outside the reduced parts under it, in this order
CLEAR = (('Waistcoat', ('Shirt',)), ('Coat', ('Waistcoat', 'Shirt')), ('Skirt', ('Trousers', 'Waistcoat')),
         ('Collar', ('Coat',)), ('Lapel', ('Coat',)), ('Cuff', ('Coat',)), ('Pocket', ('Coat', 'Skirt')),
         ('Flap', ('Pocket', 'Coat', 'Skirt')), ('Belt', ('Coat', 'Skirt', 'Waistcoat')), ('Puttee', ('Trousers',)),
         ('Boots', ('Puttee', 'Trousers')), ('Strap', ('Coat', 'Lapel', 'Pocket', 'Flap', 'Belt')),
         ('Bandolier', ('Coat', 'Lapel', 'Pocket', 'Flap', 'Strap', 'Belt', 'Waistcoat')),
         ('Kerchief', ('Pike', 'Collar', 'Coat', 'Lapel', 'Waistcoat', 'Shirt', 'Strap', 'Bandolier')),
         ('Rucksack', ('Coat', 'Strap', 'Bandolier')), ('Bedroll', ('Coat', 'Skirt', 'Belt', 'Strap', 'Bandolier')))


def make_lows(fig, highs):
    """The game mesh: a reduced copy of every detailed part (named <part>_low until the detailed ones go), the skin
    reduced by region, the hair cards copied as they are."""
    lows = []
    for ob in highs:
        low = bpy.data.objects.new(ob.name + '_low', ob.data.copy())
        bpy.context.scene.collection.objects.link(low)
        low.matrix_world = ob.matrix_world              # placed parts (eyes, buttons) keep their place
        bake_transform(low)
        lows.append(low)
        if family(ob.name) in CARDS:
            continue
        ratio = low_ratio(ob.name)
        if ob.name == 'Pike':
            before, after = decimate_skin(fig, low)
            log(f'skin: {before} -> {after} triangles by region')
            unflip(low, ob)
        elif ratio < 1.0:
            m = low.modifiers.new('decimate', 'DECIMATE')
            m.ratio = ratio
            co = gm._object_coords(low)
            band = EDGE_KEEP.get(family(ob.name))
            spare = near_open_edge(low, co, band) if band else np.zeros(len(co), dtype=bool)
            if spare.any():
                g = low.vertex_groups.new(name='keep')
                g.add([int(i) for i in np.nonzero(spare)[0]], 1.0, 'REPLACE')
                m.vertex_group = 'keep'
                m.invert_vertex_group = True
                m.vertex_group_factor = 1.0
            gm.apply_modifiers(low, keep=())
            if 'keep' in low.vertex_groups:
                low.vertex_groups.remove(low.vertex_groups['keep'])
            unflip(low, ob)
    by = lambda *names: [o for o in lows if family(o.name) in names]
    for outer, inner in CLEAR:
        under = by(*inner)
        if not under:
            continue
        tree = gm.bvh_objects(under)
        for ob in by(outer):
            co = gm._object_coords(ob)
            new = gm.clear(co, tree, 0.003, 0.02)
            ob.data.vertices.foreach_set('co', new.astype(np.float32).ravel())
            ob.data.update()
    return lows


def bake_transform(ob):
    ob.data.transform(ob.matrix_world)
    ob.matrix_world = Matrix.Identity(4)


def look_of(ob):
    return ob.material_slots[0].material['LookKey'] if ob.material_slots and ob.material_slots[0].material else None


def prepare_uvs(objects):
    """Every part gets a usable 'UVMap': MakeHuman's on the shells, arc lengths on the drapes and lofts, smart
    projection on small hard parts. (The hair cards' UVs are their atlas's, made with them.)"""
    for ob in objects:
        me = ob.data
        if family(ob.name) in CARDS:
            continue
        if family(ob.name) in SMART_UV or 'UVMap' not in me.uv_layers:
            for layer in list(me.uv_layers):
                me.uv_layers.remove(layer)
            bk.smart_uv(ob)
        me.uv_layers.active = me.uv_layers['UVMap']
        me.uv_layers['UVMap'].active_render = True


BAKE_SOURCES = [os.path.abspath(__file__)] + [os.path.join(REPO, *p) for p in (
    ('Tools', 'Blender', 'looter_human.py'), ('Tools', 'Blender', 'looter_garments.py'), ('Tools', 'Blender', 'looter_looks.py'),
    ('Tools', 'Blender', 'looter_bake.py'), ('Art', 'Portraits', 'mh.py'), ('Art', 'Portraits', 'Pike.py'),
    ('Art', 'Portraits', 'bust.py'), ('Tools', 'Blender', 'ellis_hat.py'), ('Tools', 'Blender', 'ellis_hair.py'),
    ('Tools', 'Blender', 'pike_clothes.py'), ('Tools', 'Blender', 'pike_gear.py'), ('Tools', 'Blender', 'pike_hair.py'),
    ('Tools', 'Blender', 'pike_looks.py'))]


def bake_all(lows, highs, size):
    """Packs every texture set's UVs on the game mesh and bakes it from the detailed parts (skipped when the sets on
    disk were baked from these same scripts at this size: Art/Textures/PikeSkin/bake.digest; --rebake forces it)."""
    stamp = os.path.join(lt.TEXTURE_DIR, 'PikeSkin', 'bake.digest')
    want = bk.digest(BAKE_SOURCES, f'size={size} blender={bpy.app.version_string}')
    have = open(stamp).read().strip() if os.path.exists(stamp) else ''
    out = {}
    for set_name, keys in TEXTURE_SETS.items():
        obs = [o for o in lows if look_of(o) in keys]
        src = [o for o in highs if look_of(o) in keys]
        # the face more texels than the hands; the neckerchief, the small brass and the buckles more than their area
        weights = {o: 1.7 for o in obs if o.name.startswith('Pike')}
        weights.update({o: 1.5 for o in obs if family(o.name) in ('Kerchief', 'KerchiefKnot', 'Goggle', 'GoggleCup',
                                                                 'Buckle', 'StrapBuckle', 'SpyglassBrass')})
        bk.pack(obs, weights=weights)
        if have == want and '--rebake' not in ARGS:
            out[set_name] = bk.load_set(set_name)
        if out.get(set_name) is None:
            out[set_name] = bk.bake_set(set_name, obs, size, sources=src, hide=highs)
            log(f'baked {set_name} ({len(obs)} parts from {len(src)})')
        else:
            log(f'{set_name}: kept (baked from these scripts before)')
    for set_name in MADE_SETS:
        if not ph.have_textures():
            ph.make_textures()
        out[set_name] = bk.load_set(set_name)
    os.makedirs(os.path.dirname(stamp), exist_ok=True)
    with open(stamp, 'w') as f:
        f.write(want + '\n')
    return out


def thicken(objects):
    for ob in objects:
        t = THICK.get(family(ob.name))
        if t:
            gm.solidify(ob, t[0], offset=t[1], rim=True)


def game_materials(skin):
    """The looks swapped for the game's: a textured material per GAME_MATERIALS slot (MI_<Slot> on its master, the maps
    of the set its looks bake into), Glow for the glows."""
    swap = {}
    for slot_name, (master, keys, params) in GAME_MATERIALS.items():
        sets = {s for s, ks in {**TEXTURE_SETS, **MADE_SETS}.items() if set(keys) & set(ks)}
        assert len(sets) == 1, f'{slot_name}: its looks must bake into one set, not {sets}'
        mat = lt.material(sets.pop(), name=slot_name, master=master, **params)
        for k in keys:
            swap[f'Pike_{k}'] = mat
    for k, (hex_value, strength) in GLOWS.items():
        swap[f'Pike_{k}'] = lm.material(f'Pike{k}', hex_value, 'Glow', Glow=strength)
    for i, s_ in enumerate(skin.material_slots):
        if s_.material is not None and s_.material.name in swap:
            skin.material_slots[i].material = swap[s_.material.name]
    bk.select([skin])
    bpy.ops.object.material_slot_remove_unused()


def viewer_materials(skin, baked):
    """The baked maps on a Principled BSDF (previews, the glTF viewer), emission for the glows."""
    mats = {}
    for set_name, keys in TEXTURE_SETS.items():
        imgs = baked[set_name]['images']
        mat = bpy.data.materials.new(f'View_{set_name}')
        mat.use_nodes = True
        nt = mat.node_tree
        nt.nodes.clear()
        out = nt.nodes.new('ShaderNodeOutputMaterial')
        b = nt.nodes.new('ShaderNodeBsdfPrincipled')
        nt.links.new(b.outputs[0], out.inputs['Surface'])
        col = nt.nodes.new('ShaderNodeTexImage')
        col.image = imgs['COLOR']
        nt.links.new(col.outputs['Color'], b.inputs['Base Color'])
        orm = nt.nodes.new('ShaderNodeTexImage')
        orm.image = imgs['ORM']
        sep = nt.nodes.new('ShaderNodeSeparateColor')
        nt.links.new(orm.outputs['Color'], sep.inputs[0])
        nt.links.new(sep.outputs[1], b.inputs['Roughness'])
        nt.links.new(sep.outputs[2], b.inputs['Metallic'])
        # the occlusion multiplied in (the game's DiffuseAO; the glTF exporter's group isn't needed for renders)
        ao = nt.nodes.new('ShaderNodeMix')
        ao.data_type = 'RGBA'
        ao.blend_type = 'MULTIPLY'
        ao.inputs['Factor'].default_value = 0.7
        nt.links.new(col.outputs['Color'], ao.inputs['A'])
        nt.links.new(sep.outputs[0], ao.inputs['B'])
        nt.links.new(ao.outputs['Result'], b.inputs['Base Color'])
        nrm = nt.nodes.new('ShaderNodeTexImage')
        nrm.image = imgs['NORMAL']
        nm = nt.nodes.new('ShaderNodeNormalMap')
        nt.links.new(nrm.outputs['Color'], nm.inputs['Color'])
        nt.links.new(nm.outputs['Normal'], b.inputs['Normal'])
        if set_name == 'PikeSkin':
            b.inputs['Subsurface Weight'].default_value = 0.2
            b.inputs['Subsurface Radius'].default_value = (1.0, 0.45, 0.35)
            b.inputs['Subsurface Scale'].default_value = 0.006
        for k in keys:
            mats[f'Pike_{k}'] = mat
    for set_name, keys in MADE_SETS.items():
        mat = ph.material(f'View_{set_name}')
        for k in keys:
            mats[f'Pike_{k}'] = mat
    for k in GLOWS:
        mats[f'Pike_{k}'] = bpy.data.materials.get(f'Pike_{k}')
    for i, s_ in enumerate(skin.material_slots):
        if s_.material is not None and s_.material.name in mats:
            skin.material_slots[i].material = mats[s_.material.name]


VIEW_POSE = {'upperarm_l': ((0.0, 1.0, 0.0), 34.0), 'upperarm_r': ((0.0, 1.0, 0.0), -34.0),
             'lowerarm_l': ((1.0, 0.0, 0.0), 14.0), 'lowerarm_r': ((1.0, 0.0, 0.0), 14.0)}


def pose_for_view(arm):
    """A relaxed stance for the previews: the arms let down from the bind pose's A toward the sides."""
    from mathutils import Quaternion
    for bone, (axis, deg) in VIEW_POSE.items():
        pb = arm.pose.bones[bone]
        local_axis = pb.bone.matrix_local.to_3x3().inverted() @ Vector(axis)
        pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = Quaternion(local_axis.normalized(), math.radians(deg))
    bpy.context.view_layer.update()


def face_like_mannequin(arm, skin):
    """The finished figure turned a quarter about Z into its data (bones' rest, vertices; the hit zones' places), the
    last thing before the export: in Unreal SK_Pike then faces +Y with his left side at +X, as SKM_Manny_Simple does
    (Ellis.py's face_like_mannequin and its lesson: facing +X, the retargeter clawed the hands)."""
    turn = Matrix.Rotation(math.radians(-90.0), 4, 'Z')
    bk.select([arm], arm)
    arm.data.transform(turn)
    skin.data.transform(turn)
    for ob in arm.children:
        if ob is not skin:
            ob.matrix_world = turn @ ob.matrix_world
    bpy.context.view_layer.update()
    toe = arm.data.bones['ball_l'].head_local - arm.data.bones['foot_l'].head_local
    log(f'turned to face -X here (+Y in Unreal): foot_l to ball_l {tuple(round(c, 3) for c in toe.normalized())}')


def fetch_makehuman():
    """MakeHuman's base mesh is not in git (an .obj would go to LFS): fetch it on a fresh clone."""
    if not os.path.exists(os.path.join(mh.DATA, '3dobjs', 'base.obj')):
        import subprocess
        log('fetching MakeHuman\'s CC0 assets (Art/ThirdParty/MakeHuman/fetch.py)')
        subprocess.run([sys.executable, os.path.join(REPO, 'Art', 'ThirdParty', 'MakeHuman', 'fetch.py')], check=True)


# ------------------------------------------------------------------------------------------------------ the main

def build(fig):
    """Every detailed part, in order (each layer over the ones before it). Returns {name: objects} of the parts the
    later steps need."""
    skin, eyes = build_skin(fig)
    body_ob = fig.b.mesh('_Body')
    body_ob.hide_render = True
    trousers = pc.build_trousers(M, fig)
    puttees = pc.build_puttees(M, fig)
    boots = pc.build_boots(M, fig, puttees)
    vest, shirt, vbuttons = pc.build_waistcoat(M, fig)
    log('trousers, puttees, boots, waistcoat')
    coat = pc.build_coat(M, fig, [vest, shirt])
    skirt = pc.build_skirt(M, fig, [trousers, vest, body_ob], over=[coat])
    cuffs = pc.build_cuffs(M, fig, coat)
    collar = pc.build_collar(M, fig, coat)
    lapels = pc.build_lapels(M, fig, coat)
    pockets = pc.build_pockets(M, fig, coat, skirt)
    log('coat, skirt, cuffs, collar, lapels, pockets')
    belt = pg.build_belt(M, fig, [coat, skirt])
    chest = [coat, skirt, vest, shirt] + lapels + [o for o in pockets if family(o.name) in ('Pocket', 'Flap', 'CoatButton')]
    straps = pg.build_straps(M, fig, chest + belt[:1])
    bandolier = pg.build_bandolier(M, fig, chest + belt[:1] + [o for o in straps if family(o.name) == 'Strap'])
    log('belt, straps, bandolier')
    hat = pg.build_hat(M, fig)
    specs = pg.build_specs(M, fig)
    kerchief = pc.build_kerchief(M, fig, [skin, collar, coat, vest, shirt] + lapels + straps[:2] + bandolier[:1])
    log('hat, spectacles, neckerchief')
    pack = pg.build_pack(M, fig, [coat, skirt, belt[0]] + straps + bandolier[:1])
    spyglass = pg.build_spyglass(M, fig, [skirt, belt[0], trousers])
    log('pack, spyglass')
    hair = ph.build(M, fig, skin, [kerchief[0]])
    log('hair')
    bpy.data.objects.remove(body_ob)
    return dict(skin=skin, eyes=eyes, trousers=trousers, puttees=puttees, boots=boots, vest=vest, shirt=shirt, coat=coat,
                skirt=skirt, cuffs=cuffs, collar=collar, lapels=lapels, pockets=pockets, belt=belt, straps=straps,
                bandolier=bandolier, hat=hat, specs=specs, kerchief=kerchief, pack=pack, spyglass=spyglass, hair=hair)


def main():
    fetch_makehuman()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    fig = Figure()
    FIG_TOE[0] = fig.toe_y
    parts = build(fig)
    for ob in [o for o in bpy.context.scene.objects if o.type == 'MESH' and family(o.name) in pc.SEAMED]:
        pc.set_seams(ob)
    out = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Characters', 'Pike')
    os.makedirs(out, exist_ok=True)
    for ob in sorted([o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.hide_render], key=lambda o: o.name):
        co = gm.object_points(ob)
        lo, hi = co.min(0), co.max(0)
        # nothing of him is further than 45 cm in front of or behind his middle, 70 cm to a side, or above 1.85 m
        if lo[1] < -0.45 or hi[1] > 0.45 or np.abs(co[:, 0]).max() > 0.7 or hi[2] > 1.85 or lo[2] < -0.01 or '--bounds' in ARGS:
            log(f'bounds {ob.name}: {np.round(lo, 3)} .. {np.round(hi, 3)}')
    if '--only' in ARGS:
        # a shape check of some parts alone (families, '+' between them)
        show = set(arg_value('--only', '').split('+'))
        for ob in bpy.context.scene.objects:
            if ob.type == 'MESH' and family(ob.name) not in show:
                ob.hide_render = True
    if '--shape' in ARGS:
        for shot in (arg_value('--shots', 'full')).split('+'):
            if shot == 'back':
                preview(os.path.join(out, 'Pike_clay_back'), views=(('back', 180.0), ('bq', 145.0), ('rq', -145.0)),
                        aim=SHOTS['full'])
                continue
            preview(os.path.join(out, f'Pike_clay_{shot}'), aim=SHOTS[shot])
        return
    import pike_rig as pr
    highs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    lows = make_lows(fig, highs)
    log(f'game mesh: {sum(len(p.data.polygons) for p in lows)} faces before thickness')
    prepare_uvs(lows)
    check = '--stats' in ARGS or '--rig-check' in ARGS        # no bake: triangle counts, or the swing bones' check
    baked = None if check else bake_all(lows, highs, int(arg_value('--size', 2048)))
    for ob in highs:
        me = ob.data
        bpy.data.objects.remove(ob)
        bpy.data.meshes.remove(me)
    for ob in lows:
        ob.name = ob.name[:-4] if ob.name.endswith('_low') else ob.name
    thicken(lows)
    if '--stats' in ARGS:
        tris = {}
        for ob in lows:
            k = family(ob.name)
            tris[k] = tris.get(k, 0) + sum(len(p.vertices) - 2 for p in ob.data.polygons)
        for k, n in sorted(tris.items(), key=lambda kv: -kv[1]):
            log(f'{k:16s} {n:7d}')
        log(f'{"total":16s} {sum(tris.values()):7d}')
        return
    arm, skin = pr.rig(M, fig, lows)
    if '--rig-check' in ARGS:
        pr.rig_check(M, fig, arm, skin)
        return
    if '--audit' in ARGS:
        import pike_audit
        pike_audit.run(M, fig, arm, skin, baked, arg_value('--audit', 'after'))
        return
    if '--preview' in ARGS:
        viewer_materials(skin, baked)
        pose_for_view(arm)
        for shot in (arg_value('--shots', 'full+face')).split('+'):
            views = (('front', 0.0), ('three', 35.0), ('side', 90.0), ('back', 180.0)) if shot != 'face' else \
                (('front', 0.0), ('three', 30.0))
            preview(os.path.join(out, f'Pike_{shot}'), views=views, aim=SHOTS[shot], samples=32)
        for pb in arm.pose.bones:
            pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        log('preview written')
    game_materials(skin)
    face_like_mannequin(arm, skin)
    log('ready to export')


if __name__ == '__main__':
    main()
