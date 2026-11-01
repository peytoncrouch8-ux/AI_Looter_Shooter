"""Makes (or brings up to date) Mister Sexton's secrets for silver as the level's catalog (USecretCatalog,
Source/AI_Looter_Shooter/Economy/SecretCatalog.h): /Game/Data/Secrets/DA_Secrets_RansomsRest, from story_texts.py's
SEXTON_SECRETS (its SECRETS lane block, 2026-11-01; MAINS_2_7.md section 12; Docs/Work/2026-10-31-Wardens/lanes/SECRETS.md).
Run it in the open editor once the C++ is built (any order with the mission scripts):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_secret_assets.py"
It prints a SECRETS line per secret and "SECRETS done". Running it again writes the catalog again from the data; nothing in
the level changes (the game finds the catalog by the level's name: DA_Secrets_<level without Lvl_>, USecretSubsystem).

What it checks before saving (nothing is saved on a failure): every id its own and plain (SecretSold.<id> is saved:
never rename one); every find a find story_texts.FINDS names, or one of the Ledger pages (LedgerPage_<town> of
LEDGER_TOWNS); a story secret (no finds) names a mark_tag and a done_when; prices over 0; every line fits a caption (90
characters); the regions are FIND_REGIONS' or 'The Rest'.
"""
import importlib
import os
import sys

import unreal

try:
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
except NameError:
    sys.path.insert(0, 'C:/Dev/AI_Looter_Shooter/Tools/Unreal')

FOLDER = '/Game/Data/Secrets'
CAPTION_LIMIT = 90
KINDS = {'Cache': 'CACHE', 'Strongbox': 'STRONGBOX', 'Named': 'NAMED', 'Elite': 'ELITE', 'Lore': 'LORE', 'Story': 'STORY'}


def texts():
    return importlib.reload(importlib.import_module('story_texts'))


def secret_types():
    """The reflected types, or a clear error when the C++ isn't built yet."""
    names = ['SecretCatalog', 'SecretOffer', 'SecretGreeting', 'SecretKind', 'StoryCondition']
    missing = [name for name in names if getattr(unreal, name, None) is None]
    if missing:
        raise RuntimeError(f"unreal.{', unreal.'.join(missing)} missing: build the C++ with Economy/SecretCatalog.h first; "
                           f"nothing was changed")


def load_or_create(asset_name):
    cls = unreal.SecretCatalog
    path = f'{FOLDER}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}, not a SecretCatalog: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, FOLDER, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def check(words, spec):
    """Every rule the docstring names, before anything is written; returns the problems (empty: fine)."""
    problems = []
    known_finds = set(words.FINDS) | {f'LedgerPage_{town}' for town in words.LEDGER_TOWNS}
    regions = set(words.FIND_REGIONS) | {'The Rest'}
    seen = set()
    for line in [spec['sold_out']] + [each for _, each in spec['greetings']]:
        if len(line) > CAPTION_LIMIT:
            problems.append(f'"{line[:40]}...": {len(line)} characters (a caption holds {CAPTION_LIMIT})')
    for entry in spec['secrets']:
        sid = entry['id']
        if sid in seen or not sid.isidentifier():
            problems.append(f'{sid}: an id must be its own and plain')
        seen.add(sid)
        if entry.get('kind') not in KINDS:
            problems.append(f'{sid}: kind {entry.get("kind")} is none of {", ".join(KINDS)}')
        if entry.get('price', 0) <= 0:
            problems.append(f'{sid}: a price over 0')
        if entry.get('region') not in regions:
            problems.append(f'{sid}: region {entry.get("region")} is none of the map\'s rows')
        for key in ('title', 'teaser', 'line'):
            if not entry.get(key):
                problems.append(f'{sid}: no {key}')
            elif len(entry[key]) > CAPTION_LIMIT:
                problems.append(f'{sid}: its {key} is {len(entry[key])} characters (a caption holds {CAPTION_LIMIT})')
        finds = entry.get('finds', [])
        for find in finds:
            if find not in known_finds:
                problems.append(f'{sid}: {find} is no find (story_texts.FINDS, the Ledger pages)')
        if not finds and (not entry.get('mark_tag') or not entry.get('done_when')):
            problems.append(f'{sid}: a story secret (no finds) needs a mark_tag and a done_when')
    return problems


def offer(words, entry):
    made = unreal.SecretOffer()
    made.set_editor_property('id', unreal.Name(entry['id']))
    made.set_editor_property('title', unreal.Text(entry['title']))
    made.set_editor_property('teaser', unreal.Text(entry['teaser']))
    made.set_editor_property('line', unreal.Text(entry['line']))
    made.set_editor_property('region', unreal.Text(entry['region']))
    made.set_editor_property('kind', getattr(unreal.SecretKind, KINDS[entry['kind']]))
    made.set_editor_property('price', int(entry['price']))
    made.set_editor_property('find_ids', [unreal.Name(each) for each in entry.get('finds', [])])
    made.set_editor_property('mark_tag', unreal.Name(entry.get('mark_tag', '')))
    made.set_editor_property('available_when', words.condition(entry.get('when')))
    made.set_editor_property('done_when', words.condition(entry.get('done_when')))
    made.set_editor_property('sets_flag', unreal.Name(entry.get('sets_flag', '')))
    return made


def greeting(words, when, line):
    made = unreal.SecretGreeting()
    made.set_editor_property('when', words.condition(when))
    made.set_editor_property('line', unreal.Text(line))
    return made


def run():
    secret_types()
    words = texts()
    spec = words.SEXTON_SECRETS
    problems = check(words, spec)
    if problems:
        for each in problems:
            unreal.log_error(f'SECRETS {each}')
        raise RuntimeError(f'SECRETS: {len(problems)} problem(s) in story_texts.SEXTON_SECRETS: nothing was saved')

    asset, created = load_or_create(spec['asset'])
    asset.set_editor_property('title', unreal.Text(spec['title']))
    asset.set_editor_property('tab', unreal.Text(spec['tab']))
    asset.set_editor_property('open_when', words.condition(spec['open_when']))
    asset.set_editor_property('discount_mission', unreal.Name(spec['discount_mission']))
    asset.set_editor_property('discount', float(spec['discount']))
    asset.set_editor_property('greetings', [greeting(words, when, line) for when, line in spec['greetings']])
    asset.set_editor_property('sold_out', unreal.Text(spec['sold_out']))
    asset.set_editor_property('secrets', [offer(words, entry) for entry in spec['secrets']])

    # Read back before saving: every secret there, in order, with its finds.
    stored = asset.get_editor_property('secrets')
    if len(stored) != len(spec['secrets']):
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: {len(stored)} secrets stored of {len(spec['secrets'])}: nothing was saved")
    for each, entry in zip(stored, spec['secrets']):
        if str(each.get_editor_property('id')) != entry['id'] or len(each.get_editor_property('find_ids')) != len(entry.get('finds', [])):
            raise RuntimeError(f"{FOLDER}/{spec['asset']}: {entry['id']} read back wrong: nothing was saved")
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"{FOLDER}/{spec['asset']} could not be saved")
    for entry in spec['secrets']:
        marks = ', '.join(entry.get('finds', [])) or f"actors tagged {entry.get('mark_tag')}"
        unreal.log(f"SECRETS {entry['id']}: \"{entry['title']}\", {entry['kind'].lower()}, {entry['price']} silver at level 1, "
                   f"from {entry.get('when')}, marks {marks}")
    unreal.log(f"SECRETS done: {FOLDER}/{spec['asset']} {'made' if created else 'updated'}, {len(spec['secrets'])} secrets, "
               f"open {spec['open_when']}, {int(spec['discount'] * 100)}% off after {spec['discount_mission']}")


if __name__ == '__main__':
    run()
