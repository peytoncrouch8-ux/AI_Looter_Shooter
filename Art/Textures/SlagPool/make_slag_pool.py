"""Slag Bomb's pool (Art/Models/Props/SlagPool.py), its crust: the texture set SlagPool, script-made (the user's rule: no
texture libraries). Run with Blender's Python (numpy and Tools/Blender/looter_textures.py's helpers):

    Tools\\artrun.ps1 -Script Art\\Textures\\SlagPool\\make_slag_pool.py

It writes T_SlagPool_BC, _N (DirectX) and _ORM here (1024 px, tiling), a quick look in Intermediate/Textures_scratch,
and the importer brings them to /Game/Art/Textures/SlagPool with the model (MI_SlagCrust on M_World).

The look: cooled iron slag, iron-black and a little rough. Gas pits (vesicles) with glassy lips, low ropy wrinkles where
it flowed, shrinkage cracks, rust blooms and a pale ash dusting in the hollows; dull, with a few glassy smooth spots.
Scale: the pool is modelled 1 m across and scaled to the power's radius (4.5 m: 9x); the model maps this texture once
per 0.2 m of the model, so one tile covers 1.8 m in game (px_m below), and the relief is sized for that.
"""
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Tools', 'Blender'))
import looter_textures as lt  # noqa: E402 - needs the path above

SIZE = 1024
TILE_M = 1.8                     # one tile in game, at the pool's usual 9x
PX_M = TILE_M / SIZE


def pits(shape, rng, count, r_lo, r_hi):
    """Gas pits: a bowl per pit (0..1 deep) and a raised lip just outside it (0..1), wrapping. Radii in px."""
    h, w = shape
    hole = np.zeros(shape, np.float32)
    lip = np.zeros(shape, np.float32)
    radii = np.exp(rng.uniform(math.log(r_lo), math.log(r_hi), count))
    xs = rng.uniform(0, w, count)
    ys = rng.uniform(0, h, count)
    for px, py, r in zip(xs, ys, radii):
        R = int(math.ceil(r * 1.8)) + 1
        oy, ox = np.mgrid[-R:R + 1, -R:R + 1].astype(np.float32)
        fx, fy = px - math.floor(px), py - math.floor(py)
        d = np.hypot(ox - fx, oy - fy) / r
        bowl = np.clip(1.0 - d * d, 0.0, 1.0) ** 0.6
        ring = np.exp(-((d - 1.15) / 0.28) ** 2) * (d > 0.9)
        rows = (np.arange(-R, R + 1) + int(math.floor(py))) % h
        cols = (np.arange(-R, R + 1) + int(math.floor(px))) % w
        ix = np.ix_(rows, cols)
        hole[ix] = np.maximum(hole[ix], bowl * min(1.0, r / r_hi + 0.35))
        lip[ix] = np.maximum(lip[ix], ring)
    return hole, lip * (1.0 - np.clip(hole * 4.0, 0.0, 1.0))


def main():
    shape = (SIZE, SIZE)
    rng = np.random.default_rng(4711)
    x, y = lt.coords(shape)
    # --- relief (meters, at game scale) ---
    swell = lt.noise(shape, 11, 95.0, 95.0, octaves=2)                     # the crust's broad swells
    lumps = lt.noise(shape, 12, 18.0, 18.0, octaves=3)                     # knobbly lumps
    # Ropy wrinkles where it flowed: warped stripes in patches.
    warp = lt.noise(shape, 13, 70.0, 70.0) * 40.0 + lt.noise(shape, 22, 20.0, 20.0) * 6.0
    phase = (x * 0.55 + y * 0.83 + warp) / 21.0
    ropes = np.sin(phase * 2.0 * math.pi) * lt.smooth(-0.2, 1.0, lt.noise(shape, 14, 60.0, 60.0))
    ropes *= 0.6 + 0.4 * lt.smooth(-1.0, 1.0, lt.noise(shape, 15, 9.0, 9.0))
    hole, lip = pits(shape, rng, 1100, 1.4, 8.5)
    big_hole, big_lip = pits(shape, rng, 70, 7.0, 15.0)
    hole = np.maximum(hole, big_hole)
    lip = np.maximum(lip, big_lip)
    cells = lt.worley(shape, (6, 6), 16, jitter=0.85,
                      x=x + lt.noise(shape, 17, 30.0, 30.0) * 14.0, y=y + lt.noise(shape, 18, 30.0, 30.0) * 14.0)
    cracks = 1.0 - lt.smooth(0.4, 1.8, cells['edge'])
    cracks *= lt.smooth(-0.6, 0.4, lt.noise(shape, 19, 40.0, 40.0))       # some cell walls stay whole
    fine = np.clip(lt.crack_lines(shape, 20, 30.0, 60.0, 0.7, 0.25) + lt.crack_lines(shape, 21, 26.0, 50.0, 0.7, 0.2, along_y=True), 0, 1)
    height = (0.008 * swell + 0.0028 * lumps + 0.0009 * ropes - 0.006 * hole + 0.0009 * lip
              - 0.0035 * cracks - 0.0007 * fine)
    ao = lt.occlusion(height, PX_M, (2, 6, 16), 1.0)
    curv = lt.curvature(height, PX_M, 2.0)

    # --- colour (sRGB) ---
    base = lt.mix(lt.rgb(0x1a1817), lt.rgb(0x2b2725), lt.smooth(-1.2, 1.4, lt.noise(shape, 30, 40.0, 40.0, octaves=2)))
    base = lt.mix(base, lt.rgb(0x1c1d21), lt.smooth(0.3, 1.5, lt.noise(shape, 31, 80.0, 80.0)) * 0.6)   # blue-black iron
    color = base * (1.0 + 0.10 * lt.noise(shape, 32, 2.0, 2.0))[..., None]                            # grit
    rust = lt.smooth(0.7, 1.7, lt.noise(shape, 33, 70.0, 70.0) + 0.6 * lt.noise(shape, 34, 12.0, 12.0))
    color = lt.mix(color, lt.mix(lt.rgb(0x3f2618), lt.rgb(0x5a3420), lt.smooth(-1, 1, lt.noise(shape, 35, 6.0, 6.0))),
                   rust * 0.4)
    hollow = np.clip((1.0 - ao) * 2.2, 0.0, 1.0)
    ash = np.clip(lt.smooth(0.5, 1.6, lt.noise(shape, 36, 45.0, 45.0)) * 0.45 + hollow * 0.35 * (1.0 - hole), 0.0, 1.0)
    ash *= 0.7 + 0.3 * lt.smooth(-1.0, 1.0, lt.noise(shape, 37, 3.0, 3.0))
    color = lt.mix(color, lt.rgb(0x625d57), ash * 0.6)
    ridge = np.clip(curv * 0.6, 0.0, 1.0)
    color = lt.mix(color, lt.rgb(0x403b37), ridge * 0.45 + lip * 0.25)                                # worn knobs, lips
    color = lt.mix(color, lt.rgb(0x0c0b0b), np.clip(hole * 1.3, 0.0, 1.0) * 0.85 + cracks * 0.6)      # pits, cracks
    color = color * (0.78 + 0.22 * ao)[..., None]

    # --- roughness: dull crust, ash duller, a few glassy smooth spots (slag glass) on the knobs and pit lips ---
    glass = lt.smooth(0.9, 1.7, lt.noise(shape, 40, 14.0, 14.0)) * (0.4 + 0.6 * np.clip(ridge + lip, 0.0, 1.0))
    rough = 0.84 + 0.05 * lt.noise(shape, 41, 6.0, 6.0) + 0.1 * ash - 0.38 * glass + 0.05 * hole
    lt.save_set('SlagPool', color, height, PX_M, np.clip(rough, 0.35, 1.0), ao=ao, normal_strength=1.0)


main()
