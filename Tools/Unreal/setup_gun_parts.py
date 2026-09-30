"""Fills the guns' part lists (UWeaponDefinition's Parts, Paints and reload part) with the parts that
Art/Models/Weapons/*.py make, and saves the definitions. Run it in the open editor after importing the parts:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/setup_gun_parts.py"
It rebuilds the lists from scratch, so run it again after adding or renaming a part.
"""
import unreal

PARTS = '/Game/Art/Weapons'
DATA = '/Game/Weapons/Data'
Rarity = unreal.WeaponRarity

# Panel paints a rolled gun picks from (sRGB).
PANEL_PAINTS = [0xe4dccb, 0xd9a766, 0x4f9e98, 0xe0795c, 0x8fb7d9, 0xc9d36a]
RUBBER = 0x23272e
WOOD = 0x7a4f32


def mesh(name):
    asset = unreal.load_asset(f'{PARTS}/SM_{name}')
    if asset is None:
        raise RuntimeError(f'{PARTS}/SM_{name} is missing: import Art/Models/Weapons first (Tools/models.ps1)')
    return asset


def option(name, weight=1.0, min_rarity=Rarity.COMMON):
    return unreal.WeaponPartOption(mesh=mesh(name), weight=weight, min_rarity=min_rarity)


def slot(name, options, socket=None):
    part = unreal.WeaponPartSlot(name=name, options=options)
    if socket:
        part.set_editor_property('socket', socket)
    return part


def paint(material_slot, colors):
    """colors: (sRGB hex, weight) pairs."""
    def linear(value):
        srgb = unreal.Color(r=(value >> 16) & 0xFF, g=(value >> 8) & 0xFF, b=value & 0xFF, a=255)
        return unreal.MathLibrary.conv_color_to_linear_color(srgb)
    return unreal.WeaponPaint(slot=material_slot, colors=[unreal.WeaponColorOption(color=linear(c), weight=w) for c, w in colors])


def setup(asset, kind, parts, paints, reload_slot, reload_part):
    definition = unreal.load_asset(f'{DATA}/{asset}')
    if definition is None:
        raise RuntimeError(f'{DATA}/{asset} is missing')
    definition.set_editor_property('kind', kind)
    definition.set_editor_property('parts', parts)
    definition.set_editor_property('paints', paints)
    definition.set_editor_property('rarity_glow_slot', 'GunAccent')
    definition.set_editor_property('reload_slot', reload_slot)
    definition.set_editor_property('reload_part', reload_part)
    unreal.EditorAssetLibrary.save_loaded_asset(definition, only_if_is_dirty=False)
    unreal.log(f'GUNPARTS {asset}: {len(parts)} part slots, {len(paints)} paints')


panel = paint('GunPaint', [(color, 1.0) for color in PANEL_PAINTS])

setup('DA_AssaultRifle', unreal.WeaponKind.RIFLE, [
    slot('Body', [option('RifleBody')]),
    slot('Barrel', [option('RifleBarrel10'), option('RifleBarrel14'), option('RifleBarrel18')]),
    slot('Sight', [option('RifleSightIron'), option('RifleSightRedDot'), option('RifleSightScope')]),
    slot('Magazine', [option('RifleMagazine')], socket='Magazine'),
    slot('Fins', [option('RifleFins', min_rarity=Rarity.LEGENDARY)]),
], [panel], 'Magazine', unreal.WeaponReloadPart.MAGAZINE)

setup('DA_PumpShotgun', unreal.WeaponKind.SHOTGUN, [
    slot('Body', [option('ShotgunBody')]),
    slot('Barrel', [option('ShotgunBarrel30'), option('ShotgunBarrel34'), option('ShotgunBarrel38')]),
    slot('Pump', [option('ShotgunPump')], socket='Pump'),
    slot('Shroud', [option('ShotgunShroud', min_rarity=Rarity.EPIC)], socket='Shroud'),
], [panel, paint('GunGrip', [(WOOD, 0.6), (RUBBER, 0.4)])], 'Pump', unreal.WeaponReloadPart.PUMP)

unreal.log('GUNPARTS done')
