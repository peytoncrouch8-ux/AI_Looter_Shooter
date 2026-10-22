"""The painted ghost's cloth (CHAR-ART item 1, the user's OK 2026-10-22; sheet Saved/ArtPreviews/Characters/Concepts/
CharArt_1_Unpaid_PaintedGhost.png), script-made (no texture libraries), for M_Ghost (Tools/Unreal/build_creature_materials.py):

    T_GhostCloth_M.png   linear RGB data, tileable, 1024 px (M_Ghost repeats it ClothScale times a meter, 1.25: a tile is
                         80 cm of cloth, ~1.3 px a millimetre):
                         R  the weave, mean 0.5: plain-woven threads (~4.7 mm a pair, under the ~300/m that aliases), a
                            slub here and there, the darker lines of a few sewn seams with their stitches beside them
                         G  folds, mean 0.5: long soft creases hanging down the cloth (V runs down a ghost's sides and
                            down the shroud's strips), a few broad drapes across them; above 0.5 a ridge catching the
                            light, below a crease in shadow
                         B  stains, 0 clean: grave-dirt blotches with darker tide-mark rims, sweat and damp below
M_Ghost multiplies each zone's tint by them (the weave and seams mostly on the clothes zones, the folds and stains on all),
so every ghost keeps its colours. One texture sample more than the old graph; the Polymer grain stays (Amos's hat reads its
Hay through the same parameter).

Run it with Blender's Python (numpy only; Tools/Blender/looter_textures.py's helpers), reproducible (fixed seeds):
    "C:\\Program Files\\Blender Foundation\\Blender 4.4\\4.4\\python\\bin\\python.exe" -I Art\\Textures\\GhostCloth\\make_ghost_cloth.py
or Tools\\artrun.ps1 -Script Art\\Textures\\GhostCloth\\make_ghost_cloth.py. It writes the map here and a look
(Saved/ArtPreviews/Characters/CharMat_GhostCloth.png: the channels, and the map over the Unpaid's four tints). The editor
imports it with M_Ghost: Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_creature_materials.py M_Ghost".
"""
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Tools', 'Blender'))
import looter_textures as lt  # noqa: E402 - needs the path above

SIZE = 1024
NAME = 'GhostCloth'
OUT = os.path.join(HERE, f'T_{NAME}_M.png')
LOOK = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Characters', 'CharMat_GhostCloth.png')
THREAD = 6.0                    # px a thread pair (plain weave: over one, under one)
SEAMS_X = (0.31,)               # vertical seams, share of the tile across
SEAMS_Y = (0.72,)               # a horizontal seam (a yoke, a hem let down)
# The Unpaid's four zone tints (Unpaid.py ZONE_COLORS) for the look.
ZONES = (0xBAC4C6, 0x93A5B2, 0x6C5D50, 0xA65E4C)


def weave(shape):
    """Plain weave, 0..1 around 0.5: each crossing a rounded bump (warp over weft, then weft over warp), the threads'
    thickness wandering (slubs), a faint wobble so the grid isn't ruled."""
    x, y = lt.coords(shape)
    wx = x + 1.2 * lt.noise(shape, 11, 30.0, octaves=2)
    wy = y + 1.2 * lt.noise(shape, 12, 30.0, octaves=2)
    u = wx / THREAD * 2.0 * np.pi
    v = wy / THREAD * 2.0 * np.pi
    warp = 0.5 + 0.5 * np.cos(u)                     # the warp threads (along y), bright on their crowns
    weft = 0.5 + 0.5 * np.cos(v)
    over = np.cos(u * 0.5) * np.cos(v * 0.5) > 0.0   # which thread is on top at this crossing
    cloth = np.where(over, warp ** 0.7, weft ** 0.7)
    slub_x = lt.noise((4, shape[1]), 13, 6.0, 1e4)[0][None, :]          # a thread a little thicker or thinner
    slub_y = lt.noise((shape[0], 4), 14, 1e4, 6.0)[:, 0][:, None]
    cloth = cloth * (1.0 + 0.10 * np.where(over, slub_x, slub_y))
    return np.clip(0.30 + 0.40 * cloth, 0.0, 1.0)


def seams(shape):
    """Sewn seams: a dark line where two panels meet (a little puckered either side) and a row of stitches 4 px off
    it. Returns how much each pixel is seam (0..1) and the stitches (0..1)."""
    x, y = lt.coords(shape)
    h, w = shape
    line = np.zeros(shape, np.float32)
    stitch = np.zeros(shape, np.float32)
    wob = 3.0 * lt.noise(shape, 21, 80.0)
    for k, at in enumerate(SEAMS_X):
        d = x - at * w + wob
        d = (d + w / 2) % w - w / 2
        line = np.maximum(line, 1.0 - lt.smooth(0.0, 2.2, np.abs(d)))
        dash = (np.sin(y / 7.0 * 2.0 * np.pi + k) > 0.15) * (1.0 - lt.smooth(0.4, 1.4, np.abs(np.abs(d) - 4.5)))
        stitch = np.maximum(stitch, dash * (d < 0))
    for k, at in enumerate(SEAMS_Y):
        d = y - at * h + wob.T
        d = (d + h / 2) % h - h / 2
        line = np.maximum(line, 1.0 - lt.smooth(0.0, 2.2, np.abs(d)))
        dash = (np.sin(x / 7.0 * 2.0 * np.pi + k) > 0.15) * (1.0 - lt.smooth(0.4, 1.4, np.abs(np.abs(d) - 4.5)))
        stitch = np.maximum(stitch, dash * (d > 0))
    return line, stitch


def folds(shape):
    """Creases hanging down the cloth, -1..1 (ridges up): few, broad, long and soft (a dense crumple read as cracked
    stone in the heroes' bakes), their crests a little rounder than their valleys, slow drapes across them and a little
    bunching at random heights."""
    x, y = lt.coords(shape)
    sway = 28.0 * lt.noise(shape, 36, 140.0, 200.0)           # the folds wander and lean as they hang
    hang = lt.sample(lt.noise(shape, 31, 22.0, 300.0), x + sway, y)
    fade = 0.45 + 0.55 * lt.smooth(-1.2, 1.0, lt.noise(shape, 37, 110.0, 160.0))   # each fold dies out somewhere
    folds_ = (hang - 0.35 * np.abs(hang)) * fade                # rounded crests, narrower valleys
    broad = lt.noise(shape, 32, 70.0, 360.0)
    drape = lt.noise(shape, 33, 160.0, 70.0)                  # slow swells across the folds
    bunch = lt.noise(shape, 34, 30.0, 14.0) * lt.smooth(0.8, 1.8, lt.noise(shape, 35, 90.0, 60.0))
    f = 0.6 * folds_ + 0.4 * broad + 0.25 * drape + 0.12 * bunch
    f = f - f.mean()
    return np.clip(f / (2.6 * f.std()), -1.0, 1.0)


def stains(shape):
    """Grave dirt and damp, 0..1: soft blotches (about a fifth of the cloth) with a darker tide mark round each, fine
    grit in them."""
    blot = lt.noise(shape, 41, 55.0, 70.0, octaves=3)
    threshold = float(np.quantile(blot, 0.80))
    inside = lt.smooth(threshold - 0.05, threshold + 0.45, blot)
    rim = np.exp(-((blot - threshold - 0.02) / 0.05) ** 2) * 0.35
    grit = lt.specks(shape, 42, 0.08, 1.2) * lt.smooth(threshold - 0.3, threshold + 0.2, blot)
    return np.clip(0.55 * inside + rim + 0.3 * grit, 0.0, 1.0)


def make():
    shape = (SIZE, SIZE)
    r = weave(shape)
    line, stitch = seams(shape)
    r = r * (1.0 - 0.45 * line) + 0.18 * stitch * (1.0 - line)       # the seam dark, its stitches catching light
    r = r - 0.06 * line * (1.0 - line) * 4.0                          # puckered either side
    g = 0.5 + 0.5 * folds(shape)
    g = np.clip(g - 0.12 * line, 0.0, 1.0)                            # a seam sits in a slight crease
    b = stains(shape)
    data = np.stack([np.clip(r, 0.0, 1.0), g, b], axis=-1)
    lt.write_png(OUT, lt.to8(data))
    print(f'GHOSTCLOTH {OUT}: R {r.mean():.3f}  G {g.mean():.3f}  B {b.mean():.3f} (means)')
    look(data)


def look(data):
    """The channels side by side, then each of the Unpaid's tints as M_Ghost paints it (the same formulas, at 1:1 here,
    scaled to the look's width)."""
    step = 2
    d = data[::step, ::step]
    chans = [np.repeat(d[..., k:k + 1], 3, axis=-1) for k in range(3)]
    tiles = []
    for zone, hex_value in enumerate(ZONES):
        tint = lt.rgb(hex_value) ** 2.2
        on_cloth = 0.35 if zone == 0 else 1.0
        weave_ = 1.0 + 0.30 * on_cloth * (d[..., 0] - 0.5) * 2.0
        fold = 1.0 + 0.22 * (0.8 if zone == 0 else 1.0) * (d[..., 1] - 0.5) * 2.0
        stain = 1.0 - d[..., 2:3] * 0.45 * (1.0 - np.array([0.50, 0.58, 0.70]))
        c = tint[None, None, :] * (weave_ * fold)[..., None] * stain
        tiles.append(np.clip(c, 0.0, 1.0) ** (1.0 / 2.2))
    row1 = np.concatenate(chans, axis=1)
    row2 = np.concatenate(tiles[:3], axis=1)
    sheet = np.concatenate([row1, row2], axis=0)
    lt.write_png(LOOK, lt.to8(sheet))
    print(f'GHOSTCLOTH look {LOOK}')


if __name__ == '__main__':
    make()
