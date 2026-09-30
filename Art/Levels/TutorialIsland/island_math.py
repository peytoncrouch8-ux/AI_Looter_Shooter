"""Math for the tutorial island's generator (numpy only, so it runs in Blender's Python): noise, rasters over the map
square, curves and distance fields. island_shape.py builds the island with it.

Everything works in "layout meters": x = X / 100 (north), y = Y / 100 (east), heights in meters above the meadow base.
The rasters cover the square the macro color map covers: 204.8 m on a side, centered on the world origin (Unreal X and
Y from -10240 to 10240 cm); raster cell [i, j] is at x = c[i], y = c[j].
"""
import math

import numpy as np

MAP_HALF = 102.4      # meters: the rasters and the macro map cover -MAP_HALF..MAP_HALF on both axes
RASTER = 2048         # the shape raster: 10 cm cells


# --- Noise ---

_GRADIENTS = np.array([[math.cos(a), math.sin(a)] for a in np.linspace(0.0, 2.0 * math.pi, 16, endpoint=False)])


def _perm(seed):
    p = np.random.default_rng(seed).permutation(256)
    return np.concatenate([p, p]).astype(np.int64)


def gradient_noise(x, y, seed=0):
    """2D gradient (Perlin) noise with unit wavelength, about -1..1, for arrays of any shape."""
    x = np.asarray(x, dtype=np.float64)
    y = np.asarray(y, dtype=np.float64)
    perm = _perm(seed)
    out = np.empty(np.broadcast(x, y).shape, dtype=np.float64)
    fx, fy, fo = np.broadcast_to(x, out.shape).ravel(), np.broadcast_to(y, out.shape).ravel(), out.ravel()
    step = 1 << 21
    for a in range(0, fo.size, step):
        px, py = fx[a:a + step], fy[a:a + step]
        x0, y0 = np.floor(px), np.floor(py)
        dx, dy = px - x0, py - y0
        xi, yi = x0.astype(np.int64) & 255, y0.astype(np.int64) & 255

        def corner(ox, oy):
            g = _GRADIENTS[perm[perm[xi + ox] + yi + oy] & 15]
            return g[:, 0] * (dx - ox) + g[:, 1] * (dy - oy)
        u = dx * dx * dx * (dx * (dx * 6.0 - 15.0) + 10.0)
        v = dy * dy * dy * (dy * (dy * 6.0 - 15.0) + 10.0)
        a0 = corner(0, 0) + u * (corner(1, 0) - corner(0, 0))
        a1 = corner(0, 1) + u * (corner(1, 1) - corner(0, 1))
        fo[a:a + step] = (a0 + v * (a1 - a0)) * 1.4
    return out


def fbm(x, y, wavelength, seed=0, octaves=3, gain=0.5):
    """Fractal noise: octaves of gradient noise, the first at this wavelength (m); about -1..1."""
    total = np.zeros(np.broadcast(x, y).shape)
    amp, freq, norm = 1.0, 1.0 / wavelength, 0.0
    for o in range(octaves):
        total += amp * gradient_noise(x * freq, y * freq, seed + 101 * o)
        norm += amp
        amp *= gain
        freq *= 2.03
    return total / norm


def fbm_raster(grid, wavelength, seed=0, octaves=3, gain=0.5):
    """fbm() over a whole grid, evaluated on a coarser raster (eight samples per wavelength of the finest octave)
    and resampled: the same field, much faster for long wavelengths."""
    finest = wavelength / 2.03 ** (octaves - 1)
    n = int(min(grid.n, max(64, 2 ** math.ceil(math.log2(2.0 * grid.half * 8.0 / finest)))))
    coarse = Grid(n, grid.half)
    x, y = coarse.mesh()
    field = fbm(x, y, wavelength, seed, octaves, gain).astype(np.float32)
    return field if n == grid.n else resize(field, grid.n, grid.half)


# --- Rasters ---

class Grid:
    """A square raster over the map square: cell (i, j) is at x (north) = xs[i], y (east) = ys[j]."""

    def __init__(self, n=RASTER, half=MAP_HALF):
        self.n, self.half = n, half
        self.px = 2.0 * half / n
        self.c = -half + (np.arange(n) + 0.5) * self.px

    def mesh(self, i0=0, i1=None, j0=0, j1=None):
        i1 = self.n if i1 is None else i1
        j1 = self.n if j1 is None else j1
        return np.meshgrid(self.c[i0:i1], self.c[j0:j1], indexing='ij')

    def index_range(self, lo, hi):
        a = int(max(0, math.floor((lo + self.half) / self.px)))
        b = int(min(self.n, math.ceil((hi + self.half) / self.px) + 1))
        return a, b


def sample(raster, x, y, half=MAP_HALF):
    """Bilinear sample of a raster over the map square at points (x north, y east) in meters."""
    n = raster.shape[0]
    px = 2.0 * half / n
    fi = np.clip((np.asarray(x) + half) / px - 0.5, 0.0, n - 1.000001)
    fj = np.clip((np.asarray(y) + half) / px - 0.5, 0.0, n - 1.000001)
    i0, j0 = np.floor(fi).astype(np.int64), np.floor(fj).astype(np.int64)
    ti, tj = fi - i0, fj - j0
    i1, j1 = np.minimum(i0 + 1, n - 1), np.minimum(j0 + 1, n - 1)
    top = raster[i0, j0] * (1 - tj) + raster[i0, j1] * tj
    bottom = raster[i1, j0] * (1 - tj) + raster[i1, j1] * tj
    return top * (1 - ti) + bottom * ti


def resize(raster, n, half=MAP_HALF):
    """A raster resampled (bilinear) to n x n over the same square."""
    grid = Grid(n, half)
    out = np.empty((n, n), dtype=np.float32)
    for a in range(0, n, 256):
        x, y = grid.mesh(a, min(n, a + 256))
        out[a:a + 256] = sample(raster, x, y, half)
    return out


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def smin(a, b, k):
    """Smooth minimum (polynomial), blending within k."""
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0.0, 1.0)
    return b + (a - b) * h - k * h * (1.0 - h)


def smax(a, b, k):
    return -smin(-a, -b, k)


# --- Curves ---

def catmull_rom(points, closed=False, step=0.5):
    """A centripetal Catmull-Rom curve through the points (N, 2), resampled every step meters."""
    p = np.asarray(points, dtype=np.float64)
    if closed:
        ext = np.vstack([p[-1:], p, p[:2]])
    else:
        ext = np.vstack([2 * p[0] - p[1], p, 2 * p[-1] - p[-2]])
    dense = []
    for k in range(1, len(ext) - 2):
        p0, p1, p2, p3 = ext[k - 1], ext[k], ext[k + 1], ext[k + 2]
        t0 = 0.0
        t1 = t0 + max(np.linalg.norm(p1 - p0), 1e-6) ** 0.5
        t2 = t1 + max(np.linalg.norm(p2 - p1), 1e-6) ** 0.5
        t3 = t2 + max(np.linalg.norm(p3 - p2), 1e-6) ** 0.5
        count = max(8, int(np.linalg.norm(p2 - p1) / 0.1))
        t = np.linspace(t1, t2, count, endpoint=False)[:, None]
        a1 = (t1 - t) / (t1 - t0) * p0 + (t - t0) / (t1 - t0) * p1
        a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2
        a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3
        b1 = (t2 - t) / (t2 - t0) * a1 + (t - t0) / (t2 - t0) * a2
        b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3
        dense.append((t2 - t) / (t2 - t1) * b1 + (t - t1) / (t2 - t1) * b2)
    dense = np.vstack(dense + ([p[:1]] if closed else [p[-1:]]))
    return resample(dense, step, closed)


def resample(poly, step, closed=False):
    """A polyline resampled at even spacing (about step meters)."""
    pts = np.vstack([poly, poly[:1]]) if closed else poly
    seg = np.linalg.norm(np.diff(pts, axis=0), axis=1)
    s = np.concatenate([[0.0], np.cumsum(seg)])
    count = max(2, int(round(s[-1] / step)))
    targets = np.linspace(0.0, s[-1], count, endpoint=not closed) if closed else np.linspace(0.0, s[-1], count + 1)
    out = np.column_stack([np.interp(targets, s, pts[:, 0]), np.interp(targets, s, pts[:, 1])])
    return out


def arc_length(poly, closed=False):
    pts = np.vstack([poly, poly[:1]]) if closed else poly
    return np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))])


def polyline_field(grid, poly, margin, closed=False):
    """Distance to a polyline (meters), the arc length of the closest point and the side (+1 left of the direction of
    travel, -1 right), on grid cells within margin of it. Farther cells get distance = inf."""
    n = grid.n
    dist = np.full((n, n), np.inf, dtype=np.float32)
    along = np.zeros((n, n), dtype=np.float32)
    side = np.zeros((n, n), dtype=np.int8)
    pts = np.vstack([poly, poly[:1]]) if closed else poly
    s = arc_length(poly, closed)
    for k in range(len(pts) - 1):
        a, b = pts[k], pts[k + 1]
        i0, i1 = grid.index_range(min(a[0], b[0]) - margin, max(a[0], b[0]) + margin)
        j0, j1 = grid.index_range(min(a[1], b[1]) - margin, max(a[1], b[1]) + margin)
        if i0 >= i1 or j0 >= j1:
            continue
        x, y = grid.mesh(i0, i1, j0, j1)
        d = b - a
        length2 = max(d @ d, 1e-12)
        t = np.clip(((x - a[0]) * d[0] + (y - a[1]) * d[1]) / length2, 0.0, 1.0)
        dd = np.hypot(x - (a[0] + t * d[0]), y - (a[1] + t * d[1]))
        window = dist[i0:i1, j0:j1]
        better = dd < window
        window[better] = dd[better]
        along[i0:i1, j0:j1][better] = (s[k] + t * math.sqrt(length2))[better]
        # Left of travel: cross(d, p - a) > 0 (x north, y east, so left is toward -y when heading north... the sign is
        # only used consistently by callers).
        cross = d[0] * (y - a[1]) - d[1] * (x - a[0])
        side[i0:i1, j0:j1][better] = np.where(cross[better] >= 0.0, 1, -1)
    dist[dist > margin] = np.inf
    return dist, along, side


def polygon_mask(grid, poly):
    """Cells inside a closed polygon (even-odd scanline fill)."""
    mask = np.zeros((grid.n, grid.n), dtype=bool)
    a = np.asarray(poly, dtype=np.float64)
    b = np.roll(a, -1, axis=0)
    for i, x in enumerate(grid.c):
        cross = (a[:, 0] <= x) != (b[:, 0] <= x)
        if not cross.any():
            continue
        ea, eb = a[cross], b[cross]
        t = (x - ea[:, 0]) / (eb[:, 0] - ea[:, 0])
        ys = np.sort(ea[:, 1] + t * (eb[:, 1] - ea[:, 1]))
        for k in range(0, len(ys) - 1, 2):
            j0 = int(max(0, math.ceil((ys[k] + grid.half) / grid.px - 0.5)))
            j1 = int(min(grid.n - 1, math.floor((ys[k + 1] + grid.half) / grid.px - 0.5)))
            if j1 >= j0:
                mask[i, j0:j1 + 1] = True
    return mask


def points_in_polygon(x, y, poly):
    """Even-odd test for points (arrays) against a closed polygon (N, 2)."""
    inside = np.zeros(np.shape(x), dtype=bool)
    a = np.asarray(poly, dtype=np.float64)
    b = np.roll(a, -1, axis=0)
    for (ax, ay), (bx, by) in zip(a, b):
        cond = (ax <= x) != (bx <= x)
        t = (x - ax) / np.where(bx - ax == 0.0, 1e-12, bx - ax)
        inside ^= cond & (y < ay + t * (by - ay))
    return inside


def signed_distance(grid, poly, margin):
    """Signed distance to a closed polygon, negative inside, clamped to +-margin."""
    dist, _, _ = polyline_field(grid, poly, margin, closed=True)
    dist = np.minimum(dist, margin)
    return np.where(polygon_mask(grid, poly), -dist, dist).astype(np.float32)


def normals_of(poly, closed=False):
    """Unit left normals of a polyline's points (x north, y east): the direction rotated a quarter turn."""
    if closed:
        d = np.roll(poly, -1, axis=0) - np.roll(poly, 1, axis=0)
    else:
        d = np.gradient(poly, axis=0)
    d /= np.maximum(np.linalg.norm(d, axis=1, keepdims=True), 1e-9)
    return np.column_stack([-d[:, 1], d[:, 0]])


def polygon_area(poly):
    x, y = poly[:, 0], poly[:, 1]
    return 0.5 * float(np.sum(x * np.roll(y, -1) - np.roll(x, -1) * y))


def gauss_smooth_1d(values, sigma_samples):
    if sigma_samples <= 0:
        return values
    radius = int(3 * sigma_samples) + 1
    k = np.exp(-0.5 * (np.arange(-radius, radius + 1) / sigma_samples) ** 2)
    k /= k.sum()
    padded = np.concatenate([np.full(radius, values[0]), values, np.full(radius, values[-1])])
    return np.convolve(padded, k, mode='valid')


def blur(raster, radius_px):
    """A cheap separable box blur (three passes approximate a Gaussian)."""
    out = raster.astype(np.float32)
    r = int(max(1, radius_px))
    for _ in range(3):
        for axis in (0, 1):
            c = np.cumsum(np.pad(out, [(r + 1, r) if a == axis else (0, 0) for a in (0, 1)], mode='edge'), axis=axis,
                          dtype=np.float64)
            hi = np.take(c, np.arange(2 * r + 1, c.shape[axis]), axis=axis)
            lo = np.take(c, np.arange(0, c.shape[axis] - 2 * r - 1), axis=axis)
            out = ((hi - lo) / (2 * r + 1)).astype(np.float32)
    return out
