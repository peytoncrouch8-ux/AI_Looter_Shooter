"""Makes the side missions' data assets (UMissionDefinition, Missions/MissionDefinition.h) in /Game/Data/Missions and
saves them, as create_mission_assets.py does for the others (kept apart, so the two can change side by side). Run it in
the open editor once the C++ with AWantedPoster and UMissionLastingInteractObjective is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_side_mission_assets.py"
It prints a SIDEMISSIONS line per mission and step, and "SIDEMISSIONS done" at the end. Running it again rebuilds these
missions (fields, steps and rewards) from what's written here; other mission assets are left alone.

  DA_Mission_Side1  "Wanted: Already Dead" (Docs/Areas/RansomsRest.md, Side 1), on Ransom's Rest once Main 3 is
                    finished, starting by itself. Tear down 6 of Ellis's wanted posters around town and the farms (each a
                    held Interact on an AWantedPoster tagged WantedPoster), counted from the world
                    (UMissionLastingInteractObjective): posters torn down before it opened count, and so do the ones torn
                    before a reload, since a side mission starts over from its first step. Seven hang, so one can be
                    missed. Then read Ranger Calder's note on the Rim Rangers' board (a tap on the one tagged CalderNote).
                    Reward: a side mission's share of experience (20% of the player's level) and a guaranteed Rare gun.
  DA_Mission_Side3  "The Gravemother" (Side 3), on Ransom's Rest once Main 5 is finished, starting by itself. Enter the
                    den (reach the place tagged Place_Den inside it, within 4.5 m measured with its height, so the Sink's
                    rim over the den doesn't count), then kill the Gravemother (clear her lair's encounter, Gravemother:
                    a kill before the step began counts too, and so does one on an earlier visit while she's still away).
                    Reward: a side mission's share of experience, once; her Legendary loot table drops on every kill, and
                    she comes back on an arrival 20 minutes of play after her death. Both made by build_area_den.py.
                    Made once Main 5 exists (step 19's main mission): until then the script says it waits.

Objectives are instanced objects inside the asset, one class per kind, made with unreal.new_object(<class>, asset) and
listed in each step's 'objectives'. Classes for actor filters are loaded by their script path
('/Script/AI_Looter_Shooter.WantedPoster').
"""
import unreal

FOLDER = '/Game/Data/Missions'
CLASSES = '/Script/AI_Looter_Shooter.'
# AWantedPoster's tags (World/WantedPoster.h).
WANTED_TAG = 'WantedPoster'
NOTE_TAG = 'CalderNote'
# The Gravemother's den and lair (Creatures/GravemotherCreature.h: Gravemother::DenPlaceTag and LairId; placed by
# Tools/Unreal/build_area_den.py).
DEN_TAG = 'Place_Den'
DEN_REACH = 450.0
GRAVEMOTHER_LAIR = 'Gravemother'


def mission_types():
    """The reflected mission types, or a clear error when the C++ isn't built yet."""
    names = ['MissionDefinition', 'MissionStep', 'MissionRewards', 'MissionActorFilter', 'MissionKind', 'MissionStart',
             'MissionInteractObjective', 'MissionLastingInteractObjective', 'WantedPoster', 'WeaponRarity',
             'MissionReachObjective', 'MissionClearObjective', 'MissionPlace']
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


def objective(asset, cls, text, waypoint=None, show_count=True, **settings):
    """One objective, made inside the asset (its outer) so it's saved with it."""
    made = unreal.new_object(cls, asset)
    made.set_editor_property('text', unreal.Text(text))
    if waypoint is not None:
        made.set_editor_property('waypoint', waypoint)
    made.set_editor_property('show_count', show_count)
    for name, value in settings.items():
        made.set_editor_property(name, value)
    return made


def step(*objectives):
    made = unreal.MissionStep()
    made.set_editor_property('objectives', list(objectives))
    return made


def rewards(experience_share=0.0, gun=False, gun_rarity_floor='COMMON'):
    made = unreal.MissionRewards()
    made.set_editor_property('experience_share', experience_share)
    made.set_editor_property('gun', gun)
    made.set_editor_property('gun_rarity_floor', getattr(unreal.WeaponRarity, gun_rarity_floor))
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


def side3_steps(asset):
    """Enter the den (its place, measured with its height), then kill the Gravemother (her lair's encounter cleared). The
    arrows find their own: the den's place, then the Gravemother herself (or her lair while she's in her den)."""
    den = unreal.MissionPlace()
    den.set_editor_property('actor', actor_filter(tag=DEN_TAG))
    den.set_editor_property('radius', DEN_REACH)
    den.set_editor_property('ignore_height', False)
    return [
        step(objective(asset, unreal.MissionReachObjective, 'Enter the den', show_count=False, place=den)),
        step(objective(asset, unreal.MissionClearObjective, 'Kill the Gravemother', show_count=False,
                       spawner_id=unreal.Name(GRAVEMOTHER_LAIR), count=1)),
    ]


MISSIONS = [
    dict(asset='DA_Mission_Side1', id='Side1', title='Wanted: Already Dead',
         summary='Your wanted poster hangs all over Ransom\'s Rest, and someone has written ALREADY under DEAD OR ALIVE. '
                 'Tear them down.',
         kind='SIDE', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main3'], sort_order=10, steps=side1_steps,
         rewards=dict(experience_share=0.2, gun=True, gun_rarity_floor='RARE'),
         # What setup() checks the stored steps against: each step's objective class.
         expect=['MissionLastingInteractObjective', 'MissionInteractObjective']),
    dict(asset='DA_Mission_Side3', id='Side3', title='The Gravemother',
         summary='Something big lives in the Sink\'s den, and it eats what the Unpaid leave behind.',
         kind='SIDE', start='AUTOMATIC', area='RansomsRest', prerequisites=['Main5'], sort_order=30, steps=side3_steps,
         # Experience the first time; her loot is her own (the Legendary table on every kill), so no reward gun.
         rewards=dict(experience_share=0.2, gun=False),
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
    spawner = editor_property(objective_object, 'spawner_id')
    if spawner is not None:
        words.append(f'the encounter {spawner} cleared')
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
    for index, (stored_step, expected) in enumerate(zip(stored, spec['expect'])):
        objectives = stored_step.get_editor_property('objectives')
        if not objectives or any(each is None for each in objectives):
            raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1} lost its objectives: nothing was saved")
        for each in objectives:
            if each.get_outer() != asset:
                raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1}'s {each.get_name()} lives outside the asset: nothing was saved")
            if each.get_class().get_name() != expected:
                raise RuntimeError(f"{FOLDER}/{spec['asset']}: step {index + 1} holds a {each.get_class().get_name()}, not a "
                                   f"{expected}: nothing was saved")
            unreal.log(f"SIDEMISSIONS {spec['asset']} step {index + 1}: {expected} \"{each.get_editor_property('text')}\", "
                       f"{describe(each)}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"{FOLDER}/{spec['asset']} could not be saved")
    reward = spec['rewards']
    unreal.log(f"SIDEMISSIONS {spec['asset']} {'made' if created else 'updated'}: id {spec['id']}, \"{spec['title']}\", "
               f"{spec['kind'].lower()}, starts {spec['start'].lower()} after {', '.join(spec['prerequisites'])}, "
               f"area '{spec['area']}', {len(steps)} steps; rewards {reward['experience_share']:.0%} of a level"
               f"{', a ' + reward['gun_rarity_floor'].title() + ' gun' if reward['gun'] else ''}")


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
