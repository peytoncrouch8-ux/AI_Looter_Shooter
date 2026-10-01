"""The Ranchhand pump shotgun the user chose (2026-10-01) as the base of every non-legendary shotgun, with
interchangeable parts: 8 bodies (receivers), barrels, muzzle devices, magazines, sights and stocks. Every pump shotgun
in the game is built from them (DA_PumpShotgun). Legendary shotguns will get their own models later.

How the parts fit (gun space, cm: u along the gun from the back of the receiver, v up from the bore; see looter_guns):

  RanchhandBody_<Key>      a receiver with its trigger group, top rail and rarity accent line (GunAccentGlow).
                           Sockets: Stock (its back face), Barrel (the barrel nut), Magazine and Pump (under the
                           barrel nut), Sight (the rail's middle), Grip, Foregrip. Every body puts them in the same
                           place, so any part fits any body.
  RanchhandPump_Walnut     the grooved walnut pump and its action bars, the same on every gun; it rides the magazine
                           tube (box and drum magazines bring a guide tube for it) and slides back along -X to cycle.
  RanchhandBarrel_<Key>    origin at the barrel nut; SOCKET_Muzzle at its tip.
  RanchhandMuzzle_<Key>    origin at its back face (on a barrel's SOCKET_Muzzle); SOCKET_Muzzle at its own tip.
  RanchhandMagazine_<Key>  origin under the barrel nut: tubes run forward under the barrel (the longer ones clamp to it),
                           box and drum magazines hang in a well in front of the trigger guard.
  RanchhandSight_<Key>     origin on top of the rail, at its middle (sights stay within its shortest rail);
                           SOCKET_Aim on its line of sight (the dot, the eyepiece, or the rear ring), which
                           aiming down sights puts before the eye.
  RanchhandStock_<Key>     origin at the back of the receiver on the bore line; each brings its own pistol grip.

Each option's display name, the word it can lend the gun's name, the lowest rarity it appears on and its stat ranges
(shells for magazines, zoom for sights) live in Ranchhand.parts.csv beside this file, which
Tools/Unreal/setup_gun_parts.py reads into DA_PumpShotgun; barrels give their length (cm) there, and the 8-shell tube
needs a barrel of at least 46 cm to clamp to (Needs). PARTS below was its first draft and now only lays the parts out
in the scene: a new part needs a model here and a row there, under the same key.

    blender -b --factory-startup --python Art/Models/Weapons/Ranchhand.py -- --preview
"""
import math

from mathutils import Matrix, Vector

import looter_guns as lg
import looter_props as lp
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xe8e8e8, 2.0)   # glows in the gun's rarity color in the game
RETICLE = lg.glow('GunReticle', 0xff3b2e, 6.0)
SHELL = lg.material('GunShellRed', 'Polymer', 0xb8402f)
LEATHER = lg.material('GunLeather', 'Polymer', 0x7a5236)
CASE = lg.material('GunCaseColored', 'MetalWorn', 0x9a8f80)
OLIVE = lg.material('GunOlivePaint', 'PaintWorn', 0x6f7550)
WHITE = lg.POLY_SAND

STOCK_AT = (0.0, 0.0)
BARREL_AT = (20.0, 0.0)
MAG_AT = (20.0, -2.5)
SIGHT_AT = (10.25, 3.15)
MUZZLE_AT = (100.0, 0.0)   # muzzle devices are modeled here, around their own origin

g = lg.Gun('Ranchhand')
origins = {}


def model(slot, key, at):
    name = f'{slot}_{key}'
    origins[name] = at
    return name


# --- Shared pieces ---

def shell_upright(m, u, v0, w):
    """A shotgun shell standing along v: brass head at the bottom, red hull above."""
    g.add(m, lg.upright(u, v0, v0 + 1.1, 1.02, w=w, mat=lg.BRASS, segments=12))
    g.add(m, lg.upright(u, v0 + 1.0, v0 + 6.0, 0.95, w=w, mat=SHELL, segments=12,
                        profile=[(0, 0.95), (4.8, 0.95), (5.0, 0.8)]))


def cuff(m, u0, u1, v_bottom, v_top, slope, width, mat, shells_u, v0, side_w):
    g.add(m, lg.slab([(u0, v_bottom), (u1, v_bottom + slope), (u1, v_top + slope * 0.2), (u0, v_top)], width,
                     bevel=0.6, segments=3, mat=mat, round=0.8))
    for k, u in enumerate(shells_u):
        shell_upright(m, u, v0 + k * slope / max(len(shells_u) - 1, 1) * 0.8, side_w)


def grooved(outline, width, u0, u1, v_list, mat, round=0.8):
    obj = lg.slab(outline, width, bevel=1.0, segments=3, mat=mat, round=round)
    half = width * 0.5
    cuts = []
    for v in v_list:
        cuts += [lg.cbox(u0, u1, v - 0.13, v + 0.13, half - 0.35, half + 1.0),
                 lg.cbox(u0, u1, v - 0.13, v + 0.13, -half - 1.0, -half + 0.35)]
    return lg.cut(obj, cuts)


def vent_rib(m, u0, u1, v, bead_u, post=0.2, mid_bead=None):
    g.add(m, lg.box(u0, u1, v + post - 0.2, v + post + 0.35, 0.7, bevel=0.1, mat=lg.BLUED))
    for u in range(int(u0) + 3, int(u1) - 1, 3):
        g.add(m, lg.box(u, u + 1.0, v - 0.15, v + post - 0.15, 0.72, bevel=0.0, mat=lg.STEEL))
    g.add(m, lg.upright(bead_u, v + post + 0.3, v + post + 0.7, 0.22, mat=lg.BRASS, segments=8))
    if mid_bead:
        g.add(m, lg.upright(mid_bead, v + post + 0.3, v + post + 0.55, 0.15, mat=lg.STEEL, segments=8))


def rotate_about_bore(obj, degrees):
    """Turns a part about the gun's bore line (its u axis): a shape drawn in u-v lies flat in u-w after 90."""
    obj.data.transform(Matrix.Rotation(math.radians(degrees), 4, 'Y'))
    return obj


# --- Bodies: receivers, all with the same sockets ---

def trigger_group(m, guard_mat=lg.BLUED, big=False):
    g.add(m, lg.slab([(3, -3.0), (13, -3.0), (12.4, -4.6), (4, -4.4)], 2.4, bevel=0.2, mat=lg.BLUED, round=0.3))
    if big:
        guard = lg.slab([(3.6, -4.2), (13.2, -4.6), (13.2, -7.6), (11.8, -8.4), (4.4, -8.0)], 2.2, bevel=0.2,
                        mat=guard_mat, round=0.4)
        g.add(m, lg.cut(guard, [lg.cbox(4.6, 12.2, -7.6, -4.4, -2, 2)]))
    else:
        g.add(m, lg.strip([(4.6, -4.4), (5.0, -6.8), (7.0, -7.6), (11.0, -7.2), (12.4, -4.6)], 0.8, 0.3, mat=guard_mat))
    g.add(m, lg.slab([(7.4, -4.4), (8.4, -4.4), (8.2, -5.6), (7.8, -6.6), (7.2, -6.8), (7.5, -5.6)], 0.5, bevel=0.1,
                     mat=lg.BLUED))
    g.add(m, lg.pin(10.6, -3.7, -1.5, 1.5, 0.3, mat=lg.STEEL))


def receiver(mat, outline=None, width=3.0, port=True, bolt=lg.STEEL):
    outline = outline or [(0, -3.0), (20, -3.0), (20, 1.4), (18.5, 2.2), (1.5, 2.2), (0, 1.6)]
    obj = lg.slab(outline, width, bevel=0.25, mat=mat, round=0.4)
    if port:
        obj = lg.cut(obj, [lg.cbox(6, 14, -0.8, 1.2, -width, -width * 0.5 + 0.5)])
    return obj


def accent(m, u0=3.0, u1=18.0, v=-2.74, half=1.53):
    for side in (-1.0, 1.0):
        g.add(m, lg.box(u0, u1, v - 0.11, v + 0.11, 0.08, w=side * half, bevel=0.0, mat=ACCENT))


def body_sockets(m):
    for name, uv in (('Stock', STOCK_AT), ('Barrel', BARREL_AT), ('Magazine', MAG_AT), ('Pump', MAG_AT),
                     ('Sight', SIGHT_AT), ('Grip', (-4.5, -6.0)), ('Foregrip', (37.0, -3.0))):
        g.socket(m, name, uv)


def bodies():
    B = lambda key: model('Body', key, (0.0, 0.0))

    m = B('Standard')                                          # the Ranchhand's: blued with grey plates
    g.add(m, receiver(lg.BLUED), lg.box(6.2, 13.8, -0.6, 1.0, 0.6, w=-0.9, bevel=0.1, mat=lg.STEEL))
    g.add(m, lg.slab([(2, -2.4), (17, -2.4), (18, 1.2), (3, 1.2)], 0.3, w=1.65, bevel=0.08, mat=lg.POLY_GREY, round=0.6))
    for a, b in ((1.2, 5.4), (14.6, 18.6)):
        g.add(m, lg.slab([(a, -2.4), (b, -2.4), (b + 0.2, 1.2), (a + 0.4, 1.2)], 0.3, w=-1.65, bevel=0.08,
                         mat=lg.POLY_GREY, round=0.5))
    for u, v in ((2.2, -1.6), (4.6, 0.6), (15.4, -1.6), (17.8, 0.6)):
        g.add(m, lg.rivet(u, v, -1.82, -1.0, r=0.24, mat=lg.BLACK))
    g.add(m, lg.rail(1, 19.5, 2.2, mat=lg.BLACK))
    trigger_group(m)
    accent(m)
    body_sockets(m)

    m = B('Classic')                                           # smooth blued, a low saddle mount
    g.add(m, receiver(lg.BLUED, [(0, -3.0), (20, -3.0), (20, 1.4), (18.5, 2.4), (1.5, 2.6), (0, 1.8)]))
    g.add(m, lg.box(6.2, 13.8, -0.6, 1.0, 0.6, w=-0.9, bevel=0.1, mat=lg.STEEL))
    g.add(m, lg.rail(3, 17.5, 2.2, mat=lg.BLUED))
    trigger_group(m)
    accent(m)
    body_sockets(m)

    m = B('Heritage')                                          # case-colored with brass trim and panel lines
    g.add(m, receiver(CASE), lg.box(6.2, 13.8, -0.6, 1.0, 0.6, w=-0.9, bevel=0.1, mat=lg.BRASS))
    for side in (-1.0, 1.0):
        g.add(m, lg.box(0.6, 19.4, -2.9, -2.55, 0.12, w=side * 1.52, bevel=0.0, mat=lg.BRASS))
        for a, b, c, d in ((1.2, 5.2, -2.2, 1.4), (14.8, 19.0, -2.2, 1.4)):
            for u0, u1, v0, v1 in ((a, b, c, c + 0.12), (a, b, d - 0.12, d), (a, a + 0.12, c, d), (b - 0.12, b, c, d)):
                g.add(m, lg.box(u0, u1, v0, v1, 0.1, w=side * 1.53, bevel=0.0, mat=lg.BLUED))
    g.add(m, lg.rail(2, 18.5, 2.2, mat=lg.BLUED))
    trigger_group(m, lg.BRASS)
    accent(m, 6.0, 14.0, -2.2)
    body_sockets(m)

    m = B('Tactical')                                          # black, an oversized guard, tan plates, a long rail
    g.add(m, receiver(lg.BLACK), lg.box(6.2, 13.8, -0.6, 1.0, 0.6, w=-0.9, bevel=0.1, mat=lg.STEEL))
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(14.6, -2.4), (19.0, -2.4), (19.2, 1.2), (15.0, 1.2)], 0.3, w=side * 1.65, bevel=0.08,
                         mat=lg.POLY_TAN, round=0.5))
    g.add(m, lg.rail(0.4, 19.8, 2.2, mat=lg.BLACK))
    g.add(m, lg.slab([(12.6, -3.6), (15.0, -3.8), (15.4, -4.6), (12.8, -4.4)], 0.6, w=-1.3, bevel=0.1, mat=lg.BLACK))
    trigger_group(m, lg.BLACK, big=True)
    accent(m)
    body_sockets(m)

    m = B('Salvaged')                                          # rusty plates welded and riveted on
    g.add(m, receiver(lg.SALVAGE, width=3.2), lg.box(6.2, 13.8, -0.6, 1.0, 0.6, w=-1.0, bevel=0.1, mat=lg.STEEL))
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(0.6, -2.6), (5.6, -2.6), (5.6, 1.6), (0.6, 1.6)], 0.3, w=side * 1.75, bevel=0.05,
                         mat=lg.SALVAGE))
        g.add(m, lg.slab([(14.4, -2.6), (19.4, -2.6), (19.4, 1.6), (14.4, 1.6)], 0.3, w=side * 1.75, bevel=0.05,
                         mat=lg.SALVAGE))
        for u in (1.2, 5.0, 15.0, 18.8):
            for v in (-2.0, 1.0):
                g.add(m, lg.rivet(u, v, side * 1.92, side, r=0.28, mat=lg.STEEL))
        g.add(m, lg.pipe([(0.4, 1.9, side * 1.62), (10, 2.0, side * 1.62), (19.6, 1.9, side * 1.62)], 0.18,
                         mat=lg.STEEL, sides=6))
    g.add(m, lg.rail(1, 19.5, 2.2, mat=lg.BLACK))
    trigger_group(m, lg.STEEL)
    accent(m, 6.0, 14.0, -2.74, 1.63)
    body_sockets(m)

    m = B('Skeleton')                                          # lightened, the brass bolt showing through
    windows = [lg.cslab([(u, -2.2), (u + 3.0, -2.2), (u + 3.0, 0.8), (u, 0.8)], -3, 3, round=0.6) for u in (1.2, 15.4)]
    g.add(m, lg.cut(receiver(lg.BLACK, port=False), windows + [lg.cbox(6, 14, -0.8, 1.2, -3, -1.0)]))
    g.add(m, lg.box(0.6, 19.4, -1.9, 0.9, 1.6, bevel=0.15, mat=lg.BRASS))
    g.add(m, lg.rail(1, 19.5, 2.2, mat=lg.BLACK))
    trigger_group(m, lg.BLACK)
    accent(m, 5.0, 14.6)
    body_sockets(m)

    m = B('Shell')                                             # a white shroud over it, like the bullpup AR's
    shroud = lg.slab([(-0.2, -3.2), (20.2, -3.2), (20.2, 1.2), (18.8, 2.2), (1.2, 2.2), (-0.2, 1.4)], 3.6, bevel=0.6,
                     segments=3, mat=WHITE, round=0.8)
    g.add(m, lg.cut(shroud, [lg.cbox(6, 14, -0.8, 1.2, -3, -1.2)]))
    g.add(m, lg.box(6.2, 13.8, -0.6, 1.0, 0.6, w=-1.2, bevel=0.1, mat=lg.STEEL))
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(14.6, -2.6), (19.2, -2.6), (19.4, 1.0), (15.0, 1.0)], 0.3, w=side * 1.95, bevel=0.08,
                         mat=lg.POLY_GREY, round=0.5))
        for u, v in ((15.4, -2.0), (18.6, -2.0), (15.6, 0.4), (18.8, 0.4)):
            g.add(m, lg.rivet(u, v, side * 2.12, side, r=0.22, mat=lg.BLACK))
    g.add(m, lg.rail(1, 19.5, 2.2, mat=lg.BLACK))
    trigger_group(m, lg.BLACK)
    accent(m, 1.0, 5.4, -2.74, 1.83)
    body_sockets(m)

    m = B('Armored')                                           # thick ribbed side armor in olive paint
    g.add(m, receiver(lg.BLUED), lg.box(6.2, 13.8, -0.6, 1.0, 0.6, w=-0.9, bevel=0.1, mat=lg.STEEL))
    for side in (-1.0, 1.0):
        for a, b in ((-0.4, 5.6), (14.4, 20.4)):
            g.add(m, lg.slab([(a, -3.3), (b, -3.3), (b, 1.8), (a + 0.6, 2.0)], 0.7, w=side * 1.85, bevel=0.15,
                             mat=OLIVE, round=0.4))
            for v in (-2.0, -0.6, 0.8):
                g.add(m, lg.box(a + 0.6, b - 0.4, v - 0.18, v + 0.18, 0.2, w=side * 2.28, bevel=0.05, mat=OLIVE))
    g.add(m, lg.rail(1, 19.5, 2.2, mat=lg.BLACK))
    trigger_group(m, lg.BLACK, big=True)
    accent(m, 6.0, 14.0, -2.74, 1.53)
    body_sockets(m)


def pump():
    """The walnut pump, the same on every gun, around the magazine socket."""
    m = model('Pump', 'Walnut', MAG_AT)
    g.add(m, grooved([(28, -1.4), (46, -1.4), (47, -2.4), (46.5, -4.0), (45, -4.4), (29, -4.4), (27.5, -3.6),
                      (27.5, -2.0)], 4.4, 31, 44, (-2.2, -2.9, -3.6), lg.WALNUT))
    for side in (-1.0, 1.0):
        g.add(m, lg.box(19.5, 28.5, -2.3, -1.8, 0.3, w=side * 1.2, bevel=0.05, mat=lg.STEEL))


# --- Barrels: from the barrel nut forward ---

def barrels():
    u0 = BARREL_AT[0]

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    m = model('Barrel', 'Field', BARREL_AT)                    # the Ranchhand's: vent rib and brass bead
    g.add(m, lg.tube(u0, u0 + 46, 1.15, segments=20, mat=lg.BLUED))
    vent_rib(m, u0, u0 + 46, 1.1, u0 + 45)
    tip(m, 46)

    m = model('Barrel', 'Short', BARREL_AT)                    # a short plain barrel and a ramp front sight
    g.add(m, lg.tube(u0, u0 + 38, 1.15, segments=20, mat=lg.BLUED))
    g.add(m, lg.slab([(u0 + 34.5, 1.0), (u0 + 37.5, 1.0), (u0 + 37.5, 1.9), (u0 + 36.8, 2.3)], 0.6, bevel=0.1,
                     mat=lg.BLUED))
    tip(m, 38)

    m = model('Barrel', 'Trap', BARREL_AT)                     # long, a high rib on tall posts, two beads
    g.add(m, lg.tube(u0, u0 + 56, 1.1, segments=20, mat=lg.BLUED))
    vent_rib(m, u0, u0 + 56, 1.05, u0 + 55, post=0.8, mid_bead=u0 + 28)
    tip(m, 56)

    m = model('Barrel', 'Slug', BARREL_AT)                     # thick, rifled, with rifle sights
    g.add(m, lg.turned([(0, 1.3), (45.6, 1.3), (46.0, 1.15)], u0, segments=24, mat=lg.BLUED))
    g.add(m, lg.slab([(u0 + 8, 1.2), (u0 + 11, 1.2), (u0 + 10.6, 2.6), (u0 + 8.4, 2.6)], 1.6, bevel=0.15,
                     mat=lg.BLUED, round=0.3))
    g.add(m, lg.slab([(u0 + 41, 1.2), (u0 + 45, 1.2), (u0 + 45, 2.0), (u0 + 44, 2.9), (u0 + 43.4, 2.9)], 0.7,
                     bevel=0.1, mat=lg.BLUED))
    tip(m, 46)

    m = model('Barrel', 'Shielded', BARREL_AT)                 # a perforated heat shield over the top
    g.add(m, lg.tube(u0, u0 + 46, 1.15, segments=20, mat=lg.BLACK))
    shield = lg.tube(u0 + 2, u0 + 40, 1.75, segments=20, mat=lg.STEEL)
    holes = [lg.cbox(u0 + 1, u0 + 41, -3, 0.2, -3, 3)]
    holes += [lg.cbox(u0 + u, u0 + u + 1.7, 0.6, 3.0, -0.5, 0.5, rotate_u=a) for u in range(4, 37, 3) for a in (-40, 0, 40)]
    g.add(m, lg.cut(shield, holes))
    g.add(m, lg.upright(u0 + 45, 1.1, 1.6, 0.22, mat=lg.BRASS, segments=8))
    tip(m, 46)

    m = model('Barrel', 'Ported', BARREL_AT)                   # vent rib, ported near the muzzle
    barrel = lg.tube(u0, u0 + 42, 1.15, segments=20, mat=lg.BLUED)
    g.add(m, lg.cut(barrel, [lg.cbox(u0 + u, u0 + u + 1.1, 0.4, 1.6, -0.35, 0.35, rotate_u=a)
                             for u in (34.5, 36.5, 38.5) for a in (-35, 35)]))
    vent_rib(m, u0, u0 + 33, 1.1, u0 + 32)
    tip(m, 42)

    m = model('Barrel', 'Defender', BARREL_AT)                 # matte, a winged front post for ghost rings
    g.add(m, lg.tube(u0, u0 + 46, 1.15, segments=20, mat=lg.BLACK))
    g.add(m, lg.box(u0 + 40, u0 + 44, -0.6, 1.4, 2.6, bevel=0.2, mat=lg.BLACK))
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(u0 + 40.4, 1.4), (u0 + 43.6, 1.4), (u0 + 43.2, 3.8), (u0 + 40.8, 3.8)], 0.4, w=side * 1.1,
                         bevel=0.08, mat=lg.BLACK))
    g.add(m, lg.upright(u0 + 42, 1.4, 3.3, 0.18, mat=lg.BLACK))
    tip(m, 46)

    m = model('Barrel', 'Heritage', BARREL_AT)                 # a solid raised rib, brass bands and bead
    g.add(m, lg.tube(u0, u0 + 50, 1.12, segments=20, mat=lg.BLUED))
    g.add(m, lg.box(u0, u0 + 50, 0.7, 1.45, 0.8, bevel=0.15, mat=lg.BLUED))
    for u in (2.0, 47.6):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.8, 1.22, segments=20, mat=lg.BRASS))
    g.add(m, lg.upright(u0 + 49.4, 1.4, 1.85, 0.24, mat=lg.BRASS, segments=8))
    tip(m, 50)


# --- Muzzle devices: around their own origin, on the barrel's tip ---

def muzzles():
    u0 = MUZZLE_AT[0]

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    m = model('Muzzle', 'Crown', MUZZLE_AT)                    # a plain flush choke
    g.add(m, lg.turned([(0, 1.18), (0.8, 1.18), (1.0, 1.0)], u0, segments=20, mat=lg.BLUED))
    tip(m, 1.0)

    m = model('Muzzle', 'Choke', MUZZLE_AT)                    # an extended, ported choke with a knurled ring
    choke = lg.turned([(0, 1.25), (4.2, 1.25), (4.6, 1.1)], u0, segments=20, mat=lg.STEEL)
    g.add(m, lg.cut(choke, [lg.cbox(u0 + 2.2, u0 + 3.8, 0.6, 1.6, -0.25, 0.25, rotate_u=a) for a in (0, 90, 180, 270)]))
    g.add(m, lg.tube(u0 + 0.4, u0 + 1.6, 1.38, segments=24, mat=lg.BLACK))
    tip(m, 4.6)

    m = model('Muzzle', 'Duckbill', MUZZLE_AT)                 # spreads the pattern sideways
    bill = lg.slab([(0, -1.2), (2.0, -1.2), (6.6, -2.6), (6.6, 2.6), (2.0, 1.2), (0, 1.2)], 1.3, bevel=0.2,
                   mat=lg.BLACK, round=0.4)
    rotate_about_bore(bill, 90.0)
    lp_move(bill, u0)
    g.add(m, bill)
    tip(m, 6.6)

    m = model('Muzzle', 'Breacher', MUZZLE_AT)                 # a toothed standoff for doors
    breach = lg.turned([(0, 1.6), (5.4, 1.6), (5.8, 1.4)], u0, segments=20, mat=lg.BLACK)
    g.add(m, lg.cut(breach, [lg.cbox(u0 + 4.4, u0 + 6.4, 0.9, 2.2, -0.5, 0.5, rotate_u=a) for a in (0, 90, 180, 270)]))
    tip(m, 5.8)

    m = model('Muzzle', 'Brake', MUZZLE_AT)                    # two side ports
    brake = lg.turned([(0, 1.45), (5.2, 1.45), (5.6, 1.2)], u0, segments=20, mat=lg.BLACK)
    g.add(m, lg.cut(brake, [lg.cbox(u0 + 1.0, u0 + 2.2, -0.55, 0.55, -2.5, 2.5),
                            lg.cbox(u0 + 3.0, u0 + 4.2, -0.55, 0.55, -2.5, 2.5)]))
    tip(m, 5.6)

    m = model('Muzzle', 'Compensator', MUZZLE_AT)              # top ports hold the muzzle down
    comp = lg.turned([(0, 1.35), (5.0, 1.35), (5.6, 1.05)], u0, segments=20, mat=lg.STEEL)
    g.add(m, lg.cut(comp, [lg.cbox(u0 + u, u0 + u + 0.8, 0.3, 1.8, -0.4, 0.4) for u in (1.0, 2.4, 3.8)]))
    tip(m, 5.6)

    m = model('Muzzle', 'Suppressor', MUZZLE_AT)               # a fat shotgun can
    g.add(m, lg.turned([(0, 1.2), (1.0, 2.1), (13.0, 2.1), (13.8, 1.5)], u0, segments=24, mat=lg.BLACK))
    for u in (1.0, 12.6):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.5, 2.18, segments=24, mat=lg.STEEL))
    tip(m, 13.8)

    m = model('Muzzle', 'Bell', MUZZLE_AT)                     # a flared flash cone
    g.add(m, lg.turned([(0, 1.2), (1.2, 1.2), (5.6, 2.2), (6.0, 2.2), (6.0, 1.9), (1.6, 0.95)], u0, segments=24,
                       mat=lg.BLUED))
    tip(m, 6.0)


def lp_move(obj, du):
    obj.data.transform(Matrix.Translation(lg.at(du, 0.0)))
    return obj


# --- Magazines: from under the barrel nut ---

def tube(m, length, clamp, window=False):
    u0, v = MAG_AT
    t = lg.tube(u0, u0 + length, 1.05, v=v, segments=16, mat=lg.BLUED)
    if window:
        t = lg.cut(t, [lg.cbox(u0 + length - 14, u0 + length - 4, v - 0.5, v + 0.5, -3, -0.5)])
        g.add(m, lg.tube(u0, u0 + length - 0.5, 0.88, v=v, segments=12, mat=SHELL))
    g.add(m, t)
    g.add(m, lg.turned([(0, 1.2), (1.6, 1.2), (2.0, 0.9)], u0 + length, v=v, segments=16, mat=lg.BLUED))
    if clamp:
        g.add(m, lg.slab([(u0 + length - 1.5, v - 0.9), (u0 + length + 0.5, v - 0.9), (u0 + length + 0.5, 0.9),
                          (u0 + length - 1.5, 0.9)], 2.2, bevel=0.3, mat=lg.BLUED, round=0.4))


def well(m, u_front=19.6, u_back=13.4, depth=-5.2):
    """A magazine well in front of the trigger guard, and a guide tube for the pump."""
    u0, v = MAG_AT
    g.add(m, lg.slab([(u_back, -3.0), (u_front, -3.0), (u_front, depth), (u_back - 0.2, depth)], 3.8, bevel=0.3,
                     mat=lg.BLACK, round=0.4))
    g.add(m, lg.tube(u0, u0 + 27, 0.8, v=v, segments=12, mat=lg.BLACK))
    g.add(m, lg.turned([(0, 0.95), (1.0, 0.95)], u0 + 27, v=v, segments=12, mat=lg.BLACK))


def magazines():
    M = lambda key: model('Magazine', key, MAG_AT)
    tube(M('Tube4'), 24, False)
    tube(M('Tube6'), 34, True)
    tube(M('Tube6Window'), 34, True, window=True)
    tube(M('Tube8'), 44, True)

    m = M('Box5')
    well(m)
    g.add(m, lg.slab(lg.curved_outline([(19.4, -5.2), (19.8, -10.0), (20.4, -15.0)],
                                       [(13.6, -5.2), (13.8, -10.0), (14.4, -14.6)]), 3.4, bevel=0.3,
                     mat=lg.POLY_BLACK))
    g.add(m, lg.box(14.0, 20.8, -15.8, -14.4, 3.8, bevel=0.3, mat=lg.POLY_BLACK))

    m = M('Box8')
    well(m)
    g.add(m, lg.slab(lg.curved_outline([(19.4, -5.2), (20.4, -13.0), (22.6, -21.0)],
                                       [(13.6, -5.2), (14.2, -13.0), (16.0, -20.4)]), 3.4, bevel=0.3,
                     mat=lg.POLY_BLACK))
    for k in range(3):
        v = -15.0 - k * 1.5
        g.add(m, lg.box(14.6 + k * 0.4, 21.6 + k * 0.5, v - 0.35, v + 0.35, 3.7, bevel=0.15, mat=lg.POLY_BLACK))
    g.add(m, lg.box(16.0, 23.2, -22.0, -20.6, 3.8, bevel=0.3, mat=lg.POLY_BLACK))

    m = M('Drum12')
    well(m, depth=-6.0)
    g.add(m, lg.pin(18.8, -11.4, -2.3, 2.3, 5.6, segments=28, mat=lg.POLY_BLACK,
                    profile=[(0, 5.0), (0.4, 5.6), (4.2, 5.6), (4.6, 5.0)]))
    for w0 in (-2.8, 2.3):
        g.add(m, lg.pin(18.8, -11.4, w0, w0 + 0.5, 2.0, segments=16, mat=lg.STEEL))

    m = M('Drum20')
    well(m, depth=-6.0)
    g.add(m, lg.pin(21.2, -13.6, -2.7, 2.7, 7.6, segments=32, mat=lg.SALVAGE,
                    profile=[(0, 6.8), (0.5, 7.6), (4.9, 7.6), (5.4, 6.8)]))
    for side in (-1.0, 1.0):
        w0 = 2.7 if side > 0 else -3.3
        g.add(m, lg.pin(21.2, -13.6, w0, w0 + 0.6, 2.6, segments=20, mat=lg.STEEL))
        for k in range(8):
            a = math.radians(22.5 + 45.0 * k)
            g.add(m, lg.rivet(21.2 + math.cos(a) * 5.2, -13.6 + math.sin(a) * 5.2, side * 2.72, side, r=0.3,
                              mat=lg.STEEL))


# --- See-through sights: aiming down sights puts the eye 22 cm behind SOCKET_Aim, looking along the axis ---

WALL = 0.12   # cm: thin tube walls keep the clear opening as wide as the tube


def hollow(profile, u0, v, segments, mat, wall=WALL):
    """A tube turned round an axis along u (profile [(du, r), ...] as lg.turned) but open at both ends: a thin wall
    that follows the outline inside, so the eye looks down the tube instead of at an end cap."""
    outer = [(r * lg.CM, du * lg.CM) for du, r in profile]
    inner = [((r - wall) * lg.CM, du * lg.CM) for du, r in reversed(profile)]
    obj = lp.lathe(outer + inner, segments=segments, closed=True)[0]
    obj.data.transform(Matrix.Translation(lg.at(u0, v)) @ Matrix.Rotation(math.radians(90.0), 4, 'X'))
    return lg.mapped(obj, mat)


def optics(m, u_eye, u_obj, v, r_eye, r_obj, dot, wall=WALL):
    """A tube sight's glass: a lens just inside the eyepiece and one inside the objective, and the reticle just
    behind the objective lens, a dot (dot=True) or a fine cross, tiny because it sits over the screen's crosshair."""
    g.add(m, lg.tube(u_eye + 0.15, u_eye + 0.25, r_eye - wall - 0.03, v=v, segments=20, mat=lg.LENS))
    g.add(m, lg.tube(u_obj - 0.25, u_obj - 0.15, r_obj - wall - 0.03, v=v, segments=24, mat=lg.LENS))
    u = u_obj - 0.4
    if dot:
        g.add(m, lg.pin(u, v, -0.06, 0.06, 0.06, mat=RETICLE, segments=10))
    else:
        g.add(m, lg.box(u, u + 0.02, v - 0.075, v + 0.075, 0.02, bevel=0.0, mat=RETICLE))
        g.add(m, lg.box(u, u + 0.02, v - 0.01, v + 0.01, 0.15, bevel=0.0, mat=RETICLE))


# --- Sights: on the rail, within its shortest stretch (u 3..17.5) ---

def sights():
    v0 = SIGHT_AT[1]
    S = lambda key: model('Sight', key, SIGHT_AT)

    m = S('Flip')                                              # the Ranchhand's flip-up rear
    leaf = lg.slab([(3.4, v0), (6.4, v0), (6.0, v0 + 2.05), (4.4, v0 + 2.05)], 2.0, bevel=0.15, mat=lg.BLACK, round=0.3)
    g.add(m, lg.cut(leaf, [lg.tube(2.4, 7.4, 0.3, v=v0 + 1.6, segments=16)]))   # the peep hole
    g.add(m, hollow([(0.0, 0.5), (0.6, 0.5)], 4.9, v0 + 1.6, 16, lg.BLACK, wall=0.2))   # the aperture ring
    g.socket(m, 'Aim', (5.2, v0 + 1.6))   # the aperture: the front bead is on the barrel, so the ring sets the eye

    m = S('GhostRing')                                         # a big aperture between protective wings
    g.add(m, lg.box(3.4, 8.0, v0, v0 + 0.9, 2.6, bevel=0.2, mat=lg.BLACK))
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(3.6, v0 + 0.9), (7.6, v0 + 0.9), (7.0, v0 + 3.4), (4.2, v0 + 3.4)], 0.5, w=side * 1.05,
                         bevel=0.1, mat=lg.BLACK, round=0.3))
    g.add(m, hollow([(0, 0.85), (0.5, 0.85)], 5.4, v0 + 2.2, 20, lg.BLACK, wall=0.3))
    g.socket(m, 'Aim', (5.65, v0 + 2.2))   # the ring: the front post is on the barrel

    m = S('RedDot')
    g.add(m, lg.box(6.6, 13.8, v0, v0 + 1.3, 2.2, bevel=0.25, mat=lg.BLACK))
    g.add(m, hollow([(0, 1.45), (1.0, 1.45), (1.4, 1.25), (6.6, 1.25), (7.0, 1.45), (8.0, 1.45)], 6.2, v0 + 2.8, 20,
                    lg.BLACK))
    g.add(m, lg.pin(10.2, v0 + 2.8, -2.0, -1.2, 0.55, mat=lg.BLACK))
    optics(m, 6.2, 14.2, v0 + 2.8, 1.45, 1.45, dot=True)
    g.socket(m, 'Aim', (6.4, v0 + 2.8))   # the eyepiece lens

    m = S('Reflex')
    g.add(m, lg.box(6.8, 13.8, v0, v0 + 1.0, 2.6, bevel=0.25, mat=lg.BLACK))
    frame = lg.slab([(8.2, v0 + 1.0), (13.6, v0 + 1.0), (13.6, v0 + 4.4), (12.2, v0 + 5.2), (9.6, v0 + 5.2),
                     (8.2, v0 + 4.4)], 2.4, bevel=0.2, mat=lg.BLACK, round=0.3)
    g.add(m, lg.cut(frame, [lg.cbox(7.2, 14.6, v0 + 1.6, v0 + 4.6, -0.95, 0.95)]))
    g.add(m, lg.box(12.8, 12.95, v0 + 1.6, v0 + 4.6, 1.9, bevel=0.0, mat=lg.LENS))
    g.add(m, lg.pin(12.7, v0 + 3.1, -0.1, 0.1, 0.1, mat=RETICLE))
    g.socket(m, 'Aim', (12.7, v0 + 3.1))   # the dot

    m = S('Holo')
    g.add(m, lg.box(4.75, 15.75, v0, v0 + 1.3, 3.0, bevel=0.25, mat=lg.BLACK))
    hood = lg.slab([(4.75, v0 + 1.2), (15.75, v0 + 1.2), (15.75, v0 + 5.4), (14.25, v0 + 6.3), (6.25, v0 + 6.3),
                    (4.75, v0 + 5.4)], 3.4, bevel=0.35, segments=3, mat=lg.BLACK, round=0.4)
    g.add(m, lg.cut(hood, [lg.cbox(3.75, 16.75, v0 + 1.9, v0 + 5.5, -1.25, 1.25)]))
    g.add(m, lg.box(14.25, 14.4, v0 + 1.9, v0 + 5.5, 2.5, bevel=0.0, mat=lg.LENS))
    g.add(m, lg.pin(14.15, v0 + 3.7, -0.12, 0.12, 0.12, mat=RETICLE))
    g.socket(m, 'Aim', (14.15, v0 + 3.7))   # the dot

    m = S('Prism')                                             # a compact 1.5x prism
    body = lg.slab([(5.25, v0), (15.25, v0), (15.25, v0 + 4.0), (13.75, v0 + 4.6), (6.75, v0 + 4.6), (5.25, v0 + 4.0)],
                   3.0, bevel=0.35, segments=3, mat=lg.BLACK, round=0.5)
    g.add(m, lg.cut(body, [lg.tube(4.25, 16.25, 1.05, v=v0 + 2.6, segments=20)]))   # the bore
    g.add(m, lg.box(14.8, 14.82, v0 + 2.525, v0 + 2.675, 0.02, bevel=0.0, mat=RETICLE))   # a fine cross
    g.add(m, lg.box(14.8, 14.82, v0 + 2.59, v0 + 2.61, 0.15, bevel=0.0, mat=RETICLE))
    for u in (5.15, 15.25):
        g.add(m, lg.tube(u, u + 0.12, 1.1, v=v0 + 2.6, segments=20, mat=lg.LENS))
    g.add(m, lg.upright(10.25, v0 + 4.5, v0 + 5.4, 0.7, mat=lg.BLACK, segments=14))
    g.socket(m, 'Aim', (5.15, v0 + 2.6))   # the rear lens

    m = S('Scout')                                             # a long, low 2.5x scout scope
    g.add(m, hollow([(0, 1.5), (3.0, 1.5), (4.2, 1.1), (16.0, 1.1), (17.6, 1.6), (21.0, 1.6)], 0.5, v0 + 2.6, 20,
                    lg.BLACK))
    for u in (5.0, 15.5):   # rings round the tube, open in the middle like it
        g.add(m, hollow([(0.0, 1.3), (1.2, 1.3)], u - 0.6, v0 + 2.6, 20, lg.BLACK, wall=0.2))
        g.add(m, lg.box(u - 0.9, u + 0.9, v0, v0 + 1.55, 2.0, bevel=0.2, mat=lg.BLACK))
    g.add(m, lg.upright(10.25, v0 + 3.65, v0 + 4.6, 0.75, mat=lg.BLACK, segments=14))
    optics(m, 0.5, 21.5, v0 + 2.6, 1.5, 1.6, dot=False)
    g.socket(m, 'Aim', (0.7, v0 + 2.6))   # the eyepiece lens

    m = S('Variable')                                          # a 1-4x scope on two rings
    g.add(m, hollow([(0, 1.8), (4.0, 1.8), (5.4, 1.25), (12.0, 1.25), (13.6, 1.75), (17.0, 1.75)], 1.8, v0 + 3.6, 24,
                    lg.BLACK))
    for u in (6.0, 14.5):   # rings round the tube, open in the middle like it
        g.add(m, hollow([(0.0, 1.45), (1.2, 1.45)], u - 0.6, v0 + 3.6, 20, lg.BLACK, wall=0.2))
        g.add(m, lg.box(u - 0.9, u + 0.9, v0, v0 + 2.4, 2.0, bevel=0.2, mat=lg.BLACK))
    g.add(m, lg.upright(10.25, v0 + 4.8, v0 + 6.0, 0.85, mat=lg.BLACK, segments=16))
    g.add(m, lg.pin(10.25, v0 + 3.6, -2.6, -1.18, 0.85, mat=lg.BLACK, segments=16))
    optics(m, 1.8, 18.8, v0 + 3.6, 1.8, 1.75, dot=False)
    g.socket(m, 'Aim', (2.0, v0 + 3.6))   # the eyepiece lens


# --- Stocks: behind the receiver, each with its pistol grip ---

FIELD_STOCK = [(0, 1.8), (0, -3.0), (-2.5, -4.4), (-5.5, -7.4), (-9, -8.0), (-12, -7.4), (-37, -12.6), (-38.5, -12.4),
               (-38.5, 0.2), (-36, 0.6), (-14, 1.0), (-4, 1.8)]


# A bird's-head pistol grip, curved down and back from the receiver.
RAIDER_GRIP = [(0.2, 1.4), (0.2, -3.0), (-0.4, -5.0), (-0.6, -11.0), (-1.6, -13.4), (-3.6, -14.6), (-6.6, -14.0),
               (-7.6, -12.0), (-7.0, -9.0), (-5.2, -4.6), (-4.4, -1.0), (-3.4, 0.8), (-1.6, 1.6)]


def butt_pad(m, mat=lg.RUBBER, thick=1.8, top=0.3, bottom=-12.7):
    g.add(m, lg.slab([(-38.4, bottom), (-38.4 - thick, bottom + 0.1), (-38.4 - thick, top + 0.1), (-38.4, top)], 4.4,
                     bevel=0.5, mat=mat, round=0.5))


def ar_grip(m, mat):
    g.add(m, lg.slab([(0.2, -2.8), (-2.0, -12.6), (-2.8, -14.0), (-6.2, -14.4), (-7.4, -13.4), (-4.6, -2.8)], 3.0,
                     bevel=0.7, segments=3, mat=mat, round=0.6))


def stocks():
    S = lambda key: model('Stock', key, STOCK_AT)

    m = S('Skeleton')                                          # the Ranchhand's
    stock = lg.slab(FIELD_STOCK, 4.2, bevel=1.1, segments=3, mat=WHITE, round=1.0)
    g.add(m, lg.cut(stock, [lg.cslab([(-16.5, -8.0), (-33.0, -10.9), (-34.2, -10.0), (-34.0, -6.4), (-32.5, -5.6),
                                      (-18.0, -4.9)], -3, 3, round=1.0)]))
    butt_pad(m)
    cuff(m, -30, -20, -5.8, 0.9, 0.3, 4.9, lg.POLY_GREY, (-28.2, -26.0, -23.8, -21.6), -4.9, -2.8)

    m = S('Field')                                             # the Farmhand's walnut, a leather cuff
    g.add(m, lg.slab(FIELD_STOCK, 3.8, bevel=1.1, segments=3, mat=lg.WALNUT, round=1.0))
    butt_pad(m)
    cuff(m, -24, -14, -9.6, 1.2, 1.8, 4.5, LEATHER, (-22.2, -20.0, -17.8, -15.6), -6.9, -2.6)

    m = S('Raider')                                            # a bird's-head pistol grip, no stock
    g.add(m, lg.slab(RAIDER_GRIP, 3.2, bevel=0.8, segments=3, mat=WHITE, round=1.2))
    g.add(m, lg.slab([(-2.6, -14.0), (-3.6, -14.6), (-6.6, -14.0), (-7.0, -13.4), (-4.0, -13.6)], 3.4, bevel=0.2,
                     mat=lg.RUBBER, round=0.3))
    g.add(m, lg.pipe([(-3.2, 1.4), (-4.4, 2.6), (-5.6, 1.2)], 0.22, mat=lg.STEEL))

    m = S('Collapsible')                                       # a buffer tube, a tan carbine stock, an AR grip
    g.add(m, lg.tube(-20, 0, 1.5, v=0.2, mat=lg.BLACK))
    ar_grip(m, lg.POLY_BLACK)
    carbine = lg.slab([(-10, 2.6), (-10, -1.6), (-13, -2.4), (-24, -9.6), (-26.6, -9.6), (-27, -8.6), (-27, 3.4),
                       (-25, 3.8), (-13, 3.4)], 3.4, bevel=0.6, segments=3, mat=lg.POLY_TAN, round=0.8)
    g.add(m, lg.cut(carbine, [lg.cslab([(-23.5, -6.6), (-16, -3.2), (-23.5, -3.2)], -2, 2, round=0.8)]))
    g.add(m, lg.slab([(-27, -9.6), (-28.3, -9.4), (-28.3, 3.6), (-27, 3.4)], 3.8, bevel=0.4, mat=lg.RUBBER, round=0.4))

    m = S('Thumbhole')                                         # a white thumbhole stock
    hole = [(-3.8, -3.6), (-11.6, -4.2), (-13.0, -6.0), (-12.4, -9.4), (-10.6, -10.6), (-6.6, -10.6), (-4.8, -9.4),
            (-3.6, -6.0)]
    stock = lg.slab([(0.4, 1.6), (0.4, -3.0), (-0.6, -5.2), (-0.6, -13.6), (-2.2, -14.8), (-6.0, -14.6), (-9.0, -12.6),
                     (-20, -11.0), (-37, -13.0), (-38.5, -12.8), (-38.5, 0.0), (-36, 0.4), (-14, 1.2), (-4, 1.8)], 4.2,
                    bevel=1.1, segments=3, mat=WHITE, round=1.0)
    g.add(m, lg.cut(stock, [lg.cslab(hole, -3, 3, round=1.0)]))
    butt_pad(m, bottom=-13.1, top=0.1)

    m = S('Folding')                                           # a steel frame, folded out, and a white grip
    g.add(m, lg.slab(RAIDER_GRIP, 3.2, bevel=0.8, segments=3, mat=WHITE, round=1.2))
    g.add(m, lg.pipe([(-2.0, 1.0), (-30.0, 0.4), (-31.4, -0.8), (-31.4, -9.0), (-30.0, -10.4), (-6.2, -6.0)], 0.6,
                     mat=lg.BLACK))
    g.add(m, lg.slab([(-31.4, -10.6), (-33.0, -10.4), (-33.0, 1.2), (-31.4, 1.0)], 3.4, bevel=0.4, mat=lg.RUBBER,
                     round=0.5))
    g.add(m, lg.pin(-2.4, 0.4, -2.0, 2.0, 0.55, mat=lg.STEEL))                     # the fold hinge

    m = S('Saddle')                                            # walnut wrapped in a laced leather cheek pad
    g.add(m, lg.slab(FIELD_STOCK, 3.8, bevel=1.1, segments=3, mat=lg.WALNUT, round=1.0))
    g.add(m, lg.slab([(-32, -2.2), (-16, -1.8), (-15, 1.6), (-33, 0.9)], 4.6, bevel=0.8, segments=3, mat=LEATHER,
                     round=1.0))
    for u in range(-31, -16, 3):
        g.add(m, lg.box(u, u + 0.3, -2.0, 1.2, 4.8, bevel=0.0, mat=lg.TAPE))
    butt_pad(m, LEATHER)

    m = S('Mule')                                              # heavy olive stock, cheek riser, a thick ribbed pad
    g.add(m, lg.slab(FIELD_STOCK, 4.6, bevel=1.2, segments=3, mat=OLIVE, round=1.0))
    g.add(m, lg.slab([(-33, 0.9), (-15, 1.4), (-15, 2.8), (-33, 2.4)], 3.6, bevel=0.5, segments=3, mat=lg.POLY_GREY,
                     round=0.8))
    for u in (-30.0, -18.0):
        g.add(m, lg.upright(u, 0.4, 1.4, 0.35, mat=lg.STEEL))
    pad = lg.slab([(-38.4, -12.8), (-41.6, -13.2), (-41.6, 0.8), (-38.4, 0.4)], 4.8, bevel=0.8, mat=lg.RUBBER, round=1.0)
    g.add(m, lg.cut(pad, [lg.cbox(-42.2, -41.0, v, v + 0.45, -3.0, 3.0) for v in (-10.5, -7.5, -4.5, -1.5)]))


# --- The design: names, rarities and stats (percent changes; magazines give shells, sights zoom) ---

R_COMMON, R_UNCOMMON, R_RARE, R_EPIC = 'Common', 'Uncommon', 'Rare', 'Epic'


def p(key, name, word, rarity, **stats):
    extra = {k: stats.pop(k) for k in ('length', 'needs') if k in stats}
    return dict(key=key, name=name, word=word, rarity=rarity, stats=stats, **extra)


PARTS = {
    'Body': [
        p('Standard', 'Standard receiver', '', R_COMMON),
        p('Classic', 'Classic receiver', 'Old', R_COMMON, handling=5, recoil=5),
        p('Heritage', 'Heritage receiver', 'Heirloom', R_UNCOMMON, accuracy=4, damage=3),
        p('Tactical', 'Tactical receiver', 'Tactical', R_UNCOMMON, handling=6, reload=-5),
        p('Salvaged', 'Salvaged receiver', 'Scrapped', R_UNCOMMON, damage=6, accuracy=-4),
        p('Skeleton', 'Skeleton receiver', 'Light', R_RARE, handling=12, recoil=8),
        p('Shell', 'Shell receiver', 'Shelled', R_RARE, fire_rate=8, handling=4),
        p('Armored', 'Armored receiver', 'Armored', R_EPIC, recoil=-15, damage=5, handling=-10),
    ],
    'Barrel': [
        p('Field', 'Field barrel', '', R_COMMON, length=46),
        p('Short', 'Short barrel', 'Sawn', R_COMMON, length=38, range=-25, accuracy=-12, handling=15),
        p('Ported', 'Ported barrel', 'Ported', R_UNCOMMON, length=42, recoil=-12, range=-5),
        p('Shielded', 'Heat-shielded barrel', 'Riot', R_UNCOMMON, length=46, fire_rate=6, handling=-4),
        p('Defender', 'Defender barrel', 'Defender', R_UNCOMMON, length=46, accuracy=8, handling=4),
        p('Slug', 'Rifled slug barrel', 'Slugger', R_RARE, length=46, range=40, accuracy=25, damage=-10),
        p('Trap', 'Trap barrel', 'Trap', R_RARE, length=56, range=25, accuracy=12, handling=-15),
        p('Heritage', 'Heritage barrel', 'Gilded', R_EPIC, length=50, damage=10, range=15, accuracy=8),
    ],
    'Muzzle': [
        p('Crown', 'Flush choke', '', R_COMMON),
        p('Choke', 'Extended choke', 'Choked', R_COMMON, accuracy=15, range=10),
        p('Duckbill', 'Duckbill spreader', 'Wide', R_UNCOMMON, accuracy=-20, damage=8),
        p('Bell', 'Flash bell', 'Flared', R_UNCOMMON, recoil=-5, accuracy=3),
        p('Brake', 'Muzzle brake', 'Braced', R_UNCOMMON, recoil=-18),
        p('Compensator', 'Compensator', 'Steady', R_RARE, recoil=-12, accuracy=8),
        p('Breacher', 'Breaching standoff', 'Breaching', R_RARE, damage=12, range=-10),
        p('Suppressor', 'Shotgun suppressor', 'Silent', R_EPIC, accuracy=10, range=5, damage=-4, handling=-10),
    ],
    'Magazine': [
        p('Tube4', '4-shell tube', 'Light', R_COMMON, magazine=4, reload=-10, handling=8),
        p('Tube6', '6-shell tube', '', R_COMMON, magazine=6),
        p('Tube6Window', '6-shell windowed tube', 'Ready', R_UNCOMMON, magazine=6, reload=-10),
        p('Tube8', '8-shell tube', 'Extended', R_UNCOMMON, magazine=8, reload=10, handling=-5, needs='barrel>=46'),
        p('Box5', '5-shell box', 'Boxed', R_RARE, magazine=5, reload=-35),
        p('Box8', '8-shell box', 'Magazine', R_RARE, magazine=8, reload=-25, handling=-5),
        p('Drum12', '12-shell drum', 'Drummer', R_EPIC, magazine=12, reload=-10, handling=-12),
        p('Drum20', '20-shell drum', 'Endless', R_EPIC, magazine=20, reload=15, handling=-20),
    ],
    'Sight': [
        p('Flip', 'Flip-up rear sight', '', R_COMMON, zoom=1.0, handling=8),
        p('GhostRing', 'Ghost ring', 'Ghost', R_COMMON, zoom=1.0, accuracy=4, handling=6),
        p('RedDot', 'Red dot', 'Dotted', R_COMMON, zoom=1.25, accuracy=5),
        p('Reflex', 'Reflex sight', 'Reflex', R_UNCOMMON, zoom=1.25, accuracy=6, handling=4),
        p('Holo', 'Holographic sight', 'Holo', R_UNCOMMON, zoom=1.5, accuracy=8),
        p('Prism', '1.5x prism', 'Prism', R_RARE, zoom=1.5, accuracy=10, range=5),
        p('Scout', '2.5x scout scope', 'Scout', R_RARE, zoom=2.5, accuracy=12, range=15, handling=-8),
        p('Variable', '1-4x scope', 'Hunter', R_EPIC, zoom=4.0, accuracy=15, range=20, handling=-12),
    ],
    'Stock': [
        p('Skeleton', 'Skeleton stock', '', R_COMMON),
        p('Field', 'Walnut field stock', 'Field', R_COMMON, accuracy=4, handling=-4),
        p('Raider', 'Raider grip', 'Raider', R_UNCOMMON, handling=20, recoil=25, accuracy=-12),
        p('Collapsible', 'Collapsible stock', 'Carbine', R_UNCOMMON, handling=10, recoil=5),
        p('Thumbhole', 'Thumbhole stock', 'Steady', R_UNCOMMON, accuracy=6, recoil=-4),
        p('Folding', 'Folding stock', 'Folding', R_RARE, handling=12, recoil=8),
        p('Saddle', 'Saddle stock', 'Saddled', R_RARE, recoil=-12, accuracy=4),
        p('Mule', 'Mule stock', 'Mule', R_EPIC, recoil=-25, accuracy=6, handling=-12),
    ],
}
DEFAULT = {'Body': 'Standard', 'Barrel': 'Field', 'Muzzle': 'Crown', 'Magazine': 'Tube6', 'Sight': 'Flip',
           'Stock': 'Skeleton'}


bodies()
pump()
barrels()
muzzles()
magazines()
sights()
stocks()
built = g.build(origins)


def socket_at(obj, name):
    s = next(c for c in obj.children if c.name.startswith('SOCKET_' + name))
    return obj.location + s.location


def assemble(choice, at=Vector()):
    """Puts the chosen body (choice['Body']) at `at` and the chosen parts on its sockets; returns the objects."""
    body = built['Body_' + choice['Body']]
    body.location = Vector(at)
    shown = [body]
    barrel = built['Barrel_' + choice['Barrel']]
    barrel.location = socket_at(body, 'Barrel')
    muzzle = built['Muzzle_' + choice['Muzzle']]
    muzzle.location = socket_at(barrel, 'Muzzle')
    shown += [barrel, muzzle]
    pump_obj = built['Pump_Walnut']
    pump_obj.location = socket_at(body, 'Pump')
    shown.append(pump_obj)
    for slot in ('Magazine', 'Sight', 'Stock'):
        obj = built[f'{slot}_{choice[slot]}']
        obj.location = socket_at(body, slot)
        shown.append(obj)
    return shown


# In the scene: the default gun assembled, every other part laid out in rows behind it by slot.
assemble(DEFAULT)
for row, slot in enumerate(('Body', 'Barrel', 'Muzzle', 'Magazine', 'Sight', 'Stock')):
    for k, part in enumerate(PARTS[slot]):
        obj = built[f'{slot}_{part["key"]}']
        if part['key'] != DEFAULT[slot]:
            obj.location = Vector((0.0, -k * 0.6, (row + 1) * 0.4))

if lt.want_preview():
    lt.preview(assemble(DEFAULT), lt.preview_path('Backlog', 'Ranchhand'), view=(-1.0, -0.25, 0.25), ground=False,
               lens=60.0, fit=0.95)
