#include "UI/Inventory/LoadoutBackpackRules.h"
#include "Shields/ShieldTypes.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"

// The backpack list's order with wards in it. LoadoutRules::SortBackpack still sorts guns alone (the bench's lists); this
// is the loadout's, where a ward sits among the guns by its rarity and level as a gun of another kind would.

namespace
{
	/** -1: A first (its number is the bigger), 1: B first, 0: alike within Slack. */
	int32 BiggerFirst(float A, float B, float Slack)
	{
		if (A > B + Slack)
		{
			return -1;
		}
		return B > A + Slack ? 1 : 0;
	}

	/** What an entry is sorted by. */
	struct FSortKey
	{
		bool bGun = false;
		const FWeaponInstanceData* Gun = nullptr;
		int32 Rarity = 0;
		int32 Level = 0;
		/** Where it stands in the order it went in. */
		int32 Placed = 0;
	};
}

int32 LoadoutBackpack::SortList(const TArray<FWeaponInstanceData>& Guns, const TArray<FShieldInstanceData>& Wards,
	TConstArrayView<FBackpackItem> Stored, const FWeaponInstanceData* Target, LoadoutRules::ESort Sort, TArray<int32>& OutOrder)
{
	// The order they went in, then (a list out of step) anything carried it didn't name.
	OutOrder.Reset(Guns.Num() + Wards.Num());
	TBitArray<> GunSeen(false, Guns.Num());
	TBitArray<> WardSeen(false, Wards.Num());
	for (const FBackpackItem& Item : Stored)
	{
		if (Item.Kind == EBackpackItem::Gun && Guns.IsValidIndex(Item.Index) && !GunSeen[Item.Index])
		{
			GunSeen[Item.Index] = true;
			OutOrder.Add(Item.Index);
		}
		else if (Item.Kind == EBackpackItem::Ward && Wards.IsValidIndex(Item.Index) && !WardSeen[Item.Index])
		{
			WardSeen[Item.Index] = true;
			OutOrder.Add(WardCode(Item.Index));
		}
	}
	for (int32 Index = 0; Index < Guns.Num(); ++Index)
	{
		if (!GunSeen[Index])
		{
			OutOrder.Add(Index);
		}
	}
	for (int32 Index = 0; Index < Wards.Num(); ++Index)
	{
		if (!WardSeen[Index])
		{
			OutOrder.Add(WardCode(Index));
		}
	}

	TMap<int32, FSortKey> Keys;
	for (int32 Position = 0; Position < OutOrder.Num(); ++Position)
	{
		const int32 Entry = OutOrder[Position];
		FSortKey& Key = Keys.Add(Entry);
		Key.Placed = Position;
		const int32 Ward = WardOf(Entry);
		if (Ward == INDEX_NONE)
		{
			Key.bGun = true;
			Key.Gun = &Guns[Entry];
			Key.Rarity = static_cast<int32>(Key.Gun->Rarity);
			Key.Level = Key.Gun->Level;
		}
		else
		{
			Key.Rarity = static_cast<int32>(Wards[Ward].Rarity);
			Key.Level = Wards[Ward].Level;
		}
	}

	// "Best for slot" leads with the target's kind; without a target it lists by rarity, as nothing is being replaced.
	const UWeaponDefinition* Kind = Sort == LoadoutRules::ESort::Match && Target ? Target->Definition.Get() : nullptr;
	auto IsKind = [Kind](const FSortKey& Key) { return Kind && Key.bGun && Key.Gun->Definition == Kind; };
	OutOrder.StableSort([&Keys, Sort, &IsKind](int32 EntryA, int32 EntryB)
	{
		const FSortKey& A = Keys.FindChecked(EntryA);
		const FSortKey& B = Keys.FindChecked(EntryB);
		const int32 Rarity = BiggerFirst(static_cast<float>(A.Rarity), static_cast<float>(B.Rarity), 0.f);
		const int32 Level = BiggerFirst(static_cast<float>(A.Level), static_cast<float>(B.Level), 0.f);
		// Damage per second only between two guns: a ward has none to weigh.
		const int32 Damage = A.bGun && B.bGun ? BiggerFirst(LoadoutRules::DamagePerSecond(A.Gun->Stats), LoadoutRules::DamagePerSecond(B.Gun->Stats), 0.01f) : 0;
		int32 Order = 0;
		switch (Sort)
		{
		case LoadoutRules::ESort::Match:
		{
			const bool bKindA = IsKind(A);
			const bool bKindB = IsKind(B);
			if (bKindA != bKindB)
			{
				return bKindA;
			}
			Order = bKindA ? (Damage != 0 ? Damage : (Rarity != 0 ? Rarity : Level)) : (Rarity != 0 ? Rarity : (Level != 0 ? Level : Damage));
			break;
		}
		case LoadoutRules::ESort::Rarity:
			Order = Rarity != 0 ? Rarity : (Level != 0 ? Level : Damage);
			break;
		case LoadoutRules::ESort::Level:
			Order = Level != 0 ? Level : (Rarity != 0 ? Rarity : Damage);
			break;
		case LoadoutRules::ESort::Newest:
			// The last in is the latest.
			return A.Placed > B.Placed;
		default:
			break;
		}
		return Order != 0 ? Order < 0 : A.Placed < B.Placed;
	});

	int32 NumKind = 0;
	for (const int32 Entry : OutOrder)
	{
		NumKind += IsKind(Keys.FindChecked(Entry)) ? 1 : 0;
	}
	return NumKind;
}

int32 LoadoutBackpack::CountGuns(TConstArrayView<int32> Order)
{
	int32 Found = 0;
	for (const int32 Entry : Order)
	{
		Found += Entry >= 0 ? 1 : 0;
	}
	return Found;
}

int32 LoadoutBackpack::FirstWardRow(TConstArrayView<int32> Order)
{
	for (int32 Row = 0; Row < Order.Num(); ++Row)
	{
		if (WardOf(Order[Row]) != INDEX_NONE)
		{
			return Row;
		}
	}
	return INDEX_NONE;
}
