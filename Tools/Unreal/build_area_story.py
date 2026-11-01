"""The story's actors for build_area.py (Docs/Areas/RansomsRest.md: Main 1 "Landfall" since the Wardens story of
2026-10-31, Main 2 "Shall We Talk Business?", Main 3 "Cold Welcome", Main 4 "Hallowed Ground", Main 5 "The Keeper's
Lantern", Main 6 "The Gravewind"), placed from the models build_area.py placed and their sockets (Art/Models/Props/Graves.py,
Buildings/Lookout.py, Buildings/Farmhouse.py, Buildings/FalseFronts.py, Buildings/Chapel.py, Rocks/Outcrops.py's Den Rock)
and from the layout's zones, obstacles, features and roads, so they follow the level whenever it's rebuilt. Only a level
whose layout has the Grave_Ellis placement (Ransom's Rest: kept in layout.json as the area's mark, though the grave itself
is gone from the level since 2026-10-31) gets any of it, and each piece waits for what it's placed from (no lookout: no
cold open, no Sexton). Everything goes in the area's Gameplay folder, so a "gameplay" build places it all again.

Main 1 (Landfall: the Lamplighter, the ledge, the spyglass, the barge, Abel's trail, the porch fight: build_area_landfall.py):
- The old cold open's set (AColdOpenSet) where the lookout stands, facing as it faces (no new game plays it now; kept for
  Looter.Scene.ColdOpen and old saves): its marks on Ransom's Point are the lookout's (Scenes/ColdOpenSet.h).
- Hob's roosts (2026-10-13, wave 3: they replaced the respawn graves as the one save, respawn and fast-travel point):
  build_area_roosts.py places every one from the layout's gameplay.roosts, the farm's (FamilyPlot) outside the family
  plot's east gate first.
- The family plot without Ellis's grave (the user, 2026-10-31: no grave wake; the hero is Sully Graves, a Warden, no
  Ransom): build_area.py still stands SM_Grave_Ellis at the layout's graveEllis in a full build, and the story's pass
  takes it out again. Abel's grave stays, his headboard to read ("Read", from 2.5 m) at its Interact socket, an
  ASpeakerPoint tagged Headboard_Abel.
- Delia Ransom on her porch (ADeliaRansom tagged Speaker_Delia, lane CREW), her house's screen door and plate, and the
  porch barricade (build_area_farm.py).
- Delia's salt line (the farm's safe zone) only after Main 1: Landfall's porch fight is inside it.
Main 2 (The Ferryman's Price: build_area_main2.py places Sexton's ferry and ladder, the Three Widows' spider nest, the
  boards' crew and hose cart, the gate's loudspeaker; it replaced the Sexton-on-the-rail and bluff nest pieces):
- Ransom's Point's middle (a target point tagged Place_RansomsPoint, at the middle of the layout's zone ransomsPoint): where
  the climb ends (Landfall climbs to it).
Main 3 (Under New Management: build_area_main3.py places the checkpoint and its fight, the loudspeakers, banner, chalk, the
  Gilt Spur's cart and Ned's skiff; it replaced the gate-fight bullet that stood here):
- The town gate (a target point tagged Place_TownGate) between its posts (the obstacles townGateNorth and townGateSouth).
  Main Street's set piece follows the checkpoint (layout gameplay.encounters.fights MainStreet).
- Tilly's window (ASpeakerPoint tagged Speaker_Tilly) on Bright & Daughter's SOCKET_Speaker: the shop closed until Main 3's
  last step, then her lines, then a word after.
- The shutters (AWindowShutter) on every SOCKET_Shutter_<L|R><n> of Pruitt's store (FalseFronts.py: eight), open until the
  player comes near, shut from the start after Main 3.
- The safe zones (ASafeGround) from the layout's zones: Delia's salt line round the farm (zone farm, always) and Main
  Street (zone mainStreet, after Main 3).
Main 4 (build_area_chapel.py, with these helpers): the chapel's place, the chapel yard's fight, the bell, the smashed
  Reliquary, Father Aldana's vestry door, and the Unpaid on boot hill and the north road after Main 4.
Main 5 (build_area_sink.py, with these helpers): the Sink's floor dressed (collapsed blocks and coffins, web cards),
  its three egg sacs (shot down in Main 5's second step, two spiders each), the Keeper's Lantern in the webbing (taken
  in its third), the floor's and the ramp head's places, the floor's spiders, the den's web funnel and the Webwood's
  trees.
Main 6 (build_area_deck.py, with these helpers): the burial deck's biers and keeper's lantern posts, Abel on the boards
  (during Main 6), Pa's board after it, the fog wall across the Keeper's Gate, Gravewind Point's place, the story's dusk, and
  the Gravewind's wisps and canyon fog (seen only at dusk).
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
  Main 1: away while the Lamplighter sails, is boarded and lands (steps 0-3: he rides her rigging, FLIGHT's); on the
          anchor winch on the ledge (the anchor and the dead), on the lookout's rail (the climb, then the spyglass), on
          Abel's headboard (the trail, which runs past it; silent), on the porch beam's end (the porch fight and the
          turn-in); after it on the porch beam still (Main 2's offer is his). Steps, spots and arrival lines:
          story_texts.LANDFALL_PERCHES (STORY's), on the spots build_area_landfall measures (winch, porch beam).
  Mains 2-3: story_texts.MAIN23_PERCHES and MAIN23_AFTER_PERCHES (the middle Widow's top, the ferry ladder's crossbar, the
          town gate's north post, Bright & Daughter's sign), each on the first of its spots the build has.
  Main 4: on the chapel's door hood (SOCKET_Perch_Hood) for the way up, the yard's fight, the bell and the Reliquary (a
          word at each, no flight between them), then on the vestry lantern's bracket (SOCKET_Perch_Lantern) for Aldana,
          and there after Main 4.
  Main 5: on Den Rock over the den for the way down ("Keepers walk the dead to the boards by lantern light..."), on
          a block on the Sink's floor for the egg sacs and then the lantern (a word at each, no flight), on the Sink
          road's dead tree by the ramp head once the lantern is taken, and there after Main 5.
  Main 6: on the keeper's grave's board for the way to Gravewind Point, on the Keeper's Gate's north rock for the lantern
          and the fight (a word at each, no flight between them), on the bier beside Pa's for the scene, and there after
          Main 6 (landing again as it ends: the second light, "Don't look at that one, sunshine.").
  Main 7: on the keeper's post by the leaning lantern as it begins ("That's Purcell. The Lily's moored out that way."),
          on Tilly's hearse car's roof once Delia has handed Heirloom out, and the station board (a word at each), and
          there after Main 7.
  Talked to, a line for where things stand. Passed close, from Main 1's end, his three feathers (story_texts.MUTTERS).
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
PLACE_POINT = 'Place_RansomsPoint'
PLACE_GATE = 'Place_TownGate'
NEST = 'BluffNest'
NEST_TAG = 'Spider_BluffNest'
GATE = 'TownGate'
GATE_TAG = 'Unpaid_TownGate'

# Hob's grip on top of a headboard, from its Interact socket (55 cm up its face): up to the board's top, and back over
# its thickness and lean (cm).
PERCH_UP = 30.0
PERCH_BACK = 12.0
# Reading a headboard is done close to (cm).
READ_REACH = 250.0
# (The headboard carved for each hero on Ellis's grave, SM_Headboard_<Hero>, went with the grave on 2026-10-31; the
# meshes stay imported, unused.)
# A conversation through a window carries as far as one through a door (USpeakerPointComponent's default, cm).
TALK_REACH = 400.0

# The places' markers stand about where the player's middle is, so a place measured with its height (the bluff top's)
# counts the player standing there (cm).
MARKER_UP = 90.0

# The spider nest (Main 2): it comes as the player climbs the bluff path, under the top's edge and out of sight of it (cm).
NEST_ACTIVATION = 3000.0
NEST_SPREAD = 900.0
# The gate's fight (Main 3): the Unpaid stand north-east of the gate, in the street's mouth and the field north of it,
# and on the street's south side, so they come two ways at once, 8-20 m out (cm, from the gate, X north and Y east); they
# give up 30 m from the gate. Six and a Restless one since the user's bigger fights (2026-10-21; three until then).
GATE_SPOTS = ((600.0, 900.0), (1100.0, 400.0), (300.0, 1500.0), (1300.0, 1300.0), (-400.0, 700.0), (-1300.0, 300.0),
              (-200.0, 2000.0))
GATE_BASIC = 6
# The user's call (2026-10-23, "Soften early only": fewer or weaker creatures in the first fights, later ones unchanged):
# seven out at once killed a player standing still. At most four now; the rest rise as those fall, the Restless last.
GATE_MAX_ALIVE = 4
# The spider nest (Main 2). The 2026-10-23 play-check: six out at once (five and a Restless one, three biting at a time)
# killed a level-2 player strafing with a Common rifle and ward, seven bites of 20-27 in about 3 s; the user's call that
# morning, "Soften early only". Now four and a Restless one, never more than two out (spiders outrun a player walking
# backwards, so two at a time is what a moving player can hold off), in two pushes: two from the nest, then, once they're
# down and the player has had a breath (NEST_BREATH, the ward coming back), two more and the Restless one as the first
# falls. Each lighter than its class (health shares, as spiderlings' 0.2 is): kills come quicker, bites as they were.
NEST_PUSHES = ((2, 'BASIC', 1), (2, 'BASIC', 2), (1, 'RARE', 2))
NEST_MAX_ALIVE = 2
NEST_BREATH = 3.0
NEST_HEALTH = {'BASIC': 0.7, 'RARE': 0.6}
NEST_BASIC = sum(count for count, rank, _ in NEST_PUSHES if rank == 'BASIC')
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


def topic(when, line_set, sets_flag=''):
    """FSpeakerTopic: when, its line set, and the story flag it sets when said (FSpeakerTopic::SetsFlag; none: none)."""
    made = unreal.SpeakerTopic()
    made.set_editor_property('when', when)
    if line_set:
        made.set_editor_property('line_set', line_set)
    if sets_flag:
        # Guarded: a game module built before SetsFlag still takes the topic, only without its flag (then a talk meant to
        # be given once is given every time until the module is built and this runs again).
        try:
            made.set_editor_property('sets_flag', unreal.Name(sets_flag))
        except Exception as error:
            unreal.log_warning(f'no SpeakerTopic.sets_flag ({error}): the topic sets no {sets_flag} (build the game module)')
    return made


def topics(build, *pairs):
    """Speaker topics from (condition, line set name) pairs, or (condition, line set name, flag it sets) triples, in order
    (the first that holds is said), leaving out any whose set isn't made yet: its point then says what comes next for that
    part of the story."""
    made = []
    for pair in pairs:
        when, name = pair[0], pair[1]
        line_set = lines(build, name)
        if line_set:
            made.append(topic(when, line_set, pair[2] if len(pair) > 2 else ''))
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


def group(cls, count, rank, first_wave=1, last_wave=0, health_scale=1.0, leader=False):
    """FEncounterGroup: count creatures of cls, all of one rank ('BASIC', 'RARE'), in each wave from first_wave to
    last_wave (counted from 1; 0: every wave from the first), their health health_scale of their class's; leader: the
    first of them leads its pack (it wears Rallying and its fall makes the pack falter: FIGHTS lane 6, 2026-10-28)."""
    made = unreal.EncounterGroup()
    made.set_editor_property('creature_class', cls)
    made.set_editor_property('count', count)
    made.set_editor_property('rank_roll', unreal.EncounterRankRoll.FIXED)
    made.set_editor_property('rank', getattr(unreal.CreatureRank, rank))
    made.set_editor_property('first_wave', first_wave)
    made.set_editor_property('last_wave', last_wave)
    made.set_editor_property('health_scale', health_scale)
    if leader:
        made.set_editor_property('leader', True)
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
    """The family plot without Ellis's grave (the Wardens story, 2026-10-31: nobody wakes in a grave): build_area.py's
    full build stands SM_Grave_Ellis at the layout's graveEllis (the placement stays in layout.json, where every area
    script's "is this Ransom's Rest" check reads it), and this takes it out of the level again. Returns Abel's grave (his
    headboard and Hob's perches stand on it), or None. Its respawn grave went long ago: Hob's roost by the family plot's
    gate replaced it (build_area_roosts.py, gameplay.roosts FamilyPlot)."""
    keys = [key for key, spot in build.layout['placements'].items() if spot['kind'] == 'Grave_Ellis']
    gone = [actor for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
            if str(actor.get_actor_label()) in keys and unreal.Name(build.tag) in actor.tags]
    for actor in gone:
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(actor)
    if gone:
        build.log(f'Ellis\'s grave taken out of the family plot ({len(gone)} actor(s)): Abel\'s stays')
    return placed(build, 'Grave_Abel')


def place_headboards(build, grave_abel):
    """Abel's headboard to read, at his grave's Interact socket: a speaker point (Headboard_Abel: his carving and Hob's
    frost said as captions). (Ellis's own readable, HeadboardEllis, went with his grave on 2026-10-31.)"""
    point_cls = actor_class('SpeakerPoint')
    face = socket(grave_abel, 'Interact') if grave_abel else None
    if face is None or point_cls is None:
        if grave_abel is not None and face is None:
            build.warn('Grave_Abel has no Interact socket: his headboard can\'t be read')
        return
    at = face.translation
    speaker(build, point_cls, (at.x, at.y, at.z), face.rotation.rotator().yaw, 'Headboard_Abel', 'Headboard_Abel',
            prompt='Read', reach=READ_REACH, line_set=lines(build, 'DA_Lines_HeadboardAbel'))
    build.log('Abel\'s headboard, to read')


# ---------------------------------------------------------------------------
# Main 2: Ransom's Point
# ---------------------------------------------------------------------------

def place_bluff_top(build):
    """Ransom's Point's middle, where the climb ends (Landfall climbs to its marker). The spider nest and Sexton moved to
    build_area_main2.py (the Three Widows, the Ferryman's landing). Returns None: there is no nest here any more."""
    top = layout_entry(build, 'zones', 'ransomsPoint')
    if top is None:
        return None
    cx, cy = centroid(top['polygon'])
    marker(build, cx, cy, PLACE_POINT)
    build.log(f'Ransom\'s Point\'s middle at ({cx:.0f}, {cy:.0f}), tagged {PLACE_POINT}')
    return None


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

    # The gate's fight (the checkpoint and its men) is build_area_main3.py's now.
    return gx, gy, gz


def texts_topics(build, tag):
    """A speaker's topics from the missions' data (Tools/Unreal/story_texts.py TOPICS[tag]), in its order, each with the
    flag it sets when its condition dict carries sets_flag."""
    import story_texts
    texts = importlib.reload(story_texts)
    return topics(build, *[(texts.condition(when), name, (when or {}).get('sets_flag', ''))
                           for when, name in texts.TOPICS[tag]['topics']])


def place_tilly(build, shop):
    """Tilly's window: her speaker point on Bright & Daughter's Speaker socket."""
    cls = actor_class('SpeakerPoint')
    window = socket(shop, 'Speaker') if shop else None
    if cls is None or window is None:
        if shop is not None and cls is not None:
            build.warn('Bright & Daughter has no Speaker socket: Tilly can\'t be talked to')
        return
    at = window.translation
    # Closed until Main 3's last step (from 0: 3); her lines then, Knocking Below's, a word after (story_texts.py TOPICS).
    speaker(build, cls, (at.x, at.y, at.z), window.rotation.rotator().yaw, 'Speaker_Tilly', 'Speaker_Tilly',
            name='Tilly Bright', reach=TALK_REACH, line_set=lines(build, 'DA_Lines_TillyClosed'),
            topics=texts_topics(build, 'Speaker_Tilly'))
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
    """Delia's salt line round the farm (after Main 1: Landfall's porch fight is fought inside it, and a safe zone keeps
    every creature off its ground) and Main Street (after Main 3), from the layout's zones."""
    cls = actor_class('SafeGround')
    if cls is None:
        build.warn('no SafeGround class (build the game module first): no safe zones')
        return
    for zone_id, layout_id, after in (('Farm', 'farm', (MAIN1,)), ('MainStreet', 'mainStreet', (MAIN3,))):
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

def hob_on_board(grave):
    """On a grave's headboard (Abel's, the keeper's): over its Interact socket, back over its lean. (x, y, z, yaw) or
    None."""
    face = socket(grave, 'Interact') if grave else None
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


def place_hob(build, grave_abel, lookout, seat, nest, shop, chapel_spots, sink_spots, deck_spots, depot_spots,
              landfall_spots=None, main2_spots=None):
    cls = actor_class('HobBird')
    board = hob_on_board(grave_abel)
    if cls is None or board is None:
        if cls is not None:
            build.warn('no Abel\'s grave with its Interact socket: no Hob')
        return
    landfall_spots = landfall_spots or {}
    steps = landfall_spots.get('steps', {})
    import story_texts
    texts = importlib.reload(story_texts)
    # The mountain's mouth (build_area_providence.hob_spots: the Providence's tag board's roof).
    mountain = importlib.reload(importlib.import_module('build_area_providence')).hob_spots(build) \
        if importlib.import_module('build_area_mountain').is_mountain(build) else {}
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
        # Back Pay's (story_texts.py PERCHES): the Providence's mouth.
        'mineMouth': mountain.get('mineMouth'),
        # Landfall's (story_texts.py LANDFALL_PERCHES names them): the porch beam's end over Delia's steps and the anchor
        # winch's top on the Mooring Ledge, both measured by build_area_landfall.place; Abel's headboard, which his trail
        # runs past (Hob's first spot too).
        'porchRail': landfall_spots.get('porch'),
        'mooringLedge': landfall_spots.get('winch'),
        'abelBoard': board,
        # Mains 2-3's (story_texts.MAIN23_PERCHES names them): the middle Widow's top and the ferry ladder's crossbar,
        # measured by build_area_main2.place.
        'widows': (main2_spots or {}).get('widows'),
        'ladderTop': (main2_spots or {}).get('ladderTop'),
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
    # Landfall's (Main 1) perches, latest step first (story_texts.LANDFALL_PERCHES: STORY's words and steps, each on the
    # first of its spots the build has): the porch beam for the fight and the turn-in, Abel's headboard for the trail, the
    # lookout's rail for the spyglass and the climb (a word at each, no flight), the winch for the anchor and the dead.
    # Before the anchor (the ship sailing, boarded, landing) none holds: he's away, riding her rigging (FLIGHT's).
    main1 = []
    for names, when, arrival in texts.LANDFALL_PERCHES:
        spot = next((spots[name] for name in names if spots.get(name)), None)
        if spot is None:
            build.warn(f'Hob: no spot for Landfall\'s perch {when} (tried {", ".join(names)}): left out')
        main1.append(perch(spot, texts.condition(when), arrival))
    # Main 1's data and these perches must count the same steps (build_area_landfall read the data's).
    said_steps = sorted({when.get('from_step', 0) for _, when, _ in texts.LANDFALL_PERCHES})
    data_steps = sorted(set(steps.values()))
    if data_steps and said_steps != data_steps:
        build.warn(f'Hob: story_texts.LANDFALL_PERCHES\' steps {said_steps} aren\'t Main 1\'s data\'s {data_steps} (anchor, '
                   f'point, spyglass, trail, porch): fix the perches')
    # The first that holds is his, so a mission's later steps come before its earlier ones, and each mission's perches
    # before the ones after the missions before it (steps counted from 0).
    # Where only the objective's arrow had words (the way up, the bell, the knock, the sacs, the post, the depot), he flies
    # there and says nothing: the user's cut of the story's talk (2026-10-21).
    perches = main1 + [
        # Mains 2 and 3 (story_texts.MAIN23_PERCHES, the Wardens story's: each on the first of its spots the build has).
        *[perch(next((spots[name] for name in names if spots.get(name)), None), texts.condition(when), arrival)
          for names, when, arrival in texts.MAIN23_PERCHES],
        # Main 4: on the door hood for the Reliquary and the bell; on the vestry lantern's bracket for the knock at
        # Aldana's door (step 2); on the door hood again for the yard's fight and the way up.
        perch(spots['hood'], condition(during=MAIN4, from_step=4), 'DA_Lines_HobMain4Reliquary'),
        perch(spots['hood'], condition(during=MAIN4, from_step=3)),
        perch(spots['lantern'], condition(during=MAIN4, from_step=2)),
        perch(spots['hood'], condition(during=MAIN4, from_step=1), 'DA_Lines_HobMain4Yard'),
        perch(spots['hood'], condition(during=MAIN4)),
        # Main 5: on the Sink road's tree by the ramp head once the lantern is taken; on the floor's block for the
        # lantern and the sacs (no flight between them: a word at each); on Den Rock over the den for the way down.
        perch(spots['roadTree'], condition(during=MAIN5, from_step=3), 'DA_Lines_HobMain5Out'),
        perch(spots['block'], condition(during=MAIN5, from_step=2)),
        perch(spots['block'], condition(during=MAIN5, from_step=1)),
        perch(spots['denRock'], condition(during=MAIN5), 'DA_Lines_HobMain5Way'),
        # Main 6: on the bier beside Pa's for the scene (his word is the scene's own); on the gate's north rock for the fight
        # and the lantern (no flight between them: a word at each); on the keeper's grave's board for the way there.
        perch(spots['deckBier'], condition(during=MAIN6, from_step=3)),
        perch(spots['gateRock'], condition(during=MAIN6, from_step=2), 'DA_Lines_HobMain6Fight'),
        perch(spots['gateRock'], condition(during=MAIN6, from_step=1)),
        # (story_texts.PERCHES' keepersBridge isn't one: it holds when this does, so it would take him off the main way.)
        perch(spots['keeperGrave'], condition(during=MAIN6), 'DA_Lines_HobMain6Way'),
        # Main 7: on the hearse car's roof for the depot and the board (a word at each); on the keeper's post by the lantern.
        perch(spots['hearse'], condition(during=MAIN7, from_step=2)),
        perch(spots['hearse'], condition(during=MAIN7, from_step=1)),
        perch(spots['keeperPost'], condition(during=MAIN7), 'DA_Lines_HobMain7Lean'),
        # Back Pay's first step, while no main mission holds him: on the Providence's tag board.
        perch(spots['mineMouth'], texts.condition(texts.PERCHES['mineMouth'][0]), texts.PERCHES['mineMouth'][1]),
        # After each, where the last left him. After Main 6 he lands again on the bier beside Pa's (the scene's spot, no
        # flight) with the road at dusk and the second light (story_texts.PERCHES gravewindPoint), until Main 7 is taken.
        perch(spots['hearse'], condition(after=[MAIN7])),
        perch(spots['deckBier'], texts.condition(texts.PERCHES['gravewindPoint'][0]), texts.PERCHES['gravewindPoint'][1]),
        perch(spots['roadTree'], condition(after=[MAIN5])),
        perch(spots['lantern'], condition(after=[MAIN4])),
        # After Mains 3 and 2 (story_texts.MAIN23_AFTER_PERCHES): on the undertaker's sign; on the ladder by the Ferryman.
        *[perch(next((spots[name] for name in names if spots.get(name)), None), texts.condition(when), arrival)
          for names, when, arrival in texts.MAIN23_AFTER_PERCHES],
        # After Landfall on the porch rail, where it was turned in: Main 2's offer is his.
        perch(spots['porchRail'] or spots['path'] or board, condition(after=[MAIN1])),
    ]
    hob.set_editor_property('perches', [each for each in perches if each is not None])
    # His talk: the missions' data's (story_texts.py TOPICS), the side missions after the main ones.
    talk = hob.get_editor_property('speaker_point')
    talk.set_editor_property('topics', texts_topics(build, 'Speaker_Hob'))
    # His mutters (story_texts.py MUTTERS: Hob's feathers, one line a pass as the player comes near, in order).
    mutters = texts.MUTTERS.get('Speaker_Hob', [])
    if mutters:
        talk.set_editor_property('mutters', topics(build, *[(texts.condition(when), name) for when, name in mutters]))
    missing = [name for name, spot in spots.items() if spot is None]
    build.log(f'Hob placed on Abel\'s headboard at ({x:.0f}, {y:.0f}, {z:.0f}), {len([p for p in perches if p is not None])} '
              f'perches{" (without " + ", ".join(missing) + ")" if missing else ""}')


def place(build):
    """Everything above, in the Gameplay folder (build_area.py's gameplay() calls this last). Only the Ransom farm has
    it: the level whose layout has Ellis's grave (the tutorial island has a farmhouse too, and no Delia behind its door)."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    place_cold_open(build)
    grave_abel = place_family_plot(build)
    place_headboards(build, grave_abel)
    # Delia on her porch, her house's pieces and the porch barricade (their own module, reloaded as the chapel's is).
    importlib.reload(importlib.import_module('build_area_farm')).place(build)
    # Main 2.
    lookout = placed(build, 'Lookout')
    seat = socket(lookout, 'Sit') if lookout else None
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
    # Dead Lift and The Last Watch: the ore lift, its bell pulls and speaking tube, Calder's watch fire and the far light.
    importlib.reload(importlib.import_module('build_area_lift')).place(build)
    # Main 1, Landfall (its own module, reloaded as the chapel's is): the Lamplighter, the ledge's winch and climbers, the
    # spyglass, Vane's barge and the smog, Abel's trail and hat, the porch fight, the loudspeakers; after the deck (the
    # Smog rule joins its StoryLighting); it hands back Hob's Landfall perches.
    landfall_spots = importlib.reload(importlib.import_module('build_area_landfall')).place(build)
    # Main 2, The Ferryman's Price (its own module, reloaded as the chapel's is; after the deck and Landfall): the Widows'
    # nest, the boards' cart, crew and dead, the gate's loudspeaker, the Ferryman's landing, ferry and ladder; Hob's spots.
    main2_spots = importlib.reload(importlib.import_module('build_area_main2')).place(build)
    # Main 3, Under New Management (its own module): the checkpoint and its fight, the loudspeakers, the banner, the chalk,
    # the Gilt Spur's cart, Lucky Ned's skiff.
    importlib.reload(importlib.import_module('build_area_main3')).place(build)
    place_hob(build, grave_abel, lookout, seat, nest, shop, chapel_spots, sink_spots, deck_spots, depot_spots,
              landfall_spots, main2_spots)
    # Hob's roosts, last, so each keeps clear of everything the story stands on the ground (its own module, reloaded as the
    # chapel's is; it places nothing for a layout without gameplay.roosts).
    importlib.reload(importlib.import_module('build_area_roosts')).place(build)
