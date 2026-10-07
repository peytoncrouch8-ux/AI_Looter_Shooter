"""The wanted posters of Ransom's Rest (Docs/Areas/RansomsRest.md, Side 1 "Wanted: Already Dead"): one decal atlas,
T_Posters_BC (1024 px RGBA: sRGB colour and straight, non-premultiplied alpha that carries the paper's ragged edge),
painted by numpy like looter_textures and the same bytes on every run.

The atlas is 1 m square at the size its decals are shown (1024 px/m), so a cell's size in UV is its size in metres.
UVs below have V = 0 at the top, as Unreal reads them; each sheet is centred in its cell, with transparent gutters
round it so mips don't bleed between cells.

  1 Poster     U 0.00-0.50, V 0.000-0.703  shown 0.500 x 0.703 m: Ellis Ransom's wanted poster, a 0.45 x 0.68 m sheet.
               WANTED / DEAD OR ALIVE in Rye wood type, ALREADY scrawled under it in charcoal, the option's portrait,
               ELLIS RANSOM, what for (robbery with the Dunne Gang), $500 REWARD in red, and Capt. R. Calder's printed
               name signed in ink. Four hand-forged nails painted in at the corners; the bottom-right corner is torn
               away, leaving a scrap under its nail. Aged paper: sun-bleached cream toned at the edges, water stains
               and rain runs, foxing, folds and wrinkles, uneven letterpress ink.
  2 Remnant    U 0.50-1.00, V 0.000-0.703  shown 0.500 x 0.703 m, placed exactly where the poster was: the poster's own
               pixels where torn corners stay under the same nails, with fresh fibres along the tears.
  3 Note       U 0.00-0.25, V 0.703-1.000  shown 0.250 x 0.297 m: Ranger Ruth Calder's note for the notice board, a
               0.21 x 0.28 m page torn from a field book, her pencilled block capitals about three Ranger caches
               (under the windmill, the Sink's rim, the bluff path), signed R. Calder, on one nail.
  4 Scrap      U 0.25-0.50, V 0.703-0.891  shown 0.250 x 0.188 m: a piece torn from round DEAD OR ALIVE and the scrawl,
               for the code to tumble down as the poster comes off.
  5 ScrapBack  U 0.50-0.75, V 0.703-0.891  shown 0.250 x 0.188 m: the same piece from behind (mirrored left to right):
               bare paper with the print showing faintly through. Optional, for a two-sided scrap.
  Free         U 0.75-1.00, V 0.703-1.000 and U 0.25-0.75, V 0.891-1.000.

Three options differ only in the poster's portrait, which leaves Ellis's face and gender open (they/them):
  A  type only: a letterpress broadside (ARMED & DANGEROUS, youngest of the gang and its best shot);
  B  a woodcut bust in a wide-brimmed hat, the face lost in the brim's shadow and a bandana pulled up;
  C  the outlaw's revolver engraved like a gunsmith's catalogue plate over a vignette, "the best shot in the Dunne
     Gang".
Everything else (the paper, ALREADY, the remnant, the note, the scrap) is the same in all three.

Type is the Western face Rye (Art/Fonts/Rye-Regular.ttf, SIL OFL), read straight from the font file; no other font is
used. The handwriting (ALREADY, the note, Calder's signature) is drawn by code: single-line block capitals and a
cursive signature, each letter a little different, with wobble, overshoot and pen pressure, laid down as charcoal,
pencil or iron-gall ink.

    Tools\\artrun.ps1 -Script Tools\\Blender\\looter_posters.py                      every option and every render
    Tools\\artrun.ps1 -Script Tools\\Blender\\looter_posters.py -ScriptArgs B        one option (no comparison)
    Tools\\artrun.ps1 -Script Tools\\Blender\\looter_posters.py -ScriptArgs '--no-render'   the atlases only
    Tools\\artrun.ps1 -Script Tools\\Blender\\looter_posters.py -ScriptArgs B,'--install'   after the pick: writes
                                         Art/Textures/Posters/T_Posters_BC.png (the same bytes as the B preview)

It writes Saved/ArtPreviews/RansomsRest/Posters/: T_Posters_BC_<option>.png (the atlas itself), Atlas_<option>.png
(the atlas laid flat, cells outlined), and renders in golden late-afternoon light on the real models: the poster on
the log cabin's front logs (LogCabin.py) at about 1 m and 5 m, the remnant there, the Rim Rangers' notice board
(Boardwalk.py) before the sheriff's office (FalseFronts.py) with the poster and Calder's note, the note up close, and
A, B and C side by side. The renders stand a flat plane a few millimetres off the wall in for the decal (alpha
clipped at 0.5, casting no shadow); a decal projected onto round logs wraps round them instead.
"""
import math
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import looter_textures as lt  # noqa: E402

REPO = lt.REPO
FONT_PATH = os.path.join(REPO, 'Art', 'Fonts', 'Rye-Regular.ttf')
OUT_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Posters')
FINAL_PATH = os.path.join(lt.TEXTURE_DIR, 'Posters', 'T_Posters_BC.png')
OPTIONS = ('A', 'B', 'C')

# The atlas is 1 m square at the size its decals are shown (1024 px/m): a cell's size in UV is its size in metres.
ATLAS = 1024
PX_PER_M = 1024.0
MM = PX_PER_M / 1000.0                     # px per mm
# The cells, in px of the atlas with y down (V = 0 at the top, as Unreal reads it): x0, y0, x1, y1.
CELLS = {
    'Poster': (0, 0, 512, 720),
    'Remnant': (512, 0, 1024, 720),
    'Note': (0, 720, 256, 1024),
    'Scrap': (256, 720, 512, 912),
    'ScrapBack': (512, 720, 768, 912),
}
POSTER_MM = (450.0, 680.0)                 # the poster's sheet
NOTE_MM = (210.0, 280.0)                   # Calder's note

rgb = lt.rgb
smooth = lt.smooth
mix = lt.mix


# --- TrueType outlines ---

class TrueType:
    """A minimal TrueType reader: each glyph's outline (quadratic contours), its advance and the cmap. No kerning: old
    wood type had none either (every letter sat on its own block)."""

    def __init__(self, path):
        with open(path, 'rb') as file:
            self.data = data = file.read()
        count = struct.unpack_from('>H', data, 4)[0]
        self.tables = {}
        for i in range(count):
            tag, _, offset, _ = struct.unpack_from('>4sIII', data, 12 + 16 * i)
            self.tables[tag.decode('latin-1')] = offset
        head = self.tables['head']
        self.upem = struct.unpack_from('>H', data, head + 18)[0]
        loc_format = struct.unpack_from('>h', data, head + 50)[0]
        glyphs = struct.unpack_from('>H', data, self.tables['maxp'] + 4)[0]
        metrics = struct.unpack_from('>H', data, self.tables['hhea'] + 34)[0]
        hmtx = self.tables['hmtx']
        advances = [struct.unpack_from('>H', data, hmtx + 4 * i)[0] for i in range(metrics)]
        self.advances = advances + [advances[-1]] * (glyphs - metrics)
        loca = self.tables['loca']
        if loc_format == 0:
            self.loca = [2 * v for v in struct.unpack_from(f'>{glyphs + 1}H', data, loca)]
        else:
            self.loca = list(struct.unpack_from(f'>{glyphs + 1}I', data, loca))
        self.cmap = self._cmap()
        self._contours = {}
        self.cap = max(y for c in self.contours(self.glyph('H')) for _, y, _ in c)

    def _cmap(self):
        data, base = self.data, self.tables['cmap']
        count = struct.unpack_from('>H', data, base + 2)[0]
        table = None
        for i in range(count):
            platform, encoding, offset = struct.unpack_from('>HHI', data, base + 4 + 8 * i)
            if struct.unpack_from('>H', data, base + offset)[0] == 4 and (platform, encoding) in ((3, 1), (0, 3)):
                table = base + offset
                break
        segments = struct.unpack_from('>H', data, table + 6)[0] // 2
        ends = struct.unpack_from(f'>{segments}H', data, table + 14)
        starts = struct.unpack_from(f'>{segments}H', data, table + 16 + 2 * segments)
        deltas = struct.unpack_from(f'>{segments}h', data, table + 16 + 4 * segments)
        ranges_at = table + 16 + 6 * segments
        ranges = struct.unpack_from(f'>{segments}H', data, ranges_at)
        mapping = {}
        for s in range(segments):
            for code in range(starts[s], ends[s] + 1):
                if code == 0xFFFF:
                    continue
                if ranges[s] == 0:
                    glyph = (code + deltas[s]) & 0xFFFF
                else:
                    glyph = struct.unpack_from('>H', data, ranges_at + 2 * s + ranges[s] + 2 * (code - starts[s]))[0]
                    glyph = (glyph + deltas[s]) & 0xFFFF if glyph else 0
                mapping[code] = glyph
        return mapping

    def glyph(self, char):
        return self.cmap.get(ord(char), 0)

    def contours(self, gid):
        """The glyph's outline: closed contours of (x, y, on_curve) in font units, y up."""
        if gid not in self._contours:
            start, end = self.loca[gid], self.loca[gid + 1]
            result = []
            if end > start:
                at = self.tables['glyf'] + start
                n = struct.unpack_from('>h', self.data, at)[0]
                result = self._simple(at, n) if n >= 0 else self._composite(at)
            self._contours[gid] = result
        return self._contours[gid]

    def _simple(self, at, n):
        data = self.data
        ends = struct.unpack_from(f'>{n}H', data, at + 10)
        count = ends[-1] + 1 if n else 0
        p = at + 10 + 2 * n
        p += 2 + struct.unpack_from('>H', data, p)[0]
        flags = []
        while len(flags) < count:
            f = data[p]
            p += 1
            flags.append(f)
            if f & 8:
                flags.extend([f] * data[p])
                p += 1
        flags = flags[:count]
        coords = []
        for short, same in ((2, 16), (4, 32)):
            values, v = [], 0
            for f in flags:
                if f & short:
                    v += data[p] if f & same else -data[p]
                    p += 1
                elif not f & same:
                    v += struct.unpack_from('>h', data, p)[0]
                    p += 2
                values.append(v)
            coords.append(values)
        xs, ys = coords
        contours, s = [], 0
        for e in ends:
            contours.append([(xs[i], ys[i], bool(flags[i] & 1)) for i in range(s, e + 1)])
            s = e + 1
        return contours

    def _composite(self, at):
        data = self.data
        p = at + 10
        out = []
        while True:
            flags, gid = struct.unpack_from('>HH', data, p)
            p += 4
            if flags & 1:
                a1, a2 = struct.unpack_from('>hh', data, p)
                p += 4
            else:
                a1, a2 = struct.unpack_from('>bb', data, p)
                p += 2
            dx, dy = (a1, a2) if flags & 2 else (0, 0)
            a, b, c, d = 1.0, 0.0, 0.0, 1.0
            if flags & 8:
                a = d = struct.unpack_from('>h', data, p)[0] / 16384.0
                p += 2
            elif flags & 64:
                a, d = (v / 16384.0 for v in struct.unpack_from('>hh', data, p))
                p += 4
            elif flags & 128:
                a, b, c, d = (v / 16384.0 for v in struct.unpack_from('>hhhh', data, p))
                p += 8
            for contour in self.contours(gid):
                out.append([(a * x + c * y + dx, b * x + d * y + dy, on) for x, y, on in contour])
            if not flags & 32:
                return out


def flatten(contour, steps=8):
    """A quadratic TrueType contour as a closed polyline (an (n, 2) array, no repeated end point)."""
    pts = list(contour)
    if not any(on for _, _, on in pts):
        pts = [q for i, p in enumerate(pts) for q in (((p[0] + pts[i - 1][0]) * 0.5, (p[1] + pts[i - 1][1]) * 0.5,
                                                         True), p)]
    k = next(i for i, p in enumerate(pts) if p[2])
    pts = pts[k:] + pts[:k]
    m = len(pts)
    cur = (pts[0][0], pts[0][1])
    out = [cur]
    i = 1
    while i <= m:
        p = pts[i % m]
        if p[2]:
            cur = (p[0], p[1])
            out.append(cur)
            i += 1
            continue
        q = pts[(i + 1) % m]
        if q[2]:
            end = (q[0], q[1])
            i += 2
        else:
            end = ((p[0] + q[0]) * 0.5, (p[1] + q[1]) * 0.5)
            i += 1
        for s in range(1, steps + 1):
            t = s / steps
            u = 1.0 - t
            out.append((u * u * cur[0] + 2.0 * u * t * p[0] + t * t * end[0],
                        u * u * cur[1] + 2.0 * u * t * p[1] + t * t * end[1]))
        cur = end
    a = np.array(out, np.float64)
    keep = np.ones(len(a), bool)
    keep[1:] = np.any(np.abs(np.diff(a, axis=0)) > 1e-9, axis=1)
    a = a[keep]
    if len(a) > 1 and np.allclose(a[0], a[-1]):
        a = a[:-1]
    return a


_FONT = []


def font():
    if not _FONT:
        _FONT.append(TrueType(FONT_PATH))
    return _FONT[0]


# --- Rasterizing ---

def fill(loops, shape, origin=(0.0, 0.0), ss=4, reduce=True):
    """Coverage (0..1, shape (h, w)) of closed polygons filled by the non-zero winding rule, ss x ss samples a pixel.
    Each loop is an (n, 2) array of pixel coordinates (x right, y down); origin is the region's top-left corner.
    reduce=False returns the samples themselves (h * ss, w * ss) of 0 and 1."""
    h, w = shape
    H, W = h * ss, w * ss
    acc = np.zeros((H, W + 1), np.int32)
    ox, oy = origin
    for loop in loops:
        p = np.asarray(loop, np.float64)
        if len(p) < 3:
            continue
        p = np.stack([(p[:, 0] - ox) * ss, (p[:, 1] - oy) * ss], axis=1)
        q = np.roll(p, -1, axis=0)
        x0, y0, x1, y1 = p[:, 0], p[:, 1], q[:, 0], q[:, 1]
        r0 = np.clip(np.ceil(np.minimum(y0, y1) - 0.5), 0, H).astype(np.int64)
        r1 = np.clip(np.ceil(np.maximum(y0, y1) - 0.5), 0, H).astype(np.int64)
        n = r1 - r0
        keep = n > 0
        if not keep.any():
            continue
        x0, y0, x1, y1, r0, n = (a[keep] for a in (x0, y0, x1, y1, r0, n))
        edge = np.repeat(np.arange(len(n)), n)
        rows = np.repeat(r0, n) + (np.arange(int(n.sum())) - np.repeat(np.cumsum(n) - n, n))
        t = (rows + 0.5 - y0[edge]) / (y1[edge] - y0[edge])
        cols = np.clip(np.ceil(x0[edge] + t * (x1[edge] - x0[edge]) - 0.5), 0, W).astype(np.int64)
        np.add.at(acc, (rows, cols), np.where(y1[edge] > y0[edge], 1, -1).astype(np.int32))
    inside = (np.cumsum(acc[:, :W], axis=1) != 0).astype(np.float32)
    if not reduce:
        return inside
    return inside.reshape(h, ss, w, ss).mean(axis=(1, 3))


def _box(points, pad, shape):
    """The integer pixel box (x0, y0, x1, y1) round points (px), padded, clipped to shape; None if empty."""
    p = np.asarray(points)
    x0 = max(int(math.floor(p[:, 0].min() - pad)), 0)
    y0 = max(int(math.floor(p[:, 1].min() - pad)), 0)
    x1 = min(int(math.ceil(p[:, 0].max() + pad)) + 1, shape[1])
    y1 = min(int(math.ceil(p[:, 1].max() + pad)) + 1, shape[0])
    return (x0, y0, x1, y1) if x1 > x0 and y1 > y0 else None


def fill_into(layer, loops, ss=4, value=None, density=None, dens_value=1.0):
    """Fills polygons (px) into layer (max), only over their box; density (a second image) takes dens_value where
    the fill is the new maximum. Returns the coverage and its box."""
    loops = [np.asarray(l, np.float64) for l in loops if len(l) >= 3]
    if not loops:
        return None, None
    box = _box(np.concatenate(loops), 2, layer.shape)
    if box is None:
        return None, None
    x0, y0, x1, y1 = box
    cov = fill(loops, (y1 - y0, x1 - x0), origin=(x0, y0), ss=ss)
    if value is not None:
        cov = cov * value
    sub = layer[y0:y1, x0:x1]
    if density is not None:
        dsub = density[y0:y1, x0:x1]
        dsub[...] = np.where(cov > sub, dens_value, dsub)
    np.maximum(sub, cov, out=sub)
    return cov, box


def draw_strokes(shape, strokes, width, var=0.45, rough=0.0, seed=0, nib=None, profile=False):
    """Coverage and pressure (0..1 images) of strokes drawn with a tip width px wide at full pressure. Each stroke is
    (points (n, 2) px, pressure (n,)); var is how much lighter pressure narrows the line, rough wavers its width (a
    fraction of the radius). nib = (angle degrees, thinness): a flat tip (a charcoal stick) draws thinner along the
    direction of its edge. profile=True also returns how deep each pixel lies in its stroke (1 on the centre line,
    0 at the edge)."""
    rng = np.random.default_rng(seed)
    cov = np.zeros(shape, np.float32)
    pres = np.zeros(shape, np.float32)
    prof = np.zeros(shape, np.float32)
    for pts, p in strokes:
        pts = np.asarray(pts, np.float64)
        p = np.asarray(p, np.float64)
        if len(pts) == 1:
            pts = np.vstack([pts, pts + 0.05])
            p = np.concatenate([p, p])
        r = 0.5 * width * (1.0 - var + var * p)
        if nib is not None and len(pts) > 2:
            d = np.gradient(pts, axis=0)
            direction = np.arctan2(d[:, 1], d[:, 0])
            r = r * (1.0 - nib[1] * np.abs(np.cos(direction - math.radians(nib[0]))))
        if rough:
            r = r * (1.0 + rough * noise1d(len(r), rng, 4.0))
        r = np.maximum(r, 0.25)
        for k in range(0, len(pts) - 1, 40):
            a, ra, pa = pts[k:k + 41], r[k:k + 41], p[k:k + 41]
            box = _box(a, ra.max() + 1.5, shape)
            if box is None:
                continue
            x0, y0, x1, y1 = box
            gy, gx = np.mgrid[y0:y1, x0:x1].astype(np.float64) + 0.5
            gx, gy = gx.reshape(-1, 1), gy.reshape(-1, 1)
            A, B = a[:-1], a[1:]
            ab = B - A
            l2 = (ab * ab).sum(1) + 1e-12
            t = np.clip(((gx - A[:, 0]) * ab[:, 0] + (gy - A[:, 1]) * ab[:, 1]) / l2, 0.0, 1.0)
            d = np.hypot(gx - (A[:, 0] + t * ab[:, 0]), gy - (A[:, 1] + t * ab[:, 1]))
            rad = ra[:-1] + t * (ra[1:] - ra[:-1])
            c = np.clip(rad - d + 0.5, 0.0, 1.0)
            best = c.argmax(axis=1)
            rows = np.arange(len(best))
            cmax = c[rows, best].reshape(y1 - y0, x1 - x0).astype(np.float32)
            pp = (pa[:-1][best] + t[rows, best] * (pa[1:][best] - pa[:-1][best])).reshape(y1 - y0, x1 - x0)
            sub, psub = cov[y0:y1, x0:x1], pres[y0:y1, x0:x1]
            psub[...] = np.where(cmax > sub, pp, psub)
            np.maximum(sub, cmax, out=sub)
            if profile:
                depth = np.clip((rad - d) / rad, 0.0, 1.0).max(axis=1).reshape(y1 - y0, x1 - x0)
                np.maximum(prof[y0:y1, x0:x1], depth.astype(np.float32), out=prof[y0:y1, x0:x1])
    if profile:
        return cov, pres, prof
    return cov, pres


def noise1d(n, rng, sigma):
    """n samples of smooth noise (mean 0, deviation about 1), sigma in samples."""
    pad = int(4 * sigma) + 2
    white = rng.standard_normal(n + 2 * pad)
    k = np.exp(-0.5 * (np.arange(-pad, pad + 1) / max(sigma, 1e-3)) ** 2)
    k /= k.sum()
    # Divided by the filter's own deviation, not the samples': a short run of smooth noise is nearly flat, and
    # stretching it to a deviation of 1 would fling it far off.
    return np.convolve(white, k, mode='same')[pad:pad + n] / math.sqrt(float((k * k).sum()))


def densify(points, step, closed=False):
    """A polyline resampled every step (same units), corners kept."""
    pts = np.asarray(points, np.float64)
    if closed:
        pts = np.vstack([pts, pts[:1]])
    out = [pts[:1]]
    for a, b in zip(pts[:-1], pts[1:]):
        n = max(1, int(math.ceil(np.hypot(*(b - a)) / step)))
        t = np.arange(1, n + 1, dtype=np.float64)[:, None] / n
        out.append(a + (b - a) * t)
    out = np.concatenate(out)
    return out[:-1] if closed else out


def ragged(points, rng, octaves, inward=None, bumps=()):
    """Displaces a dense polyline (px) sideways by smooth noise: octaves = [(amplitude px, sigma samples), ...].
    inward is the unit normal the offset is measured along (default: the polyline's left normal). bumps adds
    (position 0..1, depth px, width samples) nicks."""
    p = np.asarray(points, np.float64)
    n = len(p)
    if inward is None:
        tangent = np.gradient(p, axis=0)
        tangent /= np.maximum(np.hypot(tangent[:, 0], tangent[:, 1]), 1e-9)[:, None]
        normal = np.stack([-tangent[:, 1], tangent[:, 0]], axis=1)
    else:
        normal = np.broadcast_to(np.asarray(inward, np.float64), p.shape)
    offset = np.zeros(n)
    for amplitude, sigma in octaves:
        offset += amplitude * noise1d(n, rng, sigma)
    idx = np.arange(n)
    for at, depth, width in bumps:
        offset += depth * np.exp(-0.5 * ((idx - at * n) / width) ** 2) * (1.0 + 0.3 * noise1d(n, rng, 1.5))
    # The ends stay put, so pieces join.
    taper = np.minimum(1.0, np.minimum(idx, n - 1 - idx) / 6.0)
    return p + normal * (offset * taper)[:, None]


def spline(points, per=10, closed=False):
    """A Catmull-Rom curve through points: per samples per span."""
    p = np.asarray(points, np.float64)
    if closed:
        ext = np.vstack([p[-1:], p, p[:2]])
    else:
        ext = np.vstack([2 * p[0] - p[1], p, 2 * p[-1] - p[-2]])
    out = []
    spans = len(p) if closed else len(p) - 1
    t = np.linspace(0.0, 1.0, per, endpoint=False)[:, None]
    for i in range(spans):
        p0, p1, p2, p3 = ext[i], ext[i + 1], ext[i + 2], ext[i + 3]
        out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t ** 2 +
                          (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3))
    if not closed:
        out.append(p[-1:])
    return np.concatenate(out)


def arc(cx, cy, rx, ry, a0, a1, step=8.0):
    """Points on an ellipse from angle a0 to a1 (degrees, counter-clockwise, y up)."""
    n = max(3, int(abs(a1 - a0) / step) + 1)
    a = np.radians(np.linspace(a0, a1, n))
    return np.stack([cx + rx * np.cos(a), cy + ry * np.sin(a)], axis=1)


def rect(x0, y0, x1, y1):
    return np.array([(x0, y0), (x1, y0), (x1, y1), (x0, y1)], np.float64)


def star(cx, cy, r, points=5, inner=0.42):
    a = np.radians(-90.0 + np.arange(2 * points) * 180.0 / points)
    rr = np.where(np.arange(2 * points) % 2 == 0, r, r * inner)
    return np.stack([cx + rr * np.cos(a), cy + rr * np.sin(a)], axis=1)


def diamond(cx, cy, rx, ry):
    return np.array([(cx, cy - ry), (cx + rx, cy), (cx, cy + ry), (cx - rx, cy)], np.float64)


def segment_distance(x, y, a, b):
    """Distance from points (x, y arrays) to the segment a-b, and the position t (0..1) along it."""
    ab = np.subtract(b, a)
    l2 = float(ab @ ab) + 1e-12
    t = np.clip(((x - a[0]) * ab[0] + (y - a[1]) * ab[1]) / l2, 0.0, 1.0)
    return np.hypot(x - (a[0] + t * ab[0]), y - (a[1] + t * ab[1])), t


def polyline_distance(x, y, points):
    """Distance from points (x, y arrays) to a polyline (n, 2)."""
    best = np.full(np.shape(x), np.inf)
    for a, b in zip(points[:-1], points[1:]):
        d, _ = segment_distance(x, y, a, b)
        best = np.minimum(best, d)
    return best


# --- A sheet in its cell ---

class Sheet:
    """A sheet of paper centred in its atlas cell: its size (mm), the cell's shape (px) and mm -> px."""

    def __init__(self, cell, size_mm):
        x0, y0, x1, y1 = CELLS[cell]
        self.cell = cell
        self.shape = (y1 - y0, x1 - x0)
        self.size = size_mm
        self.ox = (self.shape[1] - size_mm[0] * MM) * 0.5
        self.oy = (self.shape[0] - size_mm[1] * MM) * 0.5

    def px(self, pts):
        p = np.asarray(pts, np.float64)
        return np.stack([self.ox + p[..., 0] * MM, self.oy + p[..., 1] * MM], axis=-1)

    def grid(self):
        """Every pixel's centre in mm (u right, v down)."""
        x, y = lt.coords(self.shape)
        return (x + 0.5 - self.ox) / MM, (y + 0.5 - self.oy) / MM


# --- Type ---

class Ink:
    """A printed colour on a sheet: coverage, each piece of type's inking, and where the big wood type is."""

    def __init__(self, shape):
        self.cov = np.zeros(shape, np.float32)
        self.dens = np.ones(shape, np.float32)
        self.wood = np.zeros(shape, np.float32)


def set_line(text, cap, width=None, stretch=1.0, tracking=0.0):
    """One line of Rye at cap height cap (mm): (glyphs, ink width), each glyph a list of loops in mm (x from the ink's
    left edge, y up from the baseline). width (mm) stretches or condenses the face to exactly that ink width;
    tracking (mm) spaces the letters."""
    f = font()
    s = cap / f.cap
    raw, pen = [], 0.0
    for ch in text:
        g = f.glyph(ch)
        raw.append([flatten(c) * s + (pen, 0.0) for c in f.contours(g)])
        pen += f.advances[g] * s
    xs = np.concatenate([l[:, 0] for loops in raw for l in loops])
    lo, hi = xs.min(), xs.max()
    if width is not None:
        stretch = (width - tracking * (len(text) - 1)) / (hi - lo)
    glyphs = [[np.stack([(l[:, 0] - lo) * stretch + tracking * i, l[:, 1]], axis=1) for l in loops]
              for i, loops in enumerate(raw)]
    return glyphs, (hi - lo) * stretch + tracking * (len(text) - 1)


def type_line(ink, sheet, rng, text, cap, x, y, width=None, stretch=1.0, tracking=0.0, align='center', wood=False,
              turn=0.22, wobble=0.18):
    """Prints a line of Rye on the sheet with its baseline at y (mm) and its ink centred on x (or starting at x with
    align='left', ending at x with 'right'). Every piece of type sits a hair off its neighbours and inks a little
    differently. Returns the ink width (mm)."""
    glyphs, ink_w = set_line(text, cap, width, stretch, tracking)
    left = {'center': x - ink_w * 0.5, 'left': x, 'right': x - ink_w}[align]
    for loops in glyphs:
        if not loops:
            continue
        pts = [sheet.px(np.stack([l[:, 0] + left, y - l[:, 1]], axis=1)) for l in loops]
        c = np.concatenate(pts).mean(axis=0)
        a = math.radians(rng.normal(0.0, turn))
        m = np.array([[math.cos(a), -math.sin(a)], [math.sin(a), math.cos(a)]])
        shift = rng.normal(0.0, wobble, 2)
        pts = [(p - c) @ m.T + c + shift for p in pts]
        inking = float(np.clip(rng.normal(0.93, 0.04), 0.8, 1.0))
        cov, box = fill_into(ink.cov, pts, density=ink.dens, dens_value=inking)
        if wood and box is not None:
            x0, y0, x1, y1 = box
            np.maximum(ink.wood[y0:y1, x0:x1], cov, out=ink.wood[y0:y1, x0:x1])
    return ink_w


def type_shape(ink, sheet, loops_mm, dens=0.95):
    """Prints polygons given in mm (rules, stars, ornaments)."""
    fill_into(ink.cov, [sheet.px(l) for l in loops_mm], density=ink.dens, dens_value=dens)


def rule(ink, sheet, x0, x1, y, thick, rng):
    """A brass rule: a long thin bar, very slightly uneven."""
    n = max(8, int((x1 - x0) / 4.0))
    xs = np.linspace(x0, x1, n)
    top = y - thick * 0.5 + rng.normal(0.0, 0.04, n) * min(thick, 1.0)
    bottom = y + thick * 0.5 + rng.normal(0.0, 0.04, n) * min(thick, 1.0)
    loop = np.concatenate([np.stack([xs, top], 1), np.stack([xs[::-1], bottom[::-1]], 1)])
    type_shape(ink, sheet, [loop], dens=float(np.clip(rng.normal(0.94, 0.03), 0.85, 1.0)))


# --- Paper ---

def _noise(shape, seed, sx, sy=None, octaves=1):
    return lt.noise(shape, int(seed), float(sx), None if sy is None else float(sy), octaves=octaves)


def tooth(shape, seed):
    """The paper's grain (0..1) that charcoal and pencil catch on."""
    n = 0.7 * _noise(shape, seed, 0.6) + 0.45 * _noise(shape, seed + 1, 1.6) + 0.35 * _noise(shape, seed + 2, 1.2, 0.4)
    return 0.5 + 0.5 * np.tanh(0.9 * n)


def tide_stain(u, v, center, radius, aspect, seed, shape, warp=0.32):
    """A dried water stain: (inside 0..1, tide line 0..1). center and radius in mm."""
    d = np.hypot(u - center[0], (v - center[1]) / aspect) / radius
    d = d + warp * _noise(shape, seed, 22.0) + 0.1 * _noise(shape, seed + 1, 5.0)
    inside = smooth(1.03, 0.97, d)
    tide = np.exp(-((d - 1.0) / 0.014) ** 2) + 0.45 * np.exp(-((d - 0.86) / 0.02) ** 2) * inside
    return inside, tide


def drip(u, v, x0, v0, length, seed, width=2.4):
    """A rain run down from (x0, v0) mm: (inside, tide line)."""
    rng = np.random.default_rng(seed)
    grid = np.arange(-40.0, 760.0, 1.0)
    wander = np.interp(v, grid, noise1d(len(grid), rng, 18.0)) * 1.6
    along = (v - v0) / length
    swell = 1.0 + 0.15 * np.interp(v, grid, noise1d(len(grid), rng, 6.0))
    half = width * (0.7 + 0.6 * np.clip(along, 0.0, 1.0)) * swell
    across = np.abs(u - x0 - wander)
    fade = smooth(-0.02, 0.06, along) * smooth(1.0, 0.55, along)
    inside = smooth(half + 0.4, half - 0.4, across) * fade
    tide = np.exp(-((across - half) / 0.35) ** 2) * fade
    return inside, tide


def crease(u, v, p0, p1, seed, strength=1.0, fade_end=False):
    """A fold from p0 to p1 (mm), lit from the upper left: (shade -1..1, core 0..1). The core is where the ink cracked
    along the fold."""
    rng = np.random.default_rng(seed)
    d = np.subtract(p1, p0)
    length = float(np.hypot(*d))
    tx, ty = d / length
    nx, ny = -ty, tx
    t = (u - p0[0]) * tx + (v - p0[1]) * ty
    s = (u - p0[0]) * nx + (v - p0[1]) * ny
    grid = np.arange(-20.0, length + 20.0, 1.0)
    for amplitude, sigma in ((0.7, 25.0), (0.15, 4.0)):       # the fold wanders a little along its length
        s = s - np.interp(t, grid, noise1d(len(grid), rng, sigma)) * amplitude
    vary = 0.55 + 0.45 * smooth(-1.0, 1.0, np.interp(t, grid, noise1d(len(grid), rng, 30.0)))
    span = smooth(-1.0, 3.0, t) * smooth(-1.0, 3.0, length - t)
    if fade_end:
        span = span * np.clip(1.0 - t / length, 0.0, 1.0) ** 1.4
    light = 1.0 if (nx * -0.6 + ny * -0.8) > 0 else -1.0     # which side faces the light
    shade = (np.exp(-((s - 0.75) / 0.6) ** 2) - np.exp(-((s + 0.65) / 0.6) ** 2)) * light
    core = np.exp(-(s / 0.42) ** 2)
    k = vary * span * strength
    return shade * k, core * k


def spots(shape, rng, points, sizes, strengths):
    """Foxing: small soft spots at points (px) of sizes (sigma px) -> (cores, halos)."""
    cores = np.zeros(shape, np.float32)
    halos = np.zeros(shape, np.float32)
    h, w = shape
    for sigma in sorted(set(sizes)):
        img = np.zeros(shape, np.float32)
        for (x, y), s, k in zip(points, sizes, strengths):
            if s == sigma and 0 <= int(y) < h and 0 <= int(x) < w:
                img[int(y), int(x)] += k
        cores += lt.blur(img, sigma) * (2.0 * math.pi * sigma * sigma)
        halos += lt.blur(img, sigma * 3.2) * (2.0 * math.pi * sigma * sigma)
    return np.clip(cores, 0.0, 1.0), np.clip(halos, 0.0, 1.0)


# --- Handwriting ---

def _glyph_table():
    """Block capitals as single-line strokes in em units (cap height 1, y up): {char: (advance, [stroke, ...])}."""
    P = lambda *pts: np.array(pts, np.float64)
    S = lambda *pts: spline(np.array(pts, np.float64), per=8)
    A = arc
    C = lambda *parts: np.concatenate(parts)
    o = A(0.4, 0.5, 0.4, 0.5, 100.0, 470.0)
    g = {
        'A': (0.74, [P((0, 0), (0.37, 1), (0.74, 0)), P((0.14, 0.36), (0.6, 0.38))]),
        'B': (0.62, [P((0, 1), (0, 0)), C(P((0, 1), (0.3, 1)), A(0.3, 0.76, 0.26, 0.24, 90, -90), P((0.06, 0.52)),
                                         P((0.34, 0.52)), A(0.34, 0.26, 0.28, 0.26, 90, -90), P((0.0, 0.0)))]),
        'C': (0.72, [A(0.4, 0.5, 0.4, 0.5, 48, 312)]),
        'D': (0.7, [P((0, 1), (0, 0)), C(P((0, 1), (0.26, 1)), A(0.26, 0.5, 0.4, 0.5, 90, -90), P((0.0, 0.0)))]),
        'E': (0.6, [P((0.56, 1), (0, 1), (0, 0), (0.58, 0)), P((0, 0.52), (0.46, 0.52))]),
        'F': (0.58, [P((0.56, 1), (0, 1), (0, 0)), P((0, 0.52), (0.44, 0.52))]),
        'G': (0.76, [C(A(0.4, 0.5, 0.4, 0.5, 48, 340), P((0.8, 0.46), (0.48, 0.46)))]),
        'H': (0.7, [P((0, 1), (0, 0)), P((0.68, 1), (0.68, 0)), P((0, 0.52), (0.68, 0.5))]),
        'I': (0.22, [P((0.1, 1), (0.1, 0))]),
        'J': (0.56, [C(P((0.56, 1), (0.56, 0.3)), A(0.3, 0.3, 0.26, 0.3, 0, -180))]),
        'K': (0.66, [P((0, 1), (0, 0)), P((0.62, 1), (0.03, 0.44), (0.66, 0))]),
        'L': (0.54, [P((0, 1), (0, 0), (0.54, 0))]),
        'M': (0.86, [P((0, 0), (0.02, 1), (0.42, 0.3), (0.82, 1), (0.84, 0))]),
        'N': (0.7, [P((0, 0), (0, 1), (0.68, 0), (0.68, 1))]),
        'O': (0.8, [o]),
        'P': (0.62, [C(P((0, 0), (0, 1), (0.3, 1)), A(0.3, 0.74, 0.28, 0.26, 90, -90), P((0.02, 0.48)))]),
        'Q': (0.8, [o, P((0.48, 0.26), (0.86, -0.08))]),
        'R': (0.66, [C(P((0, 0), (0, 1), (0.3, 1)), A(0.3, 0.75, 0.28, 0.25, 90, -90), P((0.06, 0.5), (0.66, 0)))]),
        'S': (0.62, [S((0.58, 0.86), (0.44, 0.99), (0.22, 0.98), (0.06, 0.84), (0.1, 0.62), (0.32, 0.52), (0.54, 0.4),
                       (0.6, 0.18), (0.46, 0.02), (0.24, 0.0), (0.02, 0.12))]),
        'T': (0.7, [P((0, 1), (0.7, 1)), P((0.35, 1), (0.35, 0))]),
        'U': (0.7, [C(P((0, 1), (0, 0.32)), A(0.34, 0.32, 0.34, 0.32, 180, 360), P((0.68, 1)))]),
        'V': (0.72, [P((0, 1), (0.36, 0), (0.72, 1))]),
        'W': (0.98, [P((0, 1), (0.22, 0), (0.48, 0.72), (0.74, 0), (0.96, 1))]),
        'X': (0.68, [P((0, 1), (0.66, 0)), P((0.66, 1), (0, 0))]),
        'Y': (0.7, [P((0, 1), (0.34, 0.5)), P((0.68, 1), (0.34, 0.5), (0.34, 0))]),
        'Z': (0.64, [P((0, 1), (0.62, 1), (0, 0), (0.64, 0))]),
        '0': (0.62, [A(0.3, 0.5, 0.3, 0.5, 95, 465)]),
        '1': (0.36, [P((0.06, 0.78), (0.24, 1), (0.24, 0))]),
        '2': (0.62, [C(S((0.02, 0.76), (0.14, 0.95), (0.32, 1.0), (0.5, 0.9), (0.54, 0.7), (0.4, 0.48), (0.0, 0.0)),
                       P((0.6, 0.0)))]),
        '3': (0.6, [S((0.02, 0.88), (0.2, 1.0), (0.44, 0.96), (0.52, 0.78), (0.4, 0.6), (0.2, 0.55), (0.44, 0.5),
                      (0.58, 0.3), (0.5, 0.08), (0.28, 0.0), (0.0, 0.1))]),
        '4': (0.66, [P((0.46, 0), (0.46, 1), (0, 0.3), (0.64, 0.3))]),
        '5': (0.6, [C(P((0.54, 1), (0.08, 1), (0.04, 0.56)), S((0.04, 0.56), (0.28, 0.64), (0.52, 0.52), (0.58, 0.28),
                                                              (0.44, 0.04), (0.2, 0.0), (0.0, 0.1)))]),
        '6': (0.6, [S((0.48, 1.0), (0.24, 0.86), (0.06, 0.52), (0.06, 0.2), (0.28, 0.0), (0.52, 0.12), (0.54, 0.36),
                      (0.3, 0.5), (0.08, 0.34))]),
        '7': (0.6, [P((0, 1), (0.6, 1), (0.18, 0))]),
        '8': (0.6, [S((0.3, 0.55), (0.08, 0.7), (0.12, 0.92), (0.3, 1.0), (0.5, 0.92), (0.52, 0.72), (0.3, 0.55),
                      (0.06, 0.36), (0.08, 0.1), (0.3, 0.0), (0.54, 0.1), (0.54, 0.36), (0.3, 0.55))]),
        '9': (0.6, [S((0.52, 0.66), (0.3, 0.5), (0.08, 0.62), (0.08, 0.88), (0.3, 1.0), (0.52, 0.9), (0.54, 0.62),
                      (0.48, 0.28), (0.28, 0.0))]),
        '.': (0.24, [P((0.1, 0.0), (0.13, 0.04))]),
        ',': (0.24, [P((0.13, 0.06), (0.05, -0.16))]),
        ':': (0.24, [P((0.1, 0.0), (0.13, 0.04)), P((0.1, 0.5), (0.13, 0.54))]),
        '-': (0.44, [P((0.04, 0.46), (0.38, 0.48))]),
        "'": (0.16, [P((0.08, 1.0), (0.06, 0.74))]),
        '/': (0.48, [P((0.46, 1), (0, 0))]),
        '!': (0.22, [P((0.1, 1), (0.1, 0.3)), P((0.1, 0.0), (0.12, 0.04))]),
        '?': (0.54, [S((0.02, 0.8), (0.16, 0.98), (0.38, 0.98), (0.5, 0.8), (0.38, 0.6), (0.24, 0.48), (0.24, 0.3)),
                     P((0.24, 0.0), (0.26, 0.04))]),
        '&': (0.68, [S((0.66, 0.0), (0.36, 0.32), (0.14, 0.6), (0.14, 0.86), (0.3, 1.0), (0.44, 0.86), (0.38, 0.66),
                       (0.06, 0.38), (0.06, 0.1), (0.28, 0.0), (0.56, 0.24))]),
        ' ': (0.44, []),
    }
    return g


GLYPHS = _glyph_table()


def _resample(pts, step):
    pts = np.asarray(pts, np.float64)
    seg = np.hypot(*np.diff(pts, axis=0).T)
    s = np.concatenate([[0.0], np.cumsum(seg)])
    total = s[-1]
    n = max(2, int(math.ceil(total / step)) + 1)
    t = np.linspace(0.0, total, n)
    return np.stack([np.interp(t, s, pts[:, 0]), np.interp(t, s, pts[:, 1])], axis=1), total


def pressure(n, rng, base=1.0, attack=0.07, release=0.2, lift=0.45, start=0.75):
    """Pen pressure along a stroke of n samples: a quick landing, a wavering middle and a lift at the end."""
    t = np.linspace(0.0, 1.0, n)
    p = base * (start + (1.0 - start) * smooth(0.0, attack, t)) * (1.0 - (1.0 - lift) * smooth(1.0 - release, 1.0, t))
    return np.clip(p * (1.0 + 0.1 * noise1d(n, rng, max(n / 6.0, 1.0))), 0.05, 1.2)


def hand_stroke(pts_px, rng, cap_px, wobble=0.012, step=0.6, overshoot=(-0.02, 0.06), **pressure_args):
    """A stroke's points (px) as a hand draws them: resampled, ends over- or undershot, wobbling a little."""
    pts = np.array(pts_px, np.float64)
    if len(pts) >= 2:
        for end, nb in ((0, 1), (-1, -2)):
            d = pts[end] - pts[nb]
            length = np.hypot(*d)
            if length > 1e-6:
                pts[end] = pts[end] + d / length * rng.uniform(*overshoot) * cap_px
    pts, total = _resample(pts, step)
    n = len(pts)
    if n > 3:
        tangent = np.gradient(pts, axis=0)
        tangent /= np.maximum(np.hypot(tangent[:, 0], tangent[:, 1]), 1e-9)[:, None]
        normal = np.stack([-tangent[:, 1], tangent[:, 0]], axis=1)
        sigma = max(0.28 * cap_px / step, 2.0)
        pts = pts + normal * (wobble * cap_px * noise1d(n, rng, sigma))[:, None]
    return pts, pressure(n, rng, **pressure_args)


class Hand:
    """Someone writing block capitals: every letter a little different in size, slant, turn and baseline; strokes
    that wobble, overshoot and lift; a pressure along each stroke."""

    def __init__(self, seed, slant=0.1, jitter=1.0, spacing=0.16, wobble=0.012, narrow=1.0):
        self.rng = np.random.default_rng(seed)
        self.slant, self.jitter, self.spacing, self.wobble, self.narrow = slant, jitter, spacing, wobble, narrow

    def line(self, text, x, y, cap, angle=0.0, sag=0.0, weight=1.0, anchor='left', grow=0.0):
        """The strokes of one line of text with its baseline at (x, y) px (y down): its left end there, or its middle
        with anchor='center'. Letters are cap px tall (grow: the last letter that much bigger, the others in
        between); the line is turned by angle (degrees, positive up to the right) round that point and bowed by sag
        (px at its middle). Returns (strokes, width px)."""
        rng, j = self.rng, self.jitter
        strokes, pen = [], 0.0
        count = max(len(text) - 1, 1)
        for index, ch in enumerate(text):
            advance, glyph = GLYPHS.get(ch.upper(), GLYPHS['?'])
            size = cap * (1.0 + grow * index / count)
            sx = size * self.narrow * rng.normal(1.0, 0.05 * j)
            sy = size * rng.normal(1.0, 0.06 * j)
            turn = math.radians(rng.normal(0.0, 2.2 * j))
            shear = self.slant + rng.normal(0.0, 0.035 * j)
            lift = rng.normal(0.0, 0.035 * j)
            cs, sn = math.cos(turn), math.sin(turn)
            for stroke in glyph:
                p = stroke.copy()
                p += rng.normal(0.0, 0.016 * j, 2)
                p = (p - (advance * 0.5, 0.5)) * rng.normal(1.0, 0.025 * j, 2) + (advance * 0.5, 0.5)
                ex = p[:, 0] + shear * p[:, 1]
                ey = p[:, 1] + lift
                rx = (ex - advance * 0.5) * cs - (ey - 0.5) * sn + advance * 0.5
                ry = (ex - advance * 0.5) * sn + (ey - 0.5) * cs + 0.5
                pts = np.stack([pen + rx * sx, -ry * sy], axis=1)
                strokes.append(pts)
            pen += (advance + self.spacing + rng.normal(0.0, 0.03 * j)) * size * self.narrow
        width = pen - self.spacing * cap * self.narrow
        shift = width * 0.5 if anchor == 'center' else 0.0
        a = math.radians(angle)
        cs, sn = math.cos(a), math.sin(a)
        out = []
        for pts in strokes:
            px = pts[:, 0]
            py = pts[:, 1] + sag * (1.0 - (2.0 * px / max(width, 1.0) - 1.0) ** 2)
            px = px - shift
            pts = np.stack([x + px * cs + py * sn, y - px * sn + py * cs], axis=1)
            out.append(hand_stroke(pts, rng, cap, self.wobble, base=float(rng.normal(1.0, 0.07)) * weight))
        return out, width


# Ruth Calder's signature, "R. Calder": quick cursive strokes in em (cap height 1, y up).
SIGNATURE = [
    [(0.2, 1.02), (0.1, 0.0)],
    [(0.06, 0.84), (0.24, 1.02), (0.46, 1.04), (0.6, 0.92), (0.58, 0.74), (0.42, 0.6), (0.2, 0.55), (0.3, 0.5),
     (0.44, 0.34), (0.56, 0.12), (0.68, 0.0), (0.8, 0.02)],
    [(0.93, 0.02), (0.95, 0.06)],
    [(1.66, 0.86), (1.55, 0.99), (1.38, 1.0), (1.22, 0.86), (1.14, 0.58), (1.16, 0.26), (1.3, 0.04), (1.5, 0.02),
     (1.62, 0.12), (1.7, 0.3), (1.74, 0.42), (1.68, 0.46), (1.6, 0.4), (1.56, 0.22), (1.6, 0.06), (1.68, 0.06),
     (1.76, 0.26), (1.78, 0.44), (1.78, 0.2), (1.82, 0.03), (1.88, 0.06), (1.96, 0.36), (2.04, 0.78), (2.06, 1.04),
     (2.0, 1.08), (1.95, 0.9), (1.94, 0.5), (1.97, 0.12), (2.03, 0.02), (2.1, 0.08), (2.18, 0.3), (2.22, 0.42),
     (2.15, 0.46), (2.08, 0.36), (2.08, 0.12), (2.15, 0.03), (2.24, 0.12), (2.3, 0.5), (2.34, 1.05), (2.31, 0.6),
     (2.3, 0.2), (2.34, 0.03), (2.42, 0.08), (2.5, 0.2), (2.6, 0.3), (2.58, 0.42), (2.5, 0.42), (2.44, 0.26),
     (2.48, 0.06), (2.58, 0.02), (2.66, 0.1), (2.72, 0.3), (2.74, 0.42), (2.76, 0.3), (2.82, 0.38), (2.9, 0.36),
     (2.94, 0.26), (2.96, 0.08), (2.84, -0.14), (2.4, -0.24), (1.7, -0.22), (1.2, -0.12)],
]


def signature(x, y, cap, seed, angle=-3.0, slant=0.18):
    """Calder's signature with its baseline starting at (x, y) px, cap px tall: strokes for draw_strokes."""
    rng = np.random.default_rng(seed)
    a = math.radians(angle)
    cs, sn = math.cos(a), math.sin(a)
    out = []
    for k, ctrl in enumerate(SIGNATURE):
        p = np.array(ctrl, np.float64)
        p[:, 0] = np.where(p[:, 0] > 1.1, 1.1 + (p[:, 0] - 1.1) * 1.2, p[:, 0])
        p = p + rng.normal(0.0, 0.012, p.shape)
        if len(p) > 2:
            p = spline(p, per=10)
        ex = (p[:, 0] + slant * p[:, 1]) * cap
        ey = -p[:, 1] * cap
        pts = np.stack([x + ex * cs + ey * sn, y - ex * sn + ey * cs], axis=1)
        out.append(hand_stroke(pts, rng, cap, wobble=0.006, overshoot=(0.0, 0.02), base=1.0, release=0.12, lift=0.35))
    return out


def charcoal(cov, pres, prof, grain, seed, smear=(5.0, 2.0)):
    """Charcoal on paper: solid where the stick bore down, catching only on the grain's peaks at the stroke's edges
    and where it lifted, with dust round it and a smear where a hand brushed it (smear: its offset in px)."""
    p = np.clip(pres, 0.0, 1.1) * (0.3 + 0.7 * np.sqrt(prof))
    thr = 1.22 - 1.25 * p
    a = cov * smooth(thr - 0.15, thr + 0.15, grain)
    dust = lt.blur(a, 1.5) * 0.16 + lt.blur(a, 4.5) * 0.06
    shifted = np.roll(np.roll(lt.blur(a, 3.0), int(smear[0]), axis=1), int(smear[1]), axis=0)
    smudge = shifted * 0.16 * smooth(-0.4, 1.2, _noise(cov.shape, seed + 1, 18.0))
    specks = smooth(2.4, 3.1, _noise(cov.shape, seed, 0.6)) * smooth(0.02, 0.25, lt.blur(cov, 7.0))
    return np.clip(a * 0.97 + dust + smudge + specks * 0.3, 0.0, 1.0)


def graphite(cov, pres, grain):
    """A soft pencil: dark grey, grainy where it was pressed lightly."""
    thr = 0.88 - 0.95 * np.clip(pres, 0.0, 1.1)
    return cov * smooth(thr - 0.2, thr + 0.2, grain) * 0.95


# --- Compositing ---

def multiply(color, alpha, ink_rgb):
    """Lays a transparent colour over the sheet (inks and stains darken what's under them)."""
    a = np.clip(alpha, 0.0, 1.0)[..., None]
    return color * (1.0 - a + a * np.asarray(ink_rgb, np.float32))


def letterpress(ink, grain_fine, fibers, seed, fade=0.9):
    """How the press laid the ink: uneven over the sheet, voids in solid areas, wood grain in the big type, the
    paper's fibres through it. Returns the ink's alpha."""
    shape = ink.cov.shape
    blotch = 0.84 + 0.16 * smooth(-1.3, 1.3, _noise(shape, seed, 28.0))
    solid = smooth(0.55, 0.95, lt.blur(ink.cov, 1.6))
    salt = smooth(1.5, 2.35, grain_fine) * solid
    grain = 0.55 * smooth(0.9, 2.0, _noise(shape, seed + 1, 46.0, 1.1)) + \
        0.35 * smooth(1.3, 2.3, _noise(shape, seed + 2, 14.0, 0.55))
    wood = np.clip(grain, 0.0, 1.0) * ink.wood
    edge = np.clip(ink.cov - lt.blur(ink.cov, 1.2), 0.0, 1.0)
    a = ink.cov * ink.dens * blotch * (1.0 - 0.8 * salt) * (1.0 - 0.45 * wood) * (1.0 + 0.1 * edge)
    a = a * (1.0 - 0.1 * np.clip(fibers, 0.0, 1.0))
    return np.clip(a * fade, 0.0, 1.0)


# --- Nails ---

NAIL_SHADOW = (0.3, 0.42)        # the head's shadow, offset in head radii (down and right)


def paint_nail(color, paper, heads, cx, cy, radius, seed, rust=1.0):
    """A hand-forged rosehead nail at (cx, cy) px: a dark iron head of five hammered facets with a glint, its
    shadow on the paper, a rust stain round it and a rust run below. The stains go only where paper is (its alpha);
    the head's coverage goes into heads."""
    rng = np.random.default_rng(seed)
    h, w = paper.shape
    reach = int(radius * 12)
    x0, x1 = max(int(cx - radius * 4), 0), min(int(cx + radius * 4) + 1, w)
    y0, y1 = max(int(cy - radius * 4), 0), min(int(cy + reach) + 1, h)
    gy, gx = np.mgrid[y0:y1, x0:x1].astype(np.float64) + 0.5
    dx, dy = gx - cx, gy - cy
    r = np.hypot(dx, dy)
    theta = np.arctan2(dy, dx)
    sub = color[y0:y1, x0:x1]
    on_paper = paper[y0:y1, x0:x1][..., None]
    # The rust run: a narrow wandering stain down from the head, and a halo round it.
    grid = np.arange(-reach - 4.0, reach + 4.0)
    wander = np.interp(dy, grid, noise1d(len(grid), rng, 9.0)) * radius * 0.25
    run_len = radius * rng.uniform(5.0, 9.0) * rust
    along = np.clip(dy / max(run_len, 1.0), 0.0, 1.5)
    half = radius * (0.35 + 0.3 * along)
    run = smooth(half + 0.8, half - 0.4, np.abs(dx - wander)) * (dy > 0) * np.clip(1.0 - along, 0.0, 1.0) ** 1.3
    halo = np.exp(-((r - radius * 1.25) / (radius * 0.55)) ** 2) * (0.6 + 0.4 * smooth(-1, 1, np.cos(theta * 3 + 1)))
    stain = np.clip(0.5 * run * rust + 0.35 * halo * rust, 0.0, 1.0)
    sub[...] = multiply(sub, stain * on_paper[..., 0], rgb(0x9a5a30))
    # The shadow, offset down-right and soft.
    sx, sy = cx + NAIL_SHADOW[0] * radius, cy + NAIL_SHADOW[1] * radius
    sr = np.hypot(gx - sx, gy - sy)
    shadow = smooth(radius * 1.25, radius * 0.75, sr) * 0.6
    sub[...] = multiply(sub, shadow * on_paper[..., 0], (0.25, 0.22, 0.2))
    # The head: an irregular disc of five facets rising to a point, lit from the upper left.
    wob = 1.0 + 0.07 * np.cos(theta * 5 + rng.uniform(0, 6.28)) + 0.04 * np.cos(theta * 3 + rng.uniform(0, 6.28))
    head = np.clip(radius * wob - r + 0.5, 0.0, 1.0)
    phase = rng.uniform(0.0, 2.0 * math.pi)
    k = np.floor(((theta - phase) % (2 * math.pi)) / (2 * math.pi / 5))
    mid = phase + (k + 0.5) * (2 * math.pi / 5)
    nx, ny, nz = np.cos(mid) * 0.55, np.sin(mid) * 0.55, 0.83
    light = np.array([-0.55, -0.62, 0.56])
    shade = np.clip(nx * light[0] + ny * light[1] + nz * light[2], 0.0, 1.0)
    shade = np.where(r < radius * 0.12, 0.8, shade)
    iron = mix(rgb(0x1d1b1a), rgb(0x5e5650), shade ** 1.5)
    iron = iron * (1.0 + 0.12 * _noise((y1 - y0, x1 - x0), seed + 5, 1.0)[..., None])
    pits = smooth(1.2, 2.0, _noise((y1 - y0, x1 - x0), seed + 6, 0.8))
    iron = mix(iron, rgb(0x6b3f22), pits * 0.5)
    rim = smooth(radius * 0.7, radius * 1.0, r) * 0.35
    iron = iron * (1.0 - rim)[..., None]
    gl = np.exp(-(((dx + radius * 0.32) / (radius * 0.2)) ** 2 + ((dy + radius * 0.36) / (radius * 0.14)) ** 2))
    iron = mix(iron, rgb(0xf1e8d6), np.clip(gl * 1.3, 0.0, 1.0))
    sub[...] = mix(sub, iron, head)
    a = heads[y0:y1, x0:x1]
    np.maximum(a, head.astype(np.float32), out=a)


# --- The poster ---

# Where the four nails go (mm from the sheet's top left), and how the bottom-right corner tore away: from the
# right edge at TEAR_A down to the bottom edge at TEAR_B, leaving a scrap under its nail.
NAILS = [(15.0, 14.0), (435.0, 15.5), (14.5, 664.0), (436.0, 666.0)]
TEAR_A = (450.0, 611.0)
TEAR_B = (391.0, 680.0)
CORNER_SCRAP = [(450.0, 649.0), (441.0, 651.0), (430.0, 655.0), (425.0, 667.0), (431.0, 680.0), (450.0, 680.0)]
CUT = [(0.28, 7.0), (0.14, 1.6)]                        # a machine-cut edge: (amplitude px, sigma samples)
TORN = [(2.0, 22.0), (1.1, 6.0), (0.5, 1.4)]           # a torn edge


def _edge(sheet, a, b, rng, octaves, bumps=(), step=0.6):
    pts = densify(sheet.px([a, b]), step)
    return ragged(pts, rng, octaves, bumps=bumps)


def poster_outline(sheet, seed):
    """The sheet's outline (px): machine-cut edges a hair uneven with a few nicks, the bottom-right corner torn
    away, and the scrap of it still under its nail. Returns (loops, torn polylines)."""
    rng = np.random.default_rng(seed)
    W, H = sheet.size
    # Clockwise in the image (y down): the left normal of each edge points into the sheet.
    top = _edge(sheet, (0.0, 0.0), (W, 0.0), rng, CUT, bumps=[(0.3, 1.6, 4.0), (0.71, 2.4, 5.0)])
    right = _edge(sheet, (W, 0.0), TEAR_A, rng, CUT, bumps=[(0.45, 1.8, 4.0)])
    tear = _edge(sheet, TEAR_A, TEAR_B, rng, TORN)
    bottom = _edge(sheet, TEAR_B, (0.0, H), rng, CUT, bumps=[(0.6, 2.0, 5.0)])
    left = _edge(sheet, (0.0, H), (0.0, 0.0), rng, CUT, bumps=[(0.38, 4.5, 6.0), (0.4, 2.5, 2.0), (0.8, 1.6, 3.0)])
    main = np.concatenate([top[:-1], right[:-1], tear[:-1], bottom[:-1], left[:-1]])
    # The corner scrap: torn on its inner side, cut along the two sheet edges.
    s = CORNER_SCRAP
    inner = ragged(densify(sheet.px(s[:5]), 0.6), rng, [(0.9, 6.0), (0.45, 1.4)])
    edges = densify(sheet.px([s[4], s[5], s[0]]), 0.6)
    scrap = np.concatenate([inner[:-1], edges[:-1]])
    return [main, scrap], [tear, inner]


def fringe(shape, torn, width=1.5, seed=0):
    """The light fibres along torn edges (0..1), on both sides of the line: mask it with the paper."""
    strokes = [(p, np.ones(len(p))) for p in torn]
    cov, _ = draw_strokes(shape, strokes, width * 2.0, var=0.0)
    return cov * (0.55 + 0.45 * smooth(-1.0, 1.0, _noise(shape, seed, 1.2)))


def poster_paper(sheet, mask, seed):
    """The poster's paper: sun-bleached cream, toned at the edges, water stains and rain runs, foxing, folds, wrinkles
    from the nails and grime. Returns (colour, maps)."""
    shape = sheet.shape
    rng = np.random.default_rng(seed)
    u, v = sheet.grid()
    W, H = sheet.size
    edge = np.minimum(np.minimum(u, W - u), np.minimum(v, H - v))
    big = _noise(shape, seed + 1, 70.0, octaves=2)
    mid = _noise(shape, seed + 2, 16.0)
    fine = _noise(shape, seed + 3, 0.6)
    fibers = 0.55 * _noise(shape, seed + 4, 3.5, 0.45) + 0.55 * _noise(shape, seed + 5, 0.45, 3.5) + \
        0.4 * _noise(shape, seed + 6, 2.2, 0.6)
    # The sheet: bleached toward the middle and the top (where the sun fell), toned yellow-brown at the edges.
    color = np.ones(shape + (3,), np.float32) * rgb(0xd9c8a3)
    sun = smooth(-0.9, 1.4, 0.6 * big - 0.002 * (v - 260.0)) * smooth(0.0, 40.0, edge)
    color = mix(color, rgb(0xe3d7bc), sun * 0.75)
    toned = (1.0 - smooth(0.0, 13.0, edge + 5.0 * mid)) * 0.75 + (1.0 - smooth(0.0, 45.0, edge)) * 0.2
    color = mix(color, rgb(0xbd9e6f), np.clip(toned, 0.0, 1.0))
    color = color * (1.0 + 0.022 * big + 0.012 * mid + 0.022 * fine + 0.014 * fibers)[..., None]
    # Water: two dried stains and rain runs from the top edge and the top nails.
    for center, radius, aspect, k in (((372.0, 70.0), 64.0, 1.6, 21), ((70.0, 560.0), 48.0, 1.3, 23),
                                     ((260.0, 690.0), 40.0, 0.7, 25)):
        inside, tide = tide_stain(u, v, center, radius, aspect, seed + k, shape)
        color = multiply(color, inside * 0.5, (0.95, 0.9, 0.78))
        color = mix(color, rgb(0x9a7b52), np.clip(tide, 0.0, 1.0) * 0.38)
    for x0, v0, length, k in ((15.0, 22.0, 150.0, 31), (300.0, 0.0, 95.0, 33), (435.0, 24.0, 120.0, 35),
                              (122.0, 0.0, 60.0, 37)):
        inside, tide = drip(u, v, x0, v0, length, seed + k)
        color = multiply(color, inside * 0.5, (0.93, 0.88, 0.76))
        color = mix(color, rgb(0x9a7b52), np.clip(tide, 0.0, 1.0) * 0.3)
    # Foxing: spots in clusters, thicker near the edges and in the stains.
    h, w = shape
    chance = (1.0 - smooth(0.0, 60.0, edge)) * 0.7 + 0.3 * smooth(0.6, 1.8, mid) + 0.05
    chance = (chance * mask).ravel()
    picks = rng.choice(h * w, size=150, replace=False, p=chance / chance.sum())
    pts = [(p % w + 0.5, p // w + 0.5) for p in picks]
    sizes = [float(rng.choice([0.6, 0.9, 1.4, 2.2], p=[0.45, 0.3, 0.17, 0.08])) for _ in picks]
    strengths = [float(rng.uniform(0.25, 0.8)) for _ in picks]
    cores, halos = spots(shape, rng, pts, sizes, strengths)
    color = multiply(color, halos * 0.35, rgb(0xc9a070))
    color = multiply(color, cores * 0.8, rgb(0xa06a3e))
    # Folds: carried folded in four. Wrinkles from the nails, where the sheet pulled.
    shade = np.zeros(shape, np.float32)
    cracks = np.zeros(shape, np.float32)
    for p0, p1, k, s in (((223.0, -10.0), (229.0, 690.0), 41, 1.0), ((-10.0, 346.0), (460.0, 342.0), 43, 0.85),
                         ((-10.0, 172.0), (460.0, 175.0), 45, 0.35)):
        sh, core = crease(u, v, p0, p1, seed + k, s)
        shade += sh
        cracks = np.maximum(cracks, core)
    for k, ((nx, ny), toward) in enumerate(zip(NAILS[:3], ((1.0, 1.4), (-1.0, 1.4), (1.0, -1.2)))):
        for j in range(3):
            a = math.atan2(toward[1], toward[0]) + rng.normal(0.0, 0.35)
            length = rng.uniform(35.0, 80.0)
            p0 = (nx + 7.0 * math.cos(a), ny + 7.0 * math.sin(a))
            p1 = (p0[0] + length * math.cos(a), p0[1] + length * math.sin(a))
            sh, core = crease(u, v, p0, p1, seed + 50 + 7 * k + j, rng.uniform(0.35, 0.6), fade_end=True)
            shade += sh
            cracks = np.maximum(cracks, core * 0.5)
    color = color * (1.0 + 0.09 * np.clip(shade, -1.5, 1.5))[..., None]
    color = multiply(color, cracks * 0.12, (0.6, 0.52, 0.42))
    # Grime splashed up the bottom and dust along the top.
    grime = (1.0 - smooth(0.0, 70.0, H - v)) * (0.5 + 0.5 * smooth(-1.0, 1.0, _noise(shape, seed + 60, 6.0)))
    grime += (1.0 - smooth(0.0, 18.0, v)) * 0.4
    color = multiply(color, grime * 0.22, rgb(0x8c7a62))
    maps = dict(fibers=fibers, fine=fine, cracks=cracks, tooth=tooth(shape, seed + 70), edge=edge)
    return np.clip(color, 0.0, 1.0), maps


# The poster's copy. The top and the bottom are the same on every option; the middle holds the option's portrait.
DUNNE_LINE = ('FOR ROBBERY OF STAGES, BANKS', '& RELIQUARIES WITH THE DUNNE GANG')
REWARD = '$500 REWARD'
SIGN_OFF = 'CAPT. R. CALDER, RIM RANGERS'
NAME_BASELINE = 473.0


def poster_print(sheet, option, seed):
    """Sets the poster's type and ornaments: (black ink, red ink)."""
    rng = np.random.default_rng(seed)
    black, red = Ink(sheet.shape), Ink(sheet.shape)
    W, H = sheet.size
    # The border: a heavy rule and a hairline inside it, square ornaments at the corners.
    for inset, thick in ((13.5, 2.6), (18.2, 1.1)):
        rule(black, sheet, inset - thick * 0.5, W - inset + thick * 0.5, inset, thick, rng)
        rule(black, sheet, inset - thick * 0.5, W - inset + thick * 0.5, H - inset, thick, rng)
        for x in (inset, W - inset):
            type_shape(black, sheet, [rect(x - thick * 0.5, inset, x + thick * 0.5, H - inset)])
    # The top: WANTED in big wood type, DEAD OR ALIVE between stars.
    type_line(black, sheet, rng, 'WANTED', 80.0, W * 0.5, 112.0, width=356.0, wood=True)
    rule(black, sheet, 52.0, W - 52.0, 124.0, 2.2, rng)
    rule(black, sheet, 52.0, W - 52.0, 128.8, 1.0, rng)
    type_line(black, sheet, rng, 'DEAD OR ALIVE', 24.0, W * 0.5, 158.0, width=280.0)
    for x in (48.0, W - 48.0):
        type_shape(black, sheet, [star(x, 146.0, 8.5)])
    # The middle: the option's portrait or type.
    middle = {'A': middle_type, 'B': middle_woodcut, 'C': middle_engraving}[option]
    middle(sheet, black, rng)
    # The name and what for.
    type_line(black, sheet, rng, 'ELLIS RANSOM', 42.0, W * 0.5, NAME_BASELINE, width=372.0, wood=True)
    type_line(black, sheet, rng, DUNNE_LINE[0], 12.5, W * 0.5, NAME_BASELINE + 26.0, width=330.0)
    type_line(black, sheet, rng, DUNNE_LINE[1], 12.5, W * 0.5, NAME_BASELINE + 46.0, width=360.0)
    rule(black, sheet, 150.0, W - 150.0, NAME_BASELINE + 60.0, 0.8, rng)
    for x in (W * 0.5 - 158.0, W * 0.5 + 158.0):
        type_shape(black, sheet, [diamond(x, NAME_BASELINE + 60.0, 4.0, 2.4)])
    # The reward in red.
    type_line(red, sheet, rng, REWARD, 46.0, W * 0.5, 591.0, width=360.0, wood=True)
    # Calder's printed name; she signs above it in ink.
    type_line(black, sheet, rng, SIGN_OFF, 11.5, W * 0.5 + 40.0, 648.0, width=250.0)
    return black, red


def middle_type(sheet, ink, rng):
    """Option A: type only, a classic broadside."""
    W = sheet.size[0]
    type_line(ink, sheet, rng, 'ARMED', 58.0, W * 0.5, 302.0, width=300.0, wood=True)
    type_line(ink, sheet, rng, '& DANGEROUS', 30.0, W * 0.5, 346.0, width=330.0, wood=True)
    rule(ink, sheet, 90.0, W - 90.0, 362.0, 1.6, rng)
    type_line(ink, sheet, rng, 'YOUNGEST OF THE GANG', 15.0, W * 0.5, 390.0, width=300.0)
    type_line(ink, sheet, rng, 'AND ITS BEST SHOT', 15.0, W * 0.5, 412.0, width=260.0)
    for x in (W * 0.5 - 150.0, W * 0.5 + 150.0):
        type_shape(ink, sheet, [diamond(x, 362.0, 4.0, 2.4)])


class Cut:
    """A picture cut in wood or engraved in metal, worked at ss x ss samples a pixel over a box of the sheet (mm).
    Shapes are polygons in the picture's own units (x right, y up from origin_mm, unit_mm to a unit); they're painted
    back to front, each covering what's under it with its own lines."""

    def __init__(self, sheet, box_mm, origin_mm, unit_mm, ss=3, seed=0, waver=0.08, swell=0.07):
        self.sheet, self.ss, self.unit = sheet, ss, unit_mm
        (ax, ay), (bx, by) = sheet.px(box_mm[:2]), sheet.px(box_mm[2:])
        self.box = (int(math.floor(ax)), int(math.floor(ay)), int(math.ceil(bx)), int(math.ceil(by)))
        x0, y0, x1, y1 = self.box
        self.shape = ((y1 - y0) * ss, (x1 - x0) * ss)
        gy, gx = np.mgrid[0:self.shape[0], 0:self.shape[1]].astype(np.float64)
        gx, gy = x0 + (gx + 0.5) / ss, y0 + (gy + 0.5) / ss
        self.u, self.v = (gx - sheet.ox) / MM, (gy - sheet.oy) / MM
        self.ox, self.oy = origin_mm
        self.X, self.Y = (self.u - self.ox) / unit_mm, (self.oy - self.v) / unit_mm
        self.ink = np.zeros(self.shape, np.float32)
        self.seed = seed
        # Hand-cut lines waver and swell a little (an engraver's burin less than a woodcutter's gouge).
        self.waver = _noise(self.shape, seed + 1, 7.0 * ss) * waver
        self.swell = _noise(self.shape, seed + 2, 5.0 * ss) * swell

    def mm(self, P):
        P = np.asarray(P, np.float64)
        return np.stack([self.ox + P[:, 0] * self.unit, self.oy - P[:, 1] * self.unit], axis=1)

    def mask(self, *polys):
        x0, y0, x1, y1 = self.box
        loops = [self.sheet.px(self.mm(p)) for p in polys]
        return fill(loops, (y1 - y0, x1 - x0), origin=(x0, y0), ss=self.ss, reduce=False)

    def dist(self, poly, reach=40.0):
        """Distance (mm) from every sample to a polyline in picture units (inf farther than reach from its box)."""
        P = self.mm(poly)
        out = np.full(self.shape, np.inf)
        lo, hi = P.min(axis=0) - reach, P.max(axis=0) + reach
        rows = np.where((self.v[:, 0] >= lo[1]) & (self.v[:, 0] <= hi[1]))[0]
        cols = np.where((self.u[0] >= lo[0]) & (self.u[0] <= hi[0]))[0]
        if len(rows) and len(cols):
            r0, r1, c0, c1 = rows[0], rows[-1] + 1, cols[0], cols[-1] + 1
            out[r0:r1, c0:c1] = polyline_distance(self.u[r0:r1, c0:c1], self.v[r0:r1, c0:c1], P)
        return out

    def lines(self, u, tone):
        """Parallel lines across u (in periods): ink where a line is, each as wide as tone (0..1) of its period."""
        u = np.where(np.isfinite(u), u, 0.0) + self.waver
        d = np.abs(u - np.round(u))
        return (d < np.clip(tone + self.swell, 0.0, 1.0) * 0.5).astype(np.float32)

    def paint(self, mask, ink):
        self.ink = self.ink * (1.0 - mask) + ink * mask

    def carve(self, poly, width, closed=False):
        """A white line cut along a polyline (width mm)."""
        P = np.vstack([poly, poly[:1]]) if closed else poly
        self.ink *= (self.dist(P, width + 2.0) > width * 0.5).astype(np.float32)

    def stroke(self, poly, width, closed=False):
        """A black line along a polyline (width mm)."""
        P = np.vstack([poly, poly[:1]]) if closed else poly
        self.ink = np.maximum(self.ink, (self.dist(P, width + 2.0) < width * 0.5).astype(np.float32))

    def print(self, ink, wood=0.0, dens=0.95):
        """Prints the picture into the sheet's ink."""
        x0, y0, x1, y1 = self.box
        ss = self.ss
        cov = self.ink.reshape(y1 - y0, ss, x1 - x0, ss).mean(axis=(1, 3))
        sub = ink.cov[y0:y1, x0:x1]
        ink.dens[y0:y1, x0:x1] = np.where(cov > sub, dens, ink.dens[y0:y1, x0:x1])
        np.maximum(sub, cov, out=sub)
        if wood:
            np.maximum(ink.wood[y0:y1, x0:x1], cov * wood, out=ink.wood[y0:y1, x0:x1])


def _mirror(points):
    p = np.array(points, np.float64)[::-1]
    p[:, 0] = -p[:, 0]
    return p


PORTRAIT = (125.0, 237.0, 325.0, 425.0)        # the portrait's frame on the sheet (mm)


def frame(ink, sheet, rng, box=PORTRAIT, heavy=2.8, gap=1.6, hair=0.9):
    """A picture frame: a heavy rule round it and a hairline inside."""
    x0, y0, x1, y1 = box
    for inset, t in ((0.0, heavy), (heavy + gap, hair)):
        a0, b0, a1, b1 = x0 + inset, y0 + inset, x1 - inset, y1 - inset
        type_shape(ink, sheet, [rect(a0, b0, a1, b0 + t), rect(a0, b1 - t, a1, b1), rect(a0, b0, a0 + t, b1),
                                rect(a1 - t, b0, a1, b1)])


def middle_woodcut(sheet, ink, rng):
    """Option B: a woodcut bust, the face lost in the hat brim's shadow and a bandana pulled up over the nose."""
    x0, y0, x1, y1 = PORTRAIT
    inner = (x0 + 5.5, y0 + 5.5, x1 - 5.5, y1 - 5.5)
    cut = Cut(sheet, inner, ((x0 + x1) * 0.5, inner[3]), 92.0, ss=3, seed=int(rng.integers(1 << 30)))
    X, Y, U = cut.X, cut.Y, cut.unit
    S = lambda *p, closed=False: spline(np.array(p, np.float64), per=8, closed=closed)
    # Back to front: the sky, the coat, the lapels and shirt, the head in shadow, the bandana, the hat.
    sky = 0.04 + 0.2 * smooth(-0.3, 1.0, X) + 0.08 * smooth(0.7, 0.0, Y) + 0.04 * _noise(cut.shape, 7, 12.0)
    cut.paint(np.ones(cut.shape, np.float32), cut.lines(cut.v / 4.0, sky))
    shoulders = S((-1.3, 0.05), (-1.1, 0.4), (-0.88, 0.55), (-0.62, 0.64), (-0.4, 0.71), (-0.22, 0.78), (0.0, 0.8),
                  (0.22, 0.78), (0.4, 0.71), (0.62, 0.64), (0.88, 0.55), (1.1, 0.4), (1.3, 0.05))
    coat = cut.mask(np.vstack([shoulders, [(1.3, -0.5), (-1.3, -0.5)]]))
    d = cut.dist(shoulders, 60.0)
    tone = np.where(X < 0.0, 0.04 + 0.026 * d, 0.55 + 0.06 * d)
    cut.paint(coat, cut.lines(d / 3.6 + 0.5, np.clip(tone, 0.0, 1.0)))
    for seam in (S((-0.86, 0.56), (-0.8, 0.3), (-0.79, 0.0)), S((0.86, 0.56), (0.8, 0.3), (0.79, 0.0))):
        cut.carve(seam, 1.0)
    # The lapels fold down from the neck to a V on the chest; the shirt shows in the V.
    cut.paint(cut.mask(np.array([(-0.2, 0.86), (0.2, 0.86), (0.0, 0.1)])), cut.lines(cut.u / 6.0, 0.08))
    lapel = S((-0.64, 0.63), (-0.5, 0.72), (-0.36, 0.83), (-0.25, 0.9), (-0.18, 0.84), (-0.13, 0.6), (-0.06, 0.32),
              (0.0, 0.1), (-0.12, 0.18), (-0.3, 0.4), (-0.48, 0.55), closed=True)
    for poly, base in ((lapel, 0.6), (_mirror(lapel), 0.8)):
        d = cut.dist(np.vstack([poly[-30:], poly[:1]]), 50.0)
        cut.paint(cut.mask(poly), cut.lines(d / 3.0 + 0.5, np.clip(base + 0.05 * d, 0.0, 1.0)))
        cut.carve(poly, 1.0, closed=True)
    # The head, the bandana and the hat are drawn in their own frame, a size up from the coat's.
    hx, hy, hz = 1.14, 1.12, 0.79
    Xh, Yh = X / hx, 0.85 + (Y - hz) / hy

    def H(*p, closed=False):
        q = S(*p, closed=closed)
        return np.stack([q[:, 0] * hx, hz + (q[:, 1] - 0.85) * hy], axis=1)
    head = H((-0.3, 1.0), (-0.315, 1.18), (-0.3, 1.36), (0.0, 1.44), (0.3, 1.36), (0.315, 1.18), (0.3, 1.0),
             (0.18, 0.86), (0.0, 0.82), (-0.18, 0.86), closed=True)
    ears = [H(*[tuple(p) for p in arc(s * 0.315, 1.17, 0.05, 0.085, 0.0, 330.0, step=30.0)], closed=True)
            for s in (-1.0, 1.0)]
    cut.paint(cut.mask(head, *ears), 1.0)
    bandana = H((-0.335, 1.095), (-0.18, 1.14), (0.0, 1.175), (0.18, 1.14), (0.335, 1.095), (0.33, 0.98),
                (0.34, 0.88), (0.43, 0.78), (0.4, 0.68), (0.27, 0.58), (0.12, 0.49), (0.0, 0.4), (-0.12, 0.49),
                (-0.27, 0.58), (-0.4, 0.68), (-0.43, 0.78), (-0.34, 0.88), (-0.33, 0.98), closed=True)
    q, r = Xh / 0.074, Yh / 0.074
    col = np.round(q)
    rs = r - 0.5 * (col % 2)
    dots = (np.hypot(q - col, rs - np.round(rs)) > 0.21).astype(np.float32)
    cut.paint(cut.mask(bandana), dots)
    for s in (-1.0, 1.0):
        for fold in (((0.06, 1.13), (0.19, 0.99), (0.33, 0.82)), ((0.03, 1.03), (0.12, 0.84), (0.26, 0.66)),
                     ((0.04, 0.88), (0.08, 0.7), (0.12, 0.54))):
            cut.carve(H(*[(s * x, y) for x, y in fold]), 0.9)
    cut.carve(bandana, 1.3, closed=True)
    crown = H((-0.355, 1.4), (-0.345, 1.58), (-0.31, 1.73), (-0.21, 1.84), (-0.09, 1.815), (0.0, 1.78),
              (0.09, 1.815), (0.21, 1.84), (0.31, 1.73), (0.345, 1.58), (0.355, 1.4))
    half = 0.355 - 0.045 * np.clip((Yh - 1.4) / 0.4, 0.0, 1.0)
    xc = Xh / half
    tone = 0.22 + 0.7 * smooth(-0.8, 0.9, xc) + 0.35 * np.exp(-(xc / 0.13) ** 2) * smooth(1.62, 1.8, Yh)
    hat = cut.mask(crown)
    cut.paint(hat, cut.lines(xc * 0.355 * hx * U / 3.5, np.clip(tone, 0.0, 1.0)))
    cut.paint(hat * ((Yh > 1.44) & (Yh < 1.53)), 1.0)
    cut.carve(H((-0.36, 1.535), (0.0, 1.53), (0.36, 1.535)), 0.8)
    cut.stroke(crown, 1.3)
    top = H((-0.88, 1.5), (-0.78, 1.455), (-0.56, 1.44), (-0.3, 1.437), (0.0, 1.433), (0.3, 1.437), (0.56, 1.44),
            (0.78, 1.455), (0.88, 1.5))
    under = H((0.88, 1.5), (0.82, 1.445), (0.66, 1.39), (0.38, 1.34), (0.0, 1.31), (-0.38, 1.34), (-0.66, 1.39),
              (-0.82, 1.445), (-0.88, 1.5))
    d = cut.dist(top, 30.0)
    cut.paint(cut.mask(np.vstack([top, under])), cut.lines(d / 2.8 + 0.5, np.clip(0.2 + 0.16 * d, 0.0, 1.0)))
    cut.stroke(top, 1.3)
    cut.carve(under, 1.2)
    cut.print(ink, wood=0.6)
    frame(ink, sheet, rng)


# A single-action revolver seen from its right side, muzzle to the right, in mm: x along the bore from the
# cylinder's back face, y up from the bore line.
GUN_OUTLINE = [
    (188, 8.5), (184, 8.5), (183, 13.5), (178, 13.5), (177, 8.5), (52, 8.5), (50, 12), (44, 17), (0, 18), (-6, 18),
    (-8, 20), (-14, 26), (-24, 33), (-34, 33.5), (-40, 30), (-36, 27), (-28, 25), (-20, 18), (-18, 12), (-22, 0),
    (-28, -20), (-37, -44), (-47, -68), (-56, -86), (-60, -95), (-57, -100), (-46, -102), (-35, -99), (-30, -88),
    (-24, -74), (-17, -60), (-8, -50), (-3, -52), (0, -60), (8, -65), (18, -64), (25, -58), (28, -48),
    (30, -40), (40, -35), (48, -30), (52, -24), (54, -21), (148, -21), (151, -18), (151, -10), (153, -8.5),
    (188, -8.5)]
GUN_GUARD_HOLE = [(2, -50), (4, -57), (10, -61), (18, -60), (23, -55), (25, -46), (23, -40), (6, -40), (2, -45)]
GUN_GUARD = [(-4, -50), (-3, -52), (0, -60), (8, -65), (18, -64), (25, -58), (28, -48), (30, -40), (26, -37),
             (4, -37), (-2, -42)]
GUN_TRIGGER = [(8, -40), (12, -40), (12, -47), (9, -55), (6, -57), (7, -52), (8, -46)]
GUN_GRIP = [(-21, -22), (-29, -40), (-39, -62), (-48, -80), (-52, -90), (-45, -96), (-37, -93), (-33, -83),
             (-26, -69), (-18, -57), (-11, -45), (-13, -30)]
GUN_CHECKER = [(-21, -33), (-29, -48), (-38, -66), (-45, -80), (-39, -87), (-34, -79), (-28, -67), (-21, -55),
               (-15, -43), (-16, -33)]
GUN_HAMMER = [(-6, 18), (-8, 20), (-14, 26), (-24, 33), (-34, 33.5), (-40, 30), (-36, 27), (-28, 25), (-20, 18),
              (-14, 10), (-6, 10)]
GUN_CYLINDER = [(1, -30), (40, -30), (41.5, -10), (40, 10), (1, 10), (-0.5, -10)]
GUN_GATE = [(-9, -18), (-1, -18), (-1, 2), (-9, 2)]
GUN_SCREWS = [(-31.0, -63.0, 3.0), (-3.0, -33.0, 2.2), (14.0, -33.0, 2.2), (31.0, -32.0, 2.2), (46.0, -10.0, 2.4),
              (141.0, -15.0, 3.8)]


def _area(loop):
    p = np.asarray(loop, np.float64)
    return 0.5 * float(np.sum(p[:, 0] * np.roll(p[:, 1], -1) - np.roll(p[:, 0], -1) * p[:, 1]))


def middle_engraving(sheet, ink, rng):
    """Option C: the outlaw's revolver, engraved like a gunsmith's catalogue plate, over a vignette."""
    W = sheet.size[0]
    center = (W * 0.5, 321.0)
    cut = Cut(sheet, (34.0, 240.0, W - 34.0, 404.0), center, 1.0, ss=3, seed=int(rng.integers(1 << 30)),
              waver=0.03, swell=0.03)
    X, Y = cut.X, cut.Y
    scale, a = 1.06, math.radians(4.0)
    mid = np.array([64.0, -34.0])
    cs, sn = math.cos(a), math.sin(a)
    gx = (X * cs + Y * sn) / scale + mid[0]
    gy = (-X * sn + Y * cs) / scale + mid[1]

    def G(points):
        p = (np.asarray(points, np.float64) - mid) * scale
        return np.stack([p[:, 0] * cs - p[:, 1] * sn, p[:, 0] * sn + p[:, 1] * cs], axis=1)

    def closed(poly):
        return np.vstack([poly, poly[:1]])

    def holed(outer, hole):
        return cut.mask(outer, hole if np.sign(_area(outer)) != np.sign(_area(hole)) else hole[::-1])

    # The vignette: level lines behind the gun inside an oval, each stopping a little short of the rim or running a
    # little past it, the way an engraver's vignette dissolves.
    row = np.floor(cut.v / 3.0).astype(np.int64)
    ends = np.random.default_rng(int(rng.integers(1 << 30))).uniform(-0.16, 0.06, 512)[row % 512]
    reach = np.hypot(X / (160.0 * (1.0 + ends)), (Y + 8.0) / 72.0)
    cut.paint(np.ones(cut.shape, np.float32), cut.lines(cut.v / 3.0, 0.3) * (reach < 1.0))
    outline = G(GUN_OUTLINE)
    hole = G(GUN_GUARD_HOLE)
    body = holed(outline, hole)
    cut.paint(body, 0.0)

    def tube(y0, y1):
        """Level lines on something round: dark on its shadowed underside, a bright band near its top."""
        t = np.clip((gy - y0) / (y1 - y0), 0.0, 1.0)
        return np.clip(0.9 - 1.05 * smooth(0.0, 0.72, t) + 0.55 * smooth(0.82, 1.0, t), 0.0, 1.0)
    # The frame first (flat steel, hatched on the diagonal, darker low down), then the round parts over it.
    cut.paint(body, cut.lines((gx + gy) / 3.0, np.clip(0.38 + 0.008 * (8.0 - gy), 0.25, 0.8)))
    cut.paint(cut.mask(G([(52, -9.5), (188, -9.5), (188, 9.5), (52, 9.5)])) * body,
              cut.lines(gy / 2.4, tube(-8.5, 8.5)))
    cut.paint(cut.mask(G([(53, -22), (152, -22), (152, -9), (53, -9)])) * body, cut.lines(gy / 2.2, tube(-21.0, -9.5)))
    cyl = G(spline(np.array(GUN_CYLINDER, np.float64), per=4, closed=True))
    flutes = sum(np.exp(-((gy - c) / 2.4) ** 2) for c in (-21.0, -9.0, 3.0)) * smooth(3.0, 9.0, gx) * \
        smooth(38.0, 32.0, gx)
    cut.paint(cut.mask(cyl), cut.lines(gy / 2.3, np.clip(tube(-30.0, 10.0) + 0.55 * flutes, 0.0, 1.0)))
    for c in (-21.0, -9.0, 3.0):
        cut.stroke(G([(6, c), (35, c)]), 0.7)
    gate = G(GUN_GATE)
    cut.paint(cut.mask(gate), cut.lines((gx - gy) / 2.4, 0.5))
    hammer = G(GUN_HAMMER)
    knurl = np.maximum(cut.lines((gx + gy) / 2.0, 0.5), cut.lines((gx - gy) / 2.0, 0.5))
    cut.paint(cut.mask(hammer), np.where(gx < -22.0, knurl, cut.lines((gx - gy) / 2.4, 0.62)))
    cut.paint(cut.mask(G(GUN_TRIGGER)), 0.85)
    guard = holed(G(GUN_GUARD), hole)
    rr = np.hypot((gx - 13.0) / 17.0, (gy + 47.0) / 16.0)
    cut.paint(guard * body, cut.lines(rr * 16.0 / 2.0, np.clip(0.2 + 0.55 * smooth(-50.0, -62.0, gy), 0.0, 1.0)))
    # The grip: walnut, checkered inside a smooth border, between the steel straps.
    grip = G(spline(np.array(GUN_GRIP, np.float64), per=4, closed=True))
    checker = G(spline(np.array(GUN_CHECKER, np.float64), per=4, closed=True))
    cut.paint(cut.mask(grip), cut.lines((gx * 0.25 - gy) / 2.6, 0.5))
    lattice = np.maximum(cut.lines((gx * 0.85 + gy * 0.5) / 2.3, 0.4), cut.lines((gx * 0.85 - gy * 0.5) / 2.3, 0.4))
    cut.paint(cut.mask(checker), lattice)
    for poly, width in ((grip, 1.0), (checker, 0.7), (cyl, 1.0), (gate, 0.8), (hammer, 0.9), (G(GUN_TRIGGER), 0.8)):
        cut.stroke(closed(poly), width)
    # Screws, the ejector's head, the parting lines and a firm outline round it all.
    for sx, sy, sr in GUN_SCREWS:
        c = G(arc(sx, sy, sr, sr, 0.0, 360.0, step=20.0))
        cut.paint(cut.mask(c), 0.0)
        cut.stroke(closed(c), 0.8)
        cut.stroke(G([(sx - sr * 0.7, sy + sr * 0.3), (sx + sr * 0.7, sy - sr * 0.3)]), 0.6)
    for line in ([(52, -8.5), (52, 8.5)], [(54, -9.5), (150, -9.5)], [(-14, 10), (-14, -30)], [(-12, -36), (52, -36)]):
        cut.stroke(G(line), 0.8)
    cut.stroke(closed(outline), 1.5)
    cut.stroke(closed(hole), 1.2)
    cut.print(ink, wood=0.0)
    type_line(ink, sheet, rng, 'THE BEST SHOT IN THE DUNNE GANG', 13.0, W * 0.5, 419.0, width=330.0)


ALREADY_AT = (226.0, 216.0)       # where the scrawl's baseline is centred (mm)


def already(sheet, grain, seed):
    """The charcoal's alpha: ALREADY written across under DEAD OR ALIVE, rising a little, and struck under."""
    hand = Hand(seed, slant=0.07, spacing=0.17, wobble=0.02, jitter=1.25)
    x, y = sheet.px(ALREADY_AT)
    cap = 42.0 * MM
    strokes, width = hand.line('ALREADY', x, y, cap, angle=3.0, sag=-1.5, anchor='center', grow=0.08)
    rng = np.random.default_rng(seed + 1)
    a = math.radians(3.0)
    half = width * 0.5
    start = (x - half * 0.8 * math.cos(a), y + 10.5 * MM + half * 0.8 * math.sin(a))
    end = (x + half * 1.02 * math.cos(a), y + 8.5 * MM - half * 1.02 * math.sin(a))
    mid = ((start[0] + end[0]) * 0.5, (start[1] + end[1]) * 0.5 + 1.5 * MM)
    hook = (end[0] - 7.0 * MM, end[1] - 4.0 * MM)
    under = spline(np.array([start, mid, end, hook]), per=24)
    strokes.append(hand_stroke(under, rng, cap, wobble=0.006, base=1.05, release=0.25, lift=0.45, start=0.9))
    cov, pres, prof = draw_strokes(sheet.shape, strokes, 8.4, var=0.35, rough=0.06, seed=seed + 2, nib=(60.0, 0.3),
                                   profile=True)
    return charcoal(cov, pres, prof, grain, seed + 3)


def poster(option, seed=1):
    """The poster cell for an option: dict(color (h, w, 3), alpha, mask (the paper), heads (the nails' heads), sheet,
    maps (the paper's grain and folds))."""
    sheet = Sheet('Poster', POSTER_MM)
    loops, torn = poster_outline(sheet, seed + 100)
    mask = fill(loops, sheet.shape, ss=4)
    color, maps = poster_paper(sheet, mask, seed + 200)
    black, red = poster_print(sheet, option, seed + 300)
    # The press: black, then red a hair out of register.
    a = letterpress(black, maps['fine'], maps['fibers'], seed + 400) * (1.0 - 0.75 * maps['cracks'])
    color = multiply(color, a, (0.17, 0.145, 0.13))
    shift = np.roll(np.roll(red.cov, 1, axis=1), -1, axis=0)
    red.cov, red.wood = shift, np.roll(np.roll(red.wood, 1, axis=1), -1, axis=0)
    a = letterpress(red, maps['fine'], maps['fibers'], seed + 410, fade=0.85) * (1.0 - 0.75 * maps['cracks'])
    color = multiply(color, a, (0.74, 0.3, 0.22))
    # ALREADY, scrawled in charcoal under DEAD OR ALIVE, and a quick line under it.
    color = multiply(color, already(sheet, maps['tooth'], seed + 500), (0.19, 0.18, 0.18))
    # Calder's signature in iron-gall ink above her printed name.
    sig = signature(sheet.ox + 246.0 * MM, sheet.oy + 633.0 * MM, 21.0 * MM, seed + 540, angle=4.0)
    cov, pres = draw_strokes(sheet.shape, sig, 2.0, var=0.55)
    color = multiply(color, cov * (0.82 + 0.18 * np.clip(pres, 0.0, 1.0)), (0.27, 0.19, 0.14))
    # Torn edges show their fibres.
    fr = fringe(sheet.shape, torn, seed=seed + 600) * mask
    color = mix(color, rgb(0xf2ecdf), fr * 0.6)
    heads = np.zeros(sheet.shape, np.float32)
    for k, (nx, ny) in enumerate(NAILS):
        cx, cy = sheet.px((nx, ny))
        paint_nail(color, mask, heads, cx, cy, 6.2 * MM, seed + 700 + k)
    return dict(color=np.clip(color, 0.0, 1.0), alpha=np.maximum(mask, heads), mask=mask, heads=heads, sheet=sheet,
                maps=maps)


# --- What's left on the wall, and what falls ---

# The pieces still nailed up after the poster is torn down (mm, reaching past the sheet's edges, which cut them):
# a strip along the top and down the left from the top-left nail, a corner under each of the other nails, and the
# scrap that was already all that held the bottom-right corner.
REMNANT_KEEP = [
    [(-6, -6), (178, -6), (173, 8), (151, 14), (126, 22), (102, 33), (82, 41), (66, 52), (52, 65), (41, 82),
     (31, 100), (22, 118), (12, 132), (-6, 141)],
    [(456, -6), (456, 90), (448, 82), (440, 70), (428, 60), (414, 52), (401, 44), (391, 36), (379, 28), (365, 20),
     (350, 12), (336, -6)],
    [(-6, 686), (-6, 515), (6, 528), (20, 546), (38, 560), (52, 578), (60, 600), (66, 622), (78, 640), (94, 652),
     (108, 664), (118, 676), (126, 686)],
]
REMNANT_CORNER = [(456, 643), (456, 686), (421, 686), (421, 660), (429, 648), (444, 643)]
# The scrap that falls: a chunk torn from round DEAD OR ALIVE and the scrawl under it (mm on the poster).
SCRAP = [(141, 129), (197, 124), (240, 134), (286, 125), (319, 141), (305, 172), (318, 206), (292, 231),
         (249, 220), (213, 233), (172, 225), (149, 204), (137, 177), (151, 153)]


def _torn_loop(sheet, points, rng):
    """A closed outline (mm) with every edge torn (px)."""
    return ragged(densify(sheet.px(points), 0.6, closed=True), rng, TORN)


def remnant(p, seed=31):
    """The torn remnant: the poster's own pixels where pieces stay under the nails, fresh fibres along the tears.
    Returns (color, alpha)."""
    sheet = p['sheet']
    rng = np.random.default_rng(seed)
    torn = [_torn_loop(sheet, keep, rng) for keep in REMNANT_KEEP]
    corner = sheet.px(REMNANT_CORNER)
    keep = fill(torn + [corner], sheet.shape, ss=4)
    mask = p['mask'] * keep
    color = p['color'].copy()
    fr = fringe(sheet.shape, [np.vstack([t, t[:1]]) for t in torn], width=1.6, seed=seed + 1) * mask
    color = mix(color, rgb(0xf3eee2), fr * 0.65)
    return color, np.maximum(mask, p['heads'])


def scrap(p, seed=41):
    """The scrap that tumbles down: a piece of the poster's own pixels, torn all round, centred in its cell.
    Returns (color, alpha)."""
    sheet = p['sheet']
    rng = np.random.default_rng(seed)
    loop = _torn_loop(sheet, SCRAP, rng)
    mask = fill([loop], sheet.shape, ss=4) * p['mask']
    color = mix(p['color'], rgb(0xf3eee2), fringe(sheet.shape, [np.vstack([loop, loop[:1]])], width=1.6,
                                                  seed=seed + 1) * mask * 0.65)
    x0, y0, x1, y1 = CELLS['Scrap']
    h, w = y1 - y0, x1 - x0
    bx0, by0 = int(np.floor(loop[:, 0].min())) - 2, int(np.floor(loop[:, 1].min())) - 2
    bx1, by1 = int(np.ceil(loop[:, 0].max())) + 3, int(np.ceil(loop[:, 1].max())) + 3
    ox, oy = (w - (bx1 - bx0)) // 2, (h - (by1 - by0)) // 2
    out_c = np.zeros((h, w, 3), np.float32)
    out_a = np.zeros((h, w), np.float32)
    out_c[oy:oy + by1 - by0, ox:ox + bx1 - bx0] = color[by0:by1, bx0:bx1]
    out_a[oy:oy + by1 - by0, ox:ox + bx1 - bx0] = mask[by0:by1, bx0:bx1]
    return out_c, out_a


def scrap_back(front, seed=43):
    """The scrap seen from behind (mirrored left to right): the bare back of the sheet, kept from the sun, the print
    and the stains showing faintly through. Returns (color, alpha)."""
    color, alpha = front
    color, alpha = color[:, ::-1], alpha[:, ::-1]
    shape = alpha.shape
    lum = color @ np.array([0.3, 0.55, 0.15], np.float32)
    through = np.clip(1.0 - lum / 0.8, 0.0, 1.0)
    back = np.ones(shape + (3,), np.float32) * rgb(0xd9c9a4)
    back = back * (1.0 + 0.02 * _noise(shape, seed, 30.0) + 0.02 * _noise(shape, seed + 1, 0.6))[..., None]
    back = multiply(back, lt.blur(through, 0.7) * 0.22, (0.55, 0.5, 0.45))
    return np.clip(back, 0.0, 1.0), alpha


# --- Calder's note ---

NOTE_LINES = ['RANGERS -', '3 CACHES LAID', 'ON THE REST:', '- UNDER THE', '  WINDMILL', '- SINK RIM',
              '- BLUFF PATH', 'TAKE WHAT', 'YOU NEED.']
NOTE_NAIL = (105.0, 10.0)


def note(seed=61):
    """Ranger Ruth Calder's note for the notice board: a page torn from a field book, folded in three, her hasty
    block capitals in pencil about the three Ranger caches, signed. Returns (color, alpha)."""
    sheet = Sheet('Note', NOTE_MM)
    shape = sheet.shape
    rng = np.random.default_rng(seed)
    W, H = sheet.size
    top = _edge(sheet, (0.0, 0.0), (W, 0.0), rng, [(1.2, 14.0), (0.6, 4.0), (0.35, 1.2)])
    right = _edge(sheet, (W, 0.0), (W, H), rng, CUT)
    bottom = _edge(sheet, (W, H), (0.0, H), rng, CUT, bumps=[(0.3, 1.5, 4.0)])
    left = _edge(sheet, (0.0, H), (0.0, 0.0), rng, CUT)
    mask = fill([np.concatenate([top[:-1], right[:-1], bottom[:-1], left[:-1]])], shape, ss=4)
    u, v = sheet.grid()
    edge = np.minimum(np.minimum(u, W - u), np.minimum(v, H - v))
    big = _noise(shape, seed + 1, 40.0)
    color = np.ones(shape + (3,), np.float32) * rgb(0xcfc4aa)
    color = color * (1.0 + 0.025 * big + 0.02 * _noise(shape, seed + 2, 0.6) +
                     0.012 * _noise(shape, seed + 3, 3.0, 0.5))[..., None]
    color = mix(color, rgb(0xb9a074), (1.0 - smooth(0.0, 10.0, edge + 3.0 * _noise(shape, seed + 4, 8.0))) * 0.65)
    # The field book's faint ruling and margin.
    rules = np.exp(-((np.mod(v - 21.0, 8.0) - 4.0) / 0.3) ** 2) * (v > 17.0)
    color = multiply(color, rules * 0.16, rgb(0x8ea2b8))
    color = multiply(color, np.exp(-((u - 18.0) / 0.35) ** 2) * 0.22, rgb(0xc07a70))
    # Folded in three to carry; a grubby thumb at the corner; a few spots.
    shade = np.zeros(shape, np.float32)
    for p0, p1, k in (((-5.0, 94.0), (215.0, 92.0), 71), ((-5.0, 187.0), (215.0, 189.0), 73)):
        sh, core = crease(u, v, p0, p1, seed + k, 0.9)
        shade += sh
    color = color * (1.0 + 0.1 * np.clip(shade, -1.5, 1.5))[..., None]
    thumb = np.exp(-(((u - 186.0) / 11.0) ** 2 + ((v - 255.0) / 14.0) ** 2)) * \
        (0.6 + 0.4 * smooth(-1.0, 1.0, _noise(shape, seed + 5, 1.5)))
    color = multiply(color, thumb * 0.25, rgb(0x8f8678))
    inside, tide = tide_stain(u, v, (30.0, 262.0), 26.0, 1.2, seed + 6, shape)
    color = multiply(color, inside * 0.4, (0.95, 0.91, 0.82))
    color = mix(color, rgb(0xa58a62), np.clip(tide, 0.0, 1.0) * 0.3)
    # Her hand: hasty block capitals in a soft pencil.
    hand = Hand(seed + 10, slant=0.08, spacing=0.19, wobble=0.014, jitter=1.0, narrow=0.9)
    strokes = []
    for k, text in enumerate(NOTE_LINES):
        x, y = sheet.px((14.0 + rng.normal(0.0, 1.0), 41.0 + 23.5 * k))
        lines, _ = hand.line(text, x, y, 16.0 * MM, angle=rng.normal(0.6, 0.6), sag=rng.normal(0.0, 0.6))
        strokes += lines
    strokes += signature(*sheet.px((96.0, 266.0)), 19.0 * MM, seed + 20, angle=6.0)
    grain = tooth(shape, seed + 30)
    cov, pres = draw_strokes(shape, strokes, 2.7, var=0.4)
    color = multiply(color, graphite(cov, pres, grain), (0.22, 0.22, 0.25))
    heads = np.zeros(shape, np.float32)
    paint_nail(color, mask, heads, *sheet.px(NOTE_NAIL), 5.0 * MM, seed + 40, rust=0.7)
    return np.clip(color, 0.0, 1.0), np.maximum(mask, heads)


# --- The atlas ---

def atlas(option):
    """The whole decal atlas for a poster option: (rgba uint8 (1024, 1024, 4), the cells' colour and alpha)."""
    p = poster(option)
    front = scrap(p)
    cells = {'Poster': (p['color'], p['alpha']), 'Remnant': remnant(p), 'Scrap': front,
             'ScrapBack': scrap_back(front), 'Note': note()}
    color = np.zeros((ATLAS, ATLAS, 3), np.float32)
    alpha = np.zeros((ATLAS, ATLAS), np.float32)
    for name, (c, a) in cells.items():
        x0, y0, x1, y1 = CELLS[name]
        color[y0:y1, x0:x1] = c
        alpha[y0:y1, x0:x1] = a
    # Transparent texels take the colour of the paper nearest them, so filtering at the edges never pulls in black.
    color = lt.fill_transparent(color, alpha)
    return np.concatenate([lt.to8(color), lt.to8(alpha)[..., None]], axis=-1), cells


# --- Writing the atlas ---

def atlas_path(option):
    return os.path.join(OUT_DIR, f'T_Posters_BC_{option}.png')


def sheet_label(img, text, x, y, cap, color=(0.86, 0.84, 0.8), align='left'):
    """Writes a label in Rye onto an image (float RGB, px; y is the baseline)."""
    glyphs, width = set_line(text, cap / MM)
    left = {'left': x, 'center': x - width * MM * 0.5, 'right': x - width * MM}[align]
    loops = [np.stack([left + l[:, 0] * MM, y - l[:, 1] * MM], axis=1) for g in glyphs for l in g]
    layer = np.zeros(img.shape[:2], np.float32)
    fill_into(layer, loops)
    img[...] = mix(img, np.asarray(color, np.float32), layer)


def atlas_sheet(rgba, option):
    """The atlas laid flat for the look sheet: the cells over a dark checker, outlined and named, with a legend."""
    a = rgba[..., 3:].astype(np.float32) / 255.0
    x, y = lt.coords((ATLAS, ATLAS))
    checker = np.where(((x // 16) + (y // 16)) % 2 == 0, 0.13, 0.16)[..., None] * np.ones(3, np.float32)
    img = rgba[..., :3].astype(np.float32) / 255.0 * a + checker * (1.0 - a)
    for k, (name, (x0, y0, x1, y1)) in enumerate(CELLS.items()):
        for sl in ((slice(y0, y0 + 1), slice(x0, x1)), (slice(y1 - 1, y1), slice(x0, x1)),
                   (slice(y0, y1), slice(x0, x0 + 1)), (slice(y0, y1), slice(x1 - 1, x1))):
            img[sl] = rgb(0xf2a64a)
        sheet_label(img, str(k + 1), x0 + 5, y0 + 17, 12, color=(0.95, 0.65, 0.29))
        sheet_label(img, f'{k + 1}  {CELL_NAMES[name]}', 786, 800 + 26 * k, 13, color=(0.78, 0.76, 0.72))
    sheet_label(img, f'OPTION {option}', 786, 762, 20, color=(0.95, 0.65, 0.29))
    return img


CELL_NAMES = {'Poster': 'POSTER', 'Remnant': 'REMNANT', 'Note': "CALDER'S NOTE", 'Scrap': 'SCRAP',
              'ScrapBack': 'SCRAP, BACK'}


def build(option, install=False):
    """Writes an option's atlas (and its flat look-sheet view) to the previews; install also writes it as the game's
    texture, Art/Textures/Posters/T_Posters_BC.png."""
    rgba, cells = atlas(option)
    lt.write_png(atlas_path(option), rgba)
    lt.write_png(os.path.join(OUT_DIR, f'Atlas_{option}.png'), lt.to8(atlas_sheet(rgba, option)))
    lt._log(f'posters: option {option} -> {atlas_path(option)}')
    if install:
        lt.write_png(FINAL_PATH, rgba)
        lt._log(f'posters: option {option} installed as {FINAL_PATH}')
    return rgba


# --- Renders: the decals on a log wall and on the notice board in golden late-afternoon light ---

WALL_Y = 1.5                 # the building fronts stand behind the boardwalk (Boardwalk.py's +y edge)
CABIN_FRONT = -2.8           # the log cabin's front logs (LogCabin.py: HY + R in front of its pivot)
POSTER_AT = (-2.24, 1.76)    # the poster's middle (x, z) on the cabin's front logs, left of its porch, clear of the
                             # shadow the corner's log ends throw
SHERIFF_X = 30.0             # the sheriff's office stands down the street, out of the cabin's views
BOARD_AT = (SHERIFF_X - 0.6, -2.45)       # the notice board, in the street in front of the sheriff's office
SUN = (-0.66, -0.72, 0.18)   # toward a low golden sun, from the left in front of the street, 10 degrees up


def _render_setup():
    import bpy
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    ee = scene.eevee
    ee.taa_render_samples = 96
    ee.use_shadows = True
    if hasattr(ee, 'use_raytracing'):
        ee.use_raytracing = True
        ee.ray_tracing_method = 'SCREEN'
    if hasattr(ee, 'use_fast_gi'):
        ee.use_fast_gi = True
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.view_settings.exposure = 0.0
    return scene


def _golden_world(sun):
    """A painted late-afternoon sky: warm haze low down, blue overhead, a glow round the sun."""
    import bpy
    from mathutils import Vector
    world = bpy.data.worlds.new('_PosterSky')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -1.0
    links.new(split.outputs['Z'], remap.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    stops = [(0.0, 0x6b6050), (0.47, 0x8e8270), (0.5, 0xe9d2a6), (0.53, 0xd9d6c4), (0.62, 0xa9bfd2), (1.0, 0x5d84b6)]
    elements = ramp.color_ramp.elements
    elements[0].position, elements[0].color = stops[0][0], lt.hex_color(stops[0][1])
    elements[1].position, elements[1].color = stops[-1][0], lt.hex_color(stops[-1][1])
    for position, color in stops[1:-1]:
        elements.new(position).color = lt.hex_color(color)
    color = ramp.outputs['Color']
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(coords.outputs['Generated'], dot.inputs[0])
    dot.inputs[1].default_value = Vector(sun).normalized()
    clamp = nodes.new('ShaderNodeMath')
    clamp.operation = 'MAXIMUM'
    links.new(dot.outputs['Value'], clamp.inputs[0])
    for glow, power, strength in ((0xffe0a8, 24.0, 0.9), (0xfff2d0, 600.0, 6.0)):
        pw = nodes.new('ShaderNodeMath')
        pw.operation = 'POWER'
        links.new(clamp.outputs['Value'], pw.inputs[0])
        pw.inputs[1].default_value = power
        mul = nodes.new('ShaderNodeMath')
        mul.operation = 'MULTIPLY'
        links.new(pw.outputs['Value'], mul.inputs[0])
        mul.inputs[1].default_value = strength
        add = nodes.new('ShaderNodeMix')
        add.data_type = 'RGBA'
        add.blend_type = 'ADD'
        links.new(mul.outputs['Value'], add.inputs['Factor'])
        links.new(color, add.inputs['A'])
        add.inputs['B'].default_value = lt.hex_color(glow)
        color = add.outputs['Result']
    links.new(color, bg.inputs['Color'])
    bg.inputs['Strength'].default_value = 1.0
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    return world


def _white_col(mesh):
    col = mesh.color_attributes.get('Col') or mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))


def _mesh_object(name, verts, faces):
    import bpy
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def _exec_models(path, calls):
    """Builds models from a model script without running the script itself: everything above its model list (what
    that builds on the way is hidden), then calls (key, function, args) of its builder functions. Returns the new
    top-level objects by key."""
    import bpy
    with open(path, encoding='utf-8') as file:
        source = file.read()
    head = source.split('\nmodels = [')[0]
    namespace = {'__name__': 'poster_set', '__file__': path}
    before = set(bpy.data.objects)
    exec(compile(head, path, 'exec'), namespace)
    for o in set(bpy.data.objects) - before:
        o.hide_render = True
    made = {}
    for key, function, args in calls:
        before = set(bpy.data.objects)
        obj = namespace[function](*args)
        for o in set(bpy.data.objects) - before:
            if o.name.startswith(('UCX_', 'SOCKET_')) or o.type != 'MESH':
                o.hide_render = True
        made[key] = obj
    return made


def _decal_material(name, image):
    """The stand-in for the decal: the atlas's colour, its alpha clipped at 0.5, matte paper."""
    import bpy
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes['Principled BSDF']
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = image
    tex.interpolation = 'Linear'
    links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    clip = nodes.new('ShaderNodeMath')
    clip.operation = 'GREATER_THAN'
    clip.inputs[1].default_value = 0.5
    links.new(tex.outputs['Alpha'], clip.inputs[0])
    links.new(clip.outputs['Value'], bsdf.inputs['Alpha'])
    bsdf.inputs['Roughness'].default_value = 0.86
    bsdf.inputs['Specular IOR Level'].default_value = 0.3
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'DITHERED'
    return mat


def _decal(name, cell, mat, center, facing='-y', lift=0.0012, turn=0.0):
    """A plane showing one atlas cell at its real size, its middle at center (x, y, z), facing -y or +x."""
    import bpy
    x0, y0, x1, y1 = CELLS[cell]
    w, h = (x1 - x0) / PX_PER_M, (y1 - y0) / PX_PER_M
    c, s = math.cos(math.radians(turn)), math.sin(math.radians(turn))
    corners = []
    for dx, dz in ((-w / 2, -h / 2), (w / 2, -h / 2), (w / 2, h / 2), (-w / 2, h / 2)):
        rx, rz = dx * c - dz * s, dx * s + dz * c
        corners.append((center[0] + rx, center[1] - lift, center[2] + rz))
    obj = _mesh_object(name, corners, [(0, 1, 2, 3)])
    uv = obj.data.uv_layers.new(name='UVMap')
    us = [(x0 / ATLAS, 1.0 - y1 / ATLAS), (x1 / ATLAS, 1.0 - y1 / ATLAS), (x1 / ATLAS, 1.0 - y0 / ATLAS),
          (x0 / ATLAS, 1.0 - y0 / ATLAS)]
    for loop, value in zip(obj.data.loops, us):
        uv.data[loop.index].uv = value
    obj.data.materials.append(mat)
    obj.visible_shadow = False          # a decal casts no shadow
    _white_col(obj.data)
    return obj


def _camera(name, location, target, lens, sensor_fit='AUTO'):
    import bpy
    from mathutils import Vector
    cam = bpy.data.objects.new(name, bpy.data.cameras.new(name))
    bpy.context.scene.collection.objects.link(cam)
    cam.location = Vector(location)
    cam.rotation_euler = (Vector(target) - Vector(location)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens = lens
    cam.data.sensor_fit = sensor_fit
    cam.data.clip_start, cam.data.clip_end = 0.02, 2000.0
    return cam


def _render(scene, cam, path, size):
    import bpy
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    lt._log(f'posters: rendered {path}')
    return path


def _pixels(path):
    import bpy
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = 'Non-Color'
    w, h = img.size
    px = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return px.reshape(h, w, 4)[::-1, :, :3].copy()


def build_set():
    """The set, from the real models: the log cabin of Main Street's side lots (LogCabin.py), whose front logs take
    the poster; down the street the sheriff's office (FalseFronts.py) behind its boardwalk, a hitch rail and the Rim
    Rangers' notice board (Boardwalk.py); the street's dirt, a golden sky and a low sun."""
    import bpy
    from mathutils import Vector
    scene = _render_setup()
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj)
    scene.world = _golden_world(SUN)
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy, sun.data.color, sun.data.angle = 4.4, (1.0, 0.8, 0.56), math.radians(2.0)
    sun.rotation_euler = (-Vector(SUN).normalized()).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    ground = _mesh_object('_Ground', [(-80, -80, 0), (80, -80, 0), (80, 80, 0), (-80, 80, 0)], [(0, 1, 2, 3)])
    lt.assign(ground, lt.material('GroundDirt'))
    ground.data.uv_layers.new(name='UVMap')
    lt.box_uv(ground, 'GroundDirt')
    _white_col(ground.data)
    made = {'Cabin': _exec_script(os.path.join(REPO, 'Art', 'Models', 'Buildings', 'LogCabin.py'), 'LogCabin')}
    fronts = os.path.join(REPO, 'Art', 'Models', 'Buildings', 'FalseFronts.py')
    made.update(_exec_models(fronts, [('Sheriff', 'sheriff', ())]))
    # The facade's front is 5 m in front of the sheriff's office's pivot; the boardwalk runs along it.
    made['Sheriff'].location = (SHERIFF_X, WALL_Y + 5.0, 0.0)
    walk = os.path.join(REPO, 'Art', 'Models', 'Props', 'Boardwalk.py')
    calls = [(f'Walk{k}', 'boardwalk', (f'Boardwalk_4m_{k}', 4.0, 501 + k)) for k in range(4)]
    calls += [('NoticeBoard', 'notice_board', ('NoticeBoard', 509)), ('HitchRail', 'hitch_rail', ('HitchRail', 508))]
    made.update(_exec_models(walk, calls))
    for k in range(4):
        made[f'Walk{k}'].location = (SHERIFF_X - 8.0 + 4.0 * k, 0.0, 0.0)
    made['NoticeBoard'].location = (BOARD_AT[0], BOARD_AT[1], 0.0)
    made['HitchRail'].location = (SHERIFF_X + 2.4, -2.0, 0.0)
    return scene, made


def _exec_script(path, name):
    """Runs a model script that builds one model at its top level (as LogCabin.py does); returns the model."""
    import bpy
    before = set(bpy.data.objects)
    saved = sys.argv[:]
    sys.argv = [sys.argv[0], '--']           # no --preview: just the model
    try:
        with open(path, encoding='utf-8') as file:
            exec(compile(file.read(), path, 'exec'), {'__name__': 'poster_set', '__file__': path})
    finally:
        sys.argv = saved
    for o in set(bpy.data.objects) - before:
        if o.name.startswith(('UCX_', 'SOCKET_')) or o.type != 'MESH':
            o.hide_render = True
    return bpy.data.objects[name]


def render_all(options):
    """Renders every option's poster on the wall at about 1 m and 5 m, on the notice board with Calder's note, the
    torn remnant, the note up close, and A, B and C side by side."""
    import bpy
    scene, made = build_set()
    mats = {}
    for option in options:
        image = bpy.data.images.load(atlas_path(option), check_existing=False)
        image.colorspace_settings.name = 'sRGB'
        image.alpha_mode = 'STRAIGHT'
        mats[option] = _decal_material(f'_Posters_{option}', image)
    first = mats[options[0]]
    px, pz = POSTER_AT
    poster = _decal('_Poster', 'Poster', first, (px, CABIN_FRONT, pz), lift=0.004, turn=-0.6)
    remnant = _decal('_Remnant', 'Remnant', first, (px, CABIN_FRONT, pz), lift=0.004, turn=-0.6)
    remnant.hide_render = True
    # On the notice board (its face is SOCKET_Decal: 0.081 m in front of the posts, 1.36 m up): the poster on the
    # left, Calder's note on the right.
    bx, by = BOARD_AT
    board_poster = _decal('_BoardPoster', 'Poster', first, (bx - 0.3, by - 0.081, 1.36), turn=-0.8)
    _decal('_BoardNote', 'Note', first, (bx + 0.34, by - 0.081, 1.44), turn=1.5)
    near = _camera('_Near', (px + 0.16, CABIN_FRONT - 1.0, 1.65), (px, CABIN_FRONT, pz - 0.03), 23.5, 'VERTICAL')
    far = _camera('_Far', (px - 1.9, CABIN_FRONT - 4.6, 1.65), (px + 0.9, CABIN_FRONT, pz - 0.1), 18.0,
                  'HORIZONTAL')
    board = _camera('_Board', (bx - 0.6, by - 3.2, 1.65), (bx, by, 1.42), 22.0, 'VERTICAL')
    note_cam = _camera('_Note', (bx + 0.36, by - 0.6, 1.56), (bx + 0.34, by - 0.08, 1.44), 28.0, 'VERTICAL')
    os.makedirs(OUT_DIR, exist_ok=True)
    near_shots = []
    for option in options:
        poster.data.materials[0] = mats[option]
        board_poster.data.materials[0] = mats[option]
        near_shots.append(_render(scene, near, os.path.join(OUT_DIR, f'Poster_{option}_1m.png'), (1080, 1350)))
        _render(scene, far, os.path.join(OUT_DIR, f'Poster_{option}_5m.png'), (1920, 1080))
        _render(scene, board, os.path.join(OUT_DIR, f'NoticeBoard_{option}.png'), (1080, 1350))
    poster.hide_render = True
    remnant.hide_render = False
    remnant.data.materials[0] = mats[options[0]]
    _render(scene, near, os.path.join(OUT_DIR, 'Remnant_1m.png'), (1080, 1350))
    _render(scene, far, os.path.join(OUT_DIR, 'Remnant_5m.png'), (1920, 1080))
    _render(scene, note_cam, os.path.join(OUT_DIR, 'Note_close.png'), (1080, 1350))
    if len(near_shots) > 1:
        panels = [_pixels(p) for p in near_shots]
        gap = np.full((panels[0].shape[0], 12, 3), 0.1, np.float32)
        row = np.concatenate([x for p in panels for x in (p, gap)][:-1], axis=1)
        for k, option in enumerate(options):
            sheet_label(row, option, k * (panels[0].shape[1] + 12) + 34, 92, 56, color=(0.95, 0.65, 0.29))
        lt.write_png(os.path.join(OUT_DIR, 'Compare_ABC_1m.png'), lt.to8(np.clip(row, 0.0, 1.0)))


def main():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    options = [a.upper() for a in argv if a.upper() in OPTIONS] or list(OPTIONS)
    install = '--install' in argv
    if install and len(options) != 1:
        raise SystemExit('--install takes exactly one option: the one the user picked (A, B or C)')
    for option in options:
        build(option, install=install)
    if '--no-render' not in argv and not install:
        render_all(options)


if __name__ == '__main__':
    main()
