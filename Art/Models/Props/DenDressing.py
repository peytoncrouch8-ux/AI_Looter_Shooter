"""Dressing for the Gravemother's den under Den Rock (Docs/Areas/RansomsRest.md: the Sink, Side 3 "The Gravemother":
"Something big lives in the Sink's den, and it eats what the Unpaid leave behind"). A spider's larder told without
gore: old bones bleached by sun and dust, a steer skull, a half-sunk ribcage, silk-wrapped prey, a coffin dragged in
in pieces, a dead man's hat and boot. A scripted model file (Art/README.md): small instanced props, no Nanite, LODs at
50% and 25%, baked vertex occlusion. Pivots on the ground in the middle of the footprint (the hung cocoon's at its
hanging point); fronts face -Y.

Three materials for the whole kit (Main's limit): BoneDry (new: Polymer, its soft mottle tinted bone cream, matte,
its occlusion darkening it strongly so sockets and crevices read in the dim den), Web (the Sink's, exactly as Sink.py
makes it: the cocoons and the larder's bundles are its opaque wound-silk cell, their fuzz and threads its fuzz and
strand cells, so no EggSilk) and HouseTrim (the graves kit's coffin wood, the horn sheaths, the hoof, hat and boot on
its dark beam strip, the hat band on its red trim, iron on its strap strip). Silk is grimed in its vertex occlusion
(darker toward the floor and in patches) after the bake: Web is shared, so that is the only dirt it can carry. The
masters darken the colour by DiffuseAO of it (Web's 0.6: at most to 40%) and the sky light by all of it.

  Bones_PileA     a low heap of old bones 1.3 m along its wall side (+Y), 0.3 m high: steer femur, tibias, humerus,
                  cannon bones, ribs, a shoulder blade, vertebrae, a man's arm bone and his thigh bone stuck up out
                  of the front, splinters; the bottom bones half buried (the floor cuts them). For wall feet and the
                  foot of a rock: its back (+Y) to the wall. About 1,500 triangles.
  Bones_PileB     the larder, 2.7 x 1.6 m (2.2 x 1.3 m of bundles), 0.6 m high: a heap of wrapped prey, a calf
                  curled on its side, a man curled up and leaning on it, a dog-sized bundle against its end and a
                  small one on top, bound with strands draped over them and threads out to the floor; a calf's
                  hoofed leg poking out of the silk, a femur standing up between the bundles, a steer's jaw, ribs,
                  a tibia, a man's thigh bone, vertebrae and splinters round the foot. The silk old and grey with
                  dust (its vertex occlusion averages 0.24). About 3,000 triangles, three slots, one low hull round
                  the bundles. Its front (-Y) faces the den's middle; in the pocket's side, off her body.
  Bones_Scatter   a thin scatter of loose bones 1.8 x 1.1 m, flat and half buried (about 670 triangles): fills the
                  gaps between the other pieces, as many times as wanted.
  CattleSkull     a weathered longhorn steer skull, 0.67 m long, 1.4 m across the horns, resting on its back teeth and
                  the back of the skull, nose toward the front (-Y), the horns sweeping out and up in a wide U; dark
                  horn sheaths (one snapped short) on the trim sheet's beam strip, deep dark sockets. Reads from 15 m.
                  One hull round the skull (none on the horns).
  Ribcage         a steer's ribcage half sunk in the floor, 1.3 m long: the slumped spine a ridge 0.4 m up with short
                  spines, ribs leaving it sideways and arching out and down into the dirt (the floor cuts the barrel
                  at its widest), a little out of line, the -Y side splayed lower; ribs gone, snapped, one dropped
                  inward, two fallen beside it. Two hulls (its two halves, down to the floor).
  Cocoon_Lying    a man-sized bundle of wound silk, 1.9 m (2.2 m with its threads), lying along +X (head end +X),
                  rolled a little onto its side, the body faint under the taut silk (head, shoulders, folded arms,
                  legs together, feet), threads wound round it, a fuzz of loose silk, threads gluing it to the
                  floor; grimy low down. One low hull.
  Cocoon_Hung     a calf-sized bundle, 1.3 m, curled under the silk, hanging from its knot on a twisted silk rope
                  and three strands: the pivot (and SOCKET_Top) is the hanging point, where they glue to the rock;
                  its knot hangs 0.6 m below it and its bottom 1.9 m. Hang it from the den's brow, ceiling or a
                  rock edge (the pivot a few cm into the rock); over her way only where its bottom clears her 3.6 m
                  (the notch at the top of the arch). It sways a little as one piece. No collision.
  CoffinBoards    a coffin dragged in in pieces, 2.8 x 0.9 m: its lid broken in two (the head half flat, the foot
                  half fallen across two side boards), the side boards splintered, one still carrying its iron
                  handle, the end board fallen across the lid, nails, splinters. The graves kit's coffin wood
                  (Graves.py's outline and grain). Hulls round the lid's head half and the rest.
  Relic_Hat       a dead man's hat, 0.46 m, lying brim-down, the brim curled, the crown pinched, a faded red band.
  Relic_Boot      a cowboy boot lying on its side, 0.4 m, a spur strapped on its heel.
Collision is only on what blocks: the skull, the ribcage, the coffin pieces, the lying cocoon and the larder's
bundles; the bone piles and scatters, the hung cocoon and the relics have none.

Running it writes the staged placement (PLACEMENT below, in SOCKET_DenMouth space) beside this file, to
DenDressing.placement.json, which the level build reads (the way guns read <Gun>.parts.csv).

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\DenDressing.py -Preview
"""
import json
import math
import os
import random
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector, noise
from mathutils.bvhtree import BVHTree

import looter_buildings as kit
import looter_model as lm
import looter_props as lp
import looter_textures as lt
import looter_webs as lw

lw.register()
# The Sink's web material, exactly as Sink.py makes it (one shared instance, MI_Web).
WEB = lt.material('Webs', name='Web', master='WorldFoliage', WindStrength=4.0, WindSpeed=1.1, DiffuseAO=0.6)
TRIM = lp.trim_material()
# Bone: Polymer's soft mottle tinted a sun-bleached cream (lighter than the den's rock and close to the silk, so it
# reads in the dark), matte (RoughnessOffset: dry bone has no sheen), and its baked occlusion darkening it more than
# the master's default (DiffuseAO), so eye sockets and the gaps in a heap stay dark under the sky light.
BONE = lt.material('Polymer', name='BoneDry', tint=0xe6d9bf, RoughnessOffset=0.3, DiffuseAO=0.85)
BONE_DENSITY = 1536.0                 # px per metre: Polymer's mottle at bone scale (stains a few cm across)
UP = Vector((0.0, 0.0, 1.0))
FRONT = Vector((0.0, -1.0, 0.0))
PREVIEW = 'RansomsRest/DenDressing'
ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []


# --- Mesh helpers ---

def tube(points, radii, sides=7, squash=None, up=None, caps=(0.6, 0.6), uv=None):
    """A closed tube along a polyline: a ring of `sides` vertices at each point (radius radii[i]; an ellipse when
    squash[i] = (sa, sb) scales its two axes: sa along `up` as carried along the path, sb across), its ends closed by a
    pole vertex pushed past the end ring by caps[end] times its radius (a rounded end). uv(s, f) -> (u, v), from the
    arc length s (m) and the fraction f round the ring, maps it (a seam where f wraps)."""
    pts = [Vector(p) for p in points]
    n = len(pts)
    tan = [(pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized() for i in range(n)]
    ref = Vector(up) if up is not None else (UP if abs(tan[0].z) < 0.9 else Vector((1.0, 0.0, 0.0)))
    a = (ref - tan[0] * ref.dot(tan[0])).normalized()
    frames = []
    for i in range(n):
        if i:
            a = (a - tan[i] * a.dot(tan[i])).normalized()
        frames.append((a.copy(), tan[i].cross(a).normalized()))
    s = [0.0]
    for p, q in zip(pts, pts[1:]):
        s.append(s[-1] + (q - p).length)
    bm = lp.new_bmesh()
    layer = bm.loops.layers.uv.active
    rings = []
    for i, p in enumerate(pts):
        ax, bx = frames[i]
        sa, sb = squash[i] if squash is not None else (1.0, 1.0)
        rings.append([bm.verts.new(p + (ax * math.cos(2.0 * math.pi * k / sides) * sa +
                                        bx * math.sin(2.0 * math.pi * k / sides) * sb) * radii[i])
                      for k in range(sides)])
    faces = []
    for i in range(n - 1):
        for k in range(sides):
            k1 = (k + 1) % sides
            f = bm.faces.new((rings[i][k], rings[i][k1], rings[i + 1][k1], rings[i + 1][k]))
            faces.append((f, [(s[i], k / sides), (s[i], (k + 1) / sides), (s[i + 1], (k + 1) / sides),
                              (s[i + 1], k / sides)]))
    for end, (i, sign) in enumerate(((0, -1.0), (n - 1, 1.0))):
        sa, sb = squash[i] if squash is not None else (1.0, 1.0)
        reach = radii[i] * 0.5 * (sa + sb) * caps[end]          # the end ring's own size, squashed or not
        pole = bm.verts.new(pts[i] + tan[i] * sign * reach)
        sp = s[i] + sign * reach
        for k in range(sides):
            k1 = (k + 1) % sides
            if sign > 0:
                f = bm.faces.new((rings[i][k], rings[i][k1], pole))
                faces.append((f, [(s[i], k / sides), (s[i], (k + 1) / sides), (sp, (k + 0.5) / sides)]))
            else:
                f = bm.faces.new((rings[i][k1], rings[i][k], pole))
                faces.append((f, [(s[i], (k + 1) / sides), (s[i], k / sides), (sp, (k + 0.5) / sides)]))
    if uv is not None:
        for f, coords in faces:
            for loop, c in zip(f.loops, coords):
                loop[layer].uv = uv(*c)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return lp.mesh_object(bm)


def ball(center, radii, rotation=(0.0, 0.0, 0.0), segs=6, rings=4):
    """A low ellipsoid (a knob, a condyle, a bone's head): poles, rings, faces in a fixed order (bmesh's own UV sphere
    came out in another face order from one export to the next)."""
    bm = lp.new_bmesh()
    rx, ry, rz = radii
    top, bottom = bm.verts.new((0.0, 0.0, rz)), bm.verts.new((0.0, 0.0, -rz))
    bands = []
    for j in range(1, rings):
        phi = math.pi * j / rings
        bands.append([bm.verts.new((math.sin(phi) * math.cos(2.0 * math.pi * k / segs) * rx,
                                    math.sin(phi) * math.sin(2.0 * math.pi * k / segs) * ry, math.cos(phi) * rz))
                      for k in range(segs)])
    for k in range(segs):
        k1 = (k + 1) % segs
        bm.faces.new((top, bands[0][k], bands[0][k1]))
        for r0, r1 in zip(bands, bands[1:]):
            bm.faces.new((r0[k], r1[k], r1[k1], r0[k1]))
        bm.faces.new((bottom, bands[-1][k1], bands[-1][k]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    obj = lp.mesh_object(bm)
    return lp.place(obj, center, rotation)


def ear_clip(points):
    """Triangles (index triples) filling a simple polygon of 2D points, by clipping ears in order: the same on every
    run (Blender's tessellate_polygon breaks ties by memory order, so the shoulder blade's fill came out in another
    face order from one export to the next)."""
    pts = [Vector((p[0], p[1])) for p in points]
    area = sum(pts[i].x * pts[(i + 1) % len(pts)].y - pts[(i + 1) % len(pts)].x * pts[i].y for i in range(len(pts)))
    left = list(range(len(pts))) if area > 0.0 else list(reversed(range(len(pts))))

    def cross(o, a, b):
        return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x)
    out = []
    while len(left) > 3:
        for k in range(len(left)):
            i0, i1, i2 = left[k - 1], left[k], left[(k + 1) % len(left)]
            a, b, c = pts[i0], pts[i1], pts[i2]
            if cross(a, b, c) <= 1e-12:
                continue
            if any(cross(a, b, pts[j]) >= 0.0 and cross(b, c, pts[j]) >= 0.0 and cross(c, a, pts[j]) >= 0.0
                   for j in left if j not in (i0, i1, i2)):
                continue
            out.append((i0, i1, i2))
            left.pop(k)
            break
        else:                       # no clean ear (a degenerate outline): fan the rest
            out += [(left[0], left[m], left[m + 1]) for m in range(1, len(left) - 1)]
            left = []
    if len(left) == 3:
        out.append(tuple(left))
    return out


def prism(outline, depth):
    """A flat piece: an outline of (x, z) points, its front at y = 0 and depth thick toward +Y. Like looter_buildings'
    _prism without its tidying of the fill (whose result came out in a different face order from run to run): the
    outline's own triangulation (ear_clip), faces in a fixed order."""
    bm = lp.new_bmesh()
    triangles = ear_clip(outline)
    front = [bm.verts.new((x, 0.0, z)) for x, z in outline]
    back = [bm.verts.new((x, depth, z)) for x, z in outline]
    for a, b, c in triangles:
        for face in ((front[a], front[b], front[c]), (back[c], back[b], back[a])):
            try:
                bm.faces.new(face)
            except ValueError:
                pass
    for i in range(len(outline)):
        j = (i + 1) % len(outline)
        bm.faces.new((front[i], front[j], back[j], back[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return lp.mesh_object(bm)


def flat(outline, thick, y=0.0):
    """A flat piece (the graves kit's): outline (x, z) counterclockwise seen from the front, thick centred on y."""
    obj = prism(outline, thick)
    obj.data.transform(Matrix.Translation((0.0, y - thick * 0.5, 0.0)))
    return obj


def extrude(points, z0, z1):
    """A plan outline (x, y) extruded from z0 up to z1 (the graves kit's)."""
    obj = prism([(x, -y) for x, y in points], z1 - z0)
    obj.data.transform(Matrix.Translation((0.0, 0.0, z0)) @ Matrix.Rotation(math.radians(90.0), 4, 'X'))
    return obj


def octahedron(size, center, rotation):
    """A small eight-sided lump: a chip of bone."""
    bm = lp.new_bmesh()
    sx, sy, sz = (v * 0.5 for v in size)
    v = [bm.verts.new(p) for p in ((sx, 0.0, 0.0), (0.0, sy, 0.0), (-sx, 0.0, 0.0), (0.0, -sy, 0.0), (0.0, 0.0, sz),
                                   (0.0, 0.0, -sz))]
    for k in range(4):
        bm.faces.new((v[k], v[(k + 1) % 4], v[4]))
        bm.faces.new((v[(k + 1) % 4], v[k], v[5]))
    return lp.place(lp.mesh_object(bm), center, rotation)


def transform(parts, matrix):
    for p in parts:
        p.data.transform(matrix)
        p.data.update()
    return parts


def points_of(parts):
    return [v.co.copy() for p in parts for v in p.data.vertices]


def settle(parts, under, ground, embed=0.0, reach=3.0):
    """Drops parts (one rigid piece) straight down until they rest on the ground plane z = ground or on the meshes
    under them, then a further embed (m) into what they landed on."""
    verts, polys = [], []
    for o in under:
        base = len(verts)
        verts.extend(v.co.copy() for v in o.data.vertices)
        polys.extend([base + i for i in p.vertices] for p in o.data.polygons)
    base = len(verts)
    verts.extend(Vector(c) for c in ((-50.0, -50.0, ground), (50.0, -50.0, ground), (50.0, 50.0, ground),
                                     (-50.0, 50.0, ground)))
    polys.append([base, base + 1, base + 2, base + 3])
    tree = BVHTree.FromPolygons(verts, polys)
    drop = reach
    for co in points_of(parts):
        hit = tree.ray_cast(co + Vector((0.0, 0.0, 1e-4)), Vector((0.0, 0.0, -1.0)), reach)
        if hit[0] is not None:
            drop = min(drop, hit[3] - 1e-4)
    return transform(parts, Matrix.Translation((0.0, 0.0, -(drop + embed))))


def lowest(parts):
    return min(c.z for c in points_of(parts))


def bone_uv(obj, seed):
    lt.assign(obj, BONE)
    lt.box_uv(obj, 'Polymer', texel_density=BONE_DENSITY, seed=seed)
    return obj


def named(obj, name):
    obj.name = name
    obj.data.name = name
    return obj


def report(obj):
    tris = lp.tri_count(obj)
    co = np.array([obj.matrix_world @ Vector(c) for c in obj.bound_box])
    size = co.max(axis=0) - co.min(axis=0)
    sockets = [c.name.split('.')[0] for c in obj.children if c.name.startswith('SOCKET_')]
    hulls = len([c for c in obj.children if c.name.startswith('UCX_')])
    print(f'DEN: {obj.name}: {tris} triangles, {size[0]:.2f} x {size[1]:.2f} x {size[2]:.2f} m, materials '
          f'{", ".join(m.name for m in obj.data.materials)}, {hulls} hulls, sockets {", ".join(sockets) or "-"}, '
          f'collision {obj.get("Collision", "hulls" if hulls else "mesh")}, LODs {obj.get("LODs")}', flush=True)


def finish(obj, ao=0.3, smooth=50.0, collision=None, ground=True, samples=32):
    """Shading, baked occlusion and the small-prop export settings."""
    lm.smooth(obj, smooth)
    lt.bake_vertex_ao(obj, samples=samples, distance=ao, ground=ground)
    obj['Nanite'] = 0
    obj['LODs'] = '50,25'
    if collision:
        obj['Collision'] = collision
    return obj


# --- Bones ---

# Long bones as radius profiles along their length (t, radius / shaft radius), their bow (a share of the length),
# knobs (t, sideways and up in shaft radii, radius in shaft radii: a femur's head, condyles) and how flat the shaft
# is (its height over its width in the middle).
BONE_SHAPES = {
    'femur': ([(0.0, 0.8), (0.04, 1.5), (0.18, 1.15), (0.5, 0.92), (0.8, 1.2), (0.92, 1.6), (1.0, 1.0)], 0.03,
              [(0.07, 1.0, 0.55, 1.05), (0.95, 0.6, -0.15, 0.95), (0.95, -0.6, -0.15, 0.95)], 0.85),
    'humerus': ([(0.0, 0.9), (0.05, 1.75), (0.22, 1.15), (0.5, 0.94), (0.8, 1.2), (0.93, 1.5), (1.0, 1.0)], 0.05,
                [(0.06, 0.7, 0.6, 1.1)], 0.9),
    'tibia': ([(0.0, 1.0), (0.04, 1.85), (0.2, 1.15), (0.55, 0.85), (0.85, 1.0), (0.95, 1.35), (1.0, 1.0)], 0.02,
              [(0.04, 0.0, 0.9, 0.9)], 0.75),
    'cannon': ([(0.0, 1.0), (0.06, 1.35), (0.5, 0.95), (0.9, 1.25), (1.0, 1.0)], 0.01,
               [(0.97, 0.55, 0.0, 0.85), (0.97, -0.55, 0.0, 0.85)], 0.7),
    'man_femur': ([(0.0, 0.9), (0.06, 1.8), (0.25, 1.0), (0.7, 0.95), (0.92, 1.8), (1.0, 1.5)], 0.035,
                  [(0.03, 2.1, 1.0, 1.65)], 0.95),
    'man_arm': ([(0.0, 1.0), (0.05, 2.0), (0.3, 1.0), (0.8, 1.0), (0.94, 1.4), (1.0, 1.3)], 0.02,
                [(0.03, 0.4, 0.6, 1.7)], 0.95),
}


def long_bone(kind, length, rs, seed, sides=6):
    """A long bone along X, centred on the origin, lying: a tube through its profile, slightly bowed, with its knobs."""
    rnd = random.Random(seed)
    profile, bow, knobs, flatness = BONE_SHAPES[kind]
    bend = bow * rnd.uniform(0.7, 1.3) * rnd.choice((-1.0, 1.0))

    def axis(t):
        return Vector(((t - 0.5) * length, bend * length * math.sin(math.pi * t), 0.0))
    pts, radii, squash = [], [], []
    for t, f in profile:
        pts.append(axis(t))
        radii.append(rs * f * rnd.uniform(0.94, 1.06))
        mid = math.sin(math.pi * t) ** 0.6
        squash.append((1.0 - (1.0 - flatness) * mid, 1.0))
    parts = [tube(pts, radii, sides, squash=squash, up=UP, caps=(0.5, 0.5))]
    for t, side, lift, f in knobs:
        r = rs * f
        parts.append(ball(axis(t) + Vector((0.0, side * rs, lift * rs)), (r, r * 0.95, r * 0.9),
                          (rnd.uniform(0, 40), 0.0, rnd.uniform(0, 90)), segs=6, rings=3))
    return parts


def rib_bone(length, width, thick, seed, arc=1.5, steps=5, sides=5, rise=True):
    """A loose rib along X, arching up (lying on its broad face, both ends down), tapering toward its tip."""
    rnd = random.Random(seed)
    angle = arc * rnd.uniform(0.85, 1.15)
    radius = length / angle
    pts, radii, squash = [], [], []
    for i in range(steps + 1):
        f = i / steps
        a = -angle * 0.5 + angle * f
        z = radius * (math.cos(a) - math.cos(angle * 0.5)) if rise else 0.0
        y = 0.0 if rise else radius * (math.cos(a) - math.cos(angle * 0.5))
        pts.append((radius * math.sin(a), y, z))
        taper = 1.0 - 0.3 * f ** 1.5
        radii.append(1.0)
        squash.append((width * 0.5 * taper, thick * 0.5 * (1.25 if i == 0 else 1.0)))
    return [tube(pts, radii, sides, squash=squash, up=Vector((0.0, 1.0, 0.0)) if rise else UP, caps=(0.9, 0.9))]


def vertebra(seed, size=1.0, spine=0.12, wings=0.0, lean=0.03):
    """A vertebra along X (the spine's axis): its body, the spinous process standing up (leaning back by lean) and,
    for a loin vertebra, two flat transverse processes out to the sides (wings)."""
    rnd = random.Random(seed)
    s = size
    body = tube([(-0.03 * s, 0.0, 0.0), (0.0, 0.0, 0.0), (0.03 * s, 0.0, 0.0)],
                [0.036 * s, 0.031 * s, 0.036 * s], 7, up=UP, caps=(0.12, 0.12))
    parts = [body]
    h = spine * s * rnd.uniform(0.85, 1.1)
    blade = flat([(-0.026 * s, 0.02 * s), (0.026 * s, 0.02 * s), (0.016 * s + lean * s, 0.02 * s + h),
                  (-0.002 * s + lean * s, 0.02 * s + h * 1.04), (-0.016 * s + lean * s * 0.5, 0.02 * s + h * 0.55)],
                 0.013 * s)
    parts.append(blade)
    parts.append(lp.block((0.05 * s, 0.05 * s, 0.026 * s), (0.0, 0.0, 0.03 * s)))       # the arch
    if wings:
        for side in (-1.0, 1.0):
            wing = extrude([(-0.018 * s, 0.0), (0.018 * s, 0.0), (0.03 * s, side * wings * s),
                            (0.002 * s, side * wings * s * 1.04)] if side > 0 else
                           [(0.018 * s, 0.0), (-0.018 * s, 0.0), (0.002 * s, side * wings * s * 1.04),
                            (0.03 * s, side * wings * s)], -0.006 * s, 0.006 * s)
            parts.append(lp.place(wing, (0.0, side * 0.02 * s, 0.006 * s), (side * 8.0, 0.0, 0.0)))
    return parts


def scapula(seed, size=0.33):
    """A shoulder blade lying flat: a dished triangular plate, its ridge (the spine) and the socket's knob."""
    rnd = random.Random(seed)
    s = size / 0.33
    plate = extrude([(0.0, -0.03 * s), (0.08 * s, -0.07 * s), (0.31 * s, -0.12 * s), (0.34 * s, -0.02 * s),
                     (0.31 * s, 0.09 * s), (0.1 * s, 0.06 * s), (0.0, 0.03 * s)], 0.0, 0.011 * s)
    for v in plate.data.vertices:
        v.co.z += 0.5 * (v.co.y / s) ** 2 * s + rnd.uniform(-0.003, 0.003)
    ridge = extrude([(0.03 * s, -0.008 * s), (0.3 * s, -0.012 * s), (0.3 * s, -0.002 * s), (0.04 * s, 0.006 * s)],
                    0.008 * s, 0.03 * s)
    knob = ball((-0.012 * s, 0.0, 0.016 * s), (0.032 * s, 0.03 * s, 0.024 * s))
    neck = tube([(-0.01 * s, 0.0, 0.012 * s), (0.06 * s, 0.0, 0.01 * s)], [0.022 * s, 0.018 * s], 6, caps=(0.2, 0.2))
    return [plate, ridge, knob, neck]


def mandible(seed, size=0.4):
    """A steer's lower jaw (half of it) lying on its side: the long body with its molar row, the ramus rising at the
    back to the condyle."""
    s = size / 0.4
    outline = [(-0.2, 0.0), (-0.05, -0.012), (0.12, -0.014), (0.18, 0.006), (0.205, 0.07), (0.205, 0.15),
               (0.185, 0.178), (0.165, 0.15), (0.15, 0.105), (0.12, 0.072), (-0.06, 0.062), (-0.13, 0.034),
               (-0.2, 0.026)]
    body = flat([(x * s, z * s) for x, z in outline], 0.02 * s)
    parts = [body]
    for k in range(5):
        x = (-0.055 + 0.034 * k) * s
        parts.append(lp.block((0.028 * s, 0.022 * s, 0.022 * s), (x, 0.0, (0.07 + 0.002 * (k % 2)) * s)))
    transform(parts, Matrix.Rotation(math.radians(90.0), 4, 'X'))       # onto its side: it lies flat
    return parts


def splinter(length, width, seed):
    """A broken piece of a long bone's shaft: a short tube, its broken ends ragged (one longer tongue)."""
    rnd = random.Random(seed)
    pts = [(-length * 0.5, 0.0, 0.0), (0.0, rnd.uniform(-0.1, 0.1) * width, 0.0), (length * 0.5, 0.0, 0.0)]
    obj = tube(pts, [width * 0.42, width * 0.5, width * 0.4], 5, squash=[(0.85, 1.0)] * 3, up=UP, caps=(0.15, 0.15))
    for v in obj.data.vertices:
        if abs(v.co.x) > length * 0.3:
            side = 1.0 if v.co.x > 0 else -1.0
            v.co.x += side * rnd.uniform(-0.25, 0.35) * width * (1.0 if v.co.y > 0 else 0.4)
    obj.data.update()
    return [obj]


def chip(size, seed):
    rnd = random.Random(seed)
    return [octahedron((size, size * rnd.uniform(0.5, 0.8), size * rnd.uniform(0.35, 0.55)), (0.0, 0.0, 0.0),
                       (0.0, 0.0, rnd.uniform(0.0, 180.0)))]


def bone_parts(spec):
    kind = spec['kind']
    seed = spec['seed']
    if kind in BONE_SHAPES:
        return long_bone(kind, spec['length'], spec['rs'], seed, spec.get('sides', 6))
    if kind == 'rib':
        return rib_bone(spec['length'], spec.get('width', 0.045), spec.get('thick', 0.014), seed,
                        spec.get('arc', 1.5), rise=spec.get('rise', True))
    if kind == 'vertebra':
        return vertebra(seed, spec.get('size', 1.0), spec.get('spine', 0.12), spec.get('wings', 0.0))
    if kind == 'scapula':
        return scapula(seed, spec.get('size', 0.33))
    if kind == 'mandible':
        return mandible(seed, spec.get('size', 0.4))
    if kind == 'splinter':
        return splinter(spec['length'], spec.get('width', 0.02), seed)
    if kind == 'chip':
        return chip(spec['size'], seed)
    raise ValueError(kind)


def lay_bones(items, under=(), embed=0.004):
    """Bones laid one by one: each item is built, turned (roll about its length, pitch, then yaw), moved over its spot
    and dropped onto the ground plane at its sink depth (negative: buried), onto the bones already down or onto the
    meshes under (bundles), sinking embed (or its own) into what it lands on. Returns the parts."""
    placed = []
    under = list(under)
    for spec in items:
        parts = bone_parts(spec)
        roll, pitch, yaw = spec.get('roll', 0.0), spec.get('pitch', 0.0), spec.get('yaw', 0.0)
        transform(parts, Matrix.Rotation(math.radians(yaw), 4, 'Z') @ Matrix.Rotation(math.radians(pitch), 4, 'Y') @
                  Matrix.Rotation(math.radians(roll), 4, 'X'))
        transform(parts, Matrix.Translation((spec['x'], spec['y'], 2.0 - lowest(parts))))
        settle(parts, placed + under, spec.get('sink', 0.0), embed=spec.get('embed', embed))
        placed += parts
    return placed


def pile(name, seed, items, ao=0.25):
    """A heap of bones (lay_bones), mapped, merged and finished: no collision."""
    placed = lay_bones(items)
    rnd = random.Random(seed)
    for p in placed:
        bone_uv(p, rnd.randint(0, 9999))
    obj = lp.join(name, placed)
    return finish(obj, ao=ao, smooth=55.0, collision='None')


# Size ranges of the heaps' bones (m): steer bones, and a man's (thinner).
HEAP_KINDS = {
    'femur': dict(length=(0.38, 0.45), rs=(0.024, 0.028)),
    'tibia': dict(length=(0.36, 0.42), rs=(0.022, 0.026)),
    'humerus': dict(length=(0.28, 0.33), rs=(0.025, 0.029)),
    'cannon': dict(length=(0.2, 0.25), rs=(0.018, 0.021)),
    'man_femur': dict(length=(0.43, 0.47), rs=(0.013, 0.015)),
    'man_arm': dict(length=(0.3, 0.34), rs=(0.011, 0.013)),
    'rib': dict(length=(0.3, 0.44), width=(0.03, 0.038), thick=(0.016, 0.02)),
    'splinter': dict(length=(0.07, 0.14), width=(0.026, 0.038)),
    'chip': dict(size=(0.03, 0.055)),
}
LONG = ('femur', 'tibia', 'humerus', 'cannon', 'man_femur', 'man_arm')


def heap(name, seed, recipe, half=(0.7, 0.35), back=0.1, align=25.0, extra=(), ao=0.25):
    """A heap of bones from a recipe [(kind, count, fixed settings)], laid in its order (big ones first): each at a
    spot drawn round the heap's middle (pulled toward its back, +Y, by back; splinters and chips toward its edge),
    turned mostly along X (within align degrees; a quarter of them anyhow), a third tipped so one end is in the dirt,
    dropped onto the dirt (the first bones sunk by half their thickness: buried) or onto the bones already down.
    extra: whole item settings laid after them (a bone standing out of the top, a run of vertebrae)."""
    rnd = random.Random(seed)
    items = []
    count = sum(c for _, c, _ in recipe)
    for kind, n, fixed in recipe:
        for _ in range(n):
            spec = dict(kind=kind, seed=rnd.randint(0, 99999))
            for key, (a, b) in HEAP_KINDS.get(kind, {}).items():
                spec[key] = rnd.uniform(a, b)
            small = kind in ('splinter', 'chip')
            r = rnd.uniform(0.5, 0.95) if small else min(1.0, abs(rnd.gauss(0.0, 0.45)))
            a = rnd.uniform(0.0, 2.0 * math.pi)
            spec['x'] = math.cos(a) * r * half[0]
            spec['y'] = math.sin(a) * r * half[1] + back * (1.0 - r)
            spec['yaw'] = (rnd.uniform(-align, align) + rnd.choice((0.0, 180.0)) if rnd.random() < 0.75 else
                           rnd.uniform(0.0, 360.0))
            if kind in LONG:
                spec['roll'] = rnd.uniform(0.0, 360.0)
                spec['pitch'] = rnd.uniform(-14.0, 14.0) if rnd.random() < 0.33 else rnd.uniform(-4.0, 4.0)
            if kind == 'rib':
                spec['rise'] = rnd.random() < 0.6
            first = len(items) < count * 0.35
            thick = 2.0 * spec.get('rs', spec.get('thick', 0.01) * 0.5)
            spec['sink'] = -(0.5 * thick if first else 0.006) - (0.04 if abs(spec.get('pitch', 0.0)) > 8.0 else 0.0)
            spec.update(fixed)
            items.append(spec)
    small = [i for i in items if i['kind'] in ('splinter', 'chip')]
    items = [i for i in items if i not in small] + list(extra) + small
    return pile(name, seed, items, ao=ao)


def bones_pile_a(name='Bones_PileA', seed=11):
    """Along a wall foot, 1.3 m: long bones drifted along it (X), piled higher toward the wall (+Y), ribs among them,
    a shoulder blade, vertebrae, a man's thigh bone stuck up out of the front of it, splinters spilling out."""
    recipe = [('femur', 1, {}), ('tibia', 1, {}), ('humerus', 1, {}), ('rib', 2, {}), ('cannon', 2, {}),
              ('tibia', 1, {}), ('rib', 1, {}), ('man_arm', 1, {}), ('splinter', 6, {}), ('chip', 4, {})]
    extra = [dict(kind='scapula', size=0.34, seed=105, x=0.5, y=0.16, yaw=165.0, sink=-0.004),
             dict(kind='vertebra', spine=0.13, seed=106, x=-0.55, y=0.02, yaw=35.0, roll=75.0, sink=-0.02),
             dict(kind='vertebra', spine=0.07, wings=0.09, seed=107, x=0.22, y=-0.18, yaw=-12.0, sink=-0.012),
             dict(kind='man_femur', length=0.46, rs=0.0145, seed=111, x=0.04, y=-0.06, yaw=24.0, pitch=-20.0,
                  sink=-0.08)]
    return heap(name, seed, recipe, half=(0.78, 0.26), back=0.1, align=20.0, extra=extra)


def grime(obj, material, height=0.3, low=0.5, patches=0.3, seed=0, scale=2.2, overall=1.0):
    """Old dust and grime on silk: multiplies the baked occlusion of the faces on material (the masters darken the
    colour by it, DiffuseAO of it: Web is shared, so this is the only dirt it can carry), darker toward the floor
    (below height over the pivot, down to low; height None: no floor), in soft patches (patches: how dark) and all
    over by overall."""
    mesh = obj.data
    index = mesh.materials.find(material.name)
    raw = lt._read_col(mesh)
    off = Vector((seed * 1.7 + 3.1, seed * 2.3 + 5.7, seed * 0.9 + 1.3))
    for poly in mesh.polygons:
        if poly.material_index != index:
            continue
        for li in poly.loop_indices:
            co = mesh.vertices[mesh.loops[li].vertex_index].co
            f = 1.0
            if height:
                t = min(max(co.z / height, 0.0), 1.0)
                f = low + (1.0 - low) * t * t * (3.0 - 2.0 * t)
            n = noise.noise(co * scale + off) + 0.5 * noise.noise(co * scale * 2.7 - off)
            f *= 1.0 - patches * min(1.0, max(0.0, n + 0.15))
            raw[li, 3] *= f * overall
    lt._write_col(mesh, raw)


def bones_scatter(name='Bones_Scatter', seed=13):
    """A thin scatter of loose bones 2 m across, lying flat and half buried: a femur, a tibia, ribs, a cannon bone, a
    vertebra, a splinter or two. Cheap (about 600 triangles): fills the gaps between the other pieces, many times."""
    recipe = [('femur', 1, {}), ('tibia', 1, {}), ('rib', 2, {}), ('cannon', 1, {}), ('splinter', 3, {}),
              ('chip', 2, {})]
    extra = [dict(kind='vertebra', spine=0.1, seed=301, x=0.45, y=0.25, yaw=20.0, roll=80.0, sink=-0.025)]
    rnd = random.Random(seed)
    items = []
    for kind, n, _ in recipe:
        for _ in range(n):
            spec = dict(kind=kind, seed=rnd.randint(0, 99999))
            for key, (a, b) in HEAP_KINDS.get(kind, {}).items():
                spec[key] = rnd.uniform(a, b)
            spec['x'], spec['y'] = rnd.uniform(-0.9, 0.9), rnd.uniform(-0.5, 0.5)
            spec['yaw'] = rnd.uniform(0.0, 360.0)
            if kind in LONG:
                spec['roll'] = rnd.uniform(0.0, 360.0)
            thick = 2.0 * spec.get('rs', spec.get('thick', 0.01) * 0.5)
            spec['sink'] = -0.45 * thick
            items.append(spec)
    return pile(name, seed, items + extra)


# --- The steer skull ---

def remesh(obj, voxel):
    mod = obj.modifiers.new('Remesh', 'REMESH')
    mod.mode = 'VOXEL'
    mod.voxel_size = voxel
    mod.adaptivity = 0.0
    lt._apply_modifiers(obj)


def sculpt(name, parts, cutters, voxel, target, smooth=(0.5, 4)):
    """One organic solid from overlapping closed parts: voxel-remeshed into their union, the cutters carved away,
    remeshed again (softening the cuts), smoothed and reduced to about target triangles."""
    obj = lp.join(name, parts)
    remesh(obj, voxel)
    for cutter in cutters:
        mod = obj.modifiers.new('Cut', 'BOOLEAN')
        mod.operation = 'DIFFERENCE'
        mod.solver = 'EXACT'
        mod.object = cutter
        lt._apply_modifiers(obj)
        bpy.data.objects.remove(cutter, do_unlink=True)
    remesh(obj, voxel)
    mod = obj.modifiers.new('Smooth', 'SMOOTH')
    mod.factor, mod.iterations = smooth
    lt._apply_modifiers(obj)
    mod = obj.modifiers.new('Reduce', 'DECIMATE')
    mod.decimate_type = 'COLLAPSE'
    mod.ratio = min(1.0, target / max(lp.tri_count(obj), 1))
    mod.use_collapse_triangulate = True
    lt._apply_modifiers(obj)
    clean(obj)
    return obj


def clean(obj, area=2e-7):
    """Merges slivers a reduction leaves (faces with almost no area), which would get no tangent in Unreal."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges[:], dist=1e-4)
    thin = [f for f in bm.faces if f.calc_area() < area]
    if thin:
        bmesh.ops.collapse(bm, edges=list({e for f in thin for e in f.edges if e.is_valid}))
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def horn_uv(seed):
    """Horn sheaths on the house trim's beam strip (C): its dark, fibrous wood reads as old horn. U along the horn,
    V once round it inside one 20 cm beam face (so its dark edges meet at the seam, underneath)."""
    rnd = random.Random(seed)
    v_lo, v_hi = lt.TRIM_STRIPS['C']
    scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
    u0 = rnd.uniform(0.0, 6.0)
    face = rnd.randrange(4)

    def uv(s, f):
        return ((u0 + s) * scale, v_lo + (face * 0.2 + 0.01 + 0.18 * f) * scale)
    return uv


def horn(side, seed, snapped=False):
    """A horn growing from the poll's end on one side (side +1 or -1): out sideways, then up and forward, tapering
    to its tip; the pale core shows for a few cm before the dark sheath (a rim where it starts)."""
    rnd = random.Random(seed)
    path = [(0.118, 0.0, 0.072), (0.16, 0.008, 0.08), (0.24, 0.012, 0.095), (0.33, -0.005, 0.125),
            (0.42, -0.04, 0.18), (0.5, -0.085, 0.26), (0.55, -0.12, 0.34), (0.575, -0.145, 0.415)]
    radii = [0.042, 0.04, 0.036, 0.031, 0.025, 0.019, 0.013, 0.006]
    if snapped:
        path, radii = path[:6], radii[:5] + [0.017]
    pts = [Vector((side * x + rnd.uniform(-0.004, 0.004), y, z)) for x, y, z in path]
    core = tube([pts[0] - (pts[1] - pts[0]) * 0.4, pts[0].lerp(pts[1], 0.3)], [0.034, 0.034], 8, caps=(0.2, 0.2))
    bone_uv(core, seed)
    sheath_pts = [pts[0].lerp(pts[1], 0.22)] + pts[1:]
    sheath_r = [radii[0] * 1.06] + radii[1:]
    sheath = tube(sheath_pts, sheath_r, 8, caps=(0.2 if snapped else 0.3, 0.25 if snapped else 0.6),
                  uv=horn_uv(seed))
    lt.assign(sheath, TRIM)
    if snapped:          # a jagged break: the end ring's vertices pulled to uneven lengths
        end = sheath_pts[-1]
        d = (sheath_pts[-1] - sheath_pts[-2]).normalized()
        for v in sheath.data.vertices:
            if (v.co - end).length < 0.03:
                v.co += d * rnd.uniform(-0.012, 0.016)
    return [core, sheath]


SKULL_SCALE = 1.3         # over life size (a longhorn's): the art bible's exaggerated shapes, and it reads from 15 m

# The skull's main mass as cross-sections from the back of the skull to the muzzle: (y, width, height, centre z,
# squareness of its top half, of its bottom half). The forehead broad and flat between the horns, widest at the
# orbits, the face narrowing to the muzzle's flared spatula.
SKULL_LOFT = [(0.035, 0.15, 0.15, -0.012, 2.4, 2.2), (0.01, 0.19, 0.19, 0.0, 3.2, 2.4),
              (-0.06, 0.205, 0.185, 0.0, 3.6, 2.4),
              (-0.13, 0.215, 0.165, -0.005, 3.6, 2.2), (-0.19, 0.2, 0.14, -0.012, 3.2, 2.2),
              (-0.245, 0.15, 0.12, -0.022, 2.8, 2.2), (-0.31, 0.128, 0.1, -0.036, 2.6, 2.4),
              (-0.38, 0.108, 0.083, -0.052, 2.4, 2.4), (-0.44, 0.092, 0.064, -0.064, 2.4, 2.6),
              (-0.49, 0.1, 0.042, -0.074, 2.6, 3.0), (-0.515, 0.08, 0.03, -0.078, 2.2, 2.6)]


def loft(sections, sides=20):
    """A closed solid through cross-sections along -Y: (y, width, height, centre z, top squareness, bottom
    squareness), each a superellipse; the ends closed by poles."""
    bm = lp.new_bmesh()
    rings = []
    for y, w, h, zc, top, bottom in sections:
        ring = []
        for k in range(sides):
            t = 2.0 * math.pi * k / sides
            c, sn = math.cos(t), math.sin(t)
            p = top if sn >= 0.0 else bottom
            x = 0.5 * w * math.copysign(abs(c) ** (2.0 / p), c)
            z = zc + 0.5 * h * math.copysign(abs(sn) ** (2.0 / p), sn)
            ring.append(bm.verts.new((x, y, z)))
        rings.append(ring)
    for r0, r1 in zip(rings, rings[1:]):
        for k in range(sides):
            k1 = (k + 1) % sides
            bm.faces.new((r0[k], r0[k1], r1[k1], r1[k]))
    for ring, (y, w, h, zc, _, _), sign in ((rings[0], sections[0], 1.0), (rings[-1], sections[-1], -1.0)):
        pole = bm.verts.new((0.0, y + sign * 0.01, zc))
        for k in range(sides):
            bm.faces.new((ring[k], ring[(k + 1) % sides], pole))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return lp.mesh_object(bm)


def ring_tube(center, normal, radius, thick, segs=12):
    """A torus round center in the plane facing normal (an orbit's rim)."""
    n = Vector(normal).normalized()
    a = n.cross(UP if abs(n.z) < 0.9 else FRONT).normalized()
    b = n.cross(a)
    pts = [Vector(center) + (a * math.cos(2.0 * math.pi * k / segs) + b * math.sin(2.0 * math.pi * k / segs)) * radius
           for k in range(segs + 2)]
    return tube(pts, [thick] * len(pts), 8, caps=(0.5, 0.5))


def skull_solid(seed):
    """The skull's bone, built in its own frame (x across, nose toward -y, the poll at the top back, z up): the lofted
    mass with the poll's ridge, the orbits' rims, the nasal ridge, cheek arches, the molar rows and the condyles melted
    on, the eye sockets, the nose opening and the hole behind carved out."""
    E = ball
    parts = [
        loft(SKULL_LOFT),
        tube([(-0.112, -0.004, 0.07), (0.0, 0.004, 0.083), (0.112, -0.004, 0.07)], [0.034, 0.03, 0.034], 10,
             caps=(0.7, 0.7)),                                                    # the poll's ridge between the horns
        tube([(0.0, -0.2, 0.045), (0.0, -0.3, 0.014), (0.0, -0.43, -0.036)], [0.02, 0.016, 0.012], 8),     # nasal ridge
        tube([(0.093, -0.19, -0.05), (0.082, -0.09, -0.06)], [0.014, 0.013], 8),                          # cheek arches
        tube([(-0.093, -0.19, -0.05), (-0.082, -0.09, -0.06)], [0.014, 0.013], 8),
        E((0.03, 0.03, -0.075), (0.024, 0.02, 0.022), segs=10, rings=6),            # the condyles
        E((-0.03, 0.03, -0.075), (0.024, 0.02, 0.022), segs=10, rings=6),
        tube([(0.042, -0.21, -0.085), (0.04, -0.36, -0.082)], [0.016, 0.014], 8),   # molar rows under the palate
        tube([(-0.042, -0.21, -0.085), (-0.04, -0.36, -0.082)], [0.016, 0.014], 8),
    ]
    for side in (-1.0, 1.0):
        parts.append(ring_tube((side * 0.106, -0.165, 0.008), (side * 0.85, -0.45, 0.22), 0.04, 0.016))
        parts.append(E((side * 0.064, -0.28, -0.04), (0.02, 0.032, 0.017), segs=10, rings=6))   # facial tubers
    cutters = []
    for side in (-1.0, 1.0):
        cutters.append(E((side * 0.112, -0.168, 0.008), (0.036, 0.034, 0.034), (0.0, 0.0, 0.0), 12, 8))
    cutters.append(E((0.0, -0.5, -0.05), (0.028, 0.06, 0.02), (14.0, 0.0, 0.0), 12, 8))      # the nose opening
    cutters.append(E((0.0, 0.05, -0.055), (0.022, 0.035, 0.022), (0.0, 0.0, 0.0), 12, 8))    # foramen magnum
    for c in parts + cutters:
        c.data.transform(Matrix.Scale(SKULL_SCALE, 4))
    return sculpt('_SkullBone', parts, cutters, 0.004, 1100, smooth=(0.5, 3))


def cattle_skull(name='CattleSkull', seed=31):
    """The steer skull as an old skull lies without its jaw: on its back teeth (or its muzzle) and the back of the
    skull, the nose toward the front (-Y), the horns sweeping out and up; a little roll, sunk 2 cm into the dust."""
    rnd = random.Random(seed)
    bone = skull_solid(seed)
    bone_uv(bone, seed)
    horns = horn(1.0, seed + 1) + horn(-1.0, seed + 2, snapped=True)
    for h in horns:
        h.data.transform(Matrix.Scale(SKULL_SCALE, 4))
    parts = [bone] + horns
    # Find the pitch at which the front of the underside (teeth, muzzle) and the back (the condyles) touch the same
    # ground: how it rests.
    front = [v.co.copy() for v in bone.data.vertices if v.co.y < -0.2 * SKULL_SCALE]
    back = [v.co.copy() for v in bone.data.vertices if v.co.y > -0.06 * SKULL_SCALE]
    best = None
    for k in range(-30, 31):
        m = Matrix.Rotation(math.radians(k), 3, 'X')
        dz = min((m @ c).z for c in front) - min((m @ c).z for c in back)
        if best is None or abs(dz) < abs(best[1]):
            best = (k, dz)
    tip = best[0]
    transform(parts, Matrix.Rotation(math.radians(rnd.uniform(-5.0, 5.0)), 4, 'Y') @
              Matrix.Rotation(math.radians(tip), 4, 'X'))
    lo_, hi_ = np.array(points_of(parts)).min(axis=0), np.array(points_of(parts)).max(axis=0)
    transform(parts, Matrix.Translation((-(lo_[0] + hi_[0]) * 0.5, -(lo_[1] + hi_[1]) * 0.5, -lo_[2] - 0.02)))
    body_pts = [v.co.copy() for v in parts[0].data.vertices]
    obj = lp.join(name, parts)
    lp.hull_points(obj, support(body_pts, 40))
    print(f'DEN: CattleSkull rests pitched {tip} degrees (front and back {best[1]:+.3f} m apart)', flush=True)
    return finish(obj, ao=0.3, smooth=60.0)


def support(points, count=40):
    """The points of a cloud that stick out furthest in count directions spread over the sphere (a hull's corners)."""
    P = np.asarray([tuple(p) for p in points], np.float64)
    k = np.arange(count) + 0.5
    z = 1.0 - 2.0 * k / count
    r = np.sqrt(np.clip(1.0 - z * z, 0.0, 1.0))
    phi = k * math.pi * (3.0 - math.sqrt(5.0))
    dirs = np.stack([r * np.cos(phi), r * np.sin(phi), z], axis=1)
    picks = sorted(set(int(i) for i in np.argmax(P @ dirs.T, axis=0)))
    return [tuple(P[i]) for i in picks]


# --- The ribcage ---

def ribcage(name='Ribcage', seed=41):
    """A steer's ribcage half sunk, along X (the neck end toward -X): its spine a slumped ridge 0.42 m up with short
    spines over the withers; nine pairs of ribs leaving it sideways and arching out and down into the floor, widest
    where they go in (the floor cuts the barrel at its widest), leaning back as ribs do, each a little out of line;
    the -Y side splayed lower (the cage has settled onto it), three ribs gone, four snapped short, one dropped inward,
    two fallen beside it."""
    rnd = random.Random(seed)
    count, x0, x1 = 9, -0.52, 0.5
    hs = 0.42
    xs = [x0 + (x1 - x0) * i / (count - 1) + rnd.uniform(-0.035, 0.035) for i in range(count)]

    def spine_at(x):
        return Vector((x, 0.03 * math.sin(2.2 * x + 0.4), hs + 0.025 * math.cos(2.6 * x) - 0.03 * max(0.0, x) ** 2))
    # The spine: one tube of vertebral bodies, swelling at each and pinched between.
    pts, radii = [], []
    step = xs[1] - xs[0]
    for i in range(count):
        pts.append(spine_at(xs[i]))
        radii.append(0.043)
        if i < count - 1:
            pts.append(spine_at((xs[i] + xs[i + 1]) * 0.5))
            radii.append(0.033)
    pts = [spine_at(xs[0] - step * 0.7)] + pts + [spine_at(xs[-1] + step * 0.7)]
    radii = [0.034] + radii + [0.034]
    parts = [tube(pts, radii, 6, squash=[(0.9, 1.0)] * len(pts), up=UP, caps=(0.6, 0.6))]
    # Spinous processes: over the withers (the front) a little taller, short toward the loin, leaning back.
    for i, x in enumerate(xs):
        h = (0.12 - 0.07 * i / (count - 1)) * rnd.uniform(0.9, 1.08)
        lean = 0.05 + 0.02 * i / count
        base = spine_at(x) + Vector((0.0, 0.0, 0.025))
        blade = flat([(-0.02, 0.0), (0.022, 0.0), (0.012 + lean, h), (-0.002 + lean, h * 1.03),
                      (-0.016 + lean * 0.5, h * 0.5)], 0.011)
        parts.append(lp.place(blade, tuple(base), (rnd.uniform(-3, 3), 0.0, 0.0)))
    gone = {3, 12, 17}
    snapped = {4, 6, 9, 15}
    fallen_in = {10}                  # one rib on the +Y side has dropped inward, its tip resting inside the cage
    for i, x in enumerate(xs):
        f = i / (count - 1)
        reach = 0.3 + 0.1 * math.sin(math.pi * min(f * 1.1, 1.0))      # half the barrel's width
        for side in (-1.0, 1.0):
            k = 2 * i + (0 if side < 0 else 1)
            if k in gone:
                continue
            splay = 1.12 if side < 0 else 0.96                               # the -Y side has settled outward
            r = reach * splay * rnd.uniform(0.88, 1.1)
            tall = 1.12 / splay
            back = rnd.uniform(0.03, 0.15)
            if k in fallen_in:
                r *= 0.55
                tall = 1.9
            end = rnd.uniform(0.45, 0.62) if k in snapped else 1.0
            twist = rnd.uniform(-0.03, 0.03)
            root = spine_at(x)
            rib_pts = []
            steps = 7
            for j in range(steps + 1):
                phi = end * j / steps * math.radians(150.0)
                y = side * (0.035 + r * math.sin(phi))
                z = root.z + 0.02 - r * tall * (1.0 - math.cos(phi))
                rib_pts.append(Vector((x + back * (1.0 - math.cos(phi)) + twist * math.sin(phi), root.y + y, z)))
            # End each rib just under the floor (where it crosses z = -0.06).
            for j in range(1, len(rib_pts)):
                if rib_pts[j].z < -0.06:
                    a, b = rib_pts[j - 1], rib_pts[j]
                    rib_pts = rib_pts[:j] + [a.lerp(b, (a.z + 0.06) / max(a.z - b.z, 1e-6))]
                    break
            widths = [(0.021 * (1.0 - 0.25 * j / steps), 0.0072) for j in range(len(rib_pts))]
            parts.append(tube(rib_pts, [1.0] * len(rib_pts), 4, squash=widths, up=Vector((1.0, 0.0, 0.0)),
                              caps=(0.3, 0.12 if k in snapped else 0.4)))
    # Two ribs fallen out, lying beside the cage on the splayed side.
    for k, (at, yaw) in enumerate((((0.2, -0.66), -24.0), ((-0.36, -0.58), 38.0))):
        fallen = rib_bone(rnd.uniform(0.45, 0.55), 0.04, 0.014, seed + 7 + k)
        transform(fallen, Matrix.Translation((at[0], at[1], 0.0)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))
        settle(fallen, [], -0.03)
        parts += fallen
    for p in parts:
        bone_uv(p, rnd.randint(0, 9999))
    obj = lp.join(name, parts)
    above = [v.co.copy() for v in obj.data.vertices if v.co.z > -0.02 and abs(v.co.y) < 0.5]
    for sign, cut in ((-1.0, 0.0), (1.0, 0.0)):
        side = [c for c in above if (c.x - cut) * sign >= -0.02]
        lp.hull_points(obj, support(side + [Vector((c.x, c.y, -0.05)) for c in side[::6]], 32))
    return finish(obj, ao=0.35, smooth=55.0)


# --- Cocoons (the Sink's web atlas, its opaque wound-silk cell) ---

class Card:
    """A model being built (Sink.py's): vertices with a position, a shading normal and their vertex colour (R wind
    weight, G sway phase, A occlusion), and faces with their own UVs per corner."""

    def __init__(self, name):
        self.name = name
        self.co, self.no, self.col = [], [], []
        self.faces, self.uvs = [], []

    def vert(self, co, normal, wind=0.0, phase=0.0, occ=1.0):
        n = Vector(normal)
        self.co.append(Vector(co))
        self.no.append(n.normalized() if n.length > 1e-9 else UP.copy())
        self.col.append((min(max(wind, 0.0), 1.0), phase % 1.0, 0.0, min(max(occ, 0.0), 1.0)))
        return len(self.co) - 1

    def face(self, verts, uvs):
        verts, uvs = list(verts), [tuple(u) for u in uvs]
        p = [self.co[i] for i in verts]
        geo = (p[2] - p[0]).cross(p[-1] - p[1]) if len(p) == 4 else (p[1] - p[0]).cross(p[2] - p[0])
        if geo.dot(sum((self.no[i] for i in verts), Vector())) < 0.0:
            verts.reverse()
            uvs.reverse()
        self.faces.append(tuple(verts))
        self.uvs.append(uvs)

    def apply(self, matrix):
        """Moves every vertex (and turns its normal) by a 4x4 matrix."""
        rot = matrix.to_3x3().inverted_safe().transposed()
        self.co = [matrix @ c for c in self.co]
        self.no = [(rot @ n).normalized() for n in self.no]

    def build(self):
        mesh = bpy.data.meshes.new(self.name)
        mesh.from_pydata([tuple(c) for c in self.co], [], self.faces)
        uv = mesh.uv_layers.new(name='UVMap')
        for poly, uvs in zip(mesh.polygons, self.uvs):
            for li, value in zip(poly.loop_indices, uvs):
                uv.data[li].uv = value
        mesh.materials.append(WEB)
        mesh.polygons.foreach_set('use_smooth', [True] * len(self.faces))
        loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
        mesh.loops.foreach_get('vertex_index', loop_vert)
        lt._write_col(mesh, np.asarray(self.col, np.float32)[loop_vert])
        mesh.update()
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


# The rows of each 32 px Strands lane its thread fills (Sink.py's LANE_FILL).
LANE_FILL = {'Cord': (8, 24), 'Rope': (8, 24), 'Tufted': (2, 30), 'Ribbon': (6, 26)}


def lane_rows(lane):
    y0, _ = lw.LANES[lane]
    f0, f1 = LANE_FILL[lane]
    top = lw.REGIONS['Strands'][1]
    return 1.0 - (top + y0 + f0 + 0.5) / lw.SIZE, 1.0 - (top + y0 + f1 - 0.5) / lw.SIZE, (f1 - f0) / 32.0


def wiggle(points, amp, rnd, waves=1.5):
    """A thread made to wander a little sideways, its ends kept where they're glued (Sink.py's)."""
    pts = [Vector(p) for p in points]
    n = len(pts)
    ph = [rnd.uniform(0.0, 6.28) for _ in range(3)]
    out = []
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized()
        s1 = t.cross(UP if abs(t.z) < 0.9 else FRONT).normalized()
        s2 = t.cross(s1).normalized()
        f = i / max(n - 1, 1)
        ease = math.sin(math.pi * f)
        out.append(p + (s1 * math.sin(2.0 * math.pi * waves * f + ph[0]) * 0.8 +
                        s2 * math.sin(2.0 * math.pi * waves * 1.3 * f + ph[1]) * 0.6) * (amp * ease))
    return out


def cord(card, points, width, lane, u0, wind, phase, occ=1.0, crossed=True):
    """A silk thread along a polyline: two crossed strips on a lane of the Strands cell (Sink.py's cord)."""
    pts = [Vector(p) for p in points]
    winds = list(wind) if isinstance(wind, (list, tuple)) else [wind] * len(pts)
    occs = list(occ) if isinstance(occ, (list, tuple)) else [occ] * len(pts)
    va, vb, share = lane_rows(lane)
    width *= share
    s = [0.0]
    for a, b in zip(pts, pts[1:]):
        s.append(s[-1] + (b - a).length)
    us = [(u0 + d * lw.PX_M) / lw.SIZE for d in s]
    frames = []
    for i in range(len(pts)):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        ref = UP if abs(t.z) < 0.9 else FRONT
        s1 = t.cross(ref).normalized()
        s2 = t.cross(s1).normalized()
        up_perp = UP - t * t.dot(UP)
        front = FRONT - t * t.dot(FRONT)
        if front.length < 0.2:
            front = Vector((1.0, 0.0, 0.0)) - t * t.x
        n = up_perp + front.normalized() * 0.6
        frames.append((s1, s2, n if n.length > 1e-6 else front))
    for k in range(2 if crossed else 1):
        rows = []
        for i, p in enumerate(pts):
            side = frames[i][k] * (width * 0.5)
            rows.append((card.vert(p - side, frames[i][2], winds[i], phase, occs[i]),
                         card.vert(p + side, frames[i][2], winds[i], phase, occs[i])))
        for i in range(len(pts) - 1):
            (a0, b0), (a1, b1) = rows[i], rows[i + 1]
            card.face((a0, b0, b1, a1), [(us[i], va), (us[i], vb), (us[i + 1], vb), (us[i + 1], va)])


def surface_cord(card, points, normals, width, lane, u0):
    """A thread lying on a surface: one strip turned flat to it (Sink.py's)."""
    pts = [Vector(p) for p in points]
    va, vb, share = lane_rows(lane)
    s = [0.0]
    for a, b in zip(pts, pts[1:]):
        s.append(s[-1] + (b - a).length)
    rows = []
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        n = Vector(normals[i]).normalized()
        side = n.cross(t).normalized() * (width * share * 0.5)
        rows.append((card.vert(p - side, n), card.vert(p + side, n), (u0 + s[i] * lw.PX_M) / lw.SIZE))
    for (a0, b0, u_0), (a1, b1, u_1) in zip(rows, rows[1:]):
        card.face((a0, b0, b1, a1), ((u_0, va), (u_0, vb), (u_1, vb), (u_1, va)))


def _ellipsoid_sdf(points, center, radii, rotation):
    p = (points - center) @ rotation
    r = np.asarray(radii)
    k0 = np.linalg.norm(p / r, axis=1)
    k1 = np.linalg.norm(p / (r * r), axis=1)
    return k0 * (k0 - 1.0) / np.maximum(k1, 1e-9)


def _capsule_sdf(points, a, b, radius):
    ab = b - a
    t = np.clip(((points - a) @ ab) / max(ab @ ab, 1e-12), 0.0, 1.0)
    return np.linalg.norm(points - (a + t[:, None] * ab), axis=1) - radius


class Shape:
    """What lies inside a bundle: ellipsoids (center, radii) and capsules (a, b, radius), as one distance field."""

    def __init__(self, ellipsoids=(), capsules=()):
        self.ellipsoids = [(np.array(c, float), np.array(r, float), np.eye(3)) for c, r in ellipsoids]
        self.capsules = [(np.array(a, float), np.array(b, float), r) for a, b, r in capsules]

    def sdf(self, points):
        d = np.full(len(points), np.inf)
        for c, r, rot in self.ellipsoids:
            d = np.minimum(d, _ellipsoid_sdf(points, c, r, rot))
        for a, b, r in self.capsules:
            d = np.minimum(d, _capsule_sdf(points, a, b, r))
        return d


def resample(profile, count):
    """count points spaced evenly along a profile polyline [(radius, z)]."""
    pts = [Vector(p) for p in profile]
    s = [0.0]
    for a, b in zip(pts, pts[1:]):
        s.append(s[-1] + (b - a).length)
    out = []
    for i in range(count):
        d = s[-1] * i / (count - 1)
        k = max([j for j in range(len(s) - 1) if s[j] <= d + 1e-9] or [0])
        f = (d - s[k]) / max(s[k + 1] - s[k], 1e-9)
        out.append(pts[k].lerp(pts[k + 1], f))
    return out


class Skin:
    """A bundle's silk skin (Sink.py's egg-sac skin, simplified): a lathe of a profile [(radius, z)] from its bottom
    pole up, segs round, squashed front to back by squash_y, pushed out to clear what is inside it by `clear` (the silk
    drawn tight over a body), wrinkled, and wound round in two families of spiral ridges (wound: their height)."""

    def __init__(self, profile, segs, rings, shape, clear=0.04, squash_y=1.0, wrinkle=0.012, wound=0.014, seed=0,
                 closed_top=True):
        self.segs = segs
        prof = resample(profile, rings + 1)
        rows = []
        for q in prof:
            rows.append([(math.cos(2.0 * math.pi * k / segs) * q.x, math.sin(2.0 * math.pi * k / segs) * q.x * squash_y,
                          q.y) for k in range(segs)])
        P = np.array(rows, dtype=np.float64)
        C = np.stack([np.zeros(len(prof)), np.zeros(len(prof)), np.array([q.y for q in prof])], axis=1)[:, None, :]
        out = P - C
        lengths = np.linalg.norm(out, axis=2, keepdims=True)
        out = np.where(lengths > 1e-6, out / np.maximum(lengths, 1e-9), np.array([0.0, 0.0, -1.0]))
        flat = P.reshape(-1, 3)
        d = shape.sdf(flat)
        push = np.maximum(clear - d, 0.0).reshape(P.shape[:2])
        for _ in range(3):
            push = 0.5 * push + 0.125 * (np.roll(push, 1, axis=1) + np.roll(push, -1, axis=1) +
                                         np.vstack([push[:1], push[:-1]]) + np.vstack([push[1:], push[-1:]]))
        rnd = random.Random(seed)
        off = Vector((rnd.uniform(0, 50), rnd.uniform(0, 50), rnd.uniform(0, 50)))
        wr = np.array([[noise.noise(Vector(P[j, k]) * 3.0 + off) + 0.4 * noise.noise(Vector(P[j, k]) * 8.0 - off)
                        for k in range(segs)] for j in range(len(rows))])
        bands = np.zeros(P.shape[:2])
        ph1, ph2 = rnd.uniform(0.0, 6.28), rnd.uniform(0.0, 6.28)
        for j in range(len(rows)):
            fade = min(1.0, float(lengths[j].mean()) / 0.12)
            for k in range(segs):
                a = 2.0 * math.pi * k / segs
                z = float(P[j, k, 2])
                b1 = max(math.sin(2.0 * a + 2.0 * math.pi * z / 0.34 + ph1), 0.0)
                b2 = max(math.sin(-3.0 * a + 2.0 * math.pi * z / 0.42 + ph2), 0.0)
                lump = 0.6 + 0.4 * noise.noise(Vector((a * 1.3, z * 2.0, seed + 7.0)))
                bands[j, k] = wound * fade * lump * (b1 * b1 + b2 * b2 - 0.5)
        P = P + out * (push + wrinkle * wr + bands)[..., None]
        if closed_top:
            P[-1] = P[-1].mean(axis=0)
        P[0] = P[0].mean(axis=0)
        self.P = P
        self.closed_top = closed_top

    def add(self, card, v_repeats=4, occ=None):
        """Faces of the skin on the Sac cell (Sink.py's Sac.add): U once round it, V up it repeating v_repeats times;
        a pole is one vertex whose corners take their own face's middle U. occ(j) shades a ring (the threads' grime)."""
        top = lw.REGIONS['Sac'][1] + lw.SAC_PAD
        rows = len(self.P)
        per = (rows - 1) // v_repeats
        assert per * v_repeats == rows - 1, 'v_repeats must divide the rings'
        center = Vector((0.0, 0.0, float(self.P[:, :, 2].mean())))
        verts = []
        for j in range(rows):
            pole = j == 0 or (j == rows - 1 and self.closed_top)
            row = []
            for k in range(self.segs):
                if pole and k:
                    row.append(row[0])
                    continue
                co = Vector(self.P[j, k])
                n = (co - center) if pole else co - Vector((0.0, 0.0, co.z))
                row.append(card.vert(co, n, 0.0, 0.0, occ(j) if occ else 1.0))
            verts.append(row)

        def v_at(j, upper):
            local = j % per
            if upper and local == 0:
                local = per
            return 1.0 - (top + lw.SAC_PERIOD * (1.0 - local / per)) / lw.SIZE

        for j in range(rows - 1):
            va, vb = v_at(j, False), v_at(j + 1, True)
            for k in range(self.segs):
                k1 = (k + 1) % self.segs
                u0, u1 = k / self.segs, (k + 1) / self.segs
                if j == 0:
                    card.face((verts[0][k], verts[1][k1], verts[1][k]), (((u0 + u1) * 0.5, va), (u1, vb), (u0, vb)))
                elif j == rows - 2 and self.closed_top:
                    card.face((verts[j][k], verts[j][k1], verts[j + 1][k]), ((u0, va), (u1, va), ((u0 + u1) * 0.5, vb)))
                else:
                    card.face((verts[j][k], verts[j][k1], verts[j + 1][k1], verts[j + 1][k]),
                              ((u0, va), (u1, va), (u1, vb), (u0, vb)))
        return verts


def halo(card, skin, rnd, segs=12, rings=6, lift=0.03, bottom=0.04, top=0.96, repeats=4, wind=0.0):
    """A thin fuzz of loose silk just off the skin, breaking its outline (Sink.py's halo, on the Wrap cell's fuzz
    half): a shell lift (m) outside the skin's furthest reach nearby, from `bottom` to `top` of its height."""
    P = skin.P
    pts = P.reshape(-1, 3)
    ring_z = P[:, :, 2].mean(axis=1)
    z_lo = float(ring_z[0] + (ring_z[-1] - ring_z[0]) * bottom)
    z_hi = float(ring_z[0] + (ring_z[-1] - ring_z[0]) * top)
    u0 = lw.WRAP_SPLIT + rnd.uniform(16.0, 40.0)
    ang = np.arctan2(pts[:, 1], pts[:, 0])
    grid = []
    for j in range(rings + 1):
        z = z_lo + (z_hi - z_lo) * j / rings
        row = []
        for k in range(segs):
            a = 2.0 * math.pi * k / segs
            near = (np.abs(pts[:, 2] - z) < 0.12) & (np.abs((ang - a + math.pi) % (2.0 * math.pi) - math.pi) < 0.35)
            r = float(np.hypot(pts[near, 0], pts[near, 1]).max()) if near.any() else 0.05
            row.append(Vector((math.cos(a) * (r + lift), math.sin(a) * (r + lift), z)))
        grid.append(row)

    def normal(j, k):
        c = Vector((0.0, 0.0, grid[j][k].z))
        up_ = grid[min(j + 1, rings)][k] - grid[max(j - 1, 0)][k]
        nn = (grid[j][(k + 1) % segs] - grid[j][(k - 1) % segs]).cross(up_)
        return (nn if nn.dot(grid[j][k] - c) >= 0.0 else -nn).normalized()
    verts = [[card.vert(grid[j][k], normal(j, k), wind * j / rings, 0.0) for k in range(segs)]
             for j in range(rings + 1)]
    per = segs // repeats

    def uv(z, k, closing):
        local = k % per
        if closing and local == 0:
            local = per
        return lw.uv('Wrap', u0 + (z - z_lo) * 200.0, lw.WRAP_PAD + lw.WRAP_PERIOD * local / per)
    for k in range(segs):
        k1 = (k + 1) % segs
        for j in range(rings):
            card.face((verts[j][k], verts[j][k1], verts[j + 1][k1], verts[j + 1][k]),
                      (uv(grid[j][k].z, k, False), uv(grid[j][k1].z, k + 1, True),
                       uv(grid[j + 1][k1].z, k + 1, True), uv(grid[j + 1][k].z, k, False)))


def wound_threads(card, skin, rnd, count, rows=(0.12, 0.9), lift=0.008, turns=(0.4, 1.1)):
    """Threads wound round the skin in loose spirals, lying on it (Sink.py's)."""
    P = skin.P
    n_rows, segs = P.shape[0], P.shape[1]
    for _ in range(count):
        j0, j1 = rnd.uniform(*rows) * (n_rows - 1), rnd.uniform(*rows) * (n_rows - 1)
        a0 = rnd.uniform(0.0, segs)
        spin = rnd.uniform(*turns) * rnd.choice((-1.0, 1.0))
        pts, nors = [], []
        for i in range(17):
            f = i / 16.0
            j = j0 + (j1 - j0) * f
            k = a0 + spin * segs * f
            jl, kl = int(math.floor(j)), int(math.floor(k))
            fj, fk = j - jl, k - kl
            jl = min(max(jl, 1), n_rows - 2)
            corners = [Vector(P[jl + dj, (kl + dk) % segs]) for dj in (0, 1) for dk in (0, 1)]
            q = (corners[0].lerp(corners[1], fk)).lerp(corners[2].lerp(corners[3], fk), fj)
            n = (corners[1] - corners[0]).cross(corners[2] - corners[0])
            if n.dot(q - Vector((0.0, 0.0, q.z))) < 0.0:
                n = -n
            n.normalize()
            pts.append(q + n * lift)
            nors.append(n)
        surface_cord(card, pts, nors, rnd.uniform(0.026, 0.036), rnd.choice(('Cord', 'Rope')), rnd.uniform(0.0, 1400.0))


def silk_rope(card, points, radius, sides):
    """A twisted rope of silk (Sink.py's sac stalk) on the Sac cell: a tube whose UVs spiral round it."""
    pts = [Vector(p) for p in points]
    radii = list(radius) if isinstance(radius, (list, tuple)) else [radius] * len(pts)
    top = lw.REGIONS['Sac'][1] + lw.SAC_PAD
    rows, params = [], []
    s = 0.0
    for i, p in enumerate(pts):
        if i:
            s += (p - pts[i - 1]).length
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        ref = Vector((1.0, 0.0, 0.0)) if abs(t.x) < 0.9 else Vector((0.0, 1.0, 0.0))
        b1 = t.cross(ref).normalized()
        b2 = t.cross(b1)
        rows.append([card.vert(p + (b1 * math.cos(2.0 * math.pi * k / sides) + b2 * math.sin(2.0 * math.pi * k / sides))
                               * radii[i], b1 * math.cos(2.0 * math.pi * k / sides) +
                               b2 * math.sin(2.0 * math.pi * k / sides)) for k in range(sides)])
        params.append((s * 1.5 * 0.08, 1.0 - (top + lw.SAC_PERIOD * min(s / 0.6, 1.0) * 0.9 + 10.0) / lw.SIZE))
    for i in range(len(rows) - 1):
        (s0, v0), (s1, v1) = params[i], params[i + 1]
        for k in range(sides):
            k1 = (k + 1) % sides
            u_a, u_b = k / sides * 0.08, (k + 1) / sides * 0.08
            card.face((rows[i][k], rows[i][k1], rows[i + 1][k1], rows[i + 1][k]),
                      ((u_a + s0, v0), (u_b + s0, v0), (u_b + s1, v1), (u_a + s1, v1)))


def cocoon_lying(name='Cocoon_Lying', seed=51):
    """A man-sized bundle, built standing (feet at z = 0, face toward -Y) round a body (head, chest, folded arms,
    hips, legs together), then laid on its back along +X (head end +X) with its back flattened on the floor; a fuzz
    of loose silk round it, threads wound round it and threads out to the floor gluing it down."""
    rnd = random.Random(seed)
    body = Shape(
        ellipsoids=[((0.0, -0.01, 1.68), (0.095, 0.105, 0.115)),          # head
                    ((0.0, 0.0, 1.24), (0.19, 0.115, 0.23)),              # chest
                    ((0.0, 0.0, 0.92), (0.16, 0.11, 0.12))],              # hips
        capsules=[((0.0, 0.0, 1.45), (0.0, -0.005, 1.6), 0.048),          # neck
                  ((0.17, -0.02, 1.42), (0.2, -0.04, 1.12), 0.05),        # arms folded down the sides
                  ((-0.17, -0.02, 1.42), (-0.2, -0.04, 1.12), 0.05),
                  ((0.2, -0.04, 1.12), (0.08, -0.1, 0.98), 0.045),
                  ((-0.2, -0.04, 1.12), (-0.07, -0.1, 0.99), 0.045),
                  ((0.08, 0.0, 0.85), (0.075, -0.01, 0.46), 0.068),      # legs
                  ((-0.08, 0.0, 0.85), (-0.075, -0.01, 0.46), 0.068),
                  ((0.075, -0.01, 0.46), (0.06, 0.0, 0.09), 0.052),
                  ((-0.075, -0.01, 0.46), (-0.06, 0.0, 0.09), 0.052),
                  ((0.06, -0.05, 0.06), (0.06, -0.14, 0.05), 0.04),      # feet
                  ((-0.06, -0.05, 0.06), (-0.06, -0.14, 0.05), 0.04)])
    profile = [(0.0, -0.02), (0.07, 0.0), (0.1, 0.1), (0.11, 0.4), (0.12, 0.75), (0.14, 1.0), (0.15, 1.25),
               (0.13, 1.42), (0.06, 1.52), (0.055, 1.58), (0.07, 1.68), (0.05, 1.79), (0.0, 1.83)]
    skin = Skin(profile, 16, 20, body, clear=0.03, squash_y=0.7, wrinkle=0.01, wound=0.016, seed=seed)
    card = Card(name)
    skin.add(card, v_repeats=5)
    halo(card, skin, rnd, segs=12, rings=7, lift=0.026, bottom=0.03, top=0.97, repeats=4)
    wound_threads(card, skin, rnd, 11)
    # Lay it down: standing frame (x, y, z) to lying (x, z, -y) puts the head at +Y and the face up; then the head
    # turned to +X, the back (now under it) flattened onto the floor and the whole sunk a little.
    lay = (Matrix.Rotation(math.radians(-90.0), 4, 'Z') @ Matrix.Rotation(math.radians(-90.0), 4, 'X') @
           Matrix.Rotation(math.radians(14.0), 4, 'Z'))                    # rolled a little onto its side
    card.apply(lay)
    for c in card.co:
        if c.z < 0.0:
            c.z *= 0.6
    zmin = min(c.z for c in card.co)
    xmid = (max(c.x for c in card.co) + min(c.x for c in card.co)) * 0.5
    card.apply(Matrix.Translation((-xmid, 0.0, -zmin - 0.03)))
    # Threads gluing it to the floor: from low on its sides out to the ground, fanning, a few over its ends.
    P = np.array([tuple(c) for c in card.co])
    for k in range(9):
        side = 1.0 if k % 2 else -1.0
        x = rnd.uniform(-0.75, 0.8) if k < 7 else (0.95 if k == 7 else -0.92)
        if k < 7:
            near = P[(np.abs(P[:, 0] - x) < 0.06) & (P[:, 1] * side > 0.0) & (P[:, 2] > 0.05) & (P[:, 2] < 0.12)]
            start = Vector(near[np.argmax(near[:, 1] * side)]) if len(near) else Vector((x, side * 0.2, 0.08))
            end = start + Vector((rnd.uniform(-0.15, 0.15), side * rnd.uniform(0.16, 0.32), 0.0))
        else:
            near = P[(np.sign(P[:, 0]) == np.sign(x)) & (np.abs(P[:, 1]) < 0.06) & (P[:, 2] > 0.08) & (P[:, 2] < 0.16)]
            start = Vector(near[np.argmax(np.abs(near[:, 0]))]) if len(near) else Vector((x, 0.0, 0.1))
            end = start + Vector((math.copysign(rnd.uniform(0.2, 0.35), x), rnd.uniform(-0.15, 0.15), 0.0))
        end.z = 0.004
        mid = start.lerp(end, 0.5) + Vector((0.0, 0.0, 0.03))
        pts = wiggle([start, start.lerp(mid, 0.5), mid, mid.lerp(end, 0.5), end], 0.012, rnd)
        cord(card, pts, 0.04, rnd.choice(('Cord', 'Rope', 'Tufted')), rnd.uniform(0.0, 1500.0), 0.0, 0.0,
             occ=[0.9, 0.85, 0.8, 0.75, 0.7])
    obj = card.build()
    lm.smooth(obj, 80.0)
    skin_pts = [card.co[i] for i in range(len(card.co))]
    body_pts = [c for c in skin_pts if c.z > -0.02]
    lp.hull_points(obj, support([c for c in body_pts if abs(c.y) < 0.32 and abs(c.x) < 0.95], 32))
    lt.bake_vertex_ao(obj, samples=32, distance=0.4, ground=True)
    grime(obj, WEB, height=0.22, low=0.55, patches=0.35, seed=seed)
    obj['Nanite'] = 0
    obj['LODs'] = '50,25'
    return obj


def cocoon_hung(name='Cocoon_Hung', seed=52):
    """A calf-sized bundle hanging from its knot: a lumpy teardrop round a curled body (head tucked down, legs folded
    against it), gathered at the top into a twisted silk rope, which runs up with three strands to where they glue to
    the rock: the pivot, SOCKET_Top. The bundle's bottom is 1.9 m under the pivot."""
    rnd = random.Random(seed)
    body = Shape(
        ellipsoids=[((0.0, 0.02, 0.66), (0.19, 0.16, 0.34)),             # the body
                    ((0.04, -0.1, 0.26), (0.1, 0.1, 0.13))],               # the head, tucked down
        capsules=[((0.13, -0.1, 0.48), (0.1, -0.15, 0.85), 0.045),         # folded legs
                  ((-0.12, -0.1, 0.5), (-0.1, -0.16, 0.86), 0.045),
                  ((0.12, 0.1, 0.45), (0.15, 0.06, 0.82), 0.045),
                  ((-0.12, 0.12, 0.45), (-0.16, 0.05, 0.8), 0.045)])
    profile = [(0.0, 0.0), (0.07, 0.02), (0.11, 0.1), (0.14, 0.28), (0.15, 0.52), (0.15, 0.78), (0.13, 0.98),
               (0.1, 1.12), (0.06, 1.21), (0.045, 1.26)]
    skin = Skin(profile, 14, 16, body, clear=0.03, squash_y=0.9, wrinkle=0.012, wound=0.016, seed=seed,
                closed_top=False)
    card = Card(name)
    skin.add(card, v_repeats=4)
    halo(card, skin, rnd, segs=12, rings=6, lift=0.026, bottom=0.04, top=0.9, repeats=4, wind=0.08)
    wound_threads(card, skin, rnd, 8)
    knot = Vector(skin.P[-1].mean(axis=0))
    rope = [knot - Vector((0.0, 0.0, 0.05)), knot + Vector((0.008, 0.0, 0.1)), knot + Vector((-0.004, 0.006, 0.22))]
    silk_rope(card, rope, [0.06, 0.045, 0.034], 6)
    pivot = rope[-1] + Vector((0.0, 0.0, 0.4))
    # The main line on up from the rope to the pivot, and three strands fanning to their own glue spots round it.
    tops = [pivot] + [pivot + Vector((math.cos(a) * r, math.sin(a) * r, rnd.uniform(-0.05, 0.03)))
                      for a, r in ((0.3, 0.26), (2.4, 0.3), (4.3, 0.24))]
    for k, top in enumerate(tops):
        start = rope[-1] if k == 0 else rope[1] + Vector((rnd.uniform(-0.02, 0.02), rnd.uniform(-0.02, 0.02), 0.0))
        pts = [start.lerp(top, f) for f in (0.0, 0.25, 0.5, 0.75, 1.0)]
        pts = wiggle(pts, 0.008, rnd, waves=1.0)
        winds = [0.08 * (1.0 - f) for f in (0.0, 0.25, 0.5, 0.75, 1.0)]
        cord(card, pts, 0.05 if k == 0 else 0.04, 'Rope' if k == 0 else rnd.choice(('Cord', 'Tufted')),
             rnd.uniform(0.0, 1500.0), winds, rnd.random(), occ=[1.0, 0.95, 0.9, 0.85, 0.75])
    # Sway: the whole thing swings a little on its rope as one piece (one phase), more the lower it hangs.
    for i, c in enumerate(card.co):
        w = min(0.25, max(0.0, (pivot.z - c.z) / 2.2) * 0.25)
        card.col[i] = (w, 0.0, 0.0, card.col[i][3])
    card.apply(Matrix.Translation(-pivot))
    obj = card.build()
    lm.smooth(obj, 80.0)
    lt.bake_vertex_ao(obj, samples=32, distance=0.4, ground=False)
    grime(obj, WEB, height=None, patches=0.35, seed=seed)
    lm.socket(obj, 'Top', (0.0, 0.0, 0.0))
    obj['Nanite'] = 0
    obj['LODs'] = '50,25'
    obj['Collision'] = 'None'
    return obj


# --- The larder: a heap of wrapped prey ---

def lay_bundle(card, roll, flatten=0.6, sink=0.03):
    """A bundle built standing (its axis up z, its front -Y) laid on its side along +X (its top end at +X), rolled
    roll degrees about its length, the part under its axis flattened by flatten (it has settled), centred on x and
    sunk sink into the floor."""
    card.apply(Matrix.Rotation(math.radians(-90.0), 4, 'Z') @ Matrix.Rotation(math.radians(-90.0), 4, 'X') @
               Matrix.Rotation(math.radians(roll), 4, 'Z'))
    for c in card.co:
        if c.z < 0.0:
            c.z *= flatten
    zmin = min(c.z for c in card.co)
    xmid = (max(c.x for c in card.co) + min(c.x for c in card.co)) * 0.5
    card.apply(Matrix.Translation((-xmid, 0.0, -zmin - sink)))
    return card


def bend(card, joints):
    """Bends a bundle built standing (axis up z): joints [(height, degrees)], lowest first; everything under a joint
    (by its height as built) turns about the x-axis line through it, blended over 14 cm, the lower joints first, so a
    knee folds the shin and the hip then swings the folded leg."""
    base = [c.copy() for c in card.co]
    for height, degrees in joints:
        pivot = Vector((0.0, 0.0, height))
        for i, c0 in enumerate(base):
            t = min(max((height + 0.14 - c0.z) / 0.28, 0.0), 1.0)
            w = t * t * (3.0 - 2.0 * t)
            if w <= 0.0:
                continue
            r = Matrix.Rotation(math.radians(degrees * w), 3, 'X')
            card.co[i] = r @ (card.co[i] - pivot) + pivot
            card.no[i] = (r @ card.no[i]).normalized()
    return card


def wrapped(name, seed, shape, profile, segs, rings, v_repeats, squash_y=0.85, roll=0.0, threads=3, fuzz=0,
            joints=()):
    """One silk-wrapped prey bundle (the cocoons' skin, wound threads and, with fuzz rings, a fuzz of loose silk),
    bent at its joints (bend) and laid on its side along +X: a card."""
    rnd = random.Random(seed)
    skin = Skin(profile, segs, rings, shape, clear=0.03, squash_y=squash_y, wrinkle=0.014, wound=0.018, seed=seed)
    card = Card(name)
    skin.add(card, v_repeats=v_repeats)
    if fuzz:
        halo(card, skin, rnd, segs=10, rings=fuzz, lift=0.024, bottom=0.08, top=0.92, repeats=5)
    wound_threads(card, skin, rnd, threads)
    return lay_bundle(bend(card, joints), roll)


def rest(obj, under, ground, yaw, at, tilts=range(-20, 21, 4), rolls=range(-30, 31, 6), embed=0.012):
    """Rests a laid bundle (an object along X, centred) on the ground and the meshes under it: tried at each tilt about
    its width (degrees, head end up or down) and each roll about its length (leaning sideways), turned by yaw (degrees)
    and moved over at (x, y), dropped till it touches, and kept at the pose whose two ends both come nearest to
    touching (a bundle leaning on another, its feet on the floor), the flattest of equals; then sunk embed into what it
    lies on. Returns the tilt, the roll and how far its ends stay off."""
    verts, polys = [], []
    for o in under:
        base = len(verts)
        verts.extend(v.co.copy() for v in o.data.vertices)
        polys.extend([base + i for i in p.vertices] for p in o.data.polygons)
    base = len(verts)
    verts.extend(Vector(c) for c in ((-50.0, -50.0, ground), (50.0, -50.0, ground), (50.0, 50.0, ground),
                                     (-50.0, 50.0, ground)))
    polys.append([base, base + 1, base + 2, base + 3])
    tree = BVHTree.FromPolygons(verts, polys)
    P0 = [v.co.copy() for v in obj.data.vertices][::2]
    xs = [c.x for c in P0]
    x0, x1 = min(xs), max(xs)
    ends = ([i for i, c in enumerate(P0) if c.x < x0 + (x1 - x0) * 0.3],
            [i for i, c in enumerate(P0) if c.x > x1 - (x1 - x0) * 0.3])
    best = None
    for tilt in tilts:
        for roll in rolls:
            m = (Matrix.Translation((at[0], at[1], 2.0)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z') @
                 Matrix.Rotation(math.radians(-tilt), 4, 'Y') @ Matrix.Rotation(math.radians(roll), 4, 'X'))
            gaps = []
            for c in P0:
                hit = tree.ray_cast(m @ c + Vector((0.0, 0.0, 1e-4)), Vector((0.0, 0.0, -1.0)), 6.0)
                gaps.append(hit[3] if hit[0] is not None else 6.0)
            drop = min(gaps)
            hover = max(min(gaps[i] for i in end) - drop for end in ends)
            score = hover + 0.0004 * (abs(tilt) + abs(roll))
            if best is None or score < best[0] - 1e-9:
                best = (score, hover, tilt, roll, m, drop)
    _, hover, tilt, roll, m, drop = best
    obj.data.transform(Matrix.Translation((0.0, 0.0, -drop - embed)) @ m)
    obj.data.update()
    return tilt, roll, hover


def hoof_leg(seed, rs=0.019):
    """A calf's lower leg poking out of the silk: the cannon bone, the pastern, a cloven hoof (two dark claws, on the
    trim sheet's beam strip like the horns), along +X from the origin."""
    parts = long_bone('cannon', 0.22, rs, seed)
    transform(parts, Matrix.Translation((0.11, 0.0, 0.0)))
    pastern = tube([(0.215, 0.0, 0.0), (0.27, 0.0, -0.012), (0.3, 0.0, -0.02)], [0.024, 0.02, 0.022], 6,
                   caps=(0.3, 0.3))
    parts.append(pastern)
    for p in parts:
        bone_uv(p, seed)
    claws = []
    for side in (-1.0, 1.0):
        claw = tube([(0.29, side * 0.019, -0.006), (0.33, side * 0.022, -0.03), (0.375, side * 0.016, -0.055)],
                    [0.03, 0.026, 0.01], 6, squash=[(1.0, 0.62), (1.0, 0.6), (0.8, 0.6)], up=UP, caps=(0.2, 0.6),
                    uv=horn_uv(seed + int(side)))
        lt.assign(claw, TRIM)
        claws.append(claw)
    return parts + claws


def larder(name='Bones_PileB', seed=12):
    """The larder: what she has wrapped and kept, heaped by where she rests: a calf curled on its side, a man curled
    up against it, a dog-sized bundle at one end and something small on top, bound together with strands
    over the top and threads out to the floor, a calf's hoofed leg poking out of the silk, a femur standing up between
    the bundles, a steer's jaw, ribs and vertebrae and splinters round the foot. The silk old and grey with dust:
    darker toward the floor, in the creases and in patches (its vertex occlusion), never the brightest thing in the
    den. One low hull round the bundles (the bones have none)."""
    rnd = random.Random(seed)
    calf = Shape(
        ellipsoids=[((0.0, 0.02, 0.55), (0.22, 0.18, 0.38)), ((0.05, -0.14, 0.2), (0.1, 0.1, 0.13))],
        capsules=[((0.14, -0.12, 0.45), (0.12, -0.18, 0.85), 0.05), ((-0.13, -0.12, 0.47), (-0.12, -0.18, 0.86), 0.05),
                  ((0.13, 0.12, 0.42), (0.16, 0.08, 0.82), 0.05), ((-0.13, 0.14, 0.42), (-0.17, 0.07, 0.8), 0.05)])
    man = Shape(
        ellipsoids=[((0.0, -0.01, 1.62), (0.095, 0.105, 0.115)), ((0.0, 0.0, 1.2), (0.18, 0.11, 0.22)),
                    ((0.0, 0.0, 0.88), (0.15, 0.1, 0.11))],
        capsules=[((0.0, 0.0, 1.4), (0.0, 0.0, 1.55), 0.046), ((0.16, -0.02, 1.38), (0.19, -0.04, 1.08), 0.048),
                  ((-0.16, -0.02, 1.38), (-0.19, -0.04, 1.08), 0.048), ((0.08, 0.0, 0.82), (0.07, -0.01, 0.1), 0.06),
                  ((-0.08, 0.0, 0.82), (-0.07, -0.01, 0.1), 0.06)])
    dog = Shape(ellipsoids=[((0.0, 0.0, 0.3), (0.19, 0.16, 0.23)), ((0.08, -0.1, 0.44), (0.08, 0.08, 0.09))],
                capsules=[((-0.12, 0.08, 0.12), (0.1, 0.12, 0.1), 0.04)])
    small = Shape(ellipsoids=[((0.0, 0.0, 0.19), (0.11, 0.1, 0.15))])
    calf_card = wrapped('_Calf', seed + 1, calf, [(0.0, 0.0), (0.1, 0.02), (0.17, 0.1), (0.21, 0.3), (0.23, 0.55),
                                                  (0.22, 0.8), (0.17, 1.0), (0.1, 1.12), (0.0, 1.18)],
                        14, 12, 4, roll=70.0, threads=3, fuzz=3)
    man_card = wrapped('_Man', seed + 2, man, [(0.0, -0.02), (0.07, 0.0), (0.1, 0.1), (0.11, 0.4), (0.12, 0.72),
                                               (0.14, 0.96), (0.15, 1.2), (0.13, 1.37), (0.06, 1.47), (0.055, 1.53),
                                               (0.07, 1.63), (0.05, 1.73), (0.0, 1.77)],
                       14, 16, 4, squash_y=0.72, roll=90.0, threads=3, fuzz=3, joints=((0.46, 70.0), (0.86, -75.0)))
    dog_card = wrapped('_Dog', seed + 3, dog, [(0.0, 0.0), (0.12, 0.03), (0.19, 0.14), (0.21, 0.3), (0.18, 0.46),
                                               (0.1, 0.56), (0.0, 0.6)], 12, 8, 2, roll=40.0, threads=2)
    small_card = wrapped('_Small', seed + 4, small, [(0.0, 0.0), (0.09, 0.03), (0.13, 0.12), (0.13, 0.26),
                                                     (0.08, 0.36), (0.0, 0.4)], 10, 6, 2, roll=20.0, threads=1)
    bundles = []
    calf_obj = calf_card.build()
    rest(calf_obj, [], -0.03, 6.0, (0.1, 0.26), tilts=(0,), rolls=(0,))
    bundles.append(calf_obj)
    dog_obj = dog_card.build()
    rest(dog_obj, bundles, -0.03, 38.0, (0.66, -0.1))
    bundles.append(dog_obj)
    man_obj = man_card.build()
    tilt, roll, hover = rest(man_obj, bundles, -0.03, -14.0, (-0.4, -0.02))
    bundles.append(man_obj)
    small_obj = small_card.build()
    rest(small_obj, bundles, -0.03, -50.0, (-0.12, 0.14), embed=0.02)
    bundles.append(small_obj)
    print(f'DEN: larder: the man lies curled against the calf, tilted {tilt} and rolled {roll} degrees (his ends '
          f'{hover:.3f} m from touching)', flush=True)
    # Bones poking out between them.
    leg = hoof_leg(seed + 5)
    transform(leg, Matrix.Translation((0.62, 0.22, 0.14)) @ Matrix.Rotation(math.radians(-38.0), 4, 'Z') @
              Matrix.Rotation(math.radians(-30.0), 4, 'Y'))
    items = [dict(kind='femur', length=0.43, rs=0.027, seed=208, x=0.4, y=-0.02, yaw=-100.0, pitch=-60.0, sink=-0.16,
                  embed=0.08),
             dict(kind='mandible', size=0.4, seed=206, x=-0.18, y=-0.66, yaw=-8.0, sink=-0.004),
             dict(kind='rib', length=0.5, width=0.042, thick=0.016, seed=231, x=0.42, y=-0.66, yaw=-14.0, sink=-0.04),
             dict(kind='rib', length=0.44, width=0.04, thick=0.015, seed=232, x=-0.86, y=0.38, yaw=12.0, sink=-0.04),
             dict(kind='tibia', length=0.39, rs=0.024, seed=233, x=-1.06, y=-0.12, yaw=72.0, roll=30.0, sink=-0.01),
             dict(kind='man_femur', length=0.45, rs=0.014, seed=234, x=0.92, y=0.36, yaw=-28.0, sink=-0.006)]
    for k in range(3):     # a run of three vertebrae, still in line, curving
        items.append(dict(kind='vertebra', spine=0.12 - 0.015 * k, seed=211 + k, x=-0.66 + 0.065 * k,
                          y=0.52 + 0.025 * k * k, yaw=-4.0 + 9.0 * k, roll=82.0, sink=-0.03))
    for k in range(6):
        a = rnd.uniform(0.0, 2.0 * math.pi)
        x, y = math.cos(a) * rnd.uniform(0.95, 1.1), math.sin(a) * rnd.uniform(0.62, 0.74)
        items.append(dict(kind='splinter' if k < 4 else 'chip', length=rnd.uniform(0.07, 0.13), width=0.03,
                          size=rnd.uniform(0.03, 0.05), seed=240 + k, x=x, y=y, yaw=rnd.uniform(0, 180), sink=-0.006))
    bones = lay_bones(items, under=bundles)
    for p in bones:
        bone_uv(p, rnd.randint(0, 9999))
    # Strands binding the heap: from high on one bundle over whatever lies between to high on another, draped on the
    # heap; threads from the bundles' feet out to the floor.
    solid = bundles + bones + leg
    verts, polys = [], []
    for o in solid:
        base = len(verts)
        verts.extend(v.co.copy() for v in o.data.vertices)
        polys.extend([base + i for i in p.vertices] for p in o.data.polygons)
    tree = BVHTree.FromPolygons(verts, polys)

    def surface(x, y):
        hit = tree.ray_cast(Vector((x, y, 3.0)), Vector((0.0, 0.0, -1.0)), 6.0)
        return hit[0].z if hit[0] is not None else -0.03

    def high_point(o, near, reach):
        flat_near = Vector((near[0], near[1], 0.0))
        cand = [v.co for v in o.data.vertices if (Vector((v.co.x, v.co.y, 0.0)) - flat_near).length < reach]
        cand = cand or [v.co for v in o.data.vertices]
        return max(cand, key=lambda c: c.z + rnd.uniform(0.0, 0.05)).copy()
    strands = Card(name + '_Strands')
    pairs = [(calf_obj, man_obj), (man_obj, dog_obj), (calf_obj, small_obj), (small_obj, man_obj), (dog_obj, calf_obj),
             (man_obj, calf_obj), (calf_obj, dog_obj), (man_obj, small_obj), (dog_obj, small_obj), (calf_obj, man_obj)]
    for a, b in pairs:
        ca = Vector(np.mean([tuple(v.co) for v in a.data.vertices], axis=0))
        cb = Vector(np.mean([tuple(v.co) for v in b.data.vertices], axis=0))
        mid = ca.lerp(cb, 0.5) + Vector((rnd.uniform(-0.25, 0.25), rnd.uniform(-0.15, 0.15), 0.0))
        p0, p1 = high_point(a, mid.lerp(ca, 0.5), 0.35), high_point(b, mid.lerp(cb, 0.5), 0.35)
        pts = []
        for i in range(8):
            q = p0.lerp(p1, i / 7.0)
            q.z = max(q.z, surface(q.x, q.y)) + 0.008
            pts.append(q)
        cord(strands, pts, 0.042, rnd.choice(('Rope', 'Cord', 'Tufted')), rnd.uniform(0.0, 1500.0), 0.0, 0.0,
             occ=0.85)
    P = np.array([tuple(v.co) for o in bundles for v in o.data.vertices])
    low = P[(P[:, 2] > 0.04) & (P[:, 2] < 0.12)]
    for k in range(10):
        a = 2.0 * math.pi * (k + rnd.uniform(-0.3, 0.3)) / 10.0
        d = np.array([math.cos(a), math.sin(a) * 1.4])
        start = Vector(low[int(np.argmax(low[:, :2] @ d))])
        end = start + Vector((d[0], d[1], 0.0)).normalized() * rnd.uniform(0.18, 0.34)
        end.z = 0.004
        mid = start.lerp(end, 0.5) + Vector((0.0, 0.0, 0.025))
        cord(strands, wiggle([start, start.lerp(mid, 0.5), mid, mid.lerp(end, 0.5), end], 0.01, rnd), 0.04,
             rnd.choice(('Cord', 'Rope', 'Tufted')), rnd.uniform(0.0, 1500.0), 0.0, 0.0, occ=[0.8, 0.7, 0.6, 0.5, 0.45])
    strand_obj = strands.build()
    obj = lp.join(name, bundles + [strand_obj] + bones + leg)
    # Centred on its footprint, the pivot on the floor.
    co = np.array([tuple(v.co) for v in obj.data.vertices])
    mid = (co[:, :2].min(axis=0) + co[:, :2].max(axis=0)) * 0.5
    obj.data.transform(Matrix.Translation((-mid[0], -mid[1], 0.0)))
    obj.data.update()
    body = [Vector((c[0] - mid[0], c[1] - mid[1], c[2])) for c in P]
    lp.hull_points(obj, support([c for c in body if c.z > -0.01] + [Vector((c.x, c.y, -0.03)) for c in body[::25]],
                                40))
    finish(obj, ao=0.4, smooth=70.0)
    grime(obj, WEB, height=0.34, low=0.3, patches=0.55, seed=seed, overall=0.62)
    return obj


# --- The coffin, dragged in in pieces (the graves kit's coffin wood) ---

COFFIN = [(-0.97, -0.17), (-0.35, -0.31), (0.97, -0.21), (0.97, 0.21), (-0.35, 0.31), (-0.97, 0.17)]   # Graves.py's


def wood(obj, seed, axis=(1.0, 0.0, 0.0)):
    """Weathered wood on the trim sheet, the grain along axis: the graves kit's coffin boards."""
    return lp.grain(obj, 'Siding', axis=axis, seed=seed)


def board(length, height, seed, jag=(True, False), thick=0.03):
    """A coffin's side board lying flat (0.4 m wide, 3 cm thick), its ends splintered where jag says."""
    rnd = random.Random(seed)

    def end(x, sign):
        pts = []
        for k in range(6):
            z = height * k / 5
            pts.append((x + sign * (rnd.uniform(-0.07, 0.05) if 0 < k < 5 else rnd.uniform(-0.03, 0.0)), z))
        return pts
    right = end(length, 1.0) if jag[1] else [(length, 0.0), (length, height)]
    left = end(0.0, -1.0) if jag[0] else [(0.0, 0.0), (0.0, height)]
    outline = right + list(reversed(left))
    piece = flat(outline, thick)
    piece.data.transform(Matrix.Rotation(math.radians(90.0), 4, 'X'))       # lying: its face up
    piece.data.transform(Matrix.Translation((-length * 0.5, height * 0.5, thick * 0.5)))
    return wood(piece, seed)


def nail(at, direction, length=0.07, bent=0.0):
    """A big square nail sticking out (the graves kit's oversized nails), maybe bent over."""
    d = Vector(direction).normalized()
    pts = [Vector(at), Vector(at) + d * length * 0.6]
    side = d.cross(UP) if abs(d.z) < 0.9 else Vector((1.0, 0.0, 0.0))
    pts.append(pts[-1] + (d * math.cos(bent) + side.normalized() * math.sin(bent)) * length * 0.4)
    obj = tube(pts, [0.004, 0.004, 0.0035], 4, caps=(0.0, 0.1))
    return lp.grain(obj, 'Iron', axis=d, seed=int(abs(at[0] * 997 + at[1] * 131)))


def coffin_boards(name='CoffinBoards', seed=61):
    """The lid broken in two (the head half flat on the floor, the foot half propped on two side boards), the two
    side boards splintered, one still carrying its iron handle, the end board, nails, splinters. About 2.2 x 1.2 m."""
    rnd = random.Random(seed)
    lid = [(x * 1.03, y * 1.06) for x, y in COFFIN]
    head_outline = [lid[0], lid[1], (0.02, -0.315), (0.09, -0.19), (-0.03, -0.06), (0.11, 0.07), (0.03, 0.19),
                    (0.12, 0.318), lid[4], lid[5]]
    foot_outline = [(0.07, -0.312), lid[2], lid[3], (0.17, 0.316), (0.08, 0.2), (0.16, 0.08), (0.02, -0.055),
                    (0.13, -0.185)]
    head = wood(extrude(head_outline, 0.0, 0.035), seed + 1)
    foot = wood(extrude(foot_outline, 0.0, 0.035), seed + 2)
    pieces = []
    # The head half flat on the floor, turned a little.
    transform([head], Matrix.Translation((-0.42, 0.18, 0.0)) @ Matrix.Rotation(math.radians(6.0), 4, 'Z'))
    pieces.append([head])
    # Two side boards crossing under where the foot half lands, one with its handle.
    b1 = board(1.15, 0.4, seed + 3, jag=(False, True))
    handle = lp.mesh_object(kit._box((0.18, 0.024, 0.035), drop=('+y',)))
    handle.data.transform(Matrix.Rotation(math.radians(-90.0), 4, 'X'))
    handle.data.transform(Matrix.Translation((-0.25, 0.04, 0.03 + 0.012)))
    lp.grain(handle, 'Iron', axis=(1.0, 0.0, 0.0), seed=seed + 4)
    first = [b1, handle]
    transform(first, Matrix.Translation((0.55, -0.1, 0.0)) @ Matrix.Rotation(math.radians(-9.0), 4, 'Z'))
    pieces.append(first)
    b2 = board(0.72, 0.4, seed + 5, jag=(True, True))
    transform([b2], Matrix.Translation((0.72, 0.14, 0.6)) @ Matrix.Rotation(math.radians(28.0), 4, 'Z'))
    settle([b2], [p for group in pieces for p in group], 0.0, embed=0.002)
    pieces.append([b2])
    # The foot half: tipped onto the boards (its +Y edge up), settled.
    transform([foot], Matrix.Rotation(math.radians(-8.0), 4, 'Z') @ Matrix.Rotation(math.radians(-7.0), 4, 'X'))
    transform([foot], Matrix.Translation((0.38, 0.02, 1.0)))
    settle([foot], [p for group in pieces for p in group], -0.004, embed=0.002)
    pieces.append([foot])
    # The end board (the head's), fallen across the head half's end.
    end_board = board(0.36, 0.4, seed + 6, jag=(False, False))
    transform([end_board], Matrix.Translation((-1.08, 0.1, 0.8)) @ Matrix.Rotation(math.radians(76.0), 4, 'Z') @
              Matrix.Rotation(math.radians(4.0), 4, 'X'))
    settle([end_board], [head], -0.004, embed=0.002)
    pieces.append([end_board])
    loose = []
    # Nails: rows along the lid halves' edges (heads), bent ones out of the boards' broken ends.
    for x, y in ((-0.85, -0.16), (-0.6, 0.24), (-0.3, -0.27)):
        p = Matrix.Translation((-0.42, 0.18, 0.0)) @ Matrix.Rotation(math.radians(6.0), 4, 'Z') @ Vector((x, y, 0.035))
        loose.append(lp.nail(tuple(p), (0.0, 0.0, 1.0), size=0.026))
    for k in range(3):
        at = b1.data.vertices[rnd.randrange(len(b1.data.vertices))].co.copy()
        loose.append(nail(at, (rnd.uniform(-1, 1), rnd.uniform(-1, 1), 0.6), 0.07, rnd.uniform(0.3, 1.2)))
    for k in range(4):
        s = lp.block((rnd.uniform(0.14, 0.3), rnd.uniform(0.02, 0.035), 0.012),
                     (rnd.uniform(-0.8, 1.0), rnd.uniform(-0.42, 0.42), 0.006), (0.0, 0.0, rnd.uniform(0.0, 180.0)))
        wood(s, seed + 20 + k)
        loose.append(s)
    groups = pieces + [loose]
    parts = [p for group in groups for p in group]
    for p in parts:
        lt.assign(p, TRIM) if not p.data.materials else None
    obj = lp.join(name, parts)
    lp.hull_points(obj, [tuple(Matrix.Translation((-0.42, 0.18, 0.0)) @ Matrix.Rotation(math.radians(6.0), 4, 'Z') @
                               Vector((x, y, z))) for x, y in head_outline for z in (0.0, 0.035)])
    hull_pts = [v.co.copy() for v in obj.data.vertices if v.co.x > 0.0 and abs(v.co.y) < 0.75]
    lp.hull_points(obj, support(hull_pts, 40))
    lp.slice_at(obj, (0.0, 0.0, 1.0), (0.012,))
    return finish(obj, ao=0.3, smooth=35.0)


# --- Relics of the Unpaid's victims ---

def relic_hat(name='Relic_Hat', seed=71):
    """A dead man's hat lying brim-down: a wide brim curled up at the sides, the crown pinched along its top and
    dented at the front, a faded red band. Felt on the trim sheet's dark beam strip, the band on its red trim."""
    rnd = random.Random(seed)
    profile = [(0.0, 0.0), (0.225, 0.0), (0.232, 0.009), (0.2, 0.013), (0.095, 0.014), (0.09, 0.022),
               (0.089, 0.048), (0.086, 0.06), (0.081, 0.115), (0.066, 0.132), (0.03, 0.124), (0.0, 0.118)]
    hat, bands = lp.lathe(profile, segments=18)
    band_faces = [i for i, b in enumerate(bands) if b == 5]
    felt_faces = [i for i, b in enumerate(bands) if b != 5]
    lp.lathe_uv(hat, 'Beams', faces=felt_faces, seed=seed)
    lp.lathe_uv(hat, 'TrimRed', faces=band_faces, seed=seed)
    lt.assign(hat, TRIM)
    for v in hat.data.vertices:
        r = math.hypot(v.co.x, v.co.y)
        a = math.atan2(v.co.y, v.co.x)
        if r > 0.1 and v.co.z < 0.02:                  # the brim curls up at the sides (x), down at front and back
            f = (r - 0.1) / 0.13
            v.co.z += 0.045 * f * f * (abs(math.cos(a)) ** 2) - 0.008 * f * abs(math.sin(a))
        if v.co.z > 0.1:                               # the crown's pinch along its top, a dent each side in front
            v.co.z -= 0.022 * math.exp(-(v.co.x / 0.03) ** 2) * min(1.0, (v.co.z - 0.1) / 0.03)
        if v.co.z > 0.05 and v.co.y < -0.03:
            v.co.x *= 1.0 - 0.18 * math.exp(-((v.co.z - 0.1) / 0.03) ** 2)
    hat.data.update()
    obj = named(hat, name)
    lp.place(obj, (0.0, 0.0, 0.0), (rnd.uniform(-4, 4), rnd.uniform(-3, 3), 0.0))
    obj.data.transform(Matrix.Translation((0.0, 0.0, -lowest([obj]) - 0.006)))
    return finish(obj, ao=0.2, smooth=45.0, collision='None')


def relic_boot(name='Relic_Boot', seed=72):
    """A cowboy boot lying on its side: the shaft, the foot to a pointed toe, the underslung heel, the sole, a spur
    strapped on. Leather on the trim sheet's dark beam strip, iron on its strap strip."""
    path = [(0.0, 0.03, 0.34), (0.0, 0.025, 0.22), (0.0, 0.02, 0.11), (0.0, 0.0, 0.065), (0.0, -0.06, 0.045),
            (0.0, -0.13, 0.038), (0.0, -0.2, 0.03), (0.0, -0.245, 0.024)]
    radii = [0.062, 0.058, 0.054, 0.06, 0.052, 0.04, 0.03, 0.014]
    squash = [(0.85, 1.0), (0.88, 1.0), (0.9, 1.0), (0.85, 0.95), (0.65, 0.95), (0.55, 0.95), (0.5, 0.9), (0.6, 0.8)]
    leather = tube(path, radii, 9, squash=squash, up=Vector((0.0, -1.0, 0.0)), caps=(0.15, 0.5))
    # Flatten the foot's sole and open the shaft's top a little (it is a boot, not a sausage).
    for v in leather.data.vertices:
        if v.co.z < 0.03 and v.co.y < 0.03:
            v.co.z = 0.03 + (v.co.z - 0.03) * 0.25
    leather.data.update()
    lp.grain(leather, 'Beams', axis=(0.0, 0.0, 1.0), seed=seed)
    sole = extrude([(-0.045, 0.04), (0.045, 0.04), (0.04, -0.12), (0.02, -0.235), (0.0, -0.255), (-0.02, -0.235),
                    (-0.04, -0.12)], 0.012, 0.028)
    lp.grain(sole, 'Beams', axis=(0.0, 1.0, 0.0), seed=seed + 1, slot=3)
    heel = extrude([(-0.035, 0.05), (0.035, 0.05), (0.03, -0.01), (-0.03, -0.01)], -0.03, 0.016)
    for v in heel.data.vertices:
        if v.co.z < -0.02:
            v.co.y -= 0.012
    heel.data.update()
    lp.grain(heel, 'Beams', axis=(0.0, 0.0, 1.0), seed=seed + 2, slot=3)
    strap = tube([(math.cos(a) * 0.066, 0.03 + math.sin(a) * 0.05, 0.075) for a in np.linspace(-1.2, 4.34, 7)],
                 [0.007] * 7, 4, squash=[(1.0, 0.45)] * 7, caps=(0.0, 0.0))
    lp.grain(strap, 'Iron', axis=(1.0, 0.0, 0.0), seed=seed + 3)
    shank = tube([(0.0, 0.11, 0.06), (0.0, 0.15, 0.055)], [0.006, 0.005], 4, caps=(0.0, 0.3))
    lp.grain(shank, 'Iron', axis=(0.0, 1.0, 0.0), seed=seed + 4)
    rowel = []
    for k in range(6):
        a = math.pi * k / 6
        spoke = lp.block((0.004, 0.05, 0.004), (0.0, 0.17, 0.055), (math.degrees(a), 0.0, 0.0))
        rowel.append(lp.grain(spoke, 'Iron', axis=(0.0, 1.0, 0.0), seed=seed + 5 + k))
    parts = [leather, sole, heel, strap, shank] + rowel
    for p in parts:
        lt.assign(p, TRIM) if not p.data.materials else None
    transform(parts, Matrix.Rotation(math.radians(-82.0), 4, 'Y'))              # onto its side
    transform(parts, Matrix.Rotation(math.radians(20.0), 4, 'Z'))
    pts = np.array(points_of(parts))
    lo_, hi_ = pts.min(axis=0), pts.max(axis=0)
    transform(parts, Matrix.Translation((-(lo_[0] + hi_[0]) * 0.5, -(lo_[1] + hi_[1]) * 0.5, -lo_[2] - 0.008)))
    obj = lp.join(name, parts)
    return finish(obj, ao=0.2, smooth=50.0, collision='None')


# --- The kit ---

BUILDERS = [
    ('Bones_PileA', bones_pile_a), ('Bones_PileB', larder), ('Bones_Scatter', bones_scatter),
    ('CattleSkull', cattle_skull),
    ('Ribcage', ribcage), ('Cocoon_Lying', cocoon_lying), ('Cocoon_Hung', cocoon_hung),
    ('CoffinBoards', coffin_boards), ('Relic_Hat', relic_hat), ('Relic_Boot', relic_boot),
]
wanted = [a for a in ARGS if a in dict(BUILDERS)]
models = [build() for name, build in BUILDERS if not wanted or name in wanted]
for model in models:
    report(model)


# --- The staged placement (DenDressing.placement.json beside this file) ---

# Where the dressing goes, as staged in the previews: in SOCKET_DenMouth's space of SM_DenRock (x right looking in,
# y into the den, z up from the den floor at the mouth), metres, yaw in degrees about z (Blender's: counterclockwise
# seen from above; a yaw of 0 keeps the model's front, -Y, facing out of the den), and whether it is mirrored (its x
# scaled by -1, so a repeated scatter doesn't repeat). z is the floor (or ceiling) height measured on the export of
# SM_DenRock under each pivot; out on the Sink floor (y < -1) it is the Sink's floor, 5 cm under the mouth's. The
# Gravemother's way down the den's middle and out across the Sink floor (|x| < 3, on the floor) is kept clear; the
# larder lies in the pocket's right side, off her body as she rests at SOCKET_Den (0, 7.3), which is narrower there.
PLACEMENT = [
    # Out on the Sink floor: a trail of bones leading to the mouth along both edges of her way.
    ('Bones_Scatter', -4.2, -9.2, -0.05, 70.0, 'the trail in: furthest out, left', False),
    ('Bones_Scatter', 4.3, -8.6, -0.05, 104.0, 'the trail in: furthest out, right (mirrored)', True),
    ('Bones_Scatter', -3.7, -6.6, -0.05, 95.0, 'the trail in, left (mirrored)', True),
    ('Bones_Scatter', 3.75, -6.0, -0.05, 82.0, 'the trail in, right', False),
    # The mouth's left side.
    ('CattleSkull', -5.0, -3.3, -0.05, 35.0, "the mouth's left side, out on the Sink floor, facing the Sink"),
    ('Bones_PileA', -6.1, -2.5, -0.05, -75.0, 'beside the skull, its back against the talus block at the face', False),
    ('Bones_Scatter', -4.4, -4.5, -0.05, 30.0, 'spilling out in front of the skull', False),
    ('Bones_Scatter', -3.65, -1.3, -0.03, 88.0, 'the trail on over the threshold, at the left jamb (mirrored)', True),
    ('Cocoon_Hung', -3.45, -0.8, 4.47, 0.0, "hung from the brow over the mouth's left corner (z: its pivot)", False),
    ('Cocoon_Hung', -1.2, -0.3, 6.62, 60.0, "hung in the notch at the top of the arch: its bottom 4.7 m up, over her",
     False),
    # The mouth's right side.
    ('Ribcage', 5.6, -3.0, -0.05, -15.0, "the mouth's right side, out on the Sink floor", False),
    ('Relic_Boot', 4.3, -3.7, -0.05, 40.0, 'by the ribcage', False),
    ('Bones_Scatter', 6.4, -4.4, -0.05, -20.0, 'in front of the ribcage', False),
    ('Cocoon_Hung', 3.6, -1.4, 4.01, 140.0, "hung from the brow over the mouth's right corner (z: its pivot)", False),
    ('CoffinBoards', 3.6, -0.1, 0.0, 96.0, 'dragged in past the right jamb, half over the threshold', False),
    ('Relic_Hat', 3.51, -0.71, 0.04, 15.0, "set on the coffin lid's head half", False),
    # Inside, the left wall foot.
    ('Cocoon_Lying', -3.45, 1.4, 0.08, 90.0, 'the left wall foot just inside the mouth, head into the den', False),
    ('Bones_PileA', -3.45, 3.95, 0.12, 90.0, 'the left wall foot past the fallen block, its back to the wall', False),
    ('Bones_Scatter', -3.56, 5.6, 0.16, 92.0, 'the left wall foot further in (mirrored)', True),
    # Inside, the right wall foot.
    ('Bones_PileA', 3.65, 2.3, 0.05, -90.0, 'the right wall foot, its back to the wall', False),
    ('Bones_Scatter', 3.62, 4.7, 0.12, 90.0, 'the right wall foot further in', False),
    # The pocket.
    ('Bones_PileB', 2.2, 6.85, 0.19, -90.0, "the larder: the pocket's right side, its front toward the den's middle, "
     "off her body as she rests at SOCKET_Den", False),
]


def write_placement(path):
    rows = []
    for row in PLACEMENT:
        asset, x, y, z, yaw, note = row[:6]
        mirror = len(row) > 6 and row[6]
        rows.append({
            'asset': 'SM_' + asset,
            'blender_m': [round(x, 3), round(y, 3), round(z, 3)],
            'yaw_blender_deg': yaw,
            'scale_blender': [-1.0 if mirror else 1.0, 1.0, 1.0],
            'unreal_cm': [round(-y * 100.0, 1), round(-x * 100.0, 1), round(z * 100.0, 1)],
            'yaw_unreal_deg': -yaw,
            'scale_unreal': [1.0, -1.0 if mirror else 1.0, 1.0],
            'note': note,
        })
    data = {
        'space': 'SOCKET_DenMouth of SM_DenRock (Intermediate/ArtExport_RR_DenRock/SM_DenRock.fbx)',
        'axes_blender': '+X right looking into the den, +Y into the den, +Z up from the den floor at the mouth; '
                        'yaw counterclockwise from above; yaw 0 = the model faces out of the den (its -Y); '
                        'scale -1 on x = mirrored (applied before the yaw)',
        'axes_unreal': 'the socket\'s own space in Unreal: +X out of the den (the socket\'s forward), +Y to the '
                       'right looking out, +Z up; cm; yaw as Unreal\'s (clockwise from above); a mirrored piece '
                       'has its actor scale Y = -1',
        'clear_way': 'nothing on the floor within |x| < 3 m (Blender) down the den and out across the Sink floor, '
                     'her way; the hung cocoon in the arch hangs over it with its bottom 4.7 m up',
        'gravemother': 'rests at SOCKET_Den (0, 7.3) facing out; her body is |x| < 0.8 m, y 6.3 to 9.7 m at 1.8x; '
                       'the larder (x 1.4 to 3.0) keeps 0.6 m off it',
        'placements': rows,
    }
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(data, f, indent=2)
        f.write('\n')
    print(f'DEN: placement: {len(rows)} pieces: {path}', flush=True)


if not wanted:
    write_placement(os.path.join(lt.REPO, 'Art', 'Models', 'Props', 'DenDressing.placement.json'))


# --- Previews: the kit side by side, and close looks at the hero pieces ---

def bone_test(path):
    """The bone material picked by test: the skull and a scatter in BoneDry (Polymer, picked) beside copies mapped on
    the other light sets a tint could make bone of (the house trim's plaster strip, BarkBirch, StoneWall), in the
    preview's sun and again dimmed as in the den."""
    skull = next(o for o in models if o.name == 'CattleSkull')
    scatter = next(o for o in models if o.name == 'Bones_Scatter')
    tests = [('BoneDry', BONE, 'Polymer'),
             ('Plaster', TRIM, None),
             ('Birch', lt.material('BarkBirch', name='_TestBirch', tint=0xf2ead8), 'BarkBirch'),
             ('StoneWall', lt.material('StoneWall', name='_TestStone', tint=0xffffff), 'StoneWall')]
    shown = []
    for k, (label, mat, tile) in enumerate(tests):
        for src, dy in ((skull, 0.0), (scatter, -1.3)):
            copy = src.copy()
            copy.data = src.data.copy()
            copy.name = 'Test_' + label + src.name          # (names starting with _ are hidden)
            bpy.context.scene.collection.objects.link(copy)
            index = copy.data.materials.find('BoneDry')
            if k:
                copy.data.materials[index] = mat
                faces = [p.index for p in copy.data.polygons if p.material_index == index]
                if tile:
                    lt.box_uv(copy, tile, faces=faces, seed=k)
                else:                                  # the plaster strip's clean stretch (Graves.py's whitewash)
                    lt.trim_uv(copy, faces, 'G', u_offset=4.32)
            copy.location = (k * 1.7, dy, 0.0)
            shown.append(copy)
    bpy.context.view_layer.update()
    lt.preview(shown, path, view=(-0.25, -1.6, 0.7), fit=0.62, resolution=(1800, 900), ground_at=0.0)
    lt.preview(shown, path.replace('.png', '_Dim.png'), view=(-0.25, -1.6, 0.7), fit=0.62, resolution=(1800, 900),
               ground_at=0.0, sun_strength=0.6)
    for copy in shown:
        bpy.data.objects.remove(copy, do_unlink=True)


def game_occlusion(materials):
    """The previews darken the colour by the vertex occlusion as the masters do: by DiffuseAO of it (0.4 unless the
    material sets it), not all of it, so grime and creases show as strong as they will in the game, no stronger."""
    for mat in materials:
        if mat is None or not mat.use_nodes or mat.get('Master') not in ('World', 'WorldFoliage'):
            continue
        for node in mat.node_tree.nodes:
            if node.type == 'MIX' and node.blend_type == 'MULTIPLY' and tuple(node.location) == (100.0, 250.0):
                node.inputs[0].default_value = float(mat.get('DiffuseAO', 0.4))


if lt.want_preview():
    for node in WEB.node_tree.nodes:           # previews clip where Unreal does (1/3), not at a half
        if node.type == 'MATH' and node.operation == 'GREATER_THAN':
            node.inputs[1].default_value = 1.0 / 3.0
    game_occlusion([WEB, BONE, TRIM])
    for m in models:
        if m.name == 'Cocoon_Hung':             # stood on the ground beside the rest for the overview
            m.location.z = -min(Vector(c).z for c in m.bound_box)
    rows = [('Bones_PileB', 'Bones_PileA', 'Bones_Scatter', 'CoffinBoards'),
            ('Ribcage', 'CattleSkull', 'Cocoon_Lying', 'Cocoon_Hung', 'Relic_Hat', 'Relic_Boot')]
    by_name = {m.name: m for m in models}
    y = 0.0
    for names in rows:
        row = [by_name[n] for n in names if n in by_name]
        x = 0.0
        for obj in row:
            lo_ = min((obj.matrix_world @ Vector(c)).x for c in obj.bound_box)
            hi_ = max((obj.matrix_world @ Vector(c)).x for c in obj.bound_box)
            obj.location.x += x - lo_
            x += hi_ - lo_ + 0.5
        for obj in row:
            obj.location.x -= (x - 0.5) * 0.5
            obj.location.y = y
        y -= 2.2
    bpy.context.view_layer.update()
    front_lo = min([(o.matrix_world @ Vector(c)).x for n in rows[-1] if n in by_name for o in [by_name[n]]
                    for c in o.bound_box] or [0.0])
    figure = lm.box('ScaleFigure', (0.5, 0.3, 1.8), (front_lo - 0.7, y + 2.2, 0.9),
                    bpy.data.materials.get('ScaleFigure') or lm.material('ScaleFigure', 0x6c7a8a))
    lt.preview(models + [figure], lt.preview_path(PREVIEW, 'Kit'), view=(-0.1, -1.0, 1.25), fit=0.58,
               resolution=(1800, 1100), ground_at=0.0)
    bpy.data.objects.remove(figure, do_unlink=True)
    for obj in models:
        obj.location = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    for name, view, fit in (('CattleSkull', (-0.35, -1.6, 0.55), 0.8), ('Ribcage', (-0.9, -1.4, 0.6), 0.8),
                            ('Cocoon_Lying', (-0.5, -1.5, 0.8), 0.8), ('Cocoon_Hung', (-0.4, -1.6, 0.3), 0.85),
                            ('Bones_PileA', (-0.3, -1.5, 0.9), 0.75), ('Bones_PileB', (-0.5, -1.5, 0.9), 0.75),
                            ('Bones_Scatter', (-0.4, -1.5, 1.0), 0.8), ('CoffinBoards', (-0.6, -1.5, 0.9), 0.8),
                            ('Relic_Hat', (-0.4, -1.5, 0.8), 0.9), ('Relic_Boot', (-0.4, -1.5, 0.8), 0.9)):
        obj = by_name.get(name)
        if obj is None:
            continue
        lt.preview([obj], lt.preview_path(PREVIEW, name), view=view, fit=fit, ground=name != 'Cocoon_Hung',
                   ground_at=0.0)
    if 'CattleSkull' in by_name and 'Bones_Scatter' in by_name:
        bone_test(lt.preview_path(PREVIEW, 'BoneMaterial_Test'))
