"""The assault rifle's parts (DA_AssaultRifle): the body, and the parts a rolled rifle picks by its seed: three barrel
lengths, three sights and, on legendaries, glowing fins. The magazine is its own part, which a reload moves.

Ported from the rifle the game used to build at runtime (Tools/Blender/looter_port.py), in the gun's own space: the
origin is the back of the receiver and +X points to the muzzle. Sockets the game reads: Grip and Foregrip (where the
hands go) and Magazine (where the magazine hangs; a reload slides it out along its -Z) on the body, Muzzle on each
barrel.
"""
import looter_model as lm
from looter_port import ALONG_X, Part, rotator, transform

METAL, PAINT, ACCENT, GRIP = range(4)
materials = [
    lm.material('GunMetal', 0x2e343d, Variation=0.05),
    # Every gun paints these three its own way: its paint, its rarity's color and glow, its grip.
    lm.material('GunPaint', 0xe4dccb, Variation=0.06),
    lm.material('GunAccent', 0xf2f2f2, Variation=0.0, Glow=2.5),
    lm.material('GunGrip', 0x23272e, Variation=0.06),
]
MAGAZINE_TILT = rotator(pitch=10.0)


def finish(part, frame=None):
    """Softens the hard boxes a touch, so they read hand-made rather than CAD (by position in the gun's space, so the
    parts still fit together), and makes the model. Guns are small and held close: no Nanite."""
    part.displace(0.25, 1.0 / 6.0, 0.0)
    obj = part.build(materials, frame)
    obj['Nanite'] = 0
    return obj


body = Part('RifleBody')
body.box(METAL, (14.0, 0.0, 0.0), (30.0, 6.0, 9.0))  # receiver
body.box(PAINT, (14.0, 0.0, 5.4), (28.0, 5.4, 2.6))  # top cover
body.box(PAINT, (38.0, 0.0, -0.5), (20.0, 6.6, 8.0))  # handguard
for side in (-3.4, 3.4):
    body.box(ACCENT, (38.0, side, 0.5), (14.0, 0.6, 1.4))  # glow vents
    body.box(ACCENT, (12.0, side, -2.0), (8.0, 0.6, 1.0))
body.box(GRIP, (5.0, 0.0, -8.0), (4.2, 3.8, 10.0), rotator(pitch=-15.0))  # pistol grip
body.box(METAL, (11.0, 0.0, -5.6), (6.0, 1.2, 1.0))  # trigger guard
body.box(PAINT, (-9.0, 0.0, -1.0), (18.0, 4.6, 7.0))  # stock
body.box(GRIP, (-18.5, 0.0, -1.5), (2.0, 5.0, 9.0))  # butt pad
body.socket('Grip', (5.0, 0.0, -5.0))
body.socket('Foregrip', (38.0, 0.0, -4.0))
body.socket('Magazine', (20.0, 0.0, -9.0), MAGAZINE_TILT)
finish(body)

for length in (10.0, 14.0, 18.0):
    barrel = Part(f'RifleBarrel{length:.0f}')
    barrel.cylinder(METAL, transform((48.0, 0.0, 0.5), ALONG_X), 1.4, length, 8)
    barrel.cylinder(METAL, transform((48.0 + length, 0.0, 0.5), ALONG_X), 2.3, 5.0, 6)  # muzzle brake
    barrel.socket('Muzzle', (53.0 + length, 0.0, 0.5))
    finish(barrel)

magazine = Part('RifleMagazine')
magazine.box(METAL, (20.0, 0.0, -9.0), (5.0, 4.0, 12.0), MAGAZINE_TILT)
finish(magazine, transform((20.0, 0.0, -9.0), MAGAZINE_TILT))

iron = Part('RifleSightIron')
iron.box(METAL, (40.0, 0.0, 4.8), (1.5, 1.2, 3.0))  # front post
iron.box(METAL, (4.0, 0.0, 7.5), (2.0, 3.0, 2.0))  # rear notch
finish(iron)

red_dot = Part('RifleSightRedDot')
red_dot.box(METAL, (16.0, 0.0, 8.6), (6.0, 3.2, 3.6))
red_dot.box(ACCENT, (12.9, 0.0, 9.0), (0.4, 2.2, 2.2))  # lens
finish(red_dot)

scope = Part('RifleSightScope')
scope.box(METAL, (10.0, 0.0, 7.8), (3.0, 2.0, 2.0))  # rings
scope.box(METAL, (20.0, 0.0, 7.8), (3.0, 2.0, 2.0))
scope.cylinder(METAL, transform((6.0, 0.0, 10.0), ALONG_X), 2.3, 18.0, 8)
scope.cylinder(ACCENT, transform((23.9, 0.0, 10.0), ALONG_X), 1.9, 0.3, 8)  # lens
finish(scope)

# Legendaries: a pair of glowing fins along the handguard.
fins = Part('RifleFins')
for side in (-2.2, 2.2):
    fins.box(ACCENT, (39.0, side, 4.6), (16.0, 0.8, 2.2))
finish(fins)
