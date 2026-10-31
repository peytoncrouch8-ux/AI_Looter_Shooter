#pragma once

#include "CoreMinimal.h"
#include "Inventory/BackpackOrder.h"
#include "UI/Inventory/LoadoutRules.h"

struct FShieldInstanceData;
struct FWeaponInstanceData;

/**
 * The loadout's backpack list with its wards (the user, 2026-10-31: guns and wards share the backpack, "Wards should go to
 * the backpack side of the loadout tab"): the guns and the spare wards in one list, in the order they went in or sorted.
 * A list entry is a gun's backpack index, or a ward's code (WardCode), so the gun paths that read the list keep working
 * and a ward's row is never mistaken for a gun's.
 */
namespace LoadoutBackpack
{
	/** A spare ward's entry in the list: below INDEX_NONE (-1), so it is never a gun's backpack index nor "none". */
	inline constexpr int32 WardCode(int32 SpareIndex) { return -2 - SpareIndex; }

	/** The spare ward a list entry names, or INDEX_NONE for a gun's (or none). */
	inline constexpr int32 WardOf(int32 Entry) { return Entry <= -2 ? -2 - Entry : INDEX_NONE; }

	/**
	 * The list in the sort's order (LoadoutRules::ESort): Match leads with the target's kind of gun (best first), then the
	 * rest, guns and wards, by rarity and level; Rarity and Level mix both by those; Newest is the order they went in,
	 * latest first. Ties keep the order they went in (Stored; anything carried that it misses follows it). Returns how many
	 * lead as the target's kind (Match with a target only).
	 */
	AI_LOOTER_SHOOTER_API int32 SortList(const TArray<FWeaponInstanceData>& Guns, const TArray<FShieldInstanceData>& Wards,
		TConstArrayView<FBackpackItem> Stored, const FWeaponInstanceData* Target, LoadoutRules::ESort Sort, TArray<int32>& OutOrder);

	/** How many of the list's entries are guns. */
	AI_LOOTER_SHOOTER_API int32 CountGuns(TConstArrayView<int32> Order);

	/** The first ward's row in the list, or INDEX_NONE. */
	AI_LOOTER_SHOOTER_API int32 FirstWardRow(TConstArrayView<int32> Order);
}
