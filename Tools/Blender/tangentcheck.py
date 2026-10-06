"""Tangent check: what Unreal's import will make of an exported model's tangents, before Unreal sees it.

Unreal's import (legacy FBX, normals imported, tangents by MikkTSpace) checks every vertex of the built mesh and logs
  "nearly zero tangents" / "nearly zero bi-normals"   a vertex whose tangent (or normal x tangent) came out zero;
  "degenerate tangent bases"                         a vertex whose tangent is its own normal.
This reads the FBX itself (no Blender import, so the normals are exactly the file's), converts it the way Unreal's
importer does (Blender's -Y forward becomes +X, -X becomes +Y, centimeters, V flipped, float32, triangles with two
corners together dropped), and runs a float32 port of mikktspace on it as Unreal calls it: one face per triangle,
corners welded only where position, normal and UV are all equal. Per mesh:
  ZERO        corners whose tangent sums to exactly zero: every face of their welded fan that adds to it does so at
              a zero angle (a sliver's sharp corner rounds to acos(1) = 0) or along the normal.
  DEFAULT     corners of faces without UV area that no good face took in: mikktspace leaves them its default +X
              tangent. Along a normal facing Unreal's +X (Blender's -Y, the front) that is a degenerate basis, facing
              -X a zero binormal; other normals get a wrong but quiet tangent ("of N corners left at +X").
  WEAK        corners whose summed tangent or bitangent is under 1e-4 before normalisation (no warning, but close).
  NO UV AREA  faces with no UV area (the cause of DEFAULT corners), NO AREA faces with no area.
  REDUCED     the model's manifest asks Unreal for a reduced Nanite fallback or LODs. Unreal makes those itself and
              recomputes their tangents (NaniteBuilder's CalcTangents, MikkTSpace without degenerate fix-ups, on its
              simplified mesh); this check covers the full mesh only. A warning from such a build is not in the FBX.
Exit code 1 when Unreal would warn about a full mesh.

  Tools/artrun.ps1 -Script Tools/Blender/tangentcheck.py -ScriptArgs <fbx or folder>[,<fbx or folder>...][,--where][,--all]
  Tools/artrun.ps1 -Script Tools/Blender/tangentcheck.py -ScriptArgs --log[,<editor log>]
(or blender -b --factory-startup --python Tools/Blender/tangentcheck.py -- <the same arguments>; paths are relative to
the project, where artrun.ps1 runs it)
--where lists where (Blender's coordinates, meters) and runs Blender's own MikkTSpace too (mesh.calc_tangents on the
imported custom normals: it gives such corners a tangent anyway, so it can't stand in for Unreal's); --all lists
clean models too. --log reads the editor's log (default Saved/Logs/AI_Looter_Shooter.log) and sorts every tangent
warning by the build that logged it: a full mesh (fix the model) or a reduced Nanite fallback / LODs (Unreal's own
reduction; before 4529a39 the import's first build also used Unreal's default fallback, before ModelImporter set the
model's, and since then Nanite builds once, with the model's own).

Checked against Unreal's import of 2026-10-02 (the logs): it flags SM_Skiff_A_Packet and SM_Skiff_A_Gang, the only
models whose full mesh Unreal warned about, and passes every other model exported for Ransom's Rest; the warnings on
SM_Ruin_LanternHouse, SM_FalseFront_Saloon, SM_FalseFront_Store and SM_Outcrop_TorB all came from reduced builds."""
import json
import math
import os
import re
import sys

import numpy as np

F32 = np.float32
FLT_MIN = np.finfo(np.float32).tiny
ONE, ZERO_F = F32(1.0), F32(0.0)
# The project folder: this file is Tools/Blender/tangentcheck.py.
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

args = [a for arg in sys.argv[sys.argv.index('--') + 1:] for a in arg.split(',') if a] if '--' in sys.argv else []
WHERE = '--where' in args
SHOW_ALL = '--all' in args
LOG = '--log' in args
paths = []
for a in (a for a in args if not a.startswith('--')):
    if os.path.isdir(a):
        paths += sorted(os.path.join(a, f) for f in os.listdir(a) if f.lower().endswith('.fbx'))
    else:
        paths.append(a)

# --- Reading the FBX (Blender's own binary parser) ---

def kid(elem, name):
    return next((e for e in elem.elems if e.id == name), None)


def read_fbx(path):
    """The file's meshes, other than collision hulls: (name, positions, polygons, normals per corner, UVs per corner,
    world transform) with the transform as a 4x4 (double) of the model chain."""
    from io_scene_fbx import parse_fbx
    root, _ = parse_fbx.parse(path)
    objects = kid(root, b'Objects')
    models, geometries, parent, geometry_model = {}, {}, {}, {}
    for e in objects.elems:
        if e.id == b'Model':
            models[e.props[0]] = e
        elif e.id == b'Geometry' and e.props[2] == b'Mesh':
            geometries[e.props[0]] = e
    for c in kid(root, b'Connections').elems:
        if c.props[0] != b'OO':
            continue
        child, owner = c.props[1], c.props[2]
        if child in geometries and owner in models:
            geometry_model[child] = owner
        elif child in models:
            parent[child] = owner

    def local(model):
        values = {b'Lcl Translation': (0.0, 0.0, 0.0), b'Lcl Rotation': (0.0, 0.0, 0.0),
                  b'Lcl Scaling': (1.0, 1.0, 1.0)}
        props = kid(model, b'Properties70')
        for p in props.elems if props else []:
            if p.props[0] in values:
                values[p.props[0]] = tuple(float(x) for x in p.props[4:7])
        t, r, s = values[b'Lcl Translation'], values[b'Lcl Rotation'], values[b'Lcl Scaling']
        rx, ry, rz = (math.radians(a) for a in r)
        mx = np.array([[1, 0, 0], [0, math.cos(rx), -math.sin(rx)], [0, math.sin(rx), math.cos(rx)]])
        my = np.array([[math.cos(ry), 0, math.sin(ry)], [0, 1, 0], [-math.sin(ry), 0, math.cos(ry)]])
        mz = np.array([[math.cos(rz), -math.sin(rz), 0], [math.sin(rz), math.cos(rz), 0], [0, 0, 1]])
        m = np.eye(4)
        m[:3, :3] = mz @ my @ mx @ np.diag(s)          # FBX's XYZ Euler order: X first
        m[:3, 3] = t
        return m

    meshes = []
    for gid, geo in geometries.items():
        mid = geometry_model.get(gid)
        if mid is None:
            continue
        name = models[mid].props[1].split(b'\x00\x01')[0].decode()
        if name.startswith('UCX_'):
            continue
        world = np.eye(4)
        node = mid
        while node in models:
            world = local(models[node]) @ world
            node = parent.get(node)
        verts = np.array(kid(geo, b'Vertices').props[0], dtype=np.float64).reshape(-1, 3)
        pvi = list(kid(geo, b'PolygonVertexIndex').props[0])
        polygons, current = [], []
        for i in pvi:
            if i < 0:
                current.append(~i)
                polygons.append(current)
                current = []
            else:
                current.append(i)
        corners = sum(len(p) for p in polygons)

        def layer(elem_name, data_name, index_name, width):
            el = kid(geo, elem_name)
            if el is None:
                return None
            data = np.array(kid(el, data_name).props[0], dtype=np.float64).reshape(-1, width)
            mapping = kid(el, b'MappingInformationType').props[0]
            reference = kid(el, b'ReferenceInformationType').props[0]
            if mapping == b'ByPolygonVertex':
                if reference == b'IndexToDirect':
                    return data[np.array(kid(el, index_name).props[0])]
                return data
            if mapping in (b'ByVertice', b'ByVertex'):
                flat = [v for p in polygons for v in p]
                return data[np.array(flat)]
            if mapping == b'ByPolygon':
                return np.repeat(data, [len(p) for p in polygons], axis=0)
            raise ValueError(f'{name}: unexpected {elem_name} mapping {mapping}')
        normals = layer(b'LayerElementNormal', b'Normals', b'NormalsIndex', 3)
        uvs = layer(b'LayerElementUV', b'UV', b'UVIndex', 2)
        if normals is None or uvs is None or len(normals) != corners or len(uvs) != corners:
            print(f'TANGENTS {name}: no normals or UVs per corner; skipped', flush=True)
            continue
        meshes.append((name, verts, polygons, normals, uvs, world))
    return meshes


# --- Into Unreal's space, as its importer does it ---

TO_BLENDER = np.array([[1.0, 0.0, 0.0, 0.0], [0.0, 0.0, -1.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 0.0, 1.0]])


def to_unreal_rows(v):
    """Blender (x, y, z) to Unreal (-y, -x, z), as Tools/Blender/looter_export.to_unreal (the sockets agree)."""
    return np.stack([-v[:, 1], -v[:, 0], v[:, 2]], axis=1)


def unreal_mesh(verts, polygons, normals, uvs, world):
    """Float32 corners per triangle as Unreal's FBX importer builds them: positions through the model's transform
    (FBX's Y-up axes back to Blender's, then Unreal's, in cm), normals through its inverse transpose and normalized,
    UVs with V flipped; polygons over three corners fanned from their first (FBX SDK's quads). Returns positions,
    normals, UVs (each per triangle corner), the Blender position of each triangle corner (for --where) and the
    count of polygons that had to be cut."""
    m = TO_BLENDER @ world                 # the exporter turned Blender's Z-up into FBX's Y-up on the root
    lin = m[:3, :3]
    pos_b = verts @ lin.T + m[:3, 3]
    pos_u = to_unreal_rows(pos_b)
    normal_matrix = np.linalg.inv(lin).T
    nrm_b = normals @ normal_matrix.T
    nrm_u = to_unreal_rows(nrm_b).astype(np.float32)
    lengths = np.sqrt((nrm_u.astype(np.float32) ** 2).sum(axis=1, dtype=np.float32)).astype(np.float32)
    safe = np.where(lengths > 0, lengths, 1).astype(np.float32)
    nrm_u = np.where(lengths[:, None] > 0, nrm_u / safe[:, None], 0).astype(np.float32)
    uv_u = np.stack([uvs[:, 0], 1.0 - uvs[:, 1]], axis=1).astype(np.float32)
    pos_f = pos_u.astype(np.float32)
    tri_v, tri_c, cut = [], [], 0
    corner = 0
    for p in polygons:
        if len(p) > 3:
            cut += 1
        for k in range(1, len(p) - 1):
            tri_v.append((p[0], p[k], p[k + 1]))
            tri_c.append((corner, corner + k, corner + k + 1))
        corner += len(p)
    tri_v = np.array(tri_v, dtype=np.int64)
    tri_c = np.array(tri_c, dtype=np.int64)
    return pos_f[tri_v], nrm_u[tri_c], uv_u[tri_c], pos_b[tri_v], cut


# --- mikktspace (Morten S. Mikkelsen), float32, as Unreal's MeshDescription interface feeds it ---

def nz(x):
    return abs(x) > FLT_MIN


def vnz(v):
    return nz(v[0]) or nz(v[1]) or nz(v[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def length(v):
    return F32(math.sqrt(dot(v, v)))


def normalize(v):
    s = ONE / length(v)
    return (v[0] * s, v[1] * s, v[2] * s)


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def scale(s, v):
    return (s * v[0], s * v[1], s * v[2])


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


GROUP_WITH_ANY, ORIENT_PRESERVING = 4, 8


def mikk(P, N, T):
    """P, N per triangle corner (n, 3, 3) float32, T (n, 3, 2). Returns per corner: tangent (3 floats; None where
    it stays mikktspace's default +X), orientation, the summed tangent and bitangent before normalisation, and
    whether the triangle is good."""
    n = len(P)
    pts = [[tuple(F32(c) for c in P[t, i]) for i in range(3)] for t in range(n)]
    nrm = [[tuple(F32(c) for c in N[t, i]) for i in range(3)] for t in range(n)]
    tex = [[(F32(T[t, i, 0]), F32(T[t, i, 1])) for i in range(3)] for t in range(n)]
    # Weld: corners with the same position, normal and UV share an index (the first one's).
    first = {}
    weld = np.empty(n * 3, dtype=np.int64)
    for t in range(n):
        for i in range(3):
            key = (pts[t][i], nrm[t][i], tex[t][i])
            weld[t * 3 + i] = first.setdefault(key, t * 3 + i)

    def pos(index):
        return pts[index // 3][index % 3]

    def nor(index):
        return nrm[index // 3][index % 3]

    def uvw(index):
        return tex[index // 3][index % 3]

    tri = [tuple(int(weld[t * 3 + i]) for i in range(3)) for t in range(n)]
    # DegenPrologue: a triangle with two corners at the same position is degenerate (its corners copy others later).
    degenerate = [pts[t][0] == pts[t][1] or pts[t][0] == pts[t][2] or pts[t][1] == pts[t][2] for t in range(n)]
    good_tris = [t for t in range(n) if not degenerate[t]]
    flags = {t: GROUP_WITH_ANY for t in good_tris}
    vos, vot, mag_s, mag_t = {}, {}, {}, {}
    zero3 = (ZERO_F, ZERO_F, ZERO_F)
    for t in good_tris:
        i0, i1, i2 = tri[t]
        v1, v2, v3 = pos(i0), pos(i1), pos(i2)
        t1, t2, t3 = uvw(i0), uvw(i1), uvw(i2)
        t21x, t21y = t2[0] - t1[0], t2[1] - t1[1]
        t31x, t31y = t3[0] - t1[0], t3[1] - t1[1]
        d1, d2 = sub(v2, v1), sub(v3, v1)
        area = t21x * t31y - t21y * t31x
        os_ = sub(scale(t31y, d1), scale(t21y, d2))
        ot_ = add(scale(-t31x, d1), scale(t21x, d2))
        if area > 0:
            flags[t] |= ORIENT_PRESERVING
        vos[t], vot[t], mag_s[t], mag_t[t] = zero3, zero3, ZERO_F, ZERO_F
        if nz(area):
            absarea = F32(abs(area))
            los, lot = length(os_), length(ot_)
            s = ONE if area > 0 else F32(-1.0)
            if nz(los):
                vos[t] = scale(s / los, os_)
            if nz(lot):
                vot[t] = scale(s / lot, ot_)
            mag_s[t], mag_t[t] = los / absarea, lot / absarea
            if nz(mag_s[t]) and nz(mag_t[t]):
                flags[t] &= ~GROUP_WITH_ANY
    # Neighbors: edges with the same welded ends, run the other way; the first unpaired match in (i0, i1, f) order.
    edges = {}
    for t in good_tris:
        for e in range(3):
            a, b = tri[t][e], tri[t][(e + 1) % 3]
            edges.setdefault((min(a, b), max(a, b)), []).append((t, e, a, b))
    neighbor = {t: [-1, -1, -1] for t in good_tris}
    for key in sorted(edges):
        entries = sorted(edges[key], key=lambda x: (x[0], x[1]))
        for k, (fa, ea, a0, a1) in enumerate(entries):
            if neighbor[fa][ea] != -1:
                continue
            for fb, eb, b0, b1 in entries[k + 1:]:
                if b0 == a1 and b1 == a0 and neighbor[fb][eb] == -1:
                    neighbor[fa][ea] = fb
                    neighbor[fb][eb] = fa
                    break
    # Groups (Build4RuleGroups / AssignRecur): faces round a welded corner, joined across shared edges, of one
    # orientation; faces without UV area join the first group that reaches them.
    assigned = {t: [None, None, None] for t in good_tris}
    groups = []
    sys.setrecursionlimit(max(10000, n * 4))

    def assign(t, group):
        rep = group['rep']
        i = tri[t].index(rep)
        if assigned[t][i] is group:
            return True
        if assigned[t][i] is not None:
            return False
        if flags[t] & GROUP_WITH_ANY and assigned[t] == [None, None, None]:
            flags[t] &= ~ORIENT_PRESERVING
            if group['orient']:
                flags[t] |= ORIENT_PRESERVING
        if bool(flags[t] & ORIENT_PRESERVING) != group['orient']:
            return False
        group['faces'].append(t)
        assigned[t][i] = group
        left, right = neighbor[t][i], neighbor[t][i - 1 if i > 0 else 2]
        if left >= 0:
            assign(left, group)
        if right >= 0:
            assign(right, group)
        return True

    for t in good_tris:
        for i in range(3):
            if not flags[t] & GROUP_WITH_ANY and assigned[t][i] is None:
                group = {'rep': tri[t][i], 'orient': bool(flags[t] & ORIENT_PRESERVING), 'faces': [t]}
                groups.append(group)
                assigned[t][i] = group
                left, right = neighbor[t][i], neighbor[t][i - 1 if i > 0 else 2]
                if left >= 0:
                    assign(left, group)
                if right >= 0:
                    assign(right, group)
    # Tangent spaces per group: subgroups by tangent direction (the 180 degree threshold merges all but exact
    # opposites), each the angle-weighted sum of its good faces' projected tangents.
    result = {}
    thres = F32(math.cos(math.pi))

    def projected(t, nrm_v):
        a = sub(vos[t], scale(dot(nrm_v, vos[t]), nrm_v))
        b = sub(vot[t], scale(dot(nrm_v, vot[t]), nrm_v))
        if vnz(a):
            a = normalize(a)
        if vnz(b):
            b = normalize(b)
        return a, b

    def eval_tspace(members, rep):
        res_s, res_t = zero3, zero3
        for t in members:
            if flags[t] & GROUP_WITH_ANY:
                continue
            i = tri[t].index(rep)
            nrm_v = nor(rep)
            s_, t_ = projected(t, nrm_v)
            p0, p1, p2 = pos(tri[t][i - 1 if i > 0 else 2]), pos(tri[t][i]), pos(tri[t][(i + 1) % 3])
            e1, e2 = sub(p0, p1), sub(p2, p1)
            e1 = sub(e1, scale(dot(nrm_v, e1), nrm_v))
            if vnz(e1):
                e1 = normalize(e1)
            e2 = sub(e2, scale(dot(nrm_v, e2), nrm_v))
            if vnz(e2):
                e2 = normalize(e2)
            c = dot(e1, e2)
            c = ONE if c > 1 else (F32(-1.0) if c < -1 else c)
            angle = F32(math.acos(float(c)))
            res_s = add(res_s, scale(angle, s_))
            res_t = add(res_t, scale(angle, t_))
        return res_s, res_t

    for group in groups:
        rep = group['rep']
        nrm_v = nor(rep)
        cache = {}
        proj = {t: projected(t, nrm_v) for t in group['faces']}
        for f in group['faces']:
            s_f, t_f = proj[f]
            members = []
            for t in group['faces']:
                s_t, t_t = proj[t]
                any_ = (flags[f] | flags[t]) & GROUP_WITH_ANY
                if any_ or f == t or (dot(s_f, s_t) > thres and dot(t_f, t_t) > thres):
                    members.append(t)
            members = tuple(sorted(members))
            if members not in cache:
                cache[members] = eval_tspace(members, rep)
            raw_s, raw_t = cache[members]
            i = tri[f].index(rep)
            tangent = normalize(raw_s) if vnz(raw_s) else raw_s
            result[(f, i)] = (tangent, group['orient'], raw_s, raw_t)
    # Degenerate triangles copy a good corner with the same welded index (DegenEpilogue).
    by_index = {}
    for (t, i), value in result.items():
        by_index.setdefault(tri[t][i], value)
    for t in range(n):
        if degenerate[t]:
            for i in range(3):
                if tri[t][i] in by_index:
                    result[(t, i)] = by_index[tri[t][i]]
    good = {t: not (flags.get(t, GROUP_WITH_ANY) & GROUP_WITH_ANY) for t in range(n)}
    return result, good, tri, degenerate


# --- Unreal's check of the built vertices (StaticMeshBuild.cpp HasBadNTB) ---

def check(name, P, N, T, B, cut):
    n = len(P)
    result, good, tri, degenerate = mikk(P, N, T)
    zero, default, weak, bad_basis, left_default = [], [], [], [], []
    for t in range(n):
        for i in range(3):
            normal = tuple(float(c) for c in N[t, i])
            value = result.get((t, i))
            if value is None:
                tangent, raw_s, raw_t = (1.0, 0.0, 0.0), None, None
            else:
                tangent, _, raw_s, raw_t = value
                tangent = tuple(float(c) for c in tangent)
            where = tuple(round(float(c), 3) for c in B[t, i])
            tx = np.array(tangent)
            nz_ = np.array(normal)
            binormal = np.cross(nz_, tx)
            zero_t = np.all(np.abs(tx) <= 1e-4)
            zero_b = np.all(np.abs(binormal) <= 1e-4)
            degenerate_basis = np.all(np.abs(tx - nz_) <= 1.0 / 255.0)
            if value is None:
                left_default.append(where)
                if zero_b or degenerate_basis:
                    default.append((where, normal, 'degenerate basis' if degenerate_basis else 'zero binormal'))
                continue
            if zero_t or zero_b:
                zero.append((where, normal, 'zero tangent' if zero_t else 'zero binormal'))
            elif degenerate_basis:
                bad_basis.append((where, normal, 'tangent along the normal'))
            if raw_s is not None and (float(length(raw_s)) < 1e-4 or float(length(raw_t)) < 1e-4):
                weak.append((where, float(length(raw_s)), float(length(raw_t))))
    no_uv, no_area = [], []
    for t in range(n):
        if degenerate[t]:
            no_area.append(tuple(round(float(c), 3) for c in B[t].mean(axis=0)))
            continue
        d1, d2 = B[t, 1] - B[t, 0], B[t, 2] - B[t, 0]
        area = 0.5 * np.linalg.norm(np.cross(d1, d2))
        e1, e2 = T[t, 1].astype(np.float64) - T[t, 0], T[t, 2].astype(np.float64) - T[t, 0]
        uv_area = 0.5 * abs(e1[0] * e2[1] - e1[1] * e2[0])
        if area < 1e-8:
            no_area.append(tuple(round(float(c), 3) for c in B[t].mean(axis=0)))
        if not good[t] or uv_area < 1e-12:
            no_uv.append(tuple(round(float(c), 3) for c in B[t].mean(axis=0)))
    bad = len(zero) + len(default) + len(bad_basis)
    if bad or weak or no_uv or no_area or SHOW_ALL:
        verdict = 'UNREAL WILL WARN' if bad else 'ok'
        print(f'TANGENTS {name}: {verdict}: {n} triangles ({cut} polygons cut); ZERO {len(zero)}, DEFAULT '
              f'{len(default)} (of {len(left_default)} corners left at +X), tangent along normal {len(bad_basis)}; '
              f'WEAK sums {len(weak)}, faces without UV area {len(no_uv)}, without area {len(no_area)}', flush=True)
        if WHERE:
            for label, items in (('ZERO', zero), ('DEFAULT', default), ('ALONG', bad_basis), ('WEAK', weak),
                                 ('NO UV AREA', no_uv), ('NO AREA', no_area)):
                for item in items[:8]:
                    print(f'TANGENTS   {label} {item}', flush=True)
    return bad


def blender_check(path):
    """Blender's own MikkTSpace on the imported FBX (custom normals kept): loops with a zero tangent or one along
    the normal, per mesh."""
    import bpy
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=path, use_custom_normals=True)
    counts = {}
    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH' or obj.name.startswith('UCX_') or not obj.data.uv_layers:
            continue
        mesh = obj.data
        try:
            mesh.calc_tangents()
        except RuntimeError as error:
            counts[obj.name] = f'calc_tangents failed: {error}'
            continue
        zero = along = 0
        for loop in mesh.loops:
            t = loop.tangent
            if t.length < 1e-4:
                zero += 1
            elif abs(t.normalized().dot(loop.normal)) > 0.999:
                along += 1
        counts[obj.name] = f'{zero} zero, {along} along the normal'
        mesh.free_tangents()
    return counts


def manifest_entries(folder):
    """The models' manifest entries (Tools/Blender/looter_export.py) in an export folder, by FBX file name."""
    entries = {}
    for f in os.listdir(folder):
        if f.lower().endswith('.json'):
            try:
                with open(os.path.join(folder, f), encoding='utf-8') as handle:
                    for entry in json.load(handle).get('models', []):
                        entries[entry.get('fbx', '')] = entry
            except (OSError, ValueError, AttributeError):
                pass
    return entries


def reduced_note(entry):
    """What Unreal will reduce on its own for a manifest entry, or ''."""
    if not entry:
        return ''
    if entry.get('nanite', True):
        share = entry.get('fallbackPercent')
        if share is None or share < 100.0:
            return f"REDUCED: Nanite fallback {'Unreal default' if share is None else f'{share:g}%'}"
        return ''
    lods = entry.get('lods')
    return f"REDUCED: LODs {','.join(f'{x:g}' for x in lods)}%" if lods else ''


def read_log(path):
    """Every tangent warning in an editor log, with the build that logged it: the static mesh, whether the build was
    the first after an FBX import (Unreal's default Nanite fallback) and the triangles in and in the fallback."""
    builds, current, out = {}, {}, []
    start = re.compile(r'FactoryCreateFile: StaticMesh with FbxFactory .*/(SM_\w+)\.fbx')
    building = re.compile(r'Building static mesh (SM_\w+)')
    adjacency = re.compile(r'Adjacency \[[^]]*\], tris: (\d+)')
    fallback = re.compile(r'Fallback \[[^]]*\], num tris: (\d+)')
    warning = re.compile(r'LogStaticMesh: Warning: (SM_\w+) has (degenerate tangent bases|some nearly zero [\w-]+)')
    last = None
    with open(path, encoding='utf-8', errors='replace') as handle:
        for line in handle:
            m = start.search(line)
            if m:
                builds[m.group(1)] = 0
                continue
            m = building.search(line)
            if m:
                name = m.group(1)
                if 'Waiting' not in line:
                    builds[name] = builds.get(name, 0) + 1
                    current[name] = {'build': builds[name], 'tris': None, 'fallback': None}
                    last = name
                continue
            m = adjacency.search(line)
            if m and last in current:
                current[last]['tris'] = int(m.group(1))
                continue
            m = fallback.search(line)
            if m and last in current:
                current[last]['fallback'] = int(m.group(1))
                continue
            m = warning.search(line)
            if m:
                info = dict(current.get(m.group(1), {}))
                out.append((line[1:20], m.group(1), m.group(2), info))
    return out


if LOG:
    log_path = next((a for a in args if not a.startswith('--')), os.path.join(ROOT, 'Saved/Logs/AI_Looter_Shooter.log'))
    paths = []
    seen = set()
    for when, name, what, info in read_log(log_path):
        tris, fb, build = info.get('tris'), info.get('fallback'), info.get('build')
        if fb is not None and tris is not None and fb < tris:
            source = f'reduced Nanite fallback ({fb} of {tris} triangles)' + (
                ": the import's first build, with Unreal's default fallback" if build == 1 else '')
        elif fb is not None:
            source = f'full Nanite fallback ({fb} triangles): the mesh itself'
        elif build == 1:
            source = 'LOD0 only (the import build): the mesh itself'
        else:
            source = 'LOD0 or its reduced LODs'
        key = (when[:16], name, what)
        if key not in seen:
            seen.add(key)
            print(f'TANGENTS log {when} {name}: {what}; from {source}', flush=True)

total_bad = 0
for path in paths:
    try:
        meshes = read_fbx(path)
    except Exception as error:      # a file the parser can't read
        print(f'TANGENTS {os.path.basename(path)}: could not read: {error}', flush=True)
        continue
    note = reduced_note(manifest_entries(os.path.dirname(path)).get(os.path.basename(path)))
    file_bad = 0
    for name, verts, polygons, normals, uvs, world in meshes:
        P, N, T, B, cut = unreal_mesh(verts, polygons, normals, uvs, world)
        # Unreal's importer drops triangles with two corners within THRESH_POINTS_ARE_SAME (bRemoveDegenerates).
        keep = [t for t in range(len(P)) if not any(np.all(np.abs(P[t, a] - P[t, b]) <= 0.00002)
                                                    for a, b in ((0, 1), (0, 2), (1, 2)))]
        P, N, T, B = P[keep], N[keep], T[keep], B[keep] / 100.0
        file_bad += check(f'{os.path.basename(path)} [{name}]' + (f' ({note})' if note else ''), P, N, T, B, cut)
    if file_bad and WHERE:
        for name, text in blender_check(path).items():
            print(f'TANGENTS   Blender MikkTSpace on {name}: {text}', flush=True)
    total_bad += file_bad
if paths:
    print(f'TANGENTS checked {len(paths)} files; {total_bad} corners Unreal would warn about', flush=True)
sys.exit(1 if total_bad else 0)
