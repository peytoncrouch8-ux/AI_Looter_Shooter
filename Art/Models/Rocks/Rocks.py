"""Boulders, rocks and a pebble scatter for the tutorial island. A scripted model (see Art/README.md).

  Boulder_A        2.6 m granite block (RockGranite), broad worn faces
  Boulder_B        1.8 m rounded granite boulder
  Boulder_C        3.0 m granite boulder split by a deep crack
  Rock_A, Rock_B   1.0 m and 0.7 m granite rocks, like small boulders
  Rock_C, Rock_D   0.9 m slab and 0.35 m chunk of the cliffs' layered rock (RockCliff), for the foot of the plateau
  PebbleCluster_A  fifteen pebbles in a 1.2 x 0.6 m patch, for road edges (no collision, no Nanite)

Rocks are weathered blocks, not blobs: a rounded box cut by a few worn planes (the joints granite splits along), lumpy
at a large scale, with a flat bottom that sinks a few centimetres below the pivot so it sits in uneven ground. The
pivot is on the ground at the middle of the footprint. The fine surface (grains, lichen) comes from the texture and
the baked ambient occlusion.

Nanite sources are denser than the Medium budget; each model's Fallback share brings it to the budget (rocks
300-1000 triangles). Collision: one convex hull each, a little inside the surface where it curves in.
"""
import math
import random

import bmesh
import bpy
from mathutils import Euler, Vector, noise

import looter_model as lm
import looter_textures as lt

GRANITE = lt.material('RockGranite', MossAmount=0.6)   # moss and lichen on up-facing rock (M_World)
CLIFF = lt.material('RockCliff', MossAmount=0.6)


def soft_max(e, k):
    """max(e, 0), with a rounded corner of size k."""
    return 0.5 * (e + math.sqrt(e * e + k * k))


class Shape:
    """A rock's surface as a function of direction: a rounded box, lumps, worn planar cuts, an optional crack, and a
    flat bottom."""

    def __init__(self, size, seed, roundness=2.8, cuts=4, soft=0.2, lumps=0.07, cut_depth=(0.62, 0.85),
                 crack=None, top_cut=None, bottom=0.14, tilt=12.0):
        rnd = random.Random(seed)
        self.half = Vector(size) * 0.5
        self.mean = sum(size) / 3.0
        self.p = roundness
        self.lumps = lumps
        self.offset = Vector((rnd.uniform(0, 50), rnd.uniform(0, 50), rnd.uniform(0, 50)))
        # Joint planes, spread around the rock (golden-angle azimuths) so the facets don't bunch up on one side.
        self.planes = []
        start = rnd.uniform(0.0, 2.0 * math.pi)
        for i in range(cuts):
            azimuth = start + i * 2.39996 + rnd.uniform(-0.3, 0.3)
            up = rnd.uniform(-0.3, 0.75)
            n = Vector((math.cos(azimuth), math.sin(azimuth), up)).normalized()
            self.planes.append((n, rnd.uniform(*cut_depth), soft * self.mean))
        # Rocks don't sit square: a small random lean before the bottom is flattened (and no turn when a crack must
        # face the front).
        self.lean = Euler((math.radians(rnd.uniform(-tilt, tilt)), math.radians(rnd.uniform(-tilt, tilt)),
                           0.0 if crack else rnd.uniform(0.0, 2.0 * math.pi))).to_matrix()
        if top_cut is not None:   # a flat-ish top (bedding plane of layered rock)
            n = Vector((rnd.uniform(-0.15, 0.15), rnd.uniform(-0.15, 0.15), 1.0)).normalized()
            self.planes.append((n, top_cut, soft * self.mean * 0.5))
        self.crack = None
        if crack is not None:     # (depth as a fraction of the radius, half width in m)
            # Across the long side, so it shows from the front.
            angle = rnd.uniform(-0.35, 0.35)
            self.crack = (Vector((math.cos(angle), math.sin(angle), rnd.uniform(-0.2, 0.2))).normalized(),
                          rnd.uniform(-0.12, 0.12) * self.half.x, crack[0], crack[1])
        self.floor = -self.half.z * (1.0 - 2.0 * bottom)

    def extent(self, n):
        """How far the box reaches along n."""
        h = self.half
        return math.sqrt((n.x * h.x) ** 2 + (n.y * h.y) ** 2 + (n.z * h.z) ** 2)

    def point(self, d):
        h, p = self.half, self.p
        r = (abs(d.x) ** p + abs(d.y) ** p + abs(d.z) ** p) ** (-1.0 / p)
        pos = Vector((d.x * r * h.x, d.y * r * h.y, d.z * r * h.z))
        pos += d * (self.lumps * self.mean * noise.fractal(d * 1.1 + self.offset, 0.5, 2.0, 3))
        pos += d * (0.012 * self.mean * noise.noise(d * 5.0 + self.offset))
        for n, depth, k in self.planes:
            e = pos.dot(n) - depth * self.extent(n)
            pos -= n * soft_max(e, k)
        if self.crack is not None:
            n, shift, depth, width = self.crack
            dist = pos.dot(n) - shift
            if abs(dist) < width:
                inward = Vector((-pos.x, -pos.y, 0.0))
                t = dist / width
                pos += inward * (depth * (1.0 - t * t) ** 2)
        pos = self.lean @ pos
        pos.z = self.floor + soft_max(pos.z - self.floor, 0.04 * self.mean)
        return pos


def build(name, shape, material, set_name, source, fallback, sink=0.05, subdivisions=5, uv_seed=0, hull=True):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdivisions, radius=1.0)
    for v in bm.verts:
        v.co = shape.point(v.co.normalized())
    # The pivot: the middle of the footprint, the flat bottom a little below the ground.
    lo = Vector((min(v.co.x for v in bm.verts), min(v.co.y for v in bm.verts), min(v.co.z for v in bm.verts)))
    hi = Vector((max(v.co.x for v in bm.verts), max(v.co.y for v in bm.verts), max(v.co.z for v in bm.verts)))
    shift = Vector((-(lo.x + hi.x) * 0.5, -(lo.y + hi.y) * 0.5, -lo.z - sink))
    for v in bm.verts:
        v.co += shift
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    reduce(obj, source)
    lm.smooth(obj, 60.0)
    lt.assign(obj, material)
    lt.box_uv(obj, set_name, seed=uv_seed)
    lt.bake_vertex_ao(obj, distance=min(1.0, shape.mean * 0.8))
    if fallback is not None:
        obj['Fallback'] = fallback
    if hull:
        make_hull(obj, shape, shift)
    return obj


def reduce(obj, target):
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    if tris <= target:
        return
    mod = obj.modifiers.new('Reduce', 'DECIMATE')
    mod.ratio = target / tris
    mod.use_collapse_triangulate = True
    with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj], selected_editable_objects=[obj]):
        bpy.ops.object.modifier_apply(modifier=mod.name)


def make_hull(obj, shape, shift):
    """One convex hull from the shape sampled coarsely (a subdivided icosahedron's directions)."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
    for v in bm.verts:
        v.co = shape.point(v.co.normalized()) + shift
    result = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    mesh = bpy.data.meshes.new('UCX_' + obj.name)
    bm.to_mesh(mesh)
    bm.free()
    hull = bpy.data.objects.new('UCX_' + obj.name, mesh)
    bpy.context.scene.collection.objects.link(hull)
    hull.parent = obj
    hull.display_type = 'WIRE'


def pebbles(name, seed, count=15, area=(1.2, 0.6), tris_each=24):
    """A patch of pebbles as one mesh: a few larger ones and many small, half sunk into the ground."""
    rnd = random.Random(seed)
    placed = []
    parts = []
    tries = 0
    while len(placed) < count and tries < 2000:
        tries += 1
        size = rnd.uniform(0.1, 0.16) if len(placed) < 3 else rnd.uniform(0.04, 0.09)
        # Denser toward the middle of the patch (an ellipse).
        a, r = rnd.uniform(0.0, 2.0 * math.pi), math.sqrt(rnd.random()) ** 1.3
        x, y = math.cos(a) * r * area[0] * 0.5, math.sin(a) * r * area[1] * 0.5
        if any((Vector((x, y)) - Vector((px, py))).length < (size + ps) * 0.55 for px, py, ps in placed):
            continue
        placed.append((x, y, size))
        shape = Shape((size, size * rnd.uniform(0.7, 0.95), size * rnd.uniform(0.45, 0.7)), rnd.randint(0, 10 ** 6),
                      roundness=rnd.uniform(2.0, 2.4), cuts=2, soft=0.3, lumps=0.1, bottom=0.05)
        bm = bmesh.new()
        bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
        yaw = rnd.uniform(0.0, 2.0 * math.pi)
        c, s = math.cos(yaw), math.sin(yaw)
        for v in bm.verts:
            p = shape.point(v.co.normalized())
            v.co = Vector((x + p.x * c - p.y * s, y + p.x * s + p.y * c, p.z + shape.half.z * 0.2))
        mesh = bpy.data.meshes.new(f'_Pebble{len(placed)}')
        bm.to_mesh(mesh)
        bm.free()
        part = bpy.data.objects.new(mesh.name, mesh)
        bpy.context.scene.collection.objects.link(part)
        reduce(part, tris_each)
        parts.append(part)
    with bpy.context.temp_override(active_object=parts[0], selected_editable_objects=parts, object=parts[0],
                                   selected_objects=parts):
        bpy.ops.object.join()
    obj = parts[0]
    obj.name = name
    obj.data.name = name
    lm.smooth(obj, 70.0)
    lt.assign(obj, GRANITE)
    lt.box_uv(obj, 'RockGranite', seed=seed)
    lt.bake_vertex_ao(obj, distance=0.2)
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    return obj


models = [
    build('Boulder_A', Shape((3.0, 2.4, 1.65), 101, roundness=2.8, cuts=6, soft=0.08, lumps=0.035, cut_depth=(0.6, 0.8)),
          GRANITE, 'RockGranite', source=3200, fallback=28, sink=0.1, uv_seed=1),
    build('Boulder_B', Shape((2.1, 1.8, 1.35), 202, roundness=2.4, cuts=7, soft=0.1, lumps=0.045, bottom=0.12),
          GRANITE, 'RockGranite', source=3000, fallback=28, sink=0.08, uv_seed=2),
    build('Boulder_C', Shape((3.3, 2.6, 1.9), 303, roundness=2.7, cuts=5, soft=0.1, lumps=0.035, crack=(0.25, 0.42)),
          GRANITE, 'RockGranite', source=4000, fallback=24, sink=0.12, uv_seed=3),
    build('Rock_A', Shape((1.1, 0.9, 0.6), 404, roundness=2.7, cuts=5, soft=0.09, lumps=0.04), GRANITE,
          'RockGranite', source=1500, fallback=30, uv_seed=4),
    build('Rock_B', Shape((0.8, 0.7, 0.45), 505, roundness=2.4, cuts=4, soft=0.13, lumps=0.045), GRANITE,
          'RockGranite', source=1200, fallback=30, uv_seed=5),
    build('Rock_C', Shape((1.0, 0.7, 0.42), 606, roundness=2.4, cuts=6, soft=0.04, lumps=0.03, cut_depth=(0.55, 0.8),
                          top_cut=0.6, tilt=6.0), CLIFF, 'RockCliff', source=1200, fallback=30, uv_seed=6),
    build('Rock_D', Shape((0.4, 0.34, 0.26), 707, roundness=2.4, cuts=5, soft=0.05, lumps=0.03, cut_depth=(0.55, 0.8),
                          top_cut=0.65, tilt=8.0), CLIFF, 'RockCliff', source=600, fallback=50, sink=0.03, uv_seed=7),
    pebbles('PebbleCluster_A', 808),
]



def preview_as_placed(model, path, **options):
    """lt.preview stands its ground under the lowest point; this shows the rock as placed, cut at its pivot's ground
    (on a temporary copy)."""
    copy = model.copy()
    copy.data = model.data.copy()
    bpy.context.scene.collection.objects.link(copy)
    bm = bmesh.new()
    bm.from_mesh(copy.data)
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=(0.0, 0.0, 0.0),
                           plane_no=(0.0, 0.0, 1.0), clear_inner=True)
    bm.to_mesh(copy.data)
    bm.free()
    lt.preview([copy], path, **options)
    mesh = copy.data
    bpy.data.objects.remove(copy)
    bpy.data.meshes.remove(mesh)


if lt.want_preview():
    for model in models:
        preview_as_placed(model, lt.preview_path('RocksProps', model.name))
