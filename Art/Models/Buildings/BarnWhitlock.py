"""Amos Whitlock's hay barn in Whitlock Fields on Ransom's Rest (Docs/Areas/RansomsRest_Expansion.md, section 9; Side 2,
Unfinished Business: his six bales go into it, and what he owed his hired hands is chalked inside its door). It replaces
Barn_Ransom at Whitlock Fields (the farm keeps Barn_Ransom).

  RR_Barn_Whitlock  a long, low hay barn, 15 x 8 m, under one long roof of shakes: weathered board and batten closing its
                    back and both gables, the front open along two thirds of its length (the hay bay, 10 m between four
                    posts under a heavy plate beam on knee braces, the roof running on past it), the last third the
                    tack room (its door standing open, a window shutter folded back). Inside the bay: a heap of loose hay
                    against the back wall, a few bales, hay strewn on the earth floor, a pitchfork, tie beams overhead;
                    the place where the six bales stack (SOCKET_BaleStack). Inside the tack room: a board floor, harness
                    pegs with tack hung on them, a saddle on its rack, a grain bin, a lantern hook, and on the inside of
                    the door the hands' wages in chalk: OWED / MERRITT 14.00 / MERRITT 14.00 / TANNER 11.50 (drafts for the
                    story; nothing crossed out).

The pivot follows the homes kit (looter_homes): the middle of the hay bay's opening on the front line (the posts' outer
faces), at the ground (z = 0); the front faces -Y. Its BARN: line gives the walls' extent and the tack room's door from it.

Sockets: BaleStack (the six bales' stack: its middle on the floor, its front out of the bay; the stack runs along its
sideways axis, as build_area_whitlock.py's STACK_SLOTS do), WagesChalk (on the door's inside, facing out of it), Decal (a
poster's spot left of the tack room's door), Light_Tack (the lantern hook) and Light_Bay (a lantern on the middle post).

A scripted model (Art/README.md) built with looter_rr_town (on looter_homes, looter_town, looter_buildings).
  --preview   renders it in the area's late-afternoon light, and inside its bay and tack room, into
              Saved/ArtPreviews/RansomsRest/Town/, and writes BarnWhitlock_report.json
  --stats     prints where the triangles go; --no-ao skips the AO bake
"""
import math

import looter_buildings as kit
import looter_homes as homes
import looter_rr_town as rr
import looter_textures as lt
import looter_town as town
from looter_buildings import Matrix, Opening, Space, Tile, Trim, Vector

X0, X1, Y0, Y1 = -7.5, 7.5, -4.0, 4.0        # the walls' outer faces (the front's is the posts' line)
T = 0.12
XI0, XI1, YI0, YI1 = X0 + T, X1 - T, Y0 + T, Y1 - T
WALL_H = 3.4                                 # ground to the plates
PITCH = 24.0
TACK = -2.5                                  # the tack room's partition (x); the hay bay is east of it
BAY_POSTS = (TACK, 0.85, 4.15, X1 - 0.1)
TACK_FLOOR = 0.2
TACK_DOOR = (-5.6, -4.5)
PIVOT = ((TACK + X1) * 0.5, Y0)


def barn():
    m = rr.Building('RR_Barn_Whitlock', seed=911)
    rng = m.rng
    tan = math.tan(math.radians(PITCH))
    rise = (Y1 - Y0) * 0.5 * tan
    ridge = WALL_H + rise
    siding = lambda: Trim('A', world=True, rotate=True, u=rng.uniform(0.0, 6.4))

    # Footings: fieldstone under the sills and the posts.
    for x in (X0, TACK, X1):
        for y in (Y0 + 0.1, 0.0, Y1 - 0.1):
            kit.stone_stack(m, (x, y, -0.1), (0.4, 0.4), 0.28, taper=0.08, rot=rng.uniform(-8.0, 8.0), bevel=0.0)
    for x in BAY_POSTS[1:3]:
        kit.stone_stack(m, (x, Y0 + 0.1, -0.1), (0.4, 0.4), 0.28, taper=0.08, rot=rng.uniform(-8.0, 8.0),
                        bevel=0.0)
    m.section('footings')

    # The walls: the back, both gables and the tack room's front, board and batten from a sill on the footings.
    TW = Opening('window', 3.3, 1.2, 0.8, 0.8)
    DOOR = Opening('door', TACK_DOOR[0] - X0, 0.0, TACK_DOOR[1] - TACK_DOOR[0], 2.15)
    walls = [((X1, Y0), (X1, Y1), [], True), ((X1, Y1), (X0, Y1), [], False), ((X0, Y1), (X0, Y0), [TW], True),
             ((X0, Y0), (TACK, Y0), [DOOR], False)]
    spaces = []
    for p0, p1, items, gable in walls:
        space = kit.wall_space(p0, p1, 0.1)
        length = (Vector(p1) - Vector(p0)).length
        top = WALL_H - 0.1
        outline = [(0.0, 0.0), (length, 0.0), (length, top)]
        if gable:
            outline.append((length * 0.5, top + rise - 0.02))
        outline.append((0.0, top))
        notched = [outline[0]]
        for o in sorted((o for o in items if o.kind == 'door'), key=lambda o: o.x):
            notched += [(o.x, 0.0), (o.x, o.h), (o.x + o.w, o.h), (o.x + o.w, 0.0)]
        notched += outline[1:]
        town.panel(m, notched, kit.holes(items), T, siding(), space=space, around=items, slices=0, back=True)
        edges = [(-0.1, 0.12, -1.0, 30.0), (length - 0.12, length + 0.1, -1.0, 30.0)]
        homes.battens(m, space, outline, 0.0, length, homes.keep_out(items) + edges, look='A', every=0.55, z0=0.05)
        homes.corners(m, space, length, top - 0.02, top - 0.02, look='C', width=0.13, wrap=not gable)
        spaces.append(space)
    for p0, p1 in (((X0 - 0.06, Y0 - 0.06), (TACK + 0.1, Y0 - 0.06)), ((X1 + 0.06, Y1 + 0.06), (X0 - 0.06, Y1 + 0.06))):
        town.board(m, (p0[0], p0[1], 0.07), (p1[0], p1[1], 0.07), 0.16, 0.18, look='C',
                   face=(0.0, -1.0 if p0[1] < 0 else 1.0, 0.0))
    west = spaces[2]
    # The tack room's window: a plank shutter folded back on its pintles, no glass.
    town.box(m, (TW.w + 0.2, 0.18, 0.05), at=(TW.x + TW.w * 0.5, 0.0, TW.z - 0.03), look='C', space=west)
    for x in (TW.x - 0.05, TW.x + TW.w + 0.05):
        town.trim_board(m, west, (x, 0.0, TW.z - 0.02), (x, 0.0, TW.z + TW.h + 0.1), 0.1, 'C', thick=0.035)
    town.trim_board(m, west, (TW.x - 0.12, 0.0, TW.z + TW.h + 0.06), (TW.x + TW.w + 0.12, 0.0, TW.z + TW.h + 0.06),
                    0.12, 'C', thick=0.04)
    shutter = Space(west.matrix @ Matrix.Translation((TW.x - 0.1, -0.06, TW.z)) @
                    Matrix.Rotation(math.radians(170.0), 4, 'Z'))
    for k in range(3):
        town.board(m, (0.04 + 0.28 * k * 0.95, 0.0, 0.0), (0.04 + 0.28 * k * 0.95, 0.0, TW.h), 0.26, 0.03, look='A',
                   space=shutter)
    m.section('walls')

    # The bay's front: posts, a heavy plate on them with knee braces; tie beams across over the bay; the roof of shakes.
    for x in BAY_POSTS:
        town.board(m, (x, Y0 + 0.1, 0.17), (x, Y0 + 0.1, WALL_H - 0.24), 0.22, 0.22, look='C', face=(0.0, -1.0, 0.0))
        town.board(m, (x, Y1 - 0.1, 0.17), (x, Y1 - 0.1, WALL_H - 0.24), 0.2, 0.2, look='C', face=(0.0, 1.0, 0.0))
        for side in (-1.0, 1.0):
            if TACK < x + side * 0.8 < X1:
                town.board(m, (x + side * 0.1, Y0 + 0.1, WALL_H - 1.05), (x + side * 0.8, Y0 + 0.1, WALL_H - 0.3), 0.14,
                           0.11, look='C')
        town.board(m, (x, Y0 + 0.1, WALL_H - 0.18), (x, Y1 - 0.1, WALL_H - 0.18), 0.2, 0.16, look='C',
                   face=(0.0, 0.0, -1.0))
    town.board(m, (TACK - 0.1, Y0 + 0.1, WALL_H - 0.13), (X1, Y0 + 0.1, WALL_H - 0.13), 0.24, 0.24, look='C')
    front_roof, back_roof = town.gable(m, X0, X1, Y0, Y1, WALL_H, PITCH, overhang=0.6, rake=0.35, deck=0.08, sag=0.08)
    for slope in (front_roof, back_roof):
        town.shakes(m, slope, piece=(3.4, 5.2))
    town.ridge_cap(m, front_roof, back_roof, uv='C', width=0.16, thick=0.05)
    town.board(m, (X0 + 0.1, 0.0, ridge - 0.18), (X1 - 0.1, 0.0, ridge - 0.18), 0.22, 0.1, look='C',
               face=(0.0, -1.0, 0.0))
    m.section('bay frame, roof')

    # The partition between the bay and the tack room: boards both sides; the tack room's inside: its board floor, walls.
    rr.wall_cells(m, (TACK - T * 0.5, Y1 - 0.1), (TACK - T * 0.5, Y0 + 0.1), 0.0, 0.1, WALL_H - 0.3, [], siding())
    rr.wall_cells(m, (TACK + T * 0.5, Y0 + 0.1), (TACK + T * 0.5, Y1 - 0.1), 0.0, 0.1, WALL_H - 0.3, [], siding())
    rr.flat_cells(m, XI0, TACK - T * 0.5, YI0, YI1, TACK_FLOOR, uv=Tile('WoodPlanks', along=None), every=2.0)
    for y in (YI0 + 0.05, YI1 - 0.05):
        town.board(m, (XI0, y, TACK_FLOOR * 0.5), (TACK - T * 0.5, y, TACK_FLOOR * 0.5), TACK_FLOOR, 0.1, look='C',
                   face=(0.0, -1.0 if y < 0 else 1.0, 0.0))
    rr.flat_cells(m, TACK + T * 0.5, X1 - T, Y0 + 0.1, YI1, 0.03, uv=Tile('GroundDirt', along=None), mat='dirt',
                  every=2.5)
    m.section('partition, floors')

    # The tack room's door, standing open inward with the wages chalked on its inside; steps up to its sill.
    leaf_w = DOOR.w - 0.02
    leaf = rr.hinged((TACK_DOOR[1] - 0.02, YI0 + 0.05, TACK_FLOOR), -70.0)      # its inside toward the room
    rr.plank_leaf(m, leaf, leaf_w, 2.1 - TACK_FLOOR + 0.1, look='A', battens_on=-1.0, hinge_left=False)
    inside = Space(leaf.matrix @ Matrix.Rotation(math.radians(180.0), 4, 'Z'))
    chalk_y = -(0.045 * 0.5 + 0.0015)
    widths = rr.chalk(m, inside, ['OWED', 'MERRITT  14.00', 'MERRITT  14.00', 'TANNER  11.50'],
                      (0.26, chalk_y, 1.62), height=0.06, tol=0.06, gap=1.9)
    m.socket('WagesChalk', inside.world((0.26 + max(widths) * 0.5, chalk_y - 0.02, 1.62 - 0.17)),
             rr.rot_to(tuple((inside.matrix.to_3x3() @ Vector((0.0, -1.0, 0.0)))[:2])))
    town.trim_board(m, spaces[3], (DOOR.x - 0.06, 0.0, -0.05), (DOOR.x - 0.06, 0.0, DOOR.h + 0.1), 0.12, 'C', thick=0.04)
    town.trim_board(m, spaces[3], (DOOR.x + DOOR.w + 0.06, 0.0, -0.05), (DOOR.x + DOOR.w + 0.06, 0.0, DOOR.h + 0.1),
                    0.12, 'C', thick=0.04)
    town.trim_board(m, spaces[3], (DOOR.x - 0.14, 0.0, DOOR.h + 0.08), (DOOR.x + DOOR.w + 0.14, 0.0, DOOR.h + 0.08),
                    0.14, 'C', thick=0.045)
    m.box((DOOR.w + 0.3, 0.4, 0.12), at=((TACK_DOOR[0] + TACK_DOOR[1]) * 0.5, Y0 - 0.2, 0.06), uv=Trim('D', world=True))
    m.hull((DOOR.w + 0.3, 0.4, 0.12), at=((TACK_DOOR[0] + TACK_DOOR[1]) * 0.5, Y0 - 0.2, 0.06))
    m.socket('Decal', spaces[3].world((DOOR.x - 0.75, -0.04, 1.55)))
    m.section('tack door')

    # The tack room's things: harness pegs with tack hung on them, a saddle on its rack, a grain bin, the lantern hook.
    pegs = kit.wall_space((TACK - T * 0.5, YI1), (TACK - T * 0.5, YI0), TACK_FLOOR)   # the partition's tack side
    town.box(m, (3.6, 0.03, 0.16), at=(4.0, -0.015, 1.6), look='C', space=pegs, drop=('+y',))
    for k in range(6):
        s = 2.4 + 0.6 * k
        town.box(m, (0.04, 0.16, 0.04), at=(s, -0.08, 1.62), look='C', space=pegs)
        if k % 2 == 0:
            town.lathe(m, [(0.16, -0.02), (0.19, 0.0), (0.16, 0.02), (0.13, 0.0)], at=(s, -0.06, 1.42), sides=10,
                       look='crepe', space=pegs, axis=Matrix.Rotation(math.radians(90.0), 4, 'X'))
        else:
            town.box(m, (0.05, 0.012, 0.7), at=(s + 0.03, -0.17, 1.27), rot=(0.0, 4.0, 0.0), look='crepe', space=pegs)
    rack_x, rack_y = -4.0, 3.0
    for dx in (-0.25, 0.25):
        for dy in (-0.18, 0.18):
            town.box(m, (0.05, 0.05, 0.78), at=(rack_x + dx, rack_y + dy, TACK_FLOOR + 0.39), look='C', drop=('-z',))
    town.box(m, (0.6, 0.12, 0.06), at=(rack_x, rack_y, TACK_FLOOR + 0.8), look='C')
    town.lathe(m, [(0.0, -0.02), (0.2, 0.0), (0.26, 0.12), (0.22, 0.2), (0.12, 0.26), (0.0, 0.27)],
               at=(rack_x, rack_y, TACK_FLOOR + 0.8), sides=8, look='woodoxide')
    town.box(m, (0.05, 0.42, 0.3), at=(rack_x - 0.24, rack_y, TACK_FLOOR + 0.78), look='woodoxide')
    town.box(m, (0.05, 0.42, 0.3), at=(rack_x + 0.24, rack_y, TACK_FLOOR + 0.78), look='woodoxide')
    m.hull((0.7, 0.5, 1.1), at=(rack_x, rack_y, TACK_FLOOR + 0.55))
    rr.crate(m, -6.6, 2.9, TACK_FLOOR, 1.0, 0.7, 0.75, yaw=0.0)
    town.box(m, (1.04, 0.74, 0.04), at=(-6.6, 2.9, TACK_FLOOR + 0.77), rot=(0.0, -3.0, 0.0), look='A')
    rr.lamp_bracket(m, (TACK - T * 0.5, -1.2), (-1.0, 0.0), TACK_FLOOR + 2.0, socket='Light_Tack')
    m.section('tack room things')

    # The bay: the hay heaped against the back wall, a stack of bales and a few loose, hay strewn, a pitchfork, a lantern.
    rr.hay_mound(m, 3.6, 2.3, 0.0, 3.0, 1.5, 2.2, rings=4, sides=12)
    rr.hay_mound(m, 1.8, 0.9, 0.0, 2.4, 1.3, 0.35, rings=2, sides=10, seed_turn=0.4)
    rr.hay_mound(m, 5.7, -1.4, 0.0, 1.2, 1.0, 0.18, rings=2, sides=8, seed_turn=1.1)
    for k, (x, y, z, yaw) in enumerate(((-1.4, 3.2, 0.0, 0.0), (-0.45, 3.2, 0.0, 2.0), (-0.95, 3.2, 0.36, -3.0),
                                        (-1.6, 1.6, 0.0, 33.0))):
        frame = Space(Matrix.Translation((x, y, 0.03 + z)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))
        m.box((0.95, 0.46, 0.36), at=(0.0, 0.0, 0.18), uv=Tile('Hay'), mat='hay', space=frame, drop=('-z',))
        for dx in (-0.25, 0.25):
            town.box(m, (0.012, 0.47, 0.37), at=(dx, 0.0, 0.18), look='A', space=frame, drop=('-z',))
    m.hull((1.9, 0.5, 0.72), at=(-0.95, 3.2, 0.39))
    m.hull_points([(3.6 + 3.0 * c * math.cos(a), 2.3 + 1.5 * c * math.sin(a), z) for a in (0.0, 1.57, 3.14, 4.71)
                   for c, z in ((1.0, 0.0), (0.55, 1.6))] + [(3.6, 2.3, 2.15)])
    fork = Space(Matrix.Translation((0.62, Y1 - 0.45, 0.03)) @ Matrix.Rotation(math.radians(12.0), 4, 'X'))
    town.board(m, (0.0, 0.0, 0.0), (0.0, 0.0, 1.5), 0.03, 0.03, look='C', space=fork)
    for dx in (-0.06, 0.0, 0.06):
        town.box(m, (0.01, 0.01, 0.28), at=(dx, -0.02, 1.62), look='iron', space=fork)
    town.box(m, (0.15, 0.015, 0.02), at=(0.0, 0.0, 1.5), look='iron', space=fork)
    rr.lamp_bracket(m, (BAY_POSTS[1] + 0.11, Y0 + 0.1), (1.0, 0.0), 2.25, socket='Light_Bay')
    m.socket('BaleStack', (2.5, -1.8, 0.03), (0.0, 0.0, 0.0))
    m.section('bay things')

    # Collision: the closed walls, the tack room's front round its door, the posts, the roof; the bay open.
    m.hull((X1 - X0, T, WALL_H), at=(0.0, Y1 - T * 0.5, WALL_H * 0.5))
    for x in (X0 + T * 0.5, X1 - T * 0.5):
        m.hull((T, Y1 - Y0, WALL_H), at=(x, 0.0, WALL_H * 0.5))
    for a, b in ((X0, TACK_DOOR[0]), (TACK_DOOR[1], TACK)):
        m.hull((b - a, T, WALL_H), at=((a + b) * 0.5, Y0 + T * 0.5, WALL_H * 0.5))
    m.hull((TACK_DOOR[1] - TACK_DOOR[0], T, WALL_H - 2.15), at=((TACK_DOOR[0] + TACK_DOOR[1]) * 0.5, Y0 + T * 0.5,
                                                               (WALL_H + 2.15) * 0.5))
    m.hull((T + 0.02, Y1 - Y0, WALL_H), at=(TACK, 0.0, WALL_H * 0.5))
    m.hull((TACK - X0, Y1 - Y0, TACK_FLOOR), at=((X0 + TACK) * 0.5, 0.0, TACK_FLOOR * 0.5))
    for x in BAY_POSTS[1:]:
        m.hull((0.22, 0.22, WALL_H), at=(x, Y0 + 0.1, WALL_H * 0.5))
    m.hull((X1 - TACK, 0.3, 0.3), at=((TACK + X1) * 0.5, Y0 + 0.1, WALL_H - 0.13))
    m.hull_points([(x, y, z) for x in (X0 - 0.35, X1 + 0.35) for y, z in (
        (Y0 - 0.6, WALL_H - 0.6 * tan + 0.15), (Y1 + 0.6, WALL_H - 0.6 * tan + 0.15), (0.0, ridge + 0.25))] +
        [(x, y, WALL_H - 0.3) for x in (X0 - 0.35, X1 + 0.35) for y in (Y0 + 0.2, Y1 - 0.2)])

    px, py = PIVOT
    m.rooms += [
        f'hay bay {X1 - TACK - 0.1:.1f} x {Y1 - Y0 - 0.2:.1f} m, open along its front between 4 posts '
        f'({WALL_H - 0.24:.2f} m under the plate), earth floor',
        f'tack room {TACK - XI0 - T * 0.5:.2f} x {YI1 - YI0:.2f} m inside, board floor +{TACK_FLOOR:.2f}; its door '
        f'{TACK_DOOR[1] - TACK_DOOR[0]:.2f} x 2.15 m at x {TACK_DOOR[0] - px:.2f}..{TACK_DOOR[1] - px:.2f} from the pivot',
    ]
    print(f'BARN: walls x {X0 - px:.2f}..{X1 - px:.2f}, y {Y0 - py:.2f}..{Y1 - py:.2f} from the pivot; ridge {ridge:.2f}',
          flush=True)
    m.captions = ["Amos's hay barn: a long, low roof, the hay bay", 'open along its front, the tack room at its end.']
    return m, m.finish(pivot=PIVOT, footprint=(X0, X1, Y0, Y1))


BUILT = [barn()]
models = [obj for _, obj in BUILT]


def at(x, y, z):
    return (x - PIVOT[0], y - PIVOT[1], z)


if lt.want_preview():
    (bm_, obj), = BUILT
    shots = {obj.name: []}
    shots[obj.name].append(rr.exterior(obj, 'RR_Barn_Whitlock', view=(-0.45, -1.0, 0.3)))
    shots[obj.name].append(rr.inside(obj, 'RR_Barn_Whitlock_Bay', at(6.5, -5.6, 1.55), at(0.5, 2.5, 1.0), lens=24.0))
    shots[obj.name].append(rr.inside(obj, 'RR_Barn_Whitlock_Tack', at(-3.2, 1.6, TACK_FLOOR + 1.45),
                                     at(-5.6, -3.6, TACK_FLOOR + 1.1), lens=22.0, lamp_energy=60.0))
    rr.write_report('BarnWhitlock', [bm_], shots)
