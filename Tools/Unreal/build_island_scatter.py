"""Builds /Game/Environment/PCG/PCG_IslandScatter and runs it over the tutorial island: grass, flowers, trees, bushes,
forest undergrowth, rocks and pebbles.

Where things grow comes from the island's scatter mask, T_TutorialIslandScatter (painted from the layout by
Art/Models/Terrain/TutorialIsland.py, so it agrees with the roads, water and buildings): R trees, G grass, B flowers,
A pebbles and rocks. Each layer is its own chain:
  grid of ray origins -> jitter -> raycast onto actors tagged Ground -> flat enough -> not inside an Obstacle
  -> density = its mask channel -> x random -> keep above a threshold -> look (size, heading, slope) -> spawn instances
A higher threshold thins a layer everywhere, and the mask's soft edges fade it out. Spatial noise splits the trees
into stands of pines and of broadleaf trees.

The graph is rebuilt from scratch (edits in the PCG editor are lost). Run it in the open editor with the island open:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_island_scatter.py"
Then save the level. After changing the terrain or the mask, select the IslandScatter volume and press Generate.
"""
import os

import unreal

GRAPH_FOLDER = '/Game/Environment/PCG'
GRAPH_NAME = 'PCG_IslandScatter'
VEGETATION = '/Game/Art/Vegetation'
ROCKS = '/Game/Art/Rocks'
MASK_FILE = 'Art/Textures/TutorialIslandMacro/T_TutorialIslandScatter_BC.png'
MASK = '/Game/Art/Textures/TutorialIslandMacro/T_TutorialIslandScatter_BC'
# The mask covers this square around the island's center (layout_computed.json macroMap.covers).
HALF = 10240.0
# Beyond this distance (cm) foliage stops swaying (saves vertex work far away).
WIND_DISTANCE = 4000

CHANNELS = {'R': unreal.PCGTextureColorChannel.RED, 'G': unreal.PCGTextureColorChannel.GREEN,
            'B': unreal.PCGTextureColorChannel.BLUE, 'A': unreal.PCGTextureColorChannel.ALPHA}


def import_mask():
    """The mask as exact, uncompressed values without mips (PCG reads it on the CPU; it never renders)."""
    if not unreal.EditorAssetLibrary.does_asset_exist(MASK):
        project = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', os.path.join(project, MASK_FILE))
        task.set_editor_property('destination_path', MASK.rsplit('/', 1)[0])
        task.set_editor_property('automated', True)
        task.set_editor_property('replace_existing', True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mask = unreal.load_asset(MASK)
    mask.set_editor_property('srgb', False)
    mask.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
    mask.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    unreal.EditorAssetLibrary.save_loaded_asset(mask)
    return mask


def mesh(folder, name):
    path = f'{folder}/SM_{name}'
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f'missing {path}')
    return asset


class Builder:
    def __init__(self, graph):
        self.graph = graph

    def node(self, settings_class, title, x, y, **props):
        node, settings = self.graph.add_node_of_type(settings_class)
        for key, value in props.items():
            settings.set_editor_property(key, value)
        try:
            node.set_node_title(title)
            node.set_node_position(int(x * 400), int(y * 250))
        except Exception:
            pass
        return node, settings

    def link(self, a, b, a_pin='Out', b_pin='In'):
        self.graph.add_edge(a, a_pin, b, b_pin)


def entry(asset, weight, cull, collide=False, shadow=False, density_scaling=False, wind=True):
    d = unreal.PCGSoftISMComponentDescriptor()
    d.set_editor_property('static_mesh', asset)
    if not collide:
        d.set_editor_property('use_default_collision', False)
        body = d.get_editor_property('body_instance')
        body.set_editor_property('collision_profile_name', 'NoCollision')
        body.set_editor_property('collision_enabled', unreal.CollisionEnabled.NO_COLLISION)
        d.set_editor_property('body_instance', body)
    d.set_editor_property('cast_shadow', shadow)
    d.set_editor_property('instance_end_cull_distance', cull)
    d.set_editor_property('enable_density_scaling', density_scaling)
    if wind:
        d.set_editor_property('world_position_offset_disable_distance', WIND_DISTANCE)
    d.set_editor_property('can_ever_affect_navigation', collide)
    e = unreal.PCGMeshSelectorWeightedEntry()
    e.set_editor_property('descriptor', d)
    e.set_editor_property('weight', weight)
    return e


class Scatter:
    """The shared inputs (obstacles, the mask's channels) and one chain per layer."""

    def __init__(self, b, mask):
        self.b = b
        self.row = 0
        self.obstacles, settings = b.node(unreal.PCGDataFromActorSettings, 'Obstacles', 3, -3,
                                          mode=unreal.PCGGetDataFromActorMode.GET_SINGLE_POINT)
        selector = settings.get_editor_property('actor_selector')
        selector.set_editor_property('actor_filter', unreal.PCGActorFilter.ALL_WORLD_ACTORS)
        selector.set_editor_property('actor_selection', unreal.PCGActorSelection.BY_TAG)
        selector.set_editor_property('actor_selection_tag', 'Obstacle')
        selector.set_editor_property('select_multiple', True)
        settings.set_editor_property('actor_selector', selector)
        # Texture space runs -1..1 across the mask. Its columns follow world Y and its rows run from north (+X) at
        # the top to south: a quarter turn and the island's half size.
        transform = unreal.Transform(location=unreal.Vector(0, 0, 0), rotation=unreal.Rotator(roll=0, pitch=0, yaw=90),
                                     scale=unreal.Vector(HALF, HALF, 1))
        self.channels = {}
        for i, (name, channel) in enumerate(CHANNELS.items()):
            self.channels[name], _ = b.node(unreal.PCGTextureSamplerSettings, f'Mask {name}', 3, -2 + i * 0.5,
                                            texture=mask, transform=transform, use_absolute_transform=True,
                                            color_channel=channel, filter=unreal.PCGTextureFilter.BILINEAR)

    def layer(self, title, cell, channel, keep, flat=0.85):
        """Points on the ground every `cell` cm where the mask's channel wins a random draw against `keep`."""
        b, y = self.b, self.row * 3
        self.row += 1
        grid, _ = b.node(unreal.PCGCreatePointsGridSettings, f'{title}: ray origins', 0, y,
                         grid_extents=unreal.Vector(HALF, HALF, 0.0), cell_size=unreal.Vector(cell, cell, 100.0),
                         coordinate_space=unreal.PCGCoordinateSpace.WORLD,
                         point_position=unreal.PCGPointPosition.CELL_CENTER)
        lift, _ = b.node(unreal.PCGTransformPointsSettings, 'Jitter, lift', 1, y,
                         offset_min=unreal.Vector(-cell / 2, -cell / 2, 4000.0),
                         offset_max=unreal.Vector(cell / 2, cell / 2, 4000.0))
        ray, ray_settings = b.node(unreal.PCGWorldRaycastElementSettings, 'Onto Ground', 2, y,
                                   raycast_mode=unreal.PCGWorldRaycastMode.NORMALIZED_WITH_LENGTH,
                                   ray_direction=unreal.Vector(0.0, 0.0, -1.0), ray_length=12000.0)
        query = ray_settings.get_editor_property('world_query_params')
        query.set_editor_property('actor_tag_filter', unreal.PCGWorldQueryFilter.INCLUDE)
        query.set_editor_property('actor_tags_list', 'Ground')
        query.set_editor_property('ignore_pcg_hits', True)
        ray_settings.set_editor_property('world_query_params', query)
        slope, _ = b.node(unreal.PCGNormalToDensitySettings, 'Slope', 3, y)
        level = b.node(unreal.PCGDensityFilterSettings, 'Flat enough', 4, y, lower_bound=flat, upper_bound=1.0)[0]
        clear, _ = b.node(unreal.PCGDifferenceSettings, 'Not in obstacles', 5, y,
                          density_function=unreal.PCGDifferenceDensityFunction.BINARY)
        sample, _ = b.node(unreal.PCGSampleTextureSettings, f'Mask {channel}', 6, y,
                           density_merge_function=unreal.PCGDensityMergeOperation.SET)
        draw, _ = b.node(unreal.PCGAttributeNoiseSettings, 'x random', 7, y, mode=unreal.PCGAttributeNoiseMode.MULTIPLY,
                         noise_min=0.0, noise_max=1.0)
        kept = b.node(unreal.PCGDensityFilterSettings, 'Keep', 8, y, lower_bound=keep, upper_bound=1.0)[0]
        b.link(grid, lift)
        b.link(lift, ray, b_pin='Origins')
        b.link(ray, slope)
        b.link(slope, level)
        b.link(level, clear, b_pin='Source')
        b.link(self.obstacles, clear, b_pin='Differences')
        b.link(clear, sample, b_pin='Point')
        b.link(self.channels[channel], sample, b_pin='BaseTexture')
        b.link(sample, draw)
        b.link(draw, kept)
        return kept, y

    def split(self, source, y, scale, offset, threshold):
        """Two outputs by spatial noise: stands of one kind and of the other."""
        b = self.b
        noise, s = b.node(unreal.PCGSpatialNoiseSettings, 'Stands', 9, y, mode=unreal.PCGSpatialNoiseMode.PERLIN2D)
        s.set_editor_property('transform', unreal.Transform(scale=unreal.Vector(scale, scale, 1.0)))
        s.set_editor_property('random_offset', unreal.Vector(offset, offset * 0.37, 0.0))
        b.link(source, noise)
        high = b.node(unreal.PCGDensityFilterSettings, 'Above', 10, y - 0.5, lower_bound=threshold, upper_bound=1.0)[0]
        low = b.node(unreal.PCGDensityFilterSettings, 'Below', 10, y + 0.5, lower_bound=0.0,
                     upper_bound=threshold - 0.0001)[0]
        b.link(noise, high)
        b.link(noise, low)
        return high, low

    def spawn(self, source, title, x, y, entries, scale=(0.85, 1.2), upright=True, fit=None, sink=0.0):
        """Heading (and, for ground cover, the slope), size, and the instanced meshes."""
        b = self.b
        look, _ = b.node(unreal.PCGTransformPointsSettings, f'{title} look', x, y,
                         rotation_min=unreal.Rotator(0.0, 0.0, 0.0), rotation_max=unreal.Rotator(0.0, 0.0, 360.0),
                         absolute_rotation=upright, scale_min=unreal.Vector(scale[0], scale[0], scale[0]),
                         scale_max=unreal.Vector(scale[1], scale[1], scale[1]), uniform_scale=True,
                         offset_min=unreal.Vector(0, 0, -sink), offset_max=unreal.Vector(0, 0, -sink))
        b.link(source, look)
        last = look
        if fit:
            # World/PCGGroundFitFilter: drops patches whose edge would hang off an edge, presses floating ones down.
            last, _ = b.node(unreal.PCGGroundFitFilterSettings, f'{title} fit', x + 0.5, y, radius=fit, max_sink=20.0)
            b.link(look, last)
        spawner, s = b.node(unreal.PCGStaticMeshSpawnerSettings, f'Spawn {title}', x + 1, y)
        selector = s.get_editor_property('mesh_selector_parameters')
        selector.set_editor_property('mesh_entries', entries)
        b.link(last, spawner)


def build_graph(mask):
    path = f'{GRAPH_FOLDER}/{GRAPH_NAME}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    graph = unreal.AssetToolsHelpers.get_asset_tools().create_asset(GRAPH_NAME, GRAPH_FOLDER, unreal.PCGGraph,
                                                                    unreal.PCGGraphFactory())
    b = Builder(graph)
    s = Scatter(b, mask)
    veg = lambda name: mesh(VEGETATION, name)  # noqa: E731
    rock = lambda name: mesh(ROCKS, name)  # noqa: E731

    # Trees: stands of pines and of broadleaf trees, upright, colliding, shadowed, never culled.
    trees, y = s.layer('Trees', 600.0, 'R', 0.12, flat=0.8)
    pines, broadleaf = s.split(trees, y, 2.5, 311.0, 0.5)
    s.spawn(pines, 'Pines', 11, y - 0.5, [entry(veg('Pine_A'), 1, 0, collide=True, shadow=True),
                                          entry(veg('Pine_B'), 1, 0, collide=True, shadow=True)], sink=15.0)
    s.spawn(broadleaf, 'Broadleaf', 11, y + 0.5, [entry(veg(n), w, 0, collide=True, shadow=True)
                                                  for n, w in (('Oak_A', 3), ('Oak_B', 3), ('Birch_A', 3),
                                                               ('Birch_B', 2), ('DeadTree_A', 0.4))], sink=15.0)

    # Bushes at the forest's edges and in its gaps, and a few out in the meadow.
    bushes, y = s.layer('Bushes', 420.0, 'R', 0.1)
    s.spawn(bushes, 'Bushes', 11, y, [entry(veg(n), 1, 9000, shadow=True) for n in ('Bush_A', 'Bush_B', 'Bush_C')],
            scale=(0.75, 1.25), sink=5.0)

    # Under the trees: ferns, with stumps and fallen logs.
    floor, y = s.layer('Forest floor', 260.0, 'R', 0.3)
    s.spawn(floor, 'Forest floor', 11, y, [entry(veg('Fern_A'), 8, 5000, density_scaling=True),
                                           entry(veg('Stump_A'), 1, 8000, collide=True, shadow=True, wind=False),
                                           entry(veg('Log_A'), 1, 8000, collide=True, shadow=True, wind=False)],
            scale=(0.8, 1.2), upright=False)

    # Grass: a patch about every 0.8 m², tall grass among it, clover in the gaps.
    grass, y = s.layer('Grass', 90.0, 'G', 0.12)
    s.spawn(grass, 'Grass', 11, y, [entry(veg('GrassClump_A'), 4, 4500, density_scaling=True),
                                    entry(veg('GrassClump_B'), 3, 4500, density_scaling=True),
                                    entry(veg('GrassClump_C'), 3, 5000, density_scaling=True),
                                    entry(veg('TallGrass_A'), 2, 5500, density_scaling=True),
                                    entry(veg('Clover_A'), 1, 3500, density_scaling=True)],
            scale=(0.8, 1.25), upright=False, fit=45.0)

    flowers, y = s.layer('Flowers', 230.0, 'B', 0.3)
    s.spawn(flowers, 'Flowers', 11, y, [entry(veg(n), 1, 4500, density_scaling=True)
                                        for n in ('Flowers_Yellow', 'Flowers_White', 'Flowers_Purple')],
            scale=(0.85, 1.2), upright=False, fit=35.0)

    # Stones: pebbles along the road edges and cliff feet, rocks among them, a few boulders.
    pebbles, y = s.layer('Pebbles', 170.0, 'A', 0.1, flat=0.7)
    s.spawn(pebbles, 'Pebbles', 11, y, [entry(rock('PebbleCluster_A'), 1, 3000, density_scaling=True, wind=False)],
            scale=(0.8, 1.3), upright=False)
    rocks, y = s.layer('Rocks', 500.0, 'A', 0.15, flat=0.7)
    s.spawn(rocks, 'Rocks', 11, y, [entry(rock(n), 1, 9000, collide=True, shadow=True, wind=False)
                                    for n in ('Rock_A', 'Rock_B', 'Rock_C', 'Rock_D')],
            scale=(0.7, 1.4), sink=4.0)
    boulders, y = s.layer('Boulders', 1200.0, 'A', 0.15, flat=0.7)
    s.spawn(boulders, 'Boulders', 11, y, [entry(rock(n), 1, 0, collide=True, shadow=True, wind=False)
                                          for n in ('Boulder_A', 'Boulder_B', 'Boulder_C')],
            scale=(0.8, 1.3), sink=10.0)

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    return graph


def place_volume(graph):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    volume = next((a for a in actors.get_all_level_actors()
                   if isinstance(a, unreal.PCGVolume) and a.get_actor_label() == 'IslandScatter'), None)
    if volume is None:
        volume = actors.spawn_actor_from_class(unreal.PCGVolume, unreal.Vector(0.0, 0.0, 0.0))
        volume.set_actor_label('IslandScatter')
        volume.set_folder_path('Island/Scatter')
    # The brush is 200 cm across: cover the mask's square and the island's heights.
    volume.set_actor_scale3d(unreal.Vector(HALF / 100.0, HALF / 100.0, 60.0))
    component = volume.get_component_by_class(unreal.PCGComponent)
    component.set_editor_property('generation_trigger', unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND)
    component.set_graph(graph)
    component.generate_local(True)
    return volume


if __name__ == '__main__':
    graph = build_graph(import_mask())
    volume = place_volume(graph)
    unreal.log(f'Island scatter: graph {graph.get_path_name()}, volume {volume.get_path_name()}; generating')
