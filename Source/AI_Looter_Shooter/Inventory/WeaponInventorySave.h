#pragma once

#include "CoreMinimal.h"
#include "Inventory/BackpackOrder.h"
#include "Shields/ShieldTypes.h"
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

	/**
	 * How the backpack's guns and spare wards (Shields.Spares) interleave, in the order they went in (FBackpackOrder; the
	 * user, 2026-10-31: guns and wards share the backpack). A session saved before reads it empty: its wards then follow
	 * its guns.
	 */
	UPROPERTY()
	TArray<EBackpackItem> BackpackOrder;

	/** Rounds carried per ammo class, indexed by EAmmoType. */
	UPROPERTY()
	TArray<int32> Ammo;

	/**
	 * The gunsmith's bench's parts box: each part by its kind of gun, slot and key, oldest first. A session saved before the
	 * bench existed reads it empty.
	 */
	UPROPERTY()
	TArray<FBoxedWeaponPart> PartsBox;

	/**
	 * The wards (SHIELD, 2026-10-22: the one worn, the spares, whether the first gun's starting ward was given;
	 * UPlayerShieldComponent::SaveWards). A session saved before wards reads it empty, and gets the starting ward on load.
	 */
	UPROPERTY()
	FShieldInventorySave Shields;
};
