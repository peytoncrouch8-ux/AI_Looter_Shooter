#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponPartSwap.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponInventorySave.generated.h"

/** What the player carries, as a session saves it (UWeaponManagerComponent::SaveInventory and RestoreInventory). */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FWeaponInventorySave
{
	GENERATED_BODY()

	/** The guns in the equip slots, in order, each with the rounds left in its magazine. */
	UPROPERTY()
	TArray<FWeaponInstanceData> Equipped;

	/** Which of them is in hand (INDEX_NONE: none). */
	UPROPERTY()
	int32 ActiveSlot = INDEX_NONE;

	UPROPERTY()
	TArray<FWeaponInstanceData> Backpack;

	/** Rounds carried per ammo class, indexed by EAmmoType. */
	UPROPERTY()
	TArray<int32> Ammo;

	/**
	 * The gunsmith's bench's parts box: each part by its kind of gun, slot and key, oldest first. A session saved before the
	 * bench existed reads it empty.
	 */
	UPROPERTY()
	TArray<FBoxedWeaponPart> PartsBox;
};
