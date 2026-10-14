"""The cliff stains' atlas, script-made (the user's rule: no texture libraries), for a stain decal on Ransom's Rest's
sandstone faces (the "make it alive" polish, concept B: "dark desert-varnish streaks and old water stains running down
the faces, so no two slabs look alike"). A DBuffer deferred decal, as M_BulletMark (Tools/Unreal/build_world_materials.py)
and the lookout's stain: drawn before the lighting, so it is lit as the rock under it, on Medium's Nanite fallback as on
High.

    T_CliffStains_BC.png    sRGB colour; alpha is the stain's opacity (0 at every cell's edge)
    T_CliffStains_ORM.png   linear: R occlusion (unused, 1), G roughness, B metallic (0)

No normal map: varnish is a film on the rock and the mineral crust a bloom, so the face's own relief shades them (a
decal that only writes colour and roughness leaves the rock's normals alone).

2048 x 1024: CELLS cells side by side, each 256 px across and 1024 down (a 1:4 strip). A cell spans the decal's box
face: its top row is the streak's top (where the water came over the lip, or the seep it came out of), its bottom the
decal's bottom. Every cell fades to nothing at its edges, so the box never shows; the streaks themselves fade out before
the bottom, at their own lengths. Undistorted on a decal 4 times as tall as it is wide (1.5 x 6 m, 2 x 8 m); stretching it
taller is fine (streaks), wider less so.

  cell  name            what
  0     VarnishCurtain  a broad dark curtain from the lip, splitting into four fingers, two to the bottom
  1     VarnishStreak   one narrow dark streak the whole length, wandering a little, fading low down
  2     VarnishTwin     two streaks from one spout, parting, the left one longer, a rusty edge
  3     VarnishTongue   a short dark tongue under the lip (half the length) with drip fingers below it
  4     VarnishRibbons  many thin ribbons of different lengths, a rain-washed band under a long lip
  5     MineralSeep     pale calcite bloom from a seep a quarter down (a parting), spreading in fingers, a damp
                        darker line along the seep itself
  6     MineralWash     a broad, faint, bleached wash with tide lines (where water once stood and ran)
  7     Whitewash       a narrow chalk-white streak under a perch or a nest, a heavier blob at its top

Colours (sRGB): the varnish's dark manganese browns (110d0b deepest, 251c17 body, 46342a its warm edge) at 0.7-0.96
opacity in their cores, mottled a little (bold on purpose: on a sunlit, cracked face in the scene renders a fainter film
read as shading, not a stain); the pale stains' creams (dcd2bf, ece5d5) at 0.3-0.6; the whitewash f0ece2. Roughness:
varnish 0.72 (a faint sheen, as desert varnish has), the bloom and whitewash 0.95, the rock under them about 0.9.

Placing (Main's placing script): a decal on a face, its X axis into the rock (the face's inward normal), its top on a
lip line or a parting. Each decal's colour picks the cell, as the bullet marks' do: R = (cell + 0.5) / 8 (the material
reads floor(R x 8)). Every decal is one draw call (decals are not instanced): give them a fade screen size, and keep them
to the faces seen from the play space.

Run it with Blender's Python (numpy; Tools/Blender/looter_textures.py's helpers):
    Tools\\artrun.ps1 -Script Art\\Textures\\CliffStains\\make_cliff_stains.py
It writes the two maps here and Saved/ArtPreviews/CliffStains/CliffStains_sheet.png (each cell over the cliffs' own
sandstone at Ransom's Rest's tint, beside the bare rock). Reproducible: fixed seeds, numpy only. --out <folder> writes a
trial run there instead.
"""
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Tools', 'Blender'))
import looter_textures as lt  # noqa: E402 - needs the path above

CELL_W, CELL_H = 256, 1024
CELLS = 8
NAMES = ('VarnishCurtain', 'VarnishStreak', 'VarnishTwin', 'VarnishTongue', 'VarnishRibbons', 'MineralSeep',
         'MineralWash', 'Whitewash')
SEED = 7307
OUT_DIR = HERE
PREVIEW = os.path.join(REPO, 'Saved', 'ArtPreviews', 'CliffStains', 'CliffStains_sheet.png')
# The cliffs' rock under them in the preview: T_RockCliff_BC at the faces' 180 px/m, times MI_RockCliff_Ransom's Tint.
ROCK_BC = os.path.join(REPO, 'Art', 'Textures', 'RockCliff', 'T_RockCliff_BC.png')
RANSOM_TINT = (0.66, 0.58, 0.5)        # linear, Art/Levels/RansomsRest/layout.json level.materials
PREVIEW_M = (2.0, 8.0)                 # the preview's decal: 2 m across, 8 m down

DEEP, BODY, EDGE = 0x110d0b, 0x251c17, 0x46342a
RUST = 0x6e4126
PALE, CRUST, DAMP = 0xdcd2bf, 0xece5d5, 0x7d6c58
CHALK = 0xf0ece2
VARNISH_ROUGH, PALE_ROUGH = 0.72, 0.95


def rgb(value):
    return lt.rgb(value)


class Cell:
    """One strip being painted: colour and opacity laid in layers, and roughness. u runs 0..1 across, v 0..1 down."""

    def __init__(self, index):
        self.rng = np.random.default_rng(SEED + 7919 * index)
        self.seed = SEED + 101 * index
        u = (np.arange(CELL_W, dtype=np.float32) + 0.5) / CELL_W
        v = (np.arange(CELL_H, dtype=np.float32) + 0.5) / CELL_H
        self.u, self.v = np.meshgrid(u, v)
        self.color = np.zeros((CELL_H, CELL_W, 3), np.float32)
        self.alpha = np.zeros((CELL_H, CELL_W), np.float32)
        self.rough = np.full((CELL_H, CELL_W), 0.9, np.float32)
        self.count = 0

    def noise(self, sx, sy=None, octaves=3):
        """Smooth noise, mean 0, deviation about 1, its features about (sx, sy) px."""
        self.count += 1
        return lt.noise((CELL_H, CELL_W), self.seed + self.count, sx, sy, octaves=octaves)

    def unit(self, sx, sy=None, octaves=3):
        return np.clip(0.5 + 0.25 * self.noise(sx, sy, octaves), 0.0, 1.0)

    def wander(self, amount, scale=0.15):
        """A smooth sideways wander down the strip (in u), its size about amount."""
        line = self.noise(1.0, CELL_H * scale, octaves=2)[:, 0]
        return line[:, None] * amount

    def over(self, color, alpha, rough=None):
        alpha = np.clip(np.asarray(alpha, np.float32), 0.0, 1.0)
        color = np.broadcast_to(np.asarray(color, np.float32), self.color.shape)
        below = self.alpha * (1.0 - alpha)
        total = alpha + below
        safe = np.where(total > 1e-5, total, 1.0)
        self.color = (color * alpha[..., None] + self.color * below[..., None]) / safe[..., None]
        self.alpha = total
        if rough is not None:
            self.rough += (np.float32(rough) - self.rough) * alpha

    def streak(self, x0, width, length, top=0.0, taper=0.55, wander=0.03, ragged=0.25, fade=0.25, lean=0.0,
               start_width=None):
        """A streak's coverage (0..1): its middle at u = x0 (+ lean x depth + wander) from v = top down to top + length,
        width (u) across at its top narrowing by taper toward its end (start_width: across at the very top, for a
        spout that spreads), soft and ragged at the edges, fading over its last share fade."""
        depth = np.clip((self.v - top) / max(length, 1e-4), 0.0, 1.0)
        middle = x0 + lean * depth + self.wander(wander)
        w = width * (1.0 - taper * depth)
        if start_width is not None:
            w = w * (1.0 - np.exp(-depth * 18.0)) + start_width * np.exp(-depth * 18.0)
        edge = 1.0 + ragged * self.noise(6.0, 22.0)
        across = np.abs(self.u - middle) / np.maximum(w * 0.5 * edge, 1e-4)
        body = lt.smooth(1.0, 0.45, across)
        along = lt.smooth(top - 0.004, top + 0.025, self.v) * lt.smooth(1.0, 1.0 - fade, depth)
        return body * along

    def finish(self):
        """The cell's maps: nothing near its edges, the clear texels given the stain's mean colour (filtering brings no
        dark fringe in)."""
        window = (lt.smooth(0.0, 0.07, self.u) * lt.smooth(1.0, 0.93, self.u) *
                  lt.smooth(0.0, 0.012, self.v) * lt.smooth(1.0, 0.97, self.v))
        alpha = np.clip(self.alpha * window, 0.0, 1.0)
        weight = alpha.sum()
        mean = (self.color * alpha[..., None]).sum(axis=(0, 1)) / max(weight, 1e-6)
        shown = np.where(alpha[..., None] > 0.02, self.color, mean[None, None, :])
        bc = np.concatenate([np.clip(shown, 0.0, 1.0), alpha[..., None]], axis=-1)
        orm = np.stack([np.ones_like(alpha), np.clip(self.rough, 0.02, 1.0), np.zeros_like(alpha)], axis=-1)
        return bc, orm


def varnish(c, cover, strength=0.96, deep_from=0.5):
    """Lays desert varnish over cover (0..1): dark brown, darkest where it ran thickest, mottled and streaky so the
    rock shows through, a warm edge."""
    # Bold: on a sunlit, cracked face a faint film reads as shading, not as a stain (the first in-scene renders).
    mottle = 0.85 + 0.15 * lt.smooth(-1.2, 1.2, c.noise(5.0, 26.0))
    streaky = 0.9 + 0.1 * lt.smooth(-1.0, 1.0, c.noise(1.5, 60.0, octaves=2))
    deep = lt.smooth(deep_from, 1.0, cover * (0.85 + 0.3 * c.unit(8.0, 40.0)))
    color = lt.mix(rgb(EDGE), rgb(BODY), lt.smooth(0.1, 0.5, cover))
    color = lt.mix(color, rgb(DEEP), deep)
    c.over(color, np.clip(cover * 1.4, 0.0, 1.0) * strength * mottle * streaky, rough=VARNISH_ROUGH)


def pale(c, cover, strength=0.5, color=PALE, crust=0.4, rough=PALE_ROUGH):
    """Lays a pale mineral bloom over cover: cream, crusty specks brighter, soft-edged and blotchy."""
    blotch = 0.65 + 0.35 * lt.smooth(-1.0, 1.0, c.noise(10.0, 30.0))
    specks = lt.smooth(0.9, 1.8, c.noise(1.2, 2.5, octaves=1))
    shade = lt.mix(rgb(color), rgb(CRUST), crust * specks)
    c.over(shade, cover * strength * blotch, rough=rough)


# --- The cells ---

def varnish_curtain(c):
    curtain = c.streak(0.5, 0.78, 0.38, taper=0.35, wander=0.03, fade=0.5)
    fingers = np.zeros_like(curtain)
    for x0, length, width in ((0.22, 0.97, 0.12), (0.43, 0.62, 0.1), (0.6, 0.99, 0.14), (0.8, 0.5, 0.09)):
        fingers = np.maximum(fingers, c.streak(x0, width, length, top=0.05, taper=0.5, wander=0.035, fade=0.35))
    varnish(c, np.maximum(curtain, fingers * 0.95))


def varnish_streak(c):
    varnish(c, c.streak(0.5, 0.34, 0.96, taper=0.6, wander=0.06, fade=0.45, start_width=0.5), strength=0.8)


def varnish_twin(c):
    left = c.streak(0.46, 0.26, 0.95, taper=0.55, wander=0.04, fade=0.4, lean=-0.18, start_width=0.42)
    right = c.streak(0.54, 0.22, 0.62, top=0.02, taper=0.6, wander=0.04, fade=0.45, lean=0.2)
    cover = np.maximum(left, right)
    # A rusty halo round them (iron in the bed above leaching out with the water).
    halo = np.clip(c.streak(0.5, 0.62, 0.7, taper=0.5, wander=0.02, fade=0.6) - cover, 0.0, 1.0)
    c.over(rgb(RUST), 0.32 * halo * c.unit(8.0, 30.0), rough=0.85)
    varnish(c, cover)


def varnish_tongue(c):
    tongue = c.streak(0.5, 0.7, 0.42, taper=0.55, wander=0.02, fade=0.35, start_width=0.85)
    drips = np.zeros_like(tongue)
    for k in range(6):
        x0 = 0.25 + 0.5 * k / 5 + c.rng.uniform(-0.04, 0.04)
        drips = np.maximum(drips, c.streak(x0, c.rng.uniform(0.04, 0.07), c.rng.uniform(0.45, 0.7), top=0.15,
                                           taper=0.7, wander=0.015, fade=0.5) * c.rng.uniform(0.6, 0.9))
    varnish(c, np.maximum(tongue, drips), strength=0.82, deep_from=0.45)


def varnish_ribbons(c):
    cover = np.zeros((CELL_H, CELL_W), np.float32)
    count = 11
    for k in range(count):
        x0 = 0.12 + 0.76 * (k + c.rng.uniform(-0.3, 0.3)) / (count - 1)
        length = c.rng.uniform(0.3, 0.98)
        cover = np.maximum(cover, c.streak(x0, c.rng.uniform(0.03, 0.065), length, top=c.rng.uniform(0.0, 0.04),
                                           taper=0.6, wander=0.012, fade=0.5) * c.rng.uniform(0.55, 1.0))
    # A thin band of film along the lip they ran from.
    band = lt.smooth(0.11, 0.0, c.v) * lt.smooth(0.05, 0.15, c.u) * lt.smooth(0.95, 0.85, c.u)
    varnish(c, np.maximum(cover, band * 0.6), strength=0.7)


def mineral_seep(c):
    seep_v = 0.24
    line = np.exp(-((c.v - seep_v - 0.012 * c.noise(24.0, 400.0, octaves=1)) / 0.007) ** 2)
    line *= lt.smooth(0.08, 0.25, c.u) * lt.smooth(0.92, 0.75, c.u)
    bloom = np.zeros_like(line)
    for x0, length, width in ((0.3, 0.42, 0.2), (0.52, 0.66, 0.26), (0.72, 0.36, 0.18)):
        bloom = np.maximum(bloom, c.streak(x0, width, length, top=seep_v, taper=0.6, wander=0.03, fade=0.6))
    # Above the seep a little bloom too, where it wicked up.
    wick = lt.smooth(seep_v - 0.06, seep_v, c.v) * lt.smooth(seep_v + 0.01, seep_v, c.v) * line.max(axis=0)[None, :]
    pale(c, np.maximum(bloom, wick * 0.6), strength=0.55, crust=0.5)
    c.over(rgb(DAMP), 0.45 * line, rough=0.8)


def mineral_wash(c):
    wash = c.streak(0.5, 0.86, 0.85, top=0.08, taper=0.3, wander=0.02, ragged=0.35, fade=0.6)
    tides = np.zeros_like(wash)
    for v0 in (0.3, 0.46, 0.58):
        tide = np.exp(-((c.v - v0 - 0.004 * c.noise(70.0, 400.0, octaves=1)) / 0.01) ** 2)
        tides = np.maximum(tides, tide)
    pale(c, wash, strength=0.36, color=0xd6ccb8, crust=0.25)
    pale(c, tides * wash, strength=0.3, color=CRUST, crust=0.0)


def whitewash(c):
    # Soft and broken: a stark white streak read as paint in the scene renders.
    blob = np.exp(-(((c.u - 0.5) / 0.2) ** 2 + ((c.v - 0.03) / 0.018) ** 2)) * (0.6 + 0.4 * c.unit(4.0))
    streak = c.streak(0.5, 0.2, 0.5, top=0.03, taper=0.75, wander=0.03, ragged=0.45, fade=0.7)
    drops = np.zeros_like(streak)
    for k in range(3):
        drops = np.maximum(drops, c.streak(0.5 + c.rng.uniform(-0.2, 0.2), 0.035, c.rng.uniform(0.2, 0.4),
                                           top=0.04, taper=0.6, wander=0.02, fade=0.6))
    broken = 0.55 + 0.45 * lt.smooth(-1.0, 1.0, c.noise(4.0, 14.0))
    pale(c, np.maximum(np.maximum(blob, streak), drops * 0.6) * broken, strength=0.42, color=CHALK, crust=0.3)


PAINTERS = (varnish_curtain, varnish_streak, varnish_twin, varnish_tongue, varnish_ribbons, mineral_seep, mineral_wash,
            whitewash)


# --- The preview ---

def rock_backdrop(shape):
    """The cliffs' sandstone under a 2 x 8 m decal: T_RockCliff_BC at the faces' 180 px/m (the bands across), at
    Ransom's Rest's tint, in sRGB. A flat sandstone colour if the texture can't be read."""
    h, w = shape
    try:
        import bpy
        image = bpy.data.images.load(ROCK_BC, check_existing=True)
        size = image.size[0]
        pixels = np.array(image.pixels[:], np.float32).reshape(image.size[1], image.size[0], 4)[::-1, :, :3]
        # The decal's metres to the texture's pixels: 180 px a metre.
        xs = (np.arange(w, dtype=np.float32) + 0.5) / w * PREVIEW_M[0] * 180.0
        ys = (np.arange(h, dtype=np.float32) + 0.5) / h * PREVIEW_M[1] * 180.0
        gx, gy = np.meshgrid(xs, ys)
        rock = lt.sample(pixels, gx % size, gy % size)
    except Exception as error:  # noqa: BLE001 - outside Blender, or no texture: a flat stand-in
        print(f'CLIFFSTAINS preview: rock texture not read ({error}); a flat stand-in', flush=True)
        rock = np.broadcast_to(rgb(0x9a8a76)[None, None, :], (h, w, 3)).astype(np.float32)
    linear = np.where(rock <= 0.04045, rock / 12.92, ((rock + 0.055) / 1.055) ** 2.4) * np.array(RANSOM_TINT, np.float32)
    return np.where(linear <= 0.0031308, linear * 12.92, 1.055 * np.power(np.maximum(linear, 0.0), 1.0 / 2.4) - 0.055)


def preview(bc):
    """Each cell over the rock (its colour laid at its opacity), the bare rock at the right."""
    h, w = CELL_H, CELL_W
    rock = rock_backdrop((h, w))
    gap = np.full((h, 10, 3), 0.08, np.float32)
    columns = []
    for index in range(CELLS):
        cell = bc[:, index * w:(index + 1) * w]
        alpha = cell[..., 3:4]
        columns += [np.clip(rock * (1.0 - alpha) + cell[..., :3] * alpha, 0.0, 1.0), gap]
    columns.append(rock)
    os.makedirs(os.path.dirname(PREVIEW), exist_ok=True)
    lt.write_png(PREVIEW, lt.to8(np.concatenate(columns, axis=1)))


def main():
    global OUT_DIR, PREVIEW
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    if '--out' in argv and argv.index('--out') + 1 < len(argv):
        OUT_DIR = argv[argv.index('--out') + 1]
        PREVIEW = os.path.join(OUT_DIR, 'CliffStains_sheet.png')
        os.makedirs(OUT_DIR, exist_ok=True)
    bc = np.zeros((CELL_H, CELL_W * CELLS, 4), np.float32)
    orm = np.zeros((CELL_H, CELL_W * CELLS, 3), np.float32)
    for index, painter in enumerate(PAINTERS):
        cell = Cell(index)
        painter(cell)
        cell_bc, cell_orm = cell.finish()
        bc[:, index * CELL_W:(index + 1) * CELL_W] = cell_bc
        orm[:, index * CELL_W:(index + 1) * CELL_W] = cell_orm
        print(f'CLIFFSTAINS {index} {NAMES[index]}: covers {float((cell_bc[..., 3] > 0.05).mean()) * 100.0:.0f}%, '
              f'mean opacity {float(cell_bc[..., 3].mean()):.2f}', flush=True)
    lt.write_png(os.path.join(OUT_DIR, 'T_CliffStains_BC.png'), lt.to8(bc))
    lt.write_png(os.path.join(OUT_DIR, 'T_CliffStains_ORM.png'), lt.to8(orm))
    preview(bc)
    print(f'CLIFFSTAINS done: {OUT_DIR} and {PREVIEW}', flush=True)


main()
