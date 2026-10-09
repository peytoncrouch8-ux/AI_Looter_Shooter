"""Makes the mission data assets (UMissionDefinition, Missions/MissionDefinition.h) in /Game/Data/Missions and saves
them. Run it in the open editor once the C++ with UMissionDefinition is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_mission_assets.py"
It prints a MISSIONS line per mission and step, and "MISSIONS done" at the end. Running it again rebuilds these
missions (fields, steps, turn-in and rewards) from what's written here; other mission assets are left alone.

The story's missions are turned in, Borderlands' way (FMissionTurnIn): once every objective is done the mission is
"ready to turn in" (the tracker says "Turn in to <giver>", the arrow on them), and talking to the giver at their speaker
point finishes it, with its rewards. Where a mission's last objective was talking to its giver, that talk is the turn-in
and the steps end one earlier; the ready state's step is that talk's old step, so the story's "from step N" conditions
for it (the giver's lines, Hob's perch) hold as before. Missions with no one to turn in to finish by themselves
(automatic). Experience is a fixed amount per mission (Docs/Progression.md has the count; Looter.Progression.Pacing
checks that the story ends about level 6-7).

  Skyreach's own missions (Docs/Polish/TutorialRework.md, 2026-10-08: no forced checklist; the town's notice board is the
  giver). None names anything of the story (Docs/Story.md, "The tutorial island, outside the story"), and none gives
  experience: Skyreach's rewards are guns, handed out at the board.
  DA_Mission_Tutorial  "Welcome to Skyreach", the first goal: find a gun in town (carrying one; the arrow on the gun rack's
                       rifle), then read the notice board in the square (ANoticeBoard sends NoticeBoard.Read; the arrow on
                       the board). It must say what the tutorial director's built-in steps say (ATutorialDirector's
                       constructor): the director plays the asset, and Looter.Missions.TutorialMission compares the two
                       objective by objective. Started by the director (manual); automatic. Its id must stay "Tutorial".
  DA_Mission_WebHollow "Clear Web Hollow", the board's main posting, up once the first goal is done (automatic, after
                       Tutorial): the six spiders of Web Hollow on the forest rise, read from the world (a zone around the
                       hollow, UMissionClearZoneObjective, so spiders killed before it went up count). Turned in at the
                       notice board (Speaker_NoticeBoard) for a pump shotgun. Turning it in ends the tutorial: the skiff's
                       gangplank comes down (ASkiffJetty::TutorialMissionId). Its id must stay "WebHollow".
  DA_Mission_RangePractice "Range Practice", optional: twelve of the player's hits on the target dummies under the windmill.
                       No reward, so it finishes on the spot (automatic) rather than send the player back to the board.
  DA_Mission_Wallow    "The Wallow", optional: the five slimes in the bog west of the range (a zone, as Web Hollow). Turned in
                       at the notice board for a gun of Uncommon or better.
  DA_Mission_Lookout   "Up to the Lookout", optional: up the plateau ramp to the ruined lookout (within 8 m of the marker
                       tagged Place_Lookout, which Tools/Unreal/build_area_board.py places). No reward: automatic.
  DA_Mission_BoardSkiff "Board the Skiff", leaving Skyreach: its own mission, which the board lists once it's up. Skyreach's
                       jetty (ASkiffJetty) starts it whenever the tutorial is done (Web Hollow turned in, or the island
                       skipped) and the player hasn't cast off for the first time, so it starts from code (manual) and asks
                       for nothing first. Its arrow is on the jetty; casting off sends Board.Skiff, which finishes it
                       (automatic). No reward.
  DA_Mission_Test      a small mission for trying the runner in a game: kill two creatures, reach the windmill, use two
                       things tagged MissionTest (no level has them: Looter.Mission.Event Interact MissionTest stands in
                       for the interaction component), for 10% of a level's experience. It starts only from the console
                       (Looter.Mission.Start Test), so players never see it. Automatic.
  DA_Mission_Main1     "Seven Days", the story's first mission on Ransom's Rest (Docs/Areas/RansomsRest.md, Main 1): the
                       cold open and the claw-out (scenes UColdOpenSubsystem plays on the story's first arrival, or
                       passes when it can't), reading Abel's headboard (tagged Headboard_Abel) and going up to the
                       farmhouse (within 9 m of Delia's door, tagged Speaker_Delia); turned in to Grandma Delia at her
                       door (her Main 1 lines). It starts by itself on Ransom's Rest. 20 XP; the family plot's respawn
                       grave opens with it (ARespawnMarker FamilyPlot, Tools/Unreal/build_area_story.py).
  DA_Mission_Main2     "Shall We Talk Business?" (Main 2), once Main 1 is turned in: up the bluff path onto Ransom's
                       Point (within 18 m of its middle, the marker tagged Place_RansomsPoint, height counted, so the path
                       under the edge isn't the top), clearing the spider nest there (the encounter BluffNest: four
                       spiders and a Restless one, counted from the spawner, so one shot from the path still counts),
                       talking to Mister Sexton on the lookout's rail (Speaker_Sexton), and opening the Ledger (the
                       inventory's second page, which is his book from that step: Bestiary/Ledger.h); turned in to
                       Sexton, still on the rail (a line of its own). 30 XP; the Ledger is the other reward.
  DA_Mission_Main3     "Cold Welcome" (Main 3), once Main 2 is turned in: the farm road through the orchard to the town
                       gate (within 10 m of the marker tagged Place_TownGate), fighting off the Unpaid there (the encounter
                       TownGate: three and a Restless one) and finding Bright & Daughter, Undertakers (within 8 m of
                       Tilly's window, tagged Speaker_Tilly); turned in to Tilly there (her Main 3 lines). 30 XP and an
                       Uncommon gun or better, the undertaker's unclaimed effects. Its id must stay "Main3": Side 1 opens
                       after it (create_side_mission_assets.py), and Main Street's safe zone and shutters follow it.
  DA_Mission_Main4     "Hallowed Ground" (Main 4), once Main 3 is turned in: up to the Chapel of Saint Ada (within 15 m of
                       the chapel's middle, the marker tagged Place_Chapel), clearing the chapel yard (the encounter
                       ChapelYard: 12 Unpaid in two waves and a Restless one, 13 in all), ringing the chapel bell (holding
                       Interact on its rope, the AChapelBell tagged Bell_Chapel) and looking at the Reliquary (the end of
                       its two-second Grave Sight flash sends GraveSight.Reliquary, AChapelReliquary::SightEvent; the arrow
                       on the one tagged Reliquary_Chapel); turned in to Father Aldana at the vestry door (Speaker_Aldana,
                       his Main 4 lines). 40 XP; the chapel yard's respawn grave opens with it (ARespawnMarker ChapelYard),
                       and the Unpaid walk boot hill and the north road from then on (Tools/Unreal/build_area_chapel.py).
                       Its id must stay "Main4": Side 2 opens after it, and Aldana's Ledger page is known after it
                       (create_bestiary_pages.py).
  DA_Mission_Main5     "The Keeper's Lantern" (Main 5), once Main 4 is turned in: down the ramp into the Sink (within 9 m
                       of its floor's middle, the marker tagged Place_SinkFloor, height counted, so the rim and the ramp's
                       upper half aren't the floor), shooting down its three egg sacs (each sends EggSac.Burst,
                       AEggSac::BurstEvent, as it lands and lets out its two spiders; the arrow on the nearest sac still
                       up, tagged EggSac), taking the Keeper's Lantern from the webbing (a tap on the AKeepersLantern
                       tagged Lantern_Keeper) and climbing out (within 6 m of the ramp head on the west rim, the marker
                       tagged Place_SinkRim, height counted); turned in to Father Aldana, who sent Ellis there (a line of
                       its own: take it home to your grandmother). 40 XP. Its id must stay "Main5" and the lantern's step
                       the third (AKeepersLantern::TakeStep): Side 3 opens after it (create_side_mission_assets.py), and
                       Ellis has the lantern from its last step on (AKeepersLantern::IsTaken), which Main 6 carries to
                       Gravewind Point (Tools/Unreal/build_area_sink.py places the Sink's pieces).
  DA_Mission_Main6     "The Gravewind" (Main 6), once Main 5 is turned in, started by Grandma Delia's word through the
                       door ("Take him the lantern...": her topic sends Delia.Main6, Tools/Unreal/build_area_farm.py),
                       which fades the Rest to dusk (AStoryLighting): carrying the lantern to Gravewind Point (within 10 m
                       of the Keeper's Gate, the marker tagged Place_GravewindPoint: both roads end at the keeper's grave
                       and the cairns lead on), hanging it on the keeper's post (a tap on the AKeeperLanternPost tagged
                       LanternPost_Keeper), defeating Abel (the AAbelKeeper tagged Boss_Abel, his boss loot at his death)
                       and sitting with Pa (the scene SitWithPa). Automatic: the scene is its hand-in to Pa, and his
                       board, the lantern lit and the train's steam follow Main 6 finished. 45 XP. Its id must stay
                       "Main6" and its steps in this order: Abel's story counts them (AAbelKeeper::HangStep, FightStep,
                       SceneStep), and his board, the lantern lit and the dusk follow it (Tools/Unreal/build_area_deck.py).
  DA_Mission_Main7     "The Lantern Leans" (Main 7), starting by itself once Main 6 is done: home to Delia (talking at
                       her door, Speaker_Delia: she hands Heirloom out through it, ADoorHandoff); turned in to Tilly at
                       her window on the way to the depot (her Main 7 lines: her father's car coupled up at the
                       platform). 45 XP, the Gilded Lily opened on the station board (GildedLily, create_area_assets.py),
                       and Heirloom, given by Delia's hand rather than dropped (named_gun_by_hand). Its id must stay
                       "Main7" and Delia's step the first: the hand-off, Ned's Ledger page and the train's steam follow it
                       (Tools/Unreal/build_area_depot.py; its depot place is no step now: the board is read after).

Objectives are instanced objects inside the asset, one class per kind (UMissionReachObjective, UMissionKillObjective, ...),
made with unreal.new_object(<class>, asset) and listed in each step's 'objectives'. Classes for actor filters are loaded
by their script path ('/Script/AI_Looter_Shooter.WeaponRack').
"""
import unreal

FOLDER = '/Game/Data/Missions'
CLASSES = '/Script/AI_Looter_Shooter.'


def mission_types():
    """The reflected mission types, or a clear error when the C++ isn't built yet."""
    names = ['MissionDefinition', 'MissionStep', 'MissionRewards', 'MissionTurnIn', 'StoryLine', 'MissionActorFilter',
             'MissionPlace', 'MissionKind', 'MissionStart', 'MissionWaypoint', 'MissionCollect', 'MissionPage',
             'MissionTravelObjective', 'MissionReachObjective', 'MissionCollectObjective', 'MissionHitObjective',
             'MissionKillObjective', 'MissionOpenPageObjective', 'MissionInteractObjective', 'MissionBoardObjective',
             'MissionSceneObjective', 'MissionTalkObjective', 'MissionClearObjective', 'MissionEventObjective',
             'MissionKillNamedObjective', 'MissionClearZoneObjective', 'WeaponRarity']
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
              show_count=True, short='', hint_action='', hint='', **settings):
    """One objective, made inside the asset (its outer) so it's saved with it. With waypoint ACTOR, the arrow points at
    the nearest actor of waypoint_class and/or carrying waypoint_tag. text is the full sentence (the Missions page);
    short is the HUD mission tracker's line (empty: text), and hint_action/hint its key hint (a binding id such as
    Reload, Move for the movement keys, and what the key does; none when empty)."""
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
    for name, value in settings.items():
        made.set_editor_property(name, value)
    return made


def step(*objectives):
    made = unreal.MissionStep()
    made.set_editor_property('objectives', list(objectives))
    return made


def rewards(experience=0, experience_share=0.0, unlock_areas=(), gun=False, gun_rarity_floor='COMMON', named_gun='',
            named_gun_by_hand=False, gun_kind=''):
    """FMissionRewards: a fixed amount of experience (Docs/Progression.md tunes each; or a share of a level's when it's 0),
    areas opened, a gun at the player's feet (at least as rare as the floor; of gun_kind, a weapon definition's asset
    path, or else one from the default loot table), and a named gun by id, dropped, or handed over in the story (by hand:
    listed, never dropped)."""
    made = unreal.MissionRewards()
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
    made.set_editor_property('named_gun', unreal.Name(named_gun))
    made.set_editor_property('named_gun_by_hand', named_gun_by_hand)
    return made


def turn_in(automatic=False, speaker='', giver='', lines=()):
    """FMissionTurnIn: who the mission is turned in to once its objectives are done (their speaker tag and their name for
    "Turn in to <giver>"), and what they say then (lines of (speaker, words); an empty speaker is the giver's speaker
    point's name; none: what they'd say there anyway, for a talk the story already wrote). automatic: it finishes by
    itself after its last objective (no one to turn it in to)."""
    made = unreal.MissionTurnIn()
    made.set_editor_property('automatic', automatic)
    made.set_editor_property('speaker_tag', unreal.Name(speaker))
    made.set_editor_property('giver_name', unreal.Text(giver))
    said = []
    for who, words in lines:
        line = unreal.StoryLine()
        line.set_editor_property('speaker', unreal.Text(who))
        line.set_editor_property('text', unreal.Text(words))
        said.append(line)
    made.set_editor_property('lines', said)
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


def main1_steps(asset):
    """Main 1, "Seven Days": the cold open, the claw-out, the headboard beside Ellis's, the farmhouse; then it's turned in
    at Delia's door (the talk that was its fifth step: her Main 1 lines, as the turn-in's own step is the old talk's, 4).
    The scenes' names are the C++'s (ColdOpen::SceneName, GraveWake::SceneName); the tags are the ones
    Tools/Unreal/build_area_story.py gives what it places."""
    waypoint = unreal.MissionWaypoint
    return [
        step(objective(asset, unreal.MissionSceneObjective, 'Seven days ago.',
                       waypoint=waypoint.NONE, scene=unreal.Name('ColdOpen'))),
        step(objective(asset, unreal.MissionSceneObjective, 'Claw out of the grave: press {Jump}.',
                       waypoint=waypoint.NONE, scene=unreal.Name('GraveWake'))),
        step(objective(asset, unreal.MissionInteractObjective, 'Read the headboard beside yours.',
                       target=actor_filter(tag='Headboard_Abel'), count=1)),
        step(objective(asset, unreal.MissionReachObjective, 'Go up to the farmhouse.',
                       place=place(tag='Speaker_Delia', radius=900.0))),
    ]


def main2_steps(asset):
    """Main 2, "Shall We Talk Business?": up onto Ransom's Point, the spider nest cleared, Sexton's deal, the Ledger. The
    tags and the encounter's id are the ones Tools/Unreal/build_area_story.py gives what it places; the Ledger's step must
    stay the fourth (Ledger::HandedOverStep, counted from 0: 3), as the book is his from it."""
    return [
        # Height counted: the bluff path runs under the top's edge, a few metres from its middle as the crow flies.
        step(objective(asset, unreal.MissionReachObjective, "Climb the bluff path to Ransom's Point.",
                       place=place(tag='Place_RansomsPoint', radius=1800.0, ignore_height=False))),
        step(objective(asset, unreal.MissionClearObjective, 'Clear the spiders nesting at the top.',
                       spawner_id=unreal.Name('BluffNest'), count=5)),
        step(objective(asset, unreal.MissionTalkObjective, 'Talk to Mister Sexton.',
                       speaker_tag=unreal.Name('Speaker_Sexton'))),
        step(objective(asset, unreal.MissionOpenPageObjective, 'Open the Ledger: {Inventory}, then 2.',
                       page=unreal.MissionPage.BESTIARY)),
    ]


def main3_steps(asset):
    """Main 3, "Cold Welcome": the farm road into town, the Unpaid at the gate, Bright & Daughter; then it's turned in at
    Tilly's window (the talk that was its fourth step: her Main 3 lines, from step 3, and the box of effects on the
    step: the reward gun)."""
    return [
        step(objective(asset, unreal.MissionReachObjective, 'Follow the farm road through the orchard into town.',
                       place=place(tag='Place_TownGate', radius=1000.0))),
        step(objective(asset, unreal.MissionClearObjective, 'Fight off the Unpaid at the town gate.',
                       spawner_id=unreal.Name('TownGate'), count=4)),
        step(objective(asset, unreal.MissionReachObjective, 'Find Bright & Daughter, Undertakers.',
                       place=place(tag='Speaker_Tilly', radius=800.0))),
    ]


def main4_steps(asset):
    """Main 4, "Hallowed Ground": up to the chapel, the yard cleared, the bell rung, the Reliquary seen; then it's turned
    in at Father Aldana's vestry door (the talk that was its fifth step: his Main 4 lines, from step 4). The tags, the
    encounter's id and the event are the ones build_area_chapel.py and the C++ give them (AChapelBell::BellTag,
    AChapelReliquary::ReliquaryTag and SightEvent)."""
    waypoint = unreal.MissionWaypoint
    return [
        step(objective(asset, unreal.MissionReachObjective, 'Go up to the Chapel of Saint Ada.',
                       place=place(tag='Place_Chapel', radius=1500.0))),
        # "12 Unpaid in two waves, and one Restless": 13, counted from the spawner.
        step(objective(asset, unreal.MissionClearObjective, 'Clear the chapel yard.',
                       spawner_id=unreal.Name('ChapelYard'), count=13)),
        step(objective(asset, unreal.MissionInteractObjective,
                       'Ring the chapel bell: hold {Interact} on the rope inside the door.',
                       target=actor_filter(tag='Bell_Chapel'), count=1, hold=True)),
        # Done as the flash ends, so the next step comes after the vision; the arrow on the Reliquary meanwhile.
        step(objective(asset, unreal.MissionEventObjective, 'Look at the Reliquary.',
                       waypoint=waypoint.ACTOR, waypoint_tag='Reliquary_Chapel', show_count=False,
                       event=unreal.Name('GraveSight.Reliquary'), count=1)),
    ]


def main5_steps(asset):
    """Main 5, "The Keeper's Lantern": down into the Sink, its three egg sacs shot down, the Keeper's Lantern taken from the
    webbing, and out at the ramp head. The tags and the event are the ones build_area_sink.py and the C++ give them
    (AEggSac::EggSacTag and BurstEvent, AKeepersLantern::LanternTag); the lantern's step must stay the third
    (AKeepersLantern::TakeStep, counted from 0: 2), as Ellis has it from the step after."""
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
                       place=place(tag='Place_SinkRim', radius=600.0, ignore_height=False))),
    ]


def main6_steps(asset):
    """Main 6, "The Gravewind": the lantern carried to Gravewind Point and hung on the keeper's post, Abel defeated, the
    scene after. The tags and the scene are the ones build_area_deck.py and the C++ give them (AKeeperLanternPost's
    KeepersPostTag, AAbelKeeper's BossTag, SitWithPa::SceneName); the steps' order is Abel's (HangStep 1, FightStep 2,
    SceneStep 3, counted from 0)."""
    waypoint = unreal.MissionWaypoint
    return [
        # Either road: the west road or the keeper's path; both end at the keeper's grave, and the cairns lead to the gate.
        step(objective(asset, unreal.MissionReachObjective,
                       "Carry the lantern to Gravewind Point, by the west road or the keeper's path.",
                       place=place(tag='Place_GravewindPoint', radius=1000.0))),
        step(objective(asset, unreal.MissionInteractObjective, "Hang the Keeper's Lantern on the keeper's post.",
                       target=actor_filter(tag='LanternPost_Keeper'), count=1)),
        # A boss falling off the deck is still beaten (any kill counts).
        step(objective(asset, unreal.MissionKillNamedObjective, 'Defeat Abel Ransom, the Keeper.',
                       actor_tag=unreal.Name('Boss_Abel'), player_kills_only=False)),
        step(objective(asset, unreal.MissionSceneObjective, 'Sit with Pa.',
                       waypoint=waypoint.NONE, scene=unreal.Name('SitWithPa'))),
    ]


def main7_steps(asset):
    """Main 7, "The Lantern Leans": home to Delia (Heirloom out through her door); then it's turned in at Tilly's window,
    which Main Street passes on the way to the depot (her Main 7 lines: her father's car coupled up at the platform). The
    Lily opens on the station board as it's turned in, so the depot and the board come after it, not as steps that would
    find the Lily still shut. Delia's step must stay the first (ADoorHandoff's Step 0), as Heirloom comes out as it
    ends."""
    return [
        step(objective(asset, unreal.MissionTalkObjective, 'Go home to Delia.', speaker_tag=unreal.Name('Speaker_Delia'))),
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
    # The story's missions are turned in (Borderlands' way): each gives its experience and the rest only then. Their
    # experience is a fixed amount each, tuned with the kills on the way so the story ends about level 6-7
    # (Docs/Progression.md; Looter.Progression.Pacing checks it).
    dict(asset='DA_Mission_Main1', id='Main1', title='Seven Days',
         summary="Seven days after the Dunne Gang's last job, Ellis claws out of a fresh grave in the Ransom family plot, "
                 "beside Pa's. Read the board beside yours, then go up to the farmhouse: Grandma Delia won't open the "
                 "door to a corpse.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=[], sort_order=1, steps=main1_steps,
         turn_in=dict(speaker='Speaker_Delia', giver='Delia'), rewards=dict(experience=20)),
    dict(asset='DA_Mission_Main2', id='Main2', title='Shall We Talk Business?',
         summary="Someone's waiting up top, on Ransom's Point, where it happened. Spiders nest there now, and a tall man in "
                 "a stovepipe hat sits on the lookout's rail with a ledger on his knee. Reward: experience, and the "
                 "Ledger.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main1'], sort_order=2, steps=main2_steps,
         # Back to him once his ledger is read; he's still on the rail (shown all through Main 2).
         turn_in=dict(speaker='Speaker_Sexton', giver='Mister Sexton', lines=[
             ('', "You've read them, then. Good."),
             ('', "Seven names, friend. The new moon won't wait."),
         ]),
         rewards=dict(experience=30)),
    dict(asset='DA_Mission_Main3', id='Main3', title='Cold Welcome',
         summary="Into town by the farm road. The living shutter their windows, and the Unpaid come for the corpse at the "
                 "gate. Tilly Bright, the undertaker's daughter, talks through her shop window.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main2'], sort_order=3, steps=main3_steps,
         turn_in=dict(speaker='Speaker_Tilly', giver='Tilly'),
         rewards=dict(experience=30, gun=True, gun_rarity_floor='UNCOMMON')),
    dict(asset='DA_Mission_Main4', id='Main4', title='Hallowed Ground',
         summary="The Unpaid hold the chapel yard, and the bell won't call them to rest: Saint Ada's Reliquary lies smashed "
                 "and dark. Father Aldana keeps to the vestry door. Reward: experience, and the chapel yard's grave to wake "
                 "at.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main3'], sort_order=4, steps=main4_steps,
         turn_in=dict(speaker='Speaker_Aldana', giver='Father Aldana'), rewards=dict(experience=40)),
    dict(asset='DA_Mission_Main5', id='Main5', title="The Keeper's Lantern",
         summary="The keepers walk the dead to the boards by lantern light, and Abel's lantern fell in the dark, whole. "
                 "Spiders hoard anything a saint has touched: it's down in the Sink, in the webbing, among their egg "
                 "sacs.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main4'], sort_order=5, steps=main5_steps,
         # Back to Aldana, who sent Ellis to the Sink: his words after it (DA_Lines_AldanaAfterMain5's) send Ellis home to
         # Delia, whose "Take him the lantern" starts Main 6 once this is turned in.
         turn_in=dict(speaker='Speaker_Aldana', giver='Father Aldana', lines=[
             ('', "You found it. Dark, but whole."),
             ('', "Take it home to your grandmother, Ellis. She'll know what's owed a keeper."),
         ]),
         rewards=dict(experience=40)),
    dict(asset='DA_Mission_Main6', id='Main6', title='The Gravewind',
         summary="Grandma Delia, through the door: take him the lantern. At dusk Pa walks the burial boards on Gravewind "
                 "Point, where the wind pours off the Rim. Hang the Keeper's Lantern on his post and show him the way, even "
                 "if he can't go. Reward: experience, and what Abel leaves when he kneels.",
         kind='MAIN', start='ON_EVENT', start_event='Delia.Main6', area='RansomsRest', prerequisites=['Main5'], sort_order=6,
         # It ends with sitting with Pa, its own hand-in: Pa on his board, the lantern lit and the train's steam all follow
         # Main 6 finished, so it finishes as that scene ends (and Main 7 sends Ellis home to Delia).
         steps=main6_steps, turn_in=dict(automatic=True), rewards=dict(experience=45)),
    dict(asset='DA_Mission_Main7', id='Main7', title='The Lantern Leans',
         summary="Pa lit the Keeper's Lantern, and its flame leans north-east, over the ridges, toward Lucky Ned Purcell. "
                 "Go home to Delia first; then Tilly's hearse car waits at the depot. Reward: experience, Heirloom, and the "
                 "line to the Gilded Lily.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main6'], sort_order=7, steps=main7_steps,
         turn_in=dict(speaker='Speaker_Tilly', giver='Tilly'),
         rewards=dict(experience=45, unlock_areas=['GildedLily'], named_gun='Heirloom', named_gun_by_hand=True)),
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


def setup(spec):
    asset, created = load_or_create(spec['asset'], unreal.MissionDefinition)
    asset.set_editor_property('id', unreal.Name(spec['id']))
    asset.set_editor_property('title', unreal.Text(spec['title']))
    asset.set_editor_property('summary', unreal.Text(spec['summary']))
    asset.set_editor_property('kind', getattr(unreal.MissionKind, spec['kind']))
    asset.set_editor_property('start', getattr(unreal.MissionStart, spec['start']))
    # What starts an ON_EVENT mission (Main 6: Delia's word at her door); the others have none.
    asset.set_editor_property('start_event', unreal.Name(spec.get('start_event', '')))
    asset.set_editor_property('area', unreal.Name(spec['area']))
    asset.set_editor_property('prerequisites', [unreal.Name(each) for each in spec['prerequisites']])
    asset.set_editor_property('sort_order', spec['sort_order'])
    steps = spec['steps'](asset)
    asset.set_editor_property('steps', steps)
    asset.set_editor_property('turn_in', turn_in(**spec['turn_in']))
    asset.set_editor_property('rewards', rewards(**spec['rewards']))

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
            unreal.log(f"MISSIONS {spec['asset']} step {index + 1}: {each.get_class().get_name()} "
                       f"\"{each.get_editor_property('text')}\"")
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"{FOLDER}/{spec['asset']} could not be saved")
    finish = ('finishes by itself' if spec['turn_in'].get('automatic')
              else f"turned in to {spec['turn_in'].get('giver')} ({giver})")
    unreal.log(f"MISSIONS {spec['asset']} {'made' if created else 'updated'}: id {spec['id']}, \"{spec['title']}\", "
               f"{spec['kind'].lower()}, starts {spec['start'].lower()}, area '{spec['area']}', {len(steps)} steps, "
               f"{finish}, {spec['rewards'].get('experience', 0)} XP")


def run():
    mission_types()
    for spec in MISSIONS:
        setup(spec)
    unreal.log('MISSIONS done')


run()
