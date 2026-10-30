"""The Stone Sentinel (Docs/Work/2026-10-29-ElementsMatter/CONTRACT.md, lane E; AStoneSentinelCreature is lane D's): a grave
statue that wakes. An original hooded mourner carved in weathered granite, about 2.4 m: a floor-length robe in deep
folds, a shoulder cape, a heavy hood with the face lost in its shadow and bowed over two clasped hands, a cord at the
waist; moss creeping up the folds from the ground and settling on the shoulders, cracks through the robe, the cape and
the hood, a piece of the cape's hem broken away. Its rest pose is the dormant statue (hands clasped, head bowed): a
creature's rest pose is its game idle.

SK_StoneSentinel is the user's Tripo model by default (2026-10-29: Art/Models/Creatures/Tripo/parts_sentinel.py:
remeshed to quads, cut into rigid stone pieces on these bones, the hood bowed, the fists at its sides as modelled, our
granite and moss baked with Tripo's carving into T_TripoStoneSentinel, slot StoneSentinelBody, about 21k triangles,
thighs hidden in the robe with hit capsules). LOOTER_SENTINEL_TRIPO=0 builds the script stand-in below (described next).

Models:
  SK_StoneSentinel        the statue, about 16k triangles, LODs at 50% and 25% (no Nanite: it moves). Slots
                          StoneSentinel (RockGranite on M_World, moss on its upward faces: MossAmount 0.7, MossThreshold
                          0.5) and StoneSentinelMoss (the moss in the folds: the same set, all moss). Vertex colour alpha
                          is baked occlusion, darker still along the cracks.
  SM_StoneSentinelPlinth  its plinth (a cut granite block on a 1.4 m footing, its pivot at its bottom's middle, its top 34
                          cm over it), slot StoneSentinel, one box hull. The creature raises it under itself in play
                          (AStoneSentinelCreature::RaisePlinth puts the mesh's lowest point on the ground).

Bones (the contract's): root (the armature), pelvis, spine_01, spine_02, head, upperarm_l, lowerarm_l, hand_l,
upperarm_r, lowerarm_r, hand_r, thigh_l, calf_l, foot_l, thigh_r, calf_r, foot_r. The sockets are small bones (the model
pipeline gives skeletal meshes no sockets; a skinned mesh's GetSocketLocation finds a bone by its name too):
SOCKET_Head (the face in the hood: the crit) under head, SOCKET_FistL and SOCKET_FistR (the clasped fists) under the
hands. They carry no weights. Their places are logged as SENTINEL lines (Unreal component cm, the rig at rest).

Hit zones (PA_StoneSentinel): a convex hull per bone round the vertices that bone moves most.

Built in creature space (cm: x forward, y left, z up, the origin on the ground under the statue; lc.to_blender turns it
into Blender metres facing -Y).

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\StoneSentinel.py -Preview
        (Saved/ArtPreviews/Creatures/StoneSentinel/StoneSentinel_sheet.png)
    powershell -NoProfile -File Tools\\artrun.ps1 -Export StoneSentinel -Family StoneSentinel
        (Intermediate/ArtExport_StoneSentinel: SK_StoneSentinel.fbx, SM_StoneSentinelPlinth.fbx, the manifest)
"""
import math
import os
import random
import runpy
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_barrow as lb
import looter_creatures as lc
import looter_model as lm
import looter_textures as lt

ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
OUT = os.path.join(lt.PREVIEW_DIR, 'Creatures', 'StoneSentinel')
TO_BLENDER = lc.TO_BLENDER


def log(message):
    print(f'SENTINEL: {message}', flush=True)


def ss(e0, e1, x):
    return lc.smoothstep(e0, e1, x)


def gauss(x, w):
    return math.exp(-(x / w) ** 2)


# --- Materials ---

SET = 'RockGranite'
UV_DENSITY = 420.0           # px a metre: a little finer than the boulders' 320, it is seen close
STONE = lt.material(SET, name='StoneSentinel', MossAmount=0.6, MossThreshold=0.55)
MOSS = lt.material(SET, name='StoneSentinelMoss', tint=0x7d8a52, MossAmount=1.0, MossThreshold=-1.5)
MATS = [STONE, MOSS]

# --- The bones (creature cm), in their dormant pose ---

BOW = math.radians(28.0)                              # the head bowed over the hands
NECK = Vector((5.0, 0.0, 187.0))
HEAD_UP = Vector((math.sin(BOW), 0.0, math.cos(BOW)))   # along the head bone
HEAD_FWD = Vector((math.cos(BOW), 0.0, -math.sin(BOW)))  # where the face looks: down at the hands
HOOD_C = NECK + HEAD_UP * 12.0                       # the hood's middle (the head inside it)
LEFT = Vector((0.0, 1.0, 0.0))


def side(p, s):
    """A left-side point (y > 0) for side s (+1 left, -1 right)."""
    return Vector((p[0], p[1] * s, p[2]))


SHOULDER, ELBOW, WRIST = Vector((0.0, 24.0, 172.0)), Vector((3.0, 33.0, 132.0)), Vector((25.0, 13.0, 121.0))
ARM_PATH = [SHOULDER, Vector((1.5, 31.0, 152.0)), ELBOW, Vector((12.0, 27.0, 125.0)), Vector((20.0, 19.0, 122.0)), WRIST]
ARM_RADII = [11.5, 12.0, 12.0, 12.5, 13.5, 15.0]
HAND = {1.0: (Vector((33.0, 2.5, 121.5)), Vector((37.0, -4.0, 121.0))),     # the left hand lies over the right
        -1.0: (Vector((32.0, -2.5, 116.5)), Vector((37.0, 4.0, 113.5)))}
FIST = {1.0: Vector((33.0, 3.0, 121.5)), -1.0: Vector((32.0, -3.0, 116.5))}
FACE = HOOD_C + HEAD_FWD * 11.0

BONES = [
    ('pelvis', Vector((0.0, 0.0, 100.0)), Vector((0.0, 0.0, 120.0)), None),
    ('spine_01', Vector((0.0, 0.0, 120.0)), Vector((0.0, 0.0, 150.0)), 'pelvis'),
    ('spine_02', Vector((0.0, 0.0, 150.0)), NECK.copy(), 'spine_01'),
    ('head', NECK.copy(), NECK + HEAD_UP * 26.0, 'spine_02'),
]
for s, sfx in ((1.0, 'l'), (-1.0, 'r')):
    hand_c, hand_tip = HAND[s]
    BONES += [
        (f'upperarm_{sfx}', side(SHOULDER, s), side(ELBOW, s), 'spine_02'),
        (f'lowerarm_{sfx}', side(ELBOW, s), side(WRIST, s), f'upperarm_{sfx}'),
        (f'hand_{sfx}', side(WRIST, s), hand_tip.copy(), f'lowerarm_{sfx}'),
        (f'thigh_{sfx}', Vector((0.0, 12.0 * s, 96.0)), Vector((2.0, 13.0 * s, 52.0)), 'pelvis'),
        (f'calf_{sfx}', Vector((2.0, 13.0 * s, 52.0)), Vector((4.0, 13.0 * s, 8.0)), f'thigh_{sfx}'),
        (f'foot_{sfx}', Vector((4.0, 13.0 * s, 8.0)), Vector((22.0, 13.0 * s, 3.0)), f'calf_{sfx}'),
    ]
SOCKET_BONES = [
    ('SOCKET_Head', FACE.copy(), FACE + HEAD_FWD * 5.0, 'head'),
    ('SOCKET_FistL', FIST[1.0].copy(), FIST[1.0] + Vector((5.0, 0.0, 0.0)), 'hand_l'),
    ('SOCKET_FistR', FIST[-1.0].copy(), FIST[-1.0] + Vector((5.0, 0.0, 0.0)), 'hand_r'),
]
BONE_NAMES = [b[0] for b in BONES]


# --- Weights (creature cm) ---

def chain(z):
    """The body's column: pelvis, spine_01, spine_02 by height."""
    a, b = ss(112.0, 132.0, z), ss(148.0, 166.0, z)
    return {'pelvis': 1.0 - a, 'spine_01': a * (1.0 - b), 'spine_02': a * b}


def body_weights(p):
    """The robe: the column above, the legs below the hips (left and right shared across the middle, calf under the
    knee), so a stride swings the skirt."""
    x, y, z = p
    w = chain(z)
    leg = ss(98.0, 62.0, z)
    if leg > 0.0:
        w = {k: v * (1.0 - leg) for k, v in w.items()}
        calf = ss(58.0, 32.0, z)
        left = ss(-16.0, 16.0, y)
        for sfx, share in (('l', left), ('r', 1.0 - left)):
            w[f'thigh_{sfx}'] = leg * (1.0 - calf) * share
            w[f'calf_{sfx}'] = leg * calf * share
    return w


def cape_weights(p):
    """The shoulder cape: spine_02, its sides lifted a little by the upper arms (never most: the hit hulls go by the
    largest weight)."""
    x, y, z = p
    arm = 0.42 * ss(18.0, 40.0, abs(y)) * ss(182.0, 160.0, z)
    sfx = 'l' if y >= 0.0 else 'r'
    return {'spine_02': 1.0 - arm, f'upperarm_{sfx}': arm}


def hood_weights(p):
    low = (Vector(p) - HOOD_C).dot(HEAD_UP)
    h = ss(-19.0, -9.0, low)
    return {'head': h, 'spine_02': 1.0 - h}


# --- Parts ---

class Part:
    """One piece of the statue being built: a bmesh in creature cm with weights and a crack value per vertex."""

    def __init__(self, name):
        self.name = name
        self.bm = bmesh.new()
        self.bm.loops.layers.uv.new('UVMap')
        self.deform = self.bm.verts.layers.deform.verify()
        self.crack = self.bm.verts.layers.float.new('Crack')
        self.points = {}           # dominant bone -> points, for the hit hulls

    def vert(self, co, weights, crack=0.0):
        v = self.bm.verts.new(co)
        total = sum(w for w in weights.values() if w > 1e-4)
        best = max(weights.items(), key=lambda kv: kv[1])[0]
        for bone, w in weights.items():
            if w > 1e-4:
                v[self.deform][BONE_NAMES.index(bone)] = w / total
        v[self.crack] = crack
        self.points.setdefault(best, []).append(Vector(co))
        return v

    def face(self, verts, mat=0):
        f = self.bm.faces.new(verts)
        f.material_index = mat
        f.smooth = True
        return f

    def finish(self, recalc=False):
        if recalc:
            bmesh.ops.recalc_face_normals(self.bm, faces=list(self.bm.faces))
        bmesh.ops.transform(self.bm, matrix=TO_BLENDER, verts=list(self.bm.verts))
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        for mat in MATS:
            mesh.materials.append(mat)
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        for bone in BONE_NAMES:
            obj.vertex_groups.new(name=bone)
        return obj


def grid(part, rows, closed=True, mats=None):
    """Quads between rows of vertices (each row a loop round, bottom to top, counter-clockwise from above)."""
    n = len(rows[0])
    for i in range(len(rows) - 1):
        for j in range(n if closed else n - 1):
            k = (j + 1) % n
            part.face((rows[i][j], rows[i][k], rows[i + 1][k], rows[i + 1][j]), mats(i, j) if mats else 0)


def fan(part, ring, centre, down):
    n = len(ring)
    for j in range(n):
        k = (j + 1) % n
        part.face((ring[k], ring[j], centre) if down else (ring[j], ring[k], centre))


def weather(p):
    """Erosion: the stone's surface wandering a centimetre or so, broad and fine."""
    q = Vector(p)
    return 0.85 * noise.noise(q * 0.05 + Vector((3.1, 7.4, 1.9))) + 0.35 * noise.noise(q * 0.17 + Vector((9.2, 2.6, 5.5)))


def jagged(points, seed, step=3.5, wander=1.6):
    """A crack's line through these control points (param space, cm), broken into short jagged runs."""
    rng = random.Random(seed)
    out = [points[0]]
    for (a0, b0), (a1, b1) in zip(points, points[1:]):
        length = math.hypot(a1 - a0, b1 - b0)
        count = max(1, int(length / step))
        na, nb = -(b1 - b0) / max(length, 1e-6), (a1 - a0) / max(length, 1e-6)
        for k in range(1, count + 1):
            t = k / count
            off = rng.uniform(-wander, wander) if k < count else 0.0
            out.append((a0 + (a1 - a0) * t + na * off, b0 + (b1 - b0) * t + nb * off))
    return out


def crack_at(a, b, cracks, width):
    """How deep in a crack (0..1) the point (a, b) is (param space, cm): 1 on a crack's line, 0 from width out."""
    best = 0.0
    for line in cracks:
        for (a0, b0), (a1, b1) in zip(line, line[1:]):
            da, db = a1 - a0, b1 - b0
            ll = da * da + db * db
            t = 0.0 if ll < 1e-9 else max(0.0, min(1.0, ((a - a0) * da + (b - b0) * db) / ll))
            d = math.hypot(a - (a0 + da * t), b - (b0 + db * t))
            if d < width:
                best = max(best, (1.0 - d / width) ** 1.5)
    return best


def catmull(points, values, count):
    """count points (and values) along a Catmull-Rom curve through points."""
    pts = [points[0]] + list(points) + [points[-1]]
    vals = [values[0]] + list(values) + [values[-1]]
    out = []
    segments = len(points) - 1
    for i in range(count):
        u = i / (count - 1) * segments
        k = min(int(u), segments - 1)
        t = u - k
        p0, p1, p2, p3 = pts[k], pts[k + 1], pts[k + 2], pts[k + 3]
        t2, t3 = t * t, t * t * t
        p = 0.5 * ((2.0 * p1) + (-p0 + p2) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3)
        v = vals[k + 1] + (vals[k + 2] - vals[k + 1]) * t
        out.append((p, v))
    return out


# --- The robe ---

ROBE = [(0.0, 44.0, 50.0), (3.0, 45.0, 51.0), (9.0, 42.0, 48.0), (25.0, 38.5, 44.5), (50.0, 34.5, 40.5),
        (75.0, 31.0, 37.0), (98.0, 28.5, 34.0), (110.0, 28.0, 33.5), (125.0, 28.5, 35.5), (145.0, 28.5, 38.5),
        (160.0, 27.0, 40.0), (170.0, 25.0, 37.5), (178.0, 21.0, 29.0), (183.0, 16.0, 20.0), (186.0, 12.0, 13.0)]
ROBE_AROUND, ROBE_TOP = 64, 186.0
ROBE_R = 37.0                 # the robe's mean radius: crack widths in its param space are cm on it
ROBE_CRACKS = [
    jagged([(0.42 * ROBE_R, 104.0), (0.5 * ROBE_R, 80.0), (0.4 * ROBE_R, 58.0), (0.58 * ROBE_R, 30.0)], 11),
    jagged([(0.5 * ROBE_R, 80.0), (0.75 * ROBE_R, 66.0), (0.86 * ROBE_R, 52.0)], 12),
    jagged([(-2.45 * ROBE_R, 152.0), (-2.2 * ROBE_R, 120.0), (-2.3 * ROBE_R, 96.0), (-2.05 * ROBE_R, 66.0)], 13),
    jagged([(-0.62 * ROBE_R, 0.0), (-0.55 * ROBE_R, 18.0), (-0.66 * ROBE_R, 36.0)], 14),
    jagged([(1.95 * ROBE_R, 0.0), (1.85 * ROBE_R, 22.0)], 15),
]


def profile(table, z):
    """The table's (rx, ry) at z: a smooth curve through its keys."""
    zs = [k[0] for k in table]
    if z <= zs[0]:
        return table[0][1], table[0][2]
    if z >= zs[-1]:
        return table[-1][1], table[-1][2]
    i = max(k for k in range(len(zs) - 1) if zs[k] <= z)
    t = (z - zs[i]) / (zs[i + 1] - zs[i])
    w = t * t * (3.0 - 2.0 * t)
    return (table[i][1] + (table[i + 1][1] - table[i][1]) * w, table[i][2] + (table[i + 1][2] - table[i][2]) * w)


def robe_fold(th, z):
    """The robe's folds (cm out from its line): long vertical folds deepening toward the hem, round ridges and sharp
    valleys, and a deep pleat falling from under the hands."""
    a = 0.6 + 3.6 * ss(108.0, 6.0, z)
    f = 0.55 * math.cos(7.0 * th + 0.4) + 0.35 * math.cos(11.0 * th + 1.9) + 0.22 * math.cos(17.0 * th + 4.1)
    v = (f - 0.32 * abs(f)) * a
    v -= 3.0 * gauss(th, 0.16) * ss(114.0, 60.0, z) * (1.0 - 0.4 * ss(30.0, 0.0, z))
    v *= ss(186.0, 170.0, z)
    return v


def robe_moss(th, z, fold):
    """Moss creeping up the robe's folds from the ground: in the valleys, thinning out by 70 cm."""
    a = 0.6 + 3.6 * ss(108.0, 6.0, z)
    valley = -fold / a
    q = Vector((math.cos(th) * 4.0, math.sin(th) * 4.0, z * 0.09))
    patch = noise.noise(q + Vector((4.4, 1.3, 8.8))) + 0.5 * noise.noise(q * 2.3 + Vector((7.1, 3.3, 0.4)))
    reach = 24.0 + 22.0 * noise.noise(Vector((math.cos(th) * 2.0, math.sin(th) * 2.0, 0.0)) + Vector((1.9, 8.2, 3.6)))
    return z < reach and valley * 0.7 + patch > 0.45 + 0.4 * z / max(reach, 1.0)


def robe(part):
    rows = []
    ring_params = []
    zs = [ROBE_TOP * i / 54.0 for i in range(55)]
    for z in zs:
        rx, ry = profile(ROBE, z)
        lean = 2.5 * ss(125.0, 182.0, z)
        row, params = [], []
        for j in range(ROBE_AROUND):
            th = 2.0 * math.pi * j / ROBE_AROUND - math.pi
            c, s = math.cos(th), math.sin(th)
            n = Vector((c / rx, s / ry, 0.0)).normalized()
            base = Vector((rx * c + lean, ry * s, z))
            fold = robe_fold(th, z)
            crack = crack_at(th * ROBE_R, z, ROBE_CRACKS, 5.5) * ss(186.0, 175.0, z)
            disp = fold + weather(base) * ss(186.0, 176.0, z) - 4.0 * crack
            co = base + n * disp
            if z == 0.0:
                co.z = 0.0
            row.append(part.vert(co, body_weights(co), crack))
            params.append((th, z, fold))
        rows.append(row)
        ring_params.append(params)

    def mat(i, j):
        th, z, fold = ring_params[i][j]
        return 1 if robe_moss(th, z + 1.5, fold) else 0
    grid(part, rows, mats=mat)
    fan(part, rows[0], part.vert(Vector((0.0, 0.0, 0.0)), body_weights((0.0, 0.0, 0.0))), down=True)
    top = Vector((2.5, 0.0, ROBE_TOP + 1.0))
    fan(part, rows[-1], part.vert(top, body_weights(top)), down=False)


def belt(part):
    """A cord round the waist, knotted at the left hip, its two ends hanging down the robe."""
    z0, sides = 109.0, 6
    rx, ry = profile(ROBE, z0)
    ring_pts = []
    for j in range(64):
        th = 2.0 * math.pi * j / 64 - math.pi
        sag = -2.0 * gauss(th - 0.0, 0.5)
        ring_pts.append(Vector(((rx + 1.6) * math.cos(th), (ry + 1.6) * math.sin(th), z0 + sag)))
    cord(part, ring_pts, 1.8, sides, closed=True, weights=lambda p: chain(p[2]))
    th = 0.95
    knot = Vector(((rx + 3.0) * math.cos(th), (ry + 3.0) * math.sin(th), z0))
    lump(part, knot, (4.2, 3.6, 3.8), lambda p: chain(p[2]))
    for k, (dth, drop) in enumerate(((0.06, 36.0), (-0.07, 30.0))):
        pts = []
        for i in range(9):
            t = i / 8.0
            z = z0 - 2.0 - drop * t
            rxz, ryz = profile(ROBE, z)
            a = th + dth * t
            out = 2.6 + robe_fold(a, z) * 0.6
            pts.append(Vector(((rxz + out) * math.cos(a), (ryz + out) * math.sin(a), z)))
        cord(part, pts, 1.5 - 0.3 * k, sides, closed=False, weights=body_weights)
        lump(part, pts[-1] + Vector((0.0, 0.0, -1.0)), (2.4, 2.4, 3.2), body_weights)


def cord(part, pts, radius, sides, closed, weights):
    rows = []
    n = len(pts)
    for i, p in enumerate(pts):
        nxt = pts[(i + 1) % n] if closed or i < n - 1 else p + (p - pts[i - 1])
        prv = pts[(i - 1) % n] if closed or i > 0 else p - (pts[1] - p)
        t = (nxt - prv).normalized()
        a = t.cross(Vector((0.0, 0.0, 1.0)))
        if a.length < 1e-3:
            a = t.cross(Vector((1.0, 0.0, 0.0)))
        a.normalize()
        b = a.cross(t)
        row = []
        for k in range(sides):
            ang = 2.0 * math.pi * k / sides
            co = p + (a * math.cos(ang) + b * math.sin(ang)) * radius
            row.append(part.vert(co, weights(co)))
        rows.append(row)
    if closed:
        rows.append(rows[0])
    for i in range(len(rows) - 1):
        for k in range(sides):
            kk = (k + 1) % sides
            part.face((rows[i][k], rows[i][kk], rows[i + 1][kk], rows[i + 1][k]))
    if not closed:
        fan(part, rows[0], part.vert(pts[0], weights(pts[0])), down=True)
        fan(part, rows[-1], part.vert(pts[-1], weights(pts[-1])), down=False)


def lump(part, centre, radii, weights, segs=10, rings=6, axes=None, mat=0, rough=0.0):
    """A small ellipsoid (a knot, a knuckle, a toe), its axes x, y, z (or the given frame), roughened a little."""
    ax = axes or (Vector((1.0, 0.0, 0.0)), Vector((0.0, 1.0, 0.0)), Vector((0.0, 0.0, 1.0)))
    tmp = bmesh.new()
    lm.uv_sphere(tmp, segs, rings, 1.0)
    tmp.verts.index_update()
    made = []
    for v in tmp.verts:
        d = v.co
        co = centre + ax[0] * (d.x * radii[0]) + ax[1] * (d.y * radii[1]) + ax[2] * (d.z * radii[2])
        if rough:
            co += (co - centre).normalized() * rough * noise.noise(co * 0.2 + Vector((1.7, 5.1, 2.3)))
        made.append(co)
    faces = [[v.index for v in f.verts] for f in tmp.faces]
    tmp.free()
    verts = [part.vert(co, weights(co)) for co in made]
    for f in faces:
        part.face([verts[i] for i in f], mat)
    return verts


# --- The shoulder cape ---

CAPE_AROUND = 72
CAPE_KEYS = [(0.0, 194.0, 15.0, 16.0, 4.0), (0.12, 190.0, 21.0, 25.0, 1.5), (0.25, 184.0, 27.0, 36.0, -0.5),
             (0.38, 175.5, 30.5, 45.0, -1.0), (0.5, 166.0, 32.0, 49.0, -1.0)]
CAPE_HEM_R = (32.5, 50.0)
CAPE_R = 44.0                 # crack param space: theta * CAPE_R across, s * CAPE_DROP down (both about cm)
CAPE_DROP = 60.0
CAPE_CRACKS = [
    jagged([(0.95 * CAPE_R, 5.0), (1.05 * CAPE_R, 18.0), (1.3 * CAPE_R, 33.0), (1.22 * CAPE_R, 48.0)], 21),
    jagged([(-1.6 * CAPE_R, 12.0), (-1.75 * CAPE_R, 27.0)], 22),
    jagged([(-0.32 * CAPE_R, 20.0), (-0.18 * CAPE_R, 34.0), (-0.24 * CAPE_R, 46.0), (-0.05 * CAPE_R, 60.0)], 23),
]


def cape_hem(th):
    """The cape's hem height: longest in front, over the elbows at the sides, a piece broken out of its back left."""
    hem = 131.0 + 19.0 * ((1.0 + math.cos(th)) * 0.5) ** 2
    notch = gauss(th - 2.3, 0.2)
    if notch > 0.05:
        rng = random.Random(int(th * 1000.0))
        hem += notch * (9.0 + rng.uniform(-2.0, 2.0))
    return hem


def cape_at(th, s):
    """(centre x, z, rx, ry) of the cape's ring at s (0 at the neck, 1 at the hem)."""
    if s <= 0.5:
        k = max(i for i in range(len(CAPE_KEYS) - 1) if CAPE_KEYS[i][0] <= s) if s < 0.5 else len(CAPE_KEYS) - 2
        a, b = CAPE_KEYS[k], CAPE_KEYS[k + 1]
        t = (s - a[0]) / (b[0] - a[0])
        w = t * t * (3.0 - 2.0 * t)
        return tuple(a[i] + (b[i] - a[i]) * w for i in (4, 1, 2, 3))
    t = (s - 0.5) / 0.5
    top = CAPE_KEYS[-1]
    return (top[4], top[1] + (cape_hem(th) - top[1]) * t, top[2] + (CAPE_HEM_R[0] - top[2]) * t,
            top[3] + (CAPE_HEM_R[1] - top[3]) * t)


def cape(part):
    ss_rows = [i / 30.0 for i in range(31)]
    rows, params = [], []
    for s in ss_rows:
        row, prm = [], []
        for j in range(CAPE_AROUND):
            th = 2.0 * math.pi * j / CAPE_AROUND - math.pi
            cx, z, rx, ry = cape_at(th, s)
            c, sn = math.cos(th), math.sin(th)
            n = Vector((c / rx, sn / ry, 0.4 * (1.0 - ss(0.2, 0.5, s)))).normalized()
            base = Vector((cx + rx * c, ry * sn, z))
            a = 0.3 + 4.2 * ss(0.3, 1.0, s)
            f = 0.6 * math.cos(9.0 * th + 1.2) + 0.4 * math.cos(14.0 * th + 0.3) + 0.25 * math.cos(5.0 * th + 2.6)
            fold = (f - 0.35 * abs(f)) * a
            # The hem follows its folds: the ridges hang a little lower than the valleys.
            base.z -= 2.2 * f * ss(0.75, 1.0, s)
            crack = crack_at(th * CAPE_R, s * CAPE_DROP, CAPE_CRACKS, 5.0)
            co = base + n * (fold + weather(base) * 0.9 - 3.0 * crack)
            row.append(part.vert(co, cape_weights(co), crack))
            prm.append((th, s, fold, a))
        rows.append(row)
        params.append(prm)
    last = len(rows) - 1

    def mat(i, j):
        # i counts up from the hem (the rows are laid bottom to top below); the face's upper row's params.
        th, s, fold, a = params[last - i - 1][j]
        patch = noise.noise(Vector((math.cos(th) * 2.5, math.sin(th) * 2.5, s * 4.0)) + Vector((2.2, 6.1, 0.7)))
        return 1 if s < 0.22 and patch > 0.05 else 0      # moss in the crease where the hood meets the cape
    grid(part, rows[::-1], mats=mat)
    # The hem rolled under, 3 cm thick, so the cape has weight at its edge (the inside is never seen): a strip turned in
    # under the hem (facing down), then one up its inside (facing the body).
    hem = rows[-1]
    under = []
    for j, v in enumerate(hem):
        th = params[-1][j][0]
        inward = Vector((-math.cos(th), -math.sin(th), 0.0))
        co = v.co + inward * 3.2 + Vector((0.0, 0.0, 2.0))
        under.append(part.vert(co, cape_weights(co)))
    grid(part, [under, hem])
    rim = []
    for j, v in enumerate(under):
        th = params[-1][j][0]
        co = v.co + Vector((0.0, 0.0, 6.0)) + Vector((-math.cos(th), -math.sin(th), 0.0)) * 0.5
        rim.append(part.vert(co, cape_weights(co)))
    grid(part, [rim, under])


# --- The hood ---

HOOD_R = (26.0, 25.0, 30.0)      # forward, across, up
HOOD_PEAK = (-HEAD_FWD * 0.5 + HEAD_UP * 0.85).normalized()
HOOD_OPEN = 0.95                 # the opening's half-angle (radians), in its own squashed measure
HOOD_CM = 22.0                # crack param space: (round the axis b, back from the face a) * HOOD_CM, about cm
HOOD_CRACKS = [jagged([(0.35 * HOOD_CM, 1.15 * HOOD_CM), (0.5 * HOOD_CM, 1.45 * HOOD_CM), (0.75 * HOOD_CM, 1.75 * HOOD_CM),
                       (0.95 * HOOD_CM, 2.1 * HOOD_CM)], 31)]


def hood(part):
    rings, segs = 28, 44
    left = LEFT
    f, u = HEAD_FWD, HEAD_UP

    def point(a, b):
        d = f * math.cos(a) + (u * math.cos(b) + left * math.sin(b)) * math.sin(a)
        df, dl, du = d.dot(f), d.dot(left), d.dot(u)
        r = 1.0 / math.sqrt((df / HOOD_R[0]) ** 2 + (dl / HOOD_R[1]) ** 2 + (du / HOOD_R[2]) ** 2)
        r *= 1.0 + 0.32 * max(0.0, d.dot(HOOD_PEAK)) ** 4
        sq = Vector((df, dl * 1.25, du * 0.92)).normalized()
        alpha = math.acos(max(-1.0, min(1.0, sq.x)))
        crack = 0.0
        if alpha < HOOD_OPEN:
            t = alpha / HOOD_OPEN
            r *= 0.28 + 0.72 * ss(0.6, 1.0, t) ** 1.4
            # The face in its shadow: only a brow and the line of a nose, bowed.
            r += 2.2 * gauss(dl, 0.07) * gauss(du + 0.05, 0.22) * (1.0 - t)
            r += 1.2 * gauss(du - 0.32, 0.08) * gauss(dl, 0.4) * (1.0 - t)
        else:
            r *= 1.0 + 0.075 * gauss(alpha - HOOD_OPEN - 0.07, 0.07)
            # Soft folds running back from the rim over the crown.
            r += 0.9 * math.cos(5.0 * b + 0.8) * ss(1.25, 2.3, a)
            r += weather(HOOD_C + d * r) * 0.8
            crack = crack_at(b * HOOD_CM, a * HOOD_CM, HOOD_CRACKS, 4.5)
            r -= 2.6 * crack
        return HOOD_C + d * r, crack

    front, _ = point(0.0, 0.0)
    rows = []
    for i in range(1, rings):
        a = math.pi * i / rings
        row = []
        for j in range(segs):
            b = 2.0 * math.pi * j / segs
            co, crack = point(a, b)
            row.append(part.vert(co, hood_weights(co), crack))
        rows.append(row)
    back, _ = point(math.pi, 0.0)
    vf = part.vert(front, hood_weights(front))
    vb = part.vert(back, hood_weights(back))
    # Rows run from the face backward, each counter-clockwise seen from the front.
    for j in range(segs):
        k = (j + 1) % segs
        part.face((vf, rows[0][j], rows[0][k]))
        part.face((vb, rows[-1][k], rows[-1][j]))
    for i in range(len(rows) - 1):
        for j in range(segs):
            k = (j + 1) % segs
            part.face((rows[i][j], rows[i + 1][j], rows[i + 1][k], rows[i][k]))


# --- The arms ---

def arm(part, s):
    """A bell sleeve from the shoulder (under the cape) to the clasped hands, hanging lower toward its cuff, its mouth
    turned in with a dark hollow inside."""
    sfx = 'l' if s > 0 else 'r'
    path = [side(p, s) for p in ARM_PATH]
    samples = catmull(path, ARM_RADII, 18)
    lengths = [0.0]
    for (p, _), (q, _) in zip(samples, samples[1:]):
        lengths.append(lengths[-1] + (q - p).length)
    elbow_at = min(range(len(samples)), key=lambda i: (samples[i][0] - side(ELBOW, s)).length)
    elbow_len = lengths[elbow_at]

    def weights(along):
        lower = ss(elbow_len - 6.0, elbow_len + 6.0, along)
        sp = 0.5 * ss(9.0, 0.0, along)
        return {'spine_02': sp, f'upperarm_{sfx}': (1.0 - lower) * (1.0 - sp), f'lowerarm_{sfx}': lower * (1.0 - sp)}
    sides_n = 18
    rows = []
    frames = []
    for i, (c, r) in enumerate(samples):
        nxt = samples[min(i + 1, len(samples) - 1)][0]
        prv = samples[max(i - 1, 0)][0]
        t = (nxt - prv).normalized()
        down = Vector((0.0, 0.0, -1.0)) - t * t.dot(Vector((0.0, 0.0, -1.0)))
        down.normalize()
        out = down.cross(t)
        hang = 0.6 * ss(elbow_len * 0.8, lengths[-1], lengths[i])
        row = []
        for k in range(sides_n):
            ph = 2.0 * math.pi * k / sides_n
            cs, sn = math.cos(ph), math.sin(ph)
            co = c + out * (r * cs) + down * (r * sn * (1.0 + hang if sn > 0.0 else 1.0))
            fold = 0.7 * math.cos(3.0 * ph + i * 0.35) * ss(0.0, 0.5, lengths[i] / lengths[-1])
            co += (co - c).normalized() * (fold + weather(co) * 0.6)
            row.append(part.vert(co, weights(lengths[i])))
        rows.append(row)
        frames.append((c, t, out, down, r, hang))
    # The cuff: a lip, then turned in and back into a hollow.
    c, t, out, down, r, hang = frames[-1]
    lip, inner = [], []
    for k in range(sides_n):
        ph = 2.0 * math.pi * k / sides_n
        cs, sn = math.cos(ph), math.sin(ph)
        sag = (1.0 + hang if sn > 0.0 else 1.0)
        lip.append(part.vert(c + t * 1.2 + out * (r * 1.04 * cs) + down * (r * 1.04 * sn * sag), weights(lengths[-1])))
        inner.append(part.vert(c - t * 3.5 + out * (r * 0.74 * cs) + down * (r * 0.74 * sn * sag), weights(lengths[-1])))
    rows += [lip, inner]
    for i in range(len(rows) - 1):
        for k in range(sides_n):
            kk = (k + 1) % sides_n
            part.face((rows[i][k], rows[i + 1][k], rows[i + 1][kk], rows[i][kk]))
    hollow = part.vert(c - t * 7.0 + down * (r * 0.2), weights(lengths[-1]))
    fan(part, inner, hollow, down=False)
    start = part.vert(samples[0][0] - (samples[1][0] - samples[0][0]).normalized() * 2.0, weights(0.0))
    fan(part, rows[0], start, down=True)


def hands(part):
    """The clasped hands: the left laid over the right, its fingers wrapped round the right hand's back, the right's
    thumb tucked up beside them; heavy and simple, as a carver leaves them."""
    for s, sfx in ((1.0, 'l'), (-1.0, 'r')):
        centre, tip = HAND[s]
        wrist = side(WRIST, s)
        fwd = (tip - wrist).normalized()
        up = Vector((0.0, 0.0, 1.0))
        across = up.cross(fwd).normalized()
        up = fwd.cross(across).normalized()
        w = {f'hand_{sfx}': 1.0}
        lump(part, centre, (10.5, 8.4, 5.0 if s > 0 else 5.6), lambda p, w=w: w, 14, 8, (fwd, across, up), rough=0.4)
        # The wrist where it leaves the sleeve.
        lump(part, wrist + fwd * 3.0, (5.0, 5.5, 4.8), lambda p, w=w: w, 10, 6, (fwd, across, up))
    # The left hand's four fingers curled over the right hand's back, and its thumb along the top.
    lw = {'hand_l': 1.0}
    for k in range(4):
        base = Vector((37.5 - k * 2.6, -4.0 - k * 0.6, 120.5 - k * 0.4))
        lump(part, base + Vector((1.5, -1.5, -3.5)), (2.0, 2.4, 4.6), lambda p: lw, 8, 5,
             (Vector((1.0, 0.0, 0.0)), Vector((0.0, 1.0, 0.0)), Vector((0.15, -0.35, 1.0)).normalized()))
    lump(part, Vector((31.0, 0.5, 124.0)), (5.5, 2.4, 2.2), lambda p: lw, 8, 5,
         (Vector((0.8, -0.6, 0.0)).normalized(), Vector((0.6, 0.8, 0.0)).normalized(), Vector((0.0, 0.0, 1.0))))
    rw = {'hand_r': 1.0}
    lump(part, Vector((35.0, 5.0, 117.5)), (2.4, 4.8, 2.2), lambda p: rw, 8, 5,
         (Vector((1.0, 0.0, 0.0)), Vector((0.0, 0.85, 0.5)).normalized(), Vector((0.0, -0.5, 0.85)).normalized()))


def toes(part):
    """The toes of two sandalled feet under the hem's front."""
    for s, sfx in ((1.0, 'l'), (-1.0, 'r')):
        w = {f'foot_{sfx}': 1.0}
        verts = lump(part, Vector((38.0, 12.5 * s, 3.6)), (9.5, 7.0, 4.6), lambda p, w=w: w, 12, 7, rough=0.3)
        for v in verts:
            v.co.z = max(v.co.z, 0.0)


# --- The plinth ---

PLINTH_TOP = 34.0           # cm: where the statue stands
PLINTH_FOOT = 6.0           # its footing's height (its bottom is its pivot: AStoneSentinelCreature::RaisePlinth sets the
                            # mesh's lowest point on the ground)


def plinth():
    """SM_StoneSentinelPlinth: a footing, the block with a sunken panel on its front, a moulded cap with a corner
    knocked off; one box hull."""
    bm = bmesh.new()
    bm.loops.layers.uv.new('UVMap')

    def block(size, centre, bevel, segments=1):
        made = bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.LocRotScale(Vector(centre), None, Vector(size)))
        verts = made['verts']
        faces = list({f for v in verts for f in v.link_faces})
        faces.sort(key=lambda f: tuple(round(c, 3) for c in f.calc_center_median()))
        return verts, faces

    _, foot = block((140.0, 140.0, PLINTH_FOOT), (0.0, 0.0, PLINTH_FOOT * 0.5), 0.0)
    body_verts, body = block((122.0, 122.0, 21.0), (0.0, 0.0, PLINTH_FOOT + 10.5), 0.0)
    bm.normal_update()
    front = next(f for f in body if f.normal.x > 0.9)
    inset = bmesh.ops.inset_individual(bm, faces=[front], thickness=5.0, depth=-1.6)
    cap_verts, cap = block((134.0, 134.0, 8.5), (0.0, 0.0, PLINTH_TOP - 4.25), 0.0)
    bmesh.ops.bevel(bm, geom=[e for e in bm.edges if all(v in cap_verts for v in e.verts)], offset=2.2, segments=2,
                    affect='EDGES', clamp_overlap=True)
    # A corner of the cap knocked off (back left), and the front right one worn round.
    for v in list(bm.verts):
        if v.co.z > PLINTH_TOP - 9.5:
            d = (Vector((v.co.x, v.co.y, 0.0)) - Vector((-67.0, 67.0, 0.0))).length
            if d < 16.0:
                v.co += Vector((5.0, -5.0, -3.5)) * (1.0 - d / 16.0)
            d2 = (Vector((v.co.x, v.co.y, 0.0)) - Vector((67.0, -67.0, 0.0))).length
            if d2 < 10.0:
                v.co += Vector((-2.0, 2.0, -1.5)) * (1.0 - d2 / 10.0)
    for f in bm.faces:
        f.smooth = False
    bmesh.ops.transform(bm, matrix=TO_BLENDER, verts=list(bm.verts))
    obj = lb.obj_from_bm('StoneSentinelPlinth', bm, [STONE], smooth=False)
    lm.hull_box(obj, (1.40, 1.40, PLINTH_TOP / 100.0), (0.0, 0.0, PLINTH_TOP / 200.0))
    lt.box_uv(obj, SET, texel_density=UV_DENSITY, seed=5)
    lt.bake_vertex_ao(obj, samples=24, distance=0.6)
    return obj


# --- Build ---

def hull(arm_obj, bone, points):
    bm = bmesh.new()
    for p in points:
        bm.verts.new(TO_BLENDER @ p)
    made = bmesh.ops.convex_hull(bm, input=list(bm.verts))
    inside = [v for v in made['geom_interior'] + made['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(dict.fromkeys(inside)), context='VERTS')
    return lm._hit_shape('UCX_', arm_obj, bone, bm, Matrix.Identity(4))


def darken_cracks(obj, strength=0.85):
    """The cracks' depth (the Crack attribute) taken off the baked occlusion, so a crack reads as a dark line from afar."""
    mesh = obj.data
    attr = mesh.attributes.get('Crack')
    if attr is None:
        return
    crack = [0.0] * len(mesh.vertices)
    attr.data.foreach_get('value', crack)
    col = mesh.color_attributes['Col']
    for loop in mesh.loops:
        c = crack[loop.vertex_index]
        if c > 0.0:
            rgba = list(col.data[loop.index].color)
            rgba[3] *= 1.0 - strength * min(1.0, c * 1.3)
            col.data[loop.index].color = rgba
    log(f'cracks darkened at {sum(1 for c in crack if c > 0.0)} of {len(crack)} vertices')
    mesh.attributes.remove(attr)


def build():
    parts = {name: Part(name) for name in ('Robe', 'Cape', 'Hood', 'Arms', 'Hands')}
    robe(parts['Robe'])
    belt(parts['Robe'])
    toes(parts['Robe'])
    cape(parts['Cape'])
    hood(parts['Hood'])
    arm(parts['Arms'], 1.0)
    arm(parts['Arms'], -1.0)
    hands(parts['Hands'])
    points = {}
    for part in parts.values():
        for bone, pts in part.points.items():
            points.setdefault(bone, []).extend(pts)
    objects = [parts['Robe'].finish(recalc=True), parts['Cape'].finish(), parts['Hood'].finish(recalc=True),
               parts['Arms'].finish(recalc=True), parts['Hands'].finish(recalc=True)]

    rig = lm.armature()
    lm.bones(rig, [(n, lc.to_blender(h), lc.to_blender(t), p) for n, h, t, p in BONES + SOCKET_BONES])
    for name in BONE_NAMES:
        if name not in points:
            raise RuntimeError(f'no vertex is mostly {name}\'s: it would have no hit zone')
        hull(rig, name, points[name])

    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    body = bpy.context.view_layer.objects.active
    body.name = body.data.name = 'StoneSentinel'
    lt.box_uv(body, SET, texel_density=UV_DENSITY, seed=3)
    body.data.set_sharp_from_angle(angle=math.radians(62.0))
    body.parent = rig
    body.modifiers.new('Armature', 'ARMATURE').object = rig
    lt.bake_vertex_ao(body, samples=24, distance=0.45)
    darken_cracks(body)
    rig['LODs'] = '50,25'
    rig['LODScreens'] = '0.35,0.15'
    return rig, body


# The user's Tripo model (Art/Models/Creatures/Tripo/parts_sentinel.py: cleaned, remeshed, cut into rigid pieces, posed into
# the dormant statue, our granite look) unless LOOTER_SENTINEL_TRIPO=0 (this script's stand-in).
TRIPO = os.environ.get('LOOTER_SENTINEL_TRIPO', '1') != '0'
SENTINEL = runpy.run_path(os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Tripo', 'parts_sentinel.py')) if TRIPO else None
RIG, BODY = SENTINEL['build']() if TRIPO else build()
PLINTH = plinth()


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def report():
    if TRIPO:
        log(f'SM_StoneSentinelPlinth: {triangles(PLINTH)} triangles, pivot at its bottom, top {PLINTH_TOP:.0f} cm over it')
        return
    lo, hi = lt._bounds([BODY])
    log(f'SK_StoneSentinel: {triangles(BODY)} triangles, {len(RIG.data.bones)} bones, height {hi.z:.2f} m, '
        f'width {hi.x - lo.x:.2f} m, depth {hi.y - lo.y:.2f} m; materials {", ".join(m.name for m in BODY.data.materials)}')
    log(f'SM_StoneSentinelPlinth: {triangles(PLINTH)} triangles, pivot at its bottom, top {PLINTH_TOP:.0f} cm over it')
    for name, h, t, parent in BONES + SOCKET_BONES:
        # Creature cm (x forward, y left) to Unreal component cm (x forward, y right).
        log(f'bone {name} (under {parent or "root"}) at ({h.x:.1f}, {-h.y:.1f}, {h.z:.1f}) cm')


report()


# --- Previews (Saved/ArtPreviews/Creatures/StoneSentinel) ---

def preview_moss():
    """Moss in the preview as the game's master grows it (Blender's material doesn't): green on the stone's upward
    faces, the moss slot all moss."""
    green = lt.hex_color(0x4f6534)
    for mat, amount, threshold in ((STONE, 0.7, 0.5), (MOSS, 1.0, -1.5)):
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
        src = bsdf.inputs['Base Color'].links[0].from_socket
        geo = nodes.new('ShaderNodeNewGeometry')
        sep = nodes.new('ShaderNodeSeparateXYZ')
        links.new(geo.outputs['Normal'], sep.inputs['Vector'])
        ramp = nodes.new('ShaderNodeMapRange')
        ramp.inputs['From Min'].default_value = threshold
        ramp.inputs['From Max'].default_value = threshold + 0.2
        ramp.inputs['To Max'].default_value = amount
        links.new(sep.outputs['Z'], ramp.inputs['Value'])
        mix = nodes.new('ShaderNodeMix')
        mix.data_type = 'RGBA'
        links.new(ramp.outputs['Result'], mix.inputs['Factor'])
        links.new(src, mix.inputs[6])
        mix.inputs[7].default_value = green
        links.new(mix.outputs[2], bsdf.inputs['Base Color'])


def previews():
    os.makedirs(OUT, exist_ok=True)
    preview_moss()
    RIG.location = (0.0, 0.0, PLINTH_TOP / 100.0)
    bpy.context.view_layer.update()
    scale = lm.cylinder('_Figure', 0.16, 1.8, center=(1.35, 0.4, 0.9), segments=16,
                        material=lm.material('_FigureGrey', 0x8a8f96))
    shown = [BODY, PLINTH]
    target = (0.0, 0.0, 1.45)
    sun = (-0.55, -0.75, 0.62)
    shots = []
    for key, cam, aim, lens, objs in (
            ('front', (0.0, -7.0, 1.7), target, 62.0, shown),
            ('three_quarter', (-4.6, -5.4, 2.0), target, 62.0, shown + [scale]),
            ('side', (-7.0, 0.0, 1.7), target, 62.0, shown),
            ('back', (2.0, 6.8, 2.2), target, 62.0, shown),
            ('close', (0.9, -2.4, 2.35), (0.0, -0.15, 1.75), 45.0, shown),
            ('distance', (-6.0, -16.0, 1.7), (0.0, 0.0, 1.3), 50.0, shown + [scale])):
        shots.append(lb.render(objs, os.path.join(OUT, f'StoneSentinel_{key}.png'), cam, aim, lens=lens, size=(900, 1000),
                               ground=0x8f8a72, sun_dir=sun))
    lb.sheet(shots, os.path.join(OUT, 'StoneSentinel_sheet.png'), cols=3)
    RIG.location = (0.0, 0.0, 0.0)


if lt.want_preview() or '--preview' in ARGV:
    if TRIPO:
        SENTINEL['previews'](RIG, BODY, PLINTH, OUT, PLINTH_TOP / 100.0)
    else:
        previews()
