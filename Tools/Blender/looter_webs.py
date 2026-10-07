"""The web atlas of the Sink in Ransom's Rest (Docs/Areas/RansomsRest.md: the Sink, the Webwood, Main 5): one texture
set, Art/Textures/Webs, shared by every web card and egg sac of Art/Models/Props/Sink.py. Painted by numpy like
looter_textures (fixed seeds, no clock-seeded noise), so every run writes the same bytes.

  T_Webs_BC.png   2048 px: sRGB colour and straight alpha, the cards' mask (M_WorldFoliage clips it at 1/3). The
                  colour of transparent texels is spread from their neighbours, so filtering never pulls in black.
  T_Webs_N.png    1024 px: DirectX normal map (green down): round threads, the tents' layers, the sacs' wound silk.
  T_Webs_ORM.png  1024 px, linear: R occlusion (the tents' gaps, the sacs' creases), G roughness (silk 0.6-0.9, matte),
                  B 0.

A sheet web (Hammock, Sheet) is a mat: fine, short fibres crossing every which way, matted into a dense gauze over
nearly three quarters of it (felt()), mottled, holed and torn so the ground or rock shows through, denser where its
anchor lines gather it, thinning at its edge into loose fibres; a few long, single strands lie over it and run out to
its anchors. Long strands with gaps between them (gauze(), the tangle and the tatters) keep their lines down the mips:
a sheet made of them reads as a heap of straw from 10 m, where a felt evens out into a veil with holes. Threads are
fine (about 1 cm), wavy, with fluff and grit caught on them. The silk is a dusty warm ivory: lighter than the rock,
never white. Everything is drawn at 320 px/m. Fine silk survives the mips only if Unreal keeps its coverage: T_Webs_BC
wants "Scale Mips for Alpha Coverage" at 0.333, or the webs thin out with distance on Medium (whose texture bias shows
the atlas at 1024 px).

Cells (px of the atlas; x right, y down from the top, so V = 1 - y / 2048; uv() converts):
  Orb      0-768 x 0-768        a torn orb web 2 m across: seven frame corners with anchor lines out to the cell's
                                edges, 30 radials, a capture spiral (a wedge torn out at the lower left, the broken
                                radials hanging from the hub), the hub, two wrapped prey bundles
  Hammock  768-1536 x 0-384     a sheet web: a lens 2.2 x 0.75 m of matted gauze (felt), a few long anchor strands out
                                to its long edges (the cell's top and bottom), its tips tapering into strands
  Tangle   768-1536 x 384-768   a cobweb tangle: gauze where its threads crowd (densest in the middle, wisps at three
                                knots), fine threads sagging between knots, fluff, dust and a leaf
  Tatters  1536-2048 x 0-768    five tatters of gauze hanging from anchor threads (1.5-2.3 m), dense where they're
                                glued, coming apart lower down into wavy strands that taper to nothing
  Sheet    0-2048 x 768-1152    a band of matted gauze (felt) tiling along U (6.4 m), gathered along a few long
                                strands, anchor strands out to both edges
  Fringe   0-2048 x 1152-1280   the torn edge of a hatched sac's silk, tiling along U: the Sac cell's wound silk
                                solid along the bottom, torn up into tongues with holes, fibres and strands pulled out
  Strands  0-2048 x 1280-1408   four lanes of 32 px tiling along U (LANES), each a fine wavy thread: Cord, Rope (two
                                finer threads twisted), Tufted (fluff caught on it), Ribbon (a loose bundle of strands)
  Sac      0-2048 x 1408-1792   the egg sacs' wound silk, opaque and matte, tiling along U; along V it tiles every
                                368 px between 8 px pads (SAC_ROWS)
  Wrap     0-1024 x 1792-2048   loose silk tiling along V every 240 px between 8 px pads (WRAP_ROWS), strands running
                                along V: silk wound round a branch on the left (about 40%), the egg sacs' fuzzy halo
                                from WRAP_SPLIT on (about 8%)
  Tent     1024-2048 x 1792-2048  a tent web, one sprite: dense layered silk from the crotch of a fork (x 0) to its
                                free top end (x 1024) fraying into fibres, across from the trunk's edge to the limb's
The models need the vector layouts of the sprite cells (orb_layout, hammock_layout, tangle_layout, tatter_layout):
the painter draws from them and Sink.py trims each card's outline to them, so a card's shape follows its web.

    Tools\\artrun.ps1 -Script Tools\\Blender\\looter_webs.py                       the atlas preview only
    Tools\\artrun.ps1 -Script Tools\\Blender\\looter_webs.py -ScriptArgs --install  writes Art/Textures/Webs (the only
                                                                                 step that writes it)
    ... -ScriptArgs --out=<folder>                                               the three files into a folder (to
                                                                                 compare two runs byte for byte)
The preview is Saved/ArtPreviews/RansomsRest/Sink/Atlas.png (the colour over a dark checker, cells outlined) and
Atlas_lit.png (lit with the normal map and occlusion).
"""
import math
import os
import random
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import looter_textures as lt  # noqa: E402

SET = 'Webs'
SIZE = 2048               # the colour map
MAP_SIZE = 1024           # the normal and ORM maps
PX_M = 320.0              # px per metre in the colour map
TEXTURE_DIR = os.path.join(lt.TEXTURE_DIR, SET)
PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Sink')

REGIONS = {
    'Orb': (0, 0, 768, 768),
    'Hammock': (768, 0, 1536, 384),
    'Tangle': (768, 384, 1536, 768),
    'Tatters': (1536, 0, 2048, 768),
    'Sheet': (0, 768, 2048, 1152),
    'Fringe': (0, 1152, 2048, 1280),
    'Strands': (0, 1280, 2048, 1408),
    'Sac': (0, 1408, 2048, 1792),
    'Wrap': (0, 1792, 1024, 2048),
    'Tent': (1024, 1792, 2048, 2048),
}
# Rows of the Strands cell (from its top) holding each lane; its thread runs along the lane's middle.
LANES = {'Cord': (0, 32), 'Rope': (32, 64), 'Tufted': (64, 96), 'Ribbon': (96, 128)}
# The tiling rows of the padded bands (from the cell's top): the content repeats every period rows, and the pads
# copy the far side so filtering at the seams sees the right texels.
SAC_PAD, SAC_PERIOD = 8, 368
WRAP_PAD, WRAP_PERIOD = 8, 240
SAC_ROWS = (SAC_PAD, SAC_PAD + SAC_PERIOD)
WRAP_ROWS = (WRAP_PAD, WRAP_PAD + WRAP_PERIOD)

# Silk, sRGB. Old webs in a pit gather dust: a dusty warm ivory, darker where it's grimy, always lighter than the
# rock (RockCliff averages 8e8170) but not paper-white, so it sits in the scene instead of glowing. Nothing near a
# loot colour.
SILK_HI = 0xd9cfba
SILK = 0xc9bda5
SILK_GREY = 0xafa48f
SILK_DUSTY = 0x8f8572
DUST = 0x5e5446
LEAF = 0x8c6b45
BUNDLE = 0xbfb49f
SAC_SILK = 0xd6ccb6
SAC_SHADE = 0xaca08a


def uv(region, x, y):
    """Atlas UV of a point given in a cell's own pixels (x right, y down from the cell's top-left corner)."""
    x0, y0, _, _ = REGIONS[region]
    return ((x0 + x) / SIZE, 1.0 - (y0 + y) / SIZE)


def register():
    """Makes looter_textures treat Webs as a masked foliage set, as the Unreal master does: lt.material('Webs') then
    clips the alpha and lights back faces with the front's normal in previews."""
    lt.SETS.setdefault(SET, dict(size=SIZE, density=PX_M, color=0xcbc3b5, roughness=0.5, master='WorldFoliage',
                                 alpha=True))


def _log(message):
    print(f'WEBS: {message}', flush=True)


# --- Painting ---

def streaks(shape, seed, angle, along, across):
    """Tileable fibrous noise (mean 0, deviation 1): white noise blurred by `along` px in the direction angle (radians,
    from image x toward image y) and by `across` px across it."""
    h, w = shape
    fy = np.fft.fftfreq(h).astype(np.float32)[:, None]
    fx = np.fft.rfftfreq(w).astype(np.float32)[None, :]
    c, s = math.cos(angle), math.sin(angle)
    fa = fx * c + fy * s
    fc = -fx * s + fy * c
    gauss = np.exp(-2.0 * math.pi ** 2 * ((along * fa) ** 2 + (across * fc) ** 2)).astype(np.float32)
    white = np.random.default_rng(seed).standard_normal(shape).astype(np.float32)
    n = np.fft.irfft2(np.fft.rfft2(white) * gauss, s=shape).astype(np.float32)
    return n / (n.std() + 1e-8)


def periodic(n, seed, sigma):
    """A smooth 1D noise of n samples that wraps around (mean 0, deviation 1)."""
    return lt.noise((1, n), seed, sigma, 0.5)[0]


def grid(w, h):
    x = np.arange(w, dtype=np.float32)[None, :] + 0.5
    y = np.arange(h, dtype=np.float32)[:, None] + 0.5
    return np.broadcast_to(x, (h, w)), np.broadcast_to(y, (h, w))


class Plate:
    """One cell being painted, in its own pixels (x right, y down). Layers composite over what is there: colour and
    roughness premultiplied by coverage, heights (metres) by maximum. A plate that wraps tiles along that axis:
    strokes crossing its edge come back on the other side."""

    def __init__(self, w, h, wrap_x=False, wrap_y=False):
        self.w, self.h = w, h
        self.wrap_x, self.wrap_y = wrap_x, wrap_y
        self.col = np.zeros((h, w, 3), np.float32)
        self.cov = np.zeros((h, w), np.float32)
        self.rgh = np.zeros((h, w), np.float32)
        self.hgt = np.zeros((h, w), np.float32)
        self.occ = np.ones((h, w), np.float32)

    def layer(self, a, color, rough, height=None, where=(slice(None), slice(None))):
        """Composites coverage a (0..1) of color (one colour or an image) and roughness over the plate."""
        a = np.clip(a, 0.0, 1.0).astype(np.float32)
        keep = 1.0 - a
        color = np.asarray(color, np.float32)
        self.col[where] = self.col[where] * keep[..., None] + color * a[..., None]
        self.cov[where] = self.cov[where] * keep + a
        self.rgh[where] = self.rgh[where] * keep + np.asarray(rough, np.float32) * a
        if height is not None:
            self.hgt[where] = np.maximum(self.hgt[where], height)

    def over(self, top):
        """Composites another plate of the same size (painted on its own) over this one."""
        a = np.clip(top.cov, 0.0, 1.0)
        keep = 1.0 - a
        self.col = self.col * keep[..., None] + top.col
        self.rgh = self.rgh * keep + top.rgh
        self.cov = self.cov * keep + a
        self.hgt = np.maximum(self.hgt, top.hgt)
        self.occ = self.occ * top.occ

    def _offsets(self, x0, y0, x1, y1):
        xs, ys = [0.0], [0.0]
        if self.wrap_x:
            if x0 < 0.0:
                xs.append(float(self.w))
            if x1 > self.w:
                xs.append(-float(self.w))
        if self.wrap_y:
            if y0 < 0.0:
                ys.append(float(self.h))
            if y1 > self.h:
                ys.append(-float(self.h))
        return [(ox, oy) for oy in ys for ox in xs]

    def _boxes(self, x0, y0, x1, y1):
        """The pixel boxes a bounding box covers, with the offset that brings each onto the plate."""
        for ox, oy in self._offsets(x0, y0, x1, y1):
            ix0, ix1 = max(int(math.floor(x0 + ox)), 0), min(int(math.ceil(x1 + ox)), self.w)
            iy0, iy1 = max(int(math.floor(y0 + oy)), 0), min(int(math.ceil(y1 + oy)), self.h)
            if ix0 < ix1 and iy0 < iy1:
                x = np.arange(ix0, ix1, dtype=np.float32)[None, :] + 0.5 - ox
                y = np.arange(iy0, iy1, dtype=np.float32)[:, None] + 0.5 - oy
                yield (slice(iy0, iy1), slice(ix0, ix1)), x, y

    def segment(self, p0, p1, r0, r1, color, rough=0.5, lift=0.6, shade=0.14):
        """A round stroke (a cord) from p0 to p1, radius r0 to r1 px, antialiased, lit a little brighter on its
        crown, raised lift times its radius."""
        (x0, y0), (x1, y1) = p0, p1
        pad = max(r0, r1) + 1.5
        dx, dy = x1 - x0, y1 - y0
        l2 = dx * dx + dy * dy
        color = np.asarray(color, np.float32)
        for where, x, y in self._boxes(min(x0, x1) - pad, min(y0, y1) - pad, max(x0, x1) + pad, max(y0, y1) + pad):
            if l2 > 1e-8:
                t = np.clip(((x - x0) * dx + (y - y0) * dy) / l2, 0.0, 1.0)
            else:
                t = np.zeros(np.broadcast(x, y).shape, np.float32)
            d = np.hypot(x - (x0 + t * dx), y - (y0 + t * dy))
            r = r0 + (r1 - r0) * t
            a = np.clip(r - d + 0.5, 0.0, 1.0)
            if not a.any():
                continue
            crown = np.sqrt(np.clip(1.0 - (d / np.maximum(r, 0.35)) ** 2, 0.0, 1.0))
            if lift >= 0.0:
                self.layer(a, color * (1.0 - shade + shade * crown)[..., None], rough, lift * r / PX_M * crown, where)
            else:
                # A crease: pressed in instead of raised.
                self.layer(a, color * (1.0 - shade + shade * crown)[..., None], rough, None, where)
                self.hgt[where] += lift * r / PX_M * crown * a

    def line(self, points, radius, color, rough=0.5, lift=0.6, shade=0.14):
        """A polyline of round strokes; radius is one value or one per point."""
        radii = list(radius) if isinstance(radius, (list, tuple, np.ndarray)) else [radius] * len(points)
        for i in range(len(points) - 1):
            self.segment(points[i], points[i + 1], radii[i], radii[i + 1], color, rough, lift, shade)

    def blob(self, center, radii, turn, color, rough=0.55, lift=0.5, stripes=0.0, seed=0):
        """A filled ellipse (a wrapped bundle, a clump): domed, optionally with wrap stripes across it."""
        cx, cy = center
        rx, ry = radii
        c, s = math.cos(math.radians(turn)), math.sin(math.radians(turn))
        pad = max(rx, ry) + 2.0
        color = np.asarray(color, np.float32)
        for where, x, y in self._boxes(cx - pad, cy - pad, cx + pad, cy + pad):
            u = (x - cx) * c + (y - cy) * s
            v = -(x - cx) * s + (y - cy) * c
            e = np.sqrt((u / rx) ** 2 + (v / ry) ** 2)
            a = np.clip((1.0 - e) * min(rx, ry) + 0.5, 0.0, 1.0)
            if not a.any():
                continue
            dome = np.sqrt(np.clip(1.0 - e * e, 0.0, 1.0))
            tone = 0.78 + 0.22 * dome
            if stripes:
                band = 0.5 + 0.5 * np.sin(u * 0.55 + v * 1.9 + seed)
                tone = tone * (1.0 - stripes * lt.smooth(0.55, 0.95, band))
            self.layer(a, color * tone[..., None], rough, lift * min(rx, ry) / PX_M * dome, where)

    def speck(self, center, radius, color, amount):
        """Tints what is painted under a small disc (dust on a thread) without adding coverage."""
        cx, cy = center
        color = np.asarray(color, np.float32)
        for where, x, y in self._boxes(cx - radius - 1.0, cy - radius - 1.0, cx + radius + 1.0, cy + radius + 1.0):
            a = np.clip(radius - np.hypot(x - cx, y - cy) + 0.5, 0.0, 1.0) * amount
            cov = self.cov[where]
            self.col[where] += (color * cov[..., None] - self.col[where]) * a[..., None]

    def finish(self, ao_radii=(2, 6), ao_strength=0.6, normal_strength=1.0):
        """Straight colour, coverage, normal (-1..1, DirectX), occlusion and roughness of the plate."""
        cov = np.clip(self.cov, 0.0, 1.0)
        safe = np.maximum(cov, 1e-6)
        color = np.where(cov[..., None] > 1e-6, self.col / safe[..., None], 0.0).astype(np.float32)
        rough = np.where(cov > 1e-6, self.rgh / safe, 0.6).astype(np.float32)
        normal = height_normals(self.hgt, normal_strength)
        ao = lt.occlusion(self.hgt, 1.0 / PX_M, radii_px=ao_radii, strength=ao_strength) * self.occ
        return dict(color=np.clip(color, 0.0, 1.0), alpha=cov, normal=normal, ao=np.clip(ao, 0.0, 1.0),
                    rough=np.clip(rough, 0.0, 1.0))


def height_normals(height, strength=1.0):
    """DirectX normals (green down) of a height field in metres at PX_M, wrapping round the plate's edges."""
    px = 1.0 / PX_M
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) / (2.0 * px)
    drow = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) / (2.0 * px)
    n = np.stack([-dx * strength, -drow * strength, np.ones_like(dx)], axis=-1)
    return (n / np.linalg.norm(n, axis=-1, keepdims=True)).astype(np.float32)


def tone(rnd, color, spread=0.05):
    return lt.rgb(color) * (1.0 + rnd.uniform(-spread, spread))


def bezier(p0, c, p1, n):
    out = []
    for i in range(n):
        t = i / (n - 1)
        out.append(((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * c[0] + t * t * p1[0],
                    (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * c[1] + t * t * p1[1]))
    return out


def sagging(p0, p1, sag, n=8):
    """A thread strung from p0 to p1, bowed down (+y) by sag times its length in the middle."""
    length = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
    mid = ((p0[0] + p1[0]) * 0.5, (p0[1] + p1[1]) * 0.5 + 2.0 * sag * length)
    return bezier(p0, mid, p1, n)


def hanging(start, direction, length, droop, steps=7):
    """A broken thread's loose end: from start along direction, curling down (+y) as it goes."""
    dx, dy = direction
    n = math.hypot(dx, dy) or 1.0
    dx, dy = dx / n, dy / n
    return [(start[0] + dx * length * t, start[1] + dy * length * t + droop * t * t)
            for t in (i / steps for i in range(steps + 1))]


def ray_hit(origin, direction, polylines):
    """The distance along a ray to the nearest of the polylines' segments (None if it misses them all)."""
    best = None
    ox, oy = origin
    dx, dy = direction
    for line in polylines:
        for (ax, ay), (bx, by) in zip(line, line[1:]):
            ex, ey = bx - ax, by - ay
            den = dx * ey - dy * ex
            if abs(den) < 1e-9:
                continue
            t = ((ax - ox) * ey - (ay - oy) * ex) / den
            s = ((ax - ox) * dy - (ay - oy) * dx) / den
            if t > 0.0 and -1e-6 <= s <= 1.0 + 1e-6 and (best is None or t < best):
                best = t
    return best


def dust(plate, rnd, lines, count, color=DUST):
    """Specks of dust caught on threads (picked along the given polylines)."""
    if not lines:
        return
    for _ in range(count):
        line = lines[rnd.randrange(len(lines))]
        k = rnd.randrange(max(len(line) - 1, 1))
        a, b = line[k], line[min(k + 1, len(line) - 1)]
        t = rnd.random()
        plate.speck((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t), rnd.uniform(0.8, 1.9), lt.rgb(color),
                    rnd.uniform(0.35, 0.8))


def clump(plate, rnd, center, size, color, count=14, radius=0.9):
    """A tuft of silk: a few overlapping knots, short fibres of uneven length straying out of them."""
    cx, cy = center
    for _ in range(rnd.choice((2, 3))):
        off = (rnd.uniform(-0.3, 0.3) * size, rnd.uniform(-0.2, 0.2) * size)
        plate.blob((cx + off[0], cy + off[1]), (size * rnd.uniform(0.2, 0.42), size * rnd.uniform(0.15, 0.3)),
                   rnd.uniform(0.0, 180.0), color * rnd.uniform(0.94, 1.04), lift=0.4)
    for _ in range(count):
        a = rnd.uniform(0.0, 2.0 * math.pi)
        length = size * rnd.uniform(0.3, 1.15) ** 1.5
        bend = rnd.uniform(-0.9, 0.9)
        pts = [(cx + math.cos(a + bend * t) * length * t, cy + math.sin(a + bend * t) * length * t)
               for t in (0.0, 0.4, 0.75, 1.0)]
        plate.line(pts, [radius * 1.2, radius, radius * 0.8, radius * 0.55], color * rnd.uniform(0.95, 1.05),
                   rough=0.5, lift=0.4)


def attachment(plate, rnd, center, color):
    """Where a thread is glued down: a small, flat, dusty knot with two or three fine fibres splayed round it."""
    plate.blob(center, (rnd.uniform(1.8, 2.8), rnd.uniform(1.2, 1.8)), rnd.uniform(0.0, 180.0), color, lift=0.2)
    for _ in range(rnd.choice((2, 3))):
        a = rnd.uniform(0.0, 2.0 * math.pi)
        length = rnd.uniform(3.0, 7.0)
        plate.line([center, (center[0] + math.cos(a) * length, center[1] + math.sin(a) * length)], [0.6, 0.35],
                   color, rough=0.62, lift=0.2)


def _box_blur(a, r, wrap_x=False, wrap_y=False):
    """a averaged over a (2r + 1) px square, wrapping along the axes that tile."""
    out = np.asarray(a, np.float32)
    for axis, wrap in ((0, wrap_y), (1, wrap_x)):
        pad = [(0, 0), (0, 0)]
        pad[axis] = (r + 1, r)
        c = np.cumsum(np.pad(out, pad, mode='wrap' if wrap else 'edge'), axis=axis, dtype=np.float64)
        n = out.shape[axis]
        hi = np.take(c, np.arange(2 * r + 1, 2 * r + 1 + n), axis=axis)
        lo = np.take(c, np.arange(0, n), axis=axis)
        out = ((hi - lo) / (2 * r + 1)).astype(np.float32)
    return out


def gauze(plate, rnd, inside, angle, coverage, extra=None, length=(30.0, 140.0), radius=(0.4, 0.62), edge=24.0,
          cross=0.35, wander=0.22, colors=(SILK_GREY, SILK_GREY, SILK, SILK, SILK_DUSTY, SILK_HI), fluff=1.0, wisps=0.3,
          max_strands=20000):
    """Old cobweb: fine strands crossing at random inside a region, covering about `coverage` of it (a number or an
    image) so what is behind it shows through, more where `extra` (an image, 0..1) asks for it, thinning out over the
    last `edge` px inside the region's edge. inside: px within the region's (ragged) edge. Strands wander (each turns
    a little at every step) mostly along angle (radians from image x), a share `cross` of them across it; a share
    `wisps` come with a few near-parallel strands drifting apart; most are dimmer than the few bright ones, and short
    fluff lies between them. Strands stop where they leave the region and go where the gauze is still thinner than
    asked, so it comes out even. Returns them."""
    h, w = plate.h, plate.w
    target = coverage * lt.smooth(-0.25 * edge, edge, inside)
    if extra is not None:
        target = np.maximum(target, np.clip(extra, 0.0, 0.9) * lt.smooth(-6.0, 4.0, inside))
    region = target > 0.03
    if not region.any():
        return []
    goal = float(target[region].mean())

    def keeps(q):
        if not plate.wrap_x and not 0.0 <= q[0] < w or not plate.wrap_y and not 0.0 <= q[1] < h:
            return False
        return inside[int(q[1]) % h, int(q[0]) % w] > -0.25 * edge

    def strand(x0, y0, a, span, r, color):
        """A wandering strand through (x0, y0), cut where it leaves the region."""
        steps = 7
        half = []
        for sign in (1.0, -1.0):
            q, d = (x0, y0), a if sign > 0 else a + math.pi
            pts = []
            for _ in range(steps // 2):
                d += rnd.gauss(0.0, wander)
                q = (q[0] + math.cos(d) * span / steps, q[1] + math.sin(d) * span / steps)
                if not keeps(q):
                    break
                pts.append(q)
            half.append(pts)
        pts = half[1][::-1] + [(x0, y0)] + half[0]
        if len(pts) < 2:
            return None
        radii = [r * (0.5 if i in (0, len(pts) - 1) else 1.0) for i in range(len(pts))]
        plate.line(pts, radii, color, rough=0.7, lift=0.35, shade=0.06)
        return pts
    lines = []
    while len(lines) < max_strands:
        local = _box_blur(np.minimum(plate.cov, 1.0), 10, plate.wrap_x, plate.wrap_y)
        if float(local[region].mean()) >= goal * 0.98:
            break
        need = np.clip(target - local, 0.0, None)
        for _ in range(80):
            for _try in range(24):
                x0, y0 = rnd.uniform(0.0, w), rnd.uniform(0.0, h)
                if rnd.random() * 0.5 < need[int(y0) % h, int(x0) % w]:
                    break
            else:
                continue
            a = angle + (rnd.uniform(0.6, 1.5) * rnd.choice((-1.0, 1.0)) if rnd.random() < cross else
                         rnd.gauss(0.0, 0.3))
            span = rnd.uniform(*length)
            color = tone(rnd, rnd.choice(colors), 0.05)
            pts = strand(x0, y0, a, span, rnd.uniform(*radius), color)
            if pts is None:
                continue
            lines.append(pts)
            if rnd.random() < wisps:            # a wisp: a few more beside it, drifting apart
                nx, ny = -math.sin(a), math.cos(a)
                for k in range(rnd.choice((2, 3, 4))):
                    off = rnd.uniform(1.5, 3.5) * (k + 1) * rnd.choice((-1.0, 1.0))
                    more = strand(x0 + nx * off, y0 + ny * off, a + rnd.gauss(0.0, 0.08), span * rnd.uniform(0.6, 1.0),
                                  rnd.uniform(radius[0], radius[1]) * 0.85, color * rnd.uniform(0.92, 1.04))
                    if more is not None:
                        lines.append(more)
    # Fluff: short fibres caught between the strands, where there is gauze.
    for _ in range(int(fluff * region.sum() / 700.0)):
        x0, y0 = rnd.uniform(0.0, w), rnd.uniform(0.0, h)
        if target[int(y0) % h, int(x0) % w] < 0.05:
            continue
        a, length_ = rnd.uniform(0.0, 2.0 * math.pi), rnd.uniform(5.0, 16.0)
        pts = [(x0 + math.cos(a + 0.7 * t * t) * length_ * t, y0 + math.sin(a + 0.7 * t * t) * length_ * t)
               for t in (0.0, 0.5, 1.0)]
        if all(keeps(q) for q in pts):
            plate.line(pts, [0.42, 0.38, 0.3], tone(rnd, rnd.choice(colors), 0.05), rough=0.7, lift=0.3)
    return lines


# The felt's fibre directions: six, spread round the half turn, a little off even so no direction lines up.
FELT_TURNS = (0.0, 0.62, 1.01, 1.63, 2.05, 2.6)


def felt(plate, inside, core, seed, angle=0.0, edge=30.0, extra=None, fibre=(4.5, 0.5)):
    """Old sheet web as a mat: fine, short fibres crossing every which way, matted into a dense gauze over about `core`
    of it where it's whole, more where `extra` (an image, 0..1) asks for it, thinning over the last `edge` px inside
    the region's edge (inside: px within it) until only loose fibres are left. Mottled thinner and thicker, pinholes,
    holes and a few slanting rents torn in it, dustier in patches. Fibres run `fibre` px (along, across: blur sigmas),
    a few more of them along angle (radians from image x), the way the sheet is stretched; kept short, or they line up
    into straws. Composited over the plate; returns its coverage."""
    shape = (plate.h, plate.w)
    fib = None
    for k, turn in enumerate(FELT_TURNS):
        f = streaks(shape, seed + k, angle + turn, fibre[0], fibre[1]) + 0.25 * math.cos(2.0 * turn)
        fib = f if fib is None else np.maximum(fib, f)
    mottle = lt.noise(shape, seed + 10, 16.0, octaves=2)
    pinholes = lt.smooth(-0.15, 0.15, lt.noise(shape, seed + 11, 4.5) - 1.75)
    holes = lt.smooth(-0.2, 0.2, lt.noise(shape, seed + 12, 11.0) - 1.6)
    rents = lt.smooth(-0.2, 0.2, np.maximum(streaks(shape, seed + 13, angle + 0.45, 16.0, 6.0),
                                            streaks(shape, seed + 16, angle - 0.6, 16.0, 6.0)) - 2.35)
    gone = np.maximum(np.maximum(pinholes, holes), rents)
    dens = core * lt.smooth(-0.3 * edge, edge, inside)
    if extra is not None:
        dens = np.maximum(dens, np.clip(extra, 0.0, 0.95) * lt.smooth(-8.0, 6.0, inside))
    dens = np.clip(dens * (0.85 + 0.3 * mottle) * (1.0 - 0.95 * gone), 0.0, 0.97).astype(np.float32)
    # Each spot keeps the share dens of the fibre field: the field's quantiles say where to cut it for each share.
    table = np.linspace(0.0, 1.0, 65)
    cut = np.interp(dens, table, np.quantile(fib, 1.0 - table)).astype(np.float32)
    a = lt.smooth(cut - 0.2, cut + 0.2, fib) * lt.smooth(0.0, 0.04, dens)
    crown = np.clip((fib - cut) / 1.2, 0.0, 1.0)               # how far a fibre stands above the cut: its crown
    color = lt.mix(lt.rgb(SILK_GREY), lt.rgb(SILK),
                   np.clip(0.42 + 0.3 * crown + 0.15 * lt.noise(shape, seed + 14, 30.0), 0.0, 1.0))
    grime = np.clip(0.28 + 0.35 * lt.noise(shape, seed + 15, 24.0), 0.0, 0.7) * (0.5 + 0.5 * dens)
    color = lt.mix(color, lt.rgb(SILK_DUSTY), grime)
    plate.layer(a, color, 0.8, (0.0004 + 0.0006 * crown) * a)
    return a


def knots(plate, rnd, lines, count, color=SILK_GREY, size=(3.0, 6.0)):
    """Little matted knots of silk where strands cross (picked along the given strands)."""
    for _ in range(count if lines else 0):
        line = lines[rnd.randrange(len(lines))]
        q = line[rnd.randrange(len(line))]
        clump(plate, rnd, q, rnd.uniform(*size), tone(rnd, color), count=5, radius=0.5)


def film(plate, rnd, centers, size=(8.0, 22.0), color=SILK_GREY):
    """Small patches of matted, dusty film where the gauze gathers (at its anchors and along its strands): ragged,
    fibrous, mostly covered, so the sheet has a few denser spots instead of reading as loose hay."""
    h, w = plate.h, plate.w
    for cx, cy in centers:
        rx, ry = rnd.uniform(*size), rnd.uniform(*size) * 0.55
        turn = rnd.uniform(0.0, math.pi)
        x0, x1 = int(max(cx - rx - 4.0, 0)), int(min(cx + rx + 4.0, w))
        y0, y1 = int(max(cy - rx - 4.0, 0)), int(min(cy + rx + 4.0, h))
        if x0 >= x1 or y0 >= y1:
            continue
        xs = np.arange(x0, x1, dtype=np.float32)[None, :] + 0.5 - cx
        ys = np.arange(y0, y1, dtype=np.float32)[:, None] + 0.5 - cy
        u = xs * math.cos(turn) + ys * math.sin(turn)
        v = -xs * math.sin(turn) + ys * math.cos(turn)
        e = np.sqrt((u / rx) ** 2 + (v / ry) ** 2)
        seed = rnd.randrange(1 << 30)
        grain = np.random.default_rng(seed).standard_normal(e.shape).astype(np.float32)
        a = lt.smooth(0.15, -0.15, e - 1.0 + 0.35 * grain) * lt.smooth(0.0, 0.6, 1.0 - e)
        plate.layer(a, tone(rnd, color) * (0.94 + 0.08 * np.clip(grain, -1.0, 1.0))[..., None], 0.72, 0.0008 * a,
                    (slice(y0, y1), slice(x0, x1)))


def anchor_fans(rnd, x_start, x_end, edge_y, sheet_y, gap=(130.0, 300.0), reach=70.0):
    """Anchor strands from a sheet out to the line y = edge_y, where they're glued down: a few anchors, spaced
    unevenly, each taking one to three long strands that start well inside the sheet (sheet_y(x, depth) gives their
    row), so the gauze gathers into them; a few doubled, a few snapped. Returns [dict(anchor, threads, broken)]."""
    fans = []
    x = x_start + rnd.uniform(0.0, 60.0)
    while x < x_end:
        anchor = (x, edge_y)
        threads, broken = [], []
        count = rnd.choice((1, 1, 2, 2, 2, 3))
        for _ in range(count):
            lx = x + (rnd.uniform(-reach, reach) if count > 1 else rnd.uniform(-reach * 0.4, reach * 0.4))
            start = (lx, sheet_y(lx, rnd.uniform(0.25, 0.7)))
            if rnd.random() < 0.08:
                # Snapped: the sheet's end hangs, curling down.
                d = (anchor[0] - start[0], anchor[1] - start[1])
                broken.append(hanging(start, d, math.hypot(*d) * rnd.uniform(0.3, 0.55), rnd.uniform(6.0, 16.0), 4))
                continue
            threads.append([start, anchor])
            if rnd.random() < 0.15:
                side = rnd.choice((-1.0, 1.0)) * rnd.uniform(3.0, 6.0)
                threads.append([(start[0] + side, start[1]), anchor])
        fans.append(dict(anchor=anchor, threads=threads, broken=broken))
        x += rnd.uniform(*gap)
    return fans


# --- The orb web ---

ORB_CENTER = (384.0, 384.0)
# Frame corners (degrees counterclockwise from +x with y up, distance px) and where each one's anchor line meets
# the cell's edge.
ORB_CORNERS = ((8.0, 322.0), (57.0, 300.0), (103.0, 336.0), (151.0, 304.0), (197.0, 326.0), (246.0, 288.0),
               (298.0, 334.0))
ORB_ANCHORS = ((764.0, 296.0), (640.0, 4.0), (296.0, 4.0), (4.0, 168.0), (4.0, 484.0), (226.0, 764.0),
               (604.0, 764.0))
ORB_TORN = (206.0, 246.0)        # the wedge torn out of it (degrees)


def _polar(deg, r):
    a = math.radians(deg)
    return (ORB_CENTER[0] + r * math.cos(a), ORB_CENTER[1] - r * math.sin(a))


def orb_layout():
    """The orb web's threads in the Orb cell's pixels: center (the hub), corners (the frame's), frame (the frame cords
    between corners, bowed in by the radials' pull), anchors (corner to the cell's edge), radials, spiral, hanging
    (broken threads), hub (the mat of fibres and its rings) and bundles (wrapped prey). Pure arithmetic from a fixed
    seed: the painter draws it and Sink.py's card follows its corners and anchors."""
    rnd = random.Random(4021)
    cx, cy = ORB_CENTER
    corners = [_polar(a, r) for a, r in ORB_CORNERS]
    frame = []
    for i, a in enumerate(corners):
        b = corners[(i + 1) % len(corners)]
        mx, my = (a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5
        tx, ty = cx - mx, cy - my
        tl = math.hypot(tx, ty)
        bow = 0.08 * math.hypot(b[0] - a[0], b[1] - a[1])
        frame.append(bezier(a, (mx + tx / tl * bow, my + ty / tl * bow), b, 18))
    anchors = [sagging(corner, end, 0.015, 8) for corner, end in zip(corners, ORB_ANCHORS)]

    def reach(deg):
        a = math.radians(deg)
        return ray_hit((cx, cy), (math.cos(a), -math.sin(a)), frame)

    radials, hang, angles, reaches = [], [], [], []
    for k in range(30):
        deg = 4.0 + k * 12.0 + rnd.uniform(-3.0, 3.0)
        r = reach(deg)
        angles.append(deg)
        reaches.append(r)
        a = math.radians(deg)
        d = (math.cos(a), -math.sin(a))
        if ORB_TORN[0] <= deg <= ORB_TORN[1]:
            # Broken: a stub stays on the frame; the inner piece hangs from the hub, curling down.
            stub = rnd.uniform(26.0, 44.0)
            hang.append(hanging((cx + d[0] * r, cy + d[1] * r), (-d[0], -d[1]), stub, rnd.uniform(4.0, 9.0), 3))
            inner = r * rnd.uniform(0.3, 0.5)
            hang.append(hanging((cx + d[0] * 14.0, cy + d[1] * 14.0), d, inner, inner * rnd.uniform(0.45, 0.8), 8))
        else:
            radials.append([(cx + d[0] * 14.0, cy + d[1] * 14.0), (cx + d[0] * (r - 1.0), cy + d[1] * (r - 1.0))])

    def torn(k):
        return ORB_TORN[0] <= angles[k] <= ORB_TORN[1]

    # The capture spiral: one point per radial per turn, from 62 px out by 18.5 px a turn, stopping short of the frame.
    points = []
    for turn in range(22):
        for k in range(30):
            r = 62.0 + 18.5 * (turn + (angles[k] - angles[0]) / 360.0) + rnd.uniform(-1.6, 1.6)
            points.append((k, r, r < 0.9 * reaches[k]))
    spiral = []
    for (k0, r0, ok0), (k1, r1, ok1) in zip(points, points[1:]):
        if not (ok0 and ok1):
            continue
        p0, p1 = _polar(angles[k0], r0), _polar(angles[k1], r1)
        if torn(k0) != torn(k1):
            # The edge of the torn wedge: the spiral's loose end hangs off the last whole radial.
            start, toward = (p0, p1) if not torn(k0) else (p1, p0)
            hang.append(hanging(start, (toward[0] - start[0], toward[1] - start[1]), rnd.uniform(8.0, 18.0),
                                rnd.uniform(10.0, 26.0), 4))
            continue
        if torn(k0):
            continue
        roll = rnd.random()
        if roll < 0.025:
            continue                                            # a gap
        if roll < 0.045:
            mid = ((p0[0] + p1[0]) * 0.5, (p0[1] + p1[1]) * 0.5)
            for a, b in ((p0, mid), (p1, mid)):                 # snapped in the middle, both ends hanging
                hang.append(hanging(a, (b[0] - a[0], b[1] - a[1]), math.hypot(b[0] - a[0], b[1] - a[1]) * 0.8,
                                    rnd.uniform(5.0, 12.0), 3))
            continue
        # Most spiral threads are taut; a few have gone slack and sag between their radials.
        spiral.append(sagging(p0, p1, rnd.uniform(0.04, 0.1), 4) if roll > 0.9 else [p0, p1])

    hub = []
    for _ in range(34):
        a0 = rnd.uniform(0.0, 2.0 * math.pi)
        a1 = a0 + rnd.uniform(1.2, 3.1)
        r0, r1 = rnd.uniform(9.0, 21.0), rnd.uniform(9.0, 21.0)
        hub.append([(cx + r0 * math.cos(a0), cy - r0 * math.sin(a0)), (cx + r1 * math.cos(a1), cy - r1 * math.sin(a1))])
    rings = []
    for radius in (27.0, 37.0):
        ring = []
        for i in range(21):
            a = 2.0 * math.pi * i / 20.0
            rr = radius + (rnd.uniform(-2.5, 2.5) if i < 20 else 0.0)
            ring.append((cx + rr * math.cos(a), cy - rr * math.sin(a)))
        ring[-1] = ring[0]
        rings.append(ring)
    bundles = [dict(at=_polar(36.0, 210.0), size=(30.0, 15.0), turn=62.0),
               dict(at=_polar(137.0, 146.0), size=(16.0, 9.0), turn=-48.0)]
    return dict(center=ORB_CENTER, corners=corners, frame=frame, anchors=anchors, radials=radials, spiral=spiral,
                hanging=hang, hub=hub, rings=rings, bundles=bundles, angles=angles, reaches=reaches)


def paint_orb():
    lay = orb_layout()
    p = Plate(768, 768)
    rnd = random.Random(5101)
    for line in lay['anchors']:
        p.line(line, 3.6, tone(rnd, SILK_GREY), rough=0.62)
    for line in lay['frame']:
        p.line(line, 4.3, tone(rnd, SILK_GREY), rough=0.6)
    for line in lay['radials']:
        p.line(line, 2.5, tone(rnd, SILK), rough=0.5)
    for line in lay['spiral']:
        p.line(line, 1.75, tone(rnd, SILK_HI, 0.04), rough=0.45)
    for line in lay['hanging']:
        p.line(line, 1.7, tone(rnd, SILK), rough=0.5)
    for line in lay['rings']:
        p.line(line, 1.3, tone(rnd, SILK_HI), rough=0.5)
    for line in lay['hub']:
        p.line(line, 1.35, tone(rnd, SILK_HI), rough=0.5)
    for b in lay['bundles']:
        x, y = b['at']
        rx, ry = b['size']
        a = math.radians(b['turn'])
        for side in (-1.0, 1.0):     # the threads it was wrapped to
            tip = (x + side * rx * math.cos(a), y + side * rx * math.sin(a))
            p.line([tip, (tip[0] + side * 14.0 * math.cos(a + 0.5), tip[1] + side * 14.0 * math.sin(a + 0.5))], 1.2,
                   lt.rgb(SILK))
        p.blob(b['at'], b['size'], b['turn'], lt.rgb(BUNDLE), rough=0.6, lift=0.6, stripes=0.22, seed=rx)
    dust(p, rnd, lay['frame'] + lay['anchors'] + lay['radials'], 160)
    dust(p, rnd, lay['spiral'], 120, SILK_DUSTY)
    return p.finish()


# --- The sheet web lens (Hammock) ---

def _lens(xa, xb, peak, rnd, n=28, power=0.7):
    xs = [xa + (xb - xa) * i / n for i in range(n + 1)]
    half = [0.0 if i in (0, n) else peak * math.sin(math.pi * i / n) ** power * rnd.uniform(0.86, 1.08)
            for i in range(n + 1)]
    return xs, half


def hammock_layout():
    """The Hammock cell's sheet web, in its pixels: the lens (xs, half: its half-height at each x around row cy,
    tips at xa and xb) and fans (anchor_fans: threads from just inside the lens's edge out to anchors on the cell's
    top edge (side -1) or bottom edge (side +1))."""
    rnd = random.Random(4022)
    w, h, cy = 768, 384, 192.0
    xa, xb = 34.0, 734.0
    xs, half = _lens(xa, xb, 120.0, rnd)

    def sheet_y(side):
        return lambda x, depth: cy + side * float(np.interp(min(max(x, xa + 20.0), xb - 20.0), xs, half)) * depth
    fans = []
    for side in (-1, 1):
        for fan in anchor_fans(rnd, xa + 16.0, xb - 24.0, 4.0 if side < 0 else h - 4.0, sheet_y(side)):
            fan['side'] = side
            for thread in fan['threads']:
                thread[0] = (min(max(thread[0][0], 10.0), w - 10.0), thread[0][1])
            fans.append(fan)
    return dict(w=w, h=h, cy=cy, xa=xa, xb=xb, xs=xs, half=half, fans=fans)


def draw_fans(p, rnd, fans):
    """The anchor strands: each a fine strand bowed a little, a finer one or two gathered into it from beside its
    start (the gauze drawing together into the line), a dusty knot where it's glued."""
    lines = []
    for fan in fans:
        for thread in fan['threads']:
            (sx, sy), (ex, ey) = thread[0], thread[1]
            pts = sagging((sx, sy), (ex, ey), rnd.uniform(-0.03, 0.03), 7)
            p.line(pts, [0.95, 0.85, 0.8, 0.75, 0.75, 0.8, 0.85], tone(rnd, SILK_GREY), rough=0.62, lift=0.4)
            lines.append(pts)
            for _ in range(rnd.choice((1, 1, 2))):
                f = rnd.uniform(0.35, 0.7)
                join = (sx + (ex - sx) * f, sy + (ey - sy) * f)
                side = (sx + rnd.uniform(-6.5, 6.5), sy)
                p.line(sagging(side, join, rnd.uniform(-0.04, 0.04), 5), 0.55, tone(rnd, SILK), rough=0.62, lift=0.3)
        for line in fan['broken']:
            p.line(line, [0.9, 0.8, 0.7, 0.55, 0.4], tone(rnd, SILK_GREY), rough=0.62, lift=0.4)
        if fan['threads']:
            attachment(p, rnd, fan['anchor'], tone(rnd, SILK_DUSTY))
    return lines


def paint_hammock():
    lay = hammock_layout()
    w, h, cy = lay['w'], lay['h'], lay['cy']
    p = Plate(w, h)
    rnd = random.Random(5102)
    x, y = grid(w, h)
    halfw = np.interp(x, lay['xs'], lay['half']).astype(np.float32)
    ragged = 8.0 * streaks((h, w), 6201, 0.0, 14.0, 4.0) + 3.0 * lt.noise((h, w), 6202, 2.0)
    inside = halfw - np.abs(y - cy) + ragged
    # A mat of matted gauze over nearly three quarters of the lens, denser where the anchor strands gather it and
    # at its tips (where it's strung), thinning out to its edge.
    extra = np.zeros((h, w), np.float32)
    for fan in lay['fans']:
        for thread in fan['threads']:
            sx, sy = thread[0]
            extra = np.maximum(extra, 0.85 * np.exp(-((x - sx) / 30.0) ** 2 - ((y - sy) / 18.0) ** 2))
    for tip in (lay['xa'], lay['xb']):
        extra = np.maximum(extra, 0.8 * np.exp(-((x - tip) / 44.0) ** 2 - ((y - cy) / 24.0) ** 2))
    felt(p, inside, 0.9, 6210, 0.0, 30.0, extra)
    # A few long strands lying over it, single and dim (a plate of their own: gauze stops at the coverage it's asked
    # for, which the mat already has).
    over = Plate(w, h)
    lines = gauze(over, rnd, inside, 0.0, 0.035, length=(100.0, 240.0), radius=(0.34, 0.44), edge=20.0, cross=0.25,
                  colors=(SILK_GREY, SILK), fluff=0.0, wisps=0.0)
    p.over(over)
    knots(p, rnd, lines, 8)
    starts = [thread[0] for fan in lay['fans'] for thread in fan['threads']]
    spots = []
    for _ in range(14):                         # matted into film here and there
        sx = rnd.uniform(lay['xa'] + 60.0, lay['xb'] - 60.0)
        spots.append((sx, cy + rnd.uniform(-0.6, 0.6) * float(np.interp(sx, lay['xs'], lay['half']))))
    film(p, rnd, starts[::2] + [(lay['xa'] + 20.0, cy), (lay['xb'] - 20.0, cy)] + spots, size=(10.0, 26.0))
    lines += draw_fans(p, rnd, lay['fans'])
    for tip, out in ((lay['xa'], -1.0), (lay['xb'], 1.0)):        # the tips taper into a few strands
        for _ in range(4):
            start = (tip + out * rnd.uniform(-14.0, 6.0), cy + rnd.uniform(-10.0, 10.0))
            p.line(hanging(start, (out, rnd.uniform(-0.5, 0.5)), rnd.uniform(16.0, 30.0), rnd.uniform(2.0, 9.0), 4),
                   [0.8, 0.7, 0.6, 0.45, 0.3], tone(rnd, SILK), rough=0.62, lift=0.3)
    dust(p, rnd, lines, 160)
    p.blob((470.0, 205.0), (6.0, 3.5), 30.0, lt.rgb(LEAF), rough=0.75, lift=0.3)     # dry leaves caught in it
    p.blob((262.0, 160.0), (4.0, 2.5), -50.0, lt.rgb(LEAF) * 0.85, rough=0.75, lift=0.3)
    return p.finish()


# --- The cobweb tangle ---

def tangle_layout():
    """The Tangle cell's cobweb, in its pixels: knots (rim: where it's strung to its supports; inner: where threads
    meet), threads strung between them (polylines, each with a radius), loose (snapped threads hanging), clumps of
    silk at some inner knots, and hull, a convex outline round everything (8 px out) for the card."""
    rnd = random.Random(4023)
    cx, cy = 384.0, 192.0
    phase = (rnd.uniform(0.0, 6.28), rnd.uniform(0.0, 6.28))

    def edge(t, k=1.0):
        r = 1.0 + 0.12 * math.sin(3.0 * t + phase[0]) + 0.06 * math.sin(5.0 * t + phase[1])
        return (cx + 332.0 * r * k * math.cos(t), cy + 160.0 * r * k * math.sin(t))

    def clamp(q):
        return (min(max(q[0], 12.0), 756.0), min(max(q[1], 12.0), 372.0))
    rim = [clamp(edge(2.0 * math.pi * i / 12.0 + rnd.uniform(-0.12, 0.12))) for i in range(12)]
    inner = []
    while len(inner) < 24:
        q = clamp(edge(rnd.uniform(0.0, 2.0 * math.pi), rnd.random() ** 0.8 * 0.84))
        if all(math.hypot(q[0] - o[0], q[1] - o[1]) > 30.0 for o in inner):
            inner.append(q)
    knots = rim + inner
    threads, pairs = [], set()
    for i, a in enumerate(knots):
        near = sorted(range(len(knots)), key=lambda j: (math.hypot(knots[j][0] - a[0], knots[j][1] - a[1]), j))
        for j in near[1:1 + (rnd.choice((2, 3, 3, 4)) if i >= 12 else rnd.choice((2, 2, 3)))]:
            key = (min(i, j), max(i, j))
            if (i < 12 and j < 12) or key in pairs:
                continue                      # no rim-to-rim lines: the outline stays loose, not a polygon
            pairs.add(key)
            slack = rnd.uniform(0.06, 0.13) if rnd.random() < 0.18 else rnd.uniform(0.025, 0.06)
            threads.append((sagging(a, knots[j], slack, 7), rnd.uniform(0.8, 1.5)))
    for _ in range(10):                       # a few long lines across, sagging a little
        i, j = rnd.randrange(12, len(knots)), rnd.randrange(len(knots))
        if i != j:
            threads.append((sagging(knots[i], knots[j], rnd.uniform(0.02, 0.05), 9), rnd.uniform(0.7, 1.1)))
    loose = []
    for _ in range(16):
        k = inner[rnd.randrange(len(inner))]
        a = rnd.uniform(0.0, 2.0 * math.pi)
        loose.append(hanging(k, (math.cos(a), math.sin(a)), rnd.uniform(24.0, 70.0), rnd.uniform(8.0, 30.0), 5))
    loose = [[clamp(q) for q in line] for line in loose]
    clumps = [k for k in inner if rnd.random() < 0.45]
    points = [q for line, _ in threads for q in line] + [q for line in loose for q in line]
    hull = _grow(_convex_hull(points), 8.0)
    return dict(rim=rim, inner=inner, knots=knots, threads=threads, loose=loose, clumps=clumps, hull=hull,
                center=(cx, cy))


def _convex_hull(points):
    pts = sorted(set((round(x, 3), round(y, 3)) for x, y in points))

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for q in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], q) <= 0:
            lower.pop()
        lower.append(q)
    for q in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], q) <= 0:
            upper.pop()
        upper.append(q)
    return lower[:-1] + upper[:-1]


def _grow(poly, by):
    """Moves every corner of a convex polygon out from its centroid by `by` px."""
    gx = sum(q[0] for q in poly) / len(poly)
    gy = sum(q[1] for q in poly) / len(poly)
    out = []
    for x, y in poly:
        d = math.hypot(x - gx, y - gy) or 1.0
        out.append((x + (x - gx) / d * by, y + (y - gy) / d * by))
    return out


def paint_tangle():
    lay = tangle_layout()
    w, h = 768, 384
    p = Plate(w, h)
    rnd = random.Random(5103)
    # Gauze where the threads crowd together (what holds the lantern), denser in the middle, and wisps of it at three
    # knots nearby: the tangle reads as a web, not a few sticks, once the fine threads fade with distance.
    x, y = grid(w, h)
    cx, cy = lay['center']
    ragged = 9.0 * streaks((h, w), 6301, 0.15, 14.0, 4.0) + 3.0 * lt.noise((h, w), 6302, 2.0)
    q = np.sqrt(((x - cx) / 150.0) ** 2 + ((y - cy - 6.0) / 62.0) ** 2)
    inside = (1.0 - q) * 62.0
    near = sorted(lay['inner'], key=lambda k: (math.hypot(k[0] - cx, k[1] - cy), k))[2:5]
    for kx, ky in near:
        qk = np.sqrt(((x - kx) / 48.0) ** 2 + ((y - ky) / 30.0) ** 2)
        inside = np.maximum(inside, (1.0 - qk) * 30.0)
    core = 0.5 * np.exp(-q * q * 2.5)
    lines = gauze(p, rnd, inside + ragged, 0.15, 0.3, core, length=(30.0, 120.0), edge=18.0)
    knots(p, rnd, lines, 8)
    film(p, rnd, [(cx + rnd.uniform(-40.0, 40.0), cy + rnd.uniform(-12.0, 12.0)) for _ in range(3)] + near[:2])
    # Fine threads strung between the knots, sagging; snapped ones hanging, tapering to nothing.
    for line, radius in lay['threads']:
        p.line(line, radius * 0.62, tone(rnd, SILK_HI if radius < 1.2 else SILK, 0.04), rough=0.62, lift=0.4,
               shade=0.06)
        lines.append(line)
    for line in lay['loose']:
        p.line(line, [0.8, 0.7, 0.6, 0.5, 0.4, 0.3], tone(rnd, SILK), rough=0.62, lift=0.3)
    for k in lay['inner']:
        p.blob(k, (rnd.uniform(1.6, 2.4), rnd.uniform(1.2, 1.8)), rnd.uniform(0.0, 180.0), tone(rnd, SILK_GREY), lift=0.3)
    for k in lay['clumps']:                   # fluff caught where threads meet
        clump(p, rnd, k, rnd.uniform(6.0, 10.0), tone(rnd, SILK_GREY), count=10, radius=0.6)
    for k in lay['rim']:
        attachment(p, rnd, k, tone(rnd, SILK_DUSTY))
    for _ in range(4):                        # dust balls
        k = lay['inner'][rnd.randrange(len(lay['inner']))]
        clump(p, rnd, (k[0] + rnd.uniform(-6.0, 6.0), k[1] + rnd.uniform(-6.0, 6.0)), rnd.uniform(5.0, 8.0),
              tone(rnd, SILK_DUSTY) * 0.85, count=8, radius=0.6)
    p.blob((300.0, 214.0), (9.0, 5.0), 64.0, lt.rgb(LEAF), rough=0.75, lift=0.3)
    p.blob((470.0, 150.0), (6.0, 3.5), -20.0, lt.rgb(LEAF) * 0.9, rough=0.75, lift=0.3)
    dust(p, rnd, lines, 90)
    return p.finish()


# --- Tatters ---

TATTER_SPECS = ((54.0, 42.0, 738.0), (152.0, 30.0, 612.0), (252.0, 40.0, 540.0), (352.0, 27.0, 696.0),
                (452.0, 37.0, 470.0))         # x of the middle, widest half-width, length (px)


def tatter_layout():
    """The Tatters cell's five torn strips, in its pixels. Each: ys (rows, every 12 px from the top at 10), centre and
    half (the strip's middle and half-width there, before the painter's ragged edge, which only eats into it), split
    (where a slit opens it into two tails, or None), top (anchor threads to the cell's top edge) and loose (threads
    hanging below its tip)."""
    rnd = random.Random(4024)
    out = []
    for xc, width, length in TATTER_SPECS:
        ys = [float(y) for y in range(10, int(length) + 1, 12)]
        ph = (rnd.uniform(0.0, 6.28), rnd.uniform(0.0, 6.28))
        centre, half = [], []
        for y in ys:
            s = (y - 10.0) / (length - 10.0)
            centre.append(xc + 6.0 * math.sin(y / 95.0 + ph[0]) + 2.5 * math.sin(y / 31.0 + ph[1]))
            profile = (0.5 + 0.5 * min(1.0, s / 0.14)) * (1.0 - 0.8 * s ** 1.5)
            half.append(width * profile * rnd.uniform(0.9, 1.08))
        split = rnd.uniform(0.55, 0.75) * length if rnd.random() < 0.6 else None
        top = []
        for k in range(rnd.choice((2, 3))):
            x0 = centre[0] + rnd.uniform(-0.6, 0.6) * half[0]
            top.append([(x0, 14.0), (x0 + rnd.uniform(-26.0, 26.0), 4.0)])
        loose = []
        for k in range(rnd.choice((2, 3, 4))):
            x0 = centre[-1] + rnd.uniform(-1.0, 1.0) * max(half[-1], 4.0)
            drop = min(rnd.uniform(24.0, 70.0), 764.0 - length)
            loose.append(hanging((x0, length - 4.0), (rnd.uniform(-0.25, 0.25), 1.0), drop, 0.0, 4))
        out.append(dict(ys=ys, centre=centre, half=half, split=split, top=top, loose=loose, length=length,
                        x0=xc - 50.0, x1=xc + 50.0))
    return out


def paint_tatters():
    """Each tatter: gauze hanging from its anchor threads, densest where it's glued at the top and about a third
    covered below, coming apart lower down into wavy strands that taper to nothing, fluff and grit caught on them."""
    p = Plate(512, 768)
    rnd = random.Random(5104)
    x, y = grid(512, 768)
    lines = []
    for i, t in enumerate(tatter_layout()):
        ys = np.array(t['ys'], np.float32)
        centre = np.interp(y, ys, t['centre']).astype(np.float32)
        half = np.interp(y, ys, t['half']).astype(np.float32)
        ragged = 4.0 * lt.noise((768, 512), 6300 + i, 6.0, 3.0) + 2.0 * lt.noise((768, 512), 6310 + i, 1.6)
        inside = half - np.abs(x - centre) + ragged - 2.0
        inside = np.where((y < 8.0) | (y > t['length']), -99.0, inside)
        if t['split'] is not None:
            slit = np.clip((y - t['split']) / 110.0, 0.0, 1.0) * half * 0.42
            inside = np.where(y > t['split'], np.minimum(inside, np.abs(x - centre - 3.0) - slit), inside)
        s = np.clip((y - 10.0) / max(t['length'] - 10.0, 1.0), 0.0, 1.0)
        coverage = 0.34 * (1.0 - lt.smooth(0.45, 0.9, s)) + 0.04
        glued = 0.55 * (1.0 - lt.smooth(0.0, 0.1, s))
        lines += gauze(p, rnd, inside, 0.5 * math.pi, coverage, glued, length=(40.0, 150.0),
                       edge=10.0, cross=0.25)
        # Strands running down it, wandering, tapering; most stop inside it, some run on below it.
        for k in range(rnd.choice((7, 8, 9))):
            off = rnd.uniform(-0.85, 0.85)
            stop = t['length'] * rnd.uniform(0.45, 1.0)
            amp, wave, ph = rnd.uniform(1.5, 4.5), rnd.uniform(70.0, 170.0), rnd.uniform(0.0, 6.28)
            yy = np.linspace(12.0, stop, 16)
            pts = [(float(np.interp(v, t['ys'], t['centre'])) + off * float(np.interp(v, t['ys'], t['half'])) * 0.8 +
                    amp * math.sin(v / wave * 2.0 * math.pi + ph), float(v)) for v in yy]
            r = rnd.uniform(0.75, 1.05)
            p.line(pts, [r * (1.0 - 0.7 * (j / 15.0) ** 1.4) for j in range(16)], tone(rnd, SILK_HI), rough=0.62,
                   lift=0.4)
            lines.append(pts)
        film(p, rnd, [(float(t["centre"][1]), 22.0)], size=(8.0, 14.0))
        for line in t['top']:                 # glued up at the top: fine anchor threads, a dusty knot
            p.line(line, [0.9, 0.7], tone(rnd, SILK_GREY), rough=0.62, lift=0.3)
            attachment(p, rnd, line[-1], tone(rnd, SILK_DUSTY))
        for line in t['loose']:
            p.line(line, [0.75, 0.65, 0.5, 0.38, 0.25], tone(rnd, SILK), rough=0.62, lift=0.3)
            lines.append(line)
        for _ in range(rnd.choice((2, 3))):  # fluff caught on the strands
            line = lines[-1 - rnd.randrange(min(len(lines), 6))]
            q = line[rnd.randrange(1, len(line))]
            clump(p, rnd, q, rnd.uniform(5.0, 8.0), tone(rnd, SILK_GREY), count=8, radius=0.55)
    dust(p, rnd, lines, 120)
    return p.finish()


# --- The sheet band ---

def paint_sheet():
    w, h = 2048, 384
    p = Plate(w, h, wrap_x=True)
    rnd = random.Random(5105)
    x, y = grid(w, h)
    cy = 192.0 + 10.0 * periodic(w, 6401, 90.0)
    hw = 104.0 + 14.0 * periodic(w, 6402, 40.0)
    ragged = 8.0 * streaks((h, w), 6403, 0.0, 14.0, 4.0) + 3.0 * lt.noise((h, w), 6404, 2.0)
    inside = hw[None, :] - np.abs(y - cy[None, :]) + ragged
    # A few long strands running along the band (tiling: whole cycles), the mat gathered a little along them.
    along = []
    for k in range(5):
        off = rnd.uniform(-0.7, 0.7)
        cyc = rnd.choice((1, 2, 3))
        ph = rnd.uniform(0.0, 6.28)
        pts = []
        for i in range(0, 129):
            xx = w * i / 128.0
            j = int(xx) % w
            pts.append((xx, float(cy[j] + off * hw[j] + 6.0 * math.sin(2.0 * math.pi * cyc * xx / w + ph))))
        along.append(pts)
    extra = np.zeros((h, w), np.float32)
    for pts in along:
        yy = np.interp(x[0], [q[0] for q in pts], [q[1] for q in pts]).astype(np.float32)
        extra = np.maximum(extra, 0.8 * np.exp(-((y - yy[None, :]) / 10.0) ** 2))
    felt(p, inside, 0.84, 6410, 0.0, 30.0, extra)
    # A few long strands over the mat, on a plate of their own (see paint_hammock), the ones it's gathered along last.
    over = Plate(w, h, wrap_x=True)
    lines = gauze(over, rnd, inside, 0.0, 0.035, length=(100.0, 260.0), radius=(0.34, 0.44), edge=20.0, cross=0.25,
                  colors=(SILK_GREY, SILK), fluff=0.0, wisps=0.0)
    for pts in along:
        over.line(pts, rnd.uniform(0.6, 0.8), tone(rnd, SILK), rough=0.62, lift=0.4)
    p.over(over)
    lines += along
    knots(p, rnd, lines, 20)
    film(p, rnd, [along[rnd.randrange(len(along))][rnd.randrange(8, 121)] for _ in range(14)] +
         [(rnd.uniform(60.0, w - 60.0), 192.0 + rnd.uniform(-70.0, 70.0)) for _ in range(14)], size=(10.0, 26.0))
    for side in (-1, 1):                       # anchor strands out to both edges, all the way round

        def sheet_y(xx, depth, side=side):
            j = int(xx) % w
            return float(cy[j]) + side * float(hw[j]) * depth
        lines += draw_fans(p, rnd, anchor_fans(rnd, 0.0, w - 30.0, 4.0 if side < 0 else h - 4.0, sheet_y))
    dust(p, rnd, lines, 600)
    for _ in range(4):                         # dry leaves caught in it
        p.blob((rnd.uniform(0.0, w), 192.0 + rnd.uniform(-60.0, 60.0)), (rnd.uniform(4.0, 7.0), rnd.uniform(2.5, 4.0)),
               rnd.uniform(0.0, 180.0), lt.rgb(LEAF) * rnd.uniform(0.8, 1.05), rough=0.75, lift=0.3)
    return p.finish()


# --- The torn fringe ---

def paint_fringe():
    """The torn edge of a hatched sac's silk: the sac's wound silk solid along the bottom (rows 102-128 everywhere),
    torn up into uneven tongues (up to 80 px) with deep notches between them, holes torn near the edge, fibres pulled
    out past it and a few strands left across the notches."""
    w, h = 2048, 128
    p = Plate(w, h, wrap_x=True)
    rnd = random.Random(5106)
    x, y = grid(w, h)
    xs = np.arange(w, dtype=np.float32) + 0.5
    e = 102.0 + 3.0 * periodic(w, 6501, 24.0) + 1.5 * periodic(w, 6502, 4.0)
    tongues = np.zeros(w, np.float32)
    for _ in range(44):
        x0, width, height = rnd.uniform(0.0, w), rnd.uniform(26.0, 84.0), rnd.uniform(20.0, 80.0)
        d = np.abs((xs - x0 + w * 0.5) % w - w * 0.5)
        shape = np.clip(1.0 - d / (width * 0.5), 0.0, 1.0) ** rnd.uniform(0.55, 1.1)
        tongues = np.maximum(tongues, height * shape)
    e = e - tongues
    # px inside the silk from its torn edge, the edge chewed at every scale.
    edge = y - e[None, :] + 4.0 * lt.noise((h, w), 6503, 3.0) + 1.5 * lt.noise((h, w), 6504, 1.0)
    near = 1.0 - lt.smooth(4.0, 30.0, edge)          # 1 at the edge, 0 deep in the silk
    holes = lt.smooth(-0.15, 0.15, lt.noise((h, w), 6505, 4.0) - (1.7 - 1.0 * near))
    solid = lt.smooth(-1.0, 1.0, edge) * (1.0 - holes * (1.0 - lt.smooth(104.0, 118.0, y)))   # the base stays whole
    # Coloured like the Sac cell: two families of wound bands, fine fibres, mottled; lighter where it frays.
    bands = 0.55 * streaks((h, w), 6506, 0.36, 170.0, 6.5) + 0.55 * streaks((h, w), 6507, -0.34, 170.0, 6.5)
    fibres = 0.5 * streaks((h, w), 6508, 0.36, 90.0, 0.8) + 0.5 * streaks((h, w), 6509, -0.34, 90.0, 0.8)
    mottle = lt.noise((h, w), 6510, 46.0)
    color = lt.mix(lt.rgb(SAC_SHADE), lt.rgb(SAC_SILK), np.clip(0.76 + 0.08 * mottle + 0.13 * bands + 0.07 * fibres,
                                                                0.0, 1.0))
    color = lt.mix(color, lt.rgb(SILK_HI), 0.18 * near)
    p.layer(solid, color, 0.85, (0.0016 * bands + 0.0003 * fibres + 0.0006 * near + 0.002) * solid)
    for _ in range(560):                       # fibres pulled out of the tear, every which way, some long and stringy
        x0 = rnd.uniform(0.0, w)
        y0 = float(e[int(x0) % w]) + 2.0
        long = rnd.random() < 0.14
        length = rnd.uniform(40.0, 80.0) if long else rnd.uniform(6.0, 28.0)
        a = -0.5 * math.pi + rnd.uniform(-1.2, 1.2)
        curl = rnd.uniform(-1.4, 1.4)
        pts = [(x0 + math.cos(a + curl * t) * length * t, max(y0 + math.sin(a + curl * t) * length * t, 5.0))
               for t in (0.0, 0.3, 0.6, 0.8, 1.0)]
        r = rnd.uniform(0.6, 0.9) if long else rnd.uniform(0.5, 0.85)
        p.line(pts, [r * 1.2, r, r * 0.8, r * 0.55, r * 0.3], tone(rnd, SAC_SILK if rnd.random() < 0.6 else SILK),
               rough=0.8, lift=0.4)
    for _ in range(34):                        # strands left across the notches
        x0 = rnd.uniform(0.0, w)
        span = rnd.uniform(50.0, 170.0)
        y_a = float(e[int(x0) % w]) + 3.0
        y_b = float(e[int(x0 + span) % w]) + 3.0
        pts = sagging((x0, max(y_a, 8.0)), (x0 + span, max(y_b, 8.0)), rnd.uniform(0.03, 0.1), 8)
        p.line(pts, rnd.uniform(0.55, 0.85), tone(rnd, SILK), rough=0.8, lift=0.4)
    return p.finish()


# --- Strands ---

def paint_strands():
    """Four lanes of 32 px, a thread along each one's middle tiling along U: fine (about 1 cm), wavy, with a little
    fluff and grit caught on it. A strip maps only the rows its thread fills (Sink.py's LANE_FILL), so it's drawn no
    wider than its thread."""
    w, h = 2048, 128
    p = Plate(w, h, wrap_x=True)
    rnd = random.Random(5107)
    n = 513

    def along(yc, amp, cycles, ph, amp2=0.0, cycles2=0, ph2=0.0):
        """A wavy line along the lane (whole cycles, so it tiles)."""
        return [(w * i / (n - 1), yc + amp * math.sin(2.0 * math.pi * cycles * i / (n - 1) + ph) +
                 amp2 * math.sin(2.0 * math.pi * cycles2 * i / (n - 1) + ph2)) for i in range(n)]

    def radii(r, var, cycles):
        return [r * (1.0 + var * math.sin(2.0 * math.pi * cycles * i / (n - 1))) for i in range(n)]
    lines = []
    # Cord: one thread, wandering a little.
    yc = sum(LANES['Cord']) * 0.5
    pts = along(yc, 2.0, 5, 0.4, 0.7, 37, 1.3)
    p.line(pts, radii(1.7, 0.18, 9), lt.rgb(SILK), rough=0.7, lift=0.6, shade=0.18)
    lines.append(pts)
    # Rope: two finer threads twisted round each other.
    yc = sum(LANES['Rope']) * 0.5
    for ph in (0.0, math.pi):
        pts = along(yc, 2.2, 44, ph, 0.8, 5, 0.7)
        p.line(pts, radii(1.15, 0.12, 13), tone(rnd, SILK), rough=0.7, lift=0.6, shade=0.2)
        lines.append(pts)
    # Tufted: a thread with fluff and grit caught on it.
    yc = sum(LANES['Tufted']) * 0.5
    pts = along(yc, 1.6, 6, 2.0, 0.6, 29, 0.2)
    p.line(pts, radii(1.1, 0.15, 11), lt.rgb(SILK), rough=0.7)
    lines.append(pts)
    xx = 40.0
    while xx < w - 40.0:
        q = pts[int(xx / w * (n - 1))]
        clump(p, rnd, (xx, q[1] + rnd.uniform(-0.8, 0.8)), rnd.uniform(6.0, 10.0), tone(rnd, SILK_GREY), count=10,
              radius=0.6)
        xx += rnd.uniform(90.0, 260.0)
    # Ribbon: three or four fine strands drifting round each other, a loose bundle.
    yc = sum(LANES['Ribbon']) * 0.5
    for k in range(4):
        pts = along(yc + rnd.uniform(-2.0, 2.0), rnd.uniform(2.0, 4.5), rnd.choice((3, 4, 5, 7)), rnd.uniform(0.0, 6.28),
                    1.0, rnd.choice((17, 23, 31)), rnd.uniform(0.0, 6.28))
        p.line(pts, rnd.uniform(0.6, 0.85), tone(rnd, SILK), rough=0.7)
        lines.append(pts)
    # A little fluff on the Cord and the Rope too.
    for lane in ('Cord', 'Rope'):
        yc = sum(LANES[lane]) * 0.5
        xx = rnd.uniform(60.0, 200.0)
        while xx < w - 60.0:
            clump(p, rnd, (xx, yc + rnd.uniform(-1.5, 1.5)), rnd.uniform(4.0, 5.5), tone(rnd, SILK_GREY), count=7,
                  radius=0.55)
            xx += rnd.uniform(280.0, 560.0)
    dust(p, rnd, lines, 240)
    # Keep every lane inside its rows: a thread's antialiasing must not reach the next lane.
    for y0, y1 in LANES.values():
        p.cov[y0:y0 + 1] *= 0.0
        p.cov[y1 - 1:y1] *= 0.0
    return p.finish()


# --- The egg sacs' silk ---

def _pad_rows(maps, pad, period):
    """A band tiling every period rows, padded: pad rows above copy its last rows and pad rows below its first."""
    out = {}
    for key, a in maps.items():
        out[key] = np.concatenate([a[period - pad:period], a, a[:pad]], axis=0)
    return out


def paint_sac_tile():
    w, h = 2048, SAC_PERIOD
    p = Plate(w, h, wrap_x=True, wrap_y=True)
    rnd = random.Random(5108)
    mottle = lt.noise((h, w), 6701, 46.0)
    # Wound like a cocoon: two families of broad bands crossing at about 40 degrees, fine fibres along them.
    bands = 0.55 * streaks((h, w), 6702, 0.36, 170.0, 6.5) + 0.55 * streaks((h, w), 6703, -0.34, 170.0, 6.5)
    fibres = (0.5 * streaks((h, w), 6704, 0.36, 90.0, 0.8) + 0.5 * streaks((h, w), 6705, -0.34, 90.0, 0.8) +
              0.3 * streaks((h, w), 6706, 0.02, 110.0, 0.9))
    puck = lt.noise((h, w), 6707, 20.0)
    color = lt.mix(lt.rgb(SAC_SHADE), lt.rgb(SAC_SILK),
                   np.clip(0.76 + 0.08 * mottle + 0.13 * bands + 0.07 * fibres, 0.0, 1.0))
    # Fully matte (no porcelain highlight), the wound bands raised enough to read as wound silk, not sewn cloth.
    height = 0.0026 * bands + 0.0003 * fibres + 0.0012 * puck
    p.layer(np.ones((h, w), np.float32), color, 0.86 + 0.04 * mottle, height)
    for _ in range(1500):                      # fine silk laid over it
        x0, y0 = rnd.uniform(0.0, w), rnd.uniform(0.0, h)
        a = rnd.choice((0.36, -0.34, 0.36, -0.34, 0.02)) + rnd.gauss(0.0, 0.06)
        a += math.pi if rnd.random() < 0.5 else 0.0
        length = rnd.uniform(30.0, 150.0)
        pts = [(x0 + math.cos(a + 0.12 * t) * length * t, y0 + math.sin(a + 0.12 * t) * length * t)
               for t in (0.0, 0.5, 1.0)]
        p.line(pts, rnd.uniform(0.5, 0.75), tone(rnd, SILK_HI, 0.03), rough=0.82, lift=0.5, shade=0.05)
    for _ in range(50):                        # grit stuck in it
        p.speck((rnd.uniform(0.0, w), rnd.uniform(0.0, h)), rnd.uniform(0.7, 1.3), lt.rgb(DUST), rnd.uniform(0.15, 0.4))
    maps = p.finish(ao_radii=(3, 9), ao_strength=0.8, normal_strength=1.4)
    maps['alpha'] = np.ones_like(maps['alpha'])
    return _pad_rows(maps, SAC_PAD, SAC_PERIOD)


WRAP_SPLIT = 512            # the Wrap cell's halves: wound silk (x < 512), fuzz (x >= 512)


def paint_wrap_tile():
    """Two kinds of loose silk, each tiling along V every WRAP_PERIOD rows, the strands running along V (round what it
    wraps). Left half: silk wound round a branch, fine strands with bundles here and there, about 40% covered so the
    bark shows through. Right half: the egg sacs' fuzzy halo, short fibres every which way, about 8% covered."""
    w, h = 1024, WRAP_PERIOD
    p = Plate(w, h, wrap_x=True, wrap_y=True)
    rnd = random.Random(5109)
    x, _ = grid(w, h)
    lines = []
    for _ in range(7):                         # a few bundles of wound strands, wrapping round (whole turns)
        x0 = rnd.uniform(20.0, WRAP_SPLIT - 40.0)
        for _k in range(rnd.choice((3, 4, 5))):
            off = rnd.uniform(-5.0, 5.0)
            pts = [(x0 + off + 2.0 * math.sin(2.0 * math.pi * t * 3 + off), h * t) for t in (i / 12.0 for i in range(13))]
            p.line(pts, rnd.uniform(0.6, 0.9), tone(rnd, SILK), rough=0.7, lift=0.4)
            lines.append(pts)
    wound = np.where(x < WRAP_SPLIT - 12.0, 100.0, -100.0).astype(np.float32)
    lines += gauze(p, rnd, wound, 0.5 * math.pi, 0.4, length=(40.0, 160.0), radius=(0.42, 0.68), edge=1.0, cross=0.3)
    knots(p, rnd, lines, 14)
    fuzz = np.where(x >= WRAP_SPLIT + 12.0, 100.0, -100.0).astype(np.float32)
    lines += gauze(p, rnd, fuzz, 0.5 * math.pi, 0.08, length=(14.0, 60.0), radius=(0.3, 0.48), edge=1.0, cross=0.5,
                   wander=0.35, fluff=2.0)
    dust(p, rnd, lines, 140)
    return _pad_rows(p.finish(), WRAP_PAD, WRAP_PERIOD)


def paint_tent():
    """A tent web for Web_Crown, as one sprite: U (0-1024) from the crotch of a fork to the mass's free top end, V
    (0-256) from the trunk's edge to the limb's. Dense silk laid on in layers (about 85% covered, thin spots and a few
    holes), densest and dustiest along both glued edges and at the crotch, ivory and fibrous, its top end thinning
    into soft fibres. The threads out to the twigs are the model's own."""
    w, h = 1024, 256
    p = Plate(w, h)
    rnd = random.Random(5110)
    x, y = grid(w, h)
    reach = 900.0 + 40.0 * lt.noise((h, w), 6901, 14.0) + 30.0 * streaks((h, w), 6902, 0.0, 40.0, 4.0)
    edge = np.minimum(y, h - y) + 3.0 * lt.noise((h, w), 6903, 2.0)
    mass = (1.0 - lt.smooth(reach - 160.0, reach, x)) * lt.smooth(0.0, 8.0, edge)
    # Gaps between the layers: long and narrow, along it (round holes read as spots).
    holes = lt.smooth(-0.2, 0.2, streaks((h, w), 6904, 0.05, 36.0, 5.0) - 1.45) * lt.smooth(20.0, 50.0, edge)
    thin = 0.25 * lt.smooth(0.3, 1.0, lt.noise((h, w), 6905, 18.0))
    a = lt.smooth(0.35, 0.6, mass * (1.0 - 0.9 * holes) - thin + 0.2 * streaks((h, w), 6906, 0.05, 30.0, 1.2))
    fibres = streaks((h, w), 6907, 0.05, 60.0, 0.8)
    layers = streaks((h, w), 6908, 0.0, 140.0, 6.0)
    color = lt.mix(lt.rgb(SILK_GREY), lt.rgb(SILK_HI), np.clip(0.6 + 0.18 * fibres + 0.12 * layers, 0.0, 1.0))
    grime = np.clip(1.0 - lt.smooth(0.0, 34.0, edge) + 0.8 * (1.0 - lt.smooth(0.0, 140.0, x)), 0.0, 1.0)
    color = lt.mix(color, lt.rgb(SILK_DUSTY), 0.45 * grime)
    p.layer(a, color, 0.8, (0.0022 + 0.0008 * fibres + 0.0012 * layers) * a)
    p.occ *= 1.0 - 0.18 * grime
    lines = []
    for _ in range(700):                       # strands laid along it, lighter, some running past its end
        x0, y0 = rnd.uniform(0.0, w), rnd.uniform(4.0, h - 4.0)
        ang = rnd.gauss(0.0, 0.22) + (math.pi if rnd.random() < 0.5 else 0.0)
        length = rnd.uniform(40.0, 240.0)
        pts = sagging((x0, y0), (x0 + math.cos(ang) * length, y0 + math.sin(ang) * length), rnd.uniform(-0.04, 0.04), 5)
        pts = [(min(max(q[0], 1.0), w - 1.0), min(max(q[1], 1.0), h - 1.0)) for q in pts]
        p.line(pts, rnd.uniform(0.55, 0.9), tone(rnd, SILK_HI if rnd.random() < 0.6 else SILK), rough=0.75, lift=0.4)
        lines.append(pts)
    for _ in range(260):                       # soft fibres at its free end
        y0 = rnd.uniform(6.0, h - 6.0)
        x0 = float(reach[int(y0), w - 1]) - rnd.uniform(40.0, 150.0)
        length = rnd.uniform(20.0, 90.0)
        ang = rnd.gauss(0.0, 0.5)
        pts = [(min(x0 + math.cos(ang + 0.6 * t * t) * length * t, w - 2.0), y0 + math.sin(ang + 0.6 * t * t) * length * t)
               for t in (0.0, 0.35, 0.7, 1.0)]
        p.line(pts, [0.8, 0.65, 0.5, 0.3], tone(rnd, SILK), rough=0.75, lift=0.3)
    for _ in range(10):
        clump(p, rnd, (rnd.uniform(60.0, 820.0), rnd.uniform(30.0, h - 30.0)), rnd.uniform(6.0, 11.0),
              tone(rnd, SILK_GREY), count=10, radius=0.6)
    dust(p, rnd, lines, 260)
    for _ in range(3):                         # dry leaves caught in it
        p.blob((rnd.uniform(80.0, 800.0), rnd.uniform(40.0, 216.0)), (rnd.uniform(4.0, 7.0), rnd.uniform(2.5, 4.0)),
               rnd.uniform(0.0, 180.0), lt.rgb(LEAF) * rnd.uniform(0.8, 1.05), rough=0.75, lift=0.3)
    return p.finish()


# --- The atlas ---

PAINTERS = (('Orb', paint_orb), ('Hammock', paint_hammock), ('Tangle', paint_tangle), ('Tatters', paint_tatters),
            ('Sheet', paint_sheet), ('Fringe', paint_fringe), ('Strands', paint_strands), ('Sac', paint_sac_tile),
            ('Wrap', paint_wrap_tile), ('Tent', paint_tent))


def paint_atlas():
    """Every cell painted into the atlas: colour (straight, transparent texels filled), alpha, normal (-1..1),
    occlusion and roughness, all at 2048."""
    color = np.zeros((SIZE, SIZE, 3), np.float32)
    alpha = np.zeros((SIZE, SIZE), np.float32)
    normal = np.zeros((SIZE, SIZE, 3), np.float32)
    normal[..., 2] = 1.0
    ao = np.ones((SIZE, SIZE), np.float32)
    rough = np.full((SIZE, SIZE), 0.6, np.float32)
    for name, painter in PAINTERS:
        maps = painter()
        x0, y0, x1, y1 = REGIONS[name]
        where = (slice(y0, y1), slice(x0, x1))
        assert maps['alpha'].shape == (y1 - y0, x1 - x0), (name, maps['alpha'].shape)
        color[where] = maps['color']
        alpha[where] = maps['alpha']
        normal[where] = maps['normal']
        ao[where] = maps['ao']
        rough[where] = maps['rough']
        _log(f'{name} painted')
    solid = alpha > 0.02
    normal = np.where(solid[..., None], normal, np.array([0.0, 0.0, 1.0], np.float32))
    ao = np.where(solid, ao, 1.0)
    rough = np.where(solid, rough, 0.6)
    color = lt.fill_transparent(color, alpha, threshold=0.3)
    return dict(color=color, alpha=alpha, normal=normal, ao=ao, rough=rough)


def _half(a):
    """Box-filtered to half size."""
    h, w = a.shape[:2]
    return a.reshape(h // 2, 2, w // 2, 2, *a.shape[2:]).mean(axis=(1, 3))


def encode(maps):
    """The three files' pixels: BC (2048 RGBA), N (1024 RGB, DirectX) and ORM (1024 RGB), uint8."""
    bc = np.concatenate([lt.to8(maps['color']), lt.to8(maps['alpha'])[..., None]], axis=-1)
    n = _half(maps['normal'])
    n = n / np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)
    normal = lt.to8(n * 0.5 + 0.5)
    orm = lt.to8(np.stack([_half(maps['ao']), _half(maps['rough']), np.zeros((MAP_SIZE, MAP_SIZE), np.float32)],
                          axis=-1))
    return bc, normal, orm


def write(maps, folder):
    bc, normal, orm = encode(maps)
    lt.write_png(os.path.join(folder, f'T_{SET}_BC.png'), bc)
    lt.write_png(os.path.join(folder, f'T_{SET}_N.png'), normal)
    lt.write_png(os.path.join(folder, f'T_{SET}_ORM.png'), orm)
    _log(f'wrote {folder}')


def preview(maps):
    """Atlas.png: the colour over a dark checker (what the clip keeps at 1/3 shows), the cells outlined; Atlas_lit.png:
    lit by its normal map and occlusion from the upper left."""
    color, alpha = maps['color'], maps['alpha']
    x, y = grid(SIZE, SIZE)
    checker = np.where(((x // 32).astype(int) + (y // 32).astype(int)) % 2 == 0, 0.16, 0.2)[..., None]
    shown = np.where((alpha > 1.0 / 3.0)[..., None], color, checker)
    n = maps['normal']
    light = np.array([-0.45, -0.55, 0.7], np.float32)   # DirectX: green (y) points down the image
    light /= np.linalg.norm(light)
    ndotl = np.clip((n * light).sum(axis=-1), 0.0, 1.0)
    lit = (color ** 2.2) * (0.35 * maps['ao'] + 0.95 * ndotl)[..., None]
    lit = np.clip(lit, 0.0, 1.0) ** (1.0 / 2.2)
    lit = np.where((alpha > 1.0 / 3.0)[..., None], lit, checker)
    for image in (shown, lit):
        for x0, y0, x1, y1 in REGIONS.values():
            image[y0, x0:x1] = image[y1 - 1, x0:x1] = (0.85, 0.45, 0.1)
            image[y0:y1, x0] = image[y0:y1, x1 - 1] = (0.85, 0.45, 0.1)
    lt.write_png(os.path.join(PREVIEW_DIR, 'Atlas.png'), lt.to8(shown))
    lt.write_png(os.path.join(PREVIEW_DIR, 'Atlas_lit.png'), lt.to8(lit))
    _log(f'preview {PREVIEW_DIR}')


def main(argv):
    maps = paint_atlas()
    if '--install' in argv:
        write(maps, TEXTURE_DIR)
    for arg in argv:
        if arg.startswith('--out='):
            write(maps, arg.split('=', 1)[1])
    if '--no-preview' not in argv:
        preview(maps)


if __name__ == '__main__':
    main(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
