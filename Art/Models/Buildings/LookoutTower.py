"""The ruined lookout on the cliff-top plateau: a square fieldstone tower whose walls have crumbled to ragged tops (the
back half-fallen, its stones heaped below), a timber platform on beams across the top, and a wooden stair in two
flights around the outside. Above the platform the old watch hut is mostly gone: four posts, one snapped, a few beams
and half of its shake roof hanging on.

A scripted model (Art/README.md) built with looter_buildings: the pivot is the middle of the tower at ground level and
the doorway faces the front (-Y). The platform's floor is 6.2 m up; the stair starts at the front right corner. Every
walking surface has a UCX box (the flights are sloped at 42 degrees, under the 44.8 a character can walk).
"""
import looter_buildings as kit
from looter_buildings import Opening, Trim, math

m = kit.Model('LookoutTower', seed=105)
rng = m.rng

H = 2.0            # half the tower's outer width
T = 0.55           # wall thickness
FLOOR = 6.2        # the platform's walking surface
BEAM_Z = FLOOR - 0.15

# --- The stone shaft: four walls with ragged tops. Front and back run the full width, the sides fit between. ---


def ragged(length, base, rough, lo_at=None, drop=0.0, steps=9):
    """A wall's top from right to left (as the outline continues): base height, stepped roughness, and a breach of
    depth drop centered at lo_at (0..1 along the wall)."""
    points = []
    for k in range(steps, -1, -1):
        t = k / steps
        z = base + rng.uniform(-rough, rough)
        if lo_at is not None:
            z -= drop * max(0.0, 1.0 - abs(t - lo_at) / 0.35)
        x = length * t
        points.append((x, z))
        if 0 < k:
            points.append((x - length / steps * rng.uniform(0.25, 0.6), z + rng.uniform(-rough, rough) * 0.8))
    return points


DOOR = Opening('door', H - 0.55, 0.0, 1.1, 2.2)
SLIT = dict(kind='window')
walls = [
    # start, end, set in (sides fit between front and back), top: base, roughness, breach
    ((-H, -H), (H, -H), 0.0, (5.55, 0.2, None, 0.0), [DOOR, Opening('window', 0.8, 3.4, 0.16, 0.85)]),
    ((H, -H), (H, H), T, (5.55, 0.2, 0.85, 1.6), [Opening('window', 1.4, 3.3, 0.16, 0.85)]),
    ((H, H), (-H, H), 0.0, (5.4, 0.25, 0.72, 2.0), [Opening('window', 1.1, 2.6, 0.16, 0.85)]),
    ((-H, H), (-H, -H), T, (5.55, 0.2, 0.2, 1.3), [Opening('window', 1.8, 3.3, 0.16, 0.85)]),
]
for p0, p1, inset, (base, rough, lo_at, drop), items in walls:
    space = kit.wall_space(p0, p1, 0.0)
    length = (kit.Vector(p1) - kit.Vector(p0)).length - 2 * inset
    local = kit.Space(space.matrix @ kit.Matrix.Translation((inset, 0.0, 0.0)))
    top = ragged(length, base, rough, lo_at, drop)
    # The bottom edge (notched for the doorway) up to the right end, then the ragged top back to the left.
    outline = kit.wall_outline(length, top[0][1], [o for o in items if o.z <= 1e-4], None)[:-2] + top
    m.panel(outline, kit.holes(items), T, Trim('D', world=True, v=0.13), space=local, around=items, slices=1.0)
    for o in items:
        if o.kind == 'door':
            # A heavy timber lintel over the doorway, darkness inside.
            m.board((o.x - 0.3, T * 0.5, o.h + 0.12), (o.x + o.w + 0.3, T * 0.5, o.h + 0.12), 0.24, T + 0.04,
                    uv=Trim('C', lane='each'), space=local)
m.section('stone walls')

# Rubble heaped below the breach at the back (higher near the wall), and stones fallen by the other walls.
for k in range(26):
    if k < 18:
        x = rng.uniform(-2.0, 0.4)
        d = rng.uniform(0.0, 1.7)
        y = H + 0.1 + d
        size = rng.uniform(0.3, 0.6)
        z = size * 0.25 + max(0.0, 0.9 - d * 0.55) * rng.uniform(0.3, 1.0)
    else:
        side = rng.choice(((1.0, 0.0), (-1.0, 0.0), (0.0, -1.0)))
        along = rng.uniform(-1.8, 1.8)
        out = H + rng.uniform(0.2, 1.0)
        x, y = (side[0] * out + side[1] * along, side[1] * out + side[0] * along)
        size = rng.uniform(0.2, 0.4)
        z = size * 0.2
    m.box((size, size * rng.uniform(0.6, 0.95), size * rng.uniform(0.45, 0.7)), at=(x, y, z),
          rot=(rng.uniform(-20, 20), rng.uniform(-20, 20), rng.uniform(0, 90)), uv='D')
# A fallen beam among them.
m.board((-0.9, H + 0.6, 0.12), (0.8, H + 1.9, 0.35), 0.2, 0.18, face=(0.0, 0.0, 1.0), uv=Trim('C', lane='each'))
m.section('rubble')

# --- The platform: beams through the walls, planks overhanging front and left, a railing with a gap at the stair. ---
PX0, PX1, PY0, PY1 = -H - 0.5, H + 0.05, -H - 0.5, H + 0.05
for x in (-2.3, -1.2, 0.0, 1.2):
    m.board((x, PY0 - 0.05, BEAM_Z - 0.12), (x, PY1, BEAM_Z - 0.12), 0.24, 0.2, face=(0.0, 0.0, 1.0),
            uv=Trim('C', lane='each'))
m.board((PX0, PY0 + 0.1, BEAM_Z - 0.36), (PX1, PY0 + 0.1, BEAM_Z - 0.36), 0.2, 0.2, uv=Trim('C', lane='each'))
for y in kit.frange(PY0 + 0.1, PY1, 0.2):
    left = PX0 - rng.uniform(0.0, 0.05)
    right = PX1 + rng.uniform(0.0, 0.04)
    m.board((left, y, FLOOR - 0.025), (right, y, FLOOR - 0.025), 0.19, 0.05, face=(0.0, 0.0, 1.0), uv='A')
# Where the back wall has fallen, a crossbeam on two props (inside the tower) holds the beams up.
m.board((-2.0, H - 0.3, BEAM_Z - 0.34), (1.7, H - 0.3, BEAM_Z - 0.34), 0.22, 0.2, uv=Trim('C', lane='each'))
for x in (-1.6, 0.6):
    m.board((x, H - 0.3, 0.0), (x, H - 0.3, BEAM_Z - 0.45), 0.18, 0.18, uv=Trim('C', lane='each'))
# Knee braces under the overhang, down to the walls.
for x in (-1.2, 1.2):
    m.board((x, -H - 0.02, BEAM_Z - 1.0), (x, PY0 + 0.05, BEAM_Z - 0.25), 0.12, 0.1, face=(1.0, 0.0, 0.0), uv='C')
for y in (-1.2, 1.2):
    m.board((-H - 0.02, y, BEAM_Z - 1.0), (PX0 + 0.05, y, BEAM_Z - 0.25), 0.12, 0.1, face=(0.0, 1.0, 0.0), uv='C')
RAIL = FLOOR + 1.0
posts = [(PX0 + 0.06, PY0 + 0.06), (0.0, PY0 + 0.06), (PX1 - 0.06, PY0 + 0.06), (PX1 - 0.06, 0.0),
         (PX1 - 0.06, PY1 - 0.06), (0.0, PY1 - 0.06), (PX0 + 0.06, PY1 - 0.06), (PX0 + 0.06, 0.0)]
for x, y in posts:
    m.board((x, y, FLOOR - 0.3), (x, y, RAIL + 0.06), 0.1, 0.1, uv=Trim('C', lane='each'))
# Rails between the posts: the right-hand stretch is broken off, and the back left is open to the stair.
runs = [(0, 1), (1, 2), (2, 3), (4, 5), (7, 0)]
for a, b in runs:
    (xa, ya), (xb, yb) = posts[a], posts[b]
    out = (0.0, -1.0, 0.0) if ya == yb and ya < 0 else (0.0, 1.0, 0.0) if ya == yb else \
        (1.0, 0.0, 0.0) if xa > 0 else (-1.0, 0.0, 0.0)
    m.board((xa, ya, RAIL), (xb, yb, RAIL), 0.1, 0.06, face=out, uv='A', lift=0.06)
    m.board((xa, ya, FLOOR + 0.5), (xb, yb, FLOOR + 0.5), 0.12, 0.04, face=out, uv='A', lift=0.06)
(xa, ya), (xb, yb) = posts[3], posts[4]
m.board((xa, ya, RAIL), (xa, ya + 0.7, RAIL - 0.08), 0.1, 0.06, face=(1.0, 0.0, 0.0), uv='A', lift=0.06)
m.board((xb, yb - 0.1, FLOOR + 0.5), (xb, yb - 0.9, FLOOR + 0.22), 0.12, 0.04, face=(1.0, 0.0, 0.0), uv='A', lift=0.06)
m.section('platform')

# --- The old watch hut: posts at the platform's corners, one snapped, a tie beam or two and half a roof. ---
HUT = [(-1.7, -1.7, 8.7), (1.7, -1.7, 7.75), (1.7, 1.7, 7.2), (-1.7, 1.7, 8.7)]  # the right-hand posts snapped
for x, y, top in HUT:
    m.board((x, y, BEAM_Z - 0.4), (x, y, top), 0.16, 0.16, uv=Trim('C', lane='each'), bevel=0.012)
m.board((-1.85, -1.7, 8.55), (1.85, -1.7, 7.72), 0.18, 0.14, uv=Trim('C', lane='each'))
m.board((-1.7, -1.85, 8.55), (-1.7, 1.85, 8.55), 0.18, 0.14, face=(-1.0, 0.0, 0.0), uv=Trim('C', lane='each'))
m.board((1.7, -1.85, 7.66), (1.75, 0.6, 6.9), 0.18, 0.14, face=(1.0, 0.0, 0.0), uv=Trim('C', lane='each'))
# What's left of the pyramid roof: the front slope, sagging, and two bare rafters.
theta = math.radians(32.0)
roof_len = 2.05 / math.cos(theta)
origin = kit.Vector((-2.05, -2.05 - 0.25, 8.62 - 0.25 * math.tan(theta)))
# It has tipped down to the right with the snapped posts, turning about its left front corner.
tip = kit.Matrix.Translation(origin) @ kit.Matrix.Rotation(math.radians(11.0), 4, 'Y') @ kit.Matrix.Translation(-origin)
front_roof = kit.Slope(origin, (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), 4.1, roof_len + 0.3, sag=0.3,
                       frame=tip)
kit.roof_deck(m, front_roof, 0.06, rake_boards=True)
kit.shingles(m, front_roof, piece=(0.8, 1.6), missing=0.15)
peak = (0.0, 0.0, 8.62 + 2.05 * math.tan(theta))
for x, y in ((-1.7, 1.7), (1.4, 1.2)):
    m.board((x, y, 8.62), (x * 0.3 + peak[0] * 0.7, y * 0.3 + peak[1] * 0.7, 8.62 + 1.2 * 0.7), 0.14, 0.1,
            face=(0.0, 0.0, 1.0), uv='C')
m.section('hut')

# --- The stair: two flights round the right and back walls, landings, posts to the ground. ---
RISE, RUN = FLOOR * 0.5, FLOOR * 0.5 / math.tan(math.radians(42.0))
SX0, SX1 = H + 0.05, H + 1.05       # the first flight's width, along the right wall
BY0, BY1 = H + 0.05, H + 1.05       # the second flight's, along the back wall
Y_START = H + 0.15 - RUN            # the first flight's foot (toward the front)
X_TOP = SX0 - RUN                   # where the second flight arrives


def flight(p0, p1, x_side, lo_z, hi_z, width_axis):
    """Treads and stringers from p0 (low end, ground or landing) to p1 (high end), width along width_axis."""
    steps = 15
    d = (kit.Vector(p1) - kit.Vector(p0)) / steps
    w = kit.Vector(width_axis)
    for k in range(steps):
        c = kit.Vector(p0) + d * (k + 0.5)
        z = lo_z + (hi_z - lo_z) * (k + 1) / steps - 0.025
        m.board(c - w * 0.5 + kit.Vector((0.0, 0.0, z)), c + w * 0.5 + kit.Vector((0.0, 0.0, z)), 0.26, 0.05,
                face=(0.0, 0.0, 1.0), uv='A')
    for side in (-0.5, 0.5):
        a = kit.Vector(p0) + w * side + kit.Vector((0.0, 0.0, lo_z - 0.1))
        b = kit.Vector(p1) + w * side + kit.Vector((0.0, 0.0, hi_z - 0.1))
        m.board(a, b, 0.26, 0.07, face=(w.x * side * 2, w.y * side * 2, 0.0), uv=Trim('C', lane='each'))
    return d


# Flight 1: up along the right wall, front to back.
cx = (SX0 + SX1) * 0.5
flight((cx, Y_START, 0.0), (cx, Y_START + RUN, 0.0), cx, 0.0, RISE, (1.0, 0.0, 0.0))
# Landing 1 at the back right corner, reaching past the back wall.
for y in kit.frange(Y_START + RUN + 0.1, BY1 + 0.05, 0.2):
    m.board((SX0 - 0.02, y, RISE - 0.025), (SX1 + 0.03, y, RISE - 0.025), 0.19, 0.05, face=(0.0, 0.0, 1.0), uv='A')
# Flight 2: up along the back wall, right to left.
cy = (BY0 + BY1) * 0.5
flight((SX0, cy, 0.0), (X_TOP, cy, 0.0), cy, RISE, FLOOR, (0.0, 1.0, 0.0))
# Landing 2 joins the platform's back left corner.
for x in kit.frange(X_TOP - 0.1, PX0 - 0.05, -0.2):
    m.board((x, PY1 - 0.1, FLOOR - 0.025), (x, BY1 + 0.03, FLOOR - 0.025), 0.19, 0.05, face=(0.0, 0.0, 1.0), uv='A')
# Beams and posts carrying the landings; a handrail up each flight's open side.
for (x0, y0), (x1, y1), z in ((((SX0, Y_START + RUN)), (SX1, BY1), RISE), ((PX0, PY1), (X_TOP, BY1), FLOOR)):
    for x in (x0 + 0.08, x1 - 0.08):
        for y in (y0 + 0.08, y1 - 0.08):
            if z == FLOOR and y < H:  # the platform carries this side
                continue
            m.board((x, y, 0.0), (x, y, z - 0.05), 0.14, 0.14, uv=Trim('C', lane='each'))
    m.board((x0, y1 - 0.08, z - 0.15), (x1, y1 - 0.08, z - 0.15), 0.18, 0.12, uv=Trim('C', lane='each'))
m.board((SX1 - 0.08, Y_START + RUN * 0.5, 0.0), (SX1 - 0.08, Y_START + RUN * 0.5, RISE * 0.5 - 0.1), 0.12, 0.12,
        uv=Trim('C', lane='each'))
m.board(((SX0 + X_TOP) * 0.5, BY1 - 0.08, 0.0), ((SX0 + X_TOP) * 0.5, BY1 - 0.08, (RISE + FLOOR) * 0.5 - 0.1), 0.12,
        0.12, uv=Trim('C', lane='each'))
kit.log(m, (SX1 + 0.02, Y_START + 0.1, 1.0), (SX1 + 0.02, Y_START + RUN, RISE + 0.95), 0.045, (1.0, 0.0, 0.0), sides=6)
kit.log(m, (SX0, BY1 + 0.02, RISE + 0.95), (X_TOP, BY1 + 0.02, FLOOR + 0.95), 0.045, (0.0, 1.0, 0.0), sides=6)
for x, y, z in ((SX1 + 0.02, Y_START + 0.1, 0.0), (SX1 + 0.02, Y_START + RUN, RISE), (SX0, BY1 + 0.02, RISE),
                (X_TOP, BY1 + 0.02, FLOOR)):
    m.board((x, y, z), (x, y, z + 1.0), 0.08, 0.08, uv='C')
m.section('stair')

# --- Collision. ---
for (p0, p1, inset, (base, rough, lo_at, drop), items) in walls:
    space = kit.wall_space(p0, p1, 0.0)
    length = (kit.Vector(p1) - kit.Vector(p0)).length - 2 * inset
    low = base - rough - (drop if lo_at is not None else 0.0)
    if items and items[0].kind == 'door':
        o = items[0]
        m.hull((o.x - inset, T, low), at=(inset + (o.x - inset) * 0.5, T * 0.5, low * 0.5), space=space)
        right = length + inset - (o.x + o.w)
        m.hull((right, T, low), at=(o.x + o.w + right * 0.5, T * 0.5, low * 0.5), space=space)
        m.hull((o.w, T, low - o.h), at=(o.x + o.w * 0.5, T * 0.5, (low + o.h) * 0.5), space=space)
    else:
        m.hull((length, T, low), at=(inset + length * 0.5, T * 0.5, low * 0.5), space=space)
m.hull((PX1 - PX0, PY1 - PY0, 0.4), at=((PX0 + PX1) * 0.5, (PY0 + PY1) * 0.5, FLOOR - 0.2))
m.hull((PX1 - PX0, 0.12, 1.0), at=((PX0 + PX1) * 0.5, PY0 + 0.06, FLOOR + 0.5))
for x, y, top in HUT:  # the hut's posts stand on the platform
    m.hull((0.18, 0.18, top - FLOOR), at=(x, y, (FLOOR + top) * 0.5))
m.hull((0.12, PY1 - PY0, 1.0), at=(PX0 + 0.06, (PY0 + PY1) * 0.5 - 0.6, FLOOR + 0.5))
for sx, ex, sy, ey, z0, z1 in ((SX0, SX1, Y_START, Y_START + RUN, 0.0, RISE),):
    m.hull_points([(x, sy, 0.0) for x in (sx, ex)] + [(x, sy, 0.05) for x in (sx, ex)] +
                  [(x, ey, z1) for x in (sx, ex)] + [(x, ey, z1 - 0.3) for x in (sx, ex)])
m.hull((SX1 - SX0 + 0.05, BY1 - (Y_START + RUN) + 0.05, 0.3), at=(cx, (Y_START + RUN + BY1) * 0.5, RISE - 0.15))
m.hull_points([(SX0, y, RISE) for y in (BY0, BY1)] + [(SX0, y, RISE - 0.3) for y in (BY0, BY1)] +
              [(X_TOP, y, FLOOR) for y in (BY0, BY1)] + [(X_TOP, y, FLOOR - 0.3) for y in (BY0, BY1)])
m.hull((X_TOP - PX0, BY1 - PY1, 0.3), at=((PX0 + X_TOP) * 0.5, (PY1 + BY1) * 0.5, FLOOR - 0.15))
m.hull((0.1, Y_START + RUN - (Y_START - 0.1), 1.0), at=(SX1 + 0.02, Y_START + RUN * 0.5, RISE * 0.5 + 0.9),
       rot=(math.degrees(math.atan2(RISE, RUN)), 0.0, 0.0))

m.finish(view=(-1.0, -1.5, 0.35), fit=0.95)
