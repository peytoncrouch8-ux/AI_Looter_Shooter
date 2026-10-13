"""The silver pickup (ASilverPickup; Docs/Progression.md, "Economy"): a little pile of silver coins in the ammo pickups'
manner (Ammo.py): the icon of the thing modeled in 3D with the Inked icons' dark line, so the pile on the ground reads like
the coin beside the HUD's silver readout (UI/Vendor/SilverUI's coin: a disc with a raised rim).

SM_SilverCoins: a short stack of five coins, each a little off the one under it, with three loose ones round it, one
leaning on the stack. A coin is a disc 3.8 cm across and 2.6 mm thick with a raised rim and a sunk face, turned on a lathe;
the pickup shows it 1.6 times bigger, so it reads in the grass (about 13 cm across). Each coin wears the ink line: an
inverted hull, its faces turned inward, so a one-sided material shows only the rim behind the coin; it joins after the
ambient occlusion bake (baked with it, it would shade the coin black).

The origin is the middle of the pile's footprint on the ground, and ASilverPickup spins the pile about it. Materials:
tinted texture sets the game already loads (MetalWorn as silver, the ammo's ink line), so no new textures. No collision
(the pickup has its own sphere), no Nanite, no LODs.

    blender -b --factory-startup --python Art/Models/Loot/Coins.py -- --preview
"""
import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_textures as lt
import looter_props as lp

RADIUS = 0.019       # a silver dollar's 3.8 cm across
THICK = 0.0026       # and 2.6 mm thick
RIM = 0.0022         # the raised rim's width
SINK = 0.0005        # how far the face sits under the rim
INK_WIDTH = 0.0016   # the ink line's thickness (thinner than the ammo's: coins are small)
SEGMENTS = 24

SILVER = lt.material('MetalWorn', name='SilverCoin', tint=0xdfe3e6)
INK_LINE = lt.material('Polymer', name='AmmoInkLine', tint=0x0a1218)
INK_LINE.use_backface_culling = True          # as the game's one-sided material: only the rim behind the coin shows
INK_LINE.use_backface_culling_shadow = True   # and it casts no shadow over the coin inside it


def coin_profile():
    """A coin's lathe profile [(radius, height)]: the sunk face underneath, the rim, the sunk face on top."""
    return [(0.0, SINK), (RADIUS - RIM, SINK), (RADIUS - RIM * 0.6, 0.0), (RADIUS, 0.0), (RADIUS, THICK),
            (RADIUS - RIM * 0.6, THICK), (RADIUS - RIM, THICK - SINK), (0.0, THICK - SINK)]


def coin():
    """One coin lying flat with its bottom at the origin."""
    obj = lp.lathe(coin_profile(), segments=SEGMENTS)[0]
    lt.assign(obj, SILVER)
    lp.lathe_uv(obj, SILVER['TextureSet'])
    return obj


def ink_shell():
    """The ink line round one coin: its outline pushed out by INK_WIDTH, turned, its faces turned inward."""
    w = INK_WIDTH
    outline = [(0.0, -w), (RADIUS + w * 0.7, -w), (RADIUS + w, 0.0), (RADIUS + w, THICK), (RADIUS + w * 0.7, THICK + w),
               (0.0, THICK + w)]
    obj = lp.lathe(outline, segments=SEGMENTS)[0]
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    lt.assign(obj, INK_LINE)
    return obj


def placements(rng):
    """Where each coin lies: (spot, tilt about X in degrees, turn about Z in degrees), a stack and three loose ones."""
    spots = []
    # The stack, each coin a little off the one under it.
    drift = Vector((0.0, 0.0, 0.0))
    for i in range(5):
        drift += Vector((rng.uniform(-0.0012, 0.0012), rng.uniform(-0.0012, 0.0012), 0.0))
        spots.append((Vector((0.004, -0.002, THICK * i)) + drift, rng.uniform(-1.5, 1.5), rng.uniform(0.0, 360.0)))
    # Two lying flat beside it, and one leaning on it.
    spots.append((Vector((-0.036, 0.014, 0.0)), rng.uniform(-2.0, 2.0), rng.uniform(0.0, 360.0)))
    spots.append((Vector((0.016, 0.036, 0.0)), rng.uniform(-2.0, 2.0), rng.uniform(0.0, 360.0)))
    # Its low edge rests on the ground: lifted by its radius times the tilt's sine.
    spots.append((Vector((0.034, -0.03, RADIUS * math.sin(math.radians(24.0)))), 24.0, 125.0))
    return spots


def build():
    rng = random.Random(41)
    parts, shells = [], []
    for spot, tilt, turn in placements(rng):
        body = coin()
        shell = ink_shell()
        # Tilted about the coin's own edge-to-edge line, then turned, then set in its place.
        m = Matrix.Translation(spot) @ Matrix.Rotation(math.radians(turn), 4, 'Z') @ Matrix.Rotation(math.radians(tilt), 4, 'X')
        for p in (body, shell):
            p.data.transform(m)
            p.data.update()
        parts.append(body)
        shells.append(shell)
    model = lp.join('SilverCoins', parts)
    lp.finish(model, ao=0.01, nanite=False, smooth=40.0)
    # The ink joins after the ambient occlusion bake: baked with it, it would shade the coins inside it black.
    model = lp.join(model.name, [model] + shells)
    lm.smooth(model, 40.0)
    model['Nanite'] = 0
    model['Collision'] = 'None'
    return model


for _obj in list(bpy.data.objects):   # the startup scene's cube, light and camera
    bpy.data.objects.remove(_obj)
pile = build()
lt._log(f'{pile.name}: {sum(len(p.vertices) - 2 for p in pile.data.polygons)} triangles, '
        f'materials {", ".join(m.name for m in pile.data.materials)}')

if lt.want_preview():
    bpy.context.view_layer.update()
    lt.preview([pile], lt.preview_path('Loot', 'Coins'), view=(-0.12, -0.3, 0.16))
