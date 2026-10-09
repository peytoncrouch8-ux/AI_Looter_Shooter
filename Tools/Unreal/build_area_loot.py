"""The lootable world (Docs/Polish/BorderlandsComparison.md, item 10: "every corner should offer something") for
build_area.py: breakable crates and barrels (ABreakableProp, Source/.../World/BreakableProp.h), graves to dig and coffins
to pry (AChest kinds Grave and Coffin), mailboxes, footlockers and a couple of strongboxes (AChest, Loot/Chest.h).

    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_loot.py RansomsRest"

places only this again (the area's Loot folder; the level is saved). build_area.py calls place(build) after the dressing
and the gameplay actors stand (it reads both), before the fauna (whose perches are the dressing's instances):
    importlib.reload(importlib.import_module('build_area_loot')).place(self)
in its full build (after self.gameplay(meshes), before build_area_bounds) and in its 'dressing' mode (after
self.dressing(...)). Everything it places is tagged with the area's build tag and Loot and sits in <area>/Loot, so building
again replaces only what this placed.

What it reads, all from the level as built (nothing hand-placed but the anchors below):
- Breakables from the dressing: the dressing's own wooden crates and barrels (AInstancedProps Dressing_Crate_A,
  Dressing_Crate_B, Dressing_Barrel_A) become breakables where they stood (the instance is taken out of the dressing
  and an ABreakableProp stands in its transform, tagged FromDressing and Mesh_<mesh>, so a later run gives it back
  first: idempotent, and a dressing rebuild's fresh instances aren't doubled). Never one with something stacked on it
  (the store's crate stack keeps its bottom crate), never one near the story's actors or on a road. Steel drums stay.
- Breakables and containers at anchors (AREAS: a layout placement, a layout obstacle's middle, a placed actor by class or
  label): each looks for its own spot round its anchor in rings, level and clear (the terrain under it within a hand,
  nothing solid in it or a hand round it, none of the dressing's pieces, off the roads, the encounters' and the story's
  spots and the boundary's edge, a metre from loot already placed), preferring a spot with a wall at its back; it faces
  away from that wall (or its anchor). An anchor with no such spot is warned of and left out.
- Graves (Ransom's Rest): boot hill's old graves (the dressing's Grave_MoundSunken inside the bootHill zone: the mound
  is taken out and the grave's own mound stands in its place, with its spade beside it, the sign it can be dug) and a
  couple of the churchyard's old boards (a grave's mound laid in front of a board in the chapel's zone, where there's
  room). Never near a respawn grave, the family plot (Ellis's and Abel's, the ancestors': the story's), the churchyard
  fight's rising spots or any encounter's spots.
- Coffins: the top coffin of each of the undertaker's yard stacks (the dressing's Coffin_Closed with another under it
  and none on it), and two lying behind the coffin shed.
- Mailboxes: one by the road in front of each house AREAS names, facing the road.
Every choice comes from sorted lists and seeded streams, so a rebuild places the same; ids come from where each stands,
so the session keeps the same one broken or opened.
"""
import importlib
import json
import math
import os
import random
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area  # noqa: E402
import build_area_caches as caches  # noqa: E402
import build_area_story as story  # noqa: E402

FOLDER = 'Loot'
TAG = 'Loot'
FROM_DRESSING = 'FromDressing'
MESH_TAG = 'Mesh_'
DRESSING_FOLDER = 'Dressing'

# The dressing's meshes that break, and the kind each is (ABreakableProp's EBreakableKind, as Python names it).
BREAKABLE_MESHES = {'Crate_A': 'SLATTED_CRATE', 'Crate_B': 'PACKING_CRATE', 'Barrel_A': 'BARREL'}
# A breakable's footprint's half sizes (cm, along its X and Y): Containers.py's.
BREAKABLE_HALF = {'Crate_A': (49.0, 37.0), 'Crate_B': (52.0, 33.0), 'Barrel_A': (33.0, 33.0)}
# What can stand on a crate in the dressing (a stack): a crate with one of these resting on it stays dressing.
STACKABLE = ('Crate_', 'Barrel_', 'Coffin_', 'HayBale_', 'FirewoodStack', 'LumberStack')
STACK_REACH = 45.0      # cm across: another piece's middle this near over it rests on it
STACK_ABOVE = 20.0      # cm up

# AChest's kinds (as Python names them) and each container's footprint's half sizes (cm, along X and Y).
CHEST_KINDS = {'Coffin': 'COFFIN', 'Grave': 'GRAVE', 'Mailbox': 'MAILBOX', 'Footlocker': 'FOOTLOCKER', 'Strongbox': 'STRONGBOX'}
CHEST_HALF = {'Coffin': (34.0, 99.0), 'Grave': (50.0, 100.0), 'Mailbox': (25.0, 14.0), 'Footlocker': (25.0, 42.0),
              'Strongbox': (31.0, 42.0)}
# A strongbox standing loose about the area gives less than the gang's in the sheriff's office (two guns at Luck 1.0):
# one gun at Luck 0.5, two ammo pickups (FChestLootOverride).
LOOSE_STRONGBOX = dict(guns=1, gun_chance=1.0, luck=0.5, ammo_pickups=2, mote_chance=0.0)

# How a spot is judged (cm).
LEVEL = 12.0            # the terrain under its footprint within this
LEVEL_POST = 25.0       # a mailbox's post goes into the ground: it may stand on a steeper slope
MARGIN = 25.0           # a hand round it, clear of anything solid
SOLID_OVER = 6.0
PROBE_FROM = 250.0
DROP = 40.0
ROAD_CLEAR = 60.0       # off a road's edge
SPOT_CLEAR = 250.0      # off an encounter's spots (where creatures rise or stand)
STORY_CLEAR = 300.0     # off the story's actors (speakers, markers, spawners, the posters, Delia's door...)
CHEST_CLEAR = 150.0     # off another chest (a camp's cache: its breakables stand round it)
RESPAWN_CLEAR = 400.0   # off a respawn grave's marker
LOOT_CLEAR = 120.0      # off loot placed already
EDGE_CLEAR = 300.0      # off the playable boundary's edge
TUCK_REACH = 140.0      # a wall this near its back tucks it
# A camp's own breakables stand among its spots and by its cache (a camp is a cluttered place).
RELAXED = dict(chest_reach=120.0, spot_reach=150.0, story_reach=200.0)
# The dressing's crates and barrels already stand in yards round the story's buildings: kept off the story a little less.
DRESSING_STORY_CLEAR = 200.0
# Story actors that aren't spots to keep off (zones and areas, whose middle means nothing).
NOT_STORY = ('SafeGround', 'PlayableArea', 'StoryLighting', 'LightingState', 'AmbientEmitter')

# The anchors: what stands round each (an anchor is a layout placement's key; 'obstacle:<id>' an obstacle's middle;
# '@<Class>' a placed actor of that class; 'label:<prefix>' placed actors whose label starts so, each its own anchor),
# the radii (cm) its spot is looked for at, and its id's word. Kinds: the dressing's meshes (breakables) or AChest's.
AREAS = {
    'RansomsRest': dict(
        anchors=[
            # The depot: travellers' luggage and freight left on the ground by the platform.
            ('depot', 'Barrel_A', (500, 700, 900)), ('depot', 'Crate_A', (500, 700, 900)),
            ('depot', 'Footlocker', (500, 700, 900)),
            # Ransom Farm: barrels by the barn, a crate by the farmhouse's side.
            ('barn', 'Barrel_A', (700, 900, 1100)), ('barn', 'Crate_B', (700, 900, 1100)),
            ('farmhouse', 'Crate_A', (700, 900, 1100)),
            # Whitlock Fields: by Amos's barn (his hay bales are the story's: kept clear by STORY_CLEAR).
            ('whitlockBarn', 'Barrel_A', (700, 900, 1100)), ('whitlockBarn', 'Crate_A', (700, 900, 1100)),
            # The ruins: what was left, and a miner's and a settler's footlockers.
            ('burntHomestead', 'Footlocker', (250, 400, 550)), ('burntHomestead', 'Crate_A', (250, 400, 550)),
            ('springhouse', 'Barrel_A', (300, 450, 600)),
            ('burntLivery', 'Crate_B', (300, 450, 600)), ('burntLivery', 'Barrel_A', (300, 450, 600)),
            ('quarryDerrick', 'Footlocker', (350, 500, 700)), ('quarryDerrick', 'Crate_A', (350, 500, 700)),
            ('quarryDerrick', 'Barrel_A', (350, 500, 700)),
            # A strongbox thrown from the burnt wagons, and one behind the saloon by its woodshed.
            ('wagonA', 'Strongbox', (250, 350, 500)),
            ('obstacle:saloonBacklot', 'Strongbox', (150, 250, 350)),
            # The coffin shed: two coffins lying behind it.
            ('coffinShed', 'Coffin', (250, 350, 450)), ('coffinShed', 'Coffin', (250, 350, 450)),
            # The encounters' camps: a barrel and a crate round each camp's Supply Crate.
            ('label:CampCache_', 'Barrel_A', (180, 260, 340)), ('label:CampCache_', 'Crate_A', (180, 260, 340)),
        ],
        mailboxes=['farmhouse', 'cottageNorth', 'cabinSouth', 'cottageSouth'],
        dressing_breakables=40,
        graves=dict(bootHill=4, churchyard=2),
        stack_coffins=True,
        # The family plot (the story's graves) is never dug, nor anywhere near it (cm).
        sacred=[('graveEllis', 2000.0), ('graveAbel', 2000.0), ('spawn', 1200.0), ('graveKeeper', 800.0)],
    ),
    'TutorialIsland': dict(
        anchors=[
            ('windmill', 'Footlocker', (350, 500, 700)),
            ('barn', 'Barrel_A', (700, 900, 1100)),
        ],
        mailboxes=['farmhouse', 'farmhouse_farmroad'],
        # Practice: about half of the town's crates and barrels.
        dressing_breakables=16,
        graves={},
        stack_coffins=False,
        sacred=[],
    ),
}

GRAVE_SPREAD = 600.0      # cm between two dug graves
GRAVE_BOARD_OUT = 110.0   # a churchyard grave's mound lies this far in front of its board (as the dressing's boot hill)
GRAVE_ROOM = 120.0        # no other board this near a churchyard grave's mound
SHOVEL_AT = (108.0, 46.0) # the grave's spade (FChestKindInfo ShovelBefore), in its frame
GRAVE_PROBE_HALF = (75.0, 40.0)   # a churchyard grave's footprint as probed (its X along the grave: short of its board)
BREAKABLE_SPREAD = 350.0  # cm between two breakables taken from the dressing on a practice island


def chest_class():
    return story.actor_class('Chest')


def breakable_class():
    return story.actor_class('BreakableProp')


def editor_actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def built(build, actor):
    return unreal.Name(build.tag) in actor.tags


def in_folder(build, actor, folder):
    path = str(actor.get_folder_path())
    return path == f'{build.folder}/{folder}' or path.startswith(f'{build.folder}/{folder}/')


def yaw_of(transform):
    return transform.rotation.rotator().yaw


# ---------------------------------------------------------------------------
# The dressing's instances
# ---------------------------------------------------------------------------

class Dressing:
    """The dressing's solid AInstancedProps by mesh name, and their instances' world transforms."""

    def __init__(self, build):
        self.actors = {}
        for actor in editor_actors():
            if not built(build, actor) or actor.get_class().get_name() != 'InstancedProps':
                continue
            label = actor.get_actor_label()
            if not label.startswith('Dressing_') or label.endswith('_Passable'):
                continue
            self.actors[label[len('Dressing_'):]] = actor

    def component(self, mesh):
        actor = self.actors.get(mesh)
        return actor.get_editor_property('instances') if actor is not None else None

    def instances(self, mesh):
        """[(index, transform)] of a mesh's instances."""
        component = self.component(mesh)
        if component is None:
            return []
        return [(i, component.get_instance_transform(i, True)) for i in range(component.get_instance_count())]

    def stackable(self):
        """Every stackable piece's (mesh, x, y, z), for telling what rests on what."""
        out = []
        for mesh in self.actors:
            if mesh.startswith(STACKABLE):
                out += [(mesh, t.translation.x, t.translation.y, t.translation.z) for _, t in self.instances(mesh)]
        return out

    def give_back(self, mesh, transform):
        """An instance back where a breakable or a grave took it, unless one stands there already (a fresh dressing)."""
        component = self.component(mesh)
        if component is None:
            return False
        at = transform.translation
        for _, t in self.instances(mesh):
            if (t.translation - at).length() < 2.0:
                return False
        component.add_instance(transform, True)
        return True

    def take(self, mesh, indices):
        """Takes these instances out of the dressing (highest first, so the others keep their indices)."""
        component = self.component(mesh)
        if component is None:
            return
        for index in sorted(set(indices), reverse=True):
            component.remove_instance(index)


def rests_on_it(stack, mesh, x, y, z):
    """Something of the dressing's stands on this piece (a crate stacked on it, a coffin on a coffin)."""
    return any(math.hypot(ox - x, oy - y) < STACK_REACH and oz > z + STACK_ABOVE for _, ox, oy, oz in stack)


def has_under(stack, x, y, z):
    return any(math.hypot(ox - x, oy - y) < STACK_REACH and oz < z - STACK_ABOVE for _, ox, oy, oz in stack)


# ---------------------------------------------------------------------------
# Where things may stand
# ---------------------------------------------------------------------------

class Site:
    """What a spot is judged against: the terrain's tiles, the dressing's pieces, the roads, the encounters' and the
    story's spots, the respawn graves, the playable boundary, and what this has placed so far."""

    def __init__(self, build, spec):
        self.build = build
        self.tiles = build_area.terrain_tiles(build.tag)
        if not self.tiles:
            build.warn('loot: no terrain tiles tagged Ground (build the whole level first): nothing can be seated')
        dressing = importlib.reload(importlib.import_module('build_area_dressing'))
        self.pieces = dressing.footprints(build.source, build.layout['placements'])
        self.roads = [([p[:2] for p in road['points']], road['width']) for road in build.layout.get('roads', {}).values()]
        self.boundary = [c[:2] for c in build.layout.get('boundary', {}).get('corners', [])]
        self.encounters = caches.encounter_spots() + self.spawner_spots()
        self.story, self.chests, self.respawns = self.gameplay_actors()
        self.sacred = []
        for key, reach in spec.get('sacred', []):
            spot = build.layout['placements'].get(key)
            if spot:
                self.sacred.append((spot['location'][0], spot['location'][1], reach))
        self.placed = []

    def spawner_spots(self):
        """The ambushes' and patrols' spots (encounter_spots reads the camps' spawners): where creatures rise or walk."""
        spots = []
        for actor in editor_actors():
            name = actor.get_class().get_name()
            if name not in ('AmbushSpawner', 'PatrolSpawner'):
                continue
            at = actor.get_actor_location()
            spots.append((at.x, at.y))
            transform = actor.get_actor_transform()
            for prop in ('spawn_points', 'route'):
                try:
                    points = actor.get_editor_property(prop)
                except Exception:
                    continue
                for point in points or []:
                    world = transform.transform_location(point)
                    spots.append((world.x, world.y))
        return spots

    def gameplay_actors(self):
        """The story's actors (everything the gameplay build placed that isn't a plain model), the chests among them,
        and the respawn graves' markers (x, y, id)."""
        story_spots, chests, respawns = [], [], []
        for actor in editor_actors():
            if not built(self.build, actor) or not in_folder(self.build, actor, 'Gameplay'):
                continue
            name = actor.get_class().get_name()
            if name in ('StaticMeshActor', 'InstancedProps', 'InstancedScenery') or name in NOT_STORY:
                continue
            at = actor.get_actor_location()
            if name == 'Chest':
                chests.append((at.x, at.y))
            elif name == 'RespawnMarker':
                respawns.append((at.x, at.y, str(actor.get_editor_property('marker_id'))))
            else:
                story_spots.append((at.x, at.y))
        return story_spots, chests, respawns

    def terrain(self, x, y):
        hit = build_area.terrain_hit(self.tiles, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -20000.0))
        return None if hit is None else hit.z

    def near(self, x, y, points, reach):
        return min((math.hypot(px - x, py - y) for px, py in points), default=float('inf')) < reach

    def road_clear(self, x, y, half):
        near_road = min((caches.line_distance((x, y), points) - width * 0.5 for points, width in self.roads), default=float('inf'))
        return near_road >= ROAD_CLEAR + half

    def keep_off(self, x, y, chest_reach=CHEST_CLEAR, spot_reach=SPOT_CLEAR, story_reach=STORY_CLEAR):
        """The reasons (x, y) is too near something the lootable world keeps off, in words (a camp's breakables stand
        nearer its spots and its cache: RELAXED)."""
        problems = []
        if self.near(x, y, self.encounters, spot_reach):
            problems.append('an encounter\'s spot')
        if self.near(x, y, self.story, story_reach):
            problems.append('the story\'s actors')
        if self.near(x, y, self.chests, chest_reach):
            problems.append('a chest')
        if self.near(x, y, [(r[0], r[1]) for r in self.respawns], RESPAWN_CLEAR):
            problems.append('a respawn grave')
        if any(math.hypot(sx - x, sy - y) < reach for sx, sy, reach in self.sacred):
            problems.append('the family plot')
        if self.near(x, y, self.placed, LOOT_CLEAR):
            problems.append('loot placed already')
        if self.boundary:
            if not build_area.inside((x, y), self.boundary):
                problems.append('outside the playable boundary')
            elif caches.line_distance((x, y), self.boundary, closed=True) < EDGE_CLEAR:
                problems.append('the boundary\'s edge')
        return problems

    def judge(self, x, y, yaw, half, level=LEVEL, reach=None):
        """(the terrain's height at its pivot, its problems in words) for something of half sizes half (along its X, its
        Y) at (x, y) facing yaw; reach: keep_off's distances where they differ (RELAXED). The height is None off the
        terrain."""
        hx, hy = half
        under = [self.terrain(*caches.frame(x, y, yaw, fx * hx, fy * hy)) for fx in (-1, 0, 1) for fy in (-1, 0, 1)]
        if any(z is None for z in under):
            return None, ['not on the terrain']
        z = self.terrain(x, y)
        problems = []
        if max(under) - min(under) > level:
            problems.append(f'not level ({max(under) - min(under):.0f} cm across it)')
        solid = drops = 0
        probes = [(fx * (hx + MARGIN), fy * (hy + MARGIN)) for fx in (-1.0, 0.0, 1.0) for fy in (-1.0, 0.0, 1.0)]
        for ax, ay in probes:
            px, py = caches.frame(x, y, yaw, ax, ay)
            ground = self.terrain(px, py)
            if ground is None or ground < z - DROP:
                drops += 1
                continue
            hit = caches.blocking(unreal.Vector(px, py, ground + PROBE_FROM), unreal.Vector(px, py, ground - 50.0))
            if hit is not None and hit.z > ground + SOLID_OVER:
                solid += 1
        if drops:
            problems.append('a drop beside it')
        if solid:
            problems.append(f'something solid in it or round it ({solid} probes)')
        mine = caches.corners(x, y, yaw, hx + MARGIN, hy + MARGIN)
        if any(math.hypot(p[0] - x, p[1] - y) < 2000.0 and caches.overlap(mine, caches.corners(p[0], p[1], p[2], p[4], p[3]))
               for p in self.pieces):
            problems.append('on the dressing\'s pieces')
        if not self.road_clear(x, y, max(hx, hy)):
            problems.append('on or by a road')
        problems += self.keep_off(x, y, **(reach or {}))
        return z, problems

    def wall_behind(self, x, y, z, yaw):
        """How near a wall stands behind (x, y) facing yaw (cm), or None within TUCK_REACH."""
        back = caches.frame(x, y, yaw, -TUCK_REACH, 0.0)
        hit = caches.blocking(unreal.Vector(x, y, z + 60.0), unreal.Vector(back[0], back[1], z + 60.0))
        return None if hit is None else math.hypot(hit.x - x, hit.y - y)


def find_spot(site, anchor, radii, half, seed, level=LEVEL, reach=None, facing=None):
    """A spot round anchor (x, y, yaw): rings at radii, every 30 degrees from a seeded start; of the clear ones on the
    nearest ring that has any, the one with a wall nearest its back. (x, y, z, yaw), facing out from the anchor (its back
    to it), or as facing(angle out) says; or None."""
    ax, ay, _ = anchor
    rnd = random.Random(seed)
    start = rnd.uniform(0.0, 360.0)
    for radius in radii:
        found = []
        for step in range(12):
            angle = start + step * 30.0
            x = ax + math.cos(math.radians(angle)) * radius
            y = ay + math.sin(math.radians(angle)) * radius
            yaw = facing(angle) if facing else angle
            z, problems = site.judge(x, y, yaw, half, level, reach)
            if z is None or problems:
                continue
            wall = site.wall_behind(x, y, z, yaw)
            found.append((wall if wall is not None else TUCK_REACH * 2.0, step, x, y, z, yaw))
        if found:
            _, _, x, y, z, yaw = min(found)
            return x, y, z, yaw
    return None


def anchors_of(build, key):
    """The anchors (x, y, yaw) a key names: a layout placement, 'obstacle:<id>' a layout obstacle's middle, '@<Class>'
    every placed actor of that class, 'label:<prefix>' every placed actor whose label starts so (sorted)."""
    if key.startswith('obstacle:'):
        entry = next((o for o in build.source.get('obstacles', []) if o.get('id') == key[9:]), None)
        points = entry and (entry.get('polygon') or entry.get('path'))
        if not points:
            return []
        x, y = story.centroid([p[:2] for p in points])
        return [(x, y, 0.0)]
    if key.startswith('@') or key.startswith('label:'):
        found = []
        for actor in editor_actors():
            if not built(build, actor):
                continue
            if (key.startswith('@') and actor.get_class().get_name() == key[1:]) or \
                    (key.startswith('label:') and actor.get_actor_label().startswith(key[6:])):
                at = actor.get_actor_location()
                found.append((actor.get_actor_label(), (at.x, at.y, actor.get_actor_rotation().yaw)))
        return [spot for _, spot in sorted(found)]
    spot = build.layout['placements'].get(key)
    if spot is None:
        return []
    return [(spot['location'][0], spot['location'][1], spot.get('yaw', 0.0))]


# ---------------------------------------------------------------------------
# Placing
# ---------------------------------------------------------------------------

def place_breakable(build, cls, mesh, transform, breakable_id, from_dressing):
    """An ABreakableProp of the kind the dressing's mesh is, in transform (a dressing instance's, tilt and all)."""
    at = transform.translation
    tags = (TAG, 'Breakable', 'Obstacle') + ((FROM_DRESSING, MESH_TAG + mesh) if from_dressing else ())
    actor = build.place(cls, (at.x, at.y, at.z), yaw_of(transform), label=breakable_id, folder=FOLDER, tags=tags)
    actor.set_actor_transform(transform, False, True)
    actor.set_editor_property('kind', getattr(unreal.BreakableKind, BREAKABLE_MESHES[mesh]))
    # Set last, so the editor builds it again with its kind's body, pieces and stump.
    actor.set_editor_property('breakable_id', unreal.Name(breakable_id))
    return actor


def place_chest(build, cls, kind, transform, chest_id, extra_tags=(), override=None):
    """An AChest of a kind in transform; its models come from its kind (FChestKindInfo)."""
    at = transform.translation
    tags = (TAG, 'Chest', kind) + (() if kind == 'Grave' else ('Obstacle',)) + tuple(extra_tags)
    actor = build.place(cls, (at.x, at.y, at.z), yaw_of(transform), label=chest_id, folder=FOLDER, tags=tags)
    actor.set_actor_transform(transform, False, True)
    actor.set_editor_property('kind', getattr(unreal.ChestKind, CHEST_KINDS[kind]))
    if override:
        loot = unreal.ChestLootOverride()
        loot.set_editor_property('override', True)
        for name, value in override.items():
            loot.set_editor_property(name, value)
        actor.set_editor_property('loot_override', loot)
    actor.set_editor_property('chest_id', unreal.Name(chest_id))
    return actor


def flat_transform(x, y, z, yaw):
    return unreal.Transform(location=unreal.Vector(x, y, z), rotation=unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw),
                            scale=unreal.Vector(1.0, 1.0, 1.0))


def spot_id(word, x, y):
    return f'{word}_{x:.0f}_{y:.0f}'.replace('-', 'm')


def give_back_old(build, dressing):
    """What the last run took from the dressing goes back (unless the dressing was rebuilt since), then the last run's
    loot goes. Returns how many instances went back."""
    back = 0
    old = [a for a in editor_actors() if built(build, a) and unreal.Name(TAG) in a.tags]
    for actor in old:
        if unreal.Name(FROM_DRESSING) not in actor.tags:
            continue
        mesh = next((str(t)[len(MESH_TAG):] for t in actor.tags if str(t).startswith(MESH_TAG)), None)
        if mesh and dressing.give_back(mesh, actor.get_actor_transform()):
            back += 1
    if old:
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actors(old)
        build.log(f'loot: removed {len(old)} actors from the last build, {back} dressing pieces given back')
    return back


def dressing_breakables(build, cls, site, dressing, most, seed):
    """The dressing's wooden crates and barrels that become breakables where they stand (see the docstring)."""
    stack = dressing.stackable()
    candidates = []
    for mesh in sorted(BREAKABLE_MESHES):
        for index, t in dressing.instances(mesh):
            at = t.translation
            candidates.append((round(at.x), round(at.y), mesh, index, t))
    candidates.sort(key=lambda c: (c[0], c[1], c[2]))
    random.Random(seed).shuffle(candidates)
    chosen, skipped = [], {}
    for x, y, mesh, index, t in candidates:
        if len(chosen) >= most:
            break
        at = t.translation
        reasons = []
        if rests_on_it(stack, mesh, at.x, at.y, at.z):
            reasons.append('something stacked on it')
        if not site.road_clear(at.x, at.y, 0.0):
            reasons.append('on a road')
        reasons += [r for r in site.keep_off(at.x, at.y, CHEST_CLEAR, SPOT_CLEAR, DRESSING_STORY_CLEAR) if r != 'loot placed already']
        if any(math.hypot(cx - at.x, cy - at.y) < BREAKABLE_SPREAD for cx, cy, *_ in chosen) and most < len(candidates):
            reasons.append('another breakable near')
        if reasons:
            for reason in reasons:
                skipped[reason] = skipped.get(reason, 0) + 1
            continue
        chosen.append((at.x, at.y, mesh, index, t))
    taken = {}
    for x, y, mesh, index, t in chosen:
        place_breakable(build, cls, mesh, t, spot_id(f'Breakable_{mesh}', x, y), True)
        site.placed.append((x, y))
        taken.setdefault(mesh, []).append(index)
    for mesh, indices in taken.items():
        dressing.take(mesh, indices)
    build.log(f'loot: {len(chosen)} of the dressing\'s {len(candidates)} wooden crates and barrels breakable'
              + (f' (left: {", ".join(f"{n} {r}" for r, n in sorted(skipped.items()))})' if skipped else ''))
    return len(chosen)


def anchored(build, breakable_cls, cls, site, spec):
    """The breakables and containers standing round the anchors (AREAS anchors)."""
    counts = {}
    for n, (key, kind, radii) in enumerate(spec.get('anchors', [])):
        spots = anchors_of(build, key)
        if not spots:
            build.warn(f'loot: no anchor {key} in this level: no {kind} there')
            continue
        for m, anchor in enumerate(spots):
            half = BREAKABLE_HALF.get(kind) or CHEST_HALF[kind]
            reach = RELAXED if key.startswith('label:CampCache_') else None
            facing = None
            if kind == 'Coffin':
                # Lying along the shed's wall, its long side (its front) turned away from the shed: behind it, facing
                # back; in front, facing out the way the shed does.
                def facing(angle, shed=anchor[2]):
                    return shed + 180.0 if abs(((angle - shed + 180.0) % 360.0) - 180.0) > 90.0 else shed
            spot = find_spot(site, anchor, radii, half, seed=f'{build.name} {key} {kind} {n} {m}', reach=reach, facing=facing)
            if spot is None:
                build.warn(f'loot: no level, clear spot for a {kind} round {key} (looked at {", ".join(str(r) for r in radii)} cm)')
                continue
            x, y, z, yaw = spot
            transform = flat_transform(x, y, z, yaw)
            if kind in BREAKABLE_MESHES:
                place_breakable(build, breakable_cls, kind, transform, spot_id(f'Breakable_{kind}', x, y), False)
            else:
                word = key.split(':')[-1].replace('@', '')
                place_chest(build, cls, kind, transform, spot_id(f'{kind}_{word}', x, y),
                            override=LOOSE_STRONGBOX if kind == 'Strongbox' else None)
            site.placed.append((x, y))
            counts[kind] = counts.get(kind, 0) + 1
    if counts:
        build.log('loot: round the anchors ' + ', '.join(f'{n} {k}' for k, n in sorted(counts.items())))
    return counts


def mailboxes(build, cls, site, spec):
    """A mailbox by the road in front of each house named, facing the road."""
    made = 0
    for key in spec.get('mailboxes', []):
        spot = build.layout['placements'].get(key)
        if spot is None:
            build.warn(f'loot: no placement {key}: no mailbox there')
            continue
        hx, hy = spot['location'][:2]
        best = None
        for points, width in site.roads:
            for a, b in zip(points, points[1:]):
                dx, dy = b[0] - a[0], b[1] - a[1]
                length = dx * dx + dy * dy
                t = 0.0 if length == 0.0 else max(0.0, min(1.0, ((hx - a[0]) * dx + (hy - a[1]) * dy) / length))
                px, py = a[0] + t * dx, a[1] + t * dy
                d = math.hypot(px - hx, py - hy)
                if best is None or d < best[0]:
                    best = (d, px, py, width)
        if best is None or best[0] > 3500.0:
            build.warn(f'loot: no road near {key}: no mailbox there')
            continue
        d, px, py, width = best
        ux, uy = (px - hx) / max(d, 1.0), (py - hy) / max(d, 1.0)
        yaw = math.degrees(math.atan2(uy, ux))
        placed = None
        for side in (160.0, -160.0, 260.0, -260.0, 60.0):
            x = px - ux * (width * 0.5 + 90.0) - uy * side
            y = py - uy * (width * 0.5 + 90.0) + ux * side
            z, problems = site.judge(x, y, yaw, CHEST_HALF['Mailbox'], LEVEL_POST)
            if z is not None and not problems:
                placed = (x, y, z)
                break
        if placed is None:
            build.warn(f'loot: no clear spot by the road for {key}\'s mailbox')
            continue
        x, y, z = placed
        place_chest(build, cls, 'Mailbox', flat_transform(x, y, z, yaw), f'Mailbox_{key}')
        site.placed.append((x, y))
        made += 1
    build.log(f'loot: {made} mailboxes')
    return made


def stack_coffins(build, cls, site, dressing):
    """The top coffin of each of the dressing's coffin stacks (another under it, none on it) can be pried open."""
    stack = dressing.stackable()
    taken, made = [], 0
    for index, t in dressing.instances('Coffin_Closed'):
        at = t.translation
        if not has_under(stack, at.x, at.y, at.z) or rests_on_it(stack, 'Coffin_Closed', at.x, at.y, at.z):
            continue
        if site.keep_off(at.x, at.y):
            continue
        place_chest(build, cls, 'Coffin', t, spot_id('Coffin_Stack', at.x, at.y), (FROM_DRESSING, MESH_TAG + 'Coffin_Closed'))
        site.placed.append((at.x, at.y))
        taken.append(index)
        made += 1
    dressing.take('Coffin_Closed', taken)
    build.log(f'loot: {made} coffins on top of the yard\'s stacks')
    return made


def zone(build, zone_id):
    entry = next((z for z in build.source.get('zones', []) if z.get('id') == zone_id), None)
    return entry['polygon'] if entry else None


def yard_spots(build):
    """The churchyard fight's rising spots (build_area_chapel.YARD_SPOTS in the chapel's frame), in the level."""
    chapel = build.layout['placements'].get('chapel')
    if chapel is None:
        return []
    try:
        chapel_script = importlib.import_module('build_area_chapel')
    except ImportError:
        return []
    cx, cy = chapel['location'][:2]
    return [caches.frame(cx, cy, chapel['yaw'], sx, sy) for sx, sy in getattr(chapel_script, 'YARD_SPOTS', ())]


def graves(build, cls, site, dressing, spec, seed):
    """Boot hill's old graves and a couple of the churchyard's, dug with a hold (see the docstring)."""
    plan = spec.get('graves', {})
    if not plan:
        return 0
    made = 0
    boards = []
    for mesh in dressing.actors:
        if mesh.startswith(('Grave_Headboard_Old', 'Grave_Cross_')):
            boards += [t for _, t in dressing.instances(mesh)]
    board_spots = [(t.translation.x, t.translation.y) for t in boards]

    # Boot hill: the dressing's sunken mounds inside its zone.
    hill = zone(build, 'bootHill')
    chosen = []
    if hill and plan.get('bootHill'):
        mounds = [(round(t.translation.x), round(t.translation.y), i, t) for i, t in dressing.instances('Grave_MoundSunken')
                  if build_area.inside((t.translation.x, t.translation.y), hill)]
        mounds.sort(key=lambda m: (m[0], m[1]))
        random.Random(seed).shuffle(mounds)
        for x, y, index, t in mounds:
            if len(chosen) >= plan['bootHill']:
                break
            if site.keep_off(t.translation.x, t.translation.y) or any(math.hypot(cx - x, cy - y) < GRAVE_SPREAD for cx, cy, *_ in chosen):
                continue
            sx, sy = caches.frame(x, y, yaw_of(t), *SHOVEL_AT)
            if any(math.hypot(bx - sx, by - sy) < 45.0 for bx, by in board_spots):
                continue
            chosen.append((x, y, index, t))
        for x, y, index, t in chosen:
            place_chest(build, cls, 'Grave', t, spot_id('Grave_BootHill', x, y), (FROM_DRESSING, MESH_TAG + 'Grave_MoundSunken'))
            site.placed.append((x, y))
        dressing.take('Grave_MoundSunken', [c[2] for c in chosen])
        made += len(chosen)
        if len(chosen) < plan['bootHill']:
            build.warn(f'loot: only {len(chosen)} of boot hill\'s {plan["bootHill"]} graves to dig had room')

    # The churchyard: a mound laid before an old board in the chapel's zone, where there's room for it and its spade.
    yard = zone(build, 'chapel')
    rising = yard_spots(build)
    churchyard = []
    if yard and plan.get('churchyard'):
        candidates = sorted(((round(t.translation.x), round(t.translation.y), t) for t in boards
                             if build_area.inside((t.translation.x, t.translation.y), yard)), key=lambda c: (c[0], c[1]))
        random.Random(seed + 1).shuffle(candidates)
        for bx, by, t in candidates:
            if len(churchyard) >= plan['churchyard']:
                break
            yaw = yaw_of(t)
            x, y = caches.frame(bx, by, yaw, GRAVE_BOARD_OUT, 0.0)
            if any(math.hypot(rx - x, ry - y) < 300.0 for rx, ry in rising):
                continue
            if any(math.hypot(cx - x, cy - y) < GRAVE_SPREAD * 1.3 for cx, cy, *_ in churchyard + chosen):
                continue
            if any(math.hypot(ox - x, oy - y) < GRAVE_ROOM for ox, oy in board_spots if (ox, oy) != (t.translation.x, t.translation.y)):
                continue
            # Probed a little short of its ends (its own board stands 10 cm past its head), and the dressing's pieces
            # there are that board: other boards are kept off by GRAVE_ROOM.
            z, problems = site.judge(x, y, yaw, GRAVE_PROBE_HALF, LEVEL * 1.5)
            problems = [p for p in problems if p != 'on the dressing\'s pieces']
            if z is None or problems:
                continue
            sx, sy = caches.frame(x, y, yaw, *SHOVEL_AT)
            if site.terrain(sx, sy) is None or any(math.hypot(ox - sx, oy - sy) < 45.0 for ox, oy in board_spots):
                continue
            churchyard.append((x, y, z, yaw))
        for x, y, z, yaw in churchyard:
            place_chest(build, cls, 'Grave', flat_transform(x, y, z, yaw), spot_id('Grave_Churchyard', x, y))
            site.placed.append((x, y))
        made += len(churchyard)
        if len(churchyard) < plan['churchyard']:
            build.warn(f'loot: only {len(churchyard)} of the churchyard\'s {plan["churchyard"]} graves to dig had room')
    build.log(f'loot: {made} graves to dig ({len(chosen)} on boot hill, {len(churchyard)} in the churchyard)')
    return made


def place(build):
    """The area's lootable world (see the module's docstring), in its Loot folder."""
    spec = AREAS.get(build.name)
    if spec is None:
        build.log(f'loot: no lootable world planned for {build.name}')
        return
    breakable_cls, cls = breakable_class(), chest_class()
    if breakable_cls is None or cls is None:
        build.warn('loot: no BreakableProp or Chest class (build the game module first): no lootable world placed')
        return
    dressing = Dressing(build)
    give_back_old(build, dressing)
    site = Site(build, spec)
    if not site.tiles:
        return
    seed = sum(ord(c) for c in build.name) + 1009
    breakables = dressing_breakables(build, breakable_cls, site, dressing, spec.get('dressing_breakables', 0), seed)
    coffins = stack_coffins(build, cls, site, dressing) if spec.get('stack_coffins') else 0
    dug = graves(build, cls, site, dressing, spec, seed + 7)
    counts = anchored(build, breakable_cls, cls, site, spec)
    boxes = mailboxes(build, cls, site, spec)
    extra = sum(n for k, n in counts.items() if k in BREAKABLE_MESHES)
    containers = coffins + dug + boxes + sum(n for k, n in counts.items() if k not in BREAKABLE_MESHES)
    build.log(f'loot: {breakables + extra} breakables ({breakables} from the dressing), {containers} containers '
              f'({dug} graves, {coffins + counts.get("Coffin", 0)} coffins, {boxes} mailboxes, '
              f'{counts.get("Footlocker", 0)} footlockers, {counts.get("Strongbox", 0)} strongboxes)')
    missing = [p for p in ('/Game/Art/Props/SM_Break_CrateA_1', '/Game/Art/Props/SM_LootGrave', '/Game/Art/Props/SM_Mailbox')
               if not unreal.EditorAssetLibrary.does_asset_exist(p)]
    if missing:
        build.warn('loot: Art/Models/Props/Lootables.py isn\'t imported yet (no ' + ', '.join(missing) + '): the breakables '
                   'break into nothing and the new containers stand unseen until it is; build again after the import')


def run(name):
    """Places only the lootable world again: the last run's given back and removed, placed, the level saved."""
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    # The level as it is: nothing cleared here (build.open_level would clear the Loot folder before place() could give
    # the dressing back what the last run took from it).
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world.get_path_name().split('.')[0] != build.level:
        unsaved = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
                   if not p.get_name().startswith('/Temp/')]
        if unsaved:
            raise RuntimeError(f'unsaved changes in {unsaved}: save or discard them first')
        unreal.EditorLoadingAndSavingUtils.load_map(build.level)
        if hasattr(unreal, 'LooterLevelTools'):
            unreal.LooterLevelTools.finish_asset_compilation()
    place(build)
    build_area.levels.save_current_level()
    build.log('lootable world placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_loot.py <Area> (RansomsRest or TutorialIsland)')
    run(sys.argv[1])
