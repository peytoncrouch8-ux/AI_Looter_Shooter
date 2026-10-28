#pragma once

#include "CoreMinimal.h"
#include "UI/Inventory/LoadoutRules.h"

struct FWeaponInstanceData;

/** Which card: where it shows decides what it compares and whether it has keys. */
enum class EGunCardMode : uint8
{
	/** Over a gun on the ground (UWeaponLabelWidget, looked at): compared with the gun in hand, with the take keys. */
	World,
	/** The gun in hand, on the screen's right while loot is looked at: compares nothing. */
	InHand,
	/** The inventory's card under the turning gun: compared with the gun of the slot it would go to. */
	Loadout
};

/** What the take keys do now (a world card's keys row; the HUD's old card's hint, as cards). */
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
 * The gun card's rules (Docs/Work/2026-10-28-UICharacter/CONTRACT.md), apart from the widget so the tests can check them:
 * what it shows (only what decides a pick: the damage, three bars, one special line, a verdict) and how its intro moves.
 */
namespace GunCardRules
{
	/** The three bars every card shows, in order: fire rate, magazine, reload (the rest wait behind the inventory's Inspect). */
	AI_LOOTER_SHOOTER_API TConstArrayView<LoadoutRules::EStat> CardStats();

	/** "FIRE RATE", "MAGAZINE", "RELOAD". */
	AI_LOOTER_SHOOTER_API FString StatLabel(LoadoutRules::EStat Stat);

	/** The one special line: the gun's element, else its part effect, else its curse, else its notches; empty for none. */
	struct FSpecialLine
	{
		/** "EMBER", "RICOCHET", "HUNGRY", "12 NOTCHES". */
		FString Word;
		/** What it does ("sets them alight"); may be empty (notches). */
		FString Words;
		/** The word's colour (the element's own, the accent for an effect, the curse's, a notch tier's). */
		FLinearColor Color = FLinearColor::White;
		bool IsSet() const { return !Word.IsEmpty(); }
	};
	AI_LOOTER_SHOOTER_API FSpecialLine SpecialLine(const FWeaponInstanceData& Gun);

	/** "UPGRADE" for an upgrade, empty otherwise (a card only ever boasts; Similar and Weaker show through the arrows). */
	AI_LOOTER_SHOOTER_API FString VerdictWord(LoadoutRules::EVerdict Verdict);

	/** "Level 6 · Pump shotgun". */
	AI_LOOTER_SHOOTER_API FString KindLine(const FWeaponInstanceData& Gun);

	/** The damage as the card's big number ("14") and its pellets ("×8"; empty for one). */
	AI_LOOTER_SHOOTER_API FString DamageNumber(const FWeaponInstanceData& Gun);
	AI_LOOTER_SHOOTER_API FString PelletText(const FWeaponInstanceData& Gun);

	/** One key cap and its word on a world card's keys row. */
	struct FKeyHint
	{
		/** "E", or "HOLD E" for the hold. */
		FString Cap;
		/** "TAKE", "EQUIP", "SWAP", "TO BACKPACK". */
		FString Word;
		/** The hold: its cap fills as the hold runs. */
		bool bHold = false;
		/** Said dimmer (the second choice, or the warning "BACKPACK FULL"). */
		bool bDim = false;
	};
	/**
	 * A world card's keys: free slot "[E] TAKE"; slots full "[E] TO BACKPACK  [HOLD E] EQUIP"; backpack full too
	 * "[E] SWAP  BACKPACK FULL" (the gun in hand drops), the take rules of UWeaponManagerComponent.
	 */
	AI_LOOTER_SHOOTER_API TArray<FKeyHint> KeyHints(const FGunCardKeys& Keys);

	// --- The intro (seconds since it started) ---

	/** The unfold: 0.15 to 1.06 by 0.08 s, settling to 1 by UnfoldSeconds. */
	inline constexpr float UnfoldSeconds = 0.12f;
	AI_LOOTER_SHOOTER_API float UnfoldScale(float Age);
	AI_LOOTER_SHOOTER_API float UnfoldOpacity(float Age);

	/** Bar Index's fill share (0-1, eased out): each starts BarStagger after the last and takes BarSeconds. */
	inline constexpr float BarDelay = 0.08f;
	inline constexpr float BarStagger = 0.06f;
	inline constexpr float BarSeconds = 0.16f;
	AI_LOOTER_SHOOTER_API float BarShare(float Age, int32 Index);

	/** The damage counting up: its share of the value (0-1, eased out) over CountSeconds. */
	inline constexpr float CountSeconds = 0.25f;
	AI_LOOTER_SHOOTER_API float CountShare(float Age);

	/** The shine across the sweep (Epic and Legendary): where it is, 0 to 1 along the band, between ShineStart and ShineEnd; <0 before, >1 after. */
	inline constexpr float ShineStart = 0.30f;
	inline constexpr float ShineEnd = 0.65f;
	AI_LOOTER_SHOOTER_API float ShinePosition(float Age);

	/** The whole intro has run (nothing moves after this). */
	inline constexpr float IntroSeconds = 0.70f;

	/** A gun this rare gets the shine. */
	AI_LOOTER_SHOOTER_API bool HasShine(const FWeaponInstanceData& Gun);
}
