"""Exports the models in the open Blender scene to FBX for Unreal with the project's fixed settings, and writes the
manifest that Looter.ImportModels reads. Tools/models.ps1 runs it; Art/README.md has the authoring rules.

  blender -b Model.blend --python looter_export.py -- --out DIR --category Props --source Art/Models/Props/Model.blend

A model is every top-level mesh whose name doesn't start with '_', 'UCX_' or 'SOCKET_'. Everything parented under it
goes with it: UCX_ meshes are its collision hulls, SOCKET_ empties become sockets, and other meshes merge into it.
The scene is never saved, so the renames and moves below don't touch the source file.
"""
import argparse
import json
import os
import re
import sys

import bpy
from mathutils import Vector

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
    (FBX sockets come in with the wrong rotation: Blender's and Unreal's axes differ in handedness.)"""
    matrix = empty.matrix_world
    rotation = matrix.to_3x3().normalized()
    return {
        'name': clean_name(empty.name[len('SOCKET_'):]),
        'location': to_unreal(matrix.translation, 100.0),
        'forward': to_unreal(rotation @ Vector((0.0, -1.0, 0.0))),
        'up': to_unreal(rotation @ Vector((0.0, 0.0, 1.0))),
    }


def export_model(root, out_dir, materials):
    base = root.name[3:] if root.name.startswith('SM_') else root.name
    name = 'SM_' + clean_name(base)
    parts = descendants(root)
    hulls = [o for o in parts if o.type == 'MESH' and o.name.startswith('UCX_')]
    meshes = [root] + [o for o in parts if o.type == 'MESH' and not o.name.startswith('UCX_')]
    sockets = [o for o in parts if o.name.startswith('SOCKET_')]

    # Unreal pairs hulls with the mesh by name: UCX_<mesh node>_<number>.
    rename(root, name)
    for index, hull in enumerate(hulls):
        rename(hull, f'UCX_{name}_{index:02d}')

    # Slot names come from material names, and each slot gets MI_<name> in Unreal.
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

    # The model's origin becomes its pivot: move it to the world origin (rotation and scale stay and get baked in).
    root.location = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()

    bpy.ops.object.select_all(action='DESELECT')
    for obj in [root] + parts:
        if obj.name not in bpy.context.view_layer.objects:
            fail(f"{name}: {obj.name} is in a collection that's excluded from the view layer")
        obj.hide_set(False)
        obj.hide_viewport = False
        obj.select_set(True)
    bpy.context.view_layer.objects.active = root

    path = os.path.join(out_dir, name + '.fbx')
    bpy.ops.export_scene.fbx(
        filepath=path,
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

    roots = [o for o in scene.objects if o.parent is None and o.type == 'MESH' and not o.name.startswith(SKIPPED_PREFIXES)]
    if not roots:
        fail(f'{args.source} has no models (top-level meshes)')
    names = ['SM_' + clean_name(o.name[3:] if o.name.startswith('SM_') else o.name) for o in roots]
    duplicates = sorted({n for n in names if names.count(n) > 1})
    if duplicates:
        fail(f"two models would share the name {', '.join(duplicates)}")

    os.makedirs(args.out, exist_ok=True)
    materials = {}
    models = [export_model(root, args.out, materials) for root in roots]
    manifest = {'source': args.source, 'category': args.category, 'models': models, 'materials': materials}
    manifest_path = os.path.join(args.out, clean_name(os.path.splitext(args.source)[0]) + '.json')
    with open(manifest_path, 'w', encoding='utf-8') as file:
        json.dump(manifest, file, indent=2)
    log(f'exported {len(models)} models from {args.source}')


main()
