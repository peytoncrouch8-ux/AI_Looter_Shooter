#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureRank.h"
#include "Economy/EconomyRules.h"
#include "Loot/Chest.h"
#include "Missions/MissionRewards.h"
#include "Weapons/WeaponDefinition.h"

// What two playthroughs of Ransom's Rest earn and can spend (Docs/Progression.md, "What a playthrough of Ransom's Rest earns"):
// the story alone (the main missions, 75 minutes) and an explorer (every side mission, the mountain, the optional camps, about
// 3 hours 15). All expected values from the game's own functions (LooterEconomy), no rolls. The table below is per stretch of
// the playthrough: the player's level there (silver and prices grow with it), the kills (counts from PacingPaths.h, the
// documented fight budget, and for the explorer the layout's optional camps and patrols on top: estimated), the containers and
// crates opened, the guns the chests give (estimated) and the mission purses. Guns the kills give are worked out from the rules.

namespace
{
	/** A kill's silver is rolled by rank and by whether it pays as the Unpaid (pockets) or as a beast. */
	struct FKills
	{
		int32 BasicUnpaid = 0;
		int32 RestlessUnpaid = 0;
		int32 GraveboundUnpaid = 0;
		/** The Foreman: a Soulfed Unpaid. */
		int32 SoulfedUnpaid = 0;
		int32 BossUnpaid = 0;
		int32 BasicBeast = 0;
		int32 RestlessBeast = 0;
		/** The Gravemother: a Soulfed beast. */
		int32 SoulfedBeast = 0;

		int32 Total() const
		{
			return BasicUnpaid + RestlessUnpaid + GraveboundUnpaid + SoulfedUnpaid + BossUnpaid + BasicBeast + RestlessBeast + SoulfedBeast;
		}
	};

	struct FOpened
	{
		EChestKind Kind;
		int32 Count;
	};

	/** One stretch of a playthrough. */
	struct FStretch
	{
		const TCHAR* Name;
		/** Minutes of play (the gun of the day arrives every LooterEconomy::OfferDaySeconds of it). */
		int32 Minutes;
		/** The player's level through it, whole (silver, drops and prices all grow with it). */
		int32 Level;
		FKills Kills;
		TArray<FOpened> Containers;
		/** Crates and barrels broken. */
		int32 Crates;
		/** Guns the chests and the set pieces' showers give (kill guns come from the kills below). */
		int32 ChestGuns;
		/** The mission purses turned in here. */
		int32 Purse;
	};

	/** A kill drops a gun 30% of the time (ULootTable::WeaponDropChance), a fifth of those a ward (Looter.Shield.Drop.Share). */
	constexpr double KillGunChance = 0.3 * (1.0 - 0.2);

	/**
	 * The rarity mix of the guns found, Common first (the planning analysis of 2026-10-30, from the loot tables at these levels):
	 * kills give mostly Common and Uncommon; chests (tiers 3 to 5 floor at Uncommon, Rare and Epic) the better ones. Together
	 * they come to the analysis' explorer: Common 49, Uncommon 34, Rare 27, Epic 18, Legendary about 2.
	 */
	constexpr double KillGunMix[5] = { 0.66, 0.24, 0.08, 0.02, 0.0 };
	constexpr double ChestGunMix[5] = { 0.10, 0.26, 0.34, 0.28, 0.02 };

	/** The share of the guns the player sells (the analysis': 75% of the kill guns, 90% of the chest guns; the rest are kept or left). */
	constexpr double KillGunSold = 0.75;
	constexpr double ChestGunSold = 0.90;

	/** Ammo and salt bought as they were in the old balance test: a box per 8 kills, a jar per 14 (ten boxes and six jars for 85). */
	constexpr double KillsPerBox = 8.0;
	constexpr double KillsPerJar = 14.0;

	/** What a playthrough comes to. */
	struct FPlaythrough
	{
		int32 Kills = 0;
		double KillGuns = 0.0;
		double ChestGuns = 0.0;
		double GunsByRarity[5] = {};
		double KillSilver = 0.0;
		double ContainerSilver = 0.0;
		int32 Purses = 0;
		double GunSales = 0.0;
		double Ammo = 0.0;
		int32 Offers = 0;
		/** The gun of the day's expected price, averaged over the offers. */
		double OfferPrice = 0.0;

		double Earned() const { return KillSilver + ContainerSilver + Purses + GunSales; }
		double Guns() const { return KillGuns + ChestGuns; }
		/** Guns of the day the silver left after ammo buys, at their average price. */
		double Affordable() const { return OfferPrice > 0.0 ? (Earned() - Ammo) / OfferPrice : 0.0; }
	};

	double KillSilverOf(const FKills& Kills, int32 Level)
	{
		auto Of = [Level](ECreatureRank Rank, bool bUnpaid, int32 Count)
		{
			return Count * static_cast<double>(LooterEconomy::Expected(LooterEconomy::KillOdds(Rank, bUnpaid), Level));
		};
		return Of(ECreatureRank::Basic, true, Kills.BasicUnpaid) + Of(ECreatureRank::Rare, true, Kills.RestlessUnpaid)
			+ Of(ECreatureRank::Epic, true, Kills.GraveboundUnpaid) + Of(ECreatureRank::Legendary, true, Kills.SoulfedUnpaid)
			+ Of(ECreatureRank::Boss, true, Kills.BossUnpaid) + Of(ECreatureRank::Basic, false, Kills.BasicBeast)
			+ Of(ECreatureRank::Rare, false, Kills.RestlessBeast) + Of(ECreatureRank::Legendary, false, Kills.SoulfedBeast);
	}

	/** The gun of the day's expected price at Level: each rarity's price by its odds. */
	double ExpectedOfferPrice(int32 Level, UWeaponDefinition* Definition)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = Definition;
		Gun.Level = Level;
		double Price = 0.0;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			Gun.Rarity = static_cast<EWeaponRarity>(Index);
			Price += LooterEconomy::OfferRarityChance(Gun.Rarity) * LooterEconomy::GunBuyPrice(Gun);
		}
		return Price;
	}

	/**
	 * Plays Stretches on paper. FinalLevel is the level the player ends on: every level-up on the way brings a gun of the day
	 * too, priced at the level reached, and the play's clock a gun of the day every OfferDaySeconds, priced at the stretch it's in.
	 */
	FPlaythrough Play(const TArray<FStretch>& Stretches, int32 FinalLevel, UWeaponDefinition* Definition)
	{
		FPlaythrough Result;
		FWeaponInstanceData Gun;
		Gun.Definition = Definition;
		double Seconds = 0.0;
		for (const FStretch& Stretch : Stretches)
		{
			const int32 Kills = Stretch.Kills.Total();
			Result.Kills += Kills;
			Result.KillSilver += KillSilverOf(Stretch.Kills, Stretch.Level);
			for (const FOpened& Opened : Stretch.Containers)
			{
				Result.ContainerSilver += Opened.Count * LooterEconomy::Expected(LooterEconomy::ContainerOdds(Opened.Kind), Stretch.Level);
			}
			Result.ContainerSilver += Stretch.Crates * LooterEconomy::Expected(LooterEconomy::BreakableOdds(), Stretch.Level);
			Result.Purses += Stretch.Purse;

			const double KillGuns = Kills * KillGunChance;
			Result.KillGuns += KillGuns;
			Result.ChestGuns += Stretch.ChestGuns;
			Gun.Level = Stretch.Level;
			for (int32 Index = 0; Index < 5; ++Index)
			{
				Gun.Rarity = static_cast<EWeaponRarity>(Index);
				const double Kill = KillGuns * KillGunMix[Index];
				const double Chest = Stretch.ChestGuns * ChestGunMix[Index];
				Result.GunsByRarity[Index] += Kill + Chest;
				Result.GunSales += (Kill * KillGunSold + Chest * ChestGunSold) * LooterEconomy::GunSellPrice(Gun);
			}
			Result.Ammo += Kills / KillsPerBox * LooterEconomy::AmmoBoxPrice(EAmmoType::AssaultRifle, Stretch.Level)
				+ Kills / KillsPerJar * LooterEconomy::GrenadePrice(Stretch.Level);

			const int32 Before = FMath::FloorToInt32(Seconds / LooterEconomy::OfferDaySeconds);
			Seconds += Stretch.Minutes * 60.0;
			const int32 Arrived = FMath::FloorToInt32(Seconds / LooterEconomy::OfferDaySeconds) - Before;
			Result.Offers += Arrived;
			Result.OfferPrice += Arrived * ExpectedOfferPrice(Stretch.Level, Definition);
		}
		for (int32 Level = 2; Level <= FinalLevel; ++Level)
		{
			++Result.Offers;
			Result.OfferPrice += ExpectedOfferPrice(Level, Definition);
		}
		Result.OfferPrice = Result.Offers > 0 ? Result.OfferPrice / Result.Offers : 0.0;
		return Result;
	}

	/** The purse of a mission the old way, by id (MissionRewards::SilverFor). */
	int32 Purse(const TCHAR* Mission)
	{
		return MissionRewards::SilverFor(FName(Mission));
	}

	/**
	 * The story alone: Main 1-7 with Dead Lift and Back Pay, the roaming dead once (PacingPaths::StoryPath: the kills of each
	 * stop). The kept payroll isn't counted (a choice at the end of Back Pay: the eleven paid get an Epic gun instead). Containers
	 * and chest guns per stretch are estimates, tuned to the analysis' 55 guns; the story ends the level 6.9 (Looter.Progression.Pacing).
	 */
	TArray<FStretch> StoryStretches()
	{
		return {
			// Main 1-2: the bluff nest's four spiders and a Restless one.
			{ TEXT("Main 1-2"), 8, 2, { .BasicBeast = 4, .RestlessBeast = 1 }, { { EChestKind::Mailbox, 1 } }, 2, 2, Purse(TEXT("Main1")) },
			// Main 3: the gate and Main Street (12 Unpaid and a Bell-Ringer, two Restless).
			{ TEXT("Main 3"), 8, 2, { .BasicUnpaid = 13, .RestlessUnpaid = 2 }, { { EChestKind::Coffin, 1 }, { EChestKind::SupplyCrate, 1 } }, 2, 3, Purse(TEXT("Main3")) },
			// Main 4: the chapel yard's 15 and the Bell-Ringer, a Restless leader and the Sexton's Debt (Gravebound).
			{ TEXT("Main 4"), 10, 3, { .BasicUnpaid = 16, .RestlessUnpaid = 1, .GraveboundUnpaid = 1 }, { { EChestKind::Grave, 1 }, { EChestKind::Footlocker, 1 } }, 2, 4, Purse(TEXT("Main4")) },
			// The roaming dead once: boot hill's and the north road's five each and a Restless leader.
			{ TEXT("Roaming"), 5, 4, { .BasicUnpaid = 10, .RestlessUnpaid = 2 }, { { EChestKind::Mailbox, 1 } }, 1, 2, 0 },
			// Main 5: the Sink's floor and the egg sacs, twelve spiders.
			{ TEXT("Main 5"), 8, 4, { .BasicBeast = 12 }, { { EChestKind::SupplyCrate, 1 } }, 1, 3, Purse(TEXT("Main5")) },
			// Dead Lift: the quarry crew and Abe Haskett, then the raising-steam fight (spiders and Spitters).
			{ TEXT("Dead Lift"), 8, 5, { .BasicUnpaid = 7, .RestlessUnpaid = 1, .BasicBeast = 6 }, { { EChestKind::Footlocker, 1 } }, 1, 3, 40 },
			// Back Pay: the mine's miners, the Foreman (Soulfed) and the cave-in; no payroll counted.
			{ TEXT("Back Pay"), 17, 6, { .BasicUnpaid = 36, .RestlessUnpaid = 2, .GraveboundUnpaid = 1, .SoulfedUnpaid = 1 },
				{ { EChestKind::SupplyCrate, 1 }, { EChestKind::Strongbox, 1 } }, 2, 5, 0 },
			// Main 6-7: Abel's adds and Abel.
			{ TEXT("Main 6-7"), 11, 7, { .BasicUnpaid = 12, .BossUnpaid = 1 }, { { EChestKind::Grave, 1 } }, 1, 2, Purse(TEXT("Main7")) },
		};
	}

	/**
	 * Everything on Ransom's Rest once (PacingPaths::EverythingPath): the story's kills, the valley's side missions, the
	 * Gravemother, the mountain, the Last Watch; the layout's optional camps and patrols on top are estimates, as are the
	 * containers and chest guns (tuned to the analysis' 125 guns, 55 of them from chests). Own purses: Calder's 30, the Old Fare 30,
	 * Knocking Below 40, the Gravemother's 80, Dead Lift 40, the Last Watch 60 (Looter.Economy.Missions checks the assets).
	 */
	TArray<FStretch> ExplorerStretches()
	{
		return {
			{ TEXT("Main 1-2"), 10, 2, { .BasicBeast = 4, .RestlessBeast = 1 }, { { EChestKind::Mailbox, 1 } }, 3, 3, Purse(TEXT("Main1")) },
			// Main 3, the posters, Calder's caches, the Old Fare and its depot fights, Lost and Found; the valley's optional camps.
			{ TEXT("The valley"), 40, 3, { .BasicUnpaid = 28, .RestlessUnpaid = 5, .BasicBeast = 8 },
				{ { EChestKind::Coffin, 2 }, { EChestKind::Grave, 1 }, { EChestKind::Footlocker, 1 }, { EChestKind::SupplyCrate, 1 }, { EChestKind::Strongbox, 1 } }, 4, 9,
				Purse(TEXT("Main3")) + Purse(TEXT("Side1")) + 30 + 30 },
			// Main 4, the Gravepickers, the Stone Sentinels, the keeper's road's hounds.
			{ TEXT("The chapel"), 25, 4, { .BasicUnpaid = 30, .RestlessUnpaid = 7, .BasicBeast = 8, .RestlessBeast = 1 },
				{ { EChestKind::Coffin, 1 }, { EChestKind::Grave, 1 }, { EChestKind::Footlocker, 1 } }, 3, 8, Purse(TEXT("Main4")) },
			// Amos's hands, Tilly's cellar and its rats, the roaming dead, an optional Gravebound leader.
			{ TEXT("Whitlock and the cellar"), 25, 4, { .BasicUnpaid = 32, .RestlessUnpaid = 6, .GraveboundUnpaid = 1, .BasicBeast = 8 },
				{ { EChestKind::Mailbox, 1 }, { EChestKind::Grave, 1 }, { EChestKind::SupplyCrate, 1 } }, 3, 8, Purse(TEXT("Side2")) + 40 },
			// Main 5 and the Gravemother: the Sink's spiders, her brood and the Soulfed Gravemother herself.
			{ TEXT("The Sink"), 20, 5, { .BasicBeast = 22, .SoulfedBeast = 1 }, { { EChestKind::SupplyCrate, 1 }, { EChestKind::Strongbox, 1 } }, 2, 8,
				Purse(TEXT("Main5")) + 80 },
			// Dead Lift with the crest's spiders and the tramway's Knockers.
			{ TEXT("Dead Lift"), 20, 5, { .BasicUnpaid = 10, .RestlessUnpaid = 1, .BasicBeast = 16, .RestlessBeast = 1 },
				{ { EChestKind::SupplyCrate, 1 }, { EChestKind::Footlocker, 1 } }, 2, 8, 40 },
			// The Last Watch: Calder's Post, the watch and the Dinner Pail camp.
			{ TEXT("The Last Watch"), 20, 6, { .BasicUnpaid = 20, .RestlessUnpaid = 3 }, { { EChestKind::SupplyCrate, 1 }, { EChestKind::Mailbox, 1 } }, 2, 8, 60 },
			{ TEXT("Back Pay"), 25, 6, { .BasicUnpaid = 36, .RestlessUnpaid = 2, .GraveboundUnpaid = 1, .SoulfedUnpaid = 1 },
				{ { EChestKind::Footlocker, 1 }, { EChestKind::Strongbox, 1 } }, 2, 9, 0 },
			{ TEXT("Main 6-7"), 10, 7, { .BasicUnpaid = 12, .BossUnpaid = 1 }, { { EChestKind::Grave, 1 } }, 1, 3, Purse(TEXT("Main7")) },
		};
	}

	FString DescribeGuns(const FPlaythrough& Result)
	{
		return FString::Printf(TEXT("%.0f guns (%.0f from kills, %.0f from chests): Common %.0f, Uncommon %.0f, Rare %.0f, Epic %.0f, Legendary %.1f"),
			Result.Guns(), Result.KillGuns, Result.ChestGuns, Result.GunsByRarity[0], Result.GunsByRarity[1], Result.GunsByRarity[2],
			Result.GunsByRarity[3], Result.GunsByRarity[4]);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyBalanceTest, "Looter.Economy.Balance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyBalanceTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Definition = GetMutableDefault<UWeaponDefinition>();
	const FPlaythrough Story = Play(StoryStretches(), 6, Definition);
	const FPlaythrough Explorer = Play(ExplorerStretches(), 7, Definition);

	auto Report = [this](const TCHAR* Who, const FPlaythrough& Result)
	{
		AddInfo(FString::Printf(TEXT("%s: %d kills, %s. Earns %.0f silver (kills %.0f, containers %.0f, purses %d, gun sales %.0f); ammo and salt %.0f; "
			"%d guns of the day at about %.0f each, %.1f of them affordable."),
			Who, Result.Kills, *DescribeGuns(Result), Result.Earned(), Result.KillSilver, Result.ContainerSilver, Result.Purses, Result.GunSales,
			Result.Ammo, Result.Offers, Result.OfferPrice, Result.Affordable()));
	};
	Report(TEXT("Story only"), Story);
	Report(TEXT("Explorer"), Explorer);

	// The guns found: the analysis' 55 for the story, 125 for an explorer.
	TestTrue(FString::Printf(TEXT("The story drops about 55 guns (%.0f)"), Story.Guns()), Story.Guns() >= 45.0 && Story.Guns() <= 65.0);
	TestTrue(FString::Printf(TEXT("An explorer finds about 125 guns (%.0f)"), Explorer.Guns()), Explorer.Guns() >= 105.0 && Explorer.Guns() <= 145.0);
	TestTrue(FString::Printf(TEXT("...Legendaries rare (%.1f)"), Explorer.GunsByRarity[4]), Explorer.GunsByRarity[4] <= 4.0);

	// What they earn: selling is most of it, so the guns the player finds are the silver.
	TestTrue(FString::Printf(TEXT("The story alone earns 2,000 to 3,200 silver (%.0f)"), Story.Earned()), Story.Earned() >= 2000.0 && Story.Earned() <= 3200.0);
	TestTrue(FString::Printf(TEXT("An explorer earns 5,000 to 7,500 (%.0f)"), Explorer.Earned()), Explorer.Earned() >= 5000.0 && Explorer.Earned() <= 7500.0);
	TestTrue(FString::Printf(TEXT("An explorer earns more than twice the story (%.0f against %.0f)"), Explorer.Earned(), Story.Earned()),
		Explorer.Earned() > 2.0 * Story.Earned());

	// What they can spend it on: ammo and salt stay a small part, and the guns of the day (half an hour of play apiece, and a
	// level-up) can mostly be bought by an explorer, though not all of them, so there is still a choice to make.
	for (const FPlaythrough* Result : { &Story, &Explorer })
	{
		TestTrue(FString::Printf(TEXT("Ammo and salt are steady, not the purse (%.0f of %.0f)"), Result->Ammo, Result->Earned()), Result->Ammo < Result->Earned() * 0.4);
	}
	TestTrue(FString::Printf(TEXT("An explorer is offered 10 to 14 guns of the day (%d)"), Explorer.Offers), Explorer.Offers >= 10 && Explorer.Offers <= 14);
	TestTrue(FString::Printf(TEXT("...and can afford more than half of them (%.1f of %d)"), Explorer.Affordable(), Explorer.Offers),
		Explorer.Affordable() > 0.5 * Explorer.Offers);
	TestTrue(FString::Printf(TEXT("...but not every one (%.1f of %d)"), Explorer.Affordable(), Explorer.Offers), Explorer.Affordable() < Explorer.Offers);
	TestTrue(FString::Printf(TEXT("The story alone can buy two or more of its %d (%.1f)"), Story.Offers, Story.Affordable()), Story.Affordable() >= 2.0);
	TestTrue(FString::Printf(TEXT("An offer costs about 690 on average (%.0f)"), Explorer.OfferPrice), Explorer.OfferPrice > 600.0 && Explorer.OfferPrice < 800.0);

	// A kill's silver is a small part of the purse: drops stay about the guns.
	TestTrue(FString::Printf(TEXT("Kills are under a third of the explorer's silver (%.0f of %.0f)"), Explorer.KillSilver, Explorer.Earned()),
		Explorer.KillSilver < Explorer.Earned() / 3.0);
	TestTrue(TEXT("A Basic kill is worth under a box of ammo"), LooterEconomy::Expected(LooterEconomy::KillOdds(ECreatureRank::Basic, true), 4)
		< LooterEconomy::AmmoBoxPrice(EAmmoType::AssaultRifle, 4));
	return true;
}

#endif
