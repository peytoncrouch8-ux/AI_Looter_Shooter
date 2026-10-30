// AUnpaidCreature as a pack with a head (FIGHTS.md lane 5, 2026-10-29; the user: fights "clearly harder", Borderlands-normal).
//  - The fade-back (Looter.Unpaid.FadeBack): a hurt Basic, at the end of a turn, may slip out of the fight once a life: it
//    phase-steps away to the ring's outer edge (500-650 cm from its target, counted at the pack's step gate like any blink),
//    waits there a moment (its brain held, no turns), then comes on again. A read the player can punish: a fading coal at
//    the edge of the fight is a kill for whoever follows it up.
//  - Its pack's leader down (ACreatureBase::OnPackLeaderDown, from PackLeaderRules): it falters as every kind does, a scream
//    or a cinder's tell dies in its throat, and its coal gutters a moment (Looter.Unpaid.GutterSeconds), the pack's fire
//    gone low. The director ends the frenzy and holds off rallies for the lull (UnpaidCreatureRing.cpp reads it).

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightEvent.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Creatures/UnpaidKitWork.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Its turns stay rested this long (s) past the fade-back's wait, so it doesn't take one the frame it comes on. */
	constexpr float RestTail = 0.3f;

	/** A guttering coal sinks this far under its glow (its Heat: -1 is out) and flickers this fast (radians a second of its clock). */
	constexpr float GutterDim = 0.6f;
	constexpr float GutterFlicker = 37.f;
}

void AUnpaidCreature::OnTurnPlayedOut()
{
	Super::OnTurnPlayedOut();
	// A missed lunge's shriek comes first (UnpaidCreatureVariety.cpp): no fade-back on top of it.
	if (FUnpaidMoveWork::OnTurnPlayedOut(*this))
	{
		return;
	}
	TryFadeBack(true);
}

bool AUnpaidCreature::TryFadeBack(bool bRoll)
{
	const APawn* Victim = GetTarget();
	if (!Victim || IsDead() || !HasFightKit() || IsBossAdd() || IsPhasing() || Kit.bLunging || IsFadingBack())
	{
		return false;
	}
	const FUnpaidFightTuning Tuning = FUnpaidFightTuning::Live();
	const float HealthShare = Health->GetHealthPercent();
	if (!UnpaidFightRules::MayFadeBack(GetRank() == ECreatureRank::Basic, HealthShare, Tuning.FadeBackBelow, Kit.bFadedBack))
	{
		return false;
	}
	// Its chance is rolled once a life, the first time it may: lost, it fights on to the end.
	if (bRoll)
	{
		if (Kit.FadeRoll < 0)
		{
			Kit.FadeRoll = Tuning.FadeBack > 0.f && FMath::FRand() < Tuning.FadeBack ? 1 : 0;
		}
		if (Kit.FadeRoll == 0)
		{
			return false;
		}
	}
	// One blink at a player at a time across the pack (StartStepTo counts it at the gate); a refused one tries at its next turn.
	FVector Feet;
	if (!MayPackStep(*Victim) || !FindFadeSpot(*Victim, Feet))
	{
		return false;
	}
	StartStepTo(Feet);
	Kit.Fade = EUnpaidFadeBack::Stepping;
	Kit.bFadedBack = true;
	Kit.bFlankArmed = false;
	Kit.WaitTime = 0.f;
	// Out of the line for turns until its wait is done: the next of the pack takes its place.
	if (UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this))
	{
		Director->RestFromTurns(*this, PhaseFadeOutSeconds + PhaseFadeInSeconds + Tuning.FadeBackWait + RestTail);
		Director->NoteEvent(*this, EFightEvent::FadeBack, HealthShare);
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s fades back from %s at %.0f%% health, %.1f m off."), *GetName(), *Victim->GetName(), 100.f * HealthShare,
		FVector::Dist2D(Feet, Victim->GetActorLocation()) / 100.0);
	return true;
}

bool AUnpaidCreature::FindFadeSpot(const APawn& Victim, FVector& OutFeet) const
{
	const FUnpaidFightTuning Tuning = FUnpaidFightTuning::Live();
	const float Scale = GetSizeScale();
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	// Away from them on its own side, on its own level of ground, never off its hunting ground, with them in sight (CanStandAt).
	for (const FVector& Candidate : UnpaidFightRules::FadeBackSpots(Feet, Victim.GetActorLocation(), Tuning.FadeBackNear * Scale,
		Tuning.FadeBackFar * Scale, static_cast<float>(Feet.Z)))
	{
		FVector Ground;
		if (!HuntingGround.ContainsSpot(Candidate, GetHome().GetLocation(), GetGroundReach()) || !CanStandAt(Candidate, Ground)
			|| FMath::Abs(Ground.Z - Feet.Z) > PhaseStep.MaxRise)
		{
			continue;
		}
		OutFeet = Ground;
		return true;
	}
	return false;
}

void AUnpaidCreature::TickFadeBack(float DeltaSeconds)
{
	// The gutter: the coal sinks low and flickers, as a fire does when the wind takes it (a crit's flare still shows through).
	if (Kit.GutterLeft > 0.f)
	{
		Kit.GutterLeft = FMath::Max(Kit.GutterLeft - DeltaSeconds, 0.f);
		const float Flicker = 0.5f + 0.5f * FMath::Sin(Kit.GutterLeft * GutterFlicker);
		Heat = FMath::Min(Heat, -GutterDim * (0.6f + 0.4f * Flicker));
	}
	if (Kit.Fade == EUnpaidFadeBack::Stepping)
	{
		if (IsPhasing())
		{
			return;
		}
		// Back in at the edge of the fight: it hangs there a moment, watching, before it comes on again.
		const float Wait = FUnpaidFightTuning::Live().FadeBackWait;
		Kit.Fade = EUnpaidFadeBack::Waiting;
		Kit.FadeLeft = Wait;
		HoldBrain(Wait);
		if (UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this))
		{
			Director->RestFromTurns(*this, Wait + RestTail);
		}
		return;
	}
	if (Kit.Fade == EUnpaidFadeBack::Waiting)
	{
		Kit.FadeLeft -= DeltaSeconds;
		if (Kit.FadeLeft <= 0.f || !GetTarget())
		{
			// It comes on again by the chase, not a blink: its wait counted as no stall.
			Kit.Fade = EUnpaidFadeBack::None;
			Kit.FadeLeft = 0.f;
			ResetProgress();
		}
	}
}

void AUnpaidCreature::OnPackLeaderDown(const ACreatureBase& Leader)
{
	// It falters as every kind does (its turn back, a wind-up called off, its brain held a moment).
	Super::OnPackLeaderDown(Leader);
	if (IsDead() || IsPassive() || KeepsOwnFight())
	{
		return;
	}
	// A scream or a cinder's tell dies in its throat; no flank waits for its next turn.
	if (Kit.Action != EUnpaidAction::None)
	{
		EndKitAction(true);
	}
	Kit.bFlankArmed = false;
	Kit.GutterLeft = FUnpaidFightTuning::Live().GutterSeconds;
}
