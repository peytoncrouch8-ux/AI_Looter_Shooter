"""The lookout on Ransom's Point (Docs/Areas/RansomsRest.md: Ransom's Point, Main 2, the cold open): the sound version
of the tutorial island's ruined LookoutTower, with its cliff stairs and the gang's mooring post. A scripted model file
(Art/README.md) built with looter_buildings through Tools/Blender/looter_ruins.py.

  Lookout      the tower at (-100, -86): a square fieldstone shaft (4.4 m, a storeroom door on its right side), a
               timber deck at 6.5 m on bearers and joists, braced out 2.2 m past the shaft toward the sunset (the
               front, -Y: face it west over Gravewind Canyon), with railings (saltire-braced toward the sunset). A
               plank watch hut with a tin roof stands at the back of the deck, a lantern by its door (SOCKET_Light).
               The stair climbs the outside in two flights (41 degrees) round the back and left sides to a landing at
               the deck's left edge. The boards where Ellis died, on the front deck right of middle, are still
               stained: a patch of darker boards, the stain soaked into the boards round it.
               SOCKET_Sit: Mister Sexton's seat on the front rail's top, left of middle, facing into the deck.
  CliffStairs  the timber stair down the bluff's 20 m south face to the Mooring Ledge (-102, -101).
  MooringPost  the gang's mooring post on the ledge, its rope cut and frayed.

CliffStairs is one model, not a kit of flights and landings. On a sheer face, one lane of flights can't zigzag: each
landing would hang right over the flight below it, with no headroom. So the flights take turns in two lanes, one
against the rock and one outside it: each turns 180 degrees on a landing that spans both lanes, and the flight below
always has the flight 10 m above it overhead, never 5. That interlock (four flights of 5 m, landings at the turns,
outriggers set into the rock under each lane at its own height, and the railings between the lanes) only works as one
piece, and it gives one draw, continuous walkable collision and no seams. A cliff of another height is a change of
DROP in this script, not new pieces.
  Its pivot is on the bluff top at the cliff's lip, where the first flight leaves the top landing, and the cliff face
  is the plane y = 0 (the rock behind, +Y; the stair hangs out over the drop, -Y). The top landing reaches 0.6 m back
  over the bluff top; the outriggers and rock pins reach 0.5 m into the rock, so the face may bulge or fall back by
  that much. It runs 8.7 m along the face (x -7.2 to 1.5) and 2.9 m out from it, and the bottom landing stands on the
  ledge 20 m down, at x 0 to 1.25.
"""
import math
import random

from mathutils import Matrix, Vector, noise

import looter_buildings as kit
from looter_buildings import Opening, Trim
import looter_props as lp
import looter_ruins as lr
import looter_textures as lt

ONLY = next((a.split('=', 1)[1].split(',') for a in kit._args() if a.startswith('--only=')), None)


# --- The lookout ---

def lookout(name='Lookout', seed=601):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    H, T = 2.2, 0.55                 # half the shaft's outer width; its wall thickness
    FLOOR = 6.5                      # the deck's walking surface
    PLANK, JOIST, BEARER = 0.05, 0.2, 0.24
    WALL_TOP = FLOOR - PLANK - JOIST - BEARER
    X0, X1, Y0, Y1 = -2.3, 3.0, -4.4, 2.6          # the deck
    HX0, HX1, HY0, HY1 = -1.9, 2.6, 0.2, 2.4       # the watch hut on it
    HUT_H = 2.3
    BAND = 3.25                                     # the timber band round the shaft, and the stair's landing

    # --- The shaft: four fieldstone walls on a footing course, slit windows, a storeroom door, a timber band. ---
    stone = Trim('D', world=True, v=0.17)
    kit.plinth(m, -H, H, -H, H, 0.42, out=0.07)
    door = Opening('door', 1.15, 0.0, 1.0, 2.1, paint='H2', hinge='right')
    walls = [((-H, -H), (H, -H), 0.0, [Opening('window', 2.12, 3.6, 0.16, 0.85)]),
             ((H, -H), (H, H), T, [door]),
             ((H, H), (-H, H), 0.0, []),
             ((-H, H), (-H, -H), T, [Opening('window', 1.57, 4.0, 0.16, 0.85)])]
    for p0, p1, inset, items in walls:
        space = kit.wall_space(p0, p1, 0.0)
        length = (Vector(p1) - Vector(p0)).length - 2.0 * inset
        local = kit.Space(space.matrix @ Matrix.Translation((inset, 0.0, 0.0)))
        # (The trim's 0.8 m bands already cut the stone into rows for the baked shading: no slices along it.)
        m.panel(kit.wall_outline(length, WALL_TOP, [o for o in items if o.kind == 'door']), kit.holes(items), T, stone,
                space=local, around=items, slices=0)
        for o in items:
            if o.kind == 'door':
                m.board((o.x - 0.32, T * 0.5, o.h + 0.12), (o.x + o.w + 0.32, T * 0.5, o.h + 0.12), 0.24, T + 0.04,
                        uv=Trim('C', lane='each'), space=local)
                kit.door(m, local, o, T)
            else:
                # A stone sill under each slit.
                m.box((o.w + 0.16, T + 0.06, 0.07), at=(o.x + o.w * 0.5, T * 0.5 - 0.03, o.z - 0.035),
                      uv=Trim('D', v=0.2), space=local)
        # Hulls: the wall, open at the door.
        doors = [o for o in items if o.kind == 'door']
        if doors:
            o = doors[0]
            m.hull((o.x, T, WALL_TOP), at=(o.x * 0.5, T * 0.5, WALL_TOP * 0.5), space=local)
            right = length - (o.x + o.w)
            m.hull((right, T, WALL_TOP), at=(o.x + o.w + right * 0.5, T * 0.5, WALL_TOP * 0.5), space=local)
            m.hull((o.w, T, WALL_TOP - o.h - 0.25), at=(o.x + o.w * 0.5, T * 0.5, (WALL_TOP + o.h + 0.25) * 0.5),
                   space=local)
            m.hull((o.w, 0.1, o.h + 0.25), at=(o.x + o.w * 0.5, T * 0.3, (o.h + 0.25) * 0.5), space=local)  # it's shut
        else:
            m.hull((length, T, WALL_TOP), at=(length * 0.5, T * 0.5, WALL_TOP * 0.5), space=local)
    # The timber band at the landing's height, pinned to the stone: front and back run through, sides between.
    for p0, p1, inset in (((-H, -H), (H, -H), 0.0), ((H, -H), (H, H), 0.0), ((H, H), (-H, H), 0.0),
                          ((-H, H), (-H, -H), 0.0)):
        space = kit.wall_space(p0, p1, 0.0)
        length = (Vector(p1) - Vector(p0)).length
        m.board((-0.14, -0.06, BAND - 0.1), (length + 0.14, -0.06, BAND - 0.1), 0.2, 0.12, uv=Trim('C', lane='each'),
                space=space, cuts=2)
    m.section('shaft')

    # --- The deck: bearers on the front and back walls, joists across them cantilevered toward the sunset on knee
    # braces, a rim, and boards. ---
    beam = Trim('C', lane='each')
    for y in (-H + T * 0.5, H - T * 0.5):
        m.board((X0, y, WALL_TOP + BEARER * 0.5), (X1, y, WALL_TOP + BEARER * 0.5), BEARER, 0.24, uv=beam)
    joists = [X0 + 0.08 + (X1 - X0 - 0.16) * k / 7 for k in range(8)]
    jz = WALL_TOP + BEARER + JOIST * 0.5
    for x in joists:
        m.board((x, Y0 + 0.06, jz), (x, Y1 - 0.06, jz), JOIST, 0.14, face=(1.0, 0.0, 0.0), uv=beam)
    m.board((X0, Y0 + 0.04, jz), (X1, Y0 + 0.04, jz), JOIST + 0.04, 0.08, uv=beam)          # the front rim
    m.board((X0, Y1 - 0.04, jz), (X1, Y1 - 0.04, jz), JOIST, 0.08, face=(0.0, 1.0, 0.0), uv=beam)
    for x in (joists[1], joists[3], joists[5]):
        m.board((x, -H - 0.04, WALL_TOP - 1.7), (x, Y0 + 0.55, jz - JOIST * 0.5 + 0.02), 0.16, 0.13,
                face=(1.0, 0.0, 0.0), uv=beam)
        m.box((0.2, 0.06, 0.2), at=(x, -H - 0.03, WALL_TOP - 1.72), uv=Trim('H3', fit=True))   # its iron shoe
    for y in (-1.4, 1.2):
        m.board((H + 0.04, y, WALL_TOP - 1.0), (joists[-1] - 0.02, y, jz - JOIST * 0.5 + 0.02), 0.14, 0.12,
                face=(0.0, -1.0, 0.0), uv=beam)
    m.section('deck frame')
    # Boards along X on the joists, each row in two lengths broken over a joist; none under the hut. The stained
    # patch: those boards are darker (the trim's hewn-beam brown), and the stain soaks out into the boards round it
    # (darkened occlusion, after the bake).
    STAIN = Vector((0.85, -2.85))

    def stain_reach(y):
        """How far the darker boards reach either side of the stain's middle on a row at y (0: none)."""
        t = abs(y - STAIN.y) / 0.5
        if t >= 1.0:
            return 0.0, 0.0
        half = 0.75 * math.sqrt(1.0 - t * t)
        return (half * (1.0 + 0.25 * noise.noise(Vector((y * 7.0, 1.3, 0.2)))),
                half * (1.0 + 0.25 * noise.noise(Vector((y * 7.0, 4.1, 0.9)))))
    plank_z = FLOOR - PLANK * 0.5
    row_index = 0
    y = Y0 + 0.1
    while y < HY0 + 0.05:
        split = joists[3 + (row_index % 3) - 1]
        pieces = [(X0 - rng.uniform(0.0, 0.03), split, False), (split, X1 + rng.uniform(0.0, 0.03), False)]
        left, right = stain_reach(y)
        if left + right > 0.0:
            a, b = STAIN.x - left, STAIN.x + right
            pieces = [(X0 - rng.uniform(0.0, 0.03), a, False), (a, b, True), (b, X1 + rng.uniform(0.0, 0.03), False)]
        for xa, xb, stained in pieces:
            # Boards in and round the stain get vertices every 25 cm there (the soak is baked into them), others
            # one cut a board.
            near = abs(y - STAIN.y) < 0.75
            cuts = [x for x in kit.frange(STAIN.x - 0.95, STAIN.x + 0.95, 0.25) if xa + 0.05 < x < xb - 0.05] \
                if near else [(xa + xb) * 0.5]
            length = xb - xa - (0.008 if stained else 0.0)
            tb = kit._box((length, PLANK, 0.19), 0.0, 0)
            kit._slice(tb, 0, [x - (xa + xb) * 0.5 for x in cuts])
            matrix = kit.place(((xa + xb) * 0.5, y, plank_z), (-90.0, 0.0, 0.0))
            if stained:
                m.emit(tb, Trim('C', lane=rng.randrange(4)), 'trim', matrix)
            else:
                m.emit(tb, lr.PlankRow(rng.randrange(16)), 'planks', matrix)
        y += 0.2
        row_index += 1
    m.hull((X1 - X0, Y1 - Y0, 0.3), at=((X0 + X1) * 0.5, (Y0 + Y1) * 0.5, FLOOR - 0.15))
    m.section('deck boards')

    # Railings: saltire-braced along the front and right, the left one open where the stair arrives.
    RAIL = 1.05
    front_posts = lr.railing(m, (X0 + 0.06, Y0 + 0.06, FLOOR), (X1 - 0.06, Y0 + 0.06, FLOOR), RAIL,
                             face=(0.0, -1.0, 0.0), post_every=1.33, cross=True, seed=seed)
    lr.railing(m, (X1 - 0.06, Y0 + 0.06, FLOOR), (X1 - 0.06, HY0 - 0.08, FLOOR), RAIL, face=(1.0, 0.0, 0.0),
               post_every=1.55, ends=(False, True), seed=seed + 1)
    GAP0, GAP1 = -2.55, -1.25                       # where the stair's landing meets the deck
    lr.railing(m, (X0 + 0.06, Y0 + 0.06, FLOOR), (X0 + 0.06, GAP0, FLOOR), RAIL, face=(-1.0, 0.0, 0.0),
               post_every=1.55, ends=(False, True), seed=seed + 2)
    lr.railing(m, (X0 + 0.06, GAP1, FLOOR), (X0 + 0.06, HY0 - 0.08, FLOOR), RAIL, face=(-1.0, 0.0, 0.0),
               post_every=1.55, seed=seed + 3)
    for p0, p1 in (((X0 + 0.06, Y0 + 0.06), (X1 - 0.06, Y0 + 0.06)), ((X1 - 0.06, Y0 + 0.06), (X1 - 0.06, HY0)),
                   ((X0 + 0.06, Y0 + 0.06), (X0 + 0.06, GAP0)), ((X0 + 0.06, GAP1), (X0 + 0.06, HY0))):
        lr.hull_along(m, (p0[0], p0[1], FLOOR), (p1[0], p1[1], FLOOR), RAIL + 0.05, thick=0.14)
    # Mister Sexton's seat: on the top rail between the first two posts from the left, facing into the deck (+Y).
    sit_x = (front_posts[0].x + front_posts[1].x) * 0.5 + 0.1
    m.socket('Sit', (sit_x, Y0 + 0.06 - 0.085, FLOOR + RAIL), (0.0, 0.0, 180.0))
    m.section('railings')

    # --- The watch hut: board walls on corner posts, a door onto the deck, windows, a tin roof. ---
    board = Trim('A', world=True, rotate=True)
    door_h = Opening('door', 0.65, 0.0, 0.9, 2.0, paint='H1', casing='A')
    front_win = Opening('window', 2.75, 0.95, 0.8, 0.7, style='glass', paint='H1', panes=(2, 2))
    pitch = 38.0
    rise = (HY1 - HY0) * 0.5 * math.tan(math.radians(pitch))
    hut_walls = [((HX0, HY0), (HX1, HY0), [door_h, front_win], None),
                 ((HX1, HY0), (HX1, HY1), [Opening('window', 0.8, 1.0, 0.65, 0.6, style='glass', panes=(2, 2))],
                  ((HY1 - HY0) * 0.5, HUT_H + rise)),
                 ((HX1, HY1), (HX0, HY1), [], None),
                 ((HX0, HY1), (HX0, HY0), [], ((HY1 - HY0) * 0.5, HUT_H + rise))]
    for p0, p1, items, apex in hut_walls:
        space = kit.wall_space(p0, p1, FLOOR - 0.04)
        length = (Vector(p1) - Vector(p0)).length
        m.panel(kit.wall_outline(length, HUT_H + 0.04, [o for o in items if o.kind == 'door'], apex),
                kit.holes(items), 0.1, board, space=space, around=items)
        kit.openings(m, space, items, 0.1)
    for x, y in ((HX0, HY0), (HX1, HY0), (HX1, HY1), (HX0, HY1)):
        m.board((x, y, FLOOR - 0.04), (x, y, FLOOR + HUT_H + 0.02), 0.15, 0.15, uv=beam, bevel=0.01)
    front, back = kit.gable(m, HX0, HX1, HY0, HY1, FLOOR + HUT_H, pitch, overhang=0.5, rake=0.32, deck=0.07, sag=0.05)
    kit.tin(m, front)
    kit.tin(m, back)
    kit.ridge_cap(m, front, back)
    m.hull((HX1 - HX0 + 0.1, HY1 - HY0 + 0.1, HUT_H), at=((HX0 + HX1) * 0.5, (HY0 + HY1) * 0.5, FLOOR + HUT_H * 0.5))
    eave_lo = FLOOR + HUT_H - 0.5 * math.tan(math.radians(pitch))
    m.hull_points([(x, y, eave_lo) for x in (HX0 - 0.32, HX1 + 0.32) for y in (HY0 - 0.5, HY1 + 0.5)] +
                  [(x, (HY0 + HY1) * 0.5, FLOOR + HUT_H + rise + 0.15) for x in (HX0 - 0.32, HX1 + 0.32)])
    # The lantern on an iron bracket right of the door, lit (SOCKET_Light).
    bx = HX0 + door_h.x + door_h.w + 0.42
    m.board((bx, HY0 - 0.02, FLOOR + 2.02), (bx, HY0 - 0.42, FLOOR + 2.02), 0.03, 0.02, face=(1.0, 0.0, 0.0),
            uv=Trim('H3', fit=True))
    m.board((bx, HY0 - 0.02, FLOOR + 1.75), (bx, HY0 - 0.3, FLOOR + 2.0), 0.025, 0.015, face=(1.0, 0.0, 0.0),
            uv=Trim('H3', fit=True))
    parts, ring_top, glass = lr.iron_lantern((bx, HY0 - 0.4, FLOOR + 1.62), seed + 5, key_iron='trim', size=0.9)
    for part in parts:
        m.add(part)
    m.socket('Light', tuple(glass))
    m.section('hut')

    # Deck furniture: a bench against the hut under its window, a crate and a coil of rope.
    bz = FLOOR + 0.44
    b0, b1 = HX0 + 2.55, HX0 + 3.9
    for k, yy in enumerate((HY0 - 0.42, HY0 - 0.24)):
        m.board((b0, yy, bz), (b1, yy, bz), 0.17, 0.05, face=(0.0, 0.0, 1.0), uv='A')
    for x in (b0 + 0.15, b1 - 0.15):
        m.board((x, HY0 - 0.33, FLOOR), (x, HY0 - 0.33, bz - 0.03), 0.3, 0.06, face=(1.0, 0.0, 0.0), uv='C')
    m.hull((b1 - b0, 0.4, 0.47), at=((b0 + b1) * 0.5, HY0 - 0.33, FLOOR + 0.235))
    cx, cy = X1 - 0.55, HY0 - 0.5
    m.box((0.6, 0.5, 0.48), at=(cx, cy, FLOOR + 0.24), rot=(0.0, 0.0, 12.0), uv='A', bevel=0.01)
    for dz in (0.06, 0.42):
        m.box((0.64, 0.54, 0.05), at=(cx, cy, FLOOR + dz), rot=(0.0, 0.0, 12.0), uv='C')
    m.hull((0.64, 0.54, 0.48), at=(cx, cy, FLOOR + 0.24), rot=(0.0, 0.0, 12.0))
    coil = []
    for k in range(2 * 9 + 1):
        a = 2.0 * math.pi * k / 9
        r = 0.22 - 0.03 * (k // 9)
        coil.append((cx - 0.05 + r * math.cos(a), cy - 0.75 + r * math.sin(a),
                     FLOOR + 0.025 + 0.03 * (k // 9) + 0.008 * k / 9))
    m.add(lr.tube(coil, 0.02, sides=4, lane=2, seed=seed + 7, caps=(False, False)), smooth=True)
    m.section('furniture')

    # --- The stair: up the back wall from the right, a landing on the band at the back left corner, up the left wall
    # to a landing at the deck's left edge. 17 risers of 19 cm a flight, 41 degrees. ---
    W = 1.0
    f1_foot, f1_head = Vector((2.66, H + 0.7, 0.0)), Vector((-1.04, H + 0.7, BAND))
    lr.flight(m, f1_foot, f1_head, (0.0, 1.0, 0.0), W, 17, seed=seed + 10)
    L1 = (-3.4, -1.04, H + 0.2, H + 1.2)          # x0, x1, y0, y1
    f2_foot, f2_head = Vector((-2.9, H + 0.2, BAND)), Vector((-2.9, GAP1 - 0.05, FLOOR))
    lr.flight(m, f2_foot, f2_head, (1.0, 0.0, 0.0), W, 17, seed=seed + 11)
    L2 = (-3.4, X0, GAP0, GAP1 - 0.05)
    for (x0, x1, y0, y1), z in ((L1, BAND), (L2, FLOOR)):
        for yy in (y0 + 0.05, y1 - 0.05):
            m.board((x0, yy, z - PLANK - 0.09), (x1, yy, z - PLANK - 0.09), 0.18, 0.12, uv=beam)
        for xx in kit.frange(x0 + 0.1, x1, 0.2):
            m.board((xx, y0 - 0.02, z - PLANK * 0.5), (xx, y1 + 0.02, z - PLANK * 0.5), 0.19, PLANK,
                    face=(0.0, 0.0, 1.0), uv=lr.PlankRow(rng.randrange(16)), mat='planks')
        m.hull((x1 - x0, y1 - y0, 0.3), at=((x0 + x1) * 0.5, (y0 + y1) * 0.5, z - 0.15))
    # The corner landing stands on posts (and the band, inside); the top landing is braced off the shaft.
    for x, y in ((L1[0] + 0.07, L1[3] - 0.07), (L1[1] - 0.07, L1[3] - 0.07), (L1[0] + 0.07, L1[2] + 0.07)):
        m.board((x, y, -0.1), (x, y, BAND - PLANK - 0.18), 0.15, 0.15, uv=beam, bevel=0.01)
        m.hull((0.16, 0.16, BAND - 0.25), at=(x, y, (BAND - 0.25) * 0.5))
    mid = f1_foot.lerp(f1_head, 0.5)
    m.board((mid.x, H + 1.2 - 0.07, -0.1), (mid.x, H + 1.2 - 0.07, mid.z - 0.45), 0.13, 0.13, uv=beam)
    for y in (L2[2] + 0.12, L2[3] - 0.12):
        m.board((-H - 0.03, y, FLOOR - 1.45), (L2[0] + 0.1, y, FLOOR - PLANK - 0.17), 0.14, 0.12, face=(0.0, -1.0, 0.0),
                uv=beam)
    # Handrails on the open sides, and their hulls; the landings' outer edges.
    f1_rail = (Vector((f1_foot.x, H + 1.27, 0.19)), Vector((f1_head.x, H + 1.27, BAND)))
    f2_rail = (Vector((-3.47, f2_foot.y, BAND + 0.19)), Vector((-3.47, f2_head.y, FLOOR)))
    lr.railing(m, f1_rail[0], f1_rail[1], 0.95, face=(0.0, 1.0, 0.0), post_every=1.35, post=0.09,
               rail=(0.1, 0.05), post_below=0.32, mid=None)
    lr.railing(m, f2_rail[0], f2_rail[1], 0.95, face=(-1.0, 0.0, 0.0), post_every=1.35, post=0.09,
               rail=(0.1, 0.05), post_below=0.32, mid=None)
    lr.railing(m, (L1[1], L1[3] + 0.0, BAND), (L1[0], L1[3], BAND), 1.0, face=(0.0, 1.0, 0.0), post=0.1,
               post_every=1.6)
    lr.railing(m, (L1[0], L1[3], BAND), (L1[0], L1[2] + 0.05, BAND), 1.0, face=(-1.0, 0.0, 0.0), post=0.1,
               ends=(False, True), mid=None)
    lr.railing(m, (L2[0], L2[3], FLOOR), (L2[0], L2[2], FLOOR), 1.0, face=(-1.0, 0.0, 0.0), post=0.1, mid=None)
    lr.railing(m, (L2[0], L2[2], FLOOR), (X0, L2[2], FLOOR), 1.0, face=(0.0, -1.0, 0.0), post=0.1, ends=(False, True),
               mid=None)
    lr.flight_hull(m, f1_foot, f1_head, (0.0, 1.0, 0.0), W)
    lr.flight_hull(m, f2_foot, f2_head, (1.0, 0.0, 0.0), W)
    for p0, p1 in (f1_rail, f2_rail):
        lr.hull_along(m, p0, p1, 1.0, thick=0.12)
    for p0, p1, z in (((L1[1], L1[3]), (L1[0], L1[3]), BAND), ((L1[0], L1[3]), (L1[0], L1[2]), BAND),
                      ((L2[0], L2[3]), (L2[0], L2[2]), FLOOR), ((L2[0], L2[2]), (X0, L2[2]), FLOOR)):
        lr.hull_along(m, (p0[0], p0[1], z), (p1[0], p1[1], z), 1.05, thick=0.12)
    m.section('stair')

    obj = m.finish(preview=False, fallback=100.0, ao_distance=1.0)

    # The stain soaking out of the dark boards into the ones round it.
    def blotch(p, n):
        if n.z < 0.5 or abs(p.z - FLOOR) > 0.03:
            return 0.0
        d = Vector((p.x - STAIN.x, (p.y - STAIN.y) * 1.45))
        edge = 0.95 + 0.15 * noise.noise(Vector((p.x * 3.0, p.y * 3.0, 0.5)))
        return max(0.0, min(1.0, (edge - d.length) / 0.35))
    lr.darken(obj, blotch, strength=0.7)
    if lt.want_preview():
        lt.preview([obj], lt.preview_path('Buildings', name), view=(-1.15, -1.5, 0.55), fit=0.92)
    return obj


# --- The cliff stairs ---

def cliff_stairs(name='CliffStairs', seed=611):
    m = lr.Model(name, seed=seed)
    rng = m.rng
    DROP, FLIGHTS = 20.0, 4
    RISE = DROP / FLIGHTS            # 5 m a flight, 26 risers of 19 cm
    RISERS = 26
    GOING = 0.235
    RUN = (RISERS - 1) * GOING       # 5.875 m: 40 degrees
    W = 1.1                          # each lane's width
    IN = (-0.15 - W, -0.15)          # the lane against the rock (y range; the face is y = 0)
    OUT = (IN[0] - 0.35 - W, IN[0] - 0.35)
    IN_Y, OUT_Y = sum(IN) * 0.5, sum(OUT) * 0.5
    LAND = 1.25                      # each turning landing's length along the face
    XL, XR = -RUN, 0.0               # where flights meet the landings: the left turns and the right ones
    PLANK = 0.05
    beam = Trim('C', lane='each')
    iron = Trim('H3', fit=True)
    ROCK = 0.5                       # how far the outriggers reach into the rock

    def outrigger(x, z_top, y_out, brace=True):
        """A beam set into the rock along Y under a walking surface whose underside is at z_top, out to y_out; a
        knee brace under it back to the face, and an iron plate where it enters the rock."""
        zb = z_top - 0.1
        m.board((x, ROCK, zb), (x, y_out, zb), 0.2, 0.16, face=(1.0, 0.0, 0.0), uv=beam)
        m.box((0.32, 0.025, 0.3), at=(x, -0.012, zb), uv=iron)
        if brace:
            m.board((x, 0.02, zb - 1.45), (x, y_out + 0.25, zb - 0.12), 0.14, 0.12, face=(1.0, 0.0, 0.0), uv=beam)
            m.box((0.22, 0.025, 0.22), at=(x, -0.012, zb - 1.47), uv=iron)

    flights = []
    for k in range(FLIGHTS):
        top, bottom = -RISE * k, -RISE * (k + 1)
        if k % 2 == 0:       # against the rock, down toward -X
            foot, head, lane = Vector((XL, IN_Y, bottom)), Vector((XR, IN_Y, top)), IN
        else:                # outside, down toward +X
            foot, head, lane = Vector((XR, OUT_Y, bottom)), Vector((XL, OUT_Y, top)), OUT
        lr.flight(m, foot, head, (0.0, 1.0, 0.0), W, RISERS, seed=seed + k)
        lr.flight_hull(m, foot, head, (0.0, 1.0, 0.0), W)
        flights.append((foot, head, lane))
        # Two outriggers under its stringers, a third of the way in from each end.
        for t in (0.3, 0.72):
            p = foot.lerp(head, t)
            outrigger(p.x, p.z - 0.32, lane[0] - 0.12)
    m.section('flights')

    # Landings: the turns span both lanes; the top one reaches back over the bluff's edge; the bottom one stands on the
    # ledge.
    landings = [((XR, XR + LAND + 0.15), (IN[0], 0.6), 0.0, 'top')]
    for k in range(1, FLIGHTS):
        x = (XL - LAND, XL) if k % 2 else (XR, XR + LAND)
        landings.append((x, (OUT[0], IN[1]), -RISE * k, 'turn'))
    landings.append(((XR, XR + LAND), (OUT[0], IN[1]), -DROP, 'bottom'))
    for (x0, x1), (y0, y1), z, kind in landings:
        for xx in kit.frange(x0 + 0.1, x1, 0.2):
            m.board((xx, y0 - 0.02, z - PLANK * 0.5), (xx, y1 + 0.02, z - PLANK * 0.5), 0.19, PLANK,
                    face=(0.0, 0.0, 1.0), uv=lr.PlankRow(rng.randrange(16)), mat='planks')
        for xx in (x0 + 0.15, x1 - 0.15):
            if kind == 'bottom':
                m.board((xx, y1, z - PLANK - 0.08), (xx, y0 - 0.05, z - PLANK - 0.08), 0.16, 0.14,
                        face=(1.0, 0.0, 0.0), uv=beam)
            else:
                outrigger(xx, z - PLANK, y0 - 0.12, brace=kind != 'top')
        m.hull((x1 - x0, y1 - y0, 0.3), at=((x0 + x1) * 0.5, (y0 + y1) * 0.5, z - 0.15))
    m.section('landings')

    # Railings: both sides of the outer flights, the open side of the inner ones, the landings' open edges.
    post, rail = 0.09, (0.1, 0.05)
    for k, (foot, head, lane) in enumerate(flights):
        rise = (head.z - foot.z) / RISERS
        sides = [(lane[0] - 0.12, (0.0, -1.0, 0.0))] + ([(lane[1] + 0.12, (0.0, 1.0, 0.0))] if k % 2 else [])
        for y, face in sides:
            p0 = Vector((foot.x, y, foot.z + rise))
            p1 = Vector((head.x, y, head.z))
            lr.railing(m, p0, p1, 0.95, face=face, post_every=2.0, post=post, rail=rail, post_below=0.3, mid=None)
            lr.hull_along(m, p0, p1, 1.0, thick=0.12)
    for (x0, x1), (y0, y1), z, kind in landings:
        if kind == 'bottom':
            continue
        far = x0 if (x0 + x1) * 0.5 < XL * 0.5 else x1
        edges = [((x0, y0 - 0.06), (x1, y0 - 0.06), (0.0, -1.0, 0.0)),
                 ((far, y0), (far, min(y1, 0.0)), (-1.0 if far == x0 else 1.0, 0.0, 0.0))]
        for (ax, ay), (bx, by), face in edges:
            lr.railing(m, (ax, ay, z), (bx, by, z), 1.0, face=face, post_every=1.4, post=0.1, mid=None)
            lr.hull_along(m, (ax, ay, z), (bx, by, z), 1.05, thick=0.12)
    m.section('railings')
    return m.finish(view=(-0.75, -1.6, 0.3), fit=0.95, fallback=100.0, ao_distance=1.0, ground=False)


# --- The mooring post ---

def mooring_post(name='MooringPost', seed=621):
    """A stout hewn post driven into a cleft of the ledge and wedged with stones, iron-banded, a ring bolt in the rock
    beside it, and the gang's rope still hitched round it: cut, its frayed end trailing off toward the drop."""
    m = lr.Model(name, seed=seed)
    rng = m.rng
    HTOP = 1.25
    m.board((0.0, 0.0, -0.35), (0.0, 0.0, HTOP), 0.3, 0.3, uv=Trim('C', lane='each'), bevel=0.035, cuts=3)
    for z in (0.28, 1.02):
        m.box((0.33, 0.33, 0.07), at=(0.0, 0.0, z), uv=Trim('H3', fit=True))
    for k in range(6):
        a = 2.0 * math.pi * (k + rng.uniform(-0.2, 0.2)) / 6
        r = rng.uniform(0.27, 0.36)
        s = rng.uniform(0.2, 0.32)
        m.add(lr.stone_block((s, s * 0.7, s * 0.55), (r * math.cos(a), r * math.sin(a), s * 0.12),
                             (rng.uniform(-25, 25), rng.uniform(-25, 25), math.degrees(a)), seed=seed + k,
                             rough=0.012, bevel=0.02, key='granite', set_name='RockGranite'))
    # The ring bolt in a flat stone beside it.
    m.add(lr.stone_block((0.5, 0.42, 0.1), (0.55, 0.35, 0.0), (0.0, 0.0, 20.0), seed=seed + 9, rough=0.008, bevel=0.02,
                         key='granite', set_name='RockGranite'))
    m.box((0.08, 0.05, 0.05), at=(0.55, 0.35, 0.07), uv=Trim('H3', fit=True))
    m.add(lr.tube([(0.55 + 0.07 * math.cos(a), 0.35, 0.16 + 0.07 * math.sin(a))
                   for a in [2.0 * math.pi * k / 10 - math.pi * 0.5 for k in range(11)]], 0.012, sides=5, strip='Iron',
                  caps=(False, False)), smooth=True)
    # The rope: two turns round the post, the standing part running off along the rock toward the drop (-Y), cut short
    # and frayed, and the hitch's tail hanging frayed.
    r = 0.2
    turns = []
    for k in range(2 * 10 + 1):
        a = 2.0 * math.pi * k / 10 + 0.4
        turns.append((r * math.cos(a), r * math.sin(a), 0.62 + 0.06 * k / 10))
    lead = [turns[-1], (0.3, -0.35, 0.5), (0.25, -0.6, 0.1), (0.1, -1.0, 0.04), (-0.05, -1.45, 0.05),
            (0.02, -1.8, 0.04)]
    m.add(lr.tube(turns + lead[1:], 0.024, sides=6, lane=1, seed=seed + 11, wobble=0.002), smooth=True)
    for part in lr.frayed(lead[-1], Vector(lead[-1]) - Vector(lead[-2]), 0.024, seed + 12, length=0.12):
        m.add(part, smooth=True)
    tail = [turns[0], (r * math.cos(0.1), r * math.sin(0.1) - 0.05, 0.5), (0.22, -0.08, 0.32)]
    m.add(lr.tube(tail, 0.022, sides=6, lane=1, seed=seed + 13, caps=(True, False)), smooth=True)
    for part in lr.frayed(tail[-1], Vector(tail[-1]) - Vector(tail[-2]), 0.022, seed + 14, length=0.09):
        m.add(part, smooth=True)
    m.hull((0.34, 0.34, HTOP), at=(0.0, 0.0, HTOP * 0.5))
    return m.finish(view=(-1.0, -1.4, 0.55), fit=1.0, fallback=None, ao_distance=0.4, Nanite=0, LODs='50,25')


BUILDERS = [('Lookout', lookout), ('CliffStairs', cliff_stairs), ('MooringPost', mooring_post)]
models = [build() for name, build in BUILDERS if ONLY is None or name in ONLY]
