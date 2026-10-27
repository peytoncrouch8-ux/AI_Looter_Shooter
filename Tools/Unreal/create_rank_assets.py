"""Makes (or brings up to date) the creature ranks' loot tables beside the default one, /Game/Weapons/Data/DA_LootTable_Rare
(Restless), _Epic (Gravebound), _Hallowed, _Ethereal, _Legendary (Soulfed) and _Boss, and saves them. Run it in the open
editor:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_rank_assets.py"

Each table is DA_LootTable_Default's guns (and their weights) and ammo classes with its rank's odds (Docs/Story.md,
"Enemy ranks and legendary drops"; the five tiers the user approved on 2026-10-20): the same numbers as
UCreatureRankSettings::DesignLootOdds, which the game uses as a stand-in until these assets exist. Ammo pickups still hold
a kill's 18-36 rounds each; ranks drop more of them. Running it again puts the tables' guns, ammo and odds back to these.
Basic (Regular) keeps DA_LootTable_Default, which this never changes.

With the stock rarity weights (60/25/10/4/1), a kill drops a legendary 0.3% of the time from Regular, 2.2% Restless,
5.5% Gravebound, 13.3% Hallowed, 21.3% Ethereal, 21.4% Soulfed and 30.3% Boss. In the early areas (guns of level 8 and
under: Ransom's Rest) the tiers' tables are capped at 1 legendary in 300 guns (ULootLibrary::LegendaryCapFor; the user,
2026-10-27: "Rarer, bosses still drop"): Regular 0.1%, Restless 0.2%, Gravebound 0.3%, Hallowed 0.5%, Ethereal 0.67%.
Soulfed and Boss keep their odds everywhere (keeps_legendary_odds). In the game, Looter.Loot.SimulateDrops <rank> [kills]
[level] and Looter.Tier.Odds print them; Looter.Loot.RankOdds and Looter.Loot.EarlyLegendary.* test them.
"""
import unreal

DATA = '/Game/Weapons/Data'
DEFAULT = f'{DATA}/DA_LootTable_Default'

# Rank: (chance a kill drops guns, fewest guns, most guns, luck, fewest ammo pickups, most ammo pickups, legendaries keep
# their odds in the early areas). The same numbers as UCreatureRankSettings::DesignLootOdds.
RANKS = {
    'Rare': (0.6, 1, 1, 0.5, 2, 3, False),
    'Epic': (0.9, 1, 1, 0.8, 2, 4, False),
    'Hallowed': (1.0, 1, 2, 1.1, 3, 5, False),
    'Ethereal': (1.0, 2, 2, 1.3, 4, 6, False),
    'Legendary': (1.0, 2, 2, 1.3, 4, 6, True),
    'Boss': (1.0, 3, 3, 1.3, 8, 10, True),
}

# What every rank table takes from the default one: the gun pool and the ammo rules.
SHARED = ['entries', 'ammo_types', 'ammo_drop_chance', 'kill_weapon_ammo_bias', 'ammo_amount_min', 'ammo_amount_max']


def rank_table(rank):
    path = f'{DATA}/DA_LootTable_{rank}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
    else:
        asset = unreal.EditorAssetLibrary.duplicate_asset(DEFAULT, path)
    if asset is None:
        raise RuntimeError(f'could not load or make {path}')
    return asset, path


def make_tables():
    default = unreal.load_asset(DEFAULT)
    if default is None:
        raise RuntimeError(f'{DEFAULT} is missing')
    for rank, (chance, fewest_guns, most_guns, luck, fewest_ammo, most_ammo, keeps_odds) in RANKS.items():
        asset, path = rank_table(rank)
        for name in SHARED:
            asset.set_editor_property(name, default.get_editor_property(name))
        asset.set_editor_property('weapon_drop_chance', chance)
        asset.set_editor_property('min_weapon_drops', fewest_guns)
        asset.set_editor_property('max_weapon_drops', most_guns)
        asset.set_editor_property('luck', luck)
        asset.set_editor_property('min_ammo_drops', fewest_ammo)
        asset.set_editor_property('max_ammo_drops', most_ammo)
        # The bool is bKeepsLegendaryOdds in C++; Python names it without the b.
        asset.set_editor_property('keeps_legendary_odds', keeps_odds)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        guns = len(asset.get_editor_property('entries'))
        unreal.log(f'RANKTABLES {path}: guns on {chance:.0%} of kills, {fewest_guns}-{most_guns} of {guns} kinds at luck {luck}, '
                   f'{fewest_ammo}-{most_ammo} ammo pickups, legendary odds {"kept everywhere" if keeps_odds else "capped early"}')
    unreal.log('RANKTABLES done')


make_tables()
