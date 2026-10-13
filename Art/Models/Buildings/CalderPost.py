"""Calder's Post, the Rim Rangers' outpost on Larkspur Bench (Docs/Areas/RansomsRest_Expansion.md, section 4 "The Last
Watch", section 6 "Calder's Post"): Ruth Calder's watchtower and cabin and the empty stable. Scripted models
(Art/README.md) on looter_mountain (looter_homes, looter_town, looter_buildings). Each faces south (-Y): the post looks
south over the valley and west over Gravewind Canyon. Ruth rode south after the Dunne Gang: nothing is lit, the
glass is dark, the stove cold.

  RR_Watchtower   a 4 x 4 m timber tower of board-and-batten on corner posts, 12 m to its deck: a door on the south
                  side, slits for light up the stair, and inside six flights (40 degrees, 0.8 m wide) climbing round the
                  walls by corner landings, a pole handrail on the open side of each, to a hatch in the deck. The deck
                  (4.8 m square, railed) has a pyramid roof of shakes on the corner posts; the watch fire's iron basket
                  on a bracket off its south-west corner, past the eaves (SOCKET_WatchFire); the heliograph on its tripod
                  (south side, SOCKET_Heliograph); Ruth's spyglass in its swivel mount on the west rail
                  (SOCKET_Spyglass). Pivot: the middle of its footprint on the ground; the deck's walking surface is 12.0 m
                  up (+62 m on the knoll's +50).
  RR_RangerCabin  Ruth's cabin: 8 x 6 m of hewn logs on a fieldstone footing under a shake roof, a log woodshed lean-to
                  on its west end, a plank door (standing open) between two front windows, a window each in the back and
                  the east end. Inside, on a board floor: her bunk, a cold box stove and its pipe, the table under the front
                  window with the places of the post's log (SOCKET_PostLog) and her journal (SOCKET_Journal) on its top,
                  a shelf with cans, the code book's place (SOCKET_CodeBook) and the oil's (SOCKET_LampOil) on it (the
                  books and the oil are props of their own: Art/Models/Props/MissionProps.py), the Rim map pinned on the east
                  wall: six pins down the Rim a month apart and a seventh, fresh, on Ransom's Rest (SOCKET_RimMap); the
                  footlocker's place at the bunk's foot (SOCKET_Footlocker) and the fourth cache's in the north-east
                  corner (SOCKET_Cache); the post's board stands at the door (SOCKET_NoticeBoard: the existing
                  SM_NoticeBoard, RIM RANGERS). Pivot: the middle of the log walls' footprint on the ground; the floor is
                  0.35 m up, stone steps at the door.
  RR_LeanTo       the empty lean-to stable (6 x 3.5 m, open to the south, two stalls, a manger, an empty saddle rack,
                  old hay) and the small corral before it (about 12 x 9 m of post-and-rail, its gate standing open on
                  the east side). Pivot: the middle of the stable's footprint on the ground; the corral runs 9 m south.
  --preview  renders the three in the area's late-afternoon light, the tower's deck and the cabin inside, into
             Saved/ArtPreviews/RansomsRest/Mountain/.
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_buildings as kit
import looter_homes as homes
import looter_mountain as mtn
import looter_ruins as lr
import looter_textures as lt
import looter_town as town
from looter_buildings import Opening, Space, Tile, Trim, place


def yaw_facing(fx, fy):
    return math.degrees(math.atan2(fx, -fy))


def unbuilt(m):
    """A stand-in for a part taken out of m because it's a mission prop of its own now (the books: MissionProps.py): it
    draws on m's random stream and registers m's material slots exactly as building into m did, but its faces are thrown
    away (discard), so everything after it in m comes out as it did before."""
    sink = mtn.Model('_Unbuilt', seed=0)
    sink.rng, sink.slots = m.rng, m.slots
    return sink


def discard(sink):
    sink.bm.free()
    bpy.data.objects.remove(sink._obj)
    bpy.data.meshes.remove(sink._mesh)


def board_wall(m, p0, p1, outline, holes=(), thick=0.05, z0=0.0, battens=True, every=0.4):
    """Board-and-batten seen from both sides (upright weathered boards, battens outside); holes: (x, z, w, h)."""
    space = kit.wall_space(p0, p1, z0)
    length = (Vector(p1) - Vector(p0)).length
    m.panel(outline, [kit.rect(*h) for h in holes], thick, Trim('A', world=True, rotate=True, u=m.rng.uniform(0.0, 6.4)),
            space=space, slices=0)
    if battens:
        keep = [(x - 0.06, x + w + 0.06, z - 0.04, z + h + 0.04) for x, z, w, h in holes]
        homes.battens(m, space, outline, 0.0, length, keep, look='A', z0=0.0, every=every)
    return space, length


# --- The watchtower ---

TW = 2.0                 # half the tower's width (walls' outer faces)
DECK = 12.0
ROOF_EAVE = 14.25
STAIR_W = 0.8


def watchtower():
    m = mtn.Model('RR_Watchtower', seed=7101)
    rng = m.rng
    wall_top = DECK - 0.36
    # Footing, floor, corner posts (they run on up to carry the roof).
    homes.plinth(m, -TW, TW, -TW, TW, 0.12, out=0.05)
    mtn.deck(m, -TW + 0.05, TW - 0.05, -TW + 0.05, TW - 0.05, 0.08)
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            mtn.post(m, sx * (TW - 0.17), sy * (TW - 0.17), -0.2, ROOF_EAVE, size=0.3)
    m.section('footing')
    # Walls: a door south, slits up the stair; each wall's hull round its openings.
    door = (1.4, 0.0, 1.0, 2.1)
    slits = {'south': [(1.8, 6.6, 0.32, 0.8)], 'east': [(0.6, 3.1, 0.32, 0.8), (2.8, 9.4, 0.32, 0.8)],
             'north': [(1.8, 5.2, 0.32, 0.8)], 'west': [(0.6, 7.6, 0.32, 0.8), (2.6, 1.6, 0.32, 0.8)]}
    corners = {'south': ((-TW, -TW), (TW, -TW)), 'east': ((TW, -TW), (TW, TW)), 'north': ((TW, TW), (-TW, TW)),
               'west': ((-TW, TW), (-TW, -TW))}
    for side, (p0, p1) in corners.items():
        L = 2.0 * TW
        if side == 'south':
            outline = [(0.0, 0.0), (door[0], 0.0), (door[0], door[3]), (door[0] + door[2], door[3]),
                       (door[0] + door[2], 0.0), (L, 0.0), (L, wall_top), (0.0, wall_top)]
        else:
            outline = [(0.0, 0.0), (L, 0.0), (L, wall_top), (0.0, wall_top)]
        space, _ = board_wall(m, p0, p1, outline, slits[side])
        for x, z, w, h in slits[side]:
            for xx in (x - 0.05, x + w + 0.05):
                town.trim_board(m, space, (xx, 0.0, z - 0.05), (xx, 0.0, z + h + 0.05), 0.1, 'C', thick=0.04)
        # Collision: the wall as two slabs round the door (south) or one.
        d = Vector(p1) - Vector(p0)
        n = Vector((d.y, -d.x, 0.0)).normalized() * 0.03
        if side == 'south':
            for a, b in ((0.0, door[0]), (door[0] + door[2], L)):
                q0, q1 = Vector(p0) + d.normalized() * a, Vector(p0) + d.normalized() * b
                mtn.hull_between(m, (q0.x, q0.y, 0.0), (q1.x, q1.y, 0.0), 0.12, wall_top)
            q0 = Vector(p0) + d.normalized() * door[0]
            q1 = Vector(p0) + d.normalized() * (door[0] + door[2])
            mtn.hull_between(m, (q0.x, q0.y, door[3]), (q1.x, q1.y, door[3]), 0.12, wall_top - door[3])
        else:
            mtn.hull_between(m, (p0[0], p0[1], 0.0), (p1[0], p1[1], 0.0), 0.12, wall_top)
    # The door's casing and its leaf standing open, swung in against the inside of the wall east of it.
    front = kit.wall_space((-TW, -TW), (TW, -TW), 0.0)
    for xx in (door[0] - 0.06, door[0] + door[2] + 0.06):
        town.trim_board(m, front, (xx, 0.0, 0.0), (xx, 0.0, door[3] + 0.1), 0.12, 'C', thick=0.05)
    town.trim_board(m, front, (door[0] - 0.14, 0.0, door[3] + 0.07), (door[0] + door[2] + 0.14, 0.0, door[3] + 0.07), 0.14,
                    'C', thick=0.06)
    hinge_x = -TW + door[0] + door[2]
    for k in range(5):
        x = hinge_x + 0.1 + 0.2 * k
        mtn.plank(m, (x, -TW + 0.1, 0.1), (x, -TW + 0.1, door[3] - 0.02), 0.19, 0.04, face=(0.0, 1.0, 0.0), look='A')
    m.section('walls')
    # The stair: six flights round the walls by corner landings, 2.0 m each.
    flights = []
    c, e = TW - 0.1 - STAIR_W * 0.5, TW - 0.1 - STAIR_W      # a flight's middle line; where landings end
    runs = [((c, -e), (c, e), (1.0, 0.0, 0.0)), ((e, c), (-e, c), (0.0, 1.0, 0.0)),
            ((-c, e), (-c, -e), (-1.0, 0.0, 0.0)), ((-e, -c), (e, -c), (0.0, -1.0, 0.0))]
    for k in range(6):
        (ax, ay), (bx, by), out = runs[k % 4]
        z0 = 2.0 * k if k else 0.08
        z1 = 2.0 * (k + 1)
        foot, head = Vector((ax, ay, z0)), Vector((bx, by, z1))
        across = Vector(out)
        lr.flight(m, foot, head, across, STAIR_W - 0.06, 10, seed=7110 + k, stringer=(0.06, 0.24))
        lr.flight_hull(m, foot, head, across, STAIR_W - 0.06)
        flights.append((foot, head, across))
        # The handrail on its open side (toward the shaft's middle): posts at its ends, a pole along it.
        inner = -across * (STAIR_W * 0.5 + 0.05)
        rise = (z1 - z0) / 10.0
        for p in (foot + inner + Vector((0.0, 0.0, rise)), head + inner):
            mtn.log(m, tuple(p - Vector((0.0, 0.0, 0.15))), tuple(p + Vector((0.0, 0.0, 1.0))), r=0.04, sides=6)
        mtn.log(m, tuple(foot + inner + Vector((0.0, 0.0, 1.0 + rise))), tuple(head + inner + Vector((0.0, 0.0, 1.0))),
                r=0.035, sides=6)
        lr.hull_along(m, foot + inner, head + inner, 1.05, thick=0.1)
    # Corner landings (the north-west one at the deck is the deck itself).
    spots ={2.0: (1, 1), 4.0: (-1, 1), 6.0: (-1, -1), 8.0: (1, -1), 10.0: (1, 1)}
    for z, (sx, sy) in spots.items():
        x0, x1 = (e, TW - 0.1) if sx > 0 else (-TW + 0.1, -e)
        y0, y1 = (e, TW - 0.1) if sy > 0 else (-TW + 0.1, -e)
        mtn.deck(m, x0, x1, y0, y1, z, thick=0.06)
        mtn.beam(m, (x0, (y0 + y1) * 0.5, z - 0.14), (x1, (y0 + y1) * 0.5, z - 0.14), 0.16, 0.12, face=(0.0, 0.0, -1.0))
        m.hull((x1 - x0, y1 - y0, 0.25), at=((x0 + x1) * 0.5, (y0 + y1) * 0.5, z - 0.125))
    m.section('stair')
    # The deck: joists out past the walls, planks round the hatch over the last flight, a railing round its edge and
    # round the hatch.
    D = 2.4
    hatch = (-e, e + 0.3, e, TW - 0.1)      # x0, x1, y0, y1: over flight 6 (along the north wall, going west)
    for x in (-1.6, -0.55, 0.55, 1.6):
        mtn.beam(m, (x, -D, DECK - 0.17), (x, D, DECK - 0.17), 0.22, 0.12, face=(1.0, 0.0, 0.0))
    for x0, x1, y0, y1 in ((-D, D, -D, hatch[2]), (-D, hatch[0], hatch[2], D), (hatch[1], D, hatch[2], D),
                           (hatch[0], hatch[1], hatch[3], D)):
        mtn.deck(m, x0, x1, y0, y1, DECK, thick=0.06)
        m.hull((x1 - x0, y1 - y0, 0.25), at=((x0 + x1) * 0.5, (y0 + y1) * 0.5, DECK - 0.125))
    rail = [((-D + 0.05, -D + 0.05), (D - 0.05, -D + 0.05)), ((D - 0.05, -D + 0.05), (D - 0.05, D - 0.05)),
            ((D - 0.05, D - 0.05), (-D + 0.05, D - 0.05)), ((-D + 0.05, D - 0.05), (-D + 0.05, -D + 0.05)),
            ((hatch[0], hatch[2] - 0.03), (hatch[1], hatch[2] - 0.03)), ((hatch[1] + 0.03, hatch[2]), (hatch[1] + 0.03, hatch[3]))]
    for (ax, ay), (bx, by) in rail:
        lr.railing(m, (ax, ay, DECK), (bx, by, DECK), height=1.05, post_every=1.2, mid=0.5, post=0.1, rail=(0.1, 0.05))
        lr.hull_along(m, (ax, ay, DECK), (bx, by, DECK), 1.1, thick=0.1)
    m.section('deck')
    # The roof: a pyramid of shakes on the corner posts (a plate round their tops), cap boards up the hips, a finial.
    R = TW + 0.75
    for (ax, ay), (bx, by) in (((-TW + 0.17, -TW + 0.17), (TW - 0.17, -TW + 0.17)), ((TW - 0.17, -TW + 0.17), (TW - 0.17, TW - 0.17)),
                               ((TW - 0.17, TW - 0.17), (-TW + 0.17, TW - 0.17)), ((-TW + 0.17, TW - 0.17), (-TW + 0.17, -TW + 0.17))):
        mtn.beam(m, (ax, ay, ROOF_EAVE - 0.1), (bx, by, ROOF_EAVE - 0.1), 0.2, 0.2, face=((ax + bx) * 0.5, (ay + by) * 0.5, 0.0))
    pitch = 36.0
    theta = math.radians(pitch)
    length = R / math.cos(theta)
    apex = Vector((0.0, 0.0, ROOF_EAVE + R * math.tan(theta)))
    for k in range(4):
        turn = Matrix.Rotation(math.radians(90.0 * k), 3, 'Z')
        origin = turn @ Vector((-R, -R, ROOF_EAVE))
        slope = kit.Slope(origin, turn @ Vector((1.0, 0.0, 0.0)), turn @ Vector((0.0, math.cos(theta), math.sin(theta))),
                          2.0 * R, length, 0.02)
        bounds = homes.hip_bounds(slope, theta)
        homes.hip_deck(m, slope, bounds, 0.06, length)
        homes.hip_shakes(m, slope, bounds, length - 0.05, piece=(2.0, 3.4))
        p0 = origin
        homes.cap_line(m, p0, apex, (turn @ Vector((-1.0, -1.0, 2.0))).normalized(), width=0.2, thick=0.05, look='C',
                       lift=0.05)
    town.lathe(m, [(0.0, 0.0), (0.09, 0.0), (0.06, 0.3), (0.1, 0.42), (0.0, 0.62)], at=tuple(apex), sides=8, look='C')
    m.hull_points([Vector((sx * R, sy * R, ROOF_EAVE)) for sx in (-1, 1) for sy in (-1, 1)] + [apex + Vector((0, 0, 0.2))])
    m.section('roof')
    # The watch fire: an iron basket on a bracket off the south-west corner, past the eaves, wood laid in it.
    corner = Vector((-D, -D, DECK))
    out = Vector((-1.0, -1.0, 0.0)).normalized()
    basket = corner + out * 1.05 + Vector((0.0, 0.0, 1.15))
    mtn.rod(m, tuple(corner + Vector((0.08, 0.08, 0.35))), tuple(basket - out * 0.3 - Vector((0.0, 0.0, 0.3))), r=0.035)
    mtn.rod(m, tuple(corner + Vector((0.08, 0.08, 1.05))), tuple(basket - out * 0.3), r=0.035)
    mtn.ring(m, [(0.33, -0.02), (0.36, -0.02), (0.36, 0.02), (0.33, 0.02), (0.33, -0.02)], tuple(basket + Vector((0, 0, 0.25))),
             (0.0, 0.0, 1.0), sides=12)
    mtn.ring(m, [(0.2, -0.02), (0.23, -0.02), (0.23, 0.02), (0.2, 0.02), (0.2, -0.02)], tuple(basket - Vector((0, 0, 0.2))),
             (0.0, 0.0, 1.0), sides=12)
    for k in range(10):
        a = 2.0 * math.pi * k / 10
        mtn.rod(m, tuple(basket + Vector((0.21 * math.cos(a), 0.21 * math.sin(a), -0.2))),
                tuple(basket + Vector((0.345 * math.cos(a), 0.345 * math.sin(a), 0.27))), r=0.012, sides=4)
    for k in range(5):
        a = math.radians(rng.uniform(0, 180))
        d = Vector((math.cos(a), math.sin(a), 0.0)) * 0.24
        mtn.log(m, tuple(basket - d + Vector((0, 0, -0.08 + 0.05 * k))), tuple(basket + d + Vector((0, 0, -0.05 + 0.05 * k))),
                r=0.045, sides=6)
    m.socket('WatchFire', tuple(basket))
    # The heliograph: a tripod, the mirror on its yoke, the sighting rod and its vane, a shutter of slats.
    hx, hy = 0.9, -1.55
    head = Vector((hx, hy, DECK + 1.3))
    for k in range(3):
        a = math.radians(90.0 + 120.0 * k)
        mtn.log(m, (hx + 0.45 * math.cos(a), hy + 0.45 * math.sin(a), DECK), tuple(head - Vector((0, 0, 0.05))), r=0.022, sides=5)
    mtn.iron_box(m, (0.06, 0.06, 0.12), at=tuple(head))
    mtn.ring(m, [(0.0, -0.012), (0.13, -0.012), (0.13, 0.012), (0.0, 0.012)], tuple(head + Vector((0.0, 0.0, 0.2))),
             (0.25, -1.0, 0.15), sides=12, look='brass')
    homes.quad(m, [tuple(head + Vector((x, -0.02, 0.2 + z))) for x, z in ((-0.09, -0.09), (0.09, -0.09), (0.09, 0.09), (-0.09, 0.09))],
               Trim('H4', fit=True), facing=(0.25, -1.0, 0.15))
    mtn.rod(m, tuple(head + Vector((0.0, -0.05, 0.05))), tuple(head + Vector((0.12, -0.75, 0.12))), r=0.01)
    mtn.iron_box(m, (0.06, 0.008, 0.08), at=tuple(head + Vector((0.12, -0.75, 0.16))))
    m.socket('Heliograph', (hx, hy + 0.55, DECK), (0.0, 0.0, yaw_facing(0.0, -1.0)))
    # The spyglass on its swivel post at the west rail, pointing out over the canyon.
    sx, sy = -D + 0.25, 0.4
    mtn.post(m, sx, sy, DECK, DECK + 1.15, size=0.1)
    mtn.iron_box(m, (0.06, 0.12, 0.08), at=(sx, sy, DECK + 1.2))
    town.cylinder(m, (sx + 0.3, sy, DECK + 1.27), (sx - 0.35, sy, DECK + 1.3), 0.03, 0.042, sides=8, look='brass',
                  face=(0.0, 0.0, 1.0))
    town.cylinder(m, (sx + 0.3, sy, DECK + 1.27), (sx + 0.42, sy, DECK + 1.265), 0.025, sides=8, look='black',
                  face=(0.0, 0.0, 1.0))
    m.socket('Spyglass', (sx + 0.7, sy, DECK), (0.0, 0.0, yaw_facing(-1.0, 0.0)))
    mtn.lantern(m, (TW - 0.17, -TW + 0.17 - 0.2, DECK + 1.9), name='Light')
    mtn.lantern(m, (-TW + 0.3, TW - 0.4, 5.8), name='Light_2')
    m.socket('Door', (-TW + door[0] + door[2] * 0.5, -TW - 0.5, 0.0), (0.0, 0.0, 0.0))
    m.section('fittings')
    return m.finish(view=(-0.7, -1.0, 0.5))


# --- Ruth's cabin ---

CX0, CX1, CY0, CY1 = -4.0, 4.0, -3.0, 3.0
CFLOOR = 0.35
COURSE = 0.3
CWALL = 2.7                     # the log walls' height over the floor


def log_wall(m, p0, p1, z0, z1, gaps, ext=0.25, r=0.155, seed=0, offset=0.0):
    """Courses of hewn round logs from p0 to p1 (their ends running past the corners by ext), from z0 to z1, broken
    round the openings in gaps ((a, b, z_lo, z_hi) along the wall)."""
    d = (Vector(p1) - Vector(p0)).normalized()
    out = Vector((d.y, -d.x, 0.0))
    length = (Vector(p1) - Vector(p0)).length
    z = z0 + offset
    while z < z1 - 0.05:
        spans = [(-ext, length + ext)]
        for a, b, lo, hi in gaps:
            if lo - 0.05 < z < hi + 0.05:
                spans = town.subtract(spans, (a, b))
        for a, b in spans:
            q0, q1 = Vector(p0) + d * a, Vector(p0) + d * b
            mtn.log(m, (q0.x, q0.y, z + r), (q1.x, q1.y, z + r), r=r, out=tuple(out), sides=8)
        z += COURSE


def ranger_cabin():
    m = mtn.Model('RR_RangerCabin', seed=7201)
    rng = m.rng
    homes.plinth(m, CX0, CX1, CY0, CY1, CFLOOR, out=0.1)
    mtn.deck(m, CX0 + 0.15, CX1 - 0.15, CY0 + 0.15, CY1 - 0.15, CFLOOR)
    m.hull((CX1 - CX0, CY1 - CY0, CFLOOR + 0.35), at=(0.0, 0.0, (CFLOOR - 0.35) * 0.5))
    m.section('floor')
    top = CFLOOR + CWALL
    DOOR = (5.0, 6.0)                  # along the front wall (x from CX0): 1.0 m wide, 2.0 m high
    WIN = 1.0, 1.0                    # windows' width and height (sill 1.0 over the floor)
    front_w = [(1.2, 2.2), (6.6, 7.6)]
    gaps_front = [(DOOR[0], DOOR[1], CFLOOR - 0.1, CFLOOR + 2.0)] + [(a, b, CFLOOR + 0.95, CFLOOR + 2.0) for a, b in front_w]
    gaps_back = [(5.4, 6.4, CFLOOR + 0.95, CFLOOR + 2.0)]                # the back wall runs east to west
    gaps_east = [(2.5, 3.5, CFLOOR + 0.95, CFLOOR + 2.0)]
    log_wall(m, (CX0, CY0), (CX1, CY0), CFLOOR - 0.05, top, gaps_front)
    log_wall(m, (CX1, CY0), (CX1, CY1), CFLOOR - 0.05, top, gaps_east, offset=COURSE * 0.5)
    log_wall(m, (CX1, CY1), (CX0, CY1), CFLOOR - 0.05, top, gaps_back)
    log_wall(m, (CX0, CY1), (CX0, CY0), CFLOOR - 0.05, top, [], offset=COURSE * 0.5)
    # The gables: boards over the end walls up to the roof.
    pitch = 34.0
    rise = (CY1 - CY0) * 0.5 * math.tan(math.radians(pitch))
    for p0, p1 in (((CX1, CY0), (CX1, CY1)), ((CX0, CY1), (CX0, CY0))):
        space = kit.wall_space(p0, p1, top)
        m.panel([(0.0, 0.0), (6.0, 0.0), (3.0, rise)], (), 0.05, Trim('A', world=True, rotate=True), space=space,
                slices=0)
    # Windows (dark glass, four panes) in their log openings, and the door's frame with the door standing open.
    for (a, b), p0, p1 in [((1.2, 2.2), (CX0, CY0), (CX1, CY0)), ((6.6, 7.6), (CX0, CY0), (CX1, CY0)),
                           ((5.4, 6.4), (CX1, CY1), (CX0, CY1)), ((2.5, 3.5), (CX1, CY0), (CX1, CY1))]:
        space = kit.wall_space(p0, p1, CFLOOR)
        o = Opening('window', a, 0.98, b - a, 1.0, panes=(2, 2), casing='C', casing_w=0.1)
        town.window(m, space, o, 0.2)
        m.box((b - a + 0.1, 0.32, 0.08), at=((a + b) * 0.5, 0.0, 0.98 - 0.04), uv='C', space=space)
    front = kit.wall_space((CX0, CY0), (CX1, CY0), CFLOOR)
    for x in (DOOR[0] - 0.06, DOOR[1] + 0.06):
        m.box((0.12, 0.34, 2.06), at=(x, 0.0, 1.0), uv='C', space=front)
    m.box((DOOR[1] - DOOR[0] + 0.24, 0.34, 0.14), at=((DOOR[0] + DOOR[1]) * 0.5, 0.0, 2.07), uv='C', space=front)
    hinge = Vector((CX0 + DOOR[1], CY0 + 0.2, CFLOOR))
    for k in range(5):
        x = hinge.x + 0.1 + 0.2 * k
        mtn.plank(m, (x, hinge.y + 0.05, CFLOOR + 0.02), (x, hinge.y + 0.05, CFLOOR + 1.96), 0.19, 0.04,
                  face=(0.0, 1.0, 0.0), look='A')
    for z in (0.35, 1.6):
        mtn.plank(m, (hinge.x + 0.05, hinge.y + 0.1, CFLOOR + z), (hinge.x + 1.0, hinge.y + 0.1, CFLOOR + z), 0.14, 0.03,
                  face=(0.0, 1.0, 0.0), look='A')
    homes.stone_steps(m, CX0 + (DOOR[0] + DOOR[1]) * 0.5, 1.3, CY0 - 0.1, CFLOOR, 2)
    # Walls' collision (logs): round the door and windows only as slabs (the windows stay shut to the player).
    for (a, b) in ((CX0 - 0.25, CX0 + DOOR[0]), (CX0 + DOOR[1], CX1 + 0.25)):
        m.hull((b - a, 0.32, top), at=((a + b) * 0.5, CY0, top * 0.5))
    m.hull((DOOR[1] - DOOR[0], 0.32, top - CFLOOR - 2.0), at=(CX0 + (DOOR[0] + DOOR[1]) * 0.5, CY0, (top + CFLOOR + 2.0) * 0.5))
    m.hull((8.5, 0.32, top), at=(0.0, CY1, top * 0.5))
    for x in (CX0, CX1):
        m.hull((0.32, 6.5, top), at=(x, 0.0, top * 0.5))
    m.section('walls')
    # The roof: shakes on a board deck, rafters showing inside, a ridge beam; the stovepipe through it.
    front_s, back_s = kit.gable(m, CX0 - 0.15, CX1 + 0.15, CY0, CY1, top, pitch, overhang=0.45, rake=0.35, deck=0.06,
                                sag=0.04)
    pipe = (1.6, 2.0)
    skip = lambda s: (lambda x, y: (Vector(s.world((x, y, 0.0)).xy) - Vector(pipe)).length < 0.3)
    kit.shingles(m, front_s, piece=(2.0, 3.4), skip=skip(front_s))
    kit.shingles(m, back_s, piece=(2.0, 3.4), skip=skip(back_s))
    kit.ridge_cap(m, front_s, back_s, uv='C', width=0.2)
    for slope in (front_s, back_s):
        x = 0.45
        while x < slope.width - 0.3:
            m.board((x, 0.35, -0.12), (x, slope.length - 0.15, -0.12), 0.12, 0.07, face=(1.0, 0.0, 0.0), uv='C', space=slope,
                    cuts=0)
            x += 0.95
    mtn.beam(m, (CX0, 0.0, top + rise - 0.15), (CX1, 0.0, top + rise - 0.15), 0.2, 0.18, face=(0.0, -1.0, 0.0))
    homes.gable_hull(m, CX0 - 0.15, CX1 + 0.15, CY0, CY1, top, top, 0.0, top + rise, overhang=0.45, rake=0.35,
                     pitch_front=pitch)
    roof_z = top + (CY1 - pipe[1]) * math.tan(math.radians(pitch))
    homes.stovepipe(m, pipe[0], pipe[1], CFLOOR + 1.0, roof_z + 0.9, radius=0.08, socket='Smoke')
    m.section('roof')
    # The woodshed lean-to on the west end: a log back post line, a plank roof sloping out, firewood stacked in.
    lx0, lx1 = CX0 - 2.0, CX0 - 0.2
    for y in (CY0 + 0.3, 0.0, CY1 - 0.3):
        mtn.post(m, lx0 + 0.1, y, -0.3, 1.9, size=0.16)
    roof =kit.Slope(Vector((lx0 - 0.25, CY1 + 0.2, 1.95)), Vector((0.0, -1.0, 0.0)),
                     Vector((2.05, 0.0, top - 0.3 - 1.95)).normalized(), CY1 - CY0 + 0.4,
                     (Vector((2.05, 0.0, top - 0.3 - 1.95))).length + 0.25, 0.03)
    homes.board_roof(m, roof, every=0.26, look='A', missing=(7,))
    mtn.beam(m, (lx0 + 0.1, CY0 + 0.1, 1.95), (lx0 + 0.1, CY1 - 0.1, 1.95), 0.16, 0.16, face=(-1.0, 0.0, 0.0))
    homes.woodpile(m, CX0 - 0.35, CY0 + 0.5, CY1 - 0.6, rows=5, depth=0.45, outward=-1.0, axis='x')
    m.hull((2.0, 6.4, 0.3), at=((lx0 + CX0) * 0.5, 0.0, (1.95 + top) * 0.5), rot=(0.0, -math.degrees(math.atan2(top - 2.25, 2.0)), 0.0))
    m.section('lean-to')
    # Inside: the bunk on the west wall, the stove, the table and benches, the shelf, the map, a peg rail.
    F = CFLOOR
    bx0, bx1, by0, by1 = CX0 + 0.2, CX0 + 1.1, -0.4, 1.7
    for x in (bx0 + 0.05, bx1 - 0.05):
        for y in (by0 + 0.05, by1 - 0.05):
            mtn.post(m, x, y, F, F + 0.55, size=0.09)
    for x in (bx0 + 0.05, bx1 - 0.05):
        mtn.beam(m, (x, by0, F + 0.42), (x, by1, F + 0.42), 0.14, 0.08, face=(-1.0 if x < 0 else 1.0, 0.0, 0.0))
    town.box(m, (bx1 - bx0 - 0.1, by1 - by0 - 0.1, 0.12), at=((bx0 + bx1) * 0.5, (by0 + by1) * 0.5, F + 0.55), look='canvas',
             bevel=0.03)
    town.box(m, (bx1 - bx0 - 0.08, 0.9, 0.06), at=((bx0 + bx1) * 0.5, by0 + 0.6, F + 0.63), rot=(0, 0, 3.0), look='crepe')
    m.hull((bx1 - bx0, by1 - by0, 0.62), at=((bx0 + bx1) * 0.5, (by0 + by1) * 0.5, F + 0.31))
    m.socket('Footlocker', ((bx0 + bx1) * 0.5 + 0.1, by0 - 0.55, F), (0.0, 0.0, yaw_facing(1.0, 0.0)))
    # The stove: an iron box on legs, its door, the pipe up through the roof.
    sx, sy = pipe
    town.box(m, (0.7, 0.5, 0.5), at=(sx, sy, F + 0.45), look='iron', bevel=0.02)
    for dx in (-0.3, 0.3):
        for dy in (-0.2, 0.2):
            mtn.rod(m, (sx + dx, sy + dy, F), (sx + dx, sy + dy, F + 0.2), r=0.02, sides=5)
    mtn.iron_box(m, (0.26, 0.02, 0.2), at=(sx, sy - 0.26, F + 0.47))
    m.hull((0.75, 0.55, 0.72), at=(sx, sy, F + 0.36))
    # The table under the front window (east), two benches, the post's log open, her journal, a lamp.
    tx, ty = CX0 + 7.1, CY0 + 0.95
    town.box(m, (1.4, 0.8, 0.05), at=(tx, ty, F + 0.76), look='C')
    for dx in (-0.6, 0.6):
        for dy in (-0.32, 0.32):
            mtn.post(m, tx + dx, ty + dy, F, F + 0.74, size=0.07)
    for dy in (-0.62, 0.62):
        town.box(m, (1.2, 0.26, 0.04), at=(tx, ty + dy, F + 0.45), look='C')
        for dx in (-0.48, 0.48):
            town.box(m, (0.05, 0.22, 0.43), at=(tx + dx, ty + dy, F + 0.215), look='C')
    m.hull((1.4, 0.8, 0.8), at=(tx, ty, F + 0.4))
    # The post's log and her journal are their own props now (SM_PostLog, SM_Journal: Art/Models/Props/MissionProps.py),
    # put on these sockets: on the table top, each turned as the baked books were. Their old books are left unbuilt.
    sink = unbuilt(m)
    mtn.book(sink, (tx - 0.2, ty + 0.05, F + 0.785), size=(0.22, 0.3, 0.03), turn=-6.0, cover='oxide', open_=True)
    m.socket('PostLog', (tx - 0.2, ty + 0.05, F + 0.785), (0.0, 0.0, yaw_facing(0.0, 1.0) - 6.0))
    mtn.book(sink, (tx + 0.38, ty + 0.12, F + 0.785), size=(0.14, 0.2, 0.03), turn=20.0, cover='black')
    m.socket('Journal', (tx + 0.38, ty + 0.12, F + 0.785), (0.0, 0.0, yaw_facing(0.0, 1.0) + 20.0))
    town.cylinder(m, (tx + 0.5, ty - 0.2, F + 0.785), (tx + 0.5, ty - 0.2, F + 0.86), 0.06, 0.05, sides=8, look='brass',
                  face=(0.0, -1.0, 0.0))
    town.cylinder(m, (tx + 0.5, ty - 0.2, F + 0.86), (tx + 0.5, ty - 0.2, F + 1.06), 0.04, 0.035, sides=8, look='glow',
                  face=(0.0, -1.0, 0.0))
    m.socket('Light', (tx + 0.5, ty - 0.2, F + 0.98))
    # The shelf on the back wall: cans, the code book; the oil's place.
    shx, shy = 0.2, CY1 - 0.32
    town.box(m, (1.6, 0.26, 0.04), at=(shx, shy, F + 1.45), look='C')
    for x in (shx - 0.7, shx + 0.7):
        town.box(m, (0.04, 0.2, 0.18), at=(x, shy + 0.02, F + 1.36), look='C')
    for k, x in enumerate((-0.55, -0.4, -0.25)):
        town.cylinder(m, (shx + x, shy, F + 1.47), (shx + x, shy, F + 1.6 + 0.02 * k), 0.045, sides=8, look='iron',
                      face=(0.0, -1.0, 0.0))
    # The code book is its own prop now (SM_CodeBook), lying on the shelf at this socket.
    mtn.book(sink, (shx + 0.25, shy, F + 1.47), size=(0.16, 0.22, 0.035), turn=90.0, cover='black')
    discard(sink)
    m.socket('CodeBook', (shx + 0.25, shy - 0.01, F + 1.47), (0.0, 0.0, yaw_facing(0.0, -1.0) - 6.0))
    m.socket('LampOil', (shx + 0.6, shy, F + 1.47), (0.0, 0.0, yaw_facing(0.0, -1.0)))
    # The Rim map on the east wall: a sheet pinned up, the Rim's edge inked down it, six old pins and a fresh one.
    mx, my, mz = CX1 - 0.17, -1.6, F + 1.25
    mtn.paper(m, (mx, my - 0.6, mz), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0), 1.2, 0.85)
    rim = [(0.1, 0.78), (0.28, 0.66), (0.35, 0.5), (0.52, 0.42), (0.6, 0.27), (0.78, 0.18), (1.05, 0.08)]
    for (a0, b0), (a1, b1) in zip(rim, rim[1:]):
        town.board(m, (mx - 0.003, my - 0.6 + a0, mz + b0), (mx - 0.003, my - 0.6 + a1, mz + b1), 0.012, 0.002, look='black',
                   face=(-1.0, 0.0, 0.0))
    for k, (a, b) in enumerate(rim[:6]):
        town.box(m, (0.02, 0.018, 0.018), at=(mx - 0.012, my - 0.6 + a + 0.03, mz + b + 0.03), look='brass')
    town.box(m, (0.025, 0.024, 0.024), at=(mx - 0.014, my - 0.6 + 0.92, mz + 0.22), look='oxide')
    m.socket('RimMap', (mx - 0.45, my, mz + 0.4), (0.0, 0.0, yaw_facing(-1.0, 0.0)))
    m.socket('Cache', (CX1 - 0.65, CY1 - 0.55, F), (0.0, 0.0, yaw_facing(-1.0, -1.0)))
    m.socket('NoticeBoard', (CX0 + DOOR[1] + 0.9, CY0 - 0.9, 0.0), (0.0, 0.0, 0.0))
    m.socket('Door', (CX0 + (DOOR[0] + DOOR[1]) * 0.5, CY0 - 0.9, 0.0), (0.0, 0.0, 0.0))
    # A peg rail by the door with her old slicker gone, one peg left with a hat.
    town.box(m, (1.0, 0.06, 0.08), at=(CX0 + 3.8, CY0 + 0.2, F + 1.7), look='C')
    m.section('inside')
    return m.finish(view=(-0.8, -1.0, 0.45))


# --- The lean-to stable and the corral ---

def lean_to():
    m = mtn.Model('RR_LeanTo', seed=7301)
    rng = m.rng
    X0, X1, Y0, Y1 = -3.0, 3.0, -1.75, 1.75
    back_h, front_h = 2.9, 2.15
    # Posts, the back wall of boards, the ends half boarded; a plank roof sloping to the front.
    for x in (X0, -1.0, 1.0, X1):
        mtn.post(m, x, Y0 + 0.1, -0.4, front_h, size=0.16)
        mtn.post(m, x, Y1 - 0.1, -0.4, back_h, size=0.16)
        m.hull((0.16, 0.16, front_h + 0.4), at=(x, Y0 + 0.1, (front_h - 0.4) * 0.5))
    board_wall(m, (X1, Y1), (X0, Y1), [(0.0, 0.0), (6.0, 0.0), (6.0, back_h), (0.0, back_h)])
    for p0, p1 in (((X0, Y1), (X0, Y0 + 1.2)), ((X1, Y0 + 1.2), (X1, Y1))):
        h0, h1 = (back_h, front_h + (back_h - front_h) * 0.35) if p0[1] > p1[1] else (front_h + (back_h - front_h) * 0.35, back_h)
        L = abs(p1[1] - p0[1])
        board_wall(m, p0, p1, [(0.0, 0.0), (L, 0.0), (L, h1), (0.0, h0)])
    m.hull((6.2, 0.15, back_h), at=(0.0, Y1, back_h * 0.5))
    for x in (X0, X1):
        m.hull((0.15, Y1 - Y0 - 1.2, back_h), at=(x, (Y0 + 1.2 + Y1) * 0.5, back_h * 0.5))
    for y, z in ((Y0 + 0.1, front_h), (Y1 - 0.1, back_h)):
        mtn.beam(m, (X0 - 0.2, y, z + 0.08), (X1 + 0.2, y, z + 0.08), 0.16, 0.16)
    roof = kit.Slope(Vector((X0 - 0.3, Y0 - 0.35, front_h + 0.16 - 0.35 * (back_h - front_h) / (Y1 - Y0 - 0.2))),
                     Vector((1.0, 0.0, 0.0)), Vector((0.0, Y1 - Y0 - 0.2, back_h - front_h)).normalized(), X1 - X0 + 0.6,
                     math.hypot(Y1 - Y0 + 0.2, back_h - front_h) + 0.2, 0.04)
    homes.board_roof(m, roof, every=0.26, look='A', missing=(17,))
    homes.slope_hull(m, roof)
    # Two stalls: a plank partition, a manger along the back, the saddle rack, old hay.
    for z in (0.35, 0.75, 1.15):
        mtn.plank(m, (0.0, Y0 + 0.6, z), (0.0, Y1 - 0.2, z), 0.22, 0.05, face=(1.0, 0.0, 0.0), look='A')
    mtn.post(m, 0.0, Y0 + 0.55, -0.3, 1.3, size=0.12)
    m.hull((0.1, Y1 - Y0 - 0.6, 1.3), at=(0.0, (Y0 + 0.6 + Y1 - 0.2) * 0.5, 0.65))
    for x0, x1 in ((X0 + 0.2, -0.15), (0.15, X1 - 0.2)):
        mtn.plank(m, (x0, Y1 - 0.55, 0.85), (x1, Y1 - 0.55, 0.85), 0.32, 0.05, face=(0.0, -1.0, 0.0), look='A')
        mtn.plank(m, (x0, Y1 - 0.38, 0.7), (x1, Y1 - 0.38, 0.7), 0.3, 0.05, face=(0.0, 0.0, 1.0), look='A')
        m.hull((x1 - x0, 0.4, 1.0), at=((x0 + x1) * 0.5, Y1 - 0.38, 0.5))
    for k in range(5):
        town.box(m, (rng.uniform(0.3, 0.6), rng.uniform(0.25, 0.5), rng.uniform(0.04, 0.1)),
                 at=(rng.uniform(X0 + 0.4, X1 - 0.4), rng.uniform(Y0 + 0.4, Y1 - 0.6), 0.02),
                 rot=(0, 0, rng.uniform(0, 180)), look='hay')
    rx = X1 - 0.6
    mtn.beam(m, (rx, Y0 + 0.6, 0.95), (rx, Y0 + 1.5, 0.95), 0.12, 0.12, face=(1.0, 0.0, 0.0))
    for y in (Y0 + 0.65, Y0 + 1.45):
        for s in (-1.0, 1.0):
            mtn.beam(m, (rx + s * 0.25, y, 0.0), (rx, y, 0.95), 0.06, 0.06, face=(1.0, 0.0, 0.0))
    # The corral before it: posts every 2.4 m, two peeled rails, its gate standing open on the east side.
    C0, C1, CS = -6.0, 6.0, -10.5
    # From the stable's front corners out and round; the east side's second bay is the gateway.
    line = [(X0, Y0), (C0, Y0), (C0, CS), (C1, CS), (C1, Y0), (X1, Y0)]
    gateway = None
    for (ax, ay), (bx, by) in zip(line, line[1:]):
        L = math.hypot(bx - ax, by - ay)
        n = max(1, int(math.ceil(L / 2.4)))
        d = Vector(((bx - ax) / L, (by - ay) / L, 0.0))
        pts = [Vector((ax, ay, 0.0)) + d * (L * k / n) for k in range(n + 1)]
        for p in pts:
            if (p.x, p.y) not in ((X0, Y0), (X1, Y0)):        # the stable's own corner posts stand there
                mtn.post(m, p.x, p.y, -0.5, 1.45, size=0.14, turn=rng.uniform(-6, 6))
        for k, (p, q) in enumerate(zip(pts, pts[1:])):
            if ax == C1 and bx == C1 and k == n - 2:
                gateway = (q, p)                              # its hinge at the post nearer the stable
                continue
            for z in (0.55, 1.2):
                mtn.log(m, (p.x, p.y, z + rng.uniform(-0.04, 0.04)), (q.x, q.y, z + rng.uniform(-0.04, 0.04)), r=0.06,
                        sides=6, caps=(False, False))
            mtn.hull_between(m, (p.x, p.y, 0.0), (q.x, q.y, 0.0), 0.12, 1.4)
    # The gate: three rails and a brace, swung 75 degrees out (east) on its hinge post.
    hinge, latch = gateway
    shut = (latch - hinge).normalized()
    a = math.radians(75.0)
    d = Vector((shut.x * math.cos(a) - shut.y * math.sin(a), shut.x * math.sin(a) + shut.y * math.cos(a), 0.0))
    if d.x < 0.0:
        d = Vector((shut.x * math.cos(-a) - shut.y * math.sin(-a), shut.x * math.sin(-a) + shut.y * math.cos(-a), 0.0))
    span = (latch - hinge).length - 0.15
    for z in (0.35, 0.8, 1.25):
        mtn.log(m, tuple(hinge + d * 0.08 + Vector((0, 0, z))), tuple(hinge + d * span + Vector((0, 0, z))), r=0.05, sides=6)
    mtn.log(m, tuple(hinge + d * 0.1 + Vector((0, 0, 1.25))), tuple(hinge + d * (span - 0.05) + Vector((0, 0, 0.35))), r=0.04,
            sides=6)
    mtn.log(m, tuple(hinge + d * span + Vector((0, 0, 0.25))), tuple(hinge + d * span + Vector((0, 0, 1.35))), r=0.05, sides=6)
    m.socket('Corral', (0.0, (Y0 + CS) * 0.5, 0.0))
    m.socket('Gate', tuple((hinge + latch) * 0.5), (0.0, 0.0, yaw_facing(1.0, 0.0)))
    return m.finish(view=(-0.8, -1.0, 0.5))


tower = watchtower()
cabin = ranger_cabin()
stable = lean_to()

if lt.want_preview():
    scene, sun = mtn.stage()
    human = homes.figure()
    human.location = (3.0, -3.0, 0.0)
    mtn.show_only([tower, human])
    mtn.shot(scene, sun, [tower], 'Watchtower', view=(-0.75, -1.0, 0.3), sun_dir=(30.0, 18.0), figure=human)
    human.location = (0.8, 0.2, DECK)
    mtn.show_only([tower, human])
    bpy.context.view_layer.update()
    homes.light(scene, sun, homes.golden_sun(35.0, 24.0))
    homes.shoot(scene, mtn.os.path.join(mtn.PREVIEW_DIR, 'Watchtower_Deck.png'), (3.9, -4.3, DECK + 1.45),
                (-1.2, 0.3, DECK + 0.7), 22.0, homes.SHOT, -0.15)
    cut = mtn.cut_copy(tower, z=7.0)
    mtn.show_only([cut])
    homes.light(scene, sun, homes.golden_sun(20.0, 60.0))
    homes.shoot(scene, mtn.os.path.join(mtn.PREVIEW_DIR, 'Watchtower_Stair.png'), (3.6, -5.2, 12.5), (0.0, 0.0, 3.0), 26.0,
                homes.SHOT, -0.1)
    bpy.data.objects.remove(cut)
    human.location = (5.2, -4.2, 0.0)
    mtn.show_only([cabin, human])
    mtn.shot(scene, sun, [cabin], 'RangerCabin', view=(-0.55, -1.0, 0.35), sun_dir=(35.0, 18.0), figure=human)
    cut = mtn.cut_copy(cabin, z=CFLOOR + 2.3)
    human.location = (1.5, -1.6, CFLOOR)
    mtn.show_only([cut, human])
    mtn.shot(scene, sun, [cut], 'RangerCabin_Inside', view=(0.3, -0.75, 1.15), sun_dir=(25.0, 60.0), figure=human, lens=30.0)
    bpy.data.objects.remove(cut)
    human.location = (1.0, -4.0, 0.0)
    mtn.show_only([stable, human])
    mtn.shot(scene, sun, [stable], 'LeanTo', view=(-0.6, -1.0, 0.45), sun_dir=(30.0, 20.0), figure=human)
mtn.save_reports('CalderPost')
