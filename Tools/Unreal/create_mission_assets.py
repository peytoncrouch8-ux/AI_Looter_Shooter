"""Makes the mission data assets (UMissionDefinition, Missions/MissionDefinition.h) in /Game/Data/Missions and saves
them. Run it in the open editor once the C++ with UMissionDefinition is built, after create_story_lines.py:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_mission_assets.py"
It prints a MISSIONS line per mission and step, and "MISSIONS done" at the end. Running it again rebuilds these
missions (fields, steps, turn-in and rewards) from what's written here; other mission assets are left alone.
create_side_mission_assets.py makes the side missions the same way (run it after this one).

The story's missions are picked up from a giver and turned in, Borderlands' way (the user's calls of 2026-10-08 and
2026-10-13; Docs/Areas/RansomsRest_Expansion.md sections 2-3). Every one but Main 1 starts OFFERED: once what comes before
it is turned in, a ? floats over its giver (someone at a speaker point, a notice board or a found artifact), and talking
to them says the offer's lines and opens the offer card with its briefing. Once every objective is done the mission is
"ready to turn in" (a ! over whoever takes it, the tracker's "Turn in to <giver>"), and talking to them finishes it, with
its turn-in lines and its rewards; a mission they offer next is offered straight after, as one conversation. Where a
mission's last objective was talking to its giver, that talk is the turn-in. Since 2026-10-22 (the user's call) a main
mission is never offered on the card: the data still says OFFERED with its giver and lines, and the game starts it the
moment it's due, tracked, the offer's lines and accept_lines said on the move (Main 2, 4, 5, 7); one with
meet_first=True (Main 3 and Main 6: Delia's word) is listed as "Talk to Grandma Delia" until that talk starts it. Side
offers open the card at once beside the giver's lines; Accept goes to accept_lines. Experience is a fixed amount per mission,
on the Borderlands-like curve of 2026-10-13: a main mission about a third to two thirds of a level where it's turned in, a
side mission about an eighth to a fifth (Docs/Progression.md has the count: since 2026-10-27, with the mine in the story,
the story alone ends about level 6.9 and everything about 7.5; Looter.Progression.Pacing checks both).

The main path since 2026-10-27 (the user's note 32: "Cyrus Teague should be a part of the main story line. Abel should be
the final boss so that means Cyrus is the second boss fight"): Main 1-5, then Dead Lift and Back Pay (main missions now,
made by create_side_mission_assets.py beside the Last Watch that follows Dead Lift; ids unchanged), then Main 6 (Abel) and
Main 7. Main 5's turn-in says why the mine comes first and points at the lift; Main 6 waits for Back Pay.

  Skyreach's own missions (Docs/Polish/TutorialRework.md, 2026-10-08: no forced checklist; the town's notice board is the
  giver). None names anything of the story, and none gives experience: Skyreach's rewards are guns, handed out at the board.
  DA_Mission_Tutorial  "Welcome to Skyreach", the first goal: find a gun in town (carrying one; the arrow on the gun rack's
                       rifle), then read the notice board in the square (ANoticeBoard sends NoticeBoard.Read; the arrow on
                       the board). It must say what the tutorial director's built-in steps say (ATutorialDirector's
                       constructor): the director plays the asset, and Looter.Missions.TutorialMission compares the two
                       objective by objective. Started by the director (manual); automatic. Its id must stay "Tutorial".
  DA_Mission_WebHollow "Clear Web Hollow", the board's main posting, up once the first goal is done (automatic, after
                       Tutorial): the six spiders of Web Hollow, read from the world (UMissionClearZoneObjective). Turned in
                       at the notice board (Speaker_NoticeBoard) for a pump shotgun. Its id must stay "WebHollow".
  DA_Mission_RangePractice "Range Practice", optional: twelve hits on the target dummies under the windmill; automatic.
  DA_Mission_Wallow    "The Wallow", optional: the five slimes in the bog west of the range; turned in at the board for a
                       gun of Uncommon or better.
  DA_Mission_Lookout   "Up to the Lookout", optional: up the plateau ramp to the ruined lookout (Place_Lookout); automatic.
  DA_Mission_BoardSkiff "Board the Skiff", started by Skyreach's jetty (manual), finished by casting off (Board.Skiff).
  DA_Mission_Test      a small mission for trying the runner in a game (Looter.Mission.Start Test only); automatic.

  Ransom's Rest's story (steps counted from 0, as the build scripts' conditions count them; "+ optional" is an optional
  objective in that step, Docs/Areas/RansomsRest_Expansion.md section 3):
  DA_Mission_Main1 "Landfall" (the Wardens story since 2026-10-31, was "Seven Days"; the id stays Main1: saves, conditions
                   and tests key on it; Docs/Work/2026-10-31-Wardens/CONTRACT.md's step table), automatic on a new game,
                   on the Lamplighter's deck: 0 the Great Lantern at the prow (within 3.5 m of Lamplighter_Prow), 1 Vane's
                   boarders (event Landfall.DeckClear), 2 the two grapple lines cut (hold on Landfall_Grapple, 2), 3 the
                   hard landing (scene HardLanding), 4 the anchor (hold on Landfall_AnchorWinch) and the dead off the ledge
                   (LedgeClimbers, 4), 5 Ransom's Point (Place_RansomsPoint, height counted), 6 the spyglass
                   (Landfall_Spyglass: scene SmogReveal), 7 Abel's trail to the farm (within 18 m of Speaker_Delia) +
                   optional Abel's hat (Landfall_AbelsHat: its readable sets Landfall.AbelsHat), 8 Delia's porch held
                   (FarmPorch, 8). Turned in to Delia Ransom on her porch: no lines of its own, so her topic says it
                   (story_texts.TOPICS['Speaker_Delia']: DA_Lines_DeliaLandfallHat with the hat's flag, else
                   DA_Lines_DeliaLandfall). 450 XP, her purse (20). The porch plate is no step now: its jars are free after
                   Main 1 (story_texts.PORCH_PLATE).
  DA_Mission_Main2 "The Ferryman's Price" (the Wardens story since 2026-11-01, was "Shall We Talk Business?"; the id stays
                   Main2; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 4; build_area_main2.py places its pieces),
                   offered by Hob (Speaker_Hob) after Main 1, started on the move: 0 the keeper's path to Gravewind Point
                   (within 25 m of Place_GravewindPoint, so the cart's step begins as its crew comes into sight past the
                   cairns; BluffNest's spiders lie on the way at the Three Widows) + optional Keeper's Bridge
                   (Place_KeepersBridge), 1 the Company's hose-cart stopped (HoseCart.Stopped, ACompanyHoseCart's event
                   for its valve shut or its tank burst; the arrow on HoseCart_Boards) and its crew and spill fought off
                   (BoardsCrew, 8, the count hidden: the spill's four dead are the twist) + optional Vane's loudspeaker at
                   the Keeper's Gate silenced (Loudspeaker_KeepersGate, killed), 2 Mister Sexton at his ferry under the
                   boards (Speaker_Sexton, down the rope ladder), 3 the Warden's Log opened (it must stay step 3:
                   Ledger::HandedOverStep: Sexton's ledger pages go into it). Turned in to Sexton at his ferry (card,
                   then his price: the barge's manifest). 400 XP (550 until 2026-11-01: the Pacing band).
  DA_Mission_Main3 "Under New Management" (the Wardens story since 2026-11-01, was "Cold Welcome"; MAINS_2_7.md section 5;
                   build_area_main3.py), offered by Delia Ransom after Main 2 (met first: her talk starts it): 0 the farm
                   road to the town gate (Place_TownGate; Lucky Ned's skiff leaves the barge as it starts), 1 the Company's
                   checkpoint broken (TownGate, 7: three riflemen, a hose-man, three Unpaid), 2 Bright & Daughter (within
                   8 m of Speaker_Tilly) + optional the UNDER NEW MANAGEMENT banner cut down (MainStreet.BannerDown, its
                   two ropes shot). Turned in to Tilly Bright at her window (card, then her lines; DA_Lines_TillyMain3 the
                   same, her topic "during Main3 from step 3"): she hands Abel's lantern bail over (Main 6's item
                   LanternBail, carried from the turn-in: hand_items); the reward gun drops. 800 XP (1,000 until 2026-11-01: the Pacing band), an Uncommon gun or
                   better, her purse (40). Its id must stay "Main3" (Main Street's safe ground and shutters, Pruitt's
                   hatch follow it).
  DA_Mission_Main4 "Hallowed Ground", offered by Tilly after Main 3: 0 the chapel (Place_Chapel), 1 the chapel yard
                   (ChapelYard, 16), 2 the knock at the vestry door (Speaker_Aldana: "ring her bell"), 3 the bell held
                   (Bell_Chapel), 4 the Reliquary seen (GraveSight.Reliquary; its look_when "during Main4 from step 4") +
                   optional the keepers' roll (KeepersRoll). Turned in to Aldana: a keeper's lantern can find an ember, and
                   Pa's fell whole. 1,300 XP, a Saint Maud Uncommon gun with Hallow rounds, the poor box (40).
  DA_Mission_Main5 "The Keeper's Lantern", offered by Aldana after Main 4 (straight after its turn-in, one conversation):
                   0 into the Sink (Place_SinkFloor, height counted), 1 the three egg sacs, 2 the lantern taken (it must
                   stay step 2: AKeepersLantern::TakeStep), 3 out at the ramp head (Place_SinkRim) + optional the
                   sheriff's hat by the den (SheriffHat). Turned in to Aldana: the altar candle won't take; Delia will know
                   what's owed a keeper, but not before Cyrus Teague's dead are settled (at dusk they'd follow Ellis to the
                   boards); Hob points at the old lift. 3,000 XP, his purse (50).
  (Dead Lift, met at Hiram's stool, and Back Pay, met at the lift head's speaking tube: create_side_mission_assets.py.)
  DA_Mission_Main6 "The Gravewind", offered by Delia after Back Pay (accepting fades the Rest to dusk): 0 Gravewind Point
                   (Place_GravewindPoint) + optional by the keeper's path (Place_KeepersBridge) + Pa's lantern bail carried
                   (its item LanternBail, off Tilly's sill since Main 3), 1 the lantern on the keeper's post, 2 Abel,
                   3 sitting with Pa (Abel's steps: HangStep 1, FightStep 2, SceneStep 3). Finishes with the scene. 5,000 XP.
  DA_Mission_Main7 "The Lantern Leans", offered by Pa on his board (Speaker_Abel) after Main 6: 0 home to Delia (her step
                   must stay 0: ADoorHandoff) + optional eat what's on the porch plate (PorchPlate.Eaten). Turned in to
                   Tilly (her father's car). 4,500 XP, the Gilded Lily opened, Heirloom by Delia's hand, her purse (60).

The mission features (wave 2a, sections 3-4), each a helper below:
  - optional objectives: objective(..., optional=True) (or event_objective / read_objective / collect_items with
    optional=True), in a step beside what it asks for; each done pays rewards(optional_silver=10) at the turn-in.
  - mission items: the spec's 'items' (item(): id, name, line, mesh, the creature that drops it), counted carried by
    collect_items(asset, text, [ids]); placed in the level as AMissionItem (its item_id) by the level's build script
    (Tools/Unreal/story_texts.py ITEMS says where and from when).
  - readables and the porch plate: read_objective(asset, text, 'KeepersRoll') counts reading the AReadableProp whose
    readable_id is KeepersRoll (its words: story_texts.py READABLES); event_objective(asset, text, PORCH_EATEN) the porch
    plate's eating (Main 7; PORCH_TAKEN, its jars, was Seven Days' step and is no mission's since Landfall).
  - a turn-in with a choice of two: turn_in(..., choices=[choice(...), choice(...)], question=...), each choice with its
    flag (BackPay.Paid), stamp, lines and rewards (rewards()'s fields, in place of the mission's own).
  - a purse of its own: rewards(silver=...) (the old missions' purses stay kept by id in MissionRewards::SilverFor).

Objectives are instanced objects inside the asset, one class per kind (UMissionReachObjective, UMissionKillObjective, ...),
made with unreal.new_object(<class>, asset) and listed in each step's 'objectives'. Classes for actor filters are loaded
by their script path ('/Script/AI_Looter_Shooter.WeaponRack').
"""
import unreal

FOLDER = '/Game/Data/Missions'
CLASSES = '/Script/AI_Looter_Shooter.'
# Where the mission props' models are imported (Art/Models/Props/MissionProps.py through Tools/models.ps1).
PROPS = '/Game/Art/Props/'


def mission_types():
    """The reflected mission types, or a clear error when the C++ isn't built yet."""
    names = ['MissionDefinition', 'MissionStep', 'MissionRewards', 'MissionTurnIn', 'MissionOffer', 'StoryLine', 'MissionActorFilter',
             'MissionPlace', 'MissionKind', 'MissionStart', 'MissionWaypoint', 'MissionCollect', 'MissionPage',
             'MissionTravelObjective', 'MissionReachObjective', 'MissionCollectObjective', 'MissionHitObjective',
             'MissionKillObjective', 'MissionOpenPageObjective', 'MissionInteractObjective', 'MissionBoardObjective',
             'MissionSceneObjective', 'MissionTalkObjective', 'MissionClearObjective', 'MissionEventObjective',
             'MissionKillNamedObjective', 'MissionClearZoneObjective', 'WeaponRarity',
             # The mission features (wave 2a): a turn-in's choice, mission items and their objective.
             'MissionTurnInChoice', 'MissionItemInfo', 'MissionCollectItemObjective']
    missing = [name for name in names if getattr(unreal, name, None) is None]
    if missing:
        raise RuntimeError(f"unreal.{', unreal.'.join(missing)} missing: build the C++ with Missions/ first")


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


def place(cls_name=None, tag=None, location=(0.0, 0.0, 0.0), radius=500.0, ignore_height=True):
    """FMissionPlace: the nearest actor of a filter, or a spot, and how close counts as there."""
    where = unreal.MissionPlace()
    where.set_editor_property('actor', actor_filter(cls_name, tag))
    where.set_editor_property('location', unreal.Vector(*location))
    where.set_editor_property('radius', radius)
    where.set_editor_property('ignore_height', ignore_height)
    return where


def objective(asset, cls, text, waypoint=None, waypoint_class=None, waypoint_tag=None, pass_without_targets=False,
              show_count=True, short='', hint_action='', hint='', optional=False, **settings):
    """One objective, made inside the asset (its outer) so it's saved with it. With waypoint ACTOR, the arrow points at
    the nearest actor of waypoint_class and/or carrying waypoint_tag. text is the full sentence (the Missions page);
    short is the HUD mission tracker's line (empty: text), and hint_action/hint its key hint (a binding id such as
    Reload, Move for the movement keys, and what the key does; none when empty). optional: it never holds its step, stays
    doable until the step ends (the last step's until the turn-in), pays the mission's optional_silver at the turn-in and
    reads "Optional:" on the tracker and the Missions page (UMissionObjective::bOptional). Write the words without
    "Optional:", the game adds it; put it in a step beside what the step asks for, never alone."""
    made = unreal.new_object(cls, asset)
    made.set_editor_property('text', unreal.Text(text))
    made.set_editor_property('short_text', unreal.Text(short))
    made.set_editor_property('hint_action', unreal.Name(hint_action))
    made.set_editor_property('hint_text', unreal.Text(hint))
    if waypoint is not None:
        made.set_editor_property('waypoint', waypoint)
    if waypoint_class or waypoint_tag:
        made.set_editor_property('waypoint_actor', actor_filter(waypoint_class, waypoint_tag))
    made.set_editor_property('pass_without_targets', pass_without_targets)
    made.set_editor_property('show_count', show_count)
    made.set_editor_property('optional', optional)
    for name, value in settings.items():
        made.set_editor_property(name, value)
    return made


# ---------------------------------------------------------------------------
# The mission features (wave 2a, Docs/Areas/RansomsRest_Expansion.md sections 3-4): helpers the missions' data uses
# ---------------------------------------------------------------------------

# Events the game's things send (the C++ names): reading a readable (AReadableProp::EventFor), the porch plate's two uses
# (APorchPlate's TakenFlag and EatenFlag, the same names as their story flags).
PORCH_TAKEN = 'PorchPlate.Taken'
PORCH_EATEN = 'PorchPlate.Eaten'
# APorchPlate::PlateTag: the arrow's actor for the plate's step.
PORCH_TAG = 'PorchPlate'


def readable_event(readable_id):
    """'Readable.KeepersRoll': the event reading the AReadableProp whose readable_id is readable_id sends."""
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
    """An Event objective: done when the named event happens count times (a readable read, the porch plate eaten)."""
    return objective(asset, unreal.MissionEventObjective, text, optional=optional, show_count=count > 1,
                     event=unreal.Name(event), count=count, **kwargs)


def read_objective(asset, text, readable_id, optional=False, **kwargs):
    """Read an AReadableProp (its Readable.<id> event): the keepers' roll, the wages chalk, Ruth's journal. Its arrow is
    on the prop (every readable tags itself Readable_<id>, AReadableProp::TagFor)."""
    kwargs.setdefault('waypoint', unreal.MissionWaypoint.ACTOR)
    kwargs.setdefault('waypoint_tag', f'Readable_{readable_id}')
    return event_objective(asset, text, readable_event(readable_id), optional=optional, **kwargs)


def collect_items(asset, text, items, optional=False, **kwargs):
    """Have mission items (UMissionCollectItemObjective): done once every one of items (their ids, from this mission's
    'items') is carried; the arrow on the nearest lying in the level, else on the living creature carrying one."""
    return objective(asset, unreal.MissionCollectItemObjective, text, optional=optional, show_count=len(items) > 1,
                     items=[unreal.Name(each) for each in items], **kwargs)


def item(item_id, name, line='', mesh='', dropped_by_class=None, dropped_by_tag=None):
    """FMissionItemInfo: a thing that belongs to the mission. item_id is what the campaign keeps it by (never rename or
    reuse one; unique across missions); name and line are the Carried list's; mesh (an asset path, may not exist yet: the
    pickup then shows a stand-in) is its model for a pickup that names none and for a drop; dropped_by_class/_tag name the
    creature that carries it, which drops it at its body on dying (the Gravemother by class, Abe Haskett's crew by
    theirs)."""
    made = unreal.MissionItemInfo()
    made.set_editor_property('id', unreal.Name(item_id))
    made.set_editor_property('name', unreal.Text(name))
    made.set_editor_property('line', unreal.Text(line))
    if mesh:
        model = unreal.load_asset(mesh) if unreal.EditorAssetLibrary.does_asset_exist(mesh) else None
        if model is None:
            unreal.log_warning(f"MISSIONS item {item_id}: {mesh} isn't made yet: its pickup shows a stand-in until it is")
        else:
            made.set_editor_property('mesh', model)
    if dropped_by_class or dropped_by_tag:
        made.set_editor_property('dropped_by', actor_filter(dropped_by_class, dropped_by_tag))
    return made


def choice(title, line, flag='', stamp='', lines=(), **reward_settings):
    """FMissionTurnInChoice: one of a turn-in's two choices. title is its button's words ("Pay the eleven"), line what it
    means; flag the story flag it sets (BackPay.Paid; other code reads it); stamp the word slammed over the card ("PAID");
    lines what's said once it's chosen ((speaker, words); an empty speaker is the giver's point). reward_settings are
    rewards()'s: what it gives in place of the mission's own (experience left at 0 keeps the mission's)."""
    made = unreal.MissionTurnInChoice()
    made.set_editor_property('title', unreal.Text(title))
    made.set_editor_property('line', unreal.Text(line))
    made.set_editor_property('flag', unreal.Name(flag))
    made.set_editor_property('stamp', unreal.Text(stamp))
    made.set_editor_property('lines', story_lines(lines))
    made.set_editor_property('rewards', rewards(**reward_settings))
    return made


def step(*objectives):
    made = unreal.MissionStep()
    made.set_editor_property('objectives', list(objectives))
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


def rewards(experience=0, experience_share=0.0, unlock_areas=(), gun=False, gun_rarity_floor='COMMON', named_gun='',
            named_gun_by_hand=False, gun_kind='', silver=0, optional_silver=10, gun_maker='', gun_element=''):
    """FMissionRewards: a fixed amount of experience (Docs/Progression.md tunes each; or a share of a level's when it's 0),
    areas opened, a gun at the player's feet (at least as rare as the floor; of gun_kind, a weapon definition's asset
    path, or else one from the default loot table), and a named gun by id, dropped, or handed over in the story (by hand:
    listed, never dropped). silver: the purse at the turn-in, the same at any level (0: the giver's purse kept by mission
    id in MissionRewards::SilverFor, which the old missions use); optional_silver: what each optional objective done adds
    at the turn-in, at level 1 (it grows with the player's level as the economy does)."""
    made = unreal.MissionRewards()
    made.set_editor_property('silver', silver)
    made.set_editor_property('optional_silver', optional_silver)
    made.set_editor_property('experience', experience)
    made.set_editor_property('experience_share', experience_share)
    made.set_editor_property('unlock_areas', [unreal.Name(area) for area in unlock_areas])
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
    made.set_editor_property('named_gun_by_hand', named_gun_by_hand)
    return made


def turn_in(automatic=False, speaker='', giver='', lines=(), choices=(), question='', hand_items=(), why=''):
    """FMissionTurnIn: who the mission is turned in to once its objectives are done (their speaker tag and their name for
    "Turn in to <giver>"), and what they say then (lines of (speaker, words); an empty speaker is the giver's speaker
    point's name; none: what they'd say there anyway, for a talk the story already wrote). automatic: it finishes by
    itself after its last objective (no one to turn it in to). Anyone at a speaker point can be the giver: a door, a
    window, a talking coffin, a speaking tube, a tag board (ASpeakerPoint, its bMissionsOnly keeping it quiet otherwise).
    choices: none, or two choice()s: the talk says lines, then the card asks the player to hold one of the two down
    (question over them; empty: the card's own words); the mission finishes with that choice's rewards and flag. why: when
    the turn-in moves to someone the steps never named, why (the tracker reads "<why>: turn in to <giver>")."""
    if len(choices) not in (0, 2):
        raise RuntimeError(f'a turn-in takes no choices or two, not {len(choices)}: nothing was changed')
    made = unreal.MissionTurnIn()
    made.set_editor_property('automatic', automatic)
    made.set_editor_property('speaker_tag', unreal.Name(speaker))
    made.set_editor_property('giver_name', unreal.Text(giver))
    made.set_editor_property('lines', story_lines(lines))
    # A choice as a dict of choice()'s fields, or one made already.
    made.set_editor_property('choices', [choice(**each) if isinstance(each, dict) else each for each in choices])
    made.set_editor_property('choice_question', unreal.Text(question))
    if hand_items:
        # Carried from the turn-in on (a later mission's items: Tilly hands Pa's bail over at Main 3's). Guarded, so a game
        # module from before FMissionTurnIn::HandItems still makes the mission (the item then lies where it's placed).
        try:
            made.set_editor_property('hand_items', [unreal.Name(each) for each in hand_items])
        except Exception as error:
            unreal.log_warning(f'MISSIONS: no MissionTurnIn.hand_items ({error}): {", ".join(hand_items)} stay where '
                               'they lie (build the game module)')
    if why:
        # Guarded, as hand_items: a game module from before FMissionTurnIn::Why still makes the mission ("Turn in to <giver>").
        try:
            made.set_editor_property('why', unreal.Text(why))
        except Exception as error:
            unreal.log_warning(f'MISSIONS: no MissionTurnIn.why ({error}): the tracker says only "Turn in to {giver}" '
                               '(build the game module)')
    return made


def offer(giver_tag='', giver='', lines=(), briefing='', accept_lines=(), meet_first=False):
    """FMissionOffer: who offers a mission picked up from its giver (start='OFFERED'), by the tag their actor carries
    (Speaker_Delia, NoticeBoard_Rangers, Artifact_MinersTag), their name for people (empty: from the tag), what a speaker
    says as they offer it (lines of (speaker, words); an empty speaker is their speaker point's name; none: their own lines),
    what they say once it's taken (accept_lines: Accept on the card cuts the offer short and goes to these), and the offer
    card's story, two to four sentences (an artifact's own words when it's one). A main mission is never offered on the
    card (2026-10-22): it starts by itself as it's due, these lines said on the move, or with meet_first it's listed as
    "Talk to <giver>" and the talk starts it. Without a giver tag the offer is empty, as every mission that isn't offered
    keeps it."""
    made = unreal.MissionOffer()
    made.set_editor_property('giver_tag', unreal.Name(giver_tag))
    made.set_editor_property('giver_name', unreal.Text(giver))
    made.set_editor_property('lines', story_lines(lines))
    made.set_editor_property('accept_lines', story_lines(accept_lines))
    made.set_editor_property('meet_first', bool(meet_first))
    made.set_editor_property('briefing', unreal.Text(briefing))
    return made


def tutorial_steps(asset):
    """Skyreach's first goal, word for word and number for number as ATutorialDirector's built-in steps
    (TutorialDirectorMission.cpp makes the same objectives from them): the full sentences for the Missions page and the
    HUD tracker's short lines. No key hints on the tracker: the contextual hints (UControlHintSubsystem) teach the keys
    when they're needed."""
    waypoint = unreal.MissionWaypoint
    return [
        # Carrying a gun; the arrow on the rifle lying on the rack (the rack once it's taken), where the farm road leads.
        step(objective(asset, unreal.MissionCollectObjective,
                       'Find a gun in town. The gun rack in the square has a rifle on it.',
                       short='Find a gun in town',
                       waypoint=waypoint.ACTOR, waypoint_class='WeaponRack', what=unreal.MissionCollect.WEAPONS, count=1)),
        # The board says it was read (ANoticeBoard::ReadEvent); the arrow on it. One read, so no count.
        step(objective(asset, unreal.MissionEventObjective,
                       'Read the notice board in the town square. The town posts its odd jobs there.',
                       short='Read the notice board', show_count=False,
                       waypoint=waypoint.ACTOR, waypoint_class='NoticeBoard', event=unreal.Name('NoticeBoard.Read'), count=1)),
    ]


# Skyreach's postings are turned in at the square's notice board (ANoticeBoard carries the tag); the tracker says "Turn in
# to the notice board", its arrow on the board.
NOTICE_BOARD = dict(speaker='Speaker_NoticeBoard', giver='the notice board')
SHOTGUN = '/Game/Weapons/Data/DA_PumpShotgun'


def web_hollow_steps(asset):
    """The board's main posting: Web Hollow's six spiders (layout.json gameplay.creatures: six around (5800, 5600) within
    12.5 m, never coming back), counted from the world: any spider whose home is within 22 m of the hollow's middle."""
    return [
        step(objective(asset, unreal.MissionClearZoneObjective,
                       'Clear the six spiders out of Web Hollow, up the forest rise past the pond.',
                       short='Clear out the spiders',
                       target=actor_filter('SpiderCreature'), zone=place(location=(5800.0, 5600.0, 0.0), radius=2200.0),
                       count=6)),
    ]


def range_practice_steps(asset):
    """Twelve of the player's hits on the dummies under the windmill (they never stay down); the arrow on the range's
    middle. A level without dummies passes it."""
    waypoint = unreal.MissionWaypoint
    return [
        step(objective(asset, unreal.MissionHitObjective,
                       'Put twelve rounds into the target dummies under the windmill.',
                       short='Hit the target dummies', waypoint=waypoint.TARGETS_CENTER, pass_without_targets=True,
                       target=actor_filter('TargetDummy'), count=12, player_hits_only=True)),
    ]


def wallow_steps(asset):
    """The Wallow's five slimes (layout.json gameplay.creatures: around (2400, -5600) within 10 m, never coming back),
    counted from the world as Web Hollow's spiders are: any slime whose home is within 20 m of the bog's middle."""
    return [
        step(objective(asset, unreal.MissionClearZoneObjective,
                       'Clear the five slimes out of the Wallow, the bog west of the range.',
                       short='Clear out the slimes',
                       target=actor_filter('SlimeCreature'), zone=place(location=(2400.0, -5600.0, 0.0), radius=2000.0),
                       count=5)),
    ]


def lookout_steps(asset):
    """Up on the plateau by the ruined lookout: within 8 m of the marker build_area_board.py places at the lookout
    (Place_Lookout), on the map (nothing under the plateau is that close)."""
    return [
        step(objective(asset, unreal.MissionReachObjective,
                       'Climb the plateau ramp to the old lookout and take in the view.',
                       short='Climb to the lookout', place=place(tag='Place_Lookout', radius=800.0))),
    ]


def board_skiff_steps(asset):
    """Leaving Skyreach: board the skiff at the jetty. The arrow points at the jetty (ASkiffJetty); casting off from it
    sends Board.Skiff."""
    waypoint = unreal.MissionWaypoint
    return [
        step(objective(asset, unreal.MissionBoardObjective,
                       "Your skiff is in. Board it at the jetty past the lookout when you're ready to leave Skyreach.",
                       waypoint=waypoint.ACTOR, waypoint_class='SkiffJetty', vehicle=unreal.Name('Skiff'))),
    ]


# ---------------------------------------------------------------------------
# Ransom's Rest's story
# ---------------------------------------------------------------------------

ELLIS = 'Ellis'
HOB = 'Hob'

# Landfall's tags, event, scene and encounters (Docs/Work/2026-10-31-Wardens/CONTRACT.md's step table: exact). Who gives
# each: FLIGHT's ship and its pieces (the prow's marker, the grapples, the winch, the spyglass, the event, the scenes),
# WORLD's build_area_landfall.py (the spawners, the hat), CREW's Delia (Speaker_Delia); Place_RansomsPoint is today's.
LANDFALL_PROW = 'Lamplighter_Prow'
DECK_CLEAR = 'Landfall.DeckClear'
LANDFALL_GRAPPLE = 'Landfall_Grapple'
HARD_LANDING = 'HardLanding'
LANDFALL_WINCH = 'Landfall_AnchorWinch'
LEDGE_CLIMBERS = 'LedgeClimbers'
LANDFALL_SPYGLASS = 'Landfall_Spyglass'
LANDFALL_HAT = 'Landfall_AbelsHat'
FARM_PORCH = 'FarmPorch'
DELIA_TAG = 'Speaker_Delia'


def main1_steps(asset):
    """Main 1, "Landfall" (2026-10-31, the Wardens story): the Lamplighter sails in over Gravewind Canyon with the player on
    her deck; the Great Lantern at the prow (Finch: it's leaning hard; Vane breaks in on the horn as they arrive), Vane's
    boarders off the deck, the grapples cut under fire, the hard landing on the Mooring Ledge, the anchor dropped while the
    dead climb out of the canyon, the climb to Ransom's Point, the spyglass's reveal (the barge over the Sink, its hose, the
    smog; then the Lantern finds Abel's trail), the trail down the bluff path (and Abel's hat on it, if they pick it up), and
    Delia's porch held. Then it's turned in to Delia. Step indices are the contract's (FLIGHT's old-save move goes to step
    5; story_texts' conditions count them): never reorder. Every step's words say why."""
    waypoint = unreal.MissionWaypoint
    return [
        # 0. A walk to the prow, the first control lesson; the arrow on the marker FLIGHT puts at the Great Lantern.
        step(objective(asset, unreal.MissionReachObjective,
                       "Go to the Great Lantern at the prow. Brother Finch says it's leaning hard.",
                       short='Go to the Great Lantern at the prow', place=place(tag=LANDFALL_PROW, radius=350.0))),
        # 1. The boarders' two waves (FLIGHT sends the event once the last of them is down; the tender's own riflemen don't
        # count). No arrow: the fight is all round the player.
        step(event_objective(asset, "Fight off Vane's boarders before they take the ship.", DECK_CLEAR,
                             short="Fight off Vane's boarders", waypoint=waypoint.NONE)),
        # 2. Both grapples, each held (a tap doesn't cut), with the tender's riflemen still firing.
        step(objective(asset, unreal.MissionInteractObjective,
                       'Cut the grapple lines: hold {Interact} on each chain, or the tender tows the ship to the barge.',
                       short='Cut the grapple lines', target=actor_filter(tag=LANDFALL_GRAPPLE), count=2, hold=True)),
        # 3. The hard landing plays (FLIGHT's scene, about 5 s, skippable): the tracker barely shows it.
        step(objective(asset, unreal.MissionSceneObjective, 'Hold on: Captain Holloway is taking her down.',
                       short='Hold on', waypoint=waypoint.NONE, scene=unreal.Name(HARD_LANDING))),
        # 4. The anchor, held (about 2.5 s on FLIGHT's winch), while the dead climb up from the canyon (WORLD's spawner).
        step(objective(asset, unreal.MissionInteractObjective,
                       'Drop the anchor: hold {Interact} on the winch, or the ship slides off the ledge into the canyon.',
                       short='Drop the anchor', target=actor_filter(tag=LANDFALL_WINCH), count=1, hold=True),
             objective(asset, unreal.MissionClearObjective, 'Fight off the dead climbing out of the canyon.',
                       short='Fight off the dead', spawner_id=unreal.Name(LEDGE_CLIMBERS), count=4)),
        # 5. Height counted, as Main 2's climb is: the bluff path runs under the top's edge.
        step(objective(asset, unreal.MissionReachObjective,
                       "Climb to Ransom's Point. Captain Holloway wants eyes on the valley.",
                       short="Climb to Ransom's Point",
                       place=place(tag='Place_RansomsPoint', radius=1800.0, ignore_height=False))),
        # 6. The spyglass on the lookout (FLIGHT's: its use plays SmogReveal). A tap or a hold counts, as FLIGHT offers it.
        step(objective(asset, unreal.MissionInteractObjective,
                       'Look over the Rest through the spyglass on the lookout.',
                       short='Look over the Rest', target=actor_filter(tag=LANDFALL_SPYGLASS), count=1)),
        # 7. The trail's glow down the bluff path to the farm (WORLD's), the arrow on Delia; on the way, Abel's hat (an
        # AReadableProp tagged Landfall_AbelsHat: story_texts.READABLES AbelsHat, held; it sets Landfall.AbelsHat).
        step(objective(asset, unreal.MissionReachObjective, "Follow Abel's trail down the bluff path to the farm.",
                       short="Follow Abel's trail", place=place(tag=DELIA_TAG, radius=1800.0)),
             objective(asset, unreal.MissionInteractObjective, "Pick up Abel's hat on the bluff path.",
                       short="Pick up Abel's hat", target=actor_filter(tag=LANDFALL_HAT), count=1, optional=True)),
        # 8. Dredgers and the dead at Delia's steps (WORLD's spawner: 3 riflemen, 2 hose-men, 3 Unpaid); she fights from
        # the porch (CREW).
        step(objective(asset, unreal.MissionClearObjective, 'Help Delia hold the porch.',
                       short='Help Delia hold the porch', spawner_id=unreal.Name(FARM_PORCH), count=8)),
    ]


# Main 2's and Main 3's tags, encounters and events (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md sections 4-5; who gives
# each: build_area_main2.py and build_area_main3.py place them, ACompanyHoseCart sends HoseCart.Stopped, ACompanyBanner
# sends its DownEvent, ACompanyLoudspeaker dies by its tag). Place_GravewindPoint and Place_KeepersBridge are build_area_deck's
# and build_area_story's; Place_TownGate is build_area_story's.
SULLY = 'Sully'
SEXTON_TAG = 'Speaker_Sexton'
BOARDS_CART = 'HoseCart_Boards'
BOARDS_CREW = 'BoardsCrew'
CART_STOPPED = 'HoseCart.Stopped'
GATE_LOUDSPEAKER = 'Loudspeaker_KeepersGate'
TOWN_GATE = 'TownGate'
TILLY_TAG = 'Speaker_Tilly'
BANNER_TAG = 'Banner_MainStreet'
BANNER_DOWN = 'MainStreet.BannerDown'


def main2_steps(asset):
    """Main 2, "The Ferryman's Price" (the Wardens story, MAINS_2_7.md section 4): the keeper's path out to Gravewind Point
    (past the Three Widows' spiders, over Keeper's Bridge if Sully walks it Abel's way), the Company's hose-cart stopped on
    the burial boards and its crew fought off (the spill's dead the twist; Vane's loudspeaker at the Keeper's Gate shot out
    if Sully will), Mister Sexton's price heard at his ferry in the fog under the boards, the Warden's Log opened. The Log's
    step must stay the fourth (Ledger::HandedOverStep, counted from 0: 3), as Sexton's pages are in it from then. Every
    step's words say why."""
    waypoint = unreal.MissionWaypoint
    return [
        # 0. 25 m round the Keeper's Gate (its marker): the cart's step begins as its crew comes into sight past the last
        # cairn, so it is the step listening when the cart is stopped. The Three Widows' nest (BluffNest) lies on the way.
        step(objective(asset, unreal.MissionReachObjective,
                       "Follow the keeper's path to Gravewind Point. The Ferryman waits by the burial boards, and he says he "
                       "knows the barge.",
                       short="Follow the keeper's path to Gravewind Point",
                       place=place(tag='Place_GravewindPoint', radius=2500.0)),
             objective(asset, unreal.MissionReachObjective, "Cross Keeper's Bridge, the way Abel walked the dead every sundown.",
                       short="Cross Keeper's Bridge", place=place(tag='Place_KeepersBridge', radius=600.0), optional=True)),
        # 1. The cart, either way it's stopped (ACompanyHoseCart sends the event about itself); its crew and the four dead
        # its spill brings, counted from the spawner (the count hidden: the spill is the twist); the gate's loudspeaker.
        step(event_objective(asset, "Stop the Company's hose-cart: it's pumping the week's dead off the boards. Hold "
                                    "{Interact} on its valve, or burst its tank.", CART_STOPPED,
                             short='Stop the hose-cart', waypoint=waypoint.ACTOR, waypoint_tag=BOARDS_CART),
             objective(asset, unreal.MissionClearObjective, "Fight off the Company's crew on the boards, and whatever the "
                                                             "cart spills.",
                       short="Fight off the crew", spawner_id=unreal.Name(BOARDS_CREW), count=8, show_count=False),
             objective(asset, unreal.MissionKillNamedObjective, "Silence Vane's loudspeaker at the Keeper's Gate.",
                       short="Silence Vane's loudspeaker", actor_tag=unreal.Name(GATE_LOUDSPEAKER), player_kills_only=True,
                       optional=True)),
        # 2. Down the rope ladder off the deck's west end to his landing (AFerryLadder); his talk is the deal.
        step(objective(asset, unreal.MissionTalkObjective,
                       "Climb down to the Ferryman's landing under the boards and hear Mister Sexton's price.",
                       short='Talk to Mister Sexton', speaker_tag=unreal.Name(SEXTON_TAG))),
        # 3. {Page.Bestiary}: the tab's number key from the inventory's own order (MissionText), never a digit written in.
        step(objective(asset, unreal.MissionOpenPageObjective,
                       "Open the Warden's Log: {Inventory}, then {Page.Bestiary}. Sexton has written his ledger's pages into "
                       "it.",
                       short="Open the Warden's Log", page=unreal.MissionPage.BESTIARY)),
    ]


def main3_steps(asset):
    """Main 3, "Under New Management" (the Wardens story, MAINS_2_7.md section 5): the farm road into town (Lucky Ned's
    skiff leaving the barge over it), the Company's checkpoint at the town gate broken, Bright & Daughter reached down a
    Main Street the Company holds (its banner cut down if Sully will); then it's turned in at Tilly's window (card, then her
    lines): Abel's lantern wasn't on him, only its bail, which she hands over (Main 6's item), and the box of effects (the
    reward gun); Main 4 offered after it. The gate's fight counts from its spawner, which stands from the road (its men at
    their sandbags as the player comes up the orchard), so a rifleman shot from the road counts."""
    waypoint = unreal.MissionWaypoint
    return [
        step(objective(asset, unreal.MissionReachObjective,
                       "Take the farm road into town. Tilly Bright laid Abel out: if his lantern came home, she has it.",
                       short='Take the farm road into town', place=place(tag='Place_TownGate', radius=1000.0))),
        step(objective(asset, unreal.MissionClearObjective,
                       "Break the Company's checkpoint at the town gate. Nobody goes in or out while the town's dead are "
                       "assessed.",
                       short="Break the Company's checkpoint", spawner_id=unreal.Name(TOWN_GATE), count=7)),
        step(objective(asset, unreal.MissionReachObjective,
                       "Get to Bright & Daughter, Undertakers. The Company holds Main Street.",
                       short='Get to Bright & Daughter', place=place(tag=TILLY_TAG, radius=800.0)),
             event_objective(asset, "Cut down the Company's UNDER NEW MANAGEMENT banner over Main Street: shoot both its "
                                    "ropes.", BANNER_DOWN,
                             short='Cut down the banner', optional=True, waypoint=waypoint.ACTOR, waypoint_tag=BANNER_TAG)),
    ]


def main4_steps(asset):
    """Main 4, "Hallowed Ground": up to the chapel, the yard cleared, the knock at the vestry door (Aldana sends Ellis to
    the bell), the bell rung, the Reliquary seen (and the keepers' roll read, if Ellis does); then it's turned in at
    Aldana's vestry door, and Main 5 offered after it. The tags, the encounter's id and the event are the ones
    build_area_chapel.py and the C++ give them (AChapelBell::BellTag, AChapelReliquary::ReliquaryTag and SightEvent)."""
    waypoint = unreal.MissionWaypoint
    return [
        step(objective(asset, unreal.MissionReachObjective, 'Go up the north road to the Chapel of Saint Ada.',
                       place=place(tag='Place_Chapel', radius=1500.0))),
        # "12 Unpaid in two waves, and one Restless": 13, counted from the spawner.
        step(objective(asset, unreal.MissionClearObjective, 'Clear the chapel yard.',
                       spawner_id=unreal.Name('ChapelYard'), count=17)),
        step(objective(asset, unreal.MissionTalkObjective, 'Knock at the vestry door.',
                       speaker_tag=unreal.Name('Speaker_Aldana'))),
        step(objective(asset, unreal.MissionInteractObjective,
                       'Ring the chapel bell: hold {Interact} on the rope inside the door.',
                       target=actor_filter(tag='Bell_Chapel'), count=1, hold=True)),
        # Done as the flash ends, so the turn-in comes after the vision; the arrow on the Reliquary meanwhile.
        step(objective(asset, unreal.MissionEventObjective, "Examine Saint Ada's smashed Reliquary, where her light burned.",
                       waypoint=waypoint.ACTOR, waypoint_tag='Reliquary_Chapel', show_count=False,
                       event=unreal.Name('GraveSight.Reliquary'), count=1),
             read_objective(asset, "Read the keepers' roll in the nave.", 'KeepersRoll', optional=True)),
    ]


def main5_steps(asset):
    """Main 5, "The Keeper's Lantern": down into the Sink, its three egg sacs shot down, the Keeper's Lantern taken from the
    webbing, and out at the ramp head (the sheriff's hat by the den on the way, if Ellis looks). The tags and the event are
    the ones build_area_sink.py and the C++ give them (AEggSac::EggSacTag and BurstEvent, AKeepersLantern::LanternTag); the
    lantern's step must stay the third (AKeepersLantern::TakeStep, counted from 0: 2), as Ellis has it from the step
    after."""
    waypoint = unreal.MissionWaypoint
    return [
        # Height counted: the rim and the ramp's upper half are within a few metres of the floor's middle as the crow flies.
        step(objective(asset, unreal.MissionReachObjective, 'Climb down into the Sink.',
                       place=place(tag='Place_SinkFloor', radius=900.0, ignore_height=False))),
        # Each sac tells the missions as it lands burst; the arrow on the nearest one still up.
        step(objective(asset, unreal.MissionEventObjective, 'Shoot down the three egg sacs.',
                       waypoint=waypoint.ACTOR, waypoint_tag='EggSac', event=unreal.Name('EggSac.Burst'), count=3)),
        step(objective(asset, unreal.MissionInteractObjective, "Take the Keeper's Lantern from the webbing.",
                       target=actor_filter(tag='Lantern_Keeper'), count=1)),
        # The ramp head on the west rim, by the fence's gap; height counted, so the ramp just under it isn't out yet.
        step(objective(asset, unreal.MissionReachObjective, 'Climb out of the Sink.',
                       place=place(tag='Place_SinkRim', radius=600.0, ignore_height=False)),
             read_objective(asset, 'Look at the hat by the den.', 'SheriffHat', optional=True)),
    ]


def main6_steps(asset):
    """Main 6, "The Gravewind": Pa's lantern bail in Ellis's pocket (off Tilly's sill, where it lies from Main 3's end:
    most players took it then, and it counts at once), the lantern carried to Gravewind Point (by the keeper's path over
    Keeper's Bridge, if Ellis walks it the Ransoms' way) and hung on the keeper's post by its bail, Abel defeated, the scene
    after. The tags and the scene are the ones build_area_deck.py and the C++ give them (AKeeperLanternPost's
    KeepersPostTag, AAbelKeeper's BossTag, SitWithPa::SceneName); the steps' order is Abel's (HangStep 1, FightStep 2,
    SceneStep 3, counted from 0)."""
    waypoint = unreal.MissionWaypoint
    return [
        # Either road: the west road or the keeper's path; both end at the keeper's grave, and the cairns lead to the gate.
        # The bail third, so the point's objective keeps its place (0, 0) and the keeper's path its (0, 1).
        step(objective(asset, unreal.MissionReachObjective,
                       "Carry the lantern to Gravewind Point, by the west road or the keeper's path.",
                       place=place(tag='Place_GravewindPoint', radius=1000.0)),
             objective(asset, unreal.MissionReachObjective,
                       "Take the keeper's path over Keeper's Bridge, the way the Ransoms walked their dead.",
                       short="Take the keeper's path", place=place(tag='Place_KeepersBridge', radius=600.0), optional=True),
             collect_items(asset, "Bring Pa's lantern bail. Tilly handed it over.", ['LanternBail'],
                           short="Bring Pa's lantern bail")),
        step(objective(asset, unreal.MissionInteractObjective, "Hang the Keeper's Lantern on the keeper's post.",
                       target=actor_filter(tag='LanternPost_Keeper'), count=1)),
        # A boss falling off the deck is still beaten (any kill counts).
        step(objective(asset, unreal.MissionKillNamedObjective, 'Defeat Abel Ransom, the Keeper.',
                       actor_tag=unreal.Name('Boss_Abel'), player_kills_only=False)),
        step(objective(asset, unreal.MissionSceneObjective, 'Sit with Pa.',
                       waypoint=waypoint.NONE, scene=unreal.Name('SitWithPa'))),
    ]


def main7_steps(asset):
    """Main 7, "The Lantern Leans": home to Delia (Heirloom out through her door), and the biscuits on the porch plate
    eaten if Ellis will (Pa asked it; the plate's eat_when is "during Main7"); then it's turned in at Tilly's window, which
    Main Street passes on the way to the depot (her Main 7 lines: her father's car coupled up at the platform). The Lily
    opens on the station board as it's turned in, so the depot and the board come after it. Delia's step must stay the
    first (ADoorHandoff's Step 0), as Heirloom comes out as it ends; its optional stays doable until the turn-in."""
    waypoint = unreal.MissionWaypoint
    return [
        step(objective(asset, unreal.MissionTalkObjective, 'Go home to Delia.', speaker_tag=unreal.Name('Speaker_Delia')),
             event_objective(asset, "Eat what's on the porch plate.", PORCH_EATEN, optional=True,
                             waypoint=waypoint.ACTOR, waypoint_tag=PORCH_TAG)),
    ]


def test_steps(asset):
    return [
        step(objective(asset, unreal.MissionKillObjective, 'Kill two creatures',
                       target=actor_filter('CreatureBase'), count=2, player_kills_only=True)),
        step(objective(asset, unreal.MissionReachObjective, 'Go to the windmill',
                       place=place('Windmill', radius=1000.0))),
        step(objective(asset, unreal.MissionInteractObjective, 'Use two test markers (Looter.Mission.Event Interact MissionTest)',
                       target=actor_filter(tag='MissionTest'), count=2)),
    ]


MISSIONS = [
    # Skyreach's: the first goal, then the notice board's postings in the town's own voice (no story names), turned in at
    # the board when they pay. Their sort orders put Web Hollow first among them, so it's the one tracked as they go up.
    dict(asset='DA_Mission_Tutorial', id='Tutorial', title='Welcome to Skyreach',
         summary="Find yourself a gun, then see what the town's notice board has going.", kind='TUTORIAL', start='MANUAL',
         area='Skyreach', prerequisites=[], sort_order=0, steps=tutorial_steps, turn_in=dict(automatic=True),
         rewards=dict()),
    dict(asset='DA_Mission_WebHollow', id='WebHollow', title='Clear Web Hollow',
         summary="SPIDERS. A nest of them has taken Web Hollow, up the forest rise past the pond. Six at the last count, "
                 "every one the size of a dog. Clear them out and there's a pump shotgun waiting at this board. The skiff "
                 "won't cast off while they're about.",
         kind='TUTORIAL', start='AUTOMATIC', area='Skyreach', prerequisites=['Tutorial'], sort_order=1,
         steps=web_hollow_steps, turn_in=NOTICE_BOARD, rewards=dict(gun=True, gun_kind=SHOTGUN)),
    dict(asset='DA_Mission_RangePractice', id='RangePractice', title='Range Practice',
         summary="RANGE OPEN. The dummies under the windmill don't bite and they don't complain. Put some lead in them "
                 "before you try it on something that does.",
         kind='TUTORIAL', start='AUTOMATIC', area='Skyreach', prerequisites=['Tutorial'], sort_order=2,
         steps=range_practice_steps, turn_in=dict(automatic=True), rewards=dict()),
    dict(asset='DA_Mission_Wallow', id='Wallow', title='The Wallow',
         summary="SLIMES in the Wallow again, the bog west of the range. Five of them, all jelly and temper. Aim for the "
                 "dark lump inside. Clear the bog and there's a gun in it for you, better than the common sort.",
         kind='TUTORIAL', start='AUTOMATIC', area='Skyreach', prerequisites=['Tutorial'], sort_order=3,
         steps=wallow_steps, turn_in=NOTICE_BOARD, rewards=dict(gun=True, gun_rarity_floor='UNCOMMON')),
    dict(asset='DA_Mission_Lookout', id='Lookout', title='Up to the Lookout',
         summary="VIEW, FREE OF CHARGE. Take the path out of the square and up the ramp to the old lookout on the plateau. "
                 "You can see the whole island from up there, and the jetty where the skiff ties up.",
         kind='TUTORIAL', start='AUTOMATIC', area='Skyreach', prerequisites=['Tutorial'], sort_order=4,
         steps=lookout_steps, turn_in=dict(automatic=True), rewards=dict()),
    dict(asset='DA_Mission_BoardSkiff', id='BoardSkiff', title='Board the Skiff',
         summary="SKIFF SERVICE. The skiff at the jetty past the lookout sails whenever you're ready. Mind your step on the "
                 "plank. You can always come back for the practice.",
         kind='TUTORIAL', start='MANUAL', area='Skyreach', prerequisites=[], sort_order=10, steps=board_skiff_steps,
         turn_in=dict(automatic=True), rewards=dict()),
    dict(asset='DA_Mission_Test', id='Test', title='Test: Kill, Reach, Use',
         summary="A short mission for trying missions as data: kill, reach and interact objectives and an experience "
                 "reward. Only Looter.Mission.Start Test starts it.",
         kind='SIDE', start='MANUAL', area='', prerequisites=[], sort_order=100, steps=test_steps,
         turn_in=dict(automatic=True), rewards=dict(experience_share=0.1)),

    # Ransom's Rest's story. Each is picked up from its giver once the one before is turned in (Main 1 starts with a new
    # game), and turned in for a fixed amount of experience (Docs/Progression.md: the story alone ends about level 5.5).
    # Their purses are the givers' kept by id (MissionRewards::SilverFor: Main 1 20, Main 3 40, Main 4 40, Main 5 50,
    # Main 7 60).
    dict(asset='DA_Mission_Main1', id='Main1', title='Landfall',
         summary="Vane Consolidated's dredge barge has hung over Ransom's Rest for a week, pressing its dead into soul coal. "
                 "Bring the Lamplighter in under the smog, find what the Great Lantern is leaning toward, and learn what "
                 "became of the town's keeper.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=[], sort_order=1, steps=main1_steps,
         # No lines of its own (as Side 2's turn-in to Amos has none): what Delia says as it's turned in is her
         # topic at that moment (story_texts.TOPICS['Speaker_Delia'], "during Main1 from step 9"), so the hat's flag can
         # choose her words: DA_Lines_DeliaLandfallHat with Landfall.AbelsHat, else DA_Lines_DeliaLandfall. The turn-in
         # card (TURNIN) takes those lines before it turns the mission in. Her purse stays the kept 20 (SilverFor).
         turn_in=dict(speaker=DELIA_TAG, giver='Delia Ransom'),
         rewards=dict(experience=450)),
    dict(asset='DA_Mission_Main2', id='Main2', title="The Ferryman's Price",
         summary="Mister Sexton, the old Ferryman, says he can sell the Wardens a way at Vane's barge. He keeps his ferry in "
                 "the fog under Gravewind Point, and a Company crew is pumping the week's dead off the burial boards above "
                 "it.",
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['Main1'], sort_order=2, steps=main2_steps,
         # Hob on Delia's porch rail, as Landfall's turn-in ends: a main mission starts on the move, these said as it does.
         offer=dict(giver_tag='Speaker_Hob', giver='Hob', lines=[
             ('', "Man down on the burial boards wants a word, sunshine. Says he knows the barge."),
             (SULLY, 'Who is he?'),
             ('', "The Ferryman. I flew for him eighty years. Don't take his first price."),
         ], briefing="Mister Sexton, the old Ferryman, is waiting at the burial boards on Gravewind Point and says he can sell "
                     "a way at the barge. Hob flew for him once and says don't take his first price. Go by the keeper's "
                     "path, the way Abel walked the dead."),
         # At his ferry, card first, then his price (the barge's manifest: Paid on Delivery after Main 6) and his trade
         # (his secrets for silver: MAINS_2_7.md section 12, a later round). Then Delia's word starts Main 3.
         turn_in=dict(speaker=SEXTON_TAG, giver='Mister Sexton', lines=[
             ('', "My price: the barge's manifest. Every soul Vane burned, with my fare beside it."),
             (SULLY, "And if I don't bring it?"),
             ('', "Then you'll owe me, friend. Ask the crow how that goes."),
             ('', "Come back when you've silver. I know where things lie on the Rest."),
         ]),
         rewards=dict(experience=400)),
    dict(asset='DA_Mission_Main3', id='Main3', title='Under New Management',
         summary="Abel's soul has to ride home in his own keeper's lantern, and nobody has seen it since Wednesday. Tilly "
                 "Bright, the undertaker, laid out his body. Vane's Company has taken the town, and its men hold the gate.",
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['Main2'], sort_order=3, steps=main3_steps,
         # Sexton said "his own lantern": Delia's word, met first (listed as "Talk to Delia Ransom"), her talk starts it.
         offer=dict(giver_tag='Speaker_Delia', giver='Delia Ransom', meet_first=True, lines=[
             ('', "Tilly Bright laid my boy out. What the hose left of him."),
             ('', "If his lantern came home, she has it. Town's Company ground now. Mind yourself."),
         ], briefing="Abel's soul has to ride home in his own keeper's lantern, and nobody has seen it since Wednesday. Tilly "
                     "Bright, the undertaker, laid out his body. Vane's Company has taken the town, and its men hold the "
                     "gate."),
         # Card first, then her lines (DA_Lines_TillyMain3 says the same, her topic from step 3): the bail out of his fist
         # (Main 6's item, carried from the turn-in), the box of effects (the reward gun); then Tilly offers Main 4.
         turn_in=dict(speaker=TILLY_TAG, giver='Tilly Bright', lines=[
             ('', "A Warden. Lord. Come close, the whole street's watching."),
             ('', "His lantern wasn't on him. Only its bail, snapped off in his fist."),
             ('', "Couldn't open his hand till morning. Here. And the box: he'd want it used."),
         ], hand_items=['LanternBail']),
         rewards=dict(experience=800, gun=True, gun_rarity_floor='UNCOMMON')),
    dict(asset='DA_Mission_Main4', id='Main4', title='Hallowed Ground',
         summary="Father Aldana blessed Pa's lantern every spring and sat up with his body the night he died. He has barred "
                 "himself in the chapel's vestry since the dead came home, and the chapel yard is full of them.",
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['Main3'], sort_order=4, steps=main4_steps,
         offer=dict(giver_tag='Speaker_Tilly', giver='Tilly', lines=[
             ('', "Father Aldana blessed that lantern every spring. He's barred in the vestry. Knock loud."),
         ], briefing="Father Aldana blessed your father's lantern every spring and sat up with his body the night he died. "
                     "Since the dead came home he has barred himself in the chapel's vestry. The chapel yard is full of "
                     "them."),
         # The doc's words, word for word (DA_Lines_AldanaMain4 holds the same); his offer of Main 5 follows them as one
         # conversation.
         turn_in=dict(speaker='Speaker_Aldana', giver='Father Aldana', lines=[
             ('', "She didn't answer. Now you've seen why."),
             (ELLIS, 'How do I find them?'),
             ('', "A keeper's lantern can find an ember."),
             ('', "They shot the keeper first in every town, and smashed his lantern."),
             ('', "Your father's fell in the dark, whole."),
             ('', "Spiders hoard anything a saint has touched."),
             ('', "Look in the Sink."),
         ]),
         # A Saint Maud Uncommon with Hallow rounds, the chapel's gift for the yard (was nothing; the RR tuning the user
         # took, 2026-10-29 night: the dead ahead are weak to Hallow).
         rewards=dict(experience=1300, gun=True, gun_rarity_floor='UNCOMMON', gun_maker='SaintMaud', gun_element='SoulLight')),
    dict(asset='DA_Mission_Main5', id='Main5', title="The Keeper's Lantern",
         summary="The gang smashed the keeper's lantern in every town they robbed, but Pa's fell in the dark, whole. "
                 "Spiders hoard anything a saint has touched: it's down in the Sink, in the webbing, among their egg sacs.",
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['Main4'], sort_order=5, steps=main5_steps,
         # Said straight after Main 4's turn-in ("...Look in the Sink."), as one conversation.
         offer=dict(giver_tag='Speaker_Aldana', giver='Father Aldana', lines=[
             ('', "Bring it back. I'd stake the chapel on it."),
         ], briefing="The gang smashed the keeper's lantern in every town they robbed, but Pa's fell in the dark, whole. "
                     "Spiders hoard anything a saint has touched. Go down into the Sink and bring it back."),
         # The twist: the altar candle won't light it. Delia's word starts Main 6 (her offer), but the mine comes first since
         # 2026-10-27 (the user's note 32: Cyrus Teague the second boss): who he is is Hiram's to say (Back Pay's offer), why
         # first is Aldana's here (at dusk the dead come to the boards, and Teague's would come too), and Hob points at the
         # lift, where Dead Lift is met. Shared words: no "Pa", "Ellis" or "kid" (the hero lines' rule), so no new rows.
         turn_in=dict(speaker='Speaker_Aldana', giver='Father Aldana', lines=[
             ('', "Dark, but whole. Let me try the altar candle."),
             ('', "...It won't take. It's lit from the saint, and the saint is out."),
             (ELLIS, 'Then how do I light it?'),
             ('', "I don't know. Your grandmother will know what's owed a keeper."),
             ('', "But not yet. Cyrus Teague still drives his dead shift in the Providence, and at dusk they'd come to the "
                  "boards with you."),
             (HOB, "The old ore lift's at the quarry, sunshine. Its engineman never left."),
         ]),
         rewards=dict(experience=3000)),
    dict(asset='DA_Mission_Main6', id='Main6', title='The Gravewind',
         summary="At dusk Pa walks the burial boards on Gravewind Point, where the wind pours off the Rim. Hang the Keeper's "
                 "Lantern on his post and show him the way, even if he can't go.",
         # After the mine since 2026-10-27 (Back Pay: the Foreman, the second boss; Dead Lift sorts 6, Back Pay 7), so Abel is
         # the last. The Old Quarry's lift and the Providence are create_side_mission_assets.py's (their ids unchanged).
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['BackPay'], sort_order=8,
         # Aldana sends Ellis to Delia, by way of the mine: met first, her talk starts it (no card), and that fades the Rest
         # to dusk (AStoryLighting, during Main 6).
         offer=dict(giver_tag='Speaker_Delia', giver='Grandma Delia', meet_first=True, lines=[
             (ELLIS, "Father Aldana couldn't light it."),
             ('', "No. That was never a priest's to do."),
             ('', "Take him the lantern. Show him the way, even if he can't go."),
             ('', "He'll be on the boards at dusk. He won't know you, Ellis. Go anyway."),
         ], briefing="At dusk Pa walks the burial boards on Gravewind Point, where the wind pours off the Rim. Hang the "
                     "Keeper's Lantern on his post and show him the way, even if he can't go."),
         # Pa's lantern bail, put out on Tilly's sill at Main 3's turn-in: this mission's item, so it lies there from then
         # (story_texts.py ITEMS: shown after Main 3) and goes with the lantern onto the post as this one finishes.
         items=[dict(item_id='LanternBail', name="Pa's lantern bail",
                     line="The ring Pa's lantern hung by, snapped off in his fist. Tilly couldn't open his hand till "
                          "morning.",
                     mesh=PROPS + 'SM_LanternBail')],
         # It ends with sitting with Pa, its own hand-in: Pa on his board, the lantern lit and the train's steam all follow
         # Main 6 finished, so it finishes as that scene ends (and Pa offers Main 7 from his board).
         steps=main6_steps, turn_in=dict(automatic=True), rewards=dict(experience=5000)),
    dict(asset='DA_Mission_Main7', id='Main7', title='The Lantern Leans',
         summary="Pa lit the Keeper's Lantern, and its flame leans north-east, over the ridges, toward Lucky Ned Purcell "
                 "and the Gilded Lily. Go home to Delia first. Then Tilly's hearse car waits at the depot.",
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['Main6'], sort_order=9, steps=main7_steps,
         offer=dict(giver_tag='Speaker_Abel', giver='Pa', lines=[
             ('', "She leans north-east, El. Over the ridges. That's Ned Purcell."),
             ('', "Your grandmother's kept something back for you. Go home first."),
             ('', "Eat what's on her plate, even if you can't taste it."),
         ], briefing="Pa lit the Keeper's Lantern, and its flame leans north-east, over the ridges, toward Lucky Ned "
                     "Purcell and the Gilded Lily. Go home to Delia first: she has kept something back. Tilly's hearse "
                     "car waits at the depot."),
         # Turned in at Tilly's, not Delia's, whose step it was: the tracker says why (round 3 of 2026-10-23: no hint).
         turn_in=dict(speaker='Speaker_Tilly', giver='Tilly', why="Tilly is readying her hearse car: see her at her shop"),
         rewards=dict(experience=4500, unlock_areas=['GildedLily'], named_gun='Heirloom', named_gun_by_hand=True)),
]


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


def check_features(spec, asset, steps, items):
    """The mission features' rules, before anything is saved: item ids unique and named, collect objectives naming this
    mission's items, optional objectives never alone in a step, a choice only with a giver, an offered mission's card with
    words. Raises (nothing saved) when one is broken; logs what the mission has."""
    path = f"{FOLDER}/{spec['asset']}"
    ids = [str(each.get_editor_property('id')) for each in items]
    if any(not each or each == 'None' for each in ids) or len(set(ids)) != len(ids):
        raise RuntimeError(f'{path}: its items need ids, each its own ({ids}): nothing was saved')
    optional = 0
    for index, each_step in enumerate(steps):
        objectives = each_step.get_editor_property('objectives')
        flags = [bool(each.get_editor_property('optional')) for each in objectives if each]
        optional += sum(flags)
        if flags and all(flags):
            raise RuntimeError(f'{path}: step {index + 1} has only optional objectives (it would pass at once): nothing was saved')
        for each in objectives:
            if isinstance(each, unreal.MissionCollectItemObjective):
                missing = [str(name) for name in each.get_editor_property('items') if str(name) not in ids]
                if missing:
                    raise RuntimeError(f"{path}: step {index + 1} collects {', '.join(missing)}, which aren't its items: nothing was saved")
    choices = spec['turn_in'].get('choices', ())
    if choices and (spec['turn_in'].get('automatic') or not spec['turn_in'].get('speaker')):
        raise RuntimeError(f'{path}: a choice at the turn-in needs someone to turn it in to: nothing was saved')
    if spec['start'] == 'OFFERED' and not spec.get('offer', {}).get('briefing'):
        raise RuntimeError(f'{path}: an offered mission needs its card\'s briefing: nothing was saved')
    if optional or items or choices:
        unreal.log(f"MISSIONS {spec['asset']}: {optional} optional objective{'' if optional == 1 else 's'}, "
                   f"{len(items)} item{'' if len(items) == 1 else 's'} ({', '.join(ids) or 'none'}), "
                   f"{'a choice of two at the turn-in' if choices else 'no choice'}")


def setup(spec):
    asset, created = load_or_create(spec['asset'], unreal.MissionDefinition)
    asset.set_editor_property('id', unreal.Name(spec['id']))
    asset.set_editor_property('title', unreal.Text(spec['title']))
    asset.set_editor_property('summary', unreal.Text(spec['summary']))
    asset.set_editor_property('kind', getattr(unreal.MissionKind, spec['kind']))
    asset.set_editor_property('start', getattr(unreal.MissionStart, spec['start']))
    # What starts an ON_EVENT mission; the others have none (Main 6 is offered by Delia now, no longer started by her word).
    asset.set_editor_property('start_event', unreal.Name(spec.get('start_event', '')))
    # Who offers an OFFERED mission (spec 'offer': offer()'s fields); every other mission's offer is cleared.
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
    steps = spec['steps'](asset)
    asset.set_editor_property('steps', steps)
    asset.set_editor_property('turn_in', turn_in(**spec['turn_in']))
    asset.set_editor_property('rewards', rewards(**spec['rewards']))
    check_features(spec, asset, steps, items)

    # A giver's talk can't also be the last objective: that talk is the turn-in (Looter.Missions.Definitions checks it).
    giver = spec['turn_in'].get('speaker', '')
    last = steps[-1].get_editor_property('objectives') if steps else []
    if giver and any(isinstance(each, unreal.MissionTalkObjective) and str(each.get_editor_property('speaker_tag')) == giver
                     for each in last):
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: its last step talks to {giver}, who it's turned in to: nothing was saved")

    # Read back what the asset holds before saving: every step with its objective, of the class asked for.
    stored = asset.get_editor_property('steps')
    if len(stored) != len(steps):
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: {len(stored)} steps stored of {len(steps)}: nothing was saved")
    for index, stored_step in enumerate(stored):
        objectives = stored_step.get_editor_property('objectives')
        if not objectives or any(each is None for each in objectives):
            raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1} lost its objectives: nothing was saved")
        for each in objectives:
            if each.get_outer() != asset:
                raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1}'s {each.get_name()} lives outside the asset: nothing was saved")
            unreal.log(f"MISSIONS {spec['asset']} step {index + 1}: {each.get_class().get_name()}"
                       f"{' (optional)' if each.get_editor_property('optional') else ''} \"{each.get_editor_property('text')}\"")
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"{FOLDER}/{spec['asset']} could not be saved")
    finish = ('finishes by itself' if spec['turn_in'].get('automatic')
              else f"turned in to {spec['turn_in'].get('giver')} ({giver})")
    start = (f"offered by {offered.get('giver')} ({offered.get('giver_tag')})" if spec['start'] == 'OFFERED'
             else f"starts {spec['start'].lower()}")
    if spec['start'] == 'OFFERED' and spec['kind'] == 'MAIN':
        start = (f"met first at {offered.get('giver')} ({offered.get('giver_tag')})" if offered.get('meet_first')
                 else f"starts by itself, {offered.get('giver')}'s words on the move")
    unreal.log(f"MISSIONS {spec['asset']} {'made' if created else 'updated'}: id {spec['id']}, \"{spec['title']}\", "
               f"{spec['kind'].lower()}, {start}, area '{spec['area']}', {len(steps)} steps, "
               f"{finish}, {spec['rewards'].get('experience', 0)} XP")


def run():
    mission_types()
    for spec in MISSIONS:
        setup(spec)
    unreal.log('MISSIONS done')


run()
