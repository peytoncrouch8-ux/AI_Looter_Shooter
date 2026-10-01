"""Photographs imported models under the tutorial island's lighting, for reviewing art.

Opens a blank untitled level (never saved), lights it like the island (build_tutorial_island.environment), lays a
grass ground, lines up the models and puts a camera on them. Screenshots are taken a frame later, so each is its own
step (give new shaders time to compile after staging):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/review_stage.py"                 stage
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/review_stage.py shoot Medium Wide"  per quality and view
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/review_stage.py restore"         Epic, and the island level
Settings come from Saved/ReviewStage.json (written by whoever runs it):
  {"meshes": ["/Game/Art/Vegetation/SM_Oak_A", ...], "cover": ["/Game/Art/Vegetation/SM_GrassClump_A", ...],
   "name": "vegetation", "distance": 1.0}
"cover" meshes are scattered densely in front of the lineup, as ground cover. "distance" scales the camera's distance.
Views: Wide (the whole lineup) and Close (eye height in the ground cover). Shots land in
Saved/Screenshots/Review/<name>_<view>_<quality>.png.
"""
import json
import math
import os
import random
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_tutorial_island as island  # noqa: E402

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SETTINGS = os.path.join(PROJECT, 'Saved/ReviewStage.json')
OUT = os.path.join(PROJECT, 'Saved/Screenshots/Review')
GROUND_SET = 'GroundGrass'

# The presets' own variables (UGraphicsSettingsSubsystem::QualityVariables) on top of the engine's levels.
QUALITY = {
    'Medium': (1, {'r.DynamicGlobalIlluminationMethod': 0, 'r.ReflectionMethod': 0, 'r.Nanite': 0,
                   'r.Shadow.Virtual.Enable': 0, 'r.Shadow.CSM.MaxCascades': 2, 'r.Shadow.MaxCSMResolution': 1536,
                   'r.Velocity.EnableVertexDeformation': 0,
                   'r.AmbientOcclusionLevels': 0, 'r.DistanceFieldAO': 0}),
    'High': (2, {'r.DynamicGlobalIlluminationMethod': 1, 'r.ReflectionMethod': 1, 'r.Nanite': 1,
                 'r.Shadow.Virtual.Enable': 1, 'r.AmbientOcclusionLevels': -1, 'r.DistanceFieldAO': 1}),
}

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def console(command):
    unreal.SystemLibrary.execute_console_command(None, command)


def import_set(name):
    """Imports a texture set the way the model importer does, if no model has brought it in yet."""
    for suffix, srgb, compression in (('BC', True, unreal.TextureCompressionSettings.TC_DEFAULT),
                                      ('N', False, unreal.TextureCompressionSettings.TC_NORMALMAP),
                                      ('ORM', False, unreal.TextureCompressionSettings.TC_MASKS)):
        path = f'/Game/Art/Textures/{name}/T_{name}_{suffix}'
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', os.path.join(PROJECT, f'Art/Textures/{name}/T_{name}_{suffix}.png'))
        task.set_editor_property('destination_path', f'/Game/Art/Textures/{name}')
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture = unreal.load_asset(path)
        texture.set_editor_property('srgb', srgb)
        texture.set_editor_property('compression_settings', compression)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)


def ground_material(component, tiles):
    """Gives the ground a grass-textured dynamic instance of M_World (nothing is saved)."""
    import_set(GROUND_SET)
    material = component.create_dynamic_material_instance(0, unreal.load_asset('/Game/Art/Materials/Masters/M_World'))
    for param, suffix in (('BaseColorMap', 'BC'), ('NormalMap', 'N'), ('ORMMap', 'ORM')):
        texture = unreal.load_asset(f'/Game/Art/Textures/{GROUND_SET}/T_{GROUND_SET}_{suffix}')
        if texture:
            material.set_texture_parameter_value(param, texture)
    material.set_scalar_parameter_value('UVScale', tiles)
    return material


def bounds(mesh):
    box = mesh.get_bounding_box()
    return box.min, box.max


def stage(settings):
    # A blank untitled map: nothing is written to disk, and leaving it never asks to save.
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    sky_light = island.environment()

    # The lineup, left to right, spaced by each model's width.
    meshes = [unreal.load_asset(p) for p in settings.get('meshes', [])]
    widths = [max(bounds(m)[1].y - bounds(m)[0].y, 50.0) for m in meshes]
    total = sum(widths) + 60.0 * max(len(meshes) - 1, 0)
    y = -total / 2
    tallest = 100.0
    for mesh, width in zip(meshes, widths):
        low, high = bounds(mesh)
        actors.spawn_actor_from_object(mesh, unreal.Vector(0, y + width / 2 - (low.y + high.y) / 2, 0))
        y += width + 60.0
        tallest = max(tallest, high.z)

    # Ground under all of it and well past the edges (the plane is 1 m across; its texture tiles every 2 m).
    size = max(total, 4000.0) * 2.0
    ground = actors.spawn_actor_from_object(unreal.load_asset('/Engine/BasicShapes/Plane'), unreal.Vector(0, 0, 0))
    ground.set_actor_scale3d(unreal.Vector(size / 100.0, size / 100.0, 1.0))
    ground_material(ground.static_mesh_component, size / 200.0)

    # Ground cover in front, several of each, as the scatter will put them.
    rng = random.Random(3)
    cover = [unreal.load_asset(p) for p in settings.get('cover', [])]
    for mesh in cover:
        for _ in range(60):
            a = actors.spawn_actor_from_object(mesh, unreal.Vector(rng.uniform(-900, -100), rng.uniform(-total / 2, total / 2), 0),
                                               unreal.Rotator(roll=0.0, pitch=0.0, yaw=rng.uniform(0, 360)))
            a.set_actor_scale3d(unreal.Vector(1, 1, 1) * rng.uniform(0.8, 1.3))

    # The cameras stand in front of the lineup, looking at it, the sun ahead and to the left.
    fov = 60.0
    span = max(total, tallest * 1.6)
    distance = span / 2 / math.tan(math.radians(fov / 2)) * 1.05 * settings.get('distance', 1.0)
    for label, location, pitch, view in (('Wide', (-distance, 0, tallest * 0.45 + 80), -6.0, fov),
                                         # Eye height in the ground cover, looking over it at the lineup.
                                         ('Close', (-1400, 0, 170), -7.0, 55.0)):
        camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*location),
                                               unreal.Rotator(roll=0.0, pitch=pitch, yaw=0.0))
        camera.set_actor_label(label)
        camera.camera_component.set_editor_property('field_of_view', view)
    sky_light.recapture_sky()


def shoot(camera, name, quality):
    level, variables = QUALITY[quality]
    console(f'scalability {level}')
    # The game renders every preset at full resolution (the engine's Medium alone would be 71%).
    console('r.ScreenPercentage 100')
    for key, value in variables.items():
        console(f'{key} {value}')
    os.makedirs(OUT, exist_ok=True)
    unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, os.path.join(OUT, f'{name}_{camera.get_actor_label()}_{quality}.png'), camera)


def restore():
    unreal.EditorLoadingAndSavingUtils.load_map(island.LEVEL)
    console('scalability 3')
    for key, value in {'r.DynamicGlobalIlluminationMethod': 1, 'r.ReflectionMethod': 1, 'r.Nanite': 1,
                       'r.Shadow.Virtual.Enable': 1, 'r.AmbientOcclusionLevels': -1, 'r.DistanceFieldAO': 1}.items():
        console(f'{key} {value}')


def run(args):
    with open(SETTINGS) as f:
        settings = json.load(f)
    if not args:
        stage(settings)
        unreal.log('Review: staged')
    elif args[0] == 'shoot':
        view = args[2] if len(args) > 2 else 'Wide'
        camera = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.CameraActor) and a.get_actor_label() == view)
        shoot(camera, settings.get('name', 'review'), args[1])
        unreal.log(f'Review: {args[1]} {view} shot requested')
    elif args[0] == 'restore':
        restore()
        unreal.log('Review: editor back on Epic and the island')


if __name__ == '__main__':
    run(sys.argv[1:])
