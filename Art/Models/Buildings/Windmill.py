"""The water-pump windmill on the hill above the target meadow: a rusty steel lattice tower on stone footings with a
ladder and a plank platform near the top, the head with its tail vane, and the pump below. Two models:

- Windmill: the tower, its head and the tail vane (which don't turn). SOCKET_Fan is the hub: its front (-Y of the empty,
  as for models) is the way the fan faces, into the wind; its top is up.
- WindmillFan: the multi-blade wheel, which the game spins about its own front axis (Blender -Y, Unreal +X). Its origin
  is the hub and it faces -Y, in the XZ plane. It moves, so it has no Nanite (LODs instead) and no collision.

Scripted models (Art/README.md) built with looter_buildings; the tower's pivot is the middle of its base at ground
level.
"""
import looter_buildings as kit
from looter_buildings import Trim, math

IRON = kit.Tile('MetalRust')

# --- The tower. ---
m = kit.Model('Windmill', seed=117)
rng = m.rng

BASE_HALF, TOP_HALF, TOP = 1.3, 0.26, 8.6


def half(z):
    """The lattice's half-width at height z."""
    return BASE_HALF + (TOP_HALF - BASE_HALF) * z / TOP


def strut(p0, p1, size, drop=('-x', '+x')):
    """A thin steel member from p0 to p1 (its ends left open where nobody sees them)."""
    length = (kit.Vector(p1) - kit.Vector(p0)).length
    matrix = kit.toward(p0, p1, (0.0, 0.0, 1.0) if abs(p1[2] - p0[2]) < 0.5 * length else (0.0, -1.0, 0.0))
    m.emit(kit._box((length, size, size), drop=drop), IRON, 'metal', matrix @ kit.Matrix.Translation((length * 0.5, 0.0, 0.0)))


corners = [(-1.0, -1.0), (1.0, -1.0), (1.0, 1.0), (-1.0, 1.0)]
for sx, sy in corners:
    kit.stone_stack(m, (sx * BASE_HALF, sy * BASE_HALF, -0.05), (0.46, 0.46), 0.35, taper=0.12,
                    rot=rng.uniform(-6.0, 6.0))
    m.board((sx * BASE_HALF, sy * BASE_HALF, 0.1), (sx * TOP_HALF, sy * TOP_HALF, TOP + 0.05), 0.085, 0.085, uv=IRON,
            mat='metal', face=(sx, sy, 0.0))
LEVELS = [0.32, 2.1, 3.7, 5.05, 6.2, 7.2, 8.05]
for z in LEVELS:
    w = half(z)
    for (ax, ay), (bx, by) in zip(corners, corners[1:] + corners[:1]):
        strut((ax * w, ay * w, z), (bx * w, by * w, z), 0.055, drop=())
# X bracing in every panel between the girts, on all four sides.
for z0, z1 in zip(LEVELS, LEVELS[1:]):
    w0, w1 = half(z0), half(z1)
    for (ax, ay), (bx, by) in zip(corners, corners[1:] + corners[:1]):
        strut((ax * w0, ay * w0, z0), (bx * w1, by * w1, z1), 0.028)
        strut((bx * w0, by * w0, z0), (ax * w1, ay * w1, z1), 0.028)
m.section('lattice')

# A ladder up the front right leg's face, and a plank platform round the top.
for k in range(22):
    z = 0.5 + k * 0.33
    w = half(z)
    strut((0.35 * w, -w - 0.06, z), (0.95 * w, -w - 0.06, z), 0.03)
for x_frac in (0.35, 0.95):
    strut((x_frac * half(0.3), -half(0.3) - 0.06, 0.3), (x_frac * half(7.6), -half(7.6) - 0.06, 7.6), 0.04, drop=())
PLAT_Z = 7.55
for k in range(8):  # set back from the front, clear of the wheel
    y = -0.45 + k * 0.2
    if abs(y) < 0.36:  # the tower's top passes through the middle
        for x0, x1 in ((-0.85, -0.36), (0.36, 0.85)):
            m.board((x0, y, PLAT_Z), (x1, y, PLAT_Z), 0.19, 0.045, face=(0.0, 0.0, 1.0), uv='A')
    else:
        m.board((-0.85, y, PLAT_Z), (0.85, y, PLAT_Z), 0.19, 0.045, face=(0.0, 0.0, 1.0), uv='A')
for y in (-half(PLAT_Z), half(PLAT_Z)):
    strut((-0.9, y, PLAT_Z - 0.06), (0.9, y, PLAT_Z - 0.06), 0.06, drop=())
m.section('ladder, platform')

# The head: a turntable on the mast top, the gearbox, the shaft out to the hub, and the tail boom with its vane.
HUB = (0.0, -0.85, 9.12)
m.cylinder((0.0, 0.0, TOP), (0.0, 0.0, TOP + 0.14), 0.3, sides=10, uv=IRON, mat='metal')
m.box((0.3, 0.3, 0.3), at=(0.0, 0.0, TOP + 0.26), uv=IRON, mat='metal')
m.box((0.34, 0.95, 0.38), at=(0.0, -0.12, HUB[2] - 0.07), uv=IRON, mat='metal', bevel=0.03)
m.cylinder((0.0, -0.55, HUB[2]), (0.0, HUB[1] + 0.07, HUB[2]), 0.06, sides=8, uv=IRON, mat='metal')
m.box((0.26, 0.12, 0.3), at=(0.0, -0.64, HUB[2] - 0.05), uv=IRON, mat='metal')
TAIL_END = 2.6
strut((0.0, 0.3, TOP + 0.42), (0.0, TAIL_END, TOP + 0.62), 0.07, drop=())
strut((0.0, 0.25, TOP + 0.2), (0.0, 1.6, TOP + 0.56), 0.04)
strut((0.0, 0.3, TOP + 0.62), (0.0, 1.9, TOP + 0.66), 0.035)
# The vane: sheet steel in faded oxide red, on the boom's end.
# The vane's own frame: x along the boom, z up, its face (-y) toward +X.
vane = kit.Space(kit.basis((0.0, TAIL_END - 0.05, TOP + 0.62), (0.0, 1.0, 0.0), (-1.0, 0.0, 0.0), (0.0, 0.0, 1.0)))
for k, (z0, z1) in enumerate(((-0.45, -0.25), (-0.25, -0.05), (-0.05, 0.15), (0.15, 0.35), (0.35, 0.55))):
    length = 1.55 - abs((z0 + z1) * 0.5 - 0.05) * 0.8
    m.board((0.0, 0.0, (z0 + z1) * 0.5), (length, 0.0, (z0 + z1) * 0.5), 0.2, 0.02, uv='H2', space=vane)
m.board((-0.02, 0.0, -0.45), (-0.02, 0.0, 0.55), 0.06, 0.04, face=(0.0, -1.0, 0.0), uv=IRON, mat='metal', space=vane)
m.section('head')

# The pump rod down the middle, the pump stand at the foot with its spout.
m.cylinder((0.0, 0.0, 0.55), (0.0, 0.0, TOP + 0.02), 0.022, sides=5, uv=IRON, mat='metal', caps=(False, False))
m.cylinder((0.0, 0.0, 0.0), (0.0, 0.0, 0.95), 0.1, sides=8, uv=IRON, mat='metal', caps=(False, True))
m.cylinder((0.0, -0.08, 0.72), (0.0, -0.55, 0.62), 0.045, 0.04, sides=6, uv=IRON, mat='metal')
kit.stone_stack(m, (0.0, 0.0, -0.05), (0.5, 0.5), 0.2, taper=0.1)
m.section('pump')

# Collision: the legs (a hull round the lattice below the platform), the platform and the head.
m.hull_points([(sx * half(z), sy * half(z), z) for sx, sy in corners for z in (0.0, PLAT_Z - 0.1)])
m.hull((1.8, 1.35, 0.1), at=(0.0, 0.0, PLAT_Z))
m.hull((0.4, 1.1, 0.8), at=(0.0, -0.15, TOP + 0.35))
m.socket('Fan', HUB)
tower = m.finish(preview=False)

# --- The fan: a steel wheel of 18 pitched blades on two rings and six arms, built round its hub, facing -Y. ---
f = kit.Model('WindmillFan', seed=118)
BLADES, R_IN, R_OUT = 18, 0.5, 1.6


def fan_part(size, matrix):
    f.emit(kit._box(size), IRON, 'metal', matrix)


for k in range(BLADES):
    a = 2.0 * math.pi * k / BLADES
    radial = kit.Vector((math.cos(a), 0.0, math.sin(a)))
    # Each blade runs out along its radius, turned about it by the pitch; it's wider toward the rim.
    blade = kit.Matrix.Rotation(-a, 4, 'Y') @ kit.Matrix.Rotation(math.radians(32.0), 4, 'X')
    tb = kit._box((R_OUT - R_IN, 0.012, 1.0))
    widths = (0.13, 0.26)

    def taper(co, widths=widths):
        t = co.x / (R_OUT - R_IN) + 0.5
        return kit.Vector((co.x + (R_OUT + R_IN) * 0.5, co.y, co.z * (widths[0] + (widths[1] - widths[0]) * t)))
    f.emit(tb, IRON, 'metal', blade, shape=taper)
for radius, size in ((R_IN + 0.05, 0.045), (R_OUT - 0.08, 0.05), (1.05, 0.03)):
    for k in range(BLADES):
        a0, a1 = 2.0 * math.pi * k / BLADES, 2.0 * math.pi * (k + 1) / BLADES
        p0 = (radius * math.cos(a0), -0.03, radius * math.sin(a0))
        p1 = (radius * math.cos(a1), -0.03, radius * math.sin(a1))
        length = (kit.Vector(p1) - kit.Vector(p0)).length
        f.emit(kit._box((length + 0.01, size, size)), IRON, 'metal',
               kit.toward(p0, p1, (0.0, -1.0, 0.0)) @ kit.Matrix.Translation((length * 0.5, 0.0, 0.0)))
for k in range(6):
    a = 2.0 * math.pi * (k + 0.5) / 6
    p1 = (R_OUT * math.cos(a), -0.05, R_OUT * math.sin(a))
    length = kit.Vector(p1).length
    f.emit(kit._box((length, 0.035, 0.05)), IRON, 'metal',
           kit.toward((0.0, -0.05, 0.0), p1, (0.0, -1.0, 0.0)) @ kit.Matrix.Translation((length * 0.5, 0.0, 0.0)))
f.cylinder((0.0, 0.08, 0.0), (0.0, -0.16, 0.0), 0.13, sides=10, uv=IRON, mat='metal', face=(0.0, 0.0, 1.0))
f.cylinder((0.0, -0.16, 0.0), (0.0, -0.3, 0.0), 0.1, 0.03, sides=10, uv=IRON, mat='metal', face=(0.0, 0.0, 1.0),
           caps=(False, False))
fan = f.finish(ground=False, preview=False, Nanite=0, LODs='40,12', Collision='None')

if kit.lt.want_preview():
    # Seen together: the fan put on the tower's socket for the picture (models export from their own origins).
    fan.location = HUB
    kit.lt.preview([tower, fan], kit.lt.preview_path('Buildings', 'Windmill'), view=(-1.0, -1.6, 0.3), fit=1.1)
    fan.location = (0.0, 0.0, 0.0)
elif __name__ == '__overview__':
    fan.location = HUB  # the overview shows it on the tower
    fan['Mounted'] = 1
