"""The story people's Tripo models (Art/Tripo/<Name>/<Name>.glb, made 2026-11-01 from Art/Tripo/specs/story_people.json)
as parts_people.py needs them. Two spaces:
  raw      the GLB as Blender imports it, turned to face -Y (the rigs' front; Tripo exports facing +X) and set on its feet
           (z 0): where the joints in parts_people_cfg.py were measured (Saved/ArtPreviews/TripoPeople/<Name>_front/
           _side.png, Docs/Work/2026-10-31-Wardens/tripo_people/glb_views.py);
  upright  raw scaled and moved so its shoulder joints sit at SHOULDER_Z and its hip joints at HIP_Z, centred on x 0
           and on the hips' depth: the frame the scripts build an upright person in (Amos.py's), in Tripo's own pose.
The bone maps (parts_people_rig) carry upright onto each rig's rest."""
import hashlib
import os

import bmesh
import bpy
import numpy as np

import looter_textures as lt
import looter_tripo as tripo
import looter_tripo_rig as tr
import parts_people_cfg as cfg

SHOULDER_Z, HIP_Z = 1.43, 0.91      # the scripts' upright frame (Amos.py: SHOULDER 1.43, HIPS 0.91)
CACHE = os.path.join(lt.REPO, 'Intermediate', 'TripoPeople', 'Quads')


def log(message):
    print(f'TRIPO PEOPLE: {message}', flush=True)


def glb_path(name):
    return os.path.join(lt.REPO, 'Art', 'Tripo', name, f'{name}.glb')


def _yaw(name):
    yaw = np.radians(cfg.PEOPLE[name].get('yaw', -90.0))      # Tripo exports facing +X; the rigs face -Y
    return np.array([[np.cos(yaw), -np.sin(yaw), 0.0], [np.sin(yaw), np.cos(yaw), 0.0], [0.0, 0.0, 1.0]])


def to_raw(name, P):
    """GLB (as imported) to raw: turned to face -Y, feet on z 0."""
    c = cfg.PEOPLE[name]
    return np.asarray(P, float) @ _yaw(name).T + np.array([0.0, 0.0, c.get('floor', 0.0)])


def upright_fit(name):
    """(s, t): upright = raw * s + t."""
    J = measure(name)
    sh, hip = np.asarray(J['shoulder_l'], float), np.asarray(J['hips'], float)
    s = (SHOULDER_Z - HIP_Z) / (sh[2] - hip[2])
    t = np.array([0.0, -hip[1] * s, HIP_Z - hip[2] * s])
    return s, t


def to_upright(name, P):
    s, t = upright_fit(name)
    return np.asarray(P, float) * s + t


_MEASURED = {}


def measure(name, V=None):
    """The joints estimated from the raw mesh (arms by their line, the hands by cfg 'hand' length, the hips at a share
    of the height), then cfg 'joints' over them: {key: raw point} (left side; joints() mirrors the right)."""
    if name in _MEASURED:
        return _MEASURED[name]
    c = cfg.PEOPLE[name]
    if V is None:
        obj, maps = tripo.load(glb_path(name))
        V, _ = tr.mesh_arrays(obj.data)
        bpy.data.objects.remove(obj)
        for image in [m for m in maps.values() if m]:
            bpy.data.images.remove(image)
        V = to_raw(name, V)
    J = {}
    crown = V[:, 2].max()
    # The left arm: its line through everything beyond the torso, its tip the fist's or fingers' end.
    arm = V[V[:, 0] > c.get('arm_from', 0.32)]
    mid = arm.mean(0)
    d = np.linalg.svd(arm - mid)[2][0]
    d = d if d[0] > 0 else -d
    t = (arm - mid) @ d
    tip = t.max()

    def section(at, reach=0.012):
        near = np.abs(t - at) < reach
        return arm[near].mean(0) if near.sum() > 8 else mid + d * at
    hand = c.get('hand', 0.17)
    wrist = section(tip - hand)
    knuckles = section(tip - hand * c.get('knuckle_share', 0.55))
    # The shoulder: on the arm's line where it crosses shoulder_x, its height from the line.
    sx = c.get('shoulder_x', 0.18)
    a = (sx - mid[0]) / d[0]
    shoulder = mid + d * a
    elbow = shoulder + (wrist - shoulder) * c.get('elbow_share', 0.53)
    elbow = section((elbow - mid) @ d) if (elbow - mid) @ d > t.min() + 0.02 else elbow
    J.update(shoulder_l=shoulder, elbow_l=elbow, wrist_l=wrist, knuckles_l=knuckles)
    hips_z = c.get('hips_share', 0.515) * crown
    slab = V[(np.abs(V[:, 2] - hips_z) < 0.03) & (np.abs(V[:, 0]) < 0.15)]
    J['hips'] = np.array([0.0, 0.5 * (slab[:, 1].min() + slab[:, 1].max()), hips_z])
    J['crown'] = np.array([0.0, 0.0, crown])
    for key, p in c['joints'].items():
        J[key] = np.asarray(p, float) if isinstance(p, (tuple, list)) and len(p) == 3 else p
    _MEASURED[name] = J
    return J


def joints(name, space='upright'):
    """The measured joints, the right side mirrored from the left unless measured too: {key: point} (raw or upright)."""
    J = {}
    for key, p in measure(name).items():
        if not isinstance(p, (tuple, list, np.ndarray)) or len(p) != 3:
            continue
        p = np.asarray(p, float)
        J[key] = p
        if key.endswith('_l') and key[:-2] + '_r' not in cfg.PEOPLE[name]['joints'] and key[:-2] + '_r' not in J:
            q = p.copy()
            q[0] *= -1.0
            J[key[:-2] + '_r'] = q
    if space == 'upright':
        J = {k: to_upright(name, v) for k, v in J.items()}
    return J


def source_object(name, space='raw'):
    """The GLB loaded, moved into raw (or upright) space: (object, V, faces). The object keeps Tripo's UVs; its colour
    map is the image named in obj['BC']."""
    obj, maps = tripo.load(glb_path(name))
    V, F = tr.mesh_arrays(obj.data)
    V = to_raw(name, V)
    if space == 'upright':
        V = to_upright(name, V)
    obj.data.vertices.foreach_set('co', V.astype(np.float32).ravel())
    obj.data.update()
    obj['BC'] = maps['BC'].name
    for image in (maps['RM'], maps['N']):
        if image is not None:
            bpy.data.images.remove(image)
    return obj, V, F


# --- What goes: the legs below the hems (the script's own shroud takes their place) ---

def cut_legs(name, V, F):
    """Faces under the person's cut (cfg 'cut': raw z, or a list of (z, (x0, x1)) bands: the lowest z that applies to
    a face's middle) dropped, with whatever pieces are left floating; returns (V, faces)."""
    c = cfg.PEOPLE[name]
    rules = c['cut'] if isinstance(c['cut'], (list, tuple)) else [(c['cut'], (-9.0, 9.0))]
    P = np.array([V[f].mean(0) for f in F])
    drop = np.zeros(len(F), bool)
    for z, (x0, x1) in rules:
        drop |= (P[:, 2] < z) & (P[:, 0] >= x0) & (P[:, 0] <= x1)
    for box in c.get('drop_boxes', ()):          # (x0, x1, y0, y1, z0, z1): stray Tripo bits (a bare foot's strap)
        lo, hi = np.array(box[0::2]), np.array(box[1::2])
        drop |= ((P >= lo) & (P <= hi)).all(1)
    keep = ~drop
    Fs = [f for f, k in zip(F, keep) if k]
    roots = _components(Fs, len(V))
    ids, counts = np.unique(roots, return_counts=True)
    big = np.isin(roots, ids[counts >= counts.max() * 0.002])
    Fs = [f for f, k in zip(Fs, big) if k]
    used = np.unique(np.concatenate(Fs))
    remap = -np.ones(len(V), int)
    remap[used] = np.arange(len(used))
    log(f'{name}: the legs cut: {int(drop.sum())} faces under the hems, {int((~big).sum())} floating faces dropped')
    return V[used], [remap[f] for f in Fs]


def _components(F, n):
    parent = np.arange(n)

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    for f in F:
        r = find(f[0])
        for v in f[1:]:
            q = find(v)
            if q != r:
                parent[q] = r
    return np.array([find(f[0]) for f in F])


# --- Even quads (cached: QuadriFlow takes a minute or two) ---

def _key(name, *parts):
    h = hashlib.sha1()
    with open(glb_path(name), 'rb') as f:
        h.update(f.read())
    h.update(repr(parts).encode())
    for helper in (tr.__file__, os.path.join(os.path.dirname(tr.__file__), 'looter_tripo_shape.py')):
        h.update(open(helper, 'rb').read())
    h.update(open(__file__, 'rb').read())
    return h.hexdigest()[:16]


def remesh(name, V, F):
    """The cut model in even quads (upright space), the hands and face blown up first for smaller quads there."""
    c = cfg.PEOPLE[name]
    J = joints(name)
    regions = [(J['face'], 1.0, 0.1, 0.16), (J['knuckles_l'], 0.6, 0.08, 0.14), (J['knuckles_r'], 0.6, 0.08, 0.14)]
    if c.get('no_blowup'):
        regions = []
    quads = c.get('tris', 16000)
    path = os.path.join(CACHE, f'{name}_{_key(name, quads, repr(regions), c.get("cut"), c.get("drop_boxes"))}.npz')
    if os.path.exists(path):
        data = np.load(path)
        sizes, flat = data['sizes'], data['flat']
        Fq, at = [], 0
        for n in sizes:
            Fq.append(flat[at:at + n])
            at += n
        return data['V'], Fq
    if c.get('remesh') == 'voxel':
        free = np.ones(len(V))
        for key in ('knuckles_l', 'knuckles_r', 'face'):     # fingers and the face keep their own thickness
            free *= np.clip((np.linalg.norm(V - J[key], axis=1) - 0.12) / 0.05, 0.0, 1.0)
        Vt = tr.thicken(V, F, c.get('thin', 0.008), where=free)
        Vq, Fq = quad_remesh(Vt, F, quads, regions, voxel=c.get('voxel', 0.004), name=f'_{name}_quads')
    else:
        Vq, Fq = collapse(V, F, quads, name=f'_{name}_collapse')
    Vq, Fq = _drop_specks(Vq, Fq, share=0.002)
    os.makedirs(CACHE, exist_ok=True)
    np.savez(path, V=Vq, flat=np.concatenate(Fq), sizes=np.array([len(f) for f in Fq]))
    return Vq, Fq


def collapse(V, faces, tris, name='_collapse'):
    """Tripo's own surface (cut) collapsed to about tris triangles: its shape and thin cloth as made, its vertices
    still on its surface. (A voxel remesh closed the thin cloth into two close sheets that the collapse could not take
    below 75-90k triangles, and QuadriFlow ran out of memory on the shared PC, 2026-11-01.)"""
    obj = tr.new_object(name, V, faces)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bm.to_mesh(obj.data)
    bm.free()
    n0 = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    for _ in range(4):
        now = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        if now <= tris * 1.08:
            break
        mod = obj.modifiers.new('collapse', 'DECIMATE')
        mod.decimate_type = 'COLLAPSE'
        mod.ratio = max(0.05, min(1.0, tris / now))
        mod.use_collapse_triangulate = True
        with bpy.context.temp_override(active_object=obj, object=obj):
            bpy.ops.object.modifier_apply(modifier=mod.name)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=1e-6)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()
    Vq, Fq = tr.mesh_arrays(obj.data)
    bpy.data.objects.remove(obj)
    log(f'{name}: the Tripo model, {n0} triangles, collapsed to {len(Fq)}')
    return Vq, Fq


def quad_remesh(V, faces, tris, regions=(), voxel=0.003, name='_quads', share=0.01):
    """An even surface of about tris triangles over (V, faces): a voxel remesh (watertight, even), the crumbs under
    share of the largest piece dropped (Tripo's coat insides), then collapsed down to tris with the regions (centre,
    weight, inner radius, outer radius: the face, the hands) collapsed less, so they keep smaller triangles.
    (QuadriFlow, the Tripo Unpaid's way, ran out of memory with the blown-up face on these T-posed people, 2026-11-01.)"""
    obj = tr.new_object(name, V, faces)
    obj.data.remesh_voxel_size = voxel
    obj.data.remesh_voxel_adaptivity = 0.0
    with bpy.context.temp_override(active_object=obj, object=obj, selected_objects=[obj],
                                   selected_editable_objects=[obj]):
        bpy.ops.object.voxel_remesh()
    Vv, Fv = tr.mesh_arrays(obj.data)
    bpy.data.objects.remove(obj)
    roots = _components(Fv, len(Vv))
    ids, counts = np.unique(roots, return_counts=True)
    big = ids[counts >= counts.max() * share]
    keep = np.isin(roots, big)
    Fv = [f for f, k in zip(Fv, keep) if k]
    used = np.unique(np.concatenate(Fv))
    remap = -np.ones(len(Vv), int)
    remap[used] = np.arange(len(used))
    Vv, Fv = Vv[used], [remap[f] for f in Fv]
    obj = tr.new_object(name, Vv, Fv)
    n0 = sum(len(f) - 2 for f in Fv)
    # Collapse keeps triangles where the surface turns (the face, the fingers, folds) and spends few on flat cloth.
    for _ in range(6):          # one collapse stops short of a big cut (919k to 93k for 19k asked): again till there
        now = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        if now <= tris * 1.08:
            break
        mod = obj.modifiers.new('collapse', 'DECIMATE')
        mod.decimate_type = 'COLLAPSE'
        mod.ratio = max(0.05, min(1.0, tris / now))
        mod.use_collapse_triangulate = True
        with bpy.context.temp_override(active_object=obj, object=obj):
            bpy.ops.object.modifier_apply(modifier=mod.name)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=1e-5)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()
    Vq, Fq = tr.mesh_arrays(obj.data)
    bpy.data.objects.remove(obj)
    log(f'{name}: voxels in {len(ids)} pieces, {len(big)} kept ({n0} triangles), collapsed to {len(Fq)}')
    return Vq, Fq


def _quadriflow(Vv, Fv, quads, regions, name):
    Vm = tr.magnify(Vv, regions) if regions else Vv
    obj = tr.new_object(name, Vm * 100.0, Fv)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    result = bpy.ops.object.quadriflow_remesh(target_faces=int(quads), use_preserve_sharp=False,
                                              use_preserve_boundary=False, use_mesh_symmetry=False,
                                              smooth_normals=False, seed=0, mode='FACES')
    if 'FINISHED' not in result:
        raise RuntimeError(f'QuadriFlow failed on {name}')
    Vq, Fq = tr.mesh_arrays(obj.data)
    Vq = Vq / 100.0
    bpy.data.objects.remove(obj)
    blown = tr.Surface(Vm, Fv)
    tri, bary, _, _ = blown.nearest(Vq)
    return blown.point(tri, bary, Vv), Fq


def _drop_specks(V, F, share=0.01):
    roots = _components(F, len(V))
    ids, counts = np.unique(roots, return_counts=True)
    keep = np.isin(roots, ids[counts >= counts.max() * share])
    Fs = [f for f, k in zip(F, keep) if k]
    used = np.unique(np.concatenate(Fs))
    remap = -np.ones(len(V), int)
    remap[used] = np.arange(len(used))
    return V[used], [remap[f] for f in Fs]


def close_holes(V, F):
    """Tripo's pinholes filled (bmesh): (V, faces)."""
    obj = tr.new_object('_holes', V, F)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.holes_fill(bm, edges=bm.edges, sides=12)
    bm.to_mesh(obj.data)
    bm.free()
    V2, F2 = tr.mesh_arrays(obj.data)
    bpy.data.objects.remove(obj)
    return V2, F2
