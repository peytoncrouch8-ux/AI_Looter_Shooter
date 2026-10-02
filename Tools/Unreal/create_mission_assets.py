"""Makes the mission data assets (UMissionDefinition, Missions/MissionDefinition.h) in /Game/Data/Missions and saves
them. Run it in the open editor once the C++ with UMissionDefinition is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_mission_assets.py"
It prints a MISSIONS line per mission and step, and "MISSIONS done" at the end. Running it again rebuilds these two
missions (fields, steps and rewards) from what's written here; other mission assets are left alone.

  DA_Mission_Tutorial  "Welcome to Skyreach", the tutorial island's steps as objectives. It must say what the tutorial
                       director's built-in steps say (ATutorialDirector's constructor): the director plays the asset,
                       and Looter.Missions.TutorialMission compares the two objective by objective.
  DA_Mission_Test      a small mission for trying the runner in a game: kill two creatures, reach the windmill, use two
                       things tagged MissionTest (no level has them: Looter.Mission.Event Interact MissionTest stands in
                       for the interaction component), for 10% of a level's experience. It starts only from the console
                       (Looter.Mission.Start Test), so players never see it.

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
             'MissionOpenPageObjective', 'MissionInteractObjective']
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


def objective(asset, cls, text, waypoint=None, waypoint_class=None, pass_without_targets=False, show_count=True, **settings):
    """One objective, made inside the asset (its outer) so it's saved with it."""
    made = unreal.new_object(cls, asset)
    made.set_editor_property('text', unreal.Text(text))
    if waypoint is not None:
        made.set_editor_property('waypoint', waypoint)
    if waypoint_class:
        made.set_editor_property('waypoint_actor', actor_filter(waypoint_class))
    made.set_editor_property('pass_without_targets', pass_without_targets)
    made.set_editor_property('show_count', show_count)
    for name, value in settings.items():
        made.set_editor_property(name, value)
    return made


def step(*objectives):
    made = unreal.MissionStep()
    made.set_editor_property('objectives', list(objectives))
    return made


def rewards(experience_share=0.0, unlock_areas=()):
    made = unreal.MissionRewards()
    made.set_editor_property('experience_share', experience_share)
    made.set_editor_property('unlock_areas', [unreal.Name(area) for area in unlock_areas])
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
    dict(asset='DA_Mission_Test', id='Test', title='Test: Kill, Reach, Use',
         summary="A short mission for trying missions as data: kill, reach and interact objectives and an experience "
                 "reward. Only Looter.Mission.Start Test starts it.",
         kind='SIDE', start='MANUAL', area='', prerequisites=[], sort_order=100, steps=test_steps,
         rewards=dict(experience_share=0.1)),
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
