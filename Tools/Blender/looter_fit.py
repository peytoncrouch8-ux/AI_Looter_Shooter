"""Fit helpers for the heroes' game meshes (the clipping and fit pass, 2026-10-27; checked with fit_audit.py): what a
fully covering layer hides is taken away, so the layer under it can't show through it when a pose bends both; weights
handed from one bone to another. A model that uses these lists this file in its BAKE_SOURCES (it shapes the baked mesh).

    looter_fit.hide_covered([trousers], [boots], margin=0.05)   # trouser faces inside the boots' shafts go
    W = looter_fit.shift_weights(W, names, ('thigh_l', 'thigh_r'), 'pelvis', 0.5)
"""
import bmesh
import bpy
import numpy as np
from mathutils import Vector

import looter_garments as gm


def covered(points, normals, tree, reach, margin):
    """Which points the outer surface (tree) lies over: a ray out along the point's normal meets it within reach, and
    so does one from margin higher up (near a layer's top edge, a band stays: the inner layer shows at the opening)."""
    out = np.zeros(len(points), dtype=bool)
    up = Vector((0.0, 0.0, margin))
    for k, (p, n) in enumerate(zip(points, normals)):
        o, d = Vector(tuple(p)), Vector(tuple(n))
        if d.length < 1e-9:
            continue
        if tree.ray_cast(o + d * 1e-4, d, reach)[0] is None:
            continue
        out[k] = tree.ray_cast(o + up + d * 1e-4, d, reach)[0] is not None
    return out


def hide_covered(inner, outer, reach=0.04, margin=0.05, log=print):
    """Deletes the faces of each inner object whose every corner the outer objects cover (covered()): a trouser leg
    inside a boot's shaft, a sleeve inside a cuff. Run on the game parts before their UVs are packed."""
    tree = gm.bvh_objects(outer)
    for ob in inner:
        me = ob.data
        me.update()
        co = np.array([v.co[:] for v in me.vertices]) @ np.array(ob.matrix_world)[:3, :3].T + np.array(ob.matrix_world)[:3, 3]
        nrm = np.array([v.normal[:] for v in me.vertices]) @ np.array(ob.matrix_world)[:3, :3].T
        hid = covered(co, nrm, tree, reach, margin)
        bm = bmesh.new()
        bm.from_mesh(me)
        bm.faces.ensure_lookup_table()
        gone = [f for f in bm.faces if all(hid[v.index] for v in f.verts)]
        bmesh.ops.delete(bm, geom=gone, context='FACES')
        loose = [v for v in bm.verts if not v.link_faces]
        bmesh.ops.delete(bm, geom=loose, context='VERTS')
        bm.to_mesh(me)
        bm.free()
        me.update()
        log(f'{ob.name}: {len(gone)} faces hidden under {", ".join(o.name for o in outer)}')


def shift_weights(W, names, from_bones, to_bone, share):
    """share of each from_bones' weight moved onto to_bone (rows still sum as before)."""
    W = np.array(W, dtype=np.float64)
    j = names.index(to_bone)
    for b in from_bones:
        k = names.index(b)
        moved = W[:, k] * share
        W[:, k] -= moved
        W[:, j] += moved
    return W


def tuck_under(inner, outer, centre, gap, reach=0.03, log=print):
    """Points of the inner parts that a level ray toward the body's axis (centre: x, y) finds the outer parts in front
    of, or within gap outside them, moved gap inside the outer layer along that ray: what the outer layer covers stays
    covered between its coarse faces; what shows past its edge is left alone (Ellis.py's, for the trousers under the
    waistcoat)."""
    tree = gm.bvh_objects(outer)
    for ob in inner:
        co = gm._object_coords(ob)
        moved = 0
        for k, p in enumerate(co):
            d = Vector((centre[0] - p[0], centre[1] - p[1], 0.0))
            if d.length < 1e-6:
                continue
            d.normalize()
            hit = tree.ray_cast(Vector(tuple(p)) - d * gap, d, reach + gap)
            if hit[0] is not None:
                co[k] = np.array(hit[0] + d * gap)
                moved += 1
        ob.data.vertices.foreach_set('co', co.astype(np.float32).ravel())
        ob.data.update()
        log(f'{ob.name}: {moved} points tucked {gap * 1000:.0f} mm under {", ".join(o.name for o in outer)}')
