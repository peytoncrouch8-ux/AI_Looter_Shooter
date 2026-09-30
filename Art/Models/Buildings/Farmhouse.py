"""The farmhouse at the farmstead (spawn): a one-and-a-half-storey plank house on a fieldstone foundation, weathered
vertical siding, a rusty corrugated tin roof with a dormer and a stovepipe, and a covered porch across the front with
a lantern by the door. The reference is the shack in Docs/Art/StyleTarget_Outpost.png.

A scripted model (Art/README.md) built with looter_buildings: the pivot is the middle of the footprint at ground level
and the porch faces the front (-Y). SOCKET_Smoke tops the stovepipe, SOCKET_Light is the lantern.
"""
import looter_buildings as kit
from looter_buildings import Opening, Trim

m = kit.Model('Farmhouse', seed=7)
rng = m.rng

X0, X1, Y0, Y1 = -4.2, 4.2, -3.1, 3.1   # outer faces of the walls
BASE = 0.6          # top of the foundation: the floor
WALL = 3.5          # siding height up to the eave (a storey and a half)
EAVE = BASE + WALL
PITCH = 40.0
THICK = 0.14
W, D = X1 - X0, Y1 - Y0
RISE = (D * 0.5) * kit.math.tan(kit.math.radians(PITCH))
SIDING = Trim('A', world=True, rotate=True)  # vertical boards

kit.plinth(m, X0, X1, Y0, Y1, BASE)

m.section('foundation')

# --- Walls, counterclockwise from above: front, right, back, left. ---
DOOR = Opening('door', W * 0.5 - 0.5, 0.0, 1.0, 2.15, hinge='left')
walls = [
    ((X0, Y0), (X1, Y0), None, [
        Opening('window', 1.15, 0.85, 0.9, 1.3, style='boarded'),
        DOOR,
        Opening('window', 6.35, 0.85, 0.9, 1.3, style='shutters', paint='H1', sash='H1')]),
    ((X1, Y0), (X1, Y1), (D * 0.5, WALL + RISE), [
        Opening('window', 2.65, 0.85, 0.9, 1.3, style='glass', paint='H1', panes=(2, 3)),
        Opening('window', 2.72, 3.75, 0.76, 0.9, style='boarded')]),
    ((X1, Y1), (X0, Y1), None, [
        Opening('window', 1.6, 0.9, 0.8, 1.2, style='shutters', paint='H2', sash='A'),
        Opening('window', 6.0, 0.9, 0.8, 1.2, style='glass', paint='A', panes=(2, 2))]),
    ((X0, Y1), (X0, Y0), (D * 0.5, WALL + RISE), [
        Opening('window', 1.8, 0.85, 0.9, 1.3, style='shutters', paint='H1', sash='H1'),
        Opening('window', 2.72, 3.75, 0.76, 0.9, style='glass', paint='A', panes=(1, 2))]),
]
for p0, p1, apex, items in walls:
    space = kit.wall_space(p0, p1, BASE)
    length = (kit.Vector(p1) - kit.Vector(p0)).length
    m.panel(kit.wall_outline(length, WALL, items, apex), kit.holes(items), THICK,
            Trim('A', world=True, rotate=True, u=rng.uniform(0.0, 6.4)), space=space, around=items)
    kit.openings(m, space, items, THICK)
    gable_end = apex is not None
    # Corner boards (the front and back ones wrap over the side walls' edges), the water table along the foundation
    # (broken by the door) and, on the eave walls, a frieze under the soffit.
    top = WALL if not gable_end else WALL - 0.02
    for x, w in ((0.07 - (0.0 if gable_end else 0.04), 0.14 + (0.0 if gable_end else 0.08) * 0.5),
                 (length - 0.07 + (0.0 if gable_end else 0.04), 0.14 + (0.0 if gable_end else 0.08) * 0.5)):
        kit.trim_board(m, space, (x, 0.0, -0.05), (x, 0.0, top), w, 'C', thick=0.04, lane=rng.randrange(4))
    doors = [o for o in items if o.kind == 'door']
    spans = [(0.1, length - 0.1)]
    for o in doors:
        spans = [(a, o.x - 0.2) for a, b in spans] + [(o.x + o.w + 0.2, b) for a, b in spans]
    for a, b in spans:
        kit.trim_board(m, space, (a, 0.0, 0.1), (b, 0.0, 0.1), 0.2, 'C', thick=0.045, lift=0.01)
    if not gable_end:
        kit.trim_board(m, space, (0.12, 0.0, WALL - 0.14), (length - 0.12, 0.0, WALL - 0.14), 0.24, 'A', thick=0.035)
    else:
        # A few horizontal boards nailed over rot low on the gable walls.
        x = rng.uniform(0.6, length - 1.6)
        for k in range(2):
            kit.trim_board(m, space, (x + rng.uniform(-0.05, 0.05), 0.0, 0.32 + k * 0.2),
                           (x + 0.9 + rng.uniform(-0.1, 0.1), 0.0, 0.34 + k * 0.2), 0.19, 'A', thick=0.03, lift=0.035)

# A patch over the front siding beside the door, and a loose board.
front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
for k in range(3):
    kit.trim_board(m, front, (5.0, 0.0, 0.35 + k * 0.2), (5.85 + rng.uniform(-0.08, 0.08), 0.0, 0.35 + k * 0.2 + 0.02),
                   0.19, 'A', thick=0.03, lift=0.035)
m.board((2.95, -0.05, 0.2), (3.05, -0.09, 1.35), 0.19, 0.03, uv='A', space=front)

m.section('walls, windows, doors')

# --- Main roof: tin over a sagging deck, with a dormer in front and the stovepipe. ---
front_roof, back_roof = kit.gable(m, X0, X1, Y0, Y1, EAVE, PITCH, overhang=0.45, rake=0.35, deck=0.12, sag=0.09)
cos_p = kit.math.cos(kit.math.radians(PITCH))
DORMER_Y, DORMER_HALF = -2.75, 0.9


def under_dormer(x, y):
    # Slope coordinates to the house's: the front slope starts 0.45 m out from the wall and 0.35 m past the corner.
    wx, wy = x + X0 - 0.35, Y0 - 0.45 + y * cos_p
    return abs(wx) < DORMER_HALF - 0.5 and DORMER_Y + 0.6 < wy < -0.75
kit.tin(m, front_roof, skip=under_dormer)
kit.tin(m, back_roof)
kit.ridge_cap(m, front_roof, back_roof)

m.section('main roof')

# The dormer: a small gable facing front, its ridge running back into the main roof.
tan_p = kit.math.tan(kit.math.radians(PITCH))
D_EAVE = 5.5
D_BASE = 4.25
dormer_frame = kit.Matrix.Translation((0.0, DORMER_Y, 0.0)) @ kit.Matrix.Rotation(kit.math.radians(90.0), 4, 'Z')
d_front, d_back = kit.gable(m, 0.0, 2.3, -DORMER_HALF, DORMER_HALF, D_EAVE, PITCH, overhang=0.12, rake=0.18, deck=0.08,
                            sag=0.0, ends=(True, False), frame=dormer_frame)
kit.tin(m, d_front, sheets=(1, 2))
kit.tin(m, d_back, sheets=(1, 2))
kit.ridge_cap(m, d_front, d_back)
d_rise = DORMER_HALF * tan_p
d_window = Opening('window', DORMER_HALF - 0.36, 0.52, 0.72, 0.62, style='glass', paint='H1', sash='H1', panes=(2, 2))
d_space = kit.wall_space((-DORMER_HALF, DORMER_Y), (DORMER_HALF, DORMER_Y), D_BASE)
m.panel(kit.wall_outline(2 * DORMER_HALF, D_EAVE - D_BASE, [], (DORMER_HALF, D_EAVE - D_BASE + d_rise)),
        kit.holes([d_window]), 0.1, SIDING, space=d_space, around=[d_window])
kit.openings(m, d_space, [d_window], 0.1)
for x in (0.06, 2 * DORMER_HALF - 0.06):
    kit.trim_board(m, d_space, (x, 0.0, 0.2), (x, 0.0, D_EAVE - D_BASE - 0.02), 0.12, 'C', thick=0.035)
# Cheek walls from the dormer's face back until they vanish into the roof.
cheek = 1.55
for p0, p1 in (((DORMER_HALF, DORMER_Y), (DORMER_HALF, DORMER_Y + cheek)),
               ((-DORMER_HALF, DORMER_Y + cheek), (-DORMER_HALF, DORMER_Y))):
    space = kit.wall_space(p0, p1, D_BASE)
    m.panel(kit.rect(0.0, 0.0, cheek, D_EAVE - D_BASE), (), 0.1, Trim('A', world=True, rotate=True), space=space)

m.section('dormer')

# The stovepipe, through the front slope, with a rain cap; smoke comes out under the cap.
PIPE = (2.75, -1.25)
roof_z = EAVE + (PIPE[1] - Y0) * tan_p
pipe_top = 7.55
m.cylinder((PIPE[0], PIPE[1], roof_z - 0.2), (PIPE[0], PIPE[1], pipe_top), 0.1, sides=10, uv=kit.Tile('MetalRust'),
           mat='metal', caps=(False, True))
m.cylinder((PIPE[0], PIPE[1], pipe_top - 0.35), (PIPE[0], PIPE[1], pipe_top - 0.28), 0.125, sides=10,
           uv=kit.Tile('MetalRust'), mat='metal')
m.cylinder((PIPE[0], PIPE[1], pipe_top + 0.12), (PIPE[0], PIPE[1], pipe_top + 0.3), 0.26, 0.03, sides=10,
           uv=kit.Tile('MetalRust'), mat='metal')
for k in range(3):
    a = kit.math.radians(90.0 + 120.0 * k)
    x, y = PIPE[0] + 0.09 * kit.math.cos(a), PIPE[1] + 0.09 * kit.math.sin(a)
    m.box((0.02, 0.02, 0.16), at=(x, y, pipe_top + 0.06), uv=kit.Tile('MetalRust'), mat='metal')
# A flashing plate where the pipe goes through the tin.
pipe_slope_x = PIPE[0] - (X0 - 0.35)
pipe_slope_y = (PIPE[1] - (Y0 - 0.45)) / cos_p
m.box((0.55, 0.6, 0.02), at=(pipe_slope_x, pipe_slope_y, 0.06), rot=(0.0, 0.0, 3.0), uv='F', space=front_roof)
m.socket('Smoke', (PIPE[0], PIPE[1], pipe_top + 0.08))

m.section('stovepipe')

# --- Porch: plank floor on stone piers, posts, a beam with knee braces and a low tin roof. ---
PX0, PX1, PY = -3.6, 3.6, Y0 - 2.2
for y in kit.frange(Y0 - 0.1, PY, -0.2):
    m.board((PX0 - rng.uniform(0.0, 0.04), y, BASE - 0.025), (PX1 + rng.uniform(0.0, 0.04), y, BASE - 0.025), 0.19,
            0.05, face=(0.0, 0.0, 1.0), uv='A')
m.board((PX0, PY + 0.05, BASE - 0.17), (PX1, PY + 0.05, BASE - 0.17), 0.24, 0.07, uv='C')
for x in (PX0 - 0.03, PX1 + 0.03):
    m.board((x, PY, BASE - 0.17), (x, Y0, BASE - 0.17), 0.24, 0.07, face=(1.0 if x > 0 else -1.0, 0.0, 0.0), uv='C')
POSTS = [-3.45, -1.15, 1.15, 3.45]
PY_POST = PY + 0.17
for x in POSTS:
    kit.stone_stack(m, (x, PY_POST, 0.0), (0.4, 0.4), BASE - 0.2, taper=0.1, rot=rng.uniform(-8.0, 8.0), bevel=0.0)
    m.board((x, PY_POST, BASE), (x, PY_POST, 3.0), 0.16, 0.16, uv=Trim('C', lane='each'), bevel=0.015)
    m.box((0.22, 0.22, 0.05), at=(x, PY_POST, BASE + 0.025), uv='C')
BEAM_Z = 3.1
m.board((PX0 - 0.15, PY_POST, BEAM_Z), (PX1 + 0.15, PY_POST, BEAM_Z), 0.2, 0.17, uv=Trim('C', lane='each'),
        bevel=0.015)
for x in POSTS:
    for side in (-1.0, 1.0):
        if (x < 0 and side < 0 and x == POSTS[0]) or (x > 0 and side > 0 and x == POSTS[-1]):
            continue
        m.board((x + side * 0.06, PY_POST, 2.5), (x + side * 0.55, PY_POST, 3.0), 0.1, 0.09, uv='C')
# Steps up to the door.
for k, (top, y) in enumerate(((0.4, PY - 0.15), (0.2, PY - 0.45))):
    m.board((-0.8, y, top - 0.025), (0.8, y, top - 0.025), 0.3, 0.05, face=(0.0, 0.0, 1.0), uv='A')
    m.board((-0.78, y - 0.13, top * 0.5 - 0.02), (0.78, y - 0.13, top * 0.5 - 0.02), top - 0.05, 0.03, uv='A')
for x in (-0.82, 0.82):
    m.board((x, PY - 0.62, 0.02), (x, PY + 0.02, BASE - 0.05), 0.18, 0.05, face=(1.0 if x > 0 else -1.0, 0.0, 0.0),
            uv='C')

m.section('porch')

# The porch roof: a shed slope from the wall down onto the beam and past it.
P_THETA = kit.math.atan2(3.55 - (BEAM_Z + 0.1), Y0 - PY_POST)
p_deck = 0.08
p_normal = kit.Vector((0.0, -kit.math.sin(P_THETA), kit.math.cos(P_THETA)))
p_over = 0.35
p_origin = kit.Vector((PX0 - 0.25, PY_POST - p_over, BEAM_Z + 0.1 - p_over * kit.math.tan(P_THETA))) + p_normal * p_deck
p_len = (Y0 - (PY_POST - p_over)) / kit.math.cos(P_THETA)
porch_roof = kit.Slope(p_origin, (1.0, 0.0, 0.0), (0.0, kit.math.cos(P_THETA), kit.math.sin(P_THETA)),
                       PX1 - PX0 + 0.5, p_len, sag=0.06)
kit.roof_deck(m, porch_roof, p_deck)
kit.tin(m, porch_roof, top=p_len - 0.03)
m.box((porch_roof.width + 0.02, 0.2, 0.05), at=(porch_roof.width * 0.5, p_len - 0.1, 0.05), rot=(-6.0, 0.0, 0.0),
      uv='F', space=porch_roof, cuts=6)

m.section('porch roof')

# A lantern on an iron bracket beside the door.
LANTERN = (DOOR.x + DOOR.w + 0.45 + X0, Y0 - 0.34, BASE + 2.02)
m.box((0.12, 0.03, 0.3), at=(LANTERN[0], Y0 - 0.015, LANTERN[2] + 0.32), uv=kit.Tile('MetalRust'), mat='metal')
m.box((0.03, 0.36, 0.03), at=(LANTERN[0], Y0 - 0.18, LANTERN[2] + 0.43), uv=kit.Tile('MetalRust'), mat='metal')
m.board((LANTERN[0], Y0 - 0.02, LANTERN[2] + 0.22), (LANTERN[0], Y0 - 0.26, LANTERN[2] + 0.42), 0.025, 0.025,
        face=(1.0, 0.0, 0.0), uv=kit.Tile('MetalRust'), mat='metal')
kit.lantern(m, LANTERN, hang=0.0)
m.box((0.02, 0.02, 0.1), at=(LANTERN[0], LANTERN[1], LANTERN[2] + 0.37), uv=kit.Tile('MetalRust'), mat='metal')

m.section('lantern')

# --- Collision: the house body, the roof, the dormer, the porch, its steps, posts and roof. ---
m.hull((W + 0.16, D + 0.16, EAVE), at=(0.0, 0.0, EAVE * 0.5))
eave_top = EAVE - 0.45 * tan_p + 0.2
ridge_top = EAVE + RISE + 0.22
m.hull_points([(x, y, eave_top) for x in (X0 - 0.35, X1 + 0.35) for y in (Y0 - 0.45, Y1 + 0.45)] +
              [(x, 0.0, ridge_top) for x in (X0 - 0.35, X1 + 0.35)])
m.hull_points([(x, y, z) for x in (-DORMER_HALF - 0.2, DORMER_HALF + 0.2) for y in (DORMER_Y - 0.15, DORMER_Y + 1.6)
               for z in (D_BASE, D_EAVE)] + [(0.0, y, D_EAVE + d_rise + 0.15) for y in (DORMER_Y - 0.15, -0.3)])
m.hull((PX1 - PX0 + 0.08, Y0 - PY + 0.05, BASE), at=(0.0, (Y0 + PY) * 0.5, BASE * 0.5))
m.hull((1.64, 0.3, 0.4), at=(0.0, PY - 0.15, 0.2))
m.hull((1.64, 0.3, 0.2), at=(0.0, PY - 0.45, 0.1))
for x in POSTS:
    m.hull((0.18, 0.18, 3.0 - BASE), at=(x, PY_POST, (BASE + 3.0) * 0.5))
m.hull((porch_roof.width, p_len, 0.2), at=(porch_roof.width * 0.5, p_len * 0.5, -0.02), space=porch_roof)

m.finish()
