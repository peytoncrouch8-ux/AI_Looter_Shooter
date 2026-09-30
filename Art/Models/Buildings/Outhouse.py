"""The outhouse: a leaning little plank privy with a crescent moon cut in its door, under a rusty tin roof that slopes
to the back, on a couple of flat stones.

A scripted model (Art/README.md) built with looter_buildings: the pivot is the middle of the footprint at ground level
and the door faces the front (-Y).
"""
import looter_buildings as kit
from looter_buildings import Opening, Trim, math

m = kit.Model('Outhouse', seed=57)
rng = m.rng

X0, X1, Y0, Y1 = -0.65, 0.65, -0.65, 0.65
BASE = 0.12
FRONT_H, BACK_H = 2.25, 1.95   # wall heights above the floor, front and back: the roof sheds water backward
THICK = 0.06
W, D = X1 - X0, Y1 - Y0
slope_z = lambda y: FRONT_H + (y - Y0) / D * (BACK_H - FRONT_H)

# Two flat stones and a plank floor it stands on.
for x in (X0 + 0.2, X1 - 0.2):
    kit.stone_stack(m, (x, 0.0, 0.0), (0.36, D + 0.1), BASE, rot=rng.uniform(-4.0, 4.0))
m.box((W + 0.04, D + 0.04, 0.05), at=(0.0, 0.0, BASE + 0.025), uv=Trim('A', world=True))

DOOR = Opening('door', 0.28, 0.0, 0.74, 1.9)
walls = [((X0, Y0), (X1, Y0), [DOOR]), ((X1, Y0), (X1, Y1), []), ((X1, Y1), (X0, Y1), []), ((X0, Y1), (X0, Y0), [])]
for p0, p1, items in walls:
    space = kit.wall_space(p0, p1, BASE)
    length = (kit.Vector(p1) - kit.Vector(p0)).length
    # The top follows the roof: sloped on the sides, level at the front and back.
    z_start, z_end = slope_z(p0[1]), slope_z(p1[1])
    outline = kit.wall_outline(length, z_end, items)
    outline[-1] = (0.0, z_start)
    m.panel(outline, (), THICK, Trim('A', world=True, rotate=True), space=space)
    for x, zt in ((0.05, z_start), (length - 0.05, z_end)):
        kit.trim_board(m, space, (x, 0.0, 0.0), (x, 0.0, zt - 0.03), 0.1, 'C', thick=0.03)
    kit.trim_board(m, space, (0.1, 0.0, 0.08), (length - 0.1, 0.0, 0.08), 0.14, 'C', thick=0.03)

# The door: planks with a crescent moon cut through, a Z brace, strap hinges and a wooden turn-button.
front = kit.wall_space((X0, Y0), (X1, Y0), BASE)
# The crescent: an outer circle less an inner one set to the right; its horns are where the two circles cross.
cx, cz = DOOR.x + DOOR.w * 0.5 - 0.03, 1.55
R1, R2, SHIFT = 0.14, 0.12, 0.07
hx = (SHIFT ** 2 + R1 ** 2 - R2 ** 2) / (2.0 * SHIFT)
hz = math.sqrt(R1 ** 2 - hx ** 2)
outer_a, inner_a = math.atan2(hz, hx), math.atan2(hz, hx - SHIFT)
moon = [(cx + R1 * math.cos(a), cz + R1 * math.sin(a))
        for a in (outer_a + (2.0 * math.pi - 2.0 * outer_a) * k / 8 for k in range(9))]
moon += [(cx + SHIFT + R2 * math.cos(a), cz + R2 * math.sin(a))
         for a in ((2.0 * math.pi - inner_a) - (2.0 * math.pi - 2.0 * inner_a) * k / 8 for k in range(1, 8))]
leaf = kit.rect(DOOR.x + 0.01, 0.0, DOOR.w - 0.02, DOOR.h)
m.panel(leaf, [moon], 0.04, Trim('A', world=True, rotate=True), space=front,
        matrix=kit.Matrix.Translation((0.0, 0.01, 0.0)))
for z in (0.3, 1.25):
    m.board((DOOR.x + 0.05, -0.02, z), (DOOR.x + DOOR.w - 0.05, -0.02, z), 0.12, 0.03, uv='A', space=front)
m.board((DOOR.x + DOOR.w - 0.1, -0.02, 0.36), (DOOR.x + 0.1, -0.02, 1.19), 0.1, 0.025, uv='A', space=front)
for z in (0.3, 1.25):
    m.board((DOOR.x - 0.04, -0.045, z), (DOOR.x + 0.32, -0.045, z), 0.06, 0.012, uv=Trim('H3', fit=True), space=front)
m.box((0.04, 0.04, 0.12), at=(DOOR.x + DOOR.w - 0.02, -0.04, 1.0), rot=(0.0, 20.0, 0.0), uv='C', space=front)
for x in (DOOR.x - 0.05, DOOR.x + DOOR.w + 0.05):
    kit.trim_board(m, front, (x, 0.0, 0.0), (x, 0.0, DOOR.h + 0.08), 0.1, 'C', thick=0.035)
kit.trim_board(m, front, (DOOR.x - 0.1, 0.0, DOOR.h + 0.06), (DOOR.x + DOOR.w + 0.1, 0.0, DOOR.h + 0.06), 0.1, 'C',
               thick=0.035)

# The roof: a shed of two tin sheets on a plank deck, sloping to the back.
theta = math.atan2(FRONT_H - BACK_H, D)
over = 0.18
deck = 0.05
normal = kit.Vector((0.0, math.sin(theta), math.cos(theta)))
origin = kit.Vector((X1 + over, Y1 + over, BASE + BACK_H - over * math.tan(theta))) + normal * deck
roof = kit.Slope(origin, (-1.0, 0.0, 0.0), (0.0, -math.cos(theta), math.sin(theta)), W + 2 * over,
                 (D + 2 * over) / math.cos(theta), sag=0.02)
kit.roof_deck(m, roof, deck)
kit.tin(m, roof, sheets=(1, 2), missing=0.0, top=roof.length + 0.03)
# A stone keeps the loose sheet down.
m.box((0.18, 0.14, 0.09), at=(0.5, roof.length * 0.6, 0.07), rot=(0.0, 0.0, 17.0), uv='D', space=roof)

# Collision: the booth and its roof.
m.hull((W + 0.08, D + 0.08, BASE + BACK_H), at=(0.0, 0.0, (BASE + BACK_H) * 0.5))
m.hull((roof.width, roof.length, 0.14), at=(roof.width * 0.5, roof.length * 0.5, -0.02), space=roof)

m.finish()
