"""Makes the area definitions (UAreaDefinition, Areas/AreaDefinition.h) in /Game/Data/Areas and saves them. Run it in the
open editor once the C++ with UAreaDefinition, its level band and its level zones is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_area_assets.py"
It prints one AREAS line per area and "AREAS done" at the end. Running it again updates the fields listed here and
leaves every other field as it is.

  asset            DA_Area_<Id>; sessions and Looter.Travel name the area by its id.
  name             what people see: the session picker, the station boards.
  spoken           what a sentence calls it, when shorter than its name ("the Lily": "The line to the Lily isn't open
                   yet."); '' for its name.
  map              the area's level; it may not exist yet (Ransom's Rest's until its level is built; the Gilded Lily's,
                   which Main 7 opens: the depot's board lists it, and nobody goes until its level is in the game).
  landings         where trips arrive: an actor or player start in the level tagged with the name. The first is the
                   default. Names start with Landing_.
  practice         outside the story (Skyreach): no experience, and ammo-only drops after the first cast-off.
  opening_mission  the story mission the first arrival starts; '' for none.
  sort_order       order in lists, lower first.
  layout           the area's Art/Levels/<Area>/layout.json, whose gameplay.levels gives its creatures' levels: the band,
                   the zones and the kinds at set levels (Docs/Progression.md, "Creature levels"). A creature's level is
                   where it stands, never the player's.
  band             for an area with no layout yet (the Lily): (lowest, highest) level of its creatures before their
                   ranks' levels, with no zones: its creatures are at the band's floor or one above until it has some.
  promotions       chances a placed Basic creature is Restless (rare) or Gravebound (epic) on an arrival, rolled at most
                   once per 20 minutes of play per map. Legendary monsters are hand-placed.
  required_kinds   kinds the area's layout must set, at these levels (its bosses and Legendary monsters, never rolled from a
                   zone): checked before anything is saved, so a layout that loses one is caught here. Ransom's Rest's
                   Foreman (Back Pay) is a set 4 before his Soulfed rank's 2, so level 6, a level under Abel's 7 (since
                   2026-10-27 he is the second boss, before Abel: the user's call; level 7 before).

A level zone's shape in the layout is a layout zone ("zone"), an obstacle ("obstacle"), its own "polygon", or a "radius"
round a point ("at") or between two obstacles ("between"), made a 16-sided outline as the tests read it
(Tests/EncounterLayoutReader.h, FAreaShape::Samples). Looter.Areas.Bands checks the assets against the layouts.
"""
import json
import math
import os

import unreal

FOLDER = '/Game/Data/Areas'
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())

AREAS = [
    dict(asset='DA_Area_Skyreach', name='Skyreach', map='/Game/Maps/Lvl_TutorialIsland', landings=['Landing_Jetty'],
         practice=True, opening_mission='', sort_order=30, layout='Art/Levels/TutorialIsland/layout.json',
         promotions=dict(rare=0.0, epic=0.0)),
    dict(asset='DA_Area_RansomsRest', name="Ransom's Rest", map='/Game/Maps/Lvl_RansomsRest', landings=['Landing_Depot'],
         practice=False, opening_mission='Main1', sort_order=10, layout='Art/Levels/RansomsRest/layout.json',
         promotions=dict(rare=0.08, epic=0.02), required_kinds=dict(ForemanCreature=4)),
    dict(asset='DA_Area_GildedLily', name='The Gilded Lily', spoken='the Lily', map='/Game/Maps/Lvl_GildedLily',
         landings=['Landing_Platform'], practice=False, opening_mission='', sort_order=20, band=(9, 15),
         promotions=dict(rare=0.08, epic=0.02)),
]

# The level band, zones, kinds and promotion chances, by their Python names, as the C++ declares them.
LEVEL_FIELDS = ('min_level', 'max_level', 'level_zones', 'kind_levels', 'rare_promotion_chance', 'epic_promotion_chance')

# A radius becomes an outline with this many corners (FAreaShape::Samples in Tests/EncounterLayoutReader.h).
CIRCLE_CORNERS = 16


def area_class():
    cls = getattr(unreal, 'AreaDefinition', None)
    if cls is None:
        raise RuntimeError('unreal.AreaDefinition is missing: build the C++ with Areas/AreaDefinition.h first')
    defaults = unreal.get_default_object(cls)
    for field in LEVEL_FIELDS:
        try:
            defaults.get_editor_property(field)
        except Exception:
            raise RuntimeError(f"unreal.AreaDefinition has no {field}: build the C++ with the areas' level zones first; "
                               "nothing was changed")
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


# ---------------------------------------------------------------------------
# The levels, from the layout
# ---------------------------------------------------------------------------

def centroid(points):
    return (sum(p[0] for p in points) / len(points), sum(p[1] for p in points) / len(points))


def zone_outline(entry, layout, label):
    """A level zone's outline [(x, y), ...] from its layout entry, as Tests/EncounterLayoutReader.h's ReadShape reads it."""
    zones = {z['id']: z for z in layout.get('zones', [])}
    obstacles = {o['id']: o for o in layout.get('obstacles', []) if 'id' in o}
    if 'polygon' in entry:
        return [tuple(p[:2]) for p in entry['polygon']]
    if 'obstacle' in entry:
        if entry['obstacle'] not in obstacles or 'polygon' not in obstacles[entry['obstacle']]:
            raise RuntimeError(f"{label}: no obstacle polygon {entry['obstacle']} in the layout: nothing was saved")
        return [tuple(p[:2]) for p in obstacles[entry['obstacle']]['polygon']]
    if 'zone' in entry:
        if entry['zone'] not in zones:
            raise RuntimeError(f"{label}: no layout zone {entry['zone']}: nothing was saved")
        return [tuple(p[:2]) for p in zones[entry['zone']]['polygon']]
    if 'radius' in entry and ('at' in entry or 'between' in entry):
        if 'at' in entry:
            middle = tuple(entry['at'][:2])
        else:
            first, second = (centroid(obstacles[name]['polygon']) for name in entry['between'])
            middle = ((first[0] + second[0]) / 2.0, (first[1] + second[1]) / 2.0)
        radius = float(entry['radius'])
        return [(middle[0] + math.cos(2.0 * math.pi * step / CIRCLE_CORNERS) * radius,
                 middle[1] + math.sin(2.0 * math.pi * step / CIRCLE_CORNERS) * radius) for step in range(CIRCLE_CORNERS)]
    raise RuntimeError(f"{label}: a level zone needs a zone, an obstacle, a polygon, or a radius round a point: "
                       "nothing was saved")


def read_levels(spec):
    """(band, zones, kinds) for an area: from its layout's gameplay.levels, or its own band with no zones or kinds."""
    if 'layout' not in spec:
        return tuple(spec['band']), [], []
    with open(os.path.join(PROJECT, spec['layout'])) as f:
        layout = json.load(f)
    levels = layout.get('gameplay', {}).get('levels')
    if levels is None:
        raise RuntimeError(f"{spec['layout']} has no gameplay.levels: nothing was saved")
    lowest, highest = levels['band']
    zones = []
    for entry in levels.get('zones', []):
        label = f"{spec['asset']}'s zone {entry['id']}"
        low, high = entry['levels']
        if not lowest <= low <= high <= highest:
            raise RuntimeError(f"{label}: its levels {entry['levels']} must lie inside the band {levels['band']}, lowest first")
        outline = zone_outline(entry, layout, label)
        if len(outline) < 3:
            raise RuntimeError(f'{label}: its outline has fewer than three corners')
        zones.append(dict(id=entry['id'], outline=outline, low=low, high=high))
    kinds = []
    for entry in levels.get('kinds', []):
        kind = getattr(unreal, entry['class'], None)
        if kind is None:
            raise RuntimeError(f"{spec['asset']}: no creature class {entry['class']} (build the game module first): "
                               "nothing was saved")
        if not lowest <= entry['level'] <= highest:
            raise RuntimeError(f"{spec['asset']}: {entry['class']}'s level {entry['level']} must lie inside the band "
                               f"{levels['band']}")
        kinds.append(dict(name=entry['class'], cls=kind.static_class(), level=entry['level']))
    return (lowest, highest), zones, kinds


def make_zones(zones):
    made = []
    for zone in zones:
        each = unreal.AreaLevelZone()
        each.set_editor_property('id', unreal.Name(zone['id']))
        each.set_editor_property('outline', [unreal.Vector2D(x, y) for x, y in zone['outline']])
        each.set_editor_property('min_level', zone['low'])
        each.set_editor_property('max_level', zone['high'])
        made.append(each)
    return made


def make_kinds(kinds):
    made = []
    for kind in kinds:
        each = unreal.AreaKindLevel()
        each.set_editor_property('kind', kind['cls'])
        each.set_editor_property('level', kind['level'])
        made.append(each)
    return made


def check_levels(spec, asset, band, zones, kinds):
    """The band, zones, kinds and chances read back as written (floats within a hair); raises before anything is saved."""
    label = f"{FOLDER}/{spec['asset']}"
    wanted = dict(min_level=band[0], max_level=band[1], rare_promotion_chance=spec['promotions']['rare'],
                  epic_promotion_chance=spec['promotions']['epic'])
    for field, value in wanted.items():
        stored = asset.get_editor_property(field)
        if abs(stored - value) > 1e-6:
            raise RuntimeError(f'{label}: its {field} reads back as {stored}, not {value}: nothing was saved')
    stored_zones = asset.get_editor_property('level_zones')
    if len(stored_zones) != len(zones):
        raise RuntimeError(f'{label}: {len(stored_zones)} level zones read back, not {len(zones)}: nothing was saved')
    for stored, zone in zip(stored_zones, zones):
        if (str(stored.get_editor_property('id')) != zone['id'] or len(stored.get_editor_property('outline')) != len(zone['outline'])
                or stored.get_editor_property('min_level') != zone['low'] or stored.get_editor_property('max_level') != zone['high']):
            raise RuntimeError(f"{label}: its level zone {zone['id']} reads back wrong: nothing was saved")
    stored_kinds = asset.get_editor_property('kind_levels')
    if len(stored_kinds) != len(kinds):
        raise RuntimeError(f'{label}: {len(stored_kinds)} kinds at set levels read back, not {len(kinds)}: nothing was saved')
    for stored, kind in zip(stored_kinds, kinds):
        if stored.get_editor_property('kind') != kind['cls'] or stored.get_editor_property('level') != kind['level']:
            raise RuntimeError(f"{label}: {kind['name']}'s set level reads back wrong: nothing was saved")


def check_required_kinds(spec, kinds):
    """The kinds the area must set at set levels are in its layout at those levels; raises before anything is saved."""
    set_levels = {kind['name']: kind['level'] for kind in kinds}
    for name, level in spec.get('required_kinds', {}).items():
        if set_levels.get(name) != level:
            raise RuntimeError(f"{spec['asset']}: {spec.get('layout', 'its band')} must set {name} at level {level} (found "
                               f"{set_levels.get(name)}): add {{ \"class\": \"{name}\", \"level\": {level} }} to "
                               f"gameplay.levels.kinds; nothing was saved")


def setup(spec, cls):
    band, zones, kinds = read_levels(spec)
    check_required_kinds(spec, kinds)
    if not 1 <= band[0] <= band[1]:
        raise RuntimeError(f"{spec['asset']}: the band {band} must be 1 or more, lowest first")
    asset, created = load_or_create(spec['asset'], cls)
    asset.set_editor_property('display_name', unreal.Text(spec['name']))
    asset.set_editor_property('spoken_name', unreal.Text(spec.get('spoken', '')))
    asset.set_editor_property('map', soft_world_path(spec['map']))
    asset.set_editor_property('landings', [unreal.Name(tag) for tag in spec['landings']])
    asset.set_editor_property('practice', spec['practice'])
    asset.set_editor_property('opening_mission', unreal.Name(spec['opening_mission']))
    asset.set_editor_property('sort_order', spec['sort_order'])
    asset.set_editor_property('min_level', band[0])
    asset.set_editor_property('max_level', band[1])
    asset.set_editor_property('level_zones', make_zones(zones))
    asset.set_editor_property('kind_levels', make_kinds(kinds))
    asset.set_editor_property('rare_promotion_chance', spec['promotions']['rare'])
    asset.set_editor_property('epic_promotion_chance', spec['promotions']['epic'])
    # A soft path prints as "{}" in Python; its text form is what it holds.
    stored_text = asset.get_editor_property('map').export_text()
    if spec['map'] not in stored_text:
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: its map reads back as '{stored_text}', not {spec['map']}: nothing was saved")
    check_levels(spec, asset, band, zones, kinds)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"{FOLDER}/{spec['asset']} could not be saved")
    level = 'in the game' if unreal.EditorAssetLibrary.does_asset_exist(spec['map']) else 'not built yet'
    promotions = spec['promotions']
    zone_text = ', '.join(f"{z['id']} {z['low']}-{z['high']}" for z in zones) or 'none'
    kind_text = ', '.join(f"{k['name']} {k['level']}" for k in kinds) or 'none'
    unreal.log(f"AREAS {spec['asset']} {'made' if created else 'updated'}: \"{spec['name']}\", {stored_text} ({level}), "
               f"landings {spec['landings']}, practice {spec['practice']}, opening mission '{spec['opening_mission']}', "
               f"levels {band[0]}-{band[1]} (zones: {zone_text}; kinds: {kind_text}), "
               f"promotions Restless {promotions['rare']:.0%} Gravebound {promotions['epic']:.0%}")


def run():
    cls = area_class()
    for spec in AREAS:
        setup(spec, cls)
    unreal.log('AREAS done')


run()
