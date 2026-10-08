"""Style Lab export, the models: runs one of the game's scripted models (Art/Models/<Category>/<Name>.py) in an empty
scene under the bpy module, the way Tools/models.ps1 does, and writes each model it makes as the game would import it:
one top-level mesh per model (its child meshes merged in, modifiers applied, its pivot at the origin), UCX_/USP_/UCP_
collision and hit zones and _ helpers left out. A rig (a top-level armature) keeps its skin and bones. Unlike the
concept viewer's exporter it keeps the real look: UVs, normals, the 'Col' vertex colours (linear in glTF's COLOR_0:
A = baked occlusion; foliage R = wind, G = phase) and the real material names. Into Saved/StyleLab/work/models:

    <Model>.glb    the model alone, one top-level node named <Model> at the identity transform (three's axes, metres)
    <Model>.json   triangles, bounds, sockets (three's axes and Unreal's), materials and their custom properties, bones
    <Model>.npz    its triangles in Unreal's local centimetres, for the level build's traces (mock_unreal.py)

    /home/user/bpyenv/bin/python Tools/StyleLab/export/model_export.py Props/LanternPost.py [--no-ao]

Guns: Weapons/Bullpup.py and Weapons/Ranchhand.py export one assembled gun each (Gun_Bullpup, Gun_Ranchhand): the
script's DEFAULT parts on its body's sockets, as the game assembles a common gun, with Muzzle, Grip, Foregrip and Aim.
"""
import math
import os
import runpy
import sys
import time
import traceback

sys.dont_write_bytecode = True
import labcommon as lc  # noqa: E402

lc.add_paths()
lc.install_write_guard('models_' + '_'.join(a.replace('/', '_').replace('.py', '') for a in sys.argv[1:]
                                             if a.endswith('.py'))[:120])

import bpy  # noqa: E402
import numpy as np  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

MODELS = os.path.join(lc.WORK, 'models')
SKIP = ('UCX_', 'USP_', 'UCP_', '_', 'SOCKET_')
HIT = ('UCX_', 'USP_', 'UCP_')
DECIMATE_OVER = 60000
GUNS = {'Weapons/Bullpup.py': 'Gun_Bullpup', 'Weapons/Ranchhand.py': 'Gun_Ranchhand'}
RYE = os.path.join(lc.REPO, 'Art', 'Fonts', 'Rye-Regular.ttf')
SERIF = '/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf'


def log(message):
    print(f'STYLELAB: {message}', flush=True)


def patch_kits():
    """Fonts the kits name by Windows paths, and the creature painter (it writes texture sets; the lab reads the
    repository's)."""
    for kit in ('looter_rail', 'looter_train', 'looter_town', 'looter_chapel'):
        try:
            module = __import__(kit)
        except Exception as error:  # noqa: BLE001
            log(f'could not patch {kit}: {error}')
            continue
        for attr in ('RYE', 'FONT'):
            if hasattr(module, attr):
                setattr(module, attr, RYE)
        if hasattr(module, 'FALLBACK_FONT'):
            module.FALLBACK_FONT = SERIF
    try:
        import looter_creatures
        looter_creatures.paint_atlas = lambda *args, **kwargs: None
    except Exception as error:  # noqa: BLE001
        log(f'could not patch looter_creatures: {error}')


def clean_name(name):
    import re
    return re.sub(r'_+', '_', re.sub(r'[^A-Za-z0-9_]', '_', name)).strip('_')


def descendants(obj):
    out = []
    for child in obj.children:
        out.append(child)
        out.extend(descendants(child))
    return out


def model_name(obj):
    if obj.type == 'ARMATURE':
        skins = sorted((o for o in descendants(obj) if o.type == 'MESH' and not o.name.startswith(SKIP)),
                       key=lambda o: o.name)
        base = skins[0].name if skins else obj.name
        return clean_name(base[3:] if base.startswith('SK_') else base)
    return clean_name(obj.name[3:] if obj.name.startswith('SM_') else obj.name)


# --- Materials ---

def plain(value):
    if hasattr(value, 'to_list'):
        value = value.to_list()
    if isinstance(value, (list, tuple)):
        return [plain(v) for v in value]
    if isinstance(value, float):
        return round(value, 5)
    if isinstance(value, (int, str, bool)) or value is None:
        return value
    return str(value)


def material_record(mat):
    rec = {'props': {k: plain(mat[k]) for k in mat.keys() if not k.startswith('_')}}
    bsdf = None
    if mat.use_nodes and mat.node_tree:
        bsdf = next((n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
        emit = next((n for n in mat.node_tree.nodes if n.type == 'EMISSION'), None)
        if emit is not None:
            rec['emission'] = [round(c, 5) for c in emit.inputs['Color'].default_value[:3]]
    if bsdf is not None:
        rec['base'] = [round(c, 5) for c in bsdf.inputs['Base Color'].default_value[:4]]
        rec['roughness'] = round(bsdf.inputs['Roughness'].default_value, 4)
        rec['metallic'] = round(bsdf.inputs['Metallic'].default_value, 4)
        rec['alpha'] = round(bsdf.inputs['Alpha'].default_value, 4)
    else:
        rec['base'] = [round(c, 5) for c in mat.diffuse_color[:4]]
    rec['backfaceCulling'] = bool(mat.use_backface_culling)
    rec['blend'] = getattr(mat, 'surface_render_method', '') or getattr(mat, 'blend_method', '')
    return rec


# --- Meshes ---

def corner_colors(me):
    """The mesh's 'Col' (or first) colour attribute per corner, linear RGBA; white where there is none."""
    n = len(me.loops)
    attr = me.color_attributes.get('Col')
    if attr is None and len(me.color_attributes):
        attr = me.color_attributes[0]
    if attr is None:
        return np.ones((n, 4), np.float32)
    values = np.empty(len(attr.data) * 4, np.float32)
    attr.data.foreach_get('color', values)
    values = values.reshape(-1, 4)
    if attr.domain == 'POINT':
        loop_vert = np.empty(n, np.int64)
        me.loops.foreach_get('vertex_index', loop_vert)
        values = values[loop_vert]
    elif attr.domain != 'CORNER':
        return np.ones((n, 4), np.float32)
    return values


def normalise_layers(me):
    """Every piece the same layers, so join keeps them: UV layers named UV0, UV1 ... (one at least) and a single
    corner colour attribute named Col."""
    colors = corner_colors(me)
    for attr in list(me.color_attributes):
        me.color_attributes.remove(attr)
    attr = me.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    attr.data.foreach_set('color', colors.ravel())
    me.color_attributes.active_color = attr
    if not len(me.uv_layers):
        me.uv_layers.new(name='UV0')
    for i, layer in enumerate(me.uv_layers):
        layer.name = f'UV{i}'
    if len(me.uv_layers):
        me.uv_layers.active_index = 0


def baked_copy(obj, matrix, name):
    """A new object holding obj's evaluated mesh (modifiers applied) transformed by matrix @ its world matrix."""
    deps = bpy.context.evaluated_depsgraph_get()
    ev = obj.evaluated_get(deps)
    me = bpy.data.meshes.new_from_object(ev, preserve_all_data_layers=True, depsgraph=deps)
    full = matrix @ obj.matrix_world
    me.transform(full)
    if full.to_3x3().determinant() < 0.0:
        me.flip_normals()
    for i, slot in enumerate(obj.material_slots):
        if i < len(me.materials) and slot.material is not None:
            me.materials[i] = slot.material
    normalise_layers(me)
    copy = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(copy)
    return copy


def take_name(name):
    """Frees a name for the exported object (the source model usually holds it)."""
    for coll in (bpy.data.objects, bpy.data.meshes, bpy.data.armatures):
        other = coll.get(name)
        if other is not None:
            other.name = name + '__src'


def join(objects, name):
    take_name(name)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    if len(objects) > 1:
        bpy.ops.object.join()
    out = bpy.context.view_layer.objects.active
    out.name = name
    out.data.name = name
    return out


def triangles(me):
    me.calc_loop_triangles()
    return len(me.loop_triangles)


def ue_triangles(me, matrix=None):
    """(T, 3, 3) float32: the mesh's triangles in Unreal local centimetres."""
    me.calc_loop_triangles()
    co = np.empty(len(me.vertices) * 3, np.float64)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    if matrix is not None:
        m = np.array(matrix)
        co = co @ m[:3, :3].T + m[:3, 3]
    tri = np.empty(len(me.loop_triangles) * 3, np.int64)
    me.loop_triangles.foreach_get('vertices', tri)
    p = co[tri.reshape(-1, 3)]
    ue = np.stack([-p[..., 1], -p[..., 0], p[..., 2]], axis=-1) * 100.0
    return ue.astype(np.float32)


def socket_info(empty, inverse):
    """A socket in model space: three's axes (metres) and Unreal's (cm, forward and up)."""
    name = clean_name(__import__('re').sub(r'\.\d+$', '', empty.name[len('SOCKET_'):]))
    m = inverse @ empty.matrix_world
    p = m.to_translation()
    rot = m.to_3x3().normalized()
    fwd = rot @ Vector((0.0, -1.0, 0.0))
    up = rot @ Vector((0.0, 0.0, 1.0))
    return name, {
        'three': lc.r([p.x, p.z, -p.y], 4),
        'threeForward': lc.r([fwd.x, fwd.z, -fwd.y], 4), 'threeUp': lc.r([up.x, up.z, -up.y], 4),
        'ue': lc.r([-p.y * 100.0, -p.x * 100.0, p.z * 100.0], 3),
        'ueForward': lc.r([-fwd.y, -fwd.x, fwd.z], 5), 'ueUp': lc.r([-up.y, -up.x, up.z], 5),
    }


def bounds_of(me, matrix=None):
    co = np.empty(len(me.vertices) * 3, np.float64)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    if matrix is not None:
        m = np.array(matrix)
        co = co @ m[:3, :3].T + m[:3, 3]
    if not len(co):
        return [[0, 0, 0], [0, 0, 0]], [[0, 0, 0], [0, 0, 0]]
    lo, hi = co.min(0), co.max(0)
    three = [[lo[0], lo[2], -hi[1]], [hi[0], hi[2], -lo[1]]]
    ue = [[-hi[1] * 100, -hi[0] * 100, lo[2] * 100], [-lo[1] * 100, -lo[0] * 100, hi[2] * 100]]
    return lc.r(three, 4), lc.r(ue, 2)


def export_glb(objects, path):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.gltf(
        filepath=path, export_format='GLB', use_selection=True, export_apply=False, export_yup=True,
        export_materials='EXPORT', export_image_format='NONE', export_texcoords=True, export_normals=True,
        export_tangents=False, export_vertex_color='NAME', export_vertex_color_name='Col',
        export_all_vertex_colors=False, export_active_vertex_color_when_no_material=True, export_attributes=False,
        export_skins=True, export_animations=False, export_morph=False, export_extras=False, export_cameras=False,
        export_lights=False, export_def_bones=False, export_rest_position_armature=True, export_leaf_bone=False,
        export_influence_nb=4)


def write_model(name, rec, tris_ue, glb_objects, source, hull_tris=None):
    path = os.path.join(MODELS, name + '.glb')
    export_glb(glb_objects, path)
    rec.update(source=source, size=os.path.getsize(path))
    lc.write_json(os.path.join(MODELS, name + '.json'), rec, indent=1)
    extra = {'hulls': hull_tris} if hull_tris is not None else {}
    np.savez_compressed(os.path.join(MODELS, name + '.npz'), tris=tris_ue, **extra)
    log(f"EXPORT {name:32s} {rec['tris']:7d} tris {rec['size'] / 1024:8.0f} KB"
        + (f" (decimated from {rec['decimatedFrom']})" if rec.get('decimatedFrom') else '')
        + (' skinned' if rec['skinned'] else ''))


def materials_of(me):
    out = {}
    for mat in me.materials:
        if mat is not None:
            if mat.name != clean_name(mat.name):
                mat.name = clean_name(mat.name)
            out[mat.name] = material_record(mat)
    return out


def export_static(root, source, name=None, extra_objects=None, origin=None):
    """A static model: root and its meshes merged, the pivot at root's origin (rotation and scale kept)."""
    name = name or model_name(root)
    parts = [root] + descendants(root) + [o for x in (extra_objects or []) for o in [x] + descendants(x)]
    meshes = [o for o in parts if o.type == 'MESH' and not o.name.startswith(SKIP) and len(o.data.polygons)]
    if root.type == 'MESH' and root.name.startswith(SKIP):
        meshes = [m for m in meshes if m is not root]
    if not meshes:
        return None
    pivot = Vector(origin) if origin is not None else root.matrix_world.to_translation()
    to_model = Matrix.Translation(-pivot)
    copies = [baked_copy(o, to_model, f'__copy_{i}') for i, o in enumerate(meshes)]
    obj = join(copies, name)
    me = obj.data
    tris = triangles(me)
    rec = {'tris': tris, 'skinned': False, 'bones': []}
    if tris > DECIMATE_OVER:
        mod = obj.modifiers.new('Decimate', 'DECIMATE')
        mod.ratio = DECIMATE_OVER / tris
        mod.use_collapse_triangulate = True
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=mod.name)
        rec['decimatedFrom'] = tris
        rec['tris'] = tris = triangles(me)
    rec['bounds'], rec['ueBounds'] = bounds_of(me)
    rec['materials'] = materials_of(me)
    rec['slots'] = [m.name if m else '' for m in me.materials]
    inverse = to_model
    sockets = {}
    for o in parts:
        if o.type == 'EMPTY' and o.name.startswith('SOCKET_'):
            key, info = socket_info(o, inverse)
            sockets.setdefault(key, info)
    rec['sockets'] = sockets
    # What the level build's traces meet (mock_unreal.py): the render mesh (Unreal's complex collision), the UCX_ hulls
    # alone for plants (Art/README.md: a plant's hulls are all of its collision, for bullets too), or nothing (a model
    # with Collision = None, or a plant without hulls).
    hulls = [o for o in parts if o.type == 'MESH' and o.name.startswith('UCX_')]
    plant = source.startswith('Vegetation/')
    if str(root.get('Collision', '')).lower() == 'none' or (plant and not hulls):
        rec['traces'] = 'none'
    else:
        rec['traces'] = 'hulls' if plant else 'mesh'
    hull_tris = None
    if hulls:
        deps = bpy.context.evaluated_depsgraph_get()
        chunks = []
        for h in hulls:
            hm = bpy.data.meshes.new_from_object(h.evaluated_get(deps), depsgraph=deps)
            chunks.append(ue_triangles(hm, to_model @ h.matrix_world))
            bpy.data.meshes.remove(hm)
        hull_tris = np.concatenate(chunks) if chunks else None
    write_model(name, rec, ue_triangles(me), [obj], source, hull_tris)
    bpy.data.objects.remove(obj)
    return name


def export_rig(arm, source):
    name = model_name(arm)
    skins = sorted((o for o in descendants(arm) if o.type == 'MESH' and not o.name.startswith(SKIP)
                    and len(o.data.polygons)), key=lambda o: o.name)
    if not skins:
        return None
    # The rig's pivot is its origin. Bake the armature's own transform (port models are built in Unreal's units and
    # carry a scale) into the bones and the skin, so the exported rig's nodes are in metres.
    pivot = arm.matrix_world.to_translation()
    to_model = Matrix.Translation(-pivot)
    arm_world = to_model @ arm.matrix_world
    deps = bpy.context.evaluated_depsgraph_get()
    sockets = {}
    for o in descendants(arm):
        if o.type == 'EMPTY' and o.name.startswith('SOCKET_'):
            key, info = socket_info(o, to_model)
            sockets.setdefault(key, info)
    copies = []
    for i, skin in enumerate(skins):
        # The skin at rest: its mesh with non-armature modifiers applied, in model space.
        disabled = [m for m in skin.modifiers if m.type == 'ARMATURE' and m.show_viewport]
        for m in disabled:
            m.show_viewport = False
        bpy.context.view_layer.update()
        deps = bpy.context.evaluated_depsgraph_get()
        ev = skin.evaluated_get(deps)
        me = bpy.data.meshes.new_from_object(ev, preserve_all_data_layers=True, depsgraph=deps)
        for m in disabled:
            m.show_viewport = True
        full = to_model @ skin.matrix_world
        me.transform(full)
        if full.to_3x3().determinant() < 0.0:
            me.flip_normals()
        for k, slot in enumerate(skin.material_slots):
            if k < len(me.materials) and slot.material is not None:
                me.materials[k] = slot.material
        normalise_layers(me)
        copy = bpy.data.objects.new(f'__skin_{i}', me)
        bpy.context.scene.collection.objects.link(copy)
        # Vertex groups carry over by index from new_from_object; keep their names.
        for group in skin.vertex_groups:
            if copy.vertex_groups.get(group.name) is None:
                copy.vertex_groups.new(name=group.name)
        copies.append(copy)
    # The armature, its transform applied.
    arm_copy = arm.copy()
    arm_copy.data = arm.data.copy()
    bpy.context.scene.collection.objects.link(arm_copy)
    arm_copy.parent = None
    arm_copy.animation_data_clear()
    arm_copy.data.transform(arm_world)
    arm_copy.matrix_world = Matrix.Identity(4)
    for bone in arm_copy.pose.bones:
        bone.location = (0.0, 0.0, 0.0)
        bone.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        bone.rotation_euler = (0.0, 0.0, 0.0)
        bone.scale = (1.0, 1.0, 1.0)
    skin_obj = join(copies, name + '_Skin')
    skin_obj.parent = arm_copy
    skin_obj.matrix_parent_inverse = Matrix.Identity(4)
    skin_obj.matrix_world = Matrix.Identity(4)
    mod = skin_obj.modifiers.new('Armature', 'ARMATURE')
    mod.object = arm_copy
    take_name(name + '_Rig')
    arm_copy.name = name + '_Rig'
    take_name(name)
    wrapper = bpy.data.objects.new(name, None)
    bpy.context.scene.collection.objects.link(wrapper)
    arm_copy.parent = wrapper
    me = skin_obj.data
    rec = {'tris': triangles(me), 'skinned': True, 'bones': [b.name for b in arm_copy.data.bones]}
    rec['boneRest'] = {b.name: {'head': lc.r([b.head_local.x, b.head_local.z, -b.head_local.y], 4),
                                'tail': lc.r([b.tail_local.x, b.tail_local.z, -b.tail_local.y], 4),
                                'parent': b.parent.name if b.parent else None} for b in arm_copy.data.bones}
    rec['bounds'], rec['ueBounds'] = bounds_of(me)
    rec['materials'] = materials_of(me)
    rec['slots'] = [m.name if m else '' for m in me.materials]
    rec['sockets'] = sockets
    write_model(name, rec, ue_triangles(me), [wrapper, arm_copy, skin_obj], source)
    for o in (skin_obj, arm_copy, wrapper):
        bpy.data.objects.remove(o)
    return name


def export_gun(scope, source):
    """The script's DEFAULT parts on the body's sockets (its assemble()), merged into one model at the body's origin,
    with the sockets the game reads: Muzzle from the last part that has one, Grip and Foregrip from the body, Aim from
    the sight."""
    name = GUNS[source]
    parts = scope['assemble'](scope['DEFAULT'])
    body = parts[0]
    origin = body.matrix_world.to_translation()
    obj_name = export_static(body, source, name=name, extra_objects=parts[1:], origin=origin)
    # Sockets: re-read with the game's precedence (a muzzle device's Muzzle wins over the barrel's).
    path = os.path.join(MODELS, name + '.json')
    import json
    with open(path, encoding='utf-8') as f:
        rec = json.load(f)
    to_model = Matrix.Translation(-origin)
    sockets = {}
    for part in parts:
        for o in descendants(part):
            if o.type == 'EMPTY' and o.name.startswith('SOCKET_'):
                key, info = socket_info(o, to_model)
                if key == 'Muzzle' or key not in sockets:
                    sockets[key] = info
    rec['sockets'] = {k: sockets[k] for k in sorted(sockets)}
    rec['parts'] = [p.name for p in parts]
    rec['forward'] = '+z (three; the muzzle points along the model\'s +z, Unreal +X)'
    lc.write_json(path, rec, indent=1)
    return obj_name


def run(source, no_ao=False):
    os.makedirs(MODELS, exist_ok=True)
    path = os.path.join(lc.REPO, 'Art', 'Models', source)
    started = time.time()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sys.argv = ['blender', '--'] + (['--no-ao'] if no_ao else [])
    try:
        scope = runpy.run_path(path, run_name='__main__')
    except SystemExit:
        scope = {}
    except Exception:  # noqa: BLE001
        log(f'FAILED to build {source}\n{traceback.format_exc()}')
        return []
    built = time.time() - started
    if bpy.context.object is not None and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    done = []
    if source in GUNS:
        try:
            done.append(export_gun(scope, source))
        except Exception:  # noqa: BLE001
            log(f'FAILED to assemble {source}\n{traceback.format_exc()}')
    else:
        roots = sorted((o for o in bpy.context.scene.objects if o.parent is None and o.type in ('MESH', 'ARMATURE')
                        and not o.name.startswith(SKIP)), key=lambda o: o.name)
        for root in roots:
            try:
                made = export_rig(root, source) if root.type == 'ARMATURE' else export_static(root, source)
                if made:
                    done.append(made)
            except Exception:  # noqa: BLE001
                log(f'FAILED to export {root.name} from {source}\n{traceback.format_exc()}')
    log(f'{source}: {len(done)} models, built in {built:.1f} s, exported in {time.time() - started - built:.1f} s')
    if lc.REDIRECTED:
        log(f'{source}: writes kept out of the repository: {", ".join(sorted(set(lc.REDIRECTED)))}')
    return done


if __name__ == '__main__':
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    patch_kits()
    for src in args:
        run(src, no_ao='--no-ao' in sys.argv[1:])
