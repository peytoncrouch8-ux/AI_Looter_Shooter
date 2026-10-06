"""Hob, the one-eyed crow (Docs/Story.md; Docs/Areas/RansomsRest.md, the NPCs table and the art needs): three concept
directions for the user to pick from (2026-10-06), in the stylized-realism style. Concept art kept in Art/Backlog (see
its README): nothing in the game reads it. The pick becomes a new small rig the game poses in code.

  A  Veteran    hunched and fluffed, a head a size too big, dusty matte plumage. His left eye is grown shut in a
                bare scarred patch with a pale gash from crown to cheek; the beak is chipped, the crown and hackles
                ragged, two primaries are gone (a gap in the open wing), a secondary and a tail feather are broken.
  B  Messenger  upright, sleek and glossy, a long neat tail and long wingtips. A brass eyecup covers the socket on a
                band of mourning crepe knotted behind the head, its two swallowtailed tails drooping; a brass message
                tube rides his right leg.
  C  Revenant   lean and brooding, a raven's heavy beak, a shaggy ruff and a long wedge tail. The socket is open, with
                a dull ember of soul-light deep in it; the good eye has gone pale; his wingtips and two tail feathers
                fade to ash and dissolve in a dither (the game's ghosts are masked and dithered, never translucent).

Every crow is about 46-50 cm from beak to tail. The left eye is the lost one; the hero shots and the comparison show
that side. The body and head are one smooth skin (metaballs polygonized: head, neck and body blend, and the head turns
on the neck); feathers are shaped geometry laid on it: contour lobes, hackles, nasal bristles, layered folded wings
(a pad of covert rows over stacked secondaries, tertials and primaries), the open wing built on its bones (shoulder,
elbow, wrist, hand: secondaries on the forearm, primaries fanning from the hand into five fingers, covert rows on top
and underneath, an airfoil leading edge), and a twelve-feather tail fan. Preview shaders only: see the game notes.

Renders (Cycles, about a minute each) in Saved/ArtPreviews/RansomsRest/Concepts/Hob/:
  Hob_<X>_Hero.png        three-quarter, perched in golden light (A a fence post, B a whitewashed headboard, C an old
                          headboard in the churchyard)
  Hob_<X>_Turnaround.png  front, left side, back; orthographic
  Hob_<X>_Wings.png       the flap pose: landing on a post, wings open, tail fanned, feet reaching
  Hob_<X>_Scale.png       on top of a 1.8 m post, from a player's eye height 3.2 m off
  Hob_Compare.png         the three side by side: the same fence, light, pose and scale

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Backlog\\Creatures\\HobConcepts.py -ScriptArgs --preview
    ... -ScriptArgs --preview,B,hero,wings          some options and shots (hero turn wings scale compare)
    ... -ScriptArgs --preview,A,head,--quick        work checks (clay, head [beak], wingtop), low quality, into work/
    ... (no --preview)                              builds each concept and logs its triangles

The game version (any option): a skeletal mesh of 2.5-3.5k triangles (LOD1 50%, LOD2 25%; Nanite off), one
near-black Polymer material for plumage, beak and legs plus at most three small slots for the eye and the option's
detail, and about 26 bones: root, body, neck, head, jaw; tail and tail_fan; per wing upper, fore, hand and primaries
(the fan that closes the primaries when the wing folds); per leg thigh, shin, foot and hallux. Code drives perch
(breathing, head snaps and tilts toward the player, tail flicks, a caw with the jaw), hop, flap and glide, and the
landing flare.
"""
import math
import os
import random
import sys
import time
import zlib

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_props as lp
import looter_textures as lt

OUT_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Concepts', 'Hob')
CM = 0.01            # creature space is centimeters; the scene is meters
THRESH = 0.6         # the metaball threshold the body is polygonized at
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
QUICK = '--quick' in ARGV
UP = Vector((0.0, 0.0, 1.0))


def log(message):
    print(f'HOB: {message}', flush=True)


def smooth(e0, e1, x):
    if e1 == e0:
        return 1.0 if x >= e1 else 0.0
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def turn(yaw=0.0, pitch=0.0, roll=0.0):
    """Degrees: yaw turns the nose left (about z), pitch tips it down (about y), roll lifts the left side (about x)."""
    return (Matrix.Rotation(math.radians(yaw), 4, 'Z') @ Matrix.Rotation(math.radians(pitch), 4, 'Y')
            @ Matrix.Rotation(math.radians(roll), 4, 'X'))


# --- Mesh building (creature space: cm, x forward, y left, z up, the origin where the feet grip) ---

class Part:
    """One mesh being built, with UVs and a material per face."""

    def __init__(self, name):
        self.name = name
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')
        self.var = self.bm.faces.layers.float.new('var')
        self.rng = random.Random(zlib.crc32(name.encode()))
        self.cur = 0.5
        self.mats = []

    def slot(self, mat):
        if mat not in self.mats:
            self.mats.append(mat)
        return self.mats.index(mat)

    def vert(self, p):
        return self.bm.verts.new(p)

    def face(self, verts, uvs, mat):
        try:
            f = self.bm.faces.new(verts)
        except ValueError:
            return None
        f.material_index = self.slot(mat)
        f.smooth = True
        f[self.var] = self.cur
        for loop, uv in zip(f.loops, uvs):
            loop[self.uv].uv = uv
        return f

    def finish(self, solidify=0.0):
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        for m in self.mats:
            mesh.materials.append(m)
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        if solidify:
            mod = obj.modifiers.new('Solidify', 'SOLIDIFY')
            mod.thickness = solidify
            mod.offset = -1.0
            mod.use_even_offset = False
        return obj


def tube(part, pts, radii, mat, sides=10, up=UP, caps=(True, True), uv_v=0.5):
    """A closed tube along pts (parallel-transported rings, so it never twists)."""
    pts = [Vector(p) for p in pts]
    n = len(pts)
    t_prev = (pts[1] - pts[0]).normalized()
    ref = Vector(up) - t_prev * Vector(up).dot(t_prev)
    if ref.length < 1e-4:
        ref = t_prev.orthogonal()
    ref.normalize()
    rings = []
    for k, p in enumerate(pts):
        t = (pts[min(k + 1, n - 1)] - pts[max(k - 1, 0)]).normalized()
        ref = t_prev.rotation_difference(t) @ ref
        ref = (ref - t * ref.dot(t)).normalized()
        side = t.cross(ref)
        rings.append([part.vert(p + (ref * math.cos(a) + side * math.sin(a)) * radii[k])
                      for a in (2.0 * math.pi * j / sides for j in range(sides))])
        t_prev = t
    for k in range(n - 1):
        for j in range(sides):
            j1 = (j + 1) % sides
            part.face((rings[k][j], rings[k][j1], rings[k + 1][j1], rings[k + 1][j]),
                      [(k / n, j / sides), (k / n, (j + 1) / sides), ((k + 1) / n, (j + 1) / sides), ((k + 1) / n, j / sides)],
                      mat)
    if caps[0]:
        hub = part.vert(pts[0] - (pts[1] - pts[0]).normalized() * radii[0] * 0.35)
        for j in range(sides):
            part.face((hub, rings[0][(j + 1) % sides], rings[0][j]), [(0, uv_v)] * 3, mat)
    if caps[1]:
        hub = part.vert(pts[-1] + (pts[-1] - pts[-2]).normalized() * radii[-1] * 0.35)
        for j in range(sides):
            part.face((hub, rings[-1][j], rings[-1][(j + 1) % sides]), [(1, uv_v)] * 3, mat)
    return rings


def lathe(part, origin, axis, ref, profile, segs, mat):
    """A surface of revolution round axis through origin; profile [(radius, height), ...] from the top down (a radius
    of 0 closes the top)."""
    axis = Vector(axis).normalized()
    ref = (Vector(ref) - axis * Vector(ref).dot(axis)).normalized()
    other = axis.cross(ref)
    origin = Vector(origin)
    rings = []
    for r, h in profile:
        if r < 1e-6:
            rings.append([part.vert(origin + axis * h)])
        else:
            rings.append([part.vert(origin + axis * h + (ref * math.cos(a) + other * math.sin(a)) * r)
                          for a in (2.0 * math.pi * j / segs for j in range(segs))])
    for i in range(len(rings) - 1):
        a, b = rings[i], rings[i + 1]
        for j in range(segs):
            j1 = (j + 1) % segs
            if len(a) == 1:
                part.face((a[0], b[j], b[j1]), [(0.5, 0.5)] * 3, mat)
            else:
                part.face((a[j], b[j], b[j1], a[j1]), [(0.5, 0.5)] * 4, mat)


def ribbon(part, pts, sides, width, mat, swallowtail=0.0, across=3):
    """A free ribbon along pts, sides[i] across it; its end cut into a swallowtail V."""
    rows = []
    n = len(pts)
    for i, (p, s) in enumerate(zip(pts, sides)):
        row = []
        for k in range(across):
            x = -1.0 + 2.0 * k / (across - 1)
            q = p + s * (x * width * 0.5)
            if i == n - 1 and swallowtail:
                q = q - (pts[-1] - pts[-2]).normalized() * swallowtail * (1.0 - abs(x))
            row.append(part.vert(q))
        rows.append(row)
    for i in range(n - 1):
        for k in range(across - 1):
            part.face((rows[i][k], rows[i + 1][k], rows[i + 1][k + 1], rows[i][k + 1]),
                      [(i / n, k / across), ((i + 1) / n, k / across), ((i + 1) / n, (k + 1) / across), (i / n, (k + 1) / across)],
                      mat)


def ellipsoid(part, center, radii, mat, rot=None, segs=16, rings=10, deform=None):
    """A smooth ellipsoid; rot (a 3x3 or 4x4 matrix) turns its axes; deform(x, y, z) bends the unit sphere."""
    rot = (rot.to_3x3() if rot is not None else Matrix.Identity(3))
    center = Vector(center)
    top = None
    verts = []
    for i in range(rings + 1):
        th = math.pi * i / rings
        row = []
        for j in range(segs if 0 < i < rings else 1):
            ph = 2.0 * math.pi * j / segs
            x, y, z = math.sin(th) * math.cos(ph), math.sin(th) * math.sin(ph), math.cos(th)
            if deform:
                x, y, z = deform(x, y, z)
            row.append(part.vert(center + rot @ Vector((x * radii[0], y * radii[1], z * radii[2]))))
        verts.append(row)
    for i in range(rings):
        for j in range(segs):
            j1 = (j + 1) % segs
            a, b = verts[i], verts[i + 1]
            uv = [(j / segs, i / rings), ((j + 1) / segs, i / rings), ((j + 1) / segs, (i + 1) / rings), (j / segs, (i + 1) / rings)]
            if i == 0:
                part.face((a[0], b[j], b[j1]), [uv[0], uv[3], uv[2]], mat)
            elif i == rings - 1:
                part.face((a[j], b[0], a[j1]), [uv[0], uv[3], uv[1]], mat)
            else:
                part.face((a[j], b[j], b[j1], a[j1]), [uv[0], uv[3], uv[2], uv[1]], mat)


# --- Feathers ---

def width_at(shape, t, tr):
    """A feather's half-width factor (0..1) at t (0 base, 1 tip); tr: where its rounded tip starts."""
    if shape == 'pointed':                       # hackles and tufts: lanceolate
        return max(0.0, math.sin(math.pi * min(0.06 + 0.94 * t, 1.0) ** 0.72)) ** 0.75
    if shape == 'primary':                       # emarginated: the outer part narrows into a finger
        f = (0.3 + 0.7 * smooth(0.0, 0.26, t)) * (1.0 - 0.34 * smooth(0.42, 0.6, t))
    elif shape == 'flight':                      # a flight feather: a narrow quill, the vane widening, a round tip
        f = 0.3 + 0.7 * smooth(0.0, 0.3, t)
        tr = min(tr, 0.8)
    elif shape == 'scale':                       # body scallops: wide and short
        f = 0.78 + 0.22 * smooth(0.0, 0.35, t)
    elif shape == 'contour':                     # a body feather's exposed tip: an oval
        f = 0.55 + 0.45 * smooth(0.0, 0.45, t)
        tr = min(tr, 0.45)
    elif shape == 'broken':                      # snapped off: no rounded tip
        return 0.62 + 0.38 * smooth(0.0, 0.3, t)
    else:                                        # 'round': coverts, secondaries, tail
        f = 0.62 + 0.38 * smooth(0.0, 0.3, t)
    if t > tr:
        k = (t - tr) / (1.0 - tr)
        f *= math.sqrt(max(0.0, 1.0 - k * k))
    return f


def feather(part, to3d, L, W, mat, shape='round', nl=9, nw=2, rachis=0.0, cup=0.0, bend=0.0, twist=0.0, asym=0.0,
            lift=None, round_k=1.0, tip_mat=None, tip_from=2.0, jag=None, curl=0.0):
    """One feather from its base (0, 0) along +a for L cm, W cm wide, through to3d(a, across, height) -> Vector.
    rachis raises the shaft, cup arches the vane (edges down), bend curves it down along its length, curl bends its
    outer half sideways (cm at the tip), twist turns the vane (radians at the tip), asym narrows the leading vane
    (across < 0), lift(t) raises it off what it lies on. Faces past tip_from (0..1 along) take tip_mat."""
    tr = min(max(1.0 - round_k * 0.5 * W / max(L, 1e-3), 0.25), 0.94)
    part.cur = part.rng.random()
    ts = [1.0 - (1.0 - j / nl) ** 1.35 for j in range(nl)]
    broken = shape == 'broken'
    if broken:
        ts.append(1.0)
    xs = [-1.0 + k / nw for k in range(2 * nw + 1)]
    rows = []
    for j, t in enumerate(ts):
        f = width_at(shape, t, tr)
        row = []
        for k, x in enumerate(xs):
            tt = t
            if broken and j == len(ts) - 1 and jag:
                tt = 1.0 - jag[k % len(jag)]
            y = x * 0.5 * W * f * ((1.0 - asym) if x < 0 else 1.0)
            h = (rachis * max(0.0, 1.0 - abs(x) * 2.2) ** 2 * (1.0 - 0.6 * tt) + cup * (1.0 - x * x) * f * 0.5 * W
                 - bend * L * tt * tt + (lift(tt) if lift else 0.0))
            y += curl * tt * tt
            if twist:
                a = twist * tt
                y, h = y * math.cos(a) - h * math.sin(a), y * math.sin(a) + h * math.cos(a)
            row.append(part.vert(to3d(tt * L, y, h)))
        rows.append(row)

    def uv(t, x):
        return (t, 0.5 * (x + 1.0))

    def mat_for(t):
        return tip_mat if (tip_mat is not None and t >= tip_from) else mat
    for j in range(len(rows) - 1):
        tm = 0.5 * (ts[j] + ts[j + 1])
        for k in range(len(xs) - 1):
            part.face((rows[j][k], rows[j + 1][k], rows[j + 1][k + 1], rows[j][k + 1]),
                      [uv(ts[j], xs[k]), uv(ts[j + 1], xs[k]), uv(ts[j + 1], xs[k + 1]), uv(ts[j], xs[k + 1])], mat_for(tm))
    if not broken:
        h_tip = -bend * L + (lift(1.0) if lift else 0.0)
        tip = part.vert(to3d(L, curl, h_tip))
        j = len(rows) - 1
        for k in range(len(xs) - 1):
            part.face((rows[j][k], tip, rows[j][k + 1]), [uv(ts[j], xs[k]), uv(1.0, 0.0), uv(ts[j], xs[k + 1])],
                      mat_for(1.0))


def free_frame(base, along, normal):
    """to3d for a feather standing free: base point, its shaft direction and the normal of its top face."""
    d = Vector(along).normalized()
    n = Vector(normal)
    n = (n - d * n.dot(d)).normalized()
    s = n.cross(d)
    base = Vector(base)

    def to3d(a, y, h):
        return base + d * a + s * y + n * h
    return to3d


# --- The body: metaballs polygonized into one smooth skin ---

def blob_mesh(name, elements, resolution):
    """elements: (center, quaternion, semi-axes, stiffness, negative). Each element's isolated surface has those
    semi-axes; overlapping ones blend. Returns a bmesh of the polygonized, lightly smoothed surface."""
    mb = bpy.data.metaballs.new(name + 'Meta')
    mb.resolution = resolution
    mb.render_resolution = resolution
    mb.threshold = THRESH
    for center, quat, semi, stiff, negative in elements:
        el = mb.elements.new(type='ELLIPSOID')
        vis = math.sqrt(1.0 - (THRESH / stiff) ** (1.0 / 3.0))
        big = max(semi)
        el.co = center
        el.radius = big / vis
        el.size_x, el.size_y, el.size_z = [max(s / big, 0.01) for s in semi]
        el.rotation = quat
        el.stiffness = stiff
        el.use_negative = negative
    obj = bpy.data.objects.new(name + 'Meta', mb)
    bpy.context.scene.collection.objects.link(obj)
    dg = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(obj.evaluated_get(dg))
    bpy.data.objects.remove(obj)
    bpy.data.metaballs.remove(mb)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bpy.data.meshes.remove(mesh)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=resolution * 0.05)
    for _ in range(3):
        bmesh.ops.smooth_vert(bm, verts=bm.verts, factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    bm.normal_update()
    return bm


class Surface:
    """Ray casts onto a mesh (creature space)."""

    def __init__(self, bm):
        self.tree = BVHTree.FromBMesh(bm)

    def cast(self, origin, direction, dist=60.0):
        loc, normal, _, _ = self.tree.ray_cast(Vector(origin), Vector(direction).normalized(), dist)
        return loc, normal

    def toward(self, inner, point):
        """Where the line from inner (inside the skin) through point leaves the skin, and the normal there."""
        d = Vector(point) - Vector(inner)
        loc, normal = self.cast(inner, d)
        if loc is None:
            return Vector(point), d.normalized()
        return loc, normal


# --- Materials (preview shaders; the game version uses the textured masters, see the report) ---

def principled(name, color, rough=0.5, metal=0.0, coat=0.0, coat_rough=0.3, sheen=0.0, sheen_tint=0xffffff,
               spec=0.5, emit=None, emit_strength=0.0):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['Metallic'].default_value = metal
    bsdf.inputs['Coat Weight'].default_value = coat
    bsdf.inputs['Coat Roughness'].default_value = coat_rough
    bsdf.inputs['Sheen Weight'].default_value = sheen
    bsdf.inputs['Sheen Tint'].default_value = lt.hex_color(sheen_tint)
    bsdf.inputs['Specular IOR Level'].default_value = spec
    if emit is not None:
        bsdf.inputs['Emission Color'].default_value = lt.hex_color(emit)
        bsdf.inputs['Emission Strength'].default_value = emit_strength
    mat.diffuse_color = lt.hex_color(color)
    return mat


def eye_material(name, iris, ring=0x0b0806, iris_out=0.3):
    """A glossy eye whose iris is painted by the ball's latitude (its pole faces out of the head)."""
    mat = principled(name, 0x060505, rough=0.06, coat=1.0, coat_rough=0.02, spec=0.6)
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    uv = nodes.new('ShaderNodeUVMap')
    uv.uv_map = 'UVMap'
    sep = nodes.new('ShaderNodeSeparateXYZ')
    links.new(uv.outputs['UV'], sep.inputs['Vector'])
    ramp = nodes.new('ShaderNodeValToRGB')
    el = ramp.color_ramp.elements
    el[0].position, el[0].color = 0.0, lt.hex_color(0x030202)
    el[1].position, el[1].color = iris_out * 0.47, lt.hex_color(0x030202)
    for pos, col in ((iris_out * 0.55, iris), (iris_out * 0.85, iris), (iris_out, ring), (1.0, ring)):
        e = el.new(pos)
        e.color = lt.hex_color(col)
    links.new(sep.outputs['Y'], ramp.inputs['Fac'])
    links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
    return mat


def ghost_material(name, base, pale=0x9d978c, start=0.8):
    """Feather tips going pale and spectral (C): the vane fades from the plumage's black to a pale ash with a faint
    glow of its own, and its last part dissolves in a dither (the game's ghosts are masked and dithered, never
    translucent). U runs base to tip."""
    mat = principled(name, pale, rough=0.6, emit=0xe6dccb, emit_strength=0.0)
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    uv = nodes.new('ShaderNodeUVMap')
    uv.uv_map = 'UVMap'
    sep = nodes.new('ShaderNodeSeparateXYZ')
    links.new(uv.outputs['UV'], sep.inputs['Vector'])
    fade = nodes.new('ShaderNodeMapRange')
    fade.inputs['From Min'].default_value = start
    fade.inputs['From Max'].default_value = start + (1.0 - start) * 0.7
    links.new(sep.outputs['X'], fade.inputs['Value'])
    mix = nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    links.new(fade.outputs['Result'], mix.inputs['Factor'])
    mix.inputs['A'].default_value = lt.hex_color(base)
    mix.inputs['B'].default_value = lt.hex_color(pale)
    links.new(mix.outputs['Result'], bsdf.inputs['Base Color'])
    glow = nodes.new('ShaderNodeMath')
    glow.operation = 'MULTIPLY'
    glow.inputs[1].default_value = 0.28
    links.new(fade.outputs['Result'], glow.inputs[0])
    links.new(glow.outputs['Value'], bsdf.inputs['Emission Strength'])
    # The dither: a fine cell pattern against how far along the tip a point is.
    coords = nodes.new('ShaderNodeTexCoord')
    vor = nodes.new('ShaderNodeTexVoronoi')
    vor.inputs['Scale'].default_value = 9.0
    links.new(coords.outputs['Object'], vor.inputs['Vector'])
    split = nodes.new('ShaderNodeSeparateColor')
    links.new(vor.outputs['Color'], split.inputs['Color'])
    edge = nodes.new('ShaderNodeMapRange')
    edge.inputs['From Min'].default_value = 0.84
    edge.inputs['From Max'].default_value = 1.0
    edge.inputs['To Min'].default_value = 1.0
    edge.inputs['To Max'].default_value = 0.0
    links.new(sep.outputs['X'], edge.inputs['Value'])
    keep = nodes.new('ShaderNodeMath')
    keep.operation = 'LESS_THAN'
    links.new(split.outputs['Red'], keep.inputs[0])
    links.new(edge.outputs['Result'], keep.inputs[1])
    links.new(keep.outputs['Value'], bsdf.inputs['Alpha'])
    return mat


def ember_material(name, color=0xb4461f, strength=3.2):
    """A dull coal of soul-light (C's socket): warm, never a rarity color, brighter at its heart."""
    mat = principled(name, 0x2a0e06, rough=0.7, emit=color, emit_strength=strength)
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    facing = nodes.new('ShaderNodeLayerWeight')
    facing.inputs['Blend'].default_value = 0.45
    curve = nodes.new('ShaderNodeMapRange')
    curve.inputs['To Min'].default_value = strength
    curve.inputs['To Max'].default_value = strength * 0.25
    links.new(facing.outputs['Facing'], curve.inputs['Value'])
    links.new(curve.outputs['Result'], bsdf.inputs['Emission Strength'])
    return mat


def feather_material(name, color, rough=0.5, coat=0.14, coat_rough=0.26, sheen=0.02, aniso=0.45, spec=0.32,
                     coat_tint=0xdfe2ea, tip_color=None, tip_from=0.8):
    """Near-black plumage: a soft gloss stretched along each feather (anisotropic along its UV U, which runs base to
    tip), a clear coat for the glossy highlights crows show in low sun. tip_color fades the tips (U) toward it."""
    mat = principled(name, color, rough=rough, coat=coat, coat_rough=coat_rough, sheen=sheen, sheen_tint=0xb0b4c0,
                     spec=spec)
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Coat Tint'].default_value = lt.hex_color(coat_tint)
    if tip_color is None:
        # Each feather a shade lighter or darker than its neighbours, as real plumage is.
        var = nodes.new('ShaderNodeAttribute')
        var.attribute_type = 'GEOMETRY'
        var.attribute_name = 'var'
        shade = nodes.new('ShaderNodeMapRange')
        shade.inputs['To Min'].default_value = 0.72
        shade.inputs['To Max'].default_value = 1.3
        links.new(var.outputs['Fac'], shade.inputs['Value'])
        mul = nodes.new('ShaderNodeMix')
        mul.data_type = 'RGBA'
        mul.blend_type = 'MULTIPLY'
        mul.inputs['Factor'].default_value = 1.0
        mul.inputs['A'].default_value = lt.hex_color(color)
        links.new(shade.outputs['Result'], mul.inputs['B'])
        links.new(mul.outputs['Result'], bsdf.inputs['Base Color'])
        rough_n = nodes.new('ShaderNodeMapRange')
        rough_n.inputs['To Min'].default_value = rough - 0.06
        rough_n.inputs['To Max'].default_value = rough + 0.06
        links.new(var.outputs['Fac'], rough_n.inputs['Value'])
        links.new(rough_n.outputs['Result'], bsdf.inputs['Roughness'])
    if aniso:
        tangent = nodes.new('ShaderNodeTangent')
        tangent.direction_type = 'UV_MAP'
        tangent.uv_map = 'UVMap'
        links.new(tangent.outputs['Tangent'], bsdf.inputs['Tangent'])
        bsdf.inputs['Anisotropic'].default_value = aniso
    if tip_color is not None:
        uv = nodes.new('ShaderNodeUVMap')
        uv.uv_map = 'UVMap'
        sep = nodes.new('ShaderNodeSeparateXYZ')
        links.new(uv.outputs['UV'], sep.inputs['Vector'])
        ramp = nodes.new('ShaderNodeMapRange')
        ramp.inputs['From Min'].default_value = tip_from
        ramp.inputs['From Max'].default_value = 1.0
        links.new(sep.outputs['X'], ramp.inputs['Value'])
        mix = nodes.new('ShaderNodeMix')
        mix.data_type = 'RGBA'
        links.new(ramp.outputs['Result'], mix.inputs['Factor'])
        mix.inputs['A'].default_value = lt.hex_color(color)
        mix.inputs['B'].default_value = lt.hex_color(tip_color)
        links.new(mix.outputs['Result'], bsdf.inputs['Base Color'])
    return mat


# --- The crow ---

# The torso, in the body frame (its origin at the hips, x along the back toward the chest): center, semi-axes.
TORSO = [
    ((0.0, 0.0, 0.0), (7.0, 3.8, 4.2)),       # core
    ((4.0, 0.0, -0.4), (4.3, 3.7, 4.3)),      # breast
    ((-1.8, 0.0, -1.5), (5.0, 3.5, 3.5)),     # belly
    ((1.6, 0.0, 2.0), (4.4, 3.7, 2.6)),       # mantle, between the shoulders
    ((-5.8, 0.0, 0.5), (3.6, 2.6, 2.4)),      # rump
    ((-7.6, 0.0, -1.3), (2.6, 2.0, 1.9)),     # vent
]
THIGH = ((-0.6, 2.2, -3.2), (2.5, 1.5, 2.2))  # mirrored
NECK_BASE = (4.4, 0.0, 1.8)                    # body frame
# The head, in the head frame (its origin mid-skull, x along the beak): center, semi-axes.
HEAD = [
    ((0.0, 0.0, 0.0), (3.3, 2.8, 2.9)),       # skull
    ((1.9, 0.0, 0.45), (2.3, 1.9, 2.0)),      # forehead, sloping into the beak
    ((0.9, 1.0, -1.2), (2.1, 1.6, 1.6)),      # cheek (mirrored)
    ((-0.4, 0.0, -2.3), (2.4, 2.0, 2.2)),     # throat
    ((-1.6, 0.0, 0.3), (2.3, 2.5, 2.5)),      # nape
]
HEAD_BASE = (-1.4, 0.0, -1.8)                  # head frame: where the neck meets the skull


class Crow:
    """One crow in one pose, built in creature space; finish() places it in the scene."""

    def __init__(self, spec, pose, tag):
        self.spec = spec
        self.pose = pose
        self.tag = tag
        self.objects = []
        tilt = pose.get('tilt', spec['tilt'])
        self.body = Matrix.Translation(Vector(pose.get('hip', spec['hip']))) @ turn(pitch=-tilt)
        self.head_center = Vector(pose.get('head_at', spec['head_at']))
        hs = spec['head_scale']
        self.head = (Matrix.Translation(self.head_center)
                     @ turn(pose.get('head_yaw', 0.0), pose.get('head_pitch', 0.0), pose.get('head_roll', 0.0))
                     @ Matrix.Scale(hs, 4))
        self.head_rot = self.head.to_3x3().normalized()

    # Frames
    def b(self, p):
        return self.body @ Vector(p)

    def h(self, p):
        return self.head @ Vector(p)

    def hdir(self, d):
        return (self.head_rot @ Vector(d)).normalized()

    def build(self):
        self.build_skin()
        self.build_beak()
        self.build_eyes()
        self.build_legs()
        self.build_coat()
        self.build_features()
        for side in (1, -1):
            if self.pose.get('wings') == 'spread':
                self.build_wing_spread(side)
            else:
                self.build_wing_folded(side)
        self.build_tail()
        return self

    # --- Feather groups laid on the skin ---

    def shingle(self, part, inner, direction, flow, L, W, mat, shape='scale', lift=0.25, root=0.35, gap=0.02, **kw):
        """A feather lying on the skin where the ray from inner along direction leaves it, pointing along flow (made
        tangent there), bent to follow the skin, its tip raised by lift cm and its base tucked under root * L."""
        p0, n0 = self.skin.cast(inner, direction)
        if p0 is None:
            return
        f = Vector(flow)
        f = (f - n0 * f.dot(n0)).normalized()
        sv = n0.cross(f)
        inner = Vector(inner)

        def to3d(a, y, h):
            target = p0 + f * (a - root * L) + sv * y + n0 * 0.4
            q, nq = self.skin.toward(inner, target)
            return q + nq * (h + gap)
        tuck = 0.18 * min(W, 2.0)
        feather(part, to3d, L, W, mat, shape=shape,
                lift=lambda t: lift * smooth(root * 0.7, 1.0, t) ** 1.4 - tuck * (1.0 - smooth(0.0, root, t)), **kw)

    def build_coat(self):
        """The plumage the eye reads as feathers: rows of scallops on the breast and flanks, the throat's hackles,
        the nape, the thighs' trousers and the nasal bristles over the beak's base."""
        s = self.spec
        m = s['mats']
        rng = random.Random(s['seed'] + 3)
        coat = s['coat']
        part = Part(f'Hob{self.tag}Coat')
        body_rot = self.body.to_3x3()
        center = self.b((0.5, 0.0, 0.0))
        # Contour feathers in soft, staggered oval lobes: the upper breast under the hackles, the flanks where they
        # tuck under the wing, the belly over the thighs. Few and large, so they read as plumage, not scales.
        ragged = coat.get('ragged', 0.0)
        for row, (elev, az0, az1, count, L0, W0, lift0) in enumerate(coat['lobes']):
            for side in ((1, -1) if az0 > 0 else (1,)):
                for k in range(count):
                    f = (k + (0.5 if row % 2 else 0.0)) / max(count - (0.5 if row % 2 else 1.0), 1)
                    az = side * lerp(az0, az1, min(f, 1.0)) + rng.uniform(-5, 5)
                    el = elev + rng.uniform(-3, 3)
                    d = body_rot @ Vector((math.cos(math.radians(el)) * math.cos(math.radians(az)),
                                           math.cos(math.radians(el)) * math.sin(math.radians(az)),
                                           math.sin(math.radians(el))))
                    flow = body_rot @ Vector((-0.6, 0.0, -1.0)) + body_rot @ Vector((0.0, rng.uniform(-0.2, 0.2), 0.0))
                    L = L0 * rng.uniform(0.88, 1.12)
                    lift = lift0 * rng.uniform(0.75, 1.25)
                    if ragged and rng.random() < 0.18 * ragged:
                        lift += 0.5 * ragged
                    if rng.random() < coat.get('sparse', 0.0):
                        continue
                    shape = coat.get('lobe_shape', 'contour')
                    width = W0 * (0.62 if shape == 'pointed' else 1.0)
                    self.shingle(part, center, d, flow, L, width * rng.uniform(0.9, 1.1), m['feather'], shape=shape,
                                 lift=lift, root=0.45, nl=7, nw=2, cup=0.05, rachis=0.02,
                                 twist=rng.uniform(-0.15, 0.15) * (1.0 + 2.0 * ragged))
        # Throat hackles: pointed feathers fanning down over the upper breast.
        hc = self.h((0.0, 0.0, -0.6))
        ghosts = coat.get('ghost_hackles', ())
        rows = ((-58.0, 1.0), (-40.0, 0.8), (-72.0, 1.1))[:coat.get('hackle_rows', 2)]
        for k in range(coat['hackles']):
            az = lerp(-64.0, 64.0, k / max(coat['hackles'] - 1, 1)) + rng.uniform(-5, 5)
            for r, (elev, scale) in enumerate(rows):
                d = self.hdir((math.cos(math.radians(elev)) * math.cos(math.radians(az)),
                               math.cos(math.radians(elev)) * math.sin(math.radians(az)), math.sin(math.radians(elev))))
                flow = Vector((0.15, 0.0, -1.0))
                L = coat['hackle_len'] * scale * rng.uniform(0.85, 1.15)
                kw = dict(tip_mat=m['ghost'], tip_from=0.72) if (r == 0 and k in ghosts and 'ghost' in m) else {}
                self.shingle(part, hc, d, flow, L, L * 0.38, m['feather'], shape='pointed', root=0.3,
                             lift=coat['hackle_lift'] * rng.uniform(0.6, 1.3), nl=7, nw=2, rachis=0.04,
                             curl=rng.uniform(-0.3, 0.3) * (1.0 + coat['hackle_lift']), **kw)
        # The nape: rows flowing back and down from the crown into the mantle.
        for elev, count, size in coat['nape_rows']:
            for k in range(count):
                az = lerp(140.0, 220.0, k / max(count - 1, 1)) + rng.uniform(-6, 6)
                d = self.hdir((math.cos(math.radians(elev)) * math.cos(math.radians(az)),
                               math.cos(math.radians(elev)) * math.sin(math.radians(az)), math.sin(math.radians(elev))))
                flow = self.hdir((-1.0, 0.0, -0.6))
                L = size * rng.uniform(0.85, 1.15)
                self.shingle(part, self.h((0.0, 0.0, 0.0)), d, flow, L, L * 0.8, m['feather'], shape='scale',
                             lift=coat['nape_lift'] * rng.uniform(0.6, 1.3), nl=6, nw=2)
        # The head's short feathers: soft low rows flowing back over the crown and down the cheeks, clear of the eyes.
        hc0 = self.h((0.0, 0.0, 0.0))
        eyes = [self.hdir((s['eye_dir'][0], sd * s['eye_dir'][1], s['eye_dir'][2])) for sd in (1, -1)]
        hs = s['head_scale']
        for elev, az0, az1, count, L, W, lift in coat.get('head_rows', ()):
            for side in (1, -1):
                for k in range(count):
                    az = side * lerp(az0, az1, k / max(count - 1, 1)) + rng.uniform(-4, 4)
                    el = elev + rng.uniform(-3, 3)
                    d = self.hdir((math.cos(math.radians(el)) * math.cos(math.radians(az)),
                                   math.cos(math.radians(el)) * math.sin(math.radians(az)), math.sin(math.radians(el))))
                    if min(math.degrees(d.angle(e)) for e in eyes) < 26.0:
                        continue
                    self.shingle(part, hc0, d, self.hdir((-1.0, 0.0, -0.3)), L * hs * rng.uniform(0.9, 1.1),
                                 W * hs, m['feather'], shape='contour', lift=lift * rng.uniform(0.7, 1.3), root=0.45,
                                 nl=6, nw=2, cup=0.04)
        # The thighs' trousers: two rows of scallops hanging over the top of the leg.
        for side in (1, -1):
            c, _ = THIGH
            inner = self.b((c[0], side * c[1] * 0.6, c[2] + 0.8))
            for elev, count, size in ((-35.0, 5, 2.6), (-58.0, 4, 2.3)):
                for k in range(count):
                    az = lerp(-50.0, 70.0, k / max(count - 1, 1))
                    a = math.radians(az)
                    d = Vector((math.cos(math.radians(elev)) * math.sin(a) * 0.6,
                                side * math.cos(math.radians(elev)) * math.cos(a),
                                math.sin(math.radians(elev))))
                    d = (d + Vector((0.0, 0.0, 0.0))).normalized()
                    self.shingle(part, inner, d, Vector((0.0, 0.0, -1.0)), size, size * 0.85, m['feather'],
                                 shape='scale', lift=0.3, nl=6, nw=2)
        # Nasal bristles: stiff little feathers lying forward over the base of the upper mandible.
        bk = s['beak']
        for k in range(coat['bristles']):
            across = lerp(-0.6, 0.6, k / max(coat['bristles'] - 1, 1))
            y = across * bk['width'] * 0.75
            x_root = self.face_x - 0.7
            z_root = bk['gape'] + bk['depth_u'] * (0.9 - 0.3 * across * across)
            base = self.h((x_root, y, z_root))
            tipd = self.hdir((1.0, across * 0.18, -0.36 - 0.3 * across * across))
            nrm = self.hdir((0.0, across * 0.6, 1.0))
            L = coat['bristle_len'] * (1.0 - 0.25 * abs(across)) * rng.uniform(0.9, 1.1)
            feather(part, free_frame(base, tipd, nrm), L, L * 0.34, m['bristle'], shape='pointed', nl=6, nw=1,
                    bend=0.03, rachis=0.03, lift=lambda t: 0.04 * t)
        self.objects.append(part.finish(solidify=0.06))

    # --- Wings ---

    def wing_bed(self, side):
        """The folded wing lies on a bed wrapped round the body's side: bed(u, v, h) in creature space, u back from
        the wrist, v down from the wing's top line, h out from the bed. It is pushed out wherever the skin would
        swallow it."""
        w = self.spec['wing']
        sc = w['scale']
        O = Vector((w['wrist'][0], side * w['wrist'][1], w['wrist'][2]))
        A = Vector((-1.0, -side * w['conv'], w['rise'])).normalized()
        beta = math.radians(w['beta'])
        N = Vector((0.0, side * math.cos(beta), math.sin(beta)))
        N = (N - A * N.dot(A)).normalized()
        Vd = A.cross(N)
        if Vd.z > 0:
            Vd = -Vd
        R = w['R']
        hand = 1.0 if A.cross(Vd).dot(N) > 0 else -1.0

        def local(u, v, h):
            ang = v / R
            return O + A * u + Vd * ((R + h) * math.sin(ang)) + N * ((R + h) * math.cos(ang) - R)

        def normal(u, v):
            ang = v / R
            return N * math.cos(ang) + Vd * math.sin(ang)
        # How far the skin reaches past the bed, sampled along the wing.
        push = []
        for k in range(36):
            u = k * 0.8 * sc
            need = 0.0
            for v in (-1.0, 0.0, 1.2, 2.4, 3.6, 4.8):
                p = local(u, v * sc, 0.0)
                inner = Vector((p.x, 0.0, p.z * 0.3))
                hit, _ = self.skin.cast(self.body @ inner, self.body.to_3x3() @ (p - inner))
                if hit is None:
                    continue
                d_skin = (self.body.inverted() @ hit - inner).length
                need = max(need, d_skin + w['gap'] - (p - inner).length)
            push.append(need)
        smoothp = [max(push[max(i - 2, 0):i + 3]) for i in range(len(push))]

        def lift(u):
            x = min(max(u / (0.8 * sc), 0.0), len(smoothp) - 1.001)
            i = int(x)
            return lerp(smoothp[i], smoothp[i + 1], x - i)

        def bed(u, v, h):
            return self.body @ local(u, v, h + lift(u))
        return bed, hand, normal

    def bed_feather(self, part, bed, hand, sc, u0, v0, ang, L, W, layer, mat, **kw):
        a = math.radians(ang)
        ca, sa = math.cos(a), math.sin(a)

        def to3d(along, y, h):
            u = u0 * sc + along * ca + hand * (-sa) * y
            v = v0 * sc + along * sa + hand * ca * y
            return bed(u, v, layer + h)
        feather(part, to3d, L * sc, W * sc, mat, **kw)

    def build_wing_folded(self, side):
        s = self.spec
        w = s['wing']
        m = s['mats']
        sc = w['scale']
        rng = random.Random(s['seed'] + (11 if side > 0 else 23))
        bed, hand, _ = self.wing_bed(side)
        part = Part(f'Hob{self.tag}Wing{"L" if side > 0 else "R"}')
        mat = m['feather']
        tip_mat = m.get('ghost')
        missing = w.get('missing', ())
        broken = w.get('broken', {})
        # Primaries: under everything, the longest at the bottom of the stack; their tips step out past the tertials.
        for i in range(9, 0, -1):
            if f'P{i}' in missing:
                continue
            k = (i - 1) / 8.0
            L = lerp(16.5, 23.5, smooth(0.0, 1.0, k ** 0.8)) * (1.0 - 0.06 * smooth(0.85, 1.0, k))
            u0 = lerp(6.0, 2.2, k)
            v0 = lerp(3.5, 4.1, k)
            ang = lerp(-5.0, -2.0, k)
            layer = 0.05 + (9 - i) * 0.035
            shape = 'primary' if i >= 5 else 'flight'
            kw = dict(shape=shape, nl=12, nw=2, rachis=0.05, cup=0.05, asym=0.35, bend=-0.004)
            if f'P{i}' in broken:
                kw.update(shape='broken', jag=broken[f'P{i}'])
                L *= 0.72
            if tip_mat is not None and i in w.get('ghost_primaries', ()):
                kw.update(tip_mat=tip_mat, tip_from=0.8)
            self.bed_feather(part, bed, hand, sc, u0, v0, ang, L, 3.3, layer, mat, **kw)
        # Secondaries: along the forearm, stacked so their tips step down the back of the wing.
        for i in range(1, 10):
            if f'S{i}' in missing:
                continue
            k = (i - 1) / 8.0
            u0, v0 = lerp(2.2, 9.6, k), lerp(3.7, 1.5, k)
            L = lerp(12.6, 11.8, k)
            ang = lerp(6.0, 4.0, k)
            layer = 0.38 + k * 0.26
            kw = dict(shape='flight', nl=10, nw=2, rachis=0.05, cup=0.05, asym=0.2)
            if f'S{i}' in broken:
                kw.update(shape='broken', jag=broken[f'S{i}'])
                L *= 0.78
            self.bed_feather(part, bed, hand, sc, u0, v0, ang, L, 4.0, layer, mat, **kw)
        # Tertials: three broad feathers on top, near the back.
        for i in range(3):
            u0, v0 = 9.2 + i * 0.9, 1.15 - i * 0.45
            L = 11.6 - i * 0.6
            self.bed_feather(part, bed, hand, sc, u0, v0, 5.0 - i * 1.5, L, 4.4, 0.68 + i * 0.07, mat, shape='flight',
                             nl=10, nw=2, rachis=0.06, cup=0.06)
        # The arm: a soft pad (the wrist's bulge and the coverts' bed), with covert rows on it.
        pad_u0, pad_u1, pad_v0, pad_v1 = -0.3, 11.0, -0.3, 4.4
        uc, vc = (pad_u0 + pad_u1) * 0.5, (pad_v0 + pad_v1) * 0.5
        au, av = (pad_u1 - pad_u0) * 0.5, (pad_v1 - pad_v0) * 0.5
        T = w['pad']
        edge_h = 0.72          # the flight feathers' stack under the pad's edge

        def pad_r(u, v):
            return (abs((u / sc - uc) / au) ** 3 + abs((v / sc - vc) / av) ** 3) ** (1.0 / 3.0)

        def pad_h(u, v):
            # A rounded-rectangle pad, thickest at the wrist; its edge sits on the flight feathers' stack.
            r = pad_r(u, v)
            front = lerp(1.0, 0.5, smooth(0.0, 1.0, (u / sc - pad_u0) / (pad_u1 - pad_u0)))
            return edge_h + T * front * max(0.0, 1.0 - r ** 4) ** 0.45
        pad = Part(f'Hob{self.tag}Pad{"L" if side > 0 else "R"}')
        nr, nt = 14, 40
        rings = []
        # The top, from the middle out to the edge, then a skirt down into the bed (the leading edge's thickness).
        profile = [(math.sin(0.5 * math.pi * i / nr), None) for i in range(1, nr + 1)] + [(1.025, 0.55), (1.035, -0.7)]
        for rho, h_fixed in profile:
            ring = []
            for j in range(nt):
                th = 2.0 * math.pi * j / nt
                c, sn = math.cos(th), math.sin(th)
                cu = math.copysign(abs(c) ** (2.0 / 3.0), c)
                sv = math.copysign(abs(sn) ** (2.0 / 3.0), sn)
                u = (uc + au * cu * rho) * sc
                v = (vc + av * sv * rho) * sc
                h = pad_h(u, v) if h_fixed is None else edge_h * h_fixed
                ring.append(pad.vert(bed(u, v, h)))
            rings.append(ring)
        center_v = pad.vert(bed(uc * sc, vc * sc, pad_h(uc * sc, vc * sc)))
        for j in range(nt):
            j1 = (j + 1) % nt
            pad.face((center_v, rings[0][j1], rings[0][j]) if hand < 0 else (center_v, rings[0][j], rings[0][j1]),
                     [(0.5, 0.5)] * 3, m['body'])
        for i in range(len(rings) - 1):
            for j in range(nt):
                j1 = (j + 1) % nt
                q = (rings[i][j], rings[i + 1][j], rings[i + 1][j1], rings[i][j1])
                pad.face(q if hand < 0 else tuple(reversed(q)), [(0.5, 0.5)] * 4, m['body'])
        self.objects.append(pad.finish())
        # Covert rows on the pad: greater, median, then lesser and marginal coverts toward the wrist.
        rows = [(3.4, 12, 4.4, 2.3, 9.0), (2.4, 12, 3.2, 1.9, 8.0), (1.5, 11, 2.5, 1.6, 7.0), (0.7, 10, 2.0, 1.4, 6.0)]
        lifted = w.get('lifted', {})
        for r, (v_row, count, L, W, ang) in enumerate(rows):
            for k in range(count):
                u0 = lerp(0.9 + r * 0.25, 10.6 - r * 0.9, k / (count - 1)) + rng.uniform(-0.15, 0.15)
                v0 = v_row - 0.12 * k / count
                up = lifted.get((r, k), 0.0) if side > 0 else 0.0

                def to3d(along, y, h, u0=u0, v0=v0, a=math.radians(ang + (14.0 if up else 0.0))):
                    ca, sa = math.cos(a), math.sin(a)
                    u = u0 * sc + along * ca + hand * (-sa) * y
                    v = v0 * sc + along * sa + hand * ca * y
                    return bed(u, v, pad_h(u, v) + h)
                feather(part, to3d, L * sc * rng.uniform(0.92, 1.08) * (1.15 if up else 1.0), W * sc, mat, shape='round',
                        nl=7, nw=2, cup=0.05, rachis=0.03, twist=0.4 if up else 0.0,
                        lift=lambda t, up=up: (0.06 + (0.1 + up) * smooth(0.3, 1.0, t) ** (1.0 if not up else 1.6)
                                              - 0.2 * (1.0 - smooth(0.0, 0.35, t))))
        # Primary coverts and the alula, over the primaries' bases at the wrist.
        for k in range(5):
            u0, v0 = 0.6 + k * 0.7, 3.9 + k * 0.1

            def to3d(along, y, h, u0=u0, v0=v0, a=math.radians(4.0)):
                ca, sa = math.cos(a), math.sin(a)
                u = u0 * sc + along * ca + hand * (-sa) * y
                v = v0 * sc + along * sa + hand * ca * y
                return bed(u, v, max(pad_h(u, v), 0.55) + h)
            feather(part, to3d, (5.4 - k * 0.3) * sc, 2.3 * sc, mat, shape='round', nl=7, nw=2, cup=0.05,
                    lift=lambda t: 0.1 + 0.1 * t)
        # Scapulars: the back's feathers lapping over the wing's top edge.
        for k in range(6):
            u0, v0 = 1.5 + k * 1.75, -1.1 + 0.1 * k

            def to3d(along, y, h, u0=u0, v0=v0, a=math.radians(6.0)):
                ca, sa = math.cos(a), math.sin(a)
                u = u0 * sc + along * ca + hand * (-sa) * y
                v = v0 * sc + along * sa + hand * ca * y
                return bed(u, v, max(pad_h(u, v), 0.0) + 0.2 + h)
            feather(part, to3d, (5.6 + k * 0.25) * sc, 3.6 * sc, mat, shape='round', nl=7, nw=2, cup=0.05,
                    lift=lambda t: 0.15 * smooth(0.3, 1.0, t))
        self.objects.append(part.finish(solidify=0.08))

    def build_wing_spread(self, side):
        """The open wing (the flap and landing poses), built on its bones: shoulder, elbow, wrist and hand tip in a
        planform (x out along the span, y back along the chord). Secondaries ride the forearm, primaries fan from the
        hand into the crow's five fingers, covert rows lap over their roots on top and underneath. The wing is
        raised, swept and pitched by the pose, and its hand bends further by pose['curl']."""
        s = self.spec
        w = s['wing']
        m = s['mats']
        pose = self.pose
        sc = w['scale']
        rng = random.Random(s['seed'] + (31 if side > 0 else 47))
        part = Part(f'Hob{self.tag}OpenWing{"L" if side > 0 else "R"}')
        mat = m['feather']
        tip_mat = m.get('ghost')
        missing = w.get('missing', ())
        broken = w.get('broken', {})
        M = (Matrix.Rotation(math.radians(-side * pose['sweep']), 3, 'Z')
             @ Matrix.Rotation(math.radians(side * pose['dihedral']), 3, 'X')
             @ Matrix.Rotation(math.radians(-pose['aoa']), 3, 'Y'))
        span = M @ Vector((0.0, side, 0.0))
        chord = M @ Vector((-1.0, 0.0, 0.0))
        up = M @ Vector((0.0, 0.0, 1.0))
        # The wing turns in the level frame (only its root rides the body): spread out to the side however the
        # body is pitched.
        root = self.b((3.6, side * 2.2, 2.6))
        wrist_x = 15.5 * sc
        curl = Matrix.Rotation(math.radians(side * pose.get('curl', 0.0)), 3, chord)
        bones = dict(S=(0.0, 0.0), E=(7.0, 1.0), W=(15.5, -0.4), T=(23.0, 1.0))

        def at(x, y, h=0.0):
            x, y = x * sc, y * sc
            if x > wrist_x:
                return root + span * wrist_x + chord * y + curl @ (span * (x - wrist_x) + up * h)
            return root + span * x + chord * y + up * h

        def direction(theta, hand):
            d = span * math.sin(math.radians(theta)) + chord * math.cos(math.radians(theta))
            return (curl @ d) if hand else d

        def normal(hand, flip=False):
            n = (curl @ up) if hand else up
            return -n if flip else n

        def lerp2(a, b, f):
            return (lerp(bones[a][0], bones[b][0], f), lerp(bones[a][1], bones[b][1], f))

        def put(base, theta, L, W, layer, under=False, **kw):
            hand = base[0] * sc > wrist_x - 0.3
            n = normal(hand, under)
            p = at(base[0], base[1]) + n * layer
            feather(part, free_frame(p, direction(theta, hand), n), L * sc, W * sc, mat, **kw)

        asym = 0.35 * side
        # Secondaries, wrist to elbow, and the tertials by the body.
        for i in range(1, 11):
            if f'S{i}' in missing:
                continue
            f = (i - 1) / 9.0
            kw = dict(shape='flight', nl=10, nw=2, rachis=0.05, cup=0.04, asym=0.18 * side, bend=0.006)
            L = lerp(14.6, 13.4, f)
            if f'S{i}' in broken:
                kw.update(shape='broken', jag=broken[f'S{i}'])
                L *= 0.78
            b = lerp2('W', 'E', f)
            put((b[0], b[1] + 0.5), lerp(10.0, -3.0, f), L, 4.0, 0.04 * (10 - i), **kw)
        for k in range(3):
            b = lerp2('E', 'S', 0.15 + 0.25 * k)
            put((b[0], b[1] + 0.3), -8.0 - 6.0 * k, 12.5 - 0.8 * k, 4.6, 0.46 + 0.03 * k, shape='flight', nl=10, nw=2,
                rachis=0.05, cup=0.05)
        # Primaries fanning from the hand: the outer five emarginated into fingers.
        lengths = (15.5, 17.2, 19.0, 20.8, 22.4, 23.8, 24.6, 24.2, 21.8)
        angles = (14.0, 19.0, 24.0, 29.0, 35.0, 43.0, 52.0, 61.0, 70.0)
        for i in range(1, 10):
            if f'P{i}' in missing:
                continue
            f = (i - 1) / 8.0
            kw = dict(shape='primary' if i >= 6 else 'flight', nl=13, nw=2, rachis=0.05, cup=0.04, asym=asym,
                      bend=-0.012 * f, curl=-0.5 * side * (1.0 - f))
            L = lengths[i - 1]
            if f'P{i}' in broken:
                kw.update(shape='broken', jag=broken[f'P{i}'])
                L *= 0.72
            if tip_mat is not None and i in w.get('ghost_primaries', ()):
                kw.update(tip_mat=tip_mat, tip_from=0.74)
            b = lerp2('W', 'T', f ** 0.9)
            put((b[0], b[1] + 0.2), angles[i - 1], L, 4.4 if i < 6 else 4.1, 0.04 * i, **kw)
        put(bones['T'], 86.0, 7.0, 2.2, 0.4, shape='round', nl=8, nw=2)
        # Coverts on top: greater, median and two rows of lesser along the arm; primary coverts and the alula on the
        # hand; scapulars lapping the wing's root.
        # Every row lies behind the arm (the bone line is the leading edge); the front row wraps over it.
        for k in range(10):
            f = k / 9.0
            b = lerp2('W', 'E', f)
            put((b[0], b[1] + 2.0), lerp(12.0, 0.0, f), 6.4 * rng.uniform(0.9, 1.08), 3.4, 0.62 + 0.015 * k,
                shape='round', nl=8, nw=2, cup=0.05, rachis=0.03)
            put((b[0], b[1] + 1.4 + 0.6 * (k % 2)), lerp(14.0, 2.0, f) + rng.uniform(-4, 4), rng.uniform(4.0, 5.4), 2.9,
                0.55 + 0.06 * (k % 2), under=True, shape='round', nl=7, nw=2, cup=0.04)
        for k in range(9):
            f = k / 8.0
            b = lerp2('W', 'E', f)
            put((b[0] + rng.uniform(-0.2, 0.2), b[1] + 0.9), lerp(14.0, 2.0, f), 4.0, 2.8, 0.86, shape='round', nl=7,
                nw=2, cup=0.05)
        for row, (dy, L, W, layer) in enumerate(((0.1, 2.9, 2.2, 1.02), (-0.7, 2.6, 1.9, 1.2))):
            for k in range(12):
                f = k / 11.0
                b = lerp2('W', 'E', f) if f > 0.0 else bones['W']
                if k > 7:
                    b = lerp2('E', 'S', (k - 7) / 5.0)
                put((b[0], b[1] + dy), lerp(18.0, -6.0, f), L, W, layer, shape='round', nl=6, nw=2, cup=0.05)
            for k in range(8):
                f = k / 7.0
                b = lerp2('E', 'S', f) if k < 4 else lerp2('W', 'E', (k - 4) / 4.0)
                put((b[0], b[1] + dy + 0.6), lerp(0.0, -10.0, f), L * 1.2, W, -0.85 - 0.12 * row, under=True,
                    shape='round', nl=6, nw=2, cup=0.04)
        for k in range(8):
            f = k / 7.0
            b = lerp2('W', 'T', f)
            put((b[0], b[1] + 0.2), lerp(14.0, 70.0, f), lerp(7.5, 5.0, f), 3.0, 0.72 + 0.02 * k, shape='round', nl=8,
                nw=2, cup=0.04)
        # The alula lies along the leading edge at the wrist.
        for k in range(3):
            put((bones['W'][0] - 1.6 + 0.6 * k, bones['W'][1] - 0.2), 84.0 + 3.0 * k, 4.2 - 0.5 * k, 1.4, 1.15 + 0.05 * k,
                shape='round', nl=7, nw=1)
        for k in range(5):
            put((0.8 + 1.4 * k, -0.6), -14.0 + 3.0 * k, 6.2, 3.6, 1.3, shape='round', nl=7, nw=2, cup=0.05)
        # The arm under the coverts: shoulder, elbow, wrist, hand.
        arm = []
        for a, b, n in (('S', 'E', 4), ('E', 'W', 5), ('W', 'T', 5)):
            for k in range(n):
                arm.append(lerp2(a, b, k / n))
        arm.append(bones['T'])
        # The leading edge is no rod but a fairing: an airfoil section, round in front, running back under the
        # coverts, from the shoulder to the hand's tip.
        rings = []
        segs = 16
        for x, y in arm:
            k = (x / 23.0) ** 0.8
            chord_len, thick = lerp(3.6, 1.1, k), lerp(1.05, 0.28, k)
            ring = []
            for j in range(segs):
                ph = 2.0 * math.pi * j / segs
                c = 0.5 * (1.0 - math.cos(ph))
                hh = math.sin(ph) * thick * 0.5 * (1.0 - 0.55 * c)
                ring.append(part.vert(at(x, y - 0.35 + c * chord_len, 0.2 + hh)))
            rings.append(ring)
        for i in range(len(rings) - 1):
            for j in range(segs):
                j1 = (j + 1) % segs
                q = (rings[i][j], rings[i][j1], rings[i + 1][j1], rings[i + 1][j])
                part.face(q if side > 0 else tuple(reversed(q)), [(0.5, 0.5)] * 4, m['body'])
        for ring, flip in ((rings[0], side > 0), (rings[-1], side < 0)):
            hub = part.vert(sum((v.co for v in ring), Vector()) / segs)
            for j in range(segs):
                tri = (hub, ring[(j + 1) % segs], ring[j])
                part.face(tri if flip else tuple(reversed(tri)), [(0.5, 0.5)] * 3, m['body'])
        self.objects.append(part.finish(solidify=0.09))

    # --- Tail ---

    def build_tail(self):
        s = self.spec
        t = s['tail']
        m = s['mats']
        part = Part(f'Hob{self.tag}Tail')
        spread = self.pose.get('tail_spread', t['spread'])
        droop = self.pose.get('tail_droop', t['droop'])
        rot = self.body.to_3x3() @ Matrix.Rotation(math.radians(droop), 3, 'Y')
        d0 = rot @ Vector((-1.0, 0.0, 0.0))
        n0 = rot @ Vector((0.0, 0.0, 1.0))
        left = n0.cross(d0)
        base = self.b(t['base'])
        tip_mat = m.get('ghost')
        missing = t.get('missing', ())
        for k in sorted(range(12), key=lambda k: -abs(k - 5.5)):
            kk = k - 5.5
            if k in missing:
                continue
            ang = math.radians(spread * kk / 11.0)
            d = Matrix.Rotation(-ang, 3, n0) @ d0
            frac = abs(kk) / 5.5
            L = t['length'] * (1.0 - t['grad'] * frac ** 1.6)
            pos = base + left * (-kk * 0.28) + n0 * (-0.11 * abs(kk))
            kw = dict(shape='flight', nl=11, nw=2, rachis=0.06, cup=0.05, bend=t.get('bend', 0.012),
                      asym=(0.3 * frac if kk < 0 else -0.3 * frac) if frac > 0.2 else 0.0)
            if tip_mat is not None and k in t.get('ghost', ()):
                kw.update(tip_mat=tip_mat, tip_from=0.86)
            if k in t.get('broken', {}):
                kw.update(shape='broken', jag=t['broken'][k])
                L *= 0.8
            feather(part, free_frame(pos, d, n0), L, t['width'], m['feather'], **kw)
        # Upper tail coverts over the tail's root, under tail coverts beneath it.
        for k in range(5):
            kk = k - 2
            d = Matrix.Rotation(math.radians(-kk * 7.0), 3, n0) @ d0
            pos = base - d0 * 2.2 + n0 * 0.55 + left * (-kk * 0.6)
            feather(part, free_frame(pos, d, n0), 7.4 - abs(kk) * 0.5, 3.4, m['feather'], shape='round', nl=8, nw=2,
                    cup=0.06, lift=lambda tt: 0.1 * tt)
        for k in range(5):
            kk = k - 2
            d = (Matrix.Rotation(math.radians(-kk * 9.0), 3, n0) @ (d0 - n0 * 0.25)).normalized()
            pos = base - d0 * 3.0 - n0 * 0.9 + left * (-kk * 0.7)
            feather(part, free_frame(pos, d, -n0), 6.4 - abs(kk) * 0.4, 3.6, m['feather'], shape='round', nl=8, nw=2,
                    cup=0.08)
        self.objects.append(part.finish(solidify=0.1))

    def build_skin(self):
        s = self.spec
        bq = self.body.to_quaternion()
        hq = self.head.to_quaternion()
        hs = s['head_scale']
        fl = s.get('fluff', 1.0)
        els = []
        for c, semi in TORSO:
            els.append((self.b(c), bq, [v * fl for v in semi], 2.4, False))
        for side in (1, -1):
            c, semi = THIGH
            els.append((self.b((c[0], side * c[1], c[2])), bq, semi, 2.4, False))
        for c, semi in HEAD:
            for side in ((1, -1) if c[1] else (1,)):
                els.append((self.h((c[0], side * c[1], c[2])), hq, [v * hs for v in semi], 2.4, False))
        # The neck: three balls from the shoulders to the base of the skull, so the head turns on a soft neck.
        a = self.b(NECK_BASE)
        z = self.h(HEAD_BASE)
        for k, t in enumerate((0.2, 0.5, 0.8)):
            r = lerp(3.5 * s.get('neck_thick', 1.0), 2.5 * hs, t)
            els.append((a.lerp(z, t), bq.slerp(hq, t), (r, r * 0.95, r), 2.4, False))
        for c, semi, stiff in s.get('sockets', []):
            els.append((self.h(c), hq, [v * hs for v in semi], stiff, True))
        bm = blob_mesh(f'Hob{self.tag}Skin', els, 0.3 if not QUICK else 0.45)
        self.skin = Surface(bm)
        mesh = bpy.data.meshes.new(f'Hob{self.tag}Skin')
        bm.to_mesh(mesh)
        bm.free()
        for p in mesh.polygons:
            p.use_smooth = True
        mesh.materials.append(self.spec['mats']['body'])
        obj = bpy.data.objects.new(f'Hob{self.tag}Skin', mesh)
        bpy.context.scene.collection.objects.link(obj)
        self.objects.append(obj)

    def build_beak(self):
        """Two mandibles lofted along the gape line; the upper one's culmen curves down over the lower's tip."""
        s = self.spec
        m = s['mats']
        bk = s['beak']
        part = Part(f'Hob{self.tag}Beak')
        L = bk['length']
        # The beak leaves the face where a ray along it from mid-skull leaves the skin; its root sits inset behind.
        hit, _ = self.skin.cast(self.h((0.0, 0.0, bk['gape'] + 0.45)), self.hdir((1.0, 0.0, 0.0)))
        self.face_x = (self.head.inverted() @ hit).x if hit is not None else 3.6
        x0 = self.face_x - bk['inset']
        n = 16
        notch = bk.get('notch')

        def gape(t):
            return bk['gape'] - bk['droop'] * t ** 1.6

        def section(t, upper):
            if upper:
                w = bk['width'] * (1.0 - t) ** 0.72 + 0.04
                h = bk['depth_u'] * (1.0 - t ** 1.25) ** 0.85 + 0.03
                if notch:
                    tc, wd, dp = notch
                    h *= 1.0 - dp * math.exp(-((t - tc) / wd) ** 2)
            else:
                w = bk['width'] * 0.86 * (1.0 - min(t, 1.0)) ** 0.8 + 0.035
                h = bk['depth_l'] * (1.0 - min(t, 1.0) ** 1.15) ** 0.9 + 0.025
            return w, h

        for upper in (True, False):
            t_end = 1.0 if upper else bk.get('lower_end', 0.92)
            rings = []
            for i in range(n):
                t = t_end * (i / (n - 1)) ** 0.9
                w, hgt = section(t, upper)
                zg = gape(t)
                ring = []
                for j in range(16):
                    ph = 2.0 * math.pi * j / 16
                    c, sn = math.cos(ph), math.sin(ph)
                    y = w * c
                    if upper:
                        z = zg + (hgt * abs(sn) ** 0.85 if sn >= 0.0 else -0.05 * abs(sn) * w)
                    else:
                        z = zg - (hgt * abs(sn) ** 0.9 if sn < 0.0 else -0.04 * abs(sn) * w) - 0.02
                    if upper and notch and t > notch[0] and y > 0:
                        y *= 1.0 - 0.35 * smooth(notch[0], notch[0] + 0.12, t)     # a chipped side near the tip
                    ring.append(part.vert(self.h((x0 + t * L, y, z))))
                rings.append(ring)
            mat = m['beak']
            for i in range(n - 1):
                for j in range(16):
                    j1 = (j + 1) % 16
                    part.face((rings[i][j], rings[i + 1][j], rings[i + 1][j1], rings[i][j1]),
                              [(i / n, j / 16), ((i + 1) / n, j / 16), ((i + 1) / n, (j + 1) / 16), (i / n, (j + 1) / 16)], mat)
            tip_t = t_end + (0.03 if upper else 0.01)
            tip = part.vert(self.h((x0 + tip_t * L, 0.0, gape(tip_t) + (0.02 if upper else -0.04))))
            for j in range(16):
                part.face((rings[-1][j], tip, rings[-1][(j + 1) % 16]), [(1, 0)] * 3, mat)
            hub = part.vert(self.h((x0 - 0.3, 0.0, gape(0.0))))
            for j in range(16):
                part.face((rings[0][(j + 1) % 16], hub, rings[0][j]), [(0, 0)] * 3, mat)
        self.objects.append(part.finish())

    def eye_spot(self, side):
        """The eye's place on the skin (a ray from mid-skull), and the skin normal there."""
        ex, ey, ez = self.spec['eye_dir']
        return self.skin.cast(self.h((0.4, 0.0, 0.0)), self.hdir((ex, side * ey, ez)))

    def eye_frame(self, side):
        """The eye's place, and its frame: z out of the skin, y up the head, x toward the beak."""
        p, n = self.eye_spot(side)
        up = self.hdir((0.0, 0.0, 1.0))
        y = (up - n * up.dot(n)).normalized()
        x = y.cross(n) if side < 0 else n.cross(y)
        fwd = self.hdir((1.0, 0.0, 0.0))
        if x.dot(fwd) < 0:
            x = -x
        return p, n, x, y

    def build_eyes(self):
        """The good (right) eye: a glossy ball with its iris, lids that set the expression (spec['eye']), and
        whatever is left of the left one (spec['missing'])."""
        s = self.spec
        m = s['mats']
        e = s['eye']
        part = Part(f'Hob{self.tag}Eye')
        skin = Part(f'Hob{self.tag}Lids')
        r = s['eye_radius'] * s['head_scale']
        p, n, x, y = self.eye_frame(-1)
        c = p - n * r * 0.45
        rot = Matrix((x, y, n)).transposed()
        ellipsoid(part, c, (r, r, r), m['eye'], rot=rot, segs=20, rings=14)
        self.lid(skin, c, r * 1.1, x, y, n, e['upper'], e['tilt'], m['skin'], top=True)
        self.lid(skin, c, r * 1.08, x, y, n, e['lower'], -e['tilt'] * 0.3, m['skin'], top=False)
        self.objects += [part.finish(), skin.finish()]
        getattr(self, 'missing_' + s['missing'])()

    def lid(self, part, c, R, x, y, n, cover, tilt, mat, top=True):
        """An eyelid: a patch of the sphere round the eye from its rim to a lid line at cover (0 open, 1 shut) of its
        height, tilted by tilt (positive: the line drops toward the beak, a scowl), with a rolled edge."""
        sgn = 1.0 if top else -1.0
        cols, rows = 14, 6
        grid = []
        edge = []
        for i in range(cols + 1):
            u = -1.0 + 2.0 * i / cols
            u *= 0.98
            rim = math.sqrt(max(1.0 - u * u, 0.0))
            line = sgn * (1.0 - 2.0 * cover) - tilt * u * sgn * (1.0 if top else 1.0)
            line = max(min(line, rim), -rim) if top else min(max(line, -rim), rim)
            col = []
            for j in range(rows + 1):
                t = j / rows
                v = lerp(line, sgn * rim * 1.0, t)
                w = math.sqrt(max(1.0 - u * u - v * v, 0.0))
                back = smooth(0.7, 1.0, t) * 0.35
                pt = c + (x * u + y * v) * R + n * (w * R - back * R)
                col.append(part.vert(pt))
            grid.append(col)
            edge.append(col[0])
        for i in range(cols):
            for j in range(rows):
                q = (grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1])
                part.face(q if top else tuple(reversed(q)), [(0.5, 0.5)] * 4, mat)
        tube(part, [v.co.copy() for v in edge], [R * 0.09] * len(edge), mat, sides=6, caps=(True, True))

    # The left eye, three ways.
    def missing_scar(self):
        """A: the lids grown shut over a sunken socket, and a pale scar of bare skin from crown to cheek."""
        s = self.spec
        m = s['mats']
        hs = s['head_scale']
        part = Part(f'Hob{self.tag}Scar')
        p, n, x, y = self.eye_frame(1)
        r = s['eye_radius'] * hs
        rot = Matrix((x, y, n)).transposed()
        inner = self.h((0.4, 0.0, 0.0))
        # A bare patch where no feathers grew back round the lost eye.
        gash = (self.hdir(s['scar_path'][-1]) - self.hdir(s['scar_path'][0])).normalized()
        self.shingle(part, inner, p - inner, gash, 2.6 * hs, 1.25 * hs, m['scar'],
                     shape='contour', lift=0.0, root=0.5, gap=-0.01, nl=9, nw=3)
        # The lids grown shut: a puckered almond, slanted, with a dark seam.
        slant = Matrix.Rotation(math.radians(-20.0), 3, n)
        ellipsoid(part, p - n * 0.04, (r * 1.0, r * 0.5, r * 0.16), m['scar'], rot=slant @ rot, segs=18, rings=10)
        seam = [p + n * (r * 0.12 + 0.01) + slant @ (x * (u * r * 0.95)) + n * (-0.06 * u * u) for u in
                (-1.0, -0.6, -0.2, 0.2, 0.6, 1.0)]
        tube(part, seam, [0.03, 0.05, 0.06, 0.06, 0.05, 0.03], m['skin'], sides=6)
        # The gash that took it: a raised scar across the patch, from above and behind the eye to the cheek.
        path = self.head_path(s['scar_path'], 24)
        widths = [0.22 * hs * math.sin(math.pi * (0.05 + 0.9 * i / max(len(path) - 1, 1))) ** 0.7 for i in range(len(path))]
        self.strip(part, path, widths, 0.04, m['gash'], lift=0.02)
        # The feathers round the patch stand ragged, lifting away from it.
        rng = random.Random(s['seed'] + 5)
        for k in range(6):
            a = 2.0 * math.pi * k / 6 + rng.uniform(-0.2, 0.2)
            radial = x * math.cos(a) + y * math.sin(a) * 0.8
            q, nq = self.skin.toward(inner, p + radial * 1.1 * hs + n * 0.3)
            d = (radial * 0.45 + nq * rng.uniform(0.08, 0.2) + self.hdir((-1.0, 0.0, -0.3)) * 0.6).normalized()
            feather(part, free_frame(q - nq * 0.15, d, nq), rng.uniform(1.0, 1.5) * hs, 0.6 * hs, m['feather'],
                    shape='pointed', nl=5, nw=1, bend=0.02)
        self.objects.append(part.finish(solidify=0.04))

    def missing_eyecup(self):
        """B: a little brass eyecup over the socket, held by a band of black crepe knotted behind the head, its two
        tails trailing."""
        s = self.spec
        m = s['mats']
        hs = s['head_scale']
        p, n, x, y = self.eye_frame(1)
        cup = Part(f'Hob{self.tag}Eyecup')
        rc = 1.12 * hs
        hc = 0.46 * hs
        prof = [(0.0, hc), (0.3 * rc, hc * 0.95), (0.62 * rc, hc * 0.74), (0.86 * rc, hc * 0.42), (0.98 * rc, hc * 0.26),
                (1.08 * rc, hc * 0.22), (1.14 * rc, hc * 0.1), (1.12 * rc, -hc * 0.05), (1.0 * rc, -hc * 0.3)]
        lathe(cup, p - n * 0.1, n, x, prof, 28, m['brass'])
        lathe(cup, p - n * 0.1 + n * hc * 0.96, n, x, [(0.0, 0.08 * hs), (0.16 * rc, 0.06 * hs), (0.2 * rc, 0.0),
                                                      (0.18 * rc, -0.05)], 16, m['brass'])
        self.objects.append(cup.finish())
        band = Part(f'Hob{self.tag}Crepe')
        path = self.head_path(s['band_path'], 48, closed=True)
        self.strip(band, path, [0.62 * hs] * len(path), 0.05, m['crepe'], lift=0.06, closed=True)
        # The knot behind the head, and its tails.
        kp, kn = self.skin.cast(self.h((0.4, 0.0, 0.0)), self.hdir(s['knot_dir']))
        ellipsoid(band, kp + kn * 0.18 * hs, (0.42 * hs, 0.32 * hs, 0.26 * hs), m['crepe'],
                  rot=kn.to_track_quat('Z', 'X').to_matrix(), segs=12, rings=8)
        down = Vector((0.0, 0.0, -1.0))
        for k, (out, length, twist) in enumerate(((0.55, 6.4, 0.7), (0.3, 5.6, -0.6))):
            d0 = (self.hdir((-1.0, out, 0.15)) + down * (0.25 + 0.2 * k)).normalized()
            side = d0.cross(kn).normalized()
            pts, sides = [], []
            for i in range(11):
                t = i / 10
                # Out from the knot, then drooping as crepe does, with a lazy S in it.
                p = (kp + kn * 0.25 * hs + d0 * (t * length * hs) + down * (1.9 * t * t * hs)
                     + side * (math.sin(t * math.pi * 1.5 + k * 1.3) * 0.45 * hs))
                pts.append(p)
                sides.append(Matrix.Rotation(twist * t, 3, d0) @ side)
            ribbon(band, pts, sides, 0.95 * hs, m['crepe'], swallowtail=0.6 * hs, across=4)
        self.objects.append(band.finish(solidify=0.04))

    def missing_ember(self):
        """C: the socket is open; a dull ember of soul-light sits deep in it and warms its rim."""
        s = self.spec
        m = s['mats']
        hs = s['head_scale']
        p, n, x, y = self.eye_frame(1)
        part = Part(f'Hob{self.tag}Ember')
        r = 0.3 * hs
        c = p - n * 0.06 * hs - x * 0.18 * hs - y * 0.1 * hs
        ellipsoid(part, c, (r, r * 0.9, r * 0.8), m['ember'], rot=Matrix((x, y, n)).transposed(), segs=14, rings=10)
        self.objects.append(part.finish())
        self.ember_at = c + n * 0.45 * hs

    def head_path(self, dirs, samples, closed=False):
        """Skin points and normals along a path given as head-frame directions (from mid-skull), smoothly
        interpolated (Catmull-Rom)."""
        pts = [Vector(d).normalized() for d in dirs]
        if closed:
            pts = pts + pts[:3]
        out = []
        n = samples
        segs = len(pts) - (3 if closed else 1)
        for i in range(n):
            t = i / (n - (0 if closed else 1)) * segs
            k = min(int(t), segs - 1)
            f = t - k
            p0 = pts[max(k - 1, 0)] if not closed or k > 0 else pts[-4]
            p1, p2 = pts[k], pts[min(k + 1, len(pts) - 1)]
            p3 = pts[min(k + 2, len(pts) - 1)]
            d = 0.5 * ((2 * p1) + (-p0 + p2) * f + (2 * p0 - 5 * p1 + 4 * p2 - p3) * f * f
                       + (-p0 + 3 * p1 - 3 * p2 + p3) * f * f * f)
            hit, nrm = self.skin.cast(self.h((0.4, 0.0, 0.0)), self.hdir(d))
            if hit is not None:
                out.append((hit, nrm))
        return out

    def strip(self, part, path, widths, height, mat, lift=0.03, across=5, closed=False):
        """A ribbon lying on the skin along path (points, normals), raised in a rounded profile."""
        rows = []
        n = len(path)
        for i, (p, nrm) in enumerate(path):
            a = path[(i + 1) % n if closed else min(i + 1, n - 1)][0]
            b = path[(i - 1) % n if closed else max(i - 1, 0)][0]
            t = (a - b).normalized()
            side = nrm.cross(t).normalized()
            w = widths[i]
            rows.append([part.vert(p + side * (0.5 * w * xx) + nrm * (lift + height * (1.0 - xx * xx)))
                         for xx in (-1.0 + 2.0 * k / (across - 1) for k in range(across))])
        last = n if closed else n - 1
        for i in range(last):
            r0, r1 = rows[i], rows[(i + 1) % n]
            for k in range(across - 1):
                part.face((r0[k], r1[k], r1[k + 1], r0[k + 1]), [(i / n, k / across), ((i + 1) / n, k / across),
                                                                 ((i + 1) / n, (k + 1) / across), (i / n, (k + 1) / across)], mat)

    def build_features(self):
        """Each concept's extras: A's ragged crown, B's message tube."""
        s = self.spec
        m = s['mats']
        hs = s['head_scale']
        rng = random.Random(s['seed'] + 9)
        part = Part(f'Hob{self.tag}Extras')
        for d, flow, L, up, shape in s.get('tufts', ()):
            p, n = self.skin.cast(self.h((0.0, 0.0, 0.0)), self.hdir(d))
            f = self.hdir(flow)
            f = (f - n * f.dot(n)).normalized()
            along = (f * math.cos(math.radians(up)) + n * math.sin(math.radians(up))).normalized()
            feather(part, free_frame(p - n * 0.25, along, n), L * hs, L * (0.32 if shape == 'pointed' else 0.42) * hs,
                    m['feather'], shape=shape, nl=7, nw=2, bend=-0.05, rachis=0.04, curl=rng.uniform(-0.25, 0.25) * L,
                    jag=(0.0, 0.12, 0.04, 0.16, 0.02))
        tube_spec = s.get('message_tube')
        if tube_spec:
            heel = Vector((s['heel'][0], -s['heel'][1], s['heel'][2]))
            foot = Vector((s['foot'][0], -s['foot'][1], 0.55))
            axis = (heel - foot).normalized()
            mid = foot.lerp(heel, 0.48)
            out = Vector((0.25, -1.0, 0.0)).normalized()
            c = mid + out * 0.62
            tube(part, [c - axis * 0.8, c + axis * 0.8], [0.3, 0.3], m['brass'], sides=14)
            for end in (-0.85, 0.85):
                ellipsoid(part, c + axis * end, (0.33, 0.33, 0.12), m['brass'],
                          rot=axis.to_track_quat('Z', 'X').to_matrix(), segs=14, rings=6)
            for z in (-0.4, 0.4):
                ring = [mid + axis * z + (out * math.cos(a) + out.cross(axis) * math.sin(a)) * 0.5 + out * 0.3 * (1 + math.cos(a)) * 0.5
                        for a in (2 * math.pi * k / 16 for k in range(17))]
                tube(part, ring, [0.07] * 17, m['crepe'], sides=6, caps=(False, False))
        if part.bm.verts:
            self.objects.append(part.finish(solidify=0.03))
        else:
            part.bm.free()

    def build_legs(self):
        s = self.spec
        m = s['mats']
        part = Part(f'Hob{self.tag}Legs')
        perch = self.pose['perch']
        reach = self.pose.get('legs') == 'reach'
        for side in (1, -1):
            if reach:
                # Landing: the legs reach forward and down for the perch, the toes spread open.
                heel = Vector((self.pose['heel'][0], side * self.pose['heel'][1], self.pose['heel'][2]))
                foot = Vector((self.pose['foot'][0], side * self.pose['foot'][1], self.pose['foot'][2]))
                pts = [heel.lerp(foot, t) for t in (0.0, 0.3, 0.6, 0.85, 1.0)]
            else:
                foot = Vector((s['foot'][0], side * s['foot'][1], 0.0))
                heel = Vector((s['heel'][0], side * s['heel'][1], s['heel'][2]))
                pts = [heel.lerp(foot + Vector((0, 0, 0.55)), t) for t in (0.0, 0.3, 0.6, 0.85, 1.0)]
            tube(part, pts, [0.5, 0.44, 0.41, 0.4, 0.45], m['leg'], sides=10)
            base = pts[-1] if reach else foot + Vector((0.0, 0.0, 0.42))
            for ang, length, claw, rad in s['toes']:
                a = math.radians(ang * side if ang < 150 else 180.0 - (180.0 - ang) * side)
                if reach:
                    pitch = math.radians(-40.0 if ang >= 150 else -22.0)
                    spread = 1.35 if ang < 150 else 1.0
                    a = math.radians((ang * spread) * side if ang < 150 else 180.0 - (180.0 - ang) * side)
                    d = Vector((math.cos(a) * math.cos(pitch), math.sin(a) * math.cos(pitch), math.sin(pitch)))
                    path = [base + d * (length * t) - UP * (0.35 * length * t * t) for t in (0.0, 0.25, 0.5, 0.75, 1.0)]
                else:
                    d = Vector((math.cos(a), math.sin(a), 0.0))
                    path = perch.wrap(base, d, length, lift=0.32)
                radii = [rad * (1.0 - 0.3 * k / (len(path) - 1)) * (1.0 + 0.12 * math.sin(k * 2.2)) for k in range(len(path))]
                tube(part, path, radii, m['leg'], sides=8, caps=(True, False))
                # The claw: a curved cone hooking down into the perch.
                end = path[-1]
                dd = (path[-1] - path[-2]).normalized()
                down = UP.copy() if reach else perch.normal_at(end)
                cpts, crad = [], []
                for k in range(7):
                    t = k / 6
                    bend = dd.lerp(-down, min(1.0, t * 1.1)).normalized()
                    cpts.append(end + dd * claw * 0.6 * t + bend * claw * 0.45 * t * t - down * 0.05 * t)
                    crad.append(radii[-1] * 0.82 * (1.0 - t) ** 0.9 + 0.025)
                tube(part, cpts, crad, m['claw'], sides=8, caps=(False, True))
        self.objects.append(part.finish())

    def check(self):
        """Logs any part reaching far outside the bird (a degenerate vertex) and its triangle count."""
        tris = 0
        for o in self.objects:
            if o.type != 'MESH':
                continue
            tris += sum(len(p.vertices) - 2 for p in o.data.polygons)
            bad = [v.index for v in o.data.vertices if not all(math.isfinite(c) for c in v.co) or v.co.length > 70.0]
            if bad:
                log(f'{o.name}: {len(bad)} stray vertices, e.g. {tuple(o.data.vertices[bad[0]].co)}')
        return tris

    def finish(self, location, heading=0.0):
        """Places the crow: its grip point at location (meters), facing heading (degrees about z)."""
        self.tris = self.check()
        mw = Matrix.Translation(Vector(location)) @ Matrix.Rotation(math.radians(heading), 4, 'Z') @ Matrix.Scale(CM, 4)
        for o in self.objects:
            o.matrix_world = mw
        if hasattr(self, 'ember_at'):
            # The coal's light on the socket's walls (the game fakes it with an emissive rim).
            glow = bpy.data.objects.new('EmberGlow', bpy.data.lights.new('EmberGlow', 'POINT'))
            glow.data.energy = 0.02
            glow.data.color = (1.0, 0.45, 0.2)
            glow.data.shadow_soft_size = 0.003
            glow.location = mw @ self.ember_at
            bpy.context.scene.collection.objects.link(glow)
            self.objects.append(glow)
        return self.objects


# --- Perches (creature space for the feet; the scene builds the wood in meters) ---

class FlatTop:
    """The flat top of a post or board around the grip point: a rectangle (x from x0 to x1, y from y0 to y1, cm)
    whose edges round over by r. Toes walk on it and curl over its edges."""

    def __init__(self, x0, x1, y0, y1, r=0.8):
        self.x0, self.x1, self.y0, self.y1, self.r = x0, x1, y0, y1, r

    def normal_at(self, p):
        if self.x0 + self.r < p.x < self.x1 - self.r and self.y0 + self.r < p.y < self.y1 - self.r and p.z > -0.2:
            return UP.copy()
        out = Vector((0.0, 0.0, 0.0))
        if p.x >= self.x1 - self.r:
            out.x = 1.0
        elif p.x <= self.x0 + self.r:
            out.x = -1.0
        if p.y >= self.y1 - self.r:
            out.y = 1.0
        elif p.y <= self.y0 + self.r:
            out.y = -1.0
        if out.length < 1e-6:
            return UP.copy()
        k = max(0.0, min(1.0, -p.z / self.r))
        return (UP * (1.0 - k) + out.normalized() * (0.3 + k)).normalized()

    def wrap(self, start, d, length, lift=0.3, step=0.25):
        """A toe's centerline from start along d over the top for length cm, over the edge and down the face."""
        pts = [Vector(start)]
        p = Vector((start.x, start.y, 0.0))
        dirv = Vector((d.x, d.y, 0.0)).normalized()
        travelled = 0.0
        state = 'top'
        while travelled < length - 1e-6:
            s = min(step, length - travelled)
            if state == 'top':
                q = p + dirv * s
                if not (self.x0 + self.r <= q.x <= self.x1 - self.r and self.y0 + self.r <= q.y <= self.y1 - self.r):
                    state = 'edge'
                    edge_start = p.copy()
                    angle = 0.0
                    continue
                p = q
            elif state == 'edge':
                angle += s / (self.r + lift)
                if angle >= math.pi / 2:
                    angle = math.pi / 2
                    state = 'face'
                c = edge_start - UP * self.r
                p = c + dirv * math.sin(angle) * self.r + UP * math.cos(angle) * self.r
            else:
                p = p - UP * s
            travelled += s
            n = self.normal_at(p)
            pts.append(p + n * lift)
        # Thin the polyline to about 8 points.
        if len(pts) > 9:
            idx = [round(i * (len(pts) - 1) / 8) for i in range(9)]
            pts = [pts[i] for i in idx]
        return pts


# --- Concepts ---

def mats_for(key, c):
    """The preview materials of a concept (c: its 'look')."""
    m = dict(
        feather=feather_material(f'Hob{key}Feather', c['black'], rough=c['rough'], coat=c['coat'], aniso=c['aniso']),
        body=feather_material(f'Hob{key}Body', c['black'], rough=min(c['rough'] + 0.1, 0.8), coat=c['coat'] * 0.6, aniso=0.0,
                              sheen=0.22),
        beak=principled(f'Hob{key}Beak', c.get('beak', 0x19191b), rough=0.4, coat=0.15, coat_rough=0.25, spec=0.4),
        leg=principled(f'Hob{key}Leg', 0x1f1e20, rough=0.55, spec=0.4),
        claw=principled(f'Hob{key}Claw', 0x121111, rough=0.3),
        skin=principled(f'Hob{key}Skin', 0x29252a, rough=0.6),
        bristle=principled(f'Hob{key}Bristle', 0x0f0f10, rough=0.75, spec=0.25),
        eye=eye_material(f'Hob{key}Eye', c['iris']),
        scar=principled(f'Hob{key}Scarskin', 0x4b4243, rough=0.55, sheen=0.1),
        gash=principled(f'Hob{key}Gash', 0x7a6e6a, rough=0.6, sheen=0.15),
        brass=principled(f'Hob{key}Brass', 0xc49c56, rough=0.3, metal=1.0),
        crepe=principled(f'Hob{key}Crepe', 0x1d1b20, rough=0.88, sheen=0.4, sheen_tint=0x8a8690, spec=0.3),
        ember=ember_material(f'Hob{key}Ember'),
    )
    if c.get('ghost'):
        m['ghost'] = ghost_material(f'Hob{key}Ghost', c['black'])
    return m


EYE_DIR = (0.42, 0.86, 0.26)

# Body lobes: (elevation, azimuth from, to (a positive start mirrors the row on both sides), count, length, width,
# lift), round the body frame's middle. Upper breast, flanks under the wing, belly over the thighs.
LOBES = [(12.0, -44.0, 44.0, 7, 4.6, 2.6, 0.13), (-14.0, -52.0, 52.0, 8, 4.6, 2.6, 0.13),
         (-12.0, 66.0, 118.0, 5, 4.8, 2.8, 0.15), (-40.0, -46.0, 46.0, 6, 4.2, 2.5, 0.12)]


# Head rows: (elevation, azimuth from, to (both sides), count, length, width, lift), head frame, round mid-skull.
HEAD_ROWS = [(62.0, 15.0, 165.0, 6, 1.5, 1.2, 0.08), (36.0, 30.0, 170.0, 7, 1.45, 1.15, 0.08),
             (10.0, 60.0, 172.0, 6, 1.4, 1.1, 0.07), (-18.0, 40.0, 150.0, 6, 1.4, 1.1, 0.07)]


def head_rows(lift=1.0):
    return [(e, a0, a1, c, L, W, li * lift) for e, a0, a1, c, L, W, li in HEAD_ROWS]


def lobes(lift=1.0, rows=None):
    return [(e, a0, a1, c, L, W, li * lift) for i, (e, a0, a1, c, L, W, li) in enumerate(LOBES)
            if rows is None or i in rows]


BASE = dict(
    seed=7, tilt=38.0, hip=(-0.4, 0.0, 10.8), head_at=(3.4, 0.0, 21.2), neck_thick=0.95, head_scale=1.3,
    eye_radius=0.56, eye_dir=EYE_DIR, fluff=1.0,
    beak=dict(inset=1.3, length=5.2, width=1.1, depth_u=1.5, depth_l=1.02, gape=-0.85, droop=0.55),
    foot=(0.6, 2.3), heel=(-0.5, 2.5, 4.8),
    toes=[(-24.0, 2.7, 1.0, 0.36), (2.0, 3.7, 1.15, 0.38), (30.0, 3.0, 1.0, 0.36), (190.0, 2.8, 1.45, 0.38)],
    coat=dict(lobes=lobes(), ragged=0.0, hackles=7, hackle_len=3.0, hackle_lift=0.22, hackle_rows=2,
              nape_rows=[(35.0, 5, 2.6), (8.0, 6, 2.8)], nape_lift=0.16, bristles=6, bristle_len=2.3),
    wing=dict(wrist=(5.6, 3.3, 2.6), conv=0.15, rise=0.1, beta=50.0, R=6.5, scale=1.0, gap=0.2, pad=0.85),
    tail=dict(base=(-7.6, 0.0, 0.6), length=19.0, width=4.1, grad=0.14, spread=16.0, droop=18.0),
    eye=dict(upper=0.3, lower=0.12, tilt=0.0),
    look=dict(black=0x141417, rough=0.5, coat=0.14, aniso=0.45, iris=0x3a2414),
    missing='scar', sockets=[],
)

# The three directions. Each overrides the base crow; dicts merge key by key.
CONCEPTS = {
    # A, the battered veteran: hunched and fluffed, a head a size too big, a scarred socket grown shut, a chipped
    # beak, ragged crown and hackles, two primaries gone and a broken secondary, dull dusty plumage.
    'A': dict(
        name='Veteran', seed=17, tilt=31.0, hip=(-0.4, 0.0, 10.3), head_at=(4.3, 0.0, 19.6), head_scale=1.44,
        fluff=1.06, neck_thick=1.08,
        beak=dict(notch=(0.56, 0.08, 0.72), length=5.0, depth_u=1.55),
        look=dict(black=0x1b1917, rough=0.62, coat=0.05, aniso=0.3, iris=0x3c2414, beak=0x232120),
        eye=dict(upper=0.46, lower=0.16, tilt=0.38),
        missing='scar', sockets=[((1.3, 2.62, 0.84), (0.95, 0.55, 0.85), 2.4)],
        scar_path=[(-0.05, 0.5, 0.86), (0.2, 0.74, 0.62), (0.42, 0.86, 0.26), (0.62, 0.72, -0.1), (0.76, 0.55, -0.36)],
        tufts=[((-0.35, 0.05, 0.94), (-1.0, 0.1, 0.3), 2.2, 28.0, 'pointed'),
               ((-0.55, -0.3, 0.78), (-1.0, -0.5, 0.1), 2.0, 36.0, 'round'),
               ((-0.1, 0.25, 0.96), (-0.9, 0.5, 0.4), 1.7, 46.0, 'pointed'),
               ((-0.75, 0.15, 0.62), (-1.0, 0.3, -0.4), 2.3, 24.0, 'broken')],
        coat=dict(lobes=lobes(1.0), ragged=0.8, lobe_shape='pointed', sparse=0.35, head_rows=head_rows(0.9),
                  hackles=8, hackle_len=3.4,
                  hackle_lift=0.42, nape_lift=0.18, nape_rows=[(30.0, 4, 2.4)]),
        wing=dict(missing=('P5', 'P6'), broken={'S4': (0.02, 0.09, 0.0, 0.12, 0.04)}, lifted={(1, 4): 0.9, (2, 8): 0.7}),
        tail=dict(length=18.0, missing=(3,), broken={8: (0.0, 0.1, 0.03, 0.14, 0.05)}),
        hero=dict(perch='post', heading=-150.0, pose=dict(head_yaw=42.0, head_pitch=8.0, head_roll=-14.0)),
    ),
    # B, the dapper messenger: upright and sleek, glossy, a long neat tail and long wingtips crossed like the tails of
    # an undertaker's coat; a brass eyecup over the socket on a band of mourning crepe, its tails trailing; a brass
    # message tube on his leg.
    'B': dict(
        name='Messenger', seed=29, tilt=46.0, hip=(-0.9, 0.0, 11.0), head_at=(3.0, 0.0, 23.0), head_scale=1.24,
        fluff=0.95, neck_thick=0.92,
        beak=dict(length=5.4, depth_u=1.45, width=1.05),
        look=dict(black=0x101114, rough=0.34, coat=0.32, aniso=0.6, iris=0x47290f),
        eye=dict(upper=0.22, lower=0.1, tilt=-0.3),
        missing='eyecup', sockets=[((1.36, 2.72, 0.86), (0.8, 0.4, 0.75), 2.4)],
        band_path=[(0.2, 0.95, 0.3), (-0.35, 0.82, 0.48), (-0.85, 0.2, 0.45), (-0.75, -0.45, 0.4),
                   (-0.25, -0.9, 0.22), (0.05, -0.62, -0.8), (0.35, 0.1, -0.95), (0.55, 0.62, -0.5), (0.5, 0.85, 0.05)],
        knot_dir=(-0.86, 0.22, 0.42),
        message_tube=True,
        coat=dict(lobes=lobes(0.45, rows=(0, 1, 2)), head_rows=(), hackles=5, hackle_len=2.2,
                  hackle_lift=0.06, hackle_rows=1,
                  nape_lift=0.05),
        wing=dict(scale=1.06, conv=0.18),
        tail=dict(length=20.5, grad=0.12),
        hero=dict(perch='headboard_fresh', heading=-145.0, pose=dict(head_yaw=22.0, head_pitch=-8.0, head_roll=8.0)),
    ),
    # C, the Revenant crow: lean and brooding, a raven's shaggy ruff and heavy beak, a long wedge tail; the empty
    # socket holds a dull ember of soul-light, the good eye has gone pale, and his wingtips, tail and a few hackles
    # fade to ash and dissolve: what he used to be.
    'C': dict(
        name='Revenant', seed=41, tilt=34.0, hip=(-0.5, 0.0, 10.6), head_at=(4.9, 0.0, 19.8), head_scale=1.32,
        fluff=0.97, neck_thick=1.0,
        beak=dict(length=6.6, depth_u=1.75, depth_l=1.12, width=1.18, droop=0.85),
        look=dict(black=0x151517, rough=0.5, coat=0.12, aniso=0.45, iris=0xb4b2a8, ghost=True),
        eye=dict(upper=0.38, lower=0.2, tilt=0.15),
        missing='ember', sockets=[((1.16, 2.38, 0.72), (1.05, 0.9, 1.0), 3.0)],
        coat=dict(lobes=lobes(0.7), ragged=0.3, head_rows=head_rows(0.55), hackles=11, hackle_len=5.0,
                  hackle_lift=0.65, hackle_rows=3,
                  nape_lift=0.32),
        wing=dict(ghost_primaries=(6, 7, 8, 9)),
        tail=dict(length=22.5, grad=0.4, width=4.3, ghost=(5, 6)),
        hero=dict(perch='headboard_old', heading=-155.0, pose=dict(head_yaw=32.0, head_pitch=12.0, head_roll=-14.0)),
    ),
}


def merged(base, over):
    out = dict(base)
    for k, v in over.items():
        out[k] = merged(base[k], v) if isinstance(v, dict) and isinstance(base.get(k), dict) else v
    return out


def spec_for(key):
    s = merged(BASE, CONCEPTS[key])
    s['hero'] = merged(HERO, s.get('hero', {}))
    s['mats'] = mats_for(key, s['look'])
    return s


# --- Scene and rendering ---

def clear_scene():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for m in list(bpy.data.meshes):
        if m.users == 0:
            bpy.data.meshes.remove(m)


def render_setup(res, samples, engine='CYCLES'):
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Punchy'
    if engine == 'CYCLES':
        scene.render.engine = 'CYCLES'
        scene.cycles.device = 'CPU'
        scene.cycles.samples = samples
        scene.cycles.use_adaptive_sampling = True
        scene.cycles.adaptive_threshold = 0.015
        scene.cycles.use_denoising = True
        scene.cycles.denoiser = 'OPENIMAGEDENOISE'
        scene.cycles.max_bounces = 8
        scene.cycles.diffuse_bounces = 3
        scene.cycles.glossy_bounces = 3
        scene.cycles.transparent_max_bounces = 8
        scene.cycles.use_auto_tile = True
        scene.render.film_transparent = False
    else:
        scene.render.engine = 'BLENDER_EEVEE_NEXT'
        scene.eevee.taa_render_samples = samples


def simple_world(color_low=0x8d877c, color_high=0xc9c2b4, strength=1.0):
    world = bpy.data.worlds.get('HobWorld') or bpy.data.worlds.new('HobWorld')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    split = nodes.new('ShaderNodeSeparateXYZ')
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(coords.outputs['Generated'], split.inputs['Vector'])
    links.new(split.outputs['Z'], ramp.inputs['Fac'])
    ramp.color_ramp.elements[0].position, ramp.color_ramp.elements[0].color = 0.45, lt.hex_color(color_low)
    ramp.color_ramp.elements[1].position, ramp.color_ramp.elements[1].color = 0.75, lt.hex_color(color_high)
    links.new(ramp.outputs['Color'], bg.inputs['Color'])
    bg.inputs['Strength'].default_value = strength
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    bpy.context.scene.world = world


def add_sun(direction_to_sun, energy=4.0, color=(1.0, 0.86, 0.68), angle=2.0):
    sun = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN'))
    sun.data.energy = energy
    sun.data.color = color
    sun.data.angle = math.radians(angle)
    sun.rotation_euler = (-Vector(direction_to_sun).normalized()).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(sun)
    return sun


def add_camera(location, target, lens=50.0, ortho=None, fstop=None, focus=None):
    cam = bpy.data.objects.new('Camera', bpy.data.cameras.new('Camera'))
    cam.location = Vector(location)
    d = Vector(target) - Vector(location)
    cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens = lens
    cam.data.clip_start = 0.01
    cam.data.clip_end = 500.0
    if ortho:
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = ortho
    if fstop:
        cam.data.dof.use_dof = True
        cam.data.dof.aperture_fstop = fstop
        cam.data.dof.focus_distance = focus if focus else d.length
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    return cam


def ground(z=0.0, color=0x77766c, size=200.0):
    mesh = bpy.data.meshes.new('Ground')
    mesh.from_pydata([(-size, -size, z), (size, -size, z), (size, size, z), (-size, size, z)], [], [(0, 1, 2, 3)])
    obj = bpy.data.objects.new('Ground', mesh)
    mesh.materials.append(principled('GroundClay', color, rough=1.0))
    bpy.context.scene.collection.objects.link(obj)
    return obj


def golden_world(sun_dir, horizon=0xf0c890, mid=0xd9c7a8, zenith=0x86a2c6, strength=1.0, glow=0x5a3510, glow_power=6.0,
                 glow_strength=2.5):
    """A late-afternoon sky: warm haze at the horizon, soft blue overhead, a warm glow round the low sun."""
    world = bpy.data.worlds.get('HobGolden') or bpy.data.worlds.new('HobGolden')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    norm = nodes.new('ShaderNodeVectorMath')
    norm.operation = 'NORMALIZE'
    links.new(coords.outputs['Generated'], norm.inputs[0])
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(norm.outputs['Vector'], split.inputs['Vector'])
    ramp = nodes.new('ShaderNodeValToRGB')
    el = ramp.color_ramp.elements
    el[0].position, el[0].color = 0.0, lt.hex_color(0x6e5a40)
    el[1].position, el[1].color = 0.55, lt.hex_color(zenith)
    e = el.new(0.06)
    e.color = lt.hex_color(horizon)
    e = el.new(0.2)
    e.color = lt.hex_color(mid)
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -0.08
    remap.inputs['From Max'].default_value = 1.0
    links.new(split.outputs['Z'], remap.inputs['Value'])
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    dot.inputs[1].default_value = Vector(sun_dir).normalized()
    links.new(norm.outputs['Vector'], dot.inputs[0])
    clamp = nodes.new('ShaderNodeMath')
    clamp.operation = 'MAXIMUM'
    clamp.inputs[1].default_value = 0.0
    links.new(dot.outputs['Value'], clamp.inputs[0])
    power = nodes.new('ShaderNodeMath')
    power.operation = 'POWER'
    power.inputs[1].default_value = glow_power
    links.new(clamp.outputs['Value'], power.inputs[0])
    scale = nodes.new('ShaderNodeMath')
    scale.operation = 'MULTIPLY'
    scale.inputs[1].default_value = glow_strength
    links.new(power.outputs['Value'], scale.inputs[0])
    add = nodes.new('ShaderNodeMix')
    add.data_type = 'RGBA'
    add.blend_type = 'ADD'
    links.new(scale.outputs['Value'], add.inputs['Factor'])
    links.new(ramp.outputs['Color'], add.inputs['A'])
    add.inputs['B'].default_value = lt.hex_color(glow)
    links.new(add.outputs['Result'], bg.inputs['Color'])
    bg.inputs['Strength'].default_value = strength
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    bpy.context.scene.world = world


def white_col(obj):
    """The textured materials darken by the 'Col' alpha (baked occlusion): white, or they'd render black."""
    mesh = obj.data
    if 'Col' not in mesh.color_attributes:
        col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
        col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))


def wood_post(size, height, at, heading, seed, top_bevel=0.012):
    """A weathered square fence post (the house trim's siding board, grain up the post), its top at height (m)."""
    post = lp.block((size, size, height + 0.3), (0.0, 0.0, (height - 0.3) * 0.5), bevel=top_bevel)
    lp.grain(post, 'Siding', axis=(0.0, 0.0, 1.0), seed=seed)
    lp.place(post, (at[0], at[1], 0.0), (0.0, 0.0, heading))
    lt.bake_vertex_ao(post, samples=12, distance=0.15, ground=False)
    post.data.polygons.foreach_set('use_smooth', [False] * len(post.data.polygons))
    return post


def wood_rail(length, at, heading, z, seed):
    rail = lp.block((length, 0.04, 0.13), (length * 0.5, 0.0, z), bevel=0.006)
    lp.grain(rail, 'Siding', axis=(1.0, 0.0, 0.0), seed=seed)
    lp.place(rail, (at[0], at[1], 0.0), (0.0, 0.0, heading))
    white_col(rail)
    return rail


def straw_ground(size=80.0):
    mesh = bpy.data.meshes.new('Ground')
    mesh.from_pydata([(-size, -size, 0.0), (size, -size, 0.0), (size, size, 0.0), (-size, size, 0.0)], [], [(0, 1, 2, 3)])
    obj = bpy.data.objects.new('Ground', mesh)
    bpy.context.scene.collection.objects.link(obj)
    lt.assign(obj, lt.material('Hay', name='HobStraw', tint=0xe8d2a0))
    lt.box_uv(obj, 'Hay')
    white_col(obj)
    return obj


def render(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    scene = bpy.context.scene
    scene.render.filepath = path
    start = time.time()
    bpy.ops.render.render(write_still=True)
    log(f'rendered {os.path.relpath(path, lt.REPO)} in {time.time() - start:.0f} s')


def shot_clay(key):
    """A quick clay check of the proportions: side, front and three-quarter views side by side."""
    clear_scene()
    spec = spec_for(key)
    perch = FlatTop(-6.0, 6.0, -6.0, 6.0)
    for k, heading in enumerate((-90.0, 180.0, -40.0, 90.0)):
        crow = Crow(spec, dict(perch=perch), f'{key}{k}').build()
        crow.finish((k * 0.5, 0.0, 0.0), heading)
    simple_world()
    add_sun((-0.5, -0.8, 0.6), energy=3.5)
    add_camera((0.75, -3.2, 0.17), (0.75, 0.0, 0.17), lens=50.0, ortho=2.0)
    ground(-0.001)
    render_setup((2000, 640), 16, engine='EEVEE')
    render(os.path.join(OUT_DIR, 'work', f'Hob_{key}_clay.png'))


def shot_head(key):
    """A work check of the head: left side, three-quarter front and front, close up."""
    clear_scene()
    spec = spec_for(key)
    perch = FlatTop(-6.0, 6.0, -6.0, 6.0)
    for k, heading in enumerate((180.0, -135.0, -90.0)):
        crow = Crow(spec, dict(perch=perch), f'{key}{k}').build()
        crow.finish((k * 0.3, 0.0, 0.0), heading)
    simple_world(0x9a958c, 0xd8d2c6, 1.0)
    add_sun((-0.3, -0.8, 0.5), energy=3.0, color=(1.0, 0.9, 0.8))
    hz = (spec['head_at'][2]) * CM
    if 'beak' in ARGV:                       # a close look at the front crow's face
        add_camera((0.6, -3.0, hz - 0.01), (0.6, 0.0, hz - 0.01), lens=50.0, ortho=0.16)
    else:
        add_camera((0.3, -3.0, hz), (0.3, 0.0, hz), lens=50.0, ortho=0.82)
    render_setup((1500, 560), 64 if not QUICK else 24)
    render(os.path.join(OUT_DIR, 'work', f'Hob_{key}_head.png'))


LAND = dict(tilt=58.0, hip=(-2.6, 0.0, 15.2), head_at=(1.0, 0.0, 25.4), head_pitch=26.0, head_yaw=8.0,
            head_roll=0.0, wings='spread', dihedral=40.0, sweep=14.0, aoa=34.0, curl=16.0, tail_spread=110.0,
            tail_droop=34.0, legs='reach', heel=(-0.6, 2.6, 9.6), foot=(3.2, 2.9, 4.4))


def shot_wings(key, view='front'):
    """The flap pose: landing on the post, wings open and raised, tail fanned, feet reaching."""
    clear_scene()
    spec = spec_for(key)
    top = 1.2
    heading = -150.0
    grip = Vector((0.0, 0.0, top))
    cam = heading_vec(heading + 38.0) * 1.75 + Vector((0.0, 0.0, -0.03))
    if view == 'top':
        cam = heading_vec(heading + 160.0) * 1.0 + Vector((0.0, 0.0, 1.3))
    perch = post_scene(grip, heading, top, cam * -1.0, seed=spec['seed'])
    pose = dict(LAND, perch=perch, **spec.get('land', {}))
    Crow(spec, pose, key).build().finish(grip, heading)
    sun_dir = Vector((-0.45, -0.85, 0.35)).normalized()
    golden_world(Vector((-0.8, 0.6, 0.06)), strength=0.7)
    add_sun(sun_dir, energy=5.0, color=(1.0, 0.72, 0.47), angle=1.0)
    rim = bpy.data.objects.new('Rim', bpy.data.lights.new('Rim', 'AREA'))
    rim.data.energy = 22.0
    rim.data.size = 0.8
    rim.data.color = (1.0, 0.85, 0.65)
    rim.location = Vector((0.75, 0.95, 1.6))
    rim.rotation_euler = (grip + Vector((0, 0, 0.2)) - rim.location).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(rim)
    # Bounce off the sunlit straw, so the undersides of the open wings read.
    fill = bpy.data.objects.new('Bounce', bpy.data.lights.new('Bounce', 'AREA'))
    fill.data.energy = 16.0
    fill.data.size = 1.5
    fill.data.color = (1.0, 0.8, 0.55)
    fill.location = grip + cam * 0.6 + Vector((0.0, 0.0, -0.9))
    fill.rotation_euler = (grip + Vector((0, 0, 0.2)) - fill.location).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(fill)
    target = grip + Vector((0.0, 0.0, 0.2))
    add_camera(target + cam, target, lens=50.0, fstop=8.0)
    res = (1600, 1000) if not QUICK else (800, 500)
    render_setup(res, 96 if not QUICK else 32)
    name = f'Hob_{key}_Wings' + ('' if view == 'front' else '_' + view) + '.png'
    render(os.path.join(OUT_DIR, name if not QUICK else os.path.join('work', name)))


def label(text, at, size=0.05, color=0xe8dcc4):
    """Flat lettering facing the camera (-Y), for the work sheets."""
    curve = bpy.data.curves.new('Label', 'FONT')
    curve.body = text
    curve.size = size
    curve.align_x = 'CENTER'
    font = os.path.join(lt.REPO, 'Art', 'Fonts', 'Rye-Regular.ttf')
    if os.path.exists(font):
        curve.font = bpy.data.fonts.load(font, check_existing=True)
    obj = bpy.data.objects.new('Label', curve)
    obj.location = Vector(at)
    obj.rotation_euler = (math.radians(90.0), 0.0, 0.0)
    mat = bpy.data.materials.get('HobLabel') or bpy.data.materials.new('HobLabel')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    em = nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = lt.hex_color(color)
    em.inputs['Strength'].default_value = 1.0
    links.new(em.outputs['Emission'], out.inputs['Surface'])
    curve.materials.append(mat)
    obj.visible_shadow = False
    bpy.context.scene.collection.objects.link(obj)
    return obj


def stub(at, height, seed, size=0.13):
    """A short weathered post for the turnaround (its top at height)."""
    post = lp.block((size, size, height + 0.05), (0.0, 0.0, (height - 0.05) * 0.5), bevel=0.01)
    lp.grain(post, 'Siding', axis=(0.0, 0.0, 1.0), seed=seed)
    lp.place(post, (at[0], at[1], 0.0), (0.0, 0.0, 0.0))
    white_col(post)
    return post


def studio(sun=(-0.5, -0.8, 0.55)):
    simple_world(0xa8a195, 0xe2dccf, 1.25)
    add_sun(sun, energy=5.0, color=(1.0, 0.86, 0.7), angle=3.0)
    rim = bpy.data.objects.new('Rim', bpy.data.lights.new('Rim', 'AREA'))
    rim.data.energy = 60.0
    rim.data.size = 2.0
    rim.data.color = (1.0, 0.9, 0.78)
    rim.location = Vector((0.6, 2.2, 1.6))
    rim.rotation_euler = (Vector((0.6, 0.0, 0.3)) - rim.location).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(rim)
    mesh = bpy.data.meshes.new('Floor')
    mesh.from_pydata([(-30, -30, 0), (30, -30, 0), (30, 30, 0), (-30, 30, 0)], [], [(0, 1, 2, 3)])
    floor = bpy.data.objects.new('Floor', mesh)
    mesh.materials.append(principled('HobFloor', 0xa59d90, rough=0.95))
    bpy.context.scene.collection.objects.link(floor)


def shot_turn(key):
    """The turnaround: front, left side (the lost eye's side) and back, orthographic, in even studio light."""
    clear_scene()
    spec = spec_for(key)
    top = 0.24
    gap = 0.46
    for k, heading in enumerate((-90.0, 180.0, 90.0)):
        x = (k - 1) * gap
        stub((x, 0.0), top, spec['seed'] + k)
        perch = FlatTop(-6.5, 6.5, -6.5, 6.5, r=1.0)
        pose = dict(perch=perch, head_yaw=0.0, head_pitch=2.0, head_roll=0.0)
        Crow(spec, pose, f'{key}T{k}').build().finish((x, 0.0, top), heading)
    studio()
    add_camera((0.0, -4.0, 0.405), (0.0, 0.0, 0.405), ortho=1.34)
    res = (2400, 1000) if not QUICK else (1200, 500)
    render_setup(res, 64 if not QUICK else 24)
    name = f'Hob_{key}_Turnaround.png'
    render(os.path.join(OUT_DIR, name if not QUICK else os.path.join('work', name)))


def tall_post(height, seed, size=0.15):
    """A 1.8 m post with cream bands every half metre (the scale reference)."""
    post = lp.block((size, size, height + 0.3), (0.0, 0.0, (height - 0.3) * 0.5), bevel=0.012)
    lp.grain(post, 'Siding', axis=(0.0, 0.0, 1.0), seed=seed)
    white_col(post)
    cream = lt.material('PaintWorn', name='PaintCream', tint=0xe6dac2)
    for z in (0.5, 1.0, 1.5):
        band = lp.block((size + 0.006, size + 0.006, 0.025), (0.0, 0.0, z))
        lt.assign(band, cream)
        lt.box_uv(band, 'PaintWorn', seed=int(z * 10))
        white_col(band)
        label(f'{z:.1f} m', (size * 0.5 + 0.17, -size * 0.5, z - 0.02), size=0.065, color=0x1e1a17)
    label('1.8 m', (size * 0.5 + 0.17, -size * 0.5, height - 0.02), size=0.065, color=0x1e1a17)
    return post


def shot_scale(key):
    """Hob on top of a 1.8 m post, seen from a player's eye height (1.7 m) a few metres off: how big he is, and how
    he reads at the distance the game shows him."""
    clear_scene()
    spec = spec_for(key)
    height = 1.8
    heading = -120.0
    tall_post(height, spec['seed'])
    perch = FlatTop(-7.5 + 2.6, 2.6, -7.5, 7.5, r=1.0)
    grip = Vector((-heading_vec(heading).x * 0.049, -heading_vec(heading).y * 0.049, height))
    pose = dict(perch=perch, **spec['hero']['pose'])
    Crow(spec, pose, key).build().finish(grip, heading)
    straw_ground()
    cam = Vector((0.35, -3.2, 1.7))
    far_ridges(Vector((0.0, 1.0, 0.0)) - cam * 0.0)
    golden_world(Vector((-0.8, 0.6, 0.06)), strength=0.7)
    add_sun(Vector((-0.45, -0.85, 0.35)), energy=5.0, color=(1.0, 0.72, 0.47), angle=1.0)
    add_camera(cam, (0.0, 0.0, 1.05), lens=26.0)
    bpy.context.scene.camera.data.sensor_fit = 'VERTICAL'
    res = (1100, 1600) if not QUICK else (550, 800)
    render_setup(res, 64 if not QUICK else 24)
    name = f'Hob_{key}_Scale.png'
    render(os.path.join(OUT_DIR, name if not QUICK else os.path.join('work', name)))


def shot_compare():
    """The three side by side: the same fence, light, pose and scale."""
    clear_scene()
    top = 1.2
    heading = -148.0
    gap = 0.62
    for k, key in enumerate(('A', 'B', 'C')):
        spec = spec_for(key)
        # In a straight line across the view, each turned to show the side he lost the eye on.
        c = Vector(((k - 1) * gap, 0.0, 0.0))
        grip = Vector((c.x, c.y, top)) + heading_vec(heading) * 0.039
        wood_post(0.13, top, (c.x, c.y), heading, spec['seed'])
        perch = FlatTop(-13.0 + 2.6, 2.6, -6.5, 6.5, r=1.0)
        pose = dict(perch=perch, head_yaw=28.0, head_pitch=4.0, head_roll=-8.0)
        Crow(spec, pose, f'{key}C').build().finish(grip, heading)
        label(key + '  ' + spec['name'], (c.x + 0.0, c.y - 0.2, top - 0.17), size=0.04)
    straw_ground()
    view = Vector((0.0, 1.0, 0.0))
    far_ridges(view)
    golden_world(Vector((-0.8, 0.6, 0.06)), strength=0.7)
    add_sun(Vector((-0.45, -0.85, 0.35)), energy=5.0, color=(1.0, 0.72, 0.47), angle=1.0)
    rim = bpy.data.objects.new('Rim', bpy.data.lights.new('Rim', 'AREA'))
    rim.data.energy = 40.0
    rim.data.size = 1.5
    rim.data.color = (1.0, 0.85, 0.65)
    rim.location = Vector((0.8, 1.4, 1.8))
    rim.rotation_euler = (Vector((0.0, 0.0, 1.35)) - rim.location).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(rim)
    add_camera((0.0, -3.1, 1.38), (0.0, 0.0, 1.31), lens=62.0, fstop=11.0)
    res = (2400, 1100) if not QUICK else (1200, 550)
    render_setup(res, 96 if not QUICK else 32)
    name = 'Hob_Compare.png'
    render(os.path.join(OUT_DIR, name if not QUICK else os.path.join('work', name)))


def heading_vec(deg):
    return Vector((math.cos(math.radians(deg)), math.sin(math.radians(deg)), 0.0))


def far_ridges(view, colors=((150.0, 4.5, 0xb39a72), (330.0, 13.0, 0xc2ad8c), (700.0, 34.0, 0xcdbfa8))):
    """Unlit hazy silhouettes on the horizon (as the game's backdrop), arcs facing the view direction."""
    fwd = Vector((view.x, view.y, 0.0)).normalized()
    side = Vector((fwd.y, -fwd.x, 0.0))
    for layer, (dist, height, color) in enumerate(colors):
        mat = bpy.data.materials.get(f'HobHaze{layer}') or bpy.data.materials.new(f'HobHaze{layer}')
        mat.use_nodes = True
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        nodes.clear()
        out = nodes.new('ShaderNodeOutputMaterial')
        em = nodes.new('ShaderNodeEmission')
        em.inputs['Color'].default_value = lt.hex_color(color)
        em.inputs['Strength'].default_value = 1.0
        links.new(em.outputs['Emission'], out.inputs['Surface'])
        verts, faces = [], []
        n = 160
        for i in range(n + 1):
            a = math.radians(-75.0 + 150.0 * i / n)
            hgt = height * (0.55 + 0.25 * math.sin(a * 5.0 + layer * 2.1) + 0.15 * math.sin(a * 13.0 + layer)
                            + 0.05 * math.sin(a * 31.0))
            p = fwd * (math.cos(a) * dist) + side * (math.sin(a) * dist)
            verts += [(p.x, p.y, -2.0), (p.x, p.y, hgt)]
            if i:
                k = 2 * i
                faces.append((k - 2, k, k + 1, k - 1))
        mesh = bpy.data.meshes.new(f'Ridge{layer}')
        mesh.from_pydata(verts, [], faces)
        mesh.materials.append(mat)
        obj = bpy.data.objects.new(f'Ridge{layer}', mesh)
        obj.visible_shadow = False
        bpy.context.scene.collection.objects.link(obj)


def post_scene(grip, heading, post_top, view, front_gap=0.026, size=0.13, seed=3, line=(-0.5, 1.0)):
    """A weathered fence post under the grip point (its front edge front_gap m ahead of it), the fence line running
    off behind it (posts every 2.4 m, two rails), straw ground and hazy ridges. Returns the perch top (creature cm)."""
    f = heading_vec(heading)
    center = Vector(grip) - f * (size * 0.5 - front_gap)
    center.z = 0.0
    wood_post(size, post_top, (center.x, center.y), heading, seed)
    run = Vector((line[0], line[1], 0.0)).normalized()
    across = Vector((run.y, -run.x, 0.0))
    run_heading = math.degrees(math.atan2(run.y, run.x))
    rng = random.Random(seed)
    prev = center
    for k in range(1, 14):
        c = center + run * (2.4 * k) + across * rng.uniform(-0.06, 0.06)
        wood_post(size, post_top + rng.uniform(-0.05, 0.04), (c.x, c.y), run_heading + rng.uniform(-6, 6), seed + k)
        for z in (post_top - 0.3, post_top - 0.72):
            start = prev + across * (size * 0.5 + 0.02)
            wood_rail(2.4, (start.x, start.y), run_heading, z + rng.uniform(-0.02, 0.02), seed + 40 + k)
        prev = c
    straw_ground()
    far_ridges(view)
    half = size * 50.0
    return FlatTop(-(size * 100.0 - front_gap * 100.0), front_gap * 100.0, -half, half, r=1.0)


def headboard(at, heading, seed, height, fresh=False, broken=False, width=0.5, thick=0.045, lean=0.0):
    """A grave's headboard (as the graves kit makes them): one board on the house trim, whitewashed with a black
    painted cross when fresh, weathered grey otherwise; its front faces heading, its top edge flat in the middle."""
    import looter_buildings as kit
    r = width * 0.5
    cut = 0.07
    if broken:
        outline = [(-r, -0.3), (r, -0.3), (r, height - 0.14), (r * 0.62, height - 0.06), (r * 0.42, height),
                   (-r + cut, height), (-r, height - cut)]
    else:
        outline = [(-r, -0.3), (r, -0.3), (r, height - cut), (r - cut, height), (-r + cut, height), (-r, height - cut)]
    board = lp.mesh_object(kit._prism([outline], thick))
    board.data.transform(Matrix.Translation((0.0, -thick * 0.5, 0.0)))
    parts = [board]
    if fresh:
        lt.assign(board, lp.trim_material())
        lt.trim_uv(board, None, 'G', u_offset=4.32, rotate=True, v_offset=0.1)
        black = lt.material('PaintWorn', name='PaintBlack', tint=0x2a2622)
        for size, c in (((0.035, 0.004, 0.32), (0.0, -thick * 0.5 - 0.002, height * 0.6)),
                        ((0.2, 0.004, 0.035), (0.0, -thick * 0.5 - 0.002, height * 0.68))):
            bar = lp.block(size, c)
            lt.assign(bar, black)
            lt.box_uv(bar, 'PaintWorn', seed=seed)
            parts.append(bar)
    else:
        lp.grain(board, 'Siding', axis=(0.0, 0.0, 1.0), seed=seed)
    obj = lp.join(f'Headboard{seed}', parts)
    obj.data.transform(Matrix.Rotation(math.radians(-lean), 4, 'X'))
    lp.place(obj, (at[0], at[1], 0.0), (0.0, 0.0, heading + 90.0))
    white_col(obj)
    return obj


def grave_scene(grip, heading, top, view, fresh, seed):
    """Hob on a headboard's top edge in the churchyard: the board under him, a few graves behind, straw ground and
    the hazy ridges. Returns the perch top (creature cm)."""
    thick, width = 0.045, 0.5
    headboard((grip.x, grip.y), heading, seed, top, fresh=fresh, broken=not fresh)
    rng = random.Random(seed)
    v = Vector((view.x, view.y, 0.0)).normalized()
    side = Vector((v.y, -v.x, 0.0))
    for k in range(7):
        c = Vector((grip.x, grip.y, 0.0)) + v * rng.uniform(1.6, 6.5) + side * rng.uniform(-2.4, 2.4)
        headboard((c.x, c.y), heading + rng.uniform(-15, 15), seed + 50 + k, rng.uniform(0.62, 0.95),
                  fresh=rng.random() < 0.35, broken=rng.random() < 0.3, width=rng.uniform(0.38, 0.52),
                  lean=rng.uniform(-6.0, 9.0))
    straw_ground()
    far_ridges(view)
    half = (width * 0.5 - 0.075) * 100.0
    return FlatTop(-thick * 50.0, thick * 50.0, -half, half, r=0.5)


def shot_hero(key):
    clear_scene()
    spec = spec_for(key)
    h = spec['hero']
    view = Vector(h['camera']) * -1.0
    if h['perch'] == 'post':
        top = 1.2
        grip = Vector((0.0, 0.0, top))
        perch = post_scene(grip, h['heading'], top, view, seed=spec['seed'])
    else:
        top = 0.86
        grip = Vector((0.0, 0.0, top))
        perch = grave_scene(grip, h['heading'], top, view, h['perch'] == 'headboard_fresh', spec['seed'])
    pose = dict(perch=perch, **h['pose'])
    Crow(spec, pose, key).build().finish(grip, h['heading'])
    sun_dir = Vector(h['sun']).normalized()
    golden_world(Vector(h.get('glow', (-0.8, 0.6, 0.06))), strength=0.7)
    add_sun(sun_dir, energy=5.0, color=(1.0, 0.72, 0.47), angle=1.0)
    rim = bpy.data.objects.new('Rim', bpy.data.lights.new('Rim', 'AREA'))
    rim.data.energy = 22.0
    rim.data.size = 0.8
    rim.data.color = (1.0, 0.85, 0.65)
    rim.location = Vector(h['rim'])
    rim.rotation_euler = (grip + Vector((0, 0, 0.18)) - rim.location).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(rim)
    target = grip + Vector(h.get('target', (0.0, 0.0, 0.15)))
    add_camera(target + Vector(h['camera']), target, lens=h.get('lens', 85.0), fstop=h.get('fstop', 5.6))
    res = (1600, 1000) if not QUICK else (800, 500)
    render_setup(res, 96 if not QUICK else 32)
    render(os.path.join(OUT_DIR, f'Hob_{key}_Hero.png' if not QUICK else os.path.join('work', f'Hob_{key}_Hero.png')))


HERO = dict(heading=-150.0, pose=dict(head_yaw=25.0, head_pitch=4.0, head_roll=-8.0), sun=(-0.45, -0.85, 0.35),
            rim=(0.75, 0.95, 1.55), camera=(0.22, -1.3, 0.05), lens=85.0, fstop=5.6)


def report():
    """Builds each concept perched, without rendering, and logs its triangles part by part (the concept's detail;
    the game mesh is far lighter, see the docstring)."""
    for key in CONCEPTS:
        clear_scene()
        spec = spec_for(key)
        crow = Crow(spec, dict(perch=FlatTop(-6.5, 6.5, -6.5, 6.5)), key).build()
        crow.finish((0.0, 0.0, 0.0))
        parts = ', '.join(f'{o.name[len("Hob") + len(key):]} {sum(len(p.vertices) - 2 for p in o.data.polygons)}'
                          for o in crow.objects if o.type == 'MESH')
        log(f'{key} {spec["name"]}: {crow.tris} triangles before solidify ({parts})')


SHOTS = ('hero', 'turn', 'wings', 'scale')
KEYS = [a for a in ARGV if a in CONCEPTS] or list(CONCEPTS)
if '--preview' not in ARGV and not lt.want_preview():
    report()
else:
    shots = [a for a in ARGV if a in SHOTS + ('clay', 'head', 'wingtop', 'compare')] or list(SHOTS) + ['compare']
    for key in KEYS:
        for name, shot in (('clay', shot_clay), ('head', shot_head), ('hero', shot_hero), ('turn', shot_turn),
                           ('wings', shot_wings), ('scale', shot_scale)):
            if name in shots:
                shot(key)
        if 'wingtop' in shots:
            shot_wings(key, 'top')
    if 'compare' in shots:
        shot_compare()
