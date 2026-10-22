"""Makes (or brings up to date) the four frontier-tech guns' definitions (Phase 12.3, Saved/Loot/CONTRACT.md "GUNS-CODE")
in /Game/Weapons/Data and puts them in the default loot table, with rockets among its ammo:
  DA_SMG       the Rustler (Art/Models/Weapons/Rustler.py): EWeaponKind::SMG, SMG ammo, full auto, the helix the reload's part
  DA_Sniper    the Longhorn (Longhorn.py): Sniper, sniper rounds, one shot a pull, the bolt and its box the reload's
  DA_LMG       the Thresher (Thresher.py): LMG, rifle ammo, full auto, the belt box the reload's
  DA_Launcher  the Powderkeg (Powderkeg.py): Launcher, rockets, one a pull, the rocket in the mouth the reload's; its
               projectile is SM_PowderkegRocket
Run it in the open editor once the C++ with the four kinds is built and the four families' parts are imported
(Tools/models.ps1), then setup_gun_parts.py (which fills their parts from the .parts.csv sheets and sets each one's kind,
reload slot and part, range and handling: its setup_new lines), then create_rank_assets.py so
the ranks' loot tables take the new guns and the rockets too:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_new_guns.py"
It prints NEWGUNS lines and "NEWGUNS done". Running it again puts the numbers below back and leaves the parts alone.

The numbers, against the rifle (DA_AssaultRifle: 18 damage at 650 a minute, 0.8 degrees, 40 m) and the revolver (52 at
140, 0.55 degrees, 30 m). The parts move every stat around these (WeaponParts' caps); the magazines and sights set the
capacity and zoom outright, and rarity never changes a capacity (what the helix, the box or the rockets show is what's in
it, as the six-gun's cylinder):
  SMG       11 a round at 900 a minute: a little under the rifle's damage a second, all of it up close, its cone a
            touch wider (1.4) and blooming as it's held, full damage to 22 m; a quick 2.1 s reload; the quickest hands
            of any gun (handling 1.25, Looter.Gunfeel.Swap.SMG). A light, fast kick that climbs a little.
  Sniper    150 a round at 45 a minute, 5 to a box: two body shots fell a spider, one crit (2x: Looter.Gunfeel.Crit.Sniper);
            full damage to 90 m; a slow hand (0.85) and a 3 s reload. Its 4-degree cone is its hip spread: through the
            scope it's 4% of that (Looter.Gunfeel.Sniper.AimedSpread). A hard punch on a slow spring.
  LMG       16 a round at 650 a minute once spun up (60% cold, Looter.Gunfeel.LMG.*), 80-150 to a box: the rifle's
            damage a second held for three times as long; full damage to 35 m; heavy (0.8), a 4.2 s reload. A steady,
            heavy kick that climbs.
  Launcher  one 320 burst a pull (1-3 rockets by the rocket's kind), at most 40 a minute, a 2.8 s reload; full damage
            in the burst's heart, falling off to its 3.5 m edge (Looter.Gunfeel.Launcher.*). The heaviest shove.
"""
import unreal

DATA = '/Game/Weapons/Data'
PARTS = '/Game/Art/Weapons'
LOOT = f'{DATA}/DA_LootTable_Default'
ROCKET_MESH = f'{PARTS}/SM_PowderkegRocket'
# Rockets drop half as often as each other class (LooterLoot::RocketAmmoWeight).
ROCKET_AMMO_WEIGHT = 0.5

# asset: (display name, kind, fire mode, ammo, starting magazines, base stats, recoil, firing, loot weight)
GUNS = {
    'DA_SMG': dict(
        name='SMG', kind='SMG', mode='FULL_AUTO', ammo='SMG', magazines=3,
        stats=dict(damage=11.0, fire_rate=900.0, magazine_size=40, reload_time=2.1, spread=1.4, pellets_per_shot=1),
        recoil=dict(kick_back=1.6, muzzle_flip=1.6, muzzle_twist=0.6, roll=1.0, aim_kick=0.22, aim_kick_side=0.14,
                    aim_recovery=8.0, snappiness=1.6),
        firing=dict(hit_impulse=12000.0, bullet_speed=28000.0, tracer_width=2.2, tracer_length=300.0, muzzle_flash_scale=0.85),
        weight=1.0),
    'DA_Sniper': dict(
        name='Sniper Rifle', kind='SNIPER', mode='SEMI_AUTO', ammo='SNIPER', magazines=4,
        stats=dict(damage=150.0, fire_rate=45.0, magazine_size=5, reload_time=3.0, spread=4.0, pellets_per_shot=1),
        recoil=dict(kick_back=5.0, muzzle_flip=9.0, muzzle_twist=1.2, roll=2.5, aim_kick=1.6, aim_kick_side=0.2,
                    aim_recovery=5.0, snappiness=1.1),
        firing=dict(hit_impulse=30000.0, bullet_speed=60000.0, tracer_width=3.5, tracer_length=700.0, muzzle_flash_scale=1.6),
        weight=0.9),
    'DA_LMG': dict(
        name='LMG', kind='LMG', mode='FULL_AUTO', ammo='ASSAULT_RIFLE', magazines=2,
        stats=dict(damage=16.0, fire_rate=650.0, magazine_size=100, reload_time=4.2, spread=1.6, pellets_per_shot=1),
        recoil=dict(kick_back=2.6, muzzle_flip=2.2, muzzle_twist=0.9, roll=1.6, aim_kick=0.3, aim_kick_side=0.18,
                    aim_recovery=6.0, snappiness=0.85),
        firing=dict(hit_impulse=22000.0, bullet_speed=32000.0, tracer_width=3.2, tracer_length=420.0, muzzle_flash_scale=1.25),
        weight=0.8),
    'DA_Launcher': dict(
        name='Rocket Launcher', kind='LAUNCHER', mode='SEMI_AUTO', ammo='ROCKET', magazines=3,
        stats=dict(damage=320.0, fire_rate=40.0, magazine_size=1, reload_time=2.8, spread=0.6, pellets_per_shot=1),
        recoil=dict(kick_back=7.0, muzzle_flip=7.0, muzzle_twist=1.5, roll=3.0, aim_kick=1.8, aim_kick_side=0.3,
                    aim_recovery=4.0, snappiness=0.8),
        firing=dict(hit_impulse=90000.0, bullet_speed=4000.0, tracer_width=4.0, tracer_length=200.0, muzzle_flash_scale=2.4),
        weight=0.6),
}
# What setup_gun_parts.py sets for these (reload slot and part); written here too so a run of this alone is consistent.
RELOAD = {'DA_SMG': ('Magazine', 'HELIX'), 'DA_Sniper': ('Magazine', 'BOLT'), 'DA_LMG': ('Magazine', 'BOX'),
          'DA_Launcher': ('Magazine', 'ROCKET')}


def log(message):
    unreal.log(f'NEWGUNS {message}')


def definition(asset_name):
    path = f'{DATA}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, unreal.WeaponDefinition):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.WeaponDefinition)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, DATA, unreal.WeaponDefinition, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def make_gun(asset_name, spec):
    gun, created = definition(asset_name)
    gun.set_editor_property('display_name', unreal.Text(spec['name']))
    gun.set_editor_property('kind', getattr(unreal.WeaponKind, spec['kind']))
    gun.set_editor_property('fire_mode', getattr(unreal.WeaponFireMode, spec['mode']))
    gun.set_editor_property('ammo_type', getattr(unreal.AmmoType, spec['ammo']))
    gun.set_editor_property('starting_reserve_magazines', spec['magazines'])
    slot, part = RELOAD[asset_name]
    gun.set_editor_property('reload_slot', slot)
    gun.set_editor_property('reload_part', getattr(unreal.WeaponReloadPart, part))

    base = gun.get_editor_property('base_stats')
    for name, value in spec['stats'].items():
        base.set_editor_property(name, value)
    gun.set_editor_property('base_stats', base)

    # Rarity raises damage, rate, reload and spread as any gun's, never the capacity: the helix, the box and the rockets
    # show what's in them.
    table = gun.get_editor_property('rarity_table')
    for rarity in list(table.keys()):
        info = table[rarity]
        info.set_editor_property('magazine_multiplier', 1.0)
        table[rarity] = info
    gun.set_editor_property('rarity_table', table)

    recoil = gun.get_editor_property('recoil')
    for name, value in spec['recoil'].items():
        recoil.set_editor_property(name, value)
    gun.set_editor_property('recoil', recoil)
    for name, value in spec['firing'].items():
        gun.set_editor_property(name, value)
    # Long guns: held in, the hip hold from the C++ table (WeaponHipHold.cpp), no hold of their own.
    gun.set_editor_property('hold_reach', 0.0)
    gun.set_editor_property('first_person_grip', unreal.Vector(0.0, 0.0, 0.0))
    gun.set_editor_property('first_person_rotation', unreal.Rotator(0.0, 0.0, 0.0))

    if spec['kind'] == 'LAUNCHER':
        rocket = unreal.load_asset(ROCKET_MESH)
        if rocket is None:
            unreal.log_warning(f'NEWGUNS {ROCKET_MESH} is missing (import Art/Models/Weapons/Powderkeg.py): the rocket flies as a plain shape')
        gun.set_editor_property('projectile_mesh', rocket)

    if not unreal.EditorAssetLibrary.save_loaded_asset(gun, only_if_is_dirty=False):
        raise RuntimeError(f'{DATA}/{asset_name} could not be saved')
    stats = spec['stats']
    log(f"{DATA}/{asset_name} {'made' if created else 'updated'}: {stats['damage']} damage at {stats['fire_rate']} a minute, "
        f"{stats['spread']} degrees, {spec['mode'].lower()}, {spec['ammo'].lower()} ammo")
    return gun


def add_to_loot(guns):
    loot = unreal.load_asset(LOOT)
    if loot is None:
        raise RuntimeError(f'{LOOT} is missing')
    entries = list(loot.get_editor_property('entries'))
    for asset_name, gun in guns.items():
        weight = GUNS[asset_name]['weight']
        found = next((entry for entry in entries if entry.get_editor_property('weapon') == gun), None)
        if found is None:
            found = unreal.LootTableEntry()
            found.set_editor_property('weapon', gun)
            entries.append(found)
        found.set_editor_property('weight', weight)
    loot.set_editor_property('entries', entries)

    # Rockets among its ammo (the rank tables copy them: create_rank_assets.py), half as likely as each other class.
    ammo = list(loot.get_editor_property('ammo_types'))
    rocket = next((entry for entry in ammo if entry.get_editor_property('type') == unreal.AmmoType.ROCKET), None)
    if rocket is None:
        rocket = unreal.AmmoLootEntry()
        rocket.set_editor_property('type', unreal.AmmoType.ROCKET)
        ammo.append(rocket)
    rocket.set_editor_property('weight', ROCKET_AMMO_WEIGHT)
    loot.set_editor_property('ammo_types', ammo)
    if not unreal.EditorAssetLibrary.save_loaded_asset(loot, only_if_is_dirty=False):
        raise RuntimeError(f'{LOOT} could not be saved')
    names = ', '.join(f'{entry.get_editor_property("weapon").get_name()} {entry.get_editor_property("weight")}' for entry in entries
                      if entry.get_editor_property('weapon'))
    log(f'{LOOT} drops {len(entries)} kinds of gun ({names}) and {len(ammo)} ammo classes, rockets at {ROCKET_AMMO_WEIGHT}')


def run():
    for needed in ('SMG', 'SNIPER', 'LMG', 'LAUNCHER'):
        if not hasattr(unreal.WeaponKind, needed):
            raise RuntimeError(f'unreal.WeaponKind.{needed} is missing: build the C++ with the four new kinds first; nothing was changed')
    if not hasattr(unreal.AmmoType, 'ROCKET') or not hasattr(unreal.WeaponReloadPart, 'HELIX'):
        raise RuntimeError('unreal.AmmoType.ROCKET or WeaponReloadPart.HELIX is missing: build the C++ first; nothing was changed')
    guns = {asset_name: make_gun(asset_name, spec) for asset_name, spec in GUNS.items()}
    add_to_loot(guns)
    log('done')


run()
