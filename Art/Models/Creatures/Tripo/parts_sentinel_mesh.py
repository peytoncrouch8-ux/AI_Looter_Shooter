"""The Stone Sentinel from the user's Tripo model: the mesh tools parts_sentinel_cut.py works with (cutter solids and
Exact booleans that cut a piece in two, capped; the remesh to quads; the clean-ups after a cut). Not a model:
Tools\models.ps1 skips parts_*.py. Blender metres throughout.
"""
import math

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

BIG = 3.0


def log(message):
    print(f'SENTINEL: {message}', flush=True)


# --- Solids ---

def _material(name, color):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.diffuse_color = color
    return mat


def solid(name, bm):
    bm.normal_update()
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.materials.append(_material('_Cap', (0.9, 0.3, 0.2, 1.0)))
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def hull(name, points):
    bm = bmesh.new()
    for p in points:
        bm.verts.new(p)
    made = bmesh.ops.convex_hull(bm, input=list(bm.verts))
    junk = [v for v in made['geom_interior'] + made['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(junk)), context='VERTS')
    return solid(name, bm)


def box(name, lo, hi):
    return hull(name, [Vector((x, y, z)) for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])])


def halfspace(name, point, normal):
    """The solid on normal's side of the plane through point (a big box)."""
    n = Vector(normal).normalized()
    a = n.orthogonal().normalized()
    b = n.cross(a)
    p = Vector(point)
    pts = [p + a * (i * BIG) + b * (j * BIG) + n * (k * BIG) for i in (-1, 1) for j in (-1, 1) for k in (0, 1)]
    return hull(name, pts)


def sphere(name, centre, radius):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=20, v_segments=10, radius=radius,
                              matrix=Matrix.Translation(Vector(centre)))
    return solid(name, bm)


def _boolean(target, operand, op, self_check=True, tolerant=False):
    """target op operand (Exact), as a new object; neither input is changed. Faces from the operand keep its _Cap slot."""
    obj = target.copy()
    obj.data = target.data.copy()
    bpy.context.scene.collection.objects.link(obj)
    mod = obj.modifiers.new('cut', 'BOOLEAN')
    mod.operation = op
    mod.solver = 'EXACT'
    mod.object = operand
    mod.material_mode = 'TRANSFER'
    mod.use_self = self_check       # QuadriFlow's shell crosses itself in a few thin folds
    mod.use_hole_tolerant = tolerant
    deps = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(obj.evaluated_get(deps))
    old = obj.data
    obj.modifiers.clear()
    obj.data = me
    bpy.data.meshes.remove(old)
    return obj


def union(name, *solids):
    out = solids[0]
    for s in solids[1:]:
        nxt = _boolean(out, s, 'UNION')
        if out is not solids[0]:
            _remove(out)
        out = nxt
    out.name = name
    return out


def minus(name, a, b):
    out = _boolean(a, b, 'DIFFERENCE')
    out.name = name
    return out


def _remove(obj):
    me = obj.data
    bpy.data.objects.remove(obj)
    if me and not me.users:
        bpy.data.meshes.remove(me)


def _open_edges(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    n = sum(1 for e in bm.edges if not e.is_manifold)
    bm.free()
    return n


def _heal(obj):
    """Mends what a cut through a thin fold leaves: edges shared by more than two faces split apart, slivers and
    duplicate points merged, holes filled."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=1e-6)
    many = [e for e in bm.edges if len(e.link_faces) > 2]
    if many:
        bmesh.ops.split_edges(bm, edges=many)
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context='VERTS')
    open_ = [e for e in bm.edges if e.is_boundary]
    if open_:
        bmesh.ops.holes_fill(bm, edges=open_, sides=0)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data)
    bm.free()


def _bounds(obj):
    co = np.array([v.co[:] for v in obj.data.vertices]) if obj.data.vertices else np.zeros((1, 3))
    return co.min(0), co.max(0)


def split(piece, cutter, inside_name, outside_name):
    """(piece inside cutter, piece outside it), both capped; piece and cutter are removed."""
    # A boolean that lost track of inside and outside hands back the cutter's own far faces: nothing cut may reach past
    # the piece it came from. Exact misses now and then on a shell crossing itself: tried again other ways, then with
    # the cutter nudged under a millimetre.
    lo, hi = _bounds(piece)
    tries = [(True, False, 0.0), (False, False, 0.0), (True, True, 0.0), (True, False, 0.0007), (False, True, 0.0011)]
    for k, (self_check, tolerant, nudge) in enumerate(tries):
        cutter.location = (nudge, nudge * 0.7, -nudge * 0.5)
        bpy.context.view_layer.update()
        a = _boolean(piece, cutter, 'INTERSECT', self_check, tolerant)
        b = _boolean(piece, cutter, 'DIFFERENCE', self_check, tolerant)
        bad = [o for o in (a, b) if o.data.polygons and (any(_bounds(o)[0][i] < lo[i] - 0.01 for i in range(3))
                                                         or any(_bounds(o)[1][i] > hi[i] + 0.01 for i in range(3)))]
        if not bad:
            break
        _remove(a)
        _remove(b)
        if k == len(tries) - 1:
            raise RuntimeError(f'the cut of {piece.name} into {inside_name} / {outside_name} failed every way')
        log(f'cut of {piece.name} into {inside_name} / {outside_name} missed (try {k + 1}); again')
    a.name, b.name = inside_name, outside_name
    for o in (a, b):
        n = _open_edges(o)
        if n:
            _heal(o)
            m = _open_edges(o)
            if m:
                log(f'{o.name}: {n} open edges after the cut of {piece.name}, {m} after healing')
    _remove(piece)
    _remove(cutter)
    return a, b



# --- 3. Retopology ---

def _apply_voxel(obj, size):
    mod = obj.modifiers.new('voxel', 'REMESH')
    mod.mode = 'VOXEL'
    mod.voxel_size = size
    deps = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(obj.evaluated_get(deps))
    old = obj.data
    obj.modifiers.clear()
    obj.data = me
    bpy.data.meshes.remove(old)


def _clean_for_quadriflow(obj, dist=3e-4):
    """QuadriFlow refuses a mesh with any edge under 0.1 mm (a voxel pass or a cut leaves some): merged away."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=dist)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=dist)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data)
    bm.free()



def _remesh(obj, want, voxel, factor, seed):
    """One try at obj remeshed to about want quads: a voxel pass (one clean solid: a cut through Tripo's shell, which
    crosses itself where parts touch, isn't always one), collapsed down to factor times the faces wanted (the voxel
    pass leaves slivers, and QuadriFlow refuses any edge under 0.1 mm), then QuadriFlow. None when it refused."""
    q = obj.copy()
    q.data = obj.data.copy()
    bpy.context.scene.collection.objects.link(q)
    _apply_voxel(q, voxel)
    dec = q.modifiers.new('dec', 'DECIMATE')
    dec.ratio = min(1.0, want * factor / max(len(q.data.polygons), 1))
    deps = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(q.evaluated_get(deps))
    old = q.data
    q.modifiers.clear()
    q.data = me
    bpy.data.meshes.remove(old)
    _clean_for_quadriflow(q, 1.5e-4)
    with bpy.context.temp_override(active_object=q, object=q, selected_objects=[q], selected_editable_objects=[q]):
        result = bpy.ops.object.quadriflow_remesh(target_faces=want, use_preserve_sharp=True,
                                                  use_preserve_boundary=False, use_mesh_symmetry=False,
                                                  smooth_normals=False, mode='FACES', seed=seed)
    if 'FINISHED' in result and want * 0.5 <= len(q.data.polygons) <= want * 2.0:
        return q
    _remove(q)
    return None


def _thin_caps(obj, angle=22.0):
    """Caps are mostly hidden (the joints' inner faces, the balls): their quads merged by a limited dissolve inside them
    (never across their rim, so seams keep their line) and triangulated, so the skin keeps the triangles."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    cap = obj.data.materials.find('_Cap')
    faces = [f for f in bm.faces if f.material_index == cap]
    inner = [e for e in {e for f in faces for e in f.edges} if all(g.material_index == cap for g in e.link_faces)]
    verts = [v for v in {v for e in inner for v in e.verts} if all(f.material_index == cap for f in v.link_faces)]
    bmesh.ops.dissolve_limit(bm, angle_limit=math.radians(angle), use_dissolve_boundaries=False, verts=verts,
                             edges=inner, delimit={'MATERIAL'})
    big = [f for f in bm.faces if f.material_index == cap and len(f.verts) > 4]
    if big:
        bmesh.ops.triangulate(bm, faces=big, quad_method='BEAUTY', ngon_method='BEAUTY')
    bm.to_mesh(obj.data)
    bm.free()



def _key(face):
    return tuple(round(c, 6) for c in face.calc_center_median())


def drop_thin(obj, thickness=0.012, least=4, where=None):
    """Razor-thin flaps a cut leaves where it ran through two surfaces pressed together (a sleeve's inner wall on the
    robe, a cuff's hem round the wrist): faces whose own stone, seen straight in from them, is under thickness thick,
    in runs of at least least faces touching a cut face, go; the holes they leave are closed (cap slot)."""
    from mathutils.bvhtree import BVHTree
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    bm.normal_update()
    tree = BVHTree.FromBMesh(bm)
    cap = obj.data.materials.find('_Cap')
    thin = set()
    for f in bm.faces:
        c, n = f.calc_center_median(), f.normal
        if where is not None and not where(c):
            continue
        hit = tree.ray_cast(c - n * 1e-4, -n, thickness)
        if hit[0] is not None and hit[2] != f.index:
            thin.add(f.index)
    doomed, seen = [], set()
    for i in thin:
        if i in seen:
            continue
        stack, run = [bm.faces[i]], []
        seen.add(i)
        while stack:
            f = stack.pop()
            run.append(f)
            for e in f.edges:
                for g in e.link_faces:
                    if g.index in thin and g.index not in seen:
                        seen.add(g.index)
                        stack.append(g)
        # Only flaps a cut made: the run touches a cut face. Tripo's own thin stone (a fist's curled fingers) stays.
        if len(run) >= least and any(g.material_index == cap for f in run for v in f.verts for g in v.link_faces):
            doomed.append([_key(g) for g in run])
    bm.free()
    # Each run on its own, kept only when the hole it leaves closes cleanly (an open seam is worse than a flap).
    gone = 0
    for run in sorted(doomed, key=len):
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        # Faces are found again by their middles: each kept deletion renumbers the rest.
        by_key = {_key(f): f for f in bm.faces}
        faces = [by_key[k] for k in run if k in by_key]
        before = sum(1 for e in bm.edges if not e.is_manifold)
        bmesh.ops.delete(bm, geom=faces, context='FACES')
        bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
        made = bmesh.ops.holes_fill(bm, edges=[e for e in bm.edges if e.is_boundary], sides=0)['faces']
        for f in made:
            f.material_index = cap
        if made:
            bmesh.ops.triangulate(bm, faces=made, quad_method='BEAUTY', ngon_method='BEAUTY')
        if sum(1 for e in bm.edges if not e.is_manifold) <= before:
            bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
            bm.to_mesh(obj.data)
            gone += len(run)
        bm.free()
    return gone




def orient_out(obj):
    """Faces turned to face out of the stone, one by one: a ray from each along its normal crosses the piece's surface
    an even number of times when it faces out. (A cap where a cut met a seam that isn't cleanly manifold can come out
    facing in, and recalculating normals over the whole piece follows it.) Returns how many were turned."""
    from mathutils.bvhtree import BVHTree
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    tree = BVHTree.FromBMesh(bm)
    flipped = []
    for f in bm.faces:
        n = f.normal
        if n.length < 0.5:
            continue
        p = f.calc_center_median() + n * 1e-4
        crossings = 0
        for _ in range(16):
            hit, _n, _i, dist = tree.ray_cast(p, n, 10.0)
            if hit is None:
                break
            crossings += 1
            p = hit + n * 1e-4
        if crossings % 2:
            flipped.append(f)
    if flipped:
        bmesh.ops.reverse_faces(bm, faces=flipped)
    bm.to_mesh(obj.data)
    bm.free()
    return len(flipped)
