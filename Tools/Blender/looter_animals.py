"""Helpers for the small animals that make Ransom's Rest feel lived in (the 2026-10-14 "make it alive" polish, look sheet
44): Art/Models/Creatures/Chicken.py (two hens and a rooster), Cat.py (a barn cat) and Swallow.py (cliff swallows and
their mud nests). Tools/Blender/animals_sheet.py composes the look sheet from their renders.

The chickens and the cat are rigs the game poses in code (UCreaturePoseAnimInstance, as the spider's and Hob's). Each
has one smooth skin made Hob.py's way: soft ellipsoids (a metaball, kept only as a BVH tree), rings along a spine cast
onto them and relaxed into even quads; feathers, fur tufts and legs are added as cards and tubes, every vertex weighted to
its bones. The swallows are instanced pieces like the fauna's other birds (AmbientFauna.py, World/FaunaFlock).

Everything is built in creature space: centimeters, x forward, y left, z up, the origin on the ground under the body
(looter_creatures.to_blender turns it into Blender's meters, front to -Y). Each animal paints its own atlas (one texture
set, one material slot) every run, deterministically: fixed seeds, numpy only, no iteration of BMesh sets.
Painting helpers (the refined pass, 2026-10-14): plumage() lays feathers in overlapping rows (each shading the one it
lies over), skin_rows() sizes them along a skin, barbs() and line_noise() give vanes and hair their fine grain;
path_rings() eases a limb's centre line through its joints (a bent knee stays round).

UV conventions (the painters rely on them):
  skins and ellipsoids  latitude-longitude: U around (0.5 = the back/top, the seam under the belly; U rises toward
                        the animal's left), V along (0 = the rear, 1 = the front)
  tubes (legs, tails)   U along (0 = the root), V around (0.5 = toward the tube's reference side, the seam opposite)
  cards (feathers)      U across (the strip's middle on the shaft), V along (0 = the root, 1 = the tip)
"""
import json
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree

import looter_creatures as lc
import looter_homes as homes
import looter_model as lm
import looter_textures as lt

UP = Vector((0.0, 0.0, 1.0))
SIDES = ((1.0, 'l'), (-1.0, 'r'))          # creature space has y to the left
SHEET_PATH = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'LookSheets', '44_Small_animals.jpg')
POSE_DIR = os.path.join(lt.REPO, 'Intermediate', 'AnimalsModel')


def args():
    """The script's own arguments (after '--'), commas split."""
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    return [a for arg in argv for a in arg.split(',') if a]


# Renders go to Saved/ArtPreviews/RansomsRest/Animals, or a folder in it with the argument previews=<folder> (the
# refined pass's renders sit in Animals/Refined beside the approved sheet's own).
PREVIEW_DIR = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Animals',
                           *[a[9:] for a in args() if a.startswith('previews=')][:1])


def log(message):
    lt._log(message)


def smooth(e0, e1, x):
    if e1 == e0:
        return 1.0 if x >= e1 else 0.0
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def turn(yaw=0.0, pitch=0.0, roll=0.0):
    """A rotation (4x4) in creature axes, degrees: yaw about z (toward the left), pitch about y (nose down), roll about x
    (the left side up)."""
    return (Matrix.Rotation(math.radians(yaw), 4, 'Z') @ Matrix.Rotation(math.radians(pitch), 4, 'Y')
            @ Matrix.Rotation(math.radians(roll), 4, 'X'))


def about(axis, degrees):
    """A rotation (4x4) about a creature-space axis."""
    return Matrix.Rotation(math.radians(degrees), 4, Vector(axis).normalized())


def rot_about(pivot, R):
    pivot = Vector(pivot)
    return Matrix.Translation(pivot) @ R.to_4x4() @ Matrix.Translation(-pivot)


def catmull(points, t):
    n = len(points) - 1
    x = min(max(t, 0.0), 1.0) * n
    k = min(int(x), n - 1)
    f = x - k
    p0, p1, p2 = points[max(k - 1, 0)], points[k], points[k + 1]
    p3 = points[min(k + 2, n)]
    return 0.5 * ((2 * p1) + (-p0 + p2) * f + (2 * p0 - 5 * p1 + 4 * p2 - p3) * f * f + (-p0 + 3 * p1 - 3 * p2 + p3) * f ** 3)


def newell(points):
    n = Vector((0.0, 0.0, 0.0))
    for a, b in zip(points, points[1:] + points[:1]):
        n.x += (a.y - b.y) * (a.z + b.z)
        n.y += (a.z - b.z) * (a.x + b.x)
        n.z += (a.x - b.x) * (a.y + b.y)
    return n


def tris(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


# --- The atlas (any width and height) ---

def region_grid(region, size):
    """The pixel block of an atlas region ((u0, u1, v0, v1) of a width x height texture) and its local U, V (0..1)."""
    width, height = size
    u0, u1, v0, v1 = region
    c0, c1 = int(round(u0 * width)), int(round(u1 * width))
    r0, r1 = int(round((1 - v1) * height)), int(round((1 - v0) * height))
    cols = (np.arange(c0, c1) + 0.5) / width
    rows = 1 - (np.arange(r0, r1) + 0.5) / height
    uu, vv = np.meshgrid((cols - u0) / (u1 - u0), (rows - v0) / (v1 - v0))
    return (slice(r0, r1), slice(c0, c1)), uu.astype(np.float32), vv.astype(np.float32)


def paint_atlas(atlas, painter, size, px_m, set_name, normal_strength=1.0):
    """Paints every region with painter(region, U, V, index) -> (color, roughness, height m, occlusion) and writes the
    texture set Art/Textures/<set>/T_<set>_BC, _N (per region, each wrapping round its own seam) and _ORM."""
    width, height = size
    color = np.zeros((height, width, 3), np.float32)
    rough = np.full((height, width), 0.7, np.float32)
    occl = np.ones((height, width), np.float32)
    normal = np.zeros((height, width, 3), np.uint8)
    normal[...] = (128, 128, 255)
    for k, (name, region) in enumerate(atlas.items()):
        sl, U, V = region_grid(region, size)
        col, r, h, ao = painter(name, U, V, k)
        color[sl], rough[sl], occl[sl] = col, r, ao
        normal[sl] = lt.normal_map(np.asarray(h, np.float32), px_m, normal_strength)
    out_dir = os.path.join(lt.TEXTURE_DIR, set_name)
    os.makedirs(out_dir, exist_ok=True)
    lt.write_png(os.path.join(out_dir, f'T_{set_name}_BC.png'), lt.to8(np.clip(color, 0, 1)))
    lt.write_png(os.path.join(out_dir, f'T_{set_name}_N.png'), normal)
    lt.write_png(os.path.join(out_dir, f'T_{set_name}_ORM.png'),
                 lt.to8(np.stack([np.clip(occl, 0, 1), np.clip(rough, 0, 1), np.zeros_like(rough)], axis=-1)))
    return out_dir


def frac(x):
    return x - np.floor(x)


def hash01(a, b=0.0):
    """A repeatable value in [0, 1) per integer cell (a, b)."""
    return frac(np.sin(a * 12.9898 + b * 78.233) * 43758.5453)


def strand_noise(shape, seed, along_axis=0, fine=1.2, long=14.0):
    """Hair or barbs: noise stretched along one axis (0: along V, the rows; 1: along U, the columns)."""
    if along_axis == 0:
        return lt.noise(shape, seed, fine, long)
    return lt.noise(shape, seed, long, fine)


def line_noise(p, seed, period=997):
    """Smooth 1D value noise (about -1..1) at positions p (any array; one random value per whole number, eased between):
    the barbs across a feather, streaks of hair. Repeats every `period`."""
    table = (np.random.default_rng(seed).random(period).astype(np.float32) - 0.5) * 2.0
    x = np.mod(p, period)
    i0 = np.floor(x).astype(np.int64)
    f = (x - i0).astype(np.float32)
    f = f * f * (3.0 - 2.0 * f)
    return table[i0 % period] * (1.0 - f) + table[(i0 + 1) % period] * f


def feather_outline(t, root=0.84, tip_from=0.42, pointed=0.0):
    """A contour feather's half-width (1 at its widest) at t (0 the root, 1 the tip): full a little after the root,
    rounding in to the tip (pointed 1: tapering to a point, as hackles do)."""
    f = root + (1.0 - root) * lt.smooth(0.0, 0.3, t)
    k = np.clip((t - tip_from) / (1.0 - tip_from), 0.0, 1.0)
    round_tip = np.sqrt(np.clip(1.0 - k * k, 0.0, 1.0)) ** 0.85
    point_tip = (1.0 - k) ** 0.9
    return f * (round_tip + (point_tip - round_tip) * pointed)


def plumage(q, U, cols_at, length=1.8, width=0.62, seed=0, irregular=1.0, tip_from=0.42, pointed=0.0, max_length=None,
            fray=0.07):
    """Overlapping feathers in rows round a skin, each row's tips lying over the next row's roots as feathers lie (the
    front ones on top): the pattern a painted body wears instead of scales. q: the row coordinate per pixel (whole
    numbers at the rows' roots, rising toward the tips, the rear); U: across (0..1, wrapping); cols_at(row): the
    feathers in a row (an array op; whole numbers keep a wrapping seam closed); length: a feather's length in rows (over
    1: they overlap); width: its width in columns; pointed: 0 round tips .. 1 pointed. length, width and pointed may be
    functions of the row (an array op), for feathers that change size along the body (max_length: the longest).
    Per pixel, of the topmost feather: t (0 its root .. 1 its tip), x (-1..1 across its vane), row, col, a random h
    (0..1 per feather) and h2, margin (<= 0 inside its outline, 0 on it), and over: how far this pixel lies outside the
    outline of the feather in front of it (in half-widths; small: in that feather's shadow). Also height: the shingled
    surface (0..1), each feather rising toward its tip and stepping down onto the next."""
    per_row = lambda f, i: f(i) if callable(f) else f
    K = int(math.ceil(max_length or (length if not callable(length) else 2.5))) + 1
    base = np.floor(q)
    cands = []
    sel = np.full(q.shape, -1, np.int64)
    for k in range(K):
        i = base - k
        n = cols_at(i)
        jit = hash01(i * 1.37 + seed, 3.1) - 0.5
        c = U * n + 0.5 * np.mod(i, 2.0) + 0.3 * irregular * jit
        col = np.floor(c + 0.5)
        h = hash01(i + seed * 0.731, col * 1.13)
        h2 = hash01(col * 0.917 + 17.0 + seed, i * 1.91)
        L = per_row(length, i) * (1.0 + 0.16 * irregular * (h - 0.5))
        W = per_row(width, i) * (1.0 + 0.2 * irregular * (h2 - 0.5))
        t = (q - i) / L
        lean = 0.34 * irregular * (hash01(i * 3.1 + seed, col * 0.7) - 0.5)
        x = ((c - col) - lean * t) / W
        margin = np.abs(x) - feather_outline(t, tip_from=tip_from, pointed=per_row(pointed, i))
        # A frayed edge: the barbs' ends never make a clean line.
        margin = margin + fray * line_noise(t * 11.0 + np.sign(x) * 0.5 + h * 300.0, seed + 11) * lt.smooth(0.25, 0.8, t)
        margin = np.where(t > 1.0, np.maximum(margin, (t - 1.0) * 3.0), margin)
        cover = (t >= 0.0) & (margin <= 0.0)
        sel = np.where(cover, k, sel)
        cands.append((t, x, i, col, h, h2, margin))
    sel = np.where(sel < 0, 0, sel)
    out = {name: np.zeros(q.shape, np.float32) for name in ('t', 'x', 'row', 'col', 'h', 'h2', 'margin')}
    over = np.full(q.shape, 9.0, np.float32)
    for k, values in enumerate(cands):
        m = sel == k
        for name, v in zip(('t', 'x', 'row', 'col', 'h', 'h2', 'margin'), values):
            out[name] = np.where(m, v, out[name])
        over = np.where(sel < k, np.minimum(over, np.maximum(values[6], 0.0)), over)
    out['over'] = over
    t, margin = out['t'], out['margin']
    roll = lt.smooth(0.0, 0.3, -margin)
    out['height'] = (0.3 + 0.7 * np.clip(t, 0.0, 1.0) ** 0.8) * (0.86 + 0.14 * roll)
    return out


def barbs(t, x, row, col, seed, count=26.0, slant=0.55):
    """The fine barbs of a feather's vane in its own coordinates (t along, x across): lines slanting out from the shaft
    toward the tip, each a little lighter or darker (about -1..1), and now and then a split where they part."""
    p = (t * count - np.abs(x) * count * slant) + hash01(row, col) * 97.0
    lines = line_noise(p, seed)
    split_at = hash01(row * 0.77 + 3.0, col * 1.3) * 0.6 + 0.3
    split = (1.0 - lt.smooth(0.0, 0.05, np.abs(t - np.abs(x) * slant * 0.4 - split_at))) * lt.smooth(0.45, 0.8, np.abs(x)) \
        * (hash01(row + 5.0, col) > 0.6)
    return lines, split


def path_rings(points, radii, weights=None, per=3):
    """A limb's centre line made smooth: Catmull-Rom through points, `per` steps a span, radii (r or (r_ref, r_side))
    and weights ({bone: w} per point) blended along it, so a bent limb is round at the joint, not a kink."""
    pts = [Vector(p) for p in points]
    rr = [(r, r) if isinstance(r, (int, float)) else tuple(r) for r in radii]
    n = len(pts) - 1
    out_p, out_r, out_w = [], [], []
    for k in range(n):
        for s in range(per if k < n - 1 else per + 1):
            f = s / per
            t = (k + f) / n
            out_p.append(catmull(pts, t))
            e = f * f * (3.0 - 2.0 * f)
            out_r.append((lerp(rr[k][0], rr[k + 1][0], e), lerp(rr[k][1], rr[k + 1][1], e)))
            if weights is not None:
                a, b = weights[k], weights[k + 1]
                w = {bone: (1.0 - f) * a.get(bone, 0.0) + f * b.get(bone, 0.0) for bone in sorted(set(a) | set(b))}
                out_w.append({bone: v for bone, v in w.items() if v > 1e-3})
    return out_p, out_r, (out_w if weights is not None else None)


# --- A mesh being built ---

class Part:
    """One mesh in creature space: vertices with bone weights, faces with UVs into an atlas ({region: (u0, u1, v0, v1)} of
    a texture of size (w, h) pixels), kept INSET pixels inside each region so filtering and mips never pull in the next
    region's colour."""

    INSET = 3.0

    def __init__(self, name, atlas, size):
        self.name = name
        self.atlas = atlas
        self.size = size
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')
        self.weights = []
        self.w = None                            # the weights new vertices take

    def at(self, region, u, v):
        u0, u1, v0, v1 = self.atlas[region]
        iu = min(self.INSET / self.size[0], 0.2 * (u1 - u0))
        iv = min(self.INSET / self.size[1], 0.2 * (v1 - v0))
        u = min(max(u, 0.0), 1.0)
        v = min(max(v, 0.0), 1.0)
        return (u0 + iu + u * (u1 - u0 - 2.0 * iu), v0 + iv + v * (v1 - v0 - 2.0 * iv))

    def vert(self, p, weights=None):
        v = self.bm.verts.new(Vector(p))
        w = weights if weights is not None else self.w
        self.weights.append(dict(w) if w else None)
        return v

    def face(self, verts, uvs, out=None):
        """A face; with out (a direction), turned to face along it (the game draws one side)."""
        verts, uvs = list(verts), list(uvs)
        if out is not None and newell([v.co for v in verts]).dot(Vector(out)) < 0.0:
            verts.reverse()
            uvs.reverse()
        try:
            f = self.bm.faces.new(verts)
        except ValueError:
            return None
        f.smooth = True
        for loop, uv in zip(f.loops, uvs):
            loop[self.uv].uv = uv
        return f

    def finish(self, material, sharp=None, scale=1.0):
        """The part as a Blender object in meters (front to -Y), its weights as vertex groups, one material; scale sizes
        the whole creature at the end (its rig takes the same scale)."""
        bmesh.ops.transform(self.bm, matrix=lc.TO_BLENDER @ Matrix.Scale(scale, 4), verts=self.bm.verts)
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        mesh.materials.append(material)
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        if sharp:
            mesh.set_sharp_from_angle(angle=math.radians(sharp))
        groups = {}
        for i, ws in enumerate(self.weights):
            if not ws:
                continue
            total = sum(ws.values()) or 1.0
            for bone, w in sorted(ws.items()):
                if w <= 1e-3:
                    continue
                if bone not in groups:
                    groups[bone] = obj.vertex_groups.new(name=bone)
                groups[bone].add([i], w / total, 'REPLACE')
        col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
        col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))
        return obj


def join(name, objects):
    """Joins meshes (their weights and UVs kept) into the first, renamed."""
    with bpy.context.temp_override(active_object=objects[0], object=objects[0], selected_objects=objects,
                                   selected_editable_objects=objects):
        bpy.ops.object.join()
    obj = objects[0]
    obj.name = name
    obj.data.name = name
    return obj


# --- The sculpt a skin is cast onto ---

def element(center, semi, rot=(0.0, 0.0, 0.0), stiff=2.4):
    """One soft ellipsoid: its center, semi-axes (cm) and turn (yaw, pitch, roll in degrees)."""
    return (Vector(center), turn(*rot).to_quaternion(), tuple(semi), stiff)


class Sculpt:
    """A soft surface of ellipsoids (Hob.py's sculpt): a metaball, its mesh smoothed and kept as a BVH tree; the metaball
    is removed at once (metaballs of one name family would melt into each other)."""

    def __init__(self, elements, threshold=0.6, resolution=0.35, smoothing=3):
        mb = bpy.data.metaballs.new('_AnimalSculpt')
        mb.resolution = mb.render_resolution = resolution
        mb.threshold = threshold
        for center, quat, semi, stiff in elements:
            el = mb.elements.new(type='ELLIPSOID')
            vis = math.sqrt(1.0 - (threshold / stiff) ** (1.0 / 3.0))
            big = max(semi)
            el.co = center
            el.radius = big / vis
            el.size_x, el.size_y, el.size_z = [s / big for s in semi]
            el.rotation = quat
            el.stiffness = stiff
        obj = bpy.data.objects.new('_AnimalSculpt', mb)
        bpy.context.scene.collection.objects.link(obj)
        mesh = bpy.data.meshes.new_from_object(obj.evaluated_get(bpy.context.evaluated_depsgraph_get()))
        bpy.data.objects.remove(obj)
        bpy.data.metaballs.remove(mb)
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bpy.data.meshes.remove(mesh)
        bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.01)
        for _ in range(smoothing):
            bmesh.ops.smooth_vert(bm, verts=bm.verts, factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
        bm.normal_update()
        self.tree = BVHTree.FromBMesh(bm)
        bm.free()

    def cast(self, origin, direction, dist=60.0):
        """The first surface along a ray (point, normal) or (None, None)."""
        loc, normal, _, _ = self.tree.ray_cast(Vector(origin), Vector(direction).normalized(), dist)
        return loc, normal

    def nearest(self, point):
        loc, normal, _, _ = self.tree.find_nearest(Vector(point))
        return loc, normal


def cast_skin(part, sculpt, spine, ts, around=18, up_at=None, weights_at=None, region='skin', relax=8,
              v_range=(0.03, 0.97), caps=(True, True), cap_push=0.25, dist=60.0, shape=None, reach=None):
    """A skin of quads over the sculpt: rings of `around` at ts (0 = the spine's first point, the front; 1 = its last, the
    rear) along a Catmull-Rom spine, each vertex cast from the spine onto the sculpt, then relaxed over it. U runs
    around (0.5 toward up_at(t), the seam opposite), V along by the spine's length (the front at v_range[1]).
    weights_at(t, point) gives each ring vertex's bones; shape(i, j, point, center) may move a vertex before it's made.
    reach: a ray goes no farther than this many times the spine point's depth (its distance to the nearest surface);
    past it, the vertex is the nearest surface to a point just that deep along the ray. Where the body runs on along a
    ray (a chicken's head over its neck: the head's downward rays would run down the neck to the belly), it keeps the
    ring round its own part. Returns the skin's rings, centers, ts and Vs (the painters' landmarks are read off them)."""
    pts = [Vector(p) for p in spine]
    S, N = len(ts), around
    centers = [catmull(pts, t) for t in ts]
    lengths = [0.0]
    for a, b in zip(centers, centers[1:]):
        lengths.append(lengths[-1] + (b - a).length)
    total = lengths[-1] or 1.0
    vs = [v_range[1] - (v_range[1] - v_range[0]) * length / total for length in lengths]
    up_at = up_at or (lambda t: UP)
    rings = []
    t_prev = (centers[1] - centers[0]).normalized()
    ref = up_at(ts[0]) - t_prev * up_at(ts[0]).dot(t_prev)
    ref.normalize()
    for i, c in enumerate(centers):
        t = (centers[min(i + 1, S - 1)] - centers[max(i - 1, 0)]).normalized()
        ref = t_prev.rotation_difference(t) @ ref
        want = up_at(ts[i])
        want = (want - t * want.dot(t)).normalized()
        ref = (ref - t * ref.dot(t)).normalized().lerp(want, 0.5).normalized()
        side = t.cross(ref)
        ring = []
        depth = None
        if reach:
            near, _ = sculpt.nearest(c)
            depth = (near - c).length if near is not None else None
        for j in range(N):
            a = 2.0 * math.pi * j / N
            d = ref * math.cos(a) + side * math.sin(a)
            limit = min(dist, reach * depth) if depth else dist
            hit, _ = sculpt.cast(c, d, limit)
            if hit is None and depth:
                hit, _ = sculpt.nearest(c + d * depth * 1.1)
            ring.append(hit if hit is not None else c + d * 1.0)
        rings.append(ring)
        t_prev = t
    # Relax: each vertex toward its neighbours' mean, then back onto the sculpt, a few times.
    for _ in range(relax):
        new = []
        for i in range(S):
            row = []
            for j in range(N):
                p = rings[i][j]
                nb = [rings[i][(j + 1) % N], rings[i][(j - 1) % N]]
                if 0 < i < S - 1:
                    nb += [rings[i - 1][j], rings[i + 1][j]]
                mean = sum(nb, Vector()) / len(nb)
                hit, _ = sculpt.nearest(p.lerp(mean, 0.5))
                row.append(hit if hit is not None else p)
            new.append(row)
        rings = new
    if shape:
        rings = [[shape(i, j, rings[i][j], centers[i]) for j in range(N)] for i in range(S)]
    weights_at = weights_at or (lambda t, p: None)
    grid = [[part.vert(rings[i][j], weights_at(ts[i], rings[i][j])) for j in range(N)] for i in range(S)]

    def u_pair(j):
        ua, ub = 0.5 + j / N, 0.5 + (j + 1) / N
        if ua >= 1.0 - 1e-9:
            ua, ub = ua - 1.0, ub - 1.0
        return ua, ub
    for i in range(S - 1):
        mid = (centers[i] + centers[i + 1]) * 0.5
        for j in range(N):
            j1 = (j + 1) % N
            ua, ub = u_pair(j)
            quad = (grid[i][j], grid[i][j1], grid[i + 1][j1], grid[i + 1][j])
            uvs = [part.at(region, ua, vs[i]), part.at(region, ub, vs[i]), part.at(region, ub, vs[i + 1]),
                   part.at(region, ua, vs[i + 1])]
            center = sum((v.co for v in quad), Vector()) / 4.0
            part.face(quad, uvs, out=center - mid)
    for end, i, nxt, v_hub in ((0, 0, 1, 1.0), (1, S - 1, S - 2, 0.0)):
        if not caps[end]:
            continue
        c = sum(rings[i], Vector()) / N
        hub = part.vert(c + (centers[i] - centers[nxt]).normalized() * cap_push, weights_at(ts[i], c))
        for j in range(N):
            j1 = (j + 1) % N
            ua, ub = u_pair(j)
            part.face((hub, grid[i][j], grid[i][j1]),
                      [part.at(region, (ua + ub) * 0.5, v_hub), part.at(region, ua, vs[i]), part.at(region, ub, vs[i])],
                      out=centers[i] - centers[nxt])
    return dict(rings=rings, centers=centers, ts=ts, vs=vs, grid=grid, around=N)


def skin_landmark(skin, point):
    """The skin's (U, V) nearest a point, and the cm per unit of U (round the ring there) and of V (the skin's length).
    The nearest ring vertex first, then the nearest point within the quads round it (a ring vertex alone can be most of
    a centimetre off on a small head: a painted mark would miss the eye it rings)."""
    rings, N = skin['rings'], skin['around']
    target = Vector(point)
    best = None
    for i, ring in enumerate(rings):
        for j, p in enumerate(ring):
            d = (p - target).length
            if best is None or d < best[0]:
                best = (d, float(i), float(j))
    _, bi, bj = best
    S = len(rings)
    steps = 12
    for i in range(max(int(bi) - 1, 0), min(int(bi) + 1, S - 2) + 1):
        for j in (int(bj) - 1, int(bj)):
            a, b = rings[i][j % N], rings[i][(j + 1) % N]
            c, e = rings[i + 1][j % N], rings[i + 1][(j + 1) % N]
            for fi in range(steps + 1):
                for fj in range(steps + 1):
                    f, g = fi / steps, fj / steps
                    p = (a.lerp(b, g)).lerp(c.lerp(e, g), f)
                    d = (p - target).length
                    if d < best[0]:
                        best = (d, i + f, j + g)
    _, fi, fj = best
    i0 = min(int(fi), S - 2)
    vs = skin['vs']
    v = vs[i0] + (vs[i0 + 1] - vs[i0]) * (fi - i0)
    u = (0.5 + fj / N) % 1.0
    ring = rings[int(round(fi))]
    circumference = sum(((ring[k] - ring[(k + 1) % N]).length for k in range(N)))
    centers = skin['centers']
    length = sum(((a - b).length for a, b in zip(centers, centers[1:])))
    vr = vs[0] - vs[-1]
    return dict(u=u, v=v, cm_u=circumference, cm_v=length / max(vr, 1e-6))


def skin_table(skin):
    """The skin's rings for its painter: each ring's V (rising order) and its circumference (cm), and the cm along the
    skin per unit of V."""
    N = skin['around']
    circ = [sum(((ring[k] - ring[(k + 1) % N]).length for k in range(N))) for ring in skin['rings']]
    centers = skin['centers']
    length = sum(((a - b).length for a, b in zip(centers, centers[1:])))
    vs = list(skin['vs'])
    order = sorted(range(len(vs)), key=lambda k: vs[k])
    return dict(v=[vs[k] for k in order], circ=[circ[k] for k in order],
                cm_v=length / max(vs[0] - vs[-1], 1e-6))


def skin_rows(table, V, along_cm, across_cm):
    """Feather (or hair) rows down a skin from its front: along_cm(v) and across_cm(v) give their spacing (cm) at each V
    (array ops). Returns q (the row coordinate per pixel of V, rising toward the rear) and cols_at(row) (the feathers
    round that row, whole, so the seam under the belly closes), and v_of(row)."""
    vs = np.linspace(0.0, 1.0, 2001)
    dens = table['cm_v'] / np.maximum(along_cm(vs), 1e-3)
    cum = np.concatenate([[0.0], np.cumsum((dens[1:] + dens[:-1]) * 0.5 * np.diff(vs))])
    q_of_v = cum[-1] - cum                                   # rows from the front (V = 1) back to v
    circ = np.interp(vs, table['v'], table['circ'])
    cols_v = np.maximum(6.0, np.round(circ / np.maximum(across_cm(vs), 1e-3)))
    q_inc, v_inc = q_of_v[::-1], vs[::-1]

    def v_of(row):
        return np.interp(np.asarray(row, np.float64) + 0.5, q_inc, v_inc)

    def cols_at(row):
        return np.round(np.interp(v_of(row), vs, cols_v)).astype(np.float32)
    q = np.interp(V.astype(np.float64), vs, q_of_v).astype(np.float32)
    return q, cols_at, v_of


# --- Shapes ---

def tube(part, pts, radii, region, sides=8, weights=None, ref=None, caps=(True, True), u_range=(0.0, 1.0), cap_push=0.5,
         flat=None, v_range=(0.0, 1.0)):
    """A closed tube along pts (rings parallel-transported from ref, default up); radii per point: r, or (r_ref, r_side)
    for an oval. U along (u_range, by length), V around (v_range; its middle toward ref, the seam opposite). weights:
    per point (a list) or one dict. flat(k, a) may scale a ring's radius by direction angle a (0 toward ref)."""
    vr = lambda v: lerp(v_range[0], v_range[1], v)
    pts = [Vector(p) for p in pts]
    n = len(pts)
    radii = [(r, r) if isinstance(r, (int, float)) else tuple(r) for r in radii]
    dist = [0.0]
    for a, b in zip(pts, pts[1:]):
        dist.append(dist[-1] + (b - a).length)
    total = dist[-1] or 1.0
    us = [u_range[0] + (u_range[1] - u_range[0]) * d / total for d in dist]
    t_prev = (pts[1] - pts[0]).normalized()
    r0 = Vector(ref) if ref is not None else UP
    r0 = r0 - t_prev * r0.dot(t_prev)
    if r0.length < 1e-4:
        r0 = t_prev.orthogonal()
    r0.normalize()
    rings = []
    for k, p in enumerate(pts):
        w = weights[k] if isinstance(weights, list) else weights
        t = (pts[min(k + 1, n - 1)] - pts[max(k - 1, 0)]).normalized()
        r0 = t_prev.rotation_difference(t) @ r0
        r0 = (r0 - t * r0.dot(t)).normalized()
        side = t.cross(r0)
        ring = []
        for j in range(sides):
            a = 2.0 * math.pi * (j / sides - 0.5)
            scale = flat(k, a) if flat else 1.0
            ring.append(part.vert(p + (r0 * math.cos(a) * radii[k][0] + side * math.sin(a) * radii[k][1]) * scale, w))
        rings.append(ring)
        t_prev = t
    for k in range(n - 1):
        mid = (pts[k] + pts[k + 1]) * 0.5
        for j in range(sides):
            j1 = (j + 1) % sides
            quad = (rings[k][j], rings[k + 1][j], rings[k + 1][j1], rings[k][j1])
            uvs = [part.at(region, us[k], vr(j / sides)), part.at(region, us[k + 1], vr(j / sides)),
                   part.at(region, us[k + 1], vr((j + 1) / sides)), part.at(region, us[k], vr((j + 1) / sides))]
            center = sum((v.co for v in quad), Vector()) / 4.0
            part.face(quad, uvs, out=center - mid)
    for end, k, nxt in ((0, 0, 1), (1, n - 1, n - 2)):
        if not caps[end]:
            continue
        w = (weights[k] if isinstance(weights, list) else weights)
        d = (pts[k] - pts[nxt]).normalized()
        hub = part.vert(pts[k] + d * min(radii[k]) * cap_push, w)
        u_hub = us[k] + (0.02 if end == 0 else -0.02) * (u_range[1] - u_range[0])
        for j in range(sides):
            j1 = (j + 1) % sides
            part.face((hub, rings[k][j], rings[k][j1]), [part.at(region, u_hub, vr(0.5)), part.at(region, us[k], vr(j / sides)),
                                                         part.at(region, us[k], vr((j + 1) / sides))], out=d)
    return rings


def ellipsoid(part, center, radii, region, rot=None, segs=12, rings=8, weights=None, deform=None, patch=False,
              u_range=(0.0, 1.0), v_range=(0.0, 1.0)):
    """A smooth ellipsoid, its poles along rot's x (fwd): U around (0.5 toward rot's z, rising toward its y), V along (0
    the rear pole, 1 the front). deform(a, s, u) takes the unit sphere's point (along, left, up) and returns it changed.
    patch keeps the UVs in the middle fifth of the region (small parts of one colour)."""
    R = rot.to_3x3() if rot is not None else Matrix.Identity(3)
    c = Vector(center)
    if patch:
        u_range, v_range = (0.4, 0.6), (0.4, 0.6)

    def point(theta, phi):
        a, r = -math.cos(theta), math.sin(theta)
        s, u = r * math.sin(phi), r * math.cos(phi)
        if deform:
            a, s, u = deform(a, s, u)
        return c + R @ Vector((a * radii[0], s * radii[1], u * radii[2]))

    def uv(u, v):
        return part.at(region, lerp(u_range[0], u_range[1], u), lerp(v_range[0], v_range[1], v))
    w = weights
    rear = part.vert(point(0.0, 0.0), w)
    front = part.vert(point(math.pi, 0.0), w)
    grid = [[part.vert(point(math.pi * i / rings, 2.0 * math.pi * (j / segs - 0.5)), w) for j in range(segs)]
            for i in range(1, rings)]
    for j in range(segs):
        j1 = (j + 1) % segs
        ua, ub = j / segs, (j + 1) / segs
        quad_out = lambda vs: sum((v.co for v in vs), Vector()) / len(vs) - c
        tri = (rear, grid[0][j], grid[0][j1])
        part.face(tri, [uv((ua + ub) / 2, 0.0), uv(ua, 1 / rings), uv(ub, 1 / rings)], out=quad_out(tri))
        for i in range(rings - 2):
            va, vb = (i + 1) / rings, (i + 2) / rings
            quad = (grid[i][j], grid[i + 1][j], grid[i + 1][j1], grid[i][j1])
            part.face(quad, [uv(ua, va), uv(ua, vb), uv(ub, vb), uv(ub, va)], out=quad_out(quad))
        tri = (grid[-1][j], front, grid[-1][j1])
        part.face(tri, [uv(ua, 1 - 1 / rings), uv((ua + ub) / 2, 1.0), uv(ub, 1 - 1 / rings)], out=quad_out(tri))


def frame(along, normal):
    """An orthonormal (d, s, n): d along, n the normal made square to it, s = n x d (so d x s = n)."""
    d = Vector(along).normalized()
    n = Vector(normal)
    n = (n - d * n.dot(d))
    if n.length < 1e-6:
        n = d.orthogonal()
    n.normalize()
    return d, n.cross(d), n


def outline(shape, t, W, L=1.0):
    """A feather's (or a tuft's) half-width at t (0 root, 1 tip)."""
    if shape == 'flight':        # a quill, the vane widening, easing in to a long round tip
        f = (0.35 + 0.65 * smooth(0.0, 0.3, t)) * (1.0 - 0.12 * smooth(0.4, 0.8, t))
        tr = 0.72
    elif shape == 'primary':     # narrowing toward a round end
        f = (0.35 + 0.65 * smooth(0.0, 0.26, t)) * (1.0 - 0.3 * smooth(0.45, 0.65, t))
        tr = 0.72
    elif shape == 'pointed':     # hackles, saddle feathers, tufts
        return 0.5 * W * max(0.1, math.sin(math.pi * min(0.1 + 0.9 * t, 1.0) ** 0.75) ** 0.8)
    elif shape == 'blade':       # a long tail feather or sickle: even width, a round tip
        f = 0.55 + 0.45 * smooth(0.0, 0.12, t)
        tr = 0.85
    else:                        # 'round': coverts, contour feathers (a broad, well rounded end)
        f = 0.62 + 0.38 * smooth(0.0, 0.3, t)
        tr = 0.48
    if t > tr:
        k = (t - tr) / (1.0 - tr)
        f *= math.sqrt(max(0.0, 1.0 - k * k))
    return 0.5 * W * f


def card(part, region, strip, rows, point, two_sided=False, thick=0.05, normal=None, weigh=None, shape='round', W=1.0,
         tip=True, shaft=False):
    """One feather or tuft as shaped geometry: point(t, x, layer) gives its vertex t along (0 root, 1 tip), x across (cm
    from the shaft) on its top (layer 0) or its underside (layer 1, thick below), so a caller can lay it in any frame
    (straight, curled, on a wing bed). rows: the t of each row; the tip is one vertex (or, with tip=False, the last row is
    its end). strip (u0, u1): the part of the region across it. normal(t): the side its top faces (else worked out from
    the first row). weigh(t): its bones along it."""
    layers = (0, 1) if two_sided else (0,)
    for layer in layers:
        grid = []
        for t in rows:
            w = outline(shape, t, W)
            xs = (-w, 0.0, w) if shaft else (-w, w)
            grid.append([(part.vert(point(t, x, layer), weigh(t) if weigh else None), t, x) for x in xs])

        def uv(t, x):
            return part.at(region, lerp(strip[0], strip[1], 0.5 + 0.5 * x / (0.5 * W)), t)
        for r0, r1 in zip(grid, grid[1:]):
            for k in range(len(r0) - 1):
                quad = (r0[k][0], r1[k][0], r1[k + 1][0], r0[k + 1][0])
                uvs = [uv(r0[k][1], r0[k][2]), uv(r1[k][1], r1[k][2]), uv(r1[k + 1][1], r1[k + 1][2]),
                       uv(r0[k + 1][1], r0[k + 1][2])]
                out = normal(r0[k][1]) if normal else None
                if out is not None and layer == 1:
                    out = -out
                part.face(quad, uvs, out=out)
        if tip:
            last = grid[-1]
            end = part.vert(point(1.0, 0.0, layer), weigh(1.0) if weigh else None)
            for k in range(len(last) - 1):
                tri = (last[k][0], end, last[k + 1][0])
                out = normal(1.0) if normal else None
                if out is not None and layer == 1:
                    out = -out
                part.face(tri, [uv(last[k][1], last[k][2]), uv(1.0, 0.0), uv(last[k + 1][1], last[k + 1][2])], out=out)


def straight(root, along, normal, L, lift=0.0, curl=0.0, bend=0.0, twist=0.0, thick=0.05, cup=0.0, half=1.0):
    """A point function for card(): a feather from root along `along` for L cm, its top facing normal; lift raises it
    evenly toward the tip, curl (cm at the tip, negative: down) curves it, bend sideways (toward s, cm at the tip), twist
    (degrees at the tip) turns the vane about its shaft, cup lowers its edges (cm at the half-width `half`)."""
    d, s, n = frame(along, normal)
    root = Vector(root)

    def point(t, x, layer):
        a = math.radians(twist) * t
        h = lift * t + curl * t * t - cup * (x / half) ** 2
        p = root + d * (t * L) + s * (x * math.cos(a) + bend * t * t) + n * (h + x * math.sin(a))
        if layer == 1:
            p -= n * thick
        return p
    return point, (lambda t: n)


def hug(point, sculpt, lift, thick=0.05):
    """A card's point function laid onto the sculpt: each point moved to the nearest surface and lifted off it by
    lift(t), so a long feather drapes over the body (hackles over the shoulders, saddle feathers down the rump) instead of
    standing off it in a straight line. Returns the point function and the normal along it."""
    def p(t, x, layer):
        q = point(t, x, 0)
        near, n = sculpt.nearest(q)
        if near is None:
            return q
        return near + n * (lift(t) - (thick if layer else 0.0))

    def normal(t):
        _, n = sculpt.nearest(point(t, 0.0, 0))
        return n if n is not None else UP
    return p, normal


class WeightField:
    """Bone weights by place, read off a weighted part's vertices (as built, before finish()): the inverse-distance blend
    of the nearest few. Cards and tufts take their weights from the skin under them this way."""

    def __init__(self, part):
        self.kd = KDTree(len(part.bm.verts))
        for i, v in enumerate(part.bm.verts):
            if part.weights[i]:
                self.kd.insert(v.co, i)
        self.kd.balance()
        self.weights = part.weights

    def at(self, point, k=4):
        total, wsum = {}, 0.0
        for _, i, dist in self.kd.find_n(Vector(point), k):
            w = 1.0 / (dist + 0.4)
            wsum += w
            for bone, x in self.weights[i].items():
                total[bone] = total.get(bone, 0.0) + x * w
        return {b: x / wsum for b, x in total.items() if x / wsum > 1e-3}


# --- The rig and its poses ---

class Rig:
    """An armature from specs [(bone, head, tail, parent)] in creature space (parents first), posed the way the game's
    code poses it: each bone turned about its head, in creature axes, under its parent's turn."""

    def __init__(self, specs, scale=1.0):
        """scale: the creature built in one size and made in another (its Part finished at the same scale). Poses are
        worked out in the built size; the armature, its posing and the pose table are at the made size."""
        self.order = [s[0] for s in specs]
        self.head = {s[0]: Vector(s[1]) for s in specs}
        self.tail = {s[0]: Vector(s[2]) for s in specs}
        self.parent = {s[0]: s[3] for s in specs}
        self.scale = scale
        self.arm = lm.armature()
        lm.bones(self.arm, [(n, lc.to_blender(Vector(h) * scale), lc.to_blender(Vector(t) * scale), p) for n, h, t, p in specs])

    def bind(self, obj):
        obj.parent = self.arm
        obj.modifiers.new('Armature', 'ARMATURE').object = self.arm

    def solve(self, pose):
        """Each bone's transform from rest (4x4, creature space). pose: {bone: (yaw, pitch, roll) degrees or a 4x4
        rotation, about the bone's head under its parent's turn; '<bone>@move': (x, y, z) cm, a shift before it}."""
        D = {}
        for name in self.order:
            spec = pose.get(name)
            if spec is None:
                R = Matrix.Identity(4)
            elif isinstance(spec, Matrix):
                R = spec.to_4x4()
            else:
                R = turn(*spec)
            local = rot_about(self.head[name], R)
            move = pose.get(name + '@move')
            if move:
                local = Matrix.Translation(Vector(move)) @ local
            p = self.parent[name]
            D[name] = (D[p] @ local) if p else local
        return D

    def aim(self, pose, bone, want):
        """Sets pose[bone] so the bone points along want (creature space), under its parent's turn in the pose."""
        D = self.solve(pose)
        p = self.parent[bone]
        above = D[p].to_3x3() if p else Matrix.Identity(3)
        rest = (self.tail[bone] - self.head[bone]).normalized()
        local = above.inverted() @ Vector(want).normalized()
        pose[bone] = rest.rotation_difference(local).to_matrix().to_4x4()
        return pose

    def level(self, pose, bone):
        """Sets pose[bone] so the bone keeps its rest orientation in the world (a foot flat on the ground)."""
        D = self.solve(pose)
        p = self.parent[bone]
        pose[bone] = (D[p].to_3x3().inverted() if p else Matrix.Identity(3)).to_4x4()
        return pose

    def where(self, pose, bone, point=None):
        """Where a bone's head (or a point carried by it, rest creature space) is in a pose."""
        return self.solve(pose)[bone] @ (self.head[bone] if point is None else Vector(point))

    def plant(self, pose, upper, lower, target, pole, flat=()):
        """Two-bone IK (the game's, looter_creatures.solve_two_bone): upper and lower are turned so lower's tail (the
        ankle) reaches target, the joint bending toward pole; flat bones (toes) then keep their rest orientation."""
        D = self.solve(pose)
        root = D[upper] @ self.head[upper]
        a = (self.head[lower] - self.head[upper]).length
        b = (self.tail[lower] - self.head[lower]).length
        joint, end = lc.solve_two_bone(root, Vector(target), a, b, Vector(pole))
        self.aim(pose, upper, joint - root)
        self.aim(pose, lower, end - joint)
        for bone in flat:
            self.level(pose, bone)
        return pose

    def apply(self, pose):
        D = self.solve(pose)
        T = lc.TO_BLENDER @ Matrix.Scale(self.scale, 4)
        Ti = T.inverted()
        bones = self.arm.data.bones
        posed = {}
        for name in self.order:
            bone = bones[name]
            rest = bone.matrix_local
            world = T @ D[name] @ Ti @ rest
            posed[name] = world
            if bone.parent:
                basis = (posed[bone.parent.name] @ bone.parent.matrix_local.inverted() @ rest).inverted() @ world
            else:
                basis = rest.inverted() @ world
            self.arm.pose.bones[name].matrix_basis = basis
        bpy.context.view_layer.update()
        return D

    def table(self, poses, notes):
        """The poses for the game's code, in Unreal's component space (cm; x forward, y right, z up), as Amos's pose table
        (Tools/amos_poses.py) has them: each bone's rest head, its head in the pose, its whole turn from rest and its own
        turn under its parent's (Turned = Above * Own), as FQuat (X, Y, Z, W)."""
        F = Matrix.Diagonal((1.0, -1.0, 1.0))
        k = self.scale

        def ue(v):
            return [round(k * v.x, 2) + 0.0, round(-k * v.y, 2) + 0.0, round(k * v.z, 2) + 0.0]

        def quat(R):
            q = (F @ R @ F).to_quaternion()
            return [round(q.x, 5) + 0.0, round(q.y, 5) + 0.0, round(q.z, 5) + 0.0, round(q.w, 5) + 0.0]
        out = dict(units='Unreal component space at scale 1: cm; x forward, y right, z up (the mesh origin on the ground)',
                   quaternions="FQuat (X, Y, Z, W); turn = the whole turn from rest; own = the turn under the parent's "
                               "(Turned = Above * Own)", bones=self.order, parents=self.parent,
                   rest={n: ue(self.head[n]) for n in self.order}, notes=notes, poses={})
        for name, spec in poses.items():
            D = self.solve(spec)
            bones = {}
            for bone in self.order:
                R = D[bone].to_3x3()
                p = self.parent[bone]
                own = (D[p].to_3x3().inverted() @ R) if p else R
                bones[bone] = dict(head=ue(D[bone] @ self.head[bone]), turn=quat(R), own=quat(own))
            out['poses'][name] = bones
        return out


def write_poses(name, table):
    os.makedirs(POSE_DIR, exist_ok=True)
    path = os.path.join(POSE_DIR, f'{name}_poses.json')
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(table, f, indent=1)
    log(f'{name}: pose table {path}')
    return path


def write_report(script, report):
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    path = os.path.join(PREVIEW_DIR, f'{script}_report.json')
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=1)
    return path


# --- Previews: Ransom's Rest's golden late afternoon (homes' stage), posed copies, renders ---

class Studio:
    """The renders' set: homes.stage()'s dirt ground, sun and golden sky; props and posed copies are '_' objects, never
    exported, cleared between shots."""

    def __init__(self, samples=64):
        self.scene, self.sun = homes.stage(samples=samples)
        self.temp = []
        self.sun_dir = homes.golden_sun(38.0, 16.0)
        homes.light(self.scene, self.sun, self.sun_dir)

    def keep(self, obj):
        if obj.name not in self.scene.collection.objects:
            self.scene.collection.objects.link(obj)
        self.temp.append(obj)
        return obj

    def clear(self):
        for o in self.temp:
            if o.name in bpy.data.objects:
                bpy.data.objects.remove(o, do_unlink=True)
        self.temp = []

    def light(self, azimuth=38.0, elevation=16.0, strength=homes.SUN_STRENGTH):
        self.sun_dir = homes.golden_sun(azimuth, elevation)
        homes.light(self.scene, self.sun, self.sun_dir, strength=strength)

    def snapshot(self, rig, skin, pose, at=(0.0, 0.0, 0.0), yaw=0.0, name='Snap'):
        """A copy of the skin as posed, placed at `at` (Blender meters) turned yaw degrees; the rig goes back to rest."""
        rig.apply(pose)
        dg = bpy.context.evaluated_depsgraph_get()
        mesh = bpy.data.meshes.new_from_object(skin.evaluated_get(dg))
        obj = self.keep(bpy.data.objects.new('_' + name, mesh))
        obj.matrix_world = Matrix.Translation(Vector(at)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z')
        rig.apply({})
        return obj

    def copy(self, obj, at=(0.0, 0.0, 0.0), yaw=0.0, name=None):
        """A linked copy of a static model placed at `at` (meters), turned yaw degrees."""
        c = obj.copy()
        c.name = '_' + (name or obj.name)
        for child in list(c.children):
            child.parent = None
        c.parent = None
        c.hide_render = False                    # (the originals are hidden from the renders; a copy takes the flag)
        self.keep(c)
        c.matrix_world = Matrix.Translation(Vector(at)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z')
        return c

    def shoot(self, name, objs, direction, lens=50.0, size=(1600, 1000), margin=1.12, height_bias=0.42, exposure=-0.1,
              eye=None, target=None):
        """Renders objs framed from direction (toward the camera) into PREVIEW_DIR/<name>.png."""
        if eye is None:
            eye, target = homes.frame_shot(objs, direction, size, lens, margin=margin, height_bias=height_bias)
        path = os.path.join(PREVIEW_DIR, name + '.png')
        return homes.shoot(self.scene, path, eye, target, lens, size, exposure)


def hide(objects, hidden=True):
    for o in objects:
        o.hide_render = hidden
        for c in o.children_recursive:
            c.hide_render = hidden


def backface_culling(materials):
    """Eevee culls back faces as Unreal does (the materials are one-sided), so the previews show what the game shows."""
    for mat in materials:
        mat.use_backface_culling = True


def plank_deck(studio, x0, x1, y0, y1, top, seed=3, name='Deck'):
    """A weathered plank porch floor ('_', never exported) on the shared WoodPlanks set, its top at `top` (m)."""
    import looter_props as lp
    deck = lp.block((x1 - x0, y1 - y0, top), ((x0 + x1) * 0.5, (y0 + y1) * 0.5, top * 0.5), bevel=0.01)
    lt.assign(deck, lt.material('WoodPlanks', name='_PreviewPlanks'))
    lt.box_uv(deck, 'WoodPlanks', seed=seed, along='long')
    deck.name = '_' + name
    return studio.keep(deck)


def rock_block(studio, size, center, seed=5, name='Rock'):
    """A block of cliff rock ('_', never exported) on the RockCliff set."""
    import looter_props as lp
    rock = lp.block(size, center, bevel=0.04)
    lt.assign(rock, lt.material('RockCliff', name='_PreviewRock'))
    lt.box_uv(rock, 'RockCliff', seed=seed)
    rock.name = '_' + name
    return studio.keep(rock)
