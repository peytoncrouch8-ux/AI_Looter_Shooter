#pragma once

#include "CoreMinimal.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponPartSwap.h"
#include "Weapons/WeaponTypes.h"

struct FWeaponPartOption;

/**
 * What the bench's screen lists and how it words it, apart from the widgets (so tests can check it): the guns to choose
 * from, the parts box's parts for a slot, and what fitting one changes.
 */
namespace BenchRules
{
	/** Every gun the player carries, as the bench lists them: the equip slots in order, then the backpack in order. */
	AI_LOOTER_SHOOTER_API TArray<FCarriedGun> CarriedGuns(const UWeaponManagerComponent& Manager);

	/** A box part listed for a slot: where it is in the box, and whether it fits the gun. */
	struct FPartEntry
	{
		int32 BoxIndex = INDEX_NONE;
		WeaponPartSwap::ECheck Check = WeaponPartSwap::ECheck::Ok;

		bool Fits() const { return Check == WeaponPartSwap::ECheck::Ok; }
	};

	/**
	 * The box's parts for Gun's slot SlotName: the ones that fit first, then its own kind's that don't, then other kinds'
	 * for that slot (each group in box order, oldest first).
	 */
	AI_LOOTER_SHOOTER_API TArray<FPartEntry> PartsForSlot(TConstArrayView<FBoxedWeaponPart> Box, const FWeaponInstanceData& Gun, FName SlotName);

	/** One stat's change in a few characters ("+12% Dmg"), whether it's for the better, and how big it is (percent). */
	struct FStatChange
	{
		FString Text;
		bool bBetter = true;
		float Size = 0.f;
	};

	/** What a gun's stats changing from Before to After amounts to, the biggest changes first; nothing that rounds to 0. */
	AI_LOOTER_SHOOTER_API TArray<FStatChange> StatChanges(const FWeaponStats& Before, const FWeaponStats& After);

	/** The part in a gun's slot (its definition's slot index) as it stands, or null (an empty slot, no such slot). */
	AI_LOOTER_SHOOTER_API const FWeaponPartOption* CurrentPart(const FWeaponInstanceData& Gun, int32 SlotIndex);

	/** A slot's name as the screen shows it ("Barrel"). */
	AI_LOOTER_SHOOTER_API FString SlotLabel(FName Slot);

	/** A boxed part's name ("Marksman barrel"); its key when its option can't be found. */
	AI_LOOTER_SHOOTER_API FString PartName(const FBoxedWeaponPart& Part);

	/** A part's name in a gun's slot ("Marksman barrel"), or "Empty". */
	AI_LOOTER_SHOOTER_API FString SlotPartName(const FWeaponInstanceData& Gun, int32 SlotIndex);

	/** The gun's slots' names, in its definition's order (none without a definition). */
	AI_LOOTER_SHOOTER_API TArray<FName> SlotNames(const FWeaponInstanceData& Gun);
}
