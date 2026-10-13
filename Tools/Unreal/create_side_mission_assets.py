"""Makes the side missions' data assets (UMissionDefinition, Missions/MissionDefinition.h) in /Game/Data/Missions and
saves them, as create_mission_assets.py does for the others (kept apart, so the two can change side by side). Run it in
the open editor once the C++ with AWantedPoster and UMissionLastingInteractObjective is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_side_mission_assets.py"
It prints a SIDEMISSIONS line per mission and step, and "SIDEMISSIONS done" at the end. Running it again rebuilds these
missions (fields, steps, turn-in and rewards) from what's written here; other mission assets are left alone.

Side missions are turned in as the main ones are (create_mission_assets.py's docstring): ready once their objectives
are done, finished by a talk to their giver. Their experience is a fixed amount each (Docs/Progression.md).

  DA_Mission_Side1  "Wanted: Already Dead" (Docs/Areas/RansomsRest.md, Side 1), on Ransom's Rest once Main 3 is
                    turned in, starting by itself. Tear down 6 of Ellis's wanted posters around town and the farms (each a
                    held Interact on an AWantedPoster tagged WantedPoster), counted from the world
                    (UMissionLastingInteractObjective): posters torn down before it opened count, and so do the ones torn
                    before a reload, since a side mission starts over from its first step. Seven hang, so one can be
                    missed. Then read Ranger Calder's note on the Rim Rangers' board (a tap on the one tagged CalderNote).
                    Turned in to Tilly at her window, Main Street's one voice (a line of its own). Reward: 35 XP and a
                    guaranteed Rare gun.
  DA_Mission_Side2  "Unfinished Business" (Side 2), on Ransom's Rest once Main 4 is turned in, starting by itself. Talk
                    to Amos at his fence (AAmosWhitlock, tagged Speaker_Amos); load his 6 hay bales into his barn (each a
                    held Interact on an AHayBale tagged HayBale, counted from the world as Side 1's posters are: the session
                    keeps the loaded ones with the map, so a reload loses none); drive off his old hired hands (clear the
                    encounter WhitlockHands: five Unpaid and a Restless one in his barn yard); turned in to Amos (the
                    talk that was its fourth step: his thanks, from build_area_whitlock.py's THANKS_STEP, 3). All placed
                    by build_area_whitlock.py. Reward: 40 XP and a guaranteed Epic gun ("the one Amos was buried with and
                    never needed"). Made once Main 4 exists.
  DA_Mission_Side3  "The Gravemother" (Side 3), on Ransom's Rest once Main 5 is turned in, starting by itself. "Find
                    where the Gravemother hungers": done on entering the Sink, not the den (a reach of the place tagged
                    Place_SinkFloor, the floor's middle that build_area_sink.py places for Main 5, within 19.5 m measured
                    with its height: the whole floor, and the ramp from about 2 m below the rim down; the rim and the
                    ramp's head stay outside it, and the den needn't be entered). Its arrow points at the ramp head (the
                    marker tagged Place_SinkRim). Then kill the Gravemother (clear her lair's encounter, Gravemother: a
                    kill before the step began counts too, and so does one on an earlier visit while she's still away).
                    Automatic: a monster hunt nobody asked for. Reward: 40 XP, once; her Legendary loot table drops on
                    every kill, and she comes back on an arrival 20 minutes of play after her death. Her lair is made by
                    build_area_den.py. Made once Main 5 exists (step 19's main mission): until then the script says it
                    waits.

Objectives are instanced objects inside the asset, one class per kind, made with unreal.new_object(<class>, asset) and
listed in each step's 'objectives'. Classes for actor filters are loaded by their script path
('/Script/AI_Looter_Shooter.WantedPoster').

The mission features (wave 2a) are here as in create_mission_assets.py (which runs on import, so its helpers are copied):
a spec's 'offer' (offer(): a mission picked up from its giver, start 'OFFERED') and 'start_event'; optional objectives
(objective(..., optional=True), event_objective, read_objective); 'items' (item()) counted by collect_items(); a turn-in
with two choice()s (turn_in(choices=..., question=...)); rewards(silver=..., optional_silver=...). A spec's 'expect' names
each step's objective class, or a list of them in order for a step of several (one with optional objectives).
"""
import unreal

FOLDER = '/Game/Data/Missions'
CLASSES = '/Script/AI_Looter_Shooter.'
# AWantedPoster's tags (World/WantedPoster.h).
WANTED_TAG = 'WantedPoster'
NOTE_TAG = 'CalderNote'
# Side 2's pieces (Story/AmosWhitlock.h: SpeakerTag; World/HayBale.h: BaleTag; build_area_whitlock.py: the hands' encounter).
AMOS_TAG = 'Speaker_Amos'
BALE_TAG = 'HayBale'
BALES = 6
HANDS = 'WhitlockHands'
HANDS_COUNT = 6
# The Sink's markers (Tools/Unreal/build_area_sink.py: the floor's middle, 90 cm over the floor 12 m under the rim, and
# the ramp head on the west rim) and the Gravemother's lair (Creatures/GravemotherCreature.h: Gravemother::LairId; placed
# by Tools/Unreal/build_area_den.py).
SINK_TAG = 'Place_SinkFloor'
SINK_RIM_TAG = 'Place_SinkRim'
# Entering the Sink, measured with height from the floor's middle (layout.json: the floor is a 12-gon of 18 m, the rim is
# 17.5 m or more out and 12 m up, so 21.2 m or more away): 19.5 m covers the whole floor, leaves the rim 1.7 m outside,
# and takes in the sinkRamp (layout_computed.json) from its 5th point, about 2 m under the rim.
SINK_REACH = 1950.0
GRAVEMOTHER_LAIR = 'Gravemother'


def mission_types():
    """The reflected mission types, or a clear error when the C++ isn't built yet."""
    names = ['MissionDefinition', 'MissionStep', 'MissionRewards', 'MissionTurnIn', 'StoryLine', 'MissionActorFilter',
             'MissionKind', 'MissionStart', 'MissionInteractObjective', 'MissionLastingInteractObjective', 'WantedPoster',
             'WeaponRarity', 'MissionReachObjective', 'MissionClearObjective', 'MissionPlace', 'MissionTalkObjective',
             'MissionWaypoint', 'MissionOffer', 'MissionEventObjective',
             # The mission features (wave 2a): a turn-in's choice, mission items and their objective.
             'MissionTurnInChoice', 'MissionItemInfo', 'MissionCollectItemObjective']
    missing = [name for name in names if getattr(unreal, name, None) is None]
    if missing:
        raise RuntimeError(f"unreal.{', unreal.'.join(missing)} missing: build the C++ with the wanted posters first")


def actor_class(name):
    cls = unreal.load_class(None, CLASSES + name)
    if cls is None:
        raise RuntimeError(f'{CLASSES}{name} is not a class: nothing was changed')
    return cls


def actor_filter(cls_name=None, tag=None):
    """FMissionActorFilter: actors of a class and/or carrying a tag."""
    actors = unreal.MissionActorFilter()
    if cls_name:
        actors.set_editor_property('actor_class', actor_class(cls_name))
    if tag:
        actors.set_editor_property('actor_tag', unreal.Name(tag))
    return actors


def objective(asset, cls, text, waypoint=None, show_count=True, short='', hint_action='', hint='', optional=False,
              waypoint_class=None, waypoint_tag=None, **settings):
    """One objective, made inside the asset (its outer) so it's saved with it, as create_mission_assets.py makes one:
    text the Missions page's sentence, short the tracker's line (empty: text), hint_action/hint its key hint, optional an
    optional objective (never holds its step; pays the mission's optional_silver at the turn-in; "Optional:" is added by
    the game), waypoint ACTOR with waypoint_class/_tag to point the arrow at an actor."""
    made = unreal.new_object(cls, asset)
    made.set_editor_property('text', unreal.Text(text))
    made.set_editor_property('short_text', unreal.Text(short))
    made.set_editor_property('hint_action', unreal.Name(hint_action))
    made.set_editor_property('hint_text', unreal.Text(hint))
    if waypoint is not None:
        made.set_editor_property('waypoint', waypoint)
    if waypoint_class or waypoint_tag:
        made.set_editor_property('waypoint_actor', actor_filter(waypoint_class, waypoint_tag))
    made.set_editor_property('show_count', show_count)
    made.set_editor_property('optional', optional)
    for name, value in settings.items():
        made.set_editor_property(name, value)
    return made


def step(*objectives):
    made = unreal.MissionStep()
    made.set_editor_property('objectives', list(objectives))
    return made


# ---------------------------------------------------------------------------
# The mission features (wave 2a), as create_mission_assets.py has them (that script runs on import, so they're copied)
# ---------------------------------------------------------------------------

# Events the game's things send: the porch plate's two uses (APorchPlate), reading a readable (AReadableProp::EventFor).
PORCH_TAKEN = 'PorchPlate.Taken'
PORCH_EATEN = 'PorchPlate.Eaten'


def readable_event(readable_id):
    """'Readable.WagesChalk': the event reading the AReadableProp whose readable_id is readable_id sends."""
    return f'Readable.{readable_id}'


def story_lines(lines):
    """A list of FStoryLine from (speaker, words) pairs; an empty speaker is whoever's point says it."""
    said = []
    for who, words in lines:
        line = unreal.StoryLine()
        line.set_editor_property('speaker', unreal.Text(who))
        line.set_editor_property('text', unreal.Text(words))
        said.append(line)
    return said


def event_objective(asset, text, event, count=1, optional=False, **kwargs):
    """An Event objective: done when the named event happens count times."""
    return objective(asset, unreal.MissionEventObjective, text, optional=optional, show_count=count > 1,
                     event=unreal.Name(event), count=count, **kwargs)


def read_objective(asset, text, readable_id, optional=False, **kwargs):
    """Read an AReadableProp (its Readable.<id> event); the arrow on the prop (tagged Readable_<id> by itself)."""
    kwargs.setdefault('waypoint', unreal.MissionWaypoint.ACTOR)
    kwargs.setdefault('waypoint_tag', f'Readable_{readable_id}')
    return event_objective(asset, text, readable_event(readable_id), optional=optional, **kwargs)


def collect_items(asset, text, items, optional=False, **kwargs):
    """Have mission items (UMissionCollectItemObjective): done once every one of items (ids from 'items') is carried."""
    return objective(asset, unreal.MissionCollectItemObjective, text, optional=optional, show_count=len(items) > 1,
                     items=[unreal.Name(each) for each in items], **kwargs)


def item(item_id, name, line='', mesh='', dropped_by_class=None, dropped_by_tag=None):
    """FMissionItemInfo: a thing that belongs to the mission (create_mission_assets.py's item(): its id, never renamed or
    reused; its Carried words; its model; the creature that drops it as it dies)."""
    made = unreal.MissionItemInfo()
    made.set_editor_property('id', unreal.Name(item_id))
    made.set_editor_property('name', unreal.Text(name))
    made.set_editor_property('line', unreal.Text(line))
    if mesh:
        model = unreal.load_asset(mesh) if unreal.EditorAssetLibrary.does_asset_exist(mesh) else None
        if model is None:
            unreal.log_warning(f"SIDEMISSIONS item {item_id}: {mesh} isn't made yet: its pickup shows a stand-in until it is")
        else:
            made.set_editor_property('mesh', model)
    if dropped_by_class or dropped_by_tag:
        made.set_editor_property('dropped_by', actor_filter(dropped_by_class, dropped_by_tag))
    return made


def rewards(experience=0, gun=False, gun_rarity_floor='COMMON', gun_kind='', named_gun='', unlock_areas=(), silver=0,
            optional_silver=10):
    """FMissionRewards: a fixed amount of experience (Docs/Progression.md), a gun at the player's feet (of gun_kind, a
    weapon definition's asset path, else one from the default loot table), a named gun, areas opened, a purse of its own
    (silver; 0: the giver's kept by mission id) and each optional objective's silver (at level 1)."""
    made = unreal.MissionRewards()
    made.set_editor_property('experience', experience)
    made.set_editor_property('experience_share', 0.0)
    made.set_editor_property('silver', silver)
    made.set_editor_property('optional_silver', optional_silver)
    made.set_editor_property('gun', gun)
    kind = None
    if gun_kind:
        kind = unreal.load_asset(gun_kind)
        if kind is None:
            raise RuntimeError(f'{gun_kind} is not an asset: nothing was saved')
    made.set_editor_property('gun_kind', kind)
    made.set_editor_property('gun_rarity_floor', getattr(unreal.WeaponRarity, gun_rarity_floor))
    made.set_editor_property('named_gun', unreal.Name(named_gun))
    made.set_editor_property('unlock_areas', [unreal.Name(area) for area in unlock_areas])
    return made


def choice(title, line, flag='', stamp='', lines=(), **reward_settings):
    """FMissionTurnInChoice: one of a turn-in's two choices (its title, what it means, the story flag it sets, the word
    stamped as it's chosen, what's said then) and rewards()'s fields for what it gives in place of the mission's own."""
    made = unreal.MissionTurnInChoice()
    made.set_editor_property('title', unreal.Text(title))
    made.set_editor_property('line', unreal.Text(line))
    made.set_editor_property('flag', unreal.Name(flag))
    made.set_editor_property('stamp', unreal.Text(stamp))
    made.set_editor_property('lines', story_lines(lines))
    made.set_editor_property('rewards', rewards(**reward_settings))
    return made


def offer(giver_tag='', giver='', lines=(), briefing=''):
    """FMissionOffer, as create_mission_assets.py makes it: who offers a mission picked up from its giver (start='OFFERED')
    by the tag their actor carries (Speaker_Agent, NoticeBoard_CalderPost, Artifact_MinersTag), their name for people,
    what a speaker says as they offer it (lines of (speaker, words)), and the offer card's two to four sentences. Without a
    giver tag the offer is empty, as every mission that isn't offered keeps it."""
    made = unreal.MissionOffer()
    made.set_editor_property('giver_tag', unreal.Name(giver_tag))
    made.set_editor_property('giver_name', unreal.Text(giver))
    made.set_editor_property('lines', story_lines(lines))
    made.set_editor_property('briefing', unreal.Text(briefing))
    return made


def turn_in(automatic=False, speaker='', giver='', lines=(), choices=(), question=''):
    """FMissionTurnIn, as create_mission_assets.py makes it: the giver's speaker tag and name (anyone at a speaker point: a
    door, a coffin, a speaking tube, a tag board), what they say as it's turned in (none: their own lines there), and none
    or two choice()s (or dicts of its fields) with the card's question; automatic: it finishes by itself after its last
    objective."""
    if len(choices) not in (0, 2):
        raise RuntimeError(f'a turn-in takes no choices or two, not {len(choices)}: nothing was changed')
    made = unreal.MissionTurnIn()
    made.set_editor_property('automatic', automatic)
    made.set_editor_property('speaker_tag', unreal.Name(speaker))
    made.set_editor_property('giver_name', unreal.Text(giver))
    made.set_editor_property('lines', story_lines(lines))
    made.set_editor_property('choices', [choice(**each) if isinstance(each, dict) else each for each in choices])
    made.set_editor_property('choice_question', unreal.Text(question))
    return made


def side1_steps(asset):
    """Tear down the posters (any 6 of the 7, held), then read Calder's note. The arrows find their own: the nearest
    poster still up, then the note."""
    return [
        step(objective(asset, unreal.MissionLastingInteractObjective,
                       'Tear down your wanted posters around town and on the farms',
                       target=actor_filter('WantedPoster', WANTED_TAG), count=6, hold=True)),
        step(objective(asset, unreal.MissionInteractObjective,
                       "Read the note on the Rim Rangers' board by the sheriff's office",
                       target=actor_filter('WantedPoster', NOTE_TAG), count=1, hold=False)),
    ]


def side2_steps(asset):
    """Talk to Amos at his fence, load his six bales (held, counted from the world), drive off his hired hands (their
    encounter cleared); then it's turned in to Amos (the talk that was its fourth step). The arrows find their own: Amos,
    the nearest bale still in the field, the nearest of the hands (or their spawner before they're up); then the turn-in's,
    on Amos."""
    return [
        step(objective(asset, unreal.MissionTalkObjective, 'Talk to Amos at his fence', show_count=False,
                       speaker_tag=unreal.Name(AMOS_TAG))),
        step(objective(asset, unreal.MissionLastingInteractObjective, 'Load the hay bales into his barn',
                       target=actor_filter('HayBale', BALE_TAG), count=BALES, hold=True)),
        step(objective(asset, unreal.MissionClearObjective, 'Drive off his old hired hands',
                       spawner_id=unreal.Name(HANDS), count=HANDS_COUNT)),
    ]


def side3_steps(asset):
    """Find where the Gravemother hungers (done on entering the Sink: the floor's place, SINK_REACH measured with its
    height), then kill the Gravemother (her lair's encounter cleared). The first arrow points at the ramp head on the rim
    (the Sink's way in), not the floor's middle; the second finds its own: the Gravemother herself (or her lair while she's
    in her den)."""
    sink = unreal.MissionPlace()
    sink.set_editor_property('actor', actor_filter(tag=SINK_TAG))
    sink.set_editor_property('radius', SINK_REACH)
    sink.set_editor_property('ignore_height', False)
    return [
        step(objective(asset, unreal.MissionReachObjective, 'Find where the Gravemother hungers', show_count=False,
                       place=sink, waypoint=unreal.MissionWaypoint.ACTOR, waypoint_actor=actor_filter(tag=SINK_RIM_TAG))),
        step(objective(asset, unreal.MissionClearObjective, 'Kill the Gravemother', show_count=False,
                       spawner_id=unreal.Name(GRAVEMOTHER_LAIR), count=1)),
    ]


MISSIONS = [
    dict(asset='DA_Mission_Side1', id='Side1', title='Wanted: Already Dead',
         summary='Your wanted poster hangs all over Ransom\'s Rest, and someone has written ALREADY under DEAD OR ALIVE. '
                 'Tear them down.',
         kind='SIDE', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main3'], sort_order=10, steps=side1_steps,
         # Back to Tilly at her window, a short walk up Main Street from the Rangers' board: the reward gun is the box of
         # effects nobody came back for.
         turn_in=dict(speaker='Speaker_Tilly', giver='Tilly', lines=[
             ('', "So it was you, tearing the posters down. Good. They were bad for trade."),
             ('', "A stranger left this with his effects and never came back for it. You'll get more use from it than he "
                  "did."),
         ]),
         rewards=dict(experience=35, gun=True, gun_rarity_floor='RARE'),
         # What setup() checks the stored steps against: each step's objective class.
         expect=['MissionLastingInteractObjective', 'MissionInteractObjective']),
    dict(asset='DA_Mission_Side2', id='Side2', title='Unfinished Business',
         summary=('Amos Whitlock died last harvest with his hay half in. He was still on the Sundown Road when the saint '
                  "went dark, and he drifted home to find it rotting in the field. He isn't angry yet."),
         kind='SIDE', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main4'], sort_order=20, steps=side2_steps,
         turn_in=dict(speaker=AMOS_TAG, giver='Amos'),
         rewards=dict(experience=40, gun=True, gun_rarity_floor='EPIC'),
         expect=['MissionTalkObjective', 'MissionLastingInteractObjective', 'MissionClearObjective']),
    dict(asset='DA_Mission_Side3', id='Side3', title='The Gravemother',
         summary='Something big lives in the Sink\'s den, and it eats what the Unpaid leave behind.',
         kind='SIDE', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main5'], sort_order=30, steps=side3_steps,
         # A hunt nobody asked for: done as she dies. Experience the first time; her loot is her own (the Legendary table
         # on every kill), so no reward gun.
         turn_in=dict(automatic=True),
         rewards=dict(experience=40, gun=False),
         expect=['MissionReachObjective', 'MissionClearObjective']),
]


def editor_property(obj, name):
    """An objective's setting, or None when its kind has none so named (a place to reach has no count)."""
    try:
        return obj.get_editor_property(name)
    except Exception:
        return None


def describe(objective_object):
    """What a stored objective asks, for its SIDEMISSIONS line: its count and targets, its place, its encounter."""
    words = []
    count = editor_property(objective_object, 'count')
    target = editor_property(objective_object, 'target')
    if target is not None:
        words.append(f"{count} of tag {target.get_editor_property('actor_tag')}")
    elif count is not None:
        words.append(f'{count}')
    place = editor_property(objective_object, 'place')
    if place is not None:
        words.append(f"the place tagged {place.get_editor_property('actor').get_editor_property('actor_tag')} within "
                     f"{place.get_editor_property('radius'):.0f} cm"
                     f"{'' if place.get_editor_property('ignore_height') else ', measured with its height'}")
    if editor_property(objective_object, 'waypoint') == unreal.MissionWaypoint.ACTOR:
        words.append(f"arrow on the actor tagged {objective_object.get_editor_property('waypoint_actor').get_editor_property('actor_tag')}")
    spawner = editor_property(objective_object, 'spawner_id')
    if spawner is not None:
        words.append(f'the encounter {spawner} cleared')
    speaker = editor_property(objective_object, 'speaker_tag')
    if speaker is not None:
        words.append(f'the speaker tagged {speaker}')
    if editor_property(objective_object, 'hold'):
        words.append('held')
    return ', '.join(words)


def load_or_create(asset_name, cls):
    path = f'{FOLDER}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}, not a MissionDefinition: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, FOLDER, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def setup(spec):
    asset, created = load_or_create(spec['asset'], unreal.MissionDefinition)
    asset.set_editor_property('id', unreal.Name(spec['id']))
    asset.set_editor_property('title', unreal.Text(spec['title']))
    asset.set_editor_property('summary', unreal.Text(spec['summary']))
    asset.set_editor_property('kind', getattr(unreal.MissionKind, spec['kind']))
    asset.set_editor_property('start', getattr(unreal.MissionStart, spec['start']))
    # What starts an ON_EVENT mission, and who offers an OFFERED one (spec 'offer': offer()'s fields); others have none.
    asset.set_editor_property('start_event', unreal.Name(spec.get('start_event', '')))
    offered = spec.get('offer', {})
    if spec['start'] == 'OFFERED' and not offered.get('giver_tag'):
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: it starts OFFERED but names no giver_tag: nothing was saved")
    asset.set_editor_property('offer', offer(**offered))
    asset.set_editor_property('area', unreal.Name(spec['area']))
    asset.set_editor_property('prerequisites', [unreal.Name(each) for each in spec['prerequisites']])
    asset.set_editor_property('sort_order', spec['sort_order'])
    # Its items first: the steps' collect objectives name them (spec 'items': item()s or dicts of item()'s fields).
    items = [item(**each) if isinstance(each, dict) else each for each in spec.get('items', ())]
    asset.set_editor_property('items', items)
    item_ids = [str(each.get_editor_property('id')) for each in items]
    if len(set(item_ids)) != len(item_ids) or any(not each or each == 'None' for each in item_ids):
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: its items need ids, each its own ({item_ids}): nothing was saved")
    steps = spec['steps'](asset)
    asset.set_editor_property('steps', steps)
    asset.set_editor_property('turn_in', turn_in(**spec['turn_in']))
    asset.set_editor_property('rewards', rewards(**spec['rewards']))

    # Read back what the asset holds before saving: every step with its objectives, of the classes asked for. A step's
    # expectation is one class name for all its objectives, or a list of them in order (a step with optional objectives).
    stored = asset.get_editor_property('steps')
    if len(stored) != len(spec['expect']):
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: {len(stored)} steps where {len(spec['expect'])} are expected: "
                           f"nothing was saved")
    if len(stored) != len(steps):
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: {len(stored)} steps stored of {len(steps)}: nothing was saved")
    for index, (stored_step, expected) in enumerate(zip(stored, spec['expect'])):
        objectives = stored_step.get_editor_property('objectives')
        if not objectives or any(each is None for each in objectives):
            raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1} lost its objectives: nothing was saved")
        wanted = list(expected) if isinstance(expected, (list, tuple)) else [expected] * len(objectives)
        if len(wanted) != len(objectives):
            raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1} holds {len(objectives)} objectives where {len(wanted)} "
                               f"are expected: nothing was saved")
        if all(bool(each.get_editor_property('optional')) for each in objectives):
            raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1} has only optional objectives (it would pass at "
                               f"once): nothing was saved")
        for each, expected_class in zip(objectives, wanted):
            if each.get_outer() != asset:
                raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1}'s {each.get_name()} lives outside the asset: nothing was saved")
            if each.get_class().get_name() != expected_class:
                raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1} holds a {each.get_class().get_name()}, not a "
                                   f"{expected_class}: nothing was saved")
            if isinstance(each, unreal.MissionCollectItemObjective):
                missing = [str(name) for name in each.get_editor_property('items') if str(name) not in item_ids]
                if missing:
                    raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1} collects {', '.join(missing)}, which aren't "
                                       f"its items: nothing was saved")
            unreal.log(f"SIDEMISSIONS {spec['asset']} step {index + 1}: {expected_class}"
                       f"{' (optional)' if each.get_editor_property('optional') else ''} \"{each.get_editor_property('text')}\", "
                       f"{describe(each)}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"{FOLDER}/{spec['asset']} could not be saved")
    reward = spec['rewards']
    finish = ('finishes by itself' if spec['turn_in'].get('automatic')
              else f"turned in to {spec['turn_in'].get('giver')} ({spec['turn_in'].get('speaker')})")
    choices = spec['turn_in'].get('choices', ())
    unreal.log(f"SIDEMISSIONS {spec['asset']} {'made' if created else 'updated'}: id {spec['id']}, \"{spec['title']}\", "
               f"{spec['kind'].lower()}, starts {spec['start'].lower()} after {', '.join(spec['prerequisites'])}, "
               f"area '{spec['area']}', {len(steps)} steps, {finish}; rewards {reward.get('experience', 0)} XP"
               f"{', a ' + reward.get('gun_rarity_floor', 'COMMON').title() + ' gun' if reward.get('gun') else ''}"
               f"{', a choice of two' if choices else ''}{', items ' + ', '.join(item_ids) if item_ids else ''}")


def mission_ids():
    """The ids of the mission definitions there are."""
    ids = set()
    for path in unreal.EditorAssetLibrary.list_assets(FOLDER, recursive=False):
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.MissionDefinition):
            ids.add(str(asset.get_editor_property('id')))
    return ids


def run():
    mission_types()
    # A side mission waits for its prerequisites: Looter.Missions.Definitions wants every prerequisite to be a mission
    # (Side 1 opens after Main 3, which comes with step 17's main mission).
    known = mission_ids()
    waiting = []
    for spec in MISSIONS:
        missing = [each for each in spec['prerequisites'] if each not in known]
        if missing:
            unreal.log_warning(f"SIDEMISSIONS {spec['asset']} waits for {', '.join(missing)}: nothing was made")
            waiting.append(spec['asset'])
            continue
        setup(spec)
    unreal.log('SIDEMISSIONS done' + (f"; waiting for their prerequisites: {', '.join(waiting)}" if waiting else ''))


run()
