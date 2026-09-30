"""The log cabin at the village crossroads: round logs with saddle-notched corners that stick out past each other, a
board gable under a shake roof carried on log purlins, a fieldstone chimney up the left gable end and a small porch in
front of the door.

A scripted model (Art/README.md) built with looter_buildings: the pivot is the middle of the footprint at ground level
and the door faces the front (-Y). SOCKET_Smoke tops the chimney.
"""
import looter_buildings as kit
from looter_buildings import Opening, Trim, math

m = kit.Model('LogCabin', seed=21)
rng = m.rng

HX, HY = 3.2, 2.6       # log centerlines: the front and back walls at y = -HY and HY, the gable walls at x = -HX, HX
R = 0.2                  # log radius: strip B's logs are 40 cm
BASE = 0.3               # top of the stone footing
OVER = 0.4               # how far the logs run past the corners
FRONT_COURSES, SIDE_COURSES = 8, 7
PITCH = 35.0
EAVE = BASE + FRONT_COURSES * 2 * R      # the roof rests on the top front and back logs
OUT_Y, OUT_X = HY + R, HX + R            # outer faces of the walls

# Stone footing under the logs.
kit.plinth(m, -OUT_X + 0.05, OUT_X - 0.05, -OUT_Y + 0.05, OUT_Y - 0.05, BASE, out=0.02)
m.section('footing')

# --- Openings, in each wall's own frame (x from its left end seen from outside, z up from the footing). ---
DOOR = Opening('door', 1.9, 0.0, 1.0, 2.0, hinge='right')
openings = {
    'front': [DOOR, Opening('window', 4.2, 0.8, 0.8, 0.8, style='glass', panes=(2, 2))],
    'right': [Opening('window', 2.2, 1.0, 0.8, 0.8, style='glass', paint='H2', panes=(2, 2))],
    'back': [Opening('window', 2.4, 0.8, 0.8, 0.8, style='boarded')],
    'left': [],
}
# Each wall: its start and end corners (log centerlines, counterclockwise from above), outward direction and the
# height of its first log's center.
walls = {
    'front': ((-HX, -HY), (HX, -HY), (0.0, -1.0, 0.0), BASE + R, FRONT_COURSES),
    'right': ((HX, -HY), (HX, HY), (1.0, 0.0, 0.0), BASE + 2 * R, SIDE_COURSES),
    'back': ((HX, HY), (-HX, HY), (0.0, 1.0, 0.0), BASE + R, FRONT_COURSES),
    'left': ((-HX, HY), (-HX, -HY), (-1.0, 0.0, 0.0), BASE + 2 * R, SIDE_COURSES),
}
for name, (p0, p1, out, z_first, courses) in walls.items():
    p0v, p1v = kit.Vector((p0[0], p0[1], 0.0)), kit.Vector((p1[0], p1[1], 0.0))
    along = (p1v - p0v).normalized()
    length = (p1v - p0v).length
    # Wall frame: x measured from the outer corner (p0 minus the log's radius), as in the opening list.
    outer = kit.Vector(out) * R
    for k in range(courses):
        z = z_first + k * 2 * R
        rel = z - BASE  # height in the wall frame
        start = -OVER + rng.uniform(-0.05, 0.05)
        end = length + 2 * R + OVER + rng.uniform(-0.05, 0.05)
        # Split the log around openings it crosses (its bottom below their top, its top above their bottom).
        pieces = [(start, end, True, True)]
        for o in openings[name]:
            if rel - R < o.z + o.h - 1e-3 and rel + R > o.z + 1e-3:
                a, b = o.x - 0.08, o.x + o.w + 0.08  # under the middle of the jambs
                split = []
                for s0, s1, c0, c1 in pieces:
                    if s1 <= a or s0 >= b:
                        split.append((s0, s1, c0, c1))
                        continue
                    if s0 < a:
                        split.append((s0, a, c0, False))
                    if s1 > b:
                        split.append((b, s1, False, c1))
                pieces = split
        for s0, s1, c0, c1 in pieces:
            q0 = p0v + along * (s0 - R)
            q1 = p0v + along * (s1 - R)
            radius = R + rng.uniform(-0.012, 0.008)
            kit.log(m, (q0.x, q0.y, z), (q1.x, q1.y, z), radius, out, r1=radius * rng.uniform(0.93, 1.0), caps=(c0, c1))
    if name in ('right', 'left'):
        # A squared sill under the side walls' first log, which sits half a log higher than the front's.
        sill_space = kit.wall_space(p0v + outer - along * R, p1v + outer + along * R, BASE)
        m.board((0.0, 0.1, 0.1), (length + 2 * R, 0.1, 0.1), 0.2, 0.2, uv=Trim('C', lane='each'), space=sill_space)
    # Openings: frames of squared jambs over the cut log ends, then the door or window itself.
    space = kit.wall_space(p0v + outer - along * R, p1v + outer + along * R, BASE)
    for o in openings[name]:
        depth = 2 * R
        for x in (o.x - 0.08, o.x + o.w + 0.08):
            m.board((x, 0.1, o.z - (0.06 if o.z > 0 else 0.0)), (x, 0.1, o.z + o.h + 0.12), 0.16, 0.2,
                    uv=Trim('C', lane='each'), space=space, lift=0.0)
        m.board((o.x - 0.16, 0.1, o.z + o.h + 0.07), (o.x + o.w + 0.16, 0.1, o.z + o.h + 0.07), 0.14, 0.22,
                uv=Trim('C', lane='each'), space=space)
        if o.kind == 'window':
            m.board((o.x - 0.2, 0.08, o.z - 0.05), (o.x + o.w + 0.2, 0.08, o.z - 0.05), 0.1, 0.3, uv='C', space=space)
            # Glass, muntins and the window's own look, in a thin frame set back into the logs.
            m.box((o.w, 0.02, o.h), at=(o.x + o.w * 0.5, depth * 0.55, o.z + o.h * 0.5), uv=Trim('H4', fit=True),
                  space=space)
            paint = o.opts.get('paint', 'A')
            m.board((o.x + o.w * 0.5, depth * 0.5, o.z), (o.x + o.w * 0.5, depth * 0.5, o.z + o.h), 0.045, 0.04,
                    uv=paint, space=space)
            m.board((o.x, depth * 0.5, o.z + o.h * 0.5), (o.x + o.w, depth * 0.5, o.z + o.h * 0.5), 0.045, 0.04,
                    uv=paint, space=space)
            if o.opts.get('style') == 'boarded':
                for k, sign in enumerate((1.0, -1.0)):
                    a = sign * math.radians(40.0 + rng.uniform(-6.0, 6.0))
                    d = kit.Vector((math.cos(a), 0.0, math.sin(a))) * 0.62
                    c = kit.Vector((o.x + o.w * 0.5, 0.0, o.z + o.h * 0.5))
                    m.board(c - d, c + d, 0.18, 0.035, space=space, lift=0.05 + k * 0.035)
        else:
            kit.door(m, space, o, depth)
m.section('log walls and openings')

# --- Gables: vertical boards between the top side log and the roof. ---
theta = math.radians(PITCH)
RISE = OUT_Y * math.tan(theta)
gable_base = BASE + 2 * R + (SIDE_COURSES - 1) * 2 * R  # middle of the top side log
for p0, p1 in (((HX + 0.05, -OUT_Y), (HX + 0.05, OUT_Y)), ((-HX - 0.05, OUT_Y), (-HX - 0.05, -OUT_Y))):
    space = kit.wall_space(p0, p1, 0.0)
    L = 2 * OUT_Y
    outline = [(0.0, gable_base), (L, gable_base), (L, EAVE), (L * 0.5, EAVE + RISE), (0.0, EAVE)]
    vent = Opening('window', L * 0.5 - 0.25, EAVE + 0.45, 0.5, 0.4, style='boarded')
    m.panel(outline, kit.holes([vent]), 0.1, Trim('A', world=True, rotate=True), space=space, around=[vent])
    m.box((0.5, 0.02, 0.4), at=(L * 0.5, 0.05, EAVE + 0.65), uv=Trim('H4', fit=True), space=space)
    for k, a in enumerate((rng.uniform(-8, 8), rng.uniform(80, 100))):
        r_ = math.radians(a)
        d = kit.Vector((math.cos(r_), 0.0, math.sin(r_))) * 0.36
        c = kit.Vector((L * 0.5, 0.0, EAVE + 0.65))
        m.board(c - d, c + d, 0.12, 0.03, space=space, lift=0.02 + 0.03 * k)
m.section('gables')

# --- Roof: shakes on a deck over log purlins that run out past the gables. ---
OVERHANG, RAKE = 0.5, 0.6
front, back = kit.gable(m, -OUT_X, OUT_X, -OUT_Y, OUT_Y, EAVE, PITCH, overhang=OVERHANG, rake=RAKE, deck=0.1, sag=0.1)
CHIMNEY = (-OUT_X - 0.45, 0.0)   # the chimney stack's middle: up the left gable end, through the rake at the ridge


def by_chimney(slope):
    def skip(x, y):
        world = slope.world((x, y, 0.0))
        return abs(world.x - CHIMNEY[0]) < 0.45 and abs(world.y - CHIMNEY[1]) < 0.45
    return skip
kit.shingles(m, front, skip=by_chimney(front))
kit.shingles(m, back, skip=by_chimney(back))
kit.ridge_cap(m, front, back, uv='C', width=0.18, thick=0.05)
m.section('roof')
for y in (-OUT_Y * 0.5, 0.0, OUT_Y * 0.5):
    r = 0.17 if y == 0.0 else 0.15
    z = EAVE + (OUT_Y - abs(y)) * math.tan(theta) - r / math.cos(theta)  # touching the deck's underside
    reach = OUT_X + RAKE - 0.05
    kit.log(m, (-reach, y, z), (reach, y, z), r, (0.0, -1.0, 0.0), sides=8)
m.section('purlins')

# --- Chimney: a fieldstone stack up the left gable, stepping in above the fireplace. ---
cx, cy = CHIMNEY
kit.stone_stack(m, (cx - 0.05, cy, 0.0), (1.0, 1.4), 2.6, taper=0.04, rot=0.0)
# The shoulder where the stack narrows, then the flue past the ridge.
m.emit(kit._box((1.0, 1.4, 0.5)), Trim('D', world=True), 'trim', kit.place((cx - 0.05, cy, 2.85)),
       shape=lambda co: kit.Vector((co.x * (1.0 - 0.35 * (co.z / 0.5 + 0.5)) + 0.08 * (co.z / 0.5 + 0.5),
                                    co.y * (1.0 - 0.45 * (co.z / 0.5 + 0.5)), co.z)))
top = EAVE + RISE + 0.9
kit.stone_stack(m, (cx + 0.02, cy, 3.1), (0.66, 0.78), top - 3.1, taper=0.03)
m.box((0.82, 0.94, 0.1), at=(cx + 0.02, cy, top + 0.05), uv=Trim('D', world=True), bevel=0.02)
m.box((0.42, 0.52, 0.16), at=(cx + 0.02, cy, top + 0.18), uv='D')
m.socket('Smoke', (cx + 0.02, cy, top + 0.28))
m.section('chimney')

# --- Porch: a plank deck on log sleepers, two log posts and a low shake roof tucked under the eave. ---
PX0, PX1 = DOOR.x - OUT_X - 0.9, DOOR.x + DOOR.w - OUT_X + 0.9   # the door's x in the house, padded
PY = -OUT_Y - 1.6
DECK = BASE - 0.02
for y in kit.frange(-OUT_Y - 0.1, PY, -0.2):
    m.board((PX0 - rng.uniform(0.0, 0.04), y, DECK - 0.025), (PX1 + rng.uniform(0.0, 0.04), y, DECK - 0.025), 0.19,
            0.05, face=(0.0, 0.0, 1.0), uv='A')
for x in (PX0 + 0.15, PX1 - 0.15):
    kit.log(m, (x, -OUT_Y, DECK - 0.15), (x, PY - 0.05, DECK - 0.15), 0.1, (1.0, 0.0, 0.0), sides=8)
mid = (PX0 + PX1) * 0.5
m.board((mid - 0.65, PY - 0.14, 0.09), (mid + 0.65, PY - 0.14, 0.09), 0.3, 0.14, face=(0.0, 0.0, 1.0), uv='C')
POST_Y = PY + 0.15
BEAM_Z = 2.6          # the beam log's axis; its top carries the porch roof
BEAM_TOP = BEAM_Z + 0.12
WALL_Z = 3.06         # where the porch roof meets the wall, under the main eave
for x in (PX0 + 0.15, PX1 - 0.15):
    kit.log(m, (x, POST_Y, DECK), (x, POST_Y, BEAM_Z), 0.11, (0.0, -1.0, 0.0), sides=8, caps=(False, False))
kit.log(m, (PX0 - 0.1, POST_Y, BEAM_Z), (PX1 + 0.1, POST_Y, BEAM_Z), 0.12, (0.0, -1.0, 0.0), sides=8)
p_theta = math.atan2(WALL_Z - BEAM_TOP, -OUT_Y - POST_Y)
p_over = 0.3
p_deck = 0.07
normal = kit.Vector((0.0, -math.sin(p_theta), math.cos(p_theta)))
origin = kit.Vector((PX0 - 0.3, POST_Y - p_over, BEAM_TOP - p_over * math.tan(p_theta))) + normal * p_deck
p_len = (-OUT_Y - (POST_Y - p_over)) / math.cos(p_theta)
porch_roof = kit.Slope(origin, (1.0, 0.0, 0.0), (0.0, math.cos(p_theta), math.sin(p_theta)), PX1 - PX0 + 0.6, p_len,
                       sag=0.04)
kit.roof_deck(m, porch_roof, p_deck)
kit.shingles(m, porch_roof, top=p_len - 0.02, piece=(1.0, 1.8))
m.section('porch')

# --- Collision: log walls with their corners, the gable roof, the chimney and the porch. ---
m.hull((2 * OUT_X, 2 * OUT_Y, EAVE), at=(0.0, 0.0, EAVE * 0.5))
for sx in (-1.0, 1.0):  # the log ends crossing at the corners
    for sy in (-1.0, 1.0):
        m.hull((2 * (OVER + R), 2 * (OVER + R), EAVE - BASE), at=(sx * HX, sy * HY, (EAVE + BASE) * 0.5))
eave_top = EAVE - OVERHANG * math.tan(theta) + 0.25
ridge_top = EAVE + RISE + 0.3
m.hull_points([(x, y, eave_top) for x in (-OUT_X - RAKE, OUT_X + RAKE) for y in (-OUT_Y - OVERHANG, OUT_Y + OVERHANG)] +
              [(x, 0.0, ridge_top) for x in (-OUT_X - RAKE, OUT_X + RAKE)])
m.hull((1.0, 1.4, 3.1), at=(cx - 0.05, cy, 1.55))
m.hull((0.7, 0.8, top - 3.1 + 0.3), at=(cx + 0.02, cy, (3.1 + top + 0.3) * 0.5))
m.hull((PX1 - PX0, -OUT_Y - PY, DECK), at=((PX0 + PX1) * 0.5, (-OUT_Y + PY) * 0.5, DECK * 0.5))
for x in (PX0 + 0.15, PX1 - 0.15):
    m.hull((0.22, 0.22, BEAM_Z - DECK), at=(x, POST_Y, (DECK + BEAM_Z) * 0.5))
m.hull((porch_roof.width, p_len, 0.18), at=(porch_roof.width * 0.5, p_len * 0.5, -0.02), space=porch_roof)

m.finish()
