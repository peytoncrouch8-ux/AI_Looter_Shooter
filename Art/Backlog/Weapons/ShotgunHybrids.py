"""Three shotguns mixing the Kestrel-S and the Farmhand (Shotguns.py), for the user to choose from (2026-10-01). Kept for
later in Art/Backlog (see its README): nothing in the game uses them.

  Heartwood     the Kestrel's bullpup layout in the Farmhand's materials: a walnut shell and grip loop, blued side
                plates with brass rivets, a raised rib and brass bead instead of the rail, two blued windowed
                magazine tubes, a grooved walnut pump with a hand stop, a leather recoil pad and shell cuff.
  Ranchhand     the Farmhand's layout in the Kestrel's styling: a blued receiver with grey side plates, rivets, the
                rarity accent line and a Picatinny rail with flip sights; the vent-rib barrel over a tube magazine and
                the grooved walnut pump; a white pistol-grip stock opened up like the Kestrel's skeleton stock, a
                grey shell cuff on it.
  Homesteader   half and half: the Kestrel's white bullpup shell, rail and accent line at the back, with a walnut
                cheek rest and a leather shell cuff; the Farmhand's front: one big blued tube magazine, a vent-rib
                barrel with a brass bead and the grooved walnut pump.

Models and sockets as in Shotguns.py: <Gun>Body and <Gun>Pump (slides back along -X to cycle), in the gun's own space
(the bullpups' origin is the back of the butt, the Ranchhand's the back of its receiver); SOCKET_Grip, SOCKET_Foregrip
and SOCKET_Muzzle on the body.

    blender -b --factory-startup --python Art/Backlog/Weapons/ShotgunHybrids.py -- --preview [Heartwood ...]
"""
import sys

import looter_guns as lg
import looter_textures as lt

ACCENT = lg.glow('GunAccentGlow', 0xe8e8e8, 2.0)    # glows in the gun's rarity color in the game
SHELL = lg.material('GunShellRed', 'Polymer', 0xb8402f)
LEATHER = lg.material('GunLeather', 'Polymer', 0x7a5236)
WALNUT_DARK = lg.material('GunWalnutDark', 'GunWood', 0xa48670)   # oiled, for the Heartwood's big wooden faces


# --- Shared pieces ---

def shell_upright(g, model, u, v0, w):
    """A shotgun shell standing along v: brass head at the bottom, red hull above."""
    g.add(model, lg.upright(u, v0, v0 + 1.1, 1.02, w=w, mat=lg.BRASS, segments=12))
    g.add(model, lg.upright(u, v0 + 1.0, v0 + 6.0, 0.95, w=w, mat=SHELL, segments=12,
                            profile=[(0, 0.95), (4.8, 0.95), (5.0, 0.8)]))


def shell_cuff(g, model, u0, u1, v_bottom, v_top, slope, width, mat, shells_u, shells_v0, side_w):
    """A cuff wrapped round a stock (its bottom edge following the stock's slope), with shells on the right side."""
    g.add(model, lg.slab([(u0, v_bottom), (u1, v_bottom + slope), (u1, v_top + slope * 0.2), (u0, v_top)], width,
                         bevel=0.6, segments=3, mat=mat, round=0.8))
    for k, u in enumerate(shells_u):
        shell_upright(g, model, u, shells_v0 + k * slope / max(len(shells_u) - 1, 1) * 0.8, side_w)


def grooved(outline, width, u0, u1, v_list, mat, round=0.8):
    """A pump forend with shallow finger grooves along both sides."""
    obj = lg.slab(outline, width, bevel=1.0, segments=3, mat=mat, round=round)
    half = width * 0.5
    cuts = []
    for v in v_list:
        cuts += [lg.cbox(u0, u1, v - 0.13, v + 0.13, half - 0.35, half + 1.0),
                 lg.cbox(u0, u1, v - 0.13, v + 0.13, -half - 1.0, -half + 0.35)]
    return lg.cut(obj, cuts)


def vent_rib(g, model, u0, u1, v, bead_u):
    g.add(model, lg.box(u0, u1, v - 0.05, v + 0.5, 0.7, bevel=0.1, mat=lg.BLUED))
    for u in range(int(u0) + 3, int(u1) - 1, 3):
        g.add(model, lg.box(u, u + 1.0, v - 0.15, v + 0.05, 0.72, bevel=0.0, mat=lg.STEEL))
    g.add(model, lg.upright(bead_u, v + 0.45, v + 0.85, 0.22, mat=lg.BRASS, segments=8))


def bullpup_shell(g, B, shell_mat, loop_hole=True):
    """The Kestrel-S shell: butt, receiver and nose, and the thumbhole grip loop, with its trigger."""
    g.add(B, lg.slab([(1.2, -9.6), (0, -8.6), (0, 2.0), (2, 3.4), (31, 3.2), (33.5, 2.0), (34.2, -1.6), (32.5, -3.0),
                      (9.0, -3.4), (3, -8.6)], 5.4, bevel=1.1, segments=3, mat=shell_mat, round=1.0))
    loop = lg.slab([(24.6, -2.8), (25.2, -11.0), (24.0, -13.0), (21.8, -13.8), (13.6, -14.0), (11.4, -13.6),
                    (10.4, -11.5), (9.6, -6.5), (9.4, -2.8)], 3.6, bevel=0.9, segments=3, mat=shell_mat, round=1.0)
    g.add(B, lg.cut(loop, [lg.cslab([(14.2, -4.8), (21.8, -4.8), (22.8, -6.0), (23.0, -10.8), (21.6, -12.0),
                                     (16.2, -12.0), (14.8, -10.8), (13.8, -6.4)], -3, 3, round=1.0)]))
    g.add(B, lg.slab([(16.0, -4.8), (17.0, -4.8), (16.8, -6.0), (16.4, -7.0), (15.8, -7.2), (16.1, -6.0)], 0.6,
                     bevel=0.12, mat=lg.BLACK))


def side_plates(g, B, mat, rivet_mat, u0=4.0, accent=True):
    for side in (-1.0, 1.0):
        g.add(B, lg.slab([(u0, -5.6 + (u0 - 4.0) * 0.07), (15.6, -4.8), (17.0, 1.8), (u0 + 1.4, 1.8)], 0.3,
                         w=side * 2.75, bevel=0.1, mat=mat, round=0.8))
        for u, v in ((u0 + 1.6, -4.4), (14.8, -4.0), (u0 + 2.6, 1.0), (16.0, 1.0)):
            g.add(B, lg.rivet(u, v, side * 2.92, side, r=0.28, mat=rivet_mat))
        if accent:
            g.add(B, lg.box(18.0, 31.0, -1.75, -1.5, 0.08, w=side * 2.73, bevel=0.0, mat=ACCENT))


# --- Heartwood: the Kestrel layout in walnut and blued steel ---

def heartwood():
    g = lg.Gun('Heartwood')
    B, P = 'Body', 'Pump'
    bullpup_shell(g, B, WALNUT_DARK)
    side_plates(g, B, lg.BLUED, lg.BRASS, u0=9.0)
    # A raised blued rib along the top with a notch sight, carried on as the barrel's vent rib.
    g.add(B, lg.box(5, 31, 3.2, 3.8, 1.2, bevel=0.15, mat=lg.BLUED))
    g.add(B, lg.slab([(6.0, 3.8), (8.6, 3.8), (8.2, 5.0), (6.4, 5.0)], 1.6, bevel=0.15, mat=lg.BLUED, round=0.3))
    g.add(B, lg.slab([(0.2, -8.8), (-1.8, -8.8), (-1.8, 2.0), (0.2, 2.0)], 5.6, bevel=0.6, mat=LEATHER, round=0.7))
    shell_cuff(g, B, 0.8, 8.4, -8.6, 2.3, 2.2, 6.0, LEATHER, (2.2, 4.0, 5.8, 7.6), -5.6, -3.15)
    g.add(B, lg.tube(33.5, 60, 1.2, segments=20, mat=lg.BLUED))
    vent_rib(g, B, 34.2, 60, 1.15, 59.2)
    for w in (-1.3, 1.3):
        t = lg.tube(24, 56, 1.25, v=-3.5, w=w, segments=16, mat=lg.BLUED)
        side = 1.0 if w > 0 else -1.0
        w0, w1 = sorted((w + side * 0.4, w + side * 2.0))
        g.add(B, lg.cut(t, [lg.cbox(47.0, 52.5, -4.1, -2.9, w0, w1)]))
        g.add(B, lg.tube(24, 55.6, 1.0, v=-3.5, w=w, segments=12, mat=SHELL))
    g.add(B, lg.slab([(55.6, -5.0), (57.6, -5.0), (57.6, 1.0), (55.6, 1.0)], 5.4, bevel=0.4, mat=lg.BLUED, round=0.6))
    g.add(P, grooved([(34.6, -1.4), (46, -1.4), (46.6, -2.6), (46.2, -6.4), (44.8, -7.2), (35.4, -7.2), (34.2, -6.2),
                      (34.2, -2.4)], 5.8, 36.5, 43.5, (-3.0, -3.9, -4.8), WALNUT_DARK, round=0.9))
    g.add(P, lg.slab([(43.6, -7.0), (46.2, -7.0), (46.8, -9.4), (45.0, -10.2), (43.8, -9.4)], 3.0, bevel=0.6,
                     segments=3, mat=WALNUT_DARK, round=0.6))
    for side in (-1.0, 1.0):
        g.add(P, lg.box(30.0, 35.0, -2.2, -1.8, 0.3, w=side * 2.5, bevel=0.05, mat=lg.STEEL))
    g.socket(B, 'Grip', (12.0, -9.0))
    g.socket(B, 'Foregrip', (40.0, -4.5))
    g.socket(B, 'Muzzle', (60.0, 0.0))
    return g.build()


# --- Ranchhand: the Farmhand layout in the Kestrel's styling ---

def ranchhand():
    g = lg.Gun('Ranchhand')
    B, P = 'Body', 'Pump'
    recv = lg.slab([(0, -3.0), (20, -3.0), (20, 1.4), (18.5, 2.2), (1.5, 2.2), (0, 1.6)], 3.0, bevel=0.25,
                   mat=lg.BLUED, round=0.4)
    g.add(B, lg.cut(recv, [lg.cbox(6, 14, -0.8, 1.2, -2.0, -1.0)]))
    g.add(B, lg.box(6.2, 13.8, -0.6, 1.0, 0.6, w=-0.9, bevel=0.1, mat=lg.STEEL))
    for side in (-1.0, 1.0):
        if side > 0:   # the grey plate on the left; the port shows on the right
            g.add(B, lg.slab([(2, -2.4), (17, -2.4), (18, 1.2), (3, 1.2)], 0.3, w=1.65, bevel=0.08, mat=lg.POLY_GREY,
                             round=0.6))
        else:
            g.add(B, lg.slab([(1.2, -2.4), (5.4, -2.4), (5.6, 1.2), (1.8, 1.2)], 0.3, w=-1.65, bevel=0.08,
                             mat=lg.POLY_GREY, round=0.5))
            g.add(B, lg.slab([(14.6, -2.4), (18.6, -2.4), (18.6, 1.2), (14.8, 1.2)], 0.3, w=-1.65, bevel=0.08,
                             mat=lg.POLY_GREY, round=0.5))
        g.add(B, lg.box(3.0, 18.0, -2.85, -2.62, 0.08, w=side * 1.53, bevel=0.0, mat=ACCENT))
    for u, v in ((2.2, -1.6), (4.6, 0.6), (15.4, -1.6), (17.8, 0.6)):
        g.add(B, lg.rivet(u, v, -1.82, -1.0, r=0.24, mat=lg.BLACK))
    g.add(B, lg.rail(1, 19.5, 2.2, mat=lg.BLACK))
    g.add(B, lg.slab([(1.5, 3.15), (4.5, 3.15), (4.1, 5.2), (2.5, 5.2)], 2.0, bevel=0.15, mat=lg.BLACK, round=0.3))
    g.add(B, lg.slab([(3, -3.0), (13, -3.0), (12.4, -4.6), (4, -4.4)], 2.4, bevel=0.2, mat=lg.BLUED, round=0.3))
    g.add(B, lg.strip([(4.6, -4.4), (5.0, -6.8), (7.0, -7.6), (11.0, -7.2), (12.4, -4.6)], 0.8, 0.3, mat=lg.BLUED))
    g.add(B, lg.slab([(7.4, -4.4), (8.4, -4.4), (8.2, -5.6), (7.8, -6.6), (7.2, -6.8), (7.5, -5.6)], 0.5, bevel=0.1,
                     mat=lg.BLUED))
    g.add(B, lg.tube(20, 66, 1.15, segments=20, mat=lg.BLUED))
    vent_rib(g, B, 20, 66, 1.1, 65.0)
    g.add(B, lg.tube(20, 58, 1.05, v=-2.5, segments=16, mat=lg.BLUED))
    g.add(B, lg.turned([(0, 1.2), (1.6, 1.2), (2.0, 0.9)], 58, v=-2.5, segments=16, mat=lg.BLUED))
    g.add(B, lg.slab([(56.5, -3.4), (58.5, -3.4), (58.5, 0.9), (56.5, 0.9)], 2.2, bevel=0.3, mat=lg.BLUED, round=0.4))
    # A white pistol-grip stock in the Kestrel's shell, opened up like its skeleton stock; a grey shell cuff on the
    # strut above the opening, a rubber pad.
    stock = lg.slab([(0, 1.8), (0, -3.0), (-2.5, -4.4), (-5.5, -7.4), (-9, -8.0), (-12, -7.4), (-37, -12.6),
                     (-38.5, -12.4), (-38.5, 0.2), (-36, 0.6), (-14, 1.0), (-4, 1.8)], 4.2, bevel=1.1, segments=3,
                    mat=lg.POLY_SAND, round=1.0)
    g.add(B, lg.cut(stock, [lg.cslab([(-16.5, -8.0), (-33.0, -10.9), (-34.2, -10.0), (-34.0, -6.4), (-32.5, -5.6),
                                      (-18.0, -4.9)], -3, 3, round=1.0)]))
    g.add(B, lg.slab([(-38.4, -12.7), (-40.2, -12.6), (-40.2, 0.4), (-38.4, 0.3)], 4.4, bevel=0.5, mat=lg.RUBBER,
                     round=0.5))
    shell_cuff(g, B, -30, -20, -5.8, 0.9, 0.3, 4.9, lg.POLY_GREY, (-28.2, -26.0, -23.8, -21.6), -4.9, -2.8)
    g.add(P, grooved([(28, -1.4), (46, -1.4), (47, -2.4), (46.5, -4.0), (45, -4.4), (29, -4.4), (27.5, -3.6),
                      (27.5, -2.0)], 4.4, 31, 44, (-2.2, -2.9, -3.6), lg.WALNUT))
    for side in (-1.0, 1.0):
        g.add(P, lg.box(19.5, 28.5, -2.3, -1.8, 0.3, w=side * 1.2, bevel=0.05, mat=lg.STEEL))
    g.socket(B, 'Grip', (-4.5, -6.0))
    g.socket(B, 'Foregrip', (37.0, -3.0))
    g.socket(B, 'Muzzle', (66.0, 0.0))
    return g.build()


# --- Homesteader: the Kestrel's white back half, the Farmhand's front ---

def homesteader():
    g = lg.Gun('Homesteader')
    B, P = 'Body', 'Pump'
    bullpup_shell(g, B, lg.POLY_SAND)
    side_plates(g, B, lg.BLUED, lg.STEEL, u0=9.0)
    g.add(B, lg.rail(9, 31, 3.3, mat=lg.BLACK))
    g.add(B, lg.slab([(10, 4.25), (13, 4.25), (12.6, 6.4), (11.0, 6.4)], 2.0, bevel=0.15, mat=lg.BLACK, round=0.3))
    g.add(B, lg.slab([(0.4, 2.0), (8.6, 2.9), (9.0, 4.6), (0.8, 4.4)], 4.4, bevel=0.6, segments=3, mat=lg.WALNUT,
                     round=0.9))                                                  # walnut cheek rest
    g.add(B, lg.slab([(0.2, -8.8), (-1.6, -8.8), (-1.6, 2.0), (0.2, 2.0)], 5.6, bevel=0.5, mat=lg.RUBBER, round=0.6))
    shell_cuff(g, B, 0.8, 8.4, -8.6, 1.9, 2.2, 6.0, LEATHER, (2.2, 4.0, 5.8, 7.6), -5.6, -3.15)
    # The Farmhand's front: blued barrel and vent rib, one big tube magazine, the grooved walnut pump.
    g.add(B, lg.tube(33.5, 66, 1.15, segments=20, mat=lg.BLUED))
    vent_rib(g, B, 34.2, 66, 1.1, 65.0)
    g.add(B, lg.tube(24, 59, 1.2, v=-2.8, segments=16, mat=lg.BLUED))
    g.add(B, lg.turned([(0, 1.32), (1.6, 1.32), (2.0, 1.0)], 59, v=-2.8, segments=16, mat=lg.BLUED))
    g.add(B, lg.slab([(57.4, -3.8), (59.4, -3.8), (59.4, 0.9), (57.4, 0.9)], 2.3, bevel=0.3, mat=lg.BLUED, round=0.4))
    g.add(P, grooved([(35.0, -1.2), (52, -1.2), (53, -2.4), (52.5, -4.4), (51, -4.9), (36, -4.9), (34.5, -4.0),
                      (34.5, -1.8)], 4.6, 37.5, 50, (-2.3, -3.1, -3.9), lg.WALNUT))
    for side in (-1.0, 1.0):
        g.add(P, lg.box(30.0, 35.5, -2.6, -2.1, 0.3, w=side * 1.3, bevel=0.05, mat=lg.STEEL))
    g.socket(B, 'Grip', (12.0, -9.0))
    g.socket(B, 'Foregrip', (43.0, -3.0))
    g.socket(B, 'Muzzle', (66.0, 0.0))
    return g.build()


HYBRIDS = {'Heartwood': heartwood, 'Ranchhand': ranchhand, 'Homesteader': homesteader}

built = {}
for k, (name, make) in enumerate(HYBRIDS.items()):
    built[name] = make()
    for obj in built[name].values():
        obj.location.z += k * 0.45   # stacked in the scene; positions don't matter to the export

if lt.want_preview():
    argv = sys.argv[sys.argv.index('--') + 1:]
    wanted = [a for a in argv if a in HYBRIDS] or list(HYBRIDS)
    for name in wanted:
        lt.preview(list(built[name].values()), lt.preview_path('Backlog', name), view=(-1.0, -0.25, 0.25), ground=False,
                   lens=60.0, fit=0.95)
