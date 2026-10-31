"""Things to find off the path, as one find system (Docs/Work/2026-10-28-Core/FINDS.md; the user's 2026-10-21 complaint
"nothing to find off the path"), for build_area.py's gameplay pass: an AFindSpot (World/FindSpot.h, hidden data, tag
Find) for each of the fourteen finds (Ranger Post Three removed 2026-10-31, the user: the chest and steps by the wake-up grave) and the six Ledger pages, and the new finds' pieces. Read from the layout's
gameplay.finds (positions, kinds, regions, sources, loot numbers; layout x north, y east, cm); the words are
story_texts.py's (FINDS: name, region, Hob's lines; READABLES: the readables' words and READABLE_MODELS). Everything goes
in the area's Gameplay/Finds folder (a gameplay build clears Gameplay first). A find whose class, source, layout entry or
spot is missing is warned of and left out (never a silent gap: Looter.Finds.Level counts them). Ransom's Rest only (the
layout whose gameplay has a finds block).

- The nine existing finds (build_area_secrets.py's chests, found here by label): only the spot, the tells and the find's
  pay (Silver), plus Pike's field-book scrap (READABLES Find_PikeScrap) beside the surveyor's tin on Coffin Rock.
- The new ones (build_area_finds_sites.py): the tarn's sunken lockbox (N1), the Tumble's bolt-hole (N2), Ranger Post
  Three on the Rimrock (N3; removed 2026-10-31, its builder kept), Pike's cache by the Nose (N4), the miser's grave on boot hill (N5), the Tollman's spot on
  the keeper's road (N6, an Elite: found by the kill of the creature tagged Find_Tollman, its encounter is the FIGHTS
  lane's patches/fights.json).
- New chest ids (never renamed once placed): Find_TarnLockbox, Find_BoltHole, Find_RangerPost (tiers: chest_tiers.py
  TIERS), Find_PikesCache, Find_MiserGrave (graves, their loot the layout's).
- Rock pieces of one find stand as one AInstancedProps per mesh (instances, not draws); the walkable tops and the
  bolt-hole's ceiling and floor are hidden engine cubes (collision only, the Coffin Rock lesson: rounded rock tops slide
  the player off), tagged for the level test (FindRangerStep, FindRangerDeck, FindBoltHoleCeiling, FindBoltHoleFloor).

Hook in build_area.py's gameplay(), right after the secrets:
    importlib.reload(importlib.import_module('build_area_finds')).place(self)
On its own, in the open editor (clears only Gameplay/Finds, places, saves; the secrets, seeds and camps must stand):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_finds.py RansomsRest"
Each find logs one FINDS line; "FINDS done" at the end.
"""
import importlib
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Finds'
FIND_TAG = 'Find'            # AFindSpot::FindTag (the spot actors only)
PIECE_TAG = 'FindPiece'      # every other piece this script places
KINDS = {'Cache': 'CACHE', 'Hidden': 'HIDDEN', 'Grave': 'GRAVE', 'Elite': 'ELITE', 'Lore': 'LORE'}
# A readable's place beside its chest (cm, to the chest's left: build_area_secrets puts its own to the right) and reach.
SCRAP_ASIDE = 70.0
# The glint's default height over a chest's spot (cm) when the layout gives none.
GLINT_UP = 45.0
# No find this near a story character or speaker point (cm): a hidden thing never stands beside someone (the level test's
# rule too).
STORY_CLEAR = 300.0


def texts():
    return importlib.reload(importlib.import_module('story_texts'))


def secrets():
    return importlib.reload(importlib.import_module('build_area_secrets'))


def sites():
    return importlib.reload(importlib.import_module('build_area_finds_sites'))


def finds_block(build):
    return build.settings.get('finds') or {}


def setp(build, actor, name, value, what):
    """Sets one property, warning (not stopping) when the built module doesn't have it."""
    try:
        actor.set_editor_property(name, value)
        return True
    except Exception as error:  # noqa: BLE001 - an older game module
        build.warn(f'{what}: no {name} ({error}): build the game module with the find system')
        return False


def placed_at(actor):
    at = actor.get_actor_location()
    return at.x, at.y, at.z


def words_for(find_id):
    """(name, region, lines) from story_texts.FINDS, or None while the words aren't written."""
    entry = getattr(texts(), 'FINDS', {}).get(find_id)
    if entry is None:
        return None
    if isinstance(entry, dict):
        return entry.get('name', find_id), entry.get('region'), entry.get('lines', ())
    return entry[0], None, entry[1] if len(entry) > 1 else ()


def place_spot(build, entry, at, source=None, tells=(), name=None):
    """The AFindSpot for a layout entry at at (x, y, z): its id, words, kind, source, pay and tells. Returns it or None."""
    cls = story.actor_class('FindSpot')
    if cls is None:
        build.warn('no FindSpot class (build the game module with the find system first): no finds')
        return None
    find_id = entry['id']
    spot = build.place(cls, at, 0.0, label=f'FindSpot_{find_id}', folder=FOLDER, tags=(FIND_TAG,))
    what = f'FindSpot_{find_id}'
    setp(build, spot, 'find_id', unreal.Name(find_id), what)
    words = words_for(find_id)
    region = entry.get('region')
    lines = ()
    if words is not None:
        name = name or words[0]
        if words[1] and region and words[1] != region:
            build.warn(f'{find_id}: the layout says region {region}, story_texts.FINDS {words[1]}: FINDS wins')
        region = words[1] or region
        lines = words[2]
    elif entry['kind'] != 'Lore':
        build.warn(f'{find_id}: no story_texts.FINDS words yet: its banner shows its id')
    setp(build, spot, 'display_name', unreal.Text(name or find_id), what)
    setp(build, spot, 'region', unreal.Text(region or ''), what)
    setp(build, spot, 'kind', getattr(unreal.FindKind, KINDS[entry['kind']]), what)
    if source is not None:
        setp(build, spot, 'source', source, what)
    if entry.get('sourceEvent'):
        setp(build, spot, 'source_event', unreal.Name(entry['sourceEvent']), what)
    if entry.get('source', {}).get('creatureTag'):
        setp(build, spot, 'source_tag', unreal.Name(entry['source']['creatureTag']), what)
    if entry.get('zoneLevel'):
        setp(build, spot, 'zone_level', int(entry['zoneLevel']), what)
    if entry.get('silver'):
        setp(build, spot, 'silver', int(entry['silver']), what)
    if entry.get('namedGun'):
        setp(build, spot, 'named_gun', unreal.Name(entry['namedGun']), what)
    if lines:
        setp(build, spot, 'lines', texts().story_lines(lines), what)
    glint = entry.get('glint', [0.0, 0.0, GLINT_UP])
    setp(build, spot, 'glint', bool(entry.get('glintOn', True)), what)
    setp(build, spot, 'glint_offset', unreal.Vector(*glint), what)
    setp(build, spot, 'crows', int(entry.get('crows', 2)), what)
    if entry.get('crowHeight'):
        setp(build, spot, 'crow_height', float(entry['crowHeight']), what)
    if tells:
        setp(build, spot, 'tells', [t for t in tells if t is not None], what)
    return spot


def story_people():
    """Where the story's characters and speaker points stand (x, y), for STORY_CLEAR."""
    kinds = [getattr(unreal, name) for name in ('StoryCharacter', 'SpeakerPoint') if hasattr(unreal, name)]
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    return [(a.get_actor_location().x, a.get_actor_location().y, str(a.get_actor_label())) for a in actors
            if any(isinstance(a, k) for k in kinds)]


def check_clear_of_story(build, find_id, at, people):
    for x, y, label in people:
        if math.hypot(x - at[0], y - at[1]) < STORY_CLEAR:
            build.warn(f'{find_id}: {math.hypot(x - at[0], y - at[1]):.0f} cm from {label} (keep {STORY_CLEAR:.0f}): '
                       'Looter.Finds.Level fails on it')


# ---------------------------------------------------------------------------
# The pieces (shared with build_area_finds_sites.py)
# ---------------------------------------------------------------------------

def place_chest(build, spec, x, y, z, yaw, find_id):
    """A find's AChest (spec: the layout entry's chest block): a loot chest tier where chest_tiers.py names its id (kind
    'Tier'), else its kind (Grave, Footlocker) with the layout's loot and its OpenWhen. Tagged Chest, its kind, Obstacle
    (dropped by AChest for a grave) and FindPiece. Returns it, or None."""
    cls = story.actor_class('Chest')
    if cls is None:
        build.warn('no Chest class (build the game module first): no find chests')
        return None
    chest_id = spec['id']
    kind = spec['kind']
    chest = build.place(cls, (x, y, z), yaw, label=chest_id, folder=FOLDER,
                        tags=('Chest', 'Obstacle', PIECE_TAG, find_id) + (() if kind == 'Tier' else (kind,)))
    tiers = importlib.reload(importlib.import_module('chest_tiers'))
    if kind == 'Tier':
        tier = tiers.TIERS.get(chest_id)
        if tier is None:
            build.warn(f'{chest_id}: kind Tier but chest_tiers.TIERS has no tier for it: tier 3')
            tier = 3
        tiers.apply(chest, tier)
    else:
        chest.set_editor_property('kind', getattr(unreal.ChestKind, {'Grave': 'GRAVE', 'Footlocker': 'FOOTLOCKER'}[kind]))
        if spec.get('loot'):
            loot = unreal.ChestLootOverride()
            loot.set_editor_property('override', True)
            for name, value in spec['loot'].items():
                loot.set_editor_property(name, value)
            chest.set_editor_property('loot_override', loot)
    if spec.get('openWhen'):
        chest.set_editor_property('open_when', texts().condition(spec['openWhen']))
    # Set last, so the chest is built again with its kind's models and the session finds it by its id.
    chest.set_editor_property('chest_id', unreal.Name(chest_id))
    return chest


def place_readable(build, readable_id, x, y, z, yaw, find_id, model=None):
    """A readable (build_area_secrets.readable: words and model from story_texts), moved into Gameplay/Finds and tagged
    FindPiece (so a secrets-only rebuild never takes it). model: a mesh name in place of READABLE_MODELS' (a prop read)."""
    prop = secrets().readable(build, readable_id, x, y, z, yaw)
    if prop is None:
        return None
    prop.set_folder_path(f'{build.folder}/{FOLDER}')
    tags = [t for t in prop.tags if str(t) != secrets().SECRET_TAG]
    prop.set_editor_property('tags', tags + [unreal.Name(PIECE_TAG), unreal.Name(find_id)])
    if model:
        mesh = find_mesh(model)
        if mesh is None:
            build.warn(f'{readable_id}: no {model}: it keeps its stand-in model')
        else:
            prop.get_editor_property('mesh').set_static_mesh(mesh)
    return prop


def find_mesh(name):
    return secrets().find_mesh(name)


def place_prop(build, mesh_name, at, yaw, find_id, label, scale=None, solid=True, tags=()):
    """One static mesh actor (a stake, a lantern, a stone): solid ones tagged Obstacle (the minimap), the rest without
    collision. Returns it, or None (warned) when the mesh isn't imported."""
    mesh = find_mesh(mesh_name)
    if mesh is None:
        build.warn(f'{find_id}: no {mesh_name}: {label} left out')
        return None
    actor = build.place(mesh, at, yaw, label=label, folder=FOLDER, scale=scale,
                        tags=(PIECE_TAG, find_id) + (('Obstacle',) if solid else ()) + tuple(tags))
    if not solid:
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return actor


def hidden_box(build, label, centre, size, yaw, find_id, tag):
    """A hidden engine cube (collision only, no shadow), centre (x, y, z) and size (x, y, z cm) in its yaw's frame."""
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')
    actor = build.place(cube, centre, yaw, label=label, folder=FOLDER,
                        scale=(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0), tags=(PIECE_TAG, find_id, tag))
    component = actor.static_mesh_component
    component.set_visibility(False)
    component.set_cast_shadow(False)
    actor.set_actor_hidden_in_game(True)
    return actor


class Rocks:
    """A find's rock pieces, one AInstancedProps per mesh (and solidity): add(), then place()."""

    def __init__(self, find_id):
        self.find_id = find_id
        self.sets = {}

    def add(self, mesh_name, location, rotation, scale, solid=True, material=None):
        key = (mesh_name, solid)
        self.sets.setdefault(key, {'material': material, 'transforms': []})['transforms'].append(
            unreal.Transform(unreal.Vector(*location), rotation, unreal.Vector(*scale)))

    def place(self, build, cull=15000.0):
        cls = story.actor_class('InstancedProps')
        made = []
        if cls is None:
            build.warn(f'{self.find_id}: no InstancedProps class (build the game module first): no rock pieces')
            return made
        for (mesh_name, solid), each in sorted(self.sets.items()):
            mesh = find_mesh(mesh_name)
            if mesh is None:
                build.warn(f'{self.find_id}: no {mesh_name}: {len(each["transforms"])} pieces left out')
                continue
            label = f'{self.find_id}_{mesh_name}' + ('' if solid else '_Passable')
            actor = build.place(cls, (0.0, 0.0, 0.0), label=label, folder=FOLDER, tags=(PIECE_TAG, self.find_id))
            actor.set_editor_property('cull_distance', float(cull))
            actor.set_editor_property('solid', bool(solid))
            actor.set_instances(mesh, each['transforms'])
            if each['material'] is not None:
                actor.get_editor_property('instances').set_material(0, each['material'])
            made.append(actor)
        return made


def area_material(build, name):
    """The finds' own material instance (gameplay.finds.materials: parent and values, as level.materials), made or updated
    and saved when it changed; None (warned) when its parent isn't there."""
    spec = finds_block(build).get('materials', {}).get(name)
    if spec is None:
        return None
    mel = unreal.MaterialEditingLibrary
    path = f'/Game/Art/Materials/{name}'
    parent = unreal.load_asset(f'/Game/Art/Materials/{spec["parent"]}')
    if parent is None:
        build.warn(f'no {spec["parent"]}: no {name}')
        return None
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        instance = unreal.load_asset(path)
    else:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, '/Game/Art/Materials', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, parent)
    for key, value in spec.get('vectors', {}).items():
        mel.set_material_instance_vector_parameter_value(instance, key, unreal.LinearColor(*(list(value) + [1.0] * (4 - len(value)))))
    for key, value in spec.get('scalars', {}).items():
        mel.set_material_instance_scalar_parameter_value(instance, key, value)
    mel.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=True)
    return instance


# ---------------------------------------------------------------------------
# The existing finds, the Elite, the Ledger pages
# ---------------------------------------------------------------------------

def place_existing(build, entry, found):
    """An E find: its spot on its secrets chest (by label). CoffinRock also gets Pike's scrap (entry 'scrap')."""
    label = entry['source']['chest']
    chest = found.get(label)
    if chest is None:
        build.warn(f'{entry["id"]}: no {label} in the level (build_area_secrets.py places it): left out')
        return None
    x, y, z = placed_at(chest)
    if entry.get('scrap'):
        place_scrap(build, entry, chest)
    return place_spot(build, entry, (x, y, z), chest), (x, y, z)


def place_scrap(build, entry, chest):
    """The scrap beside its chest: to the chest's left, else in front of it, else behind, on ground within 60 cm of it."""
    x, y, z = placed_at(chest)
    yaw = chest.get_actor_rotation().yaw
    a = math.radians(yaw)
    fwd, right = (math.cos(a), math.sin(a)), (-math.sin(a), math.cos(a))
    for dx, dy in ((-right[0], -right[1]), fwd, (-fwd[0], -fwd[1])):
        px, py = x + dx * SCRAP_ASIDE, y + dy * SCRAP_ASIDE
        pz = secrets().trace_down(px, py, z + 150.0, z - 150.0)
        if pz is not None and abs(pz - z) < 60.0:
            return place_readable(build, entry['scrap'], px, py, pz, yaw, entry['id'], entry.get('scrapModel'))
    build.warn(f'{entry["id"]}: no room beside {chest.get_actor_label()} for {entry["scrap"]}: left out')
    return None


def place_elite(build, entry, found):
    """N6: the Tollman's spot on the keeper's road (entry 'at'), its source his encounter when it's placed."""
    x, y = entry['at']
    z = secrets().trace_down(x, y)
    if z is None:
        build.warn(f'{entry["id"]}: no ground at ({x:.0f}, {y:.0f}): left out')
        return None
    spawner = found.get(entry['source'].get('encounter', ''))
    if spawner is None:
        build.warn(f'{entry["id"]}: no {entry["source"].get("encounter")} yet (the FIGHTS lane\'s patrol, '
                   'patches/fights.json): the spot counts its kill by the tag alone')
    return place_spot(build, entry, (x, y, z), spawner), (x, y, z)


def place_ledger(build, block, found):
    """The six Ledger pages' spots (Lore: their own count), on build_area_seeds.py's readables."""
    words = texts()
    made = 0
    for page in block.get('pages', []):
        readable = found.get(f'Readable_{page["id"]}')
        if readable is None:
            build.warn(f'{page["id"]}: no Readable_{page["id"]} (build_area_seeds.py): its Lore spot left out')
            continue
        entry = dict(block, id=page['id'], region=page['region'], kind='Lore', sourceEvent=f'Readable.{page["id"]}')
        town = page['id'].split('_', 1)[1]
        name = words.LEDGER_TOWNS.get(town, {}).get('name', town)
        spot = place_spot(build, entry, placed_at(readable), readable, name=f'Ledger page: {name}')
        if spot is not None:
            made += 1
            build.log(f'FINDS {page["id"]}: Lore on {readable.get_actor_label()}')
    return made


def place(build):
    """Every find in the layout's gameplay.finds, then the Ledger pages."""
    block = finds_block(build)
    if not block.get('spots'):
        return
    found = importlib.reload(importlib.import_module('build_area_missionprops_valley')).level_actors(build)
    builders = sites().BUILDERS
    people = story_people()
    made = 0
    for entry in block['spots']:
        what = entry.get('build', 'existing')
        try:
            if what == 'existing':
                result = place_existing(build, entry, found)
            elif what == 'elite':
                result = place_elite(build, entry, found)
            else:
                result = builders[what](build, entry, found)
        except Exception as error:  # noqa: BLE001 - one find's trouble never stops the others
            build.warn(f'{entry["id"]}: {what} failed ({error}): left out')
            result = None
        if not result or result[0] is None:
            build.warn(f'FINDS {entry["id"]}: left out')
            continue
        spot, at = result
        made += 1
        check_clear_of_story(build, entry['id'], at, people)
        build.log(f'FINDS {entry["id"]}: {entry["kind"]} ({entry.get("region")}) at ({at[0]:.0f}, {at[1]:.0f}, {at[2]:.0f})')
    pages = place_ledger(build, block.get('ledger', {}), found)
    build.log(f'FINDS done: {made} of {len(block["spots"])} finds, {pages} of {len(block.get("ledger", {}).get("pages", []))} '
              'Ledger pages')


def run(name):
    """Places Gameplay/Finds again on its own, saves."""
    import build_area
    build = importlib.reload(build_area).AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the finds placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_finds.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
