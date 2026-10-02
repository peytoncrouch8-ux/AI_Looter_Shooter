#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Loot/AmmoPickup.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Modules/ModuleManager.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr int32 NumTiers = static_cast<int32>(EWeaponRarity::Legendary) + 1;

	const TCHAR* TierName(int32 Tier)
	{
		static const TCHAR* Names[] = { TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"), TEXT("Epic"), TEXT("Legendary") };
		static_assert(UE_ARRAY_COUNT(Names) == NumTiers, "One name per rarity tier");
		return Names[Tier];
	}

	float TierWeight(const UWeaponDefinition& Definition, int32 Tier)
	{
		return Definition.GetRarityInfo(static_cast<EWeaponRarity>(Tier)).Weight;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootDropOddsTest, "Looter.Loot.DropOdds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootDropOddsTest::RunTest(const FString& Parameters)
{
	// A table with the code defaults and one weapon with the stock rarity weights, rolled for many kills from a fixed seed.
	ULootTable* Table = NewObject<ULootTable>();
	UWeaponDefinition* Weapon = NewObject<UWeaponDefinition>();
	FLootTableEntry Entry;
	Entry.Weapon = Weapon;
	Table->Entries.Add(Entry);

	constexpr int32 Kills = 20000;
	FRandomStream Random(20260927);
	int32 KillsWithWeapons = 0;
	int32 KillsWithoutAmmo = 0;
	int32 Weapons = 0;
	int32 AmmoDrops = 0;
	int32 Tiers[NumTiers] = {};
	int32 AmmoTypes[LooterAmmo::NumTypes] = {};
	// The fewest and most rounds seen in one pickup of each class.
	int32 FewestRounds[LooterAmmo::NumTypes];
	int32 MostRounds[LooterAmmo::NumTypes];
	for (int32 Index = 0; Index < LooterAmmo::NumTypes; ++Index)
	{
		FewestRounds[Index] = MAX_int32;
		MostRounds[Index] = 0;
	}
	for (int32 Kill = 0; Kill < Kills; ++Kill)
	{
		const FLootRoll Roll = ULootLibrary::RollLoot(Table, 1, 0.f, Random);
		KillsWithWeapons += Roll.Weapons.Num() > 0 ? 1 : 0;
		KillsWithoutAmmo += Roll.Ammo.IsEmpty() ? 1 : 0;
		for (const FWeaponInstanceData& Instance : Roll.Weapons)
		{
			++Tiers[static_cast<int32>(Instance.Rarity)];
			++Weapons;
		}
		for (const FAmmoDrop& Drop : Roll.Ammo)
		{
			const int32 TypeIndex = static_cast<int32>(Drop.Type);
			++AmmoTypes[TypeIndex];
			++AmmoDrops;
			FewestRounds[TypeIndex] = FMath::Min(FewestRounds[TypeIndex], Drop.Amount);
			MostRounds[TypeIndex] = FMath::Max(MostRounds[TypeIndex], Drop.Amount);
		}
	}

	// Weapons: only some kills, one weapon each.
	const float WeaponRate = static_cast<float>(KillsWithWeapons) / Kills;
	TestTrue(FString::Printf(TEXT("Weapons drop on 20-40%% of kills (%.1f%%)"), WeaponRate * 100.f), WeaponRate >= 0.2f && WeaponRate <= 0.4f);
	TestNearlyEqual(TEXT("Weapon drop rate matches the table"), WeaponRate, Table->WeaponDropChance, 0.015f);
	TestEqual(TEXT("One weapon per weapon drop"), Weapons, KillsWithWeapons);

	// Rarity: Common most often, each tier up rarer, Legendary least often but possible, in the proportions of the weights.
	float TotalWeight = 0.f;
	for (int32 Tier = 0; Tier < NumTiers; ++Tier)
	{
		TotalWeight += TierWeight(*Weapon, Tier);
	}
	for (int32 Tier = 0; Tier < NumTiers; ++Tier)
	{
		const float Share = static_cast<float>(Tiers[Tier]) / FMath::Max(Weapons, 1);
		TestNearlyEqual(FString::Printf(TEXT("%s share of weapon drops"), TierName(Tier)), Share, TierWeight(*Weapon, Tier) / TotalWeight, 0.02f);
		if (Tier + 1 < NumTiers)
		{
			TestTrue(FString::Printf(TEXT("%s (%d) drops more often than %s (%d)"), TierName(Tier), Tiers[Tier], TierName(Tier + 1), Tiers[Tier + 1]),
				Tiers[Tier] > Tiers[Tier + 1]);
		}
	}
	TestTrue(TEXT("Legendaries still drop"), Tiers[NumTiers - 1] > 0);

	// Ammo: every kill, every class, each pickup 18 to 36 rounds whatever its class (the user's rule), both ends reached.
	TestEqual(TEXT("Every kill drops ammo"), KillsWithoutAmmo, 0);
	for (const EAmmoType Type : LooterAmmo::AllTypes())
	{
		const int32 TypeIndex = static_cast<int32>(Type);
		const TCHAR* Name = LooterAmmo::GetInfo(Type).Name;
		const float Share = static_cast<float>(AmmoTypes[TypeIndex]) / FMath::Max(AmmoDrops, 1);
		TestNearlyEqual(FString::Printf(TEXT("%s share of ammo drops"), Name), Share, 1.f / LooterAmmo::NumTypes, 0.02f);
		TestEqual(FString::Printf(TEXT("Fewest %s in a kill's pickup"), Name), FewestRounds[TypeIndex], 18);
		TestEqual(FString::Printf(TEXT("Most %s in a kill's pickup"), Name), MostRounds[TypeIndex], 36);
	}

	AddInfo(FString::Printf(TEXT("%d kills: weapons on %.1f%% (C/U/R/E/L %d/%d/%d/%d/%d), %.2f ammo drops per kill"), Kills, WeaponRate * 100.f,
		Tiers[0], Tiers[1], Tiers[2], Tiers[3], Tiers[4], static_cast<float>(AmmoDrops) / Kills));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootKillWeaponAmmoTest, "Looter.Loot.KillWeaponAmmo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootKillWeaponAmmoTest::RunTest(const FString& Parameters)
{
	// Ammo from shotgun kills: shells come up KillWeaponAmmoBias times as often as each other class, and every other
	// class still drops.
	ULootTable* Table = NewObject<ULootTable>();
	const float Bias = Table->KillWeaponAmmoBias;
	TestTrue(FString::Printf(TEXT("A slight lean (%.1fx)"), Bias), Bias > 1.f && Bias <= 3.f);

	constexpr int32 Picks = 30000;
	FRandomStream Random(20261001);
	int32 Counts[LooterAmmo::NumTypes] = {};
	for (int32 Pick = 0; Pick < Picks; ++Pick)
	{
		++Counts[static_cast<int32>(ULootLibrary::PickAmmoType(Table, Random, EAmmoType::Shotgun))];
	}
	const float TotalWeight = LooterAmmo::NumTypes - 1 + Bias;
	for (const EAmmoType Type : LooterAmmo::AllTypes())
	{
		const float Share = static_cast<float>(Counts[static_cast<int32>(Type)]) / Picks;
		const float Expected = (Type == EAmmoType::Shotgun ? Bias : 1.f) / TotalWeight;
		TestNearlyEqual(FString::Printf(TEXT("%s share after shotgun kills"), LooterAmmo::GetInfo(Type).Name), Share, Expected, 0.015f);
	}

	// A class the table never drops stays out, even when its gun made the kill.
	Table->AmmoTypes.Reset();
	for (const EAmmoType Type : { EAmmoType::AssaultRifle, EAmmoType::Pistol })
	{
		FAmmoLootEntry& Entry = Table->AmmoTypes.AddDefaulted_GetRef();
		Entry.Type = Type;
	}
	bool bOnlyListed = true;
	for (int32 Pick = 0; Pick < 1000; ++Pick)
	{
		const EAmmoType Type = ULootLibrary::PickAmmoType(Table, Random, EAmmoType::Sniper);
		bOnlyListed &= Type == EAmmoType::AssaultRifle || Type == EAmmoType::Pistol;
	}
	TestTrue(TEXT("Only the table's classes drop"), bOnlyListed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootAmmoAmountsTest, "Looter.Loot.AmmoAmounts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootAmmoAmountsTest::RunTest(const FString& Parameters)
{
	// The user's rules: a kill's ammo pickup holds 18 to 36 rounds, a loot chest's exactly 36.
	TestEqual(TEXT("Kill pickups hold at least 18"), LooterLoot::KillAmmoAmountMin, 18);
	TestEqual(TEXT("Kill pickups hold at most 36"), LooterLoot::KillAmmoAmountMax, 36);
	TestEqual(TEXT("Chest pickups hold 36"), LooterLoot::ChestAmmoAmount, 36);

	// A new table drops a kill's range, every amount in it possible.
	ULootTable* Table = NewObject<ULootTable>();
	TestTrue(TEXT("A new table drops a kill's range"), Table->AmmoAmountMin == 18 && Table->AmmoAmountMax == 36);
	FRandomStream Random(20261002);
	TSet<int32> Seen;
	bool bInRange = true;
	for (int32 Pick = 0; Pick < 5000; ++Pick)
	{
		const int32 Amount = ULootLibrary::RollAmmoAmount(Table, Random);
		bInRange &= Amount >= 18 && Amount <= 36;
		Seen.Add(Amount);
	}
	TestTrue(TEXT("Every kill pickup holds 18 to 36"), bInRange);
	TestEqual(TEXT("Every amount from 18 to 36 comes up"), Seen.Num(), 36 - 18 + 1);

	// The same stream gives the same amounts, so a seeded loot roll repeats.
	FRandomStream First(7);
	FRandomStream Second(7);
	bool bRepeats = true;
	for (int32 Pick = 0; Pick < 100; ++Pick)
	{
		bRepeats &= ULootLibrary::RollAmmoAmount(Table, First) == ULootLibrary::RollAmmoAmount(Table, Second);
	}
	TestTrue(TEXT("A seeded stream repeats the amounts"), bRepeats);

	// A chest's table: always 36, every pickup of every roll.
	ULootTable* Chest = NewObject<ULootTable>();
	Chest->UseChestAmmoAmount();
	Chest->MinAmmoDrops = 3;
	Chest->MaxAmmoDrops = 3;
	int32 ChestDrops = 0;
	bool bChestFixed = true;
	for (int32 Open = 0; Open < 500; ++Open)
	{
		const FLootRoll Roll = ULootLibrary::RollLoot(Chest, 1, 0.f, Random);
		for (const FAmmoDrop& Drop : Roll.Ammo)
		{
			bChestFixed &= Drop.Amount == LooterLoot::ChestAmmoAmount;
			++ChestDrops;
		}
	}
	TestEqual(TEXT("Every chest opening drops its ammo"), ChestDrops, 500 * 3);
	TestTrue(TEXT("Every chest pickup holds 36"), bChestFixed);

	// Odd settings stay sane: no table means a kill's range, a backwards range drops its Min, never less than a round.
	bool bNoTableInRange = true;
	for (int32 Pick = 0; Pick < 200; ++Pick)
	{
		const int32 Amount = ULootLibrary::RollAmmoAmount(nullptr, Random);
		bNoTableInRange &= Amount >= 18 && Amount <= 36;
	}
	TestTrue(TEXT("No table: a kill's range"), bNoTableInRange);
	Table->AmmoAmountMin = 30;
	Table->AmmoAmountMax = 20;
	TestEqual(TEXT("A backwards range drops its Min"), ULootLibrary::RollAmmoAmount(Table, Random), 30);
	Table->AmmoAmountMin = 0;
	Table->AmmoAmountMax = 0;
	TestEqual(TEXT("At least one round"), ULootLibrary::RollAmmoAmount(Table, Random), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootAmmoPoolTest, "Looter.Loot.AmmoPool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootAmmoPoolTest::RunTest(const FString& Parameters)
{
	UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>();
	const EAmmoType Type = EAmmoType::Shotgun;
	const int32 Pickup = LooterAmmo::GetInfo(Type).PickupAmount;
	const int32 Max = Inventory->GetMaxAmmo(Type);

	TestEqual(TEXT("Starts empty"), Inventory->GetAmmo(Type), 0);
	TestEqual(TEXT("A pickup goes in whole"), Inventory->AddAmmo(Type, Pickup), Pickup);
	TestEqual(TEXT("Only fills up to the carry limit"), Inventory->AddAmmo(Type, Max), Max - Pickup);
	TestEqual(TEXT("Full"), Inventory->GetAmmo(Type), Max);
	TestEqual(TEXT("Nothing fits when full"), Inventory->AddAmmo(Type, Pickup), 0);
	TestEqual(TEXT("Other classes have their own pool"), Inventory->GetAmmo(EAmmoType::AssaultRifle), 0);

	TestEqual(TEXT("Takes what a reload asks for"), Inventory->TakeAmmo(Type, 5), 5);
	TestEqual(TEXT("Never more than is carried"), Inventory->TakeAmmo(Type, Max * 2), Max - 5);
	TestEqual(TEXT("Empty again"), Inventory->GetAmmo(Type), 0);
	TestEqual(TEXT("Nothing from an empty pool"), Inventory->TakeAmmo(Type, 1), 0);

	TestEqual(TEXT("Negative amounts are ignored"), Inventory->AddAmmo(Type, -10), 0);
	TestEqual(TEXT("Invalid ammo class is ignored"), Inventory->AddAmmo(EAmmoType::Count, Pickup), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootDefaultTableTest, "Looter.Loot.DefaultTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootDefaultTableTest::RunTest(const FString& Parameters)
{
	// The table every creature and the target dummy fall back on.
	const ULootTable* Table = ULootLibrary::GetDefaultLootTable();
	if (!TestNotNull(TEXT("Default loot table asset loads"), Table))
	{
		return false;
	}

	TestTrue(FString::Printf(TEXT("Weapon drop chance within 20-40%% (%.2f)"), Table->WeaponDropChance),
		Table->WeaponDropChance >= 0.2f && Table->WeaponDropChance <= 0.4f);
	TestTrue(TEXT("One weapon per weapon drop"), Table->MinWeaponDrops == 1 && Table->MaxWeaponDrops == 1);
	TestEqual(TEXT("No luck bonus, so rarity follows each weapon's own odds"), Table->Luck, 0.f);
	TestTrue(TEXT("Every kill drops ammo"), Table->AmmoDropChance >= 1.f && Table->MinAmmoDrops >= 1);
	TestTrue(FString::Printf(TEXT("Ammo leans slightly toward the kill weapon's class (%.1fx)"), Table->KillWeaponAmmoBias),
		Table->KillWeaponAmmoBias > 1.f && Table->KillWeaponAmmoBias <= 3.f);
	TestTrue(FString::Printf(TEXT("Each ammo pickup holds 18 to 36 rounds (%d to %d)"), Table->AmmoAmountMin, Table->AmmoAmountMax),
		Table->AmmoAmountMin == LooterLoot::KillAmmoAmountMin && Table->AmmoAmountMax == LooterLoot::KillAmmoAmountMax);

	int32 Weapons = 0;
	for (const FLootTableEntry& Entry : Table->Entries)
	{
		Weapons += Entry.Weapon && Entry.Weight > 0.f ? 1 : 0;
	}
	TestTrue(TEXT("Has weapons to drop"), Weapons > 0);

	for (const EAmmoType Type : LooterAmmo::AllTypes())
	{
		const FAmmoLootEntry* AmmoEntry = Table->AmmoTypes.FindByPredicate([Type](const FAmmoLootEntry& Candidate) { return Candidate.Type == Type; });
		TestTrue(FString::Printf(TEXT("%s can drop"), LooterAmmo::GetInfo(Type).Name), AmmoEntry && AmmoEntry->Weight > 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootWeaponRarityOrderTest, "Looter.Loot.WeaponRarityOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootWeaponRarityOrderTest::RunTest(const FString& Parameters)
{
	// Every weapon in the project, now and later: Common most likely, each tier up less likely, Legendary rarest but possible.
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.WaitForCompletion();
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(UWeaponDefinition::StaticClass()->GetClassPathName(), Assets, true);
	TestTrue(TEXT("Found weapon definitions"), Assets.Num() > 0);

	for (const FAssetData& Asset : Assets)
	{
		const UWeaponDefinition* Definition = Cast<UWeaponDefinition>(Asset.GetAsset());
		if (!TestNotNull(FString::Printf(TEXT("%s loads"), *Asset.AssetName.ToString()), Definition))
		{
			continue;
		}
		for (int32 Tier = 0; Tier + 1 < NumTiers; ++Tier)
		{
			TestTrue(FString::Printf(TEXT("%s: %s (%.1f) more likely than %s (%.1f)"), *Asset.AssetName.ToString(), TierName(Tier),
				TierWeight(*Definition, Tier), TierName(Tier + 1), TierWeight(*Definition, Tier + 1)),
				TierWeight(*Definition, Tier) > TierWeight(*Definition, Tier + 1));
		}
		TestTrue(FString::Printf(TEXT("%s: Legendary possible"), *Asset.AssetName.ToString()), TierWeight(*Definition, NumTiers - 1) > 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootAmmoModelsTest, "Looter.Loot.AmmoModels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootAmmoModelsTest::RunTest(const FString& Parameters)
{
	// Every ammo type drops as its own bundle of rounds (SM_Ammo<Type>, from Art/Models/Loot/Ammo.py): its HUD icon
	// modeled in 3D, under a small white beam.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	TSet<const UStaticMesh*> Seen;
	for (const EAmmoType Type : LooterAmmo::AllTypes())
	{
		const FString Name = StaticEnum<EAmmoType>()->GetNameStringByValue(static_cast<int64>(Type));
		const AAmmoPickup* Pickup = AAmmoPickup::SpawnAmmo(WorldWrapper.GetTestWorld(), Type, 10, FVector::ZeroVector);
		const UStaticMeshComponent* Model = Pickup ? Pickup->GetModel() : nullptr;
		const UStaticMesh* Mesh = Model ? Model->GetStaticMesh() : nullptr;
		if (TestNotNull(FString::Printf(TEXT("%s model"), *Name), Mesh))
		{
			TestEqual(FString::Printf(TEXT("%s bundle"), *Name), Mesh->GetName(), FString::Printf(TEXT("SM_Ammo%s"), *Name));
			Seen.Add(Mesh);
		}

		// The beam: a glowing mesh on the root (so it doesn't spin with the bundle), standing above the bundle, small
		// next to a gun's (the shortest is 350 cm tall), never in the way of anything.
		const UStaticMeshComponent* Beam = Pickup ? Pickup->GetBeam() : nullptr;
		if (TestNotNull(FString::Printf(TEXT("%s beam"), *Name), Beam) && Model)
		{
			TestNotNull(FString::Printf(TEXT("%s beam has its mesh"), *Name), Beam->GetStaticMesh().Get());
			TestTrue(FString::Printf(TEXT("%s beam hangs off the root, not the spinning bundle"), *Name),
				Beam->GetAttachParent() == Pickup->GetRootComponent());
			TestTrue(FString::Printf(TEXT("%s beam is visible"), *Name), Beam->IsVisible());
			TestTrue(FString::Printf(TEXT("%s beam has no collision"), *Name), Beam->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			TestFalse(FString::Printf(TEXT("%s beam casts no shadow"), *Name), Beam->CastShadow != 0);
			const FBox BeamBox = Beam->Bounds.GetBox();
			const FBox BundleBox = Model->Bounds.GetBox();
			const double BeamHeight = BeamBox.Max.Z - BeamBox.Min.Z;
			TestTrue(FString::Printf(TEXT("%s beam is short next to a gun's (%.0f cm)"), *Name, BeamHeight), BeamHeight > 100.0 && BeamHeight < 350.0);
			TestTrue(FString::Printf(TEXT("%s beam starts above the bundle (%.0f over %.0f)"), *Name, BeamBox.Min.Z, BundleBox.Max.Z),
				BeamBox.Min.Z >= BundleBox.Max.Z - 2.0);
		}
	}
	TestEqual(TEXT("A bundle per ammo type"), Seen.Num(), LooterAmmo::NumTypes);
	return true;
}

#endif
