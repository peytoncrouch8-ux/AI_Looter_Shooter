"""Skyreach's backdrop: four distant floating islands hanging in the sky around the tutorial island (Docs/Story.md,
"Leaving the tutorial island"). A scripted model (see Art/README.md).

  SkyIsland_A   the arch: a broad isle whose underside hangs in two keels joined by a bridge of rock, so the sky
                shows through a window in its body; a ruined watchtower on its hill (an echo of the tutorial island's
                lookout), a dirt path down to the rim
  SkyIsland_B   the stack: two islands fused one above the other, the lower one's grass a terrace under the upper's
                rim; a stream off the upper lip in a waterfall that breaks into spray and mist
  SkyIsland_C   the needle: a crag-topped islet over a slender underside drawn out into one long spire
  SkyIsland_D   the break: tipped well over, with a slab of its edge breaking away, roots torn across the gap and
                debris adrift

Each is built like the tutorial island's own underside (Art/Levels/area_mesh.py): a grassy top over a short cliff rim,
then rock in lobes, overhanging terraces and ledges tapering to keels, with spires, roots and vines hanging from it.
The tops carry rock outcrops, pale dirt and two-tone tree clumps. They are seen only from hundreds of metres away, so
silhouette is everything and they stay cheap: up to about 3k triangles, Nanite off, LODs '50,25', no collision. Two
material slots: RockCliff (the rock, wrapped cylindrically like the tutorial underside so its strata stay level; the
ruin, box-mapped; roots on the texture's darkest patch; falling water, mist and dirt paths on its palest) and
SkyIslandGrass (the grass; dark trees and vines on its darkest patch). Place them with Cast Shadow off. The pivot is at
the middle of the top, at the height of its rim.
"""
import math
import random

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_model as lm
import looter_outcrops as lo
import looter_textures as lt

ROCK = lt.material('RockCliff', MossAmount=0.6)
GRASS = lt.material('GroundGrass', name='SkyIslandGrass', tint=0xe8e0b4)    # meadow grass in low warm light
ROCK_DENSITY = 100.0         # px per metre: about a 10 m repeat, as under the tutorial island (9.6 m)
GRASS_DENSITY = 40.0
RUIN_DENSITY = 220.0
# Patches of the two textures (U, V) that faces are mapped onto whole, for a flat colour from either slot.
PATCHES = {
    'water': (0.311, 0.081),     # T_RockCliff_BC's palest band: falling water, mist, the stream
    'root': (0.041, 0.317),      # T_RockCliff_BC's darkest spot: roots
    'vine': (0.554, 0.811),      # T_GroundGrass_BC's darkest green: vines
    'dark': (0.554, 0.811),      #   and the darker tree clumps
    'dirt': (0.326, 0.087),      # T_RockCliff_BC's pale beige again: dirt paths and bare ground
}
KINDS = ('rock', 'grass', 'water', 'root', 'vine', 'dark', 'dirt', 'ruin')
GRASS_KINDS = ('grass', 'vine', 'dark')


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


class Parts:
    """Vertices and faces gathered for one model; each face has a kind (see KINDS)."""

    def __init__(self):
        self.verts = []
        self.faces = []

    def add(self, points):
        start = len(self.verts)
        self.verts.extend(tuple(float(c) for c in p) for p in points)
        return start

    def face(self, indices, kind):
        self.faces.append((tuple(indices), kind))

    def zipper(self, a0, na, b0, nb, kind):
        """Triangles joining two closed loops (counts na, nb) that start at the same angle."""
        i = j = 0
        while i < na or j < nb:
            if j >= nb or (i < na and (i + 1) / na <= (j + 1) / nb):
                self.face((a0 + i % na, a0 + (i + 1) % na, b0 + j % nb), kind)
                i += 1
            else:
                self.face((a0 + i % na, b0 + (j + 1) % nb, b0 + j % nb), kind)
                j += 1

    def fan(self, center, ring0, n, kind, reverse=False):
        for i in range(n):
            a, b = ring0 + i, ring0 + (i + 1) % n
            self.face((center, b, a) if reverse else (center, a, b), kind)

    def transform(self, start, matrix):
        """Moves every vertex from start on by a 4x4 matrix."""
        self.verts[start:] = [tuple(matrix @ Vector(v)) for v in self.verts[start:]]


PROFILE = ([0.0, 0.1, 0.24, 0.4, 0.58, 0.75, 0.88, 0.97], [1.0, 0.94, 0.82, 0.64, 0.45, 0.27, 0.14, 0.05])
# An overhanging terrace part way down: the rock steps in, then a harder bed juts out again over it.
TERRACED = ([0.0, 0.1, 0.22, 0.3, 0.36, 0.5, 0.68, 0.84, 0.96], [1.0, 0.95, 0.85, 0.8, 0.9, 0.62, 0.38, 0.17, 0.05])


class Island:
    """One floating island's shape: an outline (radius by angle, with an optional bite out of it), its top's heights
    and the underside's profile (depth share, radius share)."""

    def __init__(self, seed, radius, aspect=0.85, rim=6.0, depth=60.0, segments=40, hills=(), roll=1.5,
                 lobes=0.1, keel=(0.0, 0.0), rows=10, ledges=(0.05, 8.0), profile=PROFILE, bite=None, sag=0.18):
        self.rnd = random.Random(seed)
        self.noise = lo.Noise(seed)
        self.R, self.aspect, self.rim, self.depth, self.segments = radius, aspect, rim, depth, segments
        self.hills, self.roll, self.lobes, self.keel = hills, roll, lobes, keel
        self.rows, self.ledges, self.profile, self.bite, self.sag = rows, ledges, profile, bite, sag
        self.harmonics = [(k, self.rnd.uniform(0.03, 0.1) / (1.0 + 0.4 * k), self.rnd.uniform(0.0, 2.0 * math.pi))
                          for k in range(2, 7)]

    def radius(self, theta):
        r = 1.0 + sum(a * math.cos(k * theta + p) for k, a, p in self.harmonics)
        if self.bite is not None:
            at, depth, width = self.bite
            d = abs((theta - at + math.pi) % (2.0 * math.pi) - math.pi)
            r -= depth * max(0.0, 1.0 - (d / width) ** 2)
        return self.R * r

    def edge(self, theta, f=1.0):
        r = self.radius(theta) * f
        return r * math.cos(theta), r * math.sin(theta) * self.aspect

    def n(self, x, y, z=0.0):
        return float(self.noise(np.array([x]), np.array([y]), np.array([z]))[0])

    def top_z(self, x, y, f=None):
        """The grass's height: hills and a gentle roll, falling to the rim's height (0) toward the edge."""
        if f is None:
            f = math.hypot(x, y / self.aspect) / self.R
        z = self.roll * self.n(x / 18.0, y / 18.0, 1.3) + 0.5 * self.roll * self.n(x / 7.0, y / 7.0, 4.1)
        for hx, hy, hr, hh in self.hills:
            d2 = ((x - hx) ** 2 + (y - hy) ** 2) / (hr * hr)
            z += hh * math.exp(-d2 * 1.6)
        return z * (1.0 - smoothstep(0.72, 1.0, f)) + 0.45 * self.n(x / 6.0, y / 6.0, 7.7)


def underside(parts, isle, origin, first, z_top, below):
    """Loops from z_top down `below` metres along the island's profile, zipped on from the loop first = (start, count):
    lobes (some hanging lower), ribs, ledges at the foot of each layer. Ends in a tip, or a flat cap where the profile
    ends wide. Returns the loops as (start, count, points)."""
    ox, oy, _ = origin
    seg = isle.segments
    profile_t, profile_r = isle.profile
    loops = []
    for t in np.linspace(0.0, 1.0, isle.rows)[1:]:
        r = float(np.interp(t, profile_t, profile_r))
        count = max(8, int(seg * (0.35 + 0.65 * min(r, 1.0))))
        pts = []
        for i in range(count):
            theta = 2.0 * math.pi * i / count
            x, y = isle.edge(theta, 0.965 * r)
            belly = math.sin(math.pi * min(t * 1.2, 1.0)) ** 0.7
            lobe = isle.lobes * isle.R * isle.n(theta * 1.6, t * 1.3, 11.0) * belly
            rib = 0.03 * isle.R * isle.n(theta * 6.0, t * 2.0, 13.0)
            layer = (below * t / isle.ledges[1] + 0.35 * isle.n(theta * 0.8, 0.5, 19.0)) % 1.0
            ledge = isle.ledges[0] * isle.R * layer ** 3
            out = Vector((math.cos(theta), math.sin(theta) * isle.aspect)).normalized() * \
                (lobe + rib + ledge) * min(1.0, r * 2.0)
            sag = isle.sag * isle.depth * max(isle.n(theta * 1.2 + 9.0, t, 17.0), 0.0) * t * (1.0 - t) * 2.0
            kx, ky = isle.keel[0] * t * t, isle.keel[1] * t * t
            pts.append((ox + x + out.x + kx, oy + y + out.y + ky, z_top - below * t - sag))
        loops.append((parts.add(pts), count, pts))
    prev = first
    for start, count, _ in loops:
        parts.zipper(prev[0], prev[1], start, count, 'rock')
        prev = (start, count)
    last = loops[-1][2]
    cx, cy = sum(p[0] for p in last) / len(last), sum(p[1] for p in last) / len(last)
    lowest = min(p[2] for p in last)
    wide = profile_r[-1] > 0.2
    tip = parts.add([(cx, cy, lowest - (1.0 if wide else 0.08 * isle.depth))])
    parts.fan(tip, loops[-1][0], loops[-1][1], 'rock', reverse=True)
    return loops


def build_island(parts, isle, origin=(0.0, 0.0, 0.0), notch=None, stream=None):
    """The top (fan and rings, grass), the rim (rock) and the underside (rock) of one island into parts. notch =
    (angle, depth): the rim dips where a stream leaves; stream = angle: the top's faces along that line to the edge are
    water. Returns the underside's loops and the index of the top's edge ring (segments points in order)."""
    ox, oy, oz = origin
    seg = isle.segments
    thetas = [2.0 * math.pi * i / seg for i in range(seg)]

    def dip(theta):
        if notch is None:
            return 0.0
        d = abs((theta - notch[0] + math.pi) % (2.0 * math.pi) - math.pi)
        return notch[1] * max(0.0, 1.0 - d / 0.16)

    fractions = [0.22, 0.45, 0.66, 0.84, 1.0]
    center = parts.add([(ox, oy, oz + isle.top_z(0.0, 0.0, 0.0))])
    rings = []
    for f in fractions:
        pts = []
        for t in thetas:
            x, y = isle.edge(t, f)
            pts.append((ox + x, oy + y, oz + isle.top_z(x, y, f) - dip(t) * smoothstep(0.6, 1.0, f)))
        rings.append(parts.add(pts))
    parts.fan(center, rings[0], seg, 'grass')
    for k in range(len(rings) - 1):
        for i in range(seg):
            a0, a1 = rings[k] + i, rings[k] + (i + 1) % seg
            b0, b1 = rings[k + 1] + i, rings[k + 1] + (i + 1) % seg
            kind = 'grass'
            if stream is not None and k >= 1:
                d = abs((thetas[i] + math.pi / seg - stream + math.pi) % (2.0 * math.pi) - math.pi)
                if d < math.pi / seg:
                    kind = 'water'
            parts.face((a0, a1, b1), kind)
            parts.face((a0, b1, b0), kind)
    edge = [parts.verts[rings[-1] + i] for i in range(seg)]
    # The rim: a short cliff under the edge, leaning in (the grass's lip overhangs it), rough.
    rows = [(rings[-1], seg)]
    for frac, shrink in ((0.45, 0.975), (1.0, 0.95)):
        pts = []
        for t, (ex, ey, ez) in zip(thetas, edge):
            x, y = isle.edge(t, shrink)
            rough = 0.6 * isle.n(t * 3.0, frac * 2.0, 2.2)
            out = Vector((x, y)).normalized() * rough
            pts.append((ox + x + out.x, oy + y + out.y, ez - isle.rim * frac * (1.0 + 0.15 * isle.n(t * 2.0, 5.0))))
        rows.append((parts.add(pts), seg))
    for (a, na), (b, nb) in zip(rows, rows[1:]):
        parts.zipper(a, na, b, nb, 'rock')
    loops = underside(parts, isle, origin, rows[-1], oz - isle.rim, isle.depth - isle.rim)
    return loops, rings[-1]


def mass(parts, isle, origin, z_top, height):
    """A hanging mass of rock (a keel) under origin: a capped loop at z_top (inside what it hangs from), then the
    island's underside profile down `height` metres to a tip. Returns its loops."""
    seg = isle.segments
    pts = []
    for i in range(seg):
        x, y = isle.edge(2.0 * math.pi * i / seg, 0.98)
        pts.append((origin[0] + x, origin[1] + y, z_top))
    ring = parts.add(pts)
    parts.fan(parts.add([(origin[0], origin[1], z_top + 1.0)]), ring, seg, 'rock')
    return underside(parts, isle, origin, (ring, seg), z_top, height)


def spire(parts, root, length, width, lean, rnd, sides=6, rings=5):
    """A rock spire hanging from root: a fat root narrowing fast, then a drip."""
    start = len(parts.verts)
    for r in range(rings):
        f = r / (rings - 1)
        rad = width * ((1.0 - f) ** 1.7 * 0.85 + 0.15 * (1.0 - f)) + 0.15
        z = root[2] + 0.25 * length - 1.25 * length * f
        cx, cy = root[0] + lean[0] * length * f * f, root[1] + lean[1] * length * f * f
        pts = []
        for s in range(sides):
            a = 2.0 * math.pi * (s + 0.5 * r) / sides
            wob = 1.0 + rnd.uniform(-0.18, 0.18)
            pts.append((cx + math.cos(a) * rad * wob, cy + math.sin(a) * rad * wob, z))
        parts.add(pts)
    for r in range(rings - 1):
        parts.zipper(start + r * sides, sides, start + (r + 1) * sides, sides, 'rock')
    tip = parts.add([(root[0] + lean[0] * length, root[1] + lean[1] * length, root[2] - 1.0 * length - 1.0)])
    parts.fan(tip, start + (rings - 1) * sides, sides, 'rock', reverse=True)
    parts.fan(parts.add([(root[0], root[1], root[2] + 0.25 * length + 1.0)]), start, sides, 'rock')


def pick(loops, rows, count, spacing, rnd, where=None):
    """Points spread round the given rows of an underside's loops, at least `spacing` apart in plan (and where(p) true,
    if given)."""
    chosen = []
    for _ in range(400):
        row = loops[min(rnd.randint(*rows), len(loops) - 1)][2]
        p = row[rnd.randrange(len(row))]
        if where is not None and not where(p):
            continue
        if all(math.hypot(p[0] - q[0], p[1] - q[1]) > spacing for q in chosen):
            chosen.append(p)
        if len(chosen) == count:
            break
    return chosen


def spires(parts, isle, loops, count, rnd, center=(0.0, 0.0), length=(0.25, 0.55), width=(0.07, 0.13), rows=(2, 6),
           where=None):
    """Spires rooted in an underside's middle rows, spread round it."""
    for p in pick(loops, rows, count, isle.R * 0.35, rnd, where):
        out = Vector((p[0] - center[0], p[1] - center[1])).normalized()
        inward = (p[0] - out.x * 2.0, p[1] - out.y * 2.0, p[2])
        spire(parts, inward, isle.depth * rnd.uniform(*length), isle.R * rnd.uniform(*width),
              (out.x * rnd.uniform(0.0, 0.12), out.y * rnd.uniform(0.0, 0.12)), rnd)


def strand(parts, top, length, width, rnd, kind, out=(0.0, 0.0), segments=6):
    """A root or a vine: a three-sided strand hanging from top, swaying a little, tapering to a point."""
    start = len(parts.verts)
    phase = rnd.uniform(0.0, 2.0 * math.pi)
    side = Vector((-out[1], out[0], 0.0)) if out != (0.0, 0.0) else Vector((1.0, 0.0, 0.0))
    for r in range(segments):
        f = r / segments
        w = width * (1.0 - f) ** 0.8 + 0.04
        sway = side * (0.12 * length * math.sin(f * 4.0 + phase) * f) + Vector((out[0], out[1], 0.0)) * (1.5 * f)
        c = Vector(top) + sway + Vector((0.0, 0.0, -length * f))
        parts.add([tuple(c + Vector((math.cos(a) * w, math.sin(a) * w, 0.0))) for a in (0.0, 2.094, 4.189)])
    for r in range(segments - 1):
        parts.zipper(start + r * 3, 3, start + (r + 1) * 3, 3, kind)
    end = Vector(top) + Vector((out[0], out[1], 0.0)) * 1.6 + Vector((0.0, 0.0, -length))
    parts.fan(parts.add([tuple(end)]), start + (segments - 1) * 3, 3, kind, reverse=True)
    parts.fan(parts.add([tuple(Vector(top) + Vector((0.0, 0.0, 0.6)))]), start, 3, kind)


def roots(parts, loops, count, rnd, length=(10.0, 22.0), rows=(1, 4), spacing=10.0, where=None):
    """Roots hanging from an underside's upper rows."""
    for p in pick(loops, rows, count, spacing, rnd, where):
        strand(parts, (p[0] * 0.97, p[1] * 0.97, p[2] + 0.5), rnd.uniform(*length), rnd.uniform(0.35, 0.6), rnd,
               'root')


def vines(parts, edge_points, count, rnd, length=(8.0, 18.0), center=(0.0, 0.0)):
    """Vines spilling over the rim from the grass's edge, hanging out from the cliff."""
    for i in rnd.sample(range(len(edge_points)), count):
        p = edge_points[i]
        out = Vector((p[0] - center[0], p[1] - center[1])).normalized() * 0.6
        strand(parts, (p[0] + out.x * 0.5, p[1] + out.y * 0.5, p[2] + 0.2), rnd.uniform(*length),
               rnd.uniform(0.3, 0.5), rnd, 'vine', out=(out.x, out.y))


def crag(parts, isle, at, base, height, rnd, sides=7, origin=(0.0, 0.0, 0.0), squat=False):
    """A rock outcrop standing out of the grass: a rough, tapering, slightly leaning prism (squat: a low, wide one)."""
    x, y = at
    ground = origin[2] + isle.top_z(x - origin[0], y - origin[1])
    start = len(parts.verts)
    lean = (rnd.uniform(-0.15, 0.15), rnd.uniform(-0.15, 0.15))
    levels = ((-2.0, 1.0), (0.45, 0.9), (1.0, 0.62)) if squat else ((-2.0, 1.0), (0.35, 0.78), (0.75, 0.52),
                                                                    (1.0, 0.3))
    for f, scale in levels:
        z = ground + (f * height if f > 0 else f)
        pts = []
        for s in range(sides):
            a = 2.0 * math.pi * s / sides + rnd.uniform(-0.15, 0.15)
            r = base * scale * rnd.uniform(0.82, 1.15)
            pts.append((x + lean[0] * max(f, 0.0) * height + math.cos(a) * r,
                        y + lean[1] * max(f, 0.0) * height + math.sin(a) * r, z + rnd.uniform(-0.3, 0.3)))
        parts.add(pts)
    for k in range(len(levels) - 1):
        parts.zipper(start + k * sides, sides, start + (k + 1) * sides, sides, 'rock')
    top = parts.add([(x + lean[0] * height, y + lean[1] * height, ground + height + base * (0.04 if squat else 0.12))])
    parts.fan(top, start + (len(levels) - 1) * sides, sides, 'rock')
    parts.fan(parts.add([(x, y, ground - 2.5)]), start, sides, 'rock', reverse=True)


_ICO = None


def ico():
    """An icosahedron's corners and faces (12 and 20)."""
    global _ICO
    if _ICO is None:
        bm = bmesh.new()
        bmesh.ops.create_icosphere(bm, subdivisions=1, radius=1.0)
        _ICO = ([v.co.copy() for v in bm.verts], [[v.index for v in f.verts] for f in bm.faces])
        bm.free()
    return _ICO


def blob(parts, center, size, rnd, kind='grass', floor=-0.55):
    """A squashed, jittered icosahedron (20 triangles): a tree's crown, a puff of mist."""
    verts, faces = ico()
    sx, sy, sz = size
    yaw = rnd.uniform(0.0, 2.0 * math.pi)
    c, s = math.cos(yaw), math.sin(yaw)
    pts = []
    for v in verts:
        j = 1.0 + rnd.uniform(-0.12, 0.12)
        x, y, z = v.x * sx * j, v.y * sy * j, max(v.z, floor) * sz * j
        pts.append((center[0] + x * c - y * s, center[1] + x * s + y * c, center[2] + z))
    start = parts.add(pts)
    for f in faces:
        parts.face([start + i for i in f], kind)


def pine(parts, base, height, radius, rnd):
    """A dark conifer: two stacked six-sided cones (24 triangles)."""
    for lift, h, r in ((0.15, 0.7, 1.0), (0.45, 0.55, 0.7)):
        start = parts.add([(base[0] + math.cos(a) * radius * r, base[1] + math.sin(a) * radius * r,
                            base[2] + height * lift) for a in np.linspace(0.0, 2.0 * math.pi, 7)[:-1]])
        apex = parts.add([(base[0] + rnd.uniform(-0.3, 0.3), base[1] + rnd.uniform(-0.3, 0.3),
                           base[2] + height * (lift + h))])
        parts.fan(apex, start, 6, 'dark', reverse=True)
        parts.fan(parts.add([(base[0], base[1], base[2] + height * lift - 0.3)]), start, 6, 'dark')


def clump(parts, isle, at, count, rnd, size=(4.0, 7.0), origin=(0.0, 0.0, 0.0), pines=0):
    """A clump of trees, two-toned: rounded crowns, some tall, some of the darker green, and dark pines among them."""
    for k in range(count + pines):
        x = at[0] + rnd.uniform(-1.0, 1.0) * size[1] * 0.75
        y = at[1] + rnd.uniform(-1.0, 1.0) * size[1] * 0.75
        w = rnd.uniform(*size)
        ground = origin[2] + isle.top_z(x - origin[0], y - origin[1])
        if k >= count:
            pine(parts, (x, y, ground - 0.5), w * rnd.uniform(1.6, 2.1), w * 0.38, rnd)
            continue
        tall = rnd.random() < 0.4
        height = w * (rnd.uniform(0.95, 1.25) if tall else rnd.uniform(0.55, 0.75))
        blob(parts, (x, y, ground + height * 0.62), (w * 0.5, w * 0.5 * rnd.uniform(0.8, 1.0), height), rnd,
             'dark' if rnd.random() < 0.45 else 'grass')


def floater(parts, center, size, rnd):
    """A lump of rock adrift beside an island (20 triangles): flat-topped, drawn down to a point below."""
    verts, faces = ico()
    pts = []
    for v in verts:
        j = 1.0 + rnd.uniform(-0.2, 0.2)
        z = v.z * (1.6 if v.z < 0.0 else 0.55)
        pts.append((center[0] + v.x * size * j, center[1] + v.y * size * 0.8 * j, center[2] + z * size * j))
    start = parts.add(pts)
    for f in faces:
        parts.face([start + i for i in f], 'rock')


def path(parts, isle, points, width, kind='dirt', origin=(0.0, 0.0, 0.0), lift=0.35):
    """A strip lying on the grass along points: a pale dirt path or a band of bare ground."""
    pts = [Vector((p[0], p[1], 0.0)) for p in points]
    start = len(parts.verts)
    for i, p in enumerate(pts):
        a, b = pts[max(i - 1, 0)], pts[min(i + 1, len(pts) - 1)]
        tangent = (b - a).normalized()
        side = Vector((-tangent.y, tangent.x, 0.0)) * (width * (0.5 if 0 < i < len(pts) - 1 else 0.3))
        for q in (p - side, p + side):
            parts.add([(q.x, q.y, origin[2] + isle.top_z(q.x - origin[0], q.y - origin[1]) + lift)])
    for i in range(len(pts) - 1):
        a, b, c, d = start + 2 * i, start + 2 * i + 1, start + 2 * i + 3, start + 2 * i + 2
        parts.face((a, b, c), kind)
        parts.face((a, c, d), kind)


def patch(parts, isle, at, radius, rnd, kind='dirt', sides=7, origin=(0.0, 0.0, 0.0), lift=0.3):
    """A ragged disc of bare ground lying on the grass."""
    rim = []
    for s in range(sides):
        a = 2.0 * math.pi * s / sides
        r = radius * rnd.uniform(0.7, 1.1)
        x, y = at[0] + math.cos(a) * r, at[1] + math.sin(a) * r
        rim.append((x, y, origin[2] + isle.top_z(x - origin[0], y - origin[1]) + lift))
    start = parts.add(rim)
    parts.fan(parts.add([(at[0], at[1], origin[2] + isle.top_z(at[0] - origin[0], at[1] - origin[1]) + lift)]),
              start, sides, kind)


def box(parts, center, size, kind, yaw=0.0, tilt=(0.0, 0.0)):
    """A box (12 triangles) turned by yaw and tipped by tilt (degrees)."""
    m = (Matrix.Translation(Vector(center)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z') @
         Matrix.Rotation(math.radians(tilt[1]), 4, 'Y') @ Matrix.Rotation(math.radians(tilt[0]), 4, 'X'))
    hx, hy, hz = (s * 0.5 for s in size)
    start = parts.add([tuple(m @ Vector((x, y, z))) for z in (-hz, hz) for y in (-hy, hy) for x in (-hx, hx)])
    for quad in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
        a, b, c, d = (start + q for q in quad)
        parts.face((a, b, c), kind)
        parts.face((a, c, d), kind)


def watchtower(parts, isle, at, rnd, yaw=20.0, origin=(0.0, 0.0, 0.0)):
    """A ruined stone watchtower, 5.6 m square: walls broken off at different heights (one nearly gone), fallen blocks
    beside it and a bare patch around it, an echo of the tutorial island's ruined lookout."""
    x, y = at
    ground = origin[2] + isle.top_z(x - origin[0], y - origin[1]) - 0.6
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))

    def place(u, v):
        return x + u * c - v * s, y + u * s + v * c
    half, thick = 2.8, 0.85
    walls = [((0.0, half - thick * 0.5), (2 * half, thick), (8.6, 6.4)),      # each wall in two runs: broken heights
             ((half - thick * 0.5, 0.0), (thick, 2 * half - 2 * thick), (5.2, 3.0)),
             ((0.0, -half + thick * 0.5), (2 * half, thick), (2.2, 1.1)),
             ((-half + thick * 0.5, 0.0), (thick, 2 * half - 2 * thick), (7.4, 4.6))]
    for (u, v), (su, sv), (h1, h2) in walls:
        along_u = su > sv
        for k, h in enumerate((h1, h2)):
            off = (k - 0.5) * (su if along_u else sv) * 0.5
            cu, cv = (u + off, v) if along_u else (u, v + off)
            size = (su * 0.5, sv, h) if along_u else (su, sv * 0.5, h)
            px, py = place(cu, cv)
            box(parts, (px, py, ground + h * 0.5), size, 'ruin', yaw=yaw)
    for u, v, size in ((-4.4, -3.0, 1.3), (4.2, -3.8, 1.0), (-1.0, -5.0, 0.9), (5.0, 1.6, 1.1)):
        px, py = place(u, v)
        box(parts, (px, py, ground + 0.6 + size * 0.3), (size * 1.3, size, size * 0.8), 'ruin',
            yaw=rnd.uniform(0.0, 90.0), tilt=(rnd.uniform(-20.0, 20.0), rnd.uniform(-20.0, 20.0)))
    patch(parts, isle, at, 7.5, rnd, origin=origin, sides=8, lift=0.25)


def waterfall(parts, top, out, fall, width, rnd):
    """Falling water from a lip: a thin ribbon arcing out and down, breaking into three strands of spray that fade
    into a pale puff of mist."""
    out = Vector((out[0], out[1], 0.0)).normalized()
    side = Vector((-out.y, out.x, 0.0))
    top = Vector(top)

    def course(f):
        return top + out * (0.6 + 7.0 * f ** 0.6) + side * (0.5 * math.sin(f * 5.0 + 0.6) * f) + \
            Vector((0.0, 0.0, -fall * f))
    start = len(parts.verts)
    rings = 6
    for r in range(rings):
        f = 0.55 * r / (rings - 1)
        w = width * (1.0 + 0.3 * f) * rnd.uniform(0.9, 1.1)
        th = 0.45
        c = course(f)
        parts.add([tuple(c + side * (math.cos(a) * w * 0.5) + out * (math.sin(a) * th * 0.5))
                   for a in (0.0, 1.05, 2.1, 3.14159, 4.19, 5.24)])
    for r in range(rings - 1):
        parts.zipper(start + r * 6, 6, start + (r + 1) * 6, 6, 'water')
    parts.fan(parts.add([tuple(top + out * 0.3 + Vector((0.0, 0.0, 0.4)))]), start, 6, 'water')
    parts.fan(parts.add([tuple(course(0.55))]), start + (rings - 1) * 6, 6, 'water', reverse=True)
    # Spray: strands parting from the ribbon's end and thinning to nothing.
    for k, spread in enumerate((-1.0, 0.0, 1.0)):
        base = course(0.5) + side * (spread * width * 0.3)
        length = fall * rnd.uniform(0.35, 0.45)
        drift = side * spread * 0.18 + out * 0.12
        strand(parts, tuple(base), length, width * 0.22, rnd, 'water', out=(drift.x * 3.0, drift.y * 3.0), segments=5)
    mist = course(0.95)
    blob(parts, tuple(mist), (width * 1.6, width * 1.3, width * 1.0), rnd, 'water', floor=-1.0)
    blob(parts, tuple(mist + side * width * 0.9 + Vector((0.0, 0.0, -width * 0.6))),
         (width * 1.1, width * 1.0, width * 0.8), rnd, 'water', floor=-1.0)


# --- Making the model ---

def make(name, parts, seed, reach):
    """The model from its parts: material slots by kind, normals out (flat strips facing up), UVs by kind: the grass
    box-mapped, the rock wrapped round the island, the ruin box-mapped finer, the rest onto their patches."""
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(parts.verts, [], [f for f, _ in parts.faces])
    kind = mesh.attributes.new('kind', 'INT', 'FACE')
    kind.data.foreach_set('value', [KINDS.index(k) for _, k in parts.faces])
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-4)
    bmesh.ops.dissolve_degenerate(bm, dist=1e-4, edges=bm.edges[:])
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    layer = bm.faces.layers.int.get('kind')
    dirt = KINDS.index('dirt')
    for f in bm.faces:
        if f[layer] == dirt and f.normal.z < 0.0:
            f.normal_flip()
    bm.to_mesh(mesh)
    bm.free()
    kinds = np.empty(len(mesh.polygons), np.int32)
    mesh.attributes['kind'].data.foreach_get('value', kinds)
    mesh.attributes.remove(mesh.attributes['kind'])
    mesh.materials.append(ROCK)
    mesh.materials.append(GRASS)
    in_grass = np.isin(kinds, [KINDS.index(k) for k in GRASS_KINDS])
    mesh.polygons.foreach_set('material_index', in_grass.astype(np.int32))
    mesh.update()
    lm.smooth(obj, 50.0)
    by_kind = {k: [int(i) for i in np.nonzero(kinds == KINDS.index(k))[0]] for k in KINDS}
    if by_kind['grass']:
        lt.box_uv(obj, 'GroundGrass', texel_density=GRASS_DENSITY, faces=by_kind['grass'], seed=seed + 1)
    if by_kind['ruin']:
        lt.box_uv(obj, 'RockCliff', texel_density=RUIN_DENSITY, faces=by_kind['ruin'], seed=seed + 2)
    wrap_uv(obj, set(by_kind['rock']), ROCK_DENSITY, reach, seed)
    uv = mesh.uv_layers.active
    for k, (pu, pv) in PATCHES.items():
        for i in by_kind[k]:
            for j, li in enumerate(mesh.polygons[i].loop_indices):
                uv.data[li].uv = (pu + 0.002 * j, pv + 0.0015 * ((i + j) % 3))
    return obj, by_kind


def wrap_uv(obj, faces, density, reach, seed):
    """Cylindrical mapping of the rock around the island's axis, as under the tutorial island: U around (the angle
    times a fixed radius, `reach`, so no face shears), V up, so the strata stay level all round the cone; box mapping
    would flip between side and bottom on its slopes."""
    mesh = obj.data
    uv = mesh.uv_layers.active or mesh.uv_layers.new(name='UVMap')
    scale = density / 1024.0
    rng = random.Random(seed)
    du, dv = rng.uniform(0.0, 10.0), rng.uniform(0.0, 10.0)
    co = lo.vertices(obj)
    for i in faces:
        poly = mesh.polygons[i]
        corners = [(li, co[mesh.loops[li].vertex_index]) for li in poly.loop_indices]
        angles = [math.atan2(p[1], p[0]) for _, p in corners]
        if max(angles) - min(angles) > math.pi:
            angles = [a + 2.0 * math.pi if a < 0.0 else a for a in angles]
        for (li, p), a in zip(corners, angles):
            uv.data[li].uv = ((a * reach + du) * scale, (p[2] + dv) * scale)


def finish(obj, by_kind, depth):
    """Baked AO darkening toward the bottom (the sky lights an island from the side), water and mist kept bright,
    export properties."""
    mesh = obj.data
    lt.bake_vertex_ao(obj, samples=16, distance=6.0, ground=False)
    raw = lt._read_col(mesh)
    loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loop_vert)
    co = lo.vertices(obj)
    down = np.clip(-co[loop_vert, 2] / depth, 0.0, 1.0)
    raw[:, 3] = np.clip(raw[:, 3] * (1.0 - 0.32 * down), 0.0, 1.0)
    water = np.zeros(len(mesh.loops), bool)
    for i in by_kind['water']:
        water[list(mesh.polygons[i].loop_indices)] = True
    raw[water, 3] = np.maximum(raw[water, 3], 0.95)
    lt._write_col(mesh, raw)
    obj['Nanite'] = 0
    obj['LODs'] = '50,25'
    obj['Collision'] = 'None'
    return obj


def ring_points(parts, start, count):
    return [parts.verts[start + i] for i in range(count)]


# --- The islands ---

def island_a():
    """The arch: a broad isle whose underside hangs in two keels joined below by a bridge of rock, leaving a window
    through its body; a ruined watchtower on its hill with a dirt path down to the rim."""
    rnd = random.Random(5101)
    isle = Island(5101, 62.0, aspect=0.8, rim=7.0, depth=31.0, segments=46,
                  hills=[(16.0, 10.0, 24.0, 9.0), (-26.0, -12.0, 16.0, 3.5)], roll=1.5, lobes=0.12, rows=6,
                  ledges=(0.05, 7.0), profile=([0.0, 0.25, 0.55, 0.8, 1.0], [1.0, 0.92, 0.78, 0.64, 0.56]), sag=0.08)
    parts = Parts()
    _, edge = build_island(parts, isle)
    keels = []
    for seed, at, radius, height in ((5111, (-25.0, 3.0), 20.0, 52.0), (5112, (27.0, -4.0), 17.0, 43.0)):
        keel = Island(seed, radius, aspect=0.85, depth=height, segments=22, lobes=0.17, rows=9, ledges=(0.07, 6.0),
                      profile=TERRACED, keel=(rnd.uniform(-3.0, 3.0), rnd.uniform(-3.0, 3.0)))
        loops = mass(parts, keel, (at[0], at[1], 0.0), -24.0, height)
        keels.append((keel, loops, at))
        spires(parts, keel, loops, 2, rnd, center=at, length=(0.3, 0.55), width=(0.12, 0.2), rows=(3, 6))
        roots(parts, loops, 2, rnd, rows=(0, 2), length=(12.0, 20.0))
    # The bridge under the window: a sagging beam of rock from keel to keel.
    bridge = [(-12.0, 1.0, -49.0), (-5.0, 0.0, -53.0), (2.0, -1.0, -55.0), (9.0, -2.5, -53.0), (15.0, -3.5, -48.5)]
    tube(parts, bridge, [6.5, 5.2, 4.6, 5.0, 6.2], sides=8)
    strand(parts, (2.0, -1.0, -59.0), 11.0, 0.5, rnd, 'root')
    watchtower(parts, isle, (16.0, 10.0), rnd)
    path(parts, isle, [(13.0, 3.0), (6.0, -6.0), (-4.0, -14.0), (-12.0, -24.0), (-18.0, -36.0), (-21.0, -45.0)], 3.2)
    crag(parts, isle, (-30.0, -16.0), 7.0, 10.0, rnd)
    crag(parts, isle, (38.0, -14.0), 6.0, 2.8, rnd, squat=True)
    crag(parts, isle, (-8.0, 28.0), 5.0, 2.2, rnd, squat=True)
    for at, count, pines in (((-22.0, 14.0), 4, 2), ((26.0, -22.0), 3, 1), ((4.0, 30.0), 2, 2), ((-44.0, -2.0), 2, 1)):
        clump(parts, isle, at, count, rnd, size=(5.0, 9.0), pines=pines)
    vines(parts, ring_points(parts, edge, isle.segments), 5, rnd, length=(10.0, 20.0))
    floater(parts, (84.0, -12.0, -14.0), 5.0, rnd)
    floater(parts, (-80.0, 30.0, -32.0), 3.2, rnd)
    floater(parts, (70.0, 22.0, -46.0), 2.2, rnd)
    obj, kinds = make('SkyIsland_A', parts, 11, 0.6 * isle.R)
    return finish(obj, kinds, 85.0)


def tube(parts, points, radii, sides=8, kind='rock'):
    """A closed tube along points (a bridge of rock), rough round its section."""
    rnd = random.Random(len(points) * 7)
    start = len(parts.verts)
    pts = [Vector(p) for p in points]
    for i, (p, r) in enumerate(zip(pts, radii)):
        a, b = pts[max(i - 1, 0)], pts[min(i + 1, len(pts) - 1)]
        t = (b - a).normalized()
        u = t.cross(Vector((0.0, 0.0, 1.0))).normalized()
        v = u.cross(t).normalized()
        parts.add([tuple(p + (u * math.cos(ang) + v * math.sin(ang) * 0.85) * r * rnd.uniform(0.85, 1.12))
                   for ang in np.linspace(0.0, 2.0 * math.pi, sides + 1)[:-1]])
    for i in range(len(pts) - 1):
        parts.zipper(start + i * sides, sides, start + (i + 1) * sides, sides, kind)
    parts.fan(parts.add([tuple(pts[0] - (pts[1] - pts[0]).normalized() * radii[0] * 0.5)]), start, sides, kind,
              reverse=True)
    parts.fan(parts.add([tuple(pts[-1] + (pts[-1] - pts[-2]).normalized() * radii[-1] * 0.5)]),
              start + (len(pts) - 1) * sides, sides, kind)


def island_b():
    """The stack: two islands fused one above the other, the lower one's grass a terrace beside the upper's
    underside; a stream off the upper lip falls in a ribbon that breaks into spray and mist (at -X, -Y)."""
    rnd = random.Random(5202)
    seg = 40
    stream = 2.0 * math.pi * (22.5 / seg)
    upper = Island(5202, 37.0, aspect=0.8, rim=6.0, depth=38.0, segments=seg, hills=[(-8.0, 10.0, 18.0, 6.0)],
                   roll=1.3, lobes=0.17, keel=(-4.0, 2.0), rows=9, ledges=(0.06, 6.0), profile=TERRACED)
    parts = Parts()
    upper_loops, edge = build_island(parts, upper, notch=(stream, 1.6), stream=stream)
    lower_origin = (17.0, -7.0, -31.0)
    lower = Island(5206, 27.0, aspect=0.75, rim=5.0, depth=40.0, segments=28, hills=[(8.0, -4.0, 12.0, 3.0)],
                   roll=1.0, lobes=0.19, keel=(4.0, -3.0), rows=9, ledges=(0.07, 6.0))
    lower_loops, lower_edge = build_island(parts, lower, origin=lower_origin)
    clear = lambda p: p[0] < -8.0 or p[1] > 14.0          # the upper's underside away from the terrace below
    spires(parts, upper, upper_loops, 2, rnd, rows=(3, 6), length=(0.3, 0.5), where=clear)
    spires(parts, lower, lower_loops, 2, rnd, center=lower_origin[:2], rows=(3, 6), length=(0.35, 0.6))
    crag(parts, upper, (-20.0, 12.0), 7.5, 12.0, rnd)
    crag(parts, upper, (12.0, 16.0), 5.0, 2.4, rnd, squat=True)
    crag(parts, lower, (lower_origin[0] + 16.0, lower_origin[1] - 6.0), 4.0, 6.0, rnd, origin=lower_origin)
    for at, count, pines in (((10.0, 4.0), 3, 1), ((-6.0, -18.0), 2, 1), ((18.0, -16.0), 2, 0)):
        clump(parts, upper, at, count, rnd, size=(4.5, 8.0), pines=pines)
    clump(parts, lower, (lower_origin[0] + 12.0, lower_origin[1] - 12.0), 2, rnd, size=(4.0, 6.5),
          origin=lower_origin, pines=2)
    path(parts, upper, [(4.0, -4.0), (-6.0, -8.0), (-14.0, -14.0)], 2.6)
    patch(parts, lower, (lower_origin[0] + 4.0, lower_origin[1] - 16.0), 4.0, rnd, origin=lower_origin)
    rim = ring_points(parts, edge, seg)
    vines(parts, rim[33:] + rim[:2], 4, rnd, length=(12.0, 20.0))         # a curtain over the terrace below
    vines(parts, rim[8:20], 1, rnd, length=(10.0, 16.0))
    vines(parts, ring_points(parts, lower_edge, 28), 2, rnd, length=(8.0, 14.0), center=lower_origin[:2])
    roots(parts, lower_loops, 3, rnd, rows=(1, 3), length=(10.0, 18.0))
    roots(parts, upper_loops, 2, rnd, rows=(1, 3), length=(10.0, 16.0), where=clear)
    floater(parts, (-52.0, 18.0, -22.0), 3.6, rnd)
    tip(parts, 0, 2.5, -2.0)
    i = 22
    lip = (Vector(parts.verts[edge + i]) + Vector(parts.verts[edge + i + 1])) * 0.5
    out = Vector((math.cos(stream), math.sin(stream) * upper.aspect)).normalized()
    waterfall(parts, (lip.x, lip.y, lip.z - 0.4), (out.x, out.y), 48.0, 2.6, rnd)
    obj, kinds = make('SkyIsland_B', parts, 12, 0.6 * upper.R)
    return finish(obj, kinds, 80.0)


def tip(parts, start, angle_x, angle_y=0.0):
    """Tips every vertex from start on (an island hanging askew)."""
    m = Matrix.Rotation(math.radians(angle_y), 4, 'Y') @ Matrix.Rotation(math.radians(angle_x), 4, 'X')
    parts.transform(start, m)


def island_c():
    """The needle: an islet topped by a crag, a bare patch and a few trees over a slender underside drawn out into
    one long spire, roots trailing, rocks adrift."""
    rnd = random.Random(5303)
    isle = Island(5303, 24.0, aspect=0.9, rim=5.0, depth=50.0, segments=30, hills=[(-4.0, 3.0, 10.0, 3.0)],
                  roll=1.0, lobes=0.14, keel=(3.0, 2.0), rows=11, ledges=(0.07, 6.0))
    parts = Parts()
    loops, edge = build_island(parts, isle)
    bottom = loops[-2][2]
    root = (sum(p[0] for p in bottom) / len(bottom), sum(p[1] for p in bottom) / len(bottom), bottom[0][2])
    spire(parts, root, 30.0, 5.5, (0.06, 0.03), rnd, sides=7, rings=6)
    spires(parts, isle, loops, 2, rnd, length=(0.2, 0.35))
    crag(parts, isle, (4.0, -3.0), 7.0, 11.0, rnd)
    crag(parts, isle, (-12.0, -8.0), 3.5, 1.8, rnd, squat=True)
    patch(parts, isle, (-3.0, -10.0), 4.5, rnd)
    clump(parts, isle, (-9.0, 7.0), 2, rnd, size=(4.0, 6.5), pines=2)
    clump(parts, isle, (11.0, 9.0), 1, rnd, size=(4.0, 5.5), pines=1)
    roots(parts, loops, 3, rnd, rows=(1, 4), length=(9.0, 16.0), spacing=8.0)
    vines(parts, ring_points(parts, edge, isle.segments), 3, rnd, length=(7.0, 13.0))
    floater(parts, (34.0, 8.0, -12.0), 2.4, rnd)
    floater(parts, (-30.0, -14.0, -34.0), 1.8, rnd)
    floater(parts, (22.0, -26.0, -52.0), 1.4, rnd)
    obj, kinds = make('SkyIsland_C', parts, 13, 0.6 * isle.R)
    return finish(obj, kinds, isle.depth + 30.0)


def island_d():
    """The break: tipped well over, with a slab of its edge (out of a bite in its outline) breaking away and sliding
    down and out, roots torn across the gap, debris adrift."""
    rnd = random.Random(5404)
    bite_at = 0.35
    isle = Island(5404, 21.0, aspect=0.85, rim=4.5, depth=36.0, segments=32, hills=[(-4.0, -3.0, 10.0, 3.0)],
                  roll=0.9, lobes=0.17, keel=(-3.0, 1.0), rows=10, ledges=(0.08, 5.0), profile=TERRACED,
                  bite=(bite_at, 0.24, 0.55))
    parts = Parts()
    loops, edge = build_island(parts, isle)
    spires(parts, isle, loops, 3, rnd, length=(0.3, 0.55), rows=(4, 7))
    clump(parts, isle, (-7.0, 3.0), 3, rnd, size=(4.0, 7.0), pines=1)
    clump(parts, isle, (2.0, -10.0), 2, rnd, size=(3.5, 6.0), pines=1)
    crag(parts, isle, (-10.0, -8.0), 4.0, 2.0, rnd, squat=True)
    path(parts, isle, [(-14.0, 6.0), (-6.0, 0.0), (2.0, -2.0), (9.0, 1.0)], 2.2)
    vines(parts, ring_points(parts, edge, isle.segments), 4, rnd, length=(8.0, 15.0))
    roots(parts, loops, 3, rnd, rows=(1, 3), length=(9.0, 16.0))
    # The slab breaking away: a long, narrow island turned along the bite, slid down and out, tipped outward.
    start = len(parts.verts)
    slab = Island(5405, 9.5, aspect=0.42, rim=3.0, depth=15.0, segments=18, roll=0.5, lobes=0.12, rows=6,
                  ledges=(0.08, 4.0))
    slab_loops, slab_edge = build_island(parts, slab)
    clump(parts, slab, (-2.0, 0.5), 1, rnd, size=(3.5, 4.5), pines=1)
    spire(parts, (1.0, 0.0, -8.5), 7.0, 2.0, (0.04, 0.0), rnd)
    rim_r = isle.radius(bite_at)
    tangent = math.degrees(bite_at) + 90.0
    out = Vector((math.cos(bite_at), math.sin(bite_at) * isle.aspect)).normalized()
    place = (Matrix.Translation(Vector((out.x * (rim_r + 5.2), out.y * (rim_r + 5.2), -3.5))) @
             Matrix.Rotation(math.radians(tangent), 4, 'Z') @ Matrix.Rotation(math.radians(22.0), 4, 'X'))
    parts.transform(start, place)
    # Roots torn across the gap, and debris.
    for k in range(3):
        p = parts.verts[edge + int(bite_at / (2.0 * math.pi) * isle.segments) + k - 1]
        strand(parts, (p[0], p[1], p[2] - 1.0), rnd.uniform(9.0, 14.0), 0.45, rnd, 'root', out=(out.x, out.y))
    for k, (dist, drop, size) in enumerate(((rim_r + 9.0, -16.0, 1.6), (rim_r + 6.0, -24.0, 1.1),
                                            (rim_r + 13.0, -10.0, 0.9))):
        side = Vector((-out.y, out.x)) * ((k - 1) * 4.0)
        floater(parts, (out.x * dist + side.x, out.y * dist + side.y, drop), size, rnd)
    floater(parts, (-30.0, -10.0, -20.0), 2.0, rnd)
    tip(parts, 0, 6.0, 15.0)                              # tipped well over, toward the break
    obj, kinds = make('SkyIsland_D', parts, 14, 0.6 * isle.R)
    return finish(obj, kinds, isle.depth + 10.0)


ISLANDS = [island_a, island_b, island_c, island_d]


# --- Previews: against a sky, from below and far off ---

def sky_world():
    """A sky all round, as Skyreach sees it: warm haze at the horizon, blue above, and below it more haze (the sky
    the islands hang in)."""
    world = bpy.data.worlds.new('_SkyWorld')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    background = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    links.new(split.outputs['Z'], ramp.inputs['Fac'])
    elements = ramp.color_ramp.elements
    elements[0].position, elements[0].color = 0.0, lt.hex_color(0x8da3b6)
    elements[1].position, elements[1].color = 1.0, lt.hex_color(0x5d88c2)
    for position, color in ((0.44, 0xc3cdd0), (0.5, 0xe2e0d4), (0.57, 0xd3d9d8), (0.74, 0x95b4d8)):
        element = elements.new(position)
        element.color = lt.hex_color(color)
    links.new(ramp.outputs['Color'], background.inputs['Color'])
    links.new(background.outputs['Background'], out.inputs['Surface'])
    return world


def render(objects, path, eye, target, lens=50.0, sun=(-0.45, -0.6, 0.5), resolution=(1280, 720)):
    """Renders objects from eye toward target against the sky (Eevee, as lt.preview does)."""
    scene = bpy.context.scene
    hidden = {o: o.hide_render for o in scene.objects}
    for o in scene.objects:
        o.hide_render = o not in objects
    camera = bpy.data.objects.new('_SkyCamera', bpy.data.cameras.new('_SkyCamera'))
    camera.data.lens = lens
    camera.data.clip_start = 0.5
    camera.data.clip_end = 20000.0
    scene.collection.objects.link(camera)
    camera.location = eye
    camera.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
    light = bpy.data.objects.new('_SkySun', bpy.data.lights.new('_SkySun', 'SUN'))
    light.data.energy = 4.0
    light.data.color = (1.0, 0.9, 0.78)
    light.data.angle = math.radians(2.0)
    light.rotation_euler = (-Vector(sun).normalized()).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(light)
    world = sky_world()
    saved = (scene.camera, scene.world, scene.render.engine, scene.render.resolution_x, scene.render.resolution_y,
             scene.render.filepath, scene.view_settings.view_transform, scene.view_settings.look)
    try:
        scene.camera, scene.world = camera, world
        scene.render.resolution_x, scene.render.resolution_y = resolution
        scene.render.resolution_percentage = 100
        scene.render.film_transparent = False
        scene.render.image_settings.file_format = 'PNG'
        scene.render.image_settings.color_mode = 'RGB'
        scene.render.filepath = path
        scene.view_settings.view_transform = 'AgX'
        scene.view_settings.look = 'AgX - Medium High Contrast'
        scene.render.engine = 'BLENDER_EEVEE_NEXT'
        scene.eevee.taa_render_samples = 32
        import os
        os.makedirs(os.path.dirname(path), exist_ok=True)
        bpy.ops.render.render(write_still=True)
    finally:
        (scene.camera, scene.world, scene.render.engine, scene.render.resolution_x, scene.render.resolution_y,
         scene.render.filepath, scene.view_settings.view_transform, scene.view_settings.look) = saved
        bpy.data.worlds.remove(world)
        for o in (camera, light):
            data = o.data
            bpy.data.objects.remove(o)
            if isinstance(data, bpy.types.Camera):
                bpy.data.cameras.remove(data)
            else:
                bpy.data.lights.remove(data)
        for o, value in hidden.items():
            if o.name in scene.objects:
                o.hide_render = value
    lt._log(f'preview: {path}')


def view_from_below(obj, path, direction=(-0.9, -1.5, -0.3)):
    """One island from below at a three-quarter angle, from far enough off that it fills about half the frame."""
    lo_, hi_ = lo.bounds([obj])
    center = Vector(((lo_ + hi_) * 0.5).tolist())
    radius = float(np.linalg.norm(hi_ - lo_)) * 0.5
    distance = radius / math.sin(math.radians(11.0)) * 1.15
    render([obj], path, center + Vector(direction).normalized() * distance, center, lens=50.0)


def overview(objects, path):
    """All four as Skyreach's jetty would see them: hanging in the sky 300-650 m out, some above eye level, some
    below, the camera at a person's height on the jetty."""
    spots = {'SkyIsland_A': ((-190.0, 640.0, 70.0), 180.0), 'SkyIsland_B': ((95.0, 360.0, -5.0), 20.0),
             'SkyIsland_C': ((300.0, 560.0, 95.0), 40.0), 'SkyIsland_D': ((-40.0, 300.0, 30.0), 180.0)}
    saved = {o: o.matrix_world.copy() for o in objects}
    try:
        for o in objects:
            at, yaw = spots[o.name]
            o.matrix_world = Matrix.Translation(Vector(at)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z')
        bpy.context.view_layer.update()
        render(objects, path, (0.0, 0.0, 2.0), (40.0, 480.0, 30.0), lens=30.0, sun=(-0.6, -0.35, 0.45))
    finally:
        for o, m in saved.items():
            o.matrix_world = m
        bpy.context.view_layer.update()


models = [build() for build in ISLANDS]
for model in models:
    v = lo.vertices(model)
    size = v.max(axis=0) - v.min(axis=0)
    lo.log(f'{model.name}: {len(model.data.polygons)} triangles, {size[0]:.0f} x {size[1]:.0f} m, '
           f'z {v[:, 2].min():.0f} to {v[:, 2].max():.0f}, materials {", ".join(m.name for m in model.data.materials)}')

BELOW = {'SkyIsland_D': (0.25, -1.6, -0.3)}      # D shows its break (at +X) in profile from the front

if lt.want_preview():
    for model in models:
        path = lt.preview_path('RansomsRest/SkyIslands', model.name)
        view_from_below(model, path, BELOW.get(model.name, (-0.9, -1.5, -0.3)))
        view_from_below(model, path.replace('.png', '_Top.png'), (-0.9, -1.5, 0.75))
    overview(models, lt.preview_path('RansomsRest', 'SkyIslands_overview'))
