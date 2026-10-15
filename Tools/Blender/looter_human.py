"""MakeHuman's CC0 base mesh (Art/Portraits/mh.py) made into a game character: shaped and sized, with the UE5 mannequin's
bones placed on MakeHuman's own joints and MakeHuman's skin weights carried over onto them, so the game's mannequin
animations can drive it through Unreal's IK retargeter (its rig builds itself for mannequin names).

    body = Body(macro, face, height=1.77, lift=0.025)     # shaped, sized, standing on the origin facing -Y
    body.lids(...)                                         # optional: narrowed eyes (EllisBlender.close_lids)
    ob = body.mesh('Skin', keep=lambda w: ...)             # the visible skin, UVs from MakeHuman
    arm = body.armature('Ellis')                           # the mannequin's bones on MakeHuman's joints
    W = body.weights_at(points)                            # mannequin bone weights anywhere near the body
    skin(ob, W, arm)                                       # vertex groups + Armature modifier

Units: metres, Z up, the front -Y (Art/README.md). MakeHuman works in decimetres, +Y up, +Z forward; to_world() converts
and applies the size. The rest pose is MakeHuman's: arms about 49 degrees below level with the elbows a little bent,
legs a little apart; that is the bind pose (the retargeter's retarget pose matches the mannequin's to it).
"""
import json
import math
import os
import sys

import bpy  # first: as a Python module (the cloud box's bpy) bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(REPO, 'Art', 'Portraits'))
import mh  # noqa: E402

FINGERS = (('thumb', 1), ('index', 2), ('middle', 3), ('ring', 4), ('pinky', 5))
SIDES = (('l', 'L'), ('r', 'R'))
SPINE = ('pelvis', 'spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05')
MH_SPINE = ('root', 'spine05', 'spine04', 'spine03', 'spine02', 'spine01', 'breast.L', 'breast.R', 'pelvis.L', 'pelvis.R')


def _mh_to_ue():
    """Mannequin bone -> the MakeHuman bones whose weights it takes (the spine is shared out by height instead)."""
    m = {'neck_01': ['neck01'], 'neck_02': ['neck02', 'neck03']}
    for s, S in SIDES:
        m[f'clavicle_{s}'] = [f'clavicle.{S}', f'shoulder01.{S}']
        m[f'upperarm_{s}'] = [f'upperarm01.{S}', f'upperarm02.{S}']
        m[f'lowerarm_{s}'] = [f'lowerarm01.{S}', f'lowerarm02.{S}']
        m[f'hand_{s}'] = [f'wrist.{S}']
        for finger, k in FINGERS:
            if finger != 'thumb':
                m[f'{finger}_metacarpal_{s}'] = [f'metacarpal{k - 1}.{S}']
            for i in (1, 2, 3):
                m[f'{finger}_0{i}_{s}'] = [f'finger{k}-{i}.{S}']
        m[f'thigh_{s}'] = [f'upperleg01.{S}', f'upperleg02.{S}']
        m[f'calf_{s}'] = [f'lowerleg01.{S}', f'lowerleg02.{S}']
        m[f'foot_{s}'] = [f'foot.{S}']
        m[f'ball_{s}'] = [f'toe{t}-{i}.{S}' for t in range(1, 6) for i in (1, 2, 3)]
    return m


MH_TO_UE = _mh_to_ue()


class Body:
    """A shaped MakeHuman body at game size. Points are kept in MakeHuman space (self.h) and converted on the way out."""

    def __init__(self, macro, face, height=1.77, lift=0.0, pose=None):
        self.h = mh.build(macro=macro, face=face, pose=pose)
        self.body_faces = self.h['group_faces']('body')
        self.used = np.unique(np.concatenate(self.body_faces))
        top = self.h['verts'][self.used][:, 1].max()
        self.floor = self.h['verts'][self.used][:, 1].min()
        self.scale = height / ((top - self.floor) * 0.1)
        self.lift = lift
        self._weights = json.load(open(os.path.join(mh.DATA, 'rigs', 'default_weights.mhw')))['weights']

    # --- space
    def to_world(self, p):
        """MakeHuman points (n, 3) -> world metres, standing on the origin (raised by lift)."""
        p = np.asarray(p, dtype=np.float64)
        q = mh.to_blender(p - np.array([0.0, self.floor, 0.0])) * self.scale
        q[..., 2] += self.lift
        return q

    def joint(self, bone, end='head'):
        return self.to_world(self.h['bones'][bone][end][None])[0]

    def world_verts(self):
        return self.to_world(self.h['verts'])

    def mh_weight(self, bone):
        w = np.zeros(len(self.h['verts']))
        for i, x in self._weights.get(bone, []):
            w[i] = x
        return w

    # --- the mannequin's skeleton
    def skeleton(self):
        """name -> (head, tail, parent) in world metres: the mannequin's 63 deform bones on MakeHuman's joints."""
        J = self.joint
        b = {}
        spine = [J('spine05'), J('spine04'), J('spine03'), J('spine02'), J('spine01')]
        neck = J('neck01')
        spine.append((spine[-1] + neck) * 0.5)
        spine.append(neck)
        # pelvis from the hips' middle; spine_01..05 up MakeHuman's spine (the chest's longest bone halved)
        hips = (J('upperleg01.L') + J('upperleg01.R')) * 0.5
        b['pelvis'] = (np.array([0.0, spine[0][1], hips[2] + 0.02]), spine[1], None)
        b['spine_01'] = (spine[1], spine[2], 'pelvis')
        b['spine_02'] = (spine[2], spine[3], 'spine_01')
        b['spine_03'] = (spine[3], spine[4], 'spine_02')
        b['spine_04'] = (spine[4], spine[5], 'spine_03')
        b['spine_05'] = (spine[5], spine[6], 'spine_04')
        b['neck_01'] = (J('neck01'), J('neck02'), 'spine_05')
        b['neck_02'] = (J('neck02'), J('head'), 'neck_01')
        b['head'] = (J('head'), J('head', 'tail'), 'neck_02')
        for s, S in SIDES:
            b[f'clavicle_{s}'] = (J(f'clavicle.{S}'), J(f'upperarm01.{S}'), 'spine_05')
            b[f'upperarm_{s}'] = (J(f'upperarm01.{S}'), J(f'lowerarm01.{S}'), f'clavicle_{s}')
            b[f'lowerarm_{s}'] = (J(f'lowerarm01.{S}'), J(f'wrist.{S}'), f'upperarm_{s}')
            b[f'hand_{s}'] = (J(f'wrist.{S}'), J(f'metacarpal2.{S}', 'tail'), f'lowerarm_{s}')
            for finger, k in FINGERS:
                parent = f'hand_{s}'
                if finger != 'thumb':
                    mc = f'metacarpal{k - 1}.{S}'
                    b[f'{finger}_metacarpal_{s}'] = (J(mc), J(mc, 'tail'), parent)
                    parent = f'{finger}_metacarpal_{s}'
                for i in (1, 2, 3):
                    bone = f'finger{k}-{i}.{S}'
                    b[f'{finger}_0{i}_{s}'] = (J(bone), J(bone, 'tail'), parent)
                    parent = f'{finger}_0{i}_{s}'
            b[f'thigh_{s}'] = (J(f'upperleg01.{S}'), J(f'lowerleg01.{S}'), 'pelvis')
            b[f'calf_{s}'] = (J(f'lowerleg01.{S}'), J(f'foot.{S}'), f'thigh_{s}')
            ball = J(f'toe3-1.{S}')
            b[f'foot_{s}'] = (J(f'foot.{S}'), ball, f'calf_{s}')
            tip = J(f'toe3-3.{S}', 'tail')
            b[f'ball_{s}'] = (ball, np.array([ball[0], tip[1], ball[2]]), f'foot_{s}')
        return b

    def armature(self, name):
        """The mannequin's bones as a Blender armature at the origin (it becomes Unreal's root bone)."""
        sk = self.skeleton()
        arm = bpy.data.objects.new(name, bpy.data.armatures.new(name))
        bpy.context.scene.collection.objects.link(arm)
        bpy.ops.object.select_all(action='DESELECT')
        bpy.context.view_layer.objects.active = arm
        arm.select_set(True)
        bpy.ops.object.mode_set(mode='EDIT')
        edit = arm.data.edit_bones
        for bone, (h, t, parent) in sk.items():
            e = edit.new(bone)
            e.head, e.tail = Vector(h), Vector(t)
            d = (t - h) / max(np.linalg.norm(t - h), 1e-9)
            e.align_roll(Vector((0.0, 0.0, 1.0)) if abs(d[1]) > 0.7 else Vector((0.0, -1.0, 0.0)))
            e.use_connect = False
            e.use_deform = True
        for bone, (h, t, parent) in sk.items():
            if parent:
                edit[bone].parent = edit[parent]
        bpy.ops.object.mode_set(mode='OBJECT')
        return arm

    # --- weights
    def vertex_weights(self):
        """(names, W): the mannequin bones' weights on every MakeHuman vertex, top four, normalised. MakeHuman's spine
        weights (and the hips') are pooled and shared out over pelvis..spine_05 by height, so the back bends evenly."""
        if getattr(self, '_vw', None) is None:
            self._vw = self._vertex_weights()
        return self._vw

    def _vertex_weights(self):
        n = len(self.h['verts'])
        names = list(self.skeleton())
        col = {name: i for i, name in enumerate(names)}
        W = np.zeros((n, len(names)))
        claimed = set()
        for ue, sources in MH_TO_UE.items():
            for src in sources:
                W[:, col[ue]] += self.mh_weight(src)
                claimed.add(src)
        spine = np.zeros(n)
        for src in MH_SPINE:
            spine += self.mh_weight(src)
            claimed.add(src)
        # whatever is left (the face's, the eyes', the jaw's and the tongue's bones) belongs to the head
        for src in self._weights:
            if src not in claimed:
                W[:, col['head']] += self.mh_weight(src)
        sk = self.skeleton()
        z = self.world_verts()[:, 2]
        centres = np.array([(sk[b][0][2] + sk[b][1][2]) * 0.5 for b in SPINE])
        centres[0] = sk['pelvis'][0][2]
        for i, b in enumerate(SPINE):
            lo = centres[i - 1] if i > 0 else -1e9
            hi = centres[i + 1] if i + 1 < len(SPINE) else 1e9
            c = centres[i]
            share = np.where(z < c, np.clip((z - lo) / max(c - lo, 1e-6), 0, 1) if i > 0 else 1.0,
                             np.clip((hi - z) / max(hi - c, 1e-6), 0, 1) if i + 1 < len(SPINE) else 1.0)
            W[:, col[b]] += spine * share
        return names, top4(W)

    def weights_at(self, points, faces=None, max_dist=None):
        """Mannequin bone weights at world points: each takes the weights of the nearest point on the body (MakeHuman's
        whole body surface, or only faces if given), blended across that face. Returns (names, W)."""
        names, Wv = self.vertex_weights()
        wv = self.world_verts()
        faces = faces if faces is not None else self.body_faces
        tris = [(f[0], f[k], f[k + 1]) for f in faces for k in range(1, len(f) - 1)]
        tree = BVHTree.FromPolygons([tuple(map(float, p)) for p in wv], tris, all_triangles=True)
        tri_src = tris
        out = np.zeros((len(points), Wv.shape[1]))
        for k, p in enumerate(points):
            loc, _nrm, fi, _d = tree.find_nearest(Vector(p))
            if loc is None:
                continue
            a, b_, c = (wv[i] for i in tri_src[fi])
            bc = barycentric(np.array(loc), a, b_, c)
            out[k] = sum(bc[j] * Wv[tri_src[fi][j]] for j in range(3))
        return names, top4(out)

    # --- the skin
    def mesh(self, name, keep=None, uv=True):
        """The body's surface as a mesh object in world space, with MakeHuman's UVs (map 'UVMap'). keep(face_index,
        verts) -> bool picks the faces kept (default all). The object remembers each vertex's MakeHuman index
        (attribute 'mh_index')."""
        faces = self.body_faces
        fuv = [fu for fu, g in zip(self.h['fuv'], self.h['groups']) if g and g.startswith('body')]
        idx = [i for i, f in enumerate(faces) if keep is None or keep(i, f)]
        used = np.unique(np.concatenate([faces[i] for i in idx]))
        remap = -np.ones(len(self.h['verts']), dtype=np.int64)
        remap[used] = np.arange(len(used))
        wv = self.world_verts()[used]
        me = bpy.data.meshes.new(name)
        me.from_pydata([tuple(p) for p in wv], [], [[int(remap[i]) for i in faces[k]] for k in idx])
        if uv:
            layer = me.uv_layers.new(name='UVMap')
            data = []
            for k in idx:
                data.extend(self.h['uv'][fuv[k]].ravel())
            layer.data.foreach_set('uv', np.array(data, dtype=np.float32))
        attr = me.attributes.new('mh_index', 'INT', 'POINT')
        attr.data.foreach_set('value', used.astype(np.int32))
        me.update()
        me.shade_smooth()
        ob = bpy.data.objects.new(name, me)
        bpy.context.scene.collection.objects.link(ob)
        return ob


def barycentric(p, a, b, c):
    v0, v1, v2 = b - a, c - a, p - a
    d00, d01, d11 = v0 @ v0, v0 @ v1, v1 @ v1
    d20, d21 = v2 @ v0, v2 @ v1
    den = d00 * d11 - d01 * d01
    if abs(den) < 1e-14:
        return np.array([1.0, 0.0, 0.0])
    v = (d11 * d20 - d01 * d21) / den
    w = (d00 * d21 - d01 * d20) / den
    bc = np.clip(np.array([1.0 - v - w, v, w]), 0.0, 1.0)
    return bc / max(bc.sum(), 1e-9)


def top4(W):
    """Keeps each row's four largest weights and normalises them (Unreal's default is four influences a vertex)."""
    W = np.asarray(W, dtype=np.float64).copy()
    if W.shape[1] > 4:
        cut = np.sort(W, axis=1)[:, -4][:, None]
        W[W < cut] = 0.0
    s = W.sum(axis=1, keepdims=True)
    W = np.where(s > 1e-9, W / np.maximum(s, 1e-9), 0.0)
    return W


def skin(ob, names, W, arm):
    """Vertex groups from (names, W) on ob's vertices (in order), parented to arm with an Armature modifier."""
    for j, bone in enumerate(names):
        col = W[:, j]
        nz = np.nonzero(col > 1e-4)[0]
        if not len(nz):
            continue
        g = ob.vertex_groups.get(bone) or ob.vertex_groups.new(name=bone)
        for i in nz:
            g.add([int(i)], float(col[i]), 'REPLACE')
    ob.parent = arm
    mod = ob.modifiers.new('Armature', 'ARMATURE')
    mod.object = arm
    return ob


def rigid(names, n, bone):
    """(names, W) with every one of n vertices on one bone."""
    W = np.zeros((n, len(names)))
    W[:, names.index(bone)] = 1.0
    return W


def blend(names, n, parts):
    """(n, bones) weights from {bone: per-vertex array or scalar}, normalised."""
    W = np.zeros((n, len(names)))
    for bone, w in parts.items():
        W[:, names.index(bone)] += np.broadcast_to(np.asarray(w, dtype=np.float64), (n,))
    return top4(W)
