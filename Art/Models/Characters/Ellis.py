"""SK_Ellis: Ellis Ransom, the Revenant, the player's own body (Docs/Story.md; "they" in all text), built from the
portrait the user picked on 2026-10-15: option A of Art/Portraits/EllisBlender.py (its OPTIONS["A"] and FACE are the
spec). Boots to hat:
  - the body: MakeHuman's CC0 base (Art/ThirdParty/MakeHuman) shaped as the portrait's androgynous young adult, 1.77 m
    barefoot, standing 2.5 cm up on boot heels; the eyes narrowed to slits, burning cyan in the brim's shadow
  - a cattleman hat worn low (dust-brown felt, a leather band), collar-length dark hair under it, a lock over the left brow
  - a red bandana pulled up over nose and mouth, knotted at the back of the neck, its bib over the chest
  - a dust-brown oilskin duster to the ankle, worn open, a back vent, a fluted shoulder cape, the collar turned up and
    Saint Ada's sun-ring pin on it (Pa's keeper's badge); over a black waistcoat with brass buttons and a linen shirt
  - tan roper gloves with flared gauntlets and a brass concho (SK_FP_Arms's gloves)
  - a gun belt of brass cartridges with a cross-draw holster on the left hip, the revolver's grip forward; Pa's lantern
    clipped to the belt at the right hip, still burning
  - dark wool trousers tucked into tall stovepipe boots with stacked heels
Rig: the UE5 mannequin's 63 deform bones on MakeHuman's joints (Tools/Blender/looter_human.py), every part weighted from
MakeHuman's own skin weights, so the game's mannequin animations drive it through an IK retargeter; 31 swing bones
the game's springs move (swing_layout: the duster's skirt, the cape, the bandana's point, the back hair, the lantern);
and `hat`, which the game tips back while aiming. Built facing -Y, exported turned to face -X: +Y in Unreal, as the
mannequin (face_like_mannequin).

    blender -b --factory-startup --python Art/Models/Characters/Ellis.py -- [--preview] [--glb <path>] [--no-bake]
    ... -- --rig-check      no bake: the swing bones at rest and bent, in clay, into Saved/Concepts/EllisSwing
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
# The silhouette options the user didn't pick (Tools/Blender/ellis_options.py: A gear, B the coat; they picked C, now
# the build itself, 2026-10-15): none unless asked
OPTION = ARGS[ARGS.index('--option') + 1].upper() if '--option' in ARGS and ARGS.index('--option') + 1 < len(ARGS) else ''
OPT = eb.OPTIONS['A']
MACRO = {'gender': 0.5, 'muscle': 0.6, 'weight': 0.42, 'caucasian': 0.7, 'african': 0.15, 'asian': 0.15}
HEIGHT = 1.77            # barefoot, the top of the head (m)
LIFT = 0.025             # the boots' heels and soles raise the body this much
HEM = 0.24               # the duster's hem (m above the ground): the ankle (the user's "C", 2026-10-15; it was mid-calf, 0.36)
BOOT_TOP = 0.49          # the stovepipe boots' tops
WAIST = 1.02             # the trousers' and the waistcoat's waistline
FWD = np.array([0.0, -1.0])
SIM = '--no-sim' not in ARGS        # the duster and cape settle by cloth simulation (cached in Ellis.drape.json)
DRAPE_FILE = os.path.join(HERE, 'Ellis.drape.json') if not OPTION else \
    os.path.join(REPO, 'Saved', 'Concepts', 'EllisOptions', f'Ellis.drape.{OPTION}.json')   # an option's own cloth
SIDE_SEAM = 1.83                     # the duster's side seams, radians round from the front (about 105 degrees)
VENT_TOP = 0.80
COAT_EASE = (0.016, 0.010)           # the duster's ease off the body (m): from the chest down, and more from the hips
CAPE_EASE = 0.010                    # the cape's stand-off from the shoulders and back (m)


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
    'HatBand': lambda n: lk.leather(n, (0.14, 0.08, 0.05), grain=280.0, dust_amt=0.2, dust_height=0.001),
    'Hair': lambda n: lk.hair(n, (0.035, 0.028, 0.025), (0.13, 0.10, 0.085)),
    'Bandana': lambda n: lk.bandana(n, (0.52, 0.06, 0.05), (0.66, 0.17, 0.12)),
    'Duster': lambda n: lk.oilskin(n, OPT['coat']['cloth'][0], OPT['coat']['cloth'][1], hem=HEM),
    'Vest': lambda n: lk.brocade(n, (0.045, 0.045, 0.05), (0.12, 0.115, 0.115)),
    'Shirt': lambda n: lk.linen(n, (0.31, 0.33, 0.36)),
    'Trousers': lambda n: lk.wool(n, (0.19, 0.18, 0.19)),
    # leather grain under the bake's limit (~300/m at ~1000 px/m): at 900-1100/m it aliased into a fuzz that read as
    # suede and fur from a few metres (the gauntlets, 2026-10-15). The toe cap stitched 5.5 cm behind the toe (its tip
    # at y -0.244), the welt 5 mm over the sole
    'Boots': lambda n: lk.leather(n, (0.20, 0.12, 0.075), grain=280.0, dust_amt=0.3, dust_height=0.10, mud=0.0,
                                  boot=(-0.19, 0.0215)),
    'Soles': lambda n: lk.stacked(n, (0.12, 0.08, 0.055)),
    'Gloves': lambda n: lk.leather(n, (0.50, 0.36, 0.23), grain=280.0, dust_amt=0.25, dust_height=0.001,
                                   scuff=(0.68, 0.53, 0.38), rough=(0.5, 0.75)),
    'Belt': lambda n: lk.leather(n, (0.25, 0.145, 0.085), grain=280.0, dust_amt=0.25, dust_height=0.001),
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


BOOT_EASE = 0.012        # the shaft's room over the leg (m)
BOOT_TAPER = 0.08        # how much the shaft may narrow going down (m per m): a stovepipe, nearly straight
BOOT_WALL = 0.005        # the shaft's leather and lining (m), round the inside the trousers tuck into
BOOT_VOXEL = 0.0018      # the voxel size the shaft and the foot fuse at (m)
BOOT_TRIS = 7000         # each detailed boot's triangles (the bake's source; the game mesh keeps LOW['Boots'] of them)


def foot_frame(fig, s):
    """The foot's frame on one side: the ankle, forward along the foot, across it (level)."""
    ankle, ball = fig.sk[f'foot_{s}'][0], fig.sk[f'ball_{s}'][0]
    f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
    return ankle, f, np.array([-f[1], f[0], 0.0])


def hull_radii(hull, dirs):
    """How far out along each unit direction (k, 2) from the origin a convex polygon round it (counterclockwise
    points) reaches."""
    e = np.roll(hull, -1, axis=0) - hull
    den = dirs[:, 0, None] * e[None, :, 1] - dirs[:, 1, None] * e[None, :, 0]
    with np.errstate(divide='ignore', invalid='ignore'):
        t = (hull[None, :, 0] * e[None, :, 1] - hull[None, :, 1] * e[None, :, 0]) / den
        u = (hull[None, :, 0] * dirs[:, 1, None] - hull[None, :, 1] * dirs[:, 0, None]) / den
    ok = (np.abs(den) > 1e-12) & (t > 0.0) & (u >= -1e-9) & (u <= 1.0 + 1e-9)
    return np.where(ok, t, 0.0).max(axis=1)


def boot_shaft(fig, s):
    """A stovepipe's shaft, one closed thick-walled tube: round the leg (its slice's convex hull, with room) from the
    top down to the insole, narrowing at most BOOT_TAPER going down (straight, not shaped to the ankle as a sock is);
    its top level, dipping 3 cm at the front and the back; inside, a well the trousers tuck into."""
    sx = 1.0 if s == 'l' else -1.0
    ankle, f, lat = foot_frame(fig, s)
    # the leg's slices cut through its faces (its vertices lie in rings 4-5 cm apart down the shin)
    ed = {(min(p[k], p[k - 1]), max(p[k], p[k - 1])) for p in fig.faces for k in range(len(p))}
    ed = np.array(sorted(ed))
    a, b = fig.wv[ed[:, 0]], fig.wv[ed[:, 1]]
    keep = (a[:, 0] * sx > 0.0) & (b[:, 0] * sx > 0.0)
    a, b = a[keep], b[keep]

    def slice_at(z):
        da, db = a[:, 2] - z, b[:, 2] - z
        cut = (da * db <= 0.0) & (da != db)
        return (a[cut] + (b[cut] - a[cut]) * (da[cut] / (da[cut] - db[cut]))[:, None])[:, :2]

    z_lo = ankle[2] + 0.03                  # under this the slice takes in the foot: the tube goes straight on down
    zs = np.arange(BOOT_TOP + 0.02, z_lo - 1e-6, -0.01)
    A = np.linspace(-math.pi, math.pi, 56, endpoint=False)
    D = np.stack(gm.radial(A, FWD), axis=1)
    hulls = [hull_2d(slice_at(z)) for z in zs]
    cen = np.array([h.mean(axis=0) for h in hulls])
    for _ in range(6):
        cen[1:-1] = 0.25 * cen[:-2] + 0.5 * cen[1:-1] + 0.25 * cen[2:]
    fit = np.array([hull_radii(h - c, D) for h, c in zip(hulls, cen)])
    rad = fit + BOOT_EASE
    rad = np.maximum(0.75 * rad + 0.25 * rad.mean(axis=1, keepdims=True), fit + 0.008)    # rounder than the leg
    for i in range(1, len(zs)):
        rad[i] = np.maximum(rad[i], rad[i - 1] - BOOT_TAPER * (zs[i - 1] - zs[i]))
    up = zs[::-1]

    def ring(z, inset=0.0):
        """Points round the shaft at heights z (one per angle), inset from its outside."""
        zc = np.clip(z, up[0], up[-1])
        c = np.stack([np.interp(zc, up, cen[::-1, k]) for k in (0, 1)], axis=1)
        r = np.array([np.interp(zc[k], up, rad[::-1, k]) for k in range(len(A))]) - inset
        return np.c_[c + D * r[:, None], z]

    top = BOOT_TOP - 0.03 * np.cos(A) ** 2
    bottom = insole(((ring(np.full(len(A), z_lo)) - ankle) @ f)) + 0.001
    floor = 0.34                             # the well's floor: the trousers end at BOOT_TOP - 0.07
    rows = [ring(floor + (top - floor) * t, BOOT_WALL) for t in np.linspace(0.0, 1.0, 14)]
    rows.append(ring(top + 0.0025, BOOT_WALL * 0.5))
    rows += [ring(top + (bottom - top) * t) for t in np.linspace(0.0, 1.0, 46)]
    R = np.array(rows)
    piece = gm.grid(R[..., 0], R[..., 1], R[..., 2], wrap=True)
    verts, faces, n = list(piece.verts), piece.faces, len(A)
    for row, up_ in ((0, True), (len(R) - 1, False)):
        c = len(verts)
        verts.append(R[row].mean(axis=0))
        for k in range(n):
            a_, b_ = row * n + k, row * n + (k + 1) % n
            faces.append([c, a_, b_] if up_ else [c, b_, a_])
    return gm.Piece(np.array(verts), faces)


def boot_last(fig, s):
    """A boot's foot on its last: rounded-rectangle sections lofted from the heel to a round toe box 2.5 cm past the
    toes, each from the insole up over the foot (MakeHuman's foot and toes, measured, with room), closer together at
    the ends where they round off. Closed; the shaft fuses onto it (build_boots)."""
    ankle, f, lat = foot_frame(fig, s)
    sx = 1.0 if s == 'l' else -1.0
    used = fig.b.used
    feet = fig.wv[used][(fig.w_foot[used] > 0.3) & (fig.wv[used][:, 0] * sx > 0)]
    sf, lf, zf = (feet - ankle) @ f, (feet - ankle) @ lat, feet[:, 2]
    s0, s1 = sf.min() - 0.004, sf.max() + 0.025
    u = np.linspace(0.0, 1.0, 46)
    S = s0 + (s1 - s0) * (0.6 * u + 0.4 * (0.5 - 0.5 * np.cos(math.pi * u)))
    lo, hi, top = np.zeros(len(S)), np.zeros(len(S)), np.zeros(len(S))
    for i, q in enumerate(S):
        near = np.abs(sf - q) < 0.01
        if near.any():
            lo[i], hi[i], top[i] = lf[near].min() - 0.009, lf[near].max() + 0.009, zf[near].max() + 0.010
        else:
            lo[i], hi[i], top[i] = lo[i - 1], hi[i - 1], top[i - 1]
    sb = sf[zf < 0.05].max()                                  # the toes' front
    top = np.maximum(top, (insole(S) + 0.046) * gm.smoothstep(sb - 0.10, sb - 0.06, S))   # a roomy toe box
    # ahead of the ball the box narrows only a little (to the toes' own tips it came to a point)
    s_ball = (fig.sk[f'ball_{s}'][0] - ankle) @ f
    for i in range(1, len(S)):
        if S[i] > s_ball:
            give = 0.15 * (S[i] - S[i - 1])
            lo[i], hi[i] = min(lo[i], lo[i - 1] + give), max(hi[i], hi[i - 1] - give)
    for _ in range(6):
        for a in (lo, hi, top):
            a[1:-1] = 0.25 * a[:-2] + 0.5 * a[1:-1] + 0.25 * a[2:]
    # the toe: a quarter ellipsoid on the sole, a little square in plan, its front curving down to the welt (narrowed
    # in plan alone it came to a beak); the heel round in plan
    toe = np.clip((S - (s1 - 0.045)) / 0.045, 0.0, 1.0)
    heel = np.clip(((s0 + 0.02) - S) / 0.02, 0.0, 1.0)
    plan = np.clip(1.0 - toe ** 2.5, 0.0, 1.0) ** 0.4 * np.sqrt(np.clip(1.0 - heel ** 2, 0.0, 1.0))
    mid, half = (lo + hi) * 0.5, (hi - lo) * 0.5 * np.maximum(plan, 0.08)
    zb = insole(S)
    zt = zb + (top - zb) * np.maximum(np.sqrt(np.clip(1.0 - toe ** 2, 0.0, 1.0)), 0.08)
    phi = np.linspace(0.0, 2 * math.pi, 32, endpoint=False)
    cu, su = np.cos(phi), np.sin(phi)
    lat_u = np.sign(cu) * np.abs(cu) ** (2.0 / 3.2)
    up_u = np.where(su >= 0.0, np.abs(su) ** (2.0 / 3.0), -np.abs(su) ** (2.0 / 9.0))
    R = []
    for i in range(len(S)):
        zc, hz = (zb[i] + zt[i]) * 0.5, (zt[i] - zb[i]) * 0.5
        xy = ankle[None, :2] + f[None, :2] * S[i] + lat[None, :2] * (mid[i] + half[i] * lat_u)[:, None]
        R.append(np.c_[xy, zc + hz * up_u])
    R = np.array(R)
    piece = gm.grid(R[..., 0], R[..., 1], R[..., 2], wrap=True)
    verts, faces, n = list(piece.verts), piece.faces, len(phi)
    for row, push in ((0, -0.004), (len(S) - 1, 0.0)):
        c = len(verts)
        verts.append(R[row].mean(axis=0) + f * push)          # the heel's end domed, not cut flat
        for k in range(n):
            a_, b_ = row * n + k, row * n + (k + 1) % n
            faces.append([c, a_, b_] if row == 0 else [c, b_, a_])
    return gm.Piece(np.array(verts), faces)


def fillet(ob, near, iters=60, step=0.5):
    """Rounds the crease where fused solids meet: the vertices near both (near: 0..1 each) eased toward their
    neighbours' middle (a concave crease fills in)."""
    me = ob.data
    co = gm._object_coords(ob)
    ed = np.empty(len(me.edges) * 2, dtype=np.int64)
    me.edges.foreach_get('vertices', ed)
    a, b = ed[0::2], ed[1::2]
    cnt = np.bincount(a, minlength=len(co)) + np.bincount(b, minlength=len(co))
    k = (step * near)[:, None]
    for _ in range(iters):
        acc = np.stack([np.bincount(a, co[b, j], len(co)) + np.bincount(b, co[a, j], len(co)) for j in range(3)], 1)
        co += k * (acc / np.maximum(cnt, 1)[:, None] - co)
    me.vertices.foreach_set('co', co.astype(np.float32).ravel())
    me.update()


def build_boots(fig):
    """Tall stovepipe boots: a straight shaft (boot_shaft) and the foot on its last (boot_last) fused into one leather
    surface (a voxel remesh) and the crease between them rounded, on a stacked leather sole with a welt and a cuban
    heel. (MakeHuman's own foot reshaped into a toe box kept reading as a bare foot on a sandal, and a last with the
    calf's shell standing in it as a slipper with a sock, 2026-10-15.)"""
    shafts = [gm.mesh(f'_Shaft_{s}', boot_shaft(fig, s), slot('Boots')) for s in 'lr']
    lasts = [gm.mesh(f'_Last_{s}', boot_last(fig, s), slot('Boots')) for s in 'lr']
    shaft_tree, last_tree = gm.bvh_objects(shafts), gm.bvh_objects(lasts)
    bk.select(shafts + lasts, shafts[0])
    bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.name = ob.data.name = 'Boots'
    m = ob.modifiers.new('fuse', 'REMESH')
    m.mode, m.voxel_size, m.adaptivity = 'VOXEL', BOOT_VOXEL, 0.0
    gm.apply_modifiers(ob, keep=())
    # where the shaft meets the foot (near both): rounded into the instep and the heel
    co = gm._object_coords(ob)
    gap = np.zeros(len(co))
    for i, p in enumerate(co):
        if p[2] < 0.25:
            v = Vector(tuple(p))
            gap[i] = max(shaft_tree.find_nearest(v)[3], last_tree.find_nearest(v)[3])
        else:
            gap[i] = 1.0
    fillet(ob, gm.smoothstep(0.022, 0.004, gap))
    m = ob.modifiers.new('reduce', 'DECIMATE')
    m.ratio = min(1.0, 2 * BOOT_TRIS / max(1, sum(len(p.vertices) - 2 for p in ob.data.polygons)))
    gm.apply_modifiers(ob, keep=())
    ob.data.shade_smooth()
    # the stacked soles: under each boot's footprint (its points just over the insole), out 4 mm into a welt
    co = gm._object_coords(ob)
    soles = []
    for s in 'lr':
        sx = 1.0 if s == 'l' else -1.0
        ankle, f, lat = foot_frame(fig, s)
        side = co[co[:, 0] * sx > 0]
        foot_pts = side[side[:, 2] < insole((side - ankle) @ f) + 0.008]
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
        out += 0.004
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
            verts.append(np.array([mid[0], mid[1], zc if row == 0 else 0.0]))
            for i in range(n):
                a_, b_ = row * n + i, row * n + (i + 1) % n
                faces.append([a_, b_, cidx] if row == 0 else [b_, a_, cidx])
                uv += [(0.0, 0.0), (0.01, 0.0), (0.005, 0.01)]
        soles.append(gm.Piece(np.array(verts), faces, uv))
        log(f'boot {s}: toe tip at y {float(foot_pts[:, 1].min()):.3f}, heel at y {float(foot_pts[:, 1].max()):.3f}')
    sole_ob = gm.mesh('Soles', soles[0] + soles[1], slot('Soles'))
    return [ob, sole_ob]


GLOVE_GIRTH = 1.15      # the gloves' girth over the bare hand's (the user, 2026-10-15: the hands read thin and clawed)
CUFF_FLARE = (0.018, 0.015)   # how far the gauntlet's cuff flares from the wrist (m, across and front to back)


def thicken_hand(fig, verts, used):
    """Glove points pushed away from the bones under them by GLOVE_GIRTH: each point's offset from each of its
    skinning bones, square to that bone (girth, not length: fingertips don't grow), blended by its skin weights, so
    the palm thickens and the fingers fill out with no seam between them."""
    W = fig.Wv[used]
    out = np.zeros_like(verts)
    for j, bone in enumerate(fig.names):
        w = W[:, j]
        if not (w > 1e-4).any():
            continue
        h, t = fig.sk[bone][0], fig.sk[bone][1]
        d = gm.unit(t - h)
        q = verts - h
        perp = q - np.outer(q @ d, d)
        out += w[:, None] * perp
    return verts + (GLOVE_GIRTH - 1.0) * out


def build_gloves(fig, sleeves):
    """Tan roper gloves: the hands' own shell, filled out (thicken_hand), and a flared gauntlet cuff over each sleeve
    end with a brass concho."""
    keep = fig.w_hand > 0.45
    piece, used = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), 0.0022, iters=1)
    piece.verts = thicken_hand(fig, piece.verts, used)
    glove = gm.mesh('Gloves', piece, slot('Gloves'))
    tree = gm.bvh_objects([sleeves])
    parts = [glove]
    for s in 'lr':
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        axis = gm.unit(w - e)
        path = np.array([w + axis * (0.012 - 0.115 * t) for t in np.linspace(0, 1, 9)])
        # the cuff's mouth grows with the glove it closes over (GLOVE_GIRTH at the wrist, less up the flare); a roper's
        # gauntlet flares 1.8 cm (3 cm read as a bell, 2026-10-15)
        grow = lambda t: 1.0 + (GLOVE_GIRTH - 1.0) * (1.0 - 0.6 * t)
        rad = np.array([[(0.037 + CUFF_FLARE[0] * t ** 1.3) * grow(t), (0.031 + CUFF_FLARE[1] * t ** 1.3) * grow(t)]
                        for t in np.linspace(0, 1, 9)])
        pc = gm.tube(path, rad, n=28, up=(0.0, -1.0, 0.0))
        pc.verts = gm.clear(pc.verts, tree, 0.004, 0.06)
        cuff = gm.mesh(f'Gauntlet_{s}', pc, slot('Gloves'))
        parts.append(cuff)
        # the concho on the outside of the cuff
        out = gm.unit(np.array([1.0 if s == 'l' else -1.0, 0.0, 0.25]))
        out = gm.unit(out - axis * (out @ axis))
        mid = w - axis * 0.075
        hit = tree.ray_cast(Vector(mid + out * 0.2), Vector(-out), 0.4)
        tc = (0.075 + 0.012) / 0.115                             # the concho's place along the cuff
        swell = (0.034 + 0.5 * sum(CUFF_FLARE) * tc ** 1.3) * (grow(tc) - 1.0)   # how much the cuff grew there
        # the cuff's flare there came in from 2.8 cm to CUFF_FLARE: the concho follows it in
        swell -= (0.028 - 0.5 * sum(CUFF_FLARE)) * tc ** 1.3
        at = (np.array(hit[0]) if hit[0] is not None else mid + out * 0.05) + out * (0.012 + swell)
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
    # smoothed well: cloth over the chest, not the body's own shape (the user: the waistcoat "as cloth", 2026-10-15)
    vest, _ = gm.shell(fig.wv, fig.faces, fig.fuv, fig.face_mask(keep), 0.0075, iters=14)
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
    # simulated, the skirt starts fuller than it hangs: the spare cloth is what falls into folds. The ease and the
    # fuller skirt: the user found the coat hugging him (2026-10-15): it stands off the chest and the hips now, from
    # under the collar down (the collar and the cape's top, fitted to the coat's top, keep their size)
    flare_amt = 0.13 if SIM else 0.09
    ease = lambda v: COAT_EASE[0] * gm.smoothstep(0.02, 0.18, v) + COAT_EASE[1] * gm.smoothstep(0.30, 0.50, v)
    flare = lambda th, v: ease(v) + flare_amt * gm.smoothstep(0.42, 1.0, v) ** 1.3 * (0.55 + 0.45 * np.abs(np.sin(th)))
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
        # the vent: built, its edges overlap a little (the left flap over the right); simulated, its edges start
        # together and the cloth falls welded shut there, lapped afterwards (lap_vent). (Started apart, the flaps
        # fell into each other and tangled, one swept across the other and kicked out at the back, 2026-10-15.)
        if z < vent_top and not SIM:
            k = gm.smoothstep(vent_top, vent_top - 0.08, z)
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
    vent = [(bm.verts[j * n_t + half - 1], bm.verts[j * n_t + half]) for j in range(n_z) if zs[j] < vent_top]
    for a, b in pairs:
        mid = (a.co + b.co) * 0.5
        a.co = b.co = mid
    bmesh.ops.remove_doubles(bm, verts=[v for p in pairs for v in p], dist=1e-5)
    bm.verts.index_update()
    # the vent's edges below its top (the left flap's, the right's), for the simulation's weld and lap_vent
    ob['vent'] = [v.index for p in vent for v in p]
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


IDLE_ARM = 10.0         # degrees the game's idle arms hang off the body: the cape is draped over the arms held so


def arm_turns(fig):
    """Per side (+1 left, -1 right): the shoulder joint and the turn (3x3) taking the bind pose's upper arm (about 49
    degrees below level) down to the game's idle, IDLE_ARM off the body."""
    out = {}
    for s, sx in (('l', 1.0), ('r', -1.0)):
        h, t = fig.sk[f'upperarm_{s}'][0], fig.sk[f'upperarm_{s}'][1]
        a = math.radians(IDLE_ARM)
        to = Vector((sx * math.sin(a), -0.04, -math.cos(a))).normalized()
        out[sx] = (h, np.array(Vector(tuple(gm.unit(t - h))).rotation_difference(to).to_matrix()))
    return out


def arm_share(fig, W, sx):
    """Each row's weight on one side's arm, the upper arm down (W: rows of fig.names' columns)."""
    s = 'l' if sx > 0 else 'r'
    cols = [j for j, n in enumerate(fig.names) if n.endswith('_' + s) and
            n.split('_')[0] in ('upperarm', 'lowerarm', 'hand', 'thumb', 'index', 'middle', 'ring', 'pinky')]
    return W[:, cols].sum(1)


def arms_down(fig, P, W, back=False):
    """Points posed with the arms let down to the game's idle (or, back, the other way), each by its arm weights."""
    out = P.copy()
    for sx, (h, R) in arm_turns(fig).items():
        w = arm_share(fig, W, sx)[:, None]
        turned = (P - h) @ (R if back else R.T) + h
        out += w * (turned - P)
    return out


def build_cape(fig, body, duster, sleeves, name='Cape', drop=0.13, over=(), top_drop=0.0):
    """The shoulder cape: a bell hung from the collar over the shoulders and the tops of the arms, open at the front,
    flaring in flutes. Draped over the arms as the game holds them (let down to IDLE_ARM), then carried back up into
    the bind pose by how much each point rides its upper arm (its 'ArmFollow', which rig() weights to the upper arm):
    draped over the bind pose's raised arms, it stood out like wings and fins when they came down (2026-10-15).
    drop: how far below the shoulders it reaches; over: what else it hangs over (option B's second tier)."""
    neck = fig.neck
    sz = fig.shoulder_z
    used = fig.b.used
    posed_body = arms_down(fig, fig.wv[used], fig.Wv[used])
    sl = gm.object_points(sleeves)
    posed_sleeves = arms_down(fig, sl, fig.b.weights_at(sl)[1])
    hung = []
    for o in over:
        # a cape under this one, let down with the arms as its ArmFollow says
        p = gm.object_points(o)
        a = np.zeros(len(p), dtype=np.float32)
        if 'ArmFollow' in o.data.attributes:
            o.data.attributes['ArmFollow'].data.foreach_get('value', a)
        for sx, (h, Rm) in arm_turns(fig).items():
            sel = (p[:, 0] * sx) >= 0.0
            p[sel] += a[sel, None] * (((p[sel] - h) @ Rm.T + h) - p[sel])
        hung.append(p)
    under_pts = np.concatenate([posed_body[posed_body[:, 2] > sz - 0.25], gm.object_points(duster), posed_sleeves] + hung)
    th = np.linspace(math.radians(26), math.radians(334), 96)
    top = lambda t: neck[2] - 0.006 - top_drop
    # a pointed back, the hem dropping 7 cm to the middle of the back (the user's "C": a sharper outline, 2026-10-15)
    point = lambda t: 0.07 * max(0.0, -math.cos(t)) ** 3
    bot = lambda t: sz - drop - 0.035 * abs(math.sin(t)) + 0.012 * math.sin(t * 7.0) - point(t)
    X, Y, Z, R = gm.drape(under_pts, neck[:2], FWD, th, top, bot, 24, 0.026, hb=math.radians(7), zb=0.012, smooth=(4, 3))
    v = np.linspace(0, 1, Z.shape[0])[:, None]
    # flutes of uneven depth and spacing (frequencies that never line up), deepening toward the hem; standing off the
    # shoulders and back from under the collar down, flaring at the hem (looser: the user, 2026-10-15), less so at the
    # front corners (they read very full)
    rng = np.random.default_rng(5)
    ph = rng.random(6) * 6.28
    T = th[None, :]
    corner = gm.smoothstep(0.0, 0.6, np.minimum(T - th[0], th[-1] - T))
    # broader, squarer shoulders (the cape stands out 2.2 cm more over them; "C")
    shoulders = 0.022 * gm.smoothstep(0.15, 0.55, v) * np.sin(T) ** 2
    flute = shoulders + CAPE_EASE * gm.smoothstep(0.0, 0.4, v) + (0.024 + 0.016 * corner) * v ** 1.4 \
        + v ** 1.1 * (0.011 * np.sin(T * 7.0 + ph[0] + 0.8 * np.sin(v * 3 + ph[1]))
                      + 0.008 * np.sin(T * 11.3 + ph[2] + 0.6 * np.sin(v * 2 + ph[3]))
                      + 0.004 * np.sin(T * 19.7 + ph[4] + v * 4)) \
        + 0.004 * v * np.sin(T * 31.0 + ph[5] + v * 6)
    dx, dy = gm.radial(th[None, :], FWD)
    piece = gm.grid(X + flute * dx, Y + flute * dy, Z)
    # how much each point rides its upper arm: the arm's share of the skin nearest it (arms down), smoothed over the
    # cloth (the nearest skin jumps from arm to chest across the armpit), nothing at the collar
    from mathutils.bvhtree import BVHTree
    faces = fig.faces
    tris = [(f[0], f[k], f[k + 1]) for f in faces for k in range(1, len(f) - 1)]
    remap = -np.ones(len(fig.wv), dtype=np.int64)
    remap[used] = np.arange(len(used))
    tris_u = [tuple(int(remap[i]) for i in t) for t in tris]
    tree = BVHTree.FromPolygons([tuple(map(float, p)) for p in posed_body], tris_u, all_triangles=True)
    Wu = fig.Wv[used]
    share = {sx: arm_share(fig, Wu, sx) for sx in (1.0, -1.0)}
    P = piece.verts
    a = np.zeros(len(P))
    for i, p in enumerate(P):
        loc, _n, fi, _d = tree.find_nearest(Vector(tuple(p)))
        if loc is None:
            continue
        tri = tris_u[fi]
        bc = hu.barycentric(np.array(loc), *(posed_body[j] for j in tri))
        a[i] = sum(bc[k] * share[1.0 if p[0] >= 0.0 else -1.0][tri[k]] for k in range(3))
    a = a.reshape(Z.shape)
    for _ in range(8):
        a = 0.25 * np.concatenate([a[:, :1], a[:, :-1]], 1) + 0.5 * a + 0.25 * np.concatenate([a[:, 1:], a[:, -1:]], 1)
        a = 0.25 * np.concatenate([a[:1], a[:-1]], 0) + 0.5 * a + 0.25 * np.concatenate([a[1:], a[-1:]], 0)
    a = (a * gm.smoothstep(0.0, 0.3, v)).ravel()
    # back up into the bind pose by that share
    for sx, (h, Rm) in arm_turns(fig).items():
        sel = (P[:, 0] * sx) >= 0.0
        P[sel] += a[sel, None] * (((P[sel] - h) @ Rm + h) - P[sel])
    piece.verts = P
    ob = gm.mesh(name, piece, slot('Duster'))
    ob.data.attributes.new('ArmFollow', 'FLOAT', 'POINT').data.foreach_set('value', a.astype(np.float32))
    log(f'{name}: draped over the arms let down {IDLE_ARM:.0f} degrees off the body; rides the arms up to {a.max():.2f}')
    return ob, R[0].max()


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
            # the ends by the throat stand a little taller and splay out a little into points (twice this and the flap
            # with the pin jutted out like a box, 2026-10-15); the top edge rolls outward
            end = float(gm.smoothstep(math.radians(62), math.radians(30), min(a, 2 * math.pi - a)))
            # soft, worn oilskin: the top edge wavers, the stand ripples a little, a crease where it turns up
            hgt = 0.062 + 0.058 * back + 0.006 * end + 0.004 * math.sin(a * 5.3 + 1.1) + 0.003 * math.sin(a * 13.7)
            r = (top_r * (0.89 + 0.08 * t + 0.13 * t ** 3) + 0.004 * math.sin(a * 9 + t * 2) + 0.0025 * math.sin(a * 17.3 + t * 4)
                 - 0.010 * (1 - back) * t + 0.007 * end * t ** 2 - 0.004 * math.exp(-((t - 0.18) / 0.08) ** 2))
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

BRIM_REACH = 0.88       # the brim's reach past the crown, of the portrait's


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
    # the brim reaches a little less far past the crown than the portrait's (the user, 2026-10-15: "slightly smaller,
    # not super small"): each spoke, from its root at the crown (EllisBlender's first ring of 96) out, shortened, so
    # the curl keeps its shape
    brim = next(o for o in parts if o.name.startswith('HatBrim'))
    co = gm._object_coords(brim)
    assert len(co) % 96 == 0, 'EllisBlender.build_hat\'s brim: rings of 96 points expected'
    co = co.reshape(-1, 96, 3)
    co = co[:1] + (co - co[:1]) * BRIM_REACH
    # and a sharper outline ("C", 2026-10-15): the brim's sides curled up harder, the crown's long crease deeper
    side = np.abs(np.sin(2 * math.pi * np.arange(96) / 96))[None, :]
    dz = co[..., 2] - co[:1, :, 2]
    co[..., 2] = co[:1, :, 2] + np.where(dz > 0, dz * (1.0 + 0.9 * side ** 2), dz)
    brim.data.vertices.foreach_set('co', co.reshape(-1).astype(np.float32))
    brim.data.update()
    crown = next(o for o in parts if o.name.startswith('HatCrown'))
    cc = gm._object_coords(crown)
    top_z = cc[:, 2].max()
    cx = (cc[:, 0].max() + cc[:, 0].min()) * 0.5
    cc[:, 2] -= 0.012 * np.exp(-((cc[:, 0] - cx) / 0.022) ** 2) * gm.smoothstep(top_z - 0.06, top_z - 0.005, cc[:, 2])
    crown.data.vertices.foreach_set('co', cc.astype(np.float32).ravel())
    crown.data.update()
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
# the game's: twice the locks it had and narrower (6-7 cm wide, 44 of them, they read as flat dark slabs, 2026-10-15)
HAIR_LOW = dict(layers=((46, 0.003, 0.048, 0.0), (34, 0.009, 0.042, 0.05)), per=1, steps=7, locks=3)


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
        # the lock's sideways direction carried along it: where a lock runs straight out from the head (the brow
        # lock to the temple) the cross product with "out" vanishes and flipped, and the ribbon twisted into a hook
        out0 = gm.unit(st - head_c)
        side = np.cross(tang, out0)
        for i in range(len(st)):
            if np.linalg.norm(side[i]) < 0.3 and i > 0:
                side[i] = side[i - 1]
            side[i] = gm.unit(side[i])
            if i > 0 and side[i] @ side[i - 1] < 0.0:
                side[i] = -side[i]
        out = gm.unit(np.cross(side, tang))
        out = np.where(((out * out0).sum(1) < 0.0)[:, None], -out, out)
        # tapering to a blunt clump's end (to a needle point the locks read as flat slabs with spikes, 2026-10-15)
        width = w * (1.0 - 0.8 * t) ** 1.3 + 0.0008
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
    # keep the lantern off the thigh, fount to cap (its middle alone left the fount 2.6 cm into the thigh, 2026-10-15):
    # its 4.3 cm radius and 1.7 cm more
    for _ in range(10):
        worst = None
        for h in (0.0, 0.035, 0.07, 0.105, 0.14):
            q = Vector(tuple(c + np.array([0.0, 0.0, h])))
            hit = tree.find_nearest(q, 0.25)
            if hit[0] is not None:
                d = (q - hit[0]).dot(hit[1])
                if worst is None or d < worst[0]:
                    worst = (d, np.array(hit[1]))
        if worst is None or worst[0] > 0.059:
            break
        c = c + worst[1] * (0.06 - worst[0])
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
    parts.append(gm.mesh('LanternWire', wires, slot('Iron')))
    # the hook out from the belt, its own piece: it stays on the hip (weights()), the lantern swings from its end
    hook = np.array([top + n * 0.004, top + n * 0.03 - up * 0.01, c + [0, 0, 0.188]])
    parts.append(gm.mesh('BeltHook', gm.tube(hook, np.array([0.003] * 3), n=6), slot('Iron')))
    flame = lm_ellipsoid('LanternFlame', c + np.array([0, 0, 0.062]), (0.008, 0.008, 0.016), (0, 0, 1), 10, 8)
    flame.data.materials.append(slot('LanternGlow'))
    parts.append(flame)
    fig.lantern = c
    # lantern_01's head: the bail's top on the hook's end, right above the lantern, so it rests where it hangs (from
    # the hook's belt end the springs' gravity swung it 6 cm back into the thigh, 2026-10-15)
    fig.lantern_hook = c + np.array([0.0, 0.0, 0.188])
    fig.belt_hook = (hook[0], hook[-1])
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

# The game mesh (about 35k triangles, the user's call of 2026-10-15; it was 20k, Borderlands' budget, and read coarse):
# each detailed part's share of its triangles kept by Blender's Decimate (collapse); 1 keeps a part as it is. The
# triangles go where the silhouette and the close-ups need them: face, hands, collar, the brim, the coat's edges and hem,
# the boots; small hard parts (gun, holster, lantern, pin) whole: reduced, they shredded. Its looks, folds and creases
# are baked from the detailed parts (looter_bake, sources). The hair is built again with fewer locks (HAIR_LOW).
LOW = {'Ellis': 0.30, 'Eye': 0.30, 'Trousers': 0.42, 'Boots': 0.18, 'Soles': 0.7, 'Vest': 0.40, 'Shirt': 0.4,
       'VestButton': 0.6, 'Duster': 0.15, 'Sleeves': 0.45, 'Cape': 0.18, 'Collar': 0.55, 'SunPin': 1.0, 'Gloves': 0.36,
       'Gauntlet': 0.55, 'Concho': 1.0, 'HatCrown': 0.12, 'HatBrim': 0.20, 'HatBand': 0.4, 'Bandana': 0.20,
       'BandanaKnot': 0.6, 'Belt': 0.45, 'BeltLoops': 0.4, 'Cartridges': 0.5, 'Bullets': 0.4, 'Buckle': 1.0,
       'HolsterSkirt': 1.0, 'Holster': 1.0, 'HolsterLoop': 1.0, 'Revolver': 1.0, 'LanternWire': 0.6, 'Lantern': 1.0,
       'BeltHook': 1.0}
# Cloth whose open edges (fronts, hems, the brim's rim) Decimate must not chew: a band this wide along them is spared
EDGE_KEEP = {'Duster': 0.012, 'Cape': 0.012, 'Collar': 0.010, 'Bandana': 0.010, 'HatBrim': 0.010}


def low_ratio(name):
    key = name.split('.')[0]
    for k in sorted(LOW, key=len, reverse=True):
        if key.startswith(k):
            return LOW[k]
    return 1.0


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
    """The faces Decimate turned over (in tight folds) turned back to face as the detailed part does there, and its
    slivers of no area dissolved: Unreal draws a face from its front only, so each was a hole (the waistcoat's "black
    holes", the boots' toes, the duster's back; 2026-10-15)."""
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
        # and against its neighbours too (where the detailed cloth folds under itself, as the vent's strip, its nearest
        # face can be the fold's underside)
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
        if ob.name.startswith('Shirt'):
            # only the shirt in the waistcoat's V shows: what lies under the waistcoat goes (bent, it came through)
            bm = bmesh.new()
            bm.from_mesh(low.data)
            hw = lambda z: 0.065 * float(gm.smoothstep(1.27, 1.47, z)) + 0.025
            hidden = [f for f in bm.faces if (lambda q: q.z < 1.47 and abs(q.x) > hw(q.z))(f.calc_center_median())]
            bmesh.ops.delete(bm, geom=hidden, context='FACES')
            bm.to_mesh(low.data)
            bm.free()
        ratio = low_ratio(ob.name)
        if ratio < 1.0:
            m = low.modifiers.new('decimate', 'DECIMATE')
            m.ratio = ratio
            co = gm._object_coords(low)
            spare = np.zeros(len(co), dtype=bool)
            if ob.name == 'Ellis':
                # the eyes and lids keep their detail
                d = np.minimum(np.linalg.norm(co - fig.eyes[0], axis=1), np.linalg.norm(co - fig.eyes[1], axis=1))
                spare = d < 0.03
            band = EDGE_KEEP.get(ob.name.split('.')[0])
            if band:
                spare = near_open_edge(low, co, band)
            if spare.any():
                # (Decimate collapses only what its group weights: inverted, the group is the part it spares)
                g = low.vertex_groups.new(name='keep')
                for i in np.nonzero(spare)[0]:
                    g.add([int(i)], 1.0, 'REPLACE')
                m.vertex_group = 'keep'
                m.invert_vertex_group = True
                m.vertex_group_factor = 1.0
            gm.apply_modifiers(low, keep=())
            if 'keep' in low.vertex_groups:
                low.vertex_groups.remove(low.vertex_groups['keep'])
            unflip(low, ob)
        lows.append(low)
    hair = build_hair(fig, hair_under, name='Hair_low', **HAIR_LOW)
    lows.append(hair)
    # reduced layers can cut through one another where the detailed ones only just cleared: each outer part is pushed
    # back outside the reduced parts under it
    by = lambda *names: [o for o in lows if o.name.split('.')[0].startswith(names)]
    for outer, inner in (('Vest', ('Shirt',)), ('Duster', ('Vest', 'Shirt', 'Belt', 'Cartridges', 'Bullets', 'Trousers')),
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
    # below the chin the bandana stands a finger's width (1 cm) off the neck's skin, the face mask above it as it was:
    # it shares the skin's weights there, so a turned head keeps the gap (2026-10-15: it had cut through the neck)
    chin = fig.eye_z - 0.10
    tree = gm.bvh_objects(by('Ellis'))
    for ob in by('Bandana'):
        co = gm._object_coords(ob)
        gap = 0.003 + 0.007 * gm.smoothstep(chin + 0.02, chin, co[:, 2])
        for k, p in enumerate(co):
            loc, nrm, _i, _d = tree.find_nearest(Vector(tuple(p)), 0.03)
            if loc is not None and (Vector(tuple(p)) - loc).dot(nrm) < gap[k]:
                co[k] = np.array(loc + nrm * float(gap[k]))
        ob.data.vertices.foreach_set('co', co.astype(np.float32).ravel())
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
        # the skin's own weights where it lies (the user saw it cut through the neck as the head turned, 2026-10-15):
        # over the nose, mouth and chin the head; round the neck head, neck_02, neck_01 going down, the knot too
        return fig.b.weights_at(P)[1]
    if name.startswith(('Collar', 'SunPin')):
        t = gm.smoothstep(fig.neck[2] - 0.03, fig.neck[2] + 0.1, P[:, 2])
        return rigid_w(fig, n, {'spine_05': 1.0 - 0.5 * t, 'neck_01': 0.5 * t})
    if name.startswith('Cape'):
        # on the shoulders: spine_05 and the collarbones (what rides the arms is its 'ArmFollow', in swing_weights)
        side = gm.smoothstep(-0.08, 0.08, P[:, 0])
        return hu.blend(fig.names, n, {'spine_05': 0.5, 'clavicle_l': 0.5 * side, 'clavicle_r': 0.5 * (1 - side)})
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
    if name.startswith('BeltHook'):
        # from the belt (the hips) out to its end, where lantern_01 hangs (on thigh_r): the thigh's by its end
        a, b = fig.belt_hook
        t = 0.4 + 0.6 * np.clip(((P - a) @ (b - a)) / max((b - a) @ (b - a), 1e-9), 0.0, 1.0)
        return rigid_w(fig, n, {'pelvis': 1.0 - t, 'thigh_r': t})
    if name.startswith(('Belt', 'Cartridges', 'Bullets', 'Buckle')):
        return rigid_w(fig, n, {'pelvis': 0.85, 'spine_01': 0.15})
    if name.startswith(('Holster', 'Revolver')):
        # on the belt it rides the hips; down the pouch more and more the thigh it hangs against (rigid at 35 % thigh,
        # a stride put the thigh 5 cm through it)
        t = 0.3 + 0.55 * gm.smoothstep(0.96, 0.86, P[:, 2])
        return rigid_w(fig, n, {'pelvis': 1.0 - t, 'thigh_l': t})
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


# ------------------------------------------------------------------------------- swing bones (the game's springs)

# The game swings these after the retarget (Player/Animation/BodySwing: springs, no cloth simulation; the user found
# Ellis "stiff", 2026-10-15): the duster's skirt, the shoulder cape, the bandana's point, the back hair, Pa's lantern.
# Names and parents are the C++'s contract, found by name: <chain>_01 at the root and on down the cloth, a bone's head at
# its joint and its tail at the next one's, the last tail at the cloth's end. Each bone's Z axis points out of the cloth.
# A side's chains, at their share of its arc from the front edge (0) round to the back's middle (1); the back one (None)
# straight behind the knee, so a stride's back leg pushes it back (at 5/6, by the vent, it slid aside and the knee came
# out through the coat once the coat reached the ankle, 2026-10-15; in a full stride the shin still can: the springs
# keep points off the limbs, not the cloth between them, and a point slides round a shin swung back under it):
SKIRT_CHAINS = (('f', 1 / 6), ('s', 1 / 2), ('b', None))
BACK_BLEND = 0.2         # across the back the two back chains hand over within this share either side of the middle
CAPE_CHAINS = (('f', 0.25), ('b', 0.75))
SKIRT_TOP = 0.91         # the skirt's own weights start here (above, the duster's blend into the torso's), its chains below
# The bib's chain hangs from the neck, not the head (the contract said head): on the head it swung 2 cm out through the
# collar when the head turned or tilted onto the stock; from neck_02 the bib keeps inside the collar's opening while the
# mask turns with the face (rig_check's head tests, 2026-10-15)
BANDANA_PARENT = 'neck_02'


def round_angle(P, centre):
    """Angles of points round a vertical axis at centre (radians: 0 the front, + toward the figure's left)."""
    f, l = gm.frame2d(FWD)
    rel = np.atleast_2d(P)[:, :2] - centre
    return np.arctan2(rel @ l, rel @ f)


def on_surface(tree, centre, ang, z):
    """The outermost point of a garment (tree) at angle ang round centre and height z: a ray from outside, inward
    (through a gap between locks of hair, the nearest ray either side that hits, turned back onto ang)."""
    ex, ey = gm.radial(np.float64(ang), FWD)
    for d in np.radians((0, 2, -2, 4, -4, 6, -6, 9, -9, 12, -12, 16, -16, 20, -20)):
        dx, dy = gm.radial(np.float64(ang + d), FWD)
        hit = tree.ray_cast(Vector((centre[0] + dx, centre[1] + dy, z)), Vector((-dx, -dy, 0.0)), 1.0)
        if hit[0] is not None:
            r = math.hypot(hit[0].x - centre[0], hit[0].y - centre[1])
            return np.array([centre[0] + ex * r, centre[1] + ey * r, hit[0].z])
    raise ValueError(f'no garment at {math.degrees(ang):.0f} degrees, {z:.3f} m')


def front_edge(co, edges, centre, z, side):
    """How far round from the front (radians, on side +1 left / -1 right) a garment starts at height z: its edges'
    crossings of that height nearest the front. (Edges, not vertices: the game mesh is too sparse to slice by points.)"""
    a, b = co[edges[:, 0]], co[edges[:, 1]]
    da, db = a[:, 2] - z, b[:, 2] - z
    cut = (da * db <= 0.0) & (da != db)
    p = a[cut] + (b[cut] - a[cut]) * (da[cut] / (da[cut] - db[cut]))[:, None]
    ang = round_angle(p, centre) * side
    return float(ang[ang > 0].min())


def hem_point(co, centre, ang, width=math.radians(6)):
    """A garment's hem at angle ang: its lowest point within width of it, turned onto ang."""
    near = co[np.abs((round_angle(co, centre) - ang + math.pi) % (2 * math.pi) - math.pi) < width]
    p = near[np.argmin(near[:, 2])]
    r = math.hypot(p[0] - centre[0], p[1] - centre[1])
    ex, ey = gm.radial(np.float64(ang), FWD)
    return np.array([centre[0] + ex * r, centre[1] + ey * r, p[2]])


def mesh_arrays(ob):
    ed = np.empty(len(ob.data.edges) * 2, dtype=np.int64)
    ob.data.edges.foreach_get('vertices', ed)
    return gm._object_coords(ob), ed.reshape(-1, 2)


def swing_layout(fig, parts):
    """The swing chains placed on the built game parts: [{name, parent, part, side, centre, joints}], joints the
    bones' heads and then the last one's tail."""
    by = {o.name.split('.')[0]: o for o in parts}
    sk = fig.sk
    chains = []

    def chain(name, parent, part, side, centre, joints):
        chains.append(dict(name=name, parent=parent, part=part, side=side, centre=np.asarray(centre)[:2],
                           joints=np.array(joints)))

    # the skirt round the coat's own axis: from the belt line, mid-thigh and the knee down to the hem
    co, ed = mesh_arrays(by['Duster'])
    tree = gm.bvh_objects([by['Duster']])
    knee = (sk['calf_l'][0][2] + sk['calf_r'][0][2]) * 0.5
    mid_thigh = ((sk['thigh_l'][0][2] + sk['thigh_r'][0][2]) * 0.5 + knee) * 0.5
    for side, s in ((1, 'l'), (-1, 'r')):
        heights = (float(belt_height(side * math.pi / 2)), mid_thigh, knee, HEM + 0.03)
        edge = [front_edge(co, ed, fig.centre, z, side) for z in heights]
        behind = float(round_angle(np.array([sk[f'calf_{s}'][0][0], fig.centre[1] + 0.18]), fig.centre)[0])
        for key, u in SKIRT_CHAINS:
            a = [side * (e + u * (math.pi - e)) for e in edge] if u is not None else [behind] * 4
            joints = [on_surface(tree, fig.centre, a[0], float(belt_height(a[0])))]
            joints += [on_surface(tree, fig.centre, a[k], heights[k]) for k in (1, 2)]
            joints.append(hem_point(co, fig.centre, a[3]))
            chain(f'skirt_{key}{s}', 'pelvis', 'Duster', side, fig.centre, joints)
    # the cape round the neck: from the shoulder line down to its hem (it hangs at fixed angles: the drape's columns)
    co, ed = mesh_arrays(by['Cape'])
    tree = gm.bvh_objects([by['Cape']])
    centre = fig.neck[:2]
    for side, s in ((1, 'l'), (-1, 'r')):
        e = front_edge(co, ed, centre, fig.shoulder_z, side)
        for key, u in CAPE_CHAINS:
            a = side * (e + u * (math.pi - e))
            hem = hem_point(co, centre, a)
            joints = [on_surface(tree, centre, a, z) for z in (fig.shoulder_z, (fig.shoulder_z + hem[2]) * 0.5)]
            chain(f'cape_{key}{s}', 'spine_05', 'Cape', side, centre, joints + [hem])
    # the bandana's point: down the middle of its bib from under the chin to the point
    co, _ = mesh_arrays(by['Bandana'])
    tree = gm.bvh_objects([by['Bandana']])
    mid = (fig.eyes[0] + fig.eyes[1]) * 0.5
    centre = mid[:2] - FWD * 0.085 * fig.b.scale            # build_bandana's axis
    tip = co[np.argmin(co[:, 2])]
    chin = fig.eye_z - 0.10
    joints = [on_surface(tree, centre, 0.0, z) for z in (chin, (chin + tip[2]) * 0.5)] + [tip]
    chain('bandana', BANDANA_PARENT, 'Bandana', 0, centre, joints)
    # the back hair: from where it falls free of the skull (a quarter of the way from the back of the head's bulge down
    # to the nape's hollow, which is inside the turned-up collar: there it hardly showed) to its ends
    co, _ = mesh_arrays(by['Hair'])
    tree = gm.bvh_objects([by['Hair']])
    centre = sk['head'][0][:2]
    wv = fig.wv[fig.b.used]
    back = wv[(np.abs(wv[:, 0]) < 0.03) & (wv[:, 1] > centre[1]) & (wv[:, 2] > 1.5) & (wv[:, 2] < 1.85)]
    zs = np.array([z for z in np.arange(1.52, 1.84, 0.01) if (np.abs(back[:, 2] - z) < 0.01).any()])
    y = np.array([back[np.abs(back[:, 2] - z) < 0.01, 1].max() for z in zs])
    occiput = zs[np.argmax(y)]
    nape = zs[zs < occiput][np.argmin(y[zs < occiput])]
    top = occiput + (nape - occiput) * 0.25
    ends = hem_point(co, centre, math.pi, math.radians(20))
    chain('hair', 'head', 'Hair', 0, centre, [on_surface(tree, centre, math.pi, z) for z in (top, (top + ends[2]) * 0.5)] + [ends])
    # Pa's lantern swings from its hook on the belt
    chain('lantern', 'thigh_r', 'Lantern', 0, fig.centre, [fig.lantern_hook, fig.lantern])
    for c in chains:
        j = c['joints']
        log(f'swing {c["name"]}: ' + ' > '.join(f'({p[0] * 100:.1f}, {p[1] * 100:.1f}, {p[2] * 100:.1f})' for p in j) + ' cm')
    return chains


def swing_bones(chains):
    """(name, head, tail, parent, chain) of every swing bone, in order."""
    out = []
    for c in chains:
        j = c['joints']
        n = len(j) - 1
        for k in range(n):
            name = f'{c["name"]}_{k + 1:02d}'
            out.append((name, j[k], j[k + 1], c['parent'] if k == 0 else f'{c["name"]}_{k:02d}', c))
    return out


def add_swing_bones(arm, chains):
    """The swing bones on the armature (deforming, as the mannequin's: unconnected, their Z axes out of the cloth).
    Returns their names in order."""
    bk.select([arm], arm)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones
    bones = swing_bones(chains)
    for name, h, t, parent, c in bones:
        e = edit.new(name)
        e.head, e.tail = Vector(h), Vector(t)
        out = np.array([h[0] - c['centre'][0], h[1] - c['centre'][1], 0.0])
        e.align_roll(Vector(tuple(gm.unit(out))))
        e.use_connect = False
        e.use_deform = True
        e.parent = edit[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    return [b[0] for b in bones]


def soft_top4(W):
    """Four influences a vertex without seams: the fifth-largest weight comes off the top four (a bone fades out of a
    vertex as a fifth overtakes it, where a plain top four drops it at once: a kink in the cloth), then normalised."""
    W = np.asarray(W, dtype=np.float64).copy()
    if W.shape[1] > 4:
        fifth = np.sort(W, axis=1)[:, -5][:, None]
        W = np.where(W > fifth, W - fifth, 0.0)
    s = W.sum(axis=1, keepdims=True)
    return np.where(s > 1e-9, W / np.maximum(s, 1e-9), 0.0)


def along(z, zs):
    """Shares of a chain's bones down it (n, bones): each holds from its head to the next joint, then hands over to
    the next bone smoothly along that one's length (a smoothstep: no kink where the hand-over starts or ends)."""
    out = np.zeros((len(z), len(zs) - 1))
    out[:, 0] = 1.0
    for k in range(1, len(zs) - 1):
        moved = out[:, k - 1] * gm.smoothstep(zs[k], zs[k + 1], z)
        out[:, k - 1] -= moved
        out[:, k] += moved
    return out


def ring_weights(P, chains):
    """Each vertex's shares of the chains round a garment (n, chains): between the two nearest, the front edge's way
    all to the first, across the back's middle from each side's last into the other's. (The duster's vent doesn't
    part: its two edges are joined by a strip of narrow faces to the hem, which would stretch into a web.)"""
    n = len(P)
    out = np.zeros((n, len(chains)))
    centre = chains[0]['centre']
    ang = round_angle(P, centre)
    side = np.where(ang >= 0.0, 1.0, -1.0)
    for s in (1.0, -1.0):
        sel = side == s
        if not sel.any():
            continue
        own = [i for i, c in enumerate(chains) if c['side'] == s]
        other = [i for i, c in enumerate(chains) if c['side'] == -s][-1]
        th = ang[sel] * s
        th = np.where(th < 0.0, th + 2 * math.pi, th)          # round from the front edge on this side, past the back
        z = P[sel, 2]

        def at(i):
            j = chains[i]['joints']
            a = round_angle(j, centre) * chains[i]['side']
            return np.interp(z, j[::-1, 2], a[::-1])
        knots = np.stack([at(i) for i in own] + [2 * math.pi - at(other)], 1)
        w = np.zeros((int(sel.sum()), knots.shape[1]))
        w[:, 0] = th <= knots[:, 0]
        for i in range(knots.shape[1] - 1):
            lo, hi = knots[:, i], knots[:, i + 1]
            inside = (th > lo) & (th <= hi)
            t = np.clip((th - lo) / np.maximum(hi - lo, 1e-6), 0.0, 1.0)
            if i == knots.shape[1] - 2:
                # across the back each side's own back chain, behind its leg, keeps the panel to near the middle (a
                # stride's back leg pushes it back there, not half of it), the vent's edges shared alike
                t = gm.smoothstep(0.5 - BACK_BLEND, 0.5 + BACK_BLEND, t)
            w[inside, i] += 1.0 - t[inside]
            w[inside, i + 1] += t[inside]
        w[:, -1] += th > knots[:, -1]
        sub = np.zeros((int(sel.sum()), len(chains)))
        for k, i in enumerate(own + [other]):
            sub[:, i] += w[:, k]
        out[sel] = sub
    return out


def swing_weights(fig, ob, W):
    """ob's weights with the swing bones' columns after the mannequin's: the part's own weights (weights()) give way
    to its chains down the cloth, each chain's bones handing over in turn. The skirt: today's to the belt, all chains
    from mid-thigh (four influences: past there a vertex already shares two chains' two bones); the cape the same from
    the shoulder line to halfway down; the bandana's bib from under the chin, the back hair from where it leaves the
    skull; the lantern rigid on lantern_01."""
    chains = fig.swing
    bones = swing_bones(chains)
    col = {b[0]: i for i, b in enumerate(bones)}
    P = np.array([v.co[:] for v in ob.data.vertices])
    n = len(P)
    C = np.zeros((n, len(bones)))
    f = np.zeros(n)
    name = ob.name

    def spread(share, c, top):
        """Adds chain c's bones for vertices sharing it by share (n,); returns its body-to-chain ramp (n,)."""
        j = c['joints']
        a = along(P[:, 2], j[:, 2])
        for k in range(a.shape[1]):
            C[:, col[f'{c["name"]}_{k + 1:02d}']] += share * a[:, k]
        return share * gm.smoothstep(top, j[1, 2], P[:, 2])

    part = [c for c in chains if name.startswith(c['part']) and not name.startswith('BandanaKnot')]
    if name.startswith('Lantern'):
        C[:, col['lantern_01']] = 1.0
        f[:] = 1.0
    elif name.startswith(('Duster', 'Cape')) and part:
        share = ring_weights(P, part)
        for i, c in enumerate(part):
            f += spread(share[:, i], c, SKIRT_TOP if name.startswith('Duster') else c['joints'][0, 2])
    elif name.startswith(('Bandana', 'Hair')) and part:
        c = part[0]
        ang = np.abs(round_angle(P, c['centre']))
        if name.startswith('Bandana'):
            share = gm.smoothstep(math.radians(55), math.radians(25), ang)     # the bib, not the wrap round the jaw
        else:
            share = gm.smoothstep(math.radians(70), math.radians(130), ang)    # the back, not the temples
        f = spread(share, c, c['joints'][0, 2])
    if not f.any():
        return np.concatenate([W, C], 1)
    # the swing bones' shares come out of the part's own: f of the vertex is chains, the rest as before
    C = C / np.maximum(C.sum(1, keepdims=True), 1e-9) * f[:, None]
    out = soft_top4(np.concatenate([W * (1.0 - f)[:, None], C], 1))
    if name.startswith('Cape') and 'ArmFollow' in ob.data.attributes:
        # where the cape lies over an arm it rides the upper arm by its build's share (build_cape), so the game's idle
        # arms put it back as it was draped
        a = np.empty(n, dtype=np.float32)
        ob.data.attributes['ArmFollow'].data.foreach_get('value', a)
        a = np.clip(a, 0.0, 1.0)[:, None]
        arm = np.zeros_like(out)
        arm[P[:, 0] >= 0.0, fig.col['upperarm_l']] = 1.0
        arm[P[:, 0] < 0.0, fig.col['upperarm_r']] = 1.0
        out = soft_top4(out * (1.0 - a) + arm * a)
    return out


def hat_bone(parts):
    """The hat's bone (the game tips the hat back on it while aiming, the brim's front up, so it clears the rifle's
    scope): from the back of the band, where it meets the back of the crown, at the band's middle height, to the front
    of the band."""
    band = np.concatenate([gm._object_coords(o) for o in parts if o.name.startswith('HatBand')])
    c = band.mean(0)
    ang = round_angle(band, c[:2])
    return band[np.abs(ang) > math.radians(165)].mean(0), band[np.abs(ang) < math.radians(15)].mean(0)


def add_hat_bone(arm, head, tail):
    bk.select([arm], arm)
    bpy.ops.object.mode_set(mode='EDIT')
    e = arm.data.edit_bones.new('hat')
    e.head, e.tail = Vector(head), Vector(tail)
    e.align_roll(Vector((0.0, 0.0, 1.0)))              # Z up, X the head's left-right axis (+X tips the front up)
    e.use_connect, e.use_deform = False, True
    e.parent = arm.data.edit_bones['head']
    bpy.ops.object.mode_set(mode='OBJECT')
    return 'hat'


def rig(fig, objects):
    """Weights every part, joins them into one mesh 'Ellis' (SK_Ellis in Unreal) under the mannequin armature, the
    swing bones and the hat's bone."""
    arm = fig.b.armature('Ellis_Rig')
    fig.swing = swing_layout(fig, objects)
    fig.hat = hat_bone(objects)
    log(f'hat bone: ({fig.hat[0][0] * 100:.1f}, {fig.hat[0][1] * 100:.1f}, {fig.hat[0][2] * 100:.1f}) > '
        f'({fig.hat[1][0] * 100:.1f}, {fig.hat[1][1] * 100:.1f}, {fig.hat[1][2] * 100:.1f}) cm')
    names = fig.names + add_swing_bones(arm, fig.swing) + [add_hat_bone(arm, *fig.hat)]
    fig.rig_names = names
    # the parts rig_check measures against each other (the tipped brim, the turned head's bandana), at rest with
    # their weights
    fig.rest = {}
    for ob in objects:
        W = swing_weights(fig, ob, weights(fig, ob))
        hat = np.full((len(W), 1), 1.0 if ob.name.startswith('Hat') else 0.0)   # crown, brim and band: all on the hat
        W = np.concatenate([W * (1.0 - hat), hat], 1)
        fig.rest[ob.name] = (gm._object_coords(ob), [list(p.vertices) for p in ob.data.polygons], W)
        for j, bone in enumerate(names):
            col = W[:, j]
            nz = np.nonzero(col > 1e-4)[0]
            if not len(nz):
                continue
            g = ob.vertex_groups.get(bone) or ob.vertex_groups.new(name=bone)
            for i in nz:
                g.add([int(i)], float(col[i]), 'REPLACE')
    skin = next(o for o in objects if o.name == 'Ellis')
    for ob in objects:
        for attr in ('mh_index', 'ArmFollow'):
            if attr in ob.data.attributes:
                ob.data.attributes.remove(ob.data.attributes[attr])
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


# --rig-check: the swing bones at rest and bent, in clay (Workbench), into Saved/Concepts/EllisSwing. Each test turns
# bones toward a direction: the chain's root by the angle, each bone after it a third as much again (cloth curls).
FWD3, BACK3, LEFT3, UP3 = (0.0, -1.0, 0.0), (0.0, 1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0)
HAT_TIP = 20.0           # the most the game tips the hat back while aiming (degrees, brim front up)
SWING_TESTS = (
    ('skirt_fl', (('skirt_fl', FWD3, 35.0),), 'skirt', ('front', 'side')),
    ('skirt_bl', (('skirt_bl', BACK3, 35.0),), 'skirt', ('front', 'side', 'back')),
    ('cape_bl', (('cape_bl', BACK3, 25.0),), 'upper', ('front', 'side', 'back')),
    ('bandana', (('bandana', LEFT3, 30.0),), 'upper', ('front', 'side')),
    ('lantern', (('lantern', FWD3, 30.0),), 'hips', ('front', 'rside')),
    ('hat_tip', (('hat', UP3, HAT_TIP),), 'head', ('front', 'side')),
)
CHECK_SHOTS = dict(SHOTS, skirt=(0.66, 2.7, 0.15), head=(1.70, 1.1, 0.10), neck=(1.56, 1.2, 0.05))
CHECK_VIEWS = {'front': 0.0, 'side': 90.0, 'back': 180.0, 'rside': -90.0}
CHAIN_COLOURS = {'skirt_fl': (1.0, 0.45, 0.05), 'skirt_sl': (0.95, 0.85, 0.1), 'skirt_bl': (0.9, 0.15, 0.1),
                 'skirt_fr': (0.1, 0.55, 1.0), 'skirt_sr': (0.1, 0.85, 0.75), 'skirt_br': (0.45, 0.25, 0.95),
                 'cape_fl': (1.0, 0.45, 0.05), 'cape_bl': (0.9, 0.15, 0.1), 'cape_fr': (0.1, 0.55, 1.0),
                 'cape_br': (0.45, 0.25, 0.95), 'bandana': (0.95, 0.85, 0.1), 'hair': (0.2, 0.9, 0.3),
                 'lantern': (0.95, 0.85, 0.1)}


def swing_markers(arm, chains):
    """A bead at each swing bone's head and a rod to its tail, riding the bone, coloured by chain."""
    obs = []
    for name, h, t, parent, c in swing_bones(chains):
        bm = bmesh.new()
        bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=0.009, matrix=Matrix.Translation(Vector(h)))
        d = Vector(t) - Vector(h)
        rod = Matrix.LocRotScale((Vector(h) + Vector(t)) * 0.5, Vector((0.0, 0.0, 1.0)).rotation_difference(d), None)
        bmesh.ops.create_cone(bm, cap_ends=True, segments=8, radius1=0.004, radius2=0.002, depth=d.length, matrix=rod)
        if name.endswith(f'_{len(c["joints"]) - 1:02d}'):
            bmesh.ops.create_uvsphere(bm, u_segments=10, v_segments=6, radius=0.006, matrix=Matrix.Translation(Vector(t)))
        me = bpy.data.meshes.new('_Swing_' + name)
        bm.to_mesh(me)
        bm.free()
        ob = bpy.data.objects.new('_Swing_' + name, me)
        bpy.context.scene.collection.objects.link(ob)
        ob.vertex_groups.new(name=name).add(list(range(len(me.vertices))), 1.0, 'REPLACE')
        ob.modifiers.new('Armature', 'ARMATURE').object = arm
        ob.color = (*CHAIN_COLOURS[c['name']], 1.0)
        obs.append(ob)
    return obs


def swing_pose(arm, chains, moves):
    """Turns each named chain (or lone bone: the hat) toward a world direction: the root by deg, every later bone a
    third of it again (each in its rest frame, so a child bends on its parent)."""
    from mathutils import Quaternion
    for pb in arm.pose.bones:
        pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    for chain_name, toward, deg in moves:
        names = [b[0] for b in swing_bones(chains) if b[4]['name'] == chain_name] or [chain_name]
        for k, name in enumerate(names):
            b = arm.data.bones[name]
            axis = (b.tail_local - b.head_local).normalized().cross(Vector(toward))
            local = b.matrix_local.to_3x3().inverted() @ axis
            arm.pose.bones[name].rotation_quaternion = Quaternion(local.normalized(), math.radians(deg if k == 0 else deg / 3))
    bpy.context.view_layer.update()


def hat_contact(fig, deg):
    """How deep the brim, the hat tipped deg on its bone (front up), reaches into the hair and the collar: their
    points that end up inside the brim, or above it from under it, and how far above its underside (cm)."""
    from mathutils.bvhtree import BVHTree
    co, faces, _ = fig.rest['HatBrim']
    h, t = fig.hat
    axis = gm.unit(np.cross(gm.unit(t - h), (0.0, 0.0, 1.0)))
    R = np.array(Matrix.Rotation(math.radians(deg), 3, Vector(tuple(axis))))
    trees = [BVHTree.FromPolygons([tuple(p) for p in c], faces) for c in (co, (co - h) @ R.T + h)]

    def hits(tree, p, d):
        out, o, d = [], Vector(tuple(p)), Vector(d)
        for _ in range(8):
            loc = tree.ray_cast(o, d, 0.3)[0]
            if loc is None:
                break
            out.append(loc.z)
            o = loc + d * 1e-5
        return out
    report = []
    for part in ('Hair', 'Collar'):
        deep = []
        for p in fig.rest[part][0]:
            up0, up1 = hits(trees[0], p, (0, 0, 1)), hits(trees[1], p, (0, 0, 1))
            down = hits(trees[1], p, (0, 0, -1))
            if len(up1) % 2 == 1 and down:                            # inside the brim
                deep.append(p[2] - down[0])
            elif up0 and not up1 and len(down) >= 2:                  # the brim came down past it
                deep.append(p[2] - down[1])
        report.append(f'{part.lower()}: {len(deep)} of {len(fig.rest[part][0])} points, deepest '
                      f'{max(deep, default=0.0) * 100:.1f} cm')
    return '; '.join(report)


# The head turned on its own, the neck and the collar staying (aiming tilts it onto the stock): the bandana's band must
# keep off the neck's skin and the bandana stay inside the collar. (name, a world axis through the head's joint, degrees)
HEAD_TESTS = (('yaw_l', (0.0, 0.0, 1.0), 35.0), ('yaw_r', (0.0, 0.0, 1.0), -35.0), ('roll', (0.0, 1.0, 0.0), -20.0),
              ('pitch', (1.0, 0.0, 0.0), 20.0))


def head_pose(arm, axis, deg):
    from mathutils import Quaternion
    for pb in arm.pose.bones:
        pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    b = arm.data.bones['head']
    local = b.matrix_local.to_3x3().inverted() @ Vector(axis)
    arm.pose.bones['head'].rotation_quaternion = Quaternion(local.normalized(), math.radians(deg))
    bpy.context.view_layer.update()


def bandana_fit(fig, arm, axis, deg):
    """With the head turned deg about axis (and every bone under it): how close the bandana comes to the neck's skin
    below the chin (cm, negative through it; and how many points are through), and how far it pokes out past the
    collar (points, cm). The armature's blend skinning, from the weights rig() kept."""
    from mathutils.bvhtree import BVHTree
    sub = np.array([n == 'head' or any(p.name == 'head' for p in arm.data.bones[n].parent_recursive)
                    for n in fig.rig_names])
    R = np.array(Matrix.Rotation(math.radians(deg), 3, Vector(axis)))
    pivot = fig.sk['head'][0]

    def posed(name):
        co, faces, W = fig.rest[name]
        w = W[:, sub].sum(1)[:, None]
        return co + w * ((co - pivot) @ R.T + pivot - co), faces
    skin, sf = posed('Ellis')
    tree = BVHTree.FromPolygons([tuple(p) for p in skin], sf)
    band = np.concatenate([posed('Bandana')[0], posed('BandanaKnot')[0]])
    # out from the neck's axis (neck_01 to the head's joint) through each point below the chin: where the skin is
    # crossed against where the bandana is
    a, b = fig.sk['neck_01'][0], fig.sk['head'][0]
    gaps = []
    for p in band[band[:, 2] < fig.eye_z - 0.10]:
        c = a + (b - a) * float(np.clip((p[2] - a[2]) / (b[2] - a[2]), 0.0, 1.0))
        o = Vector((c[0], c[1], p[2]))
        d = Vector(tuple(p)) - o
        hit = tree.ray_cast(o, d.normalized(), 0.3) if d.length > 1e-6 else (None,)
        if hit[0] is not None:
            gaps.append(d.length - hit[3])
    gaps = np.array(gaps)
    collar, cf = posed('Collar')
    ctree = BVHTree.FromPolygons([tuple(p) for p in collar], cf)
    out = []
    for p in band[(band[:, 2] > collar[:, 2].min()) & (band[:, 2] < collar[:, 2].max())]:
        o = Vector((fig.neck[0], fig.neck[1], p[2]))
        d = Vector(tuple(p)) - o
        hit = ctree.ray_cast(o, d.normalized(), d.length)
        if hit[0] is not None:
            out.append(d.length - hit[3])
    return gaps.min() * 100, int((gaps < 0).sum()), len(out), max(out, default=0.0) * 100


def rig_check(fig, arm, skin):
    """The swing bones on the rigged game mesh, without the bake: rest (bones drawn, then each vertex coloured by
    its chains' shares) and the SWING_TESTS bends, front and side."""
    out = os.path.join(REPO, 'Saved', 'Concepts', 'EllisSwing')
    os.makedirs(out, exist_ok=True)
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.render.resolution_x, sc.render.resolution_y = 720, 960
    sc.display.render_aa = '8'
    sh = sc.display.shading
    sh.light = 'STUDIO'
    sh.color_type = 'OBJECT'
    sh.show_cavity = True
    sh.cavity_type = 'WORLD'
    sh.show_object_outline = False
    sh.show_specular_highlight = False
    world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    world.color = (0.30, 0.33, 0.38)
    sc.world = world
    skin.color = (0.62, 0.58, 0.53, 1.0)
    markers = swing_markers(arm, fig.swing)
    cam = bpy.data.objects.new('CheckCam', bpy.data.cameras.new('CheckCam'))
    cam.data.lens = 70
    sc.collection.objects.link(cam)
    sc.camera = cam
    written = []

    def shoot(tag, aim, views):
        for v in views:
            a = math.radians(CHECK_VIEWS[v])
            target = Vector((0.0, 0.0, aim[0]))
            cam.location = target + Vector((math.sin(a) * aim[1], -math.cos(a) * aim[1], aim[2]))
            cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
            sc.render.filepath = os.path.join(out, f'{tag}_{v}.png')
            bpy.ops.render.render(write_still=True)
            written.append(sc.render.filepath)

    swing_pose(arm, fig.swing, ())
    shoot('rest', CHECK_SHOTS['full'], ('front', 'side', 'back'))
    shoot('rest_upper', CHECK_SHOTS['upper'], ('front', 'side', 'back'))
    # each vertex coloured by its swing bones' shares (chain hue, darker down the chain), the body's share grey
    names = [g.name for g in skin.vertex_groups]
    W = np.zeros((len(skin.data.vertices), len(names)))
    for v in skin.data.vertices:
        for g in v.groups:
            W[v.index, g.group] = g.weight
    tint = np.zeros((len(names), 3))
    for j, nm in enumerate(names):
        chain = nm[:-3] if nm[-3:-2] == '_' and nm[-2:].isdigit() and nm[:-3] in CHAIN_COLOURS else None
        tint[j] = np.array(CHAIN_COLOURS[chain]) * (1.0 - 0.25 * (int(nm[-2:]) - 1)) if chain else (0.6, 0.6, 0.6)
    # in numbers: influences a vertex, the sums, and the biggest step in a swing bone's weight along an edge (a seam)
    swing_cols = [j for j, nm in enumerate(names) if nm not in fig.names]
    ed = np.empty(len(skin.data.edges) * 2, dtype=np.int64)
    skin.data.edges.foreach_get('vertices', ed)
    ed = ed.reshape(-1, 2)
    step = np.abs(W[ed[:, 0]][:, swing_cols] - W[ed[:, 1]][:, swing_cols]).max(1)
    length = np.linalg.norm(gm._object_coords(skin)[ed[:, 0]] - gm._object_coords(skin)[ed[:, 1]], axis=1)
    worst = np.argsort(-step / np.maximum(length, 0.01))[:3]
    log(f'rig check: at most {int((W > 0).sum(1).max())} influences a vertex, sums {W.sum(1).min():.3f}..{W.sum(1).max():.3f}; '
        f'steepest swing weight steps ' + ', '.join(f'{step[e]:.2f} over {length[e] * 100:.1f} cm at '
                                                     f'{tuple(np.round(gm._object_coords(skin)[ed[e, 0]] * 100, 1))}' for e in worst))
    attr = skin.data.color_attributes.new('SwingShares', 'FLOAT_COLOR', 'POINT')
    rgb = W @ tint / np.maximum(W.sum(1, keepdims=True), 1e-9)
    attr.data.foreach_set('color', np.concatenate([rgb, np.ones((len(rgb), 1))], 1).astype(np.float32).ravel())
    skin.data.color_attributes.active_color = attr
    sh.color_type = 'VERTEX'
    for m in markers:
        m.hide_render = True
    shoot('shares', CHECK_SHOTS['full'], ('front', 'side', 'back'))
    shoot('shares_upper', CHECK_SHOTS['upper'], ('front', 'side', 'back'))
    sh.color_type = 'OBJECT'
    for m in markers:
        m.hide_render = False
    for tag, moves, shot, views in SWING_TESTS:
        swing_pose(arm, fig.swing, moves)
        shoot(tag, CHECK_SHOTS[shot], views)
    swing_pose(arm, fig.swing, ())
    log(f'rig check: the hat tipped {HAT_TIP:.0f} degrees, the brim reaches into the {hat_contact(fig, HAT_TIP)}')
    for tag, axis, deg in (('rest', (0.0, 0.0, 1.0), 0.0),) + HEAD_TESTS:
        gap, through, past, most = bandana_fit(fig, arm, axis, deg)
        log(f'rig check: head {tag}: the bandana below the chin clears the neck\'s skin by {gap:.1f} cm at least '
            f'({through} points through it); {past} points out past the collar ({most:.1f} cm at most)')
        if deg:
            head_pose(arm, axis, deg)
            shoot(f'bandana_{tag}', CHECK_SHOTS['neck'], ('front', 'side'))
    head_pose(arm, (0.0, 0.0, 1.0), 0.0)
    log(f'rig check: {len(written)} renders in {out}')


def face_like_mannequin(arm, skin):
    """The finished figure turned a quarter about Z into its data (bones' rest, vertices; the hit zones' places), the
    last thing before the export: in Unreal SK_Ellis then faces +Y with his left side at +X, as SKM_Manny_Simple does
    (the exporter maps Blender's -Y to Unreal's +X and +X to -Y, so here he faces -X). Facing +X, the IK retargeter
    couldn't line his fingers and arms up with the mannequin's (the user saw clawed hands, 2026-10-15)."""
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


def hull_2d(p):
    """The convex hull of 2D points, counterclockwise (Andrew's monotone chain)."""
    p = p[np.lexsort((p[:, 1], p[:, 0]))]
    cross = lambda o, a, b: (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for q in p:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], q) <= 0:
            lower.pop()
        lower.append(q)
    for q in p[::-1]:
        while len(upper) >= 2 and cross(upper[-2], upper[-1], q) <= 0:
            upper.pop()
        upper.append(q)
    return np.array(lower[:-1] + upper[:-1])


LEG_FILL_DROP = 0.03     # behind the legs the fill narrows at most this going down (m per m): the coat's back hangs
                         # from the seat almost plumb, as heavy canvas does, clear of a leg stepping back


def leg_fill(fig, obs, z0, z1, gap=0.004):
    """A collider filling between the legs: at each height the convex hull of both legs' slice (obs), so the coat's
    back hangs straight over the gap instead of falling in between them (the user: "it should be straight",
    2026-10-15); and behind them dropping straight from the seat (LEG_FILL_DROP), not in along the calves (there a
    stride's back leg came out through it). Only the simulation sees it."""
    pts = np.concatenate([gm.object_points(o) for o in obs])
    c = fig.centre
    A = np.linspace(-math.pi, math.pi, 64, endpoint=False)
    D = np.stack(gm.radial(A, FWD), axis=1)
    back = gm.smoothstep(-0.2, -0.6, np.cos(A))
    zs, radii = [], []
    for z in np.arange(z1, z0 - 1e-6, -0.03):
        sl = pts[np.abs(pts[:, 2] - z) < 0.02][:, :2]
        if len(sl) >= 3:
            zs.append(z)
            radii.append(hull_radii(hull_2d(sl) - c, D) + gap)
    rings = []
    run = radii[0]
    for k, (z, r) in enumerate(zip(zs, radii)):
        if k:
            run = np.maximum(r, run - LEG_FILL_DROP * (zs[k - 1] - z))
        r = r + back * (run - r)
        rings.append(np.c_[c + D * r[:, None], np.full(len(D), z)])
    # rows top down (gm.grid's way), so its faces face out: Blender's cloth collides one-sided, by the collider's
    # normals, and with them inward it pulled the coat in through the fill and round each leg
    R = np.array(rings)
    return gm.mesh('_LegFill', gm.grid(R[..., 0], R[..., 1], R[..., 2], wrap=True))


def settle(fig, duster, colliders):
    """The duster's skirt falls into its folds: Blender's cloth over the body, the trousers, the waistcoat and the
    belt, the coat held above the hips (looter_garments.simulate, kept in Ellis.drape.json). Stiff canvas: bending 12
    gives the duster's broad folds; at 4 the skirt and the vent crumpled like linen. The cape is not simulated: on the
    shoulders it had nowhere to fall but into itself, and crumpled."""
    vent = np.array(duster['vent'], dtype=np.int64).reshape(-1, 2)
    cloth, at = welded(duster, vent, '_DusterCloth')
    zd = gm._object_coords(cloth)[:, 2]
    t0 = time.time()
    ran = gm.simulate([cloth], [gm.smoothstep(0.92, 1.08, zd)], colliders, DRAPE_FILE, frames=int(arg_value('--frames', 90)),
                      stiff=(40.0, 40.0, 15.0, 12.0), mass=0.3, self_collision=True,
                      note='The settled duster of Ellis (Art/Models/Characters/Ellis.py writes it: the cloth '
                           'simulation\'s result for the digest of its inputs).')
    duster.data.vertices.foreach_set('co', gm._object_coords(cloth)[at].astype(np.float32).ravel())
    duster.data.update()
    bpy.data.objects.remove(cloth)
    lap_vent(fig, duster, vent)
    log(f'cloth: {"simulated" if ran else "kept from Ellis.drape.json"} ({time.time() - t0:.0f} s)')


def welded(ob, pairs, name):
    """A copy of mesh ob with each vertex pair (keep, drop) welded into one, and for each of ob's vertices its vertex
    in the copy. (The faces between a pair's two columns, the vent's strip, fold away.)"""
    co = gm._object_coords(ob)
    to = np.arange(len(co))
    to[pairs[:, 1]] = pairs[:, 0]
    keep = np.ones(len(co), dtype=bool)
    keep[pairs[:, 1]] = False
    at = (np.cumsum(keep) - 1)[to]
    faces = []
    for p in ob.data.polygons:
        f = [int(at[v]) for v in p.vertices]
        f = [v for k, v in enumerate(f) if v != f[k - 1]]
        if len(set(f)) >= 3:
            faces.append(f)
    me = bpy.data.meshes.new(name)
    me.from_pydata(co[keep].tolist(), [], faces)
    copy = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(copy)
    return copy, at


def lap_vent(fig, duster, vent):
    """The settled vent's edges, together as the cloth fell, lapped as the built coat's are: the left flap's edge 4 mm
    out and across the right's, the right's tucked under it (the strip between them folds under)."""
    co = gm._object_coords(duster)
    k = gm.smoothstep(VENT_TOP, VENT_TOP - 0.08, co[vent[:, 0], 2])[:, None]
    for col, out, across in ((0, 0.004, 0.025), (1, 0.0, -0.02)):
        p = co[vent[:, col]]
        rel = p[:, :2] - fig.centre
        r = np.linalg.norm(rel, axis=1)
        ang = round_angle(p, fig.centre)
        ex, ey = gm.radial(ang, FWD)
        tx, ty = gm.radial(ang + math.pi / 2, FWD)
        p[:, 0] += (ex * out + tx * across * r) * k[:, 0]
        p[:, 1] += (ey * out + ty * across * r) * k[:, 0]
        co[vent[:, col]] = p
    duster.data.vertices.foreach_set('co', co.astype(np.float32).ravel())
    duster.data.update()


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
    torso_pts = fig.wv[fig.torso_idx]
    legs = fig.wv[fig.b.used][fig.w_leg[fig.b.used] > 0.3]
    belt = build_belt(fig, [trousers, vest])
    belt_pts = np.concatenate([gm.object_points(o) for o in belt])
    drape_pts = np.concatenate([torso_pts, legs, gm.object_points(vest), gm.object_points(trousers), belt_pts])
    duster = build_duster(fig, drape_pts, belt)
    sleeves = build_sleeves(fig, [body_ob])
    log('duster, sleeves')
    cape, top_r = build_cape(fig, body_ob, duster, sleeves)
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
        # the belt with its cartridges and bullets (without them the coat settled through the cartridges at the back),
        # the boots (the coat reaches the ankle), and the fill between the legs (the back hangs straight over it)
        fill = leg_fill(fig, [trousers, boots[0]], HEM - 0.04, 0.90)
        settle(fig, duster, [body_ob, trousers, vest, boots[0], fill] + belt[:4])
        bpy.data.objects.remove(fill)
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
    extra = []
    if OPTION:
        import ellis_options
        extra = ellis_options.build(OPTION, sys.modules[__name__], fig, dict(
            duster=duster, cape=cape, vest=vest, sleeves=sleeves, body=body_ob, buttons=buttons, lantern=lantern))
        log(f'option {OPTION}: {", ".join(o.name for o in extra) or "its changes to the built pieces"}')
    for ob in [cape, collar, bandana[0], vest, shirt] + belt[:2] + holster[:1] + hat[2:] + boots[:1] + \
            [g for g in gloves if g.name.startswith('Gauntlet')] + \
            [o for o in extra if o.name.startswith(('Bandolier', 'Satchel', 'Cuff', 'CapeTier', 'DusterLapel',
                                                    'DusterPocket'))]:
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
    check = '--stats' in ARGS or '--rig-check' in ARGS        # no bake: triangle counts, or the swing bones' check
    baked = None if check else bake_all(parts, highs, int(arg_value('--size', 2048)))
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
    if '--rig-check' in ARGS:
        rig_check(fig, arm, skin)
        return
    if '--audit' in ARGS:
        import ellis_audit                       # Tools/Blender: the baked look, the game's poses, every layer checked
        ellis_audit.run(fig, arm, skin, baked, arg_value('--audit', 'after'), viewer_materials, log, REPO)
        return
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
    face_like_mannequin(arm, skin)
    log('ready to export')


if __name__ == '__main__':
    main()
