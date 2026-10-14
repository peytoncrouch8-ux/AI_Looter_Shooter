"""Frontier clutter for Ransom's Rest (the "make it alive" polish, concept board A_town; look sheet 42): the small props
that turn bare porches, yards and Main Street into a lived-in frontier town. A scripted model file (Art/README.md);
Tools/models.ps1 exports it into /Game/Art/Props (SM_<name>). Built with Tools/Blender/looter_clutter.py on the house
trim sheet used as a palette (weathered wood A/B/C, cream G, faded teal and oxide paint H1/H2, iron H3, dark glass or
water H4), so nearly every piece is ONE material slot, HouseTrim; the exceptions say so. The homes already carry their
benches, rain barrels, woodpiles and crepe, and RR_Home_E and RR_Home_J a chopping block each: these go elsewhere.

Porch (wall pieces have their pivot on the wall's face, the piece in front of it toward -Y):
  RockingChair        a spindle-back rocker in dark wood: turned front legs, raked stiles with knobs, a bowed crest
                      rail, five spindles, a plank seat, flat arms, long rockers. Faces -Y. Pivot: the floor under the
                      middle of its rockers.
  RockingChair_Shawl  the same rocker with a faded oxide wool shawl folded over its crest rail, fringed, hanging
                      lower on one side behind.
  PorchChair          a plain ladder-back chair: weathered grey posts, three bent slats, a plank seat, rungs.
  Washtub             a coopered washtub (18 staves, two iron hoops, two lugs) half full of dark water, a cream sheet
                      and a teal shirt soaking in it, a wooden washboard with a zinc rubbing panel (MailboxTin: 2
                      slots) leaning in it, a rag over its rim.
  FlowerBox           a window box gone to seed: faded teal planks on two iron brackets, its soil (GroundDirt: 2
                      slots) bristling with dried stalks, seed heads and curled leaves. Pivot: on the wall at the box's
                      top back edge (set it just under a sill); the box hangs 0.2 m below it, the brackets 0.33 m.
  PorchLantern        an iron lantern on a wall bracket (looter_ruins' lantern, its frame on the trim's iron; glass on
                      LanternGlow: 2 slots; swap that slot to MI_HouseTrim for a lantern out). Pivot: the bracket's
                      wall plate. SOCKET_Light at the glass.
  BroomBucket         a flat broom of broomcorn (the Hay set: 2 slots), stitched in two rows, leaning on the wall
                      beside a wooden bucket with its bail down. Pivot: the foot of the wall (ground, wall face).
  Horseshoe           an iron horseshoe nailed up ends-up for luck. Pivot: the middle of the shoe on the wall.

Yard:
  ChoppingBlock       a round of log (the homes' block: strip B, end grain on WoodEndGrain: 2 slots), an axe sunk
                      in its top, chips and two split billets round it.
  HandPump            a cast-iron pitcher pump on a timber post, on a plank platform (1.2 m square) on sleepers, its
                      handle up, a bucket of water under the spout.
  MilkCans            three tinned milk cans (MailboxTin only): two lidded, one open with its lid on the ground.
  KitchenGarden       a 3 x 2 m patch of hoed ridges (GroundDirt) going over: dry corn stalks, two bean tepees with
                      dead vines and pods, dried squash leaves, a stake fence round its back and left with a hoe
                      leaning on it (HouseTrim + GroundDirt: 2 slots). Flat: sits on the ground, a skirt 4 cm under.
  ChickenCoop         a board-and-batten coop on legs with a plank shed roof, a pop door with its ramp, a nest box,
                      and a lath-fenced run in front of it (2.1 x 1.3 m inside; its report line gives where).
                      SOCKET_Run: the run's middle on the ground. The chickens are another agent's.

Main Street:
  Spittoon            a brass spittoon (BrassWorn only).
  BottleCrate         a slatted crate of empties (dark glass on the trim) and three loose bottles by it.
  FlourSacks          five sacks of ORRIN BROS. FLOUR (an invented mill), stacked, one stood up against them.
  Buckboard           a light two-seat spring wagon: faded oxide-red wheels, a long board bed on bolsters, two seats,
                      a dash, a brake, its shafts down on the ground in front. Pivot: the middle of its footprint with
                      the shafts (see its report line for where the bed lies).
  SignBracket         a wrought-iron blade-sign bracket (a scrolled arm, a brace and a volute on a wall plate). Pivot:
                      the arm's root on the wall face (the plate runs 5 cm above it and 55 cm below); the arm runs out
                      0.92 m toward -Y. SOCKET_Sign (0, -0.55, -0.054): where a board hangs.
  SignBoard_Pruitts   PRUITT'S in cream on oxide-red planks, a dark frame; lettered both sides.
  SignBoard_Barber    BARBER in cream on faded teal planks.
  SignBoard_Hotel     HOTEL in oxide red on a cream board.
                      A board's pivot is its swing point (the middle between its two rings): attach it at the bracket's
                      SOCKET_Sign and swing it about the socket's forward axis (Unreal X, the arm's line). No collision.
  Bunting             faded cloth pennants (oxide, cream, teal) on a cord: a 6 m run in two sagging swags, for the
                      Gilt Spur's balcony rail. Pivot: the middle pin; the end pins at x = +-3 m, all at z = 0.

Budgets: props 200-2,500 triangles, Nanite off (small props), LODs 50% and 25% from 600 triangles. Collision: convex
UCX_ hulls where something must block (chairs, tubs, pump, cans, crates, sacks, the wagon, the coop and fences, the
window box, the broom's bucket); Collision 'None' on the lantern, the horseshoe, the bracket, the signs and the
bunting. Small iron on the trim's iron strip is folded into its least rusty stretch (looter_clutter.iron_speckle).
Everything random is seeded; nothing walks a set of mesh elements (two exports compare equal, 2026-10-14).

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\FrontierClutter.py -Preview     every model, the renders and
                                                    Saved/ArtPreviews/RansomsRest/Clutter/FrontierClutter.report.json
                                                    (--only=Name,Name builds just those; --no-vignettes skips the sets)
    Tools\\artrun.ps1 -Script Tools\\Blender\\clutter_sheet.py                    the look sheet (42_FrontierClutter.jpg)
    Tools\\models.ps1 -Only FrontierClutter -Out Intermediate\\ArtExport_Clutter -NoImport          export test
"""
import math
import os
import random

import bpy
from mathutils import Matrix, Vector, noise

import looter_buildings as kit
from looter_buildings import Opening, Trim
import looter_clutter as lc
import looter_homes as homes
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt
import looter_town as town

ARGS = lc.ARGS
ONLY = next((a.split('=', 1)[1].split(',') for a in ARGS if a.startswith('--only=')), None)
UP = Vector((0.0, 0.0, 1.0))


def along(p0, p1):
    """The turn taking +Z along p0 -> p1, and the distance."""
    d = Vector(p1) - Vector(p0)
    return UP.rotation_difference(d.normalized()).to_matrix(), d.length


def spindle(m, p0, p1, radii, sides=8, strip='C'):
    """A turned part from p0 to p1: radii [(r, t), ...] along it (t from 0 to 1), its ends flat."""
    rot, length = along(p0, p1)
    prof = [(0.0, 0.0)] + [(r, t * length) for r, t in radii] + [(0.0, length)]
    lc.turned(m, prof, sides, at=p0, rot=rot, strip=strip, grain='up')


def knob(m, at, direction, size, sides=8, strip='C'):
    rot, _ = along((0.0, 0.0, 0.0), direction)
    lc.turned(m, [(0.0, 0.0), (0.6 * size, 0.0), (size, 0.4 * size), (0.55 * size, 0.88 * size), (0.0, size)], sides,
              at=at, rot=rot, strip=strip, grain='up')


def ring_hull(m, center, radius, z0, z1, count=8):
    c = Vector(center)
    m.hull_points([c + Vector((math.cos(2.0 * math.pi * k / count) * radius, math.sin(2.0 * math.pi * k / count) * radius,
                               z)) for k in range(count) for z in (z0, z1)])


# --- RockingChair, RockingChair_Shawl ---

CH_R, CH_Y0, CH_H = 1.25, 0.03, 0.058       # the rockers: radius, lowest point, depth
CH_FY, CH_BY = -0.17, 0.2                   # front legs, back stiles


def rocker_z(y):
    return (y - CH_Y0) ** 2 / (2.0 * CH_R)


def crest_y(x):
    return CH_BY + 0.112 + 0.028 * (1.0 - (x / 0.215) ** 2)


def low_y(x):
    return CH_BY + 0.014 + 0.016 * (1.0 - (x / 0.215) ** 2)


def rocking_chair(name, seed, shawl=False):
    m = lc.Model(name, seed)
    for side in (-1.0, 1.0):
        ys = [-0.47 + 1.08 * k / 10 for k in range(11)]
        lc.tube(m, [(side * 0.235, y, rocker_z(y) + CH_H * 0.5) for y in ys], lc.rect(0.026, CH_H), strip='C',
                up=(0.0, 0.0, 1.0), scales=[0.55, 0.85] + [1.0] * 7 + [0.85, 0.55])
        x = side * 0.215
        spindle(m, (x, CH_FY, rocker_z(CH_FY) + CH_H - 0.008), (x, CH_FY, 0.636),
                [(0.016, 0.0), (0.0172, 0.15), (0.015, 0.4), (0.0198, 0.47), (0.015, 0.55), (0.016, 0.85), (0.012, 1.0)])
        stile = [(x, CH_BY, rocker_z(CH_BY) + CH_H - 0.008), (x, CH_BY, 0.4), (x, CH_BY + 0.03, 0.62),
                 (x, CH_BY + 0.075, 0.84), (x, CH_BY + 0.112, 1.0), (x, CH_BY + 0.122, 1.07)]
        lc.rod(m, stile, 0.019, strip='C', sides=8, radii=[0.019, 0.019, 0.019, 0.018, 0.017, 0.016])
        knob(m, (x, CH_BY + 0.122, 1.066), (0.0, 0.13, 1.0), 0.042)
        # The arm: from the stile, over the front leg and a little past it.
        lc.tube(m, [(side * 0.226, CH_BY + 0.045, 0.646), (side * 0.244, 0.0, 0.651), (side * 0.236, CH_FY - 0.065, 0.646)],
                lc.rect(0.062, 0.022), strip='C', up=(0.0, 0.0, 1.0))
        lc.rod(m, [(x, CH_FY, 0.22), (x, CH_BY, 0.22)], 0.011, strip='C')
    lc.rod(m, [(-0.215, CH_FY, 0.2), (0.215, CH_FY, 0.2)], 0.011, strip='C')
    lc.rod(m, [(-0.215, CH_BY, 0.24), (0.215, CH_BY, 0.24)], 0.011, strip='C')
    # The seat: three boards, sloping a little back.
    for y in (-0.152, 0.0, 0.152):
        z_top = 0.43 - (y + 0.23) * 0.055
        m.board((-0.257, y, z_top - 0.014), (0.257, y, z_top - 0.014), 0.148, 0.028, face=(0.0, -0.055, 1.0), uv='C',
                bevel=0.005)
    # The back: a bowed crest rail, a lower rail and five spindles between them.
    xs = [-0.212 + 0.424 * k / 6 for k in range(7)]
    lc.tube(m, [(x, crest_y(x), 0.995) for x in xs], lc.rect(0.024, 0.09), strip='C', up=(0.0, 0.0, 1.0))
    lc.rod(m, [(x, low_y(x), 0.5) for x in xs], 0.012, strip='C')
    for x in (-0.14, -0.07, 0.0, 0.07, 0.14):
        lc.rod(m, [(x, low_y(x), 0.5), (x, (low_y(x) + crest_y(x)) * 0.5 + 0.012, 0.73), (x, crest_y(x), 0.952)],
               0.0095, strip='C')
    if shawl:
        draped_shawl(m)
    m.hull_points([Vector((sx * 0.27, y, z)) for sx in (-1.0, 1.0) for y, z in
                   ((-0.47, 0.09), (0.61, 0.12), (CH_Y0, 0.0), (-0.25, 0.66), (0.36, 1.1), (CH_BY + 0.05, 0.66))])
    caption = ('A spindle-back rocker in dark wood: turned legs,\na bowed crest, long rockers.' if not shawl else
               'The rocker with a faded red wool shawl folded\nover its crest rail.')
    return m.done('the floor under the middle of the rockers', caption, ao=0.35)


def draped_shawl(m):
    """A folded wool shawl in faded oxide over the crest rail: a short flap in front, a long fall behind hanging lower
    on the left, folds deepening toward a fringed hem."""
    rng = random.Random(m.rng.randint(0, 10 ** 6))
    count = 12
    xs = [-0.185 + 0.37 * i / (count - 1) for i in range(count)]
    # The drape's line over the crest, as (y from the crest's middle, z): front flap, over the top, the fall behind.
    path = [(-0.034, 0.9), (-0.031, 0.955), (-0.025, 1.003), (-0.015, 1.038), (0.0, 1.052), (0.017, 1.041),
            (0.03, 1.004), (0.042, 0.94), (0.054, 0.86), (0.066, 0.78), (0.076, 0.7), (0.084, 0.62)]
    phase = rng.uniform(0.0, 6.28)
    rows = []
    for j, (dy, z) in enumerate(path + [path[-1]]):
        fall = max(0.0, (j - 5) / 6.0)
        fringe = j == len(path)
        row = []
        for i, x in enumerate(xs):
            t = (x + 0.185) / 0.37
            fold = (0.003 + 0.026 * fall) * math.sin(x * 2.0 * math.pi / 0.11 + phase + fall * 1.3) \
                + 0.008 * fall * math.sin(x * 2.0 * math.pi / 0.047 + phase * 2.0)
            flare = 0.02 * fall * (abs(x) / 0.18) ** 2
            drop = 0.09 * fall * max(0.0, 1.0 - t) ** 1.5           # the left side hangs lower behind
            hem = rng.uniform(-0.01, 0.01) if j >= len(path) - 1 else 0.0
            tassel = (0.035 if i % 2 == 0 else 0.012) if fringe else 0.0
            front = -1.0 if dy < -0.01 else 1.0
            row.append((x * (1.0 + 0.06 * fall), crest_y(x) + dy + front * fold * (0.35 if front < 0 else 1.0) + flare,
                        z - drop + hem - tassel))
        rows.append(row)
    # Faded oxide wool: the paint strip H2 run along the drape (its U), the shawl's width squeezed across the strip.
    dist = [0.0]
    for j in range(1, len(rows)):
        dist.append(dist[-1] + (Vector(rows[j][count // 2]) - Vector(rows[j - 1][count // 2])).length)
    lc.cloth(m, rows, 0.011, strip='H2', edges=True, uv_of=lambda j, i: (0.6 + dist[j], 0.05 + 0.1 * i / (count - 1)))


# --- PorchChair ---

def porch_chair(name, seed):
    m = lc.Model(name, seed)
    seat = 0.45
    FY, BY = -0.17, 0.17
    for side in (-1.0, 1.0):
        x = side * 0.205
        spindle(m, (x, FY, 0.0), (x, FY, seat + 0.035), [(0.018, 0.0), (0.019, 0.5), (0.017, 0.92), (0.013, 1.0)],
                strip='A')
        knob(m, (x, FY, seat + 0.03), (0.0, 0.0, 1.0), 0.03, strip='A')
        post = [(x, BY, 0.0), (x, BY, seat), (x, BY + 0.03, 0.7), (x, BY + 0.06, 0.96)]
        lc.rod(m, post, 0.019, strip='A', sides=8, radii=[0.019, 0.019, 0.018, 0.016])
        knob(m, (x, BY + 0.06, 0.955), (0.0, 0.12, 1.0), 0.036, strip='A')
        for z in (0.14, 0.27):
            lc.rod(m, [(x, FY, z), (x, BY, z)], 0.0105, strip='A')
    for z in (0.17, 0.3):
        lc.rod(m, [(-0.205, FY, z), (0.205, FY, z)], 0.0105, strip='A')
    lc.rod(m, [(-0.205, BY, 0.21), (0.205, BY, 0.21)], 0.0105, strip='A')
    # Aprons under the seat, two seat boards.
    m.board((-0.19, FY - 0.002, seat - 0.045), (0.19, FY - 0.002, seat - 0.045), 0.055, 0.02, face=(0.0, -1.0, 0.0), uv='A')
    for side in (-1.0, 1.0):
        m.board((side * 0.205, FY + 0.02, seat - 0.045), (side * 0.205, BY - 0.02, seat - 0.045), 0.055, 0.02,
                face=(side, 0.0, 0.0), uv='A')
    for y in (-0.09, 0.1):
        m.board((-0.236, y, seat + 0.012), (0.236, y, seat + 0.012), 0.19, 0.026, face=(0.0, 0.0, 1.0), uv='A',
                bevel=0.005)
    # Three slats bent to the back, between the posts.
    for z in (0.6, 0.73, 0.86):
        y_post = BY + 0.03 * min(1.0, (z - seat) / (0.7 - seat)) + 0.03 * max(0.0, (z - 0.7) / 0.26)
        xs = [-0.2 + 0.4 * k / 4 for k in range(5)]
        lc.tube(m, [(x, y_post + 0.022 * (1.0 - (x / 0.2) ** 2), z) for x in xs], lc.rect(0.012, 0.062), strip='A',
                up=(0.0, 0.0, 1.0))
    m.hull_points([Vector((sx * 0.24, y, z)) for sx in (-1.0, 1.0) for y, z in
                   ((FY - 0.03, 0.0), (FY - 0.03, seat + 0.03), (BY + 0.03, 0.0), (BY + 0.08, 0.98))])
    return m.done('the floor under the middle of its legs', 'A plain ladder-back chair in weathered grey:\n'
                  'three bent slats, a plank seat.', ao=0.35)


# --- Washtub ---

def washtub(name, seed):
    m = lc.Model(name, seed)
    RB, RT, H = 0.3, 0.345, 0.36
    r_at = lc.vessel(m, (0.0, 0.0, 0.0), RB, RT, H, 18, wall=0.022, floor=0.04, hoops=(0.075, 0.285), water=0.2)
    # Washing soaking in the water: a cream sheet bunched up in folds, a teal shirt beside it, their edges under.
    for k, (strip, cx, cy, rx, ry, height) in enumerate((('G', 0.04, -0.06, 0.2, 0.15, 0.06),
                                                         ('H1', -0.15, 0.08, 0.11, 0.09, 0.035))):
        rows = []
        for j in range(7):
            row = []
            for i in range(7):
                x = cx + rx * 1.25 * (i / 6 - 0.5) * 2.0
                y = cy + ry * 1.25 * (j / 6 - 0.5) * 2.0
                d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
                bump = max(0.0, 1.0 - d) ** 0.6
                fold = 0.5 + 0.5 * math.sin(x * 38.0 + y * 21.0 + k) * math.cos(y * 29.0 - x * 9.0)
                reach = math.hypot(x, y)
                if reach > 0.28:                      # tucked in under the water short of the staves
                    x, y = x * 0.28 / reach, y * 0.28 / reach
                row.append((x, y, 0.2 - 0.012 + (height + 0.012) * bump * (0.55 + 0.45 * fold)))
            rows.append(row)
        lc.cloth(m, rows, 0.003, strip=strip, v0=0.05 if strip == 'H1' else 0.2)
    # Two lugs: staves standing proud of the rim with a hand hole through each.
    for side in (-1.0, 1.0):
        matrix = kit.basis((side * (RT + 0.002), 0.0, H - 0.04), (0.0, side, 0.0), (-side, 0.0, 0.0), (0.0, 0.0, 1.0))
        outline = [(-0.065, 0.0), (0.065, 0.0), (0.065, 0.085), (0.042, 0.122), (-0.042, 0.122), (-0.065, 0.085)]
        hole = [(-0.034, 0.062), (-0.034, 0.093), (0.034, 0.093), (0.034, 0.062)]
        lc.prism_part(m, [outline, hole], 0.024, matrix, uv=Trim('A', rotate=True))
    # The washboard leaning in it: a wooden frame round a ribbed zinc panel, its feet on the tub's floor.
    tilt = 25.0
    foot = Vector((0.03, 0.163, 0.04))
    frame = Matrix.Translation(foot) @ Matrix.Rotation(math.radians(6.0), 4, 'Z') @ Matrix.Rotation(math.radians(-tilt), 4, 'X')
    for x in (-0.135, 0.135):
        m.box((0.62, 0.022, 0.04), uv=Trim('A'), matrix=frame @ Matrix.Translation((x, 0.0, 0.31)) @
              Matrix.Rotation(math.radians(90.0), 4, 'Y'), bevel=0.004)
    m.box((0.31, 0.024, 0.1), uv=Trim('A'), matrix=frame @ Matrix.Translation((0.0, 0.0, 0.57)), bevel=0.005)
    m.box((0.27, 0.02, 0.032), uv=Trim('A'), matrix=frame @ Matrix.Translation((0.0, 0.0, 0.165)))
    rows = 22
    cos, faces = [], []
    scale = lt.SETS['MetalWorn']['density'] / float(lt.SETS['MetalWorn']['size'])
    for j in range(rows + 1):
        z = 0.185 + 0.33 * j / rows
        y = -0.006 if j % 2 else 0.001
        for x in (-0.116, 0.116):
            cos.append(frame @ Vector((x, y, z)))
    for j in range(rows):
        a, b, c, d = 2 * j, 2 * j + 1, 2 * j + 3, 2 * j + 2
        idx = lc._orient(cos, [a, b, c, d], frame.to_3x3() @ Vector((0.0, -1.0, 0.0)))
        faces.append((idx, [((cos[i] - frame.translation).x * scale + 0.3,
                             (cos[i] - frame.translation).z * scale + 0.7) for i in idx]))
    back = [len(cos) + k for k in range(4)]
    for x, z in ((-0.116, 0.185), (0.116, 0.185), (0.116, 0.515), (-0.116, 0.515)):
        cos.append(frame @ Vector((x, 0.006, z)))
    idx = lc._orient(cos, back, frame.to_3x3() @ Vector((0.0, 1.0, 0.0)))
    faces.append((idx, [(0.3 + 0.01 * k, 0.7) for k in range(4)]))
    m.put(cos, faces, key='tin')
    # A rag over the front rim, wet and heavy: in the water inside, over the rim, down the outside past the hoop.
    rows_ = []
    drape = [(r_at(0.22) - 0.03, 0.22), (r_at(0.3) - 0.027, 0.3), (r_at(H) - 0.026, H - 0.004), (RT - 0.011, H + 0.008),
             (RT + 0.005, H - 0.006), (r_at(0.3) + 0.007, 0.3), (r_at(0.285) + 0.013, 0.262), (r_at(0.2) + 0.008, 0.19)]
    for j, (r, z) in enumerate(drape):
        row = []
        for k in range(8):
            x = -0.2 + 0.2 * k / 7
            outside = j >= 5
            crease = (j - 4) / 3.0 if outside else 0.0          # creases deepen down the outside
            rr = r + (0.004 * math.sin(k * 1.7) if z < H - 0.01 else 0.0) + 0.011 * crease * (0.5 + 0.5 * math.sin(k * 2.4 + 0.7))
            hem = 0.07 * (k / 7) ** 2 if j == len(drape) - 1 else 0.0       # one corner hangs lower
            row.append((x, -math.sqrt(max(rr * rr - x * x, 0.0)), z + 0.008 * math.sin(k * 2.3 + z * 20.0) - hem))
        rows_.append(row)
    lc.cloth(m, rows_, 0.005, strip='G')
    ring_hull(m, (0.0, 0.0, 0.0), RT + 0.01, 0.0, H + 0.05)
    return m.done('the ground under its middle', 'A coopered washtub, washing soaking in it, a\n'
                  'zinc-panelled washboard leaning in, a rag on the rim.', ao=0.35)


# --- FlowerBox ---

def flower_box(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    L, D, H = 0.92, 0.2, 0.2           # the box: its back against the wall at y = -0.01, its top at z = 0
    yb, yf = -0.01, -0.01 - D
    teal = 3.55                        # a joint-free stretch of the teal strip
    m.board((-L * 0.5, yf + 0.011, -H * 0.5), (L * 0.5, yf + 0.011, -H * 0.5), H, 0.022, face=(0.0, -1.0, 0.0),
            uv=Trim('H1', u=teal), bevel=0.003)
    m.board((-L * 0.5 + 0.02, yb - 0.011, -H * 0.5), (L * 0.5 - 0.02, yb - 0.011, -H * 0.5), H, 0.022, face=(0.0, 1.0, 0.0),
            uv=Trim('H1', u=teal + 1.2))
    for side in (-1.0, 1.0):
        m.board((side * (L * 0.5 - 0.011), yb - 0.022, -H * 0.5), (side * (L * 0.5 - 0.011), yf + 0.022, -H * 0.5), H, 0.022,
                face=(side, 0.0, 0.0), uv=Trim('H1', u=teal + 0.3 + 0.2 * side), bevel=0.003)
    m.board((-L * 0.5 + 0.02, (yb + yf) * 0.5, -H + 0.011), (L * 0.5 - 0.02, (yb + yf) * 0.5, -H + 0.011), D - 0.04, 0.022,
            face=(0.0, 0.0, -1.0), uv='A')
    # The soil, lumpy, a little under the rim.
    cols, rws = 8, 3
    cos, faces = [], []
    sc = lt.SETS['GroundDirt']['density'] / float(lt.SETS['GroundDirt']['size'])
    for j in range(rws + 1):
        for i in range(cols + 1):
            x = -L * 0.5 + 0.022 + (L - 0.044) * i / cols
            y = yf + 0.022 + (D - 0.044) * j / rws
            z = -0.032 + 0.008 * noise.noise(Vector((x * 9.0, y * 9.0, 1.3)))
            cos.append(Vector((x, y, z)))
    for j in range(rws):
        for i in range(cols):
            a = j * (cols + 1) + i
            idx = lc._orient(cos, [a, a + 1, a + cols + 2, a + cols + 1], UP)
            faces.append((idx, [(cos[k].x * sc + 0.37, cos[k].y * sc + 0.61) for k in idx]))
    m.put(cos, faces, key='dirt')
    # Two iron brackets: a strap down the wall, an arm under the box, a brace between.
    for x in (-0.3, 0.3):
        m.box((0.026, 0.008, 0.15), at=(x, -0.004, -0.27), uv='H3')
        m.box((0.026, D + 0.01, 0.008), at=(x, (yb + yf) * 0.5 - 0.004, -H - 0.004), uv='H3')
        m.board((x, -0.008, -0.335), (x, yf + 0.04, -H - 0.006), 0.022, 0.008, face=(1.0, 0.0, 0.0), uv='H3')
    # Dead foliage matted on the soil: curled leaves fanned out from four crowns.
    for cx in (-0.33, -0.1, 0.12, 0.34):
        crown = Vector((cx + rng.uniform(-0.04, 0.04), (yb + yf) * 0.5 + rng.uniform(-0.02, 0.02), -0.028))
        for n in range(4):
            a = rng.uniform(0.0, 6.28)
            out = Vector((math.cos(a), math.sin(a) * 0.6, 0.0))
            ln = rng.uniform(0.06, 0.1)
            lc.strap(m, [crown, crown + out * ln * 0.5 + Vector((0.0, 0.0, 0.018)), crown + out * ln - Vector((0.0, 0.0, 0.004))],
                     [0.02, 0.026, 0.006], None, 0.0015, strip='C', lane_v=0.1, across=out.cross(UP))
    # Dried stalks gone to seed: coneflower heads, seed spikes, a forked one, two snapped and hanging over the front.
    for k in range(20):
        x = -0.41 + 0.82 * (k + rng.uniform(-0.35, 0.35)) / 19
        y = rng.uniform(yf + 0.045, yb - 0.045)
        h = rng.uniform(0.12, 0.46) * (0.6 if k % 5 == 4 else 1.0)
        lean = Vector((rng.uniform(-0.14, 0.14), rng.uniform(-0.22, -0.02), 1.0)).normalized()
        base = Vector((x, y, -0.03))
        bend = Vector((rng.uniform(-0.05, 0.05), rng.uniform(-0.06, 0.0), rng.uniform(-0.02, 0.0)))
        snapped = k in (3, 11)
        if snapped:
            knee = base + lean * h * 0.55
            pts = [base, base + lean * h * 0.3, knee, knee + Vector((0.0, -0.05, -0.01)),
                   knee + Vector((0.0, -0.09, -0.1))]
        else:
            pts = [base, base + lean * h * 0.35 + bend * 0.3, base + lean * h * 0.7 + bend * 0.8, base + lean * h + bend]
        thick = rng.uniform(0.0028, 0.0042)
        lc.rod(m, pts, thick, strip='A', sides=4, radii=[thick * 1.1] + [thick] * (len(pts) - 2) + [thick * 0.6])
        top, d = pts[-1], (pts[-1] - pts[-2]).normalized()
        kind = k % 4
        if not snapped and kind in (0, 2):
            size = rng.uniform(0.85, 1.25)
            lc.turned(m, [(0.0, -0.004 * size), (0.014 * size, 0.0), (0.012 * size, 0.013 * size), (0.0, 0.022 * size)], 5,
                      at=top, rot=along((0, 0, 0), d)[0], strip='C', grain='up')
        elif not snapped and kind == 1:
            lc.turned(m, [(0.0, -0.01), (0.006, 0.0), (0.0075, 0.03), (0.004, 0.055), (0.0, 0.065)], 4, at=top,
                      rot=along((0, 0, 0), d)[0], strip='C', grain='up')
        elif not snapped:
            fork = pts[2] + Vector((rng.choice((-1.0, 1.0)) * 0.05, -0.03, 0.07))
            lc.rod(m, [pts[2], (pts[2] + fork) * 0.5 + Vector((0.0, 0.0, 0.01)), fork], 0.0025, strip='A', sides=4)
            lc.turned(m, [(0.0, -0.003), (0.01, 0.0), (0.009, 0.01), (0.0, 0.016)], 5, at=fork,
                      rot=along((0, 0, 0), fork - pts[2])[0], strip='C', grain='up')
        if k % 3 == 0:
            node = pts[1]
            out = Vector((rng.uniform(-1.0, 1.0), rng.uniform(-1.0, -0.2), 0.0)).normalized()
            lc.strap(m, [node, node + out * 0.04 + Vector((0.0, 0.0, 0.01)), node + out * 0.075 - Vector((0.0, 0.0, 0.03))],
                     [0.012, 0.017, 0.004], None, 0.0015, strip='A', lane_v=0.1, across=out.cross(UP))
    m.hull((L + 0.01, D + 0.01, H + 0.01), at=(0.0, (yb + yf) * 0.5, -H * 0.5))
    return m.done('on the wall at the box\'s top back edge (set it just under a sill)',
                  'A window box gone to seed on iron brackets:\ndried stalks and seed heads.', ao=0.25, ground=False)


# --- PorchLantern ---

def porch_lantern(name, seed):
    m = lc.Model(name, seed)
    m.box((0.055, 0.012, 0.19), at=(0.0, -0.006, -0.03), uv='H3', bevel=0.002)
    for z in (0.04, -0.1):
        m.box((0.016, 0.006, 0.016), at=(0.0, -0.014, z), uv='H3')
    arm = lc.spline([(0.0, -0.012, 0.035), (0.0, -0.15, 0.04), (0.0, -0.26, 0.032), (0.0, -0.3, 0.012),
                     (0.0, -0.296, -0.012), (0.0, -0.282, -0.008)], per=3)
    lc.tube(m, arm, lc.rect(0.02, 0.011), strip='H3', up=(1.0, 0.0, 0.0))
    brace = lc.spline([(0.0, -0.012, -0.11), (0.0, -0.07, -0.06), (0.0, -0.13, 0.0), (0.0, -0.19, 0.03)], per=3)
    lc.tube(m, brace, lc.rect(0.014, 0.009), strip='H3', up=(1.0, 0.0, 0.0))
    curl = [Vector((0.0, -0.05, -0.02)) + Vector((0.0, -math.cos(a) * r, math.sin(a) * r))
            for a, r in ((math.radians(200 + 60 * k), 0.035 * (1.0 - k / 7.5)) for k in range(7))]
    lc.tube(m, curl, lc.rect(0.012, 0.008), strip='H3', up=(1.0, 0.0, 0.0))
    # The lantern hangs from the hook by its ring.
    hook = Vector((0.0, -0.288, -0.006))
    parts, ring_top, glass = lr.iron_lantern((0.0, -0.288, 0.0), seed, key_iron='trim', size=0.85)
    shift = hook + Vector((0.0, 0.0, 0.007)) - ring_top
    for part in parts:
        lp.place(part, tuple(shift))
        m.add(part)
    m.socket('Light', tuple(glass + shift))
    return m.done('on the wall at the bracket plate', 'An iron lantern on a wall bracket (LanternGlow\n'
                  'glass, SOCKET_Light).', ao=0.25, ground=False, collision='None')


# --- BroomBucket ---

def bucket(m, at, seed_turn=0.0, water=None, bail_down=1.0):
    """A wooden bucket: twelve staves, two hoops, iron ears and a bail lying down to one side."""
    x0, y0, z0 = at
    r_at = lc.vessel(m, at, 0.115, 0.14, 0.27, 12, wall=0.015, floor=0.03, hoops=(0.05, 0.215), water=water)
    c = Vector(at)
    ear = r_at(0.235)
    a = math.radians(seed_turn)
    across = Vector((math.cos(a), math.sin(a), 0.0))
    fall = Vector((-math.sin(a), math.cos(a), 0.0)) * bail_down
    for s in (-1.0, 1.0):
        m.box((0.012, 0.03, 0.045), at=tuple(c + across * s * (ear + 0.004) + Vector((0.0, 0.0, 0.235))), uv='H3',
              rot=(0.0, 0.0, seed_turn))
    lean = math.radians(65.0)
    pts = []
    for k in range(9):
        t = math.pi * k / 8
        pts.append(c + Vector((0.0, 0.0, 0.24)) + across * (math.cos(t) * (ear + 0.012)) +
                   (UP * math.cos(lean) + fall * math.sin(lean)) * (math.sin(t) * 0.16))
    lc.rod(m, pts, 0.0042, strip='H3', sides=4)
    return r_at


def broom_bucket(name, seed):
    m = lc.Model(name, seed)
    bucket(m, (0.2, -0.2, 0.0), seed_turn=8.0, bail_down=-1.0)
    # The broom: bristles down on the ground, its handle on the wall.
    foot = Vector((-0.19, -0.3, 0.0))
    top = Vector((-0.12, -0.016, 1.22))
    d = (top - foot).normalized()
    across = (Vector((1.0, 0.0, 0.0)) - d * d.x).normalized()
    rng = random.Random(seed)
    # A flat broom of broomcorn (the Hay set: 2 slots): fanned and thin at its worn tips, stitched across in two rows,
    # bound to the handle at its neck.
    sections = [(0.0, 0.31, 0.026), (0.03, 0.305, 0.04), (0.11, 0.285, 0.05), (0.21, 0.205, 0.056), (0.28, 0.12, 0.05),
                (0.335, 0.06, 0.038), (0.37, 0.036, 0.032)]
    sides = 12

    def section(w, th, scale=1.0):
        out = []
        for k in range(sides):
            a = 2.0 * math.pi * k / sides
            s = math.sin(a)
            out.append((math.cos(a) * th * 0.5 * scale, math.copysign(abs(s) ** 0.6, s) * w * 0.5 * scale))
        return out
    centers = [foot + d * (t + 0.004) for t, _, _ in sections]
    profiles = [section(w, th) for _, w, th in sections]
    shifts = [[rng.uniform(-0.022, 0.012) if i == 0 else (rng.uniform(-0.008, 0.004) if i == 1 else 0.0)
               for _ in range(sides)] for i in range(len(sections))]
    lc.tube(m, centers, None, key='hay', up=across, profiles=profiles, shifts=shifts)
    for t in (0.2, 0.25):
        f = (t - 0.11) / 0.1 if t < 0.21 else (t - 0.21) / 0.07
        w = 0.285 + (0.205 - 0.285) * f if t < 0.21 else 0.205 + (0.12 - 0.205) * f
        th = 0.05 + 0.006 * f if t < 0.21 else 0.056 - 0.006 * f
        lc.tube(m, [foot + d * (t - 0.006), foot + d * (t + 0.006)], section(w, th, 1.035), strip='H3', up=across,
                caps=(False, False))
    lc.tube(m, [foot + d * 0.34, foot + d * 0.38], section(0.05, 0.04, 1.0), strip='H3', up=across, caps=(False, False))
    lc.rod(m, [foot + d * 0.36, top], 0.0125, strip='C', sides=6)
    ring_hull(m, (0.2, -0.2, 0.0), 0.15, 0.0, 0.29)
    return m.done('the foot of the wall (on the ground, on the wall\'s face)', 'A straw broom on the wall over a\n'
                  'wooden bucket, its bail down.', ao=0.3)


# --- Horseshoe ---

def horseshoe(name, seed):
    m = lc.Model(name, seed)
    R = 0.057
    pts = []
    for k in range(17):
        a = math.radians(112.0 + (428.0 - 112.0) * k / 16)
        r = R * (1.0 + 0.04 * math.cos(a * 2.0))
        pts.append(Vector((math.cos(a) * r, -0.0055, math.sin(a) * r * 1.1 - 0.008)))
    scales = [0.82, 0.92] + [1.0] * 13 + [0.92, 0.82]
    # Its section: flat to the wall behind, rounded over the front, a groove (the fuller) down its middle.
    section = [(-0.0105, 0.0045), (-0.0105, -0.001), (-0.007, -0.0045), (-0.002, -0.003), (0.002, -0.003),
               (0.007, -0.0045), (0.0105, -0.001), (0.0105, 0.0045)]     # +up is toward the wall here
    lc.tube(m, pts, section, strip='H3', up=(0.0, 1.0, 0.0), scales=scales)
    for a in (150.0, 390.0, 250.0, 290.0):
        r = R * (1.0 + 0.04 * math.cos(math.radians(a) * 2.0))
        x, z = math.cos(math.radians(a)) * r, math.sin(math.radians(a)) * r * 1.1 - 0.008
        size = 0.008 if a in (150.0, 390.0) else 0.006
        m.box((size, 0.004 if a in (150.0, 390.0) else 0.002, size), at=(x, -0.011, z), uv='H3')
    return m.done('the middle of the shoe on the wall', 'An iron horseshoe nailed up ends-up for luck\n'
                  '(over a door).', ao=0.05, ground=False, collision='None', lods='')


# --- ChoppingBlock ---

def chopping_block(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    R, H = 0.25, 0.5
    top = Vector((0.012, -0.008, H))
    m.cylinder((0.0, 0.0, -0.04), tuple(top), R, R * 0.97, sides=12, uv=Trim('B', v=0.0), face=(0.0, -1.0, 0.0),
               caps=(False, True), cap_uv=kit.EndGrain(), cap_mat='endgrain')
    # The axe: its bit sunk into the top, the handle up and out toward the front left.
    h = Vector((-0.62, -0.42, 0.66)).normalized()
    down = Vector((0.0, 0.0, -1.0))
    b = (down - h * down.dot(h)).normalized()
    eye_z = H - 0.032 - b.z * 0.165
    eye = Vector((0.05, 0.06, eye_z))
    sections = [(-0.045, -0.032, 0.032, 0.017), (0.0, -0.036, 0.036, 0.018), (0.06, -0.025, 0.027, 0.011),
                (0.11, -0.034, 0.033, 0.005), (0.165, -0.05, 0.042, 0.0015)]
    centers = [eye + b * s for s, _, _, _ in sections]
    profiles = [[(-t, lo), (t, lo), (t, hi), (-t, hi)] for _, lo, hi, t in sections]
    lc.tube(m, centers, None, strip='H3', up=h, profiles=profiles)
    bow = -b * 0.022
    handle = [eye - h * 0.03, eye + h * 0.2 + bow * 0.6, eye + h * 0.45 + bow, eye + h * 0.68 + bow * 0.4, eye + h * 0.74]
    lc.rod(m, handle, 0.016, strip='B', sides=6, radii=[0.016, 0.0155, 0.0145, 0.0155, 0.019])
    # Chips, and two split billets on the ground.
    for k in range(13):
        a = 2.0 * math.pi * k / 13 + rng.uniform(-0.2, 0.2)
        r = rng.uniform(0.3, 0.7)
        size = rng.uniform(0.05, 0.1)
        outline = [(math.cos(2.0 * math.pi * j / 5) * size * rng.uniform(0.6, 1.0),
                    math.sin(2.0 * math.pi * j / 5) * size * rng.uniform(0.25, 0.45)) for j in range(5)]
        thick = rng.uniform(0.008, 0.014)
        matrix = (Matrix.Translation((math.cos(a) * r, math.sin(a) * r, thick + 0.001)) @
                  Matrix.Rotation(rng.uniform(0.0, 6.28), 4, 'Z') @ Matrix.Rotation(math.radians(rng.uniform(-96.0, -84.0)), 4, 'X'))
        lc.prism_part(m, outline, thick, matrix, uv=Trim('B', lane=0, v=0.14))
    for k, (x, y, yaw) in enumerate(((-0.42, 0.18, 30.0), (0.3, -0.38, -62.0))):
        d = Vector((math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0.0))
        c = Vector((x, y, 0.055))
        m.cylinder(tuple(c - d * 0.2), tuple(c + d * 0.2), 0.075, sides=3, uv=Trim('B'), face=(0.0, 0.0, 1.0),
                   phase=rng.uniform(0.0, 360.0), cap_uv=kit.EndGrain(), cap_mat='endgrain')
    ring_hull(m, (0.0, 0.0, 0.0), R + 0.01, 0.0, H + 0.02)
    return m.done('the ground under the block\'s middle', 'A round of log with the axe sunk in it,\nchips and split '
                  'billets round it.', ao=0.35)


# --- HandPump ---

def hand_pump(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    top = 0.15
    for x in (-0.5, 0.0, 0.5):
        x += rng.uniform(-0.02, 0.02)
        m.board((x, -0.625, 0.0575), (x, 0.625, 0.0575), 0.115, 0.1, face=(1.0, 0.0, 0.0), uv=Trim('C', lane='each'),
                bevel=0.008)
    for k, y in enumerate((-0.5, -0.3, -0.1, 0.1, 0.3, 0.5)):
        short = 0.12 if k == 0 else 0.0
        x0, x1 = -0.6 + rng.uniform(-0.02, 0.02), 0.6 - short + rng.uniform(-0.02, 0.02)
        lc.plank(m, (x0, y + rng.uniform(-0.006, 0.006), top - 0.0175), (x1, y, top - 0.0175), 0.188, 0.035,
                 face=(0.0, 0.0, 1.0), uv='A', sag=rng.uniform(-0.004, 0.006), twist=rng.uniform(-2.0, 2.0), bevel=0.004)
    py = 0.28
    m.board((0.0, py, top - 0.01), (0.0, py, top + 0.48), 0.2, 0.2, face=(0.0, -1.0, 0.0), uv=Trim('C', lane='each'),
            bevel=0.012)
    z0 = top + 0.48
    lc.turned(m, [(0.0, z0), (0.088, z0), (0.088, z0 + 0.018), (0.056, z0 + 0.03), (0.05, z0 + 0.1), (0.058, z0 + 0.13),
                  (0.069, z0 + 0.15), (0.071, z0 + 0.32), (0.079, z0 + 0.33), (0.079, z0 + 0.35), (0.05, z0 + 0.362),
                  (0.0, z0 + 0.37)], 12, at=(0.0, py, 0.0), strip='H3', grain='around')
    lc.rod(m, lc.spline([(0.0, py - 0.05, z0 + 0.29), (0.0, py - 0.14, z0 + 0.29), (0.0, py - 0.2, z0 + 0.265),
                         (0.0, py - 0.218, z0 + 0.21)], per=2), 0.022, strip='H3', sides=8,
           radii=[0.024, 0.023, 0.022, 0.021, 0.02, 0.019, 0.021])
    # The handle: a yoke on top, the lever up over the back, a rod down into the cap.
    m.box((0.05, 0.04, 0.05), at=(0.0, py + 0.06, z0 + 0.39), uv='H3')
    lever = lc.spline([(0.0, py - 0.04, z0 + 0.4), (0.0, py + 0.08, z0 + 0.43), (0.0, py + 0.27, z0 + 0.5),
                       (0.0, py + 0.44, z0 + 0.52), (0.0, py + 0.5, z0 + 0.48)], per=2)
    lc.tube(m, lever, lc.rect(0.032, 0.02), strip='H3', up=(1.0, 0.0, 0.0),
            scales=[1.0] * 6 + [0.95, 0.9, 0.95])
    lc.rod(m, [(0.0, py, z0 + 0.36), (0.0, py - 0.012, z0 + 0.41)], 0.008, strip='H3', sides=6)
    bucket(m, (0.0, py - 0.25, top), seed_turn=-5.0, water=0.17, bail_down=1.0)
    m.hull((1.25, 1.25, top), at=(0.0, 0.0, top * 0.5))
    m.hull((0.24, 0.42, z0 + 0.45 - top), at=(0.0, py + 0.02, top + (z0 + 0.45 - top) * 0.5))
    return m.done('the ground under the platform\'s middle', 'A cast-iron pitcher pump on a timber post and a\n'
                  'plank platform; a bucket of water under the spout.', ao=0.4)


# --- MilkCans ---

def can_profile(h, r, lid=True):
    body = [(0.0, 0.003), (r - 0.006, 0.0), (r, 0.012), (r, 0.042), (r - 0.006, 0.05), (r - 0.006, 0.6 * h),
            (0.62 * r, 0.8 * h), (0.52 * r, 0.84 * h), (0.52 * r, 0.93 * h), (0.64 * r, 0.945 * h)]
    if lid:
        return body + [(0.6 * r, 0.975 * h), (0.26 * r, 0.997 * h), (0.0, h + 0.004)]
    return body + [(0.6 * r, 0.958 * h), (0.47 * r, 0.95 * h), (0.47 * r, 0.86 * h), (0.0, 0.86 * h)]


def milk_cans(name, seed):
    m = lc.Model(name, seed)
    cans = [((-0.17, 0.06), 0.62, 0.165, True, 12.0), ((0.17, 0.1), 0.52, 0.15, True, -20.0),
            ((0.02, -0.24), 0.5, 0.145, False, 40.0)]
    for (x, y), h, r, lid, turn in cans:
        lc.turned(m, can_profile(h, r, lid), 14, at=(x, y, 0.0), key='tin', grain='around', phase=math.radians(turn),
                  density=1024.0)
        for s in (-1.0, 1.0):
            a = math.radians(turn) + (0.0 if s > 0 else math.pi)
            out = Vector((math.cos(a), math.sin(a), 0.0))
            side = Vector((-out.y, out.x, 0.0))
            z = 0.73 * h
            c = Vector((x, y, 0.0))
            pts = [c + out * (0.66 * r) + side * 0.035 + UP * z, c + out * (0.98 * r) + side * 0.035 + UP * (z + 0.025),
                   c + out * (1.04 * r) + UP * (z + 0.03), c + out * (0.98 * r) - side * 0.035 + UP * (z + 0.025),
                   c + out * (0.66 * r) - side * 0.035 + UP * z]
            lc.rod(m, pts, 0.0055, key='tin', sides=5)
        ring_hull(m, (x, y, 0.0), r + 0.005, 0.0, h)
    # The open can's lid, on the ground beside it.
    r = 0.145
    lc.turned(m, [(0.0, 0.0), (0.64 * r, 0.0), (0.62 * r, 0.03), (0.3 * r, 0.045), (0.0, 0.05)], 14,
              at=(0.25, -0.33, 0.0), rot=(8.0, -5.0, 0.0), key='tin', grain='around', density=1024.0)
    return m.done('the ground under the group\'s middle', 'Three tinned milk cans (MailboxTin), one open\n'
                  'with its lid on the ground.', ao=0.3)


# --- KitchenGarden ---

def garden_height(x, y):
    """The hoed ridges (along X, every 0.4 m), worn down toward the edges, with clods."""
    edge = min(1.0, (1.5 - abs(x)) / 0.16) * min(1.0, (1.0 - abs(y)) / 0.12)
    ridge = 0.036 + 0.034 * math.cos(2.0 * math.pi * y / 0.4)
    clods = 0.012 * noise.noise(Vector((x * 4.0, y * 6.0, 0.7))) + 0.006 * noise.noise(Vector((x * 13.0, y * 15.0, 2.1)))
    return max(0.0, edge) * (ridge + clods)


def kitchen_garden(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    # The soil: a grid over the patch and a skirt round it tucked under the ground.
    nx, ny = 9, 25
    cos, faces = [], []
    sc = lt.SETS['GroundDirt']['density'] / float(lt.SETS['GroundDirt']['size'])
    grid = []
    for j in range(ny):
        row = []
        for i in range(nx):
            x = -1.5 + 3.0 * i / (nx - 1)
            y = -1.0 + 2.0 * j / (ny - 1)
            row.append(len(cos))
            cos.append(Vector((x, y, garden_height(x, y))))
        grid.append(row)
    for j in range(ny - 1):
        for i in range(nx - 1):
            idx = lc._orient(cos, [grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]], UP)
            faces.append((idx, [(cos[k].x * sc + 0.2, cos[k].y * sc + 0.4) for k in idx]))
    rim = ([grid[0][i] for i in range(nx)] + [grid[j][nx - 1] for j in range(1, ny)] +
           [grid[ny - 1][i] for i in range(nx - 2, -1, -1)] + [grid[j][0] for j in range(ny - 2, 0, -1)])
    skirt = []
    for k in rim:
        p = cos[k]
        out = Vector((math.copysign(0.06, p.x) if abs(abs(p.x) - 1.5) < 1e-6 else 0.0,
                      math.copysign(0.06, p.y) if abs(abs(p.y) - 1.0) < 1e-6 else 0.0, 0.0))
        skirt.append(len(cos))
        cos.append(Vector((p.x + out.x, p.y + out.y, -0.04)))
    for k in range(len(rim)):
        a, b = rim[k], rim[(k + 1) % len(rim)]
        c, d = skirt[(k + 1) % len(rim)], skirt[k]
        mid = (cos[a] + cos[b]) * 0.5
        idx = lc._orient(cos, [a, b, c, d], Vector((mid.x, mid.y, 0.5)))
        faces.append((idx, [(cos[q].x * sc + 0.2, cos[q].y * sc + 0.4) for q in idx]))
    m.put(cos, faces, key='dirt')
    # Dry corn on the back ridge (the trim's pale log wood, strip B: straw-coloured): stalks, long leaves arching out
    # and hanging, a husked ear or two, tassels; one stalk snapped over.
    for k in range(7):
        x = -1.25 + 0.415 * k + rng.uniform(-0.06, 0.06)
        y = 0.8 + rng.uniform(-0.05, 0.05)
        base = Vector((x, y, garden_height(x, y) - 0.01))
        h = rng.uniform(1.3, 1.8)
        lean = Vector((rng.uniform(-0.09, 0.09), rng.uniform(-0.12, 0.06), 1.0)).normalized()
        sway = Vector((rng.uniform(-0.06, 0.06), rng.uniform(-0.06, 0.06), 0.0))
        pts = [base + lean * h * t + sway * t * t for t in (0.0, 0.3, 0.6, 0.85, 1.0)]
        if k == 4:
            knee = base + lean * 0.72
            pts = [base, base + lean * 0.36, knee, knee + Vector((0.12, -0.08, -0.05)), knee + Vector((0.22, -0.2, -0.36))]
        lc.rod(m, pts, 0.012, strip='B', sides=5, radii=[0.013, 0.011, 0.009, 0.007, 0.004], lane=0)
        for n in range(5):
            t = 0.16 + 0.15 * n + rng.uniform(-0.03, 0.03)
            node = pts[0].lerp(pts[2], t / 0.6) if t < 0.6 else pts[2].lerp(pts[3], (t - 0.6) / 0.25)
            a = rng.uniform(0.0, 6.28)
            out = Vector((math.cos(a), math.sin(a), 0.0))
            ln = rng.uniform(0.45, 0.75) * (1.0 - 0.25 * n / 4)
            droop = rng.uniform(0.35, 0.75)
            leaf = [node, node + out * ln * 0.22 + UP * ln * 0.16, node + out * ln * 0.5 + UP * ln * 0.1,
                    node + out * ln * 0.75 - UP * ln * droop * 0.4, node + out * ln * 0.85 - UP * ln * droop]
            lc.strap(m, leaf, [0.04, 0.07, 0.06, 0.04, 0.008], None, 0.002, strip='B', lane_v=0.2, across=out.cross(UP))
        if k != 4:
            tip = pts[-1]
            for n in range(2):
                a = math.pi * n + k
                out = Vector((math.cos(a), math.sin(a), 0.0))
                lc.strap(m, [tip, tip + out * 0.06 + UP * 0.06, tip + out * 0.12 - UP * 0.02], [0.012, 0.01, 0.003], None,
                         0.002, strip='B', lane_v=0.2, across=out.cross(UP))
        if k % 2 == 0:
            node = pts[0].lerp(pts[2], 0.75)
            d = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), 1.6)).normalized()
            lc.turned(m, [(0.0, 0.0), (0.022, 0.03), (0.025, 0.1), (0.013, 0.17), (0.0, 0.2)], 5, at=node + d * 0.012,
                      rot=along((0, 0, 0), d)[0], strip='B', grain='up', lane=0)
    # Two bean tepees: three poles tied at the top, dead vines up two of them, pods hanging.
    for cx in (-0.7, 0.55):
        cy = 0.38
        apex = Vector((cx + rng.uniform(-0.03, 0.03), cy, 1.62))
        feet = [Vector((cx + 0.22 * math.cos(a), cy + 0.2 * math.sin(a), 0.0)) for a in (0.4, 2.5, 4.4)]
        for n, f in enumerate(feet):
            f.z = garden_height(f.x, f.y) - 0.05
            d = (apex - f).normalized()
            lc.rod(m, [f, apex + d * 0.14], 0.014, strip='A', sides=5, radii=[0.015, 0.011])
            if n < 2:
                vine = []
                for s in range(9):
                    t = s / 8 * 0.86
                    p = f.lerp(apex, t)
                    side = d.cross(UP).normalized()
                    other = d.cross(side)
                    ang = s * 1.9 + n
                    vine.append(p + (side * math.cos(ang) + other * math.sin(ang)) * 0.022)
                lc.rod(m, vine, 0.0045, strip='C', sides=3)
                for s in (3, 5, 7):
                    q = vine[s]
                    lc.strap(m, [q, q + Vector((0.01, -0.01, -0.06)), q + Vector((0.02, -0.025, -0.12))],
                             [0.012, 0.015, 0.004], (0.7, 0.7, 0.0), 0.004, strip='C')
    # Dried squash leaves sprawled on the middle ridge.
    for k in range(5):
        x = -1.0 + 0.5 * k + rng.uniform(-0.1, 0.1)
        y = rng.uniform(-0.06, 0.06)
        base = Vector((x, y, garden_height(x, y)))
        a = rng.uniform(0.0, 6.28)
        out = Vector((math.cos(a), math.sin(a), 0.0))
        side = Vector((-out.y, out.x, 0.0))
        size = rng.uniform(0.12, 0.2)
        rows = []
        for t in (0.0, 0.5, 1.0):
            row = []
            for s in (-1.0, 0.0, 1.0):
                w = size * 0.5 * (0.3 + math.sin(math.pi * min(t + 0.2, 1.0)))
                curl = 0.03 * (s * s) + 0.02 * t
                row.append(base + out * (t * size) + side * (s * w) + UP * (0.012 + curl + rng.uniform(-0.005, 0.005)))
            rows.append(row)
        lc.cloth(m, rows, 0.003, strip='C', v0=0.03)
    # The stake fence round the back and the left, two rails of rough poles; a hoe leaning in the corner.
    back = [(-1.62 + 0.32 * k, 1.16) for k in range(10)]
    left = [(-1.62, 1.16 - 0.32 * k) for k in range(1, 8)]
    for n, (x, y) in enumerate(back + left):
        h = rng.uniform(0.7, 0.86)
        outline = [(-0.025, -0.12), (0.025, -0.12), (0.025, h), (0.004, h + 0.06), (-0.025, h - 0.01)]
        yaw = 0.0 if n < len(back) else 90.0
        matrix = (Matrix.Translation((x, y, 0.0)) @ Matrix.Rotation(math.radians(yaw + rng.uniform(-8.0, 8.0)), 4, 'Z')
                  @ Matrix.Rotation(math.radians(rng.uniform(-3.0, 3.0)), 4, 'Y')
                  @ Matrix.Rotation(math.radians(rng.uniform(-3.0, 3.0)), 4, 'X') @ Matrix.Translation((0.0, -0.02, 0.0)))
        lc.prism_part(m, outline, 0.04, matrix, uv=Trim('A', rotate=True))
    for z in (0.26, 0.6):
        lc.rod(m, [(-1.66, 1.13, z), (-0.3, 1.13, z - 0.025), (1.3, 1.13, z + 0.01)], 0.02, strip='A', sides=4,
               wobble=0.002)
        lc.rod(m, [(-1.59, 1.18, z + 0.02), (-1.59, 0.0, z - 0.02), (-1.59, -1.1, z + 0.01)], 0.02, strip='A', sides=4,
               wobble=0.002)
    hoe_foot = Vector((-1.3, 0.92, 0.02))
    hoe_top = Vector((-1.48, 1.1, 1.3))
    lc.rod(m, [hoe_foot, hoe_top], 0.013, strip='C', sides=6)
    d = (hoe_top - hoe_foot).normalized()
    m.box((0.15, 0.004, 0.09), at=tuple(hoe_foot + Vector((0.0, -0.02, 0.025))), uv='H3', rot=(-20.0, 0.0, 15.0))
    lr.hull_along(m, (-1.66, 1.16, 0.0), (1.32, 1.16, 0.0), 0.85, thick=0.08)
    lr.hull_along(m, (-1.62, 1.16, 0.0), (-1.62, -1.08, 0.0), 0.85, thick=0.08)
    return m.done('the ground under the patch\'s middle (it lies flat; its skirt reaches 4 cm under)',
                  'A 3 x 2 m patch going over: dry corn, bean\ntepees, dead squash; a stake fence and a hoe.', ao=0.3,
                  darken={'dirt': 0.62})


# --- ChickenCoop ---

def chicken_coop(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    X0, X1, Y0, Y1 = -0.55, 0.55, -0.4, 0.4
    FLOOR = 0.49
    FRONT_H, BACK_H = 0.93, 0.63
    for x in (X0 + 0.05, X1 - 0.05):
        for y in (Y0 + 0.05, Y1 - 0.05):
            m.board((x, y, -0.03), (x, y, FLOOR - 0.02), 0.07, 0.07, face=(0.0, -1.0, 0.0), uv=Trim('C', lane='each'),
                    bevel=0.006)
    m.box((X1 - X0 + 0.02, Y1 - Y0 - 0.02, 0.04), at=(0.0, 0.0, FLOOR - 0.02), uv='A')
    pop = Opening('door', 0.12, 0.0, 0.26, 0.3)
    window = Opening('window', 0.72, 0.52, 0.22, 0.18)
    walls = [
        ((X0, Y0), (X1, Y0), kit.wall_outline(X1 - X0, FRONT_H, [pop]), kit.holes([window]), [pop, window]),
        ((X1, Y0), (X1, Y1), [(0.0, 0.0), (Y1 - Y0, 0.0), (Y1 - Y0, BACK_H), (0.0, FRONT_H)], [], []),
        ((X1, Y1), (X0, Y1), kit.wall_outline(X1 - X0, BACK_H), [], []),
        ((X0, Y1), (X0, Y0), [(0.0, 0.0), (Y1 - Y0, 0.0), (Y1 - Y0, FRONT_H), (0.0, BACK_H)], [], []),
    ]
    for k, (p0, p1, outline, holes, around) in enumerate(walls):
        space = kit.wall_space(p0, p1, FLOOR)
        m.panel(outline, holes, 0.022, uv=Trim('A', world=True, rotate=True), space=space, around=around, slices=0)
        if k == 2:
            continue
        length = (Vector(p1) - Vector(p0)).length
        top = (lambda x: FRONT_H) if k == 0 else (
            (lambda x: FRONT_H + (BACK_H - FRONT_H) * x / length) if k == 1 else (lambda x: BACK_H + (FRONT_H - BACK_H) * x / length))
        for x in kit.frange(0.1, length - 0.05, 0.2):
            if k == 0 and (0.08 < x < 0.42 or 0.68 < x < 0.98):
                continue
            m.board((x, 0.0, 0.0), (x, 0.0, top(x) - 0.01), 0.045, 0.014, face=(0.0, -1.0, 0.0), uv='A', space=space,
                    lift=0.007)
    # The window's two laths, the pop door's leaf swung open against the wall.
    front = kit.wall_space((X0, Y0), (X1, Y0), FLOOR)
    for z in (0.57, 0.64):
        m.board((0.7, 0.0, z), (0.96, 0.0, z), 0.025, 0.012, face=(0.0, -1.0, 0.0), uv='A', space=front, lift=0.012)
    m.box((0.27, 0.02, 0.31), at=(0.56, -0.03, 0.155), rot=(0.0, 0.0, -12.0), uv=Trim('A', rotate=True), space=front)
    # The roof: planks down the slope, overhanging all round.
    rise = FRONT_H - BACK_H
    run = Y1 - Y0
    pitch = math.atan2(rise, run)
    for k in range(6):
        x = X0 - 0.1 + (X1 - X0 + 0.2) * (k + 0.5) / 6
        p_front = Vector((x, Y0 - 0.14, FLOOR + FRONT_H + 0.03 + 0.14 * math.tan(pitch)))
        p_back = Vector((x, Y1 + 0.12, FLOOR + BACK_H + 0.03 - 0.12 * math.tan(pitch)))
        m.board(tuple(p_front + Vector((rng.uniform(-0.005, 0.005), 0.0, 0.0))), tuple(p_back), 0.205, 0.022,
                face=(0.0, math.sin(pitch), math.cos(pitch)), uv='A')
    # The nest box on the right side, its lid sloping, two iron hinges.
    nb = (X1 + 0.15, 0.0, FLOOR + 0.3)
    m.box((0.3, 0.52, 0.3), at=nb, uv=Trim('A', rotate=True))
    m.box((0.36, 0.58, 0.022), at=(X1 + 0.16, 0.0, FLOOR + 0.48), rot=(0.0, 18.0, 0.0), uv='A')
    for y in (-0.15, 0.15):
        m.box((0.08, 0.025, 0.006), at=(X1 + 0.03, y, FLOOR + 0.505), rot=(0.0, 18.0, 0.0), uv='H3')
    # The ramp from the pop door down into the run, with cleats.
    door_x = X0 + 0.25
    r0, r1 = Vector((door_x, Y0 - 0.01, FLOOR - 0.01)), Vector((door_x, Y0 - 0.82, 0.01))
    m.board(tuple(r0), tuple(r1), 0.24, 0.022, face=(0.0, -0.5, 1.0), uv='A')
    for k in range(5):
        c = r0.lerp(r1, 0.12 + 0.18 * k) + Vector((0.0, 0.0, 0.018))
        m.box((0.22, 0.02, 0.014), at=tuple(c), rot=(math.degrees(math.atan2(FLOOR, 0.82)), 0.0, 0.0), uv='A')
    # The run: a lath fence on three sides of the coop's front, its back closed by the coop and a short stretch.
    RX0, RX1, RY0 = -1.6, X1, -1.75
    runs = [((RX0, Y0), (RX0, RY0)), ((RX0, RY0), (RX1, RY0)), ((RX1, RY0), (RX1, Y0)), ((RX0, Y0), (X0, Y0))]
    for p0, p1 in runs:
        p0, p1 = Vector((p0[0], p0[1], 0.0)), Vector((p1[0], p1[1], 0.0))
        d = (p1 - p0)
        length = d.length
        d.normalize()
        out = Vector((d.y, -d.x, 0.0))
        for z in (0.1, 0.7):
            m.board(tuple(p0 + UP * z), tuple(p1 + UP * z), 0.07, 0.025, face=tuple(out), uv='A')
        count = int(length / 0.11)
        for k in range(1, count):
            p = p0 + d * (length * k / count) + out * 0.02
            h = 0.8 + rng.uniform(-0.04, 0.03)
            m.box((0.034, 0.012, h), at=tuple(p + UP * (h * 0.5)), rot=(0.0, 0.0, math.degrees(math.atan2(d.y, d.x))),
                  uv=Trim('A', rotate=True), drop=('-z',))
    for x, y in ((RX0, Y0), (RX0, RY0), (RX1, RY0)):
        m.board((x, y, -0.05), (x, y, 0.86), 0.07, 0.07, face=(0.0, -1.0, 0.0), uv=Trim('C', lane='each'), bevel=0.006)
    m.hull_points([Vector((x, y, z)) for x in (X0 - 0.1, X1 + 0.1) for y in (Y0 - 0.14, Y1 + 0.12)
                   for z in (0.0, FLOOR + (FRONT_H if y < 0.0 else BACK_H) + 0.06)])
    m.hull((0.3, 0.52, 0.32), at=(X1 + 0.15, 0.0, FLOOR + 0.31))
    for p0, p1 in runs:
        lr.hull_along(m, (p0[0], p0[1], 0.0), (p1[0], p1[1], 0.0), 0.85, thick=0.06)
    # SOCKET_Run: the middle of the run on the ground, facing out of it (-Y), for whatever keeps the chickens there.
    m.socket('Run', ((RX0 + RX1) * 0.5, (RY0 + Y0) * 0.5, 0.0))
    move = m.center_footprint()
    lt._log(f'CLUTTER ChickenCoop: the run inside x {RX0 + 0.03 + move.x:.2f}..{RX1 - 0.03 + move.x:.2f}, '
            f'y {RY0 + 0.03 + move.y:.2f}..{Y0 - 0.02 + move.y:.2f} from the pivot (the ramp comes down into it)')
    return m.done('the ground under the middle of the coop and its run', 'A board-and-batten coop on legs, its ramp\n'
                  'and nest box, and a lath-fenced run (SOCKET_Run).', ao=0.4)


# --- Spittoon ---

def spittoon(name, seed):
    m = lc.Model(name, seed)
    lc.turned(m, [(0.0, 0.0), (0.112, 0.0), (0.12, 0.006), (0.118, 0.016), (0.127, 0.04), (0.129, 0.072),
                  (0.117, 0.1), (0.085, 0.124), (0.064, 0.134), (0.072, 0.146), (0.11, 0.166), (0.118, 0.175),
                  (0.11, 0.181), (0.058, 0.158), (0.04, 0.138), (0.0, 0.132)], 16, key='brass', grain='up',
              density=1280.0)
    ring_hull(m, (0.0, 0.0, 0.0), 0.13, 0.0, 0.18)
    return m.done('the floor under its middle', 'A brass spittoon (BrassWorn).', ao=0.15)


# --- BottleCrate ---

def bottle(m, at, rot=None, full=True, tall=True, z0=0.12, sides=6):
    """A bottle on the dark glass strip: the whole of it (full) or only what shows over a crate's dividers."""
    if tall:
        prof = [(0.031, 0.19), (0.024, 0.226), (0.012, 0.252), (0.012, 0.292), (0.015, 0.297), (0.013, 0.305), (0.0, 0.304)]
        base = [(0.0, 0.004), (0.029, 0.0), (0.031, 0.012)]
    else:
        prof = [(0.036, 0.15), (0.03, 0.18), (0.013, 0.2), (0.012, 0.235), (0.015, 0.24), (0.013, 0.247), (0.0, 0.246)]
        base = [(0.0, 0.004), (0.034, 0.0), (0.036, 0.012)]
    profile = (base if full else [(prof[0][0], z0)]) + prof
    lc.turned(m, profile, sides, at=at, rot=rot, strip='H4', grain='fit')


def bottle_crate(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    W, D, H = 0.5, 0.34, 0.26
    for side in (-1.0, 1.0):
        matrix = kit.basis((side * (W * 0.5 - 0.011), 0.0, 0.0), (0.0, side, 0.0), (-side, 0.0, 0.0), (0.0, 0.0, 1.0))
        outline = [(-D * 0.5, 0.0), (D * 0.5, 0.0), (D * 0.5, H), (-D * 0.5, H)]
        hole = [(-0.055, H - 0.06), (-0.055, H - 0.03), (0.055, H - 0.03), (0.055, H - 0.06)]
        lc.prism_part(m, [[(x, z) for x, z in outline], hole], 0.022, matrix @ Matrix.Translation((0.0, -0.011, 0.0)),
                      uv=Trim('A'))
        for z in (0.065, 0.195):
            m.board((-W * 0.5 + 0.022, side * (D * 0.5 - 0.006), z), (W * 0.5 - 0.022, side * (D * 0.5 - 0.006), z), 0.1, 0.012,
                    face=(0.0, side, 0.0), uv='A')
    for y in (-0.11, 0.0, 0.11):
        m.board((-W * 0.5 + 0.022, y, 0.006), (W * 0.5 - 0.022, y, 0.006), 0.1, 0.012, face=(0.0, 0.0, 1.0), uv='A')
    for y in (-0.0525, 0.0525):
        m.box((W - 0.044, 0.006, 0.14), at=(0.0, y, 0.082), uv='A', drop=('-z',))
    for x in (-0.114, 0.0, 0.114):
        m.box((0.006, D - 0.024, 0.14), at=(x, 0.0, 0.082), uv='A', drop=('-z',))
    cells = [(x, y) for x in (-0.171, -0.057, 0.057, 0.171) for y in (-0.105, 0.0, 0.105)]
    empty = {1, 6, 10, 11}
    for k, (x, y) in enumerate(cells):
        if k in empty:
            continue
        tilt = (rng.uniform(-4.0, 4.0), rng.uniform(-4.0, 4.0), 0.0)
        bottle(m, (x, y, 0.012), rot=tilt, full=False, tall=k % 4 != 2, z0=0.13)
    bottle(m, (0.4, -0.06, 0.0), tall=True, sides=8)
    bottle(m, (0.38, 0.2, 0.031), rot=(90.0, 0.0, 70.0), tall=True, sides=8)
    bottle(m, (-0.42, -0.22, 0.036), rot=(90.0, 0.0, -25.0), tall=False, sides=8)
    m.hull((W, D, H), at=(0.0, 0.0, H * 0.5))
    return m.done('the ground under the crate\'s middle', 'A slatted crate of empties (dark glass on the\n'
                  'trim) and three loose bottles.', ao=0.3)


# --- FlourSacks ---

MILL = ('ORRIN BROS.', 'FLOUR')


class Sack:
    """A stuffed sack: a pillow of two faces sewn round their edge (length along local X, width along Y, its faces
    toward +-Z), placed by matrix and pressed flat where it meets the ground (floor) or what it lies on."""

    def __init__(self, length, width, thick, seed, nu=9, nv=6):
        rng = random.Random(seed)
        self.L, self.W, self.T = length, width, thick
        self.nu, self.nv = nu, nv
        self.top, self.bot = {}, {}
        off = Vector((rng.uniform(0, 50), rng.uniform(0, 50), rng.uniform(0, 50)))
        for j in range(nv):
            v = -1.0 + 2.0 * j / (nv - 1)
            for i in range(nu):
                u = -1.0 + 2.0 * i / (nu - 1)
                x = u * length * 0.5 * (1.0 - 0.07 * (1.0 - v * v))
                y = v * width * 0.5 * (1.0 - 0.1 * (1.0 - u * u))
                if abs(u) == 1.0 and abs(v) == 1.0:
                    x *= 1.01
                    y *= 1.015
                bulge = max(0.0, 1.0 - abs(u) ** 6) ** 0.55 * max(0.0, 1.0 - abs(v) ** 4) ** 0.55
                lump = 0.012 * noise.noise(Vector((x * 6.0, y * 6.0, 0.0)) + off)
                seam = 0.006 * noise.noise(Vector((x * 9.0, y * 9.0, 3.0)) + off)
                self.top[(i, j)] = Vector((x, y, (thick * 0.5 + lump) * bulge + seam * (1.0 - bulge)))
                self.bot[(i, j)] = Vector((x, y, -(thick * 0.5 - lump) * bulge * 0.95 + seam * (1.0 - bulge)))

    def edge(self, i, j):
        return i in (0, self.nu - 1) or j in (0, self.nv - 1)

    def surface(self, x, y):
        """The point on the top face over (x, y) (the grid's own bilinear surface)."""
        fu = (x / (self.L * 0.5) + 1.0) * 0.5 * (self.nu - 1)
        fv = (y / (self.W * 0.5) + 1.0) * 0.5 * (self.nv - 1)
        i, j = min(int(fu), self.nu - 2), min(int(fv), self.nv - 2)
        a, b = fu - i, fv - j
        p00, p10 = self.top[(i, j)], self.top[(i + 1, j)]
        p01, p11 = self.top[(i, j + 1)], self.top[(i + 1, j + 1)]
        return (p00 * (1 - a) + p10 * a) * (1 - b) + (p01 * (1 - a) + p11 * a) * b

    def emit(self, m, matrix, floor=0.0, u0=5.88, v0=0.1):
        def place(p):
            q = matrix @ p
            return Vector((q.x, q.y, max(q.z, floor)))
        cos, index = [], {}
        for key in sorted(self.top):
            index[('t',) + key] = len(cos)
            cos.append(place(self.top[key]))
            if self.edge(*key):
                index[('b',) + key] = index[('t',) + key]
            else:
                index[('b',) + key] = len(cos)
                cos.append(place(self.bot[key]))
        normal = matrix.to_3x3() @ UP
        faces = []
        for face, sign in (('t', 1.0), ('b', -1.0)):
            src = self.top if face == 't' else self.bot
            for j in range(self.nv - 1):
                for i in range(self.nu - 1):
                    keys = [(i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1)]
                    idx = [index[(face,) + k] for k in keys]
                    uvs = {index[(face,) + k]: lc.strip_uv('G', u0 + src[k].x, v0 + src[k].y + self.W * 0.5) for k in keys}
                    idx = lc._orient(cos, idx, normal * sign)
                    faces.append((idx, [uvs[q] for q in idx]))
        # The seams fold the two faces together, which the occlusion bake reads as a crevice: keep them light.
        m.put(cos, faces, ao_floor=0.55)

    def print_on(self, m, matrix, lines, floor=0.0, upright=False):
        """The mill's name printed on the top face: (text, height, y offset, strip) per line. Lying, the text runs along
        the sack; upright (stood on end), across it, reading up its length."""
        rot = matrix.to_3x3()
        normal = (rot @ UP).normalized()
        for body, height, dy, strip in lines:
            def mapper(x, y, dy=dy):
                lx, ly = (y + dy, -x) if upright else (x, y + dy)
                p = self.surface(lx, ly) + Vector((0.0, 0.0, 0.004))
                q = matrix @ p
                return Vector((q.x, q.y, max(q.z, floor + 0.004))), normal
            lc.lettering(m, body, height, mapper, strip=strip, width=self.W * 0.82 if upright else self.L * 0.62,
                         tol=0.03, v0=0.05)


def flour_sacks(name, seed):
    m = lc.Model(name, seed)
    L, W, T = 0.88, 0.5, 0.2
    print_lines = [(MILL[0], 0.062, 0.075, 'H2'), (MILL[1], 0.1, -0.055, 'H1')]
    # Two lying side by side, lengths along Y; one across them; one stood up in front; one slumped at the right.
    placed = [
        (Matrix.Translation((-0.27, 0.15, 0.085)) @ Matrix.Rotation(math.radians(92.0), 4, 'Z'), 0.0, None),
        (Matrix.Translation((0.26, 0.13, 0.085)) @ Matrix.Rotation(math.radians(86.0), 4, 'Z'), 0.0, None),
        (Matrix.Translation((0.0, 0.2, 0.245)) @ Matrix.Rotation(math.radians(3.0), 4, 'Z'), 0.15, 'lying'),
        (Matrix.Translation((-0.12, -0.42, 0.42)) @ Matrix.Rotation(math.radians(-6.0), 4, 'Z')
         @ Matrix.Rotation(math.radians(-9.0), 4, 'X')
         @ Matrix(((0.0, -1.0, 0.0, 0.0), (0.0, 0.0, -1.0, 0.0), (1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0))), 0.0, 'upright'),
        (Matrix.Translation((0.52, -0.44, 0.1)) @ Matrix.Rotation(math.radians(-34.0), 4, 'Z')
         @ Matrix.Rotation(math.radians(14.0), 4, 'X'), 0.0, None),
    ]
    for k, (matrix, floor, printed) in enumerate(placed):
        sack = Sack(L * (0.96 if k == 4 else 1.0), W, T, seed * 10 + k)
        sack.emit(m, matrix, floor=floor, u0=5.86 + 0.02 * k, v0=0.05 + 0.05 * k)
        if printed:
            sack.print_on(m, matrix, print_lines, floor=floor, upright=printed == 'upright')
    m.hull_points([Vector((x, y, z)) for x, y in ((-0.56, 0.62), (0.53, 0.62), (0.9, -0.25), (0.65, -0.72),
                                                    (-0.42, -0.6), (-0.56, -0.25)) for z in (0.0, 0.33)] +
                  [Vector((x, y, 0.86)) for x, y in ((-0.35, -0.42), (0.1, -0.42), (-0.35, -0.25), (0.1, -0.25))])
    return m.done('the ground under the stack\'s middle', 'Five sacks of ORRIN BROS. FLOUR (an invented\n'
                  'mill), one stood up against the stack.', ao=0.35)


# --- Buckboard ---

def wheel(m, center, radius, spokes, seed, width=0.05, felloe=0.05, hub=0.065, hub_len=0.2, segs=14):
    """A spoked wheel on an axle along X at center: an iron tyre on a faded oxide-red felloe, spokes and hub."""
    rng = random.Random(seed)
    rot = Matrix.Rotation(math.radians(90.0), 3, 'Y')
    inner = radius - felloe
    c = Vector(center)
    lc.turned(m, [(inner, -width * 0.5), (radius - 0.012, -width * 0.5)], segs, at=c, rot=rot, strip='H2', grain='around')
    lc.turned(m, [(radius - 0.012, -width * 0.56), (radius, -width * 0.52), (radius, width * 0.52), (radius - 0.012, width * 0.56)],
              segs, at=c, rot=rot, strip='H3', grain='around')
    lc.turned(m, [(radius - 0.012, width * 0.5), (inner, width * 0.5), (inner, -width * 0.5)], segs, at=c, rot=rot,
              strip='H2', grain='around')
    lc.turned(m, [(0.0, -hub_len * 0.5), (hub * 0.62, -hub_len * 0.5), (hub, -hub_len * 0.2), (hub, hub_len * 0.2),
                  (hub * 0.62, hub_len * 0.5), (0.0, hub_len * 0.5)], 8, at=c, rot=rot, strip='H2', grain='up')
    start = rng.uniform(0.0, math.pi)
    for k in range(spokes):
        a = start + 2.0 * math.pi * k / spokes
        d = Vector((0.0, math.cos(a), math.sin(a)))
        lc.tube(m, [c + d * (hub * 0.8), c + d * (inner + 0.012)], lc.rect(0.022, 0.03), strip='H2', up=(1.0, 0.0, 0.0),
                scales=[1.0, 0.8], caps=(False, False))


def wheel_hull(m, center, radius, width=0.07):
    c = Vector(center)
    m.hull_points([c + Vector((dx, math.cos(a) * radius, math.sin(a) * radius)) for a in
                   (k * math.pi / 4 for k in range(8)) for dx in (-width * 0.5, width * 0.5)])


def buckboard(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    RR, RF = 0.56, 0.46
    YR, YF = 0.78, -0.8
    TRACK = 0.7
    BED = 0.8                      # the bed boards' top
    for side in (-1.0, 1.0):
        wheel(m, (side * TRACK, YR, RR), RR, 12, seed + int(side + 2), segs=18)
        wheel(m, (side * TRACK, YF, RF), RF, 10, seed + int(side + 6), segs=16)
    for y, r in ((YR, RR), (YF, RF)):
        m.box((1.3, 0.09, 0.08), at=(0.0, y, r), uv=Trim('C', lane='each'))
        lc.rod(m, [(-TRACK - 0.12, y, r), (TRACK + 0.12, y, r)], 0.024, strip='H3', sides=6)
        top = r + 0.04
        height = BED - 0.025 - top
        m.box((1.08, 0.11, height), at=(0.0, y, top + height * 0.5), uv=Trim('C', lane='each'))
    # Leaf springs over the rear axle.
    for side in (-1.0, 1.0):
        pts = [(side * 0.42, YR + dy, RR + 0.045 + 0.06 * (1.0 - (dy / 0.3) ** 2)) for dy in (-0.3, -0.15, 0.0, 0.15, 0.3)]
        lc.tube(m, pts, lc.rect(0.05, 0.018), strip='H3', up=(1.0, 0.0, 0.0))
    # The bed: long boards from bolster to bolster, flexing a little in the middle; low side rails.
    for k in range(6):
        x = -0.43 + 0.172 * k
        lc.plank(m, (x, -1.18, BED - 0.0125), (x, 1.22, BED - 0.0125), 0.162, 0.025, face=(0.0, 0.0, 1.0), uv='A',
                 sag=0.03, cuts=2, twist=rng.uniform(-1.0, 1.0))
    for side in (-1.0, 1.0):
        m.board((side * 0.52, -1.18, BED + 0.05), (side * 0.52, 1.22, BED + 0.05), 0.11, 0.025, face=(side, 0.0, 0.0), uv='A',
                cuts=2)
    m.board((-0.53, 1.225, BED + 0.05), (0.53, 1.225, BED + 0.05), 0.11, 0.022, face=(0.0, 1.0, 0.0), uv='A')
    # The dash: a board raked forward over the front, on iron stays; a footboard.
    dash0, dash1 = Vector((0.0, -1.16, BED + 0.02)), Vector((0.0, -1.32, BED + 0.33))
    m.board(tuple(dash0 + Vector((-0.5, 0.0, 0.0))), tuple(dash0 + Vector((0.5, 0.0, 0.0))), 0.04, 0.03,
            face=(0.0, -1.0, 0.0), uv='C')
    mid = (dash0 + dash1) * 0.5
    m.box((1.0, 0.025, 0.34), at=tuple(mid), rot=(-27.0, 0.0, 0.0), uv=Trim('C'))
    for side in (-1.0, 1.0):
        lc.rod(m, [(side * 0.46, -1.12, BED + 0.01), (side * 0.46, -1.28, BED + 0.26)], 0.008, strip='H3', sides=4)
    # Two seats: a riser box, a plank seat, a lazy back on iron stays.
    for y in (-0.42, 0.38):
        m.box((0.96, 0.3, 0.26), at=(0.0, y, BED + 0.13), uv=Trim('C', lane='each'), drop=('-z',))
        m.box((1.0, 0.36, 0.03), at=(0.0, y - 0.01, BED + 0.275), uv='A', bevel=0.006)
        back_z = BED + 0.47
        m.box((0.94, 0.03, 0.12), at=(0.0, y + 0.17, back_z), rot=(-12.0, 0.0, 0.0), uv=Trim('C'))
        for side in (-1.0, 1.0):
            lc.rod(m, [(side * 0.4, y + 0.13, BED + 0.29), (side * 0.41, y + 0.165, back_z - 0.04)], 0.008, strip='H3',
                   sides=4)
    # The shafts, down on the ground in front, a crossbar between them, their tips curled.
    for side in (-1.0, 1.0):
        pts = [(side * 0.36, YF + 0.05, RF + 0.03), (side * 0.35, YF - 0.6, 0.3), (side * 0.33, YF - 1.55, 0.09),
               (side * 0.32, YF - 2.2, 0.035), (side * 0.32, YF - 2.36, 0.06)]
        lc.rod(m, lc.spline(pts, per=2), 0.03, strip='C', sides=6,
               radii=[0.032, 0.031, 0.03, 0.028, 0.026, 0.025, 0.024, 0.023, 0.021])
    lc.rod(m, [(-0.36, YF - 0.42, 0.37), (0.36, YF - 0.42, 0.37)], 0.024, strip='C', sides=6)
    # The brake: a lever by the front seat, a beam with blocks before the rear wheels.
    lc.rod(m, [(0.55, -0.3, BED - 0.02), (0.58, -0.36, BED + 0.55)], 0.011, strip='H3', sides=5)
    lc.rod(m, [(-TRACK - 0.02, YR - RR - 0.06, 0.52), (TRACK + 0.02, YR - RR - 0.06, 0.52)], 0.016, strip='H3', sides=5)
    for side in (-1.0, 1.0):
        m.box((0.07, 0.07, 0.16), at=(side * TRACK, YR - RR - 0.01, 0.53), rot=(-12.0, 0.0, 0.0), uv=Trim('C'))
    lc.rod(m, [(0.56, -0.3, BED - 0.02), (0.5, YR - RR - 0.06, 0.53)], 0.008, strip='H3', sides=4)
    # The hulls: the body, each wheel; none on the shafts.
    m.hull_points([Vector((x, y, z)) for x in (-0.55, 0.55) for y, z in ((-1.3, BED - 0.2), (-1.3, BED + 0.4),
                                                                        (1.24, BED - 0.25), (1.24, BED + 0.1),
                                                                        (0.55, BED + 0.6), (-0.25, BED + 0.6))])
    for side in (-1.0, 1.0):
        wheel_hull(m, (side * TRACK, YR, RR), RR)
        wheel_hull(m, (side * TRACK, YF, RF), RF)
    move = m.center_footprint()
    m.notes['bed_middle'] = tuple(round(c, 3) for c in Vector((0.0, 0.02, 0.0)) + move)
    lt._log(f'CLUTTER Buckboard: the bed\'s middle is at {m.notes["bed_middle"]} from the pivot; the shaft tips at y '
            f'{YF - 2.36 + move.y:.2f}')
    return m.done(f'the middle of the footprint with the shafts (the bed\'s middle {m.notes["bed_middle"][1]:+.2f} m '
                  f'along Y)', 'A light two-seat spring wagon: faded red wheels,\na board bed, its shafts down.', ao=0.5,
                  nanite=False)


# --- SignBracket and the boards ---

SIGN_Y = -0.55                    # the bracket's SOCKET_Sign along its arm
SIGN_RINGS = 0.3                  # a board's rings, either side of its middle
EYE_Z = -0.0535                   # the hook eyes' bottoms, under the arm


def sign_bracket(name, seed):
    m = lc.Model(name, seed)
    m.board((0.0, -0.006, 0.05), (0.0, -0.006, -0.55), 0.05, 0.012, face=(0.0, -1.0, 0.0), uv='H3', bevel=0.002)
    for z in (0.02, -0.25, -0.52):
        m.box((0.022, 0.008, 0.022), at=(0.0, -0.014, z), uv='H3')
    arm = lc.spline([(0.0, -0.01, 0.0), (0.0, -0.5, 0.0), (0.0, -0.92, 0.0), (0.0, -0.96, 0.025), (0.0, -0.955, 0.06),
                     (0.0, -0.925, 0.07), (0.0, -0.91, 0.05)], per=2)
    lc.tube(m, arm, lc.rect(0.034, 0.014), strip='H3', up=(1.0, 0.0, 0.0))
    lc.tube(m, [(0.0, -0.01, -0.5), (0.0, -0.62, -0.02)], lc.rect(0.024, 0.012), strip='H3', up=(1.0, 0.0, 0.0))
    # A volute in the angle of the plate, the arm and the brace: it leaves the arm's underside and winds in, clear of
    # the brace below it.
    c = Vector((0.0, -0.15, -0.12))
    spiral = []
    for k in range(17):
        a = math.radians(90.0 + 28.0 * k)
        r = 0.098 * (1.0 - k / 24.0)
        spiral.append(c + Vector((0.0, math.cos(a) * r, math.sin(a) * r)))
    lc.tube(m, spiral, lc.rect(0.016, 0.01), strip='H3', up=(1.0, 0.0, 0.0))
    for y in (SIGN_Y - SIGN_RINGS, SIGN_Y + SIGN_RINGS):
        eye = [Vector((0.0, y + math.cos(a) * 0.016, -0.034 + math.sin(a) * 0.016))
               for a in (math.radians(90.0 + 45.0 * k) for k in range(9))]
        lc.rod(m, eye, 0.0042, strip='H3', sides=4, caps=(False, False))
    m.socket('Sign', (0.0, SIGN_Y, EYE_Z))
    return m.done('on the wall at the top of the plate (the arm runs out 0.92 m toward -Y)',
                  'A wrought-iron blade-sign bracket, its volute;\nSOCKET_Sign where a board hangs.', ao=0.2,
                  ground=False, collision='None')


def sign_board(name, seed, text, board, letters, u_board, u_text, height=0.15):
    """A board hanging under its two rings (the pivot is the swing point between them): planks of board ('H1'/'H2', two
    of 19 cm, u_board the two planks' stretches along the strip) or one cream panel ('G', u_board its stretch), in a
    dark frame, text lettered on both faces in letters (u_text: its stretch)."""
    m = lc.Model(name, seed)
    W = 0.8
    top, bottom = -0.065, -0.475
    for y in (-SIGN_RINGS, SIGN_RINGS):
        ring = [Vector((math.cos(a) * 0.02, y, -0.02 + math.sin(a) * 0.02)) for a in
                (math.radians(90.0 + 360.0 * k / 8) for k in range(9))]
        lc.rod(m, ring, 0.0042, strip='H3', sides=4, caps=(False, False))
        m.box((0.016, 0.012, 0.03), at=(0.0, y, top - 0.0), uv='H3')
    if board == 'G':
        m.box((0.028, W, top - bottom - 0.05), at=(0.0, 0.0, (top + bottom) * 0.5), uv=Trim('G', u=u_board, v=0.2),
              bevel=0.003)
    else:
        for k in range(2):
            z = top - 0.03 - 0.0955 - 0.192 * k
            m.board((0.0, -W * 0.5, z), (0.0, W * 0.5, z), 0.19, 0.028, face=(1.0, 0.0, 0.0),
                    uv=Trim(board, u=u_board[k]), bevel=0.002)
    for z in (top - 0.015, bottom + 0.015):
        m.board((0.0, -W * 0.5 - 0.035, z), (0.0, W * 0.5 + 0.035, z), 0.035, 0.042, face=(1.0, 0.0, 0.0), uv='C',
                bevel=0.004)
    for y in (-W * 0.5 - 0.0175, W * 0.5 + 0.0175):
        m.board((0.0, y, top - 0.032), (0.0, y, bottom + 0.032), 0.035, 0.042, face=(1.0, 0.0, 0.0), uv='C', bevel=0.004)
    mid = (top + bottom) * 0.5
    for s in (1.0, -1.0):
        mapper = lc.plane_mapper((s * 0.014, 0.0, mid), (0.0, s, 0.0), (0.0, 0.0, 1.0), lift=0.0016)
        lc.lettering(m, text, height, mapper, strip=letters, width=W - 0.12, tol=0.024, u0=u_text,
                     v0=0.4 if letters == 'G' else 0.015)
    return m.done('the swing point between its two rings (hang it at SignBracket\'s SOCKET_Sign)',
                  f'{text}, lettered both sides; it hangs at the\nbracket\'s SOCKET_Sign and swings about its axis.',
                  ao=0.15, ground=False, collision='None', smooth=40.0)


# --- Bunting ---

def bunting(name, seed):
    m = lc.Model(name, seed)
    rng = m.rng
    pins = [-3.0, 0.0, 3.0]
    sag = 0.3
    colors = [('H2', 4.6), ('G', 4.2), ('H1', 3.0)]
    cord = []
    for a, b in zip(pins, pins[1:]):
        for k in range(13):
            t = k / 12
            if cord and k == 0:
                continue
            x = a + (b - a) * t
            cord.append(Vector((x, -0.012, -sag * 4.0 * t * (1.0 - t) * (1.0 + 0.04 * math.sin(x * 3.0)))))
    lc.rod(m, cord, 0.0055, strip='A', sides=4, wobble=0.0008)
    for x in pins:
        m.box((0.012, 0.012, 0.012), at=(x, -0.004, 0.0), uv='H3')

    def cord_at(x):
        for a, b in zip(pins, pins[1:]):
            if a <= x <= b:
                t = (x - a) / (b - a)
                return Vector((x, -0.012, -sag * 4.0 * t * (1.0 - t) * (1.0 + 0.04 * math.sin(x * 3.0))))
        return Vector((x, -0.012, 0.0))
    n = 0
    for a, b in zip(pins, pins[1:]):
        for k in range(10):
            x = a + (b - a) * (k + 0.75) / 10.5 + rng.uniform(-0.015, 0.015)
            n += 1
            if n == 7:
                continue                                    # one gone
            strip, u0 = colors[n % 3]
            p = cord_at(x)
            q = cord_at(x + 0.01)
            t = (q - p).normalized()
            w = 0.24
            length = 0.3 if n != 13 else 0.19                # one torn short
            twist = math.radians(rng.uniform(-18.0, 18.0))
            swing = math.radians(rng.uniform(-10.0, 12.0))
            down = Vector((0.0, math.sin(swing), -math.cos(swing)))
            across = Vector((t.x, t.y * math.cos(twist), t.z))
            front = across.cross(down).normalized()
            pts = [p - across * w * 0.5 + Vector((0.0, -0.004, 0.006)), p + Vector((0.0, -0.004, 0.008)),
                   p + across * w * 0.5 + Vector((0.0, -0.004, 0.006))]
            mid = [p + down * length * 0.5 - across * w * 0.27 + front * 0.008 * math.sin(twist * 3.0),
                   p + down * length * 0.52 + front * rng.uniform(-0.01, 0.01),
                   p + down * length * 0.5 + across * w * 0.27 - front * 0.008 * math.sin(twist * 3.0)]
            tip = p + down * length + across * rng.uniform(-0.02, 0.02) + front * rng.uniform(-0.015, 0.015)
            cos = []
            uvs = []
            for layer, off in ((0, 0.0012), (1, -0.0012)):
                for q_, (u, v) in zip(pts + mid + [tip], [(-w / 2, 0.0), (0.0, 0.0), (w / 2, 0.0), (-w * 0.27, length * 0.5),
                                                         (0.0, length * 0.52), (w * 0.27, length * 0.5), (0.0, length)]):
                    cos.append(q_ + front * off)
                    uvs.append(lc.strip_uv(strip, u0 + u + n * 0.25, 0.01 + v * 0.6))
            faces = []
            for layer in (0, 1):
                o = layer * 7
                f = front if layer == 0 else -front
                for idx in ([0, 1, 4, 3], [1, 2, 5, 4], [3, 4, 6], [4, 5, 6]):
                    ids = [o + i for i in idx]
                    ids = lc._orient(cos, ids, f)
                    faces.append((ids, [uvs[i] for i in ids]))
            m.put(cos, faces)
    return m.done('the middle pin (the end pins at x = -3 and +3 m, all at z = 0)', 'Faded pennants on a cord, a 6 m '
                  'run in two\nswags, for the Gilt Spur\'s balcony.', ao=0.1, ground=False, collision='None')


# --- The family ---

BUILDERS = [
    ('RockingChair', lambda: rocking_chair('RockingChair', 4201)),
    ('RockingChair_Shawl', lambda: rocking_chair('RockingChair_Shawl', 4201, shawl=True)),
    ('PorchChair', lambda: porch_chair('PorchChair', 4203)),
    ('Washtub', lambda: washtub('Washtub', 4204)),
    ('FlowerBox', lambda: flower_box('FlowerBox', 4205)),
    ('PorchLantern', lambda: porch_lantern('PorchLantern', 4206)),
    ('BroomBucket', lambda: broom_bucket('BroomBucket', 4207)),
    ('Horseshoe', lambda: horseshoe('Horseshoe', 4208)),
    ('ChoppingBlock', lambda: chopping_block('ChoppingBlock', 4209)),
    ('HandPump', lambda: hand_pump('HandPump', 4210)),
    ('MilkCans', lambda: milk_cans('MilkCans', 4211)),
    ('KitchenGarden', lambda: kitchen_garden('KitchenGarden', 4212)),
    ('ChickenCoop', lambda: chicken_coop('ChickenCoop', 4213)),
    ('Spittoon', lambda: spittoon('Spittoon', 4214)),
    ('BottleCrate', lambda: bottle_crate('BottleCrate', 4215)),
    ('FlourSacks', lambda: flour_sacks('FlourSacks', 4216)),
    ('Buckboard', lambda: buckboard('Buckboard', 4217)),
    ('SignBracket', lambda: sign_bracket('SignBracket', 4218)),
    ('SignBoard_Pruitts', lambda: sign_board('SignBoard_Pruitts', 4219, "PRUITT'S", 'H2', 'G', (4.75, 6.6), 4.45)),
    ('SignBoard_Barber', lambda: sign_board('SignBoard_Barber', 4220, 'BARBER', 'H1', 'G', (3.1, 4.0), 4.5)),
    ('SignBoard_Hotel', lambda: sign_board('SignBoard_Hotel', 4221, 'HOTEL', 'G', 'H2', 5.9, 5.0, height=0.17)),
    ('Bunting', lambda: bunting('Bunting', 4222)),
]


def build():
    return {name: make() for name, make in BUILDERS if ONLY is None or name in ONLY}


# --- Previews: each piece, the set at one scale, and three vignettes on the town's own buildings ---

EXPORTS = os.path.join(lt.REPO, 'Intermediate')
HOME_H = os.path.join(EXPORTS, 'ArtExport_RR_Homes', 'SM_RR_Home_H.fbx')
SALOON = os.path.join(EXPORTS, 'ArtExport_PlayFix', 'SM_FalseFront_Saloon.fbx')
STORE = os.path.join(EXPORTS, 'ArtExport_PlayFix', 'SM_FalseFront_Store.fbx')
WALK = os.path.join(EXPORTS, 'ArtExport_NoticeBoardTown', 'SM_Boardwalk_4m.fbx')
WALL_Z = {'FlowerBox': 1.2, 'PorchLantern': 2.0, 'Horseshoe': 2.3}
VIEWS = {   # per piece: view (toward the camera), lens, sun (azimuth, elevation), margin
    'RockingChair': ((-0.75, -1.0, 0.45), 45.0, (40.0, 24.0), 1.08),
    'RockingChair_Shawl': ((0.55, 1.0, 0.5), 45.0, (-150.0, 24.0), 1.08),
    'PorchChair': ((-0.7, -1.0, 0.45), 45.0, (40.0, 24.0), 1.1),
    'Washtub': ((-0.45, -1.0, 0.75), 45.0, (35.0, 30.0), 1.08),
    'ChoppingBlock': ((-0.55, -1.0, 0.65), 40.0, (40.0, 26.0), 1.06),
    'HandPump': ((-0.85, -1.0, 0.5), 40.0, (45.0, 24.0), 1.06),
    'MilkCans': ((-0.6, -1.0, 0.55), 45.0, (40.0, 24.0), 1.1),
    'KitchenGarden': ((-0.5, -1.0, 0.62), 35.0, (40.0, 22.0), 1.04),
    'ChickenCoop': ((-0.7, -1.0, 0.55), 35.0, (40.0, 24.0), 1.04),
    'Spittoon': ((-0.5, -1.0, 0.55), 60.0, (40.0, 26.0), 1.2),
    'BottleCrate': ((-0.55, -1.0, 0.7), 45.0, (40.0, 28.0), 1.08),
    'FlourSacks': ((-0.45, -1.0, 0.45), 40.0, (35.0, 24.0), 1.08),
    'Buckboard': ((-1.0, -0.75, 0.45), 35.0, (45.0, 22.0), 1.04),
}


def backdrop(name, width, height, finish='woodteal', x=0.0, z0=0.0):
    """A plain wall for the wall pieces' renders, its face at y = 0 (never exported: its name starts with _)."""
    m = town.Model(name, 1)
    space = kit.wall_space((x - width * 0.5, 0.0), (x + width * 0.5, 0.0), z0)
    if finish == 'trim':
        m.panel(kit.wall_outline(width, height), (), 0.12, uv=Trim('A', world=True), space=space, slices=0)
    else:
        m.panel(kit.wall_outline(width, height), (), 0.12, uv=kit.Tile('WoodPlanks'), mat=finish, space=space, slices=0)
    obj = m.finish(ao=False, preview=False)
    obj.hide_render = True
    return obj


def socket_matrix(obj, name):
    sock = next(c for c in obj.children if c.name.split('.')[0] == 'SOCKET_' + name)
    return obj.matrix_world @ sock.matrix_local


def hang(bracket, board, swing=0.0, holder=None):
    """A copy of board hung on bracket's SOCKET_Sign (on holder, a staged copy of the bracket, if given), swung by swing
    degrees about the socket's forward axis."""
    bpy.context.view_layer.update()
    sock = next(c for c in bracket.children if c.name.split('.')[0] == 'SOCKET_Sign')
    frame = (holder or bracket).matrix_world @ sock.matrix_local
    copy = lc.copy_of(board)
    copy.matrix_world = frame @ Matrix.Rotation(math.radians(swing), 4, 'Y')
    return copy


def tiles(built):
    shots = {}
    wall = backdrop('_WallTeal', 4.0, 3.6)
    for name, obj in built.items():
        if name.startswith('SignBoard') or name in ('SignBracket', 'Bunting'):
            continue
        if name in WALL_Z:
            obj.location = (0.0, 0.0, WALL_Z[name])
            bpy.context.view_layer.update()
            shots[name] = lc.shoot(name, [obj], [wall], view=(-0.5, -1.0, 0.18), lens=55.0, sun=(30.0, 26.0), margin=1.3)
            obj.location = (0.0, 0.0, 0.0)
        elif name == 'BroomBucket':
            shots[name] = lc.shoot(name, [obj], [wall], view=(-0.55, -1.0, 0.35), lens=45.0, sun=(30.0, 26.0), margin=1.12)
        else:
            view, lens, sun, margin = VIEWS.get(name, ((-0.6, -1.0, 0.5), 45.0, (40.0, 24.0), 1.1))
            shots[name] = lc.shoot(name, [obj], view=view, lens=lens, sun=sun, margin=margin)
    # The signs: the bracket on a storefront's siding three metres up, each board hung on it.
    if 'SignBracket' in built:
        bracket = built['SignBracket']
        siding = backdrop('_WallSiding', 3.0, 4.2, finish='trim')
        bracket.location = (0.0, 0.0, 3.0)
        for board_name, tile in (('SignBoard_Pruitts', 'SignBracket'), ('SignBoard_Barber', 'SignBoard_Barber'),
                                 ('SignBoard_Hotel', 'SignBoard_Hotel')):
            if board_name not in built:
                continue
            board = hang(bracket, built[board_name], swing=4.0)
            shots[tile] = lc.shoot(tile, [bracket, board], [siding], view=(-1.0, -0.62, 0.02), lens=50.0, sun=(-60.0, 24.0),
                                   margin=1.12, bias=0.5)
            lc.clear_copies()
        bracket.location = (0.0, 0.0, 0.0)
    if 'Bunting' in built:
        rail = backdrop('_WallBalcony', 7.4, 3.4, finish='woodcream')
        obj = built['Bunting']
        obj.location = (0.0, -0.01, 2.9)
        bpy.context.view_layer.update()
        shots['Bunting'] = lc.shoot('Bunting', [obj], [rail], view=(-0.3, -1.0, 0.1), lens=40.0, sun=(30.0, 24.0),
                                    margin=1.04, size=(1800, 700))
        obj.location = (0.0, 0.0, 0.0)
    return shots


def lineup(built):
    """The ground pieces at one scale, a 1.8 m figure among them."""
    rows = [['Buckboard', 'ChickenCoop', 'KitchenGarden'],
            ['HandPump', 'ChoppingBlock', 'Washtub', 'MilkCans', 'FlourSacks'],
            ['RockingChair_Shawl', 'RockingChair', 'PorchChair', 'BottleCrate', 'Spittoon']]
    figure = homes.figure()
    shown = []
    y = 0.0
    for row in rows:
        x = 0.0
        depth = 0.0
        placed = []
        for name in row:
            if name not in built:
                continue
            c = lc.copy_of(built[name], turn=-90.0 if name == 'Buckboard' else 0.0)
            bpy.context.view_layer.update()
            pts = [c.matrix_world @ v.co for v in c.data.vertices]
            lo = Vector((min(p.x for p in pts), min(p.y for p in pts), 0.0))
            hi = Vector((max(p.x for p in pts), max(p.y for p in pts), 0.0))
            c.location = (x - lo.x, y - hi.y, 0.0)
            x += hi.x - lo.x + 0.45
            depth = max(depth, hi.y - lo.y)
            placed.append(c)
        for c in placed:
            c.location.x -= (x - 0.45) * 0.5
        shown += placed
        y -= depth + 0.6
    figure.location = (shown[-1].location.x + 1.2 if shown else 0.0, y + 0.9, 0.0)
    bpy.context.view_layer.update()
    path = lc.shoot('Lineup', shown + [figure], view=(-0.25, -1.0, 0.75), lens=35.0, size=(2400, 1100), sun=(35.0, 30.0),
                    margin=1.03, bias=0.3)
    lc.clear_copies()
    bpy.data.objects.remove(figure)
    return path


def put(built, name, at, turn=0.0, surface=None, start=2.4):
    """A copy of a piece set down at (x, y) on whatever surface is there (a deck, the ground)."""
    if name not in built:
        return None
    x, y = at[0], at[1]
    z = at[2] if len(at) > 2 else (surface.down(x, y, start=start) if surface else 0.0)
    return lc.copy_of(built[name], (x, y, z), turn)


def on_wall(built, name, x, z, surface, y_from=-3.0, turn=0.0):
    """A copy of a wall piece set on the wall facing -Y at (x, z), found by a ray toward +Y."""
    if name not in built:
        return None
    hit = surface.toward((x, y_from, z), (0.0, 1.0, 0.0))
    y = hit[0].y if hit else 0.0
    return lc.copy_of(built[name], (x, y, z), turn)


def vignette_porch(built, home):
    """RR_Home_H's front porch, lived in: the rocker with its shawl, a chair, the washtub, a box gone to seed under the
    left window, the lantern by the door, the broom, a horseshoe over the door."""
    s = lc.Surface([home])
    deck = s.down(-1.6, -1.0, start=2.4)
    staged = [put(built, 'RockingChair_Shawl', (-1.75, -1.0, deck), -22.0),
              put(built, 'PorchChair', (-2.85, -0.55, deck), 18.0),
              put(built, 'Washtub', (2.15, -0.7, deck), 0.0)]
    # Rays toward the wall start on the porch, behind its railing.
    floor_y = s.toward((1.3, -1.5, deck + 0.3), (0.0, 1.0, 0.0))
    staged.append(lc.copy_of(built['BroomBucket'], (1.3, floor_y[0].y if floor_y else 0.0, deck)) if 'BroomBucket' in built
                  else None)
    staged.append(on_wall(built, 'FlowerBox', -2.12, 1.27, s, y_from=-1.5))
    staged.append(on_wall(built, 'PorchLantern', 0.92, 2.2, s, y_from=-1.5))
    staged.append(on_wall(built, 'Horseshoe', 0.0, 3.06, s, y_from=-1.5))
    staged = [o for o in staged if o is not None]
    target = Vector((-0.55, -0.9, 1.25))
    eye = Vector((-5.2, -6.4, 1.75))
    path = lc.shoot('Vignette_Porch', staged + [home], eye=eye, target=target, lens=32.0, size=(1800, 1100),
                    sun=(42.0, 17.0))
    # From the porch's left end, looking along it to the door.
    close = lc.shoot('Vignette_PorchClose', staged + [home], eye=Vector((-3.25, -1.7, deck + 1.5)),
                     target=Vector((0.2, -0.35, deck + 0.95)), lens=30.0, size=(1600, 1000), sun=(42.0, 17.0))
    lc.clear_copies()
    return path, close


def vignette_yard(built, home):
    """The side yard of RR_Home_H: the pump by the corner, the chopping block, milk cans by the steps, the garden and
    the coop out back."""
    staged = [put(built, 'HandPump', (-4.75, 1.3, 0.0), -90.0),
              put(built, 'ChoppingBlock', (-5.1, 4.7, 0.0), 40.0),
              put(built, 'MilkCans', (-2.75, -2.55, 0.0), -20.0),
              put(built, 'KitchenGarden', (-8.4, 4.6, 0.0), 0.0),
              put(built, 'ChickenCoop', (-8.6, 0.9, 0.0), 0.0)]
    staged = [o for o in staged if o is not None]
    path = lc.shoot('Vignette_Yard', staged + [home], eye=Vector((-12.2, -6.4, 2.9)), target=Vector((-6.0, 2.4, 0.6)),
                    lens=32.0, size=(1800, 1100), sun=(-42.0, 20.0))
    lc.clear_copies()
    return path


def vignette_street(built, saloon, store, walks):
    """Main Street: bunting on the Gilt Spur's balcony, a crate of empties and the spittoon on its walk, Pruitt's blade
    sign, the buckboard drawn up at the store with the flour sacks half unloaded."""
    s = lc.Surface([o for o in [saloon, store] + walks if o is not None])
    staged = []
    if saloon is not None and 'Bunting' in built:
        rail = s.toward((0.0, -14.0, 4.36), (0.0, 1.0, 0.0))
        y = rail[0].y - 0.012 if rail else -9.9
        staged.append(lc.copy_of(built['Bunting'], (0.0, y, 4.36)))
    walk_z = s.down(-1.6, -8.0, start=2.0, default=0.38)
    staged.append(put(built, 'BottleCrate', (-1.55, -7.85, walk_z), 12.0))
    staged.append(put(built, 'Spittoon', (1.25, -8.3, walk_z), 0.0))
    staged.append(put(built, 'RockingChair', (2.6, -7.75, walk_z), -10.0))
    if store is not None and 'SignBracket' in built:
        sx = 10.0 - 4.4
        hit = s.toward((sx, -14.0, 3.3), (0.0, 1.0, 0.0))
        if hit:
            bracket = lc.copy_of(built['SignBracket'], (sx, hit[0].y, 3.3))
            staged.append(bracket)
            if 'SignBoard_Pruitts' in built:
                staged.append(hang(built['SignBracket'], built['SignBoard_Pruitts'], swing=6.0, holder=bracket))
    staged.append(put(built, 'Buckboard', (12.9, -11.75, 0.0), 92.0))
    staged.append(put(built, 'FlourSacks', (10.0, -10.7, 0.0), 20.0))
    staged = [o for o in staged if o is not None]
    sets = [o for o in [saloon, store] + walks if o is not None]
    path = lc.shoot('Vignette_Street', staged + sets, eye=Vector((-7.0, -21.5, 2.2)), target=Vector((4.8, -9.6, 2.4)),
                    lens=28.0, size=(1800, 1100), sun=(-35.0, 18.0))
    lc.clear_copies()
    return path


def previews(built):
    lc.town_materials()
    lc.stage(samples=48)
    shots = tiles(built)
    shots['Lineup'] = lineup(built)
    if '--no-vignettes' not in ARGS and ONLY is None:
        home = lc.import_fbx(HOME_H)
        if home is not None:
            shots['Vignette_Porch'], shots['Vignette_PorchClose'] = vignette_porch(built, home)
            shots['Vignette_Yard'] = vignette_yard(built, home)
            home.hide_render = True
        saloon = lc.import_fbx(SALOON)
        store = lc.import_fbx(STORE)
        if store is not None:
            store.location = (10.0, 0.0, 0.0)
        walks = []
        for x in (-6.0, -2.0, 2.0, 6.0):
            w = lc.import_fbx(WALK)
            if w is not None:
                w.location = (x, -8.5, 0.0)
                walks.append(w)
        bpy.context.view_layer.update()
        shots['Vignette_Street'] = vignette_street(built, saloon, store, walks)
    lt._log('CLUTTER previews: ' + ', '.join(sorted(shots)))
    return shots


if __name__ == '__main__':
    BUILT = build()
    total = sum(info['triangles'] for info in lc.REPORT.values())
    lt._log(f'CLUTTER: {len(lc.REPORT)} models, {total:,} triangles')
    if lt.want_preview():
        lc.write_report()
        previews(BUILT)
