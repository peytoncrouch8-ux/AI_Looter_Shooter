"""Building blocks for creatures in the stylized-realism style (Art/Models/Creatures/*.py): smooth segmented bodies,
banded limbs, bristles, and one painted texture atlas per creature.

Geometry is built in a creature space in centimeters (x forward, y left, z up, the origin on the ground under the body)
and turned into Blender meters, front to -Y, by to_blender(). Every part maps its UVs onto a rectangle of the creature's
atlas: body parts latitude-longitude (U around, 0.5 = the back; V along, 0 = the rear pole, 1 = the front), limbs along
(U along, 0 = the body end; V around, 0.5 = the top), and small parts (eyes, bristles, fangs) onto a flat patch, the
middle of a rectangle. The atlas's painters use the same axes (body_axes, limb_axes).

    import looter_creatures as lc
    mesh = lc.Mesh(atlas)
    lc.ellipsoid(mesh, (0, 0, 60), (46, 36, 22), 'carapace')
    obj = mesh.finish('Thorax', material)
"""
import math

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_textures as lt

# Creature centimeters (x forward, y left) to Blender meters (front to -Y): a quarter turn and 1/100.
TO_BLENDER = Matrix.Rotation(-math.pi / 2.0, 4, 'Z') @ Matrix.Scale(0.01, 4)


def to_blender(point):
    return TO_BLENDER @ Vector(point)


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def solve_two_bone(hip, target, upper, lower, pole):
    """The game's two-bone IK (ASpiderCreature): the knee keeps both segments their length, bent toward pole. Returns
    (knee, foot)."""
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
    """One part being built: a bmesh with UVs into the creature's atlas ({region: (u0, u1, v0, v1)})."""

    def __init__(self, atlas):
        self.atlas = atlas
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')

    def at(self, region, u, v):
        u0, u1, v0, v1 = self.atlas[region]
        return (u0 + min(max(u, 0.0), 1.0) * (u1 - u0), v0 + min(max(v, 0.0), 1.0) * (v1 - v0))

    def face(self, verts, uvs):
        f = self.bm.faces.new(verts)
        f.smooth = True
        for loop, uv in zip(f.loops, uvs):
            loop[self.uv].uv = uv
        return f

    def finish(self, name, material, sharp=70.0):
        """The part as a Blender object (in Blender meters), one material, smooth with hard edges past sharp degrees."""
        bmesh.ops.transform(self.bm, matrix=TO_BLENDER, verts=self.bm.verts)
        mesh = bpy.data.meshes.new(name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        mesh.materials.append(material)
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        mesh.set_sharp_from_angle(angle=math.radians(sharp))
        return obj


def ellipsoid(m, center, radii, region, fwd=(1, 0, 0), up=(0, 0, 1), segs=24, rings=14, deform=None, patch=None):
    """A smooth ellipsoid with its poles along fwd, latitude-longitude UVs on its atlas region (or every vertex on the
    middle of region when patch is set: eyes and other small solid-colored parts). deform(a, s, u) takes the unit
    sphere's point (along, left, up) and returns it changed."""
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

    uv = (lambda u, v: m.at(region, 0.5, 0.5)) if patch else (lambda u, v: m.at(region, u, v))
    bm = m.bm
    rear = bm.verts.new(point(0.0, 0.0))
    front = bm.verts.new(point(math.pi, 0.0))
    ring = [[bm.verts.new(point(math.pi * i / rings, -math.pi + 2.0 * math.pi * j / segs)) for j in range(segs)]
            for i in range(1, rings)]
    for j in range(segs):
        j1 = (j + 1) % segs
        ua, ub = j / segs, (j + 1) / segs
        m.face((rear, ring[0][j1], ring[0][j]), [uv((ua + ub) / 2, 0.0), uv(ub, 1 / rings), uv(ua, 1 / rings)])
        for i in range(rings - 2):
            va, vb = (i + 1) / rings, (i + 2) / rings
            m.face((ring[i][j], ring[i][j1], ring[i + 1][j1], ring[i + 1][j]), [uv(ua, va), uv(ub, va), uv(ub, vb), uv(ua, vb)])
        m.face((ring[-1][j], ring[-1][j1], front), [uv(ua, 1 - 1 / rings), uv(ub, 1 - 1 / rings), uv((ua + ub) / 2, 1.0)])


def tube(m, pts, radii, region, sides=10, up=(0, 0, 1), plane=None, patch=False):
    """A closed tube along pts with a radius per point; U along, V around (0.5 = toward up). plane: the normal of the
    plane the tube bends in (a leg's), which keeps the rings from twisting where a segment passes the up direction."""
    pts = [Vector(p) for p in pts]
    up = Vector(up).normalized()
    dist = [0.0]
    for a, b in zip(pts, pts[1:]):
        dist.append(dist[-1] + (b - a).length)
    total = max(dist[-1], 1e-6)
    bm = m.bm
    # In a plane, which way the rings' top faces is settled once from the whole tube's direction.
    flip = 1.0
    if plane is not None and Vector(plane).cross((pts[-1] - pts[0]).normalized()).dot(up) < 0:
        flip = -1.0
    rings = []
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
    uv = (lambda u, v: m.at(region, 0.5, 0.5)) if patch else (lambda u, v: m.at(region, u, v))
    for k in range(len(pts) - 1):
        ua, ub = dist[k] / total, dist[k + 1] / total
        for j in range(sides):
            j1 = (j + 1) % sides
            va, vb = j / sides, (j + 1) / sides
            m.face((rings[k][j], rings[k + 1][j], rings[k + 1][j1], rings[k][j1]), [uv(ua, va), uv(ub, va), uv(ub, vb), uv(ua, vb)])
    for k, end_u in ((0, 0.0), (len(pts) - 1, 1.0)):
        hub = bm.verts.new(pts[k])
        for j in range(sides):
            j1 = (j + 1) % sides
            m.face((hub, rings[k][j1], rings[k][j]) if k == 0 else (hub, rings[k][j], rings[k][j1]),
                   [uv(end_u, 0.5), uv(end_u, (j + 1) / sides), uv(end_u, j / sides)])


def bristle(m, base, direction, length, radius, region):
    """A coarse hair: a thin closed three-sided cone, on the middle of region (a solid patch)."""
    d = Vector(direction).normalized()
    a = d.orthogonal().normalized()
    b = d.cross(a)
    ring = [m.bm.verts.new(base + (a * math.cos(t) + b * math.sin(t)) * radius) for t in (0.0, 2.094, 4.189)]
    tip = m.bm.verts.new(base + d * length)
    uv = m.at(region, 0.5, 0.5)
    for j in range(3):
        m.face((ring[j], ring[(j + 1) % 3], tip), [uv] * 3)
    m.face((ring[2], ring[1], ring[0]), [uv] * 3)


def surface_points(center, radii, fwd, rng, count, where, deform=None):
    """Random points on an ellipsoid (fwd along its x), kept where where(a, s, u) holds, with outward normals."""
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
        da, ds, du = deform(a, s, u) if deform else (a, s, u)
        p = Vector(center) + fwd * (da * radii[0]) + left * (ds * radii[1]) + up * (du * radii[2])
        n = (fwd * (a / radii[0]) + left * (s / radii[1]) + up * (u / radii[2])).normalized()
        out.append((p, n, a, s, u))
    return out


def seat_on(center, radii, point, push):
    """point moved onto an ellipsoid's surface (axis-aligned, around center) along the line from its middle, then
    pushed out by push along the surface normal: how an eye sits on a head."""
    center = Vector(center)
    rel = Vector(point) - center
    rx, ry, rz = radii
    rel = rel / math.sqrt((rel.x / rx) ** 2 + (rel.y / ry) ** 2 + (rel.z / rz) ** 2)
    normal = Vector((rel.x / rx ** 2, rel.y / ry ** 2, rel.z / rz ** 2)).normalized()
    return center + rel + normal * push


# --- Painting an atlas ---

def region_grid(region, size):
    """The pixel block of an atlas region ((u0, u1, v0, v1) of a size x size texture) and its local U, V (0..1)."""
    u0, u1, v0, v1 = region
    c0, c1 = int(round(u0 * size)), int(round(u1 * size))
    r0, r1 = int(round((1 - v1) * size)), int(round((1 - v0) * size))
    cols = (np.arange(c0, c1) + 0.5) / size
    rows = 1 - (np.arange(r0, r1) + 0.5) / size
    uu, vv = np.meshgrid((cols - u0) / (u1 - u0), (rows - v0) / (v1 - v0))
    return (slice(r0, r1), slice(c0, c1)), uu.astype(np.float32), vv.astype(np.float32)


def body_axes(U, V):
    """For body regions: d = 1 on the back, -1 on the belly; s = -1..1 from side to side."""
    ang = 2.0 * np.pi * (U - 0.5)
    return np.cos(ang), np.sin(ang)


def limb_axes(U, V):
    """For limbs: along = U, top = 1 on the upper side, -1 underneath."""
    return U, np.cos(2.0 * np.pi * (V - 0.5))


def bands(x, centers, width):
    out = np.zeros_like(x)
    for c0 in centers:
        out = np.maximum(out, 1.0 - lt.smooth(width * 0.5, width, np.abs(x - c0)))
    return out


def paint_atlas(atlas, painter, size, px_m, out_dir, set_name, normal_strength=1.0):
    """Paints every region with painter(region, U, V, index) -> (color, roughness, height, occlusion), and writes the
    texture set T_<set>_BC, _N (from the height, per region so each wraps around its own seam) and _ORM."""
    color = np.zeros((size, size, 3), np.float32)
    rough = np.full((size, size), 0.7, np.float32)
    occl = np.ones((size, size), np.float32)
    normal = np.zeros((size, size, 3), np.uint8)
    normal[...] = (128, 128, 255)
    for k, (name, region) in enumerate(atlas.items()):
        sl, U, V = region_grid(region, size)
        col, r, h, ao = painter(name, U, V, k)
        color[sl], rough[sl], occl[sl] = col, r, ao
        normal[sl] = lt.normal_map(h.astype(np.float32), px_m, normal_strength)
    lt.write_png(f'{out_dir}/T_{set_name}_BC.png', lt.to8(np.clip(color, 0, 1)))
    lt.write_png(f'{out_dir}/T_{set_name}_N.png', normal)
    lt.write_png(f'{out_dir}/T_{set_name}_ORM.png', lt.to8(np.stack([np.clip(occl, 0, 1), np.clip(rough, 0, 1),
                                                                     np.zeros_like(rough)], axis=-1)))
