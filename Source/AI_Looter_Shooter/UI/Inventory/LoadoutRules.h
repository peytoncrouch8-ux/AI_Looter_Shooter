#pragma once

#include "CoreMinimal.h"

class UWeaponDefinition;
struct FWeaponInstanceData;

/** Rules behind the loadout screen's backpack list, kept apart from the widget so they can be tested. */
namespace LoadoutRules
{
	enum class EVerdict : uint8
	{
		/** Not compared (a different kind of gun, or nothing to compare with). */
		None,
		Upgrade,
		Similar,
		Weaker
	};

	/** How a backpack gun compares with the gun in a slot. Only guns of the same kind are compared, on damage per second. */
	AI_LOOTER_SHOOTER_API EVerdict Compare(const FWeaponInstanceData& Candidate, const FWeaponInstanceData* Current);

	/**
	 * The backpack in the order the swap list shows it: guns of the given kind first, then the rest, each in backpack
	 * order. Returns how many are of that kind (none when Kind is null).
	 */
	AI_LOOTER_SHOOTER_API int32 SortForSwap(const TArray<FWeaponInstanceData>& Backpack, const UWeaponDefinition* Kind, TArray<int32>& OutOrder);
}
