// AUnpaidCreature's sight of its target, and its pack's blinks (the 2026-10-28 play-check).
//  - Sight: with its kit no melee turn starts and no lunge flies without a clear line at its target through real geometry
//    (Looter.Unpaid.Sight). Unpaid 1.4-2.6 m outside the chapel nave's wall, or behind an open door leaf, kept taking turns
//    and lunging into it (one made 21 attacks, landing none), too near for a step that gains ground: a stalemate. Now one
//    holding a turn it can't see by lets it go (another takes it) and asks for none until it sees them again (a blind
//    spell), and stalled out of sight it may blink to where it sees them, nearer or not, on its own floor or theirs
//    (UnpaidCreaturePhase.cpp, UnpaidRules::ChooseSightSpot): into the doorway, into the nave, up onto the dais.
//  - The pack's blinks: those past FarBehind all stepped on one frame as they noticed the player or a wave spawned hunting
//    (5-6 at once). Now at most one phase-step or flank at a player every Looter.Unpaid.PackStepGap across all the Unpaid
//    (the fight director's step gate, FUnpaidStepGate; an owed press step first), and each one's step clock is jittered
//    (Looter.Unpaid.StepJitter) as it gets a new target.

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** It looks again this often (s), plus up to SightJitter, so a pack's traces fall on different frames. */
	constexpr float SightRecheck = 0.25f;
	constexpr float SightJitter = 0.1f;

	/** Its eyes: this share of its half-height over its middle (where its wait always looked from); theirs this far over their middle (cm). */
	constexpr float EyeShare = 0.6f;
	constexpr float TargetEyeLift = 50.f;

	/**
	 * A blind spell keeps it out of the line for turns: the director's rest is set this long (s) and renewed once less than
	 * BlindRenew is left, until it sees them again; then it's back in line at once.
	 */
	constexpr float BlindHold = 0.6f;
	constexpr float BlindRenew = 0.3f;
}

bool AUnpaidCreature::NeedsSight() const
{
	return HasFightKit() && FUnpaidFightTuning::Live().Sight != 0;
}

bool AUnpaidCreature::SeesTarget(const APawn& Victim, bool bFresh) const
{
	// Asked from its attack's start, its turn and its step each frame: one trace, kept a moment.
	const uint32 Id = Victim.GetUniqueID();
	if (!bFresh && Sight.TargetId == Id && Sight.RecheckIn > 0.f)
	{
		return Sight.bClear;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}
	// Real geometry only: the level's volumes answer world-static traces too (the scatter's PCG volume over all of Ransom's
	// Rest once refused every blink); the scatter's trees go with them, so it sees "through" a tree, never through a wall.
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("UnpaidSight"), this);
	Params.AddIgnoredActor(&Victim);
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * EyeShare);
	Sight.bClear = !World->LineTraceTestByObjectType(Eye, Victim.GetActorLocation() + FVector(0.f, 0.f, TargetEyeLift),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	Sight.TargetId = Id;
	Sight.RecheckIn = SightRecheck + FMath::FRand() * SightJitter;
	return Sight.bClear;
}

void AUnpaidCreature::TickSight(float DeltaSeconds)
{
	Sight.RecheckIn -= DeltaSeconds;
	Sight.BlindHoldLeft = FMath::Max(Sight.BlindHoldLeft - DeltaSeconds, 0.f);
	const APawn* Victim = GetTarget();
	const uint32 Id = Victim ? Victim->GetUniqueID() : 0;
	if (Id == Sight.StepTargetId)
	{
		return;
	}
	Sight.StepTargetId = Id;
	// A new target is a new look: no blind spell carried over (its rest runs out on its own).
	Sight.bBlindSpell = false;
	// A new target (it spawned hunting, or noticed them): its next step waits a moment of its own, so a pack that sees the
	// player together doesn't blink together. Never holding back an owed press step, nor a boss's own.
	if (Id != 0 && !bPressStepOwed && !IsBossCreature())
	{
		SinceLastStep = UnpaidFightRules::JitterStepClock(SinceLastStep, PhaseStep.Cooldown, FUnpaidFightTuning::Live().StepJitter,
			FMath::FRand());
	}
}

bool AUnpaidCreature::TickBlindTurn(UFightDirectorSubsystem& Director, const APawn& Victim)
{
	const bool bHolding = Director.HoldsToken(*this);
	if (!bHolding && !Sight.bBlindSpell)
	{
		return false;
	}
	if (!NeedsSight())
	{
		Sight.bBlindSpell = false;
		return false;
	}
	if (SeesTarget(Victim))
	{
		if (Sight.bBlindSpell && !bHolding)
		{
			// In sight again: back in line for a turn at once (its rest cut to nothing).
			Director.RestFromTurns(*this, UE_KINDA_SMALL_NUMBER);
			Sight.BlindHoldLeft = 0.f;
		}
		Sight.bBlindSpell = false;
		return false;
	}
	// Its turn would only bring it into the wall: another of the pack, with a clear way in, takes it. Out of the line while
	// it can't see them (not taken and let go again a few times a second); out of their sight it may blink to where it sees
	// them (WantsToStep).
	if (!Sight.bBlindSpell)
	{
		Sight.bBlindSpell = true;
		UE_LOG(LogLooter, Verbose, TEXT("%s can't see %s: it lets its turn go until it does."), *GetName(), *Victim.GetName());
	}
	if (bHolding || Sight.BlindHoldLeft <= BlindRenew)
	{
		Director.RestFromTurns(*this, BlindHold);
		Sight.BlindHoldLeft = BlindHold;
	}
	return bHolding;
}

bool AUnpaidCreature::MayPackStep(const APawn& Victim)
{
	// A boss's steps are its own fight's.
	if (IsBossCreature())
	{
		return true;
	}
	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this);
	return !Director || Director->MayStep(*this, Victim, FUnpaidFightTuning::Live().PackStepGap, bPressStepOwed);
}
