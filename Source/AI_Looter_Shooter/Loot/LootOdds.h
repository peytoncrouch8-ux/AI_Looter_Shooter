#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

class ULootTable;

/**
 * A loot table's odds: worked out exactly from its numbers and its guns' rarity weights, and counted over many kills
 * rolled from a seed the way real kills roll (Looter.Loot.SimulateDrops prints both; Looter.Loot.RankOdds tests them).
 * Ammo doesn't lean toward a kill gun here: that changes which class drops, not how many.
 */
namespace LootOdds
{
	inline constexpr int32 NumRarities = static_cast<int32>(EWeaponRarity::Legendary) + 1;

	/** What a table gives a kill on average. */
	struct FExpected
	{
		/** Share of kills that drop any guns. */
		double KillsWithWeapons = 0.0;
		double WeaponsPerKill = 0.0;
		/** Each rarity's share of the guns, Common first. */
		double RarityShares[NumRarities] = {};
		/** Share of kills that drop at least one legendary gun. */
		double LegendaryPerKill = 0.0;
		double AmmoPickupsPerKill = 0.0;
	};

	/** What many kills dropped, counted. */
	struct FTally
	{
		int32 Kills = 0;
		int32 KillsWithWeapons = 0;
		int32 KillsWithLegendary = 0;
		int32 Weapons = 0;
		/** Guns of each rarity, Common first. */
		int32 Rarities[NumRarities] = {};
		int32 AmmoPickups = 0;
		/** The fewest and most rounds one ammo pickup held (0 when none dropped). */
		int32 FewestRounds = 0;
		int32 MostRounds = 0;

		/** Count as a share of the kills. */
		double PerKill(int32 Count) const { return Kills > 0 ? static_cast<double>(Count) / Kills : 0.0; }

		/** A rarity's share of the guns. */
		double RarityShare(int32 Tier) const { return Weapons > 0 ? static_cast<double>(Rarities[Tier]) / Weapons : 0.0; }
	};

	FExpected Expected(const ULootTable* LootTable, float ExtraLuck = 0.f);

	/** Rolls Kills kills of the table from Seed, the same draws as ULootLibrary::RollLoot, without rolling the guns' stats. */
	FTally Simulate(const ULootTable* LootTable, int32 Kills, int32 Seed, float ExtraLuck = 0.f);

	/** "Common 43.2%, Uncommon 27.1%, ..." for logs. */
	FString DescribeShares(const double (&Shares)[NumRarities]);
}
