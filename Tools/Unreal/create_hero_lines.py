"""Makes (or brings up to date) the hero variants of the story's words, /Game/Data/Story/DA_HeroLines (UHeroLineTable,
Story/HeroLineTable.h), from Tools/Unreal/hero_lines.json: every text stays Ellis's in its own script, asset and LOCTEXT,
and this table says what each other hero reads in place of those of Ellis's words that name Ellis's family or Ellis's
own life, and where a hero has a voice of their own (Story/HeroLines.h: where words reach the screen, one call swaps
them). Run it in the open editor once the C++ is built, after Tools/Unreal/create_story_lines.py:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_hero_lines.py"
It prints a HEROLINES line per row and "HEROLINES done: N". Running it again writes the rows whole (the json is the one
place the variants are written: change them there, run this, run the Looter.Lines tests).

Per json entry but the painted ones (their words are on meshes and atlases, which the art makes):
  current         -> Source: Ellis's words, the key, word for word (a key that drifts from the text never fires;
                     Looter.Lines.NoFamily finds what that leaves showing).
  neutral         -> Other: what every hero but Ellis reads; left empty when it is the current itself (a row only for
                     some heroes' own voices).
  heroes          -> Heroes: a hero's own words over the neutral, by id.
  speaker         -> Speaker: who says the line in Ellis's story; the same words said by someone else stay as they are.
  drop_for_others -> bDropForOthers: Ellis's alone, nobody else hears it unless heroes has their own.
  id              -> Id: for the log and the tests.
Tokens a variant may use: {Hero} (the hero's short name), {HeroFull}, {HERO} (in capitals), {Initialed} ("O. Lark"); any
other {...} only when the current has it too (a format argument like {0}, a key like {Interact}), and every one the current
has must stay.

Nothing is saved when an entry is wrong: a current twice (ignoring case, as the game's lookup does), a hero id not in the
roster or Ellis's own (Ellis always reads the current), a token not allowed, or one the current has gone missing.
"""
import json
import os
import re

import unreal

FOLDER = '/Game/Data/Story'
ASSET = 'DA_HeroLines'
JSON = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'hero_lines.json')

# UHeroDefinition::RosterIds (Embers/HeroDefinition.cpp): Ellis first, whose words are the currents themselves.
ELLIS = 'Ellis'
ROSTER = ('Ellis', 'Odessa', 'Crane', 'Gauge', 'Pike')
TOKENS = ('{Hero}', '{HeroFull}', '{HERO}', '{Initialed}')
TOKEN = re.compile(r'\{[A-Za-z0-9_]+\}')


def table_class():
    cls = getattr(unreal, 'HeroLineTable', None)
    if cls is None or getattr(unreal, 'HeroLineVariant', None) is None:
        raise RuntimeError('unreal.HeroLineTable is missing: build the C++ with Story/HeroLineTable.h first; nothing was changed')
    return cls


def load_or_create(cls):
    path = f'{FOLDER}/{ASSET}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}, not a HeroLineTable: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(ASSET, FOLDER, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def check(entries):
    """Every entry's words as the game will read them, or a RuntimeError naming each problem (nothing is saved)."""
    problems = []
    seen = {}
    for entry in entries:
        where = entry.get('id', '?')
        current = entry.get('current', '')
        if not current:
            problems.append(f'{where}: no current')
            continue
        key = current.lower()
        if key in seen:
            problems.append(f'{where}: its current is {seen[key]}\'s too (ignoring case)')
        seen[key] = where
        current_tokens = set(TOKEN.findall(current))
        heroes = entry.get('heroes', {})
        for hero in heroes:
            if hero not in ROSTER or hero == ELLIS:
                problems.append(f'{where}: hero "{hero}" is not one of {", ".join(ROSTER[1:])}')
        variants = [('neutral', entry.get('neutral', ''))] + [(f'heroes.{hero}', words) for hero, words in heroes.items()]
        for name, words in variants:
            if not words:
                continue
            for token in TOKEN.findall(words):
                if token not in TOKENS and token not in current_tokens:
                    problems.append(f'{where} {name}: token {token} is not one of {", ".join(TOKENS)}')
            for token in current_tokens:
                if token not in TOKENS and token not in words:
                    problems.append(f'{where} {name}: lost {token}, which the current has')
    if problems:
        raise RuntimeError('hero_lines.json is wrong, nothing was saved:\n  ' + '\n  '.join(problems))


def variant(entry):
    """FHeroLineVariant for one entry."""
    current = entry['current']
    neutral = entry.get('neutral', '')
    row = unreal.HeroLineVariant()
    row.set_editor_property('id', unreal.Name(entry.get('id', '')))
    row.set_editor_property('source', current)
    row.set_editor_property('speaker', entry.get('speaker', ''))
    # The neutral that is the current itself reads as the current: no Other (the row is only for heroes' own voices).
    row.set_editor_property('other', unreal.Text('' if neutral == current else neutral))
    row.set_editor_property('heroes', {unreal.Name(hero): unreal.Text(words) for hero, words in entry.get('heroes', {}).items()})
    row.set_editor_property('drop_for_others', bool(entry.get('drop_for_others', False)))
    return row


def run():
    cls = table_class()
    with open(JSON, encoding='utf-8') as f:
        data = json.load(f)
    entries = [entry for entry in data['entries'] if entry.get('kind') != 'painted']
    check(entries)
    asset, created = load_or_create(cls)
    rows = [variant(entry) for entry in entries]
    asset.set_editor_property('lines', rows)
    stored = asset.get_editor_property('lines')
    if len(stored) != len(entries) or str(stored[0].get_editor_property('source')) != entries[0]['current']:
        raise RuntimeError(f'{FOLDER}/{ASSET}: its rows read back wrong: nothing was saved')
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f'{FOLDER}/{ASSET} could not be saved')
    for entry in entries:
        others = 'drop' if entry.get('drop_for_others') else ('same' if entry.get('neutral', '') == entry['current'] else 'neutral')
        unreal.log(f"HEROLINES {entry.get('id', '?')}: {others}, {len(entry.get('heroes', {}))} hero(es) of their own")
    unreal.log(f"HEROLINES done: {len(entries)} rows in {FOLDER}/{ASSET} ({'made' if created else 'updated'})")


# Run in the editor (console.ps1 "py ..."); another script may import check() without making anything.
if __name__ == '__main__':
    run()
