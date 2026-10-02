"""Freestanding rock built as a solid, for the outcrop kit (Art/Models/Rocks/Outcrops.py). A scripted-model helper (see
Art/README.md, looter_model.py and looter_textures.py).

The cliff kit (Cliffs.py) shapes a face as a height field over its front. An outcrop is seen from every side, so it is
built as a solid instead: a signed distance field (negative inside the rock) made of blocks, the beds and the columns
between joints, each a rounded convex prism; weathered by partings and noise; meshed by surface nets on a fine grid with
each vertex pulled onto the surface; then reduced to its Nanite source budget and finished like the cliff kit: RockCliff
box-mapped at the cliff kit's scale, smooth shading, baked vertex AO, convex hulls.

    import looter_outcrops as lo
    rock = lo.Rock(seed=11, sink=0.9)
    rock.add(lo.Block(lo.rect(3.0, 2.4), z0=-0.9, z1=1.8, yaw=12.0))
    rock.parting(1.8, depth=0.35)
    obj = lo.build('Outcrop_TorA', rock, cell=0.075, source=9000, fallback=30, uv_seed=3)
    lo.column_hulls(obj, [(-1.5, 0.0), (1.5, 0.0)])

  Rock          the field: blocks joined by a smooth union, then partings, carves (notches, cracks) and noise near
                the surface, and the flat bottom at -sink (the part that sits in the ground).
  Block         a rounded convex prism: a plan polygon between two heights, in its own frame (yaw, tilt), with its own
                rounding of the side, top and bottom edges and an inward taper.
  build         meshes a Rock and finishes it (Fallback, material, UVs, AO); column_hulls adds the collision.
  preview       renders a model as placed (cut at its ground) beside a 1.8 m figure.

Everything is seeded, so a script gives the same mesh on every run. Distances are metres, Z up, the front faces -Y.
"""
import math
import random
import time

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_model as lm
import looter_textures as lt

DENSITY = 180.0                              # px per metre of RockCliff: the cliff kit's, so the beds read alike
WALKABLE_Z = math.cos(math.radians(44.8))    # a surface whose normal's z exceeds this can be walked on


def log(message):
    print(f'OUTCROPS: {message}', flush=True)


# --- Noise (numpy, seeded: mathutils.noise can't be vectorized) ---

_GRAD = np.array([[1, 1, 0], [-1, 1, 0], [1, -1, 0], [-1, -1, 0], [1, 0, 1], [-1, 0, 1], [1, 0, -1], [-1, 0, -1],
                  [0, 1, 1], [0, -1, 1], [0, 1, -1], [0, -1, -1], [1, 1, 0], [-1, 1, 0], [0, -1, 1], [0, -1, -1]],
                 np.float32)


class Noise:
    """Gradient (Perlin) noise in about [-1, 1], vectorized over arrays of points, from a seeded permutation."""

    def __init__(self, seed):
        rnd = random.Random(seed)
        perm = list(range(256))
        rnd.shuffle(perm)
        self.perm = np.array(perm * 2, np.int32)
        self.shift = np.array([rnd.uniform(0.0, 200.0) for _ in range(3)], np.float32)

    def __call__(self, x, y, z):
        x = np.asarray(x, np.float32) + self.shift[0]
        y = np.asarray(y, np.float32) + self.shift[1]
        z = np.asarray(z, np.float32) + self.shift[2]
        xf, yf, zf = np.floor(x), np.floor(y), np.floor(z)
        xi, yi, zi = xf.astype(np.int32) & 255, yf.astype(np.int32) & 255, zf.astype(np.int32) & 255
        x, y, z = x - xf, y - yf, z - zf
        u, v, w = (t * t * t * (t * (t * 6.0 - 15.0) + 10.0) for t in (x, y, z))
        p = self.perm
        a = p[xi] + yi
        b = p[xi + 1] + yi
        aa, ab, ba, bb = p[a] + zi, p[a + 1] + zi, p[b] + zi, p[b + 1] + zi
        gx, gy, gz = _GRAD[:, 0], _GRAD[:, 1], _GRAD[:, 2]

        def g(h, dx, dy, dz):
            h = h & 15
            return gx[h] * dx + gy[h] * dy + gz[h] * dz

        x1, y1, z1 = x - 1.0, y - 1.0, z - 1.0
        n0 = g(p[aa], x, y, z) + u * (g(p[ba], x1, y, z) - g(p[aa], x, y, z))
        n1 = g(p[ab], x, y1, z) + u * (g(p[bb], x1, y1, z) - g(p[ab], x, y1, z))
        n2 = g(p[aa + 1], x, y, z1) + u * (g(p[ba + 1], x1, y, z1) - g(p[aa + 1], x, y, z1))
        n3 = g(p[ab + 1], x, y1, z1) + u * (g(p[bb + 1], x1, y1, z1) - g(p[ab + 1], x, y1, z1))
        m0 = n0 + v * (n1 - n0)
        m1 = n2 + v * (n3 - n2)
        return m0 + w * (m1 - m0)

    def fbm(self, x, y, z, octaves=3, gain=0.5):
        total, amp, freq, norm = 0.0, 1.0, 1.0, 0.0
        for o in range(octaves):
            total = total + amp * self(x * freq + 17.3 * o, y * freq - 9.1 * o, z * freq + 4.7 * o)
            norm += amp
            amp *= gain
            freq *= 2.03
        return total / norm


# --- Field helpers ---

def smin(a, b, k):
    """A union of two fields with a fillet of size k where they meet."""
    if k <= 0.0:
        return np.minimum(a, b)
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0.0, 1.0)
    return b + (a - b) * h - k * h * (1.0 - h)


def smax(a, b, k):
    """An intersection of two fields with its edge rounded over k."""
    return -smin(-a, -b, k)


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def bell(x, w):
    """1 at x = 0 falling smoothly to 0 at |x| = w."""
    t = np.clip(np.abs(x) / w, 0.0, 1.0)
    return (1.0 - t * t) ** 2


def rotation(yaw=0.0, tilt=(0.0, 0.0)):
    """A frame turned by tilt (degrees about X, then Y), then by yaw (degrees about Z), as a 3x3 numpy matrix."""
    rx, ry = math.radians(tilt[0]), math.radians(tilt[1])
    rz = math.radians(yaw)
    mx = np.array([[1, 0, 0], [0, math.cos(rx), -math.sin(rx)], [0, math.sin(rx), math.cos(rx)]])
    my = np.array([[math.cos(ry), 0, math.sin(ry)], [0, 1, 0], [-math.sin(ry), 0, math.cos(ry)]])
    mz = np.array([[math.cos(rz), -math.sin(rz), 0], [math.sin(rz), math.cos(rz), 0], [0, 0, 1]])
    return mz @ my @ mx


# --- Plan polygons (convex, counter-clockwise, as lists of (x, y)) ---

def rect(hx, hy, cx=0.0, cy=0.0):
    return [(cx - hx, cy - hy), (cx + hx, cy - hy), (cx + hx, cy + hy), (cx - hx, cy + hy)]


def ngon(radius, sides, rnd=None, jitter=0.0, squash=1.0, start=0.0):
    """A regular polygon, its corners jittered in radius (a share of it) by rnd."""
    points = []
    for i in range(sides):
        a = start + 2.0 * math.pi * i / sides
        r = radius * (1.0 + (rnd.uniform(-jitter, jitter) if rnd else 0.0))
        points.append((r * math.cos(a), r * math.sin(a) * squash))
    return hull2d(points)


def area2d(poly):
    return 0.5 * sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(poly, poly[1:] + poly[:1]))


def ccw(poly):
    return list(poly) if area2d(poly) >= 0.0 else list(reversed(poly))


def hull2d(points):
    """The convex hull of plan points, counter-clockwise (monotone chain)."""
    pts = sorted(set((round(p[0], 6), round(p[1], 6)) for p in points))
    if len(pts) < 3:
        return pts

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def clip(poly, n, c):
    """The part of a convex polygon where n . p <= c."""
    out = []
    count = len(poly)
    for i in range(count):
        a, b = poly[i - 1], poly[i]
        da = n[0] * a[0] + n[1] * a[1] - c
        db = n[0] * b[0] + n[1] * b[1] - c
        if db <= 0.0:
            if da > 0.0:
                t = da / (da - db)
                out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
            out.append(b)
        elif da <= 0.0:
            t = da / (da - db)
            out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
    return out


def split(poly, n, c, gap=0.0):
    """Both sides of a convex polygon cut by the line n . p = c, each set back gap/2 from it (an open joint)."""
    return clip(poly, n, c - gap * 0.5), clip(poly, (-n[0], -n[1]), -c - gap * 0.5)


def chamfer(poly, rnd, chance=0.6, cut=(0.15, 0.5)):
    """Cuts some corners off a convex polygon: worn and broken corners."""
    out = []
    count = len(poly)
    for i in range(count):
        p0, p1, p2 = Vector(poly[i - 1]), Vector(poly[i]), Vector(poly[(i + 1) % count])
        if rnd.random() < chance and (p1 - p0).length > 0.2 and (p2 - p1).length > 0.2:
            a = min(rnd.uniform(*cut), (p1 - p0).length * 0.35)
            b = min(rnd.uniform(*cut), (p2 - p1).length * 0.35)
            out.append(tuple(p1 + (p0 - p1).normalized() * a))
            out.append(tuple(p1 + (p2 - p1).normalized() * b))
        else:
            out.append(tuple(p1))
    return out


def offset(poly, distance):
    """A convex polygon grown (or shrunk, for a negative distance) by moving every side out along its normal."""
    planes = plan_planes(poly)
    points = []
    count = len(planes)
    for i in range(count):
        (n0x, n0y, c0), (n1x, n1y, c1) = planes[i - 1], planes[i]
        c0 += distance
        c1 += distance
        det = n0x * n1y - n0y * n1x
        if abs(det) < 1e-9:
            continue
        points.append(((c0 * n1y - c1 * n0y) / det, (n0x * c1 - n1x * c0) / det))
    return hull2d(points)


def offset_edges(poly, distances):
    """A convex polygon with each side (in counter-clockwise order, starting with the side from its first corner)
    moved out along its normal by its own distance."""
    poly = ccw(poly)
    lines = []
    for (a, b), dist in zip(zip(poly, poly[1:] + poly[:1]), distances):
        dx, dy = b[0] - a[0], b[1] - a[1]
        length = math.hypot(dx, dy)
        if length < 1e-6:
            continue
        nx, ny = dy / length, -dx / length
        lines.append((nx, ny, nx * a[0] + ny * a[1] + dist))
    points = []
    for i in range(len(lines)):
        (n0x, n0y, c0), (n1x, n1y, c1) = lines[i - 1], lines[i]
        det = n0x * n1y - n0y * n1x
        if abs(det) < 1e-6:
            continue
        points.append(((c0 * n1y - c1 * n0y) / det, (n0x * c1 - n1x * c0) / det))
    return hull2d(points)


def plan_planes(poly):
    """The sides of a convex polygon as (nx, ny, c): outside where nx*x + ny*y > c."""
    poly = ccw(poly)
    planes = []
    for a, b in zip(poly, poly[1:] + poly[:1]):
        dx, dy = b[0] - a[0], b[1] - a[1]
        length = math.hypot(dx, dy)
        if length < 1e-6:
            continue
        nx, ny = dy / length, -dx / length
        planes.append((nx, ny, nx * a[0] + ny * a[1]))
    return planes


def transform2d(poly, yaw=0.0, move=(0.0, 0.0), scale=1.0):
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return [(move[0] + scale * (x * c - y * s), move[1] + scale * (x * s + y * c)) for x, y in poly]


# --- Blocks and the rock ---

class Block:
    """A rounded convex prism: poly (plan, in the block's frame) from z0 to z1. Its frame is turned by tilt (degrees
    about X then Y, around the point (center, z0)) and yaw. r_side, r_top and r_bottom round its vertical, top and
    bottom edges (blend widths, metres); taper moves its sides in per metre of height (a block narrowing upward);
    blend is the fillet where it joins the rock. Returns distances through dist(P)."""

    def __init__(self, poly, z0, z1, center=(0.0, 0.0), yaw=0.0, tilt=(0.0, 0.0), r_side=0.18, r_top=0.45,
                 r_bottom=0.2, taper=0.0, blend=0.12, facets=(), r_facet=0.1):
        self.poly = ccw(poly)
        self.planes = np.array(plan_planes(self.poly), np.float64)
        self.z0, self.z1 = float(z0), float(z1)
        self.origin = np.array([center[0], center[1], z0], np.float64)
        self.matrix = rotation(yaw, tilt)
        self.r_side, self.r_top, self.r_bottom = r_side, r_top, r_bottom
        self.taper = taper
        self.blend = blend
        corners = np.array([(x, y, z) for x, y in self.poly for z in (0.0, self.z1 - self.z0)])
        # Facets: planes (local normal, depth) shaving the block where it would be a perfect prism, a broken face.
        self.facets = []
        for n, depth in facets:
            n = np.asarray(n, np.float64) / np.linalg.norm(n)
            self.facets.append((n, float((corners @ n).max()) - depth))
        self.r_facet = r_facet
        # A box around the block in the rock's space, to skip points far from it.
        world = corners @ self.matrix.T + self.origin
        margin = max(r_side, r_top, r_bottom, blend) + 0.35
        self.lo = world.min(axis=0) - margin
        self.hi = world.max(axis=0) + margin

    def local(self, P):
        return (P - self.origin) @ self.matrix          # rows: points in the block's frame (w up from z0)

    def dist(self, P):
        L = self.local(P)
        u, v, w = L[:, 0], L[:, 1], L[:, 2]
        height = self.z1 - self.z0
        inward = self.taper * w
        d = None
        for nx, ny, c in self.planes:
            e = nx * u + ny * v - c + inward
            d = e if d is None else smax(d, e, self.r_side)
        d = smax(d, w - height, self.r_top)
        d = smax(d, -w, self.r_bottom)
        for n, c in self.facets:
            d = smax(d, L @ n - c, self.r_facet)
        return d


class Rock:
    """The solid: blocks in a smooth union, then near its surface the partings, carves and weathering noise, and the
    flat bottom at -sink. field(P) gives the signed distance (about metres) for an (N, 3) array of points."""

    def __init__(self, seed, sink=0.9, lumps=(0.16, 3.6), detail=(0.055, 1.25), grain=(0.022, 0.42), rain=0.035,
                 band=1.1, warp=(0.32, 5.0), fold=0.18):
        self.seed = seed
        self.sink = sink
        self.blocks = []
        self.partings = []
        self.carves = []
        self.lumps, self.detail, self.grain, self.rain = lumps, detail, grain, rain
        self.band = band
        # Warp: the blocks are looked up at points moved by smooth noise (amplitude, wavelength), so joints wander and
        # lean and faces bow, as the cliff kit's do; fold lifts and drops the beds a little (metres).
        self.warp, self.fold = warp, fold
        self.noise = [Noise(seed * 31 + i) for i in range(11)]
        self.calm = []

    def calm_box(self, lo, hi, factor=0.15):
        """Weathering noise is scaled by factor inside this box (fading back over 0.6 m): a floor kept walkable."""
        self.calm.append((np.array(lo, np.float64), np.array(hi, np.float64), factor))

    def _calm(self, P):
        f = np.ones(len(P))
        for lo, hi, k in self.calm:
            outside = np.linalg.norm(np.maximum(np.maximum(lo - P, P - hi), 0.0), axis=1)
            f *= k + (1.0 - k) * smoothstep(0.0, 0.6, outside)
        return f

    def warped(self, P):
        if not self.warp or self.warp[0] <= 0.0:
            return P
        a, wl = self.warp
        n = self.noise
        x, y, z = P[:, 0] / wl, P[:, 1] / wl, P[:, 2] / wl
        Q = P.copy()
        Q[:, 0] += a * n[8].fbm(x, y, z * 0.7, 2)
        Q[:, 1] += a * n[9].fbm(x, y, z * 0.7, 2)
        fold = self.fold * (self._calm(P) if self.calm else 1.0)    # a calm floor stays level
        Q[:, 2] += fold * n[10].fbm(x * 0.8, y * 0.8, z * 0.3, 2)
        return Q

    def add(self, block):
        self.blocks.append(block)
        return block

    def parting(self, z, depth=0.35, width=0.16, wander=0.12, fade=0.25):
        """A thin soft bed at height z: a shadow line recessed by depth, coming and going around the rock."""
        self.partings.append((z, depth, width, wander, fade))

    def notch(self, planes, depth_limit=None):
        """Carves away the convex region inside every plane ((point, outward normal) pairs): a wedge broken out of an
        edge. With depth_limit, only within that depth of the surface."""
        self.carves.append(('region', [(np.array(p, np.float64), np.array(n, np.float64) / np.linalg.norm(n))
                                       for p, n in planes], depth_limit))

    def cut(self, block):
        """Carves a block out of the rock (a corner broken off, a wedge fallen out of an edge)."""
        self.carves.append(('block', block, None))
        return block

    def crack(self, point, normal, width=0.12, depth=0.45, radius=2.0, shift=0.0):
        """A fracture: a slot of half width `width` along the plane (point, normal), reaching depth into the rock and
        fading out radius from point; shift sets the far side back (the block below a diagonal crack slumped)."""
        n = np.array(normal, np.float64)
        self.carves.append(('crack', (np.array(point, np.float64), n / np.linalg.norm(n), width, depth, radius, shift),
                            None))

    def bounds(self, margin=0.6):
        lo = np.min([b.lo for b in self.blocks], axis=0)
        hi = np.max([b.hi for b in self.blocks], axis=0)
        lo[2] = max(lo[2], -self.sink - 0.3)
        return lo - margin, hi + margin

    def field(self, P):
        P = np.asarray(P, np.float64)
        Q = self.warped(P)
        d = np.full(len(P), 50.0)
        q_lo, q_hi = Q.min(axis=0), Q.max(axis=0)
        for block in self.blocks:
            if np.any(block.lo > q_hi) or np.any(block.hi < q_lo):
                continue                    # nowhere near these points
            inside = np.all((Q >= block.lo) & (Q <= block.hi), axis=1)
            if inside.any():
                d[inside] = smin(d[inside], block.dist(Q[inside]), block.blend)
        if self.carves:
            d = self._carve(Q, d)
        near = np.abs(d) < self.band
        if near.any():
            d[near] = self._weather(P[near], Q[near, 2], d[near])
        return np.maximum(d, -self.sink - P[:, 2])

    def _carve(self, P, d):
        for kind, data, limit in self.carves:
            if kind == 'block':
                inside = np.all((P >= data.lo) & (P <= data.hi), axis=1)
                if inside.any():
                    d[inside] = smax(d[inside], -data.dist(P[inside]), data.blend)
            elif kind == 'region':
                region = None
                for p, normal in data:
                    e = (P - p) @ normal
                    region = e if region is None else np.maximum(region, e)
                if limit is not None:
                    region = np.maximum(region, -limit - d)
                d = np.maximum(d, -region)
            else:
                p, normal, width, depth, radius, shift = data
                s = (P - p) @ normal
                r = np.linalg.norm(P - p, axis=1)
                window = 1.0 - smoothstep(radius * 0.5, radius, r)
                # A V in section: width at the surface, closing to nothing at its depth.
                taper = np.clip(1.0 + d / np.maximum(depth * window, 1e-3), 0.0, 1.0)
                slot = np.maximum(np.abs(s) - width * window * taper, -depth * window - d)
                d = np.maximum(d, -slot) + shift * smoothstep(0.03, -0.03, s) * window
        return d

    def _weather(self, P, folded_z, d):
        x, y, z = P[:, 0], P[:, 1], P[:, 2]
        n = self.noise
        for zp, depth, width, wander, fade in self.partings:
            level = zp + wander * n[0](x / 3.0, y / 3.0, zp)
            # Here a deep shadow line, there nearly closed: it comes and goes around the rock. It folds with the beds.
            open_ = 0.15 + 0.85 * smoothstep(-0.3, 0.3, n[1](x / 3.4, y / 3.4, zp * 0.7) + fade)
            d = d + depth * open_ * bell(folded_z - level, width)
        f = self._calm(P) if self.calm else 1.0
        a, wl = self.lumps
        d = d + f * a * n[2].fbm(x / wl, y / wl, z / (wl * 1.3), 3)
        a, wl = self.detail
        d = d + f * a * n[3].fbm(x / wl, y / wl, z / wl, 2)
        a, wl = self.grain
        d = d + f * a * n[4](x / wl, y / wl, z / wl)
        if self.rain:
            # Rain grooves: fine vertical runnels, few and far between.
            r = np.abs(n[5](x * 2.2, y * 2.2, z * 0.22 + 0.3 * n[6](x, y, z * 0.3)))
            d = d + f * self.rain * np.clip(1.0 - r / 0.12, 0.0, 1.0) ** 2
        return d


# --- Meshing ---

def _evaluate_full(rock, lo, shape, cell):
    nx, ny, nz = shape
    xs = lo[0] + cell * np.arange(nx)
    ys = lo[1] + cell * np.arange(ny)
    zs = lo[2] + cell * np.arange(nz)
    F = np.empty(shape, np.float32)
    step = max(1, int(1_500_000 // (nx * ny)))
    for k0 in range(0, nz, step):
        k1 = min(nz, k0 + step)
        X, Y, Z = np.meshgrid(xs, ys, zs[k0:k1], indexing='ij')
        P = np.stack([X.ravel(), Y.ravel(), Z.ravel()], axis=1)
        F[:, :, k0:k1] = rock.field(P).reshape(nx, ny, k1 - k0)
    return F


def evaluate(rock, lo, shape, cell, coarse=4, lipschitz=1.8):
    """The field on the grid lo + cell * (i, j, k), with the border forced outside so the surface closes. A grid
    `coarse` times coarser comes first; fine points are evaluated only in the coarse cells the surface could pass
    through (a corner within lipschitz times the coarse cell's diagonal of it), the rest take their coarse cell's sign."""
    shape_c = tuple(int(math.ceil((n - 1) / coarse)) + 2 for n in shape)
    C = _evaluate_full(rock, lo, shape_c, cell * coarse)
    reach = lipschitz * cell * coarse * math.sqrt(3.0)
    near = np.abs(C) < reach
    cells = np.zeros(tuple(n - 1 for n in shape_c), bool)
    for dx, dy, dz in _CORNERS:
        cells |= near[dx:shape_c[0] - 1 + dx, dy:shape_c[1] - 1 + dy, dz:shape_c[2] - 1 + dz]
    # Each fine point belongs to the coarse cell below it; far from the surface it takes that cell's first corner.
    idx = [np.minimum(np.arange(n) // coarse, cells.shape[a] - 1) for a, n in enumerate(shape)]
    F = C[np.ix_(idx[0], idx[1], idx[2])].astype(np.float32)
    F = np.where(F < 0.0, np.float32(-reach), np.float32(reach))
    todo = cells[np.ix_(idx[0], idx[1], idx[2])]
    points = np.argwhere(todo)
    for start in range(0, len(points), 1_500_000):
        part = points[start:start + 1_500_000]
        F[part[:, 0], part[:, 1], part[:, 2]] = rock.field(lo + part * cell)
    F[0], F[-1], F[:, 0], F[:, -1], F[:, :, 0], F[:, :, -1] = 1.0, 1.0, 1.0, 1.0, 1.0, 1.0
    log(f'evaluated {len(points)} of {F.size} grid points near the surface')
    return F


_CORNERS = np.array([(0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0), (0, 0, 1), (1, 0, 1), (0, 1, 1), (1, 1, 1)])
_EDGES = [(0, 1), (2, 3), (4, 5), (6, 7), (0, 2), (1, 3), (4, 6), (5, 7), (0, 4), (1, 5), (2, 6), (3, 7)]


def surface_nets(F, lo, cell):
    """The zero surface of a sampled field as quads: one vertex per cell the surface crosses (the mean of its edge
    crossings), one quad per crossed grid edge, wound so normals point out of the rock."""
    inside = F < 0.0
    nx, ny, nz = F.shape
    count = np.zeros((nx - 1, ny - 1, nz - 1), np.int8)
    for dx, dy, dz in _CORNERS:
        count += inside[dx:nx - 1 + dx, dy:ny - 1 + dy, dz:nz - 1 + dz]
    active = (count > 0) & (count < 8)
    cells = np.argwhere(active)
    index = np.full(active.shape, -1, np.int64)
    index[active] = np.arange(len(cells))
    values = np.stack([F[cells[:, 0] + o[0], cells[:, 1] + o[1], cells[:, 2] + o[2]] for o in _CORNERS], axis=1)
    total = np.zeros((len(cells), 3))
    hits = np.zeros(len(cells))
    for a, b in _EDGES:
        fa, fb = values[:, a].astype(np.float64), values[:, b].astype(np.float64)
        crossed = (fa < 0.0) != (fb < 0.0)
        t = np.where(crossed, fa / np.where(crossed, fa - fb, 1.0), 0.0)
        p = _CORNERS[a] + t[:, None] * (_CORNERS[b] - _CORNERS[a])
        total[crossed] += p[crossed]
        hits[crossed] += 1.0
    V = lo + (cells + total / np.maximum(hits, 1.0)[:, None]) * cell
    quads = []
    for axis in range(3):
        b_axis, c_axis = (axis + 1) % 3, (axis + 2) % 3
        s0 = [slice(None)] * 3
        s1 = [slice(None)] * 3
        s0[axis], s1[axis] = slice(0, -1), slice(1, None)
        a_in, b_in = inside[tuple(s0)], inside[tuple(s1)]
        nodes = np.argwhere(a_in != b_in)
        out = a_in[tuple(nodes.T)]                       # inside below: the surface faces +axis
        eb = np.zeros(3, np.int64)
        ec = np.zeros(3, np.int64)
        eb[b_axis], ec[c_axis] = 1, 1
        ring = [nodes - eb - ec, nodes - ec, nodes, nodes - eb]
        q = np.stack([index[tuple(r.T)] for r in ring], axis=1)
        q[~out] = q[~out][:, ::-1]
        quads.append(q)
    return V, np.concatenate(quads)


def project(V, rock, cell, iterations=2):
    """Pulls each vertex onto the zero surface along the field's gradient (at most half a cell per step)."""
    e = cell * 0.3
    offsets = [np.array(o) * e for o in ((1, 0, 0), (0, 1, 0), (0, 0, 1))]
    for _ in range(iterations):
        d = rock.field(V)
        g = np.stack([(rock.field(V + o) - rock.field(V - o)) / (2.0 * e) for o in offsets], axis=1)
        g2 = np.maximum((g * g).sum(axis=1), 1e-6)
        move = -(d / g2)[:, None] * g
        length = np.linalg.norm(move, axis=1)
        limit = np.minimum(1.0, (0.5 * cell) / np.maximum(length, 1e-9))
        V = V + move * limit[:, None]
    return V


def mesh_object(name, V, faces):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([tuple(v) for v in V], [], [tuple(f) for f in faces])
    mesh.validate()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def reduce(obj, target):
    """Collapses the mesh to about target triangles (Blender's quadric-weighted edge collapse keeps the ledges)."""
    tris = triangles(obj)
    if tris <= target:
        return
    mod = obj.modifiers.new('Reduce', 'DECIMATE')
    mod.ratio = target / tris
    mod.use_collapse_triangulate = True
    with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj], selected_editable_objects=[obj]):
        bpy.ops.object.modifier_apply(modifier=mod.name)


def _sliver(face):
    longest = max(e.calc_length() for e in face.edges)
    return face.calc_area() < max(1e-4 * longest * longest, 1e-9)


def clean(obj):
    """Welds duplicates and removes zero-area faces (they have no tangents in Unreal); slivers get their long edge turned."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-4)
    bmesh.ops.dissolve_degenerate(bm, dist=1e-4, edges=bm.edges[:])
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 3])
    for _ in range(6):
        slivers = [f for f in bm.faces if _sliver(f)]
        if not slivers:
            break
        edges = {max(f.edges, key=lambda e: e.calc_length()) for f in slivers}
        bmesh.ops.rotate_edges(bm, edges=[e for e in edges if len(e.link_faces) == 2], use_ccw=False)
        bmesh.ops.dissolve_degenerate(bm, dist=1e-4, edges=bm.edges[:])
        bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 3])
    flat = [f for f in bm.faces if _sliver(f)]
    if flat:
        bmesh.ops.delete(bm, geom=flat, context='FACES_ONLY')
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context='VERTS')
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()


def mesh_rock(name, rock, cell=0.075, source=9000, iterations=2):
    """The rock's surface as a mesh object reduced to about `source` triangles."""
    start = time.time()
    lo, hi = rock.bounds()
    shape = tuple(int(math.ceil((h - l) / cell)) + 1 for l, h in zip(lo, hi))
    F = evaluate(rock, lo, shape, cell)
    V, Q = surface_nets(F, lo, cell)
    del F
    V = project(V, rock, cell, iterations)
    obj = mesh_object(name, V, Q)
    raw = len(Q) * 2
    reduce(obj, source)
    clean(obj)
    log(f'{name}: grid {shape[0]}x{shape[1]}x{shape[2]}, {raw} -> {triangles(obj)} triangles '
        f'({time.time() - start:.1f} s)')
    return obj


ROCK_MATERIAL = None


def rock_material():
    """RockCliff with moss on up-facing rock, exactly as Cliffs.py and Rocks.py make it (a shared material)."""
    global ROCK_MATERIAL
    if ROCK_MATERIAL is None or ROCK_MATERIAL.name not in bpy.data.materials:
        ROCK_MATERIAL = lt.material('RockCliff', MossAmount=0.6)
    return ROCK_MATERIAL


def finish(obj, fallback=None, uv_seed=0, ao_distance=1.6, smooth=55.0, density=DENSITY, frames=None, nanite=True,
           matrix=None, ao=True):
    """Shading, material, UVs (the cliff kit's world-scale box mapping, or per-part frames), then matrix (a 4x4 moving
    the mapped mesh: a slab built upright and leaned afterwards keeps its strata along it), baked AO, export props."""
    lm.smooth(obj, smooth)
    lt.assign(obj, rock_material())
    if frames is None:
        lt.box_uv(obj, 'RockCliff', texel_density=density, seed=uv_seed)
    else:
        frame_box_uv(obj, frames, density, uv_seed)
    if matrix is not None:
        obj.data.transform(Matrix(matrix))
        obj.data.update()
    if ao:
        lt.bake_vertex_ao(obj, distance=ao_distance)
    if fallback is not None and nanite:
        obj['Fallback'] = float(fallback)
    if not nanite:
        obj['Nanite'] = 0
    return obj


def darken(obj, multiplier):
    """Multiplies the baked AO (vertex color alpha) by multiplier(P), P the (N, 3) vertex positions: a cave's back
    kept dim beyond what the AO's reach sees."""
    mesh = obj.data
    raw = lt._read_col(mesh)
    loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loop_vert)
    factor = np.asarray(multiplier(vertices(obj)), np.float64)
    raw[:, 3] = np.clip(raw[:, 3] * factor[loop_vert], 0.0, 1.0)
    lt._write_col(mesh, raw)


def frame_box_uv(obj, frames, density, seed):
    """Box mapping in per-part frames: frames(center) -> (3x3 rotation, origin) for each face's center, so a tilted
    slab's strata follow the slab. Within a frame it is lt.box_uv's projection (side faces keep V up the frame)."""
    rng = random.Random(seed)
    scale = density / 1024.0
    shift = (rng.uniform(0.0, 5.0), rng.uniform(0.0, 5.0))
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    uv = bm.loops.layers.uv.active or bm.loops.layers.uv.new('UVMap')
    for face in bm.faces:
        R, origin, extra = frames(np.array(face.calc_center_median()))
        Rm = Matrix(R.tolist())
        n = Rm.transposed() @ face.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        ex, ey, ez = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))
        if axis == 2:
            u_axis, v_axis = ex, (ey if n.z > 0.0 else -ey)
        elif axis == 0:
            u_axis, v_axis = (ey if n.x > 0.0 else -ey), ez
        else:
            u_axis, v_axis = (-ex if n.y > 0.0 else ex), ez
        o = Vector(origin)
        for loop in face.loops:
            p = Rm.transposed() @ (loop.vert.co - o)
            loop[uv].uv = ((p.dot(u_axis) + shift[0] + extra[0]) * scale, (p.dot(v_axis) + shift[1] + extra[1]) * scale)
    bm.to_mesh(obj.data)
    bm.free()


def build(name, rock, cell=0.075, source=9000, fallback=30, uv_seed=0, ao_distance=1.6, frames=None, matrix=None):
    obj = mesh_rock(name, rock, cell, source)
    return finish(obj, fallback, uv_seed, ao_distance, frames=frames, matrix=matrix)


# --- Collision ---

def _directions(subdivisions=2):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdivisions, radius=1.0)
    dirs = np.array([v.co.normalized()[:] for v in bm.verts])
    bm.free()
    return dirs


def hull_object(obj, points, name=None):
    """A convex collision hull of obj around points (model space)."""
    bm = bmesh.new()
    for p in points:
        bm.verts.new(Vector(p))
    result = bmesh.ops.convex_hull(bm, input=bm.verts[:])
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    mesh = bpy.data.meshes.new(name or 'UCX_' + obj.name)
    bm.to_mesh(mesh)
    bm.free()
    hull = bpy.data.objects.new(name or 'UCX_' + obj.name, mesh)
    bpy.context.scene.collection.objects.link(hull)
    hull.parent = obj
    hull.display_type = 'WIRE'
    return hull


def extremes(points, dirs):
    """The points reaching furthest along each direction: their hull is the points' hull, simplified."""
    if len(points) <= len(dirs):
        return points
    picks = np.unique(np.argmax(points @ dirs.T, axis=0))
    return points[picks]


def vertices(obj):
    co = np.empty(3 * len(obj.data.vertices))
    obj.data.vertices.foreach_get('co', co)
    return co.reshape(-1, 3)


def column_hulls(obj, seeds, margin=0.3, z_range=None, dirs=None, extra=None):
    """One convex hull per plan seed: the vertices nearer that seed than any other (and those within margin of the
    boundary, so neighbours overlap and leave no gap), each a column from the ground to the rock's top above it. Its
    sides bridge the beds' ledges, so the player finds no step to stand on. extra adds point lists as hulls of their own
    (fallen blocks)."""
    V = vertices(obj)
    if z_range is not None:
        V = V[(V[:, 2] >= z_range[0]) & (V[:, 2] <= z_range[1])]
    S = np.array(seeds, np.float64)
    dirs = _directions(2) if dirs is None else dirs
    hulls = []
    for i, s in enumerate(S):
        within = np.ones(len(V), bool)
        for j, t in enumerate(S):
            if j == i:
                continue
            # Distance past the bisector of seeds i and j, toward j.
            gap = np.linalg.norm(t - s)
            beyond = ((V[:, :2] - s) ** 2).sum(axis=1) - ((V[:, :2] - t) ** 2).sum(axis=1)
            within &= beyond / (2.0 * gap) <= margin
        points = V[within]
        if len(points) >= 4:
            hulls.append(hull_object(obj, extremes(points, dirs)))
    for points in extra or []:
        hulls.append(hull_object(obj, extremes(np.asarray(points), dirs)))
    return hulls


def block_hulls(obj, rock, groups, margin=0.2, dirs=None):
    """One convex hull per group of blocks: the mesh's vertices nearest a block of the group (or within margin of
    their nearest, so neighbouring hulls overlap and leave no gap)."""
    V = vertices(obj)
    Q = rock.warped(V)
    flat = [b for group in groups for b in group]
    D = np.stack([b.dist(Q) for b in flat], axis=1)
    best = D.min(axis=1)
    dirs = _directions(2) if dirs is None else dirs
    hulls, start = [], 0
    for group in groups:
        mine = D[:, start:start + len(group)].min(axis=1)
        start += len(group)
        points = V[mine <= best + margin]
        if len(points) >= 4:
            hulls.append(hull_object(obj, extremes(points, dirs)))
    return hulls


def hull_report(obj):
    """For each hull: its top, and the lowest face a player could stand on (normal within 44.8 degrees of up)."""
    lines = []
    lowest = None
    for hull in [c for c in obj.children if c.name.startswith('UCX_')]:
        mesh = hull.data
        top = max(v.co.z for v in mesh.vertices)
        walk = [min(mesh.vertices[i].co.z for i in p.vertices) for p in mesh.polygons if p.normal.z > WALKABLE_Z]
        low = min(walk) if walk else None
        lowest = low if lowest is None or (low is not None and low < lowest) else lowest
        lines.append(f'{len(mesh.vertices)}v top {top:.2f}' + (f' stand>={low:.2f}' if low is not None else ''))
    log(f"{obj.name}: {len(lines)} hulls: " + '; '.join(lines))
    return lowest


# --- Previews ---

def scale_figure(location, height=1.8):
    """A 1.8 m box for scale (removed again after the render)."""
    mat = bpy.data.materials.get('ScaleFigure') or lm.material('ScaleFigure', 0x6c7a8a)
    figure = lm.box('ScaleFigure', (0.5, 0.3, height), (location[0], location[1], location[2] + height * 0.5), mat)
    return figure


def placed_copy(model, ground=0.0):
    """A temporary copy of a model cut at its ground (lt.preview would stand the ground under its lowest point)."""
    copy = model.copy()
    copy.data = model.data.copy()
    copy.name = 'Preview_' + model.name
    for child in list(copy.children):
        child.parent = None
    bpy.context.scene.collection.objects.link(copy)
    bm = bmesh.new()
    bm.from_mesh(copy.data)
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=(0.0, 0.0, ground),
                           plane_no=(0.0, 0.0, 1.0), clear_inner=True)
    bm.to_mesh(copy.data)
    bm.free()
    refresh(copy)
    return copy


def refresh(obj):
    """After editing a mesh: its bounds (which previews frame by) follow it again."""
    obj.data.update()
    bpy.context.view_layer.update()


def remove(objects):
    for o in objects:
        data = o.data
        bpy.data.objects.remove(o)
        if isinstance(data, bpy.types.Mesh) and data.users == 0:
            bpy.data.meshes.remove(data)


def bounds(objects):
    pts = np.array([(o.matrix_world @ Vector(c))[:] for o in objects for c in o.bound_box])
    return pts.min(axis=0), pts.max(axis=0)


def preview(model, path, view=(-1.0, -1.6, 0.55), fit=0.95, figure=True, ground=0.0, figure_at=None, **options):
    """Renders a model as placed, cut at its ground, beside a 1.8 m figure."""
    shown = [placed_copy(model, ground)]
    if figure:
        lo, hi = bounds(shown)
        at = figure_at or (hi[0] + 0.8, lo[1] + 0.3, ground)
        shown.append(scale_figure(at))
    try:
        lt.preview(shown, path, view=view, fit=fit, ground_at=ground, **options)
    finally:
        remove(shown)
