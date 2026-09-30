"""Helpers for models ported from the game's code, which built them in Unreal's space and units: X forward, Y right,
Z up, centimeters, Unreal's rotators and GeometryScript's primitives. A Part is built that way and turned into
Blender's space at the end (UNREAL_TO_BLENDER), so the numbers can be copied from the old C++ as they were.
Used by Art/Models/Creatures/Spider.py and Art/Models/Weapons/*.py.
"""
import math
import struct

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_model as lm

# Unreal (cm; X forward, Y right) to Blender (m; the front faces -Y, and +X is Unreal's -Y). A mirror, so faces turn
# inside out with it and get turned back.
UNREAL_TO_BLENDER = Matrix(((0.0, -0.01, 0.0, 0.0), (-0.01, 0.0, 0.0, 0.0), (0.0, 0.0, 0.01, 0.0), (0.0, 0.0, 0.0, 1.0)))


def to_blender(point):
    return UNREAL_TO_BLENDER @ Vector(point)


def direction_to_blender(direction):
    return Vector((-direction[1], -direction[0], direction[2]))


# --- Unreal's math ---

def axes(x, y, z):
    """The rotation whose turned X, Y and Z axes are these."""
    return Matrix((x, y, z)).transposed()


def rotator(pitch=0.0, yaw=0.0, roll=0.0):
    """Unreal's FRotator, in degrees."""
    sp, cp = math.sin(math.radians(pitch)), math.cos(math.radians(pitch))
    sy, cy = math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
    sr, cr = math.sin(math.radians(roll)), math.cos(math.radians(roll))
    return axes((cp * cy, cp * sy, sp),
                (sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp),
                (-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp))


def make_from_z(z_axis):
    """FRotationMatrix::MakeFromZ."""
    z = Vector(z_axis).normalized()
    up = Vector((0.0, 0.0, 1.0)) if abs(z.z) < 1.0 - 1e-4 else Vector((1.0, 0.0, 0.0))
    x = up.cross(z).normalized()
    return axes(x, z.cross(x), z)


def make_from_xz(x_axis, z_axis):
    """FRotationMatrix::MakeFromXZ."""
    x = Vector(x_axis).normalized()
    near = Vector(z_axis).normalized()
    if abs(abs(x.dot(near)) - 1.0) < 1e-8:
        near = Vector((0.0, 0.0, 1.0)) if abs(x.z) < 1.0 - 1e-4 else Vector((1.0, 0.0, 0.0))
    y = near.cross(x).normalized()
    return axes(x, y, x.cross(y))


def transform(location=(0.0, 0.0, 0.0), rotation=None, scale=(1.0, 1.0, 1.0)):
    """Unreal's FTransform: scale, then rotation, then location."""
    turn = rotation.to_4x4() if rotation is not None else Matrix.Identity(4)
    return Matrix.Translation(Vector(location)) @ turn @ Matrix.Diagonal(Vector((*scale, 1.0)))


ALONG_X = rotator(pitch=-90.0)  # points a primitive's +Z along +X


class RandomStream:
    """Unreal's FRandomStream, so a ported model's random details come out as they did in the game."""

    def __init__(self, seed):
        self.seed = seed & 0xFFFFFFFF

    def frand_range(self, low, high):
        self.seed = (self.seed * 196314165 + 907633515) & 0xFFFFFFFF
        fraction = struct.unpack('<f', struct.pack('<I', 0x3F800000 | (self.seed >> 9)))[0] - 1.0
        return low + (high - low) * fraction


# --- Parts ---

class Part:
    """One model's geometry, built in Unreal's space with GeometryScript's primitives. Material slots are indexes into
    the materials given to build(). smooth_angle is FinishNormals': 0 is faceted."""

    def __init__(self, name, smooth_angle=0.0):
        self.name = name
        self.smooth_angle = smooth_angle
        self.bm = bmesh.new()
        self.bm.loops.layers.uv.new('UVMap')
        self.sockets = []

    def _face(self, verts, slot):
        self.bm.faces.new(verts).material_index = slot

    def box(self, slot, center, size, rotation=None):
        """A box of size (x, y, z) centered on center (GeometryScript's AppendBox with no subdivisions)."""
        matrix = transform(center, rotation, size)
        corners = [self.bm.verts.new(matrix @ Vector((x * 0.5, y * 0.5, z * 0.5)))
                   for x in (-1.0, 1.0) for y in (-1.0, 1.0) for z in (-1.0, 1.0)]
        for quad in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
            self._face([corners[i] for i in quad], slot)

    def ball(self, slot, matrix, radius, steps_phi, steps_theta):
        """A lat-long sphere: steps_phi vertices from pole to pole, steps_theta around."""
        rings = []
        for p in range(1, steps_phi - 1):
            phi = math.pi * p / (steps_phi - 1)
            ring = []
            for t in range(steps_theta):
                theta = 2.0 * math.pi * t / steps_theta
                ring.append(self.bm.verts.new(matrix @ Vector((radius * math.sin(phi) * math.cos(theta),
                                                               radius * math.sin(phi) * math.sin(theta), radius * math.cos(phi)))))
            rings.append(ring)
        north = self.bm.verts.new(matrix @ Vector((0.0, 0.0, radius)))
        south = self.bm.verts.new(matrix @ Vector((0.0, 0.0, -radius)))
        count = steps_theta
        for t in range(count):
            self._face((north, rings[0][t], rings[0][(t + 1) % count]), slot)
            self._face((south, rings[-1][(t + 1) % count], rings[-1][t]), slot)
        for upper, lower in zip(rings, rings[1:]):
            for t in range(count):
                self._face((upper[t], lower[t], lower[(t + 1) % count], upper[(t + 1) % count]), slot)

    def cone(self, slot, matrix, base, top, height, sides, height_steps=0):
        """A capped cone (a cylinder when top is base) along +Z from its base at the origin; a point when top is 0."""
        rings = []
        for step in range(height_steps + 2):
            z = height * step / (height_steps + 1)
            radius = base + (top - base) * step / (height_steps + 1)
            if radius <= 1e-6:
                rings.append([self.bm.verts.new(matrix @ Vector((0.0, 0.0, z)))])
                continue
            rings.append([self.bm.verts.new(matrix @ Vector((radius * math.cos(2.0 * math.pi * k / sides),
                                                             radius * math.sin(2.0 * math.pi * k / sides), z))) for k in range(sides)])
        for lower, upper in zip(rings, rings[1:]):
            for k in range(sides):
                following = (k + 1) % sides
                if len(upper) == 1:
                    self._face((lower[k], lower[following], upper[0]), slot)
                else:
                    self._face((lower[k], lower[following], upper[following], upper[k]), slot)
        self._face(list(reversed(rings[0])), slot)
        if len(rings[-1]) > 1:
            self._face(rings[-1], slot)

    def cylinder(self, slot, matrix, radius, height, sides):
        self.cone(slot, matrix, radius, radius, height, sides)

    def displace(self, magnitude, frequency, seed_offset):
        """Smooth noise over every vertex, by position, like the game's StylizedMesh::Displace (with Blender's Perlin)."""
        offset = Vector((seed_offset * 1.37, seed_offset * 2.11, seed_offset * 0.73))

        def octave(p):
            return Vector((noise.noise(p, noise_basis='PERLIN_ORIGINAL'),
                           noise.noise(p + Vector((31.7, 11.3, 5.1)), noise_basis='PERLIN_ORIGINAL'),
                           noise.noise(p + Vector((7.9, 57.3, 23.9)), noise_basis='PERLIN_ORIGINAL')))

        for vert in self.bm.verts:
            sample = vert.co * frequency + offset
            vert.co += (octave(sample) + octave(sample * 2.3 + Vector((3.3, 3.3, 3.3))) * 0.5) * 1.5 * magnitude

    def scale(self, factor):
        """Scales everything built so far (and its sockets) about the origin, like the component scale a code-built
        model was shown at."""
        bmesh.ops.scale(self.bm, vec=(factor, factor, factor), verts=self.bm.verts)
        self.sockets = [(name, location * factor, rotation) for name, location, rotation in self.sockets]

    def socket(self, name, location, rotation=None):
        """An attach point on this model, in the model's own space: relative to build's frame when it has one."""
        self.sockets.append((name, Vector(location), rotation))

    def build(self, materials, frame=None):
        """The part as a Blender object: turned into Blender's space, facing outward, with its hard and soft edges.
        With frame (an Unreal transform), the geometry built at that place becomes the model's own: its origin is the
        frame's origin and its axes the frame's, like a part that hangs from a socket."""
        local = frame.inverted() if frame is not None else Matrix.Identity(4)
        bmesh.ops.transform(self.bm, matrix=UNREAL_TO_BLENDER @ local, verts=self.bm.verts)
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces)
        # Only the materials it uses become its slots.
        used = sorted({face.material_index for face in self.bm.faces})
        for face in self.bm.faces:
            face.material_index = used.index(face.material_index)
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        for index in used:
            mesh.materials.append(materials[index])
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        if self.smooth_angle > 0.0:
            lm.smooth(obj, self.smooth_angle)
        for name, location, rotation in self.sockets:
            place_socket(obj, name, location, rotation)
        return obj


def place_socket(parent, name, location, rotation=None):
    """A SOCKET_ empty on parent from an Unreal location and rotation: the socket's +X (forward) and +Z (up) end up as
    the empty's -Y and +Z, which the exporter reads back as the socket's forward and up."""
    turn = rotation if rotation is not None else Matrix.Identity(3)
    forward = direction_to_blender(turn @ Vector((1.0, 0.0, 0.0))).normalized()
    up = direction_to_blender(turn @ Vector((0.0, 0.0, 1.0))).normalized()
    y = -forward
    x = y.cross(up)
    empty = lm.socket(parent, name)
    empty.matrix_world = Matrix.Translation(to_blender(location)) @ axes(x, y, up).to_4x4()
    return empty
