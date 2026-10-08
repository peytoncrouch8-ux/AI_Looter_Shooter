"""A stand-in for Unreal's `unreal` Python module, enough for Tools/Unreal/build_area.py and its build_area_*.py helpers
to run under plain Python (the pipeline's "dry runs with a mocked unreal" idea), so the Style Lab gets the level's
placements exactly as the editor build makes them. It records every actor the build spawns (class, label, folder,
tags, transform, components and their meshes, materials set on slots, instanced meshes, editor properties) and answers
the build's questions with real geometry:

- static meshes are the lab's exported models (Saved/StyleLab/work/models/<Name>.json/.npz: bounds, sockets, material
  slots, triangles in Unreal's local cm) and the terrain's pieces (Saved/StyleLab/work/terrain_pieces.npz);
- traces (line_trace_component, SystemLibrary.line_trace_single) and capsule overlaps meet those triangles (complex
  collision), through each component's world transform, with mathutils' BVH trees;
- sockets, bounds and attachments follow Unreal's rules (FRotationMatrix, scale before rotation before translation).

Install it before importing the build: sys.modules['unreal'] = mock_unreal.
"""
import json
import math
import os
import sys

import bpy  # noqa: F401  (the bpy module brings mathutils)
import numpy as np
from mathutils import Vector as _BV
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
WORK = os.path.join(REPO, 'Saved', 'StyleLab', 'work')
LOG = []


# ---------------------------------------------------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------------------------------------------------

def log(message):
    LOG.append(('log', str(message)))


def log_warning(message):
    LOG.append(('warning', str(message)))


def log_error(message):
    LOG.append(('error', str(message)))


# ---------------------------------------------------------------------------------------------------------------------
# Value types
# ---------------------------------------------------------------------------------------------------------------------

class Name(str):
    pass


class Text(str):
    pass


class Vector:
    __slots__ = ('x', 'y', 'z')

    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.x, self.y, self.z = float(x), float(y), float(z)

    def __repr__(self):
        return f'Vector({self.x:.2f}, {self.y:.2f}, {self.z:.2f})'

    def __add__(self, o):
        return Vector(self.x + o.x, self.y + o.y, self.z + o.z)

    def __sub__(self, o):
        return Vector(self.x - o.x, self.y - o.y, self.z - o.z)

    def __neg__(self):
        return Vector(-self.x, -self.y, -self.z)

    def __mul__(self, o):
        if isinstance(o, Vector):
            return Vector(self.x * o.x, self.y * o.y, self.z * o.z)
        return Vector(self.x * o, self.y * o, self.z * o)

    __rmul__ = __mul__

    def __truediv__(self, o):
        if isinstance(o, Vector):
            return Vector(self.x / o.x, self.y / o.y, self.z / o.z)
        return Vector(self.x / o, self.y / o, self.z / o)

    def __eq__(self, o):
        return isinstance(o, Vector) and (self.x, self.y, self.z) == (o.x, o.y, o.z)

    def __hash__(self):
        return hash((self.x, self.y, self.z))

    def length(self):
        return math.sqrt(self.x * self.x + self.y * self.y + self.z * self.z)

    size = length

    def length_squared(self):
        return self.x * self.x + self.y * self.y + self.z * self.z

    def size_squared(self):
        return self.length_squared()

    def normal(self, tolerance=1e-8):
        n = self.length()
        return Vector() if n < tolerance else self / n

    get_safe_normal = normal
    normalized = normal

    def normalize(self, tolerance=1e-8):
        n = self.length()
        if n < tolerance:
            return False
        self.x, self.y, self.z = self.x / n, self.y / n, self.z / n
        return True

    def dot(self, o):
        return self.x * o.x + self.y * o.y + self.z * o.z

    def cross(self, o):
        return Vector(self.y * o.z - self.z * o.y, self.z * o.x - self.x * o.z, self.x * o.y - self.y * o.x)

    def distance(self, o):
        return (self - o).length()

    def to_tuple(self):
        return (self.x, self.y, self.z)

    def copy(self):
        return Vector(self.x, self.y, self.z)

    def a(self):
        return np.array([self.x, self.y, self.z])


class Vector2D:
    def __init__(self, x=0.0, y=0.0):
        self.x, self.y = float(x), float(y)


class Vector4:
    def __init__(self, x=0.0, y=0.0, z=0.0, w=0.0):
        self.x, self.y, self.z, self.w = x, y, z, w


class LinearColor:
    def __init__(self, r=0.0, g=0.0, b=0.0, a=1.0):
        self.r, self.g, self.b, self.a = float(r), float(g), float(b), float(a)

    def to_tuple(self):
        return (self.r, self.g, self.b, self.a)


class Color:
    def __init__(self, r=0, g=0, b=0, a=255):
        self.r, self.g, self.b, self.a = r, g, b, a


def rot_matrix(roll, pitch, yaw):
    """FRotationMatrix as a numpy 3x3 whose columns are the rotated X, Y, Z axes."""
    sp, cp = math.sin(math.radians(pitch)), math.cos(math.radians(pitch))
    sy, cy = math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
    sr, cr = math.sin(math.radians(roll)), math.cos(math.radians(roll))
    x = (cp * cy, cp * sy, sp)
    y = (sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp)
    z = (-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp)
    return np.array([x, y, z]).T


def matrix_rotator(m):
    """FMatrix::Rotator: (roll, pitch, yaw) of a rotation matrix (columns X, Y, Z)."""
    x, y, z = m[:, 0], m[:, 1], m[:, 2]
    pitch = math.degrees(math.atan2(x[2], math.sqrt(x[0] ** 2 + x[1] ** 2)))
    yaw = math.degrees(math.atan2(x[1], x[0]))
    sy_axis = rot_matrix(0.0, pitch, yaw)[:, 1]
    roll = math.degrees(math.atan2(float(np.dot(z, sy_axis)), float(np.dot(y, sy_axis))))
    return roll, pitch, yaw


class Rotator:
    __slots__ = ('roll', 'pitch', 'yaw')

    def __init__(self, roll=0.0, pitch=0.0, yaw=0.0):
        self.roll, self.pitch, self.yaw = float(roll), float(pitch), float(yaw)

    def __repr__(self):
        return f'Rotator(roll={self.roll:.2f}, pitch={self.pitch:.2f}, yaw={self.yaw:.2f})'

    def matrix(self):
        return rot_matrix(self.roll, self.pitch, self.yaw)

    def get_forward_vector(self):
        return Vector(*self.matrix()[:, 0])

    def get_right_vector(self):
        return Vector(*self.matrix()[:, 1])

    def get_up_vector(self):
        return Vector(*self.matrix()[:, 2])

    def rotate_vector(self, v):
        return Vector(*(self.matrix() @ v.a()))

    def unrotate_vector(self, v):
        return Vector(*(self.matrix().T @ v.a()))

    def to_tuple(self):
        return (self.roll, self.pitch, self.yaw)

    def rotator(self):
        return self

    def quaternion(self):
        return Quat.from_matrix(self.matrix())

    def __add__(self, o):
        return Rotator(self.roll + o.roll, self.pitch + o.pitch, self.yaw + o.yaw)


class Quat:
    """A rotation kept as its matrix (Transform.rotation)."""

    def __init__(self, x=0.0, y=0.0, z=0.0, w=1.0):
        self.m = np.eye(3)
        self._xyzw = (x, y, z, w)

    @staticmethod
    def from_matrix(m):
        q = Quat()
        q.m = np.array(m, dtype=np.float64)
        return q

    def rotator(self):
        return Rotator(*matrix_rotator(self.m))

    def get_forward_vector(self):
        return Vector(*self.m[:, 0])

    def get_right_vector(self):
        return Vector(*self.m[:, 1])

    def get_up_vector(self):
        return Vector(*self.m[:, 2])

    def rotate_vector(self, v):
        return Vector(*(self.m @ v.a()))

    def unrotate_vector(self, v):
        return Vector(*(self.m.T @ v.a()))


def _rot_m(rotation):
    if rotation is None:
        return np.eye(3)
    if isinstance(rotation, Rotator):
        return rotation.matrix()
    if isinstance(rotation, Quat):
        return rotation.m
    return np.eye(3)


class Transform:
    def __init__(self, location=None, rotation=None, scale=None):
        self.translation = location.copy() if isinstance(location, Vector) else Vector()
        self.rotation = Quat.from_matrix(_rot_m(rotation))
        self.scale3d = scale.copy() if isinstance(scale, Vector) else Vector(1.0, 1.0, 1.0)

    @staticmethod
    def from_matrix(m):
        """From a 4x4 (scale x rotation, no shear)."""
        t = Transform()
        cols = m[:3, :3]
        s = np.linalg.norm(cols, axis=0)
        r = cols / np.where(s > 1e-12, s, 1.0)
        if np.linalg.det(r) < 0.0:   # a mirror: carried by the Y scale, as the build's mirrored cliff pieces do
            s[1] = -s[1]
            r[:, 1] = -r[:, 1]
        t.translation = Vector(*m[:3, 3])
        t.rotation = Quat.from_matrix(r)
        t.scale3d = Vector(*s)
        return t

    def matrix(self):
        m = np.eye(4)
        m[:3, :3] = self.rotation.m @ np.diag([self.scale3d.x, self.scale3d.y, self.scale3d.z])
        m[:3, 3] = self.translation.a()
        return m

    def transform_location(self, v):
        return Vector(*(self.matrix() @ np.array([v.x, v.y, v.z, 1.0]))[:3])

    def transform_direction(self, v):
        return Vector(*(self.matrix()[:3, :3] @ v.a()))

    def inverse_transform_location(self, v):
        return Vector(*(np.linalg.inv(self.matrix()) @ np.array([v.x, v.y, v.z, 1.0]))[:3])

    def inverse(self):
        return Transform.from_matrix(np.linalg.inv(self.matrix()))

    def __mul__(self, other):
        # Unreal: A * B applies A first, then B.
        return Transform.from_matrix(other.matrix() @ self.matrix())

    def to_tuple(self):
        return (self.translation, self.rotation, self.scale3d)


class Box:
    def __init__(self, lo, hi):
        self.min, self.max = lo, hi


class _Enum:
    """Enum namespaces (unreal.TraceTypeQuery.TRACE_TYPE_QUERY1 ...): any member is its own name."""

    def __init__(self, name):
        self._name = name

    def __getattr__(self, item):
        if item.startswith('__'):
            raise AttributeError(item)
        return f'{self._name}.{item}'


class Struct:
    """Unreal structs the build fills in (StoryCondition, EncounterGroup, ...): editor properties in a dict."""

    def __init__(self, *args, **kwargs):
        self.props = dict(kwargs)
        self.args = args

    def set_editor_property(self, name, value, *args):
        self.props[name] = value

    def get_editor_property(self, name):
        return self.props.get(name, Struct())

    def __getattr__(self, item):
        if item.startswith('__') or item in ('props', 'args'):
            raise AttributeError(item)
        return self.props.get(item, 0.0)


# ---------------------------------------------------------------------------------------------------------------------
# Assets
# ---------------------------------------------------------------------------------------------------------------------

class Object:
    def get_name(self):
        return getattr(self, '_name', type(self).__name__)

    def get_path_name(self):
        return getattr(self, '_path', '/Script/' + self.get_name())

    def get_class(self):
        return type(self)

    def set_editor_property(self, name, value, *args):
        self.__dict__.setdefault('_props', {})[name] = value

    def get_editor_property(self, name):
        return self.__dict__.setdefault('_props', {}).get(name, Struct())


class Socket:
    def __init__(self, name, info):
        self.socket_name = Name(name)
        self.info = info

    def matrix(self):
        x = np.array(self.info['ueForward'], dtype=np.float64)
        z = np.array(self.info['ueUp'], dtype=np.float64)
        x /= max(np.linalg.norm(x), 1e-9)
        z = z - x * np.dot(z, x)
        z /= max(np.linalg.norm(z), 1e-9)
        y = np.cross(z, x)
        m = np.eye(4)
        m[:3, :3] = np.column_stack([x, y, z])
        m[:3, 3] = self.info['ue']
        return m


class StaticMesh(Object):
    _bvh = None

    def __init__(self, name, path, rec, tris_loader):
        self._name = 'SM_' + name
        self.model = name
        self._path = f'{path}.SM_{name}'
        self.rec = rec
        self._tris_loader = tris_loader
        self._tris = None
        lo, hi = rec['ueBounds']
        self.box = Box(Vector(*lo), Vector(*hi))
        self.sockets = {k: Socket(k, v) for k, v in rec.get('sockets', {}).items()}
        self.slots = rec.get('slots', [])

    def tris(self):
        if self._tris is None:
            self._tris = self._tris_loader()
        return self._tris

    def bvh(self):
        if self._bvh is None:
            t = self.tris()
            if len(t) == 0:
                self._bvh = False
            else:
                verts = t.reshape(-1, 3).astype(np.float64).tolist()
                polys = np.arange(len(verts)).reshape(-1, 3).tolist()
                self._bvh = BVHTree.FromPolygons(verts, polys, epsilon=0.0)
        return self._bvh or None

    def get_bounding_box(self):
        return Box(self.box.min.copy(), self.box.max.copy())

    def get_bounds(self):
        lo, hi = self.box.min, self.box.max
        return Struct(origin=(lo + hi) * 0.5, box_extent=(hi - lo) * 0.5)

    def find_socket(self, name):
        return self.sockets.get(str(name))

    def get_all_socket_names(self):
        return [Name(k) for k in self.sockets]

    def get_num_sections(self, lod=0):
        return len(self.slots)

    def get_material(self, slot):
        return MaterialInstance(self.slots[slot]) if 0 <= slot < len(self.slots) else None


class MaterialInstance(Object):
    def __init__(self, name):
        self._name = name if name.startswith(('MI_', 'M_')) else 'MI_' + name
        self._path = f'/Game/Art/Materials/{self._name}.{self._name}'

    def __eq__(self, other):
        return isinstance(other, MaterialInstance) and other._name == self._name

    def __hash__(self):
        return hash(self._name)


class Asset(Object):
    def __init__(self, path):
        self._path = path
        self._name = path.rsplit('/', 1)[-1].split('.')[0]


class MaterialInstanceConstant(MaterialInstance):
    pass


class _Registry:
    """The meshes the build can find: the exported models and the terrain's pieces, by Unreal path."""

    def __init__(self):
        self.meshes = {}
        self.by_name = {}
        self.loaded = False

    def load(self):
        if self.loaded:
            return
        self.loaded = True
        models = os.path.join(WORK, 'models')
        for fname in sorted(os.listdir(models)) if os.path.isdir(models) else []:
            if not fname.endswith('.json') or fname.startswith('_'):
                continue
            name = fname[:-5]
            with open(os.path.join(models, fname), encoding='utf-8') as f:
                rec = json.load(f)
            if rec.get('skinned') or name.startswith('Gun_'):
                continue   # skeletal meshes and assembled guns aren't static mesh assets
            category = rec.get('source', 'Props/x.py').split('/')[0]
            path = f'/Game/Art/{category}/SM_{name}'
            npz = os.path.join(models, name + '.npz')
            mesh = StaticMesh(name, path, rec, lambda p=npz, t=rec.get('traces', 'mesh'): _trace_tris(p, t))
            self.meshes[path] = mesh
            self.by_name[name] = mesh
        pieces = os.path.join(WORK, 'terrain_pieces.npz')
        if os.path.exists(pieces):
            data = np.load(pieces)
            for name in sorted(data.files):
                tris = data[name]
                lo, hi = tris.reshape(-1, 3).min(0), tris.reshape(-1, 3).max(0)
                rec = {'ueBounds': [lo.tolist(), hi.tolist()], 'sockets': {}, 'slots': ['Terrain']}
                path = f'/Game/Art/Terrain/SM_{name}'
                mesh = StaticMesh(name, path, rec, lambda t=tris: t)
                self.meshes[path] = mesh
                self.by_name[name] = mesh

    def get(self, path):
        self.load()
        p = path.split('.')[0]
        return self.meshes.get(p)


def _trace_tris(npz, traces):
    """What a model's traces meet (model_export.py's 'traces'): its render mesh, its hulls (plants), or nothing."""
    if traces == 'none':
        return np.zeros((0, 3, 3), np.float32)
    data = np.load(npz)
    if traces == 'hulls' and 'hulls' in data.files:
        return data['hulls']
    return data['tris']


REGISTRY = _Registry()


# ---------------------------------------------------------------------------------------------------------------------
# Components and actors
# ---------------------------------------------------------------------------------------------------------------------

WORLD = None


class ActorComponent(Object):
    def __init__(self, owner=None, name=''):
        self.owner = owner
        self._name = name or type(self).__name__
        self.relative = np.eye(4)
        self.parent = None
        self.parent_socket = ''
        self.collision = True
        self.props = {}

    def get_owner(self):
        return self.owner

    def set_editor_property(self, name, value, *args):
        if name == 'static_mesh':
            self.set_static_mesh(value)
            return
        if name == 'relative_location':
            self.set_relative_location(value, False, True)
            return
        self.props[name] = value

    def get_editor_property(self, name):
        if name == 'static_mesh':
            return getattr(self, 'static_mesh', None)
        if name in self.props:
            return self.props[name]
        if name == 'lighting_channels':
            self.props[name] = Struct()
            return self.props[name]
        return Struct()


class SceneComponent(ActorComponent):
    def world_matrix(self):
        if self.parent is None:
            return self.relative.copy()
        base = self.parent.world_matrix()
        if self.parent_socket and isinstance(self.parent, StaticMeshComponent):
            sock = self.parent.socket_matrix(self.parent_socket)
            if sock is not None:
                base = base @ sock
        return base @ self.relative

    def _set_world(self, m):
        if self.parent is None:
            self.relative = m
        else:
            base = self.parent.world_matrix()
            if self.parent_socket and isinstance(self.parent, StaticMeshComponent):
                sock = self.parent.socket_matrix(self.parent_socket)
                if sock is not None:
                    base = base @ sock
            self.relative = np.linalg.inv(base) @ m
        WORLD.touch()

    def get_world_transform(self):
        return Transform.from_matrix(self.world_matrix())

    def get_world_location(self):
        return Vector(*self.world_matrix()[:3, 3])

    def get_world_rotation(self):
        return self.get_world_transform().rotation.rotator()

    def get_world_scale(self):
        return self.get_world_transform().scale3d

    def get_forward_vector(self):
        return self.get_world_transform().rotation.get_forward_vector()

    def get_right_vector(self):
        return self.get_world_transform().rotation.get_right_vector()

    def get_up_vector(self):
        return self.get_world_transform().rotation.get_up_vector()

    def _relative_parts(self):
        t = Transform.from_matrix(self.relative)
        return t.translation, t.rotation.m, t.scale3d

    def _compose(self, loc, rot_m, scale):
        m = np.eye(4)
        m[:3, :3] = rot_m @ np.diag([scale.x, scale.y, scale.z])
        m[:3, 3] = loc.a()
        self.relative = m
        WORLD.touch()

    def set_relative_location(self, location, sweep=False, teleport=True):
        _, r, s = self._relative_parts()
        self._compose(location, r, s)

    def set_relative_rotation(self, rotation, sweep=False, teleport=True):
        t, _, s = self._relative_parts()
        self._compose(t, _rot_m(rotation), s)

    def set_relative_scale3d(self, scale):
        t, r, _ = self._relative_parts()
        self._compose(t, r, scale)

    def set_relative_transform(self, transform, sweep=False, teleport=True):
        self.relative = transform.matrix()
        WORLD.touch()

    def get_relative_location(self):
        return self._relative_parts()[0]

    def set_world_location(self, location, sweep=False, teleport=True):
        m = self.world_matrix()
        m[:3, 3] = location.a()
        self._set_world(m)

    def set_world_rotation(self, rotation, sweep=False, teleport=True):
        t = self.get_world_transform()
        self._set_world(Transform(t.translation, rotation, t.scale3d).matrix())

    def set_world_location_and_rotation(self, location, rotation, sweep=False, teleport=True):
        t = self.get_world_transform()
        self._set_world(Transform(location, rotation, t.scale3d).matrix())

    def set_world_scale3d(self, scale):
        t = self.get_world_transform()
        self._set_world(Transform(t.translation, t.rotation, scale).matrix())

    def set_world_transform(self, transform, sweep=False, teleport=True):
        self._set_world(transform.matrix())

    def attach_to_component(self, parent, socket_name='', location_rule=None, rotation_rule=None, scale_rule=None,
                            weld=False):
        world = self.world_matrix()
        self.parent = parent
        self.parent_socket = str(socket_name or '')
        keep_world = str(location_rule).endswith('KEEP_WORLD')
        if str(location_rule).endswith('SNAP_TO_TARGET'):
            self.relative = np.eye(4)
        elif keep_world:
            self._set_world(world)
        WORLD.touch()
        return True

    def k2_attach_to_component(self, *args, **kwargs):
        return self.attach_to_component(*args, **kwargs)

    def detach_from_component(self, *args, **kwargs):
        world = self.world_matrix()
        self.parent = None
        self.parent_socket = ''
        self.relative = world

    def set_mobility(self, mobility):
        self.props['mobility'] = mobility

    def set_visibility(self, visible, propagate=False):
        self.props['visible'] = visible

    def set_hidden_in_game(self, hidden, propagate=False):
        self.props['hidden_in_game'] = hidden

    def set_collision_enabled(self, value):
        self.collision = not str(value).endswith('NO_COLLISION')

    def set_collision_profile_name(self, name, update=True):
        self.props['collision_profile'] = str(name)
        if str(name) == 'NoCollision':
            self.collision = False

    def get_collision_response_to_channel(self, channel):
        return 'CollisionResponseType.ECR_BLOCK' if self.collision else 'CollisionResponseType.ECR_IGNORE'

    def get_socket_transform(self, name, space=None):
        m = self.world_matrix()
        return Transform.from_matrix(m)

    def get_socket_location(self, name):
        return self.get_socket_transform(name).translation

    def does_socket_exist(self, name):
        return False


class PrimitiveComponent(SceneComponent):
    pass


class StaticMeshComponent(PrimitiveComponent):
    def __init__(self, owner=None, name=''):
        super().__init__(owner, name)
        self.static_mesh = None
        self.materials = {}

    def set_static_mesh(self, mesh):
        self.static_mesh = mesh if isinstance(mesh, StaticMesh) else None
        WORLD.touch()
        return True

    def get_static_mesh(self):
        return self.static_mesh

    def socket_matrix(self, name):
        if self.static_mesh is None:
            return None
        sock = self.static_mesh.find_socket(name)
        return None if sock is None else sock.matrix()

    def does_socket_exist(self, name):
        return self.static_mesh is not None and self.static_mesh.find_socket(str(name)) is not None

    def get_socket_transform(self, name, space=None):
        m = self.world_matrix()
        sock = self.socket_matrix(str(name))
        if sock is not None:
            m = m @ sock
        return Transform.from_matrix(m)

    def get_all_socket_names(self):
        return self.static_mesh.get_all_socket_names() if self.static_mesh else []

    def set_material(self, slot, material):
        self.materials[int(slot)] = material

    def get_material(self, slot):
        if int(slot) in self.materials:
            return self.materials[int(slot)]
        if self.static_mesh is not None and 0 <= slot < len(self.static_mesh.slots):
            return MaterialInstance(self.static_mesh.slots[slot])
        return None

    def get_num_materials(self):
        return len(self.static_mesh.slots) if self.static_mesh else 0

    def get_material_index(self, name):
        if self.static_mesh is None:
            return -1
        try:
            return self.static_mesh.slots.index(str(name))
        except ValueError:
            return -1

    def get_material_slot_names(self):
        return [Name(s) for s in (self.static_mesh.slots if self.static_mesh else [])]

    def colliders(self):
        """(world matrix, mesh) pairs this component traces against."""
        if self.static_mesh is None or not self.collision or not self.owner.collision:
            return []
        return [(self.world_matrix(), self.static_mesh)]

    def line_trace_component(self, start, end, trace_complex=True, show=False, shapes=False):
        best = None
        for m, mesh in self.colliders():
            hit = _ray_mesh(m, mesh, start, end)
            if hit is not None and (best is None or hit[0] < best[0]):
                best = hit
        if best is None:
            return None
        _, loc, nrm = best
        result = HitResult(loc, nrm, self.owner, self, start, end)
        return (loc, nrm, Name('None'), result)


class InstancedStaticMeshComponent(StaticMeshComponent):
    def __init__(self, owner=None, name=''):
        super().__init__(owner, name)
        self.instances = []

    def add_instance(self, transform, world_space=False):
        self.instances.append(transform.matrix())
        WORLD.touch()
        return len(self.instances) - 1

    def colliders(self):
        if self.static_mesh is None or not self.collision or not self.owner.collision:
            return []
        base = self.world_matrix()
        return [(base @ m, self.static_mesh) for m in self.instances]


class HierarchicalInstancedStaticMeshComponent(InstancedStaticMeshComponent):
    pass


class LightComponent(SceneComponent):
    pass


class PointLightComponent(LightComponent):
    pass


class SpotLightComponent(LightComponent):
    pass


class DirectionalLightComponent(LightComponent):
    pass


class SkyLightComponent(SceneComponent):
    def recapture_sky(self):
        pass


class SkyAtmosphereComponent(SceneComponent):
    pass


class ExponentialHeightFogComponent(SceneComponent):
    pass


class DecalComponent(SceneComponent):
    pass


class BoxComponent(PrimitiveComponent):
    pass


class HitResult:
    def __init__(self, location, normal, actor, component, start, end):
        self.location, self.normal, self.actor, self.component = location, normal, actor, component
        self.start, self.end = start, end

    def to_tuple(self):
        d = (self.location - self.start).length()
        total = max((self.end - self.start).length(), 1e-6)
        return (True, False, d / total, d, self.location, self.location, self.normal, self.normal, None, self.actor,
                self.component, Name('None'), Name('None'), -1, -1, -1, self.start, self.end)


# Which component names a C++ actor exposes that the build reads with get_editor_property (anything else reads as a
# struct), and the meshes its constructor gives them (Source/: the class's .cpp).
COMPONENT_NAMES = {'tower', 'fan', 'rack', 'sac', 'post', 'leaf', 'bell', 'lamp', 'mesh', 'keepers_lantern',
                   'hearse_car', 'snare', 'lantern', 'light', 'body', 'lid', 'door', 'passenger_car', 'locomotive',
                   'flame', 'plume', 'decal', 'building'}
VALUE_DEFAULTS = {'spawn_points': [], 'landing_name': Name(''), 'other_absorption_scale': 1.0, 'burst_out': 0.0,
                  'drop_height': 0.0}


class Actor(Object):
    def __init__(self, cls_name='Actor'):
        self._name = cls_name
        self.label = cls_name
        self.folder = ''
        self.tags = []
        self.collision = True
        self.hidden = False
        self.props = {}
        self.components = []
        self.root_component = self._make_root()
        self.instance_sets = []   # (mesh, [4x4]) from set_instances
        self.destroyed = False

    def _make_root(self):
        root = SceneComponent(self, 'Root')
        self.components.append(root)
        return root

    @classmethod
    def get_name(cls):
        return cls.__name__

    @classmethod
    def static_class(cls):
        return cls

    def get_class(self):
        return type(self)

    # --- labels, tags, folders ---
    def set_actor_label(self, label, mark_dirty=True):
        self.label = str(label)

    def get_actor_label(self):
        return self.label

    def set_folder_path(self, path):
        self.folder = str(path)

    def get_folder_path(self):
        return Name(self.folder)

    def set_editor_property(self, name, value, *args):
        if name == 'tags':
            self.tags = [Name(t) for t in value]
            return
        self.props[name] = value
        if name == 'building_mesh' and isinstance(value, StaticMesh):
            # ATrainStation shows its building_mesh on its own component at the actor's origin (TrainStation.cpp).
            comp = next((c for c in self.components if c._name == 'Building'), None)
            if comp is None:
                comp = StaticMeshComponent(self, 'Building')
                comp.parent = self.root_component
                self.components.append(comp)
            comp.set_static_mesh(value)

    def get_editor_property(self, name):
        if name == 'tags':
            return list(self.tags)
        if name == 'root_component':
            return self.root_component
        if name in self.props:
            return self.props[name]
        if name in COMPONENT_NAMES or name not in VALUE_DEFAULTS:
            # Mostly a component the C++ made (a chest's body, lid and wheel, a sac, a bell); a struct the build
            # fills in works the same way (editor properties).
            comp = StaticMeshComponent(self, name)
            comp.parent = self.root_component
            self.components.append(comp)
            self.props[name] = comp
            return comp
        if name in ('speaker_point', 'boss'):
            comp = SceneComponent(self, name)
            comp.parent = self.root_component
            self.components.append(comp)
            self.props[name] = comp
            return comp
        if name in VALUE_DEFAULTS:
            return VALUE_DEFAULTS[name]
        return Struct()

    # --- transforms ---
    def get_actor_location(self):
        return self.root_component.get_world_location()

    def get_actor_rotation(self):
        return self.root_component.get_world_rotation()

    def get_actor_scale3d(self):
        return self.root_component.get_world_scale()

    def get_actor_transform(self):
        return self.root_component.get_world_transform()

    def get_actor_forward_vector(self):
        return self.root_component.get_forward_vector()

    def get_actor_right_vector(self):
        return self.root_component.get_right_vector()

    def get_actor_up_vector(self):
        return self.root_component.get_up_vector()

    def set_actor_location(self, location, sweep=False, teleport=True):
        self.root_component.set_world_location(location)
        return True

    def set_actor_rotation(self, rotation, teleport=True):
        self.root_component.set_world_rotation(rotation)
        return True

    def set_actor_location_and_rotation(self, location, rotation, sweep=False, teleport=True):
        self.root_component.set_world_location_and_rotation(location, rotation)
        return True

    def set_actor_scale3d(self, scale):
        self.root_component.set_world_scale3d(scale)

    def set_actor_transform(self, transform, sweep=False, teleport=True):
        self.root_component.set_world_transform(transform)
        return True

    def set_actor_relative_location(self, *args):
        pass

    def add_actor_world_offset(self, delta, sweep=False, teleport=True):
        self.set_actor_location(self.get_actor_location() + delta)

    def set_actor_enable_collision(self, enabled):
        self.collision = bool(enabled)
        WORLD.touch()

    def set_actor_hidden_in_game(self, hidden):
        self.hidden = bool(hidden)

    def attach_to_actor(self, parent, socket_name='', location_rule=None, rotation_rule=None, scale_rule=None,
                        weld=False):
        self.root_component.attach_to_component(parent.root_component, socket_name, location_rule, rotation_rule,
                                                scale_rule, weld)

    # --- components ---
    def get_components_by_class(self, cls):
        return [c for c in self.components if isinstance(c, cls)]

    def get_component_by_class(self, cls):
        found = self.get_components_by_class(cls)
        return found[0] if found else None

    def get_actor_bounds(self, only_colliding=False, include_children=False):
        lo, hi = None, None
        for c in self.components:
            for m, mesh in _visual_parts(c):
                a, b = _world_aabb(m, mesh)
                lo = a if lo is None else np.minimum(lo, a)
                hi = b if hi is None else np.maximum(hi, b)
        if lo is None:
            at = self.get_actor_location()
            return at, Vector()
        return Vector(*((lo + hi) * 0.5)), Vector(*((hi - lo) * 0.5))

    def set_instances(self, mesh, transforms):
        comp = InstancedStaticMeshComponent(self, f'Instances_{len(self.instance_sets)}')
        comp.parent = self.root_component
        comp.set_static_mesh(mesh)
        comp.instances = [t.matrix() for t in transforms]
        self.components.append(comp)
        self.instance_sets.append(comp)
        WORLD.touch()

    def __getattr__(self, item):
        # A C++ function the build calls that the mock doesn't model (a setter, a refresh): recorded, returns None.
        if item.startswith('_') or item in ('static_mesh_component',):
            raise AttributeError(item)

        def call(*args, **kwargs):
            self.__dict__.setdefault('calls', []).append((item, args))
            return None
        return call


class StaticMeshActor(Actor):
    def _make_root(self):
        root = StaticMeshComponent(self, 'StaticMeshComponent0')
        self.static_mesh_component = root
        self.components.append(root)
        return root


def _visual_parts(component):
    if isinstance(component, InstancedStaticMeshComponent):
        if component.static_mesh is None:
            return []
        base = component.world_matrix()
        return [(base @ m, component.static_mesh) for m in component.instances]
    if isinstance(component, StaticMeshComponent) and component.static_mesh is not None:
        return [(component.world_matrix(), component.static_mesh)]
    return []


def _world_aabb(m, mesh):
    lo, hi = mesh.box.min.a(), mesh.box.max.a()
    corners = np.array([[x, y, z] for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])])
    w = corners @ m[:3, :3].T + m[:3, 3]
    return w.min(0), w.max(0)


def _ray_mesh(m, mesh, start, end):
    """(world distance, location, normal) where the segment start-end first meets mesh under world matrix m."""
    bvh = mesh.bvh()
    if bvh is None:
        return None
    inv = np.linalg.inv(m)
    s = inv @ np.array([start.x, start.y, start.z, 1.0])
    e = inv @ np.array([end.x, end.y, end.z, 1.0])
    d = e[:3] - s[:3]
    length = float(np.linalg.norm(d))
    if length < 1e-9:
        return None
    hit = bvh.ray_cast(_BV(s[:3]), _BV(d / length), length)
    if hit[0] is None:
        return None
    loc = m @ np.array([hit[0].x, hit[0].y, hit[0].z, 1.0])
    nrm = np.linalg.inv(m[:3, :3]).T @ np.array([hit[1].x, hit[1].y, hit[1].z])
    nrm /= max(np.linalg.norm(nrm), 1e-9)
    w = Vector(*loc[:3])
    return (w - start).length(), w, Vector(*nrm)


# The engine's own actor classes the build spawns by name.
class PlayerStart(Actor):
    pass


class TargetPoint(Actor):
    pass


class TriggerBox(Actor):
    pass


class PostProcessVolume(Actor):
    pass


class PCGVolume(Actor):
    pass


class CullDistanceVolume(Actor):
    pass


class DirectionalLight(Actor):
    def _make_root(self):
        root = DirectionalLightComponent(self, 'LightComponent0')
        self.components.append(root)
        return root


class SkyLight(Actor):
    def _make_root(self):
        root = SkyLightComponent(self, 'SkyLightComponent0')
        self.components.append(root)
        return root


class SkyAtmosphere(Actor):
    def _make_root(self):
        root = SkyAtmosphereComponent(self, 'SkyAtmosphereComponent')
        self.components.append(root)
        return root


class ExponentialHeightFog(Actor):
    def _make_root(self):
        root = ExponentialHeightFogComponent(self, 'HeightFogComponent0')
        self.components.append(root)
        return root


class PointLight(Actor):
    def _make_root(self):
        root = PointLightComponent(self, 'LightComponent0')
        self.components.append(root)
        return root


_CLASSES = {}


def _game_class(name):
    """A game C++ class (/Script/AI_Looter_Shooter.<Name>) as an Actor subclass of that name."""
    if name not in _CLASSES:
        _CLASSES[name] = type(name, (Actor,), {})
    return _CLASSES[name]


def load_class(outer, path):
    name = str(path).rsplit('.', 1)[-1]
    if str(path).startswith('/Script/'):
        return _game_class(name)
    return _game_class(name)


def get_default_object(cls):
    if isinstance(cls, type) and issubclass(cls, Actor):
        return cls.__new__(cls) if False else _DefaultObject(cls)
    return _DefaultObject(cls)


class _DefaultObject(Object):
    def __init__(self, cls):
        self.cls = cls

    def get_editor_property(self, name):
        return VALUE_DEFAULTS.get(name, Struct())


def load_asset(path, *args, **kwargs):
    path = str(path)
    mesh = REGISTRY.get(path)
    if mesh is not None:
        return mesh
    base = path.split('.')[0].rsplit('/', 1)[-1]
    if base.startswith(('MI_', 'M_')):
        return MaterialInstance(base)
    if base.startswith('SM_') or base.startswith('SK_'):
        return None
    return Asset(path)


def load_object(outer, path):
    return load_asset(path)


def find_object(outer, path):
    return load_asset(path)


# ---------------------------------------------------------------------------------------------------------------------
# The world
# ---------------------------------------------------------------------------------------------------------------------

class World(Object):
    def __init__(self):
        self.actors = []
        self.version = 0
        self._name = 'World'
        self.path = '/Game/Maps/Lvl_RansomsRest'
        self._colliders = None
        self._colliders_version = -1

    def touch(self):
        self.version += 1

    def get_path_name(self):
        return f'{self.path}.{self.path.rsplit("/", 1)[-1]}'

    def get_world_settings(self):
        return Struct()

    def colliders(self):
        """Every colliding (world matrix, mesh, actor, component, aabb lo, aabb hi), cached until something moves."""
        if self._colliders_version == self.version:
            return self._colliders
        out = []
        for actor in self.actors:
            if actor.destroyed or not actor.collision:
                continue
            for comp in actor.components:
                if not isinstance(comp, StaticMeshComponent) or not comp.collision:
                    continue
                for m, mesh in comp.colliders():
                    lo, hi = _world_aabb(m, mesh)
                    out.append((m, mesh, actor, comp, lo, hi))
        self._colliders = out
        self._colliders_lo = np.array([c[4] for c in out]) if out else np.zeros((0, 3))
        self._colliders_hi = np.array([c[5] for c in out]) if out else np.zeros((0, 3))
        self._colliders_version = self.version
        return out

    def trace(self, start, end, ignore=()):
        cols = self.colliders()
        if not cols:
            return None
        s, e = start.a(), end.a()
        lo = np.minimum(s, e) - 1.0
        hi = np.maximum(s, e) + 1.0
        near = np.nonzero(np.all(self._colliders_lo <= hi, axis=1) & np.all(self._colliders_hi >= lo, axis=1))[0]
        ignored = set(id(a) for a in ignore if a is not None)
        best = None
        for i in near:
            m, mesh, actor, comp, _, _ = cols[i]
            if id(actor) in ignored:
                continue
            hit = _ray_mesh(m, mesh, start, end)
            if hit is not None and (best is None or hit[0] < best[0]):
                best = (hit[0], hit[1], hit[2], actor, comp)
        if best is None:
            return None
        return HitResult(best[1], best[2], best[3], best[4], start, end)

    def capsule_overlap(self, centre, radius, half_height, ignore=()):
        cols = self.colliders()
        if not cols:
            return []
        c = centre.a()
        reach = np.array([radius, radius, half_height])
        lo, hi = c - reach, c + reach
        near = np.nonzero(np.all(self._colliders_lo <= hi, axis=1) & np.all(self._colliders_hi >= lo, axis=1))[0]
        ignored = set(id(a) for a in ignore if a is not None)
        found = []
        axis_half = max(half_height - radius, 0.0)
        samples = [c + np.array([0.0, 0.0, t]) for t in np.linspace(-axis_half, axis_half, 5)]
        for i in near:
            m, mesh, actor, comp, _, _ = cols[i]
            if id(actor) in ignored or comp in found:
                continue
            bvh = mesh.bvh()
            if bvh is None:
                continue
            inv = np.linalg.inv(m)
            scale = np.abs(np.linalg.svd(m[:3, :3], compute_uv=False))
            local_r = radius / max(scale.min(), 1e-6)
            for p in samples:
                lp = inv @ np.append(p, 1.0)
                hit = bvh.find_nearest(_BV(lp[:3]), local_r)
                if hit[0] is None:
                    continue
                w = m @ np.array([hit[0].x, hit[0].y, hit[0].z, 1.0])
                if np.linalg.norm(w[:3] - p) <= radius:
                    found.append(comp)
                    break
        return found


WORLD = World()


class _ActorSubsystem:
    def spawn_actor_from_object(self, obj, location, rotation=None, transient=False):
        if isinstance(obj, StaticMesh):
            actor = StaticMeshActor('StaticMeshActor')
            actor.static_mesh_component.set_static_mesh(obj)
        elif isinstance(obj, type) and issubclass(obj, Actor):
            return self.spawn_actor_from_class(obj, location, rotation)
        else:
            actor = Actor('Actor')
        actor.root_component.relative = Transform(location, rotation or Rotator()).matrix()
        WORLD.actors.append(actor)
        WORLD.touch()
        return actor

    def spawn_actor_from_class(self, cls, location, rotation=None, transient=False):
        if not (isinstance(cls, type) and issubclass(cls, Actor)):
            cls = Actor
        actor = cls(cls.__name__)
        actor.root_component.relative = Transform(location, rotation or Rotator()).matrix()
        WORLD.actors.append(actor)
        WORLD.touch()
        return actor

    def get_all_level_actors(self):
        return [a for a in WORLD.actors if not a.destroyed]

    def get_selected_level_actors(self):
        return []

    def destroy_actors(self, actors):
        for a in actors:
            a.destroyed = True
        WORLD.actors = [a for a in WORLD.actors if not a.destroyed]
        WORLD.touch()
        return True

    def destroy_actor(self, actor):
        return self.destroy_actors([actor])

    def set_actor_selection_state(self, *args):
        pass


class _LevelSubsystem:
    def new_level(self, path):
        WORLD.path = path
        return True

    def save_current_level(self):
        return True

    def load_level(self, path):
        return True


class _EditorSubsystem:
    def get_editor_world(self):
        return WORLD

    def get_game_world(self):
        return WORLD


class EditorActorSubsystem:
    pass


class LevelEditorSubsystem:
    pass


class UnrealEditorSubsystem:
    pass


_SUBSYSTEMS = {EditorActorSubsystem: _ActorSubsystem(), LevelEditorSubsystem: _LevelSubsystem(),
               UnrealEditorSubsystem: _EditorSubsystem()}


def get_editor_subsystem(cls):
    return _SUBSYSTEMS.get(cls) or _EditorSubsystem()


class EditorLevelLibrary:
    @staticmethod
    def get_editor_world():
        return WORLD

    @staticmethod
    def get_all_level_actors():
        return _SUBSYSTEMS[EditorActorSubsystem].get_all_level_actors()


class EditorAssetLibrary:
    @staticmethod
    def list_assets(path, recursive=True, include_folder=False):
        REGISTRY.load()
        return [f'{p}.{p.rsplit("/", 1)[-1]}' for p in sorted(REGISTRY.meshes) if p.startswith(str(path))]

    @staticmethod
    def does_asset_exist(path):
        path = str(path)
        if REGISTRY.get(path) is not None:
            return True
        base = path.split('.')[0].rsplit('/', 1)[-1]
        return not base.startswith(('SM_', 'SK_'))

    @staticmethod
    def load_asset(path):
        return load_asset(path)

    @staticmethod
    def load_blueprint_class(path):
        return _game_class(str(path).rsplit('/', 1)[-1].split('.')[0])

    @staticmethod
    def save_loaded_asset(*args, **kwargs):
        return True

    @staticmethod
    def save_asset(*args, **kwargs):
        return True


class EditorLoadingAndSavingUtils:
    @staticmethod
    def get_dirty_map_packages():
        return []

    @staticmethod
    def get_dirty_content_packages():
        return []

    @staticmethod
    def load_map(path):
        WORLD.path = str(path)
        return WORLD

    @staticmethod
    def save_dirty_packages(*args):
        return True


class Paths:
    @staticmethod
    def project_dir():
        return REPO + '/'

    @staticmethod
    def convert_relative_path_to_full(path):
        return os.path.abspath(path) + ('/' if str(path).endswith('/') else '')

    @staticmethod
    def project_saved_dir():
        return os.path.join(REPO, 'Saved') + '/'


class SystemLibrary:
    @staticmethod
    def line_trace_single(world, start, end, channel, trace_complex, ignore, draw, ignore_self=True, *args, **kwargs):
        return WORLD.trace(start, end, ignore or ())

    @staticmethod
    def line_trace_multi(world, start, end, *args, **kwargs):
        hit = WORLD.trace(start, end)
        return [hit] if hit else []

    @staticmethod
    def capsule_overlap_components(world, centre, radius, half_height, object_types=None, class_filter=None,
                                   ignore=None):
        return WORLD.capsule_overlap(centre, radius, half_height, ignore or ())


class MathLibrary:
    @staticmethod
    def make_rot_from_yz(y, z):
        ny = y.normal()
        nz = z.normal()
        nx = ny.cross(nz).normal()
        nz = nx.cross(ny)
        m = np.column_stack([nx.a(), ny.a(), nz.a()])
        return Rotator(*matrix_rotator(m))

    @staticmethod
    def make_rot_from_x(x):
        x = x.normal()
        return Rotator(0.0, math.degrees(math.asin(max(-1.0, min(1.0, x.z)))), math.degrees(math.atan2(x.y, x.x)))

    @staticmethod
    def find_look_at_rotation(start, target):
        return MathLibrary.make_rot_from_x(target - start)


class MaterialEditingLibrary:
    @staticmethod
    def set_material_instance_parent(instance, parent):
        instance.set_editor_property('parent', parent)

    @staticmethod
    def get_material_instance_vector_parameter_value(instance, name):
        return LinearColor(-1.0, -1.0, -1.0, -1.0)

    @staticmethod
    def get_material_instance_scalar_parameter_value(instance, name):
        return -1.0

    @staticmethod
    def set_material_instance_vector_parameter_value(instance, name, value):
        instance.set_editor_property(f'vector:{name}', value)
        return True

    @staticmethod
    def set_material_instance_scalar_parameter_value(instance, name, value):
        instance.set_editor_property(f'scalar:{name}', value)
        return True

    @staticmethod
    def update_material_instance(instance):
        return None


class _AssetTools:
    def create_asset(self, name, folder, cls, factory):
        return MaterialInstanceConstant(name)


class AssetToolsHelpers:
    @staticmethod
    def get_asset_tools():
        return _AssetTools()


class MaterialInstanceConstantFactoryNew:
    pass


class LooterLevelTools:
    @staticmethod
    def finish_asset_compilation():
        pass


# Enums and structs by name.
for _enum in ('TraceTypeQuery', 'DrawDebugTrace', 'RelativeTransformSpace', 'AttachmentRule', 'ComponentMobility',
              'CollisionEnabled', 'CollisionChannel', 'CollisionResponseType', 'ObjectTypeQuery', 'SkyLightSourceType',
              'SkyAtmosphereTransformMode', 'AutoExposureMethod', 'EncounterRankRoll', 'CreatureRank', 'ChestKind',
              'BossSealShape', 'WantedPosterVariant', 'DetachmentRule', 'TeleportType'):
    globals()[_enum] = _Enum(_enum)
for _struct in ('StoryCondition', 'EncounterGroup', 'SpeakerTopic', 'LightingState', 'StoryLightingRule',
                'CullDistanceSizePair', 'HuntingGround', 'HobPerch', 'HayBaleRemark', 'LightingChannels'):
    globals()[_struct] = type(_struct, (Struct,), {})


def __getattr__(name):
    """Anything else the build names: a game class (an Actor subclass) for capitalised names."""
    if name[:1].isupper():
        return _game_class(name)
    raise AttributeError(name)
