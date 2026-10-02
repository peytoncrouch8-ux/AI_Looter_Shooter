"""Builds an area's PCG scatter graph (/Game/Environment/PCG/PCG_IslandScatter on the tutorial island; layout.json
level.scatterGraph) and runs it over the area: grass, flowers, trees (in stands and lone in the meadows), bushes, forest
undergrowth, rocks, pebbles, and reeds and lily pads on the pond and creek (placed from the layout).

Where things grow comes from the area's scatter mask, T_<Area>Scatter (painted from the layout by
Art/Models/Terrain/<Area>.py, so it agrees with the roads, water and buildings): R trees, G grass, B flowers, A pebbles
and rocks. layout_computed.json macroMap.scatterMap names it and the square it covers. Each layer is its own chain:
  grid of ray origins -> jitter -> raycast onto actors tagged Ground -> flat enough -> not inside an Obstacle
  -> density = its mask channel -> x random -> keep above a threshold -> look (size, heading, slope) -> spawn instances
A higher threshold thins a layer everywhere, and the mask's soft edges fade it out. Spatial noise splits the trees
into stands of pines and of broadleaf trees.

The graph is rebuilt from scratch (edits in the PCG editor are lost). Run it in the open editor with the area's level
open (the area defaults to TutorialIsland):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_island_scatter.py [Area]"
Then save the level. After changing the terrain or the mask, select the scatter volume (IslandScatter on the tutorial
island; layout.json level.scatterVolume) and press Generate.
"""
import json
import math
import os
import random
import sys

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
GRAPH_FOLDER = '/Game/Environment/PCG'
VEGETATION = '/Game/Art/Vegetation'
ROCKS = '/Game/Art/Rocks'
# Beyond this distance (cm) foliage stops swaying (saves vertex work far away).
WIND_DISTANCE = 4000

CHANNELS = {'R': unreal.PCGTextureColorChannel.RED, 'G': unreal.PCGTextureColorChannel.GREEN,
            'B': unreal.PCGTextureColorChannel.BLUE, 'A': unreal.PCGTextureColorChannel.ALPHA}


class Area:
    """What the scatter needs to know about an area: its layout_computed.json, the scatter mask (its file, its asset
    under /Game/Art/Textures, the square it covers) and, from layout.json's level, the graph and volume names."""

    def __init__(self, name):
        folder = os.path.join(PROJECT, 'Art', 'Levels', name)
        with open(os.path.join(folder, 'layout.json')) as f:
            level = json.load(f).get('level', {})
        with open(os.path.join(folder, 'layout_computed.json')) as f:
            self.data = json.load(f)
        scatter = self.data['macroMap']['scatterMap']
        self.mask_file = scatter['texture']
        # Art/Textures/<Set>/<File>.png is imported as /Game/Art/Textures/<Set>/<File>.
        self.mask = '/Game/' + os.path.splitext(self.mask_file)[0]
        (x0, y0), (x1, y1) = scatter.get('covers', self.data['macroMap']['covers'])
        # The grid of ray origins spreads around the world origin, so the square must be centered there.
        if abs(x0 + x1) > 1.0 or abs(y0 + y1) > 1.0 or abs((x1 - x0) - (y1 - y0)) > 1.0:
            raise ValueError(f'{name}: the scatter mask covers {[[x0, y0], [x1, y1]]}, not a square around the origin')
        self.half = (x1 - x0) / 2.0
        # The rays start above the area's highest ground and reach below its lowest (layout_computed.json heightRange:
        # a grounded area's ridges stand far higher than anything on the tutorial island); never less than the
        # island's 40 m lift and 120 m reach.
        low, high = self.data.get('heightRange', [-8000.0, 3000.0])
        self.lift = max(4000.0, high + 1000.0)
        self.ray = max(12000.0, self.lift - low + 1000.0)
        self.graph = level.get('scatterGraph', f'PCG_{name}Scatter')
        self.volume = level.get('scatterVolume', f'{name}Scatter')
        self.folder = level.get('folder', name)


def import_mask(area):
    """The mask as exact, uncompressed values without mips (PCG reads it on the CPU; it never renders)."""
    if not unreal.EditorAssetLibrary.does_asset_exist(area.mask):
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', os.path.join(PROJECT, area.mask_file))
        task.set_editor_property('destination_path', area.mask.rsplit('/', 1)[0])
        task.set_editor_property('automated', True)
        task.set_editor_property('replace_existing', True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mask = unreal.load_asset(area.mask)
    mask.set_editor_property('srgb', False)
    mask.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
    mask.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    unreal.EditorAssetLibrary.save_loaded_asset(mask)
    return mask


def mesh(folder, name, required=True):
    path = f'{folder}/SM_{name}'
    asset = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if asset is None and required:
        raise RuntimeError(f'missing {path}')
    return asset


def point(x, y, z, seed):
    p = unreal.PCGPoint()
    p.set_editor_property('transform', unreal.Transform(location=unreal.Vector(x, y, z)))
    p.set_editor_property('seed', seed)
    return p


def shore_points(data):
    """Reed clumps around the pond's edge (a little into the shallows) and along both creek banks, clear of the bridge
    and the waterfall's lip; lily pads in a few drifts on the pond. Deterministic, from the computed layout; an area
    without a pond or a creek gets none there."""
    rng = random.Random(41)
    pond, creek = data.get('pond'), data.get('creek')
    away = [(p['location'][:2], clear) for p, clear in ((data.get('bridge'), 600), (data.get('waterfall'), 400)) if p]
    reeds = []
    if pond:
        (cx, cy), (rx, ry) = pond['center'], pond['radii']
        steps = 64
        for i in range(steps):
            angle = 2 * math.pi * (i + rng.random() * 0.6) / steps
            if rng.random() < 0.3:
                continue
            f = rng.uniform(0.9, 1.0)
            reeds.append((cx + rx * f * math.cos(angle), cy + ry * f * math.sin(angle)))
    if creek:
        half = creek['waterWidth'] / 2
        for (x0, y0, _), (x1, y1, _) in zip(creek['points'], creek['points'][1:]):
            length = math.hypot(x1 - x0, y1 - y0) or 1.0
            nx, ny = -(y1 - y0) / length, (x1 - x0) / length
            for side in (-1, 1):
                if rng.random() < 0.55:
                    offset = side * (half + rng.uniform(30, 90))
                    reeds.append((x0 + nx * offset, y0 + ny * offset))
    reeds = [p for p in reeds if all(math.dist(p, where) > clear for where, clear in away)]
    pads = []
    if pond:
        for _ in range(3):
            angle, f = rng.uniform(0, 2 * math.pi), rng.uniform(0.3, 0.7)
            px, py = cx + rx * f * math.cos(angle), cy + ry * f * math.sin(angle)
            for _ in range(rng.randint(3, 6)):
                pads.append((px + rng.uniform(-220, 220), py + rng.uniform(-220, 220)))
    return reeds, pads, pond['waterZ'] if pond else 0.0


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


def entry(asset, weight, cull, collide=False, shadow=False, density_scaling=False, wind=True, tree=False):
    d = unreal.PCGSoftISMComponentDescriptor()
    d.set_editor_property('static_mesh', asset)
    # PCG's instances don't collide unless told to: say which way, both ways. Colliding ones block like the buildings
    # (BlockAll) with their mesh's hulls (a tree's trunk, a rock); without this, trees and rocks were walk-through.
    d.set_editor_property('use_default_collision', False)
    body = d.get_editor_property('body_instance')
    body.set_editor_property('collision_profile_name', 'BlockAll' if collide else 'NoCollision')
    body.set_editor_property('collision_enabled', unreal.CollisionEnabled.QUERY_AND_PHYSICS if collide
                             else unreal.CollisionEnabled.NO_COLLISION)
    d.set_editor_property('body_instance', body)
    d.set_editor_property('cast_shadow', shadow)
    d.set_editor_property('instance_end_cull_distance', cull)
    d.set_editor_property('enable_density_scaling', density_scaling)
    if wind:
        d.set_editor_property('world_position_offset_disable_distance', WIND_DISTANCE)
    d.set_editor_property('can_ever_affect_navigation', collide)
    if tree:
        # The minimap draws a crown for every component tagged Tree.
        d.set_editor_property('component_tags', [unreal.Name('Tree')])
    e = unreal.PCGMeshSelectorWeightedEntry()
    e.set_editor_property('descriptor', d)
    e.set_editor_property('weight', weight)
    return e


class Scatter:
    """The shared inputs (obstacles, the mask's channels) and one chain per layer."""

    def __init__(self, b, mask, half, lift=4000.0, ray=12000.0):
        self.b = b
        self.half = half
        self.lift, self.ray = lift, ray  # how high over the ground the rays start, and how far down they reach (cm)
        self.row = 0
        self.obstacles, settings = b.node(unreal.PCGDataFromActorSettings, 'Obstacles', 3, -3,
                                          mode=unreal.PCGGetDataFromActorMode.GET_SINGLE_POINT)
        selector = settings.get_editor_property('actor_selector')
        selector.set_editor_property('actor_filter', unreal.PCGActorFilter.ALL_WORLD_ACTORS)
        selector.set_editor_property('actor_selection', unreal.PCGActorSelection.BY_TAG)
        selector.set_editor_property('actor_selection_tag', 'Obstacle')
        selector.set_editor_property('select_multiple', True)
        settings.set_editor_property('actor_selector', selector)
        # Boxes the island builder puts where no tree may stand (the target range, the village square).
        self.no_trees, settings = b.node(unreal.PCGDataFromActorSettings, 'No trees', 3, -3.5,
                                         mode=unreal.PCGGetDataFromActorMode.GET_SINGLE_POINT)
        selector = settings.get_editor_property('actor_selector')
        selector.set_editor_property('actor_filter', unreal.PCGActorFilter.ALL_WORLD_ACTORS)
        selector.set_editor_property('actor_selection', unreal.PCGActorSelection.BY_TAG)
        selector.set_editor_property('actor_selection_tag', 'NoTrees')
        selector.set_editor_property('select_multiple', True)
        settings.set_editor_property('actor_selector', selector)
        # Texture space runs -1..1 across the mask. Its columns follow world Y and its rows run from north (+X) at
        # the top to south: a quarter turn and half the side of the square the mask covers.
        transform = unreal.Transform(location=unreal.Vector(0, 0, 0), rotation=unreal.Rotator(roll=0, pitch=0, yaw=90),
                                     scale=unreal.Vector(half, half, 1))
        self.channels = {}
        for i, (name, channel) in enumerate(CHANNELS.items()):
            self.channels[name], _ = b.node(unreal.PCGTextureSamplerSettings, f'Mask {name}', 3, -2 + i * 0.5,
                                            texture=mask, transform=transform, use_absolute_transform=True,
                                            color_channel=channel, filter=unreal.PCGTextureFilter.BILINEAR)

    def layer(self, title, cell, channel, keep, flat=0.85, trees=False):
        """Points on the ground every `cell` cm where the mask's channel wins a random draw against `keep`."""
        b, y = self.b, self.row * 3
        self.row += 1
        grid, _ = b.node(unreal.PCGCreatePointsGridSettings, f'{title}: ray origins', 0, y,
                         grid_extents=unreal.Vector(self.half, self.half, 0.0),
                         cell_size=unreal.Vector(cell, cell, 100.0),
                         coordinate_space=unreal.PCGCoordinateSpace.WORLD,
                         point_position=unreal.PCGPointPosition.CELL_CENTER)
        lift, _ = b.node(unreal.PCGTransformPointsSettings, 'Jitter, lift', 1, y,
                         offset_min=unreal.Vector(-cell / 2, -cell / 2, self.lift),
                         offset_max=unreal.Vector(cell / 2, cell / 2, self.lift))
        ray, ray_settings = b.node(unreal.PCGWorldRaycastElementSettings, 'Onto Ground', 2, y,
                                   raycast_mode=unreal.PCGWorldRaycastMode.NORMALIZED_WITH_LENGTH,
                                   ray_direction=unreal.Vector(0.0, 0.0, -1.0), ray_length=self.ray)
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
        if trees:
            b.link(self.no_trees, clear, b_pin='Differences')
        b.link(clear, sample, b_pin='Point')
        b.link(self.channels[channel], sample, b_pin='BaseTexture')
        b.link(sample, draw)
        b.link(draw, kept)
        return kept, y

    def listed(self, title, points, onto_ground=True):
        """Points the script worked out itself (x, y, z), dropped onto the ground when onto_ground."""
        b, y = self.b, self.row * 3
        self.row += 1
        made, _ = b.node(unreal.PCGCreatePointsSettings, f'{title}: points', 0, y,
                         points_to_create=[point(x, yy, z, i + 1) for i, (x, yy, z) in enumerate(points)],
                         coordinate_space=unreal.PCGCoordinateSpace.WORLD)
        if not onto_ground:
            return made, y
        ray, ray_settings = b.node(unreal.PCGWorldRaycastElementSettings, 'Onto Ground', 2, y,
                                   raycast_mode=unreal.PCGWorldRaycastMode.NORMALIZED_WITH_LENGTH,
                                   ray_direction=unreal.Vector(0.0, 0.0, -1.0), ray_length=12000.0)
        query = ray_settings.get_editor_property('world_query_params')
        query.set_editor_property('actor_tag_filter', unreal.PCGWorldQueryFilter.INCLUDE)
        query.set_editor_property('actor_tags_list', 'Ground')
        query.set_editor_property('ignore_pcg_hits', True)
        ray_settings.set_editor_property('world_query_params', query)
        b.link(made, ray, b_pin='Origins')
        return ray, y

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


def build_graph(area, mask):
    path = f'{GRAPH_FOLDER}/{area.graph}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    graph = unreal.AssetToolsHelpers.get_asset_tools().create_asset(area.graph, GRAPH_FOLDER, unreal.PCGGraph,
                                                                    unreal.PCGGraphFactory())
    b = Builder(graph)
    s = Scatter(b, mask, area.half, area.lift, area.ray)
    veg = lambda name, required=True: mesh(VEGETATION, name, required)  # noqa: E731
    rock = lambda name: mesh(ROCKS, name)  # noqa: E731

    # Trees: stands of pines and of broadleaf trees, upright, colliding, shadowed, never culled.
    trees, y = s.layer('Trees', 600.0, 'R', 0.12, flat=0.8, trees=True)
    pines, broadleaf = s.split(trees, y, 2.5, 311.0, 0.5)
    s.spawn(pines, 'Pines', 11, y - 0.5, [entry(veg('Pine_A'), 1, 0, collide=True, shadow=True, tree=True),
                                          entry(veg('Pine_B'), 1, 0, collide=True, shadow=True, tree=True)], sink=15.0)
    s.spawn(broadleaf, 'Broadleaf', 11, y + 0.5, [entry(veg(n), w, 0, collide=True, shadow=True, tree=True)
                                                  for n, w in (('Oak_A', 3), ('Oak_B', 3), ('Birch_A', 3),
                                                               ('Birch_B', 2), ('DeadTree_A', 0.4))], sink=15.0)

    # Lone trees and the odd pair out in the meadows, where the grass grows; the big field oak when it exists.
    meadow, y = s.layer('Meadow trees', 1300.0, 'G', 0.8, flat=0.85, trees=True)
    lone = [(veg(n, False), w) for n, w in (('Oak_A', 3), ('Oak_B', 3), ('Birch_A', 2), ('Birch_B', 1), ('Pine_A', 1),
                                            ('Oak_C', 3))]
    s.spawn(meadow, 'Meadow trees', 11, y, [entry(m, w, 0, collide=True, shadow=True, tree=True) for m, w in lone if m],
            scale=(0.9, 1.25), sink=15.0)

    # Reeds along the pond and the creek, lily pads on the pond.
    reeds, pads, water = shore_points(area.data)
    reed_points, y = s.listed('Reeds', [(x, yy, 2000.0) for x, yy in reeds])
    reed_meshes = [m for m in (veg('Reeds_A', False), veg('Reeds_B', False)) if m]
    s.spawn(reed_points, 'Reeds', 11, y, [entry(m, 1, 5000) for m in reed_meshes], scale=(0.8, 1.3))
    if veg('LilyPads_A', False):
        pad_points, y = s.listed('Lily pads', [(x, yy, water + 1.0) for x, yy in pads], onto_ground=False)
        s.spawn(pad_points, 'Lily pads', 11, y, [entry(veg('LilyPads_A'), 1, 4000)], scale=(0.8, 1.2))

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


def place_volume(area, graph):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    volume = next((a for a in actors.get_all_level_actors()
                   if isinstance(a, unreal.PCGVolume) and a.get_actor_label() == area.volume), None)
    if volume is None:
        volume = actors.spawn_actor_from_class(unreal.PCGVolume, unreal.Vector(0.0, 0.0, 0.0))
        volume.set_actor_label(area.volume)
        volume.set_folder_path(f'{area.folder}/Scatter')
    # The brush is 200 cm across: cover the mask's square and the area's heights.
    volume.set_actor_scale3d(unreal.Vector(area.half / 100.0, area.half / 100.0, max(60.0, area.lift / 100.0 + 10.0)))
    component = volume.get_component_by_class(unreal.PCGComponent)
    component.set_editor_property('generation_trigger', unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND)
    component.set_graph(graph)
    component.generate_local(True)
    return volume


if __name__ == '__main__':
    AREA = Area(sys.argv[1] if len(sys.argv) > 1 else 'TutorialIsland')
    graph = build_graph(AREA, import_mask(AREA))
    volume = place_volume(AREA, graph)
    unreal.log(f'Island scatter: graph {graph.get_path_name()}, volume {volume.get_path_name()}; generating')
