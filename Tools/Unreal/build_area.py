"""Builds an area's level from its layout: build_area.py <Area> [gameplay|beyond].

Art/Models/Terrain/<Area>.py turns Art/Levels/<Area>/layout.json into the terrain and into layout_computed.json: where
every building, cliff piece, road, the bridge and the water go, at the built terrain's heights. This script places all
of that in the area's level, with the new style's lighting and the gameplay actors. Run it in the open editor:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area.py TutorialIsland [gameplay]"
What differs per area comes from layout.json: "level" (the map, the tag and outliner folder of everything placed, the
zones kept free of scattered trees) and "gameplay" (the director that comes with the PlayerStart, the gun rack's weapon,
creature groups). The terrain's meshes are SM_<Area>_<part> (level.meshPrefix overrides <Area>_).

Everything it places carries the area's tag (IslandBuild on the tutorial island) and sits under its outliner folder
(Island). Building again replaces those actors, so actors placed by hand survive; with "gameplay" it only places the
gameplay actors again, and with "beyond" only what lies past the boundary (sky islands, a grounded area's ring, canyon
wall and backdrop). Models that aren't imported yet are skipped with a warning. The level is saved at the end.
Grass, flowers, trees and rocks come from the scatter (build_island_scatter.py <Area>).

A grounded area's terrain also has what lies past its core (Art/Levels/area_beyond.py): the surround ring
(SM_<Area>_Ring_<n>), the canyon wall (_CanyonWall_<n>) and the backdrop (_Backdrop_<n>). They go in the Beyond
folder, tagged Beyond (Looter.Perf.HideTag measures them by difference); only the core's tiles are tagged Ground, so the
minimap covers the valley alone. Cliff points with stacked courses get one piece per course; a knob's point places the
outcrop kit's piece it names (SM_Outcrop_<piece>), and a gully's sloped banks get no faces. Its playable area, KillZ
and cull distance volume come from build_area_bounds.py; every area's light, sky and fog from build_area_environment.py;
the skiff jetty, the depot's station and the landings trips arrive at from build_area_travel.py; the story's actors
(the cold open's set, the family plot's grave, the headboards, Delia's door, Hob) from build_area_story.py; the wanted
posters and Calder's note (layout.json gameplay.posters) from build_area_posters.py.
"""
import importlib
import json
import math
import os
import random
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_bounds  # noqa: E402
import build_area_environment  # noqa: E402
import build_area_posters  # noqa: E402
import build_area_story  # noqa: E402
import build_area_travel  # noqa: E402

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
ART = '/Game/Art'
DUMMY = '/Game/Combat/Blueprints/BP_TargetDummy'
CLASSES = '/Script/AI_Looter_Shooter.'
# The weapon a gun rack offers when the layout names none.
RACK_WEAPON = '/Game/Weapons/Data/DA_AssaultRifle'

# The cliff kit (Art/Models/Rocks/Cliffs.py), and how the pieces sit on the terrain's walls (cm): how far inside the
# wall's foot a piece stands, how far over the top it reaches, and how much neighbours overlap.
CLIFF_PIECES = ('CliffFace_A', 'CliffFace_B', 'CliffFace_C', 'CliffFace_D')
CLIFF_INSET = 120.0
CLIFF_OVERTOP = 20.0
CLIFF_OVERLAP = 250.0
CLIFF_COURSE_OVERLAP = 30.0  # a lower course reaches this far up behind the one stacked on it
# What lies past a grounded area's core: its terrain parts by name, none of them Ground.
BEYOND_PARTS = ('Ring_', 'CanyonWall_', 'Backdrop_')
# Drops the waterfall model (made for the tutorial island's rim, its strands thinning out 40-58 m down) is shortened
# for: a falls into a gorge is squashed to fit (cm).
WATERFALL_LENGTH = 4800.0

# The way smoke leans (the foliage master's default WindDirection, 1 : 0.35).
WIND_YAW = 19.0

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def mesh_index():
    """Every static mesh under /Game/Art by its name without SM_."""
    index = {}
    for path in unreal.EditorAssetLibrary.list_assets(ART, recursive=True, include_folder=False):
        name = path.rsplit('/', 1)[-1].split('.')[0]
        if name.startswith('SM_'):
            index[name[3:]] = path.split('.')[0]
    return index


def component(actor, cls):
    return actor.get_component_by_class(cls)



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


class AreaBuild:
    """One area's level build: its settings from Art/Levels/<name>/layout.json, and the steps that place it."""

    def __init__(self, name):
        self.name = name
        folder = os.path.join(PROJECT, 'Art', 'Levels', name)
        with open(os.path.join(folder, 'layout.json')) as f:
            self.source = json.load(f)
        self.computed_path = os.path.join(folder, 'layout_computed.json')
        level = self.source.get('level', {})
        self.level = level.get('map', f'/Game/Maps/Lvl_{name}')
        self.tag = level.get('tag', f'{name}Build')
        self.folder = level.get('folder', name)
        self.prefix = level.get('meshPrefix', f'{name}_')
        self.tree_free_zones = level.get('noTreeZones', [])
        self.settings = self.source.get('gameplay', {})
        self.layout = None  # layout_computed.json, read by run()

    def log(self, message):
        unreal.log(f'{self.folder}: {message}')

    def warn(self, message):
        unreal.log_warning(f'{self.folder}: {message}')

    def open_level(self, folder=None):
        """Opens (or makes) the level and removes what the last build placed (only in the outliner folder
        <area folder>/<folder> when given). Stops if another level has unsaved edits."""
        world = unreal.EditorLevelLibrary.get_editor_world()
        if world.get_path_name().split('.')[0] != self.level:
            # Untitled scratch maps (/Temp, such as review_stage.py's) are never saved, so they don't count.
            unsaved = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
                       if not p.get_name().startswith('/Temp/')]
            if unsaved:
                raise RuntimeError(f'unsaved changes in {unsaved}: save or discard them first')
            if unreal.EditorAssetLibrary.does_asset_exist(self.level):
                # Loads without asking about the scratch maps.
                unreal.EditorLoadingAndSavingUtils.load_map(self.level)
            else:
                levels.new_level(self.level)
        built = [a for a in actors.get_all_level_actors() if unreal.Name(self.tag) in a.tags
                 and (folder is None or str(a.get_folder_path()) == f'{self.folder}/{folder}')]
        if built:
            actors.destroy_actors(built)
            self.log(f'removed {len(built)} actors from the last build')

    def place(self, what, location, yaw=0.0, label=None, folder='', scale=None, tags=()):
        """Spawns an actor class or a static mesh (as a static mesh actor) and tags it as built."""
        loc = unreal.Vector(*location)
        rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)
        if isinstance(what, unreal.StaticMesh):
            actor = actors.spawn_actor_from_object(what, loc, rot)
        else:
            actor = actors.spawn_actor_from_class(what, loc, rot)
        if scale is not None:
            actor.set_actor_scale3d(unreal.Vector(*scale))
        actor.set_editor_property('tags', [unreal.Name(self.tag)] + [unreal.Name(t) for t in tags])
        actor.set_folder_path(f'{self.folder}/{folder}' if folder else self.folder)
        if label:
            actor.set_actor_label(label)
        return actor

    def environment(self):
        """The light, sky and fog (build_area_environment.py): the tutorial island's afternoon, or what the layout's
        level.environment sets. Returns the sky light, recaptured once the level stands."""
        return importlib.reload(build_area_environment).place(self)

    def gameplay(self, meshes):
        """The spawn (with the area's director, the tutorial's on the tutorial island), the gun rack, the target
        dummies, the skiff jetty and the landings (build_area_travel.py), and the creature groups (layout.json
        gameplay.creatures, in order).

        Everything here sits in the <area>/Gameplay folder, which a "gameplay" build clears first: whatever lives
        there must be placed here (the rack once lived in models() and a gameplay build left the island without
        it)."""
        dummy = unreal.EditorAssetLibrary.load_blueprint_class(DUMMY)
        for key, spot in self.layout['placements'].items():
            x, y, z = spot['location']
            if spot['kind'] == 'GunRack':
                if 'GunRack' not in meshes:
                    self.warn(f'no SM_GunRack yet (placement {key})')
                    continue
                # AWeaponRack lays its rifle on itself, with ammo beside it.
                rack = self.place(unreal.load_class(None, CLASSES + 'WeaponRack'), spot['location'], spot['yaw'],
                                  label=key, folder='Gameplay', tags=('Obstacle',))
                rack.get_editor_property('rack').set_static_mesh(unreal.load_asset(meshes['GunRack']))
                rack.set_editor_property('weapon', unreal.load_asset(self.settings.get('rackWeapon', RACK_WEAPON)))
            elif spot['kind'] == 'PlayerStart':
                self.place(unreal.PlayerStart, (x, y, z + 100.0), spot['yaw'], label='PlayerStart', folder='Gameplay')
                self.travel.place_spawn_landing(self, (x, y, z + 100.0), spot['yaw'])
                director = self.settings.get('director')
                if director:
                    # The tutorial's prompts (ATutorialDirector: its steps are in C++).
                    self.place(unreal.load_class(None, CLASSES + director), (x, y, z + 300.0), label=director,
                               folder='Gameplay')
            elif spot['kind'] == 'TargetDummy':
                self.place(dummy, (x, y, z), spot['yaw'], label=f'TargetDummy_{key[-1]}', folder='Gameplay')
        self.travel.place_jetty(self)
        self.travel.place_landings(self)
        # The story's actors stand on the models placed already (the graves, the lookout, the farmhouse), and so do the
        # posters (on the buildings' walls and the notice board).
        self.story.place(self)
        self.posters.place(self)

        # The groups draw from one random stream, in order, so each lands where it did last time.
        rng = random.Random(self.settings.get('seed', 7))
        for group in self.settings.get('creatures', []):
            creature = unreal.load_class(None, CLASSES + group['class'])
            for i, (x, y) in enumerate(self.creature_spots(group, rng)):
                self.place(creature, (x, y, ground_height(x, y, 0.0) + 60.0), rng.uniform(-180.0, 180.0),
                           label=f"{group['label']}_{i + 1:02d}", folder='Gameplay')

    def creature_spots(self, group, rng):
        """Where a group's creatures start: anywhere in a zone (the zone's id), or around a center within spread
        (cm), at least spacing apart. A loose group wanders together: each strolls around its own spot."""
        spots = []
        if 'zone' in group:
            zone = next(z for z in self.source['zones'] if z['id'] == group['zone'])['polygon']
            xs, ys = [p[0] for p in zone], [p[1] for p in zone]
            while len(spots) < group['count']:
                point = (rng.uniform(min(xs), max(xs)), rng.uniform(min(ys), max(ys)))
                if inside(point, zone) and all(math.dist(point, s) > group['spacing'] for s in spots):
                    spots.append(point)
        else:
            (cx, cy), spread = group['center'], group['spread']
            while len(spots) < group['count']:
                point = (cx + rng.uniform(-spread, spread), cy + rng.uniform(-spread, spread))
                if all(math.dist(point, s) > group['spacing'] for s in spots):
                    spots.append(point)
        return spots

    def models(self, meshes):
        """Buildings, structures and props at their placements, and the orchards' apple trees."""
        placed = 0
        stations = []
        for key, spot in self.layout['placements'].items():
            kind = spot['kind']
            # Gameplay actors are gameplay()'s, so placing only those again ("gameplay") brings them all back.
            if kind in ('PlayerStart', 'TargetDummy', 'GunRack', 'Landing'):
                continue
            name = kind
            if name not in meshes:
                self.warn(f'no SM_{name} yet (placement {key})')
                continue
            if kind == 'Depot':
                station = self.travel.place_station(self, key, spot, unreal.load_asset(meshes[name]))
                if station:
                    stations.append((station, key, spot))
                    placed += 1
                    continue
            if kind == 'Windmill':
                # The fan turns: AWindmill hangs it from the tower's Fan socket.
                windmill = self.place(unreal.load_class(None, CLASSES + 'Windmill'), spot['location'], spot['yaw'],
                                      label=key, folder='Buildings', tags=('Obstacle',))
                tower = windmill.get_editor_property('tower')
                tower.set_static_mesh(unreal.load_asset(meshes[name]))
                if 'WindmillFan' in meshes:
                    fan = windmill.get_editor_property('fan')
                    fan.set_static_mesh(unreal.load_asset(meshes['WindmillFan']))
                    # The socket exists only now that the tower has its mesh.
                    snap = unreal.AttachmentRule.SNAP_TO_TARGET
                    fan.attach_to_component(tower, 'Fan', snap, snap, unreal.AttachmentRule.KEEP_RELATIVE, False)
                    # Attaching doesn't move it in the editor until its transform changes (setting the same one is
                    # skipped).
                    fan.set_relative_location(unreal.Vector(0.0, 0.0, 1.0), False, True)
                    fan.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, True)
            else:
                self.place(unreal.load_asset(meshes[name]), spot['location'], spot['yaw'], label=key,
                           folder='Buildings', tags=('Obstacle',))
            placed += 1

        # A station's landing lies on its platform, which stands only now.
        for station, key, spot in stations:
            self.travel.land_station(self, station, key, spot)

        # The bridge's ramps end at its pivot's height, which the layout gives as the road on both banks; it
        # stretches to the span. Untagged, so the minimap draws it as ground but the scatter doesn't grow grass on it.
        bridge = self.layout.get('bridge')
        if bridge and 'Bridge' in meshes:
            deck = unreal.load_asset(meshes['Bridge'])
            box = deck.get_bounding_box()
            stretch = bridge['span'] / max(box.max.x - box.min.x, 1.0)
            self.place(deck, bridge['location'], bridge['yaw'], label='Bridge', folder='Buildings',
                       scale=(stretch, 1.0, 1.0))
            placed += 1

        apple = unreal.load_asset(meshes['Apple_A']) if 'Apple_A' in meshes else None
        rng = random.Random(11)
        for r, row in enumerate(self.layout.get('orchardRows', [])):
            for t, tree in enumerate(row['trees']):
                if apple:
                    self.place(apple, tree, rng.uniform(-180.0, 180.0), label=f'AppleTree_{r + 1}_{t + 1}',
                               folder='Orchard', tags=('Obstacle', 'Tree'))
                    placed += 1
        self.log(f'placed {placed} models')

    def sky_islands(self, meshes):
        """Islands hanging in the sky past an island's rim (layout.json level.skyIslands: Skyreach's, seen beyond its
        jetty). Each stands by bearing (0 north, 90 east) and distance from a point on the rim, its top (the mesh's
        pivot) a rise above the ground there, turned by yaw or, with faceFrom, with its +X back toward that point (an
        island's waterfall toward the jetty). No collision and no shadows; tagged Beyond, so Looter.Perf.HideTag Beyond
        measures them with everything else past the boundary."""
        spec = self.source.get('level', {}).get('skyIslands')
        if not spec:
            return
        fx, fy = spec['from']
        ground = ground_height(fx, fy, 0.0)
        placed = 0
        for island in spec['islands']:
            name = island['mesh']
            if name not in meshes:
                self.warn(f'no SM_{name} yet (a sky island)')
                continue
            bearing = math.radians(island['bearing'])
            location = (fx + math.cos(bearing) * island['distance'], fy + math.sin(bearing) * island['distance'],
                        ground + island.get('rise', 0.0))
            yaw = (island['bearing'] + 180.0) % 360.0 if island.get('faceFrom') else island.get('yaw', 0.0)
            actor = self.place(unreal.load_asset(meshes[name]), location, yaw, label=f'SkyIsland_{placed + 1}',
                               folder='Beyond', tags=('Beyond',))
            actor.static_mesh_component.set_editor_property('cast_shadow', False)
            placed += 1
        self.log(f'placed {placed} sky islands')

    def terrain(self, meshes, only_beyond=False):
        """The terrain tiles (walkable, tagged Ground for the minimap and the scatter), the rock underside and the
        water; and a grounded area's ring, canyon wall and backdrop (tagged Beyond, in their own folder; only the
        ring casts shadows). They are all modeled in the area's space, so they sit at the origin."""
        count = 0
        for name, path in sorted(meshes.items()):
            if not name.startswith(self.prefix):
                continue
            part = name[len(self.prefix):]
            if only_beyond and not part.startswith(BEYOND_PARTS):
                continue
            if part.startswith(BEYOND_PARTS):
                actor = self.place(unreal.load_asset(path), (0, 0, 0), label=part, folder='Beyond', tags=('Beyond',))
                if not part.startswith('Ring_'):
                    actor.static_mesh_component.set_editor_property('cast_shadow', False)
            else:
                self.place(unreal.load_asset(path), (0, 0, 0), label=part, folder='Terrain',
                           tags=('Ground',) if part.startswith('Tile_') else ())
            count += 1
        if only_beyond:
            if count:
                self.log(f'placed {count} pieces past the boundary')
            return
        if not count:
            self.warn(f'no SM_{self.prefix}* terrain yet')
        self.log(f'placed {count} terrain pieces')

    def cliffs(self, meshes):
        """Cliff faces over the terrain's steep walls, one group per feature in layout_computed.json (a plateau's
        edge, its ramp's cut walls) and the island's rim.

        Each dressing point is where a wall meets the ground below it (a hanging cliff such as the rim: the wall's
        top, with the drop below it), facing out. A piece stands a little inside that line so it covers the wall and
        its lip; it reaches just over the top, is widened to overlap its neighbours, and is chosen among the kit's
        pieces by how little it must stretch."""
        pieces = []
        for name in CLIFF_PIECES:
            if name in meshes:
                mesh = unreal.load_asset(meshes[name])
                box = mesh.get_bounding_box()
                pieces.append((mesh, box.max.z, box.max.y - box.min.y))
        if not pieces:
            self.warn('no cliff pieces yet')
            return
        rng = random.Random(23)
        placed = 0
        for group, points in self.layout.get('cliffs', {}).items():
            for i, point in enumerate(points):
                kind = point.get('kind')
                if kind == 'bank':
                    continue  # a sloped bank: no face
                if kind == 'outcrop':
                    placed += self.outcrop(meshes, group, point)
                    continue
                x, y, z = point['location']
                if 'drop' in point:
                    bottom, height = z - point['drop'], point['drop']
                else:
                    bottom, height = z, point.get('height', point.get('top', z) - z)
                # A wall taller than the kit's tallest piece comes in stacked courses, bottom first, each standing on
                # its own foot; the top one reaches just over the wall, the others a little up behind the next.
                courses = point.get('courses') or [{'location': (x, y, bottom), 'height': height}]
                # Neighbours along the wall set the width (the points run along it in order).
                gaps = [math.dist(point['location'][:2], points[j]['location'][:2]) for j in (i - 1, i + 1)
                        if 0 <= j < len(points)]
                gap = min(gaps) if gaps else 1000.0
                yaw = point['yaw']
                for k, course in enumerate(courses):
                    cx, cy, cz = course['location']
                    reach = course['height'] + (CLIFF_OVERTOP if k == len(courses) - 1 else CLIFF_COURSE_OVERLAP)
                    choices = sorted(pieces, key=lambda p: abs(math.log(reach / p[1])) + rng.uniform(0.0, 0.25))
                    mesh, piece_height, piece_width = choices[0]
                    inward = (-math.cos(math.radians(yaw)) * CLIFF_INSET, -math.sin(math.radians(yaw)) * CLIFF_INSET)
                    width = min(max((gap + CLIFF_OVERLAP) / piece_width, 0.75), 1.6)
                    suffix = f'_{k + 1}' if len(courses) > 1 else ''
                    self.place(mesh, (cx + inward[0], cy + inward[1], cz), yaw + rng.uniform(-4.0, 4.0),
                               label=f'Cliff_{group}_{i + 1:02d}{suffix}', folder=f'Cliffs/{group}',
                               scale=(1.0, width, reach / piece_height), tags=('Obstacle',))
                    placed += 1
        self.log(f'placed {placed} cliff pieces')

    def outcrop(self, meshes, group, point):
        """A knob's freestanding rock: the outcrop kit's piece (Art/Models/Rocks/Outcrops.py), scaled to the height
        the layout asks for, on the knob's level seat. Waits for the kit's import."""
        name = 'Outcrop_' + point.get('piece', 'TorA')
        if name not in meshes:
            self.warn(f'no SM_{name} yet (knob {group})')
            return 0
        mesh = unreal.load_asset(meshes[name])
        box = mesh.get_bounding_box()
        scale = point['height'] / max(box.max.z, 1.0)
        self.place(mesh, point['location'], point.get('yaw', 0.0), label=f'Outcrop_{group}', folder='Outcrops',
                   scale=(scale, scale, scale), tags=('Obstacle',))
        return 1

    def no_tree_zones(self):
        """Invisible boxes tagged NoTrees over the zones that must stay open (layout.json level.noTreeZones: a zone's
        id and the share of its bounding box), which the scatter's tree layers avoid: on the tutorial island the
        target range, so trees never block a shot at the dummies, and the village square."""
        for zone_id, shrink in self.tree_free_zones:
            polygon = next(z for z in self.source['zones'] if z['id'] == zone_id)['polygon']
            xs, ys = [p[0] for p in polygon], [p[1] for p in polygon]
            size = ((max(xs) - min(xs)) * shrink, (max(ys) - min(ys)) * shrink)
            box = self.place(unreal.TriggerBox, ((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, 0.0),
                             label=f'NoTrees_{zone_id}', folder='Scatter', tags=('NoTrees',),
                             # The box is 64 cm across; it reaches 100 m up and down so every ray's hit is inside it.
                             scale=(size[0] / 64.0, size[1] / 64.0, 20000.0 / 64.0))
            box.set_actor_enable_collision(False)

    def effects(self, meshes):
        """The waterfall off the creek's lip, and smoke from every chimney (the buildings' Smoke sockets), leaning
        downwind. Each waits for its model."""
        falls = self.layout.get('waterfalls') or ({'waterfall': self.layout['waterfall']}
                                                  if self.layout.get('waterfall') else {})
        for n, (key, fall) in enumerate(falls.items()):
            if 'Waterfall' not in meshes:
                break
            # The model falls about 48 m; a shorter falls (into a gorge) gets it squashed to its drop.
            drop = fall['location'][2] - fall.get('dropTo', fall['location'][2] - WATERFALL_LENGTH)
            squash = (1.0, 1.0, max(min(drop / WATERFALL_LENGTH, 1.0), 0.15))
            label = 'Waterfall' if n == 0 else f'Waterfall_{key}'
            self.place(unreal.load_asset(meshes['Waterfall']), fall['location'], fall['yaw'], label=label,
                       folder='Effects', scale=squash if squash[2] < 1.0 else None)
            if 'WaterfallMist' in meshes:
                self.place(unreal.load_asset(meshes['WaterfallMist']), fall['location'], fall['yaw'],
                           label=label.replace('Waterfall', 'WaterfallMist'), folder='Effects')
        if 'SmokePlume' not in meshes:
            self.warn('no SM_SmokePlume yet')
            return
        plume = unreal.load_asset(meshes['SmokePlume'])
        count = 0
        for building in actors.get_all_level_actors():
            if unreal.Name(self.tag) not in building.tags:
                continue
            # A building standing as an actor of its own (the depot's station) carries its mesh in a component.
            for mesh_component in building.get_components_by_class(unreal.StaticMeshComponent):
                mesh = mesh_component.static_mesh
                if mesh is None or mesh.find_socket('Smoke') is None:
                    continue
                where = mesh_component.get_socket_transform('Smoke', unreal.RelativeTransformSpace.RTS_WORLD).translation
                self.place(plume, (where.x, where.y, where.z), WIND_YAW, label=f'Smoke_{building.get_actor_label()}',
                           folder='Effects')
                count += 1
        self.log(f'placed {count} chimney smoke plumes')

    def run(self, mode=None):
        """Builds the whole level, or with mode "gameplay" only the gameplay actors, or with "beyond" only what lies
        past the boundary (a grounded area's ring, canyon wall and backdrop; an island's sky islands)."""
        with open(self.computed_path) as f:
            self.layout = json.load(f)
        # Reloaded, as the editor keeps modules between runs, so an edited one takes effect.
        self.travel = importlib.reload(build_area_travel)
        self.story = importlib.reload(build_area_story)
        self.posters = importlib.reload(build_area_posters)
        if mode == 'gameplay':
            self.open_level('Gameplay')
            self.gameplay(mesh_index())
            levels.save_current_level()
            self.log('gameplay actors placed and saved')
            return
        if mode == 'beyond':
            self.open_level('Beyond')
            meshes = mesh_index()
            self.terrain(meshes, only_beyond=True)
            self.sky_islands(meshes)
            levels.save_current_level()
            self.log('beyond placed and saved')
            return
        self.open_level()
        meshes = mesh_index()
        sky_light = self.environment()
        self.terrain(meshes)
        self.sky_islands(meshes)
        self.cliffs(meshes)
        self.models(meshes)
        self.effects(meshes)
        self.no_tree_zones()
        self.gameplay(meshes)
        # Reloaded, as the editor keeps modules between runs, so an edited one takes effect.
        importlib.reload(build_area_bounds).place(self)
        sky_light.recapture_sky()
        levels.save_current_level()
        self.log('built and saved')


def run(name, only_gameplay=False, mode=None):
    AreaBuild(name).run('gameplay' if only_gameplay else mode)


if __name__ == '__main__':
    args = sys.argv[1:]
    if not args or args[0] in ('gameplay', 'beyond'):
        raise SystemExit('usage: build_area.py <Area> [gameplay|beyond] (the area is a folder under Art/Levels)')
    run(args[0], mode=next((a for a in args[1:] if a in ('gameplay', 'beyond')), None))
