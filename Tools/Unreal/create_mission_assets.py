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
                   same, her topic "during Main3 from step 3"): she hands Abel's lantern bail over (Main 5's item
                   LanternBail, carried from the turn-in: hand_items); the reward gun drops. 800 XP (1,000 until 2026-11-01: the Pacing band), an Uncommon gun or
                   better, her purse (40). Its id must stay "Main3" (Main Street's safe ground and shutters, Pruitt's
                   hatch follow it).
  DA_Mission_Main4 "A Coal Under Ash" (the Wardens story since 2026-11-01, was "Hallowed Ground"; the id stays Main4;
                   MAINS_2_7.md section 6, lane MAIN45: build_area_chapel.py and build_area_main4.py place its pieces),
                   offered by Tilly after Main 3, started on the move: 0 the chapel (Place_Chapel), 1 the chapel yard
                   (ChapelYard, 16: eight Unpaid, then two hose-men, five Unpaid and the Leftover) + the Company's hose
                   pulled out of the side window (Hose_ChapelWindow, held; or its cart stopped), 2 the knock at the
                   vestry door (Speaker_Aldana: "ring her bell"), 3 the bell held (Bell_Chapel), 4 the Reliquary seen
                   (GraveSight.Reliquary: her ember flares once; its look_when "during Main4 from step 4") + the barge's
                   answer held off (BellAnswered, 6: a tender's drop on ropes) + optional the keepers' roll (KeepersRoll).
                   Turned in to Aldana (card, then his words: she's going out; every grave open to the hose; Ned threw the
                   lantern down the Sink). 1,000 XP, a Saint Maud Uncommon gun with Hallow rounds, the poor box (40).
                   Brother Finch stands at the Reliquary from its turn-in.
  DA_Mission_Main5 "Choke Point" (since 2026-11-01, was "The Keeper's Lantern"; MAINS_2_7.md section 7; build_area_sink.py
                   and build_area_main5.py), offered by Aldana after Main 4 (straight after its turn-in, one conversation;
                   Junie and Hob join in), started on the move: 0 Junie's salt charges (SaltCharges, in Hob's satchel at
                   the ramp head) + into the Sink (Place_SinkFloor, height counted), 1 the three egg sacs, 2 Abel's lantern
                   taken (it must stay step 2: AKeepersLantern::TakeStep), 3 three charges set on the intake's collar
                   (Intake_Charge, 3 holds: the set piece; the hose blows, Intake.Choked), 4 out at the ramp head
                   (Place_SinkRim) + optional the sheriff's hat by the den (SheriffHat). Turned in to Brother Finch at
                   the Reliquary (Speaker_FinchChapel; card, then his words, Junie's and Hob's: the second hose is down
                   the mine's air shaft, the old lift at the quarry). 1,400 XP, his purse (50). SmogThin from its turn-in.
  (Dead Lift, met at Hiram's stool, and Back Pay, met at the lift head's speaking tube: create_side_mission_assets.py.)
  DA_Mission_Main6 "The Pilot Light" (since 2026-11-01, was "The Gravewind"; MAINS_2_7.md section 10, lane MAIN67:
                   main67_src/CONTRACT67.md's step table; build_area_main6.py and build_area_main6_sortie.py place its
                   pieces), offered by Captain Holloway at the Lamplighter's wheel after Back Pay (met first: her talk starts
                   it and fades the Rest to dusk): 0 cast off (Landfall_AnchorWinch, held), 1 the Company's tenders fought
                   off (Pilot.TendersDown), 2 the barge grappled (Lamplighter_GrappleLauncher, 2 holds), 3 her top deck
                   (BargeDeck, 9) and the furnace hatch (Barge_FurnaceHatch) + optional the hose drum jammed (Barge_HoseDrum,
                   held: Pilot.DrumJammed) + optional her four horns (Barge_Horn), 4 the manifest (Item_BargeManifest: Paid on
                   Delivery's BargeManifest, carried from here), 5 the three draft valves (Furnace_DraftValve, held) and the
                   Chief Stoker's crew (FurnaceHall, 8), 6 the soul cage (Furnace_SoulCage, held; scene CageBurst), 7 the jump
                   to her deck (Lamplighter_Deck) and the barge's fall (scene BargeFall). Turned in to Holloway (card, then
                   hers, Finch's and Hob's words). 3,000 XP (5,000 until 2026-11-01: the Pacing band), 50 silver.
  DA_Mission_Main7 "Given to the Wind" (since 2026-11-01, was "The Lantern Leans"; MAINS_2_7.md section 11, lane MAIN67;
                   build_area_deck.py (the fight) and build_area_main7.py), offered by Holloway straight after Main 6's
                   turn-in, started on the move: 0 the burial boards (Place_GravewindPoint) + optional the souls in the
                   barge's coal freed (SoulCoal, 12 lasting holds) + optional the keeper's path, 1 Abel's lantern on the
                   keeper's post (AAbelKeeper::HangStep), 2 Abel freed (Boss_Abel; FightStep) + optional the six dead saved
                   (Abel.SixSaved), 3 Abel called home (scene FreeAbel; SceneStep), 4 his lantern carried to the chapel
                   (Place_Chapel; WestRoadCrew latch its light), 5 set on the Reliquary (Reliquary_Lantern, held; scene
                   SaintAdaWakes). Turned in to Delia at the chapel door (Speaker_DeliaChapel; card, then her topic's words).
                   3,000 XP (4,500 until 2026-11-01), the Gilded Lily opened, Heirloom by Delia's hand, her purse (60), the
                   hearse car coupled.

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


# Main 4's and Main 5's tags, encounters and events (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md sections 6-7, lane MAIN45;
# who gives each: build_area_chapel.py places the chapel's marker, the yard's set piece ChapelYard, the bell, the Reliquary
# and Aldana's door; build_area_main4.py the window's hose (AChapelWindowHose), the yard's cart, the tender and its drop
# BellAnswered, the lychgate's loudspeaker and Finch at the Reliquary; build_area_sink.py the egg sacs, the lantern and the
# floor's places; build_area_main5.py the satchel (Junie's charges), the intake (ASinkIntake, which sends Intake.Choked) and
# its three clamp spots (AIntakeCharge), the drops down the hose and the ramp's spiders).
ALDANA_TAG = 'Speaker_Aldana'
CHAPEL_HOSE = 'Hose_ChapelWindow'
YARD = 'ChapelYard'
BELL_ANSWERED = 'BellAnswered'
RELIQUARY_TAG = 'Reliquary_Chapel'
RELIQUARY_SIGHT = 'GraveSight.Reliquary'
FINCH_CHAPEL = 'Speaker_FinchChapel'
SALT_CHARGES = 'SaltCharges'
INTAKE_CHARGE = 'Intake_Charge'
INTAKE_TAG = 'Intake_Sink'
# The crew's speaker names, as their points name them (AWardenCrewMember::NameFor).
JUNIE = 'Junie Calloway'
FINCH = 'Brother Finch'


def main4_steps(asset):
    """Main 4, "A Coal Under Ash" (the Wardens story since 2026-11-01, was "Hallowed Ground"; MAINS_2_7.md section 6): up
    the north road to the chapel; its yard cleared of the dead the Company's crew spilled and its hose pulled out of the
    side window, where it drinks the saint; the knock at the vestry door (Aldana sends Sully to the bell); the bell rung;
    the Reliquary seen (Saint Ada's ember, a coal under ash, flares once: AChapelReliquary's flash) while the barge's answer,
    a tender's drop over the knoll, is fought off (and the keepers' roll read, if Sully will). Then it's turned in at
    Aldana's vestry door (card, then his words), and Main 5 offered after it. The Reliquary's look must stay step 4 (its
    look_when, build_area_chapel.py). Every step's words say why."""
    waypoint = unreal.MissionWaypoint
    return [
        step(objective(asset, unreal.MissionReachObjective,
                       "Go up the north road to the Chapel of Saint Ada. If her light still burns, Father Aldana will know.",
                       short='Go up to the Chapel of Saint Ada', place=place(tag='Place_Chapel', radius=1500.0))),
        # Saint Ada's Yard (16: eight Unpaid; then two hose-men, five Unpaid and the Leftover, counted from the set piece's
        # spawner) and the hose: a hold on it at the window, or its cart stopped (AChapelWindowHose tells the missions it's
        # out either way, as a held Interact on itself).
        step(objective(asset, unreal.MissionClearObjective,
                       "Clear the chapel yard of the dead the Company's crew spilled.",
                       short='Clear the chapel yard', spawner_id=unreal.Name(YARD), count=16),
             objective(asset, unreal.MissionInteractObjective,
                       "Pull the Company's hose out of the chapel's side window: it's drinking the saint. Hold {Interact} "
                       "on it, or stop its cart.",
                       short="Pull the hose out of the window", target=actor_filter(tag=CHAPEL_HOSE), count=1, hold=True)),
        step(objective(asset, unreal.MissionTalkObjective,
                       "Knock at the vestry door. Father Aldana has barred himself in since Wednesday.",
                       short='Knock at the vestry door', speaker_tag=unreal.Name(ALDANA_TAG))),
        step(objective(asset, unreal.MissionInteractObjective,
                       'Ring the chapel bell: hold {Interact} on the rope inside the door. If Saint Ada can answer, she will.',
                       short='Ring the chapel bell', target=actor_filter(tag='Bell_Chapel'), count=1, hold=True)),
        # Done as the flash ends (AChapelReliquary::SightEvent); the arrow on the Reliquary. The barge's answer, counted from
        # the tender's drop (four riflemen and two hose-men); the count shown: six on ropes.
        step(objective(asset, unreal.MissionEventObjective,
                       "Look at Saint Ada's Reliquary: see if her ember still burns.",
                       short="Look at Saint Ada's Reliquary", waypoint=waypoint.ACTOR, waypoint_tag=RELIQUARY_TAG,
                       show_count=False, event=unreal.Name(RELIQUARY_SIGHT), count=1),
             objective(asset, unreal.MissionClearObjective,
                       "Hold the chapel against the barge's answer: the Company is coming down on ropes.",
                       short='Hold the chapel', spawner_id=unreal.Name(BELL_ANSWERED), count=6),
             read_objective(asset, "Read the keepers' roll in the nave.", 'KeepersRoll', optional=True)),
    ]


def main5_steps(asset):
    """Main 5, "Choke Point" (the Wardens story since 2026-11-01, was "The Keeper's Lantern"; MAINS_2_7.md section 7): Junie's
    salt charges taken from Hob's satchel at the ramp head and down into the Sink; its three egg sacs shot down (the hoard
    behind them); Abel's lantern taken from the webbing; three charges set on the barge's intake collar against its draw
    while the Company comes down the hose and the spiders down the ramp (the set piece, "the Intake": the hose blows,
    swings and hangs limp, Intake.Choked); and out at the ramp head (the sheriff's hat by the den on the way, if Sully
    looks). Then it's turned in to Brother Finch at the Reliquary (card, then his words). The lantern's step must stay the
    third (AKeepersLantern::TakeStep, counted from 0: 2). Every step's words say why."""
    waypoint = unreal.MissionWaypoint
    return [
        # The satchel first (an item: the arrow on it at the ramp head), then the floor; height counted, as the rim and the
        # ramp's upper half are within a few metres of the floor's middle as the crow flies.
        step(collect_items(asset, "Take Junie's salt charges from Hob's satchel at the ramp head.", [SALT_CHARGES],
                           short="Take Junie's salt charges"),
             objective(asset, unreal.MissionReachObjective,
                       "Climb down into the Sink. Abel's lantern is down there, and so is the barge's mouth.",
                       short='Climb down into the Sink', place=place(tag='Place_SinkFloor', radius=900.0, ignore_height=False))),
        # Each sac tells the missions as it lands burst; the arrow on the nearest one still up.
        step(objective(asset, unreal.MissionEventObjective,
                       "Shoot down the three egg sacs. The spiders' hoard is behind them.",
                       short='Shoot down the egg sacs', waypoint=waypoint.ACTOR, waypoint_tag='EggSac',
                       event=unreal.Name('EggSac.Burst'), count=3)),
        step(objective(asset, unreal.MissionInteractObjective, "Take Abel's lantern from the webbing.",
                       short="Take Abel's lantern", target=actor_filter(tag='Lantern_Keeper'), count=1)),
        # Each clamp spot a hold (AIntakeCharge, 1.5 s); a set one can't be set again, so three holds are three spots.
        step(objective(asset, unreal.MissionInteractObjective,
                       "Choke the hose: set three salt charges on the intake's collar. Hold {Interact} at each clamp, and "
                       "lean against its pull.",
                       short='Set the salt charges on the intake', target=actor_filter(tag=INTAKE_CHARGE), count=3,
                       hold=True),
             # The blast itself (ASinkIntake::ChokedEvent, 5 s after the third charge lights the fuses): the step ends on
             # the choke, not on the last clamp, so "climb out" never shows while the fuses still burn.
             event_objective(asset, "Get clear of the intake's pull before the fuses blow.", 'Intake.Choked',
                             short='Get clear of the blast')),
        # The ramp head on the west rim, by the fence's gap; height counted, so the ramp just under it isn't out yet.
        step(objective(asset, unreal.MissionReachObjective, 'Climb out of the Sink before the barge finds its breath.',
                       short='Climb out of the Sink', place=place(tag='Place_SinkRim', radius=600.0, ignore_height=False)),
             read_objective(asset, 'Look at the hat by the den.', 'SheriffHat', optional=True)),
    ]


# Mains 6-7's tags, encounters, events and scenes (Docs/Work/2026-10-31-Wardens/main67_src/CONTRACT67.md, lane MAIN67; who
# gives each: the Lamplighter's sortie (APilotSortie, World/PilotSortieRules.h's PilotLight names) the winch's cast-off,
# Pilot.TendersDown, the two launchers and her deck's marker; the barge (ABargeBoarding, build_area_main6.py) BargeDeck, the
# hatch, the hose drum, the horns, the manifest (Paid on Delivery's item, picked up here), the valves, FurnaceHall, the cage
# and the scenes CageBurst and BargeFall; Abel's (AAbelKeeper, its corruption, build_area_deck.py) the keeper's post, the
# boss, Abel.SixSaved and FreeAbel; Main 7's world (build_area_main7.py) the soul coal, WestRoadCrew, the Reliquary's lantern
# rest, SaintAdaWakes and Delia at the chapel door).
HOLLOWAY = 'Captain Holloway'
HOLLOWAY_TAG = 'Speaker_Holloway'
DELIA_CHAPEL = 'Speaker_DeliaChapel'
LAUNCHER_TAG = 'Lamplighter_GrappleLauncher'
SHIP_DECK = 'Lamplighter_Deck'
TENDERS_DOWN = 'Pilot.TendersDown'
BARGE_DECK = 'BargeDeck'
FURNACE_HATCH = 'Barge_FurnaceHatch'
HOSE_DRUM = 'Barge_HoseDrum'
BARGE_HORN = 'Barge_Horn'
MANIFEST_TAG = 'Item_BargeManifest'
DRAFT_VALVE = 'Furnace_DraftValve'
FURNACE_HALL = 'FurnaceHall'
SOUL_CAGE = 'Furnace_SoulCage'
SOUL_COAL = 'SoulCoal'
SIX_SAVED = 'Abel.SixSaved'
RELIQUARY_LANTERN = 'Reliquary_Lantern'


def main6_steps(asset):
    """Main 6, "The Pilot Light" (the Wardens story since 2026-11-01, was "The Gravewind"; MAINS_2_7.md section 10, lane
    MAIN67): starved of both her hoses, Vane's barge comes down over the chapel at dusk to hose Saint Ada herself, the one
    time she's in reach. Cast off from the Mooring Ledge, fight off the Company's tenders in the air, grapple the barge with
    the Company's own launchers, fight across her top deck to the furnace hatch (her hose drum jammed and her horns shot out,
    if Sully will), take her manifest (Sexton's price: Paid on Delivery's item, carried from here), shut her furnace's three
    draft valves against the Chief Stoker, open the soul cage (what comes out isn't Abel: CageBurst), and get off her as she
    dies (BargeFall: down into Gravewind Canyon). Turned in to Holloway at the wheel as the Lamplighter flies home. The steps
    are the sortie's and the barge's (PilotLight's CastOffStep 0 .. EscapeStep 7): never reorder. Every step says why."""
    waypoint = unreal.MissionWaypoint
    return [
        # 0. Landfall's winch, reversed: its cast-off hold while she's anchored (ALandfallWinch's CastOffWhen).
        step(objective(asset, unreal.MissionInteractObjective,
                       "Cast off: hold {Interact} on the anchor winch. The barge is coming down over the chapel.",
                       short='Cast off the anchor', target=actor_filter(tag=LANDFALL_WINCH), count=1, hold=True)),
        # 1. The sortie sends it when both tenders are down and every boarder is dead; the arrow on the nearer tender is the
        # sortie's own (no waypoint: they move).
        step(event_objective(asset, "Fight off the Company's tenders. Shoot their gas-bags: they burn.", TENDERS_DOWN,
                             short="Fight off the Company's tenders", waypoint=waypoint.NONE)),
        # 2. The two launchers on her starboard rail, a hold each (ALamplighterLauncher): the Company's own grapples.
        step(objective(asset, unreal.MissionInteractObjective,
                       "Grapple the barge: hold {Interact} on both launchers on the starboard rail.",
                       short='Fire the grapple launchers', target=actor_filter(tag=LAUNCHER_TAG), count=2, hold=True)),
        # 3. The top deck: its fight counted from the spawner, the furnace hatch reached (height counted: not from the hall
        # under it); the drum's jam (sets Pilot.DrumJammed: Delia's turn-in knows it) and the horns, optional.
        step(objective(asset, unreal.MissionClearObjective,
                       "Fight across the barge's top deck. Mind the furnace vents.",
                       short="Fight across the barge's deck", spawner_id=unreal.Name(BARGE_DECK), count=9),
             objective(asset, unreal.MissionReachObjective, 'Get to the furnace hatch. Her pilot light burns below.',
                       short='Get to the furnace hatch', place=place(tag=FURNACE_HATCH, radius=300.0, ignore_height=False)),
             objective(asset, unreal.MissionInteractObjective,
                       "Jam the hose drum: hold {Interact} on its lever. Stop the hose short of the chapel roof.",
                       short='Jam the hose drum', target=actor_filter(tag=HOSE_DRUM), count=1, hold=True, optional=True),
             objective(asset, unreal.MissionKillObjective, "Silence the barge's horns.",
                       short="Silence the barge's horns", target=actor_filter(tag=BARGE_HORN), count=4,
                       player_kills_only=True, optional=True)),
        # 4. Sexton's price: an Interact on the AMissionItem (Paid on Delivery's BargeManifest; a collect objective here would
        # name another mission's item, which check_features refuses). Picking it up carries it into Paid on Delivery.
        step(objective(asset, unreal.MissionInteractObjective,
                       "Take the barge's manifest from the Company office. Sexton's price.",
                       short="Take the barge's manifest", target=actor_filter(tag=MANIFEST_TAG), count=1)),
        # 5. The set piece: three valves under the turning hot ring, while the Chief Stoker's crew comes in two pushes.
        step(objective(asset, unreal.MissionInteractObjective,
                       "Kill the furnace: hold {Interact} on its three draft valves.",
                       short="Shut the furnace's draft valves", target=actor_filter(tag=DRAFT_VALVE), count=3, hold=True),
             objective(asset, unreal.MissionClearObjective, 'Put down the Chief Stoker and his crew.',
                       short='Put down the stokers', spawner_id=unreal.Name(FURNACE_HALL), count=8)),
        # 6. The cage (a hold, only with Abel's lantern); the step ends as CageBurst has played, so "get off" never shows
        # under the twist.
        step(objective(asset, unreal.MissionInteractObjective,
                       "Open the soul cage: hold {Interact}. Abel's lantern is ready.",
                       short='Open the soul cage', target=actor_filter(tag=SOUL_CAGE), count=1, hold=True),
             objective(asset, unreal.MissionSceneObjective, "See what comes out of the cage.",
                       short='Abel', waypoint=waypoint.NONE, scene=unreal.Name('CageBurst'), show_count=False)),
        # 7. The jump onto her deck (height counted: the barge's deck is above it), then the barge's fall from it.
        step(objective(asset, unreal.MissionReachObjective, "Get off the barge! Jump for the Lamplighter's deck.",
                       short='Get off the barge!', place=place(tag=SHIP_DECK, radius=400.0, ignore_height=False)),
             objective(asset, unreal.MissionSceneObjective, 'Watch the barge go down.',
                       short='Watch her go', waypoint=waypoint.NONE, scene=unreal.Name('BargeFall'), show_count=False)),
    ]


def main7_steps(asset):
    """Main 7, "Given to the Wind" (the Wardens story since 2026-11-01, was "The Lantern Leans"; MAINS_2_7.md section 11,
    lane MAIN67): the furnace twisted Abel and he's drinking the six dead on the burial boards. To the Point (freeing the
    souls in the barge's fallen coal on the way, if Sully will), his own lantern hung on the keeper's post to call him, the
    fight to burn the smog off him (all six dead saved, if Sully can), his call home into his lantern (FreeAbel), the lantern
    carried home to the chapel under the Company's hoses, and set on Saint Ada's Reliquary (SaintAdaWakes). Turned in to
    Delia at the chapel door. Abel's step constants keep their numbers, read on Main7 now (AAbelKeeper::HangStep 1,
    FightStep 2, SceneStep 3); CarryStep 4 and ReliquaryStep 5 are World/CarriedLanternRules.h's. Never reorder."""
    waypoint = unreal.MissionWaypoint
    return [
        # 0. Either road ends at the keeper's grave; the coal lies along both (ASoulCoalLump, a 1 s hold each, used up for
        # good: the lasting objective counts what's freed before the step too).
        step(objective(asset, unreal.MissionReachObjective,
                       "Go to the burial boards on Gravewind Point. Abel came down there, among the six dead.",
                       short='Go to the burial boards', place=place(tag='Place_GravewindPoint', radius=1000.0)),
             objective(asset, unreal.MissionLastingInteractObjective,
                       "Free the souls in the barge's fallen coal: hold {Interact} on each lump.",
                       short='Free the souls in the coal', target=actor_filter(tag=SOUL_COAL), count=12, hold=True,
                       optional=True),
             objective(asset, unreal.MissionReachObjective,
                       "Take the keeper's path over Keeper's Bridge, the way the Ransoms walked their dead.",
                       short="Take the keeper's path", place=place(tag='Place_KeepersBridge', radius=600.0), optional=True)),
        # 1. (AAbelKeeper::HangStep) His own lantern on its post calls him up out of the canyon fog.
        step(objective(asset, unreal.MissionInteractObjective,
                       "Hang Abel's lantern on the keeper's post. It's the one thing of his he'll come to.",
                       short="Hang Abel's lantern on the keeper's post", target=actor_filter(tag='LanternPost_Keeper'),
                       count=1)),
        # 2. (FightStep) Any kill counts (a fall off the deck too); the six saved: his corruption sends Abel.SixSaved as the
        # fight is won with none of them drunk.
        step(objective(asset, unreal.MissionKillNamedObjective,
                       "Free Abel: burn the smog off him. When he falls to his knees, call him back at the keeper's post.",
                       short='Free Abel', actor_tag=unreal.Name('Boss_Abel'), player_kills_only=False),
             event_objective(asset, "Save all six of the dead on the boards: shoot the smog where it bites.", SIX_SAVED,
                             short='Save the six dead', optional=True, waypoint=waypoint.NONE)),
        # 3. (SceneStep) The last call is a 3 s hold on the post at zero; FreeAbel plays once it's held.
        step(objective(asset, unreal.MissionSceneObjective, "Call Abel home: hold {Interact} on the keeper's post.",
                       short='Call Abel home', waypoint=waypoint.ACTOR, waypoint_tag='LanternPost_Keeper',
                       scene=unreal.Name('FreeAbel'), show_count=False)),
        # 4. The lantern carried lit (UCarriedLanternSubsystem): WestRoadCrew's hose-men latch it, not Sully; nothing fails.
        step(objective(asset, unreal.MissionReachObjective,
                       "Carry Abel home to Saint Ada. Keep the Company's hoses off his light.",
                       short='Carry Abel home to Saint Ada', place=place(tag='Place_Chapel', radius=1500.0))),
        # 5. The lid's recess (AReliquaryLantern, a 3 s hold), then the relighting; the turn-in after it, at the door.
        step(objective(asset, unreal.MissionInteractObjective,
                       "Set Abel's lantern on Saint Ada's Reliquary: hold {Interact}.",
                       short="Set the lantern on the Reliquary", target=actor_filter(tag=RELIQUARY_LANTERN), count=1,
                       hold=True),
             objective(asset, unreal.MissionSceneObjective, 'Saint Ada wakes.', short='Saint Ada wakes',
                       waypoint=waypoint.NONE, scene=unreal.Name('SaintAdaWakes'), show_count=False)),
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
         # (Main 5's item, carried from the turn-in), the box of effects (the reward gun); then Tilly offers Main 4.
         turn_in=dict(speaker=TILLY_TAG, giver='Tilly Bright', lines=[
             ('', "A Warden. Lord. Come close, the whole street's watching."),
             ('', "His lantern wasn't on him. Only its bail, snapped off in his fist."),
             ('', "Couldn't open his hand till morning. Here. And the box: he'd want it used."),
         ], hand_items=['LanternBail']),
         rewards=dict(experience=800, gun=True, gun_rarity_floor='UNCOMMON')),
    dict(asset='DA_Mission_Main4', id='Main4', title='A Coal Under Ash',
         summary="A keeper's lantern is lit from his saint, and nobody has seen Saint Ada burn since Abel was taken. Father "
                 "Aldana has barred himself in the chapel's vestry, and a Company crew is in the chapel yard.",
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['Main3'], sort_order=4, steps=main4_steps,
         # Tilly at her window, straight after Main 3's turn-in (one conversation): a main mission starts on the move.
         offer=dict(giver_tag='Speaker_Tilly', giver='Tilly Bright', lines=[
             ('', "Father Aldana blessed Abel's lantern every spring. If Saint Ada's still lit, he'd know."),
             ('', "He's barred in the vestry since Wednesday. The Company's at the chapel. Knock loud."),
         ], briefing="A keeper's lantern is lit from his saint, and nobody has seen Saint Ada burn since Abel was taken. "
                     "Father Aldana has barred himself in the chapel's vestry. A Company crew is in the chapel yard."),
         # Card first, then the doc's words, word for word (DA_Lines_AldanaMain4 holds the same): the clock (she's going out),
         # the stakes (every grave open to the hose), Ned's luck and the Sink. His offer of Main 5 follows as one conversation.
         turn_in=dict(speaker=ALDANA_TAG, giver='Father Aldana', lines=[
             ('', "She's a coal under ash, Warden. Without her keeper she's going out."),
             ('', "When she does, every grave on the Rest is open to that hose."),
             ('', "I watched from the vestry. Ned Purcell wouldn't break Abel's lantern. Bad luck, he said."),
             ('', "He threw it down the Sink. Spiders hoard what a saint has touched."),
         ]),
         # A Saint Maud Uncommon with Hallow rounds, the chapel's gift for the yard (was nothing; the RR tuning the user
         # took, 2026-10-29 night: the dead ahead are weak to Hallow). His poor box stays the kept 40 (SilverFor).
         # 1,000 (was 1,300; 2026-11-01): the yard's two pushes and the tender's drop put eight Dredgers more on the way, so
         # the mission gives less to hold the Pacing bands (MAINS_2_7.md section 15: trim mission XP, not kills).
         rewards=dict(experience=1000, gun=True, gun_rarity_floor='UNCOMMON', gun_maker='SaintMaud', gun_element='SoulLight')),
    dict(asset='DA_Mission_Main5', id='Main5', title='Choke Point',
         summary="Ned threw Abel's lantern into the Sink, where the spiders hoard what a saint has touched. The barge's hose "
                 "drinks from the Sink's floor too. Junie says three salt charges on its intake collar will choke it.",
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['Main4'], sort_order=5, steps=main5_steps,
         # Said straight after Main 4's turn-in, as one conversation: Aldana's word, Junie on the tube (the Lamplighter's
         # speaking tube, a voice only), Hob's grumble. Started on the move.
         offer=dict(giver_tag=ALDANA_TAG, giver='Father Aldana', lines=[
             ('', "Bring up Abel's lantern. And that hose drinks from the Sink's floor."),
             (JUNIE, "Choke it? Salt round the intake collar and blow it. Hob's bringing you charges."),
             (HOB, "I'm a lookout, not a mule. ...Fine."),
         ], briefing="Ned threw Abel's lantern into the Sink, where the spiders hoard what a saint has touched. The barge's "
                     "hose drinks from the Sink's floor too. Junie says three salt charges on its intake collar will choke "
                     "it: starve the barge, Sexton said, and her crew will have to bring her down."),
         # Junie's charges: in Hob's satchel at the ramp head (story_texts.ITEMS SaltCharges), gone with the mission.
         items=[dict(item_id=SALT_CHARGES, name="Junie's salt charges",
                     line="Three bundles of salt jars wired to a fuse and a magnet clamp. Junie packed them. Hob carried "
                          "them, complaining.",
                     mesh='/Game/Art/Tripo/SM_SaltSatchel'),
                # Pa's lantern bail, put out on Tilly's sill at Main 3's turn-in: this mission's item (it goes on Abel's
                # lantern here; story_texts.py ITEMS: shown after Main 3).
                dict(item_id='LanternBail', name="Pa's lantern bail",
                     line="The ring Pa's lantern hung by, snapped off in his fist. Tilly couldn't open his hand till "
                          "morning.",
                     mesh=PROPS + 'SM_LanternBail')],
         # Card first, then: the lantern home, the sunrise it'll burn with, and where the barge drinks next (the tube, the
         # lift): Dead Lift is met at Hiram's stool. Finch was never named by its steps, so the tracker says why.
         turn_in=dict(speaker=FINCH_CHAPEL, giver=FINCH, why="Brother Finch keeps Saint Ada lit at the Reliquary", lines=[
             ('', "Abel's lantern. Whole. Ned's luck is ours, Warden."),
             ('', "It'll stay cold till his soul's in it. Then it'll burn like a sunrise."),
             (JUNIE, "That second hose is down the old mine's air shaft. Up the ridge."),
             (HOB, "Old ore lift at the quarry. Somebody dead's sat by it four years."),
         ]),
         # 1,400 (was 3,000; 2026-11-01): the Intake's two drops and the ramp's spiders (nine kills more, the Sink's 4-5)
         # pushed everything-once past 7.7 and the explorer over the Foreman's 6.5: trimmed with Main 4 and Paid on Delivery
         # (MAINS_2_7.md section 15: mission XP, not kills; Docs/Progression.md).
         rewards=dict(experience=1400)),
    dict(asset='DA_Mission_Main6', id='Main6', title='The Pilot Light',
         summary="Starved of both her hoses, Vane's barge is coming down over the chapel at dusk to hose Saint Ada herself. "
                 "The Lamplighter is patched. Board the barge, kill her furnace, and take Abel out of it.",
         # After the mine since 2026-10-27 (Back Pay: the Foreman, the second boss; Dead Lift sorts 6, Back Pay 7). The Old
         # Quarry's lift and the Providence are create_side_mission_assets.py's (their ids unchanged).
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['BackPay'], sort_order=8,
         # Holloway at the Lamplighter's wheel on the Mooring Ledge (the ship's crew member tagged Speaker_Holloway): met
         # first, her talk starts it (no card), and that fades the Rest to dusk (AStoryLighting during Main 6: DuskSmog).
         offer=dict(giver_tag=HOLLOWAY_TAG, giver=HOLLOWAY, meet_first=True, lines=[
             ('', "She's starving. She's coming down over the chapel to drink Saint Ada dry."),
             (JUNIE, "Bag's patched. Their grapples were still in my hull, so I re-rigged them. Waste not."),
             ('', "We go now, Warden. Bring Abel's lantern."),
         ], briefing="Starved of both her hoses, Vane's barge is coming down over the chapel at dusk to hose Saint Ada "
                     "herself. The Lamplighter is patched, and Junie has re-rigged the Company's own grapples. Board the "
                     "barge, kill her furnace, and take Abel out of it in his own lantern."),
         # Card first, then her words and the crew's, as the Lamplighter sets back down on the ledge (the sortie flies her
         # home after BargeFall). The manifest stays carried: it's Paid on Delivery's item. Main 7 follows (one conversation).
         turn_in=dict(speaker=HOLLOWAY_TAG, giver=HOLLOWAY, lines=[
             ('', "Eleven years since Vane's first barge took my town. That one's for them."),
             (FINCH, "That wasn't Abel, Warden. That was a week of furnace wearing him."),
             (FINCH, "He's gone to the boards. A keeper goes where his dead are."),
             (HOB, "Six of them laid out there, sunshine. And he's thirsty."),
         ]),
         # 3,000 (was 5,000; 2026-11-01, lane MAIN67): the barge's twenty Dredgers and the Hallowed Chief Stoker (about 3,600
         # kill XP at level 6-7) moved onto the story path, so the mission gives less to hold the Pacing bands (MAINS_2_7.md
         # section 15: trim mission XP, not kills; lanes/MAIN67.md "Pacing"). Its purse 50.
         steps=main6_steps, rewards=dict(experience=3000, silver=50)),
    dict(asset='DA_Mission_Main7', id='Main7', title='Given to the Wind',
         summary="The furnace twisted Abel's soul, and he fled to the burial boards on Gravewind Point, where six of the "
                 "Rest's dead lie unwalked. Burn the smog off him, call him back into his lantern, and take him home.",
         kind='MAIN', start='OFFERED', area='RansomsRest', prerequisites=['Main6'], sort_order=9, steps=main7_steps,
         # Holloway, straight after Main 6's turn-in (one conversation), Finch on the tube: started on the move.
         offer=dict(giver_tag=HOLLOWAY_TAG, giver=HOLLOWAY, lines=[
             (FINCH, "Burn the smog off him. Then hang his lantern where he can see it. He'll come."),
             ('', "Go, Warden. Before he drinks those six."),
         ], briefing="The furnace twisted Abel's soul, and when the cage broke he fled to the burial boards on Gravewind "
                     "Point, where six of the Rest's dead lie unwalked. Burn the smog off him and call him back into his own "
                     "lantern. Then take him home to Saint Ada before she goes out."),
         # Delia at the chapel door (a second placement, tagged Speaker_DeliaChapel, build_area_main7.py). No lines of its
         # own, as Landfall's turn-in: her topic says them (story_texts.TOPICS['Speaker_DeliaChapel']: the hose drum's flag
         # picks DA_Lines_DeliaMain7Jammed, else DA_Lines_DeliaMain7), card first. She hands Heirloom over (by hand: it
         # retires ADoorHandoff); the Lily opens on the station board and Tilly's hearse car is coupled. Its steps never named
         # her, so the tracker says why.
         turn_in=dict(speaker=DELIA_CHAPEL, giver='Delia Ransom', why="Delia has come up the hill to the chapel"),
         # 3,000 (was 4,500; lane MAIN67): Abel (2,066), his adds and the west road's crew moved here from Main 6 (Pacing).
         rewards=dict(experience=3000, silver=60, unlock_areas=['GildedLily'], named_gun='Heirloom',
                      named_gun_by_hand=True)),
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
