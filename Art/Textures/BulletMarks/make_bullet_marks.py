"""The bullet marks' atlas, script-made (the user's rule: no texture libraries), for M_BulletMark
(Tools/Unreal/build_world_materials.py) and the marks the game leaves (Source/AI_Looter_Shooter/Weapons/BulletMarks):

    T_BulletMarks_BC.png   sRGB colour; alpha is the mark's opacity (0 at every cell's edge)
    T_BulletMarks_N.png    DirectX normal map (green down, as Unreal expects) of the mark's relief
    T_BulletMarks_ORM.png  linear: R occlusion (unused, 1), G roughness, B metallic

2048 x 2048 (2048 x 1024 before the element marks, 2026-10-29): COLUMNS x ROWS cells of CELL px, a style's VARIANTS side by side (cell = style * 2 + variant, counted left to
right, top to bottom). A cell spans the decal's square: its middle is where the bullet struck, its edge the decal's
radius (FBulletMarks' MarkLook gives each style's radius in cm, RADIUS_CM here: the normal map's scale). Across the cell
(+x) is the decal's U, along the shot (a grazing round's gouge is stretched that way by the decal's box).

The styles, in FBulletMarks' order (never reorder: the game sends the index):
    0 Hole        nothing known: a plain dark hole and a ring of grey dust
    1 Dirt        a puff mark: a dark moist crater, clods and thrown soil darkening the ground round it
    2 Grass       a torn divot: dark soil, shreds of green and straw, a little soil
    3 Gravel      a crunched crater: pale pulverised stone dust, chips light and dark
    4 Sand        a soft shallow crater with pale rays of spray
    5 Stone       a chip: a jagged crater of fresher, paler stone in facets, a dark core, hairline cracks, powder
    6 Wood        a scorched hole with pale splinters torn out along the grain
    7 Metal       a dent: a dark core, a grey lead smear, a bright scraped ring of bare steel, paint flaked round it
    8 SheetMetal  a punched hole: black, with bright torn petals bent out and scraped paint
    9 Glass       a star of cracks: a crushed white centre, radial cracks and broken rings
    10 Cloth      a small frayed hole
    11 Hay        a dark puncture and broken straws strewn round it
    12 Bone       a pale ivory chip with fine cracks
    13 StoneHeavy a heavy round's chip (revolver, sniper): the stone chip ringed by cracks running out to its edge
    14 WoodHeavy  a heavy round's hole in timber: the scorched hole split along the grain, splinters lifted
  The element rounds' marks ("Elements matter", 2026-10-29; FBulletMarks::ElementLookOf), on any surface:
    15 Scorch     Cinder: a charred disc, a star of soot streaks thrown out, ember-pocked char, heat-browned round it
    16 Burn       Thunder: a fused dark burn ring with a pale blue-white glassy edge, branching burn lines running out
    17 Acid       Rot: a wet sick-green splash with gobs thrown round it, its middle eaten into pits, a stain soaking out
    18 Frost      Wardbreak: salt rime packed round the hole in facets, feathery fronds of frost, teal-lit glints
    19 Gilt       Hallow: a gold splash (metallic), drops and short rays flung out, a burnished ring, a dark punched core
  (The decal can't glow: the brief flare on a fresh element mark is the game's particles, FWeaponFX::SpawnElementMark.)

Run it with Blender's Python (numpy; Tools/Blender/looter_textures.py's helpers):
    Tools\\artrun.ps1 -Script Art\\Textures\\BulletMarks\\make_bullet_marks.py
It writes the three maps here and Saved/ArtPreviews/BulletMarks/BulletMarks_sheet.png (each cell over a surface of its
kind, lit from the top left). Reproducible: fixed seeds, numpy only. Then, in the editor:
    Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_world_materials.py M_BulletMark"
"""
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Tools', 'Blender'))
import looter_textures as lt  # noqa: E402 - needs the path above

CELL = 256
# 8 rows since the element marks (32 style slots; M_BulletMark and FBulletMarks::StyleSlots read the same).
COLUMNS, ROWS, VARIANTS = 8, 8, 2
# A pixel in the cell's own units (it spans -1..1).
AA = 2.0 / CELL
STYLES = ('Hole', 'Dirt', 'Grass', 'Gravel', 'Sand', 'Stone', 'Wood', 'Metal', 'SheetMetal', 'Glass', 'Cloth', 'Hay', 'Bone',
          'StoneHeavy', 'WoodHeavy', 'Scorch', 'Burn', 'Acid', 'Frost', 'Gilt')
# Each style's radius (cm) for a rifle round, as FBulletMarks' MarkLook (and ElementLookOf) has it: how big a cell is in the world.
RADIUS_CM = dict(Hole=5.0, Dirt=9.0, Grass=8.0, Gravel=8.0, Sand=9.5, Stone=7.0, Wood=6.0, Metal=5.0, SheetMetal=5.5,
                 Glass=11.0, Cloth=4.5, Hay=6.0, Bone=5.0, StoneHeavy=8.5, WoodHeavy=7.5,
                 Scorch=9.0, Burn=8.5, Acid=9.0, Frost=9.5, Gilt=8.0)
# The preview's surface under each style (sRGB); the element marks go on any surface, shown on a few.
BACKDROPS = dict(Hole=0x808080, Dirt=0x7a6248, Grass=0x5e6d38, Gravel=0x8c8578, Sand=0xc6ad84, Stone=0x837c72, Wood=0x8a6a48,
                 Metal=0x46524a, SheetMetal=0x7a6c5e, Glass=0x1c2228, Cloth=0xb3aa96, Hay=0xc2a05a, Bone=0xd8cfbe,
                 StoneHeavy=0x837c72, WoodHeavy=0x8a6a48,
                 Scorch=0x8a6a48, Burn=0x837c72, Acid=0x46524a, Frost=0x7a6248, Gilt=0x837c72)
OUT_DIR = HERE
PREVIEW = os.path.join(REPO, 'Saved', 'ArtPreviews', 'BulletMarks', 'BulletMarks_sheet.png')
# The styles the closer looks (BulletMarks_zoom.png, BulletMarks_zoom_elements.png) show.
ZOOM = ('Dirt', 'Stone', 'Wood', 'Metal', 'StoneHeavy', 'WoodHeavy')
ZOOM_ELEMENTS = ('Scorch', 'Burn', 'Acid', 'Frost', 'Gilt')


def rgb(value):
    return lt.rgb(value)


class Mark:
    """One cell being painted: layers laid over each other (colour and opacity), relief in meters, roughness, metallic."""

    def __init__(self, seed, radius_cm, rough=0.85):
        self.rng = np.random.default_rng(seed)
        c = (np.arange(CELL, dtype=np.float32) + 0.5) / CELL * 2.0 - 1.0
        self.x, self.y = np.meshgrid(c, c)
        self.r = np.hypot(self.x, self.y)
        self.a = np.arctan2(self.y, self.x)
        self.color = np.zeros((CELL, CELL, 3), np.float32)
        self.alpha = np.zeros((CELL, CELL), np.float32)
        self.height = np.zeros((CELL, CELL), np.float32)
        self.rough = np.full((CELL, CELL), rough, np.float32)
        self.metal = np.zeros((CELL, CELL), np.float32)
        self.px_m = radius_cm * 2.0 / 100.0 / CELL

    # --- fields ---

    def noise(self, sigma, octaves=3):
        """Smooth noise, mean 0 and deviation about 1, its features about sigma px."""
        return lt.noise((CELL, CELL), int(self.rng.integers(1 << 30)), sigma, octaves=octaves)

    def unit_noise(self, sigma, octaves=3):
        return np.clip(0.5 + 0.25 * self.noise(sigma, octaves), 0.0, 1.0)

    def ragged(self, amount, harmonics=12, falloff=1.2):
        """A wobble round the circle (by angle), its deviation amount: a ragged edge's radius times (1 + this)."""
        out = np.zeros_like(self.a)
        for k in range(1, harmonics + 1):
            out += self.rng.normal() / k ** falloff * np.cos(k * self.a + self.rng.uniform(0.0, 2.0 * math.pi))
        return out / (out.std() + 1e-6) * amount

    def disc(self, radius, soft, ragged=0.0):
        """1 inside a (ragged) circle, 0 outside, over soft."""
        edge = radius * (1.0 + ragged)
        return lt.smooth(edge + soft, edge - soft, self.r)

    def ring(self, inner, outer, soft):
        return lt.smooth(inner - soft, inner + soft, self.r) * lt.smooth(outer + soft, outer - soft, self.r)

    # --- painting ---

    def over(self, color, alpha, rough=None, metal=None):
        """Lays a layer over what's there: colour (a colour or an image) at alpha; roughness and metallic follow it."""
        alpha = np.clip(np.asarray(alpha, np.float32), 0.0, 1.0)
        color = np.broadcast_to(np.asarray(color, np.float32), self.color.shape)
        below = self.alpha * (1.0 - alpha)
        total = alpha + below
        safe = np.where(total > 1e-5, total, 1.0)
        self.color = (color * alpha[..., None] + self.color * below[..., None]) / safe[..., None]
        self.alpha = total
        if rough is not None:
            self.rough += (np.float32(rough) - self.rough) * alpha
        if metal is not None:
            self.metal += (np.float32(metal) - self.metal) * alpha

    def bowl(self, radius, depth, ragged=0.0):
        """A crater sunk depth meters in the middle, easing out to nothing at its (ragged) edge: no hard ring of light."""
        edge = radius * (1.0 + ragged)
        self.height -= depth * np.clip(1.0 - (self.r / edge) ** 2, 0.0, 1.0) ** 2

    def rim(self, radius, width, height):
        self.height += height * np.exp(-((self.r - radius) / width) ** 2)

    # --- strokes and specks (each drawn only over its own box) ---

    def _box(self, x0, y0, x1, y1):
        c0 = max(int(math.floor((x0 + 1.0) * 0.5 * CELL)) - 1, 0)
        c1 = min(int(math.ceil((x1 + 1.0) * 0.5 * CELL)) + 1, CELL)
        r0 = max(int(math.floor((y0 + 1.0) * 0.5 * CELL)) - 1, 0)
        r1 = min(int(math.ceil((y1 + 1.0) * 0.5 * CELL)) + 1, CELL)
        return slice(r0, max(r0, r1)), slice(c0, max(c0, c1))

    def stroke(self, points, w0, w1, mask=None):
        """A tapering line through points, width w0 to w1 (cell units, half widths), into mask (a new one if none)."""
        mask = np.zeros((CELL, CELL), np.float32) if mask is None else mask
        count = len(points) - 1
        for index in range(count):
            (x0, y0), (x1, y1) = points[index], points[index + 1]
            wa = w0 + (w1 - w0) * index / count
            wb = w0 + (w1 - w0) * (index + 1) / count
            pad = max(wa, wb) + 2.0 * AA
            box = self._box(min(x0, x1) - pad, min(y0, y1) - pad, max(x0, x1) + pad, max(y0, y1) + pad)
            px, py = self.x[box], self.y[box]
            dx, dy = x1 - x0, y1 - y0
            t = np.clip(((px - x0) * dx + (py - y0) * dy) / (dx * dx + dy * dy + 1e-12), 0.0, 1.0)
            d = np.hypot(px - (x0 + t * dx), py - (y0 + t * dy))
            w = wa + (wb - wa) * t
            mask[box] = np.maximum(mask[box], lt.smooth(w + AA, w - AA, d))
        return mask

    def path(self, angle, r0, r1, steps, wander):
        """A crack's or a splinter's line: from r0 out to about r1 along angle, wandering, held roughly outward."""
        px, py = r0 * math.cos(angle), r0 * math.sin(angle)
        points = [(px, py)]
        heading = angle
        step = (r1 - r0) / steps
        for _ in range(steps):
            radial = math.atan2(py, px)
            heading += self.rng.normal(0.0, wander)
            off = (heading - radial + math.pi) % (2.0 * math.pi) - math.pi
            heading = radial + off * 0.7
            px += step * math.cos(heading)
            py += step * math.sin(heading)
            points.append((px, py))
        return points

    def specks(self, count, r_lo, r_hi, size_lo, size_hi, falloff=1.0):
        """Small irregular blobs between r_lo and r_hi (more of them inward as falloff grows; the far ones smaller): their
        mask, and a random shade (0..1) per blob."""
        mask = np.zeros((CELL, CELL), np.float32)
        shade = np.zeros((CELL, CELL), np.float32)
        for _ in range(count):
            reach = self.rng.random() ** falloff
            radius = r_lo + (r_hi - r_lo) * reach
            angle = self.rng.uniform(0.0, 2.0 * math.pi)
            cx, cy = radius * math.cos(angle), radius * math.sin(angle)
            size = self.rng.uniform(size_lo, size_hi) * (1.0 - 0.45 * reach)
            box = self._box(cx - size * 1.6, cy - size * 1.6, cx + size * 1.6, cy + size * 1.6)
            px, py = self.x[box] - cx, self.y[box] - cy
            # A lumpy outline: two or three lobes of their own sizes, never a tidy triangle.
            turn = np.arctan2(py, px)
            lobes = (1.0 + self.rng.uniform(0.08, 0.22) * np.cos(2.0 * turn + self.rng.uniform(0.0, 2.0 * math.pi))
                     + self.rng.uniform(0.05, 0.18) * np.cos(3.0 * turn + self.rng.uniform(0.0, 2.0 * math.pi)))
            blob = lt.smooth(size + AA, size - AA, np.hypot(px, py) * lobes)
            shade[box] = np.where(blob > mask[box], self.rng.random(), shade[box])
            mask[box] = np.maximum(mask[box], blob)
        return mask, shade

    def facets(self, cells, seed):
        """Voronoi facets: (each pixel's facet's random value -1..1, the facet borders' closeness 0..1, two tilts)."""
        cell = lt.worley((CELL, CELL), (cells, cells), seed)
        value = lt.per_cell(cell['id'], cell['count'], seed + 1, -1.0, 1.0)
        tilt_x = lt.per_cell(cell['id'], cell['count'], seed + 2, -1.0, 1.0)
        tilt_y = lt.per_cell(cell['id'], cell['count'], seed + 3, -1.0, 1.0)
        border = lt.smooth(2.0, 0.4, cell['edge'])
        return value, border, tilt_x, tilt_y


# --- The styles: each paints one Mark (variant 0 or 1, with its own seed) ---

def style_hole(m, variant):
    rag = m.ragged(0.15)
    m.over(rgb(0x5a5650), 0.35 * lt.smooth(0.65, 0.2, m.r) * m.unit_noise(8), rough=0.9)
    specks, _ = m.specks(18 + 6 * variant, 0.2, 0.7, 0.01, 0.025)
    m.over(rgb(0x2e2b27), 0.7 * specks, rough=0.9)
    m.over(rgb(0x0c0b0a), m.disc(0.16, AA * 1.5, rag), rough=0.95)
    m.bowl(0.18, 0.004, rag)
    m.rim(0.18, 0.04, 0.0006)


def style_dirt(m, variant):
    rag = m.ragged(0.22 + 0.06 * variant, harmonics=14)
    # Thrown soil darkening the ground round it, broken up.
    m.over(rgb(0x3a2a1c), 0.3 * lt.smooth(0.85, 0.3, m.r) * m.unit_noise(10), rough=0.9)
    clods, shade = m.specks(70 + 20 * variant, 0.22, 0.92, 0.012, 0.04, falloff=1.6)
    m.over(lt.mix(rgb(0x2a1d13), rgb(0x4e3a27), shade), 0.9 * clods, rough=0.85)
    # The crater: dark, moist soil turned up, darker still where the round went in.
    crater = m.disc(0.32, 0.06, rag)
    m.over(lt.mix(rgb(0x2c1f15), rgb(0x45321f), m.unit_noise(4)), 0.92 * crater, rough=0.78)
    m.over(rgb(0x140d08), 0.95 * m.disc(0.12, 0.03, m.ragged(0.25)), rough=0.7)
    m.bowl(0.34, 0.012, rag)
    m.rim(0.34, 0.06, 0.002)
    m.height += clods * 0.0015


def style_grass(m, variant):
    rag = m.ragged(0.25)
    m.over(rgb(0x3a2e1d), 0.2 * lt.smooth(0.75, 0.25, m.r) * m.unit_noise(10), rough=0.9)
    soil, soil_shade = m.specks(35 + 10 * variant, 0.2, 0.85, 0.01, 0.03, falloff=1.5)
    m.over(lt.mix(rgb(0x2a1d13), rgb(0x46331f), soil_shade), 0.85 * soil, rough=0.85)
    # Shreds of grass torn up: dark green and pale straw, flung out.
    for _ in range(26 + 8 * variant):
        angle = m.rng.uniform(0.0, 2.0 * math.pi)
        start = m.rng.uniform(0.18, 0.55)
        heading = angle + m.rng.normal(0.0, 0.8)
        length = m.rng.uniform(0.06, 0.16)
        x0, y0 = start * math.cos(angle), start * math.sin(angle)
        shred = m.stroke([(x0, y0), (x0 + length * math.cos(heading), y0 + length * math.sin(heading))], 0.012, 0.005)
        m.over(lt.mix(rgb(0x2e3a14), rgb(0x7a7444), m.rng.random() ** 2), 0.85 * shred, rough=0.8)
        m.height += shred * 0.001
    m.over(rgb(0x2a1e14), 0.92 * m.disc(0.24, 0.05, rag), rough=0.8)
    m.over(rgb(0x120c07), 0.95 * m.disc(0.1, 0.025, m.ragged(0.3)), rough=0.7)
    m.bowl(0.26, 0.009, rag)
    m.rim(0.26, 0.05, 0.0015)


def style_gravel(m, variant):
    rag = m.ragged(0.3, harmonics=18)
    # Stone crushed to a pale powder, chips light and dark thrown round it.
    m.over(rgb(0xb1a99b), 0.45 * lt.smooth(0.7, 0.2, m.r) * m.unit_noise(6), rough=0.92)
    dark, _ = m.specks(25 + 8 * variant, 0.2, 0.85, 0.01, 0.028, falloff=1.3)
    m.over(rgb(0x3b352e), 0.8 * dark, rough=0.8)
    chips, chip_shade = m.specks(40 + 10 * variant, 0.15, 0.88, 0.012, 0.035, falloff=1.3)
    m.over(lt.mix(rgb(0xb8b0a2), rgb(0xdcd5c8), chip_shade), 0.9 * chips, rough=0.75)
    m.over(lt.mix(rgb(0x4a433a), rgb(0x6d6455), m.unit_noise(4)), 0.8 * m.disc(0.26, 0.04, rag), rough=0.85)
    m.over(rgb(0x1c1915), 0.95 * m.disc(0.08, AA * 2.0, m.ragged(0.3)), rough=0.8)
    m.bowl(0.28, 0.007, rag)
    m.height += chips * 0.0012


def style_sand(m, variant):
    # Pale rays of spray thrown out, a soft shallow crater with no hard edge.
    rays = np.zeros((CELL, CELL), np.float32)
    for index in range(16 + 4 * variant):
        angle = (index + m.rng.uniform(-0.35, 0.35)) / (16 + 4 * variant) * 2.0 * math.pi
        m.stroke(m.path(angle, 0.22, m.rng.uniform(0.6, 0.9), 5, 0.08), 0.022, 0.004, rays)
    m.over(rgb(0xd8c6a2), 0.35 * rays * m.unit_noise(5), rough=0.95)
    m.over(rgb(0x9c8461), 0.45 * m.disc(0.42, 0.12, m.ragged(0.15)), rough=0.92)
    m.over(rgb(0x7f6a4c), 0.35 * m.disc(0.18, 0.06, m.ragged(0.2)), rough=0.9)
    m.bowl(0.44, 0.006)
    m.rim(0.44, 0.08, 0.0015)


def style_stone(m, variant, fresh=0x9a948a, crack=0x24211d, powder=0xcfc8bc, core=0x1f1c18, chip=0.42, cracks=5):
    rag = m.ragged(0.24, harmonics=24, falloff=0.9)
    m.over(rgb(powder), 0.3 * lt.smooth(0.85, 0.35, m.r) * m.unit_noise(6), rough=0.9)
    # The chip: fresher stone under the weathered face, broken into facets.
    value, border, tilt_x, tilt_y = m.facets(8 + variant, 4100 + 37 * variant + int(m.rng.integers(1000)))
    inside = m.disc(chip, AA * 1.5, rag)
    facet = rgb(fresh)[None, None, :] * (1.0 + 0.09 * value[..., None]) * (1.0 - 0.25 * border[..., None])
    m.over(facet, inside, rough=0.7)
    m.bowl(chip, 0.0035, rag)
    m.height += (tilt_x * m.x + tilt_y * m.y) * 0.0012 * inside
    # Hairline cracks running out of it, and its dark core.
    lines = np.zeros((CELL, CELL), np.float32)
    for index in range(cracks + variant):
        angle = (index + m.rng.uniform(-0.4, 0.4)) / (cracks + variant) * 2.0 * math.pi
        m.stroke(m.path(angle, 0.1, m.rng.uniform(0.55, 0.95), 7, 0.25), 0.0065, 0.002, lines)
    m.over(rgb(crack), 0.85 * lines, rough=0.85)
    m.height -= lines * 0.0004
    m.over(rgb(core), m.disc(0.08, AA * 1.5, m.ragged(0.3)), rough=0.85)


def style_wood(m, variant):
    rag = m.ragged(0.2)
    # Burnt round the hole, broken up.
    m.over(rgb(0x2a1a10), 0.55 * lt.smooth(0.34, 0.12, m.r) * m.unit_noise(5), rough=0.9)
    # Pale raw wood torn open round it.
    torn = m.disc(0.22, 0.03, m.ragged(0.35)) * m.unit_noise(3)
    m.over(rgb(0xb08e62), 0.85 * np.clip(torn * 1.6, 0.0, 1.0), rough=0.85)
    m.height += torn * 0.0005
    # Splinters torn out, most along the grain (across the cell).
    for _ in range(22 + 6 * variant):
        if m.rng.random() < 0.65:
            angle = (0.0 if m.rng.random() < 0.5 else math.pi) + m.rng.normal(0.0, 0.35)
            length = m.rng.uniform(0.2, 0.45)
        else:
            angle = m.rng.uniform(0.0, 2.0 * math.pi)
            length = m.rng.uniform(0.1, 0.25)
        start = m.rng.uniform(0.11, 0.18)
        splinter = m.stroke(m.path(angle, start, start + length, 4, 0.12), 0.022, 0.003)
        m.over(lt.mix(rgb(0x9a7650), rgb(0xd8bb90), m.rng.random()), 0.95 * splinter, rough=0.85)
        m.height += splinter * 0.0012
    m.over(rgb(0x120a06), m.disc(0.13, AA * 1.5, rag), rough=0.95)
    m.bowl(0.15, 0.006, rag)


def style_metal(m, variant):
    rag = m.ragged(0.2, harmonics=16)
    # Paint flaked off round the dent, in patches.
    flakes = m.disc(0.5, 0.03, m.ragged(0.25, harmonics=18)) * (m.noise(5) > -0.3)
    m.over(rgb(0x3a2a20), 0.75 * flakes, rough=0.8, metal=0.0)
    # Bare steel scraped bright, with fine radial scratches.
    # Few enough lines to stay some pixels apart at the ring (more would alias into moire), bent by the noise.
    scratch = np.clip(0.5 + 0.2 * np.sin(m.a * (44.0 + 8.0 * variant) + 3.0 * m.noise(6)) + 0.12 * m.noise(1.5), 0.0, 1.0)
    steel = lt.mix(rgb(0x8c9094), rgb(0xdfe2e6), scratch)
    m.over(steel, m.disc(0.36, 0.02, rag), rough=0.28, metal=1.0)
    # The round's grey lead smeared in the middle, and the dark core.
    m.over(rgb(0x5d5f63), 0.85 * m.disc(0.2, 0.02, m.ragged(0.3)), rough=0.55, metal=0.3)
    m.over(rgb(0x1e1f21), m.disc(0.09, AA * 1.5, m.ragged(0.3)), rough=0.6, metal=0.5)
    m.bowl(0.32, 0.003, rag)
    m.rim(0.33, 0.05, 0.0008)


def style_sheet_metal(m, variant):
    # Torn petals of uneven size and spacing (the angle itself wanders), with a jagged little tear along every edge.
    phase = m.rng.uniform(0.0, 2.0 * math.pi)
    petals = 5 + variant
    warped = m.a + 0.45 * np.sin(2.0 * m.a + m.rng.uniform(0.0, 2.0 * math.pi)) + 0.25 * np.sin(3.0 * m.a + phase)
    reach = np.clip(0.75 + m.ragged(0.3, harmonics=4, falloff=0.6), 0.3, 1.3)
    edge = 0.16 + 0.07 * reach * np.abs(np.sin(petals * 0.5 * warped + phase)) ** 2.2 + m.ragged(0.012, harmonics=28, falloff=0.5)
    # Scraped paint round it, then the torn rim bent out, bright, and the black hole.
    m.over(rgb(0x4a3a2e), 0.6 * m.ring(0.32, 0.46, 0.03) * (m.noise(5) > -0.2), rough=0.8, metal=0.0)
    m.over(rgb(0xa9adb1), 0.7 * m.disc(0.34, 0.03, m.ragged(0.2)), rough=0.4, metal=1.0)
    rim = lt.smooth(edge - AA, edge + AA, m.r) * lt.smooth(edge + 0.06, edge + 0.03, m.r)
    m.over(rgb(0xc9ccd0), 0.95 * rim, rough=0.35, metal=1.0)
    hole = lt.smooth(edge + AA, edge - AA, m.r)
    m.over(rgb(0x070707), hole, rough=1.0, metal=0.0)
    m.height += rim * 0.0015 - hole * 0.01


def style_glass(m, variant):
    lines = np.zeros((CELL, CELL), np.float32)
    count = 10 + 3 * variant
    radials = []
    for index in range(count):
        angle = (index + m.rng.uniform(-0.3, 0.3)) / count * 2.0 * math.pi
        points = m.path(angle, 0.04, m.rng.uniform(0.6, 0.97), 9, 0.12)
        radials.append(points)
        m.stroke(points, 0.008, 0.004, lines)
    # Broken rings between the radial cracks.
    for ring_index, step in enumerate((2, 4)):
        for index in range(count):
            if m.rng.random() < 0.3 + 0.1 * ring_index:
                continue
            a, b = radials[index], radials[(index + 1) % count]
            if step < len(a) and step < len(b):
                (ax, ay), (bx, by) = a[step], b[step]
                mid = (0.5 * (ax + bx) * 0.95, 0.5 * (ay + by) * 0.95)
                m.stroke([(ax, ay), mid, (bx, by)], 0.006, 0.006, lines)
    crushed, _ = m.specks(20 + 6 * variant, 0.02, 0.18, 0.008, 0.02)
    m.over(rgb(0xdfe8ee), 0.85 * np.clip(lines * (1.0 - 0.4 * m.r), 0.0, 1.0), rough=0.08, metal=0.0)
    m.over(rgb(0xe8eef2), 0.85 * np.maximum(m.disc(0.08, 0.02, m.ragged(0.3)), crushed), rough=0.1, metal=0.0)
    m.height += lines * 0.0003


def style_cloth(m, variant):
    m.over(rgb(0x2f2a24), 0.3 * lt.smooth(0.36, 0.1, m.r) * m.unit_noise(5), rough=0.95)
    fibers = np.zeros((CELL, CELL), np.float32)
    for _ in range(40 + 10 * variant):
        angle = m.rng.uniform(0.0, 2.0 * math.pi)
        start = m.rng.uniform(0.09, 0.13)
        m.stroke(m.path(angle, start, start + m.rng.uniform(0.03, 0.09), 2, 0.4), 0.006, 0.002, fibers)
    m.over(rgb(0xbdb3a0), 0.8 * fibers, rough=0.95)
    m.over(rgb(0x0d0b09), m.disc(0.12, AA * 1.5, m.ragged(0.3)), rough=1.0)
    m.bowl(0.13, 0.004)
    m.height += fibers * 0.0005


def style_hay(m, variant):
    m.over(rgb(0x3d2f16), 0.35 * lt.smooth(0.42, 0.12, m.r) * m.unit_noise(5), rough=0.95)
    for _ in range(28 + 8 * variant):
        angle = m.rng.uniform(0.0, 2.0 * math.pi)
        heading = m.rng.uniform(0.0, 2.0 * math.pi)
        start = m.rng.uniform(0.08, 0.6)
        length = m.rng.uniform(0.12, 0.32)
        x0, y0 = start * math.cos(angle), start * math.sin(angle)
        straw = m.stroke([(x0, y0), (x0 + length * math.cos(heading), y0 + length * math.sin(heading))], 0.012, 0.008)
        m.over(lt.mix(rgb(0xc9a85a), rgb(0xe6cf8a), m.rng.random()), 0.9 * straw, rough=0.85)
        m.height += straw * 0.001
    m.over(rgb(0x1a1208), m.disc(0.11, AA * 1.5, m.ragged(0.3)), rough=0.95)
    m.bowl(0.12, 0.005)


def style_bone(m, variant):
    style_stone(m, variant, fresh=0xd8cfbb, crack=0x5a5243, powder=0xe9e2d2, core=0x2a241c, chip=0.36, cracks=3)


def ring_cracks(m, inner, outer, count, crack, depth):
    """A heavy round's crack ring (the gun feel pass, 2026-10-22): radial cracks from the chip out to near the edge, joined
    by broken arcs bowing outward round it (the face split by the blow). Laid in crack's colour, cut into the relief."""
    lines = np.zeros((CELL, CELL), np.float32)
    radials = []
    for index in range(count):
        angle = (index + m.rng.uniform(-0.35, 0.35)) / count * 2.0 * math.pi
        points = m.path(angle, inner * 0.6, m.rng.uniform(outer * 0.85, outer), 8, 0.18)
        radials.append(points)
        m.stroke(points, 0.007, 0.0025, lines)
    for step in (3, 6):
        for index in range(count):
            if m.rng.random() < 0.3:
                continue
            (ax, ay), (bx, by) = radials[index][step], radials[(index + 1) % count][step]
            mid = (0.5 * (ax + bx) * 1.04, 0.5 * (ay + by) * 1.04)
            m.stroke([(ax, ay), mid, (bx, by)], 0.0055, 0.0045, lines)
    m.over(rgb(crack), 0.85 * lines, rough=0.85)
    m.height -= lines * depth
    return lines


def style_stone_heavy(m, variant):
    # The plain chip at the same size in the world (a wider cell: radius 8.5 cm, chip 0.34), a ring of pale crushed powder
    # round it, and the cracks ringing it out to the edge.
    m.over(rgb(0xcfc8bc), 0.28 * m.ring(0.3, 0.62, 0.06) * m.unit_noise(5), rough=0.9)
    style_stone(m, variant, chip=0.34, cracks=3)
    ring_cracks(m, 0.34, 0.95, 9 + variant, 0x24211d, 0.0005)


def style_wood_heavy(m, variant):
    # The scorched hole, split along the grain (across the cell) both ways nearly to the edge, raw wood lifted along the
    # splits, a wider scorch.
    m.over(rgb(0x2a1a10), 0.4 * lt.smooth(0.5, 0.2, m.r) * m.unit_noise(6), rough=0.9)
    style_wood(m, variant)
    splits = np.zeros((CELL, CELL), np.float32)
    raw = np.zeros((CELL, CELL), np.float32)
    for index in range(3 + variant):
        angle = (0.0 if index % 2 == 0 else math.pi) + m.rng.normal(0.0, 0.12)
        points = m.path(angle, 0.14, m.rng.uniform(0.7, 0.97), 8, 0.06)
        m.stroke(points, 0.02, 0.006, raw)
        m.stroke(points, 0.009, 0.002, splits)
    m.over(rgb(0xc9a876), 0.8 * np.clip(raw - splits, 0.0, 1.0), rough=0.85)
    m.height += (raw - splits) * 0.0008
    m.over(rgb(0x1e120a), 0.9 * splits, rough=0.95)
    m.height -= splits * 0.0012


# --- The element rounds' marks (2026-10-29): each reads on any surface (the game lays them on dirt, stone, wood or metal), so
# each carries its own dark hole and its own ground colour rather than the surface's ---

def branch(m, x, y, heading, length, steps, wander):
    """A wandering line from (x, y) along heading for about length (cell units): a burn line's or a frond's twig."""
    points = [(x, y)]
    step = length / steps
    for _ in range(steps):
        heading += m.rng.normal(0.0, wander)
        x += step * math.cos(heading)
        y += step * math.sin(heading)
        points.append((x, y))
    return points


def style_scorch(m, variant):
    # Cinder: heat-browned round it, a star of soot streaks thrown out, a charred disc pocked with cinders, a black core.
    rag = m.ragged(0.25, harmonics=16)
    m.over(rgb(0x5a3a22), 0.35 * lt.smooth(0.95, 0.45, m.r) * m.unit_noise(8), rough=0.9)
    streaks = np.zeros((CELL, CELL), np.float32)
    count = 14 + 4 * variant
    for index in range(count):
        angle = (index + m.rng.uniform(-0.4, 0.4)) / count * 2.0 * math.pi
        m.stroke(m.path(angle, 0.15, m.rng.uniform(0.55, 0.92), 5, 0.1), 0.05, 0.006, streaks)
    m.over(rgb(0x14100d), 0.75 * streaks * m.unit_noise(4), rough=0.95)
    m.over(lt.mix(rgb(0x0f0c0a), rgb(0x2a1f18), m.unit_noise(3)), 0.92 * m.disc(0.42, 0.08, rag), rough=0.95)
    # The char's edge scarred orange-brown (the lit edge itself is the game's flare: a decal can't glow).
    m.over(rgb(0x7a3a14), 0.55 * m.ring(0.3, 0.42, 0.04) * m.unit_noise(3), rough=0.8)
    cinders, shade = m.specks(10 + 4 * variant, 0.12, 0.38, 0.008, 0.02)
    m.over(lt.mix(rgb(0x8a3a10), rgb(0xc8682a), shade), 0.8 * cinders, rough=0.7)
    m.over(rgb(0x060505), m.disc(0.12, AA * 1.5, m.ragged(0.3)), rough=1.0)
    m.bowl(0.4, 0.004, rag)
    m.height -= streaks * 0.0002


def style_burn(m, variant):
    # Thunder: branching burn lines (a strike's tree) running out, a fused dark ring, its edge glassy and pale blue-white,
    # a blued sheen inside it, a black core.
    rag = m.ragged(0.12, harmonics=20)
    lines = np.zeros((CELL, CELL), np.float32)
    count = 7 + 2 * variant
    for index in range(count):
        angle = (index + m.rng.uniform(-0.35, 0.35)) / count * 2.0 * math.pi
        trunk = m.path(angle, 0.3, m.rng.uniform(0.7, 0.95), 7, 0.45)
        m.stroke(trunk, 0.012, 0.003, lines)
        for _ in range(2):
            k = int(m.rng.integers(1, len(trunk) - 2))
            bx, by = trunk[k]
            heading = math.atan2(by, bx) + (1.0 if m.rng.random() < 0.5 else -1.0) * m.rng.uniform(0.4, 0.9)
            m.stroke(branch(m, bx, by, heading, m.rng.uniform(0.12, 0.25), 4, 0.5), 0.007, 0.002, lines)
    m.over(rgb(0x1a1612), 0.85 * lines, rough=0.85)
    m.height -= lines * 0.0002
    m.over(lt.mix(rgb(0x15120f), rgb(0x2b2622), m.unit_noise(3)), 0.9 * m.ring(0.1, 0.34, 0.03), rough=0.45)
    m.over(rgb(0x3a4a78), 0.4 * m.ring(0.2, 0.3, 0.03) * m.unit_noise(4), rough=0.4)
    edge = m.ring(0.3, 0.36, 0.015) * (m.noise(4) > -0.4)
    m.over(rgb(0xb8c8e8), 0.85 * edge, rough=0.3)
    m.over(rgb(0x050505), m.disc(0.12, AA * 1.5, rag), rough=0.9)
    m.bowl(0.14, 0.004, rag)
    m.rim(0.33, 0.02, 0.0005)


def style_acid(m, variant):
    # Rot: a stain soaking out, gobs of the splash thrown round it, its wet glossy body eaten into dark pits, a bright wet rim
    # where it pools.
    rag = m.ragged(0.3, harmonics=10)
    m.over(rgb(0x5a6a1e), 0.45 * m.disc(0.62, 0.15, m.ragged(0.3, harmonics=8)) * m.unit_noise(6), rough=0.5)
    gobs, shade = m.specks(28 + 8 * variant, 0.3, 0.9, 0.02, 0.05, falloff=1.4)
    m.over(lt.mix(rgb(0x4e6a14), rgb(0x8aa02a), shade), 0.85 * gobs, rough=0.25)
    m.height += gobs * 0.0008
    m.over(lt.mix(rgb(0x3a5210), rgb(0x6a8a1e), m.unit_noise(4)), 0.9 * m.disc(0.36, 0.05, rag), rough=0.2)
    pits, _ = m.specks(16 + 6 * variant, 0.05, 0.3, 0.012, 0.03)
    m.over(rgb(0x1a2008), 0.9 * pits, rough=0.6)
    m.height -= pits * 0.0012
    m.over(rgb(0xa8c040), 0.5 * m.ring(0.3, 0.37, 0.02) * m.unit_noise(3), rough=0.15)
    m.over(rgb(0x0c0e05), m.disc(0.1, AA * 1.5, m.ragged(0.3)), rough=0.6)
    m.bowl(0.36, 0.004, rag)
    m.rim(0.36, 0.04, 0.0008)


def style_frost(m, variant):
    # Wardbreak: a cold haze of rime well out, feathery fronds of frost (stems with barbs), salt rime packed round the hole in
    # facets, teal-lit glints that catch the sun, a dark hole.
    m.over(rgb(0xc8dcdc), 0.3 * lt.smooth(0.95, 0.4, m.r) * m.unit_noise(5), rough=0.6)
    fronds = np.zeros((CELL, CELL), np.float32)
    count = 9 + 2 * variant
    for index in range(count):
        angle = (index + m.rng.uniform(-0.3, 0.3)) / count * 2.0 * math.pi
        stem = m.path(angle, 0.12, m.rng.uniform(0.6, 0.92), 6, 0.15)
        m.stroke(stem, 0.012, 0.004, fronds)
        for k in range(1, len(stem) - 1):
            x, y = stem[k]
            heading = math.atan2(y, x)
            for side in (-1.0, 1.0):
                barb = heading + side * m.rng.uniform(0.5, 0.9)
                length = m.rng.uniform(0.04, 0.09) * (1.0 - 0.5 * k / len(stem))
                m.stroke([(x, y), (x + length * math.cos(barb), y + length * math.sin(barb))], 0.006, 0.002, fronds)
    m.over(rgb(0xe6f2f0), 0.9 * fronds, rough=0.35)
    m.height += fronds * 0.0004
    value, border, tilt_x, tilt_y = m.facets(9 + variant, 5200 + 41 * variant + int(m.rng.integers(1000)))
    rime = m.disc(0.32, 0.04, m.ragged(0.25, harmonics=20))
    facet = rgb(0xd8ecea)[None, None, :] * (1.0 + 0.08 * value[..., None]) * (1.0 - 0.18 * border[..., None])
    m.over(facet, 0.95 * rime, rough=0.3)
    m.height += (tilt_x * m.x + tilt_y * m.y) * 0.0008 * rime + rime * 0.0008
    glints, shade = m.specks(30 + 8 * variant, 0.05, 0.7, 0.006, 0.016, falloff=1.2)
    m.over(lt.mix(rgb(0x9ee6dc), rgb(0xffffff), shade), 0.95 * glints, rough=0.08)
    m.over(rgb(0x1c2628), m.disc(0.09, AA * 1.5, m.ragged(0.3)), rough=0.7)
    m.bowl(0.1, 0.003)


def style_gilt(m, variant):
    # Hallow: gold flung out in drops and short rays, the splash's bright gilt body with a burnished ring, a dark punched
    # core ringed by deeper gold. Metallic: it shines as gold does in the sun.
    rag = m.ragged(0.28, harmonics=12)

    def gold(t):
        return lt.mix(rgb(0xa8782a), rgb(0xf0d27a), t)

    rays = np.zeros((CELL, CELL), np.float32)
    count = 12 + 3 * variant
    for index in range(count):
        angle = (index + m.rng.uniform(-0.4, 0.4)) / count * 2.0 * math.pi
        m.stroke(m.path(angle, 0.25, m.rng.uniform(0.5, 0.85), 4, 0.1), 0.022, 0.004, rays)
    drops, shade = m.specks(24 + 6 * variant, 0.35, 0.92, 0.012, 0.035, falloff=1.3)
    m.over(gold(shade), 0.95 * drops, rough=0.25, metal=1.0)
    m.over(gold(m.unit_noise(3)), 0.9 * rays, rough=0.25, metal=1.0)
    m.height += (drops + rays) * 0.0006
    m.over(gold(np.clip(0.4 + 0.5 * m.unit_noise(4), 0.0, 1.0)), m.disc(0.34, 0.03, rag), rough=0.2, metal=1.0)
    m.over(rgb(0xfff0b8), 0.7 * m.ring(0.24, 0.3, 0.02) * m.unit_noise(3), rough=0.12, metal=1.0)
    m.over(rgb(0x6a4a18), 0.8 * m.disc(0.16, 0.02, m.ragged(0.25)), rough=0.35, metal=1.0)
    m.over(rgb(0x0a0806), m.disc(0.08, AA * 1.5, m.ragged(0.3)), rough=0.8, metal=0.0)
    m.bowl(0.34, 0.003, rag)
    m.rim(0.34, 0.03, 0.0008)


PAINTERS = dict(Hole=style_hole, Dirt=style_dirt, Grass=style_grass, Gravel=style_gravel, Sand=style_sand,
                Stone=style_stone, Wood=style_wood, Metal=style_metal, SheetMetal=style_sheet_metal, Glass=style_glass,
                Cloth=style_cloth, Hay=style_hay, Bone=style_bone, StoneHeavy=style_stone_heavy, WoodHeavy=style_wood_heavy,
                Scorch=style_scorch, Burn=style_burn, Acid=style_acid, Frost=style_frost, Gilt=style_gilt)


def finish(m):
    """The cell's maps: colour (sRGB) and opacity, nothing past its edge, the clear texels given the mark's mean colour (so
    filtering brings no dark fringe in); the normal map; roughness and metallic."""
    alpha = np.clip(m.alpha * lt.smooth(0.98, 0.86, m.r), 0.0, 1.0)
    weight = alpha.sum()
    mean = (m.color * alpha[..., None]).sum(axis=(0, 1)) / max(weight, 1e-6)
    color = np.where(alpha[..., None] > 0.02, m.color, mean[None, None, :])
    bc = np.concatenate([np.clip(color, 0.0, 1.0), alpha[..., None]], axis=-1)
    normal = lt.normal_map(m.height * lt.smooth(0.98, 0.86, m.r), m.px_m)
    orm = np.stack([np.ones_like(alpha), np.clip(m.rough, 0.02, 1.0), np.clip(m.metal, 0.0, 1.0)], axis=-1)
    return bc, normal, orm


def preview(bc, normal):
    """Each cell over a surface of its kind, lit from the top left by its relief."""
    sheet = np.zeros(bc.shape[:2] + (3,), np.float32)
    light = np.array([-0.45, -0.5, 0.74], np.float32)
    light /= np.linalg.norm(light)
    for style, name in enumerate(STYLES):
        for variant in range(VARIANTS):
            index = style * VARIANTS + variant
            box = (slice((index // COLUMNS) * CELL, (index // COLUMNS + 1) * CELL),
                   slice((index % COLUMNS) * CELL, (index % COLUMNS + 1) * CELL))
            alpha = bc[box][..., 3:4]
            n = normal[box].astype(np.float32) / 127.5 - 1.0
            # The map is DirectX (green down); the light is in image space, y down as well.
            lit = np.clip((n * light[None, None, :]).sum(axis=-1, keepdims=True), 0.0, 1.0)
            flat = float(light[2])
            ground = rgb(BACKDROPS[name])[None, None, :]
            mixed = ground * (1.0 - alpha) + bc[box][..., :3] * alpha
            shade = flat + (lit - flat) * alpha
            sheet[box] = np.clip(mixed * (0.45 + 0.75 * shade), 0.0, 1.0)
    os.makedirs(os.path.dirname(PREVIEW), exist_ok=True)
    lt.write_png(PREVIEW, lt.to8(sheet))
    # Closer looks: the first variant of a few styles, twice the size (the surfaces' and the element marks').
    for names, suffix in ((ZOOM, '_zoom.png'), (ZOOM_ELEMENTS, '_zoom_elements.png')):
        crops = []
        for name in names:
            index = STYLES.index(name) * VARIANTS
            crop = sheet[(index // COLUMNS) * CELL:(index // COLUMNS + 1) * CELL, (index % COLUMNS) * CELL:(index % COLUMNS + 1) * CELL]
            crops.append(np.repeat(np.repeat(crop, 2, axis=0), 2, axis=1))
        lt.write_png(PREVIEW.replace('_sheet.png', suffix), lt.to8(np.concatenate(crops, axis=1)))


def main():
    # Optional, after Blender's "--": --out <folder> writes the maps and the sheet there instead (a trial run).
    global OUT_DIR, PREVIEW
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    if '--out' in argv and argv.index('--out') + 1 < len(argv):
        OUT_DIR = argv[argv.index('--out') + 1]
        PREVIEW = os.path.join(OUT_DIR, 'BulletMarks_sheet.png')
        os.makedirs(OUT_DIR, exist_ok=True)
    shape = (ROWS * CELL, COLUMNS * CELL)
    bc = np.zeros(shape + (4,), np.float32)
    normal = np.zeros(shape + (3,), np.uint8)
    normal[...] = (128, 128, 255)
    orm = np.zeros(shape + (3,), np.float32)
    orm[..., 0] = 1.0
    orm[..., 1] = 0.85
    for style, name in enumerate(STYLES):
        for variant in range(VARIANTS):
            index = style * VARIANTS + variant
            mark = Mark(1009 + 7919 * index, RADIUS_CM[name])
            PAINTERS[name](mark, variant)
            cell_bc, cell_normal, cell_orm = finish(mark)
            box = (slice((index // COLUMNS) * CELL, (index // COLUMNS + 1) * CELL),
                   slice((index % COLUMNS) * CELL, (index % COLUMNS + 1) * CELL))
            bc[box] = cell_bc
            normal[box] = cell_normal
            orm[box] = cell_orm
            print(f'BULLETMARKS {name} {variant}: cell {index}, covers {float((cell_bc[..., 3] > 0.05).mean()) * 100.0:.0f}%')
    lt.write_png(os.path.join(OUT_DIR, 'T_BulletMarks_BC.png'), lt.to8(bc))
    lt.write_png(os.path.join(OUT_DIR, 'T_BulletMarks_N.png'), normal)
    lt.write_png(os.path.join(OUT_DIR, 'T_BulletMarks_ORM.png'), lt.to8(orm))
    preview(bc, normal)
    print(f'BULLETMARKS done: {OUT_DIR} and {PREVIEW}')


main()
