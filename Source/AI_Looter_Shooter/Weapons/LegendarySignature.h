#pragma once

#include "CoreMinimal.h"
#include "LegendarySignature.generated.h"

/**
 * A unique legendary's own trick (the user's call, 2026-10-29 night: each Ransom's Rest boss drops its own legendary, 10%
 * a kill, farmable at its vigil stone like Borderlands 2). A named gun carries one (UNamedWeaponDefinition::Signature);
 * the play is ULegendarySubsystem and ULegendaryEffectComponent, the numbers FLegendaryTuning (Looter.Legendary.*).
 * Saved assets name these by value: append only, never reorder.
 */
UENUM(BlueprintType)
enum class ELegendarySignature : uint8
{
	None,
	/** Gravesilk (the Gravemother's Rustler): hits lay silk; enough of it roots the target a moment (never a boss). */
	Gravesilk,
	/** Company Scrip (the Foreman's Thresher): kills in one burst speed it up; a reload pays some of the magazine back. */
	CompanyScrip,
	/** Lamplighter (Abel's Longhorn): a scoped kill leaves a lantern that heals the player near it. */
	Lamplighter
};
