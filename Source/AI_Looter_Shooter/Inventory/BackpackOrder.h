#pragma once

#include "CoreMinimal.h"
#include "BackpackOrder.generated.h"

/**
 * What a place in the backpack holds. Guns and spare wards share the backpack (the user, 2026-10-31: "they don't get a
 * separate storage amount than guns"). Saved by name: append only, never rename.
 */
UENUM()
enum class EBackpackItem : uint8
{
	Gun,
	Ward,
};

/**
 * One thing in the backpack: a gun (UWeaponManagerComponent::GetBackpack()[Index]) or a spare ward
 * (UPlayerShieldComponent::GetSpares()[Index]).
 */
struct FBackpackItem
{
	EBackpackItem Kind = EBackpackItem::Gun;
	int32 Index = INDEX_NONE;

	bool operator==(const FBackpackItem& Other) const { return Kind == Other.Kind && Index == Other.Index; }
};

/**
 * The order the backpack's guns and wards went in, which the loadout lists them by and the session saves. One entry per
 * thing carried, its kind only: the n-th Gun entry is the backpack's n-th gun, the n-th Ward entry the n-th spare ward. The
 * guns and the wards keep their own lists (the weapon manager's Backpack, the shield component's Spares); this only says
 * how the two interleave, so neither list had to change its type.
 */
struct AI_LOOTER_SHOOTER_API FBackpackOrder
{
	TArray<EBackpackItem> Items;

	/** A thing of Kind went in at the backpack's end. */
	void Add(EBackpackItem Kind);

	/** The Index-th thing of Kind left; returns where its entry stood (INDEX_NONE: there was none). */
	int32 RemoveAt(EBackpackItem Kind, int32 Index);

	/**
	 * A thing of Kind went in as the Index-th of its kind: at Position when that keeps the count right (a swap putting one
	 * back where another came out), else just before the one that was Index-th, else at the end.
	 */
	void Insert(EBackpackItem Kind, int32 Index, int32 Position = INDEX_NONE);

	/** How many things of Kind it lists. */
	int32 Count(EBackpackItem Kind) const;

	/** Where the Index-th thing of Kind's entry stands; INDEX_NONE without one. */
	int32 PositionOf(EBackpackItem Kind, int32 Index) const;

	/**
	 * Brought into step with what is carried: a missing gun's or ward's entry is added at the end (an old save's wards
	 * come after its guns), one too many is taken from the end. True when anything changed.
	 */
	bool Reconcile(int32 NumGuns, int32 NumWards);

	/** Every thing in order, each with its index in its own list. */
	TArray<FBackpackItem> Entries() const;
};
