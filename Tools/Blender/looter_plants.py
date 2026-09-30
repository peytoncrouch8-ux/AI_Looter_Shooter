"""Helpers for scripted vegetation (Art/Models/Vegetation/<Name>.py): trees, bushes, undergrowth and ground cover
built from bark tubes, leaf-cluster cards and opaque blades, finished with the shading normals, foliage vertex colors
and occlusion the stylized materials read. Every vegetation model imports it, with looter_textures for its materials:

    import looter_plants as lp
    import looter_textures as lt

    rng = random.Random(11)
    plant = lp.Plant('Oak_A', 11)
    trunk = lp.Limb(points, 0.4, 0.0, 10, wind=(0.0, 0.3))           # a centerline, radius and tip radius, sides
    plant.tube(trunk, lt.material('BarkOak'), lp.root_flare(0.8, 5, 0.3))
    for center, radius in lp.layout_clumps(rng, crown_center, crown_radii, 9, (1.8, 2.3)):
        lp.leaf_clump(plant, rng, center, radius, crown_center, lt.material('LeavesOak'), 30, (1.7, 2.2))
    oak = lp.finish(plant, hull_points)                                # the model object, ready to export
    lp.preview_all([oak])                                              # with --preview after '--'

Distances are meters, Z is up and a model's front faces -Y (Art/README.md). A model's origin is where it meets the
ground; trunks and stems reach a little below it so they sit on slopes. Everything is seeded (random.Random and
mathutils' noise(); never noise_vector(), whose offset is seeded from the clock), so a model comes out the same on
every run.

  Limb, curved_limb(), gnarl()   a branch's centerline, with a radius and a wind weight at each point
  Plant                          one model's mesh: tube() (bark), card() (a leaf-cluster card: one quadrant of a 2x2
                                 atlas, its stem at the bottom middle), ball() (fruit), and per vertex a wind weight,
                                 an occlusion guess and, for cards and blades, a shading normal
  crown_shade(), crown_occlusion(), layout_clumps(), leaf_clump()
                                 crowns: leaf clumps whose cards shade with normals out of the clump and crown centers,
                                 so a crown lights like a few soft volumes (M_WorldFoliage keeps a back face's normal)
  blade(), petal(), swatch(), up_normal()
                                 opaque ground cover on FoliagePalette's gradient swatches (lt.PALETTE), shaded mostly
                                 up so it lights like the ground it stands on
  finish()                       the model object: normals, the foliage vertex colors (lt.set_foliage_colors: R wind
                                 weight, G a random value per card, blade or branch, A occlusion, baked with
                                 lt.bake_vertex_ao and blended with the plant's own guess), a convex UCX_ hull or
                                 Collision 'none', Nanite off, and optional LODs
  preview_all(), lineup(), report()
                                 previews in Saved/ArtPreviews/Vegetation/ (lt.preview) and the triangle counts
"""
import math
import random
import sys

import bmesh
import bpy
from mathutils import Vector, kdtree, noise

import looter_textures as lt

UP = Vector((0.0, 0.0, 1.0))
# Bark textures are 1024 px at 320 px per meter: a tile is 3.2 m of bark.
BARK_TILE = 3.2


# --- Small math ---

def lerp(a, b, t):
    return a + (b - a) * t


def smoothstep(lo, hi, x):
    t = min(max((x - lo) / (hi - lo), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def random_unit(rng):
    """A random direction, uniform over the sphere."""
    z = rng.uniform(-1.0, 1.0)
    a = rng.uniform(0.0, 2.0 * math.pi)
    s = math.sqrt(max(1.0 - z * z, 0.0))
    return Vector((s * math.cos(a), s * math.sin(a), z))


def perpendicular(v):
    other = UP if abs(v.z) < 0.9 else Vector((1.0, 0.0, 0.0))
    return v.cross(other).normalized()


def horizontal(angle, length=1.0):
    return Vector((math.cos(angle) * length, math.sin(angle) * length, 0.0))


def bezier(p0, p1, p2, p3, params):
    """Points on a cubic Bezier curve at the parameters (0..1)."""
    points = []
    for t in params:
        u = 1.0 - t
        points.append(p0 * (u * u * u) + p1 * (3.0 * u * u * t) + p2 * (3.0 * u * t * t) + p3 * (t * t * t))
    return points


def steps(count):
    return [i / count for i in range(count + 1)]


def smooth_vector(p):
    """Smooth noise, a vector of about -0.5..0.5 per axis. (mathutils' own noise_vector adds an offset seeded from the
    clock, so it differs from run to run; plain noise() doesn't.)"""
    return Vector((noise.noise(p), noise.noise(p + Vector((31.4, 0.0, 0.0))),
                   noise.noise(p + Vector((0.0, 47.2, 0.0)))))


def gnarl(points, amount, frequency, seed, keep_base=True):
    """Bends a centerline with smooth noise (amount in meters). The base stays put, where it joins its parent."""
    offset = Vector((seed * 7.31, seed * 3.17, seed * 5.53))
    count = len(points) - 1
    bent = []
    for i, p in enumerate(points):
        weight = min(i / max(count * 0.3, 1e-6), 1.0) if keep_base else 1.0
        bent.append(p + smooth_vector(p * frequency + offset) * amount * weight)
    return bent


# --- Branches ---

class Limb:
    """A branch: its centerline from base to tip, with a radius and a wind weight at each point. A tip radius of 0
    closes the tube to a point."""

    def __init__(self, points, radius, tip_radius, sides, wind=(0.0, 0.5), taper=1.0):
        self.points = points
        lengths = [0.0]
        for a, b in zip(points, points[1:]):
            lengths.append(lengths[-1] + (b - a).length)
        self.length = max(lengths[-1], 1e-6)
        self.params = [d / self.length for d in lengths]
        self.radii = [lerp(radius, tip_radius, t ** taper) for t in self.params]
        self.winds = [lerp(wind[0], wind[1], t) for t in self.params]
        self.sides = sides

    def param_at_height(self, z):
        """Where (0..1, by length) the limb first reaches height z (its tip if it never does)."""
        for i in range(len(self.points) - 1):
            a, b = self.points[i].z, self.points[i + 1].z
            if (a - z) * (b - z) <= 0.0 and a != b:
                return lerp(self.params[i], self.params[i + 1], (z - a) / (b - a))
        return 1.0 if z > self.points[0].z else 0.0

    def at(self, t):
        """(point, radius, wind, direction) at t along the limb (0 base, 1 tip), by length."""
        t = min(max(t, 0.0), 1.0)
        for i in range(len(self.points) - 1):
            a, b = self.params[i], self.params[i + 1]
            if t <= b or i == len(self.points) - 2:
                f = 0.0 if b - a < 1e-9 else (t - a) / (b - a)
                point = self.points[i].lerp(self.points[i + 1], f)
                direction = (self.points[i + 1] - self.points[i]).normalized()
                return (point, lerp(self.radii[i], self.radii[i + 1], f), lerp(self.winds[i], self.winds[i + 1], f),
                        direction)


def curved_limb(start, end, out, up_bias, sides, radius, tip_radius, wind, rng, gnarl_amount=0.0, segment=0.45,
                bow=0.35, seed=0):
    """A limb from start to end that leaves start along out (a direction) and arrives bowed upward (up_bias)."""
    span = (end - start).length
    count = max(2, int(math.ceil(span / segment)))
    p1 = start + out.normalized() * span * bow
    p2 = end - (end - start).normalized() * span * 0.3 + UP * span * up_bias
    points = bezier(start, p1, p2, end, steps(count))
    if gnarl_amount > 0.0:
        points = gnarl(points, gnarl_amount, 0.9 / max(span, 0.5) + 0.25, seed)
    return Limb(points, radius, tip_radius, sides, wind)


# --- One model's mesh ---

class Plant:
    """One model being built: a bmesh with UVs, and per vertex a wind weight, an occlusion guess (1 open) and, for
    cards and blades, a shading normal (the rest keep their own smooth normals)."""

    def __init__(self, name, seed=0):
        self.name = name
        self.seed = seed
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')
        self.materials = []
        self.winds = []
        self.normals = []
        self.occlusion = []

    def slot(self, mat):
        if mat not in self.materials:
            self.materials.append(mat)
        return self.materials.index(mat)

    def vert(self, co, wind=0.0, normal=None, occlusion=1.0):
        vertex = self.bm.verts.new(co)
        self.winds.append(wind)
        self.normals.append(normal)
        self.occlusion.append(occlusion)
        return vertex

    def face(self, verts, uvs, mat):
        face = self.bm.faces.new(verts)
        face.material_index = self.slot(mat)
        face.smooth = True
        for loop, uv in zip(face.loops, uvs):
            loop[self.uv].uv = uv
        return face

    def tube(self, limb, mat, flare=None, occlusion=None, tile=BARK_TILE, u_offset=0.0, cap=None):
        """limb as a tapered tube. Bark wraps a whole number of times around its base and keeps square texels along
        it, so the pattern gets finer as the branch thins. flare(point, angle) scales the radius (root buttresses);
        occlusion(point) guesses how open each vertex is. A tip that doesn't close to a point gets a cap: a jagged
        break by default, or cap(plant, ring, center, direction) to make another end (a saw cut)."""
        pts, radii, winds, sides = limb.points, limb.radii, limb.winds, limb.sides
        count = len(pts)
        tangents = [(pts[min(i + 1, count - 1)] - pts[max(i - 1, 0)]).normalized() for i in range(count)]
        side = perpendicular(tangents[0])
        u_repeat = max(1, round(2.0 * math.pi * radii[0] / tile))
        rings, vs, v = [], [], u_offset
        for i in range(count):
            t = tangents[i]
            if i:
                # Parallel transport: the ring's frame turns with the curve, so the tube never twists.
                side = tangents[i - 1].rotation_difference(t) @ side
                side = (side - t * side.dot(t)).normalized()
                around = max(math.pi * (radii[i] + radii[i - 1]), tile * 0.06)
                v += (pts[i] - pts[i - 1]).length * u_repeat / around
            other = t.cross(side)
            vs.append(v)
            if radii[i] < 1e-4:
                rings.append([self.vert(pts[i], winds[i], None, occlusion(pts[i]) if occlusion else 1.0)])
                continue
            ring = []
            for s in range(sides):
                a = 2.0 * math.pi * s / sides
                r = radii[i] * (flare(pts[i], a) if flare else 1.0)
                p = pts[i] + (side * math.cos(a) + other * math.sin(a)) * r
                ring.append(self.vert(p, winds[i], None, occlusion(p) if occlusion else 1.0))
            rings.append(ring)
        for i in range(count - 1):
            lower, upper = rings[i], rings[i + 1]
            for s in range(sides):
                s1 = (s + 1) % sides
                u0, u1 = s / sides * u_repeat, (s + 1) / sides * u_repeat
                if len(upper) == 1:
                    self.face((lower[s], lower[s1], upper[0]),
                              [(u0, vs[i]), (u1, vs[i]), ((u0 + u1) * 0.5, vs[i + 1])], mat)
                else:
                    self.face((lower[s], lower[s1], upper[s1], upper[s]),
                              [(u0, vs[i]), (u1, vs[i]), (u1, vs[i + 1]), (u0, vs[i + 1])], mat)
        if len(rings[-1]) > 1:
            if cap is not None:
                cap(self, rings[-1], pts[-1], tangents[-1])
            else:
                self.break_cap(rings[-1], pts[-1], tangents[-1], radii[-1], mat, winds[-1], u_repeat, vs[-1])

    def break_cap(self, ring, center, direction, radius, mat, wind, u_repeat, v):
        """Closes a snapped branch: a ring of splinters of uneven length, then a low torn middle."""
        rng = random.Random(len(self.winds))
        sides = len(ring)
        splinters = []
        for s, vertex in enumerate(ring):
            out = (vertex.co - center) * rng.uniform(0.55, 0.8)
            splinters.append(self.vert(center + out + direction * radius * rng.uniform(0.1, 0.9) ** 2 * 1.6, wind, None,
                                       0.8))
        tip = self.vert(center + direction * radius * 0.25, wind, None, 0.7)
        for s in range(sides):
            s1 = (s + 1) % sides
            u0, u1 = s / sides * u_repeat, (s + 1) / sides * u_repeat
            self.face((ring[s], ring[s1], splinters[s1], splinters[s]),
                      [(u0, v), (u1, v), (u1, v + 0.05), (u0, v + 0.05)], mat)
            self.face((splinters[s], splinters[s1], tip), [(u0, v + 0.05), (u1, v + 0.05), ((u0 + u1) * 0.5, v + 0.08)],
                      mat)

    def card(self, base, facing, up, width, height, mat, quadrant, shade, occlusion=None, wind=(0.75, 1.0), fold=0.12,
             droop=0.0, mirror=False, rows=1):
        """A leaf-cluster card: one quadrant (0-3) of a 2x2 atlas, its bottom edge centered on base (where the twig
        attaches), rising along up and facing facing. Folded along its middle line (fold, a share of its width) for
        volume from the side; droop bends its tip down (a share of its height; needs rows=2). shade(point) gives each
        vertex's shading normal."""
        f = facing.normalized()
        y = up - f * up.dot(f)
        y = y.normalized() if y.length > 1e-6 else perpendicular(f)
        x = y.cross(f)
        qx, qy = (quadrant % 2) * 0.5, (quadrant // 2) * 0.5
        inset = 0.004
        grid = []
        for r in range(rows + 1):
            t = r / rows
            row = []
            for c in range(3):
                s = c * 0.5
                p = (base + x * ((s - 0.5) * width) + y * (t * height) + f * (fold * width * (1.0 - abs(2.0 * s - 1.0)))
                     - UP * (droop * height * t * t))
                w = lerp(wind[0], wind[1], t)
                uv = (qx + inset + ((1.0 - s) if mirror else s) * (0.5 - 2.0 * inset),
                      qy + inset + t * (0.5 - 2.0 * inset))
                row.append((self.vert(p, w, shade(p), occlusion(p) if occlusion else 1.0), uv))
            grid.append(row)
        for r in range(rows):
            for c in range(2):
                corners = (grid[r][c], grid[r][c + 1], grid[r + 1][c + 1], grid[r + 1][c])
                self.face([v for v, _ in corners], [uv for _, uv in corners], mat)

    def ball(self, center, radius, mat, uv, wind=1.0, occlusion=1.0, segments=6, rings=4):
        """A small low-poly sphere (an apple, a berry) mapped onto one point of a texture."""
        top = self.vert(center + UP * radius, wind, None, occlusion)
        bottom = self.vert(center - UP * radius, wind, None, occlusion)
        bands = []
        for r in range(1, rings):
            phi = math.pi * r / rings
            bands.append([self.vert(center + Vector((math.sin(phi) * math.cos(2 * math.pi * s / segments),
                                                     math.sin(phi) * math.sin(2 * math.pi * s / segments),
                                                     math.cos(phi))) * radius, wind, None, occlusion)
                          for s in range(segments)])
        for s in range(segments):
            s1 = (s + 1) % segments
            self.face((top, bands[0][s], bands[0][s1]), [uv] * 3, mat)
            self.face((bottom, bands[-1][s1], bands[-1][s]), [uv] * 3, mat)
            for b in range(len(bands) - 1):
                self.face((bands[b][s], bands[b + 1][s], bands[b + 1][s1], bands[b][s1]), [uv] * 4, mat)


# --- Crowns ---

def crown_shade(clump_center, crown_center, clump_share=0.5):
    """Shading normals for a leaf clump: the direction from the clump's center blended with the direction from the
    crown's, so each clump reads as a soft ball and the crown as one volume."""
    def shade(p):
        a = (p - clump_center)
        b = (p - crown_center)
        a = a.normalized() if a.length > 1e-6 else UP
        b = b.normalized() if b.length > 1e-6 else UP
        return (a * clump_share + b * (1.0 - clump_share)).normalized()
    return shade


def crown_occlusion(center, radii, floor=0.4):
    """How open a point is in a crown (an ellipsoid): dark deep inside and underneath, open on the outer shell."""
    def occlusion(p):
        d = p - center
        e = math.sqrt((d.x / radii[0]) ** 2 + (d.y / radii[1]) ** 2 + (d.z / radii[2]) ** 2)
        depth = lerp(floor, 1.0, smoothstep(0.25, 1.0, e))
        below = lerp(0.72, 1.0, smoothstep(-0.9, 0.3, d.z / radii[2]))
        return depth * below
    return occlusion


def root_flare(strength, lobes, phase, height=0.9):
    """Root buttresses: the trunk widens toward the ground in lobes (flare(point, angle) for Plant.tube)."""
    def flare(p, a):
        near = (1.0 - smoothstep(-0.35, height, p.z)) ** 2
        lobe = max(0.0, math.cos(lobes * a + phase)) ** 3
        return 1.0 + strength * near * (0.35 + 0.65 * lobe)
    return flare


def spiral(count, low=-1.0, offset=0.0):
    """count directions spread evenly over the sphere (a golden spiral), only those with z above low."""
    golden = math.pi * (3.0 - math.sqrt(5.0))
    total = max(int(round(count * 2.0 / (1.0 - low))), count)
    found = []
    for i in range(total):
        z = 1.0 - (i + 0.5) / total * 2.0
        if z < low:
            continue
        r = math.sqrt(max(1.0 - z * z, 0.0))
        a = offset + golden * i
        found.append(Vector((r * math.cos(a), r * math.sin(a), z)))
    return found


def layout_clumps(rng, center, radii, count, size, low=-0.35, jitter=0.35):
    """Leaf clumps (center, radius) spread over the upper part of the crown's ellipsoid, each reaching its surface."""
    clumps = []
    for d in spiral(count, low, rng.uniform(0.0, 2.0 * math.pi)):
        radius = rng.uniform(*size)
        reach = Vector([max(r - radius * 0.8, 0.2) for r in radii])
        c = center + Vector((d.x * reach.x, d.y * reach.y, d.z * reach.z)) + random_unit(rng) * radius * jitter
        clumps.append((c, radius))
    return clumps


def clump_occlusion(center, radius, crown):
    """How open a point of a leaf clump is: the crown's guess, darker toward the clump's core and underside, so each
    clump reads as its own leaf mass."""
    def occlusion(p):
        d = p - center
        core = lerp(0.55, 1.0, smoothstep(0.35, 0.95, d.length / radius))
        under = lerp(0.8, 1.0, smoothstep(-0.8, 0.2, d.z / radius))
        return (crown(p) if crown else 1.0) * core * under
    return occlusion


def leaf_clump(plant, rng, center, radius, crown_center, mat, cards, size, others=(), occlusion=None, droop=0.0,
               rows=1, shell=(0.5, 1.0), hanging=0.0, fold=0.14, clump_share=0.65, wind=(0.7, 1.0)):
    """Covers a clump with leaf cards facing mostly outward (a random tilt each), most near its surface and a few
    deeper so it doesn't look hollow, skipping spots buried inside other clumps. hanging is the share of the cards
    underneath that hang tip-down, like birch twigs."""
    shade = crown_shade(center, crown_center, clump_share)
    occlusion = clump_occlusion(center, radius, occlusion)
    for d in spiral(cards, -0.8, rng.uniform(0.0, 2.0 * math.pi)):
        d = (d + random_unit(rng) * 0.25).normalized()
        middle = center + d * radius * lerp(shell[0], shell[1], rng.random() ** 0.5)
        if any((middle - c).length < r * 0.72 for c, r in others):
            continue
        s = rng.uniform(*size)
        if d.z < -0.2 and rng.random() < hanging:
            facing = (horizontal(math.atan2(d.y, d.x)) + random_unit(rng) * 0.3).normalized()
            up = -UP + d * 0.3 + random_unit(rng) * 0.2
            plant.card(middle + UP * s * 0.35, facing, up, s * rng.uniform(0.75, 0.95), s * 1.1, mat, rng.randrange(4),
                       shade, occlusion, wind, fold, 0.0, rng.random() < 0.5, rows)
            continue
        facing = (d + random_unit(rng) * 0.45).normalized()
        up = UP * 0.6 + d * 0.6 + random_unit(rng) * 0.5
        up = up - facing * up.dot(facing)
        up = up.normalized() if up.length > 1e-4 else perpendicular(facing)
        plant.card(middle - up * s * 0.5, facing, up, s * rng.uniform(0.9, 1.1), s, mat, rng.randrange(4), shade,
                   occlusion, wind, fold, droop, rng.random() < 0.5, rows)


# --- Ground cover: opaque blades and petals on FoliagePalette swatches ---

def swatch(name, u, across):
    """A UV on the FoliagePalette swatch name: u from root (0) to tip (1), across from one edge (0) to the other (1)."""
    rows = len(lt.PALETTE)
    index = lt.PALETTE.index(name)
    return (min(max(u, 0.02), 0.98), (index + 0.15 + 0.7 * min(max(across, 0.0), 1.0)) / rows)


def up_normal(center, radius, lean=0.45):
    """Shading normals for ground cover: up, tilted out of the clump's middle by up to lean at its edge."""
    def shade(p):
        out = Vector((p.x - center.x, p.y - center.y, 0.0)) / max(radius, 1e-3)
        return (UP + out * lean).normalized()
    return shade


def blade(plant, mat, root, heading, height, width, lean, color, rng, segments=2, shade=None, twist=0.3,
          wind=(0.0, 1.0), occlusion=(0.7, 1.0), taper=0.8, tip=None):
    """A grass blade (or a leaf, a reed) from root: it rises height meters and bends toward heading by lean (a share
    of its height), narrowing from width to a point, its flat side facing the way it bends. color is the swatch; tip
    another swatch for the top segment (dry tips)."""
    heading = Vector((heading.x, heading.y, 0.0)).normalized()
    side = UP.cross(heading)
    a = rng.uniform(-twist, twist)
    side = (side * math.cos(a) + heading * math.sin(a)).normalized()
    rows = []
    for i in range(segments + 1):
        t = i / segments
        p = root + heading * (lean * height * t * t) + UP * (height * t * (1.0 - 0.35 * lean * lean * t))
        w = width * (1.0 - t) ** taper if i < segments else 0.0
        rows.append((t, p, w))
    verts = []
    for t, p, w in rows:
        o = lerp(occlusion[0], occlusion[1], t ** 0.8)
        n = shade(p) if shade else UP
        wind_t = lerp(wind[0], wind[1], t ** 1.5)
        if w > 0.0:
            verts.append((plant.vert(p - side * w * 0.5, wind_t, n, o),
                          plant.vert(p + side * w * 0.5, wind_t, n, o), t))
        else:
            verts.append((plant.vert(p, wind_t, n, o), None, t))
    for i in range(segments):
        (l0, r0, t0), (l1, r1, t1) = verts[i], verts[i + 1]
        c = tip if (tip and i == segments - 1) else color
        if r1 is None:
            plant.face((l0, r0, l1), [swatch(c, t0, 0.0), swatch(c, t0, 1.0), swatch(c, t1, 0.5)], mat)
        else:
            plant.face((l0, r0, r1, l1),
                       [swatch(c, t0, 0.0), swatch(c, t0, 1.0), swatch(c, t1, 1.0), swatch(c, t1, 0.0)], mat)
    return rows[-1][1]


def petal(plant, mat, center, direction, length, width, color, normal, wind=1.0, occlusion=1.0, cup=0.0, round=False):
    """A petal (or a leaflet) from center out along direction, cupped up by cup (a share of its length): a pointed
    diamond of two triangles, widest at 40%, or with round=True a rounded one of three, widest at 60%. Its swatch
    runs from the flower's middle (U 0) to the petal's tip."""
    d = direction.normalized()
    side = normal.cross(d).normalized()

    def at(along, across):
        return center + d * length * along + side * width * across + normal * length * cup * along * along

    if round:
        outline = [(0.55, -0.5), (0.95, -0.3), (0.95, 0.3), (0.55, 0.5)]
    else:
        outline = [(0.4, -0.5), (1.0, 0.0), (0.4, 0.5)]
    verts = [plant.vert(center, wind, normal, occlusion * 0.85)] + [plant.vert(at(a, c), wind, normal, occlusion)
                                                                   for a, c in outline]
    uvs = [swatch(color, 0.0, 0.5)] + [swatch(color, a, c + 0.5) for a, c in outline]
    for i in range(1, len(verts) - 1):
        plant.face((verts[0], verts[i], verts[i + 1]), [uvs[0], uvs[i], uvs[i + 1]], mat)


# --- Finishing a model ---

def hull(parent, points, name=None):
    """A convex collision hull (UCX_) around points, parented to the model."""
    bm = bmesh.new()
    for p in points:
        bm.verts.new(p)
    result = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    mesh = bpy.data.meshes.new('UCX_' + (name or parent.name))
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new('UCX_' + (name or parent.name), mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.parent = parent
    obj.display_type = 'WIRE'
    return obj


def cylinder_points(bottom, top, radius, sides=8):
    """The corners of an upright prism from bottom to top (points), for hull()."""
    points = []
    axis = (top - bottom).normalized()
    side = perpendicular(axis)
    other = axis.cross(side)
    for end in (bottom, top):
        for s in range(sides):
            a = 2.0 * math.pi * s / sides
            points.append(end + (side * math.cos(a) + other * math.sin(a)) * radius)
    return points


def finish(plant, hull_points=None, ao_distance=1.0, ao_samples=24, ao_blend=0.5, lods=None, lod_screens=None):
    """Turns plant into its model object: shading normals, the foliage vertex colors (wind, a random value per card
    or blade), baked occlusion blended with the plant's own guess (ao_blend: 0 baked only, 1 the guess only, which
    skips the bake), a collision hull around hull_points (none: walk-through), and the export properties (no Nanite;
    LODs as '40,12')."""
    import numpy as np
    mesh = bpy.data.meshes.new(plant.name)
    plant.bm.to_mesh(mesh)
    plant.bm.free()
    obj = bpy.data.objects.new(plant.name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for mat in plant.materials:
        mesh.materials.append(mat)

    # Cards and blades shade with their volume's normals; bark keeps its smooth ones (a zero vector keeps them).
    loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loop_vert)
    mesh.normals_split_custom_set([tuple(plant.normals[i]) if plant.normals[i] is not None else (0.0, 0.0, 0.0)
                                   for i in loop_vert])

    tree = kdtree.KDTree(len(mesh.vertices))
    for vertex in mesh.vertices:
        tree.insert(vertex.co, vertex.index)
    tree.balance()
    lt.set_foliage_colors(obj, wind=lambda p: plant.winds[tree.find(p)[1]], variation='island', seed=plant.seed)
    if ao_blend < 1.0:
        lt.bake_vertex_ao(obj, samples=ao_samples, distance=ao_distance, ground=True)

    # The alpha lt keeps is the bake (1 where there was none); the plant's guess is blended in. 'Col' holds values
    # through its linear accessor, as lt writes it and the exporter writes it out.
    col = mesh.color_attributes['Col']
    values = np.empty(4 * len(mesh.loops), dtype=np.float32)
    col.data.foreach_get('color', values)
    values = values.reshape(-1, 4)
    guess = np.array(plant.occlusion, dtype=np.float32)[loop_vert]
    values[:, 3] = np.clip(values[:, 3] * (1.0 - ao_blend) + guess * ao_blend, 0.0, 1.0)
    col.data.foreach_set('color', values.ravel())
    mesh.update()

    if hull_points:
        hull(obj, hull_points)
    else:
        obj['Collision'] = 'none'  # walk-through (the importer would infer it for vegetation; this says so)
    obj['Nanite'] = 0
    if lods:
        obj['LODs'] = lods
    if lod_screens:
        obj['LODScreens'] = lod_screens
    return obj


# --- Previews ---

def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def report(models):
    for obj in models:
        mats = ', '.join(m.name for m in obj.data.materials)
        hulls = sum(1 for c in obj.children if c.name.startswith('UCX_'))
        print(f'LOOTER: {obj.name}: {triangles(obj)} triangles, materials {mats}, {hulls} hulls', flush=True)


def planted(obj):
    """A copy of obj cut off at its origin's height, for previews: lt.preview puts its ground at the lowest point,
    and trunks and stems reach below the ground (to sit on slopes)."""
    copy = obj.copy()
    copy.data = obj.data.copy()
    copy.name = obj.name + '_Planted'
    bpy.context.scene.collection.objects.link(copy)
    bm = bmesh.new()
    bm.from_mesh(copy.data)
    geom = list(bm.verts) + list(bm.edges) + list(bm.faces)
    bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-5, plane_co=(0.0, 0.0, 0.0), plane_no=(0.0, 0.0, 1.0),
                           clear_inner=True)
    bm.to_mesh(copy.data)
    bm.free()
    return copy


def remove(obj):
    mesh = obj.data
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)


def preview_all(models, category='Vegetation', only=None, view=(-0.8, -1.6, 0.3), fit=None):
    """Renders each model on its own when --preview was given; names after --preview pick models."""
    if not lt.want_preview():
        return
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    wanted = [a for a in argv if not a.startswith('--')] or only
    for obj in models:
        if wanted and obj.name not in wanted:
            continue
        # The camera frames the bounding sphere: tall models need more of it in view than round ones.
        tall = obj.dimensions.z / max(obj.dimensions.x, obj.dimensions.y, 1e-6)
        shown = planted(obj)
        lt.preview([shown], lt.preview_path(category, obj.name), view=view,
                   fit=fit or lerp(0.72, 0.98, smoothstep(1.0, 2.0, tall)))
        remove(shown)


def lineup(models, out_png, gap=0.5, view=(0.0, -1.0, 0.1), fit=0.5):
    """Renders models side by side along X, in order, for scale (the models themselves stay where they are)."""
    x = 0.0
    shown = []
    for obj in models:
        copy = planted(obj)
        lo = min(c[0] for c in obj.bound_box)
        hi = max(c[0] for c in obj.bound_box)
        copy.location.x = x - lo
        shown.append(copy)
        x += (hi - lo) + gap
    bpy.context.view_layer.update()
    lt.preview(shown, out_png, view=view, fit=fit)
    for copy in shown:
        remove(copy)
