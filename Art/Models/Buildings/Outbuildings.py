"""Ransom's Rest's outbuildings for the home lots (Docs/Work/2026-11-04-Towns/PLAN.md 4.2 item 5; the towns round,
2026-11-04): small sheds, a privy, a smokehouse and a root cellar, so every lot gets one or two beside its house (with the
existing Woodshed, ChickenCoop and RR_Outhouse) and the back lots stop being bare dust. Each is small and instanced
(AInstancedProps, placed by the DRESSING lane): Nanite off with LODs at 50% and 25%, a few simple UCX hulls, 1-3
material slots, built with Tools/Blender/looter_homes.py's kits on the town's own materials (the trim sheet; the
galvanized sheet only here, on outbuildings: the user's call D4).

  RR_Shed_A      a side-gable tool shed, 3.0 x 2.4 m, 2.9 m to the ridge: grey board-and-batten on sills on flat stones,
                 R1 silver shakes, a plank door in its long front, a little shuttered window in its right gable.
  RR_Shed_B      a lean-to shed, 2.4 x 1.8 m, 2.3 m high at its front falling to 1.9 m at the back: rough boards nailed
                 level, a galvanized corrugated roof (G) overhanging the door, a plank door.
  RR_Shed_C      a front-gable shed, 2.6 x 3.0 m, 3.0 m to the ridge: board-and-batten, a galvanized gable roof, a wide
                 plank door in the gable end, a boarded window on each side.
  RR_Privy_B     a two-holer privy, 1.8 x 1.4 m, 2.4 m: upright boards, an R4 tar-paper shed roof, a diamond cut through
                 its door (dark behind), a vent slot under the eave.
  RR_Smokehouse  a hewn-log smokehouse, 2.2 x 2.2 m, 3.3 m: chinked timbers on a fieldstone plinth, steep R1 shakes, a
                 vent cap over the ridge (SOCKET_Smoke there: faint smoke), a low plank door.
  RR_RootCellar  a root cellar under a sod mound, 4.0 x 3.4 m at its foot, 1.45 m high: dry grass (Hay) over the mound
                 and bare earth round its foot (the dugout's), a fieldstone headwall following the mound's edge, a timber
                 frame and a low plank door.

The pivot is the door's middle on the front wall's outer face, at ground level (front -Y, +X in Unreal), as the homes';
each model's report line (HOMES: ...) gives its bounds and walls from the pivot.

  --preview   renders each into Saved/ArtPreviews/Buildings2/ and their lineup (Lineup_Outbuildings.png)
  --only A,SMOKEHOUSE   builds just those (the keys below); --no-ao skips the AO bake; --stats prints where the
              triangles go
"""
import math
import os

import bmesh

import looter_buildings as kit
import looter_homes as homes
import looter_textures as lt
import looter_town as town
from looter_buildings import Matrix, Opening, Space, Tile, Trim, Vector, place

ARGS = homes.args()
ONLY = []
if '--only' in ARGS:
    for arg in ARGS[ARGS.index('--only') + 1:]:
        if arg.startswith('--'):
            break
        ONLY += [k.strip().upper() for k in arg.split(',') if k.strip()]
SMALL = dict(Nanite=0, LODs='50,25')         # every outbuilding: no Nanite, LODs at 50% and 25%
LAP = homes.LAP
PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'Buildings2')
wall_spaces, rect, gable_outline, notch, clad = homes.wall_spaces, homes.rect, homes.gable_outline, homes.notch, homes.clad


def stones_and_sills(m, x0, x1, y0, y1, base, xs=None):
    """A light frame building's footing: flat fieldstones under its corners (and xs between) and hewn sills round its
    foot on them, its floor at base."""
    rng = m.rng
    xs = [x0 + 0.14, x1 - 0.14] if xs is None else xs
    for x in xs:
        for y in (y0 + 0.14, y1 - 0.14):
            kit.stone_stack(m, (x, y, -0.15), (0.32, 0.3), base + 0.08, taper=0.12, rot=rng.uniform(-7.0, 7.0),
                            bevel=0.02)
    for space, length in wall_spaces(x0, x1, y0, y1, 0.0):
        m.board((0.0, 0.06, base - 0.07), (length, 0.06, base - 0.07), 0.14, 0.12, uv=Trim('C', lane='each'),
                space=space)


def level_boards(m, space, outline, items, look='A', h=0.22, gone=0.04, out=0.022):
    """Rough boards nailed level over a wall's face (the claim shack's), a little apart, now and then one gone."""
    rng = m.rng
    top = max(z for x, z in outline)
    z = 0.0
    while z < top - 0.05:
        spans = town.intersect(town.spans_at(outline, z + 0.01), town.spans_at(outline, z + h - 0.01))
        for x0, x1, k0, k1 in homes.keep_out(items, casing=0.09):
            if k0 < z + h and k1 > z:
                spans = town.subtract(spans, (x0, x1))
        for a, b in spans:
            if rng.random() < gone:
                continue
            town.board(m, (a - 0.02, 0.0, z + h * 0.5), (b + 0.02, 0.0, z + h * 0.5 + rng.uniform(-0.01, 0.01)),
                       h - 0.025, out, look=look, space=space, lift=out * 0.5, drop=('+y',))
        z += h


def cut_door(m, space, door, cut, out=0.0, look='A'):
    """A privy's plank door in opening door: upright planks with a shape cut through (cut: an outline (x, z)), a Z
    brace, strap hinges, a turn-button and the dark behind the cut (RR_Outhouse's)."""
    leaf = kit.rect(door.x + 0.01, 0.0, door.w - 0.02, door.h)
    m.panel(leaf, [cut], 0.04, Trim(look, world=True, rotate=True), space=space,
            matrix=Matrix.Translation((0.0, 0.015, 0.0)))
    cz = sum(z for x, z in cut) / len(cut)
    homes.quad(m, [(door.x, 0.4, cz - 0.2), (door.x + door.w, 0.4, cz - 0.2), (door.x + door.w, 0.4, cz + 0.25),
                   (door.x, 0.4, cz + 0.25)], Trim('A', world=True, rotate=True), space=space, facing=(0.0, -1.0, 0.0))
    for z in (0.3, door.h - 0.7):
        m.board((door.x + 0.05, -0.015, z), (door.x + door.w - 0.05, -0.015, z), 0.12, 0.03, uv=look, space=space)
    m.board((door.x + door.w - 0.1, -0.015, 0.36), (door.x + 0.1, -0.015, door.h - 0.76), 0.1, 0.025, uv=look,
            space=space)
    for z in (0.3, door.h - 0.7):
        m.board((door.x - 0.04, -0.04, z), (door.x + 0.34, -0.04, z), 0.06, 0.012, uv=Trim('H3', fit=True), space=space)
    m.box((0.04, 0.04, 0.12), at=(door.x + door.w - 0.03, -0.035, 1.0), rot=(0.0, 25.0, 0.0), uv='C', space=space)
    for x in (door.x - 0.05, door.x + door.w + 0.05):
        kit.trim_board(m, space, (x, 0.0, -0.02), (x, 0.0, door.h + 0.08), 0.1, 'C', thick=0.035 + out)
    kit.trim_board(m, space, (door.x - 0.1, 0.0, door.h + 0.06), (door.x + door.w + 0.1, 0.0, door.h + 0.06), 0.1, 'C',
                   thick=0.035 + out)


def eave_fascia(m, x0, x1, y, z, look='C'):
    """A fascia board along a shed roof's high edge (the claim shack's)."""
    m.board((x0, y, z), (x1, y, z), 0.18, 0.04, uv=look)


# --- RR_Shed_A: the side-gable tool shed ---

def shed_a():
    m = homes.Home('RR_Shed_A', seed=601)
    X0, X1, Y0, Y1 = -1.5, 1.5, -1.2, 1.2
    BASE, WALL, PITCH = 0.16, 1.95, 33.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (Y1 - Y0) * 0.5 * tan
    RIDGE_Z = EAVE + RISE
    stones_and_sills(m, X0, X1, Y0, Y1, BASE, xs=(X0 + 0.14, 0.0, X1 - 0.14))
    m.section('footing')
    DOOR = Opening('door', 1.75, 0.0, 0.9, 1.78, style='plank', finish='A', casing='C', hinge='left')
    win = dict(paint='C', sash='A', shutter='A', casing_w=0.08, style='shut', panes=(2, 1), single=True)
    walls = [([DOOR], False), ([Opening('window', 0.95, 1.05, 0.55, 0.5, **win)], True), ([], False), ([], True)]
    for k, ((space, length), (items, gable)) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls)):
        outline = gable_outline(length, WALL, RISE) if gable else rect(length, WALL)
        clad(m, space, length, outline, items, cladding='batten', look='A', trim='C', wrap=gable, table=False,
             corner_w=0.11)
    m.section('walls')
    front = homes.plane(m, X0, X1, Y0, EAVE, PITCH, (Y1 - Y0) * 0.5, 1.0, overhang=0.28, rake=(0.22, 0.22), deck=0.06,
                        sag=0.03)
    back = homes.plane(m, X0, X1, Y1, EAVE, PITCH, (Y1 - Y0) * 0.5, -1.0, overhang=0.28, rake=(0.22, 0.22), deck=0.06,
                       sag=0.03)
    for slope in (front, back):
        homes.cover(m, slope, 'R1', piece=(1.0, 1.8), missing=0.03)
    homes.ridge(m, front, back, 'R1', width=0.13)
    m.section('roof')
    m.hull((X1 - X0 + 0.06, Y1 - Y0 + 0.06, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    homes.gable_hull(m, X0, X1, Y0, Y1, EAVE, EAVE, 0.0, RIDGE_Z, overhang=0.28, rake=0.22, pitch_front=PITCH)
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1), **SMALL)


# --- RR_Shed_B: the lean-to shed under galvanized sheet ---

def shed_b():
    m = homes.Home('RR_Shed_B', seed=602)
    rng = m.rng
    X0, X1, Y0, Y1 = -1.2, 1.2, -0.9, 0.9
    BASE, FRONT_H, BACK_H = 0.12, 2.12, 1.72
    stones_and_sills(m, X0, X1, Y0, Y1, BASE)
    m.section('footing')
    DOOR = Opening('door', 0.72, 0.0, 0.85, 1.82, style='plank', finish='A', casing='C', hinge='right')
    walls = [
        ([DOOR], rect(X1 - X0, FRONT_H)),
        ([], [(0.0, 0.0), (Y1 - Y0, 0.0), (Y1 - Y0, BACK_H), (0.0, FRONT_H)]),
        ([], rect(X1 - X0, BACK_H)),
        ([], [(0.0, 0.0), (Y1 - Y0, 0.0), (Y1 - Y0, FRONT_H), (0.0, BACK_H)]),
    ]
    for (space, length), (items, outline) in zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls):
        homes.shell(m, notch(outline, items), kit.holes(items), 0.08, Trim('A', world=True, u=rng.uniform(0.0, 6.4)),
                    space, slices=0)
        level_boards(m, space, outline, items)
        homes.openings(m, space, items, 0.08, out=0.022)
        for x in (0.05, length - 0.05):
            m.board((x, -0.035, -0.04), (x, -0.035, (outline[-1][1] if x < 1.0 else outline[2][1]) - 0.02), 0.1, 0.08,
                    uv=Trim('C', lane='each'), space=space)
    m.section('walls')
    # The roof falls to the back; it runs on past the front wall to keep the rain off the door.
    fall = (FRONT_H - BACK_H) / (Y1 - Y0)
    roof = homes.shed_roof(m, X0, X1, Y1, BASE + BACK_H, Y0 - 0.25, BASE + FRONT_H + 0.25 * fall, overhang=0.25,
                           rake=(0.16, 0.16), deck=0.05, sag=0.02)
    homes.cover(m, roof, 'G')
    eave_fascia(m, X0 - 0.16, X1 + 0.16, Y0 - 0.27, BASE + FRONT_H + 0.25 * fall - 0.04)
    m.section('roof')
    m.hull_points([(x, y, z) for x in (X0 - 0.04, X1 + 0.04) for y, z in ((Y0 - 0.04, 0.0), (Y1 + 0.04, 0.0),
                                                                          (Y0 - 0.04, BASE + FRONT_H),
                                                                          (Y1 + 0.04, BASE + BACK_H))])
    homes.slope_hull(m, roof, thick=0.1)
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1), **SMALL)


# --- RR_Shed_C: the front-gable shed under galvanized sheet ---

def shed_c():
    m = homes.Home('RR_Shed_C', seed=603)
    X0, X1, Y0, Y1 = -1.3, 1.3, -1.5, 1.5
    BASE, WALL, PITCH = 0.16, 2.02, 30.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (X1 - X0) * 0.5 * tan
    stones_and_sills(m, X0, X1, Y0, Y1, BASE)
    m.section('footing')
    DOOR = Opening('door', 0.7, 0.0, 1.2, 1.85, style='plank', finish='A', casing='C', hinge='left')
    shut = dict(paint='C', sash='A', casing_w=0.08, style='boarded')
    walls = [([DOOR, Opening('window', 0.95, WALL + 0.12, 0.7, 0.4, paint='C', sash='A', casing_w=0.07,
                             style='shut', shutter='A', panes=(2, 1), single=True)], True),
             ([Opening('window', 1.15, 1.0, 0.65, 0.6, **shut)], False), ([], True),
             ([Opening('window', 1.2, 1.0, 0.65, 0.6, **shut)], False)]
    for (space, length), (items, gable) in zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls):
        outline = gable_outline(length, WALL, RISE) if gable else rect(length, WALL)
        clad(m, space, length, outline, items, cladding='batten', look='A', trim='C', wrap=gable, table=False,
             corner_w=0.11)
    m.section('walls')
    frame = Matrix.Rotation(math.radians(90.0), 4, 'Z')
    right = homes.plane(m, Y0, Y1, -X1, EAVE, PITCH, (X1 - X0) * 0.5, 1.0, overhang=0.26, rake=(0.24, 0.18), deck=0.05,
                        sag=0.03, frame=frame)
    left = homes.plane(m, Y0, Y1, -X0, EAVE, PITCH, (X1 - X0) * 0.5, -1.0, overhang=0.26, rake=(0.24, 0.18), deck=0.05,
                       sag=0.03, frame=frame)
    homes.cover(m, right, 'G', loose=0.2)
    homes.cover(m, left, 'G', gone=0.08)
    homes.ridge(m, right, left, 'G')
    m.section('roof')
    m.hull((X1 - X0 + 0.06, Y1 - Y0 + 0.06, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    m.hull_points([(x, y, EAVE - 0.26 * tan + 0.12) for x in (X0 - 0.26, X1 + 0.26) for y in (Y0 - 0.24, Y1 + 0.18)] +
                  [(0.0, y, EAVE + RISE + 0.18) for y in (Y0 - 0.24, Y1 + 0.18)])
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1), **SMALL)


# --- RR_Privy_B: the two-holer ---

def diamond(cx, cz, rx, rz):
    return [(cx, cz - rz), (cx + rx, cz), (cx, cz + rz), (cx - rx, cz)]


def privy_b():
    m = homes.Home('RR_Privy_B', seed=604)
    rng = m.rng
    X0, X1, Y0, Y1 = -0.9, 0.9, -0.7, 0.7
    BASE, FRONT_H, BACK_H = 0.16, 2.05, 1.7
    for x in (X0 + 0.2, X1 - 0.2):
        kit.stone_stack(m, (x, 0.0, -0.15), (0.42, Y1 - Y0 + 0.16), BASE + 0.13, rot=rng.uniform(-3.0, 3.0), bevel=0.02)
    m.section('footing')
    DOOR = Opening('door', 0.45, 0.0, 0.9, 1.85)
    walls = [
        ([DOOR], rect(X1 - X0, FRONT_H)),
        ([], [(0.0, 0.0), (Y1 - Y0, 0.0), (Y1 - Y0, BACK_H), (0.0, FRONT_H)]),
        ([], rect(X1 - X0, BACK_H)),
        ([], [(0.0, 0.0), (Y1 - Y0, 0.0), (Y1 - Y0, FRONT_H), (0.0, BACK_H)]),
    ]
    spaces = wall_spaces(X0, X1, Y0, Y1, BASE)
    for k, ((space, length), (items, outline)) in enumerate(zip(spaces, walls)):
        homes.shell(m, notch(outline, items), (), 0.05, Trim('A', world=True, rotate=True, u=rng.uniform(0.0, 6.4)),
                    space, around=items)
        homes.corners(m, space, length, outline[-1][1] - 0.02, outline[2][1] - 0.02, look='C', width=0.1, wrap=k % 2 == 0)
    front = spaces[0][0]
    cut_door(m, front, DOOR, diamond(DOOR.x + DOOR.w * 0.5, 1.5, 0.08, 0.12))
    # A vent slot high in each side wall, under the roof, with a drip board.
    for k in (1, 3):
        space, length = spaces[k]
        vz = BACK_H - 0.25
        m.box((0.5, 0.02, 0.09), at=(length * 0.5, 0.03, vz), uv=Trim('H4', fit=True), space=space)
        m.box((0.58, 0.05, 0.03), at=(length * 0.5, -0.03, vz + 0.07), uv='C', space=space)
    m.box((1.0, 0.34, 0.05), at=(DOOR.x + DOOR.w * 0.5 + X0, Y0 - 0.25, -0.01), rot=(0.0, 0.0, 3.0), uv='D', bevel=0.02)
    m.section('walls, door')
    fall = (FRONT_H - BACK_H) / (Y1 - Y0)
    roof = homes.shed_roof(m, X0, X1, Y1, BASE + BACK_H, Y0 - 0.2, BASE + FRONT_H + 0.2 * fall, overhang=0.2,
                           rake=(0.14, 0.14), deck=0.045, sag=0.02)
    homes.cover(m, roof, 'R4', tears=1)
    eave_fascia(m, X0 - 0.14, X1 + 0.14, Y0 - 0.22, BASE + FRONT_H + 0.2 * fall - 0.04)
    m.section('roof')
    m.hull_points([(x, y, z) for x in (X0 - 0.04, X1 + 0.04) for y, z in ((Y0 - 0.04, 0.0), (Y1 + 0.04, 0.0),
                                                                          (Y0 - 0.04, BASE + FRONT_H),
                                                                          (Y1 + 0.04, BASE + BACK_H))])
    homes.slope_hull(m, roof, thick=0.1)
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1), **SMALL)


# --- RR_Smokehouse: hewn logs, a vent cap over the ridge ---

def smokehouse():
    m = homes.Home('RR_Smokehouse', seed=605)
    rng = m.rng
    X0, X1, Y0, Y1 = -1.1, 1.1, -1.1, 1.1
    BASE, WALL, PITCH = 0.22, 1.9, 46.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (Y1 - Y0) * 0.5 * tan
    RIDGE_Z = EAVE + RISE
    homes.plinth(m, X0, X1, Y0, Y1, BASE, out=0.04)
    m.section('plinth')
    DOOR = Opening('door', 0.74, 0.0, 0.72, 1.5, style='plank', finish='A', casing='C', hinge='left')
    items_by_wall = [[DOOR], [], [], []]
    for k, ((space, length), items) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE), items_by_wall)):
        homes.hewn_wall(m, space, length, WALL, items, parity=k % 2, course=0.28, log_h=0.23, depth=0.2, ext=0.12)
        homes.openings(m, space, items, 0.2, out=0.0)
        if k % 2:
            homes.shell(m, [(0.0, WALL), (length, WALL), (length * 0.5, WALL + RISE)], (), 0.05,
                        Trim('A', world=True, rotate=True, u=rng.uniform(0.0, 6.4)),
                        Space(space.matrix @ Matrix.Translation((0.0, 0.08, 0.0))))
    m.section('walls')
    front = homes.plane(m, X0, X1, Y0, EAVE, PITCH, (Y1 - Y0) * 0.5, 1.0, overhang=0.26, rake=(0.2, 0.2), deck=0.06,
                        sag=0.04)
    back = homes.plane(m, X0, X1, Y1, EAVE, PITCH, (Y1 - Y0) * 0.5, -1.0, overhang=0.26, rake=(0.2, 0.2), deck=0.06,
                       sag=0.04)
    for slope in (front, back):
        homes.cover(m, slope, 'R1', piece=(0.9, 1.6), missing=0.03)
    homes.ridge(m, front, back, 'R1', width=0.13)
    # The vent cap: a little gable of boards on four short posts over the ridge, the smoke seeping out under it.
    VZ = RIDGE_Z + 0.04
    for x in (-0.36, 0.36):
        for y in (-0.12, 0.12):
            m.box((0.05, 0.05, 0.2), at=(x, y, VZ + 0.08), uv=Trim('C', lane='each'))
    cap = Matrix.Translation((0.0, 0.0, VZ + 0.18))
    for side in (-1.0, 1.0):
        m.box((0.9, 0.3, 0.025), matrix=cap @ place((0.0, side * 0.12, 0.05), (side * -34.0, 0.0, 0.0)),
              uv=Trim('A', width=0.3))
    m.socket('Smoke', (0.0, 0.0, VZ + 0.12))
    m.section('roof, vent')
    m.hull((X1 - X0 + 0.24, Y1 - Y0 + 0.04, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    homes.gable_hull(m, X0, X1, Y0, Y1, EAVE, EAVE, 0.0, RIDGE_Z, overhang=0.26, rake=0.2, pitch_front=PITCH)
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1), **SMALL)


# --- RR_RootCellar: the root cellar under a sod mound ---

def cellar_z(x, y, top=1.42, xo=2.0, yo=3.4, k=0.95):
    """The mound's height: a plateau over the cellar falling a little to the back, its sides and back sloping away
    (a min of planes, so it is convex and one hull fits it; the dugout's mound_z, smaller)."""
    return min(top - 0.05 * y, k * (xo - abs(x)), k * (yo - y))


def root_cellar():
    m = homes.Home('RR_RootCellar', seed=606)
    rng = m.rng
    xs = [-2.12, -1.7, -1.3, -0.9, -0.5, 0.0, 0.5, 0.9, 1.3, 1.7, 2.12]
    ys = [-0.16, 0.0, 0.6, 1.3, 2.0, 2.6, 3.1, 3.5]

    def z_at(x, y):
        if y < 0.0:
            return cellar_z(x, 0.0) - 0.12 - rng.uniform(0.0, 0.04)
        return max(cellar_z(x, y), -0.25)
    grid = []
    for j, y in enumerate(ys):
        row = []
        for i, x in enumerate(xs):
            z = z_at(x, y)
            if 0.0 < y < 3.3 and 0.0 < z < 1.35:
                z += rng.uniform(-0.04, 0.04)
            row.append(Vector((x + (rng.uniform(-0.06, 0.06) if 0 < i < len(xs) - 1 else 0.0),
                               y + (rng.uniform(-0.06, 0.06) if 1 < j < len(ys) - 1 else 0.0), z)))
        grid.append(row)
    for key, pick in (('hay', lambda zc: zc > 0.26), ('dirt', lambda zc: zc <= 0.26)):
        tb = kit._new_bmesh()
        verts = [[tb.verts.new(p) for p in row] for row in grid]
        for j in range(len(ys) - 1):
            for i in range(len(xs) - 1):
                quad = (verts[j][i], verts[j][i + 1], verts[j + 1][i + 1], verts[j + 1][i])
                if pick(sum(v.co.z for v in quad) / 4.0):
                    tb.faces.new(quad)
        tb.normal_update()
        for f in tb.faces:
            if f.normal.z < 0.0:
                f.normal_flip()
        loose = [v for v in tb.verts if not v.link_faces]
        bmesh.ops.delete(tb, geom=loose, context='VERTS')
        m.emit(tb, homes.Tiled('Hay', 900) if key == 'hay' else Tile('GroundDirt'), key, smooth=True)
    m.section('mound')
    # The headwall: dry stone across the front under the mound's lip, following its edge, the door in its middle.
    XF = 2.0
    steps = [-XF + 2.0 * XF * t / 10.0 for t in range(11)]
    top = [(s + XF, max(cellar_z(s, 0.0) - 0.05, -0.12)) for s in steps]
    DOOR = Opening('door', XF - 0.4, 0.0, 0.8, 1.18, style='plank', finish='A', casing='C', hinge='left')
    front = kit.wall_space((-XF, 0.0), (XF, 0.0), 0.0)
    outline = [(0.0, -0.3), (2.0 * XF, -0.3)] + list(reversed(top))
    homes.shell(m, outline, [kit.rect(DOOR.x, 0.0, DOOR.w, DOOR.h)], 0.35,
                Trim('D', world=True, u=rng.uniform(0.0, 6.4)), front, slices=0.8)
    # Its timber frame: two posts and a lintel, the door hung in it, a flat stone at its sill.
    for x in (DOOR.x - 0.08, DOOR.x + DOOR.w + 0.08):
        m.board((x, -0.06, -0.1), (x, -0.06, DOOR.h + 0.16), 0.15, 0.15, uv=Trim('C', lane='each'), space=front)
    m.board((DOOR.x - 0.26, -0.07, DOOR.h + 0.1), (DOOR.x + DOOR.w + 0.26, -0.07, DOOR.h + 0.1), 0.16, 0.17,
            uv=Trim('C', lane='each'), space=front)
    town.door(m, front, DOOR, 0.35, out=0.0)
    m.box((1.1, 0.42, 0.06), at=(0.0, -0.25, 0.0), rot=(0.0, 0.0, 2.0), uv='D', bevel=0.02)
    m.section('headwall, door')
    pts = []
    for y in (0.0, 0.6, 1.9, 2.6, 3.5):
        for x in (-2.12, -1.2, 0.0, 1.2, 2.12):
            pts.append((x, y, max(cellar_z(x, y), -0.25) + 0.03))
    m.hull_points(pts)
    # The headwall's: under its sloping top (the mound's edge is a min of planes, so the shape is convex).
    m.hull_points([(s, y, max(cellar_z(s, 0.0), 0.0) + 0.02) for s in (-XF, -0.48, 0.48, XF) for y in (-0.08, 0.35)] +
                  [(s, y, -0.1) for s in (-XF, XF) for y in (-0.08, 0.35)])
    return m.finish(pivot=(0.0, 0.0), footprint=(-XF, XF, 0.0, 3.4), **SMALL)


BUILDERS = [('A', shed_a), ('B', shed_b), ('C', shed_c), ('PRIVY', privy_b), ('SMOKEHOUSE', smokehouse),
            ('CELLAR', root_cellar)]
CAPTIONS = {
    'RR_Shed_A': 'A side-gable tool shed: grey battens, silver shakes.',
    'RR_Shed_B': 'A lean-to shed: level boards under galvanized sheet.',
    'RR_Shed_C': 'A front-gable shed: battens, a galvanized gable roof.',
    'RR_Privy_B': 'A two-holer: upright boards, tar paper, a diamond cut.',
    'RR_Smokehouse': 'A hewn-log smokehouse: steep shakes, a vent cap.',
    'RR_RootCellar': 'A root cellar under a sod mound, a stone headwall.',
}
SHOTS = {
    'RR_Shed_B': dict(view=(-0.7, -1.0, 0.45)),
    'RR_RootCellar': dict(view=(-0.5, -1.0, 0.4)),
}

models = [build() for key, build in BUILDERS if not ONLY or key in ONLY]

if lt.want_preview():
    homes.previews(models, CAPTIONS, sheet=False, shots_by_name=SHOTS, out_dir=PREVIEW_DIR)
    if not ONLY:
        homes.lineup(models, os.path.join(PREVIEW_DIR, 'Lineup_Outbuildings.png'), gap=1.6,
                     view=(-0.3, -1.0, 0.45), lens=35.0)
