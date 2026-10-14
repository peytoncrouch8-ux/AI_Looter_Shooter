"""The old stain on the lookout's deck boards, script-made (the user's rule: no texture libraries), for the decal
MI_LookoutStain (Tools/Unreal/build_stain_decal.py builds M_StainDecal and the instance) on the boards where Ellis died
(Main 2's LookoutStains readable; the stain the lookout's model already bakes into its darker boards, Art/Models/Buildings/
Lookout.py's STAIN, is under it):

    T_LookoutStain_BC.png   sRGB colour; alpha is the stain's opacity (0 well inside the edge of the square)
    T_LookoutStain_N.png    DirectX normal map (green down, as Unreal expects): next to flat, the grain a hair raised
    T_LookoutStain_ORM.png  linear: R occlusion (unused, 1), G roughness, B metallic (0)

SIZE px square over the decal's SPAN_M (m): across the image (+x, the decal's U) runs along the boards, so the soak
follows the wood's grain when the decal's U lies along them (its local Y: the decal's yaw is the boards' heading less
90 degrees, pitched straight down).

The look (the story's tone: a death long ago, not gore): a dark brown blot soaked deep into weathered wood, never red,
faded by years of sun and rain. A pool longer along the grain with a lobe where it ran toward the deck's edge, the
darkest where it lay deepest, a slightly darker tide line where its edge dried; fingers of it soaked out along the grain
past the edge; a scatter of old drops on one side; weathering mottles it lighter in patches, and the grain shows through
everywhere (it's in the wood, not on it).

Run it with Blender's Python (numpy; Tools/Blender/looter_textures.py's helpers):
    Tools\\artrun.ps1 -Script Art\\Textures\\LookoutStain\\make_lookout_stain.py
It writes the three maps here and Saved/ArtPreviews/LookoutStain/LookoutStain_sheet.png (the stain over weathered boards
running across the image, lit from the top left; beside it the boards alone). Reproducible: fixed seeds, numpy only.
Then, in the editor:
    Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_stain_decal.py"
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
SPAN_M = 1.5                     # the decal's square on the boards (m): its placement's size (half extents 75 cm)
PX_M = SPAN_M / SIZE
SEED = 5113
# The pool (m, from the middle; x along the grain): its half lengths along and across, and the lobe it ran into.
POOL = (0.48, 0.3)
LOBE = ((0.24, 0.16), 0.2)       # its middle and radius
# The colours (sRGB): soaked deepest, the body of it, its thin soaked edge, and the tide line where its edge dried. Dark
# browns with only a breath of red in the deepest: old blood in grey wood reads brown-black, not red.
DEEP, BODY, EDGE, TIDE = 0x1f140e, 0x2e1e15, 0x4a3526, 0x241711
# How opaque each part is (the grain shows through everywhere).
DEEP_ALPHA, BODY_ALPHA, EDGE_ALPHA = 0.88, 0.72, 0.38
OUT_DIR = HERE
PREVIEW = os.path.join(REPO, 'Saved', 'ArtPreviews', 'LookoutStain', 'LookoutStain_sheet.png')


def rgb(value):
    return lt.rgb(value)


def field(seed, sx, sy=None, octaves=3):
    """Smooth noise, mean 0, deviation about 1, its features about (sx, sy) px."""
    return lt.noise((SIZE, SIZE), SEED + seed, sx, sy, octaves=octaves)


def make():
    """The stain's colour (sRGB), opacity, height (m) and roughness."""
    c = (np.arange(SIZE, dtype=np.float32) + 0.5) * PX_M - SPAN_M * 0.5
    x, y = np.meshgrid(c, c)
    rng = np.random.default_rng(SEED)

    # The pool's outline, lumpy: an ellipse longer along the grain and the lobe it ran into, warped by noise.
    warp_x = field(1, 60.0, octaves=2) * 0.045
    warp_y = field(2, 60.0, octaves=2) * 0.035
    wx, wy = x + warp_x, y + warp_y
    pool = 1.0 - np.hypot(wx / POOL[0], wy / POOL[1])
    (lx, ly), lr = LOBE
    lobe = 1.0 - np.hypot((wx - lx) / lr, (wy - ly) / (lr * 0.75))
    shape = np.maximum(pool, lobe * 0.8) + field(3, 25.0, octaves=3) * 0.06

    # Soaked out along the grain: streaks long along x and fine across, reaching past the edge as fingers.
    grain = field(4, 70.0, 2.2, octaves=2)
    fine = field(5, 18.0, 0.9, octaves=2)
    soaked = shape + 0.11 * np.clip(grain, -1.0, 2.0) + 0.03 * fine
    inside = lt.smooth(-0.02, 0.06, soaked)                  # the stain at all, with its fingers
    body = lt.smooth(0.05, 0.25, shape)                      # where it pooled
    deep = lt.smooth(0.3, 0.75, shape + 0.08 * field(6, 40.0))
    tide = np.exp(-((shape - 0.06) / 0.03) ** 2)

    # A few old drops, most on the side it fell toward (+y, the lobe's), soaked out ragged and dragged along the grain,
    # faint: weathered as long as the pool.
    ragged_x = field(8, 3.0, octaves=2) * 0.003
    ragged_y = field(9, 3.0, octaves=2) * 0.002
    drops = np.zeros((SIZE, SIZE), np.float32)
    for _ in range(18):
        angle = rng.normal(math.atan2(LOBE[0][1], LOBE[0][0]), 0.9)
        reach = rng.uniform(0.42, 0.66)
        cx, cy = reach * math.cos(angle) * 1.1, reach * math.sin(angle) * 0.85
        size = rng.uniform(0.004, 0.012) * (1.2 - 0.6 * (reach - 0.42) / 0.24)
        drag = rng.uniform(1.3, 3.0)
        d = np.hypot((x + ragged_x - cx) / (size * drag), (y + ragged_y - cy) / size)
        drops = np.maximum(drops, rng.uniform(0.5, 1.0) * lt.smooth(1.3, 0.6, d))

    # Weathering: years of sun and rain have faded it in patches, more at the edge than in the deep.
    faded = 0.78 + 0.22 * lt.smooth(-1.2, 1.2, field(7, 90.0, octaves=3))
    faded = faded + (1.0 - faded) * deep * 0.6
    # The grain shows through: the wood took more of it along its darker lines.
    through = 0.86 + 0.14 * lt.smooth(-1.5, 1.5, fine) * (0.6 + 0.4 * lt.smooth(-1.0, 1.0, grain))

    alpha = inside * (EDGE_ALPHA + (BODY_ALPHA - EDGE_ALPHA) * body + (DEEP_ALPHA - BODY_ALPHA) * deep)
    alpha = np.maximum(alpha, 0.42 * drops)
    alpha = alpha * faded * through
    # Nothing near the square's edge: the decal's box never shows.
    edge = np.maximum(np.abs(x), np.abs(y)) / (SPAN_M * 0.5)
    alpha = np.clip(alpha * lt.smooth(0.97, 0.88, edge), 0.0, 1.0)

    color = lt.mix(rgb(EDGE), rgb(BODY), body)
    color = lt.mix(color, rgb(DEEP), deep)
    color = lt.mix(color, rgb(TIDE), 0.45 * tide * (1.0 - deep))
    color = lt.mix(color, rgb(BODY), drops * (1.0 - body))
    color = color * (0.94 + 0.06 * lt.smooth(-1.0, 1.0, fine))[..., None]

    # Next to flat: soaked wood's grain a hair raised where it's darkest (it swelled and dried), no crust.
    height = 0.00012 * (alpha / DEEP_ALPHA) * lt.smooth(-1.0, 1.5, fine)
    # A little smoother where it soaked deep (the wood sealed by it), as weathered as the boards at its edge.
    rough = 0.86 - 0.12 * deep * inside
    return color, alpha, height, rough


def finish(color, alpha, height, rough):
    """The maps: colour (sRGB) with the opacity in alpha, the clear texels given the stain's mean colour (filtering brings
    no dark fringe in); the normal map; roughness."""
    weight = alpha.sum()
    mean = (color * alpha[..., None]).sum(axis=(0, 1)) / max(weight, 1e-6)
    shown = np.where(alpha[..., None] > 0.02, color, mean[None, None, :])
    bc = np.concatenate([np.clip(shown, 0.0, 1.0), alpha[..., None]], axis=-1)
    normal = lt.normal_map(height, PX_M)
    orm = np.stack([np.ones_like(alpha), np.clip(rough, 0.02, 1.0), np.zeros_like(alpha)], axis=-1)
    return bc, normal, orm


def boards():
    """Weathered grey-brown boards for the preview: 19 cm planks on a 20 cm pitch across the image, their grain along x,
    dark gaps between."""
    c = (np.arange(SIZE, dtype=np.float32) + 0.5) * PX_M - SPAN_M * 0.5
    x, y = np.meshgrid(c, c)
    row = np.floor((y + 0.07) / 0.2)
    tone = lt.per_cell(row.astype(np.int64) - int(row.min()), int(row.max() - row.min()) + 1, SEED + 20, 0.85, 1.1)
    grain = lt.noise((SIZE, SIZE), SEED + 21, 80.0, 1.5, octaves=2)
    wood = rgb(0x7d6e5e)[None, None, :] * (tone * (0.9 + 0.08 * np.clip(grain, -2.0, 2.0)))[..., None]
    gap = (y + 0.07) % 0.2 < 0.01
    return np.where(gap[..., None], rgb(0x2a231c)[None, None, :], wood)


def preview(bc, normal):
    """The stain over the boards, lit from the top left by its relief, and the boards alone beside it."""
    wood = boards()
    alpha = bc[..., 3:4]
    n = normal.astype(np.float32) / 127.5 - 1.0
    light = np.array([-0.45, -0.5, 0.74], np.float32)
    light /= np.linalg.norm(light)
    lit = np.clip((n * light[None, None, :]).sum(axis=-1, keepdims=True), 0.0, 1.0)
    flat = float(light[2])
    shade = 0.45 + 0.75 * (flat + (lit - flat) * alpha)
    stained = np.clip((wood * (1.0 - alpha) + bc[..., :3] * alpha) * shade, 0.0, 1.0)
    plain = np.clip(wood * (0.45 + 0.75 * flat), 0.0, 1.0)
    gap = np.full((SIZE, 16, 3), 0.08, np.float32)
    os.makedirs(os.path.dirname(PREVIEW), exist_ok=True)
    lt.write_png(PREVIEW, lt.to8(np.concatenate([stained, gap, plain], axis=1)))


def main():
    # Optional, after Blender's "--": --out <folder> writes the maps and the sheet there instead (a trial run).
    global OUT_DIR, PREVIEW
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    if '--out' in argv and argv.index('--out') + 1 < len(argv):
        OUT_DIR = argv[argv.index('--out') + 1]
        PREVIEW = os.path.join(OUT_DIR, 'LookoutStain_sheet.png')
        os.makedirs(OUT_DIR, exist_ok=True)
    bc, normal, orm = finish(*make())
    lt.write_png(os.path.join(OUT_DIR, 'T_LookoutStain_BC.png'), lt.to8(bc))
    lt.write_png(os.path.join(OUT_DIR, 'T_LookoutStain_N.png'), normal)
    lt.write_png(os.path.join(OUT_DIR, 'T_LookoutStain_ORM.png'), lt.to8(orm))
    preview(bc, normal)
    print(f'LOOKOUTSTAIN covers {float((bc[..., 3] > 0.05).mean()) * 100.0:.0f}% of its square; '
          f'{OUT_DIR} and {PREVIEW}', flush=True)


main()
