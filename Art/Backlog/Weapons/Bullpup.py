"""The bullpup assault rifle the user chose (2026-09-30) as the base of every non-legendary AR, with interchangeable
parts: 8 bodies, 8 barrels, 8 muzzle devices, 8 magazines, 8 sights and 8 stocks. Kept for later in Art/Backlog (see its
README): nothing in the game uses it yet. Legendary ARs will get their own unique models and names later.

How the parts fit (gun space, cm: u along the gun from the back of the butt, v up from the bore; see looter_guns):

  BullpupBody_<Key>      the shell: receiver, thumbhole grip loop, trigger, charging handle, a Picatinny rail on the
                         optic mount, a rarity accent line on each side (GunAccentGlow); the styles differ in shape,
                         materials and fittings (plates, armor, windows). Sockets: Stock (the butt face), Barrel (the
                         barrel nut on the nose), Magazine (the well behind the grip), Sight (the rail's middle), Grip,
                         Foregrip, in the same place on every body, so every part fits every body.
  BullpupBarrel_<Key>    origin at the barrel nut; SOCKET_Muzzle at its tip (barrels differ in length).
  BullpupMuzzle_<Key>    origin at its back face (screws onto a barrel's SOCKET_Muzzle); SOCKET_Muzzle at its own
                         tip, where the flash comes from.
  BullpupMagazine_<Key>  origin where it seats.
  BullpupSight_<Key>     origin on top of the rail, at its middle.
  BullpupStock_<Key>     origin at the middle of the butt face; extends backward (and, for cheek risers, forward
                         over the shell).

A rolled AR picks one option per slot from PARTS below: each carries a display name, the word it can lend the gun's
name, the lowest rarity it appears on, and its stat changes in percent. Damage, accuracy (the game's Spread, inverted),
fire rate, magazine size and reload map onto FWeaponPartStats today; range, recoil, handling (aim and swap speed) and
zoom would be new stats. Nothing reads PARTS yet: it's the design, kept next to the models it belongs to.

    blender -b --factory-startup --python Art/Backlog/Weapons/Bullpup.py -- --preview
"""
import math

from mathutils import Vector

import looter_guns as lg
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xe8e8e8, 2.0)   # glows in the gun's rarity color in the game
RETICLE = lg.glow('GunReticle', 0xff3b2e, 6.0)
SHELL = lg.POLY_SAND

# Where the parts attach (gun space, cm).
STOCK_AT = (0.0, -3.3)
BARREL_AT = (51.9, 0.0)
MAG_AT = (14.0, -5.6)
SIGHT_AT = (23.0, 6.15)
MUZZLE_AT = (100.0, 0.0)   # muzzle devices are modeled here, around their own origin

g = lg.Gun('Bullpup')
origins = {}


def model(slot, key, at):
    name = f'{slot}_{key}'
    origins[name] = at
    return name


# --- Bodies: the shell and its fittings, all with the same sockets ---

SHELL_OUTLINE = [(1.2, -9.6), (0, -8.6), (0, 2.0), (2, 3.4), (40, 3.2), (47, 2.4), (51, 0.6), (51.5, -1.8), (49, -3.2),
                 (38.5, -3.5), (37, -4.6), (19, -5.2), (18.6, -5.8), (9.5, -5.8), (3, -8.6)]
LOOP_OUTLINE = [(36.8, -3.8), (37.6, -12.0), (36.4, -14.0), (34, -14.8), (25, -15.2), (22.0, -14.8), (20.6, -12.5),
                (19.6, -7.0), (19.4, -3.8)]
LOOP_HOLE = [(25.0, -5.8), (34.4, -5.8), (35.4, -7.0), (35.6, -11.8), (34.2, -13.0), (27.0, -13.0), (25.4, -11.8),
             (24.2, -7.2)]
WALNUT_DARK = lg.material('GunWalnutDark', 'GunWood', 0xa48670)
OLIVE = lg.material('GunOlivePaint', 'PaintWorn', 0x6f7550)


def shell(m, mat, outline=SHELL_OUTLINE, width=5.0, round=1.0, bevel=1.1, vents=True, extra_cuts=()):
    obj = lg.slab(outline, width, bevel=bevel, segments=3, mat=mat, round=round)
    cuts = [lg.cbox(2.6, 7.4, -1.0, 1.0, -3.0, -2.0)] + list(extra_cuts)
    if vents:
        for w0, w1 in ((-3.0, -2.1), (2.1, 3.0)):
            cuts += [lg.cslab([(u, -1.6), (u + 0.9, -1.6), (u + 2.2, 1.2), (u + 1.3, 1.2)], w0, w1, round=0.3)
                     for u in (40.6, 42.8, 45.0)]
    g.add(m, lg.cut(obj, cuts))


def grip_loop(m, mat, outline=LOOP_OUTLINE, round=1.0, bevel=0.9):
    loop = lg.slab(outline, 3.6, bevel=bevel, segments=3, mat=mat, round=round)
    g.add(m, lg.cut(loop, [lg.cslab(LOOP_HOLE, -3.0, 3.0, round=1.0)]))


def side_plates(m, mat, rivet_mat, outline=((8, -4.8), (18.2, -4.8), (19.4, 1.6), (9.4, 1.6)), round=0.8, w=2.55):
    for side in (-1.0, 1.0):
        g.add(m, lg.slab(list(outline), 0.3, w=side * w, bevel=0.1, mat=mat, round=round))
        for u, v in ((9.6, -4.0), (17.6, -4.0), (10.6, 0.8), (18.2, 0.8)):
            g.add(m, lg.rivet(u, v, side * (w + 0.17), side, r=0.28, mat=rivet_mat))


def fittings(m, bolt=lg.BLACK, mount=SHELL, rail=lg.BLACK, accent=(21.0, 37.0, -2.75), accent_w=2.53):
    """What every body has: the bolt in the port, the accent line, the stock plate, trigger, charging handle, the optic
    mount and rail, the barrel nut and sling loop, and the sockets."""
    g.add(m, lg.box(2.8, 7.2, -0.9, 0.9, 4.2, bevel=0.1, mat=bolt))
    u0, u1, v = accent
    for side in (-1.0, 1.0):
        g.add(m, lg.box(u0, u1, v, v + 0.25, 0.08, w=side * accent_w, bevel=0.0, mat=ACCENT))
    g.add(m, lg.box(-0.3, 0.3, -8.4, 1.8, 4.4, bevel=0.15, mat=lg.BLACK))
    g.add(m, lg.slab([(26.4, -5.8), (27.4, -5.8), (27.2, -7.0), (26.8, -8.0), (26.2, -8.2), (26.5, -7.0)], 0.6,
                     bevel=0.12, mat=lg.BLACK))
    g.add(m, lg.slab([(38.8, 2.9), (44.0, 2.6), (44.0, 3.6), (39.6, 3.9)], 1.0, w=1.9, bevel=0.2, mat=lg.BLACK))
    if mount is not None:
        g.add(m, lg.slab([(12, 3.0), (34, 3.0), (34, 4.4), (32.5, 5.2), (13.5, 5.2), (12, 4.4)], 3.6, bevel=0.6,
                         segments=3, mat=mount, round=0.5))
    g.add(m, lg.rail(14, 32, 5.2, mat=rail))
    g.add(m, lg.tube(50.6, BARREL_AT[0], 1.25, segments=20, mat=lg.BLACK))
    g.add(m, lg.pipe([(43.0, -3.0), (43.6, -4.6), (45.6, -4.6), (46.2, -3.0)], 0.25, mat=lg.BLACK))
    for name, uv in (('Stock', STOCK_AT), ('Barrel', BARREL_AT), ('Magazine', MAG_AT), ('Sight', SIGHT_AT),
                     ('Grip', (22.5, -10.0)), ('Foregrip', (44.0, -4.5))):
        g.socket(m, name, uv)


def bodies():
    B = lambda key: model('Body', key, (0.0, 0.0))

    m = B('Standard')                                          # the white shell the user picked
    shell(m, SHELL)
    grip_loop(m, SHELL)
    side_plates(m, lg.POLY_GREY, lg.BLACK)
    fittings(m)

    m = B('Heritage')                                          # oiled walnut, blued plates, brass rivets
    shell(m, WALNUT_DARK)
    grip_loop(m, WALNUT_DARK)
    side_plates(m, lg.BLUED, lg.BRASS)
    fittings(m, mount=WALNUT_DARK, rail=lg.BLUED)

    m = B('Carbon')                                            # black and angular, tan plates
    angular = [(0.8, -9.6), (0, -8.8), (0, 2.4), (2.6, 3.4), (39, 3.4), (49.4, 1.0), (51.6, -0.6), (51.6, -2.2),
               (49.4, -3.4), (38.5, -3.6), (37, -4.6), (19, -5.2), (18.6, -5.8), (9.5, -5.8), (3, -8.8)]
    shell(m, lg.POLY_BLACK, outline=angular, round=0.3, bevel=0.5)
    grip_loop(m, lg.POLY_BLACK, outline=[(36.8, -3.8), (37.8, -12.6), (35.6, -14.8), (22.6, -15.2), (20.4, -13.0),
                                         (19.4, -3.8)], round=0.4, bevel=0.5)
    side_plates(m, lg.POLY_TAN, lg.BLACK, outline=((7.4, -5.0), (19.0, -5.0), (20.6, 1.8), (10.4, 1.8)), round=0.2)
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(38, 1.2), (48.4, 0.6), (49.6, -0.6), (38, -0.2)], 0.3, w=side * 2.55, bevel=0.05,
                         mat=lg.POLY_TAN))
    fittings(m, mount=lg.POLY_BLACK)

    m = B('Salvager')                                          # welded rusty panels, a pipe trigger guard
    shell(m, lg.SALVAGE, round=0.3, bevel=0.2, vents=False)
    for side in (-1.0, 1.0):
        w = side * 2.55
        for u0, u1 in ((1.0, 18.0), (21.0, 46.0)):
            g.add(m, lg.slab([(u0, -3.6), (u1, -3.6), (u1, 2.4), (u0, 2.4)], 0.3, w=w, bevel=0.05, mat=lg.SALVAGE))
            for u in range(int(u0) + 1, int(u1), 4):
                for v in (-2.9, 1.7):
                    g.add(m, lg.rivet(u, v, w + side * 0.17, side, r=0.3, mat=lg.STEEL))
        g.add(m, lg.pipe([(0.6, 3.0, w * 0.98), (25, 3.1, w * 0.98), (48, 2.4, w * 0.98)], 0.2, mat=lg.STEEL, sides=6))
    g.add(m, lg.slab([(19.6, -5.2), (24.4, -5.2), (23.0, -15.4), (21.6, -16.2), (18.4, -15.8), (17.8, -14.6)], 3.4,
                     bevel=0.8, segments=3, mat=lg.TAPE, round=0.6))           # taped grip
    g.add(m, lg.pipe([(24.6, -5.4), (26.0, -13.2), (34.6, -13.2), (36.4, -4.6)], 0.45, mat=lg.STEEL))   # guard
    fittings(m, bolt=lg.STEEL, mount=lg.SALVAGE)

    m = B('Armored')                                           # olive paint, bolted-on plates, a nose guard
    shell(m, OLIVE)
    grip_loop(m, OLIVE)
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(5, -5.4), (20.0, -5.0), (21.0, 2.6), (6.0, 2.6)], 0.7, w=side * 2.75, bevel=0.15, mat=OLIVE,
                         round=0.5))
        g.add(m, lg.slab([(38, -3.0), (49.8, -2.2), (50.6, 0.0), (38, 2.4)], 0.7, w=side * 2.75, bevel=0.15, mat=OLIVE,
                         round=0.5))
        for u, v in ((6.6, -4.4), (19.2, -4.0), (7.0, 1.8), (19.8, 1.8), (39.4, -2.2), (39.4, 1.6), (48.6, -1.4)):
            g.add(m, lg.rivet(u, v, side * 3.12, side, r=0.34, mat=lg.STEEL))
    fittings(m, mount=OLIVE, accent=(22.0, 36.5, -2.75))

    m = B('Skeleton')                                          # windows onto the action
    windows = [lg.cslab([(9.0, -4.0), (19.0, -4.0), (19.0, 1.4), (9.0, 1.4)], -3.0, 3.0, round=1.0),
               lg.cslab([(39.0, -2.4), (47.4, -2.0), (47.4, 1.0), (39.0, 1.6)], -3.0, 3.0, round=0.8)]
    shell(m, SHELL, vents=False, extra_cuts=windows)
    g.add(m, lg.box(8.6, 19.4, -3.6, 1.0, 1.8, bevel=0.2, mat=lg.BRASS))      # the bolt carrier
    g.add(m, lg.tube(38.6, 48.0, 0.9, v=-0.4, mat=lg.STEEL))                   # recoil spring guide
    for u in range(39, 48):
        g.add(m, lg.tube(u, u + 0.35, 1.05, v=-0.4, segments=12, mat=lg.BLACK))
    grip_loop(m, SHELL)
    fittings(m)

    m = B('Sleek')                                             # a longer, smoother two-tone shell, no plates
    sleek = [(1.6, -9.4), (0, -8.0), (0, 1.6), (3.0, 3.4), (36, 3.6), (45, 2.8), (50.4, 1.0), (51.8, -1.2), (50.0, -3.0),
             (38.5, -3.6), (37, -4.6), (19, -5.2), (18.6, -5.8), (9.5, -5.8), (3.6, -8.4)]
    shell(m, SHELL, outline=sleek, round=2.0, bevel=1.4, vents=False)
    g.add(m, lg.slab([(1.4, -9.2), (3.6, -8.4), (9.5, -5.8), (37, -4.6), (38.6, -3.4), (49.6, -2.8), (50, -1.6),
                      (38, -2.0), (10, -3.6), (1.0, -7.0)], 5.2, bevel=1.0, segments=3, mat=lg.POLY_GREY, round=1.2))
    grip_loop(m, lg.POLY_GREY, round=1.6)
    fittings(m, accent=(6.0, 47.0, -2.35), accent_w=2.6)

    m = B('Marksman')                                          # a built-in cheek rest and a bipod rail under the nose
    shell(m, SHELL)
    grip_loop(m, SHELL)
    side_plates(m, lg.POLY_GREY, lg.BLACK)
    g.add(m, lg.slab([(0.2, 2.0), (11.0, 3.0), (11.6, 4.8), (0.6, 4.6)], 4.4, bevel=0.6, segments=3, mat=lg.POLY_GREY,
                     round=0.9))
    g.add(m, lg.rail(39.5, 49.0, -3.3, width=2.0, mat=lg.BLACK, down=True))
    g.add(m, lg.slab([(36.6, -3.6), (39.0, -3.6), (39.0, -6.4), (37.8, -6.8), (36.8, -5.6)], 3.0, bevel=0.5,
                     segments=3, mat=SHELL, round=0.5))                         # a thumb rest ahead of the loop
    fittings(m)


# --- Barrels: from the barrel nut forward ---

def barrels():
    u0 = BARREL_AT[0]

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    m = model('Barrel', 'Stub', BARREL_AT)                      # a short stub with a gas collar
    g.add(m, lg.tube(u0, u0 + 6.0, 0.82, mat=lg.BLACK), lg.tube(u0 + 2.0, u0 + 3.4, 1.15, segments=20, mat=lg.BLACK))
    tip(m, 6.0)

    m = model('Barrel', 'Carbine', BARREL_AT)                   # the standard barrel and its gas block
    g.add(m, lg.tube(u0, u0 + 12.0, 0.8, mat=lg.BLACK))
    g.add(m, lg.slab([(u0 + 3.0, -1.0), (u0 + 5.6, -1.0), (u0 + 5.6, 1.5), (u0 + 3.4, 1.5)], 2.0, bevel=0.25,
                     mat=lg.BLACK, round=0.3))
    tip(m, 12.0)

    m = model('Barrel', 'Fluted', BARREL_AT)                    # longer, with six flutes
    fluted = lg.tube(u0, u0 + 16.0, 0.9, segments=24, mat=lg.STEEL)
    g.add(m, lg.cut(fluted, [lg.cbox(u0 + 3.0, u0 + 14.2, 0.62, 1.3, -0.17, 0.17, rotate_u=a) for a in range(0, 360, 60)]))
    g.add(m, lg.tube(u0, u0 + 2.2, 1.2, segments=20, mat=lg.BLACK))
    tip(m, 16.0)

    m = model('Barrel', 'Marksman', BARREL_AT)                  # long and thin, with a folding front post
    g.add(m, lg.tube(u0, u0 + 24.0, 0.75, segments=20, mat=lg.BLACK))
    g.add(m, lg.slab([(u0 + 2.0, -1.0), (u0 + 4.6, -1.0), (u0 + 4.6, 1.4), (u0 + 2.4, 1.4)], 2.0, bevel=0.25,
                     mat=lg.BLACK, round=0.3))
    g.add(m, lg.box(u0 + 19.6, u0 + 22.0, -0.6, 1.1, 1.8, bevel=0.2, mat=lg.BLACK))
    g.add(m, lg.slab([(u0 + 20.2, 1.1), (u0 + 21.6, 1.1), (u0 + 21.3, 4.2), (u0 + 20.6, 4.2)], 0.5, bevel=0.1,
                     mat=lg.BLACK))
    tip(m, 24.0)

    m = model('Barrel', 'Heavy', BARREL_AT)                     # a thick bull barrel
    g.add(m, lg.turned([(0, 1.25), (16.6, 1.25), (17.0, 1.05)], u0, segments=24, mat=lg.BLACK))
    tip(m, 17.0)

    m = model('Barrel', 'Shrouded', BARREL_AT)                  # in a ported cooling shroud
    g.add(m, lg.tube(u0, u0 + 16.5, 0.8, mat=lg.STEEL))
    shroud = lg.tube(u0 + 0.4, u0 + 14.0, 1.9, segments=24, mat=lg.BLACK)
    holes = []
    for u in (2.4, 5.0, 7.6, 10.2):
        holes += [lg.cpin(u0 + u, 0.0, -2.5, 2.5, 0.55), lg.upright(u0 + u + 1.3, -2.5, 2.5, 0.55)]
    g.add(m, lg.cut(shroud, holes))
    g.add(m, lg.tube(u0 + 13.6, u0 + 14.6, 2.05, segments=24, mat=lg.STEEL))
    tip(m, 16.5)

    m = model('Barrel', 'Railed', BARREL_AT)                    # a sand handguard extension with a rail beneath
    guard = lg.slab([(u0 - 0.6, 0.6), (u0 + 11.4, 0.6), (u0 + 12.6, -0.2), (u0 + 12.6, -2.4), (u0 + 11.6, -3.2),
                     (u0 - 0.6, -3.2)], 4.2, bevel=0.8, segments=3, mat=SHELL, round=0.8)
    slots = [lg.cslab([(u0 + u, -2.2), (u0 + u + 2.4, -2.2), (u0 + u + 2.4, -0.8), (u0 + u, -0.8)], w0, w1, round=0.4)
             for u in (1.5, 5.0, 8.5) for w0, w1 in ((-3, -1.7), (1.7, 3))]
    g.add(m, lg.cut(guard, slots))
    g.add(m, lg.rail(u0 + 0.4, u0 + 11.0, -3.2, width=2.0, mat=lg.BLACK, down=True))
    g.add(m, lg.tube(u0, u0 + 16.0, 0.8, mat=lg.BLACK))
    tip(m, 16.0)

    m = model('Barrel', 'Piston', BARREL_AT)                    # a gas piston tube over the barrel
    g.add(m, lg.tube(u0, u0 + 15.0, 0.78, mat=lg.BLACK))
    g.add(m, lg.tube(u0, u0 + 10.6, 0.48, v=1.75, mat=lg.STEEL))
    g.add(m, lg.slab([(u0 + 9.8, -1.0), (u0 + 12.2, -1.0), (u0 + 12.2, 2.5), (u0 + 10.6, 2.6)], 2.0, bevel=0.25,
                     mat=lg.BLACK, round=0.3))
    g.add(m, lg.tube(u0, u0 + 1.4, 0.65, v=1.75, segments=12, mat=lg.BLACK))
    tip(m, 15.0)


# --- Muzzle devices: around their own origin, screwed onto the barrel's tip ---

def muzzles():
    u0 = MUZZLE_AT[0]

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    m = model('Muzzle', 'Cap', MUZZLE_AT)                       # a thread protector
    g.add(m, lg.turned([(0, 0.92), (1.5, 0.92), (1.8, 0.7)], u0, segments=20, mat=lg.STEEL))
    tip(m, 1.8)

    m = model('Muzzle', 'Birdcage', MUZZLE_AT)                  # slotted flash hider, closed underneath
    cage = lg.turned([(0, 0.98), (5.4, 0.98), (5.8, 0.82)], u0, segments=20, mat=lg.BLACK)
    g.add(m, lg.cut(cage, [lg.cbox(u0 + 1.6, u0 + 5.0, 0.55, 1.3, -0.16, 0.16, rotate_u=a) for a in (-72, -36, 0, 36, 72)]))
    tip(m, 5.8)

    m = model('Muzzle', 'Prong', MUZZLE_AT)                     # three-prong flash hider
    g.add(m, lg.turned([(0, 0.98), (1.8, 0.98)], u0, segments=20, mat=lg.BLACK))
    for a in (0, 120, 240):
        g.add(m, lg.mapped(lg.cbox(u0 + 1.6, u0 + 6.2, 0.42, 1.0, -0.3, 0.3, rotate_u=a), lg.BLACK))
    tip(m, 6.2)

    m = model('Muzzle', 'Brake', MUZZLE_AT)                     # two side ports
    brake = lg.turned([(0, 1.15), (4.8, 1.15), (5.2, 0.95)], u0, segments=20, mat=lg.BLACK)
    g.add(m, lg.cut(brake, [lg.cbox(u0 + 1.0, u0 + 2.1, -0.5, 0.5, -2, 2), lg.cbox(u0 + 2.8, u0 + 3.9, -0.5, 0.5, -2, 2)]))
    tip(m, 5.2)

    m = model('Muzzle', 'Compensator', MUZZLE_AT)               # ports on top push the muzzle down
    comp = lg.turned([(0, 1.1), (4.6, 1.1), (5.4, 0.8)], u0, segments=20, mat=lg.STEEL)
    g.add(m, lg.cut(comp, [lg.cbox(u0 + u, u0 + u + 0.7, 0.2, 1.6, -0.36, 0.36) for u in (1.0, 2.2, 3.4)]))
    tip(m, 5.4)

    m = model('Muzzle', 'BoxBrake', MUZZLE_AT)                  # a big boxy brake with three ports
    box = lg.box(u0, u0 + 7.4, -1.6, 1.6, 3.6, bevel=0.35, mat=lg.BLACK)
    g.add(m, lg.cut(box, [lg.cbox(u0 + u, u0 + u + 1.0, -1.2, 1.2, -3, 3) for u in (1.3, 3.2, 5.1)]))
    tip(m, 7.4)

    m = model('Muzzle', 'Suppressor', MUZZLE_AT)                # a short can
    g.add(m, lg.turned([(0, 0.95), (0.7, 1.6), (9.4, 1.6), (10.0, 1.2)], u0, segments=24, mat=lg.BLACK))
    g.add(m, lg.tube(u0 + 9.0, u0 + 9.8, 1.66, segments=24, mat=lg.STEEL))
    tip(m, 10.0)

    m = model('Muzzle', 'LongSuppressor', MUZZLE_AT)            # a long can in a heat wrap
    g.add(m, lg.turned([(0, 0.95), (1.0, 1.85), (17.0, 1.85), (17.8, 1.3)], u0, segments=24, mat=lg.BLACK))
    g.add(m, lg.tube(u0 + 3.0, u0 + 14.0, 1.97, segments=24, mat=lg.TAPE))
    for u in (3.0, 13.6):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.5, 2.05, segments=24, mat=lg.STEEL))
    tip(m, 17.8)


# --- Magazines: around where they seat ---

def mag(m, front, rear, width, mat, ribs=0, rib_top=-17.0, steps=10):
    outline = lg.curved_outline(front, rear, steps)
    g.add(m, lg.slab(outline, width, bevel=0.3, mat=mat))
    fr, rr = outline[:steps + 1], list(reversed(outline[steps + 1:]))
    a, b = fr[-1], rr[-1]
    g.add(m, lg.slab([b, a, (a[0] + 0.25, a[1] - 0.8), (b[0] + 0.25, b[1] - 0.8)], width + 0.4, bevel=0.2, mat=mat))
    for k in range(ribs):
        v = rib_top - k * 1.3
        # The rib spans the magazine's width at that height (edges found on the outline's curves).
        f = min(fr, key=lambda p: abs(p[1] - v))
        r = min(rr, key=lambda p: abs(p[1] - v))
        g.add(m, lg.box(r[0] - 0.2, f[0] + 0.2, v - 0.35, v + 0.35, width + 0.3, bevel=0.15, mat=mat))


def magazines():
    top = MAG_AT[1]
    F, R = 17.6, 10.4

    m = model('Magazine', '20', MAG_AT)
    mag(m, [(F, top), (17.9, -10), (18.3, -16.0)], [(R, top), (10.6, -10), (10.9, -15.6)], 2.4, lg.POLY_BLACK, 2, -12.5)

    m = model('Magazine', '30', MAG_AT)
    mag(m, [(F, top), (18.3, -15.0), (19.8, -24.0)], [(R, top), (10.9, -15.0), (12.0, -23.4)], 2.4, lg.POLY_BLACK, 4)

    m = model('Magazine', '30Window', MAG_AT)                   # smoke polymer with a window onto the rounds
    mag(m, [(F, top), (18.3, -15.0), (19.8, -24.0)], [(R, top), (10.9, -15.0), (12.0, -23.4)], 2.4, lg.POLY_GREY, 0)
    g.parts[m][0] = lg.cut(g.parts[m][0], [lg.cbox(13.0, 15.4, -21.0, -8.0, -2.0, -0.7)])
    g.add(m, lg.box(13.0, 15.4, -21.0, -8.0, 1.0, w=-0.3, bevel=0.3, mat=lg.BRASS))

    m = model('Magazine', '40', MAG_AT)                         # a long curved steel magazine
    mag(m, [(F, top), (18.9, -17.0), (22.2, -30.0)], [(R, top), (11.3, -17.0), (14.2, -29.4)], 2.5, lg.BLUED, 0)
    outline = lg.curved_outline([(F, top), (18.9, -17.0), (22.2, -30.0)], [(R, top), (11.3, -17.0), (14.2, -29.4)])
    mid = [((a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5) for a, b in zip(outline[:11], reversed(outline[11:]))][2:-1]
    for side in (-1.0, 1.0):
        g.add(m, lg.strip(mid, 0.3, 1.0, w=side * 1.3, mat=lg.BLUED))

    m = model('Magazine', 'Coupled', MAG_AT)                    # two 30s clamped side by side
    for w, dv in ((0.0, 0.0), (2.7, -1.0)):
        o = lg.slab(lg.curved_outline([(F, top + dv), (18.3, -15.0 + dv), (19.8, -24.0 + dv)],
                                      [(R, top + dv), (10.9, -15.0 + dv), (12.0, -23.4 + dv)]), 2.4, w=w, bevel=0.3,
                    mat=lg.POLY_BLACK)
        g.add(m, o)
    for v in (-10.5, -18.5):
        g.add(m, lg.box(10.2, 19.4, v - 0.8, v + 0.8, 5.6, w=1.35, bevel=0.3, mat=lg.STEEL))

    m = model('Magazine', '60', MAG_AT)                         # a wide quad-stack casket
    g.add(m, lg.box(R, F, -9.0, top, 2.4, bevel=0.2, mat=lg.POLY_BLACK))
    mag(m, [(F + 0.1, -8.6), (18.6, -18.0), (19.8, -30.0)], [(R - 0.1, -8.6), (10.9, -18.0), (11.8, -29.6)], 3.8,
        lg.POLY_BLACK, 3, -24.0)

    m = model('Magazine', 'Drum', MAG_AT)                       # a 75-round drum
    g.add(m, lg.slab([(R, top), (F, top), (F + 0.4, -10.0), (R - 0.4, -10.0)], 2.4, bevel=0.2, mat=lg.POLY_BLACK))
    g.add(m, lg.pin(14.0, -16.4, -2.4, 2.4, 6.8, segments=32, mat=lg.POLY_BLACK,
                    profile=[(0, 6.0), (0.5, 6.8), (4.3, 6.8), (4.8, 6.0)]))
    for side in (-1.0, 1.0):
        w0 = 2.4 if side > 0 else -2.9
        g.add(m, lg.pin(14.0, -16.4, w0, w0 + 0.5, 2.6, segments=20, mat=lg.STEEL))

    m = model('Magazine', 'TwinDrum', MAG_AT)                   # a 100-round twin drum
    g.add(m, lg.slab([(R, top), (F, top), (F + 0.3, -20.0), (R - 0.3, -20.0)], 2.4, bevel=0.25, mat=lg.POLY_BLACK))
    for side in (-1.0, 1.0):
        w0 = 1.1 if side > 0 else -4.3
        g.add(m, lg.pin(14.0, -14.5, w0, w0 + 3.2, 6.2, segments=32, mat=lg.POLY_BLACK,
                        profile=[(0, 5.6), (0.4, 6.2), (2.8, 6.2), (3.2, 5.6)]))
        cap = 4.3 if side > 0 else -4.7
        g.add(m, lg.pin(14.0, -14.5, cap, cap + 0.4, 2.2, segments=20, mat=lg.STEEL))


# --- Sights: around the middle of the rail ---

def sights():
    v0 = SIGHT_AT[1]
    S = lambda key: model('Sight', key, SIGHT_AT)

    m = S('Iron')                                                # flip-up rear aperture and front post
    g.add(m, lg.slab([(15, v0), (18, v0), (17.6, v0 + 2.2), (16.0, v0 + 2.2)], 2.0, bevel=0.15, mat=lg.BLACK, round=0.3))
    g.add(m, lg.tube(16.5, 17.1, 0.55, v=v0 + 1.7, segments=12, mat=lg.BLACK))
    g.add(m, lg.slab([(29.6, v0), (32, v0), (31.6, v0 + 2.6), (30.2, v0 + 2.6)], 1.8, bevel=0.15, mat=lg.BLACK, round=0.3))

    m = S('RedDot')                                              # a 1x tube
    g.add(m, lg.box(19.4, 26.6, v0, v0 + 1.3, 2.2, bevel=0.25, mat=lg.BLACK))
    g.add(m, lg.turned([(0, 1.45), (1.0, 1.45), (1.4, 1.25), (6.6, 1.25), (7.0, 1.45), (8.0, 1.45)], 19.0,
                       v=v0 + 2.8, segments=20, mat=lg.BLACK))
    g.add(m, lg.pin(23.0, v0 + 2.8, -2.0, -1.2, 0.55, mat=lg.BLACK))
    g.add(m, lg.tube(26.85, 26.95, 1.3, v=v0 + 2.8, segments=20, mat=lg.LENS))

    m = S('Reflex')                                              # an open window
    g.add(m, lg.box(19.5, 26.5, v0, v0 + 1.0, 2.6, bevel=0.25, mat=lg.BLACK))
    frame = lg.slab([(21.0, v0 + 1.0), (26.4, v0 + 1.0), (26.4, v0 + 4.4), (25.0, v0 + 5.2), (22.4, v0 + 5.2),
                     (21.0, v0 + 4.4)], 2.4, bevel=0.2, mat=lg.BLACK, round=0.3)
    g.add(m, lg.cut(frame, [lg.cbox(20.0, 27.4, v0 + 1.6, v0 + 4.6, -0.95, 0.95)]))
    g.add(m, lg.box(25.6, 25.75, v0 + 1.6, v0 + 4.6, 1.9, bevel=0.0, mat=lg.LENS))
    g.add(m, lg.pin(25.5, v0 + 3.1, -0.1, 0.1, 0.1, mat=RETICLE))

    m = S('Holo')                                                # a holographic box with a hood
    g.add(m, lg.box(17.5, 28.5, v0, v0 + 1.3, 3.0, bevel=0.25, mat=lg.BLACK))
    hood = lg.slab([(17.5, v0 + 1.2), (28.5, v0 + 1.2), (28.5, v0 + 5.4), (27.0, v0 + 6.3), (19.0, v0 + 6.3),
                    (17.5, v0 + 5.4)], 3.4, bevel=0.35, segments=3, mat=lg.BLACK, round=0.4)
    g.add(m, lg.cut(hood, [lg.cbox(16.5, 29.5, v0 + 1.9, v0 + 5.5, -1.25, 1.25)]))
    g.add(m, lg.box(27.0, 27.15, v0 + 1.9, v0 + 5.5, 2.5, bevel=0.0, mat=lg.LENS))
    g.add(m, lg.pin(26.9, v0 + 3.7, -0.12, 0.12, 0.12, mat=RETICLE))

    m = S('Prism')                                               # a compact 2x prism sight
    g.add(m, lg.slab([(18, v0), (28, v0), (28, v0 + 4.0), (26.5, v0 + 4.6), (19.5, v0 + 4.6), (18, v0 + 4.0)], 3.0,
                     bevel=0.35, segments=3, mat=lg.BLACK, round=0.5))
    for u in (17.9, 28.0):
        g.add(m, lg.tube(u, u + 0.12, 1.1, v=v0 + 2.6, segments=20, mat=lg.LENS))
    g.add(m, lg.upright(23.0, v0 + 4.5, v0 + 5.4, 0.7, mat=lg.BLACK, segments=14))

    m = S('ACOG')                                                # a 4x combat optic, tapered to its objective
    g.add(m, lg.box(18.0, 26.0, v0, v0 + 1.4, 2.4, bevel=0.25, mat=lg.BLACK))
    g.add(m, lg.turned([(0, 1.35), (2.4, 1.35), (3.0, 1.15), (6.0, 1.15), (7.0, 1.6), (11.0, 1.75)], 17.0,
                       v=v0 + 2.7, segments=20, mat=lg.BLACK))
    g.add(m, lg.box(19.5, 25.0, v0 + 3.9, v0 + 4.6, 1.0, bevel=0.15, mat=lg.BLACK))
    g.add(m, lg.box(20.0, 24.5, v0 + 4.5, v0 + 4.7, 0.5, bevel=0.0, mat=lg.POLY_OLIVE))   # fiber-optic strip
    g.add(m, lg.tube(27.9, 28.0, 1.6, v=v0 + 2.7, segments=20, mat=lg.LENS))

    m = S('Variable')                                            # a 3-9x scope on two rings
    g.add(m, lg.turned([(0, 1.8), (4.0, 1.8), (5.5, 1.25), (13.5, 1.25), (15.5, 2.0), (19.5, 2.0)], 13.5,
                       v=v0 + 3.9, segments=24, mat=lg.BLACK))
    for u in (18.5, 27.0):
        g.add(m, lg.tube(u - 0.6, u + 0.6, 1.45, v=v0 + 3.9, segments=20, mat=lg.BLACK))
        g.add(m, lg.box(u - 0.9, u + 0.9, v0, v0 + 2.8, 2.0, bevel=0.2, mat=lg.BLACK))
    g.add(m, lg.upright(23.0, v0 + 5.0, v0 + 6.4, 0.85, mat=lg.BLACK, segments=16))
    g.add(m, lg.pin(23.0, v0 + 3.9, -2.6, -1.1, 0.85, mat=lg.BLACK, segments=16))
    g.add(m, lg.tube(32.9, 33.0, 1.85, v=v0 + 3.9, segments=24, mat=lg.LENS))

    m = S('LongRange')                                           # an 8x scope with a sunshade and an offset dot
    g.add(m, lg.turned([(0, 1.9), (4.5, 1.9), (6.0, 1.35), (14.0, 1.35), (16.5, 2.6), (26.0, 2.6)], 11.0,
                       v=v0 + 4.6, segments=28, mat=lg.BLACK))
    for u in (17.0, 27.5):
        g.add(m, lg.tube(u - 0.7, u + 0.7, 1.55, v=v0 + 4.6, segments=20, mat=lg.BLACK))
        g.add(m, lg.box(u - 1.0, u + 1.0, v0, v0 + 3.4, 2.2, bevel=0.2, mat=lg.BLACK))
    g.add(m, lg.upright(22.0, v0 + 5.8, v0 + 7.6, 1.0, mat=lg.BLACK, segments=16))
    g.add(m, lg.pin(22.0, v0 + 4.6, -3.0, -1.2, 1.0, mat=lg.BLACK, segments=16))
    g.add(m, lg.tube(36.9, 37.0, 2.45, v=v0 + 4.6, segments=28, mat=lg.LENS))
    g.add(m, lg.box(13.0, 15.8, v0 + 6.1, v0 + 7.3, 1.6, bevel=0.2, mat=lg.BLACK))   # the offset red dot
    g.add(m, lg.box(15.2, 15.3, v0 + 6.4, v0 + 7.1, 1.2, bevel=0.0, mat=lg.LENS))


# --- Stocks: on the butt face, backward ---

def stocks():
    S = lambda key: model('Stock', key, STOCK_AT)

    def pad(m, u_back, thick=1.6, flare=0.0, mat=lg.RUBBER):
        g.add(m, lg.slab([(u_back, -8.8), (u_back - thick, -8.8 - flare), (u_back - thick, 2.0 + flare * 0.3),
                          (u_back, 2.0)], 5.2, bevel=0.5, mat=mat, round=0.6))

    m = S('Pad')                                                 # a plain rubber butt pad
    pad(m, 0.3)

    m = S('Recoil')                                              # a thick ribbed recoil pad
    thick = lg.slab([(0.3, -9.0), (-3.0, -9.5), (-3.0, 2.4), (0.3, 2.1)], 5.4, bevel=0.8, mat=lg.RUBBER, round=1.0)
    g.add(m, lg.cut(thick, [lg.cbox(-3.6, -2.5, v, v + 0.45, -3.0, 3.0) for v in (-7.5, -5.0, -2.5, 0.0)]))

    m = S('Spacers')                                             # length-of-pull spacers and a pad
    for k, mat in enumerate((SHELL, lg.POLY_GREY)):
        g.add(m, lg.slab([(0.3 - k * 1.3, -8.6), (-1.0 - k * 1.3, -8.6), (-1.0 - k * 1.3, 1.9), (0.3 - k * 1.3, 1.9)],
                         4.8, bevel=0.3, mat=mat, round=0.5))
    for v in (-6.0, -0.6):
        g.add(m, lg.pin(-1.0, v, -2.6, -2.35, 0.35, mat=lg.STEEL))
    pad(m, -2.3)

    m = S('Cheek')                                               # a pad and a raised cheek rest on posts
    pad(m, 0.3)
    g.add(m, lg.slab([(0.4, 3.6), (11.0, 4.0), (11.4, 5.4), (0.8, 5.4)], 3.8, bevel=0.5, segments=3,
                     mat=lg.POLY_GREY, round=0.8))
    for u in (2.6, 8.8):
        g.add(m, lg.upright(u, 3.0, 4.0, 0.35, mat=lg.STEEL))

    m = S('Skeleton')                                            # an open extension frame
    frame = lg.slab([(0.3, -8.4), (-7.5, -9.4), (-8.6, -8.0), (-8.6, 1.4), (-7.5, 2.4), (0.3, 2.0)], 3.6, bevel=0.6,
                    segments=3, mat=SHELL, round=1.0)
    g.add(m, lg.cut(frame, [lg.cslab([(-1.4, -6.8), (-6.8, -7.4), (-6.8, 0.2), (-1.4, 0.2)], -3, 3, round=1.0)]))
    pad(m, -8.4)

    m = S('Padded')                                              # a cushioned cheek wrap and a thick pad
    g.add(m, lg.slab([(-1.6, 0.6), (9.0, 1.6), (9.6, 4.2), (-1.6, 3.6)], 5.6, bevel=1.0, segments=3, mat=lg.POLY_OLIVE,
                     round=1.2))
    for u in (1.0, 6.5):
        g.add(m, lg.slab([(u, 1.0), (u + 0.9, 1.1), (u + 0.9, 4.2), (u, 4.0)], 5.8, bevel=0.2, mat=lg.POLY_BLACK))
    pad(m, 0.3, thick=2.6, flare=0.3)

    m = S('Monopod')                                             # a pad over a fold-down monopod leg
    pad(m, 0.3)
    g.add(m, lg.box(-1.4, 2.6, -10.4, -8.4, 2.4, bevel=0.3, mat=lg.BLACK))
    g.add(m, lg.upright(0.6, -17.6, -10.2, 0.45, mat=lg.STEEL))
    g.add(m, lg.upright(0.6, -14.0, -10.4, 0.6, mat=lg.BLACK))
    g.add(m, lg.upright(0.6, -18.4, -17.4, 1.0, mat=lg.RUBBER, segments=16))

    m = S('Plate')                                               # a steel butt plate with a sling loop, angled pad
    g.add(m, lg.slab([(0.3, -8.8), (-1.0, -9.2), (-1.0, 2.3), (0.3, 2.0)], 5.0, bevel=0.3, mat=lg.STEEL, round=0.4))
    g.add(m, lg.slab([(-1.0, -9.2), (-2.8, -10.0), (-2.4, 2.6), (-1.0, 2.3)], 5.2, bevel=0.5, mat=lg.RUBBER, round=0.6))
    g.add(m, lg.pipe([(-0.4, -9.2), (-0.6, -10.9), (-2.4, -11.2), (-2.6, -9.6)], 0.28, mat=lg.STEEL))


# --- The design: names, rarities and stats (percent changes; magazines give their rounds) ---

R_COMMON, R_UNCOMMON, R_RARE, R_EPIC = 'Common', 'Uncommon', 'Rare', 'Epic'


def p(key, name, word, rarity, **stats):
    return dict(key=key, name=name, word=word, rarity=rarity, stats=stats)


PARTS = {
    'Body': [
        p('Standard', 'Standard shell', '', R_COMMON),
        p('Heritage', 'Heritage shell', 'Heirloom', R_UNCOMMON, accuracy=4, damage=3),
        p('Carbon', 'Carbon shell', 'Carbon', R_UNCOMMON, handling=8, recoil=4),
        p('Salvager', 'Salvager shell', 'Scrapped', R_UNCOMMON, damage=6, accuracy=-4),
        p('Skeleton', 'Skeleton shell', 'Light', R_RARE, handling=12, recoil=6),
        p('Sleek', 'Sleek shell', 'Sleek', R_RARE, fire_rate=6, handling=6),
        p('Marksman', 'Marksman shell', 'Marksman', R_RARE, accuracy=10, recoil=-6, handling=-6),
        p('Armored', 'Armored shell', 'Armored', R_EPIC, recoil=-15, damage=5, handling=-10),
    ],
    'Barrel': [
        p('Stub', 'Stub barrel', 'Compact', R_COMMON, damage=-8, accuracy=-10, range=-25, fire_rate=8, handling=15),
        p('Carbine', 'Carbine barrel', '', R_COMMON),
        p('Fluted', 'Fluted barrel', 'Fluted', R_UNCOMMON, accuracy=8, range=10, handling=5),
        p('Heavy', 'Heavy barrel', 'Heavy', R_UNCOMMON, damage=8, accuracy=12, recoil=-10, handling=-15),
        p('Shrouded', 'Cooled shroud', 'Cooled', R_UNCOMMON, fire_rate=10, accuracy=4, handling=-5),
        p('Railed', 'Railed handguard', 'Tactical', R_RARE, accuracy=6, range=5, recoil=-8, handling=8),
        p('Piston', 'Gas piston', 'Piston', R_RARE, fire_rate=15, recoil=-5, damage=-4),
        p('Marksman', 'Marksman barrel', 'Marksman', R_EPIC, damage=12, accuracy=20, range=40, fire_rate=-10,
          handling=-20),
    ],
    'Muzzle': [
        p('Cap', 'Thread cap', '', R_COMMON),
        p('Birdcage', 'Birdcage flash hider', '', R_COMMON, accuracy=4, recoil=-3),
        p('Prong', 'Three-prong flash hider', 'Flashless', R_UNCOMMON, accuracy=6),
        p('Brake', 'Two-port brake', 'Braced', R_UNCOMMON, recoil=-15, accuracy=3),
        p('Compensator', 'Compensator', 'Steady', R_RARE, recoil=-12, accuracy=8),
        p('BoxBrake', 'Box brake', 'Thumping', R_RARE, recoil=-25, handling=-8),
        p('Suppressor', 'Suppressor', 'Silent', R_RARE, accuracy=6, range=5, damage=-3),
        p('LongSuppressor', 'Long suppressor', 'Whisper', R_EPIC, accuracy=12, range=15, damage=-5, handling=-10),
    ],
    'Magazine': [
        p('20', '20-round magazine', 'Light', R_COMMON, magazine=20, reload=-15, handling=5),
        p('30', '30-round magazine', '', R_COMMON, magazine=30),
        p('30Window', '30-round windowed', 'Ready', R_UNCOMMON, magazine=30, reload=-10),
        p('40', '40-round steel', 'Extended', R_UNCOMMON, magazine=40, reload=8),
        p('Coupled', 'Coupled 30s', 'Jungle', R_RARE, magazine=30, reload=-35),
        p('60', '60-round casket', 'Hungry', R_RARE, magazine=60, reload=20, handling=-10),
        p('Drum', '75-round drum', 'Drummer', R_EPIC, magazine=75, reload=35, handling=-15),
        p('TwinDrum', '100-round twin drum', 'Endless', R_EPIC, magazine=100, reload=50, handling=-25, accuracy=-5),
    ],
    'Sight': [
        p('Iron', 'Flip-up irons', '', R_COMMON, zoom=1.0, handling=10),
        p('RedDot', 'Red dot', 'Dotted', R_COMMON, zoom=1.25, accuracy=5),
        p('Reflex', 'Reflex sight', 'Reflex', R_UNCOMMON, zoom=1.25, accuracy=6, handling=5),
        p('Holo', 'Holographic sight', 'Holo', R_UNCOMMON, zoom=1.5, accuracy=10),
        p('Prism', '2x prism', 'Prism', R_RARE, zoom=2.0, accuracy=12, handling=-5),
        p('ACOG', '4x combat optic', 'Scoped', R_RARE, zoom=4.0, accuracy=18, range=10, handling=-10),
        p('Variable', '3-9x scope', 'Hunter', R_EPIC, zoom=6.0, accuracy=22, range=20, handling=-15),
        p('LongRange', '8x long-range scope', 'Longshot', R_EPIC, zoom=8.0, accuracy=28, range=30, handling=-25,
          fire_rate=-5),
    ],
    'Stock': [
        p('Pad', 'Butt pad', '', R_COMMON),
        p('Recoil', 'Recoil pad', 'Cushioned', R_COMMON, recoil=-10),
        p('Spacers', 'Spacer stock', 'Measured', R_UNCOMMON, accuracy=6, handling=-5),
        p('Cheek', 'Cheek riser', 'Steady', R_UNCOMMON, accuracy=8, recoil=-4),
        p('Skeleton', 'Skeleton stock', 'Nimble', R_UNCOMMON, handling=12, recoil=5),
        p('Padded', 'Padded stock', 'Padded', R_RARE, recoil=-15, handling=-5),
        p('Monopod', 'Monopod stock', 'Anchored', R_RARE, accuracy=12, handling=-12),
        p('Plate', 'Butt plate', 'Slung', R_RARE, handling=8, recoil=-6),
    ],
}
DEFAULT = {'Body': 'Standard', 'Barrel': 'Carbine', 'Muzzle': 'Birdcage', 'Magazine': '30', 'Sight': 'RedDot', 'Stock': 'Pad'}


bodies()
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
    """Puts the body at `at` and the chosen parts (slot -> key) on its sockets; returns the objects."""
    body_obj = built['Body_' + choice.get('Body', 'Standard')]
    body_obj.location = Vector(at)
    shown = [body_obj]
    barrel = built['Barrel_' + choice['Barrel']]
    barrel.location = socket_at(body_obj, 'Barrel')
    shown.append(barrel)
    muzzle = built['Muzzle_' + choice['Muzzle']]
    muzzle.location = socket_at(barrel, 'Muzzle')
    shown.append(muzzle)
    for slot in ('Magazine', 'Sight', 'Stock'):
        obj = built[f'{slot}_{choice[slot]}']
        obj.location = socket_at(body_obj, slot)
        shown.append(obj)
    return shown


# In the scene: the default rifle assembled, every other part laid out in rows behind it by slot.
assemble(DEFAULT)
for row, slot in enumerate(('Body', 'Barrel', 'Muzzle', 'Magazine', 'Sight', 'Stock')):
    for k, part in enumerate(PARTS[slot]):
        obj = built[f'{slot}_{part["key"]}']
        if part['key'] != DEFAULT[slot]:
            obj.location = Vector((0.0, -k * 0.3, (row + 1) * 0.4))

if lt.want_preview():
    lt.preview(assemble(DEFAULT), lt.preview_path('Backlog', 'Bullpup'), view=(-1.0, -0.25, 0.25), ground=False,
               lens=60.0, fit=0.95)
