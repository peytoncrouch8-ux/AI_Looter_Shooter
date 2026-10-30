"""Makes the side missions' data assets (UMissionDefinition, Missions/MissionDefinition.h) in /Game/Data/Missions and
saves them, as create_mission_assets.py does for the others (kept apart, so the two can change side by side). Run it in
the open editor after create_story_lines.py and create_mission_assets.py:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_side_mission_assets.py"
It prints a SIDEMISSIONS line per mission and step, and "SIDEMISSIONS done" at the end. Running it again rebuilds these
missions (fields, steps, turn-in and rewards) from what's written here; other mission assets are left alone.

Side missions are picked up from a giver and turned in as the main ones are (create_mission_assets.py's docstring;
Docs/Areas/RansomsRest_Expansion.md sections 2-4, with the user's decisions of 2026-10-13 in its section 13). Each is
worth about an eighth to a fifth of a level where it's met, as a Borderlands side mission is, besides its purse and its
gun (Docs/Progression.md: everything done ends about level 7.5, the story alone about 6.9 since the mine joined the story
on 2026-10-27; Dead Lift and Back Pay below are main missions and carry a main mission's share). Steps are counted from 0, as
the build scripts' conditions count them; "+
optional" is an optional objective in that step. Where things are placed and what's said where is in
Tools/Unreal/story_texts.py (ITEMS, READABLES, TOPICS, ENCOUNTERS).

  The valley, after Main 3:
  DA_Mission_Side1  "Wanted: Already Dead", offered by the poster on the town gate (Artifact_PosterGate): 0 four of the
                    seven wanted posters torn down (held; counted from the world, so the ones torn before count), 1 Ruth
                    Calder's note on the Rim Rangers' board read (anything tagged CalderNote: the AWantedPoster, or the
                    card over it while it offers Calder's Caches) + optional the sheriff's daybook (OdellDaybook). Turned
                    in to Tilly: she wrote ALREADY. 300 XP, a Rare gun, her purse (50).
  DA_Mission_CaldersCaches "Calder's Caches", offered by Ruth's note (Artifact_CalderNote, pinned on the Rangers' board:
                    the board's screen offers it too): 0 the three caches' tags
                    (items CacheTag1-3, shown from Main 1 on, so the bluff path's can be taken in Main 2) + optional the
                    three caches opened (counted from the world), 1 the gang's strongbox in the sheriff's lockup opened
                    (counted from the world), 2 the Deacon's slip in it read (DeaconsSlip). Turned in at the Rangers' board
                    (NoticeBoard_Rangers): the tags on Calder's tally, Hob's word, her fourth cache up the mountain. 250
                    XP, 30 silver.
  DA_Mission_OldFare "The Old Fare", offered by Mr. Sloane through his upstairs door (Speaker_Agent): 0 the depot's waiting
                    room cleared (DepotWaiting, 6), 1 the baggage room (Place_BaggageRoom), 2 Fenn's coffin looked at
                    (FennCoffin, a tap: its words DA_Lines_FennLook), 3 Mr. Sloane asked about the coins (his talk: the
                    confession, DA_Lines_AgentCoins) + optional his desk (AgentDesk), 4 the coins under his door (item
                    FennsFare, shown from step 4), 5 laid back on Fenn's eyes (FennCoffin again, a tap: DA_Lines_FennCoins;
                    the look is its first Interact FennCoffin, the lay its last). Turned in to Josiah Fenn in his coffin
                    (Speaker_Fenn, the same actor). 450 XP, a Rare Drover revolver, 30 silver.
  DA_Mission_LostAndFound "Lost and Found" (2026-10-27, the user's note 37: the quest that opens Lost Property), offered by
                    Mr. Sloane at his ticket window after the Old Fare (Speaker_AgentWindow): 0 his keys under the waiting
                    room's benches (item LostPropertyKeys), 1 the LOST PROPERTY window's shutter unlocked at its hatch (held;
                    the fitting LostPropertyShutter, counted from the world). Both placed by build_area_lostloot.py. Turned
                    in to him at his window, which opens the window (its open_when). 150 XP, 20 silver.
  The valley, after Main 4:
  DA_Mission_Side2  "Unfinished Business", offered by Amos at his fence (Speaker_Amos: his meet lines): 0 three of his
                    hay bales loaded (held; counted from the world), 1 his old hired hands driven off (WhitlockHands, 10;
                    as the last falls he says who they were, the Merritt brothers and Sol Tanner, unpaid: the spawner's
                    cleared_lines) + optional what he owed, chalked in the barn (WagesChalk). Turned in to Amos (his
                    thanks, the topic from step 2). 600
                    XP, a Rare gun, his purse (60).
  DA_Mission_KnockingBelow "Knocking Below", offered by Tilly: 0 the cellar key off the workshop's hook (item
                    CellarKey), 1 down into the cellar (Place_CellarFloor, height counted) + optional the coffin orders
                    (OrdersSlate: by the cellar door, beside the key's hook, so not on the key's step), 2 the cellar's dead
                    quieted (TillyCellar, 6), 3 the cold room's door (ColdRoomDoor), 4 Thaddeus at his bench
                    (Speaker_Thaddeus: his words DA_Lines_ThaddeusTalk) and his tape (item ThaddeusTape, shown from
                    step 4). Turned in to Tilly. 600 XP,
                    a Cinder (Ember) Rare gun, 40 silver.
  After Main 5:
  DA_Mission_Side3  "The Gravemother", offered by Sheriff Odell's bounty on the Rangers' board (NoticeBoard_Rangers):
                    0 where she hungers (entering the Sink: Place_SinkFloor within 19.5 m, height counted; the arrow on the
                    ramp head), 1 her lair cleared (Gravemother: a kill on an earlier visit counts while she's away), 2 her
                    fang (item GravemotherFang, dropped where she falls the first time) + optional the sheriff's badge in
                    her den (SheriffBadge). Turned in to Mrs. Pruitt at the store's hatch (Speaker_Pruitt). 900 XP, 80
                    silver; her Legendary table on every kill.
  The mine, MAIN missions since 2026-10-27 (the user's note 32: Cyrus Teague, the Foreman, the story's second boss; Abel
  the last): made here still, beside the Last Watch that follows Dead Lift; ids and steps unchanged; met first (no card).
  DA_Mission_DeadLift "Dead Lift", met at Hiram Coyle's stool in the winch house (Speaker_Hiram): 0 the clamp pin (item
                    ClampPin, dropped by Abe Haskett, the lift crew's Restless brakeman, tagged AbeHaskett; the crank and
                    the depot's coal went 2026-10-21), 1 the boiler stoked from the quarry's bin and the pin driven (held;
                    each AMissionFitting counted done from the world: LiftFitting_Boiler, LiftFitting_Pin), 2 the quarry held for 60 s while she raises
                    steam (Place_OldQuarry; RaisingSteam), 3 the lift ridden up (Board.Lift: AOreLift opens at step 3).
                    Turned in to Hiram through the speaking tube at the lift head (Speaker_LiftTube). 5,000 XP, an Epic
                    rifle, 40 silver.
  After Dead Lift (side):
  DA_Mission_LastWatch "The Last Watch", offered by the board at Calder's Post (NoticeBoard_CalderPost): 0 the post's yard
                    cleared (CalderPost, 6), 1 lamp oil in Ruth's cabin (item LampOil) + optional her journal
                    (RuthJournal), 2 the watch fire lit (WatchFire, held: AWatchFire's LightStep 2), 3 the post kept for 45 s
                    (Place_CalderPost; Watch), 4 the far light's answer read with Ruth's code book (CodeBook: the far light
                    shows from step 4) + optional the post's log signed (PostLog). Turned in at the post's board (Ellis's
                    and Hob's words). 900 XP, 60 silver (and Calder's fourth cache in the cabin opens, the placing
                    script's).
  DA_Mission_BackPay "Back Pay", met at the lift head's speaking tube (Speaker_LiftTube: Hiram on Teague, said straight
                    after Dead Lift's turn-in there; No. 7's tag, Artifact_MinersTag, offers nothing now): 0 into the
                    Providence (Place_UpperGallery, height counted), 1 the upper gallery cleared (MineUpper, 8), 2 the
                    Foreman's payroll book read (PayrollBook) + optional the chalk on the wall (ChalkTally), 3 the lower
                    drift cleared (MineLower, 9), 4 the eleven's tags from their barricade (Barricade), 5 the Foreman put
                    down (his lair, Foreman), 6 the payroll from the stope (item Payroll). Turned in at the tag board
                    (Speaker_TagBoard) with a choice: pay the eleven (story flag BackPay.Paid, an Epic gun) or keep the
                    payroll (BackPay.Kept, 400 silver); Hob then sends Ellis to Delia's door (Main 6). 6,500 XP either way.

Objectives are instanced objects inside the asset, one class per kind, made with unreal.new_object(<class>, asset) and
listed in each step's 'objectives'. Classes for actor filters are loaded by their script path
('/Script/AI_Looter_Shooter.WantedPoster'). A spec's 'expect' names each step's objective class, or a list of them in
order for a step of several (one with optional objectives).

The mission features (wave 2a) are here as in create_mission_assets.py (which runs on import, so its helpers are copied):
a spec's 'offer' (offer(): a mission picked up from its giver, start 'OFFERED'); optional objectives (objective(...,
optional=True), event_objective, read_objective); 'items' (item()) counted by collect_items(); a turn-in with two choice()s
(turn_in(choices=..., question=...)); rewards(silver=..., optional_silver=...).
"""
import unreal

FOLDER = '/Game/Data/Missions'
CLASSES = '/Script/AI_Looter_Shooter.'
# Where the mission props' models are imported (Art/Models/Props/MissionProps.py through Tools/models.ps1).
PROPS = '/Game/Art/Props/'
# The reward guns of a kind (UWeaponDefinition assets).
REVOLVER = '/Game/Weapons/Data/DA_Revolver'
RIFLE = '/Game/Weapons/Data/DA_AssaultRifle'
# AWantedPoster's tags (World/WantedPoster.h).
WANTED_TAG = 'WantedPoster'
NOTE_TAG = 'CalderNote'
# Side 2's pieces (Story/AmosWhitlock.h: SpeakerTag; World/HayBale.h: BaleTag; build_area_whitlock.py: the hands' encounter).
AMOS_TAG = 'Speaker_Amos'
BALE_TAG = 'HayBale'
# Any three of the six bales in the field (the user's cut of the chores, 2026-10-21).
BALES = 3
HANDS = 'WhitlockHands'
HANDS_COUNT = 10  # Barn Doors set piece (2026-10-28): 5 + a Restless leader, then 4 (the Bell-Ringer is placed, not counted)
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
# The Ranger caches and the gang's strongbox (Tools/Unreal/build_area_caches.py: AChest, tagged Chest and these).
CACHE_TAG = 'RangerCache'
STRONGBOX_TAG = 'GangStrongbox'
# Givers and turn-ins (the shared names of the wave 4 contract).
TILLY_TAG = 'Speaker_Tilly'
RANGERS_BOARD = 'NoticeBoard_Rangers'
POST_BOARD = 'NoticeBoard_CalderPost'

ELLIS = 'Ellis'
HOB = 'Hob'
# Back Pay's last word, either choice (2026-10-27: the mine is the story's now, and the boards come next): Hob sends Ellis
# to Delia's door, where The Gravewind is met. Shared words (no "Pa", "Ellis" or "kid"), so no hero line row.
BOARDS_NEXT = "Teague's settled. Now the Keeper, sunshine. Delia's door first."


def mission_types():
    """The reflected mission types, or a clear error when the C++ isn't built yet."""
    names = ['MissionDefinition', 'MissionStep', 'MissionRewards', 'MissionTurnIn', 'StoryLine', 'MissionActorFilter',
             'MissionKind', 'MissionStart', 'MissionInteractObjective', 'MissionLastingInteractObjective', 'WantedPoster',
             'WeaponRarity', 'MissionReachObjective', 'MissionClearObjective', 'MissionPlace', 'MissionTalkObjective',
             'MissionWaypoint', 'MissionOffer', 'MissionEventObjective', 'MissionDefendObjective', 'MissionBoardObjective',
             # The mission features (wave 2a): a turn-in's choice, mission items and their objective.
             'MissionTurnInChoice', 'MissionItemInfo', 'MissionCollectItemObjective']
    missing = [name for name in names if getattr(unreal, name, None) is None]
    if missing:
        raise RuntimeError(f"unreal.{', unreal.'.join(missing)} missing: build the C++ with the mission features first")


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


def place(tag, radius, ignore_height=True):
    """FMissionPlace: the nearest actor carrying tag, and how close counts as there (height counted unless ignored)."""
    where = unreal.MissionPlace()
    where.set_editor_property('actor', actor_filter(tag=tag))
    where.set_editor_property('radius', radius)
    where.set_editor_property('ignore_height', ignore_height)
    return where


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


def interact(asset, text, tag, cls_name=None, count=1, hold=False, lasting=False, **kwargs):
    """Use count things carrying tag (held when hold); lasting counts what stays used in the world (a poster torn down,
    a chest opened), so the ones used before the step began count too."""
    cls = unreal.MissionLastingInteractObjective if lasting else unreal.MissionInteractObjective
    return objective(asset, cls, text, target=actor_filter(cls_name, tag), count=count, hold=hold,
                     show_count=count > 1, **kwargs)


def clear(asset, text, spawner_id, count, **kwargs):
    """An encounter's creatures all down (UMissionClearObjective: counted from its spawner, so kills before count)."""
    return objective(asset, unreal.MissionClearObjective, text, spawner_id=unreal.Name(spawner_id), count=count, **kwargs)


def reach(asset, text, tag, radius, ignore_height=True, **kwargs):
    return objective(asset, unreal.MissionReachObjective, text, place=place(tag, radius, ignore_height), show_count=False,
                     **kwargs)


def talk(asset, text, tag, **kwargs):
    return objective(asset, unreal.MissionTalkObjective, text, speaker_tag=unreal.Name(tag), show_count=False, **kwargs)


def defend(asset, text, tag, radius, seconds, **kwargs):
    """Stay within radius of the place tagged tag for seconds (UMissionDefendObjective): the count is the time."""
    return objective(asset, unreal.MissionDefendObjective, text, place=place(tag, radius), hold_seconds=seconds,
                     stay_inside=True, **kwargs)


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


def element_enum(name):
    """unreal.LooterElement by the enum's C++ name ('SoulLight', 'Ember'; '' = None): Python spells it SOUL_LIGHT."""
    if not name:
        return unreal.LooterElement.NONE
    import re
    for spelled in (re.sub(r'(?<!^)(?=[A-Z])', '_', name).upper(), name.upper()):
        value = getattr(unreal.LooterElement, spelled, None)
        if value is not None:
            return value
    raise RuntimeError(f'unreal.LooterElement has no {name}: nothing was saved')


def rewards(experience=0, gun=False, gun_rarity_floor='COMMON', gun_kind='', named_gun='', unlock_areas=(), silver=0,
            optional_silver=10, gun_maker='', gun_element=''):
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
    # The gun's maker by key ('SaintMaud') and element by the enum's name ('SoulLight'): Main 4's Saint Maud with Hallow
    # rounds, Knocking Below's Cinder (the RR tuning the user took, 2026-10-29 night; MissionRewards::MakeRewardGun).
    made.set_editor_property('gun_maker', unreal.Name(gun_maker))
    made.set_editor_property('gun_element', element_enum(gun_element))
    made.set_editor_property('named_gun', unreal.Name(named_gun))
    made.set_editor_property('unlock_areas', [unreal.Name(area) for area in unlock_areas])
    return made


def choice(title, line, flag='', stamp='', lines=(), **reward_settings):
    """FMissionTurnInChoice: one of a turn-in's two choices (its title, what it means, the story flag it sets, the word
    stamped as it's chosen, what's said then) and rewards()'s fields for what it gives in place of the mission's own
    (experience left at 0 keeps the mission's)."""
    made = unreal.MissionTurnInChoice()
    made.set_editor_property('title', unreal.Text(title))
    made.set_editor_property('line', unreal.Text(line))
    made.set_editor_property('flag', unreal.Name(flag))
    made.set_editor_property('stamp', unreal.Text(stamp))
    made.set_editor_property('lines', story_lines(lines))
    made.set_editor_property('rewards', rewards(**reward_settings))
    return made


def offer(giver_tag='', giver='', lines=(), briefing='', accept_lines=(), meet_first=False):
    """FMissionOffer, as create_mission_assets.py makes it: who offers a mission picked up from its giver (start='OFFERED')
    by the tag their actor carries (Speaker_Agent, NoticeBoard_CalderPost, Artifact_MinersTag), their name for people
    (empty: an artifact's own title), what a speaker says as they offer it (lines of (speaker, words): said beside the card,
    which opens at once), what they say once it's taken (accept_lines: Accept cuts the offer's lines short and goes
    straight to these, Borderlands' way, 2026-10-22), and the offer card's story. Without a giver tag the offer is empty,
    as every mission that isn't offered keeps it. Its prerequisites are its story gate: nobody offers it before.
    meet_first (a main mission only: Dead Lift and Back Pay since 2026-10-27): never a card; listed as "Talk to <giver>"
    until talking to them starts it, their lines then their accept_lines said as one talk."""
    made = unreal.MissionOffer()
    made.set_editor_property('giver_tag', unreal.Name(giver_tag))
    made.set_editor_property('giver_name', unreal.Text(giver))
    made.set_editor_property('lines', story_lines(lines))
    made.set_editor_property('accept_lines', story_lines(accept_lines))
    made.set_editor_property('meet_first', bool(meet_first))
    made.set_editor_property('briefing', unreal.Text(briefing))
    return made


def turn_in(automatic=False, speaker='', giver='', lines=(), choices=(), question='', hand_items=()):
    """FMissionTurnIn, as create_mission_assets.py makes it: the giver's speaker tag and name (anyone at a speaker point: a
    door, a coffin, a speaking tube, a tag board; or a notice board, which turns it in through its screen and says the
    lines as captions once the screen closes, a line naming no speaker shown as written), what they say as it's turned in
    (none: their own lines there), and none or two choice()s (or dicts of its fields) with the card's question; automatic:
    it finishes by itself after its last objective."""
    if len(choices) not in (0, 2):
        raise RuntimeError(f'a turn-in takes no choices or two, not {len(choices)}: nothing was changed')
    made = unreal.MissionTurnIn()
    made.set_editor_property('automatic', automatic)
    made.set_editor_property('speaker_tag', unreal.Name(speaker))
    made.set_editor_property('giver_name', unreal.Text(giver))
    made.set_editor_property('lines', story_lines(lines))
    made.set_editor_property('choices', [choice(**each) if isinstance(each, dict) else each for each in choices])
    made.set_editor_property('choice_question', unreal.Text(question))
    if hand_items:
        try:
            made.set_editor_property('hand_items', [unreal.Name(each) for each in hand_items])
        except Exception as error:
            unreal.log_warning(f'SIDEMISSIONS: no MissionTurnIn.hand_items ({error}): build the game module')
    return made


# ---------------------------------------------------------------------------
# The valley's side missions
# ---------------------------------------------------------------------------

def side1_steps(asset):
    """Tear down the posters (any 4 of the 7, held), then read Calder's note on the Rangers' board (the sheriff's daybook
    in the office beside it, if Ellis looks). The arrows find their own: the nearest poster still up, then the note. The
    note is anything tagged CalderNote, not only the paper (AWantedPoster): while Calder's Caches is on offer the card laid
    over the paper (Artifact_CalderNote, tagged CalderNote too) takes the key, and reading it there counts."""
    return [
        step(interact(asset, 'Tear down your wanted posters around town and on the farms.', WANTED_TAG, 'WantedPoster',
                      count=4, hold=True, lasting=True, short='Tear down your wanted posters')),
        step(interact(asset, "Read the note on the Rim Rangers' board by the sheriff's office.", NOTE_TAG,
                      short="Read the note on the Rangers' board"),
             read_objective(asset, "Look round the sheriff's office: the daybook on his desk.", 'OdellDaybook',
                            optional=True, short="Read the sheriff's daybook")),
    ]


def caches_steps(asset):
    """Ruth's three caches by her riddles (the windmill, the Sink's rim, the bluff path): the tag pinned to each, taken
    (the arrow on the nearest tag still lying; one taken before the mission counts), and the crates opened if Ellis
    likes (counted from the world: Main 2's bluff crate counts); then the gang's strongbox in the lockup (counted from the
    world too) and the Deacon's slip in it."""
    return [
        step(collect_items(asset, "Find Calder's three caches and take the Ranger's tag from each.",
                           ['CacheTag1', 'CacheTag2', 'CacheTag3'], short="Take the tags from Calder's caches"),
             # Supply Crates and the Strongbox open on a tap (AChest: HoldSeconds is only the coffin's and the grave's), so
             # neither the words nor the objective say hold (2026-10-14; the play-through read "hold [E]" on a tap).
             interact(asset, "Open Calder's three caches.", CACHE_TAG, 'Chest', count=3, lasting=True, optional=True,
                      short="Open Calder's caches")),
        step(interact(asset, "Open the gang's strongbox in the sheriff's lockup. You know the combination.", STRONGBOX_TAG,
                      'Chest', lasting=True, short="Open the gang's strongbox")),
        step(read_objective(asset, 'Read the slip folded in with the banknotes.', 'DeaconsSlip',
                            short='Read the slip in the strongbox')),
    ]


def old_fare_steps(asset):
    """The depot's waiting room cleared, the baggage room, Fenn's coffin, Mr. Sloane's confession through his door (his
    desk's marker, if Ellis looks), Fenn's coins under the door, the coins back on Fenn's eyes."""
    return [
        step(clear(asset, "Clear the dead out of the depot's waiting room.", 'DepotWaiting', 6,
                   short='Clear the waiting room')),
        # 2.5 m: inside the baggage room only (its middle is 2.8 m from the waiting room's partition and 2.7 m from the
        # platform door; at 4.5 m the waiting room's east end counted, and a player who won the fight there skipped this
        # step, the play-check of 2026-10-23).
        step(reach(asset, 'Find what they were waiting on, in the baggage room.', 'Place_BaggageRoom', 250.0,
                   ignore_height=False, short='Search the baggage room')),
        step(interact(asset, 'Look at the coffin tagged for Gravesend.', 'FennCoffin', short='Look at the coffin')),
        step(talk(asset, "Ask Mr. Sloane about Fenn's coins.", 'Speaker_Agent'),
             read_objective(asset, "Look in the agent's desk.", 'AgentDesk', optional=True)),
        step(collect_items(asset, "Take Fenn's coins from under Mr. Sloane's door.", ['FennsFare'],
                           short="Take Fenn's coins")),
        # Any use of the coffin counts (a held one too), whatever the placing script makes of it.
        step(interact(asset, "Lay the coins back on Fenn's eyes.", 'FennCoffin', short="Lay the coins on Fenn's eyes")),
    ]


def lost_and_found_steps(asset):
    """Mr. Sloane's keys off the waiting room's floor (a find: the arrow on them), then the LOST PROPERTY window's shutter
    unlocked at its hatch (held; an AMissionFitting tagged LostPropertyShutter, counted from the world). Both placed by
    build_area_lostloot.py, whose KEYS_STEP 0 and SHUTTER_STEP 1 are these steps; the window opens at the turn-in."""
    return [
        step(collect_items(asset, "Find Mr. Sloane's keys. They went under the waiting room's benches when he ran.",
                           ['LostPropertyKeys'], short="Find Mr. Sloane's keys")),
        step(interact(asset, "Unlock the LOST PROPERTY window in the baggage room: hold {Interact} at its hatch.",
                      'LostPropertyShutter', 'MissionFitting', hold=True, lasting=True, short='Unlock the Lost Property window',
                      waypoint=unreal.MissionWaypoint.ACTOR, waypoint_tag='LostPropertyShutter')),
    ]


def side2_steps(asset):
    """Three of Amos's bales loaded (held, counted from the world: the session keeps the loaded ones with the map), then
    his old hired hands, come up the road as the last goes in, driven off (their encounter cleared; the twist, who they are
    and what he owed them, is said as the last one falls: the spawner's cleared_lines DA_Lines_AmosRoad; the wages chalked
    in the barn, if Ellis reads it); then it's turned in to Amos (his thanks: the topic from step 2)."""
    return [
        step(interact(asset, 'Load three hay bales into his barn.', BALE_TAG, 'HayBale', count=BALES, hold=True,
                      lasting=True, short='Load the hay bales')),
        step(clear(asset, 'Drive off the men on the road.', HANDS, HANDS_COUNT),
             read_objective(asset, 'Read what Amos owed, chalked inside the barn door.', 'WagesChalk', optional=True,
                            short='Read what Amos owed')),
    ]


def knocking_steps(asset):
    """The key on Thaddeus's hook in the workshop, down into the cellar (the coffin orders on the slate by the cellar
    door, if Ellis reads them), its dead quieted, the cold room's door, Thaddeus at his bench and his tape for Tilly (one step: the tape lies by him). The slate
    hangs beside the key's hook, so its optional can't sit on the key's step (an optional ends with its step, and a player
    who takes the key first would lose it at once); it's on the step through the cellar door, which the slate is by."""
    return [
        step(collect_items(asset, "Take the cellar key off Thaddeus's measuring hook, in the workshop off the yard.",
                           ['CellarKey'], short='Take the cellar key')),
        # Height counted, and under the 3.6 m between the workshop's floor and the cellar's, so standing over it upstairs
        # isn't down.
        step(reach(asset, 'Go down into the cellar.', 'Place_CellarFloor', 350.0, ignore_height=False),
             read_objective(asset, 'Read the coffin orders on the slate.', 'OrdersSlate', optional=True)),
        step(clear(asset, 'Quiet the dead in the cellar.', 'TillyCellar', 6)),
        step(interact(asset, "Find what's still knocking: open the cold room.", 'ColdRoomDoor',
                      short="Find what's still knocking")),
        step(talk(asset, 'Talk to the man at the bench.', 'Speaker_Thaddeus'),
             collect_items(asset, "Take Thaddeus's tape measure for Tilly.", ['ThaddeusTape'],
                           short="Take Thaddeus's tape")),
    ]


# ---------------------------------------------------------------------------
# After Main 5: the Gravemother, and the mountain
# ---------------------------------------------------------------------------

def side3_steps(asset):
    """Find where the Gravemother hungers (done on entering the Sink: the floor's place, SINK_REACH measured with its
    height; the arrow on the ramp head on the rim, the Sink's way in), kill her (her lair's encounter cleared; the arrow
    finds her), take her fang where she fell (the sheriff's badge in her den, if Ellis looks)."""
    sink = unreal.MissionPlace()
    sink.set_editor_property('actor', actor_filter(tag=SINK_TAG))
    sink.set_editor_property('radius', SINK_REACH)
    sink.set_editor_property('ignore_height', False)
    return [
        # The user's own words, with the full stop every objective ends with (2026-10-14; Looter.Story.Gravemother.Mission
        # checks them).
        step(objective(asset, unreal.MissionReachObjective, 'Find where the Gravemother hungers.', show_count=False,
                       place=sink, waypoint=unreal.MissionWaypoint.ACTOR, waypoint_actor=actor_filter(tag=SINK_RIM_TAG))),
        step(clear(asset, 'Kill the Gravemother.', GRAVEMOTHER_LAIR, 1, show_count=False)),
        step(collect_items(asset, 'Take her fang.', ['GravemotherFang']),
             read_objective(asset, "Find what's left of Sheriff Odell in her den.", 'SheriffBadge', optional=True,
                            short="Find what's left of the sheriff")),
    ]


def dead_lift_steps(asset):
    """The clamp pin off Abe Haskett's crew on the quarry floor (2026-10-21: the crank and the depot's coal went, the user's
    cut of the fetches; the boiler takes the quarry's own coal), the two fittings, the quarry held while she raises steam,
    the ride up. The ride must stay the fourth step (AOreLift::OpensAtStep 3)."""
    return [
        step(collect_items(asset, 'Get the clamp pin back from Abe Haskett, out on the quarry floor.', ['ClampPin'],
                           short='Get the clamp pin from Abe Haskett')),
        # The fittings (AMissionFitting) count as used up once done, so each is counted from the world (lasting).
        step(interact(asset, 'Stoke the boiler from the coal bin: hold {Interact}.', 'LiftFitting_Boiler', hold=True,
                      lasting=True,
                      short='Stoke the boiler'),
             # Its arrow leads to the landing it's driven from (Place_PinSpot, build_area_missionprops_mountain.py): under
             # the clamp, the gallows' arm hides it.
             interact(asset, 'Drive the clamp pin at the bottom sheave: hold {Interact}.', 'LiftFitting_Pin', hold=True,
                      lasting=True, short='Drive the clamp pin', waypoint=unreal.MissionWaypoint.ACTOR,
                      waypoint_tag='Place_PinSpot')),
        step(defend(asset, 'Hold the quarry while she raises steam.', 'Place_OldQuarry', 2500.0, 60.0,
                    short='Hold the quarry')),
        step(objective(asset, unreal.MissionBoardObjective, 'Ride the lift to the top: hold {Interact} on the bell pull.',
                       short='Ride the lift up', waypoint=unreal.MissionWaypoint.ACTOR, waypoint_tag='OreLift',
                       vehicle=unreal.Name('Lift'), show_count=False)),
    ]


def last_watch_steps(asset):
    """The post's yard cleared, the oil in Ruth's cabin (her journal, if Ellis reads it), the watch fire lit (it must stay
    the third step: AWatchFire's LightStep 2), the post kept until a post down the Rim answers, the answer read with
    Ruth's code book (the far light shows from the fifth: AFarSignalLight's AnswerStep 4; the post's log signed, if
    Ellis will)."""
    return [
        step(clear(asset, "Clear the post's yard.", 'CalderPost', 6)),
        step(collect_items(asset, "Find oil for the watch fire in Ruth's cabin.", ['LampOil'], short='Find lamp oil'),
             read_objective(asset, "Read Ruth Calder's journal.", 'RuthJournal', optional=True)),
        step(interact(asset, 'Light the watch fire on the tower: hold {Interact}.', 'WatchFire', hold=True,
                      short='Light the watch fire')),
        step(defend(asset, 'Keep the post until a post down the Rim answers.', 'Place_CalderPost', 2000.0, 45.0,
                    short='Keep the post')),
        step(read_objective(asset, "Read the answer with Ruth's code book.", 'CodeBook', short='Read the answer'),
             read_objective(asset, "Sign the post's log.", 'PostLog', optional=True)),
    ]


def back_pay_steps(asset):
    """Into the Providence, the upper gallery's shift cleared, the Foreman's payroll book (the chalk on the wall, if Ellis
    reads it), the lower drift cleared, the eleven's tags off their barricade, the Foreman put down, the payroll taken
    from the stope; then the choice at the tag board."""
    return [
        step(reach(asset, 'Go into the Providence.', 'Place_UpperGallery', 1200.0, ignore_height=False)),
        step(clear(asset, 'Clear the upper gallery.', 'MineUpper', 8)),
        step(read_objective(asset, "Search the Foreman's office.", 'PayrollBook'),
             read_objective(asset, 'Read the chalk on the gallery wall.', 'ChalkTally', optional=True)),
        step(clear(asset, 'Go down the decline and clear the lower drift.', 'MineLower', 9, short='Clear the lower drift')),
        step(read_objective(asset, "Take the eleven's tags from their barricade.", 'Barricade')),
        step(clear(asset, 'Put the Foreman down.', 'Foreman', 1, show_count=False)),
        step(collect_items(asset, 'Take the payroll from the stope.', ['Payroll'])),
    ]


MISSIONS = [
    # --- The valley, after Main 3 ---
    dict(asset='DA_Mission_Side1', id='Side1', title='Wanted: Already Dead',
         summary="Your wanted poster hangs all over Ransom's Rest, and someone has written ALREADY under DEAD OR ALIVE. "
                 "Tear them down, and find out who keeps putting them up.",
         kind='SIDE', start='OFFERED', area='RansomsRest', prerequisites=['Main3'], sort_order=10, steps=side1_steps,
         offer=dict(giver_tag='Artifact_PosterGate', giver='', briefing=(
             "WANTED: ELLIS RANSOM, OF THE DUNNE GANG. DEAD OR ALIVE. Somebody has written ALREADY under the last line. "
             "There's one on every post in town and on both farms.")),
         # Back to Tilly at her window, a short walk up Main Street from the Rangers' board: the twist, then the reward
         # gun, the box of effects nobody came back for.
         turn_in=dict(speaker=TILLY_TAG, giver='Tilly', lines=[
             ('', "So it was you, tearing them down. Good."),
             (ELLIS, 'Who wrote ALREADY?'),
             ('', "I did. On the first one. Somebody had to."),
         ]),
         rewards=dict(experience=300, gun=True, gun_rarity_floor='RARE'),
         expect=['MissionLastingInteractObjective', ['MissionInteractObjective', 'MissionEventObjective']]),
    dict(asset='DA_Mission_CaldersCaches', id='CaldersCaches', title="Calder's Caches",
         summary="Ruth Calder of the Rim Rangers laid three caches on Ransom's Rest for her Rangers, and left a riddle for "
                 "each. Find them and take their tags, then open the gang's strongbox in the sheriff's lockup.",
         kind='SIDE', start='OFFERED', area='RansomsRest', prerequisites=['Main3'], sort_order=11, steps=caches_steps,
         offer=dict(giver_tag='Artifact_CalderNote', giver='', briefing=(
             "RIM RANGERS. Three caches laid for any Ranger riding through: under the iron flower that drinks the wind; on "
             "the lip of the hungry hole; on the path where the Ransom kid used to go to sulk. Take what you need and pin "
             "the tag. The gang's strongbox is in the sheriff's lockup as evidence. The fourth cache is at the Larkspur "
             "post, if the lift ever runs again. \u2014 Capt. R. Calder")),
         items=[dict(item_id='CacheTag1', name="Ranger's tag: the windmill",
                     line="A Rim Rangers' brass tag from the cache under the iron flower that drinks the wind.",
                     mesh=PROPS + 'SM_CacheTag'),
                dict(item_id='CacheTag2', name="Ranger's tag: the Sink",
                     line="A Rim Rangers' brass tag from the cache on the lip of the hungry hole.",
                     mesh=PROPS + 'SM_CacheTag'),
                dict(item_id='CacheTag3', name="Ranger's tag: the bluff path",
                     line="A Rim Rangers' brass tag from the cache where the Ransom kid used to go to sulk.",
                     mesh=PROPS + 'SM_CacheTag')],
         # Pinned on Calder's tally at the board, Ranger fashion: turned in through the board's screen, whose words play
         # as it closes (ANoticeBoard::TurnIn). Her note's last line points up the mountain, to Dead Lift.
         turn_in=dict(speaker=RANGERS_BOARD, giver="the Rangers' board", lines=[
             (HOB, "Three tags for the Rangers. She'll still hang you, sunshine."),
             (ELLIS, "Her fourth's at the Larkspur post. Up the old lift."),
         ]),
         rewards=dict(experience=250, silver=30),
         expect=[['MissionCollectItemObjective', 'MissionLastingInteractObjective'], 'MissionLastingInteractObjective',
                 'MissionEventObjective']),
    dict(asset='DA_Mission_OldFare', id='OldFare', title='The Old Fare',
         summary="Mr. Sloane, the station agent, has locked himself upstairs in the depot. Every night the dead stand in his "
                 "waiting room as if for a train. Clear them out, and find out what they're waiting on.",
         kind='SIDE', start='OFFERED', area='RansomsRest', prerequisites=['Main3'], sort_order=12, steps=old_fare_steps,
         # His last line is what he says once it's taken (Accept goes straight to it, 2026-10-22).
         offer=dict(giver_tag='Speaker_Agent', giver='Mr. Sloane', lines=[
             ('', "Who's there? ...Lord. The Ransom kid."),
             ('', "Every night since Wednesday they come and stand in my waiting room like they've a train to catch. I "
                  "can hear them breathing."),
         ], accept_lines=[
             ('', "If you're dead too, maybe they'll listen to you."),
         ], briefing="Mr. Sloane, the station agent, has locked himself upstairs. Every night since the saint went dark "
                     "the dead have stood in his waiting room as if for a train. Clear them out, and find out what "
                     "they're waiting on."),
         items=[dict(item_id='FennsFare', name="Fenn's fare",
                     line="Two old gold coins, worn thin: grave gold, the fare the dead once paid. Mr. Sloane borrowed "
                          "them off Josiah Fenn's eyes.",
                     mesh=PROPS + 'SM_GraveCoins')],
         # Josiah Fenn, a talking coffin bound for Gravesend: the waiting room stays quiet from then on, and Mr. Sloane
         # comes down to his ticket window.
         turn_in=dict(speaker='Speaker_Fenn', giver='Josiah Fenn', lines=[
             ('', "Much obliged. A man likes to pay his own way."),
             ('', "Tell Sloane I forgive him. Don't tell him I said it like that."),
             ('', "There's a pistol under me. Never fired it once. Take it."),
         ]),
         rewards=dict(experience=450, gun=True, gun_rarity_floor='RARE', gun_kind=REVOLVER, silver=30),
         expect=['MissionClearObjective', 'MissionReachObjective', 'MissionInteractObjective',
                 ['MissionTalkObjective', 'MissionEventObjective'], 'MissionCollectItemObjective',
                 'MissionInteractObjective']),
    # The quest that opens Lost Property (the user's note 37, 2026-10-27: "There should be a quest to unlock Lost Loot"):
    # Mr. Sloane, back at his ticket window once the Old Fare is done, has his keys to find. Short on purpose (a find and a
    # held unlock), early enough to matter: until it's turned in the window stays shut and guns left lying stay where they
    # lie (ULostLootSubsystem holds the ground's clock), so nothing is lost before it.
    dict(asset='DA_Mission_LostAndFound', id='LostAndFound', title='Lost and Found',
         summary="Mr. Sloane kept the depot's LOST PROPERTY window until the night the dead came, when he dropped its keys "
                 "running for the stair. Find them and open the window, and guns left lying about the Rest will find their "
                 "way to his shelves.",
         kind='SIDE', start='OFFERED', area='RansomsRest', prerequisites=['OldFare'], sort_order=13,
         steps=lost_and_found_steps,
         offer=dict(giver_tag='Speaker_AgentWindow', giver='Mr. Sloane', lines=[
             ('', "Lost Property's been shut since Wednesday. I dropped the keys running for the stair."),
             ('', "They went under the benches out there. I'm not crawling about where they stood."),
         ], accept_lines=[
             ('', "Bring them and open her up. Folk leave half their lives on a railroad."),
         ], briefing="Mr. Sloane kept the depot's LOST PROPERTY window until the night the dead came, when he dropped its "
                     "keys running for the stair. Find them in the waiting room and unlock the window: from then on, "
                     "guns left lying about the Rest find their way to his shelves."),
         items=[dict(item_id='LostPropertyKeys', name="Mr. Sloane's keys",
                     line="The depot's ring of keys. The brass one says LOST PROPERTY.",
                     mesh=PROPS + 'SM_CellarKey')],
         # At his window: how the shelves work, in his words, and who brings the guns in (Hob's crows: they carry Ellis
         # between roosts already).
         turn_in=dict(speaker='Speaker_AgentWindow', giver='Mr. Sloane', lines=[
             ('', "There she is. Shelves are bare, but they won't stay that way."),
             ('', "Anything left lying about the Rest a while ends up here. Claim it at the window. Shelving's extra."),
             (HOB, "My crows pick up after you, sunshine. Somebody has to."),
         ]),
         rewards=dict(experience=150, silver=20),
         expect=['MissionCollectItemObjective', 'MissionLastingInteractObjective']),

    # --- The valley, after Main 4 ---
    dict(asset='DA_Mission_Side2', id='Side2', title='Unfinished Business',
         summary=("Amos Whitlock died last harvest with his hay half in. He was still on the Sundown Road when the saint "
                  "went dark, and drifted home to find it rotting in the field. He isn't angry yet. He'd like it in."),
         kind='SIDE', start='OFFERED', area='RansomsRest', prerequisites=['Main4'], sort_order=20, steps=side2_steps,
         offer=dict(giver_tag=AMOS_TAG, giver='Amos Whitlock', lines=[
             (ELLIS, 'Amos Whitlock?'),
             ('', "What's left of him. And you're Abel's young'un. I'm sorry for it."),
             ('', "Died last harvest with the hay half in. Got turned back on the road."),
             ('', "My hands go through a bale like smoke. Load it for me?"),
         ], accept_lines=[
             ('', "Obliged. I'll be at the fence."),
         ], briefing="Amos Whitlock died last harvest with his hay half in. He was still on the Sundown Road when the saint "
                     "went dark, and drifted home to find it rotting in the field. He isn't angry yet. He'd like it in."),
         # His thanks are his topic at the turn-in's step (DA_Lines_AmosThanks, build_area_whitlock's THANKS_STEP 3).
         turn_in=dict(speaker=AMOS_TAG, giver='Amos'),
         # Rare (was Epic: the RR tuning the user took, 2026-10-29 night).
         rewards=dict(experience=600, gun=True, gun_rarity_floor='RARE'),
         expect=['MissionLastingInteractObjective', ['MissionClearObjective', 'MissionEventObjective']]),
    dict(asset='DA_Mission_KnockingBelow', id='KnockingBelow', title='Knocking Below',
         summary="Bright & Daughter keeps the Rest's dead in its cellar until their seven days are up. Since the saint went "
                 "dark something down there knocks all night. The way in is through Tilly's father's workshop.",
         kind='SIDE', start='OFFERED', area='RansomsRest', prerequisites=['Main4'], sort_order=21, steps=knocking_steps,
         offer=dict(giver_tag=TILLY_TAG, giver='Tilly', lines=[
             ('', "Something's knocked in my cellar every night since Wednesday. No keeper to walk them."),
             ('', "I'd go down myself, except I'd rather not."),
         ], accept_lines=[
             ('', "The key's on the hook in the workshop. Mind the steps."),
         ], briefing="Bright & Daughter keeps the Rest's dead in its cellar until their seven days are up. Since the saint "
                     "went dark something down there knocks all night, and Tilly hasn't been below since. The way in is "
                     "through her father's workshop, off the undertaker's yard."),
         items=[dict(item_id='CellarKey', name='The cellar key',
                     line="Iron, from Thaddeus Bright's measuring hook. Tilly never moved it.",
                     mesh=PROPS + 'SM_CellarKey'),
                dict(item_id='ThaddeusTape', name="Thaddeus's tape measure",
                     line="Brass-cased, the initials worn off. For Tilly, with a word about the brass.",
                     mesh=PROPS + 'SM_TapeMeasure')],
         turn_in=dict(speaker=TILLY_TAG, giver='Tilly', lines=[
             ('', "...He's down there."),
             ('', "Of course he is. He never once came up for supper."),
             ('', "Brass handles. He'd have hated them."),
             ('', "A box of his is under the counter. Don't make me look at it."),
         ]),
         # A Cinder Rare (was any Rare: the RR tuning the user took, 2026-10-29 night).
         rewards=dict(experience=600, gun=True, gun_rarity_floor='RARE', gun_element='Ember', silver=40),
         expect=['MissionCollectItemObjective', ['MissionReachObjective', 'MissionEventObjective'],
                 'MissionClearObjective', 'MissionInteractObjective', ['MissionTalkObjective', 'MissionCollectItemObjective']]),

    # --- After Main 5 ---
    dict(asset='DA_Mission_Side3', id='Side3', title='The Gravemother',
         summary="A month ago Sheriff Odell posted a bounty on the big one in the Sink's den, then went down after her "
                 "himself. The town's purse is paid at Pruitt's for her fang.",
         kind='SIDE', start='OFFERED', area='RansomsRest', prerequisites=['Main5'], sort_order=30, steps=side3_steps,
         offer=dict(giver_tag=RANGERS_BOARD, giver="Sheriff Odell's bounty", briefing=(
             "BOUNTY. The big one in the Sink's den. She took my dog and the Haskett mule. The town's purse to whoever "
             "brings me her fang, paid at Pruitt's. I'm going down after her myself. \u2014 Sheriff W. Odell")),
         items=[dict(item_id='GravemotherFang', name="The Gravemother's fang",
                     line="As long as a forearm and still wet. The sheriff's bounty is paid on it at Pruitt's.",
                     mesh=PROPS + 'SM_GravemotherFang', dropped_by_class='GravemotherCreature')],
         # Mrs. Pruitt at the store's hatch: the purse, and her brother.
         turn_in=dict(speaker='Speaker_Pruitt', giver='Mrs. Pruitt', lines=[
             ('', "Lord, the size of it."),
             ('', "Wade was my brother. He always said he'd die with his boots on. Never said a word about his hat."),
         ]),
         rewards=dict(experience=900, silver=80),
         expect=['MissionReachObjective', 'MissionClearObjective', ['MissionCollectItemObjective', 'MissionEventObjective']]),
    dict(asset='DA_Mission_DeadLift', id='DeadLift', title='Dead Lift',
         summary="The Providence Mining Company's ore lift still hangs over the Old Quarry, and its engineman, Hiram Coyle, "
                 "still waits by its cold boiler. Get its clamp pin back from Abe Haskett, and he'll take you up Larkspur "
                 "Ridge to the Providence, where Cyrus Teague's dead shift still works.",
         # A main mission since 2026-10-27 (the user's note 32: Cyrus Teague, the Foreman, the story's second boss): after
         # Main 5, met first at Hiram's stool ("Talk to Hiram Coyle", Main 5's turn-in sends Ellis there), no card. Its
         # id and steps are unchanged (AOreLift opens at step 3; old saves keep their steps).
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['Main5'], sort_order=6, steps=dead_lift_steps,
         # Ellis's answer is the taking: the talk says his offer, then the answer and his ask (2026-10-22).
         offer=dict(giver_tag='Speaker_Hiram', giver='Hiram Coyle', meet_first=True, lines=[
             ('', "Ain't had a passenger in four years. Riding up?"),
         ], accept_lines=[
             (ELLIS, 'Riding.'),
             ('', "Then I need her clamp pin back. Abe Haskett took it down the hill the day of the fall."),
         ], briefing="The old ore lift still hangs over the Old Quarry, and its engineman, Hiram Coyle, still waits by its "
                     "boiler. Get its clamp pin back from Abe Haskett, its brakeman, stoke the boiler from the quarry's "
                     "coal, and Hiram will take you up the mountain, to the Providence."),
         # (WinchCrank and CoalSack went with the crank and coal fetches, 2026-10-21: never reuse their ids.)
         items=[# Abe Haskett carries it, the lift crew's Restless brakeman (the dead miner the mountain's script tags
                # AbeHaskett): it falls where he does.
                dict(item_id='ClampPin', name='The clamp pin',
                     line="The pin that locks the lift's cable at the bottom sheave, on a cord. Abe Haskett wore it to the "
                          "end.",
                     mesh=PROPS + 'SM_ClampPin', dropped_by_tag='AbeHaskett')],
         # Through the speaking tube at the lift head: from now on the lift runs both ways.
         turn_in=dict(speaker='Speaker_LiftTube', giver='Hiram, by the speaking tube', lines=[
             ('', "Hear that? She still runs."),
             ('', "There's a carbine in the head's tool box. Company's. They'll never miss it."),
         ]),
         # A main mission's share since 2026-10-27: about half a level where it's met (Docs/Progression.md).
         # An Epic rifle (was Rare: the RR tuning the user took, 2026-10-29 night).
         rewards=dict(experience=5000, gun=True, gun_rarity_floor='EPIC', gun_kind=RIFLE, silver=40),
         expect=['MissionCollectItemObjective',
                 ['MissionLastingInteractObjective', 'MissionLastingInteractObjective'],
                 'MissionDefendObjective', 'MissionBoardObjective']),

    # --- After Dead Lift: Calder's Post (side) and the Providence (main since 2026-10-27) ---
    dict(asset='DA_Mission_LastWatch', id='LastWatch', title='The Last Watch',
         summary="Calder's Post on the Rim stands empty: Ruth Calder rode south after the Dunne Gang. Light the watch fire "
                 "so the posts down the Rim know Larkspur stands.",
         kind='SIDE', start='OFFERED', area='RansomsRest', prerequisites=['DeadLift'], sort_order=32, steps=last_watch_steps,
         offer=dict(giver_tag=POST_BOARD, giver="Calder's Post's board", briefing=(
             "RIM RANGERS, LARKSPUR POST. Post unmanned: gone after the Dunne Gang's trail. Any Ranger riding through, "
             "light the watch fire on the tower, so the posts down the Rim know Larkspur stands. If it stays dark they'll "
             "take the post for fallen and pull back. \u2014 Capt. R. Calder")),
         items=[dict(item_id='LampOil', name='Lamp oil',
                     line="A can of Ruth Calder's lamp oil, for the watch fire on the tower.",
                     mesh=PROPS + 'SM_LampOil')],
         # Signed off at the post's board: its words play as the board's screen closes (ANoticeBoard::TurnIn). Calder's
         # fourth cache, the Supply Crate in the cabin, opens with it (the placing script's).
         turn_in=dict(speaker=POST_BOARD, giver="the post's board", lines=[
             (ELLIS, "Post held, Captain. Wherever you're riding."),
             (HOB, "Larkspur stands, sunshine. On a dead outlaw's word."),
         ]),
         rewards=dict(experience=900, silver=60),
         expect=['MissionClearObjective', ['MissionCollectItemObjective', 'MissionEventObjective'], 'MissionInteractObjective',
                 'MissionDefendObjective', ['MissionEventObjective', 'MissionEventObjective']]),
    dict(asset='DA_Mission_BackPay', id='BackPay', title='Back Pay',
         summary="Eleven brass tags still hang on the IN side of the Providence's tag board. Go down into the mine and find "
                 "out why eleven men never came up, and what still works the dark.",
         # A main mission since 2026-10-27 (the user's note 32): Cyrus Teague, the Foreman, is the story's second boss, before
         # Abel at the boards. Met first at the lift head's speaking tube, where Dead Lift is turned in, so Hiram's words
         # about Teague follow its turn-in as one talk and start it (no card); his are who Teague is (the payroll book says
         # what he did), Hob's the push. No. 7's tag (Artifact_MinersTag) offers nothing now and stays hung on IN.
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['DeadLift'], sort_order=7, steps=back_pay_steps,
         offer=dict(giver_tag='Speaker_LiftTube', giver='Hiram Coyle', meet_first=True, lines=[
             ('', "Providence is up the Bench. Cyrus Teague ran her: foreman, company man to the bone."),
             ('', "Eleven men barricaded the lower drift for three months' pay. Then the roof came down on them."),
             ('', "Teague swore it was the roof. He's down there yet, digging at the fall."),
         ], accept_lines=[
             (HOB, "A foreman who never paid his men. Go and collect, sunshine."),
         ], briefing=(
             "No. 7. J. ODELL. IN. Eleven brass tags still hang on the IN side of the Providence's tag board. A miner hung "
             "his tag there going down and took it off coming up. These eleven never came up, and Cyrus Teague, the "
             "Foreman, has worked the dark galleries ever since.")),
         items=[dict(item_id='Payroll', name='The Providence payroll',
                     line="The company's strongbox: three months' wages for eleven men, never paid out.",
                     mesh=PROPS + 'SM_PayrollChest')],
         # The shift's voice out of the dark at the tag board, then the choice; the chosen one's lines, flag and rewards.
         turn_in=dict(speaker='Speaker_TagBoard', giver='the tag board', lines=[
             ('', "Who's at the board? ...Shift's not over."),
             (ELLIS, "It is. I've got your tags. And Teague's payroll."),
             ('', "Three months owed. Eleven men. Pay us, and we come up."),
         ], question="The company's silver is in your hands, and eleven tags still hang on IN.", choices=[
             dict(title='Pay the eleven', line="Hang their tags on OUT with their wages. The shift ends, and the "
                                               "Providence's dead rest.",
                  flag='BackPay.Paid', stamp='PAID', lines=[
                      ('', "Paid in full. Shift's over, boys."),
                      (HOB, "First honest pay this mine ever made."),
                      (HOB, BOARDS_NEXT),
                  # Epic (was Rare: the RR tuning the user took, 2026-10-29 night).
                  ], gun=True, gun_rarity_floor='EPIC'),
             dict(title='Keep the payroll', line="Leave the tags on IN and pocket the company's silver. The dead go on "
                                                 "working the dark.",
                  flag='BackPay.Kept', stamp='KEPT', lines=[
                      ('', "...Company man after all."),
                      (HOB, "Can't spend it in the ground, sunshine."),
                      (HOB, BOARDS_NEXT),
                  ], silver=400),
         ]),
         # A main mission's share since 2026-10-27: about half a level where it's met, the story's last before the boards.
         rewards=dict(experience=6500),
         expect=['MissionReachObjective', 'MissionClearObjective', ['MissionEventObjective', 'MissionEventObjective'],
                 'MissionClearObjective', 'MissionEventObjective', 'MissionClearObjective', 'MissionCollectItemObjective']),
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
    place_set = editor_property(objective_object, 'place')
    if place_set is not None:
        words.append(f"the place tagged {place_set.get_editor_property('actor').get_editor_property('actor_tag')} within "
                     f"{place_set.get_editor_property('radius'):.0f} cm"
                     f"{'' if place_set.get_editor_property('ignore_height') else ', measured with its height'}")
    if editor_property(objective_object, 'waypoint') == unreal.MissionWaypoint.ACTOR:
        words.append(f"arrow on the actor tagged {objective_object.get_editor_property('waypoint_actor').get_editor_property('actor_tag')}")
    spawner = editor_property(objective_object, 'spawner_id')
    if spawner is not None:
        words.append(f'the encounter {spawner} cleared')
    speaker = editor_property(objective_object, 'speaker_tag')
    if speaker is not None:
        words.append(f'the speaker tagged {speaker}')
    carried = editor_property(objective_object, 'items')
    if carried:
        words.append(f"carrying {', '.join(str(each) for each in carried)}")
    event = editor_property(objective_object, 'event')
    if event is not None:
        words.append(f'the event {event}')
    seconds = editor_property(objective_object, 'hold_seconds')
    if seconds is not None:
        words.append(f'held {seconds:.0f} s')
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
    if spec['start'] == 'OFFERED' and (not offered.get('giver_tag') or not offered.get('briefing')):
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: it starts OFFERED but names no giver_tag or briefing: nothing was saved")
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
    choices = spec['turn_in'].get('choices', ())
    if choices and (spec['turn_in'].get('automatic') or not spec['turn_in'].get('speaker')):
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: a choice at the turn-in needs someone to turn it in to: nothing was saved")

    # A giver's talk can't also be the last objective: that talk is the turn-in (Looter.Missions.Definitions checks it).
    giver = spec['turn_in'].get('speaker', '')
    last = steps[-1].get_editor_property('objectives') if steps else []
    if giver and any(isinstance(each, unreal.MissionTalkObjective) and str(each.get_editor_property('speaker_tag')) == giver
                     for each in last):
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: its last step talks to {giver}, who it's turned in to: nothing was saved")

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
    unreal.log(f"SIDEMISSIONS {spec['asset']} {'made' if created else 'updated'}: id {spec['id']}, \"{spec['title']}\", "
               f"{spec['kind'].lower()}, offered by {offered.get('giver_tag')} after {', '.join(spec['prerequisites'])}, "
               f"area '{spec['area']}', {len(steps)} steps, {finish}; rewards {reward.get('experience', 0)} XP"
               f"{', a ' + reward.get('gun_rarity_floor', 'COMMON').title() + ' gun' if reward.get('gun') else ''}"
               f"{', ' + str(reward['silver']) + ' silver' if reward.get('silver') else ''}"
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
    # (the main ones are create_mission_assets.py's; the mountain's later ones follow Dead Lift, made here first).
    known = mission_ids()
    waiting = []
    for spec in MISSIONS:
        missing = [each for each in spec['prerequisites'] if each not in known]
        if missing:
            unreal.log_warning(f"SIDEMISSIONS {spec['asset']} waits for {', '.join(missing)}: nothing was made")
            waiting.append(spec['asset'])
            continue
        setup(spec)
        known.add(spec['id'])
    unreal.log('SIDEMISSIONS done' + (f"; waiting for their prerequisites: {', '.join(waiting)}" if waiting else ''))


run()
