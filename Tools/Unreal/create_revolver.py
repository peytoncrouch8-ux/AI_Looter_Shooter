"""Makes (or brings up to date) the Drover revolver's definition, /Game/Weapons/Data/DA_Revolver, and puts it in the default
loot table beside the rifle and the shotgun. Run it in the open editor once the C++ with EWeaponKind::Revolver is built,
before Tools/Unreal/setup_gun_parts.py (which fills its parts from Art/Models/Weapons/Drover.parts.csv):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_revolver.py"
Then run setup_gun_parts.py, and create_rank_assets.py so the ranks' loot tables take the new gun too. It prints REVOLVER
lines and "REVOLVER done". Running it again puts the numbers below back and leaves the parts (setup_gun_parts.py's) alone.

The numbers, against the rifle (DA_AssaultRifle: 18 damage at 650 rounds a minute, 0.8 degrees of spread, 40 m) and the
shotgun (DA_PumpShotgun: 8 pellets of 12 at 75 a minute, 5 degrees, 15 m):
  a heavy single shot of 52 (nearly three rifle bullets) at 140 a minute, semi-automatic: about the shotgun's damage a
  second and two thirds of the rifle's, paid for in precision, since its 0.55 degrees are the tightest cone of the three;
  full damage out to 30 m; six rounds a cylinder (its part sets how many, and rarity never changes it, so the cylinder
  you see is the rounds you have); a 2.3 s reload; it comes up and aims quicker than a long gun (handling 1.3, set by
  setup_gun_parts.py). One cylinder fells a spider (300 health) on body shots, fewer with crits.
  The kick: a sharp muzzle flip (8 degrees, between the rifle's 2.5 and the shotgun's 11) on a quick spring
  (snappiness 1.5) whose aim kick settles fast (recovery 9), so the sights are back before the next pull.
"""
import unreal

DATA = '/Game/Weapons/Data'
ASSET = 'DA_Revolver'
LOOT = f'{DATA}/DA_LootTable_Default'

STATS = dict(damage=52.0, fire_rate=140.0, magazine_size=6, reload_time=2.3, spread=0.55, pellets_per_shot=1)
RECOIL = dict(kick_back=3.6, muzzle_flip=8.0, muzzle_twist=1.0, roll=2.2, aim_kick=1.0, aim_kick_side=0.15,
              aim_recovery=9.0, snappiness=1.5)


def definition():
    path = f'{DATA}/{ASSET}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, unreal.WeaponDefinition):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.WeaponDefinition)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(ASSET, DATA, unreal.WeaponDefinition, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def run():
    if not hasattr(unreal.WeaponKind, 'REVOLVER') or not hasattr(unreal.WeaponReloadPart, 'CYLINDER'):
        raise RuntimeError('unreal.WeaponKind.REVOLVER is missing: build the C++ with the revolver first; nothing was changed')
    gun, created = definition()
    gun.set_editor_property('display_name', unreal.Text('Drover'))
    gun.set_editor_property('kind', unreal.WeaponKind.REVOLVER)
    gun.set_editor_property('fire_mode', unreal.WeaponFireMode.SEMI_AUTO)
    gun.set_editor_property('ammo_type', unreal.AmmoType.PISTOL)
    gun.set_editor_property('starting_reserve_magazines', 4)

    base = gun.get_editor_property('base_stats')
    for name, value in STATS.items():
        base.set_editor_property(name, value)
    gun.set_editor_property('base_stats', base)

    # Rarity raises its damage, rate, reload and spread as any gun's, but never its capacity: a six-gun holds six.
    table = gun.get_editor_property('rarity_table')
    for rarity in list(table.keys()):
        info = table[rarity]
        info.set_editor_property('magazine_multiplier', 1.0)
        table[rarity] = info
    gun.set_editor_property('rarity_table', table)

    recoil = gun.get_editor_property('recoil')
    for name, value in RECOIL.items():
        recoil.set_editor_property(name, value)
    gun.set_editor_property('recoil', recoil)

    # A heavy round: a harder shove on what it hits, a quick bullet, a tracer between the rifle's and the shotgun's, and
    # a big flash from the muzzle and the cylinder gap.
    gun.set_editor_property('hit_impulse', 14000.0)
    gun.set_editor_property('bullet_speed', 26000.0)
    gun.set_editor_property('tracer_width', 2.4)
    gun.set_editor_property('tracer_length', 300.0)
    gun.set_editor_property('muzzle_flash_scale', 1.35)
    # Held out at arm's length in the third person (the stance layer) and pushed out ahead of a long gun's hold in the first.
    gun.set_editor_property('hold_reach', 14.0)

    if not unreal.EditorAssetLibrary.save_loaded_asset(gun, only_if_is_dirty=False):
        raise RuntimeError(f'{DATA}/{ASSET} could not be saved')
    unreal.log(f"REVOLVER {DATA}/{ASSET} {'made' if created else 'updated'}: {STATS['damage']} damage at "
               f"{STATS['fire_rate']} a minute, {STATS['spread']} degrees, semi-automatic, pistol ammo")

    loot = unreal.load_asset(LOOT)
    if loot is None:
        raise RuntimeError(f'{LOOT} is missing')
    entries = list(loot.get_editor_property('entries'))
    if any(entry.get_editor_property('weapon') == gun for entry in entries):
        unreal.log(f'REVOLVER {LOOT} already drops it')
    else:
        entry = unreal.LootTableEntry()
        entry.set_editor_property('weapon', gun)
        entry.set_editor_property('weight', 1.0)
        entries.append(entry)
        loot.set_editor_property('entries', entries)
        if not unreal.EditorAssetLibrary.save_loaded_asset(loot, only_if_is_dirty=False):
            raise RuntimeError(f'{LOOT} could not be saved')
        unreal.log(f'REVOLVER {LOOT} drops it now: {len(entries)} kinds of gun, each as likely')
    unreal.log('REVOLVER done')


run()
