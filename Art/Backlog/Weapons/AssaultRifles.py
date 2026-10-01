"""Five assault rifle designs for the user to choose from (2026-09-30), in the stylized-realism style: real proportions
and parts, softened edges, worn textured materials. The user picked Kestrel, which became the modular base of every
non-legendary AR in Bullpup.py; the other four are kept for reference. Kept for later in Art/Backlog (see its README): nothing in the
game uses them, and the game's current rifle (Art/Models/Weapons/Rifle.py) is unchanged.

  Homestead   a wood-and-blued-steel frontier carbine: stamped receiver, walnut stock, grip and handguard, curved
              steel magazine, slanted muzzle brake. The rustic one.
  Regulator   a modern service carbine: flat-top receiver in black anodized metal, octagonal free-float handguard
              with slots and rails, tan polymer stock and grip, polymer magazine, holographic sight.
  Kestrel     a compact bullpup in sand polymer: magazine behind the grip, a thumbhole grip loop, an integrated
              optic housing with a scope, vents along the nose.
  Scrapjack   a salvaged heavy rifle: a welded and riveted receiver in rusty teal paint, a ported barrel shroud,
              a drum magazine, a bent-rebar carry handle with a scope clamped on, a pipe skeleton stock.
  Zephyr      a high-end precision rifle: faceted black receiver with brass details, a hexagonal handguard with
              angled vents, a glowing accent line, an integrated suppressor, a long scope on brass rings.

Each gun is a set of models in the gun's own space, as Art/README.md's "Gun parts" asks (origin at the back of the
receiver, +X toward the muzzle; the bullpup's origin is the back of its stock): <Gun>Body, <Gun>Magazine (its origin
where it seats, SOCKET_Magazine on the body) and, for the guns with an optic, <Gun>Sight (SOCKET_Sight). Bodies carry
SOCKET_Grip, SOCKET_Foregrip and SOCKET_Muzzle. The materials are tinted texture sets (MetalWorn, Polymer, GunWood,
MetalRust); before one of these joins the game, map its paint, grip and accent slots onto GunPaint, GunGrip and
GunAccent so the game can recolor it.

    blender -b --factory-startup --python Art/Backlog/Weapons/AssaultRifles.py -- --preview [Homestead ...]
"""
import math
import sys

import looter_guns as lg
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xffa62e, 3.0)    # glows in the gun's rarity color in the game (Legendary here)
RETICLE = lg.glow('GunReticle', 0xff3b2e, 6.0)


def mag_details(gun, front, rear, width, mat, rib_w, plate=0.7, steps=10):
    """A magazine between two edge curves: the body, a rib down each side and a floor plate."""
    outline = lg.curved_outline(front, rear, steps)
    gun.add('Magazine', lg.slab(outline, width, bevel=0.3, mat=mat))
    fr, rr = outline[:steps + 1], list(reversed(outline[steps + 1:]))
    mid = [((a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5) for a, b in zip(fr, rr)][1:-1]
    if rib_w:
        for side in (-1.0, 1.0):
            gun.add('Magazine', lg.strip(mid, 0.3, rib_w, w=side * (width * 0.5 + 0.05), mat=mat))
    a, b = fr[-1], rr[-1]
    du, dv = a[0] - fr[-2][0], a[1] - fr[-2][1]
    k = plate / max((du * du + dv * dv) ** 0.5, 1e-6)
    gun.add('Magazine', lg.slab([b, a, (a[0] + du * k, a[1] + dv * k), (b[0] + du * k, b[1] + dv * k)], width + 0.4,
                                bevel=0.2, mat=mat))


# --- Homestead: wood and blued steel ---

def homestead():
    g = lg.Gun('Homestead')
    B = 'Body'
    recv = lg.slab([(0, -4.2), (33, -4.2), (33, 0.8), (31.5, 1.6), (2, 1.6), (0, 1.0)], 3.6, bevel=0.3, mat=lg.BLUED)
    g.add(B, lg.cut(recv, [lg.cbox(15, 24, -1.0, 1.4, -2.6, -1.0)]))     # the ejection port, right side
    g.add(B, lg.slab([(2.5, 1.2), (31, 1.2), (31, 2.0), (29.5, 2.9), (5, 3.1), (3, 2.8), (2.2, 2.0)], 3.9, bevel=0.6,
                     segments=3, mat=lg.BLUED))                              # dust cover
    for u in (3.0, 5.0, 28.5, 31.0):
        for side in (-1.0, 1.0):
            g.add(B, lg.rivet(u, -3.0, side * 1.8, side, mat=lg.BLUED))
    g.add(B, lg.box(15.3, 23.7, -5.0, -3.8, 3.1, bevel=0.25, mat=lg.BLUED))  # magazine well lip
    g.add(B, lg.slab([(4, -0.6), (23, 0.3), (23.6, -0.5), (22, -1.3), (5, -1.8)], 0.25, w=-1.95, bevel=0.08,
                     mat=lg.BLUED))                                          # selector lever
    g.add(B, lg.pin(26.5, 0.6, -3.6, -1.8, 0.35, mat=lg.BLUED))
    g.add(B, lg.pin(26.5, 0.6, -4.4, -3.5, 0.6, mat=lg.BLUED, profile=[(0, 0.3), (0.25, 0.62), (0.65, 0.62), (0.9, 0.3)]))
    g.add(B, lg.box(33, 38, 0.6, 2.6, 2.8, bevel=0.3, mat=lg.BLUED))        # rear sight block
    g.add(B, lg.slab([(34, 2.6), (38.5, 2.6), (38.5, 3.2), (34.5, 3.5)], 1.8, bevel=0.1, mat=lg.BLUED))
    # Walnut handguard, lower and upper, held by steel bands; the gas tube and block.
    g.add(B, lg.slab([(35, -4.0), (55.5, -3.7), (57, -2.6), (57, 0.0), (35, 0.0)], 4.8, bevel=1.0, segments=3,
                     mat=lg.WALNUT))
    g.add(B, lg.slab([(37, 0.4), (53.5, 0.4), (53.5, 2.6), (52, 3.5), (38.5, 3.5), (37, 2.6)], 3.5, bevel=0.9,
                     segments=3, mat=lg.WALNUT))
    g.add(B, lg.box(34, 35.3, -4.5, 0.7, 5.1, bevel=0.2, mat=lg.BLUED))
    g.add(B, lg.box(56.6, 58.3, -3.0, 0.9, 4.0, bevel=0.2, mat=lg.BLUED))
    g.add(B, lg.tube(53, 60, 1.0, v=2.2, mat=lg.BLUED))
    g.add(B, lg.slab([(58.5, -1.2), (62, -1.2), (62, 3.2), (61, 3.6), (58.5, 3.6)], 2.6, bevel=0.3, mat=lg.BLUED))
    g.add(B, lg.tube(33, 80, 0.85, mat=lg.BLUED))
    # Front sight: a base, two protective ears and the post.
    g.add(B, lg.box(72, 75.5, -1.3, 1.4, 2.4, bevel=0.3, mat=lg.BLUED))
    for side in (-1.0, 1.0):
        g.add(B, lg.slab([(72.4, 1.3), (75.2, 1.3), (74.8, 4.9), (72.9, 4.9)], 0.45, w=side * 1.0, bevel=0.1,
                         mat=lg.BLUED))
    g.add(B, lg.upright(73.8, 1.3, 4.3, 0.18, mat=lg.BLUED))
    brake = lg.turned([(0, 1.25), (5.4, 1.25), (6.0, 0.95)], 80, mat=lg.BLUED)
    g.add(B, lg.cut(brake, [lg.cbox(81.6, 82.6, 0.3, 2.0, -0.55, 0.55), lg.cbox(83.4, 84.4, 0.3, 2.0, -0.55, 0.55),
                            lg.cbox(82.0, 84.8, -0.5, 0.5, -2.0, 2.0)]))
    # Walnut stock with a steel tang and butt plate; the pistol grip.
    g.add(B, lg.slab([(1, 1.4), (1, -3.9), (-6, -5.4), (-37, -12.6), (-38.5, -12.4), (-38.5, -0.6), (-37, -0.2),
                      (-6, 1.0)], 3.9, bevel=1.1, segments=3, mat=lg.WALNUT, round=0.8))
    g.add(B, lg.slab([(-38.4, -12.7), (-39.5, -12.5), (-39.5, -0.4), (-38.4, -0.2)], 4.1, bevel=0.3, mat=lg.BLUED))
    g.add(B, lg.box(-1.0, 1.4, -4.1, 1.3, 4.1, bevel=0.3, mat=lg.BLUED))
    g.add(B, lg.slab([(4.6, -4.0), (9.2, -4.0), (6.8, -15.0), (5.6, -15.8), (2.2, -15.8), (1.2, -14.8)], 3.0, bevel=0.9,
                     segments=3, mat=lg.WALNUT, round=0.6))
    g.add(B, lg.strip([(8.6, -4.2), (9.0, -6.6), (11.0, -7.6), (16.0, -7.4), (18.0, -4.3)], 0.8, 0.3, mat=lg.BLUED))
    g.add(B, lg.slab([(10.6, -4.2), (11.6, -4.2), (11.4, -5.4), (11.0, -6.4), (10.3, -6.6), (10.7, -5.4)], 0.5,
                     bevel=0.1, mat=lg.BLUED))
    g.add(B, lg.pipe([(-30, -11.0, -0.0), (-29.0, -12.6, 0.0), (-27.4, -11.9, 0.0)], 0.22, mat=lg.BLUED))  # sling loop
    mag_details(g, [(23.2, -4.2), (25.0, -15.0), (31.0, -25.5)], [(15.8, -4.2), (16.8, -15.0), (23.0, -26.5)], 2.8,
                lg.BLUED, 0.9)
    g.socket(B, 'Grip', (5.5, -9.0))
    g.socket(B, 'Foregrip', (46.0, -2.0))
    g.socket(B, 'Muzzle', (86.0, 0.0))
    g.socket(B, 'Magazine', (19.5, -4.2))
    return g.build(origins={'Magazine': (19.5, -4.2)})


# --- Regulator: modern service carbine ---

def regulator():
    g = lg.Gun('Regulator')
    B = 'Body'
    upper = lg.slab([(0, -1.6), (27, -1.6), (27, 2.2), (0, 2.2)], 2.9, bevel=0.25, mat=lg.BLACK)
    g.add(B, lg.cut(upper, [lg.cbox(9, 17, -0.3, 1.3, -2.0, -0.9)]))
    g.add(B, lg.box(9.2, 16.8, -0.15, 1.15, 0.6, w=-0.95, bevel=0.1, mat=lg.BRASS))   # bolt carrier in the port
    g.add(B, lg.slab([(17, 0.3), (19.2, 0.3), (19.2, 1.9), (17.4, 1.5)], 0.9, w=-1.7, bevel=0.15, mat=lg.BLACK))
    g.add(B, lg.tube(1.5, 6.5, 0.65, v=1.0, w=-1.8, mat=lg.BLACK))                      # forward assist
    g.add(B, lg.rail(0, 27, 2.2, mat=lg.BLACK))
    lower = lg.slab([(1, -1.6), (24, -1.6), (24, -3.2), (22, -5.0), (21.6, -8.4), (13.2, -8.4), (13.0, -5.6),
                     (8, -5.4), (3, -4.6), (1, -3.0)], 2.8, bevel=0.3, mat=lg.BLACK)
    g.add(B, lower)
    guard = lg.slab([(8.2, -5.3), (13.1, -5.5), (13.1, -8.0), (12.0, -8.5), (8.7, -8.0)], 2.0, bevel=0.25, mat=lg.BLACK)
    g.add(B, lg.cut(guard, [lg.cbox(9.0, 12.3, -7.6, -5.5, -2.0, 2.0)]))
    g.add(B, lg.slab([(10.0, -5.3), (10.9, -5.3), (10.8, -6.2), (10.4, -7.1), (9.8, -7.2), (10.2, -6.2)], 0.5,
                     bevel=0.1, mat=lg.BLACK))
    g.add(B, lg.slab([(3.2, -4.4), (8.4, -5.2), (6.4, -11.0), (6.9, -12.5), (5.6, -16.8), (1.6, -16.0), (0.8, -15.0),
                      (1.6, -9.0)], 3.0, bevel=0.8, segments=3, mat=lg.POLY_TAN, round=0.7))
    g.add(B, lg.slab([(4, -0.5), (8, -0.5), (8, 0.2), (4, 0.2)], 0.3, w=-1.5, bevel=0.08, mat=lg.BLACK))  # selector
    g.add(B, lg.tube(-21, 0, 1.5, v=0.3, mat=lg.BLACK))                                   # buffer tube
    stock = lg.slab([(-11, 2.6), (-11, -1.6), (-14, -2.4), (-25, -9.6), (-27.6, -9.6), (-28, -8.6), (-28, 3.4),
                     (-26, 3.8), (-14, 3.4)], 3.4, bevel=0.6, segments=3, mat=lg.POLY_TAN, round=0.8)
    g.add(B, lg.cut(stock, [lg.cslab([(-24.5, -6.6), (-17, -3.2), (-24.5, -3.2)], -2.0, 2.0, round=0.8)]))
    g.add(B, lg.slab([(-28, -9.6), (-29.3, -9.4), (-29.3, 3.6), (-28, 3.4)], 3.8, bevel=0.4, mat=lg.RUBBER))
    # Free-float handguard: an octagon with slots on its sides and bottom, rail on top.
    hg = lg.turned([(0, 2.4), (0.4, 2.65), (32.4, 2.65), (32.8, 2.4)], 27.3, segments=8, spin=22.5, mat=lg.BLACK)
    slots = []
    for u in (30.5, 37.5, 44.5, 51.5):
        slots += [lg.cbox(u, u + 3.4, -0.4, 0.4, 2.3, 3.0), lg.cbox(u, u + 3.4, -0.4, 0.4, -3.0, -2.3)]
        slots.append(lg.cbox(u, u + 3.4, -3.0, -2.25, -0.4, 0.4))
    g.add(B, lg.cut(hg, slots))
    g.add(B, lg.rail(27.3, 60.1, 2.45, mat=lg.BLACK))
    g.add(B, lg.tube(27, 72, 0.75, mat=lg.BLACK))
    hider = lg.turned([(0, 0.95), (5.6, 0.95), (6.0, 0.8)], 72, mat=lg.BLACK)
    g.add(B, lg.cut(hider, [lg.cbox(73.6, 77.6, 0.55, 1.2, -0.17, 0.17, rotate_u=a) for a in (0, 60, 120, 180, 240, 300)]))
    # Flip-up sights front and back.
    g.add(B, lg.slab([(1, 3.15), (4, 3.15), (3.6, 4.9), (2.1, 4.9)], 2.0, bevel=0.15, mat=lg.BLACK))
    g.add(B, lg.slab([(56.5, 3.4), (59.6, 3.4), (59.0, 5.4), (57.5, 5.4)], 1.8, bevel=0.15, mat=lg.BLACK))
    # Holographic sight.
    S = 'Sight'
    g.add(S, lg.box(8, 19, 3.15, 4.4, 3.0, bevel=0.25, mat=lg.BLACK))
    hood = lg.slab([(8, 4.3), (19, 4.3), (19, 8.6), (17.5, 9.5), (9.5, 9.5), (8, 8.6)], 3.4, bevel=0.4, segments=3,
                   mat=lg.BLACK)
    g.add(S, lg.cut(hood, [lg.cbox(7, 20, 5.0, 8.7, -1.25, 1.25)]))
    g.add(S, lg.box(17.4, 17.6, 5.0, 8.7, 2.5, bevel=0.0, mat=lg.LENS))
    g.add(S, lg.pin(17.2, 6.9, -0.12, 0.12, 0.12, mat=RETICLE))
    for u in (11.5, 13.5):
        g.add(S, lg.pin(u, 6.4, -2.1, -1.6, 0.45, mat=lg.POLY_BLACK))
    mag_details(g, [(21.0, -3.0), (21.8, -14.0), (23.6, -24.0)], [(13.8, -3.0), (14.4, -14.0), (15.8, -23.4)], 2.4,
                lg.POLY_BLACK, 0.0)
    for k in range(4):   # grip ribs low on the magazine
        v = -17.5 - k * 1.3
        g.add('Magazine', lg.box(15.0 + k * 0.25, 22.6 + k * 0.4, v - 0.35, v + 0.35, 2.7, bevel=0.15, mat=lg.POLY_BLACK))
    g.socket(B, 'Grip', (4.5, -10.0))
    g.socket(B, 'Foregrip', (44.0, -2.6))
    g.socket(B, 'Muzzle', (78.0, 0.0))
    g.socket(B, 'Magazine', (17.4, -3.0))
    g.socket(B, 'Sight', (13.5, 3.15))
    return g.build(origins={'Magazine': (17.4, -3.0), 'Sight': (13.5, 3.15)})


# --- Kestrel: bullpup ---

def kestrel():
    g = lg.Gun('Kestrel')
    B = 'Body'
    # A molded shell: the receiver, butt and sloped nose, and a slimmer thumbhole grip loop below.
    shell = lg.slab([(1.2, -9.6), (0, -8.6), (0, 2.0), (2, 3.4), (40, 3.2), (47, 2.4), (51, 0.6), (51.5, -1.8),
                     (49, -3.2), (38.5, -3.5), (37, -4.6), (19, -5.2), (18.6, -5.8), (9.5, -5.8), (3, -8.6)], 5.0,
                    bevel=1.1, segments=3, mat=lg.POLY_SAND, round=1.0)
    loop = lg.slab([(36.8, -3.8), (37.6, -12.0), (36.4, -14.0), (34, -14.8), (25, -15.2), (22.0, -14.8), (20.6, -12.5),
                    (19.6, -7.0), (19.4, -3.8)], 3.6, bevel=0.9, segments=3, mat=lg.POLY_SAND, round=1.0)
    g.add(B, lg.cut(loop, [lg.cslab([(25.0, -5.8), (34.4, -5.8), (35.4, -7.0), (35.6, -11.8), (34.2, -13.0),
                                     (27.0, -13.0), (25.4, -11.8), (24.2, -7.2)], -3.0, 3.0, round=1.0)]))
    vents = []
    for w0, w1 in ((-3.0, -2.1), (2.1, 3.0)):
        vents += [lg.cslab([(u, -1.6), (u + 0.9, -1.6), (u + 2.2, 1.2), (u + 1.3, 1.2)], w0, w1, round=0.3)
                  for u in (40.6, 42.8, 45.0)]
    port = [lg.cbox(2.6, 7.4, -1.0, 1.0, -3.0, -2.0)]
    g.add(B, lg.cut(shell, vents + port))
    g.add(B, lg.box(2.8, 7.2, -0.9, 0.9, 4.2, bevel=0.1, mat=lg.BLACK))       # bolt seen through the port
    for side in (-1.0, 1.0):
        g.add(B, lg.slab([(8, -4.8), (18.2, -4.8), (19.4, 1.6), (9.4, 1.6)], 0.3, w=side * 2.55, bevel=0.1,
                         mat=lg.POLY_GREY, round=0.8))                          # receiver side plates
        for u, v in ((9.6, -4.0), (17.6, -4.0), (10.6, 0.8), (18.2, 0.8)):
            g.add(B, lg.rivet(u, v, side * 2.72, side, r=0.28, mat=lg.BLACK))
    g.add(B, lg.slab([(0.5, 1.8), (11, 2.6), (11.5, 4.1), (1.2, 3.9)], 4.2, bevel=0.5, mat=lg.POLY_GREY, round=0.8))
    g.add(B, lg.slab([(0.2, -8.8), (-1.5, -8.8), (-1.5, 2.0), (0.2, 2.0)], 5.2, bevel=0.5, mat=lg.RUBBER, round=0.6))
    g.add(B, lg.slab([(26.4, -5.8), (27.4, -5.8), (27.2, -7.0), (26.8, -8.0), (26.2, -8.2), (26.5, -7.0)], 0.6,
                     bevel=0.12, mat=lg.BLACK))                                 # trigger
    g.add(B, lg.slab([(38.8, 2.9), (44.0, 2.6), (44.0, 3.6), (39.6, 3.9)], 1.0, w=1.9, bevel=0.2, mat=lg.BLACK))  # charging handle
    # Optic housing and its scope.
    g.add(B, lg.slab([(12, 3.0), (34, 3.0), (34, 5.0), (31, 6.2), (15, 6.2), (12, 5.0)], 3.6, bevel=0.8, segments=3,
                     mat=lg.POLY_SAND, round=0.6))
    S = 'Sight'
    g.add(S, lg.turned([(0, 2.0), (3.0, 2.0), (4.4, 1.5), (14.6, 1.5), (16.0, 2.2), (19.0, 2.2)], 14, v=8.0,
                       segments=20, mat=lg.BLACK))
    g.add(S, lg.tube(32.9, 33.05, 1.9, v=8.0, segments=20, mat=lg.LENS))
    g.add(S, lg.tube(13.95, 14.05, 1.8, v=8.0, segments=20, mat=lg.LENS))
    for u in (17.5, 27.0):
        g.add(S, lg.box(u, u + 2.2, 6.0, 7.4, 2.4, bevel=0.3, mat=lg.BLACK))
    g.add(S, lg.upright(23.0, 9.4, 10.6, 0.8, mat=lg.BLACK, segments=14))
    # Barrel, muzzle brake, a sling loop under the nose.
    g.add(B, lg.tube(50, 62, 0.8, mat=lg.BLACK))
    brake = lg.turned([(0, 1.3), (5.2, 1.3), (5.6, 1.0)], 62, mat=lg.BLACK)
    g.add(B, lg.cut(brake, [lg.cbox(63.0, 66.2, -0.35, 0.35, -2, 2), lg.cbox(63.0, 66.2, -2, 2, -0.35, 0.35)]))
    g.add(B, lg.pipe([(43.0, -3.0), (43.6, -4.6), (45.6, -4.6), (46.2, -3.0)], 0.25, mat=lg.BLACK))
    mag_details(g, [(17.6, -5.6), (18.3, -15.0), (19.8, -24.0)], [(10.4, -5.6), (10.9, -15.0), (12.0, -23.4)], 2.4,
                lg.POLY_BLACK, 0.0)
    for k in range(4):
        v = -17.0 - k * 1.3
        g.add('Magazine', lg.box(11.2 + k * 0.15, 19.2 + k * 0.25, v - 0.35, v + 0.35, 2.7, bevel=0.15, mat=lg.POLY_BLACK))
    g.socket(B, 'Grip', (22.5, -10.0))
    g.socket(B, 'Foregrip', (44.0, -4.5))
    g.socket(B, 'Muzzle', (67.6, 0.0))
    g.socket(B, 'Magazine', (14.0, -5.6))
    g.socket(B, 'Sight', (23.0, 6.2))
    return g.build(origins={'Magazine': (14.0, -5.6), 'Sight': (23.0, 6.2)})


# --- Scrapjack: salvaged heavy rifle ---

def scrapjack():
    g = lg.Gun('Scrapjack')
    B = 'Body'
    g.add(B, lg.slab([(0, -4.5), (34, -4.5), (34, 3.6), (30, 4.4), (2, 4.4), (0, 3.2)], 4.2, bevel=0.15, mat=lg.SALVAGE))
    for side in (-1.0, 1.0):
        w = side * 2.2
        g.add(B, lg.slab([(3, -3.6), (27, -3.6), (28, 2.8), (3, 2.8)], 0.3, w=w, bevel=0.06, mat=lg.SALVAGE))
        for u, v in ((4, -2.7), (26, -2.7), (4, 1.9), (26.8, 1.9), (15, -2.7), (15, 1.9)):
            g.add(B, lg.rivet(u, v, w + side * 0.15, side, r=0.35, mat=lg.STEEL))
        # Weld beads where the plates meet.
        g.add(B, lg.pipe([(0.5, 3.4, w * 0.98), (15, 3.5, w * 0.98), (29.5, 3.5, w * 0.98)], 0.22, mat=lg.STEEL, sides=6))
    g.add(B, lg.pipe([(6, 4.4), (6.8, 8.0), (8.8, 9.2), (23.2, 9.2), (25.2, 8.0), (26, 4.4)], 0.5, mat=lg.BLACK))
    g.add(B, lg.pin(24.0, 1.0, -3.9, -2.1, 0.38, mat=lg.STEEL))                   # bolt handle
    g.add(B, lg.pin(24.0, 1.0, -4.9, -3.8, 0.75, mat=lg.STEEL, profile=[(0, 0.3), (0.3, 0.75), (0.8, 0.75), (1.1, 0.4)]))
    g.add(B, lg.slab([(16, 0.4), (30, 0.4), (30, 1.6), (16, 1.6)], 0.4, w=-2.45, bevel=0.1, mat=lg.STEEL))  # its slot
    for u in (12.0, 20.0):                                                       # hose clamps on the scope
        g.add(B, lg.box(u - 0.6, u + 0.6, 8.6, 10.0, 1.8, bevel=0.2, mat=lg.STEEL))
    # Barrel shroud with cooling holes, the barrel inside, a box brake.
    shroud = lg.tube(34, 68, 2.3, segments=20, mat=lg.BLACK)
    holes = []
    for u in (37.5, 41.5, 45.5, 49.5, 53.5, 57.5, 61.5):
        holes.append(lg.cpin(u, 0.0, -3.0, 3.0, 0.75))
        holes.append(lg.upright(u + 2.0, -3.0, 3.0, 0.75))
    g.add(B, lg.cut(shroud, holes))
    g.add(B, lg.tube(33, 69, 1.0, mat=lg.STEEL))
    for u in (34.0, 67.0):
        g.add(B, lg.tube(u, u + 1.4, 2.55, segments=20, mat=lg.STEEL))
    brake = lg.box(69, 77, -1.9, 1.9, 4.4, bevel=0.4, mat=lg.SALVAGE)
    g.add(B, lg.cut(brake, [lg.cbox(u, u + 1.0, -1.4, 1.4, -3, 3) for u in (70.2, 72.2, 74.2)]))
    # Grip wrapped in tape, a bent-strip guard, the trigger.
    g.add(B, lg.slab([(4, -4.5), (9.4, -4.5), (7.4, -15.6), (6.2, -16.4), (2.8, -16.4), (2.0, -15.2)], 3.3, bevel=0.8,
                     segments=3, mat=lg.TAPE))
    for v in (-6.5, -9.0, -11.5, -14.0):
        g.add(B, lg.slab([(4.0 - (v + 4.5) * 0.19, v - 0.25), (9.6 - (v + 4.5) * 0.18, v - 0.25),
                          (9.6 - (v + 4.5) * 0.18, v + 0.25), (4.0 - (v + 4.5) * 0.19, v + 0.25)], 3.5, bevel=0.2,
                         mat=lg.TAPE))
    g.add(B, lg.strip([(9.2, -4.5), (9.4, -7.2), (11.4, -8.2), (17.0, -8.0), (18.5, -4.5)], 1.0, 0.45, mat=lg.STEEL))
    g.add(B, lg.slab([(11.0, -4.5), (12.0, -4.5), (11.8, -5.8), (11.4, -6.9), (10.7, -7.1), (11.1, -5.8)], 0.6,
                     bevel=0.1, mat=lg.STEEL))
    # Pipe skeleton stock, a rubber butt and a wrapped cheek pad.
    g.add(B, lg.pipe([(0.5, 2.0), (-28.5, 0.8), (-30.8, -0.6), (-31.0, -9.8), (-29.6, -11.4), (-6, -6.4), (0.5, -3.6)],
                     0.8, mat=lg.BLACK))
    g.add(B, lg.slab([(-31.2, -12.4), (-33.2, -12.2), (-33.2, 1.6), (-31.2, 1.6)], 3.8, bevel=0.5, mat=lg.RUBBER))
    g.add(B, lg.slab([(-25, 0.4), (-8, 1.4), (-8, 3.2), (-25, 2.4)], 3.0, bevel=0.8, segments=3, mat=lg.TAPE))
    # A vertical foregrip on a clamp under the shroud.
    g.add(B, lg.tube(48.5, 51.5, 2.75, segments=20, mat=lg.STEEL))
    g.add(B, lg.upright(50.0, -12.0, -2.3, 1.4, mat=lg.TAPE, segments=12,
                        profile=[(0, 1.2), (0.6, 1.5), (8.6, 1.35), (9.7, 1.0)]))
    # The scope, strapped to the handle.
    S = 'Sight'
    g.add(S, lg.turned([(0, 1.7), (2.6, 1.7), (3.6, 1.25), (12.4, 1.25), (13.6, 1.9), (16.6, 1.9)], 8.0, v=11.2,
                       mat=lg.STEEL))
    g.add(S, lg.tube(24.5, 24.6, 1.7, v=11.2, mat=lg.LENS))
    g.add(S, lg.upright(16.0, 12.4, 13.6, 0.7, mat=lg.STEEL))
    # Drum magazine with a feed neck and a winding key.
    M = 'Magazine'
    g.add(M, lg.slab([(13.8, -4.5), (20.2, -4.5), (20.8, -8.4), (13.2, -8.4)], 2.8, bevel=0.2, mat=lg.SALVAGE))
    g.add(M, lg.pin(17.0, -14.6, -2.7, 2.7, 7.4, segments=28, mat=lg.SALVAGE,
                    profile=[(0, 6.6), (0.5, 7.4), (4.9, 7.4), (5.4, 6.6)]))
    for side in (-1.0, 1.0):
        g.add(M, lg.pin(17.0, -14.6, side * 2.7 - (0.6 if side < 0 else 0.0), side * 2.7 + (0.6 if side > 0 else 0.0),
                        2.6, segments=16, mat=lg.STEEL))
    for k in range(8):
        ang = math.radians(22.5 + 45.0 * k)
        for side in (-1.0, 1.0):
            g.add(M, lg.rivet(17.0 + math.cos(ang) * 4.9, -14.6 + math.sin(ang) * 4.9, side * 2.72, side, r=0.32,
                              mat=lg.STEEL))
    g.add(M, lg.pin(17.0, -14.6, 3.2, 4.6, 0.5, mat=lg.STEEL))
    g.add(M, lg.box(16.4, 17.6, -15.2, -11.6, 0.5, w=4.4, bevel=0.15, mat=lg.STEEL))
    g.socket(B, 'Grip', (5.5, -10.0))
    g.socket(B, 'Foregrip', (50.0, -7.0))
    g.socket(B, 'Muzzle', (77.0, 0.0))
    g.socket(B, 'Magazine', (17.0, -4.5))
    g.socket(B, 'Sight', (16.0, 9.7))
    return g.build(origins={'Magazine': (17.0, -4.5), 'Sight': (16.0, 9.7)})


# --- Zephyr: high-end precision rifle ---

def zephyr():
    g = lg.Gun('Zephyr')
    B = 'Body'
    upper = lg.slab([(0, -1.8), (30, -1.8), (30, 2.0), (28.5, 3.0), (1.5, 3.0), (0, 2.0)], 3.0, bevel=0.15, mat=lg.BLACK)
    g.add(B, lg.cut(upper, [lg.cbox(10, 18.5, -0.4, 1.5, -2.0, -1.0)]))
    g.add(B, lg.box(10.2, 18.3, -0.2, 1.3, 0.6, w=-1.0, bevel=0.1, mat=lg.BRASS))
    for side in (-1.0, 1.0):
        g.add(B, lg.slab([(4, -1.4), (26, -1.4), (24, 1.7), (6, 1.7)], 0.25, w=side * 1.62, bevel=0.06, mat=lg.BLACK))
        g.add(B, lg.box(5.5, 24.5, -1.3, -1.0, 0.1, w=side * 1.78, bevel=0.0, mat=ACCENT))
    g.add(B, lg.slab([(25, 1.0), (29.5, 1.0), (29.5, 1.8), (25, 1.8)], 0.8, w=-1.9, bevel=0.15, mat=lg.BRASS))  # charging handle
    g.add(B, lg.rail(0, 30, 3.0, mat=lg.BLACK))
    lower = lg.slab([(1, -1.8), (26, -1.8), (26, -3.0), (23.6, -5.2), (23.2, -8.8), (14.2, -8.8), (13.8, -5.6), (8, -5.6),
                     (3, -4.8), (1, -3.2)], 2.9, bevel=0.15, mat=lg.BLACK)
    g.add(B, lower)
    guard = lg.slab([(8.2, -5.5), (13.9, -5.7), (13.9, -8.2), (12.6, -8.9), (8.8, -8.4)], 2.1, bevel=0.15, mat=lg.BLACK)
    g.add(B, lg.cut(guard, [lg.cbox(9.0, 13.1, -8.0, -5.7, -2.0, 2.0)]))
    g.add(B, lg.slab([(10.2, -5.5), (11.1, -5.5), (11.0, -6.4), (10.6, -7.4), (10.0, -7.5), (10.4, -6.4)], 0.5, bevel=0.1,
                     mat=lg.BRASS))
    g.add(B, lg.slab([(3.0, -4.6), (8.6, -5.4), (6.6, -16.6), (5.4, -17.2), (1.9, -16.4), (1.2, -14.6)], 3.0, bevel=0.5,
                     segments=3, mat=lg.POLY_BLACK, round=0.5))
    # Hexagonal handguard with angled vents, an accent line along each side, rail on top.
    hg = lg.turned([(0, 2.6), (0.4, 2.9), (37.6, 2.9), (38.0, 2.6)], 30.2, segments=6, mat=lg.BLACK)
    vents = []
    for u in (33, 38, 43, 48, 53, 58):
        for w0, w1 in ((-3.6, -2.25), (2.25, 3.6)):
            vents.append(lg.cslab([(u, -2.0), (u + 2.4, -2.0), (u + 3.6, -0.6), (u + 1.2, -0.6)], w0, w1))
    g.add(B, lg.cut(hg, vents))
    for side in (-1.0, 1.0):
        g.add(B, lg.tube(33, 66, 0.13, w=side * 2.86, segments=6, mat=ACCENT))
    g.add(B, lg.rail(30.2, 68.2, 2.5, mat=lg.BLACK))
    # Integrated suppressor with brass rings.
    g.add(B, lg.tube(68, 70, 1.2, mat=lg.BLACK))
    g.add(B, lg.turned([(0, 1.5), (0.4, 1.75), (17.4, 1.75), (18.0, 1.4)], 70, segments=20, mat=lg.BLACK))
    for u in (70.4, 86.4):
        g.add(B, lg.tube(u, u + 0.8, 1.82, segments=20, mat=lg.BRASS))
    # Skeletonized precision stock with a brass cheek-riser knob, a rubber pad.
    stock = lg.slab([(0.5, 2.4), (0.5, -3.4), (-8, -4.4), (-26, -10.8), (-30, -10.8), (-30, 4.6), (-27, 5.0),
                     (-14, 4.0), (-8, 2.6)], 3.2, bevel=0.2, mat=lg.BLACK)
    g.add(B, lg.cut(stock, [lg.cslab([(-24.5, -7.6), (-11, -3.2), (-24.5, 1.4)], -2, 2, round=0.8)]))
    for u in (-24.0, -18.0):
        g.add(B, lg.upright(u, 4.0, 5.2, 0.32, mat=lg.BRASS))
    g.add(B, lg.slab([(-27.5, 4.6), (-15, 3.8), (-15, 5.6), (-27.5, 6.4)], 2.8, bevel=0.5, segments=3, mat=lg.POLY_BLACK))
    g.add(B, lg.pin(-21, 4.6, -2.6, -1.4, 0.55, mat=lg.BRASS))
    g.add(B, lg.slab([(-30, -10.8), (-31.4, -10.6), (-31.4, 4.8), (-30, 4.6)], 3.6, bevel=0.4, mat=lg.RUBBER))
    # Long scope on brass rings.
    S = 'Sight'
    g.add(S, lg.turned([(0, 1.9), (5.0, 1.9), (6.6, 1.35), (18.0, 1.35), (20.5, 2.35), (26.0, 2.35)], 3.0, v=6.8,
                       segments=24, mat=lg.BLACK))
    g.add(S, lg.tube(28.9, 29.05, 2.2, v=6.8, segments=24, mat=lg.LENS))
    g.add(S, lg.upright(13.0, 8.0, 9.8, 0.95, mat=lg.BLACK, segments=16))
    g.add(S, lg.pin(13.0, 6.8, -3.0, -1.2, 0.95, mat=lg.BLACK, segments=16))
    for u in (9.5, 19.0):
        g.add(S, lg.tube(u - 0.6, u + 0.6, 1.62, v=6.8, segments=20, mat=lg.BRASS))
        g.add(S, lg.box(u - 0.9, u + 0.9, 3.95, 5.4, 2.2, bevel=0.2, mat=lg.BRASS))
    # Straight aluminum magazine with a witness window showing brass.
    M = 'Magazine'
    mag = lg.slab(lg.curved_outline([(22.6, -3.0), (23.0, -12.0), (23.8, -21.5)], [(14.6, -3.0), (15.0, -12.0),
                                                                                    (15.6, -21.0)]), 2.4, bevel=0.25,
                  mat=lg.BLACK)
    g.add(M, lg.cut(mag, [lg.cbox(17.5, 19.0, -18.5, -6.0, -2.0, -0.6)]))
    g.add(M, lg.box(17.5, 19.0, -18.5, -6.0, 1.2, w=-0.2, bevel=0.3, mat=lg.BRASS))
    g.add(M, lg.box(14.8, 24.2, -22.4, -21.0, 2.8, bevel=0.3, mat=lg.BRASS))
    g.socket(B, 'Grip', (4.5, -10.5))
    g.socket(B, 'Foregrip', (48.0, -2.9))
    g.socket(B, 'Muzzle', (88.0, 0.0))
    g.socket(B, 'Magazine', (18.6, -3.0))
    g.socket(B, 'Sight', (14.0, 3.95))
    return g.build(origins={'Magazine': (18.6, -3.0), 'Sight': (14.0, 3.95)})


RIFLES = {'Homestead': homestead, 'Regulator': regulator, 'Kestrel': kestrel, 'Scrapjack': scrapjack, 'Zephyr': zephyr}

built = {}
for k, (name, make) in enumerate(RIFLES.items()):
    built[name] = make()
    for obj in built[name].values():
        obj.location.z += k * 0.45   # stacked in the scene; positions don't matter to the export

if lt.want_preview():
    argv = sys.argv[sys.argv.index('--') + 1:]
    wanted = [a for a in argv if a in RIFLES] or list(RIFLES)
    for name in wanted:
        lt.preview(list(built[name].values()), lt.preview_path('Backlog', name), view=(-1.0, -0.25, 0.25), ground=False,
                   lens=60.0, fit=0.95)
