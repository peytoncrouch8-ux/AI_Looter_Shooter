"""SK_Gravemother's pieces from the user's Tripo model (parts_gravemother_shape.py reads it), on Spider.py's rig: the
legs, the cephalothorax, the abdomen, the palps, the chelicerae, two skulls and the headstone, each cut off cleanly
(plane cuts, the cut capped: a dome, or for a leg's root a stub pushed into the body), carried into the rig's rest pose
and remeshed into even quads (QuadriFlow, Blender's built-in). Not a model: Tools\\models.ps1 skips parts_*.py.

- The rig needs eight legs; Tripo made six: the middle pair is used for the rig's pairs 1 and 2 (they share their
  lengths in Spider.py: femur 90, tibia 116), the front pair for 0, the back pair for 3.
- Each leg is cut at its body end and at its knee (the station most raised over its hip-to-tip chord), and each half is
  carried by its own bone's map (looter_tripo_rig.bone_map: the half's ends onto the bone's, the leg's bending plane onto
  the rig's) into the rig's rest pose, solved with the game's IK: so whatever stance Tripo gave a leg (its front feet
  down, its back legs off the ground: the tilt), it stands as the rig does. The femur's root starts where the bone's line
  leaves the cephalothorax, and its stub runs on into the body.
- The cephalothorax, palps and chelicerae are carried by one map (BODY_*: the hips' heights onto the rig's, its width
  onto the rig's carapace), level already (its six hips lie within 0.8 units of one height).
- The abdomen by its own (AB_*): Tripo's ball, a little smaller and flatter, so the game's headstone (its place is
  fixed in code: GravemotherCreatureStone.cpp, stone_attach.json) stands sunk about a quarter of its height in her back,
  as the scripted one did, and her belly keeps 26 cm off the ground. Where Tripo's own stone stood the hide is closed.
- The headstone into the stone's own frame (looter_barrow: x across, +y out of the carved face, z up from the foot, cm),
  80 cm wide and 116 tall as the scripted slab, its foot run down to z = 0.
- Cached (Intermediate/GravemotherModel/TripoQuads): QuadriFlow takes a few minutes for the lot.
Space: creature cm (x forward, y left, z up; the origin on the ground under the thorax), the spider's 1x."""
import hashlib
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_textures as lt
import looter_tripo_rig as tr
import parts_gravemother_shape as gs

CACHE = os.path.join(lt.REPO, 'Intermediate', 'GravemotherModel', 'TripoQuads')
# The cephalothorax's map (Tripo units to creature cm): its hips' middle (x, z) onto the rig's (0.5, 58).
BODY_SCALE = 4.2
BODY_FROM = np.array([4.67, 0.0, 16.0])
BODY_TO = np.array([0.5, 0.0, 58.0])
# The abdomen's: Tripo's ball (its fitted sphere) onto this ellipsoid (creature cm).
AB_CENTRE = np.array([-72.0, 0.0, 72.0])
AB_RADII = np.array([54.0, 52.0, 46.0])
# The headstone (stone cm): its width and top as the scripted slab's (its thickness too: looter_barrow.STONE_T).
STONE_WIDTH, STONE_TOP = 80.0, 116.0
# Which Tripo leg each rig pair is made from (gs.LEGS: 2 * pair + side).
LEG_FROM = {0: 0, 1: 1, 2: 1, 3: 2}
# Even quads per piece (QuadriFlow's target) and the voxel size (cm) that makes each watertight first.
QUADS = dict(thorax=2400, abdomen=2500, femur=380, tibia=520, palp=220, claw=150, chel=260, skull=170, stone=3200)
VOXEL = dict(thorax=0.9, abdomen=1.4, femur=0.55, tibia=0.5, palp=0.3, claw=0.25, chel=0.3, skull=0.35, stone=0.7)


def log(message):
    print(f'GRAVEMOTHER TRIPO: {message}', flush=True)


def to_body(P):
    return (np.asarray(P, float) - BODY_FROM) * BODY_SCALE + BODY_TO


# --- Cutting the source into pieces ---

class Cutter:
    """The source mesh as a bmesh whose faces each get an owner (a piece's name; '' none yet, '-' dropped)."""

    def __init__(self, src, labels):
        self.src, self.labels = src, labels
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')     # Tripo's UVs ride along (a bisect splits them too)
        verts = [self.bm.verts.new(p) for p in src.V]
        for f, uvs in zip(src.F, src.UV):
            try:
                face = self.bm.faces.new([verts[i] for i in f])
            except ValueError:
                continue
            for loop, uv in zip(face.loops, uvs):
                loop[self.uv].uv = uv
        self.bm.verts.ensure_lookup_table()
        self.n_orig = len(src.V)
        self.owner = {}

    def label_of(self, face):
        """A face's label: its first original vertex's (a bisect's new vertices have none)."""
        for v in face.verts:
            if v.index < self.n_orig:
                return int(self.labels[v.index])
        return -1

    def bisect(self, co, no, near, reach):
        """Splits the faces within reach of near by the plane (co, no)."""
        near = Vector(near)
        faces = [f for f in self.bm.faces if f not in self.owner and (f.calc_center_median() - near).length < reach]
        geom = list({e for f in faces for e in f.edges}) + faces + list({v for f in faces for v in f.verts})
        bmesh.ops.bisect_plane(self.bm, geom=geom, plane_co=Vector(co), plane_no=Vector(no), dist=1e-5)
        self.bm.verts.index_update()
        self.bm.faces.ensure_lookup_table()
        self.bm.verts.ensure_lookup_table()

    def flood(self, name, seed_vertex, allowed):
        """Faces reached from seed_vertex's faces through faces allowed(face) passes, owned by name."""
        seed = self.bm.verts[seed_vertex]
        stack = [f for f in seed.link_faces if f not in self.owner and allowed(f)]
        got = set(stack)
        while stack:
            f = stack.pop()
            for e in f.edges:
                for g in e.link_faces:
                    if g not in got and g not in self.owner and allowed(g):
                        got.add(g)
                        stack.append(g)
        for f in got:
            self.owner[f] = name
        return len(got)

    def take(self, name, test):
        n = 0
        for f in self.bm.faces:
            if f not in self.owner and test(f):
                self.owner[f] = name
                n += 1
        return n

    def piece(self, name, uvs=False):
        """A piece's faces as (V, faces) arrays (and with uvs, Tripo's UVs per face corner)."""
        faces = [f for f, o in self.owner.items() if o == name and f.is_valid]
        index = {}
        V, F, UV = [], [], []
        for f in faces:
            ids = []
            for v in f.verts:
                if v not in index:
                    index[v] = len(V)
                    V.append(v.co[:])
                ids.append(index[v])
            F.append(np.array(ids))
            UV.append([tuple(loop[self.uv].uv) for loop in f.loops])
        return (np.array(V), F, UV) if uvs else (np.array(V), F)


def branch_cut(cut, label, line, name):
    """Cuts a branch (a leg, palp or chelicera) off at its line's body end: the plane square to its last stretch."""
    end = line[-1, 1:4]
    back = line[max(len(line) - 6, 0), 1:4]
    no = end - back
    no /= np.linalg.norm(no)
    reach = max(line[:, 4].max() * 3.2, 3.0)
    cut.bisect(end, no, end, reach)
    tip = int(np.argmin(np.linalg.norm(cut.src.V - line[0, 1:4], axis=1)))
    endv = Vector(end)
    nov = Vector(no)

    def allowed(f):
        c = f.calc_center_median()
        if (c - endv).length < reach:
            return (c - endv).dot(nov) < 0.0
        return cut.label_of(f) == label
    count = cut.flood(name, tip, allowed)
    return count, end, no


def extract(src, labels, info):
    """The pieces in Tripo's space: {name: (V, faces, kind, extra)}, every cut closed."""
    cut = Cutter(src, labels)
    out = {}
    branches = [(leg, f'leg{leg}') for leg in gs.LEGS] + [(gs.PALP_L, 'palp_l'), (gs.PALP_R, 'palp_r'),
                                                         (gs.CHEL_L, 'chel_l'), (gs.CHEL_R, 'chel_r'),
                                                         (gs.CLAW_L, 'claw_l'), (gs.CLAW_R, 'claw_r')]
    roots = {}
    for label, name in branches:
        line = info['lines'][label]
        count, end, no = branch_cut(cut, label, line, name)
        roots[name] = (end, no)
    # The headstone: above a plane just over its foot, inside its slab's box.
    st = info['stone']
    c, (wx, th, up) = st['centre'], st['axes']
    h_cut = st['foot'] + 1.0
    co = c + up * h_cut
    cut.bisect(co, up, co, st['half_w'] * 2.5)
    cv, wxv, thv, upv = Vector(c), Vector(wx), Vector(th), Vector(up)

    def in_box(f, h_min, grow=0.8):
        p = f.calc_center_median() - cv
        return (abs(p.dot(wxv)) < st['half_w'] + grow and abs(p.dot(thv)) < st['half_t'] + grow and p.dot(upv) > h_min)
    top = int(np.argmax(src.V[:, 2]))
    thin = lambda f: any(v.index < cut.n_orig and src.T[v.index] < 1.0 for v in f.verts)
    cut.flood('stone', top, lambda f: in_box(f, h_cut) and not thin(f))
    # Her hide under it, the rest of its foot, the railings and the root fibres they carry: dropped (closed later).
    cut.take('-', lambda f: in_box(f, st['foot'] - 1.5, 1.2))
    cut.take('-', lambda f: cut.label_of(f) == gs.RAILING)
    for k, (centre, r) in enumerate(gs.SKULLS):
        cv_k = Vector(centre)
        cut.take(f'skull{k}', lambda f: cut.label_of(f) == gs.SKULL and (f.calc_center_median() - cv_k).length < r)
    cut.take('-', lambda f: cut.label_of(f) == gs.SKULL)
    # The pedicel: the cephalothorax in front of its plane, the abdomen behind.
    x = info['pedicel']
    cut.bisect((x, 0.0, 0.0), (1.0, 0.0, 0.0), (x, 0.0, 18.0), 14.0)
    cut.take('thorax', lambda f: f.calc_center_median().x > x)
    # Root fibres standing off her hide (thin, outside her ball): dropped; the hide under them is closed later.
    ab = labels == gs.ABDOMEN
    c_ab = src.V[ab].mean(0)
    r_ab = np.median(np.linalg.norm(src.V[ab] - c_ab, axis=1))
    cv_ab = Vector(c_ab)
    cut.take('-', lambda f: all(v.index >= cut.n_orig or src.T[v.index] < 0.8 for v in f.verts)
             and (f.calc_center_median() - cv_ab).length > r_ab * 1.04)
    cut.take('abdomen', lambda f: True)
    for name in [n for _, n in branches] + ['stone', 'skull0', 'skull1', 'thorax', 'abdomen']:
        V, F = cut.piece(name)
        if not len(F):
            raise RuntimeError(f'extract: no faces for {name}')
        out[name] = (V, F)
    # The stone's own Tripo surface and UVs (its texture's carving is baked from them).
    out['stone_tripo'] = cut.piece('stone', uvs=True)
    cut.bm.free()
    return out, roots


# --- Closing cuts ---

def loop_frame(V, loop):
    P = V[loop]
    c = P.mean(0)
    n = np.zeros(3)
    for a, b in zip(P, np.roll(P, -1, axis=0)):
        n += np.cross(a - c, b - c)
    return c, n / max(np.linalg.norm(n), 1e-12), np.sqrt(((P - c) ** 2).sum(1)).mean()


def close(V, F, piece_centre, stub=None, dome=0.35):
    """Closes every open loop of a piece: a dome (its height dome times the loop's radius, out of the piece), or for
    the loop nearest stub's point a stub first: the loop pushed along stub's way (its length) and drawn in 15%, then
    the dome. Face windings are made consistent after."""
    V = np.asarray(V, float)
    F = list(F)
    loops = tr.boundary_loops(F)
    stub_loop = None
    if stub is not None and loops:
        stub_loop = min(range(len(loops)), key=lambda k: np.linalg.norm(V[loops[k]].mean(0) - stub[0]))
    for k, loop in enumerate(loops):
        c, n, r = loop_frame(V, loop)
        out = c - piece_centre
        if n @ out < 0:
            n = -n
        if k == stub_loop:
            way = np.asarray(stub[1], float)
            ring = c + (V[loop] - c) * 0.85 + way * stub[2]
            base = len(V)
            V = np.vstack([V, ring])
            m = len(loop)
            for i in range(m):
                F.append(np.array([loop[i], loop[(i + 1) % m], base + (i + 1) % m, base + i]))
            loop = list(range(base, base + m))
            c, n, r = c + way * stub[2], way / np.linalg.norm(way), r * 0.85
        new_V, new_F = tr.cap(V, list(loop), n, r * dome, rings=4)
        F += [np.array(f) for f in new_F]
        V = np.vstack([V, new_V])
    return consistent(V, F)


def consistent(V, F):
    obj = tr.new_object('_closed', V, F)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()
    V, F = tr.mesh_arrays(obj.data)
    mesh = obj.data
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    return V, F


def split_plane(V, F, co, no):
    """A closed piece cut by a plane into its two sides (the side no points away from first), each closed with a dome."""
    obj = tr.new_object('_split', V, F)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=Vector(co), plane_no=Vector(no),
                           dist=1e-6)
    sides = ([], [])
    for f in bm.faces:
        s = (f.calc_center_median() - Vector(co)).dot(Vector(no))
        sides[1 if s > 0 else 0].append([v.index for v in f.verts])
    Vb = np.array([v.co[:] for v in bm.verts])
    bm.free()
    mesh = obj.data
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    out = []
    for faces in sides:
        used = sorted({i for f in faces for i in f})
        remap = {old: new for new, old in enumerate(used)}
        out.append((Vb[used], [np.array([remap[i] for i in f]) for f in faces]))
    return out


# --- The legs' joints ---

def knee_of(line):
    """A leg line's (tip first) hip, knee and tip: the knee is the station most raised over the hip-to-tip chord."""
    P = line[:, 1:4]
    hip, tip = P[-1], P[0]
    chord = tip - hip
    t = np.clip(((P - hip) @ chord) / (chord @ chord), 0.0, 1.0)
    rise = P[:, 2] - (hip[2] + chord[2] * t)
    inner = (t > 0.12) & (t < 0.8)
    k = int(np.argmax(np.where(inner, rise, -np.inf)))
    return hip, P[k], tip, k
