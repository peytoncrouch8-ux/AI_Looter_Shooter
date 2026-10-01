"""Fills the guns' part lists (UWeaponDefinition's Parts, Paints and reload part) from the parts spreadsheets
Art/Models/Weapons/<Gun>.parts.csv, and saves the definitions. Run it in the open editor after importing the parts:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/setup_gun_parts.py"
It rebuilds the lists from scratch, so run it again after changing a spreadsheet or adding a part.

One row per part option (the model is SM_<Gun><Slot>_<Key>, made by Art/Models/Weapons/<Gun>.py):
  Slot, Key          the slot it fills and its name there. Guns save their parts by key: never rename one that has dropped.
  Name, Word         what it's called, and the word it can put before the gun's name ("Scoped").
  MinRarity          the lowest rarity it comes on: rarity unlocks the better parts.
  Weight             its chance against the slot's other options.
  NamePriority       the gun's part with the highest priority (and a word) names it.
  Magazine, Zoom     the capacity (before rarity) and magnification it sets outright; empty = it doesn't.
  Length             cm, for parts others depend on (barrels).
  Needs              a rule like barrel>=46: only with an earlier slot's part at least that long.
  <Stat>Min, Max     its change to Damage, Accuracy, Range, FireRate, Reload, Recoil and Handling in percent; each gun
                     rolls its own amount between the two. A gun's percentages add up per stat, capped (WeaponParts.h).
"""
import csv
import os

import unreal

PARTS = '/Game/Art/Weapons'
DATA = '/Game/Weapons/Data'
SHEETS = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), 'Art', 'Models', 'Weapons')
Rarity = unreal.WeaponRarity
RARITIES = {'Common': Rarity.COMMON, 'Uncommon': Rarity.UNCOMMON, 'Rare': Rarity.RARE, 'Epic': Rarity.EPIC,
            'Legendary': Rarity.LEGENDARY}
# Spreadsheet column -> FWeaponPartStats property.
STATS = {'Damage': 'damage', 'Accuracy': 'accuracy', 'Range': 'range', 'FireRate': 'fire_rate', 'Reload': 'reload',
         'Recoil': 'recoil', 'Handling': 'handling'}

# Where each slot's part hangs: the socket on an earlier part (the latest one that has it); the body sits at the origin.
SOCKETS = {'Body': None, 'Barrel': 'Barrel', 'Muzzle': 'Muzzle', 'Magazine': 'Magazine', 'Pump': 'Pump',
           'Sight': 'Sight', 'Stock': 'Stock'}

# Each gun's shell is tinted per gun (sRGB hex, weight): mostly field colors, sometimes a bright one.
SHELL_PAINTS = [(0xe6d8b6, 3.0), (0xe8e8e4, 2.0), (0xc9ad84, 2.0), (0x8c7a5b, 1.5), (0x6f7550, 1.5), (0x5d6b78, 1.0),
                (0x38393b, 1.0), (0x4f9e98, 0.5), (0xe0795c, 0.5), (0xb8402f, 0.4)]
# The grey polymer fittings (grips, pads, guards) go darker or lighter with it.
FITTING_PAINTS = [(0x6a6d70, 3.0), (0x38393b, 2.0), (0x56503f, 1.0), (0x8a8d90, 1.0)]


def mesh(name):
    asset = unreal.load_asset(f'{PARTS}/SM_{name}')
    if asset is None:
        raise RuntimeError(f'{PARTS}/SM_{name} is missing: import Art/Models/Weapons first (Tools/models.ps1)')
    return asset


def number(text, default=0.0):
    return float(text) if text.strip() else default


def stat_range(row, stat):
    low, high = number(row[f'{stat}Min']), number(row[f'{stat}Max'])
    return unreal.WeaponStatRange(min=min(low, high), max=max(low, high))


def requirement(needs):
    """'barrel>=46' -> needs the Barrel slot's part to be at least 46 cm long."""
    if not needs.strip():
        return unreal.WeaponPartRequirement()
    slot, length = needs.split('>=')
    return unreal.WeaponPartRequirement(slot=slot.strip().capitalize(), min_length=float(length))


def option(gun, row):
    stats = unreal.WeaponPartStats(magazine=int(number(row['Magazine'])), zoom=number(row['Zoom']))
    for column, prop in STATS.items():
        stats.set_editor_property(prop, stat_range(row, column))
    return unreal.WeaponPartOption(
        key=row['Key'], display_name=unreal.Text(row['Name']), mesh=mesh(f"{gun}{row['Slot']}_{row['Key']}"),
        weight=number(row['Weight'], 1.0), min_rarity=RARITIES[row['MinRarity']], stats=stats,
        length=number(row['Length']), requires=requirement(row['Needs']),
        name_prefix=unreal.Text(row['Word']), name_priority=int(number(row['NamePriority'])))


def slots(gun):
    """The gun's slots in spreadsheet order (a part's socket and requirement can only name an earlier slot)."""
    order, options = [], {}
    with open(os.path.join(SHEETS, f'{gun}.parts.csv'), newline='', encoding='utf-8') as sheet:
        for row in csv.DictReader(sheet):
            if row['Slot'] not in options:
                order.append(row['Slot'])
                options[row['Slot']] = []
            options[row['Slot']].append(option(gun, row))
    result = []
    for name in order:
        part = unreal.WeaponPartSlot(name=name, options=options[name])
        if SOCKETS[name]:
            part.set_editor_property('socket', SOCKETS[name])
        result.append(part)
    return result


def paint(material_slot, colors):
    """colors: (sRGB hex, weight) pairs."""
    def linear(value):
        srgb = unreal.Color(r=(value >> 16) & 0xFF, g=(value >> 8) & 0xFF, b=value & 0xFF, a=255)
        return unreal.MathLibrary.conv_color_to_linear_color(srgb)
    return unreal.WeaponPaint(slot=material_slot, colors=[unreal.WeaponColorOption(color=linear(c), weight=w) for c, w in colors])


def setup(asset, gun, kind, reload_slot, reload_part, full_damage_range):
    definition = unreal.load_asset(f'{DATA}/{asset}')
    if definition is None:
        raise RuntimeError(f'{DATA}/{asset} is missing')
    parts = slots(gun)
    definition.set_editor_property('kind', kind)
    definition.set_editor_property('parts', parts)
    definition.set_editor_property('paints', [paint('GunPolymerSand', SHELL_PAINTS), paint('GunPolymerGrey', FITTING_PAINTS)])
    definition.set_editor_property('rarity_glow_slot', 'GunAccentGlow')
    definition.set_editor_property('reload_slot', reload_slot)
    definition.set_editor_property('reload_part', reload_part)
    # The parts set the new stats around these: full damage out to the range, normal recoil and handling, no zoom.
    base = definition.get_editor_property('base_stats')
    base.set_editor_property('range', full_damage_range)
    base.set_editor_property('recoil', 1.0)
    base.set_editor_property('handling', 1.0)
    base.set_editor_property('zoom', 1.0)
    definition.set_editor_property('base_stats', base)
    unreal.EditorAssetLibrary.save_loaded_asset(definition, only_if_is_dirty=False)
    unreal.log(f'GUNPARTS {asset}: {", ".join(f"{p.name} {len(p.options)}" for p in parts)}')


setup('DA_AssaultRifle', 'Bullpup', unreal.WeaponKind.RIFLE, 'Magazine', unreal.WeaponReloadPart.MAGAZINE, 4000.0)
setup('DA_PumpShotgun', 'Ranchhand', unreal.WeaponKind.SHOTGUN, 'Pump', unreal.WeaponReloadPart.PUMP, 1500.0)
unreal.log('GUNPARTS done')
