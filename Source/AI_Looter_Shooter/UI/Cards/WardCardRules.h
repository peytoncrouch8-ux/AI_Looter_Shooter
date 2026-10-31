#pragma once

#include "CoreMinimal.h"
#include "Shields/ShieldTypes.h"
#include "UI/Cards/GunCardRules.h"
#include "UI/Inventory/LoadoutRules.h"

/** Where a ward card shows: it decides whether its take keys float under it. */
enum class EWardCardMode : uint8
{
	/** Over a ward on the ground (UShieldLabelWidget, looked at): compared with the ward worn, with the take keys. */
	World,
	/** The inventory's card under the showcase: a backpack ward compared with the ward worn, the worn one with nothing. */
	Loadout
};

/** What the take keys do now (a world card's keys, under it). */
struct FWardCardKeys
{
	/** The Interact key's name ("E"). */
	FString Key = TEXT("E");
	/** A ward is worn: a tap puts this one in the backpack (or swaps), a hold wears it. */
	bool bWorn = false;
	/** The backpack (guns and wards together) is full: the tap swaps with the ward worn, which drops. */
	bool bBackpackFull = false;
	/** The interact hold's progress (0-1): the HOLD cap fills with it. */
	float Hold = 0.f;
};

/**
 * The ward card's rules (the user, 2026-10-31: "Wards should have a similar UI to the weapons", in the world and in the
 * loadout): the gun card's tag card (GunCardRules) with a ward's numbers. Apart from the widget so the tests check them.
 */
namespace WardCardRules
{
	/** The numbers on its stat rows, in order. */
	enum class EStat : uint8
	{
		Capacity,
		Delay,
		Rate
	};

	/** Capacity, recharge delay, recharge rate. */
	AI_LOOTER_SHOOTER_API TConstArrayView<EStat> CardStats();

	/** "Capacity", "Recharge delay", "Recharge rate". */
	AI_LOOTER_SHOOTER_API FString StatLabel(EStat Stat);

	/** "52", "2.9 s", "15 / s" (ShieldText's numbers). */
	AI_LOOTER_SHOOTER_API FString StatValueText(EStat Stat, const FShieldStats& Stats);

	/**
	 * Against the ward worn: more capacity, a shorter delay and a faster rate are better. Within 3% (a whole point of
	 * capacity, a tenth of a second) is the same, so a ward's rounding never shows an arrow.
	 */
	AI_LOOTER_SHOOTER_API LoadoutRules::EChange CompareStat(EStat Stat, const FShieldStats& New, const FShieldStats& Old);

	/** "▲", "▼" or empty: a row's arrow. */
	AI_LOOTER_SHOOTER_API FString ArrowText(LoadoutRules::EChange Change);

	/** One of the rows under the stats (a gun card's element rows): its label and value. */
	struct FExtraRow
	{
		FString Label;
		FString Value;
		/** The trait's row: its value in the trait's colour. */
		bool bTrait = false;
	};
	/** "Build: Bulwark", and "Trait: Harvest" on a ward with one. */
	AI_LOOTER_SHOOTER_API TArray<FExtraRow> ExtraRows(const FShieldInstanceData& Ward);

	/**
	 * The bullet lines: what its build does to its numbers ("Bulwark: holds 20% more, waits 10% longer, refills 20% slower";
	 * none for a plain Ward Box), then its trait's effect with this ward's numbers (ShieldText::TraitLine). Trait first.
	 */
	AI_LOOTER_SHOOTER_API TArray<FString> Lines(const FShieldInstanceData& Ward);

	/** The build's line alone (empty for Standard). */
	AI_LOOTER_SHOOTER_API FString BuildLine(EShieldBuild Build);

	/** "UPGRADE" when it beats the ward worn (ShieldRules::Compare), empty otherwise (the inventory reads it). */
	AI_LOOTER_SHOOTER_API FString VerdictWord(const FShieldInstanceData& Ward, const FShieldInstanceData* Worn);

	/** "RARE WARD": the line over the name, the rarity's word (as a gun without a maker's card says its rarity). */
	AI_LOOTER_SHOOTER_API FString TopWord(const FShieldInstanceData& Ward);

	/**
	 * The keys, as a gun's (GunCardRules::KeyHints) with ShieldPickupRules' taps: none worn "[E] WEAR"; worn "[E] TO
	 * BACKPACK  [HOLD E] WEAR"; the backpack full "[E] SWAP  BACKPACK FULL" (the worn one drops).
	 */
	AI_LOOTER_SHOOTER_API TArray<GunCardRules::FKeyHint> KeyHints(const FWardCardKeys& Keys);
}
