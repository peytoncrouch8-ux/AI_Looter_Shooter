"""The timber-frame cottage at the village square: close-set oak posts over lime plaster on a fieldstone base, a steep
shake roof whose gable faces the front, a stone chimney, a bay window and a teal door under a little gabled hood.

The posts stand every 0.8 m: the plaster (strip G) is mapped in upright 0.8 m bands, so every seam in it hides under a
post. A scripted model (Art/README.md) built with looter_buildings: the pivot is the middle of the footprint at ground
level and the door faces the front (-Y). SOCKET_Smoke tops the chimney.
"""
import looter_buildings as kit
from looter_buildings import Opening, Trim, math

m = kit.Model('Cottage', seed=33)
rng = m.rng

X0, X1, Y0, Y1 = -2.4, 2.4, -3.2, 3.2   # outer faces of the plaster: the gable ends face front and back
BASE = 0.45                            # top of the stone base
WALL = 2.5                             # sill beam to the top of the wall plate
EAVE = BASE + WALL
PITCH = 55.0
THICK = 0.16
GRID = 0.8                             # post spacing = the plaster band width
TIMBER = 0.18                          # post and beam faces
PROUD = 0.05                           # how far the timbers stand out of the plaster
PLASTER = Trim('G', world=True, rotate=True)
theta = math.radians(PITCH)
RISE = (X1 - X0) * 0.5 * math.tan(theta)

kit.plinth(m, X0, X1, Y0, Y1, BASE, out=0.06)
m.section('base')


def timber(p0, p1, space, lift=PROUD, width=TIMBER):
    """An oak timber on the wall face, its back sunk into the plaster."""
    m.board(p0, p1, width, 0.1, uv=Trim('C', lane='each'), space=space, lift=lift - 0.05)


def frame_wall(space, length, items, apex=None):
    """Plaster with its windows and door, and the timber frame over it: sill beam, wall plate, a rail under the
    windows, posts every GRID (split around openings) and braces in the corner bays; in a gable, posts up to the
    roof line and a collar beam."""
    top = WALL + (RISE if apex else 0.0)
    m.panel(kit.wall_outline(length, WALL, items, (length * 0.5, WALL + RISE) if apex else None), kit.holes(items),
            THICK, PLASTER, space=space, around=items)
    kit.openings(m, space, items, THICK)

    def roof_line(x):
        return WALL + (length * 0.5 - abs(x - length * 0.5)) * math.tan(theta) if apex else WALL

    def gaps(x, half):
        """The heights a post at x keeps clear of: openings it would cross."""
        return [(o.z - (0.06 if o.z > 0 else 1.0), o.z + o.h + 0.16) for o in items
                if o.x - half < x < o.x + o.w + half]

    # Beams: the sill, the plate (the gable's tie beam) and a rail under the windows, broken by openings.
    timber((0.0, 0.0, 0.1), (length, 0.0, 0.1), space, lift=PROUD + 0.01, width=0.2)
    timber((0.0, 0.0, WALL - 0.1), (length, 0.0, WALL - 0.1), space, lift=PROUD + 0.01, width=0.2)
    rail_z = 0.85
    spans = [(TIMBER, length - TIMBER)]
    for o in items:
        if o.z < rail_z + 0.1 < o.z + o.h or o.z > 1e-4 and abs(o.z - rail_z) < 0.15:
            spans = [(a, min(b, o.x - 0.02)) for a, b in spans] + [(max(a, o.x + o.w + 0.02), b) for a, b in spans]
    for a, b in spans:
        if b - a > 0.1:
            timber((a, 0.0, rail_z), (b, 0.0, rail_z), space, lift=PROUD + 0.005, width=0.16)
    for o in items:
        if o.z > 1e-4:  # a lintel over windows (the door has its own casing)
            timber((o.x - 0.2, 0.0, o.z + o.h + 0.09), (o.x + o.w + 0.2, 0.0, o.z + o.h + 0.09), space,
                   lift=PROUD + 0.012, width=0.16)
    # Posts: every GRID along the wall (the corners set in by half a post), and at the sides of every opening.
    xs = [TIMBER * 0.5] + [GRID * k for k in range(1, int(length / GRID + 1e-6) + 1) if GRID * k < length - 0.2]
    xs.append(length - TIMBER * 0.5)
    for o in items:
        xs += [o.x - TIMBER * 0.5, o.x + o.w + TIMBER * 0.5]
    xs = sorted(xs)
    kept = []
    for x in xs:  # posts closer than a post's width merge (an opening's side on the grid)
        if kept and x - kept[-1] < TIMBER * 0.9:
            kept[-1] = (kept[-1] + x) * 0.5 if abs(x - kept[-1]) < 0.02 else kept[-1]
            continue
        kept.append(x)
    corner_bays = [(kept[0], kept[1]), (kept[-2], kept[-1])]
    for x in kept:
        z_top = min(WALL, roof_line(x)) - 0.2 if not apex else WALL - 0.2
        runs = [(0.2, z_top)]
        for g0, g1 in gaps(x, TIMBER * 0.5 - 0.01):
            runs = [(a, min(b, g0)) for a, b in runs] + [(max(a, g1), b) for a, b in runs]
        for a, b in runs:
            if b - a > 0.08:
                timber((x, 0.0, a), (x, 0.0, b), space)
        if apex and roof_line(x) - WALL > 0.35:
            # Up into the gable, to under the roof.
            runs = [(WALL, roof_line(x) - 0.12)]
            for g0, g1 in gaps(x, TIMBER * 0.5 - 0.01):
                runs = [(a, min(b, g0)) for a, b in runs] + [(max(a, g1), b) for a, b in runs]
            for a, b in runs:
                if b - a > 0.08:
                    timber((x, 0.0, a), (x, 0.0, b), space)
    # Braces in the corner bays above the rail, rising toward the corners.
    for (a, b), toward_left in zip(corner_bays, (True, False)):
        if any(o.x < b and o.x + o.w > a for o in items):
            continue
        lo, hi = rail_z + 0.08, WALL - 0.2
        if toward_left:
            timber((b - TIMBER * 0.5, 0.0, lo), (a + TIMBER * 0.5, 0.0, hi), space, lift=PROUD - 0.004, width=0.15)
        else:
            timber((a + TIMBER * 0.5, 0.0, lo), (b - TIMBER * 0.5, 0.0, hi), space, lift=PROUD - 0.004, width=0.15)
    if apex:
        collar = WALL + 1.25
        half = (top - collar) / math.tan(theta)
        spans = [(length * 0.5 - half + 0.05, length * 0.5 + half - 0.05)]
        for o in items:
            if o.z < collar < o.z + o.h + 0.2:
                spans = [(a, min(b, o.x - 0.1)) for a, b in spans] + [(max(a, o.x + o.w + 0.1), b) for a, b in spans]
        for a, b in spans:
            if b - a > 0.1:
                timber((a, 0.0, collar), (b, 0.0, collar), space, lift=PROUD + 0.008, width=0.17)


# --- Walls, counterclockwise from above. The front gable has the door and the bay (in front of plain plaster). ---
DOOR = Opening('door', 0.89, 0.0, 1.0, 2.05, paint='H1', casing='C', hinge='left')
window = dict(style='glass', frame=False, paint='C', sash='H1')
walls = [
    ((X0, Y0), (X1, Y0), True, [DOOR, Opening('window', 2.09, 3.35, 0.62, 0.7, panes=(2, 2), **window)]),
    ((X1, Y0), (X1, Y1), False, [Opening('window', 1.69, 0.95, 1.42, 1.1, panes=(3, 2), **window),
                                  Opening('window', 4.09, 0.95, 0.62, 1.1, panes=(2, 3), **window)]),
    ((X1, Y1), (X0, Y1), True, [Opening('window', 0.89, 0.95, 0.62, 1.1, panes=(2, 3), **window),
                                 Opening('window', 3.29, 0.95, 0.62, 1.1, panes=(2, 3), **window),
                                 Opening('window', 2.09, 3.35, 0.62, 0.7, style='boarded', frame=False)]),
    ((X0, Y1), (X0, Y0), False, [Opening('window', 3.29, 0.95, 0.62, 1.1, panes=(2, 3), **window)]),
]
for p0, p1, gable_end, items in walls:
    space = kit.wall_space(p0, p1, BASE)
    frame_wall(space, (kit.Vector(p1) - kit.Vector(p0)).length, items, apex=gable_end)
m.section('walls')

# --- The door's hood: a little gable on two carved brackets. ---
front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
door_mid = DOOR.x + DOOR.w * 0.5
for side in (-1.0, 1.0):
    x = door_mid + side * 0.56
    m.board((x, -0.02, 2.48), (x, -0.7, 2.48), 0.12, 0.09, face=(1.0, 0.0, 0.0), uv='C', space=front)
    m.board((x, -0.02, 1.98), (x, -0.44, 2.43), 0.1, 0.08, face=(1.0, 0.0, 0.0), uv='C', space=front)
hood_frame = front.matrix @ kit.Matrix.Translation((door_mid, 0.0, 0.0)) @ kit.Matrix.Rotation(math.radians(-90.0), 4, 'Z')
h_front, h_back = kit.gable(m, 0.0, 0.62, -0.62, 0.62, 2.62, 45.0, overhang=0.1, rake=0.12, deck=0.05, frame=hood_frame,
                            ends=(False, True))
kit.shingles(m, h_front, piece=(0.7, 1.0))
kit.shingles(m, h_back, piece=(0.7, 1.0))
kit.ridge_cap(m, h_front, h_back, uv='C', width=0.1, thick=0.04)
m.section('door hood')

# --- The bay window: a stone footing, timber posts, plaster under three windows and a lean-to roof. ---
BX0, BX1, BDEPTH = X0 + 2.4 + 0.09, X0 + 4.0 - 0.09, 0.6
BY = Y0 - BDEPTH
kit.stone_stack(m, ((BX0 + BX1) * 0.5, Y0 - BDEPTH * 0.5 + 0.02, 0.0), (BX1 - BX0 + 0.12, BDEPTH + 0.1), BASE)
bay_sides = [((BX0, Y0), (BX0, BY)), ((BX0, BY), (BX1, BY)), ((BX1, BY), (BX1, Y0))]
for p0, p1 in bay_sides:
    space = kit.wall_space(p0, p1, BASE)
    length = (kit.Vector(p1) - kit.Vector(p0)).length
    pane = Opening('window', 0.1, 0.95, length - 0.2, 1.2, panes=(max(1, round(length / 0.5)), 2), **window)
    m.panel(kit.rect(0.0, 0.0, length, 2.3), kit.holes([pane]), 0.1, PLASTER, space=space)
    kit.window(m, space, pane, 0.1)
    for x in (0.05, length - 0.05):
        timber((x, 0.0, 0.0), (x, 0.0, 2.3), space, width=0.12)
    timber((0.0, 0.0, 0.1), (length, 0.0, 0.1), space, lift=PROUD + 0.01, width=0.2)
    timber((0.0, 0.0, 0.87), (length, 0.0, 0.87), space, lift=PROUD + 0.01, width=0.14)
    timber((0.0, 0.0, 2.24), (length, 0.0, 2.24), space, lift=PROUD + 0.01, width=0.16)
b_theta = math.atan2(0.28, BDEPTH)
b_over = 0.15
b_origin = kit.Vector((BX0 - b_over, BY - b_over, BASE + 2.32 - b_over * math.tan(b_theta)))
b_origin += kit.Vector((0.0, -math.sin(b_theta), math.cos(b_theta))) * 0.05
bay_roof = kit.Slope(b_origin, (1.0, 0.0, 0.0), (0.0, math.cos(b_theta), math.sin(b_theta)), BX1 - BX0 + 2 * b_over,
                     (BDEPTH + b_over) / math.cos(b_theta))
kit.roof_deck(m, bay_roof, 0.05)
kit.shingles(m, bay_roof, piece=(0.8, 1.2))
m.section('bay window')

# --- Roof: steep shakes on a deck, a ridge board and a fieldstone chimney near the left gable. ---
OVERHANG, RAKE = 0.35, 0.35
# The ridge runs front to back: the roof is built along X and turned a quarter (its right slope is the kit's front).
right_roof, left_roof = kit.gable(m, Y0, Y1, -X1, -X0, EAVE, PITCH, overhang=OVERHANG, rake=RAKE, deck=0.1, sag=0.08,
                                  frame=kit.Matrix.Rotation(math.radians(90.0), 4, 'Z'))
CHIMNEY = (-1.0, 1.9)


def by_chimney(slope):
    def skip(x, y):
        world = slope.world((x, y, 0.0))
        return abs(world.x - CHIMNEY[0]) < 0.42 and abs(world.y - CHIMNEY[1]) < 0.4
    return skip
kit.shingles(m, right_roof, skip=by_chimney(right_roof))
kit.shingles(m, left_roof, skip=by_chimney(left_roof))
kit.ridge_cap(m, right_roof, left_roof, uv='C', width=0.16, thick=0.05)
roof_at_chimney = EAVE + (CHIMNEY[0] - X0) * math.tan(theta)
top = EAVE + RISE + 0.75
kit.stone_stack(m, (CHIMNEY[0], CHIMNEY[1], roof_at_chimney - 0.6), (0.62, 0.72), top - roof_at_chimney + 0.6,
                taper=0.03)
m.box((0.78, 0.88, 0.1), at=(CHIMNEY[0], CHIMNEY[1], top + 0.05), uv=Trim('D', world=True), bevel=0.02)
m.box((0.36, 0.44, 0.18), at=(CHIMNEY[0], CHIMNEY[1], top + 0.19), uv='D')
m.socket('Smoke', (CHIMNEY[0], CHIMNEY[1], top + 0.3))
m.section('roof')

# --- Collision: the walls, the roof, the bay and its roof, the chimney. ---
m.hull((X1 - X0 + 0.14, Y1 - Y0 + 0.14, EAVE), at=(0.0, 0.0, EAVE * 0.5))
eave_top = EAVE - OVERHANG * math.tan(theta) + 0.25
m.hull_points([(x, y, eave_top) for x in (X0 - OVERHANG, X1 + OVERHANG) for y in (Y0 - RAKE, Y1 + RAKE)] +
              [(0.0, y, EAVE + RISE + 0.3) for y in (Y0 - RAKE, Y1 + RAKE)])
m.hull((BX1 - BX0 + 0.1, BDEPTH + 0.05, BASE + 2.4), at=((BX0 + BX1) * 0.5, Y0 - BDEPTH * 0.5, (BASE + 2.4) * 0.5))
m.hull((0.64, 0.74, top - roof_at_chimney + 0.4), at=(CHIMNEY[0], CHIMNEY[1], (roof_at_chimney + top) * 0.5))

m.finish()
