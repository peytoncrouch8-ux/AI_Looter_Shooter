"""The farmstead well: a ring of hand-laid fieldstone with a coping of flat stones, dark water below, two posts carrying
a little shake roof, and a windlass with an iron crank, its rope down to a bucket.

A scripted model (Art/README.md) built with looter_buildings: the pivot is the middle of the well at ground level; the
crank is on the right as seen from the front (-Y).
"""
import looter_buildings as kit
from looter_buildings import Trim, math

m = kit.Model('Well', seed=69)
rng = m.rng

SIDES = 16
R_OUT, R_IN = 0.8, 0.58
RING_TOP = 0.78


def ring(r_out, r_in, z0, z1, jitter=0.0, uv=Trim('D', world=True), top=True, inner=True):
    """A ring of stone from z0 to z1 (outer wall, inner wall and top), a little uneven: its outline wanders, and so
    does its height where it has no top (a top must stay flat: trim_uv maps planar faces)."""
    tb = kit._new_bmesh()
    wobble = [(1.0 + rng.uniform(-jitter, jitter), 0.0 if top else rng.uniform(-jitter, jitter) * 0.8)
              for _ in range(SIDES)]
    rows = []
    for radius, h_jitter in ((r_out, True), (r_in, False)):
        lo, hi = [], []
        for k in range(SIDES):
            a = 2.0 * math.pi * (k + 0.5) / SIDES
            scale, dz = wobble[k]
            r = radius * (scale if h_jitter else 1.0)
            lo.append(tb.verts.new((r * math.cos(a), r * math.sin(a), z0)))
            hi.append(tb.verts.new((r * math.cos(a), r * math.sin(a), z1 + dz)))
        rows.append((lo, hi))
    (olo, ohi), (ilo, ihi) = rows
    for k in range(SIDES):
        j = (k + 1) % SIDES
        tb.faces.new((olo[k], olo[j], ohi[j], ohi[k]))
        if inner:
            tb.faces.new((ilo[j], ilo[k], ihi[k], ihi[j]))
        if top:
            tb.faces.new((ohi[k], ohi[j], ihi[j], ihi[k]))
    m.emit(tb, uv, 'trim')


# The stone ring, its coping and the water inside.
ring(R_OUT, R_IN, -0.05, RING_TOP, jitter=0.025, top=False)
ring(R_OUT + 0.07, R_IN - 0.03, RING_TOP - 0.02, RING_TOP + 0.12, jitter=0.02)
water = kit._new_bmesh()
verts = [water.verts.new(((R_IN - 0.01) * math.cos(2.0 * math.pi * (k + 0.5) / SIDES),
                          (R_IN - 0.01) * math.sin(2.0 * math.pi * (k + 0.5) / SIDES), 0.25)) for k in range(SIDES)]
water.faces.new(verts)
m.emit(water, Trim('H4', fit=True), 'trim')
# A few loose stones at the foot of the ring.
for k in range(4):
    a = rng.uniform(0.0, 2.0 * math.pi)
    r = R_OUT + rng.uniform(0.08, 0.3)
    m.box((rng.uniform(0.16, 0.26), rng.uniform(0.12, 0.2), rng.uniform(0.07, 0.12)),
          at=(r * math.cos(a), r * math.sin(a), 0.03), rot=(rng.uniform(-8, 8), rng.uniform(-8, 8), math.degrees(a)),
          uv='D')
m.section('ring')

# Posts on the ring with cross-beams on top, plates along the eaves and a ridge beam.
POST_X = R_OUT - 0.1
PLATE_Z = 2.05
for x in (-POST_X, POST_X):
    m.board((x, 0.0, RING_TOP - 0.3), (x, 0.0, PLATE_Z + 0.34), 0.14, 0.14, uv=Trim('C', lane='each'), bevel=0.01)
    m.board((x, -0.62, PLATE_Z - 0.08), (x, 0.62, PLATE_Z - 0.08), 0.14, 0.12, face=(0.0, 0.0, 1.0),
            uv=Trim('C', lane='each'))
    for side in (-1.0, 1.0):
        m.board((x, side * 0.05, PLATE_Z - 0.55), (x, side * 0.42, PLATE_Z - 0.14), 0.08, 0.07, face=(1.0, 0.0, 0.0),
                uv='C')
for y in (-0.52, 0.52):
    m.board((-POST_X - 0.18, y, PLATE_Z + 0.02), (POST_X + 0.18, y, PLATE_Z + 0.02), 0.1, 0.1, uv=Trim('C', lane='each'))
m.board((-POST_X - 0.2, 0.0, PLATE_Z + 0.39), (POST_X + 0.2, 0.0, PLATE_Z + 0.39), 0.1, 0.1, uv=Trim('C', lane='each'))
m.section('frame')

# The roof: a small gable of shakes, with boards closing its gable ends.
PITCH = 40.0
front, back = kit.gable(m, -POST_X, POST_X, -0.52, 0.52, PLATE_Z + 0.07, PITCH, overhang=0.18, rake=0.28, deck=0.05,
                        sag=0.03)
kit.shingles(m, front, piece=(0.8, 1.3))
kit.shingles(m, back, piece=(0.8, 1.3))
kit.ridge_cap(m, front, back, uv='C', width=0.12, thick=0.04)
rise = 0.52 * math.tan(math.radians(PITCH))
for x, p0, p1 in ((POST_X + 0.08, (POST_X + 0.08, -0.52), (POST_X + 0.08, 0.52)),
                  (-POST_X - 0.08, (-POST_X - 0.08, 0.52), (-POST_X - 0.08, -0.52))):
    space = kit.wall_space(p0, p1, PLATE_Z + 0.07)
    m.panel([(0.0, 0.0), (1.04, 0.0), (0.52, rise - 0.02)], (), 0.03, Trim('A', world=True, rotate=True), space=space)
m.section('roof')

# The windlass: a log drum between the posts, rope wound on it and down to a bucket, and an iron crank.
DRUM_Z = 1.3
kit.log(m, (-POST_X - 0.05, 0.0, DRUM_Z), (POST_X + 0.05, 0.0, DRUM_Z), 0.085, (0.0, -1.0, 0.0), sides=8)
# Rope: a lane of weathered grey siding, which at this size reads as old hemp.
m.cylinder((-0.22, 0.0, DRUM_Z), (0.18, 0.0, DRUM_Z), 0.105, sides=8, uv=Trim('A', lane=3), caps=(False, False))
iron = kit.Tile('MetalRust')
m.cylinder((POST_X + 0.05, 0.0, DRUM_Z), (POST_X + 0.2, 0.0, DRUM_Z), 0.025, sides=6, uv=iron, mat='metal')
m.board((POST_X + 0.19, 0.0, DRUM_Z), (POST_X + 0.19, -0.02, DRUM_Z - 0.3), 0.04, 0.025, face=(1.0, 0.0, 0.0), uv=iron,
        mat='metal')
m.cylinder((POST_X + 0.19, -0.02, DRUM_Z - 0.3), (POST_X + 0.36, -0.02, DRUM_Z - 0.3), 0.022, sides=6,
           uv=Trim('C', lane=1))
ROPE_X = -0.05
BUCKET = (ROPE_X, -0.1, 0.8)
m.cylinder((ROPE_X, -0.06, DRUM_Z - 0.06), (ROPE_X, -0.1, BUCKET[2] + 0.27), 0.014, sides=5, uv=Trim('A', lane=3),
           caps=(False, False))
# The bucket, drawn up: staves on a slight taper, two iron hoops and a bail.
bx, by, bz = BUCKET
m.cylinder((bx, by, bz - 0.13), (bx, by, bz + 0.13), 0.13, 0.155, sides=10, uv=Trim('A', lane=1), caps=(True, False))
for z in (bz - 0.07, bz + 0.08):
    m.cylinder((bx, by, z - 0.02), (bx, by, z + 0.02), 0.144 + (z - bz) * 0.1, sides=10, uv=iron, mat='metal',
               caps=(False, False))
m.board((bx - 0.16, by, bz + 0.12), (bx, by, bz + 0.27), 0.02, 0.015, face=(0.0, -1.0, 0.0), uv=iron, mat='metal')
m.board((bx, by, bz + 0.27), (bx + 0.16, by, bz + 0.12), 0.02, 0.015, face=(0.0, -1.0, 0.0), uv=iron, mat='metal')
m.section('windlass')

# Collision: the ring (capped over the water), the posts and the roof.
m.hull_points([((R_OUT + 0.08) * math.cos(2.0 * math.pi * k / 12), (R_OUT + 0.08) * math.sin(2.0 * math.pi * k / 12), z)
               for k in range(12) for z in (0.0, RING_TOP + 0.12)])
for x in (-POST_X, POST_X):
    m.hull((0.16, 0.16, PLATE_Z - RING_TOP), at=(x, 0.0, (PLATE_Z + RING_TOP) * 0.5 + 0.06))
roof_lo = PLATE_Z + 0.07 - 0.18 * math.tan(math.radians(PITCH))
m.hull_points([(x, y, roof_lo) for x in (-POST_X - 0.28, POST_X + 0.28) for y in (-0.7, 0.7)] +
              [(x, 0.0, PLATE_Z + 0.07 + rise + 0.15) for x in (-POST_X - 0.28, POST_X + 0.28)])

m.finish(view=(-1.0, -1.6, 0.45))
