"""Puts an AI-made creature (a Tripo .glb, loaded by looter_tripo.load) onto one of our scripted rigs: clean even quads,
the rig's own skeleton and rest pose kept exactly, the mesh brought onto it.

- Surface: the source mesh as triangles with a BVH: the nearest point on it (its triangle and barycentric weights), so
  anything the source carries per corner (its UVs, so its texture's colours) can be read anywhere near it.
- magnify() / quad_remesh(): QuadriFlow lays its quads at one size all over. Small parts needing more of them (fingers,
  a face) are blown up first by a radial map round each centre (a diffeomorphism: no fold anywhere), QuadriFlow runs on
  the blown-up surface, and every new vertex is taken back through the triangle it lies on (barycentric), so the quads
  come back smaller there. A voxel remesh first makes the source watertight (QuadriFlow needs a manifold).
- cap(): closes a cut's open loop with rings toward a raised middle (a skull under a hat, a hat's lining).
- ao(): ambient occlusion per vertex by rays (a cavity map for vertex colours).
- frame() / bone_map() / warp(): each bone's map from where the creature's own joint is to where the rig's is (a turn,
  a stretch along the bone, a scale across it), blended by the skin weights (linear blend skinning): the mesh lands in
  the rig's rest pose. The weights are made in the creature's own pose (where its joints really are), then kept.
- heat_weights(): Blender's bone-heat weights from an armature laid on the creature's own joints.
Everything is numpy over arrays (V: n x 3 vertices, faces: lists of index arrays)."""

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree


# --- Arrays <-> Blender ---

def mesh_arrays(mesh):
    """Vertices and faces (a list of index arrays, one per face) of a Blender mesh."""
    V = np.empty(len(mesh.vertices) * 3)
    mesh.vertices.foreach_get('co', V)
    starts = np.empty(len(mesh.polygons), dtype=np.int64)
    counts = np.empty(len(mesh.polygons), dtype=np.int64)
    mesh.polygons.foreach_get('loop_start', starts)
    mesh.polygons.foreach_get('loop_total', counts)
    loops = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loops)
    return V.reshape(-1, 3), [loops[s:s + c] for s, c in zip(starts, counts)]


def new_mesh(name, V, faces):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(np.asarray(V, float).tolist(), [], [list(map(int, f)) for f in faces])
    mesh.validate()
    return mesh


def new_object(name, V, faces):
    obj = bpy.data.objects.new(name, new_mesh(name, V, faces))
    bpy.context.scene.collection.objects.link(obj)
    return obj


def triangles(faces):
    """Fan-triangulates faces: (m x 3 indices, the face each triangle came from)."""
    tris, owner = [], []
    for i, f in enumerate(faces):
        for k in range(1, len(f) - 1):
            tris.append((f[0], f[k], f[k + 1]))
            owner.append(i)
    return np.array(tris, dtype=np.int64), np.array(owner, dtype=np.int64)


def by_size(faces):
    """Faces grouped by corner count as 2-D arrays (what face loops over many faces at once want)."""
    groups = {}
    for f in faces:
        groups.setdefault(len(f), []).append(f)
    return [np.array(g) for g in groups.values()]


# --- The source surface ---

class Surface:
    """A mesh as triangles with a BVH, for the nearest point on it."""

    def __init__(self, V, faces, corner_uv=None):
        self.V = np.asarray(V, float)
        self.T, self.owner = triangles(faces)
        self.bvh = BVHTree.FromPolygons(self.V.tolist(), self.T.tolist(), all_triangles=True)
        self.uv = None
        if corner_uv is not None:
            # corner_uv: per face, its corners' UVs (k x 2); per triangle the same fan.
            uv = []
            for f_uv in corner_uv:
                for k in range(1, len(f_uv) - 1):
                    uv.append((f_uv[0], f_uv[k], f_uv[k + 1]))
            self.uv = np.array(uv, float)

    def nearest(self, P, reach=1e9):
        """For each point: (triangle index or -1, barycentric weights (3), the nearest point, its distance)."""
        P = np.asarray(P, float)
        tri = np.full(len(P), -1, dtype=np.int64)
        loc = P.copy()
        dist = np.full(len(P), np.inf)
        for i, p in enumerate(P):
            hit = self.bvh.find_nearest(Vector(p), reach)
            if hit[0] is not None:
                loc[i], tri[i], dist[i] = hit[0], hit[2], hit[3]
        bary = self.barycentric(tri, loc)
        return tri, bary, loc, dist

    def barycentric(self, tri, P):
        A, B, C = (self.V[self.T[np.maximum(tri, 0), k]] for k in range(3))
        v0, v1, v2 = B - A, C - A, P - A
        d00, d01, d11 = (v0 * v0).sum(1), (v0 * v1).sum(1), (v1 * v1).sum(1)
        d20, d21 = (v2 * v0).sum(1), (v2 * v1).sum(1)
        den = np.where(np.abs(d00 * d11 - d01 * d01) < 1e-20, 1e-20, d00 * d11 - d01 * d01)
        b = (d11 * d20 - d01 * d21) / den
        c = (d00 * d21 - d01 * d20) / den
        W = np.clip(np.column_stack([1.0 - b - c, b, c]), 0.0, 1.0)
        return W / np.maximum(W.sum(1, keepdims=True), 1e-12)

    def point(self, tri, bary, V=None):
        """The point at these barycentric weights on these triangles, of V (other positions of the same vertices)."""
        V = self.V if V is None else V
        return sum(bary[:, k:k + 1] * V[self.T[tri, k]] for k in range(3))

    def vertex_attr(self, tri, bary, values):
        """A per-vertex array of the source, read at these points."""
        values = np.asarray(values, float)
        return sum(bary[:, k:k + 1] * values[self.T[tri, k]].reshape(len(tri), -1) for k in range(3))

    def uv_at(self, tri, bary):
        return sum(bary[:, k:k + 1] * self.uv[tri, k] for k in range(3))


def corner_uvs(mesh, faces):
    """Each face's corner UVs (k x 2) from the mesh's active UV layer."""
    data = np.empty(len(mesh.loops) * 2)
    mesh.uv_layers.active.data.foreach_get('uv', data)
    data = data.reshape(-1, 2)
    starts = np.empty(len(mesh.polygons), dtype=np.int64)
    mesh.polygons.foreach_get('loop_start', starts)
    return [data[s:s + len(f)] for s, f in zip(starts, faces)]


def sample_image(pixels, uv):
    """Bilinear colour of an image (h x w x c, float, row 0 at the bottom as Blender stores it) at UVs (wrapped)."""
    h, w = pixels.shape[:2]
    x = (np.mod(uv[:, 0], 1.0) * w - 0.5) % w
    y = (np.mod(uv[:, 1], 1.0) * h - 0.5) % h
    x0, y0 = np.floor(x).astype(int), np.floor(y).astype(int)
    fx, fy = (x - x0)[:, None], (y - y0)[:, None]
    x1, y1 = (x0 + 1) % w, (y0 + 1) % h
    return ((pixels[y0, x0] * (1 - fx) + pixels[y0, x1] * fx) * (1 - fy)
            + (pixels[y1, x0] * (1 - fx) + pixels[y1, x1] * fx) * fy)


# --- Even quads ---

def _radial(r, factor, inner, outer):
    """The radial map's new radius: r times factor inside inner, easing to a plain shift past outer; its slope (the
    magnification along the radius) is 1 + (factor - 1) * s(r), s a smooth step from 1 at inner to 0 at outer, so it
    never folds."""
    t = np.clip((r - inner) / max(outer - inner, 1e-9), 0.0, 1.0)
    # The integral of s over [inner, r]: s = 1 - (3t^2 - 2t^3).
    span = outer - inner
    integral = span * (t - t ** 3 + 0.5 * t ** 4)
    return r + (factor - 1.0) * (np.minimum(r, inner) + integral)


def magnify(V, regions):
    """V blown up round each region (centre, factor, inner radius, outer radius), one after another."""
    V = np.asarray(V, float).copy()
    # Each map moves everything round it (a blown-up hand pushes the head away), so the later centres go with it.
    centers = np.array([c for c, *_ in regions], float).reshape(-1, 3)
    for k, (_, factor, inner, outer) in enumerate(regions):
        center = centers[k].copy()
        for P in (V, centers):
            d = P - center
            r = np.linalg.norm(d, axis=1)
            scale = np.where(r > 1e-12, _radial(r, factor, inner, outer) / np.maximum(r, 1e-12), factor)
            P[:] = center + d * scale[:, None]
    return V


def quad_remesh(V, faces, quads, regions=(), voxel=0.005, seed=0, name='_quads'):
    """An even quad mesh over the surface (V, faces): a voxel remesh (watertight), blown up round the regions,
    QuadriFlow to about quads faces, taken back. Returns (vertices, faces)."""
    obj = new_object(name, V, faces)
    obj.data.remesh_voxel_size = voxel
    obj.data.remesh_voxel_adaptivity = 0.0
    with bpy.context.temp_override(active_object=obj, object=obj, selected_objects=[obj],
                                   selected_editable_objects=[obj]):
        bpy.ops.object.voxel_remesh()
    # Specks the voxels leave (a few faces apart, or touching at a corner) go: QuadriFlow needs a clean manifold.
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    islands, seen = [], set()
    for f in bm.faces:
        if f.index in seen:
            continue
        stack, group = [f], []
        seen.add(f.index)
        while stack:
            g = stack.pop()
            group.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h.index not in seen:
                        seen.add(h.index)
                        stack.append(h)
        islands.append(group)
    biggest = max(len(g) for g in islands)
    junk = [f for g in islands if len(g) < biggest * 0.002 for f in g]
    bmesh.ops.delete(bm, geom=junk, context='FACES')
    bad = [v for v in bm.verts if not v.is_manifold]
    if bad:
        bmesh.ops.delete(bm, geom=bad, context='VERTS')
        bmesh.ops.holes_fill(bm, edges=bm.edges, sides=0)
    print(f'quad_remesh {name}: voxels {len(bm.faces)} faces in {len(islands)} pieces, {len(junk)} specks dropped, '
          f'{len(bad)} bad corners, {sum(1 for e in bm.edges if not e.is_manifold)} open edges left')
    bm.to_mesh(obj.data)
    bm.free()
    Vv, Fv = mesh_arrays(obj.data)
    Vm = magnify(Vv, regions)
    # QuadriFlow's manifold check has a fixed tolerance that fails small meshes (a 10 cm sphere): it runs 100 times up.
    obj.data.vertices.foreach_set('co', (Vm * 100.0).ravel())
    obj.data.update()
    # QuadriFlow reads the view layer's active object, not an override.
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    if True:
        result = bpy.ops.object.quadriflow_remesh(target_faces=int(quads), use_preserve_sharp=False,
                                                  use_preserve_boundary=False, use_mesh_symmetry=False,
                                                  smooth_normals=False, seed=seed, mode='FACES')
    if 'FINISHED' not in result:
        raise RuntimeError(f'QuadriFlow failed on {name}')
    Vq, Fq = mesh_arrays(obj.data)
    Vq = Vq / 100.0
    if len(Fq) > 3 * quads or any(len(f) != 4 for f in Fq[:50]):
        raise RuntimeError(f"QuadriFlow made nothing of {name} ({len(Fq)} faces): its voxel mesh is not manifold")
    # Back through the blown-up voxel surface's triangles.
    blown = Surface(Vm, Fv)
    tri, bary, _, _ = blown.nearest(Vq)
    Vq = blown.point(tri, bary, Vv)
    bpy.data.objects.remove(obj)
    return Vq, Fq


def edges_of(faces):
    edges = set()
    for f in faces:
        for k in range(len(f)):
            a, b = int(f[k]), int(f[(k + 1) % len(f)])
            edges.add((min(a, b), max(a, b)))
    return np.array(sorted(edges))


def neighbours(n, faces):
    """Each vertex's neighbours along the edges."""
    out = [[] for _ in range(n)]
    for a, b in edges_of(faces):
        out[a].append(b)
        out[b].append(a)
    return out


def smooth_values(values, faces, passes=3, amount=0.5):
    """A per-vertex array averaged with its neighbours a few times (weights, fades)."""
    X = np.asarray(values, float).copy()
    E = edges_of(faces)
    n = len(X)
    for _ in range(passes):
        acc = np.zeros_like(X)
        cnt = np.zeros(n)
        np.add.at(acc, E[:, 0], X[E[:, 1]])
        np.add.at(acc, E[:, 1], X[E[:, 0]])
        np.add.at(cnt, E[:, 0], 1)
        np.add.at(cnt, E[:, 1], 1)
        shape = (-1,) + (1,) * (X.ndim - 1)
        X = X + (acc / np.maximum(cnt, 1).reshape(shape) - X) * amount
    return X


def vertex_normals(V, faces):
    V = np.asarray(V, float)
    N = np.zeros_like(V)
    for F in by_size(faces):
        P = V[F]
        n = np.zeros((len(F), 3))
        for k in range(1, F.shape[1] - 1):
            n += np.cross(P[:, k] - P[:, 0], P[:, k + 1] - P[:, 0])
        for k in range(F.shape[1]):
            np.add.at(N, F[:, k], n)
    return N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)


# --- From the creature's joints to the rig's ---

def frame(head, tail, side):
    """A bone's frame: origin its head, y along it, x toward side (made square to y), z = x cross y."""
    head, tail = np.asarray(head, float), np.asarray(tail, float)
    y = tail - head
    length = np.linalg.norm(y)
    y = y / length
    x = np.asarray(side, float) - y * (np.asarray(side, float) @ y)
    x /= np.linalg.norm(x)
    z = np.cross(x, y)
    return np.column_stack([x, y, z]), head, length


def bone_map(source, target, along=None, across=1.0):
    """The 4x4 map from a bone in the creature's pose (frame(...) of its joints) to the rig's: its head onto the rig
    bone's head, its axes onto the rig's, stretched along the bone (the rig's length over its own, or along) and
    scaled across it by across."""
    Rs, hs, ls = source
    Rt, ht, lt_ = target
    stretch = (lt_ / ls) if along is None else along
    S = np.diag([across, stretch, across])
    A = Rt @ S @ Rs.T
    M = np.eye(4)
    M[:3, :3] = A
    M[:3, 3] = ht - A @ hs
    return M


def warp(V, weights, maps):
    """Linear blend skinning: each vertex by its bones' maps, weighted ({bone: n weights}, {bone: 4x4})."""
    V = np.asarray(V, float)
    out = np.zeros_like(V)
    total = np.zeros(len(V))
    H = np.column_stack([V, np.ones(len(V))])
    for bone, w in weights.items():
        if bone not in maps:
            continue
        out += w[:, None] * (H @ maps[bone].T)[:, :3]
        total += w
    return out / np.maximum(total, 1e-12)[:, None]


def heat_weights(V, faces, specs, deform=None):
    """Blender's bone-heat weights of the mesh (V, faces) for an armature of specs (name, head, tail, parent); deform:
    the bones that may take weight (others are laid out but take none). Returns {bone: n weights} (rows sum to 1 where
    any bone reached the vertex)."""
    import looter_model as lm
    rig = lm.armature('_heat')
    lm.bones(rig, specs)
    keep = set(name for name, *_ in specs) if deform is None else set(deform)
    for b in rig.data.bones:
        b.use_deform = b.name in keep
    obj = new_object('_heatmesh', V, faces)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.parent_set(type='ARMATURE_AUTO')
    out = {}
    n = len(obj.data.vertices)
    for group in obj.vertex_groups:
        w = np.zeros(n)
        for v in obj.data.vertices:
            for g in v.groups:
                if g.group == group.index:
                    w[v.index] = g.weight
        if w.any():
            out[group.name] = w
    bpy.data.objects.remove(obj)
    bpy.data.objects.remove(rig)
    return out


def normalize(weights, n, top=4, floor=0.01):
    """Weights per bone limited to the top few a vertex, small ones dropped, rows summed to 1."""
    names = sorted(weights)
    W = np.zeros((n, len(names)))
    for i, b in enumerate(names):
        W[:, i] = np.clip(weights[b], 0.0, None)
    order = np.argsort(-W, axis=1)
    keep = np.zeros_like(W, dtype=bool)
    np.put_along_axis(keep, order[:, :top], True, axis=1)
    W = np.where(keep, W, 0.0)
    W[W < floor] = 0.0
    W = W / np.maximum(W.sum(1, keepdims=True), 1e-9)
    return {b: W[:, i] for i, b in enumerate(names) if W[:, i].any()}


def mesh_bmesh_clean(V, faces, merge=1e-6):
    """Merges coincident vertices and drops degenerate faces: (V, faces)."""
    bm = bmesh.new()
    vs = [bm.verts.new(p) for p in np.asarray(V, float)]
    for f in faces:
        try:
            bm.faces.new([vs[int(i)] for i in f])
        except ValueError:
            pass
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=merge)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=merge)
    bm.verts.index_update()
    V2 = np.array([v.co[:] for v in bm.verts])
    F2 = [np.array([v.index for v in f.verts]) for f in bm.faces]
    bm.free()
    return V2, F2


# The shape helpers (thin cloth, needles, colour borders, caps, occlusion) live in looter_tripo_shape; imported last, as
# it builds on the functions above.
from looter_tripo_shape import (ao, boundary_loops, cap, carry, cut_field, despike,  # noqa: E402,F401
                                thicken, thickness, tidy_polygons)
