#pragma once

#include "CoreMinimal.h"
#include "Creatures/UnpaidFightRules.h"

/** Where an Unpaid is in a phase-step (UnpaidCreaturePhase.cpp). */
enum class EUnpaidPhaseState : uint8
{
	None,
	/** Fading away where it stands. */
	Out,
	/** Gone, moved, and fading back in nearer its target. */
	In
};

/**
 * What AUnpaidCreature keeps of its fighting kit as it fights, held by value as its Kit (split out of UnpaidCreature.h for
 * the ~500-line rule, 2026-10-29; nothing here is a UPROPERTY): the kit's state (FUnpaidKitState, which it extends, so
 * Kit.Move and Kit.Reset() read as they always did; Reset clears only that part), the lunge, the phase-step and its sight.
 * The lunge and the phase-step are put back by its death and coming back (UnpaidCreature.cpp), as before the split.
 */
struct FUnpaidKitMembers : FUnpaidKitState
{
	// --- The lunge (UnpaidCreatureAttack.cpp) ---
	bool bLunging = false;
	bool bLungeLanded = false;
	/** In the committed last stretch (Looter.Unpaid.LungeCommit): its hit slow suspended. */
	bool bLungeCommitted = false;
	FVector LungeDirection = FVector::ForwardVector;
	/** How far it still flies (cm), and how long it has flown (s). */
	float LungeLeft = 0.f;
	float LungeTime = 0.f;
	/** Seconds since its last slowing shriek (a Gravebound's). */
	float SinceSlowingShriek = 1000.f;

	// --- Phase-steps (UnpaidCreaturePhase.cpp) ---
	EUnpaidPhaseState PhaseState = EUnpaidPhaseState::None;
	float PhaseTime = 0.f;
	FVector PhaseDestination = FVector::ZeroVector;
	float SinceLastStep = 1000.f;
	/**
	 * How far from its target its last step or flank landed (cm, flat; set at Arrive): a landing this near gets the longer
	 * arrival hold (Looter.Unpaid.ArrivalHoldNear, the chapel dais).
	 */
	float LandedGap = TNumericLimits<float>::Max();
	/** Pressed by a hit from past its edge: its next step comes as soon as its cooldown allows, stuck or not. */
	bool bPressStepOwed = false;
	/** What it last saw of its target, and the target its step clock was jittered for (mutable: SeesTarget keeps it). */
	mutable FUnpaidSightState Sight;
	/** How long it has chased without getting nearer, the nearest it has been, and when it may look for a spot again (s). */
	float StallTime = 0.f;
	float BestDistance = TNumericLimits<float>::Max();
	float RetryIn = 0.f;
};
