"""Makes the named guns (UNamedWeaponDefinition, Weapons/NamedWeaponDefinition.h) in /Game/Data/Weapons and saves them.
Run it in the open editor once the C++ with named guns is built, after Tools/Unreal/setup_gun_parts.py has filled the
guns' parts from their spreadsheets:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_named_weapons.py"
It prints one NAMEDGUNS line per gun and "NAMEDGUNS done" at the end. Running it again puts the fields listed here back
and leaves every other field as it is. Each gun is checked (UNamedWeaponDefinition::FindProblems) before it's saved:
a part its slot doesn't have, one that doesn't come at its rarity or doesn't fit stops the script with nothing saved.

  asset     DA_Named_<Id>; mission rewards (FMissionRewards::NamedGun) and Looter.GiveWeapon name the gun by its id.
  weapon    the kind of gun it is, in /Game/Weapons/Data.
  name      its name wherever a gun's name shows, in place of the one its parts would give it.
  line      the flavor line its card and label show under the name.
  rarity    every copy is this rare, and each part must come on guns this rare (the spreadsheet's MinRarity).
  parts     a part for every slot of its kind, slot: key, as Art/Models/Weapons/<Gun>.parts.csv names them (Slot, Key).
  quality   where its stats sit in what its parts' ranges and its kind's variance allow, the same on every copy: 0 the
            worse end of each, 1 the better.
  wear      how worn it looks, 0 (factory fresh) to 1 (battered); below 0, rolled from its seed as any gun's.
  seed      every copy is made with it: its paint, and its wear when it has none of its own.
"""
import unreal

FOLDER = '/Game/Data/Weapons'
GUNS = '/Game/Weapons/Data'
Rarity = unreal.WeaponRarity
RARITIES = {'Common': Rarity.COMMON, 'Uncommon': Rarity.UNCOMMON, 'Rare': Rarity.RARE, 'Epic': Rarity.EPIC,
            'Legendary': Rarity.LEGENDARY}

NAMED = [
    # Heirloom (Main 7, The Lantern Leans; Docs/Areas/RansomsRest.md, step 23): Abel Ransom's Ranchhand pump. A keeper
    # is buried with his lantern, not his iron, so Delia kept it back and hands it out through the door. A named Epic (the
    # user's call, 2026-10-07): a keeper's working gun, worn but cared for. Its special effect waits for unique legendaries.
    # Its parts match its spectral twin, which Abel's ghost carries (SM_AbelPump: a classic walnut pump, a 56 cm barrel
    # with a bead over its tube); the user picked this set (candidate B). Every one comes at Epic, and none needs another:
    # together they add +2 to +4% damage, +11 to +29% accuracy and +15 to +35% range for -22 to 0% handling, and hold 6
    # shells. Its own name wins over its parts' word (the Trap barrel's would name it "Trap Pump Shotgun").
    dict(asset='DA_Named_Heirloom', weapon='DA_PumpShotgun', name='Heirloom', line='Hold the door.', rarity='Epic',
         parts={
             'Body': 'Heritage',   # Uncommon+. Case-colored with brass trim: the family's old receiver, kept oiled.
             'Barrel': 'Trap',     # Rare+. Long (56 cm, as the twin's), a high rib and two beads: it reaches the road.
             'Muzzle': 'Crown',    # Common. A plain flush choke, nothing added: the bead at the muzzle, as on the twin.
             'Magazine': 'Tube6',  # Common. The Ranchhand's own 6-shell tube under the barrel, as on the twin (8 at Epic).
             'Sight': 'Flip',      # Common. The Ranchhand's own flip-up rear over the bead: no glass on a keeper's gun.
             'Stock': 'Field',     # Common. The farmhand's walnut with a leather shell cuff: Abel farmed.
             'Pump': 'Walnut',     # Common. The walnut pump every Ranchhand has.
         },
         # Three quarters of the way to the best of every range: better kept than an average gun, never a perfect one,
         # and the same Heirloom in every game.
         quality=0.75,
         # Honest wear on the edges of a gun that was looked after (Epic guns roll 0.05-0.45, commons 0.45-1).
         wear=0.35,
         seed=1),
]

# What the C++ declares, by Python name; a field missing means the C++ with named guns isn't built.
FIELDS = ('weapon', 'display_name', 'flavor_text', 'rarity', 'parts', 'stat_quality', 'wear', 'seed')


def named_class():
    cls = getattr(unreal, 'NamedWeaponDefinition', None)
    if cls is None:
        raise RuntimeError('unreal.NamedWeaponDefinition is missing: build the C++ with Weapons/NamedWeaponDefinition.h first')
    defaults = unreal.get_default_object(cls)
    for field in FIELDS:
        try:
            defaults.get_editor_property(field)
        except Exception:
            raise RuntimeError(f'unreal.NamedWeaponDefinition has no {field}: build the C++ with named guns first; nothing was changed')
    return cls


def load_or_create(asset_name, cls):
    path = f'{FOLDER}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}, not a NamedWeaponDefinition: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, FOLDER, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def part_names(weapon, parts):
    """'Body Heritage receiver, Barrel Heritage barrel, ...' in the gun's slot order, for the log."""
    names = []
    for slot in weapon.get_editor_property('parts'):
        slot_name = str(slot.get_editor_property('name'))
        key = parts.get(slot_name)
        option = next((o for o in slot.get_editor_property('options') if str(o.get_editor_property('key')) == key), None)
        names.append(f"{slot_name} {option.get_editor_property('display_name') if option else key}")
    return ', '.join(names)


def setup(spec, cls):
    weapon = unreal.load_asset(f"{GUNS}/{spec['weapon']}")
    if weapon is None:
        raise RuntimeError(f"{GUNS}/{spec['weapon']} is missing")
    if not 0.0 <= spec['quality'] <= 1.0 or spec['wear'] > 1.0:
        raise RuntimeError(f"{spec['asset']}: quality is 0 to 1 and wear at most 1")
    asset, created = load_or_create(spec['asset'], cls)
    asset.set_editor_property('weapon', weapon)
    asset.set_editor_property('display_name', unreal.Text(spec['name']))
    asset.set_editor_property('flavor_text', unreal.Text(spec['line']))
    asset.set_editor_property('rarity', RARITIES[spec['rarity']])
    asset.set_editor_property('parts', dict(spec['parts']))
    asset.set_editor_property('stat_quality', spec['quality'])
    asset.set_editor_property('wear', spec['wear'])
    asset.set_editor_property('seed', spec['seed'])
    stored = {str(slot): str(key) for slot, key in asset.get_editor_property('parts').items()}
    if stored != spec['parts']:
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: its parts read back as {stored}, not {spec['parts']}: nothing was saved")
    problems = list(asset.find_problems())
    if problems:
        raise RuntimeError(f"{FOLDER}/{spec['asset']}: {'; '.join(problems)}: nothing was saved")
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"{FOLDER}/{spec['asset']} could not be saved")
    unreal.log(f"NAMEDGUNS {spec['asset']} {'made' if created else 'updated'}: \"{spec['name']}\", {spec['rarity']} {spec['weapon']} "
               f"({part_names(weapon, spec['parts'])}), line \"{spec['line']}\", quality {spec['quality']}, wear {spec['wear']}, "
               f"seed {spec['seed']}")


def run():
    cls = named_class()
    for spec in NAMED:
        setup(spec, cls)
    unreal.log('NAMEDGUNS done')


run()
