"""Rock at the feet of Ransom's Rest's sandstone cliffs (the "make it alive" polish, concept B): fans of broken stone
banked against a wall's foot, and slabs broken off a band, in the cliff kit's own rock (RockCliff, the same instance as
Cliffs.py's faces, so the area's swap to MI_RockCliff_Ransom takes them too). A scripted model (Art/README.md). The
plants, drapes and nests are Art/Models/Vegetation/CliffLife.py.

What the talus pieces already do: Rocks.py's Rock_C (a 0.9 m slab) and Rock_D (a 0.35 m chunk) are single loose rocks,
banked in rows along the Sink's panels by build_area_talus.py, and every cliff face bakes two or three fallen blocks into
its own foot. None of them is a spread of rubble: the line where a face meets the grass stays hard. These add that.

  piece          size                       use
  ScreeFan_A     5.5 m along the wall, 2.6 m out, 0.75 m high at the wall
                                            a broad apron of mixed rubble, five big blocks rolled out toward its toe
  ScreeFan_B     3.4 m, 2.0 m out, 0.6 m    a cone under a gully or a notch: finer scree, two blocks
  ScreeFan_C     6.5 m, 1.8 m out, 0.4 m    a long, low apron for a straight run of wall, its blocks anywhere
  FallenSlab_A   2.9 x 2.1 m, 0.7 m high    a slab of two beds: the lower 0.5 m with a parting groove round its side
                                            and one long edge freshly broken, the upper 0.22 m broken away over half
                                            of it (a step); tipped 10 degrees, its low side 15 cm in the ground
  FallenSlab_B   2.2 x 1.4 m, 0.8 m high    a thick slab broken in two, the bigger piece propped up 18 degrees on a
                                            chunk at its far end

Frame and pivot (Art/README.md: Blender's -Y becomes the actor's +X). A fan's pivot is on the ground where the wall's foot
meets it, in the middle of the fan's width; it spreads out along -Y here (+X in Unreal: out from the face) and its back
edge rises and runs on 0.6 m into +Y (into the wall), sinking, so it tucks under the face's foot whatever that foot
does. Its other edges sink 10-15 cm below the pivot's ground. Turn it so +X is the face's outward normal and tilt it to
the ground's slope. A slab's pivot is on the ground under its middle; it stands alone (turn it any way).

Materials, both existing instances: RockCliff (MossAmount 0.6, the faces' own) for every rock; a fan's bed of grit and
dust is DenFloor (Den Rock's floor, GroundDirt tinted c9bfb2), so a fan has two slots: the rock's texture on a near-level
bed read as wood grain at any scale (its bands run one way), and the dust blends the fan's rim into the ground (the
seam it is there to hide). UVs: chips and blocks at the outcrops' 320 px/m, each in its own frame before it is laid down;
a slab at the faces' 180 px/m in its own frame before it is tipped, so its bands lie along its bedding. Occlusion baked
into the vertex alpha (the chips' contact shadows). Nanite on, every triangle kept in the fallback (Fallback 100: the
chips are already as few triangles as they can be: 8-12 points each, only the big blocks bevelled) for the fans,
Fallback 60 for the slabs. Collision: convex hulls over each fan's bed (two rows of three; the chips sit in them) and one
per block of 0.45 m or more; one per slab piece and its prop. Triangles: ScreeFan_A ~2.45k, _B ~1.65k, _C ~2.25k,
FallenSlab_A 1.5k, _B 1.3k (sources; Medium draws the fans whole, the slabs at 60%).

    Tools\\artrun.ps1 -Script Art\\Models\\Rocks\\CliffScree.py [-Preview]
-Preview renders each model to Saved/ArtPreviews/RansomsRest/CliffLife/<name>.png; the look sheet (43) is
Tools/Blender/cliff_life_sheet.py.
"""
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector, noise

import looter_model as lm
import looter_textures as lt

ROCK = lt.material('RockCliff', MossAmount=0.6)      # Cliffs.py's ROCK: the same name and properties
# A fan's bed of grit and dust: Den Rock's pale floor (Outcrops.py's den_floor_material(): the same name and properties).
# The rock's own texture on a near-level bed read as wood grain at any scale (its bands run one way).
DUST = lt.material('GroundDirt', name='DenFloor', tint=0xc9bfb2, MossAmount=0.0, DiffuseAO=0.85)
CHIP_DENSITY = 320.0
SLAB_DENSITY = 180.0
OUT = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'CliffLife')
BACK = 0.6          # how far a fan runs on into the wall behind its pivot


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def new_object(name, bm):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def join(parts, name):
    with bpy.context.temp_override(active_object=parts[0], object=parts[0], selected_objects=parts,
                                   selected_editable_objects=parts):
        bpy.ops.object.join()
    obj = parts[0]
    obj.name = name
    obj.data.name = name
    return obj


def hull(obj, points, name=None):
    """A convex collision hull (UCX_) of obj round points."""
    bm = bmesh.new()
    for p in points:
        bm.verts.new(p)
    result = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    part = new_object('UCX_' + (name or obj.name), bm)
    part.parent = obj
    part.display_type = 'WIRE'
    return part


def clean(obj):
    """Welds duplicates and drops zero-area faces (they have no tangents in Unreal)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-4)
    bmesh.ops.dissolve_degenerate(bm, dist=1e-4, edges=bm.edges[:])
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 3])
    flat = [f for f in bm.faces if f.calc_area() < 1e-7]
    if flat:
        bmesh.ops.delete(bm, geom=flat, context='FACES_ONLY')
    bm.to_mesh(obj.data)
    bm.free()


# --- Chips and blocks ---

def chip(size, seed, flat=0.45, bevel=0.0, points=12, bedding=3):
    """A broken piece of sandstone: a tabular chunk (its height flat of its length), the hull of points on a rough
    ellipsoid with its near-flat facets merged, bedding points on its top and bottom planes (none on the smallest:
    their triangles go further as more chips); bevel (a share of its size) wears its edges round (larger pieces).
    Returns a bmesh centred on the origin and the hull's points."""
    rnd = random.Random(seed)
    sx, sy, sz = size, size * rnd.uniform(0.6, 0.9), size * flat * rnd.uniform(0.75, 1.15)
    bm = bmesh.new()
    for _ in range(points):
        d = Vector((rnd.gauss(0.0, 1.0), rnd.gauss(0.0, 1.0), rnd.gauss(0.0, 1.0) * 0.6)).normalized()
        bm.verts.new(Vector((d.x * sx, d.y * sy, d.z * sz)) * (0.5 * rnd.uniform(0.8, 1.0)))
    # Bedding faces: a flat top and bottom, as sandstone splits.
    for z in (0.5, -0.5):
        for k in range(bedding):
            a = rnd.uniform(0.0, 2.0 * math.pi)
            bm.verts.new(Vector((math.cos(a) * sx * 0.3, math.sin(a) * sy * 0.3, z * sz * rnd.uniform(0.92, 1.0))))
    result = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    bmesh.ops.dissolve_limit(bm, angle_limit=math.radians(8.0), verts=bm.verts[:], edges=bm.edges[:])
    if bevel > 0.0:
        bmesh.ops.bevel(bm, geom=bm.edges[:], offset=size * bevel, segments=1, affect='EDGES', clamp_overlap=True)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    offset = Vector((seed * 0.37 % 50.0, seed * 0.11 % 50.0, 0.0))
    for v in bm.verts:
        v.co += v.co.normalized() * size * 0.03 * noise.noise(v.co * (4.0 / size) + offset)
    return bm, [v.co.copy() for v in bm.verts]


def place_chip(bm, matrix, name, density, seed):
    """The chip as an object, its UVs in its own frame (its bands along its bedding), moved into place by matrix."""
    obj = new_object(name, bm)
    lt.assign(obj, ROCK)
    lt.box_uv(obj, 'RockCliff', texel_density=density, seed=seed)
    obj.data.transform(matrix)
    return obj


# --- Fans of scree ---

class Fan:
    """A fan's bed: a half-cone of grit and rubble against the wall (y = 0), width wide along it, reaching out out (-Y)
    and height high at the wall's foot, falling off in a scree profile (steep near the wall, flattening to its toe),
    lobed round its edge; behind the wall it rises a little and then dives into the rock."""

    def __init__(self, width, out, height, seed, lobes=0.12, profile=1.6, cone=0.7):
        self.width, self.out, self.height, self.seed = width, out, height, seed
        self.lobes, self.profile, self.cone = lobes, profile, cone

    def s(self, x, y):
        """0 at the wall's middle, 1 at the fan's (lobed) edge."""
        a = x / (self.width * 0.5)
        b = min(y, 0.0) / self.out
        r = math.sqrt(a * a + b * b)
        angle = math.atan2(-b, a)
        wobble = 1.0 + self.lobes * noise.noise(Vector((math.cos(angle) * 1.7, math.sin(angle) * 1.7, self.seed * 0.31)))
        return r / wobble

    def z(self, x, y):
        s = self.s(x, y)
        if s < 1.0:
            # Along the wall the height falls off less than outward (cone: how much), so a broad fan keeps its height.
            a = abs(x) / (self.width * 0.5)
            along = 1.0 - (1.0 - self.cone) * smoothstep(0.0, 1.0, a)
            h = self.height * along * (1.0 - s) ** self.profile
            h -= 0.14 * smoothstep(0.82, 1.0, s)
        else:
            h = -0.14 - (s - 1.0) * 0.8
        h += 0.05 * noise.noise(Vector((x * 1.6, y * 1.6, self.seed * 0.7))) * (1.0 - 0.5 * smoothstep(0.6, 1.0, s))
        h += 0.02 * noise.noise(Vector((x * 4.5, y * 4.5, self.seed * 0.7 + 9.0)))
        if y > 0.0:
            # Behind the wall's line: on up a little, then down into the rock.
            h += 0.1 * smoothstep(0.0, 0.25, y) - 1.4 * max(y - 0.25, 0.0)
        return h

    def normal(self, x, y):
        e = 0.05
        dx = (self.z(x + e, y) - self.z(x - e, y)) / (2.0 * e)
        dy = (self.z(x, y + e) - self.z(x, y - e)) / (2.0 * e)
        return Vector((-dx, -dy, 1.0)).normalized()


def fan_bed(fan, name, cell=0.3):
    """The bed of grit under the chips: a grid over the fan, its cells outside the edge (all under the ground) left
    out."""
    half = fan.width * 0.5 + 0.3
    nx = int(math.ceil(2.0 * half / cell)) + 1
    y0, y1 = -fan.out - 0.3, BACK
    ny = int(math.ceil((y1 - y0) / cell)) + 1
    bm = bmesh.new()
    grid = []
    for j in range(ny):
        y = y0 + (y1 - y0) * j / (ny - 1)
        row = []
        for i in range(nx):
            x = -half + 2.0 * half * i / (nx - 1)
            row.append(bm.verts.new((x, y, fan.z(x, y))))
        grid.append(row)
    for j in range(ny - 1):
        for i in range(nx - 1):
            quad = (grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i])
            if max(v.co.z for v in quad) < -0.06:
                continue
            # Split along the shorter diagonal in height, so ridges stay ridges.
            a, b, c, d = quad
            if abs(a.co.z - c.co.z) < abs(b.co.z - d.co.z):
                bm.faces.new((a, b, c))
                bm.faces.new((a, c, d))
            else:
                bm.faces.new((a, b, d))
                bm.faces.new((b, c, d))
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context='VERTS')
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    obj = new_object(name, bm)
    lt.assign(obj, DUST)
    lt.box_uv(obj, 'GroundDirt', seed=fan.seed)
    return obj


def scree_fan(name, seed, width, out, height, counts, sizes, lobes=0.12, profile=1.6, cone=0.7, big_to_toe=True):
    """A fan of scree: its bed (Fan) under chips of three sizes (counts: small, medium, large; sizes: the (low, high)
    of each, metres), spread over it without overlapping much, the large ones rolled toward its toe (big_to_toe), the
    small everywhere, each sunk a third into the bed and lying with its slope."""
    rng = random.Random(seed)
    fan = Fan(width, out, height, seed, lobes, profile, cone)
    parts = [fan_bed(fan, name)]
    placed = []
    block_hulls = []
    index = 0
    # The big blocks first, then the medium, then the chips in the gaps (the other way round, the chips left the
    # blocks no room).
    for kind in (2, 1, 0):
        count, (low, high) = counts[kind], sizes[kind]
        made = 0
        tries = 0
        while made < count and tries < count * 80:
            tries += 1
            size = rng.uniform(low, high)
            # Where along the fan's reach (s) it lies: big pieces rolled out toward the toe, small ones anywhere.
            if kind == 2 and big_to_toe:
                s = rng.uniform(0.45, 0.85)
            elif kind == 1:
                s = rng.uniform(0.15, 0.85)
            else:
                s = rng.random() ** 0.75 * 0.92      # a little thicker toward the wall, where they fell
            angle = rng.uniform(math.radians(8.0), math.radians(172.0))
            x = math.cos(angle) * s * width * 0.5
            y = -math.sin(angle) * s * out
            if fan.s(x, y) > 0.9:
                continue
            if any((Vector((x, y)) - Vector((px, py))).length < (size + ps) * 0.36 for px, py, ps in placed):
                continue
            placed.append((x, y, size))
            flat = rng.uniform(0.38, 0.55)
            # Only the big blocks are worn round; the chips' facets are their fresh breaks.
            bevel = 0.06 if kind == 2 else 0.0
            bm, points = chip(size, seed * 101 + index, flat, bevel, points=(8, 10, 12)[kind], bedding=(0, 2, 3)[kind])
            n = fan.normal(x, y)
            tilt = Vector((0.0, 0.0, 1.0)).rotation_difference(n).to_matrix().to_4x4()
            wobble = Euler((math.radians(rng.uniform(-10.0, 10.0)), math.radians(rng.uniform(-10.0, 10.0)),
                            rng.uniform(0.0, 2.0 * math.pi))).to_matrix().to_4x4()
            ground = fan.z(x, y)
            sz = size * flat
            matrix = Matrix.Translation((x, y, ground + sz * rng.uniform(0.12, 0.25))) @ tilt @ wobble
            parts.append(place_chip(bm, matrix, f'_chip{index}', CHIP_DENSITY, seed * 7 + index))
            if size >= 0.45:
                block_hulls.append([matrix @ p for p in points])
            index += 1
            made += 1
    obj = join(parts, name)
    clean(obj)
    lm.smooth(obj, 48.0)
    lt.bake_vertex_ao(obj, samples=32, distance=0.5)
    obj['Fallback'] = 100
    # Hulls: the bed in two rows of three (each a convex piece over its patch's surface down to below the ground),
    # and every big block its own.
    rows = ((-fan.out - 0.2, -fan.out * 0.45), (-fan.out * 0.45, BACK * 0.4))
    columns = 3
    for y0, y1 in rows:
        for c in range(columns):
            x0 = -width * 0.5 + width * c / columns
            x1 = -width * 0.5 + width * (c + 1) / columns
            points = []
            for i in range(7):
                for j in range(5):
                    x = lerp(x0, x1, i / 6)
                    y = lerp(y0, y1, j / 4)
                    z = fan.z(x, y)
                    if z > -0.1:
                        points.append(Vector((x, y, z)))
                        points.append(Vector((x, y, -0.15)))
            if len(points) >= 8 and max(p.z for p in points) > 0.03:
                hull(obj, points)
    for points in block_hulls:
        hull(obj, points)
    return obj


def scree_fan_a():
    """A broad apron of mixed rubble, 5.5 m along the wall, 2.6 m out, 0.75 m high at the wall; four big blocks rolled
    out toward its toe."""
    return scree_fan('ScreeFan_A', 601, 5.5, 2.6, 0.75, (80, 22, 5), ((0.1, 0.24), (0.26, 0.45), (0.55, 0.9)))


def scree_fan_b():
    """A cone of fine scree under a gully or a notch: 3.4 m, 2.0 m out, 0.6 m high, peaked at its middle; one block."""
    return scree_fan('ScreeFan_B', 613, 3.4, 2.0, 0.6, (75, 14, 2), ((0.08, 0.2), (0.22, 0.38), (0.45, 0.65)),
                     lobes=0.1, profile=1.4, cone=0.25)


def scree_fan_c():
    """A long, low apron for a straight run of wall: 6.5 m, 1.8 m out, 0.4 m high, a few big pieces scattered."""
    return scree_fan('ScreeFan_C', 627, 6.5, 1.8, 0.4, (85, 18, 4), ((0.09, 0.22), (0.24, 0.42), (0.5, 0.75)),
                     lobes=0.16, profile=1.8, cone=0.85, big_to_toe=False)


# --- Fallen slabs ---

def outline(length, width, seed, count=44, broken=None, chamfer=0.18):
    """A slab's plan: a rounded rectangle (a superellipse) with worn, wandering sides and chamfered corners; broken:
    (angle of the edge's middle, half its span in radians) for a freshly broken edge, jagged and stepped."""
    rnd = random.Random(seed)
    a, b = length * 0.5, width * 0.5
    points = []
    for k in range(count):
        t = 2.0 * math.pi * k / count
        c, s = math.cos(t), math.sin(t)
        r = (abs(c / a) ** 4 + abs(s / b) ** 4) ** -0.25
        r *= 1.0 - chamfer * max(0.0, abs(math.sin(2.0 * t))) ** 6
        r *= 1.0 + 0.05 * noise.noise(Vector((c * 1.5, s * 1.5, seed * 0.13)))
        fresh = 0.0
        if broken is not None:
            d = (t - broken[0] + math.pi) % (2.0 * math.pi) - math.pi
            if abs(d) < broken[1]:
                fresh = 1.0
                r *= 1.0 + 0.09 * math.sin(d * 9.0 + seed) + 0.06 * (1.0 if math.sin(d * 3.0 + seed) > 0.0 else -1.0)
        points.append((Vector((c * r, s * r)), fresh))
    return points


def slab_piece(name, length, width, thick, seed, parting=0.55, broken=None, cut=None, lift=0.0):
    """One slab: its outline extruded through its bed (the bottom buried), a soft parting groove round its side at
    parting of its height, its top edge worn round (barely, along a fresh break), its top weathered uneven. cut: (n, c)
    keeps only the side n . p <= c of the plan (a slab broken in two). Built in its own frame, origin at its bottom's
    middle."""
    rnd = random.Random(seed)
    plan = outline(length, width, seed, broken=broken)
    if cut is not None:
        n, c = cut
        kept = []
        for i, (p, fresh) in enumerate(plan):
            q, qf = plan[(i + 1) % len(plan)]
            inside_p, inside_q = n.dot(p) <= c, n.dot(q) <= c
            if inside_p:
                kept.append((p, fresh, False))
            if inside_p != inside_q:
                t = (c - n.dot(p)) / (n.dot(q) - n.dot(p))
                kept.append((p.lerp(q, t), 1.0, True))
        # The break between the two crossings (where the outline leaves the cut and comes back): points along it,
        # wandering a few centimetres either side, a fresh face.
        count = len(kept)
        for i in range(count):
            a, b = kept[i], kept[(i + 1) % count]
            if a[2] and b[2]:
                along = [a[0].lerp(b[0], k / 7) for k in range(1, 7)]
                side = Vector((n.x, n.y))
                jag = [(p + side * (0.035 * noise.noise(Vector((p.x * 3.0, p.y * 3.0, seed * 0.7)))
                                    + 0.02 * (1.0 if k % 2 else -1.0)), 1.0, False) for k, p in enumerate(along)]
                kept = kept[:i + 1] + jag + kept[i + 1:]
                break
        plan = [(p, fresh) for p, fresh, _ in kept]
    middle = sum((p for p, _ in plan), Vector((0.0, 0.0))) / len(plan)
    # The side's profile, bottom to top: (height share, inset m, fresh inset m).
    profile = [(0.0, 0.0, 0.0), (0.2, 0.0, 0.0), (parting - 0.06, 0.005, 0.0), (parting, 0.04, 0.012),
               (parting + 0.06, 0.005, 0.0), (0.82, 0.0, 0.0), (0.94, 0.025, 0.006), (1.0, 0.075, 0.018)]
    bm = bmesh.new()
    rings = []
    for share, inset, fresh_inset in profile:
        ring = []
        for p, fresh in plan:
            out = p - middle
            out = out.normalized() if out.length > 1e-6 else Vector((1.0, 0.0))
            d = lerp(inset, fresh_inset, fresh)
            q = p - out * d
            z = share * thick
            ring.append(bm.verts.new((q.x, q.y, z)))
        rings.append(ring)
    count = len(plan)
    for lower, upper in zip(rings, rings[1:]):
        for k in range(count):
            k1 = (k + 1) % count
            bm.faces.new((lower[k], lower[k1], upper[k1], upper[k]))
    # The top: two inner rings toward the middle, so it can be weathered uneven.
    top = rings[-1]
    inner = []
    for share in (0.66, 0.33):
        ring = []
        for v in top:
            p = middle.to_3d() + (v.co - middle.to_3d()) * share
            p.z = thick
            ring.append(bm.verts.new(p))
        inner.append(ring)
    centre = bm.verts.new((middle.x, middle.y, thick))
    for outer, ring in zip([top] + inner[:-1], inner):
        for k in range(count):
            k1 = (k + 1) % count
            bm.faces.new((outer[k], outer[k1], ring[k1], ring[k]))
    for k in range(count):
        bm.faces.new((inner[-1][k], inner[-1][(k + 1) % count], centre))
    bottom = rings[0]
    bm.faces.new(list(reversed(bottom)))
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 3])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    # Weathering: the top dished and lumpy, the sides wavy.
    offset = Vector((seed * 0.29 % 40.0, seed * 0.17 % 40.0, 3.0))
    for v in bm.verts:
        p = v.co
        if p.z > thick * 0.97:
            p.z += 0.03 * noise.noise(p * 1.3 + offset) + 0.012 * noise.noise(p * 4.0 + offset)
        else:
            side = Vector((p.x - middle.x, p.y - middle.y, 0.0))
            if side.length > 1e-6:
                p += side.normalized() * 0.018 * noise.noise(p * 2.2 + offset)
    # Raised onto the bed under it before its UVs, so the bands run on from one bed to the next.
    for v in bm.verts:
        v.co.z += lift
    obj = new_object(name, bm)
    lt.assign(obj, ROCK)
    lt.box_uv(obj, 'RockCliff', texel_density=SLAB_DENSITY, seed=seed)
    return obj, [v.co.copy() for v in obj.data.vertices]


def finish_slab(obj, hulls):
    clean(obj)
    lm.smooth(obj, 50.0)
    lt.bake_vertex_ao(obj, samples=32, distance=0.8)
    obj['Fallback'] = 60
    for points in hulls:
        hull(obj, points)
    return obj


def fallen_slab_a():
    """A slab of two beds, 2.8 x 1.8 m: the lower 0.5 m thick with a parting groove round its side, one long edge
    freshly broken (jagged, barely worn); the upper bed, 0.22 m, broken away over half of it, leaving a step. Lying
    tipped 10 degrees, its low side 15 cm in the ground."""
    lower, lower_points = slab_piece('FallenSlab_A', 2.8, 1.8, 0.5, 701, parting=0.42, broken=(math.pi * 0.5, 0.9))
    upper, upper_points = slab_piece('_FallenSlab_A_upper', 2.8, 1.8, 0.22, 701, parting=0.5,
                                     broken=(math.pi * 0.5, 0.9), cut=(Vector((1.0, -0.35)).normalized(), 0.15),
                                     lift=0.47)
    matrix = Matrix.Translation((0.0, 0.0, -0.17)) @ Euler((math.radians(10.0), math.radians(-3.0),
                                                            math.radians(12.0))).to_matrix().to_4x4()
    obj = join([lower, upper], 'FallenSlab_A')
    obj.data.transform(matrix)
    return finish_slab(obj, [[matrix @ p for p in lower_points], [matrix @ p for p in upper_points]])


def fallen_slab_b():
    """A thick slab, 1.9 x 1.3 m and 0.65 m, broken across in two: the bigger piece propped up 18 degrees on a chunk at
    its far end, the smaller lying flat a hand's width off it."""
    big, big_points = slab_piece('FallenSlab_B', 1.9, 1.3, 0.65, 709, parting=0.62,
                                 cut=(Vector((1.0, 0.25)).normalized(), 0.22))
    small, small_points = slab_piece('_FallenSlab_B_small', 1.9, 1.3, 0.65, 709, parting=0.62,
                                     cut=(-Vector((1.0, 0.25)).normalized(), -0.32))
    m_big = Matrix.Translation((-0.12, 0.0, -0.12)) @ Euler((math.radians(2.0), math.radians(18.0),
                                                              0.0)).to_matrix().to_4x4()
    m_small = Matrix.Translation((0.06, 0.05, -0.2)) @ Euler((math.radians(-3.0), math.radians(4.0),
                                                              math.radians(-6.0))).to_matrix().to_4x4()
    big.data.transform(m_big)
    small.data.transform(m_small)
    bm, chip_points = chip(0.45, 719, 0.6, 0.06, points=13)
    m_chip = Matrix.Translation((-0.95, 0.05, 0.05)) @ Euler((0.0, 0.0, 0.7)).to_matrix().to_4x4()
    prop = place_chip(bm, m_chip, '_prop', CHIP_DENSITY, 719)
    obj = join([big, small, prop], 'FallenSlab_B')
    return finish_slab(obj, [[m_big @ p for p in big_points], [m_small @ p for p in small_points],
                             [m_chip @ p for p in chip_points]])


BUILDERS = {'ScreeFan_A': scree_fan_a, 'ScreeFan_B': scree_fan_b, 'ScreeFan_C': scree_fan_c,
            'FallenSlab_A': fallen_slab_a, 'FallenSlab_B': fallen_slab_b}


def build(names=None):
    return [maker() for name, maker in BUILDERS.items() if not names or name in names]


def report(models):
    for obj in models:
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        hulls = sum(1 for c in obj.children if c.name.startswith('UCX_'))
        print(f'LOOTER: {obj.name}: {tris} triangles, materials {", ".join(m.name for m in obj.data.materials)}, '
              f'{hulls} hulls, {obj.dimensions.x:.2f} x {obj.dimensions.y:.2f} x {obj.dimensions.z:.2f} m', flush=True)


if __name__ == '__main__':
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    # Model names after '--' build only those (the exporter's own arguments are never model names).
    models = build([a for a in argv if a in BUILDERS])
    report(models)
    if lt.want_preview():
        for model in models:
            lt.preview([model], os.path.join(OUT, model.name + '.png'), view=(-0.7, -1.6, 0.9), fit=0.8,
                       ground_at='origin')
