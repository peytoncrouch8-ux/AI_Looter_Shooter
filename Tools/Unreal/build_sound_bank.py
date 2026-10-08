"""Builds the game's sounds in Unreal from Art/Sounds: the synthesized WAVs in Art/Sounds/Out and Art/Sounds/cues.json (one
entry per cue: its files, volume, pitch jitter, sound class, attenuation, loop, 2D or 3D, how many at once). The C++ plays
cues by name through the bank (Audio/LooterSound.h, Audio/LooterSoundCues.h); this makes what it plays:

  /Game/Audio/Mix/SC_Master    the sound classes: Master over Effects, Interface, Ambience and Music (the volume
  /Game/Audio/Mix/SC_<Class>   sliders in Settings > Audio; the world's ambience follows Effects). Interface sounds go on
                               over a paused game (the menus').
  /Game/Audio/Mix/SMX_Volumes  the sound mix the sliders set the classes' volumes in (ULooterSoundSubsystem).
  /Game/Audio/Mix/ATT_Gun      how far sounds carry: natural falloff, spatialized, the far end muffled a little as air
  /Game/Audio/Mix/ATT_Creature does (a low-pass growing with distance), and folded to plain stereo right at the listener
  /Game/Audio/Mix/ATT_Near     (the player's own steps). Gun ~60 m, Creature ~35 m, Near ~15 m.
  /Game/Audio/<Part>/S_<File>  each WAV as a sound wave (Weapon.Rifle.Fire's Weapon_Rifle_Fire_01.wav is
                               /Game/Audio/Weapon/S_Weapon_Rifle_Fire_01), with its cue's class, attenuation and loop.
                               A WAV is imported again only when it changed (its MD5 is kept on the wave).
  /Game/Audio/DA_SoundBank     the bank (ULooterSoundBank): every cue with its waves and settings, sorted by cue.

Run it in the open editor once the C++ is built (it needs unreal.LooterSoundBank):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_sound_bank.py"         build, or bring up to date
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_sound_bank.py check"   report only, change nothing
Running it again updates everything in place: the bank is written whole from cues.json (a cue gone from the JSON is gone
from the bank), and waves under /Game/Audio named S_* that no cue uses any more are deleted (nothing else is touched).
It prints SOUNDBANK lines for what it made, imported, changed and removed, "SOUNDBANK warning: ..." for problems in the
JSON or the WAVs (a bad cue is left out, the rest go in), and "SOUNDBANK done ..." at the end. It also compares the cues
with Audio/LooterSoundCues.h: a cue the code plays with no sound yet plays nothing (and the game says so once).
"""
import hashlib
import json
import os
import re
import sys
import wave as wavefile

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.normpath(os.path.join(HERE, '..', '..'))
SOUNDS = os.path.join(PROJECT, 'Art', 'Sounds')
OUT = os.path.join(SOUNDS, 'Out')
CUES_JSON = os.path.join(SOUNDS, 'cues.json')
CUES_HEADER = os.path.join(PROJECT, 'Source', 'AI_Looter_Shooter', 'Audio', 'LooterSoundCues.h')

AUDIO = '/Game/Audio'
MIX = '/Game/Audio/Mix'
BANK_NAME = 'DA_SoundBank'
MIX_NAME = 'SMX_Volumes'

# The classes under Master, which cues.json names; Master holds them all (the Master slider).
CLASSES = ('Effects', 'Interface', 'Ambience', 'Music')

# How far each kind of sound carries: (radius heard at full volume, distance it fades out at, low-pass at that far end),
# in cm and Hz. Natural falloff reaches -60 dB at the far end.
ATTENUATIONS = {
    'Gun': (400.0, 6000.0, 3500.0),
    'Creature': (300.0, 3500.0, 5000.0),
    'Near': (150.0, 1500.0, 8000.0),
}
# Within this of the listener a sound folds toward plain stereo (cm): the player's own gun and feet don't swing from ear
# to ear as the view turns.
NON_SPATIAL_START = 150.0
NON_SPATIAL_END = 50.0
# The low-pass starts this share of the way out.
LPF_START_SHARE = 0.15

DEFAULTS = {'volume_db': 0.0, 'pitch_jitter': 0.0, 'class': 'Effects', 'loop': False, 'space': '3D', 'max_concurrent': 4}

warnings = []


def log(message):
    unreal.log(f'SOUNDBANK {message}')


def warn(message):
    warnings.append(message)
    unreal.log_warning(f'SOUNDBANK warning: {message}')


def set_props(target, label, **values):
    """Sets each property it can; one the engine doesn't know by that name is a warning, not a stop."""
    for name, value in values.items():
        try:
            target.set_editor_property(name, value)
        except Exception as error:  # noqa: BLE001 - the engine's own exception types vary by property
            warn(f'{label}: could not set {name} ({error})')


def set_if_changed(target, name, value):
    """Sets a property only when it differs (so an unchanged asset isn't dirtied); True when it changed."""
    try:
        if target.get_editor_property(name) == value:
            return False
    except Exception:  # noqa: BLE001
        pass
    target.set_editor_property(name, value)
    return True


def save(asset, label):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f'{label} could not be saved')


def load_or_create(folder, name, cls, factory_name):
    """The asset at folder/name, or a new one; an asset there of another kind stops everything."""
    path = f'{folder}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}, not a {cls.__name__}: nothing more was changed')
        return asset, False
    factory_class = getattr(unreal, factory_name, None)
    factory = factory_class() if factory_class else None
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


# --- Reading the cues ---

def header_cues():
    """The cue names LooterSoundCues.h gives (the C++ plays these)."""
    if not os.path.isfile(CUES_HEADER):
        warn(f'{CUES_HEADER} not found: the cues are not compared with the code')
        return set()
    with open(CUES_HEADER, encoding='utf-8') as f:
        return set(re.findall(r'TEXT\("([^"]+)"\)', f.read()))


def check_format(cue, name, path, spatial):
    """The contract's format (48 kHz, 16-bit PCM, mono when placed in the world): a stereo sound in 3D is spread over the
    speakers rather than placed, so it says where it should."""
    try:
        with wavefile.open(path, 'rb') as audio:
            channels, rate, width = audio.getnchannels(), audio.getframerate(), audio.getsampwidth()
    except (wavefile.Error, EOFError) as error:
        warn(f'{cue}: {name} is not a PCM WAV Unreal reads well ({error})')
        return
    if spatial and channels != 1:
        warn(f'{cue}: {name} has {channels} channels but plays in 3D: it should be mono')
    if rate != 48000 or width != 2:
        warn(f'{cue}: {name} is {rate} Hz, {width * 8}-bit: the sounds are 48 kHz, 16-bit')


def read_cues():
    """cues.json's cues, checked and filled in with the defaults: {cue: settings}. A cue that can't be used is left out."""
    if not os.path.isfile(CUES_JSON):
        raise RuntimeError(f'{CUES_JSON} not found: the sounds are made first (Tools/sounds.ps1); nothing was changed')
    with open(CUES_JSON, encoding='utf-8') as f:
        raw = json.load(f)
    cues = {}
    for cue, spec in sorted(raw.items()):
        # A note in the file ("_comment", "$schema") is no cue.
        if cue.startswith(('_', '$')) or not isinstance(spec, dict):
            continue
        settings = dict(DEFAULTS)
        settings.update(spec)
        problems = []
        if settings['class'] not in CLASSES:
            problems.append(f"class {settings['class']!r} is not one of {', '.join(CLASSES)}")
        space = str(settings['space']).upper()
        if space not in ('2D', '3D'):
            problems.append(f"space {settings['space']!r} is not 2D or 3D")
        attenuation = settings.get('attenuation') or ('None' if space == '2D' else 'Near')
        if attenuation not in ATTENUATIONS and attenuation != 'None':
            problems.append(f'attenuation {attenuation!r} is not one of {", ".join(ATTENUATIONS)} or None')
        if space == '3D' and attenuation == 'None':
            warn(f'{cue}: 3D with no attenuation would be heard everywhere at full volume: given Near')
            attenuation = 'Near'
        if space == '2D':
            attenuation = 'None'
        files = []
        for name in settings.get('files', []):
            path = os.path.join(OUT, name)
            if not name.lower().endswith('.wav'):
                problems.append(f'{name} is not a .wav')
            elif not os.path.isfile(path):
                warn(f'{cue}: {name} is not in Art/Sounds/Out: left out')
            else:
                files.append(path)
                check_format(cue, name, path, space == '3D')
        if not files:
            problems.append('no sound files')
        try:
            max_concurrent = max(1, int(settings['max_concurrent']))
            volume_db = float(settings['volume_db'])
            jitter = min(max(float(settings['pitch_jitter']), 0.0), 0.5)
        except (TypeError, ValueError) as error:
            problems.append(f'a number that is not one ({error})')
            max_concurrent, volume_db, jitter = 1, 0.0, 0.0
        if problems:
            warn(f'{cue} left out: {"; ".join(problems)}')
            continue
        cues[cue] = {
            'files': files,
            'volume_db': volume_db,
            'pitch_jitter': jitter,
            'class': settings['class'],
            'attenuation': None if attenuation == 'None' else attenuation,
            'loop': bool(settings['loop']),
            'spatial': space == '3D',
            'max_concurrent': max_concurrent,
        }
    return cues


# --- The mix ---

def build_classes():
    """SC_Master over the four classes; the Interface class's sounds play over a paused game."""
    made = {}
    for name in ('Master',) + CLASSES:
        sound_class, created = load_or_create(MIX, f'SC_{name}', unreal.SoundClass, 'SoundClassFactory')
        made[name] = sound_class
        if created:
            log(f'{MIX}/SC_{name} made')
    master = made['Master']
    children = [made[name] for name in CLASSES]
    # Setting the children sets a child's parent (USoundClass does it as the property changes), but only the first new
    # child of each change: so keep the ones already parented and add the others one at a time.
    current = [child for child in master.get_editor_property('child_classes')
               if child in children and child.get_editor_property('parent_class') == master]
    if len(current) != len(children):
        # The unparented ones out first, or adding them back wouldn't count as new.
        master.set_editor_property('child_classes', list(current))
        for child in children:
            if child not in current:
                current.append(child)
                master.set_editor_property('child_classes', list(current))
        log(f'SC_Master over {", ".join("SC_" + name for name in CLASSES)}')
    for name in CLASSES:
        parent = made[name].get_editor_property('parent_class')
        if parent != master:
            warn(f'SC_{name}\'s parent is {parent.get_name() if parent else "none"}, not SC_Master: the Master slider misses it')
    for name, flag in (('Interface', 'is_ui_sound'), ('Music', 'is_music')):
        properties = made[name].get_editor_property('properties')
        set_props(properties, f'SC_{name}', **{flag: True})
        made[name].set_editor_property('properties', properties)
    for name, sound_class in made.items():
        save(sound_class, f'SC_{name}')
    return made


def build_mix():
    """SMX_Volumes: no adjusters of its own; the game sets the classes' volumes in it from the sliders."""
    mix, created = load_or_create(MIX, MIX_NAME, unreal.SoundMix, 'SoundMixFactory')
    save(mix, MIX_NAME)
    if created:
        log(f'{MIX}/{MIX_NAME} made')
    return mix


def build_attenuations():
    made = {}
    for name, (inner, reach, far_lpf) in ATTENUATIONS.items():
        asset_name = f'ATT_{name}'
        attenuation, created = load_or_create(MIX, asset_name, unreal.SoundAttenuation, 'SoundAttenuationFactory')
        settings = attenuation.get_editor_property('attenuation')
        set_props(settings, asset_name,
                  attenuate=True,
                  spatialize=True,
                  distance_algorithm=unreal.AttenuationDistanceModel.NATURAL_SOUND,
                  attenuation_shape=unreal.AttenuationShape.SPHERE,
                  attenuation_shape_extents=unreal.Vector(inner, 0.0, 0.0),
                  falloff_distance=reach - inner,
                  non_spatialized_radius_start=NON_SPATIAL_START,
                  non_spatialized_radius_end=NON_SPATIAL_END,
                  attenuate_with_lpf=True,
                  lpf_radius_min=inner + (reach - inner) * LPF_START_SHARE,
                  lpf_radius_max=reach,
                  lpf_frequency_at_min=20000.0,
                  lpf_frequency_at_max=far_lpf)
        attenuation.set_editor_property('attenuation', settings)
        stored = attenuation.get_editor_property('attenuation')
        if abs(stored.get_editor_property('falloff_distance') - (reach - inner)) > 0.5:
            raise RuntimeError(f'{asset_name}: its falloff reads back wrong: nothing more was changed')
        save(attenuation, asset_name)
        made[name] = attenuation
        log(f"{MIX}/{asset_name} {'made' if created else 'set'}: full to {inner / 100:.1f} m, gone at {reach / 100:.0f} m, "
            f'{far_lpf / 1000:.1f} kHz at the far end')
    return made


# --- The waves ---

def wave_path(cue, file):
    """/Game/Audio/<the cue's first part>/S_<the file's name>."""
    folder = f"{AUDIO}/{cue.split('.')[0]}"
    return folder, 'S_' + os.path.splitext(os.path.basename(file))[0]


def import_wave(file, folder, name):
    """The WAV as a sound wave, imported again only when it changed. Returns (wave, imported)."""
    with open(file, 'rb') as f:
        md5 = hashlib.md5(f.read()).hexdigest()
    path = f'{folder}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        existing = unreal.load_asset(path)
        if isinstance(existing, unreal.SoundWave) and unreal.EditorAssetLibrary.get_metadata_tag(existing, 'SourceMD5') == md5:
            return existing, False
        if not isinstance(existing, unreal.SoundWave):
            raise RuntimeError(f'{path} exists but is a {type(existing).__name__}, not a sound wave: nothing more was changed')
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', file)
    task.set_editor_property('destination_path', folder)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    wave = unreal.load_asset(path)
    if not isinstance(wave, unreal.SoundWave):
        raise RuntimeError(f'{os.path.relpath(file, PROJECT)} did not import as {path}: nothing more was changed')
    unreal.EditorAssetLibrary.set_metadata_tag(wave, 'SourceMD5', md5)
    return wave, True


def build_waves(cues, classes, attenuations):
    """Every cue's waves, with its class, attenuation and loop on each. Returns ({cue: [waves]}, {package paths}, counts)."""
    waves = {}
    expected = set()
    imported = unchanged = 0
    for cue, spec in cues.items():
        sound_class = classes[spec['class']]
        attenuation = attenuations[spec['attenuation']] if spec['attenuation'] else None
        waves[cue] = []
        for file in spec['files']:
            folder, name = wave_path(cue, file)
            wave, fresh = import_wave(file, folder, name)
            changed = fresh
            changed |= set_if_changed(wave, 'sound_class_object', sound_class)
            changed |= set_if_changed(wave, 'attenuation_settings', attenuation)
            changed |= set_if_changed(wave, 'looping', spec['loop'])
            if changed:
                save(wave, f'{folder}/{name}')
            imported += 1 if fresh else 0
            unchanged += 0 if changed else 1
            if fresh:
                log(f'{folder}/{name} imported from Art/Sounds/Out/{os.path.basename(file)}')
            elif changed:
                log(f'{folder}/{name} set to its cue\'s class, attenuation and loop')
            waves[cue].append(wave)
            expected.add(f'{folder}/{name}')
    return waves, expected, imported, unchanged


def remove_stale_waves(expected):
    """S_* sound waves under /Game/Audio that no cue uses now (a cue or a variation gone from cues.json)."""
    removed = []
    for path in unreal.EditorAssetLibrary.list_assets(AUDIO, recursive=True, include_folder=False):
        package = path.split('.')[0]
        name = package.rsplit('/', 1)[-1]
        if package.startswith(MIX + '/') or not name.startswith('S_') or package in expected:
            continue
        if not isinstance(unreal.load_asset(path), unreal.SoundWave):
            continue
        if unreal.EditorAssetLibrary.delete_asset(package):
            removed.append(package)
            log(f'{package} removed: no cue uses it now')
        else:
            warn(f'{package} is no cue\'s now but could not be deleted (something else holds it)')
    return removed


# --- The bank ---

def build_bank(cues, waves, classes, attenuations):
    bank_class = getattr(unreal, 'LooterSoundBank', None)
    entry_class = getattr(unreal, 'LooterSoundCueEntry', None)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', bank_class)
    path = f'{AUDIO}/{BANK_NAME}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        bank = unreal.load_asset(path)
        if not isinstance(bank, bank_class):
            raise RuntimeError(f'{path} exists but is a {type(bank).__name__}, not a LooterSoundBank: nothing more was changed')
        created = False
    else:
        bank = unreal.AssetToolsHelpers.get_asset_tools().create_asset(BANK_NAME, AUDIO, bank_class, factory)
        if bank is None:
            raise RuntimeError(f'{path} could not be created')
        created = True
    before = {str(entry.get_editor_property('cue')) for entry in bank.get_editor_property('cues')}

    entries = []
    for cue, spec in sorted(cues.items()):
        entry = entry_class()
        entry.set_editor_property('cue', cue)
        entry.set_editor_property('sounds', waves[cue])
        entry.set_editor_property('volume_db', spec['volume_db'])
        entry.set_editor_property('pitch_jitter', spec['pitch_jitter'])
        entry.set_editor_property('sound_class', classes[spec['class']])
        entry.set_editor_property('attenuation', attenuations[spec['attenuation']] if spec['attenuation'] else None)
        entry.set_editor_property('loop', spec['loop'])
        entry.set_editor_property('spatial', spec['spatial'])
        # The menus' sounds go on over a paused game; the world's pause with it.
        entry.set_editor_property('plays_while_paused', spec['class'] == 'Interface')
        entry.set_editor_property('max_concurrent', spec['max_concurrent'])
        entries.append(entry)
    bank.set_editor_property('cues', entries)
    stored = bank.get_editor_property('cues')
    if len(stored) != len(entries) or (entries and str(stored[0].get_editor_property('cue')) != str(entries[0].get_editor_property('cue'))):
        raise RuntimeError(f'{path}: its cues read back wrong: nothing was saved')
    save(bank, path)

    for cue, spec in sorted(cues.items()):
        log(f"cue {cue}: {len(waves[cue])} sound(s), {spec['class']}, {spec['attenuation'] or '2D'}, "
            f"{spec['volume_db']:+.1f} dB, jitter {spec['pitch_jitter']:.2f}, {'a loop, ' if spec['loop'] else ''}"
            f"{spec['max_concurrent']} at once")
    gone = sorted(before - set(cues))
    for cue in gone:
        log(f'cue {cue} removed from the bank: no longer in cues.json')
    log(f"{path} {'made' if created else 'written'}: {len(entries)} cue(s)")
    return gone


# --- Run ---

def compare_with_code(cues):
    code = header_cues()
    for cue in sorted(code - set(cues)):
        warn(f'{cue} is played by the code but has no sound yet: it plays nothing')
    for cue in sorted(set(cues) - code):
        warn(f'{cue} is in cues.json but not in Audio/LooterSoundCues.h: nothing plays it (a typo, or a cue to add there)')


def run(args):
    if getattr(unreal, 'LooterSoundBank', None) is None or getattr(unreal, 'LooterSoundCueEntry', None) is None:
        raise RuntimeError('unreal.LooterSoundBank is missing: build the C++ with Audio/ first; nothing was changed')
    check_only = 'check' in args
    cues = read_cues()
    compare_with_code(cues)
    sound_count = sum(len(spec['files']) for spec in cues.values())
    if check_only:
        log(f'done (check only, nothing changed): {len(cues)} cue(s) with {sound_count} sound(s), {len(warnings)} warning(s)')
        return
    classes = build_classes()
    build_mix()
    attenuations = build_attenuations()
    waves, expected, imported, unchanged = build_waves(cues, classes, attenuations)
    # The bank first, so no wave about to go is still in it.
    gone = build_bank(cues, waves, classes, attenuations)
    removed = remove_stale_waves(expected)
    log(f'done: {len(cues)} cue(s), {sound_count} sound(s) ({imported} imported, {unchanged} unchanged), '
        f'{len(gone)} cue(s) and {len(removed)} wave(s) removed, {len(warnings)} warning(s)')


run(sys.argv[1:])
