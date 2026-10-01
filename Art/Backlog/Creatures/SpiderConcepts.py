"""Five reworked spider concepts for the user to choose from (2026-10-01), in the stylized-realism style: smooth,
segmented bodies with a painted pattern texture, banded jointed legs, bristles, glossy eyes. Concept art kept in
Art/Backlog (see its README): nothing in the game changes, and SK_Spider (Art/Models/Creatures/Spider.py) stays as it is.

  MeadowWolf     the current brown wolf spider, refined: a light stripe down the carapace, a heart mark and chevrons
                 on the abdomen, ringed legs, short bristles.
  MossBack       a garden-spider shape whose humped abdomen grows moss and grass, a leaf (folium) pattern in bark
                 brown, lichen spots, long olive-banded legs.
  CliffHuntsman  a flat, wide huntsman with very long sideways legs, granite grey with dark speckles and orange
                 lichen, built to lie flat on the island's cliffs.
  EmberTarantula a heavy, hairy tarantula, near black with rust-orange knees and abdomen hair, glowing ember eyes and
                 faintly glowing abdomen stripes.
  SkyJumper      a compact jumping spider with two huge front eyes, cream-banded slate fur, cream "eyebrows" and teal
                 iridescent chelicerae.

Every concept keeps SK_Spider's anatomy, so the chosen one can become the game's rig without new code: a thorax and a
head (the critical spot), two palps, two fangs, an abdomen, and four pairs of two-segment legs (femur, tibia) posed
with the game's two-bone IK (ASpiderCreature measures its legs from the skeleton, so leg lengths may differ). Each is
one mesh here with five materials; its pattern is a painted texture atlas (2048 x 1024, written to
Intermediate/SpiderConcepts at build time), the way the final creature would carry its own texture set.

Spider space is centimeters, x forward, y left, z up, the origin on the ground under the thorax; the finished mesh is
turned and scaled into Blender meters (front to -Y).

    blender -b --factory-startup --python Art/Backlog/Creatures/SpiderConcepts.py -- --preview [MeadowWolf ...]
"""
import math
import os
import random
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_textures as lt
from looter_plants import swatch

OUT_DIR = os.path.join(lt.REPO, 'Intermediate', 'SpiderConcepts')
W, H = 2048, 1024
BODY, EYE, BRISTLE, BRISTLE2, FOLIAGE, ACCENT, FANG = range(7)
FINAL = Matrix.Rotation(-math.pi / 2.0, 4, 'Z') @ Matrix.Scale(0.01, 4)

# Where each part lies in the texture atlas (u0, u1, v0, v1). Body parts map latitude-longitude: U around (0.5 = the
# back), V along (0 = the rear pole, 1 = the front). Limbs: U along (0 = the body end), V around (0.5 = the top).
REGIONS = {
    'abdomen': (0.0, 0.5, 0.5, 1.0), 'carapace': (0.5, 0.75, 0.5, 1.0), 'head': (0.75, 1.0, 0.5, 1.0),
    'femur': (0.0, 0.5, 0.25, 0.5), 'tibia': (0.0, 0.5, 0.0, 0.25), 'palp': (0.5, 0.75, 0.25, 0.5),
    'chel': (0.75, 1.0, 0.25, 0.5), 'coxa': (0.5, 0.75, 0.0, 0.25), 'misc': (0.75, 1.0, 0.0, 0.25),
}


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def solve_two_bone(hip, target, upper, lower, pole):
    """The game's two-bone IK (as Spider.py): the knee keeps both segments their length, bent toward pole."""
    to_target = target - hip
    distance = to_target.length
    direction = to_target / distance
    distance = min(max(distance, abs(upper - lower) + 1.0), (upper + lower) * 0.999)
    along = (upper * upper - lower * lower + distance * distance) / (2.0 * distance)
    height = math.sqrt(max(upper * upper - along * along, 0.0))
    bend = (pole - direction * pole.dot(direction)).normalized()
    return hip + direction * along + bend * height, hip + direction * distance


# --- Geometry ---

class Mesh:
    def __init__(self):
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')

    def face(self, verts, uvs, mat):
        f = self.bm.faces.new(verts)
        f.material_index = mat
        f.smooth = True
        for loop, uv in zip(f.loops, uvs):
            loop[self.uv].uv = uv
        return f


def atlas(region, u, v):
    u0, u1, v0, v1 = REGIONS[region]
    return (u0 + min(max(u, 0.0), 1.0) * (u1 - u0), v0 + min(max(v, 0.0), 1.0) * (v1 - v0))


def ellipsoid(m, center, radii, region, mat=BODY, fwd=(1, 0, 0), up=(0, 0, 1), segs=32, rings=20, deform=None):
    """A smooth ellipsoid, poles along fwd, latitude-longitude UVs on its atlas region. deform(a, s, u) takes the
    unit-sphere point (along, left, up) and returns it changed."""
    fwd = Vector(fwd).normalized()
    up = Vector(up)
    if (up - fwd * up.dot(fwd)).length < 0.3:
        up = Vector((1, 0, 0)) if abs(fwd.x) < 0.7 else Vector((0, 1, 0))
    up = (up - fwd * up.dot(fwd)).normalized()
    left = up.cross(fwd)
    center = Vector(center)

    def point(theta, phi):
        a, r = -math.cos(theta), math.sin(theta)
        s, u = r * math.sin(phi), r * math.cos(phi)
        if deform:
            a, s, u = deform(a, s, u)
        return center + fwd * (a * radii[0]) + left * (s * radii[1]) + up * (u * radii[2])

    bm = m.bm
    rear = bm.verts.new(point(0.0, 0.0))
    front = bm.verts.new(point(math.pi, 0.0))
    ring = [[bm.verts.new(point(math.pi * i / rings, -math.pi + 2.0 * math.pi * j / segs)) for j in range(segs)]
            for i in range(1, rings)]
    for j in range(segs):
        j1 = (j + 1) % segs
        ua, ub = j / segs, (j + 1) / segs
        uv = lambda u, v: atlas(region, u, v)
        m.face((rear, ring[0][j1], ring[0][j]), [uv((ua + ub) / 2, 0.0), uv(ub, 1 / rings), uv(ua, 1 / rings)], mat)
        for i in range(rings - 2):
            va, vb = (i + 1) / rings, (i + 2) / rings
            m.face((ring[i][j], ring[i][j1], ring[i + 1][j1], ring[i + 1][j]),
                   [uv(ua, va), uv(ub, va), uv(ub, vb), uv(ua, vb)], mat)
        m.face((ring[-1][j], ring[-1][j1], front), [uv(ua, 1 - 1 / rings), uv(ub, 1 - 1 / rings),
                                                     uv((ua + ub) / 2, 1.0)], mat)


def tube(m, pts, radii, region, mat=BODY, sides=10, up=(0, 0, 1), plane=None):
    """A closed tube along pts with a radius per point; U along, V around (0.5 = toward up). plane: the normal of
    the plane the tube bends in (a leg's), which keeps the rings from twisting where a segment points along up."""
    pts = [Vector(p) for p in pts]
    up = Vector(up).normalized()
    dist = [0.0]
    for a, b in zip(pts, pts[1:]):
        dist.append(dist[-1] + (b - a).length)
    total = max(dist[-1], 1e-6)
    bm = m.bm
    rings = []
    # In a plane, which way the rings' top faces is settled once from the whole tube's direction: settled per ring,
    # it would flip (and twist the tube) where an arched segment passes the up direction.
    flip = 1.0
    if plane is not None and (Vector(plane).cross((pts[-1] - pts[0]).normalized())).dot(up) < 0:
        flip = -1.0
    for k, p in enumerate(pts):
        t = (pts[min(k + 1, len(pts) - 1)] - pts[max(k - 1, 0)]).normalized()
        if plane is not None:
            ref = Vector(plane).cross(t) * flip
        else:
            ref = up - t * up.dot(t)
            if ref.length < 1e-4:
                ref = Vector((0, 0, 1)) - t * t.z
        ref.normalize()
        side = t.cross(ref)
        rings.append([bm.verts.new(p + (ref * math.cos(a) + side * math.sin(a)) * radii[k])
                      for a in (-math.pi + 2.0 * math.pi * j / sides for j in range(sides))])
    uv = lambda u, v: atlas(region, u, v)
    for k in range(len(pts) - 1):
        ua, ub = dist[k] / total, dist[k + 1] / total
        for j in range(sides):
            j1 = (j + 1) % sides
            va, vb = j / sides, (j + 1) / sides
            m.face((rings[k][j], rings[k + 1][j], rings[k + 1][j1], rings[k][j1]),
                   [uv(ua, va), uv(ub, va), uv(ub, vb), uv(ua, vb)], mat)
    for k, end_u in ((0, 0.0), (len(pts) - 1, 1.0)):
        hub = bm.verts.new(pts[k])
        for j in range(sides):
            j1 = (j + 1) % sides
            m.face((hub, rings[k][j1], rings[k][j]) if k == 0 else (hub, rings[k][j], rings[k][j1]),
                   [uv(end_u, 0.5), uv(end_u, (j + 1) / sides), uv(end_u, j / sides)], mat)


def bristle(m, base, direction, length, radius, mat=BRISTLE):
    """A coarse hair: a thin closed cone."""
    d = Vector(direction).normalized()
    a = d.orthogonal().normalized()
    b = d.cross(a)
    ring = [m.bm.verts.new(base + (a * math.cos(t) + b * math.sin(t)) * radius) for t in (0.0, 2.094, 4.189)]
    tip = m.bm.verts.new(base + d * length)
    uv = (0.5, 0.5)
    for j in range(3):
        m.face((ring[j], ring[(j + 1) % 3], tip), [uv] * 3, mat)
    m.face((ring[2], ring[1], ring[0]), [uv] * 3, mat)


def surface_points(center, radii, fwd, rng, count, where):
    """Random points on an ellipsoid (fwd along its x), kept where where(a, s, u) holds; with outward normals."""
    fwd = Vector(fwd).normalized()
    up = (Vector((0, 0, 1)) - fwd * fwd.z).normalized()
    left = up.cross(fwd)
    out = []
    tries = 0
    while len(out) < count and tries < count * 40:
        tries += 1
        z = rng.uniform(-1, 1)
        t = rng.uniform(0, 2 * math.pi)
        r = math.sqrt(1 - z * z)
        a, s, u = r * math.cos(t), r * math.sin(t), z
        if not where(a, s, u):
            continue
        p = Vector(center) + fwd * (a * radii[0]) + left * (s * radii[1]) + up * (u * radii[2])
        n = (fwd * (a / radii[0]) + left * (s / radii[1]) + up * (u / radii[2])).normalized()
        out.append((p, n, a, s, u))
    return out


def leg(m, c, rng, pair, side):
    """One leg standing on its rest spot: coxa, an arched femur, a knee joint, a bowed tibia tapering to the claw."""
    ride = c['ride']
    ha, ra = math.radians(c['hip_angle'][pair]), math.radians(c['rest_angle'][pair])
    hx, hy, hz = c['hip_radius']
    hip = Vector((math.cos(ha) * hx, side * math.sin(ha) * hy, ride + hz))
    rest = Vector((math.cos(ra) * c['rest_radius'][pair], side * math.sin(ra) * c['rest_radius'][pair], 0.0))
    outward = Vector((hip.x, hip.y, 0.0)).normalized()
    pole = Vector((0.0, 0.0, 1.0)) + outward * c.get('knee_out', 0.4)
    femur_len, tibia_len = c['femur'][pair], c['tibia'][pair]
    r = c['leg_radius'] * c.get('leg_scale', (1, 1, 1, 1))[pair]
    knee, foot = solve_two_bone(hip, rest, femur_len, tibia_len, pole)
    plane = (knee - hip).cross(foot - knee).normalized()
    ellipsoid(m, hip, (r * 1.25, r * 1.15, r * 1.1), 'coxa', segs=12, rings=8, fwd=(knee - hip))
    fdir = (knee - hip).normalized()
    fbend = (pole - fdir * pole.dot(fdir)).normalized()
    steps = [i / 7 for i in range(8)]
    fpts = [hip.lerp(knee, t) + fbend * (math.sin(math.pi * t) * femur_len * 0.05) for t in steps]
    frad = [r * (1.0 + 0.14 * math.sin(math.pi * min(t * 1.5, 1.0)) - 0.2 * t) for t in steps]
    tube(m, fpts, frad, 'femur', sides=12, up=pole, plane=plane)
    ellipsoid(m, knee, (r * 0.95, r * 0.92, r * 0.92), 'coxa', segs=12, rings=8, fwd=fdir)
    tdir = (foot - knee).normalized()
    tbend = (pole - tdir * pole.dot(tdir)).normalized()
    tsteps = [i / 10 for i in range(11)]
    tpts = [knee.lerp(foot, t) + tbend * (math.sin(math.pi * t) * tibia_len * 0.06) for t in tsteps]
    trad = [r * (0.84 - 0.62 * t ** 1.1) + (0.05 * r if t > 0.98 else 0.0) for t in tsteps]
    tube(m, tpts, trad, 'tibia', sides=10, up=pole, plane=plane)
    # Bristles lie along the leg toward its tip, on the top and sides.
    for points, rads, n_hair in ((fpts, frad, c['leg_hairs'][0]), (tpts, trad, c['leg_hairs'][1])):
        flip = -1.0 if plane.cross((points[-1] - points[0]).normalized()).dot(pole) < 0 else 1.0
        for _ in range(n_hair):
            t = rng.uniform(0.05, 0.92)
            k = min(int(t * (len(points) - 1)), len(points) - 2)
            f = t * (len(points) - 1) - k
            p = points[k].lerp(points[k + 1], f)
            tangent = (points[k + 1] - points[k]).normalized()
            ref = plane.cross(tangent).normalized() * flip
            sidev = tangent.cross(ref)
            a = math.radians(rng.uniform(-115, 115))
            normal = ref * math.cos(a) + sidev * math.sin(a)
            rad = rads[k] * (1 - f) + rads[k + 1] * f
            mat = BRISTLE2 if c.get('bristle2_legs') and rng.random() < c['bristle2_legs'] else BRISTLE
            bristle(m, p + normal * rad * 0.8, normal * 0.55 + tangent * 0.85,
                    c['hair_len'] * rng.uniform(0.6, 1.2), c['hair_radius'], mat)
    return hip, knee, foot


def build(c, name):
    """Builds concept c as one mesh object called name, in Blender meters; returns it."""
    rng = random.Random(c['seed'])
    m = Mesh()
    ride = c['ride']
    car = c['carapace']
    ellipsoid(m, (car['x'], 0, ride + car['z']), car['radii'], 'carapace', fwd=car.get('fwd', (1, 0, 0)),
              deform=car.get('deform'))
    hd = c['head']
    ellipsoid(m, (hd['x'], 0, ride + hd['z']), hd['radii'], 'head', fwd=hd.get('fwd', (1, 0, 0)), deform=hd.get('deform'))
    ab = c['abdomen']
    ab_center = Vector((ab['x'], 0, ride + ab['z']))
    ellipsoid(m, ab_center, ab['radii'], 'abdomen', fwd=ab['fwd'], segs=40, rings=26, deform=ab.get('deform'))
    # The pedicel ties the abdomen to the thorax; spinnerets at the back.
    ab_fwd = Vector(ab['fwd']).normalized()
    front_of_abdomen = ab_center + ab_fwd * ab['radii'][0] * 0.9
    tube(m, [Vector((car['x'] - car['radii'][0] * 0.85, 0, ride + car['z'] * 0.6)), front_of_abdomen],
         [6.0, 6.0], 'misc', sides=8)
    rear = ab_center - ab_fwd * ab['radii'][0]
    for side in (-1, 1):
        tube(m, [rear + ab_fwd * 4 + Vector((0, side * 3, -2)), rear - ab_fwd * 6 + Vector((0, side * 4, -5))],
             [3.0, 1.2], 'misc', sides=6)
    # Eyes, chelicerae with their fangs, palps.
    for x, y, z, r in c['eyes']:
        for side in ((-1, 1) if y else (1,)):
            ellipsoid(m, (x, side * y, ride + z), (r, r, r), 'misc', mat=EYE, segs=14, rings=10)
    ch = c['chel']
    chel_mat = ACCENT if ch.get('accent') else BODY
    for side in (-1, 1):
        base = Vector((ch['x'], side * ch['y'], ride + ch['z']))
        ellipsoid(m, base, ch['radii'], 'chel', mat=chel_mat, fwd=(0.35, 0, -1), up=(1, 0, 0.3), segs=14, rings=10)
        tip0 = base + Vector((ch['radii'][1] * 0.3, -side * 2.0, -ch['radii'][0] * 0.85))
        fang = [tip0, tip0 + Vector((4, -side * 3, -6)) * ch.get('fang', 1.0), tip0 + Vector((5, -side * 7, -9)) * ch.get('fang', 1.0)]
        tube(m, fang, [ch['radii'][1] * 0.35, ch['radii'][1] * 0.22, 0.4], 'misc', mat=FANG, sides=8)
    pp = c['palp']
    for side in (-1, 1):
        start = Vector((pp['x'], side * pp['y'], ride + pp['z']))
        elbow = start + Vector((pp['len'] * 0.55, side * 4, -pp['len'] * 0.35))
        tip = elbow + Vector((pp['len'] * 0.3, side * 1.0, -pp['len'] * 0.55))
        tube(m, [start, start.lerp(elbow, 0.5), elbow], [pp['r'], pp['r'] * 0.95, pp['r'] * 0.85], 'palp', sides=10)
        tube(m, [elbow, elbow.lerp(tip, 0.5), tip], [pp['r'] * 0.85, pp['r'] * 0.9, pp['r'] * pp.get('club', 0.7)],
             'palp', sides=10)
    # Legs.
    for pair in range(4):
        for side in (-1, 1):
            leg(m, c, rng, pair, side)
    # Body bristles: they lie back along the body.
    for region_key, count, where in c['body_hairs']:
        part = {'carapace': car, 'abdomen': ab, 'head': hd}[region_key]
        center = Vector((part['x'], 0, ride + part['z']))
        fwd = Vector(part.get('fwd', (1, 0, 0)))
        for p, n, a, s, u in surface_points(center, part['radii'], fwd, rng, count, where):
            mat = BRISTLE2 if c.get('bristle2_body', {}).get(region_key, 0) > rng.random() else BRISTLE
            bristle(m, p - n * 0.5, n * 0.45 - fwd.normalized() * 0.9, c['hair_len'] * rng.uniform(0.7, 1.3),
                    c['hair_radius'], mat)
    if c.get('extras'):
        c['extras'](m, c, rng)
    mesh = bpy.data.meshes.new(name)
    m.bm.to_mesh(mesh)
    m.bm.free()
    mesh.transform(FINAL)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for mat in materials(c, name):
        mesh.materials.append(mat)
    for p in mesh.polygons:
        p.use_smooth = True
    obj.data.set_sharp_from_angle(angle=math.radians(70.0))
    return obj


# --- Materials ---

def solid(name, color, rough, metal=0.0, coat=0.0, emit=None, emit_strength=0.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = lt.hex_color(color)
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['Metallic'].default_value = metal
    bsdf.inputs['Coat Weight'].default_value = coat
    if emit is not None:
        bsdf.inputs['Emission Color'].default_value = lt.hex_color(emit)
        bsdf.inputs['Emission Strength'].default_value = emit_strength
    return mat


def body_material(c, name):
    """The painted atlas (color, roughness, height, glow), fine fur bumps, and an optional detail texture set laid
    over it by box projection (the huntsman's granite)."""
    paths = paint(c, name)
    mat = bpy.data.materials.new(name + 'Body')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes['Principled BSDF']

    def image(path, colorspace):
        node = nodes.new('ShaderNodeTexImage')
        node.image = bpy.data.images.load(path, check_existing=False)
        node.image.colorspace_settings.name = colorspace
        node.interpolation = 'Cubic'
        return node
    bc, rough, height, glow = (image(paths['BC'], 'sRGB'), image(paths['R'], 'Non-Color'),
                               image(paths['H'], 'Non-Color'), image(paths['E'], 'Non-Color'))
    color = bc.outputs['Color']
    coords = nodes.new('ShaderNodeTexCoord')
    fur = nodes.new('ShaderNodeTexNoise')
    fur.inputs['Scale'].default_value = c.get('fur_scale', 140.0)
    fur.inputs['Detail'].default_value = 8.0
    fur.inputs['Roughness'].default_value = 0.65
    links.new(coords.outputs['Object'], fur.inputs['Vector'])
    shade = nodes.new('ShaderNodeMapRange')
    shade.inputs['To Min'].default_value = 0.86
    shade.inputs['To Max'].default_value = 1.1
    links.new(fur.outputs['Fac'], shade.inputs['Value'])
    mul = nodes.new('ShaderNodeMix')
    mul.data_type = 'RGBA'
    mul.blend_type = 'MULTIPLY'
    mul.inputs['Factor'].default_value = 1.0
    links.new(color, mul.inputs['A'])
    links.new(shade.outputs['Result'], mul.inputs['B'])
    color = mul.outputs['Result']
    if c.get('detail_set'):
        detail = image(lt.texture_path(c['detail_set'], 'BC'), 'sRGB')
        detail.projection = 'BOX'
        detail.projection_blend = 0.3
        scaled = nodes.new('ShaderNodeMapping')
        scaled.inputs['Scale'].default_value = (c.get('detail_scale', 0.6),) * 3
        links.new(coords.outputs['Object'], scaled.inputs['Vector'])
        links.new(scaled.outputs['Vector'], detail.inputs['Vector'])
        over = nodes.new('ShaderNodeMix')
        over.data_type = 'RGBA'
        over.blend_type = 'OVERLAY'
        over.inputs['Factor'].default_value = c.get('detail_amount', 0.6)
        links.new(color, over.inputs['A'])
        links.new(detail.outputs['Color'], over.inputs['B'])
        color = over.outputs['Result']
    links.new(color, bsdf.inputs['Base Color'])
    links.new(rough.outputs['Color'], bsdf.inputs['Roughness'])
    bump = nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.5
    bump.inputs['Distance'].default_value = 0.02
    links.new(height.outputs['Color'], bump.inputs['Height'])
    fine = nodes.new('ShaderNodeBump')
    fine.inputs['Strength'].default_value = c.get('fur_bump', 0.25)
    fine.inputs['Distance'].default_value = 0.002
    links.new(fur.outputs['Fac'], fine.inputs['Height'])
    links.new(bump.outputs['Normal'], fine.inputs['Normal'])
    links.new(fine.outputs['Normal'], bsdf.inputs['Normal'])
    bsdf.inputs['Coat Weight'].default_value = c.get('coat', 0.15)
    bsdf.inputs['Coat Roughness'].default_value = 0.3
    if c.get('glow_color'):
        bsdf.inputs['Emission Color'].default_value = lt.hex_color(c['glow_color'])
        links.new(glow.outputs['Color'], bsdf.inputs['Emission Strength'])
    return mat


def moss_material(name, color):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes['Principled BSDF']
    coords = nodes.new('ShaderNodeTexCoord')
    lumps = nodes.new('ShaderNodeTexNoise')
    lumps.inputs['Scale'].default_value = 35.0
    lumps.inputs['Detail'].default_value = 10.0
    lumps.inputs['Roughness'].default_value = 0.7
    links.new(coords.outputs['Object'], lumps.inputs['Vector'])
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].position, ramp.color_ramp.elements[0].color = 0.35, lt.hex_color(0x3a4f1c)
    ramp.color_ramp.elements[1].position, ramp.color_ramp.elements[1].color = 0.7, lt.hex_color(color)
    ramp.color_ramp.elements.new(0.85).color = lt.hex_color(0x9fae4a)
    links.new(lumps.outputs['Fac'], ramp.inputs['Fac'])
    links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
    bump = nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.9
    bump.inputs['Distance'].default_value = 0.01
    links.new(lumps.outputs['Fac'], bump.inputs['Height'])
    links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    bsdf.inputs['Roughness'].default_value = 0.95
    bsdf.inputs['Sheen Weight'].default_value = 0.4
    return mat


def materials(c, name):
    eye_emit = c.get('eye_glow')
    return [
        body_material(c, name),
        solid(name + 'Eye', c.get('eye_color', 0x070606), 0.04, coat=1.0, emit=eye_emit,
              emit_strength=c.get('eye_glow_strength', 0.0)),
        solid(name + 'Bristle', c['bristle'], 0.6),
        solid(name + 'Bristle2', c.get('bristle2', c['bristle']), 0.6),
        lt.material('FoliagePalette'),
        moss_material(name + 'Moss', c['accent']) if c.get('accent_moss') else
        solid(name + 'Accent', c.get('accent', 0x2fb7b0), c.get('accent_rough', 0.22), metal=c.get('accent_metal', 0.65),
              coat=c.get('accent_coat', 1.0)),
        solid(name + 'Fang', c.get('fang_color', 0x1a120c), 0.22, coat=0.8),
    ]


# --- Painting the atlas ---

def region_grid(region):
    u0, u1, v0, v1 = REGIONS[region]
    c0, c1 = int(u0 * W), int(u1 * W)
    r0, r1 = int((1 - v1) * H), int((1 - v0) * H)
    cols = (np.arange(c0, c1) + 0.5) / W
    rows = 1 - (np.arange(r0, r1) + 0.5) / H
    uu, vv = np.meshgrid((cols - u0) / (u1 - u0), (rows - v0) / (v1 - v0))
    return (slice(r0, r1), slice(c0, c1)), uu.astype(np.float32), vv.astype(np.float32)


def rgb(hexes):
    return lt.rgb(hexes)


def mix(a, b, t):
    return lt.mix(a, b, t)


def paint(c, name):
    color = np.zeros((H, W, 3), np.float32)
    rough = np.full((H, W), 0.65, np.float32)
    height = np.full((H, W), 0.5, np.float32)
    glow = np.zeros((H, W), np.float32)
    for k, region in enumerate(REGIONS):
        sl, U, V = region_grid(region)
        col, r, h, e = c['painter'](region, U, V, k + c['seed'])
        color[sl], rough[sl], height[sl], glow[sl] = col, r, h, e
    os.makedirs(OUT_DIR, exist_ok=True)
    paths = {kind: os.path.join(OUT_DIR, f'T_{name}_{kind}.png') for kind in ('BC', 'R', 'H', 'E')}
    lt.write_png(paths['BC'], lt.to8(np.clip(color, 0, 1)))
    lt.write_png(paths['R'], lt.to8(np.clip(rough, 0, 1)))
    lt.write_png(paths['H'], lt.to8(np.clip(height, 0, 1)))
    lt.write_png(paths['E'], lt.to8(np.clip(glow, 0, 1)))
    return paths


def body_axes(U, V):
    """For body regions: d = 1 on the back, -1 on the belly; s = -1..1 side to side; V along (0 rear, 1 front)."""
    ang = 2.0 * np.pi * (U - 0.5)
    return np.cos(ang), np.sin(ang)


def limb_axes(U, V):
    """For limbs: along = U, top = 1 on the upper side, -1 underneath."""
    return U, np.cos(2.0 * np.pi * (V - 0.5))


def bumps(shape, seed, spots):
    return lt.noise(shape, seed, spots, spots)


def bands(x, centers, width):
    out = np.zeros_like(x)
    for c0 in centers:
        out = np.maximum(out, 1.0 - lt.smooth(width * 0.5, width, np.abs(x - c0)))
    return out


def finish_color(base, seed, amount=0.06):
    n = lt.noise(base.shape[:2], seed, 3.0, 3.0)
    return base * (1.0 + amount * n)[..., None]


# Each painter returns color (h, w, 3), roughness, height (0.5 = level) and glow (0..1) for a region.

def paint_meadow_wolf(region, U, V, seed):
    shape = U.shape
    brown, dark, light, belly = rgb(0x6b4a2e), rgb(0x2c1c0f), rgb(0xb89466), rgb(0xa08055)
    n = lt.noise(shape, seed, 12.0, 12.0, octaves=3)
    rough = np.full(shape, 0.68, np.float32)
    height = 0.5 + 0.06 * n
    glow = np.zeros(shape, np.float32)
    if region in ('abdomen', 'carapace', 'head'):
        d, s = body_axes(U, V)
        col = mix(brown, light, np.clip(0.25 + 0.2 * n, 0, 1))
        col = mix(col, belly, lt.smooth(-0.1, -0.5, d))
        if region == 'abdomen':
            field = lt.smooth(0.62, 0.42, np.abs(s) + 0.05 * n) * lt.smooth(-0.15, 0.25, d) * lt.smooth(0.02, 0.15, V)
            col = mix(col, dark, field * 0.8)
            heart = (np.abs(s) < 0.11 * np.clip(1 - ((V - 0.78) / 0.2) ** 2, 0, 1)) & (d > 0)
            col = mix(col, light * 0.95, lt.blur(heart.astype(np.float32), 2.0))
            for k in range(4):
                vk = 0.5 - k * 0.11
                line = 1 - lt.smooth(0.012, 0.03, np.abs(V - (vk - 0.22 * np.abs(s))))
                col = mix(col, light, line * (np.abs(s) < 0.42) * (d > 0) * 0.85)
            col = mix(col, light * 0.9, lt.specks(shape, seed + 3, 0.04, 1.5) * 0.6 * (d > -0.2))
        else:
            stripe = lt.smooth(0.15, 0.08, np.abs(s)) * (d > 0)
            lateral = lt.smooth(0.12, 0.2, np.abs(s)) * lt.smooth(0.72, 0.55, np.abs(s)) * (d > 0)
            col = mix(col, dark, lateral * 0.85)
            col = mix(col, light, stripe * (0.9 if region == 'carapace' else 0.6))
            margin = lt.smooth(0.75, 0.92, np.abs(s)) * (d > -0.3)
            col = mix(col, light * 0.95, margin * 0.7)
        return finish_color(col, seed), rough, height, glow
    along, top = limb_axes(U, V)
    col = mix(brown, light, np.clip(0.3 + 0.2 * n, 0, 1))
    col = mix(col, belly * 1.05, lt.smooth(0.0, -0.6, top))
    if region == 'femur':
        col = mix(col, dark, bands(along, (0.42, 0.78), 0.07) * 0.62)
    elif region == 'tibia':
        col = mix(col, dark, bands(along, (0.18, 0.46, 0.72, 0.94), 0.05) * 0.62)
    elif region in ('palp', 'chel'):
        col = mix(col, dark, 0.45)
    else:
        col = mix(col, dark, 0.3)
    return finish_color(col, seed), rough, height, glow


def paint_moss_back(region, U, V, seed):
    shape = U.shape
    olive, bark, moss, lichen, dark = rgb(0x4d4a2a), rgb(0x4a3420), rgb(0x5e7a2c), rgb(0xb3b58a), rgb(0x241c12)
    n = lt.noise(shape, seed, 10.0, 10.0, octaves=3)
    rough = np.full(shape, 0.75, np.float32)
    height = 0.5 + 0.05 * n
    glow = np.zeros(shape, np.float32)
    if region == 'abdomen':
        d, s = body_axes(U, V)
        patches = lt.smooth(-0.2, 0.6, lt.noise(shape, seed + 1, 18.0, 18.0, octaves=3))
        col = mix(bark * 1.2, moss * 0.85, patches * 0.7)
        leaf_w = 0.32 * np.sin(np.pi * np.clip(V, 0, 1)) * (1 + 0.15 * np.sin(V * 40.0))
        folium = lt.smooth(0.03, 0.0, np.abs(s) - leaf_w) * (d > 0)
        edge = (1 - lt.smooth(0.0, 0.03, np.abs(np.abs(s) - leaf_w))) * (d > 0)
        col = mix(col, bark, folium * 0.85)
        col = mix(col, dark, edge * 0.8)
        col = mix(col, lichen, lt.specks(shape, seed + 4, 0.05, 2.5) * 0.9)
        col = mix(col, olive * 0.8, lt.smooth(-0.2, -0.6, d))
        height = height + 0.25 * patches * (d > 0)
        rough = rough + 0.1 * patches
        return finish_color(col, seed, 0.1), rough, height, glow
    if region in ('carapace', 'head'):
        d, s = body_axes(U, V)
        col = mix(olive, bark, np.clip(0.4 + 0.3 * n, 0, 1))
        col = mix(col, dark, lt.smooth(0.25, 0.05, np.abs(s)) * (d > 0) * 0.7)
        col = mix(col, lichen * 0.85, lt.smooth(0.7, 0.9, np.abs(s)) * 0.8)
        return finish_color(col, seed), rough, height, glow
    along, top = limb_axes(U, V)
    col = mix(olive * 1.15, lichen * 0.7, np.clip(0.2 + 0.2 * n, 0, 1))
    if region == 'femur':
        col = mix(col, dark, bands(along, (0.1, 0.5, 0.88), 0.08) * 0.6)
    elif region == 'tibia':
        col = mix(col, dark, bands(along, (0.22, 0.5, 0.78, 0.97), 0.06) * 0.6)
    elif region in ('palp', 'chel', 'coxa'):
        col = mix(col, bark, 0.6)
    return finish_color(col, seed), rough, height, glow


def paint_cliff_huntsman(region, U, V, seed):
    shape = U.shape
    grey, dark, pale, lichen = rgb(0x8c897f), rgb(0x3b3934), rgb(0xc6c0ae), rgb(0xc77b30)
    n = lt.noise(shape, seed, 8.0, 8.0, octaves=4)
    rough = np.full(shape, 0.7, np.float32)
    height = 0.5 + 0.08 * n
    glow = np.zeros(shape, np.float32)
    col = mix(grey, pale, np.clip(0.3 + 0.35 * n, 0, 1))
    col = mix(col, dark, lt.specks(shape, seed + 2, 0.12, 1.2) * 0.8)
    col = mix(col, dark, lt.smooth(0.4, 1.2, lt.noise(shape, seed + 5, 22.0, 22.0)) * 0.5)
    lich = lt.smooth(0.9, 1.3, lt.noise(shape, seed + 7, 14.0, 14.0, octaves=2))
    if region in ('abdomen', 'carapace', 'head'):
        d, s = body_axes(U, V)
        col = mix(col, lichen, lich * (d > 0) * 0.85)
        col = mix(col, pale, lt.smooth(-0.2, -0.6, d) * 0.7)
        if region == 'carapace':
            col = mix(col, pale * 1.05, lt.smooth(0.06, 0.02, np.abs(s)) * (d > 0) * 0.8)
        height = height + 0.15 * lich * (d > 0)
        return finish_color(col, seed), rough, height, glow
    along, top = limb_axes(U, V)
    col = mix(col, pale, lt.smooth(0.0, -0.7, top) * 0.6)
    if region in ('femur', 'tibia'):
        col = mix(col, dark, bands(along, (0.3, 0.62, 0.9), 0.04) * 0.55)
        col = mix(col, lichen, lich * (top > 0.2) * 0.6)
    return finish_color(col, seed), rough, height, glow


def paint_ember_tarantula(region, U, V, seed):
    shape = U.shape
    black, brown, rust, ember = rgb(0x1f1712), rgb(0x3a2a1e), rgb(0xc4561e), rgb(0xff8a32)
    n = lt.noise(shape, seed, 6.0, 6.0, octaves=3)
    rough = np.full(shape, 0.8, np.float32)
    height = 0.5 + 0.05 * n
    glow = np.zeros(shape, np.float32)
    if region == 'abdomen':
        d, s = body_axes(U, V)
        col = mix(black, brown, np.clip(0.4 + 0.3 * n, 0, 1))
        hair = lt.smooth(-0.3, 0.8, lt.noise(shape, seed + 1, 1.2, 9.0, octaves=2))
        col = mix(col, rust * 0.8, hair * 0.55 * (d > -0.3))
        stripes = np.zeros(shape, np.float32)
        for k in range(4):
            vk = 0.25 + k * 0.15
            stripes = np.maximum(stripes, 1 - lt.smooth(0.008, 0.022, np.abs(V - (vk - 0.12 * np.abs(s) ** 1.5))))
        stripes *= (np.abs(s) < 0.55) * lt.smooth(0.0, 0.3, d)
        col = mix(col, ember, stripes * 0.9)
        glow = stripes * 0.9
        return finish_color(col, seed), rough, height, glow
    if region in ('carapace', 'head'):
        d, s = body_axes(U, V)
        col = mix(black, brown, np.clip(0.35 + 0.3 * n, 0, 1))
        rays = np.abs(np.sin(np.arctan2(s, V - 0.45) * 6.0))
        col = mix(col, black * 0.7, (rays > 0.85) * (d > 0) * 0.6)
        col = mix(col, rust * 0.7, lt.smooth(0.8, 0.95, np.abs(s)) * 0.6)
        return finish_color(col, seed), rough, height, glow
    along, top = limb_axes(U, V)
    col = mix(black, brown, np.clip(0.35 + 0.3 * n, 0, 1))
    if region == 'femur':
        knee = bands(along, (0.9,), 0.12)
        col = mix(col, rust, knee)
        glow = knee * 0.25
    elif region == 'tibia':
        rings = bands(along, (0.05, 0.42), 0.07)
        col = mix(col, rust, rings)
        glow = rings * 0.2
    return finish_color(col, seed), rough, height, glow


def paint_sky_jumper(region, U, V, seed):
    shape = U.shape
    slate, deep, cream, teal = rgb(0x323843), rgb(0x1d2129), rgb(0xe8dfc8), rgb(0x2fa8a2)
    n = lt.noise(shape, seed, 5.0, 5.0, octaves=3)
    rough = np.full(shape, 0.72, np.float32)
    height = 0.5 + 0.05 * n
    glow = np.zeros(shape, np.float32)
    if region == 'abdomen':
        d, s = body_axes(U, V)
        col = mix(deep, slate, np.clip(0.45 + 0.3 * n, 0, 1))
        band = (1 - lt.smooth(0.02, 0.05, np.abs(V - 0.86))) * (d > -0.1)
        spots = np.zeros(shape, np.float32)
        for vv, ss, rr in ((0.6, 0.22, 0.045), (0.42, 0.26, 0.04), (0.27, 0.2, 0.035)):
            dist = np.sqrt(((V - vv) / rr) ** 2 + ((np.abs(s) - ss) / (rr * 1.6)) ** 2)
            spots = np.maximum(spots, 1 - lt.smooth(0.8, 1.0, dist))
        chev = (1 - lt.smooth(0.01, 0.025, np.abs(V - (0.16 - 0.15 * np.abs(s))))) * (np.abs(s) < 0.35)
        col = mix(col, cream, np.maximum(np.maximum(band, spots), chev) * (d > 0) * 0.95)
        col = mix(col, teal * 0.6, lt.smooth(0.85, 1.0, d) * 0.15)
        return finish_color(col, seed), rough, height, glow
    if region in ('carapace', 'head'):
        d, s = body_axes(U, V)
        col = mix(deep, slate, np.clip(0.4 + 0.3 * n, 0, 1))
        if region == 'carapace':
            brow = (1 - lt.smooth(0.03, 0.07, np.abs(np.abs(s) - 0.62))) * (V > 0.35)
            col = mix(col, cream, brow * 0.85)
        else:
            # The face: a cream fringe arching over the big front eyes, a slate face under it.
            fringe = (1 - lt.smooth(0.025, 0.06, np.abs(V - (0.86 - 0.35 * s * s)))) * (d > 0.0)
            col = mix(col, cream, fringe * 0.95)
            col = mix(col, cream * 0.85, lt.smooth(0.55, 0.75, np.abs(s)) * (V > 0.6) * 0.6)
        return finish_color(col, seed), rough, height, glow
    along, top = limb_axes(U, V)
    col = mix(deep, slate, np.clip(0.4 + 0.3 * n, 0, 1))
    if region == 'femur':
        col = mix(col, cream, bands(along, (0.92,), 0.05) * 0.6)
    elif region == 'tibia':
        col = mix(col, cream, bands(along, (0.04, 0.4, 0.75), 0.035) * 0.6)
    elif region == 'palp':
        col = mix(col, cream, 0.75)
    return finish_color(col, seed), rough, height, glow


# --- Extras ---

def moss_carpet(m, c, center, fwd, radii, seed_rng):
    """A lumpy shell of moss over the abdomen's top, thickest in the middle and tucked under the surface at its
    ragged edge; the deform the abdomen used is applied, so it sits on the real shape."""
    deform = c['abdomen'].get('deform')
    fwd = Vector(fwd).normalized()
    up = (Vector((0, 0, 1)) - fwd * fwd.z).normalized()
    left = up.cross(fwd)
    rings, segs = 60, 96
    seed = seed_rng.randint(0, 999)

    def wobble(a, s, u, k):
        return 0.5 * math.sin(a * 9.0 + seed + k) * math.cos(s * 8.0 - k) + 0.5 * math.sin(u * 11.0 + a * 4.0 + k * 2.0)

    grid = {}
    for i in range(1, rings):
        theta = math.pi * i / rings
        for j in range(segs):
            phi = -math.pi + 2.0 * math.pi * j / segs
            a0, r0 = -math.cos(theta), math.sin(theta)
            s0, u0 = r0 * math.sin(phi), r0 * math.cos(phi)
            cover = u0 + 0.18 * wobble(a0, s0, u0, 1.0) - 0.1
            if cover < -0.08:
                continue
            a, s_, u = deform(a0, s0, u0) if deform else (a0, s0, u0)
            lump = 0.5 * wobble(a0, s0, u0, 3.0) + 0.5 * math.sin(a0 * 31.0 + seed) * math.sin(s0 * 27.0 + u0 * 19.0 - seed)
            thick = 0.04 * smoothstep(-0.08, 0.35, cover) + 0.03 * lump * smoothstep(0.0, 0.3, cover)
            k = 0.99 + thick
            grid[i, j] = m.bm.verts.new(center + (fwd * (a * radii[0]) + left * (s_ * radii[1]) + up * (u * radii[2])) * 1.0
                                        + (fwd * (a * radii[0]) + left * (s_ * radii[1]) + up * (u * radii[2])).normalized()
                                        * (k - 1.0) * radii[2])
    uv = (0.5, 0.5)
    for (i, j), v00 in list(grid.items()):
        j1 = (j + 1) % segs
        q = [v00, grid.get((i, j1)), grid.get((i + 1, j1)), grid.get((i + 1, j))]
        if all(q):
            m.face(q, [uv] * 4, ACCENT)


def moss_tufts(m, c, rng):
    """Moss cushions, grass blades and a few tiny white flowers growing on the abdomen's back."""
    ab = c['abdomen']
    center = Vector((ab['x'], 0, c['ride'] + ab['z']))
    fwd = Vector(ab['fwd'])
    moss_carpet(m, c, center, fwd, ab['radii'], rng)
    for p, n, a, s, u in surface_points(center, ab['radii'], fwd, rng, 90, lambda a, s, u: u > 0.25):
        for _ in range(rng.randint(3, 6)):
            heading = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), 0)).normalized()
            h = rng.uniform(9.0, 22.0)
            w = rng.uniform(0.7, 1.3)
            side = n.cross(heading).normalized()
            root = p + side * rng.uniform(-2, 2) + heading * rng.uniform(-2, 2) - n * 0.5
            mid = root + n * h * 0.55 + heading * h * 0.15
            tip = root + n * h * 0.9 + heading * h * 0.45
            colour = rng.choice(['GrassFresh', 'GrassDeep', 'GrassOlive', 'GrassYellow'])
            v = [m.bm.verts.new(x) for x in (root - side * w, root + side * w, mid + side * w * 0.6, mid - side * w * 0.6, tip)]
            m.face((v[0], v[1], v[2], v[3]), [swatch(colour, 0, 0), swatch(colour, 0, 1), swatch(colour, 0.55, 1),
                                              swatch(colour, 0.55, 0)], FOLIAGE)
            m.face((v[3], v[2], v[4]), [swatch(colour, 0.55, 0), swatch(colour, 0.55, 1), swatch(colour, 1, 0.5)], FOLIAGE)
    for p, n, a, s, u in surface_points(center, ab['radii'], fwd, rng, 9, lambda a, s, u: u > 0.45):
        stem_top = p + n * 13.0
        tube(m, [p, stem_top], [0.4, 0.3], 'misc', mat=FOLIAGE)
        t1 = n.orthogonal().normalized()
        t2 = n.cross(t1)
        for k in range(6):
            ang = 2 * math.pi * k / 6
            d = t1 * math.cos(ang) + t2 * math.sin(ang)
            tip = stem_top + d * 2.8 + n * 0.4
            v = [m.bm.verts.new(x) for x in (stem_top, tip + d.cross(n) * 1.2, tip - d.cross(n) * 1.2)]
            m.face(v, [swatch('FlowerWhite', 0.1, 0.5), swatch('FlowerWhite', 0.9, 0), swatch('FlowerWhite', 0.9, 1)],
                   FOLIAGE)
        ellipsoid(m, stem_top + n * 0.5, (0.9, 0.9, 0.6), 'misc', mat=FOLIAGE, segs=8, rings=6, fwd=t1, up=n)


# --- The five concepts ---

def flat_bottom(k):
    return lambda a, s, u: (a, s, u * (k if u < 0 else 1.0))


def taper(front=0.15, bottom=0.65):
    return lambda a, s, u: (a, s * (1 - front * max(a, 0)), u * (bottom if u < 0 else 1.0))


def humped(a, s, u):
    bump = 0.18 * math.exp(-((a - 0.45) ** 2 + (abs(s) - 0.45) ** 2) / 0.05) * max(u, 0)
    return (a, s * (1 + bump), u * (1 + bump * 1.5) * (0.85 if u < 0 else 1.0))


def egg(front=0.12, bottom=0.8):
    return lambda a, s, u: (a, s * (1 - front * max(a, 0)), u * (1 - front * max(a, 0)) * (bottom if u < 0 else 1.0))


CONCEPTS = {
    'MeadowWolf': dict(
        seed=11, ride=60, painter=paint_meadow_wolf, coat=0.05,
        carapace=dict(x=0, z=2, radii=(46, 36, 22), deform=taper(0.15, 0.6)),
        head=dict(x=34, z=8, radii=(24, 22, 17), deform=flat_bottom(0.6)),
        abdomen=dict(x=-72, z=10, radii=(62, 44, 38), fwd=(1, 0, 0.12), deform=egg(0.12, 0.8)),
        eyes=[(54, 7.5, 18, 6.0), (58, 3.6, 10, 2.8), (56, 10.5, 10, 2.5), (45, 11, 24, 4.2)],
        chel=dict(x=55, y=7, z=-2, radii=(10, 7, 7)), palp=dict(x=50, y=12, z=0, len=40, r=4.0),
        hip_angle=(35, 70, 108, 142), rest_angle=(38, 72, 110, 148), rest_radius=(175, 155, 152, 178),
        hip_radius=(32, 27, -2), femur=(100, 90, 90, 104), tibia=(130, 116, 116, 134), leg_radius=7.8,
        leg_hairs=(26, 30), hair_len=7.0, hair_radius=0.45, bristle=0x24170c,
        body_hairs=[('carapace', 90, lambda a, s, u: u > 0.1), ('abdomen', 260, lambda a, s, u: u > -0.4)]),
    'MossBack': dict(
        seed=23, ride=58, painter=paint_moss_back, extras=moss_tufts, coat=0.0, accent=0x56742a, accent_rough=0.95,
        accent_metal=0.0, accent_coat=0.0, accent_moss=True,
        carapace=dict(x=0, z=2, radii=(38, 32, 20), deform=taper(0.2, 0.6)),
        head=dict(x=28, z=7, radii=(19, 17, 14), deform=flat_bottom(0.6)),
        abdomen=dict(x=-64, z=30, radii=(66, 58, 54), fwd=(1, 0, 0.45), deform=humped),
        eyes=[(45, 4.5, 15, 3.2), (46, 2.0, 11, 2.2), (42, 8.0, 15, 2.6), (41, 9.5, 11, 2.2)],
        chel=dict(x=44, y=6, z=-2, radii=(9, 6.5, 6.5)), palp=dict(x=40, y=10, z=0, len=34, r=3.4),
        hip_angle=(35, 70, 108, 142), rest_angle=(36, 72, 110, 150), rest_radius=(188, 160, 156, 182),
        hip_radius=(27, 24, -2), femur=(110, 96, 96, 112), tibia=(142, 126, 124, 146), leg_radius=7.0,
        leg_hairs=(18, 22), hair_len=6.0, hair_radius=0.4, bristle=0x2b2a1c, fur_bump=0.2,
        body_hairs=[('carapace', 40, lambda a, s, u: u > 0.1)]),
    'CliffHuntsman': dict(
        seed=37, ride=40, painter=paint_cliff_huntsman, detail_set='RockGranite', detail_scale=0.8,
        detail_amount=0.55, knee_out=0.9,
        carapace=dict(x=0, z=0, radii=(48, 44, 15), deform=taper(0.1, 0.5)),
        head=dict(x=32, z=3, radii=(20, 24, 11), deform=flat_bottom(0.6)),
        abdomen=dict(x=-62, z=2, radii=(64, 44, 21), fwd=(1, 0, 0.04), deform=egg(0.1, 0.7)),
        eyes=[(50, 5.5, 8, 2.6), (50, 13.5, 8, 2.4), (47, 4.0, 12, 2.2), (46, 12.0, 12, 2.2)],
        chel=dict(x=48, y=7, z=-3, radii=(8, 6, 6)), palp=dict(x=46, y=12, z=-2, len=36, r=3.4),
        hip_angle=(42, 74, 106, 138), rest_angle=(32, 76, 110, 152), rest_radius=(212, 198, 194, 210),
        hip_radius=(30, 32, -2), femur=(122, 116, 116, 124), tibia=(156, 150, 150, 160), leg_radius=7.2,
        leg_hairs=(14, 16), hair_len=6.0, hair_radius=0.4, bristle=0x67635a, fur_bump=0.15,
        body_hairs=[('abdomen', 60, lambda a, s, u: u > 0.0)]),
    'EmberTarantula': dict(
        seed=53, ride=56, painter=paint_ember_tarantula, glow_color=0xff7a2a, coat=0.0, fur_scale=220.0, fur_bump=0.4,
        eye_color=0x2a0e04, eye_glow=0xff6a1a, eye_glow_strength=3.0,
        carapace=dict(x=0, z=2, radii=(50, 44, 22), deform=taper(0.1, 0.6)),
        head=dict(x=30, z=10, radii=(22, 22, 18), deform=flat_bottom(0.6)),
        abdomen=dict(x=-76, z=8, radii=(68, 56, 48), fwd=(1, 0, 0.1), deform=egg(0.1, 0.8)),
        eyes=[(36, 3.0, 28, 2.6), (37, 7.0, 27, 2.0), (34, 5.0, 30, 1.8)],
        chel=dict(x=50, y=9, z=0, radii=(14, 9, 9), fang=1.4), palp=dict(x=46, y=15, z=0, len=50, r=6.0, club=1.05),
        hip_angle=(33, 68, 108, 142), rest_angle=(30, 70, 112, 150), rest_radius=(170, 150, 148, 172),
        hip_radius=(35, 32, -2), femur=(98, 88, 88, 102), tibia=(116, 104, 104, 120), leg_radius=10.5,
        leg_hairs=(140, 150), hair_len=9.0, hair_radius=0.5, bristle=0x1a120d, bristle2=0xc8642a,
        bristle2_legs=0.18, bristle2_body={'abdomen': 0.45},
        body_hairs=[('carapace', 220, lambda a, s, u: u > 0.0), ('abdomen', 900, lambda a, s, u: u > -0.6)]),
    'SkyJumper': dict(
        seed=71, ride=58, painter=paint_sky_jumper, coat=0.1, fur_scale=200.0, fur_bump=0.35,
        eye_color=0x050708, accent=0x2fb0a8, bristle=0x2a2f37, bristle2=0xe8dfc8, bristle2_body={'head': 0.0},
        carapace=dict(x=-4, z=8, radii=(44, 38, 28), deform=lambda a, s, u: (a, s * (1 - 0.05 * max(a, 0)), u * (0.55 if u < 0 else 1.0))),
        head=dict(x=22, z=14, radii=(28, 34, 27), deform=lambda a, s, u: (min(a, 0.62) * (1.0 if a < 0.62 else 1.0) + max(a - 0.62, 0) * 0.15, s, u * (0.6 if u < 0 else 1.0))),
        abdomen=dict(x=-62, z=8, radii=(52, 40, 34), fwd=(1, 0, 0.1), deform=egg(0.12, 0.8)),
        eyes=[(37.5, 9.8, 16, 9.5), (35.5, 20.5, 18.5, 5.0), (14, 24, 33, 3.4), (26, 25, 31, 1.9)],
        chel=dict(x=37, y=7, z=-1, radii=(10, 5.5, 5.5), accent=True), palp=dict(x=38, y=15, z=2, len=28, r=4.0, club=1.15),
        hip_angle=(35, 70, 108, 142), rest_angle=(34, 70, 110, 148), rest_radius=(148, 132, 130, 148),
        hip_radius=(30, 28, -2), femur=(88, 74, 74, 86), tibia=(104, 88, 88, 102), leg_radius=8.4,
        leg_scale=(1.35, 1.0, 1.0, 1.05), leg_hairs=(80, 80), hair_len=4.5, hair_radius=0.32,
        bristle2_legs=0.15,
        body_hairs=[('carapace', 240, lambda a, s, u: u > 0.0), ('head', 120, lambda a, s, u: u > 0.2 and a > 0.25),
                    ('abdomen', 420, lambda a, s, u: u > -0.5)]),
}

built = {}
for k, (name, concept) in enumerate(CONCEPTS.items()):
    obj = build(concept, name)
    obj.location.x = k * 4.5   # side by side in the scene
    built[name] = obj
    lt._log(f'{name}: {sum(len(p.vertices) - 2 for p in obj.data.polygons)} triangles')

if lt.want_preview():
    argv = sys.argv[sys.argv.index('--') + 1:]
    for name in [a for a in argv if a in CONCEPTS] or list(CONCEPTS):
        lt.preview([built[name]], lt.preview_path('Backlog', 'Spider' + name), view=(0.7, -1.0, 0.55))
