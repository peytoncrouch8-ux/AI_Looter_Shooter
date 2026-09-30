"""The tutorial's weapon pickup stand at the crossroads: a heavy plank table with a shelf, a rack of notched rails
behind it and a little tin canopy on braced arms, so the first gun lies in the shade where the player can't miss it.

A scripted model (Art/README.md) built with looter_buildings: the pivot is the middle of the stand at ground level and
the table's front faces -Y. SOCKET_Weapon is where the gun lies, on the tabletop: its front (-Y of the empty, the gun's
muzzle) points along +X, to the right as seen from the front, and its top is up.
"""
import looter_buildings as kit
from looter_buildings import Trim, math

m = kit.Model('GunRack', seed=93)
rng = m.rng

TOP = 0.92            # the tabletop's height
HX, HY = 0.78, 0.36   # half the tabletop's size

# The table: four planks on aprons and square legs, a low shelf.
for k, y in enumerate((-0.27, -0.09, 0.09, 0.27)):
    m.board((-HX - rng.uniform(0.0, 0.03), y, TOP - 0.025), (HX + rng.uniform(0.0, 0.03), y, TOP - 0.025), 0.175, 0.05,
            face=(0.0, 0.0, 1.0), uv='A')
for x in (-HX + 0.08, HX - 0.08):
    for y in (-HY + 0.07, HY - 0.07):
        m.board((x, y, 0.0), (x, y, TOP - 0.05), 0.09, 0.09, uv=Trim('C', lane='each'))
for y in (-HY + 0.07, HY - 0.07):
    m.board((-HX + 0.03, y, TOP - 0.12), (HX - 0.03, y, TOP - 0.12), 0.12, 0.04, face=(0.0, -1.0 if y < 0 else 1.0, 0.0),
            uv='C', lift=0.045)
for x in (-HX + 0.08, HX - 0.08):
    m.board((x, -HY + 0.07, 0.25), (x, HY - 0.07, 0.25), 0.08, 0.05, face=(1.0, 0.0, 0.0), uv='C')
for y in (-0.15, 0.12):
    m.board((-HX + 0.05, y, 0.3), (HX - 0.05, y, 0.3), 0.2, 0.035, face=(0.0, 0.0, 1.0), uv='A')
m.section('table')

# The rack behind: two posts, a notched rail with pegs, and a plank at the foot.
BACK_Y = HY + 0.12
POST_TOP = 2.25
for x in (-HX + 0.02, HX - 0.02):
    m.board((x, BACK_Y, 0.0), (x, BACK_Y, POST_TOP), 0.12, 0.12, uv=Trim('C', lane='each'), bevel=0.01)
for z in (1.05, 1.55):
    m.board((-HX - 0.1, BACK_Y - 0.08, z), (HX + 0.1, BACK_Y - 0.08, z), 0.12, 0.05, uv='A')
    for k in range(5):
        x = -0.56 + k * 0.28
        m.box((0.04, 0.12, 0.04), at=(x, BACK_Y - 0.16, z + 0.05), rot=(-20.0, 0.0, 0.0), uv='C')
m.board((-HX - 0.05, BACK_Y - 0.07, 0.18), (HX + 0.05, BACK_Y - 0.07, 0.18), 0.18, 0.04, uv='A')
# A red-painted plank nailed across the top of the rack: the stand's one spot of color.
m.board((-0.5, BACK_Y - 0.08, 1.9), (0.5, BACK_Y - 0.08, 1.93), 0.2, 0.035, uv='H2')
m.section('rack')

# The canopy: two arms from the post tops out over the table, braced, carrying a tin shed roof.
FRONT_Y = -HY - 0.32
ARM_Z = POST_TOP - 0.06
for x in (-HX + 0.02, HX - 0.02):
    m.board((x, BACK_Y + 0.1, ARM_Z + 0.02), (x, FRONT_Y, ARM_Z - 0.12), 0.12, 0.08, face=(1.0, 0.0, 0.0),
            uv=Trim('C', lane='each'))
    m.board((x, BACK_Y - 0.05, ARM_Z - 0.62), (x, BACK_Y - 0.55, ARM_Z - 0.06), 0.08, 0.07, face=(1.0, 0.0, 0.0), uv='C')
theta = math.atan2(0.14, BACK_Y - FRONT_Y)
over = 0.12
deck = 0.04
normal = kit.Vector((0.0, -math.sin(theta), math.cos(theta)))
origin = kit.Vector((-HX - 0.22, FRONT_Y - over, ARM_Z - 0.07 - over * math.tan(theta))) + normal * deck
canopy = kit.Slope(origin, (1.0, 0.0, 0.0), (0.0, math.cos(theta), math.sin(theta)), 2 * HX + 0.44,
                   (BACK_Y - FRONT_Y + 2 * over) / math.cos(theta), sag=0.02)
kit.roof_deck(m, canopy, deck)
kit.tin(m, canopy, sheets=(1, 2), missing=0.0, loose=0.3, replaced=0.5)
m.section('canopy')

# The gun lies across the middle of the tabletop, muzzle to the right.
m.socket('Weapon', (0.0, -0.05, TOP + 0.02), (0.0, 0.0, 90.0))

# Collision: the table, the rack and the canopy.
m.hull((2 * HX + 0.06, 2 * HY, TOP), at=(0.0, 0.0, TOP * 0.5))
m.hull((2 * HX + 0.2, 0.2, POST_TOP), at=(0.0, BACK_Y - 0.03, POST_TOP * 0.5))
m.hull((canopy.width, canopy.length, 0.12), at=(canopy.width * 0.5, canopy.length * 0.5, -0.01), space=canopy)

m.finish(view=(-0.9, -1.6, 0.6))
