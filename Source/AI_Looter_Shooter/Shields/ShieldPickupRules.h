#pragma once

#include "CoreMinimal.h"

class UPlayerShieldComponent;

/**
 * What the Interact key does with a ward lying as loot, as it does with a gun (the user, 2026-10-31: "Wards should have a
 * similar UI to the weapons"): the gun's tap goes to a free slot, else the backpack, else swaps with the gun in hand; a
 * ward's tap is worn when none is, else goes in the backpack (the shared count), else swaps with the worn one, which drops.
 * A hold always wears it. Apart from the pickup so the card's keys and the tests read the same rule.
 */
namespace ShieldPickupRules
{
	enum class ETap : uint8
	{
		/** No ward worn: a tap wears it (there's no hold then). */
		Wear,
		/** A ward worn and room in the backpack: a tap puts it there. */
		ToBackpack,
		/** The backpack full: a tap wears it and the worn one drops ("BACKPACK FULL" under the card). */
		Swap,
	};

	AI_LOOTER_SHOOTER_API ETap TapFor(bool bWardWorn, bool bBackpackRoom);

	/** For the holder's shield component (null: as if none were worn). */
	AI_LOOTER_SHOOTER_API ETap TapFor(const UPlayerShieldComponent* Shield);
}
