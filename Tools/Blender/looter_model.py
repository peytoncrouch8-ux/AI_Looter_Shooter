"""Helpers for scripted models (Art/Models/<Category>/<Name>.py). models.ps1 runs a model script in an empty scene and
then exports it; see Art/README.md.

Distances are meters. Colors are sRGB hex (0xRRGGBB), as picked, like the game's code. Build every part around the
world origin: the model's origin is its pivot in Unreal, and its front faces -Y (Blender's Front view).

A rigged model (a skeletal mesh in Unreal) is an armature() with bones(), its parts joined by skin(), and hit zones
from hit_sphere(), hit_capsule() and hit_hull().
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


# --- Rigged models (skeletal meshes) ---

def armature(name='root'):
    """The skeleton of a rigged model. Unreal takes the armature object itself as the root bone, hence the name."""
    arm = bpy.data.objects.new(name, bpy.data.armatures.new(name))
    bpy.context.scene.collection.objects.link(arm)
    return arm


def bones(arm, specs):
    """Adds bones to arm: specs is a list of (name, head, tail, parent name or None), positions in meters."""
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones
    for name, head, tail, parent in specs:
        bone = edit.new(name)
        bone.head = head
        bone.tail = tail
        bone.use_connect = False
        if parent is not None:
            bone.parent = edit[parent]
    bpy.ops.object.mode_set(mode='OBJECT')


def skin(arm, name, parts):
    """Joins parts ({bone name: [mesh objects]}) into one mesh called name, each part moving rigidly with its bone."""
    objects = []
    for bone, meshes in parts.items():
        for obj in meshes:
            group = obj.vertex_groups.new(name=bone)
            group.add(range(len(obj.data.vertices)), 1.0, 'REPLACE')
            objects.append(obj)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    body = bpy.context.view_layer.objects.active
    body.name = name
    body.data.name = name
    body.parent = arm
    body.modifiers.new('Armature', 'ARMATURE').object = arm
    return body


def _hit_shape(prefix, arm, bone, bm, matrix):
    obj = _link(f'{prefix}{bone}', bm, None, arm)
    obj.matrix_world = matrix
    obj.display_type = 'WIRE'
    obj['Bone'] = bone
    return obj


def hit_sphere(arm, bone, center, radius):
    """A sphere hit zone moving with bone (a physics-asset body in Unreal: what shots hit)."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=2, radius=radius)
    return _hit_shape('USP_', arm, bone, bm, Matrix.Translation(Vector(center)))


def hit_capsule(arm, bone, start, end, radius):
    """A capsule hit zone around the segment start-end (the centers of its round ends), moving with bone."""
    start, end = Vector(start), Vector(end)
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=12, radius1=radius, radius2=radius, depth=(end - start).length + 2.0 * radius)
    rotation = Vector((0.0, 0.0, 1.0)).rotation_difference(end - start)
    return _hit_shape('UCP_', arm, bone, bm, Matrix.LocRotScale((start + end) * 0.5, rotation, None))


def hit_hull(arm, bone, objects):
    """A convex hit zone moving with bone: the hull around these meshes. Make it from a part's own meshes before skin()
    merges them, and it covers exactly what's drawn (and a little more where the part curves inward)."""
    bm = bmesh.new()
    for obj in objects:
        for vertex in obj.data.vertices:
            bm.verts.new(obj.matrix_world @ vertex.co)
    hull = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in hull['geom_interior'] + hull['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    return _hit_shape('UCX_', arm, bone, bm, Matrix.Identity(4))
