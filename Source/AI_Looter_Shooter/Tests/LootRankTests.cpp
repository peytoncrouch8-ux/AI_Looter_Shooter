#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Loot/LootDropComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootOdds.h"
#include "Loot/LootTable.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SpiderCreature.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const TCHAR* TierName(int32 Tier)
	{
		static const TCHAR* Names[] = { TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"), TEXT("Epic"), TEXT("Legendary") };
		static_assert(UE_ARRAY_COUNT(Names) == LootOdds::NumRarities, "One name per rarity tier");
		return Names[Tier];
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootRankOddsTest, "Looter.Loot.RankOdds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootRankOddsTest::RunTest(const FString& Parameters)
{
	// The design's odds rank by rank (Docs/Story.md, "Enemy ranks and legendary drops"), from each rank's own table (its
	// asset, or until Tools/Unreal/create_rank_assets.py has made it, the stand-in with the same odds): a legendary on
	// 0.3% / 2.2% / 11.8% / 21.4% / 30.3% of kills; guns on 30% of kills (Basic, the default table), 60% (Rare) and every
	// kill from Epic up; more ammo pickups up the ranks, each still a kill's 18 to 36 rounds; Common the likeliest rarity.
	struct FRankTarget
	{
		ECreatureRank Rank;
		double Legendary;
		double KillsWithGuns;
	};
	const FRankTarget Targets[] = {
		{ ECreatureRank::Basic, 0.003, 0.3 }, { ECreatureRank::Rare, 0.022, 0.6 }, { ECreatureRank::Epic, 0.118, 1.0 },
		{ ECreatureRank::Legendary, 0.214, 1.0 }, { ECreatureRank::Boss, 0.303, 1.0 } };
	TestTrue(TEXT("Basic keeps the default table"), UCreatureRankSettings::GetLootTable(ECreatureRank::Basic) == ULootLibrary::GetDefaultLootTable());

	double LastLegendary = -1.0;
	double LastGuns = -1.0;
	double LastAmmo = -1.0;
	for (const FRankTarget& Target : Targets)
	{
		const FString Name = UCreatureRankSettings::GetRankName(Target.Rank);
		const ULootTable* Table = UCreatureRankSettings::GetLootTable(Target.Rank);
		if (!TestNotNull(FString::Printf(TEXT("%s has a loot table"), *Name), Table))
		{
			continue;
		}

		// Enough kills that even Basic's 1 in 333 is counted to a few percent, from a fixed seed so the result never flickers.
		const int32 Kills = FMath::Clamp(FMath::RoundToInt32(3000.0 / Target.Legendary), 200000, 1000000);
		const LootOdds::FTally Tally = LootOdds::Simulate(Table, Kills, 20261001 + static_cast<int32>(Target.Rank));
		const LootOdds::FExpected Expected = LootOdds::Expected(Table);
		const double Legendary = Tally.PerKill(Tally.KillsWithLegendary);
		const double KillsWithGuns = Tally.PerKill(Tally.KillsWithWeapons);
		const double Guns = Tally.PerKill(Tally.Weapons);
		const double Ammo = Tally.PerKill(Tally.AmmoPickups);
		TestTrue(FString::Printf(TEXT("%s drops guns (%d)"), *Name, Tally.Weapons), Tally.Weapons > 0);

		// The table's numbers give the design's odds, and the rolls bear the numbers out.
		TestTrue(FString::Printf(TEXT("%s: the table gives a legendary on %.2f%% of kills (design %.1f%%, within a tenth of it)"), *Name,
			Expected.LegendaryPerKill * 100.0, Target.Legendary * 100.0), FMath::Abs(Expected.LegendaryPerKill - Target.Legendary) <= Target.Legendary * 0.1);
		TestTrue(FString::Printf(TEXT("%s: %d kills dropped a legendary on %.2f%% (design %.1f%%, within a tenth of it)"), *Name, Kills,
			Legendary * 100.0, Target.Legendary * 100.0), FMath::Abs(Legendary - Target.Legendary) <= Target.Legendary * 0.1);
		const double Spread = 5.0 * FMath::Sqrt(Expected.LegendaryPerKill * (1.0 - Expected.LegendaryPerKill) / Kills);
		TestTrue(FString::Printf(TEXT("%s: the rolls match the table's numbers (%.3f%% against %.3f%%)"), *Name, Legendary * 100.0,
			Expected.LegendaryPerKill * 100.0), FMath::Abs(Legendary - Expected.LegendaryPerKill) <= Spread);
		TestTrue(FString::Printf(TEXT("%s: guns on %.1f%% of kills (decided: %.0f%%)"), *Name, KillsWithGuns * 100.0, Target.KillsWithGuns * 100.0),
			FMath::Abs(KillsWithGuns - Target.KillsWithGuns) <= 0.01);

		// Common the likeliest at every rank, each rarity up rarer than the one below it.
		for (int32 Tier = 0; Tier + 1 < LootOdds::NumRarities; ++Tier)
		{
			TestTrue(FString::Printf(TEXT("%s: %s (%d) drops more often than %s (%d)"), *Name, TierName(Tier), Tally.Rarities[Tier],
				TierName(Tier + 1), Tally.Rarities[Tier + 1]), Tally.Rarities[Tier] > Tally.Rarities[Tier + 1]);
		}

		// Up the ranks: legendaries more often, more guns, more ammo pickups, each still holding a kill's 18 to 36 rounds.
		TestTrue(FString::Printf(TEXT("%s: legendaries more often than the rank below"), *Name), Legendary > LastLegendary);
		TestTrue(FString::Printf(TEXT("%s: more guns a kill than the rank below (%.2f)"), *Name, Guns), Guns > LastGuns);
		TestTrue(FString::Printf(TEXT("%s: more ammo pickups a kill than the rank below (%.2f)"), *Name, Ammo), Ammo > LastAmmo);
		TestTrue(FString::Printf(TEXT("%s: each ammo pickup holds 18 to 36 rounds (%d to %d)"), *Name, Tally.FewestRounds, Tally.MostRounds),
			Tally.FewestRounds >= LooterLoot::KillAmmoAmountMin && Tally.MostRounds <= LooterLoot::KillAmmoAmountMax);
		LastLegendary = Legendary;
		LastGuns = Guns;
		LastAmmo = Ammo;

		double Shares[LootOdds::NumRarities];
		for (int32 Tier = 0; Tier < LootOdds::NumRarities; ++Tier)
		{
			Shares[Tier] = Tally.RarityShare(Tier);
		}
		AddInfo(FString::Printf(TEXT("%s (%s%s): %d kills, %.2f guns a kill, a legendary on %.2f%% (1 in %.0f); %s; %.2f ammo pickups a kill"),
			*Name, *Table->GetName(), Table->HasAnyFlags(RF_Transient) ? TEXT(", a stand-in until its asset is made") : TEXT(""), Kills, Guns,
			Legendary * 100.0, Legendary > 0.0 ? 1.0 / Legendary : 0.0, *LootOdds::DescribeShares(Shares), Ammo));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootLevelTest, "Looter.Loot.Level",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootLevelTest::RunTest(const FString& Parameters)
{
	// The guns a kill drops are the level of what was killed, stats and all (before, every creature dropped level 1 guns).
	ULootTable* Table = NewObject<ULootTable>();
	UWeaponDefinition* Weapon = NewObject<UWeaponDefinition>();
	FLootTableEntry Entry;
	Entry.Weapon = Weapon;
	Table->Entries.Add(Entry);
	Table->WeaponDropChance = 1.f;
	Table->MinWeaponDrops = 3;
	Table->MaxWeaponDrops = 3;
	FRandomStream Random(20261004);
	for (const int32 KillLevel : { 1, 6, 30 })
	{
		const FLootRoll Roll = ULootLibrary::RollLoot(Table, KillLevel, 0.f, Random);
		TestEqual(FString::Printf(TEXT("Level %d kill: three guns"), KillLevel), Roll.Weapons.Num(), 3);
		for (const FWeaponInstanceData& Gun : Roll.Weapons)
		{
			TestEqual(FString::Printf(TEXT("Level %d kill: the gun's level"), KillLevel), Gun.Level, KillLevel);
			// Its damage is its level's, not just its label: the same gun at level 1 hits that much softer.
			const FWeaponStats AtOne = UWeaponRollLibrary::ComputeStatsWithParts(Gun.Definition, Gun.Rarity, 1, Gun.Seed, Gun.Parts);
			const float Growth = 1.f + Weapon->DamagePerLevel * static_cast<float>(KillLevel - 1);
			TestNearlyEqual(FString::Printf(TEXT("Level %d kill: the gun's damage"), KillLevel), Gun.Stats.Damage, AtOne.Damage * Growth,
				AtOne.Damage * 0.001f);
		}
	}

	// A creature hands its level to its loot whenever it's set: as placed, set in play, and with its rank's levels on top.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ASpiderCreature* Spider = WorldWrapper.GetTestWorld()->SpawnActor<ASpiderCreature>(FVector::ZeroVector, FRotator::ZeroRotator);
	const ULootDropComponent* Loot = Spider ? Spider->FindComponentByClass<ULootDropComponent>() : nullptr;
	if (!TestNotNull(TEXT("Spider spawned"), Spider) || !TestNotNull(TEXT("The spider drops loot"), Loot))
	{
		return false;
	}
	// Placed at level 4. The test world never begins play itself, so the spider is started by hand.
	Spider->Level = 4;
	Spider->DispatchBeginPlay();
	const int32 BasicOffset = UCreatureRankSettings::Get(ECreatureRank::Basic).LevelOffset;
	TestEqual(TEXT("Placed at level 4, it drops level 4 guns"), Loot->Level, 4 + BasicOffset);
	Spider->SetLevel(9);
	TestEqual(TEXT("Set to level 9, it drops level 9 guns"), Loot->Level, 9);
	Spider->SetRank(ECreatureRank::Rare);
	const int32 RareLevel = 9 + UCreatureRankSettings::Get(ECreatureRank::Rare).LevelOffset - BasicOffset;
	TestEqual(TEXT("Restless, it's its rank's levels higher"), Spider->Level, RareLevel);
	TestEqual(TEXT("Restless, so are the guns it drops"), Loot->Level, RareLevel);
	TestTrue(TEXT("Restless, it drops from the Rare table"), Loot->LootTable.Get() == UCreatureRankSettings::GetLootTable(ECreatureRank::Rare));
	Spider->SetRank(ECreatureRank::Basic);
	TestEqual(TEXT("Back to Basic, back to level 9"), Loot->Level, 9);
	TestTrue(TEXT("Back to Basic, back to the default table"), Loot->LootTable.Get() == ULootLibrary::GetDefaultLootTable());
	return true;
}

#endif
