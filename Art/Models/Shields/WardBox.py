"""The ward box (SHIELD, 2026-10-22; Docs/Work/2026-10-22-Next/SHIELD.md): the player's shield as loot. A frontier-tech
belt box, the kind a Teropa prospector buys off a saint-house's tinker: a riveted iron case holding a soul-coal behind a
barred window, its light thrown out round the wearer as a ward that soaks hits. Worn on the hip (a leather loop at its
back); seen in the game as loot (AShieldPickup shows it 1.7 times as big, spinning under its rarity's beam) and as an
Inked icon. A scripted model file (Art/README.md); one model per rarity, each the one under it with more on it:

  WardBox_Common      the plain box: an iron case 14 x 7 x 11 cm in a worn blue-grey paint, a dark steel frame round its
                      front with rivets, the coal window (7.4 x 5.4 cm, WardGlow: the game lights it in the rarity's colour)
                      behind four upright bars (a stove grate), a hinged lid with the coal's damper on top (a small valve wheel),
                      a leather belt loop on the back.
  WardBox_Uncommon    + brass caps on its eight corners.
  WardBox_Rare        + a pressure gauge on its right side (brass rim, pale face, a dark needle).
  WardBox_Epic        + a brass ward-sigil inlaid on the lid (a ring crossed by four spokes, a dot at each end).
  WardBox_Legendary   + a saint's medal hung from the frame's top on a short chain, over the window's corner.

Pivot: the middle of its foot; its front (-Y) is the window. About 13.5 cm to the wheel's top. No collision (the pickup has its
own sphere), no Nanite (small, seen close), no LODs. Materials (7 slots), from the textured sets the game already loads:
  WardIron     PaintWorn, blue-grey paint         WardSteel    MetalWorn, dark steel (frame, bars, rivets, hinge)
  WardBrass    MetalWorn, brass                   WardLeather  Polymer, oiled brown leather
  WardFace     Polymer, the gauge's pale face     WardInk      Polymer, the needle and the gauge's marks
  WardGlow     the flat glow (Color, Glow): the coal window, lit per rarity by the game (UShieldDefinition::GlowSlot)

    blender -b --factory-startup --python-expr "import sys; sys.path.insert(0, 'Tools/Blender')" \\
        --python Art/Models/Shields/WardBox.py -- --preview
    (or Tools\\artrun.ps1 -Script Art\\Models\\Shields\\WardBox.py -Preview)
renders Saved/ArtPreviews/Shields/WardBox.png (all five, three-quarter front), WardBox_Legendary.png and WardBox_Back.png
(the Legendary close, from the front and from behind: the loop and the gauge).
"""
import math
import random

import bpy
from mathutils import Vector

import looter_model as lm
import looter_props as lp
import looter_textures as lt

# --- Sizes (metres) ---
W = 0.14                 # the case, side to side (X)
D = 0.07                 # front to back (Y; the window faces -Y)
H = 0.11                 # foot to the lid's seam
BEVEL = 0.005
LID = 0.012              # the lid's thickness over the seam
WIN_W = 0.12 * 0.62      # the window's opening (it reads about 7.4 x 5.4 cm)
WIN_H = 0.054
WIN_Z = 0.052            # the window's middle, up from the foot
FRAME = 0.009            # the steel frame round the window, its width
SEGMENTS = 16

IRON = lt.material('PaintWorn', name='WardIron', tint=0x55606c)
STEEL = lt.material('MetalWorn', name='WardSteel', tint=0x3a3e44)
BRASS = lt.material('MetalWorn', name='WardBrass', tint=0xd2a24e)
LEATHER = lt.material('Polymer', name='WardLeather', tint=0x4b3524)
FACE = lt.material('Polymer', name='WardFace', tint=0xe6dfcc)
INK = lt.material('Polymer', name='WardInk', tint=0x2a2622)
GLOW = lm.material('WardGlow', 0xa8ecff, Glow=2.4, Variation=0.0)

RARITIES = ('Common', 'Uncommon', 'Rare', 'Epic', 'Legendary')

RNG = random.Random(2210)


def part(size, center, mat, set_name, bevel=0.0, rotation=(0.0, 0.0, 0.0)):
    obj = lp.block(size, center, rotation, bevel)
    lt.assign(obj, mat)
    lt.box_uv(obj, set_name)
    return obj


def cylinder(radius, depth, center, axis, mat, set_name, segments=SEGMENTS):
    """A short cylinder of radius and depth along axis ('X', 'Y' or 'Z') at center."""
    profile = [(0.0, -depth * 0.5), (radius, -depth * 0.5), (radius, depth * 0.5), (0.0, depth * 0.5)]
    obj = lp.lathe(profile, segments=segments)[0]
    rot = {'X': (0.0, 90.0, 0.0), 'Y': (90.0, 0.0, 0.0), 'Z': (0.0, 0.0, 0.0)}[axis]
    lp.place(obj, center, rot)
    lt.assign(obj, mat)
    lp.lathe_uv(obj, set_name)
    return obj


def case():
    """The iron case and its lid, the lid a hair wider so its seam reads, two hinge knuckles at the back."""
    parts = [part((W, D, H), (0.0, 0.0, H * 0.5), IRON, 'PaintWorn', BEVEL),
             part((W + 0.004, D + 0.004, LID), (0.0, 0.0, H + LID * 0.5 - 0.001), IRON, 'PaintWorn', 0.003)]
    for x in (-W * 0.3, W * 0.3):
        parts.append(cylinder(0.0042, 0.026, (x, D * 0.5 + 0.002, H - 0.001), 'X', STEEL, 'MetalWorn', 10))
    return parts


def window():
    """The coal window: the glowing pane set back in the case's front, the steel frame round it, three bars and a crossbar
    across it, a rivet at each of the frame's corners and middles."""
    y_front = -D * 0.5
    parts = [part((WIN_W, 0.004, WIN_H), (0.0, y_front + 0.0012, WIN_Z), GLOW, 'Polymer')]
    # The frame: four plates round the opening, proud of the case.
    fw, fh = WIN_W + FRAME * 2.0, WIN_H + FRAME * 2.0
    y = y_front - 0.0018
    parts.append(part((fw, 0.004, FRAME), (0.0, y, WIN_Z + WIN_H * 0.5 + FRAME * 0.5), STEEL, 'MetalWorn', 0.0012))
    parts.append(part((fw, 0.004, FRAME), (0.0, y, WIN_Z - WIN_H * 0.5 - FRAME * 0.5), STEEL, 'MetalWorn', 0.0012))
    parts.append(part((FRAME, 0.004, WIN_H), (-WIN_W * 0.5 - FRAME * 0.5, y, WIN_Z), STEEL, 'MetalWorn', 0.0012))
    parts.append(part((FRAME, 0.004, WIN_H), (WIN_W * 0.5 + FRAME * 0.5, y, WIN_Z), STEEL, 'MetalWorn', 0.0012))
    # The bars, a stove grate's (upright only: crossed, it read as a house window), set a little back from the frame so
    # the light shows between them.
    for k in (-1.5, -0.5, 0.5, 1.5):
        parts.append(part((0.0034, 0.0034, WIN_H), (k * WIN_W * 0.2, y + 0.0012, WIN_Z), STEEL, 'MetalWorn'))
    # Rivets on the frame.
    for rx in (-fw * 0.5 + FRAME * 0.5, 0.0, fw * 0.5 - FRAME * 0.5):
        for rz in (WIN_Z + fh * 0.5 - FRAME * 0.5, WIN_Z - fh * 0.5 + FRAME * 0.5):
            parts.append(cylinder(0.0022, 0.002, (rx, y - 0.0028, rz), 'Y', STEEL, 'MetalWorn', 8))
    return parts


def damper():
    """The coal's damper on the lid: a brass boss, a short stem and a small steel valve wheel with three spokes (the Inked
    icon's wheel)."""
    top = H + LID
    x = W * 0.22
    wheel = lp.lathe([(0.0098, -0.0016), (0.0125, -0.0016), (0.0125, 0.0016), (0.0098, 0.0016)], segments=20, closed=True)[0]
    lp.place(wheel, (x, 0.0, top + 0.0135))
    lt.assign(wheel, STEEL)
    lp.lathe_uv(wheel, 'MetalWorn')
    parts = [cylinder(0.0075, 0.004, (x, 0.0, top + 0.002), 'Z', BRASS, 'MetalWorn'),
             cylinder(0.0028, 0.011, (x, 0.0, top + 0.0085), 'Z', STEEL, 'MetalWorn', 10), wheel]
    for k in range(3):
        parts.append(part((0.021, 0.0022, 0.0018), (x, 0.0, top + 0.0135), STEEL, 'MetalWorn', rotation=(0.0, 0.0, 60.0 * k)))
    return parts


def belt_loop():
    """The leather loop on the back the belt passes through: a strap standing off the case top and bottom."""
    y0 = D * 0.5
    path = [Vector((0.0, y0 - 0.001, H * 0.86)), Vector((0.0, y0 + 0.012, H * 0.8)), Vector((0.0, y0 + 0.014, H * 0.5)),
            Vector((0.0, y0 + 0.012, H * 0.2)), Vector((0.0, y0 - 0.001, H * 0.14))]
    # Across the path (its side, here Y) the strap is 3 mm thick; along up (X) 4.4 cm wide.
    strap = lp.sweep(path, [(-0.0015, -0.022), (0.0015, -0.022), (0.0015, 0.022), (-0.0015, 0.022)], up=(1.0, 0.0, 0.0))
    lt.assign(strap, LEATHER)
    lt.box_uv(strap, 'Polymer')
    # Two brass rivets where it's fixed.
    rivets = [cylinder(0.0024, 0.003, (sx * 0.014, y0 + 0.0005, z), 'Y', BRASS, 'MetalWorn', 8)
              for sx in (-1, 1) for z in (H * 0.86, H * 0.14)]
    return [strap, *rivets]


def corner_caps():
    """Brass caps on the eight corners (Uncommon and up)."""
    parts = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            for z in (0.006, H - 0.006):
                parts.append(part((0.013, 0.013, 0.013), (sx * (W * 0.5 - 0.004), sy * (D * 0.5 - 0.004), z), BRASS, 'MetalWorn', 0.002))
    return parts


def gauge():
    """A pressure gauge on the right side (Rare and up): a brass rim, a pale face, six marks and a dark needle."""
    x = W * 0.5 + 0.003
    c = Vector((x, 0.0, WIN_Z + 0.008))
    parts = [cylinder(0.017, 0.006, (c.x, c.y, c.z), 'X', BRASS, 'MetalWorn', 20),
             cylinder(0.0138, 0.0012, (c.x + 0.0034, c.y, c.z), 'X', FACE, 'Polymer', 20)]
    for k in range(6):
        a = math.radians(-120.0 + k * 48.0)
        p = Vector((c.x + 0.0042, c.y + math.sin(a) * 0.0105, c.z + math.cos(a) * 0.0105))
        parts.append(part((0.0008, 0.0012, 0.003), (p.x, p.y, p.z), INK, 'Polymer', rotation=(math.degrees(-a), 0.0, 0.0)))
    a = math.radians(35.0)
    tip = Vector((c.x + 0.0046, c.y + math.sin(a) * 0.009, c.z + math.cos(a) * 0.009))
    mid = (tip + Vector((c.x + 0.0046, c.y, c.z))) * 0.5
    parts.append(part((0.0008, 0.0011, 0.0095), (mid.x, mid.y, mid.z), INK, 'Polymer', rotation=(math.degrees(-a), 0.0, 0.0)))
    parts.append(cylinder(0.0018, 0.0016, (c.x + 0.0048, c.y, c.z), 'X', BRASS, 'MetalWorn', 8))
    return parts


def sigil():
    """The ward-sigil inlaid on the lid (Epic and up): a brass ring crossed by four spokes, a dot at each spoke's end."""
    top = H + LID + 0.0002
    cx, cy = -W * 0.14, 0.0
    ring = lp.lathe([(0.0128, -0.0006), (0.0148, -0.0006), (0.0148, 0.0006), (0.0128, 0.0006)], segments=24, closed=True)[0]
    lp.place(ring, (cx, cy, top))
    lt.assign(ring, BRASS)
    lp.lathe_uv(ring, 'MetalWorn')
    parts = [ring]
    for k in range(4):
        a = math.radians(45.0 + k * 90.0)
        parts.append(part((0.026, 0.0016, 0.0012), (cx, cy, top), BRASS, 'MetalWorn', rotation=(0.0, 0.0, math.degrees(a))))
        parts.append(cylinder(0.0024, 0.0014, (cx + math.cos(a) * 0.0185, cy + math.sin(a) * 0.0185, top), 'Z', BRASS, 'MetalWorn', 8))
    return parts


def medal():
    """A saint's medal (Legendary): a gold disc on a short chain hung from the frame's top, by the window's corner."""
    y = -D * 0.5 - 0.006
    hook_z = WIN_Z + WIN_H * 0.5 + FRAME
    x = -WIN_W * 0.32
    parts = []
    for k in range(3):
        link = lp.lathe([(0.0016, -0.0007), (0.0026, -0.0007), (0.0026, 0.0007), (0.0016, 0.0007)], segments=10, closed=True)[0]
        lp.place(link, (x, y, hook_z - 0.004 - k * 0.0042), (90.0, 0.0, 90.0 * k))
        lt.assign(link, BRASS)
        lp.lathe_uv(link, 'MetalWorn')
        parts.append(link)
    disc_z = hook_z - 0.004 - 3 * 0.0042 - 0.009
    parts.append(cylinder(0.0105, 0.0022, (x, y, disc_z), 'Y', lt.material('MetalWorn', name='WardGold', tint=0xf0c862), 'MetalWorn', 20))
    # The saint's figure in relief: a raised oval and a small cross over it.
    parts.append(cylinder(0.0055, 0.0012, (x, y - 0.0016, disc_z - 0.001), 'Y', BRASS, 'MetalWorn', 12))
    parts.append(part((0.0012, 0.0012, 0.0065), (x, y - 0.0022, disc_z + 0.0035), BRASS, 'MetalWorn'))
    parts.append(part((0.0045, 0.0012, 0.0012), (x, y - 0.0022, disc_z + 0.0048), BRASS, 'MetalWorn'))
    return parts


def build(rarity_index):
    name = 'WardBox_' + RARITIES[rarity_index]
    parts = [*case(), *window(), *damper(), *belt_loop()]
    if rarity_index >= 1:
        parts += corner_caps()
    if rarity_index >= 2:
        parts += gauge()
    if rarity_index >= 3:
        parts += sigil()
    if rarity_index >= 4:
        parts += medal()
    model = lp.join(name, parts)
    lp.finish(model, ao=0.03, nanite=False, smooth=40.0)
    model['Collision'] = 'None'
    return model


for _obj in list(bpy.data.objects):   # the startup scene's cube, light and camera
    bpy.data.objects.remove(_obj)
models = []
for index in range(len(RARITIES)):
    box = build(index)
    # Side by side for the preview; the export takes each from its own origin.
    box.location = (index * 0.22, 0.0, 0.0)
    models.append(box)
    lt._log(f'{box.name}: {lp.tri_count(box)} triangles, materials {", ".join(m.name for m in box.data.materials)}')

if lt.want_preview():
    bpy.context.view_layer.update()
    lt.preview(models, lt.preview_path('Shields', 'WardBox'), view=(-0.25, -1.5, 0.45), fit=0.62)
    lt.preview([models[4]], lt.preview_path('Shields', 'WardBox_Back'), view=(0.9, 1.2, 0.6), fit=0.9)
    lt.preview([models[4]], lt.preview_path('Shields', 'WardBox_Legendary'), view=(-0.5, -1.4, 0.5), fit=0.9)
