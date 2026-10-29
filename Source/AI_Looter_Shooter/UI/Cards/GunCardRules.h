#pragma once

#include "CoreMinimal.h"
#include "UI/Inventory/LoadoutRules.h"

struct FElementTuning;
struct FWeaponInstanceData;
struct FWeaponStats;

/** Which card: where it shows decides what it compares and whether it has keys. */
enum class EGunCardMode : uint8
{
	/** Over a gun on the ground (UWeaponLabelWidget, looked at): compared with a carried gun of its kind, with the take keys. */
	World,
	/** The gun in hand, on the screen's right while loot is looked at: compares nothing. */
	InHand,
	/** The inventory's card under the turning gun: compared with the gun of the slot it would go to. */
	Loadout
};

/** What the take keys do now (a world card's keys, under it). */
struct FGunCardKeys
{
	/** The Interact key's name ("E"). */
	FString Key = TEXT("E");
	/** Every weapon slot holds a gun: a tap stashes or swaps, a hold takes it in hand. */
	bool bSlotsFull = false;
	/** The backpack is full too: the tap swaps with the gun in hand, which drops. */
	bool bBackpackFull = false;
	/** The interact hold's progress (0-1): the HOLD cap fills with it. */
	float Hold = 0.f;
};

/**
 * The gun card's rules (the tag card, the user's pick 2026-10-28 evening; Docs/Work/2026-10-28-UICharacter/CONTRACT.md
 * "Round 2"), apart from the widget so the tests can check them: a stat list with values and arrows, bullet lines, a
 * flavour line, the maker's badge and the element; how its intro moves.
 */
namespace GunCardRules
{
	/** The stats a card lists, in order: damage, accuracy, handling, reload, fire rate, magazine, and zoom on a scoped gun. */
	AI_LOOTER_SHOOTER_API TArray<LoadoutRules::EStat> CardStats(const FWeaponStats& Stats);

	/** "Damage", "Accuracy", "Handling", "Reload", "Fire rate", "Magazine", "Zoom". */
	AI_LOOTER_SHOOTER_API FString StatLabel(LoadoutRules::EStat Stat);

	/** The value on its row: "34" or "21 × 8", "81%", "58%", "2.2 s", "540 rpm", "30", "6×". */
	AI_LOOTER_SHOOTER_API FString StatValueText(LoadoutRules::EStat Stat, const FWeaponStats& Stats);

	/**
	 * The card's bullet lines, in order: what its element does, its part effect, its maker's trait with its two biggest
	 * numbers, its curse's perk and drawback, its notches. Empty for a plain gun with no maker.
	 */
	AI_LOOTER_SHOOTER_API TArray<FString> Lines(const FWeaponInstanceData& Gun);

	/** One of the element's rows under the stats: its label and value. */
	struct FElementRow
	{
		FString Label;
		FString Value;
	};
	/**
	 * The element's rows at the stat list's foot, like Borderlands' "Burn Dmg/Sec" and "Chance to Ignite" (the user,
	 * 2026-10-29): Ember "Burn dmg/sec" and "Chance to burn", Soul-light "Smite damage" and "Chance to smite", Grave-salt
	 * "Salt dmg/sec" and "Chance to salt" (per bullet, before a weakness). A gun without one: one row, "Element" "Normal".
	 */
	AI_LOOTER_SHOOTER_API TArray<FElementRow> ElementRows(const FWeaponInstanceData& Gun, const FElementTuning& Tuning);

	/** "True Glass: accuracy +30%, range +20%": the maker's trait and its two biggest changes; empty without a maker. */
	AI_LOOTER_SHOOTER_API FString MakerTraitLine(const FWeaponInstanceData& Gun);

	/** The italic flavour line (a named gun's own words); empty for most guns. */
	AI_LOOTER_SHOOTER_API FString FlavorLine(const FWeaponInstanceData& Gun);

	/**
	 * What a world card compares with (the user's call, 2026-10-28: compare like with like): the gun in hand when it's the
	 * same kind, else the first carried gun of that kind (in the order given), else nothing.
	 */
	AI_LOOTER_SHOOTER_API const FWeaponInstanceData* PickCompare(const FWeaponInstanceData& Gun, const FWeaponInstanceData* InHand,
		TConstArrayView<const FWeaponInstanceData*> Carried);

	/** Same kind of gun (both have a definition of one kind). */
	AI_LOOTER_SHOOTER_API bool SameKind(const FWeaponInstanceData& A, const FWeaponInstanceData& B);

	/** "UPGRADE" for an upgrade, empty otherwise (not drawn on the tag card; the inventory reads it). */
	AI_LOOTER_SHOOTER_API FString VerdictWord(LoadoutRules::EVerdict Verdict);

	/** One key cap and its word under a world card. */
	struct FKeyHint
	{
		/** "E", or "HOLD E" for the hold; empty for a word alone ("BACKPACK FULL"). */
		FString Cap;
		/** "TAKE", "EQUIP", "SWAP", "TO BACKPACK". */
		FString Word;
		/** The hold: its cap fills as the hold runs. */
		bool bHold = false;
		/** Said dimmer (the warning "BACKPACK FULL"). */
		bool bDim = false;
	};
	/**
	 * The keys: free slot "[E] TAKE  [HOLD E] EQUIP"; slots full "[E] TO BACKPACK  [HOLD E] EQUIP"; backpack full too
	 * "[E] SWAP  BACKPACK FULL" (the gun in hand drops), the take rules of UWeaponManagerComponent.
	 */
	AI_LOOTER_SHOOTER_API TArray<FKeyHint> KeyHints(const FGunCardKeys& Keys);

	// --- The intro (seconds since it started) ---

	/** The unfold: 0.15 to 1.06 by 0.08 s, settling to 1 by UnfoldSeconds. */
	inline constexpr float UnfoldSeconds = 0.12f;
	AI_LOOTER_SHOOTER_API float UnfoldScale(float Age);
	AI_LOOTER_SHOOTER_API float UnfoldOpacity(float Age);

	/** The rows come in one after another: row Index's share (0-1, eased out), RowStagger after the last, RowSeconds each. */
	inline constexpr float RowDelay = 0.06f;
	inline constexpr float RowStagger = 0.035f;
	inline constexpr float RowSeconds = 0.14f;
	AI_LOOTER_SHOOTER_API float RowShare(float Age, int32 Index);

	/** The damage counting up: its share of the value (0-1, eased out) over CountSeconds. */
	inline constexpr float CountSeconds = 0.25f;
	AI_LOOTER_SHOOTER_API float CountShare(float Age);

	/** The gem's pop as the card lands (scale; 1 at rest). */
	AI_LOOTER_SHOOTER_API float GemPop(float Age);

	/** The whole intro has run (nothing moves after this). */
	inline constexpr float IntroSeconds = 0.55f;
}
