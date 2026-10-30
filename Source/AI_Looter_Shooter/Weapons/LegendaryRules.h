#pragma once

#include "CoreMinimal.h"
#include "Weapons/LegendarySignature.h"
#include "Weapons/LegendaryTuning.h"

struct FWeaponInstanceData;

/**
 * The unique legendaries' plain rules (numbers in, numbers out; the tests drive them without a world): a gun's signature,
 * its card line, its boss's drop roll, Gravesilk's silk and root, Company Scrip's burst and refund, Lamplighter's scoped
 * kill and lantern. The play is ULegendarySubsystem (the bullets' hooks), ULegendaryEffectComponent (the gun's side) and
 * ALegendaryLantern; the numbers FLegendaryTuning.
 */
namespace LegendaryRules
{
	/** The gun's signature: its named gun's (UNamedWeaponDefinition::Signature), None for every other gun. */
	AI_LOOTER_SHOOTER_API ELegendarySignature SignatureOf(const FWeaponInstanceData& Gun);

	/** What it does, in a line for the gun card, with the live numbers ("Hits lay silk: 6 root the target for 1 s..."). */
	AI_LOOTER_SHOOTER_API FString Description(ELegendarySignature Signature, const FLegendaryTuning& Tuning);

	/** Its boss's chance a kill to drop it (Looter.Legendary.<Name>.DropChance); 0 for None. */
	AI_LOOTER_SHOOTER_API float DropChance(ELegendarySignature Signature, const FLegendaryTuning& Tuning);

	/** A roll (0-1) under the chance drops it. */
	inline bool RollsDrop(float Chance, float Roll01) { return Chance > 0.f && Roll01 < Chance; }

	// --- Gravesilk ---

	/** The silk on one creature. */
	struct FSilk
	{
		int32 Stacks = 0;
		/** When the last stack was laid; the silk falls away SilkSeconds after it. */
		double LastLaid = -1.0e9;
		/** When its last root ends (the gap after it counts from there). */
		double RootEnds = -1.0e9;
	};

	enum class ESilk : uint8
	{
		/** Nothing took: a boss, a creature rooted now or in the gap after. */
		None,
		/** A stack laid, not yet enough. */
		Laid,
		/** The stack that roots it (the stacks are spent). */
		Rooted
	};

	/** A Gravesilk hit at Now on a creature with Silk (a boss: never). */
	AI_LOOTER_SHOOTER_API ESilk LaySilk(FSilk& Silk, double Now, bool bBoss, const FLegendaryTuning& Tuning);

	/** The stacks still on it at Now (0 once fallen away). */
	AI_LOOTER_SHOOTER_API int32 StacksAt(const FSilk& Silk, double Now, const FLegendaryTuning& Tuning);

	inline bool IsRooted(const FSilk& Silk, double Now) { return Now < Silk.RootEnds; }

	// --- Company Scrip ---

	/** The fire rate's multiplier with Stacks kills in this burst: 1 + FireRatePerStack each, up to MaxStacks. */
	AI_LOOTER_SHOOTER_API float ScripFireRateScale(int32 Stacks, const FLegendaryTuning& Tuning);

	/** Stacks after one more kill in the burst (never past MaxStacks). */
	AI_LOOTER_SHOOTER_API int32 ScripAddKill(int32 Stacks, const FLegendaryTuning& Tuning);

	/** A burst still holds SinceLastShot s after its last round (a reload or a swap lets it lapse). */
	inline bool ScripBurstHolds(double SinceLastShot, const FLegendaryTuning& Tuning) { return SinceLastShot <= Tuning.CompanyScripBurstGap; }

	/** Rounds a reload that took RoundsTaken pays back to the pool: RefundShare of the magazine, never more than it took. */
	AI_LOOTER_SHOOTER_API int32 ScripRefund(int32 RoundsTaken, int32 MagazineSize, const FLegendaryTuning& Tuning);

	// --- Lamplighter ---

	/** A shot with the sights raised AimAlpha of the way (0 hip, 1 through the scope) counts as scoped. */
	inline bool IsScopedShot(float AimAlpha, const FLegendaryTuning& Tuning) { return AimAlpha >= Tuning.LamplighterScopedFrom && AimAlpha > 0.f; }

	/** Health a lantern gives a player of MaxHealth over DeltaSeconds. */
	AI_LOOTER_SHOOTER_API float LanternHeal(float MaxHealth, float DeltaSeconds, const FLegendaryTuning& Tuning);

	/** A body at Body is in reach of a lantern at Lantern (on the ground). */
	AI_LOOTER_SHOOTER_API bool InLanternReach(const FVector& Lantern, const FVector& Body, float Radius);

	/** How bright a lantern Age s old with Life s to burn shows (0-1): lit quickly, guttering out over its last half second. */
	AI_LOOTER_SHOOTER_API float LanternGlow(float Age, float Life);
}
