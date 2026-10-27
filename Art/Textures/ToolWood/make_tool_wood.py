"""ToolWood: pale, straight-grained tool-handle wood, made to be tinted (the melee tools, Art/Models/Weapons/Melee/):
ash (the Grave Spade's shaft), weathered grey ash (the Ferry Gaff's pole), hickory (the Rock Pick's handle). GunWood is
walnut and only tints darker, so the pale woods need their own set. Long, tight grain along U (box_uv(along='long')
lays it along a handle), fine pores, darker grime where hands don't reach, worn lighter and glossier where they do.
1024 px tiling at 1024 px/m (held close in first person). Fixed seeds: every run writes the same maps.

Run with Blender (looter_textures' numpy helpers):
    Tools\\artrun.ps1 -Script Art\\Textures\\ToolWood\\make_tool_wood.py
Writes T_ToolWood_BC / _N / _ORM here; the model importer brings them to /Game/Art/Textures/ToolWood.
"""
import numpy as np

import looter_textures as lt
from looter_textures import blur, mix, noise, occlusion, rgb, smooth, specks, wood_grain

SIZE = 1024
PX_M = 1.0 / 1024.0


def make():
    shape = (SIZE, SIZE)
    # Straight, tight grain (ash and hickory are ring-porous: a band of pores at each ring's start).
    rings, streaks, fibers, knot = wood_grain(shape, 7711, period=6.0, warp=0.6, knots=1, knot_size=8.0)
    early = np.roll(rings, 2, axis=0)
    pores = specks(shape, 7712, 0.05, 0.6) * (0.4 + 0.6 * smooth(0.05, 0.4, early))
    wear = smooth(0.3, 1.6, noise(shape, 7713, 220.0, 50.0, octaves=2))
    grime = smooth(0.2, 1.4, noise(shape, 7714, 300.0, 50.0, octaves=2))
    dings = specks(shape, 7715, 0.003, 2.0)
    scratches = np.clip(blur(specks(shape, 7716, 0.0015, 1.0), 9.0, 0.6) * 20.0, 0.0, 1.0)
    base = mix(rgb(0xe8dcc6), rgb(0xf2e8d6), smooth(-1.0, 1.0, noise(shape, 7717, 220.0, 30.0)))
    color = mix(base, rgb(0xa08560), np.clip(rings * 0.85 + 0.3 * np.maximum(streaks, 0.0), 0.0, 1.0))
    color = color * (1.0 + 0.04 * fibers)[..., None]
    color = mix(color, rgb(0x8a7258), np.maximum(knot * 0.7, pores * 0.45))
    color = mix(color, rgb(0x9a8a74), grime * 0.22)
    color = mix(color, rgb(0xfaf4ea), wear * 0.2)
    color = mix(color, rgb(0x6a5848), dings * 0.5)
    color = mix(color, rgb(0xd8ccb8), scratches * 0.5)
    height = 0.00010 * rings - 0.00014 * pores + 0.00005 * fibers - 0.0002 * dings - 0.00006 * scratches
    ao = occlusion(height, PX_M, (2, 5), 0.5)
    rough = 0.62 + 0.08 * rings + 0.1 * pores + 0.08 * grime - 0.2 * wear
    lt.save_set('ToolWood', color, height, PX_M, np.clip(rough, 0.0, 1.0), None, ao=ao, normal_strength=0.8)


make()
