"""Makes the mission data assets (UMissionDefinition, Missions/MissionDefinition.h) in /Game/Data/Missions and saves
them. Run it in the open editor once the C++ with UMissionDefinition is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_mission_assets.py"
It prints a MISSIONS line per mission and step, and "MISSIONS done" at the end. Running it again rebuilds these
missions (fields, steps and rewards) from what's written here; other mission assets are left alone.

  DA_Mission_Tutorial  "Welcome to Skyreach", the tutorial island's steps as objectives. It must say what the tutorial
                       director's built-in steps say (ATutorialDirector's constructor): the director plays the asset,
                       and Looter.Missions.TutorialMission compares the two objective by objective.
  DA_Mission_BoardSkiff "Board the skiff", leaving Skyreach: its own mission, not a seventh tutorial step. Skyreach's
                       jetty (ASkiffJetty) starts it whenever the tutorial is done and the player hasn't cast off for the
                       first time (after the tutorial, after skipping it, and for sessions that finished it before the
                       skiff came), so it starts from code (manual) and asks for nothing first. Its arrow is on the
                       jetty; casting off sends Board.Skiff, which finishes it. No reward: Skyreach gives no experience.
  DA_Mission_Test      a small mission for trying the runner in a game: kill two creatures, reach the windmill, use two
                       things tagged MissionTest (no level has them: Looter.Mission.Event Interact MissionTest stands in
                       for the interaction component), for 10% of a level's experience. It starts only from the console
                       (Looter.Mission.Start Test), so players never see it.
  DA_Mission_Main1     "Seven Days", the story's first mission on Ransom's Rest (Docs/Areas/RansomsRest.md, Main 1): the
                       cold open and the claw-out (scenes UColdOpenSubsystem plays on the story's first arrival, or
                       passes when it can't), reading Abel's headboard (tagged Headboard_Abel), going up to the farmhouse
                       (within 9 m of Delia's door, tagged Speaker_Delia) and talking to Grandma Delia there. It starts by
                       itself on Ransom's Rest. 30% of a level's experience; the family plot's respawn grave opens with
                       it (ARespawnMarker FamilyPlot, Tools/Unreal/build_area_story.py).
  DA_Mission_Main2     "Shall We Talk Business?" (Main 2), once Main 1 is done: up the bluff path onto Ransom's Point
                       (within 18 m of its middle, the marker tagged Place_RansomsPoint, height counted, so the path
                       under the edge isn't the top), clearing the spider nest there (the encounter BluffNest: four
                       spiders and a Restless one, counted from the spawner, so one shot from the path still counts),
                       talking to Mister Sexton on the lookout's rail (Speaker_Sexton), and opening the Ledger (the
                       inventory's second page, which is his book from that step: Bestiary/Ledger.h). 30% of a level;
                       the Ledger is the other reward.
  DA_Mission_Main3     "Cold Welcome" (Main 3), once Main 2 is done: the farm road through the orchard to the town gate
                       (within 10 m of the marker tagged Place_TownGate), fighting off the Unpaid there (the encounter
                       TownGate: three and a Restless one), finding Bright & Daughter, Undertakers (within 8 m of Tilly's
                       window, tagged Speaker_Tilly) and talking to Tilly there. 30% of a level and an Uncommon gun or
                       better, the undertaker's unclaimed effects. Its id must stay "Main3": Side 1 opens after it
                       (create_side_mission_assets.py), and Main Street's safe zone and shutters follow it.
  DA_Mission_Main4     "Hallowed Ground" (Main 4), once Main 3 is done: up to the Chapel of Saint Ada (within 15 m of the
                       chapel's middle, the marker tagged Place_Chapel), clearing the chapel yard (the encounter
                       ChapelYard: 12 Unpaid in two waves and a Restless one, 13 in all), ringing the chapel bell (holding
                       Interact on its rope, the AChapelBell tagged Bell_Chapel), looking at the Reliquary (the end of its
                       two-second Grave Sight flash sends GraveSight.Reliquary, AChapelReliquary::SightEvent; the arrow on
                       the one tagged Reliquary_Chapel) and talking to Father Aldana at the vestry door (Speaker_Aldana).
                       30% of a level; the chapel yard's respawn grave opens with it (ARespawnMarker ChapelYard), and the
                       Unpaid walk boot hill and the north road from then on (Tools/Unreal/build_area_chapel.py). Its id
                       must stay "Main4": Side 2 opens after it, and Aldana's Ledger page is known after it
                       (create_bestiary_pages.py).
  DA_Mission_Main5     "The Keeper's Lantern" (Main 5), once Main 4 is done: down the ramp into the Sink (within 9 m of
                       its floor's middle, the marker tagged Place_SinkFloor, height counted, so the rim and the ramp's
                       upper half aren't the floor), shooting down its three egg sacs (each sends EggSac.Burst,
                       AEggSac::BurstEvent, as it lands and lets out its two spiders; the arrow on the nearest sac still
                       up, tagged EggSac), taking the Keeper's Lantern from the webbing (a tap on the AKeepersLantern
                       tagged Lantern_Keeper) and climbing out (within 6 m of the ramp head on the west rim, the marker
                       tagged Place_SinkRim, height counted). 30% of a level. Its id must stay "Main5" and the lantern's
                       step the third (AKeepersLantern::TakeStep): Side 3 opens after it (create_side_mission_assets.py),
                       and Ellis has the lantern from its last step on (AKeepersLantern::IsTaken), which Main 6 carries
                       to Gravewind Point (Tools/Unreal/build_area_sink.py places the Sink's pieces).
  DA_Mission_Main6     "The Gravewind" (Main 6), once Main 5 is done, started by Grandma Delia's word through the door
                       ("Take him the lantern...": her topic sends Delia.Main6, Tools/Unreal/build_area_farm.py), which
                       fades the Rest to dusk (AStoryLighting): carrying the lantern to Gravewind Point (within 10 m of the
                       Keeper's Gate, the marker tagged Place_GravewindPoint: both roads end at the keeper's grave and the
                       cairns lead on), hanging it on the keeper's post (a tap on the AKeeperLanternPost tagged
                       LanternPost_Keeper), defeating Abel (the AAbelKeeper tagged Boss_Abel, his boss loot at his death)
                       and sitting with Pa (the scene SitWithPa). 30% of a level. Its id must stay "Main6" and its steps in
                       this order: Abel's story counts them (AAbelKeeper::HangStep, FightStep, SceneStep), and his board,
                       the lantern lit and the dusk follow it (Tools/Unreal/build_area_deck.py).

Objectives are instanced objects inside the asset, one class per kind (UMissionReachObjective, UMissionKillObjective, ...),
made with unreal.new_object(<class>, asset) and listed in each step's 'objectives'. Classes for actor filters are loaded
by their script path ('/Script/AI_Looter_Shooter.WeaponRack').
"""
import unreal

FOLDER = '/Game/Data/Missions'
CLASSES = '/Script/AI_Looter_Shooter.'


def mission_types():
    """The reflected mission types, or a clear error when the C++ isn't built yet."""
    names = ['MissionDefinition', 'MissionStep', 'MissionRewards', 'MissionActorFilter', 'MissionPlace', 'MissionKind',
             'MissionStart', 'MissionWaypoint', 'MissionCollect', 'MissionPage', 'MissionTravelObjective',
             'MissionReachObjective', 'MissionCollectObjective', 'MissionHitObjective', 'MissionKillObjective',
             'MissionOpenPageObjective', 'MissionInteractObjective', 'MissionBoardObjective', 'MissionSceneObjective',
             'MissionTalkObjective', 'MissionClearObjective', 'MissionEventObjective', 'MissionKillNamedObjective',
             'WeaponRarity']
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
              show_count=True, **settings):
    """One objective, made inside the asset (its outer) so it's saved with it. With waypoint ACTOR, the arrow points at
    the nearest actor of waypoint_class and/or carrying waypoint_tag."""
    made = unreal.new_object(cls, asset)
    made.set_editor_property('text', unreal.Text(text))
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


def rewards(experience_share=0.0, unlock_areas=(), gun=False, gun_rarity_floor='COMMON'):
    """FMissionRewards: a share of a level's experience, areas opened, and a gun at the player's feet (at least as rare as
    the floor)."""
    made = unreal.MissionRewards()
    made.set_editor_property('experience_share', experience_share)
    made.set_editor_property('unlock_areas', [unreal.Name(area) for area in unlock_areas])
    made.set_editor_property('gun', gun)
    made.set_editor_property('gun_rarity_floor', getattr(unreal.WeaponRarity, gun_rarity_floor))
    return made


def tutorial_steps(asset):
    """The tutorial island's six steps, word for word and number for number as ATutorialDirector's built-in ones
    (TutorialDirectorMission.cpp makes the same objectives from them)."""
    waypoint = unreal.MissionWaypoint
    return [
        # Moving at all; the road leads to the village and its gun rack, so the arrow already points there.
        step(objective(asset, unreal.MissionTravelObjective,
                       'Welcome to Skyreach. Move with {Move} and look around with the mouse.',
                       waypoint=waypoint.ACTOR, waypoint_class='WeaponRack', distance=600.0)),
        # Within 9 m of the gun rack, on the map; a level without one passes it.
        step(objective(asset, unreal.MissionReachObjective,
                       'Hold {Sprint} to run. Follow the road to the village.',
                       pass_without_targets=True, place=place('WeaponRack', radius=900.0))),
        # Carrying a gun; the arrow on the rifle lying on the rack (the rack once it's taken).
        step(objective(asset, unreal.MissionCollectObjective,
                       'Grab the rifle on the gun rack: look at it and press {Interact}.',
                       waypoint=waypoint.ACTOR, waypoint_class='WeaponRack', what=unreal.MissionCollect.WEAPONS, count=1)),
        # Five of the player's hits on any dummy; the arrow on the training ground's middle.
        step(objective(asset, unreal.MissionHitObjective,
                       'Shoot the target dummies in the meadow under the windmill. {Reload} reloads.',
                       waypoint=waypoint.TARGETS_CENTER, pass_without_targets=True, show_count=False,
                       target=actor_filter('TargetDummy'), count=5, player_hits_only=True)),
        # Two of the player's kills of any creature; the arrow on the nearest spider (any creature without one).
        step(objective(asset, unreal.MissionKillObjective,
                       'Spiders nest in the woods past the pond. Hunt down two of them.',
                       waypoint=waypoint.ACTOR, waypoint_class='SpiderCreature', pass_without_targets=True, show_count=False,
                       target=actor_filter('CreatureBase'), count=2, player_kills_only=True)),
        # The inventory, open on any page; no arrow.
        step(objective(asset, unreal.MissionOpenPageObjective,
                       "Press {Inventory} to see your loadout and your weapons' stats.",
                       waypoint=waypoint.NONE, page=unreal.MissionPage.ANY)),
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
    """Main 1, "Seven Days": the cold open, the claw-out, the headboard beside Ellis's, the farmhouse, Delia's door. The
    scenes' names are the C++'s (ColdOpen::SceneName, GraveWake::SceneName); the tags are the ones
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
        step(objective(asset, unreal.MissionTalkObjective, 'Talk to Grandma Delia at the screen door.',
                       speaker_tag=unreal.Name('Speaker_Delia'))),
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
    """Main 3, "Cold Welcome": the farm road into town, the Unpaid at the gate, Bright & Daughter, Tilly at her window."""
    return [
        step(objective(asset, unreal.MissionReachObjective, 'Follow the farm road through the orchard into town.',
                       place=place(tag='Place_TownGate', radius=1000.0))),
        step(objective(asset, unreal.MissionClearObjective, 'Fight off the Unpaid at the town gate.',
                       spawner_id=unreal.Name('TownGate'), count=4)),
        step(objective(asset, unreal.MissionReachObjective, 'Find Bright & Daughter, Undertakers.',
                       place=place(tag='Speaker_Tilly', radius=800.0))),
        step(objective(asset, unreal.MissionTalkObjective, 'Talk to Tilly at the window.',
                       speaker_tag=unreal.Name('Speaker_Tilly'))),
    ]


def main4_steps(asset):
    """Main 4, "Hallowed Ground": up to the chapel, the yard cleared, the bell rung, the Reliquary seen, Father Aldana at
    the vestry door. The tags, the encounter's id and the event are the ones build_area_chapel.py and the C++ give them
    (AChapelBell::BellTag, AChapelReliquary::ReliquaryTag and SightEvent)."""
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
        step(objective(asset, unreal.MissionTalkObjective, 'Talk to Father Aldana at the vestry door.',
                       speaker_tag=unreal.Name('Speaker_Aldana'))),
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
    dict(asset='DA_Mission_Tutorial', id='Tutorial', title='Welcome to Skyreach',
         summary='Learn to move, fight and loot on Skyreach.', kind='TUTORIAL', start='MANUAL', area='Skyreach',
         prerequisites=[], sort_order=0, steps=tutorial_steps, rewards=dict()),
    dict(asset='DA_Mission_BoardSkiff', id='BoardSkiff', title='Board the skiff',
         summary="The skiff at the jetty past the lookout takes you off Skyreach, and your story begins. You can come "
                 "back to practice any time.",
         kind='TUTORIAL', start='MANUAL', area='Skyreach', prerequisites=[], sort_order=10, steps=board_skiff_steps,
         rewards=dict()),
    dict(asset='DA_Mission_Test', id='Test', title='Test: Kill, Reach, Use',
         summary="A short mission for trying missions as data: kill, reach and interact objectives and an experience "
                 "reward. Only Looter.Mission.Start Test starts it.",
         kind='SIDE', start='MANUAL', area='', prerequisites=[], sort_order=100, steps=test_steps,
         rewards=dict(experience_share=0.1)),
    dict(asset='DA_Mission_Main1', id='Main1', title='Seven Days',
         summary="Seven days after the Dunne Gang's last job, Ellis claws out of a fresh grave in the Ransom family plot, "
                 "beside Pa's. Read the board beside yours, then go up to the farmhouse: Grandma Delia won't open the "
                 "door to a corpse.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=[], sort_order=1, steps=main1_steps,
         rewards=dict(experience_share=0.3)),
    dict(asset='DA_Mission_Main2', id='Main2', title='Shall We Talk Business?',
         summary="Someone's waiting up top, on Ransom's Point, where it happened. Spiders nest there now, and a tall man in "
                 "a stovepipe hat sits on the lookout's rail with a ledger on his knee. Reward: experience, and the "
                 "Ledger.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main1'], sort_order=2, steps=main2_steps,
         rewards=dict(experience_share=0.3)),
    dict(asset='DA_Mission_Main3', id='Main3', title='Cold Welcome',
         summary="Into town by the farm road. The living shutter their windows, and the Unpaid come for the corpse at the "
                 "gate. Tilly Bright, the undertaker's daughter, talks through her shop window.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main2'], sort_order=3, steps=main3_steps,
         rewards=dict(experience_share=0.3, gun=True, gun_rarity_floor='UNCOMMON')),
    dict(asset='DA_Mission_Main4', id='Main4', title='Hallowed Ground',
         summary="The Unpaid hold the chapel yard, and the bell won't call them to rest: Saint Ada's Reliquary lies smashed "
                 "and dark. Father Aldana keeps to the vestry door. Reward: experience, and the chapel yard's grave to wake "
                 "at.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main3'], sort_order=4, steps=main4_steps,
         rewards=dict(experience_share=0.3)),
    dict(asset='DA_Mission_Main5', id='Main5', title="The Keeper's Lantern",
         summary="The keepers walk the dead to the boards by lantern light, and Abel's lantern fell in the dark, whole. "
                 "Spiders hoard anything a saint has touched: it's down in the Sink, in the webbing, among their egg "
                 "sacs.",
         kind='MAIN', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main4'], sort_order=5, steps=main5_steps,
         rewards=dict(experience_share=0.3)),
    dict(asset='DA_Mission_Main6', id='Main6', title='The Gravewind',
         summary="Grandma Delia, through the door: take him the lantern. At dusk Pa walks the burial boards on Gravewind "
                 "Point, where the wind pours off the Rim. Hang the Keeper's Lantern on his post and show him the way, even "
                 "if he can't go. Reward: experience, and what Abel leaves when he kneels.",
         kind='MAIN', start='ON_EVENT', start_event='Delia.Main6', area='RansomsRest', prerequisites=['Main5'], sort_order=6,
         steps=main6_steps, rewards=dict(experience_share=0.3)),
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
    asset.set_editor_property('rewards', rewards(**spec['rewards']))

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
    unreal.log(f"MISSIONS {spec['asset']} {'made' if created else 'updated'}: id {spec['id']}, \"{spec['title']}\", "
               f"{spec['kind'].lower()}, starts {spec['start'].lower()}, area '{spec['area']}', {len(steps)} steps")


def run():
    mission_types()
    for spec in MISSIONS:
        setup(spec)
    unreal.log('MISSIONS done')


run()
