"""The ammo boxes (AAmmoPickup): one model per ammo type, SM_AmmoBox<Type>. Every type is the same olive can with a
stenciled band, so no box reads as a rarity color; only the cartridges standing on top tell the types apart (fat red
shells, long sniper rounds, ...).

Ported from the box the game used to build at runtime (Tools/Blender/looter_port.py): the origin is the middle of the
can's bottom, and the old code's 1.6x display scale is built in.
"""
import looter_model as lm
from looter_port import Part, transform

CAN, BAND, BRASS, TIP, HULL = range(5)
materials = [
    lm.material('AmmoCan', 0x4d5a38, Variation=0.06),  # olive drab
    lm.material('AmmoBand', 0xe6dcc0, Variation=0.0, Glow=0.35),  # a faint glow, so boxes catch the eye in the grass
    lm.material('AmmoBrass', 0xd6a64c, Variation=0.05),
    lm.material('AmmoTip', 0xb8683a, Variation=0.05),
    lm.material('AmmoHull', 0xb8392c, Variation=0.05),
]
SCALE = 1.6

# Per type (named like EAmmoType): cartridge radius, height and how many stand on the box; shells are shotgun hulls.
CARTRIDGES = {
    'AssaultRifle': (1.1, 7.0, 4),
    'Shotgun': (2.0, 6.0, 3),
    'Pistol': (1.0, 4.0, 5),
    'SMG': (0.9, 5.0, 6),
    'Sniper': (1.3, 11.0, 3),
}

for ammo, (radius, height, count) in CARTRIDGES.items():
    box = Part(f'AmmoBox{ammo}')
    box.box(CAN, (0.0, 0.0, 6.0), (20.0, 13.0, 12.0))
    box.box(BAND, (0.0, 0.0, 6.5), (20.6, 13.6, 3.2))
    box.box(CAN, (0.0, 0.0, 12.4), (21.0, 14.0, 1.2))  # lid
    box.box(CAN, (0.0, 0.0, 13.6), (8.0, 2.0, 1.2))  # handle

    spacing = min(radius * 2.6, 16.0 / max(count - 1, 1))
    case_height = height * 0.65
    shells = ammo == 'Shotgun'
    for index in range(count):
        base = ((index - (count - 1) * 0.5) * spacing, 3.2, 13.0)
        if shells:
            # A red plastic hull on a brass base.
            box.cylinder(BRASS, transform(base), radius, height * 0.25, 8)
            box.cylinder(HULL, transform((base[0], base[1], base[2] + height * 0.25)), radius * 0.95, height * 0.75, 8)
        else:
            # A brass case with a copper tip.
            box.cylinder(BRASS, transform(base), radius, case_height, 8)
            box.cone(TIP, transform((base[0], base[1], base[2] + case_height)), radius, radius * 0.3, height - case_height, 8)
    box.scale(SCALE)
    model = box.build(materials)
    model['Nanite'] = 0
