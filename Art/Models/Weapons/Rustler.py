"""The Rustler, the game's submachine gun (2026-10-21): the "Helix" design the user picked from the frontier-tech concept
sheets (Saved/ArtPreviews/Weapons/Concepts/SMG_Options2.png, option 7): a slim sand upper, a long round magazine under
the barrel with a glowing spiral window (the rounds left), a wire stock; long and low. Every SMG in the game is built
from these parts (EWeaponKind::SMG, SMG ammo). Sand polymer and gunmetal at heart; rarer parts bring brass, engraving
and more glow.

How the parts fit (gun space, cm: u along the gun from the back of the receiver, v up from the bore; see looter_guns):

  RustlerBody_<Key>      the upper and its fittings: the gunmetal trigger housing, pistol grip, trigger and guard, the
                         ejection port on the right with its bolt, the charging knob on the left, the magazine cradle
                         under the front, the top rail, a rarity accent line on each side (GunAccentGlow). Sockets:
                         Stock (the back face), Barrel (the barrel nut), Magazine (the cradle's back, where the helix
                         seats), Sight (the rail's middle), Grip, Foregrip (the left hand holds the helix, as on a
                         helical-magazine gun), Eject (the port). Every body puts them in the same place.
  RustlerBarrel_<Key>    origin at the barrel nut: its shroud and barrel; SOCKET_Muzzle at its tip.
  RustlerMuzzle_<Key>    origin at its back face (on a barrel's SOCKET_Muzzle); SOCKET_Muzzle at its own tip.
  RustlerMagazine_<Key>  the helix: origin at its rear top, where it seats in the cradle; it runs forward under the
                         barrel on the axis MAG_V and slides out downward (-Z) to reload. Its glowing core is GunAmmoGlow,
                         whose UV0.x runs 0 at the feed end (the back) to 1 at the front: the game lights it from 0 up
                         to the share of rounds left. Its length is its capacity; the long ones need a barrel long
                         enough to reach over them (the CSV's Needs).
  RustlerSight_<Key>     origin on top of the rail, at its middle; SOCKET_Aim on its line of sight.
  RustlerStock_<Key>     origin at the middle of the back face; extends backward.

Each option's display name, the word it can lend the gun's name, the lowest rarity it appears on and its stat ranges
(rounds for magazines, zoom for sights, length for barrels) live in Rustler.parts.csv beside this file, which
Tools/Unreal/setup_gun_parts.py reads into the SMG's definition. A new part needs a model here and a row there, under
the same key; keys never change once guns drop.

    blender -b --factory-startup --python Art/Models/Weapons/Rustler.py -- --preview
"""
import math

import bpy
from mathutils import Vector

import looter_guns as lg
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xe8e8e8, 2.0)   # glows in the gun's rarity color in the game
SHELL = lg.POLY_SAND                               # the upper: tinted per gun
GRIPS = lg.POLY_GREY                               # grip and fittings: tinted per gun
METAL = lg.BLUED                                   # gunmetal
DARK = lg.BLACK
OLIVE = lg.material('GunOlivePaint', 'PaintWorn', 0x6f7550)
LEATHER = lg.material('GunLeather', 'Polymer', 0x7a5236)
SILVER = lg.material('GunSilverEngraved', 'MetalWorn', 0xb3b6ba)

# The layout (gun space, cm).
TOP, BOT, HALF = 3.4, -2.6, 2.0     # the upper's top, bottom and half width
STOCK_AT = (0.0, 0.4)
BARREL_AT = (31.0, 0.0)
MAG_U = 15.0                        # the helix's back face
MAG_V, MAG_R = -5.4, 2.45           # its axis and radius: its top touches the cradle
MAG_AT = (MAG_U, MAG_V + MAG_R)
SIGHT_AT = (16.0, TOP + 0.95)       # on the rail's teeth
MUZZLE_AT = (100.0, 0.0)            # muzzle devices are modeled here, around their own origin
HAND = (5.4, -8.6)
SUPPORT = (25.0, MAG_V - MAG_R)     # the left hand under the helix
EJECT = (16.0, 1.0, -HALF)

g = lg.Gun('Rustler')
origins = {}


def model(slot, key, at):
    name = f'{slot}_{key}'
    origins[name] = at
    return name


# --- Bodies: the upper and its fittings, all with the same sockets ---

UPPER = [(0.0, -2.0), (0.0, 2.8), (0.6, TOP), (26.0, TOP), (29.6, 2.0), (31.0, 1.4), (31.0, BOT), (0.6, BOT)]
HOUSING = [(1.6, -1.8), (14.8, -1.8), (14.8, -3.4), (13.8, -4.3), (3.0, -4.3), (1.6, -3.4)]
GRIP = [(10.2, -3.9), (9.7, -5.4), (9.0, -6.4), (9.1, -7.2), (8.5, -8.2), (8.6, -9.0), (8.0, -10.0), (8.0, -11.0),
        (7.5, -12.4), (7.0, -13.3), (6.2, -13.8), (3.2, -14.0), (2.0, -13.6), (1.6, -12.8), (1.9, -11.0), (2.6, -8.6),
        (3.0, -6.6), (3.4, -5.0), (3.9, -3.9)]
GUARD = lg.bezier((9.6, -5.2), (10.0, -7.9), (12.4, -7.7)) + lg.bezier((12.4, -7.7), (14.4, -7.5), (14.3, -3.8))[1:]
VENTS = (21.6, 23.6, 25.6)          # the slanted vents' back edges on the Vented upper


def port_cuts(extra=()):
    """The ejection port on the right and the charging handle's slot on the left, cut into the upper."""
    grooves = [lg.cbox(1.2, 26.0, 2.22, 2.4, -3.0, -HALF + 0.12), lg.cbox(1.2, 26.0, 2.22, 2.4, HALF - 0.12, 3.0)]
    return [lg.cbox(13.0, 19.0, 0.0, 2.0, -3.0, -HALF + 0.65), lg.cbox(17.0, 27.4, 0.85, 1.55, HALF - 0.6, 3.0)] + grooves +         list(extra)


def upper(m, mat, outline=UPPER, round=0.8, bevel=0.6, extra_cuts=()):
    g.add(m, lg.cut(lg.slab(outline, 2 * HALF, bevel=bevel, segments=3, mat=mat, round=round), port_cuts(extra_cuts)))


def grip(m, mat, outline=GRIP):
    g.add(m, lg.slab(outline, 3.0, bevel=0.7, segments=3, mat=mat, round=0.5))


def fittings(m, housing=METAL, bolt=DARK, knob=METAL, knob_tip=lg.STEEL, guard=METAL, trigger=lg.STEEL, rail=DARK,
             accent=(4.0, 24.0, -1.25), band=METAL):
    """What every body has besides its upper and grip: the gunmetal band along the upper's foot (band, None for
    none), the takedown pins and selector, the trigger housing, guard and trigger, the bolt in the port, the charging
    knob, the cradle, the rail, the back plate and barrel nut, the accent line and the sockets."""
    for side in (-1.0, 1.0):
        if band is not None:
            g.add(m, lg.slab([(0.5, BOT - 0.05), (30.5, BOT - 0.05), (30.9, -1.7), (0.5, -1.7)], 0.2,
                             w=side * (HALF + 0.03), bevel=0.05, mat=band, round=0.2))
    for u in (2.4, 11.8):
        g.add(m, lg.pin(u, -0.3, -HALF - 0.08, HALF + 0.08, 0.3, mat=lg.STEEL, segments=12))
    g.add(m, lg.pin(7.0, -0.2, HALF - 0.1, HALF + 0.25, 0.55, mat=housing, segments=14))       # the selector
    g.add(m, lg.slab([(7.0, -0.55), (9.8, -0.95), (9.9, -0.35), (7.0, 0.15)], 0.2, w=HALF + 0.32, bevel=0.05,
                     mat=knob_tip, round=0.2))
    g.add(m, lg.slab(HOUSING, 3.4, bevel=0.4, segments=2, mat=housing, round=0.4))
    g.add(m, lg.strip(GUARD, 1.0, 0.45, mat=guard))
    g.add(m, lg.slab([(11.0, -4.2), (12.0, -4.2), (11.9, -5.2), (11.5, -6.2), (10.9, -6.6), (11.2, -5.4)], 0.6, bevel=0.1,
                     mat=trigger, round=0.2))
    g.add(m, lg.box(13.2, 18.8, 0.15, 1.85, 0.3, w=-HALF + 0.5, bevel=0.05, mat=bolt))
    for u in (14.2, 15.2):   # the bolt's grooves
        g.add(m, lg.box(u, u + 0.25, 0.4, 1.6, 0.06, w=-HALF + 0.67, bevel=0.0, mat=DARK))
    g.add(m, lg.pin(26.6, 1.2, HALF - 0.6, HALF + 0.9, 0.36, mat=knob, segments=12))
    g.add(m, lg.pin(26.6, 1.2, HALF + 0.9, HALF + 1.25, 0.55, mat=knob_tip, segments=14,
                    profile=[(0.0, 0.42), (0.1, 0.55), (0.3, 0.55), (0.35, 0.4)]))
    g.add(m, lg.box(MAG_U, 30.6, MAG_V + MAG_R, BOT + 0.3, 3.0, bevel=0.15, mat=housing))       # the cradle
    g.add(m, lg.rail(7.0, 25.0, TOP, mat=rail))
    g.add(m, lg.box(-0.5, 0.6, -1.8, 3.0, 3.4, bevel=0.25, mat=housing))                         # the back plate
    g.add(m, lg.tube(30.4, BARREL_AT[0], 1.35, segments=20, mat=DARK))
    g.add(m, lg.pipe([(1.0, -1.9, HALF + 0.05), (1.4, -3.0, HALF + 0.6), (3.0, -3.0, HALF + 0.6),
                      (3.4, -1.9, HALF + 0.05)], 0.22, mat=lg.STEEL))                            # sling loop
    u0, u1, v = accent
    for side in (-1.0, 1.0):
        g.add(m, lg.box(u0, u1, v - 0.13, v + 0.13, 0.08, w=side * (HALF + 0.01), bevel=0.0, mat=ACCENT))
    for name, uv in (('Stock', STOCK_AT), ('Barrel', BARREL_AT), ('Magazine', MAG_AT), ('Sight', SIGHT_AT),
                     ('Grip', HAND), ('Foregrip', SUPPORT)):
        g.socket(m, name, uv)
    g.socket(m, 'Eject', EJECT[:2], EJECT[2])


def side_plates(m, mat, rivet, outline=((3.0, -1.6), (12.0, -1.6), (12.6, 2.6), (3.6, 2.6)), round=0.5):
    for side in (-1.0, 1.0):
        g.add(m, lg.slab(list(outline), 0.25, w=side * (HALF + 0.08), bevel=0.06, mat=mat, round=round))
        o = outline
        for u, v in ((o[0][0] + 0.6, o[0][1] + 0.5), (o[1][0] - 0.6, o[1][1] + 0.5), (o[2][0] - 0.7, o[2][1] - 0.5),
                     (o[3][0] + 0.7, o[3][1] - 0.5)):
            g.add(m, lg.rivet(u, v, side * (HALF + 0.2), side, r=0.24, mat=rivet))


def scroll(m, cu, cv, r0, side, w, turns=1.6, mat=lg.BRASS):
    """A curl of inlaid engraving on a side face."""
    pts = []
    for k in range(18):
        t = k / 17.0
        a = 2.0 * math.pi * turns * t
        r = r0 * (1.0 - 0.75 * t)
        pts.append((cu + side * r * math.cos(a), cv + r * math.sin(a), side * w))
    g.add(m, lg.pipe(pts, 0.06, mat=mat, sides=5))


def bodies():
    B = lambda key: model('Body', key, (0.0, 0.0))

    m = B('Standard')                                          # the Helix as drawn: sand upper, grey grip
    upper(m, SHELL)
    grip(m, GRIPS)
    fittings(m)

    m = B('Ironside')                                          # a gunmetal upper with sand inset panels, brass rivets
    upper(m, METAL)
    grip(m, GRIPS)
    for side in (-1.0, 1.0):
        for o in (((2.4, -1.7), (11.4, -1.7), (11.4, 2.5), (2.4, 2.5)), ((19.2, -1.7), (27.6, -1.7), (28.8, 0.6),
                                                                          (27.8, 2.5), (19.2, 2.5))):
            g.add(m, lg.slab(list(o), 0.2, w=side * (HALF + 0.05), bevel=0.05, mat=SHELL, round=0.4))
        for u, v in ((1.6, -1.8), (1.6, 2.6), (12.3, -1.8), (12.3, 2.6), (18.4, -1.8), (18.4, 2.6)):
            g.add(m, lg.rivet(u, v, side * (HALF + 0.02), side, r=0.26, mat=lg.BRASS))
    fittings(m, housing=DARK, accent=(4.0, 10.0, -1.25), band=DARK)
    for side in (-1.0, 1.0):   # the accent runs on past the panels
        g.add(m, lg.box(20.0, 27.0, -1.38, -1.12, 0.08, w=side * (HALF + 0.26), bevel=0.0, mat=ACCENT))

    m = B('Scrapper')                                          # rusty upper, patched and riveted, a taped grip
    upper(m, lg.SALVAGE, round=0.3, bevel=0.2)
    for side in (-1.0, 1.0):
        w = side * (HALF + 0.12)
        for u0, u1 in ((0.8, 12.6), (19.0, 30.0)):
            g.add(m, lg.slab([(u0, -2.0), (u1, -2.0), (u1, 2.8), (u0, 2.8)], 0.25, w=w, bevel=0.04, mat=lg.SALVAGE))
            for u in range(int(u0) + 1, int(u1), 3):
                for v in (-1.5, 2.3):
                    g.add(m, lg.rivet(u + 0.3, v, w + side * 0.14, side, r=0.24, mat=lg.STEEL))
        g.add(m, lg.pipe([(0.4, 3.0, side * HALF * 0.95), (15, 3.15, side * HALF * 0.95), (29.5, 2.2, side * HALF * 0.95)],
                         0.16, mat=lg.STEEL, sides=6))           # weld bead
    grip(m, lg.TAPE)
    for v in (-6.0, -8.0, -10.0, -12.0):                          # the tape's turns, across the raked grip
        ur, uf = 3.9 + (v + 3.9) * 0.26 - 0.2, 9.9 + (v + 5.0) * 0.3 + 0.2
        g.add(m, lg.slab([(ur, v - 0.2), (uf, v), (uf, v + 0.4), (ur, v + 0.2)], 3.15, bevel=0.1, mat=lg.TAPE))
    fittings(m, housing=lg.SALVAGE, bolt=lg.STEEL, guard=lg.STEEL, band=None)

    m = B('Vented')                                            # slanted vents up front glow ember as it fires
    vents = []
    for side in (-1.0, 1.0):
        w0, w1 = (HALF - 0.7, 3.0) if side > 0 else (-3.0, -HALF + 0.7)
        vents += [lg.cslab([(u, -1.6), (u + 0.9, -1.6), (u + 2.0, 1.6), (u + 1.1, 1.6)], w0, w1, round=0.3) for u in VENTS]
    upper(m, SHELL, extra_cuts=vents)
    g.add(m, lg.box(21.0, 28.6, -1.8, 1.8, 2 * (HALF - 0.62), bevel=0.1, mat=lg.EMBER))
    g.add(m, lg.slab([(20.6, TOP - 0.2), (28.0, TOP - 0.2), (28.0, TOP + 0.5), (21.2, TOP + 0.5)], 2.6, bevel=0.2,
                     mat=METAL))                                 # a heat shield over the vents
    grip(m, GRIPS)
    fittings(m, accent=(4.0, 12.6, -1.25))

    m = B('Skeleton')                                          # windows onto a brass bolt carrier and its spring
    windows = [lg.cslab([(3.0, -1.6), (12.4, -1.6), (12.4, 2.4), (3.0, 2.4)], -3.0, 3.0, round=0.9),
               lg.cslab([(20.0, -1.6), (27.0, -1.6), (27.0, 0.4), (20.0, 0.4)], -3.0, 3.0, round=0.6)]
    upper(m, SHELL, extra_cuts=windows)
    g.add(m, lg.box(2.8, 12.6, -1.0, 2.0, 2.2, bevel=0.3, mat=lg.BRASS))                      # the bolt carrier
    g.add(m, lg.tube(19.6, 27.4, 0.45, v=-0.6, mat=lg.STEEL))                                 # the spring's guide
    g.add(m, lg.helix(19.8, 27.2, 0.55, 0.8, 0.7, 0.35, v=-0.6, steps=12, mat=lg.STEEL))     # the spring
    grip(m, GRIPS)
    fittings(m, bolt=lg.BRASS, accent=(13.2, 19.6, -1.25))

    m = B('Plated')                                            # olive armor bolted over the upper, a guard plate
    upper(m, SHELL)
    for side in (-1.0, 1.0):
        w = side * (HALF + 0.3)
        g.add(m, lg.slab([(1.0, -2.4), (12.4, -2.4), (12.4, 3.0), (1.0, 3.0)], 0.6, w=w, bevel=0.15, mat=OLIVE, round=0.6))
        g.add(m, lg.slab([(19.4, -2.4), (29.6, -2.4), (30.4, 0.4), (29.0, 2.2), (19.4, 3.0)], 0.6, w=w, bevel=0.15,
                         mat=OLIVE, round=0.6))
        for u, v in ((1.8, -1.7), (11.6, -1.7), (1.8, 2.3), (11.6, 2.3), (20.2, -1.7), (28.8, -1.7), (20.2, 2.3)):
            g.add(m, lg.rivet(u, v, side * (HALF + 0.62), side, r=0.3, mat=lg.STEEL))
    grip(m, GRIPS)
    g.add(m, lg.slab([(9.2, -4.0), (14.8, -4.0), (14.6, -8.2), (12.4, -8.6), (9.6, -8.0)], 0.4, w=HALF * 0.8, bevel=0.1,
                     mat=OLIVE, round=0.6))                      # a plate guarding the trigger hand
    fittings(m, accent=(13.0, 19.0, -1.25))

    m = B('Engraved')                                          # silvered side plates scrolled in brass, brass fittings
    upper(m, SHELL)
    side_plates(m, SILVER, lg.BRASS)
    for side in (-1.0, 1.0):
        w = HALF + 0.21
        scroll(m, 6.2, 0.6, 1.4, side, w)
        scroll(m, 9.4, -0.2, 1.0, -side, w)
        g.add(m, lg.pipe([(0.5, 3.05, side * HALF * 1.01), (16.0, 3.1, side * HALF * 1.01), (25.8, 3.05, side * HALF * 1.01),
                          (29.4, 1.8, side * HALF * 1.01)], 0.07, mat=lg.BRASS, sides=5))
    grip(m, GRIPS)
    for side in (-1.0, 1.0):   # brass medallions on the grip
        g.add(m, lg.pin(5.2, -8.4, side * 1.5 - 0.07, side * 1.5 + 0.07, 0.75, mat=lg.BRASS, segments=16))
    fittings(m, guard=lg.BRASS, trigger=lg.BRASS, knob_tip=lg.BRASS, accent=(13.2, 24.0, -1.25))

    m = B('Emberheart')                                        # a glass eye on each side onto the ember core within
    eye_u, eye_v, eye_r = 8.0, 0.4, 1.7
    upper(m, SHELL, extra_cuts=[lg.cpin(eye_u, eye_v, -3.0, 3.0, eye_r, segments=24)])
    g.add(m, lg.pin(eye_u, eye_v, -HALF + 0.2, HALF - 0.2, eye_r - 0.1, mat=lg.EMBER, segments=24))   # the core
    for side in (-1.0, 1.0):
        w0 = HALF - 0.25 if side > 0 else -HALF - 0.25
        bezel = lg.pin(eye_u, eye_v, w0, w0 + 0.5, eye_r + 0.45, mat=lg.BRASS, segments=24)
        g.add(m, lg.cut(bezel, [lg.cpin(eye_u, eye_v, w0 - 0.2, w0 + 0.7, eye_r - 0.05, segments=24)]))
        w1 = side * (HALF - 0.12)
        g.add(m, lg.pin(eye_u, eye_v, w1 - 0.04, w1 + 0.04, eye_r - 0.02, mat=lg.LENS, segments=24))
        for k in range(8):   # the bezel's screws
            a = 2.0 * math.pi * k / 8
            g.add(m, lg.rivet(eye_u + (eye_r + 0.22) * math.cos(a), eye_v + (eye_r + 0.22) * math.sin(a),
                              side * (HALF + 0.27), side, r=0.13, mat=lg.STEEL))
        g.add(m, lg.box(18.0, 28.0, 2.1, 2.3, 0.08, w=side * (HALF + 0.01), bevel=0.0, mat=lg.EMBER))   # ember pinstripe
    grip(m, GRIPS)
    fittings(m, bolt=lg.BRASS, guard=lg.BRASS, trigger=lg.BRASS, knob_tip=lg.BRASS, accent=(13.2, 24.0, -1.25))


# --- Barrels: the shroud and barrel, from the barrel nut forward ---

def barrels():
    u0 = BARREL_AT[0]
    B = lambda key: model('Barrel', key, BARREL_AT)

    def tip(m, length, r=0.72, collar=None):
        """The bare barrel's last stretch to its crown, and the muzzle socket."""
        g.add(m, lg.tube(u0, u0 + length, r, segments=16, mat=DARK))
        if collar:
            g.add(m, lg.tube(u0 + length - 0.6, u0 + length, r + 0.12, segments=16, mat=collar))
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    def shroud(m, length, r=1.75, mat=METAL, nose=1.0, cuts=()):
        obj = lg.turned([(0.0, r), (length - nose, r), (length - nose * 0.3, r - 0.35), (length, r - 0.8)], u0,
                        segments=24, mat=mat)
        g.add(m, lg.cut(obj, list(cuts)) if cuts else obj)

    m = B('Stub')                                              # a stubby shroud, the barrel just showing
    shroud(m, 4.8, r=1.65)
    tip(m, 6.0)

    m = B('Standard')                                          # the Helix's: a chunky shroud with top slots
    shroud(m, 7.6, cuts=[lg.cbox(u0 + u, u0 + u + 1.3, 0.8, 2.2, -0.45, 0.45) for u in (1.4, 3.2, 5.0)])
    g.add(m, lg.tube(u0 + 1.2, u0 + 6.6, 1.3, segments=16, mat=DARK))                       # seen through the slots
    tip(m, 9.0)

    m = B('Ported')                                            # ember shows through slanted ports along its sides
    ports = []
    for w0, w1 in ((-2.5, -0.6), (0.6, 2.5)):
        ports += [lg.cslab([(u0 + u, -0.9), (u0 + u + 0.8, -0.9), (u0 + u + 1.5, 0.9), (u0 + u + 0.7, 0.9)], w0, w1,
                           round=0.25) for u in (1.2, 2.9, 4.6)]
    shroud(m, 7.6, cuts=ports)
    g.add(m, lg.tube(u0 + 0.8, u0 + 7.0, 1.4, segments=16, mat=lg.EMBER))
    tip(m, 9.0, collar=lg.STEEL)

    m = B('Long')                                              # a longer, slimmer shroud with a front post block
    shroud(m, 11.0, r=1.5, nose=1.4)
    g.add(m, lg.slab([(u0 + 8.4, 0.6), (u0 + 10.2, 0.6), (u0 + 9.9, 2.6), (u0 + 8.8, 2.6)], 1.0, bevel=0.12,
                     mat=DARK, round=0.2))
    for u in (2.0, 4.4, 6.8):                                  # grip bands
        g.add(m, lg.tube(u0 + u, u0 + u + 0.5, 1.58, segments=24, mat=DARK))
    tip(m, 13.0)

    m = B('Heatsink')                                          # a stack of cooling fins with a brass collar
    g.add(m, lg.tube(u0, u0 + 9.6, 1.0, segments=16, mat=METAL))
    for k in range(11):
        u = u0 + 0.6 + k * 0.8
        g.add(m, lg.tube(u, u + 0.35, 1.95, segments=24, mat=METAL))
    g.add(m, lg.tube(u0 + 9.4, u0 + 10.2, 1.3, segments=20, mat=lg.BRASS))
    tip(m, 11.0)

    m = B('Shrouded')                                          # a long perforated jacket over the barrel
    jacket = lg.tube(u0, u0 + 12.4, 1.6, segments=24, mat=METAL)
    holes = []
    for k, u in enumerate((1.4, 3.4, 5.4, 7.4, 9.4, 11.0)):
        holes += [lg.cpin(u0 + u, 0.0, -2.5, 2.5, 0.42, segments=10), lg.upright(u0 + u + 1.0, -2.5, 2.5, 0.42, segments=10)]
    g.add(m, lg.cut(jacket, holes))
    g.add(m, lg.tube(u0 + 12.2, u0 + 13.0, 1.72, segments=24, mat=lg.STEEL))
    g.add(m, lg.tube(u0 + 0.0, u0 + 0.6, 1.72, segments=24, mat=lg.STEEL))
    tip(m, 14.0)

    m = B('Carbine')                                           # a long sand handguard with ember lines, a rail beneath
    guard = lg.slab([(u0 - 0.4, 1.6), (u0 + 12.6, 1.6), (u0 + 13.4, 0.8), (u0 + 13.4, -1.0), (u0 + 12.6, -1.6),
                     (u0 - 0.4, -1.6)], 3.4, bevel=0.6, segments=3, mat=SHELL, round=0.6)
    slots = [lg.cslab([(u0 + u, -0.35), (u0 + u + 2.6, -0.35), (u0 + u + 2.6, 0.35), (u0 + u, 0.35)], w0, w1, round=0.3)
             for u in (1.2, 4.6, 8.0) for w0, w1 in ((-2.0, -1.2), (1.2, 2.0))]
    g.add(m, lg.cut(guard, slots))
    g.add(m, lg.box(u0 + 0.8, u0 + 11.2, -0.5, 0.5, 2.5, bevel=0.05, mat=lg.EMBER))
    g.add(m, lg.rail(u0 + 0.6, u0 + 12.0, 1.6, width=1.8, mat=DARK))
    tip(m, 16.0, collar=lg.STEEL)

    m = B('Gilded')                                            # gunmetal shroud bound in brass, a fluted brass tip
    shroud(m, 10.0, nose=1.2)
    for u in (0.4, 4.6, 8.0):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.7, 1.82, segments=24, mat=lg.BRASS))
    for side in (-1.0, 1.0):   # engraved vines between the bands
        for a in (1.2, 5.4):
            vine = [(a + k * 0.4, 0.5 * math.sin(k * 0.9)) for k in range(8)]
            g.add(m, lg.pipe([(u0 + u, v, side * (math.sqrt(1.75 ** 2 - v * v) + 0.02)) for u, v in vine], 0.06,
                             mat=lg.BRASS, sides=5))
    fluted = lg.tube(u0 + 9.4, u0 + 12.0, 0.95, segments=24, mat=lg.BRASS)
    g.add(m, lg.cut(fluted, [lg.cbox(u0 + 10.0, u0 + 11.6, 0.7, 1.3, -0.17, 0.17, rotate_u=a) for a in range(0, 360, 60)]))
    tip(m, 12.0)


# --- Muzzle devices: around their own origin, screwed onto the barrel's tip ---

def muzzles():
    u0 = MUZZLE_AT[0]
    M = lambda key: model('Muzzle', key, MUZZLE_AT)

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    m = M('Cap')                                               # the Helix's brass thread cap
    g.add(m, lg.turned([(0, 0.88), (1.3, 0.88), (1.6, 0.66)], u0, segments=20, mat=lg.BRASS))
    tip(m, 1.6)

    m = M('Cone')                                              # a flared flash cone
    g.add(m, lg.turned([(0, 0.88), (0.8, 0.88), (3.4, 1.3), (3.8, 1.15)], u0, segments=20, mat=METAL))
    tip(m, 3.8)

    m = M('Brake')                                             # two side ports and a brass ring
    brake = lg.turned([(0, 1.1), (4.4, 1.1), (4.8, 0.9)], u0, segments=20, mat=DARK)
    g.add(m, lg.cut(brake, [lg.cbox(u0 + 1.2, u0 + 2.2, -0.45, 0.45, -2, 2), lg.cbox(u0 + 2.8, u0 + 3.8, -0.45, 0.45, -2, 2)]))
    g.add(m, lg.tube(u0, u0 + 0.5, 1.18, segments=20, mat=lg.BRASS))
    tip(m, 4.8)

    m = M('Comp')                                              # ports on top hold the muzzle down
    comp = lg.turned([(0, 1.05), (4.2, 1.05), (5.0, 0.8)], u0, segments=20, mat=lg.STEEL)
    g.add(m, lg.cut(comp, [lg.cbox(u0 + u, u0 + u + 0.6, 0.2, 1.6, -0.34, 0.34) for u in (1.0, 2.1, 3.2)]))
    tip(m, 5.0)

    m = M('Glowcage')                                          # a slotted cage with an ember throat
    cage = lg.turned([(0, 1.05), (5.4, 1.05), (5.8, 0.88)], u0, segments=20, mat=DARK)
    g.add(m, lg.cut(cage, [lg.cbox(u0 + 1.4, u0 + 5.0, 0.5, 1.4, -0.17, 0.17, rotate_u=a) for a in range(0, 360, 45)]))
    g.add(m, lg.tube(u0 + 1.2, u0 + 5.2, 0.6, segments=12, mat=lg.EMBER))
    tip(m, 5.8)

    m = M('Can')                                               # a short suppressor in a sand sleeve
    g.add(m, lg.turned([(0, 0.95), (0.6, 1.7), (8.6, 1.7), (9.2, 1.25)], u0, segments=24, mat=DARK))
    g.add(m, lg.tube(u0 + 1.8, u0 + 7.4, 1.78, segments=24, mat=SHELL))
    tip(m, 9.2)

    m = M('LongCan')                                           # a long can bound in brass, ember vents at its nose
    can = lg.turned([(0, 0.95), (0.8, 1.85), (14.4, 1.85), (15.2, 1.3)], u0, segments=24, mat=DARK)
    g.add(m, lg.cut(can, [lg.cbox(u0 + u, u0 + u + 0.5, 0.4, 2.2, -0.3, 0.3, rotate_u=a)
                          for u in (12.0, 13.0) for a in (-50, 0, 50)]))
    g.add(m, lg.tube(u0 + 11.4, u0 + 14.0, 1.5, segments=16, mat=lg.EMBER))
    for u in (1.2, 6.8, 10.8):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.5, 1.93, segments=24, mat=lg.BRASS))
    tip(m, 15.2)


# --- Magazines: the helix, around where it seats ---

def helix_mag(m, length, band=METAL, caps=DARK, pitch=2.6, width=1.8, starts=1, keel=None, rings=(), ring_mat=lg.BRASS,
              pull=False, front_mat=None):
    """The helical magazine: a glowing core (GunAmmoGlow, lit from the back by the rounds left) inside a coil band
    whose gaps are the spiral window, a back cap with its feed tower up into the cradle, a domed front cap."""
    u0, u1, v, r = MAG_U, MAG_U + length, MAG_V, MAG_R
    core = lg.tube(u0 + 1.4, u1 - 1.4, r - 0.38, v=v, segments=24, mat=lg.AMMO_GLOW)
    g.add(m, lg.along_uv(core, u0 + 1.4, u1 - 1.4, v=v))
    for k in range(starts):
        g.add(m, lg.helix(u0 + 1.2, u1 - 1.5, r - 0.45, r, pitch * starts, width, v=v, steps=20,
                          phase=360.0 * k / starts + 90.0, mat=band))
    g.add(m, lg.turned([(0, r - 0.35), (0.3, r + 0.08), (1.5, r + 0.08), (1.75, r - 0.1)], u0, v=v, segments=28,
                       mat=caps))                                                   # back cap
    g.add(m, lg.slab([(u0 + 0.1, v + r - 0.4), (u0 + 1.55, v + r - 0.4), (u0 + 1.55, MAG_AT[1]), (u0 + 0.1, MAG_AT[1])],
                     2.0, bevel=0.15, mat=caps))                                    # feed tower
    g.add(m, lg.turned([(0, r + 0.08), (0.9, r + 0.08), (1.5, r - 0.3), (2.0, r - 1.1), (2.25, 0.9), (2.3, 0.0)],
                       u1 - 2.3, v=v, segments=28, mat=front_mat or caps))          # domed front cap
    if keel:
        g.add(m, lg.box(u0 + 1.2, u1 - 1.4, v - r - 0.2, v - r + 0.6, 1.3, bevel=0.2, mat=keel))
    for du in rings:
        g.add(m, lg.tube(u0 + du, u0 + du + 0.45, r + 0.1, v=v, segments=28, mat=ring_mat))
    if pull:   # a loop on the front cap to tear it out fast
        g.add(m, lg.pipe(lg.bezier((u1 - 0.6, v - 1.6, 0.0), (u1 + 2.4, v - 2.6, 0.0), (u1 + 0.8, v - 3.6, 0.0)) +
                         lg.bezier((u1 + 0.8, v - 3.6, 0.0), (u1 - 0.6, v - 3.4, 0.0), (u1 - 1.6, v - 2.2, 0.0))[1:],
                         0.22, mat=lg.STEEL, sides=8))
    # The latch tab under its back, which the thumb presses to drop it.
    g.add(m, lg.box(u0 + 0.2, u0 + 1.4, v - r - 0.35, v - r + 0.3, 1.6, bevel=0.15, mat=caps))


def magazines():
    M = lambda key: model('Magazine', key, MAG_AT)
    # Lengths: 40 rounds 22 cm, 45 24, 50 26, 55 28, 60 30. A helix may end at most 1 cm past its barrel's tip (the
    # muzzle device covers that), hence the CSV's Needs: barrel>=9 for 45 and 50, 11 for 55, 13 for 60.
    helix_mag(M('40'), 22.0)                                                         # the Helix's, 40 rounds
    helix_mag(M('45Quick'), 24.0, band=SHELL, caps=METAL, pull=True)                 # 45, a pull loop
    helix_mag(M('50'), 26.0, keel=METAL)                                             # 50, a keel beneath
    helix_mag(M('50Twin'), 26.0, starts=2, pitch=2.8, width=1.9, caps=lg.BRASS)     # twin spirals, brass caps
    helix_mag(M('60'), 30.0, keel=DARK, rings=(10.0, 20.0), ring_mat=DARK)          # 60, banded
    helix_mag(M('60Brass'), 30.0, band=SHELL, caps=lg.BRASS, rings=(7.5, 15.0, 22.5))  # sand and brass
    helix_mag(M('Gilded'), 28.0, band=lg.BRASS, caps=METAL, pitch=3.0, width=1.7, front_mat=lg.BRASS,
              rings=(1.75,))                                                         # 55, a brass coil


# --- Sights: on the rail (u 7..25), around its middle ---

def sights():
    S = lambda key: model('Sight', key, SIGHT_AT)
    uc, v0 = SIGHT_AT
    lg.frontier_sight(g, S('Iron'), 'Iron', uc, v0, span=14.0)
    lg.frontier_sight(g, S('Pip'), 'Pip', uc, v0)
    lg.frontier_sight(g, S('Reflex'), 'Reflex', uc, v0)
    lg.frontier_sight(g, S('Holo'), 'Holo', uc, v0)
    lg.frontier_sight(g, S('Prism'), 'Prism', uc, v0)
    lg.frontier_sight(g, S('Scope'), 'Scope', uc, v0, zoom=3.0)
    lg.frontier_sight(g, S('Gilded'), 'Reflex', uc, v0, shell=lg.BRASS, trim=lg.BRASS, metal=METAL)


# --- Stocks: from the back face, backward ---

WIRE_TOP, WIRE_BOT = 2.2, -1.5     # where the wire stock's struts leave the back face


def wire(m, mat=METAL, length=19.0, butt=lg.RUBBER, r=0.36, half=1.25, spring=False, pad=True):
    """The Helix's wire stock: a strut each side from the back face's top straight back, one from its bottom raked down
    to the butt, a cross bar and a butt pad; the hinge block at the back face."""
    ub = -length
    for side in (-1.0, 1.0):
        w = side * half
        g.add(m, lg.pipe([(0.4, WIRE_TOP, w), (ub + 0.9, WIRE_TOP, w), (ub + 0.2, WIRE_TOP - 0.6, w),
                          (ub + 0.2, -6.6, w), (ub + 1.0, -7.2, w)], r, mat=mat))
        g.add(m, lg.pipe([(0.4, WIRE_BOT, w), (ub + 1.0, -7.2, w)], r, mat=mat))
    for v in (WIRE_TOP, -3.0):
        g.add(m, lg.pin(ub + 0.2, v, -half, half, r, mat=mat, segments=10))
    g.add(m, lg.box(-1.0, 0.4, WIRE_BOT - 0.6, WIRE_TOP + 0.6, 2 * half + 0.8, bevel=0.3, mat=DARK))   # hinge block
    g.add(m, lg.pin(-0.4, WIRE_BOT, -half - 0.5, half + 0.5, 0.45, mat=lg.STEEL, segments=12))
    if spring:   # a buffer spring on the lower struts
        for side in (-1.0, 1.0):
            a, b = Vector((-1.6, WIRE_BOT - 0.35)), Vector((ub + 3.0, -6.2))
            d = (b - a)
            n = int(d.length / 0.55)
            pts = [(a.x + d.x * k / n + 0.0, a.y + d.y * k / n + 0.5 * math.sin(k * 2.2), side * half +
                    0.5 * math.cos(k * 2.2)) for k in range(n + 1)]
            g.add(m, lg.pipe(pts, 0.11, mat=lg.STEEL, sides=6))
    if pad:
        g.add(m, lg.slab([(ub + 0.3, WIRE_TOP + 0.6), (ub - 1.4, WIRE_TOP + 0.5), (ub - 1.4, -7.6), (ub + 0.3, -7.8)],
                         3.2, bevel=0.5, segments=3, mat=butt, round=0.5))


def stocks():
    S = lambda key: model('Stock', key, STOCK_AT)

    wire(S('Wire'))                                            # the Helix's wire stock

    m = S('Stub')                                              # no stock: a buffer cap and a sling loop
    g.add(m, lg.turned([(0, 1.6), (2.2, 1.6), (2.8, 1.1)], -2.8, v=0.6, segments=20, mat=METAL, spin=0.0))
    g.add(m, lg.box(-1.0, 0.4, WIRE_BOT - 0.6, WIRE_TOP + 0.6, 3.3, bevel=0.3, mat=DARK))
    g.add(m, lg.pipe([(-2.2, -1.0), (-2.8, -2.8), (-4.0, -2.8), (-4.4, -0.9)], 0.24, mat=lg.STEEL))

    m = S('Skeleton')                                          # a molded sand skeleton stock
    frame = lg.slab([(0.4, 2.8), (-17.6, 2.6), (-19.2, 1.8), (-19.2, -7.0), (-18.0, -8.0), (-14.0, -7.0), (0.4, -2.0)],
                    3.0, bevel=0.6, segments=3, mat=SHELL, round=1.2)
    g.add(m, lg.cut(frame, [lg.cslab([(-2.0, 1.2), (-16.4, 1.0), (-16.6, -5.0), (-13.6, -5.0), (-2.0, -1.0)], -3, 3,
                                      round=1.0)]))
    g.add(m, lg.slab([(-19.0, 2.4), (-20.4, 2.3), (-20.4, -7.8), (-18.6, -8.0)], 3.2, bevel=0.5, mat=lg.RUBBER, round=0.4))
    g.add(m, lg.box(-1.0, 0.4, WIRE_BOT - 0.6, WIRE_TOP + 0.6, 3.3, bevel=0.3, mat=DARK))

    m = S('Telescope')                                         # a buffer tube and a sliding sand butt
    g.add(m, lg.tube(-15.0, 0.4, 1.25, v=0.8, segments=20, mat=METAL))
    for u in range(-14, -5, 2):   # position holes along its bottom
        g.add(m, lg.box(u, u + 0.4, -0.55, -0.3, 0.6, bevel=0.0, mat=DARK))
    butt = lg.slab([(-6.0, 3.2), (-17.4, 3.2), (-18.6, 2.4), (-18.6, -7.2), (-17.2, -7.8), (-13.6, -6.6), (-10.4, -1.4),
                    (-6.0, -1.0)], 3.4, bevel=0.6, segments=3, mat=SHELL, round=1.0)
    g.add(m, lg.cut(butt, [lg.cslab([(-11.4, -2.2), (-16.4, -2.2), (-16.4, -5.6), (-14.0, -5.6)], -3, 3, round=0.8)]))
    g.add(m, lg.slab([(-18.4, 2.8), (-19.8, 2.7), (-19.8, -7.5), (-18.4, -7.6)], 3.4, bevel=0.5, mat=lg.RUBBER, round=0.4))
    g.add(m, lg.slab([(-7.0, -1.0), (-9.0, -1.2), (-8.6, -2.2), (-7.2, -2.0)], 1.0, bevel=0.15, mat=DARK))   # lever
    g.add(m, lg.box(-1.0, 0.4, WIRE_BOT - 0.6, WIRE_TOP + 0.6, 3.3, bevel=0.3, mat=DARK))

    m = S('Cheek')                                             # the wire stock with a leather cheek wrap
    wire(m)
    g.add(m, lg.slab([(-3.6, WIRE_TOP - 1.2), (-13.4, WIRE_TOP - 1.2), (-13.4, WIRE_TOP + 1.0), (-3.6, WIRE_TOP + 1.0)],
                     3.4, bevel=0.6, segments=3, mat=LEATHER, round=0.8))
    for u in (-4.6, -12.4):   # the lacing
        g.add(m, lg.box(u, u + 0.4, WIRE_TOP - 1.3, WIRE_TOP + 1.1, 3.55, bevel=0.1, mat=lg.STEEL))

    m = S('Spring')                                            # the wire stock on buffer springs, brass collars
    wire(m, spring=True)
    for side in (-1.0, 1.0):
        for u, v in ((-1.6, WIRE_BOT - 0.35), (-16.0, -6.2)):
            g.add(m, lg.pin(u, v, side * 1.25 - 0.5, side * 1.25 + 0.5, 0.55, mat=lg.BRASS, segments=12))

    m = S('Brass')                                             # brass struts and an engraved sand butt
    wire(m, mat=lg.BRASS, pad=False)
    g.add(m, lg.slab([(-18.6, WIRE_TOP + 0.7), (-20.6, WIRE_TOP + 0.6), (-20.6, -7.6), (-18.6, -7.9)], 3.4, bevel=0.5,
                     segments=3, mat=SHELL, round=0.5))
    for side in (-1.0, 1.0):
        scroll(m, -19.6, -2.4, 1.1, side, 1.72, mat=lg.BRASS)
    g.add(m, lg.slab([(-20.6, WIRE_TOP + 0.5), (-21.2, WIRE_TOP + 0.4), (-21.2, -7.4), (-20.6, -7.5)], 3.2, bevel=0.2,
                     mat=lg.BRASS, round=0.3))


DEFAULT = {'Body': 'Standard', 'Barrel': 'Standard', 'Muzzle': 'Cap', 'Magazine': '40', 'Sight': 'Pip', 'Stock': 'Wire'}
SLOTS = {
    'Body': ['Standard', 'Ironside', 'Scrapper', 'Vented', 'Skeleton', 'Plated', 'Engraved', 'Emberheart'],
    'Barrel': ['Stub', 'Standard', 'Ported', 'Long', 'Heatsink', 'Shrouded', 'Carbine', 'Gilded'],
    'Muzzle': ['Cap', 'Cone', 'Brake', 'Comp', 'Glowcage', 'Can', 'LongCan'],
    'Magazine': ['40', '45Quick', '50', '50Twin', '60', '60Brass', 'Gilded'],
    'Sight': ['Iron', 'Pip', 'Reflex', 'Holo', 'Prism', 'Scope', 'Gilded'],
    'Stock': ['Wire', 'Stub', 'Skeleton', 'Telescope', 'Cheek', 'Spring', 'Brass'],
}

bodies()
barrels()
muzzles()
magazines()
sights()
stocks()
lg.fill_uvs(g)   # glows and glass get UVs, so Unreal builds their tangents quietly
built = g.build(origins)


def socket_at(obj, name):
    """Where obj's socket name is (its own name, or Blender's numbered copy of it), in the scene."""
    s = next(c for c in obj.children if c.name == 'SOCKET_' + name or c.name.startswith('SOCKET_' + name + '.'))
    return obj.location + s.location


def assemble(choice, at=Vector()):
    """Puts the chosen body at `at` and the chosen parts on its sockets (the muzzle on the barrel); returns them."""
    body = built['Body_' + choice['Body']]
    body.location = Vector(at)
    shown = [body]
    barrel = built['Barrel_' + choice['Barrel']]
    barrel.location = socket_at(body, 'Barrel')
    muzzle = built['Muzzle_' + choice['Muzzle']]
    muzzle.location = socket_at(barrel, 'Muzzle')
    shown += [barrel, muzzle]
    for slot in ('Magazine', 'Sight', 'Stock'):
        obj = built[f'{slot}_{choice[slot]}']
        obj.location = socket_at(body, slot)
        shown.append(obj)
    bpy.context.view_layer.update()
    return shown


def lay_out(slot_keys, gap=0.04):
    """Every part of each slot in a row by itself, for the parts sheet; returns them."""
    shown = []
    z = 0.0
    for slot, keys in slot_keys.items():
        objs = [built[f'{slot}_{k}'] for k in keys]
        y, tallest = 0.0, 0.0
        for obj in objs:
            lo_y, hi_y = min(b[1] for b in obj.bound_box), max(b[1] for b in obj.bound_box)
            lo_z, hi_z = min(b[2] for b in obj.bound_box), max(b[2] for b in obj.bound_box)
            obj.location = Vector((0.0, y - hi_y, z - hi_z))
            y -= (hi_y - lo_y) + gap
            tallest = max(tallest, hi_z - lo_z)
            shown.append(obj)
        z -= tallest + gap
    bpy.context.view_layer.update()
    return shown


# In the scene: the plain Rustler assembled, every other part laid out in rows behind it by slot.
assemble(DEFAULT)
for row, (slot, keys) in enumerate(SLOTS.items()):
    for k, key in enumerate(keys):
        if key != DEFAULT[slot]:
            built[f'{slot}_{key}'].location = Vector((0.0, -k * 0.4, (row + 1) * 0.3))

VARIANTS = {
    'Rustler_Uncommon': {'Body': 'Vented', 'Barrel': 'Ported', 'Muzzle': 'Cone', 'Magazine': '45Quick',
                         'Sight': 'Reflex', 'Stock': 'Skeleton'},
    'Rustler_Rare': {'Body': 'Skeleton', 'Barrel': 'Heatsink', 'Muzzle': 'Glowcage', 'Magazine': '50Twin',
                     'Sight': 'Holo', 'Stock': 'Spring'},
    'Rustler_Epic': {'Body': 'Engraved', 'Barrel': 'Long', 'Muzzle': 'Can', 'Magazine': '60Brass', 'Sight': 'Scope',
                     'Stock': 'Cheek'},
    'Rustler_Legendary': {'Body': 'Emberheart', 'Barrel': 'Gilded', 'Muzzle': 'Brake', 'Magazine': 'Gilded',
                          'Sight': 'Gilded', 'Stock': 'Brass'},
    'Rustler_Scrap': {'Body': 'Scrapper', 'Barrel': 'Stub', 'Muzzle': 'Comp', 'Magazine': '40', 'Sight': 'Iron',
                      'Stock': 'Stub'},
    'Rustler_Heavy': {'Body': 'Plated', 'Barrel': 'Carbine', 'Muzzle': 'LongCan', 'Magazine': '60', 'Sight': 'Prism',
                      'Stock': 'Telescope'},
    'Rustler_Ironside': {'Body': 'Ironside', 'Barrel': 'Shrouded', 'Muzzle': 'Brake', 'Magazine': '50',
                         'Sight': 'Pip', 'Stock': 'Wire'},
}

if lt.want_preview():
    out = lambda name: lt.preview_path('Weapons', name)
    side, three_q, first_person = (-1.0, -0.02, 0.05), (-1.0, -0.35, 0.3), (0.42, 1.0, 0.32)
    lt.preview(assemble(DEFAULT), out('Rustler'), view=three_q, ground=False, lens=60.0, fit=0.95)
    lt.preview(assemble(DEFAULT), out('Rustler_Side'), view=side, ground=False, lens=80.0, fit=0.9)
    lt.preview(assemble(DEFAULT), out('Rustler_FP'), view=first_person, ground=False, lens=45.0, fit=0.8)
    for name, choice in VARIANTS.items():
        lt.preview(assemble(choice), out(name), view=three_q, ground=False, lens=60.0, fit=0.95)
    lt.preview(assemble(VARIANTS['Rustler_Legendary']), out('Rustler_Legendary_FP'), view=first_person, ground=False,
               lens=45.0, fit=0.8)
    lt.preview(lay_out(SLOTS), out('Rustler_Parts'), resolution=(2200, 1600), view=(-1.0, 0.0, 0.0), ground=False,
               lens=80.0, fit=0.75)
    assemble(DEFAULT)
