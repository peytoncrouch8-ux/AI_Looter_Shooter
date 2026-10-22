"""The Gravemother's Barrow fight, its shared art (Saved/Gravemother/CONTRACT.md, BARROW-ART, 2026-10-19). What the three
model scripts build from:

  Art/Models/Creatures/Gravemother.py       SK_Gravemother: Spider.py's mesh and rig dressed (hide, grave mud, root stubs)
  Art/Models/Creatures/GravemotherStone.py  the headstone in three states, its shards and chips (looter_barrow_stone.py)
  Art/Models/Props/GraveFight.py            the fight's earth: clods, rubble, berms, mounds, debris

Here: the spider's parts (Spider.py run above its rig, its painter switched off, as Spitter.py does), the abdomen's
surface, where the headstone sits on it and the roots that bind it, mesh helpers (tubes and lumps with tiling UVs), the
two shared tileable sets (GravemotherMud: the grave mud caked on her legs and round the stone's foot; GravemotherEarth:
the fresh grave earth of the clods, rubble, berms and mounds), the materials the scripts share, and the previews.

Spaces: creature cm (Spider.py's: x forward, y left, z up, the origin on the ground under the thorax, the spider at 1x;
the game draws her at 1.8x) and the stone's own frame, cm: x across the slab, +y out of its carved face (which faces her
head), z up along it from the foot, the origin at the middle of its foot (the pivot, where it meets the abdomen).
"""
import math
import os
import random
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector, noise

import looter_creatures as lc
import looter_textures as lt

REPO = lt.REPO
SPIDER_PY = os.path.join(REPO, 'Art', 'Models', 'Creatures', 'Spider.py')
RIG_MARK = '# --- The rig ---'
PREVIEW_DIR = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Creatures', 'Gravemother')
FAMILY_DIR = os.path.join(REPO, 'Intermediate', 'ArtExport_Gravemother')

# The contract's colour key (sRGB).
HIDE = 0x8c8070          # mud-grey hide
GRANITE = 0x65625d       # headstone granite (dark weathered since CHAR-ART, 2026-10-22; was #8E8B86)
BONE = 0xbab2a1          # the bare Bone abdomen
MUD = 0x7c6650           # grave earth caked on her legs (dry; pale #C9B392 until CHAR-ART, 2026-10-22)
ROOT_BROWN = 0x5b4330    # roots binding the stone on
EARTH = 0x6e5c49         # fresh grave earth (clods, rubble, berms, mounds)
EARTH_DUST = 0x806b55    # its dust
FLOOR_DUST = 0xdcc9a4    # the sun-baked floor's dust (footfalls)
SCALE = 1.8              # her size in the game; every creature-space number here is the spider's 1x


def log(*parts):
    print('BARROW:', *parts, flush=True)


def argv():
    """The script's arguments after '--' (models.ps1 passes --out <folder> there when it exports)."""
    return sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []


def export_dir():
    """The folder an export writes to (models.ps1's --out), or None outside an export."""
    args = argv()
    return args[args.index('--out') + 1] if '--out' in args and args.index('--out') + 1 < len(args) else None


# --- The spider ---

def spider_parts():
    """Runs Spider.py's parts code (above its rig) and returns its names: the parts, bristles and bones not yet skinned,
    ATLAS, paint, the hides. Its texture sets aren't painted again (lc.paint_atlas is off meanwhile), so this never
    touches Art/Textures/SpiderBody."""
    with open(SPIDER_PY, encoding='utf-8') as source:
        code = source.read()
    head, mark, _ = code.partition(RIG_MARK)
    if not mark:
        raise RuntimeError(f'{SPIDER_PY} has no "{RIG_MARK}" line: looter_barrow builds on the spider\'s parts above it')
    painter = lc.paint_atlas
    lc.paint_atlas = lambda *args, **kwargs: None
    names = {'__name__': 'spider_parts_for_gravemother', '__file__': SPIDER_PY}
    try:
        exec(compile(head, SPIDER_PY, 'exec'), names)
    finally:
        lc.paint_atlas = painter
    return names


class Abdomen:
    """The spider's abdomen as Spider.py builds it (an egg-shaped ellipsoid along fwd): points on it by (a, phi), a from
    -1 (its rear pole) to 1 (its front), phi degrees round from its top, + toward her left."""

    def __init__(self, S):
        spec = S['ABDOMEN']
        self.center = S['lifted'](spec['center'])
        self.radii = spec['radii']
        self.fwd = Vector(spec['fwd']).normalized()
        self.up = (Vector((0.0, 0.0, 1.0)) - self.fwd * self.fwd.z).normalized()
        self.left = self.up.cross(self.fwd)
        self.deform = spec['deform']

    def point(self, a, phi, out=1.0):
        r = math.sqrt(max(1.0 - a * a, 0.0))
        s, u = r * math.sin(math.radians(phi)), r * math.cos(math.radians(phi))
        da, ds, du = self.deform(a, s, u)
        rx, ry, rz = self.radii
        p = self.center + (self.fwd * da * rx + self.left * ds * ry + self.up * du * rz) * out
        n = (self.fwd * (a / rx) + self.left * (s / ry) + self.up * (u / rz)).normalized()
        return p, n

    def from_uv(self, U, V):
        """The surface points (creature cm, arrays) of the abdomen's atlas UVs (lc.ellipsoid's: U round, 0.5 its top;
        V along, 0 its rear pole): what a painter needs to paint by place."""
        theta, phi = np.pi * V, -np.pi + 2.0 * np.pi * U
        a, r = -np.cos(theta), np.sin(theta)
        s, u = r * np.sin(phi), r * np.cos(phi)
        front = 1.0 - 0.12 * np.maximum(a, 0.0)               # Spider.py's egg(0.12, 0.8)
        s2, u2 = s * front, u * front * np.where(u < 0.0, 0.8, 1.0)
        rx, ry, rz = self.radii
        out = [self.center[i] + self.fwd[i] * a * rx + self.left[i] * s2 * ry + self.up[i] * u2 * rz for i in range(3)]
        return np.stack(out, axis=-1).astype(np.float32)

    def coords(self, p):
        """(a, phi) of a point near the surface (the egg's deform ignored: close enough to route a root)."""
        rel = Vector(p) - self.center
        rx, ry, rz = self.radii
        x, y, z = rel.dot(self.fwd) / rx, rel.dot(self.left) / ry, rel.dot(self.up) / rz
        length = math.sqrt(x * x + y * y + z * z) or 1.0
        return max(-0.98, min(0.98, x / length)), math.degrees(math.atan2(y, z))


# --- The headstone's seat ---

STONE_W, STONE_H, STONE_T = 80.0, 116.0, 17.0      # cm at 1x: 1.44 x 2.09 m (31 cm thick) on her at 1.8x
STONE_SEAT = (-0.12, 0.0, 0.52)                    # (a, phi, out): its foot sunk deep into the abdomen's back
STONE_LEAN, STONE_ROLL, STONE_YAW = 26.0, 5.0, 4.0  # leaning back, tipped to her right, turned a little


def stone_frame(S):
    """The stone's frame in creature cm: a 4x4 from stone coordinates (see the module's docstring)."""
    base, _ = Abdomen(S).point(*STONE_SEAT)
    R = (Matrix.Rotation(math.radians(-90.0 + STONE_YAW), 3, 'Z') @ Matrix.Rotation(math.radians(STONE_LEAN), 3, 'X')
         @ Matrix.Rotation(math.radians(STONE_ROLL), 3, 'Y'))
    return Matrix.Translation(base) @ R.to_4x4()


def stone_outline(n=96, seed=3):
    """The slab's outline in its face plane (x, z), counter-clockwise from the foot's middle: straight sides, a round
    top, its edges worn a little uneven (small chips)."""
    rng = random.Random(seed)
    half, arch = STONE_W / 2.0, STONE_H - STONE_W / 2.0
    pts = []
    side_n, top_n, foot_n = 14, 40, 8
    for k in range(foot_n):                                    # the foot, middle to the right corner
        pts.append((half * k / foot_n, 0.0))
    for k in range(side_n):                                    # up the right side
        pts.append((half, arch * k / side_n))
    for k in range(top_n + 1):                                 # over the top
        a = math.pi * k / top_n
        pts.append((math.cos(a) * half, arch + math.sin(a) * half))
    for k in range(1, side_n + 1):                             # down the left side
        pts.append((-half, arch * (1.0 - k / side_n)))
    for k in range(1, foot_n):                                 # the foot, left corner to the middle
        pts.append((-half + half * k / foot_n, 0.0))
    out = []
    for x, z in pts:
        if z <= 0.01:
            out.append((x, z))
            continue
        # Worn: the edge pushed in a little (outward normal: sideways on the sides, from the arch's middle over the
        # top), more where weather took a chip; never out past the slab.
        if z <= arch:
            nx, nz = (1.0 if x > 0 else -1.0), 0.0
        else:
            nx, nz = x, z - arch
        length = math.hypot(nx, nz) or 1.0
        wear = 0.35 + 0.5 * noise.noise(Vector((x * 0.11, z * 0.11, seed))) ** 2
        if rng.random() < 0.06:
            wear += rng.uniform(0.8, 1.8)
        out.append((x - nx / length * wear, z - nz / length * wear))
    return out


# Roots binding the stone on: each runs over the stone (points in stone coordinates: x, the face it lies on (+1 the
# carved face, -1 the back, 0 over the edge at that point of the outline), z), then down onto her abdomen to where it
# enters her hide (a, phi). r0 is its radius at its stone end (cm), r1 where it enters her (the stub's).
ROOTS = [
    dict(name='FrontLeft', r0=1.5, r1=4.2, path=[(-14, -1, 84), (-24, -1, 98), (-26.5, 0, 106), (-22, 1, 99),
                                                  (-15, 1, 74), (-8, 1, 46), (-5, 1, 26)], anchor=(0.16, 24.0)),
    dict(name='FrontRight', r0=1.4, r1=3.8, path=[(16, -1, 80), (24, -1, 95), (25.0, 0, 106), (21, 1, 96),
                                                   (11, 1, 62), (6, 1, 40), (5, 1, 24)], anchor=(0.14, -22.0)),
    dict(name='BackLeft', r0=1.4, r1=3.6, path=[(-4, -1, 70), (-12, -1, 48), (-17, -1, 28)], anchor=(-0.46, 34.0)),
    dict(name='BackRight', r0=1.3, r1=3.4, path=[(30, -1, 62), (24, -1, 40), (20, -1, 26)], anchor=(-0.52, -30.0)),
    dict(name='SideLeft', r0=1.3, r1=3.4, path=[(-30, 1, 46), (-40.5, 0, 36), (-30, -1, 28)], anchor=(-0.22, 64.0)),
    dict(name='SideRight', r0=1.2, r1=3.1, path=[(33, 1, 38), (40.5, 0, 30), (34, -1, 24)], anchor=(-0.16, -66.0)),
]


def root_anchor(S, root, out=0.985):
    """Where a root enters her hide (creature cm) and the surface's normal there."""
    return Abdomen(S).point(root['anchor'][0], root['anchor'][1], out)


ROOT_DIVE_CM = 10.0          # a root's last stretch, diving into her hide (its stub is the part that stays)


def root_dive(S, root):
    """How a root goes into her hide: where (creature cm) and the way back out along it, a unit vector up off her hide
    and leaning toward the stone. The root dives along it; when the stone cracks off, its stub stands out along it."""
    p, n = root_anchor(S, root)
    t = stone_frame(S).translation - p
    t = t - n * t.dot(n)
    t = t.normalized() if t.length > 1e-6 else Vector((0.0, 0.0, 1.0))
    return p, (n * 0.55 + t * 0.85).normalized()


# --- Stone frame to the exported model and to Unreal ---

# The stone's model faces its carved face to Blender's -Y (a model's front, Unreal's +X): stone (x, y, z) -> (-x, -y, z).
STONE_TO_MODEL = Matrix.Rotation(math.pi, 4, 'Z')


def unreal_point(c):
    """A creature-space point (cm) in SK_Gravemother's component space (Unreal cm: x forward, y right, z up)."""
    return Vector((c[0], -c[1], c[2]))


def unreal_rotator(x_axis, y_axis, z_axis):
    """FRotator (pitch, yaw, roll in degrees) of a rotation given by where it sends X, Y and Z (Unreal's
    FMatrix::Rotator)."""
    pitch = math.degrees(math.atan2(x_axis.z, math.hypot(x_axis.x, x_axis.y)))
    yaw = math.degrees(math.atan2(x_axis.y, x_axis.x))
    sp, cp = math.sin(math.radians(pitch)), math.cos(math.radians(pitch))
    sy, cy = math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
    # FRotationMatrix(pitch, yaw, 0)'s Y axis: (-sy, cy, 0).
    sy_axis = Vector((-sy, cy, 0.0))
    roll = math.degrees(math.atan2(z_axis.dot(sy_axis), y_axis.dot(sy_axis)))
    return pitch, yaw, roll


def rotator_axes(pitch, yaw, roll):
    """FRotationMatrix's X, Y and Z axes (Unreal's convention), to check unreal_rotator against."""
    sp, cp = math.sin(math.radians(pitch)), math.cos(math.radians(pitch))
    sy, cy = math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
    sr, cr = math.sin(math.radians(roll)), math.cos(math.radians(roll))
    x = Vector((cp * cy, cp * sy, sp))
    y = Vector((sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp))
    z = Vector((-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp))
    return x, y, z


MIRROR_Y = Matrix(((1.0, 0.0, 0.0), (0.0, -1.0, 0.0), (0.0, 0.0, 1.0)))


def bone_rest_unreal(bone_matrix_local):
    """A bone's rest transform in SK_Gravemother's component space (a rotation 3x3, a location in Unreal cm) as the
    import makes it. bone_matrix_local: the Blender bone's matrix_local (armature space, before the export bakes it).
    looter_export.bake_for_unreal turns the armature a quarter about Z and scales it to cm, which puts its bones in
    creature cm (x forward, y left); the FBX keeps a bone's own matrix (Y along the bone: the default bone axes); Unreal's
    FBX import mirrors Y (right- to left-handed), a rotation becoming its mirror image M R M."""
    turn = Matrix.Rotation(math.pi / 2.0, 4, 'Z') @ Matrix.Scale(100.0, 4)
    baked = turn @ bone_matrix_local
    R_fbx = baked.to_3x3().normalized()
    return MIRROR_Y @ R_fbx @ MIRROR_Y, MIRROR_Y @ baked.translation


def stone_attach(S, abdomen_matrix_local):
    """The stone's place on SK_Gravemother at rest: in the mesh's component space (Unreal cm and degrees at the spider's
    1x; the game's 1.8 applies on top) and, what the game attaches with, in the abdomen bone's own space (its rest
    transform taken back off). The stone's own Unreal axes: X out of its carved face, Y across it, Z up along it; its
    pivot the middle of its foot."""
    F = stone_frame(S)
    R = F.to_3x3()
    # Unreal stone-local (X, Y, Z) is stone (y, x, z) (STONE_TO_MODEL, then the exporter's axes); creature to component
    # space mirrors y.
    axes = [unreal_point(R @ Vector(a)).normalized() for a in ((0.0, 1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0))]
    Rc = Matrix((axes[0], axes[1], axes[2])).transposed()          # columns: where X, Y and Z go
    location = unreal_point(F.translation)
    Rb, head = bone_rest_unreal(abdomen_matrix_local)
    head_c = next(Vector(h) for name, h, t, p in S['bones'] if name == 'abdomen')
    if (head - unreal_point(head_c)).length > 0.05:
        raise RuntimeError(f"the abdomen bone is at {head}, not at Spider.py's {unreal_point(head_c)}")
    Rx = Rb.transposed() @ Rc
    out = {}
    for key, cols, loc in (('component', Rc, location), ('bone', Rx, Rb.transposed() @ (location - head)),
                           ('bone_rest', Rb, head)):
        ax = [cols.col[i].normalized() for i in range(3)]
        rot = unreal_rotator(*ax)
        err = max((a - b).length for a, b in zip(rotator_axes(*rot), ax))
        if err > 1e-4 or abs(ax[0].cross(ax[1]).dot(ax[2]) - 1.0) > 1e-4:
            raise RuntimeError(f'stone_attach: the {key} rotation is no proper rotation (error {err})')
        out[key] = dict(location=loc, rotation=rot, axes=ax)
    return out


# --- Mesh helpers ---

def new_bm():
    bm = bmesh.new()
    bm.loops.layers.uv.new('UVMap')
    return bm


def obj_from_bm(name, bm, materials, smooth=True, link=True, sharp=None):
    """A mesh object from bm (its faces' material_index picks among materials). sharp: hard edges past that angle;
    smooth None keeps each face's own flag."""
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    for mat in materials:
        mesh.materials.append(mat)
    if smooth is not None:             # None: as the faces are flagged
        mesh.polygons.foreach_set("use_smooth", [smooth] * len(mesh.polygons))
    if sharp is not None:
        mesh.set_sharp_from_angle(angle=math.radians(sharp))
    obj = bpy.data.objects.new(name, mesh)
    if link:
        bpy.context.scene.collection.objects.link(obj)
    return obj


def fbm(p, octaves=3, gain=0.5):
    total, amp = 0.0, 1.0
    for _ in range(octaves):
        total += amp * noise.noise(p)
        p, amp = p * 2.03, amp * gain
    return total


def frames(pts):
    """Rotation-minimizing frames (tangent, normal, binormal) along a polyline."""
    n = len(pts)
    tangents = []
    for k in range(n):
        t = pts[min(k + 1, n - 1)] - pts[max(k - 1, 0)]
        tangents.append(t.normalized() if t.length > 1e-9 else Vector((0.0, 0.0, 1.0)))
    ref = Vector((0.0, 0.0, 1.0)) if abs(tangents[0].z) < 0.9 else Vector((1.0, 0.0, 0.0))
    normal = (ref - tangents[0] * ref.dot(tangents[0])).normalized()
    out = []
    for k in range(n):
        if k:
            normal = normal - tangents[k] * normal.dot(tangents[k])
            normal = normal.normalized() if normal.length > 1e-9 else out[-1][1]
        out.append((tangents[k], normal, tangents[k].cross(normal)))
    return out


def tube(bm, pts, radii, mat=0, sides=8, uv_per=0.01, wobble=0.0, seed=0, caps=(True, True), squash=None):
    """A closed tube along pts (Vectors) with a radius per point, UVs that tile: V along (its length times uv_per: a
    texture's grain running down V runs along it), U round its girth (its mean circumference times uv_per). wobble: noise
    pushing the surface in and out (a share of the radius); squash(k) -> (sx, sy) scales a ring across (its normal,
    binormal)."""
    uv = bm.loops.layers.uv.active
    pts = [Vector(p) for p in pts]
    fr = frames(pts)
    dist = [0.0]
    for a, b in zip(pts, pts[1:]):
        dist.append(dist[-1] + (b - a).length)
    girth = 2.0 * math.pi * (sum(radii) / len(radii)) * uv_per
    rings = []
    off = Vector((seed * 3.1, seed * 1.7, seed * 0.3))
    for k, p in enumerate(pts):
        t, nrm, bin_ = fr[k]
        sx, sy = squash(k) if squash else (1.0, 1.0)
        ring = []
        for j in range(sides):
            a = 2.0 * math.pi * j / sides
            d = nrm * math.cos(a) * sx + bin_ * math.sin(a) * sy
            r = radii[k]
            if wobble:
                r *= 1.0 + wobble * fbm(p * 0.15 + d * 0.8 + off, 2)
            ring.append(bm.verts.new(p + d * r))
        rings.append(ring)
    for k in range(len(pts) - 1):
        ua, ub = dist[k] * uv_per, dist[k + 1] * uv_per
        for j in range(sides):
            j1 = (j + 1) % sides
            f = bm.faces.new((rings[k][j], rings[k + 1][j], rings[k + 1][j1], rings[k][j1]))
            f.material_index = mat
            f.smooth = True
            va, vb = girth * j / sides, girth * (j + 1) / sides
            for loop, c in zip(f.loops, ((va, ua), (va, ub), (vb, ub), (vb, ua))):
                loop[uv].uv = c
    for k, end in ((0, caps[0]), (len(pts) - 1, caps[1])):
        if not end:
            continue
        tip = pts[k] + fr[k][0] * (radii[k] * (0.35 if k else -0.35))
        hub = bm.verts.new(tip)
        u = dist[k] * uv_per
        # The cap laid flat on the texture round its middle (no corner of it without UV area: Unreal's tangents).
        cap = {hub: (0.0, u)}
        for j, v in enumerate(rings[k]):
            a = 2.0 * math.pi * j / sides
            cap[v] = (math.cos(a) * radii[k] * uv_per, u + math.sin(a) * radii[k] * uv_per)
        for j in range(sides):
            j1 = (j + 1) % sides
            f = bm.faces.new((hub, rings[k][j1], rings[k][j]) if k == 0 else (hub, rings[k][j], rings[k][j1]))
            f.material_index = mat
            f.smooth = True
            for loop in f.loops:
                loop[uv].uv = cap[loop.vert]
    return rings


def blob(bm, center, radii, R=None, seed=0, lump=0.3, subdiv=2, freq=1.6, mat=0, flat_below=None):
    """A lumpy ellipsoid (noise-displaced icosphere). flat_below: a z under which it's squashed flat (resting on it)."""
    made = bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=1.0)
    off = Vector((seed * 1.7, seed * 2.3, seed * 0.9))
    R = R or Matrix.Identity(3)
    center = Vector(center)
    for v in made['verts']:
        d = v.co.copy()
        k = 1.0 + lump * fbm(d * freq + off, 2)
        p = center + R @ Vector((d.x * radii[0], d.y * radii[1], d.z * radii[2])) * k
        if flat_below is not None and p.z < flat_below:
            p.z = flat_below + (p.z - flat_below) * 0.08
        v.co = p
    faces = {f for v in made['verts'] for f in v.link_faces}
    for f in faces:
        f.material_index = mat
        f.smooth = True
    return made['verts']


def box_uv_faces(bm, faces, per, offset=(0.0, 0.0)):
    """World-scale cube projection (U, V = position times per) on these faces of bm, in its own coordinates."""
    uv = bm.loops.layers.uv.active
    x, y, z = Vector((1.0, 0.0, 0.0)), Vector((0.0, 1.0, 0.0)), Vector((0.0, 0.0, 1.0))
    for face in faces:
        n = face.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        if axis == 2:
            ua, va = x, (y if n.z > 0.0 else -y)
        elif axis == 0:
            ua, va = (y if n.x > 0.0 else -y), z
        else:
            ua, va = (-x if n.y > 0.0 else x), z
        for loop in face.loops:
            co = loop.vert.co
            loop[uv].uv = (co.dot(ua) * per + offset[0], co.dot(va) * per + offset[1])


# --- The shared tileable sets ---

MUD_SET, EARTH_SET = 'GravemotherMud', 'GravemotherEarth'
SET_PX_M = 1.0 / 512.0           # both: 1024 px, 2 m a repeat


def paint_mud():
    """GravemotherMud: grave earth caked on her legs in a crust, broken into curling plates by cracks, darker and
    smoother where it's still damp, grit and root hairs in it. 2 m a repeat. Her own look (CHAR-ART item 4b, the user's
    OK 2026-10-22: "the headstone, earth and pale hide"): dark wet grave earth (about #7C6650 dry, #4A3B2E damp), so her
    legs go dark toward the ground under the pale hide; it was a pale dry crust (#C9B392) that read as more of the
    grey-tan spider."""
    shape = (1024, 1024)
    px_m = SET_PX_M
    x, y = lt.coords(shape)
    warp = 9.0
    wx, wy = x + warp * lt.noise(shape, 301, 14.0, octaves=2), y + warp * lt.noise(shape, 302, 14.0, octaves=2)
    plates = lt.worley(shape, (22, 22), 303, x=wx, y=wy)
    fine = lt.worley(shape, (70, 70), 304, x=wx, y=wy)
    edge = plates['edge']
    crack = 1.0 - lt.smooth(0.6, 2.4, edge)
    fine_crack = (1.0 - lt.smooth(0.3, 1.2, fine['edge'])) * lt.smooth(-0.3, 0.8, lt.noise(shape, 305, 40.0))
    damp = lt.smooth(0.2, 1.4, lt.noise(shape, 306, 90.0, octaves=2))
    lift = lt.per_cell(plates['id'], plates['count'], 307, 0.0, 1.0)
    curl = lt.smooth(2.0, 10.0, edge)                         # a plate rises toward its middle, its rims curl up a hair
    rim = lt.smooth(6.0, 1.5, edge) * (1.0 - crack)
    grain = lt.noise(shape, 308, 0.9)
    lumps = lt.noise(shape, 309, 6.0, octaves=3)
    base = lt.mix(lt.rgb(0x86705a), lt.rgb(0x6c5845), lt.smooth(-1.0, 1.2, lt.noise(shape, 310, 60.0, octaves=2)))
    base = lt.mix(base, lt.rgb(0x9a8467), lt.smooth(0.3, 1.0, lift) * 0.35)
    color = lt.mix(base, lt.rgb(0x4a3b2e), damp * 0.6)
    broken = lt.smooth(-0.4, 0.5, lt.noise(shape, 314, 26.0))         # the cracks come and go
    color = lt.mix(color, lt.rgb(0x33281f), np.clip(crack * broken + fine_crack * 0.3, 0.0, 1.0) * (0.5 - 0.2 * damp))
    color = lt.mix(color, lt.rgb(0xa8937a), rim * 0.25 * (1.0 - damp))
    grit = lt.specks(shape, 311, 0.035, 0.9)
    color = lt.mix(color, lt.rgb(0x6f675b), grit * 0.5)
    color = lt.mix(color, lt.rgb(0x9a958a), lt.specks(shape, 312, 0.01, 1.6) * 0.6)
    color = color * (1.0 + 0.06 * grain + 0.05 * lumps)[..., None]
    # Root hairs dried into it: thin dark threads.
    hairs = lt.crack_lines(shape, 313, 60.0, 50.0, 0.7, 0.18) * (1.0 - damp)
    color = lt.mix(color, lt.rgb(0x4a3a2c), hairs * 0.6)
    height = 0.0025 * curl * (0.6 + 0.4 * lift) + 0.0008 * rim + 0.0012 * lumps + 0.0002 * grain
    height = height * (1.0 - 0.6 * damp) - 0.0022 * crack * broken - 0.0005 * fine_crack + 0.0003 * hairs
    ao = lt.occlusion(height, px_m, (2, 6, 16), 1.2)
    color = color * (0.7 + 0.3 * ao)[..., None]
    rough = 0.93 - 0.2 * damp + 0.03 * grain
    lt.save_set(MUD_SET, color, height, px_m, rough, ao=ao, normal_strength=1.0)


def paint_earth():
    """GravemotherEarth: freshly dug grave earth (#6E5C49), darker than the sun-baked floor so it reads on it and on the
    Sink's wall: crumbly clods lit from above, small stones, broken root threads, damp darker hollows. 2 m a repeat."""
    shape = (1024, 1024)
    px_m = SET_PX_M
    rng = np.random.default_rng(401)
    x, y = lt.coords(shape)
    wx, wy = x + 6.0 * lt.noise(shape, 402, 9.0, octaves=2), y + 6.0 * lt.noise(shape, 403, 9.0, octaves=2)
    clods = lt.worley(shape, (34, 34), 404, x=wx, y=wy)
    small = lt.worley(shape, (190, 190), 405, x=wx, y=wy)
    clod_r = lt.per_cell(clods['id'], clods['count'], 406, 0.6, 1.0)
    raised = (lt.per_cell(clods["id"], clods["count"], 413) < 0.6).astype(np.float32)
    dome = lt.smooth(0.0, 9.0, clods["edge"]) * clod_r * raised
    crumbs = lt.smooth(0.0, 2.2, small["edge"]) * lt.per_cell(small["id"], small["count"], 407, 0.2, 1.0)
    broad = lt.noise(shape, 408, 90.0, octaves=1)
    grain = lt.noise(shape, 409, 0.8)
    damp = lt.smooth(0.0, 1.3, lt.noise(shape, 410, 55.0, octaves=2))
    height = 0.005 * dome + 0.0022 * crumbs + 0.002 * broad + 0.0003 * grain
    base = lt.mix(lt.rgb(0x76644f), lt.rgb(0x5f4f3f), lt.smooth(-1.2, 1.2, lt.noise(shape, 411, 45.0, octaves=2)))
    base = lt.mix(base, lt.rgb(0x4d3e30), damp * 0.5)
    tone = lt.per_cell(clods['id'], clods['count'], 412, 0.88, 1.1)
    color = base * tone[..., None] * (1.0 + 0.08 * grain)[..., None]
    gy, gx = np.gradient(lt.blur(height, 2.0))
    lit = np.clip(1.0 + (-gy * 0.9 + -gx * 0.45) * 240.0, 0.8, 1.2)     # crumbs catch the light on their upper sides
    color = color * lit[..., None]
    canvas = lt.Canvas(shape, 0.0, color=color, kind=np.zeros(shape))
    canvas.top[:] = height.ravel()
    stones = [0x77726a, 0x6c675f, 0x837b6e, 0x5d5953, 0x8a8273, 0x66605a]
    lt.pebbles(canvas, rng, 500, 1.5, 3.2, stones, px_m, lift=0.004)
    lt.pebbles(canvas, rng, 90, 4.0, 9.0, stones, px_m, lift=0.003)
    # Broken root threads lying in it.
    count = 260
    x0 = rng.uniform(0, 1024, count).astype(np.float32)
    y0 = rng.uniform(0, 1024, count).astype(np.float32)
    sx, sy, t, i, across = lt.stroke_points(x0, y0, rng.uniform(0, 2 * np.pi, count).astype(np.float32),
                                            rng.uniform(12, 40, count).astype(np.float32),
                                            rng.uniform(1.2, 2.6, count).astype(np.float32),
                                            rng.normal(0.0, 0.25, count).astype(np.float32), taper=0.8)
    canvas.paint(sx, sy, 0.007 + 0.0006 * (1.0 - across ** 2),
                 color=np.broadcast_to(lt.rgb(0x5a4330), (len(t), 3)) * (0.8 + 0.3 * (1.0 - across ** 2))[:, None],
                 kind=np.full(len(t), 2.0))
    height = canvas.height()
    color = canvas.get('color')
    ao = lt.occlusion(height, px_m, (2, 6, 16), 1.3)
    color = color * (0.62 + 0.38 * ao)[..., None]
    kind = canvas.get('kind')
    rough = np.where(kind == 1.0, 0.72, 0.94) - 0.16 * damp
    lt.save_set(EARTH_SET, color, height, px_m, rough, ao=ao, normal_strength=0.8)


ROOT_SET = 'GravemotherRoots'
ROOT_REPEAT_M = 0.6              # 512 px, 0.6 m a repeat
ROOT_UV_PER_CM = SCALE * 0.01 / ROOT_REPEAT_M      # on her and her stone (creature cm, she is 1.8x)


def paint_roots():
    """GravemotherRoots: the bark of the roots that bind her stone (#5B4330): fine wrinkles running along a root (down
    V), a few rings across it, scuffs where the bark is worn through to paler wood, grave earth in the cracks, root hairs.
    Tiles in 0.6 m."""
    shape = (512, 512)
    px_m = ROOT_REPEAT_M / 512.0
    x, y = lt.coords(shape)
    wrinkle = lt.noise(shape, 501, 1.2, 14.0, octaves=3)                  # long along V, thin across
    furrow = lt.smooth(-0.2, -1.2, lt.noise(shape, 502, 2.5, 40.0, octaves=2))
    rings = lt.smooth(0.6, 0.0, np.abs(np.sin(y / 512.0 * 2.0 * math.pi * 3.0 + 0.8 * lt.noise(shape, 503, 30.0))))
    rings *= lt.smooth(0.2, 1.0, lt.noise(shape, 504, 20.0, 4.0))
    scuff = lt.smooth(0.9, 1.6, lt.noise(shape, 505, 10.0, 24.0, octaves=2))
    base = lt.mix(lt.rgb(0x5f4632), lt.rgb(0x4c3626), lt.smooth(-1.0, 1.0, lt.noise(shape, 506, 40.0, octaves=2)))
    color = base * (1.0 + 0.12 * wrinkle)[..., None]
    color = lt.mix(color, lt.rgb(0x2e2118), furrow * 0.7)
    color = lt.mix(color, lt.rgb(0x3a2b20), rings * 0.5)
    color = lt.mix(color, lt.rgb(0x8c7052), scuff * 0.55)
    soil = furrow * lt.smooth(0.0, 1.0, lt.noise(shape, 507, 12.0))
    color = lt.mix(color, lt.rgb(0x6e5c49), soil * 0.45)
    hairs = lt.crack_lines(shape, 508, 30.0, 26.0, 0.6, 0.2)
    color = lt.mix(color, lt.rgb(0x2a1f17), hairs * 0.5)
    height = 0.0012 * wrinkle - 0.0018 * furrow - 0.0008 * rings + 0.0004 * scuff
    ao = lt.occlusion(height, px_m, (2, 5, 12), 1.0)
    color = color * (0.75 + 0.25 * ao)[..., None]
    rough = 0.82 + 0.08 * furrow - 0.1 * scuff
    lt.save_set(ROOT_SET, color, height, px_m, rough, ao=ao, normal_strength=1.0)


def ensure_set(name, painter, always=False):
    """Paints a set when its maps are missing (or always): the scripts that own a set paint it every run."""
    if always or not all(os.path.exists(lt.texture_path(name, k)) for k in ('BC', 'N', 'ORM')):
        painter()


# --- Materials (each name is its MI_<name> in /Game/Art/Materials) ---

def mud():
    """Grave mud: GravemotherMud, as painted."""
    return lt.material(MUD_SET, name='GravemotherMud', DiffuseAO=0.7)


def earth():
    return lt.material(EARTH_SET, name='GraveEarth', DiffuseAO=0.75)


def roots():
    """The roots that bind the stone (and their bits in the earth): GravemotherRoots, painted when it's missing."""
    ensure_set(ROOT_SET, paint_roots)
    return lt.material(ROOT_SET, name='GraveRoots', DiffuseAO=0.6)


def bone():
    """Bone (a clod's bone, debris): the den's dry bone, exactly as Art/Models/Props/DenDressing.py makes MI_BoneDry."""
    return lt.material('Polymer', name='BoneDry', tint=0xe6d9bf, RoughnessOffset=0.3, DiffuseAO=0.85)


def fieldstone():
    """Field stones in the earth: the granite set, a little paler, no moss (just dug up)."""
    return lt.material('RockGranite', name='GraveFieldstone', tint=0xe6e2da, MossAmount=0.0)


def coffin_wood():
    """Old coffin wood (a clod's plank, splinters): the planks set, greyed and darkened."""
    return lt.material('WoodPlanks', name='GraveCoffinWood', tint=0x9c8f80)


def stone_break():
    """Granite broken fresh (the headstone's fracture faces): the granite set, paler and cleaner than its weathered
    face."""
    return lt.material('RockGranite', name='GravemotherStoneBreak', tint=0xf2efea, MossAmount=0.0)


def tint_for(set_name, target):
    """The Tint (sRGB hex) that takes a set's mean color (its BC map's) to target: a ratio per channel in linear,
    at most 1."""
    srgb_px = read_png(lt.texture_path(set_name, 'BC')).reshape(-1, 3)
    linear = np.where(srgb_px <= 0.04045, srgb_px / 12.92, ((srgb_px + 0.055) / 1.055) ** 2.4)
    mean = linear.mean(axis=0)
    want = np.array(lt.hex_color(target)[:3], np.float32)
    ratio = np.clip(want / np.maximum(mean, 1e-4), 0.0, 1.0)

    def srgb(c):
        return 12.92 * c if c <= 0.0031308 else 1.055 * c ** (1.0 / 2.4) - 0.055
    return int(round(srgb(ratio[0]) * 255)) << 16 | int(round(srgb(ratio[1]) * 255)) << 8 | int(round(srgb(ratio[2]) * 255))


# --- Previews ---

def _world(sky=0x9bb4cf, horizon=0xcfc6b4, strength=0.9):
    world = bpy.data.worlds.new('_BarrowWorld')
    world.use_nodes = True
    nt = world.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputWorld')
    bg = nt.nodes.new('ShaderNodeBackground')
    bg.inputs['Strength'].default_value = strength
    coords = nt.nodes.new('ShaderNodeTexCoord')
    split = nt.nodes.new('ShaderNodeSeparateXYZ')
    ramp = nt.nodes.new('ShaderNodeValToRGB')
    nt.links.new(coords.outputs['Generated'], split.inputs['Vector'])
    nt.links.new(split.outputs['Z'], ramp.inputs['Fac'])
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.45, lt.hex_color(0x6f6658)
    els[1].position, els[1].color = 0.9, lt.hex_color(sky)
    mid = els.new(0.52)
    mid.color = lt.hex_color(horizon)
    nt.links.new(ramp.outputs['Color'], bg.inputs['Color'])
    nt.links.new(bg.outputs['Background'], out.inputs['Surface'])
    return world


def ground_material(color=0xb59a76):
    """The Sink's sun-baked floor, near enough: what the fight's earth must read against."""
    name = f'_BarrowGround_{color:06x}'
    mat = bpy.data.materials.get(name)
    if mat:
        return mat
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
    tex = nt.nodes.new('ShaderNodeTexNoise')
    tex.inputs['Scale'].default_value = 1.2
    tex.inputs['Detail'].default_value = 6.0
    ramp = nt.nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].color = [c * 0.86 for c in lt.hex_color(color)[:3]] + [1.0]
    ramp.color_ramp.elements[1].color = [min(1.0, c * 1.1) for c in lt.hex_color(color)[:3]] + [1.0]
    nt.links.new(tex.outputs['Fac'], ramp.inputs['Fac'])
    nt.links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = 0.95
    return mat


def render(objects, out_png, cam_loc, target, lens=45.0, size=(1280, 800), ground=0xb59a76, sun_dir=(-0.5, -0.6, 0.62),
           sun_strength=4.2, samples=32, ortho=None, black=False, ground_z=0.0):
    """Renders objects (and nothing else) from cam_loc toward target (metres): Eevee, a warm sun and a sky, a floor
    of the Sink's colour (ground None: none; black: a black backdrop for FX pieces)."""
    scene = bpy.context.scene
    if bpy.context.object is not None and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    shown = set(objects)
    hidden = {o: o.hide_render for o in scene.objects}
    for o in scene.objects:
        o.hide_render = o not in shown
    added = []
    cam = bpy.data.objects.new('_BarrowCam', bpy.data.cameras.new('_BarrowCam'))
    cam.data.lens = lens
    cam.data.sensor_fit = 'HORIZONTAL'
    if ortho:
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = ortho
    cam.location = Vector(cam_loc)
    cam.rotation_euler = (Vector(target) - Vector(cam_loc)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.clip_start, cam.data.clip_end = 0.02, 500.0
    scene.collection.objects.link(cam)
    added.append(cam)
    sun = bpy.data.objects.new('_BarrowSun', bpy.data.lights.new('_BarrowSun', 'SUN'))
    sun.data.energy = sun_strength
    sun.data.color = (1.0, 0.92, 0.8)
    sun.data.angle = math.radians(1.5)
    sun.rotation_euler = (-Vector(sun_dir).normalized()).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    added.append(sun)
    if ground is not None and not black:
        me = bpy.data.meshes.new('_BarrowFloor')
        s = 200.0
        me.from_pydata([(-s, -s, ground_z), (s, -s, ground_z), (s, s, ground_z), (-s, s, ground_z)], [], [(0, 1, 2, 3)])
        me.materials.append(ground_material(ground))
        floor = bpy.data.objects.new('_BarrowFloor', me)
        scene.collection.objects.link(floor)
        added.append(floor)
    saved = dict(camera=scene.camera, world=scene.world, engine=scene.render.engine, x=scene.render.resolution_x,
                 y=scene.render.resolution_y, path=scene.render.filepath, transparent=scene.render.film_transparent)
    world = _world(strength=0.0 if black else 0.9)
    try:
        scene.camera = cam
        scene.world = world
        scene.render.resolution_x, scene.render.resolution_y = size
        scene.render.resolution_percentage = 100
        scene.render.film_transparent = False
        scene.render.image_settings.file_format = 'PNG'
        scene.render.image_settings.color_mode = 'RGB'
        scene.render.filepath = out_png
        scene.view_settings.view_transform = 'AgX'
        scene.view_settings.look = 'AgX - Medium High Contrast'
        scene.render.engine = 'BLENDER_EEVEE_NEXT'
        scene.eevee.taa_render_samples = samples
        os.makedirs(os.path.dirname(out_png), exist_ok=True)
        bpy.ops.render.render(write_still=True)
    finally:
        scene.camera = saved['camera']
        scene.world = saved['world']
        scene.render.engine = saved['engine']
        scene.render.resolution_x, scene.render.resolution_y = saved['x'], saved['y']
        scene.render.filepath = saved['path']
        scene.render.film_transparent = saved['transparent']
        bpy.data.worlds.remove(world)
        for o in added:
            data = o.data
            bpy.data.objects.remove(o)
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
            elif isinstance(data, bpy.types.Camera):
                bpy.data.cameras.remove(data)
            elif isinstance(data, bpy.types.Light):
                bpy.data.lights.remove(data)
        for o, value in hidden.items():
            if o.name in scene.objects:
                o.hide_render = value
    log(f'preview {out_png}')
    return out_png


def read_png(path):
    """A PNG's stored values as float RGB 0..1 (rows from the top; sRGB as written, no colour conversion), through
    Blender's image loader."""
    image = bpy.data.images.load(path, check_existing=False)
    image.colorspace_settings.name = 'Non-Color'
    w, h = image.size
    px = np.empty(w * h * 4, np.float32)
    image.pixels.foreach_get(px)
    bpy.data.images.remove(image)
    return px.reshape(h, w, 4)[::-1, :, :3]


def sheet(paths, out_png, cols=2, gap=8):
    """Images side by side (cols across), on dark grey, as one PNG."""
    tiles = [read_png(p) for p in paths]
    th = max(t.shape[0] for t in tiles)
    tw = max(t.shape[1] for t in tiles)
    rows = (len(tiles) + cols - 1) // cols
    out = np.full((rows * th + (rows + 1) * gap, cols * tw + (cols + 1) * gap, 3), 0.08, np.float32)
    for i, t in enumerate(tiles):
        r, c = divmod(i, cols)
        y0, x0 = gap + r * (th + gap), gap + c * (tw + gap)
        out[y0:y0 + t.shape[0], x0:x0 + t.shape[1]] = t
    # The renders were read as stored (display-referred sRGB bytes), so they're written back as they are.
    lt.write_png(out_png, lt.to8(np.clip(out, 0.0, 1.0)))
    log(f'sheet {out_png}')
    return out_png
