"""The Powderkeg rocket launcher (Phase 12.3, concept B "frontier-tech": sand panels, gunmetal, brass, ember glow, cyan
optics), with interchangeable parts: 7 bodies (tubes), 7 barrels (front bells and extensions), 7 muzzle rings, 7
rocket kinds (the magazine), 6 sights and 7 shoulder rests, plus the flying rocket. Every launcher in the game is built
from them (EWeaponKind::Launcher, Rocket ammo; its definition made by the C++ work that follows). A segmented gunmetal
tube under two sand sleeves with an ember line, a flared exhaust behind, a bell in front with a brass ring, the loaded
rocket's nose showing and glowing in the bell's mouth, a cyan optic on top, a shoulder rest under the tube.

How the parts fit (gun space, cm: u along the tube from its back end, v up from its axis; see looter_guns):

  PowderkegBody_<Key>      the tube: the exhaust venturi (open, a dark throat inside), the segmented tube, two sand
                           sleeves (an ember line on the front one, rarity accent rings on the rear one), the sight
                           mount and rail between them, the pistol grip and trigger, the front grip. Sockets: Stock
                           (under the tube, where the shoulder rest hangs), Barrel (the tube's front face), Sight (the
                           rail's middle), Grip, Foregrip, Backblast (the exhaust's mouth, facing back: the back-blast
                           comes from it); the same on every body, so every part fits every body.
  PowderkegBarrel_<Key>    the front bell or extension, open, origin at the tube's front face. SOCKET_Rocket at its
                           mouth, where the loaded rocket seats; SOCKET_Muzzle there too (the muzzle ring hangs on it).
                           Every mouth is 5.8 cm round outside and at least 4.7 inside, so every ring and rocket fits.
  PowderkegMuzzle_<Key>    a ring on the mouth (origin at its back face); open (the rocket's nose pokes through it);
                           SOCKET_Muzzle at its front face, where the launch flash comes from.
  PowderkegMagazine_<Key>  the loaded rocket of that kind, hung on SOCKET_Rocket (the Magazine slot's socket is Rocket
                           here, not Magazine): its origin is where it seats in the mouth, its nose 5-12 cm out in
                           front (the part the player sees, glowing at the tip), the rest a short stub hidden in the
                           bell. The kind sets the rockets a load holds (the CSV's Magazine: 1, or 2-3 small ones).
  PowderkegRocket          the rocket that flies (and that the reload slides into the mouth): the frag nose, the motor
                           and a ring tail that fits the tube. Origin where it seats in the mouth, as the magazine's,
                           so the reload can slide it in along -X onto the socket; SOCKET_Exhaust at its tail, facing
                           back (the flame and smoke trail). No collision (the game sweeps it).
  PowderkegSight_<Key>     origin on top of the rail at its middle; SOCKET_Aim on its line of sight (the rear ring, the
                           dot or the eyepiece lens). Optics are open tubes, so they can be aimed through.
  PowderkegStock_<Key>     the shoulder rest, origin at the tube's underside where it hangs; reaches down and along.

Each option's display name, name word, lowest rarity and stat ranges live in Powderkeg.parts.csv beside this file
(Magazine = rockets, Zoom = magnification), which Tools/Unreal/setup_gun_parts.py reads into the launcher's definition.
A new part needs a model here and a row there, under the same key; never rename or reuse a key.

    blender -b --factory-startup --python Art/Models/Weapons/Powderkeg.py -- --preview
"""
import math

import bpy
from mathutils import Matrix, Vector

import looter_guns as lg
import looter_props as lp
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xe8e8e8, 2.0)   # glows in the gun's rarity color in the game
RETICLE = lg.RETICLE                                 # the kit's frontier-tech glows, shared by the four new guns
EMBER = lg.EMBER                                     # GunEmberGlow: fixed ember lines, vents and the rockets' tips
OPTIC = lg.CYAN                                      # GunCyanGlow: cyan optics
SHELL = lg.POLY_SAND
OLIVE = lg.material('GunOlivePaint', 'PaintWorn', 0x6f7550)
WARHEAD = lg.material('GunShellRed', 'Polymer', 0xb8402f)   # the Ranchhand's shell red: a frag rocket's nose
LEATHER = lg.material('GunLeather', 'Polymer', 0x7a5236)

TUBE_R = 4.2          # the tube's radius
SLEEVE_R = 4.8        # the sand sleeves'
TUBE_END = 78.0       # the tube's front face
MOUTH_R = 5.8         # every barrel's mouth, outside
WALL_MOUTH = 0.9      # its wall: 4.9 inside

# Where the parts attach (gun space, cm).
STOCK_AT = (27.0, -TUBE_R)
BARREL_AT = (TUBE_END, 0.0)
SIGHT_AT = (42.0, 5.95)
MUZZLE_AT = (140.0, 0.0)   # muzzle rings are modeled here, around their own origin
ROCKET_AT = (180.0, 0.0)   # the rockets: here, around where they seat
GRIP = (48.6, -10.0)
FOREGRIP = (70.0, -9.5)

g = lg.Gun('Powderkeg')
origins = {}


def model(slot, key, at):
    name = f'{slot}_{key}'
    origins[name] = at
    return name


# --- Shared pieces ---

WALL = 0.12   # cm: thin tube walls keep a sight's clear opening as wide as the tube


def hollow(profile, u0, v, segments, mat, wall=WALL, w=0.0, spin=0.0):
    """A tube turned round an axis along u (profile [(du, r), ...]) but open at both ends: a wall that follows the
    outline inside (sight tubes, bells, rings)."""
    outer = [(r * lg.CM, du * lg.CM) for du, r in profile]
    inner = [((r - wall) * lg.CM, du * lg.CM) for du, r in reversed(profile)]
    obj = lp.lathe(outer + inner, segments=segments, closed=True)[0]
    obj.data.transform(Matrix.Translation(lg.at(u0, v, w)) @ Matrix.Rotation(math.radians(90.0), 4, 'X')
                       @ Matrix.Rotation(math.radians(spin), 4, 'Z'))
    return lg.mapped(obj, mat)


def ring(u, v, r, length, mat, wall=0.25, segments=32):
    return hollow([(0.0, r), (length, r)], u, v, segments, mat, wall=wall)


def about_bore(obj, degrees):
    """Turns a part about the tube's axis (a shape drawn on top lands round the side)."""
    obj.data.transform(Matrix.Rotation(math.radians(degrees), 4, 'Y'))
    return obj


def optics(m, u_eye, u_obj, v, r_eye, r_obj, wall=WALL, dot=False):
    """A sight's glass: a lens inside each end and a reticle just behind the objective (a dot, or a fine cross)."""
    g.add(m, lg.tube(u_eye + 0.15, u_eye + 0.25, r_eye - wall - 0.03, v=v, segments=20, mat=lg.LENS))
    g.add(m, lg.tube(u_obj - 0.25, u_obj - 0.15, r_obj - wall - 0.03, v=v, segments=24, mat=lg.LENS))
    u = u_obj - 0.4
    if dot:
        g.add(m, lg.pin(u, v, -0.07, 0.07, 0.07, mat=RETICLE, segments=10))
    else:
        g.add(m, lg.box(u, u + 0.02, v - 0.075, v + 0.075, 0.02, bevel=0.0, mat=RETICLE))
        g.add(m, lg.box(u, u + 0.02, v - 0.01, v + 0.01, 0.15, bevel=0.0, mat=RETICLE))


def scroll(m, cu, cv, w, r0, turns=1.6, mat=lg.BRASS, flip=1.0):
    """A curl of inlaid engraving on a side face at w."""
    pts = []
    for k in range(18):
        t = k / 17.0
        a = flip * 2.0 * math.pi * turns * t
        r = r0 * (1.0 - 0.75 * t)
        pts.append((cu + r * math.cos(a), cv + r * math.sin(a), w))
    g.add(m, lg.pipe(pts, 0.07, mat=mat, sides=5))


def sleeve(u0, u1, mat, r=SLEEVE_R, segments=32):
    """A sand sleeve round the tube, its ends rounded down onto it."""
    return lg.turned([(0.0, TUBE_R + 0.05), (0.7, r), (u1 - u0 - 0.7, r), (u1 - u0, TUBE_R + 0.05)], u0,
                     segments=segments, mat=mat)


def side_line(m, u0, u1, mat, r=SLEEVE_R, height=0.4):
    """A flat glowing line along both sides of a sleeve, at its equator."""
    for side in (-1.0, 1.0):
        g.add(m, lg.box(u0, u1, -height * 0.5, height * 0.5, 0.12, w=side * (r + 0.03), bevel=0.0, mat=mat))


# --- Bodies: the tube and its fittings, all with the same sockets ---

GRIP_OUT = [(46.6, -4.9), (52.4, -4.9), (52.0, -6.0), (50.8, -12.6), (50.2, -14.2), (48.8, -14.8), (46.4, -14.6),
            (45.6, -13.8), (45.2, -12.4), (46.4, -6.4)]
FRONT_GRIP = [(68.2, -5.0), (72.0, -5.0), (71.6, -12.8), (70.8, -13.8), (69.0, -13.8), (68.4, -13.0)]


def tube(m, mat=lg.BLUED, rings=lg.BLACK, venturi=lg.BLACK):
    """The exhaust venturi (open, a dark throat inside), the tube and its segment rings."""
    g.add(m, hollow([(0.0, 6.2), (1.2, 6.2), (4.5, 5.1), (9.6, 4.5), (11.2, 4.5)], 0.0, 0.0, 32, venturi, wall=0.5))
    g.add(m, lg.turned([(0.0, 3.8), (2.6, 2.0), (2.8, 0.0)], 8.6, segments=24, mat=lg.BLACK))   # the throat
    g.add(m, lg.tube(10.8, TUBE_END, TUBE_R, segments=32, mat=mat))
    for u0, u1, r in ((11.0, 12.6, 4.6), (34.4, 35.6, 4.5), (48.4, 49.6, 4.5), (76.4, TUBE_END, 4.65)):
        g.add(m, lg.turned([(0.0, r - 0.15), (0.2, r), (u1 - u0 - 0.2, r), (u1 - u0, r - 0.15)], u0, segments=32,
                           mat=rings))


def sleeves(m, mat=SHELL, rear=(13.4, 34.2), front=(50.0, 76.0), ember=True, accent=True):
    g.add(m, sleeve(*rear, mat))
    g.add(m, sleeve(*front, mat))
    if ember:
        side_line(m, front[0] + 2.4, front[1] - 2.4, EMBER, height=0.55)
    if accent:   # rarity rings near the rear sleeve's ends
        for u in (rear[0] + 1.4, rear[1] - 1.8):
            g.add(m, ring(u, 0.0, SLEEVE_R + 0.06, 0.4, ACCENT, wall=0.2))


def fittings(m, grip=lg.POLY_GREY, metal=lg.BLACK, guard=lg.BLACK, trigger=lg.STEEL, cap=None):
    """What every body has: the sight mount and rail, the grip bracket, pistol grip, trigger and guard, the front grip,
    a sling loop, and the sockets."""
    g.add(m, lg.slab([(36.2, 3.4), (47.8, 3.4), (47.8, 4.4), (47.0, 5.05), (37.0, 5.05), (36.2, 4.4)], 3.2, bevel=0.3,
                     mat=metal, round=0.4))
    g.add(m, lg.rail(36.6, 47.4, 5.0, width=2.0, mat=metal))
    g.add(m, lg.slab([(45.6, -3.4), (58.4, -3.4), (58.4, -4.6), (57.6, -5.3), (46.2, -5.3), (45.6, -4.6)], 3.0,
                     bevel=0.3, mat=metal, round=0.4))                          # the grip bracket
    g.add(m, lg.slab(GRIP_OUT, 3.2, bevel=0.8, segments=3, mat=grip, round=0.8))
    if cap:
        g.add(m, lg.slab([(45.4, -13.6), (50.6, -13.8), (50.0, -14.9), (48.8, -15.4), (46.2, -15.2)], 3.0, bevel=0.25,
                         mat=cap, round=0.4))
    g.add(m, lg.strip([(52.4, -5.2), (52.8, -7.8), (55.0, -8.6), (57.4, -7.4), (57.6, -5.2)], 1.0, 0.36, mat=guard))
    g.add(m, lg.slab([(53.8, -5.2), (54.8, -5.2), (54.6, -6.4), (54.2, -7.4), (53.6, -7.6), (53.9, -6.4)], 0.6,
                     bevel=0.12, mat=trigger))
    g.add(m, lg.slab([(67.4, -3.4), (72.8, -3.4), (72.8, -4.6), (72.0, -5.4), (68.2, -5.4), (67.4, -4.6)], 3.0,
                     bevel=0.3, mat=metal, round=0.4))                          # the front grip's clamp
    fg = lg.slab(FRONT_GRIP, 3.0, bevel=0.7, segments=3, mat=grip, round=0.8)
    g.add(m, lg.cut(fg, [lg.cbox(67.0, 68.9, v - 0.25, v + 0.25, -2.0, 2.0) for v in (-7.0, -8.6, -10.2, -11.8)]))
    g.add(m, lg.pipe([(14.6, -4.0), (14.8, -5.6), (17.0, -5.6), (17.2, -4.0)], 0.28, mat=lg.STEEL))   # sling loop
    for name, uv in (('Stock', STOCK_AT), ('Barrel', BARREL_AT), ('Sight', SIGHT_AT), ('Grip', GRIP),
                     ('Foregrip', FOREGRIP)):
        g.socket(m, name, uv)
    g.socket(m, 'Backblast', (0.0, 0.0), rotation=(0.0, 0.0, 180.0))


def bodies():
    B = lambda key: model('Body', key, (0.0, 0.0))

    m = B('Standard')                                          # the Powderkeg's: gunmetal, sand sleeves, ember line
    tube(m)
    sleeves(m)
    fittings(m)

    m = B('Carbon')                                            # black, tan sleeves, black grips
    tube(m, lg.BLACK)
    sleeves(m, lg.POLY_TAN)
    fittings(m, grip=lg.POLY_BLACK)

    m = B('Salvager')                                          # a rusty tube, welded patch sleeves, taped grips
    tube(m, lg.SALVAGE, rings=lg.STEEL, venturi=lg.SALVAGE)
    sleeves(m, lg.SALVAGE, ember=False)
    for side in (-1.0, 1.0):
        for u in (16.0, 22.0, 28.0, 52.0, 58.0, 64.0, 70.0, 74.0):
            for v in (-1.6, 1.6):
                g.add(m, lg.rivet(u, v, side * (SLEEVE_R - 0.25), side, r=0.32, mat=lg.STEEL))
        g.add(m, lg.pipe([(12.0, 3.2, side * 3.2), (45.0, 3.4, side * 3.4), (77.0, 3.2, side * 3.2)], 0.3,
                         mat=lg.STEEL, sides=6))
    for u in (55.0, 66.0):                                     # straps
        g.add(m, ring(u, 0.0, SLEEVE_R + 0.12, 1.4, lg.TAPE, wall=0.3))
    fittings(m, grip=lg.TAPE, guard=lg.STEEL, metal=lg.STEEL)

    m = B('Armored')                                           # olive sleeves, bolted side plates
    tube(m)
    sleeves(m, OLIVE)
    for side in (-1.0, 1.0):
        w = side * (SLEEVE_R + 0.1)
        for u0, u1 in ((15.0, 32.6), (51.6, 74.4)):
            g.add(m, lg.slab([(u0, -1.8), (u1, -1.8), (u1 + 0.6, -1.0), (u1 + 0.6, 1.0), (u1, 1.8), (u0, 1.8),
                              (u0 - 0.6, 1.0), (u0 - 0.6, -1.0)], 0.6, w=w, bevel=0.15, mat=lg.STEEL, round=0.4))
            for u in (u0 + 0.8, u1 - 0.8):
                for v in (-1.1, 1.1):
                    g.add(m, lg.rivet(u, v, w + side * 0.32, side, r=0.32, mat=lg.BLACK))
        g.add(m, lg.box(53.0, 73.0, -0.2, 0.2, 0.1, w=side * (SLEEVE_R + 0.43), bevel=0.0, mat=EMBER))
    fittings(m, grip=lg.RUBBER)

    m = B('Ribbed')                                            # cooling fins down the middle, brass collars
    tube(m)
    sleeves(m, rear=(13.4, 30.0), front=(54.0, 76.0))
    for k in range(13):
        u = 31.0 + k * 1.75
        if 35.4 < u < 48.6:
            r = 5.1                                            # under the sight mount: lower fins
        else:
            r = 5.5
        g.add(m, lg.turned([(0.0, TUBE_R), (0.15, r), (0.55, r), (0.7, TUBE_R)], u, segments=24, mat=lg.BLACK))
    for u in (29.4, 53.4):
        g.add(m, ring(u, 0.0, SLEEVE_R + 0.15, 1.2, lg.BRASS, wall=0.4))
    fittings(m)

    m = B('Lattice')                                           # sleeves cut in a lattice over a glowing ember core
    tube(m)
    for u0, u1 in ((13.4, 34.2), (50.0, 76.0)):
        g.add(m, lg.tube(u0 + 1.0, u1 - 1.0, TUBE_R + 0.2, segments=32, mat=EMBER))
        windows = []
        for k, u in enumerate(range(int(u0) + 2, int(u1) - 3, 4)):
            for a in range(30 if k % 2 else 0, 360, 60):
                if 0 <= a % 360 < 31 or a % 360 > 329:
                    continue                                   # keep the top whole (the sight mount's line)
                windows.append(lg.cbox(u, u + 2.6, TUBE_R + 0.1, SLEEVE_R + 1.0, -0.9, 0.9, rotate_u=a))
        g.add(m, lg.cut(sleeve(u0, u1, SHELL), windows))
    fittings(m)

    m = B('Gilded')                                            # sand sleeves, brass bands and engraving, more ember
    tube(m, rings=lg.BRASS, venturi=lg.BRASS)
    sleeves(m)
    side_line(m, 16.0, 31.6, EMBER, height=0.3)
    for u in (13.6, 33.0, 50.2, 74.8):
        g.add(m, ring(u, 0.0, SLEEVE_R + 0.1, 0.8, lg.BRASS, wall=0.4))
    for side in (-1.0, 1.0):                                   # engraved brass plates on the grip bracket
        w = side * 1.56
        g.add(m, lg.slab([(46.4, -3.7), (57.6, -3.7), (57.6, -5.0), (46.4, -5.0)], 0.12, w=w, bevel=0.0, mat=lg.BRASS))
        for k, u in enumerate((48.4, 52.0, 55.6)):
            scroll(m, u, -4.35, w + side * 0.1, 0.5, flip=side if k % 2 == 0 else -side)
    fittings(m, guard=lg.BRASS, metal=lg.BLACK, cap=lg.BRASS)


# --- Barrels: the front bells and extensions, open, from the tube's front face ---

def barrels():
    u0 = BARREL_AT[0]

    def mouth(m, length):
        g.socket(m, 'Rocket', (u0 + length, 0.0))
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    m = model('Barrel', 'Bell', BARREL_AT)                     # the Powderkeg's: a short flared bell
    g.add(m, hollow([(0.0, 4.55), (2.0, 4.7), (8.0, 5.3), (12.0, MOUTH_R), (13.0, MOUTH_R)], u0, 0.0, 32, lg.BLACK,
                    wall=WALL_MOUTH))
    g.add(m, ring(u0 - 0.6, 0.0, 4.9, 1.8, lg.BLACK, wall=0.6))             # the collar over the tube's end
    mouth(m, 13.0)

    m = model('Barrel', 'Stub', BARREL_AT)                     # a short straight bell
    g.add(m, hollow([(0.0, 5.2), (1.0, 5.5), (7.0, 5.6), (8.0, MOUTH_R)], u0, 0.0, 32, lg.BLACK, wall=WALL_MOUTH))
    g.add(m, ring(u0 - 0.6, 0.0, 5.0, 1.8, lg.BLACK, wall=0.6))
    mouth(m, 8.0)

    m = model('Barrel', 'Long', BARREL_AT)                     # a long extension with a sand sleeve, then a flare
    g.add(m, hollow([(0.0, 5.0), (19.0, 5.2), (22.5, MOUTH_R), (24.0, MOUTH_R)], u0, 0.0, 32, lg.BLUED,
                    wall=WALL_MOUTH))
    g.add(m, hollow([(0.0, 5.2), (0.6, 5.55), (9.4, 5.55), (10.0, 5.2)], u0 + 4.0, 0.0, 32, SHELL, wall=0.4))
    for u in (2.4, 15.6):
        g.add(m, ring(u0 + u, 0.0, 5.45, 1.0, lg.BLACK, wall=0.5))
    g.add(m, ring(u0 - 0.6, 0.0, 5.1, 1.8, lg.BLACK, wall=0.6))
    mouth(m, 24.0)

    m = model('Barrel', 'Flared', BARREL_AT)                   # a wide trumpet bell, its lip rolled back to the mouth
    g.add(m, hollow([(0.0, 4.55), (5.0, 4.8), (10.0, 5.6), (13.0, 6.9), (14.2, 7.1), (15.0, 6.6), (15.4, MOUTH_R)], u0,
                    0.0, 32, lg.BLACK, wall=WALL_MOUTH))
    g.add(m, ring(u0 - 0.6, 0.0, 4.9, 1.8, lg.BLACK, wall=0.6))
    mouth(m, 15.4)

    m = model('Barrel', 'Vented', BARREL_AT)                   # a ported extension, rows of round vents
    vented = hollow([(0.0, 5.1), (16.0, 5.3), (17.0, MOUTH_R), (18.0, MOUTH_R)], u0, 0.0, 32, lg.BLACK, wall=WALL_MOUTH)
    holes = []
    for k, u in enumerate((3.0, 6.0, 9.0, 12.0)):
        holes.append(lg.cpin(u0 + u, 0.0, -7.0, 7.0, 0.7, segments=12))
        holes.append(about_bore(lg.cpin(u0 + u + 1.5, 0.0, -7.0, 7.0, 0.7, segments=12), 45.0))
        holes.append(about_bore(lg.cpin(u0 + u + 1.5, 0.0, -7.0, 7.0, 0.7, segments=12), -45.0))
    g.add(m, lg.cut(vented, holes))
    g.add(m, hollow([(0.0, 4.3), (14.0, 4.3)], u0 + 1.0, 0.0, 32, lg.BLACK, wall=0.2))   # the liner seen through the vents
    g.add(m, ring(u0 - 0.6, 0.0, 5.2, 1.8, lg.BLACK, wall=0.6))
    mouth(m, 18.0)

    m = model('Barrel', 'Guided', BARREL_AT)                   # a bell with a cyan guidance band and a seeker pod
    g.add(m, hollow([(0.0, 4.55), (2.0, 4.8), (9.0, 5.4), (13.0, MOUTH_R), (14.0, MOUTH_R)], u0, 0.0, 32, lg.BLUED,
                    wall=WALL_MOUTH))
    g.add(m, ring(u0 + 4.6, 0.0, 5.12, 0.6, OPTIC, wall=0.3))
    g.add(m, ring(u0 - 0.6, 0.0, 4.9, 1.8, lg.BLACK, wall=0.6))
    pod = lg.slab([(u0 + 2.0, 4.6), (u0 + 10.0, 5.0), (u0 + 10.0, 6.6), (u0 + 9.0, 7.2), (u0 + 3.0, 7.0),
                   (u0 + 2.0, 6.4)], 2.4, bevel=0.3, mat=lg.BLACK, round=0.4)
    g.add(m, pod)
    g.add(m, lg.tube(u0 + 9.95, u0 + 10.15, 0.6, v=6.0, segments=16, mat=OPTIC))
    mouth(m, 14.0)

    m = model('Barrel', 'Siege', BARREL_AT)                    # long, brass-banded, ember vents, a heavy flare
    g.add(m, hollow([(0.0, 5.1), (18.0, 5.3), (22.0, 6.2), (24.0, 6.3), (24.6, MOUTH_R)], u0, 0.0, 32, lg.BLUED,
                    wall=WALL_MOUTH))
    for u, r in ((1.0, 5.25), (8.0, 5.35), (15.0, 5.4), (22.6, 6.45)):
        g.add(m, ring(u0 + u, 0.0, r + 0.05, 1.0, lg.BRASS, wall=0.4))
    for side in (-1.0, 1.0):
        for u in (3.0, 10.0):
            for a in (-28.0, 0.0, 28.0):                       # ember slots round the sides
                slot = lg.box(u0 + u, u0 + u + 3.6, -0.24, 0.24, 0.12, w=side * 5.36, bevel=0.0, mat=EMBER)
                g.add(m, about_bore(slot, a))
    g.add(m, ring(u0 - 0.6, 0.0, 5.2, 1.8, lg.BLACK, wall=0.6))
    mouth(m, 24.6)


# --- Muzzle rings: on the mouth, open, around their own origin ---

def muzzles():
    u0 = MUZZLE_AT[0]

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    m = model('Muzzle', 'Ring', MUZZLE_AT)                     # the Powderkeg's: a brass ring
    g.add(m, hollow([(0.0, 6.0), (0.3, 6.15), (1.0, 6.15), (1.3, 6.0)], u0, 0.0, 32, lg.BRASS, wall=1.2))
    tip(m, 1.3)

    m = model('Muzzle', 'Lip', MUZZLE_AT)                      # a plain gunmetal lip
    g.add(m, hollow([(0.0, 6.0), (0.5, 6.2), (0.9, 6.0)], u0, 0.0, 32, lg.BLACK, wall=1.15))
    tip(m, 0.9)

    m = model('Muzzle', 'Crown', MUZZLE_AT)                    # a toothed crown
    g.add(m, hollow([(0.0, 6.0), (1.0, 6.0)], u0, 0.0, 32, lg.BLACK, wall=1.2))
    for a in range(0, 360, 45):
        tooth = lg.slab([(u0 + 0.8, 4.95), (u0 + 2.8, 5.2), (u0 + 2.8, 5.9), (u0 + 0.8, 6.0)], 1.3, bevel=0.15,
                        mat=lg.BLACK)
        g.add(m, about_bore(tooth, a + 22.5))
    tip(m, 2.8)

    m = model('Muzzle', 'Deflector', MUZZLE_AT)                # a slotted collar that throws the blast sideways
    defl = hollow([(0.0, 6.0), (3.2, 6.3)], u0, 0.0, 32, lg.STEEL, wall=1.3)
    g.add(m, lg.cut(defl, [about_bore(lg.cbox(u0 + 0.8, u0 + 2.6, 4.0, 7.0, -0.5, 0.5), a) for a in range(0, 360, 45)]))
    tip(m, 3.2)

    m = model('Muzzle', 'Baffle', MUZZLE_AT)                   # a cage of three rods and a front ring
    g.add(m, hollow([(0.0, 6.0), (0.9, 6.0)], u0, 0.0, 32, lg.BLACK, wall=1.2))
    g.add(m, hollow([(0.0, 5.6), (0.8, 5.6)], u0 + 6.0, 0.0, 32, lg.STEEL, wall=0.7))
    for a in (90.0, 210.0, 330.0):
        x, y = math.cos(math.radians(a)) * 5.3, math.sin(math.radians(a)) * 5.3
        g.add(m, lg.tube(u0 + 0.8, u0 + 6.1, 0.32, v=y, w=x, segments=8, mat=lg.STEEL))
    tip(m, 6.8)

    m = model('Muzzle', 'Coil', MUZZLE_AT)                     # a brass ring wound with a glowing ember coil
    g.add(m, hollow([(0.0, 6.0), (0.8, 6.0)], u0, 0.0, 32, lg.BRASS, wall=1.2))
    g.add(m, lg.helix(u0 + 0.8, u0 + 4.0, 5.0, 5.7, 0.9, 0.45, steps=28, mat=EMBER))
    g.add(m, hollow([(0.0, 5.9), (0.7, 5.9)], u0 + 4.0, 0.0, 32, lg.BRASS, wall=1.1))
    tip(m, 4.7)

    m = model('Muzzle', 'Gilded', MUZZLE_AT)                   # a flared engraved brass crown, an ember inlay
    g.add(m, hollow([(0.0, 6.0), (1.4, 6.3), (2.4, 6.9), (2.6, 6.8)], u0, 0.0, 32, lg.BRASS, wall=1.6))
    g.add(m, ring(u0 + 1.0, 0.0, 6.22, 0.3, EMBER, wall=0.2))
    for a in range(0, 360, 60):
        stud = lg.upright(u0 + 0.6, 6.0, 6.35, 0.25, mat=lg.BRASS, segments=8)
        g.add(m, about_bore(stud, a))
    tip(m, 2.6)


# --- Rockets: the loaded kinds (the magazine) and the flying rocket, around where they seat ---

FRAG_NOSE = [(-12.0, 1.9), (-7.0, 2.0), (-6.0, 3.2), (-4.2, 4.0), (-0.6, 4.0), (1.2, 3.9), (3.6, 3.3), (5.6, 2.4),
             (7.0, 1.6)]


def glow_tip(m, u, r, length=0.9):
    """The rocket's lit fuze cap, glowing at the nose (what the player sees in the bell's mouth)."""
    g.add(m, lg.turned([(0.0, r), (length * 0.45, r * 0.8), (length * 0.85, r * 0.38), (length, 0.0)], u, segments=20,
                       mat=EMBER))


def frag(m, u0, body=WARHEAD, band=lg.BRASS, tip=True):
    g.add(m, lg.turned(FRAG_NOSE, u0, segments=28, mat=body))
    g.add(m, lg.tube(u0 + 0.4, u0 + 1.3, 4.0, segments=28, mat=band))      # the fuze band, just out of the mouth
    if tip:
        glow_tip(m, u0 + 6.95, 1.62, length=2.1)


def mini(m, u0, w=0.0, r=1.9, length=6.6, body=WARHEAD):
    """A small rocket's nose (two or three to a load), in a sabot."""
    g.add(m, lg.turned([(-6.0, r), (0.8, r * 1.02), (length * 0.52, r * 0.9), (length * 0.82, r * 0.5),
                        (length, r * 0.2)], u0, w=w, segments=20, mat=body))
    glow_tip(m, u0 + length - 0.1, r * 0.24, length=0.5)


def rockets():
    u0 = ROCKET_AT[0]
    M = lambda key: model('Magazine', key, ROCKET_AT)

    m = M('Frag')                                              # the Powderkeg's: a red frag nose, a glowing fuze
    frag(m, u0)

    m = M('Slammer')                                           # an olive shaped charge on a standoff probe
    g.add(m, lg.turned([(-12.0, 1.9), (-7.0, 2.0), (-6.0, 3.2), (-4.2, 4.0), (-0.5, 4.0), (1.5, 3.6), (4.5, 1.6),
                        (5.0, 0.9)], u0, segments=28, mat=OLIVE))
    g.add(m, lg.tube(u0 + 0.4, u0 + 1.2, 4.0, segments=28, mat=lg.BLACK))
    g.add(m, lg.tube(u0 + 4.6, u0 + 11.6, 0.42, segments=12, mat=lg.STEEL))   # the probe
    g.add(m, lg.tube(u0 + 9.0, u0 + 9.6, 0.62, segments=12, mat=lg.BLACK))
    glow_tip(m, u0 + 11.5, 0.5, length=0.7)

    m = M('Cluster')                                           # a blunt tan dome in petals over its bomblets
    dome = lg.turned([(-12.0, 1.9), (-7.0, 2.0), (-6.0, 3.2), (-4.2, 4.0), (-0.5, 4.0), (1.6, 3.9), (3.6, 3.4),
                      (5.0, 2.6), (6.0, 1.6), (6.5, 0.6)], u0, segments=28, mat=lg.POLY_TAN)
    g.add(m, lg.cut(dome, [about_bore(lg.cbox(u0 + 1.6, u0 + 7.0, 0.0, 5.0, -0.12, 0.12), a) for a in (45, 135, 225, 315)]))
    g.add(m, lg.tube(u0 + 0.4, u0 + 1.2, 4.02, segments=28, mat=lg.BLACK))
    g.add(m, lg.turned([(1.4, 3.65), (3.6, 3.15), (5.0, 2.35), (6.0, 1.35), (6.3, 0.5)], u0, segments=28,
                       mat=EMBER))                     # the glowing core the petals' seams show
    glow_tip(m, u0 + 6.4, 0.6, length=0.4)

    m = M('Hornet')                                            # three small rockets a load: one in a sabot
    g.add(m, lg.turned([(-3.0, 4.4), (0.6, 4.4), (1.0, 3.9), (1.2, 2.0)], u0, segments=28, mat=lg.BLACK))
    for a in range(0, 360, 90):                                # the sabot's petals
        g.add(m, about_bore(lg.box(u0 - 2.8, u0 + 0.8, 3.9, 4.5, 0.4, bevel=0.0, mat=lg.STEEL), a + 45))
    mini(m, u0)

    m = M('Twin')                                              # two small rockets a load, side by side
    g.add(m, lg.turned([(-3.0, 4.4), (0.6, 4.4), (1.0, 4.0), (1.2, 3.6)], u0, segments=28, mat=lg.BLACK))
    for w in (-1.95, 1.95):
        mini(m, u0, w=w, r=1.7, length=6.0)

    m = M('Incendiary')                                        # a black nose ringed with ember bands
    frag(m, u0, body=lg.BLACK, band=lg.STEEL)
    for du, r in ((2.4, 3.65), (4.4, 3.0), (6.2, 2.05)):
        g.add(m, lg.tube(u0 + du, u0 + du + 0.5, r + 0.04, segments=28, mat=EMBER))

    m = M('Thunder')                                           # a long brass warhead, blued bands, a cyan seeker eye
    g.add(m, lg.turned([(-12.0, 1.9), (-7.0, 2.0), (-6.0, 3.2), (-4.2, 4.1), (-0.5, 4.1), (2.0, 4.0), (5.0, 3.4),
                        (8.0, 2.4), (10.2, 1.3)], u0, segments=28, mat=lg.BRASS))
    for du, r in ((0.4, 4.12), (3.4, 3.78), (6.6, 3.0)):
        g.add(m, lg.tube(u0 + du, u0 + du + 0.7, r, segments=28, mat=lg.BLUED))
    g.add(m, ring(u0 + 1.4, 0.0, 4.1, 0.35, EMBER, wall=0.2))
    g.add(m, lg.turned([(0.0, 1.3), (0.6, 1.1), (1.0, 0.0)], u0 + 10.1, segments=20, mat=OPTIC))   # the seeker eye

    # The rocket that flies (and that the reload slides in): the frag nose, the motor, a ring tail that fits the tube.
    m = 'Rocket'
    origins[m] = ROCKET_AT
    frag(m, u0)
    g.add(m, lg.tube(u0 - 38.0, u0 - 6.8, 1.95, segments=20, mat=lg.BLUED))          # the motor
    for du in (-30.0, -20.0, -10.0):
        g.add(m, lg.tube(u0 + du, u0 + du + 0.6, 2.05, segments=20, mat=lg.BLACK))
    for a in range(0, 360, 90):                                 # tail fins, in a ring that fits the tube
        g.add(m, about_bore(lg.box(u0 - 40.0, u0 - 33.0, 1.8, 3.55, 0.25, bevel=0.0, mat=lg.BLACK), a + 45))
    g.add(m, hollow([(0.0, 3.75), (4.0, 3.75)], u0 - 40.2, 0.0, 28, lg.BLACK, wall=0.3))
    g.add(m, hollow([(0.0, 1.5), (2.0, 1.95)], u0 - 40.0, 0.0, 20, lg.STEEL, wall=0.3))   # the nozzle
    g.add(m, lg.turned([(0.0, 1.3), (0.4, 0.0)], u0 - 38.6, segments=16, mat=EMBER))   # the burn inside the nozzle
    g.socket(m, 'Exhaust', (u0 - 40.4, 0.0), rotation=(0.0, 0.0, 180.0))


# --- Sights: around the middle of the rail ---

def sights():
    v0 = SIGHT_AT[1]
    S = lambda key: model('Sight', key, SIGHT_AT)

    m = S('Ladder')                                            # a flip-up ladder rear and a hooded front post
    h = v0 + 2.6
    g.add(m, lg.box(36.6, 39.0, v0, v0 + 0.6, 2.4, bevel=0.15, mat=lg.BLACK))
    leaf = lg.slab([(37.0, v0 + 0.6), (38.6, v0 + 0.6), (38.3, h + 1.2), (37.3, h + 1.2)], 2.2, bevel=0.15,
                   mat=lg.BLACK, round=0.3)
    leaf = lg.cut(leaf, [lg.tube(36.0, 40.0, 0.4, v=h, segments=16),
                         lg.cbox(36.0, 40.0, v0 + 0.9, h - 0.8, -0.6, 0.6)])     # the peep and the ladder's window
    g.add(m, leaf)
    g.add(m, hollow([(0.0, 0.62), (0.5, 0.62)], 37.55, h, 16, lg.BRASS, wall=0.2))
    for k in range(3):                                         # range marks
        g.add(m, lg.box(37.4, 37.5, v0 + 1.0 + k * 0.4, v0 + 1.1 + k * 0.4, 2.3, bevel=0.0, mat=lg.BRASS))
    g.add(m, lg.box(45.4, 47.4, v0, v0 + 0.6, 2.4, bevel=0.15, mat=lg.BLACK))
    g.add(m, lg.slab([(45.9, v0 + 0.6), (46.9, v0 + 0.6), (46.6, h), (46.2, h)], 0.7, bevel=0.1, mat=lg.BLACK))
    g.add(m, lg.pin(46.25, h - 0.15, -0.08, 0.08, 0.09, mat=OPTIC, segments=8))
    g.add(m, ring(46.0, h, 1.25, 0.6, lg.BLACK, wall=0.25))
    for w in (-1.1, 1.1):
        g.add(m, lg.box(45.8, 46.8, v0 + 0.6, h - 0.9, 0.3, w=w, bevel=0.05, mat=lg.BLACK))
    g.socket(m, 'Aim', (37.8, h))   # the rear peep: the front post's tip shows in it

    def optic(m, u_eye, length, axis, r_eye, r_tube, r_obj, mat=lg.BLACK, rings=lg.BLACK, glow=True, ember=False,
              dot=True):
        g.add(m, hollow([(0.0, r_eye), (2.4, r_eye), (3.2, r_tube), (length - 3.0, r_tube), (length - 2.0, r_obj),
                         (length, r_obj)], u_eye, axis, 24, mat))
        for u in (u_eye + 2.8, u_eye + length - 3.6):
            g.add(m, ring(u - 0.6, axis, r_tube + 0.22, 1.2, rings, wall=0.24))
            g.add(m, lg.box(u - 0.9, u + 0.9, v0, axis - r_tube + 0.05, 2.0, bevel=0.2, mat=rings))
        if glow:
            g.add(m, ring(u_eye + length - 0.6, axis, r_obj + 0.08, 0.5, OPTIC, wall=0.3))
        if ember:
            for side in (-1.0, 1.0):
                g.add(m, lg.box(u_eye + 4.0, u_eye + length - 4.4, axis - 0.1, axis + 0.1, 0.08,
                                w=side * (r_tube + 0.02), bevel=0.0, mat=EMBER))
        optics(m, u_eye, u_eye + length, axis, r_eye, r_obj, dot=dot)
        g.socket(m, 'Aim', (u_eye + 0.2, axis))   # the eyepiece lens

    m = S('Optic')                                             # the Powderkeg's: a short tube, a cyan objective
    optic(m, 36.6, 11.0, v0 + 2.9, 1.6, 1.3, 1.75)
    g.add(m, lg.upright(42.0, v0 + 2.9 + 1.2, v0 + 2.9 + 2.0, 0.7, mat=lg.BLACK, segments=14))

    m = S('Reflex')                                            # an open window, a cyan-lit frame, a red dot
    g.add(m, lg.box(37.4, 46.6, v0, v0 + 1.0, 2.8, bevel=0.25, mat=lg.BLACK))
    frame = lg.slab([(41.0, v0 + 1.0), (46.4, v0 + 1.0), (46.4, v0 + 5.0), (45.0, v0 + 5.8), (42.4, v0 + 5.8),
                     (41.0, v0 + 5.0)], 2.8, bevel=0.2, mat=lg.BLACK, round=0.3)
    g.add(m, lg.cut(frame, [lg.cbox(40.0, 47.4, v0 + 1.6, v0 + 5.2, -1.1, 1.1)]))
    g.add(m, lg.box(45.6, 45.75, v0 + 1.6, v0 + 5.2, 2.2, bevel=0.0, mat=lg.LENS))
    g.add(m, lg.box(41.2, 46.2, v0 + 5.75, v0 + 5.9, 1.2, bevel=0.0, mat=OPTIC))
    g.add(m, lg.box(37.8, 40.2, v0 + 1.0, v0 + 1.8, 2.0, bevel=0.15, mat=lg.BLACK))   # the emitter
    g.add(m, lg.pin(45.5, v0 + 3.4, -0.12, 0.12, 0.12, mat=RETICLE))
    g.socket(m, 'Aim', (45.5, v0 + 3.4))   # the dot

    m = S('Rangefinder')                                       # the optic with a cyan-screened rangefinder on its left
    optic(m, 36.6, 11.0, v0 + 2.9, 1.6, 1.3, 1.75)
    g.add(m, lg.box(38.4, 45.4, v0 + 1.6, v0 + 4.0, 2.0, w=2.6, bevel=0.3, mat=lg.BLACK))
    g.add(m, lg.box(38.25, 38.4, v0 + 2.0, v0 + 3.6, 1.4, w=2.6, bevel=0.0, mat=OPTIC))    # its screen, facing back
    g.add(m, lg.tube(45.4, 45.6, 0.5, v=v0 + 2.8, w=2.6, segments=14, mat=lg.LENS))

    m = S('Tracker')                                           # a wide glass screen in cyan-cornered frame
    g.add(m, lg.box(37.0, 47.0, v0, v0 + 1.2, 3.4, bevel=0.25, mat=lg.BLACK))
    hood = lg.slab([(40.0, v0 + 1.1), (47.0, v0 + 1.1), (47.0, v0 + 6.2), (45.6, v0 + 7.0), (41.4, v0 + 7.0),
                    (40.0, v0 + 6.2)], 4.4, bevel=0.35, segments=3, mat=lg.BLACK, round=0.4)
    g.add(m, lg.cut(hood, [lg.cbox(39.0, 48.0, v0 + 1.8, v0 + 6.3, -1.7, 1.7)]))
    g.add(m, lg.box(46.0, 46.15, v0 + 1.8, v0 + 6.3, 3.4, bevel=0.0, mat=lg.LENS))
    for v in (v0 + 1.85, v0 + 6.1):                            # cyan corner brackets on the glass
        for w in (-1.45, 1.45):
            g.add(m, lg.box(45.85, 45.95, v - 0.05, v + 0.05, 0.6, w=w * 0.88, bevel=0.0, mat=OPTIC))
    g.add(m, lg.box(37.4, 39.6, v0 + 1.2, v0 + 2.6, 2.4, bevel=0.2, mat=lg.BLACK))
    g.add(m, lg.box(37.6, 39.4, v0 + 2.6, v0 + 2.75, 1.6, bevel=0.0, mat=OPTIC))
    g.add(m, lg.pin(45.8, v0 + 4.05, -0.13, 0.13, 0.13, mat=RETICLE))
    g.socket(m, 'Aim', (45.8, v0 + 4.05))   # the dot

    m = S('Longeye')                                           # a 3x long optic: brass rings, a sunshade, ember line
    optic(m, 34.0, 17.0, v0 + 3.3, 1.75, 1.35, 2.1, rings=lg.BRASS, ember=True, dot=False)
    g.add(m, hollow([(0.0, 2.22), (3.2, 2.22)], 50.8, v0 + 3.3, 24, SHELL, wall=0.15))   # the sunshade
    g.add(m, ring(53.8, v0 + 3.3, 2.3, 0.4, OPTIC, wall=0.25))
    g.add(m, ring(34.0, v0 + 3.3, 1.85, 0.7, lg.BRASS, wall=0.2))
    g.add(m, lg.upright(42.5, v0 + 3.3 + 1.25, v0 + 3.3 + 2.4, 0.8, mat=lg.BLACK, segments=16))
    g.add(m, lg.upright(42.5, v0 + 3.3 + 2.4, v0 + 3.3 + 2.65, 0.6, mat=lg.BRASS, segments=16))


# --- Shoulder rests: under the tube, from where they hang ---

def rest(m, mat, outline=None, width=5.0, round=1.0, cuts=()):
    outline = outline or [(17.0, -2.8), (37.6, -2.8), (36.4, -7.0), (34.6, -7.6), (19.6, -7.6), (18.2, -7.0)]
    obj = lg.slab(outline, width, bevel=0.9, segments=3, mat=mat, round=round)
    g.add(m, lg.cut(obj, list(cuts)) if cuts else obj)


def rubber_strip(m, u0=19.6, u1=34.6, v=-7.6, width=5.2, ribs=False):
    obj = lg.slab([(u0, v + 0.3), (u1, v + 0.3), (u1 - 0.4, v - 0.7), (u0 + 0.4, v - 0.7)], width, bevel=0.3,
                  mat=lg.RUBBER, round=0.4)
    if ribs:
        obj = lg.cut(obj, [lg.cbox(u, u + 0.5, v - 1.2, v - 0.3, -3.0, 3.0) for u in range(int(u0) + 2, int(u1) - 1, 2)])
    g.add(m, obj)


def stocks():
    S = lambda key: model('Stock', key, STOCK_AT)

    m = S('Pad')                                               # the Powderkeg's: a sand rest, a rubber strip
    rest(m, SHELL)
    rubber_strip(m)

    m = S('Rubber')                                            # a thick ribbed rubber rest
    rest(m, lg.POLY_GREY)
    rubber_strip(m, v=-7.4, width=5.6, ribs=True)

    m = S('Skeleton')                                          # an open gunmetal frame
    rest(m, lg.BLACK, width=4.0, round=0.8,
         cuts=[lg.cslab([(20.6, -4.4), (34.4, -4.4), (33.6, -6.2), (21.2, -6.2)], -3.0, 3.0, round=0.6)])
    rubber_strip(m, width=4.4)

    m = S('Brace')                                             # a rest with a hook behind it for the shoulder
    rest(m, SHELL)
    rubber_strip(m)
    g.add(m, lg.slab([(17.4, -3.2), (19.6, -3.2), (19.4, -7.0), (17.6, -11.4), (14.6, -12.6), (12.4, -12.0), (12.6, -11.0),
                      (15.4, -10.4), (16.8, -7.4)], 3.0, bevel=0.6, segments=3, mat=lg.POLY_GREY, round=0.8))

    m = S('Quilted')                                           # padded leather, quilted
    rest(m, LEATHER, width=5.4, round=1.4)
    grooves = []
    for side in ((2.45, 4.0), (-4.0, -2.45)):                  # stitched quilting on both sides
        grooves += [lg.cbox(u, u + 0.3, -7.0, -3.2, *side) for u in (22.0, 25.4, 28.8, 32.2)]
        grooves += [lg.cbox(18.0, 37.0, v, v + 0.3, *side) for v in (-4.6, -6.0)]
    g.parts[m][-1] = lg.cut(g.parts[m][-1], grooves)
    for u in (18.4, 36.0):
        g.add(m, lg.box(u - 0.4, u + 0.4, -7.2, -3.0, 5.6, bevel=0.15, mat=lg.BRASS))

    m = S('Monopod')                                           # a rest with a fold-down leg at its front
    rest(m, SHELL)
    rubber_strip(m)
    g.add(m, lg.box(35.0, 38.4, -8.4, -6.6, 2.4, bevel=0.3, mat=lg.BLACK))
    g.add(m, lg.upright(36.7, -21.0, -8.2, 0.5, mat=lg.STEEL))
    g.add(m, lg.upright(36.7, -15.0, -8.4, 0.68, mat=lg.BLACK))
    g.add(m, lg.upright(36.7, -21.8, -20.8, 1.1, mat=lg.RUBBER, segments=16))
    g.add(m, lg.pin(36.7, -12.0, -1.3, -0.68, 0.45, mat=lg.BRASS))

    m = S('Brass')                                             # sand in a brass frame, engraved, an ember line
    rest(m, SHELL)
    rubber_strip(m)
    for side in (-1.0, 1.0):
        w = side * 2.55
        g.add(m, lg.slab([(18.8, -3.4), (35.8, -3.4), (34.8, -6.9), (19.8, -6.9)], 0.12, w=w, bevel=0.03, mat=lg.BRASS,
                         round=0.8))
        g.add(m, lg.slab([(19.4, -3.8), (35.2, -3.8), (34.4, -6.5), (20.2, -6.5)], 0.12, w=w + side * 0.05, bevel=0.0,
                         mat=SHELL, round=0.8))
        for k, u in enumerate((22.6, 27.2, 31.8)):
            scroll(m, u, -5.1, w + side * 0.13, 0.95, flip=side if k % 2 == 0 else -side)
        g.add(m, lg.box(20.0, 34.6, -7.05, -6.95, 0.08, w=side * 2.53, bevel=0.0, mat=EMBER))


# --- The parts by slot (names, rarities and stats live in Powderkeg.parts.csv) ---

SLOTS = {
    'Body': ['Standard', 'Carbon', 'Salvager', 'Armored', 'Ribbed', 'Lattice', 'Gilded'],
    'Barrel': ['Bell', 'Stub', 'Long', 'Flared', 'Vented', 'Guided', 'Siege'],
    'Muzzle': ['Ring', 'Lip', 'Crown', 'Deflector', 'Baffle', 'Coil', 'Gilded'],
    'Magazine': ['Frag', 'Slammer', 'Cluster', 'Hornet', 'Twin', 'Incendiary', 'Thunder'],
    'Sight': ['Ladder', 'Optic', 'Reflex', 'Rangefinder', 'Tracker', 'Longeye'],
    'Stock': ['Pad', 'Rubber', 'Skeleton', 'Brace', 'Quilted', 'Monopod', 'Brass'],
}
DEFAULT = {'Body': 'Standard', 'Barrel': 'Bell', 'Muzzle': 'Ring', 'Magazine': 'Frag', 'Sight': 'Optic', 'Stock': 'Pad'}
SHEETS = {'Body': (1.0, 0.3), 'Barrel': (0.35, 0.16), 'Muzzle': (0.18, 0.16), 'Magazine': (0.2, 0.12),
          'Sight': (0.25, 0.1), 'Stock': (0.3, 0.22)}   # previews' grid (m): column step, row step


def uv_unmapped():
    """Box-maps the glow, lens and reticle parts too (the kit maps only textured materials), so every face has UV
    area and Unreal's tangent build has nothing to warn about (they don't read their UVs)."""
    for parts in g.parts.values():
        for obj in parts:
            mat = obj.data.materials[0] if obj.data.materials else None
            if mat is not None and mat.name not in lg.SET_OF and obj.data.polygons:
                lt.box_uv(obj, 'MetalWorn', texel_density=lg.DENSITY)


bodies()
barrels()
muzzles()
rockets()
sights()
stocks()
uv_unmapped()
built = g.build(origins)
ROCKET = built.pop('Rocket')        # PowderkegRocket: not a part, the flying rocket
ROCKET['Collision'] = 'None'         # the game sweeps it itself


def socket_at(obj, name):
    s = next(c for c in obj.children if c.name == 'SOCKET_' + name or c.name.startswith('SOCKET_' + name + '.'))
    return obj.location + s.location


def assemble(choice, at=Vector()):
    """Puts the chosen body at `at` and the chosen parts on its sockets; returns the objects."""
    body = built['Body_' + choice['Body']]
    body.location = Vector(at)
    shown = [body]
    barrel = built['Barrel_' + choice['Barrel']]
    barrel.location = socket_at(body, 'Barrel')
    muzzle = built['Muzzle_' + choice['Muzzle']]
    muzzle.location = socket_at(barrel, 'Muzzle')
    rocket = built['Magazine_' + choice['Magazine']]
    rocket.location = socket_at(barrel, 'Rocket')
    shown += [barrel, muzzle, rocket]
    for slot in ('Sight', 'Stock'):
        obj = built[f'{slot}_{choice[slot]}']
        obj.location = socket_at(body, slot)
        shown.append(obj)
    for obj in shown:
        obj.rotation_euler = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    return shown


def layout():
    """In the scene: the default launcher assembled, every other part laid out in rows above it by slot, the flying
    rocket beside them."""
    assemble(DEFAULT)
    for row, (slot, keys) in enumerate(SLOTS.items()):
        for k, key in enumerate(keys):
            if key != DEFAULT[slot]:
                built[f'{slot}_{key}'].location = Vector((0.0, -k * 0.9, (row + 1) * 0.5))
    ROCKET.location = Vector((0.0, 0.3, 3.5))


layout()


def ads_view(objs, sight, out_png, lens=45.0, eye_relief=0.22):
    """Renders what aiming down the sights shows: a camera 22 cm behind the sight's SOCKET_Aim, looking along +u."""
    scene = bpy.context.scene
    eye = socket_at(sight, 'Aim') + Vector((0.0, eye_relief, 0.0))   # Blender +Y is toward the back
    cam = bpy.data.objects.new('_ADS', bpy.data.cameras.new('_ADS'))
    cam.data.lens = lens
    cam.data.clip_start = 0.005
    scene.collection.objects.link(cam)
    cam.location = eye
    cam.rotation_euler = Vector((0.0, -1.0, 0.0)).to_track_quat('-Z', 'Y').to_euler()
    hidden = {o: o.hide_render for o in scene.objects}
    for o in scene.objects:
        if o.type == 'MESH' and o not in objs:
            o.hide_render = True
    saved = (scene.camera, scene.world, scene.render.filepath)
    scene.camera, scene.world = cam, lt._preview_world(scene)
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.render.resolution_x, scene.render.resolution_y = 960, 540
    scene.render.image_settings.file_format = 'PNG'
    scene.render.filepath = out_png
    scene.view_settings.view_transform = 'AgX'
    bpy.ops.render.render(write_still=True)
    scene.camera, scene.world, scene.render.filepath = saved
    for o, h in hidden.items():
        o.hide_render = h
    bpy.data.objects.remove(cam)


if lt.want_preview():
    out = lambda name: lt.preview_path('Weapons', name)
    side = dict(view=(-1.0, -0.12, 0.12), ground=False, lens=60.0, fit=0.62)
    lt.preview(assemble(DEFAULT), out('Powderkeg'), **side)
    lt.preview(assemble(DEFAULT), out('Powderkeg_ThreeQuarter'), view=(-1.0, -0.9, 0.55), ground=False, lens=50.0,
               fit=0.7)
    lt.preview(assemble(DEFAULT), out('Powderkeg_Front'), view=(-0.35, -1.0, 0.2), ground=False, lens=50.0, fit=0.62)
    lt.preview(assemble(DEFAULT), out('Powderkeg_FirstPerson'), view=(-0.22, 1.0, 0.25), ground=False, lens=35.0,
               fit=0.5)
    # The reload: the flying rocket sliding into the mouth.
    shown = assemble(DEFAULT)
    ROCKET.location = socket_at(built['Barrel_' + DEFAULT['Barrel']], 'Rocket') + Vector((0.0, -0.3, 0.0))
    built['Magazine_' + DEFAULT['Magazine']].hide_render = True
    bpy.context.view_layer.update()
    lt.preview(shown[:3] + shown[4:] + [ROCKET], out('Powderkeg_Reload'), view=(-1.0, -0.6, 0.35), ground=False,
               lens=50.0, fit=0.62)
    built['Magazine_' + DEFAULT['Magazine']].hide_render = False
    variants = {
        'Powderkeg_Gilded': {'Body': 'Gilded', 'Barrel': 'Siege', 'Muzzle': 'Gilded', 'Magazine': 'Thunder',
                             'Sight': 'Longeye', 'Stock': 'Brass'},
        'Powderkeg_Lattice': {'Body': 'Lattice', 'Barrel': 'Guided', 'Muzzle': 'Coil', 'Magazine': 'Incendiary',
                              'Sight': 'Tracker', 'Stock': 'Quilted'},
        'Powderkeg_Salvager': {'Body': 'Salvager', 'Barrel': 'Stub', 'Muzzle': 'Lip', 'Magazine': 'Slammer',
                               'Sight': 'Ladder', 'Stock': 'Rubber'},
        'Powderkeg_Ribbed': {'Body': 'Ribbed', 'Barrel': 'Vented', 'Muzzle': 'Baffle', 'Magazine': 'Twin',
                             'Sight': 'Rangefinder', 'Stock': 'Monopod'},
        'Powderkeg_Armored': {'Body': 'Armored', 'Barrel': 'Long', 'Muzzle': 'Deflector', 'Magazine': 'Cluster',
                              'Sight': 'Reflex', 'Stock': 'Brace'},
        'Powderkeg_Carbon': {'Body': 'Carbon', 'Barrel': 'Flared', 'Muzzle': 'Crown', 'Magazine': 'Hornet',
                             'Sight': 'Optic', 'Stock': 'Skeleton'},
    }
    for name, choice in variants.items():
        lt.preview(assemble(choice), out(name), **side)
    for key in SLOTS['Sight']:
        ads_view(assemble(dict(DEFAULT, Sight=key)), built['Sight_' + key], out(f'Powderkeg_ADS_{key}'))
    for slot, (w, h) in SHEETS.items():
        objs = []
        for k, key in enumerate(SLOTS[slot]):
            obj = built[f'{slot}_{key}']
            obj.location = Vector((0.0, -(k % 2) * w, -(k // 2) * h))
            obj.rotation_euler = (0.0, 0.0, 0.0)
            objs.append(obj)
        if slot == 'Magazine':
            ROCKET.location = Vector((0.0, -0.1, -4 * h))
            objs.append(ROCKET)
        bpy.context.view_layer.update()
        lt.preview(objs, out(f'Powderkeg_Parts_{slot}'), view=(-1.0, -0.3, 0.2), ground=False, lens=60.0, fit=0.8)
    layout()
