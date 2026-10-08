#include "UI/Bench/BenchRules.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponPartSwap.h"
#include "Weapons/WeaponParts.h"

TArray<FCarriedGun> BenchRules::CarriedGuns(const UWeaponManagerComponent& Manager)
{
	TArray<FCarriedGun> Guns;
	const TArray<AWeaponBase*> Weapons = Manager.GetWeapons();
	for (int32 Slot = 0; Slot < Weapons.Num(); ++Slot)
	{
		if (IsValid(Weapons[Slot]))
		{
			Guns.Add(FCarriedGun::Equipped(Slot));
		}
	}
	const TArray<FWeaponInstanceData>& Backpack = Manager.GetBackpack();
	for (int32 Index = 0; Index < Backpack.Num(); ++Index)
	{
		if (Backpack[Index].Definition)
		{
			Guns.Add(FCarriedGun::InBackpack(Index));
		}
	}
	return Guns;
}

TArray<BenchRules::FPartEntry> BenchRules::PartsForSlot(TConstArrayView<FBoxedWeaponPart> Box, const FWeaponInstanceData& Gun, FName SlotName)
{
	TArray<FPartEntry> Fitting;
	TArray<FPartEntry> OwnKind;
	TArray<FPartEntry> OtherKinds;
	for (int32 Index = 0; Index < Box.Num(); ++Index)
	{
		const FBoxedWeaponPart& Part = Box[Index];
		if (Part.Slot != SlotName)
		{
			continue;
		}
		FPartEntry Entry;
		Entry.BoxIndex = Index;
		Entry.Check = WeaponPartSwap::CanFit(Gun, Part);
		(Entry.Fits() ? Fitting : (Part.Definition == Gun.Definition ? OwnKind : OtherKinds)).Add(Entry);
	}
	Fitting.Append(OwnKind);
	Fitting.Append(OtherKinds);
	return Fitting;
}

namespace
{
	/** A whole number with its sign: "+12", "-8". */
	FString Signed(int32 Value)
	{
		return (Value > 0 ? TEXT("+") : TEXT("")) + FString::FromInt(Value);
	}
}

TArray<BenchRules::FStatChange> BenchRules::StatChanges(const FWeaponStats& Before, const FWeaponStats& After)
{
	TArray<FStatChange> Changes;
	// The parts change stats by percentages, so their changes read as percentages too.
	auto Percent = [&Changes](const TCHAR* Name, float From, float To, bool bHigherIsBetter)
	{
		if (From <= UE_KINDA_SMALL_NUMBER || To <= 0.f)
		{
			return;
		}
		const int32 Rounded = FMath::RoundToInt((To / From - 1.f) * 100.f);
		if (Rounded == 0)
		{
			return;
		}
		FStatChange Change;
		Change.Text = FString::Printf(TEXT("%s%% %s"), *Signed(Rounded), Name);
		Change.bBetter = (Rounded > 0) == bHigherIsBetter;
		Change.Size = static_cast<float>(FMath::Abs(Rounded));
		Changes.Add(MoveTemp(Change));
	};
	// Damage is the whole shot's, so a shotgun's pellets count.
	Percent(TEXT("Dmg"), Before.Damage * Before.PelletsPerShot, After.Damage * After.PelletsPerShot, true);
	Percent(TEXT("Rate"), Before.FireRate, After.FireRate, true);
	// Accuracy is the spread's inverse: a tighter spread is more accurate.
	if (Before.Spread > UE_KINDA_SMALL_NUMBER && After.Spread > UE_KINDA_SMALL_NUMBER)
	{
		Percent(TEXT("Acc"), 1.f / Before.Spread, 1.f / After.Spread, true);
	}
	Percent(TEXT("Range"), Before.Range, After.Range, true);
	Percent(TEXT("Reload"), Before.ReloadTime, After.ReloadTime, false);
	Percent(TEXT("Recoil"), Before.Recoil, After.Recoil, false);
	Percent(TEXT("Handling"), Before.Handling, After.Handling, true);

	// Magazines and sights set their numbers outright: the rounds and the magnification themselves.
	const int32 Rounds = After.MagazineSize - Before.MagazineSize;
	if (Rounds != 0)
	{
		FStatChange Change;
		Change.Text = FString::Printf(TEXT("%s Mag"), *Signed(Rounds));
		Change.bBetter = Rounds > 0;
		Change.Size = 100.f * FMath::Abs(Rounds) / FMath::Max(Before.MagazineSize, 1);
		Changes.Add(MoveTemp(Change));
	}
	const float Zoom = After.Zoom - Before.Zoom;
	if (FMath::Abs(Zoom) >= 0.05f)
	{
		FStatChange Change;
		Change.Text = FString::Printf(TEXT("%s%.1fx Zoom"), Zoom > 0.f ? TEXT("+") : TEXT("-"), FMath::Abs(Zoom));
		Change.bBetter = Zoom > 0.f;
		Change.Size = 100.f * FMath::Abs(Zoom) / FMath::Max(Before.Zoom, 1.f);
		Changes.Add(MoveTemp(Change));
	}
	Changes.StableSort([](const FStatChange& A, const FStatChange& B) { return A.Size > B.Size; });
	return Changes;
}

const FWeaponPartOption* BenchRules::CurrentPart(const FWeaponInstanceData& Gun, int32 SlotIndex)
{
	if (!Gun.Definition || !Gun.Definition->Parts.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}
	// Picked as the gun is built: its saved keys, or its seed's parts for a gun saved before parts were kept.
	const FWeaponLook Look = WeaponParts::Pick(Gun);
	return Look.Parts.IsValidIndex(SlotIndex) ? Look.Parts[SlotIndex] : nullptr;
}

FString BenchRules::SlotLabel(FName Slot)
{
	return Slot.ToString();
}

FString BenchRules::PartName(const FBoxedWeaponPart& Part)
{
	const FWeaponPartOption* Option = WeaponPartSwap::FindOption(Part);
	return Option && !Option->DisplayName.IsEmpty() ? Option->DisplayName.ToString() : Part.Key.ToString();
}

FString BenchRules::SlotPartName(const FWeaponInstanceData& Gun, int32 SlotIndex)
{
	const FWeaponPartOption* Option = CurrentPart(Gun, SlotIndex);
	if (!Option)
	{
		return TEXT("Empty");
	}
	return Option->DisplayName.IsEmpty() ? Option->Key.ToString() : Option->DisplayName.ToString();
}

TArray<FName> BenchRules::SlotNames(const FWeaponInstanceData& Gun)
{
	TArray<FName> Names;
	if (Gun.Definition)
	{
		for (const FWeaponPartSlot& Slot : Gun.Definition->Parts)
		{
			Names.Add(Slot.Name);
		}
	}
	return Names;
}
