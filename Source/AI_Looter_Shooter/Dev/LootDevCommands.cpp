// Developer console commands for loot (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureRankSettings.h"
#include "Loot/LootOdds.h"
#include "Loot/LootTable.h"
#include "HAL/IConsoleManager.h"

namespace
{
	void SimulateDrops(const TArray<FString>& Args)
	{
		ECreatureRank Rank = ECreatureRank::Basic;
		if (Args.Num() > 0 && !UCreatureRankSettings::ParseRank(Args[0], Rank))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Loot.SimulateDrops: no rank called '%s' (Basic, Rare, Epic, Legendary, Boss)."), *Args[0]);
			return;
		}
		const int32 Kills = Args.Num() > 1 ? FMath::Clamp(FCString::Atoi(*Args[1]), 1, 10000000) : 100000;
		const ULootTable* Table = UCreatureRankSettings::GetLootTable(Rank);
		if (!Table)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Loot.SimulateDrops: %s has no loot table."), *UCreatureRankSettings::GetRankName(Rank));
			return;
		}

		// Rolled from a new seed each time, the way kills roll, next to what the table's numbers work out to exactly.
		const int32 Seed = FMath::Rand();
		const LootOdds::FTally Tally = LootOdds::Simulate(Table, Kills, Seed);
		const LootOdds::FExpected Expected = LootOdds::Expected(Table);
		double Shares[LootOdds::NumRarities];
		for (int32 Tier = 0; Tier < LootOdds::NumRarities; ++Tier)
		{
			Shares[Tier] = Tally.RarityShare(Tier);
		}
		const double LegendaryRate = Tally.PerKill(Tally.KillsWithLegendary);

		UE_LOG(LogLooter, Display, TEXT("Looter.Loot.SimulateDrops: %s, %s, %d kills (seed %d)"), *UCreatureRankSettings::GetRankName(Rank),
			*Table->GetName(), Kills, Seed);
		UE_LOG(LogLooter, Display, TEXT("  guns on %.1f%% of kills (table %.1f%%), %.2f a kill (table %.2f): %d to %d at luck %.2f"),
			Tally.PerKill(Tally.KillsWithWeapons) * 100.0, Expected.KillsWithWeapons * 100.0, Tally.PerKill(Tally.Weapons),
			Expected.WeaponsPerKill, Table->MinWeaponDrops, Table->MaxWeaponDrops, Table->Luck);
		UE_LOG(LogLooter, Display, TEXT("  %d guns: %s"), Tally.Weapons, *LootOdds::DescribeShares(Shares));
		UE_LOG(LogLooter, Display, TEXT("  table:    %s"), *LootOdds::DescribeShares(Expected.RarityShares));
		UE_LOG(LogLooter, Display, TEXT("  counts: Common %d, Uncommon %d, Rare %d, Epic %d, Legendary %d"), Tally.Rarities[0], Tally.Rarities[1],
			Tally.Rarities[2], Tally.Rarities[3], Tally.Rarities[4]);
		UE_LOG(LogLooter, Display, TEXT("  a legendary on %.2f%% of kills, 1 in %.1f (table %.2f%%)"), LegendaryRate * 100.0,
			LegendaryRate > 0.0 ? 1.0 / LegendaryRate : 0.0, Expected.LegendaryPerKill * 100.0);
		UE_LOG(LogLooter, Display, TEXT("  ammo: %.2f pickups a kill (table %.2f), %d to %d rounds each"), Tally.PerKill(Tally.AmmoPickups),
			Expected.AmmoPickupsPerKill, Tally.FewestRounds, Tally.MostRounds);
	}

	FAutoConsoleCommand SimulateDropsCommand(
		TEXT("Looter.Loot.SimulateDrops"),
		TEXT("Looter.Loot.SimulateDrops [Basic|Rare|Epic|Legendary|Boss] [kills, 100000 by default]: rolls the rank's loot table ")
		TEXT("for that many kills and prints how often guns drop, their rarities and legendaries per kill, and the ammo pickups, ")
		TEXT("beside what the table's numbers work out to."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SimulateDrops));
}

#endif
