#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Templates/Function.h"
#include "UnpaidRules.generated.h"

struct FHuntingGround;

/**
 * When an Unpaid phase-steps, fading out and coming back nearer its target (Docs/Areas/RansomsRest.md, "Enemies by rank"):
 * when it's stuck, or has fallen far behind, while it chases. The distances are the design's whatever its size: a step is
 * always 3 to 5 m.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FPhaseStepRules
{
	GENERATED_BODY()

	/** A step brings it at least this much nearer its target (cm, seen from above)... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase Step", meta = (ClampMin = "50", Units = "cm"))
	float MinStep = 300.f;

	/** ...and at most this much. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase Step", meta = (ClampMin = "50", Units = "cm"))
	float MaxStep = 500.f;

	/** It never comes back nearer its target than this (cm): in front of them, never on top of them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase Step", meta = (ClampMin = "0", Units = "cm"))
	float NearestToTarget = 250.f;

	/** Chasing a target farther away than this (cm), it has fallen far behind: it steps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase Step", meta = (ClampMin = "0", Units = "cm"))
	float FarBehind = 1400.f;

	/** Chasing this long (s) without getting ProgressNeeded nearer than it has been, it's stuck (or being outrun): it steps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase Step", meta = (ClampMin = "0.1", Units = "s"))
	float StuckSeconds = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase Step", meta = (ClampMin = "0", Units = "cm"))
	float ProgressNeeded = 60.f;

	/** At least this long (s) from the end of one step to the start of the next. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase Step", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 3.f;

	/** The ground it comes back on is at most this far above or below its own (cm): a step never takes it to another level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase Step", meta = (ClampMin = "0", Units = "cm"))
	float MaxRise = 120.f;
};

/** What an Unpaid's rank adds to it (Docs/Areas/RansomsRest.md, "Enemies by rank"). */
struct FUnpaidRankTraits
{
	/** Its lunge flies this much faster, after a wind-up this share as long: the Restless's quicker lunge. */
	float LungeSpeedScale = 1.f;
	float WindupScale = 1.f;

	/** Its shriek sends out a ring that slows the player: the Gravebound's trait. */
	bool bSlowingShriek = false;

	/** How strongly its coal and ember edge glow in its color: a Basic one's dull red needs more to read as an ember. */
	float CoalGlow = 1.f;
};

/** The Unpaid's rules as plain functions, so the tests check them without a level (AUnpaidCreature plays them). */
namespace UnpaidRules
{
	/**
	 * What each rank adds: Restless and up lunge faster, Gravebound and Soulfed shriek to slow. A boss's own code tunes its
	 * fight (Abel), so Boss adds nothing here.
	 */
	AI_LOOTER_SHOOTER_API FUnpaidRankTraits RankTraits(ECreatureRank Rank);

	/**
	 * Whether a shot is on the coal: it came from within ViewAngleDegrees of the way the coal faces (from behind, the body
	 * is in the way), and its line from where it hit (ImpactPoint) on into the body, as far as Reach, passes within Radius of
	 * the coal's middle. Like the slime's core, the coal is found by the line, not by a hit zone of its own.
	 */
	AI_LOOTER_SHOOTER_API bool IsCoalShot(const FVector& Coal, const FVector& CoalFacing, const FVector& ImpactPoint,
		const FVector& ShotDirection, float Radius, float ViewAngleDegrees, float Reach);

	/**
	 * Whether a chasing Unpaid phase-steps now: its last step ended at least the cooldown ago (SinceLastStep), a step can
	 * take it at least MinStep nearer without landing nearer than NearestToTarget, and it's stuck (StuckSeconds without
	 * progress) or far behind.
	 */
	AI_LOOTER_SHOOTER_API bool WantsPhaseStep(const FPhaseStepRules& Rules, float DistanceToTarget, float StuckSeconds,
		float SinceLastStep);

	/** How far a step from DistanceToTarget (cm) takes it: up to MaxStep, never nearer than NearestToTarget; 0 under MinStep. */
	AI_LOOTER_SHOOTER_API float StepLength(const FPhaseStepRules& Rules, float DistanceToTarget);

	/**
	 * Where a step from Here (its feet) may land, in order: straight at Target first, then fanning out round the target up
	 * to 60 degrees either side, at the longest step and then shorter ones down to MinStep. Every spot is exactly its
	 * step nearer the target, seen from above, at Here's height.
	 */
	AI_LOOTER_SHOOTER_API TArray<FVector> PhaseCandidates(const FPhaseStepRules& Rules, const FVector& Here, const FVector& Target);

	/**
	 * The first of PhaseCandidates that lies on its hunting ground (Ground round Home) and where CanStand finds its feet
	 * (ground, room for its body and no wall: the level's checks), no more than MaxRise above or below Here. False when
	 * none does: it doesn't step.
	 */
	AI_LOOTER_SHOOTER_API bool ChoosePhaseSpot(const FPhaseStepRules& Rules, const FVector& Here, const FVector& Target,
		const FHuntingGround& Ground, const FVector& Home, TFunctionRef<bool(const FVector& Candidate, FVector& OutFeet)> CanStand,
		FVector& OutFeet);
}
