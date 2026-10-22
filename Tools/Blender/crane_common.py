"""What every part of Hollis Crane's game model shares (Art/Models/Characters/Crane.py builds it; the crane_*.py
modules here build its parts): the run's arguments and log, the figure's measures, the shaped body (Figure), the looks
registry (slot) and small geometry helpers copied from Ellis.py (one class per file is the C++ rule; these are
scripts, split by topic to stay under ~1500 lines).

The spec: the portrait the user picked on 2026-10-19, option A "The Marshal" of Art/Portraits/Crane.py (its OPTIONS["A"],
FACE, MACRO and builders), the bust Saved/ArtPreviews/Portraits/Crane/CraneA.png, and Saved/Heroes/CONTRACT.md:
1.95 m, gaunt, the head and shoulders carried forward in the mesh (a stoop as a shape only; the posture is the
mannequin's animations'), grey-green skin. Units metres, Z up, built facing -Y (turned last, face_like_mannequin).
"""
import importlib.util
import math
import os
import sys
import time

import bpy  # first: as a Python module (the cloud box's bpy) bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
for _p in (HERE, os.path.join(REPO, 'Art', 'Portraits')):
    if _p not in sys.path:
        sys.path.insert(0, _p)
import bust  # noqa: E402
import looter_garments as gm  # noqa: E402
import looter_face as lf  # noqa: E402
import looter_human as hu  # noqa: E402
import looter_looks as lk  # noqa: E402
import looter_model as lm  # noqa: E402
import mh  # noqa: E402


def _portrait():
    """Art/Portraits/Crane.py as the module 'CranePortrait' (by path: this model's own script is Crane.py too)."""
    if 'CranePortrait' in sys.modules:
        return sys.modules['CranePortrait']
    spec = importlib.util.spec_from_file_location('CranePortrait', os.path.join(REPO, 'Art', 'Portraits', 'Crane.py'))
    mod = importlib.util.module_from_spec(spec)
    sys.modules['CranePortrait'] = mod
    spec.loader.exec_module(mod)
    return mod


CP = _portrait()
OPT = CP.OPTIONS['A']
PORTRAIT_FILE = os.path.join(REPO, 'Art', 'Portraits', 'Crane.py')

ARGS = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
T0 = time.time()


def log(message):
    print(f'CRANE [{time.time() - T0:6.1f}s] {message}', flush=True)


def arg_value(name, default=None):
    return ARGS[ARGS.index(name) + 1] if name in ARGS and ARGS.index(name) + 1 < len(ARGS) else default


# ------------------------------------------------------------------------------------------------------- measures

HEIGHT = 1.92            # barefoot, the top of the stooped head (m): 1.95 in his boots
LIFT = 0.03              # the worn boots' heels and soles raise the body this much
HEM = 0.30               # the coat's hem (m over the ground) before its tatters (they reach ~6 cm lower): low on the calf
BOOT_TOP = 0.50          # the boots' tops (mid-calf on a 1.95 m man)
FWD = np.array([0.0, -1.0])
SIM = '--no-sim' not in ARGS
DRAPE_FILE = os.path.join(REPO, 'Art', 'Models', 'Characters', 'Crane.drape.json')
OUT_DIR = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Characters', 'Crane')
CONCEPTS = os.path.join(REPO, 'Saved', 'Concepts')

# The stoop as a shape (the contract: "head and shoulders carried forward in the mesh; the posture itself comes from the
# mannequin's animations"): MakeHuman's upper spine and neck bent a little forward, the head turned back up so he looks
# ahead under the brim, the collarbones rolled forward. (MakeHuman bones, axis in its space: +x his left, +y up, +z
# forward; + about x bends forward.) The portrait's bust stooped 24 degrees and looked up 40: a pose, not a shape.
STOOP = {
    'spine02': ((1.0, 0.0, 0.0), 2.0), 'spine01': ((1.0, 0.0, 0.0), 5.0),
    'neck01': ((1.0, 0.0, 0.0), 11.0), 'neck02': ((1.0, 0.0, 0.0), 5.0), 'head': ((1.0, 0.0, 0.0), -16.0),
    'clavicle.L': ((0.0, 1.0, 0.0), -6.0), 'clavicle.R': ((0.0, 1.0, 0.0), 6.0),
}
BOOT_LOOK = (-0.21, 0.0255)   # the boots' toe cap and welt (looter_looks.leather's boot=), measured by crane_clothes.boot_look
HAND_GIRTH = 1.07        # bare hands filled out a little (the user found Ellis's thin hands "clawed"; Crane's stay bony)
# The face's planes (looter_face.carve; the portrait's: a heavy brow over the sockets, high cheekbones over hollow
# cheeks, a long sharp nose, deep folds from the nose to the mouth's corners, a jutting chin and a hard jaw):
# (kind, from, to, reach dm (x, y, z), depth m). Points: ('side'|'mid', landmark, (dx, dy)) from looter_face.landmarks.
FACE_PLANES = [
    ('ridge', ('side', 'eye', (-0.13, 0.195)), ('side', 'eye', (0.16, 0.17)), (0.15, 0.062, 0.3), 0.0035),  # brow ridge
    ('ridge', ('side', 'eye', (-0.25, 0.12)), ('side', 'eye', (-0.26, 0.23)), (0.028, 0.06, 0.2), -0.0012), # frown lines
    ('ridge', ('side', 'eye', (0.0, -0.27)), ('side', 'eye', (0.24, -0.20)), (0.14, 0.075, 0.3), 0.0035),   # cheekbone
    ('blob', ('side', 'eye', (0.10, -0.55)), None, (0.17, 0.16, 0.35), -0.0030),                            # its hollow
    ('ridge', ('side', 'nose', (0.13, -0.12)), ('side', 'mouth', (0.30, -0.12)), (0.04, 0.04, 0.3), -0.0022),    # fold
    ('ridge', ('side', 'nose', (0.19, -0.12)), ('side', 'mouth', (0.36, -0.10)), (0.05, 0.05, 0.3), 0.0012),     # pad
    ('ridge', ('mid', 'nose', (0.0, 0.42)), ('mid', 'nose', (0.0, 0.08)), (0.05, 0.2, 0.3), 0.0015),       # nose bridge
    ('ridge', ('side', 'nose', (0.10, 0.35)), ('side', 'nose', (0.11, 0.05)), (0.04, 0.15, 0.3), -0.0010), # its sides
    ('ridge', ('side', 'mouth', (0.0, -0.17)), ('side', 'mouth', (0.15, -0.14)), (0.12, 0.04, 0.25), -0.0015),   # lip groove
    ('blob', ('mid', 'chin', (0.0, 0.05)), None, (0.15, 0.12, 0.3), 0.0025),                                # chin
    ('ridge', ('side', 'chin', (0.15, 0.0)), ('side', 'jaw', (-0.04, -0.05)), (0.06, 0.06, 0.15), 0.0015),  # jaw line
]


# ------------------------------------------------------------------------------------------------------- the body

class Body(hu.Body):
    """looter_human's Body, posed: its joints carried by MakeHuman's posed bones (hu.Body places the mannequin's bones
    on the rest joints, which a posed mesh leaves behind)."""

    def joint(self, bone, end='head'):
        p = self.h['bones'][bone][end][None]
        g = self.h['glob'].get(bone)
        if g is not None:
            p = mh.apply(g, p)
        return self.to_world(p)[0]


def sunken(h, hf, k):
    """The portrait's deepen() on the MakeHuman mesh: skin tight on the bone (the cheeks under the cheekbones and the
    temples pushed in), and the colour zones (bust.zones: sunken eyes, a cold flush, bloodless lips; darker round the
    sockets and in the hollows). Moves h['verts'] (decimetres); returns the zones' colours per MakeHuman vertex."""
    faces = h['group_faces']('body')
    used = np.unique(np.concatenate(faces))
    g = h['glob']['head']
    eyes_posed = [mh.apply(g, e[None])[0] for e in hf['eyes']]
    col = np.ones((len(h['verts']), 4))
    col[used] = bust.zones(h, h['verts'][used], eyes_posed, used)
    loc = mh.apply(np.linalg.inv(g), h['verts'][used])
    c = col[used]
    push = np.zeros(len(used))
    for e in hf['eyes']:
        sx = math.copysign(1.0, e[0])
        for at, rad, sq, tint, depth in (((0, 0.02, 0.05), 0.30, (1.0, 1.15, 1.0), (0.50, 0.46, 0.42), 0.0),
                                         ((sx * 0.12, -0.45, -0.30), 0.30, (1.0, 0.8, 1.0), (0.34, 0.26, 0.30), 0.0045),
                                         ((sx * 0.40, 0.30, -0.55), 0.32, (1.0, 1.0, 1.0), (0.26, 0.20, 0.24), 0.0025)):
            s = np.clip(1.0 - np.linalg.norm((loc - (e + np.array(at))) * np.array(sq), axis=1) / rad, 0.0, 1.0)
            c[:, :3] *= 1.0 - np.minimum(s ** 1.4 * k, 1.0)[:, None] * np.array(tint)
            push += depth * min(k, 1.3) * (s * s * (3 - 2 * s))
    col[used] = c
    nrm = gm.vertex_normals(h['verts'], faces)
    h['verts'][used] -= nrm[used] * (push * 10.0)[:, None]        # the bust's metres at MakeHuman's size: x10 dm
    return col


class Figure:
    """The shaped body and everything measured from it that the clothes are fitted to (Ellis.py's, posed)."""

    def __init__(self):
        mh.ONE_SIDED.update(CP.ONE_FILE)
        self.b = Body(MACRO, FACE, HEIGHT, LIFT, pose=STOOP)
        h = self.b.h
        self.balls = bust.close_lids(h, OPT['lids'])      # the narrowed lids slid over the balls (MakeHuman space)
        self.hf = bust.head_frame(h)
        self.zones = sunken(h, self.hf, OPT['sunken'])
        lf.carve(h, self.hf, self.b.used, FACE_PLANES, self.b.scale, log, 'face planes')
        self.g_head = h['glob']['head']
        self.wv = self.b.world_verts()
        self.names, self.Wv = self.b.vertex_weights()
        self.col = {n: i for i, n in enumerate(self.names)}
        W = lambda *bones: sum(self.b.mh_weight(x) for x in bones)
        sides = ('L', 'R')
        self.w_arm = sum(W(f'shoulder01.{s}', f'upperarm01.{s}', f'upperarm02.{s}', f'lowerarm01.{s}', f'lowerarm02.{s}')
                         for s in sides)
        self.w_hand = sum(W(f'wrist.{s}', *[f'finger{k}-{i}.{s}' for k in range(1, 6) for i in (1, 2, 3)],
                            *[f'metacarpal{k}.{s}' for k in range(1, 5)]) for s in sides)
        self.w_head = self.Wv[:, self.col['head']] + self.Wv[:, self.col['neck_02']] + 0.6 * self.Wv[:, self.col['neck_01']]
        self.w_leg = sum(self.Wv[:, self.col[f'{p}_{s}']] for p in ('thigh', 'calf', 'foot', 'ball') for s in 'lr')
        self.w_foot = sum(self.Wv[:, self.col[f'{p}_{s}']] for p in ('foot', 'ball') for s in 'lr')
        self.faces = self.b.body_faces
        self.fuv = [self.b.h['uv'][fu] for fu, g in zip(self.b.h['fuv'], self.b.h['groups']) if g and g.startswith('body')]
        sk = self.b.skeleton()
        self.sk = sk
        self.neck = sk['neck_01'][0]
        self.shoulder_z = sk['clavicle_l'][1][2]
        self.hip_z = sk['thigh_l'][0][2]
        used = self.b.used
        torso = used[(self.w_arm[used] < 0.3) & (self.w_hand[used] < 0.1) & (self.w_head[used] < 0.3)]
        tz = self.wv[torso]
        lo, hi = self.shoulder_z - 0.30, self.shoulder_z - 0.10
        chest = tz[(tz[:, 2] > lo) & (tz[:, 2] < hi)]
        self.centre = np.array([0.0, (chest[:, 1].min() + chest[:, 1].max()) * 0.5])
        self.torso_idx = torso
        self.eyes = [self.head_point(e) for e in self.hf['eyes']]
        self.eye_z = (self.eyes[0][2] + self.eyes[1][2]) * 0.5
        g = self.g_head[:3, :3]
        # the head's own axes in the figure's space: forward (out of the face), up, its left
        self.axes = [Vector(tuple(mh.to_blender(g @ np.array(a, float)))).normalized()
                     for a in ((0, 0, 1), (0, 1, 0), (1, 0, 0))]
        self.waist = self.hip_z + 0.10                  # the trousers' waistline and the belt's
        log(f'body: {len(used)} vertices, scale {self.b.scale:.3f}, eyes at {self.eye_z:.3f} m, neck {self.neck[2]:.3f}, '
            f'shoulders {self.shoulder_z:.3f}, hips {self.hip_z:.3f}, chest centre y {self.centre[1]:.3f}, '
            f'head joint y {sk["head"][0][1]:.3f} (the neck\'s {self.neck[1]:.3f})')

    def head_point(self, p):
        """A point of the head's rest space (MakeHuman, decimetres) where the posed head carries it, in metres."""
        return self.b.to_world(mh.apply(self.g_head, np.asarray(p, dtype=np.float64)[None]))[0]

    def face_mask(self, vert_mask):
        """Faces of the body whose every vertex is in vert_mask (bool per MakeHuman vertex)."""
        return [bool(vert_mask[f].all()) for f in self.faces]

    def to_portrait(self, ob):
        """Moves an object the portrait built (MakeHuman metres, posed by the same STOOP, no size or lift) into this
        figure's space."""
        s = self.b.scale
        ob.matrix_world = Matrix.Translation((0.0, 0.0, -s * 0.1 * self.b.floor + LIFT)) @ Matrix.Scale(s, 4) @ ob.matrix_world


MACRO = CP.MACRO
FACE = CP.FACE


# ------------------------------------------------------------------------------------------------------- looks

SLOTS = {}
HAT = {}                             # the hat band's height (the felt's sweat stain) and the hat bone's pivot
LOOKS = {}                           # filled by Crane.py (key -> lambda name: material)


def slot(key):
    """The look (looter_looks / crane_looks) for a part of the costume, one material per look; the bake gathers them into
    the texture sets."""
    if key not in SLOTS:
        SLOTS[key] = LOOKS[key](f'Crane_{key}')
        SLOTS[key]['LookKey'] = key
    return SLOTS[key]


# ------------------------------------------------------------------------------------------------------- helpers

def bake_transform(ob):
    ob.data.transform(ob.matrix_world)
    ob.matrix_world = Matrix.Identity(4)


def ellipsoid(name, centre, radii, z_axis, segments=16, rings=10):
    bm = bmesh.new()
    lm.uv_sphere(bm, segments, rings, 1.0)      # fixed face order: the bake must fit the next run
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    rot = Vector(tuple(z_axis)).to_track_quat('Z', 'Y').to_matrix().to_4x4()
    ob.matrix_world = Matrix.Translation(Vector(tuple(centre))) @ rot @ Matrix.Diagonal((*radii, 1.0))
    bake_transform(ob)
    return ob


def uv_cylinder(ob):
    """A plain cylindrical UV map round the object's vertical axis (u round, v up, in metres), for pieces that come
    without one; the bake repacks every island anyway."""
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


def fillet(ob, near, iters=60, step=0.5):
    """Rounds the crease where fused solids meet: the vertices near both (near: 0..1 each) eased toward their
    neighbours' middle (a concave crease fills in)."""
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
    """How far out along each unit direction (k, 2) from the origin a convex polygon round it (counterclockwise
    points) reaches."""
    e = np.roll(hull, -1, axis=0) - hull
    den = dirs[:, 0, None] * e[None, :, 1] - dirs[:, 1, None] * e[None, :, 0]
    with np.errstate(divide='ignore', invalid='ignore'):
        t = (hull[None, :, 0] * e[None, :, 1] - hull[None, :, 1] * e[None, :, 0]) / den
        u = (hull[None, :, 0] * dirs[:, 1, None] - hull[None, :, 1] * dirs[:, 0, None]) / den
    ok = (np.abs(den) > 1e-12) & (t > 0.0) & (u >= -1e-9) & (u <= 1.0 + 1e-9)
    return np.where(ok, t, 0.0).max(axis=1)


def set_seams(ob, seam_dist=None):
    """The 'Seams' colour attribute the looks sew from (looter_looks.construction): R how close each vertex is to
    an open edge, G to a sewn seam (seam_dist, metres per vertex), both 1 there and 0 from 10 cm."""
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


def round_angle(P, centre):
    """Angles of points round a vertical axis at centre (radians: 0 the front, + toward the figure's left)."""
    f, l = gm.frame2d(FWD)
    rel = np.atleast_2d(P)[:, :2] - centre
    return np.arctan2(rel @ l, rel @ f)


def lathe(name, profile, centre, n=24, material=None, cap_top=True):
    """A body of revolution: profile [(radius, height)] from the bottom up about a vertical axis at centre."""
    profile = np.asarray(profile, dtype=np.float64)
    ang = np.linspace(0, 2 * math.pi, n, endpoint=False)
    X = centre[0] + profile[:, 0:1] * np.cos(ang)[None]
    Y = centre[1] + profile[:, 0:1] * np.sin(ang)[None]
    Z = centre[2] + np.repeat(profile[:, 1:2], n, 1)
    pc = gm.grid(X[::-1], Y[::-1], Z[::-1], wrap=True)
    verts = list(pc.verts)
    faces, uv = pc.faces, pc.uv
    for row, at in ((0, profile[-1]), (len(profile) - 1, profile[0])):
        if (row == 0 and not cap_top) or at[0] < 1e-4:
            continue
        c = len(verts)
        verts.append(np.array([centre[0], centre[1], centre[2] + at[1]]))
        for i in range(n):
            a, b = row * n + i, row * n + (i + 1) % n
            faces.append([b, a, c] if row == 0 else [a, b, c])
            uv += [(0.0, 0.0), (0.01, 0.0), (0.005, 0.01)]
    return gm.mesh(name, gm.Piece(np.array(verts), faces, uv), material)


def tris_of(ob):
    return sum(len(p.vertices) - 2 for p in ob.data.polygons)


def delete_faces(ob, pick):
    """Deletes the faces of ob for which pick(centre Vector, face) is true; returns how many went."""
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    dead = [f for f in bm.faces if pick(f.calc_center_median(), f)]
    bmesh.ops.delete(bm, geom=dead, context='FACES')
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.update()
    return len(dead)
