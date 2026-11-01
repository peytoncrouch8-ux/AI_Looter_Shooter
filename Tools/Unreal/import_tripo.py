"""Brings the Wardens' Tripo models into the game (lane IMPORT, Docs/Work/2026-10-31-Wardens/lanes/IMPORT.md: run order).

Run in the editor (no PIE):  Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/import_tripo.py [step] [names]"
  manny      exports the mannequin (SKM_Manny_Simple) to Intermediate/TripoRig/SKM_Manny_Simple.fbx, which
             Tools/Blender/tripo_people.py fits every Tripo person's skeleton from (do this once, first).
  statics    imports Intermediate/ArtExport_Tripo (Tools\\models.ps1 -Only Wardens,TripoProps -Out
             Intermediate\\ArtExport_Tripo -NoImport made it) with Looter.ImportModels: /Game/Art/Tripo/SM_<Name>,
             /Game/Art/Props/SM_DeliaShotgun and SM_AbelsHat, MI_Tripo<Name> on the textured masters, LODs 50/25/12,
             no Nanite, box hulls, sockets.
  people     imports Intermediate/TripoPeople (Tools/Blender/tripo_people.py made it): its materials through
             Looter.ImportModels (MI_Tripo<Name> on M_Character), then each SK_<Name>.fbx onto the mannequin's skeleton
             (/Game/Characters/Mannequins/Meshes/SK_Mannequin) as /Game/Art/Characters/SK_<Name>: PA_Mannequin, the
             mannequin's mesh sockets (HandGrip_R...) copied over, LODs 50/25/12, no Nanite.
  (none)     statics, then people.
names (people only) limits it: `people Delia,Junie`. Importing again replaces the assets in place (idempotent).
Then Saved/tripo_import_probe.py prints what landed.
"""
import os
import sys

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
STATICS_DIR = os.path.join(PROJECT, 'Intermediate', 'ArtExport_Tripo')
PEOPLE_DIR = os.path.join(PROJECT, 'Intermediate', 'TripoPeople')
MANNY_FBX = os.path.join(PROJECT, 'Intermediate', 'TripoRig', 'SKM_Manny_Simple.fbx')
MANNY = '/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple'
SKELETON = '/Game/Characters/Mannequins/Meshes/SK_Mannequin'
PHYSICS = '/Game/Characters/Mannequins/Rigs/PA_Mannequin'
PEOPLE = ('Delia', 'Holloway', 'Finch', 'Junie', 'DredgerRifleman', 'DredgerHoseMan')
PEOPLE_PATH = '/Game/Art/Characters'
MATERIALS = '/Game/Art/Materials'
# LOD1.. shares of the triangles and the screen sizes they start at (the user's rule: lower-detail LODs).
LOD_SHARES, LOD_SCREENS = (0.5, 0.25, 0.12), (0.5, 0.25, 0.12)


def log(message):
    unreal.log(f'TRIPO: {message}')


def warn(message):
    unreal.log_warning(f'TRIPO: {message}')


def put(obj, prop, value):
    """set_editor_property that names a missing property instead of stopping the whole run."""
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as error:
        warn(f'{type(obj).__name__}.{prop}: {error}')
        return False


def console(command):
    unreal.SystemLibrary.execute_console_command(unreal.EditorLevelLibrary.get_editor_world(), command)


# --- The mannequin for Blender ---

def export_manny():
    mesh = unreal.load_asset(MANNY)
    if mesh is None:
        warn(f'no {MANNY}')
        return
    os.makedirs(os.path.dirname(MANNY_FBX), exist_ok=True)
    task = unreal.AssetExportTask()
    task.object = mesh
    task.filename = MANNY_FBX
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    if hasattr(unreal, 'SkeletalMeshExporterFBX'):
        task.exporter = unreal.SkeletalMeshExporterFBX()
    options = unreal.FbxExportOption()
    put(options, 'ascii', False)
    put(options, 'force_front_x_axis', False)
    put(options, 'level_of_detail', False)
    put(options, 'collision', False)
    put(options, 'export_morph_targets', False)
    task.options = options
    ok = unreal.Exporter.run_asset_export_task(task)
    log(f'mannequin exported to {MANNY_FBX}: {"ok" if ok and os.path.exists(MANNY_FBX) else "FAILED"}')


# --- Static models (the project's own importer) ---

def import_statics():
    manifests = [f for f in os.listdir(STATICS_DIR) if f.endswith('.json')] if os.path.isdir(STATICS_DIR) else []
    if not manifests:
        warn(f'no manifests in {STATICS_DIR}: run Tools\\models.ps1 -Only Wardens,TripoProps -Out Intermediate\\ArtExport_Tripo -NoImport')
        return
    console(f'Looter.ImportModels {STATICS_DIR.replace(os.sep, "/")}')
    log('statics: Looter.ImportModels ran (its LogModelImporter lines list each model)')


# --- People ---

def fbx_options(skeleton):
    ui = unreal.FbxImportUI()
    for prop, value in (('import_mesh', True), ('import_as_skeletal', True), ('import_animations', False),
                        ('create_physics_asset', False), ('import_materials', False), ('import_textures', False),
                        ('automated_import_should_detect_type', False), ('skeleton', skeleton),
                        ('mesh_type_to_import', unreal.FBXImportType.FBXIT_SKELETAL_MESH),
                        ('original_import_type', unreal.FBXImportType.FBXIT_SKELETAL_MESH)):
        put(ui, prop, value)
    data = ui.get_editor_property('skeletal_mesh_import_data')
    # tripo_people.py writes centimetres, the axes Unreal expects (looter_export's rig convention): nothing to convert.
    for prop, value in (('convert_scene', True), ('force_front_x_axis', False), ('convert_scene_unit', True),
                        ('import_uniform_scale', 1.0), ('import_translation', unreal.Vector(0.0, 0.0, 0.0)),
                        ('import_rotation', unreal.Rotator(0.0, 0.0, 0.0)),
                        ('import_content_type', unreal.FBXImportContentType.FBXICT_ALL),
                        ('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS),
                        ('normal_generation_method', unreal.FBXNormalGenerationMethod.MIKK_T_SPACE),
                        ('vertex_color_import_option', unreal.VertexColorImportOption.REPLACE),
                        ('import_morph_targets', False), ('preserve_smoothing_groups', True),
                        ('import_meshes_in_bone_hierarchy', True),
                        # Never touch the mannequin's own reference pose: the player's body plays on it.
                        ('update_skeleton_reference_pose', False), ('use_t0_as_ref_pose', False),
                        ('reorder_material_to_fbx_order', True)):
        put(data, prop, value)
    return ui


def copy_mesh_sockets(mesh, manny):
    """The mannequin's mesh-only sockets (HandGrip_R, where Delia's gun and the Dredgers' rifles hang, if it is one)."""
    copied, missing = [], []
    for index in range(manny.num_sockets()):
        socket = manny.get_socket_by_index(index)
        name = socket.get_editor_property('socket_name')
        if mesh.find_socket(name) is not None:
            continue   # on the skeleton (shared) or already copied
        try:
            mine = unreal.new_object(unreal.SkeletalMeshSocket, outer=mesh)
            for prop in ('socket_name', 'bone_name', 'relative_location', 'relative_rotation', 'relative_scale'):
                mine.set_editor_property(prop, socket.get_editor_property(prop))
            mesh.add_socket(mine, False)
            copied.append(str(name))
        except Exception as error:
            missing.append(f'{name} ({error})')
    if copied:
        log(f'{mesh.get_name()}: mannequin sockets copied: {", ".join(copied)}')
    if missing:
        warn(f'{mesh.get_name()}: add these sockets by hand (Skeleton tree, on the mesh): {", ".join(missing)}')


def make_lods(mesh):
    sub = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    count = 1 + len(LOD_SHARES)
    try:
        sub.regenerate_lod(mesh, count, True, False)
    except Exception as error:
        warn(f'{mesh.get_name()}: LODs not made ({error}); make them in the mesh editor (LOD Settings, 4 LODs)')
        return
    try:
        # Our shares and screen sizes; 5.8's Python has no lod_info on a SkeletalMesh, so the engine's default shares stay.
        infos = mesh.get_editor_property('lod_info')
        for i, (share, screen) in enumerate(zip(LOD_SHARES, LOD_SCREENS), start=1):
            info = infos[i]
            reduction = info.get_editor_property('reduction_settings')
            put(reduction, 'termination_criterion', unreal.SkeletalMeshTerminationCriterion.SMTC_NUM_OF_TRIANGLES)
            put(reduction, 'num_of_triangles_percentage', share)
            put(info, 'reduction_settings', reduction)
            size = info.get_editor_property('screen_size')
            put(size, 'default', screen)
            put(info, 'screen_size', size)
            infos[i] = info
        put(mesh, 'lod_info', infos)
        sub.regenerate_lod(mesh, count, True, False)
        log(f'{mesh.get_name()}: {count} LODs ({", ".join(f"{int(s * 100)}%" for s in LOD_SHARES)})')
    except Exception as error:
        log(f'{mesh.get_name()}: {count} LODs at the engine default shares ({error})')


def import_person(name, skeleton, physics, manny):
    fbx = os.path.join(PEOPLE_DIR, f'SK_{name}.fbx')
    if not os.path.exists(fbx):
        warn(f'no {fbx}: run Tools\\artrun.ps1 -Script Tools\\Blender\\tripo_people.py')
        return None
    task = unreal.AssetImportTask()
    task.filename = fbx
    task.destination_path = PEOPLE_PATH
    task.destination_name = f'SK_{name}'
    task.replace_existing = True
    task.replace_existing_settings = True
    task.automated = True
    task.save = False
    # The project's FBX importer, not Interchange (Interchange is off for FBX here).
    task.factory = unreal.FbxFactory()
    task.options = fbx_options(skeleton)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.load_asset(f'{PEOPLE_PATH}/SK_{name}')
    if not isinstance(mesh, unreal.SkeletalMesh):
        warn(f'SK_{name}: the import failed (see LogFbx: a bone whose parent differs from SK_Mannequin\'s stops it)')
        return None
    if mesh.get_editor_property('skeleton') != skeleton:
        warn(f'SK_{name}: not on SK_Mannequin ({mesh.get_editor_property("skeleton").get_path_name()}): the game keeps the stand-in')
    material = unreal.load_asset(f'{MATERIALS}/MI_Tripo{name}')
    # New slot structs (setting a field on the array's items edits copies, and the slots stayed empty: 2026-11-01).
    slots = []
    for slot in mesh.get_editor_property('materials'):
        new = unreal.SkeletalMaterial()
        new.set_editor_property('material_interface', material if material is not None else slot.get_editor_property('material_interface'))
        new.set_editor_property('material_slot_name', slot.get_editor_property('material_slot_name'))
        slots.append(new)
    put(mesh, 'materials', slots)
    if material is None:
        warn(f'SK_{name}: no MI_Tripo{name} (the people step makes it from TripoPeople.json first)')
    if physics is not None:
        put(mesh, 'physics_asset', physics)
    try:
        nanite = mesh.get_editor_property('nanite_settings')
        if nanite.get_editor_property('enabled'):
            put(nanite, 'enabled', False)
            put(mesh, 'nanite_settings', nanite)
    except Exception:
        pass   # no skeletal Nanite in this build: nothing to turn off
    copy_mesh_sockets(mesh, manny)
    make_lods(mesh)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
    if physics is None:
        warn(f'SK_{name}: no {PHYSICS}: hit zones and ragdoll need a physics asset')
    log(f'SK_{name} imported onto {skeleton.get_name()}')
    return mesh


def import_people(names):
    if not os.path.exists(os.path.join(PEOPLE_DIR, 'TripoPeople.json')):
        warn(f'no {PEOPLE_DIR}/TripoPeople.json: run Tools\\artrun.ps1 -Script Tools\\Blender\\tripo_people.py')
        return
    # Their materials and maps the project's way (the manifest holds materials only).
    console(f'Looter.ImportModels {PEOPLE_DIR.replace(os.sep, "/")}')
    skeleton = unreal.load_asset(SKELETON)
    manny = unreal.load_asset(MANNY)
    physics = unreal.load_asset(PHYSICS)
    if skeleton is None or manny is None:
        warn(f'no {SKELETON} or {MANNY}')
        return
    for name in names:
        import_person(name, skeleton, physics, manny)


def cap_textures():
    """The user's caps, also on the assets (the PNGs already are this size): people and ships 2K, the rest 1K."""
    big = {'Lamplighter', 'DredgeBarge', 'CompanyTender'} | set(PEOPLE)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for data in registry.get_assets_by_path('/Game/Art/Textures', recursive=True):
        path = str(data.package_name)
        folder = path.split('/')[-2]
        if not folder.startswith('Tripo') or folder[len('Tripo'):] not in {*big, *SET_NAMES}:
            continue
        texture = unreal.load_asset(path)
        cap = 2048 if folder[len('Tripo'):] in big else 1024
        if isinstance(texture, unreal.Texture2D) and texture.get_editor_property('max_texture_size') != cap:
            put(texture, 'max_texture_size', cap)
            unreal.EditorAssetLibrary.save_loaded_asset(texture, False)


SET_NAMES = {'GreatLantern', 'GrappleHook', 'DredgeHose', 'HoseIntake', 'AnchorWinch', 'LanternPost', 'LoudspeakerPost',
             'BarricadeTable', 'BarricadeBoards', 'BarricadeSacks', 'DeliaShotgun', 'AbelsHat', 'CompanyRifle',
             'HoseNozzle'}


def main(argv):
    step = argv[0] if argv else 'all'
    names = [n for a in argv[1:] for n in a.split(',') if n] or list(PEOPLE)
    if step == 'manny':
        export_manny()
        return
    if step in ('statics', 'all'):
        import_statics()
    if step in ('people', 'all'):
        import_people(names)
    cap_textures()
    log('done: run Saved/tripo_import_probe.py to see what landed')


main(sys.argv[1:])
