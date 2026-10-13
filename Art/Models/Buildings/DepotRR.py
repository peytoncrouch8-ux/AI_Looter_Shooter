"""The station house at Ransom's Rest, rebuilt larger with its rooms inside (Docs/Areas/RansomsRest_Expansion.md, sections
4 and 9: The Old Fare, Dead Lift's coal), and the depot's coal stage. It replaces Depot.py's SM_Depot on Ransom's Rest
(the platform kit, the water tower and the coffin shed stay Depot.py's).

  RR_Depot       a two-storey combination depot, 14 x 8 m, its floor at the platform's height (0.40 m): railroad-red
                 board and batten (WoodOxide) with cream trim, a shake roof, the platform canopy on knee brackets with
                 RANSOM'S REST hung on its fascia and lettered on both gables. Downstairs, walked into from the platform:
                 - the waiting room, 8.8 x 7.7 m inside, 3.4 m to its board ceiling: a wainscot under cream plaster,
                   slatted benches round the walls and a double one in the middle, a pot-bellied stove, the schedule
                   board, a clock stopped at twenty to nine, two hanging lamps; its double doors stand open onto the
                   platform and the dead have broken most of the panes in its four windows;
                 - the ticket office behind the iron grille (its ticket window, the talking window to come): the
                   ticket counter and its till, the ticket case, the agent's desk with the telegraph key and sounder,
                   his chair, a desk lamp;
                 - the baggage room onto the platform through a wide door slid open on its track, trunks, the skid
                   where Fenn's coffin goes (a prop of its own), and the LOST PROPERTY window: a counter under an iron
                   grille with a hatch, a painted sign over it, and behind it rifles and shotguns tagged in a rack and
                   tagged bundles on shelves;
                 - a stair up the waiting room's back wall, open at its foot and boxed in above, to a landing and the
                   station agent's closed door (Mr. Sloane is barricaded upstairs: his window is lamplit at dusk and his
                   stovepipe smokes).
  RR_CoalStage   the coal stage by the water tower: a timber bin of coal raised on posts, its chute and iron apron
                 over the track side, a ladder, a shovel and a sack under the chute, coal spilled on the ground.

Pivots follow the homes kit (looter_homes): the depot's is the middle of the waiting room's double doors on the front
wall's outer face, at the ground (z = 0: the platform's ground; the floor is 0.40 over it); its front faces the track
(-Y). Stand it with that wall on the platform's back edge (Depot.py: 1.65 + 4.0 m from the track's centreline). The coal
stage's pivot is the middle of its posts' front line on the ground, its chute toward the track (-Y).

Sockets: StationBoard, Light (the platform lamp over it), Smoke and Smoke_2, Crepe (over the doors), Sit_1 and Sit_2
(the platform benches), Speaker (= TicketWindow, as SM_Depot's ticket window was), and for the rooms TicketWindow,
AgentDoor, AgentDesk, BaggageCoffin, LostProperty (where the player stands at the hatch, facing it), LostPropertyCounter
(the hatch's counter, facing the room), Decal (a poster's spot on the front), Wait_1..4 (where the dead stand waiting for
a train, facing the platform doors) and the lamps Light_Waiting_1, Light_Waiting_2, Light_Ticket, Light_LostProperty,
Light_Stair and Light_Upstairs (behind the lamplit upper window). The coal stage: CoalStage (the chute's mouth, facing
the track) and Sack.

A scripted model (Art/README.md) built with looter_rr_town (on looter_homes, looter_town, looter_buildings).
  --preview   renders both in the area's late-afternoon light, the depot with its roof and upper floor off and from
              inside its rooms, into Saved/ArtPreviews/RansomsRest/Town/, and writes DepotRR_report.json
  --stats     prints where the triangles go; --no-ao skips the AO bake
"""
import math

import looter_buildings as kit
import looter_homes as homes
import looter_rr_town as rr
import looter_textures as lt
import looter_town as town
from looter_buildings import Matrix, Opening, Space, Tile, Trim, Vector

BASE = 0.40                                  # the platform's walking surface: the depot's floor
X0, X1, Y0, Y1 = -7.0, 7.0, -4.0, 4.0        # the walls' outer faces
T = 0.15
XI0, XI1, YI0, YI1 = X0 + T, X1 - T, Y0 + T, Y1 - T
P1 = (1.94, 2.06)                            # the partition between the waiting room and the east rooms (x)
P2 = (-0.06, 0.06)                           # between the baggage room and the ticket office (y)
CEIL = 3.4                                   # the ground storey's ceiling over its floor
UPPER = 3.6                                  # the upper floor's top over the ground floor
HALL_CEIL = 6.0                              # the stair hall's ceiling
WALL_H = 6.1                                 # floor to wall plate
EAVE = BASE + WALL_H
PITCH = 30.0
RISE = (Y1 - Y0) * 0.5 * math.tan(math.radians(PITCH))
RIDGE = EAVE + RISE
# The stair: up the waiting room's back wall toward the west, 19 risers to the upper floor.
STAIR_W = 1.15
SY0 = YI1 - STAIR_W                          # its open side (2.70)
RISERS, R_RISE, R_RUN = 19, UPPER / 19, 0.26
FOOT = -0.42                                 # the first riser's x
TOP_X = FOOT - (RISERS - 1) * R_RUN          # the landing's edge (-5.1)
OPEN_X = -1.7                                # the stairwell's east edge in the ceiling
ENCLOSE_X = -3.0                             # from here up the stair runs between walls
WALL_P = 0.12                                # partitions
AGENT = (-6.55, -5.45)                       # the agent's door along the upper partition
DOOR_MID = -2.5                              # the waiting room's double doors' middle: the pivot


def nose(x):
    """The stair's nose line over the floor at x (for railings, its spandrel and its enclosure)."""
    return (FOOT - x) / R_RUN * R_RISE + R_RISE


def notch(outline, items):
    doors = sorted((o for o in items if o.kind == 'door' and o.z <= 1e-4), key=lambda o: o.x)
    points = [outline[0]]
    for o in doors:
        points += [(o.x, 0.0), (o.x, o.h), (o.x + o.w, o.h), (o.x + o.w, 0.0)]
    return points + list(outline[1:])


WAINSCOT = 0.85            # the outer walls' wainscot of upright boards, under a cream cap rail


def outer_wall(m, p0, p1, items, gable, keep=()):
    """One outer wall, both storeys: the shell of red drop siding (WoodOxide, its boards level) with its reveals, a
    wainscot of upright boards 2.5 cm proud under a cream cap rail, cream corner boards, a water table, a belt course at
    the upper floor and a frieze under the eaves (not on the gables)."""
    space = kit.wall_space(p0, p1, BASE)
    length = (Vector(p1) - Vector(p0)).length
    outline = [(0.0, 0.0), (length, 0.0), (length, WALL_H)]
    if gable:
        outline.append((length * 0.5, WALL_H + RISE - 0.02))
    outline.append((0.0, WALL_H))
    homes.shell(m, notch(outline, items), kit.holes(items), T, homes.Boards(rotate=False), space, around=items,
                mat='woodoxide', slices=0)
    spans = homes.door_spans(items, length, margin=0.15)
    for a, b in spans:
        wain = Space(space.matrix @ Matrix.Translation((0.0, -0.025, 0.0)))
        town.panel(m, [(a, -0.05), (b, -0.05), (b, WAINSCOT), (a, WAINSCOT)], (), 0.025, homes.Boards(rotate=True),
                   mat='woodoxide', space=wain, slices=2.4, back=False)
        town.trim_board(m, space, (a, 0.0, WAINSCOT), (b, 0.0, WAINSCOT), 0.09, 'band_G', thick=0.05)
    homes.corners(m, space, length, WALL_H - 0.02, WALL_H - 0.02, look='band_G', width=0.15, wrap=not gable,
                  out=0.025)
    homes.band(m, space, spans, 0.1, 0.2, look='band_G', out=0.025)
    belt = [(0.0, length)]
    for a, b, z0, z1 in keep:
        if z0 < UPPER - 0.1 < z1:
            belt = town.subtract(belt, (a, b))
    homes.band(m, space, belt, UPPER - 0.12, 0.22, look='band_G')
    if not gable:
        homes.band(m, space, [(0.12, length - 0.12)], WALL_H - 0.14, 0.24, look='band_G')
    return space, length


def broken_window(m, space, o, broken):
    rr.room_window(m, space, o, T, paint='band_G', sash='band_G', broken=broken)


def sign_board(m, space, cx, cz, w, h, body, text_h, frame='band_G', look='cream', tol=0.045):
    """A painted name board: dark boards framed in cream, its lettering."""
    rows = max(2, int(round(h / 0.2)))
    for k in range(rows):
        z = cz - h * 0.5 + (h / rows) * (k + 0.5)
        town.board(m, (cx - w * 0.5, -0.05, z), (cx + w * 0.5, -0.05, z), h / rows - 0.004, 0.035, look='C', space=space)
    for z in (cz - h * 0.5 - 0.03, cz + h * 0.5 + 0.03):
        town.board(m, (cx - w * 0.5 - 0.06, -0.06, z), (cx + w * 0.5 + 0.06, -0.06, z), 0.07, 0.05, look=frame,
                   space=space)
    for x in (cx - w * 0.5 - 0.03, cx + w * 0.5 + 0.03):
        town.board(m, (x, -0.06, cz - h * 0.5), (x, -0.06, cz + h * 0.5), 0.07, 0.05, look=frame, space=space)
    town.text(m, body, (cx, -0.0675, cz), height=text_h, width=w - 0.3, space=space, look=look, tol=tol)


def shakes(m, slope, piece=(1.6, 3.0), start=-0.08, missing=0.02, skip=None):
    """Depot.py's shakes: courses two of the texture's rows deep (0.4 m), each standing proud of the one below."""
    rng = m.rng
    course, butt = 0.4, 0.03
    top = slope.length + 0.02
    y, k = start, 0
    while y < top - 0.04:
        h = min(course, top - y)
        x = -0.05 + rng.uniform(-0.3, 0.0)
        while x < slope.width + 0.05 - 1e-3:
            w = rng.uniform(*piece)
            if slope.width + 0.05 - (x + w) < 0.6:
                w = slope.width + 0.05 - x
            x_lo = max(x, -0.05)
            spans = [(x_lo, x + w)]
            if k >= 1 and w > 0.9 and rng.random() < missing * 4.0:
                gap = rng.uniform(0.2, 0.4)
                at = rng.uniform(x_lo + 0.3, x + w - 0.3 - gap)
                spans = [(x_lo, at), (at + gap, x + w)]
            for a, b in spans:
                cx, cy = (a + b) * 0.5, y + h * 0.5
                if skip is not None and skip(cx, cy):
                    continue
                tilt = -math.degrees(math.atan2(butt, h))
                matrix = kit.place((cx, cy + rng.uniform(-0.012, 0.012), butt * 0.5 + 0.004 * (k % 2)),
                                   (tilt, rng.uniform(-0.4, 0.4), 0.0))
                m.box((b - a, h, butt), matrix=matrix, uv=Trim('E', lane=2 * (k % 2)), space=slope, ao_floor=kit.ROOF_AO,
                      drop=('-z', '+y'))
            x += w
        y += course - 0.02
        k += 1


def skip_round(slope, boxes):
    def skip(x, y):
        w = slope.world((x, y, 0.0))
        return any(abs(w.x - cx) < hx and abs(w.y - cy) < hy for cx, cy, hx, hy in boxes)
    return skip


# --- The depot ---

def depot():
    m = rr.Building('RR_Depot', seed=711, lived=True)
    rng = m.rng
    UPZ = UPPER + 0.75                       # the upper windows' sills
    win = dict(panes=(2, 2), casing_w=0.11)
    W_F1 = Opening('window', 1.4, 1.0, 1.0, 1.5, **win)
    DOOR = Opening('door', DOOR_MID - 0.7 - X0, 0.0, 1.4, 2.6)
    W_F2 = Opening('window', 7.2, 1.0, 0.9, 1.5, **win)
    BAG = Opening('door', 10.0, 0.0, 1.9, 2.5)
    dark = dict(style='sash', paint='band_G', sash='band_G', panes=(2, 2))
    U_F = [Opening('window', 1.75, UPZ, 0.9, 1.3, **dark), Opening('window', 6.05, UPZ, 0.9, 1.3, **dark),
           Opening('window', 10.45, UPZ, 0.9, 1.3, **dict(dark, lit=True))]
    W_E1 = Opening('window', 5.4, 1.0, 0.9, 1.4, **win)
    W_B1 = Opening('window', 2.2, 1.0, 0.9, 1.4, **win)
    BACK = Opening('door', 5.5, 0.0, 1.2, 2.2)
    U_B1 = Opening('window', 10.5, UPZ, 0.9, 1.3, **win)
    U_B = [Opening('window', 4.2, UPZ, 0.9, 1.3, **dark)]
    W_W1 = Opening('window', 2.6, 1.0, 0.9, 1.5, **win)
    W_W2 = Opening('window', 5.5, 1.0, 0.9, 1.5, **win)
    slid = (9.85, 14.1, -1.0, 2.8)           # where the baggage door's leaf lies slid open (no battens behind it)
    front, fl = outer_wall(m, (X0, Y0), (X1, Y0), [W_F1, DOOR, W_F2, BAG] + U_F, False, keep=[slid])
    east, el = outer_wall(m, (X1, Y0), (X1, Y1), [W_E1], True)
    back, bl = outer_wall(m, (X1, Y1), (X0, Y1), [W_B1, BACK, U_B1] + U_B, False)
    west, wl = outer_wall(m, (X0, Y1), (X0, Y0), [W_W1, W_W2], True)
    homes.plinth(m, X0, X1, Y0, Y1, BASE)
    m.section('outer walls')

    # The windows: downstairs the waiting room's, most panes broken, the ticket office's whole; upstairs the closed rooms'
    # (dark, one lamplit) and the stair hall's, seen from inside.
    broken_window(m, front, W_F1, (0, 3))
    broken_window(m, front, W_F2, (1, 2))
    broken_window(m, west, W_W1, (0, 1, 3))
    broken_window(m, west, W_W2, (2,))
    rr.room_window(m, east, W_E1, T, paint='band_G', sash='band_G')
    rr.room_window(m, back, W_B1, T, paint='band_G', sash='band_G')
    rr.room_window(m, back, U_B1, T, paint='band_G', sash='band_G')
    for space, items in ((front, U_F), (back, U_B)):
        homes.openings(m, space, items, T)
    for p0, p1, o in (((X0, Y0), (X1, Y0), W_F1), ((X0, Y0), (X1, Y0), W_F2), ((X0, Y1), (X0, Y0), W_W1),
                      ((X0, Y1), (X0, Y0), W_W2), ((X1, Y0), (X1, Y1), W_E1), ((X1, Y1), (X0, Y1), W_B1),
                      ((X1, Y1), (X0, Y1), U_B1)):
        rr.window_inside(m, p0, p1, BASE, T, o, look='C')
    m.section('windows')

    # The waiting room's double doors, standing open into the room against its front wall; a glazed transom over them.
    rr.casing(m, front, DOOR, 'band_G', cw=0.14, sill=False)
    rr.window_inside(m, (X0, Y0), (X1, Y0), BASE, T, DOOR, look='C', stool=False)
    town.box(m, (DOOR.w, 0.06, 0.06), at=(DOOR.x + DOOR.w * 0.5, T * 0.5, 2.18), look='C', space=front)
    m.box((DOOR.w, 0.008, 0.36), at=(DOOR.x + DOOR.w * 0.5, T * 0.5, 2.39), uv=Trim('H4', fit=True), space=front,
          drop=('-x', '+x', '-z', '+z'))
    rr.reveal_floor(m, front, DOOR, T)
    dx0, dx1 = X0 + DOOR.x, X0 + DOOR.x + DOOR.w
    rr.panel_leaf(m, rr.hinged((dx0 + 0.03, YI0 + 0.05, BASE), 174.0), 0.7, 2.13, look='woodoxide', glass=True)
    rr.panel_leaf(m, rr.hinged((dx1 - 0.03, YI0 + 0.05, BASE), -174.0), 0.7, 2.13, look='woodoxide', glass=True,
                  hinge_left=False)
    town.crepe_swag(m, front, DOOR.x - 0.25, DOOR.x + DOOR.w + 0.25, DOOR.h + 0.42, sag=0.16, y=-0.12)
    m.socket('Crepe', front.world((DOOR.x + DOOR.w * 0.5, -0.08, DOOR.h + 0.25)))
    m.section('front doors')

    # The baggage door: its leaf slid open east along an iron track, BAGGAGE over the doorway.
    rr.casing(m, front, BAG, 'band_G', cw=0.14, sill=False)
    rr.reveal_floor(m, front, BAG, T)
    rr.window_inside(m, (X0, Y0), (X1, Y0), BASE, T, BAG, look='C', stool=False)
    town.box(m, (4.2, 0.04, 0.06), at=(BAG.x + 2.0, -0.09, 2.72), look='iron', space=front)
    for x in (BAG.x - 0.05, BAG.x + 1.9, BAG.x + 3.9):
        town.box(m, (0.05, 0.08, 0.14), at=(x, -0.05, 2.72), look='iron', space=front)
    leaf = Space(front.matrix @ Matrix.Translation((BAG.x + BAG.w - 0.05, -0.1, 0.03)))
    m.box((2.0, 0.04, 2.6), at=(1.0, 0.0, 1.3), uv=homes.Boards(rotate=True), mat='woodoxide', space=leaf,
          drop=('+y',))
    for z in (0.25, 1.3, 2.35):
        town.board(m, (0.05, -0.035, z), (1.95, -0.035, z), 0.16, 0.035, look='band_G', space=leaf, drop=('+y',))
    for a, b in (((0.12, 0.35), (1.88, 1.2)), ((0.12, 1.42), (1.88, 2.25))):
        town.board(m, (a[0], -0.035, a[1]), (b[0], -0.035, b[1]), 0.12, 0.03, look='band_G', space=leaf, drop=('+y',))
    for x in (0.3, 1.7):
        town.box(m, (0.08, 0.05, 0.16), at=(x, 0.0, 2.68), look='iron', space=leaf)
    sign_board(m, front, BAG.x + BAG.w * 0.5, 2.98, 1.7, 0.36, 'BAGGAGE', 0.2)
    m.section('baggage door')

    # The back door, shut and barred inside.
    rr.casing(m, back, BACK, 'band_G', cw=0.14, sill=False)
    rr.window_inside(m, (X1, Y1), (X0, Y1), BASE, T, BACK, look='C', stool=False)
    rr.plank_leaf(m, Space(back.matrix @ Matrix.Translation((BACK.x + 0.01, T * 0.45, 0.0))), BACK.w - 0.02, 2.18,
                  look='A', battens_on=1.0)
    for x in (BACK.x - 0.12, BACK.x + BACK.w + 0.12):
        town.box(m, (0.06, 0.1, 0.12), at=(x, T + 0.05, 1.1), look='iron', space=back)
    town.box(m, (BACK.w + 0.4, 0.07, 0.1), at=(BACK.x + BACK.w * 0.5, T + 0.08, 1.1), look='C', space=back)
    m.section('back door')

    # --- Inside: the waiting room (a wainscot under cream plaster, a chair rail), the baggage room (bare boards), the
    # ticket office (as the waiting room); ceilings of boards; floors of planks. ---
    WAIN = 0.9
    planks_up = homes.Boards(rotate=True)

    def papered(a, b, holes, s_range=None, top=CEIL):
        rr.wall_cells(m, a, b, BASE, 0.0, WAIN, holes, planks_up, 'planks', s_range=s_range)
        rr.wall_cells(m, a, b, BASE, WAIN, top, holes, rr.plaster(), 'trim', s_range=s_range)
        length = (Vector(b) - Vector(a)).length
        lo, hi = s_range or (0.0, length)
        spans = [(lo, hi)]
        for h in holes:
            if h[2] < WAIN + 0.05:
                spans = town.subtract(spans, (h[0] - 0.08, h[1] + 0.08))
        space = kit.wall_space(a, b, BASE)
        for s0, s1 in spans:
            town.trim_board(m, space, (s0, 0.0, WAIN), (s1, 0.0, WAIN), 0.08, 'woodoxide', thick=0.025)
        return space

    wf = papered((P1[0], YI0), (XI0, YI0), [(0.84, 1.74, 1.0, 2.5), (P1[0] - dx1, P1[0] - dx0, 0.0, 2.6),
                                            (6.54, 7.54, 1.0, 2.5)])
    ww = papered((XI0, YI0), (XI0, YI1), [(1.45, 2.35, 1.0, 2.5), (4.35, 5.25, 1.0, 2.5)], s_range=(0.0, SY0 - YI0))
    wb = papered((FOOT, YI1), (P1[0], YI1), [(0.3 - FOOT, 1.5 - FOOT, 0.0, 2.2)])
    rr.polygon(m, [(FOOT, YI1, BASE), (FOOT, YI1, BASE + CEIL), (-4.6, YI1, BASE + CEIL),
                   (-4.6, YI1, BASE + nose(-4.6) - 0.2)], rr.plaster(BASE), facing=(0.0, -1.0, 0.0))
    wp = papered((P1[0], YI1), (P1[0], YI0), [(YI1 - 2.25, YI1 - 1.15, 0.95, 2.0), (YI1 + 2.3, YI1 + 3.5, 0.0, 2.15)])
    boards = Trim('A', world=True, rotate=True)
    rr.wall_cells(m, (XI1, YI0), (P1[1], YI0), BASE, 0.0, CEIL, [(XI1 - X0 - BAG.x - BAG.w, XI1 - X0 - BAG.x, 0.0, 2.5)],
                  boards)
    rr.wall_cells(m, (XI1, P2[0]), (XI1, YI0), BASE, 0.0, CEIL, [], boards)
    rr.wall_cells(m, (P1[1], P2[0]), (XI1, P2[0]), BASE, 0.0, CEIL, [(0.39, 1.49, 0.0, 2.15)], boards)
    rr.wall_cells(m, (P1[1], YI0), (P1[1], P2[0]), BASE, 0.0, CEIL, [(0.35, 1.55, 0.0, 2.15)], boards)
    papered((XI1, P2[1]), (P1[1], P2[1]), [(3.3, 4.4, 0.0, 2.15)])
    papered((XI1, YI1), (XI1, P2[1]), [(1.55, 2.45, 1.0, 2.4)])
    papered((P1[1], YI1), (XI1, YI1), [(1.84, 2.74, 1.0, 2.4)])
    tp = papered((P1[1], P2[1]), (P1[1], YI1), [(1.09, 2.19, 0.95, 2.0)])
    m.section('room walls')
    ceiling = Tile('WoodPlanks', along=None)
    rr.flat_cells(m, XI0, P1[0], YI0, YI1, BASE + CEIL, holes=[(XI0, OPEN_X, SY0, YI1)], uv=ceiling, up=False)
    rr.flat_cells(m, P1[1], XI1, YI0, P2[0], BASE + CEIL, uv=ceiling, up=False)
    rr.flat_cells(m, P1[1], XI1, P2[1], YI1, BASE + CEIL, uv=ceiling, up=False)
    for x0, x1, y0, y1 in ((XI0, P1[0], YI0, YI1), (P1[1], XI1, YI0, P2[0]), (P1[1], XI1, P2[1], YI1)):
        rr.flat_cells(m, x0, x1, y0, y1, BASE, uv=Tile('WoodPlanks', along=None), up=True)
    # The beams across the waiting room's ceiling.
    for x in (-4.5, -1.2):
        town.board(m, (x, YI0, BASE + CEIL - 0.08), (x, SY0 if x < OPEN_X else YI1, BASE + CEIL - 0.08), 0.16, 0.14,
                   look='C', face=(0.0, 0.0, -1.0), drop=('+y',))
    m.section('floors, ceilings')

    # The doorways through the partitions: linings, casings both sides, thresholds; the leaves standing open.
    rr.doorway_trim(m, (P1[0], YI1), (P1[0], YI0), BASE, WALL_P, Opening('door', YI1 + 2.3, 0.0, 1.2, 2.15), 'C')
    rr.lining(m, (P1[0], -2.3), (P1[0], -3.5), (1.0, 0.0), WALL_P, BASE, BASE + 2.15, look='C')
    rr.plank_leaf(m, rr.hinged((P1[1] + 0.03, -3.48, BASE), 0.0), 0.98, 2.1, look='A', straps=False, battens_on=1.0)
    rr.doorway_trim(m, (P1[1], P2[0]), (XI1, P2[0]), BASE, WALL_P, Opening('door', 0.39, 0.0, 1.1, 2.15), 'C')
    rr.lining(m, (2.45, P2[0]), (3.55, P2[0]), (0.0, 1.0), WALL_P, BASE, BASE + 2.15, look='C')
    rr.panel_leaf(m, rr.hinged((2.47, P2[1] + 0.03, BASE), 62.0), 1.06, 2.1, look='woodoxide', glass=False)
    m.section('inner doors')

    # The ticket window: an opening through the partition with a counter ledge both sides, an iron grille, TICKETS
    # over it on the waiting room's side.
    gy0, gy1, gz0, gz1 = 1.15, 2.25, 0.95, 2.0
    rr.lining(m, (P1[0], gy1), (P1[0], gy0), (1.0, 0.0), WALL_P, BASE + gz0, BASE + gz1, look='C')
    town.box(m, (0.32, gy1 - gy0 + 0.1, 0.04), at=(P1[0] - 0.1, (gy0 + gy1) * 0.5, BASE + gz0 + 0.02), look='C')
    rr.casing(m, wp, Opening('window', YI1 - gy1, gz0, gy1 - gy0, gz1 - gz0), 'C', cw=0.1, sill=False)
    rr.casing(m, tp, Opening('window', gy0 - P2[1], gz0, gy1 - gy0, gz1 - gz0), 'C', cw=0.1, sill=False)
    for k in range(6):
        y = gy0 + (gy1 - gy0) * (k + 0.5) / 6
        town.box(m, (0.018, 0.018, gz1 - gz0 - 0.1), at=(2.0, y, BASE + (gz0 + gz1) * 0.5 + 0.04), look='iron',
                 drop=('-z', '+z'))
    for z in (gz0 + 0.24, gz1 - 0.05):
        town.box(m, (0.02, gy1 - gy0, 0.025), at=(2.0, (gy0 + gy1) * 0.5, BASE + z), look='iron')
    town.box(m, (0.04, 0.34, 0.012), at=(2.0, (gy0 + gy1) * 0.5, BASE + gz0 + 0.12), look='brass')
    sign = Space(wp.matrix @ Matrix.Translation((0.0, -0.02, 0.0)))
    sign_board(m, sign, YI1 - (gy0 + gy1) * 0.5, gz1 + 0.27, 1.0, 0.22, 'TICKETS', 0.13, frame='C')
    m.socket('TicketWindow', (P1[0] - 0.06, (gy0 + gy1) * 0.5, BASE + 1.45), rr.rot_to((-1.0, 0.0)))
    m.socket('Speaker', (P1[0] - 0.06, (gy0 + gy1) * 0.5, BASE + 1.45), rr.rot_to((-1.0, 0.0)))
    m.section('ticket window')

    # --- The stair up the back wall: open at its foot with a railing, boxed in above; the spandrel boarded under it. ---
    rr.flight(m, (FOOT, (SY0 + YI1) * 0.5), (-1.0, 0.0), STAIR_W - 0.02, RISERS, R_RISE, R_RUN, BASE, look='A',
              top_tread=False)
    sp = [(-0.5, 0.0), (-0.5, nose(-0.5) - 0.05), (-4.6, nose(-4.6) - 0.05), (-4.6, CEIL), (XI0, CEIL), (XI0, 0.0)]
    rr.polygon(m, [(x, SY0 - 0.015, BASE + z) for x, z in sp], boards, facing=(0.0, -1.0, 0.0))
    enc = [(ENCLOSE_X, nose(ENCLOSE_X) + 0.12), (ENCLOSE_X, CEIL), (-4.6, CEIL), (-4.6, nose(-4.6) + 0.12)]
    rr.polygon(m, [(x, SY0 - WALL_P, BASE + z) for x, z in enc], rr.plaster(BASE), facing=(0.0, -1.0, 0.0))
    rr.polygon(m, [(x, SY0, BASE + z) for x, z in enc], rr.plaster(BASE), facing=(0.0, 1.0, 0.0))
    town.board(m, (ENCLOSE_X + 0.03, SY0 - WALL_P * 0.5, BASE + nose(ENCLOSE_X)),
               (ENCLOSE_X + 0.03, SY0 - WALL_P * 0.5, BASE + CEIL), WALL_P + 0.02, 0.06, look='C', face=(1.0, 0.0, 0.0))
    rr.handrail(m, (FOOT + 0.03, SY0 + 0.03, BASE + R_RISE), (ENCLOSE_X + 0.1, SY0 + 0.03, BASE + nose(ENCLOSE_X + 0.1)),
                height=0.9, every=0.2)
    m.section('stair')

    # The stair hall upstairs: the landing, the hall's walls and ceiling, the agent's closed door with its lamp.
    rr.flat_cells(m, XI0, TOP_X, SY0, YI1, BASE + UPPER, uv=Tile('WoodPlanks', along=None), up=True)
    town.board(m, (TOP_X, SY0, BASE + UPPER - 0.03), (TOP_X, YI1, BASE + UPPER - 0.03), 0.06, 0.04, look='C',
               face=(0.0, 0.0, 1.0))
    rr.flat_cells(m, XI0, OPEN_X, SY0, YI1, BASE + HALL_CEIL, uv=Tile('WoodPlanks', along=None), up=False)
    hall_win = (X0 + bl - U_B1.x - U_B1.w - XI0, X0 + bl - U_B1.x - XI0, U_B1.z, U_B1.z + U_B1.h)
    rr.wall_cells(m, (XI0, YI1), (OPEN_X, YI1), BASE, CEIL, HALL_CEIL, [hall_win], rr.plaster(CEIL))
    rr.wall_cells(m, (XI0, SY0), (XI0, YI1), BASE, UPPER, HALL_CEIL, [], rr.plaster(UPPER))
    rr.wall_cells(m, (OPEN_X, YI1), (OPEN_X, SY0), BASE, CEIL, HALL_CEIL, [], rr.plaster(CEIL))
    hall = rr.wall_cells(m, (OPEN_X, SY0), (XI0, SY0), BASE, CEIL, HALL_CEIL,
                         [(OPEN_X - AGENT[1], OPEN_X - AGENT[0], UPPER, UPPER + 2.1)], rr.plaster(CEIL))
    agent = Opening('door', OPEN_X - AGENT[1], UPPER, AGENT[1] - AGENT[0], 2.1)
    rr.casing(m, hall, agent, 'C', cw=0.11, sill=False)
    rr.panel_leaf(m, Space(hall.matrix @ Matrix.Translation((agent.x, WALL_P * 0.5, UPPER))), agent.w, 2.08,
                  look='woodoxide', glass=False)
    rr.lamp_bracket(m, (XI0, YI1 - 0.4), (1.0, 0.0), BASE + UPPER + 1.9, socket='Light_Stair')
    m.socket('AgentDoor', hall.world((agent.x + agent.w * 0.5, -0.06, UPPER + 1.55)), rr.rot_to((0.0, 1.0)))
    m.section('stair hall')

    # --- The waiting room's things: benches, the stove, the schedule board and the clock, two lamps, bills. ---
    west_frame = Space(Matrix.Translation((XI0, 0.0, 0.0)) @ Matrix.Rotation(math.radians(-90.0), 4, 'Z'))
    rr.bench(m, -0.6, 3.2, 0.0, BASE, 1.0, space=west_frame)
    rr.bench(m, -6.45, -4.15, YI0, BASE, 1.0)
    rr.bench(m, -1.45, 0.75, -1.03, BASE, -1.0, hull=False)
    rr.bench(m, -1.45, 0.75, -0.97, BASE, 1.0, hull=False)
    m.hull((2.2, 1.1, 0.98), at=(-0.35, -1.0, BASE + 0.49))
    rr.stove(m, (-2.6, 0.6, BASE), BASE + CEIL)
    town.box(m, (0.3, 0.24, 0.26), at=(-2.05, 0.9, BASE + 0.13), rot=(0.0, 0.0, 12.0), look='iron', drop=('-z',))
    rr.lamp_hanging(m, (-4.4, 0.2, BASE + 2.55), BASE + CEIL, socket='Light_Waiting_1')
    rr.lamp_hanging(m, (-0.3, 1.25, BASE + 2.55), BASE + CEIL, socket='Light_Waiting_2')
    # The schedule board on the partition: TRAINS in cream over chalked lines.
    s0, s1, z0, z1 = YI1 - 0.5, YI1 + 1.7, 1.3, 2.4
    for k in range(5):
        z = z0 + (z1 - z0) * (k + 0.5) / 5
        town.board(m, (s0, -0.02, z), (s1, -0.02, z), (z1 - z0) / 5 - 0.004, 0.03, look='woodblack', space=wp)
    for z in (z0 - 0.03, z1 + 0.03):
        town.board(m, (s0 - 0.05, -0.04, z), (s1 + 0.05, -0.04, z), 0.06, 0.04, look='C', space=wp)
    for x in (s0 - 0.02, s1 + 0.02):
        town.board(m, (x, -0.04, z0), (x, -0.04, z1), 0.06, 0.04, look='C', space=wp)
    town.text(m, 'TRAINS', ((s0 + s1) * 0.5, -0.0355, z1 - 0.13), height=0.13, space=wp, look='cream', tol=0.05)
    for row in range(5):
        z = z1 - 0.34 - row * 0.15
        x = s0 + 0.12
        for word in range(4):
            w = rng.uniform(0.12, 0.42) if word != 1 else rng.uniform(0.45, 0.7)
            town.box(m, (w, 0.004, 0.035), at=(x + w * 0.5, -0.036, z + rng.uniform(-0.008, 0.008)), look='cream',
                     space=wp, drop=('+y', '-x', '+x', '-z', '+z'))
            x += w + rng.uniform(0.08, 0.16)
            if x > s1 - 0.3:
                break
    # The clock over it, stopped at twenty to nine.
    cs, cz = (s0 + s1) * 0.5, 2.88
    town.lathe(m, [(0.23, 0.0), (0.23, 0.06), (0.2, 0.07)], at=(cs, -0.0, cz), sides=12, look='woodblack', space=wp,
               axis=Matrix.Rotation(math.radians(90.0), 4, 'X'))
    town.prism(m, [(cs + 0.19 * math.cos(2.0 * math.pi * k / 16), cz + 0.19 * math.sin(2.0 * math.pi * k / 16))
                   for k in range(16)], 0.004, look='cream', space=wp, y=-0.074)
    for k in range(12):
        a = 2.0 * math.pi * k / 12
        town.box(m, (0.012, 0.004, 0.035 if k % 3 else 0.05), at=(cs + 0.16 * math.sin(a), -0.077, cz + 0.16 * math.cos(a)),
                 rot=(0.0, math.degrees(a), 0.0), look='black', space=wp, drop=('+y', '-x', '+x', '-z', '+z'))
    for hand, length, width in (((8.0 + 40.0 / 60.0) * 30.0, 0.1, 0.018), (240.0, 0.15, 0.012)):
        a = math.radians(hand)
        town.box(m, (width, 0.004, length), at=(cs + length * 0.45 * math.sin(a), -0.08, cz + length * 0.45 * math.cos(a)),
                 rot=(0.0, math.degrees(a), 0.0), look='black', space=wp, drop=('+y', '-x', '+x', '-z', '+z'))
    # Two bills on the front wall by the doors, faded.
    for s, z, a, u in ((P1[0] + 4.2, 1.55, -4.0, 4.3), (P1[0] + 0.5, 1.6, 3.0, 5.5)):
        r = math.radians(a)
        rr.face_part(m, [wf.world((s + dx_ * math.cos(r) - dz_ * math.sin(r), -0.003, z + dx_ * math.sin(r) + dz_ * math.cos(r)))
                         for dx_, dz_ in ((-0.21, -0.28), (0.21, -0.28), (0.21, 0.28), (-0.21, 0.28))],
                     Trim('G', fit=True, u=u), facing=(0.0, 1.0, 0.0))
    for k, (x, y) in enumerate(((-4.7, -0.9), (-3.4, 2.0), (-0.9, 1.9), (0.9, -0.5))):
        m.socket(f'Wait_{k + 1}', (x, y, BASE), (0.0, 0.0, 0.0))
    m.section('waiting room things')

    # --- The ticket office: the counter under the grille and its till, the ticket case, the desk with the telegraph
    # key and sounder, a lamp, the agent's chair. ---
    town.box(m, (0.55, 1.9, 0.94), at=(P1[1] + 0.275, (gy0 + gy1) * 0.5, BASE + 0.47), look='planks', drop=('-z',))
    town.box(m, (0.62, 2.0, 0.04), at=(P1[1] + 0.29, (gy0 + gy1) * 0.5, BASE + 0.96), look='A')
    town.box(m, (0.3, 0.32, 0.15), at=(P1[1] + 0.3, 1.9, BASE + 1.055), look='woodblack')
    town.box(m, (0.02, 0.24, 0.06), at=(P1[1] + 0.455, 1.9, BASE + 1.03), look='brass')
    town.box(m, (0.26, 0.2, 0.04), at=(P1[1] + 0.3, 1.25, BASE + 1.0), rot=(0.0, 0.0, 8.0), look='band_G')
    m.hull((0.6, 1.95, 1.0), at=(P1[1] + 0.3, (gy0 + gy1) * 0.5, BASE + 0.5))
    case = Space(Matrix.Translation((P1[1], 3.2, BASE)) @ Matrix.Rotation(math.radians(90.0), 4, 'Z'))
    town.box(m, (0.9, 0.012, 0.86), at=(0.0, -0.006, 1.58), look='planks', space=case, drop=('+y',))
    for z in (1.1625, 1.9975):
        town.box(m, (0.9, 0.2, 0.025), at=(0.0, -0.1, z), look='C', space=case, drop=('+y',))
    for x in (-0.4375, 0.4375):
        town.box(m, (0.025, 0.2, 0.86), at=(x, -0.1, 1.58), look='planks', space=case, drop=('+y',))
    for k in range(1, 6):
        town.box(m, (0.012, 0.18, 0.8), at=(-0.45 + 0.9 * k / 6, -0.11, 1.58), look='A', space=case, drop=('+y', '-y'))
    for k in range(1, 4):
        town.box(m, (0.86, 0.18, 0.012), at=(0.0, -0.11, 1.15 + 0.86 * k / 4), look='A', space=case, drop=('+y', '-y'))
    for k in range(9):
        col, row = rng.randrange(6), rng.randrange(4)
        town.box(m, (0.1, 0.12, 0.05), at=(-0.45 + 0.15 * (col + 0.5), -0.13, 1.18 + 0.215 * row + 0.03), look='band_G',
                 space=case, drop=('+y',))
    dkx, dky = 4.45, YI1 - 0.34
    town.box(m, (1.4, 0.66, 0.04), at=(dkx, dky, BASE + 0.76), look='C')
    for sx in (-1.0, 1.0):
        town.box(m, (0.38, 0.6, 0.72), at=(dkx + sx * 0.5, dky, BASE + 0.36), look='C', drop=('-z',))
        for k in range(3):
            town.box(m, (0.32, 0.02, 0.2), at=(dkx + sx * 0.5, dky - 0.31, BASE + 0.13 + 0.235 * k), look='A')
            town.box(m, (0.06, 0.02, 0.02), at=(dkx + sx * 0.5, dky - 0.325, BASE + 0.13 + 0.235 * k), look='brass')
    town.box(m, (0.6, 0.02, 0.14), at=(dkx, dky - 0.31, BASE + 0.66), look='A')
    town.box(m, (0.16, 0.08, 0.02), at=(dkx - 0.25, dky - 0.12, BASE + 0.79), look='woodblack')
    town.box(m, (0.13, 0.016, 0.012), at=(dkx - 0.25, dky - 0.12, BASE + 0.815), rot=(0.0, 4.0, 0.0), look='brass')
    town.lathe(m, [(0.0, 0.0), (0.018, 0.0), (0.016, 0.02), (0.0, 0.022)], at=(dkx - 0.31, dky - 0.12, BASE + 0.82),
               sides=6, look='black')
    town.box(m, (0.18, 0.1, 0.02), at=(dkx + 0.05, dky, BASE + 0.79), look='woodblack')
    for x in (-0.05, 0.05):
        town.box(m, (0.025, 0.025, 0.08), at=(dkx + 0.05 + x, dky, BASE + 0.84), look='brass')
    town.box(m, (0.13, 0.03, 0.02), at=(dkx + 0.05, dky, BASE + 0.885), look='brass')
    town.box(m, (0.36, 0.26, 0.03), at=(dkx + 0.3, dky - 0.05, BASE + 0.795), rot=(0.0, 0.0, -6.0), look='band_G')
    rr.lamp_table(m, (dkx + 0.55, dky + 0.1, BASE + 0.78), socket='Light_Ticket')
    m.hull((1.4, 0.66, 0.78), at=(dkx, dky, BASE + 0.39))
    rr.chair(m, dkx - 0.1, dky - 0.85, BASE, yaw=165.0)
    m.socket('AgentDesk', (dkx, dky - 0.34, BASE + 0.62), (0.0, 0.0, 0.0))
    m.section('ticket office things')

    # --- The baggage room: the lost property window, trunks, the skid for Fenn's coffin. ---
    cx0, cx1 = 5.75, 6.25
    town.box(m, (cx1 - cx0, P2[0] - YI0, 1.0), at=((cx0 + cx1) * 0.5, (YI0 + P2[0]) * 0.5, BASE + 0.5), look='planks',
             drop=('-z',))
    town.box(m, (cx1 - cx0 + 0.1, P2[0] - YI0, 0.045), at=((cx0 + cx1) * 0.5 - 0.03, (YI0 + P2[0]) * 0.5, BASE + 1.02),
             look='C')
    hy0, hy1, hz0, hz1 = -2.4, -1.5, 1.045, 1.65
    sx = cx0 + 0.04
    y = YI0 + 0.08
    while y < P2[0] - 0.05:
        if not (hy0 - 0.02 < y < hy1 + 0.02):
            town.box(m, (0.018, 0.018, 2.45 - 1.045), at=(sx, y, BASE + (2.45 + 1.045) * 0.5), look='iron',
                     drop=('-z', '+z'))
        else:
            town.box(m, (0.018, 0.018, 2.45 - hz1), at=(sx, y, BASE + (2.45 + hz1) * 0.5), look='iron', drop=('-z', '+z'))
        y += 0.15
    for z in (1.08, hz1, 2.45):
        town.box(m, (0.03, P2[0] - YI0, 0.03), at=(sx, (YI0 + P2[0]) * 0.5, BASE + z), look='iron')
    for y in (hy0, hy1):
        town.box(m, (0.05, 0.05, hz1 - hz0), at=(sx, y, BASE + (hz0 + hz1) * 0.5), look='C')
    town.box(m, (0.06, P2[0] - YI0, 0.35), at=(sx - 0.02, (YI0 + P2[0]) * 0.5, BASE + 2.625), look='woodblack')
    sign = Space(Matrix.Translation((sx - 0.05, (hy0 + hy1) * 0.5, BASE + 2.625)) @
                 Matrix.Rotation(math.radians(-90.0), 4, 'Z'))
    town.text(m, 'LOST PROPERTY', (0.0, 0.0, 0.0), height=0.17, space=sign, look='cream', tol=0.045)
    # Behind it: rifles and shotguns stood in a rack on a ledge along the east wall, high enough to be seen over the
    # counter, each tagged; tagged bundles on two shelves over them.
    LEDGE = 0.9
    town.box(m, (0.34, P2[0] - YI0 - 0.1, 0.05), at=(XI1 - 0.17, (YI0 + P2[0]) * 0.5, BASE + LEDGE - 0.025), look='A')
    for y in (YI0 + 0.25, (YI0 + P2[0]) * 0.5, P2[0] - 0.25):
        town.box(m, (0.3, 0.05, LEDGE - 0.05), at=(XI1 - 0.17, y, BASE + (LEDGE - 0.05) * 0.5), look='planks', drop=('-z',))
    town.box(m, (0.08, P2[0] - YI0 - 0.2, 0.06), at=(XI1 - 0.28, (YI0 + P2[0]) * 0.5, BASE + LEDGE + 0.03), look='C')
    town.box(m, (0.06, P2[0] - YI0 - 0.2, 0.08), at=(XI1 - 0.04, (YI0 + P2[0]) * 0.5, BASE + LEDGE + 1.32), look='C')
    for k in range(7):
        gy = YI0 + 0.45 + k * 0.47
        lean = Space(Matrix.Translation((XI1 - 0.28, gy, BASE + LEDGE + 0.02)) @
                     Matrix.Rotation(math.radians(-90.0), 4, 'Z') @ Matrix.Rotation(math.radians(-11.0), 4, 'X'))
        rr.long_gun(m, lean, kind='shotgun' if k % 3 == 1 else 'rifle', look_wood='A' if k % 2 else 'woodoxide')
        rr.tag(m, (0.0, -0.04, 0.5), space=lean)
    for z in (2.42, 2.85):
        town.board(m, (XI1 - 0.18, YI0 + 0.04, BASE + z - 0.015), (XI1 - 0.18, P2[0] - 0.04, BASE + z - 0.015), 0.36,
                   0.03, look='A', face=(0.0, 0.0, 1.0))
        for y in (YI0 + 0.3, (YI0 + P2[0]) * 0.5, P2[0] - 0.3):
            town.board(m, (XI1 - 0.005, y, BASE + z - 0.05), (XI1 - 0.3, y, BASE + z - 0.05), 0.05, 0.025,
                       look='iron', face=(0.0, -1.0, 0.0))
    for k in range(7):
        y = YI0 + 0.35 + k * 0.5 + rng.uniform(-0.05, 0.05)
        z = BASE + (2.435 if k % 2 == 0 else 2.865)
        w, d, h = rng.uniform(0.2, 0.34), rng.uniform(0.18, 0.28), rng.uniform(0.12, 0.26)
        if k == 3:
            town.lathe(m, [(0.0, 0.0), (0.17, 0.0), (0.17, 0.012), (0.085, 0.03), (0.08, 0.13), (0.0, 0.12)],
                       at=(XI1 - 0.22, y, z), sides=8, look='crepe')
            continue
        m.box((d, w, h), at=(XI1 - 0.22, y, z + h * 0.5), rot=(0.0, 0.0, rng.uniform(-8.0, 8.0)),
              uv=Tile('Polymer') if k % 3 else Tile('WoodPlanks', along=None), mat='canvas' if k % 3 else 'woodoxide',
              drop=('-z',))
        rr.tag(m, (XI1 - 0.22 - d * 0.5 - 0.005, y, z + h - 0.02))
    # The agent's lamp on the counter inside the grille, beside the hatch: it lights the counter and the racks behind.
    rr.lamp_table(m, (6.02, hy1 + 0.28, BASE + 1.045), socket='Light_LostProperty')
    m.socket('LostProperty', (5.15, (hy0 + hy1) * 0.5, BASE), rr.rot_to((1.0, 0.0)))
    m.socket('LostPropertyCounter', (cx0 - 0.03, (hy0 + hy1) * 0.5, BASE + 1.05), rr.rot_to((-1.0, 0.0)))
    m.hull((XI1 - cx0 + 0.05, P2[0] - YI0, 2.8), at=((cx0 + XI1) * 0.5, (YI0 + P2[0]) * 0.5, BASE + 1.4))
    # Trunks along the partition, a hand truck by the door, the skid where Fenn's coffin comes to rest.
    rr.trunk(m, 2.45, -1.65, BASE, 0.85, 0.5, 0.56, yaw=90.0)
    rr.trunk(m, 2.43, -0.82, BASE, 0.72, 0.46, 0.5, yaw=93.0, look='woodblack')
    rr.trunk(m, 2.47, -1.68, BASE + 0.56, 0.6, 0.38, 0.34, yaw=84.0, look='woodblack', hull=False)
    m.hull((0.5, 0.6, 0.9), at=(2.45, -1.65, BASE + 0.45))
    for y in (-0.9, -0.42):
        town.board(m, (3.55, y, BASE + 0.05), (5.6, y, BASE + 0.05), 0.1, 0.1, look='C', face=(0.0, 0.0, 1.0))
    m.socket('BaggageCoffin', (4.58, -0.66, BASE + 0.1), (0.0, 0.0, 0.0))
    truck = Space(Matrix.Translation((5.3, YI0 + 0.32, BASE)) @ Matrix.Rotation(math.radians(14.0), 4, 'X'))
    for x in (-0.18, 0.18):
        town.board(m, (x, 0.12, 0.05), (x, 0.0, 1.3), 0.04, 0.04, look='C', space=truck, face=(1.0, 0.0, 0.0))
    for z in (0.5, 0.9, 1.25):
        town.box(m, (0.38, 0.03, 0.03), at=(0.0, 0.03, z), look='C', space=truck)
    town.box(m, (0.42, 0.22, 0.02), at=(0.0, -0.06, 0.03), look='iron', space=truck)
    for x in (-0.24, 0.24):
        town.cylinder(m, (x - 0.02, 0.08, 0.15), (x + 0.02, 0.08, 0.15), 0.14, sides=8, look='iron', space=truck)
    m.section('baggage room things')

    # --- The roof: shakes on both slopes, a chimney stack through the back near the ridge (the waiting room's stove), a
    # stovepipe through the front (the agent's, upstairs). ---
    front_roof, back_roof = town.gable(m, X0, X1, Y0, Y1, EAVE, PITCH, overhang=0.45, rake=0.4, deck=0.1, sag=0.05)
    CH = (-2.6, 0.7)
    PIPE = (4.4, -1.2)
    for slope in (front_roof, back_roof):
        shakes(m, slope, piece=(3.2, 4.8), skip=skip_round(slope, [(CH[0], CH[1], 0.45, 0.5),
                                                                   (PIPE[0], PIPE[1], 0.2, 0.2)]))
    town.ridge_cap(m, front_roof, back_roof, uv='C', width=0.16, thick=0.05)
    tan = math.tan(math.radians(PITCH))
    homes.stack(m, CH[0], CH[1], EAVE + (Y1 - CH[1]) * tan - 0.6, RIDGE + 0.85, size=(0.66, 0.72))
    pipe_roof = EAVE + (PIPE[1] - Y0) * tan
    rr.chimney_pipe(m, PIPE[0], PIPE[1], pipe_roof - 0.3, pipe_roof + 1.1, socket='Smoke_2')
    for x, side in ((X0, -1.0), (X1, 1.0)):
        for y in (Y0 + 0.3, Y1 - 0.3):
            z_rake = EAVE + ((Y1 - Y0) * 0.5 - abs(y)) * tan
            town.board(m, (x + side * 0.02, y, z_rake - 0.75), (x + side * 0.42, y, z_rake - 0.06), 0.1, 0.08,
                       face=(0.0, 1.0, 0.0), look='band_G')
    m.section('roof')

    # RANSOM'S REST on both gables, over the upper windows.
    for space, length in ((east, el), (west, wl)):
        sign_board(m, space, length * 0.5, WALL_H + 0.62, 3.3, 0.6, "RANSOM'S REST", 0.32, tol=0.06)
    m.section('gable signs')

    # --- The platform canopy (Depot.py's): shakes at 9 degrees on outlookers and knee brackets, a fascia with the
    # station's name board hung on it, a ledger along the wall. ---
    C_DEPTH, C_PITCH = 3.2, 9.0
    C_X0, C_X1 = X0 - 0.4, X1 + 0.4
    tc = math.radians(C_PITCH)
    c_top = BASE + UPPER - 0.22
    c_edge = c_top - C_DEPTH * math.tan(tc)
    canopy = kit.Slope((C_X0, Y0 - C_DEPTH, c_edge), (1.0, 0.0, 0.0), (0.0, math.cos(tc), math.sin(tc)), C_X1 - C_X0,
                       C_DEPTH / math.cos(tc) + 0.02)
    town.roof_deck(m, canopy, 0.07)
    shakes(m, canopy, piece=(2.6, 4.0))
    fascia_y = Y0 - C_DEPTH - 0.025
    town.board(m, (C_X0 - 0.03, fascia_y, c_edge - 0.1), (C_X1 + 0.03, fascia_y, c_edge - 0.1), 0.2, 0.05, look='C')
    town.board(m, (C_X0 - 0.04, fascia_y - 0.03, c_edge - 0.215), (C_X1 + 0.04, fascia_y - 0.03, c_edge - 0.215), 0.06,
               0.03, look='band_G')
    name = kit.wall_space((-5.0, fascia_y - 0.035), (5.0, fascia_y - 0.035), 0.0)
    sign_board(m, name, 5.0, c_edge - 0.25, 3.5, 0.4, "RANSOM'S REST", 0.25, tol=0.055)
    for x in (-1.2, 1.2):
        town.box(m, (0.06, 0.1, 0.16), at=(x, fascia_y - 0.01, c_edge - 0.25 + 0.27), look='iron')

    def under(y_out, depth):
        return c_top - y_out * math.tan(tc) - 0.07 - depth
    brackets = [-6.75, -4.0, 0.0, 2.45, 6.8]
    for x in brackets:
        town.board(m, (x, Y0 - 0.02, under(0.0, 0.1)), (x, Y0 - C_DEPTH + 0.02, under(C_DEPTH, 0.1)), 0.2, 0.12,
                   face=(1.0, 0.0, 0.0), look='C')
        low = BASE + (2.9 if x > 6.0 else 2.05)
        town.board(m, (x, Y0 - 0.075, low), (x, Y0 - 0.075, under(0.0, 0.2)), 0.16, 0.09, look='band_G')
        town.board(m, (x, Y0 - 0.12, low + 0.15), (x, Y0 - 1.95, under(1.95, 0.2) + 0.03), 0.15, 0.1,
                   face=(1.0, 0.0, 0.0), look='band_G')
        town.box(m, (0.09, 0.09, 0.22), at=(x, Y0 - 1.95, under(1.95, 0.2) - 0.1), look='band_G', drop=('+z',))
    for lx in kit.frange(C_X0 + 0.9, C_X1 - 0.3, 0.9):
        if all(abs(lx - b) > 0.35 for b in brackets):
            town.board(m, (lx, Y0 - 0.02, under(0.0, 0.07)), (lx, Y0 - C_DEPTH + 0.03, under(C_DEPTH, 0.07)), 0.14,
                       0.06, face=(1.0, 0.0, 0.0), look='C')
    town.board(m, (C_X0 + 0.4, Y0 - 0.06, under(0.0, 0.0) - 0.12), (C_X1 - 0.4, Y0 - 0.06, under(0.0, 0.0) - 0.12),
               0.2, 0.06, look='C')
    m.section('canopy')

    # Under it: the station board's place by the doors with its lamp, two benches against the wall.
    bx = -0.9
    rr.lamp_bracket(m, (bx, Y0 - 0.04), (0.0, -1.0), BASE + 2.62, socket='Light')
    m.socket('StationBoard', (bx, Y0 - 0.03, BASE + 1.6))
    rr.bench(m, -6.25, -4.45, Y0 - 0.03, BASE, -1.0)
    rr.bench(m, 0.0, 1.8, Y0 - 0.03, BASE, -1.0)
    m.socket('Sit_1', (-5.35, Y0 - 0.33, BASE + 0.45))
    m.socket('Sit_2', (0.9, Y0 - 0.33, BASE + 0.45))
    m.socket('Decal', (2.15, Y0 - 0.03, BASE + 1.7))
    m.socket('Light_Upstairs', (X0 + U_F[2].x + U_F[2].w * 0.5, Y0 + 1.0, BASE + UPPER + 1.5))
    m.section('platform side')

    # --- Collision: the floor, the outer walls round their doorways, the partitions round theirs, the upper storey solid
    # but for the stair hall, the stair and its enclosure, the canopy and the roof (furniture above). ---
    m.hull((X1 - X0, Y1 - Y0, BASE + 0.1), at=(0.0, 0.0, (BASE - 0.1) * 0.5))
    for a, b in ((X0, dx0), (dx1, X0 + BAG.x), (X0 + BAG.x + BAG.w, X1)):
        m.hull((b - a, T, EAVE - BASE), at=((a + b) * 0.5, Y0 + T * 0.5, (EAVE + BASE) * 0.5))
    m.hull((dx1 - dx0, T, EAVE - BASE - DOOR.h), at=((dx0 + dx1) * 0.5, Y0 + T * 0.5, (EAVE + BASE + DOOR.h) * 0.5))
    m.hull((BAG.w, T, EAVE - BASE - BAG.h), at=(X0 + BAG.x + BAG.w * 0.5, Y0 + T * 0.5, (EAVE + BASE + BAG.h) * 0.5))
    m.hull((X1 - X0, T, EAVE), at=(0.0, Y1 - T * 0.5, EAVE * 0.5))
    for x in (X0 + T * 0.5, X1 - T * 0.5):
        m.hull((T, Y1 - Y0, EAVE), at=(x, 0.0, EAVE * 0.5))
    pc = (P1[0] + P1[1]) * 0.5
    m.hull((WALL_P, YI1 - (-2.3), CEIL), at=(pc, (YI1 - 2.3) * 0.5, BASE + CEIL * 0.5))
    m.hull((WALL_P, -3.5 - YI0, CEIL), at=(pc, (YI0 - 3.5) * 0.5, BASE + CEIL * 0.5))
    m.hull((WALL_P, 1.2, CEIL - 2.15), at=(pc, -2.9, BASE + (CEIL + 2.15) * 0.5))
    m.hull((XI1 - 3.55, WALL_P, CEIL), at=((3.55 + XI1) * 0.5, 0.0, BASE + CEIL * 0.5))
    m.hull((2.45 - P1[1], WALL_P, CEIL), at=((2.45 + P1[1]) * 0.5, 0.0, BASE + CEIL * 0.5))
    m.hull((1.1, WALL_P, CEIL - 2.15), at=(3.0, 0.0, BASE + (CEIL + 2.15) * 0.5))
    m.hull((X1 - OPEN_X, Y1 - Y0, EAVE - BASE - CEIL), at=((OPEN_X + X1) * 0.5, 0.0, (EAVE + BASE + CEIL) * 0.5))
    m.hull((OPEN_X - X0, SY0 - Y0, EAVE - BASE - CEIL), at=((X0 + OPEN_X) * 0.5, (Y0 + SY0) * 0.5,
                                                            (EAVE + BASE + CEIL) * 0.5))
    m.hull((OPEN_X - X0, Y1 - SY0, EAVE - BASE - HALL_CEIL), at=((X0 + OPEN_X) * 0.5, (SY0 + Y1) * 0.5,
                                                                 (EAVE + BASE + HALL_CEIL) * 0.5))
    m.hull((TOP_X - X0, Y1 - SY0, BASE + UPPER), at=((X0 + TOP_X) * 0.5, (SY0 + Y1) * 0.5, (BASE + UPPER) * 0.5))
    m.hull_points([(x, y, BASE + z) for x, z in ((ENCLOSE_X, nose(ENCLOSE_X)), (ENCLOSE_X, CEIL), (TOP_X, CEIL),
                                                 (TOP_X, UPPER)) for y in (SY0 - WALL_P, SY0)])
    m.hull_points([(x, y, BASE + z) for x, z in ((FOOT + 0.05, 0.0), (FOOT + 0.05, R_RISE + 0.95),
                                                 (ENCLOSE_X, nose(ENCLOSE_X) + 0.95), (ENCLOSE_X, 0.0))
                   for y in (SY0 - 0.02, SY0 + 0.06)])
    back_eave = EAVE - 0.45 * tan
    m.hull_points([(x, Y0 - 0.45, back_eave) for x in (X0 - 0.4, X1 + 0.4)] +
                  [(x, Y1 + 0.45, back_eave) for x in (X0 - 0.4, X1 + 0.4)] +
                  [(x, 0.0, RIDGE + 0.22) for x in (X0 - 0.4, X1 + 0.4)])
    m.hull((canopy.width, canopy.length, 0.16), at=(canopy.width * 0.5, canopy.length * 0.5, -0.04), space=canopy)
    m.hull((0.26, 0.26, 0.3), at=(-1.75, YI0 + 0.4, BASE + 0.15))

    m.rooms += [
        f'waiting room {P1[0] - XI0:.2f} x {YI1 - YI0:.2f} m inside, floor +{BASE:.2f}, board ceiling {CEIL:.2f} m over it',
        f'ticket office {XI1 - P1[1]:.2f} x {YI1 - P2[1]:.2f} m, behind the grille (ticket window {gy1 - gy0:.2f} m wide)',
        f'baggage room {XI1 - P1[1]:.2f} x {P2[0] - YI0:.2f} m, its door {BAG.w:.2f} x {BAG.h:.2f} m onto the platform; '
        f'the lost property counter along its east wall',
        f'stair: {RISERS} risers of {R_RISE * 100:.1f} cm, treads {R_RUN * 100:.0f} cm, {STAIR_W:.2f} m wide, to a '
        f'landing at +{BASE + UPPER:.2f} (the agent\'s door, {AGENT[1] - AGENT[0]:.2f} x 2.10 m)',
        f'doorways: platform double doors {DOOR.w:.2f} x {DOOR.h:.2f}, waiting room-baggage 1.20 x 2.15, '
        f'baggage-ticket office 1.10 x 2.15',
    ]
    m.captions = ['The depot, two storeys with the agent upstairs:', "waiting room, ticket office, baggage, lost property."]
    return m, m.finish(pivot=(DOOR_MID, Y0), footprint=(X0, X1, Y0, Y1))


# --- The coal stage ---

def coal_lumps(m, cx, cy, z, rx, ry, count, size=(0.05, 0.11), heap=0.0):
    """Lumps of coal (black paint, PaintBlack) scattered over an ellipse, or heaped (heap: how high the middle rises)."""
    rng = m.rng
    for _ in range(count):
        a = rng.uniform(0.0, 2.0 * math.pi)
        r = math.sqrt(rng.uniform(0.0, 1.0))
        x, y = cx + rx * r * math.cos(a), cy + ry * r * math.sin(a)
        s = rng.uniform(*size)
        zz = z + heap * (1.0 - r * r) + s * 0.3
        town.box(m, (s, s * rng.uniform(0.7, 1.1), s * rng.uniform(0.6, 0.9)), at=(x, y, zz),
                 rot=(rng.uniform(-30, 30), rng.uniform(-30, 30), rng.uniform(0, 90)), look='black')


def coal_stage():
    m = rr.Building('RR_CoalStage', seed=721)
    rng = m.rng
    BX0, BX1, BY0, BY1 = -2.2, 2.2, -1.4, 1.4      # the bin's outer faces
    FLOOR, WALL = 2.1, 1.3                          # the bin's floor over the ground, its walls' height
    posts = [(x, y) for x in (-2.0, 0.0, 2.0) for y in (-1.2, 1.2)]
    for x, y in posts:
        kit.stone_stack(m, (x, y, -0.1), (0.42, 0.42), 0.32, taper=0.1, rot=rng.uniform(-8.0, 8.0))
        town.board(m, (x, y, 0.2), (x, y, FLOOR - 0.12), 0.22, 0.22, look='C', face=(0.0, -1.0, 0.0))
    # Girts and cross braces between the posts, two sill beams under the bin.
    for y in (-1.2, 1.2):
        town.board(m, (BX0 - 0.1, y, FLOOR - 0.06), (BX1 + 0.1, y, FLOOR - 0.06), 0.2, 0.24, look='C')
        for xa, xb in ((-2.0, 0.0), (0.0, 2.0)):
            town.board(m, (xa + 0.1, y, 0.35), (xb - 0.1, y, FLOOR - 0.25), 0.12, 0.06, look='C',
                       face=(0.0, -1.0 if y < 0 else 1.0, 0.0), lift=0.14)
    for x in (-2.0, 2.0):
        town.board(m, (x, -1.1, 0.4), (x, 1.1, FLOOR - 0.3), 0.12, 0.06, look='C', face=(-1.0 if x < 0 else 1.0, 0.0, 0.0),
                   lift=0.14)
    m.section('frame')
    # The bin: plank walls on corner posts, both faces, a plank floor; coal heaped in it.
    for p0, p1 in (((BX0, BY0), (BX1, BY0)), ((BX1, BY0), (BX1, BY1)), ((BX1, BY1), (BX0, BY1)), ((BX0, BY1), (BX0, BY0))):
        p0v, p1v = Vector(p0), Vector(p1)
        length = (p1v - p0v).length
        space = kit.wall_space(p0, p1, FLOOR)
        rows = 6
        for k in range(rows):
            z = WALL * (k + 0.5) / rows
            gap = k >= 3 and p0 == (BX0, BY0)
            if gap:
                for a, b in ((0.0, length * 0.5 - 0.42), (length * 0.5 + 0.42, length)):
                    town.board(m, (a, 0.03, z), (b, 0.03, z), WALL / rows - 0.01, 0.05, look='A', space=space)
            else:
                town.board(m, (0.0, 0.03, z), (length, 0.03, z), WALL / rows - 0.01, 0.05, look='A', space=space)
        for x in (0.08, length - 0.08):
            town.board(m, (x, -0.04, -0.15), (x, -0.04, WALL + 0.08), 0.14, 0.1, look='C', space=space)
        town.board(m, (0.0, -0.02, WALL + 0.03), (length, -0.02, WALL + 0.03), 0.06, 0.12, look='C', space=space,
                   face=(0.0, 0.0, 1.0))
    town.box(m, (BX1 - BX0, BY1 - BY0, 0.06), at=(0.0, 0.0, FLOOR + 0.0), look='planks')
    tb_top = FLOOR + WALL + 0.25
    coal = kit._new_bmesh()
    rows = []
    for j in range(4):
        t = j / 3.0
        r = 1.0 - t
        row = []
        if j == 3:
            rows.append([coal.verts.new((0.2, 0.1, tb_top + 0.2))])
            continue
        for i in range(10):
            a = 2.0 * math.pi * i / 10
            k = 1.0 + rng.uniform(-0.06, 0.06)
            row.append(coal.verts.new(((BX1 - 0.12) * r * k * math.cos(a) * 1.02, (BY1 - 0.12) * r * k * math.sin(a) * 1.02,
                                       FLOOR + WALL - 0.15 + t * 0.5 + rng.uniform(-0.04, 0.04))))
        rows.append(row)
    for j in range(3):
        a_row, b_row = rows[j], rows[j + 1]
        for i in range(10):
            i1 = (i + 1) % 10
            if len(b_row) == 1:
                coal.faces.new((a_row[i], a_row[i1], b_row[0]))
            else:
                coal.faces.new((a_row[i], a_row[i1], b_row[i1], b_row[i]))
    spec, key = town.finish('black')
    m.emit(coal, spec, key)
    coal_lumps(m, 0.2, 0.1, FLOOR + WALL - 0.05, 1.5, 0.85, 26, size=(0.08, 0.16), heap=0.4)
    m.section('bin, coal')
    # The chute: a trough of planks lined with iron from a gap in the bin's front wall, out and down over the track side,
    # an iron apron hinged at its end and held up by chains from a gallows beam.
    top = Vector((0.0, BY0, FLOOR + WALL * 0.62))
    end = Vector((0.0, BY0 - 1.45, FLOOR - 0.45))
    d = (end - top).normalized()
    chute = Space(kit.toward(top, end, (0.0, 0.0, 1.0)))
    length = (end - top).length
    town.box(m, (length, 0.04, 0.8), at=(length * 0.5, 0.02, 0.0), look='A', space=chute)
    town.box(m, (length, 0.01, 0.72), at=(length * 0.5, -0.006, 0.0), look='iron', space=chute)
    for s in (-1.0, 1.0):
        town.box(m, (length, 0.3, 0.04), at=(length * 0.5, -0.15, s * 0.42), look='A', space=chute)
    apron = Space(chute.matrix @ Matrix.Translation((length, 0.0, 0.0)) @ Matrix.Rotation(math.radians(-24.0), 4, 'Z'))
    town.box(m, (0.55, 0.02, 0.76), at=(0.275, 0.0, 0.0), look='iron', space=apron)
    # The gallows: two posts up the bin's front from the ground, beams out over the chute, a cross beam and the chains.
    gy, gz, reach = BY0 - 0.12, FLOOR + 1.85, BY0 - 1.62
    for x in (-0.75, 0.75):
        town.board(m, (x, gy, 0.0), (x, gy, gz + 0.08), 0.16, 0.16, look='C')
        town.board(m, (x, gy + 0.02, gz), (x, reach, gz), 0.18, 0.14, look='C', face=(0.0, 0.0, 1.0))
        town.board(m, (x, gy - 0.08, gz - 0.8), (x, gy - 0.75, gz - 0.06), 0.1, 0.08, look='C', face=(1.0, 0.0, 0.0))
    town.board(m, (-0.85, reach, gz + 0.12), (0.85, reach, gz + 0.12), 0.14, 0.12, look='C', face=(0.0, 0.0, 1.0))
    tip = Vector(apron.world((0.5, 0.0, 0.0)))
    for x in (-0.36, 0.36):
        town.board(m, (x, reach, gz + 0.05), (x, tip.y, tip.z), 0.025, 0.02, look='iron', face=(1.0, 0.0, 0.0))
    m.section('chute')
    # A ladder up the east side, a shovel against the front post, a sack under the chute, coal spilled on the ground.
    for y in (-0.25, 0.25):
        town.board(m, (BX1 + 0.55, y, 0.0), (BX1 + 0.05, y, FLOOR + WALL + 0.5), 0.07, 0.05, look='C',
                   face=(1.0, 0.0, 0.0))
    for k in range(1, 12):
        t = k / 12.0
        x = BX1 + 0.55 - 0.5 * t
        z = (FLOOR + WALL + 0.5) * t
        town.box(m, (0.035, 0.5, 0.035), at=(x, 0.0, z), look='C', drop=('-y', '+y'))
    shovel = Space(Matrix.Translation((-0.6, BY0 - 0.46, 0.0)) @ Matrix.Rotation(math.radians(-16.0), 4, 'X'))
    town.board(m, (0.0, 0.0, 0.32), (0.0, 0.0, 1.38), 0.035, 0.035, look='C', space=shovel)
    town.box(m, (0.26, 0.02, 0.32), at=(0.0, 0.0, 0.17), look='iron', space=shovel)
    # The sack: half full of coal, slumped against itself, its mouth folded over.
    sack_at = (-0.55, BY0 - 1.75, 0.0)
    sack = kit._new_bmesh()
    prof = [(0.0, 0.0), (0.27, 0.0), (0.31, 0.12), (0.27, 0.3), (0.19, 0.4), (0.09, 0.45), (0.15, 0.5), (0.0, 0.48)]
    rings = []
    for r, z in prof:
        lean = z * 0.22
        if r <= 0.0:
            rings.append([sack.verts.new((sack_at[0] + lean, sack_at[1], z))])
        else:
            rings.append([sack.verts.new((sack_at[0] + lean + r * math.cos(2 * math.pi * i / 8) * rng.uniform(0.9, 1.1),
                                          sack_at[1] + r * 0.85 * math.sin(2 * math.pi * i / 8) * rng.uniform(0.9, 1.1),
                                          z + (rng.uniform(-0.02, 0.02) if z > 0.0 else 0.0))) for i in range(8)])
    for ra, rb in zip(rings, rings[1:]):
        for i in range(8):
            i1 = (i + 1) % 8
            if len(ra) == 1:
                sack.faces.new((ra[0], rb[i1], rb[i]))
            elif len(rb) == 1:
                sack.faces.new((ra[i], ra[i1], rb[0]))
            else:
                sack.faces.new((ra[i], ra[i1], rb[i1], rb[i]))
    m.emit(sack, Tile('Polymer'), 'canvas', smooth=True)
    coal_lumps(m, 0.1, BY0 - 2.0, 0.0, 0.9, 0.55, 16, size=(0.05, 0.1))
    coal_lumps(m, -0.5, BY0 - 1.75, 0.6, 0.12, 0.1, 4, size=(0.05, 0.08))
    m.section('ladder, shovel, sack')

    m.socket('CoalStage', tuple(end + Vector((0.0, -0.3, 0.05))), rr.rot_to((0.0, -1.0)))
    m.socket('Sack', sack_at, (0.0, 0.0, 0.0))
    for x, y in posts:
        m.hull((0.24, 0.24, FLOOR), at=(x, y, FLOOR * 0.5))
    m.hull((BX1 - BX0, BY1 - BY0, WALL + 0.35), at=(0.0, 0.0, FLOOR + (WALL + 0.35) * 0.5))
    m.hull((length, 0.36, 0.86), at=(length * 0.5, -0.15, 0.0), space=chute)
    m.hull((0.55, 0.45, 0.5), at=(sack_at[0], sack_at[1], 0.25))
    m.rooms.append(f'the bin {BX1 - BX0:.1f} x {BY1 - BY0:.1f} m, its floor {FLOOR:.1f} m up; the chute ends '
                   f'{(BY0 + 0.2) - (BY0 - 1.45):.2f} m in front of the posts at {FLOOR - 0.45:.2f} m up')
    m.captions = ['The coal stage by the water tower: a bin of coal', 'on posts, the chute over the track, a sack under it.']
    return m, m.finish(pivot=(0.0, BY0 + 0.2), footprint=(BX0, BX1, BY0, BY1))


BUILT = [depot(), coal_stage()]
models = [obj for _, obj in BUILT]


def at(x, y, z):
    """A point in the depot's frame as built (the middle of its footprint) where the exported model has it."""
    return (x - DOOR_MID, y - Y0, z)


def inner(c):
    """For the roof-off view: only what's inside the outer walls' inner faces (the near walls' outsides go)."""
    x, y = c.x + DOOR_MID, c.y + Y0
    return XI0 - 0.01 <= x <= XI1 + 0.01 and YI0 - 0.01 <= y <= YI1 + 0.01


if lt.want_preview():
    (dm, dep), (cm, coal) = BUILT
    shots = {dep.name: [], coal.name: []}
    shots[dep.name].append(rr.exterior(dep, 'RR_Depot', view=(-0.55, -1.0, 0.3), sun=(38.0, 16.0)))
    shots[dep.name].append(rr.exterior(dep, 'RR_Depot_Back', view=(0.55, 1.0, 0.32), sun=(-150.0, 18.0),
                                       figure='left'))
    shots[dep.name].append(rr.plan_view(dep, 'RR_Depot_Plan', z_cut=BASE + CEIL - 0.25, direction=(-0.22, -0.55, 1.0),
                                        lamp_energy=40.0, keep=inner))
    shots[dep.name].append(rr.inside(dep, 'RR_Depot_WaitingRoom', at(-1.9, -3.55, BASE + 1.45),
                                     at(-4.6, 3.0, BASE + 1.55), lens=20.0))
    shots[dep.name].append(rr.inside(dep, 'RR_Depot_TicketWindow', at(-5.2, -2.6, BASE + 1.45),
                                     at(1.94, 0.7, BASE + 1.55), lens=22.0))
    shots[dep.name].append(rr.inside(dep, 'RR_Depot_LostProperty', at(3.4, -3.6, BASE + 1.5),
                                     at(6.3, -1.2, BASE + 1.35), lens=20.0, lamp_energy=120.0))
    shots[dep.name].append(rr.inside(dep, 'RR_Depot_TicketOffice', at(5.9, 0.6, BASE + 1.45),
                                     at(2.2, 2.6, BASE + 1.2), lens=20.0))
    shots[dep.name].append(rr.inside(dep, 'RR_Depot_StairHall', at(-3.3, 3.3, BASE + nose(-3.3) + 1.42),
                                     at(-6.2, 2.75, BASE + UPPER + 1.1), lens=20.0))
    shots[coal.name].append(rr.exterior(coal, 'RR_CoalStage', view=(-0.75, -1.0, 0.32)))
    rr.write_report('DepotRR', [dm, cm], shots)
