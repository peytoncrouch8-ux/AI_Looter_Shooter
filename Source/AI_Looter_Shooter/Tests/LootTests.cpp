#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"

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
	int32 Boxes = 0;
	int32 Tiers[NumTiers] = {};
	int32 AmmoTypes[LooterAmmo::NumTypes] = {};
	bool bBoxesFull = true;
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
			++AmmoTypes[static_cast<int32>(Drop.Type)];
			++Boxes;
			bBoxesFull &= Drop.Amount == LooterAmmo::GetInfo(Drop.Type).BoxAmount;
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

	// Ammo: every kill, every class, each box a full box of its class.
	TestEqual(TEXT("Every kill drops ammo"), KillsWithoutAmmo, 0);
	TestTrue(TEXT("Each box holds its class's box amount"), bBoxesFull);
	for (const EAmmoType Type : LooterAmmo::AllTypes())
	{
		const float Share = static_cast<float>(AmmoTypes[static_cast<int32>(Type)]) / FMath::Max(Boxes, 1);
		TestNearlyEqual(FString::Printf(TEXT("%s share of ammo boxes"), LooterAmmo::GetInfo(Type).Name), Share, 1.f / LooterAmmo::NumTypes, 0.02f);
	}

	AddInfo(FString::Printf(TEXT("%d kills: weapons on %.1f%% (C/U/R/E/L %d/%d/%d/%d/%d), %.2f ammo boxes per kill"), Kills, WeaponRate * 100.f,
		Tiers[0], Tiers[1], Tiers[2], Tiers[3], Tiers[4], static_cast<float>(Boxes) / Kills));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootAmmoPoolTest, "Looter.Loot.AmmoPool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootAmmoPoolTest::RunTest(const FString& Parameters)
{
	UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>();
	const EAmmoType Type = EAmmoType::Shotgun;
	const int32 Box = LooterAmmo::GetInfo(Type).BoxAmount;
	const int32 Max = Inventory->GetMaxAmmo(Type);

	TestEqual(TEXT("Starts empty"), Inventory->GetAmmo(Type), 0);
	TestEqual(TEXT("A box goes in whole"), Inventory->AddAmmo(Type, Box), Box);
	TestEqual(TEXT("Only fills up to the carry limit"), Inventory->AddAmmo(Type, Max), Max - Box);
	TestEqual(TEXT("Full"), Inventory->GetAmmo(Type), Max);
	TestEqual(TEXT("Nothing fits when full"), Inventory->AddAmmo(Type, Box), 0);
	TestEqual(TEXT("Other classes have their own pool"), Inventory->GetAmmo(EAmmoType::AssaultRifle), 0);

	TestEqual(TEXT("Takes what a reload asks for"), Inventory->TakeAmmo(Type, 5), 5);
	TestEqual(TEXT("Never more than is carried"), Inventory->TakeAmmo(Type, Max * 2), Max - 5);
	TestEqual(TEXT("Empty again"), Inventory->GetAmmo(Type), 0);
	TestEqual(TEXT("Nothing from an empty pool"), Inventory->TakeAmmo(Type, 1), 0);

	TestEqual(TEXT("Negative amounts are ignored"), Inventory->AddAmmo(Type, -10), 0);
	TestEqual(TEXT("Invalid ammo class is ignored"), Inventory->AddAmmo(EAmmoType::Count, Box), 0);
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

#endif
