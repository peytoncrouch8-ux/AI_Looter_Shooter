"""Ransom's Rest's new town buildings (Docs/Work/2026-11-04-Towns/PLAN.md 4.2 item 4; the towns round, 2026-11-04): the
trades a frontier town needs beside Main Street's fronts, for the new lots off the yard road and the south back (the
DRESSING lane and Main place them). Each is built with Tools/Blender/looter_homes.py's kits on the town's own
materials (the trim sheet for nearly everything, so the level's TrimRecolor tints roofs and siding per building),
Nanite on with every triangle in its fallback (what Medium draws), convex UCX hulls, baked AO, Rye lettering with
invented wording only.

  RR_Smithy_Town    the smith and wheelwright, 9 x 7 m, 5.3 m to the ridge: open-fronted under an R3 board roof, grey
                    board-and-batten on a hewn frame, an earth floor; a fieldstone forge in the back left corner with
                    its hood and chimney (SOCKET_Fire on the hearth: the user's D3 fire; SOCKET_Smoke on the chimney),
                    bellows, an anvil on its stump, a quench tub, wheels racked on the right wall, a bench and vise,
                    SMITH & WHEELWRIGHT on a board over the front.
  RR_FeedStore      the feed store, 7 x 10 m, 6.5 m: a frame warehouse on a fieldstone foundation with its floor at a 1 m
                    loading dock (2.5 m deep, steps at its right end) across the gable front, a plank sliding door on
                    an iron track a little open on the dark, a loft door under a hoist beam with its block and rope,
                    FEED & GRAIN painted across the gable, an R4 tar-paper roof. Faces the yard road.
  RR_BoardingHouse  the boarding house (lived in, the user's D5), 11 x 8 m, 8.6 m, two storeys: whitewashed clapboards
                    with teal trim, eight sashes on the front (six lamplit: SOCKET_Window_1..6) and dark ones round the
                    sides, a P2 shed porch across the front with its lantern (SOCKET_Light_Porch), an outside stair up
                    the right gable to the upper door, a ROOMS blade sign, a C1 fieldstone chimney and a C2 stovepipe.
  RR_Bathhouse      the bathhouse and laundry, 6 x 5 m, 4.3 m: weathered clapboards, an R4 roof, BATHS . LAUNDRY over
                    the door, two lamplit front windows (an open shop), a wash bench with tubs; against its right gable
                    the boiler on a fieldstone firebox under its own little roof, its fire door (SOCKET_Fire: the user's
                    D3 fire) and a tall stovepipe (SOCKET_Smoke: steam and smoke).

The pivot is the front door's middle on the front wall's outer face at ground level (the smithy's: the middle of its open
front; the feed store's: its sliding door's, the dock standing in front), front -Y (+X in Unreal). Each model's report
line (HOMES: ...) gives its bounds and walls from the pivot.

  --preview   renders each into Saved/ArtPreviews/Buildings2/ and their lineup (Lineup_TownBuildings.png)
  --only SMITHY,FEED,BOARDING,BATHS   builds just those; --no-ao skips the AO bake; --stats prints where the
              triangles go
"""
import math
import os

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
LAP = homes.LAP
PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'Buildings2')
wall_spaces, rect, gable_outline, notch, clad = homes.wall_spaces, homes.rect, homes.gable_outline, homes.notch, homes.clad
IRON = Tile('MetalWorn')


def sign_board(m, space, cx, z, body, width, height, y=0.0, board='C', letters='band_G', letter_h=None):
    """A sign board on a wall (its back on the face at y in space), its middle at cx, z: a board in board's finish with
    Rye lettering in letters a hair proud of it, and a narrow frame along its top and bottom."""
    depth = 0.04
    town.box(m, (width, depth, height), at=(cx, y - depth * 0.5, z), look=board, space=space, drop=('+y',))
    for dz in (-height * 0.5 - 0.02, height * 0.5 + 0.02):
        town.box(m, (width + 0.06, depth + 0.02, 0.05), at=(cx, y - depth * 0.5 - 0.01, z + dz), look='C', space=space,
                 drop=('+y',))
    town.text(m, body, (cx, y - depth, z), height=letter_h or height * 0.55, width=width - 0.25, space=space,
              look=letters)


def panel_wall(m, space, outline, items, thick=0.06, look='A', every=0.42):
    """A thin board-and-batten wall seen from both sides (an open-fronted shed's): a two-faced panel of upright boards,
    battens over the joints outside, its openings."""
    rng = m.rng
    m.panel(notch(outline, items), kit.holes(items), thick, Trim(look, world=True, rotate=True,
                                                                 u=rng.uniform(0.0, kit.U_REPEAT)), space=space,
            around=items)
    xs = [x for x, z in outline]
    homes.battens(m, space, outline, min(xs), max(xs), homes.keep_out(items), look=look, every=every)
    homes.openings(m, space, items, thick)


def wheel(m, center, normal, radius=0.62, spokes=12, lean=0.0):
    """A wagon wheel: a felloe of hewn segments, an iron tyre round it, spokes to a turned hub (strip C, IronBlack),
    standing in the plane facing normal through center (a wheel racked against a wall)."""
    n = Vector(normal).normalized()
    up = Vector((0.0, 0.0, 1.0))
    side = up.cross(n).normalized()
    up = n.cross(side).normalized()
    c = Vector(center)
    segs = 12
    for k in range(segs):
        a0, a1 = 2.0 * math.pi * k / segs, 2.0 * math.pi * (k + 1) / segs
        p0 = c + (side * math.cos(a0) + up * math.sin(a0)) * (radius - 0.05)
        p1 = c + (side * math.cos(a1) + up * math.sin(a1)) * (radius - 0.05)
        m.board(p0, p1, 0.08, 0.07, face=n, uv=Trim('C', lane='each'), cuts=0)
        q0 = c + (side * math.cos(a0) + up * math.sin(a0)) * radius
        q1 = c + (side * math.cos(a1) + up * math.sin(a1)) * radius
        m.board(q0, q1, 0.06, 0.012, face=(q0 + q1) * 0.5 - c, uv=IRON, mat='iron', cuts=0)
    for k in range(spokes):
        a = 2.0 * math.pi * (k + 0.5) / spokes
        d = side * math.cos(a) + up * math.sin(a)
        m.board(c + d * 0.1, c + d * (radius - 0.09), 0.045, 0.035, face=n, uv='C', cuts=0)
    m.cylinder(c - n * 0.12, c + n * 0.12, 0.1, 0.085, sides=8, uv=Trim('C'), face=up)


def anvil(m, x, y, z, turn=0.0):
    """A blacksmith's anvil (IronBlack): its face, waist and foot as boxes, the horn a tapered round, turned turn
    degrees about Z, standing at (x, y, z)."""
    frame = Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(turn), 4, 'Z')
    space = Space(frame)
    m.box((0.34, 0.24, 0.1), at=(0.0, 0.0, 0.05), uv=IRON, mat='iron', space=space, bevel=0.01)
    m.box((0.18, 0.12, 0.14), at=(0.0, 0.0, 0.17), uv=IRON, mat='iron', space=space)
    m.box((0.5, 0.15, 0.1), at=(0.0, 0.0, 0.29), uv=IRON, mat='iron', space=space, bevel=0.008)
    m.cylinder((0.25, 0.0, 0.3), (0.5, 0.0, 0.32), 0.06, 0.01, sides=8, uv=IRON, mat='iron', space=space,
               face=(0.0, 0.0, 1.0))


def tub(m, at, radius=0.34, height=0.5, water=True):
    """A half barrel tub: staves (WoodPlanks) round a slight belly, two iron hoops, dark water a little under the rim."""
    x, y, z = at
    homes.lathe(m, [(0.0, 0.0), (radius * 0.9, 0.0), (radius, height * 0.6), (radius * 0.98, height),
                    (radius * 0.9, height), (radius * 0.9, height - 0.06), (0.0, height - 0.06)], (x, y, z), 12,
                homes.Boards(rotate=True), 'planks')
    for hz in (height * 0.25, height * 0.8):
        r = radius * (0.93 + 0.08 * math.sin(math.pi * hz / height * 0.8)) + 0.008
        town.cylinder(m, (x, y, z + hz - 0.022), (x, y, z + hz + 0.022), r, sides=12, look='iron', caps=(False, False),
                      face=(0.0, 0.0, 1.0))
    if water:
        m.emit(kit._disc(radius * 0.88, 12, 0.0), Trim('H4', fit=True), 'trim', place((x, y, z + height - 0.12),
                                                                                         (-90.0, 0.0, 0.0)))
    m.hull((radius * 2.0, radius * 2.0, height), at=(x, y, z + height * 0.5))


def workbench(m, x0, x1, y_back, z=0.0, deep=0.7, top=0.86, vise=True):
    """A plank bench against a wall (back at y_back, facing -y): a top of three planks, four legs and a stretcher, a
    shelf under it, an iron vise at its left end."""
    rng = m.rng
    y0 = y_back - deep
    for k in range(3):
        yy = y0 + deep * (k + 0.5) / 3.0
        town.board(m, (x0, yy, z + top - 0.025), (x1, yy + rng.uniform(-0.005, 0.005), z + top - 0.025), deep / 3.0 - 0.01,
                   0.05, look='planks', face=(0.0, 0.0, 1.0))
    for x in (x0 + 0.08, x1 - 0.08):
        for yy in (y0 + 0.08, y_back - 0.08):
            m.box((0.08, 0.08, top - 0.05), at=(x, yy, z + (top - 0.05) * 0.5), uv=Trim('C', lane='each'))
    town.board(m, (x0 + 0.08, y0 + 0.08, z + 0.25), (x1 - 0.08, y0 + 0.08, z + 0.25), 0.1, 0.04, look='C')
    town.box(m, (x1 - x0 - 0.2, deep - 0.12, 0.03), at=((x0 + x1) * 0.5, y0 + deep * 0.5, z + 0.22), look='A')
    if vise:
        m.box((0.16, 0.12, 0.16), at=(x0 + 0.2, y0 - 0.02, z + top + 0.06), uv=IRON, mat='iron', bevel=0.01)
        m.cylinder((x0 + 0.2, y0 - 0.14, z + top + 0.08), (x0 + 0.2, y0 - 0.02, z + top + 0.08), 0.015, sides=6, uv=IRON,
                   mat='iron')
    m.hull((x1 - x0, deep, top), at=((x0 + x1) * 0.5, y0 + deep * 0.5, z + top * 0.5))


# --- RR_Smithy_Town ---

def smithy():
    m = homes.Home('RR_Smithy_Town', seed=701)
    rng = m.rng
    X0, X1, Y0, Y1 = -4.5, 4.5, -3.5, 3.5
    WALL, PITCH = 3.3, 30.0
    EAVE = WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (Y1 - Y0) * 0.5 * tan
    RIDGE_Z = EAVE + RISE
    # The earth floor: a slab of packed dirt sunk a little into the ground, so a lot's grass stops at the walls.
    m.box((X1 - X0 + 0.3, Y1 - Y0 + 0.5, 0.26), at=(0.0, -0.1, -0.11), uv=Tile('GroundDirt'), mat='dirt', cuts=4,
          drop=('-z',))
    m.section('floor')

    # The hewn frame: posts round it (the front's open between them), plates along the top, two tie beams across.
    PS = 0.2
    front_posts = [X0 + 0.1, -1.6, 1.6, X1 - 0.1]
    for x in front_posts:
        kit.stone_stack(m, (x, Y0 + 0.1, -0.12), (0.34, 0.34), 0.2, taper=0.1, rot=rng.uniform(-6.0, 6.0))
        m.board((x, Y0 + 0.1, 0.06), (x, Y0 + 0.1, EAVE - 0.2), PS, PS, uv=Trim('C', lane='each'))
        m.hull((PS, PS, EAVE), at=(x, Y0 + 0.1, EAVE * 0.5))
    for x in (X0 + 0.1, -1.6, 1.6, X1 - 0.1):
        m.board((x, Y1 - 0.1, 0.0), (x, Y1 - 0.1, EAVE - 0.2), PS, PS, uv=Trim('C', lane='each'))
    for y in (0.0,):
        for x in (X0 + 0.1, X1 - 0.1):
            m.board((x, y, 0.0), (x, y, EAVE + RISE * 0.9 - 0.2), PS, PS, uv=Trim('C', lane='each'))
    for y in (Y0 + 0.1, Y1 - 0.1):
        m.board((X0 - 0.05, y, EAVE - 0.1), (X1 + 0.05, y, EAVE - 0.1), 0.2, 0.2, uv=Trim('C', lane='each'))
    for x in (-1.6, 1.6):
        m.board((x, Y0 - 0.05, EAVE - 0.08), (x, Y1 + 0.05, EAVE - 0.08), 0.2, 0.16, uv=Trim('C', lane='each'))
        # A king post from the tie beam to the ridge.
        m.board((x, 0.0, EAVE), (x, 0.0, RIDGE_Z - 0.15), 0.14, 0.14, uv=Trim('C', lane='each'))
    m.board((X0 - 0.2, 0.0, RIDGE_Z - 0.12), (X1 + 0.2, 0.0, RIDGE_Z - 0.12), 0.18, 0.14, uv=Trim('C', lane='each'))
    m.section('frame')

    # The walls: upright boards and battens round the sides and back, seen inside and out; a shuttered window on
    # each side, a plank door at the back right.
    win = dict(paint='C', sash='A', shutter='A', casing_w=0.08)
    spaces = wall_spaces(X0, X1, Y0, Y1, 0.0)
    walls = {
        1: [Opening('window', 2.6, 1.1, 0.9, 0.9, style='open', **win)],
        2: [Opening('door', 1.0, 0.0, 1.0, 2.1, style='plank', finish='A', casing='C', hinge='right')],
        3: [Opening('window', 3.4, 1.1, 0.9, 0.9, style='shut', **win)],
    }
    for k, items in walls.items():
        space, length = spaces[k]
        outline = gable_outline(length, WALL, RISE) if k % 2 else rect(length, WALL)
        panel_wall(m, space, outline, items)
        # A sill under the boards, a little into the ground: on a lot's slope the wall never stands on air (and the
        # outline's foot stays level with the door's, so its fill has no slivers: zero tangents on 2026-11-05).
        for a, b in homes.door_spans(items, length, margin=0.0):
            m.board((a, 0.02, -0.02), (b, 0.02, -0.02), 0.2, 0.1, uv=Trim('C', lane='each'), space=space)
        m.hull((length, 0.1, WALL), at=(length * 0.5, 0.05, WALL * 0.5), space=space)
    m.section('walls')

    # The roof: weathered boards down the slope, battened, the forge's chimney through the back slope.
    FX = X0 + 1.45
    CH = (FX, Y1 - 0.42)
    front = homes.plane(m, X0, X1, Y0, EAVE, PITCH, (Y1 - Y0) * 0.5, 1.0, overhang=0.45, rake=(0.35, 0.35), deck=0.06,
                        sag=0.06)
    back = homes.plane(m, X0, X1, Y1, EAVE, PITCH, (Y1 - Y0) * 0.5, -1.0, overhang=0.35, rake=(0.35, 0.35), deck=0.06,
                       sag=0.06)
    homes.cover(m, front, 'R3', every=0.3)
    homes.cover(m, back, 'R3', every=0.3, gone=(11,), skip=homes.skip_round(back, [(CH[0], CH[1], 0.42, 0.36)]))
    homes.ridge(m, front, back, 'R3')
    # The sign hangs on the front plate's face, under the eave.
    sign_board(m, kit.wall_space((X0, Y0 - 0.0), (X1, Y0 - 0.0), 0.0), 4.5, EAVE - 0.42, 'SMITH & WHEELWRIGHT', 4.6,
               0.42)
    m.section('roof, sign')

    # The forge: a fieldstone hearth in the back left corner, the chimney rising behind it through the roof, a stone
    # hood over the hearth on it; the coals' bed in the hearth (SOCKET_Fire) and the bellows beside it.
    HZ = 0.78
    kit.stone_stack(m, (FX, Y1 - 1.0, -0.1), (1.7, 1.3), HZ + 0.1, taper=0.02, bevel=0.03)
    m.box((0.9, 0.6, 0.06), at=(FX, Y1 - 1.05, HZ + 0.01), uv=IRON, mat='iron')
    for sx in (-1.0, 1.0):
        m.box((0.18, 0.7, 0.12), at=(FX + sx * 0.55, Y1 - 1.05, HZ + 0.06), uv='D')
    kit.stone_stack(m, (CH[0], CH[1], -0.1), (1.1, 0.62), RIDGE_Z - 0.7, taper=0.04, bevel=0.03)
    hood_z = 1.75
    m.emit(kit._box((1.5, 1.0, 0.7)), Trim('D', world=True), 'trim', place((FX, Y1 - 0.95, hood_z + 0.35)),
           shape=lambda co: Vector((co.x * (1.0 - 0.5 * (co.z / 0.7 + 0.5)),
                                    co.y * (1.0 - 0.35 * (co.z / 0.7 + 0.5)) + 0.12 * (co.z / 0.7 + 0.5), co.z)))
    homes.stack(m, CH[0], CH[1] + 0.02, RIDGE_Z - 0.9, RIDGE_Z + 0.75, size=(0.68, 0.56))
    m.socket('Fire', (FX, Y1 - 1.05, HZ + 0.05))
    # The bellows: a wedge of boards on a frame beside the hearth, its nozzle into the forge, its lever.
    bx = FX + 1.25
    m.emit(kit._prism([[(0.0, 0.1), (1.1, 0.0), (1.1, 0.42), (0.0, 0.22)]], 0.55), Trim('C', world=True), 'trim',
           place((bx - 0.55, Y1 - 1.25, 0.8)))
    for x in (bx - 0.2, bx + 0.2):
        m.box((0.07, 0.07, 0.78), at=(x, Y1 - 0.95, 0.39), uv=Trim('C', lane='each'))
    m.cylinder((bx, Y1 - 1.0, 0.98), (FX + 0.6, Y1 - 1.0, 0.86), 0.035, sides=6, uv=IRON, mat='iron')
    m.board((bx, Y1 - 1.25, 1.2), (bx + 0.1, Y1 - 2.0, 1.55), 0.05, 0.05, uv='C')
    m.hull((1.75, 1.35, HZ + 0.1), at=(FX, Y1 - 1.0, (HZ + 0.1) * 0.5 - 0.05))
    m.hull((1.12, 0.64, RIDGE_Z + 0.8), at=(CH[0], CH[1], (RIDGE_Z + 0.8) * 0.5 - 0.1))
    m.hull((1.5, 1.0, 0.7), at=(FX, Y1 - 0.95, hood_z + 0.35))
    m.hull((0.6, 1.2, 1.2), at=(bx, Y1 - 0.85, 0.6))
    m.section('forge')

    # The anvil on its stump, the quench tub by it, wheels racked against the right wall, the bench at the back.
    sx, sy = FX + 1.3, Y1 - 2.6
    m.cylinder((sx, sy, -0.05), (sx, sy, 0.55), 0.27, sides=10, uv=Trim('B', v=0.0), face=(0.0, -1.0, 0.0),
               caps=(False, True), cap_uv=Trim('C', lane=0))
    anvil(m, sx, sy, 0.55, turn=18.0)
    m.hull((0.6, 0.6, 0.88), at=(sx, sy, 0.44))
    tub(m, (sx + 0.85, sy + 0.35, 0.0))
    for k, (wx, wy) in enumerate(((X1 - 0.32, -1.6), (X1 - 0.42, -0.6), (X1 - 0.3, 0.9))):
        wheel(m, (wx, wy, 0.64), (-1.0, rng.uniform(-0.08, 0.08), 0.0), radius=0.62)
    m.hull((0.4, 3.2, 1.3), at=(X1 - 0.35, -0.3, 0.65))
    workbench(m, -0.9, 1.3, Y1 - 0.12)
    m.section('anvil, tub, wheels, bench')

    homes.gable_hull(m, X0, X1, Y0, Y1, EAVE, EAVE, 0.0, RIDGE_Z, overhang=0.4, rake=0.35, pitch_front=PITCH)
    return m.finish(pivot=(0.0, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_FeedStore ---

def feed_store():
    m = homes.Home('RR_FeedStore', seed=702)
    rng = m.rng
    X0, X1, Y0, Y1 = -3.5, 3.5, -5.0, 5.0
    BASE, WALL, PITCH = 1.0, 3.2, 33.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (X1 - X0) * 0.5 * tan
    RIDGE_Z = EAVE + RISE
    homes.plinth(m, X0, X1, Y0, Y1, BASE)
    m.section('foundation')

    win = dict(paint='C', sash='A', shutter='A', casing_w=0.09)
    DOOR_X, DOOR_W, DOOR_H = 2.3, 2.4, 2.5           # the sliding door's opening on the front gable
    LOFT = Opening('door', 3.05, WALL + 0.72, 0.9, 1.1, style='plank', finish='A', casing='C', hinge='left')
    walls = [
        ([Opening('door', DOOR_X, 0.0, DOOR_W, DOOR_H), homes.raised_cut(LOFT)], True),
        ([Opening('window', 2.0, 1.4, 0.75, 0.75, style='shut', **win),
          Opening('door', 5.6, 0.0, 1.0, 2.15, style='plank', finish='A', casing='C', hinge='right'),
          Opening('window', 8.0, 1.4, 0.75, 0.75, style='open', **win)], False),
        ([Opening('window', 3.1, 1.3, 0.8, 0.8, style='shut', **win)], True),
        ([Opening('window', 2.5, 1.4, 0.75, 0.75, style='open', **win),
          Opening('window', 7.0, 1.4, 0.75, 0.75, style='shut', **win)], False),
    ]
    spaces = wall_spaces(X0, X1, Y0, Y1, BASE)
    for k, ((space, length), (items, gable)) in enumerate(zip(spaces, walls)):
        outline = gable_outline(length, WALL, RISE) if gable else rect(length, WALL)
        # The sliding door's opening and the loft door's are cut here and built below; the rest as usual.
        shown = [] if k == 0 else items
        homes.shell(m, notch(outline, items), kit.holes(items), 0.16,
                    Trim('A', world=True, rotate=True, u=rng.uniform(0.0, kit.U_REPEAT)), space, around=items)
        edges = [(-0.1, 0.13, -1.0, 30.0), (length - 0.13, length + 0.1, -1.0, 30.0)]
        homes.battens(m, space, outline, 0.0, length, homes.keep_out(items) + edges, look='A', every=0.4)
        homes.openings(m, space, shown, 0.16)
        homes.corners(m, space, length, outline[-1][1] - 0.02, outline[2][1] - 0.02, look='C', width=0.13, wrap=gable)
    front = spaces[0][0]
    homes.raised_door(m, front, LOFT, 0.16)
    m.section('walls')

    # The sliding door: planks and battens hung from an iron track over the opening, slid a little to the right on
    # the dark inside; its stop, a hasp.
    track_z = DOOR_H + 0.18
    town.board(m, (DOOR_X - 0.15, -0.03, track_z), (DOOR_X + DOOR_W * 2.0 + 0.1, -0.03, track_z), 0.09, 0.03,
               look='H3', space=front, drop=('+y',))
    slide = 0.55
    lx = DOOR_X + slide
    boards = 12
    for k in range(boards):
        x = lx + DOOR_W * (k + 0.5) / boards + 0.04
        town.board(m, (x, -0.07, 0.0), (x, -0.07, DOOR_H + 0.06 - rng.uniform(0.0, 0.02)), DOOR_W / boards - 0.006, 0.045,
                   look='A', space=front, drop=('+y',))
    for z in (0.35, DOOR_H * 0.5, DOOR_H - 0.3):
        town.board(m, (lx + 0.1, -0.11, z), (lx + DOOR_W, -0.11, z), 0.16, 0.035, look='A', space=front, drop=('+y',))
    town.board(m, (lx + DOOR_W - 0.05, -0.115, 0.45), (lx + 0.15, -0.115, DOOR_H - 0.4), 0.14, 0.03, look='A', space=front,
               drop=('+y',))
    for x in (lx + 0.35, lx + DOOR_W - 0.3):
        m.box((0.06, 0.03, 0.28), at=(x, -0.06, track_z - 0.12), uv=Trim('H3', fit=True), space=front)
    m.box((0.06, 0.04, 0.18), at=(lx + 0.12, -0.14, 1.1), uv=Trim('H3', fit=True), space=front)
    # The dark inside, seen through the gap, and the floor's edge at the sill.
    homes.quad(m, [(DOOR_X, 0.7, 0.0), (DOOR_X + slide + 0.1, 0.7, 0.0), (DOOR_X + slide + 0.1, 0.7, DOOR_H),
                   (DOOR_X, 0.7, DOOR_H)], Trim('H4', fit=True), space=front, facing=(0.0, -1.0, 0.0))
    homes.quad(m, [(DOOR_X, 0.0, 0.005), (DOOR_X + slide + 0.1, 0.0, 0.005), (DOOR_X + slide + 0.1, 0.7, 0.005),
                   (DOOR_X, 0.7, 0.005)], Trim('A', world=True), space=front, facing=(0.0, 0.0, 1.0))
    town.trim_board(m, front, (DOOR_X - 0.07, 0.0, -0.02), (DOOR_X - 0.07, 0.0, DOOR_H + 0.08), 0.12, 'C', thick=0.05)
    m.section('sliding door')

    # FEED & GRAIN across the gable; the hoist beam out of the peak over the loft door, its block and rope.
    town.text(m, 'FEED & GRAIN', ((X1 - X0) * 0.5, -0.03, WALL + 0.38), height=0.42, width=4.4, space=front,
              look='band_G')
    apex = WALL + RISE
    HY = 1.1
    m.board((0.0, Y0 + 0.6, BASE + apex - 0.5), (0.0, Y0 - HY, BASE + apex - 0.5), 0.2, 0.18, face=(0.0, 0.0, 1.0),
            uv=Trim('C', lane='each'))
    m.box((0.1, 0.1, 0.2), at=(0.0, Y0 - HY + 0.12, BASE + apex - 0.72), uv=IRON, mat='iron', bevel=0.02)
    m.cylinder((0.0, Y0 - HY + 0.12, BASE + apex - 0.82), (0.0, Y0 - HY + 0.12, BASE + 2.4), 0.016, sides=5,
               uv=Trim('A', lane=3), caps=(False, False))
    m.box((0.05, 0.05, 0.14), at=(0.0, Y0 - HY + 0.12, BASE + 2.33), uv=IRON, mat='iron')
    m.section('lettering, hoist')

    # The loading dock across the front: a plank deck on posts at the floor's height, a bumper timber, steps down at
    # its right end.
    DY = 2.5
    dx0, dx1 = X0 - 0.3, X1 + 0.3
    homes.porch_deck(m, dx0, dx1, Y0 - DY, Y0, BASE, posts=(dx0 + 0.15, -1.2, 1.2, dx1 - 0.15), front=True, sill=True,
                     ends=(dx0, dx1))
    m.board((dx0, Y0 - DY - 0.08, BASE - 0.14), (dx1, Y0 - DY - 0.08, BASE - 0.14), 0.22, 0.14, uv=Trim('C', lane='each'))
    homes.steps(m, dx1 - 0.75, 1.1, Y0 - DY, BASE, homes.step_count(BASE))
    m.section('dock')

    frame = Matrix.Rotation(math.radians(90.0), 4, 'Z')
    right = homes.plane(m, Y0, Y1, -X1, EAVE, PITCH, (X1 - X0) * 0.5, 1.0, overhang=0.35, rake=(0.45, 0.3), deck=0.08,
                        sag=0.08, frame=frame)
    left = homes.plane(m, Y0, Y1, -X0, EAVE, PITCH, (X1 - X0) * 0.5, -1.0, overhang=0.35, rake=(0.45, 0.3), deck=0.08,
                       sag=0.08, frame=frame)
    homes.cover(m, right, 'R4', tears=1)
    homes.cover(m, left, 'R4', tears=2)
    homes.ridge(m, right, left, 'R4')
    m.section('roof')

    m.hull((X1 - X0 + 0.1, Y1 - Y0 + 0.1, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    m.hull_points([(x, y, EAVE - 0.35 * tan + 0.22) for x in (X0 - 0.35, X1 + 0.35) for y in (Y0 - 0.45, Y1 + 0.3)] +
                  [(0.0, y, RIDGE_Z + 0.25) for y in (Y0 - 0.45, Y1 + 0.3)])
    m.hull((0.22, HY + 0.6, 0.2), at=(0.0, Y0 - HY * 0.5 + 0.3, BASE + apex - 0.5))
    return m.finish(pivot=(X0 + DOOR_X + DOOR_W * 0.5, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_BoardingHouse ---

def boarding_house():
    m = homes.Home('RR_BoardingHouse', seed=703, lived=True, window_sockets=True)
    rng = m.rng
    X0, X1, Y0, Y1 = -5.5, 5.5, -4.0, 4.0
    BASE, STOREY, PITCH = 0.55, 2.75, 32.0
    WALL = STOREY * 2.0
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (Y1 - Y0) * 0.5 * tan
    RIDGE_Z = EAVE + RISE
    homes.plinth(m, X0, X1, Y0, Y1, BASE)
    m.section('foundation')

    lit = dict(paint='H1', sash='H1', lit=True, panes=(2, 2))
    dark = dict(paint='H1', sash='H1', panes=(2, 2))
    W, H, Z = 0.82, 1.45, 0.85
    U = STOREY + 0.72
    DOOR = Opening('door', 5.0, 0.0, 1.0, 2.3, style='panel', finish='planks', casing='H1', knob='H3', transom=0.32,
                   glass=True)
    UPDOOR = Opening('door', 5.3, STOREY + 0.02, 0.9, 2.1, style='plank', finish='planks', casing='H1', hinge='right')
    walls = [
        [Opening('window', 0.9, Z, W, H, **lit), Opening('window', 2.6, Z, W, H, **dark), DOOR,
         Opening('window', 7.6, Z, W, H, **dark), Opening('window', 9.3, Z, W, H, **lit),
         Opening('window', 0.9, U, W, 1.35, **lit), Opening('window', 3.4, U, W, 1.35, **lit),
         Opening('window', 6.8, U, W, 1.35, **dark), Opening('window', 9.3, U, W, 1.35, **lit)],
        [Opening('window', 1.6, Z, W, H, **dark), Opening('window', 5.5, Z, W, H, **dark), UPDOOR,
         Opening('window', 1.6, U, W, 1.35, **lit), Opening('window', 3.4, WALL + 0.35, 0.7, 0.85, **dark)],
        [Opening('door', 2.0, 0.0, 0.95, 2.15, style='plank', finish='A', casing='H1', hinge='right'),
         Opening('window', 4.6, Z, W, H, **dark), Opening('window', 8.2, Z, W, H, **dark),
         Opening('window', 4.6, U, W, 1.35, **dark), Opening('window', 8.2, U, W, 1.35, **dark)],
        [Opening('window', 2.2, Z, W, H, **dark), Opening('window', 5.4, Z, W, H, **dark),
         Opening('window', 3.8, U, W, 1.35, **dark), Opening('window', 3.6, WALL + 0.35, 0.7, 0.85, **dark)],
    ]
    for k, ((space, length), items) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls)):
        gable = k % 2 == 1
        outline = gable_outline(length, WALL, RISE) if gable else rect(length, WALL)
        upper = [o for o in items if o.kind == 'door' and o.z > STOREY - 0.1]
        lower = [o for o in items if o not in upper]
        clad(m, space, length, outline, lower + [homes.raised_cut(o) for o in upper], look='band_G', trim='H1',
             wrap=not gable, openings_for=lower)
        for o in upper:      # the upper door off the stair's landing: its sill is the landing's floor
            homes.raised_door(m, space, o, 0.16, out=LAP)
        homes.band(m, space, [(0.14, length - 0.14)], STOREY + 0.02, 0.18, look='H1', out=LAP)
        if not gable:
            homes.band(m, space, [(0.12, length - 0.12)], WALL - 0.115, 0.19, look='H1', out=LAP)
    m.section('walls')

    # The roof: R1 shakes, the stone chimney through the ridge at the left, the kitchen stovepipe through the back.
    CH = (X0 + 1.6, 0.0)
    PIPE = (X1 - 2.4, Y1 - 1.1)
    front = homes.plane(m, X0, X1, Y0, EAVE, PITCH, (Y1 - Y0) * 0.5, 1.0, overhang=0.4, rake=(0.35, 0.35), deck=0.1,
                        sag=0.06)
    back = homes.plane(m, X0, X1, Y1, EAVE, PITCH, (Y1 - Y0) * 0.5, -1.0, overhang=0.4, rake=(0.35, 0.35), deck=0.1,
                       sag=0.07)
    homes.cover(m, front, 'R1', skip=homes.skip_round(front, [(CH[0], CH[1], 0.42, 0.48)]))
    homes.cover(m, back, 'R1', skip=homes.skip_round(back, [(CH[0], CH[1], 0.42, 0.48), (PIPE[0], PIPE[1], 0.14, 0.14)]))
    homes.ridge(m, front, back, 'R1')
    homes.stack(m, CH[0], CH[1], RIDGE_Z - 1.0, RIDGE_Z + 0.85, size=(0.7, 0.8))
    homes.chimney_pipe_roof(m, PIPE[0], PIPE[1], homes.roof_z(EAVE, Y1, PIPE[1], PITCH) - 0.3, RIDGE_Z + 0.3,
                            radius=0.1, socket='Smoke_2')
    m.section('roof, chimneys')

    # P2: the shed porch across the front under the upper windows, its lantern by the door.
    homes.porch(m, X0 + 1.4, X1 - 1.4, Y0 - LAP, 2.0, BASE, BASE + 2.3, BASE + STOREY - 0.08, kind='P2', roof='R1',
                posts=(X0 + 1.52, -1.1, 1.1, X1 - 1.52), steps=0.0, steps_w=1.6)
    homes.porch_lantern(m, 0.92, Y0 - LAP, BASE + 1.75)
    m.section('porch')

    # The outside stair up the right gable to the upper door: treads between stringers, a landing on posts, a rail.
    SX = X1 + 0.7                                    # the stair's middle, out from the wall
    UP = BASE + STOREY
    LAND = (Y0 + 5.3 - 0.25, Y0 + 5.3 + 1.15)       # the landing's front and back edges (at the door)
    homes.porch_deck(m, X1 + 0.05, X1 + 1.35, LAND[0], LAND[1], UP, posts=(X1 + 1.2,), front=True, back=True, sill=False,
                     ends=(X1 + 1.35,))
    count = int(math.ceil(UP / 0.19)) - 1
    tread_hulls = len(m.hulls)
    homes.steps(m, SX, 1.0, LAND[0], UP, count, run=0.27)
    foot = LAND[0] - 0.27 * count
    # One ramp under the treads instead of a hull a tread (the capsule climbs it as stairs, and 16 hulls go).
    del m.hulls[tread_hulls:]
    m.hull_points([(SX + dx, y, z) for dx in (-0.5, 0.5) for y, z in ((foot - 0.05, 0.0), (LAND[0], UP), (LAND[0], 0.0))])
    for y0, y1, z0, z1 in ((foot, LAND[0], 0.95, UP + 0.95), (LAND[0], LAND[1], UP + 0.95, UP + 0.95)):
        m.board((SX + 0.55, y0, z0), (SX + 0.55, y1, z1), 0.08, 0.06, face=(0.0, 0.0, 1.0), uv=Trim('C', lane='each'))
    for y, z in ((foot + 0.1, 0.0), (LAND[0], UP), (LAND[1] - 0.05, UP)):
        m.board((SX + 0.55, y, z), (SX + 0.55, y, z + 1.0), 0.08, 0.08, uv=Trim('C', lane='each'))
    t = 0.0
    while t < 1.0:
        y = foot + (LAND[0] - foot) * t
        z = UP * t
        m.board((SX + 0.55, y, z + 0.12), (SX + 0.55, y, z + 0.92), 0.045, 0.03, uv='C')
        t += 0.12
    m.hull_points([(SX + dx, y, z + dz) for dx in (0.5, 0.6) for y, z in ((foot, 0.0), (LAND[0], UP))
                   for dz in (0.0, 1.0)])
    m.hull((0.1, LAND[1] - LAND[0], 1.0), at=(SX + 0.55, (LAND[0] + LAND[1]) * 0.5, UP + 0.5))
    m.section('stair')

    # ROOMS: a blade sign off the front at the upper storey, on an iron bracket, lettered both sides.
    BX, BZ = X0 + 2.56, BASE + STOREY + 1.45        # between the first two upper windows
    m.board((BX, Y0 - LAP, BZ + 0.45), (BX, Y0 - 1.15, BZ + 0.45), 0.04, 0.05, face=(0.0, 0.0, 1.0), uv=IRON, mat='iron')
    m.board((BX, Y0 - LAP, BZ - 0.2), (BX, Y0 - 0.9, BZ + 0.43), 0.03, 0.03, face=(1.0, 0.0, 0.0), uv=IRON, mat='iron')
    for y in (Y0 - 0.35, Y0 - 1.0):
        m.box((0.02, 0.02, 0.1), at=(BX, y, BZ + 0.37), uv=IRON, mat='iron')
    for side in (-1.0, 1.0):
        blade = Space(Matrix.Translation((BX, Y0 - 0.68, BZ)) @ Matrix.Rotation(math.radians(-90.0 * side), 4, 'Z'))
        if side < 0:
            town.box(m, (0.95, 0.05, 0.6), at=(0.0, 0.0, 0.0), look='H2', space=blade)
            for dz in (-0.32, 0.32):
                town.box(m, (1.0, 0.07, 0.05), at=(0.0, 0.0, dz), look='C', space=blade)
        town.text(m, 'ROOMS', (0.0, -0.026, 0.0), height=0.3, width=0.8, space=blade, look='band_G')
    m.section('sign')

    m.hull((X1 - X0 + 0.1, Y1 - Y0 + 0.1, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    homes.gable_hull(m, X0, X1, Y0, Y1, EAVE, EAVE, 0.0, RIDGE_Z, overhang=0.4, rake=0.35, pitch_front=PITCH)
    m.socket('Light', (-3.4, Y0 + 1.0, BASE + 1.5))
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1))


# --- RR_Bathhouse ---

def bathhouse():
    m = homes.Home('RR_Bathhouse', seed=704, lived=True, window_sockets=True)
    rng = m.rng
    X0, X1, Y0, Y1 = -3.0, 3.0, -2.5, 2.5
    BASE, WALL, PITCH = 0.3, 3.0, 30.0              # tall enough for the sign between the door and the eave
    EAVE = BASE + WALL
    tan = math.tan(math.radians(PITCH))
    RISE = (Y1 - Y0) * 0.5 * tan
    RIDGE_Z = EAVE + RISE
    homes.plinth(m, X0, X1, Y0, Y1, BASE)
    m.section('foundation')

    lit = dict(paint='C', sash='C', lit=True, panes=(2, 2))
    dark = dict(paint='C', sash='C', panes=(2, 2))
    W, H, Z = 0.8, 1.2, 0.95
    DOOR = Opening('door', 2.5, 0.0, 1.0, 2.05, style='plank', finish='A', casing='C', hinge='left')
    walls = [
        [Opening('window', 0.75, Z, W, H, **lit), DOOR, Opening('window', 4.45, Z, W, H, **lit)],
        [Opening('window', 1.6, Z + 0.25, 0.7, 0.8, **dark)],
        [Opening('window', 1.5, Z + 0.2, 0.8, 0.9, **dark), Opening('window', 3.8, Z + 0.2, 0.8, 0.9, **dark)],
        [Opening('window', 2.1, Z, W, H, **dark)],
    ]
    for k, ((space, length), items) in enumerate(zip(wall_spaces(X0, X1, Y0, Y1, BASE), walls)):
        gable = k % 2 == 1
        outline = gable_outline(length, WALL, RISE) if gable else rect(length, WALL)
        clad(m, space, length, outline, items, look='A', trim='C', wrap=not gable, frieze=not gable)
    front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
    # Over the door, in front of the frieze board (its face 4 cm out of the clapboards').
    sign_board(m, front, DOOR.x + DOOR.w * 0.5, 2.56, 'BATHS . LAUNDRY', 2.6, 0.36, y=-LAP - 0.045)
    homes.stone_steps(m, 0.0, 1.3, Y0 - 0.04, BASE, 1, run=0.36)
    m.section('walls, sign')

    front_roof = homes.plane(m, X0, X1, Y0, EAVE, PITCH, (Y1 - Y0) * 0.5, 1.0, overhang=0.3, rake=(0.32, 0.32),
                             deck=0.08, sag=0.05)
    back_roof = homes.plane(m, X0, X1, Y1, EAVE, PITCH, (Y1 - Y0) * 0.5, -1.0, overhang=0.3, rake=(0.32, 0.32),
                            deck=0.08, sag=0.05)
    homes.cover(m, front_roof, 'R4', tears=0)
    homes.cover(m, back_roof, 'R4', tears=1)
    homes.ridge(m, front_roof, back_roof, 'R4')
    # A steam vent on the ridge: a little cap of boards on posts (SOCKET_Smoke_2: steam from the tubs).
    for x in (-0.8, -0.2):
        for y in (-0.1, 0.1):
            m.box((0.05, 0.05, 0.2), at=(x, y, RIDGE_Z + 0.08), uv=Trim('C', lane='each'))
    for side in (-1.0, 1.0):
        m.box((0.8, 0.26, 0.025), matrix=place((-0.5, side * 0.1, RIDGE_Z + 0.24), (side * -30.0, 0.0, 0.0)),
              uv=Trim('A', width=0.26))
    m.socket('Smoke_2', (-0.5, 0.0, RIDGE_Z + 0.16))
    m.section('roof')

    # The boiler against the right gable: a fieldstone firebox with its iron fire door (SOCKET_Fire), an iron boiler
    # lying on it, its stovepipe up past the ridge (SOCKET_Smoke), a pipe into the wall, under a little R4 roof.
    BX0, BX1 = X1 + 0.05, X1 + 1.75
    BY = 0.4
    kit.stone_stack(m, ((BX0 + BX1) * 0.5, BY, -0.15), (1.5, 1.1), 0.95, taper=0.03, bevel=0.03)
    m.box((0.04, 0.42, 0.34), at=(BX1 - 0.06, BY - 0.05, 0.42), uv=IRON, mat='iron', bevel=0.01)
    m.box((0.05, 0.08, 0.06), at=(BX1 - 0.02, BY + 0.12, 0.5), uv=IRON, mat='iron')
    m.socket('Fire', (BX1 - 0.12, BY - 0.05, 0.42), (0.0, 0.0, 90.0))
    BZ = 1.23                                         # the boiler's axis: it lies on the firebox's top
    m.cylinder(((BX0 + BX1) * 0.5 - 0.65, BY, BZ), ((BX0 + BX1) * 0.5 + 0.6, BY, BZ), 0.42, sides=12, uv=IRON,
               mat='iron', face=(0.0, 0.0, 1.0))
    for x in ((BX0 + BX1) * 0.5 - 0.35, (BX0 + BX1) * 0.5 + 0.3):
        m.cylinder((x - 0.03, BY, BZ), (x + 0.03, BY, BZ), 0.445, sides=12, uv=IRON, mat='iron', face=(0.0, 0.0, 1.0),
                   caps=(False, False))
    m.cylinder((X1 + LAP, BY + 0.2, BZ + 0.2), ((BX0 + BX1) * 0.5 - 0.6, BY + 0.2, BZ + 0.2), 0.05, sides=6, uv=IRON,
               mat='iron', caps=(False, False))
    homes.stovepipe(m, (BX0 + BX1) * 0.5 + 0.3, BY, BZ + 0.36, RIDGE_Z + 0.9, radius=0.11)
    lean = homes.shed_roof(m, BX0 - 0.02, BX1 + 0.05, BY + 1.15, EAVE - 0.55, BY - 0.85, EAVE - 0.3, overhang=0.2,
                           rake=(0.15, 0.15), deck=0.05, sag=0.02)
    # The little roof's plane runs along X (from the wall out), so it is built across: a shed whose eave is at its far
    # side would be neater; this one falls to the back (y), out from under the main eave.
    homes.cover(m, lean, 'R4', tears=0)
    for x in (BX1 - 0.05,):
        for y in (BY - 0.75, BY + 1.05):
            m.board((x, y, -0.05), (x, y, EAVE - 0.5 - (0.2 if y > BY else 0.0)), 0.12, 0.12, uv=Trim('C', lane='each'))
    m.hull((BX1 - BX0, 1.1, 0.95), at=((BX0 + BX1) * 0.5, BY, 0.4))
    m.hull((1.3, 0.9, 0.9), at=((BX0 + BX1) * 0.5, BY, BZ))
    m.hull((0.25, 0.25, RIDGE_Z + 0.9 - BZ - 0.36), at=((BX0 + BX1) * 0.5 + 0.3, BY, (RIDGE_Z + 0.9 + BZ + 0.36) * 0.5))
    homes.slope_hull(m, lean, thick=0.12)
    m.section('boiler')

    # The wash bench along the front, left of the door, two tubs on it, a washboard leaning.
    homes.seat(m, X0 + 1.2, Y0 - LAP, 0.0, length=1.6, height=0.62, deep=0.42)
    for x in (X0 + 0.75, X0 + 1.6):
        homes.lathe(m, [(0.0, 0.0), (0.2, 0.0), (0.27, 0.22), (0.25, 0.235), (0.0, 0.235)], (x, Y0 - 0.25, 0.62), 10,
                    IRON, 'iron')
    m.box((0.36, 0.03, 0.62), at=(X0 + 2.15, Y0 - 0.2, 0.3), rot=(-12.0, 0.0, 0.0), uv=Trim('C', width=0.36))
    m.section('wash bench')

    m.hull((X1 - X0 + 0.1, Y1 - Y0 + 0.1, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    homes.gable_hull(m, X0, X1, Y0, Y1, EAVE, EAVE, 0.0, RIDGE_Z, overhang=0.3, rake=0.32, pitch_front=PITCH)
    m.socket('Light', (0.0, Y0 + 1.0, BASE + 1.5))
    return m.finish(pivot=(X0 + DOOR.x + DOOR.w * 0.5, Y0), footprint=(X0, X1, Y0, Y1))


BUILDERS = [('SMITHY', smithy), ('FEED', feed_store), ('BOARDING', boarding_house), ('BATHS', bathhouse)]
CAPTIONS = {
    'RR_Smithy_Town': 'The smithy: open-fronted, the forge, anvil, wheels.',
    'RR_FeedStore': 'The feed store: a warehouse on its loading dock.',
    'RR_BoardingHouse': 'The boarding house (lived in): ROOMS, an outside stair.',
    'RR_Bathhouse': 'The bathhouse and laundry: the boiler on its firebox.',
}
SHOTS = {
    'RR_Smithy_Town': dict(view=(-0.45, -1.0, 0.3)),
    'RR_Bathhouse': dict(view=(0.65, -1.0, 0.38), figure='left'),
    'RR_BoardingHouse': dict(view=(0.6, -1.0, 0.34), figure='left'),
}

models = [build() for key, build in BUILDERS if not ONLY or key in ONLY]

if lt.want_preview():
    homes.previews(models, CAPTIONS, sheet=False, shots_by_name=SHOTS, out_dir=PREVIEW_DIR)
    if not ONLY:
        homes.lineup(models, os.path.join(PREVIEW_DIR, 'Lineup_TownBuildings.png'), gap=3.0)
