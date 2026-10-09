#include "Player/PlayerLocomotionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// UPlayerLocomotionComponent's forgiveness: the jump key still jumps a moment after walking off an edge (coyote time), a
// press just before landing jumps on landing (the buffer), and a player hung in the air on rocks or props is nudged free.
// All on the component's own clock, which the test levels can drive.

bool UPlayerLocomotionComponent::IsMovementHeld() const
{
	const ACharacter* Owner = Character.Get();
	const AController* Controller = Owner ? Owner->GetController() : nullptr;
	return !Owner || Owner->GetAttachParentActor() || (Controller && Controller->IsMoveInputIgnored());
}

bool UPlayerLocomotionComponent::CanCoyoteJump() const
{
	const ACharacter* Owner = Character.Get();
	const UCharacterMovementComponent* Move = Movement.Get();
	return Owner && Move && Move->IsFalling() && bLeftByWalking && !Owner->bIsCrouched
		&& Clock - LeftGroundClock <= LooterTraversal::CoyoteSeconds;
}

void UPlayerLocomotionComponent::UpdateJumpAssist()
{
	const ACharacter* Owner = Character.Get();
	const UCharacterMovementComponent* Move = Movement.Get();
	if (Move->IsMovingOnGround())
	{
		LastFloorZ = static_cast<float>(GetFeetHeight());
		bHaveFloor = true;
		bLeftByWalking = false;
		const bool bJustLanded = !bWasGrounded;
		bWasGrounded = true;
		// A press from just before the landing: the jump (or the climb in front) goes now.
		if (bJustLanded && Clock - JumpBufferedClock <= LooterTraversal::JumpBufferSeconds)
		{
			JumpBufferedClock = -100.0;
			UE_LOG(LogLooter, Verbose, TEXT("Jump held from before the landing."));
			HandleJumpPressed();
		}
		return;
	}
	if (bWasGrounded)
	{
		// Just left the ground: walked off an edge (not a jump or a launch), so the key still jumps for a moment.
		LeftGroundClock = Clock;
		bLeftByWalking = Move->IsFalling() && Move->Velocity.Z < 100.0 && Owner->JumpCurrentCount == 0;
	}
	bWasGrounded = false;
}

void UPlayerLocomotionComponent::CoyoteJump()
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	// The jump the ground would have given at the edge just walked off: the full lift, whatever the fall so far. As the
	// engine's own jump does: counted (no second one in the air), and told (its sound).
	bLeftByWalking = false;
	Move->Velocity.Z = FMath::Max(Move->Velocity.Z, static_cast<double>(Move->JumpZVelocity));
	Move->SetMovementMode(MOVE_Falling);
	Owner->JumpCurrentCount = FMath::Max(Owner->JumpCurrentCount, 1);
	Owner->OnJumped();
	// The gun lags as the body goes up, as with any jump (UpdateViewModel only sees jumps from the ground).
	KickVelocity -= 18.f;
	UE_LOG(LogLooter, Verbose, TEXT("Jump %.2f s after walking off an edge."), Clock - LeftGroundClock);
}

void UPlayerLocomotionComponent::UpdateStuck()
{
	ACharacter* Owner = Character.Get();
	const UCharacterMovementComponent* Move = Movement.Get();
	const FVector Here = Owner->GetActorLocation();
	// Only hanging in the air counts (balanced on rocks, caught between props or on a creature); pressing into a wall
	// while walking is the player's own doing, and a scene holding the player is the scene's.
	if (!Move->IsFalling() || IsMovementHeld() || FVector::DistSquared(Here, StuckAnchor) > FMath::Square(LooterTraversal::StuckRadius))
	{
		StuckAnchor = Here;
		StuckSinceClock = Clock;
		return;
	}
	if (Clock - StuckSinceClock < LooterTraversal::StuckSeconds)
	{
		return;
	}
	StuckAnchor = Here;
	StuckSinceClock = Clock;
	FVector Spot;
	if (!FTraversalProbe::FindFreeSpot(*Owner, GetTraversalHeading(), Spot))
	{
		UE_LOG(LogLooter, Verbose, TEXT("Hung in the air at %s, with no free spot near."), *Here.ToCompactString());
		return;
	}
	// A short glide there (an unstick traversal), not a pop; the fall takes it the last little way.
	FTraversalFind Free;
	Free.Kind = ETraversalKind::Unstick;
	Free.Refusal = ETraversalRefusal::None;
	const FVector Away(Spot.X - Here.X, Spot.Y - Here.Y, 0.0);
	Free.Direction = Away.IsNearlyZero() ? Owner->GetActorForwardVector() : Away.GetSafeNormal();
	Free.End = Spot;
	Free.EndFeetZ = static_cast<float>(Spot.Z - Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	Free.TopZ = Free.EndFeetZ;
	Free.Duration = LooterTraversal::UnstickSeconds;
	UE_LOG(LogLooter, Log, TEXT("Player wedged at %s: nudged free to %s."), *Here.ToCompactString(), *Spot.ToCompactString());
	StartTraversal(Free);
}
