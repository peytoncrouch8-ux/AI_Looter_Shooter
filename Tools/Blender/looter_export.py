"""Exports the models in the open Blender scene to FBX for Unreal with the project's fixed settings, and writes the
manifest that Looter.ImportModels reads. Tools/models.ps1 runs it; Art/README.md has the authoring rules.

  blender -b Model.blend --python looter_export.py -- --out DIR --category Props --source Art/Models/Props/Model.blend

A model is every top-level mesh whose name doesn't start with '_', 'UCX_' or 'SOCKET_'. Everything parented under it
goes with it: UCX_ meshes are its collision hulls, SOCKET_ empties become sockets, and other meshes merge into it.

A rigged model (a skeletal mesh in Unreal) is every top-level armature. The meshes under it are its skin, and the meshes
named USP_ (sphere), UCP_ (capsule along its own Z) or UCX_ (convex hull) are its hit zones: each belongs to the bone
it's parented to (or named in its Bone property) and becomes a body of the model's physics asset.

The scene is never saved, so the renames and moves below don't touch the source file.
"""
import argparse
import json
import math
import os
import re
import sys

import bpy
from mathutils import Matrix, Vector

# FStylizedSurface's defaults (Source/AI_Looter_Shooter/Procedural/StylizedSurface.h). A material's custom properties
# override them; distances are meters here and centimeters in Unreal.
SURFACE_DEFAULTS = {
    'TopColor': [0.2, 0.35, 0.1, 1.0],
    'TopBlend': 0.0,
    'TopThreshold': 0.72,
    'GradHeight': 0.0,
    'GradDark': 0.0,
    'Strata': 0.0,
    'Wind': 0.0,
    'Glow': 0.0,
    'Variation': 0.12,
    'UpNormal': 0.0,
}
METER_SETTINGS = ('GradHeight', 'Wind')
KINDS = ('Surface', 'Foliage', 'Glow')
SKIPPED_PREFIXES = ('_', 'UCX_', 'SOCKET_')
HIT_PREFIXES = ('USP_', 'UCP_', 'UCX_')


def log(message):
    """Lines models.ps1 shows."""
    print(f'LOOTER: {message}', flush=True)


def fail(message):
    log(f'ERROR {message}')
    sys.exit(1)


def clean_name(name):
    """Unreal-safe: letters, digits and single underscores."""
    return re.sub(r'_+', '_', re.sub(r'[^A-Za-z0-9_]', '_', name)).strip('_')


def rename(obj, name):
    """Gives obj exactly this name, moving any other object that has it out of the way."""
    other = bpy.data.objects.get(name)
    if other is not None and other != obj:
        other.name = name + '_moved'
    obj.name = name


def descendants(obj):
    found = []
    for child in obj.children:
        found.append(child)
        found.extend(descendants(child))
    return found


def base_color(material):
    """The Principled BSDF's base color (linear), else the viewport color."""
    if material.use_nodes and material.node_tree:
        for node in material.node_tree.nodes:
            if node.type == 'BSDF_PRINCIPLED':
                return [float(c) for c in node.inputs['Base Color'].default_value]
    return [float(c) for c in material.diffuse_color]


def material_look(material):
    kind = str(material.get('Kind', 'Surface'))
    if kind not in KINDS:
        fail(f"material {material.name}: Kind must be one of {', '.join(KINDS)}, not {kind}")
    look = {'Kind': kind, 'Color': base_color(material)}
    for key, default in SURFACE_DEFAULTS.items():
        value = material.get(key, default)
        if hasattr(value, 'to_list'):
            value = value.to_list()
        if isinstance(value, (list, tuple)):
            value = [float(v) for v in value]
            value += [1.0] * (4 - len(value))
        else:
            value = float(value) * (100.0 if key in METER_SETTINGS else 1.0)
        look[key] = value
    return look


def to_unreal(vector, scale=1.0):
    """A Blender position or direction in Unreal's axes: the Front view's -Y becomes +X (forward), +X becomes -Y, and
    Z stays up. scale=100 turns meters into centimeters."""
    return [-vector.y * scale, -vector.x * scale, vector.z * scale]


def socket_entry(empty):
    """A socket, in the model's Unreal space. Its front is the empty's -Y and its top the empty's +Z, like a model's.
    (FBX sockets come in with the wrong rotation: Blender's and Unreal's axes differ in handedness.) Blender numbers
    names that repeat across models (a second SOCKET_Muzzle becomes SOCKET_Muzzle.001); the number is dropped."""
    matrix = empty.matrix_world
    rotation = matrix.to_3x3().normalized()
    return {
        'name': clean_name(re.sub(r'\.\d+$', '', empty.name[len('SOCKET_'):])),
        'location': to_unreal(matrix.translation, 100.0),
        'forward': to_unreal(rotation @ Vector((0.0, -1.0, 0.0))),
        'up': to_unreal(rotation @ Vector((0.0, 0.0, 1.0))),
    }


def skin_meshes(arm):
    """A rig's skin: the meshes under its armature, other than hit zones and helpers."""
    return sorted((o for o in descendants(arm) if o.type == 'MESH' and not o.name.startswith(HIT_PREFIXES + ('_',))),
                  key=lambda o: o.name)


def model_name(obj):
    """SM_<object> for a model; a rig is SK_<its skin mesh>, since its armature is always called root (see export_rig)."""
    if obj.type == 'ARMATURE':
        skins = skin_meshes(obj)
        base = skins[0].name if skins else obj.name
        return 'SK_' + clean_name(base[3:] if base.startswith('SK_') else base)
    return 'SM_' + clean_name(obj.name[3:] if obj.name.startswith('SM_') else obj.name)


def collect_materials(name, meshes, materials):
    """Slot names come from material names, and each slot gets MI_<name> in Unreal. Returns the names used."""
    used = []
    for mesh in meshes:
        if not mesh.material_slots or any(slot.material is None for slot in mesh.material_slots):
            fail(f'{name}: {mesh.name} has an empty material slot; give every slot a material')
        for slot in mesh.material_slots:
            material = slot.material
            clean = clean_name(material.name)
            if material.name != clean:
                other = bpy.data.materials.get(clean)
                if other is not None and other != material:
                    fail(f'materials {material.name} and {other.name} would both be called {clean} in Unreal')
                material.name = clean
            if material.name not in used:
                used.append(material.name)
            materials[material.name] = material_look(material)
    return used


def select_only(objects, active):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        if obj.name not in bpy.context.view_layer.objects:
            fail(f"{obj.name} is in a collection that's excluded from the view layer")
        obj.hide_set(False)
        obj.hide_viewport = False
        obj.select_set(True)
    bpy.context.view_layer.objects.active = active


def hit_shape_entry(obj, bones):
    """A hit zone in the model's Unreal space (cm): a sphere (center, radius), a capsule (start, end: the centers of its
    round ends; radius) or a convex hull (its points). The importer turns it into a physics-asset body on its bone."""
    bone = obj.parent_bone if obj.parent_type == 'BONE' and obj.parent_bone else str(obj.get('Bone', ''))
    if bone not in bones:
        fail(f"hit zone {obj.name} names no bone of the rig (parent it to a bone or set its Bone property)")
    matrix = obj.matrix_world
    size = obj.dimensions
    if obj.name.startswith('UCX_'):
        return {'bone': bone, 'shape': 'convex', 'points': [to_unreal(matrix @ v.co, 100.0) for v in obj.data.vertices]}
    if obj.name.startswith('USP_'):
        return {'bone': bone, 'shape': 'sphere', 'center': to_unreal(matrix.translation, 100.0), 'radius': max(size) * 50.0}
    radius = max(size.x, size.y) * 0.5
    axis = (matrix.to_3x3() @ Vector((0.0, 0.0, 1.0))).normalized()
    half = max(size.z * 0.5 - radius, 0.0)
    return {'bone': bone, 'shape': 'capsule', 'radius': radius * 100.0,
            'start': to_unreal(matrix.translation - axis * half, 100.0), 'end': to_unreal(matrix.translation + axis * half, 100.0)}


def bake_for_unreal(arm, skins):
    """Unreal takes the armature's transform as the root bone's, and its unit and axis conversions end up there too (a
    scale of 100 and a quarter turn). So a rig reaches the FBX with nothing left to convert: rebuilt in centimeters (in a
    scene whose unit is the centimeter), turned so that Unreal's usual right-to-left-handed flip leaves it facing +X
    like other models, with the armature's own transform baked in."""
    to_unreal_space = Matrix.Rotation(math.radians(90.0), 4, 'Z') @ Matrix.Scale(100.0, 4)
    to_centimeters = to_unreal_space @ arm.matrix_world
    skin_matrices = {mesh.name: to_unreal_space @ mesh.matrix_world for mesh in skins}
    arm.data.transform(to_centimeters)
    arm.matrix_world = Matrix.Identity(4)
    for mesh in skins:
        mesh.data.transform(skin_matrices[mesh.name])
        mesh.parent = arm
        mesh.matrix_parent_inverse = Matrix.Identity(4)
        mesh.matrix_basis = Matrix.Identity(4)
    bpy.context.scene.unit_settings.scale_length = 0.01
    bpy.context.view_layer.update()


def export_rig(arm, out_dir, materials):
    name = model_name(arm)
    hits = [o for o in descendants(arm) if o.type == 'MESH' and o.name.startswith(HIT_PREFIXES)]
    skins = skin_meshes(arm)
    if not skins:
        fail(f'{name}: the armature has no meshes under it')
    used = collect_materials(name, skins, materials)

    # Unreal makes the armature object the root bone.
    bone_names = {bone.name for bone in arm.data.bones}
    if 'root' in bone_names:
        fail(f"{name}: a bone is called root; that name is the armature's (it becomes the root bone in Unreal)")
    rename(arm, 'root')

    # The rig's origin is its pivot, as for other models.
    arm.location = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    shapes = [hit_shape_entry(obj, bone_names) for obj in hits]
    bake_for_unreal(arm, skins)

    select_only([arm] + skins, arm)
    bpy.ops.export_scene.fbx(
        filepath=os.path.join(out_dir, name + '.fbx'),
        use_selection=True,
        object_types={'ARMATURE', 'MESH'},
        use_mesh_modifiers=False,
        mesh_smooth_type='FACE',
        use_tspace=False,
        use_custom_props=False,
        colors_type='SRGB',
        add_leaf_bones=False,
        use_armature_deform_only=True,
        armature_nodetype='NULL',
        bake_anim=False,
        # The scene's centimeters and Blender's own axes go into the file as they are (see bake_for_unreal): they're
        # what Unreal expects, so it converts nothing.
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_ALL',
        global_scale=1.0,
        axis_forward='Y',
        axis_up='Z',
        use_space_transform=True,
        bake_space_transform=False,
        path_mode='AUTO',
        embed_textures=False,
    )
    log(f"{name}: rig of {len(bone_names)} bones, {len(skins)} meshes, {len(shapes)} hit zones, materials {', '.join(used)}")
    return {'name': name, 'fbx': name + '.fbx', 'skeletal': True, 'materials': used, 'hitShapes': shapes}


def export_model(root, out_dir, materials):
    name = model_name(root)
    parts = descendants(root)
    hulls = [o for o in parts if o.type == 'MESH' and o.name.startswith('UCX_')]
    meshes = [root] + [o for o in parts if o.type == 'MESH' and not o.name.startswith('UCX_')]
    sockets = [o for o in parts if o.name.startswith('SOCKET_')]

    # Unreal pairs hulls with the mesh by name: UCX_<mesh node>_<number>.
    rename(root, name)
    for index, hull in enumerate(hulls):
        rename(hull, f'UCX_{name}_{index:02d}')

    used = collect_materials(name, meshes, materials)

    # The model's origin becomes its pivot: move it to the world origin (rotation and scale stay and get baked in).
    root.location = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()

    select_only([root] + parts, root)
    bpy.ops.export_scene.fbx(
        filepath=os.path.join(out_dir, name + '.fbx'),
        use_selection=True,
        object_types={'MESH'},
        use_mesh_modifiers=True,
        mesh_smooth_type='FACE',
        use_triangles=False,
        use_tspace=False,
        use_custom_props=False,
        colors_type='SRGB',
        add_leaf_bones=False,
        bake_anim=False,
        # 1 Blender unit = 1 m = 100 Unreal cm, carried in the object transforms Unreal bakes into the vertices.
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_NONE',
        global_scale=1.0,
        # Blender's default axes; the importer turns the Front view (-Y) to the actor's forward (+X).
        axis_forward='-Z',
        axis_up='Y',
        use_space_transform=True,
        bake_space_transform=False,
        path_mode='AUTO',
        embed_textures=False,
    )
    log(f"{name}: {len(meshes)} meshes, {len(hulls)} hulls, {len(sockets)} sockets, materials {', '.join(used)}")
    return {
        'name': name,
        'fbx': name + '.fbx',
        'collision': 'hulls' if hulls else 'mesh',
        'nanite': bool(root.get('Nanite', True)),
        'materials': used,
        'sockets': [socket_entry(empty) for empty in sockets],
    }


def main():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', required=True)
    parser.add_argument('--category', required=True)
    parser.add_argument('--source', required=True)
    args = parser.parse_args(argv)

    scene = bpy.context.scene
    if abs(scene.unit_settings.scale_length - 1.0) > 1e-6:
        log(f'warning: unit scale is {scene.unit_settings.scale_length}; models are meant to be made in meters (1.0)')
    if bpy.context.object is not None and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')

    roots = [o for o in scene.objects if o.parent is None and o.type in ('MESH', 'ARMATURE') and not o.name.startswith(SKIPPED_PREFIXES)]
    if not roots:
        fail(f'{args.source} has no models (top-level meshes or armatures)')
    names = [model_name(o) for o in roots]
    duplicates = sorted({n for n in names if names.count(n) > 1})
    if duplicates:
        fail(f"two models would share the name {', '.join(duplicates)}")

    os.makedirs(args.out, exist_ok=True)
    materials = {}
    models = [export_rig(root, args.out, materials) if root.type == 'ARMATURE' else export_model(root, args.out, materials) for root in roots]
    manifest = {'source': args.source, 'category': args.category, 'models': models, 'materials': materials}
    manifest_path = os.path.join(args.out, clean_name(os.path.splitext(args.source)[0]) + '.json')
    with open(manifest_path, 'w', encoding='utf-8') as file:
        json.dump(manifest, file, indent=2)
    log(f'exported {len(models)} models from {args.source}')


main()
