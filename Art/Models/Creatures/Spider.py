"""The brown hunting spider (ASpiderCreature; a wolf-spider look): a rig the game animates in code, with an
eight-legged stepping gait and two-bone IK, and a hit zone around every part.

Ported from the spider the game used to build at runtime, so its parts are built the same way: in Unreal's space and
units (X forward, Y right, Z up, centimeters) with Unreal's rotators, then turned into Blender's (UNREAL_TO_BLENDER).
The origin is the ground under the thorax, which rides RIDE_HEIGHT above it.

Bones, which the game finds by name: body (the thorax); head, with the eyes, and its pedipalps palp_l and palp_r;
fang_l and fang_r; abdomen; and per leg femur_<pair>_<side>, tibia_<pair>_<side> and foot_<pair>_<side> (the tip),
pairs 0 (front) to 3 (back) on sides l and r. The legs rest in the standing pose, solved with the game's two-bone IK:
ASpiderCreature measures its legs from these bones and must bend the knees toward the same pole (knee_pole).
"""
import math
import struct

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_model as lm

RIDE_HEIGHT = 62.0

# Leg layout per pair, front (0) to back (3). Angles are degrees from straight ahead.
HIP_ANGLE = (38.0, 72.0, 106.0, 140.0)
REST_ANGLE = (40.0, 74.0, 108.0, 146.0)
REST_RADIUS = (170.0, 152.0, 150.0, 172.0)
FEMUR_LENGTH = (98.0, 88.0, 88.0, 100.0)
TIBIA_LENGTH = (122.0, 110.0, 110.0, 126.0)
SIDES = ((-1.0, 'l'), (1.0, 'r'))

# Material slots, in the order every part lists the materials.
BODY_SLOT, MARK_SLOT, BELLY_SLOT, EYE_SLOT = range(4)

# Unreal (cm; X forward, Y right) to Blender (m; the front faces -Y, and +X is Unreal's -Y). A mirror, so faces turn
# inside out with it and get turned back.
UNREAL_TO_BLENDER = Matrix(((0.0, -0.01, 0.0, 0.0), (-0.01, 0.0, 0.0, 0.0), (0.0, 0.0, 0.01, 0.0), (0.0, 0.0, 0.0, 1.0)))


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
    """Unreal's FRandomStream, so the bristles grow where the old spider's did."""

    def __init__(self, seed):
        self.seed = seed & 0xFFFFFFFF

    def frand_range(self, low, high):
        self.seed = (self.seed * 196314165 + 907633515) & 0xFFFFFFFF
        fraction = struct.unpack('<f', struct.pack('<I', 0x3F800000 | (self.seed >> 9)))[0] - 1.0
        return low + (high - low) * fraction


def solve_two_bone(hip, target, upper, lower, pole):
    """The game's two-bone IK: the knee keeps both segments their length, bent toward pole. Returns (knee, foot)."""
    to_target = target - hip
    distance = to_target.length
    direction = to_target / distance
    distance = min(max(distance, abs(upper - lower) + 1.0), (upper + lower) * 0.999)
    along = (upper * upper - lower * lower + distance * distance) / (2.0 * distance)
    height = math.sqrt(max(upper * upper - along * along, 0.0))
    bend = (pole - direction * pole.dot(direction)).normalized()
    return hip + direction * along + bend * height, hip + direction * distance


def knee_pole(hip):
    """Where knees bend: up, and a little out from the body (ASpiderCreature uses the same rule)."""
    outward = Vector((hip.x, hip.y, 0.0)).normalized()
    return Vector((0.0, 0.0, 1.0)) + outward * 0.4


# --- Parts ---

class Part:
    """One bone's geometry, built in Unreal's space with GeometryScript's primitives."""

    def __init__(self, name, smooth_angle):
        self.name = name
        self.smooth_angle = smooth_angle
        self.bm = bmesh.new()
        self.bm.loops.layers.uv.new('UVMap')

    def _face(self, verts, slot):
        self.bm.faces.new(verts).material_index = slot

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

    def build(self, materials):
        """The part as a Blender object: turned into Blender's space, facing outward, with its hard and soft edges."""
        bmesh.ops.transform(self.bm, matrix=UNREAL_TO_BLENDER, verts=self.bm.verts)
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces)
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        for material in materials:
            mesh.materials.append(material)
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        lm.smooth(obj, self.smooth_angle)
        return obj


def add_bristles(part, frame, random, length, radius, count):
    """Coarse hairs lying toward the tip of a limb segment built along +X."""
    for _ in range(count):
        x = length * random.frand_range(0.15, 0.9)
        around = math.radians(random.frand_range(-110.0, 110.0))  # mostly top and sides
        normal = Vector((0.0, math.sin(around), math.cos(around)))
        direction = normal * 0.6 + Vector((0.75, 0.0, 0.0))
        part.cone(MARK_SLOT, frame @ transform(Vector((x, 0.0, 0.0)) + normal * radius * 0.85, make_from_z(direction)),
                  1.3, 0.0, random.frand_range(7.0, 11.0), 3)


def build_femur(part, frame, length, random):
    part.ball(BELLY_SLOT, frame, 9.5, 4, 6)
    part.cone(BODY_SLOT, frame @ transform(rotation=ALONG_X), 8.5, 6.5, length, 7, 2)
    part.cylinder(MARK_SLOT, frame @ transform((length * 0.62, 0.0, 0.0), ALONG_X), 7.6, length * 0.14, 7)
    add_bristles(part, frame, random, length, 7.5, 6)


def build_tibia(part, frame, length, random):
    shaft = length * 0.92
    part.ball(BELLY_SLOT, frame, 8.0, 4, 6)
    part.cone(BODY_SLOT, frame @ transform(rotation=ALONG_X), 6.5, 2.6, shaft, 7, 2)
    part.cylinder(MARK_SLOT, frame @ transform((length * 0.3, 0.0, 0.0), ALONG_X), 6.1, length * 0.1, 7)
    part.cylinder(MARK_SLOT, frame @ transform((length * 0.65, 0.0, 0.0), ALONG_X), 4.6, length * 0.08, 7)
    part.cone(MARK_SLOT, frame @ transform((shaft, 0.0, 0.0), ALONG_X), 2.6, 0.4, length - shaft, 5)
    add_bristles(part, frame, random, length * 0.8, 5.5, 5)


def to_blender(point):
    return UNREAL_TO_BLENDER @ Vector(point)


# --- The spider ---

materials = [
    lm.material('SpiderBody', 0x6e4a2c, Variation=0.14),
    lm.material('SpiderMark', 0x33200f, Variation=0.08),
    lm.material('SpiderBelly', 0xb88c5c, Variation=0.1),
    lm.material('SpiderEye', 0x3a1606, Variation=0.0, Glow=6.0),  # a dim ember glint, so the eyes read at range
]

# The body frame: the thorax's center, where the old spider's parts were placed from.
body = transform((0.0, 0.0, RIDE_HEIGHT))
parts = {}  # bone name: Part
bones = [('body', Vector((0.0, 0.0, RIDE_HEIGHT)), Vector((25.0, 0.0, RIDE_HEIGHT)), None),
         ('head', Vector((26.0, 0.0, RIDE_HEIGHT + 8.0)), Vector((46.0, 0.0, RIDE_HEIGHT + 8.0)), 'body')]

thorax = parts['body'] = Part('Thorax', 60.0)
thorax.ball(BODY_SLOT, body @ transform(scale=(1.3, 1.05, 0.62)), 34.0, 7, 12)
thorax.ball(MARK_SLOT, body @ transform((-2.0, 0.0, 15.0), scale=(1.25, 0.32, 0.28)), 28.0, 5, 8)
for side, _ in SIDES:
    thorax.ball(BELLY_SLOT, body @ transform((-4.0, side * 22.0, 6.0), rotator(yaw=side * 8.0), (1.1, 0.25, 0.3)), 26.0, 4, 8)

head = parts['head'] = Part('Head', 60.0)
head.ball(BODY_SLOT, body @ transform((36.0, 0.0, 9.0), rotator(pitch=-8.0), (0.95, 1.0, 0.8)), 25.0, 6, 10)
head.ball(MARK_SLOT, body @ transform((34.0, 0.0, 24.0), scale=(1.0, 0.35, 0.2)), 16.0, 4, 6)
# Wolf-spider eyes: a big forward pair, a row of four small ones below, a pair on top.
for location, radius in (((56.0, 8.0, 17.0), 5.5), ((56.0, -8.0, 17.0), 5.5), ((59.0, 4.0, 9.0), 2.8), ((59.0, -4.0, 9.0), 2.8),
                         ((57.0, 11.0, 9.0), 2.5), ((57.0, -11.0, 9.0), 2.5), ((46.0, 11.0, 25.0), 4.0), ((46.0, -11.0, 25.0), 4.0)):
    head.ball(EYE_SLOT, body @ transform(location), radius, 4, 6)

# Pedipalps: short two-part feelers reaching forward and down.
for side, suffix in SIDES:
    start = Vector((54.0, side * 13.0, 2.0))
    first = Vector((0.8, side * 0.25, -0.55)).normalized()
    elbow = start + first * 26.0
    second = Vector((0.5, side * 0.1, -0.85)).normalized()
    palp = parts[f'palp_{suffix}'] = Part(f'Palp_{suffix}', 60.0)
    palp.cone(BODY_SLOT, body @ transform(start, make_from_z(first)), 4.5, 3.0, 26.0, 6)
    palp.cone(BODY_SLOT, body @ transform(elbow, make_from_z(second)), 3.0, 2.2, 22.0, 6)
    palp.ball(MARK_SLOT, body @ transform(elbow + second * 22.0), 3.2, 3, 5)
    bones.append((f'palp_{suffix}', body @ start, body @ elbow, 'head'))

# Chelicerae, each with a curved fang hooking inward underneath. They spread open for a bite.
for side, suffix in SIDES:
    pivot = Vector((58.0, side * 8.0, -2.0))
    fang_frame = body @ transform(pivot)
    fang = parts[f'fang_{suffix}'] = Part(f'Fang_{suffix}', 60.0)
    fang.ball(BODY_SLOT, fang_frame @ transform((2.0, 0.0, -6.0), scale=(0.9, 0.85, 1.35)), 8.0, 5, 8)
    fang.cone(MARK_SLOT, fang_frame @ transform((4.0, 0.0, -15.0), make_from_z((0.35, 0.0, -1.0))), 3.2, 0.3, 13.0, 5)
    bones.append((f'fang_{suffix}', body @ pivot, body @ (pivot + Vector((4.0, 0.0, -15.0))), 'head'))

# The abdomen sways from a pivot behind the thorax.
abdomen_pivot = Vector((-30.0, 0.0, 6.0))
abdomen_frame = body @ transform(abdomen_pivot)
abdomen = parts['abdomen'] = Part('Abdomen', 60.0)
center = Vector((-52.0, 0.0, 12.0))
radii = Vector((46.0 * 1.35, 46.0, 46.0 * 0.88))
abdomen.ball(BODY_SLOT, abdomen_frame @ transform(center, scale=(1.35, 1.0, 0.88)), 46.0, 9, 14)
abdomen.ball(BELLY_SLOT, abdomen_frame @ transform((-50.0, 0.0, -4.0), scale=(1.2, 0.78, 0.5)), 42.0, 6, 10)
# Chevrons: pairs of dark marks angled down the back, shrinking toward the spinnerets.
for index in range(4):
    x = -28.0 - index * 20.0
    along = (x - center.x) / radii.x
    top_z = center.z + radii.z * math.sqrt(max(0.0, 1.0 - along * along))
    size = 14.0 - index * 2.2
    for side, _ in SIDES:
        abdomen.ball(MARK_SLOT, abdomen_frame @ transform((x, side * size * 0.55, top_z - 3.0), rotator(yaw=side * 35.0), (1.2, 0.35, 0.22)), size, 4, 6)
# Heart mark up front, spinnerets at the back.
abdomen.ball(MARK_SLOT, abdomen_frame @ transform((-14.0, 0.0, 44.0), scale=(1.1, 0.3, 0.2)), 14.0, 4, 6)
abdomen.cone(MARK_SLOT, abdomen_frame @ transform((-110.0, 0.0, 8.0), make_from_z((-1.0, 0.0, -0.2))), 6.0, 1.5, 10.0, 6)
abdomen.displace(1.2, 1.0 / 25.0, 7.0)
bones.append(('abdomen', body @ abdomen_pivot, body @ (abdomen_pivot + Vector((-50.0, 0.0, 12.0))), 'body'))

# Legs, standing: each foot on the ground at its resting spot.
random = RandomStream(1847)
for pair in range(4):
    for side, suffix in SIDES:
        hip_angle, rest_angle = math.radians(HIP_ANGLE[pair]), math.radians(REST_ANGLE[pair])
        hip = Vector((math.cos(hip_angle) * 30.0, side * math.sin(hip_angle) * 26.0, RIDE_HEIGHT - 2.0))
        rest = Vector((math.cos(rest_angle) * REST_RADIUS[pair], side * math.sin(rest_angle) * REST_RADIUS[pair], 0.0))
        pole = knee_pole(hip)
        knee, foot = solve_two_bone(hip, rest, FEMUR_LENGTH[pair], TIBIA_LENGTH[pair], pole)
        leg = f'{pair}_{suffix}'
        femur = parts[f'femur_{leg}'] = Part(f'Femur_{leg}', 50.0)
        build_femur(femur, transform(hip, make_from_xz(knee - hip, pole)), FEMUR_LENGTH[pair], random)
        tibia = parts[f'tibia_{leg}'] = Part(f'Tibia_{leg}', 50.0)
        build_tibia(tibia, transform(knee, make_from_xz(foot - knee, pole)), TIBIA_LENGTH[pair], random)
        bones += [(f'femur_{leg}', hip, knee, 'body'), (f'tibia_{leg}', knee, foot, f'femur_{leg}'),
                  (f'foot_{leg}', foot, foot + (foot - knee).normalized() * 8.0, f'tibia_{leg}')]

rig = lm.armature()
lm.bones(rig, [(name, to_blender(start), to_blender(end), parent) for name, start, end, parent in bones])
skin = {}
for bone, part in parts.items():
    obj = part.build(materials)
    lm.hit_hull(rig, bone, [obj])
    skin[bone] = [obj]
lm.skin(rig, 'Spider', skin)
