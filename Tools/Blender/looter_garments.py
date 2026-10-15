"""Clothes for the MakeHuman-based characters (looter_human.py), built as geometry in numpy and handed to Blender.

Two ways of making cloth, both from Art/Portraits/EllisBlender.py's bust and grown for a whole body:
- drape(): cloth hung round a vertical axis. Each column (an angle round the axis) runs down from a top line; at every
  height it sits just outside the widest part of what's under it at or above that height, so it tents over shoulders and
  hips and hangs straight below them, as heavy cloth does (a coat's skirt, a cape, a bandana's bib).
- shell(): part of the body pushed out along its normals and smoothed until the anatomy is gone (sleeves, a waistcoat,
  trousers). MakeHuman's UVs come with it.
Then clear() pushes anything that dips under the layers beneath back out, so nothing pokes through at rest.

Every builder returns plain arrays (verts, faces, uvs per face corner); mesh() makes the Blender object. Units are
metres, Z up, the front -Y.
"""
import math

import bpy  # first: as a Python module (the cloud box's bpy) bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree


def unit(v):
    v = np.asarray(v, dtype=np.float64)
    n = np.linalg.norm(v, axis=-1, keepdims=True)
    return v / np.maximum(n, 1e-12)


def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=np.float64) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


# ------------------------------------------------------------------------------------------------------------ meshes

class Piece:
    """A garment piece: verts (n, 3), faces (lists), uv (one (u, v) per face corner, in face order) or None."""

    def __init__(self, verts, faces, uv=None):
        self.verts = np.asarray(verts, dtype=np.float64)
        self.faces = [list(f) for f in faces]
        self.uv = uv

    def __add__(self, other):
        off = len(self.verts)
        uv = None
        if self.uv is not None and other.uv is not None:
            uv = list(self.uv) + list(other.uv)
        return Piece(np.concatenate([self.verts, other.verts]), self.faces + [[i + off for i in f] for f in other.faces], uv)


def mesh(name, piece, material=None, smooth=True):
    """A Blender object from a Piece (UV map 'UVMap' if it has UVs)."""
    me = bpy.data.meshes.new(name)
    me.from_pydata([tuple(p) for p in piece.verts], [], [tuple(f) for f in piece.faces])
    if piece.uv is not None:
        layer = me.uv_layers.new(name='UVMap')
        layer.data.foreach_set('uv', np.asarray(piece.uv, dtype=np.float32).ravel())
    me.validate(clean_customdata=False)
    me.update()
    if smooth:
        me.shade_smooth()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    if material is not None:
        ob.data.materials.append(material)
    return ob


def grid(X, Y, Z, wrap=False, close_v=False):
    """A Piece from (rows, cols) coordinate grids: rows run down (v), columns round (u). UVs are arc lengths in metres
    (u along the middle row, v down the middle column), so a later packing keeps the texel density even."""
    rows, cols = X.shape
    P = np.stack([X, Y, Z], axis=-1)
    verts = P.reshape(-1, 3)
    mid = P[rows // 2]
    du = np.linalg.norm(np.diff(np.concatenate([mid, mid[:1]]) if wrap else mid, axis=0), axis=1)
    u = np.concatenate([[0.0], np.cumsum(du)])
    midc = P[:, cols // 2]
    v = -np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(midc, axis=0), axis=1))])
    faces, uv = [], []
    for j in range(rows - 1):
        for i in range(cols if wrap else cols - 1):
            i2 = (i + 1) % cols
            a, b, c, d = j * cols + i, j * cols + i2, (j + 1) * cols + i2, (j + 1) * cols + i
            faces.append([a, d, c, b])
            uv += [(u[i], v[j]), (u[i], v[j + 1]), (u[i + 1], v[j + 1]), (u[i + 1], v[j])]
    return Piece(verts, faces, uv)


def tube(path, radii, n=16, up=(0.0, 0.0, 1.0), cap0=False, cap1=False, shape=None):
    """A lofted tube along path (k, 3) with radii (k,) or (k, 2) for an ellipse; shape(i, angles) -> radius scale."""
    path = np.asarray(path, dtype=np.float64)
    k = len(path)
    radii = np.asarray(radii, dtype=np.float64)
    if radii.ndim == 1:
        radii = np.stack([radii, radii], 1)
    t = np.gradient(path, axis=0)
    t = unit(t)
    ang = np.linspace(0, 2 * math.pi, n, endpoint=False)
    rings = []
    ref = np.asarray(up, dtype=np.float64)
    for i in range(k):
        a = unit(np.cross(t[i], ref) if abs(t[i] @ ref) < 0.95 else np.cross(t[i], [1.0, 0.0, 0.0]))
        b = np.cross(t[i], a)
        s = shape(i, ang) if shape is not None else 1.0
        rings.append(path[i] + (np.cos(ang)[:, None] * a * radii[i, 0] + np.sin(ang)[:, None] * b * radii[i, 1]) * np.reshape(s, (-1, 1)))
    R = np.array(rings)
    piece = grid(R[..., 0], R[..., 1], R[..., 2], wrap=True)
    verts, faces, uv = list(piece.verts), piece.faces, piece.uv
    for cap, row in ((cap0, 0), (cap1, k - 1)):
        if cap:
            c = len(verts)
            verts.append(path[row])
            for i in range(n):
                a, b = row * n + i, row * n + (i + 1) % n
                faces.append([a, b, c] if row == 0 else [b, a, c])
                uv += [(0.0, 0.0), (0.01, 0.0), (0.005, 0.01)]
    return flipped(Piece(np.array(verts), faces, uv))


def flipped(piece):
    """The same piece with every face turned over (tube() builds its rings inward-facing)."""
    faces, uv, k = [], [], 0
    for f in piece.faces:
        n = len(f)
        corners = piece.uv[k:k + n] if piece.uv is not None else None
        faces.append(list(reversed(f)))
        if corners is not None:
            uv.extend(reversed(corners))
        k += n
    return Piece(piece.verts, faces, uv if piece.uv is not None else None)


# --------------------------------------------------------------------------------------------------------- draping

def frame2d(fwd):
    f = np.array([fwd[0], fwd[1]], dtype=np.float64)
    f /= np.linalg.norm(f)
    return f, np.array([-f[1], f[0]])   # forward, the figure's left


def drape(pts, centre, fwd, thetas, z_top, z_bot, n_z, offset, hb=math.radians(3.0), zb=0.006, smooth=(2, 2), wrap=False,
          flare=None):
    """Cloth hung round a vertical axis at centre (x, y): columns at angles thetas from fwd (toward the figure's left),
    each from z_top(theta) down to z_bot(theta) in n_z rows, offset outside the widest part of pts at or above each
    height in its column. flare(theta, v) adds to the radius (v 0 at the top, 1 at the hem). Returns X, Y, Z, R."""
    f, l = frame2d(fwd)
    rel = pts[:, :2] - centre
    ang = np.arctan2(rel @ l, rel @ f)
    rad = np.linalg.norm(rel, axis=1)
    nt = len(thetas)
    R = np.zeros((n_z, nt))
    Z = np.zeros((n_z, nt))
    for i, th in enumerate(thetas):
        zt, zbo = z_top(th), z_bot(th)
        zs = np.linspace(zt, zbo, n_z)
        Z[:, i] = zs
        d = np.abs((ang - th + math.pi) % (2 * math.pi) - math.pi)
        sel = (d < hb) & (pts[:, 2] < zt + zb) & (pts[:, 2] > zbo - zb)
        if not sel.any():
            continue
        zp, rp = pts[sel, 2], rad[sel]
        col = np.zeros(n_z)
        dz = (zt - zbo) / max(n_z - 1, 1)
        idx = np.clip(np.round((zt - zp) / max(dz, 1e-6)).astype(int), 0, n_z - 1)
        np.maximum.at(col, idx, rp)
        R[:, i] = np.maximum.accumulate(col)
    for i in range(nt):
        if R[:, i].max() == 0:
            j = i - 1 if i > 0 else i + 1
            R[:, i] = R[:, j]
    for _ in range(smooth[0]):
        if wrap:
            R = 0.25 * np.roll(R, 1, 1) + 0.5 * R + 0.25 * np.roll(R, -1, 1)
        else:
            R = np.concatenate([R[:, :1], 0.25 * R[:, :-2] + 0.5 * R[:, 1:-1] + 0.25 * R[:, 2:], R[:, -1:]], 1)
    for _ in range(smooth[1]):
        R = np.concatenate([R[:1], 0.25 * R[:-2] + 0.5 * R[1:-1] + 0.25 * R[2:], R[-1:]], 0)
    R = R + offset
    if flare is not None:
        v = np.linspace(0.0, 1.0, n_z)[:, None]
        R = R + flare(thetas[None, :], v)
    cosT, sinT = np.cos(thetas), np.sin(thetas)
    X = centre[0] + R * (cosT * f[0] + sinT * l[0])
    Y = centre[1] + R * (cosT * f[1] + sinT * l[1])
    return X, Y, Z, R


def radial(thetas, fwd):
    """Unit outward directions (x, y) for column angles (any shape) about fwd."""
    f, l = frame2d(fwd)
    return np.cos(thetas) * f[0] + np.sin(thetas) * l[0], np.cos(thetas) * f[1] + np.sin(thetas) * l[1]


# --------------------------------------------------------------------------------------------------------- shells

def edges_of(faces, n):
    """Unique undirected edges (m, 2) of a face list."""
    e = []
    for f in faces:
        for k in range(len(f)):
            a, b = f[k], f[(k + 1) % len(f)]
            e.append((min(a, b), max(a, b)))
    return np.unique(np.array(e, dtype=np.int64), axis=0)


def laplacian(verts, faces, iters=10, lam=0.5, mu=-0.53, pinned=None, weight=None):
    """Taubin smoothing (shrink, then grow back): the shape's small bumps go, its volume stays. pinned (bool per vertex)
    stay put; weight (0..1 per vertex) scales the move."""
    v = np.asarray(verts, dtype=np.float64).copy()
    n = len(v)
    e = edges_of(faces, n)
    deg = np.bincount(e.ravel(), minlength=n).astype(np.float64)
    w = np.ones(n) if weight is None else np.asarray(weight, dtype=np.float64)
    if pinned is not None:
        w = np.where(pinned, 0.0, w)
    for _ in range(iters):
        for k in (lam, mu):
            acc = np.zeros_like(v)
            np.add.at(acc, e[:, 0], v[e[:, 1]])
            np.add.at(acc, e[:, 1], v[e[:, 0]])
            avg = acc / np.maximum(deg, 1)[:, None]
            v += (avg - v) * (k * w)[:, None]
    return v


def vertex_normals(verts, faces):
    n = np.zeros_like(verts)
    for f in faces:
        p = verts[f]
        c = np.zeros(3)
        for k in range(len(f)):
            c += np.cross(p[k], p[(k + 1) % len(f)])
        n[f] += c
    return unit(n)


def boundary(faces, n):
    """Boolean per vertex: on an open edge."""
    count = {}
    for f in faces:
        for k in range(len(f)):
            a, b = f[k], f[(k + 1) % len(f)]
            key = (min(a, b), max(a, b))
            count[key] = count.get(key, 0) + 1
    out = np.zeros(n, dtype=bool)
    for (a, b), c in count.items():
        if c == 1:
            out[a] = out[b] = True
    return out


def subset(verts, faces, uv_corners, keep_face):
    """The faces where keep_face[i] is True, with their vertices renumbered. uv_corners is a list of per-face lists."""
    idx = [i for i, k in enumerate(keep_face) if k]
    used = np.unique(np.concatenate([faces[i] for i in idx]))
    remap = -np.ones(len(verts), dtype=np.int64)
    remap[used] = np.arange(len(used))
    f2 = [[int(remap[j]) for j in faces[i]] for i in idx]
    uv = [tuple(c) for i in idx for c in uv_corners[i]] if uv_corners is not None else None
    return Piece(verts[used], f2, uv), used


def shell(verts, faces, uv_corners, keep_face, offset, iters=12, normal_from=None):
    """Part of a surface (the faces where keep_face) pushed out by offset (scalar or per original vertex) along its
    normals and smoothed. Returns (Piece, original vertex indices)."""
    piece, used = subset(verts, faces, uv_corners, keep_face)
    off = np.broadcast_to(np.asarray(offset, dtype=np.float64), (len(verts),))[used] if np.ndim(offset) else np.full(len(used), float(offset))
    nrm = vertex_normals(piece.verts, piece.faces) if normal_from is None else normal_from[used]
    piece.verts = piece.verts + nrm * off[:, None]
    if iters:
        piece.verts = laplacian(piece.verts, piece.faces, iters, pinned=None)
    return piece, used


# ------------------------------------------------------------------------------------------------------- clearance

def bvh_points(verts, faces):
    return BVHTree.FromPolygons([tuple(map(float, p)) for p in verts], [tuple(f) for f in faces])


def bvh_objects(obs):
    """One BVH over objects' evaluated meshes, world space."""
    dg = bpy.context.evaluated_depsgraph_get()
    bm = bmesh.new()
    for ob in obs:
        ev = ob.evaluated_get(dg)
        me = ev.to_mesh()
        tmp = bmesh.new()
        tmp.from_mesh(me)
        tmp.transform(ob.matrix_world)
        me2 = bpy.data.meshes.new('_tmp')
        tmp.to_mesh(me2)
        tmp.free()
        bm.from_mesh(me2)
        bpy.data.meshes.remove(me2)
        ev.to_mesh_clear()
    tree = BVHTree.FromBMesh(bm)
    bm.free()
    return tree


def clear(points, tree, gap, reach=0.08):
    """Pushes points that sit less than gap outside (or anywhere inside) the surface in tree back out to gap, along
    the surface's normal. Points farther than reach from it are left alone. Returns the new points."""
    out = np.array(points, dtype=np.float64)
    for k, p in enumerate(out):
        loc, nrm, _i, dist = tree.find_nearest(Vector(p), reach)
        if loc is None:
            continue
        d = (Vector(p) - loc).dot(nrm)
        if d < gap:
            out[k] = np.array(loc + nrm * gap)
    return out


def object_points(ob):
    """World-space vertices of ob with its modifiers."""
    dg = bpy.context.evaluated_depsgraph_get()
    ev = ob.evaluated_get(dg)
    me = ev.to_mesh()
    pts = np.empty(len(me.vertices) * 3, dtype=np.float64)
    me.vertices.foreach_get('co', pts)
    pts = pts.reshape(-1, 3)
    mw = np.array(ob.matrix_world)
    ev.to_mesh_clear()
    return pts @ mw[:3, :3].T + mw[:3, 3]


def strip(path, side, width, normals, lift=0.0):
    """A ribbon along path (k, 3): across it toward side (k, 3) unit vectors, width (scalar or (k,)), lifted along
    normals. Two rows; UVs: u along the path in metres, v across."""
    path = np.asarray(path, dtype=np.float64)
    w = np.broadcast_to(np.asarray(width, dtype=np.float64), (len(path),))
    a = path + normals * lift - side * (w[:, None] * 0.5)
    b = path + normals * lift + side * (w[:, None] * 0.5)
    X = np.stack([a[:, 0], b[:, 0]])
    Y = np.stack([a[:, 1], b[:, 1]])
    Z = np.stack([a[:, 2], b[:, 2]])
    return grid(X, Y, Z)


def apply_modifiers(ob, keep=('ARMATURE',)):
    """Applies ob's modifiers except those of the kinds in keep."""
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    for m in list(ob.modifiers):
        if m.type in keep:
            continue
        bpy.ops.object.modifier_apply(modifier=m.name)


def solidify(ob, thickness, offset=1.0, rim=True):
    """Gives a single-sided piece thickness (applied). The inner side shares the outer side's UVs."""
    m = ob.modifiers.new('solidify', 'SOLIDIFY')
    m.thickness = thickness
    m.offset = offset
    m.use_rim = rim
    # even thickness divides by the angle at each corner and shoots spikes from sharp ones (a sole's edge, a vent):
    # plain offset along the normals instead
    m.use_even_offset = False
    m.use_quality_normals = False
    apply_modifiers(ob)
