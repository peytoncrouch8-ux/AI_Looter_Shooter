"""Hob, the one-eyed crow: the Revenant crow the user picked on 2026-10-06 (Art/Backlog/Creatures/HobConcepts.py,
option C), as the small rig the game poses in code. Lean and brooding, a raven's heavy beak, a shaggy ruff and a long
wedge tail; his left socket is empty with a dull ember deep in it, his right eye has gone pale, and his wingtips and two
tail feathers fade to ash. He is a friend: no hit zones, so shots pass through him.

SK_Hob, about 4.4k triangles, smooth-skinned. Rest pose: perched, wings folded, head level, the feet on a flat perch
top at the origin (the grip point). The front faces Blender -Y (Unreal +X). Under the plumage the skin is one clean quad
surface; the plumage is feather cards over it: rows of short round feathers over the head, nasal bristles over the
beak's base, the beard and ruff, loose contour feathers, and on the wings the coverts in rows down to the leading edge.

Materials (no new textures):
  HobFeathers  Polymer tinted near-black, its roughness raised by M_World's RoughnessOffset (below): plumage, beak,
               legs, the socket's bowl
  HobEye       Polymer, pale grey: the good eye's iris (its pupil and lids are HobFeathers)
  HobEmber     a dull emissive surface (the old flat style, like LanternGlow): the coal deep in the socket
  HobAsh       the Ghost master on Polymer: the fading tips of P6-P9 and the two middle tail feathers. Vertex colors
               on these faces: R the tone zone (0, 1/3, 2/3, 1 from the dark base to pale ash), G cavity, B 0, A the
               fade (1 solid, toward 0 at the tip, where the master's dither dissolves it). Elsewhere the color is
               white; A is the masters' occlusion, a little at each feather's root (it tucks under the next) and none
               on the skin, which the open wings uncover.

Bones (parent):
  body (root), neck (body), head (neck), jaw (head), beak (head; a leaf at the beak's tip, the attach point the
  rig exporter can't make a socket for), tail (body), tail_fan_l, tail_fan_r (tail), per side wing_upper (body),
  wing_fore (wing_upper), wing_hand (wing_fore), wing_fan (wing_hand), thigh (body), shin (thigh), toes (shin),
  hallux (shin).
The wing bones carry the feathers: each feather moves rigidly with a blend of the two bones beside its root (the
secondaries between fore at the elbow and hand at the wrist, the primaries between hand and fan, the tertials between
upper and fore). A folded wing is no rotated open wing (the secondaries point back in both, while their roots swing
out along the arm), so the code opens a wing by blending each wing bone from its rest transform to its open transform
(open_pose() computes them; the script prints them in Unreal's component space as it runs), then flaps the whole wing
about the shoulder. Bones between two carriers turned alike move their feathers exactly; the primaries' fan turns 56
degrees across the hand, which shortens the middle primaries by up to an eighth when open. The folded wing's top edge
tucks under the neck's and breast's feathers, and the arm's pad has a floor: open, it is the underside of the arm and
of the leading edge, which stretches from the shoulder to the wrist.

    blender -b --factory-startup --python Art/Models/Creatures/Hob.py -- --preview
"""
import math
import os
import random
import sys
import zlib

import bmesh
import bpy
from mathutils import Matrix, Quaternion, Vector
from mathutils.bvhtree import BVHTree

import looter_creatures as lc
import looter_model as lm
import looter_textures as lt

ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
PREVIEW = '--preview' in ARGV or lt.want_preview()
UP = Vector((0.0, 0.0, 1.0))
SIDES = ((1, 'l'), (-1, 'r'))           # creature space has y to the left


def smooth(e0, e1, x):
    if e1 == e0:
        return 1.0 if x >= e1 else 0.0
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def turn(yaw=0.0, pitch=0.0, roll=0.0):
    return (Matrix.Rotation(math.radians(yaw), 4, 'Z') @ Matrix.Rotation(math.radians(pitch), 4, 'Y')
            @ Matrix.Rotation(math.radians(roll), 4, 'X'))


# --- The approved concept's shape (creature space: cm, x forward, y left, z up, origin at the grip point) ---

TILT = 34.0                              # the body's pitch, perched
HIP = Vector((-0.5, 0.0, 10.6))
HEAD_AT = Vector((4.9, 0.0, 19.8))
HS = 1.32                                # the head's scale over a crow's
BODY = Matrix.Translation(HIP) @ turn(pitch=-TILT)
HEAD = Matrix.Translation(HEAD_AT) @ Matrix.Scale(HS, 4)
BODY_ROT = BODY.to_3x3()


def b(p):
    return BODY @ Vector(p)


def h(p):
    return HEAD @ Vector(p)


def hdir(d):
    return Vector(d).normalized()


# The torso, thighs and head as soft ellipsoids (the concept's), only to retopologise the skin onto.
TORSO = [((0.0, 0.0, 0.0), (7.0, 3.8, 4.2)), ((4.0, 0.0, -0.4), (4.3, 3.7, 4.3)), ((-1.8, 0.0, -1.5), (5.0, 3.5, 3.5)),
         ((1.6, 0.0, 2.0), (4.4, 3.7, 2.6)), ((-5.8, 0.0, 0.5), (3.6, 2.6, 2.4)), ((-7.6, 0.0, -1.3), (2.6, 2.0, 1.9))]
FLUFF = 0.97
THIGH = ((-0.6, 2.2, -3.2), (2.5, 1.5, 2.2))
SKULL = [((0.0, 0.0, 0.0), (3.3, 2.8, 2.9)), ((1.9, 0.0, 0.45), (2.3, 1.9, 2.0)), ((0.9, 1.0, -1.2), (2.1, 1.6, 1.6)),
         ((-0.4, 0.0, -2.3), (2.4, 2.0, 2.2)), ((-1.6, 0.0, 0.3), (2.3, 2.5, 2.5))]
BROW = ((1.2, 0.0, 1.1), (2.4, 2.4, 1.7))   # a heavy brow over both eyes
NECK_BASE = (4.4, 0.0, 1.8)              # body frame
HEAD_BASE = (-1.4, 0.0, -1.8)            # head frame
BEAK = dict(inset=1.3, length=6.6, width=1.18, depth_u=1.75, depth_l=1.12, gape=-0.85, droop=0.85)
EYE_DIR = (0.42, 0.86, 0.26)             # mirrored: the left socket is empty, the right eye pale
EYE_R = 0.62 * HS                        # a touch larger than the concept's, so it reads
# The head's feather rows (the concept's): elevation, azimuth from and to (each side, from the front), count per side,
# length, width (head units) and lift (cm).
HEAD_ROWS = [(76.0, 20.0, 160.0, 3, 1.5, 1.2, 0.07), (56.0, 8.0, 168.0, 7, 1.5, 1.2, 0.07),
             (34.0, 8.0, 172.0, 8, 1.45, 1.15, 0.07), (12.0, 25.0, 172.0, 7, 1.4, 1.1, 0.06),
             (-12.0, 40.0, 160.0, 6, 1.4, 1.1, 0.06)]
# The face between the bristles and the eyes: small feathers, those by the eye lying down rather than back toward it.
FACE_ROWS = [(30.0, 5.0, 26.0, 3, 1.2, 1.0, 0.05, (-1.0, 0.0, -0.3)),
             (6.0, 6.0, 30.0, 3, 1.0, 0.9, 0.04, (-0.3, 0.0, -1.0)),
             (-14.0, 18.0, 34.0, 2, 1.0, 0.9, 0.04, (-0.5, 0.0, -1.0))]
WING = dict(wrist=(5.6, 3.3, 2.6), conv=0.15, rise=0.1, beta=50.0, R=6.5, gap=0.2)
TAIL = dict(base=(-7.6, 0.0, 0.6), length=22.5, width=4.3, grad=0.4, spread=16.0, droop=18.0)
FOOT, HEEL = (0.6, 2.3), (-0.5, 2.5, 4.8)
TOES = [(-24.0, 2.7, 1.0, 0.36), (2.0, 3.7, 1.15, 0.38), (30.0, 3.0, 1.0, 0.36), (190.0, 2.8, 1.45, 0.38)]

# The open wing (the concept's): bones in a planform (x out along the span, y back along the chord, cm) and the
# glide pose it turns by (degrees), round the shoulder.
PLAN = dict(S=(0.0, 0.0), E=(7.0, 1.0), W=(15.5, -0.4), T=(23.0, 1.0))
OPEN = dict(dihedral=8.0, sweep=4.0, aoa=6.0)
PRIMARY_LENGTHS = (15.5, 17.2, 19.0, 20.8, 22.4, 23.8, 24.6, 24.2, 21.8)
PRIMARY_ANGLES = (14.0, 19.0, 24.0, 29.0, 35.0, 43.0, 52.0, 61.0, 70.0)
GHOST_PRIMARIES = (6, 7, 8, 9)
GHOST_TAIL = (5, 6)


# --- Materials ---

# Polymer's own roughness (about 0.6) made the near-black plumage read as vinyl. M_World's RoughnessOffset (added to the
# map's roughness, clamped) takes it to about 0.75: a crow's soft sheen. The tint stays, so the ash still matches it.
ROUGHNESS_OFFSET = 0.15
FEATHERS = lt.material('Polymer', name='HobFeathers', tint=0x16161a, RoughnessOffset=ROUGHNESS_OFFSET)
EYE = lt.material('Polymer', name='HobEye', tint=0xb4b2a8)
EMBER = lm.material('HobEmber', 0xb4461f, Glow=0.6, Variation=0.04)


def ash_material():
    """The fading tips, on the Ghost master (masked, dithered). Every value is a property: the importer resets the
    instance to the master's defaults on each import, then sets only what is given here. Its nodes only preview it."""
    mat = bpy.data.materials.get('HobAsh') or bpy.data.materials.new('HobAsh')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    col = nodes.new('ShaderNodeVertexColor')
    col.layer_name = 'Col'
    sep = nodes.new('ShaderNodeSeparateColor')
    links.new(col.outputs['Color'], sep.inputs['Color'])
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.interpolation = 'CONSTANT'
    el = ramp.color_ramp.elements
    el[0].position, el[0].color = 0.0, lt.hex_color(0x16161a)
    el[1].position, el[1].color = 0.17, lt.hex_color(0x34322f)
    for pos, c in ((0.5, 0x625e57), (0.83, 0x9d978c)):
        e = el.new(pos)
        e.color = lt.hex_color(c)
    links.new(sep.outputs['Red'], ramp.inputs['Fac'])
    grain = nodes.new('ShaderNodeTexImage')
    grain.image = bpy.data.images.load(lt.texture_path('Polymer', 'BC'), check_existing=True)
    uvn = nodes.new('ShaderNodeUVMap')
    uvn.uv_map = 'UVMap'
    links.new(uvn.outputs['UV'], grain.inputs['Vector'])
    mul = nodes.new('ShaderNodeMix')
    mul.data_type = 'RGBA'
    mul.blend_type = 'MULTIPLY'
    mul.inputs['Factor'].default_value = 1.0
    links.new(ramp.outputs['Color'], mul.inputs['A'])
    links.new(grain.outputs['Color'], mul.inputs['B'])
    links.new(mul.outputs['Result'], bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = 0.62
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(0xb9b4aa)
    facing = nodes.new('ShaderNodeLayerWeight')
    facing.inputs['Blend'].default_value = 0.35
    rim = nodes.new('ShaderNodeMath')
    rim.operation = 'MULTIPLY'
    rim.inputs[1].default_value = 0.35
    links.new(facing.outputs['Facing'], rim.inputs[0])
    links.new(rim.outputs['Value'], bsdf.inputs['Emission Strength'])
    # The dither: a fine cell pattern against the fade (vertex alpha).
    coords = nodes.new('ShaderNodeTexCoord')
    vor = nodes.new('ShaderNodeTexVoronoi')
    vor.inputs['Scale'].default_value = 600.0
    links.new(coords.outputs['Object'], vor.inputs['Vector'])
    cell = nodes.new('ShaderNodeSeparateColor')
    links.new(vor.outputs['Color'], cell.inputs['Color'])
    keep = nodes.new('ShaderNodeMath')
    keep.operation = 'LESS_THAN'
    links.new(cell.outputs['Red'], keep.inputs[0])
    links.new(col.outputs['Alpha'], keep.inputs[1])
    links.new(keep.outputs['Value'], bsdf.inputs['Alpha'])
    mat.surface_render_method = 'DITHERED'
    for key in [k for k in mat.keys() if not k.startswith('_')]:
        del mat[key]
    mat['Master'] = 'Ghost'
    mat['TextureSet'] = 'Polymer'
    mat['Zone1Color'] = '#16161A'       # the feather's own black, where the fade starts
    mat['Zone2Color'] = '#34322F'
    mat['Zone3Color'] = '#625E57'
    mat['Zone4Color'] = '#9D978C'       # pale ash at the tip
    mat['RimColor'] = '#B9B4AA'         # a soft warm ash, dimmer and warmer than the Unpaid's
    mat['RimStrength'] = 0.5
    mat['GlowStrength'] = 0.05
    mat['NoiseScale'] = 4.0
    mat['NoiseSpeed'] = 0.05
    mat['FadeSoftness'] = 0.3
    mat['EmberStrength'] = 0.0
    mat.diffuse_color = lt.hex_color(0x625e57)
    return mat


ASH = ash_material()
WHITE = (1.0, 1.0, 1.0, 1.0)


# --- Mesh building: parts carry UVs, a material per face, a color per corner and bone weights per vertex ---

class Part:
    def __init__(self, name):
        self.name = name
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')
        self.weights = []                # per vertex: {bone: weight}
        self.colors = []                 # per face: corner colors (sRGB values as the FBX gets them) or None
        self.mats = []
        self.w = {'body': 1.0}           # the weights new vertices take

    def slot(self, mat):
        if mat not in self.mats:
            self.mats.append(mat)
        return self.mats.index(mat)

    def vert(self, p, weights=None):
        v = self.bm.verts.new(Vector(p))
        self.weights.append(dict(weights or self.w))
        return v

    def face(self, verts, mat, colors=None):
        try:
            f = self.bm.faces.new(verts)
        except ValueError:
            return None
        f.material_index = self.slot(mat)
        f.smooth = True
        self.colors.append(colors)
        return f

    def orient(self, faces, outward):
        """Turns faces to face outward(center) (a vector function of the face's center): the game draws one side."""
        for f in faces:
            f.normal_update()
            if f.normal.dot(outward(f.calc_center_median())) < 0.0:
                f.normal_flip()

    def finish(self):
        """The part as an object in Blender meters (front to -Y), with its weights and colors."""
        bmesh.ops.transform(self.bm, matrix=lc.TO_BLENDER, verts=self.bm.verts)
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        for m in self.mats:
            mesh.materials.append(m)
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        groups = {}
        for i, ws in enumerate(self.weights):
            total = sum(ws.values()) or 1.0
            for bone, w in ws.items():
                if w <= 1e-4:
                    continue
                if bone not in groups:
                    groups[bone] = obj.vertex_groups.new(name=bone)
                groups[bone].add([i], w / total, 'REPLACE')
        col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
        values = []
        for poly, colors in zip(mesh.polygons, self.colors):
            for k in range(poly.loop_total):
                values.extend(colors[k] if colors else WHITE)
        col.data.foreach_set('color_srgb', values)
        return obj


def tube(part, pts, radii, mat, sides=6, caps=(True, True), weights=None):
    """A closed tube along pts (parallel-transported rings); weights: per point, or one dict for all."""
    pts = [Vector(p) for p in pts]
    n = len(pts)
    t_prev = (pts[1] - pts[0]).normalized()
    ref = UP - t_prev * UP.dot(t_prev)
    if ref.length < 1e-4:
        ref = t_prev.orthogonal()
    ref.normalize()
    rings = []
    for k, p in enumerate(pts):
        w = weights[k] if isinstance(weights, list) else weights
        t = (pts[min(k + 1, n - 1)] - pts[max(k - 1, 0)]).normalized()
        ref = t_prev.rotation_difference(t) @ ref
        ref = (ref - t * ref.dot(t)).normalized()
        side = t.cross(ref)
        rings.append([part.vert(p + (ref * math.cos(a) + side * math.sin(a)) * radii[k], w)
                      for a in (2.0 * math.pi * j / sides for j in range(sides))])
        t_prev = t
    for k in range(n - 1):
        for j in range(sides):
            j1 = (j + 1) % sides
            part.face((rings[k][j], rings[k][j1], rings[k + 1][j1], rings[k + 1][j]), mat)
    for end, (ring, p, d) in enumerate(((rings[0], pts[0], pts[0] - pts[1]), (rings[-1], pts[-1], pts[-1] - pts[-2]))):
        if not caps[end]:
            continue
        w = (weights[0] if end == 0 else weights[-1]) if isinstance(weights, list) else weights
        hub = part.vert(p + d.normalized() * radii[0 if end == 0 else -1] * 0.4, w)
        for j in range(sides):
            j1 = (j + 1) % sides
            part.face((hub, ring[j1], ring[j]) if end == 0 else (hub, ring[j], ring[j1]), mat)
    return rings


def ellipsoid(part, center, radii, mat, rot=None, segs=10, rings=6, weights=None, half=False):
    """A smooth ellipsoid (or, half=True, its +z half as a dome); rot turns its axes."""
    rot = rot.to_3x3() if rot is not None else Matrix.Identity(3)
    center = Vector(center)
    last = rings // 2 if half else rings
    grid = []
    for i in range(last + 1):
        th = math.pi * i / rings
        if i == 0:
            grid.append([part.vert(center + rot @ Vector((0.0, 0.0, radii[2])), weights)])
            continue
        if i == rings:
            grid.append([part.vert(center + rot @ Vector((0.0, 0.0, -radii[2])), weights)])
            continue
        grid.append([part.vert(center + rot @ Vector((math.sin(th) * math.cos(2 * math.pi * j / segs) * radii[0],
                                                       math.sin(th) * math.sin(2 * math.pi * j / segs) * radii[1],
                                                       math.cos(th) * radii[2])), weights) for j in range(segs)])
    for i in range(last):
        a, c = grid[i], grid[i + 1]
        for j in range(segs):
            j1 = (j + 1) % segs
            if len(a) == 1:
                part.face((a[0], c[j], c[j1]), mat)
            elif len(c) == 1:
                part.face((a[j], c[0], a[j1]), mat)
            else:
                part.face((a[j], c[j], c[j1], a[j1]), mat)


# --- Feathers at game density ---

def outline(shape, t, L, W):
    """A feather's half-width (cm) at t (0 root, 1 tip)."""
    if shape == 'flight':        # a narrow quill, the vane widening, then easing in to a long round tip
        f = (0.32 + 0.68 * smooth(0.0, 0.3, t)) * (1.0 - 0.12 * smooth(0.4, 0.8, t))
        tr = 0.72
    elif shape == 'primary':     # emarginated: the outer part narrows into a finger, tapering to a round end
        f = (0.32 + 0.68 * smooth(0.0, 0.26, t)) * (1.0 - 0.32 * smooth(0.42, 0.6, t)) * (1.0 - 0.15 * smooth(0.6, 1.0, t))
        tr = 0.7
    elif shape == 'pointed':     # hackles and tufts
        return 0.5 * W * max(0.12, math.sin(math.pi * min(0.08 + 0.92 * t, 1.0) ** 0.75) ** 0.8)
    else:                        # 'round': coverts, contour feathers
        f = 0.6 + 0.4 * smooth(0.0, 0.35, t)
        tr = max(0.4, 1.0 - 0.5 * W / L)
    if t > tr:
        k = (t - tr) / (1.0 - tr)
        f *= math.sqrt(max(0.0, 1.0 - k * k))
    return 0.5 * W * f


def card(part, to3d, L, W, mat, shape='flight', rows=(0.0, 0.3, 0.6, 0.86), lift=None, asym=0.0, two_sided=False,
         thick=0.06, ash_from=None, end=None, twist=0.0, curl=0.0, curl_from=0.0, bend=0.0, ao_root=None, weigh=None):
    """One feather as shaped geometry: rows across its length, two edge vertices each, and a tip; with end, a short
    end edge (that share of the last row's width) instead, so the tip is round, not a point. From curl_from out, twist
    turns the vane about its shaft (degrees at the tip) and curl lifts it (cm at the tip); bend sweeps the shaft
    sideways (cm at the tip, toward +y). two_sided adds an underside a hair below (its own normals, so neither side
    shades the other). ash_from (0..1 along) turns the rest of the feather into HobAsh, its corners colored for the
    Ghost master. ao_root darkens the root (vertex alpha: the masters' occlusion), clearing by half the length, so each
    feather shows where it tucks under the next. weigh(t) gives the bones along the feather (else the part's weights)."""
    rows = list(rows)
    if ash_from is not None:
        rows = [t for t in rows if t < ash_from - 0.05] + [ash_from, ash_from + (1.0 - ash_from) * 0.4,
                                                            ash_from + (1.0 - ash_from) * 0.72]
    layers = [(0.0, False)] + ([(-thick, True)] if two_sided else [])

    def shaped(t):
        """(turn of the vane in radians, extra height, sideways sweep) at t along."""
        k = max(0.0, (t - curl_from) / (1.0 - curl_from)) if curl_from < 1.0 else 0.0
        return math.radians(twist) * k, curl * k * k + (lift(t) if lift else 0.0), bend * t * t

    def edge(t, half, dh, sign):
        a, hh, sweep = shaped(t)
        return part.vert(to3d(t * L, sign * half * math.cos(a) + sweep, hh + dh - sign * half * math.sin(a)),
                         weigh(t) if weigh else None)
    for dh, under in layers:
        grid = []
        for t in rows:
            w = outline(shape, t, L, W)
            lw = w * ((1.0 - asym) if asym > 0 else 1.0)
            rw = w * ((1.0 + asym) if asym < 0 else 1.0)
            grid.append((edge(t, lw, dh, -1.0), edge(t, rw, dh, 1.0), t, lw, rw))

        def colors(ts):
            if ash_from is not None and min(ts) >= ash_from - 1e-6:
                out = []
                for t in ts:
                    s = min(max((t - ash_from) / (1.0 - ash_from), 0.0), 1.0)
                    out.append((s, 0.3, 0.0, 1.0 - 0.9 * s ** 1.2))
                return out
            if ao_root is None:
                return None
            return [(1.0, 1.0, 1.0, lerp(ao_root, 1.0, smooth(0.0, 0.5, t))) for t in ts]

        def mat_for(t0):
            return ASH if (ash_from is not None and t0 >= ash_from - 1e-6) else mat
        for (l0, r0, t0, _, _), (l1, r1, t1, _, _) in zip(grid, grid[1:]):
            q = (l0, l1, r1, r0)
            cs = colors((t0, t1, t1, t0))
            if under:
                q, cs = tuple(reversed(q)), (list(reversed(cs)) if cs else None)
            part.face(q, mat_for(t0), cs)
        l0, r0, t0, lw0, rw0 = grid[-1]
        if end:
            face = (l0, edge(1.0, lw0 * end, dh, -1.0), edge(1.0, rw0 * end, dh, 1.0), r0)
            cs = colors((t0, 1.0, 1.0, t0))
        else:
            a, hh, sweep = shaped(1.0)
            face = (l0, part.vert(to3d(L, sweep, hh + dh), weigh(1.0) if weigh else None), r0)
            cs = colors((t0, 1.0, t0))
        if under:
            face, cs = tuple(reversed(face)), (list(reversed(cs)) if cs else None)
        part.face(face, mat_for(t0), cs)


def free_frame(base, along, normal):
    d = Vector(along).normalized()
    n = Vector(normal)
    n = (n - d * n.dot(d)).normalized()
    s = n.cross(d)
    base = Vector(base)

    def to3d(a, y, hh):
        return base + d * a + s * y + n * hh
    return to3d


# --- The sculpt the skin is fitted to (the concept's metaballs; removed again) ---

THRESH = 0.6


def sculpt(brow=True):
    """The concept's body and head as one soft surface (a BVH tree); the metaball and its mesh are removed."""
    els = []
    bq, hq = BODY.to_quaternion(), HEAD.to_quaternion()

    def add(center, quat, semi, stiff=2.4):
        els.append((center, quat, semi, stiff))
    for c, semi in TORSO:
        add(b(c), bq, [v * FLUFF for v in semi])
    for side in (1, -1):
        c, semi = THIGH
        add(b((c[0], side * c[1], c[2])), bq, semi)
    for c, semi in SKULL + ([BROW] if brow else []):
        for side in ((1, -1) if c[1] else (1,)):
            add(h((c[0], side * c[1], c[2])), hq, [v * HS for v in semi])
    a, z = b(NECK_BASE), h(HEAD_BASE)
    for t in (0.2, 0.5, 0.8):
        r = lerp(3.5, 2.5 * HS, t)
        add(a.lerp(z, t), bq.slerp(hq, t), (r, r * 0.95, r))
    mb = bpy.data.metaballs.new('_HobSculpt')
    mb.resolution = mb.render_resolution = 0.3
    mb.threshold = THRESH
    for center, quat, semi, stiff in els:
        el = mb.elements.new(type='ELLIPSOID')
        vis = math.sqrt(1.0 - (THRESH / stiff) ** (1.0 / 3.0))
        big = max(semi)
        el.co = center
        el.radius = big / vis
        el.size_x, el.size_y, el.size_z = [s / big for s in semi]
        el.rotation = quat
        el.stiffness = stiff
    obj = bpy.data.objects.new('_HobSculpt', mb)
    bpy.context.scene.collection.objects.link(obj)
    mesh = bpy.data.meshes.new_from_object(obj.evaluated_get(bpy.context.evaluated_depsgraph_get()))
    bpy.data.objects.remove(obj)
    bpy.data.metaballs.remove(mb)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bpy.data.meshes.remove(mesh)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.015)
    for _ in range(3):
        bmesh.ops.smooth_vert(bm, verts=bm.verts, factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    bm.normal_update()
    tree = BVHTree.FromBMesh(bm)
    bm.free()
    return tree


SCULPT = sculpt(brow=False)          # first without the brow: where the beak leaves the face (below)


def cast(origin, direction, dist=60.0):
    loc, normal, _, _ = SCULPT.ray_cast(Vector(origin), Vector(direction).normalized(), dist)
    return loc, normal


def toward(inner, point):
    d = Vector(point) - Vector(inner)
    loc, normal = cast(inner, d)
    return (loc, normal) if loc is not None else (Vector(point), d.normalized())


# --- The skin: rings along a spine, cast onto the sculpt, relaxed, as clean quads ---

def catmull(points, t):
    n = len(points) - 1
    x = min(max(t, 0.0), 1.0) * n
    k = min(int(x), n - 1)
    f = x - k
    p0, p1, p2 = points[max(k - 1, 0)], points[k], points[k + 1]
    p3 = points[min(k + 2, n)]
    return 0.5 * ((2 * p1) + (-p0 + p2) * f + (2 * p0 - 5 * p1 + 4 * p2 - p3) * f * f + (-p0 + 3 * p1 - 3 * p2 + p3) * f ** 3)


def face_x():
    """Where the beak leaves the face (head frame x)."""
    hit, _ = cast(h((0.0, 0.0, BEAK['gape'] + 0.45)), (1.0, 0.0, 0.0))
    return (HEAD.inverted() @ hit).x if hit is not None else 3.6


FACE_X = face_x()
SCULPT = sculpt()                     # the brow's field reaches the face, so it comes after: the beak stays put
SOCKET_CENTER = None                     # set by the skin: where the left socket is


def skin_weights(s):
    """The skin's bones by how far along the spine (0 the beak, 1 the rump)."""
    head = 1.0 - smooth(0.2, 0.36, s)
    neck = smooth(0.2, 0.34, s) * (1.0 - smooth(0.36, 0.5, s))
    tail = 0.45 * smooth(0.9, 1.0, s)
    body = max(0.0, 1.0 - head - neck - tail)
    return {k: v for k, v in (('head', head), ('neck', neck), ('body', body), ('tail', tail)) if v > 1e-3}


def build_skin():
    """The body, neck and head as one quad skin: 16 around, rings along a spine from the face to the rump, each ray
    from the spine to the sculpt, then relaxed over the surface. The left socket is pressed in for its bowl."""
    pts = [h((FACE_X - 0.6, 0.0, BEAK['gape'] + 0.55)), h((1.6, 0.0, 0.15)), h((0.0, 0.0, 0.0)), h((-1.2, 0.0, -1.0)),
           (h((-1.6, 0.0, -2.2)) + b(NECK_BASE)) * 0.5 + Vector((0.6, 0.0, 0.0)), b((4.4, 0.0, 0.6)), b((2.0, 0.0, 0.1)),
           b((-1.0, 0.0, 0.0)), b((-4.0, 0.0, 0.0)), b((-6.6, 0.0, 0.0)), b((-8.6, 0.0, -0.3))]
    pts = [Vector(p) for p in pts]
    N, S = 16, 22
    # Denser over the head, where the silhouette turns fastest.
    ts = [(i / (S - 1)) ** 1.3 for i in range(S)]
    centers = [catmull(pts, t) for t in ts]
    head_up = UP.copy()
    body_up = BODY_ROT @ UP
    rings = []
    t_prev = (centers[1] - centers[0]).normalized()
    ref = head_up - t_prev * head_up.dot(t_prev)
    ref.normalize()
    for i, c in enumerate(centers):
        t = (centers[min(i + 1, S - 1)] - centers[max(i - 1, 0)]).normalized()
        ref = t_prev.rotation_difference(t) @ ref
        want = head_up.lerp(body_up, smooth(0.25, 0.5, ts[i]))
        want = (want - t * want.dot(t)).normalized()
        ref = (ref - t * ref.dot(t)).normalized().lerp(want, 0.5).normalized()
        side = t.cross(ref)
        ring = []
        for j in range(N):
            a = 2.0 * math.pi * j / N
            d = ref * math.cos(a) + side * math.sin(a)
            hit, _ = cast(c, d)
            ring.append(hit if hit is not None else c + d * 3.0)
        rings.append(ring)
        t_prev = t
    # Relax: each vertex toward its neighbours' mean, then back onto the sculpt (closest point), a few times.
    for _ in range(8):
        new = []
        for i in range(S):
            row = []
            for j in range(N):
                p = rings[i][j]
                nb = [rings[i][(j + 1) % N], rings[i][(j - 1) % N]]
                if 0 < i < S - 1:
                    nb += [rings[i - 1][j], rings[i + 1][j]]
                mean = sum(nb, Vector()) / len(nb)
                q = p.lerp(mean, 0.5)
                hit = SCULPT.find_nearest(q)
                row.append(hit[0] if hit[0] is not None else q)
            new.append(row)
        rings = new
    # Press the left socket in, so its bowl shows (the bowl's rim covers the dent's edge).
    global SOCKET_CENTER
    eye = hdir(EYE_DIR)
    hit, nrm = cast(h((0.4, 0.0, 0.0)), eye)
    SOCKET_CENTER = (hit, nrm)
    for i in range(S):
        for j in range(N):
            p = rings[i][j]
            d = (p - hit).length
            if d < 1.9 * HS and p.y > 0:
                rings[i][j] = p - nrm * 1.25 * HS * smooth(1.9 * HS, 0.4 * HS, d)
    part = Part('HobSkin')
    grid = [[part.vert(rings[i][j], skin_weights(ts[i])) for j in range(N)] for i in range(S)]
    for i in range(S - 1):
        for j in range(N):
            j1 = (j + 1) % N
            part.face((grid[i][j], grid[i][j1], grid[i + 1][j1], grid[i + 1][j]), FEATHERS)
    for end, i in ((0, 0), (1, S - 1)):
        c = sum(rings[i], Vector()) / N
        hub = part.vert(c + (centers[0] - centers[1]).normalized() * 0.3 if end == 0 else c, skin_weights(ts[i]))
        for j in range(N):
            j1 = (j + 1) % N
            part.face((hub, grid[i][j1], grid[i][j]) if end == 0 else (hub, grid[i][j], grid[i][j1]), FEATHERS)
    return part


# --- Head parts ---

def build_beak():
    """Two mandibles lofted along the gape line: the upper on the head, the lower on the jaw."""
    part = Part('HobBeak')
    bk = BEAK
    L = bk['length']
    x0 = FACE_X - bk['inset']

    def gape(t):
        return bk['gape'] - bk['droop'] * t ** 1.6
    n, around = 6, 7
    for upper in (True, False):
        bone = {'head': 1.0} if upper else {'jaw': 1.0}
        t_end = 1.0 if upper else 0.92
        rings = []
        for i in range(n):
            t = t_end * (i / (n - 1)) ** 0.85
            if upper:
                w = bk['width'] * (1.0 - t) ** 0.72 + 0.05
                hh = bk['depth_u'] * (1.0 - t ** 1.25) ** 0.85 + 0.04
            else:
                w = bk['width'] * 0.86 * (1.0 - t) ** 0.8 + 0.045
                hh = bk['depth_l'] * (1.0 - t ** 1.15) ** 0.9 + 0.03
            zg = gape(t)
            ring = []
            for j in range(around):
                ph = 2.0 * math.pi * j / around
                c, sn = math.cos(ph), math.sin(ph)
                y = w * c
                if upper:
                    z = zg + (hh * abs(sn) ** 0.85 if sn >= 0.0 else -0.05 * abs(sn) * w)
                else:
                    z = zg - (hh * abs(sn) ** 0.9 if sn < 0.0 else -0.04 * abs(sn) * w) - 0.02
                ring.append(part.vert(h((x0 + t * L, y, z)), bone))
            rings.append(ring)
        faces = []
        for i in range(n - 1):
            for j in range(around):
                j1 = (j + 1) % around
                faces.append(part.face((rings[i][j], rings[i][j1], rings[i + 1][j1], rings[i + 1][j]), FEATHERS))
        tip_t = t_end + (0.03 if upper else 0.01)
        tip = part.vert(h((x0 + tip_t * L, 0.0, gape(tip_t) + (0.02 if upper else -0.04))), bone)
        for j in range(around):
            faces.append(part.face((rings[-1][j], rings[-1][(j + 1) % around], tip), FEATHERS))
        hub = part.vert(h((x0 - 0.3, 0.0, gape(0.0))), bone)
        for j in range(around):
            faces.append(part.face((hub, rings[0][(j + 1) % around], rings[0][j]), FEATHERS))
        # Each mandible is closed: its normals made consistent and outward.
        bmesh.ops.recalc_face_normals(part.bm, faces=[f for f in faces if f])
    # Nasal bristles: a tuft of short stiff feathers lying forward over the base of the upper mandible (every crow has
    # them), in two layers, the top one longer and over the other. They hide where the beak meets the face.
    rng = random.Random(23)
    part.w = {'head': 1.0}
    for side in (1, -1):
        for a in (0.62, 0.82, 1.0):
            across = side * a
            base = h((FACE_X - 0.6, across * bk['width'] * 0.8, bk['gape'] + bk['depth_u'] * (0.9 - 0.42 * a * a)))
            d = hdir((1.0, across * 0.3, -0.3 - 0.35 * a * a))
            nrm = hdir((0.0, across * 0.85, 1.0 - 0.3 * a * a))
            Lb = 2.0 * HS * rng.uniform(0.9, 1.1)
            card(part, free_frame(base, d, nrm), Lb, Lb * 0.3, FEATHERS, shape='pointed', rows=(0.0, 0.4, 0.75),
                 lift=lambda t: 0.02 + 0.06 * t)
    for k in range(7):
        across = lerp(-0.6, 0.6, k / 6)
        base = h((FACE_X - 0.95, across * bk['width'] * 0.75,
                  bk['gape'] + bk['depth_u'] * (0.95 - 0.3 * across * across) + 0.05))
        d = hdir((1.0, across * 0.2, -0.3 - 0.3 * across * across))
        nrm = hdir((0.0, across * 0.6, 1.0))
        Lb = 2.6 * HS * (1.0 - 0.2 * abs(across)) * rng.uniform(0.9, 1.1)
        card(part, free_frame(base, d, nrm), Lb, Lb * 0.3, FEATHERS, shape='pointed', rows=(0.0, 0.4, 0.75),
             lift=lambda t: 0.04 + 0.1 * t)
    tip = h((x0 + 1.03 * L, 0.0, gape(1.03)))
    return part, tip


def eye_frame(side):
    hit, n = cast(h((0.4, 0.0, 0.0)), hdir((EYE_DIR[0], side * EYE_DIR[1], EYE_DIR[2])))
    upv = UP - n * UP.dot(n)
    y = upv.normalized()
    x = y.cross(n) if side < 0 else n.cross(y)
    if x.x < 0:
        x = -x
    return hit, n, x, y


def build_eye_and_socket():
    """The pale right eye (a dome with a dark pupil and a heavy upper lid), and the empty left socket: a dark bowl
    with a rim, the dull coal set deep and low in it, so the brow hides it from the front."""
    part = Part('HobEye')
    part.w = {'head': 1.0}
    p, n, x, y = eye_frame(-1)
    rot = Matrix((x, y, n)).transposed()
    # A crow's eye, all iris and no white: a low lens nearly flush with the head (not a ball), pale grey round a big
    # dark pupil, so it reads at a few metres without staring.
    c = p - n * EYE_R * 0.3
    ellipsoid(part, c, (EYE_R, EYE_R * 0.88, EYE_R * 0.55), EYE, rot=rot, segs=10, rings=6, half=True, weights=part.w)
    ellipsoid(part, c + n * EYE_R * 0.49, (EYE_R * 0.47, EYE_R * 0.47, EYE_R * 0.1), FEATHERS, rot=rot, segs=8,
              rings=4, half=True, weights=part.w)
    # The lids: a dark rim over the lens's edge, so only a ring of iris shows round the pupil, the upper lid heavy and
    # low over it (a brooding look).
    ring, radii = [], []
    for k in range(11):
        a = 2.0 * math.pi * k / 10
        up = max(0.0, math.sin(a))
        ring.append(c + (x * math.cos(a) * 1.0 + y * (math.sin(a) * 0.86 - 0.32 * up + 0.06)) * EYE_R
                    + n * EYE_R * (0.16 + 0.24 * up))
        radii.append(EYE_R * (0.2 + 0.12 * up))
    tube(part, ring, radii, FEATHERS, sides=4, caps=(False, False), weights=part.w)
    # The socket.
    sp, sn = SOCKET_CENTER             # on the sculpt: the head's surface before the skin was pressed in
    _, _, sx, sy = eye_frame(1)
    r = 1.05 * HS
    # The lip (its first two rings) lies on the head's surface; the bowl drops below it.
    prof = [(r * 1.18, 0.03), (r * 0.98, 0.12), (r * 0.82, -0.4), (r * 0.55, -0.9), (0.0, -1.12)]
    origin = sp - sn * 0.1
    inner = h((0.4, 0.0, 0.0))
    rings = []
    for k, (rad, dz) in enumerate(prof):
        if rad < 1e-6:
            rings.append([part.vert(origin + sn * dz * HS)])
            continue
        ring = []
        for a in (2.0 * math.pi * j / 10 for j in range(10)):
            p = origin + (sx * math.cos(a) + sy * math.sin(a) * 0.86) * rad
            if k < 2:
                q, nq = toward(inner, p + sn * 0.5)
                p = q + nq * dz * HS
            else:
                p = p + sn * dz * HS
            ring.append(part.vert(p))
        rings.append(ring)
    faces = []
    for i in range(len(rings) - 1):
        a, c2 = rings[i], rings[i + 1]
        for j in range(10):
            j1 = (j + 1) % 10
            if len(c2) == 1:
                faces.append(part.face((a[j], a[j1], c2[0]), FEATHERS))
            else:
                faces.append(part.face((a[j], a[j1], c2[j1], c2[j]), FEATHERS))
    part.orient([f for f in faces if f], lambda c: sn)
    # The coal: small, deep, low and to the back of the bowl, under the brow, so it warms the socket's floor rather
    # than staring out of it.
    coal = origin + sn * (-0.86 * HS) - sy * 0.2 * HS - sx * 0.3 * HS
    ellipsoid(part, coal, (0.24 * HS, 0.2 * HS, 0.16 * HS), EMBER, rot=Matrix((sx, sy, sn)).transposed(), segs=6,
              rings=4, weights=part.w)
    return part, coal


# --- The coat: the ruff, the nape, trousers, flank feathers (few, so they read as plumage, not scales) ---

def shingle(part, inner, direction, flow, L, W, shape='pointed', lift=0.3, root=0.35, rows=(0.0, 0.35, 0.7),
            ash_from=None, end=None, ao_root=0.6, weigh=None):
    """A feather on the skin: rooted where the ray from inner along direction meets the sculpt, lying along flow and
    following the surface, its root tucked in and its tip lifted off by lift (cm)."""
    p0, n0 = cast(inner, direction)
    if p0 is None:
        return
    f = Vector(flow)
    f = (f - n0 * f.dot(n0)).normalized()
    sv = n0.cross(f)

    def to3d(a, y, hh):
        target = p0 + f * (a - root * L) + sv * y + n0 * 0.4
        q, nq = toward(inner, target)
        return q + nq * (hh + 0.03)
    tuck = 0.18 * min(W, 2.0)
    card(part, to3d, L, W, FEATHERS, shape=shape, rows=rows,
         lift=lambda t: lift * smooth(root * 0.7, 1.0, t) ** 1.4 - tuck * (1.0 - smooth(0.0, root, t)), ash_from=ash_from,
         end=end, ao_root=ao_root, weigh=weigh)


def around(el, az):
    """A unit direction el degrees up from the xy plane and az degrees round from +x toward +y."""
    e, a = math.radians(el), math.radians(az)
    return Vector((math.cos(e) * math.cos(a), math.cos(e) * math.sin(a), math.sin(e)))


def build_coat():
    """The plumage over the skin: a raven's shaggy beard and ruff, a heavy brow and crown, the nape and the neck's ruff,
    loose contour feathers breaking the flanks and the back, and the thighs' trousers. Narrow pointed hackles and
    feathers, not round lobes, so it reads as plumage rather than scales; the skin under it stays clean."""
    rng = random.Random(41)
    part = Part('HobCoat')
    hc = h((0.0, 0.0, -0.6))
    # The beard: long, narrow hackles hanging from the throat under the beak, their tips standing off it.
    for k in range(7):
        d = around(-76.0 + rng.uniform(-4, 4), lerp(-36.0, 36.0, k / 6) + rng.uniform(-4, 4))
        part.w = {'head': 0.45, 'neck': 0.55}
        shingle(part, hc, d, (0.35, 0.0, -1.0), 6.0 * rng.uniform(0.88, 1.12), 1.4, lift=rng.uniform(0.9, 1.3),
                root=0.22, rows=(0.0, 0.3, 0.6, 0.85))
    # The ruff: two rows of pointed hackles fanning down over the breast and round the neck's sides.
    for row, (elev, scale, count, span) in enumerate(((-56.0, 1.0, 9, 70.0), (-36.0, 0.8, 7, 82.0))):
        for k in range(count):
            d = around(elev + rng.uniform(-4, 4), lerp(-span, span, k / (count - 1)) + rng.uniform(-5, 5))
            part.w = {'neck': 0.6, 'head': 0.4} if row == 1 else {'neck': 0.55, 'body': 0.45}
            shingle(part, hc, d, (0.15, 0.0, -1.0), 5.4 * scale * rng.uniform(0.85, 1.12), 1.6 * scale,
                    lift=0.85 * scale * rng.uniform(0.7, 1.3), root=0.28, rows=(0.0, 0.3, 0.6, 0.85))
    # The neck's ruff: pointed feathers down its sides (the nape stays smooth), lying down it.
    base, top = b(NECK_BASE), h(HEAD_BASE)
    axis = (top - base).normalized()
    across = Vector((0.0, 1.0, 0.0))
    back = axis.cross(across).normalized()
    for k in range(8):
        phi = math.radians(math.copysign(lerp(52.0, 112.0, (k % 4) / 3), k - 3.5) + rng.uniform(-6, 6))
        d = (back * math.cos(phi) + across * math.sin(phi) - axis * 0.25).normalized()
        part.w = {'neck': 0.75, 'body': 0.25}
        shingle(part, base.lerp(top, 0.45), d, -axis, 3.6 * rng.uniform(0.85, 1.12), 1.6,
                lift=0.45 * rng.uniform(0.75, 1.25), root=0.3)
    # The head: rows of short round feathers lying nearly flat, overlapping like shingles, flowing back from the brow over
    # the crown and the nape and down the cheeks (as the concept's), kept clear of the eye's lids and the socket's rim.
    # Those at the back of the head lift for a brooding ruffle.
    hc0 = h((0.0, 0.0, 0.0))
    eye_at = eye_frame(-1)[0]
    socket_at = SOCKET_CENTER[0]
    rows = [r + ((-1.0, 0.0, -0.3),) for r in HEAD_ROWS] + FACE_ROWS
    for elev, az0, az1, count, L, W, lift, flow in rows:
        flow = Vector(flow).normalized()
        for side, _ in SIDES:
            for k in range(count):
                az = side * lerp(az0, az1, k / (count - 1)) + rng.uniform(-4, 4)
                d = around(elev + rng.uniform(-3, 3), az)
                p0, n0 = cast(hc0, d)
                if p0 is None:
                    continue
                f = (flow - n0 * flow.dot(n0)).normalized()
                length, width = L * HS * rng.uniform(0.9, 1.1), W * HS
                span = [p0 + f * (length * s) for s in (-0.45, 0.0, 0.3, 0.55)]
                if any((q - eye_at).length < 1.25 + width * 0.5 for q in span) or \
                        any((q - socket_at).length < 1.9 + width * 0.5 for q in span):
                    continue
                ruffle = smooth(130.0, 170.0, abs(az))
                part.w = {'head': 1.0} if abs(az) < 150.0 else {'head': 0.7, 'neck': 0.3}
                shingle(part, hc0, d, flow, length, width, shape='round', lift=lift + 0.22 * ruffle, root=0.45,
                        rows=(0.0, 0.5, 0.85), end=0.35, ao_root=0.7)
    # The flanks: feathers tucked under the folded wing's lower edge, and a few loose ones standing off the side and
    # the belly; the thighs' trousers over the top of the leg.
    center = b((0.5, 0.0, 0.0))
    for side, _ in SIDES:
        for k in range(4):
            az = side * lerp(70.0, 120.0, k / 3)
            el = -14.0 - 4.0 * (k % 2)
            part.w = {'body': 1.0}
            shingle(part, center, BODY_ROT @ around(el, az), BODY_ROT @ Vector((-0.6, 0.0, -1.0)), 4.6, 2.8,
                    shape='round', lift=0.15, root=0.45, end=0.4)
        for az, el, L in ((100.0, -38.0, 4.6), (122.0, -24.0, 4.2), (142.0, -46.0, 4.8), (112.0, -64.0, 4.4),
                          (150.0, -76.0, 4.6)):
            part.w = {'body': 1.0}
            shingle(part, center, BODY_ROT @ around(el, side * az), BODY_ROT @ Vector((-0.8, 0.0, -0.6)),
                    L * rng.uniform(0.9, 1.1), 2.1, shape='flight', lift=0.85 * rng.uniform(0.8, 1.2), root=0.3,
                    end=0.35)
        c, _ = THIGH
        inner = b((c[0], side * c[1] * 0.6, c[2] + 0.8))
        for k in range(4):
            a = math.radians(lerp(-50.0, 70.0, k / 3))
            el = math.radians(-40.0)
            d = Vector((math.cos(el) * math.sin(a) * 0.6, side * math.cos(el) * math.cos(a), math.sin(el))).normalized()
            thigh = 'thigh_' + ('l' if side > 0 else 'r')
            # The root rides the body and the tip the thigh, so they drape over a swinging leg instead of fanning out.
            shingle(part, inner, d, (0.0, 0.0, -1.0), 3.0, 2.4, shape='round', lift=0.25, root=0.4, end=0.4,
                    weigh=lambda t, th=thigh: {th: lerp(0.3, 0.85, smooth(0.0, 1.0, t)),
                                               'body': 1.0 - lerp(0.3, 0.85, smooth(0.0, 1.0, t))})
    # The breast below the ruff: a few loose feathers, so it isn't one smooth front.
    for az, el in ((-26.0, -34.0), (2.0, -44.0), (28.0, -36.0)):
        part.w = {'body': 1.0}
        shingle(part, b((2.5, 0.0, 0.0)), BODY_ROT @ around(el, az), BODY_ROT @ Vector((-0.5, 0.0, -1.0)),
                4.2 * rng.uniform(0.9, 1.1), 2.0, shape='flight', lift=0.6 * rng.uniform(0.8, 1.2), root=0.3, end=0.35)
    # The mantle: loose feathers between the shoulders, breaking the back's line.
    for x, az, el in ((2.4, 0.0, 74.0), (1.0, 22.0, 70.0), (1.0, -22.0, 70.0)):
        part.w = {'body': 1.0}
        shingle(part, b((x, 0.0, 0.0)), BODY_ROT @ around(el, az), BODY_ROT @ Vector((-1.0, 0.0, 0.08)),
                4.4 * rng.uniform(0.9, 1.1), 2.2, shape='flight', lift=0.7 * rng.uniform(0.8, 1.2), root=0.3, end=0.35)
    return part


# --- The folded wing (rest) and its carriers ---

def wing_bed(side):
    """The folded wing's bed wrapped round the body's side (as the concept's): bed(u, v, h) in creature space."""
    w = WING
    O = Vector((w['wrist'][0], side * w['wrist'][1], w['wrist'][2]))
    A = Vector((-1.0, -side * w['conv'], w['rise'])).normalized()
    beta = math.radians(w['beta'])
    N = Vector((0.0, side * math.cos(beta), math.sin(beta)))
    N = (N - A * N.dot(A)).normalized()
    Vd = A.cross(N)
    if Vd.z > 0:
        Vd = -Vd
    R = w['R']
    hand = 1.0 if A.cross(Vd).dot(N) > 0 else -1.0

    def local(u, v, hh):
        ang = v / R
        return O + A * u + Vd * ((R + hh) * math.sin(ang)) + N * ((R + hh) * math.cos(ang) - R)
    # The bed stands off the body just enough that the wing's side clears it. Its top edge (the shoulder and the arm)
    # may sink under the neck's and the breast's feathers, as a perched crow's does: were the top to clear the
    # shoulder too, the whole wing would stand out from the body and its front would hook up over the shoulder.
    push = []
    for k in range(40):
        u = k * 0.8
        need = 0.0
        for v in (-1.0, 0.0, 1.2, 2.4, 3.6, 4.8):
            p = local(u, v, 0.0)
            inner = Vector((p.x, 0.0, p.z * 0.3))
            hit, _ = cast(BODY @ inner, BODY_ROT @ (p - inner))
            if hit is None:
                continue
            d_skin = (BODY.inverted() @ hit - inner).length
            sink = 0.75 * max(0.0, 1.2 - v)
            need = max(need, d_skin + w['gap'] - (p - inner).length - sink)
        push.append(need)
    smoothp = [max(push[max(i - 2, 0):i + 3]) for i in range(len(push))]

    def lift(u):
        x = min(max(u / 0.8, 0.0), len(smoothp) - 1.001)
        i = int(x)
        return lerp(smoothp[i], smoothp[i + 1], x - i)

    def bed(u, v, hh):
        return BODY @ local(u, v, hh + lift(u))

    def frame(u, v, ang):
        """The bed's along direction turned ang degrees toward v, and its normal, at (u, v): creature space."""
        e = 0.05
        p = bed(u, v, 0.0)
        du = (bed(u + e, v, 0.0) - p).normalized()
        dv = (bed(u, v + e, 0.0) - p).normalized()
        a = math.radians(ang)
        d = (du * math.cos(a) + dv * math.sin(a)).normalized()
        n = (bed(u, v, 0.1) - p).normalized()
        return d, n
    return bed, hand, frame


# The folded layout in bed coordinates (u back from the wrist, v down from the wing's top line, cm).
JOINTS_FOLDED = dict(S=(1.2, -1.0), E=(9.0, 0.8), W=(0.0, 3.2), T=(7.5, 4.3))


def lerp2(points, a, c, f):
    return (lerp(points[a][0], points[c][0], f), lerp(points[a][1], points[c][1], f))


def folded_feathers():
    """Every flight feather, tertial and covert of a folded wing: (kind, index, u, v, angle, length, width, layer,
    shape, carriers {bone: weight}, open (x, y, angle) in the planform or None)."""
    J = JOINTS_FOLDED
    out = []
    for i in range(9, 0, -1):                        # primaries: P1 at the wrist, P9 at the hand's tip; P9 lowest
        f = (i - 1) / 8.0
        u, v = lerp2(J, 'W', 'T', f)
        x, y = lerp2(PLAN, 'W', 'T', f ** 0.9)
        out.append(('P', i, u + 0.4, v + 0.25, -2.0 - 1.5 * f, PRIMARY_LENGTHS[i - 1], 4.4 if i < 6 else 4.1,
                    0.05 + (9 - i) * 0.035, 'primary' if i >= 6 else 'flight', {'hand': 1.0 - f, 'fan': f},
                    (x, y + 0.2, PRIMARY_ANGLES[i - 1])))
    for i in range(1, 11):                           # secondaries: S1 at the wrist, S10 at the elbow; S10 on top
        f = (i - 1) / 9.0
        u, v = lerp2(J, 'W', 'E', f)
        x, y = lerp2(PLAN, 'W', 'E', f)
        # Lengths bowed, so the open wing's trailing edge curves (with the feathers' round tips, scalloped).
        out.append(('S', i, u + 0.8, v + 0.6, lerp(6.0, 4.0, f),
                    (lerp(13.2, 12.0, f) + math.sin(math.pi * f)) * (1.0 + 0.025 * math.sin(i * 2.7)), 4.0,
                    0.38 + f * 0.26, 'flight', {'hand': 1.0 - f, 'fore': f}, (x, y + 0.5, lerp(10.0, -3.0, f))))
    for k in range(3):                               # tertials, on top by the back
        f = 0.25 + 0.3 * k
        u, v = lerp2(J, 'E', 'S', f * 0.5)
        x, y = lerp2(PLAN, 'E', 'S', 0.15 + 0.25 * k)
        out.append(('T', k, u + 0.6, v + 0.2 - 0.3 * k, 5.0 - 1.5 * k, 11.6 - 0.6 * k, 4.4, 0.68 + 0.07 * k, 'flight',
                    {'fore': 1.0 - 0.6 * f, 'upper': 0.6 * f}, (x, y + 0.3, -8.0 - 6.0 * k)))
    return out


def build_wing(side):
    """A folded wing: the arm's pad with covert rows, the flight feathers stacked under it (two-sided: they show from
    below when open), the greater coverts one per secondary, primary coverts and the underwing coverts against the
    body. Returns the part and the carriers' rest points and open poses."""
    sfx = 'l' if side > 0 else 'r'
    bed, hand, frame = wing_bed(side)
    part = Part(f'HobWing{sfx.upper()}')

    def wb(name):
        return f'wing_{name}_{sfx}'

    def weights(ws):
        return {wb(k): v for k, v in ws.items()}

    def mapped(u0, v0, ang):
        a = math.radians(ang)
        ca, sa = math.cos(a), math.sin(a)

        def to3d(along, y, hh, base_h=0.0):
            u = u0 + along * ca + hand * (-sa) * y
            v = v0 + along * sa + hand * ca * y
            return bed(u, v, base_h + hh)
        return to3d

    feathers = folded_feathers()
    for kind, i, u0, v0, ang, L, W, layer, shape, ws, _ in feathers:
        part.w = weights(ws)
        ash = 0.78 if (kind == 'P' and i in GHOST_PRIMARIES) else None
        to3d = mapped(u0, v0, ang)
        asym = (0.32 if kind == 'P' else 0.18) * hand * side
        shaping = {}
        if kind == 'P':
            # Past the secondaries' tips (u about 15, folded) the primaries' fingers twist a little, curl up and sweep
            # back, so the open hand reads as feathers, not slats. A shorter primary lies higher and turns more at any
            # one place, so the folded stack never crosses.
            shaping = dict(twist=6.0, curl=0.35, curl_from=min(max((15.0 - u0) / L, 0.3), 0.95),
                           bend=math.copysign(0.5, asym))
        card(part, lambda a, y, hh, t=to3d, l=layer: t(a, y, hh, l), L, W, FEATHERS, shape=shape,
             rows=(0.0, 0.32, 0.62, 0.9) if shape == 'primary' else (0.0, 0.6, 0.85, 0.95), asym=asym, two_sided=True,
             ash_from=ash, end=0.3, ao_root=0.55, **shaping)
    # The pad: the arm's bulk, thickest at the wrist, its edge on the feathers' stack; skinned across the shoulder,
    # elbow and wrist so it stretches along the leading edge when the wing opens.
    J = JOINTS_FOLDED
    pad_u0, pad_u1, pad_v0, pad_v1 = -0.4, 10.6, -0.4, 4.2
    uc, vc = (pad_u0 + pad_u1) * 0.5, (pad_v0 + pad_v1) * 0.5
    au, av = (pad_u1 - pad_u0) * 0.5, (pad_v1 - pad_v0) * 0.5
    edge_h = 0.62

    def pad_h(u, v):
        r = (abs((u - uc) / au) ** 3 + abs((v - vc) / av) ** 3) ** (1.0 / 3.0)
        front = lerp(1.0, 0.5, smooth(0.0, 1.0, (u - pad_u0) / (pad_u1 - pad_u0)))
        return edge_h + 0.6 * front * max(0.0, 1.0 - r ** 4) ** 0.45

    def pad_w(u, v):
        ds = {k: math.hypot(u - J[k][0], (v - J[k][1]) * 1.4) for k in ('S', 'E', 'W')}
        raw = {k: 1.0 / (d + 0.6) ** 3 for k, d in ds.items()}
        tot = sum(raw.values())
        names = {'S': 'upper', 'E': 'fore', 'W': 'hand'}
        out = weights({names[k]: v2 / tot for k, v2 in raw.items()})
        root = 0.45 * raw['S'] / tot
        out = {k: v2 * (1.0 - root) for k, v2 in out.items()}
        out['body'] = root
        return out
    nr, nt = 4, 12
    profile = [(math.sin(0.5 * math.pi * i / nr), None) for i in range(1, nr + 1)] + [(1.03, 0.4), (1.04, -0.9)]
    rings = []
    for rho, fixed in profile:
        ring = []
        for j in range(nt):
            th = 2.0 * math.pi * j / nt
            c, sn = math.cos(th), math.sin(th)
            u = uc + au * math.copysign(abs(c) ** (2.0 / 3.0), c) * rho
            v = vc + av * math.copysign(abs(sn) ** (2.0 / 3.0), sn) * rho
            ring.append(part.vert(bed(u, v, pad_h(u, v) if fixed is None else edge_h * fixed), pad_w(u, v)))
        rings.append(ring)
    center = part.vert(bed(uc, vc, pad_h(uc, vc)), pad_w(uc, vc))
    faces = []
    for j in range(nt):
        j1 = (j + 1) % nt
        faces.append(part.face((center, rings[0][j], rings[0][j1]), FEATHERS))
    for i in range(len(rings) - 1):
        for j in range(nt):
            j1 = (j + 1) % nt
            faces.append(part.face((rings[i][j1], rings[i][j], rings[i + 1][j], rings[i + 1][j1]), FEATHERS))
    inv = BODY.inverted()

    def outward(c):
        return BODY_ROT @ ((inv @ c) * Vector((0.0, 1.0, 1.0)) - Vector((0, 0, -1.5)))
    part.orient([f for f in faces if f], outward)
    # Its floor, against the body at rest: open, it is the underside of the arm and of the leading edge, which
    # stretches from the shoulder to the wrist (without it the open arm shows through from below).
    floor_center = part.vert(bed(uc, vc, edge_h * -0.9), pad_w(uc, vc))
    floor = [part.face((floor_center, rings[-1][(j + 1) % nt], rings[-1][j]), FEATHERS) for j in range(nt)]
    part.orient([f for f in floor if f], lambda c: -outward(c))
    # Coverts on the pad. The greater coverts: one over the base of each secondary, lying and moving with it, their
    # round tips a row across the secondaries, so the open wing reads in layers (the arm, the coverts, the flight
    # feathers); each inner one lies over the next outer one, as the secondaries do. The median row above them along
    # the forearm, and the primary coverts at the wrist.
    rng = random.Random(53 + side)
    for kind, i, u0, v0, ang, L, W, layer, shape, ws, _ in feathers:
        if kind != 'S':
            continue
        part.w = weights(ws)

        def to3d(along, y, hh, u0=u0 - 0.6, v0=v0 - 0.2, a=math.radians(ang + 2.0)):
            ca, sa = math.cos(a), math.sin(a)
            u = u0 + along * ca + hand * (-sa) * y
            v = v0 + along * sa + hand * ca * y
            return bed(u, v, pad_h(u, v) + hh)
        card(part, to3d, 6.0 * rng.uniform(0.94, 1.06), 2.6, FEATHERS, shape='round', rows=(0.0, 0.5, 0.85), end=0.25,
             lift=lambda t, k=0.025 * (i - 1): k + 0.04 + 0.05 * smooth(0.4, 1.0, t) - 0.18 * (1.0 - smooth(0.0, 0.3, t)),
             ao_root=0.72)
    for k in range(6):
        f = k / 5
        u0, v0 = lerp2(J, 'W', 'E', f)
        u0 += 0.6 + rng.uniform(-0.15, 0.15)
        v0 -= 1.5
        part.w = weights({'hand': 1.0 - f, 'fore': f})

        def to3d(along, y, hh, u0=u0, v0=v0, a=math.radians(10.0)):
            ca, sa = math.cos(a), math.sin(a)
            u = u0 + along * ca + hand * (-sa) * y
            v = v0 + along * sa + hand * ca * y
            return bed(u, v, pad_h(u, v) + hh)
        card(part, to3d, 3.8 * rng.uniform(0.92, 1.08), 1.9, FEATHERS, shape='round', rows=(0.0, 0.5, 0.85), end=0.25,
             lift=lambda t: 0.16 + 0.06 * smooth(0.3, 1.0, t) - 0.2 * (1.0 - smooth(0.0, 0.35, t)), ao_root=0.72)
    # The leading edge, shoulder to wrist, is the pad's front: opening, it stretches to about three times its folded
    # length, most where the arm's bones part. Its coverts are rigid, each moving with the pad at its root (the pad's own
    # weights there), so they are set where they come out evenly spaced along the OPEN arm: folded, each row is a deck
    # bunched along the pad's front; open, it spreads and covers the arm.
    poses = open_pose(side, bed, frame, feathers)
    carriers = {wb(c): c for c in ('upper', 'fore', 'hand', 'fan')}

    def front_u(v):
        """The pad's leading edge (the front of its outline) at v."""
        x = min(abs((v - vc) / av), 1.0)
        return uc - au * (1.0 - x ** 3) ** (1.0 / 3.0)

    def opened(u, v):
        """Where the pad's top at (u, v) goes when the wing is open (the skinning's blend of its bones)."""
        p = bed(u, v, pad_h(u, v))
        out = Vector()
        for name, w in pad_w(u, v).items():
            out += ((poses[carriers[name]] @ p) if name in carriers else p) * w
        return out

    def spread(n, du):
        """n places along the leading edge (as v from the shoulder to the wrist) evenly spaced on the open arm."""
        vs = [lerp2(J, 'S', 'W', i / 48)[1] for i in range(49)]
        pts = [opened(front_u(v) + du, v) for v in vs]
        acc = [0.0]
        for a, c in zip(pts, pts[1:]):
            acc.append(acc[-1] + (c - a).length)
        out = []
        for k in range(n):
            target = acc[-1] * k / (n - 1)
            i = min(max(j for j in range(len(acc)) if acc[j] <= target + 1e-9), len(acc) - 2)
            out.append(lerp(vs[i], vs[i + 1], (target - acc[i]) / max(acc[i + 1] - acc[i], 1e-9)))
        return out
    # Marginal and lesser coverts over the arm's top: three rows of small round feathers, each row's tips over the next
    # one's roots toward the trailing edge; folded, flush under the median row.
    for row, (du, count, L, W) in enumerate(((0.2, 9, 2.0, 1.6), (1.3, 8, 2.4, 1.8), (2.5, 7, 2.8, 2.0))):
        for k, v0 in enumerate(spread(count, du)):
            u0 = front_u(v0) + du + rng.uniform(-0.1, 0.1)
            part.w = pad_w(u0, v0)

            def to3d(along, y, hh, u0=u0, v0=v0, a=math.radians(8.0), L=L):
                ca, sa = math.cos(a), math.sin(a)
                along -= 0.3 * L
                u = u0 + along * ca + hand * (-sa) * y
                v = v0 + along * sa + hand * ca * y
                return bed(u, v, pad_h(u, v) + hh)
            card(part, to3d, L * rng.uniform(0.92, 1.08), W, FEATHERS, shape='round', rows=(0.0, 0.5, 0.85), end=0.3,
                 lift=lambda t, k=k, row=row: 0.008 * k - 0.03 * row + 0.03 + 0.06 * smooth(0.3, 1.0, t)
                 - 0.16 * (1.0 - smooth(0.0, 0.3, t)), ao_root=0.72)
    # The marginal coverts on the edge itself: rooted on the pad's top and draping forward over its front, and a row
    # along its underside, so the open arm's leading edge is feathered from the front and from below as well.
    for k, v0 in enumerate(spread(11, 0.7)):
        u0 = front_u(v0) + 0.7
        part.w = pad_w(u0, v0)

        def to3d(along, y, hh, u0=u0, v0=v0, a=math.radians(8.0)):
            ca, sa = math.cos(a), math.sin(a)
            along -= 0.5
            u = u0 - along * ca - hand * (-sa) * y
            v = v0 - along * sa - hand * ca * y
            return bed(u, v, pad_h(u, v) + hh - min(1.8 * max(0.0, front_u(v) - u), 1.3))
        card(part, to3d, 2.4 * rng.uniform(0.92, 1.08), 1.8, FEATHERS, shape='round', rows=(0.0, 0.5, 0.85), end=0.3,
             lift=lambda t, k=k: 0.05 + 0.008 * k + 0.04 * smooth(0.3, 1.0, t) - 0.14 * (1.0 - smooth(0.0, 0.25, t)),
             ao_root=0.72)
    for k, v0 in enumerate(spread(9, 0.4)):
        u0 = front_u(v0) + 0.4
        part.w = pad_w(u0, v0)

        def to3d(along, y, hh, u0=u0, v0=v0, a=math.radians(8.0)):
            ca, sa = math.cos(a), math.sin(a)
            u = u0 + along * ca + hand * (-sa) * y
            v = v0 + along * sa + hand * ca * y
            return bed(u, v, edge_h * -0.9 + hh)
        card(part, lambda a, y, hh, t=to3d, k=k: t(a, -y, -hh - 0.04 - 0.015 * k), 3.0 * rng.uniform(0.92, 1.08), 2.2,
             FEATHERS, shape='round', rows=(0.0, 0.5, 0.85), end=0.3, ao_root=0.72)
    for k in range(4):
        f = k / 3.0
        u0, v0 = lerp2(J, 'W', 'T', f * 0.6)
        part.w = weights({'hand': 1.0 - 0.6 * f, 'fan': 0.6 * f})

        def to3d(along, y, hh, u0=u0, v0=v0):
            a = math.radians(4.0)
            ca, sa = math.cos(a), math.sin(a)
            u = u0 + 0.4 + along * ca + hand * (-sa) * y
            v = v0 + 0.5 + along * sa + hand * ca * y
            return bed(u, v, max(pad_h(u, v), 0.55) + hh)
        card(part, to3d, 5.4 - 0.4 * k, 2.4, FEATHERS, shape='round', rows=(0.0, 0.5, 0.85), lift=lambda t: 0.1 * t,
             end=0.25, ao_root=0.72)
    # Underwing coverts: a strip against the body, which faces down when the wing opens.
    for k in range(6):
        f = k / 5.0
        u0, v0 = lerp2(J, 'W', 'E', f)
        part.w = weights({'hand': 1.0 - f, 'fore': f})
        to3d = mapped(u0 + 0.8, v0 + 1.2, 6.0)
        card(part, lambda a, y, hh, t=to3d: t(a, -y, -hh, -0.25), 5.6, 3.0, FEATHERS, shape='round',
             rows=(0.0, 0.5, 0.85), end=0.35, ao_root=0.6)
    # Carriers: their rest points, and the open pose of each (a rotation and translation, creature space).
    rest = {k: bed(J[k][0], J[k][1], 0.3) for k in J}
    return part, rest, poses


def open_frame(side):
    """The open wing's span, chord and normal (creature space), the glide pose, and its shoulder."""
    M = (Matrix.Rotation(math.radians(-side * OPEN['sweep']), 3, 'Z')
         @ Matrix.Rotation(math.radians(side * OPEN['dihedral']), 3, 'X')
         @ Matrix.Rotation(math.radians(-OPEN['aoa']), 3, 'Y'))
    span = M @ Vector((0.0, side, 0.0))
    chord = M @ Vector((-1.0, 0.0, 0.0))
    up = M @ UP
    return span, chord, up, b((3.6, side * 2.2, 2.6))


def frame_matrix(d, n):
    d = d.normalized()
    n = (n - d * n.dot(d)).normalized()
    return Matrix((d, n.cross(d), n)).transposed()


def open_pose(side, bed, frame, feathers):
    """Each carrier's open transform: the rotation taking its defining feathers from folded to open, and the
    translation putting their roots on the open arm. Returns {carrier: Matrix} (creature space, 4x4)."""
    span, chord, up, root = open_frame(side)

    def at(x, y):
        return root + span * x + chord * y

    def direction(theta):
        return span * math.sin(math.radians(theta)) + chord * math.cos(math.radians(theta))
    picks = {'upper': [('T', 1)], 'fore': [('S', 10)], 'hand': [('S', 1), ('P', 1)], 'fan': [('P', 9)]}
    table = {(f[0], f[1]): f for f in feathers}
    poses = {}
    for carrier, keys in picks.items():
        rots, pairs = [], []
        for key in keys:
            kind, i, u0, v0, ang, L, W, layer, shape, ws, (x, y, theta) = table[key]
            d_f, n_f = frame(u0, v0, ang)
            R = frame_matrix(direction(theta), up) @ frame_matrix(d_f, n_f).inverted()
            rots.append(R.to_quaternion())
            pairs.append((bed(u0, v0, layer), at(x, y) + up * layer))
        q = rots[0] if len(rots) == 1 else rots[0].slerp(rots[1], 0.5)
        R = q.to_matrix()
        t = sum((o - R @ f for f, o in pairs), Vector()) / len(pairs)
        poses[carrier] = Matrix.Translation(t) @ R.to_4x4()
    return poses


# --- The tail ---

def build_tail():
    part = Part('HobTail')
    t = TAIL
    rot = BODY_ROT @ Matrix.Rotation(math.radians(t['droop']), 3, 'Y')
    d0 = rot @ Vector((-1.0, 0.0, 0.0))
    n0 = rot @ UP
    left = n0.cross(d0)
    base = b(t['base'])
    for k in sorted(range(12), key=lambda k: -abs(k - 5.5)):
        kk = k - 5.5
        frac = abs(kk) / 5.5
        ang = math.radians(t['spread'] * kk / 11.0)
        d = Matrix.Rotation(-ang, 3, n0) @ d0
        L = t['length'] * (1.0 - t['grad'] * frac ** 1.6)
        pos = base + left * (-kk * 0.28) + n0 * (-0.11 * abs(kk))
        side = 'l' if kk < 0 else 'r'
        fan = smooth(0.1, 1.0, frac)
        part.w = {'tail': 1.0 - fan, f'tail_fan_{side}': fan}
        card(part, free_frame(pos, d, n0), L, t['width'], FEATHERS, shape='flight', rows=(0.0, 0.6, 0.85, 0.95),
             asym=(0.3 * frac if kk < 0 else -0.3 * frac) if frac > 0.2 else 0.0, two_sided=True,
             ash_from=0.8 if k in GHOST_TAIL else None, end=0.3, ao_root=0.55)
    for k in range(3):                     # upper tail coverts over the root, under tail coverts beneath
        kk = k - 1
        d = Matrix.Rotation(math.radians(-kk * 9.0), 3, n0) @ d0
        part.w = {'tail': 0.6, 'body': 0.4}
        card(part, free_frame(base - d0 * 2.2 + n0 * 0.55 + left * (-kk * 0.9), d, n0), 7.4 - abs(kk) * 0.6, 3.8,
             FEATHERS, shape='round', rows=(0.0, 0.5, 0.85), lift=lambda tt: 0.1 * tt, end=0.35, ao_root=0.6)
        dd = (Matrix.Rotation(math.radians(-kk * 11.0), 3, n0) @ (d0 - n0 * 0.25)).normalized()
        card(part, free_frame(base - d0 * 3.0 - n0 * 0.9 + left * (-kk * 1.0), dd, -n0), 6.4 - abs(kk) * 0.5, 4.0,
             FEATHERS, shape='round', rows=(0.0, 0.5, 0.85), end=0.35, ao_root=0.6)
    return part, base, d0, n0


# --- The legs ---

def build_legs():
    part = Part('HobLegs')
    for side, sfx in SIDES:
        foot = Vector((FOOT[0], side * FOOT[1], 0.0))
        heel = Vector((HEEL[0], side * HEEL[1], HEEL[2]))
        top = foot + Vector((0.0, 0.0, 0.55))
        pts = [heel.lerp(top, t) for t in (0.0, 0.5, 1.0)]
        ws = [{f'thigh_{sfx}': 0.5, f'shin_{sfx}': 0.5}, {f'shin_{sfx}': 1.0},
              {f'shin_{sfx}': 0.7, f'toes_{sfx}': 0.15, f'hallux_{sfx}': 0.15}]
        tube(part, pts, [0.5, 0.42, 0.45], FEATHERS, sides=6, weights=ws)
        base = foot + Vector((0.0, 0.0, 0.42))
        for ang, length, claw, rad in TOES:
            back = ang >= 150
            a = math.radians(ang * side if not back else 180.0 - (180.0 - ang) * side)
            d = Vector((math.cos(a), math.sin(a), 0.0))
            bone = {f'hallux_{sfx}': 1.0} if back else {f'toes_{sfx}': 1.0}
            path = [base + d * (length * t) + Vector((0.0, 0.0, -0.08 * t)) for t in (0.0, 1.0)]
            radii = [rad, rad * 0.72]
            w0 = {f'shin_{sfx}': 0.5, **{k2: 0.5 for k2 in bone}}
            tube(part, path, radii, FEATHERS, sides=4, caps=(True, False), weights=[w0, bone])
            end = path[-1]
            cpts = []
            for k in range(3):
                t = k / 2
                bend = d.lerp(-UP, min(1.0, t * 1.2)).normalized()
                cpts.append(end + d * claw * 0.6 * t + bend * claw * 0.4 * t * t)
            tube(part, cpts, [radii[-1] * 0.85, radii[-1] * 0.45, 0.04], FEATHERS, sides=4, caps=(False, True),
                 weights=bone)
    return part


# --- The rig ---

def bone_specs(beak_tip, coal, wings, tail_base, tail_dir, tail_up):
    """(name, head, tail, parent) in creature space."""
    specs = [('body', HIP, b((6.0, 0.0, 0.0)), None),
             ('neck', b(NECK_BASE), h((-1.0, 0.0, -1.2)), 'body'),
             ('head', h((-1.0, 0.0, -1.2)), h((2.5, 0.0, -1.2)), 'neck')]
    x0 = FACE_X - BEAK['inset']
    specs.append(('jaw', h((x0 + 0.2, 0.0, BEAK['gape'])), h((x0 + 5.0, 0.0, BEAK['gape'] - 0.8)), 'head'))
    specs.append(('beak', beak_tip, beak_tip + hdir((1.0, 0.0, -0.1)) * 0.6, 'head'))
    specs.append(('tail', tail_base, tail_base + tail_dir * 8.0, 'body'))
    left = tail_up.cross(tail_dir)
    for side, sfx in SIDES:
        d = (Matrix.Rotation(math.radians(-side * 7.0), 3, tail_up) @ tail_dir).normalized()
        specs.append((f'tail_fan_{sfx}', tail_base + left * side * 0.9, tail_base + left * side * 0.9 + d * 7.0, 'tail'))
    for side, sfx in SIDES:
        rest = wings[sfx][1]
        specs += [(f'wing_upper_{sfx}', rest['S'], rest['E'], 'body'),
                  (f'wing_fore_{sfx}', rest['E'], rest['W'], f'wing_upper_{sfx}'),
                  (f'wing_hand_{sfx}', rest['W'], rest['T'], f'wing_fore_{sfx}'),
                  (f'wing_fan_{sfx}', rest['T'], rest['T'] + (rest['T'] - rest['W']).normalized() * 5.0,
                   f'wing_hand_{sfx}')]
        foot = Vector((FOOT[0], side * FOOT[1], 0.42))
        heel = Vector((HEEL[0], side * HEEL[1], HEEL[2]))
        specs += [(f'thigh_{sfx}', b((-0.2, side * 2.0, -1.5)), heel, 'body'),
                  (f'shin_{sfx}', heel, foot, f'thigh_{sfx}'),
                  (f'toes_{sfx}', foot, foot + Vector((3.5, 0.0, -0.1)), f'shin_{sfx}'),
                  (f'hallux_{sfx}', foot, foot + Vector((-2.6, 0.0, -0.1)), f'shin_{sfx}')]
    return specs


def join(name, objects):
    with bpy.context.temp_override(active_object=objects[0], object=objects[0], selected_objects=objects,
                                   selected_editable_objects=objects):
        bpy.ops.object.join()
    obj = objects[0]
    obj.name = name
    obj.data.name = name
    return obj


def build():
    skin = build_skin()
    beak, beak_tip = build_beak()
    eye, coal = build_eye_and_socket()
    coat = build_coat()
    wings = {}
    for side, sfx in SIDES:
        part, rest, poses = build_wing(side)
        wings[sfx] = (part, rest, poses)
    tail, tail_base, tail_dir, tail_up = build_tail()
    legs = build_legs()
    parts = [skin, beak, eye, coat, wings['l'][0], wings['r'][0], tail, legs]
    objects = [p.finish() for p in parts]
    lt._log('Hob triangles by part: ' + ', '.join(f'{o.name[3:]} {sum(len(q.vertices) - 2 for q in o.data.polygons)}'
                                                  for o in objects))
    hob = join('Hob', objects)
    hob.data.polygons.foreach_set('use_smooth', [True] * len(hob.data.polygons))
    lt.box_uv(hob, 'Polymer')
    rig = lm.armature()
    specs = bone_specs(beak_tip, coal, wings, tail_base, tail_dir, tail_up)
    lm.bones(rig, [(n, lc.to_blender(a), lc.to_blender(c), p) for n, a, c, p in specs])
    hob.parent = rig
    hob.modifiers.new('Armature', 'ARMATURE').object = rig
    # The beak's attach point, for the record (the rig exporter makes no sockets; code attaches to the bone 'beak').
    sock = bpy.data.objects.new('SOCKET_Beak', None)
    bpy.context.scene.collection.objects.link(sock)
    sock.parent = rig
    sock.parent_type = 'BONE'
    sock.parent_bone = 'beak'
    sock.matrix_world = Matrix.Translation(lc.to_blender(beak_tip))
    rig['LODs'] = '50,25'
    rig['LODScreens'] = '0.08,0.03'
    opens = {sfx: wings[sfx][2] for _, sfx in SIDES}
    rests = {sfx: wings[sfx][1] for _, sfx in SIDES}
    return rig, hob, opens, rests, coal


RIG, HOB, OPENS, RESTS, COAL = build()
TRIS = sum(len(p.vertices) - 2 for p in HOB.data.polygons)
lt._log(f'Hob: {TRIS} triangles, {len(RIG.data.bones)} bones, materials {", ".join(m.name for m in HOB.data.materials)}')


# --- Posing by bones (previews; the same blends the game's code makes) ---

def rot_about(pivot, R):
    """A rotation R (3x3) about pivot (creature space) as a 4x4."""
    pivot = Vector(pivot)
    return Matrix.Translation(pivot) @ R.to_4x4() @ Matrix.Translation(-pivot)


def rot(yaw=0.0, pitch=0.0, roll=0.0, frame=None):
    """Degrees, in the creature's axes (yaw about z, pitch about y (nose down), roll about x), or in frame's."""
    R = turn(yaw, pitch, roll).to_3x3()
    return frame @ R @ frame.inverted() if frame is not None else R


def blend(O, head, t):
    """A carrier's transform t of the way from rest (identity) to its open transform O: its head along a line, its
    rotation by slerp."""
    R = Quaternion().slerp(O.to_quaternion(), t).to_matrix()
    head = Vector(head)
    target = head.lerp(O @ head, t)
    return Matrix.Translation(target) @ R.to_4x4() @ Matrix.Translation(-head)


def rest_head(name):
    return lc.TO_BLENDER.inverted() @ RIG.data.bones[name].head_local


def pose(p):
    """Poses the rig: p holds degrees per bone ('head': (yaw, pitch, roll), ...), 'open' (0 folded, 1 open),
    'flap' (dihedral, sweep: degrees round the shoulder), 'flex' (the hand's bend at the wrist)."""
    D = {}
    D['body'] = rot_about(rest_head('body'), rot(*p.get('body', (0, 0, 0))))
    D['neck'] = D['body'] @ rot_about(rest_head('neck'), rot(*p.get('neck', (0, 0, 0))))
    D['head'] = D['neck'] @ rot_about(rest_head('head'), rot(*p.get('head', (0, 0, 0))))
    D['jaw'] = D['head'] @ rot_about(rest_head('jaw'), rot(pitch=p.get('jaw', 0.0)))
    D['beak'] = D['head']
    D['tail'] = D['body'] @ rot_about(rest_head('tail'), rot(*p.get('tail', (0, 0, 0))))
    fan = p.get('fan', 0.0)
    tail_up = BODY_ROT @ UP
    for side, sfx in SIDES:
        D[f'tail_fan_{sfx}'] = D['tail'] @ rot_about(rest_head('tail'), Matrix.Rotation(math.radians(-side * fan), 3,
                                                                                     tail_up))
        dihedral, sweep = p.get('flap', (0.0, 0.0))
        span, chord, up, root = open_frame(side)
        # Positive dihedral raises the wing, positive sweep swings it forward (as open_frame turns them).
        flap = rot_about(root, Matrix.Rotation(math.radians(-side * sweep), 3, 'Z') @
                         Matrix.Rotation(math.radians(side * dihedral), 3, 'X'))
        t = p.get('open', 0.0)
        flex = p.get('flex', 0.0)
        for carrier in ('upper', 'fore', 'hand', 'fan'):
            name = f'wing_{carrier}_{sfx}'
            M = blend(OPENS[sfx][carrier], rest_head(name), t)
            if carrier in ('hand', 'fan') and flex:
                wrist = OPENS[sfx]['hand'] @ rest_head(f'wing_hand_{sfx}') if t > 0.5 else rest_head(f'wing_hand_{sfx}')
                M = rot_about(wrist, Matrix.Rotation(math.radians(-side * flex), 3, chord)) @ M
            D[name] = D['body'] @ flap @ M
        D[f'thigh_{sfx}'] = D['body'] @ rot_about(rest_head(f'thigh_{sfx}'), rot(*p.get('thigh', (0, 0, 0))))
        D[f'shin_{sfx}'] = D[f'thigh_{sfx}'] @ rot_about(rest_head(f'shin_{sfx}'), rot(*p.get('shin', (0, 0, 0))))
        D[f'toes_{sfx}'] = D[f'shin_{sfx}'] @ rot_about(rest_head(f'toes_{sfx}'), rot(*p.get('toes', (0, 0, 0))))
        D[f'hallux_{sfx}'] = D[f'shin_{sfx}'] @ rot_about(rest_head(f'hallux_{sfx}'), rot(*p.get('hallux', (0, 0, 0))))
    T = lc.TO_BLENDER
    Ti = T.inverted()
    posed = {}
    for bone in RIG.data.bones:          # parents come before children
        rest = bone.matrix_local
        world = T @ D[bone.name] @ Ti @ rest
        posed[bone.name] = world
        if bone.parent:
            basis = (posed[bone.parent.name] @ bone.parent.matrix_local.inverted() @ rest).inverted() @ world
        else:
            basis = rest.inverted() @ world
        RIG.pose.bones[bone.name].matrix_basis = basis
    bpy.context.view_layer.update()


def unreal_open_table():
    """The open transforms for the game's code, in Unreal's component space (cm; x forward, y right, z up): where
    each wing bone's head goes, and its rotation from rest (axis, degrees)."""
    F = Matrix.Diagonal((1.0, -1.0, 1.0))
    rows = []
    for _, sfx in SIDES:
        for carrier in ('upper', 'fore', 'hand', 'fan'):
            name = f'wing_{carrier}_{sfx}'
            O = OPENS[sfx][carrier]
            rest = rest_head(name)
            q = (F @ O.to_3x3() @ F).to_quaternion()
            axis, angle = q.to_axis_angle()
            ue = lambda v: (round(v.x, 1), round(-v.y, 1), round(v.z, 1))
            rows.append(f'{name}: rest {ue(rest)} -> open {ue(O @ rest)}, turn {math.degrees(angle):.0f} deg about '
                        f'({axis.x:.2f}, {axis.y:.2f}, {axis.z:.2f})')
    return rows


for row in unreal_open_table():
    lt._log(row)


# --- Previews (with --preview), in Saved/ArtPreviews/RansomsRest/Hob ---

OUT = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Hob')
_scene = []


def keep(obj):
    if obj.name not in bpy.context.scene.collection.objects:
        bpy.context.scene.collection.objects.link(obj)
    _scene.append(obj)
    return obj


def game_shading():
    """The previews draw what the game draws: Eevee culls back faces as Unreal does (the materials are one-sided),
    the ash dithers, and HobEmber glows as its Glow asks (lm.material's nodes show only its color)."""
    for mat in HOB.data.materials:
        mat.use_backface_culling = True
        if mat.name == 'HobEmber':
            bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
            bsdf.inputs['Emission Color'].default_value = lt.hex_color(0xb4461f)
            bsdf.inputs['Emission Strength'].default_value = float(mat.get('Glow', 1.0))
        if mat.name == 'HobFeathers':
            # M_World's RoughnessOffset: added to the roughness map's value, clamped.
            nodes, links = mat.node_tree.nodes, mat.node_tree.links
            bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
            if bsdf.inputs['Roughness'].links:
                source = bsdf.inputs['Roughness'].links[0].from_socket
                rough = nodes.new('ShaderNodeMath')
                rough.operation = 'ADD'
                rough.use_clamp = True
                rough.inputs[1].default_value = ROUGHNESS_OFFSET
                links.new(source, rough.inputs[0])
                links.new(rough.outputs['Value'], bsdf.inputs['Roughness'])


def reset():
    for o in _scene:
        if o.name in bpy.data.objects:
            bpy.data.objects.remove(o, do_unlink=True)
    _scene.clear()
    RIG.location = (0.0, 0.0, 0.0)
    RIG.rotation_euler = (0.0, 0.0, 0.0)
    pose({})


def world():
    w = bpy.data.worlds.get('HobPreview') or bpy.data.worlds.new('HobPreview')
    w.use_nodes = True
    nodes, links = w.node_tree.nodes, w.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    coords = nodes.new('ShaderNodeTexCoord')
    norm = nodes.new('ShaderNodeVectorMath')
    norm.operation = 'NORMALIZE'
    links.new(coords.outputs['Generated'], norm.inputs[0])
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(norm.outputs['Vector'], split.inputs['Vector'])
    ramp = nodes.new('ShaderNodeValToRGB')
    el = ramp.color_ramp.elements
    el[0].position, el[0].color = 0.0, lt.hex_color(0x6e5a40)
    el[1].position, el[1].color = 0.55, lt.hex_color(0x86a2c6)
    for pos, c in ((0.06, 0xf0c890), (0.2, 0xd9c7a8)):
        e = el.new(pos)
        e.color = lt.hex_color(c)
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -0.08
    links.new(split.outputs['Z'], remap.inputs['Value'])
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    links.new(ramp.outputs['Color'], bg.inputs['Color'])
    bg.inputs['Strength'].default_value = 0.7
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    bpy.context.scene.world = w


def light(sun=(-0.45, -0.85, 0.35), rim_at=(0.75, 0.95, 1.55), target=(0.0, 0.0, 1.35)):
    s = keep(bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN')))
    s.data.energy = 5.0
    s.data.color = (1.0, 0.72, 0.47)
    s.data.angle = math.radians(1.0)
    s.rotation_euler = (-Vector(sun).normalized()).to_track_quat('-Z', 'Y').to_euler()
    r = keep(bpy.data.objects.new('_Rim', bpy.data.lights.new('_Rim', 'AREA')))
    r.data.energy = 22.0
    r.data.size = 0.8
    r.data.color = (1.0, 0.85, 0.65)
    r.location = Vector(rim_at)
    r.rotation_euler = (Vector(target) - r.location).to_track_quat('-Z', 'Y').to_euler()


def white(mesh):
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    col.data.foreach_set('color', [1.0] * (4 * len(mesh.loops)))


def ground():
    mesh = bpy.data.meshes.new('_Ground')
    mesh.from_pydata([(-60, -60, 0), (60, -60, 0), (60, 60, 0), (-60, 60, 0)], [], [(0, 1, 2, 3)])
    obj = keep(bpy.data.objects.new('_Ground', mesh))
    lt.assign(obj, lt.material('Hay', name='_Straw', tint=0xe8d2a0))
    lt.box_uv(obj, 'Hay')
    white(mesh)


def post(top, at=(0.0, 0.0), size=0.13, seed=3):
    """A weathered fence post (the shared props kit's) whose front edge lies 2.6 cm ahead of the grip point."""
    import looter_props as lp
    p = lp.block((size, size, top + 0.3), (0.0, 0.0, (top - 0.3) * 0.5), bevel=0.012)
    lp.grain(p, 'Siding', axis=(0.0, 0.0, 1.0), seed=seed)
    lp.place(p, (at[0], at[1] + (size * 0.5 - 0.026), 0.0), (0.0, 0.0, 0.0))
    white(p.data)
    p.name = '_Post'
    return keep(p)


def label(text, at, size=0.04, color=0xe8dcc4):
    curve = bpy.data.curves.new('_Label', 'FONT')
    curve.body = text
    curve.size = size
    curve.align_x = 'CENTER'
    font = os.path.join(lt.REPO, 'Art', 'Fonts', 'Rye-Regular.ttf')
    if os.path.exists(font):
        curve.font = bpy.data.fonts.load(font, check_existing=True)
    obj = keep(bpy.data.objects.new('_Label', curve))
    obj.visible_shadow = False
    obj.location = Vector(at)
    obj.rotation_euler = (math.radians(90.0), 0.0, 0.0)
    mat = bpy.data.materials.get('_Label') or bpy.data.materials.new('_Label')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    em = nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = lt.hex_color(color)
    links.new(em.outputs['Emission'], nodes.new('ShaderNodeOutputMaterial').inputs['Surface'])
    curve.materials.append(mat)


def camera(location, target, lens=70.0, fstop=None, ortho=None):
    cam = keep(bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera')))
    cam.location = Vector(location)
    cam.rotation_euler = (Vector(target) - Vector(location)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens = lens
    cam.data.clip_start = 0.01
    if ortho:
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = ortho
    if fstop:
        cam.data.dof.use_dof = True
        cam.data.dof.aperture_fstop = fstop
        cam.data.dof.focus_distance = (Vector(target) - Vector(location)).length
    bpy.context.scene.camera = cam


def render(name, res=(1400, 900), samples=64):
    """Eevee: a rasterizer like the game's, with real back-face culling and dithered alpha."""
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = samples
    scene.eevee.use_raytracing = True
    scene.eevee.use_shadows = True
    scene.eevee.shadow_ray_count = 2
    scene.eevee.shadow_step_count = 8
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Punchy'
    os.makedirs(OUT, exist_ok=True)
    scene.render.filepath = os.path.join(OUT, name + '.png')
    bpy.ops.render.render(write_still=True)
    lt._log(f'preview: {scene.render.filepath}')


TOP = 1.2           # the posts' tops (m)


def staged(p, cam, target, rim=(0.75, 0.95, 1.55), lift=0.0, lens=70.0, fstop=8.0):
    reset()
    pose(p)
    RIG.location = (0.0, 0.0, TOP + lift)
    world()
    ground()
    post(TOP)
    light(rim_at=rim, target=target)
    camera(cam, target, lens=lens, fstop=fstop)


def previews(which):
    if 'tilt' in which:      # turned to look at you, the head cocked so the pale eye turns up to the camera
        staged(dict(head=(-28.0, 4.0, -16.0), neck=(-8.0, 0.0, -4.0)), (-1.05, -0.95, TOP + 0.22),
               (0.0, 0.0, TOP + 0.17), rim=(0.8, 0.9, 1.6))
        render('Hob_Pose_HeadTilt')
    if 'head' in which:      # the head close, three-quarter, from 1.5 m (a long lens): the eye, the brow, the beard
        staged(dict(head=(-14.0, 4.0, -6.0)), (-0.9, -1.22, TOP + 0.47), (0.0, -0.045, TOP + 0.19), lens=180.0,
               fstop=None)
        render('Hob_Head')
    if 'half' in which:
        staged(dict(open=0.5, head=(15.0, 6.0, -8.0), fan=10.0), (-0.45, -1.35, TOP + 0.75), (0.0, 0.02, TOP + 0.1),
               lens=60.0)
        render('Hob_Pose_WingsHalf')
    if 'open' in which:
        staged(dict(open=1.0, flap=(12.0, 0.0), head=(20.0, 4.0, -6.0), fan=18.0), (0.72, -0.95, TOP + 0.95),
               (0.0, 0.05, TOP + 0.12), lens=45.0)
        render('Hob_Pose_WingsOpen')
    if 'flare' in which:
        flare = dict(open=1.0, flap=(42.0, 16.0), flex=14.0, body=(0.0, -24.0, 0.0), head=(8.0, 34.0, 0.0),
                     neck=(0.0, 10.0, 0.0), tail=(0.0, 22.0, 0.0), fan=42.0, thigh=(0.0, -42.0, 0.0),
                     shin=(0.0, -18.0, 0.0), toes=(0.0, -28.0, 0.0), hallux=(0.0, 24.0, 0.0))
        staged(flare, (-0.34, -1.5, TOP + 0.12), (0.0, 0.0, TOP + 0.27), lift=0.07, lens=50.0)
        render('Hob_Pose_LandingFlare')
    if 'lods' in which:
        lod_sheet()
    if 'concept' in which:
        beside_concept()


def studio_floor():
    w = bpy.data.worlds.get('HobStudio') or bpy.data.worlds.new('HobStudio')
    w.use_nodes = True
    w.node_tree.nodes['Background'].inputs['Color'].default_value = lt.hex_color(0xb9b2a5)
    w.node_tree.nodes['Background'].inputs['Strength'].default_value = 1.0
    bpy.context.scene.world = w
    mesh = bpy.data.meshes.new('_Floor')
    mesh.from_pydata([(-9, -9, 0), (9, -9, 0), (9, 9, 0), (-9, 9, 0)], [], [(0, 1, 2, 3)])
    keep(bpy.data.objects.new('_Floor', mesh))
    mat = bpy.data.materials.get('_Floor') or bpy.data.materials.new('_Floor')
    mat.use_nodes = True
    mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = lt.hex_color(0xa59d90)
    mesh.materials.append(mat)


def lod_sheet():
    """LOD0 and the importer's two reductions, approximated here by Blender's decimation (Unreal makes its own)."""
    reset()
    pose(dict(head=(-20.0, 4.0, 10.0)))
    depsgraph = bpy.context.evaluated_depsgraph_get()
    base = bpy.data.meshes.new_from_object(HOB.evaluated_get(depsgraph))
    for k, (share, x) in enumerate(((1.0, -0.62), (0.5, 0.0), (0.25, 0.62))):
        obj = keep(bpy.data.objects.new(f'_LOD{k}', base.copy()))
        obj.location = (x, 0.0, 0.0)
        if share < 1.0:
            mod = obj.modifiers.new('Decimate', 'DECIMATE')
            mod.ratio = share
        dg = bpy.context.evaluated_depsgraph_get()
        tris = sum(len(p.vertices) - 2 for p in obj.evaluated_get(dg).data.polygons)
        label(f'LOD{k}  {tris} triangles', (x, 0.0, 0.36), size=0.032, color=0x2a2420)
    RIG.location = (0.0, 0.0, -10.0)          # the rig itself out of view
    studio_floor()
    light(sun=(-0.5, -0.8, 0.6), rim_at=(0.6, 1.8, 1.4), target=(0.0, 0.0, 0.15))
    camera((0.0, -2.6, 0.32), (0.0, 0.0, 0.14), lens=50.0)
    render('Hob_LODs', res=(1800, 760), samples=48)


def beside_concept():
    """The game mesh beside the concept it was built from (option C), in the same light and pose."""
    reset()
    src = open(os.path.join(lt.REPO, 'Art', 'Backlog', 'Creatures', 'HobConcepts.py'), encoding='utf-8').read()
    ns = {'__name__': 'hob_concept'}
    exec(compile(src[:src.index('\ndef report():')], 'HobConcepts.py', 'exec'), ns)
    spec = ns['spec_for']('C')
    perch = ns['FlatTop'](-10.4, 2.6, -6.5, 6.5, r=1.0)
    crow = ns['Crow'](spec, dict(perch=perch, head_yaw=0.0), 'Concept').build()
    for o in crow.finish((0.36, 0.0, TOP), -90.0):
        keep(o)
    pose({})
    RIG.location = (-0.36, 0.0, TOP)
    world()
    ground()
    post(TOP, at=(-0.36, 0.0))
    post(TOP, at=(0.36, 0.0), seed=5)
    light(rim_at=(0.4, 1.4, 1.7), target=(0.0, 0.0, TOP + 0.15))
    label('Game mesh', (-0.36, -0.12, TOP - 0.12), size=0.035)
    label('Concept C', (0.36, -0.12, TOP - 0.12), size=0.035)
    # From far off with a long lens, so both are seen from the same side (three-quarter front, from the right).
    camera((-2.5, -4.33, TOP + 0.5), (0.0, 0.0, TOP + 0.13), lens=135.0, fstop=11.0)
    render('Hob_vs_Concept', res=(1800, 900), samples=64)


def check():
    """A quick look at the rest pose from four sides (work only)."""
    reset()
    studio_floor()
    light(sun=(-0.4, -0.8, 0.5), rim_at=(0.0, 1.5, 1.0), target=(0.0, 0.0, 0.15))
    depsgraph = bpy.context.evaluated_depsgraph_get()
    base = bpy.data.meshes.new_from_object(HOB.evaluated_get(depsgraph))
    for k, heading in enumerate((0.0, -90.0, 180.0, 90.0)):
        o = keep(bpy.data.objects.new(f'_View{k}', base))
        o.location = ((k - 1.5) * 0.42, 0.0, 0.0)
        o.rotation_euler = (0.0, 0.0, math.radians(heading))
    RIG.location = (0.0, 0.0, -10.0)
    camera((0.0, -3.0, 0.16), (0.0, 0.0, 0.16), ortho=1.75)
    render('_check', res=(1800, 520), samples=24)


if PREVIEW:
    game_shading()
    which = [a for a in ARGV if a in ('tilt', 'head', 'half', 'open', 'flare', 'lods', 'concept', 'check')] or \
        ['tilt', 'head', 'half', 'open', 'flare', 'lods', 'concept']
    if 'check' in which:
        check()
    previews(which)
