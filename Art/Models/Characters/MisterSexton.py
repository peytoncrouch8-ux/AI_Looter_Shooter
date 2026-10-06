"""Mister Sexton, the game's first human (Docs/Story.md, "Antagonist: Mister Sexton"; Docs/Areas/RansomsRest.md, step
15): the user's pick from Art/Backlog/Creatures/SextonConcepts.py, option A, the Gentleman (2026-10-06). A scripted
model file (Art/README.md) with two models:

  MisterSexton  SM_MisterSexton: seated on a rail, legs crossed knee over knee, the ledger on his right knee, his left
                hand spread on its page, a dip pen in his right. A fitted black frock coat buttoned to the throat: a
                waist seam, small rolled lapels, a stand-and-fall collar, nine buttons, its skirt and tails draped over
                the rail (Blender's cloth simulation; see the drape below). A starched wing collar (its points folded
                over, the cloth's thickness on both faces, its top edge rolled) and a tied bow tie, charcoal trousers
                breaking over pointed boots, long pale hands with knuckles and tendons, a tall stovepipe whose brim
                curls up at the sides to a rolled edge, a band and a flat bow. Under the hat a gaunt man's head
                (brow ridge, deep eye sockets, a long nose, hard cheekbones over hollow cheeks, thin closed lips, a
                narrow pointed chin, a jaw running back under the ears, an Adam's apple): his face is in shadow
                down to his upper lip, and only the point of his chin catches the light. A static mesh with a posed
                idle only: no rig. The script lays his hands on the page: each resting finger bends at its knuckle
                until it touches the page, the pen hand's index, thumb and middle finger are aimed at the pen, and no
                finger goes through the ledger.
  SextonLedger  SM_SextonLedger: the open ledger, its own prop: oxblood boards and a ribbon, cream pages, brass
                corners, the ribbon weighted with a tarnished coin.

Pivot and attaching. Sexton's pivot is the seat point on the rail's top under him, his front -Y as every model's: it
is the Lookout's SOCKET_Sit (Art/Models/Buildings/Lookout.py; its front faces into the deck), so attach
SM_MisterSexton to the Lookout's Sit socket, snapped to target, and he sits on its front rail facing the deck; the
deck is 1.05 m below the pivot. The ledger's pivot is the middle of its boards' underside, its front (-Y) toward the
reader: attach SM_SextonLedger to Sexton's Ledger socket, snapped to target. Sockets:
  SOCKET_Ledger   on his right knee, where the ledger rests (its front toward him)
  SOCKET_Speaker  at his chin, facing his front: where his captions come from

Materials (MI_<slot> on the textured masters; vertex alpha is baked occlusion everywhere):
  SextonWool      M_World with no texture set: a flat warm near-black (#2A2826) on the master's default ORM,
                  roughness 0.8 + RoughnessOffset 0.1, Specular 0.15: matte wool that reflects little. Coat, hat,
                  bow tie, boots, the pen's holder. (In the game the cooler, more reflective wool caught the blue
                  sky: the trousers read pale blue-grey and the coat navy.)
  SextonTrousers  the same, warm charcoal (#343230): the trousers.
  SextonLinen     Polymer tinted #F4EFE2: the wing collar and the shirt cuffs.
  SextonSkin      Polymer tinted #C9B9AA (pale but alive), DiffuseAO 1: his lips, his chin and his hands. Toward
                  the shadow's edge its vertex alpha fades to black over 2 cm (measured across the edge), so with
                  DiffuseAO 1 the skin darkens into the shadow without a seam and the lips sink into it.
  SextonShadow    M_Backdrop (unlit): #141110 at Brightness 1. His face above the shadow line (SHADOW_LINE: across
                  his upper lip, down round his mouth and chin), his jaw, his throat under the jaw's edge, his ears
                  and the back of his head and neck: it never lights, at noon or at dusk ("his face in shadow even
                  at noon"); only the head's outline shows its forms.
  SextonMetal     MetalWorn tinted #A88A52, old tarnished brass: the buttons, the nib, the coin and the ledger's corners.
  The ledger adds SextonLedgerCover (Polymer #6E3226, oxblood) and SextonLedgerPages (Polymer #E9DFC6).
For the cold open (his seated model drawn black), the scene swaps every slot for an unlit black instance.

Nanite off (it's a character seen up close): SM_MisterSexton LODs 50% and 20%, one convex hull (UCX) round his body
and legs; SM_SextonLedger LODs 50% and 25%, no collision. Every fold is geometry: no normal map, no new texture.

The drape. His skirt and tails come from a cloth simulation over his body and the rail (two panels from the waist
seam, the front edges parting over the crossed legs, the tails falling behind the rail), run at a fine grid and kept
at every second row and column. The settled cloth is stored beside this file in MisterSexton.drape.json with a digest
of everything it depends on (the starting cloth, the colliders, the settings), so an export never runs it again; a
change to the body changes the digest, and the next run simulates and rewrites the file (--resim forces it).

    blender -b --factory-startup --python Art/Models/Characters/MisterSexton.py -- [--preview] [--stats] [--resim]
--preview renders Saved/ArtPreviews/RansomsRest/Sexton/: on the real Lookout in a golden sun, backlit by the Day
state's sun as at Main 2, the cold open's black silhouette at dusk, close-ups of the hands and ledger and of the
face's shadow edge, his head three-quarter as a player standing on the deck sees it (Sexton_Head), the LOD sheet
(Blender's decimation standing in for Unreal's) and the game mesh beside the concept.
"""
import hashlib
import json
import math
import os
import sys
import time

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_model as lm
import looter_textures as lt

REPO = lt.REPO
HERE = os.path.dirname(os.path.abspath(__file__))
DRAPE_FILE = os.path.join(HERE, 'MisterSexton.drape.json')
OUT_DIR = os.path.join(REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Sexton')
CONCEPT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'Sexton', 'Sexton_A_Hero.png')
FONT = os.path.join(REPO, 'Content', 'UI', 'Fonts', 'ChakraPetch-Bold.ttf')
ARGS = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
DECK = -1.05              # the deck under the rail's top (the Lookout's RAIL)
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


def bump(d, width):
    """1 at d = 0, falling smoothly to 0 at width."""
    x = abs(d) / width
    return 0.0 if x >= 1.0 else (1.0 - x * x) ** 2


def wrap(a):
    return (a + math.pi) % (2.0 * math.pi) - math.pi


def _hash(ix, iy, iz, seed):
    h = (ix * 374761393 + iy * 668265263 + iz * 1442695041 + seed * 3266489917) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((h ^ (h >> 16)) & 0xFFFF) / 32767.5 - 1.0


def vnoise(x, y=0.0, z=0.0, seed=0):
    """Smooth value noise in -1..1, deterministic."""
    ix, iy, iz = math.floor(x), math.floor(y), math.floor(z)
    fx, fy, fz = x - ix, y - iy, z - iz
    ux, uy, uz = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy), fz * fz * (3 - 2 * fz)

    def h(a, b, c):
        return _hash(ix + a, iy + b, iz + c, seed)
    x00, x10 = lerp(h(0, 0, 0), h(1, 0, 0), ux), lerp(h(0, 1, 0), h(1, 1, 0), ux)
    x01, x11 = lerp(h(0, 0, 1), h(1, 0, 1), ux), lerp(h(0, 1, 1), h(1, 1, 1), ux)
    return lerp(lerp(x00, x10, uy), lerp(x01, x11, uy), uz)


# --- Paths, frames, sections ---

def catmull(points, per=6):
    """A Catmull-Rom spline through points, per samples a span."""
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


def arc_params(path):
    dist = [0.0]
    for a, b in zip(path, path[1:]):
        dist.append(dist[-1] + (b - a).length)
    total = max(dist[-1], 1e-9)
    return [d / total for d in dist], total


def resample(points, count, weight=None):
    """count points along a polyline, evenly by arc length, or denser where weight(t) (t = arc share) is larger."""
    P = [Vector(p) for p in points]
    ts, _ = arc_params(P)
    cum = [0.0]
    for k in range(1, len(P)):
        w = 1.0 if weight is None else weight((ts[k - 1] + ts[k]) * 0.5)
        cum.append(cum[-1] + w * (ts[k] - ts[k - 1]))
    out = []
    k = 0
    for i in range(count):
        target = cum[-1] * i / (count - 1)
        while k < len(P) - 2 and cum[k + 1] < target:
            k += 1
        span = max(cum[k + 1] - cum[k], 1e-12)
        out.append(P[k].lerp(P[k + 1], min(max((target - cum[k]) / span, 0.0), 1.0)))
    return out


def path_through(points, count, weight=None, per=10):
    """A smooth path through points with count rings: dense where weight(t) is large."""
    return resample(resample(catmull(points, per), 400), count, weight)


def dense_near(*zones, base=1.0):
    """A ring-density function: base, plus each zone (t, width, extra) as a bump."""
    return lambda t: base + sum(extra * bump(t - tc, w) for tc, w, extra in zones)


def rm_frames(path, up):
    """Rotation-minimizing frames (tangent, normal, side = tangent x normal), the first normal near up."""
    n = len(path)
    tangents = [(path[min(k + 1, n - 1)] - path[max(k - 1, 0)]).normalized() for k in range(n)]
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


def keys(table, t):
    """Smoothly interpolated (t, value or tuple) keys."""
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
    """The point at angle phi (0 = +y, 90 degrees = +x) on a superellipse."""
    c, s = math.cos(phi), math.sin(phi)
    e = 2.0 / n
    x = rx * math.copysign(abs(s) ** e, s)
    ry = ry_pos if c >= 0.0 else (ry_pos if ry_neg is None else ry_neg)
    return x, ry * math.copysign(abs(c) ** e, c)


def grow(xy, extra):
    """A section point pushed out from its centre by extra."""
    x, y = xy[0], xy[1]
    d = max(math.hypot(x, y), 1e-6)
    k = 1.0 + extra / d
    return (x * k, y * k) + tuple(xy[2:])


def wrinkles(seed, zones):
    """Cloth folds: zones of (t, t width, phi, phi width, depth, ridges) across a sleeve or a leg, a little noise."""
    def folds(t, phi):
        out = 0.0
        for k, (tc, tw, pc, pw, amp, ridges) in enumerate(zones):
            w = bump(t - tc, tw) * bump(wrap(phi - pc), pw)
            if w > 0.0:
                wave = math.sin((t - tc) / tw * math.pi * ridges + 1.3 * k + 2.0 * math.sin(phi * 2.0 + seed))
                out += amp * w * (0.55 + 0.45 * wave)
        return out + 0.0012 * vnoise(t * 9.0, phi * 1.6, 0.0, seed)
    return folds


# --- Meshes ---

PARTS = []


class Part:
    """A mesh being built from lofts and primitives, its faces tagged with material slot names."""

    def __init__(self, name):
        self.name = name
        self.bm = bmesh.new()
        self.slots = []

    def slot(self, key):
        if key not in self.slots:
            self.slots.append(key)
        return self.slots.index(key)

    def finish(self, keep=True):
        mesh = bpy.data.meshes.new(self.name)
        bmesh.ops.remove_doubles(self.bm, verts=self.bm.verts, dist=1e-6)
        self.bm.normal_update()
        self.bm.to_mesh(mesh)
        self.bm.free()
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        for p in mesh.polygons:
            p.use_smooth = True
        for key in self.slots:
            mesh.materials.append(MATERIALS[key]())
        if keep:
            PARTS.append(obj)
        return obj


def loft(part, path, up, section, segs=12, caps=(None, None), mat='wool', pole_ext=(0.0, 0.0), face_mat=None,
         arc=None, lip=None, warp=0.0):
    """Sweeps a ring of segs points along path. section(t, phi) -> (x, y[, z]) offsets in the frame's (side, normal)
    plane at arc share t and angle phi (0 toward the normal, which starts near up); a third value moves the point
    along the path. caps: None or 'pole' per end. arc=(a0, a1) makes an open strip. lip=(inset, depth) turns an open
    end's edge inward and back into the tube: the cloth's thickness at a cuff or a hem. Returns the rings."""
    bm = part.bm
    frames = rm_frames(path, up)
    ts, _ = arc_params(path)
    closed = arc is None
    count = segs if closed else segs + 1
    angles = [2.0 * math.pi * j / segs - warp * math.sin(2.0 * math.pi * j / segs) for j in range(segs)] if closed \
        else [lerp(arc[0], arc[1], j / segs) for j in range(segs + 1)]
    rings = []
    for p, (tan, nr, side), t in zip(path, frames, ts):
        ring = []
        for phi in angles:
            xy = section(t, phi)
            ring.append(p + side * xy[0] + nr * xy[1] + (tan * xy[2] if len(xy) > 2 else Vector()))
        rings.append(ring)
    if lip is not None:
        inset, depth = lip
        last, tan = rings[-1], frames[-1][0]
        center = sum(last, Vector()) / len(last)
        ring_a = [v + (center - v).normalized() * inset + tan * 0.0005 for v in last]
        ring_b = [v - tan * depth + (center - v).normalized() * 0.002 for v in ring_a]
        rings += [ring_a, ring_b]
        ts = ts + [1.0, 1.0]
    vrings = [[bm.verts.new(p) for p in ring] for ring in rings]
    default = part.slot(mat)
    step = (2.0 * math.pi / segs) if closed else (arc[1] - arc[0]) / segs
    for k in range(len(vrings) - 1):
        a, b = vrings[k], vrings[k + 1]
        for j in range(segs if closed else count - 1):
            j1 = (j + 1) % count
            f = bm.faces.new((a[j], a[j1], b[j1], b[j]))
            mid = (angles[j] + (angles[j1] if j1 else 2.0 * math.pi)) * 0.5 if closed else angles[j] + step * 0.5
            f.material_index = part.slot(face_mat((ts[k] + ts[k + 1]) * 0.5, mid, k)) \
                if face_mat is not None else default
    for end, cap in enumerate(caps):
        if cap is None or not closed:
            continue
        ring = vrings[0] if end == 0 else vrings[len(path) - 1]
        tan = frames[0][0] if end == 0 else frames[-1][0]
        center = sum((v.co for v in ring), Vector()) / segs
        pole = bm.verts.new(center + tan * (pole_ext[end] if end else -pole_ext[end]))
        index = part.slot(face_mat(0.0 if end == 0 else 1.0, 0.0, 0 if end == 0 else len(path) - 1)) \
            if face_mat is not None else default
        for j in range(segs):
            j1 = (j + 1) % segs
            f = bm.faces.new((ring[j1], ring[j], pole) if end == 0 else (ring[j], ring[j1], pole))
            f.material_index = index
    return vrings


def tube(part, points, radius, segs=6, mat='wool', caps=('pole', 'pole'), per=4, radius_end=None, count=None):
    path = catmull(points, per) if len(points) > 2 else [Vector(points[0]).lerp(Vector(points[1]), k / per)
                                                       for k in range(per + 1)]
    if count:
        path = resample(path, count)
    r1 = radius if radius_end is None else radius_end
    return loft(part, path, UP, lambda t, phi: (lerp(radius, r1, t) * math.sin(phi), lerp(radius, r1, t) * math.cos(phi)),
                segs=segs, caps=caps, mat=mat, pole_ext=(radius * 0.45, r1 * 0.45))


def ellipsoid(part, center, radii, frame3=None, segs=8, rings=6, mat='wool'):
    frame3 = frame3 or Matrix.Identity(3)
    m = Matrix.Translation(Vector(center)) @ frame3.to_4x4()
    path = [m @ Vector((0.0, 0.0, -radii[2] * math.cos(math.pi * i / rings))) for i in range(1, rings)]

    def section(t, phi):
        z = -math.cos(math.pi * lerp(1.0 / rings, 1.0 - 1.0 / rings, t))
        r = math.sqrt(max(1.0 - z * z, 0.0))
        return r * radii[0] * math.sin(phi), r * radii[1] * math.cos(phi)
    ext = radii[2] * (1.0 - math.cos(math.pi / rings))
    loft(part, path, frame3 @ Vector((0.0, 1.0, 0.0)), section, segs=segs, caps=('pole', 'pole'), mat=mat,
         pole_ext=(ext, ext))


def dome(part, center, normal, radius, height, segs=6, mat='metal'):
    """A button: a low dome on its base ring."""
    n = Vector(normal).normalized()
    a = n.orthogonal().normalized()
    b = n.cross(a)
    c = Vector(center)
    bm = part.bm
    slot = part.slot(mat)
    base = [bm.verts.new(c + (a * math.cos(2 * math.pi * k / segs) + b * math.sin(2 * math.pi * k / segs)) * radius
                         - n * height * 0.3) for k in range(segs)]
    mid = [bm.verts.new(c + (a * math.cos(2 * math.pi * k / segs) + b * math.sin(2 * math.pi * k / segs)) * radius * 0.75
                        + n * height * 0.55) for k in range(segs)]
    top = bm.verts.new(c + n * height)
    for k in range(segs):
        k1 = (k + 1) % segs
        bm.faces.new((base[k], base[k1], mid[k1], mid[k])).material_index = slot
        bm.faces.new((mid[k], mid[k1], top)).material_index = slot


def surface_hit(obj, origin, direction, distance=3.0):
    dg = bpy.context.evaluated_depsgraph_get()
    tree = BVHTree.FromObject(obj, dg)
    hit, normal, _, _ = tree.ray_cast(Vector(origin), Vector(direction).normalized(), distance)
    return None if hit is None else (hit, normal.normalized())


# --- Materials ---

def _flat_world(name, tint, roughness=0.8, diffuse_ao=0.4, specular=None, roughness_offset=None):
    """M_World without a texture set: the master's white base x Tint on its default ORM (roughness 0.8). specular and
    roughness_offset set the master's Specular (0.5 by default: 4% reflectance) and RoughnessOffset (added to the
    roughness, clamped) and reach the manifest as its scalars; the preview's BSDF follows them. The preview darkens
    the base by the vertex alpha as the master does (DiffuseAO of it)."""
    mat = bpy.data.materials.get(name)
    if mat is not None:
        return mat
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Roughness'].default_value = min(roughness + (roughness_offset or 0.0), 1.0)
    if specular is not None:
        # The master's Specular is Unreal's (reflectance 0.08 x it), as Blender's Specular IOR Level is at IOR 1.5.
        bsdf.inputs['Specular IOR Level'].default_value = specular
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    vc = nodes.new('ShaderNodeVertexColor')
    vc.layer_name = 'Col'
    occ = nodes.new('ShaderNodeMix')
    occ.data_type = 'RGBA'
    occ.inputs['Factor'].default_value = diffuse_ao
    occ.inputs['A'].default_value = (1.0, 1.0, 1.0, 1.0)
    links.new(vc.outputs['Alpha'], occ.inputs['B'])
    mul = nodes.new('ShaderNodeMix')
    mul.data_type = 'RGBA'
    mul.blend_type = 'MULTIPLY'
    mul.inputs['Factor'].default_value = 1.0
    mul.inputs['A'].default_value = lt.hex_color(tint)
    links.new(occ.outputs['Result'], mul.inputs['B'])
    links.new(mul.outputs['Result'], bsdf.inputs['Base Color'])
    mat.diffuse_color = lt.hex_color(tint)
    mat['Master'] = 'World'
    mat['Tint'] = '#%06X' % tint
    mat['Kind'] = 'Surface'
    if specular is not None:
        mat['Specular'] = float(specular)
    if roughness_offset is not None:
        mat['RoughnessOffset'] = float(roughness_offset)
    return mat


def _shadow():
    """M_Backdrop: unlit, a flat tint x Brightness x the lighting state's BackdropTint."""
    name = 'SextonShadow'
    mat = bpy.data.materials.get(name)
    if mat is not None:
        return mat
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    em = nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = lt.hex_color(0x141110)
    em.inputs['Strength'].default_value = 1.0
    links.new(em.outputs['Emission'], out.inputs['Surface'])
    mat.diffuse_color = lt.hex_color(0x141110)
    mat['Master'] = 'Backdrop'
    mat['Tint'] = '#141110'
    mat['Brightness'] = 1.0
    mat['Kind'] = 'Surface'
    return mat


MATERIALS = {
    # The wool reflects little (Specular 0.15) and is rough, and its tints are warm neutrals: in the game the
    # near-black wool caught the sky's reflection (4% dielectric specular under a bright blue sky), so the trousers
    # read pale blue-grey and the coat navy.
    'wool': lambda: _flat_world('SextonWool', 0x2A2826, specular=0.15, roughness_offset=0.1),
    'trousers': lambda: _flat_world('SextonTrousers', 0x343230, specular=0.15, roughness_offset=0.1),
    'linen': lambda: bpy.data.materials.get('SextonLinen') or lt.material('Polymer', name='SextonLinen', tint=0xF4EFE2),
    'skin': lambda: bpy.data.materials.get('SextonSkin') or lt.material('Polymer', name='SextonSkin', tint=0xC9B9AA,
                                                                          DiffuseAO=1.0),
    'shadow': _shadow,
    'metal': lambda: bpy.data.materials.get('SextonMetal') or lt.material('MetalWorn', name='SextonMetal', tint=0xA88A52),
    'cover': lambda: bpy.data.materials.get('SextonLedgerCover') or lt.material('Polymer', name='SextonLedgerCover',
                                                                                  tint=0x6E3226),
    'pages': lambda: bpy.data.materials.get('SextonLedgerPages') or lt.material('Polymer', name='SextonLedgerPages',
                                                                                  tint=0xE9DFC6),
}
# Which texture set each slot's UVs are laid out for (box projection at the set's texel density).
UV_SETS = {'SextonWool': 'Polymer', 'SextonTrousers': 'Polymer', 'SextonLinen': 'Polymer', 'SextonSkin': 'Polymer',
           'SextonShadow': 'Polymer', 'SextonMetal': 'MetalWorn', 'SextonLedgerCover': 'Polymer',
           'SextonLedgerPages': 'Polymer'}


# --- The coat's body ---

class Torso:
    """The coat's body: a loft up the spine, superelliptic sections (half-width, front, back, exponent) by keys, the
    seams of a fitted frock coat pressed into it (centre back, side backs, the front edge's overlap). It knows its
    surface, so the skirt, the lapels, the collar and the buttons can be laid on it."""

    def __init__(self, points, table, rings, weight=None, folds=None, segs=20):
        self.path = path_through(points, rings, weight)
        self.frames = rm_frames(self.path, FRONT)
        self.ts, _ = arc_params(self.path)
        self.table = table
        self.folds = folds
        self.segs = segs

    def section(self, t, phi):
        rx, rf, rb, n = keys(self.table, t)
        x, y = superellipse(phi, rx, rf, rb, n)
        extra = 0.0 if self.folds is None else self.folds(t, phi)
        ph = wrap(phi)
        # Seams: the centre back and the two side backs, pressed in.
        extra -= 0.0025 * bump(abs(ph) - math.pi, 0.2) + 0.0015 * bump(abs(ph) - 2.3, 0.22)
        return grow((x, y), extra)

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

    def point(self, t, phi, out=0.0):
        p, tan, nr, side = self.at(t)
        x, y = grow(self.section(t, phi), out)[:2]
        return p + side * x + nr * y

    def t_at_z(self, z):
        for k in range(len(self.path) - 1):
            if self.path[k + 1].z >= z:
                f = (z - self.path[k].z) / max(self.path[k + 1].z - self.path[k].z, 1e-9)
                return lerp(self.ts[k], self.ts[k + 1], min(max(f, 0.0), 1.0))
        return 1.0

    def build(self, part, mat='wool'):
        loft(part, self.path, FRONT, self.section, segs=self.segs, caps=('pole', 'pole'), mat=mat,
             pole_ext=(0.03, 0.01))


def orient_out(faces, center):
    """Turns each face to look away from center (a point, or a function of the face's middle giving one)."""
    for f in faces:
        f.normal_update()
        c = f.calc_center_median()
        ref = center(c) if callable(center) else Vector(center)
        if f.normal.dot(c - ref) < 0.0:
            f.normal_flip()


def coat_collar(part, torso, n0, nd, arc0, segs=16):
    """The frock coat's stand-and-fall collar: a closed profile (the stand, folded over into the fall) swept round
    the neck just outside the wing collar, its foot sunk into the coat's neck opening. It leans halfway with the neck
    (n0 up along nd), rides higher at the back, and thins and drops toward its front ends, where the lapels start."""
    n0, nd = Vector(n0), Vector(nd).normalized()
    top = torso.at(torso.t_at_z(0.768))[0]
    centre = n0 + nd * ((top.z - n0.z) / nd.z)
    up = ((torso.path[-1] - torso.path[-4]).normalized() + nd).normalized()
    side = up.cross(FRONT).normalized()
    front = side.cross(up).normalized()
    profile = [(0.0, -0.012), (0.003, -0.012), (0.003, 0.0155), (0.010, 0.0115), (0.016, 0.0035), (0.0165, 0.008),
               (0.005, 0.023), (-0.001, 0.0195)]
    bm = part.bm
    slot = part.slot('wool')
    rows = []
    for i in range(segs + 1):
        phi = lerp(arc0, 2.0 * math.pi - arc0, i / segs)
        end = min(i, segs - i) / (segs * 0.5)
        # Toward its ends it closes in on the wing collar, so no gap shows between them against the sky.
        x, y = superellipse(phi, 0.064, 0.06, 0.066, 2.0)
        x, y = (v * lerp(0.86, 1.0, smoothstep(0.0, 0.5, end)) for v in (x, y))
        o = (side * x + front * y).normalized()
        scale = lerp(0.06, 1.0, smoothstep(0.0, 0.65, end))
        lift = 0.003 * (1.0 - math.cos(phi)) * 0.5 - 0.014 * (1.0 - scale)
        p = centre + side * x + front * y + up * lift
        rows.append([bm.verts.new(p + o * po * lerp(0.3, 1.0, scale) + up * pu * scale) for po, pu in profile])
    faces = []
    n = len(profile)
    for i in range(segs):
        for k in range(n):
            k1 = (k + 1) % n
            faces.append(bm.faces.new((rows[i][k], rows[i + 1][k], rows[i + 1][k1], rows[i][k1])))
    faces.append(bm.faces.new(list(reversed(rows[0]))))
    faces.append(bm.faces.new(rows[-1]))
    for f in faces:
        f.material_index = slot
    mid = sum((v.co for row in rows for v in row), Vector()) / (len(rows) * n)
    orient_out(faces[-2:], lambda c: mid + (c - mid) * 0.5)
    return faces


def lapels(part, torso):
    """Small rolled lapels from the collar's front ends down to the top button: each a thin raised flap lying on the
    chest, its outer edge standing a few millimetres off the coat."""
    bm = part.bm
    slot = part.slot('wool')
    axis = torso.at(0.9)[0]
    for s in (-1.0, 1.0):
        rows = []
        for i in range(7):
            u = i / 6
            t = lerp(0.985, 0.84, u)
            fold = s * lerp(0.9, 0.1, u ** 1.1)
            width = 0.36 * (1.0 - u) ** 0.9 + 0.02
            rows.append([bm.verts.new(torso.point(t, fold + s * width * v, out))
                         for v, out in ((0.0, 0.0012), (0.4, 0.0045), (0.8, 0.0062), (1.0, 0.0058), (1.0, 0.0008))])
        faces = [bm.faces.new((rows[i][k], rows[i][k + 1], rows[i + 1][k + 1], rows[i + 1][k]))
                 for i in range(6) for k in range(4)]
        orient_out(faces, lambda c: Vector((axis.x, axis.y, c.z)))
        for f in faces:
            f.material_index = slot


def torso_band(part, torso, t0, t1, out, mat='wool', segs=20):
    """The waist seam: a narrow band standing out of the coat where its skirt is sewn on."""
    path = [torso.at(lerp(t0, t1, k / 3))[0] for k in range(4)]

    def section(t, phi):
        return grow(torso.section(lerp(t0, t1, t), phi), out * (0.55 + 0.45 * math.sin(math.pi * t)))
    loft(part, path, FRONT, section, segs=segs, mat=mat)


# --- Sleeves, cuffs, trousers, boots ---

def sleeve(part, joint, table, seed, folds_amp=0.011):
    """A sleeve from inside the shoulder to 3 cm short of the wrist: rings crowd the elbow and the cuff for their
    folds (compression ridges inside the elbow, a pull across its outside, a stack above the cuff), the cuff flares a
    little, and its edge turns in (the cloth's thickness)."""
    start, S, E, W = (Vector(p) for p in joint)
    d = (W - E).normalized()
    end = W - d * 0.03
    pts = [start, S, E, end]
    lengths = [(b - a).length for a, b in zip(pts, pts[1:])]
    te = (lengths[0] + lengths[1]) / sum(lengths)
    path = path_through(pts, 19, dense_near((te, 0.13, 2.6), (0.97, 0.12, 1.8)))
    folds = wrinkles(seed, [(te, 0.075, 0.0, 1.35, folds_amp, 4), (te * 0.45, 0.14, 2.2, 0.9, folds_amp * 0.5, 3),
                            (lerp(te, 1.0, 0.72), 0.1, 3.1, 1.2, folds_amp * 0.35, 2), (0.93, 0.05, 0.0, 3.2, 0.0035, 2)])

    def section(t, phi):
        rx, rf, rb, n = keys(table, t)
        xy = superellipse(phi, rx, rf, rb, n)
        return grow(xy, folds(t, phi) + 0.004 * smoothstep(0.9, 1.0, t))
    loft(part, path, FRONT, section, segs=12, caps=('pole', None), mat='wool', pole_ext=(0.03, 0.0),
         lip=(0.005, 0.014))
    return W, d


def cuff(part, W, d, radius):
    """The shirt cuff showing past the sleeve: a short linen tube, its edge turned in."""
    path = [W - d * 0.05, W - d * 0.03, W - d * 0.006]
    loft(part, path, UP, lambda t, phi: (radius * math.sin(phi), radius * math.cos(phi)), segs=10, mat='linen',
         lip=(0.003, 0.008))


def trouser_leg(part, hip, knee, ankle, table, seed, rings=23):
    """A trouser leg from inside the hip to the ankle: rings crowd the knee (folds behind it), the groin (the seated
    crease) and the hem; at the hem the cloth breaks over the boot: a fold across the front, a slight flare, and
    the front edge dropping onto the instep."""
    hip, knee, ankle = Vector(hip), Vector(knee), Vector(ankle)
    pts = [hip - (knee - hip).normalized() * 0.06, hip, knee, ankle + (ankle - knee).normalized() * 0.012]
    lengths = [(b - a).length for a, b in zip(pts, pts[1:])]
    tk = (lengths[0] + lengths[1]) / sum(lengths)
    path = path_through(pts, rings, dense_near((tk, 0.12, 2.2), (0.08, 0.07, 1.0), (0.95, 0.1, 2.2)))
    folds = wrinkles(seed, [(tk, 0.075, math.pi, 1.3, 0.008, 4), (0.09, 0.06, 0.0, 1.0, 0.005, 3),
                            (tk * 0.55, 0.12, 0.0, 0.9, 0.0025, 2), (0.925, 0.035, 0.0, 1.35, 0.007, 1)])

    def section(t, phi):
        rx, rf, rb, n = keys(table, t)
        x, y = superellipse(phi, rx, rf, rb, n)
        extra = folds(t, phi) + 0.006 * smoothstep(0.9, 1.0, t)
        xy = grow((x, y), extra)
        # The hem's front drops onto the instep.
        drop = 0.018 * bump(wrap(phi), 1.3) * smoothstep(0.95, 1.0, t)
        return xy[0], xy[1], drop
    loft(part, path, UP, section, segs=12, caps=('pole', None), mat='trousers', pole_ext=(0.03, 0.0),
         lip=(0.005, 0.016))


def boot(part, ankle, fdir, shin_up, length=0.3, width=0.09, height=0.083, toe=0.85, sole_rings=9, heel_segs=8):
    """A pointed elastic-sided boot: its shaft up inside the trouser hem, the foot lofted heel to toe with a flat
    sole side, a proud sole and a heel block."""
    f = nrm(fdir)
    up = Vector(shin_up)
    up = (up - f * up.dot(f)).normalized()
    a = Vector(ankle)
    sh = nrm(shin_up)
    loft(part, [a + sh * 0.075, a + sh * 0.035, a + sh * 0.005, a - up * height * 0.25 + f * 0.01], f,
         lambda t, phi: superellipse(phi, keys([(0, 0.046), (0.6, 0.045), (1.0, width * 0.46)], t),
                                     keys([(0, 0.046), (1.0, height * 0.55)], t), None, 2.0),
         segs=10, mat='wool')
    heel_pt = a - up * height * 0.62 - f * length * 0.12
    toe_pt = a - up * height * 0.62 + f * length * 0.88
    path = path_through([heel_pt, heel_pt.lerp(toe_pt, 0.35) + up * height * 0.06, toe_pt], 11,
                        dense_near((0.0, 0.15, 1.0), (1.0, 0.2, 1.4)))

    def section(t, phi):
        rx = keys([(0.0, width * 0.28), (0.08, width * 0.40), (0.3, width * 0.44), (0.62, width * 0.5),
                   (0.85, width * lerp(0.42, 0.3, toe)), (1.0, width * lerp(0.2, 0.08, toe))], t)
        rtop = keys([(0.0, height * 0.45), (0.15, height * 0.62), (0.35, height * 0.6), (0.62, height * 0.36),
                     (0.85, height * 0.26), (1.0, height * 0.16)], t)
        rbot = keys([(0.0, height * 0.32), (0.2, height * 0.38), (0.7, height * 0.3), (1.0, height * 0.12)], t)
        return superellipse(phi, rx, rtop, rbot, 2.6 if math.cos(phi) < 0 else 2.0)
    loft(part, path, up, section, segs=10, caps=('pole', 'pole'), mat='wool', pole_ext=(0.012, 0.01))
    sole = [p - up * height * 0.33 for p in resample(path, sole_rings)]

    def sole_sec(t, phi):
        rx = keys([(0.0, width * 0.32), (0.08, width * 0.44), (0.3, width * 0.47), (0.62, width * 0.53),
                   (0.85, width * lerp(0.45, 0.33, toe)), (1.0, width * lerp(0.22, 0.1, toe))], t)
        return superellipse(phi, rx, 0.007, 0.007, 3.0)
    loft(part, sole, up, sole_sec, segs=8, caps=('pole', 'pole'), mat='wool', pole_ext=(0.01, 0.008))
    hp = heel_pt - up * height * 0.33 - up * 0.014 + f * length * 0.07
    loft(part, [hp - f * 0.035, hp + f * 0.005, hp + f * 0.045], up,
         lambda t, phi: superellipse(phi, width * 0.36, 0.014, 0.014, 3.0), segs=heel_segs, caps=('pole', 'pole'),
         mat='wool', pole_ext=(0.005, 0.005))


# --- Hands ---

FINGERS = ('index', 'middle', 'ring', 'little')
FINGER_LEN = {'index': (0.044, 0.027, 0.023), 'middle': (0.048, 0.031, 0.025), 'ring': (0.046, 0.030, 0.024),
              'little': (0.035, 0.022, 0.021)}
FINGER_AT = {'index': (0.031, 0.0), 'middle': (0.010, 0.004), 'ring': (-0.011, -0.001), 'little': (-0.029, -0.011)}
FINGER_R = {'index': 0.0092, 'middle': 0.0095, 'ring': 0.0088, 'little': 0.0078}
# The thumb's bones: the metacarpal (most of it inside the ball of the thumb) and the two phalanges.
THUMB_LEN = (0.046, 0.032, 0.026)
POSES = {
    # Spread on a page: the palm resting on it, the fingers long and nearly straight, each bent at its knuckle until
    # its tip rests on the page; the thumb lying beside them.
    'flat': dict(curl={'index': (4, 10, 6), 'middle': (4, 12, 7), 'ring': (6, 14, 8), 'little': (8, 16, 10)},
                 spread={'index': 6, 'middle': 0, 'ring': -6, 'little': -14}, thumb=(30, 10, 8, 10),
                 touch=('index', 'middle', 'ring', 'little', 'thumb')),
    # Writing with a dip pen (a tripod grip): the pen lies along the index, its pad on top, the thumb's pad on its
    # side and the middle finger under it; it rests in the web of the thumb. The ring and little fingers curl under
    # (held off the page: tightening them until they touched it curled them through the palm). The pen's contact
    # point and the web are given from the index's knuckle (ahead, toward the back of the hand, toward the thumb),
    # and how high the contact point is over the page.
    'pen': dict(curl={'index': (24, 40, 22), 'middle': (40, 58, 34), 'ring': (52, 66, 40), 'little': (56, 70, 44)},
                spread={'index': 2, 'middle': 1, 'ring': -4, 'little': -9}, thumb=(34, 30, 26, 32),
                touch=(), pen=dict(contact=(0.026, -0.055, 0.008), web=(-0.012, 0.004, 0.018), height=0.024)),
}


def settle(gap, touch):
    """The extra bend (degrees) at a digit's first joint, given gap(bend), the digit's clearance over the page: down
    until it just rests on the page if it touches it, otherwise up only as far as it needs to clear it."""
    if touch:
        lo, hi = -50.0, 70.0
        if gap(lo) <= 0.0:
            return lo
        if gap(hi) >= 0.0:
            return hi
    else:
        if gap(0.0) >= 0.0:
            return 0.0
        lo, hi = -90.0, 0.0
    for _ in range(28):
        mid = 0.5 * (lo + hi)
        if gap(mid) > 0.0:
            lo = mid
        else:
            hi = mid
    return lo


def aim(build, target, ranges):
    """The parameters (bend, curl, spread; each within its range) that bring build's tip nearest target, a little
    in favour of the pose as drawn (0, 1, 0): a coarse grid, then finer ones round the best. Returns them and the
    distance left."""
    centre = [(lo + hi) * 0.5 for lo, hi in ranges]
    steps = [(hi - lo) / 8.0 for lo, hi in ranges]
    best = None
    for _ in range(5):
        axes = [sorted({min(max(c + st * i, lo), hi) for i in range(-4, 5)}) for c, st, (lo, hi) in
                zip(centre, steps, ranges)]
        for a in axes[0]:
            for b in axes[1]:
                for c in axes[2]:
                    d = (build(a, b, c)[3] - target).length
                    cost = d + 0.0004 * (abs(a) / 20.0 + abs(b - 1.0) + abs(c) / 20.0)
                    if best is None or cost < best[0]:
                        best = (cost, (a, b, c), d)
        centre = list(best[1])
        steps = [st * 0.4 for st in steps]
    return best[1], best[2]


def hand(part, wrist, fwd, back, thumb_side, pose, rest=None, s=1.04, fingers=1.0, bulk=0.8):
    """A long pale hand: a palm lofted from the wrist to the knuckles, four three-jointed fingers and a thumb, each a
    tapered tube with knuckles, a flatter nail side and a rounded tip.
    rest is the page under it (a point on it and its normal). The hand moves along the normal until it lies on the
    page: its palm and the digits that don't rest on the page just clear it (a pen hand instead holds its pen's
    contact point at the pose's height); then the resting digits bend at their first joint until they touch it. A
    pen pose aims the index, the thumb and the middle finger at the pen: each one's bend, curl and spread are searched
    until its tip reaches its place on the pen. With part None nothing is built.
    Returns the tips, the wrist where the hand ended, and the pen (its nib on the page and its top) or None."""
    pose = POSES[pose]
    fwd = nrm(fwd)
    back0 = Vector(back)
    back0 = (back0 - fwd * back0.dot(fwd)).normalized()
    across = Vector(thumb_side)
    across = (across - fwd * across.dot(fwd) - back0 * across.dot(back0)).normalized()
    palm_len, half_w = 0.098 * s, 0.042 * s * bulk
    sign = 1.0 if fwd.cross(back0).dot(across) > 0.0 else -1.0
    thumbs = s * (1.0 + (fingers - 1.0) * 0.4)
    r_thumb = 0.0108 * s * bulk

    def palm_radii(t):
        return (keys([(0.0, 0.025 * s), (0.4, 0.035 * s * bulk), (0.85, half_w), (1.0, half_w * 0.96)], t),
                keys([(0.0, 0.017 * s), (0.5, 0.0125 * s), (1.0, 0.0115 * s)], t),
                keys([(0.0, 0.018 * s), (0.45, 0.02 * s * bulk), (1.0, 0.0145 * s)], t))

    def finger(w, name, extra=0.0, kappa=1.0, sigma=0.0):
        """A finger's loft points, the points and radii to keep off the page, and its tip."""
        off, ahead = FINGER_AT[name]
        base = w + fwd * palm_len + across * off * s * bulk + fwd * (ahead * s - 0.010 * s) + back0 * 0.001 * s
        sp = math.radians(pose['spread'][name] + sigma)
        d = (fwd * math.cos(sp) + across * math.sin(sp)).normalized()
        fb = (back0 - d * back0.dot(d)).normalized()
        p = base.copy()
        pts = [base - d * 0.008 * s]
        for k, seg in enumerate(FINGER_LEN[name]):
            a = math.radians(pose['curl'][name][k] * kappa + (extra if k == 0 else 0.0))
            d, fb = (d * math.cos(a) - fb * math.sin(a)).normalized(), (fb * math.cos(a) + d * math.sin(a)).normalized()
            seg *= s * fingers
            pts += [p + d * seg * 0.5, p + d * seg]
            p = p + d * seg
        r0 = FINGER_R[name] * s * bulk
        return pts, pts[1:] + [p + d * r0 * 0.6], [r0] * (len(pts) - 2) + [r0 * 0.8, r0 * 0.25], p

    def thumb(w, extra=0.0, kappa=1.0, sigma=0.0):
        """The thumb's loft points, the points and radii to keep off the page (its metacarpal is inside the ball of
        the thumb, so only the phalanges count), and its tip."""
        ab, f1, f2, lift = pose['thumb']
        base = w + fwd * 0.026 * s + across * 0.017 * s * bulk - back0 * 0.009 * s
        d = (fwd * math.cos(math.radians(ab + sigma)) + across * math.sin(math.radians(ab + sigma))).normalized()
        a = math.radians(lift + extra)
        d = (d * math.cos(a) - back0 * math.sin(a)).normalized()
        tb = back0 * 0.6 + across * 0.8
        tb = (tb - d * tb.dot(d)).normalized()
        pts = [base]
        p = base.copy()
        for seg, ang in zip(THUMB_LEN, (0.0, f1 * kappa, f2 * kappa)):
            a = math.radians(ang)
            d, tb = (d * math.cos(a) - tb * math.sin(a)).normalized(), (tb * math.cos(a) + d * math.sin(a)).normalized()
            seg *= thumbs
            pts += [p + d * seg * 0.5, p + d * seg]
            p = p + d * seg
        return pts, pts[3:] + [p + d * r_thumb * 0.55], [r_thumb] * (len(pts) - 4) + [r_thumb * 0.8, r_thumb * 0.25], p

    def palm_points(w):
        """Points round the palm's underside and its edges."""
        out = []
        for k in range(7):
            t = k / 6
            c = w + fwd * lerp(-0.016 * s, palm_len, t)
            rx, rb, rp = palm_radii(t)
            out += [c - back0 * rp, c + across * rx * 0.85 - back0 * rp * 0.5,
                    c - across * rx * 0.85 - back0 * rp * 0.5, c + across * rx, c - across * rx]
        return out

    def height(points, radii=None):
        """The least height of points (less their radii) over the page."""
        p0, n = Vector(rest[0]), Vector(rest[1]).normalized()
        radii = radii or [0.0] * len(points)
        return min((q - p0).dot(n) - r for q, r in zip(points, radii))

    def pen_line(w):
        """The pen's contact point (at the index's tip) and its axis, up through the web of the thumb."""
        ibase = w + fwd * palm_len + across * FINGER_AT['index'][0] * s * bulk
        cf, cb, ca = pose['pen']['contact']
        wf, wb, wa = pose['pen']['web']
        c = ibase + fwd * cf + back0 * cb + across * ca
        return c, (ibase + fwd * wf + back0 * wb + across * wa - c).normalized()

    w = Vector(wrist)
    touch = pose['touch']
    params = {}
    if 'pen' in pose:
        c, a = pen_line(w)
        targets = {'index': c + back0 * 0.0105, 'thumb': c + across * 0.011 + a * 0.006,
                   'middle': c - back0 * 0.0095 - across * 0.004 - a * 0.004}
        for name, target in targets.items():
            if name == 'thumb':
                # The thumb's metacarpal stays on the palm's side (it must not swing out of the back of the hand).
                params[name], miss = aim(lambda e, k, g: thumb(w, e, k, g), target,
                                         ((-15.0, 30.0), (0.3, 1.6), (-20.0, 15.0)))
            else:
                params[name], miss = aim(lambda e, k, g, name=name: finger(w, name, e, k, g), target,
                                         ((-30.0, 50.0), (0.3, 1.7), (-12.0, 12.0)))
            if part is not None:
                log(f'  pen grip: {name} {miss * 1000:.1f} mm off its place on the pen')
    if rest is not None:
        n = Vector(rest[1]).normalized()
        clear = height(palm_points(w))
        for name in FINGERS:
            if name not in touch:
                clear = min(clear, height(*finger(w, name, *params.get(name, (0.0, 1.0, 0.0)))[1:3]))
        if 'thumb' not in touch:
            clear = min(clear, height(*thumb(w, *params.get('thumb', (0.0, 1.0, 0.0)))[1:3]))
        if 'pen' in pose:
            # The pen's contact point at its height, unless that would put something through the page.
            w = w - n * min((pen_line(w)[0] - Vector(rest[0])).dot(n) - pose['pen']['height'], clear)
        else:
            w = w - n * clear
        for name in touch:
            if name == 'thumb':
                params[name] = (settle(lambda e: height(*thumb(w, e)[1:3]), True), 1.0, 0.0)
            else:
                params[name] = (settle(lambda e, name=name: height(*finger(w, name, e)[1:3]), True), 1.0, 0.0)
    tips = {}
    for name in FINGERS:
        pts, _, _, tips[name] = finger(w, name, *params.get(name, (0.0, 1.0, 0.0)))
        if part is not None:
            r0 = FINGER_R[name] * s * bulk
            loft(part, path_through(pts, 9, dense_near((0.37, 0.1, 1.0), (0.67, 0.1, 1.0), (1.0, 0.12, 0.8))), back0,
                 finger_section(r0), segs=8, caps=('pole', 'pole'), mat='skin', pole_ext=(0.004 * s, r0 * 0.6))
    pts, _, _, tips['thumb'] = thumb(w, *params.get('thumb', (0.0, 1.0, 0.0)))
    pen = None
    if 'pen' in pose and rest is not None:
        c, a = pen_line(w)
        p0, n = Vector(rest[0]), Vector(rest[1]).normalized()
        reach = ((c - p0).dot(n) - 0.0005) / max(a.dot(n), 0.2)
        pen = (c - a * reach, c + a * 0.12)
    if part is None:
        return tips, w, pen

    def thumb_sec(t, phi):
        r = r_thumb * keys([(0.0, 1.2), (0.35, 1.0), (0.65, 0.95), (1.0, 0.8)], t)
        return superellipse(phi, r, r * 0.88, r * 0.95, 2.2)
    loft(part, path_through(pts, 9), back0, thumb_sec, segs=8, caps=('pole', 'pole'), mat='skin',
         pole_ext=(0.004 * s, r_thumb * 0.55))

    def palm(t, phi):
        rx, rb, rp = palm_radii(t)
        x, y = superellipse(phi, rx, rb, rp, 2.4)
        if y < 0.0 and x * sign > 0.0:      # the thumb's mound
            y -= 0.006 * s * (x * sign / rx) * (1.0 - smoothstep(0.25, 0.8, t)) * smoothstep(0.0, 0.25, t)
        if y > 0.0:                          # the back's tendons, and the knuckles' ridge at its end
            y += 0.002 * s * abs(math.sin(x / max(rx, 1e-6) * 4.0)) ** 1.5 * smoothstep(0.25, 0.85, t)
            y += 0.0016 * s * smoothstep(0.82, 0.97, t) * (1.0 - smoothstep(0.75, 1.0, abs(x) / max(rx, 1e-6)))
        return x, y
    path = path_through([w - fwd * 0.016 * s, w + fwd * palm_len * 0.45, w + fwd * palm_len], 7)
    loft(part, path, back0, palm, segs=12, caps=('pole', 'pole'), mat='skin', pole_ext=(0.008 * s, 0.005 * s))
    return tips, w, pen


def finger_section(r0):
    """A finger's section along it (t): the knuckles, the joints' creases, the tip; the nail side flattens toward
    the tip."""
    def sec(t, phi):
        r = r0 * keys([(0.0, 1.0), (0.3, 1.04), (0.37, 0.93), (0.6, 0.93), (0.67, 0.88), (0.77, 0.83), (1.0, 0.78)], t)
        # The knuckle where the finger leaves the hand, standing on its back.
        r *= 1.0 + 0.16 * bump(t - 0.07, 0.08) * smoothstep(0.1, 0.6, math.cos(phi))
        top = math.cos(phi) > 0
        return superellipse(phi, r, r * lerp(0.95, 0.82, smoothstep(0.65, 1.0, t)) if top else r * 0.98,
                            r * 0.98, lerp(2.2, 2.7, smoothstep(0.65, 1.0, t)) if top else 2.1)
    return sec


# --- Head, neckwear, hat ---

def spline(table, x):
    """A smooth curve through table's points (x, value or tuple of values): cubic pieces whose slope at each point
    comes from its neighbours (level at the ends), held level beyond the ends."""
    if x <= table[0][0]:
        return table[0][1]
    if x >= table[-1][0]:
        return table[-1][1]
    i = 0
    while table[i + 1][0] < x:
        i += 1
    xa, xb = table[i][0], table[i + 1][0]
    h = xb - xa
    u = (x - xa) / h
    h00, h10, h01, h11 = 2 * u ** 3 - 3 * u ** 2 + 1, u ** 3 - 2 * u ** 2 + u, 3 * u ** 2 - 2 * u ** 3, u ** 3 - u ** 2
    single = not isinstance(table[0][1], (tuple, list))

    def val(k, c):
        return table[k][1] if single else table[k][1][c]

    def slope(k, c):
        if k <= 0 or k >= len(table) - 1:
            return 0.0
        return (val(k + 1, c) - val(k - 1, c)) / (table[k + 1][0] - table[k - 1][0])
    out = [h00 * val(i, c) + h10 * h * slope(i, c) + h01 * val(i + 1, c) + h11 * h * slope(i + 1, c)
           for c in range(1 if single else len(table[0][1]))]
    return out[0] if single else tuple(out)


# The face's shadow line, as head-local heights by the angle round the head (0 in front, pi behind). It follows the
# face the way the brim's and the nose's shadow would: across his upper lip, then down round the corners of his mouth
# and the sides of his chin, so his cheekbones, the hollows under them and his jaw are all in shadow, and the back of
# his head and neck. The skin fades to black over FADE below it: the lips sink into the shadow and the light catches
# only the point of his chin.
SHADOW_LINE = [(0.0, -0.0505), (0.15, -0.054), (0.3, -0.068), (0.45, -0.08), (0.6, -0.086), (0.9, -0.088),
               (1.2, -0.085), (1.5, -0.081), (1.7, -0.09), (1.9, -0.14), (2.2, -0.2), (math.pi, -0.2)]
FADE = 0.02
# The jaw's lower edge (the bottom of the chin in front, rising to the jaw's angle): under it the throat is in the
# chin's and the jaw's own shadow (the Adam's apple shows by its outline).
JAW_EDGE = [(0.0, -0.096), (0.8, -0.092), (1.5, -0.086), (2.0, -0.09)]
HEAD_CENTER, HEAD_PITCH = V(0.0, -0.065, 0.905), 14.0
# Where his neck leaves the coat (it leans far forward to the head, which sits 0.128 below its centre on its axis).
NECK_BASE = V(0.0, -0.012, 0.735)


def shadow_line(ph):
    return spline(SHADOW_LINE, min(abs(ph), math.pi))


def head_frame():
    rot = Matrix.Rotation(math.radians(-HEAD_PITCH), 3, 'X')
    return Matrix.Translation(HEAD_CENTER) @ rot.to_4x4()


# The head in its own frame (head_frame: z up its axis, -y forward), by height: the half-width, the face's depth in
# front (its plane, without the nose, the lips or the chin), the depth behind, and how square the front is.
HEAD_TABLE = [(-0.128, (0.0455, 0.050, 0.055, 2.0)), (-0.112, (0.0455, 0.050, 0.057, 2.0)),
              (-0.105, (0.045, 0.053, 0.060, 2.0)), (-0.099, (0.046, 0.059, 0.063, 1.8)),
              (-0.092, (0.048, 0.069, 0.066, 1.7)), (-0.084, (0.049, 0.073, 0.069, 1.7)),
              (-0.072, (0.051, 0.077, 0.073, 1.8)), (-0.056, (0.054, 0.081, 0.079, 1.9)),
              (-0.040, (0.060, 0.084, 0.085, 2.1)), (-0.020, (0.064, 0.086, 0.090, 2.4)),
              (0.000, (0.068, 0.087, 0.094, 2.6)), (0.020, (0.070, 0.089, 0.098, 2.5)),
              (0.036, (0.071, 0.089, 0.100, 2.3)), (0.052, (0.071, 0.086, 0.101, 2.1)),
              (0.068, (0.069, 0.081, 0.099, 2.0))]
# The nose down the middle of the face, by height: how far it stands out of the face's plane, and how far round
# (radians) it reaches.
NOSE = [(-0.037, (0.0, 0.2)), (-0.034, (0.009, 0.19)), (-0.031, (0.019, 0.16)), (-0.027, (0.0245, 0.13)),
        (-0.023, (0.026, 0.12)), (-0.016, (0.0235, 0.11)), (-0.006, (0.018, 0.1)), (0.006, (0.011, 0.1)),
        (0.016, (0.005, 0.11)), (0.024, (0.0, 0.12))]
# The face's forms: (height, height reach, angle, angle reach, amount), mirrored when the angle isn't 0.
FACE = [(-0.124, 0.012, 0.0, 0.24, 0.009),       # the Adam's apple
        (-0.081, 0.016, 0.0, 0.28, 0.008),       # the chin, narrow, coming to a point
        (-0.065, 0.005, 0.0, 0.45, -0.004),      # the crease under the lower lip
        (-0.0565, 0.0035, 0.0, 0.42, 0.0035),    # the lower lip, thin
        (-0.0515, 0.0018, 0.0, 0.5, -0.0028),    # the line of the mouth
        (-0.048, 0.003, 0.0, 0.42, 0.003),       # the upper lip
        (-0.041, 0.007, 0.0, 0.18, 0.0015),      # the philtrum
        (-0.0515, 0.003, 0.42, 0.1, -0.002),     # the corners of the mouth
        (-0.031, 0.005, 0.21, 0.09, 0.005),      # the wings of the nostrils
        (0.024, 0.008, 0.0, 0.95, 0.005),        # the brow ridge
        (0.022, 0.007, 0.62, 0.25, 0.004),       # its outer ends over the eyes
        (0.008, 0.011, 0.44, 0.22, -0.012),      # the eye sockets, deep
        (-0.004, 0.011, 0.95, 0.3, 0.008),       # the cheekbones
        (0.0, 0.008, 1.3, 0.25, 0.004),          # their arches back to the ears
        (-0.044, 0.017, 1.05, 0.36, -0.013),     # the hollow cheeks
        (-0.04, 0.016, 1.45, 0.22, -0.006),      # the hollows seen from in front
        (0.032, 0.012, 1.3, 0.25, -0.007),       # the temples
        (-0.090, 0.008, 0.75, 0.3, 0.004),       # the jawline, running back
        (-0.087, 0.008, 1.2, 0.3, 0.004),
        (-0.101, 0.006, 1.0, 0.45, -0.003),      # the hollow under the jaw
        (0.018, 0.008, 0.8, 0.2, 0.004),         # the brow's outer end, seen three-quarter
        (0.004, 0.009, 0.68, 0.16, -0.007),      # the eye socket's outer rim
        (-0.010, 0.010, 0.86, 0.2, 0.006),       # the cheekbone's corner
        (-0.045, 0.014, 0.82, 0.22, -0.008),     # the hollow under it
        (-0.080, 0.010, 0.82, 0.22, 0.003),      # the jaw below
        (-0.083, 0.012, 1.58, 0.26, 0.006)]      # the angle of the jaw, under the ear
# Ring heights: crowded at the lips, the nose and the chin, where the profile turns.
HEAD_RINGS = [-0.128, -0.118, -0.108, -0.1, -0.094, -0.089, -0.083, -0.077, -0.071, -0.066,
              -0.061, -0.0565, -0.0535, -0.0515, -0.0495, -0.0465, -0.043, -0.039, -0.034, -0.030, -0.026, -0.021,
              -0.014, -0.006, 0.003, 0.012, 0.022, 0.032, 0.046, 0.060, 0.070]


def head_xy(z, phi):
    """The head's surface at head-local height z and angle phi round it (0 in front): (x to his left, forward)."""
    rx, rf, rb, n = spline(HEAD_TABLE, z)
    x, y = superellipse(phi, rx, rf, rb, n if math.cos(phi) > 0.0 else 2.0)
    ph = wrap(phi)
    off = 0.0
    for zc, zw, pc, pw, amp in FACE:
        if abs(z - zc) < zw:
            off += amp * bump(z - zc, zw) * (bump(ph - pc, pw) + (bump(ph + pc, pw) if pc else 0.0))
    p, w = spline(NOSE, z)
    if p > 0.0:
        off += p * bump(ph, w)
    return grow((x, y), off)


def in_shadow(local):
    """How far a head-local point lies inside the face's shadow (positive inside): above the shadow line, or under
    the jaw's edge, where the throat is in the chin's and the jaw's own shadow."""
    ph = abs(math.atan2(local.x, -local.y))
    return max(local.z - shadow_line(ph), spline(JAW_EDGE, ph) - 0.004 - local.z)


def cut_shadow(part, faces, hf):
    """Cuts the head's faces along the edge of the face's shadow (in_shadow's zero line: the shadow line above the
    lit skin and the jaw's edge below it, one closed curve) and gives the faces inside the shadow the shadow, the
    others the skin, so the shadow ends in a clean curve. A vertex lying near where the curve crosses one of its
    edges (within a quarter of the edge) moves onto the nearest such crossing instead of the edge splitting; the
    other crossed edges split, and the faces the cut leaves with more than four corners are triangulated (no slivers,
    no faces without area: Unreal's tangents need neither).
    It runs over lists in the faces' order, never over sets of BMesh elements: a set's order follows memory addresses,
    which differ from one Blender process to the next, and the export must come out the same every time. Which
    vertices move is decided from the whole cut before anything moves, so it doesn't depend on the order either."""
    inv = hf.inverted()
    bm = part.bm
    faces = list(faces)
    val = {v: in_shadow(inv @ v.co) for v in dict.fromkeys(v for fc in faces for v in fc.verts)}
    crossings = []
    for e in dict.fromkeys(e for fc in faces for e in fc.edges):
        a, b = e.verts
        if (val[a] > 0.0) != (val[b] > 0.0):
            crossings.append((e, val[a] / (val[a] - val[b])))
    # Each vertex near a crossing goes to the nearest one, measured along its edge.
    moves = {}
    for e, t in crossings:
        a, b = e.verts
        length = e.calc_length()
        for v, share, other in ((a, t, b), (b, 1.0 - t, a)):
            if share < 0.25 and (v not in moves or share * length < moves[v][0]):
                moves[v] = (share * length, v.co.lerp(other.co, share))
    for v, (_, co) in moves.items():
        v.co = co
    on = set(moves)
    for e, t in crossings:
        a, b = e.verts
        if a not in on and b not in on:
            on.add(bmesh.utils.edge_split(e, a, t)[1])
    for fc in list(faces):
        vs = list(fc.verts)
        idx = [k for k, v in enumerate(vs) if v in on]
        if len(idx) == 2 and (idx[1] - idx[0]) % len(vs) not in (1, len(vs) - 1):
            faces.append(bmesh.utils.face_split(fc, vs[idx[0]], vs[idx[1]])[0])
    # A face the cut left with more than four corners has one lying on a straight edge (where the curve passed through
    # a corner of the face beside it): fanned from that corner, none of its triangles is a sliver.
    for fc in [fc for fc in faces if len(fc.verts) > 4]:
        vs = list(fc.verts)
        n = len(vs)

        def opening(k):
            u, w = vs[k - 1].co - vs[k].co, vs[(k + 1) % n].co - vs[k].co
            return u.angle(w, 0.0) if u.length > 1e-9 and w.length > 1e-9 else 0.0
        k = max(range(n), key=opening)
        for j in range(1, n - 1):
            tri = bm.faces.new((vs[k], vs[(k + j) % n], vs[(k + j + 1) % n]))
            tri.smooth = fc.smooth
            faces.append(tri)
        faces.remove(fc)
        bm.faces.remove(fc)
    skin, shadow = part.slot('skin'), part.slot('shadow')
    for fc in faces:
        fc.material_index = shadow if in_shadow(inv @ fc.calc_center_median()) > 0.0 else skin


def head(part):
    """The head, ring by ring up its axis (tilted back a little) from the neck to inside the hat: a gaunt man's skull
    and face (HEAD_TABLE, NOSE, FACE): a high forehead under the brim, a brow ridge over deep eye sockets, a long
    nose, hard cheekbones over hollow cheeks, thin closed lips, a narrow pointed chin, the jawline running back to its
    angle under the ear, an Adam's apple on the throat; below it the neck bends down into the coat. Cut along the
    edges of the face's shadow (cut_shadow): the shadow inside them, the skin (the lips and the chin) outside. The
    ears, in shadow. Returns the head's frame."""
    hf = head_frame()
    rot = hf.to_3x3()
    bm = part.bm
    segs, warp = 26, 0.55
    angles = [2.0 * math.pi * j / segs - warp * math.sin(2.0 * math.pi * j / segs) for j in range(segs)]
    rings = [[bm.verts.new(hf @ Vector((x, -y, z))) for x, y in (head_xy(z, phi) for phi in angles)]
             for z in HEAD_RINGS]
    # The neck below: rings down a curve from the head's lowest ring into the coat, turning from the head's axis to
    # the neck's lean and rounding to a plain neck.
    hup = rot @ UP
    n1 = hf @ Vector((0.0, 0.0, HEAD_RINGS[0]))
    nd = (n1 - NECK_BASE).normalized()
    curve = [n1, n1 - hup * 0.025, NECK_BASE + nd * 0.035, NECK_BASE - nd * 0.01]
    neck = []
    for s in (0.3, 0.6, 1.0):
        c = sum((w * q for w, q in zip(((1 - s) ** 3, 3 * s * (1 - s) ** 2, 3 * s * s * (1 - s), s ** 3), curve)),
                Vector())
        d = sum((w * (b - a) for w, a, b in zip((3 * (1 - s) ** 2, 6 * s * (1 - s), 3 * s * s), curve, curve[1:])),
                Vector()).normalized()
        turn = hup.rotation_difference(-d).to_matrix()
        k = smoothstep(0.0, 1.0, s)
        neck.append([bm.verts.new(c + turn @ (rot @ Vector((lerp(x, 0.043 * math.sin(phi), k),
                                                            -lerp(y, 0.043 * math.cos(phi), k), 0.0))))
                     for phi, (x, y) in ((phi, head_xy(HEAD_RINGS[0], phi)) for phi in angles)])
    rings = neck[::-1] + rings
    faces = [bm.faces.new((a[j], a[(j + 1) % segs], b[(j + 1) % segs], b[j]))
             for a, b in zip(rings, rings[1:]) for j in range(segs)]
    top = rings[-1]
    pole = bm.verts.new(sum((v.co for v in top), Vector()) / segs + (rot @ UP) * 0.006)
    faces += [bm.faces.new((top[j], top[(j + 1) % segs], pole)) for j in range(segs)]
    cut_shadow(part, faces, hf)
    # The ears, flat shells flaring from the head at the back.
    for s in (-1.0, 1.0):
        p = hf @ Vector((s * 0.067, 0.012, -0.004))
        er = rot @ Matrix.Rotation(math.radians(-15.0), 3, 'X') @ Matrix.Rotation(math.radians(s * 24.0), 3, 'Z')
        ellipsoid(part, p, (0.0065, 0.018, 0.029), er, segs=8, rings=5, mat='shadow')
    return hf


def wing_collar(part, base, axis, front, r=0.05, depth=1.08, height=0.035, thick=0.0022, gap=0.3, wing=0.5):
    """The wing collar: a band of starched linen standing round the neck (from base up along axis), its top edge
    rolled, open in front. At each front edge its top corner folds out and down over the band (a wing): the fold
    runs from half the band's height at the edge up to near its top a little way round, bending round a small
    radius, so the wing has the cloth's thickness on both faces."""
    up = Vector(axis).normalized()
    fr = (Vector(front) - up * Vector(front).dot(up)).normalized()
    side = up.cross(fr).normalized()
    base = Vector(base)
    top = height - 0.0008
    bend_r = thick * 0.5 + 0.0004
    reach = [0.0, 0.08, 0.17, 0.28, 0.4, 0.52, 0.85, 1.45, 2.15, math.pi - gap]
    phis = [gap + e for e in reach] + [2.0 * math.pi - gap - e for e in reversed(reach[:-1])]
    bm = part.bm
    slot = part.slot('linen')
    rows = []
    for phi in phis:
        e = min(phi - gap, 2.0 * math.pi - gap - phi)
        frac = min(e / wing, 1.0)
        fold = lerp(0.58, 0.92, smoothstep(0.0, 1.0, frac)) * height
        angle = math.radians(170.0) * (1.0 - smoothstep(0.65, 1.0, frac))
        upper = fold + 0.6 * (top - fold)
        profile = [(0.0, 0.0), (0.0, fold), (0.0, upper), (0.0004, height - 0.0004), (0.0014, height + 0.0004),
                   (thick + 0.0004, height - 0.0008), (thick, upper), (thick, fold), (thick, 0.0)]
        out = (side * math.sin(phi) + fr * math.cos(phi) * depth).normalized()
        ring = base + (side * math.sin(phi) + fr * math.cos(phi) * depth) * r
        row = []
        for dr, h in profile:
            if h > fold and angle > 0.0:
                # Bent round the fold: s across the cloth from its middle, l along it past the fold.
                s, l = dr - thick * 0.5, h - fold
                a = min(l / bend_r, angle)
                cx = thick * 0.5 + bend_r
                dr = cx - (bend_r - s) * math.cos(a) + max(l - bend_r * angle, 0.0) * math.sin(angle)
                h = fold + (bend_r - s) * math.sin(a) + max(l - bend_r * angle, 0.0) * math.cos(angle)
            row.append(bm.verts.new(ring + out * dr + up * h))
        rows.append(row)
    n = len(rows[0])
    faces = [bm.faces.new((rows[i][k], rows[i][(k + 1) % n], rows[i + 1][(k + 1) % n], rows[i + 1][k]))
             for i in range(len(rows) - 1) for k in range(n)]
    faces += [bm.faces.new(list(reversed(rows[0]))), bm.faces.new(rows[-1])]
    for f in faces:
        f.material_index = slot
    # The band's faces wind one way all round its section: if the outer face low at the back looks inward, turn
    # them all. The ends look away from the band beside them.
    back = faces[(len(rows) // 2) * n + 7]
    back.normal_update()
    if back.normal.dot(back.calc_center_median() - (base + up * up.dot(back.calc_center_median() - base))) < 0.0:
        for f in faces[:-2]:
            f.normal_flip()
    orient_out(faces[-2:-1], sum((v.co for v in rows[1]), Vector()) / n)
    orient_out(faces[-1:], sum((v.co for v in rows[-2]), Vector()) / n)
    # The ends' sections are concave where the wing folds over: cut them into triangles here (ear clipping), not by
    # whatever reads the file.
    bmesh.ops.triangulate(bm, faces=faces[-2:], quad_method='BEAUTY', ngon_method='EAR_CLIP')


def bow_tie(part, at, axis, front):
    """A black bow tie, tied: the knot wrapped round its pinched middle, a loop flaring to each side with a pleat
    along its face, and the two ends showing behind and below the loops."""
    up = Vector(axis).normalized()
    fr = (Vector(front) - up * Vector(front).dot(up)).normalized()
    side = up.cross(fr).normalized()
    at = Vector(at)

    def knot(t, phi):
        k = 0.82 + 0.18 * math.sin(math.pi * t)
        return superellipse(phi, 0.0056 * k, 0.0048 * k, 0.0036 * k, 3.0)
    loft(part, [at - up * 0.0065, at - up * 0.002, at + up * 0.002, at + up * 0.0065], fr, knot, segs=8,
         caps=('pole', 'pole'), mat='wool', pole_ext=(0.0016, 0.0016))

    def lobe(h0, h1, h2, w0, w1, pleat):
        def sec(t, phi):
            h = keys([(0.0, h0), (0.3, h1), (0.75, h2), (1.0, h2 * 0.85)], t)
            w = keys([(0.0, w0), (0.5, w1), (1.0, w1 * 0.8)], t)
            x, y = superellipse(phi, w, h, h, 2.6)
            if x > 0.0:
                # A pleat along the loop's face, its fold catching the light.
                x -= pleat * bump(y / max(h, 1e-6), 0.4) * smoothstep(0.15, 0.4, t)
            return y, x
        return sec
    for s in (-1.0, 1.0):
        loop = path_through([at + side * s * 0.003, at + side * s * 0.016 + fr * 0.0008,
                             at + side * s * 0.03 - fr * 0.0018 + up * 0.0008,
                             at + side * s * 0.041 - fr * 0.0038 - up * 0.0012], 6)
        loft(part, loop, fr, lobe(0.0045, 0.009, 0.0125, 0.0028, 0.0042, 0.0016), segs=6, caps=('pole', 'pole'),
             mat='wool', pole_ext=(0.001, 0.003))
        end = path_through([at + side * s * 0.003 - fr * 0.003 - up * 0.002,
                            at + side * s * 0.018 - fr * 0.0055 - up * 0.0085,
                            at + side * s * 0.03 - fr * 0.0065 - up * 0.0125], 4)
        loft(part, end, fr, lobe(0.003, 0.006, 0.008, 0.002, 0.0028, 0.0), segs=6, caps=('pole', 'pole'),
             mat='wool', pole_ext=(0.001, 0.0025))


def lathe(part, frame, points, segs, slot, pole=None):
    """Rings round frame's Z at points(th) -> [(r, z), ...] per angle, joined into a surface (closed at the top by
    a pole at height pole, if given). Returns the rings."""
    bm = part.bm
    cols = [points(2.0 * math.pi * j / segs) for j in range(segs)]
    rings = [[bm.verts.new(frame @ Vector((r * math.sin(2.0 * math.pi * j / segs),
                                          -r * math.cos(2.0 * math.pi * j / segs), z)))
              for j, (r, z) in enumerate(col[k] for col in cols)] for k in range(len(cols[0]))]
    for k in range(len(rings) - 1):
        for j in range(segs):
            j1 = (j + 1) % segs
            bm.faces.new((rings[k][j], rings[k][j1], rings[k + 1][j1], rings[k + 1][j])).material_index = slot
    if pole is not None:
        top = bm.verts.new(frame @ Vector((0.0, 0.0, pole)))
        for j in range(segs):
            bm.faces.new((rings[-1][j], rings[-1][(j + 1) % segs], top)).material_index = slot
    return rings


def hat(part, frame, crown_r=0.097, height=0.33, brim=(0.052, 0.04), curl=0.034, flare=1.06, oval=0.07, segs=32,
        brim_segs=44):
    """The stovepipe, lathed about frame's Z: the tall crown flaring a little to its top, a lining inside it, and the
    brim (finer round, for its curved edge): its underside, its edge bound in a rolled bead, its top, curling up at
    the sides and dipping a little front and back; a band at the crown's foot with a flat bow on his left."""
    def crown(th, z):
        return crown_r * (1.0 + oval * math.cos(2.0 * th)) * lerp(1.0, flare, smoothstep(0.0, 1.0, z / height))

    def brim_w(th):
        return lerp(brim[1], brim[0], math.cos(th) ** 2)

    def brim_z(th, s):
        return curl * math.sin(th) ** 2 * s ** 2.2 - 0.004 * math.cos(th) ** 2 * s
    slot = part.slot('wool')
    # The brim, from under the crown's foot out to the rolled edge and back over the top (s across the brim; the
    # edge's bead stands out of its line).
    under = [(0.0, -0.0015, 0.0015), (0.45, -0.0018, 0.0), (0.82, -0.0013, 0.0), (0.95, -0.0014, 0.0)]
    bead = [(1.0, 0.0002, 0.0019), (1.0, 0.0038, 0.0024), (0.95, 0.0062, 0.0)]
    over = [(0.82, 0.0052, 0.0), (0.45, 0.0045, 0.0), (0.03, 0.0055, 0.0)]
    lathe(part, frame, lambda th: [(crown(th, 0.0) + brim_w(th) * s + dr, brim_z(th, s) + dz)
                                   for s, dz, dr in under + bead + over], brim_segs, slot)
    # The crown: its lining from inside down to the brim's foot, then the outside from inside the brim up to the
    # top, closed.
    lathe(part, frame, lambda th: [(crown(th, 0.0) - 0.004, 0.04), (crown(th, 0.0) + 0.0015, -0.0015)], segs, slot)
    lathe(part, frame, lambda th: [(crown(th, 0.003), 0.003), (crown(th, 0.33 * height), 0.33 * height + 0.008),
                                   (crown(th, 0.75 * height), 0.75 * height + 0.008),
                                   (crown(th, 0.975 * height), 0.975 * height + 0.008),
                                   (crown(th, height) * 0.9, height + 0.008 - 0.0004),
                                   (crown(th, height) * 0.5, height + 0.008 - 0.002)], segs, slot,
          pole=height + 0.008)
    # The band, a little proud of the crown, and a flat bow on its left side.
    lathe(part, frame, lambda th: [(crown(th, z) + g, z + 0.008) for z, g in
                                   ((0.006, 0.0018), (0.01, 0.003), (0.041, 0.003), (0.045, 0.0015))], segs, slot)
    bow_at = frame @ Vector((crown(math.pi * 0.5, 0.026) + 0.006, 0.0, 0.034))
    r3 = frame.to_3x3()
    f3 = Matrix((r3 @ Vector((0.0, 1.0, 0.0)), r3 @ Vector((0.0, 0.0, 1.0)), r3 @ Vector((1.0, 0.0, 0.0)))).transposed()
    ellipsoid(part, bow_at, (0.016, 0.007, 0.003), f3, segs=8, rings=4, mat='wool')


# --- The skirt: a cloth simulation, kept beside this file ---

def ring_columns(torso, t, phis, out):
    p0 = torso.at(t)[0]
    cols = []
    for phi in phis:
        p = torso.point(t, phi, out)
        d = p - p0
        cols.append((p, Vector((d.x, d.y, 0.0)).normalized()))
    return cols


def rail_colliders():
    """The rail's top and its two posts either side of him, as boxes for the cloth to fall on."""
    part = Part('_RailCollider')
    bm = part.bm
    for center, size in (((0.1, 0.0, -0.06), (4.0, 0.06, 0.12)), ((-0.565, -0.085, -0.5), (0.11, 0.11, 1.1)),
                         ((0.765, -0.085, -0.5), (0.11, 0.11, 1.1))):
        bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Translation(center) @
                              Matrix.Diagonal((size[0], size[1], size[2], 1.0)))
    part.slots = []
    obj = part.finish(keep=False)
    return obj


def _digest(grids, colliders, settings):
    h = hashlib.sha1(repr(settings).encode())
    h.update(np.round(np.array([[p.x, p.y, p.z] for g in grids for row in g for p in row], np.float64), 4).tobytes())
    dg = bpy.context.evaluated_depsgraph_get()
    for c in colliders:
        ev = c.evaluated_get(dg)
        me = ev.to_mesh()
        co = np.empty(len(me.vertices) * 3, np.float64)
        me.vertices.foreach_get('co', co)
        ev.to_mesh_clear()
        h.update(np.round(co, 4).tobytes())
    return h.hexdigest()[:16]


def simulate(grids, colliders, frames, stiff, mass=0.4, pin=(1.0, 0.55)):
    """Blender's cloth: the grids (rows of points, the top row first, round the body counter-clockwise from above)
    fall from their pinned top rows onto the colliders. Returns the settled points, grid by grid."""
    bm = bmesh.new()
    pins = []
    verts = []
    for grid in grids:
        vs = [[bm.verts.new(p) for p in row] for row in grid]
        verts.append(vs)
        for i, row in enumerate(vs):
            if i < len(pin):
                pins += [(v, pin[i]) for v in row]
        for i in range(len(vs) - 1):
            for j in range(len(vs[0]) - 1):
                bm.faces.new((vs[i][j], vs[i + 1][j], vs[i + 1][j + 1], vs[i][j + 1]))
    bm.verts.index_update()
    mesh = bpy.data.meshes.new('_drape')
    order = [[[v.index for v in row] for row in vs] for vs in verts]
    pin_list = [(v.index, w) for v, w in pins]
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new('_drape', mesh)
    bpy.context.scene.collection.objects.link(obj)
    group = obj.vertex_groups.new(name='pin')
    for vi, w in pin_list:
        group.add([vi], w, 'REPLACE')
    for c in colliders:
        c.modifiers.new('Collision', 'COLLISION')
        c.collision.thickness_outer = 0.006
        c.collision.thickness_inner = 0.01
        c.collision.cloth_friction = 8.0
        c.collision.damping = 0.2
    cloth = obj.modifiers.new('Cloth', 'CLOTH')
    st = cloth.settings
    st.quality, st.mass, st.air_damping = 7, mass, 2.0
    st.tension_stiffness, st.compression_stiffness, st.shear_stiffness, st.bending_stiffness = stiff
    st.tension_damping = st.compression_damping = st.shear_damping = 6.0
    st.bending_damping = 1.0
    st.vertex_group_mass = 'pin'
    st.pin_stiffness = 1.0
    cs = cloth.collision_settings
    cs.use_collision, cs.distance_min, cs.collision_quality = True, 0.005, 4
    cs.use_self_collision, cs.self_distance_min, cs.self_friction = True, 0.004, 5.0
    cloth.point_cache.frame_start, cloth.point_cache.frame_end = 1, frames
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 1, frames
    for f in range(1, frames + 1):
        scene.frame_set(f)
    dg = bpy.context.evaluated_depsgraph_get()
    settled = obj.evaluated_get(dg).to_mesh()
    co = [Vector(v.co) for v in settled.vertices]
    obj.evaluated_get(dg).to_mesh_clear()
    scene.frame_set(1)
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    for c in colliders:
        for m in list(c.modifiers):
            if m.type == 'COLLISION':
                c.modifiers.remove(m)
    return [[[co[i] for i in row] for row in grid] for grid in order]


def skirt(part, torso, colliders):
    """The frock coat's skirt: two panels sewn on at the waist seam (the front edges parting over the crossed legs,
    the back vent's tails falling over the rail behind him), simulated on a fine grid and kept at every second row
    and column (the caller gives it its cloth's thickness)."""
    t_w = torso.t_at_z(0.30)
    panels = [ring_columns(torso, t_w, [lerp(0.07, math.pi - 0.03, k / 33) for k in range(34)], 0.012),
              ring_columns(torso, t_w, [lerp(-math.pi + 0.03, -0.07, k / 33) for k in range(34)], 0.012)]
    rows = 20
    grids = []
    for panel in panels:
        grid = []
        for i in range(rows + 1):
            row = []
            for top, out in panel:
                a = math.radians(8.0 + 76.0 * smoothstep(-0.9, 0.5, out.y))
                d = Vector((out.x, out.y, 0.0)).normalized() * math.cos(a) - UP * math.sin(a)
                row.append(Vector(top) + d * lerp(0.58, 0.70, smoothstep(-0.6, 0.9, out.y)) * i / rows)
            grid.append(row)
        grids.append(grid)
    settings = (70, (30.0, 30.0, 12.0, 3.0), 0.4)
    key = _digest(grids, colliders, settings)
    settled = None
    if os.path.exists(DRAPE_FILE) and '--resim' not in ARGS:
        with open(DRAPE_FILE, encoding='utf-8') as file:
            data = json.load(file)
        if data.get('digest') == key:
            settled = [[[Vector(p) for p in row] for row in grid] for grid in data['grids']]
            log('skirt: the drape from MisterSexton.drape.json')
    if settled is None:
        log('skirt: simulating the drape (the body or the settings changed)')
        settled = simulate(grids, colliders, *settings[:2], mass=settings[2])
        with open(DRAPE_FILE, 'w', encoding='utf-8', newline='\n') as file:
            json.dump({'about': 'The settled drape of Mister Sexton\'s coat skirt (MisterSexton.py writes it: the '
                                'cloth simulation\'s result for the digest of its inputs).', 'digest': key,
                       'grids': [[[[round(c, 5) for c in p] for p in row] for row in grid] for grid in settled]},
                      file, separators=(',', ':'))
            file.write('\n')
        log(f'skirt: wrote {os.path.relpath(DRAPE_FILE, REPO)}')
    # Every second row and column of the fine drape, the outer surface.
    bm = part.bm
    slot = part.slot('wool')
    outer_faces = []
    for grid in settled:
        rows_k = list(range(0, len(grid), 2))
        cols_k = list(range(0, len(grid[0]), 2))
        if cols_k[-1] != len(grid[0]) - 1:
            cols_k.append(len(grid[0]) - 1)
        vs = [[bm.verts.new(grid[i][j]) for j in cols_k] for i in rows_k]
        for i in range(len(vs) - 1):
            for j in range(len(vs[0]) - 1):
                f = bm.faces.new((vs[i][j], vs[i + 1][j], vs[i + 1][j + 1], vs[i][j + 1]))
                f.material_index = slot
                outer_faces.append(f)
    return outer_faces


def buttons(part, body, spots, radius, height):
    for origin, direction in spots:
        hit = surface_hit(body, origin, direction)
        if hit is not None:
            p, n = hit
            dome(part, p + n * 0.0005, n, radius, height, segs=6, mat='metal')


# --- Putting him together ---

COAT_POINTS = [(0.0, 0.05, 0.03), (0.0, 0.06, 0.15), (0.0, 0.055, 0.30), (0.0, 0.035, 0.47), (0.0, 0.015, 0.58),
               (0.0, 0.0, 0.665), (0.0, -0.012, 0.775)]
COAT_TABLE = [(0.0, (0.16, 0.10, 0.14, 2.2)), (0.16, (0.168, 0.118, 0.13, 2.2)), (0.36, (0.14, 0.106, 0.102, 2.2)),
              (0.59, (0.158, 0.122, 0.106, 2.3)), (0.74, (0.172, 0.118, 0.104, 2.5)),
              (0.855, (0.195, 0.092, 0.094, 3.0)), (0.93, (0.15, 0.08, 0.086, 2.6)), (1.0, (0.07, 0.062, 0.068, 2.0))]
# The legs, the right crossed over the left knee on knee: hip, knee, ankle, and the foot's direction.
LEGS = {'L': (V(0.093, 0.03, 0.10), V(0.125, -0.48, -0.03), V(0.12, -0.47, -0.58), (0.04, -0.6, -0.8)),
        'R': (V(-0.093, 0.03, 0.10), V(0.07, -0.56, 0.08), V(0.205, -0.76, -0.42), (0.42, -0.5, -0.75))}
LEG_TABLE = [(0.0, (0.086, 0.086, 0.086, 2.0)), (0.1, (0.088, 0.084, 0.09, 2.0)), (0.3, (0.078, 0.073, 0.077, 2.0)),
             (0.47, (0.064, 0.066, 0.066, 2.0)), (0.53, (0.066, 0.07, 0.064, 2.0)), (0.63, (0.057, 0.051, 0.063, 2.0)),
             (0.86, (0.049, 0.047, 0.051, 2.0)), (1.0, (0.055, 0.054, 0.056, 2.0))]
# The arms: inside the shoulder, the shoulder, the elbow, the wrist.
ARMS = {'L': (V(0.11, 0.0, 0.665), V(0.175, 0.0, 0.64), V(0.26, -0.10, 0.32), V(0.20, -0.355, 0.235)),
        'R': (V(-0.11, 0.0, 0.665), V(-0.175, 0.0, 0.64), V(-0.26, -0.07, 0.34), V(-0.10, -0.32, 0.29))}
SLEEVE_TABLE = [(0.0, (0.052, 0.054, 0.054, 2.0)), (0.12, (0.057, 0.058, 0.058, 2.0)), (0.5, (0.049, 0.05, 0.052, 2.0)),
                (0.8, (0.044, 0.044, 0.047, 2.0)), (1.0, (0.044, 0.044, 0.046, 2.0))]
# The ledger on his right knee: the middle of its boards' underside, and its frame (x to his right, y along the spine
# away from him, z out of the pages; the far edge tilted up so the pages face him).
LEDGER_TILT, LEDGER_YAW = 16.0, -8.0
LEDGER_AT = V(0.03, -0.40, 0.172)
# One page's width, the pages' height, a board's thickness.
LEDGER_W, LEDGER_H, LEDGER_C = 0.235, 0.34, 0.005


def ledger_page_top(u, v=0.5):
    """The page block's top over its board, at u (0 at the gutter, 1 at the fore-edge) and v (0 at the near end, 1 at
    the far one): diving into the gutter, highest a little out from it, settling toward the fore-edge, which rounds
    down; the far outer corner curls up a little."""
    z = LEDGER_C + 0.006 + 0.016 * smoothstep(0.0, 0.16, u) * (1.0 - 0.3 * u)
    z -= 0.004 * smoothstep(0.9, 1.0, u) ** 2
    return z + 0.003 * smoothstep(0.55, 1.0, u) * smoothstep(0.7, 1.0, v)


def page_under(point):
    """The page's surface under a point (the ledger as it lies on his knee): a point on it and its normal."""
    m = ledger_matrix()
    local = m.inverted() @ Vector(point)
    u = min(abs(local.x) / LEDGER_W, 1.0)
    v = min(max(local.y / LEDGER_H + 0.5, 0.0), 1.0)
    return m @ Vector((local.x, local.y, ledger_page_top(u, v))), (m.to_3x3() @ UP).normalized()


def ledger_frame():
    a = math.radians(LEDGER_TILT)
    x = Vector((-1.0, 0.0, 0.0))
    y = Vector((0.0, -math.cos(a), math.sin(a)))
    return Matrix.Rotation(math.radians(LEDGER_YAW), 3, 'Z') @ Matrix((x, y, x.cross(y))).transposed()


def ledger_matrix():
    f3 = ledger_frame()
    return Matrix.Translation(LEDGER_AT + f3 @ Vector((0.0, 0.0, -0.005))) @ f3.to_4x4()


def apply_modifiers(obj):
    for mod in list(obj.modifiers):
        with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj],
                                       selected_editable_objects=[obj]):
            bpy.ops.object.modifier_apply(modifier=mod.name)


def join(objects, name):
    root = objects[0]
    for o in objects:
        apply_modifiers(o)
    with bpy.context.temp_override(active_object=root, object=root, selected_objects=objects,
                                   selected_editable_objects=objects):
        bpy.ops.object.join()
    root.name = name
    root.data.name = name
    return root


def uv_by_slot(obj):
    for mat in obj.data.materials:
        lt.box_uv(obj, UV_SETS[mat.name], faces=mat)


def smooth_occlusion(obj, iterations=2):
    """Averages the baked occlusion (the 'Col' alpha) per vertex and blends each with its neighbours' mean."""
    mesh = obj.data
    col = mesh.color_attributes['Col']
    raw = np.empty(4 * len(mesh.loops), np.float32)
    col.data.foreach_get('color', raw)
    loop_v = np.empty(len(mesh.loops), np.int64)
    mesh.loops.foreach_get('vertex_index', loop_v)
    nv = len(mesh.vertices)
    a = np.bincount(loop_v, weights=raw[3::4], minlength=nv) / np.maximum(np.bincount(loop_v, minlength=nv), 1)
    edges = np.empty(2 * len(mesh.edges), np.int64)
    mesh.edges.foreach_get('vertices', edges)
    edges = edges.reshape(-1, 2)
    for _ in range(iterations):
        total, count = np.zeros(nv), np.zeros(nv)
        np.add.at(total, edges[:, 0], a[edges[:, 1]])
        np.add.at(total, edges[:, 1], a[edges[:, 0]])
        np.add.at(count, edges[:, 0], 1.0)
        np.add.at(count, edges[:, 1], 1.0)
        a = 0.5 * a + 0.5 * total / np.maximum(count, 1.0)
    raw[3::4] = a[loop_v]
    col.data.foreach_set('color', raw)
    mesh.update()


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def build_sexton():
    PARTS.clear()
    rails = rail_colliders()
    torso = Torso(COAT_POINTS, COAT_TABLE, 22, dense_near((0.38, 0.12, 1.6), (0.9, 0.1, 1.2)),
                  folds=wrinkles(110, [(0.42, 0.07, 0.0, 1.1, 0.0045, 3), (0.62, 0.12, 0.0, 0.5, 0.002, 2)]))
    coat = Part('Coat')
    torso.build(coat)
    coat_obj = coat.finish()
    # The skirt drapes over the legs and boots at their full resolution (the drape's digest keys on them); the
    # model keeps lighter ones.
    legs_c, boots_c = Part('_LegsCollider'), Part('_BootsCollider')
    for side, (hip, knee, ankle, fdir) in LEGS.items():
        trouser_leg(legs_c, hip, knee, ankle, LEG_TABLE, 104 if side == 'L' else 105)
    for side, (hip, knee, ankle, fdir) in LEGS.items():
        boot(boots_c, ankle, fdir, knee - ankle)
    legs_c, boots_c = legs_c.finish(keep=False), boots_c.finish(keep=False)
    legs = Part('Legs')
    for side, (hip, knee, ankle, _) in LEGS.items():
        trouser_leg(legs, hip, knee, ankle, LEG_TABLE, 104 if side == 'L' else 105, rings=19)
    legs.finish()
    boots = Part('Boots')
    for side, (hip, knee, ankle, fdir) in LEGS.items():
        boot(boots, ankle, fdir, knee - ankle, sole_rings=7, heel_segs=6)
    boots.finish()
    sk = Part('Skirt')
    skirt(sk, torso, [coat_obj, legs_c, boots_c, rails])
    sk_obj = sk.finish()
    for o in (legs_c, boots_c):
        bpy.data.objects.remove(o)
    mod = sk_obj.modifiers.new('Solidify', 'SOLIDIFY')
    mod.thickness, mod.offset, mod.use_rim, mod.use_even_offset = 0.006, -1.0, True, False
    bpy.data.objects.remove(rails)
    tailoring = Part('Tailoring')
    t_w = torso.t_at_z(0.30)
    torso_band(tailoring, torso, t_w - 0.01, t_w + 0.014, 0.0145)
    neck0 = V(0.0, -0.012, 0.735)
    coat_collar(tailoring, torso, neck0, HEAD_CENTER + head_frame().to_3x3() @ UP * -0.128 - neck0, 1.3)
    lapels(tailoring, torso)
    tailoring.finish()
    # The hands lie on the ledger: his left spread flat on its left page, his right writing on the right one. They
    # are laid on the page first (nothing built), so the sleeves can end at the wrists they found.
    frame = ledger_frame()
    up_page, page = frame.col[1], frame.col[2]
    fwd_l = nrm(up_page * math.cos(math.radians(5.0)) - page * math.sin(math.radians(5.0)) + Vector((-0.15, 0.0, 0.0)))
    fwd_r = nrm(up_page * math.cos(math.radians(30.0)) - page * math.sin(math.radians(30.0)) + Vector((0.12, 0.0, 0.0)))
    back_r = nrm(page * math.cos(math.radians(45.0)) + Vector((-1.0, 0.0, 0.0)) * math.sin(math.radians(45.0)))
    specs = {'L': (fwd_l, page, (-1.0, 0.0, 0.0), 'flat'), 'R': (fwd_r, back_r, (1.0, 0.0, 0.0), 'pen')}
    arms = {}
    for side, (fwd, back, thumb_side, pose) in specs.items():
        w = ARMS[side][3]
        _, w, _ = hand(None, w, fwd, back, thumb_side, pose, rest=page_under(w + fwd * 0.11))
        arms[side] = ARMS[side][:3] + (w,)
        log(f'  {side} hand: the wrist moved {(w - ARMS[side][3]).length * 100:.1f} cm onto the page')
    sleeves = Part('Sleeves')
    cuffs = Part('Cuffs')
    for side, joint in arms.items():
        W, d = sleeve(sleeves, joint, SLEEVE_TABLE, 101 if side == 'L' else 102)
        cuff(cuffs, W, d, 0.035)
    sleeves.finish()
    cuffs.finish()
    hands = Part('Hands')
    pen = Part('Pen')
    for side, (fwd, back, thumb_side, pose) in specs.items():
        w = ARMS[side][3]
        _, _, nib_top = hand(hands, w, fwd, back, thumb_side, pose, rest=page_under(w + fwd * 0.11))
        if nib_top is not None:
            # The dip pen: its holder down to the nib's ferrule, the steel nib on the page.
            nib, top = nib_top
            axis = (top - nib).normalized()
            tube(pen, [top, nib + axis * 0.024], 0.0045, segs=6, mat='wool', per=2)
            tube(pen, [nib + axis * 0.026, nib], 0.0034, segs=6, mat='metal', per=2, radius_end=0.0006)
    hands.finish()
    pen.finish()
    hd = Part('Head')
    hf = head(hd)
    hd.finish()
    rot = hf.to_3x3()
    hup, hfront = rot @ UP, rot @ FRONT
    # The neckwear: a black stock round the neck's foot (it fills the coat's collar behind the neck), the wing
    # collar round the neck's bend (along the middle of its turn from the neck's lean to the head's axis), the bow
    # tied in front at its foot.
    neck = Part('Neck')
    n0, n1 = NECK_BASE, HEAD_CENTER + hup * -0.128
    nd = (n1 - n0).normalized()
    tube(neck, [n0 + nd * 0.005, n0 + nd * 0.068], 0.05, segs=12, mat='wool', caps=(None, None), per=2)
    bend = (nd + hup).normalized()
    nfront = (hfront - bend * hfront.dot(bend)).normalized()
    collar0 = n1 - bend * 0.03
    wing_collar(neck, collar0, bend, hfront, r=0.051)
    bow_tie(neck, collar0 + bend * 0.008 + nfront * 0.063, bend, hfront)
    neck.finish()
    ht = Part('Hat')
    hat(ht, hf @ Matrix.Translation((0.0, 0.005, 0.058)))
    ht.finish()
    btn = Part('Buttons')
    buttons(btn, coat_obj, [((0.0, -0.5, z), (0.0, 1.0, 0.0)) for z in (0.34, 0.392, 0.444, 0.496, 0.548, 0.6, 0.652)],
            0.0085, 0.0042)
    buttons(btn, coat_obj, [((s * 0.05, 0.5, 0.33), (0.0, -1.0, 0.0)) for s in (-1.0, 1.0)], 0.009, 0.0042)
    btn.finish()
    if '--stats' in ARGS:
        for o in PARTS:
            apply_modifiers(o)
            log(f'  {o.name}: {triangles(o)} triangles')
    root = join(list(PARTS), 'MisterSexton')
    uv_by_slot(root)
    lt.bake_vertex_ao(root, samples=48, distance=0.2, ground=False)
    smooth_occlusion(root)
    # The skin darkens to black toward the face's shadow line (on the head only, not the hands), and down toward the
    # jaw's edge, under which the throat is in shadow.
    mesh = root.data
    col = mesh.color_attributes['Col']
    raw = np.empty(4 * len(mesh.loops), np.float32)
    col.data.foreach_get('color', raw)
    skin = mesh.materials.find('SextonSkin')
    inv = hf.inverted()
    for poly in mesh.polygons:
        if poly.material_index != skin or (Vector(poly.center) - HEAD_CENTER).length > 0.2:
            continue
        for li in poly.loop_indices:
            local = inv @ mesh.vertices[mesh.loops[li].vertex_index].co
            ph = abs(math.atan2(local.x, -local.y))
            line, edge = shadow_line(ph), spline(JAW_EDGE, ph) - 0.004
            # Measured across the line, not straight down: where it drops round the chin the skin still fades over
            # FADE (0.08 is about the face's radius there, turning the angle into a distance).
            lo, hi = max(ph - 0.01, 0.0), ph + 0.01
            slope = (shadow_line(hi) - shadow_line(lo)) / (hi - lo)
            across = (line - local.z) / math.sqrt(1.0 + (slope / 0.08) ** 2)
            fade = smoothstep(0.0, FADE, across) * smoothstep(edge, edge + 0.008, local.z)
            raw[4 * li + 3] *= fade
    col.data.foreach_set('color', raw)
    mesh.update()
    root['Nanite'] = 0
    root['LODs'] = '50,20'
    # The ledger rests on his right knee; his captions come from his chin.
    for name, matrix in (('Ledger', ledger_matrix()), ('Speaker', hf @ Matrix.Translation((0.0, -0.105, -0.095)))):
        empty = bpy.data.objects.new('SOCKET_' + name, None)
        empty.empty_display_type = 'ARROWS'
        empty.empty_display_size = 0.08
        bpy.context.scene.collection.objects.link(empty)
        empty.parent = root
        empty.matrix_world = matrix
    # One convex hull round him: the extreme points of his mesh in 26 directions.
    co = np.empty(len(mesh.vertices) * 3, np.float64)
    mesh.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    dirs = [np.array((x, y, z), np.float64) for x in (-1, 0, 1) for y in (-1, 0, 1) for z in (-1, 0, 1) if (x, y, z) != (0, 0, 0)]
    picks = sorted({int(np.argmax(co @ d)) for d in dirs})
    hb = bmesh.new()
    pts = [hb.verts.new(co[i]) for i in picks]
    res = bmesh.ops.convex_hull(hb, input=pts)
    inside = [v for v in res['geom_interior'] + res['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(hb, geom=list(dict.fromkeys(inside)), context='VERTS')
    hull_mesh = bpy.data.meshes.new('UCX_MisterSexton')
    hb.to_mesh(hull_mesh)
    hb.free()
    hull = bpy.data.objects.new('UCX_MisterSexton', hull_mesh)
    bpy.context.scene.collection.objects.link(hull)
    hull.parent = root
    hull.display_type = 'WIRE'
    log(f'MisterSexton: {triangles(root)} triangles, {len(mesh.materials)} materials '
        f'({", ".join(m.name for m in mesh.materials)}), hull of {len(hull_mesh.vertices)} points')
    return root


# --- The ledger ---

def build_ledger():
    """The open ledger in its own frame (x to the reader's right, y up the page away from him, z out of the pages;
    the pivot under the middle of its boards): two oxblood boards with bevelled edges, the page blocks rising from
    their outer edges and diving into the gutter, a round spine, brass corners, and a ribbon over the far edge with
    a coin on its end."""
    part = Part('SextonLedger')
    bm = part.bm
    W, H, c = LEDGER_W, LEDGER_H, LEDGER_C
    for s in (-1.0, 1.0):
        before = set(bm.faces)
        cube = bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Translation((s * (W * 0.5 + 0.004), 0.0, c * 0.5)) @
                                     Matrix.Diagonal((W + 0.012, H + 0.014, c, 1.0)))['verts']
        edges = list(dict.fromkeys(e for v in cube for e in v.link_edges))
        # (The bevel clears every element's tag, so the board's faces are the ones that weren't there before it.)
        bmesh.ops.bevel(bm, geom=cube + edges, offset=0.0014, segments=1, affect='EDGES', profile=0.5)
        for f in bm.faces:
            if f not in before:
                f.material_index = part.slot('cover')
        slot = part.slot('pages')
        us = [0.0, 0.03, 0.07, 0.12, 0.18, 0.26, 0.36, 0.48, 0.62, 0.76, 0.88, 0.95, 1.0]
        ny = 5
        top, bot = [], []
        for u in us:
            x = s * (0.004 + u * (W - 0.006))
            row_t, row_b = [], []
            for j in range(ny + 1):
                v = j / ny
                y = -H * 0.5 + 0.003 + (H - 0.006) * v
                row_t.append(bm.verts.new((x, y, ledger_page_top(u, v))))
                row_b.append(bm.verts.new((x, y, c + 0.0005)))
            top.append(row_t)
            bot.append(row_b)
        faces = []
        nx = len(us) - 1
        for i in range(nx):
            for j in range(ny):
                faces.append(bm.faces.new((top[i][j], top[i + 1][j], top[i + 1][j + 1], top[i][j + 1])))
            faces.append(bm.faces.new((bot[i][0], bot[i + 1][0], top[i + 1][0], top[i][0])))
            faces.append(bm.faces.new((top[i][ny], top[i + 1][ny], bot[i + 1][ny], bot[i][ny])))
        for j in range(ny):
            faces.append(bm.faces.new((top[nx][j], bot[nx][j], bot[nx][j + 1], top[nx][j + 1])))
        mid = Vector((s * W * 0.5, 0.0, c + 0.008))
        orient_out(faces, mid)
        for f in faces:
            f.material_index = slot
        # Brass corners on the board's outer corners.
        for yc in (-1.0, 1.0):
            cx, cy = s * (W + 0.009), yc * (H * 0.5 + 0.0065)
            tri = [Vector((cx, cy, 0.0)), Vector((cx - s * 0.026, cy, 0.0)), Vector((cx, cy - yc * 0.026, 0.0))]
            lo = [bm.verts.new(p + Vector((0.0, 0.0, -0.0006))) for p in tri]
            hi = [bm.verts.new(p + Vector((0.0, 0.0, c + 0.0006))) for p in tri]
            cf = [bm.faces.new(lo), bm.faces.new(hi)] + [bm.faces.new((lo[k], lo[(k + 1) % 3], hi[(k + 1) % 3], hi[k]))
                                                        for k in range(3)]
            orient_out(cf, sum(tri, Vector()) / 3.0 + Vector((0.0, 0.0, c * 0.5)))
            for f in cf:
                f.material_index = part.slot('metal')
    # The spine: half a tube under the gutter.
    loft(part, [Vector((0.0, -H * 0.5 - 0.007, -0.0005)), Vector((0.0, H * 0.5 + 0.007, -0.0005))], UP,
         lambda t, phi: (0.012 * math.sin(phi), 0.012 * math.cos(phi) - 0.002), segs=8, caps=('pole', 'pole'),
         mat='cover', pole_ext=(0.001, 0.001))
    # The ribbon: out of the gutter at the far end, over the edge, hanging down; the coin on its end.
    pts = [(0.0, H * 0.32, ledger_page_top(0.0) + 0.001), (0.006, H * 0.5 + 0.004, c + 0.011),
           (0.014, H * 0.5 + 0.02, -0.02), (0.02, H * 0.5 + 0.03, -0.07), (0.022, H * 0.5 + 0.034, -0.12)]
    loft(part, path_through(pts, 9), UP, lambda t, phi: superellipse(phi, 0.0055, 0.0009, 0.0009, 2.0), segs=4,
         caps=('pole', 'pole'), mat='cover', pole_ext=(0.001, 0.001))
    end = Vector(pts[-1]) + Vector((0.0, 0.0, -0.009))
    n = Vector((0.2, 1.0, 0.0)).normalized()
    a = n.orthogonal().normalized()
    b = n.cross(a)
    ring0 = [bm.verts.new(end + (a * math.cos(2 * math.pi * k / 12) + b * math.sin(2 * math.pi * k / 12)) * 0.012
                          - n * 0.0012) for k in range(12)]
    ring1 = [bm.verts.new(v.co + n * 0.0024) for v in ring0]
    cf = [bm.faces.new(list(reversed(ring0))), bm.faces.new(ring1)] + \
         [bm.faces.new((ring0[k], ring0[(k + 1) % 12], ring1[(k + 1) % 12], ring1[k])) for k in range(12)]
    orient_out(cf, end)
    for f in cf:
        f.material_index = part.slot('metal')
    obj = part.finish(keep=False)
    obj.data.set_sharp_from_angle(angle=math.radians(40.0))
    uv_by_slot(obj)
    lt.bake_vertex_ao(obj, samples=48, distance=0.04, ground=False)
    smooth_occlusion(obj, 1)
    obj['Nanite'] = 0
    obj['LODs'] = '50,25'
    obj['Collision'] = 'None'
    log(f'SextonLedger: {triangles(obj)} triangles, materials {", ".join(m.name for m in obj.data.materials)}')
    return obj


SEXTON = build_sexton()
LEDGER = build_ledger()


# --- Previews ---

def _collection(name):
    coll = bpy.data.collections.get(name) or bpy.data.collections.new(name)
    if coll.name not in bpy.context.scene.collection.children:
        bpy.context.scene.collection.children.link(coll)
    return coll


def _move(obj, coll):
    for c in list(obj.users_collection):
        c.objects.unlink(obj)
    coll.objects.link(obj)


def _unlit(name, color):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    em = nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = lt.hex_color(color)
    mat.node_tree.links.new(em.outputs['Emission'], out.inputs['Surface'])
    return mat


def _white_col(mesh):
    col = mesh.color_attributes.get('Col') or mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))


def _sky(name, stops, sun_dir, glows, clouds=None):
    """A painted sky: colours by the view's height, glows round the sun, optionally a layer of painted cloud."""
    world = bpy.data.worlds.get(name) or bpy.data.worlds.new(name)
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    tc = nodes.new('ShaderNodeTexCoord')
    sep = nodes.new('ShaderNodeSeparateXYZ')
    links.new(tc.outputs['Generated'], sep.inputs['Vector'])
    mr = nodes.new('ShaderNodeMapRange')
    mr.inputs['From Min'].default_value = -1.0
    links.new(sep.outputs['Z'], mr.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(mr.outputs['Result'], ramp.inputs['Fac'])
    el = ramp.color_ramp.elements
    el[0].position, el[0].color = stops[0][0], lt.hex_color(stops[0][1])
    el[1].position, el[1].color = stops[-1][0], lt.hex_color(stops[-1][1])
    for pos, col in stops[1:-1]:
        el.new(pos).color = lt.hex_color(col)
    color = ramp.outputs['Color']
    dot = nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    links.new(tc.outputs['Generated'], dot.inputs[0])
    dot.inputs[1].default_value = Vector(sun_dir).normalized()
    mx = nodes.new('ShaderNodeMath')
    mx.operation = 'MAXIMUM'
    links.new(dot.outputs['Value'], mx.inputs[0])
    for gcol, power, gstr in glows:
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
        elv = nodes.new('ShaderNodeMapRange')
        elv.inputs['From Min'].default_value = 0.03
        elv.inputs['From Max'].default_value = 0.16
        links.new(sep.outputs['Z'], elv.inputs['Value'])
        mask = nodes.new('ShaderNodeMath')
        mask.operation = 'MULTIPLY'
        links.new(cr.outputs['Result'], mask.inputs[0])
        links.new(elv.outputs['Result'], mask.inputs[1])
        amt = nodes.new('ShaderNodeMath')
        amt.operation = 'MULTIPLY'
        links.new(mask.outputs['Value'], amt.inputs[0])
        amt.inputs[1].default_value = amount
        lit = nodes.new('ShaderNodeMath')
        lit.operation = 'POWER'
        links.new(mx.outputs['Value'], lit.inputs[0])
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


def _golden_world(sun):
    return _sky('SextonGolden', [(0.0, 0x6b6050), (0.47, 0x8e8270), (0.5, 0xe9d2a6), (0.53, 0xd9d6c4),
                                 (0.62, 0xa9bfd2), (1.0, 0x5d84b6)], sun,
                [(0xffe0a8, 24.0, 0.9), (0xfff2d0, 600.0, 6.0)], clouds=(0xe4e0dc, 0xfff0d8, 3.0, 0.56, 0.6))


def _dusk_world(sun):
    # The game's Dusk state: the sun 4 degrees up at 252 degrees glowing amber, the haze cooling to violet away from
    # it, dusky pink clouds.
    return _sky('SextonDusk', [(0.0, 0x1a1418), (0.47, 0x2e2630), (0.5, 0x7c7290), (0.512, 0x6e6478),
                               (0.56, 0x54484e), (0.7, 0x3e3438), (1.0, 0x2a2428)], sun,
                [(0xff9a48, 6.0, 0.5), (0xffc27a, 60.0, 1.3), (0xfff2dc, 1400.0, 40.0)],
                clouds=(0x8c7678, 0xe8b4a0, 3.2, 0.52, 0.85))


def _studio_world():
    return _sky('SextonStudio', [(0.0, 0x5c5850), (0.5, 0x9c968c), (1.0, 0x8fa0b4)], (0.3, -0.8, 0.5),
                [(0xffe6c0, 4.0, 0.25)])


def _sun(toward, strength, color, angle=3.0):
    obj = bpy.data.objects.get('_Sun')
    if obj is None:
        obj = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
        bpy.context.scene.collection.objects.link(obj)
    obj.data.energy, obj.data.color, obj.data.angle = strength, color, math.radians(angle)
    if hasattr(obj.data, 'use_shadow_jitter'):
        obj.data.use_shadow_jitter = True
    if hasattr(obj.data, 'shadow_maximum_resolution'):
        # Shadow texels no finer than 6 mm: finer ones resolve the smooth low-poly cloth's own facets, which Eevee
        # then draws as a hatching along the light's terminator (Unreal's shadow bias doesn't).
        obj.data.shadow_maximum_resolution = 0.006
    obj.rotation_euler = (-Vector(toward).normalized()).to_track_quat('-Z', 'Y').to_euler()


def _sun_dir(azimuth, elevation):
    """In the Lookout's frame (its front, -Y, faces west; north is -X, east +Y)."""
    a, e = math.radians(azimuth), math.radians(elevation)
    h = Vector((-1.0, 0.0, 0.0)) * math.cos(a) + Vector((0.0, 1.0, 0.0)) * math.sin(a)
    return (h * math.cos(e) + UP * math.sin(e)).normalized()


def _camera(loc, target, lens=50.0, ortho=None):
    cam = bpy.data.objects.get('_Cam')
    if cam is None:
        cam = bpy.data.objects.new('_Cam', bpy.data.cameras.new('_Cam'))
        bpy.context.scene.collection.objects.link(cam)
    cam.location = Vector(loc)
    cam.rotation_euler = (Vector(target) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens, cam.data.clip_start, cam.data.clip_end = lens, 0.02, 30000.0
    cam.data.type = 'ORTHO' if ortho else 'PERSP'
    if ortho:
        cam.data.ortho_scale = ortho
        cam.data.sensor_fit = 'VERTICAL'
    else:
        cam.data.sensor_fit = 'AUTO'
    bpy.context.view_layer.update()
    return cam


_LABELS = []


def _label(text, cam, sx, sy, size_px, W, H, color=0xf2a64a, align='LEFT', depth=1.0):
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
    curve.materials.append(_unlit(f'_Label{color:06x}', color))
    _LABELS.append(obj)


def _clear_labels():
    while _LABELS:
        obj = _LABELS.pop()
        data = obj.data
        bpy.data.objects.remove(obj)
        bpy.data.curves.remove(data)


def _render(path, cam, W, H, world, samples=96, exposure=0.0):
    scene = bpy.context.scene
    scene.camera, scene.world = cam, world
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    ee = scene.eevee
    ee.taa_render_samples = samples
    ee.use_shadows = ee.use_raytracing = ee.use_fast_gi = True
    ee.ray_tracing_method = 'SCREEN'
    scene.render.resolution_x, scene.render.resolution_y, scene.render.resolution_percentage = W, H, 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format, scene.render.image_settings.color_mode = 'PNG', 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.view_settings.exposure = exposure
    scene.render.filepath = path
    t = time.time()
    bpy.ops.render.render(write_still=True)
    log(f'rendered {os.path.basename(path)} in {time.time() - t:.1f}s')
    return path


def _pixels(path):
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = 'Non-Color'
    w, h = img.size
    px = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return px.reshape(h, w, 4)[::-1, :, :3]


def _lookout(coll):
    """The real Lookout, built without its occlusion bake; returns SOCKET_Sit's matrix."""
    before = set(bpy.data.objects)
    saved = sys.argv[:]
    sys.argv = [sys.argv[0], '--', '--only=Lookout', '--no-ao']
    path = os.path.join(REPO, 'Art', 'Models', 'Buildings', 'Lookout.py')
    try:
        with open(path, encoding='utf-8') as file:
            exec(compile(file.read(), path, 'exec'), {'__name__': 'lookout_build', '__file__': path})
    finally:
        sys.argv = saved
    bpy.context.view_layer.update()
    sit = None
    for obj in set(bpy.data.objects) - before:
        _move(obj, coll)
        if obj.name.startswith('UCX_'):
            obj.hide_render = True
        if obj.type == 'MESH':
            _white_col(obj.data)
        if obj.name.startswith('SOCKET_Sit'):
            sit = obj.matrix_world.copy()
        obj.name = '_' + obj.name
    return sit


def _ridges(coll, center, heading):
    """Silhouettes past the canyon (unlit, like the game's M_Backdrop), and the canyon floor far below."""
    objs = []
    for k, (dist, base, h, seed, kind) in enumerate([(380.0, -85.0, 50.0, 11, 'wall'), (1600.0, -60.0, 110.0, 23, 'mesa'),
                                                     (4200.0, -40.0, 260.0, 37, 'ridge'), (9000.0, -20.0, 480.0, 41, 'ridge')]):
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
        mesh.materials.append(_unlit(f'_Ridge{k}', 0x808080))
        objs.append(obj)
    floor = bpy.data.meshes.new('_CanyonFloor')
    s = 30000.0
    floor.from_pydata([(-s, -s, -92.0), (s, -s, -92.0), (s, s, -92.0), (-s, s, -92.0)], [], [(0, 1, 2, 3)])
    fo = bpy.data.objects.new('_CanyonFloor', floor)
    coll.objects.link(fo)
    floor.materials.append(_unlit('_CanyonFloor', 0x808080))
    objs.append(fo)
    return objs


def _recolor(objs, colors):
    for obj, color in zip(objs, colors):
        obj.data.materials[0].node_tree.nodes['Emission'].inputs['Color'].default_value = lt.hex_color(color)


def _blacken(objs):
    black = _unlit('_Black', 0x000000)
    saved = [(o, list(o.data.materials)) for o in objs]
    for o, mats in saved:
        for i in range(len(mats)):
            o.data.materials[i] = black

    def restore():
        for o, mats in saved:
            for i, m in enumerate(mats):
                o.data.materials[i] = m
    return restore


def _stand_in(coll):
    """The Lookout's front rail as a stand-in (its railing code: posts, a top rail at 1.05 m, a mid rail, saltire
    braces, on the house trim sheet) and a strip of deck boards, for the studio views."""
    import looter_ruins as lr
    before = set(bpy.data.objects)
    m = lr.Model('_StandIn', seed=601)
    lr.railing(m, (-1.895, -0.085, DECK), (2.095, -0.085, DECK), -DECK, face=(0.0, 1.0, 0.0), post_every=1.33, cross=True,
               seed=601)
    obj = m.finish(ao=False, preview=False, fallback=None)
    for o in set(bpy.data.objects) - before:
        _move(o, coll)
        o.name = '_' + o.name.lstrip('_')
    _white_col(obj.data)
    floor = bpy.data.meshes.new('_Floor')
    floor.from_pydata([(-30.0, -30.0, DECK - 0.002), (30.0, -30.0, DECK - 0.002), (30.0, 6.0, DECK - 0.002),
                       (-30.0, 6.0, DECK - 0.002), (-30.0, 6.0, 20.0), (30.0, 6.0, 20.0)], [],
                      [(0, 1, 2, 3), (3, 2, 5, 4)])
    fo = bpy.data.objects.new('_Floor', floor)
    coll.objects.link(fo)
    mat = bpy.data.materials.new('_FloorMat')
    mat.use_nodes = True
    mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = lt.hex_color(0x8d877d)
    mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = 0.95
    floor.materials.append(mat)


def previews():
    os.makedirs(OUT_DIR, exist_ok=True)
    scene = bpy.context.scene
    figure = [SEXTON, LEDGER]
    ledger_view = LEDGER.copy()
    ledger_view.name = '_LedgerOnKnee'
    scene.collection.objects.link(ledger_view)
    ledger_view.parent = SEXTON
    ledger_view.matrix_world = SEXTON.matrix_world @ ledger_matrix()
    LEDGER.hide_render = True
    figure_view = [SEXTON, ledger_view]
    hull = next(o for o in SEXTON.children if o.name.startswith('UCX_'))
    hull.hide_render = True
    lookout = _collection('_LookoutSet')
    sit = _lookout(lookout)
    ridges = _ridges(lookout, sit.translation, math.atan2(-1.0, 0.0))
    eye, aim = Vector((-1.45, -3.05, 0.62)), Vector((0.06, -0.24, 0.30))
    rot = sit.to_3x3()
    SEXTON.matrix_world = sit
    bpy.context.view_layer.update()
    day = (0xbca592, 0xc4b5aa, 0xbdbabf, 0xc7c9d0, 0xae9a86)
    # 1. A golden sun across him (the concept's hero view), and 2. the Day state's sun behind him, as at Main 2.
    golden = rot @ Vector((0.766, -0.643, math.tan(math.radians(15.0)))).normalized()
    _recolor(ridges, day)
    cam = _camera(sit @ eye, sit @ aim, lens=45.0)
    _sun(golden, 4.2, (1.0, 0.84, 0.64))
    _label('MISTER SEXTON  GAME MESH', cam, -0.9, 0.9, 40, 1080, 1350)
    _render(os.path.join(OUT_DIR, 'Sexton_Golden.png'), cam, 1080, 1350, _golden_world(golden))
    _clear_labels()
    day_sun = _sun_dir(247.5, 15.0)
    _sun(day_sun, 4.6, (1.0, 0.82, 0.62))
    _label('BACKLIT: THE DAY SUN AT MAIN 2', cam, -0.9, 0.9, 40, 1080, 1350)
    _render(os.path.join(OUT_DIR, 'Sexton_Backlit.png'), cam, 1080, 1350, _golden_world(day_sun))
    _clear_labels()
    # 3. The cold open: drawn black on the rail at dusk, seen low from the deck.
    dusk_sun = _sun_dir(252.0, 4.0)
    _recolor(ridges, (0x3a2620, 0x4a3634, 0x4e4454, 0x5c566a, 0x3a2c30))
    _sun(dusk_sun, 2.6, (1.0, 0.6, 0.34))
    restore = _blacken(figure_view)
    cam = _camera(sit @ Vector((-0.55, -2.9, DECK + 1.0)), sit @ Vector((0.0, -0.1, 0.33)), lens=30.0)
    _render(os.path.join(OUT_DIR, 'Sexton_Dusk.png'), cam, 1920, 1080, _dusk_world(dusk_sun), samples=64, exposure=0.2)
    restore()
    # 4. The hands and the ledger, and 5. the face's shadow edge, close.
    _recolor(ridges, day)
    _sun(golden, 4.2, (1.0, 0.84, 0.64))
    world = _golden_world(golden)
    shots = []
    for name, e, a, lens in (('hands', (-0.42, -1.0, 0.62), (0.04, -0.36, 0.22), 50.0),
                             ('hands2', (0.55, -0.95, 0.55), (0.06, -0.38, 0.22), 50.0)):
        cam = _camera(sit @ Vector(e), sit @ Vector(a), lens=lens)
        shots.append(_render(os.path.join(OUT_DIR, f'_{name}.png'), cam, 960, 960, world, samples=64))
    lt.write_png(os.path.join(OUT_DIR, 'Sexton_Hands.png'), lt.to8(np.concatenate([_pixels(p) for p in shots], axis=1)))
    shots = []
    for name, e, a, lens in (('face', (-0.32, -0.72, 0.9), (0.0, -0.08, 0.84), 60.0),
                             ('face2', (-0.75, -0.25, 0.86), (0.0, -0.07, 0.85), 60.0)):
        cam = _camera(sit @ Vector(e), sit @ Vector(a), lens=lens)
        shots.append(_render(os.path.join(OUT_DIR, f'_{name}.png'), cam, 960, 960, world, samples=64))
    lt.write_png(os.path.join(OUT_DIR, 'Sexton_FaceEdge.png'), lt.to8(np.concatenate([_pixels(p) for p in shots], axis=1)))
    for p in [os.path.join(OUT_DIR, f'_{n}.png') for n in ('hands', 'hands2', 'face', 'face2')]:
        os.remove(p)
    # 8. His head three-quarter, as a player standing on the deck sees it from about 1.5 m (eyes 1.65 m up).
    cam = _camera(sit @ Vector((-0.9, -1.265, DECK + 1.65)), sit @ Vector((0.0, -0.085, 0.83)), lens=110.0)
    _label('HEAD: A PLAYER ON THE DECK, 1.5 M AWAY', cam, -0.9, 0.9, 34, 1080, 1350)
    _render(os.path.join(OUT_DIR, 'Sexton_Head.png'), cam, 1080, 1350, world, samples=96)
    _clear_labels()
    # 7. Beside the concept: the same view and light.
    if os.path.exists(CONCEPT):
        game = _pixels(os.path.join(OUT_DIR, 'Sexton_Golden.png'))
        concept = _pixels(CONCEPT)
        if concept.shape == game.shape:
            lt.write_png(os.path.join(OUT_DIR, 'Sexton_VsConcept.png'), lt.to8(np.concatenate([concept, game], axis=1)))
    # 6. The LOD sheet, in the studio: LOD0, and Blender's decimation standing in for Unreal's 50% and 20%.
    lookout.hide_render = True
    studio = _collection('_Studio')
    _stand_in(studio)
    SEXTON.matrix_world = Matrix.Identity(4)
    ledger_view.matrix_world = ledger_matrix()
    bpy.context.view_layer.update()
    cam = _camera((-1.35, -2.75, 0.55), (0.02, -0.25, 0.2), lens=40.0)
    _sun((-0.55, -0.75, 0.62), 3.4, (1.0, 0.9, 0.78))
    panels = []
    base = triangles(SEXTON)
    for k, ratio in enumerate((1.0, 0.5, 0.2)):
        dec = None
        if ratio < 1.0:
            dec = SEXTON.modifiers.new('_Decimate', 'DECIMATE')
            dec.ratio = ratio
            bpy.context.view_layer.update()
        ev = SEXTON.evaluated_get(bpy.context.evaluated_depsgraph_get())
        tris = sum(len(p.vertices) - 2 for p in ev.data.polygons)
        _label(f'LOD{k}  {tris:,} TRIANGLES', cam, 0.0, -0.9, 30, 640, 900, align='CENTER')
        panels.append(_render(os.path.join(OUT_DIR, f'_lod{k}.png'), cam, 640, 900, _studio_world(), samples=48))
        _clear_labels()
        if dec is not None:
            SEXTON.modifiers.remove(dec)
    lt.write_png(os.path.join(OUT_DIR, 'Sexton_LODs.png'), lt.to8(np.concatenate([_pixels(p) for p in panels], axis=1)))
    for p in panels:
        os.remove(p)
    log(f'previews in {os.path.relpath(OUT_DIR, REPO)} (LOD0 {base} triangles)')
    # Leave the scene as the exporter wants it: the models at the origin, no helpers.
    bpy.data.objects.remove(ledger_view)
    LEDGER.hide_render = False
    SEXTON.matrix_world = Matrix.Identity(4)
    for coll in (lookout, studio):
        for o in list(coll.objects):
            bpy.data.objects.remove(o)
        bpy.data.collections.remove(coll)


if '--preview' in ARGS or lt.want_preview():
    previews()
