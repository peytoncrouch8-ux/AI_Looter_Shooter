"""Helpers for scripted models (Art/Models/<Category>/<Name>.py). models.ps1 runs a model script in an empty scene and
then exports it; see Art/README.md.

Distances are meters. Colors are sRGB hex (0xRRGGBB), as picked, like the game's code. Build every part around the
world origin: the model's origin is its pivot in Unreal, and its front faces -Y (Blender's Front view).
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector


def hex_color(value):
    """0xRRGGBB in sRGB to linear RGBA."""
    def channel(byte):
        c = byte / 255.0
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    return [channel((value >> 16) & 0xFF), channel((value >> 8) & 0xFF), channel(value & 0xFF), 1.0]


def material(name, color, kind='Surface', **look):
    """A material with the game's stylized look. kind is Surface, Foliage (two-sided) or Glow (additive). look takes
    FStylizedSurface's settings by name (TopColor, TopBlend, GradHeight in meters, Glow...); colors may be hex."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    rgba = hex_color(color) if isinstance(color, int) else list(color)
    bsdf = next(node for node in mat.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = rgba
    mat.diffuse_color = rgba
    mat['Kind'] = kind
    for key, value in look.items():
        mat[key] = hex_color(value) if key.endswith('Color') and isinstance(value, int) else value
    return mat


def _new_bmesh():
    bm = bmesh.new()
    bm.loops.layers.uv.new('UVMap')  # the primitives only fill in UVs when the layer exists
    return bm


def _link(name, bm, mat, parent):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    if mat is not None:
        mesh.materials.append(mat)
    if parent is not None:
        obj.parent = parent
    return obj


def box(name, size, center=(0.0, 0.0, 0.0), material=None, parent=None, bevel=0.0):
    """A box of size (x, y, z) centered on center, with chamfered edges if bevel > 0."""
    bm = _new_bmesh()
    bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.LocRotScale(Vector(center), None, Vector(size)), calc_uvs=True)
    if bevel > 0.0:
        bmesh.ops.bevel(bm, geom=list(bm.edges), offset=bevel, segments=1, affect='EDGES', clamp_overlap=True)
    return _link(name, bm, material, parent)


def cylinder(name, radius, height, center=(0.0, 0.0, 0.0), segments=8, material=None, parent=None, top_radius=None):
    """An upright cylinder (or a cone, with top_radius) centered on center."""
    bm = _new_bmesh()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments, radius1=radius,
                          radius2=radius if top_radius is None else top_radius, depth=height,
                          matrix=Matrix.Translation(Vector(center)), calc_uvs=True)
    return _link(name, bm, material, parent)


def smooth(obj, angle=40.0):
    """Soft shading, with hard edges where faces meet at more than angle degrees."""
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    obj.data.set_sharp_from_angle(angle=math.radians(angle))


def hull_box(parent, size, center=(0.0, 0.0, 0.0)):
    """A box collision hull for the model parent. Hulls must be convex; use several for other shapes."""
    hull = box('UCX_' + parent.name, size, center, parent=parent)
    hull.display_type = 'WIRE'
    return hull


def socket(parent, name, location=(0.0, 0.0, 0.0), rotation=(0.0, 0.0, 0.0)):
    """An attach point called name on the model parent (rotation in degrees)."""
    empty = bpy.data.objects.new('SOCKET_' + name, None)
    empty.empty_display_type = 'ARROWS'
    empty.empty_display_size = 0.15
    bpy.context.scene.collection.objects.link(empty)
    empty.parent = parent
    empty.location = location
    empty.rotation_euler = [math.radians(a) for a in rotation]
    return empty
