"""The timber-frame cottage at the village square: close-set oak posts over lime plaster on a fieldstone base, a steep
shake roof whose gable faces the front, a stone chimney, a bay window and a teal door under a little gabled hood.

The posts stand every 0.8 m: the plaster (strip G) is mapped in upright 0.8 m bands, so every seam in it hides under a
post. A scripted model (Art/README.md) built with looter_buildings: the pivot is the middle of the footprint at ground
level and the door faces the front (-Y). SOCKET_Smoke tops the chimney.

The file also makes the cottage Ransom's Rest puts on Main Street's side lots (Docs/Areas/RansomsRest.md):

  Cottage          the tutorial island's timber-frame cottage. It's in the game there, so its build is exactly what it
                   was: cottage() without ransom makes the same calls in the same order.
  Cottage_Ransom   a frontier settler's cottage on the same footprint, pivot and fieldstone base, under the same steep
                   shake roof and stone chimney (SOCKET_Smoke), with the door where it was: clapboard walls (the trim
                   sheet's siding, strip A, run level) under vertical boards in the gables, hewn corner boards, water
                   table and belt; two-over-two sash windows in faded teal casings, where the leaded bay was and round
                   the walls; and over the door, a plank stoop on a fieldstone block with a stone step, under a little
                   shed roof of shakes on two posts. Lived in, a week after the raid: black crepe on the door (a bow and
                   its tails), linen curtains in the front room's windows, a bench under the front window and firewood
                   stacked under the west eave. The front room's four windows (the front, the front gable's and the
                   front one on each side) are lamplit glass (WindowGlow, Delia's house's material: dark warm panes by
                   day; the game raises their Glow at dusk), and SOCKET_Light stands in the front room behind them,
                   where the level's dusk lamp goes. The stoop, its step, its posts, the bench and the woodpile have
                   collision; the stoop and step are walkable.

The two share the plinth's random draws only; the settler's parts draw on from there.
"""
import math

import bmesh

import looter_buildings as kit
import looter_model as lm
import looter_textures as lt
from looter_buildings import Matrix, Opening, Tile, Trim, Vector

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
OVERHANG, RAKE = 0.35, 0.35
CHIMNEY = (-1.0, 1.9)


# --- The tutorial island's cottage: the timber frame, the door's hood and the bay window ---

def timber(m, p0, p1, space, lift=PROUD, width=TIMBER):
    """An oak timber on the wall face, its back sunk into the plaster."""
    m.board(p0, p1, width, 0.1, uv=Trim('C', lane='each'), space=space, lift=lift - 0.05)


def frame_wall(m, space, length, items, apex=None):
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
    timber(m, (0.0, 0.0, 0.1), (length, 0.0, 0.1), space, lift=PROUD + 0.01, width=0.2)
    timber(m, (0.0, 0.0, WALL - 0.1), (length, 0.0, WALL - 0.1), space, lift=PROUD + 0.01, width=0.2)
    rail_z = 0.85
    spans = [(TIMBER, length - TIMBER)]
    for o in items:
        if o.z < rail_z + 0.1 < o.z + o.h or o.z > 1e-4 and abs(o.z - rail_z) < 0.15:
            spans = [(a, min(b, o.x - 0.02)) for a, b in spans] + [(max(a, o.x + o.w + 0.02), b) for a, b in spans]
    for a, b in spans:
        if b - a > 0.1:
            timber(m, (a, 0.0, rail_z), (b, 0.0, rail_z), space, lift=PROUD + 0.005, width=0.16)
    for o in items:
        if o.z > 1e-4:  # a lintel over windows (the door has its own casing)
            timber(m, (o.x - 0.2, 0.0, o.z + o.h + 0.09), (o.x + o.w + 0.2, 0.0, o.z + o.h + 0.09), space,
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
                timber(m, (x, 0.0, a), (x, 0.0, b), space)
        if apex and roof_line(x) - WALL > 0.35:
            # Up into the gable, to under the roof.
            runs = [(WALL, roof_line(x) - 0.12)]
            for g0, g1 in gaps(x, TIMBER * 0.5 - 0.01):
                runs = [(a, min(b, g0)) for a, b in runs] + [(max(a, g1), b) for a, b in runs]
            for a, b in runs:
                if b - a > 0.08:
                    timber(m, (x, 0.0, a), (x, 0.0, b), space)
    # Braces in the corner bays above the rail, rising toward the corners.
    for (a, b), toward_left in zip(corner_bays, (True, False)):
        if any(o.x < b and o.x + o.w > a for o in items):
            continue
        lo, hi = rail_z + 0.08, WALL - 0.2
        if toward_left:
            timber(m, (b - TIMBER * 0.5, 0.0, lo), (a + TIMBER * 0.5, 0.0, hi), space, lift=PROUD - 0.004, width=0.15)
        else:
            timber(m, (a + TIMBER * 0.5, 0.0, lo), (b - TIMBER * 0.5, 0.0, hi), space, lift=PROUD - 0.004, width=0.15)
    if apex:
        collar = WALL + 1.25
        half = (top - collar) / math.tan(theta)
        spans = [(length * 0.5 - half + 0.05, length * 0.5 + half - 0.05)]
        for o in items:
            if o.z < collar < o.z + o.h + 0.2:
                spans = [(a, min(b, o.x - 0.1)) for a, b in spans] + [(max(a, o.x + o.w + 0.1), b) for a, b in spans]
        for a, b in spans:
            if b - a > 0.1:
                timber(m, (a, 0.0, collar), (b, 0.0, collar), space, lift=PROUD + 0.008, width=0.17)


DOOR = Opening('door', 0.89, 0.0, 1.0, 2.05, paint='H1', casing='C', hinge='left')
BX0, BX1, BDEPTH = X0 + 2.4 + 0.09, X0 + 4.0 - 0.09, 0.6
BY = Y0 - BDEPTH


def timber_frame(m):
    """The walls, counterclockwise from above. The front gable has the door and the bay (in front of plain plaster)."""
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
        frame_wall(m, space, (kit.Vector(p1) - kit.Vector(p0)).length, items, apex=gable_end)
    m.section('walls')

    # --- The door's hood: a little gable on two carved brackets. ---
    front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
    door_mid = DOOR.x + DOOR.w * 0.5
    for side in (-1.0, 1.0):
        x = door_mid + side * 0.56
        m.board((x, -0.02, 2.48), (x, -0.7, 2.48), 0.12, 0.09, face=(1.0, 0.0, 0.0), uv='C', space=front)
        m.board((x, -0.02, 1.98), (x, -0.44, 2.43), 0.1, 0.08, face=(1.0, 0.0, 0.0), uv='C', space=front)
    hood_frame = (front.matrix @ kit.Matrix.Translation((door_mid, 0.0, 0.0))
                  @ kit.Matrix.Rotation(math.radians(-90.0), 4, 'Z'))
    h_front, h_back = kit.gable(m, 0.0, 0.62, -0.62, 0.62, 2.62, 45.0, overhang=0.1, rake=0.12, deck=0.05,
                                frame=hood_frame, ends=(False, True))
    kit.shingles(m, h_front, piece=(0.7, 1.0))
    kit.shingles(m, h_back, piece=(0.7, 1.0))
    kit.ridge_cap(m, h_front, h_back, uv='C', width=0.1, thick=0.04)
    m.section('door hood')

    # --- The bay window: a stone footing, timber posts, plaster under three windows and a lean-to roof. ---
    kit.stone_stack(m, ((BX0 + BX1) * 0.5, Y0 - BDEPTH * 0.5 + 0.02, 0.0), (BX1 - BX0 + 0.12, BDEPTH + 0.1), BASE)
    bay_sides = [((BX0, Y0), (BX0, BY)), ((BX0, BY), (BX1, BY)), ((BX1, BY), (BX1, Y0))]
    for p0, p1 in bay_sides:
        space = kit.wall_space(p0, p1, BASE)
        length = (kit.Vector(p1) - kit.Vector(p0)).length
        pane = Opening('window', 0.1, 0.95, length - 0.2, 1.2, panes=(max(1, round(length / 0.5)), 2), **window)
        m.panel(kit.rect(0.0, 0.0, length, 2.3), kit.holes([pane]), 0.1, PLASTER, space=space)
        kit.window(m, space, pane, 0.1)
        for x in (0.05, length - 0.05):
            timber(m, (x, 0.0, 0.0), (x, 0.0, 2.3), space, width=0.12)
        timber(m, (0.0, 0.0, 0.1), (length, 0.0, 0.1), space, lift=PROUD + 0.01, width=0.2)
        timber(m, (0.0, 0.0, 0.87), (length, 0.0, 0.87), space, lift=PROUD + 0.01, width=0.14)
        timber(m, (0.0, 0.0, 2.24), (length, 0.0, 2.24), space, lift=PROUD + 0.01, width=0.16)
    b_theta = math.atan2(0.28, BDEPTH)
    b_over = 0.15
    b_origin = kit.Vector((BX0 - b_over, BY - b_over, BASE + 2.32 - b_over * math.tan(b_theta)))
    b_origin += kit.Vector((0.0, -math.sin(b_theta), math.cos(b_theta))) * 0.05
    bay_roof = kit.Slope(b_origin, (1.0, 0.0, 0.0), (0.0, math.cos(b_theta), math.sin(b_theta)),
                         BX1 - BX0 + 2 * b_over, (BDEPTH + b_over) / math.cos(b_theta))
    kit.roof_deck(m, bay_roof, 0.05)
    kit.shingles(m, bay_roof, piece=(0.8, 1.2))
    m.section('bay window')


# --- Both cottages: the roof ---

def roof(m):
    """Steep shakes on a deck, a ridge board and a fieldstone chimney near the left gable, with SOCKET_Smoke on top.
    Returns the chimney's top and where it leaves the roof (for its hull)."""
    # The ridge runs front to back: the roof is built along X and turned a quarter (its right slope is the kit's front).
    right_roof, left_roof = kit.gable(m, Y0, Y1, -X1, -X0, EAVE, PITCH, overhang=OVERHANG, rake=RAKE, deck=0.1,
                                      sag=0.08, frame=kit.Matrix.Rotation(math.radians(90.0), 4, 'Z'))

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
    return top, roof_at_chimney


def body_hulls(m):
    """The walls' and the roof's collision."""
    m.hull((X1 - X0 + 0.14, Y1 - Y0 + 0.14, EAVE), at=(0.0, 0.0, EAVE * 0.5))
    eave_top = EAVE - OVERHANG * math.tan(theta) + 0.25
    m.hull_points([(x, y, eave_top) for x in (X0 - OVERHANG, X1 + OVERHANG) for y in (Y0 - RAKE, Y1 + RAKE)] +
                  [(0.0, y, EAVE + RISE + 0.3) for y in (Y0 - RAKE, Y1 + RAKE)])


def cottage(name='Cottage', ransom=False):
    """The cottage. ransom makes the settler's (Cottage_Ransom); without it, the tutorial island's, exactly as it was."""
    m = (SettlerModel if ransom else kit.Model)(name, seed=33)
    kit.plinth(m, X0, X1, Y0, Y1, BASE, out=0.06)
    m.section('base')
    if ransom:
        settler_walls(m)
        settler_front(m)
    else:
        timber_frame(m)
    top, roof_at_chimney = roof(m)

    # --- Collision: the walls, the roof, the bay and its roof (or the settler's stoop and things), the chimney. ---
    body_hulls(m)
    if ransom:
        settler_hulls(m)
    else:
        m.hull((BX1 - BX0 + 0.1, BDEPTH + 0.05, BASE + 2.4),
               at=((BX0 + BX1) * 0.5, Y0 - BDEPTH * 0.5, (BASE + 2.4) * 0.5))
    m.hull((0.64, 0.74, top - roof_at_chimney + 0.4), at=(CHIMNEY[0], CHIMNEY[1], (roof_at_chimney + top) * 0.5))
    if not ransom:
        return m.finish()
    m.socket('Light', LAMP)
    return m.finish(view=(-1.0, -1.45, 0.5))


# --- The settler's cottage (Cottage_Ransom) ---
# Wall spaces as kit.wall_space gives them: x along the wall from its first corner (walls listed counterclockwise from
# above, the front first), z up from the top of the base, the outer face at y = 0 and the outside toward -y.

CASING = 0.11                                 # the window casings' width
# Every window a two-over-two sash in a teal casing; lit=True puts its glass on WindowGlow, with curtains behind.
SASH_W, SASH_H, SASH_Z = 0.82, 1.3, 0.85
DOOR_MID = DOOR.x + DOOR.w * 0.5               # along the front wall (m from its left corner)
SETTLER_WALLS = [
    # (p0, p1, gable end, openings in the wall, openings in its gable)
    ((X0, Y0), (X1, Y0), True,
     [DOOR, Opening('window', 2.95, SASH_Z, SASH_W, SASH_H, lit=True)],
     [Opening('window', 2.09, 3.35, 0.62, 0.7, lit=True, single=True)]),
    ((X1, Y0), (X1, Y1), False,
     [Opening('window', 1.25, SASH_Z, SASH_W, SASH_H, lit=True), Opening('window', 4.35, SASH_Z, SASH_W, SASH_H)],
     []),
    ((X1, Y1), (X0, Y1), True,
     [Opening('window', 0.9, SASH_Z, SASH_W, SASH_H), Opening('window', 3.08, SASH_Z, SASH_W, SASH_H)],
     [Opening('window', 2.09, 3.35, 0.62, 0.7, single=True)]),
    ((X0, Y1), (X0, Y0), False,
     [Opening('window', 1.25, SASH_Z, SASH_W, SASH_H), Opening('window', 4.35, SASH_Z, SASH_W, SASH_H, lit=True)],
     []),
]
# The stoop before the door (world coordinates): a fieldstone block under a plank deck 5 cm below the threshold, a stone
# step, two posts at its front corners under a header, and a shed roof of shakes from the wall over it.
STOOP_X0, STOOP_X1 = X0 + DOOR_MID - 0.95, X0 + DOOR_MID + 0.95
STOOP_Y = Y0 - 0.06 - 0.5                      # the deck's front edge: 0.5 m out from the plinth's face
DECK_TOP = BASE - 0.05
STEP_TOP, STEP_DEPTH, STEP_W = 0.2, 0.28, 1.2
POST_Y, POST_INSET, POST = STOOP_Y + 0.07, 0.07, 0.12
SHED_HIGH, SHED_PITCH, SHED_RUN = 3.2, 22.0, 0.8   # the shed roof's deck top at the wall, its pitch, how far out
# The bench under the front window, against the plinth, and the woodpile under the west eave (world coordinates).
BENCH_X, BENCH_LEN, BENCH_SEAT, BENCH_DEPTH = X0 + 2.95 + SASH_W * 0.5, 1.3, 0.44, 0.32
WOOD_Y0, WOOD_Y1, WOOD_DEPTH, WOOD_ROWS = 0.75, 2.35, 0.42, 4
# The front room's lamp, where the level's dusk light goes: a metre behind the front wall, at a lamp's height, between
# the front window and the right one.
LAMP = (0.1, Y0 + 1.0, BASE + 1.5)
# Delia's house's lamplit panes (Farmhouse.py, WINDOW_*): the same material, so the two share one instance in Unreal.
WINDOW_COLOR, WINDOW_GLOW_DAY = 0x2e2219, 1.0
# Clean stretches of the plaster strip (G) along U, m (Farmhouse.py's CREAM_U): the curtains' old linen.
CREAM_U = (4.2, 4.3, 4.4, 4.5, 4.6, 5.45, 5.55, 5.65, 5.75, 5.85)
SETTLER = {
    'crepe': lambda: lt.material('Polymer', name='MourningCrepe', tint=0x161518),
    'windowglow': lambda: lm.material('WindowGlow', WINDOW_COLOR, Glow=WINDOW_GLOW_DAY, Variation=0.04),
}


class SettlerModel(kit.Model):
    """A kit Model that also knows the settler's cottage's materials (SETTLER)."""

    def slot(self, key):
        if key not in self.slots:
            make = SETTLER.get(key)
            self.slots[key] = (len(self.slots), make() if make else kit._material(key))
        return self.slots[key][0]


def shell_panel(m, outline, holes, thick, uv, space, around=(), slices=1.2):
    """m.panel without its inner face: a wall of the closed shell, which nobody sees from inside (its door is shut and
    its glass is opaque), so only its outer face, edges and the reveals of its openings are kept. Sliced as m.panel
    slices it, for the baked shading."""
    tb = kit._prism([list(outline)] + [list(h) for h in holes], thick)
    xs = [x for x, z in outline]
    zs = [z for x, z in outline]
    cut_x, cut_z = [], set()
    if slices and not uv.rotate:
        cut_x += kit.frange(min(xs) + slices, max(xs) - 0.1, slices)
    for o in around:
        if o.z > 0.15:
            cut_z.add(round(o.z - 0.1, 2))
        if not uv.rotate:
            cut_x += [o.x - 0.2, o.x + o.w + 0.2]
    kit._slice(tb, 0, [x for x in cut_x if min(xs) + 0.05 < x < max(xs) - 0.05])
    kit._slice(tb, 2, [z for z in sorted(cut_z) if min(zs) + 0.05 < z < max(zs) - 0.05])
    tb.normal_update()
    lo, hi = min(zs), max(zs)

    def hidden(f):
        # The inner face; the bottom, on the base; the top, under the roof deck (or under the gable's boards).
        flat = [v.co.z for v in f.verts]
        return (f.normal.y > 0.99 or f.normal.z < -0.99 and max(flat) < lo + 1e-4
                or f.normal.z > 0.99 and min(flat) > hi - 1e-4)
    bmesh.ops.delete(tb, geom=[f for f in tb.faces if hidden(f)], context='FACES_ONLY')
    m.emit(tb, uv, 'trim', None, space)


def settler_walls(m):
    """Clapboard walls with their sash windows and the door; on the gable ends a belt board at the wall plate and
    vertical boards above it up to the roof; hewn corner boards, a water table along the base and a frieze under the
    eaves."""
    rng = m.rng
    for p0, p1, gable_end, items, high in SETTLER_WALLS:
        space = kit.wall_space(p0, p1, BASE)
        length = (Vector(p1) - Vector(p0)).length
        shell_panel(m, kit.wall_outline(length, WALL, items), kit.holes(items), THICK,
                    Trim('A', world=True, u=rng.uniform(0.0, kit.U_REPEAT)), space, around=items)
        if gable_end:
            shell_panel(m, [(0.0, WALL), (length, WALL), (length * 0.5, WALL + RISE)], kit.holes(high), THICK,
                        Trim('A', world=True, rotate=True, u=rng.uniform(0.0, kit.U_REPEAT)), space, around=high)
        for o in items + high:
            if o.kind == 'door':
                kit.door(m, space, o, THICK)
            else:
                sash_window(m, space, o, THICK)
        # Corner boards: the eave walls' wrap past the corner over the gable walls' boards' edges.
        for x, w in ((0.07, 0.14), (length - 0.07, 0.14)) if gable_end else ((0.03, 0.18), (length - 0.03, 0.18)):
            kit.trim_board(m, space, (x, 0.0, -0.05), (x, 0.0, WALL), w, 'C', thick=0.04, lane=rng.randrange(4))
        spans = [(0.14, length - 0.14)]
        for o in items:
            if o.kind == 'door':
                spans = [(a, o.x - 0.2) for a, b in spans] + [(o.x + o.w + 0.2, b) for a, b in spans]
        for a, b in spans:
            kit.trim_board(m, space, (a, 0.0, 0.1), (b, 0.0, 0.1), 0.2, 'C', thick=0.045, lift=0.005)
        if gable_end:
            kit.trim_board(m, space, (0.0, 0.0, WALL), (length, 0.0, WALL), 0.2, 'C', thick=0.05, lift=0.01)
        else:
            kit.trim_board(m, space, (0.14, 0.0, WALL - 0.13), (length - 0.14, 0.0, WALL - 0.13), 0.22, 'C',
                           thick=0.035)
    m.section('walls, windows, door')


def sash_window(m, space, o, depth):
    """A two-over-two sash (or a single two-by-two sash, single=True) in a teal casing with a drip cap and a thick sill:
    the glass half way into the wall, the sashes' stiles and rails round it, a meeting rail and a bar down the middle.
    lit puts the glass on WindowGlow and hangs curtains behind the bars."""
    rng = m.rng
    x0, z0, w, h = o.x, o.z, o.w, o.h
    lit = o.opts.get('lit', False)
    cw = CASING
    for x in (x0 - cw * 0.5, x0 + w + cw * 0.5):
        kit.trim_board(m, space, (x, 0.0, z0 - 0.02), (x, 0.0, z0 + h + cw), cw, 'H1')
    kit.trim_board(m, space, (x0 - cw - 0.02, 0.0, z0 + h + cw * 0.65), (x0 + w + cw + 0.02, 0.0, z0 + h + cw * 0.65),
                   cw * 1.3, 'H1')
    m.box((w + 2.0 * cw + 0.12, 0.08, 0.04), at=(x0 + w * 0.5, -0.04, z0 + h + cw * 1.3 + 0.02), uv='C', space=space)
    m.box((w + 2.0 * cw + 0.1, 0.1 + depth * 0.5, 0.06), at=(x0 + w * 0.5, -0.05 + depth * 0.25, z0 - 0.03),
          rot=(rng.uniform(-1.5, 1.5), 0.0, 0.0), uv='C', space=space)
    glass_y = depth * 0.55
    m.box((w, 0.02, h), at=(x0 + w * 0.5, glass_y, z0 + h * 0.5), uv=Trim('H4', fit=True),
          mat='windowglow' if lit else 'trim', space=space)
    bar_y = glass_y - 0.03
    stile = 0.045
    for x in (x0 + stile * 0.5, x0 + w - stile * 0.5):
        m.board((x, bar_y, z0), (x, bar_y, z0 + h), stile, 0.04, uv='H1', space=space)
    for z, rail in ((z0 + 0.035, 0.07), (z0 + h - 0.025, 0.05)):
        m.board((x0 + stile, bar_y, z), (x0 + w - stile, bar_y, z), rail, 0.04, uv='H1', space=space)
    m.board((x0 + w * 0.5, bar_y - 0.004, z0 + 0.07), (x0 + w * 0.5, bar_y - 0.004, z0 + h - 0.05), 0.035, 0.035,
            uv='H1', space=space)
    if o.opts.get('single'):
        m.board((x0 + stile, bar_y - 0.004, z0 + h * 0.5), (x0 + w - stile, bar_y - 0.004, z0 + h * 0.5), 0.035,
                0.035, uv='H1', space=space)
    else:  # the meeting rail, where the lower sash closes against the upper
        m.board((x0 + stile, bar_y - 0.012, z0 + h * 0.5), (x0 + w - stile, bar_y - 0.012, z0 + h * 0.5), 0.055,
                0.05, uv='H1', space=space)
    if lit:
        curtains(m, space, o, depth)


class Cloth:
    """A curtain on the trim sheet's cream plaster strip G, which reads as old linen (Farmhouse.py's): U along the
    wall from a clean stretch of the strip (CREAM_U), V up the curtain fitted into the strip."""

    def __init__(self, u0):
        self.u0 = u0

    def apply(self, obj, rng):
        mesh = obj.data
        v0, v1 = lt.TRIM_STRIPS['G']
        inset = 2.0 / lt.TRIM_SIZE
        scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
        x_lo = min(v.co.x for v in mesh.vertices)
        z_lo = min(v.co.z for v in mesh.vertices)
        z_span = max(max(v.co.z for v in mesh.vertices) - z_lo, 1e-6)
        uv = mesh.uv_layers.active.data
        for loop in mesh.loops:
            co = mesh.vertices[loop.vertex_index].co
            uv[loop.index].uv = ((self.u0 + co.x - x_lo) * scale,
                                 v0 + inset + (co.z - z_lo) / z_span * (v1 - v0 - 2.0 * inset))


def sheet(m, rows, uv, mat='trim', space=None, facing=(0.0, -1.0, 0.0), smooth=True):
    """A one-sided cloth through a grid of points (rows of (x, y, z) in space), turned to face facing."""
    tb = kit._new_bmesh()
    grid = [[tb.verts.new(p) for p in row] for row in rows]
    for j in range(len(grid) - 1):
        for i in range(len(grid[j]) - 1):
            try:
                tb.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
            except ValueError:
                pass
    tb.normal_update()
    if sum((f.normal * f.calc_area() for f in tb.faces), Vector()).dot(Vector(facing)) < 0.0:
        bmesh.ops.reverse_faces(tb, faces=tb.faces[:])
    m.emit(tb, uv, mat, None, space, smooth=smooth)


def curtains(m, space, o, depth):
    """Two linen curtains behind the sash bars, gathered at the head, tied back a little under half way and falling to
    the sill, drawn aside to leave the middle of the window clear (Farmhouse.py's)."""
    rng = m.rng
    x0, z0, w, h = o.x, o.z, o.w, o.h
    glass = depth * 0.55 - 0.01
    for side in (-1.0, 1.0):
        edge = x0 - 0.012 if side < 0 else x0 + w + 0.012    # tucked behind the reveal
        cover = w * rng.uniform(0.3, 0.36) + 0.012
        rows = []
        for z, span, fold in ((z0 + h - 0.004, cover, 0.016), (z0 + h * 0.44, cover * 0.42, 0.007),
                              (z0 + 0.012, cover * 0.68, 0.012)):
            rows.append([(edge - side * span * i / 4.0, glass - 0.009 - (fold if i % 2 else 0.0), z) for i in range(5)])
        sheet(m, rows, Cloth(rng.choice(CREAM_U)), space=space)


def settler_front(m):
    """The stoop and its shed roof, the crepe on the door, the bench and the woodpile."""
    rng = m.rng
    front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
    mid_x = X0 + DOOR_MID
    # The stoop: a fieldstone block against the base, a deck of three planks on it and a stone step.
    kit.stone_stack(m, (mid_x, (STOOP_Y + Y0 - 0.02) * 0.5, 0.0), (STOOP_X1 - STOOP_X0, Y0 - 0.02 - STOOP_Y),
                    DECK_TOP - 0.05, bevel=0.02)
    planks = 3
    plank = (Y0 - 0.06 - STOOP_Y) / planks
    for k in range(planks):
        y = STOOP_Y + plank * (k + 0.5)
        end = STOOP_X1 + 0.03 + rng.uniform(-0.02, 0.02)
        m.board((STOOP_X0 - 0.03, y, DECK_TOP - 0.025), (end, y, DECK_TOP - 0.025), plank - 0.008, 0.05,
                face=(0.0, 0.0, 1.0), uv='A')
    kit.stone_stack(m, (mid_x, STOOP_Y - STEP_DEPTH * 0.5 + 0.03, 0.0), (STEP_W, STEP_DEPTH + 0.06), STEP_TOP,
                    bevel=0.02)
    # Two posts on the deck's front corners, a header across them with knee braces, and a ledger on the wall.
    slope_t = math.tan(math.radians(SHED_PITCH))
    under = SHED_HIGH - (Y0 - POST_Y) * slope_t - 0.05 / math.cos(math.radians(SHED_PITCH))
    header_h = 0.14
    post_top = under - header_h
    posts = (STOOP_X0 + POST_INSET, STOOP_X1 - POST_INSET)
    for x in posts:
        m.board((x, POST_Y, DECK_TOP), (x, POST_Y, post_top + 0.02), POST, POST, uv=Trim('C', lane='each'))
    m.board((STOOP_X0 - 0.02, POST_Y, under - header_h * 0.5), (STOOP_X1 + 0.02, POST_Y, under - header_h * 0.5),
            header_h, 0.12, uv=Trim('C', lane='each'), bevel=0.008)
    for x, sign in zip(posts, (1.0, -1.0)):
        m.board((x + sign * POST * 0.5, POST_Y, post_top - 0.36), (x + sign * 0.36, POST_Y, post_top + 0.01), 0.08,
                0.07, uv='C')
    # The shed roof: its deck rests on the ledger at the wall and on the header, its covering the main roof's shakes.
    s_theta = math.radians(SHED_PITCH)
    s_width = STOOP_X1 - STOOP_X0 + 0.2
    origin = Vector((STOOP_X0 - 0.1, Y0 - SHED_RUN, SHED_HIGH - SHED_RUN * slope_t))
    shed = kit.Slope(origin, (1.0, 0.0, 0.0), (0.0, math.cos(s_theta), math.sin(s_theta)), s_width,
                     SHED_RUN / math.cos(s_theta) - 0.02)
    kit.roof_deck(m, shed, 0.05)
    for x in (0.12, s_width * 0.5, s_width - 0.12):  # rafters under the deck, from over the header to the wall
        m.board((x, 0.04, -0.09), (x, shed.length, -0.09), 0.08, 0.05, face=(1.0, 0.0, 0.0), uv='C', space=shed)
    kit.shingles(m, shed, piece=(0.7, 1.0))
    kit.trim_board(m, front, (STOOP_X0 - 0.1 - X0, 0.0, SHED_HIGH - BASE - 0.02),
                   (STOOP_X1 + 0.1 - X0, 0.0, SHED_HIGH - BASE - 0.02), 0.16, 'C', thick=0.06)
    m.section('stoop and shed roof')

    crepe_bow(m, front)
    m.section('crepe')

    # The bench under the front window: a two-plank seat on slab legs, against the base.
    seat_y = Y0 - 0.06 - BENCH_DEPTH * 0.5 - 0.01
    for dy in (-0.075, 0.075):
        m.board((BENCH_X - BENCH_LEN * 0.5, seat_y + dy, BENCH_SEAT - 0.025),
                (BENCH_X + BENCH_LEN * 0.5, seat_y + dy + rng.uniform(-0.004, 0.004), BENCH_SEAT - 0.025),
                0.15, 0.045, face=(0.0, 0.0, 1.0), uv='A')
    for x in (BENCH_X - BENCH_LEN * 0.5 + 0.16, BENCH_X + BENCH_LEN * 0.5 - 0.16):
        m.box((0.05, BENCH_DEPTH - 0.04, BENCH_SEAT - 0.05), at=(x, seat_y, (BENCH_SEAT - 0.05) * 0.5),
              uv=Trim('C', lane='each'))
    m.board((BENCH_X - BENCH_LEN * 0.5 + 0.16, seat_y, 0.16), (BENCH_X + BENCH_LEN * 0.5 - 0.16, seat_y, 0.16), 0.08,
            0.04, face=(0.0, 0.0, 1.0), uv='C')
    woodpile(m)
    m.section('bench, woodpile')


def crepe_bow(m, front):
    """Black crepe hung on the door, as a house in mourning hangs it: a bow at eye height on its nail, two puffed
    loops either side of a gathered knot, and two long tails falling from it with their ends cut in a swallowtail.
    All of it stands clear of the door's battens and brace."""
    cx, cz = DOOR_MID + 0.02, 1.47
    face = -0.024                       # just proud of the battens and the brace
    for side in (-1.0, 1.0):
        rows = []
        for j in range(5):
            v = j / 4.0 * 2.0 - 1.0
            row = []
            for i in range(5):
                u = i / 4.0
                half = 0.026 + 0.05 * math.sin(math.pi * min(u * 1.15, 1.0) * 0.62)
                bulge = 0.034 * math.sin(math.pi * min(u, 0.999)) ** 0.6 * (1.0 - v * v) + 0.012 * (1.0 - u)
                rim = 1.0 if i == 4 else 0.0
                row.append((cx + side * (0.024 + 0.15 * u), face - bulge * (1.0 - 0.6 * rim),
                            cz + v * half * (1.0 - 0.25 * rim) + 0.012 * u))
            rows.append(row)
        sheet(m, rows, Tile('Polymer'), mat='crepe', space=front)
    knot = [(0.0, 0.03), (0.022, 0.028), (0.034, 0.016), (0.036, 0.0), (0.0, 0.0)]
    from_door = Matrix.Rotation(math.radians(90.0), 4, 'X')
    lathe(m, knot, at=(cx, face + 0.002, cz), uv=Tile('Polymer'), mat='crepe', space=front, axis=from_door)
    for k, (drift, length) in enumerate(((-0.07, 0.6), (0.05, 0.52))):
        rows = []
        steps = 5
        for j in range(steps + 1):
            v = j / steps
            z = cz - 0.02 - length * v
            xc = cx + (0.008 if k else -0.008) + drift * v + 0.006 * math.sin(v * 5.0 + k)
            y = face - 0.006 - 0.005 * math.sin(v * 4.0 + k)
            width = 0.03 + 0.008 * v
            notch = 0.045 if j == steps else 0.0
            rows.append([(xc - width, y, z), (xc, y - 0.003, z + notch), (xc + width, y, z)])
        sheet(m, rows, Tile('Polymer'), mat='crepe', space=front)


def lathe(m, profile, at=(0.0, 0.0, 0.0), sides=8, uv=None, mat='trim', space=None, axis=None, smooth=True):
    """A turned part around a vertical axis through at: profile [(radius, z), ...] in order (a radius of 0 closes an
    end); axis (a matrix) turns it (Farmhouse.py's)."""
    tb = kit._new_bmesh()
    rings = []
    for r, z in profile:
        if r <= 1e-6:
            rings.append([tb.verts.new((0.0, 0.0, z))])
        else:
            rings.append([tb.verts.new((r * math.cos(2.0 * math.pi * k / sides),
                                        r * math.sin(2.0 * math.pi * k / sides), z)) for k in range(sides)])
    for r0, r1 in zip(rings, rings[1:]):
        for k in range(sides):
            k1 = (k + 1) % sides
            if len(r0) == 1 and len(r1) == 1:
                continue
            if len(r0) == 1:
                face = (r0[0], r1[k1], r1[k])
            elif len(r1) == 1:
                face = (r0[k], r0[k1], r1[0])
            else:
                face = (r0[k], r0[k1], r1[k1], r1[k])
            try:
                tb.faces.new(face)
            except ValueError:
                pass
    bmesh.ops.recalc_face_normals(tb, faces=tb.faces[:])
    matrix = Matrix.Translation(Vector(at)) @ (axis if axis is not None else Matrix.Identity(4))
    m.emit(tb, uv, mat, matrix, space, smooth=smooth)


def woodpile(m):
    """Firewood stacked against the west wall's base under the eave, between two stakes: rows of billets (strip C's
    hewn faces), their sawn ends out, most of them split (wedges and quarters turned every way), a few still round,
    each a little longer or shorter than the next."""
    rng = m.rng
    x_face = X0 - 0.06                  # the plinth's face
    r = (WOOD_Y1 - WOOD_Y0) / 20.0
    for row in range(WOOD_ROWS):
        count = 10 - (1 if row % 2 else 0)
        y_start = WOOD_Y0 + r * (2 if row % 2 else 1)
        for k in range(count):
            if row == WOOD_ROWS - 1 and rng.random() < 0.3:
                continue
            y = y_start + k * 2.0 * r + rng.uniform(-0.012, 0.012)
            z = r + row * r * 1.75 + rng.uniform(-0.01, 0.006)
            out = WOOD_DEPTH + rng.uniform(-0.06, 0.03)
            kind = rng.random()
            if kind < 0.45:
                sides, radius = 3, r * 1.12
            elif kind < 0.75:
                sides, radius = 4, r * 0.98
            else:
                sides, radius = 6, r * rng.uniform(0.8, 0.95)
            m.cylinder((x_face - 0.02, y, z), (x_face - out, y, z + rng.uniform(-0.01, 0.01)), radius, sides=sides,
                       uv=Trim('C'), face=(0.0, -1.0, 0.0), phase=rng.uniform(0.0, 360.0), caps=(False, True),
                       cap_uv=kit.EndGrain(), cap_mat='endgrain')
    for y in (WOOD_Y0 - 0.03, WOOD_Y1 + 0.03):
        m.board((x_face - WOOD_DEPTH * 0.5, y, 0.0), (x_face - WOOD_DEPTH * 0.5, y, WOOD_ROWS * r * 1.75 + 0.12),
                0.05, 0.05, face=(-1.0, 0.0, 0.0), uv='C')


def settler_hulls(m):
    """The stoop's deck and step (walkable), its posts, the bench and the woodpile."""
    m.hull((STOOP_X1 - STOOP_X0, Y0 - 0.02 - STOOP_Y, DECK_TOP), at=((STOOP_X0 + STOOP_X1) * 0.5,
                                                                     (STOOP_Y + Y0 - 0.02) * 0.5, DECK_TOP * 0.5))
    m.hull((STEP_W, STEP_DEPTH, STEP_TOP), at=(X0 + DOOR_MID, STOOP_Y - STEP_DEPTH * 0.5, STEP_TOP * 0.5))
    slope_t = math.tan(math.radians(SHED_PITCH))
    height = SHED_HIGH - (Y0 - POST_Y) * slope_t - DECK_TOP
    for x in (STOOP_X0 + POST_INSET, STOOP_X1 - POST_INSET):
        m.hull((POST, POST, height), at=(x, POST_Y, DECK_TOP + height * 0.5))
    m.hull((BENCH_LEN, BENCH_DEPTH, BENCH_SEAT), at=(BENCH_X, Y0 - 0.06 - BENCH_DEPTH * 0.5, BENCH_SEAT * 0.5))
    pile_h = WOOD_ROWS * (WOOD_Y1 - WOOD_Y0) / 20.0 * 1.75 + 0.05
    m.hull((WOOD_DEPTH, WOOD_Y1 - WOOD_Y0 + 0.1, pile_h),
           at=(X0 - 0.06 - WOOD_DEPTH * 0.5, (WOOD_Y0 + WOOD_Y1) * 0.5, pile_h * 0.5))


# --- The models ---

cottage()
if __name__ != '__overview__':            # the buildings' overview shows the tutorial island's cottage only
    cottage('Cottage_Ransom', ransom=True)
