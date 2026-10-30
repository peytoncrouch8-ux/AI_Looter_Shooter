"""The Gravemother's headstone from the user's Tripo model (parts_gravemother_build.py's 'stone' piece: its slab cut off
her back, set in the stone's own frame at the scripted slab's size, 80 x 17 x 116 cm), for GravemotherStone.py
(LOOTER_GRAVEMOTHER_TRIPO, default on). Not a model: Tools\\models.ps1 skips parts_*.py.

- The slab: Tripo's (its arched top, raised border and carved cross) as clean even quads, on the scripted stone's very
  texture layout (looter_barrow_stone.REGIONS: front and back by x and z, the edge strip along the rim and across its
  thickness), so the chips' patch and every crack line still land where the scripted ones did.
- The states: the slab cut into looter_barrow_stone's five pieces (its cracks' polygons, their rim sides pushed out past
  Tripo's outline) by exact booleans: state 1 every piece a hair apart on the cracked half of the texture, state 2 shifted
  and tipped, the corner piece gone, and the five shards; each cut face is fresh granite (GravemotherStoneBreak). The
  roots and the earth round the foot are looter_barrow_stone's.
- The texture GravemotherStone: its painted granite, lichen, rain streaks, soil and cracks (looter_barrow_stone's
  painters, its carving left out), with Tripo's own carving baked in from its dense surface (Cycles, selected to active):
  the relief in the normal map, the grooves (its concave places) darkened and mossed."""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Vector

import looter_barrow as lb
import looter_barrow_stone as lbs
import looter_textures as lt

W, H, T = lb.STONE_W, lb.STONE_H, lb.STONE_T
PUSH = 10.0           # cm: a piece's rim and foot sides pushed out past Tripo's outline before it cuts the slab


def log(message):
    print(f'GRAVEMOTHER STONE: {message}', flush=True)


# --- UVs on the scripted layout ---

def _rim():
    pts = [p for p in lbs.rim_outline() if p['kind'] != 'foot']
    xz = np.array([p['co'] for p in pts], float)
    s = np.array([p['s'] for p in pts], float)
    return xz, s


def rim_s(x, z):
    """The rim's length (cm from the right foot corner, looter_barrow_stone.rim_outline's s) at the outline point
    nearest each (x, z)."""
    xz, s = _rim()
    a, b = xz[:-1], xz[1:]
    P = np.column_stack([x, z])[:, None, :]
    ab = b - a
    t = np.clip(((P - a) * ab).sum(-1) / np.maximum((ab * ab).sum(-1), 1e-9), 0.0, 1.0)
    d = np.linalg.norm(a + ab * t[..., None] - P, axis=-1)
    k = np.argmin(d, axis=1)
    rows = np.arange(len(k))
    return s[:-1][k] + (s[1:][k] - s[:-1][k]) * t[rows, k]


def corner_uvs(V, F, variant):
    """Every face's corner UVs on the scripted layout (variant '0' intact, '1' cracked), by the way the face looks: front
    (+y), back, the foot, or the edge strip."""
    V = np.asarray(V, float)
    out = []
    centres = np.array([V[f].mean(0) for f in F])
    s_c = rim_s(centres[:, 0], centres[:, 2])
    for i, f in enumerate(F):
        P = V[f]
        n = np.cross(P[1] - P[0], P[2] - P[0])
        n = n / max(np.linalg.norm(n), 1e-12)
        if n[1] > 0.55:
            out.append([lbs.face_uv('front' + variant, p[0], p[2]) for p in P])
        elif n[1] < -0.55:
            out.append([lbs.face_uv('back' + variant, p[0], p[2], mirror=True) for p in P])
        elif n[2] < -0.6 or centres[i, 2] < 1.0:
            out.append([lbs.edge_uv('edge' + variant, 3, p[0] + W / 2.0, p[1]) for p in P])
        else:
            col = min(int(s_c[i] // lbs.COL_CM), 2)
            s = rim_s(P[:, 0], P[:, 2])
            # A face near the right foot corner (s 0) with a corner past the rim's end (s near its total) wraps.
            total = lbs.rim_total()
            s = np.where(np.abs(s - s_c[i]) > total * 0.5, s + np.sign(s_c[i] - s) * total, s)
            out.append([lbs.edge_uv('edge' + variant, col, sv - col * lbs.COL_CM, p[1]) for sv, p in zip(s, P)])
    return out


# --- Meshes ---

def to_bm(V, F, uvs, mats=None, bm=None):
    """V, F (stone cm) into bm with UVs and a material index per face (looter_barrow_stone.M_*; M_STONE by default)."""
    bm = bm or lb.new_bm()
    uv = bm.loops.layers.uv.active
    verts = [bm.verts.new(Vector(p)) for p in V]
    for i, f in enumerate(F):
        try:
            face = bm.faces.new([verts[k] for k in f])
        except ValueError:
            continue
        face.material_index = lbs.M_STONE if mats is None else mats[i]
        face.smooth = True
        for loop, c in zip(face.loops, uvs[i]):
            loop[uv].uv = c
    return bm


def mesh_object(name, V, F, uvs, scale=0.01):
    bm = to_bm(np.asarray(V) * scale, F, uvs)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.set_sharp_from_angle(angle=math.radians(55.0))
    return obj


def mesh_arrays(mesh):
    mesh.calc_loop_triangles()
    V = np.array([v.co[:] for v in mesh.vertices])
    F = [np.array(p.vertices) for p in mesh.polygons]
    uv = mesh.uv_layers.active.data
    uvs = [[tuple(uv[li].uv) for li in p.loop_indices] for p in mesh.polygons]
    return V, F, uvs


def prism(poly, name):
    """A piece's outline (x, z: stone cm, counter-clockwise) run through the slab along y: its rim and foot sides
    pushed PUSH out (Tripo's outline strays past the scripted one), every face's UVs (-1, -1) (its faces are the cuts)."""
    pts = []
    for p in poly:
        x, z = p['co']
        if p['kind'] != 'crack':
            if z <= 0.5:
                out = Vector((math.copysign(0.7, x) if abs(x) > W / 2.0 - 1.0 else 0.0, -1.0))
            elif z > lbs.ARCH:
                out = Vector((x, z - lbs.ARCH))
            else:
                out = Vector((math.copysign(1.0, x), 0.0))
            out.normalize()
            x, z = x + out.x * PUSH, z + out.y * PUSH
        pts.append((x, z))
    bm = lb.new_bm()
    uv = bm.loops.layers.uv.active
    front = [bm.verts.new((x, 3.0 * T, z)) for x, z in pts]
    back = [bm.verts.new((x, -3.0 * T, z)) for x, z in pts]
    faces = [bm.faces.new(list(reversed(front))), bm.faces.new(back)]
    n = len(pts)
    for i in range(n):
        j = (i + 1) % n
        faces.append(bm.faces.new((front[i], front[j], back[j], back[i])))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    for f in bm.faces:
        for loop in f.loops:
            loop[uv].uv = (-1.0, -1.0)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def intersect(V, F, uvs, poly, name):
    """The slab (stone cm) inside a piece's prism: (V, F, uvs, which faces are cuts)."""
    slab = mesh_object('_slab_' + name, V, F, uvs, scale=1.0)
    cutter = prism(poly, '_prism_' + name)
    mod = slab.modifiers.new('cut', 'BOOLEAN')
    mod.operation = 'INTERSECT'
    mod.solver = 'EXACT'
    mod.object = cutter
    graph = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(slab.evaluated_get(graph))
    Vp, Fp, uvp = mesh_arrays(mesh)
    cut = [all(c[0] < -0.5 for c in u) for u in uvp]
    for o in (slab, cutter):
        data = o.data
        bpy.data.objects.remove(o)
        bpy.data.meshes.remove(data)
    bpy.data.meshes.remove(mesh)
    return Vp, Fp, uvp, cut


def pieces(V, F, state, only=None):
    """The slab's pieces in a state (1 or 2): {name: (V, F, uvs, materials)}, stone cm (state 2's moved)."""
    out = {}
    uvs1 = corner_uvs(V, F, '1')
    for poly in lbs.cut_pieces():
        key = lbs.piece_name(poly)
        if state == 2 and key == 'corner':
            continue
        if only is not None and key != only:
            continue
        gap = 0.3 if state == 1 else 0.6
        Vp, Fp, uvp, cut = intersect(V, F, uvs1, lbs.shrink(poly, gap), key)
        mats = [lbs.M_BREAK if c else lbs.M_STONE for c in cut]
        if state == 2:
            M = lbs.piece_move(key, poly)
            Vp = np.array([(M @ Vector(p))[:] for p in Vp])
        out[key] = (Vp, Fp, uvp, mats)
        log(f'state {state} {key}: {len(Fp)} faces ({sum(cut)} cut)')
    return out


def slab(bm, low, state, S, only=None):
    """GravemotherStone.py's lbs.slab + roots + collar for the Tripo slab: into bm (stone cm), its faces in the stone's
    slots, ready for lbs.finish_uvs."""
    V, F = low
    if state == 0:
        to_bm(V, F, corner_uvs(V, F, '0'), bm=bm)
    else:
        for key, (Vp, Fp, uvp, mats) in pieces(V, F, state, only).items():
            to_bm(Vp, Fp, uvp, mats, bm=bm)
    return bm
