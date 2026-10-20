"""What Odessa's model (Art/Models/Characters/Odessa.py) and its builders share: the run's arguments and log, the
figure's measures, the looks' slots, the per-part tables the game mesh reads (thickness, reduction), and small geometry
helpers copied from Ellis.py (which can't be imported: it builds Ellis when loaded). Units metres, Z up, the figure
built facing -Y, its left +X."""
import math
import os
import sys
import time

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector

import looter_garments as gm

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
ARGS = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
T0 = time.time()
FWD = np.array([0.0, -1.0])


def log(message):
    print(f'ODESSA [{time.time() - T0:6.1f}s] {message}', flush=True)


def arg_value(name, default=None):
    return ARGS[ARGS.index(name) + 1] if name in ARGS and ARGS.index(name) + 1 < len(ARGS) else default


# ------------------------------------------------------------------------------------------- the per-part tables

SLOTS = {}
LOOKS = {}                  # key -> builder(name): filled by odessa_looks (the costume's looks)
# Sheets given thickness after the bake (m); the inner side shares the outer side's UVs (Odessa.thicken).
THICK = {}
# The game mesh: each detailed part's share of its triangles kept by Decimate (1 keeps it whole); see Odessa.LOW
LOW = {}
# Cloth whose open edges Decimate must spare (a band this wide along them)
EDGE_KEEP = {}
# Parts whose game mesh is built apart (name -> its low object, or None: a bake source only, no game part)
CUSTOM_LOW = {}
BOOT_CAP = None             # (toe cap's y, welt's z) for the boots' leather: Odessa.Figure measures them


def slot(key):
    """The look for a part of the costume, one material per look; the bake gathers them into the texture sets."""
    if key not in SLOTS:
        SLOTS[key] = LOOKS[key](f'Odessa_{key}')
        SLOTS[key]['LookKey'] = key
    return SLOTS[key]


# ------------------------------------------------------------------------------------------------ small geometry

def bake_transform(ob):
    ob.data.transform(ob.matrix_world)
    ob.matrix_world = Matrix.Identity(4)


def uv_sphere(bm, segments, rings, radius):
    """bmesh's UV sphere in a fixed order. create_uvsphere's faces came out in another order (each starting at another
    corner) from one Blender run to the next: Decimate then gave the same shape with its vertices in another order,
    smart UV and the atlas packed round it moved, and a bake kept from an earlier run no longer fit the game mesh.
    Rebuilt with each face from its lowest vertex, the faces sorted."""
    bmesh.ops.create_uvsphere(bm, u_segments=segments, v_segments=rings, radius=radius)
    bm.verts.index_update()
    co = [v.co.copy() for v in bm.verts]
    faces = []
    for f in bm.faces:
        idx = [v.index for v in f.verts]
        k = idx.index(min(idx))
        faces.append(tuple(idx[k:] + idx[:k]))
    bm.clear()
    verts = [bm.verts.new(c) for c in co]
    for f in sorted(faces):
        bm.faces.new([verts[i] for i in f])
    bm.verts.index_update()
    bm.faces.index_update()


def ellipsoid(name, centre, radii, z_axis, segments=16, rings=10, material=None):
    bm = bmesh.new()
    uv_sphere(bm, segments, rings, 1.0)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    rot = Vector(tuple(z_axis)).to_track_quat('Z', 'Y').to_matrix().to_4x4()
    ob.matrix_world = Matrix.Translation(Vector(tuple(centre))) @ rot @ Matrix.Diagonal((*radii, 1.0))
    bake_transform(ob)
    if material is not None:
        ob.data.materials.append(material)
    return ob


def lathe(name, profile, centre, n=24, material=None, cap_top=True, axis=(0.0, 0.0, 1.0)):
    """A body of revolution: profile [(radius, height)] from the bottom up about an axis (default vertical) at centre."""
    profile = np.asarray(profile, dtype=np.float64)
    ang = np.linspace(0, 2 * math.pi, n, endpoint=False)
    X = profile[:, 0:1] * np.cos(ang)[None]
    Y = profile[:, 0:1] * np.sin(ang)[None]
    Z = np.repeat(profile[:, 1:2], n, 1)
    pc = gm.grid(X[::-1], Y[::-1], Z[::-1], wrap=True)
    verts = list(pc.verts)
    faces, uv = pc.faces, pc.uv
    for row, at in ((0, profile[-1]), (len(profile) - 1, profile[0])):
        if (row == 0 and not cap_top) or at[0] < 1e-4:
            continue
        c = len(verts)
        verts.append(np.array([0.0, 0.0, at[1]]))
        for i in range(n):
            a, b = row * n + i, row * n + (i + 1) % n
            faces.append([b, a, c] if row == 0 else [a, b, c])
            uv += [(0.0, 0.0), (0.01, 0.0), (0.005, 0.01)]
    R = np.array(Vector((0.0, 0.0, 1.0)).rotation_difference(Vector(axis)).to_matrix())
    V = np.array(verts) @ R.T + np.asarray(centre)
    return gm.mesh(name, gm.Piece(V, faces, uv), material)


def uv_cylinder(ob):
    """A plain cylindrical UV map round the object's vertical axis, for pieces that come without one (the bake
    repacks every island anyway)."""
    me = ob.data
    if me.uv_layers:
        return
    layer = me.uv_layers.new(name='UVMap')
    co = np.array([v.co[:] for v in me.vertices])
    c = co.mean(axis=0)
    uv = []
    for p in me.polygons:
        ids = list(p.vertices)
        ang = np.unwrap(np.arctan2(co[ids, 0] - c[0], -(co[ids, 1] - c[1])))
        r = np.linalg.norm(co[ids, :2] - c[:2], axis=1).mean()
        for a, i in zip(ang, ids):
            uv.append((a * max(r, 0.02), co[i, 2] + np.linalg.norm(co[i, :2] - c[:2]) * (1.0 if co[i, 2] < c[2] else 0.0)))
    layer.data.foreach_set('uv', np.array(uv, dtype=np.float32).ravel())


def hull_2d(p):
    """The convex hull of 2D points, counterclockwise (Andrew's monotone chain)."""
    p = p[np.lexsort((p[:, 1], p[:, 0]))]
    cross = lambda o, a, b: (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for q in p:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], q) <= 0:
            lower.pop()
        lower.append(q)
    for q in p[::-1]:
        while len(upper) >= 2 and cross(upper[-2], upper[-1], q) <= 0:
            upper.pop()
        upper.append(q)
    return np.array(lower[:-1] + upper[:-1])


def hull_radii(hull, dirs):
    """How far out along each unit direction (k, 2) from the origin a convex polygon round it reaches."""
    e = np.roll(hull, -1, axis=0) - hull
    den = dirs[:, 0, None] * e[None, :, 1] - dirs[:, 1, None] * e[None, :, 0]
    with np.errstate(divide='ignore', invalid='ignore'):
        t = (hull[None, :, 0] * e[None, :, 1] - hull[None, :, 1] * e[None, :, 0]) / den
        u = (hull[None, :, 0] * dirs[:, 1, None] - hull[None, :, 1] * dirs[:, 0, None]) / den
    ok = (np.abs(den) > 1e-12) & (t > 0.0) & (u >= -1e-9) & (u <= 1.0 + 1e-9)
    return np.where(ok, t, 0.0).max(axis=1)


def fillet(ob, near, iters=60, step=0.5):
    """Rounds the crease where fused solids meet: the vertices near both (near: 0..1 each) eased toward their
    neighbours' middle."""
    me = ob.data
    co = gm._object_coords(ob)
    ed = np.empty(len(me.edges) * 2, dtype=np.int64)
    me.edges.foreach_get('vertices', ed)
    a, b = ed[0::2], ed[1::2]
    cnt = np.bincount(a, minlength=len(co)) + np.bincount(b, minlength=len(co))
    k = (step * near)[:, None]
    for _ in range(iters):
        acc = np.stack([np.bincount(a, co[b, j], len(co)) + np.bincount(b, co[a, j], len(co)) for j in range(3)], 1)
        co += k * (acc / np.maximum(cnt, 1)[:, None] - co)
    me.vertices.foreach_set('co', co.astype(np.float32).ravel())
    me.update()


def round_angle(P, centre):
    """Angles of points round a vertical axis at centre (radians: 0 the front, + toward the figure's left)."""
    f, l = gm.frame2d(FWD)
    rel = np.atleast_2d(P)[:, :2] - centre
    return np.arctan2(rel @ l, rel @ f)


def on_surface(tree, centre, ang, z, spread=(0, 2, -2, 4, -4, 6, -6, 9, -9, 12, -12)):
    """The outermost point of a garment (tree) at angle ang round centre and height z: a ray from outside, inward
    (the nearest ray either side that hits, turned back onto ang)."""
    ex, ey = gm.radial(np.float64(ang), FWD)
    for d in np.radians(np.asarray(spread, dtype=np.float64)):
        dx, dy = gm.radial(np.float64(ang + d), FWD)
        hit = tree.ray_cast(Vector((centre[0] + dx, centre[1] + dy, z)), Vector((-dx, -dy, 0.0)), 1.0)
        if hit[0] is not None:
            r = math.hypot(hit[0].x - centre[0], hit[0].y - centre[1])
            return np.array([centre[0] + ex * r, centre[1] + ey * r, hit[0].z])
    raise ValueError(f'no garment at {math.degrees(ang):.0f} degrees, {z:.3f} m')


def mesh_arrays(ob):
    ed = np.empty(len(ob.data.edges) * 2, dtype=np.int64)
    ob.data.edges.foreach_get('vertices', ed)
    return gm._object_coords(ob), ed.reshape(-1, 2)


def set_seams(ob, seam_dist=None):
    """The 'Seams' colour attribute the looks sew from (looter_looks.construction): R how close each vertex is to an
    open edge, G to a sewn seam (seam_dist, metres per vertex), both 1 there and 0 from 10 cm."""
    from mathutils import kdtree
    me = ob.data
    co = gm._object_coords(ob)
    count = {}
    for p in me.polygons:
        vs = list(p.vertices)
        for k in range(len(vs)):
            key = (min(vs[k], vs[(k + 1) % len(vs)]), max(vs[k], vs[(k + 1) % len(vs)]))
            count[key] = count.get(key, 0) + 1
    d_edge = np.full(len(co), 1.0)
    pts = []
    for (a, b), c in count.items():
        if c == 1:
            for t in np.linspace(0.0, 1.0, 6):
                pts.append(co[a] * (1 - t) + co[b] * t)
    if pts:
        tree = kdtree.KDTree(len(pts))
        for i, q in enumerate(pts):
            tree.insert(q, i)
        tree.balance()
        d_edge = np.array([tree.find(q)[2] for q in co])
    col = np.ones((len(co), 4), dtype=np.float32)
    col[:, 0] = 1.0 - np.clip(d_edge / 0.1, 0.0, 1.0)
    col[:, 1] = 0.0 if seam_dist is None else 1.0 - np.clip(np.asarray(seam_dist) / 0.1, 0.0, 1.0)
    col[:, 2] = 0.0
    attr = me.color_attributes.get('Seams') or me.color_attributes.new('Seams', 'FLOAT_COLOR', 'POINT')
    attr.data.foreach_set('color', col.ravel())


def rounded_slab(name, outline, thickness, frame, material=None):
    """A thin closed slab (a card, a case's face): outline [(x, z)] in its own plane, faces front (-y) and back, a rim;
    frame (origin, x, y, z axes) places it."""
    o, ax, ay, az = (np.asarray(a, dtype=np.float64) for a in frame)
    n = len(outline)
    pts = []
    for y in (-thickness * 0.5, thickness * 0.5):
        for x, z in outline:
            pts.append(o + ax * x + ay * y + az * z)
    # a counterclockwise outline in (x, z) faces -y (x cross z): the front as it is, the back and the rim turned
    faces = [list(range(n)), list(range(n, 2 * n))[::-1]]
    for i in range(n):
        j = (i + 1) % n
        faces.append([i, n + i, n + j, j])
    return gm.mesh(name, gm.Piece(np.array(pts), faces), material, smooth=False)


def rounded_rect(w, h, r, k=5):
    """A rounded rectangle's outline (w x h, corner radius r), counterclockwise in (x, z)."""
    out = []
    for cx, cz, a0 in ((1, 1, 0), (-1, 1, 90), (-1, -1, 180), (1, -1, 270)):
        for a in np.radians(a0 + np.linspace(0, 90, k)):
            out.append((cx * (w / 2 - r) + r * math.cos(a), cz * (h / 2 - r) + r * math.sin(a)))
    return out


def faces_out(ob, point, axis):
    """Turns a sheet round a line (through point, along axis: a sleeve's, a neck's) to face away from it if most of its
    faces face in. (A grid's faces face by the order of its rows and columns: one laid the other way came out inside
    out, back-face culled in the game and its occlusion baked 0.)"""
    me = ob.data
    point, axis = np.asarray(point, dtype=np.float64), gm.unit(axis)
    score = 0.0
    for p in me.polygons:
        r = np.array(p.center) - point
        r -= axis * (r @ axis)
        score += float(np.array(p.normal) @ r) * p.area
    if score < 0.0:
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
        me.update()
    return score < 0.0
