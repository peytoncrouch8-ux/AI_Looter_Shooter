"""The footbridge where the forest road crosses the creek: three log stringers bedded on flat stones in each bank, a
deck of cross planks, and railings on posts held by outrigger braces; short plank ramps run down onto the banks.

A scripted model (Art/README.md) built with looter_buildings. The pivot is the middle of the bridge at bank level; the
span runs along Blender Y (the actor's forward in Unreal), 7 m between the ramps, for a creek about 3.5 m wide. The deck
is 2.2 m wide and its top 0.3 m above the pivot; UCX boxes follow the deck and ramps, with rails along the sides.
"""
import looter_buildings as kit
from looter_buildings import Trim, math

m = kit.Model('Bridge', seed=81)
rng = m.rng

HALF = 3.5         # half the deck's length (along Y)
WIDE = 1.1         # half the deck's width
DECK = 0.3         # the deck's top above the banks
PLANK = 0.05
RAMP = 0.8         # each ramp's length

# Flat stones bedded in the banks under the stringer ends.
for y in (-HALF + 0.35, HALF - 0.35):
    for x in (-0.8, 0.0, 0.8):
        m.box((0.55, 0.62, 0.16), at=(x + rng.uniform(-0.05, 0.05), y + rng.uniform(-0.05, 0.05), 0.02),
              rot=(rng.uniform(-3, 3), rng.uniform(-3, 3), rng.uniform(-12, 12)), uv='D')
# Three log stringers, the deck planks on them.
R = 0.16
for x in (-0.78, 0.0, 0.78):
    z = DECK - PLANK - R
    kit.log(m, (x, -HALF - 0.1 + rng.uniform(-0.05, 0.05), z), (x, HALF + 0.1 + rng.uniform(-0.05, 0.05), z),
            R + rng.uniform(-0.01, 0.01), (1.0, 0.0, 0.0), sides=10)
m.section('stringers')
for k, y in enumerate(kit.frange(-HALF + 0.1, HALF, 0.2)):
    left, right = -WIDE - rng.uniform(0.0, 0.06), WIDE + rng.uniform(0.0, 0.06)
    z = DECK - PLANK * 0.5 + rng.uniform(-0.006, 0.004)
    tilt = rng.uniform(-1.0, 1.0)
    if k == 11:  # one plank split and shorter
        right -= 0.35
    m.board((left, y, z), (right, y + math.radians(tilt) * 0.3, z), 0.19, PLANK, face=(0.0, 0.0, 1.0), uv='A')
m.section('deck')

# Ramps onto the banks: three planks each on a sloping sleeper.
for sign in (-1.0, 1.0):
    for k in range(4):
        t = (k + 0.5) / 4
        y = sign * (HALF + RAMP * t)
        z = DECK * (1.0 - t) - PLANK * 0.5 + 0.02
        m.board((-WIDE + 0.08 - rng.uniform(0, 0.05), y, z), (WIDE - 0.08 + rng.uniform(0, 0.05), y, z), 0.19, PLANK,
                face=(0.0, math.sin(math.atan2(DECK, RAMP)) * sign, 1.0), uv='A')
    for x in (-0.7, 0.7):
        m.board((x, sign * (HALF - 0.05), DECK - PLANK - 0.06), (x, sign * (HALF + RAMP + 0.05), -0.02), 0.1, 0.1,
                face=(0.0, 0.0, 1.0), uv='C')
m.section('ramps')

# Railings: posts outside the deck on outrigger beams with braces, a round handrail and a mid rail.
POSTS = [-HALF + 0.2, -HALF * 0.5, 0.0, HALF * 0.5, HALF - 0.2]
TOP = DECK + 1.0
for side in (-1.0, 1.0):
    px = side * (WIDE + 0.08)
    for k, y in enumerate(POSTS):
        m.board((0.0, y, DECK - PLANK - 0.07), (side * (WIDE + 0.42), y, DECK - PLANK - 0.07), 0.12, 0.1,
                face=(0.0, 0.0, 1.0), uv=Trim('C', lane='each'))
        m.board((px, y, DECK - 0.2), (px, y, TOP + 0.08), 0.12, 0.12, face=(side, 0.0, 0.0), uv=Trim('C', lane='each'),
                bevel=0.012)
        m.board((side * (WIDE + 0.38), y, DECK - PLANK - 0.1), (px + side * 0.04, y, DECK + 0.45), 0.09, 0.07,
                face=(0.0, -1.0, 0.0), uv='C')
    kit.log(m, (px, -HALF - 0.05, TOP), (px, HALF + 0.05, TOP), 0.055, (side, 0.0, 0.0), sides=8)
    # The mid rail: whole on one side, broken and patched on the other.
    if side < 0:
        m.board((px + side * 0.075, -HALF + 0.1, DECK + 0.5), (px + side * 0.075, HALF - 0.1, DECK + 0.5), 0.14, 0.04,
                face=(side, 0.0, 0.0), uv='A')
    else:
        m.board((px + side * 0.075, -HALF + 0.1, DECK + 0.5), (px + side * 0.075, 0.35, DECK + 0.5), 0.14, 0.04,
                face=(side, 0.0, 0.0), uv='A')
        m.board((px + side * 0.075, 0.6, DECK + 0.47), (px + side * 0.075, HALF - 0.1, DECK + 0.53), 0.14, 0.04,
                face=(side, 0.0, 0.0), uv='A')
        m.board((px + side * 0.06, 0.2, DECK + 0.62), (px + side * 0.06, 0.95, DECK + 0.4), 0.13, 0.035,
                face=(side, 0.0, 0.0), uv='A', lift=0.05)
m.section('railings')

# Collision: the deck, the ramps (sloped hulls) and the railings.
m.hull((2 * WIDE, 2 * HALF, 0.2), at=(0.0, 0.0, DECK - 0.1))
for sign in (-1.0, 1.0):
    m.hull_points([(x, sign * y, z) for x in (-WIDE + 0.05, WIDE - 0.05)
                   for y, z in ((HALF, DECK), (HALF, DECK - 0.2), (HALF + RAMP, 0.0), (HALF + RAMP, -0.1))])
for side in (-1.0, 1.0):
    m.hull((0.14, 2 * HALF, TOP + 0.08 - DECK), at=(side * (WIDE + 0.08), 0.0, (DECK + TOP + 0.08) * 0.5))

m.finish(view=(-1.3, -1.1, 0.55))
