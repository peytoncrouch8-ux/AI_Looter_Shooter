"""The Thresher, the game's light machine gun (2026-10-21), from the frontier-tech concept sheet the user picked
(Saved/ArtPreviews/Weapons/Concepts/NewGuns_ConceptB.png): a blocky gunmetal receiver under a sand top cover, a carry
handle, a belt box hung on the left with brass rounds climbing into the feed, a thick barrel whose heat rings glow
hotter the longer it fires, a bipod. Every LMG in the game is built from these parts (EWeaponKind::LMG, rifle ammo).
Rarer parts bring brass, engraving and more glow.

How the parts fit (gun space, cm: u along the gun from the back of the receiver, v up from the bore; see looter_guns):

  ThresherBody_<Key>      the receiver and its fittings: the top cover, carry handle (its top is the sight rail), pistol
                          grip, trigger and guard, the ejection port on the right, the feed tray on the left, the
                          handguard under the barrel and the bipod on it, a rarity accent line on each side
                          (GunAccentGlow). Sockets: Stock (the back face), Barrel (the barrel nut), Magazine (where the
                          belt box seats), Sight (the handle's rail), Grip, Foregrip (under the handguard), Eject. Every
                          body puts them in the same place.
  ThresherBarrel_<Key>    origin at the barrel nut; SOCKET_Muzzle at its tip. Its heat glow (GunHeatGlow: rings, a core
                          seen through a jacket or fins) has UV0.x 0 at the barrel nut and 1 at the tip, so the game can
                          heat it from the breech forward.
  ThresherMuzzle_<Key>    origin at its back face (on a barrel's SOCKET_Muzzle); SOCKET_Muzzle at its own tip.
  ThresherMagazine_<Key>  the belt box and the belt: origin at the box's back top, where it seats; it slides out
                          downward (-Z) to reload. The belt climbs from its top-left corner into the feed tray.
  ThresherSight_<Key>     origin on top of the handle's rail, at its middle; SOCKET_Aim on its line of sight.
  ThresherStock_<Key>     origin at the middle of the back face; extends backward.

Each option's display name, the word it can lend the gun's name, the lowest rarity it appears on and its stat ranges
(rounds for boxes, zoom for sights, length for barrels) live in Thresher.parts.csv beside this file, which
Tools/Unreal/setup_gun_parts.py reads into the LMG's definition. A new part needs a model here and a row there, under
the same key; keys never change once guns drop.

The element boxes (Ember100, Saint100, Salt100: the 100's box and belt)
wear their element (2026-10-22, looter_guns' element pieces): Ember heat-scored metal and ember vents, Soul-light
pale gold filigree, saint's medals or lanterns and a gold-white glow, Grave-salt salt crystals and cyan-white inlays.
`-- --elements` renders Saved/ArtPreviews/Weapons/Thresher_Elements.png (a row per element: first person, side, the part
alone).

    blender -b --factory-startup --python Art/Models/Weapons/Thresher.py -- --preview
"""
import math

import bpy
from mathutils import Vector

import looter_guns as lg
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xe8e8e8, 2.0)   # glows in the gun's rarity color in the game
SHELL = lg.POLY_SAND                               # the top cover: tinted per gun
GRIPS = lg.POLY_GREY                               # grip, stock and box: tinted per gun
METAL = lg.BLUED                                   # gunmetal
DARK = lg.BLACK
OLIVE = lg.material('GunOlivePaint', 'PaintWorn', 0x6f7550)
LEATHER = lg.material('GunLeather', 'Polymer', 0x7a5236)
SILVER = lg.material('GunSilverEngraved', 'MetalWorn', 0xb3b6ba)

# The layout (gun space, cm).
LEN, TOP, BOT, HALF = 38.0, 3.0, -4.6, 3.0   # the receiver's length, top, bottom and half width
COVER = 5.0                                 # the top cover's top
STOCK_AT = (0.0, -0.8)
BARREL_AT = (LEN, 0.0)
MAG_AT = (15.0, BOT)
HANDLE_TOP = 8.6                            # the carry handle's top: the sight rail sits on it
SIGHT_AT = (19.0, HANDLE_TOP + 0.95)
MUZZLE_AT = (100.0, 0.0)
GUARD_END = 50.0                            # the handguard under the barrel ends here; heat rings start past it
HAND = (5.8, -9.6)
SUPPORT = (44.0, -4.8)
EJECT = (19.0, -2.0, -HALF)
FEED_U0, FEED_U1 = 20.2, 27.8               # the feed tray on the left
BELT_W = HALF + 0.65                        # the belt's rounds climb here
BELT_U = 21.2                               # their bases

g = lg.Gun('Thresher')
origins = {}


def model(slot, key, at):
    name = f'{slot}_{key}'
    origins[name] = at
    return name


# --- Bodies ---

RECEIVER = [(0.0, -4.0), (0.0, 2.6), (0.6, TOP), (LEN, TOP), (LEN, -3.6), (37.0, BOT), (0.8, BOT)]
TOP_COVER = [(0.4, 2.6), (0.9, 4.4), (2.0, COVER), (34.6, COVER), (36.4, 4.0), (36.8, 2.6)]
GRIP = [(10.4, -4.5), (9.9, -6.0), (9.2, -7.0), (9.3, -7.8), (8.7, -8.8), (8.8, -9.6), (8.2, -10.6), (8.2, -11.6),
        (7.7, -13.0), (7.2, -13.9), (6.4, -14.4), (3.4, -14.6), (2.2, -14.2), (1.8, -13.4), (2.1, -11.6), (2.8, -9.2),
        (3.2, -7.2), (3.6, -5.6), (4.1, -4.5)]
GUARD = lg.bezier((9.8, -5.8), (10.2, -8.5), (12.6, -8.3)) + lg.bezier((12.6, -8.3), (14.6, -8.1), (14.5, -4.4))[1:]
HANDGUARD = [(37.6, -1.85), (49.0, -1.85), (GUARD_END, -2.2), (GUARD_END, -4.0), (49.0, -4.8), (37.6, -4.8)]


def receiver_cuts(extra=()):
    """The ejection port on the right, the feed port on the left, and a groove along each side."""
    return [lg.cbox(16.0, 22.0, -3.2, -0.8, -4.0, -HALF + 0.6), lg.cbox(FEED_U0 + 0.4, FEED_U1 - 0.4, -1.6, 1.2, HALF - 0.8, 4.0),
            lg.cbox(1.5, 15.0, 0.5, 0.75, -4.0, -HALF + 0.15), lg.cbox(1.5, 15.0, 0.5, 0.75, HALF - 0.15, 4.0),
            lg.cbox(28.6, 36.6, 0.5, 0.75, -4.0, -HALF + 0.15), lg.cbox(28.6, 36.6, 0.5, 0.75, HALF - 0.15, 4.0)] + list(extra)


def receiver(m, mat=METAL, cover=SHELL, extra_cuts=(), cover_cuts=()):
    g.add(m, lg.cut(lg.slab(RECEIVER, 2 * HALF, bevel=0.4, segments=2, mat=mat, round=0.5), receiver_cuts(extra_cuts)))
    top = lg.slab(TOP_COVER, 2 * HALF - 0.4, bevel=0.6, segments=3, mat=cover, round=0.8)
    g.add(m, lg.cut(top, list(cover_cuts)) if cover_cuts else top)


def grip(m, mat=GRIPS):
    g.add(m, lg.slab(GRIP, 3.2, bevel=0.75, segments=3, mat=mat, round=0.5))


def fittings(m, rivets=lg.BRASS, handle=METAL, guard=METAL, trigger=lg.STEEL, bolt=DARK, rail=DARK, handguard=METAL,
             accent=(2.0, 36.0)):
    """What every body has besides its receiver, cover and grip: rivets, the carry handle and its rail, the trigger and
    guard, the bolt in the port, the feed tray, the cover's hinge and latch, the handguard and bipod, the accent line
    and the sockets."""
    for side in (-1.0, 1.0):
        for u, v in ((4.6, 1.6), (4.6, -3.4), (34.4, 1.6), (34.4, -3.4)):
            g.add(m, lg.rivet(u, v, side * HALF, side, r=0.42, mat=rivets))
    arch = lg.slab([(9.4, COVER - 0.2), (28.6, COVER - 0.2), (27.0, HANDLE_TOP), (11.0, HANDLE_TOP)], 2.2, bevel=0.3,
                   mat=handle, round=0.5)
    g.add(m, lg.cut(arch, [lg.cslab([(12.6, COVER - 0.6), (25.4, COVER - 0.6), (24.4, HANDLE_TOP - 1.3),
                                     (13.6, HANDLE_TOP - 1.3)], -2.0, 2.0, round=0.6)]))
    g.add(m, lg.rail(11.4, 26.6, HANDLE_TOP, mat=rail))
    g.add(m, lg.strip(GUARD, 1.1, 0.5, mat=guard))
    g.add(m, lg.slab([(11.2, -4.5), (12.3, -4.5), (12.2, -5.6), (11.8, -6.7), (11.1, -7.1), (11.5, -5.8)], 0.7, bevel=0.1,
                     mat=trigger, round=0.2))
    g.add(m, lg.box(16.2, 21.8, -3.0, -1.0, 0.3, w=-HALF + 0.45, bevel=0.05, mat=bolt))
    g.add(m, lg.box(15.4, 22.6, -3.5, -3.2, 0.5, w=-HALF - 0.1, bevel=0.05, mat=METAL))           # port's lower lip
    # The feed tray on the left: a roof and two cheeks the belt climbs between.
    wt = HALF + 0.65
    g.add(m, lg.box(FEED_U0, FEED_U1, 1.2, 1.9, 1.5, w=wt, bevel=0.15, mat=METAL))
    for u in (FEED_U0, FEED_U1 - 0.6):
        g.add(m, lg.box(u, u + 0.6, -1.8, 1.9, 1.5, w=wt, bevel=0.1, mat=METAL))
    g.add(m, lg.pin(1.5, 3.9, -HALF + 0.1, HALF - 0.1, 0.5, mat=METAL, segments=14))             # cover hinge
    g.add(m, lg.box(35.4, 36.6, 2.6, 4.4, 1.6, w=HALF - 0.5, bevel=0.15, mat=lg.STEEL))         # cover latch
    g.add(m, lg.slab(HANDGUARD, 4.6, bevel=0.5, segments=2, mat=handguard, round=0.4))
    for side in (-1.0, 1.0):   # its cooling slots
        g.add(m, lg.box(39.5, 47.5, -3.2, -2.6, 0.1, w=side * 2.31, bevel=0.0, mat=DARK))
    g.add(m, lg.rail(39.0, 47.6, -4.8, width=2.0, mat=DARK, down=True))
    g.add(m, lg.tube(LEN - 0.4, LEN, 2.2, segments=24, mat=DARK))                                 # barrel nut
    bipod(m)
    u0, u1 = accent
    for side in (-1.0, 1.0):
        g.add(m, lg.box(u0, u1, 2.08, 2.32, 0.08, w=side * (HALF + 0.01), bevel=0.0, mat=ACCENT))
    for name, uv in (('Stock', STOCK_AT), ('Barrel', BARREL_AT), ('Magazine', MAG_AT), ('Sight', SIGHT_AT),
                     ('Grip', HAND), ('Foregrip', SUPPORT)):
        g.socket(m, name, uv)
    g.socket(m, 'Eject', EJECT[:2], EJECT[2])


def bipod(m, mat=METAL):
    """Under the handguard's nose, legs splayed and raked forward."""
    g.add(m, lg.box(47.0, GUARD_END - 0.2, -5.6, -4.6, 3.6, bevel=0.2, mat=mat))
    g.add(m, lg.pin(48.6, -5.3, -2.2, 2.2, 0.4, mat=lg.STEEL, segments=12))
    for side in (-1.0, 1.0):
        top, knee, foot = (48.6, -5.3, side * 1.5), (53.4, -10.6, side * 3.4), (57.2, -14.8, side * 4.9)
        g.add(m, lg.pipe([top, knee], 0.48, mat=mat, sides=10))
        g.add(m, lg.pipe([knee, foot], 0.34, mat=lg.STEEL, sides=10))
        g.add(m, lg.pipe([(53.2, -10.4, side * 3.33), (53.7, -10.95, side * 3.53)], 0.6, mat=mat, sides=10))   # its lock
        g.add(m, lg.pipe([(56.9, -14.5, side * 4.8), (57.6, -15.3, side * 5.05)], 0.62, mat=lg.RUBBER, sides=10))


def side_panels(m, mat, rivet=lg.BRASS, w_off=0.06):
    for side in (-1.0, 1.0):
        for o in (((1.4, -3.9), (14.6, -3.9), (14.6, 1.8), (1.4, 1.8)), ((28.4, -3.9), (36.6, -3.9), (36.6, 1.8),
                                                                       (28.4, 1.8))):
            g.add(m, lg.slab(list(o), 0.2, w=side * (HALF + w_off), bevel=0.05, mat=mat, round=0.5))


def scroll(m, cu, cv, r0, side, w, turns=1.6, mat=lg.BRASS):
    pts = []
    for k in range(18):
        t = k / 17.0
        a = 2.0 * math.pi * turns * t
        r = r0 * (1.0 - 0.75 * t)
        pts.append((cu + side * r * math.cos(a), cv + r * math.sin(a), side * w))
    g.add(m, lg.pipe(pts, 0.07, mat=mat, sides=5))


def bodies():
    B = lambda key: model('Body', key, (0.0, 0.0))

    m = B('Standard')                                          # as drawn: gunmetal, a sand top cover, brass rivets
    receiver(m)
    grip(m)
    fittings(m)

    m = B('Ironside')                                          # a gunmetal cover, sand side panels
    receiver(m, cover=METAL)
    side_panels(m, SHELL)
    grip(m)
    fittings(m, handle=DARK, accent=(15.6, 28.0))

    m = B('Scrapper')                                          # rusty plates welded over it, a taped grip
    receiver(m, mat=lg.SALVAGE, cover=lg.SALVAGE)
    for side in (-1.0, 1.0):
        w = side * (HALF + 0.12)
        for u0, u1 in ((0.8, 14.8), (28.2, 37.2)):
            g.add(m, lg.slab([(u0, -4.2), (u1, -4.2), (u1, 1.8), (u0, 1.8)], 0.25, w=w, bevel=0.04, mat=lg.SALVAGE))
            for u in range(int(u0) + 1, int(u1), 3):
                for v in (-3.7, 1.3):
                    g.add(m, lg.rivet(u + 0.4, v, w + side * 0.14, side, r=0.26, mat=lg.STEEL))
    grip(m, lg.TAPE)
    for v in (-6.6, -8.8, -11.0, -13.0):
        ur, uf = 4.1 + (v + 4.5) * 0.24 - 0.2, 10.2 + (v + 5.0) * 0.3 + 0.2
        g.add(m, lg.slab([(ur, v - 0.2), (uf, v), (uf, v + 0.4), (ur, v + 0.2)], 3.35, bevel=0.1, mat=lg.TAPE))
    fittings(m, rivets=lg.STEEL, handle=lg.STEEL, guard=lg.STEEL, handguard=lg.SALVAGE)

    m = B('Vented')                                            # ember shows through slots across the cover
    slots = [lg.cbox(u, u + 0.9, 3.6, 6.0, -1.8, 1.8) for u in (29.0, 30.6, 32.2, 33.8)]
    receiver(m, cover_cuts=slots)
    g.add(m, lg.box(28.6, 35.0, 2.6, 4.0, 3.8, bevel=0.1, mat=lg.EMBER))
    grip(m)
    fittings(m)

    m = B('Skeleton')                                          # a window onto the brass bolt and its spring
    receiver(m, extra_cuts=[lg.cslab([(3.0, -3.4), (14.4, -3.4), (14.4, 1.4), (3.0, 1.4)], -4.0, 4.0, round=0.9)])
    g.add(m, lg.box(2.6, 9.6, -2.6, 1.0, 3.0, bevel=0.3, mat=lg.BRASS))                      # the bolt
    g.add(m, lg.tube(9.4, 14.8, 0.5, v=-0.8, mat=lg.STEEL))
    g.add(m, lg.helix(9.6, 14.6, 0.6, 0.95, 0.8, 0.4, v=-0.8, steps=12, mat=lg.STEEL))
    grip(m)
    fittings(m, bolt=lg.BRASS)

    m = B('Armored')                                           # olive armor bolted over the sides and cover
    receiver(m)
    for side in (-1.0, 1.0):
        w = side * (HALF + 0.35)
        for o in (((0.6, -4.4), (15.0, -4.4), (15.0, 2.4), (0.6, 2.4)), ((28.0, -4.4), (37.4, -4.4), (37.4, 2.4),
                                                                          (28.0, 2.4))):
            g.add(m, lg.slab(list(o), 0.7, w=w, bevel=0.15, mat=OLIVE, round=0.6))
        for u, v in ((1.4, -3.7), (14.2, -3.7), (1.4, 1.7), (14.2, 1.7), (28.8, -3.7), (36.6, -3.7), (28.8, 1.7), (36.6, 1.7)):
            g.add(m, lg.rivet(u, v, side * (HALF + 0.7), side, r=0.34, mat=lg.STEEL))
    g.add(m, lg.slab([(2.0, COVER - 0.2), (8.8, COVER - 0.2), (8.4, COVER + 0.5), (2.4, COVER + 0.5)], 4.6, bevel=0.2,
                     mat=OLIVE, round=0.3))
    grip(m)
    fittings(m, rivets=lg.STEEL, handguard=OLIVE, accent=(15.6, 28.0))

    m = B('Engraved')                                          # silvered side plates scrolled in brass
    receiver(m)
    side_panels(m, SILVER)
    for side in (-1.0, 1.0):
        w = HALF + 0.17
        for cu, cv, r in ((5.0, -1.0, 1.9), (10.6, -1.6, 1.4), (32.4, -1.2, 1.6)):
            scroll(m, cu, cv, r, side, w)
    grip(m)
    for side in (-1.0, 1.0):
        g.add(m, lg.pin(5.6, -9.4, side * 1.6 - 0.07, side * 1.6 + 0.07, 0.85, mat=lg.BRASS, segments=16))
    fittings(m, handle=lg.BRASS, guard=lg.BRASS, trigger=lg.BRASS, accent=(15.6, 28.0))

    m = B('Forge')                                             # glass eyes onto an ember core, brass fittings
    eyes = ((8.0, -1.0, 2.0), (32.6, -1.0, 1.7))
    receiver(m, extra_cuts=[lg.cpin(u, v, -4.0, 4.0, r, segments=24) for u, v, r in eyes])
    for u, v, r in eyes:
        g.add(m, lg.pin(u, v, -HALF + 0.3, HALF - 0.3, r - 0.1, mat=lg.EMBER, segments=24))
        for side in (-1.0, 1.0):
            w0 = HALF - 0.25 if side > 0 else -HALF - 0.25
            bezel = lg.pin(u, v, w0, w0 + 0.5, r + 0.5, mat=lg.BRASS, segments=24)
            g.add(m, lg.cut(bezel, [lg.cpin(u, v, w0 - 0.2, w0 + 0.7, r - 0.05, segments=24)]))
            w1 = side * (HALF - 0.15)
            g.add(m, lg.pin(u, v, w1 - 0.04, w1 + 0.04, r - 0.02, mat=lg.LENS, segments=24))
    for side in (-1.0, 1.0):
        g.add(m, lg.box(11.0, 28.0, -3.9, -3.7, 0.08, w=side * (HALF + 0.01), bevel=0.0, mat=lg.EMBER))
    grip(m)
    fittings(m, handle=lg.BRASS, guard=lg.BRASS, trigger=lg.BRASS, bolt=lg.BRASS, accent=(11.0, 28.0))


# --- Barrels: heavy, ringed with heat ---

def barrels():
    u0 = BARREL_AT[0]
    B = lambda key: model('Barrel', key, BARREL_AT)

    def barrel(m, length, r=1.8, tip=6.0, tip_r=1.05, mat=METAL, rings=(), ring_mat=METAL, collar=True):
        """The tube from its nut, stepping down to its thin tip section, with glowing heat rings in collars."""
        g.add(m, lg.turned([(0.0, 2.15), (1.4, 2.15), (1.7, r), (length - tip, r), (length - tip + 0.6, tip_r),
                            (length - 0.2, tip_r), (length, tip_r - 0.15)], u0, segments=24, mat=mat))
        heat = []
        for p in rings:
            heat.append(lg.tube(u0 + p, u0 + p + 1.1, r + 0.14, segments=24, mat=lg.HEAT_GLOW))
            if collar:
                for a, b in ((p - 0.5, p), (p + 1.1, p + 1.6)):
                    g.add(m, lg.tube(u0 + a, u0 + b, r + 0.36, segments=24, mat=ring_mat))
        for h in heat:
            g.add(m, lg.along_uv(h, u0, u0 + length))
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    barrel(B('Standard'), 34.0, rings=(14.0, 18.0, 22.0, 26.0))       # as drawn: four rings
    barrel(B('Short'), 26.0, tip=4.0, rings=(13.5, 17.0, 20.5))      # three rings, a short reach

    m = B('Fluted')                                            # a steel barrel fluted between its rings
    body = lg.turned([(0.0, 2.15), (1.4, 2.15), (1.7, 1.8), (28.0, 1.8), (28.6, 1.05), (33.8, 1.05), (34.0, 0.9)], u0,
                     segments=24, mat=lg.STEEL)
    g.add(m, lg.cut(body, [lg.cbox(u0 + 12.6, u0 + 27.4, 1.45, 2.2, -0.26, 0.26, rotate_u=a) for a in range(0, 360, 45)]))
    heat = [lg.tube(u0 + p, u0 + p + 1.1, 1.94, segments=24, mat=lg.HEAT_GLOW) for p in (14.0, 20.0, 26.0)]
    for p in (14.0, 20.0, 26.0):
        for a, b in ((p - 0.5, p), (p + 1.1, p + 1.6)):
            g.add(m, lg.tube(u0 + a, u0 + b, 2.16, segments=24, mat=METAL))
    for h in heat:
        g.add(m, lg.along_uv(h, u0, u0 + 34.0))
    g.socket(m, 'Muzzle', (u0 + 34.0, 0.0))

    m = B('Jacketed')                                          # a perforated cooling jacket over a glowing barrel
    barrel(m, 34.0)
    jacket = lg.turned([(0.0, 2.35), (19.0, 2.35), (19.6, 1.9)], u0 + 12.0, segments=24, mat=METAL)
    holes = []
    for k in range(7):
        u = u0 + 13.4 + k * 2.5
        holes += [lg.cpin(u, 0.0, -3.0, 3.0, 0.55, segments=10), lg.upright(u + 1.25, -3.0, 3.0, 0.55, segments=10)]
    g.add(m, lg.cut(jacket, holes))
    core = lg.tube(u0 + 12.4, u0 + 31.0, 1.9, segments=20, mat=lg.HEAT_GLOW)
    g.add(m, lg.along_uv(core, u0, u0 + 34.0))
    for u in (12.0, 31.0):
        g.add(m, lg.tube(u0 + u - 0.3, u0 + u + 0.5, 2.45, segments=24, mat=lg.STEEL))

    m = B('Finned')                                            # a radiator of fins, the heat glowing between them
    barrel(m, 32.0)
    for k in range(16):
        u = u0 + 12.2 + k * 0.95
        g.add(m, lg.tube(u, u + 0.32, 2.75, segments=24, mat=METAL))
    core = lg.tube(u0 + 12.0, u0 + 27.6, 1.95, segments=20, mat=lg.HEAT_GLOW)
    g.add(m, lg.along_uv(core, u0, u0 + 32.0))
    g.add(m, lg.tube(u0 + 27.4, u0 + 28.2, 2.2, segments=24, mat=lg.BRASS))

    barrel(B('Heavy'), 38.0, r=2.1, tip=6.0, tip_r=1.2, rings=(13.5, 17.5, 21.5, 25.5, 29.5))   # a bull barrel

    m = B('Gilded')                                            # steel, brass collars, engraved bands
    barrel(m, 36.0, mat=lg.STEEL, rings=(14.0, 18.5, 23.0, 27.5), ring_mat=lg.BRASS)
    for p in (16.5, 21.0, 25.5):
        g.add(m, lg.tube(u0 + p - 0.2, u0 + p + 0.2, 1.86, segments=24, mat=lg.BRASS))
    g.add(m, lg.tube(u0 + 34.6, u0 + 36.0, 1.12, segments=20, mat=lg.BRASS))

    m = B('Furnace')                                           # a slotted brass-capped jacket over a white-hot core
    barrel(m, 36.0)
    jacket = lg.turned([(0.0, 2.45), (20.0, 2.45), (20.8, 2.0)], u0 + 11.6, segments=24, mat=DARK)
    g.add(m, lg.cut(jacket, [lg.cbox(u0 + 13.0, u0 + 30.6, 1.4, 3.0, -0.46, 0.46, rotate_u=a) for a in range(0, 360, 60)]))
    core = lg.tube(u0 + 12.0, u0 + 32.0, 2.0, segments=20, mat=lg.HEAT_GLOW)
    g.add(m, lg.along_uv(core, u0, u0 + 36.0))
    for u in (11.3, 31.6):
        g.add(m, lg.tube(u0 + u, u0 + u + 1.0, 2.6, segments=24, mat=lg.BRASS))


# --- Muzzle devices ---

def muzzles():
    u0 = MUZZLE_AT[0]
    M = lambda key: model('Muzzle', key, MUZZLE_AT)

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    m = M('Cap')                                               # as drawn: a brass cap
    g.add(m, lg.turned([(0, 1.18), (1.6, 1.18), (2.0, 0.92)], u0, segments=20, mat=lg.BRASS))
    tip(m, 2.0)

    m = M('Cone')                                              # a flared flash cone
    g.add(m, lg.turned([(0, 1.15), (1.0, 1.15), (4.4, 1.75), (4.8, 1.55)], u0, segments=24, mat=METAL))
    tip(m, 4.8)

    m = M('Slotted')                                           # a slotted flash hider, closed beneath
    cage = lg.turned([(0, 1.3), (6.0, 1.3), (6.4, 1.1)], u0, segments=24, mat=DARK)
    g.add(m, lg.cut(cage, [lg.cbox(u0 + 1.6, u0 + 5.6, 0.7, 1.7, -0.2, 0.2, rotate_u=a) for a in (-72, -36, 0, 36, 72)]))
    tip(m, 6.4)

    m = M('Booster')                                           # a recoil booster's bell
    g.add(m, lg.turned([(0, 1.3), (0.8, 2.05), (5.6, 2.05), (6.2, 1.6), (6.8, 1.6), (7.0, 1.3)], u0, segments=28, mat=DARK))
    g.add(m, lg.tube(u0 + 0.9, u0 + 1.4, 2.12, segments=28, mat=lg.STEEL))
    tip(m, 7.0)

    m = M('Brake')                                             # a big three-port brake on a brass ring
    brake = lg.box(u0 + 0.6, u0 + 7.4, -1.7, 1.7, 3.8, bevel=0.4, mat=DARK)
    g.add(m, lg.cut(brake, [lg.cbox(u0 + u, u0 + u + 1.1, -1.25, 1.25, -3, 3) for u in (1.6, 3.6, 5.6)]))
    g.add(m, lg.tube(u0, u0 + 0.7, 1.35, segments=20, mat=lg.BRASS))
    tip(m, 7.4)

    m = M('Can')                                               # a fat suppressor in a sand sleeve
    g.add(m, lg.turned([(0, 1.2), (0.8, 2.2), (12.4, 2.2), (13.2, 1.6)], u0, segments=28, mat=DARK))
    g.add(m, lg.tube(u0 + 2.0, u0 + 10.6, 2.28, segments=28, mat=SHELL))
    for u in (1.4, 11.2):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.5, 2.32, segments=28, mat=lg.STEEL))
    tip(m, 13.2)

    m = M('Flare')                                             # a brass bell with an ember throat
    g.add(m, lg.turned([(0, 1.2), (1.2, 1.2), (5.0, 2.3), (5.6, 2.45), (5.9, 2.2)], u0, segments=32, mat=lg.BRASS))
    g.add(m, lg.tube(u0 + 5.85, u0 + 5.95, 1.7, segments=24, mat=lg.EMBER))
    g.add(m, lg.tube(u0 + 0.2, u0 + 0.9, 1.3, segments=20, mat=DARK))
    tip(m, 5.9)


# --- Magazines: the belt box and its belt ---

BOX_U0, BOX_U1 = 15.4, 30.6
BOX_W0, BOX_W1 = -2.4, 4.4


def belt(m, rounds=3, case=lg.BRASS, tip=lg.COPPER):
    """The rounds climbing from the box's top-left corner into the feed tray, each held in a link."""
    for k in range(rounds):
        v = BOT + 0.6 + k * 1.05
        g.add(m, *lg.cartridge(BELT_U, v, BELT_W, length=5.6, r=0.46, case=case, tip=tip))
        g.add(m, lg.tube(BELT_U + 1.3, BELT_U + 2.5, 0.56, v=v, w=BELT_W, segments=10, mat=DARK))
    for u in (BELT_U + 1.5, BELT_U + 2.2):   # the links' spine on the outside
        g.add(m, lg.box(u, u + 0.4, BOT + 0.1, BOT + 0.6 + (rounds - 1) * 1.05 + 0.4, 0.14, w=BELT_W + 0.56, bevel=0.0,
                        mat=DARK))


def box(m, depth, mat=GRIPS, lid=METAL, w0=BOX_W0, w1=BOX_W1, handle=True, ribs=True, round=0.5):
    """The box from the receiver's foot down depth cm, its lid, latch, side ribs and a strap handle on the left face."""
    wc, width = (w0 + w1) * 0.5, w1 - w0
    bot = BOT - depth
    g.add(m, lg.slab([(BOX_U0, BOT - 0.5), (BOX_U1, BOT - 0.5), (BOX_U1, bot + 0.8), (BOX_U1 - 0.8, bot),
                      (BOX_U0 + 0.8, bot), (BOX_U0, bot + 0.8)], width, w=wc, bevel=0.35, segments=2, mat=mat,
                     round=round))
    g.add(m, lg.box(BOX_U0 - 0.2, BOX_U1 + 0.2, BOT - 0.9, BOT, width + 0.3, w=wc, bevel=0.2, mat=lid))
    g.add(m, lg.box(BOX_U1 + 0.1, BOX_U1 + 0.7, BOT - 2.6, BOT - 0.6, 1.6, w=wc, bevel=0.15, mat=lg.STEEL))   # latch
    if ribs:
        for v in (BOT - 2.4, bot + 1.6):
            g.add(m, lg.box(BOX_U0 + 0.6, BOX_U1 - 0.6, v - 0.25, v + 0.25, width + 0.25, w=wc, bevel=0.1, mat=mat))
    if handle:
        wf = w1 + 0.1
        g.add(m, lg.pipe([(19.4, BOT - 1.8, wf), (20.0, BOT - 2.8, wf + 0.8), (26.0, BOT - 2.8, wf + 0.8),
                          (26.6, BOT - 1.8, wf)], 0.3, mat=lg.STEEL))
    return bot


def count_window(m, v, w, u0=BOX_U0 + 1.4, u1=BOX_U1 - 1.4, frame=lg.BRASS):
    """A glowing count window along the left face (GunAmmoGlow, lit from the back by the rounds left)."""
    glow = lg.box(u0, u1, v - 0.4, v + 0.4, 0.2, w=w + 0.05, bevel=0.0, mat=lg.AMMO_GLOW)
    g.add(m, lg.along_uv(glow, u0, u1, v=v))
    g.add(m, lg.box(u0 - 0.4, u1 + 0.4, v - 0.75, v + 0.75, 0.16, w=w + 0.02, bevel=0.05, mat=frame))
    for u in range(int(u0) + 2, int(u1), 3):   # tick marks over the glow
        g.add(m, lg.box(u, u + 0.12, v - 0.42, v + 0.42, 0.06, w=w + 0.18, bevel=0.0, mat=DARK))


def element_boxes(M):
    """The element boxes (2026-10-22): the 100's box and belt, dressed in their element (the belt's bullets too). The
    strap handle crosses the left face at v -6.4..-7.4 and the ribs stand at -7.0 and -13.0: the dress sits between."""
    bot = BOT - 10.0
    faces = ((1.0, BOX_W1), (-1.0, BOX_W0))   # (side, w) of the left and right faces

    # Heat-scored steel, a temper-blued lid and ribs, ember vents in both faces with the heat behind, ember-hot bullets.
    m = M('Ember100')
    box(m, 10.0, mat=lg.SCORCHED, lid=lg.TEMPER)
    cuts = []
    for u in (17.6, 20.6, 23.6, 26.6):
        slot = [(u, bot + 2.4), (u + 1.1, bot + 2.4), (u + 2.2, BOT - 3.4), (u + 1.1, BOT - 3.4)]
        cuts += [lg.cslab(slot, BOX_W1 - 0.35, BOX_W1 + 1.0, round=0.3), lg.cslab(slot, BOX_W0 - 1.0, BOX_W0 + 0.35,
                                                                               round=0.3)]
    g.parts[m][0] = lg.cut(g.parts[m][0], cuts)
    for side, w in faces:
        g.add(m, lg.box(17.0, 29.4, bot + 2.2, BOT - 3.0, 0.04, w=w - side * 0.31, bevel=0.0, mat=lg.EMBER))
        g.add(m, lg.box(BOX_U0 + 0.6, BOX_U1 - 0.6, bot + 0.55, bot + 0.75, 0.05, w=w + side * 0.02, bevel=0.0,
                        mat=lg.EMBER))                                           # a hot seam along the foot
    belt(m, tip=lg.EMBER)

    # Its own grey box in pale gold: gilt corner guards, a lantern on the left face between gilt curls, a saint's medal
    # on the right, a gold-white glow band under the lid, saint-lit bullets.
    m = M('Saint100')
    box(m, 10.0, lid=lg.GILT)
    for u in (BOX_U0 - 0.1, BOX_U1 - 1.5):
        for v in (BOT - 2.0, bot + 0.2):
            g.add(m, lg.box(u, u + 1.6, v, v + 1.8, BOX_W1 - BOX_W0 + 0.6, w=1.0, bevel=0.2, mat=lg.GILT))
    for side, w in faces:
        g.add(m, lg.box(BOX_U0 + 1.4, BOX_U1 - 1.4, BOT - 1.45, BOT - 1.05, 0.12, w=w + side * 0.03, bevel=0.0,
                        mat=lg.SOUL))
    g.add(m, *lg.lantern((23.0, BOT - 5.9, BOX_W1 + 0.02), (0.0, 0.0, 1.0), 4.6))
    for flip, cu in ((1.0, 19.6), (-1.0, 26.4)):   # curls either side of the lantern, mirrored
        g.add(m, lg.curl((cu, BOT - 6.6, BOX_W1), (0.0, 0.0, 1.0), 1.3, flip=flip, thick=0.08))
        g.add(m, lg.curl((cu + flip * 0.3, BOT - 7.4, BOX_W1), (0.0, 0.0, 1.0), 0.6, flip=-flip, start=180.0,
                         thick=0.07))
    g.add(m, *lg.medal((23.0, BOT - 5.9, BOX_W0), (0.0, 0.0, -1.0), 1.9, up=(0.0, 1.0, 0.0)))
    for flip, cu in ((1.0, 19.8), (-1.0, 26.2)):
        g.add(m, lg.curl((cu, BOT - 6.0, BOX_W0), (0.0, 0.0, -1.0), 1.1, flip=flip, thick=0.08))
    belt(m, tip=lg.SOUL)

    # Bare steel crusted with salt: crystals along its foot and up its front corners, cyan-white frost veins, a salted
    # lid, salt-white bullets.
    m = M('Salt100')
    box(m, 10.0, mat=lg.STEEL, lid=lg.SALT)
    for k, (u, w, n) in enumerate(((17.0, BOX_W1, (0.0, -0.5, 1.0)), (24.0, BOX_W1, (0.0, -0.6, 1.0)),
                                   (29.6, BOX_W1, (0.6, -0.5, 1.0)), (18.0, BOX_W0, (0.0, -0.5, -1.0)),
                                   (27.5, BOX_W0, (0.4, -0.5, -1.0)))):
        g.add(m, *lg.salt_cluster(900 + k, (u, bot + 0.9, w), n, 1.8, 6, length=(1.05, 2.62), r=(0.27, 0.51),
                                  up=(1.0, 0.0, 0.0), squash=0.5))
    for k, v in enumerate((BOT - 4.5, BOT - 8.0)):   # up the front corner
        g.add(m, *lg.salt_cluster(910 + k, (BOX_U1 + 0.05, v, BOX_W1 - 0.4), (1.0, 0.0, 0.5), 0.9, 3,
                                  length=(0.88, 1.75), r=(0.21, 0.39)))
    g.add(m, *lg.salt_cluster(915, (23.0, bot - 0.05, 1.0), (0.0, -1.0, 0.0), 3.0, 6, length=(0.88, 2.1),
                              r=(0.24, 0.45), up=(1.0, 0.0, 0.0), squash=0.6))
    for side, w in faces:
        vein = [(16.6, BOT - 3.2), (18.4, BOT - 4.8), (17.4, BOT - 6.6), (19.8, BOT - 8.4), (18.8, bot + 2.2)]
        g.add(m, lg.pipe([(u, v, w + side * 0.02) for u, v in vein], 0.1, mat=lg.SALT_GLOW, sides=4))
        vein = [(28.8, BOT - 3.4), (27.2, BOT - 5.6), (28.4, BOT - 7.6), (26.6, BOT - 9.4)]
        g.add(m, lg.pipe([(u, v, w + side * 0.02) for u, v in vein], 0.08, mat=lg.SALT_GLOW, sides=4))
    belt(m, tip=lg.SALT_GLOW)


def magazines():
    M = lambda key: model('Magazine', key, MAG_AT)

    m = M('80')                                                # a short box
    box(m, 7.5, handle=False)
    belt(m)

    m = M('100')                                               # as drawn
    box(m, 10.0)
    belt(m)

    m = M('100Window')                                         # a window onto the rows of brass inside
    box(m, 10.0, handle=False)
    g.parts[m][0] = lg.cut(g.parts[m][0], [lg.cbox(17.2, 28.8, BOT - 9.0, BOT - 2.6, BOX_W1 - 0.7, BOX_W1 + 1.0)])
    for row in range(5):
        v = BOT - 3.3 - row * 1.15
        for u in (17.4, 23.2):
            g.add(m, *lg.cartridge(u, v, BOX_W1 - 0.75, length=5.5, r=0.46))
    g.add(m, lg.box(17.0, 29.0, BOT - 9.2, BOT - 2.4, 0.1, w=BOX_W1 - 0.05, bevel=0.0, mat=lg.LENS))
    belt(m)

    m = M('120')                                               # a deep box with sand panels
    bot = box(m, 12.5)
    for side, w in ((-1.0, BOX_W0 - 0.05), (1.0, BOX_W1 + 0.05)):
        g.add(m, lg.slab([(17.0, BOT - 3.2), (29.0, BOT - 3.2), (29.0, bot + 2.4), (17.0, bot + 2.4)], 0.12, w=w,
                         bevel=0.03, mat=SHELL, round=0.4))
    belt(m)

    m = M('Pouch')                                             # a soft canvas pouch, quick to rip off
    g.add(m, lg.slab([(BOX_U0 + 0.2, BOT - 0.6), (BOX_U1 - 0.2, BOT - 0.6), (BOX_U1 + 0.4, BOT - 6.0),
                      (BOX_U1 - 1.0, BOT - 10.4), (BOX_U0 + 1.4, BOT - 10.6), (BOX_U0 - 0.2, BOT - 6.0)], 6.6, w=1.0,
                     bevel=1.4, segments=3, mat=lg.TAPE, round=2.0))
    g.add(m, lg.box(BOX_U0, BOX_U1, BOT - 0.9, BOT, 6.6, w=1.0, bevel=0.2, mat=METAL))
    for u in (18.6, 26.0):   # straps and buckles
        g.add(m, lg.box(u, u + 1.4, BOT - 9.0, BOT - 0.8, 7.2, w=1.0, bevel=0.15, mat=LEATHER))
        g.add(m, lg.box(u - 0.2, u + 1.6, BOT - 5.2, BOT - 4.0, 0.2, w=BOX_W1 + 0.55, bevel=0.05, mat=lg.BRASS))
    belt(m)

    m = M('150')                                               # a big box with brass corner guards
    bot = box(m, 12.0, lid=DARK, w0=BOX_W0 - 0.5, w1=BOX_W1 + 0.3)
    for u in (BOX_U0 - 0.1, BOX_U1 - 1.5):
        for v in (BOT - 2.0, bot + 0.2):
            g.add(m, lg.box(u, u + 1.6, v, v + 1.8, BOX_W1 - BOX_W0 + 1.2, w=1.0 - 0.1, bevel=0.2, mat=lg.BRASS))
    belt(m, rounds=3)

    m = M('Drum')                                              # a drum hung under the feed, a glowing count window
    g.add(m, lg.box(BOX_U0 + 2.0, BOX_U1 - 2.0, BOT - 2.8, BOT, 5.4, w=1.0, bevel=0.3, mat=METAL))
    g.add(m, lg.pin(23.0, BOT - 7.8, -2.2, 4.2, 6.6, mat=GRIPS, segments=40,
                    profile=[(0, 6.0), (0.5, 6.6), (5.9, 6.6), (6.4, 6.0)]))
    for w0 in (-2.4, 4.2):
        g.add(m, lg.pin(23.0, BOT - 7.8, w0, w0 + 0.2, 2.4, mat=lg.STEEL, segments=20))
    count_window(m, BOT - 4.6, 4.2, u0=19.0, u1=27.0)
    belt(m)

    m = M('Brass')                                             # a brass-clad box with a glowing count window
    box(m, 12.0, mat=lg.BRASS, lid=METAL)
    count_window(m, BOT - 6.6, BOX_W1, frame=METAL)
    for side, w in ((-1.0, BOX_W0), (1.0, BOX_W1)):
        scroll(m, 19.6, BOT - 10.0, 1.3, side, abs(w) + 0.05)
        scroll(m, 26.4, BOT - 10.0, 1.3, -side, abs(w) + 0.05)
    belt(m)
    element_boxes(M)


# --- Sights: on the carry handle's rail (u 11.4..26.6) ---

def sights():
    S = lambda key: model('Sight', key, SIGHT_AT)
    uc, v0 = SIGHT_AT
    lg.frontier_sight(g, S('Iron'), 'Iron', uc, v0, span=12.0)
    lg.frontier_sight(g, S('Pip'), 'Pip', uc, v0)
    lg.frontier_sight(g, S('Reflex'), 'Reflex', uc, v0)
    lg.frontier_sight(g, S('Holo'), 'Holo', uc, v0)
    lg.frontier_sight(g, S('Prism'), 'Prism', uc, v0)
    lg.frontier_sight(g, S('Scope'), 'Scope', uc, v0, zoom=3.0)
    lg.frontier_sight(g, S('Tracker'), 'Scope', uc, v0, zoom=4.0, trim=lg.BRASS)


# --- Stocks ---

FIXED = [(0.3, 4.6), (-21.0, 3.8), (-22.6, 3.0), (-22.6, -9.4), (-21.6, -10.2), (-17.0, -9.4), (-2.0, -4.6), (0.3, -4.4)]
HOLE = [(-3.4, 2.2), (-19.4, 1.8), (-19.6, -6.6), (-16.6, -6.8), (-3.4, -1.8)]


def pad(m, u=-22.6, thick=1.6, mat=lg.RUBBER):
    g.add(m, lg.slab([(u + 0.2, 3.3), (u - thick, 3.1), (u - thick, -9.6), (u + 0.2, -10.0)], 4.8, bevel=0.5, segments=3,
                     mat=mat, round=0.5))


def fixed(m, mat=GRIPS, hole=False):
    obj = lg.slab(FIXED, 4.4, bevel=0.7, segments=3, mat=mat, round=1.2)
    g.add(m, lg.cut(obj, [lg.cslab(HOLE, -3, 3, round=1.2)]) if hole else obj)
    g.add(m, lg.box(-0.8, 0.4, -4.2, 4.2, 5.0, bevel=0.3, mat=METAL))                       # the buttplate's mount


def stocks():
    S = lambda key: model('Stock', key, STOCK_AT)

    m = S('Fixed')                                             # as drawn: a solid polymer stock
    fixed(m)
    pad(m)

    m = S('Skeleton')                                          # a sand skeleton stock
    fixed(m, mat=SHELL, hole=True)
    pad(m)

    m = S('Buffer')                                            # a buffer tube, its spring bare, a sand butt
    g.add(m, lg.tube(-17.0, 0.4, 1.5, v=0.4, segments=24, mat=METAL))
    g.add(m, lg.helix(-15.6, -3.0, 1.45, 1.8, 1.1, 0.45, v=0.4, steps=16, mat=lg.STEEL))
    g.add(m, lg.slab([(-16.0, 3.6), (-20.8, 3.4), (-21.6, 2.6), (-21.6, -8.6), (-20.6, -9.4), (-17.4, -8.6),
                      (-16.0, -2.0)], 4.4, bevel=0.6, segments=3, mat=SHELL, round=1.0))
    g.add(m, lg.box(-0.8, 0.4, -4.2, 4.2, 5.0, bevel=0.3, mat=METAL))
    pad(m, u=-21.6)

    m = S('Cheek')                                             # a leather cheek riser on the fixed stock
    fixed(m)
    pad(m)
    g.add(m, lg.slab([(-5.0, 3.6), (-17.0, 3.2), (-17.4, 5.0), (-5.4, 5.4)], 4.6, bevel=0.7, segments=3, mat=LEATHER,
                     round=0.9))
    for u in (-6.4, -15.6):
        g.add(m, lg.box(u, u + 0.45, 3.0, 5.5, 4.75, bevel=0.1, mat=lg.STEEL))

    m = S('Monopod')                                           # a rear monopod under the butt
    fixed(m)
    pad(m)
    g.add(m, lg.box(-20.2, -17.2, -10.6, -9.2, 2.6, bevel=0.3, mat=METAL))
    g.add(m, lg.upright(-18.7, -17.6, -10.4, 0.5, mat=lg.STEEL))
    g.add(m, lg.upright(-18.7, -14.0, -10.4, 0.7, mat=METAL))
    g.add(m, lg.upright(-18.7, -18.6, -17.4, 1.1, mat=lg.RUBBER, segments=16))

    m = S('RearGrip')                                          # a support grip under the stock for the off hand
    fixed(m)
    pad(m)
    g.add(m, lg.slab([(-10.0, -7.2), (-13.6, -8.2), (-14.6, -13.0), (-13.6, -14.2), (-11.6, -14.2), (-11.0, -9.0),
                      (-9.0, -6.6)], 3.0, bevel=0.6, segments=3, mat=GRIPS, round=0.6))

    m = S('Engraved')                                          # sand, a brass butt plate, scrolled brass inlay
    fixed(m, mat=SHELL)
    pad(m, thick=1.0, mat=lg.BRASS)
    for side in (-1.0, 1.0):
        scroll(m, -9.0, -1.0, 1.8, side, 2.21)
        scroll(m, -16.0, -3.0, 1.5, -side, 2.21)
        g.add(m, lg.pipe([(-1.0, 3.4, side * 2.22), (-20.0, 2.7, side * 2.22)], 0.08, mat=lg.BRASS, sides=5))


DEFAULT = {'Body': 'Standard', 'Barrel': 'Standard', 'Muzzle': 'Cap', 'Magazine': '100', 'Sight': 'Reflex',
           'Stock': 'Fixed'}
SLOTS = {
    'Body': ['Standard', 'Ironside', 'Scrapper', 'Vented', 'Skeleton', 'Armored', 'Engraved', 'Forge'],
    'Barrel': ['Standard', 'Short', 'Fluted', 'Jacketed', 'Finned', 'Heavy', 'Gilded', 'Furnace'],
    'Muzzle': ['Cap', 'Cone', 'Slotted', 'Booster', 'Brake', 'Can', 'Flare'],
    'Magazine': ['80', '100', '100Window', '120', 'Pouch', '150', 'Drum', 'Brass', 'Ember100', 'Saint100', 'Salt100'],
    'Sight': ['Iron', 'Pip', 'Reflex', 'Holo', 'Prism', 'Scope', 'Tracker'],
    'Stock': ['Fixed', 'Skeleton', 'Buffer', 'Cheek', 'Monopod', 'RearGrip', 'Engraved'],
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
    s = next(c for c in obj.children if c.name == 'SOCKET_' + name or c.name.startswith('SOCKET_' + name + '.'))
    return obj.location + s.location


def assemble(choice, at=Vector()):
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
    shown = []
    z = 0.0
    for slot, keys in slot_keys.items():
        y, tallest = 0.0, 0.0
        for key in keys:
            obj = built[f'{slot}_{key}']
            lo_y, hi_y = min(b[1] for b in obj.bound_box), max(b[1] for b in obj.bound_box)
            lo_z, hi_z = min(b[2] for b in obj.bound_box), max(b[2] for b in obj.bound_box)
            obj.location = Vector((0.0, y - hi_y, z - hi_z))
            y -= (hi_y - lo_y) + gap
            tallest = max(tallest, hi_z - lo_z)
            shown.append(obj)
        z -= tallest + gap
    bpy.context.view_layer.update()
    return shown


assemble(DEFAULT)
for row, (slot, keys) in enumerate(SLOTS.items()):
    for k, key in enumerate(keys):
        if key != DEFAULT[slot]:
            built[f'{slot}_{key}'].location = Vector((0.0, -k * 0.6, (row + 1) * 0.4))

VARIANTS = {
    'Thresher_Uncommon': {'Body': 'Vented', 'Barrel': 'Fluted', 'Muzzle': 'Slotted', 'Magazine': '100Window',
                          'Sight': 'Holo', 'Stock': 'Skeleton'},
    'Thresher_Rare': {'Body': 'Skeleton', 'Barrel': 'Jacketed', 'Muzzle': 'Booster', 'Magazine': 'Pouch',
                      'Sight': 'Prism', 'Stock': 'Monopod'},
    'Thresher_Epic': {'Body': 'Engraved', 'Barrel': 'Gilded', 'Muzzle': 'Flare', 'Magazine': 'Drum', 'Sight': 'Tracker',
                      'Stock': 'Engraved'},
    'Thresher_Legendary': {'Body': 'Forge', 'Barrel': 'Furnace', 'Muzzle': 'Flare', 'Magazine': 'Brass',
                           'Sight': 'Tracker', 'Stock': 'Cheek'},
    'Thresher_Scrap': {'Body': 'Scrapper', 'Barrel': 'Short', 'Muzzle': 'Cone', 'Magazine': '80', 'Sight': 'Iron',
                       'Stock': 'Buffer'},
    'Thresher_Heavy': {'Body': 'Armored', 'Barrel': 'Heavy', 'Muzzle': 'Brake', 'Magazine': '150', 'Sight': 'Scope',
                       'Stock': 'RearGrip'},
    'Thresher_Ironside': {'Body': 'Ironside', 'Barrel': 'Finned', 'Muzzle': 'Can', 'Magazine': '120', 'Sight': 'Pip',
                          'Stock': 'Fixed'},
}

if lt.want_preview():
    out = lambda name: lt.preview_path('Weapons', name)
    side, three_q, first_person = (-1.0, -0.02, 0.05), (-1.0, -0.35, 0.3), (0.42, 1.0, 0.32)
    lt.preview(assemble(DEFAULT), out('Thresher'), view=three_q, ground=False, lens=60.0, fit=0.95)
    lt.preview(assemble(DEFAULT), out('Thresher_Side'), view=side, ground=False, lens=80.0, fit=0.9)
    lt.preview(assemble(DEFAULT), out('Thresher_FP'), view=first_person, ground=False, lens=45.0, fit=0.8)
    for name, choice in VARIANTS.items():
        lt.preview(assemble(choice), out(name), view=three_q, ground=False, lens=60.0, fit=0.95)
    lt.preview(assemble(VARIANTS['Thresher_Legendary']), out('Thresher_Legendary_FP'), view=first_person, ground=False,
               lens=45.0, fit=0.8)
    lt.preview(lay_out(SLOTS), out('Thresher_Parts'), resolution=(2200, 1600), view=(-1.0, 0.0, 0.0), ground=False,
               lens=80.0, fit=0.75)
    assemble(DEFAULT)

if lg.want_elements():   # -- --elements: Thresher_Elements.png, the plain 100 and its element boxes
    lg.element_preview('Thresher_Elements', assemble, DEFAULT, 'Magazine', ['100', 'Ember100', 'Saint100', 'Salt100'],
                       built)
