"""The Drover, the game's six-gun (2026-10-09): a swing-out revolver on pistol ammo with interchangeable parts, the third
non-legendary gun family beside the Bullpup rifle and the Ranchhand shotgun. Every revolver in the game is built from these
(DA_Revolver). Blued steel and walnut at heart, a working gun of the Reaches; rarer parts bring nickel, case colours,
engraving, ebony, bone and pearl.

How the parts fit (gun space, cm: u along the gun from the back of the frame, v up from the bore; see looter_guns):

  DroverBody_<Key>      the frame: recoil shield, top strap, the window the cylinder sits in, the front lug, hammer,
                        trigger and guard, the cylinder latch on the left, a rarity accent line low on each side
                        (GunAccentGlow). Sockets: Barrel (the frame's front, on the bore), Cylinder (the cylinder's axis at
                        its rear face), Sight (on the top strap), GripMount (under the recoil shield), Grip and Foregrip
                        (the hands; the grip parts bring their own). Every frame puts them in the same place.
  DroverBarrel_<Key>    origin at the frame's front on the bore; SOCKET_Muzzle at its tip. Each carries its front sight,
                        whose top sits on SIGHT_LINE so every iron rear sight lines up with it, and the latch the ejector
                        rod's tip seats in.
  DroverCylinder_<Key>  origin on the cylinder's axis at its rear face. It turns about that axis a chamber per shot, and
                        swings out to the gun's left about SOCKET_Crane (the crane's hinge, low on the left) to reload; it
                        carries the ejector rod and the crane with it. Its chambers' count is its capacity (the CSV's
                        Magazine): the top chamber lines up with the barrel at rest.
  DroverSight_<Key>     origin on the top strap; SOCKET_Aim on its line of sight (the rear's shoulders on SIGHT_LINE for
                        the irons, the dot or the eyepiece for the glass).
  DroverGrip_<Key>      origin at GripMount: the grip frame and its panels, with SOCKET_Grip (the shooting hand) and
                        SOCKET_Foregrip (the support hand wrapped round it, on the left). A gun's notches are cut into
                        its grip panels (Weapons/WeaponModelNotches.cpp has each grip's row).

Each option's display name, the word it can lend the gun's name, the lowest rarity it appears on and its stat ranges
(rounds for cylinders, zoom for sights) live in Drover.parts.csv beside this file, which Tools/Unreal/setup_gun_parts.py
reads into DA_Revolver. A new part needs a model here and a row there, under the same key; keys never change once guns
drop.

    blender -b --factory-startup --python Art/Models/Weapons/Drover.py -- --preview
"""
import math

import bpy
from mathutils import Matrix, Vector

import looter_guns as lg
import looter_props as lp
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xe8e8e8, 2.0)   # glows in the gun's rarity color in the game
RETICLE = lg.glow('GunReticle', 0xff3b2e, 6.0)
CASE = lg.material('GunCaseColored', 'MetalWorn', 0x9a8f80)
OLIVE = lg.material('GunOlivePaint', 'PaintWorn', 0x6f7550)
WALNUT_DARK = lg.material('GunWalnutDark', 'GunWood', 0xa48670)
NICKEL = lg.material('GunNickel', 'MetalWorn', 0xc9ccd1)
SILVER = lg.material('GunSilverEngraved', 'MetalWorn', 0xb3b6ba)
COPPER = lg.material('GunCopper', 'MetalWorn', 0xc9794a)
EBONY = lg.material('GunEbony', 'GunWood', 0x5c4a40)
STAG = lg.material('GunStag', 'Polymer', 0xb49a78)
BONE = lg.material('GunBone', 'Polymer', 0xe6dcc6)
PEARL = lg.material('GunPearl', 'Polymer', 0xece8de)

# The layout (gun space, cm).
CYL_V = -1.2                 # the cylinder's axis, under the bore by its chambers' offset
CYL_U0, CYL_U1 = 2.1, 6.45   # its rear and front faces
STRAP_TOP = 1.8              # the top strap's top
SIGHT_LINE = 2.25            # the iron sights' line: every rear's shoulders and every barrel's front blade top
HALF = 1.3                   # half the frame's width
ROD_END = CYL_U1 + 4.6       # the ejector rod's tip, where it seats in the barrel's latch
BARREL_AT = (9.2, 0.0)
CYL_AT = (CYL_U0, CYL_V)
SIGHT_AT = (3.6, STRAP_TOP)
GRIP_AT = (0.0, -3.6)
CRANE = (4.0, CYL_V - 2.4, 1.4)   # the crane's hinge (u, v, w): low on the gun's left, under the cylinder
HAND = (-1.4, -7.4)               # the shooting hand on the grip
SUPPORT = (-0.6, -8.2, 3.2)       # the support hand's palm, wrapped round the shooting hand from the left

g = lg.Gun('Drover')
origins = {}


def model(slot, key, at):
    name = f'{slot}_{key}'
    origins[name] = at
    return name


def about_bore(obj, degrees):
    """Turns a part about the bore (the u axis)."""
    obj.data.transform(Matrix.Rotation(math.radians(degrees), 4, 'Y'))
    return obj


def lowered(obj, dv):
    obj.data.transform(Matrix.Translation(lg.at(0.0, dv)))
    return obj


def bez(p0, p1, p2, n=8):
    """A quadratic curve from p0 to p2 pulled toward p1, as n + 1 points."""
    return [((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
             (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]) for t in (k / n for k in range(n + 1))]


# --- See-through sights (as the other guns': aiming puts the eye 22 cm behind SOCKET_Aim, looking along the axis) ---

WALL = 0.12


def hollow(profile, u0, v, segments, mat, wall=WALL):
    """A tube turned round an axis along u, open at both ends (a thin wall following the outline inside)."""
    outer = [(r * lg.CM, du * lg.CM) for du, r in profile]
    inner = [((r - wall) * lg.CM, du * lg.CM) for du, r in reversed(profile)]
    obj = lp.lathe(outer + inner, segments=segments, closed=True)[0]
    obj.data.transform(Matrix.Translation(lg.at(u0, v)) @ Matrix.Rotation(math.radians(90.0), 4, 'X'))
    return lg.mapped(obj, mat)


def optics(m, u_eye, u_obj, v, r_eye, r_obj, wall=WALL):
    """A scope's glass: a lens inside each end and a fine cross behind the objective, tiny over the screen's crosshair."""
    g.add(m, lg.tube(u_eye + 0.15, u_eye + 0.25, r_eye - wall - 0.03, v=v, segments=20, mat=lg.LENS))
    g.add(m, lg.tube(u_obj - 0.25, u_obj - 0.15, r_obj - wall - 0.03, v=v, segments=24, mat=lg.LENS))
    u = u_obj - 0.4
    g.add(m, lg.box(u, u + 0.02, v - 0.075, v + 0.075, 0.02, bevel=0.0, mat=RETICLE))
    g.add(m, lg.box(u, u + 0.02, v - 0.01, v + 0.01, 0.15, bevel=0.0, mat=RETICLE))


# --- Frames: all with the same sockets ---

FRAME = [(-0.4, -3.9), (2.0, -3.9), (7.3, -3.85), (8.4, -3.2), (9.0, -1.05), (9.2, -0.95), (9.2, 1.3), (8.7, 1.8),
         (1.2, 1.8), (0.5, 1.5), (-0.1, 0.6), (-0.6, -0.8), (-0.6, -3.0)]
# The cylinder's window, open through both sides, with the crane's room below the barrel's breech.
WINDOW = [(2.05, -3.4), (7.3, -3.4), (7.3, -0.95), (6.5, -0.95), (6.5, 1.08), (2.05, 1.08)]
SIDE_PLATE = [(-0.3, -3.5), (1.85, -3.5), (1.85, -0.4), (0.9, 0.55), (-0.3, 0.05)]


def frame(m, mat, width=2 * HALF, extra_cuts=()):
    obj = lg.slab(FRAME, width, bevel=0.22, mat=mat, round=0.5)
    # The window, and a channel in the front lug for the ejector rod.
    cuts = [lg.cslab(WINDOW, -3.0, 3.0, round=0.15), lg.cbox(7.2, 9.4, -1.62, -0.85, -0.42, 0.42)] + list(extra_cuts)
    g.add(m, lg.cut(obj, cuts))


def side_plate(m, mat, side=-1.0):
    """The raised side plate over the lockwork (on the right, as on most swing-out guns)."""
    g.add(m, lg.slab(SIDE_PLATE, 0.1, w=side * (HALF + 0.02), bevel=0.04, mat=mat, round=0.3))


def hammer(m, mat=lg.STEEL):
    """At rest, leaning on the frame, its spur raked back for the thumb."""
    g.add(m, lg.slab([(-0.55, -1.3), (0.45, -1.3), (0.6, 0.6), (0.45, 1.5), (0.0, 2.0), (-0.8, 2.15), (-1.6, 2.35),
                      (-2.3, 2.55), (-2.6, 2.35), (-2.4, 2.05), (-1.6, 1.65), (-1.0, 1.2), (-0.8, 0.3)], 0.85, bevel=0.12,
                     mat=mat, round=0.3))


def trigger_group(m, guard=lg.BLUED, trigger=lg.STEEL):
    bow = bez((2.5, -3.75), (2.55, -6.9), (4.6, -6.85)) + bez((4.6, -6.85), (7.0, -6.8), (7.1, -3.8))[1:]
    g.add(m, lg.strip(bow, 0.95, 0.42, mat=guard))
    g.add(m, lg.slab([(3.7, -3.8), (4.6, -3.8), (4.55, -4.7), (4.2, -5.6), (3.7, -6.0), (3.9, -4.9)], 0.55, bevel=0.1,
                     mat=trigger, round=0.2))
    g.add(m, lg.pin(4.2, -3.4, -HALF - 0.04, HALF + 0.04, 0.2, mat=lg.STEEL))   # the trigger's pin


def latch(m, mat=lg.STEEL):
    """The cylinder latch under the left thumb."""
    w = HALF + 0.14
    g.add(m, lg.slab([(0.5, -1.5), (1.9, -1.5), (1.9, -0.6), (0.7, -0.6)], 0.32, w=w, bevel=0.06, mat=mat, round=0.15))
    for u in (0.85, 1.15, 1.45):
        g.add(m, lg.box(u, u + 0.08, -1.42, -0.68, 0.06, w=w + 0.17, bevel=0.0, mat=lg.BLACK))


def screw(m, u, v, side, mat=lg.STEEL, r=0.2):
    w0, w1 = (HALF - 0.05, HALF + 0.08) if side > 0 else (-HALF - 0.08, -HALF + 0.05)
    g.add(m, lg.pin(u, v, w0, w1, r, mat=mat, segments=10))


def screws(m, mat=lg.STEEL):
    for u, v in ((0.4, -2.9), (1.4, -1.0), (0.0, -0.6)):
        screw(m, u, v, -1.0, mat)
    screw(m, 0.0, -0.6, 1.0, mat)   # the hammer's pivot shows on both sides
    screw(m, 8.1, -2.2, 1.0, mat)   # the crane's lock


def accent(m, u0=2.4, u1=7.0, v=-3.62, half=HALF):
    for side in (-1.0, 1.0):
        g.add(m, lg.box(u0, u1, v - 0.09, v + 0.09, 0.08, w=side * (half + 0.01), bevel=0.0, mat=ACCENT))


def frame_sockets(m):
    for name, uv in (('Barrel', BARREL_AT), ('Cylinder', CYL_AT), ('Sight', SIGHT_AT), ('GripMount', GRIP_AT),
                     ('Grip', rk(*HAND))):
        g.socket(m, name, uv)
    g.socket(m, 'Foregrip', rk(*SUPPORT[:2]), SUPPORT[2])


def fittings(m, mat, hammer_mat=lg.STEEL, guard=lg.BLUED, trigger=lg.STEEL, screw_mat=lg.STEEL, plate=True, half=HALF):
    """What every frame has besides its body: side plate, hammer, trigger and guard, latch, screws, accent, sockets
    (half: the frame's half width, for the accent line on its sides)."""
    if plate:
        side_plate(m, mat)
    hammer(m, hammer_mat)
    trigger_group(m, guard, trigger)
    latch(m)
    screws(m, screw_mat)
    accent(m, half=half)
    frame_sockets(m)


def scroll(m, cu, cv, r0, side, turns=1.6, mat=lg.BRASS):
    """A curl of inlaid engraving on a side face."""
    w = side * (HALF + 0.02)
    pts = []
    for k in range(18):
        t = k / 17.0
        a = 2.0 * math.pi * turns * t
        r = r0 * (1.0 - 0.75 * t)
        pts.append((cu + r * math.cos(a), cv + r * math.sin(a), w))
    g.add(m, lg.pipe(pts, 0.05, mat=mat, sides=5))


def frames():
    B = lambda key: model('Body', key, (0.0, 0.0))

    m = B('Blued')                                             # the Drover's: blued steel, a plain working gun
    frame(m, lg.BLUED)
    fittings(m, lg.BLUED)

    m = B('Nickel')                                            # nickel-plated, blued hammer and trigger
    frame(m, NICKEL)
    fittings(m, NICKEL, hammer_mat=lg.BLUED, guard=NICKEL, trigger=lg.BLUED)

    m = B('Homestead')                                         # case-hardened, with a brass guard and screws
    frame(m, CASE)
    fittings(m, CASE, hammer_mat=CASE, guard=lg.BRASS, screw_mat=lg.BRASS)

    m = B('Scrapper')                                          # scrap iron, patched plates riveted on
    frame(m, lg.SALVAGE, width=2 * HALF + 0.2)
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(-0.4, -3.7), (1.9, -3.7), (1.9, -0.2), (-0.4, -0.6)], 0.25, w=side * (HALF + 0.2), bevel=0.04,
                         mat=lg.SALVAGE))
        g.add(m, lg.slab([(7.5, -3.6), (8.9, -3.2), (9.0, -1.2), (7.5, -1.2)], 0.25, w=side * (HALF + 0.2), bevel=0.04,
                         mat=lg.SALVAGE))
        for u, v in ((-0.1, -3.4), (1.6, -3.4), (1.6, -0.5), (-0.1, -0.9), (7.8, -3.3), (8.6, -1.5)):
            g.add(m, lg.rivet(u, v, side * (HALF + 0.33), side, r=0.24, mat=lg.STEEL))
        g.add(m, lg.pipe([(2.2, -3.75, side * (HALF + 0.12)), (4.6, -3.8, side * (HALF + 0.12)),
                          (7.1, -3.75, side * (HALF + 0.12))], 0.1, mat=lg.STEEL, sides=6))   # a weld bead
    fittings(m, lg.SALVAGE, guard=lg.STEEL, plate=False, half=HALF + 0.1)

    m = B('Hollow')                                            # lightened: windows through the frame, a slotted strap
    holes = [lg.cslab([(-0.3, -3.3), (1.6, -3.3), (1.6, -2.0), (-0.3, -2.3)], -3.0, 3.0, round=0.4),
             lg.cslab([(7.6, -3.1), (8.5, -2.9), (8.8, -1.4), (7.6, -1.4)], -3.0, 3.0, round=0.3),
             lg.cslab([(2.8, 1.25), (5.9, 1.25), (5.9, 1.55), (2.8, 1.55)], -3.0, 3.0, round=0.12)]
    frame(m, lg.BLACK, extra_cuts=holes)
    fittings(m, lg.BLACK, guard=lg.BLACK, plate=False)

    m = B('Ranger')                                            # matte black with painted side panels, like the Rim's
    frame(m, lg.BLACK)
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(-0.25, -3.45), (1.8, -3.45), (1.8, -1.9), (-0.25, -2.2)], 0.12, w=side * (HALF + 0.03),
                         bevel=0.03, mat=lg.POLY_SAND, round=0.25))
        g.add(m, lg.slab([(7.6, -3.3), (8.5, -3.0), (8.8, -1.6), (7.6, -1.6)], 0.12, w=side * (HALF + 0.03), bevel=0.03,
                         mat=lg.POLY_SAND, round=0.2))
    fittings(m, lg.BLACK, hammer_mat=lg.BLACK, guard=lg.BLACK, plate=False)

    m = B('Engraved')                                          # bright steel scrolled with gold
    frame(m, SILVER)
    for side in (-1.0, 1.0):
        for cu, cv, r in ((0.6, -2.4, 0.75), (8.0, -1.9, 0.5)):
            scroll(m, cu, cv, r, side)
        g.add(m, lg.pipe([(1.4, 1.48, side * (HALF + 0.02)), (5.0, 1.4, side * (HALF + 0.02)),
                          (8.5, 1.48, side * (HALF + 0.02))], 0.05, mat=lg.BRASS, sides=5))
    fittings(m, SILVER, hammer_mat=SILVER, guard=lg.BRASS, trigger=lg.BRASS, screw_mat=lg.BRASS, plate=False)

    m = B('Ironclad')                                          # thick olive armor bolted over the frame
    frame(m, lg.BLUED, width=2 * HALF + 0.2)
    for side in (-1.0, 1.0):
        w = side * (HALF + 0.35)
        g.add(m, lg.slab([(-0.5, -3.8), (1.95, -3.8), (1.95, -0.2), (0.6, 1.2), (-0.5, 0.3)], 0.5, w=w, bevel=0.12,
                         mat=OLIVE, round=0.3))
        g.add(m, lg.slab([(7.35, -3.75), (8.5, -3.2), (9.1, -1.1), (7.35, -1.1)], 0.5, w=w, bevel=0.12, mat=OLIVE,
                         round=0.25))
        for u, v in ((0.0, -3.3), (1.5, -3.3), (1.5, -0.6), (7.7, -3.2), (8.5, -1.5)):
            g.add(m, lg.rivet(u, v, side * (HALF + 0.6), side, r=0.26, mat=lg.STEEL))
    fittings(m, lg.BLUED, guard=lg.BLACK, plate=False, half=HALF + 0.1)


# --- Barrels: from the frame's front, each with its front sight and the rod's latch ---

def front_blade(m, tip, base, mat=lg.BLUED, bead=None):
    """A ramped blade whose top is on the sight line; a bead (a material) caps it."""
    top = SIGHT_LINE - (0.2 if bead else 0.0)
    g.add(m, lg.slab([(tip - 1.9, base), (tip - 0.3, base), (tip - 0.3, top), (tip - 0.9, top)], 0.34, bevel=0.06,
                     mat=mat, round=0.1))
    if bead:
        g.add(m, lg.upright(tip - 0.6, top - 0.05, SIGHT_LINE, 0.18, mat=bead, segments=10))


def rod_latch(m, mat=lg.BLUED):
    """The lug under the barrel the ejector rod's tip seats in."""
    g.add(m, lg.slab([(ROD_END - 0.05, -0.6), (ROD_END + 0.9, -0.6), (ROD_END + 0.9, -1.25), (ROD_END + 0.4, -1.7),
                      (ROD_END - 0.05, -1.7)], 0.85, bevel=0.1, mat=mat, round=0.15))


def barrel(m, length, mat=lg.BLUED, rib=True, r=0.9, segments=20, spin=0.0, blade=True, bead=None, latch_mat=None):
    """The tube with its shank collar and crowned muzzle, a top rib, the front blade and the rod's latch."""
    u0 = BARREL_AT[0]
    tip = u0 + length
    g.add(m, lg.turned([(0.0, 1.08), (0.9, 1.08), (1.1, r), (length - 0.25, r), (length, r - 0.14)], u0,
                       segments=segments, mat=mat, spin=spin))
    if rib:
        g.add(m, lg.box(u0 + 0.6, tip - 0.1, 0.5, 1.4, 0.7, bevel=0.12, mat=mat))
    if blade:
        front_blade(m, tip, 1.35 if rib else r - 0.1, mat, bead)
    rod_latch(m, latch_mat or mat)
    g.socket(m, 'Muzzle', (tip, 0.0))
    return tip


def barrels():
    B = lambda key: model('Barrel', key, BARREL_AT)
    u0 = BARREL_AT[0]

    barrel(B('Service'), 14.0)                                 # the Drover's: a ribbed 14 cm barrel

    barrel(B('Snub'), 6.0)                                     # a stubby belly gun

    m = B('Vented')                                            # a ventilated rib on posts
    tip = barrel(m, 15.0, rib=False, blade=False)
    g.add(m, lg.box(u0 + 0.6, tip - 0.1, 1.0, 1.4, 0.7, bevel=0.1, mat=lg.BLUED))
    for k in range(int((tip - u0 - 1.6) // 1.4) + 1):
        u = u0 + 0.8 + k * 1.4
        g.add(m, lg.box(u, u + 0.55, 0.5, 1.05, 0.6, bevel=0.05, mat=lg.BLUED))
    front_blade(m, tip, 1.35)

    m = B('Comped')                                            # ports cut through the top near the muzzle
    tip = u0 + 14.0
    tube = lg.turned([(0.0, 1.08), (0.9, 1.08), (1.1, 0.9), (13.75, 0.9), (14.0, 0.76)], u0, segments=20, mat=lg.BLUED)
    rib = lg.box(u0 + 0.6, tip - 0.1, 0.5, 1.4, 0.7, bevel=0.12, mat=lg.BLUED)
    ports = lambda: [lg.cbox(tip - 3.4 + k * 0.85, tip - 3.0 + k * 0.85, 0.1, 2.0, -0.28, 0.28) for k in range(3)]
    g.add(m, lg.cut(tube, ports()), lg.cut(rib, ports()))
    front_blade(m, tip - 3.25, 1.35)   # the blade sits just behind the ports
    rod_latch(m)
    g.socket(m, 'Muzzle', (tip, 0.0))

    m = B('Lugged')                                            # a full-length underlug, two cheeks round the rod
    tip = barrel(m, 15.0)
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(u0 + 0.2, -0.5), (tip - 0.3, -0.5), (tip - 0.3, -1.6), (tip - 1.3, -2.05), (u0 + 0.2, -2.05)],
                         0.42, w=side * 0.62, bevel=0.1, mat=lg.BLUED, round=0.3))

    m = B('Octagon')                                           # a long octagonal barrel and a brass bead
    tip = barrel(m, 20.0, rib=False, r=0.95, segments=8, spin=22.5, bead=lg.BRASS)

    barrel(B('Long'), 25.0)                                    # a long ribbed reach

    m = B('Gilded')                                            # brass bands, a brass crown and bead
    tip = barrel(m, 18.0, bead=lg.BRASS, latch_mat=lg.BRASS)
    for u in (u0 + 1.2, tip - 1.6):
        g.add(m, lg.tube(u, u + 0.6, 0.98, segments=20, mat=lg.BRASS))
    g.add(m, lg.turned([(0.0, 0.92), (0.35, 0.92), (0.45, 0.8)], tip - 0.45, segments=20, mat=lg.BRASS))


# --- Cylinders: modelled round the bore, then lowered onto their axis ---

def cylinder(m, chambers, radius, body=lg.BLUED, flutes=True, flute_len=2.6, chamber_r=0.52, chamber_at=1.2,
             rims=lg.BRASS, moon=False, notches=False, bands=None):
    u0, u1 = CYL_U0, CYL_U1
    length = u1 - u0
    obj = lg.turned([(0.0, radius - 0.22), (0.22, radius), (length - 0.3, radius), (length, radius - 0.3)], u0,
                    segments=36, mat=body)
    cuts = []
    angles = [360.0 * k / chambers for k in range(chambers)]
    for a in angles:
        cuts.append(about_bore(lg.tube(u1 - 0.55, u1 + 0.5, chamber_r, v=chamber_at, segments=12), a))
        if flutes:
            start = u0 + (length - flute_len) * 0.5 + 0.2
            cutter = lg.turned([(0.0, 0.0), (0.15, 0.3), (0.45, 0.44), (flute_len - 0.45, 0.44), (flute_len - 0.15, 0.3),
                                (flute_len, 0.0)], start, v=radius + 0.16, segments=12)
            cuts.append(about_bore(cutter, a + 180.0 / chambers))
        if notches:
            cuts.append(about_bore(lg.cbox(u0 + 0.6, u0 + 1.2, radius - 0.18, radius + 0.5, -0.22, 0.22), a))
    obj = lg.cut(obj, cuts)
    parts = [obj]
    for a in angles:
        # A loaded round in each chamber: its bullet's nose in the mouth, its brass rim at the back.
        parts.append(about_bore(lg.turned([(0.0, chamber_r - 0.08), (0.25, chamber_r - 0.15), (0.42, 0.0)], u1 - 0.5,
                                          v=chamber_at, segments=10, mat=COPPER), a))
        parts.append(about_bore(lg.tube(u0 - 0.12, u0 + 0.02, chamber_r + 0.1, v=chamber_at, segments=12, mat=rims), a))
    if moon:   # a steel clip holding the rims together
        parts.append(lg.turned([(0.0, chamber_at + chamber_r + 0.05), (0.08, chamber_at + chamber_r + 0.05)],
                               u0 - 0.2, segments=chambers * 4, mat=lg.STEEL))
    for at in (bands or ()):
        parts.append(lg.tube(u0 + at, u0 + at + 0.25, radius + 0.03, segments=36, mat=lg.BRASS))
    parts.append(lg.turned([(0.0, 0.5), (0.25, 0.5)], u0 - 0.25, segments=6, mat=lg.STEEL))           # the ratchet
    # The ejector rod and its knurled head, and the crane: its arm in front of the cylinder and its hinge low on the left.
    parts.append(lg.tube(u1, ROD_END - 0.9, 0.26, segments=10, mat=lg.STEEL))
    parts.append(lg.tube(ROD_END - 0.9, ROD_END, 0.36, segments=12, mat=lg.STEEL))
    for k in range(3):
        parts.append(lg.tube(ROD_END - 0.8 + k * 0.28, ROD_END - 0.7 + k * 0.28, 0.375, segments=12, mat=lg.BLACK))
    cw, cv = CRANE[2], CRANE[1] - CYL_V
    parts.append(lg.tube(u1, u1 + 0.45, 0.45, segments=12, mat=body))                               # the rod's sleeve
    parts.append(lg.pipe([(u1 + 0.42, 0.0, 0.0), (u1 + 0.42, cv * 0.55, cw * 0.55), (u1 + 0.42, cv, cw)], 0.3, mat=body))
    parts.append(lg.tube(u0 + 0.35, u1 + 0.75, 0.34, v=cv, w=cw, segments=12, mat=body))
    for p in parts:
        lowered(p, CYL_V)
        g.add(m, p)
    g.socket(m, 'Crane', (CRANE[0], CRANE[1]), CRANE[2])


def cylinders():
    C = lambda key: model('Cylinder', key, CYL_AT)
    cylinder(C('Fluted6'), 6, 2.15)                            # the Drover's: six rounds, fluted
    cylinder(C('Plain6'), 6, 2.15, flutes=False, notches=True)  # unfluted, heavier, with its stop notches showing
    cylinder(C('Bigbore5'), 5, 2.3, flute_len=2.8, chamber_r=0.62, chamber_at=1.2)
    cylinder(C('Moon6'), 6, 2.15, moon=True, rims=lg.BRASS)
    cylinder(C('Seven'), 7, 2.2, flute_len=2.4, chamber_r=0.47, chamber_at=1.2)
    cylinder(C('Eight'), 8, 2.25, flute_len=2.0, chamber_r=0.42, chamber_at=1.2, body=lg.BLUED, bands=(0.35, 3.75))


# --- Sights: on the top strap (u 1..7) ---

def sights():
    S = lambda key: model('Sight', key, SIGHT_AT)
    v0 = STRAP_TOP

    m = S('Notch')                                             # the Drover's: a fixed notch block
    block = lg.slab([(1.0, v0), (2.7, v0), (2.7, SIGHT_LINE), (1.3, SIGHT_LINE)], 1.2, bevel=0.08, mat=lg.BLUED,
                    round=0.1)
    g.add(m, lg.cut(block, [lg.cbox(0.5, 3.2, SIGHT_LINE - 0.32, SIGHT_LINE + 0.5, -0.24, 0.24)]))
    g.socket(m, 'Aim', (1.8, SIGHT_LINE))   # the rear's shoulders: the front blade's top shows in the notch

    m = S('Blade')                                             # an adjustable target rear
    g.add(m, lg.box(1.0, 4.2, v0, v0 + 0.3, 1.0, bevel=0.08, mat=lg.BLACK))
    blade = lg.slab([(1.0, v0 + 0.3), (1.9, v0 + 0.3), (1.9, SIGHT_LINE), (1.0, SIGHT_LINE)], 1.6, bevel=0.05,
                    mat=lg.BLACK, round=0.05)
    g.add(m, lg.cut(blade, [lg.cbox(0.5, 2.4, SIGHT_LINE - 0.3, SIGHT_LINE + 0.5, -0.24, 0.24)]))
    g.add(m, lg.upright(2.6, v0 + 0.3, v0 + 0.5, 0.16, mat=lg.STEEL, segments=10))     # elevation screw
    g.add(m, lg.pin(1.45, v0 + 0.4, -0.95, 0.95, 0.08, mat=lg.STEEL, segments=8))      # windage screw
    g.socket(m, 'Aim', (1.45, SIGHT_LINE))

    m = S('Peep')                                              # an aperture on a post
    g.add(m, lg.box(1.0, 3.4, v0, v0 + 0.25, 1.0, bevel=0.06, mat=lg.BLACK))
    g.add(m, lg.slab([(1.4, v0 + 0.25), (2.2, v0 + 0.25), (2.1, SIGHT_LINE - 0.42), (1.5, SIGHT_LINE - 0.42)], 0.5,
                     bevel=0.05, mat=lg.BLACK))
    g.add(m, hollow([(0.0, 0.55), (0.5, 0.55)], 1.55, SIGHT_LINE, 18, lg.BLACK, wall=0.22))
    g.socket(m, 'Aim', (1.8, SIGHT_LINE))   # the ring: the front blade's top shows in its middle

    m = S('Pip')                                               # a little open dot sight
    g.add(m, lg.box(1.0, 6.6, v0, v0 + 0.35, 1.4, bevel=0.1, mat=lg.BLACK))
    housing = lg.slab([(3.8, v0 + 0.35), (6.4, v0 + 0.35), (6.4, v0 + 2.3), (5.6, v0 + 2.9), (4.4, v0 + 2.9),
                       (3.8, v0 + 2.3)], 1.8, bevel=0.15, mat=lg.BLACK, round=0.25)
    g.add(m, lg.cut(housing, [lg.cbox(3.0, 7.2, v0 + 0.8, v0 + 2.5, -0.65, 0.65)]))
    g.add(m, lg.box(5.9, 6.0, v0 + 0.8, v0 + 2.5, 1.3, bevel=0.0, mat=lg.LENS))
    g.add(m, lg.pin(5.85, v0 + 1.65, -0.07, 0.07, 0.07, mat=RETICLE, segments=10))
    g.socket(m, 'Aim', (5.85, v0 + 1.65))   # the dot

    def scope(m, u_eye, profile, v, rings, turret):
        g.add(m, lg.box(1.0, 6.6, v0, v0 + 0.4, 1.3, bevel=0.1, mat=lg.BLACK))
        g.add(m, hollow(profile, u_eye, v, 22, lg.BLACK))
        r_tube = min(r for _, r in profile)
        for u in rings:   # rings round the tube, open in the middle like it, on posts to the base
            g.add(m, hollow([(0.0, r_tube + 0.2), (1.0, r_tube + 0.2)], u - 0.5, v, 20, lg.BLACK, wall=0.22))
            g.add(m, lg.box(u - 0.5, u + 0.5, v0 + 0.4, v - r_tube - 0.05, 1.0, bevel=0.1, mat=lg.BLACK))
        g.add(m, lg.upright(turret, v + r_tube, v + r_tube + 0.8, 0.6, mat=lg.BLACK, segments=14))
        g.add(m, lg.pin(turret, v, -r_tube - 0.8, -r_tube + 0.05, 0.6, mat=lg.BLACK, segments=14))
        u_obj = u_eye + profile[-1][0]
        optics(m, u_eye, u_obj, v, profile[0][1], profile[-1][1])
        g.socket(m, 'Aim', (u_eye + 0.2, v))   # the eyepiece lens

    scope(S('Glass'), -4.0, [(0.0, 1.15), (2.2, 1.15), (3.0, 0.85), (12.0, 0.85), (13.0, 1.15), (15.0, 1.15)],
          v0 + 2.5, (2.0, 5.6), 3.8)                           # a 2x long-eye-relief scope
    scope(S('Longeye'), -5.0, [(0.0, 1.25), (2.6, 1.25), (3.4, 0.9), (13.0, 0.9), (14.4, 1.55), (18.0, 1.55)],
          v0 + 2.8, (2.0, 5.6), 3.8)                           # a 3x scope with a wide objective


# --- Grips: the grip frame and its panels, below and behind the recoil shield ---

# Panels stop just under the frame (whose bottom is at v -3.9); the grip frame's tang and back strap rise behind the
# recoil shield to meet it. Wrap-around grips (WRAP, TARGET) cover the back strap themselves.
PLOW = [(2.15, -3.7), (1.95, -4.6), (1.4, -6.6), (0.75, -9.0), (0.35, -10.9), (0.15, -11.5), (-0.3, -12.05),
        (-1.2, -12.3), (-2.6, -12.3), (-3.7, -11.9), (-4.3, -11.1), (-4.15, -9.2), (-3.5, -6.8), (-2.6, -4.6), (-2.1, -3.7)]
PLOW_FRONT = [(2.15, -3.75), (1.95, -4.6), (1.4, -6.6), (0.75, -9.0), (0.35, -10.9), (0.15, -11.5)]
PLOW_BACK = [(-0.75, -2.2), (-1.6, -2.95), (-2.6, -4.6), (-3.5, -6.8), (-4.15, -9.2), (-4.3, -11.1)]
PLOW_BUTT = [(0.15, -11.55), (-0.3, -12.1), (-1.2, -12.35), (-2.6, -12.35), (-3.7, -11.95), (-4.3, -11.15)]
PLOW_TANG = [(-0.3, -3.95), (-2.15, -3.95), (-1.3, -2.6), (-0.75, -2.15), (-0.5, -2.3)]
WRAP = [(2.25, -3.7), (2.05, -4.5), (1.6, -5.3), (1.75, -6.1), (1.3, -6.9), (1.35, -7.8), (0.9, -8.6), (0.95, -9.5),
        (0.5, -10.4), (0.4, -11.4), (-0.4, -12.2), (-2.6, -12.3), (-4.1, -11.8), (-4.6, -10.8), (-4.4, -8.8), (-3.7, -6.5),
        (-2.7, -4.4), (-1.7, -2.7), (-0.9, -1.95), (-0.5, -2.3), (-0.45, -3.7)]
BIRD = [(2.1, -3.7), (1.9, -4.7), (1.3, -6.7), (0.5, -8.7), (-0.3, -9.9), (-1.5, -10.6), (-3.0, -10.6), (-4.2, -9.9),
        (-4.7, -8.9), (-4.5, -8.0), (-3.9, -7.5), (-3.3, -6.4), (-2.6, -4.6), (-2.1, -3.7)]
BIRD_FRONT = [(2.1, -3.75), (1.9, -4.7), (1.3, -6.7), (0.5, -8.7), (-0.3, -9.9)]
BIRD_BACK = [(-0.75, -2.2), (-1.6, -2.95), (-2.6, -4.6), (-3.3, -6.4)]
TARGET = [(2.2, -3.7), (2.0, -4.6), (1.5, -6.6), (1.05, -9.0), (0.85, -11.4), (0.6, -12.4), (-0.4, -12.8), (-3.4, -12.8),
          (-4.5, -12.2), (-4.9, -10.8), (-4.6, -8.6), (-3.8, -6.2), (-2.8, -4.3), (-1.7, -2.7), (-0.9, -2.1), (-0.5, -2.4),
          (-0.45, -3.7)]
COFFIN = [(2.15, -3.7), (1.95, -4.6), (1.3, -7.0), (0.7, -10.6), (0.5, -11.7), (-0.1, -12.1), (-2.9, -12.2), (-3.5, -11.8),
          (-4.6, -8.4), (-3.4, -5.2), (-2.46, -3.7)]
COFFIN_FRONT = [(2.15, -3.75), (1.95, -4.6), (1.3, -7.0), (0.7, -10.6), (0.52, -11.6)]
COFFIN_BACK = [(-0.75, -2.2), (-1.5, -2.65), (-2.4, -3.65), (-3.4, -5.25), (-4.55, -8.4)]
COFFIN_TANG = [(-0.3, -3.95), (-2.5, -3.95), (-1.5, -2.65), (-0.75, -2.15), (-0.5, -2.3)]
RAKE = 0.15   # the grips lean back from where they meet the frame (about 9 degrees more than drawn)


def raked(points):
    """Points below the frame shifted back by RAKE per cm down: the grip leans back from its top, which stays put. A
    point may carry a w after its (u, v)."""
    return [(p[0] + RAKE * min(p[1] - GRIP_AT[1], 0.0),) + tuple(p[1:]) for p in points]


def rk(u, v, *w):
    """One raked point."""
    return raked([(u, v) + w])[0]


PLOW, PLOW_FRONT, PLOW_BACK, PLOW_BUTT, PLOW_TANG = (raked(p) for p in (PLOW, PLOW_FRONT, PLOW_BACK, PLOW_BUTT, PLOW_TANG))
WRAP, BIRD, BIRD_FRONT, BIRD_BACK, TARGET = (raked(p) for p in (WRAP, BIRD, BIRD_FRONT, BIRD_BACK, TARGET))
COFFIN, COFFIN_FRONT, COFFIN_BACK, COFFIN_TANG = (raked(p) for p in (COFFIN, COFFIN_FRONT, COFFIN_BACK, COFFIN_TANG))


def grip(m, outline, mat, width=3.3, strap=lg.BLUED, front=PLOW_FRONT, back=PLOW_BACK, butt=None, tang=PLOW_TANG,
         round=0.7, hand=HAND, support=SUPPORT):
    """The panels (outline, width wide) and, with a strap material, the grip frame: its tang behind the recoil shield and
    its straps along the panels' edges."""
    g.add(m, lg.slab(outline, width, bevel=0.55, segments=3, mat=mat, round=round))
    if strap is not None:
        g.add(m, lg.slab(tang, 1.6, bevel=0.2, mat=strap, round=0.3))
        for edge in (front, back, butt):
            if edge:
                g.add(m, lg.strip(edge, 1.7, 0.42, mat=strap))
    g.socket(m, 'Grip', rk(*hand))
    g.socket(m, 'Foregrip', rk(*support[:2]), support[2])


def escutcheon(m, u, v, mat=lg.BRASS, r=0.42):
    """A small diamond inlay on each panel (u, v as drawn, before the rake)."""
    u, v = rk(u, v)
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(u, v + r), (u + r * 0.8, v), (u, v - r), (u - r * 0.8, v)], 0.12, w=side * 1.62, bevel=0.03,
                         mat=mat))


def grips():
    G = lambda key: model('Grip', key, GRIP_AT)

    m = G('Plowhandle')                                        # the Drover's: smooth walnut on a blued frame
    grip(m, PLOW, lg.WALNUT, butt=PLOW_BUTT)

    m = G('Wrapped')                                           # a wrap-around rubber grip with finger grooves
    grip(m, WRAP, lg.POLY_GREY, width=3.5, strap=None, round=0.5, hand=(-1.6, -7.6))

    m = G('Birdshead')                                         # a short bird's-head grip with a lanyard ring
    grip(m, BIRD, WALNUT_DARK, front=BIRD_FRONT, back=BIRD_BACK, hand=(-1.4, -6.8), support=(-0.6, -7.6, 3.2))
    ring = raked([(-3.0 + 0.55 * math.sin(2 * math.pi * k / 12), -11.2 - 0.55 * math.cos(2 * math.pi * k / 12))
                  for k in range(13)])
    g.add(m, lg.pipe(ring, 0.1, mat=lg.STEEL, sides=6))
    eye_u, eye_v = rk(-3.0, -10.6)
    g.add(m, lg.box(eye_u - 0.3, eye_u + 0.3, eye_v - 0.15, eye_v + 0.15, 0.5, bevel=0.05, mat=lg.STEEL))

    m = G('Target')                                            # a big target grip with a thumb rest on the left
    grip(m, TARGET, WALNUT_DARK, width=3.6, strap=None, round=0.6, hand=(-1.6, -7.8), support=(-0.8, -8.6, 3.3))
    g.add(m, lg.slab(raked([(-1.6, -3.4), (1.6, -3.9), (1.2, -5.0), (-1.8, -4.7)]), 0.6, w=2.0, bevel=0.2,
                     mat=WALNUT_DARK, round=0.4))
    for side in (-1.0, 1.0):   # grooves across the panels, for purchase
        for v in (-8.0, -8.7, -9.4, -10.1, -10.8):
            u = rk(0.0, v)[0]
            g.add(m, lg.box(u - 3.6, u + 0.2, v - 0.05, v + 0.05, 0.08, w=side * 1.78, bevel=0.0, mat=lg.BLACK))

    m = G('Stag')                                              # stag horn, knobbly, on a blued frame
    grip(m, PLOW, STAG, butt=PLOW_BUTT)
    knobs = ((-0.6, -5.0, 0.26), (-1.8, -5.6, 0.2), (-0.9, -6.6, 0.3), (-2.4, -7.0, 0.22), (-1.2, -8.2, 0.24),
             (-2.9, -8.6, 0.28), (-1.6, -9.6, 0.2), (-0.4, -10.2, 0.22), (-2.6, -10.6, 0.26), (-3.2, -9.7, 0.18))
    for side in (-1.0, 1.0):
        for u, v, r in knobs:
            g.add(m, lg.rivet(*rk(u, v), side * 1.6, side, r=r, mat=STAG))

    m = G('Coffin')                                            # a coffin-shaped grip in ebony, a brass diamond
    grip(m, COFFIN, EBONY, front=COFFIN_FRONT, back=COFFIN_BACK, tang=COFFIN_TANG, round=0.35)
    escutcheon(m, -0.9, -4.6)

    m = G('Bone')                                              # bone, scrimshawed with an inked vine
    grip(m, PLOW, BONE, butt=PLOW_BUTT)
    for side in (-1.0, 1.0):
        w = side * 1.66
        g.add(m, lg.pipe(raked([(-0.2, -4.6, w), (-1.0, -6.0, w), (-1.6, -7.6, w), (-2.6, -9.4, w), (-3.0, -10.8, w)]),
                         0.05, mat=lg.BLACK, sides=5))
        for u, v in ((-0.7, -5.3), (-1.4, -6.9), (-2.1, -8.5), (-2.8, -10.1)):
            g.add(m, lg.pipe(raked([(u, v, w), (u + 0.6, v - 0.2, w), (u + 0.9, v - 0.7, w)]), 0.04, mat=lg.BLACK,
                             sides=5))

    m = G('Pearl')                                             # mother-of-pearl on gilt straps
    grip(m, PLOW, PEARL, strap=lg.BRASS, butt=PLOW_BUTT)
    escutcheon(m, -0.8, -4.5, r=0.32)


# --- The design: names, rarities and stats live in Drover.parts.csv; DEFAULT is the plain Drover ---

DEFAULT = {'Body': 'Blued', 'Barrel': 'Service', 'Cylinder': 'Fluted6', 'Sight': 'Notch', 'Grip': 'Plowhandle'}
SLOTS = {
    'Body': ['Blued', 'Nickel', 'Homestead', 'Scrapper', 'Hollow', 'Ranger', 'Engraved', 'Ironclad'],
    'Barrel': ['Service', 'Snub', 'Vented', 'Comped', 'Lugged', 'Octagon', 'Long', 'Gilded'],
    'Cylinder': ['Fluted6', 'Plain6', 'Bigbore5', 'Moon6', 'Seven', 'Eight'],
    'Sight': ['Notch', 'Blade', 'Peep', 'Pip', 'Glass', 'Longeye'],
    'Grip': ['Plowhandle', 'Wrapped', 'Birdshead', 'Target', 'Stag', 'Coffin', 'Bone', 'Pearl'],
}
SOCKET_OF = {'Barrel': 'Barrel', 'Cylinder': 'Cylinder', 'Sight': 'Sight', 'Grip': 'GripMount'}

frames()
barrels()
cylinders()
sights()
grips()
built = g.build(origins)


def socket_at(obj, name):
    """Where obj's socket name is (its own name, or Blender's numbered copy of it), in the scene."""
    s = next(c for c in obj.children if c.name == 'SOCKET_' + name or c.name.startswith('SOCKET_' + name + '.'))
    return obj.location + s.location


def assemble(choice, at=Vector()):
    """Puts the chosen frame at `at` and the chosen parts on its sockets; returns the objects."""
    body = built['Body_' + choice['Body']]
    body.location = Vector(at)
    body.rotation_euler = (0.0, 0.0, 0.0)
    shown = [body]
    for slot, socket in SOCKET_OF.items():
        obj = built[f'{slot}_{choice[slot]}']
        obj.location = socket_at(body, socket)
        obj.rotation_euler = (0.0, 0.0, 0.0)
        shown.append(obj)
    bpy.context.view_layer.update()   # so the preview frames the parts where they are now
    return shown


def swing_out(cylinder_obj, body, degrees=100.0):
    """Swings a placed cylinder out to the gun's left about its crane, as the reload does in the game."""
    crane = socket_at(cylinder_obj, 'Crane')
    turn = Matrix.Translation(crane) @ Matrix.Rotation(math.radians(degrees), 4, 'Y') @ Matrix.Translation(-crane)
    cylinder_obj.matrix_world = turn @ cylinder_obj.matrix_world
    bpy.context.view_layer.update()


# In the scene: the plain Drover assembled, every other part laid out in rows behind it by slot.
assemble(DEFAULT)
for row, (slot, keys) in enumerate(SLOTS.items()):
    for k, key in enumerate(keys):
        if key != DEFAULT[slot]:
            built[f'{slot}_{key}'].location = Vector((0.0, -k * 0.35, (row + 1) * 0.3))

if lt.want_preview():
    out = lambda name: lt.preview_path('Weapons', name)
    lt.preview(assemble(DEFAULT), out('Drover'), view=(-1.0, -0.25, 0.25), ground=False, lens=60.0, fit=0.95)
    shown = assemble(DEFAULT)
    swing_out(built['Cylinder_' + DEFAULT['Cylinder']], shown[0])
    lt.preview(shown, out('Drover_Reload'), view=(1.0, -0.6, 0.45), ground=False, lens=60.0, fit=0.95)
    variants = {
        'Drover_Gilded': {'Body': 'Engraved', 'Barrel': 'Long', 'Cylinder': 'Seven', 'Sight': 'Glass', 'Grip': 'Pearl'},
        'Drover_Snub': {'Body': 'Nickel', 'Barrel': 'Snub', 'Cylinder': 'Bigbore5', 'Sight': 'Notch', 'Grip': 'Birdshead'},
        'Drover_Ironclad': {'Body': 'Ironclad', 'Barrel': 'Lugged', 'Cylinder': 'Eight', 'Sight': 'Longeye',
                            'Grip': 'Coffin'},
        'Drover_Ranger': {'Body': 'Ranger', 'Barrel': 'Comped', 'Cylinder': 'Moon6', 'Sight': 'Pip', 'Grip': 'Wrapped'},
        'Drover_Hollow': {'Body': 'Hollow', 'Barrel': 'Vented', 'Cylinder': 'Plain6', 'Sight': 'Peep', 'Grip': 'Target'},
        'Drover_Homestead': {'Body': 'Homestead', 'Barrel': 'Octagon', 'Cylinder': 'Fluted6', 'Sight': 'Blade',
                             'Grip': 'Stag'},
        'Drover_Scrapper': {'Body': 'Scrapper', 'Barrel': 'Gilded', 'Cylinder': 'Fluted6', 'Sight': 'Notch',
                            'Grip': 'Bone'},
    }
    for name, choice in variants.items():
        lt.preview(assemble(choice), out(name), view=(-1.0, -0.25, 0.25), ground=False, lens=60.0, fit=0.95)
    assemble(DEFAULT)
