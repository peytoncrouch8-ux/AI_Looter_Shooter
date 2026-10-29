#pragma once

#include "CoreMinimal.h"
#include "LooterElement.generated.h"

/**
 * The story's elements a gun's part can carry (Phase 12.3, the user's pick): each hit of such a gun has a chance to set its
 * status on the creature it struck (UElementStatusComponent), and every status deals damage, more to the monsters weakest
 * to it (ElementRules' weakness table).
 *  - Ember: the coal that burns in the Unpaid, turned on the living. A burn: damage over time, stacking.
 *  - Soul-light: what the saints are made of. A smite: a delayed burst of light that leaps to one more of the dead.
 *  - Grave-salt: Delia's salt line, in a round. Salted: damage over time and a slow; wards crumble under it.
 * Saved with nothing (a gun's element comes from its parts, which it saves by key), so the numbers below may be appended to
 * but never reordered.
 */
UENUM(BlueprintType)
enum class ELooterElement : uint8
{
	None,
	Ember,
	SoulLight,
	GraveSalt,
	/** A jolt that staggers and arcs to the next living thing (the user's pick, 2026-10-29; Docs/Archive/Work/2026-10-29-NewElements). */
	Thunder,
	/** A rot that grows and spreads from the dying (the user's pick, 2026-10-29). */
	Rot
};
