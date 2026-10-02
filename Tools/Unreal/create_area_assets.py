"""Makes the area definitions (UAreaDefinition, Areas/AreaDefinition.h) in /Game/Data/Areas and saves them. Run it in the
open editor once the C++ with UAreaDefinition and its level band is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_area_assets.py"
It prints one AREAS line per area and "AREAS done" at the end. Running it again updates the fields listed here and
leaves every other field as it is.

  asset            DA_Area_<Id>; sessions and Looter.Travel name the area by its id.
  name             what people see: the session picker, the station boards.
  map              the area's level; it may not exist yet (Ransom's Rest's until its level is built).
  landings         where trips arrive: an actor or player start in the level tagged with the name. The first is the
                   default. Names start with Landing_.
  practice         outside the story (Skyreach): no experience, and ammo-only drops after the first cast-off.
  opening_mission  the story mission the first arrival starts; '' for none.
  sort_order       order in lists, lower first.
  band             (lowest, highest) level of the area's creatures before their ranks' levels: their level follows the
                   player's inside it, give or take one (Docs/Story.md, "Enemy levels"). Skyreach's creatures are level 1.
  promotions       chances a placed Basic creature is Restless (rare) or Gravebound (epic) on an arrival, rolled at most
                   once per 20 minutes of play per map. Legendary monsters are hand-placed.
"""
import unreal

FOLDER = '/Game/Data/Areas'

AREAS = [
    dict(asset='DA_Area_Skyreach', name='Skyreach', map='/Game/Maps/Lvl_TutorialIsland', landings=['Landing_Jetty'],
         practice=True, opening_mission='', sort_order=0, band=(1, 1), promotions=dict(rare=0.0, epic=0.0)),
    dict(asset='DA_Area_RansomsRest', name="Ransom's Rest", map='/Game/Maps/Lvl_RansomsRest', landings=['Landing_Depot'],
         practice=False, opening_mission='Main1', sort_order=10, band=(1, 10), promotions=dict(rare=0.08, epic=0.02)),
]

# The level band and promotion chances, by their Python names, as the C++ declares them.
BAND_FIELDS = ('min_level', 'max_level', 'rare_promotion_chance', 'epic_promotion_chance')


def area_class():
    cls = getattr(unreal, 'AreaDefinition', None)
    if cls is None:
        raise RuntimeError('unreal.AreaDefinition is missing: build the C++ with Areas/AreaDefinition.h first')
    defaults = unreal.get_default_object(cls)
    for field in BAND_FIELDS:
        try:
            defaults.get_editor_property(field)
        except Exception:
            raise RuntimeError(f'unreal.AreaDefinition has no {field}: build the C++ with the areas\' level bands first; nothing was changed')
    return cls


def load_or_create(asset_name, cls):
    path = f'{FOLDER}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}, not an AreaDefinition: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, FOLDER, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def soft_world_path(package):
    """'/Game/Maps/Lvl_X' -> the path to its world object, '/Game/Maps/Lvl_X.Lvl_X', which may not exist yet."""
    return unreal.SoftObjectPath(f"{package}.{package.rsplit('/', 1)[-1]}")


def check_band(spec, asset):
    """The band and chances read back as written (floats within a hair); raises before anything is saved otherwise."""
    lowest, highest = spec['band']
    wanted = dict(min_level=lowest, max_level=highest, rare_promotion_chance=spec['promotions']['rare'],
                  epic_promotion_chance=spec['promotions']['epic'])
    for field, value in wanted.items():
        stored = asset.get_editor_property(field)
        if abs(stored - value) > 1e-6:
            raise RuntimeError(f"{FOLDER}/{spec['asset']}: its {field} reads back as {stored}, not {value}: nothing was saved")


def setup(spec, cls):
    lowest, highest = spec['band']
    if not 1 <= lowest <= highest:
        raise RuntimeError(f"{spec['asset']}: the band {spec['band']} must be 1 or more, lowest first")
    asset, created = load_or_create(spec['asset'], cls)
    asset.set_editor_property('display_name', unreal.Text(spec['name']))
    asset.set_editor_property('map', soft_world_path(spec['map']))
    asset.set_editor_property('landings', [unreal.Name(tag) for tag in spec['landings']])
    asset.set_editor_property('practice', spec['practice'])
    asset.set_editor_property('opening_mission', unreal.Name(spec['opening_mission']))
    asset.set_editor_property('sort_order', spec['sort_order'])
    asset.set_editor_property('min_level', lowest)
    asset.set_editor_property('max_level', highest)
    asset.set_editor_property('rare_promotion_chance', spec['promotions']['rare'])
    asset.set_editor_property('epic_promotion_chance', spec['promotions']['epic'])
    # A soft path prints as "{}" in Python; its text form is what it holds.
    stored_text = asset.get_editor_property('map').export_text()
    if spec['map'] not in stored_text:
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: its map reads back as '{stored_text}', not {spec['map']}: nothing was saved")
    check_band(spec, asset)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"{FOLDER}/{spec['asset']} could not be saved")
    level = 'in the game' if unreal.EditorAssetLibrary.does_asset_exist(spec['map']) else 'not built yet'
    promotions = spec['promotions']
    unreal.log(f"AREAS {spec['asset']} {'made' if created else 'updated'}: \"{spec['name']}\", {stored_text} ({level}), "
               f"landings {spec['landings']}, practice {spec['practice']}, opening mission '{spec['opening_mission']}', "
               f"levels {lowest}-{highest}, promotions Restless {promotions['rare']:.0%} Gravebound {promotions['epic']:.0%}")


def run():
    cls = area_class()
    for spec in AREAS:
        setup(spec, cls)
    unreal.log('AREAS done')


run()
