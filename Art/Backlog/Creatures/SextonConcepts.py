"""Mister Sexton, the game's first human (Docs/Story.md, "Antagonist: Mister Sexton"; Docs/Areas/RansomsRest.md, the
NPCs table and step 15): three concept directions for the user to choose from (2026-10-06). Concept art kept in
Art/Backlog (see its README): nothing in the game uses it.

He is a seated static mesh with a posed idle only: on the railing of Ransom's Point (the Lookout's SOCKET_Sit, on the
front rail's top, 1.05 m over the deck, facing into the deck), legs crossed, the ledger open on his knee, one hand on
it. His face stays in shadow: no facial animation. Each direction is a different silhouette:

  A  Gentleman   very tall and gaunt (about 2.15 m standing): an exaggerated stovepipe, a fitted black frock coat
                 buttoned to the throat, a white wing collar and a black bow tie, long legs crossed knee over knee, the
                 coat's tails hanging over the rail behind him. Long pale bare fingers spread on the page, a dip pen in
                 the other hand. The shadow covers his face past the mouth; only the point of a pale chin catches the
                 light. A tarnished coin weights his ledger's ribbon.
  B  Undertaker  older and rounder (about 1.95 m), kindly and sinister: a lower, wide-brimmed top hat with a deep
                 crepe band, a caped greatcoat (two shoulder capes make a bell), a dove-grey waistcoat with a gold
                 watch chain, the watch dangling from grey-gloved fingers, legs crossed ankle on knee. Below the
                 shadow, white side-whiskers and a soft chin; in it, two round gold spectacle rims catch the light,
                 like coins on a dead man's eyes.
  C  Ferryman    a high-collared, near-cloak coat: a funnel collar up to his nose (no face at all), a long cape over a
                 coat whose ragged skirt hangs nearly to the deck behind the rail, sea boots, a battered, leaning
                 stovepipe banded with tarnished old coins, a coin clasp, black gloves, and a furled umbrella as long
                 as a punt pole in his hand (he smells like rain).

Everything is built in Sexton space: meters, the origin on the top rail's top under his seat (the Lookout's
SOCKET_Sit), his front toward -Y (into the deck in the game), his left +X, up +Z; the deck is 1.05 m below. Bodies
and clothes are smooth lofts and lathes on subdivision surfaces. The coats' skirts and the capes are draped over the
body and the rail by Blender's cloth simulation (skirts from their waist seam, capes from a fitted bell that only
settles); a drape is cached in Intermediate/SextonConcepts under a digest of its inputs, so runs repeat it exactly
(--resim runs it again). Materials are the game's texture sets, tinted: Polymer for cloth, skin, hair and leather
(cloth matte, as an instance with the Default set's ORM would be), MetalWorn for coins, chain, buttons and rims,
GunWood for the pen and the umbrella's crook. The face's shadow is a material drawn unlit near-black, and the skin
below it darkens toward its edge by vertex alpha (M_World's baked-occlusion channel, applied in full with DiffuseAO
1), so shadow and skin meet without a seam. The ledger's ink lines are the one preview-only touch.

Renders (Saved/ArtPreviews/RansomsRest/Concepts/Sexton/):
  Sexton_<X>_Hero.png        three-quarter view on the real Lookout (Art/Models/Buildings/Lookout.py) in a low golden
                             sun, from a standing player's eye height on the deck
  Sexton_<X>_Turnaround.png  a 1.8 m figure, then front, side and back on a stand-in of the Lookout's front rail
  Sexton_<X>_Dusk.png        the cold open: his seated model drawn black on the rail, seen low from the deck, against
                             the game's Dusk state (the sun 4 degrees up at a bearing of 252 degrees, a violet haze
                             glowing amber toward the sun, dusky pink clouds, darkened warm ridges)
  Sexton_Comparison.png      the three side by side on one rail, in the same light and at the same scale, with the
                             1.8 m figure

Run it through Tools\\artrun.ps1 (it waits while the main session measures performance); PowerShell wants the
arguments as one comma-separated list:
  artrun.ps1 -Script Art\\Backlog\\Creatures\\SextonConcepts.py -ScriptArgs --preview,A,B,C     everything
  ... -ScriptArgs --preview,A,hero,turn     only option A's hero and turnaround (options A B C; renders hero turn dusk
                                            compare); --clay grey clay (forms only); --quick fewer samples; close
                                            (with SEXTON_SCRATCH set) checking close-ups instead of the deliverables
"""
import math
import os
import sys
import time

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_textures as lt

REPO = lt.REPO
OUT_DIR = os.path.join(REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'Sexton')
FONT = os.path.join(REPO, 'Content', 'UI', 'Fonts', 'ChakraPetch-Bold.ttf')
ARGS = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
CLAY = '--clay' in ARGS
QUICK = '--quick' in ARGS
DECK = -1.05              # the deck's height under the rail's top (the Lookout's RAIL)
UP = Vector((0.0, 0.0, 1.0))
FRONT = Vector((0.0, -1.0, 0.0))
T0 = time.time()


def log(message):
    print(f'SEXTON [{time.time() - T0:6.1f}s] {message}', flush=True)


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def V(x, y=None, z=None):
    return Vector(x) if y is None else Vector((x, y, z))


def nrm(v):
    return Vector(v).normalized()


def _hash(ix, iy, iz, seed):
    h = (ix * 374761393 + iy * 668265263 + iz * 1442695041 + seed * 3266489917) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((h ^ (h >> 16)) & 0xFFFF) / 32767.5 - 1.0


def vnoise(x, y=0.0, z=0.0, seed=0):
    """Smooth value noise in -1..1 (deterministic: no mathutils.noise)."""
    ix, iy, iz = math.floor(x), math.floor(y), math.floor(z)
    fx, fy, fz = x - ix, y - iy, z - iz
    ux, uy, uz = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy), fz * fz * (3 - 2 * fz)

    def h(a, b, c):
        return _hash(ix + a, iy + b, iz + c, seed)
    x00 = lerp(h(0, 0, 0), h(1, 0, 0), ux)
    x10 = lerp(h(0, 1, 0), h(1, 1, 0), ux)
    x01 = lerp(h(0, 0, 1), h(1, 0, 1), ux)
    x11 = lerp(h(0, 1, 1), h(1, 1, 1), ux)
    return lerp(lerp(x00, x10, uy), lerp(x01, x11, uy), uz)


# --- Paths and frames ---

def catmull(points, per=6):
    """A Catmull-Rom spline through points, per samples a span (the end points kept)."""
    P = [Vector(p) for p in points]
    if len(P) == 2:
        return [P[0].lerp(P[1], k / per) for k in range(per + 1)]
    out = []
    for i in range(len(P) - 1):
        p0 = P[i - 1] if i > 0 else P[i] + (P[i] - P[i + 1])
        p1, p2 = P[i], P[i + 1]
        p3 = P[i + 2] if i + 2 < len(P) else P[i + 1] + (P[i + 1] - P[i])
        for k in range(per):
            t = k / per
            t2, t3 = t * t, t * t * t
            out.append(0.5 * ((2.0 * p1) + (-p0 + p2) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
                              (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3))
    out.append(P[-1].copy())
    return out


def resample(points, count):
    """count points evenly spaced by arc length along a polyline."""
    P = [Vector(p) for p in points]
    dist = [0.0]
    for a, b in zip(P, P[1:]):
        dist.append(dist[-1] + (b - a).length)
    total = dist[-1]
    out = []
    k = 0
    for i in range(count):
        d = total * i / (count - 1)
        while k < len(P) - 2 and dist[k + 1] < d:
            k += 1
        span = max(dist[k + 1] - dist[k], 1e-9)
        out.append(P[k].lerp(P[k + 1], min(max((d - dist[k]) / span, 0.0), 1.0)))
    return out


def smooth_path(points, count, per=6):
    return resample(catmull(points, per), count)


def rm_frames(path, up):
    """Rotation-minimizing frames (tangent, normal, side = tangent x normal) along path, the first normal as close to
    up as it can be."""
    n = len(path)
    tangents = []
    for k in range(n):
        a, b = path[max(k - 1, 0)], path[min(k + 1, n - 1)]
        tangents.append((b - a).normalized())
    up = Vector(up)
    normal = up - tangents[0] * up.dot(tangents[0])
    if normal.length < 1e-6:
        normal = tangents[0].orthogonal()
    normal.normalize()
    frames = []
    for k in range(n):
        if k:
            normal = tangents[k - 1].rotation_difference(tangents[k]) @ normal
            normal = (normal - tangents[k] * normal.dot(tangents[k])).normalized()
        frames.append((tangents[k], normal, tangents[k].cross(normal)))
    return frames


def arc_params(path):
    dist = [0.0]
    for a, b in zip(path, path[1:]):
        dist.append(dist[-1] + (b - a).length)
    total = max(dist[-1], 1e-9)
    return [d / total for d in dist], total


def keys(table, t):
    """Smoothly interpolates a list of (t, value or tuple) keys at t."""
    if t <= table[0][0]:
        return table[0][1]
    for (t0, a), (t1, b) in zip(table, table[1:]):
        if t <= t1:
            f = smoothstep(0.0, 1.0, (t - t0) / max(t1 - t0, 1e-9))
            if isinstance(a, (tuple, list)):
                return tuple(lerp(x, y, f) for x, y in zip(a, b))
            return lerp(a, b, f)
    return table[-1][1]


def superellipse(phi, rx, ry_pos, ry_neg=None, n=2.0):
    """The point at angle phi (0 = +y, 90 degrees = +x) on a superellipse: half-width rx along x, ry_pos toward +y,
    ry_neg toward -y (default ry_pos), exponent n (2 = an ellipse; more is boxier)."""
    c, s = math.cos(phi), math.sin(phi)
    e = 2.0 / n
    x = rx * math.copysign(abs(s) ** e, s)
    ry = ry_pos if c >= 0.0 else (ry_pos if ry_neg is None else ry_neg)
    return x, ry * math.copysign(abs(c) ** e, c)


def bump(d, width):
    """A smooth bump: 1 at d = 0, 0 beyond width."""
    x = abs(d) / width
    return 0.0 if x >= 1.0 else (1.0 - x * x) ** 2


def wrap(a):
    """An angle into -pi..pi."""
    return (a + math.pi) % (2.0 * math.pi) - math.pi


# --- Meshes ---

COLLECTION = None
PARTS = []
SHADE = {}      # object name -> vertex alpha per face corner (the face's shadow fading into the skin)


class Part:
    """One mesh being built (several lofts can go into it), finished into a smooth-shaded object."""

    def __init__(self, name):
        self.name = name
        self.bm = bmesh.new()
        self.slots = []

    def slot(self, key):
        if key not in self.slots:
            self.slots.append(key)
        return self.slots.index(key)

    def finish(self, subsurf=2, solidify=0.0, solid_offset=-1.0, collide=False):
        mesh = bpy.data.meshes.new(self.name)
        self.bm.normal_update()
        self.bm.to_mesh(mesh)
        self.bm.free()
        obj = bpy.data.objects.new(self.name, mesh)
        COLLECTION.objects.link(obj)
        for p in mesh.polygons:
            p.use_smooth = True
        obj['slots'] = ','.join(self.slots)
        if solidify:
            mod = obj.modifiers.new('Solidify', 'SOLIDIFY')
            mod.thickness = solidify
            mod.offset = solid_offset
            mod.use_even_offset = False
            mod.use_quality_normals = False
        if subsurf:
            mod = obj.modifiers.new('Subsurf', 'SUBSURF')
            mod.levels = 1
            mod.render_levels = subsurf
            mod.quality = 3
        if collide:
            obj['collide'] = 1
        PARTS.append(obj)
        return obj


def loft(part, path, up, section, segs=16, caps=(None, None), mat='coat', pole_ext=(0.0, 0.0), face_mat=None,
         arc=None):
    """Sweeps a ring of segs points along path (a list of points). section(t, phi) -> (x, y): the ring point's offset
    at the path's arc-length parameter t (0..1) and angle phi (0 = toward the frame's normal, which starts as close to
    up as it can; increasing toward its side = tangent x normal), x along side, y along normal. caps: None (open) or
    'pole' (closed in a point pole_ext past that end). face_mat(t, phi) -> a material key per face. arc=(phi0, phi1)
    makes an open strip over those angles instead of a closed ring. Returns the rings."""
    bm = part.bm
    frames = rm_frames(path, up)
    ts, _ = arc_params(path)
    closed = arc is None
    count = segs if closed else segs + 1
    angles = [2.0 * math.pi * j / segs for j in range(segs)] if closed else \
        [lerp(arc[0], arc[1], j / segs) for j in range(segs + 1)]
    rings = []
    for p, (tan, nr, side), t in zip(path, frames, ts):
        ring = []
        for phi in angles:
            xy = section(t, phi)
            # A third value moves the point along the path (a collar lower at its front, a wing tip folding down).
            ring.append(bm.verts.new(p + side * xy[0] + nr * xy[1] + (tan * xy[2] if len(xy) > 2 else Vector())))
        rings.append(ring)
    default = part.slot(mat)
    step = (2.0 * math.pi / segs) if closed else (arc[1] - arc[0]) / segs
    for k in range(len(rings) - 1):
        a, b = rings[k], rings[k + 1]
        for j in range(segs if closed else count - 1):
            j1 = (j + 1) % count
            f = bm.faces.new((a[j], a[j1], b[j1], b[j]))
            if face_mat is not None:
                f.material_index = part.slot(face_mat((ts[k] + ts[k + 1]) * 0.5, angles[j] + step * 0.5))
            else:
                f.material_index = default
            f.smooth = True
    for end, cap in enumerate(caps):
        if cap is None or not closed:
            continue
        ring = rings[0] if end == 0 else rings[-1]
        tan = frames[0][0] if end == 0 else frames[-1][0]
        center = sum((v.co for v in ring), Vector()) / segs
        pole = bm.verts.new(center + tan * (pole_ext[end] if end else -pole_ext[end]))
        index = default
        if face_mat is not None:
            index = part.slot(face_mat(0.0 if end == 0 else 1.0, 0.0))
        for j in range(segs):
            j1 = (j + 1) % segs
            f = bm.faces.new((ring[j1], ring[j], pole) if end == 0 else (ring[j], ring[j1], pole))
            f.material_index = index
            f.smooth = True
    return rings


def tube(part, points, radius, segs=8, mat='iron', caps=('pole', 'pole'), up=UP, per=4, radius_end=None, count=None):
    """A round tube through points (smoothed): straps, chains, pens, shafts."""
    path = catmull(points, per) if len(points) > 2 else resample(points, per + 1)
    if count:
        path = resample(path, count)
    r1 = radius if radius_end is None else radius_end
    return loft(part, path, up, lambda t, phi: (lerp(radius, r1, t) * math.sin(phi), lerp(radius, r1, t) * math.cos(phi)),
                segs=segs, caps=caps, mat=mat, pole_ext=(radius * 0.4, r1 * 0.4))


def ellipsoid(part, center, radii, frame3=None, segs=16, rings=10, mat='coat'):
    """An ellipsoid, radii along frame3's columns."""
    frame3 = frame3 or Matrix.Identity(3)
    m = Matrix.Translation(Vector(center)) @ frame3.to_4x4()
    path = [m @ Vector((0.0, 0.0, -radii[2] * math.cos(math.pi * i / rings))) for i in range(1, rings)]
    up = frame3 @ Vector((0.0, 1.0, 0.0))

    def section(t, phi):
        z = -math.cos(math.pi * lerp(1.0 / rings, 1.0 - 1.0 / rings, t))
        r = math.sqrt(max(1.0 - z * z, 0.0))
        return r * radii[0] * math.sin(phi), r * radii[1] * math.cos(phi)
    loft(part, path, up, section, segs=segs, caps=('pole', 'pole'), mat=mat,
         pole_ext=(radii[2] * (1.0 - math.cos(math.pi / rings)), radii[2] * (1.0 - math.cos(math.pi / rings))))


def box(part, center, size, frame3=None, mat='wood'):
    frame3 = frame3 or Matrix.Identity(3)
    m = Matrix.Translation(Vector(center)) @ frame3.to_4x4() @ Matrix.Diagonal((size[0], size[1], size[2], 1.0))
    res = bmesh.ops.create_cube(part.bm, size=1.0, matrix=m)
    slot = part.slot(mat)
    for f in {f for v in res['verts'] for f in v.link_faces}:
        f.material_index = slot


def disc(part, center, normal, radius, thick, segs=20, mat='gold', taper=0.92):
    """A coin or button: a short closed cylinder, its axis along normal."""
    n = Vector(normal).normalized()
    a = n.orthogonal().normalized()
    m = Matrix.Translation(Vector(center)) @ Matrix((a, n.cross(a), n)).transposed().to_4x4()
    res = bmesh.ops.create_cone(part.bm, cap_ends=True, segments=segs, radius1=radius, radius2=radius * taper,
                                depth=thick, matrix=m)
    slot = part.slot(mat)
    for f in {f for v in res['verts'] for f in v.link_faces}:
        f.material_index = slot
        f.smooth = True


def torus(part, center, normal, radius, minor, segs=24, minor_segs=8, mat='gold'):
    n = Vector(normal).normalized()
    a = n.orthogonal().normalized()
    b = n.cross(a)
    pts = [Vector(center) + (a * math.cos(2 * math.pi * k / segs) + b * math.sin(2 * math.pi * k / segs)) * radius
           for k in range(segs)]
    bm = part.bm
    slot = part.slot(mat)
    rings = []
    for k, p in enumerate(pts):
        out = (p - Vector(center)).normalized()
        rings.append([bm.verts.new(p + (out * math.cos(2 * math.pi * j / minor_segs) + n * math.sin(2 * math.pi * j / minor_segs)) * minor)
                      for j in range(minor_segs)])
    for k in range(segs):
        k1 = (k + 1) % segs
        for j in range(minor_segs):
            j1 = (j + 1) % minor_segs
            f = bm.faces.new((rings[k][j], rings[k1][j], rings[k1][j1], rings[k][j1]))
            f.material_index = slot
            f.smooth = True


def surface_hit(obj, origin, direction, distance=3.0):
    """The point and normal where a ray meets obj's evaluated surface (world space), or None."""
    dg = bpy.context.evaluated_depsgraph_get()
    tree = BVHTree.FromObject(obj, dg)
    inv = obj.matrix_world.inverted()
    o = inv @ Vector(origin)
    d = (inv.to_3x3() @ Vector(direction)).normalized()
    hit, normal, _, _ = tree.ray_cast(o, d, distance)
    if hit is None:
        return None
    return obj.matrix_world @ hit, (obj.matrix_world.to_3x3().inverted().transposed() @ normal).normalized()


# --- Hands ---

FINGERS = ('index', 'middle', 'ring', 'little')
FINGER_LEN = {'index': (0.044, 0.027, 0.023), 'middle': (0.048, 0.031, 0.025), 'ring': (0.046, 0.030, 0.024),
              'little': (0.035, 0.022, 0.021)}
FINGER_AT = {'index': (0.031, 0.0), 'middle': (0.010, 0.004), 'ring': (-0.011, -0.001), 'little': (-0.029, -0.011)}
FINGER_R = {'index': 0.0092, 'middle': 0.0095, 'ring': 0.0088, 'little': 0.0078}
POSES = {
    # A hand spread on a page: nearly flat fingers, splayed, the thumb out to the side.
    'flat': dict(curl={'index': (6, 10, 6), 'middle': (5, 12, 7), 'ring': (8, 14, 8), 'little': (12, 16, 10)},
                 spread={'index': 7, 'middle': 0, 'ring': -7, 'little': -16}, thumb=(55, 8, 10, 12)),
    # Holding a pen: the index and thumb pinch it, the others curl under.
    'pen': dict(curl={'index': (28, 30, 12), 'middle': (48, 52, 26), 'ring': (70, 72, 38), 'little': (78, 76, 42)},
                spread={'index': 4, 'middle': -2, 'ring': -6, 'little': -10}, thumb=(32, 22, 18, 40)),
    # Round a pole.
    'grip': dict(curl={'index': (62, 78, 45), 'middle': (66, 80, 48), 'ring': (70, 82, 50), 'little': (74, 84, 52)},
                 spread={'index': 4, 'middle': 0, 'ring': -4, 'little': -8}, thumb=(25, 35, 30, 58)),
    # Relaxed, a watch chain over the fingers.
    'loose': dict(curl={'index': (22, 28, 16), 'middle': (28, 34, 20), 'ring': (34, 40, 24), 'little': (40, 44, 28)},
                  spread={'index': 5, 'middle': 0, 'ring': -5, 'little': -11}, thumb=(38, 18, 14, 34)),
}


def hand(part, wrist, fwd, back, thumb_side, pose, s=1.0, fingers=1.0, bulk=1.0, mat='skin'):
    """A hand: a palm lofted from the wrist to the knuckles, four three-jointed fingers and a thumb as tapered, capped
    tubes. fwd points from the wrist to the middle knuckle, back out of the back of the hand, thumb_side toward the
    thumb. s scales it all, fingers the fingers' length, bulk their thickness and the palm's width. Returns the tips."""
    pose = POSES[pose] if isinstance(pose, str) else pose
    fwd = nrm(fwd)
    back0 = Vector(back)
    back0 = (back0 - fwd * back0.dot(fwd)).normalized()
    across = Vector(thumb_side)
    across = (across - fwd * across.dot(fwd) - back0 * across.dot(back0)).normalized()
    palm_len = 0.098 * s
    half_w = 0.042 * s * bulk
    w = Vector(wrist)
    knuckles = w + fwd * palm_len
    path = smooth_path([w - fwd * 0.016 * s, w + fwd * palm_len * 0.45, knuckles], 9, 4)
    sign = 1.0 if fwd.cross(back0).dot(across) > 0.0 else -1.0

    def palm(t, phi):
        rx = keys([(0.0, 0.025 * s), (0.4, 0.035 * s * bulk), (0.85, half_w), (1.0, half_w * 0.96)], t)
        rb = keys([(0.0, 0.017 * s), (0.5, 0.0125 * s), (1.0, 0.0115 * s)], t)
        rp = keys([(0.0, 0.018 * s), (0.45, 0.02 * s * bulk), (1.0, 0.0145 * s)], t)
        x, y = superellipse(phi, rx, rb, rp, 2.4)
        if y < 0.0 and x * sign > 0.0:   # the thumb's mound, on the palm side toward the thumb, near the wrist
            y -= 0.006 * s * (x * sign / rx) * (1.0 - smoothstep(0.25, 0.8, t)) * smoothstep(0.0, 0.25, t)
        return x, y
    loft(part, path, back0, palm, segs=16, caps=('pole', 'pole'), mat=mat, pole_ext=(0.008 * s, 0.005 * s))
    tips = {}
    for name in FINGERS:
        off, ahead = FINGER_AT[name]
        base = knuckles + across * off * s * bulk + fwd * (ahead * s - 0.010 * s) + back0 * 0.001 * s
        sp = math.radians(pose['spread'].get(name, 0.0))
        d = (fwd * math.cos(sp) + across * math.sin(sp)).normalized()
        fb = (back0 - d * back0.dot(d)).normalized()
        p = base.copy()
        pts = [base - d * 0.008 * s]
        for k, seg in enumerate(FINGER_LEN[name]):
            a = math.radians(pose['curl'][name][k])
            d, fb = (d * math.cos(a) - fb * math.sin(a)).normalized(), (fb * math.cos(a) + d * math.sin(a)).normalized()
            seg *= s * fingers
            pts += [p + d * seg * 0.5, p + d * seg]
            p = p + d * seg
        r0 = FINGER_R[name] * s * bulk

        def fsec(t, phi, r0=r0):
            r = r0 * keys([(0.0, 1.0), (0.3, 1.03), (0.37, 0.93), (0.62, 0.92), (0.68, 0.85), (1.0, 0.8)], t)
            return superellipse(phi, r, r * 0.92, r * 0.98, 2.2)
        loft(part, smooth_path(pts, 14, 3), back0, fsec, segs=10, caps=('pole', 'pole'), mat=mat,
             pole_ext=(0.004 * s, r0 * 0.55))
        tips[name] = p
    ab, f1, f2, lift = pose['thumb']
    base = w + fwd * 0.016 * s + across * 0.018 * s * bulk - back0 * 0.007 * s
    d = (fwd * math.cos(math.radians(ab)) + across * math.sin(math.radians(ab))).normalized()
    d = (d * math.cos(math.radians(lift)) - back0 * math.sin(math.radians(lift))).normalized()
    tb = back0 * 0.6 + across * 0.8
    tb = (tb - d * tb.dot(d)).normalized()
    pts = [base]
    p = base.copy()
    for seg, ang in ((0.042, 0.0), (0.033, f1), (0.028, f2)):
        a = math.radians(ang)
        d, tb = (d * math.cos(a) - tb * math.sin(a)).normalized(), (tb * math.cos(a) + d * math.sin(a)).normalized()
        seg *= s * fingers
        pts += [p + d * seg * 0.5, p + d * seg]
        p = p + d * seg
    r0 = 0.0108 * s * bulk

    def tsec(t, phi):
        r = r0 * keys([(0.0, 1.35), (0.35, 1.0), (0.65, 0.95), (1.0, 0.8)], t)
        return superellipse(phi, r, r * 0.88, r * 0.95, 2.2)
    loft(part, smooth_path(pts, 14, 3), back0, tsec, segs=10, caps=('pole', 'pole'), mat=mat,
         pole_ext=(0.004 * s, r0 * 0.5))
    tips['thumb'] = p
    return tips


# --- Body and clothes ---

class Torso:
    """The coat's body: a loft up the spine, superelliptic sections (half-width, front depth, back depth, exponent)
    by keys. It knows its surface, so skirts, capes and buttons can be laid on it."""

    def __init__(self, points, table, segs=28, open_front=None, folds=None):
        self.path = smooth_path(points, 34, 6)
        self.frames = rm_frames(self.path, FRONT)
        self.ts, _ = arc_params(self.path)
        self.table = table
        self.segs = segs
        self.open_front = open_front
        self.folds = folds

    def section(self, t, phi):
        rx, rf, rb, n = keys(self.table, t)
        x, y = superellipse(phi, rx, rf, rb, n)
        if self.folds is not None:
            d = max(math.hypot(x, y), 1e-6)
            k = 1.0 + self.folds(t, phi) / d
            x, y = x * k, y * k
        return x, y

    def at(self, t):
        for k in range(len(self.ts) - 1):
            if self.ts[k + 1] >= t:
                f = (t - self.ts[k]) / max(self.ts[k + 1] - self.ts[k], 1e-9)
                p = self.path[k].lerp(self.path[k + 1], f)
                tan = self.frames[k][0].lerp(self.frames[k + 1][0], f).normalized()
                nr = self.frames[k][1].lerp(self.frames[k + 1][1], f).normalized()
                return p, tan, nr, tan.cross(nr)
        tan, nr, side = self.frames[-1]
        return self.path[-1], tan, nr, side

    def point(self, t, phi, grow=0.0):
        p, tan, nr, side = self.at(t)
        x, y = self.section(t, phi)
        d = max(math.hypot(x, y), 1e-6)
        k = 1.0 + grow / d
        return p + side * x * k + nr * y * k

    def t_at_z(self, z):
        for k in range(len(self.path) - 1):
            if self.path[k + 1].z >= z:
                f = (z - self.path[k].z) / max(self.path[k + 1].z - self.path[k].z, 1e-9)
                return lerp(self.ts[k], self.ts[k + 1], min(max(f, 0.0), 1.0))
        return 1.0

    def build(self, name, mat='coat', solid=0.0):
        part = Part(name)
        if self.open_front:
            a0 = math.radians(self.open_front)
            loft(part, self.path, FRONT, self.section, segs=self.segs, mat=mat,
                 arc=(a0, 2.0 * math.pi - a0))
            return part.finish(solidify=solid or 0.012)
        loft(part, self.path, FRONT, self.section, segs=self.segs, caps=('pole', 'pole'), mat=mat,
             pole_ext=(0.03, 0.01))
        return part.finish()


def limb(name, points, up, table, mat, segs=16, caps=('pole', None), pole_ext=(0.02, 0.0), folds=None, solid=0.0,
         count=26, per=6, face_mat=None):
    """A sleeve or trouser leg: a loft through the joints, sections by keys [(t, (rx, ry_front, ry_back, n))], and
    folds(t, phi, x, y) -> extra outward offset (cloth wrinkles)."""
    part = Part(name)
    path = smooth_path(points, count, per)

    def section(t, phi):
        rx, rf, rb, n = keys(table, t)
        x, y = superellipse(phi, rx, rf, rb, n)
        if folds is not None:
            extra = folds(t, phi)
            d = max(math.hypot(x, y), 1e-6)
            x, y = x * (1.0 + extra / d), y * (1.0 + extra / d)
        return x, y
    loft(part, path, up, section, segs=segs, caps=caps, mat=mat, pole_ext=pole_ext, face_mat=face_mat)
    return part.finish(solidify=solid), path


def wrinkles(seed, zones):
    """Cloth folds for a limb: zones of (t_center, t_width, phi_center, phi_width, amplitude, ridges): ridges across
    the limb (compression at a bend), with a little noise."""
    def folds(t, phi):
        out = 0.0
        for k, (tc, tw, pc, pw, amp, ridges) in enumerate(zones):
            w = bump(t - tc, tw) * bump(wrap(phi - pc), pw)
            if w <= 0.0:
                continue
            wave = math.sin((t - tc) / tw * math.pi * ridges + 1.3 * k + 2.0 * math.sin(phi * 2.0 + seed))
            out += amp * w * (0.55 + 0.45 * wave)
        out += 0.0015 * vnoise(t * 9.0, phi * 1.6, 0.0, seed)
        return out
    return folds


def foot(name, ankle, fdir, shin_up, length, width, height, toe=0.5, mat='boot', heel=0.028, shaft=0.07,
         shaft_r=0.048, tall=0.0):
    """A boot: a shaft from inside the trouser hem down to the ankle and a foot lofted from the heel to the toe (a
    flat sole side, the toe's point set by toe: 0 round, 1 pointed), a sole and a heel block."""
    part = Part(name)
    f = nrm(fdir)
    up = Vector(shin_up)
    up = (up - f * up.dot(f)).normalized()   # the instep's up
    a = Vector(ankle)
    # The shaft: along the shin, down to the ankle (a tall boot reaches toward the knee).
    top = a + nrm(shin_up) * (shaft + tall)
    loft(part, smooth_path([top, a + nrm(shin_up) * 0.01, a - up * height * 0.25 + f * 0.01], 10, 4), f,
         lambda t, phi: superellipse(phi, keys([(0, shaft_r * (1.08 if tall else 1.0)), (0.7, shaft_r * 0.95),
                                                (1.0, width * 0.46)], t),
                                     keys([(0, shaft_r), (1.0, height * 0.55)], t), None, 2.0),
         segs=16, caps=(None, None), mat=mat)
    heel_pt = a - up * height * 0.62 - f * length * 0.12
    toe_pt = a - up * height * 0.62 + f * length * 0.88
    path = smooth_path([heel_pt, heel_pt.lerp(toe_pt, 0.35) + up * height * 0.06, toe_pt], 18, 6)

    def section(t, phi):
        rx = keys([(0.0, width * 0.28), (0.08, width * 0.40), (0.3, width * 0.44), (0.62, width * 0.5),
                   (0.85, width * lerp(0.42, 0.3, toe)), (1.0, width * lerp(0.2, 0.08, toe))], t)
        rtop = keys([(0.0, height * 0.45), (0.15, height * 0.62), (0.35, height * 0.6), (0.62, height * 0.36),
                     (0.85, height * 0.26), (1.0, height * 0.16)], t)
        rbot = keys([(0.0, height * 0.32), (0.2, height * 0.38), (0.7, height * 0.3), (1.0, height * 0.12)], t)
        return superellipse(phi, rx, rtop, rbot, 2.6 if math.cos(phi) < 0 else 2.0)
    loft(part, path, up, section, segs=18, caps=('pole', 'pole'), mat=mat, pole_ext=(0.012, 0.01))
    # The sole under the foot, a little proud of it, and the heel block.
    sole = [p - up * height * 0.33 for p in path]

    def sole_sec(t, phi):
        rx = keys([(0.0, width * 0.32), (0.08, width * 0.44), (0.3, width * 0.47), (0.62, width * 0.53),
                   (0.85, width * lerp(0.45, 0.33, toe)), (1.0, width * lerp(0.22, 0.1, toe))], t)
        return superellipse(phi, rx, 0.007, 0.007, 3.0)
    loft(part, sole, up, sole_sec, segs=16, caps=('pole', 'pole'), mat='sole', pole_ext=(0.01, 0.008))
    hp = heel_pt - up * height * 0.33 - up * heel * 0.5 + f * length * 0.07
    loft(part, smooth_path([hp - f * 0.035, hp + f * 0.045], 5, 2), up,
         lambda t, phi: superellipse(phi, width * 0.36, heel * 0.5, heel * 0.5, 3.0), segs=12, caps=('pole', 'pole'),
         mat='sole', pole_ext=(0.005, 0.005))
    return part.finish()


# --- The hat ---

def hat(name, frame, crown_r, height, brim, curl, flare=1.04, oval=0.07, band=None, dent=None, warp=0.0, lean=0.0,
        seed=0, segs=56, mat='hat'):
    """A top hat lathed about frame's Z (frame: 4x4, Z up out of the head, -Y its front): crown_r at the base, height,
    brim width (front/back, sides), curl (how far the sides roll up), flare (the top's radius over the base's), oval
    (front-back over side), band (height, thickness, material) as its own loft, dent and warp for a battered one, lean
    (the crown leaning sideways, meters at the top). Returns the part (unfinished: callers add to it) and the band's
    radius function."""
    part = Part(name)
    front_w, side_w = brim

    def crown(th, z):
        r = crown_r * (1.0 + oval * math.cos(2.0 * th)) * lerp(1.0, flare, smoothstep(0.0, 1.0, z / height))
        if dent:
            r += dent * (vnoise(math.cos(th) * 2.2, math.sin(th) * 2.2, z * 9.0, seed) * smoothstep(0.1, 0.9, z / height))
            # A crease pushed into the top's front-left.
            r -= dent * 1.6 * bump(wrap(th - 0.9), 0.9) * smoothstep(0.55, 1.0, z / height)
        return r

    def brim_w(th):
        c2 = math.cos(th) ** 2
        return lerp(side_w, front_w, c2)

    def brim_z(th, s):
        c, si = math.cos(th), math.sin(th)
        z = curl * si * si * s * s - 0.004 * c * c * s
        if warp:
            z += warp * vnoise(math.cos(th) * 1.8 + 4.0, math.sin(th) * 1.8, 0.0, seed + 3) * s
        return z

    # Profile from the lining inside the crown, out under the brim, round its bound edge, back along its top, up the
    # crown and over the top: (kind, share): 'in' lining, 'b' brim at its share s out, 'e' edge, 'c' crown at z share.
    profile = [('in', 0.045), ('in', 0.0), ('b', 0.0, -0.0015), ('b', 0.35, -0.002), ('b', 0.75, -0.001),
               ('b', 0.97, 0.0), ('e', 1.0, 0.0025), ('b', 0.97, 0.0055), ('b', 0.72, 0.0045), ('b', 0.3, 0.0045),
               ('b', 0.02, 0.0055), ('c', 0.02), ('c', 0.08), ('c', 0.35), ('c', 0.7), ('c', 0.93), ('c', 0.975),
               ('top', 0.92), ('top', 0.6)]
    rings = []
    bm = part.bm
    slot = part.slot(mat)
    for item in profile:
        ring = []
        for j in range(segs):
            th = 2.0 * math.pi * j / segs
            kind = item[0]
            if kind == 'in':
                r, z = crown(th, 0.0) - 0.004, item[1]
            elif kind == 'b':
                s, dz = item[1], item[2]
                r = crown(th, 0.0) + brim_w(th) * s + (0.0015 if s < 0.05 else 0.0)
                z = brim_z(th, s) + dz
            elif kind == 'e':
                r = crown(th, 0.0) + brim_w(th) * 1.0 + 0.0025
                z = brim_z(th, 1.0) + item[2]
            elif kind == 'c':
                zz = item[1] * height
                r, z = crown(th, zz), zz + 0.008
            else:
                r, z = crown(th, height) * item[1], height + 0.008 - 0.004 * (1.0 - item[1])
                if dent:
                    z -= dent * 0.8 * bump(wrap(th - 0.9), 1.2) * (1.0 - item[1] * 0.3)
            x, y = r * math.sin(th), -r * math.cos(th)
            if lean and kind in ('c', 'top'):
                x += lean * (z / height) ** 1.5
            ring.append(bm.verts.new(frame @ Vector((x, y, z))))
        rings.append(ring)
    for k in range(len(rings) - 1):
        for j in range(segs):
            j1 = (j + 1) % segs
            f = bm.faces.new((rings[k][j], rings[k][j1], rings[k + 1][j1], rings[k + 1][j]))
            f.material_index = slot
            f.smooth = True
    center = frame @ Vector((lean if lean else 0.0, 0.0, height + 0.008 - (dent * 0.6 if dent else 0.0)))
    pole = bm.verts.new(center)
    top = rings[-1]
    for j in range(segs):
        f = bm.faces.new((top[j], top[(j + 1) % segs], pole))
        f.material_index = slot
        f.smooth = True
    if band:
        bh, bt, bmat = band
        bslot = part.slot(bmat)
        brings = []
        for z, grow in ((0.006, bt * 0.6), (0.01, bt), (bh - 0.004, bt), (bh, bt * 0.5), (bh + 0.002, 0.0)):
            ring = []
            for j in range(segs):
                th = 2.0 * math.pi * j / segs
                r = crown(th, z) + grow
                x, y = r * math.sin(th), -r * math.cos(th)
                if lean:
                    x += lean * (z / height) ** 1.5
                ring.append(bm.verts.new(frame @ Vector((x, y, z + 0.008))))
            brings.append(ring)
        for k in range(len(brings) - 1):
            for j in range(segs):
                j1 = (j + 1) % segs
                f = bm.faces.new((brings[k][j], brings[k][j1], brings[k + 1][j1], brings[k + 1][j]))
                f.material_index = bslot
                f.smooth = True
    return part, crown


# --- The head ---

def head(name, center, pitch, yaw, spec, roll=0.0):
    """The head as one loft up its axis: neck, jaw and chin, mouth, nose, brow and skull, with the face's features as
    offsets. Faces above spec['shadow_z'] take the unlit shadow, below it skin (a hard line, as a brim's shadow cuts
    a face at noon); spec['whiskers'] marks side-whiskers in hair. Returns (object, head frame 4x4)."""
    rot = Matrix.Rotation(math.radians(yaw), 3, 'Z') @ Matrix.Rotation(math.radians(-pitch), 3, 'X') @ \
        Matrix.Rotation(math.radians(roll), 3, 'Y')
    hup = rot @ UP
    hfront = rot @ FRONT
    c = Vector(center)
    zs = spec['levels']          # [(z, half-width, front, back)]
    z0, z1 = zs[0][0], zs[-1][0]
    N = 46
    path = [c + hup * lerp(z0, z1, i / N) for i in range(N + 1)]
    frame = Matrix.Translation(c) @ rot.to_4x4()
    sx = spec.get('scale', 1.0)

    def section(t, phi):
        z = lerp(z0, z1, t)
        rx, rf, rb = 0.0, 0.0, 0.0
        for (za, ra, fa, ba), (zb, rb_, fb, bb) in zip(zs, zs[1:]):
            if za <= z <= zb:
                f = smoothstep(0.0, 1.0, (z - za) / max(zb - za, 1e-9))
                rx, rf, rb = lerp(ra, rb_, f), lerp(fa, fb, f), lerp(ba, bb, f)
                break
        x, y = superellipse(phi, rx, rf, rb, spec.get('face_n', 2.5) if math.cos(phi) > 0 else 2.0)
        off = 0.0
        ph = wrap(phi)
        for zc, zw, pc, pw, amp in spec.get('features', []):
            off += amp * bump(z - zc, zw) * bump(ph - pc, pw)
            if pc != 0.0:
                off += amp * bump(z - zc, zw) * bump(ph + pc, pw)
        d = max(math.hypot(x, y), 1e-6)
        return x * (1.0 + off / d) * sx, y * (1.0 + off / d) * sx

    shadow_z = spec['shadow_z']
    whiskers = spec.get('whiskers')

    def face_mat(t, phi):
        z = lerp(z0, z1, t)
        ph = abs(wrap(phi))
        if spec.get('all_shadow'):
            return 'shadow'
        if z > shadow_line(shadow_z, ph):
            return 'shadow'
        if whiskers and whiskers[0] <= z <= whiskers[1] and whiskers[2] <= ph <= whiskers[3]:
            return 'whisker'
        return 'skin'

    def section2(t, phi):
        x, y = section(t, phi)
        if whiskers:
            z = lerp(z0, z1, t)
            ph = abs(wrap(phi))
            w = smoothstep(whiskers[0] - 0.006, whiskers[0] + 0.01, z) * (1.0 - smoothstep(whiskers[1] - 0.01, whiskers[1] + 0.006, z)) * \
                smoothstep(whiskers[2] - 0.12, whiskers[2] + 0.1, ph) * (1.0 - smoothstep(whiskers[3] - 0.1, whiskers[3] + 0.12, ph))
            d = max(math.hypot(x, y), 1e-6)
            k = 1.0 + whiskers[4] * w * (0.8 + 0.2 * vnoise(z * 90.0, phi * 6.0, 0.0, 5)) / d
            x, y = x * k, y * k
        return x, y
    part = Part(name)
    loft(part, path, hfront, section2, segs=32, caps=(None, 'pole'), mat='skin', pole_ext=(0.0, 0.004),
         face_mat=face_mat)
    hair = spec.get('hair')
    if hair:
        zt, zb, open_a, thick_top, thick_bot, hmat = hair
        ta, tb = (zb - z0) / (z1 - z0), (zt - z0) / (z1 - z0)
        sub = path[int(ta * N):int(math.ceil(tb * N)) + 1]

        def hsec(t, phi):
            tt = lerp(ta, tb, t)
            x, y = section(tt, phi)
            d = max(math.hypot(x, y), 1e-6)
            g = lerp(thick_bot, thick_top, t) * (0.85 + 0.3 * vnoise(phi * 5.0, t * 4.0, 0.0, 9))
            # Lank strands: ridges round the head.
            g += 0.0025 * math.sin(phi * 23.0 + 2.0 * math.sin(t * 5.0))
            return x * (1.0 + g / d), y * (1.0 + g / d)
        loft(part, sub, hfront, hsec, segs=28, mat=hmat, arc=(open_a, 2.0 * math.pi - open_a))
    if spec.get('ears'):
        at, size = spec['ears']
        ears(part, c, frame, at, size, mat=spec.get('ear_mat', 'shadow'))
    obj = part.finish(solidify=0.0)
    fade = spec.get('fade', 0.0)
    if fade and not spec.get('all_shadow'):
        shade(obj, frame, shadow_z, fade)
    return obj, frame


def shadow_line(shadow_z, ph):
    """How high the face's shadow comes at angle ph from the front: (front, side[, back]) heights, head-local."""
    if not isinstance(shadow_z, tuple):
        return shadow_z
    if ph <= math.pi * 0.5 or len(shadow_z) < 3:
        return lerp(shadow_z[0], shadow_z[1], min(1.0, 1.0 - math.cos(min(ph, math.pi * 0.5))))
    return lerp(shadow_z[1], shadow_z[2], smoothstep(math.pi * 0.5, math.pi * 0.8, ph))


def shade(obj, frame, shadow_z, fade):
    """The skin (and hair) darken toward the shadow's edge: vertex alpha per face corner, 0 above the line."""
    inv = frame.inverted()
    mesh = obj.data
    alpha = []
    for poly in mesh.polygons:
        for vi in poly.vertices:
            local = inv @ mesh.vertices[vi].co
            line = shadow_line(shadow_z, abs(math.atan2(local.x, -local.y)))
            alpha.append(smoothstep(line, line - fade, local.z))
    SHADE[obj.name] = alpha


# --- The ledger ---

def ledger(name, center, frame3, width=0.235, height=0.34, ribbon_coin=False, seed=0):
    """The open ledger: two leather boards, the page blocks (rising from the outer edges and diving into the gutter),
    a round spine under the gutter, brass corners, and a ribbon hanging over the far edge (with a coin on its end).
    Built in its own frame (x across the spread, y along the spine toward the far edge, z out of the pages), so its
    object's transform is that frame and the ink lines can follow it."""
    part = Part(name)
    W, H = width, height
    cover, block = 0.005, 0.016

    def page_top(u):
        """The page block's top over its share u of the width (0 at the gutter, 1 at the outer edge)."""
        return 0.004 + block * (smoothstep(0.0, 0.16, u) * (1.0 - 0.3 * u)) + 0.002

    for side in (-1.0, 1.0):
        # The board, a little larger than the pages.
        box(part, (side * (W * 0.5 + 0.004), 0.0, -cover * 0.5), (W + 0.012, H + 0.014, cover), mat='leather')
        # The page block: a grid over x, the top following page_top.
        bm = part.bm
        slot = part.slot('pages')
        nx, ny = 18, 10
        top, bot = [], []
        for i in range(nx + 1):
            u = i / nx
            x = side * (0.004 + u * (W - 0.006))
            rt, rb = [], []
            for j in range(ny + 1):
                y = -H * 0.5 + 0.003 + (H - 0.006) * j / ny
                rt.append(bm.verts.new((x, y, page_top(u))))
                rb.append(bm.verts.new((x, y, 0.0005)))
            top.append(rt)
            bot.append(rb)
        def quad(a, b, c, d):
            f = bm.faces.new((a, b, c, d) if side > 0 else (a, d, c, b))
            f.material_index = slot
            f.smooth = False
        for i in range(nx):
            for j in range(ny):
                quad(top[i][j], top[i + 1][j], top[i + 1][j + 1], top[i][j + 1])
                quad(bot[i][j + 1], bot[i + 1][j + 1], bot[i + 1][j], bot[i][j])
        for i in range(nx):
            quad(bot[i][0], bot[i + 1][0], top[i + 1][0], top[i][0])
            quad(top[i][ny], top[i + 1][ny], bot[i + 1][ny], bot[i][ny])
        for j in range(ny):
            quad(top[nx][j], bot[nx][j], bot[nx][j + 1], top[nx][j + 1])
            quad(bot[0][j], top[0][j], top[0][j + 1], bot[0][j + 1])
        # Brass corners on the board's outer corners.
        for yc in (-1.0, 1.0):
            cx, cy = side * (W + 0.008), yc * (H * 0.5 + 0.006)
            box(part, (cx - side * 0.012, cy - yc * 0.012, -cover * 0.5), (0.03, 0.03, cover + 0.002),
                Matrix.Rotation(math.radians(45.0), 3, 'Z'), mat='brass')
    # The spine: half a round tube under the gutter.
    tube(part, [(0.0, -H * 0.5 - 0.007, -0.006), (0.0, H * 0.5 + 0.007, -0.006)], 0.012, segs=12, mat='leather',
         caps=('pole', 'pole'), per=3)
    # The ribbon: out of the gutter at the far end, over the edge, hanging down.
    pts = [(0.0, H * 0.32, 0.006), (0.006, H * 0.5 + 0.004, 0.009), (0.014, H * 0.5 + 0.02, -0.02),
           (0.02, H * 0.5 + 0.03, -0.07), (0.022, H * 0.5 + 0.034, -0.12)]
    path = smooth_path(pts, 18, 4)
    loft(part, path, (0.0, 0.0, 1.0), lambda t, phi: superellipse(phi, 0.0055, 0.0009, 0.0009, 2.0), segs=8,
         caps=('pole', 'pole'), mat='ribbon', pole_ext=(0.001, 0.001))
    if ribbon_coin:
        end = Vector(pts[-1]) + Vector((0.0, 0.0, -0.008))
        disc(part, end, (0.2, 1.0, 0.0), 0.012, 0.0025, mat='gold')
    obj = part.finish(subsurf=0)
    obj.data.set_sharp_from_angle(angle=math.radians(38.0))
    obj.matrix_world = Matrix.Translation(Vector(center)) @ frame3.to_4x4()
    return obj


def book_frame(tilt, yaw=0.0):
    """The ledger's frame on a lap: x toward his right, y along the spine away from him, z out of the pages; the far
    edge tilted up by tilt degrees so the pages face him, turned by yaw."""
    a = math.radians(tilt)
    x = Vector((-1.0, 0.0, 0.0))
    y = Vector((0.0, -math.cos(a), math.sin(a)))
    z = x.cross(y)
    return Matrix.Rotation(math.radians(yaw), 3, 'Z') @ Matrix((x, y, z)).transposed()


# --- The drape (cloth simulation) ---

def drape(name, columns, length, rows, droop, colliders, frames=60, mass=0.4, stiff=(30.0, 30.0, 12.0, 3.0),
          self_collide=True, pin=(1.0, 0.55), quality=7, ragged=None, solid=0.011, mat='coat', subsurf=2):
    """Hangs cloth from a line of top points and lets it fall onto the body. columns: panels, each a list of
    (top point, outward direction) in order around (counter-clockwise from above); length(direction) the cloth's
    length down each column; droop(direction) its first angle below the horizontal (shallow where it must clear
    thighs or shoulders, steep where it can fall straight); colliders get a Collision modifier for the run. The top
    rows are pinned. Returns a static object with the settled shape."""
    grids = []
    for pi, panel in enumerate(columns):
        grid = []
        for i in range(rows + 1):
            row = []
            for j, (top, out) in enumerate(panel):
                out = Vector(out)
                a = math.radians(droop(out))
                d = (Vector((out.x, out.y, 0.0)).normalized() * math.cos(a) - UP * math.sin(a))
                L = length(out)
                if ragged:
                    L *= 1.0 + ragged * vnoise(j * 0.45, pi * 7.0, 0.0, 77) * smoothstep(0.6, 1.0, i / rows)
                row.append(Vector(top) + d * L * i / rows)
            grid.append(row)
        grids.append(grid)
    return drape_grids(name, grids, colliders, frames=frames, mass=mass, stiff=stiff, self_collide=self_collide,
                       pin=pin, quality=quality, solid=solid, mat=mat, subsurf=subsurf)


def drape_grids(name, grids, colliders, frames=60, mass=0.4, stiff=(30.0, 30.0, 12.0, 3.0), self_collide=True,
                pin=(1.0, 0.55), quality=7, solid=0.011, mat='coat', subsurf=2):
    """Simulates cloth given as grids (rows of points, the top row first, each row in order round the body,
    counter-clockwise from above), its top rows pinned, falling onto the colliders."""
    bm = bmesh.new()
    pins = []
    for grid in grids:
        verts = [[bm.verts.new(p) for p in row] for row in grid]
        for i, row in enumerate(verts):
            if i < len(pin):
                pins += [(v, pin[i]) for v in row]
        for i in range(len(verts) - 1):
            for j in range(len(verts[0]) - 1):
                bm.faces.new((verts[i][j], verts[i + 1][j], verts[i + 1][j + 1], verts[i][j + 1]))
    mesh = bpy.data.meshes.new(name + '_sim')
    bm.to_mesh(mesh)
    index = {v: v.index for v in bm.verts}
    pin_list = [(index[v], w) for v, w in pins]
    bm.free()
    obj = bpy.data.objects.new(name + '_sim', mesh)
    COLLECTION.objects.link(obj)
    # The settled drape is cached (Intermediate/SextonConcepts) under a digest of everything it depends on: the
    # starting cloth, the colliders' shapes and the settings. Same inputs, same drape, without running it again.
    key = _drape_key(mesh, colliders, (frames, mass, stiff, self_collide, pin, quality))
    cached = os.path.join(CACHE_DIR, f'{name}_{key}.npy')
    if os.path.exists(cached) and '--resim' not in ARGS:
        coords = np.load(cached)
        settled = mesh.copy()
        settled.vertices.foreach_set('co', coords.ravel())
        settled.update()
        bpy.data.objects.remove(obj)
        log(f'{name}: drape from the cache')
        return _drape_result(name, settled, mat, solid, subsurf)
    group = obj.vertex_groups.new(name='pin')
    for vi, w in pin_list:
        group.add([vi], w, 'REPLACE')
    added = []
    for c in colliders:
        if not any(m.type == 'COLLISION' for m in c.modifiers):
            c.modifiers.new('Collision', 'COLLISION')
            added.append(c)
        c.collision.thickness_outer = 0.006
        c.collision.thickness_inner = 0.01
        c.collision.cloth_friction = 8.0
        c.collision.damping = 0.2
    cloth = obj.modifiers.new('Cloth', 'CLOTH')
    st = cloth.settings
    st.quality = quality
    st.mass = mass
    st.air_damping = 2.0
    st.tension_stiffness, st.compression_stiffness, st.shear_stiffness, st.bending_stiffness = stiff
    st.tension_damping = st.compression_damping = st.shear_damping = 6.0
    st.bending_damping = 1.0
    st.vertex_group_mass = 'pin'
    st.pin_stiffness = 1.0
    cs = cloth.collision_settings
    cs.use_collision = True
    cs.distance_min = 0.005
    cs.collision_quality = 4
    cs.use_self_collision = self_collide
    cs.self_distance_min = 0.004
    cs.self_friction = 5.0
    cloth.point_cache.frame_start = 1
    cloth.point_cache.frame_end = frames
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 1, frames
    t = time.time()
    for f in range(1, frames + 1):
        scene.frame_set(f)
    dg = bpy.context.evaluated_depsgraph_get()
    settled = bpy.data.meshes.new_from_object(obj.evaluated_get(dg), depsgraph=dg)
    log(f'{name}: draped {len(settled.vertices)} vertices over {frames} frames in {time.time() - t:.1f}s')
    os.makedirs(CACHE_DIR, exist_ok=True)
    coords = np.empty(len(settled.vertices) * 3, np.float32)
    settled.vertices.foreach_get('co', coords)
    np.save(cached, coords.reshape(-1, 3))
    scene.frame_set(1)
    bpy.data.objects.remove(obj)
    for c in added:
        for m in list(c.modifiers):
            if m.type == 'COLLISION':
                c.modifiers.remove(m)
    return _drape_result(name, settled, mat, solid, subsurf)


CACHE_DIR = os.path.join(REPO, 'Intermediate', 'SextonConcepts')


def _drape_key(mesh, colliders, settings):
    import hashlib
    h = hashlib.sha1(repr(settings).encode())
    co = np.empty(len(mesh.vertices) * 3, np.float32)
    mesh.vertices.foreach_get('co', co)
    h.update(np.round(co, 4).tobytes())
    dg = bpy.context.evaluated_depsgraph_get()
    for c in colliders:
        ev = c.evaluated_get(dg)
        me = ev.to_mesh()
        cc = np.empty(len(me.vertices) * 3, np.float32)
        me.vertices.foreach_get('co', cc)
        ev.to_mesh_clear()
        h.update(np.round(cc, 4).tobytes())
        h.update(np.round(np.array(c.matrix_world, np.float32), 4).tobytes())
    return h.hexdigest()[:12]


def _drape_result(name, settled, mat, solid, subsurf):
    result = bpy.data.objects.new(name, settled)
    COLLECTION.objects.link(result)
    settled.materials.clear()
    for p in settled.polygons:
        p.use_smooth = True
        p.material_index = 0
    result['slots'] = mat
    if solid:
        mod = result.modifiers.new('Solidify', 'SOLIDIFY')
        mod.thickness = solid
        mod.offset = -1.0
        mod.use_even_offset = False
        mod.use_quality_normals = False
    if subsurf:
        mod = result.modifiers.new('Subsurf', 'SUBSURF')
        mod.levels = 1
        mod.render_levels = subsurf
    PARTS.append(result)
    return result


def bell(center, levels, segs, open_top, open_bottom, folds=(12, 0.012), seed=0, ragged=0.0):
    """A cape's starting shape: rings from its neck down (levels: (z, half-width, front depth, back depth)), open in
    front by open_top at the neck widening to open_bottom at the hem, with soft vertical folds growing toward the
    hem (count, depth). The drape then only settles it onto the shoulders and arms."""
    rows = []
    n = len(levels)
    for i, (z, rx, rf, rb) in enumerate(levels):
        t = i / (n - 1)
        a0 = lerp(open_top, open_bottom, smoothstep(0.0, 1.0, t))
        row = []
        for j in range(segs + 1):
            phi = lerp(a0, 2.0 * math.pi - a0, j / segs)
            x, y = superellipse(phi, rx, rf, rb, 2.0)
            w = 0.5 + 0.5 * math.sin(folds[0] * phi + seed + 1.1 * t)
            extra = folds[1] * smoothstep(0.2, 1.0, t) * w ** 1.6
            d = max(math.hypot(x, y), 1e-6)
            k = 1.0 + extra / d
            zz = z
            if ragged:
                zz -= ragged * (0.5 + 0.5 * vnoise(j * 0.5, 3.0, 0.0, seed + 11)) * smoothstep(0.7, 1.0, t)
            row.append(Vector((center[0] + x * k, center[1] - y * k, zz)))
        rows.append(row)
    return rows


def ring_columns(torso, t, phis, grow):
    """Top points round the torso at t for a drape, each with its outward direction."""
    p0, tan, nr, side = torso.at(t)
    out = []
    for phi in phis:
        p = torso.point(t, phi, grow)
        d = p - p0
        out.append((p, Vector((d.x, d.y, 0.0)).normalized()))
    return out


def stand_in_colliders(x0=-1.9, x1=2.1):
    """The rail and posts as plain boxes, for the drape to rest on."""
    part = Part('_RailCollider')
    box(part, ((x0 + x1) * 0.5, 0.0, -0.06), (x1 - x0, 0.06, 0.12), mat='wood')
    for x in (-0.565, 0.765):
        box(part, (x, -0.085, -0.5), (0.11, 0.11, 1.1), mat='wood')
    obj = part.finish(subsurf=0)
    PARTS.remove(obj)
    return obj


# --- Materials ---

# Every look is a game texture set, tinted (as the importer makes MI_<name> of M_World): key -> (set, tint).
PALETTES = {
    'A': dict(coat=('Polymer', 0x3c3c44), trousers=('Polymer', 0x5e5f66), shirt=('Polymer', 0xf4efe2),
              stock=('Polymer', 0x2a2a2e), skin=('Polymer', 0xb9ada3), hair=('Polymer', 0x77736e),
              hat=('Polymer', 0x38383d), band=('Polymer', 0x232326), boot=('Polymer', 0x3a3634),
              sole=('Polymer', 0x2a2420), iron=('MetalWorn', 0x3c3b3a), gold=('MetalWorn', 0xc9a050),
              brass=('MetalWorn', 0xbf9a5c), leather=('Polymer', 0x70342a), ribbon=('Polymer', 0x5c1f1f),
              wood=('GunWood', 0xffffff)),
    'B': dict(coat=('Polymer', 0x3f3b3a), cape=('Polymer', 0x3f3b3a), trousers=('Polymer', 0x55555a),
              shirt=('Polymer', 0xf4efe2), stock=('Polymer', 0x2a2a2e), skin=('Polymer', 0xc4a698),
              whisker=('Polymer', 0xf8f6f2), hair=('Polymer', 0xe6e2da), hat=('Polymer', 0x3a3838),
              band=('Polymer', 0x262426), crepe=('Polymer', 0x2a2829), boot=('Polymer', 0x3a3532),
              sole=('Polymer', 0x2a2420), glove=('Polymer', 0x948f88), waistcoat=('Polymer', 0x7c766e),
              iron=('MetalWorn', 0x3c3b3a), gold=('MetalWorn', 0xd6ae5c), brass=('MetalWorn', 0xbf9a5c),
              leather=('Polymer', 0x5a3a26), ribbon=('Polymer', 0x2a2a2e), wood=('GunWood', 0xffffff)),
    'C': dict(coat=('Polymer', 0x3e3c38), cape=('Polymer', 0x3a3834), trousers=('Polymer', 0x4c4a48),
              skin=('Polymer', 0xd8cfc6), hat=('Polymer', 0x403a34), band=('Polymer', 0x4a3a2c),
              boot=('Polymer', 0x35302c), sole=('Polymer', 0x2a2420), glove=('Polymer', 0x2e2c2c),
              iron=('MetalWorn', 0x4a4846), gold=('MetalWorn', 0xb88e48), brass=('MetalWorn', 0xb08a50),
              leather=('Polymer', 0x3c2a22), ribbon=('Polymer', 0x5c1f1f), wood=('GunWood', 0xb0a090),
              canopy=('Polymer', 0x2c2c30)),
}
MATS = {}


def unlit(name, color):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    em = nt.nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = lt.hex_color(color)
    em.inputs['Strength'].default_value = 1.0
    nt.links.new(em.outputs['Emission'], out.inputs['Surface'])
    return mat


def clay_material():
    mat = bpy.data.materials.get('Clay') or bpy.data.materials.new('Clay')
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x9a958c)
    bsdf.inputs['Roughness'].default_value = 0.62
    return mat


def page_material(name, tint):
    """Cream pages with rows of brown ink in a copperplate rhythm and a faint ruled column (preview only: in the game
    the pages are plain cream Polymer). Follows the ledger object's own coordinates (x across, y along the spine)."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nodes, links = nt.nodes, nt.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Roughness'].default_value = 0.85
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    tc = nodes.new('ShaderNodeTexCoord')
    sep = nodes.new('ShaderNodeSeparateXYZ')
    links.new(tc.outputs['Object'], sep.inputs['Vector'])
    nsep = nodes.new('ShaderNodeSeparateXYZ')
    links.new(tc.outputs['Normal'], nsep.inputs['Vector'])

    def m(op, a, b=None):
        n = nodes.new('ShaderNodeMath')
        n.operation = op
        for k, v in enumerate((a, b)):
            if v is None:
                continue
            if isinstance(v, (int, float)):
                n.inputs[k].default_value = v
            else:
                links.new(v, n.inputs[k])
        return n.outputs[0]
    # The writing wobbles a little: the rows are measured at a y that drifts with noise.
    wob = nodes.new('ShaderNodeTexNoise')
    wob.inputs['Scale'].default_value = 40.0
    links.new(tc.outputs['Object'], wob.inputs['Vector'])
    y = m('ADD', sep.outputs['Y'], m('MULTIPLY', m('SUBTRACT', wob.outputs['Fac'], 0.5), 0.0016))
    rows = m('MULTIPLY', y, 1.0 / 0.0105)
    dist = m('ABSOLUTE', m('SUBTRACT', m('FRACT', rows), 0.5))
    line = m('SUBTRACT', 1.0, m('MINIMUM', 1.0, m('MAXIMUM', 0.0, m('MULTIPLY', m('SUBTRACT', dist, 0.035), 22.0))))
    # Words: gaps along each row, from noise over (x, row).
    row_id = m('FLOOR', rows)
    cmb = nodes.new('ShaderNodeCombineXYZ')
    links.new(m('MULTIPLY', sep.outputs['X'], 70.0), cmb.inputs['X'])
    links.new(m('MULTIPLY', row_id, 3.7), cmb.inputs['Y'])
    words = nodes.new('ShaderNodeTexNoise')
    words.inputs['Scale'].default_value = 1.0
    words.inputs['Detail'].default_value = 1.0
    links.new(cmb.outputs['Vector'], words.inputs['Vector'])
    word = m('GREATER_THAN', words.outputs['Fac'], 0.47)
    # Each row's writing ends somewhere along the page.
    cmb2 = nodes.new('ShaderNodeCombineXYZ')
    links.new(m('MULTIPLY', row_id, 1.9), cmb2.inputs['X'])
    endn = nodes.new('ShaderNodeTexNoise')
    endn.inputs['Scale'].default_value = 1.0
    links.new(cmb2.outputs['Vector'], endn.inputs['Vector'])
    ax = m('ABSOLUTE', sep.outputs['X'])
    reach = m('ADD', 0.09, m('MULTIPLY', endn.outputs['Fac'], 0.2))
    inside = m('MULTIPLY', m('GREATER_THAN', ax, 0.024), m('LESS_THAN', ax, reach))
    inside = m('MULTIPLY', inside, m('LESS_THAN', m('ABSOLUTE', sep.outputs['Y']), 0.145))
    top = m('GREATER_THAN', nsep.outputs['Z'], 0.6)
    ink = m('MULTIPLY', m('MULTIPLY', m('MULTIPLY', line, word), inside), top)
    # A faint red rule near each page's outer edge: the ledger's amount column.
    rule = m('LESS_THAN', m('ABSOLUTE', m('SUBTRACT', ax, 0.19)), 0.0006)
    rule = m('MULTIPLY', m('MULTIPLY', rule, top), m('LESS_THAN', m('ABSOLUTE', sep.outputs['Y']), 0.155))
    base = nodes.new('ShaderNodeRGB')
    base.outputs['Color'].default_value = [c * 0.62 for c in lt.hex_color(tint)[:3]] + [1.0]
    red = nodes.new('ShaderNodeMix')
    red.data_type = 'RGBA'
    links.new(rule, red.inputs['Factor'])
    links.new(base.outputs['Color'], red.inputs['A'])
    red.inputs['B'].default_value = lt.hex_color(0xa0584c)
    mix = nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    links.new(m('MULTIPLY', ink, 0.85), mix.inputs['Factor'])
    links.new(red.outputs['Result'], mix.inputs['A'])
    mix.inputs['B'].default_value = lt.hex_color(0x2c2118)
    links.new(mix.outputs['Result'], bsdf.inputs['Base Color'])
    return mat


def lens_material(name):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = (0.02, 0.02, 0.02, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.05
    bsdf.inputs['Coat Weight'].default_value = 1.0
    bsdf.inputs['Specular IOR Level'].default_value = 1.0
    return mat


def material_for(X, key):
    name = f'Sexton{X}_{key}'
    if name in MATS:
        return MATS[name]
    if CLAY and key not in ('shadow', 'lens'):
        mat = clay_material()
    elif key == 'shadow':
        # The face in shadow: drawn unlit, a deep warm black (in the game: an unlit near-black material instance).
        mat = unlit(name, 0x141110)
    elif key == 'pages':
        mat = page_material(name, 0xe9dfc6)
    elif key == 'lens':
        mat = lens_material(name)
    else:
        set_name, tint = PALETTES[X][key]
        mat = lt.material(set_name, name=name, tint=tint)
        if key in MATTE:
            bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
            for link in list(bsdf.inputs['Roughness'].links):
                mat.node_tree.links.remove(link)
            bsdf.inputs['Roughness'].default_value = MATTE[key]
    MATS[name] = mat
    return mat


MATTE = {'coat': 0.8, 'cape': 0.84, 'trousers': 0.78, 'stock': 0.62, 'band': 0.7, 'crepe': 0.92, 'waistcoat': 0.58,
         'hat': 0.5, 'canopy': 0.6, 'skin': 0.62, 'glove': 0.55, 'whisker': 0.9, 'hair': 0.85, 'shirt': 0.75,
         'pages': 0.85}


def white_col(mesh):
    """The textured materials darken by the 'Col' alpha (baked occlusion): white, or they'd render black."""
    col = mesh.color_attributes.get('Col')
    if col is None:
        col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))


def assign_materials(obj, X):
    mesh = obj.data
    keys_ = obj['slots'].split(',')
    if obj.name in SHADE:
        white_col(mesh)
        col = mesh.color_attributes['Col']
        raw = np.ones(4 * len(mesh.loops), np.float32)
        raw[3::4] = np.array(SHADE[obj.name], np.float32)
        col.data.foreach_set('color', raw)
    # (Not materials.clear(): clearing the slots resets every face's material index to 0.)
    for i, key in enumerate(keys_):
        if i < len(mesh.materials):
            mesh.materials[i] = material_for(X, key)
        else:
            mesh.materials.append(material_for(X, key))
    if obj.name not in SHADE:
        white_col(mesh)
    if CLAY:
        return
    for key in keys_:
        if key in ('shadow', 'pages', 'lens'):
            continue
        try:
            lt.box_uv(obj, PALETTES[X][key][0], faces=material_for(X, key))
        except Exception as error:   # a slot no face uses
            log(f'box_uv {obj.name} {key}: {error}')


# --- Pieces the designs share ---

def t_along(points, index):
    """The arc-length share of points[index] along a polyline through points (straight spans)."""
    P = [Vector(p) for p in points]
    lengths = [(b - a).length for a, b in zip(P, P[1:])]
    return sum(lengths[:index]) / max(sum(lengths), 1e-9)


def sleeves(X, seed, joints, table, cuff=None, mat='coat', solid=0.006, folds_amp=0.011):
    """Both sleeves from joints {'L': (shoulder, elbow, wrist), 'R': ...}, ending 3 cm short of the wrist, with folds
    at the inner elbow and a shirt cuff showing (cuff: its radius, or None)."""
    out = []
    for side, joint in joints.items():
        S, E, W = (Vector(p) for p in joint[-3:])
        d = (W - E).normalized()
        end = W - d * 0.03
        start = Vector(joint[0]) if len(joint) == 4 else S + UP * 0.012 + (S - E).normalized() * 0.012
        pts = [start, S, E, end]
        te = t_along(pts, 2)
        folds = wrinkles(seed + (1 if side == 'L' else 2),
                         [(te, 0.075, 0.0, 1.35, folds_amp, 4), (te * 0.45, 0.14, 2.2, 0.9, folds_amp * 0.5, 3),
                          (lerp(te, 1.0, 0.75), 0.1, 3.1, 1.2, folds_amp * 0.3, 2)])
        sleeve, _ = limb(f'Sexton{X}_Sleeve{side}', pts, FRONT, table, mat, segs=16, caps=('pole', None),
                         pole_ext=(0.032, 0.0), folds=folds, solid=solid)
        out.append(sleeve)
        if cuff:
            part = Part(f'Sexton{X}_Cuff{side}')
            tube(part, [W - d * 0.05, W - d * 0.006], cuff, segs=14, mat='shirt', caps=(None, None), per=2)
            part.finish(solidify=0.004)
    return out


def trousers(X, seed, legs, table, solid=0.006, hem_folds=0.0035):
    out = []
    for side, (hip, knee, ankle) in legs.items():
        hip, knee, ankle = Vector(hip), Vector(knee), Vector(ankle)
        pts = [hip - (knee - hip).normalized() * 0.06, hip, knee, ankle + (ankle - knee).normalized() * 0.015]
        tk = t_along(pts, 2)
        folds = wrinkles(seed + (3 if side == 'L' else 4),
                         [(tk, 0.075, math.pi, 1.3, 0.007, 4), (0.09, 0.06, 0.0, 1.0, 0.004, 3),
                          (0.93, 0.08, 0.0, 3.2, hem_folds, 3), (tk * 0.55, 0.12, 0.0, 0.9, 0.0025, 2)])
        obj, _ = limb(f'Sexton{X}_Leg{side}', pts, UP, table, 'trousers', segs=18, caps=('pole', None),
                      pole_ext=(0.03, 0.0), folds=folds, solid=solid)
        out.append(obj)
    return out


def torso_band(name, torso, t0, t1, grow, mat='coat', segs=28):
    """A band round the torso (a waist seam, a belt) from t0 to t1, standing grow off it."""
    part = Part(name)
    path = [torso.at(lerp(t0, t1, k / 3))[0] for k in range(4)]

    def section(t, phi):
        tt = lerp(t0, t1, t)
        x, y = torso.section(tt, phi)
        d = max(math.hypot(x, y), 1e-6)
        g = grow * (0.6 + 0.4 * math.sin(math.pi * t))
        return x * (1.0 + g / d), y * (1.0 + g / d)
    loft(part, path, FRONT, section, segs=segs, mat=mat)
    return part.finish(solidify=0.004)


def neck_piece(part, p0, p1, front, r0, r1, mat, arc=None, flare=0.0, front_low=0.0, wings=0.0, segs=28, rings=6,
               depth=1.08, puff=0.0):
    """A ring round the neck from p0 up to p1 (a stock, a shirt collar, a coat collar), radius r0 to r1, its top
    turning outward by flare. arc leaves it open at the front; front_low (0..1) lowers its top toward the open ends
    (a coat collar); wings folds the top corners at the ends out and down (a wing collar)."""
    p0, p1 = Vector(p0), Vector(p1)
    path = [p0.lerp(p1, k / rings) for k in range(rings + 1)]
    h = (p1 - p0).length

    def section(t, phi):
        r = lerp(r0, r1, t) + flare * smoothstep(0.45, 1.0, t) + puff * math.sin(math.pi * t)
        z = 0.0
        if arc is not None:
            near = smoothstep(math.pi, arc[0], abs(wrap(phi)))     # 0 at the back, 1 at the open ends
            z -= front_low * near * t * h
            if wings:
                w = smoothstep(arc[0] + 0.45, arc[0] + 0.02, abs(wrap(phi)))
                f = smoothstep(0.45, 1.0, t)
                r += wings * w * f * 1.2
                z -= wings * w * f * 1.6
        return r * math.sin(phi), r * math.cos(phi) * depth, z
    return loft(part, path, front, section, segs=segs, mat=mat, arc=arc)


def bow_tie(part, at, hup, hfront, size=1.0, mat='stock'):
    """A bow tie: a pinched knot and two wings flaring out from it."""
    side = Vector(hup).cross(Vector(hfront)).normalized()
    up = Vector(hup)
    f3 = Matrix((side, Vector(hfront), up)).transposed()
    at = Vector(at)
    ellipsoid(part, at + Vector(hfront) * 0.002, (0.009 * size, 0.007 * size, 0.009 * size), f3, segs=12, rings=8, mat=mat)
    for s in (-1.0, 1.0):
        path = [at + side * s * 0.004 * size, at + side * s * 0.02 * size + up * 0.0015 * size,
                at + side * s * 0.036 * size - Vector(hfront) * 0.003 * size]

        def sec(t, phi):
            h = keys([(0.0, 0.006), (0.35, 0.011), (0.8, 0.015), (1.0, 0.012)], t) * size
            w = keys([(0.0, 0.004), (0.5, 0.0055), (1.0, 0.004)], t) * size
            x, y = superellipse(phi, w, h, h, 2.4)
            # A dent across each wing, where the silk folds.
            y *= 1.0 - 0.18 * bump(t - 0.55, 0.2) * (1.0 - abs(math.cos(phi)))
            return x, y
        loft(part, smooth_path(path, 8, 3), Vector(hfront), lambda t, phi: (lambda xy: (xy[1], xy[0]))(sec(t, phi)),
             segs=12, caps=('pole', 'pole'), mat=mat, pole_ext=(0.002, 0.004 * size))


def ears(part, center, hf, at=(0.071, 0.006, 0.004), size=1.0, mat='shadow'):
    """Two ears, flattened and tilted back, at head-local at (x side, y back, z up)."""
    for s in (-1.0, 1.0):
        local = Vector((s * at[0], at[1], at[2]))
        p = hf @ local
        rot = hf.to_3x3() @ Matrix.Rotation(math.radians(-14.0), 3, 'X') @ Matrix.Rotation(math.radians(s * 18.0), 3, 'Z')
        ellipsoid(part, p, (0.009 * size, 0.022 * size, 0.031 * size), rot, segs=12, rings=8, mat=mat)


def buttons(part, body, spots, radius, thick, mat='iron'):
    """Buttons laid on the coat: spots are (origin, direction) rays toward its surface."""
    for origin, direction in spots:
        hit = surface_hit(body, origin, direction)
        if hit is None:
            continue
        p, n = hit
        disc(part, p + n * thick * 0.25, n, radius, thick, segs=16, mat=mat, taper=0.8)


def ledger_on(X, center, tilt, yaw, coin=False, width=0.235, height=0.34):
    obj = ledger(f'Sexton{X}_Ledger', center, book_frame(tilt, yaw), width=width, height=height, ribbon_coin=coin)
    return obj


# --- A: the Gentleman ---

def build_A():
    X, seed = 'A', 101
    rail = stand_in_colliders()
    # The coat's body: a narrow chest, a pinched waist, sloping shoulders, creases where he leans forward.
    torso = Torso([(0.0, 0.05, 0.03), (0.0, 0.06, 0.15), (0.0, 0.055, 0.30), (0.0, 0.035, 0.47), (0.0, 0.015, 0.58),
                   (0.0, 0.0, 0.665), (0.0, -0.012, 0.775)],
                  [(0.0, (0.16, 0.10, 0.14, 2.2)), (0.16, (0.168, 0.118, 0.13, 2.2)), (0.36, (0.14, 0.106, 0.102, 2.2)),
                   (0.59, (0.158, 0.122, 0.106, 2.3)), (0.74, (0.172, 0.118, 0.104, 2.5)),
                   (0.855, (0.195, 0.092, 0.094, 3.0)), (0.93, (0.15, 0.08, 0.086, 2.6)),
                   (1.0, (0.07, 0.062, 0.068, 2.0))],
                  folds=wrinkles(seed + 9, [(0.42, 0.07, 0.0, 1.1, 0.0045, 3), (0.62, 0.12, 0.0, 0.5, 0.002, 2)]))
    body = torso.build(f'Sexton{X}_Coat')
    # The legs: the right crossed over the left, knee on knee, both feet hanging well clear of the deck.
    hipL, hipR = V(0.093, 0.03, 0.10), V(-0.093, 0.03, 0.10)
    kneeL, ankleL = V(0.125, -0.48, -0.03), V(0.12, -0.47, -0.58)
    kneeR, ankleR = V(0.07, -0.56, 0.08), V(0.205, -0.76, -0.42)
    leg_table = [(0.0, (0.086, 0.086, 0.086, 2.0)), (0.1, (0.088, 0.084, 0.09, 2.0)), (0.3, (0.078, 0.073, 0.077, 2.0)),
                 (0.47, (0.064, 0.066, 0.066, 2.0)), (0.53, (0.066, 0.07, 0.064, 2.0)), (0.63, (0.057, 0.051, 0.063, 2.0)),
                 (0.86, (0.049, 0.047, 0.051, 2.0)), (1.0, (0.055, 0.054, 0.056, 2.0))]
    legs = trousers(X, seed, {'L': (hipL, kneeL, ankleL), 'R': (hipR, kneeR, ankleR)}, leg_table)
    boots = [foot(f'Sexton{X}_BootL', ankleL, (0.04, -0.6, -0.8), kneeL - ankleL, 0.30, 0.09, 0.083, toe=0.85),
             foot(f'Sexton{X}_BootR', ankleR, (0.42, -0.5, -0.75), kneeR - ankleR, 0.30, 0.09, 0.083, toe=0.85)]
    # The coat's skirt: two panels from the waist seam, the front edges parting over the crossed legs, the tails
    # falling over the rail behind him.
    t_w = torso.t_at_z(0.30)
    cols = [ring_columns(torso, t_w, [lerp(0.07, math.pi - 0.03, k / 33) for k in range(34)], 0.012),
            ring_columns(torso, t_w, [lerp(-math.pi + 0.03, -0.07, k / 33) for k in range(34)], 0.012)]
    drape(f'Sexton{X}_Skirt', cols, lambda o: lerp(0.58, 0.70, smoothstep(-0.6, 0.9, o.y)), 20,
          lambda o: lerp(8.0, 74.0, smoothstep(-0.75, 0.85, o.y)), [body, rail] + legs + boots, frames=70)
    torso_band(f'Sexton{X}_Waist', torso, t_w - 0.01, t_w + 0.014, 0.011)
    # The arms: the left hand spread on the ledger, the right holding a dip pen over it.
    arms = {'L': (V(0.11, 0.0, 0.665), V(0.175, 0.0, 0.64), V(0.26, -0.10, 0.32), V(0.20, -0.355, 0.235)),
            'R': (V(-0.11, 0.0, 0.665), V(-0.175, 0.0, 0.64), V(-0.26, -0.07, 0.34), V(-0.10, -0.32, 0.29))}
    sleeve_table = [(0.0, (0.052, 0.054, 0.054, 2.0)), (0.12, (0.057, 0.058, 0.058, 2.0)), (0.5, (0.049, 0.05, 0.052, 2.0)),
                    (0.8, (0.044, 0.044, 0.047, 2.0)), (1.0, (0.044, 0.044, 0.046, 2.0))]
    sleeves(X, seed, arms, sleeve_table, cuff=0.035)
    bf = book_frame(16.0, -8.0)
    page_n = bf.col[2]
    ledger_on(X, (0.03, -0.40, 0.172), 16.0, -8.0, coin=True)
    hp = Part(f'Sexton{X}_Hands')
    hand(hp, arms['L'][3], (-0.15, -0.95, -0.12), page_n, (-1.0, 0.0, 0.0), 'flat', s=1.08, fingers=1.3, bulk=0.82)
    fwd_r = nrm((0.10, -0.75, -0.65))
    back_r = nrm((-0.45, 0.2, 0.87))
    tips = hand(hp, arms['R'][3], fwd_r, back_r, (1.0, 0.0, 0.0), 'pen', s=1.08, fingers=1.3, bulk=0.82)
    hp.finish()
    pinch = (tips['thumb'] + tips['index']) * 0.5
    axis = nrm(-fwd_r * 0.55 + back_r * 0.75 + Vector((1.0, 0.0, 0.0)) * 0.3)
    pen = Part(f'Sexton{X}_Pen')
    tube(pen, [pinch + axis * 0.14, pinch - axis * 0.012], 0.0045, segs=10, mat='wood', per=2)
    tube(pen, [pinch - axis * 0.01, pinch - axis * 0.036], 0.0034, segs=10, mat='gold', per=2, radius_end=0.0004)
    pen.finish(subsurf=1)
    # The head, bowed a little: the shadow covers him down to the upper lip; a long nose, a sharp pale chin, a thin
    # mouth, hollow cheeks, grey hair to the collar.
    center, pitch = V(0.0, -0.065, 0.905), 14.0
    spec = dict(
        levels=[(-0.15, 0.046, 0.048, 0.052), (-0.13, 0.048, 0.054, 0.056), (-0.122, 0.049, 0.06, 0.058),
                (-0.114, 0.051, 0.075, 0.064), (-0.106, 0.052, 0.086, 0.07), (-0.09, 0.056, 0.093, 0.075),
                (-0.068, 0.06, 0.093, 0.081), (-0.047, 0.063, 0.095, 0.087), (-0.026, 0.066, 0.097, 0.092),
                (0.0, 0.072, 0.097, 0.097), (0.024, 0.074, 0.096, 0.1), (0.045, 0.075, 0.097, 0.102),
                (0.07, 0.075, 0.093, 0.104), (0.095, 0.071, 0.083, 0.1), (0.115, 0.06, 0.066, 0.086),
                (0.13, 0.038, 0.042, 0.054), (0.138, 0.012, 0.012, 0.016)],
        features=[(-0.014, 0.03, 0.0, 0.2, 0.026), (0.016, 0.024, 0.0, 0.15, 0.009), (0.04, 0.012, 0.0, 1.0, 0.004),
                  (0.022, 0.016, 0.5, 0.28, -0.008), (0.004, 0.016, 1.0, 0.3, 0.004),
                  (-0.042, 0.022, 1.05, 0.42, -0.008), (-0.041, 0.006, 0.0, 0.45, 0.003),
                  (-0.049, 0.003, 0.0, 0.55, -0.0035), (-0.056, 0.006, 0.0, 0.4, 0.0028),
                  (-0.067, 0.008, 0.0, 0.35, -0.003), (-0.094, 0.014, 0.0, 0.36, 0.008),
                  (-0.086, 0.022, 1.45, 0.35, 0.005)],
        shadow_z=(-0.075, -0.09, 0.07), fade=0.02, hair=(0.062, -0.13, 1.75, 0.008, 0.012, 'hair'), ears=((0.068, 0.016, 0.004), 0.85),
        face_n=2.2)
    head_obj, hf = head(f'Sexton{X}_Head', center, pitch, 0.0, spec)
    hup, hfront = hf.to_3x3() @ UP, hf.to_3x3() @ FRONT
    # The neck rises from the coat's neckline to under his jaw; the stock, the wing collar and the bow follow it.
    n0, n1 = V(0.0, -0.012, 0.735), center + hup * -0.128
    nd = (n1 - n0).normalized()
    neck = Part(f'Sexton{X}_Collar')
    tube(neck, [n0 - nd * 0.01, n1 + nd * 0.03], 0.046, segs=16, mat='shadow', caps=(None, None), per=2)
    neck_piece(neck, n0 + nd * 0.005, n1 + nd * 0.004, hfront, 0.0495, 0.049, 'stock', puff=0.002)
    neck_piece(neck, n0.lerp(n1, 0.62), n1 + nd * 0.02, hfront, 0.0525, 0.0545, 'shirt',
               arc=(0.55, 2.0 * math.pi - 0.55), wings=0.004)
    bow_tie(neck, n0.lerp(n1, 0.72) + hfront * 0.055, hup, hfront, 1.0)
    neck.finish(solidify=0.003)
    coat_collar = Part(f'Sexton{X}_CoatCollar')
    neck_piece(coat_collar, n0 - nd * 0.012, n0 + nd * 0.082, hfront, 0.064, 0.06, 'coat',
               arc=(0.95, 2.0 * math.pi - 0.95), flare=0.013, front_low=0.75, depth=1.1)
    coat_collar.finish(solidify=0.007)
    hp2, _ = hat(f'Sexton{X}_Hat', hf @ Matrix.Translation((0.0, 0.005, 0.058)), 0.097, 0.33, (0.052, 0.04), 0.034,
                 flare=1.06, band=(0.045, 0.003, 'band'))
    hp2.finish(subsurf=2)
    btn = Part(f'Sexton{X}_Buttons')
    buttons(btn, body, [((0.0, -0.5, z), (0.0, 1.0, 0.0)) for z in (0.34, 0.392, 0.444, 0.496, 0.548, 0.6, 0.652)],
            0.0085, 0.004)
    buttons(btn, body, [((s * 0.05, 0.5, 0.33), (0.0, -1.0, 0.0)) for s in (-1.0, 1.0)], 0.009, 0.004)
    btn.finish(subsurf=1)
    bpy.data.objects.remove(rail)


def cravat(part, at, hup, hfront, size=1.0, mat='stock'):
    """A cravat's knot with its two ends falling into the waistcoat's opening."""
    side = Vector(hup).cross(Vector(hfront)).normalized()
    up, fr = Vector(hup), Vector(hfront)
    f3 = Matrix((side, fr, up)).transposed()
    at = Vector(at)
    ellipsoid(part, at, (0.014 * size, 0.009 * size, 0.012 * size), f3, segs=12, rings=8, mat=mat)
    for s in (-1.0, 1.0):
        path = [at - up * 0.006 + side * s * 0.004, at - up * 0.035 + side * s * 0.012 + fr * 0.006,
                at - up * 0.07 + side * s * 0.016 + fr * 0.004]
        loft(part, smooth_path(path, 8, 3), fr,
             lambda t, phi: superellipse(phi, keys([(0.0, 0.009), (1.0, 0.016)], t) * size, 0.0035, 0.0035, 2.6),
             segs=10, caps=('pole', 'pole'), mat=mat, pole_ext=(0.002, 0.003))


def spectacles(part, hf, eye_z=0.022, eye_x=0.033, front=0.104, radius=0.017):
    """Round gold spectacles in front of the eyes: rims, lenses, a bridge, temples back to the ears."""
    centers = []
    for s in (-1.0, 1.0):
        c = hf @ Vector((s * eye_x, -front, eye_z))
        n = hf.to_3x3() @ Vector((0.0, -1.0, 0.0))
        torus(part, c, n, radius, 0.0017, segs=28, minor_segs=8, mat='gold')
        disc(part, c, n, radius * 0.96, 0.0012, segs=24, mat='lens', taper=1.0)
        centers.append(c)
        out = hf @ Vector((s * (eye_x + radius), -front + 0.002, eye_z + 0.002))
        ear = hf @ Vector((s * 0.074, -0.01, eye_z + 0.006))
        tube(part, [out, out.lerp(ear, 0.5) + hf.to_3x3() @ Vector((s * 0.006, 0.0, 0.0)), ear], 0.0012, segs=6,
             mat='gold', per=4)
    mid = (centers[0] + centers[1]) * 0.5 + hf.to_3x3() @ Vector((0.0, 0.002, 0.008))
    tube(part, [centers[0] + hf.to_3x3() @ Vector((radius * 0.95, 0.0, 0.004)), mid,
                centers[1] + hf.to_3x3() @ Vector((-radius * 0.95, 0.0, 0.004))], 0.0013, segs=6, mat='gold', per=4)


# --- B: the Undertaker ---

def build_B():
    X, seed = 'B', 202
    rail = stand_in_colliders()
    path = [(0.0, 0.06, 0.03), (0.0, 0.07, 0.15), (0.0, 0.045, 0.30), (0.0, 0.03, 0.46), (0.0, 0.02, 0.56),
            (0.0, 0.01, 0.645), (0.0, -0.005, 0.755)]
    # The waistcoat over a round belly, and the open greatcoat round it.
    vest = Torso(path, [(0.0, (0.18, 0.13, 0.15, 2.2)), (0.16, (0.19, 0.17, 0.14, 2.2)), (0.38, (0.185, 0.19, 0.12, 2.2)),
                        (0.6, (0.18, 0.15, 0.115, 2.3)), (0.75, (0.185, 0.125, 0.11, 2.5)),
                        (0.86, (0.2, 0.095, 0.098, 3.0)), (0.93, (0.155, 0.082, 0.088, 2.6)),
                        (1.0, (0.075, 0.066, 0.072, 2.0))],
                 folds=wrinkles(seed + 9, [(0.47, 0.06, 0.0, 0.8, 0.003, 3)]))
    vest_obj = vest.build(f'Sexton{X}_Waistcoat', mat='waistcoat')
    coat = Torso(path, [(0.0, (0.195, 0.145, 0.165, 2.2)), (0.16, (0.205, 0.18, 0.155, 2.2)),
                        (0.38, (0.2, 0.2, 0.135, 2.2)), (0.6, (0.197, 0.162, 0.128, 2.3)),
                        (0.75, (0.2, 0.138, 0.122, 2.5)), (0.86, (0.215, 0.106, 0.108, 3.0)),
                        (0.93, (0.165, 0.092, 0.098, 2.6)), (1.0, (0.085, 0.076, 0.082, 2.0))], open_front=34.0)
    coat_obj = coat.build(f'Sexton{X}_Coat', solid=0.012)
    # The legs, ankle on knee: the right shin lies across the left knee, its boot past it.
    hipL, hipR = V(0.105, 0.03, 0.10), V(-0.105, 0.03, 0.10)
    kneeL, ankleL = V(0.14, -0.45, -0.02), V(0.13, -0.44, -0.54)
    kneeR, ankleR = V(-0.33, -0.38, 0.11), V(0.12, -0.5, 0.115)
    leg_table = [(0.0, (0.096, 0.096, 0.096, 2.0)), (0.1, (0.098, 0.094, 0.1, 2.0)), (0.3, (0.088, 0.083, 0.087, 2.0)),
                 (0.47, (0.07, 0.072, 0.072, 2.0)), (0.53, (0.072, 0.076, 0.07, 2.0)), (0.63, (0.064, 0.058, 0.07, 2.0)),
                 (0.86, (0.055, 0.053, 0.057, 2.0)), (1.0, (0.06, 0.059, 0.061, 2.0))]
    legs = trousers(X, seed, {'L': (hipL, kneeL, ankleL), 'R': (hipR, kneeR, ankleR)}, leg_table)
    boots = [foot(f'Sexton{X}_BootL', ankleL, (0.0, -0.62, -0.78), kneeL - ankleL, 0.29, 0.1, 0.09, toe=0.2),
             foot(f'Sexton{X}_BootR', ankleR, (0.7, -0.3, -0.62), kneeR - ankleR, 0.29, 0.1, 0.09, toe=0.2)]
    # The greatcoat's skirt, long and open in front, over the lap and down behind the rail.
    t_w = coat.t_at_z(0.28)
    cols = [ring_columns(coat, t_w, [lerp(0.6, math.pi - 0.03, k / 33) for k in range(34)], 0.006),
            ring_columns(coat, t_w, [lerp(-math.pi + 0.03, -0.6, k / 33) for k in range(34)], 0.006)]
    skirt = drape(f'Sexton{X}_Skirt', cols, lambda o: lerp(0.64, 0.76, smoothstep(-0.6, 0.9, o.y)), 21,
                  lambda o: 5.0 + 78.0 * smoothstep(-0.9, 0.5, o.y), [vest_obj, coat_obj, rail] + legs + boots,
                  frames=70)
    # The arms: the left hand on the ledger, the right holding up his watch on its chain.
    arms = {'L': (V(0.12, 0.01, 0.655), V(0.2, 0.01, 0.625), V(0.29, -0.06, 0.34), V(0.14, -0.3, 0.255)),
            'R': (V(-0.12, 0.01, 0.655), V(-0.2, 0.01, 0.625), V(-0.3, -0.12, 0.35), V(-0.2, -0.38, 0.4))}
    sleeve_table = [(0.0, (0.06, 0.062, 0.062, 2.0)), (0.12, (0.066, 0.068, 0.068, 2.0)), (0.5, (0.058, 0.06, 0.062, 2.0)),
                    (0.8, (0.053, 0.053, 0.056, 2.0)), (1.0, (0.054, 0.054, 0.056, 2.0))]
    sl = sleeves(X, seed, arms, sleeve_table, cuff=0.04)
    # The cape: a shoulder cape to the elbows, and a shorter one over it, both draped.
    levels = [(0.748, 0.114, 0.102, 0.11), (0.735, 0.15, 0.115, 0.125), (0.71, 0.215, 0.14, 0.15),
              (0.675, 0.272, 0.166, 0.172), (0.63, 0.302, 0.182, 0.188), (0.56, 0.326, 0.196, 0.202),
              (0.48, 0.342, 0.208, 0.212), (0.41, 0.352, 0.216, 0.218), (0.36, 0.357, 0.222, 0.222)]
    cape1 = drape_grids(f'Sexton{X}_Cape', [bell((0.0, -0.005), levels, 48, 0.4, 0.85, folds=(12, 0.014), seed=seed)],
                        [vest_obj, coat_obj, skirt, rail] + sl + legs, frames=40, stiff=(30.0, 30.0, 12.0, 2.5),
                        mat='cape')
    levels2 = [(0.754, 0.112, 0.1, 0.108), (0.742, 0.155, 0.118, 0.128), (0.718, 0.225, 0.148, 0.158),
               (0.685, 0.288, 0.178, 0.184), (0.64, 0.322, 0.196, 0.202), (0.58, 0.344, 0.21, 0.216),
               (0.54, 0.352, 0.216, 0.222)]
    drape_grids(f'Sexton{X}_Cape2', [bell((0.0, -0.005), levels2, 48, 0.38, 0.7, folds=(14, 0.01), seed=seed + 4)],
                [vest_obj, coat_obj, cape1] + sl, frames=35, stiff=(30.0, 30.0, 12.0, 2.5), mat='cape')
    torso_band(f'Sexton{X}_Waist', vest, vest.t_at_z(0.27), vest.t_at_z(0.29), 0.004, mat='waistcoat')
    bf = book_frame(20.0, 10.0)
    ledger_on(X, (-0.06, -0.37, 0.205), 20.0, 10.0)
    hp = Part(f'Sexton{X}_Hands')
    hand(hp, arms['L'][3], (0.2, -0.95, -0.15), bf.col[2], (-1.0, 0.0, 0.0), 'flat', s=1.02, fingers=1.05, bulk=1.12,
         mat='glove')
    fwd_r = nrm((0.25, -0.9, 0.05))
    back_r = nrm((-0.2, -0.1, 0.97))
    tips = hand(hp, arms['R'][3], fwd_r, back_r, (1.0, 0.0, 0.0), 'loose', s=1.02, fingers=1.05, bulk=1.12, mat='glove')
    hp.finish()
    # The watch chain: from a waistcoat buttonhole over his fingers, the watch hanging below them.
    chain = Part(f'Sexton{X}_Watch')
    hold = (tips['index'] + tips['middle']) * 0.5 - back_r * 0.004
    hole = vest.point(vest.t_at_z(0.33), 0.0, 0.004)
    sag = [hole, hole.lerp(hold, 0.33) - UP * 0.05, hole.lerp(hold, 0.66) - UP * 0.045, hold]
    tube(chain, sag, 0.0022, segs=6, mat='gold', caps=('pole', 'pole'), per=6)
    drop = hold - UP * 0.07
    tube(chain, [hold, drop], 0.002, segs=6, mat='gold', per=3)
    disc(chain, drop - UP * 0.024, (0.25, -0.95, 0.15), 0.024, 0.011, segs=28, mat='gold', taper=0.9)
    torus(chain, drop - UP * 0.001, (1.0, 0.25, 0.0), 0.006, 0.0016, segs=12, minor_segs=6, mat='gold')
    chain.finish(subsurf=1)
    btn = Part(f'Sexton{X}_Buttons')
    buttons(btn, vest_obj, [((0.0, -0.6, z), (0.0, 1.0, 0.0)) for z in (0.3, 0.355, 0.41, 0.465, 0.52)], 0.006, 0.0035,
            mat='gold')
    for s in (-1.0, 1.0):
        buttons(btn, coat_obj, [((s * 0.2, -0.6, z), (-s * 0.35, 1.0, 0.0)) for z in (0.36, 0.45)], 0.011, 0.005)
    btn.finish(subsurf=1)
    # The head, a little turned: white side-whiskers and a soft chin below the shadow; round gold spectacles in it.
    center = V(0.0, -0.06, 0.88)
    spec = dict(
        levels=[(-0.15, 0.052, 0.056, 0.058), (-0.128, 0.056, 0.068, 0.062), (-0.118, 0.062, 0.084, 0.068),
                (-0.108, 0.068, 0.094, 0.074), (-0.092, 0.073, 0.098, 0.08), (-0.07, 0.076, 0.098, 0.086),
                (-0.048, 0.077, 0.1, 0.092), (-0.026, 0.078, 0.102, 0.096), (0.0, 0.08, 0.102, 0.1),
                (0.024, 0.08, 0.1, 0.103), (0.045, 0.08, 0.1, 0.105), (0.07, 0.079, 0.096, 0.106),
                (0.095, 0.074, 0.086, 0.1), (0.115, 0.062, 0.068, 0.086), (0.128, 0.04, 0.044, 0.056),
                (0.135, 0.012, 0.012, 0.016)],
        features=[(-0.016, 0.026, 0.0, 0.26, 0.024), (0.014, 0.02, 0.0, 0.17, 0.007), (0.04, 0.012, 0.0, 1.0, 0.003),
                  (0.022, 0.015, 0.5, 0.28, -0.006), (-0.02, 0.03, 0.9, 0.5, 0.005), (-0.05, 0.003, 0.0, 0.55, -0.003),
                  (-0.057, 0.006, 0.0, 0.4, 0.0025), (-0.092, 0.02, 0.0, 0.5, 0.005)],
        shadow_z=(-0.07, -0.026, 0.07), fade=0.022,
        hair=(0.062, -0.078, 2.05, 0.011, 0.014, 'hair'), ears=((0.077, 0.014, 0.004), 0.95), face_n=2.3)
    head_obj, hf = head(f'Sexton{X}_Head', center, 8.0, -6.0, spec)
    hup, hfront = hf.to_3x3() @ UP, hf.to_3x3() @ FRONT
    # Mutton-chop whiskers: soft white tufts from the sideburns down the cheeks to the jowls, fading up into the
    # shadow like the skin does.
    wk = Part(f'Sexton{X}_Whiskers')
    for sd in (-1.0, 1.0):
        pts = [hf @ Vector((sd * 0.086, 0.012, 0.034)), hf @ Vector((sd * 0.091, 0.0, -0.01)),
               hf @ Vector((sd * 0.088, -0.026, -0.05)), hf @ Vector((sd * 0.076, -0.052, -0.079))]
        out = hf.to_3x3() @ Vector((sd, 0.0, 0.0))

        def wsec(t, phi):
            w = keys([(0.0, 0.012), (0.35, 0.02), (0.75, 0.027), (1.0, 0.016)], t)
            th = keys([(0.0, 0.006), (0.5, 0.012), (0.85, 0.013), (1.0, 0.007)], t)
            x, y = superellipse(phi, w, th, th * 0.5, 2.2)
            y += 0.0015 * math.sin(phi * 9.0 + t * 14.0)
            return x, y
        loft(wk, smooth_path(pts, 14, 4), out, wsec, segs=14, caps=('pole', 'pole'), mat='whisker',
             pole_ext=(0.006, 0.008))
    wk_obj = wk.finish()
    shade(wk_obj, hf, spec['shadow_z'], spec['fade'])
    glasses = Part(f'Sexton{X}_Spectacles')
    spectacles(glasses, hf)
    glasses.finish(subsurf=1)
    n0, n1 = V(0.0, -0.01, 0.725), center + hup * -0.125
    nd = (n1 - n0).normalized()
    neck = Part(f'Sexton{X}_Collar')
    tube(neck, [n0 - nd * 0.01, n1 + nd * 0.03], 0.052, segs=16, mat='shadow', caps=(None, None), per=2)
    neck_piece(neck, n0 + nd * 0.005, n1 + nd * 0.004, hfront, 0.0555, 0.055, 'stock', puff=0.003)
    neck_piece(neck, n0.lerp(n1, 0.5), n1 + nd * 0.02, hfront, 0.0585, 0.06, 'shirt', arc=(0.5, 2.0 * math.pi - 0.5),
               wings=0.004)
    cravat(neck, n0.lerp(n1, 0.62) + hfront * 0.062, hup, hfront, 1.0)
    neck.finish(solidify=0.003)
    collar = Part(f'Sexton{X}_CapeCollar')
    neck_piece(collar, n0 + nd * 0.01, n0 + nd * 0.085, hfront, 0.074, 0.072, 'cape', arc=(0.6, 2.0 * math.pi - 0.6),
               flare=0.016, front_low=0.6, depth=1.08)
    collar.finish(solidify=0.008)
    # A lower top hat with a broad flat brim and a deep crepe band.
    hpart, _ = hat(f'Sexton{X}_Hat', hf @ Matrix.Translation((0.0, 0.004, 0.056)), 0.101, 0.175, (0.078, 0.07), 0.014,
                   flare=1.0, oval=0.06, band=(0.075, 0.0035, 'crepe'))
    hpart.finish(subsurf=2)
    bpy.data.objects.remove(rail)


# --- C: the Ferryman ---

def build_C():
    X, seed = 'C', 303
    rail = stand_in_colliders()
    torso = Torso([(0.0, 0.05, 0.03), (0.0, 0.06, 0.15), (0.0, 0.055, 0.30), (0.0, 0.035, 0.47), (0.0, 0.015, 0.58),
                   (0.0, 0.0, 0.665), (0.0, -0.012, 0.775)],
                  [(0.0, (0.168, 0.105, 0.145, 2.2)), (0.16, (0.176, 0.124, 0.135, 2.2)), (0.36, (0.152, 0.114, 0.11, 2.2)),
                   (0.59, (0.166, 0.128, 0.112, 2.3)), (0.74, (0.18, 0.122, 0.11, 2.5)), (0.855, (0.205, 0.096, 0.098, 3.0)),
                   (0.93, (0.158, 0.084, 0.09, 2.6)), (1.0, (0.075, 0.066, 0.072, 2.0))])
    body = torso.build(f'Sexton{X}_Coat')
    # The legs: the left crossed over the right, in tall sea boots.
    hipL, hipR = V(0.095, 0.03, 0.10), V(-0.095, 0.03, 0.10)
    kneeR, ankleR = V(-0.125, -0.48, -0.03), V(-0.12, -0.47, -0.58)
    kneeL, ankleL = V(-0.07, -0.56, 0.08), V(-0.205, -0.76, -0.42)
    leg_table = [(0.0, (0.09, 0.09, 0.09, 2.0)), (0.1, (0.092, 0.088, 0.094, 2.0)), (0.3, (0.082, 0.077, 0.081, 2.0)),
                 (0.47, (0.068, 0.07, 0.07, 2.0)), (0.53, (0.07, 0.074, 0.068, 2.0)), (0.63, (0.06, 0.055, 0.066, 2.0)),
                 (0.86, (0.052, 0.05, 0.054, 2.0)), (1.0, (0.056, 0.055, 0.057, 2.0))]
    legs = trousers(X, seed, {'L': (hipL, kneeL, ankleL), 'R': (hipR, kneeR, ankleR)}, leg_table)
    boots = [foot(f'Sexton{X}_BootL', ankleL, (-0.42, -0.5, -0.75), kneeL - ankleL, 0.3, 0.1, 0.09, toe=0.35,
                  tall=0.2, shaft_r=0.062),
             foot(f'Sexton{X}_BootR', ankleR, (-0.04, -0.6, -0.8), kneeR - ankleR, 0.3, 0.1, 0.09, toe=0.35,
                  tall=0.2, shaft_r=0.062)]
    # The coat's skirt, long and ragged, hanging nearly to the deck behind the rail.
    t_w = torso.t_at_z(0.30)
    cols = [ring_columns(torso, t_w, [lerp(0.07, math.pi - 0.03, k / 35) for k in range(36)], 0.012),
            ring_columns(torso, t_w, [lerp(-math.pi + 0.03, -0.07, k / 35) for k in range(36)], 0.012)]
    skirt = drape(f'Sexton{X}_Skirt', cols, lambda o: lerp(0.9, 1.06, smoothstep(-0.6, 0.9, o.y)), 26,
                  lambda o: 8.0 + 77.0 * smoothstep(-0.9, 0.5, o.y), [body, rail] + legs + boots, frames=75,
                  ragged=0.08)
    torso_band(f'Sexton{X}_Waist', torso, t_w - 0.01, t_w + 0.014, 0.011)
    # The arms: the left hand on the ledger on his left knee, the right holding the umbrella beside him.
    tip, top = V(-0.5, -0.36, DECK), V(-0.41, -0.13, 0.86)
    u = (top - tip).normalized()
    grip = tip.lerp(top, (0.31 - DECK) / (top.z - DECK))
    out = Vector((-1.0, 0.1, 0.0))
    out = (out - u * out.dot(u)).normalized()
    fwd_g = u.cross(out).normalized()
    if fwd_g.y > 0.0:
        fwd_g = -fwd_g
    wrist_r = grip + out * 0.03 - fwd_g * 0.07 - u * 0.012
    arms = {'L': (V(0.12, 0.0, 0.665), V(0.18, 0.0, 0.64), V(0.27, -0.10, 0.36), V(0.12, -0.36, 0.225)),
            'R': (V(-0.12, 0.0, 0.665), V(-0.18, 0.0, 0.64), V(-0.3, 0.02, 0.36), wrist_r)}
    sleeve_table = [(0.0, (0.056, 0.058, 0.058, 2.0)), (0.12, (0.062, 0.064, 0.064, 2.0)), (0.5, (0.054, 0.056, 0.058, 2.0)),
                    (0.8, (0.05, 0.05, 0.053, 2.0)), (1.0, (0.052, 0.052, 0.054, 2.0))]
    sl = sleeves(X, seed, arms, sleeve_table, cuff=None)
    bf = book_frame(16.0, 8.0)
    ledger_on(X, (-0.03, -0.40, 0.172), 16.0, 8.0)
    hp = Part(f'Sexton{X}_Hands')
    hand(hp, arms['L'][3], (0.12, -0.96, -0.12), bf.col[2], (-1.0, 0.0, 0.0), 'flat', s=1.06, fingers=1.2, bulk=0.98,
         mat='glove')
    hand(hp, wrist_r, fwd_g, out, u, 'grip', s=1.06, fingers=1.2, bulk=0.98, mat='glove')
    hp.finish()
    # The umbrella, as long as a punt pole: a brass ferrule on the deck, the furled canopy, a crook handle.
    um = Part(f'Sexton{X}_Umbrella')
    tube(um, [tip, top], 0.0075, segs=10, mat='iron', caps=('pole', 'pole'), per=2)
    tube(um, [tip, tip + u * 0.07], 0.009, segs=12, mat='brass', per=2, radius_end=0.0065)
    path = [tip + u * (0.06 + 1.2 * k / 30) for k in range(31)]

    def canopy(t, phi):
        r = keys([(0.0, 0.0085), (0.08, 0.016), (0.5, 0.032), (0.78, 0.034), (0.93, 0.02), (1.0, 0.009)], t)
        r += 0.004 * math.sin(phi * 7.0 + t * 26.0) * smoothstep(0.05, 0.3, t) * (1.0 - smoothstep(0.85, 1.0, t))
        return r * math.sin(phi), r * math.cos(phi)
    loft(um, path, (1.0, 0.0, 0.0), canopy, segs=28, caps=('pole', 'pole'), mat='canopy', pole_ext=(0.01, 0.01))
    strap_c = tip + u * 0.86
    loft(um, [strap_c - u * 0.012, strap_c + u * 0.012], (1.0, 0.0, 0.0),
         lambda t, phi: (0.0365 * math.sin(phi), 0.0365 * math.cos(phi)), segs=28, mat='canopy')
    crook = [top - u * 0.02, top + u * 0.06, top + u * 0.1 + out * 0.025, top + u * 0.085 + out * 0.07,
             top + u * 0.04 + out * 0.085, top + out * 0.075]
    tube(um, crook, 0.011, segs=12, mat='wood', per=5)
    um_obj = um.finish(subsurf=2)
    # The cape, long: from the shoulders to the hips, open in front where his arms come out.
    levels = [(0.75, 0.118, 0.104, 0.112), (0.735, 0.156, 0.118, 0.128), (0.71, 0.222, 0.146, 0.156),
              (0.675, 0.282, 0.17, 0.178), (0.62, 0.318, 0.19, 0.196), (0.54, 0.344, 0.206, 0.212),
              (0.45, 0.362, 0.218, 0.226), (0.36, 0.376, 0.228, 0.24), (0.28, 0.386, 0.234, 0.25),
              (0.22, 0.392, 0.238, 0.258)]
    drape_grids(f'Sexton{X}_Cape', [bell((0.0, -0.01), levels, 52, 0.8, 1.12, folds=(13, 0.016), seed=seed,
                                         ragged=0.05)],
                [body, skirt, rail, um_obj] + sl + legs, frames=40, stiff=(30.0, 30.0, 12.0, 2.0), mat='cape')
    # The head stays wholly in shadow, behind a funnel collar up to his nose and under the battered hat.
    center, pitch = V(0.0, -0.06, 0.895), 10.0
    spec = dict(
        levels=[(-0.15, 0.048, 0.05, 0.054), (-0.12, 0.052, 0.07, 0.06), (-0.1, 0.06, 0.09, 0.072),
                (-0.07, 0.066, 0.095, 0.082), (-0.04, 0.069, 0.097, 0.09), (0.0, 0.073, 0.098, 0.097),
                (0.04, 0.075, 0.097, 0.102), (0.07, 0.075, 0.093, 0.104), (0.095, 0.071, 0.083, 0.1),
                (0.115, 0.06, 0.066, 0.086), (0.13, 0.038, 0.042, 0.054), (0.138, 0.012, 0.012, 0.016)],
        features=[(-0.014, 0.03, 0.0, 0.2, 0.024)], shadow_z=1.0, all_shadow=True, ears=((0.071, 0.012, 0.004), 0.9))
    head_obj, hf = head(f'Sexton{X}_Head', center, pitch, 0.0, spec)
    hup, hfront = hf.to_3x3() @ UP, hf.to_3x3() @ FRONT
    n0 = V(0.0, -0.012, 0.735)
    nd = (center + hup * -0.12 - n0).normalized()
    fc = Part(f'Sexton{X}_Collar')
    # The funnel collar: standing from the cape's neckline up past his mouth, flaring a little, nearly closed in front.
    neck_piece(fc, n0 - nd * 0.01, center + hup * -0.035 + hfront * 0.012, hfront, 0.08, 0.096, 'cape',
               arc=(0.2, 2.0 * math.pi - 0.2), flare=0.012, rings=10, depth=1.12)
    fc.finish(solidify=0.008)
    clasp = Part(f'Sexton{X}_Clasp')
    for s in (-1.0, 1.0):
        p = n0 + nd * 0.07 + hfront * 0.093 + Vector((s * 0.02, 0.0, 0.0))
        disc(clasp, p, hfront, 0.011, 0.0025, segs=20, mat='gold')
    tube(clasp, [n0 + nd * 0.07 + hfront * 0.096 + Vector((-0.01, 0.0, 0.0)),
                 n0 + nd * 0.07 + hfront * 0.099 + Vector((0.0, 0.0, -0.006)),
                 n0 + nd * 0.07 + hfront * 0.096 + Vector((0.01, 0.0, 0.0))], 0.0015, segs=6, mat='gold', per=3)
    clasp.finish(subsurf=1)
    # The battered stovepipe: dented, leaning, its brim warped; a band of tarnished old coins.
    hframe = hf @ Matrix.Translation((0.0, 0.005, 0.058)) @ Matrix.Rotation(math.radians(4.0), 4, 'Y')
    hpart, crown = hat(f'Sexton{X}_Hat', hframe, 0.098, 0.3, (0.062, 0.056), 0.012, flare=1.03, oval=0.07,
                       band=(0.04, 0.004, 'band'), dent=0.008, warp=0.012, lean=0.028, seed=seed)
    for k in range(11):
        th = -0.95 * math.pi + 1.9 * math.pi * k / 10
        z = 0.03
        r = crown(th, z) + 0.0045
        x, y = r * math.sin(th), -r * math.cos(th)
        x += 0.028 * (z / 0.3) ** 1.5
        n = Vector((math.sin(th), -math.cos(th), 0.0))
        disc(hpart, hframe @ Vector((x, y, z + 0.008)), hframe.to_3x3() @ n, 0.0105, 0.0022, segs=18, mat='gold')
    hpart.finish(subsurf=2)
    btn = Part(f'Sexton{X}_Buttons')
    buttons(btn, body, [((0.0, -0.5, z), (0.0, 1.0, 0.0)) for z in (0.34, 0.41, 0.48)], 0.01, 0.0045)
    btn.finish(subsurf=1)
    bpy.data.objects.remove(rail)


BUILDERS = {'A': build_A, 'B': build_B, 'C': build_C}


# --- The 1.8 m figure ---

def mannequin(name, collection):
    """A plain 1.8 m figure (clay) standing on the deck, for scale."""
    global COLLECTION
    keep = COLLECTION
    COLLECTION = collection
    part = Part(name)
    base = DECK
    ellipsoid(part, (0.0, 0.0, base + 1.685), (0.073, 0.092, 0.112), segs=16, rings=10, mat='clay')
    tube(part, [(0.0, 0.0, base + 1.47), (0.0, -0.01, base + 1.6)], 0.05, segs=12, mat='clay', per=2)
    t = Torso([(0.0, 0.0, base + 0.92), (0.0, 0.0, base + 1.1), (0.0, -0.005, base + 1.3), (0.0, 0.0, base + 1.47)],
              [(0.0, (0.16, 0.1, 0.11, 2.2)), (0.3, (0.145, 0.1, 0.1, 2.2)), (0.7, (0.17, 0.11, 0.1, 2.3)),
               (0.92, (0.19, 0.08, 0.08, 2.5)), (1.0, (0.08, 0.06, 0.06, 2.0))])
    loft(part, t.path, FRONT, t.section, segs=24, caps=('pole', 'pole'), mat='clay', pole_ext=(0.03, 0.01))
    for s in (-1.0, 1.0):
        tube(part, [(s * 0.2, 0.0, base + 1.44), (s * 0.24, 0.02, base + 1.15), (s * 0.25, -0.03, base + 0.86)], 0.042,
             segs=12, mat='clay', radius_end=0.03)
        ellipsoid(part, (s * 0.255, -0.035, base + 0.8), (0.022, 0.035, 0.06), segs=10, rings=6, mat='clay')
        tube(part, [(s * 0.09, 0.0, base + 0.95), (s * 0.095, -0.01, base + 0.5), (s * 0.1, 0.0, base + 0.09)], 0.07,
             segs=14, mat='clay', radius_end=0.045)
        loft(part, smooth_path([(s * 0.1, 0.05, base + 0.04), (s * 0.1, -0.06, base + 0.045), (s * 0.1, -0.16, base + 0.03)], 8, 3),
             UP, lambda tt, phi: superellipse(phi, 0.045, keys([(0.0, 0.05), (1.0, 0.025)], tt), 0.03, 2.4),
             segs=12, caps=('pole', 'pole'), mat='clay', pole_ext=(0.02, 0.02))
    obj = part.finish()
    PARTS.remove(obj)
    obj.data.materials.append(clay_material())
    white_col(obj.data)
    COLLECTION = keep
    return obj


# --- Scenes ---

def new_collection(name):
    coll = bpy.data.collections.get(name) or bpy.data.collections.new(name)
    if coll.name not in bpy.context.scene.collection.children:
        bpy.context.scene.collection.children.link(coll)
    return coll


def report_triangles(X, parts):
    dg = bpy.context.evaluated_depsgraph_get()
    cage = smooth1 = 0
    for o in parts:
        cage += sum(len(p.vertices) - 2 for p in o.data.polygons)
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        smooth1 += sum(len(p.vertices) - 2 for p in me.polygons)
        ev.to_mesh_clear()
    log(f'{X}: {len(parts)} parts, cage {cage} triangles, subdivided once {smooth1}')
    return cage, smooth1


def make_option(X):
    global COLLECTION, PARTS
    coll = new_collection(f'Sexton_{X}')
    COLLECTION, PARTS = coll, []
    BUILDERS[X]()
    root = bpy.data.objects.new(f'Sexton{X}', None)
    coll.objects.link(root)
    for obj in PARTS:
        assign_materials(obj, X)
        obj.parent = root
        if '--debug' in ARGS:
            dg = bpy.context.evaluated_depsgraph_get()
            me = obj.evaluated_get(dg).to_mesh()
            lo = Vector([min(v.co[i] for v in me.vertices) for i in range(3)])
            hi = Vector([max(v.co[i] for v in me.vertices) for i in range(3)])
            obj.evaluated_get(dg).to_mesh_clear()
            counts = {}
            for p in obj.data.polygons:
                counts[p.material_index] = counts.get(p.material_index, 0) + 1
            log(f'  {obj.name}: {len(obj.data.polygons)} faces, evaluated {tuple(round(c, 3) for c in lo)} .. '
                f'{tuple(round(c, 3) for c in hi)}; slots {[m.name for m in obj.data.materials]} {counts}')
            if obj.name in SHADE:
                a = np.array(SHADE[obj.name])
                col = obj.data.color_attributes.get('Col')
                raw = np.empty(4 * len(obj.data.loops), np.float32)
                col.data.foreach_get('color', raw)
                log(f'    shade: {len(a)} corners for {len(obj.data.loops)} loops, alpha {a.min():.2f}..{a.max():.2f} '
                    f'mean {a.mean():.2f}; stored alpha mean {raw[3::4].mean():.2f}, colors '
                    f'{[c.name + ":" + c.domain + ":" + c.data_type for c in obj.data.color_attributes]}')
    stats = report_triangles(X, PARTS)
    return dict(X=X, root=root, coll=coll, parts=list(PARTS), stats=stats)


def show_only(options, keep=()):
    for o in options.values():
        o['coll'].hide_render = o['X'] not in keep


def sky(name, stops, sun_dir, glows, strength=1.0, clouds=None):
    """A painted sky: colors by the view's height (stops: [(position 0..1 from straight down to straight up, hex)]),
    plus glows round the sun [(hex, sharpness, strength)]."""
    world = bpy.data.worlds.get(name) or bpy.data.worlds.new(name)
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    bg.inputs['Strength'].default_value = strength
    tc = nodes.new('ShaderNodeTexCoord')
    sep = nodes.new('ShaderNodeSeparateXYZ')
    links.new(tc.outputs['Generated'], sep.inputs['Vector'])
    mr = nodes.new('ShaderNodeMapRange')
    mr.inputs['From Min'].default_value = -1.0
    mr.inputs['From Max'].default_value = 1.0
    links.new(sep.outputs['Z'], mr.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(mr.outputs['Result'], ramp.inputs['Fac'])
    el = ramp.color_ramp.elements
    el[0].position, el[0].color = stops[0][0], lt.hex_color(stops[0][1])
    el[1].position, el[1].color = stops[-1][0], lt.hex_color(stops[-1][1])
    for pos, col in stops[1:-1]:
        e = el.new(pos)
        e.color = lt.hex_color(col)
    color = ramp.outputs['Color']
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(tc.outputs['Generated'], dot.inputs[0])
    dot.inputs[1].default_value = Vector(sun_dir).normalized()
    for gcol, power, gstr in glows:
        mx = nodes.new('ShaderNodeMath')
        mx.operation = 'MAXIMUM'
        links.new(dot.outputs['Value'], mx.inputs[0])
        mx.inputs[1].default_value = 0.0
        pw = nodes.new('ShaderNodeMath')
        pw.operation = 'POWER'
        links.new(mx.outputs['Value'], pw.inputs[0])
        pw.inputs[1].default_value = power
        mul = nodes.new('ShaderNodeMath')
        mul.operation = 'MULTIPLY'
        links.new(pw.outputs['Value'], mul.inputs[0])
        mul.inputs[1].default_value = gstr
        add = nodes.new('ShaderNodeMix')
        add.data_type = 'RGBA'
        add.blend_type = 'ADD'
        links.new(mul.outputs['Value'], add.inputs['Factor'])
        links.new(color, add.inputs['A'])
        add.inputs['B'].default_value = lt.hex_color(gcol)
        color = add.outputs['Result']
    if clouds:
        ccol, clit, scale, cover, amount = clouds
        squash = nodes.new('ShaderNodeVectorMath')
        squash.operation = 'MULTIPLY'
        links.new(tc.outputs['Generated'], squash.inputs[0])
        squash.inputs[1].default_value = (1.0, 1.0, 3.2)
        tex = nodes.new('ShaderNodeTexNoise')
        tex.inputs['Scale'].default_value = scale
        tex.inputs['Detail'].default_value = 6.0
        tex.inputs['Roughness'].default_value = 0.58
        links.new(squash.outputs['Vector'], tex.inputs['Vector'])
        cr = nodes.new('ShaderNodeMapRange')
        cr.inputs['From Min'].default_value = cover
        cr.inputs['From Max'].default_value = cover + 0.14
        links.new(tex.outputs['Fac'], cr.inputs['Value'])
        el = nodes.new('ShaderNodeMapRange')
        el.inputs['From Min'].default_value = 0.03
        el.inputs['From Max'].default_value = 0.16
        links.new(sep.outputs['Z'], el.inputs['Value'])
        mask = nodes.new('ShaderNodeMath')
        mask.operation = 'MULTIPLY'
        links.new(cr.outputs['Result'], mask.inputs[0])
        links.new(el.outputs['Result'], mask.inputs[1])
        amt = nodes.new('ShaderNodeMath')
        amt.operation = 'MULTIPLY'
        links.new(mask.outputs['Value'], amt.inputs[0])
        amt.inputs[1].default_value = amount
        lit = nodes.new('ShaderNodeMath')
        lit.operation = 'POWER'
        mx2 = nodes.new('ShaderNodeMath')
        mx2.operation = 'MAXIMUM'
        links.new(dot.outputs['Value'], mx2.inputs[0])
        mx2.inputs[1].default_value = 0.0
        links.new(mx2.outputs['Value'], lit.inputs[0])
        lit.inputs[1].default_value = 3.0
        cc = nodes.new('ShaderNodeMix')
        cc.data_type = 'RGBA'
        links.new(lit.outputs['Value'], cc.inputs['Factor'])
        cc.inputs['A'].default_value = lt.hex_color(ccol)
        cc.inputs['B'].default_value = lt.hex_color(clit)
        over = nodes.new('ShaderNodeMix')
        over.data_type = 'RGBA'
        links.new(amt.outputs['Value'], over.inputs['Factor'])
        links.new(color, over.inputs['A'])
        links.new(cc.outputs['Result'], over.inputs['B'])
        color = over.outputs['Result']
    links.new(color, bg.inputs['Color'])
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    return world


def sun_light(name, toward, strength, color, angle=1.0):
    obj = bpy.data.objects.get(name)
    if obj is None:
        obj = bpy.data.objects.new(name, bpy.data.lights.new(name, 'SUN'))
        bpy.context.scene.collection.objects.link(obj)
    obj.data.energy = strength
    obj.data.color = color
    obj.data.angle = math.radians(angle)
    if hasattr(obj.data, 'use_shadow_jitter'):
        obj.data.use_shadow_jitter = True
    obj.rotation_euler = (-Vector(toward).normalized()).to_track_quat('-Z', 'Y').to_euler()
    obj.hide_render = False
    return obj


def sun_dir(azimuth, elevation, north=Vector((-1.0, 0.0, 0.0)), east=Vector((0.0, 1.0, 0.0))):
    """The way to the sun at an azimuth (degrees from north toward east) and elevation, in the Lookout's frame (its
    front, -Y, faces west over the canyon, so north is -X and east +Y)."""
    a, e = math.radians(azimuth), math.radians(elevation)
    h = north * math.cos(a) + east * math.sin(a)
    return (h * math.cos(e) + UP * math.sin(e)).normalized()


def camera(name, loc, target, lens=50.0, ortho=None, fit='AUTO'):
    cam = bpy.data.objects.get(name)
    if cam is None:
        cam = bpy.data.objects.new(name, bpy.data.cameras.new(name))
        bpy.context.scene.collection.objects.link(cam)
    cam.location = Vector(loc)
    cam.rotation_euler = (Vector(target) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens = lens
    cam.data.sensor_fit = fit
    cam.data.clip_start = 0.05
    cam.data.clip_end = 30000.0
    if ortho:
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = ortho
    else:
        cam.data.type = 'PERSP'
    bpy.context.view_layer.update()
    return cam


LABELS = []


def label(text, cam, sx, sy, size_px, W, H, color=0xf0ece4, align='LEFT', depth=2.0):
    """Text on the frame: sx, sy from -1 (left, bottom) to 1, size in pixels of the W x H render."""
    data = cam.data
    aspect = W / H
    vertical = data.sensor_fit == 'VERTICAL' or (data.sensor_fit == 'AUTO' and H > W)
    if data.type == 'ORTHO':
        span = data.ortho_scale
        half_w, half_h = (span * 0.5 * aspect, span * 0.5) if vertical else (span * 0.5, span * 0.5 / aspect)
    else:
        half = depth * data.sensor_width * 0.5 / data.lens
        half_w, half_h = (half * aspect, half) if vertical else (half, half / aspect)
    curve = bpy.data.curves.new('_Label', 'FONT')
    curve.body = text
    curve.font = bpy.data.fonts.load(FONT, check_existing=True)
    curve.size = size_px / H * 2.0 * half_h
    curve.align_x = align
    obj = bpy.data.objects.new('_Label', curve)
    bpy.context.scene.collection.objects.link(obj)
    obj.parent = cam
    obj.matrix_parent_inverse = Matrix.Identity(4)
    obj.location = (sx * half_w, sy * half_h, -depth)
    curve.materials.append(unlit(f'_Label{color:06x}', color))
    LABELS.append(obj)
    return obj


def clear_labels():
    while LABELS:
        obj = LABELS.pop()
        data = obj.data
        bpy.data.objects.remove(obj)
        bpy.data.curves.remove(data)


def render(path, cam, W, H, world, samples=64, look='AgX - Medium High Contrast', exposure=0.0):
    scene = bpy.context.scene
    scene.camera = cam
    scene.world = world
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    ee = scene.eevee
    ee.taa_render_samples = 16 if QUICK else samples
    ee.use_shadows = True
    ee.use_raytracing = True
    ee.ray_tracing_method = 'SCREEN'
    ee.use_fast_gi = True
    ee.shadow_resolution_scale = 1.0
    ee.volumetric_end = 200.0
    scene.render.resolution_x, scene.render.resolution_y = W, H
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = look
    scene.view_settings.exposure = exposure
    scene.render.filepath = path
    t = time.time()
    bpy.ops.render.render(write_still=True)
    log(f'rendered {os.path.basename(path)} in {time.time() - t:.1f}s')
    return path


def load_pixels(path):
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = 'Non-Color'
    w, h = img.size
    px = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return px.reshape(h, w, 4)[::-1, :, :3]   # rows top-down


def save_pixels(path, rgb):
    lt.write_png(path, lt.to8(rgb))
    log(f'wrote {path}')


# --- Sets ---

def lookout_set():
    """The real Lookout (Art/Models/Buildings/Lookout.py), built without its occlusion bake; returns (collection,
    SOCKET_Sit's world matrix)."""
    coll = new_collection('Lookout')
    before = set(bpy.data.objects)
    saved = sys.argv[:]
    sys.argv = [sys.argv[0], '--', '--only=Lookout', '--no-ao']
    path = os.path.join(REPO, 'Art', 'Models', 'Buildings', 'Lookout.py')
    try:
        with open(path, encoding='utf-8') as file:
            exec(compile(file.read(), path, 'exec'), {'__name__': 'lookout_build', '__file__': path})
    finally:
        sys.argv = saved
    sit = None
    bpy.context.view_layer.update()
    for obj in set(bpy.data.objects) - before:
        for c in list(obj.users_collection):
            c.objects.unlink(obj)
        coll.objects.link(obj)
        if obj.name.startswith('UCX_'):
            obj.hide_render = True
        if obj.type == 'MESH':
            white_col(obj.data)
        if obj.name.startswith('SOCKET_Sit'):
            sit = obj.matrix_world.copy()
    # The bluff top round the shaft, and the canyon floor far below in haze.
    part = Part('_Bluff')
    bm = part.bm
    slot = part.slot('ground')
    pts = [bm.verts.new((x, y, -0.02)) for x, y in ((-40.0, -2.4), (40.0, -2.4), (40.0, 60.0), (-40.0, 60.0))]
    bm.faces.new(pts).material_index = slot
    bluff = part.finish(subsurf=0)
    PARTS.remove(bluff)
    for c in list(bluff.users_collection):
        c.objects.unlink(bluff)
    coll.objects.link(bluff)
    bluff.data.materials.append(lt.material('GroundGrass', name='SextonBluff', tint=0xe0c890))
    white_col(bluff.data)
    lt.box_uv(bluff, 'GroundGrass')
    return coll, sit


def ridge_set(center, heading, layers):
    """Silhouettes past the canyon (unlit, like the game's M_Backdrop): layers of (distance, base z, height, hex,
    seed, kind) spread over 160 degrees round heading."""
    coll = new_collection('Ridges')
    objs = []
    for k, (dist, base, h, color, seed, kind) in enumerate(layers):
        bm = bmesh.new()
        n = 240
        top, bot = [], []
        for i in range(n + 1):
            a = heading - 1.4 + 2.8 * i / n
            u = i / n * 30.0
            if kind == 'mesa':
                prof = 0.08 + smoothstep(0.0, 0.07, vnoise(u * 0.55, 0.0, 0.0, seed) - 0.22) * \
                    (0.85 + 0.15 * vnoise(u * 2.2, 1.0, 0.0, seed + 1))
            elif kind == 'wall':
                prof = 0.75 + 0.18 * vnoise(u * 0.6, 0.0, 0.0, seed) + 0.07 * vnoise(u * 3.0, 0.0, 0.0, seed + 1)
            else:
                prof = 0.45 + 0.4 * abs(vnoise(u * 0.45, 0.0, 0.0, seed)) + 0.15 * vnoise(u * 1.8, 0.0, 0.0, seed + 2)
            x, y = center.x + math.cos(a) * dist, center.y + math.sin(a) * dist
            top.append(bm.verts.new((x, y, base + h * prof)))
            bot.append(bm.verts.new((x, y, base - 600.0)))
        for i in range(n):
            bm.faces.new((bot[i], bot[i + 1], top[i + 1], top[i]))
        mesh = bpy.data.meshes.new(f'_Ridge{k}')
        bm.to_mesh(mesh)
        bm.free()
        obj = bpy.data.objects.new(f'_Ridge{k}', mesh)
        coll.objects.link(obj)
        mesh.materials.append(unlit(f'_Ridge{k}Mat', color))
        objs.append(obj)
    floor = bpy.data.meshes.new('_CanyonFloor')
    s = 30000.0
    floor.from_pydata([(-s, -s, -92.0), (s, -s, -92.0), (s, s, -92.0), (-s, s, -92.0)], [], [(0, 1, 2, 3)])
    fo = bpy.data.objects.new('_CanyonFloor', floor)
    coll.objects.link(fo)
    floor.materials.append(unlit('_CanyonFloorMat', 0x9c8a74))
    objs.append(fo)
    return coll, objs


def recolor(obj, color):
    mat = obj.data.materials[0]
    mat.node_tree.nodes['Emission'].inputs['Color'].default_value = lt.hex_color(color)


def studio_set():
    """A warm grey cyclorama with a plank deck strip under the rail: for the turnaround and the comparison."""
    coll = new_collection('Studio')
    bm = bmesh.new()
    prof = [(-14.0, DECK - 0.004)]
    R = 4.0
    for k in range(13):
        a = math.pi * 0.5 * k / 12
        prof.append((5.0 + R * math.sin(a), DECK - 0.004 + R * (1.0 - math.cos(a))))
    prof.append((5.0 + R, DECK + 14.0))
    rows = []
    for x in (-30.0, 30.0):
        rows.append([bm.verts.new((x, y, z)) for y, z in prof])
    for i in range(len(prof) - 1):
        bm.faces.new((rows[0][i], rows[1][i], rows[1][i + 1], rows[0][i + 1]))
    mesh = bpy.data.meshes.new('_Cyc')
    bm.to_mesh(mesh)
    bm.free()
    cyc = bpy.data.objects.new('_Cyc', mesh)
    coll.objects.link(cyc)
    for p in mesh.polygons:
        p.use_smooth = True
    mat = bpy.data.materials.new('_CycMat')
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x8d877d)
    bsdf.inputs['Roughness'].default_value = 0.95
    mesh.materials.append(mat)
    return coll


def stand_in(name, x0, x1, coll):
    """A stand-in of the Lookout's front rail (Lookout.py's railing: square posts, a top rail at 1.05 m, a mid rail,
    saltire braces, on the house trim sheet) and a strip of deck boards. Posts fall every 1.33 m from x0."""
    import looter_ruins as lr
    before = set(bpy.data.objects)
    m = lr.Model(name, seed=601)
    lr.railing(m, (x0, -0.085, DECK), (x1, -0.085, DECK), -DECK, face=(0.0, 1.0, 0.0), post_every=1.33, cross=True,
               seed=601)
    obj = m.finish(ao=False, preview=False, fallback=None)
    for o in set(bpy.data.objects) - before:
        for c in list(o.users_collection):
            c.objects.unlink(o)
        coll.objects.link(o)
        if o.name.startswith('UCX_'):
            o.hide_render = True
    white_col(obj.data)
    deck = Part(name + 'Deck')
    for k in range(5):
        y = -0.95 + k * 0.2
        box(deck, ((x0 + x1) * 0.5 + (k % 2) * 0.3, y, DECK - 0.025), (x1 - x0 + 0.6, 0.192, 0.05), mat='planks')
    d = deck.finish(subsurf=0)
    PARTS.remove(d)
    for c in list(d.users_collection):
        c.objects.unlink(d)
    coll.objects.link(d)
    d.data.materials.append(lt.material('WoodPlanks', name='SextonDeck'))
    white_col(d.data)
    lt.box_uv(d, 'WoodPlanks', along='long')
    return [obj, d]


def set_root(option, matrix):
    option['root'].matrix_world = matrix
    bpy.context.view_layer.update()


# --- The renders ---

def golden_world(sun):
    return sky('SextonGolden', [(0.0, 0x6b6050), (0.47, 0x8e8270), (0.5, 0xe9d2a6), (0.53, 0xd9d6c4),
                                (0.62, 0xa9bfd2), (1.0, 0x5d84b6)], sun,
               [(0xffe0a8, 24.0, 0.9), (0xfff2d0, 600.0, 6.0)], strength=1.0,
               clouds=(0xe4e0dc, 0xfff0d8, 3.0, 0.56, 0.6))


def studio_world():
    return sky('SextonStudio', [(0.0, 0x5c5850), (0.5, 0x9c968c), (1.0, 0x8fa0b4)], (0.3, -0.8, 0.5),
               [(0xffe6c0, 4.0, 0.25)], strength=0.9)


def dusk_world(sun):
    return sky('SextonDusk', [(0.0, 0x1a1418), (0.47, 0x2e2630), (0.5, 0x7c7290), (0.512, 0x6e6478),
                              (0.56, 0x54484e), (0.7, 0x3e3438), (1.0, 0x2a2428)], sun,
               [(0xff9a48, 6.0, 0.5), (0xffc27a, 60.0, 1.3), (0xfff2dc, 1400.0, 40.0)], strength=1.0,
               clouds=(0x8c7678, 0xe8b4a0, 3.2, 0.52, 0.85))


def do_turnaround(option, studio, rail_parts, ref, W=600, H=1080):
    X = option['X']
    cam = camera('_TurnCam', (0.0, -12.0, 0.22), (0.0, 0.0, 0.22), ortho=2.9, fit='VERTICAL')
    world = studio_world()
    sun_light('_Sun', (-0.55, -0.75, 0.62), 3.2, (1.0, 0.9, 0.78), angle=2.0)
    panels = []
    tmp = os.path.join(CACHE_DIR, '_panels')    # the strip's parts, kept out of the review folder
    os.makedirs(tmp, exist_ok=True)
    # The 1.8 m figure, alone, at the same scale.
    show_only(OPTIONS, keep=())
    for o in rail_parts:
        o.hide_render = True
    ref.hide_render = False
    ref.matrix_world = Matrix.Identity(4)
    label('1.8 m', cam, 0.0, (DECK + 1.86 - 0.22) / 1.45, 30, 400, H, align='CENTER')
    panels.append(render(os.path.join(tmp, f'{X}_ref.png'), cam, 400, H, world, samples=48))
    clear_labels()
    ref.hide_render = True
    for o in rail_parts:
        if not o.name.startswith('UCX_'):
            o.hide_render = False
    show_only(OPTIONS, keep=(X,))
    for view, angle in (('FRONT', 0.0), ('SIDE', -90.0), ('BACK', 180.0)):
        m = Matrix.Rotation(math.radians(angle), 4, 'Z')
        set_root(option, m)
        for o in rail_parts:
            o.matrix_world = m
        label(view, cam, 0.0, -0.9, 30, W, H, align='CENTER')
        if view == 'FRONT':
            label(f'{X}  {TITLES[X].upper()}', cam, -0.92, 0.9, 34, W, H, color=0xf2a64a)
        panels.append(render(os.path.join(tmp, f'{X}_{view}.png'), cam, W, H, world, samples=48))
        clear_labels()
    set_root(option, Matrix.Identity(4))
    for o in rail_parts:
        o.matrix_world = Matrix.Identity(4)
    strip = np.concatenate([load_pixels(p) for p in panels], axis=1)
    save_pixels(os.path.join(OUT_DIR, f'Sexton_{X}_Turnaround.png'), strip)


TITLES = {'A': 'The Gentleman', 'B': 'The Undertaker', 'C': 'The Ferryman'}
OPTIONS = {}
# Where the hero camera stands and looks, and where the golden sun comes from, in Sexton space: on the deck at his
# front right, a standing player's eye height; the sun low from his front left.
HERO_EYE, HERO_AIM = Vector((-1.45, -3.05, 0.62)), Vector((0.06, -0.24, 0.30))
HERO_SUN = Vector((0.766, -0.643, math.tan(math.radians(15.0)))).normalized()


def blacken(option):
    """Swaps every material of the option for flat black (the cold open draws his seated model in black); returns
    a function that puts them back."""
    black = unlit('_Black', 0x000000)
    saved = []
    for obj in option['parts']:
        mats = list(obj.data.materials)
        saved.append((obj, mats))
        for i in range(len(mats)):
            obj.data.materials[i] = black

    def restore():
        for obj, mats in saved:
            for i, m in enumerate(mats):
                obj.data.materials[i] = m
    return restore


def to_screen(cam, point, W, H):
    from bpy_extras.object_utils import world_to_camera_view
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = W, H
    co = world_to_camera_view(scene, cam, Vector(point))
    return co.x * 2.0 - 1.0, co.y * 2.0 - 1.0


def do_hero(option, sit, sets):
    X = option['X']
    show_only(OPTIONS, keep=(X,))
    set_root(option, sit)
    rot = sit.to_3x3()
    sun = rot @ HERO_SUN
    sun_light('_Sun', sun, 4.2, (1.0, 0.84, 0.64), angle=1.2)
    world = golden_world(sun)
    for obj, color in zip(sets['ridges'], (0xbca592, 0xc4b5aa, 0xbdbabf, 0xc7c9d0, 0xae9a86)):
        recolor(obj, color)
    W, H = 1080, 1350
    cam = camera("_HeroCam", sit @ HERO_EYE, sit @ HERO_AIM, lens=45.0)
    label(f'{X}  {TITLES[X].upper()}', cam, -0.9, 0.9, 40, W, H, color=0xf2a64a)
    render(os.path.join(OUT_DIR, f'Sexton_{X}_Hero.png'), cam, W, H, world, samples=96, exposure=-0.1)
    clear_labels()
    set_root(option, Matrix.Identity(4))


def do_dusk(option, sit, sets):
    """The cold open: dusk on Ransom's Point through Ellis's eyes, low on the deck; far along the railing his seated
    model drawn in black against the setting sun (the Dusk state: 4 degrees up at a bearing of 252, deep amber haze,
    the ridges darkened and warmed)."""
    X = option['X']
    show_only(OPTIONS, keep=(X,))
    set_root(option, sit)
    sun = sun_dir(252.0, 4.0)
    sun_light('_Sun', sun, 2.6, (1.0, 0.6, 0.34), angle=1.2)
    world = dusk_world(sun)
    for obj, color in zip(sets['ridges'], (0x3a2620, 0x4a3634, 0x4e4454, 0x5c566a, 0x3a2c30)):
        recolor(obj, color)
    restore = blacken(option)
    W, H = 1920, 1080
    eye = sit @ Vector((-0.55, -2.9, DECK + 1.0))
    cam = camera('_DuskCam', eye, sit @ Vector((0.0, -0.1, 0.33)), lens=30.0)
    render(os.path.join(OUT_DIR, f'Sexton_{X}_Dusk.png'), cam, W, H, world, samples=64, exposure=0.2)
    restore()
    set_root(option, Matrix.Identity(4))


def do_compare(sets, ref):
    show_only(OPTIONS, keep=tuple(OPTIONS))
    seats = {'A': -1.33, 'B': 0.0, 'C': 1.33}
    for X, x in seats.items():
        set_root(OPTIONS[X], Matrix.Translation((x, 0.0, 0.0)))
    ref.hide_render = False
    ref.matrix_world = Matrix.Translation((-2.45, -0.55, 0.0))
    sun = HERO_SUN.copy()
    sun_light('_Sun', sun, 3.8, (1.0, 0.86, 0.68), angle=1.5)
    world = studio_world()
    W, H = 1920, 1080
    cam = camera('_CompareCam', (-2.45, -7.7, 0.72), (-0.32, -0.2, 0.12), lens=45.0)
    for X, x in seats.items():
        sx, sy = to_screen(cam, (x + 0.05, -0.45, DECK - 0.02), W, H)
        label(f'{X}  {TITLES[X].upper()}', cam, sx, sy - 0.06, 30, W, H, color=0xf2a64a, align='CENTER')
    sx, sy = to_screen(cam, (-2.45, -0.55, DECK + 1.86), W, H)
    label('1.8 m', cam, sx, sy + 0.02, 26, W, H, align='CENTER')
    render(os.path.join(OUT_DIR, 'Sexton_Comparison.png'), cam, W, H, world, samples=96)
    clear_labels()
    ref.hide_render = True
    for X in seats:
        set_root(OPTIONS[X], Matrix.Identity(4))


def do_closeups(option, rail_parts, out_dir):
    """Checking renders (not deliverables): the upper body at three quarters, the face, the lap and hands, the back."""
    X = option['X']
    show_only(OPTIONS, keep=(X,))
    for o in rail_parts:
        o.hide_render = False
        o.matrix_world = Matrix.Identity(4)
    sun_light('_Sun', (-0.55, -0.75, 0.62), 3.2, (1.0, 0.9, 0.78), angle=2.0)
    world = studio_world()
    shots = [('upper', (-0.8, -1.5, 0.95), (0.0, -0.12, 0.66), 50.0),
             ('face', (-0.28, -0.72, 0.93), (0.0, -0.08, 0.86), 50.0),
             ('lap', (-0.35, -1.05, 0.7), (0.04, -0.36, 0.2), 50.0),
             ('back', (0.9, 1.6, 0.6), (0.0, 0.05, 0.25), 40.0)]
    panels = []
    for name, eye, aim, lens in shots:
        cam = camera('_CloseCam', eye, aim, lens=lens)
        panels.append(render(os.path.join(out_dir, f'_close_{X}_{name}.png'), cam, 720, 900, world, samples=32))
    strip = np.concatenate([load_pixels(p) for p in panels], axis=1)
    save_pixels(os.path.join(out_dir, f'Close_{X}.png'), strip)


def main():
    want = [x for x in 'ABC' if x in ARGS and x in BUILDERS] or [x for x in 'ABC' if x in BUILDERS]
    renders = [r for r in ('hero', 'turn', 'dusk', 'compare') if r in ARGS] or ['hero', 'turn', 'dusk', 'compare']
    if 'compare' in renders and len(BUILDERS) == 3:
        build = list('ABC')
    else:
        build = want
        renders = [r for r in renders if r != 'compare']
    bpy.context.scene.render.fps = 24
    for X in build:
        OPTIONS[X] = make_option(X)
    if '--preview' not in ARGS and not lt.want_preview():
        return
    os.makedirs(OUT_DIR, exist_ok=True)
    global COLLECTION, PARTS
    COLLECTION, PARTS = new_collection('Sets'), []
    ref = mannequin('_Ref18', new_collection('Reference'))
    ref.hide_render = True
    studio = studio_set()
    rail_parts = stand_in('_StandIn', -1.895, 2.095, studio)
    if 'close' in ARGS:
        scratch = os.environ.get('SEXTON_SCRATCH', os.path.join(REPO, 'Intermediate', 'SextonConcepts'))
        os.makedirs(scratch, exist_ok=True)
        for X in want:
            do_closeups(OPTIONS[X], rail_parts, scratch)
        return
    if 'turn' in renders:
        for X in want:
            do_turnaround(OPTIONS[X], studio, rail_parts, ref)
    if 'compare' in renders:
        for o in rail_parts:
            o.hide_render = True
        long_rail = stand_in('_LongRail', -3.225, 3.425, studio)
        do_compare(None, ref)
        for o in long_rail:
            o.hide_render = True
    if 'hero' in renders or 'dusk' in renders:
        studio.hide_render = True
        lookout, sit = lookout_set()
        ridges_coll, ridges = ridge_set(sit.translation, math.atan2(-1.0, 0.0),
                                        [(380.0, -85.0, 50.0, 0xc7a88c, 11, 'wall'),
                                         (1600.0, -60.0, 110.0, 0xb6a49a, 23, 'mesa'),
                                         (4200.0, -40.0, 260.0, 0xa9a8ac, 37, 'ridge'),
                                         (9000.0, -20.0, 480.0, 0xb4b6be, 41, 'ridge')])
        sets = dict(ridges=ridges)
        for X in want:
            if 'hero' in renders:
                do_hero(OPTIONS[X], sit, sets)
            if 'dusk' in renders:
                do_dusk(OPTIONS[X], sit, sets)
    log('done')


main()
