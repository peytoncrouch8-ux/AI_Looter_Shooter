"""The Wardens' Tripo people (TRIPO_MODELS.md 1-6: Delia, Holloway, Finch, Junie, the two Dredgers) put on the UE5 mannequin's
own skeleton, so the game wears them as the mannequin itself and its animations play on them (lane CREW's rule: any other
skeleton warns and the stand-in stays; DredgerLook's bodies the same).

Why not Tripo's rig as it comes: its "unreal" rig has the UE4 mannequin's 61 bones (spine_01-03, neck_01, fingers straight
off the hand). The game's SK_Mannequin is UE5's (spine_01-05, neck_01-02, finger metacarpals, ik bones), and Unreal
imports a mesh onto a skeleton only when every bone's parent matches. So each person gets the mannequin's whole
skeleton, read from the mannequin itself (import_tripo.py's `manny` step exports SKM_Manny_Simple to
Intermediate/TripoRig/SKM_Manny_Simple.fbx), fitted to the person:
- every bone Tripo has goes to Tripo's joint (scaled to the person's real height, turned to face the mannequin's way);
  spine_04/05, neck_02 and the metacarpals are placed between Tripo's joints at the mannequin's own proportions; the ik
  bones on the feet and hands; anything else keeps its offset from its parent;
- each bone keeps the mannequin's own axes, turned only as far as its limb points another way (the shortest turn from the
  mannequin's bone to the person's), so an animation's rotations mean the same on both bodies;
- Tripo's skin weights are kept; spine_03's are shared over spine_03-05 and neck_01's over neck_01-02 by height.
The mesh itself is Tripo's (its triangles, its UVs); its maps are the GLB's, graded and on M_Character
(looter_tripo_wardens.write_maps: the style rule), 2K.

Run (the editor may be open): Tools\\artrun.ps1 -Script Tools\\Blender\\tripo_people.py [-ScriptArgs Delia,Junie]
Writes Intermediate/TripoPeople/SK_<Name>.fbx and TripoPeople.json (the materials, for Looter.ImportModels); then
Tools/Unreal/import_tripo.py imports them onto SK_Mannequin. LOOTER_MANNY_FBX overrides the mannequin file (tests).
"""
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_textures as lt
import looter_tripo as tripo
import looter_tripo_wardens as tw

REPO = lt.REPO
MANNY = os.environ.get('LOOTER_MANNY_FBX') or os.path.join(REPO, 'Intermediate', 'TripoRig', 'SKM_Manny_Simple.fbx')
OUT = os.path.join(REPO, 'Intermediate', 'TripoPeople')
# Real heights (m, TRIPO_MODELS.md); the game then wears them at its 0.85 (FWardenLook::ModelScale).
PEOPLE = {'Delia': 1.60, 'Holloway': 1.75, 'Finch': 1.80, 'Junie': 1.65, 'DredgerRifleman': 1.80, 'DredgerHoseMan': 1.85}
MAPS = 2048
FINGERS = ('index', 'middle', 'ring', 'pinky')
# Each bone's main child: its direction is the limb's (others: the only child with a place, else none: a leaf).
MAIN_CHILD = {'pelvis': 'spine_01', 'spine_01': 'spine_02', 'spine_02': 'spine_03', 'spine_03': 'spine_04',
              'spine_04': 'spine_05', 'spine_05': 'neck_01', 'neck_01': 'neck_02', 'neck_02': 'head'}
for _s in ('l', 'r'):
    MAIN_CHILD.update({f'clavicle_{_s}': f'upperarm_{_s}', f'upperarm_{_s}': f'lowerarm_{_s}',
                       f'lowerarm_{_s}': f'hand_{_s}', f'hand_{_s}': f'middle_metacarpal_{_s}',
                       f'thigh_{_s}': f'calf_{_s}', f'calf_{_s}': f'foot_{_s}', f'foot_{_s}': f'ball_{_s}',
                       f'thumb_01_{_s}': f'thumb_02_{_s}', f'thumb_02_{_s}': f'thumb_03_{_s}'})
    for _f in FINGERS:
        MAIN_CHILD.update({f'{_f}_metacarpal_{_s}': f'{_f}_01_{_s}', f'{_f}_01_{_s}': f'{_f}_02_{_s}',
                           f'{_f}_02_{_s}': f'{_f}_03_{_s}'})
IK_AT = {'ik_foot_l': 'foot_l', 'ik_foot_r': 'foot_r', 'ik_hand_l': 'hand_l', 'ik_hand_r': 'hand_r', 'ik_hand_gun': 'hand_r'}


def log(message):
    print(f'LOOTER: {message}', flush=True)


def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def import_fbx(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path, automatic_bone_orientation=False, ignore_leaf_bones=False,
                             use_custom_normals=True)
    return [o for o in bpy.data.objects if o not in before]


def armature_frames(arm):
    """name -> (world head, world 3x3 frame with unit columns, parent name or None, length)."""
    out = {}
    for b in arm.data.bones:
        m = arm.matrix_world @ b.matrix_local
        out[b.name] = (m.translation.copy(), m.to_3x3().normalized(), b.parent.name if b.parent else None,
                       (arm.matrix_world.to_3x3() @ (b.tail_local - b.head_local)).length)
    return out


def facing(frames):
    """Where a body faces (flat): square to its hip line, its left hip to its right (left x up)."""
    left = frames["thigh_l"][0] - frames["thigh_r"][0]
    return flat(left.cross(Vector((0.0, 0.0, 1.0))))


def flat(v):
    v = Vector((v.x, v.y, 0.0))
    return v.normalized() if v.length > 1e-6 else Vector((0.0, -1.0, 0.0))


def frac_along(p, a, b):
    d = b - a
    return max(0.02, min(0.98, (p - a).dot(d) / max(d.length_squared, 1e-9)))


def read_manny():
    clear()
    new = import_fbx(MANNY)
    arm = next((o for o in new if o.type == 'ARMATURE'), None)
    if arm is None:
        raise RuntimeError(f'{MANNY}: no armature')
    M = armature_frames(arm)
    # Meters whatever the file's unit: the pelvis stands about a metre up.
    if M['pelvis'][0].z > 10.0:
        M = {k: (h * 0.01, f, p, l * 0.01) for k, (h, f, p, l) in M.items()}
    names = []   # parents before children
    pending = [b for b in arm.data.bones if b.parent is None]
    while pending:
        b = pending.pop(0)
        names.append(b.name)
        pending.extend(b.children)
    log(f'mannequin: {len(names)} bones, pelvis at {M["pelvis"][0].z:.3f} m, facing {tuple(round(c, 2) for c in facing(M))}')
    return M, names


def uv_signature(obj):
    """The mesh's UVs as a sorted, rounded set summary: equal for the same mesh whatever its vertex order."""
    uv = np.empty(len(obj.data.loops) * 2)
    obj.data.uv_layers.active.data.foreach_get('uv', uv)
    uv = np.round(uv.reshape(-1, 2), 3)
    return len(obj.data.polygons), tuple(np.round(uv.mean(axis=0), 4)), tuple(np.round(uv.std(axis=0), 4))


def maps_from_glb(name):
    """The GLB's maps, as Tripo made them (the rigged FBX is the same mesh: the same triangles and UVs, checked)."""
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(tw.TRIPO_DIR, name, name + '.glb'))
    new = [o for o in bpy.data.objects if o not in before]
    mesh = next(o for o in new if o.type == 'MESH')
    maps = {'BC': None, 'RM': None, 'N': None}
    for m in mesh.data.materials:
        bsdf = next((n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None) if m and m.use_nodes else None
        if bsdf:
            maps['BC'] = maps['BC'] or tripo._upstream_image(bsdf.inputs['Base Color'])
            maps['RM'] = maps['RM'] or tripo._upstream_image(bsdf.inputs['Roughness']) or tripo._upstream_image(bsdf.inputs['Metallic'])
            maps['N'] = maps['N'] or tripo._upstream_image(bsdf.inputs['Normal'])
    signature = uv_signature(mesh)
    for o in new:
        bpy.data.objects.remove(o)
    return maps, signature


def split_weights(mesh, source, targets, heads):
    """Shares the vertex group source over targets by where each vertex lies along heads (one point per target, the last
    target taking everything past its point): linear between neighbours."""
    groups = {n: mesh.vertex_groups.get(n) or mesh.vertex_groups.new(name=n) for n in targets}
    src = mesh.vertex_groups.get(source)
    if src is None:
        return
    a, b = heads[0], heads[-1]
    axis = b - a
    ts = [frac_along(h, a, b) if i else 0.0 for i, h in enumerate(heads)]
    ts[-1] = 1.0
    gi = src.index
    for v in mesh.data.vertices:
        w = next((g.weight for g in v.groups if g.group == gi), 0.0)
        if w <= 0.0:
            continue
        t = (v.co - a).dot(axis) / max(axis.length_squared, 1e-9)
        shares = np.zeros(len(targets))
        if t <= ts[0]:
            shares[0] = 1.0
        elif t >= ts[-1]:
            shares[-1] = 1.0
        else:
            k = max(i for i in range(len(ts) - 1) if ts[i] <= t)
            u = (t - ts[k]) / max(ts[k + 1] - ts[k], 1e-6)
            shares[k], shares[k + 1] = 1.0 - u, u
        for n, s in zip(targets, shares):
            if n == source:
                src.add([v.index], w * s, 'REPLACE')
            elif s > 0.0:
                groups[n].add([v.index], w * s, 'ADD')


def person(name, height, M, order):
    clear()
    maps, glb_uv = maps_from_glb(name)
    new = import_fbx(os.path.join(tw.TRIPO_DIR, name, f'{name}_rigged.fbx'))
    ta = next(o for o in new if o.type == 'ARMATURE')
    tm = next(o for o in new if o.type == 'MESH')
    if uv_signature(tm) != glb_uv:
        log(f'warning: {name}: the rigged mesh {uv_signature(tm)} isn\'t the GLB\'s {glb_uv}: its maps may not fit')
    T = armature_frames(ta)
    lo = min((tm.matrix_world @ v.co).z for v in tm.data.vertices)
    hi = max((tm.matrix_world @ v.co).z for v in tm.data.vertices)
    k = height / (hi - lo)
    # Turn to face the mannequin's way (its toes' way), about the root, and scale to the real height (feet on 0).
    yaw = math.atan2(facing(M).y, facing(M).x) - math.atan2(facing(T).y, facing(T).x)
    X = Matrix.Rotation(yaw, 4, 'Z') @ Matrix.Scale(k, 4) @ Matrix.Translation((0.0, 0.0, -lo))
    P = {n: X @ h for n, (h, f, p, l) in T.items() if n != 'root'}

    # Where every mannequin bone goes (first the ones placed from Tripo's joints).
    ratio = P['pelvis'].z / M['pelvis'][0].z
    heads = {}
    for n in order:
        if n == 'root':
            continue
        if n in P:
            heads[n] = P[n]
        elif n in ('spine_04', 'spine_05'):
            heads[n] = P['spine_03'].lerp(P['neck_01'], frac_along(M[n][0], M['spine_03'][0], M['neck_01'][0]))
        elif n == 'neck_02':
            heads[n] = P['neck_01'].lerp(P['head'], frac_along(M[n][0], M['neck_01'][0], M['head'][0]))
        elif '_metacarpal_' in n:
            finger, side = n.split('_metacarpal_')
            first = f'{finger}_01_{side}'
            if first in P:
                heads[n] = P[f'hand_{side}'].lerp(P[first], frac_along(M[n][0], M[f'hand_{side}'][0], M[first][0]))
        elif n in IK_AT and IK_AT[n] in P:
            heads[n] = P[IK_AT[n]].copy()
    turns, frames = {}, {}
    for n in order:
        if n == 'root':
            continue
        mh, mf, mp, ml = M[n]
        parent = mp if mp in M and mp != 'root' else None
        child = MAIN_CHILD.get(n)
        if child not in heads or child not in M:
            kids = [c for c in order if M[c][2] == n and c in heads]
            child = kids[0] if len(kids) == 1 and not n.startswith('ik_') else None
        if n not in heads:
            # Not on Tripo's body: its offset from its parent as on the mannequin, turned and scaled with it.
            base_turn = turns.get(parent, Matrix.Identity(3))
            heads[n] = (heads[parent] if parent else Vector()) + base_turn @ ((mh - (M[parent][0] if parent else Vector())) * ratio)
        if n in IK_AT:
            turn = turns.get(IK_AT[n], Matrix.Identity(3))
        elif child and n in heads:
            dm, dn = M[child][0] - mh, heads[child] - heads[n]
            turn = dm.rotation_difference(dn).to_matrix() if dm.length > 1e-6 and dn.length > 1e-6 else Matrix.Identity(3)
        else:
            turn = turns.get(parent, Matrix.Identity(3))
        turns[n] = turn
        frames[n] = turn @ mf

    # The new armature: the object is the root bone (looter_export's rule), so the mannequin's root bone isn't a bone here.
    arm = bpy.data.objects.new('root', bpy.data.armatures.new('root'))
    bpy.context.scene.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones
    for n in order:
        if n == 'root':
            continue
        eb = edit.new(n)
        length = max(M[n][3] * ratio, 0.01)
        eb.head = heads[n]
        eb.tail = heads[n] + Vector((0.0, 0.0, length))
        m4 = frames[n].to_4x4()
        m4.translation = heads[n]
        eb.matrix = m4
        eb.length = length
        parent = M[n][2]
        if parent and parent != 'root':
            eb.parent = edit[parent]
        eb.use_connect = False
        eb.use_deform = True
    bpy.ops.object.mode_set(mode='OBJECT')

    # The skin: Tripo's mesh moved the same way, its weights kept, the spine's and neck's shared out.
    world = X @ tm.matrix_world
    tm.parent = None
    tm.data.transform(world)
    tm.matrix_world = Matrix.Identity(4)
    for mod in list(tm.modifiers):
        tm.modifiers.remove(mod)
    bpy.data.objects.remove(ta)
    split_weights(tm, 'spine_03', ['spine_03', 'spine_04', 'spine_05'], [heads['spine_03'], heads['spine_04'], heads['spine_05']])
    split_weights(tm, 'neck_01', ['neck_01', 'neck_02'], [heads['neck_01'], heads['neck_02']])
    known = {b.name for b in arm.data.bones}
    stray = [g.name for g in tm.vertex_groups if g.name not in known]
    if stray:
        log(f'warning: {name}: weights on bones the mannequin hasn\'t: {stray}')
    tm.parent = arm
    mod = tm.modifiers.new('Armature', 'ARMATURE')
    mod.object = arm
    tm.name = name
    tm.data.name = name

    # Its look: the GLB's maps through M_Character.
    mat = tw.write_maps(maps, name, 'Character', MAPS, 'person')
    tm.data.materials.clear()
    tm.data.materials.append(mat)
    for poly in tm.data.polygons:
        poly.material_index = 0
    arm['LODs'] = '50,25,12'
    arm['LODScreens'] = '0.5,0.25,0.12'
    return arm, tm, mat, {'height m': round(hi * k - lo * k, 3), 'scale': round(k, 4), 'tris': len(tm.data.polygons),
                          'bones': len(arm.data.bones), 'turned deg': round(math.degrees(yaw), 1)}


def export(arm, mesh, name):
    """As looter_export.export_rig writes a rig (centimetres, the armature object as the root bone), but every bone kept:
    the mannequin's ik and metacarpal bones have no weights, and Unreal needs them to match the skeleton."""
    unit = bpy.context.scene.unit_settings.scale_length
    to_cm = Matrix.Scale(100.0, 4)
    arm.data.transform(to_cm)
    mesh.data.transform(to_cm)
    bpy.context.scene.unit_settings.scale_length = 0.01
    bpy.context.view_layer.update()
    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True)
    mesh.select_set(True)
    bpy.context.view_layer.objects.active = arm
    path = os.path.join(OUT, f'SK_{name}.fbx')
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, object_types={'ARMATURE', 'MESH'}, use_mesh_modifiers=False,
        mesh_smooth_type='FACE', use_tspace=False, use_custom_props=False, colors_type='SRGB', add_leaf_bones=False,
        use_armature_deform_only=False, armature_nodetype='NULL', bake_anim=False, apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_ALL', global_scale=1.0, axis_forward='Y', axis_up='Z', use_space_transform=True,
        bake_space_transform=False, path_mode='AUTO', embed_textures=False)
    bpy.context.scene.unit_settings.scale_length = unit
    return path


def material_entry(mat):
    """The manifest's look for a textured material (looter_export.textured_look's shape)."""
    set_name = mat['TextureSet']
    files = {}
    for param, kind in (('BaseColorMap', 'BC'), ('NormalMap', 'N'), ('ORMMap', 'ORM')):
        path = lt.texture_path(set_name, kind)
        if os.path.exists(path):
            files[param] = os.path.relpath(path, REPO).replace('\\', '/')
    return {'Master': mat['Master'], 'Textures': files, 'Tint': [1.0, 1.0, 1.0, 1.0], 'UVScale': 1.0, 'Scalars': {},
            'Colors': {}}


def main():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    wanted = [n for a in argv for n in a.split(',') if n] or list(PEOPLE)
    if not os.path.exists(MANNY):
        log(f'ERROR no {MANNY}: run `py Tools/Unreal/import_tripo.py manny` in the editor first')
        sys.exit(1)
    os.makedirs(OUT, exist_ok=True)
    M, order = read_manny()
    manifest_path = os.path.join(OUT, 'TripoPeople.json')
    manifest = {'source': 'Tools/Blender/tripo_people.py', 'category': 'Characters', 'models': [], 'materials': {}}
    if os.path.exists(manifest_path):
        with open(manifest_path, encoding='utf-8') as f:
            manifest['materials'] = json.load(f).get('materials', {})
    for name in wanted:
        arm, mesh, mat, report = person(name, PEOPLE[name], M, order)
        path = export(arm, mesh, name)
        manifest['materials'][mat.name] = material_entry(mat)
        log(f'SK_{name}: {report} -> {path}')
    with open(manifest_path, 'w', encoding='utf-8') as f:
        json.dump(manifest, f, indent=2)
    log(f'exported {len(wanted)} people to {OUT}')


main()
