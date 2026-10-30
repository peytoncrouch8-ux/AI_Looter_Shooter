// AUnpaidCreature's varied moves in their frames (2026-10-29; UnpaidCreatureVariety.cpp picks them): the feint's and the low
// swipe's flights (TickLunge flies them, UnpaidCreatureAttack.cpp), the phase-strike's claw and its tell once it's back, the
// feint's slide aside, a lunge's miss (its chance of a shriek), and each move's pose on the body's channels.

#include "Creatures/UnpaidKitWork.h"
#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/UnpaidBodyParts.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Creatures/UnpaidMoveRules.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Creatures/UnpaidPackSubsystem.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	namespace Cue = LooterSoundCue::UnpaidScream;

	/** Up and down a claw or a skim still reaches (cm at size 1 or more), as the lunge does. */
	constexpr float ReachUpDown = 220.f;

	/** The phase-strike's tell flares its coal this bright (the lunge's tell is Looter.Unpaid.TellFlare, 0.6). */
	constexpr float StrikeTellFlare = 0.9f;

	/** Its whisper is heard from about where its head is (cm over its middle). */
	constexpr float WhisperLift = 60.f;

	float Ease(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	/** Nothing solid between its middle and Victim's (or their feet): as the rake's and a bite's strike ask. */
	bool ClawClear(const AActor& Creature, const APawn& Victim)
	{
		const UWorld* World = Creature.GetWorld();
		if (!World)
		{
			return true;
		}
		FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("UnpaidStrike"), &Creature, false);
		Params.AddIgnoredActor(&Victim);
		const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
		const float Half = Victim.GetSimpleCollisionHalfHeight();
		for (const FVector& To : { Victim.GetActorLocation(), Victim.GetActorLocation() - FVector(0.f, 0.f, Half * 0.7f) })
		{
			if (!World->LineTraceTestByObjectType(Creature.GetActorLocation(), To, Solid, Params))
			{
				return true;
			}
		}
		return false;
	}
}

void FUnpaidMoveWork::StartFlight(AUnpaidCreature& Unpaid, EUnpaidMove Move)
{
	// Like the lunge's flight (UnpaidCreatureAttack.cpp), each with its own length and pace.
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	const FUnpaidFightTuning Kit = FUnpaidFightTuning::Live();
	const float Scale = Unpaid.GetSizeScale();
	const APawn* Victim = Unpaid.GetTarget();
	const bool bLow = Move == EUnpaidMove::LowSwipe;
	Unpaid.Kit.bLunging = false;
	Unpaid.Kit.bLungeLanded = false;
	Unpaid.Kit.LungeLeft = 0.f;
	// Never into a wall: its target out of sight through the wind-up, it holds where it is (as the lunge does).
	if (Victim && Kit.Sight != 0 && !Unpaid.SeesTarget(*Victim, true))
	{
		return;
	}
	State.FlightSpeed = bLow ? Tuning.LowSwipeSpeed * Scale : 0.f;
	const float Speed = FUnpaidMoveWork::FlightSpeed(Unpaid);
	const FVector Here = Unpaid.GetActorLocation();
	FVector Aim = Here + Unpaid.GetActorForwardVector() * (Kit.LungeDistance * Scale);
	if (Victim)
	{
		Aim = bLow ? UnpaidFightRules::LeadPoint(Here, Victim->GetActorLocation(), Victim->GetVelocity(), Speed, Kit.LeadShare, Kit.LeadMaxSeconds)
			: Victim->GetActorLocation();
	}
	FVector To = Aim - Here;
	To.Z = 0.f;
	Unpaid.Kit.LungeDirection = To.IsNearlyZero() ? Unpaid.GetActorForwardVector() : To.GetSafeNormal();
	const float Gap = static_cast<float>(To.Size());
	float Length = 0.f;
	if (bLow)
	{
		// It skims through where they stand and on past: their feet are in its way unless they're off the ground.
		Length = FMath::Clamp(Gap + Tuning.LowSwipePast * Scale, 0.f, Tuning.LowSwipeDistance * Scale);
	}
	else
	{
		// The feint: part of the lunge's way, stopping well short of them.
		const float Full = FMath::Clamp(Gap - Unpaid.LungeStopShort * Scale, 0.f, Kit.LungeDistance * Scale);
		Length = FMath::Min(Full * Tuning.FeintShare, FMath::Max(Gap - Tuning.FeintKeep * Scale, 0.f));
	}
	// Never off a drop: cut short where the ground ends, as the lunge is.
	const float HalfHeight = Unpaid.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector Ground;
	while (Length > 40.f * Scale && !Unpaid.FindGround(Here + Unpaid.Kit.LungeDirection * Length, 50.f * Scale, HalfHeight + 350.f * Scale, Ground))
	{
		Length *= 0.6f;
	}
	Unpaid.Kit.LungeLeft = Length > 40.f * Scale ? Length : 0.f;
	Unpaid.Kit.LungeTime = 0.f;
	Unpaid.Kit.LungeReach = FMath::Max(Length, 1.f);
	Unpaid.Kit.bLunging = Unpaid.Kit.LungeLeft > 0.f;
	Unpaid.CommitLunge(Unpaid.Kit.bLunging && Unpaid.Kit.LungeLeft <= Speed * Kit.LungeCommit);
	// The rush of cloth and cold air (a low swipe's lower and shorter).
	Unpaid.Voice->Play(LooterSoundCue::UnpaidLunge, bLow ? 0.75f : 1.f);
	if (!Unpaid.Kit.bLunging)
	{
		// Already there, or nowhere to fly.
		if (bLow)
		{
			FUnpaidMoveWork::TryLowSwipeHit(Unpaid);
		}
		else
		{
			FUnpaidMoveWork::OnFlightEnded(Unpaid);
		}
	}
}

bool FUnpaidMoveWork::Claw(AUnpaidCreature& Unpaid)
{
	// The phase-strike's claw across its front.
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	const float Scale = Unpaid.GetSizeScale();
	Unpaid.Voice->Play(FName(UnpaidKitCue::Rake));
	APawn* Victim = Unpaid.GetTarget();
	if (!Unpaid.IsValidTarget(Victim))
	{
		return false;
	}
	const FVector Here = Unpaid.GetActorLocation();
	const FVector There = Victim->GetActorLocation();
	if (!UnpaidFightRules::InSwipe(Here, Unpaid.GetActorForwardVector(), There, Tuning.PhaseStrikeReach * Scale, Tuning.PhaseStrikeArc,
		ReachUpDown * FMath::Max(Scale, 1.f)) || !ClawClear(Unpaid, *Victim))
	{
		return false;
	}
	// Knocked aside the way the arm swept.
	const FVector Across = Unpaid.GetActorRightVector() * (Unpaid.Kit.Moves.StrikeArm == 0 ? 1.f : -1.f);
	Unpaid.HitWithAttack(Victim, ((There - Here).GetSafeNormal2D() + 0.6f * Across).GetSafeNormal2D(), Tuning.PhaseStrikeDamageShare);
	UUnpaidPackSubsystem::Note(&Unpaid, EUnpaidTally::PhaseStrike, true);
	return true;
}

bool FUnpaidMoveWork::Strike(AUnpaidCreature& Unpaid)
{
	switch (Unpaid.Kit.Move)
	{
	case EUnpaidMove::PhaseStrike:
		Unpaid.OnAttackStrike(Claw(Unpaid));
		return true;
	case EUnpaidMove::Feint:
		StartFlight(Unpaid, EUnpaidMove::Feint);
		Unpaid.OnAttackStrike(false);
		return true;
	case EUnpaidMove::LowSwipe:
		StartFlight(Unpaid, EUnpaidMove::LowSwipe);
		Unpaid.OnAttackStrike(Unpaid.Kit.bLungeLanded);
		return true;
	default:
		return false;
	}
}

float FUnpaidMoveWork::FlightSpeed(const AUnpaidCreature& Unpaid)
{
	return Unpaid.Kit.Move == EUnpaidMove::LowSwipe && Unpaid.Kit.Moves.FlightSpeed > 0.f ? Unpaid.Kit.Moves.FlightSpeed : Unpaid.GetLungeSpeed();
}

bool FUnpaidMoveWork::TryLowSwipeHit(AUnpaidCreature& Unpaid)
{
	APawn* Victim = Unpaid.GetTarget();
	if (Unpaid.Kit.bLungeLanded || !Unpaid.IsValidTarget(Victim))
	{
		return false;
	}
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	const float Scale = Unpaid.GetSizeScale();
	const FVector ToVictim = Victim->GetActorLocation() - Unpaid.GetActorLocation();
	if (ToVictim.Size2D() > Tuning.LowSwipeRadius * Scale || FMath::Abs(ToVictim.Z) > ReachUpDown * FMath::Max(Scale, 1.f))
	{
		return false;
	}
	// Its claws sweep the ground: feet off it (a jump, a step up) pass over.
	const float MyFeet = static_cast<float>(Unpaid.GetActorLocation().Z) - Unpaid.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float TheirFeet = static_cast<float>(Victim->GetActorLocation().Z) - Victim->GetSimpleCollisionHalfHeight();
	const ACharacter* Walker = Cast<ACharacter>(Victim);
	const bool bAirborne = Walker && Walker->GetCharacterMovement() && Walker->GetCharacterMovement()->IsFalling();
	if (UnpaidMoveRules::ClearsLowSwipe(TheirFeet - MyFeet, bAirborne, Tuning.LowSwipeClear * Scale))
	{
		return false;
	}
	if (FUnpaidFightTuning::Live().Sight != 0 && !Unpaid.SeesTarget(*Victim, true))
	{
		return false;
	}
	Unpaid.Kit.bLungeLanded = true;
	Unpaid.HitWithAttack(Victim, ToVictim.GetSafeNormal2D(), Tuning.LowSwipeDamageShare);
	UUnpaidPackSubsystem::Note(&Unpaid, EUnpaidTally::LowSwipe, true);
	return true;
}

void FUnpaidMoveWork::OnFlightEnded(AUnpaidCreature& Unpaid)
{
	if (!IsOn(Unpaid))
	{
		return;
	}
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	if (Unpaid.Kit.Move == EUnpaidMove::Feint)
	{
		// Stopped short: it slides aside, to whichever side has ground (its dice first), its eyes on them.
		const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
		const float Scale = Unpaid.GetSizeScale();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Unpaid.Kit.LungeDirection).GetSafeNormal2D();
		const float HalfHeight = Unpaid.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const float First = FMath::RandBool() ? 1.f : -1.f;
		FVector Ground;
		for (const float Way : { First, -First })
		{
			if (Unpaid.FindGround(Unpaid.GetActorLocation() + Side * (Way * Tuning.FeintSlide * Scale), 50.f * Scale, HalfHeight + 200.f * Scale, Ground))
			{
				State.SlideWay = Side * Way;
				State.SlideLeft = Tuning.FeintSlideSeconds;
				// Eased out (fast, then slowing): twice the average pace at its start.
				State.SlideSpeed = 2.f * Tuning.FeintSlide * Scale / Tuning.FeintSlideSeconds;
				Unpaid.Voice->Play(FName(Cue::Feint));
				break;
			}
		}
		return;
	}
	if (Unpaid.Kit.Move != EUnpaidMove::Lunge)
	{
		return;
	}
	if (Unpaid.Kit.bLungeLanded)
	{
		State.bLastLungeMissed = false;
		return;
	}
	NoteLungeMissed(Unpaid);
}

void FUnpaidMoveWork::NoteLungeMissed(AUnpaidCreature& Unpaid)
{
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	if (!IsOn(Unpaid) || State.bMissNoted || Unpaid.Kit.Move != EUnpaidMove::Lunge || Unpaid.Kit.bLungeLanded)
	{
		return;
	}
	State.bMissNoted = true;
	State.bLastLungeMissed = true;
	const APawn* Victim = Unpaid.GetTarget();
	if (!Victim)
	{
		return;
	}
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	const UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid);
	const float SincePack = Packs ? Packs->SincePackShriek(*Victim, Unpaid.PackTag) : 1000.f;
	if (!UnpaidMoveRules::ShriekOnMiss(FMath::FRand(), Tuning.ShriekChance, State.SinceShriek, Tuning.ShriekCooldown, SincePack, Tuning.ShriekPackGap))
	{
		return;
	}
	// Missed: it doesn't lunge again, it turns on them and shrieks. Its recovery is cut short, the shriek comes as its turn ends.
	State.bShriekOwed = true;
	Unpaid.AttackRecovery = FMath::Max(Unpaid.GetStateTime() - Unpaid.AttackWindup, 0.f);
	UE_LOG(LogLooter, Verbose, TEXT("%s's lunge missed %s: it shrieks."), *Unpaid.GetName(), *Victim->GetName());
}

// ---------------------------------------------------------------------------
// Each frame
// ---------------------------------------------------------------------------

void FUnpaidMoveWork::TickAttack(AUnpaidCreature& Unpaid, float DeltaSeconds)
{
	if (!IsOn(Unpaid))
	{
		return;
	}
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	const APawn* Victim = Unpaid.GetTarget();
	// The phase-strike's tell: back at their side, its claw raised, a whisper close by, its coal flaring bright.
	if (Unpaid.Kit.Move == EUnpaidMove::PhaseStrike && Unpaid.GetStateTime() < Unpaid.AttackWindup)
	{
		if (!State.bStrikeTold && Unpaid.Kit.PhaseState == EUnpaidPhaseState::In)
		{
			State.bStrikeTold = true;
			LooterSound::PlayAt(&Unpaid, FName(Cue::PhaseStrike), Unpaid.GetActorLocation() + FVector(0.f, 0.f, WhisperLift * Unpaid.GetSizeScale()), 1.f,
				Unpaid.Voice->GetPitch());
		}
		if (!Unpaid.IsPhasing())
		{
			Unpaid.Heat = FMath::Max(Unpaid.Heat, StrikeTellFlare);
		}
	}
	// The feint's slide aside: fast and slowing, eyes kept on them (no input: the movement doesn't turn it to its way).
	if (State.SlideLeft > 0.f && !Unpaid.Kit.bLunging)
	{
		const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
		State.SlideLeft = FMath::Max(State.SlideLeft - DeltaSeconds, 0.f);
		const float Alpha = 1.f - State.SlideLeft / FMath::Max(Tuning.FeintSlideSeconds, 0.01f);
		UCharacterMovementComponent* Movement = Unpaid.GetCharacterMovement();
		Movement->Velocity = State.SlideWay * (State.SlideSpeed * (1.f - Alpha)) + FVector(0.f, 0.f, Movement->Velocity.Z);
		if (Victim)
		{
			const FRotator Facing(0.f, static_cast<float>((Victim->GetActorLocation() - Unpaid.GetActorLocation()).Rotation().Yaw), 0.f);
			Unpaid.SetActorRotation(FMath::RInterpTo(Unpaid.GetActorRotation(), Facing, DeltaSeconds, 10.f));
		}
	}
}

// ---------------------------------------------------------------------------
// The body and the end
// ---------------------------------------------------------------------------

void FUnpaidMoveWork::Channels(const AUnpaidCreature& Unpaid, UnpaidBody::FPoseChannels& Goal)
{
	if (!FUnpaidMoveTuning::IsVarietyOn() || Unpaid.IsDead())
	{
		return;
	}
	const FUnpaidMoveState& State = Unpaid.Kit.Moves;
	const ECreatureState BrainState = Unpaid.GetCreatureState();
	if (BrainState == ECreatureState::Chase)
	{
		// Hanging back: wary, upright, arms half down; rallied: hungrier, mouth wider.
		if (State.HangLeft > 0.f)
		{
			Goal.Lean -= 6.f;
			Goal.Hunt *= 0.5f;
			Goal.Spread = FMath::Max(Goal.Spread, 0.2f);
		}
		if (State.RalliedLeft > 0.f)
		{
			Goal.Hunt = 1.f;
			Goal.Jaw += 6.f;
		}
		return;
	}
	if (BrainState != ECreatureState::Attack)
	{
		return;
	}
	const float InState = Unpaid.GetStateTime();
	const float Windup = FMath::Max(Unpaid.AttackWindup, 0.01f);
	switch (Unpaid.Kit.Move)
	{
	case EUnpaidMove::LowSwipe:
	{
		// Sunk low, arms out wide and down along the ground, claws curled; then thrown forward skimming, arms swept in.
		const float Sink = Ease(InState / Windup);
		const bool bSkim = InState >= Windup;
		Goal.Rear = 0.f;
		Goal.Lean = bSkim ? 34.f : FMath::Lerp(Goal.Lean, 30.f, Sink);
		Goal.Crumple = 0.3f * (bSkim ? 1.f : Sink);
		Goal.Spread = bSkim ? 0.5f : 0.9f * Sink;
		Goal.Reach = bSkim ? 0.7f : 0.15f * Sink;
		Goal.Curl = 55.f;
		Goal.Splay = 6.f;
		Goal.Jaw = FMath::Lerp(12.f, 26.f, Sink);
		Goal.Hunt = 0.f;
		Goal.Rake[0] = bSkim ? 0.9f : -0.4f * Sink;
		Goal.Rake[1] = bSkim ? 0.9f : -0.4f * Sink;
		Goal.Snap = bSkim && Unpaid.Kit.bLunging ? 1.f : 0.f;
		break;
	}
	case EUnpaidMove::PhaseStrike:
	{
		// Back in, its claw drawn high and back (the tell), the other arm out for balance; then the claw swept across.
		const int32 Arm = FMath::Clamp(State.StrikeArm, 0, 1);
		const float Tell = Ease((InState - (Windup - 0.45f)) / 0.3f);
		if (InState < Windup)
		{
			Goal.Rake[Arm] = -Tell;
			Goal.Rake[1 - Arm] = -0.3f * Tell;
			Goal.Rear = 0.3f * Tell;
			Goal.Jaw = FMath::Lerp(12.f, Unpaid.JawOpenDegrees * 0.85f, Tell);
			Goal.Curl = -5.f;
			Goal.Splay = 18.f;
			Goal.Spread = FMath::Max(Goal.Spread, 0.25f * Tell);
			Goal.Lean = FMath::Lerp(Goal.Lean, -4.f, Tell);
		}
		else
		{
			const float Done = Ease((InState - Windup - 0.2f) / 0.4f);
			Goal.Rake[Arm] = 1.f - 0.6f * Done;
			Goal.Rake[1 - Arm] = -0.5f * (1.f - Done);
			Goal.Lean = 8.f + 16.f * (1.f - Done);
			Goal.Reach = 0.f;
		}
		Goal.Hunt = 0.f;
		break;
	}
	case EUnpaidMove::Feint:
		// The lunge's own wind-up and flight (that is the feint); sliding aside, it hangs upright, arms out, shroud flung.
		if (State.SlideLeft > 0.f)
		{
			Goal.Reach = 0.3f;
			Goal.Spread = 0.55f;
			Goal.Lean = 2.f;
			Goal.Snap = 0.7f;
			Goal.Jaw = 18.f;
		}
		break;
	default:
		break;
	}
}

