#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureTrait.h"
#include "Elements/LooterElement.h"

class ACreatureBase;
struct FGraveBurstTraitTuning;
struct FPhasingTraitTuning;

/** The kinds of creature the traits tell apart (CreatureTraitRules::KindOf; by class, most specific first). */
enum class ECreatureTraitKind : uint8
{
	/** The brown spider: a melee chaser in packs. */
	Spider,
	/** The meadow slime: a hopper, its leap its bite. */
	Slime,
	/** The Venom Spitter: ranged, keeps its distance. */
	Spitter,
	/** The Unpaid: floating melee ghosts (their own phase-step besides). */
	Unpaid,
	/** Providence's dead miners: slow, heavy pick swings. */
	DeadMiner,
	/** The Bell-Ringer: support, keeps behind its pack and raises the fallen. */
	BellRinger,
	/** Any other (a kind added later): every trait but those its kind would need to be told about. */
	Other
};

/**
 * The traits' rules as plain functions, so the tests check them without a level (UCreatureTraitComponent plays them):
 * which kinds a trait fits, the roll, the ward's sums, mending, the burst's falloff, where a blink may land.
 */
namespace CreatureTraitRules
{
	/** Creature's kind, by class (most specific first: a Bell-Ringer and a dead miner are built on the Unpaid, the Spitter on the spider). */
	AI_LOOTER_SHOOTER_API ECreatureTraitKind KindOf(const ACreatureBase& Creature);

	/**
	 * Whether Trait fits Kind (the reasons are the kinds' and the traits' docs):
	 *  - Fleet: not the Spitter (it keeps away: a faster kiter with a quicker spit is only harder to reach, never more to
	 *    read) nor the Bell-Ringer (it keeps behind its pack: speed only makes "kill it first" harder).
	 *  - Grave-burst: not the Spitter nor the Bell-Ringer: they die at range, so the burst would never be a threat to step from.
	 *  - Vengeful: not the Bell-Ringer (a 6-damage swipe; enraged it would only back off faster).
	 *  - Mending, Warded, Rallying, Phasing: every kind (Phasing is the Ethereal's own mark: the soul blinks, whatever the
	 *    body; a slime phases too).
	 */
	AI_LOOTER_SHOOTER_API bool Fits(ECreatureTrait Trait, ECreatureTraitKind Kind);

	/**
	 * Count traits for a creature of Kind from Seed (the same seed always gives the same traits, in the same order): an
	 * Ethereal's first is always Phasing, the rest drawn without repeats from what fits its kind; nobody else ever phases.
	 * Fewer when its kind has fewer that fit.
	 */
	AI_LOOTER_SHOOTER_API TArray<ECreatureTrait> Roll(int32 Count, bool bEthereal, ECreatureTraitKind Kind, int32 Seed);

	/** The trait's word, for the tag's trait line ("Grave-burst"). */
	AI_LOOTER_SHOOTER_API FText Word(ECreatureTrait Trait);

	/** The trait's name in code ("GraveBurst"), for logs and commands. */
	AI_LOOTER_SHOOTER_API FString Name(ECreatureTrait Trait);

	/** A trait from its code name or its word, any case ("graveburst", "Grave-burst"); false when it's neither. */
	AI_LOOTER_SHOOTER_API bool Parse(const FString& Text, ECreatureTrait& OutTrait);

	/**
	 * A hit of Damage on a ward holding InOutWard: the ward takes it first, counting ElementScale times over against it (its
	 * type: WardElementScale), then a critical one CritBreakScale times over on top; what it couldn't hold is returned for
	 * health. A hit that breaks it spills the rest of itself over, as it was (no ward's scale on health).
	 */
	AI_LOOTER_SHOOTER_API float AbsorbWard(float& InOutWard, float Damage, bool bCritical, float CritBreakScale, float ElementScale = 1.f);

	/**
	 * A typed ward's say on a hit of HitElement (the user, 2026-10-29 night): Match when it's the ward's own element,
	 * WardbreakAny for Wardbreak on any other ward, else 1; the larger, never the two multiplied.
	 */
	AI_LOOTER_SHOOTER_API float WardElementScale(ELooterElement Ward, ELooterElement HitElement, float Match, float WardbreakAny);

	/** A ward's element from Seed: Thunder 0.5 (the common shock ward), Wardbreak 0.2 (a true ward), Cinder, Hallow, Rot 0.1 each. */
	AI_LOOTER_SHOOTER_API ELooterElement RollWardElement(int32 Seed);

	/** RollWardElement's pick from a roll in 0 to 1 (the tests read the weights). */
	AI_LOOTER_SHOOTER_API ELooterElement WardElementAt(float Roll);

	/** Health mended over DeltaSeconds: HealSharePerSecond of MaxHealth a second once SinceHit reaches UnhitSeconds, else none. */
	AI_LOOTER_SHOOTER_API float MendAmount(float SinceHit, float UnhitSeconds, float HealSharePerSecond, float MaxHealth, float DeltaSeconds);

	/** The burst's share of its damage Distance from its heart (to a body's nearest side): 1 within FullRadius, straight to EdgeShare at Radius, 0 past it. */
	AI_LOOTER_SHOOTER_API float BurstShare(float Distance, const FGraveBurstTraitTuning& Tuning);

	/** Taken Damage within the window is a hard hit: at least HardHitShare of MaxHealth. */
	AI_LOOTER_SHOOTER_API bool IsHardHit(float RecentDamage, float MaxHealth, float HardHitShare);

	/**
	 * Where a blink from Feet may land, in order: aside of Away (the way from whoever hit it to it, flat), the first side
	 * first, square to it, then a little away and a little toward (65 and 115 degrees off Away); each at the longest
	 * distance first, then the middle, then the shortest; then the other side the same way. Flat, at Feet's height.
	 */
	AI_LOOTER_SHOOTER_API TArray<FVector> BlinkCandidates(const FVector& Feet, const FVector& Away, bool bRightFirst, const FPhasingTraitTuning& Tuning);
}
