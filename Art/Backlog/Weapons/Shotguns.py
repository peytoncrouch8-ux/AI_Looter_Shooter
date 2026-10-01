"""Five shotgun designs for the user to choose from (2026-10-01), in the stylized-realism style of the AR concepts:
real proportions and parts, softened edges, worn textured materials. Kept for later in Art/Backlog (see its README):
nothing in the game uses them, and the game's current shotgun (Art/Models/Weapons/Shotgun.py) is unchanged.

  Farmhand     a walnut-and-blued-steel pump-action: vent-rib barrel over a tube magazine, a grooved walnut pump,
               a pistol-grip stock with a leather shell cuff holding four shells. The rustic one.
  Coachman     a side-by-side double-barrel coach gun: short twin barrels, case-colored side locks with exposed
               hammers, twin triggers, a straight English stock and splinter forend in walnut, brass details.
  Breacher     a tactical semi-auto fed by a wide box magazine: black receiver railed end to end, a tan slotted
               handguard and collapsible stock, ghost-ring sights, a toothed breaching muzzle.
  KestrelS     a twin-tube bullpup pump in the AR's white shell: the same side plates, rivets and rarity accent line,
               two windowed magazine tubes (red shells showing) under the barrel, a grey pump with a hand stop.
  Thunderdrum  a salvaged full-auto drum shotgun: a long welded receiver in rusty teal with a carry handle, a ported
               shroud and box brake, a 20-round drum, a taped grip and foregrip, an old walnut stock bolted on.

Each gun is a set of models in the gun's own space (Art/README.md's "Gun parts": origin at the back of the receiver,
+X toward the muzzle; the bullpup's origin is the back of its butt): <Gun>Body, a <Gun>Pump for the pump-actions (it
slides back along -X to cycle), a <Gun>Magazine for the box- and drum-fed ones (its origin where it seats,
SOCKET_Magazine on the body). Bodies carry SOCKET_Grip, SOCKET_Foregrip and SOCKET_Muzzle (the Coachman's second
barrel: SOCKET_Muzzle2).

    blender -b --factory-startup --python Art/Backlog/Weapons/Shotguns.py -- --preview [Farmhand ...]
"""
import math
import sys

import looter_guns as lg
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xe8e8e8, 2.0)    # glows in the gun's rarity color in the game
SHELL = lg.material('GunShellRed', 'Polymer', 0xb8402f)
CASE = lg.material('GunCaseColored', 'MetalWorn', 0x9a8f80)   # case-hardened side locks
LEATHER = lg.material('GunLeather', 'Polymer', 0x7a5236)


def shell_upright(g, model, u, v0, w):
    """A shotgun shell standing along v: brass head at the bottom, red hull above."""
    g.add(model, lg.upright(u, v0, v0 + 1.1, 1.02, w=w, mat=lg.BRASS, segments=12))
    g.add(model, lg.upright(u, v0 + 1.0, v0 + 6.0, 0.95, w=w, mat=SHELL, segments=12,
                            profile=[(0, 0.95), (4.8, 0.95), (5.0, 0.8)]))


def stock_grooves(obj, u0, u1, v_list, half_w, depth=0.35):
    """Finger grooves along a pump or forend: shallow slots in both sides."""
    cuts = []
    for v in v_list:
        cuts += [lg.cbox(u0, u1, v - 0.13, v + 0.13, half_w - depth, half_w + 1.0),
                 lg.cbox(u0, u1, v - 0.13, v + 0.13, -half_w - 1.0, -half_w + depth)]
    return lg.cut(obj, cuts)


# --- Farmhand: walnut pump-action ---

def farmhand():
    g = lg.Gun('Farmhand')
    B, P = 'Body', 'Pump'
    recv = lg.slab([(0, -3.0), (20, -3.0), (20, 1.4), (18.5, 2.4), (1.5, 2.6), (0, 1.8)], 3.0, bevel=0.25,
                   mat=lg.BLUED, round=0.4)
    g.add(B, lg.cut(recv, [lg.cbox(6, 14, -0.8, 1.4, -2.0, -1.0)]))              # ejection port, right side
    g.add(B, lg.box(6.2, 13.8, -0.6, 1.2, 0.6, w=-0.9, bevel=0.1, mat=lg.STEEL))  # the bolt in the port
    g.add(B, lg.slab([(3, -3.0), (13, -3.0), (12.4, -4.6), (4, -4.4)], 2.4, bevel=0.2, mat=lg.BLUED, round=0.3))
    g.add(B, lg.strip([(4.6, -4.4), (5.0, -6.8), (7.0, -7.6), (11.0, -7.2), (12.4, -4.6)], 0.8, 0.3, mat=lg.BLUED))
    g.add(B, lg.slab([(7.4, -4.4), (8.4, -4.4), (8.2, -5.6), (7.8, -6.6), (7.2, -6.8), (7.5, -5.6)], 0.5, bevel=0.1,
                     mat=lg.BLUED))
    g.add(B, lg.pin(10.6, -3.7, -1.5, 1.5, 0.3, mat=lg.STEEL))                   # safety button
    g.add(B, lg.tube(20, 66, 1.15, segments=20, mat=lg.BLUED))                   # barrel
    g.add(B, lg.box(20, 66, 1.1, 1.65, 0.7, bevel=0.1, mat=lg.BLUED))            # vent rib
    for u in range(23, 65, 3):
        g.add(B, lg.box(u, u + 1.0, 1.0, 1.2, 0.72, bevel=0.0, mat=lg.STEEL))
    g.add(B, lg.upright(65.0, 1.6, 2.0, 0.22, mat=lg.BRASS, segments=8))        # bead sight
    g.add(B, lg.tube(20, 58, 1.05, v=-2.5, segments=16, mat=lg.BLUED))          # magazine tube
    g.add(B, lg.turned([(0, 1.2), (1.6, 1.2), (2.0, 0.9)], 58, v=-2.5, segments=16, mat=lg.BLUED))
    g.add(B, lg.slab([(56.5, -3.4), (58.5, -3.4), (58.5, 0.9), (56.5, 0.9)], 2.2, bevel=0.3, mat=lg.BLUED, round=0.4))
    stock = lg.slab([(0, 1.8), (0, -3.0), (-2.5, -4.4), (-5.5, -7.4), (-9, -8.0), (-12, -7.4), (-37, -12.6),
                     (-38.5, -12.4), (-38.5, 0.2), (-36, 0.6), (-14, 1.0), (-4, 1.8)], 3.8, bevel=1.1, segments=3,
                    mat=lg.WALNUT, round=1.0)
    g.add(B, stock)
    g.add(B, lg.slab([(-38.4, -12.7), (-40.2, -12.6), (-40.2, 0.4), (-38.4, 0.3)], 4.0, bevel=0.5, mat=lg.RUBBER,
                     round=0.5))
    # A leather shell cuff on the stock, four shells in it on the right side.
    g.add(B, lg.slab([(-24, -9.6), (-14, -7.8), (-14, 1.2), (-24, 0.9)], 4.5, bevel=0.6, segments=3, mat=LEATHER,
                     round=0.8))
    for k, u in enumerate((-22.2, -20.0, -17.8, -15.6)):
        shell_upright(g, B, u, -6.9 + k * 0.42, -2.6)
    g.add(B, lg.pipe([(-30, -11.2), (-29.0, -12.8), (-27.4, -12.1)], 0.22, mat=lg.STEEL))   # sling loop
    # The pump: a grooved walnut forend riding the magazine tube, action bars back to the receiver.
    pump = lg.slab([(28, -1.4), (46, -1.4), (47, -2.4), (46.5, -4.0), (45, -4.4), (29, -4.4), (27.5, -3.6),
                    (27.5, -2.0)], 4.4, bevel=1.0, segments=3, mat=lg.WALNUT, round=0.8)
    g.add(P, stock_grooves(pump, 31, 44, (-2.2, -2.9, -3.6), 2.2))
    for side in (-1.0, 1.0):
        g.add(P, lg.box(19.5, 28.5, -2.3, -1.8, 0.3, w=side * 1.2, bevel=0.05, mat=lg.STEEL))
    g.socket(B, 'Grip', (-4.5, -6.0))
    g.socket(B, 'Foregrip', (37.0, -3.0))
    g.socket(B, 'Muzzle', (66.0, 0.0))
    return g.build()


# --- Coachman: side-by-side double barrel ---

def coachman():
    g = lg.Gun('Coachman')
    B = 'Body'
    for w in (-1.05, 1.05):
        g.add(B, lg.turned([(0, 1.18), (3.0, 1.18), (5.0, 1.08), (46.5, 1.08)], 15.5, w=w, segments=20, mat=lg.BLUED))
    g.add(B, lg.box(16, 62, 0.55, 1.25, 0.8, bevel=0.1, mat=lg.BLUED))           # top rib in the valley
    g.add(B, lg.box(16, 62, -1.25, -0.55, 0.8, bevel=0.1, mat=lg.BLUED))
    g.add(B, lg.upright(61.4, 1.2, 1.6, 0.22, mat=lg.BRASS, segments=8))         # bead
    action = lg.slab([(0, -3.4), (16, -3.4), (16, 1.2), (14, 1.6), (4, 1.6), (0, 0.8)], 4.2, bevel=0.3, mat=CASE,
                     round=0.5)
    g.add(B, action)
    for side in (-1.0, 1.0):
        g.add(B, lg.slab([(1, -2.6), (13, -2.6), (13.5, -0.8), (12, 0.8), (3, 1.0), (0.6, 0.0)], 0.25, w=side * 2.2,
                         bevel=0.08, mat=CASE, round=0.8))                        # side lock plates
        g.add(B, lg.slab([(2.4, 0.6), (4.4, 0.6), (5.2, 2.8), (4.0, 4.8), (1.8, 5.6), (1.2, 5.0), (2.8, 3.2)], 0.8,
                         w=side * 1.25, bevel=0.15, mat=lg.BLUED, round=0.25))   # hammers
        g.add(B, lg.pin(8.0, -0.8, side * 2.2 - (0.25 if side < 0 else 0.0), side * 2.2 + (0.25 if side > 0 else 0.0),
                        0.35, mat=lg.STEEL))                                      # lock pins
    g.add(B, lg.slab([(-1.5, 0.7), (3.5, 0.9), (3.0, 1.8), (-1.2, 1.5)], 1.0, bevel=0.15, mat=lg.BLUED, round=0.3))
    g.add(B, lg.strip([(4.0, -3.4), (4.4, -6.2), (6.6, -7.2), (11.4, -6.8), (13.0, -3.6)], 0.8, 0.3, mat=lg.BRASS))
    for u in (6.6, 8.6):
        g.add(B, lg.slab([(u, -3.4), (u + 0.9, -3.4), (u + 0.7, -4.8), (u + 0.3, -5.8), (u - 0.3, -6.0), (u, -4.8)],
                         0.5, bevel=0.1, mat=lg.BLUED))
    # Splinter forend with its iron, a straight English stock with a brass oval.
    g.add(B, lg.slab([(16, -0.9), (33, -0.9), (34, -1.6), (33.5, -2.8), (16, -2.8)], 3.6, bevel=0.6, segments=3,
                     mat=lg.WALNUT, round=0.6))
    g.add(B, lg.box(15.8, 19.5, -3.0, -1.0, 3.0, bevel=0.2, mat=CASE))
    g.add(B, lg.slab([(0, 0.8), (0, -3.4), (-7, -3.9), (-36, -12.0), (-37.5, -11.8), (-37.5, -0.6), (-36, -0.2),
                      (-7, 0.3)], 3.8, bevel=1.1, segments=3, mat=lg.WALNUT, round=1.0))
    g.add(B, lg.slab([(-37.4, -12.1), (-38.3, -12.0), (-38.3, -0.4), (-37.4, -0.5)], 3.9, bevel=0.3, mat=CASE))
    g.add(B, lg.pin(-12.0, -2.8, -2.3, -1.8, 0.9, mat=lg.BRASS, segments=14))
    g.socket(B, 'Grip', (-4.0, -2.0))
    g.socket(B, 'Foregrip', (25.0, -2.0))
    g.socket(B, 'Muzzle', (62.0, 0.0), w=1.05)
    g.socket(B, 'Muzzle2', (62.0, 0.0), w=-1.05)
    return g.build()


# --- Breacher: tactical semi-auto, box magazine ---

def breacher():
    g = lg.Gun('Breacher')
    B, M = 'Body', 'Magazine'
    recv = lg.slab([(0, -3.4), (28, -3.4), (28, 1.6), (26.5, 2.6), (1.5, 2.6), (0, 1.6)], 3.6, bevel=0.25,
                   mat=lg.BLACK, round=0.3)
    g.add(B, lg.cut(recv, [lg.cbox(9, 19, -0.6, 1.6, -2.2, -1.2)]))
    g.add(B, lg.box(9.2, 18.8, -0.4, 1.4, 0.6, w=-1.2, bevel=0.1, mat=lg.STEEL))
    g.add(B, lg.slab([(20, 0.4), (24, 0.4), (24, 1.2), (20, 1.2)], 1.0, w=-2.2, bevel=0.15, mat=lg.BLACK))   # charging handle
    g.add(B, lg.rail(1, 27.5, 2.6, mat=lg.BLACK))
    g.add(B, lg.slab([(12.6, -3.4), (23.2, -3.4), (23.6, -5.2), (12.2, -5.2)], 4.4, bevel=0.3, mat=lg.BLACK, round=0.4))  # flared well
    guard = lg.slab([(4.0, -3.4), (11.6, -3.4), (11.6, -6.6), (10.4, -7.2), (4.6, -6.8)], 2.2, bevel=0.2,
                    mat=lg.BLACK, round=0.4)
    g.add(B, lg.cut(guard, [lg.cbox(5.0, 10.8, -6.4, -3.6, -2, 2)]))
    g.add(B, lg.slab([(7.2, -3.4), (8.1, -3.4), (8.0, -4.4), (7.6, -5.4), (7.0, -5.6), (7.4, -4.4)], 0.5, bevel=0.1,
                     mat=lg.BLACK))
    g.add(B, lg.slab([(0.6, -3.2), (5.2, -3.8), (3.6, -15.0), (2.4, -15.6), (-1.0, -15.0), (-1.4, -13.6)], 3.0,
                     bevel=0.7, segments=3, mat=lg.POLY_BLACK, round=0.6))
    # Slotted tan handguard with a rail beneath, front ghost-ring post; barrel and breaching muzzle.
    hg = lg.slab([(28, 1.6), (52, 1.6), (53, 0.8), (53, -3.2), (52, -3.8), (28, -3.8)], 4.4, bevel=0.6, segments=3,
                 mat=lg.POLY_TAN, round=0.6)
    g.add(B, lg.cut(hg, [lg.cslab([(u, -2.6), (u + 3.0, -2.6), (u + 3.0, -1.2), (u, -1.2)], w0, w1, round=0.5)
                         for u in (30.5, 35.5, 40.5, 45.5) for w0, w1 in ((-3, -1.8), (1.8, 3))]))
    g.add(B, lg.rail(31, 50, -3.8, width=2.0, mat=lg.BLACK, down=True))
    g.add(B, lg.slab([(48.4, 1.6), (51.0, 1.6), (50.6, 4.4), (48.8, 4.4)], 2.6, bevel=0.2, mat=lg.BLACK, round=0.3))
    g.add(B, lg.tube(28, 74, 1.2, segments=20, mat=lg.BLACK))
    muzzle = lg.turned([(0, 1.6), (5.6, 1.6), (6.0, 1.4)], 74, segments=20, mat=lg.BLACK)
    g.add(B, lg.cut(muzzle, [lg.cbox(78.6, 80.6, 0.9, 2.2, -0.5, 0.5, rotate_u=a) for a in (0, 90, 180, 270)]))
    # Rear ghost-ring sight on the rail.
    g.add(B, lg.slab([(1.5, 3.55), (5.0, 3.55), (4.6, 6.6), (2.0, 6.6)], 2.6, bevel=0.2, mat=lg.BLACK, round=0.4))
    # Collapsible tan stock on a buffer tube.
    g.add(B, lg.tube(-20, 0, 1.5, v=0.3, mat=lg.BLACK))
    stock = lg.slab([(-10, 2.6), (-10, -1.6), (-13, -2.4), (-24, -9.6), (-26.6, -9.6), (-27, -8.6), (-27, 3.4),
                     (-25, 3.8), (-13, 3.4)], 3.4, bevel=0.6, segments=3, mat=lg.POLY_TAN, round=0.8)
    g.add(B, lg.cut(stock, [lg.cslab([(-23.5, -6.6), (-16, -3.2), (-23.5, -3.2)], -2, 2, round=0.8)]))
    g.add(B, lg.slab([(-27, -9.6), (-28.3, -9.4), (-28.3, 3.6), (-27, 3.4)], 3.8, bevel=0.4, mat=lg.RUBBER, round=0.4))
    # Wide double-stack box magazine.
    outline = lg.curved_outline([(22.6, -3.4), (23.4, -12.0), (25.2, -21.0)], [(13.2, -3.4), (13.8, -12.0),
                                                                               (15.4, -20.4)])
    g.add(M, lg.slab(outline, 4.0, bevel=0.35, mat=lg.POLY_BLACK))
    for k in range(3):
        v = -15.0 - k * 1.5
        g.add(M, lg.box(14.4 + k * 0.3, 24.2 + k * 0.4, v - 0.4, v + 0.4, 4.3, bevel=0.2, mat=lg.POLY_BLACK))
    g.add(M, lg.box(15.0, 25.8, -22.0, -20.6, 4.4, bevel=0.3, mat=lg.POLY_BLACK))
    g.socket(B, 'Grip', (2.0, -9.0))
    g.socket(B, 'Foregrip', (41.0, -3.0))
    g.socket(B, 'Muzzle', (80.0, 0.0))
    g.socket(B, 'Magazine', (17.9, -3.4))
    return g.build(origins={'Magazine': (17.9, -3.4)})


# --- KestrelS: twin-tube bullpup pump in the AR's shell ---

def kestrel_s():
    g = lg.Gun('KestrelS')
    B, P = 'Body', 'Pump'
    shell = lg.slab([(1.2, -9.6), (0, -8.6), (0, 2.0), (2, 3.4), (31, 3.2), (33.5, 2.0), (34.2, -1.6), (32.5, -3.0),
                     (9.0, -3.4), (3, -8.6)], 5.4, bevel=1.1, segments=3, mat=lg.POLY_SAND, round=1.0)
    g.add(B, shell)
    loop = lg.slab([(24.6, -2.8), (25.2, -11.0), (24.0, -13.0), (21.8, -13.8), (13.6, -14.0), (11.4, -13.6),
                    (10.4, -11.5), (9.6, -6.5), (9.4, -2.8)], 3.6, bevel=0.9, segments=3, mat=lg.POLY_SAND, round=1.0)
    g.add(B, lg.cut(loop, [lg.cslab([(14.2, -4.8), (21.8, -4.8), (22.8, -6.0), (23.0, -10.8), (21.6, -12.0),
                                     (16.2, -12.0), (14.8, -10.8), (13.8, -6.4)], -3, 3, round=1.0)]))
    g.add(B, lg.slab([(16.0, -4.8), (17.0, -4.8), (16.8, -6.0), (16.4, -7.0), (15.8, -7.2), (16.1, -6.0)], 0.6,
                     bevel=0.12, mat=lg.BLACK))
    for side in (-1.0, 1.0):
        g.add(B, lg.slab([(4, -5.6), (15.6, -4.8), (17.0, 1.8), (5.4, 1.8)], 0.3, w=side * 2.75, bevel=0.1,
                         mat=lg.POLY_GREY, round=0.8))
        for u, v in ((5.6, -4.6), (14.8, -4.0), (6.6, 1.0), (16.0, 1.0)):
            g.add(B, lg.rivet(u, v, side * 2.92, side, r=0.28, mat=lg.BLACK))
        g.add(B, lg.box(18.0, 31.0, -1.75, -1.5, 0.08, w=side * 2.73, bevel=0.0, mat=ACCENT))
    g.add(B, lg.rail(5, 31, 3.3, mat=lg.BLACK))
    g.add(B, lg.slab([(0.2, -8.8), (-1.6, -8.8), (-1.6, 2.0), (0.2, 2.0)], 5.6, bevel=0.5, mat=lg.RUBBER, round=0.6))
    # Flip-up sights on the rail.
    g.add(B, lg.slab([(6, 4.25), (9, 4.25), (8.6, 6.4), (7.0, 6.4)], 2.0, bevel=0.15, mat=lg.BLACK, round=0.3))
    g.add(B, lg.slab([(28.6, 4.25), (31, 4.25), (30.6, 6.8), (29.2, 6.8)], 1.8, bevel=0.15, mat=lg.BLACK, round=0.3))
    # Barrel and choke; two windowed magazine tubes beneath with red shells showing, joined by an end cap.
    g.add(B, lg.tube(33.5, 58, 1.2, segments=20, mat=lg.BLACK))
    g.add(B, lg.turned([(0, 1.35), (2.0, 1.35), (2.3, 1.2)], 58, segments=20, mat=lg.BLACK))
    for w in (-1.3, 1.3):
        t = lg.tube(24, 56, 1.25, v=-3.5, w=w, segments=16, mat=lg.BLACK)
        side = 1.0 if w > 0 else -1.0
        w0, w1 = sorted((w + side * 0.4, w + side * 2.0))
        g.add(B, lg.cut(t, [lg.cbox(47.0, 52.5, -4.1, -2.9, w0, w1)]))     # a window onto the shells
        g.add(B, lg.tube(24, 55.6, 1.0, v=-3.5, w=w, segments=12, mat=SHELL))
    g.add(B, lg.slab([(55.6, -5.0), (57.8, -5.0), (57.8, 1.0), (55.6, 1.0)], 5.4, bevel=0.4, mat=lg.BLACK, round=0.6))
    # The pump: a grey grooved forend around both tubes with a hand stop, action bars back into the shell.
    pump = lg.slab([(34.6, -1.4), (46, -1.4), (46.6, -2.6), (46.2, -6.4), (44.8, -7.2), (35.4, -7.2), (34.2, -6.2),
                    (34.2, -2.4)], 5.8, bevel=1.0, segments=3, mat=lg.POLY_GREY, round=0.9)
    g.add(P, stock_grooves(pump, 36.5, 43.5, (-3.0, -3.9, -4.8), 2.9))
    g.add(P, lg.slab([(43.6, -7.0), (46.2, -7.0), (46.8, -9.4), (45.0, -10.2), (43.8, -9.4)], 3.0, bevel=0.6,
                     segments=3, mat=lg.POLY_GREY, round=0.6))
    for side in (-1.0, 1.0):
        g.add(P, lg.box(30.0, 35.0, -2.2, -1.8, 0.3, w=side * 2.5, bevel=0.05, mat=lg.STEEL))
    g.socket(B, 'Grip', (12.0, -9.0))
    g.socket(B, 'Foregrip', (40.0, -4.5))
    g.socket(B, 'Muzzle', (60.3, 0.0))
    return g.build()


# --- Thunderdrum: salvaged full-auto drum shotgun ---

def thunderdrum():
    g = lg.Gun('Thunderdrum')
    B, M = 'Body', 'Magazine'
    g.add(B, lg.slab([(0, -4.0), (40, -4.0), (40, 3.4), (38, 4.2), (2, 4.2), (0, 3.0)], 4.6, bevel=0.15,
                     mat=lg.SALVAGE))
    for side in (-1.0, 1.0):
        w = side * 2.4
        g.add(B, lg.slab([(3, -3.2), (36, -3.2), (36.5, 3.2), (3, 3.2)], 0.3, w=w, bevel=0.06, mat=lg.SALVAGE))
        for u in (4, 13, 22, 31, 35.4):
            for v in (-2.4, 2.4):
                g.add(B, lg.rivet(u, v, w + side * 0.15, side, r=0.34, mat=lg.STEEL))
        g.add(B, lg.pipe([(0.5, 3.8, w * 0.97), (20, 3.9, w * 0.97), (39.5, 3.8, w * 0.97)], 0.22, mat=lg.STEEL, sides=6))
    handle = lg.slab([(8, 4.2), (30, 4.2), (30, 7.6), (28, 8.4), (10, 8.4), (8, 7.6)], 2.2, bevel=0.3, mat=lg.SALVAGE,
                     round=0.6)
    g.add(B, lg.cut(handle, [lg.cslab([(11, 4.6), (27, 4.6), (27, 6.9), (11, 6.9)], -2, 2, round=0.8)]))
    g.add(B, lg.slab([(9, 8.3), (11.4, 8.3), (11.2, 10.2), (9.4, 10.2)], 1.6, bevel=0.2, mat=lg.STEEL, round=0.3))   # peep sight
    g.add(B, lg.slab([(6, -4.0), (22, -4.0), (21, -6.2), (7, -6.2)], 3.6, bevel=0.3, mat=lg.STEEL, round=0.4))
    g.add(B, lg.strip([(8.6, -6.2), (8.8, -8.6), (10.8, -9.6), (16.4, -9.4), (18.0, -6.2)], 1.0, 0.45, mat=lg.STEEL))
    g.add(B, lg.slab([(11.0, -6.2), (12.0, -6.2), (11.8, -7.4), (11.4, -8.4), (10.7, -8.6), (11.1, -7.4)], 0.6,
                     bevel=0.1, mat=lg.STEEL))
    g.add(B, lg.slab([(4.6, -6.0), (9.8, -6.0), (8.0, -17.0), (6.8, -17.8), (3.4, -17.6), (2.6, -16.2)], 3.4,
                     bevel=0.8, segments=3, mat=lg.TAPE, round=0.6))
    g.add(B, lg.pin(30.0, 1.0, -4.2, -2.3, 0.4, mat=lg.STEEL))                    # charging knob
    g.add(B, lg.pin(30.0, 1.0, -5.2, -4.1, 0.8, mat=lg.STEEL, profile=[(0, 0.3), (0.3, 0.8), (0.8, 0.8), (1.1, 0.4)]))
    # Ported shroud over the barrel, a box brake, a taped vertical foregrip.
    shroud = lg.tube(40, 70, 2.0, segments=24, mat=lg.BLACK)
    holes = []
    for u in (43.0, 47.0, 51.0, 55.0, 59.0, 63.0):
        holes += [lg.cpin(u, 0.0, -3.0, 3.0, 0.7), lg.upright(u + 2.0, -3.0, 3.0, 0.7)]
    g.add(B, lg.cut(shroud, holes))
    g.add(B, lg.tube(39, 71, 1.15, mat=lg.STEEL))
    for u in (40.0, 69.0):
        g.add(B, lg.tube(u, u + 1.2, 2.25, segments=24, mat=lg.STEEL))
    g.add(B, lg.slab([(68.6, 2.2), (70.4, 2.2), (70.0, 4.4), (69.0, 4.4)], 0.6, bevel=0.1, mat=lg.STEEL))   # front post
    brake = lg.box(71, 79, -2.0, 2.0, 4.6, bevel=0.4, mat=lg.SALVAGE)
    g.add(B, lg.cut(brake, [lg.cbox(u, u + 1.1, -1.5, 1.5, -3, 3) for u in (72.2, 74.4, 76.6)]))
    g.add(B, lg.tube(53.5, 56.5, 2.3, segments=24, mat=lg.STEEL))
    g.add(B, lg.upright(55.0, -12.0, -2.0, 1.4, mat=lg.TAPE, segments=12,
                        profile=[(0, 1.2), (0.6, 1.5), (8.8, 1.35), (10.0, 1.0)]))
    # An old walnut stock bolted on with two straps, a rubber pad.
    stock = lg.slab([(0.5, 3.0), (0.5, -4.0), (-5, -5.0), (-29, -10.6), (-31, -10.4), (-31, 2.6), (-28, 3.2),
                     (-6, 3.6)], 3.8, bevel=1.0, segments=3, mat=lg.WALNUT, round=1.0)
    g.add(B, stock)
    for u in (-3.0, -8.0):
        g.add(B, lg.slab([(u - 0.8, -5.6 + u * 0.06), (u + 0.8, -5.4 + u * 0.06), (u + 0.8, 4.0), (u - 0.8, 4.0)], 4.3,
                         bevel=0.15, mat=lg.STEEL))
    g.add(B, lg.slab([(-31, -10.6), (-33, -10.4), (-33, 2.8), (-31, 2.6)], 4.0, bevel=0.5, mat=lg.RUBBER, round=0.5))
    # A 20-round drum on a feed neck.
    g.add(M, lg.slab([(22.6, -4.0), (29.4, -4.0), (30.0, -8.6), (22.0, -8.6)], 3.0, bevel=0.2, mat=lg.SALVAGE))
    g.add(M, lg.pin(26.0, -16.6, -2.9, 2.9, 8.0, segments=32, mat=lg.SALVAGE,
                    profile=[(0, 7.2), (0.5, 8.0), (5.3, 8.0), (5.8, 7.2)]))
    for side in (-1.0, 1.0):
        w0 = 2.9 if side > 0 else -3.5
        g.add(M, lg.pin(26.0, -16.6, w0, w0 + 0.6, 2.8, segments=20, mat=lg.STEEL))
        for k in range(8):
            a = math.radians(22.5 + 45.0 * k)
            g.add(M, lg.rivet(26.0 + math.cos(a) * 5.4, -16.6 + math.sin(a) * 5.4, side * 2.92, side, r=0.34,
                              mat=lg.STEEL))
    g.socket(B, 'Grip', (6.5, -11.0))
    g.socket(B, 'Foregrip', (55.0, -7.0))
    g.socket(B, 'Muzzle', (79.0, 0.0))
    g.socket(B, 'Magazine', (26.0, -4.0))
    return g.build(origins={'Magazine': (26.0, -4.0)})


SHOTGUNS = {'Farmhand': farmhand, 'Coachman': coachman, 'Breacher': breacher, 'KestrelS': kestrel_s,
            'Thunderdrum': thunderdrum}

built = {}
for k, (name, make) in enumerate(SHOTGUNS.items()):
    built[name] = make()
    for obj in built[name].values():
        obj.location.z += k * 0.45   # stacked in the scene; positions don't matter to the export

if lt.want_preview():
    argv = sys.argv[sys.argv.index('--') + 1:]
    wanted = [a for a in argv if a in SHOTGUNS] or list(SHOTGUNS)
    for name in wanted:
        lt.preview(list(built[name].values()), lt.preview_path('Backlog', name), view=(-1.0, -0.25, 0.25), ground=False,
                   lens=60.0, fit=0.95)
