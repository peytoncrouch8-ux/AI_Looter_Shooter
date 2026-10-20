"""Gauge's geometry kit (Art/Models/Characters/Gauge.py, Tools/Blender/gauge_parts.py): closed hard-surface solids built
in numpy (and bmesh for bevels and letters), as Shapes that are moved into place and joined before they become Blender
objects. Every builder returns a Shape at the origin; Shape.at(M) places it with a 4x4 matrix (numpy).

    sh = cyl(0.06, 0.3, 24, b=0.008, seg=2).at(frame(p, axis))    # a bevelled cylinder along axis from p
    sh += sphere(0.07, 24, 12).at(T(q))

Units metres. Faces are fixed outward when the object is made (each part is a set of closed solids), so the builders
needn't care about winding.
"""
import math

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector


def unit(v):
    v = np.asarray(v, dtype=np.float64)
    n = np.linalg.norm(v)
    return v / n if n > 1e-12 else v


def mat(x, y, z, o):
    M = np.eye(4)
    M[:3, 0], M[:3, 1], M[:3, 2], M[:3, 3] = x, y, z, o
    return M


def frame(o, z, hint=(0.0, -1.0, 0.0)):
    """A frame at o: its z along z, its x toward hint (square to z), y = z cross x."""
    z = unit(z)
    x = np.asarray(hint, dtype=np.float64)
    x = x - z * (x @ z)
    if np.linalg.norm(x) < 1e-6:
        x = np.cross((0.0, 1.0, 0.0) if abs(z[1]) < 0.9 else (1.0, 0.0, 0.0), z)
    x = unit(x)
    return mat(x, np.cross(z, x), z, np.asarray(o, dtype=np.float64))


def T(o):
    M = np.eye(4)
    M[:3, 3] = o
    return M


def R(axis, deg):
    return np.array(Matrix.Rotation(math.radians(deg), 4, Vector(axis)))


def S(sx, sy, sz):
    return np.diag([sx, sy, sz, 1.0])


def perp(v, axis):
    """v with its part along axis taken out, unit."""
    axis = unit(axis)
    v = np.asarray(v, dtype=np.float64)
    return unit(v - axis * (v @ axis))


class Shape:
    """Vertices (n, 3) and faces (lists of vertex indices)."""

    def __init__(self, V=(), F=()):
        self.V = np.asarray(V, dtype=np.float64).reshape(-1, 3)
        self.F = [list(f) for f in F]

    def __iadd__(self, o):
        n = len(self.V)
        self.V = np.concatenate([self.V, o.V])
        self.F += [[i + n for i in f] for f in o.F]
        return self

    def __add__(self, o):
        s = Shape(self.V.copy(), self.F)
        s += o
        return s

    def at(self, M):
        M = np.asarray(M, dtype=np.float64)
        return Shape(self.V @ M[:3, :3].T + M[:3, 3], self.F)

    def tris(self):
        return sum(len(f) - 2 for f in self.F)


def join(shapes):
    out = Shape()
    for s in shapes:
        out += s
    return out


def instances(shape, mats):
    """Copies of one shape at many 4x4 matrices, as one shape (rivets)."""
    if not len(mats):
        return Shape()
    mats = np.asarray(mats, dtype=np.float64)
    V = np.einsum('kij,nj->kni', mats[:, :3, :3], shape.V) + mats[:, None, :3, 3]
    n = len(shape.V)
    F = [[i + k * n for i in f] for k in range(len(mats)) for f in shape.F]
    return Shape(V.reshape(-1, 3), F)


# ------------------------------------------------------------------------------------------------------- primitives

def lathe(profile, n=24, cap=True, closed=False):
    """A surface of revolution about +z: profile [(r, z), ...] (bottom to top, or a closed loop); a point with r 0 is a
    pole. Open ends with r > 0 get an n-gon cap when cap."""
    t = np.linspace(0.0, 2.0 * math.pi, n, endpoint=False)
    c, s = np.cos(t), np.sin(t)
    V, F, ids = [], [], []
    for r, z in profile:
        if r <= 1e-7:
            ids.append([len(V)] * n)
            V.append((0.0, 0.0, z))
        else:
            ids.append(list(range(len(V), len(V) + n)))
            V.extend(zip(r * c, r * s, np.full(n, z)))
    pairs = list(zip(ids, ids[1:])) + ([(ids[-1], ids[0])] if closed else [])
    for a, b in pairs:
        if a[0] == a[1] and b[0] == b[1]:
            continue
        for k in range(n):
            k1 = (k + 1) % n
            f = [a[k], a[k1], b[k1], b[k]]
            f = [v for i, v in enumerate(f) if v != f[i - 1]]
            if len(set(f)) >= 3:
                F.append(f)
    if cap and not closed:
        if ids[0][0] != ids[0][1]:
            F.append(ids[0][::-1])
        if ids[-1][0] != ids[-1][1]:
            F.append(ids[-1])
    return Shape(V, F)


def arc(cr, cz, r, a0, a1, k):
    """k points on a circle in the (r, z) plane, angles in degrees."""
    return [(cr + r * math.cos(math.radians(a)), cz + r * math.sin(math.radians(a))) for a in np.linspace(a0, a1, k)]


def rounded(profile, b, seg):
    """A (r, z) profile [(r0, z0), (r1, z1), ...] up a side, its two ends' rims rounded by b: the end rings turn in to
    r - b over a quarter round (the caps close them)."""
    if b <= 0.0:
        return list(profile)
    (r0, z0), (r1, z1) = profile[0], profile[-1]
    return arc(r0 - b, z0 + b, b, -90, 0, seg + 1) + list(profile[1:-1]) + arc(r1 - b, z1 - b, b, 0, 90, seg + 1)


def cyl(r, h, n=24, b=0.0, seg=2, r1=None):
    """A cylinder (a cone with r1) along +z from 0 to h, its rims rounded by b."""
    return lathe(rounded([(r, 0.0), (r if r1 is None else r1, h)], b, seg), n)


def housing(radii, n=24, b=0.008, seg=2):
    """A limb's casing along +z: radii [(z, r), ...] bottom to top, its ends' rims rounded."""
    return lathe(rounded([(r, z) for z, r in radii], b, seg), n)


def sphere(r, n=24, rings=12):
    return lathe(arc(0.0, 0.0, r, -90, 90, rings + 1), n)


def torus(R_, r, n=40, m=8):
    return lathe(arc(R_, 0.0, r, 0, 360, m + 1)[:-1], n, cap=False, closed=True)


def ring(r_in, r_out, h, n=40, b=0.0015):
    """A flat band (an annulus h tall along +z, from 0), its outer edges rounded."""
    prof = [(r_in, 0.0)] + rounded([(r_out, 0.0), (r_out, h)], b, 1) + [(r_in, h)]
    return lathe(prof, n, cap=False, closed=True)


def dome_shell(rx, ry, rz, t, n=40, rings=10, lip=0.0):
    """Half an ellipsoid opening down (rx, ry across, rz up), a shell t thick, its rim rounded; lip > 0 runs the
    outside straight down that far first (a skirt)."""
    a = np.linspace(0.0, math.pi / 2, rings + 1)
    outer = ([(rx, -lip)] if lip > 0 else []) + [(rx * math.cos(x), rz * math.sin(x)) for x in a]
    inner = [((rx - t) * math.cos(x), (rz - t) * math.sin(x)) for x in a[::-1]] + ([(rx - t, -lip)] if lip > 0 else [])
    outer[-1], inner[0] = (0.0, rz), (0.0, rz - t)      # the two poles
    return lathe(outer + inner, n, cap=False, closed=True).at(S(1.0, ry / rx, 1.0))


def superellipse(hw, hd, cy, z, t, p=3.2, cx=0.0):
    c, s = np.cos(t), np.sin(t)
    return np.stack([cx + hw * np.sign(c) * np.abs(c) ** (2.0 / p), cy + hd * np.sign(s) * np.abs(s) ** (2.0 / p),
                     np.broadcast_to(np.asarray(z, dtype=np.float64), np.shape(t))], 1)


def loft(rings, cap0=True, cap1=True, closed=False):
    """Rings (each (n, 3)) in order; quads between neighbours; n-gon caps on the ends (or the last joined back to the
    first when closed)."""
    n = len(rings[0])
    V = np.concatenate(rings)
    F = []
    pairs = [(j, j + 1) for j in range(len(rings) - 1)] + ([(len(rings) - 1, 0)] if closed else [])
    for a, b in pairs:
        for k in range(n):
            F.append([a * n + k, a * n + (k + 1) % n, b * n + (k + 1) % n, b * n + k])
    if not closed:
        if cap0:
            F.append(list(range(n))[::-1])
        if cap1:
            F.append(list(range((len(rings) - 1) * n, len(rings) * n)))
    return Shape(V, F)


def sheet(P, us, vs, thick, out=None):
    """A thick sheet: P(u, v) -> points (arrays broadcast), the front surface; the back thick in along its normal
    (turned to agree with out(points) -> outward vectors when given), the border closed."""
    U, Vv = np.meshgrid(us, vs, indexing='ij')
    front = P(U, Vv)                                   # (nu, nv, 3)
    du = np.gradient(front, axis=0)
    dv = np.gradient(front, axis=1)
    nrm = np.cross(du, dv)
    nrm /= np.maximum(np.linalg.norm(nrm, axis=2, keepdims=True), 1e-12)
    if out is not None and (nrm * out(front)).sum() < 0:
        nrm = -nrm
    back = front - nrm * thick
    nu, nv = len(us), len(vs)
    V = np.concatenate([front.reshape(-1, 3), back.reshape(-1, 3)])
    fi = lambda i, j: i * nv + j
    bi = lambda i, j: nu * nv + i * nv + j
    F = []
    for i in range(nu - 1):
        for j in range(nv - 1):
            F.append([fi(i, j), fi(i + 1, j), fi(i + 1, j + 1), fi(i, j + 1)])
            F.append([bi(i, j), bi(i, j + 1), bi(i + 1, j + 1), bi(i + 1, j)])
    border = [(i, 0) for i in range(nu)] + [(nu - 1, j) for j in range(1, nv)] + \
             [(i, nv - 1) for i in range(nu - 2, -1, -1)] + [(0, j) for j in range(nv - 2, 0, -1)]
    for a, b in zip(border, border[1:] + border[:1]):
        F.append([fi(*a), fi(*b), bi(*b), bi(*a)])
    return Shape(V, F)


def _from_bm(bm):
    bm.verts.index_update()
    V = [v.co[:] for v in bm.verts]
    F = [[v.index for v in f.verts] for f in bm.faces]
    bm.free()
    return Shape(V, F)


def box(half, taper=(1.0, 1.0), b=0.0, seg=2):
    """A box of half-sizes half centred on the origin, its top face scaled by taper, its edges rounded by b."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=2.0)
    for v in bm.verts:
        k = taper if v.co.z > 0 else (1.0, 1.0)
        v.co = Vector((v.co.x * half[0] * k[0], v.co.y * half[1] * k[1], v.co.z * half[2]))
    if b > 0:
        bmesh.ops.bevel(bm, geom=list(bm.edges), offset=min(b, 0.98 * min(half)), offset_type='OFFSET',
                        segments=seg, profile=0.5, affect='EDGES', clamp_overlap=True)
    return _from_bm(bm)


def prism(outline, depth, b=0.0, seg=1):
    """A flat outline [(x, y), ...] (counterclockwise) extruded depth along z, centred, its edges rounded by b."""
    bm = bmesh.new()
    vs = [bm.verts.new((x, y, -depth / 2)) for x, y in outline]
    f = bm.faces.new(vs)
    res = bmesh.ops.extrude_face_region(bm, geom=[f])
    top = [e for e in res['geom'] if isinstance(e, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, vec=(0.0, 0.0, depth), verts=top)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    if b > 0:
        bmesh.ops.bevel(bm, geom=list(bm.edges), offset=b, offset_type='OFFSET', segments=seg, profile=0.5,
                        affect='EDGES', clamp_overlap=True)
    return _from_bm(bm)


def text(body, size, extrude):
    """Raised letters (Blender's own font) in the xy plane, facing +z, centred."""
    cu = bpy.data.curves.new('_letters', 'FONT')
    cu.body, cu.size, cu.extrude, cu.align_x, cu.align_y = body, size, extrude, 'CENTER', 'CENTER'
    ob = bpy.data.objects.new('_letters', cu)
    bpy.context.scene.collection.objects.link(ob)
    me = bpy.data.meshes.new_from_object(ob.evaluated_get(bpy.context.evaluated_depsgraph_get()))
    bpy.data.objects.remove(ob)
    bpy.data.curves.remove(cu)
    V = [v.co[:] for v in me.vertices]
    F = [list(p.vertices) for p in me.polygons]
    bpy.data.meshes.remove(me)
    return Shape(V, F)


def tube(points, r, n=12, steps=6):
    """A round tube along a path (Catmull-Rom through points), capped."""
    P = np.asarray(points, dtype=np.float64)
    ext = np.concatenate([P[:1] * 2 - P[1:2], P, P[-1:] * 2 - P[-2:-1]])
    pts = []
    for i in range(1, len(ext) - 2):
        p0, p1, p2, p3 = ext[i - 1:i + 3]
        for t in np.linspace(0.0, 1.0, steps, endpoint=(i == len(ext) - 3)):
            pts.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
                              (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3))
    pts = np.array(pts)
    tan = np.gradient(pts, axis=0)
    rings = []
    x = perp((0.0, 0.0, 1.0) if abs(unit(tan[0])[2]) < 0.9 else (1.0, 0.0, 0.0), tan[0])
    a = np.linspace(0.0, 2.0 * math.pi, n, endpoint=False)
    for p, d in zip(pts, tan):
        d = unit(d)
        x = perp(x, d)                                 # carried along: no twist
        y = np.cross(d, x)
        rings.append(p + r * (np.cos(a)[:, None] * x + np.sin(a)[:, None] * y))
    return loft(rings)


def rivet(r, n=8):
    """A round-headed rivet on +z: the head r across, 0.6 r high, a short shank into the plate."""
    return lathe([(r * 0.9, -0.5 * r)] + [(r * math.cos(a), 0.6 * r * math.sin(a))
                                          for a in np.radians([0.0, 35.0, 65.0, 90.0])], n)


def rivets(points, normals, r, n=8):
    """Rivets standing on their normals."""
    mats = [frame(p, nm, (1.0, 0.0, 0.0) if abs(unit(nm)[0]) < 0.9 else (0.0, 1.0, 0.0))
            for p, nm in zip(points, normals)]
    return instances(rivet(r, n), mats)


def spline(xs, ys, x):
    """A smooth curve through (xs, ys) (cubic Hermite, central-difference slopes) at x (clamped)."""
    xs, ys = np.asarray(xs, dtype=np.float64), np.asarray(ys, dtype=np.float64)
    x = np.clip(np.asarray(x, dtype=np.float64), xs[0], xs[-1])
    m = np.gradient(ys, xs)
    i = np.clip(np.searchsorted(xs, x) - 1, 0, len(xs) - 2)
    h = xs[i + 1] - xs[i]
    t = (x - xs[i]) / h
    h00, h10, h01, h11 = 2 * t ** 3 - 3 * t ** 2 + 1, t ** 3 - 2 * t ** 2 + t, -2 * t ** 3 + 3 * t ** 2, t ** 3 - t ** 2
    return h00 * ys[i] + h10 * h * m[i] + h01 * ys[i + 1] + h11 * h * m[i + 1]


def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=np.float64) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


# ------------------------------------------------------------------------------------------------ into Blender

def make_object(name, shape, material=None, sharp=45.0, weighted=False):
    """The shape as a mesh object at the origin (object space = world): faces turned outward, shaded smooth with
    edges over sharp degrees hard; weighted: face-area weighted normals (the game mesh's flat faces stay flat)."""
    bm = bmesh.new()
    vs = [bm.verts.new(tuple(p)) for p in shape.V]
    for f in shape.F:
        try:
            bm.faces.new([vs[i] for i in f])
        except ValueError:                             # a face already there
            pass
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.dissolve_degenerate(bm, dist=1e-7, edges=bm.edges[:])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    me.set_sharp_from_angle(angle=math.radians(sharp))
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    if material is not None:
        me.materials.append(material)
    if weighted:
        m = ob.modifiers.new('weighted', 'WEIGHTED_NORMAL')
        m.mode, m.weight, m.keep_sharp = 'FACE_AREA', 50, True
        bpy.context.view_layer.objects.active = ob
        for o in bpy.context.selected_objects:
            o.select_set(False)
        ob.select_set(True)
        bpy.ops.object.modifier_apply(modifier=m.name)
    return ob
