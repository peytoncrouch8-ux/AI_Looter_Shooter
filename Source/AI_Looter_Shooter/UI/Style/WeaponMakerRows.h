#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

class UWidget;
class UWidgetTree;

/**
 * A gun's maker on its cards (the loot card, the loadout card): its mark, Inked and white, and its name in the kit's dim
 * text, so the gun's own name in its rarity's colour still leads; with the trade's numbers and the maker's line under it
 * where there's room. LooterUI kit, Concept C; text only (no backing panel of its own).
 */
namespace WeaponMakerRows
{
	/** "[mark] MERIDIAN · TRUE GLASS" on one line; null for a gun without a maker (a named gun). */
	AI_LOOTER_SHOOTER_API UWidget* MakeLine(UWidgetTree* Tree, const FWeaponInstanceData& Gun, int32 FontSize);

	/**
	 * The line, then its trade in numbers ("accuracy +30%, range +20%..."), and with bFiction the maker's full name, place
	 * and one-line story, dim and wrapping. Null without a maker.
	 */
	AI_LOOTER_SHOOTER_API UWidget* MakeRows(UWidgetTree* Tree, const FWeaponInstanceData& Gun, int32 FontSize, bool bFiction);
}
