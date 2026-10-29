#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureRank.h"
#include "Economy/EconomyRules.h"
#include "Loot/Chest.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRewards.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "Math/RandomStream.h"

// The economy's rules alone (Docs/Progression.md, "Economy"): silver's growth with level, the drops' odds (the Unpaid over
// beasts, every rank, the containers), the vendors' prices and rules, the gun of the day's rarity, the missions' purses,
// and the balance a normal playthrough of Ransom's Rest strikes. The play tests (wallet, pickup, trades, save) are
// EconomyPlayTests.cpp.

namespace
{
	/** Every rank, lowest first (the five tiers, Soulfed, Boss). */
	const TConstArrayView<ECreatureRank> Ranks = LooterRanks::All;
	const EChestKind Containers[] = { EChestKind::SupplyCrate, EChestKind::Strongbox, EChestKind::Coffin, EChestKind::Grave,
		EChestKind::Mailbox, EChestKind::Footlocker };
	const EWeaponRarity Rarities[] = { EWeaponRarity::Common, EWeaponRarity::Uncommon, EWeaponRarity::Rare, EWeaponRarity::Epic,
		EWeaponRarity::Legendary };

	/** What Odds give per roll at Level over Rolls seeded draws. */
	double MeanOf(const LooterEconomy::FDropOdds& Odds, int32 Level, int32 Rolls, int32 Seed, int32* OutMin = nullptr, int32* OutMax = nullptr)
	{
		FRandomStream Random(Seed);
		int64 Sum = 0;
		int32 Low = MAX_int32;
		int32 High = 0;
		for (int32 Roll = 0; Roll < Rolls; ++Roll)
		{
			const int32 Amount = LooterEconomy::Roll(Odds, Level, Random);
			Sum += Amount;
			if (Amount > 0)
			{
				Low = FMath::Min(Low, Amount);
				High = FMath::Max(High, Amount);
			}
		}
		if (OutMin)
		{
			*OutMin = Low;
		}
		if (OutMax)
		{
			*OutMax = High;
		}
		return static_cast<double>(Sum) / Rolls;
	}

	FWeaponInstanceData MakeGun(EWeaponRarity Rarity, int32 Level, UWeaponDefinition* Definition)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = Definition;
		Gun.Rarity = Rarity;
		Gun.Level = Level;
		return Gun;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyScaleTest, "Looter.Economy.Rules.Scale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyScaleTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Level 1 is the base"), LooterEconomy::LevelScale(1), 1.f);
	TestEqual(TEXT("Below level 1 too"), LooterEconomy::LevelScale(0), 1.f);
	TestTrue(TEXT("8% of level 1's a level: level 11 is 1.8"), FMath::IsNearlyEqual(LooterEconomy::LevelScale(11), 1.8f, 0.001f));
	TestEqual(TEXT("15 at level 1"), LooterEconomy::AtLevel(15, 1), 15);
	TestEqual(TEXT("15 at level 6: 21"), LooterEconomy::AtLevel(15, 6), 21);
	TestEqual(TEXT("0 stays 0"), LooterEconomy::AtLevel(0, 9), 0);
	TestEqual(TEXT("1 stays at least 1"), LooterEconomy::AtLevel(1, 1), 1);
	TestEqual(TEXT("Grouped for people"), LooterEconomy::Format(1240), FText::AsNumber(1240).ToString());
	TestTrue(TEXT("Said in words with the currency's name"), LooterEconomy::Words(12).ToString().Contains(TEXT("silver")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyKillDropsTest, "Looter.Economy.Drops.Kills",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyKillDropsTest::RunTest(const FString& Parameters)
{
	float LastUnpaid = 0.f;
	float LastBeast = 0.f;
	for (const ECreatureRank Rank : Ranks)
	{
		const FString Name = UEnum::GetValueAsString(Rank);
		const LooterEconomy::FDropOdds Unpaid = LooterEconomy::KillOdds(Rank, true);
		const LooterEconomy::FDropOdds Beast = LooterEconomy::KillOdds(Rank, false);
		// The Unpaid had pockets: at least as often, and more on average, than a beast of the same rank.
		TestTrue(FString::Printf(TEXT("%s: the Unpaid drop silver at least as often as beasts"), *Name), Unpaid.Chance >= Beast.Chance);
		TestTrue(FString::Printf(TEXT("%s: and more of it"), *Name), Unpaid.Average() > Beast.Average());
		TestTrue(FString::Printf(TEXT("%s: a higher rank gives more (Unpaid)"), *Name), Unpaid.Average() > LastUnpaid);
		TestTrue(FString::Printf(TEXT("%s: a higher rank gives more (beasts)"), *Name), Beast.Average() > LastBeast);
		LastUnpaid = Unpaid.Average();
		LastBeast = Beast.Average();
	}
	TestTrue(TEXT("A boss always drops silver"), LooterEconomy::KillOdds(ECreatureRank::Boss, true).Chance >= 1.f
		&& LooterEconomy::KillOdds(ECreatureRank::Boss, false).Chance >= 1.f);

	// Regular, Restless and Gravebound pay exactly what they did before the top two tiers (Main's call, 2026-10-20: the tiers
	// took nothing away); Hallowed and Ethereal 1.6 and 2.5 times a Gravebound kill.
	struct FKept
	{
		ECreatureRank Rank;
		bool bUnpaid;
		LooterEconomy::FDropOdds Odds;
	};
	const FKept Kept[] = {
		{ ECreatureRank::Basic, true, { 0.55f, 3, 7 } }, { ECreatureRank::Rare, true, { 0.85f, 9, 15 } }, { ECreatureRank::Epic, true, { 1.f, 18, 30 } },
		{ ECreatureRank::Basic, false, { 0.2f, 2, 4 } }, { ECreatureRank::Rare, false, { 0.5f, 5, 9 } }, { ECreatureRank::Epic, false, { 0.8f, 10, 18 } } };
	for (const FKept& Each : Kept)
	{
		const LooterEconomy::FDropOdds Odds = LooterEconomy::KillOdds(Each.Rank, Each.bUnpaid);
		TestTrue(FString::Printf(TEXT("%s %s: as before (%.0f%%, %d-%d)"), *UEnum::GetValueAsString(Each.Rank), Each.bUnpaid ? TEXT("Unpaid") : TEXT("beast"),
			Each.Odds.Chance * 100.f, Each.Odds.Min, Each.Odds.Max),
			FMath::IsNearlyEqual(Odds.Chance, Each.Odds.Chance) && Odds.Min == Each.Odds.Min && Odds.Max == Each.Odds.Max);
	}
	for (const bool bUnpaid : { true, false })
	{
		const float Gravebound = LooterEconomy::KillOdds(ECreatureRank::Epic, bUnpaid).Average();
		const float Hallowed = LooterEconomy::KillOdds(ECreatureRank::Hallowed, bUnpaid).Average();
		const float Ethereal = LooterEconomy::KillOdds(ECreatureRank::Ethereal, bUnpaid).Average();
		const TCHAR* Kind = bUnpaid ? TEXT("Unpaid") : TEXT("beast");
		TestTrue(FString::Printf(TEXT("A Hallowed %s pays 1.6 Gravebound kills (%.2f against %.2f)"), Kind, Hallowed, Gravebound),
			FMath::Abs(Hallowed - 1.6f * Gravebound) <= 0.05f * 1.6f * Gravebound);
		TestTrue(FString::Printf(TEXT("An Ethereal %s pays 2.5 (%.2f)"), Kind, Ethereal), FMath::Abs(Ethereal - 2.5f * Gravebound) <= 0.05f * 2.5f * Gravebound);
	}
	TestTrue(TEXT("A Basic beast drops it now and then, not every kill"), LooterEconomy::KillOdds(ECreatureRank::Basic, false).Chance < 0.5f);

	// Rolled: the mean comes out as the odds say, at the level's growth, every amount within the range at that level.
	for (const int32 Level : { 1, 5 })
	{
		const LooterEconomy::FDropOdds Odds = LooterEconomy::KillOdds(ECreatureRank::Basic, true);
		int32 Low = 0;
		int32 High = 0;
		const double Mean = MeanOf(Odds, Level, 20000, 7 + Level, &Low, &High);
		const double Expected = LooterEconomy::Expected(Odds, Level);
		TestTrue(FString::Printf(TEXT("Level %d: a Basic Unpaid's mean %.2f is the expected %.2f (within 5%%)"), Level, Mean, Expected),
			FMath::Abs(Mean - Expected) <= Expected * 0.05);
		TestTrue(FString::Printf(TEXT("Level %d: every amount within %d-%d (seen %d-%d)"), Level, LooterEconomy::AtLevel(Odds.Min, Level),
			LooterEconomy::AtLevel(Odds.Max, Level), Low, High), Low >= LooterEconomy::AtLevel(Odds.Min, Level) && High <= LooterEconomy::AtLevel(Odds.Max, Level));
	}

	// A roll always takes the same two numbers, whatever it gives, so what's rolled after it doesn't move.
	FRandomStream Sure(99);
	FRandomStream Never(99);
	LooterEconomy::Roll(LooterEconomy::KillOdds(ECreatureRank::Boss, true), 1, Sure);
	LooterEconomy::Roll(LooterEconomy::FDropOdds{ 0.f, 1, 2 }, 1, Never);
	TestEqual(TEXT("The stream moves the same after any roll"), Sure.GetCurrentSeed(), Never.GetCurrentSeed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyContainerDropsTest, "Looter.Economy.Drops.Containers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyContainerDropsTest::RunTest(const FString& Parameters)
{
	const float Strongbox = LooterEconomy::ContainerOdds(EChestKind::Strongbox).Average();
	for (const EChestKind Kind : Containers)
	{
		const LooterEconomy::FDropOdds Odds = LooterEconomy::ContainerOdds(Kind);
		const FString Name = UEnum::GetValueAsString(Kind);
		TestTrue(FString::Printf(TEXT("%s gives silver"), *Name), Odds.Chance > 0.f && Odds.Min > 0 && Odds.Max >= Odds.Min);
		TestTrue(FString::Printf(TEXT("%s: the gang's Strongbox gives the most"), *Name), Kind == EChestKind::Strongbox || Odds.Average() < Strongbox);
		// A container is worth opening: more than a Basic Unpaid's kill.
		TestTrue(FString::Printf(TEXT("%s: more than a Basic kill"), *Name), Odds.Average() > LooterEconomy::KillOdds(ECreatureRank::Basic, true).Average());
	}
	const LooterEconomy::FDropOdds Crate = LooterEconomy::BreakableOdds();
	TestTrue(TEXT("A crate now and then, a little"), Crate.Chance > 0.f && Crate.Chance < 0.5f
		&& Crate.Average() < LooterEconomy::KillOdds(ECreatureRank::Basic, true).Average());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyPricesTest, "Looter.Economy.Prices",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyPricesTest::RunTest(const FString& Parameters)
{
	// Ammo by the box: a chest pickup's 36, at most half the most carried.
	TestEqual(TEXT("A box of rifle rounds: 36"), LooterEconomy::AmmoBoxRounds(EAmmoType::AssaultRifle), 36);
	TestEqual(TEXT("A box of shells: 24 (half of 48)"), LooterEconomy::AmmoBoxRounds(EAmmoType::Shotgun), 24);
	TestEqual(TEXT("A box of pistol rounds: 36"), LooterEconomy::AmmoBoxRounds(EAmmoType::Pistol), 36);
	TestEqual(TEXT("A box of sniper rounds: 18"), LooterEconomy::AmmoBoxRounds(EAmmoType::Sniper), 18);
	TestEqual(TEXT("A full box at level 1: 15"), LooterEconomy::AmmoPrice(EAmmoType::AssaultRifle, 36, 1), 15);
	TestEqual(TEXT("More than a box is a box"), LooterEconomy::AmmoPrice(EAmmoType::AssaultRifle, 80, 1), 15);
	TestEqual(TEXT("A third of a box: 5"), LooterEconomy::AmmoPrice(EAmmoType::AssaultRifle, 12, 1), 5);
	TestEqual(TEXT("One round still costs 1"), LooterEconomy::AmmoPrice(EAmmoType::AssaultRifle, 1, 1), 1);
	TestEqual(TEXT("None costs nothing"), LooterEconomy::AmmoPrice(EAmmoType::AssaultRifle, 0, 1), 0);
	TestTrue(TEXT("Dearer at a higher level"), LooterEconomy::AmmoBoxPrice(EAmmoType::Shotgun, 7) > LooterEconomy::AmmoBoxPrice(EAmmoType::Shotgun, 1));
	TestEqual(TEXT("A jar of grave salt at level 1: 25"), LooterEconomy::GrenadePrice(1), 25);

	// Guns: dearer by rarity both ways, a vendor asking far more than it pays (selling never buys the gun back).
	int32 LastSell = 0;
	int32 LastBuy = 0;
	for (const EWeaponRarity Rarity : Rarities)
	{
		const FString Name = UEnum::GetValueAsString(Rarity);
		TestTrue(FString::Printf(TEXT("%s: worth more than the rarity under it"), *Name), LooterEconomy::SellBase(Rarity) > LastSell
			&& LooterEconomy::BuyBase(Rarity) > LastBuy);
		TestTrue(FString::Printf(TEXT("%s: bought at more than five times what it sells for"), *Name),
			LooterEconomy::BuyBase(Rarity) > 5 * LooterEconomy::SellBase(Rarity));
		const bool bAsks = LooterEconomy::SaleNeedsConfirm(MakeGun(Rarity, 1, nullptr));
		TestTrue(FString::Printf(TEXT("%s: Rare and up ask before they're sold"), *Name),
			bAsks == (static_cast<uint8>(Rarity) >= static_cast<uint8>(EWeaponRarity::Rare)));
		LastSell = LooterEconomy::SellBase(Rarity);
		LastBuy = LooterEconomy::BuyBase(Rarity);
	}

	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	const FWeaponInstanceData Plain = MakeGun(EWeaponRarity::Uncommon, 4, Rifle);
	TestEqual(TEXT("An Uncommon at level 4 sells for 18 grown 24%: 22"), LooterEconomy::GunSellPrice(Plain), 22);
	TestEqual(TEXT("...and costs 150 grown 24%: 186"), LooterEconomy::GunBuyPrice(Plain), 186);
	FWeaponInstanceData Cursed = Plain;
	Cursed.Curse = TEXT("Hungry");
	TestTrue(TEXT("A cursed iron fetches less"), LooterEconomy::GunSellPrice(Cursed) < LooterEconomy::GunSellPrice(Plain)
		&& LooterEconomy::GunSellPrice(Cursed) > 0);
	FWeaponInstanceData Named = Plain;
	UNamedWeaponDefinition* Heirloom = NewObject<UNamedWeaponDefinition>();
	Heirloom->DisplayName = FText::FromString(TEXT("Heirloom"));
	Named.Named = Heirloom;
	FText Why;
	TestFalse(TEXT("A named gun isn't for sale"), LooterEconomy::CanSell(Named, &Why));
	TestTrue(TEXT("...and the vendor says so"), Why.ToString().Contains(TEXT("Heirloom")));
	TestEqual(TEXT("...it fetches nothing"), LooterEconomy::GunSellPrice(Named), 0);
	TestFalse(TEXT("No gun, no sale"), LooterEconomy::CanSell(FWeaponInstanceData()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyOfferOddsTest, "Looter.Economy.Offer.Odds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyOfferOddsTest::RunTest(const FString& Parameters)
{
	float Sum = 0.f;
	for (const EWeaponRarity Rarity : Rarities)
	{
		Sum += LooterEconomy::OfferRarityChance(Rarity);
	}
	TestTrue(TEXT("The odds add to 1"), FMath::IsNearlyEqual(Sum, 1.f, 0.0001f));
	TestEqual(TEXT("Never a Common"), LooterEconomy::OfferRarityChance(EWeaponRarity::Common), 0.f);
	TestTrue(TEXT("Uncommon most days"), LooterEconomy::OfferRarityChance(EWeaponRarity::Uncommon) > 0.5f);

	FRandomStream Random(2026);
	int32 Counts[5] = {};
	constexpr int32 Rolls = 20000;
	for (int32 Roll = 0; Roll < Rolls; ++Roll)
	{
		++Counts[static_cast<int32>(LooterEconomy::RollOfferRarity(Random))];
	}
	TestEqual(TEXT("None Common in 20,000 days"), Counts[0], 0);
	for (const EWeaponRarity Rarity : Rarities)
	{
		const float Seen = static_cast<float>(Counts[static_cast<int32>(Rarity)]) / Rolls;
		const float Chance = LooterEconomy::OfferRarityChance(Rarity);
		TestTrue(FString::Printf(TEXT("%s: %.3f of days, the odds' %.3f"), *UEnum::GetValueAsString(Rarity), Seen, Chance),
			FMath::Abs(Seen - Chance) <= FMath::Max(0.01f, Chance * 0.1f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyMissionPursesTest, "Looter.Economy.Missions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyMissionPursesTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest's givers pay a fixed purse at the turn-in; Sexton, the boss, the Gravemother and practice pay none.
	int32 Total = 0;
	for (const TCHAR* Mission : { TEXT("Main1"), TEXT("Main2"), TEXT("Main3"), TEXT("Main4"), TEXT("Main5"), TEXT("Main6"), TEXT("Main7"),
		TEXT("Side1"), TEXT("Side2"), TEXT("Side3") })
	{
		Total += MissionRewards::SilverFor(FName(Mission));
	}
	TestEqual(TEXT("The story's purses add to 320 (Docs/Progression.md)"), Total, 320);
	for (const TCHAR* None : { TEXT("Main2"), TEXT("Main6"), TEXT("Side3"), TEXT("Tutorial"), TEXT("WebHollow"), TEXT("BoardSkiff"), TEXT("NoSuchMission") })
	{
		TestEqual(FString::Printf(TEXT("%s pays no silver"), None), MissionRewards::SilverFor(FName(None)), 0);
	}
	TestEqual(TEXT("Cold Welcome pays 40"), MissionRewards::SilverFor(TEXT("Main3")), 40);
	TestEqual(TEXT("Without a world, nothing is paid"), MissionRewards::GiveSilver(nullptr, TEXT("Main3")), 0);

	// The missions of 2026-10-13 carry purses of their own (FMissionRewards::Silver, the mission scripts'): Calder's 30,
	// Fenn's 30, Thaddeus's 40, the sheriff's bounty at Pruitt's 80, Hiram's 40, the post's 60; Back Pay's is its choice's
	// (the kept payroll, 400). Checked where their assets are made.
	struct FOwnPurse
	{
		const TCHAR* Mission;
		int32 Silver;
	};
	const FOwnPurse OwnPurses[] = { { TEXT("CaldersCaches"), 30 }, { TEXT("OldFare"), 30 }, { TEXT("KnockingBelow"), 40 },
		{ TEXT("Side3"), 80 }, { TEXT("DeadLift"), 40 }, { TEXT("LastWatch"), 60 } };
	const TArray<UMissionDefinition*> Missions = UMissionDefinition::LoadAll();
	for (const FOwnPurse& Purse : OwnPurses)
	{
		const FName Id(Purse.Mission);
		UMissionDefinition* const* Found = Missions.FindByPredicate([Id](const UMissionDefinition* Mission) { return Mission && Mission->GetMissionId() == Id; });
		if (Found)
		{
			TestEqual(FString::Printf(TEXT("%s pays its own purse"), Purse.Mission), MissionRewards::PurseOf((*Found)->Rewards, Id), Purse.Silver);
		}
	}
	UMissionDefinition* const* BackPay = Missions.FindByPredicate([](const UMissionDefinition* Mission)
	{
		return Mission && Mission->GetMissionId() == FName(TEXT("BackPay"));
	});
	if (BackPay)
	{
		TestEqual(TEXT("Back Pay's kept payroll: 400 silver"), MissionRewards::PurseOf((*BackPay)->RewardsFor(1), TEXT("BackPay")), 400);
		TestEqual(TEXT("...the eleven paid: none (a Rare gun instead)"), MissionRewards::PurseOf((*BackPay)->RewardsFor(0), TEXT("BackPay")), 0);
	}
	FMissionRewardsGiven Given;
	TestTrue(TEXT("Nothing given is empty"), Given.IsEmpty());
	Given.Silver = 40;
	TestFalse(TEXT("A purse alone isn't empty: the banner announces it"), Given.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyBalanceTest, "Looter.Economy.Balance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyBalanceTest::RunTest(const FString& Parameters)
{
	// A normal playthrough of Ransom's Rest as Docs/Progression.md counts it, at the levels it's played at (1 to 7, about 4
	// on average): every main fight, the valley's side missions (the posters, Calder's caches, the old fare, Amos's hay,
	// Tilly's cellar), the roaming walkers, about twelve containers opened, fifteen crates broken, twenty guns sold. It
	// should buy a few guns of the day and steady ammo, and never so much that loot stops mattering. (The mountain, the
	// Gravemother's bounty and a kept payroll come on top, for an explorer.)
	constexpr int32 Level = 4;
	double Earned = 0.0;
	auto Kills = [&Earned](ECreatureRank Rank, bool bUnpaid, int32 Count)
	{
		Earned += Count * LooterEconomy::Expected(LooterEconomy::KillOdds(Rank, bUnpaid), Level);
	};
	// (FIGHTS lane 6, 2026-10-28: the yard's Bell-Ringer, Amos's nine and his Bell-Ringer, the fold's sixth: 53 -> 58; a Restless
	// leader on boot hill, the north road and the west road: 7 -> 10; the yard's Gravebound leader.)
	Kills(ECreatureRank::Basic, true, 58);   // the gate, the chapel yard, Amos's hands, the roaming, the churchyard, the fold, Abel's adds, the depot, the cellar
	Kills(ECreatureRank::Rare, true, 10);    // a Restless in each of seven fights and on the three roaming stretches
	Kills(ECreatureRank::Epic, true, 1);     // the Sexton's Debt
	Kills(ECreatureRank::Basic, false, 14);  // the bluff nest and the Sink's spiders
	Kills(ECreatureRank::Rare, false, 1);
	Kills(ECreatureRank::Boss, true, 1);     // Abel
	const double KillSilver = Earned;
	for (const EChestKind Kind : { EChestKind::Mailbox, EChestKind::Mailbox, EChestKind::Footlocker, EChestKind::Footlocker,
		EChestKind::Coffin, EChestKind::Coffin, EChestKind::Grave, EChestKind::Grave, EChestKind::SupplyCrate, EChestKind::SupplyCrate,
		EChestKind::SupplyCrate, EChestKind::Strongbox })
	{
		Earned += LooterEconomy::Expected(LooterEconomy::ContainerOdds(Kind), Level);
	}
	Earned += 15 * LooterEconomy::Expected(LooterEconomy::BreakableOdds(), Level);
	for (const TCHAR* Mission : { TEXT("Main1"), TEXT("Main3"), TEXT("Main4"), TEXT("Main5"), TEXT("Main7"), TEXT("Side1"), TEXT("Side2") })
	{
		Earned += MissionRewards::SilverFor(FName(Mission));
	}
	// The valley's new purses, their missions' own (Looter.Economy.Missions checks the assets): Calder's, Fenn's, Thaddeus's.
	Earned += 30 + 30 + 40;
	// Twenty guns sold, as they drop: twelve Common, six Uncommon, two Rare.
	Earned += 12 * LooterEconomy::AtLevel(LooterEconomy::SellBase(EWeaponRarity::Common), Level)
		+ 6 * LooterEconomy::AtLevel(LooterEconomy::SellBase(EWeaponRarity::Uncommon), Level)
		+ 2 * LooterEconomy::AtLevel(LooterEconomy::SellBase(EWeaponRarity::Rare), Level);
	AddInfo(FString::Printf(TEXT("A normal playthrough earns about %.0f silver (kills %.0f)."), Earned, KillSilver));
	TestTrue(FString::Printf(TEXT("It earns 1,000 to 1,800 silver (%.0f)"), Earned), Earned >= 1000.0 && Earned <= 1800.0);

	// Steady ammo and salt: ten boxes and six jars.
	const double Supplies = 10 * LooterEconomy::AmmoBoxPrice(EAmmoType::AssaultRifle, Level) + 6 * LooterEconomy::GrenadePrice(Level);
	const double ForGuns = Earned - Supplies;
	const int32 Uncommon = LooterEconomy::AtLevel(LooterEconomy::BuyBase(EWeaponRarity::Uncommon), Level);
	const int32 Rare = LooterEconomy::AtLevel(LooterEconomy::BuyBase(EWeaponRarity::Rare), Level);
	const int32 Legendary = LooterEconomy::AtLevel(LooterEconomy::BuyBase(EWeaponRarity::Legendary), 6);
	TestTrue(FString::Printf(TEXT("Supplies are steady, not the whole purse (%.0f of %.0f)"), Supplies, Earned), Supplies < Earned * 0.4);
	TestTrue(FString::Printf(TEXT("After them, a few guns of the day: 3 to 6 Uncommon (%.1f)"), ForGuns / Uncommon),
		ForGuns / Uncommon >= 3.0 && ForGuns / Uncommon <= 6.0);
	TestTrue(FString::Printf(TEXT("...or one or two Rare (%.1f)"), ForGuns / Rare), ForGuns / Rare >= 1.0 && ForGuns / Rare < 3.0);
	TestTrue(FString::Printf(TEXT("A Legendary of the day costs more than the whole playthrough earns (%d)"), Legendary), Legendary > Earned);
	// What a kill gives is a small part of the purse: drops stay about the guns.
	TestTrue(TEXT("A Basic kill is worth under a box of ammo"), LooterEconomy::Expected(LooterEconomy::KillOdds(ECreatureRank::Basic, true), Level)
		< LooterEconomy::AmmoBoxPrice(EAmmoType::AssaultRifle, Level));
	return true;
}

#endif
