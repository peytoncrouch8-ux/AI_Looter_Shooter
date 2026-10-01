"""Builds the tutorial island level, /Game/Maps/Lvl_TutorialIsland, from its layout.

Art/Models/Terrain/TutorialIsland.py turns Art/Levels/TutorialIsland/layout.json into the terrain and into
layout_computed.json: where every building, cliff piece, road, the bridge and the water go, at the built terrain's
heights. This script places all of that in the level, with the new style's lighting and the gameplay actors (spawn,
target dummies, spiders). Run it in the open editor:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_tutorial_island.py"
Everything it places carries the IslandBuild tag and sits under the Island outliner folder. Building again replaces
those actors, so actors placed by hand survive. Models that aren't imported yet are skipped with a warning. The level
is saved at the end. Grass, flowers, trees and rocks come from the scatter (build_island_scatter.py).
"""
import json
import math
import os
import random

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
LAYOUT = os.path.join(PROJECT, 'Art/Levels/TutorialIsland/layout_computed.json')
LEVEL = '/Game/Maps/Lvl_TutorialIsland'
ART = '/Game/Art'
DUMMY = '/Game/Combat/Blueprints/BP_TargetDummy'
TAG = 'IslandBuild'

# Where the spiders live, and how many.
SPIDER_ZONE = 'forest'
SPIDER_COUNT = 8

# The terrain's models are SM_TutorialIsland_<part> (Art/Models/Terrain/TutorialIsland.py).
TERRAIN_PREFIX = 'TutorialIsland_'
# The cliff kit (Art/Models/Rocks/Cliffs.py), and how the pieces sit on the terrain's walls (cm): how far inside the
# wall's foot a piece stands, how far over the top it reaches, and how much neighbours overlap.
CLIFF_PIECES = ('CliffFace_A', 'CliffFace_B', 'CliffFace_C', 'CliffFace_D')
CLIFF_INSET = 120.0
CLIFF_OVERTOP = 20.0
CLIFF_OVERLAP = 250.0

# The sky's clouds: a dome of this radius (cm) around the island with the painted cloud material.
SKY_DOME = '/Engine/EngineSky/SM_SkySphere'
SKY_CLOUDS = '/Game/Art/Materials/Masters/M_SkyClouds'
SKY_RADIUS = 100000.0

# Placement kinds whose model name differs from the kind (the rest are SM_<kind>).
KIND_MODELS = {
    'GunRack': 'GunRack',
}

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def log(message):
    unreal.log(f'Island: {message}')


def warn(message):
    unreal.log_warning(f'Island: {message}')


def mesh_index():
    """Every static mesh under /Game/Art by its name without SM_."""
    index = {}
    for path in unreal.EditorAssetLibrary.list_assets(ART, recursive=True, include_folder=False):
        name = path.rsplit('/', 1)[-1].split('.')[0]
        if name.startswith('SM_'):
            index[name[3:]] = path.split('.')[0]
    return index


def open_level():
    """Opens (or makes) the level and removes what the last build placed. Stops if another level has unsaved edits."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world.get_path_name().split('.')[0] != LEVEL:
        # Untitled scratch maps (/Temp, such as review_stage.py's) are never saved, so they don't count.
        unsaved = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
                   if not p.get_name().startswith('/Temp/')]
        if unsaved:
            raise RuntimeError(f'unsaved changes in {unsaved}: save or discard them first')
        if unreal.EditorAssetLibrary.does_asset_exist(LEVEL):
            # Loads without asking about the scratch maps.
            unreal.EditorLoadingAndSavingUtils.load_map(LEVEL)
        else:
            levels.new_level(LEVEL)
    built = [a for a in actors.get_all_level_actors() if unreal.Name(TAG) in a.tags]
    if built:
        actors.destroy_actors(built)
        log(f'removed {len(built)} actors from the last build')


def place(what, location, yaw=0.0, label=None, folder='', scale=None, tags=()):
    """Spawns an actor class or a static mesh (as a static mesh actor) and tags it as built."""
    loc = unreal.Vector(*location)
    rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)
    if isinstance(what, unreal.StaticMesh):
        actor = actors.spawn_actor_from_object(what, loc, rot)
    else:
        actor = actors.spawn_actor_from_class(what, loc, rot)
    if scale is not None:
        actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_editor_property('tags', [unreal.Name(TAG)] + [unreal.Name(t) for t in tags])
    actor.set_folder_path(f'Island/{folder}' if folder else 'Island')
    if label:
        actor.set_actor_label(label)
    return actor


def component(actor, cls):
    return actor.get_component_by_class(cls)


def environment():
    """Warm afternoon light from the west-northwest, so the view from the spawn (looking northeast) is lit from the side."""
    sun = place(unreal.DirectionalLight, (0, 0, 3000), label='Sun', folder='Environment')
    sun.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-38.0, yaw=112.0), False)
    light = component(sun, unreal.DirectionalLightComponent)
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    for name, value in (('intensity', 7.0), ('use_temperature', True), ('temperature', 5300.0),
                        ('atmosphere_sun_light', True), ('cast_cloud_shadows', False),
                        # Cascaded shadows (Low and Medium) reach 100 m before the preset's scale (70 m on Medium).
                        ('dynamic_shadow_distance_movable_light', 10000.0), ('dynamic_shadow_cascades', 2)):
        light.set_editor_property(name, value)

    sky = place(unreal.SkyLight, (0, 0, 2000), label='SkyLight', folder='Environment')
    sky_light = component(sky, unreal.SkyLightComponent)
    sky_light.set_mobility(unreal.ComponentMobility.MOVABLE)
    # Captured once when the level loads rather than every frame (0.12 ms on Medium): the sun never moves.
    for name, value in (('real_time_capture', False), ('source_type', unreal.SkyLightSourceType.SLS_CAPTURED_SCENE),
                        ('intensity', 1.2), ('lower_hemisphere_is_black', False),
                        # Light bouncing off the meadow.
                        ('lower_hemisphere_color', unreal.LinearColor(0.26, 0.30, 0.18, 1.0))):
        sky_light.set_editor_property(name, value)

    place(unreal.SkyAtmosphere, (0, 0, 0), label='SkyAtmosphere', folder='Environment')

    fog = place(unreal.ExponentialHeightFog, (0, 0, -2000), label='HeightFog', folder='Environment')
    fog_component = component(fog, unreal.ExponentialHeightFogComponent)
    for name, value in (('fog_density', 0.03), ('fog_height_falloff', 0.12), ('start_distance', 3000.0),
                        ('fog_max_opacity', 0.85),
                        ('fog_inscattering_luminance', unreal.LinearColor(0.20, 0.29, 0.44, 1.0))):
        fog_component.set_editor_property(name, value)

    # Painted clouds on a dome 1 km around the island (M_SkyClouds). Volumetric clouds cost Medium 2 ms and more,
    # looking up through their layer, and thinned out enough to be cheap they vanished.
    dome_mesh = unreal.load_asset(SKY_DOME)
    radius = max(dome_mesh.get_bounding_box().max.x, 1.0)
    dome = place(dome_mesh, (0, 0, 0), label='SkyClouds', folder='Environment', scale=(SKY_RADIUS / radius,) * 3)
    sky_mesh = dome.static_mesh_component
    sky_mesh.set_material(0, unreal.load_asset(SKY_CLOUDS))
    sky_mesh.set_editor_property('cast_shadow', False)
    sky_mesh.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    post = place(unreal.PostProcessVolume, (0, 0, 0), label='IslandPost', folder='Environment')
    post.set_editor_property('unbound', True)
    settings = post.get_editor_property('settings')
    # No outline material: the new style draws no ink lines.
    for name, value in (('auto_exposure_method', unreal.AutoExposureMethod.AEM_HISTOGRAM), ('auto_exposure_bias', 0.4),
                        # Exposure adapts only a little (EV100 0.3 to 1): shade under the trees stays
                        # shade instead of brightening to look like the open meadow.
                        ('auto_exposure_min_brightness', 0.3), ('auto_exposure_max_brightness', 1.0),
                        ('bloom_intensity', 0.6), ('vignette_intensity', 0.3), ('film_slope', 0.88), ('film_toe', 0.55),
                        ('color_saturation', unreal.Vector4(1.05, 1.05, 1.05, 1.0))):
        settings.set_editor_property(name, value)
        settings.set_editor_property(f'override_{name}', True)
    post.set_editor_property('settings', settings)
    return sky_light


def ground_height(x, y, default):
    """The terrain's height under (x, y), by a trace onto the placed terrain."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -20000.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        return default
    return hit.to_tuple()[5].z


def inside(point, polygon):
    x, y = point
    result = False
    for i in range(len(polygon)):
        (x1, y1), (x2, y2) = polygon[i], polygon[i - 1]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
            result = not result
    return result


def gameplay(layout, source):
    """The spawn, the target dummies and the spiders."""
    dummy = unreal.EditorAssetLibrary.load_blueprint_class(DUMMY)
    spider = unreal.load_class(None, '/Script/AI_Looter_Shooter.SpiderCreature')
    for key, spot in layout['placements'].items():
        x, y, z = spot['location']
        if spot['kind'] == 'PlayerStart':
            place(unreal.PlayerStart, (x, y, z + 100.0), spot['yaw'], label='PlayerStart', folder='Gameplay')
        elif spot['kind'] == 'TargetDummy':
            place(dummy, (x, y, z), spot['yaw'], label=f'TargetDummy_{key[-1]}', folder='Gameplay')

    zone = next(z for z in source['zones'] if z['id'] == SPIDER_ZONE)['polygon']
    xs, ys = [p[0] for p in zone], [p[1] for p in zone]
    rng = random.Random(7)
    spots = []
    while len(spots) < SPIDER_COUNT:
        point = (rng.uniform(min(xs), max(xs)), rng.uniform(min(ys), max(ys)))
        if inside(point, zone) and all(math.dist(point, s) > 900.0 for s in spots):
            spots.append(point)
    for i, (x, y) in enumerate(spots):
        place(spider, (x, y, ground_height(x, y, 0.0) + 60.0), rng.uniform(-180.0, 180.0),
              label=f'Spider_{i + 1:02d}', folder='Gameplay')


def models(layout, meshes):
    """Buildings, structures and props at their placements, and the orchard's apple trees."""
    placed = 0
    for key, spot in layout['placements'].items():
        kind = spot['kind']
        if kind in ('PlayerStart', 'TargetDummy'):
            continue
        name = KIND_MODELS.get(kind, kind)
        if name not in meshes:
            warn(f'no SM_{name} yet (placement {key})')
            continue
        if kind == 'Windmill':
            # The fan turns: AWindmill hangs it from the tower's Fan socket.
            windmill = place(unreal.load_class(None, '/Script/AI_Looter_Shooter.Windmill'), spot['location'],
                             spot['yaw'], label=key, folder='Buildings', tags=('Obstacle',))
            tower = windmill.get_editor_property('tower')
            tower.set_static_mesh(unreal.load_asset(meshes[name]))
            if 'WindmillFan' in meshes:
                fan = windmill.get_editor_property('fan')
                fan.set_static_mesh(unreal.load_asset(meshes['WindmillFan']))
                # The socket exists only now that the tower has its mesh.
                snap = unreal.AttachmentRule.SNAP_TO_TARGET
                fan.attach_to_component(tower, 'Fan', snap, snap, unreal.AttachmentRule.KEEP_RELATIVE, False)
                # Attaching doesn't move it in the editor until its transform changes (setting the same one is skipped).
                fan.set_relative_location(unreal.Vector(0.0, 0.0, 1.0), False, True)
                fan.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, True)
        else:
            place(unreal.load_asset(meshes[name]), spot['location'], spot['yaw'], label=key, folder='Buildings',
                  tags=('Obstacle',))
        placed += 1

    # The bridge's ramps end at its pivot's height, which the layout gives as the road on both banks; it stretches to
    # the span. Untagged, so the minimap draws it as ground but the scatter doesn't grow grass on it.
    bridge = layout.get('bridge')
    if bridge and 'Bridge' in meshes:
        deck = unreal.load_asset(meshes['Bridge'])
        box = deck.get_bounding_box()
        stretch = bridge['span'] / max(box.max.x - box.min.x, 1.0)
        place(deck, bridge['location'], bridge['yaw'], label='Bridge', folder='Buildings', scale=(stretch, 1.0, 1.0))
        placed += 1

    apple = unreal.load_asset(meshes['Apple_A']) if 'Apple_A' in meshes else None
    rng = random.Random(11)
    for r, row in enumerate(layout.get('orchardRows', [])):
        for t, tree in enumerate(row['trees']):
            if apple:
                place(apple, tree, rng.uniform(-180.0, 180.0), label=f'AppleTree_{r + 1}_{t + 1}', folder='Orchard',
                      tags=('Obstacle',))
                placed += 1
    log(f'placed {placed} models')


def terrain(meshes):
    """The terrain tiles (walkable, tagged Ground for the minimap and the scatter), the rock underside and the water.
    They are all modeled in island space, so they sit at the origin."""
    count = 0
    for name, path in sorted(meshes.items()):
        if not name.startswith(TERRAIN_PREFIX):
            continue
        part = name[len(TERRAIN_PREFIX):]
        place(unreal.load_asset(path), (0, 0, 0), label=part, folder='Terrain',
              tags=('Ground',) if part.startswith('Tile_') else ())
        count += 1
    if not count:
        warn(f'no SM_{TERRAIN_PREFIX}* terrain yet')
    log(f'placed {count} terrain pieces')


def cliffs(layout, meshes):
    """Cliff faces over the terrain's steep walls: the plateau's edge, the ramp's cut walls and the island's rim.

    Each dressing point is where a wall meets the ground below it (the rim: the rim's top, with the drop below it),
    facing out. A piece stands a little inside that line so it covers the wall and its lip; it reaches just over the
    top, is widened to overlap its neighbours, and is chosen among the kit's pieces by how little it must stretch."""
    pieces = []
    for name in CLIFF_PIECES:
        if name in meshes:
            mesh = unreal.load_asset(meshes[name])
            box = mesh.get_bounding_box()
            pieces.append((mesh, box.max.z, box.max.y - box.min.y))
    if not pieces:
        warn('no cliff pieces yet')
        return
    rng = random.Random(23)
    placed = 0
    for group, points in layout.get('cliffs', {}).items():
        for i, point in enumerate(points):
            x, y, z = point['location']
            if group == 'rim':
                bottom, height = z - point['drop'], point['drop']
            else:
                bottom, height = z, point.get('height', point.get('top', z) - z)
            height += CLIFF_OVERTOP
            # Neighbours along the wall set the width (the points run along it in order).
            gaps = [math.dist(point['location'][:2], points[j]['location'][:2]) for j in (i - 1, i + 1)
                    if 0 <= j < len(points)]
            gap = min(gaps) if gaps else 1000.0
            choices = sorted(pieces, key=lambda p: abs(math.log(height / p[1])) + rng.uniform(0.0, 0.25))
            mesh, piece_height, piece_width = choices[0]
            yaw = point['yaw']
            inward = (-math.cos(math.radians(yaw)) * CLIFF_INSET, -math.sin(math.radians(yaw)) * CLIFF_INSET)
            width = min(max((gap + CLIFF_OVERLAP) / piece_width, 0.75), 1.6)
            place(mesh, (x + inward[0], y + inward[1], bottom), yaw + rng.uniform(-4.0, 4.0),
                  label=f'Cliff_{group}_{i + 1:02d}', folder=f'Cliffs/{group}',
                  scale=(1.0, width, height / piece_height), tags=('Obstacle',))
            placed += 1
    log(f'placed {placed} cliff pieces')


def run():
    with open(LAYOUT) as f:
        layout = json.load(f)
    with open(os.path.join(os.path.dirname(LAYOUT), 'layout.json')) as f:
        source = json.load(f)
    open_level()
    meshes = mesh_index()
    sky_light = environment()
    terrain(meshes)
    cliffs(layout, meshes)
    models(layout, meshes)
    gameplay(layout, source)
    sky_light.recapture_sky()
    levels.save_current_level()
    log('built and saved')


if __name__ == '__main__':
    run()
