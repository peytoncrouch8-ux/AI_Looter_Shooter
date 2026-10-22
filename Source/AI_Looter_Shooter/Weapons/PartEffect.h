#pragma once

#include "CoreMinimal.h"
#include "PartEffect.generated.h"

/**
 * What a part makes a gun do, beyond its numbers (Phase 12.3: parts worth hunting). One a gun at most (its first part with
 * one, in slot order: PartEffectRules::Of). Epic parts carry them, a few Rare ones. The bullets' side is
 * UPartEffectSubsystem (through UBulletSubsystem's hooks), the trigger's WeaponTrigger::PlanFor. Parts save by key, never
 * by these numbers, but they're appended to only all the same.
 */
UENUM(BlueprintType)
enum class EPartEffect : uint8
{
	None,
	/** A round that hits bounces once to the nearest creature within 8 m. */
	Ricochet,
	/** A pull fires a three-round burst. */
	Burst,
	/** A round bursts where it lands: most of it to what it hit, the rest splashed round within 1.5 m. */
	Explosive,
	/** A round punches through one creature into the next. */
	Piercing,
	/** A pull winds up for 0.6 s and fires a double-damage round, at half the fire rate. */
	Overcharge,
	/** Two bullets a shot for one round. */
	Twin
};
