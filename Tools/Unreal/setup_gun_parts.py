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
  Effect             what the part makes the gun do (EPartEffect: Ricochet, Burst, Explosive, Piercing, Overcharge,
                     Twin); empty = nothing. A gun has one at most: its first part with one (Phase 12.3).
  Element            the story element it gives the gun's rounds (ELooterElement: Ember, SoulLight, GraveSalt); empty =
                     none.
  Model              the key of another part of the same slot whose model it wears (SM_<Gun><Slot>_<Model>): an
                     element or effect part before it has a model of its own (the gun's glow says which); empty = its own.
Older sheets without the last three columns read as empty there.

The gun makers' mark (Affixes/WeaponMakers, Docs/Work/2026-10-22-Next/MAKERS.md): MARKS gives each family the slot whose
parts carry it and where on them (FWeaponMarkPlace, cm in the part's space: centre X along the gun, Z up, the mark's
size, and Depth, how far out from the part's middle a surface must be to take it). Laid out from the Blender scripts'
outlines; tune them by eye in PIE (a gun is marked by the first part in slot order with a place).

The new families (Phase 12.3): the Rustler SMG, Thresher LMG, Longhorn sniper (its fixed Bolt on the body's Bolt socket)
and Powderkeg launcher (its Magazine slot, the loaded rocket, hangs on the barrel's Rocket socket). Their definitions
(DA_SMG, DA_LMG, DA_Sniper, DA_Launcher) come from Tools/Unreal/create_new_guns.py: run that first; a family whose
definition is missing yet is skipped with a warning.
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
# A revolver's grip hangs from GripMount, since its own Grip socket is where the hand holds it.
SOCKETS = {'Body': None, 'Barrel': 'Barrel', 'Muzzle': 'Muzzle', 'Magazine': 'Magazine', 'Pump': 'Pump',
           'Sight': 'Sight', 'Stock': 'Stock', 'Cylinder': 'Cylinder', 'Grip': 'GripMount', 'Bolt': 'Bolt'}
# A family whose slot hangs elsewhere: the Powderkeg's loaded rocket sits in the barrel's mouth (SOCKET_Rocket).
GUN_SOCKETS = {'Powderkeg': {'Magazine': 'Rocket'}}

# Spreadsheet words -> the C++ enums (Python names them in capitals).
EFFECTS = {'Ricochet': 'RICOCHET', 'Burst': 'BURST', 'Explosive': 'EXPLOSIVE', 'Piercing': 'PIERCING',
           'Overcharge': 'OVERCHARGE', 'Twin': 'TWIN'}
ELEMENTS = {'Ember': 'EMBER', 'SoulLight': 'SOUL_LIGHT', 'Soul-light': 'SOUL_LIGHT', 'GraveSalt': 'GRAVE_SALT',
            'Grave-salt': 'GRAVE_SALT'}

# Where each family's parts carry the maker's mark: (slot, X, Z, size, depth), cm in that slot's part space. Clear of the
# notch tally rows (Weapons/WeaponModelNotches.cpp) where there's room; the Drover's grips are small, so the tally is cut
# over the mark once the gun has notches.
MARKS = {
    'Bullpup': ('Body', 29.0, -0.9, 5.0, 1.9),        # the shell's flank ahead of the trigger loop, behind the vents
    'Ranchhand': ('Body', 16.5, -0.6, 2.8, 1.1),      # the receiver's flank ahead of the ejection port
    'Drover': ('Grip', -1.9, -4.1, 2.6, 1.2),         # the grip panel, as a maker's medallion
    'Rustler': ('Body', 20.0, 0.4, 4.0, 1.5),         # the upper's flank ahead of the grip
    # The receiver's side ahead of the belt box, a little smaller and higher: at 5 cm, Z -0.8 the belt's rounds covered
    # its lower half on the -Y side, the side first person sees (PIE 2026-10-22; at X 4.5 first person never saw it).
    'Thresher': ('Body', 24.0, -0.4, 4.0, 2.2),
    # First person sees a gun from a little behind its grip forward (the Thresher's mark 4.5 cm behind its grip never
    # showed): the Longhorn's butt (X -6, 26 cm behind its grip at 19.8) and the Powderkeg's rear sleeve (X 24, 25 cm behind
    # its grip at 48.6) never did (PIE 2026-10-22). Both moved ahead of the grip; check them in PIE.
    # The chassis flank under the action, between the trigger and the magazine well (32.8), clear of the screws (21, 39)
    # and the accent strips at Z -3.25; depth 1.6 keeps it off the round receiver above (it reaches 1.49 out at Z -0.7).
    'Longhorn': ('Body', 29.5, -1.9, 2.4, 1.6),
    # The front sleeve's upper flank ahead of the grip, over the ember side line (Z 0), clear of the straps (55, 66) and
    # rivets (58, 64) of the Salvager; the Armored's side plates reach Z 1.8, the Salvager's side pipe runs at Z 3.3.
    'Powderkeg': ('Body', 61.0, 2.6, 2.2, 2.9),
}

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


def cell(row, column):
    """A cell that older sheets may not have (Effect, Element, Model): empty then."""
    return (row.get(column) or '').strip()


def enum_value(enum, table, word, what):
    if not word:
        return getattr(enum, 'NONE')
    if word not in table:
        raise RuntimeError(f'Unknown {what} "{word}" (one of {", ".join(sorted(table))})')
    return getattr(enum, table[word])


def option(gun, row):
    stats = unreal.WeaponPartStats(magazine=int(number(row['Magazine'])), zoom=number(row['Zoom']))
    for column, prop in STATS.items():
        stats.set_editor_property(prop, stat_range(row, column))
    model = cell(row, 'Model') or row['Key']
    part = unreal.WeaponPartOption(
        key=row['Key'], display_name=unreal.Text(row['Name']), mesh=mesh(f"{gun}{row['Slot']}_{model}"),
        weight=number(row['Weight'], 1.0), min_rarity=RARITIES[row['MinRarity']], stats=stats,
        length=number(row['Length']), requires=requirement(row['Needs']),
        name_prefix=unreal.Text(row['Word']), name_priority=int(number(row['NamePriority'])))
    part.set_editor_property('effect', enum_value(unreal.PartEffect, EFFECTS, cell(row, 'Effect'), 'effect'))
    part.set_editor_property('element', enum_value(unreal.LooterElement, ELEMENTS, cell(row, 'Element'), 'element'))
    mark = MARKS.get(gun)
    # Before the makers' C++ is built the struct isn't there yet: the parts are set up without marks.
    if mark and row['Slot'] == mark[0] and hasattr(unreal, 'WeaponMarkPlace'):
        part.set_editor_property('mark_place', unreal.WeaponMarkPlace(x=mark[1], z=mark[2], size=mark[3], depth=mark[4]))
    return part


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
        socket = GUN_SOCKETS.get(gun, {}).get(name, SOCKETS[name])
        if socket:
            part.set_editor_property('socket', socket)
        result.append(part)
    return result


def paint(material_slot, colors):
    """colors: (sRGB hex, weight) pairs."""
    def linear(value):
        srgb = unreal.Color(r=(value >> 16) & 0xFF, g=(value >> 8) & 0xFF, b=value & 0xFF, a=255)
        return unreal.MathLibrary.conv_color_to_linear_color(srgb)
    return unreal.WeaponPaint(slot=material_slot, colors=[unreal.WeaponColorOption(color=linear(c), weight=w) for c, w in colors])


def setup(asset, gun, kind, reload_slot, reload_part, full_damage_range, base_handling=1.0):
    definition = unreal.load_asset(f'{DATA}/{asset}')
    if definition is None:
        raise RuntimeError(f'{DATA}/{asset} is missing (DA_Revolver: run create_revolver.py first)')
    parts = slots(gun)
    definition.set_editor_property('kind', kind)
    definition.set_editor_property('parts', parts)
    definition.set_editor_property('paints', [paint('GunPolymerSand', SHELL_PAINTS), paint('GunPolymerGrey', FITTING_PAINTS)])
    definition.set_editor_property('rarity_glow_slot', 'GunAccentGlow')
    definition.set_editor_property('reload_slot', reload_slot)
    definition.set_editor_property('reload_part', reload_part)
    # The parts set the new stats around these: full damage out to the range, normal recoil, the kind's handling (a
    # six-gun comes up and aims quicker than a long gun), no zoom.
    base = definition.get_editor_property('base_stats')
    base.set_editor_property('range', full_damage_range)
    base.set_editor_property('recoil', 1.0)
    base.set_editor_property('handling', base_handling)
    base.set_editor_property('zoom', 1.0)
    definition.set_editor_property('base_stats', base)
    unreal.EditorAssetLibrary.save_loaded_asset(definition, only_if_is_dirty=False)
    unreal.log(f'GUNPARTS {asset}: {", ".join(f"{p.name} {len(p.options)}" for p in parts)}')


def reload_part(*names):
    """The first of names the C++ has (the new kinds' reload parts are GUNS-CODE's), else the magazine's."""
    for name in names:
        if hasattr(unreal.WeaponReloadPart, name):
            return getattr(unreal.WeaponReloadPart, name)
    return unreal.WeaponReloadPart.MAGAZINE


def setup_new(asset, gun, kind_name, reload_slot, reload_part_value, full_damage_range, base_handling):
    """A new family: skipped (with a warning) until create_new_guns.py has made its definition."""
    if unreal.load_asset(f'{DATA}/{asset}') is None:
        unreal.log_warning(f'GUNPARTS {asset} missing: run Tools/Unreal/create_new_guns.py first; {gun} skipped')
        return
    setup(asset, gun, getattr(unreal.WeaponKind, kind_name), reload_slot, reload_part_value, full_damage_range, base_handling)


setup('DA_AssaultRifle', 'Bullpup', unreal.WeaponKind.RIFLE, 'Magazine', unreal.WeaponReloadPart.MAGAZINE, 4000.0)
setup('DA_PumpShotgun', 'Ranchhand', unreal.WeaponKind.SHOTGUN, 'Pump', unreal.WeaponReloadPart.PUMP, 1500.0)
setup('DA_Revolver', 'Drover', unreal.WeaponKind.REVOLVER, 'Cylinder', unreal.WeaponReloadPart.CYLINDER, 3000.0, 1.3)
# The four new kinds (Phase 12.3): full-damage ranges by role, handling the kind's (an SMG comes up quickest, an LMG and a
# launcher slowest). GUNS-CODE's kick/aim tuning sits on top of these.
setup_new('DA_SMG', 'Rustler', 'SMG', 'Magazine', reload_part('HELIX', 'MAGAZINE'), 2200.0, 1.25)
setup_new('DA_LMG', 'Thresher', 'LMG', 'Magazine', reload_part('BOX', 'MAGAZINE'), 3500.0, 0.8)
setup_new('DA_Sniper', 'Longhorn', 'SNIPER', 'Magazine', reload_part('BOLT', 'MAGAZINE'), 9000.0, 0.85)
setup_new('DA_Launcher', 'Powderkeg', 'LAUNCHER', 'Magazine', reload_part('ROCKET', 'MAGAZINE'), 4000.0, 0.8)
unreal.log('GUNPARTS done')
