"""Builds /Game/Environment/PCG/PCG_Meadow and puts a PCG volume running it over Lvl_Skyreach's island.

The graph scatters grass, poppy fields and marigold patches over Ground-tagged terrain:
  grid of ray origins above the volume -> jitter -> raycast down onto actors tagged Ground -> flat enough (slope)
  -> not inside anything tagged Obstacle -> Perlin fields split poppies / marigolds / grass -> upright, random yaw
  and size -> spawn instanced meshes (no collision, no shadow, outline-free stencil, density scaling on).

The graph asset is what levels use; this rebuilds it from scratch (edits made in the PCG editor are lost) and
regenerates the volume labeled Meadow. Run it in the open editor with Unreal Python:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_meadow.py"
Then save the level. After moving terrain, just select the Meadow volume and press Generate.
"""
import unreal

GRAPH_FOLDER = '/Game/Environment/PCG'
GRAPH_NAME = 'PCG_Meadow'
PROPS = '/Game/Environment/Props'

# Spacing of the ray grid (cm) and how much of the plain grass survives thinning.
CELL = 450.0
GRASS_KEEP = 0.62
# Noise scale 1 = features about 100 m across.
POPPY_NOISE_SCALE = 3.5
POPPY_THRESHOLD = 0.66
MARIGOLD_NOISE_SCALE = 5.0
MARIGOLD_THRESHOLD = 0.72
# Beyond this distance (cm) grass stops swaying (saves shading work far away).
WIND_DISTANCE = 5000


def load_mesh(kind, variant):
    path = f'{PROPS}/{kind}/SM_{kind}_V{variant}'
    mesh = unreal.load_asset(path)
    if mesh is None:
        raise RuntimeError(f'missing {path}')
    return mesh


def entry(mesh, weight, cull):
    d = unreal.PCGSoftISMComponentDescriptor()
    d.set_editor_property('static_mesh', mesh)
    d.set_editor_property('use_default_collision', False)
    body = d.get_editor_property('body_instance')
    body.set_editor_property('collision_profile_name', 'NoCollision')
    body.set_editor_property('collision_enabled', unreal.CollisionEnabled.NO_COLLISION)
    d.set_editor_property('body_instance', body)
    d.set_editor_property('cast_shadow', False)
    d.set_editor_property('render_custom_depth', True)
    d.set_editor_property('custom_depth_stencil_value', 1)
    d.set_editor_property('instance_end_cull_distance', cull)
    d.set_editor_property('enable_density_scaling', True)
    d.set_editor_property('world_position_offset_disable_distance', WIND_DISTANCE)
    d.set_editor_property('can_ever_affect_navigation', False)
    e = unreal.PCGMeshSelectorWeightedEntry()
    e.set_editor_property('descriptor', d)
    e.set_editor_property('weight', weight)
    return e


class Builder:
    def __init__(self, graph):
        self.graph = graph

    def node(self, settings_class, title, x, y, **props):
        node, settings = self.graph.add_node_of_type(settings_class)
        for key, value in props.items():
            settings.set_editor_property(key, value)
        try:
            node.set_node_title(title)
        except Exception:
            pass
        try:
            node.set_node_position(x * 400, y * 250)
        except Exception:
            pass
        return node, settings

    def link(self, a, b, a_pin='Out', b_pin='In'):
        self.graph.add_edge(a, a_pin, b, b_pin)


def density_filter(b, title, x, y, low, high):
    return b.node(unreal.PCGDensityFilterSettings, title, x, y, lower_bound=low, upper_bound=high)[0]


def noise(b, title, x, y, scale, offset):
    node, s = b.node(unreal.PCGSpatialNoiseSettings, title, x, y, mode=unreal.PCGSpatialNoiseMode.PERLIN2D)
    s.set_editor_property('transform', unreal.Transform(scale=unreal.Vector(scale, scale, 1.0)))
    s.set_editor_property('random_offset', unreal.Vector(offset, offset * 0.37, 0.0))
    return node


def look(b, title, x, y, scale_min, scale_max):
    # Upright (the raycast tilts points to the slope), any heading, a little size variation.
    return b.node(unreal.PCGTransformPointsSettings, title, x, y,
                  rotation_min=unreal.Rotator(0.0, 0.0, 0.0), rotation_max=unreal.Rotator(0.0, 0.0, 360.0),
                  absolute_rotation=True, scale_min=unreal.Vector(scale_min, scale_min, scale_min),
                  scale_max=unreal.Vector(scale_max, scale_max, scale_max), uniform_scale=True)[0]


def spawner(b, title, x, y, entries):
    node, s = b.node(unreal.PCGStaticMeshSpawnerSettings, title, x, y)
    selector = s.get_editor_property('mesh_selector_parameters')
    selector.set_editor_property('mesh_entries', entries)
    return node


def build_graph():
    path = f'{GRAPH_FOLDER}/{GRAPH_NAME}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    graph = unreal.AssetToolsHelpers.get_asset_tools().create_asset(GRAPH_NAME, GRAPH_FOLDER, unreal.PCGGraph, unreal.PCGGraphFactory())
    b = Builder(graph)

    grid, _ = b.node(unreal.PCGCreatePointsGridSettings, 'Ray origins', 0, 0,
                     grid_extents=unreal.Vector(8000.0, 8000.0, 0.0), cell_size=unreal.Vector(CELL, CELL, 100.0),
                     coordinate_space=unreal.PCGCoordinateSpace.LOCAL_COMPONENT, point_position=unreal.PCGPointPosition.CELL_CENTER)
    lift, _ = b.node(unreal.PCGTransformPointsSettings, 'Jitter, lift to the top', 1, 0,
                     offset_min=unreal.Vector(-CELL / 2, -CELL / 2, 1200.0), offset_max=unreal.Vector(CELL / 2, CELL / 2, 1200.0))
    ray, ray_settings = b.node(unreal.PCGWorldRaycastElementSettings, 'Onto Ground', 2, 0,
                               raycast_mode=unreal.PCGWorldRaycastMode.NORMALIZED_WITH_LENGTH,
                               ray_direction=unreal.Vector(0.0, 0.0, -1.0), ray_length=3000.0)
    query = ray_settings.get_editor_property('world_query_params')
    query.set_editor_property('actor_tag_filter', unreal.PCGWorldQueryFilter.INCLUDE)
    query.set_editor_property('actor_tags_list', 'Ground')
    query.set_editor_property('ignore_pcg_hits', True)
    ray_settings.set_editor_property('world_query_params', query)

    slope, _ = b.node(unreal.PCGNormalToDensitySettings, 'Slope', 3, 0)
    flat = density_filter(b, 'Flat enough', 4, 0, 0.9, 1.0)

    obstacles, obstacle_settings = b.node(unreal.PCGDataFromActorSettings, 'Obstacles', 4, 1, mode=unreal.PCGGetDataFromActorMode.GET_SINGLE_POINT)
    selector = obstacle_settings.get_editor_property('actor_selector')
    selector.set_editor_property('actor_filter', unreal.PCGActorFilter.ALL_WORLD_ACTORS)
    selector.set_editor_property('actor_selection', unreal.PCGActorSelection.BY_TAG)
    selector.set_editor_property('actor_selection_tag', 'Obstacle')
    selector.set_editor_property('select_multiple', True)
    obstacle_settings.set_editor_property('actor_selector', selector)
    clear, _ = b.node(unreal.PCGDifferenceSettings, 'Not in obstacles', 5, 0, density_function=unreal.PCGDifferenceDensityFunction.BINARY)

    fields = noise(b, 'Poppy fields', 6, 0, POPPY_NOISE_SCALE, 1731.0)
    poppies = density_filter(b, 'Poppies', 7, -1, POPPY_THRESHOLD, 1.0)
    not_poppies = density_filter(b, 'Not poppies', 7, 1, 0.0, POPPY_THRESHOLD - 0.0001)
    marigold_noise = noise(b, 'Marigold patches', 8, 1, MARIGOLD_NOISE_SCALE, 5923.0)
    marigolds = density_filter(b, 'Marigolds', 9, 0, MARIGOLD_THRESHOLD, 1.0)
    rest = density_filter(b, 'Grass', 9, 2, 0.0, MARIGOLD_THRESHOLD - 0.0001)
    thin, _ = b.node(unreal.PCGAttributeNoiseSettings, 'Random', 10, 2, mode=unreal.PCGAttributeNoiseMode.SET, noise_min=0.0, noise_max=1.0)
    keep = density_filter(b, 'Keep some', 11, 2, 0.0, GRASS_KEEP)

    poppy_look = look(b, 'Poppy look', 12, -1, 0.9, 1.35)
    marigold_look = look(b, 'Marigold look', 12, 0, 0.9, 1.3)
    grass_look = look(b, 'Grass look', 12, 2, 0.85, 1.35)

    spawn_poppies = spawner(b, 'Spawn poppies', 13, -1, [entry(load_mesh('PoppyField', v), 1, 14000) for v in range(1, 7)])
    spawn_marigolds = spawner(b, 'Spawn marigolds', 13, 0, [entry(load_mesh('Marigolds', v), 1, 14000) for v in range(1, 5)])
    spawn_grass = spawner(b, 'Spawn grass', 13, 2,
                          [entry(load_mesh('GrassPatch', v), 12, 9000) for v in range(1, 7)]
                          + [entry(load_mesh('TallGrass', v), 5, 12000) for v in range(1, 7)]
                          + [entry(load_mesh('WildGrass', v), 3, 12000) for v in range(1, 5)])

    b.link(grid, lift)
    b.link(lift, ray, b_pin='Origins')
    b.link(ray, slope)
    b.link(slope, flat)
    b.link(flat, clear, b_pin='Source')
    b.link(obstacles, clear, b_pin='Differences')
    b.link(clear, fields)
    b.link(fields, poppies)
    b.link(fields, not_poppies)
    b.link(poppies, poppy_look)
    b.link(poppy_look, spawn_poppies)
    b.link(not_poppies, marigold_noise)
    b.link(marigold_noise, marigolds)
    b.link(marigold_noise, rest)
    b.link(marigolds, marigold_look)
    b.link(marigold_look, spawn_marigolds)
    b.link(rest, thin)
    b.link(thin, keep)
    b.link(keep, grass_look)
    b.link(grass_look, spawn_grass)

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    return graph


def place_volume(graph):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    volume = None
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.PCGVolume) and actor.get_actor_label() == 'Meadow':
            volume = actor
    if volume is None:
        volume = actors.spawn_actor_from_class(unreal.PCGVolume, unreal.Vector(0.0, 0.0, 0.0))
        volume.set_actor_label('Meadow')
        volume.set_folder_path('Props')
    # The brush is 200 cm across: cover the island (160 m) and 30 m of height.
    volume.set_actor_scale3d(unreal.Vector(80.0, 80.0, 15.0))
    component = volume.get_component_by_class(unreal.PCGComponent)
    component.set_editor_property('generation_trigger', unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND)
    component.set_graph(graph)
    component.generate_local(True)
    return volume


graph = build_graph()
volume = place_volume(graph)
unreal.log(f'LOOTER meadow: graph {graph.get_path_name()}, volume {volume.get_path_name()}; generating')
