#pragma once

#include "CoreMinimal.h"
#include "CreatureTrait.generated.h"

/**
 * STAND-IN (TIER-CORE, 2026-10-20): the contract's names only, so the tier core builds; the TRAITS agent's file replaces
 * this one whole. What each trait does: Saved/Tiers/DESIGN.md, "Traits" (Gravebound 1, Hallowed 2, Ethereal 3).
 */
UENUM(BlueprintType)
enum class ECreatureTrait : uint8
{
	Fleet,
	Mending,
	Warded,
	GraveBurst,
	Rallying,
	Vengeful,
	Phasing
};
