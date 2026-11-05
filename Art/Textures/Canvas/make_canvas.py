"""Canvas: heavy woven sailcloth in a neutral off-white, made to be tinted (the Lamplighter's gas bag, sacking, awnings):
the weave (warp along V, weft along U: fine threads over and under), thicker slub threads here and there, soft
creases, water marks and soot run down from where it hangs, darker and rougher where it is dirty, a little lighter and
smoother where it is rubbed. 1024 px tiling at 512 px/m (2 m a tile: a ship's bag seen from its deck). Fixed seeds:
every run writes the same maps. The rebake (Tools/Blender/looter_tripo_zones.py) and the detail layer read it like
PaintWorn or MetalWorn.

Run with Blender (looter_textures' numpy helpers):
    Tools\\artrun.ps1 -Script Art\\Textures\\Canvas\\make_canvas.py
Writes T_Canvas_BC / _N / _ORM here; the model importer brings them to /Game/Art/Textures/Canvas when a material uses them.
"""
import numpy as np

import looter_textures as lt
from looter_textures import blur, coords, crack_lines, mix, noise, occlusion, rgb, smooth, specks, streaks_below

SIZE = 1024
PX_M = 1.0 / 512.0


def make():
    shape = (SIZE, SIZE)
    x, y = coords(shape)
    # The weave: threads 2 px apart (4 mm, sailcloth's heavy duck), over and under in a checker, each thread uneven.
    warp = noise(shape, 9101, 0.5, 14.0)
    weft = noise(shape, 9102, 14.0, 0.5)
    over = np.sin(x * np.pi * 0.5) * np.sin(y * np.pi * 0.5)
    weave = 0.5 * (warp + weft) * 0.6 + over * 0.4
    slubs = np.clip(smooth(1.9, 2.6, noise(shape, 9103, 0.6, 60.0)) + smooth(1.9, 2.6, noise(shape, 9104, 60.0, 0.6)),
                    0.0, 1.0)
    creases = np.clip(crack_lines(shape, 9105, 90.0, 140.0, 0.9, 0.25, along_y=True) +
                      crack_lines(shape, 9106, 120.0, 100.0, 0.8, 0.15), 0.0, 1.0)
    stains = smooth(0.5, 1.9, noise(shape, 9107, 70.0, 70.0, octaves=3))
    tide = np.clip(stains * (1.0 - smooth(0.8, 1.0, stains)) * 3.0, 0.0, 1.0)       # a water mark's darker rim
    soot = np.clip(streaks_below(specks(shape, 9108, 0.01, 6.0), 120.0, 9109), 0.0, 1.0)
    soot *= smooth(0.0, 1.5, noise(shape, 9110, 4.0, 80.0))
    rubbed = smooth(0.6, 1.8, noise(shape, 9111, 120.0, 120.0, octaves=2))
    base = mix(rgb(0xddd5c4), rgb(0xebe5d8), smooth(-1.0, 1.0, noise(shape, 9112, 160.0, 160.0, octaves=2)) * 0.7)
    color = base * (1.0 + 0.05 * weave)[..., None]
    color = mix(color, rgb(0xf2ede2), slubs * 0.35)
    color = mix(color, rgb(0xb4a78e), stains * 0.25)
    color = mix(color, rgb(0x9a8a6c), tide * 0.3)
    color = mix(color, rgb(0x5e584e), soot * 0.35)
    color = mix(color, color * 0.86, creases * 0.5)
    color = mix(color, rgb(0xf4f0e6), rubbed * 0.1)
    height = 0.00018 * weave + 0.00025 * slubs - 0.0008 * blur(creases, 1.5) + 0.0012 * noise(shape, 9113, 90.0, 90.0)
    ao = occlusion(height, PX_M, (2, 6), 0.5)
    rough = 0.82 + 0.06 * stains + 0.06 * soot - 0.08 * rubbed + 0.03 * weave
    lt.save_set('Canvas', color, height, PX_M, np.clip(rough, 0.0, 1.0), None, ao=ao, normal_strength=0.9)


make()
