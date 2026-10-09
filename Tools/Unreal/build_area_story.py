"""The story's actors for build_area.py (Docs/Areas/RansomsRest.md: Main 1 "Seven Days", Main 2 "Shall We Talk Business?",
Main 3 "Cold Welcome", Main 4 "Hallowed Ground", Main 5 "The Keeper's Lantern", Main 6 "The Gravewind"), placed from the models build_area.py
placed and their sockets (Art/Models/Props/Graves.py, Buildings/Lookout.py, Buildings/Farmhouse.py,
Buildings/FalseFronts.py, Buildings/Chapel.py, Rocks/Outcrops.py's Den Rock) and from the layout's zones, obstacles,
features and roads, so they follow the level whenever it's rebuilt. Only a level whose layout has Ellis's grave
(Ransom's Rest) gets any of it, and each piece waits for what it's placed from (no lookout: no cold open, no Sexton).
Everything goes in the area's Gameplay folder, so a "gameplay" build places it all again.

Main 1:
- The cold open's set (AColdOpenSet) where the lookout stands, facing as it faces: its marks on Ransom's Point are the
  lookout's (Scenes/ColdOpenSet.h); the gang's skiff's course is in the world's own frame.
- The family plot's respawn grave (ARespawnMarker FamilyPlot, open once Main1 is finished) at the foot of Ellis's grave,
  where the wake-up stands the player (GraveWake::StandOut), facing as the level's start does (toward the bell tower).
- The two headboards to read (ASpeakerPoint tagged Headboard_Ellis and Headboard_Abel: "Read", from 2.5 m) at the graves'
  Interact sockets.
- Grandma Delia's screen door (ASpeakerPoint tagged Speaker_Delia) on the farmhouse's front door, and her house's screen
  door and plate (build_area_farm.py).
Main 2:
- Mister Sexton (AMisterSexton, tagged Speaker_Sexton) on the lookout's SOCKET_Sit, facing into the deck, there during
  Main 2. Talked to: the spiders first; the deal once they're cleared; the Ledger; and a word after, before he's gone.
- Ransom's Point's middle (a target point tagged Place_RansomsPoint, at the middle of the layout's zone ransomsPoint): where
  the climb ends.
- The spider nest (AEncounterSpawner BluffNest) in the middle of the nest ring: four spiders and a Restless one, tagged
  Spider_BluffNest, on during Main 2's climb and fight (its steps 1-2), appearing as the player comes up the bluff path
  (30 m), hunting only on the bluff top (the zone ransomsPoint as their ground), so the fight stays on one level.
Main 3:
- The town gate (a target point tagged Place_TownGate) between its posts (the obstacles townGateNorth and townGateSouth).
- The gate's fight (AEncounterSpawner TownGate) on the same spot: three Unpaid and a Restless one, tagged Unpaid_TownGate,
  on during Main 3's second step only, standing north-east of the gate (clear of the farm fence), coming for the player as
  they appear, and giving up 30 m from the gate (or at Delia's salt line, the farm's safe zone).
- Tilly's window (ASpeakerPoint tagged Speaker_Tilly) on Bright & Daughter's SOCKET_Speaker: the shop closed until Main 3's
  last step, then her lines, then a word after.
- The shutters (AWindowShutter) on every SOCKET_Shutter_<L|R><n> of Pruitt's store (FalseFronts.py: eight), open until the
  player comes near, shut from the start after Main 3.
- The safe zones (ASafeGround) from the layout's zones: Delia's salt line round the farm (zone farm, always) and Main
  Street (zone mainStreet, after Main 3).
Main 4 (build_area_chapel.py, with these helpers): the chapel's place, the chapel yard's fight, the bell, the smashed
  Reliquary, Father Aldana's vestry door, the chapel yard's respawn grave, and the Unpaid on boot hill and the north road
  after Main 4.
Main 5 (build_area_sink.py, with these helpers): the Sink's floor dressed (collapsed blocks and coffins, web cards),
  its three egg sacs (shot down in Main 5's second step, two spiders each), the Keeper's Lantern in the webbing (taken
  in its third), the floor's and the ramp head's places, the floor's spiders, the den's web funnel and the Webwood's
  trees.
Main 6 (build_area_deck.py, with these helpers): the burial deck's biers and keeper's lantern posts, Abel on the boards
  (during Main 6), Pa's board after it, the fog wall across the Keeper's Gate, Gravewind Point's place, the keeper's grave's
  respawn (after Main 5), the story's dusk, and the Gravewind's wisps and canyon fog (seen only at dusk).
Main 7 (build_area_depot.py, with these helpers): the train at the depot's platform (cold until Main 7), the depot's
  place at the hearse car's door, Delia's hand-off of Heirloom through her screen door, and the lit lantern's leaning flame.
Side 2 (build_area_whitlock.py, with these helpers): Amos leaning on his fence by the gate (its span from
  build_area_dressing's plan, which places Whitlock Fields' fences, wall and round bales; after Main 4; on the rail after
  Side 2), his six hay bales in the west field with their stack by the barn's doors, and his old hired hands' fight in
  the barn yard (during Side 2's third step). Hob has no perches of his own for it.
Side 3 (build_area_den.py): the Gravemother's lair at the den's mouth under Den Rock (after Main 5) and the den's place.
The Ranger caches (build_area_caches.py, step 26): Ruth Calder's three Supply Crates (under the windmill, on the Sink's
  rim, on the bluff path) and the gang's Strongbox on the sheriff's office's floor, open from the start.
Hob (AHobBird, tagged Speaker_Hob), perched near the next thing to do, saying his piece as he lands:
  Main 1: on Ellis's headboard (silent from the claw-out, "Morning, sunshine" once Ellis is out), and after it.
  Main 2: at the bluff path's foot ("Someone's waiting on you..."), on the bluff top's east rock as the fight begins ("See
          the blue on that one?..."), on the lookout's front rail beside Sexton for the deal, and there after it.
  Main 3: on the town gate's north post (its SOCKET_Perch: the farm road's way, then the fight), on Bright & Daughter's
          sign once the gate is won, and there after Main 3.
  Main 4: on the chapel's door hood (SOCKET_Perch_Hood) for the way up, the yard's fight, the bell and the Reliquary (a
          word at each, no flight between them), then on the vestry lantern's bracket (SOCKET_Perch_Lantern) for Aldana,
          and there after Main 4.
  Main 5: on Den Rock over the den for the way down ("Keepers walk the dead to the boards by lantern light..."), on
          a block on the Sink's floor for the egg sacs and then the lantern (a word at each, no flight), on the Sink
          road's dead tree by the ramp head once the lantern is taken, and there after Main 5.
  Main 6: on the keeper's grave's board for the way to Gravewind Point, on the Keeper's Gate's north rock for the lantern
          and the fight (a word at each, no flight between them), on the bier beside Pa's for the scene, and there after
          Main 6.
  Main 7: on the keeper's post by the leaning lantern as it begins ("That's Purcell. The Lily's moored out that way."),
          on Tilly's hearse car's roof once Delia has handed Heirloom out, and the station board (a word at each), and
          there after Main 7.
  Talked to, a line for where things stand.
Their words are line sets in /Game/Data/Story (Tools/Unreal/create_story_lines.py makes them first); a set that's
missing is left out with a warning, and its speaker says nothing until the sets are made and this runs again.
"""
import importlib
import math

import unreal

CLASSES = '/Script/AI_Looter_Shooter.'
LINES = '/Game/Data/Story/'

# The story's ids and tags, as the C++ and the mission assets (Tools/Unreal/create_mission_assets.py) name them.
MAIN1 = 'Main1'
MAIN2 = 'Main2'
MAIN3 = 'Main3'
MAIN4 = 'Main4'
MAIN5 = 'Main5'
MAIN6 = 'Main6'
MAIN7 = 'Main7'
FAMILY_PLOT = 'FamilyPlot'
PLACE_POINT = 'Place_RansomsPoint'
PLACE_GATE = 'Place_TownGate'
NEST = 'BluffNest'
NEST_TAG = 'Spider_BluffNest'
GATE = 'TownGate'
GATE_TAG = 'Unpaid_TownGate'

# How far out of the grave's foot the wake-up stands the player, from the hole's middle (cm; GraveWake::StandOut).
STAND_OUT = 190.0
# Hob's grip on top of a headboard, from its Interact socket (55 cm up its face): up to the board's top, and back over
# its thickness and lean (cm).
PERCH_UP = 30.0
PERCH_BACK = 12.0
# Reading a headboard is done close to (cm).
READ_REACH = 250.0
# A conversation through a window carries as far as one through a door (USpeakerPointComponent's default, cm).
TALK_REACH = 400.0

# The places' markers stand about where the player's middle is, so a place measured with its height (the bluff top's)
# counts the player standing there (cm).
MARKER_UP = 90.0

# The spider nest (Main 2): it comes as the player climbs the bluff path, under the top's edge and out of sight of it (cm).
NEST_ACTIVATION = 3000.0
NEST_SPREAD = 900.0
# The gate's fight (Main 3): the Unpaid stand north-east of the gate, in the street's mouth and the field north of it,
# 9-19 m out (cm, from the gate, X north and Y east); they give up 30 m from the gate.
GATE_SPOTS = ((600.0, 900.0), (1100.0, 400.0), (300.0, 1500.0), (1300.0, 1300.0))
GATE_GIVE_UP = 3000.0

# Shutters (FalseFronts.py's Shutter_Left and _Right, hung on the store's sockets).
SHUTTER_MESHES = {'L': '/Game/Art/Buildings/SM_Shutter_Left', 'R': '/Game/Art/Buildings/SM_Shutter_Right'}

# Hob on the lookout's front rail beside Sexton: this far along it from Sexton's seat, toward the deck's middle (cm; the
# rail runs along the lookout's model X, which is Unreal's -Y: the seat is left of middle, Lookout.py).
RAIL_ALONG = 120.0
# Hob on Bright & Daughter's sign: the ledge over the sign board, in the shop's model frame (FalseFronts.py: the facade's
# face 6 m out from the pivot, the ledge 0.22 m deep on it, its top 4.67 m over the 0.38 m floor) (cm).
SIGN_LEDGE = unreal.Vector(615.0, 0.0, 505.0)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def actor_class(name):
    return unreal.load_class(None, CLASSES + name)


def placed(build, kind):
    """The actor build_area.py placed for the first placement (or level prop) of this kind (labelled with its key), or
    None."""
    keys = [key for key, spot in build.layout['placements'].items() if spot['kind'] == kind]
    keys += [prop['id'] for prop in build.props if prop['kind'] == kind]
    if not keys:
        return None
    level_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    for actor in level_actors:
        if actor.get_actor_label() == keys[0] and unreal.Name(build.tag) in actor.tags:
            return actor
    build.warn(f'no {kind} placed for {keys[0]} (build the whole level first): what stands on it is left out')
    return None


def layout_entry(build, section, entry_id):
    """An entry of the layout's own lists (zones, obstacles, features, roads) by its id, or None (with a warning)."""
    found = next((each for each in build.source.get(section, []) if each.get('id') == entry_id), None)
    if found is None:
        build.warn(f'layout.json has no {section} entry {entry_id}: what stands on it is left out')
    return found


def centroid(points):
    """The middle of a polygon's area, seen from above ((x, y) pairs in order)."""
    area = cx = cy = 0.0
    for (x0, y0), (x1, y1) in zip(points, list(points[1:]) + [points[0]]):
        cross = x0 * y1 - x1 * y0
        area += cross
        cx += (x0 + x1) * cross
        cy += (y0 + y1) * cross
    if abs(area) < 1e-6:
        return sum(p[0] for p in points) / len(points), sum(p[1] for p in points) / len(points)
    return cx / (3.0 * area), cy / (3.0 * area)


def lines(build, name):
    """A line set the story script made, or None (with a warning)."""
    path = LINES + name
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        build.warn(f'no {path} yet (run Tools/Unreal/create_story_lines.py first)')
        return None
    return unreal.load_asset(path)


def condition(after=(), before=(), during='', from_step=0, before_step=0):
    """FStoryCondition: after these missions, before those, during one (from a step, and before a step, counted from 0)."""
    made = unreal.StoryCondition()
    made.set_editor_property('after_missions', [unreal.Name(m) for m in after])
    made.set_editor_property('before_missions', [unreal.Name(m) for m in before])
    made.set_editor_property('during_mission', unreal.Name(during))
    made.set_editor_property('from_step', from_step)
    if before_step:
        made.set_editor_property('before_step', before_step)
    return made


def topic(when, line_set):
    made = unreal.SpeakerTopic()
    made.set_editor_property('when', when)
    if line_set:
        made.set_editor_property('line_set', line_set)
    return made


def topics(build, *pairs):
    """Speaker topics from (condition, line set name) pairs, in order (the first that holds is said), leaving out any
    whose set isn't made yet: its point then says what comes next for that part of the story."""
    made = []
    for when, name in pairs:
        line_set = lines(build, name)
        if line_set:
            made.append(topic(when, line_set))
    return made


def socket(actor, name):
    """A socket of the actor's model in the world (an unreal.Transform), or None when the model has none so named."""
    mesh = actor.static_mesh_component
    if mesh is None or not mesh.does_socket_exist(name):
        return None
    return mesh.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)


def ground(x, y, z, ignore=()):
    """The ground's height under (x, y), looking from a little above z, past the actors in ignore (the grave itself,
    whose heap reaches toward its foot), as the wake-up's own trace does (GraveWake.cpp)."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, z + 150.0), unreal.Vector(x, y, z - 300.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, list(ignore),
                                                 unreal.DrawDebugTrace.NONE, True)
    return z if hit is None else hit.to_tuple()[5].z


def ground_at(x, y, default=0.0, warn=True):
    """The first thing under (x, y) from high over the level: the terrain, or the top of a rock or a post standing there.
    Nothing there (open canyon past a rim, or no terrain placed) gives default, with a warning unless warn is off."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -20000.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        if warn:
            unreal.log_warning(f'no ground under ({x:.0f}, {y:.0f}): it stands at {default:.0f} (is the terrain placed?)')
        return default
    return hit.to_tuple()[5].z


def onto_host(host, start, end):
    """Where the line from start to end first meets one of the host's own meshes, or None (as the posters snap)."""
    best = None
    for mesh in host.get_components_by_class(unreal.StaticMeshComponent):
        hit = mesh.line_trace_component(start, end, True, False, False)
        if not hit:
            continue
        location = hit[0] if isinstance(hit, tuple) else hit.to_tuple()[5]
        distance = (location - start).length()
        if best is None or distance < best[0]:
            best = (distance, location)
    return None if best is None else best[1]


def facing(from_xy, to_xy):
    """The yaw (degrees) looking from one point to another, seen from above."""
    return math.degrees(math.atan2(to_xy[1] - from_xy[1], to_xy[0] - from_xy[0]))


def speaker(build, cls, location, yaw, label, tag, name='', prompt='', reach=0.0, line_set=None, topics=()):
    """A speaker point (a door, a headboard, a window) with its point at location when reach is set (a headboard's face, a
    window's socket), else over it (a door's middle)."""
    point = build.place(cls, location, yaw, label=label, folder='Gameplay', tags=(tag,))
    talk = point.get_editor_property('speaker_point')
    talk.set_editor_property('speaker_name', unreal.Text(name))
    if prompt:
        talk.set_editor_property('prompt', unreal.Text(prompt))
    if reach:
        talk.set_editor_property('reach', reach)
        talk.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, True)
    if line_set:
        talk.set_editor_property('line_set', line_set)
    if topics:
        talk.set_editor_property('topics', list(topics))
    return point


def marker(build, x, y, tag):
    """A target point tagged for a mission's place, about the player's middle over the ground there."""
    z = ground_at(x, y) + MARKER_UP
    build.place(unreal.TargetPoint, (x, y, z), 0.0, label=tag, folder='Gameplay', tags=(tag,))
    return z


def group(cls, count, rank, first_wave=1, last_wave=0):
    """FEncounterGroup: count creatures of cls, all of one rank ('BASIC', 'RARE'), in each wave from first_wave to
    last_wave (counted from 1; 0: every wave from the first)."""
    made = unreal.EncounterGroup()
    made.set_editor_property('creature_class', cls)
    made.set_editor_property('count', count)
    made.set_editor_property('rank_roll', unreal.EncounterRankRoll.FIXED)
    made.set_editor_property('rank', getattr(unreal.CreatureRank, rank))
    made.set_editor_property('first_wave', first_wave)
    made.set_editor_property('last_wave', last_wave)
    return made


# ---------------------------------------------------------------------------
# Main 1
# ---------------------------------------------------------------------------

def place_cold_open(build):
    lookout = placed(build, 'Lookout')
    cls = actor_class('ColdOpenSet')
    if lookout is None or cls is None:
        if cls is None:
            build.warn('no ColdOpenSet class (build the game module first): no cold open')
        return
    at = lookout.get_actor_location()
    build.place(cls, (at.x, at.y, at.z), lookout.get_actor_rotation().yaw, label='ColdOpenSet', folder='Gameplay')
    build.log(f'the cold open\'s set at the lookout ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')


def place_family_plot(build):
    grave = placed(build, 'Grave_Ellis')
    marker_cls = actor_class('RespawnMarker')
    if grave is None or marker_cls is None:
        return None
    rise = socket(grave, 'Respawn')
    hole = rise.translation if rise else grave.get_actor_location()
    foot = grave.get_actor_forward_vector()
    x, y = hole.x + foot.x * STAND_OUT, hole.y + foot.y * STAND_OUT
    spawn = next((s for s in build.layout['placements'].values() if s['kind'] == 'PlayerStart'), None)
    yaw = spawn['yaw'] if spawn else grave.get_actor_rotation().yaw
    grave_marker = build.place(marker_cls, (x, y, ground(x, y, hole.z, ignore=[grave])), yaw, label='Respawn_FamilyPlot',
                               folder='Gameplay')
    grave_marker.set_editor_property('marker_id', unreal.Name(FAMILY_PLOT))
    # What the map calls it (the layout has no label for it; the design's words: "the family plot").
    grave_marker.set_editor_property('display_name', unreal.Text('Family plot'))
    grave_marker.set_editor_property('active_after_mission', unreal.Name(MAIN1))
    build.log(f'the family plot\'s respawn grave at ({x:.0f}, {y:.0f}), open after {MAIN1}')
    return grave


def place_headboards(build, grave_ellis):
    cls = actor_class('SpeakerPoint')
    if cls is None:
        return
    for kind, tag, line_set in (('Grave_Ellis', 'Headboard_Ellis', 'DA_Lines_HeadboardEllis'),
                                ('Grave_Abel', 'Headboard_Abel', 'DA_Lines_HeadboardAbel')):
        grave = grave_ellis if kind == 'Grave_Ellis' else placed(build, kind)
        face = socket(grave, 'Interact') if grave else None
        if face is None:
            if grave:
                build.warn(f'{kind} has no Interact socket: its headboard can\'t be read')
            continue
        at = face.translation
        speaker(build, cls, (at.x, at.y, at.z), face.rotation.rotator().yaw, tag, tag, prompt='Read', reach=READ_REACH,
                line_set=lines(build, line_set))
    build.log('the headboards, to read')


# ---------------------------------------------------------------------------
# Main 2: Ransom's Point
# ---------------------------------------------------------------------------

def place_sexton(build, lookout, seat):
    """Mister Sexton on the lookout's Sit socket (seat), there during Main 2."""
    cls = actor_class('MisterSexton')
    if cls is None or seat is None:
        if cls is None:
            build.warn('no MisterSexton class (build the game module first): no Sexton on the rail')
        elif lookout is not None:
            build.warn('the lookout has no Sit socket: no Sexton on the rail')
        return
    at = seat.translation
    sexton = build.place(cls, (at.x, at.y, at.z), seat.rotation.rotator().yaw, label='MisterSexton', folder='Gameplay',
                         tags=('Speaker_Sexton',))
    # Snapped to the socket, as his model's notes ask (MisterSexton.py): its whole turn, not only its yaw.
    sexton.set_actor_rotation(seat.rotation.rotator(), False)
    sexton.set_editor_property('shown_when', condition(during=MAIN2))
    # The first that holds is said: the Ledger's step (from 0: 3), the deal (step 2), the climb and the fight, after.
    sexton.get_editor_property('speaker_point').set_editor_property('topics', topics(
        build,
        (condition(during=MAIN2, from_step=3), 'DA_Lines_SextonLedger'),
        (condition(during=MAIN2, from_step=2), 'DA_Lines_SextonDeal'),
        (condition(during=MAIN2), 'DA_Lines_SextonWaiting'),
        (condition(after=[MAIN2]), 'DA_Lines_SextonAfter'),
    ))
    build.log(f'Mister Sexton on the lookout\'s rail at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}), during {MAIN2}')


def place_bluff_top(build):
    """Ransom's Point's middle, where the climb ends, and the spider nest in its ring. Returns the nest's middle."""
    top = layout_entry(build, 'zones', 'ransomsPoint')
    ring = layout_entry(build, 'obstacles', 'nestRing')
    if top is None:
        return None
    cx, cy = centroid(top['polygon'])
    marker(build, cx, cy, PLACE_POINT)
    build.log(f'Ransom\'s Point\'s middle at ({cx:.0f}, {cy:.0f}), tagged {PLACE_POINT}')

    spider = actor_class('SpiderCreature')
    cls = actor_class('EncounterSpawner')
    if ring is None or spider is None or cls is None:
        if cls is None or spider is None:
            build.warn('no EncounterSpawner or SpiderCreature class (build the game module first): no spider nest')
        return None
    nx, ny = centroid(ring['polygon'])
    nz = ground_at(nx, ny)
    nest = build.place(cls, (nx, ny, nz + 50.0), 0.0, label=f'Encounter_{NEST}', folder='Gameplay')
    nest.set_editor_property('spawner_id', unreal.Name(NEST))
    # "4, and one Restless Meadow Wolf": four Basic spiders and a Restless one.
    nest.set_editor_property('groups', [group(spider, 4, 'BASIC'), group(spider, 1, 'RARE')])
    nest.set_editor_property('creature_tags', [unreal.Name(NEST_TAG)])
    # The climb and the fight (Main 2's steps 1 and 2, from 0: before 2): a session loaded at the deal finds no spiders.
    nest.set_editor_property('active_when', condition(during=MAIN2, before_step=2))
    nest.set_editor_property('activation_radius', NEST_ACTIVATION)
    nest.set_editor_property('spawn_radius', NEST_SPREAD)
    # Their ground is the bluff top: nobody on the path below it, or on the farm, is hunted from up here.
    nest.set_editor_property('ground_corners', [unreal.Vector(x, y, nz) for x, y in top['polygon']])
    build.log(f'the spider nest ({NEST}: 4 spiders and a Restless one) at ({nx:.0f}, {ny:.0f}, {nz:.0f}), during {MAIN2}\'s '
              f'climb and fight')
    return nx, ny, nz


# ---------------------------------------------------------------------------
# Main 3: the town
# ---------------------------------------------------------------------------

def place_town_gate(build):
    """The town gate between its posts, and the Unpaid's fight there. Returns the gate's middle (x, y, z), or None."""
    posts = [layout_entry(build, 'obstacles', key) for key in ('townGateNorth', 'townGateSouth')]
    if any(post is None for post in posts):
        return None
    middles = [centroid(post['polygon']) for post in posts]
    gx, gy = (middles[0][0] + middles[1][0]) * 0.5, (middles[0][1] + middles[1][1]) * 0.5
    marker(build, gx, gy, PLACE_GATE)
    gz = ground_at(gx, gy)
    build.log(f'the town gate at ({gx:.0f}, {gy:.0f}), tagged {PLACE_GATE}')

    unpaid = actor_class('UnpaidCreature')
    cls = actor_class('EncounterSpawner')
    if unpaid is None or cls is None:
        build.warn('no EncounterSpawner or UnpaidCreature class (build the game module first): no fight at the gate')
        return gx, gy, gz
    gate = build.place(cls, (gx, gy, gz + 50.0), 0.0, label=f'Encounter_{GATE}', folder='Gameplay')
    gate.set_editor_property('spawner_id', unreal.Name(GATE))
    # "4, one of them Restless": three Basic Unpaid and a Restless one.
    gate.set_editor_property('groups', [group(unpaid, 3, 'BASIC'), group(unpaid, 1, 'RARE')])
    gate.set_editor_property('creature_tags', [unreal.Name(GATE_TAG)])
    # Main 3's second step only (from 0: from 1, before 2): a fight already won doesn't come back on a later load.
    gate.set_editor_property('active_when', condition(during=MAIN3, from_step=1, before_step=2))
    # They come for the corpse: from the street's mouth and the field north of it, at the player as they appear.
    gate.set_editor_property('spawn_points', [unreal.Vector(x, y, 0.0) for x, y in GATE_SPOTS])
    gate.set_editor_property('hunt_on_spawn', True)
    gate.set_editor_property('give_up_radius', GATE_GIVE_UP)
    build.log(f'the gate\'s fight ({GATE}: 3 Unpaid and a Restless one) at ({gx:.0f}, {gy:.0f}), during {MAIN3}\'s second '
              f'step')
    return gx, gy, gz


def place_tilly(build, shop):
    """Tilly's window: her speaker point on Bright & Daughter's Speaker socket."""
    cls = actor_class('SpeakerPoint')
    window = socket(shop, 'Speaker') if shop else None
    if cls is None or window is None:
        if shop is not None and cls is not None:
            build.warn('Bright & Daughter has no Speaker socket: Tilly can\'t be talked to')
        return
    at = window.translation
    # Closed until Main 3's last step (from 0: 3); her lines then; a word after.
    speaker(build, cls, (at.x, at.y, at.z), window.rotation.rotator().yaw, 'Speaker_Tilly', 'Speaker_Tilly',
            name='Tilly Bright', reach=TALK_REACH, line_set=lines(build, 'DA_Lines_TillyClosed'),
            topics=topics(build,
                          (condition(during=MAIN3, from_step=3), 'DA_Lines_TillyMain3'),
                          (condition(during=MAIN7), 'DA_Lines_TillyMain7'),
                          (condition(after=[MAIN7]), 'DA_Lines_TillyAfterMain7'),
                          (condition(after=[MAIN3]), 'DA_Lines_TillyAfterMain3')))
    build.log(f'Tilly\'s window at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')


def place_shutters(build):
    """A shutter on every Shutter_ socket of the store, shut for good after Main 3."""
    store = placed(build, 'FalseFront_Store')
    cls = actor_class('WindowShutter')
    if store is None or cls is None:
        if cls is None:
            build.warn('no WindowShutter class (build the game module first): no shutters')
        return
    meshes = {tag: unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
              for tag, path in SHUTTER_MESHES.items()}
    mesh_component = store.static_mesh_component
    count = 0
    for name in sorted(str(each) for each in mesh_component.get_all_socket_names()):
        if not name.startswith('Shutter_') or name[len('Shutter_')] not in meshes:
            continue
        leaf = meshes[name[len('Shutter_')]]
        if leaf is None:
            build.warn(f'no {SHUTTER_MESHES[name[len("Shutter_")]]} yet (import FalseFronts.py): the {name} shutter is left out')
            continue
        hinge = mesh_component.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)
        at = hinge.translation
        shutter = build.place(cls, (at.x, at.y, at.z), hinge.rotation.rotator().yaw, label=name, folder='Gameplay')
        shutter.set_actor_rotation(hinge.rotation.rotator(), False)
        shutter.get_editor_property('leaf').set_static_mesh(leaf)
        # Set last, so the editor builds it again with its leaf and shows it swung open the right way.
        shutter.set_editor_property('shut_when', condition(after=[MAIN3]))
        count += 1
    build.log(f'{count} shutters on the store, shut after {MAIN3}')


def place_safe_zones(build):
    """Delia's salt line round the farm (always) and Main Street (after Main 3), from the layout's zones."""
    cls = actor_class('SafeGround')
    if cls is None:
        build.warn('no SafeGround class (build the game module first): no safe zones')
        return
    for zone_id, layout_id, after in (('Farm', 'farm', ()), ('MainStreet', 'mainStreet', (MAIN3,))):
        zone = layout_entry(build, 'zones', layout_id)
        if zone is None:
            continue
        cx, cy = centroid(zone['polygon'])
        z = ground_at(cx, cy)
        made = build.place(cls, (cx, cy, z), 0.0, label=f'SafeZone_{zone_id}', folder='Gameplay')
        made.set_editor_property('zone_id', unreal.Name(zone_id))
        made.set_editor_property('corners', [unreal.Vector(x, y, ground_at(x, y, z)) for x, y in zone['polygon']])
        made.set_editor_property('active_when', condition(after=after))
        build.log(f'the safe zone {zone_id} ({len(zone["polygon"])} corners), '
                  f'{"after " + ", ".join(after) if after else "always"}')


# ---------------------------------------------------------------------------
# Hob
# ---------------------------------------------------------------------------

def hob_on_board(grave_ellis):
    """On Ellis's headboard: over its Interact socket, back over its lean. (x, y, z, yaw) or None."""
    face = socket(grave_ellis, 'Interact') if grave_ellis else None
    if face is None:
        return None
    # A Quat has no forward vector in Python: the rotator's does.
    front = face.rotation.rotator().get_forward_vector()
    at = face.translation
    return at.x - front.x * PERCH_BACK, at.y - front.y * PERCH_BACK, at.z + PERCH_UP, face.rotation.rotator().yaw


def hob_at_path_foot(build):
    """At the bluff path's foot (the end of the way from the family plot), facing back down it toward the player."""
    road = layout_entry(build, 'roads', 'bluffApproach')
    if road is None or len(road['path']) < 2:
        return None
    (fx, fy), back = road['path'][-1], road['path'][-2]
    return fx, fy, ground_at(fx, fy), facing((fx, fy), back)


def hob_on_bluff_rock(build, nest):
    """On top of the bluff top's east rock, looking over the nest."""
    rock = layout_entry(build, 'features', 'bluffRockEast')
    if rock is None:
        return None
    rx, ry = rock['center']
    return rx, ry, ground_at(rx, ry), facing((rx, ry), nest[:2]) if nest else 0.0


def hob_on_rail(lookout, seat):
    """On the lookout's front rail a little along from Sexton, facing into the deck as he does."""
    if lookout is None or seat is None:
        return None
    along = lookout.get_actor_right_vector()
    at = seat.translation
    return at.x - along.x * RAIL_ALONG, at.y - along.y * RAIL_ALONG, at.z, seat.rotation.rotator().yaw


def hob_on_gate_post(build):
    """On the town gate's north post, looking down the farm road the player comes by: on the gate's SOCKET_Perch, on the
    post's cap (TownGate.py), or without the gate's model over the post's footprint."""
    gate = placed(build, 'TownGate')
    perch = socket(gate, 'Perch') if gate is not None else None
    if perch is not None:
        at = perch.translation
        return at.x, at.y, at.z, perch.rotation.rotator().yaw
    post = layout_entry(build, 'obstacles', 'townGateNorth')
    road = layout_entry(build, 'roads', 'farmRoad')
    if post is None:
        return None
    px, py = centroid(post['polygon'])
    toward = road['path'][-2] if road and len(road['path']) >= 2 else (px - 1000.0, py - 1000.0)
    return px, py, ground_at(px, py), facing((px, py), toward)


def hob_on_sign(shop):
    """On the ledge over Bright & Daughter's sign board, facing the street."""
    if shop is None:
        return None
    at = shop.get_actor_transform().transform_location(SIGN_LEDGE)
    top = onto_host(shop, unreal.Vector(at.x, at.y, at.z + 60.0), unreal.Vector(at.x, at.y, at.z - 60.0))
    z = top.z if top is not None else at.z
    return at.x, at.y, z, shop.get_actor_rotation().yaw


def place_hob(build, grave_ellis, lookout, seat, nest, shop, chapel_spots, sink_spots, deck_spots, depot_spots):
    cls = actor_class('HobBird')
    board = hob_on_board(grave_ellis)
    if cls is None or board is None:
        return
    spots = {
        'path': hob_at_path_foot(build),
        'rock': hob_on_bluff_rock(build, nest),
        'rail': hob_on_rail(lookout, seat),
        'gate': hob_on_gate_post(build),
        'sign': hob_on_sign(shop),
        # The chapel's door hood and vestry lantern (build_area_chapel.hob_spots).
        'hood': chapel_spots.get('hood'),
        'lantern': chapel_spots.get('lantern'),
        # The Sink's: Den Rock's top, the north pile's top block, the Sink road's dead tree (build_area_sink.hob_spots).
        'denRock': sink_spots.get('way'),
        'block': sink_spots.get('block'),
        'roadTree': sink_spots.get('rim'),
        # Main 6's: the keeper's grave's board, the Keeper's Gate's north rock, the bier beside Pa's (build_area_deck.hob_spots).
        'keeperGrave': deck_spots.get('keeperGrave'),
        'gateRock': deck_spots.get('gateRock'),
        'deckBier': deck_spots.get('deckBier'),
        # Main 7's: the keeper's post's top, Tilly's hearse car's roof (build_area_depot.place).
        'keeperPost': depot_spots.get('post'),
        'hearse': depot_spots.get('hearse'),
    }

    def perch(spot, when, arrival=None):
        """One perch, or None when its spot isn't there (its model isn't placed): a later perch holds instead."""
        if spot is None:
            return None
        x, y, z, yaw = spot
        made = unreal.HobPerch()
        made.set_editor_property('when', when)
        made.set_editor_property('location', unreal.Vector(x, y, z))
        made.set_editor_property('yaw', yaw)
        if arrival:
            line_set = lines(build, arrival)
            if line_set:
                made.set_editor_property('arrival_set', line_set)
        return made

    x, y, z, yaw = board
    hob = build.place(cls, (x, y, z), yaw, label='Hob', folder='Gameplay', tags=('Speaker_Hob',))
    # The first that holds is his, so a mission's later steps come before its earlier ones, and each mission's perches
    # before the ones after the missions before it (steps counted from 0).
    perches = [
        # Main 1: on the board saying his piece once Ellis is out (step 2), on it silently while Ellis claws out (step 1).
        perch(board, condition(during=MAIN1, from_step=2), 'DA_Lines_HobWakes'),
        perch(board, condition(during=MAIN1, from_step=1)),
        # Main 2: beside Sexton for the deal and the Ledger; on the rock for the fight; at the path's foot for the climb.
        perch(spots['rail'], condition(during=MAIN2, from_step=2)),
        perch(spots['rock'], condition(during=MAIN2, from_step=1), 'DA_Lines_HobMain2Blue'),
        perch(spots['path'], condition(during=MAIN2), 'DA_Lines_HobMain2Climb'),
        # Main 3: on the sign once the gate is won; on the gate's post for the fight (no flight: the same spot) and the road.
        perch(spots['sign'], condition(during=MAIN3, from_step=2), 'DA_Lines_HobMain3Tilly'),
        perch(spots['gate'], condition(during=MAIN3, from_step=1), 'DA_Lines_HobMain3Gate'),
        perch(spots['gate'], condition(during=MAIN3), 'DA_Lines_HobMain3Road'),
        # Main 4: on the vestry lantern's bracket for Aldana; on the door hood for the Reliquary, the bell, the yard's fight
        # and the way up (no flights between those: the same spot, a word at each).
        perch(spots['lantern'], condition(during=MAIN4, from_step=4), 'DA_Lines_HobMain4Aldana'),
        perch(spots['hood'], condition(during=MAIN4, from_step=3), 'DA_Lines_HobMain4Reliquary'),
        perch(spots['hood'], condition(during=MAIN4, from_step=2), 'DA_Lines_HobMain4Bell'),
        perch(spots['hood'], condition(during=MAIN4, from_step=1), 'DA_Lines_HobMain4Yard'),
        perch(spots['hood'], condition(during=MAIN4), 'DA_Lines_HobMain4Road'),
        # Main 5: on the Sink road's tree by the ramp head once the lantern is taken; on the floor's block for the
        # lantern and the sacs (no flight between them: a word at each); on Den Rock over the den for the way down.
        perch(spots['roadTree'], condition(during=MAIN5, from_step=3), 'DA_Lines_HobMain5Out'),
        perch(spots['block'], condition(during=MAIN5, from_step=2), 'DA_Lines_HobMain5Lantern'),
        perch(spots['block'], condition(during=MAIN5, from_step=1), 'DA_Lines_HobMain5Sacs'),
        perch(spots['denRock'], condition(during=MAIN5), 'DA_Lines_HobMain5Way'),
        # Main 6: on the bier beside Pa's for the scene (his word is the scene's own); on the gate's north rock for the fight
        # and the lantern (no flight between them: a word at each); on the keeper's grave's board for the way there.
        perch(spots['deckBier'], condition(during=MAIN6, from_step=3)),
        perch(spots['gateRock'], condition(during=MAIN6, from_step=2), 'DA_Lines_HobMain6Fight'),
        perch(spots['gateRock'], condition(during=MAIN6, from_step=1), 'DA_Lines_HobMain6Post'),
        perch(spots['keeperGrave'], condition(during=MAIN6), 'DA_Lines_HobMain6Way'),
        # Main 7: on the hearse car's roof for the depot and the board (a word at each); on the keeper's post by the lantern.
        perch(spots['hearse'], condition(during=MAIN7, from_step=2), 'DA_Lines_HobMain7Board'),
        perch(spots['hearse'], condition(during=MAIN7, from_step=1), 'DA_Lines_HobMain7Depot'),
        perch(spots['keeperPost'], condition(during=MAIN7), 'DA_Lines_HobMain7Lean'),
        # After each, where the last left him.
        perch(spots['hearse'], condition(after=[MAIN7])),
        perch(spots['deckBier'], condition(after=[MAIN6])),
        perch(spots['roadTree'], condition(after=[MAIN5])),
        perch(spots['lantern'], condition(after=[MAIN4])),
        perch(spots['sign'], condition(after=[MAIN3])),
        perch(spots['rail'], condition(after=[MAIN2])),
        perch(board, condition(after=[MAIN1])),
    ]
    hob.set_editor_property('perches', [each for each in perches if each is not None])
    hob.get_editor_property('speaker_point').set_editor_property('topics', topics(
        build,
        (condition(during=MAIN7), 'DA_Lines_HobMain7'),
        (condition(during=MAIN6), 'DA_Lines_HobMain6'),
        (condition(during=MAIN5), 'DA_Lines_HobMain5'),
        (condition(during=MAIN4), 'DA_Lines_HobMain4'),
        (condition(during=MAIN3), 'DA_Lines_HobMain3'),
        (condition(during=MAIN2), 'DA_Lines_HobMain2'),
        (condition(during=MAIN1), 'DA_Lines_HobMain1'),
        (condition(after=[MAIN1]), 'DA_Lines_HobAfterMain1'),
    ))
    missing = [name for name, spot in spots.items() if spot is None]
    build.log(f'Hob on Ellis\'s headboard at ({x:.0f}, {y:.0f}, {z:.0f}), {len([p for p in perches if p is not None])} perches'
              f'{" (without " + ", ".join(missing) + ")" if missing else ""}')


def place(build):
    """Everything above, in the Gameplay folder (build_area.py's gameplay() calls this last). Only the Ransom farm has
    it: the level whose layout has Ellis's grave (the tutorial island has a farmhouse too, and no Delia behind its door)."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    place_cold_open(build)
    grave_ellis = place_family_plot(build)
    place_headboards(build, grave_ellis)
    # Delia's door and her house's pieces (their own module, reloaded as the chapel's is).
    importlib.reload(importlib.import_module('build_area_farm')).place(build)
    # Main 2.
    lookout = placed(build, 'Lookout')
    seat = socket(lookout, 'Sit') if lookout else None
    place_sexton(build, lookout, seat)
    nest = place_bluff_top(build)
    # Main 3.
    place_town_gate(build)
    shop = placed(build, 'FalseFront_Undertaker')
    place_tilly(build, shop)
    place_shutters(build)
    place_safe_zones(build)
    # Main 4, at the chapel (its own module, reloaded as the editor keeps modules between runs, so an edited one takes
    # effect); it hands back Hob's chapel perches.
    chapel_spots = importlib.reload(importlib.import_module('build_area_chapel')).place(build)
    # Main 5, in the Sink (its own module, reloaded as the chapel's is); it hands back Hob's Sink perches.
    sink_spots = importlib.reload(importlib.import_module('build_area_sink')).place(build)
    # Side 3: the Gravemother's lair and the den's place (its own module, reloaded as the chapel's is).
    importlib.reload(importlib.import_module('build_area_den')).place(build)
    # Step 26: Ruth's Ranger caches and the gang's Strongbox (their own module, reloaded as the chapel's is).
    importlib.reload(importlib.import_module('build_area_caches')).place(build)
    # Side 2: Amos, his bales and his hired hands (its own module, reloaded as the chapel's is). Side
    # missions don't move Hob, so it hands back no perches.
    importlib.reload(importlib.import_module('build_area_whitlock')).place(build)
    # Main 6, on the burial deck (its own module, reloaded as the chapel's is); it hands back Hob's Main 6 perches.
    deck_spots = importlib.reload(importlib.import_module('build_area_deck')).place(build)
    # Main 7: the train, the depot's place, Delia's hand-off, the lantern's flame (after the deck: its lantern must stand).
    depot_spots = importlib.reload(importlib.import_module('build_area_depot')).place(build)
    place_hob(build, grave_ellis, lookout, seat, nest, shop, chapel_spots, sink_spots, deck_spots, depot_spots)
