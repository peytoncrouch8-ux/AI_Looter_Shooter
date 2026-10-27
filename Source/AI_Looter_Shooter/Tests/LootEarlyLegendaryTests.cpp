#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Creatures/CreatureRankSettings.h"
#include "Loot/Chest.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootOdds.h"
#include "Loot/LootTable.h"
#include "Weapons/WeaponDefinition.h"
#include "UObject/Package.h"

// Legendaries rarer early (Looter.Loot.EarlyLegendary.*; the user, 2026-10-27: "Rarer, bosses still drop"). On Ransom's
// Rest (guns of level 8 and under: ULootLibrary::LegendaryCapFor) a gun from a regular creature's or a chest's table is
// legendary at most 1 time in 300, so a Regular kill drops one 0.1% of the time and a Strongbox 0.67%; Soulfed monsters,
// bosses and bounties keep their odds (ULootTable::bKeepsLegendaryOdds), and from the Lily on (level 9) every table has its
// full odds again. The rules read the cvars' defaults (Looter.Loot.EarlyLegendary.Share 1/300, .MaxLevel 8).

namespace
{
	/** Ransom's Rest's guns (its first level), its highest (a level-6 Hallowed's), and the Lily's first. */
	constexpr int32 EarlyLevel = 1;
	constexpr int32 EarlyTop = 8;
	constexpr int32 LateLevel = 9;
	constexpr double DesignCap = 1.0 / 300.0;
	constexpr int32 Top = LootOdds::NumRarities - 1;

	/** A table of one gun with the stock rarity weights (60/25/10/4/1): Guns on every kill, at Luck. */
	ULootTable* StockTable(int32 Guns, float Luck)
	{
		ULootTable* Table = NewObject<ULootTable>();
		FLootTableEntry Entry;
		Entry.Weapon = NewObject<UWeaponDefinition>();
		Table->Entries.Add(Entry);
		Table->WeaponDropChance = 1.f;
		Table->MinWeaponDrops = Guns;
		Table->MaxWeaponDrops = Guns;
		Table->Luck = Luck;
		Table->AmmoDropChance = 0.f;
		return Table;
	}

	/** A chest kind's table as AChest::MakeLootTable builds it (the default table's guns at the kind's numbers). */
	ULootTable* ChestTable(EChestKind Kind, const ULootTable& Default)
	{
		const FChestKindInfo Info = FChestKindInfo::Get(Kind);
		ULootTable* Table = NewObject<ULootTable>();
		Table->Entries = Default.Entries;
		Table->WeaponDropChance = FMath::Clamp(Info.GunChance, 0.f, 1.f);
		Table->MinWeaponDrops = Info.Guns;
		Table->MaxWeaponDrops = Info.Guns;
		Table->Luck = Info.Luck;
		return Table;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootEarlyLegendaryRulesTest, "Looter.Loot.EarlyLegendary.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootEarlyLegendaryRulesTest::RunTest(const FString& Parameters)
{
	// The cap's arithmetic: a legendary's share held to the cap, the rest of the odds as they were.
	const double Capped = UWeaponRollLibrary::CapLegendaryWeight(1.0, 99.0, static_cast<float>(DesignCap));
	TestNearlyEqual(TEXT("A 1% legendary capped to 1 in 300"), Capped / (Capped + 99.0), DesignCap, 1e-6);
	TestEqual(TEXT("Under the cap, untouched"), UWeaponRollLibrary::CapLegendaryWeight(0.1, 99.9, 0.01f), 0.1);
	TestEqual(TEXT("A cap of 1: none"), UWeaponRollLibrary::CapLegendaryWeight(20.0, 80.0, 1.f), 20.0);
	TestEqual(TEXT("A cap of 0: no legendary"), UWeaponRollLibrary::CapLegendaryWeight(1.0, 99.0, 0.f), 0.0);

	// Where it applies: early levels, tables that don't keep their odds.
	ULootTable* Table = StockTable(1, 0.f);
	TestNearlyEqual(TEXT("Ransom's Rest's first level: capped at 1 in 300"), ULootLibrary::LegendaryCapFor(Table, EarlyLevel), static_cast<float>(DesignCap),
		1e-6f);
	TestTrue(TEXT("...and its highest (a level-6 Hallowed's 8)"), ULootLibrary::LegendaryCapFor(Table, EarlyTop) < 1.f);
	TestEqual(TEXT("The Lily's first level: full odds"), ULootLibrary::LegendaryCapFor(Table, LateLevel), 1.f);
	TestEqual(TEXT("No level named: the table's own odds"), ULootLibrary::LegendaryCapFor(Table, ULootLibrary::NoLevel), 1.f);
	TestEqual(TEXT("No table: no cap"), ULootLibrary::LegendaryCapFor(nullptr, EarlyLevel), 1.f);

	// A bounty's copy keeps its odds; the table it came from doesn't change.
	ULootTable* Bounty = ULootLibrary::KeepingLegendaryOdds(Table, GetTransientPackage());
	TestTrue(TEXT("A bounty's copy keeps its odds"), Bounty && Bounty != Table && Bounty->bKeepsLegendaryOdds);
	TestFalse(TEXT("...the table it came from doesn't"), Table->bKeepsLegendaryOdds);
	TestEqual(TEXT("...never capped early"), ULootLibrary::LegendaryCapFor(Bounty, EarlyLevel), 1.f);
	TestTrue(TEXT("...the same guns and numbers"), Bounty && Bounty->Entries.Num() == Table->Entries.Num()
		&& Bounty->WeaponDropChance == Table->WeaponDropChance && Bounty->Luck == Table->Luck);
	TestTrue(TEXT("A table that keeps its odds is its own"), ULootLibrary::KeepingLegendaryOdds(Bounty, nullptr) == Bounty);

	// Uncapped, a seeded roll draws exactly as it did before the cap.
	const UWeaponDefinition* Gun = Table->Entries[0].Weapon;
	FRandomStream Before(20261027);
	FRandomStream After(20261027);
	int32 Differ = 0;
	for (int32 Draw = 0; Draw < 20000; ++Draw)
	{
		Differ += UWeaponRollLibrary::RollRarityWith(Gun, 0.5f, Before) != UWeaponRollLibrary::RollRarityWith(Gun, 0.5f, After, 1.f) ? 1 : 0;
	}
	TestEqual(TEXT("Uncapped rolls draw as before"), Differ, 0);

	// Capped, the rolls bear it out at any luck (a Restless's 0.5, a Strongbox's 1.0, an Ethereal's 1.3), Common still the likeliest.
	for (const float Luck : { 0.f, 1.f, 1.3f })
	{
		constexpr int32 Draws = 600000;
		FRandomStream Random(20261027 + FMath::RoundToInt32(Luck * 10.f));
		int32 Tiers[LootOdds::NumRarities] = {};
		for (int32 Draw = 0; Draw < Draws; ++Draw)
		{
			++Tiers[static_cast<int32>(UWeaponRollLibrary::RollRarityWith(Gun, Luck, Random, static_cast<float>(DesignCap)))];
		}
		const double Share = static_cast<double>(Tiers[Top]) / Draws;
		const double Spread = 5.0 * FMath::Sqrt(DesignCap * (1.0 - DesignCap) / Draws);
		TestTrue(FString::Printf(TEXT("Luck %.1f: a legendary %.3f%% of the time (cap %.3f%%)"), Luck, Share * 100.0, DesignCap * 100.0),
			FMath::Abs(Share - DesignCap) <= Spread);
		TestTrue(FString::Printf(TEXT("Luck %.1f: Common still the likeliest"), Luck), Tiers[0] > Tiers[1] && Tiers[0] > Tiers[Top]);
	}

	// A stock table's numbers: a Regular kill's (a gun on 30% of kills, Luck 0) and a Strongbox's (2 guns, Luck 1).
	ULootTable* Regular = StockTable(1, 0.f);
	Regular->WeaponDropChance = 0.3f;
	const double RegularEarly = LootOdds::Expected(Regular, 0.f, EarlyLevel).LegendaryPerKill;
	TestNearlyEqual(*FString::Printf(TEXT("A Regular kill early: %.3f%% (0.1%%)"), RegularEarly * 100.0), RegularEarly, 0.001, 0.00002);
	TestNearlyEqual(TEXT("...later: 0.3% as before"), LootOdds::Expected(Regular, 0.f, LateLevel).LegendaryPerKill, 0.003, 0.0001);
	const double BoxEarly = LootOdds::Expected(StockTable(2, 1.f), 0.f, EarlyLevel).LegendaryPerKill;
	TestTrue(FString::Printf(TEXT("A Strongbox early: %.2f%%, under 1%%"), BoxEarly * 100.0), BoxEarly < 0.01 && BoxEarly > 0.006);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootEarlyLegendaryOddsTest, "Looter.Loot.EarlyLegendary.Odds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootEarlyLegendaryOddsTest::RunTest(const FString& Parameters)
{
	// The game's own tables on Ransom's Rest: the tiers almost never, Soulfed and bosses as before (Docs/Story.md, "Enemy ranks
	// and legendary drops"). At most the design (a rank's guns may be rarer still), never none; from the Lily on, today's odds.
	struct FRankTarget
	{
		ECreatureRank Rank;
		double Early;
		bool bKeeps;
	};
	const FRankTarget Targets[] = {
		{ ECreatureRank::Basic, 0.001, false }, { ECreatureRank::Rare, 0.002, false }, { ECreatureRank::Epic, 0.003, false },
		{ ECreatureRank::Hallowed, 0.005, false }, { ECreatureRank::Ethereal, 0.00666, false },
		{ ECreatureRank::Legendary, 0.214, true }, { ECreatureRank::Boss, 0.303, true } };
	for (const FRankTarget& Target : Targets)
	{
		const FString Name = UCreatureRankSettings::GetTierName(Target.Rank);
		const ULootTable* Table = UCreatureRankSettings::GetLootTable(Target.Rank);
		if (!TestNotNull(FString::Printf(TEXT("%s has a loot table"), *Name), Table))
		{
			continue;
		}
		if (Table->Entries.IsEmpty())
		{
			AddWarning(FString::Printf(TEXT("%s's table lists no guns in this checkout: its odds aren't checked."), *Name));
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s: keeps its legendary odds early: %s (Tools/Unreal/create_rank_assets.py sets it)"), *Name,
			Target.bKeeps ? TEXT("yes") : TEXT("no")), Table->bKeepsLegendaryOdds, Target.bKeeps);
		const double Early = LootOdds::Expected(Table, 0.f, EarlyLevel).LegendaryPerKill;
		const double Late = LootOdds::Expected(Table, 0.f, LateLevel).LegendaryPerKill;
		const double Before = LootOdds::Expected(Table).LegendaryPerKill;
		if (Target.bKeeps)
		{
			TestTrue(FString::Printf(TEXT("%s: a legendary on %.1f%% of kills on Ransom's Rest (design %.1f%%)"), *Name, Early * 100.0,
				Target.Early * 100.0), FMath::Abs(Early - Target.Early) <= Target.Early * 0.1);
		}
		else
		{
			TestTrue(FString::Printf(TEXT("%s: a legendary on %.3f%% of kills on Ransom's Rest (at most %.3f%%, never none)"), *Name,
				Early * 100.0, Target.Early * 100.0), Early <= Target.Early * 1.1 && Early > 0.0);
		}
		TestNearlyEqual(*FString::Printf(TEXT("%s: from the Lily on, today's odds (%.2f%%)"), *Name, Before * 100.0), Late, Before, 1e-9);
		AddInfo(FString::Printf(TEXT("%s (%s): a legendary on %.3f%% of kills early (1 in %.0f), %.2f%% later"), *Name, *Table->GetName(),
			Early * 100.0, Early > 0.0 ? 1.0 / Early : 0.0, Late * 100.0));
	}

	// A million Regular kills on Ransom's Rest, from a fixed seed: the rolls bear out the table's 0.1%.
	if (const ULootTable* Regular = UCreatureRankSettings::GetLootTable(ECreatureRank::Basic); Regular && !Regular->Entries.IsEmpty())
	{
		constexpr int32 Kills = 1000000;
		const LootOdds::FTally Tally = LootOdds::Simulate(Regular, Kills, 20261027, 0.f, EarlyLevel);
		const double Expected = LootOdds::Expected(Regular, 0.f, EarlyLevel).LegendaryPerKill;
		const double Rolled = Tally.PerKill(Tally.KillsWithLegendary);
		const double Spread = 5.0 * FMath::Sqrt(Expected * (1.0 - Expected) / Kills);
		TestTrue(FString::Printf(TEXT("%d Regular kills: a legendary on %.3f%% (table %.3f%%)"), Kills, Rolled * 100.0, Expected * 100.0),
			FMath::Abs(Rolled - Expected) <= Spread);
	}

	// The chests on Ransom's Rest: a Strongbox under 1%, the rest less; full odds again later (a Strongbox 15.5%).
	const ULootTable* Default = ULootLibrary::GetDefaultLootTable();
	if (!Default || Default->Entries.IsEmpty())
	{
		AddWarning(TEXT("The default loot table lists no guns in this checkout: the chests' odds aren't checked."));
		return true;
	}
	for (const EChestKind Kind : FChestKindInfo::All())
	{
		const FString Name = AChest::KindName(Kind).ToString();
		const ULootTable* Table = ChestTable(Kind, *Default);
		const double Early = LootOdds::Expected(Table, 0.f, EarlyLevel).LegendaryPerKill;
		const double Late = LootOdds::Expected(Table, 0.f, LateLevel).LegendaryPerKill;
		TestTrue(FString::Printf(TEXT("%s: a legendary %.3f%% of the time on Ransom's Rest, under 1%%"), *Name, Early * 100.0), Early < 0.01);
		TestTrue(FString::Printf(TEXT("%s: more later (%.2f%%)"), *Name, Late * 100.0), Late >= Early);
		AddInfo(FString::Printf(TEXT("%s: %.3f%% early, %.2f%% later"), *Name, Early * 100.0, Late * 100.0));
	}
	return true;
}

#endif
