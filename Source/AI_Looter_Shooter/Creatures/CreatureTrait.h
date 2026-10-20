#pragma once

#include "CoreMinimal.h"
#include "CreatureTrait.generated.h"

/**
 * What a high-tier creature carries on top of its tougher body (the enemy tiers, 2026-10-20): Gravebound one, Hallowed
 * two, Ethereal three with Phasing always one of them, each rolled from the traits that fit its kind
 * (CreatureTraitRules::Fits, UCreatureTraitComponent::RollTraits). UCreatureTraitComponent plays them, every number in
 * UCreatureTraitSettings (Project Settings > Game > Creature Traits). Nothing saves a trait: a load rolls them again from
 * the creature's tier seed, so new ones only ever go at the end.
 */
UENUM(BlueprintType)
enum class ECreatureTrait : uint8
{
	/** Moves 25% faster and winds its attacks up 15% quicker: pale wind streaks trail it, a glint flashes as it winds up. */
	Fleet,
	/** 3 s unhit, it heals 3% of its health a second, green motes rising off it; any hit breaks it off. */
	Mending,
	/** A soul ward worth 30% of its health takes damage first (a turning cage of cyan light); crits break it twice as fast; it shatters with a pop and leaves it dazed. */
	Warded,
	/** Dead, it swells for 1 s, its 3 m ring pulsing on the ground, then bursts: step out of the ring. */
	GraveBurst,
	/** Creatures within 8 m take 15% less damage and move 10% faster while it lives: its ring and its tethers show who. Kill it first. */
	Rallying,
	/** Below 30% health it enrages for good: 25% more damage and speed, a red flare, embers rising off it. */
	Vengeful,
	/** Hit hard, it blinks 4-6 m aside (every 4 s at most), a shimmer where it left and where it comes back. The Ethereal's own. */
	Phasing
};

namespace CreatureTraits
{
	inline constexpr int32 NumTraits = static_cast<int32>(ECreatureTrait::Phasing) + 1;

	/** Every trait, in the enum's order. */
	inline constexpr ECreatureTrait All[] = { ECreatureTrait::Fleet, ECreatureTrait::Mending, ECreatureTrait::Warded,
		ECreatureTrait::GraveBurst, ECreatureTrait::Rallying, ECreatureTrait::Vengeful, ECreatureTrait::Phasing };
	static_assert(UE_ARRAY_COUNT(All) == NumTraits, "Every trait, once");
}
