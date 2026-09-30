"""The pump shotgun's parts (DA_PumpShotgun): the body, and the parts a rolled shotgun picks by its seed: three barrel
lengths and, on epics and better, a heat shroud with glowing slots. The pump is its own part, which a reload moves.

Ported from the shotgun the game used to build at runtime (Tools/Blender/looter_port.py), in the gun's own space: the
origin is the back of the receiver and +X points to the muzzle. Sockets the game reads: Grip on the body; Muzzle, Pump
and Shroud (where those hang) on each barrel; Foregrip (the left hand) on the pump, which a reload slides back along
its -X.
"""
import looter_model as lm
from looter_port import ALONG_X, Part, rotator, transform

METAL, PAINT, ACCENT, GRIP = range(4)
materials = [
    lm.material('GunMetal', 0x2e343d, Variation=0.05),
    # Every gun paints these three its own way: its paint, its rarity's color and glow, its grip (wood or rubber).
    lm.material('GunPaint', 0xe4dccb, Variation=0.06),
    lm.material('GunAccent', 0xf2f2f2, Variation=0.0, Glow=2.5),
    lm.material('GunGrip', 0x23272e, Variation=0.06),
]
LENGTHS = (30.0, 34.0, 38.0)


def finish(part, frame=None):
    """Softens the hard boxes a touch, so they read hand-made rather than CAD (by position in the gun's space, so the
    parts still fit together), and makes the model. Guns are small and held close: no Nanite."""
    part.displace(0.25, 1.0 / 6.0, 0.0)
    obj = part.build(materials, frame)
    obj['Nanite'] = 0
    return obj


def pump_center(length):
    return (21.0 + length * 0.45, 0.0, -2.4)


def shroud_center(length):
    return (21.0 + length * 0.8, 0.0, 2.0)


body = Part('ShotgunBody')
body.box(METAL, (10.0, 0.0, 0.0), (22.0, 6.6, 10.0))  # receiver
for side in (-3.4, 3.4):
    body.box(PAINT, (10.0, side, -1.5), (18.0, 0.5, 5.0))  # side plates
    body.box(ACCENT, (10.0, side * 1.05, 2.3), (14.0, 0.5, 1.3))  # glow strips
body.box(GRIP, (1.0, 0.0, -8.0), (4.4, 3.8, 10.0), rotator(pitch=-18.0))  # grip
body.box(METAL, (7.0, 0.0, -5.8), (6.0, 1.2, 1.0))  # trigger guard
body.box(GRIP, (-11.0, 0.0, -2.5), (20.0, 5.0, 7.6), rotator(pitch=-5.0))  # stock
body.box(METAL, (-21.5, 0.0, -3.5), (2.0, 5.4, 9.0))  # butt pad
body.socket('Grip', (1.0, 0.0, -5.0))
finish(body)

for length in LENGTHS:
    barrel = Part(f'ShotgunBarrel{length:.0f}')
    barrel.cylinder(METAL, transform((21.0, 0.0, 2.0), ALONG_X), 2.2, length, 8)
    barrel.cylinder(METAL, transform((21.0, 0.0, -2.6), ALONG_X), 1.8, length - 6.0, 8)  # magazine tube
    barrel.box(PAINT, (21.0 + length * 0.5, 0.0, 3.8), (length, 1.1, 0.9))  # vent rib
    barrel.box(ACCENT, (20.0 + length, 0.0, 4.6), (1.0, 1.0, 1.0))  # bead
    barrel.socket('Muzzle', (21.0 + length, 0.0, 2.0))
    barrel.socket('Pump', pump_center(length))
    barrel.socket('Shroud', shroud_center(length))
    finish(barrel)

# Built where it sits on the middle barrel (for the softening noise), then centered on its own origin.
middle = LENGTHS[1]
pump = Part('ShotgunPump')
pump.box(GRIP, pump_center(middle), (14.0, 6.2, 5.6))
pump.socket('Foregrip', (0.0, 0.0, -1.6))
finish(pump, transform(pump_center(middle)))

shroud = Part('ShotgunShroud')
x, _, z = shroud_center(middle)
shroud.box(PAINT, (x, 0.0, z), (10.0, 5.6, 5.6))
for side in (-2.9, 2.9):
    shroud.box(ACCENT, (x, side, z), (7.0, 0.4, 1.2))  # glowing slots
finish(shroud, transform(shroud_center(middle)))
