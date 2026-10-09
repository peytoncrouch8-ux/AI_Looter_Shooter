// ACreatureBase: getting free when it's stuck (FCreatureUnstick has the rules). Blocked a while, its steering feels a wall
// ahead and goes round it. Wedged against something (pressing on without getting anywhere) or hung in the air on a prop's
// edge, it glides to the nearest free spot with ground under it, as the player's unstick does
// (PlayerLocomotionJumpAssist.cpp, FTraversalProbe::FindFreeSpot): a quarter-second glide, then the fall takes it the last
// little way.

#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureSteerProbe.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void ACreatureBase::TickUnstick(float DeltaSeconds, const FVector& Goal, float Speed)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	// Only a body its own movement carries: one held still by a scene, dead, or not yet moving has nothing to get free of.
	if (!Movement->IsMovingOnGround() && !Movement->IsFalling())
	{
		Unstick.Reset(GetActorLocation());
		return;
	}
	FCreatureUnstick::FFrame Frame;
	Frame.Here = GetActorLocation();
	Frame.bBlocked = IsStuck(Speed);
	// Its walking movement took its steering. A body standing still for something else (an Unpaid fading out for a
	// phase-step, a boss in one of its moments) has its steering taken from it, and isn't wedged.
	Frame.bPressing = !SteerDirection.IsNearlyZero() && !Movement->GetCurrentAcceleration().IsNearlyZero();
	Frame.bFalling = Movement->IsFalling();
	Frame.SizeScale = SizeScale;
	Frame.DeltaSeconds = DeltaSeconds;
	const FCreatureUnstick::EAction Action = Unstick.Update(Frame);
	switch (Action)
	{
	case FCreatureUnstick::EAction::GoRound:
		SteerPlanner.NoteStuck(MakeSteerRequest(Goal, SteerDirection, Speed), SteerDirection.IsNearlyZero() ? GetActorForwardVector() : SteerDirection,
			GetCapsuleComponent()->GetScaledCapsuleRadius());
		// It plans its way round at once.
		SteerTimer = 0.f;
		break;
	case FCreatureUnstick::EAction::NudgeWedged:
	case FCreatureUnstick::EAction::NudgeHung:
		if (!NudgeFree(Action == FCreatureUnstick::EAction::NudgeHung))
		{
			Unstick.NudgeFailed();
		}
		break;
	default:
		break;
	}
}

bool ACreatureBase::NudgeFree(bool bHung)
{
	const FCreatureSteerProbe Probe(*this);
	const FVector Here = GetActorLocation();
	FVector Spot;
	if (!Probe.FindFreeSpot(SteerDirection.IsNearlyZero() ? GetActorForwardVector() : SteerDirection, Spot))
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s %s at %s, with no free spot near."), *GetName(), bHung ? TEXT("hung") : TEXT("wedged"),
			*Here.ToCompactString());
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("%s %s at %s: slid free to %s."), *GetName(), bHung ? TEXT("hung") : TEXT("wedged"), *Here.ToCompactString(),
		*Spot.ToCompactString());
	Unstick.StartGlide(Here, Spot);
	GetCharacterMovement()->StopMovementImmediately();
	// Its way round what held it starts afresh from the free spot.
	SteerPlanner.Forget(true);
	return true;
}

bool ACreatureBase::TickNudge(float DeltaSeconds)
{
	if (!Unstick.IsGliding())
	{
		return false;
	}
	bool bDone = false;
	const FVector Where = Unstick.StepGlide(DeltaSeconds, bDone);
	SetActorLocation(Where, false, nullptr, ETeleportType::TeleportPhysics);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	if (bDone)
	{
		// The fall takes it the last little way, and its movement finds the ground under it. (Not one its movement was
		// taken from meanwhile: a boss's scripted moment keeps it.)
		if (Movement->IsMovingOnGround() || Movement->IsFalling())
		{
			Movement->SetMovementMode(MOVE_Falling);
		}
		SteerTimer = 0.f;
	}
	return true;
}
