#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Find.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Math/RandomStream.h"
#include "Weapons/AmmoTypes.h"

// Ammo pickups by class (the user, 2026-10-29 night): a kill's AR 18-36, SMG 24-48, pistol 12-24, shotgun 4-8, sniper 3-6,
// rockets one to three as before; a chest's 1.5 boxes of the class; the kill gun's class still leans x2.

namespace
{
	struct FClassRange
	{
		EAmmoType Type;
		const TCHAR* Name;
		int32 KillLeast;
		int32 KillMost;
		int32 Chest;
	};

	const FClassRange Ranges[] = {
		{ EAmmoType::AssaultRifle, TEXT("AR"), 18, 36, 54 },
		{ EAmmoType::SMG, TEXT("SMG"), 24, 48, 72 },
		{ EAmmoType::Pistol, TEXT("Pistol"), 12, 24, 36 },
		{ EAmmoType::Shotgun, TEXT("Shotgun"), 4, 8, 12 },
		{ EAmmoType::Sniper, TEXT("Sniper"), 3, 6, 9 },
		{ EAmmoType::Rocket, TEXT("Rockets"), 1, 3, 3 },
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootAmmoByClassTest, "Looter.Loot.AmmoByClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootAmmoByClassTest::RunTest(const FString& Parameters)
{
	// The user's numbers, class by class: a kill's least and most, a chest's.
	for (const FClassRange& Range : Ranges)
	{
		TestEqual(FString::Printf(TEXT("%s: a kill's least"), Range.Name), LooterLoot::AmmoAmountFor(Range.Type, LooterLoot::KillAmmoAmountMin), Range.KillLeast);
		TestEqual(FString::Printf(TEXT("%s: a kill's most"), Range.Name), LooterLoot::AmmoAmountFor(Range.Type, LooterLoot::KillAmmoAmountMax), Range.KillMost);
		TestEqual(FString::Printf(TEXT("%s: a chest's, 1.5 boxes"), Range.Name), LooterLoot::AmmoAmountFor(Range.Type, LooterLoot::ChestAmmoAmount), Range.Chest);
	}

	// A kill table's roll for a class: always in its range, every amount in it coming up (an even roll over the class's own
	// range, so a shotgun's 4 and 8 come up as often as the shells between).
	ULootTable* Table = NewObject<ULootTable>();
	FRandomStream Random(20261029);
	for (const FClassRange& Range : Ranges)
	{
		TMap<int32, int32> Seen;
		bool bInRange = true;
		constexpr int32 Rolls = 6000;
		for (int32 Roll = 0; Roll < Rolls; ++Roll)
		{
			const int32 Amount = ULootLibrary::RollAmmoAmountOf(Table, Range.Type, Random);
			bInRange &= Amount >= Range.KillLeast && Amount <= Range.KillMost;
			++Seen.FindOrAdd(Amount);
		}
		TestTrue(FString::Printf(TEXT("%s: every kill pickup %d to %d"), Range.Name, Range.KillLeast, Range.KillMost), bInRange);
		TestEqual(FString::Printf(TEXT("%s: every amount comes up"), Range.Name), Seen.Num(), Range.KillMost - Range.KillLeast + 1);
		if (Range.Type == EAmmoType::Shotgun)
		{
			// Five amounts, a fifth each give or take.
			for (const TPair<int32, int32>& Each : Seen)
			{
				const float Share = static_cast<float>(Each.Value) / Rolls;
				TestTrue(FString::Printf(TEXT("Shotgun: %d shells on %.3f of kills, about a fifth"), Each.Key, Share), FMath::Abs(Share - 0.2f) <= 0.025f);
			}
		}
	}

	// A chest's table: always its 1.5 boxes.
	ULootTable* Chest = NewObject<ULootTable>();
	Chest->UseChestAmmoAmount();
	for (const FClassRange& Range : Ranges)
	{
		TestEqual(FString::Printf(TEXT("%s: a chest's pickup"), Range.Name), ULootLibrary::RollAmmoAmountOf(Chest, Range.Type, Random), Range.Chest);
	}

	// The user's older rule kept: the kill gun's class leans x2 (five classes at 1 and rockets at 0.5: 2 of 6.5).
	Table->MinAmmoDrops = 1;
	Table->MaxAmmoDrops = 1;
	int32 Shells = 0;
	int32 Pickups = 0;
	for (int32 Kill = 0; Kill < 20000; ++Kill)
	{
		for (const FAmmoDrop& Drop : ULootLibrary::RollAmmo(Table, Random, EAmmoType::Shotgun))
		{
			++Pickups;
			Shells += Drop.Type == EAmmoType::Shotgun ? 1 : 0;
			const FClassRange* Range = Algo::FindByPredicate(Ranges, [&Drop](const FClassRange& Each) { return Each.Type == Drop.Type; });
			if (Range && (Drop.Amount < Range->KillLeast || Drop.Amount > Range->KillMost))
			{
				AddError(FString::Printf(TEXT("A %s pickup of %d, outside %d-%d"), Range->Name, Drop.Amount, Range->KillLeast, Range->KillMost));
			}
		}
	}
	const float ShellShare = static_cast<float>(Shells) / FMath::Max(Pickups, 1);
	TestTrue(FString::Printf(TEXT("A shotgun kill's pickups are shells %.3f of the time (2 of 6.5: 0.308)"), ShellShare),
		FMath::Abs(ShellShare - 2.f / 6.5f) <= 0.02f);
	return true;
}

#endif
