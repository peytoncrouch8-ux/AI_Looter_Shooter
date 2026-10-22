"""The Longhorn bolt-action sniper rifle (Phase 12.3, concept B "frontier-tech": sand polymer, gunmetal, brass, ember
glow, cyan optics), with interchangeable parts: 8 bodies, 8 barrels, 8 muzzle devices, 8 magazines, 8 scopes, 8 stocks
and the bolt. Every sniper rifle in the game is built from them (EWeaponKind::Sniper, its definition made by the C++
work that follows). Long and lean: a thumbhole sand chassis, a fluted free-floated barrel, a long scope with a cyan
objective, a folded bipod, the bolt's brass knob on the right.

How the parts fit (gun space, cm: u along the gun from the back of the chassis, v up from the bore; see looter_guns):

  LonghornBody_<Key>      the chassis: butt, thumbhole wrist and pistol grip, the round receiver bedded in it (ejection
                          port and bolt notch on the right), the scope rail, trigger and guard, the fore-end with a folded
                          bipod under its tip and a rarity accent line on each side (GunAccentGlow). The butt belongs to
                          the body so it always matches the chassis (walnut, olive, rust...). Sockets: Stock (the middle
                          of the butt's back face, u -15.2), Barrel (the receiver's front), Magazine (the well ahead of
                          the guard), Sight (the rail's middle), Bolt (the bolt's axis behind the receiver), Eject (the
                          ejection port), Grip, Foregrip; the same on every body, so every part fits every body. u 0 is
                          the comb's front, where the wrist dips (the drawn bolt clears it).
  LonghornBolt_Standard   the bolt, the same on every gun: its body in the receiver, the shroud behind it and the handle
                          with its brass knob on the right. Its origin is on the bore axis at the shroud's back: the
                          cycle turns it up about its own X (the handle lifts, about 60 degrees) and slides it back along
                          -X (8-9 cm: the shroud then rides over the dipped wrist), so it is the reload's and every
                          shot's moving part.
  LonghornBarrel_<Key>    origin at the receiver's front; SOCKET_Muzzle at its tip (barrels differ in length).
  LonghornMuzzle_<Key>    origin at its back face (screws onto a barrel's SOCKET_Muzzle); SOCKET_Muzzle at its tip.
  LonghornMagazine_<Key>  a small box, origin where it seats under the chassis; slides out along -Z.
  LonghornSight_<Key>     a scope (4x to 8x: the CSV's Zoom), origin on top of the rail at its middle; SOCKET_Aim at the
                          eyepiece lens's center, on the scope's axis (its line of sight). Every scope is an open tube:
                          aiming down sights looks through it at its fine cross.
  LonghornStock_<Key>     what fits on the butt's back face (pads, spacers, a cheek riser over the comb, a toe hook, an
                          adjustable plate, a monopod, an engraved brass plate), origin at the middle of that face;
                          extends backward (and, for cheek risers, forward over the comb). Pads lap 0.3 cm over the face.

Each option's display name, name word, lowest rarity and stat ranges live in Longhorn.parts.csv beside this file
(Magazine = rounds, Zoom = magnification), which Tools/Unreal/setup_gun_parts.py reads into the sniper's definition.
A new part needs a model here and a row there, under the same key; never rename or reuse a key.

    blender -b --factory-startup --python Art/Models/Weapons/Longhorn.py -- --preview
"""
import math

import bpy
from mathutils import Matrix, Vector

import looter_guns as lg
import looter_props as lp
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xe8e8e8, 2.0)   # glows in the gun's rarity color in the game
RETICLE = lg.RETICLE                                 # the kit's frontier-tech glows, shared by the four new guns
EMBER = lg.EMBER                                     # GunEmberGlow: fixed ember lines and vents
OPTIC = lg.CYAN                                      # GunCyanGlow: cyan optics
SHELL = lg.POLY_SAND
WALNUT_DARK = lg.material('GunWalnutDark', 'GunWood', 0xa48670)
OLIVE = lg.material('GunOlivePaint', 'PaintWorn', 0x6f7550)

# Where the parts attach (gun space, cm).
STOCK_AT = (-15.2, -3.9)
BARREL_AT = (41.5, 0.0)
MAG_AT = (35.6, -4.9)
SIGHT_AT = (29.5, 2.55)
BOLT_AT = (13.4, 0.0)
MUZZLE_AT = (110.0, 0.0)   # muzzle devices are modeled here, around their own origin
GRIP = (19.8, -9.0)
FOREGRIP = (52.0, -5.2)
EJECT = (28.4, 0.7, -1.8)

g = lg.Gun('Longhorn')
origins = {}


def model(slot, key, at):
    name = f'{slot}_{key}'
    origins[name] = at
    return name


# --- Shared pieces ---

WALL = 0.12   # cm: thin tube walls keep a scope's clear opening as wide as the tube


def hollow(profile, u0, v, segments, mat, wall=WALL, w=0.0, spin=0.0):
    """A tube turned round an axis along u (profile [(du, r), ...]) but open at both ends: a thin wall that follows
    the outline inside, so the eye looks down the tube instead of at an end cap."""
    outer = [(r * lg.CM, du * lg.CM) for du, r in profile]
    inner = [((r - wall) * lg.CM, du * lg.CM) for du, r in reversed(profile)]
    obj = lp.lathe(outer + inner, segments=segments, closed=True)[0]
    obj.data.transform(Matrix.Translation(lg.at(u0, v, w)) @ Matrix.Rotation(math.radians(90.0), 4, 'X')
                       @ Matrix.Rotation(math.radians(spin), 4, 'Z'))
    return lg.mapped(obj, mat)


def ring(u, v, r, length, mat, wall=0.25, segments=24):
    return hollow([(0.0, r), (length, r)], u, v, segments, mat, wall=wall)


def optics(m, u_eye, u_obj, v, r_eye, r_obj, wall=WALL):
    """A scope's glass: a lens just inside the eyepiece and one inside the objective, and a fine cross just behind the
    objective lens, tiny because it sits over the screen's crosshair."""
    g.add(m, lg.tube(u_eye + 0.15, u_eye + 0.25, r_eye - wall - 0.03, v=v, segments=20, mat=lg.LENS))
    g.add(m, lg.tube(u_obj - 0.25, u_obj - 0.15, r_obj - wall - 0.03, v=v, segments=24, mat=lg.LENS))
    u = u_obj - 0.4
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
    g.add(m, lg.pipe(pts, 0.06, mat=mat, sides=5))


def side_strip(m, u0, u1, v, half, mat, height=0.24, thick=0.08):
    """A flat line on both sides of a part (accent and ember lines)."""
    for side in (-1.0, 1.0):
        g.add(m, lg.box(u0, u1, v - height * 0.5, v + height * 0.5, thick, w=side * half, bevel=0.0, mat=mat))


# --- Bodies: the chassis and its fittings, all with the same sockets ---

# The thumbhole chassis's side outline: the butt and its straight comb, the wrist dipping behind the action (so the
# drawn bolt clears it), the receiver bedded at mid-bore, the fore-end under the free-floated barrel, the raked pistol
# grip, and the straight line from its foot back to the butt's toe. The butt is the body's, so it always matches the
# chassis; the stocks are what fits on its back face (pads, risers, plates, a monopod).
BUTT_U = -15.2   # the butt's back face
CHASSIS = [(BUTT_U + 0.6, 2.0), (1.0, 2.25), (3.4, 1.6), (5.6, -1.0), (7.0, -1.55), (16.2, -1.55), (17.5, -0.3),
           (42.5, -0.3), (44.5, -1.3), (60.0, -1.35), (61.6, -2.2), (61.6, -3.9), (60.0, -4.8), (40.0, -4.9),
           (31.0, -4.7), (25.6, -4.7), (24.4, -5.6), (22.4, -10.8), (21.6, -12.6), (20.2, -13.4), (15.6, -13.4),
           (13.4, -12.8), (BUTT_U + 0.8, -10.0), (BUTT_U, -9.2), (BUTT_U, 1.4)]
HOLE = [(7.0, -3.5), (12.8, -3.3), (15.8, -3.9), (17.2, -5.5), (16.4, -10.2), (14.8, -11.0), (12.6, -10.6),
        (9.0, -9.4), (6.4, -7.0), (5.8, -4.8)]
GRIP_PANEL = [(23.5, -6.2), (21.8, -10.8), (21.0, -12.0), (18.0, -12.0), (17.6, -10.4), (18.4, -6.2)]
HALF = 1.9   # half the chassis's width


def offset(outline, d):
    """The outline grown outward by d cm (a skin laid a hair proud over a part of the same shape)."""
    pts = [Vector(p) for p in outline]
    area = sum(a.x * b.y - b.x * a.y for a, b in zip(pts, pts[1:] + pts[:1]))
    s = 1.0 if area > 0 else -1.0
    out = []
    for i, p1 in enumerate(pts):
        p0, p2 = pts[i - 1], pts[(i + 1) % len(pts)]
        e0, e1 = (p1 - p0).normalized(), (p2 - p1).normalized()
        n0, n1 = Vector((e0.y, -e0.x)) * s, Vector((e1.y, -e1.x)) * s
        bis = n0 + n1
        bis = bis.normalized() if bis.length > 1e-6 else n0
        k = d / max(bis.dot(n0), 0.3)
        out.append((p1.x + bis.x * k, p1.y + bis.y * k))
    return out


def chassis(m, mat, outline=CHASSIS, width=2 * HALF, round=1.0, bevel=0.9, extra_cuts=()):
    obj = lg.slab(outline, width, bevel=bevel, segments=3, mat=mat, round=round)
    g.add(m, lg.cut(obj, [lg.cslab(HOLE, -3.0, 3.0, round=1.4)] + list(extra_cuts)))


def receiver(m, mat, r=1.65):
    """The round action bedded in the chassis: the ejection port and the bolt handle's notch on the right."""
    tube = lg.turned([(0.0, r - 0.35), (0.5, r), (24.0, r), (24.0, r - 0.1)], 17.5, segments=24, mat=mat)
    cuts = [lg.cbox(25.0, 31.6, -0.2, 1.4, -2.2, -0.75),     # the ejection port
            lg.cbox(17.2, 19.6, -1.2, 0.5, -2.2, -0.9)]      # the handle's notch at the back
    g.add(m, lg.cut(tube, cuts))
    g.add(m, lg.turned([(0.0, 1.3), (1.2, 1.3)], 40.5, segments=24, mat=lg.BLACK))   # the barrel nut


def grip_panels(m, mat, outline=GRIP_PANEL):
    for side in (-1.0, 1.0):
        g.add(m, lg.slab(outline, 0.3, w=side * (HALF + 0.05), bevel=0.08, mat=mat, round=0.6))


def fittings(m, rail=lg.BLACK, guard=lg.BLACK, trigger=lg.STEEL, bipod=True, bipod_mat=lg.BLACK, screws=lg.BLACK,
             accent=(44.5, 58.5, -3.0)):
    """What every body has: the scope base and rail, trigger and guard, action screws, the folded bipod, the accent
    line and the sockets."""
    g.add(m, lg.box(19.5, 39.5, 1.0, 1.75, 1.9, bevel=0.15, mat=rail))
    g.add(m, lg.rail(19.5, 39.5, 1.6, width=1.8, mat=rail))
    g.add(m, lg.strip([(25.2, -4.7), (25.5, -7.4), (27.6, -8.4), (30.6, -7.8), (31.6, -4.8)], 0.9, 0.32, mat=guard))
    g.add(m, lg.slab([(27.6, -4.6), (28.5, -4.6), (28.3, -5.8), (27.9, -6.8), (27.3, -7.0), (27.6, -5.8)], 0.55,
                     bevel=0.12, mat=trigger))
    for u in (21.0, 39.0):
        for side in (-1.0, 1.0):
            g.add(m, lg.rivet(u, -2.2, side * (HALF + 0.02), side, r=0.3, mat=screws))
    if bipod:
        g.add(m, lg.box(55.4, 58.8, -6.1, -4.6, 2.6, bevel=0.3, mat=bipod_mat))       # the bipod's hinge block
        g.add(m, lg.pin(57.1, -5.5, -1.45, 1.45, 0.45, mat=lg.STEEL))
        for side in (-1.0, 1.0):
            g.add(m, lg.pipe([(56.6, -5.7, side * 0.85), (44.0, -5.75, side * 0.85)], 0.34, mat=bipod_mat, sides=8))
            g.add(m, lg.pipe([(50.0, -5.75, side * 0.85), (43.2, -5.8, side * 0.85)], 0.27, mat=lg.STEEL, sides=8))
            g.add(m, lg.box(41.6, 43.6, -6.25, -5.25, 1.0, w=side * 0.85, bevel=0.25, mat=lg.RUBBER))
    g.add(m, lg.pipe([(-11.2, -10.0), (-11.1, -11.1), (-9.9, -11.1), (-9.8, -10.0)], 0.2, mat=lg.STEEL))   # sling stud
    if accent:
        u0, u1, v = accent
        side_strip(m, u0, u1, v, HALF + 0.02, ACCENT)
    for name, uv in (('Stock', STOCK_AT), ('Barrel', BARREL_AT), ('Magazine', MAG_AT), ('Sight', SIGHT_AT),
                     ('Bolt', BOLT_AT), ('Grip', GRIP), ('Foregrip', FOREGRIP)):
        g.socket(m, name, uv)
    g.socket(m, 'Eject', EJECT[:2], EJECT[2])


def bodies():
    B = lambda key: model('Body', key, (0.0, 0.0))

    m = B('Standard')                                          # sand chassis, gunmetal action, grey grip panels
    chassis(m, SHELL)
    receiver(m, lg.BLUED)
    grip_panels(m, lg.POLY_GREY)
    fittings(m)

    m = B('Heritage')                                          # oiled walnut, blued action, brass furniture
    chassis(m, WALNUT_DARK)
    receiver(m, lg.BLUED)
    g.add(m, lg.slab(offset([(59.4, -1.3), (60.0, -1.35), (61.6, -2.2), (61.6, -3.9), (60.0, -4.8), (59.4, -4.8)], 0.06),
                     2 * HALF + 0.12, bevel=0.3, mat=lg.BRASS, round=0.4))   # the fore-end cap
    g.add(m, lg.slab([(20.4, -13.0), (21.4, -12.4), (20.6, -13.6), (16.0, -13.7), (15.6, -13.2)], 2 * HALF - 0.2,
                     bevel=0.2, mat=lg.BRASS, round=0.3))               # the grip cap
    for side in (-1.0, 1.0):                                   # checkering on the grip
        for k in range(5):
            u = 18.6 + k * 0.9
            g.add(m, lg.slab([(u, -6.6), (u + 0.5, -6.6), (u - 0.9, -11.2), (u - 1.4, -11.2)], 0.12,
                             w=side * (HALF + 0.04), bevel=0.0, mat=lg.BLUED))
        w0, w1 = (HALF - 0.1, HALF + 0.08) if side > 0 else (-HALF - 0.08, -HALF + 0.1)
        g.add(m, lg.pin(-6.0, -3.2, w0, w1, 0.7, segments=16, mat=lg.BRASS))   # a brass escutcheon on the butt
    fittings(m, rail=lg.BLUED, guard=lg.BRASS, screws=lg.BRASS, bipod_mat=lg.BLUED)

    m = B('Carbon')                                            # black and lean, tan panels
    chassis(m, lg.POLY_BLACK, round=0.4, bevel=0.5)
    receiver(m, lg.BLACK)
    for side in (-1.0, 1.0):
        g.add(m, lg.slab([(45.0, -2.15), (58.8, -2.2), (60.2, -2.6), (60.2, -4.0), (45.0, -4.2)], 0.3,
                         w=side * (HALF + 0.05), bevel=0.06, mat=lg.POLY_TAN, round=0.3))
        g.add(m, lg.slab([(-13.6, 0.6), (-2.0, 0.9), (-2.0, -5.4), (-13.6, -6.6)], 0.3, w=side * (HALF + 0.05),
                         bevel=0.06, mat=lg.POLY_TAN, round=0.8))   # a tan panel on the butt
    grip_panels(m, lg.POLY_TAN)
    fittings(m, accent=(45.6, 57.8, -4.55))

    m = B('Salvager')                                          # rusty plates riveted on, a taped grip
    chassis(m, lg.SALVAGE, round=0.4, bevel=0.3)
    receiver(m, lg.STEEL)
    for side in (-1.0, 1.0):
        w = side * (HALF + 0.12)
        for u0, u1, v0, v1 in ((-13.4, -1.0, -6.6, 1.0), (44.0, 60.0, -4.3, -1.6)):
            g.add(m, lg.slab([(u0, v0), (u1, v0), (u1, v1), (u0, v1)], 0.24, w=w, bevel=0.04, mat=lg.SALVAGE))
            for u in (u0 + 0.7, u1 - 0.7):
                for v in (v0 + 0.6, v1 - 0.6):
                    g.add(m, lg.rivet(u, v, w + side * 0.13, side, r=0.28, mat=lg.STEEL))
    g.add(m, lg.slab([(24.6, -5.4), (22.6, -10.8), (21.6, -12.8), (17.0, -12.6), (17.4, -10.4), (18.2, -5.4)],
                     2 * HALF + 0.3, bevel=0.6, segments=3, mat=lg.TAPE, round=0.6))     # the taped grip
    fittings(m, guard=lg.STEEL, bipod_mat=lg.STEEL, screws=lg.STEEL)

    m = B('Skeleton')                                          # a lightened chassis showing its brass bedding bar
    windows = [lg.cslab([(46.0, -2.1), (51.0, -2.1), (51.0, -4.1), (46.0, -4.1)], -3, 3, round=0.6),
               lg.cslab([(53.0, -2.1), (58.0, -2.1), (58.0, -4.1), (53.0, -4.1)], -3, 3, round=0.6),
               lg.cslab([(-12.6, 0.0), (-2.4, 0.2), (-1.0, -1.6), (-2.6, -6.6), (-12.6, -7.6)], -3, 3, round=1.2)]
    chassis(m, SHELL, extra_cuts=windows)
    g.add(m, lg.box(44.0, 59.0, -3.5, -2.7, 1.6, bevel=0.15, mat=lg.BRASS))      # the bedding bar seen through
    receiver(m, lg.STEEL)
    grip_panels(m, lg.POLY_GREY)
    fittings(m, accent=(45.0, 58.0, -1.75))

    m = B('Ranger')                                            # two-tone, ember vents along the fore-end
    chassis(m, SHELL)
    upper = [(-17.0, 10.0), (70.0, 10.0), (70.0, -3.0), (61.6, -3.0), (40.0, -3.6), (25.5, -3.6), (22.6, -5.2),
             (19.0, -4.4), (12.0, -3.9), (5.0, -4.6), (-17.0, -4.6)]
    skin = lg.slab(offset(CHASSIS, 0.06), 2 * HALF + 0.12, bevel=0.9, segments=3, mat=lg.POLY_GREY, round=1.0)
    g.add(m, lg.cut(skin, [lg.cslab(upper, -3.0, 3.0), lg.cslab(offset(HOLE, 0.08), -3.0, 3.0, round=1.4)]))
    receiver(m, lg.BLUED)
    for k in range(4):                                         # ember vents
        u = 46.0 + k * 3.2
        for side in (-1.0, 1.0):
            g.add(m, lg.slab([(u, -2.95), (u + 1.0, -2.95), (u + 1.7, -2.15), (u + 0.7, -2.15)], 0.1,
                             w=side * (HALF + 0.03), bevel=0.0, mat=EMBER))
    g.add(m, lg.rail(46.0, 54.0, -4.75, width=1.8, mat=lg.BLACK, down=True))
    fittings(m, bipod=False, accent=(26.0, 39.5, -3.25))
    g.add(m, lg.box(54.6, 59.4, -6.0, -4.6, 2.6, bevel=0.3, mat=lg.BLACK))       # a short bipod folded forward
    for side in (-1.0, 1.0):
        g.add(m, lg.pipe([(55.2, -5.6, side * 0.85), (66.0, -5.0, side * 0.85)], 0.32, mat=lg.BLACK, sides=8))
        g.add(m, lg.box(65.6, 67.4, -5.6, -4.5, 1.0, w=side * 0.85, bevel=0.25, mat=lg.RUBBER))

    m = B('Armored')                                           # olive paint, steel plates bolted over the fore-end
    chassis(m, OLIVE)
    receiver(m, lg.BLUED)
    for side in (-1.0, 1.0):
        w = side * (HALF + 0.3)
        g.add(m, lg.slab([(43.6, -1.5), (60.4, -1.6), (61.2, -2.6), (61.0, -4.5), (43.6, -4.6)], 0.6, w=w, bevel=0.15,
                         mat=lg.STEEL, round=0.5))
        g.add(m, lg.slab([(-13.6, 0.8), (-1.4, 1.0), (-1.4, -6.0), (-13.6, -7.0)], 0.6, w=w, bevel=0.15, mat=OLIVE,
                         round=0.5))                           # a plate over the butt
        for u, v in ((44.6, -2.2), (44.6, -3.9), (52.0, -2.2), (52.0, -3.9), (59.6, -2.3), (59.6, -3.9), (-12.8, 0.1),
                     (-2.2, 0.3), (-12.8, -6.2), (-2.2, -5.3)):
            g.add(m, lg.rivet(u, v, w + side * 0.33, side, r=0.34, mat=lg.BLACK))
    grip_panels(m, lg.RUBBER)
    fittings(m, accent=(26.0, 39.5, -3.25))

    m = B('Gilded')                                            # sand with brass inlays, engraved plates, ember lines
    chassis(m, SHELL)
    receiver(m, lg.BLUED)
    for u in (18.2, 24.0, 33.5, 39.6):                         # brass bands round the action
        g.add(m, lg.tube(u, u + 0.6, 1.72, segments=24, mat=lg.BRASS))
    for side in (-1.0, 1.0):
        w = side * (HALF + 0.06)
        g.add(m, lg.slab([(19.0, -1.4), (39.0, -1.4), (39.6, -2.4), (39.0, -3.7), (19.0, -3.7), (18.4, -2.4)], 0.2,
                         w=w, bevel=0.05, mat=lg.BRASS, round=0.8))   # an engraved plate under the action
        for k, u in enumerate((24.0, 29.2, 34.4)):
            scroll(m, u, -2.55, w + side * 0.1, 0.95, flip=side if k % 2 == 0 else -side)
        g.add(m, lg.box(44.5, 60.0, -2.45, -2.3, 0.1, w=w, bevel=0.0, mat=lg.BRASS))
        g.add(m, lg.box(44.5, 60.0, -3.75, -3.6, 0.1, w=w, bevel=0.0, mat=lg.BRASS))
        # A brass-framed inlay on the butt with engraved curls, and an ember line along its toe.
        g.add(m, lg.slab([(-2.0, 0.8), (-12.6, 0.4), (-12.6, -6.0), (-7.6, -7.4), (-2.0, -5.2)], 0.12, w=w, bevel=0.03,
                         mat=lg.BRASS, round=1.0))
        g.add(m, lg.slab([(-2.6, 0.2), (-12.0, -0.2), (-12.0, -5.6), (-7.6, -6.8), (-2.6, -4.7)], 0.12,
                         w=w + side * 0.05, bevel=0.0, mat=SHELL, round=1.0))
        scroll(m, -5.6, -2.4, w + side * 0.13, 1.6, flip=side)
        scroll(m, -9.8, -2.8, w + side * 0.13, 1.3, flip=-side)
        g.add(m, lg.slab([(10.0, -11.25), (-13.6, -9.2), (-13.6, -8.98), (10.0, -11.03)], 0.08, w=side * (HALF + 0.03),
                         bevel=0.0, mat=EMBER))
    side_strip(m, 45.0, 59.0, -2.75, HALF + 0.04, EMBER, height=0.2)
    grip_panels(m, lg.BRASS)
    fittings(m, guard=lg.BRASS, screws=lg.BRASS, accent=(45.0, 59.0, -3.2))


# --- The bolt: the same on every gun ---

def bolt():
    m = model('Bolt', 'Standard', BOLT_AT)
    g.add(m, lg.tube(17.4, 31.2, 0.9, segments=16, mat=lg.STEEL))                 # the body, seen in the port
    g.add(m, lg.turned([(0.0, 0.7), (0.5, 1.05), (3.4, 1.25), (4.2, 1.25)], 13.4, segments=20, mat=lg.BLACK))  # shroud
    for k in range(3):                                         # locking lugs' grooves, seen in the port
        g.add(m, lg.tube(25.6 + k * 1.6, 26.2 + k * 1.6, 0.97, segments=16, mat=lg.BLUED))
    g.add(m, lg.pipe([(18.4, 0.0, -0.8), (18.4, -0.25, -2.0), (18.0, -1.0, -3.05)], 0.3, mat=lg.STEEL, sides=10))
    g.add(m, lg.pin(17.95, -1.15, -4.75, -2.95, 0.8, segments=16, mat=lg.BRASS,
                    profile=[(0.0, 0.25), (0.3, 0.62), (0.85, 0.8), (1.4, 0.62), (1.8, 0.3)]))   # the brass knob


# --- Barrels: from the receiver's front forward ---

def barrels():
    u0 = BARREL_AT[0]

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    def shank(length, r0=1.2, r1=0.88, mat=lg.BLUED, segments=20):
        return lg.turned([(0.0, r0), (5.0, r0), (8.0, r0 - 0.12), (length, r1)], u0, segments=segments, mat=mat)

    m = model('Barrel', 'Short', BARREL_AT)                    # a short sporter barrel
    g.add(m, shank(38.0, mat=lg.BLACK))
    tip(m, 38.0)

    m = model('Barrel', 'Fluted', BARREL_AT)                   # the Longhorn's: long, six flutes
    fluted = shank(50.0, r0=1.25, r1=0.95, segments=24)
    g.add(m, lg.cut(fluted, [lg.cbox(u0 + 9.0, u0 + 46.0, 0.62, 1.6, -0.26, 0.26, rotate_u=a) for a in range(0, 360, 60)]))
    tip(m, 50.0)

    m = model('Barrel', 'Heavy', BARREL_AT)                    # a thick bull barrel
    g.add(m, lg.turned([(0.0, 1.38), (47.4, 1.38), (48.0, 1.15)], u0, segments=24, mat=lg.BLACK))
    tip(m, 48.0)

    m = model('Barrel', 'Long', BARREL_AT)                     # long and slim, a steel band at the tip
    g.add(m, shank(60.0, r0=1.15, r1=0.82))
    g.add(m, lg.tube(u0 + 57.0, u0 + 58.2, 0.95, segments=20, mat=lg.STEEL))
    tip(m, 60.0)

    m = model('Barrel', 'Shrouded', BARREL_AT)                 # a slotted sand shroud over a steel barrel
    g.add(m, shank(50.0, r0=1.0, r1=0.9, mat=lg.STEEL))
    shroud = lg.turned([(0.0, 1.45), (0.8, 1.7), (40.0, 1.7), (41.0, 1.4)], u0 + 1.0, segments=24, mat=SHELL)
    slots = []
    for k in range(6):
        u = u0 + 6.0 + k * 5.6
        slots += [lg.cbox(u, u + 3.2, -0.45, 0.45, -3.0, 3.0), lg.cbox(u + 2.8, u + 6.0, 0.9, 2.5, -0.4, 0.4)]
    g.add(m, lg.cut(shroud, slots))
    g.add(m, lg.tube(u0 + 41.6, u0 + 42.6, 1.5, segments=24, mat=lg.BLACK))
    tip(m, 50.0)

    m = model('Barrel', 'Finned', BARREL_AT)                   # heat-sink fins at the chamber, a brass collar
    g.add(m, shank(46.0, r0=1.15, r1=0.92))
    for k in range(9):
        u = u0 + 2.5 + k * 1.15
        g.add(m, lg.turned([(0.0, 1.2), (0.15, 1.85), (0.45, 1.85), (0.6, 1.2)], u, segments=24, mat=lg.BLACK))
    g.add(m, lg.turned([(0.0, 1.35), (0.3, 1.55), (1.4, 1.55), (1.7, 1.35)], u0 + 13.4, segments=24, mat=lg.BRASS))
    tip(m, 46.0)

    m = model('Barrel', 'Tensioned', BARREL_AT)                # a slim barrel in a tension cage with brass rings
    g.add(m, shank(54.0, r0=1.05, r1=0.85))
    for u in (2.0, 18.0, 34.0, 50.0):
        g.add(m, lg.turned([(0.0, 1.2), (0.3, 1.75), (1.3, 1.75), (1.6, 1.2)], u0 + u, segments=24, mat=lg.BRASS))
    for a in (0.0, 120.0, 240.0):
        x, y = math.cos(math.radians(a + 90.0)) * 1.42, math.sin(math.radians(a + 90.0)) * 1.42
        g.add(m, lg.tube(u0 + 2.2, u0 + 51.4, 0.2, v=x, w=y, segments=8, mat=lg.STEEL))
    for u in (10.0, 26.0, 42.0):                               # ember glow rings between the cages
        g.add(m, lg.tube(u0 + u, u0 + u + 0.5, 1.02, segments=20, mat=EMBER))
    tip(m, 54.0)

    m = model('Barrel', 'Engraved', BARREL_AT)                 # octagonal, brass inlay bands and a scroll at the breech
    g.add(m, lg.turned([(0.0, 1.35), (6.0, 1.35), (8.0, 1.2), (58.0, 1.02)], u0, segments=8, spin=22.5, mat=lg.BLUED))
    for u, r in ((3.0, 1.38), (5.0, 1.38), (30.0, 1.15), (55.6, 1.06)):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.4, r, segments=8, spin=22.5, mat=lg.BRASS))
    for side in (-1.0, 1.0):
        scroll(m, u0 + 9.6, 0.0, side * 1.24, 0.8, flip=side)
        scroll(m, u0 + 12.0, 0.0, side * 1.22, 0.65, flip=-side)
    tip(m, 58.0)


# --- Muzzle devices: around their own origin, screwed onto the barrel's tip ---

def muzzles():
    u0 = MUZZLE_AT[0]

    def tip(m, length):
        g.socket(m, 'Muzzle', (u0 + length, 0.0))

    m = model('Muzzle', 'Crown', MUZZLE_AT)                    # a plain target crown cap
    g.add(m, lg.turned([(0, 1.0), (1.6, 1.0), (1.9, 0.8)], u0, segments=20, mat=lg.STEEL))
    tip(m, 1.9)

    m = model('Muzzle', 'Brake', MUZZLE_AT)                    # the Longhorn's: a ported brake with a brass ring
    brake = lg.turned([(0, 1.25), (6.0, 1.25), (6.4, 1.05)], u0, segments=20, mat=lg.BLACK)
    g.add(m, lg.cut(brake, [lg.cbox(u0 + u, u0 + u + 1.0, -0.55, 0.55, -2, 2) for u in (1.2, 2.8, 4.4)]))
    g.add(m, lg.tube(u0 + 6.4, u0 + 7.2, 1.1, segments=20, mat=lg.BRASS))
    tip(m, 7.2)

    m = model('Muzzle', 'Hood', MUZZLE_AT)                     # a slotted flash hood, closed on top
    hood = lg.turned([(0, 1.05), (1.0, 1.05), (5.4, 1.35), (5.8, 1.2)], u0, segments=20, mat=lg.BLACK)
    g.add(m, lg.cut(hood, [lg.cbox(u0 + 1.8, u0 + 5.2, 0.6, 1.6, -0.17, 0.17, rotate_u=a) for a in (90, 135, 180, 225, 270)]))
    tip(m, 5.8)

    m = model('Muzzle', 'Compensator', MUZZLE_AT)              # top ports keep the muzzle down
    comp = lg.turned([(0, 1.15), (5.0, 1.15), (5.8, 0.85)], u0, segments=20, mat=lg.STEEL)
    g.add(m, lg.cut(comp, [lg.cbox(u0 + u, u0 + u + 0.7, 0.2, 1.7, -0.4, 0.4) for u in (1.0, 2.3, 3.6)]))
    tip(m, 5.8)

    m = model('Muzzle', 'Radial', MUZZLE_AT)                   # a big radial brake, rows of round ports
    radial = lg.turned([(0, 1.2), (0.6, 1.55), (8.0, 1.55), (8.6, 1.25)], u0, segments=24, mat=lg.BLACK)
    holes = []
    for k, u in enumerate((1.8, 3.4, 5.0, 6.6)):
        holes += [lg.cpin(u0 + u, 0.0, -2.0, 2.0, 0.42), lg.upright(u0 + u + 0.8 * (k % 2), -2.0, 2.0, 0.42)]
    g.add(m, lg.cut(radial, holes))
    tip(m, 8.6)

    m = model('Muzzle', 'Suppressor', MUZZLE_AT)               # a slim can
    g.add(m, lg.turned([(0, 1.0), (0.8, 1.65), (12.4, 1.65), (13.0, 1.25)], u0, segments=24, mat=lg.BLACK))
    g.add(m, lg.tube(u0 + 11.6, u0 + 12.4, 1.71, segments=24, mat=lg.STEEL))
    tip(m, 13.0)

    m = model('Muzzle', 'LongSuppressor', MUZZLE_AT)           # a long can in a heat wrap, brass bands
    g.add(m, lg.turned([(0, 1.0), (1.0, 1.9), (19.0, 1.9), (19.8, 1.35)], u0, segments=24, mat=lg.BLACK))
    g.add(m, lg.tube(u0 + 3.0, u0 + 16.0, 2.0, segments=24, mat=lg.TAPE))
    for u in (3.0, 15.6):
        g.add(m, lg.tube(u0 + u, u0 + u + 0.6, 2.08, segments=24, mat=lg.BRASS))
    tip(m, 19.8)

    m = model('Muzzle', 'Forge', MUZZLE_AT)                    # a brass brake whose ports glow ember
    forge = lg.turned([(0, 1.3), (0.5, 1.5), (7.4, 1.5), (8.0, 1.2)], u0, segments=24, mat=lg.BRASS)
    ports = [lg.cbox(u0 + u, u0 + u + 1.1, -0.5, 0.5, -2.2, 2.2) for u in (1.4, 3.2, 5.0)]
    g.add(m, lg.cut(forge, ports))
    for u in (1.4, 3.2, 5.0):                                  # the glowing cores seen in the ports
        g.add(m, lg.tube(u0 + u - 0.1, u0 + u + 1.2, 1.0, segments=16, mat=EMBER))
    tip(m, 8.0)


# --- Magazines: small boxes, around where they seat ---

def boxmag(m, bottom, mat, floor=lg.BLACK, u0=32.8, u1=38.4, width=2.5, rake=0.0, w=0.0, top=-3.4, ribs=0):
    outline = [(u0, top), (u1, top), (u1 + rake, bottom), (u0 + rake, bottom)]
    g.add(m, lg.slab(outline, width, w=w, bevel=0.25, mat=mat, round=0.3))
    g.add(m, lg.box(u0 + rake - 0.25, u1 + rake + 0.25, bottom - 0.55, bottom + 0.15, width + 0.3, w=w, bevel=0.18,
                    mat=floor))
    for k in range(ribs):
        v = bottom + 1.0 + k * 0.9
        g.add(m, lg.box(u0 + rake * 0.5 - 0.1, u1 + rake * 0.5 + 0.1, v - 0.18, v + 0.18, width + 0.2, w=w,
                        bevel=0.06, mat=mat))


def rounds_window(m, u0, u1, v0, v1, w):
    """Brass cartridges' sides seen through a window on the magazine's right."""
    for k in range(int((v1 - v0) / 0.85)):
        v = v0 + 0.45 + k * 0.85
        g.add(m, lg.tube(u0, u1, 0.42, v=v, w=w, segments=10, mat=lg.BRASS))


def magazines():
    M = lambda key: model('Magazine', key, MAG_AT)

    m = M('5')                                                 # the Longhorn's: a short polymer box
    boxmag(m, -7.2, lg.POLY_BLACK)

    m = M('5Flush')                                            # flush with the chassis
    boxmag(m, -5.4, lg.POLY_BLACK, floor=lg.STEEL)

    m = M('5Window')                                           # smoke polymer, a window onto the rounds
    boxmag(m, -7.2, lg.POLY_GREY)
    g.parts[m][0] = lg.cut(g.parts[m][0], [lg.cbox(33.6, 37.6, -6.8, -5.0, -2.0, -0.6)])
    rounds_window(m, 33.4, 37.8, -6.9, -5.0, -0.3)

    m = M('6')                                                 # blued steel, ribbed
    boxmag(m, -8.0, lg.BLUED, floor=lg.BLUED, ribs=2)

    m = M('7')                                                 # a longer polymer box
    boxmag(m, -9.2, lg.POLY_BLACK, rake=0.3, ribs=3)

    m = M('8')                                                 # long steel, raked forward
    boxmag(m, -10.4, lg.BLUED, floor=lg.STEEL, rake=0.8, ribs=3)

    m = M('Coupled5')                                          # a spare clamped beside the loaded one
    boxmag(m, -7.2, lg.POLY_BLACK)
    boxmag(m, -8.0, lg.POLY_BLACK, w=2.7, top=-5.4)
    for v in (-6.0, -7.2):
        g.add(m, lg.box(32.6, 38.6, v - 0.35, v + 0.35, 5.6, w=1.35, bevel=0.15, mat=lg.STEEL))

    m = M('8Brass')                                            # brass-bound steel with a window and an ember follower
    boxmag(m, -10.4, lg.BLUED, floor=lg.BRASS, rake=0.8)
    g.parts[m][0] = lg.cut(g.parts[m][0], [lg.cbox(33.8, 37.8, -9.4, -5.4, -2.0, -0.6)])
    rounds_window(m, 33.6, 38.2, -9.0, -5.4, -0.3)
    g.add(m, lg.box(33.7, 38.3, -9.5, -9.1, 1.4, w=-0.4, bevel=0.05, mat=EMBER))
    for v in (-6.0, -9.9):
        shift = 0.8 * (-3.4 - v) / 7.0                         # the box rakes 0.8 cm forward over its height
        g.add(m, lg.box(32.65 + shift, 38.55 + shift, v - 0.3, v + 0.3, 2.8, bevel=0.08, mat=lg.BRASS))


# --- Scopes: around the middle of the rail ---

def scope(m, u_eye, u_obj, v, r_eye, r_tube, r_obj, ep=4.5, bell=7.0, mat=lg.BLACK, rings=(23.0, 35.5),
          ring_mat=lg.BLACK, glow=True, ember=True, turret=lg.POLY_GREY, segments=24):
    """A long scope on two rings: eyepiece, tube, objective bell (open, with glass and a fine cross), the turret saddle
    in the middle, a cyan ring round the objective and an ember line along each side."""
    v0 = SIGHT_AT[1]
    L = u_obj - u_eye
    g.add(m, hollow([(0.0, r_eye), (ep, r_eye), (ep + 1.4, r_tube), (L - bell, r_tube), (L - bell + 2.2, r_obj),
                     (L, r_obj)], u_eye, v, segments, mat))
    for u in rings:
        g.add(m, ring(u - 0.6, v, r_tube + 0.22, 1.2, ring_mat, wall=0.24))
        g.add(m, lg.box(u - 0.9, u + 0.9, v0, v - r_tube + 0.05, 2.0, bevel=0.2, mat=ring_mat))
        g.add(m, lg.pin(u, v - r_tube - 0.2, 1.0, 1.7, 0.35, mat=lg.STEEL))
    mid = (rings[0] + rings[1]) * 0.5
    if turret is not None:
        saddle = lg.box(mid - 2.0, mid + 2.0, v - r_tube - 0.35, v + r_tube + 0.35, 2 * r_tube + 0.7, bevel=0.4,
                        mat=turret)
        g.add(m, lg.cut(saddle, [lg.tube(mid - 3.0, mid + 3.0, r_tube - 0.05, v=v, segments=24)]))   # open for the eye
        g.add(m, lg.upright(mid, v + r_tube + 0.3, v + r_tube + 1.6, 0.9, mat=mat, segments=16))
        g.add(m, lg.pin(mid, v, -r_tube - 1.6, -r_tube - 0.3, 0.9, mat=mat, segments=16))
    if glow:
        g.add(m, ring(u_obj - 0.7, v, r_obj + 0.1, 0.5, OPTIC, wall=0.3))
    if ember:
        a, b = u_eye + ep + 2.0, u_obj - bell - 0.6
        for side in (-1.0, 1.0):
            g.add(m, lg.box(a, b, v - 0.12, v + 0.12, 0.08, w=side * (r_tube + 0.02), bevel=0.0, mat=EMBER))
    optics(m, u_eye, u_obj, v, r_eye, r_obj)
    g.socket(m, 'Aim', (u_eye + 0.2, v))   # the eyepiece lens


def sights():
    v0 = SIGHT_AT[1]
    S = lambda key: model('Sight', key, SIGHT_AT)

    m = S('Scout')                                             # a short 4x
    scope(m, 16.0, 40.0, v0 + 3.0, 1.7, 1.25, 1.95, ep=4.0, bell=5.0, ember=False, turret=None)
    g.add(m, lg.upright(28.5, v0 + 4.15, v0 + 5.2, 0.75, mat=lg.BLACK, segments=14))

    m = S('Longhorn')                                          # the Longhorn's: a long 5x, cyan objective, ember line
    scope(m, 13.0, 45.0, v0 + 3.4, 1.8, 1.3, 2.35)

    m = S('Ranger')                                            # 6x with a sunshade
    scope(m, 12.0, 47.0, v0 + 3.6, 1.8, 1.3, 2.5, glow=False)
    g.add(m, hollow([(0.0, 2.62), (5.0, 2.62)], 46.6, v0 + 3.6, 24, lg.BLACK, wall=0.15))   # the shade
    g.add(m, ring(51.0, v0 + 3.6, 2.7, 0.5, OPTIC, wall=0.3))

    m = S('Tactical')                                          # 5x, big turrets and a cyan offset dot at 45 degrees
    scope(m, 14.0, 44.0, v0 + 3.4, 1.8, 1.35, 2.3, ember=False)
    for u in (29.25,):
        g.add(m, lg.upright(u, v0 + 3.4 + 1.6, v0 + 3.4 + 2.6, 1.15, mat=lg.BLACK, segments=18,
                            profile=[(0, 1.0), (0.2, 1.15), (0.9, 1.15), (1.0, 0.9)]))
    dot = lg.box(30.0, 33.6, v0 + 4.6, v0 + 6.0, 1.6, bevel=0.25, mat=lg.BLACK)
    dot.data.transform(Matrix.Translation(lg.at(31.8, v0 + 3.4)) @ Matrix.Rotation(math.radians(-45.0), 4, 'Y')
                       @ Matrix.Translation(-lg.at(31.8, v0 + 3.4)))
    g.add(m, dot)
    lens = lg.box(30.0, 30.2, v0 + 4.9, v0 + 5.7, 1.0, bevel=0.0, mat=OPTIC)
    lens.data.transform(Matrix.Translation(lg.at(31.8, v0 + 3.4)) @ Matrix.Rotation(math.radians(-45.0), 4, 'Y')
                        @ Matrix.Translation(-lg.at(31.8, v0 + 3.4)))
    g.add(m, lens)

    m = S('Variable')                                          # 4-8x: a power ring with brass marks and a throw lever
    scope(m, 12.0, 46.0, v0 + 3.6, 1.85, 1.3, 2.45)
    g.add(m, ring(16.6, v0 + 3.6, 1.95, 2.2, lg.POLY_GREY, wall=0.4))   # open, like the tube
    for k in range(5):
        a = math.radians(-60.0 + k * 30.0)
        g.add(m, lg.box(17.4, 18.0, -0.08, 0.08, 0.3, w=0.0, bevel=0.0, mat=lg.BRASS))
        g.parts[m][-1].data.transform(Matrix.Translation(lg.at(0.0, v0 + 3.6)) @ Matrix.Rotation(a, 4, 'Y')
                                      @ Matrix.Translation(lg.at(0.0, 1.98)))
    g.add(m, lg.box(17.2, 18.2, v0 + 3.6 + 1.9, v0 + 3.6 + 3.0, 0.5, bevel=0.1, mat=lg.BRASS))   # the throw lever

    m = S('Seeker')                                            # 6x with a cyan rangefinder pod under the bell
    scope(m, 13.0, 46.0, v0 + 3.8, 1.8, 1.3, 2.5)
    g.add(m, lg.box(37.0, 44.0, v0 + 0.6, v0 + 1.5, 1.6, bevel=0.2, mat=lg.BLACK))
    g.add(m, lg.tube(44.2, 44.5, 0.6, v=v0 + 1.05, segments=16, mat=OPTIC))
    for u in (19.0, 22.0):                                     # cyan rings on the eyepiece
        g.add(m, ring(u, v0 + 3.8, 1.38, 0.35, OPTIC, wall=0.15))

    m = S('Marshal')                                           # 8x: long, brass rings and caps
    scope(m, 10.0, 50.0, v0 + 3.8, 1.9, 1.35, 2.65, ring_mat=lg.BRASS, turret=lg.BRASS, bell=8.0)
    g.add(m, ring(10.0, v0 + 3.8, 1.98, 0.8, lg.BRASS))
    g.add(m, ring(48.6, v0 + 3.8, 2.72, 0.6, lg.BRASS))

    m = S('Deadeye')                                           # 8x in a sand armor shell, brass and cyan
    scope(m, 10.0, 52.0, v0 + 4.0, 2.0, 1.4, 3.0, ring_mat=lg.BRASS, bell=9.0, turret=lg.BRASS, ember=False)
    g.add(m, hollow([(0.0, 1.62), (1.2, 1.78), (20.0, 1.78), (21.0, 1.62)], 19.0, v0 + 4.0, 8, SHELL, wall=0.2,
                    spin=22.5))                                # the armor shell over the tube, open like it
    for u in (19.6, 39.4):
        g.add(m, hollow([(0.0, 1.8), (0.5, 1.8)], u, v0 + 4.0, 8, lg.BRASS, wall=0.4, spin=22.5))
    for side in (-1.0, 1.0):
        g.add(m, lg.box(20.6, 38.4, v0 + 4.0 - 0.12, v0 + 4.0 + 0.12, 0.08, w=side * 1.66, bevel=0.0, mat=EMBER))
        scroll(m, 21.8, v0 + 4.0 + 0.9, side * 1.7, 0.5, flip=side)
    g.add(m, ring(14.0, v0 + 4.0, 2.06, 0.4, OPTIC, wall=0.3))


# --- Stocks: what fits on the butt's back face (the body's butt), from it backward ---

def pad(m, u_back=BUTT_U, thick=1.6, top=2.0, bottom=-10.0, mat=lg.RUBBER, ribs=False):
    """A pad on a back face at u_back: it laps 0.3 cm over it and stands a hair proud of the butt all round."""
    obj = lg.slab([(u_back + 0.3, bottom - 0.1), (u_back - thick, bottom - 0.3), (u_back - thick, top + 0.15),
                   (u_back + 0.3, top + 0.1)], 2 * HALF + 0.3, bevel=0.5, mat=mat, round=0.6)
    if ribs:
        obj = lg.cut(obj, [lg.cbox(u_back - thick - 0.6, u_back - thick + 0.5, v, v + 0.45, -3.0, 3.0)
                           for v in (-8.0, -5.6, -3.2, -0.8)])
    g.add(m, obj)


def cheek_piece(m, mat, posts=lg.STEEL, u0=-1.4, u1=-13.4, v=2.9, knob=lg.BLACK):
    """A raised cheek rest over the comb on two posts, its height knob on the left."""
    g.add(m, lg.slab([(u0, v), (u1 + 0.4, v - 0.1), (u1 - 0.4, v + 0.8), (u1 + 0.6, v + 1.7), (u0 - 0.4, v + 1.8),
                      (u0 + 0.6, v + 1.0)], 3.2, bevel=0.6, segments=3, mat=mat, round=0.8))
    for u in (u0 - 2.0, u1 + 2.4):
        g.add(m, lg.upright(u, 1.4, v + 0.2, 0.36, mat=posts))
    g.add(m, lg.pin((u0 + u1) * 0.5, 1.0, HALF - 0.05, HALF + 0.6, 0.55, segments=16, mat=knob))


def stocks():
    S = lambda key: model('Stock', key, STOCK_AT)
    b = BUTT_U

    m = S('Pad')                                               # the Longhorn's: a rubber pad
    pad(m)

    m = S('Recoil')                                            # a thick ribbed recoil pad
    pad(m, thick=3.0, ribs=True)

    m = S('Cheek')                                             # a grey cheek riser on steel posts
    pad(m)
    cheek_piece(m, lg.POLY_GREY)

    m = S('Spacers')                                           # length-of-pull spacers and a pad
    for k, mat in enumerate((lg.POLY_GREY, lg.BLACK, lg.POLY_GREY)):
        u = b + 0.3 - k * 0.9
        g.add(m, lg.slab([(u, -9.9), (u - 0.9, -9.9), (u - 0.9, 1.95), (u, 1.95)], 2 * HALF + 0.1, bevel=0.2, mat=mat,
                         round=0.5))
    pad(m, b - 2.4)

    m = S('Hook')                                              # a toe hook under the butt for the support hand
    pad(m)
    hook = [(b + 0.4, -9.4), (b - 1.6, -9.6), (b - 1.9, -13.6), (b - 0.6, -15.0), (b + 3.6, -15.0), (b + 4.6, -14.0),
            (b + 4.0, -13.2), (b + 0.6, -13.0), (b + 0.4, -11.0)]
    g.add(m, lg.slab(hook, 2.6, bevel=0.5, segments=3, mat=lg.POLY_GREY, round=0.6))
    g.add(m, lg.pin(b - 0.6, -10.6, -1.5, 1.5, 0.35, mat=lg.STEEL))

    m = S('Adjustable')                                        # a butt plate and a cheek piece on rods, brass knobs
    for v in (-0.8, -7.6):                                     # the butt plate's rods
        g.add(m, lg.tube(b - 2.6, b + 0.6, 0.42, v=v, mat=lg.STEEL))
    g.add(m, lg.slab([(b - 2.2, -10.2), (b - 3.0, -10.4), (b - 3.0, 2.1), (b - 2.2, 2.0)], 2 * HALF + 0.2, bevel=0.3,
                     mat=lg.BLACK, round=0.6))
    pad(m, b - 3.0, thick=1.2)
    cheek_piece(m, lg.POLY_GREY, posts=lg.STEEL, knob=lg.BRASS)
    g.add(m, lg.pin(b - 1.2, -4.2, HALF - 0.6, HALF + 0.5, 0.65, segments=16, mat=lg.BRASS))   # the plate's lock
    g.add(m, lg.box(b - 2.4, b + 0.6, -4.8, -3.6, 1.6, bevel=0.2, mat=lg.BLACK))

    m = S('Monopod')                                           # a pad over a fold-down monopod leg
    pad(m)
    g.add(m, lg.box(-13.0, -9.0, -11.8, -9.9, 2.4, bevel=0.3, mat=lg.BLACK))
    g.add(m, lg.upright(-11.0, -19.0, -11.6, 0.45, mat=lg.STEEL))
    g.add(m, lg.upright(-11.0, -15.4, -11.8, 0.62, mat=lg.BLACK))
    g.add(m, lg.upright(-11.0, -19.8, -18.8, 1.0, mat=lg.RUBBER, segments=16))
    g.add(m, lg.pin(-11.0, -13.4, -1.2, -0.62, 0.45, mat=lg.BRASS))

    m = S('Engraved')                                          # a heavy engraved brass butt plate on an ember seam
    g.add(m, lg.slab([(b + 0.3, -10.1), (b - 2.6, -10.3), (b - 2.6, 2.15), (b + 0.3, 2.1)], 2 * HALF + 0.3,
                     bevel=0.35, mat=lg.BRASS, round=0.6))
    g.add(m, lg.slab([(b - 1.2, -10.3), (b - 1.5, -10.3), (b - 1.5, 2.2), (b - 1.2, 2.2)], 2 * HALF + 0.36, bevel=0.0,
                     mat=EMBER))
    for side in (-1.0, 1.0):
        w = side * (HALF + 0.17)
        for v, flip in ((-1.0, side), (-6.8, -side)):          # curls either side of the seam
            scroll(m, b - 0.45, v, w, 0.6, turns=1.4, flip=flip)
        scroll(m, b - 2.05, -3.9, w, 0.48, turns=1.4, flip=side)
    pad(m, b - 2.6, thick=1.2)


# --- The design: names, rarities and stats live in Longhorn.parts.csv; here the parts by slot for the layout ---

SLOTS = {
    'Body': ['Standard', 'Heritage', 'Carbon', 'Salvager', 'Skeleton', 'Ranger', 'Armored', 'Gilded'],
    'Barrel': ['Short', 'Fluted', 'Heavy', 'Long', 'Shrouded', 'Finned', 'Tensioned', 'Engraved'],
    'Muzzle': ['Crown', 'Brake', 'Hood', 'Compensator', 'Radial', 'Suppressor', 'LongSuppressor', 'Forge'],
    'Magazine': ['5', '5Flush', '5Window', '6', '7', '8', 'Coupled5', '8Brass'],
    'Sight': ['Scout', 'Longhorn', 'Ranger', 'Tactical', 'Variable', 'Seeker', 'Marshal', 'Deadeye'],
    'Stock': ['Pad', 'Recoil', 'Cheek', 'Spacers', 'Hook', 'Adjustable', 'Monopod', 'Engraved'],
    'Bolt': ['Standard'],
}
DEFAULT = {'Body': 'Standard', 'Barrel': 'Fluted', 'Muzzle': 'Brake', 'Magazine': '5', 'Sight': 'Longhorn',
           'Stock': 'Pad', 'Bolt': 'Standard'}
SHEETS = {'Body': (0.85, 0.2), 'Barrel': (0.7, 0.05), 'Muzzle': (0.25, 0.06), 'Magazine': (0.12, 0.1),
          'Sight': (0.5, 0.1), 'Stock': (0.3, 0.26)}   # previews' grid (m): column step, row step
SOCKET_OF = {'Barrel': 'Barrel', 'Magazine': 'Magazine', 'Sight': 'Sight', 'Stock': 'Stock', 'Bolt': 'Bolt'}


def uv_unmapped():
    """Box-maps the glow, lens and reticle parts too (the kit maps only textured materials), so every face has UV
    area and Unreal's tangent build has nothing to warn about (they don't read their UVs)."""
    for parts in g.parts.values():
        for obj in parts:
            mat = obj.data.materials[0] if obj.data.materials else None
            if mat is not None and mat.name not in lg.SET_OF and obj.data.polygons:
                lt.box_uv(obj, 'MetalWorn', texel_density=lg.DENSITY)


bodies()
bolt()
barrels()
muzzles()
magazines()
sights()
stocks()
uv_unmapped()
built = g.build(origins)


def socket_at(obj, name):
    s = next(c for c in obj.children if c.name == 'SOCKET_' + name or c.name.startswith('SOCKET_' + name + '.'))
    return obj.location + s.location


def assemble(choice, at=Vector()):
    """Puts the chosen body at `at` and the chosen parts on its sockets; returns the objects."""
    body = built['Body_' + choice['Body']]
    body.location = Vector(at)
    shown = [body]
    for slot, socket in SOCKET_OF.items():
        obj = built[f'{slot}_{choice[slot]}']
        obj.location = socket_at(body, socket)
        obj.rotation_euler = (0.0, 0.0, 0.0)
        shown.append(obj)
    barrel = built['Barrel_' + choice['Barrel']]
    muzzle = built['Muzzle_' + choice['Muzzle']]
    muzzle.location = socket_at(barrel, 'Muzzle')
    shown.append(muzzle)
    bpy.context.view_layer.update()
    return shown


def layout():
    """In the scene: the default rifle assembled, every other part laid out in rows above it by slot."""
    assemble(DEFAULT)
    for row, (slot, keys) in enumerate(SLOTS.items()):
        for k, key in enumerate(keys):
            if key != DEFAULT[slot]:
                built[f'{slot}_{key}'].location = Vector((0.0, -k * 0.7, (row + 1) * 0.4))
                built[f'{slot}_{key}'].rotation_euler = (0.0, 0.0, 0.0)


layout()


def ads_view(objs, sight, out_png, lens=45.0, eye_relief=0.22):
    """Renders what aiming down the sights shows: a camera 22 cm behind the sight's SOCKET_Aim, looking along +u."""
    scene = bpy.context.scene
    eye = socket_at(sight, 'Aim') + Vector((0.0, eye_relief, 0.0))   # Blender +Y is toward the butt
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
    lt.preview(assemble(DEFAULT), out('Longhorn'), **side)
    lt.preview(assemble(DEFAULT), out('Longhorn_ThreeQuarter'), view=(-1.0, -0.9, 0.55), ground=False, lens=50.0,
               fit=0.7)
    lt.preview(assemble(DEFAULT), out('Longhorn_FirstPerson'), view=(-0.22, 1.0, 0.2), ground=False, lens=35.0, fit=0.5)
    # The bolt cycled: turned up about the bore and drawn back, as the game does it.
    shown = assemble(DEFAULT)
    bolt_obj = built['Bolt_Standard']
    bolt_obj.rotation_euler = (0.0, math.radians(60.0), 0.0)
    bolt_obj.location = bolt_obj.location + Vector((0.0, 0.09, 0.0))
    bpy.context.view_layer.update()
    lt.preview(shown, out('Longhorn_Bolt'), view=(-1.0, 0.3, 0.45), ground=False, lens=60.0, fit=0.62)
    variants = {
        'Longhorn_Gilded': {'Body': 'Gilded', 'Barrel': 'Engraved', 'Muzzle': 'Forge', 'Magazine': '8Brass',
                            'Sight': 'Deadeye', 'Stock': 'Engraved', 'Bolt': 'Standard'},
        'Longhorn_Ranger': {'Body': 'Ranger', 'Barrel': 'Tensioned', 'Muzzle': 'Radial', 'Magazine': '7',
                            'Sight': 'Seeker', 'Stock': 'Adjustable', 'Bolt': 'Standard'},
        'Longhorn_Salvager': {'Body': 'Salvager', 'Barrel': 'Short', 'Muzzle': 'Crown', 'Magazine': '5Flush',
                              'Sight': 'Scout', 'Stock': 'Recoil', 'Bolt': 'Standard'},
        'Longhorn_Heritage': {'Body': 'Heritage', 'Barrel': 'Long', 'Muzzle': 'LongSuppressor', 'Magazine': 'Coupled5',
                              'Sight': 'Marshal', 'Stock': 'Cheek', 'Bolt': 'Standard'},
        'Longhorn_Armored': {'Body': 'Armored', 'Barrel': 'Heavy', 'Muzzle': 'Hood', 'Magazine': '6',
                             'Sight': 'Tactical', 'Stock': 'Monopod', 'Bolt': 'Standard'},
        'Longhorn_Skeleton': {'Body': 'Skeleton', 'Barrel': 'Shrouded', 'Muzzle': 'Suppressor', 'Magazine': '5Window',
                              'Sight': 'Variable', 'Stock': 'Hook', 'Bolt': 'Standard'},
        'Longhorn_Carbon': {'Body': 'Carbon', 'Barrel': 'Finned', 'Muzzle': 'Compensator', 'Magazine': '8',
                            'Sight': 'Ranger', 'Stock': 'Spacers', 'Bolt': 'Standard'},
    }
    for name, choice in variants.items():
        lt.preview(assemble(choice), out(name), **side)
    # Aiming down the sights: the eye 22 cm behind each scope's SOCKET_Aim, looking along the gun.
    for key in SLOTS['Sight']:
        choice = dict(DEFAULT, Sight=key)
        ads_view(assemble(choice), built['Sight_' + key], out(f'Longhorn_ADS_{key}'))
    # A sheet per slot: every option side by side, two to a row.
    for slot, (w, h) in SHEETS.items():
        objs = []
        for k, key in enumerate(SLOTS[slot]):
            obj = built[f'{slot}_{key}']
            obj.location = Vector((0.0, -(k % 2) * w, -(k // 2) * h))
            obj.rotation_euler = (0.0, 0.0, 0.0)
            objs.append(obj)
        bpy.context.view_layer.update()
        lt.preview(objs, out(f'Longhorn_Parts_{slot}'), view=(-1.0, -0.15, 0.12), ground=False, lens=60.0, fit=0.8)
    layout()
