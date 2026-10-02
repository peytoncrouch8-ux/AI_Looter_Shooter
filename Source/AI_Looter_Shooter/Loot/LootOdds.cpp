#include "Loot/LootOdds.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Weapons/WeaponDefinition.h"

namespace
{
	/** Each rarity's chance for one gun of this kind at this luck: its weights, tier n times (1 + luck)^n (RollRarityWith). */
	void RarityChances(const UWeaponDefinition& Definition, float Luck, double (&OutChances)[LootOdds::NumRarities])
	{
		double Total = 0.0;
		for (int32 Tier = 0; Tier < LootOdds::NumRarities; ++Tier)
		{
			const double TierScale = FMath::Pow(1.0 + FMath::Max(static_cast<double>(Luck), 0.0), static_cast<double>(Tier));
			OutChances[Tier] = Definition.GetRarityInfo(static_cast<EWeaponRarity>(Tier)).Weight * TierScale;
			Total += OutChances[Tier];
		}
		for (int32 Tier = 0; Tier < LootOdds::NumRarities; ++Tier)
		{
			// No weights at all rolls Common, as RollRarityWith does.
			OutChances[Tier] = Total > 0.0 ? OutChances[Tier] / Total : (Tier == 0 ? 1.0 : 0.0);
		}
	}
}

LootOdds::FExpected LootOdds::Expected(const ULootTable* LootTable, float ExtraLuck)
{
	FExpected Result;
	if (!LootTable)
	{
		return Result;
	}

	const double AmmoChance = FMath::Clamp(static_cast<double>(LootTable->AmmoDropChance), 0.0, 1.0);
	const int32 MinAmmo = FMath::Max(LootTable->MinAmmoDrops, 0);
	const int32 MaxAmmo = FMath::Max(LootTable->MaxAmmoDrops, MinAmmo);
	Result.AmmoPickupsPerKill = AmmoChance * (MinAmmo + MaxAmmo) * 0.5;

	// Each gun is a kind picked by weight, then a rarity from that kind's own odds shifted by the luck.
	double TotalWeight = 0.0;
	for (const FLootTableEntry& Entry : LootTable->Entries)
	{
		TotalWeight += Entry.Weapon ? FMath::Max(static_cast<double>(Entry.Weight), 0.0) : 0.0;
	}
	if (TotalWeight <= 0.0)
	{
		return Result;
	}
	for (const FLootTableEntry& Entry : LootTable->Entries)
	{
		if (!Entry.Weapon || Entry.Weight <= 0.f)
		{
			continue;
		}
		double Chances[NumRarities];
		RarityChances(*Entry.Weapon, LootTable->Luck + ExtraLuck, Chances);
		for (int32 Tier = 0; Tier < NumRarities; ++Tier)
		{
			Result.RarityShares[Tier] += Chances[Tier] * Entry.Weight / TotalWeight;
		}
	}

	// How many guns: none, or Min to Max of them, every count as likely; a kill has a legendary unless every gun misses it.
	const double WeaponChance = FMath::Clamp(static_cast<double>(LootTable->WeaponDropChance), 0.0, 1.0);
	const int32 MinWeapons = FMath::Max(LootTable->MinWeaponDrops, 0);
	const int32 MaxWeapons = FMath::Max(LootTable->MaxWeaponDrops, MinWeapons);
	const double Counts = MaxWeapons - MinWeapons + 1;
	const double NotLegendary = 1.0 - Result.RarityShares[NumRarities - 1];
	double AnyLegendary = 0.0;
	double AnyGuns = 0.0;
	for (int32 Count = MinWeapons; Count <= MaxWeapons; ++Count)
	{
		AnyLegendary += (1.0 - FMath::Pow(NotLegendary, static_cast<double>(Count))) / Counts;
		AnyGuns += Count > 0 ? 1.0 / Counts : 0.0;
	}
	Result.KillsWithWeapons = WeaponChance * AnyGuns;
	Result.WeaponsPerKill = WeaponChance * (MinWeapons + MaxWeapons) * 0.5;
	Result.LegendaryPerKill = WeaponChance * AnyLegendary;
	return Result;
}

LootOdds::FTally LootOdds::Simulate(const ULootTable* LootTable, int32 Kills, int32 Seed, float ExtraLuck)
{
	FTally Tally;
	FRandomStream Random(Seed);
	int32 Fewest = MAX_int32;
	for (int32 Kill = 0; Kill < Kills; ++Kill)
	{
		// The ammo first and then the guns, as a real kill draws them (ULootLibrary::RollLoot).
		const TArray<FAmmoDrop> Ammo = ULootLibrary::RollAmmo(LootTable, Random);
		const TArray<FLootWeaponPick> Picks = ULootLibrary::RollWeaponPicks(LootTable, ExtraLuck, Random);
		++Tally.Kills;
		for (const FAmmoDrop& Drop : Ammo)
		{
			++Tally.AmmoPickups;
			Fewest = FMath::Min(Fewest, Drop.Amount);
			Tally.MostRounds = FMath::Max(Tally.MostRounds, Drop.Amount);
		}
		bool bLegendary = false;
		for (const FLootWeaponPick& Pick : Picks)
		{
			++Tally.Weapons;
			++Tally.Rarities[static_cast<int32>(Pick.Rarity)];
			bLegendary |= Pick.Rarity == EWeaponRarity::Legendary;
		}
		Tally.KillsWithWeapons += Picks.IsEmpty() ? 0 : 1;
		Tally.KillsWithLegendary += bLegendary ? 1 : 0;
	}
	Tally.FewestRounds = Tally.AmmoPickups > 0 ? Fewest : 0;
	return Tally;
}

FString LootOdds::DescribeShares(const double (&Shares)[NumRarities])
{
	TArray<FString> Parts;
	for (int32 Tier = 0; Tier < NumRarities; ++Tier)
	{
		Parts.Add(FString::Printf(TEXT("%s %.1f%%"), *UEnum::GetDisplayValueAsText(static_cast<EWeaponRarity>(Tier)).ToString(), Shares[Tier] * 100.0));
	}
	return FString::Join(Parts, TEXT(", "));
}
