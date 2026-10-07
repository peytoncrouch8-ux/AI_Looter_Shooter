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
import hashlib
import importlib
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
            layout = json.load(f)
        level = layout.get('level', {})
        self.obstacles = layout.get('obstacles', [])
        with open(os.path.join(folder, 'layout_computed.json')) as f:
            self.data = json.load(f)
        # The dressing's pieces (build_area_dressing.py): instanced, and tagged Obstacle only in play (a map-wide actor's
        # bounds would clear the whole valley), so their own boxes keep the layers off them here.
        # Reloaded, as the editor keeps modules between runs.
        dressing = importlib.reload(importlib.import_module('build_area_dressing'))
        self.dressing = dressing.footprints(layout, self.data.get('placements', {}))
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
        # A grounded area's mask carries steep layers too (Art/Levels/area_scatter.py: computed's scatterMap.steep).
        self.steep = 'steep' in scatter
        self.graph = level.get('scatterGraph', f'PCG_{name}Scatter')
        self.volume = level.get('scatterVolume', f'{name}Scatter')
        self.folder = level.get('folder', name)


def import_mask(area):
    """The mask as exact, uncompressed values without mips (PCG reads it on the CPU; it never renders). It's imported
    again whenever the PNG changes: the PNG's MD5 is kept on the texture as metadata (SourceMD5). Importing only when the
    asset was missing once left a regenerated mask out of the scatter."""
    source = os.path.join(PROJECT, area.mask_file)
    with open(source, 'rb') as f:
        digest = hashlib.md5(f.read()).hexdigest()
    library = unreal.EditorAssetLibrary
    current = unreal.load_asset(area.mask) if library.does_asset_exist(area.mask) else None
    if current is None or library.get_metadata_tag(current, 'SourceMD5') != digest:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', source)
        task.set_editor_property('destination_path', area.mask.rsplit('/', 1)[0])
        task.set_editor_property('automated', True)
        task.set_editor_property('replace_existing', True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        unreal.log(f'Island scatter: {area.mask} imported from {area.mask_file} (MD5 {digest})')
    mask = unreal.load_asset(area.mask)
    mask.set_editor_property('srgb', False)
    mask.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
    mask.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    library.set_metadata_tag(mask, 'SourceMD5', digest)
    library.save_loaded_asset(mask)
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


# The newer layers' meshes, by name under VEGETATION: (mesh, weight, cull distance in cm). Each layer's list is its
# own table, so the art session's dry scrub kit for the slopes (Art/Models/Vegetation/Scrub.py: Sagebrush,
# Rabbitbrush, Juniper, DryTuft) can take over by editing them once it's imported.
SLOPE_TUFT_MESHES = (('GrassClump_A', 3, 4500), ('GrassClump_C', 2, 5000), ('TallGrass_A', 3, 5500))
SLOPE_BUSH_MESHES = (('Bush_A', 1, 9000), ('Bush_B', 1, 9000), ('Bush_C', 1, 9000))
CREASE_PINE_MESHES = (('Pine_A', 1, 0), ('Pine_B', 1, 0))
LARKSPUR_CLUMP_MESHES = (('Larkspur_A', 1, 6000),)
LARKSPUR_STRIP_MESHES = (('Larkspur_B', 1, 6000),)

# Larkspur along fences and walls (the art session's rules), by the obstacle it lines (layout.json obstacles): the share
# of spots left out, how far past the line's centre a wall's face adds (cm), and its gates [X, Y] (cm, from the
# obstacles' notes), kept LARKSPUR_GATE_CLEAR clear. Each side is walked, a spot every 80-140 cm: LARKSPUR_STRIP of them
# the strip (Larkspur_B, along the line within LARKSPUR_TURN degrees, 40-55 cm off it), the rest the clump
# (Larkspur_A, any heading, 35-60 cm off).
LARKSPUR = {
    'saltLine': (0.15, 0.0, ((-1100, -2400), (-1000, -8400))),  # Delia's salt line: the farm road's and keeper's gates
    'fieldWall': (0.55, 20.0, ()),                               # the stone field wall, sparser
    'amosFence': (0.55, 0.0, ((-3800, 5300),)),                  # Amos's fence, his gate
    'barnYardFence': (0.55, 0.0, ((-6000, 4800),)),              # the barn yard fence, its gate
}
LARKSPUR_GATE_CLEAR = 200.0
LARKSPUR_STRIP = 0.6
LARKSPUR_TURN = 12.0
LARKSPUR_LIFT = 3000.0  # cm: where the rays dropping them onto the ground start


def larkspur_points(obstacles):
    """The larkspur's spots along the LARKSPUR obstacles: the clumps (x, y, z) and the strips in runs, one per straight
    stretch of a line ((its heading in degrees, [(x, y, z), ...])). Deterministic."""
    rng = random.Random(53)
    clumps, runs = [], []
    for ob in obstacles:
        rule = LARKSPUR.get(ob['id'])
        if rule is None:
            continue
        skip, extra, gates = rule
        corners = [tuple(p) for p in ob.get('polygon') or ob.get('path', [])]
        if 'polygon' in ob and corners:
            corners.append(corners[0])
        for (x0, y0), (x1, y1) in zip(corners, corners[1:]):
            length = math.hypot(x1 - x0, y1 - y0)
            if length < 1.0:
                continue
            ux, uy = (x1 - x0) / length, (y1 - y0) / length
            strips = []
            for side in (-1.0, 1.0):
                along = rng.uniform(0.0, 60.0)
                while along < length:
                    left_out, kind, off, step = rng.random(), rng.random(), rng.random(), rng.uniform(80.0, 140.0)
                    if left_out >= skip:
                        strip = kind < LARKSPUR_STRIP
                        off = extra + (40.0 + 15.0 * off if strip else 35.0 + 25.0 * off)
                        px, py = x0 + ux * along - uy * side * off, y0 + uy * along + ux * side * off
                        if all(math.hypot(px - gx, py - gy) > LARKSPUR_GATE_CLEAR for gx, gy in gates):
                            (strips if strip else clumps).append((px, py, LARKSPUR_LIFT))
                    along += step
            if strips:
                runs.append((math.degrees(math.atan2(uy, ux)), strips))
    return clumps, runs


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

    def __init__(self, b, mask, half, lift=4000.0, ray=12000.0, dressing=()):
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
        # The dressing's boxes, a little wider than the pieces: every one for the woody and rocky layers, and only the
        # solid ones (props, graves, stacks) for grass and flowers, which grow up to a fence or a wall.
        self.dressing = {'all': self.boxes(b, 'Dressing', -4.0, dressing),
                         'solid': self.boxes(b, 'Dressing, solid', -4.5, [d for d in dressing if not d[5]])}
        # Texture space runs -1..1 across the mask. Its columns follow world Y and its rows run from north (+X) at
        # the top to south: a quarter turn and half the side of the square the mask covers.
        transform = unreal.Transform(location=unreal.Vector(0, 0, 0), rotation=unreal.Rotator(roll=0, pitch=0, yaw=90),
                                     scale=unreal.Vector(half, half, 1))
        self.channels = {}
        for i, (name, channel) in enumerate(CHANNELS.items()):
            self.channels[name], _ = b.node(unreal.PCGTextureSamplerSettings, f'Mask {name}', 3, -2 + i * 0.5,
                                            texture=mask, transform=transform, use_absolute_transform=True,
                                            color_channel=channel, filter=unreal.PCGTextureFilter.BILINEAR)

    @staticmethod
    def boxes(b, title, row, footprints, margin=40.0):
        """A point per footprint (x, y, yaw, half along Y, half along X, line), its bounds the piece's box on the ground
        plus margin, reaching far above and below; None without any."""
        if not footprints:
            return None
        points = []
        for i, (x, y, yaw, along, across, _) in enumerate(footprints):
            p = unreal.PCGPoint()
            p.set_editor_property('transform', unreal.Transform(location=unreal.Vector(x, y, 0.0),
                                                                rotation=unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)))
            p.set_editor_property('bounds_min', unreal.Vector(-across - margin, -along - margin, -20000.0))
            p.set_editor_property('bounds_max', unreal.Vector(across + margin, along + margin, 20000.0))
            p.set_editor_property('steepness', 1.0)
            p.set_editor_property('seed', i + 1)
            points.append(p)
        node, _ = b.node(unreal.PCGCreatePointsSettings, title, 3, row, points_to_create=points,
                         coordinate_space=unreal.PCGCoordinateSpace.WORLD)
        return node

    def layer(self, title, cell, channel, keep, flat=0.85, trees=False, flat_max=1.0, dressing='all'):
        """Points on the ground every `cell` cm where the mask's channel wins a random draw against `keep`, on ground
        whose flatness (the up component of its normal) is from `flat` to `flat_max` (below 1: steep ground only), off
        the obstacles and the dressing's boxes (`dressing`: 'all', or 'solid' for a layer that grows up to fences)."""
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
        level = b.node(unreal.PCGDensityFilterSettings, 'Flat enough' if flat_max >= 1.0 else 'Steep enough', 4, y,
                       lower_bound=flat, upper_bound=flat_max)[0]
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
        if self.dressing[dressing] is not None:
            b.link(self.dressing[dressing], clear, b_pin='Differences')
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

    def headed(self, title, runs, entries, scale, turn):
        """Points the script worked out itself in runs that share a heading ((yaw, [(x, y, z), ...]) each), dropped
        onto the ground, upright, each turned to its run's heading within turn degrees, and spawned together."""
        y = self.row * 3
        merge, _ = self.b.node(unreal.PCGMergeSettings, f'{title}: all', 10, y)
        for yaw, points in runs:
            dropped, row = self.listed(f'{title} {yaw:.0f}', points)
            look, _ = self.b.node(unreal.PCGTransformPointsSettings, f'{title} heading', 3, row,
                                  rotation_min=unreal.Rotator(0.0, 0.0, yaw - turn),
                                  rotation_max=unreal.Rotator(0.0, 0.0, yaw + turn), absolute_rotation=True,
                                  scale_min=unreal.Vector(scale[0], scale[0], scale[0]),
                                  scale_max=unreal.Vector(scale[1], scale[1], scale[1]), uniform_scale=True)
            self.b.link(dropped, look)
            self.b.link(look, merge)
        spawner, settings = self.b.node(unreal.PCGStaticMeshSpawnerSettings, f'Spawn {title}', 11, y)
        selector = settings.get_editor_property('mesh_selector_parameters')
        selector.set_editor_property('mesh_entries', entries)
        self.b.link(merge, spawner)

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
    s = Scatter(b, mask, area.half, area.lift, area.ray, area.dressing)
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

    # Larkspur along Delia's salt line, and sparser along the field wall and the farms' fences.
    clumps, runs = larkspur_points(area.obstacles)
    clump = [entry(m, w, cull) for m, w, cull in ((veg(n, False), w, c) for n, w, c in LARKSPUR_CLUMP_MESHES) if m]
    strip = [entry(m, w, cull) for m, w, cull in ((veg(n, False), w, c) for n, w, c in LARKSPUR_STRIP_MESHES) if m]
    if clumps and clump:
        clump_points, y = s.listed('Larkspur clumps', clumps)
        s.spawn(clump_points, 'Larkspur clumps', 11, y, clump, scale=(0.85, 1.15))
    if runs and strip:
        s.headed('Larkspur strips', runs, strip, (0.85, 1.15), LARKSPUR_TURN)

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
    grass, y = s.layer('Grass', 90.0, 'G', 0.12, dressing='solid')
    s.spawn(grass, 'Grass', 11, y, [entry(veg('GrassClump_A'), 4, 4500, density_scaling=True),
                                    entry(veg('GrassClump_B'), 3, 4500, density_scaling=True),
                                    entry(veg('GrassClump_C'), 3, 5000, density_scaling=True),
                                    entry(veg('TallGrass_A'), 2, 5500, density_scaling=True),
                                    entry(veg('Clover_A'), 1, 3500, density_scaling=True)],
            scale=(0.8, 1.25), upright=False, fit=45.0)

    flowers, y = s.layer('Flowers', 230.0, 'B', 0.3, dressing='solid')
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

    if area.steep:
        steep_layers(s, veg)

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    return graph


def steep_layers(s, veg):
    """A grounded area's steep ground, from 32 to 50 degrees (flatness 0.85 to 0.64), which none of the layers above
    reach: the mask's G there is slope tufts, its B low bushes, and its R, past the tree stands' 37 degrees, pines in
    the ridges' creases (Art/Levels/area_scatter.py's steep layers and Art/Levels/area_faces.py). Tufts and bushes
    climb the ridges' faces from their foot and creases, thinning upward; the pines stand few and in groups. Past the
    boundary the creases' trees on gentler ground come from the tree stands above. Their meshes are the tables
    SLOPE_TUFT_MESHES, SLOPE_BUSH_MESHES and CREASE_PINE_MESHES."""
    tufts, y = s.layer('Slope tufts', 170.0, 'G', 0.15, flat=0.64, flat_max=0.85)
    s.spawn(tufts, 'Slope tufts', 11, y, [entry(veg(n), w, cull, density_scaling=True)
                                          for n, w, cull in SLOPE_TUFT_MESHES],
            scale=(0.85, 1.3), upright=False, fit=35.0)
    shrubs, y = s.layer('Slope bushes', 350.0, 'B', 0.15, flat=0.64, flat_max=0.85)
    s.spawn(shrubs, 'Slope bushes', 11, y, [entry(veg(n), w, cull, shadow=True) for n, w, cull in SLOPE_BUSH_MESHES],
            scale=(0.6, 1.0), sink=20.0)
    creases, y = s.layer('Crease pines', 700.0, 'R', 0.15, flat=0.64, flat_max=0.8, trees=True)
    s.spawn(creases, 'Crease pines', 11, y, [entry(veg(n), w, cull, collide=True, shadow=True, tree=True)
                                             for n, w, cull in CREASE_PINE_MESHES], sink=40.0)


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
