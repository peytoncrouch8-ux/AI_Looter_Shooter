"""Helpers for the playable heroes (Art/Backlog/Characters/): a skeleton with the UE5 mannequin's bone names, hierarchy
and A-pose, sized for any build, the shapes the heroes are modeled from, and skinning by distance to the bones.

Why the mannequin's names: the game animates the player with the mannequin's animations (/Game/Characters/Mannequins).
A hero rigged on bones with the same names and pose can be driven by them through Unreal's IK retargeter, which builds
its rig for a skeleton named like the mannequin by itself, whatever the hero's proportions.

Units are meters. A hero stands on the origin facing -Y (Blender's Front view), as Art/README.md asks; its left side
is +X. The armature becomes the root bone in Unreal (looter_export.export_rig), so the first bone is pelvis, as in the
mannequin. A hero is one skinned mesh, Hero_<Name> (SK_Hero_<Name> in Unreal), with one material per palette color
(Hero_<Name>_<color>), and a 'Flat' color attribute in the Screen Print Wash palette for the concept viewer.

    hero = Hero('Ellis', Build(height=1.80), palette)
    hero.add('Shirt', loft(rings), 'shirt', bones=TORSO)
    ...
    hero.finish()
"""
import colorsys
import math

import bmesh
import bpy
import numpy as np
from mathutils import Vector

import looter_model as lm

CREAM = (0xf6 / 255, 0xee / 255, 0xdc / 255)
FINGERS = ('index', 'middle', 'ring', 'pinky')
SIDES = (('l', 1.0), ('r', -1.0))      # the hero's left is +X


def v3(*a):
    return np.array(a[0] if len(a) == 1 else a, dtype=np.float64)


def unit(v):
    v = np.asarray(v, dtype=np.float64)
    n = np.linalg.norm(v)
    return v / n if n > 1e-12 else v


# ---------------------------------------------------------------------------------------------------------- the build

class Build:
    """A body's proportions. height is the top of the head (m). The scales stretch one part against the others and
    the whole is fitted back to height; shoulders and hips are the distances between the joints (m)."""

    def __init__(self, height=1.80, shoulders=0.36, hips=0.19, leg=1.0, torso=1.0, neck=1.0, head=1.0, arm=1.0,
                 hand=1.0, foot=1.0, arm_angle=45.0, stoop=0.0):
        self.height, self.shoulders, self.hips = height, shoulders, hips
        self.leg, self.torso, self.neck, self.head, self.arm = leg, torso, neck, head, arm
        self.hand, self.foot, self.arm_angle, self.stoop = hand, foot, arm_angle, stoop


class Skeleton:
    """The mannequin's bones for a build: name -> (head, tail, parent), numpy points in meters. Also keeps the
    landmarks the modeling works from (ground, hip, neck base, head size, palm frames)."""

    def __init__(self, b):
        self.b = b
        k = b.height / 1.80
        A, L, T, N, H = 0.085, 0.835 * b.leg, 0.55 * b.torso, 0.10 * b.neck, 0.23 * b.head
        f = b.height / (A + L + T + N + H)
        A, L, T, N, H = A * f, L * f, T * f, N * f, H * f
        self.k, self.T, self.N, self.Hd = k, T, N, H
        self.ankle_z, self.knee_z, self.hip_z = A, A + 0.49 * L, A + L
        self.neck_z = self.hip_z + T
        bones = {}

        def spine_pt(t):
            # A gentle S: the lower back curves in, the chest back; stoop bends the upper spine forward (-Y).
            y = 0.012 * k * math.sin(math.pi * t) - b.stoop * t * t
            return v3(0.0, y, self.hip_z + T * t)

        cuts = (0.07, 0.22, 0.40, 0.58, 0.76, 1.0)
        bones['pelvis'] = (v3(0.0, 0.004 * k, self.hip_z + 0.012 * k), spine_pt(cuts[0]), None)
        parent = 'pelvis'
        for i in range(5):
            name = f'spine_0{i + 1}'
            bones[name] = (spine_pt(cuts[i]), spine_pt(cuts[i + 1]), parent)
            parent = name
        base = spine_pt(1.0)
        lean = v3(0.0, -0.012 * k - b.stoop * 0.6, 0.0)
        neck2 = base + v3(0.0, 0.0, N * 0.5) + lean * 0.5
        skull = base + v3(0.0, 0.0, N) + lean
        bones['neck_01'] = (base, neck2, 'spine_05')
        bones['neck_02'] = (neck2, skull, 'neck_01')
        bones['head'] = (skull, skull + v3(0.0, -0.01 * k, H), 'neck_02')
        self.skull, self.head_top = skull, skull + v3(0.0, -0.01 * k, H)
        # The head's middle, and its size: the cranium sits a little behind and above the skull base.
        self.head_center = skull + v3(0.0, -0.012 * k, H * 0.42)

        clav_z = self.neck_z - 0.075 * T
        theta = math.radians(b.arm_angle)
        self.palm = {}
        for side, sx in SIDES:
            c0 = v3(sx * 0.022 * k, base[1] - 0.035 * k, clav_z + 0.012 * k)
            shoulder = v3(sx * b.shoulders / 2.0, base[1] + 0.006 * k, clav_z - 0.004 * k)
            bones[f'clavicle_{side}'] = (c0, shoulder, 'spine_05')
            d = unit(v3(sx * math.cos(theta), -0.06, -math.sin(theta)))
            elbow = shoulder + d * 0.163 * b.height * b.arm
            d2 = unit(d + v3(0.0, -0.14, 0.0))
            wrist = elbow + d2 * 0.145 * b.height * b.arm
            q = b.height / 1.80 * b.hand
            knuckle = wrist + d2 * 0.085 * q
            bones[f'upperarm_{side}'] = (shoulder, elbow, f'clavicle_{side}')
            bones[f'lowerarm_{side}'] = (elbow, wrist, f'upperarm_{side}')
            bones[f'hand_{side}'] = (wrist, knuckle, f'lowerarm_{side}')
            # The palm's frame: a along the fingers, n out of the palm (down in the A-pose), s toward the thumb.
            a = d2
            n = unit(v3(0.0, 0.0, -1.0) - a * a[2])
            front = v3(0.0, -1.0, 0.0)
            s = unit(front - a * a.dot(front) - n * n.dot(front))
            self.palm[side] = dict(a=a, n=n, s=s, wrist=wrist, knuckle=knuckle, q=q)
            offs = {'index': 0.022, 'middle': 0.0065, 'ring': -0.009, 'pinky': -0.023}
            reach = {'index': 0.98, 'middle': 1.0, 'ring': 0.97, 'pinky': 0.9}
            fan = {'index': 0.06, 'middle': 0.0, 'ring': -0.05, 'pinky': -0.11}
            lengths = {'index': (0.040, 0.025, 0.021), 'middle': (0.044, 0.028, 0.022),
                       'ring': (0.041, 0.026, 0.021), 'pinky': (0.033, 0.021, 0.018)}
            for finger in FINGERS:
                mc0 = wrist + a * 0.012 * q + s * offs[finger] * 0.6 * q + n * 0.004 * q
                k1 = wrist + a * 0.085 * q * reach[finger] + s * offs[finger] * q
                bones[f'{finger}_metacarpal_{side}'] = (mc0, k1, f'hand_{side}')
                p, prev = k1, f'{finger}_metacarpal_{side}'
                for i, length in enumerate(lengths[finger]):
                    dirn = unit(a + s * fan[finger] + n * 0.07 * (i + 1))
                    p2 = p + dirn * length * q
                    name = f'{finger}_0{i + 1}_{side}'
                    bones[name] = (p, p2, prev)
                    p, prev = p2, name
            t0 = wrist + a * 0.018 * q + s * 0.016 * q + n * 0.010 * q
            dirs = [unit(a * 0.55 + s * 0.65 + n * 0.35)]
            dirs.append(unit(dirs[0] + a * 0.25))
            dirs.append(unit(dirs[1] + a * 0.2))
            p, prev = t0, f'hand_{side}'
            for i, length in enumerate((0.038, 0.030, 0.025)):
                p2 = p + dirs[i] * length * q
                name = f'thumb_0{i + 1}_{side}'
                bones[name] = (p, p2, prev)
                p, prev = p2, name

            hx = sx * b.hips / 2.0
            hip = v3(hx, 0.005 * k, self.hip_z)
            knee = v3(hx + sx * 0.008 * k, -0.012 * k, self.knee_z)
            ankle = v3(hx + sx * 0.014 * k, 0.006 * k, self.ankle_z)
            ball = v3(hx + sx * 0.020 * k, -0.118 * k * b.foot, 0.028 * k)
            toe = v3(hx + sx * 0.024 * k, -0.19 * k * b.foot, 0.022 * k)
            bones[f'thigh_{side}'] = (hip, knee, 'pelvis')
            bones[f'calf_{side}'] = (knee, ankle, f'thigh_{side}')
            bones[f'foot_{side}'] = (ankle, ball, f'calf_{side}')
            bones[f'ball_{side}'] = (ball, toe, f'foot_{side}')
        self.bones = bones

    def head(self, name):
        return self.bones[name][0]

    def tail(self, name):
        return self.bones[name][1]

    def joint(self, name, t):
        """A point t of the way along a bone (0 its head, 1 its tail)."""
        h, tl, _ = self.bones[name]
        return h + (tl - h) * t


# ------------------------------------------------------------------------------------------------------------ shapes
# Every shape is a Shape: vertices (numpy N x 3) and faces (lists of indices), built around world points so parts line
# up with the bones. Rings go counterclockwise seen along their axis, so lofts face outward.

class Shape:
    def __init__(self, verts, faces, smooth=True):
        self.verts = np.asarray(verts, dtype=np.float64)
        self.faces = faces
        self.smooth = smooth

    def moved(self, fn):
        """A copy with every vertex passed through fn (N x 3 -> N x 3)."""
        return Shape(fn(self.verts.copy()), self.faces, self.smooth)


def frame_for(axis, hint=(0.0, -1.0, 0.0)):
    """Two unit vectors across axis: r (from hint, made square to axis) and f = axis x r."""
    axis = unit(axis)
    hint = v3(hint)
    if abs(axis.dot(unit(hint))) > 0.95:
        hint = v3(1.0, 0.0, 0.0) if abs(axis[0]) < 0.9 else v3(0.0, 0.0, 1.0)
    r = unit(hint - axis * axis.dot(hint))
    return r, np.cross(axis, r)


def ring(center, r_axis, f_axis, rx, ry, n=24, power=2.0, shape=None, start=0.0):
    """n points round center: rx along r_axis, ry along f_axis, a superellipse of this power (2 = ellipse, higher is
    squarer). shape(u, x, y) may reshape the local coordinates (u is the angle from r_axis toward f_axis)."""
    u = np.linspace(0.0, 2.0 * math.pi, n, endpoint=False) + start
    cu, su = np.cos(u), np.sin(u)
    e = 2.0 / power
    x = rx * np.sign(cu) * np.abs(cu) ** e
    y = ry * np.sign(su) * np.abs(su) ** e
    if shape is not None:
        x, y = shape(u, x, y)
    return v3(center)[None, :] + x[:, None] * v3(r_axis)[None, :] + y[:, None] * v3(f_axis)[None, :]


def loft(rings, cap_start=True, cap_end=True, smooth=True, closed=False):
    """Joins rings (each n x 3, all the same n) into a tube, capped with a fan at either end. closed joins the last ring
    back to the first (a torus)."""
    m, n = len(rings), len(rings[0])
    verts = np.vstack(rings)
    faces = []
    count = m if closed else m - 1
    for i in range(count):
        i2 = (i + 1) % m
        for j in range(n):
            j2 = (j + 1) % n
            faces.append([i * n + j, i * n + j2, i2 * n + j2, i2 * n + j])
    extra = []
    if cap_start and not closed:
        c = rings[0].mean(axis=0)
        ci = len(verts) + len(extra)
        extra.append(c)
        faces += [[ci, (j + 1) % n, j] for j in range(n)]
    if cap_end and not closed:
        c = rings[-1].mean(axis=0)
        ci = len(verts) + len(extra)
        extra.append(c)
        base = (m - 1) * n
        faces += [[ci, base + j, base + (j + 1) % n] for j in range(n)]
    if extra:
        verts = np.vstack([verts, np.array(extra)])
    return Shape(verts, faces, smooth)


def path_frames(points, hint=(0.0, -1.0, 0.0)):
    """Tangents and parallel-transported (r, f) frames along a polyline."""
    P = np.asarray(points, dtype=np.float64)
    T = np.zeros_like(P)
    T[1:-1] = P[2:] - P[:-2]
    T[0], T[-1] = P[1] - P[0], P[-1] - P[-2]
    T = np.array([unit(t) for t in T])
    r, _ = frame_for(T[0], hint)
    frames = []
    for i, t in enumerate(T):
        if i:
            r = unit(r - t * t.dot(r))
        frames.append((r, np.cross(t, r)))
    return T, frames


def tube(points, radii, n=16, hint=(0.0, -1.0, 0.0), power=2.0, cap_start=True, cap_end=True, shape=None, smooth=True):
    """A tube through points. radii: one number, or per point a number or (r, f) pair (r along the frame's first axis,
    which starts square to the path from hint). shape(i, u, x, y) may reshape ring i."""
    P = np.asarray(points, dtype=np.float64)
    _, frames = path_frames(P, hint)
    rings = []
    for i, p in enumerate(P):
        rad = radii if np.isscalar(radii) else radii[i]
        rx, ry = (rad, rad) if np.isscalar(rad) else rad
        fn = None if shape is None else (lambda u, x, y, i=i: shape(i, u, x, y))
        rings.append(ring(p, frames[i][0], frames[i][1], rx, ry, n, power, fn))
    return loft(rings, cap_start, cap_end, smooth)


def limb(a, b, r0, r1, steps=6, n=16, hint=(0.0, -1.0, 0.0), bulge=0.0, at=0.4, power=2.0, flat=1.0, cap=True):
    """A tapered tube from a to b (radius r0 to r1), swelling by bulge (a share of the radius) around at; flat squashes
    its cross-section along the second axis."""
    a, b = v3(a), v3(b)
    pts, rads = [], []
    for i in range(steps + 1):
        t = i / steps
        r = r0 + (r1 - r0) * t
        r *= 1.0 + bulge * math.exp(-((t - at) / 0.28) ** 2)
        pts.append(a + (b - a) * t)
        rads.append((r, r * flat))
    return tube(pts, rads, n, hint, power, cap, cap)


def ellipsoid(center, radii, n=24, rings_count=14, axes=None, power=2.0, shape=None, smooth=True):
    """An ellipsoid (radii along axes, default x, y, z). shape(p_local) -> p_local may reshape it (local units are
    radii-scaled before axes are applied)."""
    rx, ry, rz = radii
    ax = axes or (v3(1, 0, 0), v3(0, 1, 0), v3(0, 0, 1))
    rings_list = []
    for i in range(1, rings_count):
        phi = math.pi * i / rings_count
        z = -math.cos(phi)
        rr = math.sin(phi)
        u = np.linspace(0.0, 2.0 * math.pi, n, endpoint=False)
        cu, su = np.cos(u), np.sin(u)
        e = 2.0 / power
        loc = np.stack([rr * np.sign(cu) * np.abs(cu) ** e, rr * np.sign(su) * np.abs(su) ** e, np.full(n, z)], axis=1)
        if shape is not None:
            loc = shape(loc)
        rings_list.append(loc)
    loc_all = np.vstack(rings_list)
    bottom, top = np.array([[0.0, 0.0, -1.0]]), np.array([[0.0, 0.0, 1.0]])
    if shape is not None:
        bottom, top = shape(bottom), shape(top)
    loc_all = np.vstack([loc_all, bottom, top])
    world = v3(center)[None, :] + (loc_all[:, 0:1] * rx) * ax[0][None, :] + (loc_all[:, 1:2] * ry) * ax[1][None, :] + \
        (loc_all[:, 2:3] * rz) * ax[2][None, :]
    m = rings_count - 1
    faces = []
    for i in range(m - 1):
        for j in range(n):
            j2 = (j + 1) % n
            faces.append([i * n + j, i * n + j2, (i + 1) * n + j2, (i + 1) * n + j])
    b_i, t_i = m * n, m * n + 1
    faces += [[b_i, j2, j] for j, j2 in ((j, (j + 1) % n) for j in range(n))]
    base = (m - 1) * n
    faces += [[t_i, base + j, base + (j + 1) % n] for j in range(n)]
    return Shape(world, faces, smooth)


def lathe(profile, center=(0, 0, 0), axis=(0, 0, 1), n=32, hint=(1.0, 0.0, 0.0), cap_start=True, cap_end=True,
          power=2.0, squash=1.0, shape=None, smooth=True):
    """Turns a profile of (radius, height) round axis through center: hats, buttons, lanterns. squash scales the
    second cross axis (an oval crown)."""
    axis = unit(axis)
    r_axis, f_axis = frame_for(axis, hint)
    rings_list = []
    for i, (r, h) in enumerate(profile):
        fn = None if shape is None else (lambda u, x, y, i=i: shape(i, u, x, y))
        rings_list.append(ring(v3(center) + axis * h, r_axis, f_axis, max(r, 1e-5), max(r, 1e-5) * squash, n, power, fn))
    return loft(rings_list, cap_start, cap_end, smooth)


def torus(center, axis, major, minor, n=32, m=10, squash=1.0, hint=(1.0, 0.0, 0.0), minor_squash=1.0):
    """A torus round axis (major radius, squash scales it across), its tube minor thick (minor_squash along the axis)."""
    axis = unit(axis)
    r_axis, f_axis = frame_for(axis, hint)
    u = np.linspace(0.0, 2.0 * math.pi, n, endpoint=False)
    rings_list = []
    for a in u:
        c = v3(center) + r_axis * math.cos(a) * major + f_axis * math.sin(a) * major * squash
        out = unit(r_axis * math.cos(a) + f_axis * math.sin(a) * squash)
        rings_list.append(ring(c, out, axis, minor, minor * minor_squash, m))
    return loft(rings_list, False, False, True, closed=True)


def band(center, radii, z0, z1, n=32, power=2.0, offset=0.0, shape=None):
    """A belt or a cuff: a short tube round a vertical axis (radii x, y), from height z0 to z1, pushed out by offset."""
    c = v3(center)
    r0 = ring(c + v3(0, 0, z0), v3(1, 0, 0), v3(0, -1, 0), radii[0] + offset, radii[1] + offset, n, power, shape)
    r1 = ring(c + v3(0, 0, z1), v3(1, 0, 0), v3(0, -1, 0), radii[0] + offset, radii[1] + offset, n, power, shape)
    return loft([r0, r1], True, True, True)


def prism(points2d, depth, origin, x_axis, y_axis, bevel=0.0, smooth=False):
    """An extruded flat shape (badge, card, plate): 2D points counterclockwise in the (x_axis, y_axis) plane, depth
    along x_axis x y_axis, centered on origin. bevel shrinks the faces' outlines (a chamfered edge)."""
    o, xa, ya = v3(origin), unit(x_axis), unit(y_axis)
    za = np.cross(xa, ya)
    p2 = np.asarray(points2d, dtype=np.float64)
    m = len(p2)

    def place(pts, z):
        return o[None, :] + pts[:, 0:1] * xa[None, :] + pts[:, 1:2] * ya[None, :] + z * za[None, :]

    if bevel > 0.0:
        c = p2.mean(axis=0)
        inner = c + (p2 - c) * (1.0 - bevel)
        layers = [place(inner, -depth / 2), place(p2, -depth / 2 + depth * 0.25), place(p2, depth / 2 - depth * 0.25),
                  place(inner, depth / 2)]
    else:
        layers = [place(p2, -depth / 2), place(p2, depth / 2)]
    verts = np.vstack(layers)
    faces = []
    L = len(layers)
    for i in range(L - 1):
        for j in range(m):
            j2 = (j + 1) % m
            faces.append([i * m + j, i * m + j2, (i + 1) * m + j2, (i + 1) * m + j])
    faces.append(list(reversed(range(m))))
    faces.append([(L - 1) * m + j for j in range(m)])
    return Shape(verts, faces, smooth)


def box(center, size, axes=None, bevel=0.0):
    """A box (size x, y, z along axes), optionally chamfered."""
    sx, sy, sz = (s / 2.0 for s in size)
    pts = [(-sx, -sy), (sx, -sy), (sx, sy), (-sx, sy)]
    ax = axes or (v3(1, 0, 0), v3(0, 1, 0), v3(0, 0, 1))
    return prism(pts, sz * 2.0, center, ax[0], ax[1], bevel)


def star(points=5, outer=1.0, inner=0.45, rot=math.pi / 2):
    out = []
    for i in range(points * 2):
        r = outer if i % 2 == 0 else inner
        a = rot + math.pi * i / points
        out.append((r * math.cos(a), r * math.sin(a)))
    return out


def rounded_rect(w, h, r, steps=4):
    pts = []
    for cx, cy, a0 in ((w / 2 - r, h / 2 - r, 0.0), (-w / 2 + r, h / 2 - r, 90.0), (-w / 2 + r, -h / 2 + r, 180.0),
                       (w / 2 - r, -h / 2 + r, 270.0)):
        for i in range(steps + 1):
            a = math.radians(a0 + 90.0 * i / steps)
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return pts


# ------------------------------------------------------------------------------------------------------------ helpers

def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


# --------------------------------------------------------------------------------------------------------- the hero

def print_color(hex_value, flag):
    """The Screen Print Wash fill rule (Docs/Art/ScreenPrintWash.md, as the concept viewer bakes it): fills are lifted
    toward the paper; accents keep more of their color; glows stay bright and go unshaded. Returns sRGB and the
    viewer's flag (1 shaded, 0.5 shaded teal, 0 unshaded)."""
    c = (((hex_value >> 16) & 255) / 255.0, ((hex_value >> 8) & 255) / 255.0, (hex_value & 255) / 255.0)
    h, s, v = colorsys.rgb_to_hsv(*c)
    if flag == 'glow':
        return colorsys.hsv_to_rgb(h, min(1.0, s * 1.2), min(1.0, v * 1.08)), 0.0
    if flag == 'accent':
        c = colorsys.hsv_to_rgb(h, min(1.0, s * 1.35), min(1.0, v * 1.05))
        lift = 0.1
        teal = 0.42 < h < 0.6
    else:
        c = colorsys.hsv_to_rgb(h, min(1.0, s * 1.22), v)
        lift = 0.17
        teal = False
    c = tuple(min(1.0, (1.0 - lift) * a + lift * b) for a, b in zip(c, CREAM))
    return c, (0.5 if teal else 1.0)


class Hero:
    """Collects a hero's parts, then rigs, skins and joins them (finish)."""

    def __init__(self, name, build, palette):
        self.name = name
        self.sk = Skeleton(build)
        self.b = build
        self.palette = palette          # color key -> (0xRRGGBB, 'fill' | 'accent' | 'glow')
        self.parts = []                 # (name, Shape, color key, weighting)
        self.sockets = {}               # name -> (bone, position, x axis, y axis)

    # weighting: ('rigid', bone) | ('bones', {bone: bias}, power) | ('skirt', settings) | callable(verts) -> {bone: w}
    def add(self, name, shape, color, rigid=None, bones=None, power=5.0, skirt=None, weights=None):
        if rigid is not None:
            how = ('rigid', rigid)
        elif skirt is not None:
            how = ('skirt', skirt)
        elif weights is not None:
            how = ('fn', weights)
        else:
            how = ('bones', bones if isinstance(bones, dict) else {b: 1.0 for b in bones}, power)
        self.parts.append((name, shape, color, how))
        return shape

    def socket(self, name, bone, position, x_axis=(1, 0, 0), y_axis=(0, 1, 0)):
        self.sockets[name] = (bone, v3(position), unit(x_axis), unit(y_axis))

    # --- weights
    def _weights(self, verts, how):
        sk = self.sk
        kind = how[0]
        if kind == 'rigid':
            return {how[1]: np.ones(len(verts))}
        if kind == 'fn':
            return how[1](verts)
        if kind == 'skirt':
            return self._skirt(verts, how[1])
        biases, power = how[1], how[2]
        names = list(biases)
        D = np.stack([seg_distance(verts, sk.head(b), sk.tail(b)) for b in names], axis=1)
        W = np.array([biases[b] for b in names])[None, :] / (D + 0.012) ** power
        return dict(zip(names, normalize_top4(W).T))

    def _skirt(self, verts, opts):
        """A coat's skirt or tails: near the waist it follows the pelvis; lower down, the side over each leg follows that
        thigh (stiff = how much the cloth lags behind), the back blending both. Below the knee it stays with the thigh,
        so a long coat swings with the legs."""
        sk = self.sk
        stiff = opts.get('stiff', 0.8)
        top = opts.get('top', sk.hip_z)
        z = verts[:, 2]
        t = np.clip((top - z) / max(1e-3, top - sk.knee_z), 0.0, 1.0)
        leg = stiff * t ** 0.7
        width = opts.get('width', sk.b.hips * 1.4)
        side = np.clip(0.5 + verts[:, 0] / width, 0.0, 1.0)
        side = smoothstep(0.0, 1.0, side)
        out = {'pelvis': 1.0 - leg, 'thigh_l': leg * side, 'thigh_r': leg * (1.0 - side)}
        if opts.get('spine', 0.0) > 0.0:
            up = np.clip((z - top) / 0.15, 0.0, 1.0) * opts['spine']
            out = {k: v * (1.0 - up) for k, v in out.items()}
            out['spine_01'] = up
        return {k: np.broadcast_to(v, (len(verts),)).astype(np.float64) for k, v in out.items()}

    # --- assembly
    def finish(self):
        """Builds the armature and the skinned mesh, and bakes the print colors. Returns (armature, mesh)."""
        arm = bpy.data.objects.new(self.name, bpy.data.armatures.new(self.name))
        bpy.context.scene.collection.objects.link(arm)
        bpy.ops.object.select_all(action='DESELECT')
        bpy.context.view_layer.objects.active = arm
        arm.select_set(True)
        bpy.ops.object.mode_set(mode='EDIT')
        edit = arm.data.edit_bones
        for name, (h, t, parent) in self.sk.bones.items():
            bone = edit.new(name)
            bone.head, bone.tail = Vector(h), Vector(t)
            d = unit(t - h)
            bone.align_roll(Vector((0.0, 0.0, 1.0)) if abs(d[1]) > 0.7 else Vector((0.0, -1.0, 0.0)))
            bone.use_connect = False
            bone.use_deform = True
        for name, (h, t, parent) in self.sk.bones.items():
            if parent:
                edit[name].parent = edit[parent]
        bpy.ops.object.mode_set(mode='OBJECT')
        arm['Hero'] = self.name

        materials = {}
        objects = []
        for part_name, shape, color, how in self.parts:
            key = f'Hero_{self.name}_{color}'
            if key not in materials:
                hex_value, flag = self.palette[color]
                mat = lm.material(key, hex_value, 'Glow' if flag == 'glow' else 'Surface')
                mat['PrintHex'] = hex_value
                mat['PrintFlag'] = flag
                materials[key] = mat
            mesh = bpy.data.meshes.new(part_name)
            mesh.from_pydata([tuple(v) for v in shape.verts], [], [tuple(f) for f in shape.faces])
            mesh.validate(clean_customdata=False)
            mesh.materials.append(materials[key])
            obj = bpy.data.objects.new(part_name, mesh)
            bpy.context.scene.collection.objects.link(obj)
            fix_normals(obj)
            if shape.smooth:
                mesh.shade_smooth()
            verts = np.array([v.co[:] for v in mesh.vertices])
            raw = self._weights(verts, how)
            names = list(raw)
            W = normalize_top4(np.stack([np.broadcast_to(np.asarray(raw[b], dtype=np.float64), (len(verts),)) for b in names],
                                        axis=1) + 1e-12)
            for bone, w in zip(names, W.T):
                group = obj.vertex_groups.get(bone) or obj.vertex_groups.new(name=bone)
                for i in np.nonzero(w > 1e-4)[0]:
                    group.add([int(i)], float(w[i]), 'REPLACE')
            objects.append(obj)

        bpy.ops.object.select_all(action='DESELECT')
        for obj in objects:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = objects[0]
        bpy.ops.object.join()
        body = bpy.context.view_layer.objects.active
        body.name = body.data.name = f'Hero_{self.name}'
        body.parent = arm
        body.modifiers.new('Armature', 'ARMATURE').object = arm
        bake_print_colors(body)
        for name, (bone, pos, xa, ya) in self.sockets.items():
            empty = bpy.data.objects.new(f'SOCKET_{name}', None)
            bpy.context.scene.collection.objects.link(empty)
            za = np.cross(xa, ya)
            from mathutils import Matrix
            m = Matrix(((xa[0], ya[0], za[0], pos[0]), (xa[1], ya[1], za[1], pos[1]), (xa[2], ya[2], za[2], pos[2]),
                        (0, 0, 0, 1)))
            empty.matrix_world = m
            empty.parent = arm
            empty.parent_type = 'BONE'
            empty.parent_bone = bone
            empty.matrix_world = m
            empty['Bone'] = bone
        return arm, body


def seg_distance(P, a, b):
    ab = b - a
    t = np.clip(((P - a[None, :]) @ ab) / max(ab @ ab, 1e-12), 0.0, 1.0)
    return np.linalg.norm(P - (a[None, :] + t[:, None] * ab[None, :]), axis=1)


def normalize_top4(W):
    """Keeps each vertex's four strongest weights, dropping the faint ones, and makes them sum to 1."""
    W = W / W.sum(axis=1, keepdims=True)
    if W.shape[1] > 4:
        cut = -np.sort(-W, axis=1)[:, 3:4]
        W = np.where(W >= cut, W, 0.0)
    W = np.where(W >= 0.02, W, 0.0)
    return W / W.sum(axis=1, keepdims=True)


def fix_normals(obj):
    """Points every face outward (closed shapes) or consistently (open ones)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()


def bake_print_colors(obj):
    """The concept viewer's colors: each face's material color in the print palette, its flag in alpha."""
    mesh = obj.data
    attr = mesh.color_attributes.get('Flat') or mesh.color_attributes.new('Flat', 'BYTE_COLOR', 'CORNER')
    colors = np.empty((len(mesh.loops), 4), np.float32)
    cache = {}
    for poly in mesh.polygons:
        if poly.material_index not in cache:
            mat = mesh.materials[poly.material_index]
            rgb, flag = print_color(int(mat['PrintHex']), mat['PrintFlag'])
            cache[poly.material_index] = (*rgb, flag)
        colors[poly.loop_start:poly.loop_start + poly.loop_total] = cache[poly.material_index]
    attr.data.foreach_set('color_srgb', colors.reshape(-1))
    mesh.color_attributes.active_color = attr
    mesh.color_attributes.render_color_index = mesh.color_attributes.find('Flat')


# ------------------------------------------------------------------------------------------------------- body pieces
# Shared pieces every hero dresses: they follow the skeleton, so a part's bones line up with what it covers.

TORSO_BONES = {'pelvis': 1.0, 'spine_01': 1.0, 'spine_02': 1.0, 'spine_03': 1.0, 'spine_04': 1.0, 'spine_05': 1.0,
               'clavicle_l': 0.7, 'clavicle_r': 0.7, 'upperarm_l': 0.3, 'upperarm_r': 0.3, 'neck_01': 0.4}


def spine_at(sk, z):
    """The spine's point at height z (between the pelvis and the skull), from the bones' heads."""
    names = ['pelvis', 'spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05', 'neck_01', 'neck_02', 'head']
    pts = [sk.head(n) for n in names] + [sk.tail('head')]
    pts = sorted(pts, key=lambda p: p[2])
    zs = np.array([p[2] for p in pts])
    ys = np.array([p[1] for p in pts])
    return v3(0.0, float(np.interp(z, zs, ys)), z)


def torso(sk, profile, n=28, power=2.4, caps=(True, True), shape=None):
    """A torso lofted up the spine. profile: (t, half width, half depth, forward) rows, t from the hip joints' height
    (0) to the neck base (1), below 0 for the seat. shape(i, u, x, y) may reshape ring i."""
    rings_list = []
    for i, (t, w, d, fwd) in enumerate(profile):
        z = sk.hip_z + t * sk.T
        c = spine_at(sk, z) + v3(0.0, -fwd, 0.0)
        fn = None if shape is None else (lambda u, x, y, i=i: shape(i, u, x, y))
        rings_list.append(ring(c, v3(1, 0, 0), v3(0, -1, 0), w, d, n, power, fn))
    return loft(rings_list, caps[0], caps[1])


def arm_points(sk, side, start=-0.25, end=1.0, steps=10):
    """Points down an arm from a bit inside the shoulder (start < 0) to the wrist (end 1), through the elbow."""
    s, e, w = sk.head(f'upperarm_{side}'), sk.tail(f'upperarm_{side}'), sk.tail(f'lowerarm_{side}')
    pts = []
    for i in range(steps + 1):
        t = start + (end - start) * i / steps
        if t <= 0.5:
            u = t / 0.5
            pts.append(s + (e - s) * u)
        else:
            u = (t - 0.5) / 0.5
            pts.append(e + (w - e) * u)
    return pts


def sleeve(sk, side, radii, start=-0.25, end=1.0, n=16, power=2.0, flat=0.92):
    """An arm or sleeve: radii is a function of t (0 shoulder, 0.5 elbow, 1 wrist) giving the radius."""
    steps = 12
    pts = arm_points(sk, side, start, end, steps)
    rads = []
    for i in range(steps + 1):
        t = start + (end - start) * i / steps
        r = radii(t)
        rads.append((r, r * flat))
    return tube(pts, rads, n, hint=(0.0, -1.0, 0.0), power=power)


ARM_BONES = lambda side: {f'clavicle_{side}': 0.5, f'upperarm_{side}': 1.0, f'lowerarm_{side}': 1.0, f'hand_{side}': 0.5}


def leg_points(sk, side, start=-0.12, end=1.0, steps=12):
    """Points down a leg from a bit above the hip joint (start < 0) to the ankle (1), through the knee (0.5)."""
    h, kn, a = sk.head(f'thigh_{side}'), sk.tail(f'thigh_{side}'), sk.tail(f'calf_{side}')
    pts = []
    for i in range(steps + 1):
        t = start + (end - start) * i / steps
        if t <= 0.5:
            pts.append(h + (kn - h) * (t / 0.5))
        else:
            pts.append(kn + (a - kn) * ((t - 0.5) / 0.5))
    return pts


def trouser(sk, side, radii, start=-0.12, end=1.0, n=18, power=2.0, flat=1.0, shape=None):
    steps = 14
    pts = leg_points(sk, side, start, end, steps)
    rads = []
    for i in range(steps + 1):
        t = start + (end - start) * i / steps
        r = radii(t)
        rads.append((r, r * flat))
    return tube(pts, rads, n, hint=(0.0, -1.0, 0.0), power=power, shape=shape)


LEG_BONES = lambda side: {'pelvis': 0.5, f'thigh_{side}': 1.0, f'calf_{side}': 1.0, f'foot_{side}': 0.3}


def hand_shapes(sk, side, palm_scale=(1.0, 1.0, 1.0), finger_r=0.0098, thumb_r=0.0115, glove=1.0):
    """A hand: a palm block and five fingers along their bones. Returns [(Shape, weighting)] pairs."""
    p = sk.palm[side]
    a, n, s, q = p['a'], p['n'], p['s'], p['q'] * glove
    wrist, knuckle = p['wrist'], p['knuckle']
    length = np.linalg.norm(knuckle - wrist) * palm_scale[0]
    width, thick = 0.082 * q * palm_scale[1], 0.032 * q * palm_scale[2]
    center = wrist + a * length * 0.52 + n * 0.002
    palm = box(center, (length, width, thick), axes=(a, s, n), bevel=0.25)
    out = [(palm, ('bones', {f'hand_{side}': 1.0, f'index_metacarpal_{side}': 0.5, f'middle_metacarpal_{side}': 0.5,
                             f'ring_metacarpal_{side}': 0.5, f'pinky_metacarpal_{side}': 0.5}, 6.0))]
    for finger in FINGERS:
        names = [f'{finger}_0{i}_{side}' for i in (1, 2, 3)]
        pts = [sk.head(names[0]) - a * 0.008 * q] + [sk.tail(nm) for nm in names]
        r0 = finger_r * q * (0.95 if finger == 'pinky' else 1.0)
        shape = tube(pts, [r0 * 1.05, r0, r0 * 0.93, r0 * 0.85], 10, hint=s, cap_start=True, cap_end=True)
        out.append((shape, ('bones', {nm: 1.0 for nm in names} | {f'{finger}_metacarpal_{side}': 0.6}, 6.0)))
    names = [f'thumb_0{i}_{side}' for i in (1, 2, 3)]
    pts = [sk.head(names[0])] + [sk.tail(nm) for nm in names]
    out.append((tube(pts, [thumb_r * q * 1.25, thumb_r * q, thumb_r * q * 0.95, thumb_r * q * 0.85], 10, hint=n),
                ('bones', {nm: 1.0 for nm in names} | {f'hand_{side}': 0.6}, 6.0)))
    return out


def boot(sk, side, shaft_top=0.32, shaft_r=0.062, toe_r=0.05, heel=0.035, width=0.052, cuff=0.0, n=20, sole=0.012,
         toe_up=0.0):
    """A boot: a shaft from the ankle up to shaft_top (share of the shin's length, up from the ankle) and a foot lofted
    from the heel to the toe on a flat sole. Returns (shaft Shape, foot Shape)."""
    ank, ball, toe = sk.tail(f'calf_{side}'), sk.tail(f'foot_{side}'), sk.tail(f'ball_{side}')
    kn = sk.head(f'calf_{side}')
    top = ank + (kn - ank) * shaft_top
    shaft = tube([ank + v3(0, 0, -0.03), ank + (top - ank) * 0.5, top, top + (top - ank) * 0.02],
                 [shaft_r * 0.9, shaft_r * 0.97, shaft_r + cuff, shaft_r + cuff], n, hint=(0.0, -1.0, 0.0))
    # The foot: rings across the forward (-Y) axis, from behind the heel to the toe tip, flat at the sole.
    x0 = (ank[0] + toe[0]) / 2.0
    heel_y = ank[1] + heel
    toe_y = toe[1] - 0.012
    rows = 9
    rings_list = []
    for i in range(rows):
        t = i / (rows - 1)
        y = heel_y + (toe_y - heel_y) * t
        hgt = 0.10 * (1 - t) ** 1.2 + 0.045 * t if t < 0.55 else 0.065 - 0.035 * ((t - 0.55) / 0.45) ** 1.5
        hgt = max(hgt, 0.018)
        wd = width * (0.85 + 0.25 * math.sin(math.pi * min(1.0, t * 1.15)))
        if t > 0.85:
            wd *= 1.0 - (t - 0.85) * 2.2
        zc = sole + hgt * 0.5 + toe_up * t * t
        rx, rz = wd, hgt * 0.5 + sole * 0.5

        def flat_bottom(u, x, y_, rz=rz):
            return x, np.maximum(y_, -rz)

        r_ = ring(v3(x0, y, zc), v3(1, 0, 0), v3(0, 0, 1), rx, rz, n, 2.6, flat_bottom)
        r_[:, 2] = np.maximum(r_[:, 2], 0.0)
        rings_list.append(r_)
    foot = loft(rings_list)
    return shaft, foot


FOOT_BONES = lambda side: {f'calf_{side}': 0.6, f'foot_{side}': 1.0, f'ball_{side}': 1.0}


def neck(sk, r0, r1, n=18):
    a = sk.head('neck_01') + v3(0.0, 0.0, -0.03)
    b = sk.head('head') + v3(0.0, 0.0, 0.03)
    return tube([a, (a + b) / 2, b], [r0, (r0 + r1) / 2, r1], n, hint=(0.0, -1.0, 0.0))


NECK_BONES = {'spine_05': 0.6, 'neck_01': 1.0, 'neck_02': 1.0, 'head': 0.7}


def head_frame(sk):
    """The head's center and its axes (right +X, forward -Y, up), tilted as the head bone is."""
    up = unit(sk.tail('head') - sk.head('head'))
    fwd = unit(v3(0.0, -1.0, 0.0) - up * up[1] * -1.0)
    right = unit(np.cross(fwd, up))
    return sk.head_center, -right, fwd, up


def on_head(sk, x, y, z):
    """A point on the head's frame: x right, y forward, z up (meters from the head's center)."""
    c, rgt, fwd, up = head_frame(sk)
    return c + rgt * x + fwd * y + up * z


# ------------------------------------------------------------------------------------------------- cloth: sheets
# Coat skirts, lapels and capes are open sheets given a thickness, so they read from both sides and outline cleanly.

def arc(center, r_axis, f_axis, rx, ry, u0, u1, n=24, power=2.0):
    """Points along part of a superellipse ring, from angle u0 to u1 (radians from r_axis toward f_axis)."""
    u = np.linspace(u0, u1, n)
    cu, su = np.cos(u), np.sin(u)
    e = 2.0 / power
    x = rx * np.sign(cu) * np.abs(cu) ** e
    y = ry * np.sign(su) * np.abs(su) ** e
    return v3(center)[None, :] + x[:, None] * v3(r_axis)[None, :] + y[:, None] * v3(f_axis)[None, :]


def sheet(rows, smooth=True):
    """An open surface through rows of points (each row the same length)."""
    m, n = len(rows), len(rows[0])
    verts = np.vstack(rows)
    faces = [[i * n + j, i * n + j + 1, (i + 1) * n + j + 1, (i + 1) * n + j] for i in range(m - 1) for j in range(n - 1)]
    return Shape(verts, faces, smooth)


def vertex_normals(shape):
    V = shape.verts
    N = np.zeros_like(V)
    for f in shape.faces:
        p = V[f]
        n = np.zeros(3)
        for i in range(len(f)):
            n += np.cross(p[i], p[(i + 1) % len(f)])
        for i in f:
            N[i] += n
    lens = np.linalg.norm(N, axis=1, keepdims=True)
    return N / np.maximum(lens, 1e-12)


def thicken(shape, thickness, outward=1.0):
    """Gives an open sheet a thickness: a second layer behind it along the normals, and a rim round its open edges."""
    N = vertex_normals(shape) * outward
    V = shape.verts
    n = len(V)
    inner = V - N * thickness
    faces = [list(f) for f in shape.faces] + [[i + n for i in reversed(f)] for f in shape.faces]
    edges = {}
    for f in shape.faces:
        for i in range(len(f)):
            e = (f[i], f[(i + 1) % len(f)])
            key = tuple(sorted(e))
            edges.setdefault(key, []).append(e)
    for key, uses in edges.items():
        if len(uses) == 1:
            a, b = uses[0]
            faces.append([b, a, a + n, b + n])
    return Shape(np.vstack([V, inner]), faces, shape.smooth)


def cut(shape, keep):
    """Keeps the faces with at least one vertex where keep(vertices) is true, and drops the vertices left unused."""
    mask = np.asarray(keep(shape.verts), dtype=bool)
    faces = [f for f in shape.faces if mask[f].any()]
    used = sorted({i for f in faces for i in f})
    remap = {old: new for new, old in enumerate(used)}
    return Shape(shape.verts[used], [[remap[i] for i in f] for f in faces], shape.smooth)


def head_local(sk, verts):
    """Vertices in the head's frame: x toward the hero's left, y forward, z up, in meters from the head's center."""
    c, lft, fwd, up = head_frame(sk)
    rel = verts - c[None, :]
    return np.stack([rel @ lft, rel @ fwd, rel @ up], axis=1)


# ------------------------------------------------------------------------------------------- garments and face wraps

def garment(sk, rows, n=44, thickness=0.008, power=2.3, hem=None):
    """A coat, jacket or cape as one sheet given thickness. rows (top to bottom) are dicts: z (height), rx, ry (half
    width and depth), fwd (how far its middle sits forward of the spine, m), gap (half the opening at the front,
    radians; pi or more leaves only the back), or u0 and u1 (the arc's ends, radians from the hero's left toward the
    front). hem(u, z) may move the last row's points down (ragged tatters).
    Returns (Shape, edge rows) where the edges are the opening's points per row, left side first."""
    out, edges = [], []
    for i, r in enumerate(rows):
        z = r['z']
        c = spine_at(sk, min(max(z, sk.hip_z - 0.3), sk.neck_z)) * v3(0, 1, 0) + v3(0.0, -r.get('fwd', 0.0), z)
        g = r.get('gap', 0.0)
        u0, u1 = r.get('u0', math.pi / 2 + g), r.get('u1', math.pi / 2 + 2 * math.pi - g)
        pts = arc(c, v3(1, 0, 0), v3(0, -1, 0), r['rx'], r['ry'], u0, u1, n, r.get('power', power))
        if hem is not None and i == len(rows) - 1:
            u = np.linspace(u0, u1, n)
            pts[:, 2] -= hem(u, z)
        out.append(pts)
        edges.append((pts[0], pts[-1]))
    return thicken(sheet(out), thickness), edges


def garment_weights(hero, waist, blend=0.08, stiff=0.85, width=None, torso=None):
    """Weights for a garment that is a jacket above the waist (follows the spine and shoulders) and a skirt below it
    (follows the legs), blended over a band round the waist."""
    torso_bones = torso or TORSO_BONES

    def fn(verts):
        top = hero._weights(verts, ('bones', torso_bones, 5.0))
        low = hero._skirt(verts, {'stiff': stiff, 'top': waist, 'width': width or hero.b.hips * 1.5})
        k = smoothstep(waist - blend, waist + blend, verts[:, 2])
        out = {}
        for b, w in top.items():
            out[b] = out.get(b, 0) + w * k
        for b, w in low.items():
            out[b] = out.get(b, 0) + w * (1.0 - k)
        return out
    return fn


def lapel_strip(edges, rows_range, widths, side_sign, lift=0.004):
    """A strip folded back along a garment's opening (lapels, facings): for each row in rows_range the edge point and
    a point widths[i] toward the side, lifted off the cloth. side_sign +1 for the hero's left edge."""
    inner, outer = [], []
    for i, w in zip(rows_range, widths):
        p = edges[i][1] if side_sign > 0 else edges[i][0]
        out_dir = unit(v3(side_sign, -0.35, 0.0))
        inner.append(p + v3(0, -lift, 0))
        outer.append(p + out_dir * w + v3(0, -lift * 2, 0))
    return thicken(sheet([np.array(inner), np.array(outer)]), 0.006)


SKULL = dict(rx=0.085, ry=0.1, rz=0.112, jaw=0.78, chin=0.012, back=1.0)


def skull_shape_fn(rx, ry, jaw, chin, back):
    def shape(p):
        p = p.copy()
        low = np.clip(-p[:, 2], 0.0, 1.0)
        p[:, 0] *= 1.0 - (1.0 - jaw) * low ** 1.3
        front = p[:, 1] > 0
        p[:, 1] = np.where(front, p[:, 1] + chin / ry * low ** 2, p[:, 1] * back)
        return p
    return shape


def skull_surface(sk, u, z_local, s):
    """Points on a skull built with settings s (SKULL keys) at angles u (radians: 0 the hero's left, pi/2 the front)
    and height z_local (meters above the head's center), in world space."""
    c, lft, fwd, up = head_frame(sk)
    zu = np.clip(z_local / s['rz'], -0.999, 0.999)
    rr = math.sqrt(1.0 - zu * zu)
    p = np.stack([rr * np.cos(u), rr * np.sin(u), np.full(len(u), zu)], axis=1)
    p = skull_shape_fn(s['rx'], s['ry'], s['jaw'], s['chin'], s['back'])(p)
    return c[None, :] + (p[:, 0:1] * s['rx']) * lft[None, :] + (p[:, 1:2] * s['ry']) * fwd[None, :] + \
        (p[:, 2:3] * s['rz']) * up[None, :]


def face_wrap(sk, s, z_rows, offset, gap=0.0, nose=0.0, n=36):
    """Cloth wrapped round the head (a bandana, a scarf, a mask): rows at the heights z_rows (head frame), pushed off the
    skull by offset (one per row), open at the back by gap radians; nose lifts the front of the upper rows over a nose.
    Returns an open sheet (thicken it)."""
    c, lft, fwd, up = head_frame(sk)
    u = np.linspace(-math.pi / 2 + gap, 3 * math.pi / 2 - gap, n)
    rows = []
    for i, z in enumerate(z_rows):
        pts = skull_surface(sk, u, z, s)
        out = pts - (c[None, :] + up[None, :] * z)
        out = out / np.maximum(np.linalg.norm(out, axis=1, keepdims=True), 1e-9)
        o = offset[i] if not np.isscalar(offset) else offset
        pts = pts + out * o
        if nose:
            k = max(0.0, 1.0 - i / max(1, len(z_rows) - 1) * 1.6)
            front = np.exp(-((u - math.pi / 2) / 0.32) ** 2) * nose * k
            pts = pts + fwd[None, :] * front[:, None]
        rows.append(pts)
    return sheet(rows)


def skull(sk, s=None, n=32, rings_count=20):
    """A skull with settings s (see SKULL)."""
    s = s or SKULL
    c, lft, fwd, up = head_frame(sk)
    return ellipsoid(c, (s['rx'], s['ry'], s['rz']), n, rings_count, axes=(lft, fwd, up),
                     shape=skull_shape_fn(s['rx'], s['ry'], s['jaw'], s['chin'], s['back']))


def bandana(sk, s, top=0.01, chin=-0.1, drop=0.11, spread=1.35, offset=(0.006, 0.014), n=31, rows_face=5, rows_drape=4,
            nose=0.02):
    """A kerchief tied over the nose: it hugs the face from the nose bridge (top) to under the chin, round the front
    and sides (spread radians either side of the front), then hangs in a point drop below the chin. One open sheet."""
    c, lft, fwd, up = head_frame(sk)
    u = np.linspace(math.pi / 2 - spread, math.pi / 2 + spread, n)
    rows = []
    for i in range(rows_face):
        t = i / (rows_face - 1)
        z = top + (chin - top) * t
        pts = skull_surface(sk, u, z, s)
        out = pts - (c[None, :] + up[None, :] * z)
        out = out / np.maximum(np.linalg.norm(out, axis=1, keepdims=True), 1e-9)
        pts = pts + out * (offset[0] + (offset[1] - offset[0]) * t)
        front = np.exp(-((u - math.pi / 2) / 0.32) ** 2) * nose * max(0.0, 1.0 - t * 1.6)
        rows.append(pts + fwd[None, :] * front[:, None])
    last = rows[-1]
    mid = last[n // 2]
    for k in range(1, rows_drape + 1):
        t = k / rows_drape
        # The sides gather toward the middle as the cloth falls, so the bottom edge comes to a point.
        pts = last + (mid[None, :] - last) * (t ** 0.85) - up[None, :] * drop * t + fwd[None, :] * 0.012 * t
        rows.append(pts)
    return sheet(rows)


def garment_surface(sk, rows, x, z, power=2.3):
    """A point on a garment's front (rows as given to garment()) at sideways offset x and height z, with its outward
    normal and sideways tangent, for seating pockets, badges and straps on the cloth."""
    rs = sorted(rows, key=lambda r: r['z'])
    zs = [r['z'] for r in rs]
    rx = float(np.interp(z, zs, [r['rx'] for r in rs]))
    ry = float(np.interp(z, zs, [r['ry'] for r in rs]))
    fwd = float(np.interp(z, zs, [r.get('fwd', 0.0) for r in rs]))
    q = power
    ax = min(abs(x) / rx, 0.999)
    y = ry * (1.0 - ax ** q) ** (1.0 / q)
    c = spine_at(sk, min(max(z, sk.hip_z - 0.3), sk.neck_z))
    point = v3(x, c[1] - fwd - y, z)
    normal = unit(v3(math.copysign(ax ** (q - 1) / rx, x), -((y / ry) ** (q - 1)) / ry, 0.0))
    tangent = unit(np.cross(v3(0, 0, 1), normal))
    return point, normal, tangent
