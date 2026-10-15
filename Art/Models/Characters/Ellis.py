"""SK_Ellis: Ellis Ransom, the Revenant, the player's own body (Docs/Story.md; "they" in all text), built from the
portrait the user picked on 2026-10-15: option A of Art/Portraits/EllisBlender.py (its OPTIONS["A"] and FACE are the
spec). Boots to hat:
  - the body: MakeHuman's CC0 base (Art/ThirdParty/MakeHuman) shaped as the portrait's androgynous young adult, 1.77 m
    barefoot, standing 2.5 cm up on boot heels; the eyes narrowed to slits, burning cyan in the brim's shadow
  - a cattleman hat worn low (dust-brown felt, a leather band), collar-length dark hair under it, a lock over the left brow
  - a red bandana pulled up over nose and mouth, knotted at the back of the neck, its bib over the chest
  - a dust-brown oilskin duster to mid-calf, worn open, a back vent, a fluted shoulder cape, the collar turned up and
    Saint Ada's sun-ring pin on it (Pa's keeper's badge); over a black waistcoat with brass buttons and a linen shirt
  - tan roper gloves with flared gauntlets and a brass concho (SK_FP_Arms's gloves)
  - a gun belt of brass cartridges with a cross-draw holster on the left hip, the revolver's grip forward; Pa's lantern
    clipped to the belt at the right hip, still burning
  - dark wool trousers tucked into tall stovepipe boots with stacked heels
Rig: the UE5 mannequin's 63 deform bones on MakeHuman's joints (Tools/Blender/looter_human.py), every part weighted from
MakeHuman's own skin weights, so the game's mannequin animations drive it through an IK retargeter.

    blender -b --factory-startup --python Art/Models/Characters/Ellis.py -- [--preview] [--glb <path>] [--no-bake]
"""
import math
import os
import sys
import time

import bpy  # first: as a Python module (the cloud box's bpy) bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
for p in (os.path.join(REPO, 'Tools', 'Blender'), os.path.join(REPO, 'Art', 'Portraits')):
    if p not in sys.path:
        sys.path.insert(0, p)
import EllisBlender as eb  # noqa: E402
import looter_garments as gm  # noqa: E402
import looter_human as hu  # noqa: E402
import looter_bake as bk  # noqa: E402
import looter_looks as lk  # noqa: E402
import looter_model as lm  # noqa: E402
import looter_textures as lt  # noqa: E402
import mh  # noqa: E402

ARGS = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
T0 = time.time()
OPT = eb.OPTIONS['A']
MACRO = {'gender': 0.5, 'muscle': 0.6, 'weight': 0.42, 'caucasian': 0.7, 'african': 0.15, 'asian': 0.15}
HEIGHT = 1.77            # barefoot, the top of the head (m)
LIFT = 0.025             # the boots' heels and soles raise the body this much
HEM = 0.36               # the duster's hem (m above the ground): just below mid-calf
BOOT_TOP = 0.49          # the stovepipe boots' tops
WAIST = 1.02             # the trousers' and the waistcoat's waistline
FWD = np.array([0.0, -1.0])
SIM = '--no-sim' not in ARGS        # the duster and cape settle by cloth simulation (cached in Ellis.drape.json)
DRAPE_FILE = os.path.join(HERE, 'Ellis.drape.json')
SIDE_SEAM = 1.83                     # the duster's side seams, radians round from the front (about 105 degrees)
VENT_TOP = 0.80


def log(message):
    print(f'ELLIS [{time.time() - T0:6.1f}s] {message}', flush=True)


def arg_value(name, default=None):
    return ARGS[ARGS.index(name) + 1] if name in ARGS and ARGS.index(name) + 1 < len(ARGS) else default


# ------------------------------------------------------------------------------------------------------------ the body

class Figure:
    """The shaped body and everything measured from it that the clothes are fitted to."""

    def __init__(self):
        self.b = hu.Body(MACRO, eb.FACE, HEIGHT, LIFT)
        h = self.b.h
        self.balls = eb.close_lids(h, OPT['lids'])      # narrowed eyes: the lids slide over the balls (MakeHuman space)
        self.hf = eb.head_frame(h)
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
        chest = tz[(tz[:, 2] > 1.2) & (tz[:, 2] < 1.4)]
        self.centre = np.array([0.0, (chest[:, 1].min() + chest[:, 1].max()) * 0.5])
        self.torso_idx = torso
        g = self.head_point
        self.eyes = [g(e) for e in self.hf['eyes']]
        self.eye_z = (self.eyes[0][2] + self.eyes[1][2]) * 0.5
        log(f'body: {len(used)} vertices, scale {self.b.scale:.3f}, eyes at {self.eye_z:.3f} m, chest centre y '
            f'{self.centre[1]:.3f}')

    def head_point(self, p):
        return self.b.to_world(np.asarray(p, dtype=np.float64)[None])[0]

    def face_mask(self, vert_mask):
        """Faces of the body whose every vertex is in vert_mask (bool per MakeHuman vertex)."""
        return [bool(vert_mask[f].all()) for f in self.faces]

    def weights_for(self, points, faces=None):
        names, W = self.b.weights_at(points, faces)
        return W

    def to_portrait(self, ob):
        """Moves an object EllisBlender built (MakeHuman metres, no size or lift) into this figure's space."""
        s = self.b.scale
        ob.matrix_world = Matrix.Translation((0.0, 0.0, -s * 0.1 * self.b.floor + LIFT)) @ Matrix.Scale(s, 4) @ ob.matrix_world


# ------------------------------------------------------------------------------------------------------------ looks

SLOTS = {}
HAT = {}                             # the hat band's height, for the felt's sweat stain


def slot(key):
    """The look (looter_looks) for a part of the costume, one material per look; the bake gathers them into the
    texture sets of TEXTURE_SETS."""
    if key not in SLOTS:
        SLOTS[key] = LOOKS[key](f'Ellis_{key}')
        SLOTS[key]['LookKey'] = key
    return SLOTS[key]


LOOKS = {
    'Skin': lambda n: lk.skin(n, OPT['skin']),
    'Scalp': lambda n: lk.scalp(n, (0.07, 0.06, 0.055)),
    'EyeGlow': lambda n: lk.glow(n, OPT['glow'], 6.0),
    'Felt': lambda n: lk.felt(n, (0.11, 0.095, 0.085), (0.30, 0.255, 0.21), band_z=HAT.get('band_z')),
    'HatBand': lambda n: lk.leather(n, (0.14, 0.08, 0.05), dust_amt=0.2, dust_height=0.001),
    'Hair': lambda n: lk.hair(n, (0.035, 0.028, 0.025), (0.13, 0.10, 0.085)),
    'Bandana': lambda n: lk.bandana(n, (0.52, 0.06, 0.05), (0.66, 0.17, 0.12)),
    'Duster': lambda n: lk.oilskin(n, OPT['coat']['cloth'][0], OPT['coat']['cloth'][1], hem=HEM),
    'Vest': lambda n: lk.brocade(n, (0.045, 0.045, 0.05), (0.12, 0.115, 0.115)),
    'Shirt': lambda n: lk.linen(n, (0.31, 0.33, 0.36)),
    'Trousers': lambda n: lk.wool(n, (0.19, 0.18, 0.19)),
    'Boots': lambda n: lk.leather(n, (0.20, 0.12, 0.075), grain=900.0, dust_amt=0.3, dust_height=0.10, mud=0.0,
                                  boot=(-0.205, 0.0215)),
    'Soles': lambda n: lk.stacked(n, (0.12, 0.08, 0.055)),
    'Gloves': lambda n: lk.leather(n, (0.50, 0.36, 0.23), grain=1100.0, dust_amt=0.25, dust_height=0.001,
                                   scuff=(0.68, 0.53, 0.38), rough=(0.5, 0.75)),
    'Belt': lambda n: lk.leather(n, (0.25, 0.145, 0.085), dust_amt=0.25, dust_height=0.001),
    'Brass': lambda n: lk.metal(n, (0.58, 0.42, 0.19), (0.86, 0.68, 0.36)),
    'Lead': lambda n: lk.metal(n, (0.38, 0.37, 0.36), (0.58, 0.56, 0.54), rough=(0.4, 0.6), tarnish=(0.2, 0.2, 0.2)),
    'Iron': lambda n: lk.metal(n, (0.05, 0.055, 0.065), (0.17, 0.18, 0.20), rough=(0.3, 0.55), tarnish=(0.05, 0.04, 0.03)),
    'Walnut': lambda n: lk.walnut(n),
    # The holstered revolver's own looks (the same iron and walnut): its own game slot, which the game hides while
    # the revolver is in hand (the user, 2026-10-15: "empty only while drawn").
    'GunIron': lambda n: lk.metal(n, (0.05, 0.055, 0.065), (0.17, 0.18, 0.20), rough=(0.3, 0.55), tarnish=(0.05, 0.04, 0.03)),
    'GunWalnut': lambda n: lk.walnut(n),
    'LanternGlow': lambda n: lk.glow(n, (1.0, 0.62, 0.25), 8.0),
}

# The texture sets the looks bake into (Art/Textures/<Set>/), and the glowing looks the game draws as Glow materials.
TEXTURE_SETS = {
    'EllisSkin': ('Skin', 'Scalp'),
    'EllisDuster': ('Duster',),
    'EllisGarb': ('Felt', 'HatBand', 'Hair', 'Bandana', 'Vest', 'Shirt', 'Trousers'),
    'EllisGear': ('Boots', 'Soles', 'Gloves', 'Belt', 'Brass', 'Lead', 'Iron', 'Walnut', 'GunIron', 'GunWalnut'),
}
GLOWS = {'EyeGlow': (0x5AD8FF, 4.0), 'LanternGlow': (0xFFA040, 3.0)}

# The game's material slots: each kind of surface on the master made for it (Tools/Unreal/build_world_materials.py's
# characters), several slots sharing one baked set (the user found him "plastic" on M_World alone, 2026-10-15). Slot:
# (master, the looks on it, the master's parameters). Cloth's sheen is FuzzStrength (tuned in game: the oilskin waxed
# and dull, the garb's wool, linen and felt softer).
GAME_MATERIALS = {
    'EllisSkin': ('CharacterSkin', ('Skin', 'Scalp'), {}),
    'EllisDuster': ('CharacterCloth', ('Duster',), {'FuzzStrength': 0.2, 'FuzzLift': 0.08}),
    'EllisGarbCloth': ('CharacterCloth', ('Felt', 'Bandana', 'Vest', 'Shirt', 'Trousers'), {'FuzzStrength': 0.3, 'FuzzLift': 0.12}),
    'EllisGarbHair': ('Character', ('Hair', 'HatBand'), {'Specular': 0.3}),
    'EllisGear': ('Character', ('Boots', 'Soles', 'Gloves', 'Belt', 'Brass', 'Lead', 'Iron', 'Walnut'), {}),
    'EllisHolsterGun': ('Character', ('GunIron', 'GunWalnut'), {}),
}


# --------------------------------------------------------------------------------------------------------- the skin

def build_skin(fig):
    """The visible skin: the head and the neck down under the collar (the clothes cover the rest, and the gloves are
    their own shell of the hands)."""
    keep = fig.w_head > 0.45
    ob = fig.b.mesh('Ellis', keep=lambda i, f: bool(keep[f].all()))
    ob.data.materials.append(slot('Skin'))
    ob.data.materials.append(slot('Scalp'))
    # the scalp under the hair takes the hair's colour, so nothing bald shows between the clumps
    hv = fig.b.h['verts']
    hf = fig.hf
    cz = (hf['verts'][:, 2].max() + hf['verts'][:, 2].min()) * 0.5 - 0.1
    az = np.degrees(np.abs(np.arctan2(hv[:, 0], hv[:, 2] - cz)))
    line = np.interp(az, [0, 46, 70, 100, 125, 180], [0.44, 0.42, 0.20, 0.0, -0.85, -0.95]) + hf['eye_y']
    scalp = hv[:, 1] > line
    idx = np.array([d.value for d in ob.data.attributes['mh_index'].data])
    for p in ob.data.polygons:
        if scalp[idx[list(p.vertices)]].all():
            p.material_index = 1
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


# ------------------------------------------------------------------------------------------- trousers, boots, gloves

def build_trousers(fig):
    """Dark wool trousers: the hips and legs pushed out and smoothed, from the waist to inside the boots, bloused a
    little over the boot tops."""
    z = fig.wv[:, 2]
    keep = (z < WAIST + 0.02) & (z > BOOT_TOP - 0.07) & (fig.w_arm < 0.2) & (fig.w_hand < 0.1)
    loose = gm.smoothstep(0.95, 0.55, z)                     # looser down the leg
    blouse = gm.smoothstep(BOOT_TOP + 0.01, BOOT_TOP + 0.05, z) * gm.smoothstep(BOOT_TOP + 0.16, BOOT_TOP + 0.06, z)
    inside = gm.smoothstep(BOOT_TOP + 0.01, BOOT_TOP - 0.02, z)    # tucked in: tighter than the boot's shaft
    off = (0.009 + 0.010 * loose + 0.007 * blouse) * (1.0 - inside) + 0.010 * inside
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), off, iters=14)
    # creases: bunched rings behind the knee and above the boot tops, a few drag lines down from the seat
    v = piece.verts
    nrm = gm.vertex_normals(v, piece.faces)
    for s_ in 'lr':
        knee = fig.sk[f'calf_{s_}'][0]
        side = (v[:, 0] * knee[0]) > 0
        dz = v[:, 2] - knee[2]
        behind = gm.smoothstep(knee[1] - 0.02, knee[1] + 0.05, v[:, 1])
        ring = np.sin(dz * 160.0 + v[:, 0] * 40.0) * np.exp(-(dz / 0.06) ** 2) * (0.35 + 0.65 * behind)
        tuck = np.sin(v[:, 2] * 210.0 + np.arctan2(v[:, 1] - knee[1], v[:, 0] - knee[0]) * 3.0) * \
            gm.smoothstep(BOOT_TOP + 0.13, BOOT_TOP + 0.03, v[:, 2]) * gm.smoothstep(BOOT_TOP - 0.01, BOOT_TOP + 0.03, v[:, 2])
        v[side] += nrm[side] * (0.004 * ring[side] + 0.004 * tuck[side])[:, None]
    piece.verts = v
    ob = gm.mesh('Trousers', piece, slot('Trousers'))
    return ob


def insole(s):
    """The height of the boot's insole along the foot (s, m ahead of the ankle): on the heel 3.6 cm up, sloping down
    over the arch to 1.4 cm under the ball and the toes, the toe tip sprung up a little."""
    return np.interp(s, [-0.2, 0.03, 0.10, 0.17, 0.25], [0.036, 0.036, 0.014, 0.014, 0.019])


def build_boots(fig):
    """Tall stovepipe boots: the calves and feet pushed out, the toes made one rounded toe box reaching past them, the
    bottom flattened onto a stacked leather sole with a cuban heel, the tops cut level with a dip front and back."""
    z = fig.wv[:, 2]
    top_at = np.full(len(z), BOOT_TOP)
    for s in 'lr':
        axis = fig.sk[f'calf_{s}'][1][:2]
        side = (fig.wv[:, 0] * axis[0]) > 0
        ang = np.arctan2(fig.wv[side, 0] - axis[0], -(fig.wv[side, 1] - axis[1]))
        top_at[side] = BOOT_TOP - 0.03 * np.cos(ang) ** 2
    keep = z < top_at + 0.012
    off = 0.008 + 0.012 * gm.smoothstep(0.22, BOOT_TOP, z)
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), off, iters=3)
    top_at = top_at[used]
    v = piece.verts
    soles = []
    for s in 'lr':
        ankle, ball = fig.sk[f'foot_{s}'][0], fig.sk[f'ball_{s}'][0]
        f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
        lat = np.array([-f[1], f[0], 0.0])
        side = (v[:, 0] * ball[0]) > 0
        # the toes into one toe box: the forefoot's points moved onto a rounded box (a superellipsoid in the foot's
        # frame) reaching 2.5 cm past the toes, blended in from just behind the ball
        c = ball + f * 0.022
        c[2] = LIFT + 0.026
        d = v - c
        along = d @ f
        sel = side & (along > -0.08) & (v[:, 2] < 0.13)
        dx, dy, dz = d[sel] @ lat, along[sel], d[sel, 2]
        ay = np.where(dy > 0, 0.056, 0.07)
        az = np.where(dz > 0, 0.034, c[2])
        p = 2.6
        F = (np.abs(dx / 0.049) ** p + np.abs(dy / ay) ** p + np.abs(dz / az) ** p) ** (1.0 / p)
        k = gm.smoothstep(-0.075, 0.0, dy)[:, None]          # a long blend: a short one left a ridge round the box
        toe_w = np.zeros(len(v))
        toe_w[sel] = gm.smoothstep(-0.08, 0.0, dy) * 0.8
        # project, relax, project again: the toes' crowded points spread evenly over the box
        for it in range(8):
            d = v[sel] - c
            dx, dy2, dz = d @ lat, d @ f, d[:, 2]
            ay = np.where(dy2 > 0, 0.056, 0.07)
            az = np.where(dz > 0, 0.034, c[2])
            F = (np.abs(dx / 0.049) ** p + np.abs(dy2 / ay) ** p + np.abs(dz / az) ** p) ** (1.0 / p)
            target = c + d / np.maximum(F, 1e-6)[:, None]
            v[sel] = v[sel] * (1.0 - k) + target * k
            if it < 7:
                v = gm.laplacian(v, piece.faces, 3, weight=toe_w)
        # down onto the insole: the forefoot lowered, the heel raised onto its stack, the bottom flattened
        sft = (v - ankle) @ f
        low = side & (v[:, 2] < 0.15)
        w = 1.0 - gm.smoothstep(0.03, 0.15, v[low, 2])
        v[low, 2] += (insole(sft[low]) - (LIFT - 0.008)) * w
        under = side & (v[:, 2] < insole(sft) + 0.003)
        v[under, 2] = insole(sft[under])
        # the sole's outline: the boot's footprint, round the foot's middle, pushed out into a welt
        foot_pts = v[side & (v[:, 2] < insole(sft) + 0.012)]
        mid = foot_pts[:, :2].mean(axis=0)
        ang = np.linspace(-math.pi, math.pi, 72, endpoint=False)
        rel = foot_pts[:, :2] - mid
        a = np.arctan2(rel[:, 1], rel[:, 0])
        r = np.linalg.norm(rel, axis=1)
        out = np.zeros(len(ang))
        for i, t in enumerate(ang):
            dd = np.abs((a - t + math.pi) % (2 * math.pi) - math.pi)
            near = dd < math.radians(9)
            out[i] = r[near].max() if near.any() else 0.0
        for _ in range(3):
            out = 0.25 * np.roll(out, 1) + 0.5 * out + 0.25 * np.roll(out, -1)
        out += 0.005
        ring = np.stack([mid[0] + out * np.cos(ang), mid[1] + out * np.sin(ang)], 1)
        s_ring = (np.c_[ring, np.zeros(len(ring))] - ankle) @ f
        top = insole(s_ring) + 0.002
        heel = s_ring < 0.035
        arch = gm.smoothstep(0.03, 0.045, s_ring) * gm.smoothstep(0.11, 0.085, s_ring)
        bottom = 0.011 * arch
        # the cuban heel tapers: its bottom 4 mm in from its top, its breast slanted
        inward = np.where(heel, 0.004, 0.0)
        rel_r = ring - mid
        ring_b = ring - gm.unit(np.c_[rel_r, np.zeros(len(rel_r))])[:, :2] * inward[:, None]
        X = np.stack([ring[:, 0], ring[:, 0], ring_b[:, 0]])
        Y = np.stack([ring[:, 1], ring[:, 1], ring_b[:, 1]])
        Z = np.stack([top, top - 0.006, bottom])
        sole = gm.grid(X, Y, Z, wrap=True)
        verts = list(sole.verts)
        faces, uv = sole.faces, sole.uv
        n = len(ang)
        for row, zc in ((0, top.mean()), (2, 0.0)):
            cidx = len(verts)
            cz = zc if row == 0 else 0.0
            verts.append(np.array([mid[0], mid[1], cz]))
            for i in range(n):
                a_, b_ = row * n + i, row * n + (i + 1) % n
                faces.append([a_, b_, cidx] if row == 0 else [b_, a_, cidx])
                uv += [(0.0, 0.0), (0.01, 0.0), (0.005, 0.01)]
        soles.append(gm.Piece(np.array(verts), faces, uv))
    # creases across the instep and round the ankle where the leather folds as the foot bends
    nrm = gm.vertex_normals(v, piece.faces)
    for s_ in 'lr':
        ankle = fig.sk[f'foot_{s_}'][0]
        side = (v[:, 0] * ankle[0]) > 0
        front = gm.smoothstep(ankle[1] + 0.01, ankle[1] - 0.04, v[:, 1])
        band = gm.smoothstep(0.06, 0.09, v[:, 2]) * gm.smoothstep(0.20, 0.14, v[:, 2])
        crease = -np.abs(np.sin((v[:, 2] - 0.08) * 150.0 + v[:, 0] * 30.0)) ** 3
        v[side] += nrm[side] * (0.003 * crease * band * (0.4 + 0.6 * front))[side][:, None]
    piece.verts = v
    # the shaft's top: level, dipping 3 cm at the front and back (stovepipes)
    edge = gm.boundary(piece.faces, len(v))
    topv = edge & (v[:, 2] > BOOT_TOP - 0.07)
    v[topv, 2] = top_at[topv]
    piece.verts = v
    ob = gm.mesh('Boots', piece, slot('Boots'))
    sole_ob = gm.mesh('Soles', soles[0] + soles[1], slot('Soles'))
    return [ob, sole_ob]


def build_gloves(fig, sleeves):
    """Tan roper gloves: the hands' own shell, and a flared gauntlet cuff over each sleeve end with a brass concho."""
    keep = fig.w_hand > 0.45
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), 0.0022, iters=1)
    glove = gm.mesh('Gloves', piece, slot('Gloves'))
    tree = gm.bvh_objects([sleeves])
    parts = [glove]
    for s in 'lr':
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        axis = gm.unit(w - e)
        path = np.array([w + axis * (0.012 - 0.115 * t) for t in np.linspace(0, 1, 9)])
        rad = np.array([[0.037 + 0.030 * t ** 1.3, 0.031 + 0.026 * t ** 1.3] for t in np.linspace(0, 1, 9)])
        pc = gm.tube(path, rad, n=28, up=(0.0, -1.0, 0.0))
        pc.verts = gm.clear(pc.verts, tree, 0.004, 0.06)
        cuff = gm.mesh(f'Gauntlet_{s}', pc, slot('Gloves'))
        parts.append(cuff)
        # the concho on the outside of the cuff
        out = gm.unit(np.array([1.0 if s == 'l' else -1.0, 0.0, 0.25]))
        out = gm.unit(out - axis * (out @ axis))
        mid = w - axis * 0.075
        hit = tree.ray_cast(Vector(mid + out * 0.2), Vector(-out), 0.4)
        at = (np.array(hit[0]) if hit[0] is not None else mid + out * 0.05) + out * 0.012
        bm = bmesh.new()
        bmesh.ops.create_cone(bm, cap_ends=True, segments=20, radius1=0.011, radius2=0.008, depth=0.004)
        me = bpy.data.meshes.new(f'Concho_{s}')
        bm.to_mesh(me)
        bm.free()
        me.shade_smooth()
        co = bpy.data.objects.new(f'Concho_{s}', me)
        bpy.context.scene.collection.objects.link(co)
        co.matrix_world = Matrix.Translation(Vector(at)) @ Vector(out).to_track_quat('Z', 'Y').to_matrix().to_4x4()
        co.data.materials.append(slot('Brass'))
        parts.append(co)
    return parts


# ---------------------------------------------------------------------------------------- shirt, waistcoat, sleeves

def build_vest(fig):
    """A black waistcoat (and the linen shirt in its V): the torso pushed out a little, cut to a V at the front."""
    z = fig.wv[:, 2]
    x = fig.wv[:, 0]
    front = fig.wv[:, 1] < fig.centre[1] - 0.02
    torso = (fig.w_arm < 0.25) & (fig.w_hand < 0.05) & (fig.w_head < 0.25)
    hw = 0.065 * gm.smoothstep(1.27, 1.47, z)
    vee = front & (np.abs(x) < hw)
    keep = torso & (z > WAIST - 0.04) & (z < fig.neck[2] - 0.005) & ~vee
    vest, _ = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), 0.0075, iters=6)
    # a clean hem: level round the back, two points at the front either side of the buttons
    vv = vest.verts
    hem = gm.boundary(vest.faces, len(vv)) & (vv[:, 2] < WAIST + 0.03)
    fr = vv[:, 1] < fig.centre[1] - 0.02
    vv[hem, 2] = WAIST - 0.03 - 0.05 * fr[hem] * np.exp(-((np.abs(vv[hem, 0]) - 0.05) / 0.04) ** 2)
    vest.verts = gm.laplacian(vv, vest.faces, 2, weight=(vv[:, 2] < WAIST + 0.06) * 0.5)
    ob = gm.mesh('Vest', vest, slot('Vest'))
    shirt_keep = torso & front & (np.abs(x) < 0.12) & (z > 1.18) & (z < fig.neck[2] + 0.01)
    shirt, _ = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(shirt_keep), 0.003, iters=3)
    sh = gm.mesh('Shirt', shirt, slot('Shirt'))
    # brass buttons down the waistcoat's front edge
    tree = gm.bvh_objects([ob])
    buttons = []
    for k in range(5):
        zz = 1.255 - k * 0.048
        hit = tree.ray_cast(Vector((0.012, -0.5, zz)), Vector((0, 1, 0)), 1.0)
        if hit[0] is None:
            continue
        bm = bmesh.new()
        bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=0.0058)
        me = bpy.data.meshes.new('VestButton')
        bm.to_mesh(me)
        bm.free()
        me.shade_smooth()
        bo = bpy.data.objects.new('VestButton', me)
        bpy.context.scene.collection.objects.link(bo)
        bo.location = hit[0] + hit[1] * 0.003
        bo.scale = (1.0, 1.0, 0.55)
        bo.rotation_euler = hit[1].to_track_quat('Z', 'Y').to_euler()
        bo.data.materials.append(slot('Brass'))
        buttons.append(bo)
    return ob, sh, buttons


def build_sleeves(fig, under):
    """The duster's sleeves: the arms pushed out loose (looser toward the cuff) and smoothed, folded at the elbow and
    down the forearm, ending at the wrist under the gauntlet."""
    z = fig.wv[:, 2]
    keep = (fig.w_arm > 0.45) & (fig.w_hand < 0.25)
    for s, sx in (('l', 1.0), ('r', -1.0)):
        w = fig.sk[f'lowerarm_{s}'][1]
        e = fig.sk[f'lowerarm_{s}'][0]
        axis = gm.unit(w - e)
        beyond = ((fig.wv - w) @ axis) > -0.012
        keep &= ~((fig.wv[:, 0] * sx > 0) & beyond)
    sh = fig.sk['upperarm_l'][0]
    along = np.zeros(len(fig.wv))
    for s, sx in (('l', 1.0), ('r', -1.0)):
        a, w = fig.sk[f'upperarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        t = np.clip(((fig.wv - a) @ gm.unit(w - a)) / np.linalg.norm(w - a), 0, 1)
        along = np.where(fig.wv[:, 0] * sx > 0, t, along)
    off = 0.020 + 0.016 * along
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), off, iters=18)
    # folds: rings bunched above the cuff, crescents in the crook of the elbow
    v = piece.verts
    nrm = gm.vertex_normals(v, piece.faces)
    for s, sx in (('l', 1.0), ('r', -1.0)):
        a, el, w = fig.sk[f'upperarm_{s}'][0], fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        side = v[:, 0] * sx > 0
        d = gm.unit(w - el)
        t = (v - el) @ d
        ring = np.cos(t / 0.055 * 2 * math.pi + 0.8 * np.sin(np.arctan2(v[:, 2] - el[2], v[:, 1] - el[1]) * 2)) * 0.5 + 0.5
        bunch = gm.smoothstep(0.10, 0.2, t) * 0.010 * ring ** 2
        crook = np.exp(-(t / 0.06) ** 2) * 0.007 * np.sin(t * 90)
        v[side] += nrm[side] * (bunch[side] + crook[side])[:, None]
    piece.verts = v
    tree = gm.bvh_objects(under)
    piece.verts = gm.clear(piece.verts, tree, 0.012, 0.1)
    return gm.mesh('Sleeves', piece, slot('Duster'))


# ----------------------------------------------------------------------------------------------------- the duster

def opening(z, side):
    """Half the gap between the duster's fronts (m, across) at height z: a V under the collar, open wide at the hips
    (the holster and the lantern show), hanging open below. side +1 the figure's left, -1 its right."""
    v = 0.05 + (1.52 - z) * 0.19
    hips = 0.17 + (0.02 if side < 0 else 0.0)
    return np.where(z > 1.06, v, np.where(z > 0.86, hips - (z - 0.86) / 0.2 * (hips - (0.05 + 0.46 * 0.19)), hips))


def build_duster(fig, under_pts, clear_obs=()):
    """The coat's body: draped round the torso from under the collar to the hem, open at the front, a vent up the
    back, flared and fluted toward the hem."""
    rng = np.random.default_rng(21)
    top = fig.neck[2] - 0.012
    n_z, n_t = 46, 104
    fine = np.linspace(-math.pi, math.pi, 720, endpoint=False)
    hip = 0.95
    # simulated, the skirt starts fuller than it hangs: the spare cloth is what falls into folds
    flare_amt = 0.10 if SIM else 0.07
    flare = lambda th, v: flare_amt * gm.smoothstep(0.42, 1.0, v) ** 1.3 * (0.55 + 0.45 * np.abs(np.sin(th)))
    X, Y, Z, R = gm.drape(under_pts, fig.centre, FWD, fine, lambda th: top, lambda th: HEM, n_z, 0.016,
                          hb=math.radians(7.0), zb=0.01, smooth=(10, 3), wrap=True, flare=flare)
    zs = Z[:, 0]
    # columns per row from the left front edge round the back to the right front edge, one extra at the back for the
    # vent's two edges
    rows = []
    vent_top = VENT_TOP
    ph = rng.random(8) * 6.28
    for j, z in enumerate(zs):
        r_front = np.interp(0.0, fine, R[j], period=2 * math.pi)
        a_l = math.asin(min(0.98, float(opening(z, 1.0)) / max(r_front, 0.05)))
        a_r = math.asin(min(0.98, float(opening(z, -1.0)) / max(r_front, 0.05)))
        half = n_t // 2
        left = np.linspace(a_l, math.pi, half)
        right = np.linspace(-math.pi, -a_r, n_t - half)
        th = np.concatenate([left, right])
        r = np.interp(th, fine, R[j], period=2 * math.pi)
        v = (top - z) / (top - HEM)
        below = gm.smoothstep(hip + 0.05, HEM, z)
        # broad, irregular folds (frequencies that never line up), deepening toward the hem
        fold = (0.010 if SIM else 0.026) * below ** 1.3 * (0.45 * np.sin(th * 4.0 + ph[0] + 0.7 * np.sin(v * 2.0 + ph[1]))
                                       + 0.35 * np.sin(th * 7.3 + ph[2] + 0.9 * np.sin(v * 2.6 + ph[3]))
                                       + 0.20 * np.sin(th * 12.7 + ph[4] + v * 1.5))
        fold += 0.005 * gm.smoothstep(1.2, 1.0, z) * gm.smoothstep(0.8, 1.0, z) * np.sin(th * 9.1 + ph[5])
        r = r + fold
        # the vent: built, its edges overlap a little (the left flap over the right); simulated, they start apart
        # (cloth that starts through itself throws its points away under self-collision) and fall together
        if z < vent_top:
            k = gm.smoothstep(vent_top, vent_top - 0.08, z)
            if SIM:
                th[half - 1] -= 0.03 * k
                th[half] += 0.03 * k
            else:
                r[half - 1] += 0.004 * k
                th[half - 1] += 0.025 * k
                th[half] -= 0.02 * k
        rows.append((th, r, z))
    TH = np.array([r[0] for r in rows])
    RR = np.array([r[1] for r in rows])
    ZZ = np.array([[r[2]] * n_t for r in rows])
    # the hem: a little uneven, lower at the back
    ZZ[-1] += (0.012 * np.sin(TH[-1] * 7 + 1.0) - 0.015 * np.cos(TH[-1]) ** 2 * (np.cos(TH[-1]) < 0))
    dx, dy = gm.radial(TH, FWD)
    X = fig.centre[0] + RR * dx
    Y = fig.centre[1] + RR * dy
    if clear_obs:
        P = gm.clear(np.stack([X, Y, ZZ], -1).reshape(-1, 3), gm.bvh_objects(clear_obs), 0.008, 0.05).reshape(X.shape + (3,))
        X, Y, ZZ = P[..., 0], P[..., 1], P[..., 2]
    piece = gm.grid(X, Y, ZZ)
    # weld the vent's two columns above its top
    ob = gm.mesh('Duster', piece, slot('Duster'))
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    bm.verts.ensure_lookup_table()
    half = n_t // 2
    pairs = [(bm.verts[j * n_t + half - 1], bm.verts[j * n_t + half]) for j in range(n_z) if zs[j] >= vent_top]
    for a, b in pairs:
        mid = (a.co + b.co) * 0.5
        a.co = b.co = mid
    bmesh.ops.remove_doubles(bm, verts=[v for p in pairs for v in p], dist=1e-5)
    bm.to_mesh(ob.data)
    bm.free()
    # the sewn seams: up the middle of the back above the vent, and down each side
    co = gm._object_coords(ob)
    rel = co[:, :2] - fig.centre
    f, l = gm.frame2d(FWD)
    ang = np.arctan2(rel @ l, rel @ f)
    rad = np.linalg.norm(rel, axis=1)
    wrap = lambda a: np.abs((a + math.pi) % (2 * math.pi) - math.pi)
    back = np.where(co[:, 2] > vent_top - 0.01, wrap(ang - math.pi) * rad, 1.0)
    sides = np.minimum(wrap(ang - SIDE_SEAM), wrap(ang + SIDE_SEAM)) * rad
    set_seams(ob, np.minimum(back, sides))
    return ob


def set_seams(ob, seam_dist=None):
    """The 'Seams' colour attribute the looks sew from (looter_looks.construction): R how close each vertex is to
    an open edge, G to a sewn seam (seam_dist, metres per vertex), both 1 there and 0 from 10 cm."""
    from mathutils import kdtree
    me = ob.data
    co = gm._object_coords(ob)
    count = {}
    for p in me.polygons:
        vs = list(p.vertices)
        for k in range(len(vs)):
            key = (min(vs[k], vs[(k + 1) % len(vs)]), max(vs[k], vs[(k + 1) % len(vs)]))
            count[key] = count.get(key, 0) + 1
    rim = sorted({v for key, c in count.items() if c == 1 for v in key})
    d_edge = np.full(len(co), 1.0)
    if rim:
        # the edge as a dense chain of points, so the distance is to the edge itself, not its vertices
        pts = []
        for (a, b), c in count.items():
            if c == 1:
                for t in np.linspace(0.0, 1.0, 6):
                    pts.append(co[a] * (1 - t) + co[b] * t)
        tree = kdtree.KDTree(len(pts))
        for i, q in enumerate(pts):
            tree.insert(q, i)
        tree.balance()
        d_edge = np.array([tree.find(q)[2] for q in co])
    col = np.ones((len(co), 4), dtype=np.float32)
    col[:, 0] = 1.0 - np.clip(d_edge / 0.1, 0.0, 1.0)
    col[:, 1] = 0.0 if seam_dist is None else 1.0 - np.clip(np.asarray(seam_dist) / 0.1, 0.0, 1.0)
    col[:, 2] = 0.0
    attr = me.color_attributes.get('Seams') or me.color_attributes.new('Seams', 'FLOAT_COLOR', 'POINT')
    attr.data.foreach_set('color', col.ravel())


def build_cape(fig, under_pts):
    """The shoulder cape: a bell hung from the collar over the shoulders and the tops of the arms, open at the front,
    flaring in flutes."""
    neck = fig.neck
    sz = fig.shoulder_z
    th = np.linspace(math.radians(26), math.radians(334), 96)
    top = lambda t: neck[2] - 0.006
    bot = lambda t: sz - 0.13 - 0.035 * abs(math.sin(t)) + 0.012 * math.sin(t * 7.0)
    X, Y, Z, R = gm.drape(under_pts, neck[:2], FWD, th, top, bot, 24, 0.026, hb=math.radians(7), zb=0.012, smooth=(4, 3))
    v = np.linspace(0, 1, Z.shape[0])[:, None]
    # flutes of uneven depth and spacing (frequencies that never line up), deepening toward the hem
    rng = np.random.default_rng(5)
    ph = rng.random(6) * 6.28
    T = th[None, :]
    flute = 0.024 * v ** 1.4 + v ** 1.1 * (0.011 * np.sin(T * 7.0 + ph[0] + 0.8 * np.sin(v * 3 + ph[1]))
                                           + 0.008 * np.sin(T * 11.3 + ph[2] + 0.6 * np.sin(v * 2 + ph[3]))
                                           + 0.004 * np.sin(T * 19.7 + ph[4] + v * 4)) \
        + 0.004 * v * np.sin(T * 31.0 + ph[5] + v * 6)
    dx, dy = gm.radial(th[None, :], FWD)
    piece = gm.grid(X + flute * dx, Y + flute * dy, Z)
    return gm.mesh('Cape', piece, slot('Duster')), R[0].max()


def build_collar(fig, top_r):
    """The duster's collar turned up high round the jaw, highest at the back, open at the throat."""
    neck = fig.neck
    th = np.linspace(math.radians(28), math.radians(332), 84)
    rows = 8
    cv = []
    for j in range(rows):
        t = j / (rows - 1)
        for a in th:
            back = 0.5 - 0.5 * math.cos(a)
            # the ends by the throat stand a little taller and splay out into points; the top edge rolls outward
            end = float(gm.smoothstep(math.radians(62), math.radians(30), min(a, 2 * math.pi - a)))
            # soft, worn oilskin: the top edge wavers, the stand ripples a little, a crease where it turns up
            hgt = 0.062 + 0.058 * back + 0.012 * end + 0.004 * math.sin(a * 5.3 + 1.1) + 0.003 * math.sin(a * 13.7)
            r = (top_r * (0.89 + 0.08 * t + 0.13 * t ** 3) + 0.004 * math.sin(a * 9 + t * 2) + 0.0025 * math.sin(a * 17.3 + t * 4)
                 - 0.010 * (1 - back) * t + 0.016 * end * t ** 2 - 0.004 * math.exp(-((t - 0.18) / 0.08) ** 2))
            d = math.cos(a) * np.array([0.0, -1.0]) + math.sin(a) * np.array([1.0, 0.0])
            cv.append([neck[0] + d[0] * r, neck[1] + d[1] * r, neck[2] - 0.03 + hgt * t])
    cv = np.array(cv).reshape(rows, -1, 3)[::-1]          # rows from the top down, so the faces look outward
    piece = gm.grid(cv[..., 0], cv[..., 1], cv[..., 2])
    ob = gm.mesh('Collar', piece, slot('Duster'))
    # Saint Ada's sun-ring on the collar's left front
    a = math.radians(42)
    d = np.array([math.sin(a), -math.cos(a)])
    r = top_r * 0.99 + 0.008
    pin = eb.sun_pin(Vector((neck[0] + d[0] * r, neck[1] + d[1] * r, neck[2] + 0.018)),
                     Vector((d[0], d[1], 0.15)).normalized())
    pin.data.materials.clear()
    pin.data.materials.append(slot('Brass'))
    return ob, pin


# ----------------------------------------------------------------------------------------- hat, hair and bandana

def build_hat(fig):
    """The portrait's cattleman (EllisBlender.build_hat: tapered creased crown, brim curled at the sides, a leather
    band), worn low, moved into the figure's space and given its thickness."""
    parts = eb.build_hat(fig.b.h, fig.hf, OPT)
    for ob in parts:
        for m in list(ob.modifiers):
            if m.type in ('SUBSURF', 'SOLIDIFY'):
                ob.modifiers.remove(m)
        fig.to_portrait(ob)
        ob.data.transform(ob.matrix_world)
        ob.matrix_world = Matrix.Identity(4)
    band = next(o for o in parts if o.name.startswith('HatBand'))
    HAT['band_z'] = float(gm._object_coords(band)[:, 2].min())
    for ob in parts:
        ob.data.materials.clear()
        ob.data.materials.append(slot('HatBand' if ob.name.startswith('HatBand') else 'Felt'))
        uv_cylinder(ob)
    return parts


def uv_cylinder(ob):
    """A plain cylindrical UV map round the object's vertical axis (u round, v up, in metres), for pieces that come
    without one; the bake repacks every island anyway."""
    me = ob.data
    if me.uv_layers:
        return
    layer = me.uv_layers.new(name='UVMap')
    co = np.array([v.co[:] for v in me.vertices])
    c = co.mean(axis=0)
    uv = []
    for p in me.polygons:
        ids = list(p.vertices)
        ang = np.arctan2(co[ids, 0] - c[0], -(co[ids, 1] - c[1]))
        ang = np.unwrap(ang)
        r = np.linalg.norm(co[ids, :2] - c[:2], axis=1).mean()
        for a, i in zip(ang, ids):
            uv.append((a * max(r, 0.02), co[i, 2] + np.linalg.norm(co[i, :2] - c[:2]) * (1.0 if co[i, 2] < c[2] else 0.0)))
    layer.data.foreach_set('uv', np.array(uv, dtype=np.float32).ravel())


def build_bandana(fig, under_obs):
    """The red bandana pulled up over nose and mouth: draped from the bridge of the nose round the head, its bib
    tented off the nose over the chest, a band round the back of the neck inside the collar and a knot there."""
    pts = np.concatenate([gm.object_points(o) for o in under_obs])
    s = fig.b.scale
    mid = (fig.eyes[0] + fig.eyes[1]) * 0.5
    centre = mid[:2] - FWD * 0.085 * s
    bridge = fig.eye_z - 0.022 * s
    span = math.radians(108)
    th = np.linspace(-math.pi, math.pi, 104, endpoint=False)

    def top(t):
        a = abs(t)
        if a < span:
            return bridge - 0.055 * (a / span) ** 2
        return bridge - 0.055 - (a - span) / (math.pi - span) * 0.05

    def bot(t):
        # a triangle's point down the chest, the sides wrapping the jaw and neck under the collar
        a = abs(t)
        if a < span:
            return top(t) - (0.095 + 0.165 * max(0.0, 1.0 - a / 0.8) ** 1.25) * s
        return top(t) - (0.095 - 0.055 * min(1.0, (a - span) / 0.5)) * s

    X, Y, Z, R = gm.drape(pts, centre, FWD, th, top, bot, 26, 0.004, hb=math.radians(3.0), zb=0.008, smooth=(2, 2), wrap=True)
    n_z = Z.shape[0]
    v = np.linspace(0, 1, n_z)[:, None]
    rng = np.random.default_rng(11)
    ph = rng.random(4) * 6.28
    front = np.clip(1.0 - np.abs(th[None, :]) / span, 0, 1)
    fold = 0.0035 * np.sin(th[None, :] * 13 + 1.5 * np.sin(v * 4 + ph[0])) * np.clip((v - 0.12) / 0.3, 0, 1) * front
    fold += 0.004 * np.sin(v * 18 + th[None, :] * 3 + ph[1]) * np.clip((v - 0.55) / 0.3, 0, 1) * front
    dx, dy = gm.radial(th[None, :], FWD)
    piece = gm.grid(X + fold * dx, Y + fold * dy, Z, wrap=True)
    ob = gm.mesh('Bandana', piece, slot('Bandana'))
    # the knot at the back of the neck and its two ends
    back = np.argmin(np.abs(np.abs(th) - math.pi))
    k = np.array([X[n_z // 3, back], Y[n_z // 3, back], Z[n_z // 3, back]])
    out = gm.unit(np.array([k[0] - centre[0], k[1] - centre[1], 0.0]))
    knot = lm_ellipsoid('BandanaKnot', k + out * 0.006, (0.018, 0.013, 0.012), out)
    knot.data.materials.append(slot('Bandana'))
    return [ob, knot]


def lm_ellipsoid(name, centre, radii, z_axis, segments=16, rings=10):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segments, v_segments=rings, radius=1.0)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    rot = Vector(tuple(z_axis)).to_track_quat('Z', 'Y').to_matrix().to_4x4()
    ob.matrix_world = Matrix.Translation(Vector(tuple(centre))) @ rot @ Matrix.Diagonal((*radii, 1.0))
    bake_transform(ob)
    return ob


def bake_transform(ob):
    ob.data.transform(ob.matrix_world)
    ob.matrix_world = Matrix.Identity(4)


HAIR = dict(layers=((70, 0.003, 0.040, 0.0), (56, 0.008, 0.032, 0.05)), per=2, steps=10, locks=4)
HAIR_LOW = dict(layers=((26, 0.003, 0.072, 0.0), (18, 0.009, 0.062, 0.05)), per=1, steps=5, locks=2)


def build_hair(fig, under_obs, layers, per, steps, locks, name='Hair'):
    """Collar-length dark hair falling from under the hat in clumps (the portrait's strands, EllisBlender.grow,
    draped over the head, the collar and the cape), each clump per tapered locks of steps segments; locks over the
    left brow. HAIR builds the detailed hair the game's is baked from; HAIR_LOW the game's (fewer, wider locks)."""
    tree = gm.bvh_objects(under_obs)
    rng = np.random.default_rng(7)
    hf = fig.hf
    hv = hf['verts']
    s = fig.b.scale
    band_y = hf['eye_y'] + 0.47
    cz = (hv[:, 2].max() + hv[:, 2].min()) * 0.5 - 0.1
    centre = np.array([0.0, band_y, cz])
    head_c = fig.head_point(np.array([0.0, band_y - 0.3, cz]))
    paths = []
    # two layers of clumps round the head under the band, skipping the forehead: the inner close to the scalp, the
    # outer a little lower and looser, so no scalp shows between them
    for layer, (count, lift, width, drop) in enumerate(layers):
        for c in range(count):
            # round the back from the left temple to the right one (0 is the face): the forehead stays bare
            deg = 46.0 + 268.0 * (c + rng.random() * 0.8) / count
            ang = math.radians(deg)
            back = min(1.0, (180.0 - abs(180.0 - deg)) / 150.0)
            L = (0.10 + 0.04 * back ** 1.5) * s * (0.85 + 0.3 * rng.random()) - drop * 0.1
            r = centre + np.array([math.sin(ang) * 0.7, rng.normal() * 0.03 - drop, math.cos(ang) * 0.9])
            hit = tree.find_nearest(Vector(tuple(fig.head_point(r))))
            base = np.array(hit[0]) + np.array(hit[1]) * lift
            for k in range(per):
                rr = base + rng.normal(size=3) * 0.006
                st = eb.grow(tree, Vector(tuple(rr)), Vector((0, 0, -1)), L * (0.8 + 0.3 * rng.random()), steps, rng,
                             0.3 + 0.3 * rng.random(), lift + 0.003 + 0.005 * rng.random())
                paths.append((st, width * (0.8 + 0.4 * rng.random())))
    # the lock out from under the brim over the left brow, to the temple
    for k in range(locks):
        f = k * 4.0 / locks
        r = np.array([0.30 + 0.04 * f + rng.normal() * 0.01, band_y - 0.02, hv[:, 2].max() - 0.22])
        root = fig.head_point(r)
        tip = fig.head_point(np.array([0.48 + 0.03 * f, band_y - 0.36 + 0.03 * f, hv[:, 2].max() - 0.02]))
        d = Vector(tuple(tip - root))
        st = eb.grow(tree, Vector(tuple(root)), d.normalized(), d.length, min(6, steps), rng, 0.25, 0.004)
        paths.append((st, 0.016 * 4.0 / locks ** 0.5 / 2.0))
    piece = None
    for st, w in paths:
        st = np.asarray(st)
        t = np.linspace(0, 1, len(st))
        tang = gm.unit(np.gradient(st, axis=0))
        out = gm.unit(st - head_c)
        side = gm.unit(np.cross(tang, out))
        width = w * (1.0 - t) ** 1.1 + 0.0006
        # a lock, not a paper strip: its middle stands out from its edges, so it has a rounded back
        mid = st + out * (width * 0.28)[:, None]
        rows = np.stack([st - side * (width * 0.5)[:, None], mid, st + side * (width * 0.5)[:, None]], 1)
        rib = gm.grid(rows[..., 0], rows[..., 1], rows[..., 2])
        piece = rib if piece is None else piece + rib
    ob = gm.mesh(name, piece, slot('Hair'))
    log(f'{name}: {len(paths)} locks')
    return ob


# ------------------------------------------------------------------------------------- the gun belt and what hangs on it

def belt_height(th):
    """The gun belt's middle at angle th round the hips (0 the front, + the figure's left): riding low on the left,
    where the holster pulls it."""
    return 0.975 - 0.03 * np.maximum(0.0, np.cos(th - 0.8)) ** 2 - 0.008 * np.maximum(0.0, np.cos(th + 0.9)) ** 2


def build_belt(fig, under_obs):
    """The cartridge belt: a band round the hips over the trousers and the waistcoat's points, a row of brass
    cartridges in loops round the back, a brass buckle in front."""
    tree = gm.bvh_objects(under_obs)
    n = 120
    th = np.linspace(-math.pi, math.pi, n, endpoint=False)
    dx, dy = gm.radial(th, FWD)
    path, nrm = [], []
    for i in range(n):
        z = float(belt_height(th[i]))
        d = Vector((dx[i], dy[i], 0.0))
        o = Vector((fig.centre[0], fig.centre[1], z)) + d * 0.6
        hit = tree.ray_cast(o, -d, 1.0)
        p = np.array(hit[0]) if hit[0] is not None else np.array(o - d * 0.45)
        nn = np.array(hit[1]) if hit[0] is not None else np.array(d)
        nn[2] *= 0.5
        path.append(p + gm.unit(nn) * 0.006)
        nrm.append(gm.unit(nn))
    path, nrm = np.array(path), np.array(nrm)
    path = gm.laplacian(path, [[i, (i + 1) % n] for i in range(n)], 3)
    up = gm.unit(np.array([0.0, 0.0, 1.0]) - nrm * nrm[:, 2:3])
    rows = [path + up * 0.026, path + nrm * 0.002, path - up * 0.026]
    P = np.array(rows)
    belt = gm.mesh('Belt', gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True), slot('Belt'))
    parts = [belt]
    # the loops' band and the cartridges, round the back from hip to hip
    sel = np.nonzero(np.abs(th) > math.radians(70))[0]
    loops = [path[sel] + up[sel] * 0.006 + nrm[sel] * 0.006, path[sel] + nrm[sel] * 0.008, path[sel] - up[sel] * 0.016 + nrm[sel] * 0.006]
    order = np.argsort(np.where(th[sel] < 0, th[sel] + 2 * math.pi, th[sel]))
    L = np.array(loops)[:, order]
    parts.append(gm.mesh('BeltLoops', gm.grid(L[..., 0], L[..., 1], L[..., 2]), slot('Belt')))
    arc = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(path[sel][order], axis=0), axis=1))])
    pp, nn, uu = path[sel][order], nrm[sel][order], up[sel][order]
    case, bullet = None, None
    for a in np.arange(0.012, arc[-1] - 0.01, 0.0165):
        i = int(np.searchsorted(arc, a))
        p0 = pp[i] + nn[i] * 0.012
        c = gm.tube(np.array([p0 - uu[i] * 0.016, p0 + uu[i] * 0.02]), np.array([0.0057, 0.0057]), n=8, cap0=False)
        b = gm.tube(np.array([p0 + uu[i] * 0.02, p0 + uu[i] * 0.028, p0 + uu[i] * 0.033]), np.array([0.0052, 0.0042, 0.0012]), n=8)
        case = c if case is None else case + c
        bullet = b if bullet is None else bullet + b
    parts.append(gm.mesh('Cartridges', case, slot('Brass')))
    parts.append(gm.mesh('Bullets', bullet, slot('Lead')))
    # the buckle: a brass frame in front
    i0 = int(np.argmin(np.abs(th)))
    f = path[i0] + nrm[i0] * 0.004
    side = gm.unit(np.cross(up[i0], nrm[i0]))
    frame = None
    for (a, b, w) in (((-1, -1), (1, -1), 0.007), ((1, -1), (1, 1), 0.007), ((1, 1), (-1, 1), 0.007), ((-1, 1), (-1, -1), 0.007),
                      ((0.05, -1), (0.05, 1), 0.004)):
        p0 = f + side * a[0] * 0.03 + up[i0] * a[1] * 0.027
        p1 = f + side * b[0] * 0.03 + up[i0] * b[1] * 0.027
        bar = gm.tube(np.array([p0, p1]), np.array([w * 0.5, w * 0.5]), n=8, up=nrm[i0])
        frame = bar if frame is None else frame + bar
    parts.append(gm.mesh('Buckle', frame, slot('Brass')))
    fig.belt = (th, path, nrm, up)
    return parts


def belt_frame(fig, angle):
    """Point, outward normal, up and the direction round the belt (toward the back on the left side) at angle."""
    th, path, nrm, up = fig.belt
    i = int(np.argmin(np.abs((th - angle + math.pi) % (2 * math.pi) - math.pi)))
    tangent = gm.unit(path[(i + 1) % len(th)] - path[i - 1])
    return path[i], nrm[i], up[i], tangent


def build_holster(fig, under_obs):
    """The cross-draw holster on the left front hip, a Mexican loop holster as the 1870s wore them: a leather skirt
    folded over the belt, the pouch pushed through two loops cut in it, canted so the revolver's walnut grip points
    forward for the right hand. Above the pouch's throat the cylinder, the hammer and the frame show."""
    tree = gm.bvh_objects(under_obs)
    p, n, up, t = belt_frame(fig, 0.70)
    cant = math.radians(30)
    a = gm.unit(-up * math.cos(cant) + t * math.sin(cant))      # down the barrel: down and back
    g = gm.unit(up - a * (up @ a))                                # the gun's top, in the hip's plane
    across = gm.unit(np.cross(n, a))
    parts = []
    # the skirt: a panel on the hip from over the belt down along the pouch, tapering
    rows = []
    for u in np.linspace(-0.06, 0.20, 9):
        row = []
        half = 0.05 - 0.016 * max(u, 0.0) / 0.20
        for w in np.linspace(-1.0, 1.0, 5):
            q = p + a * u + across * (half * w)
            hit = tree.ray_cast(Vector(tuple(q + n * 0.15)), Vector(tuple(-n)), 0.3)
            base = np.array(hit[0]) if hit[0] is not None else q
            row.append(base + n * (0.010 if u > 0.03 else 0.016))
        rows.append(row)
    P = np.array(rows)
    parts.append(gm.mesh('HolsterSkirt', gm.grid(P[..., 0], P[..., 1], P[..., 2]), slot('Belt')))
    # the pouch: flattened, swelling round the cylinder at the throat, tapering to the toe, closed there
    mouth = p + a * 0.035 + n * 0.034
    us = np.linspace(0, 1, 11)
    path = np.array([mouth + a * 0.20 * u for u in us])
    rad = np.array([[0.031 + 0.006 * math.exp(-((u - 0.12) / 0.12) ** 2) - 0.014 * u, 0.020 - 0.008 * u] for u in us])
    parts.append(gm.mesh('Holster', gm.tube(path, rad, n=22, up=n, cap1=True), slot('Belt')))
    for u in (0.12, 0.55):
        c0 = mouth + a * 0.20 * u
        r0 = np.interp(u, us, rad[:, 0]) + 0.005
        r1 = np.interp(u, us, rad[:, 1]) + 0.005
        loop = gm.tube(np.array([c0 - a * 0.014, c0 + a * 0.014]), np.array([[r0, r1], [r0, r1]]), n=22, up=n)
        parts.append(gm.mesh('HolsterLoop', loop, slot('Belt')))
    # the revolver: the cylinder half out of the throat, the frame and hammer behind it, the grip curving forward
    cyl_c = mouth - a * 0.012 + g * 0.004
    parts.append(gm.mesh('RevolverCylinder', gm.tube(np.array([cyl_c - a * 0.02, cyl_c + a * 0.02]),
                                                     np.array([0.0185, 0.0185]), n=18, up=n, cap0=True, cap1=True), slot('GunIron')))
    back = mouth - a * 0.04 + g * 0.002
    parts.append(gm.mesh('RevolverFrame', gm.tube(np.array([back - a * 0.008, back + a * 0.026]),
                                                  np.array([[0.016, 0.0105], [0.019, 0.0105]]), n=12, up=n, cap0=True, cap1=True),
                         slot('GunIron')))
    hammer = np.array([back + g * 0.012, back + g * 0.021 - a * 0.012, back + g * 0.022 - a * 0.02])
    parts.append(gm.mesh('RevolverHammer', gm.tube(hammer, np.array([[0.0035, 0.0045], [0.003, 0.004], [0.0035, 0.004]]),
                                                   n=8, up=n, cap0=True, cap1=True), slot('GunIron')))
    grip_dir = gm.unit(-g * 0.80 - a * 0.60)
    us = np.linspace(0, 1, 11)
    gp = np.array([back - g * 0.006 + grip_dir * 0.105 * u - g * 0.018 * u * u for u in us])
    # a plow-handle grip: slim at the frame, swelling to the butt, the butt rounded off
    grad = np.array([[0.0135 + 0.007 * u ** 1.5, 0.0118 + 0.0015 * u] for u in us])
    grad[-1] = grad[-2] * 0.55
    parts.append(gm.mesh('RevolverGrip', gm.tube(gp, grad, n=14, up=n, cap1=True), slot('GunWalnut')))
    fig.holster = (mouth, a, g, n)
    return parts


def lathe(name, profile, centre, n=24, material=None, cap_top=True):
    """A body of revolution: profile [(radius, height)] from the bottom up about a vertical axis at centre."""
    profile = np.asarray(profile, dtype=np.float64)
    ang = np.linspace(0, 2 * math.pi, n, endpoint=False)
    X = centre[0] + profile[:, 0:1] * np.cos(ang)[None]
    Y = centre[1] + profile[:, 0:1] * np.sin(ang)[None]
    Z = centre[2] + np.repeat(profile[:, 1:2], n, 1)
    pc = gm.grid(X[::-1], Y[::-1], Z[::-1], wrap=True)
    verts = list(pc.verts)
    faces, uv = pc.faces, pc.uv
    for row, at in ((0, profile[-1]), (len(profile) - 1, profile[0])):
        if (row == 0 and not cap_top) or at[0] < 1e-4:
            continue
        c = len(verts)
        verts.append(np.array([centre[0], centre[1], centre[2] + at[1]]))
        for i in range(n):
            a, b = row * n + i, row * n + (i + 1) % n
            faces.append([b, a, c] if row == 0 else [a, b, c])
            uv += [(0.0, 0.0), (0.01, 0.0), (0.005, 0.01)]
    return gm.mesh(name, gm.Piece(np.array(verts), faces, uv), material)


def build_lantern(fig, under_obs):
    """Pa's lantern clipped to the belt at the right hip, still burning: a brass fount, a glass globe in a wire
    guard, a chimney cap and a bail hung on a belt hook."""
    p, n, up, t = belt_frame(fig, -0.95)
    tree = gm.bvh_objects(under_obs)
    top = p - up * 0.032
    c = top + n * 0.05 - np.array([0, 0, 0.19])
    # keep the lantern off the thigh
    for _ in range(6):
        hit = tree.find_nearest(Vector(tuple(c + np.array([0, 0, 0.07]))), 0.2)
        if hit[0] is None:
            break
        d = (Vector(tuple(c + np.array([0, 0, 0.07]))) - hit[0]).dot(hit[1])
        if d > 0.055:
            break
        c = c + np.array(hit[1]) * (0.056 - d)
        c[2] = top[2] - 0.19
    parts = []
    parts.append(lathe('LanternFount', [(0.0, 0.0), (0.040, 0.0), (0.043, 0.006), (0.043, 0.022), (0.036, 0.03)], c,
                       material=slot('Brass')))
    globe = [(0.026, 0.03), (0.033, 0.045), (0.036, 0.068), (0.032, 0.092), (0.022, 0.108)]
    parts.append(lathe('LanternGlobe', globe, c, material=slot('LanternGlow'), cap_top=False))
    parts.append(lathe('LanternCap', [(0.042, 0.105), (0.040, 0.112), (0.024, 0.135), (0.018, 0.138), (0.018, 0.148),
                                      (0.010, 0.152), (0.0, 0.153)], c, material=slot('Iron')))
    wires = None
    for k in range(4):
        a = k * math.pi / 2 + math.pi / 4
        d = np.array([math.cos(a), math.sin(a), 0.0])
        w = gm.tube(np.array([c + d * 0.037 + [0, 0, 0.028], c + d * 0.040 + [0, 0, 0.068], c + d * 0.036 + [0, 0, 0.106]]),
                    np.array([0.0022] * 3), n=6)
        wires = w if wires is None else wires + w
    for zz, rr in ((0.052, 0.039), (0.085, 0.038)):
        ring = gm.tube(np.array([c + [rr * math.cos(a), rr * math.sin(a), zz] for a in np.linspace(0, 2 * math.pi, 25)]),
                       np.array([0.002] * 25), n=6)
        wires = wires + ring
    bail = np.array([c + [0.043 * math.cos(a), 0.0, 0.118 + 0.07 * math.sin(a)] for a in np.linspace(0, math.pi, 15)])
    wires = wires + gm.tube(bail, np.array([0.0022] * 15), n=6)
    hook = np.array([top + n * 0.004, top + n * 0.03 - up * 0.01, c + [0, 0, 0.188]])
    wires = wires + gm.tube(hook, np.array([0.003] * 3), n=6)
    parts.append(gm.mesh('LanternWire', wires, slot('Iron')))
    flame = lm_ellipsoid('LanternFlame', c + np.array([0, 0, 0.062]), (0.008, 0.008, 0.016), (0, 0, 1), 10, 8)
    flame.data.materials.append(slot('LanternGlow'))
    parts.append(flame)
    fig.lantern = c
    return parts


# ------------------------------------------------------------------------------------------------------ the main

SHOTS = {'full': (0.92, 4.3, 0.25), 'upper': (1.45, 1.7, 0.08), 'feet': (0.22, 1.5, 0.35), 'hips': (0.95, 1.8, 0.1)}


def preview(objects, path, views=(('front', 0.0), ('three', 35.0), ('side', 90.0), ('back', 180.0)), size=900, samples=24,
            aim=SHOTS['full']):
    """Quick Cycles renders round the figure (shape checks)."""
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
        # shape checks in clay: the looks unbaked are minutes a frame
        clay = bpy.data.materials.get('_Clay') or bpy.data.materials.new('_Clay')
        clay.diffuse_color = (0.55, 0.52, 0.48, 1.0)
        bpy.context.view_layer.material_override = clay
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


# ------------------------------------------------------------------------------------------- UVs, bake, rig, export

SMART_UV = ('Eye_', 'Soles', 'HatCrown', 'HatBrim', 'HatBand', 'VestButton', 'Concho', 'SunPin', 'BandanaKnot',
            'Cartridges', 'Bullets', 'Buckle', 'Holster', 'HolsterLoop', 'Revolver', 'Lantern', 'Gauntlet')
# Sheets given thickness after the bake (m); the inner side shares the outer side's UVs.
THICK = {'Duster': 0.004, 'Cape': 0.004, 'Collar': 0.005, 'Bandana': 0.0015, 'Hair': 0.0012, 'Belt': 0.005,
         'BeltLoops': 0.003, 'HolsterSkirt': 0.004, 'HatBrim': 0.0055, 'HatBand': 0.0015, 'Gauntlet_l': 0.003,
         'Gauntlet_r': 0.003, 'HolsterLoop': 0.002}
# Hidden insides stay single-sided: the waistcoat, shirt and trousers under the coat, the sleeves inside the gauntlets,
# the boots' shafts round the trousers, the crown above the band.

# The game mesh (about 20k triangles, the user's call of 2026-10-15: Borderlands' budget): each detailed part's share of
# its triangles kept by Blender's Decimate (collapse); 1 keeps a part as it is. Its looks, folds and creases are baked
# from the detailed parts (looter_bake, sources). The hair is built again with fewer, wider locks (HAIR_LOW).
LOW = {'Ellis': 0.20, 'Eye': 0.22, 'Trousers': 0.42, 'Boots': 0.22, 'Soles': 0.5, 'Vest': 0.36, 'Shirt': 0.4,
       'VestButton': 0.25, 'Duster': 0.09, 'Sleeves': 0.42, 'Cape': 0.10, 'Collar': 0.20, 'SunPin': 0.4, 'Gloves': 0.18,
       'Gauntlet': 0.30, 'Concho': 0.5, 'HatCrown': 0.07, 'HatBrim': 0.09, 'HatBand': 0.3, 'Bandana': 0.13,
       'BandanaKnot': 0.4, 'Belt': 0.4, 'BeltLoops': 0.35, 'Cartridges': 0.4, 'Bullets': 0.3, 'Buckle': 1.0,
       'HolsterSkirt': 1.0, 'Holster': 0.6, 'HolsterLoop': 0.5, 'Revolver': 0.8, 'LanternWire': 0.3, 'Lantern': 0.5}


def low_ratio(name):
    key = name.split('.')[0]
    for k in sorted(LOW, key=len, reverse=True):
        if key.startswith(k):
            return LOW[k]
    return 1.0


def make_lows(fig, highs, hair_under):
    """The game mesh: a reduced copy of every detailed part (named <part>_low until the detailed ones go), the eye
    region of the face kept dense, the bandana's back (under the collar and hair) left out, the hair rebuilt."""
    lows = []
    for ob in highs:
        if ob.name.startswith('Hair'):
            continue
        low = bpy.data.objects.new(ob.name + '_low', ob.data.copy())
        bpy.context.scene.collection.objects.link(low)
        low.matrix_world = ob.matrix_world              # placed parts (eyes, buttons, pin, conchos) keep their place
        bake_transform(low)
        if ob.name.startswith('Bandana') and not ob.name.startswith('BandanaKnot'):
            mid = (fig.eyes[0] + fig.eyes[1]) * 0.5
            c = mid[:2] - FWD * 0.085 * fig.b.scale
            bm = bmesh.new()
            bm.from_mesh(low.data)
            back = [f for f in bm.faces if (lambda q: math.atan2(abs(q.x - c[0]), -(q.y - c[1])))(f.calc_center_median()) > 2.0]
            bmesh.ops.delete(bm, geom=back, context='FACES')
            bm.to_mesh(low.data)
            bm.free()
        ratio = low_ratio(ob.name)
        if ratio < 1.0:
            m = low.modifiers.new('decimate', 'DECIMATE')
            m.ratio = ratio
            if ob.name == 'Ellis':
                # the eyes and lids keep their detail: the decimation spares a group weighted round them
                g = low.vertex_groups.new(name='keep')
                co = gm._object_coords(low)
                d = np.minimum(np.linalg.norm(co - fig.eyes[0], axis=1), np.linalg.norm(co - fig.eyes[1], axis=1))
                for i in np.nonzero(d < 0.03)[0]:
                    g.add([int(i)], 1.0, 'REPLACE')
                # (Decimate collapses only what its group weights: inverted, the eyes are the part it spares)
                m.vertex_group = 'keep'
                m.invert_vertex_group = True
                m.vertex_group_factor = 1.0
            gm.apply_modifiers(low, keep=())
            if 'keep' in low.vertex_groups:
                low.vertex_groups.remove(low.vertex_groups['keep'])
        lows.append(low)
    hair = build_hair(fig, hair_under, name='Hair_low', **HAIR_LOW)
    lows.append(hair)
    # reduced layers can cut through one another where the detailed ones only just cleared: each outer part is pushed
    # back outside the reduced parts under it
    by = lambda *names: [o for o in lows if o.name.split('.')[0].startswith(names)]
    for outer, inner in (('Vest', ('Shirt',)), ('Duster', ('Vest', 'Shirt', 'Belt', 'Trousers')),
                         ('Cape', ('Duster', 'Sleeves')), ('Collar', ('Cape',)), ('Gauntlet', ('Sleeves',)),
                         ('Bandana', ('Shirt', 'Vest', 'Ellis')), ('Boots', ('Trousers',))):
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


def look_of(ob):
    return ob.material_slots[0].material['LookKey'] if ob.material_slots and ob.material_slots[0].material else None


def prepare_uvs(objects):
    """Every part gets a usable 'UVMap': MakeHuman's on the shells, arc lengths on the drapes, smart projection on
    small hard parts. The hair keeps its ribbon coordinates as 'Strand' for its look."""
    for ob in objects:
        me = ob.data
        if ob.name.startswith('Hair'):
            strand = me.uv_layers.new(name='Strand')
            src = me.uv_layers['UVMap']
            uv = np.empty(len(src.data) * 2, dtype=np.float32)
            src.data.foreach_get('uv', uv)
            strand.data.foreach_set('uv', uv)
            me.uv_layers.active = src
        if ob.name.startswith(SMART_UV) or 'UVMap' not in me.uv_layers:
            for layer in list(me.uv_layers):
                if layer.name != 'Strand':
                    me.uv_layers.remove(layer)
            bk.smart_uv(ob)
        me.uv_layers.active = me.uv_layers['UVMap']
        me.uv_layers['UVMap'].active_render = True


BAKE_SOURCES = [os.path.abspath(__file__)] + [os.path.join(REPO, *p) for p in (
    ('Tools', 'Blender', 'looter_human.py'), ('Tools', 'Blender', 'looter_garments.py'), ('Tools', 'Blender', 'looter_looks.py'),
    ('Tools', 'Blender', 'looter_bake.py'), ('Art', 'Portraits', 'mh.py'), ('Art', 'Portraits', 'EllisBlender.py'))]


def bake_all(lows, highs, size):
    """Packs every texture set's UVs on the game mesh and bakes it from the detailed parts. The bake is skipped when the
    sets on disk were baked from these same scripts at this size (the digest beside them, Art/Textures/EllisSkin/
    bake.digest; --rebake forces it): the model and its UV packing are deterministic, so the old maps still fit.
    Returns {set: bake result}."""
    stamp = os.path.join(lt.TEXTURE_DIR, 'EllisSkin', 'bake.digest')
    want = bk.digest(BAKE_SOURCES, f'size={size} blender={bpy.app.version_string}')
    have = open(stamp).read().strip() if os.path.exists(stamp) else ''
    out = {}
    for set_name, keys in TEXTURE_SETS.items():
        obs = [o for o in lows if look_of(o) in keys]
        src = [o for o in highs if look_of(o) in keys]
        weights = {o: 1.6 for o in obs if o.name.startswith(('Bandana', 'Gloves'))}
        bk.pack(obs, weights=weights)
        if have == want and '--rebake' not in ARGS:
            out[set_name] = bk.load_set(set_name)
        if out.get(set_name) is None:
            out[set_name] = bk.bake_set(set_name, obs, size, sources=src, hide=highs)
            log(f'baked {set_name} ({len(obs)} parts from {len(src)})')
        else:
            log(f'{set_name}: kept (baked from these scripts before)')
    os.makedirs(os.path.dirname(stamp), exist_ok=True)
    with open(stamp, 'w') as f:
        f.write(want + '\n')
    return out


def thicken(objects):
    for ob in objects:
        t = THICK.get(ob.name.split('.')[0])
        if t:
            gm.solidify(ob, t, offset=0.0 if ob.name.startswith('HatBrim') else 1.0, rim=not ob.name.startswith('Hair'))


def rigid_w(fig, n, parts):
    return hu.blend(fig.names, n, parts)


def weights(fig, ob):
    """The mannequin bone weights of ob's vertices, by what the part is (see the module's docstring)."""
    me = ob.data
    P = np.array([v.co[:] for v in me.vertices])
    n = len(P)
    name = ob.name
    sk = fig.sk
    if name == 'Ellis':
        return fig.b.weights_at(P)[1]         # the decimated skin: its own place on the body
    if name.startswith(('Eye_', 'Hat', 'Hair')):
        return rigid_w(fig, n, {'head': 1.0})
    if name.startswith('Bandana'):
        chin = fig.eye_z - 0.10
        t = gm.smoothstep(chin, chin - 0.14, P[:, 2])
        return rigid_w(fig, n, {'head': 1.0 - t, 'neck_01': t * 0.4, 'spine_05': t * 0.6})
    if name.startswith(('Collar', 'SunPin')):
        t = gm.smoothstep(fig.neck[2] - 0.03, fig.neck[2] + 0.1, P[:, 2])
        return rigid_w(fig, n, {'spine_05': 1.0 - 0.5 * t, 'neck_01': 0.5 * t})
    if name.startswith('Cape'):
        names, near = fig.b.weights_at(P)
        side = gm.smoothstep(-0.08, 0.08, P[:, 0])
        anchor = hu.blend(fig.names, n, {'spine_05': 0.5, 'clavicle_l': 0.5 * side, 'clavicle_r': 0.5 * (1 - side)})
        return hu.top4(0.45 * near + 0.55 * anchor)
    if name.startswith('Duster'):
        torso_faces = [f for f, k in zip(fig.faces, fig.face_mask((fig.w_arm < 0.3) & (fig.w_head < 0.3))) if k]
        names, near = fig.b.weights_at(P, torso_faces)
        hip, knee = 0.97, sk['thigh_l'][1][2]
        t = np.clip((hip - P[:, 2]) / (hip - knee), 0.0, 1.0)
        leg = 0.75 * t ** 0.8
        side = gm.smoothstep(0.0, 1.0, 0.5 + P[:, 0] / 0.36)
        skirt = hu.blend(fig.names, n, {'pelvis': 1.0 - leg, 'thigh_l': leg * side, 'thigh_r': leg * (1.0 - side)})
        k = gm.smoothstep(hip + 0.06, hip - 0.06, P[:, 2])[:, None]
        return hu.top4((1.0 - k) * near + k * skirt)
    if name.startswith(('Belt', 'Cartridges', 'Bullets', 'Buckle')):
        return rigid_w(fig, n, {'pelvis': 0.85, 'spine_01': 0.15})
    if name.startswith(('Holster', 'Revolver')):
        return rigid_w(fig, n, {'pelvis': 0.65, 'thigh_l': 0.35})
    if name.startswith('Lantern'):
        return rigid_w(fig, n, {'pelvis': 0.6, 'thigh_r': 0.4})
    if name.startswith(('Gauntlet', 'Concho')):
        s = name.split('_')[1][0]
        e, w = sk[f'lowerarm_{s}'][0], sk[f'lowerarm_{s}'][1]
        t = np.clip(((P - e) @ gm.unit(w - e)) / np.linalg.norm(w - e), 0, 1)
        k = gm.smoothstep(0.75, 1.05, t)
        return rigid_w(fig, n, {f'lowerarm_{s}': 1.0 - 0.5 * k, f'hand_{s}': 0.5 * k})
    if name.startswith('Gloves'):
        hand_faces = [f for f, k in zip(fig.faces, fig.face_mask(fig.w_hand > 0.3)) if k]
        return fig.b.weights_at(P, hand_faces)[1]
    if name.startswith('Soles'):
        out = np.zeros((n, len(fig.names)))
        for s in 'lr':
            ankle, ball = sk[f'foot_{s}'][0], sk[f'ball_{s}'][0]
            f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
            sel = (P[:, 0] * ball[0]) > 0
            t = gm.smoothstep(0.09, 0.14, (P[sel] - ankle) @ f)
            out[sel] = hu.blend(fig.names, int(sel.sum()), {f'foot_{s}': 1.0 - t, f'ball_{s}': t})
        return out
    return fig.b.weights_at(P)[1]


def rig(fig, objects):
    """Weights every part, joins them into one mesh 'Ellis' (SK_Ellis in Unreal) under the mannequin armature."""
    arm = fig.b.armature('Ellis_Rig')
    for ob in objects:
        W = weights(fig, ob)
        for j, bone in enumerate(fig.names):
            col = W[:, j]
            nz = np.nonzero(col > 1e-4)[0]
            if not len(nz):
                continue
            g = ob.vertex_groups.get(bone) or ob.vertex_groups.new(name=bone)
            for i in nz:
                g.add([int(i)], float(col[i]), 'REPLACE')
    skin = next(o for o in objects if o.name == 'Ellis')
    for ob in objects:
        if 'mh_index' in ob.data.attributes:
            ob.data.attributes.remove(ob.data.attributes['mh_index'])
    bk.select(objects, skin)
    bpy.ops.object.join()
    skin = bpy.context.view_layer.objects.active
    skin.name = skin.data.name = 'Ellis'
    if 'Strand' in skin.data.uv_layers:
        skin.data.uv_layers.remove(skin.data.uv_layers['Strand'])
    skin.parent = arm
    mod = skin.modifiers.new('Armature', 'ARMATURE')
    mod.object = arm
    arm['LODs'] = '50,25'
    arm['LODScreens'] = '0.5,0.2'
    hit_zones(fig, arm)
    log(f'rigged: {len(skin.data.polygons)} faces, {sum(len(p.vertices) - 2 for p in skin.data.polygons)} triangles, '
        f'{len(skin.vertex_groups)} weighted bones')
    return arm, skin


def hit_zones(fig, arm):
    """Hit zones (the physics asset's bodies): the head with its hat, the trunk in its coat, the coat's skirt, and
    each limb."""
    sk = fig.sk
    mid = lambda b, t=0.5: sk[b][0] + (sk[b][1] - sk[b][0]) * t
    lm.hit_sphere(arm, 'head', tuple(mid('head', 0.55)), 0.15)
    lm.hit_capsule(arm, 'spine_03', (0.0, fig.centre[1], 1.12), (0.0, fig.centre[1], 1.40), 0.2)
    lm.hit_capsule(arm, 'pelvis', (0.0, fig.centre[1], 0.62), (0.0, fig.centre[1], 0.92), 0.24)
    for s in 'lr':
        for bone, r in (('upperarm', 0.075), ('lowerarm', 0.065), ('thigh', 0.11), ('calf', 0.08)):
            a, b = sk[f'{bone}_{s}'][0], sk[f'{bone}_{s}'][1]
            lm.hit_capsule(arm, f'{bone}_{s}', tuple(a + (b - a) * 0.15), tuple(a + (b - a) * 0.85), r)
        lm.hit_sphere(arm, f'hand_{s}', tuple(mid(f'hand_{s}', 0.6)), 0.06)
        lm.hit_capsule(arm, f'foot_{s}', tuple(sk[f'foot_{s}'][0] + np.array([0, 0.04, -0.03])),
                       tuple(sk[f'ball_{s}'][0] + np.array([0, -0.03, -0.02])), 0.055)
    for ob in arm.children:
        if ob.name.startswith(('USP_', 'UCP_')):
            ob.hide_render = True


def game_materials(skin):
    """The looks swapped for the game's: a textured material per GAME_MATERIALS slot (MI_<Slot> on its master, the maps
    of the set its looks bake into), Glow for the glows."""
    swap = {}
    for slot_name, (master, keys, params) in GAME_MATERIALS.items():
        sets = {s for s, ks in TEXTURE_SETS.items() if set(keys) & set(ks)}
        assert len(sets) == 1, f'{slot_name}: its looks must bake into one set, not {sets}'
        mat = lt.material(sets.pop(), name=slot_name, master=master, **params)
        for k in keys:
            swap[f'Ellis_{k}'] = mat
    for k, (hex_value, strength) in GLOWS.items():
        swap[f'Ellis_{k}'] = lm.material(f'Ellis{k}', hex_value, 'Glow', Glow=strength)
    for i, s_ in enumerate(skin.material_slots):
        if s_.material is not None and s_.material.name in swap:
            skin.material_slots[i].material = swap[s_.material.name]
    # merge slots that now share a material
    bk.select([skin])
    bpy.ops.object.material_slot_remove_unused()


def viewer_materials(skin, baked):
    """Materials for the glTF viewer: the baked maps on a Principled BSDF the glTF exporter understands (base colour,
    normal map, roughness and metal from the ORM's G and B, occlusion from its R), emission for the glows."""
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
        nrm = nt.nodes.new('ShaderNodeTexImage')
        nrm.image = imgs['NORMAL']
        nm = nt.nodes.new('ShaderNodeNormalMap')
        nt.links.new(nrm.outputs['Color'], nm.inputs['Color'])
        nt.links.new(nm.outputs['Normal'], b.inputs['Normal'])
        group = bpy.data.node_groups.get('glTF Material Output')
        if group is None:
            group = bpy.data.node_groups.new('glTF Material Output', 'ShaderNodeTree')
            group.interface.new_socket('Occlusion', in_out='INPUT', socket_type='NodeSocketFloat')
        gn = nt.nodes.new('ShaderNodeGroup')
        gn.node_tree = group
        nt.links.new(sep.outputs[0], gn.inputs['Occlusion'])
        for k in keys:
            mats[f'Ellis_{k}'] = mat
    for k in GLOWS:
        mats[f'Ellis_{k}'] = bpy.data.materials.get(f'Ellis_{k}')
    for i, s_ in enumerate(skin.material_slots):
        if s_.material is not None and s_.material.name in mats:
            skin.material_slots[i].material = mats[s_.material.name]


VIEW_POSE = {'upperarm_l': ((0.0, 1.0, 0.0), 34.0), 'upperarm_r': ((0.0, 1.0, 0.0), -34.0),
             'lowerarm_l': ((1.0, 0.0, 0.0), 14.0), 'lowerarm_r': ((1.0, 0.0, 0.0), 14.0)}


def pose_for_view(arm):
    """A relaxed stance for the viewer: the arms let down from the bind pose's A toward the sides."""
    for bone, (axis, deg) in VIEW_POSE.items():
        pb = arm.pose.bones[bone]
        pb.rotation_mode = 'XYZ'
        m = pb.bone.matrix_local.to_3x3()
        world_axis = Vector(axis)
        local_axis = m.inverted() @ world_axis
        pb.rotation_mode = 'QUATERNION'
        from mathutils import Quaternion
        pb.rotation_quaternion = Quaternion(local_axis.normalized(), math.radians(deg))
    bpy.context.view_layer.update()


def export_glb(skin, arm, path, lantern, posed=True):
    """A static, posed copy of the textured figure as a .glb for the spin viewer (images as WebP)."""
    if posed:
        pose_for_view(arm)
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(skin.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
    ob = bpy.data.objects.new('EllisView', me)
    bpy.context.scene.collection.objects.link(ob)
    ob.matrix_world = skin.matrix_world
    anchor = bpy.data.objects.new('LanternAnchor', None)          # where the viewer puts the lantern's light
    bpy.context.scene.collection.objects.link(anchor)
    anchor.location = Vector(tuple(lantern + np.array([0.0, 0.0, 0.065])))
    bk.select([ob, anchor], ob)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    kw = dict(filepath=path, export_format='GLB', use_selection=True, export_apply=True, export_yup=True, export_extras=False,
              export_texcoords=True, export_normals=True, export_tangents=False, export_materials='EXPORT',
              export_skins=False, export_animations=False)
    try:
        bpy.ops.export_scene.gltf(export_image_format='WEBP', export_image_quality=88, **kw)
    except TypeError:
        bpy.ops.export_scene.gltf(**kw)
    bpy.data.objects.remove(ob)
    bpy.data.objects.remove(anchor)
    for pb in arm.pose.bones:
        pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    log(f'wrote {path} ({os.path.getsize(path) / 1e6:.1f} MB)')


def settle(fig, duster, colliders):
    """The duster's skirt falls into its folds: Blender's cloth over the body, the trousers, the waistcoat and the
    belt, the coat held above the hips (looter_garments.simulate, kept in Ellis.drape.json). Stiff canvas: bending 12
    gives the duster's broad folds; at 4 the skirt and the vent crumpled like linen. The cape is not simulated: on the
    shoulders it had nowhere to fall but into itself, and crumpled."""
    zd = gm._object_coords(duster)[:, 2]
    t0 = time.time()
    ran = gm.simulate([duster], [gm.smoothstep(0.92, 1.08, zd)], colliders, DRAPE_FILE, frames=int(arg_value('--frames', 90)),
                      stiff=(40.0, 40.0, 15.0, 12.0), mass=0.3, self_collision=True,
                      note='The settled duster of Ellis (Art/Models/Characters/Ellis.py writes it: the cloth '
                           'simulation\'s result for the digest of its inputs).')
    log(f'cloth: {"simulated" if ran else "kept from Ellis.drape.json"} ({time.time() - t0:.0f} s)')


def fetch_makehuman():
    """MakeHuman's base mesh is not in git (an .obj would go to LFS): fetch it on a fresh clone."""
    if not os.path.exists(os.path.join(mh.DATA, '3dobjs', 'base.obj')):
        import subprocess
        log('fetching MakeHuman\'s CC0 assets (Art/ThirdParty/MakeHuman/fetch.py)')
        subprocess.run([sys.executable, os.path.join(REPO, 'Art', 'ThirdParty', 'MakeHuman', 'fetch.py')], check=True)


def main():
    fetch_makehuman()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    fig = Figure()
    skin, eyes = build_skin(fig)
    trousers = build_trousers(fig)
    boots = build_boots(fig)
    vest, shirt, buttons = build_vest(fig)
    log('trousers, boots, waistcoat')
    body_ob = fig.b.mesh('_Body')
    body_ob.hide_render = True
    under = gm.object_points(body_ob)
    torso_pts = fig.wv[fig.torso_idx]
    legs = fig.wv[fig.b.used][fig.w_leg[fig.b.used] > 0.3]
    belt = build_belt(fig, [trousers, vest])
    belt_pts = np.concatenate([gm.object_points(o) for o in belt])
    drape_pts = np.concatenate([torso_pts, legs, gm.object_points(vest), gm.object_points(trousers), belt_pts])
    duster = build_duster(fig, drape_pts, belt)
    sleeves = build_sleeves(fig, [body_ob])
    log('duster, sleeves')
    cape_pts = np.concatenate([under[under[:, 2] > fig.shoulder_z - 0.25], gm.object_points(duster), gm.object_points(sleeves)])
    cape, top_r = build_cape(fig, cape_pts)
    holster = build_holster(fig, [trousers])
    lantern = build_lantern(fig, [trousers])
    log('belt, holster, lantern')
    if '--selfx' in ARGS:
        from mathutils.bvhtree import BVHTree
        for ob in (duster, cape):
            me = ob.data
            tree = BVHTree.FromPolygons([v.co[:] for v in me.vertices], [p.vertices[:] for p in me.polygons])
            polys = [set(p.vertices) for p in me.polygons]
            bad = [(a, b) for a, b in tree.overlap(tree) if a < b and not (polys[a] & polys[b])]
            cen = [tuple(round(c, 3) for c in me.polygons[a].center) for a, b in bad[:12]]
            log(f'{ob.name}: {len(bad)} crossing face pairs, e.g. {cen}')
        return
    if SIM:
        settle(fig, duster, [body_ob, trousers, vest] + belt[:2])
        if '--sim-only' in ARGS:
            for ob in (duster, cape):
                co = gm._object_coords(ob)
                log(f'{ob.name}: {co.min(0).round(3)} .. {co.max(0).round(3)}')
            return
    collar, pin = build_collar(fig, top_r)
    gloves = build_gloves(fig, sleeves)
    log('cape, collar, gloves')
    hat = build_hat(fig)
    bandana = build_bandana(fig, [skin, vest, shirt])
    hair = build_hair(fig, [skin, collar, cape, bandana[0]], **HAIR)
    log('hat, bandana, hair')
    for ob in [cape, collar, bandana[0], vest, shirt] + belt[:2] + holster[:1] + hat[2:] + boots[:1] + \
            [g for g in gloves if g.name.startswith('Gauntlet')]:
        set_seams(ob)
    bpy.data.objects.remove(body_ob)
    out = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Characters', 'Ellis')
    os.makedirs(out, exist_ok=True)
    if '--shape' in ARGS:
        for shot in (arg_value('--shots', 'full')).split('+'):
            preview(None, os.path.join(out, f'Ellis_{shot}'), aim=SHOTS[shot])
        return
    highs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    prepare_uvs([o for o in highs if o.name.startswith('Hair')])
    parts = make_lows(fig, highs, [skin, collar, cape, bandana[0]])
    log(f'game mesh: {sum(len(p.data.polygons) for p in parts)} faces before thickness')
    prepare_uvs(parts)
    baked = None if '--stats' in ARGS else bake_all(parts, highs, int(arg_value('--size', 2048)))
    # the detailed parts have done their work: the game parts take their names
    for ob in highs:
        me = ob.data
        bpy.data.objects.remove(ob)
        bpy.data.meshes.remove(me)
    for ob in parts:
        ob.name = ob.name[:-4] if ob.name.endswith('_low') else ob.name
    thicken(parts)
    if '--stats' in ARGS:
        tris = {}
        for ob in parts:
            key = ob.name.split('.')[0].rstrip('_lr').rstrip('_')
            tris[key] = tris.get(key, 0) + sum(len(p.vertices) - 2 for p in ob.data.polygons)
        for k, n in sorted(tris.items(), key=lambda kv: -kv[1]):
            log(f'{k:16s} {n:7d}')
        log(f'{"total":16s} {sum(tris.values()):7d}')
        return
    arm, skin = rig(fig, parts)
    if '--glb' in ARGS:
        viewer_materials(skin, baked)
        export_glb(skin, arm, os.path.abspath(arg_value('--glb')), fig.lantern)
    if '--preview' in ARGS:
        viewer_materials(skin, baked)
        pose_for_view(arm)
        for shot in (arg_value('--shots', 'full')).split('+'):
            preview(None, os.path.join(out, f'Ellis_{shot}'), aim=SHOTS[shot])
        for pb in arm.pose.bones:
            pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        log('preview written')
    game_materials(skin)
    log('ready to export')


if __name__ == '__main__':
    main()
