#include "Player/PlayerLocomotionComponent.h"
#include "AI_Looter_Shooter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// UPlayerLocomotionComponent's slide (FPlayerSlide holds its rules), how it ends (back into the sprint with forward
// held, else in the crouch), and the jump key, which stands a crouched or sliding player up before it jumps.

namespace
{
	const TCHAR* SlideEndName(FPlayerSlide::EEnd End)
	{
		switch (End)
		{
		case FPlayerSlide::EEnd::Time: return TEXT("on time");
		case FPlayerSlide::EEnd::Stalled: return TEXT("against something");
		case FPlayerSlide::EEnd::Airborne: return TEXT("off the ground");
		case FPlayerSlide::EEnd::Cancelled: return TEXT("cancelled");
		default: return TEXT("-");
		}
	}
}

void UPlayerLocomotionComponent::HandleJumpPressed()
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	if (!Owner || !Move)
	{
		return;
	}
	if (!Owner->bIsCrouched && !Slide.IsActive())
	{
		Owner->Jump();
		return;
	}

	// Crouched or sliding (the user's rule): the press stands the player up instead of jumping, in either crouch mode, and
	// the next press jumps. Stand at once, through the engine's own headroom test: under something low it can't, and the
	// press does nothing at all (a slide carries on).
	if (Owner->bIsCrouched)
	{
		Move->UnCrouch(false);
		if (Owner->bIsCrouched)
		{
			UE_LOG(LogLooter, Verbose, TEXT("Jump while crouched: no room to stand."));
			return;
		}
	}
	EndSlide();
	Intent.StandUp();
	// The movement's own wish too, or it would crouch again on its next move.
	Owner->UnCrouch();
	UE_LOG(LogLooter, Verbose, TEXT("Jump while crouched: stood up."));
}

bool UPlayerLocomotionComponent::TryStartSlide()
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	if (!Owner || !Move || Slide.IsActive() || !FPlayerSlide::CanStart(bSprinting, Move->IsMovingOnGround()))
	{
		return false;
	}

	// The slide holds 10% over the full sprint (whatever slows the player scales that down as it does the sprint), and
	// at its end eases to the speed the player goes on at (UpdateSlide).
	const float TopSpeed = BaseWalkSpeed * SprintSpeedMultiplier * FPlayerSlide::SpeedMultiplier;
	Slide.Start(Move->Velocity, Owner->GetActorForwardVector(), TopSpeed);
	bSlideHoldsSpeed = true;
	bSlideExitsToSprint = false;
	bSprinting = false;

	// Straight into it, for this frame's move already: the crouched capsule, the slide's speed and its line.
	Owner->Crouch();
	Move->MaxWalkSpeedCrouched = Slide.GetSpeed();
	Move->Velocity = Slide.GetDirection() * Slide.GetStartSpeed();
	Owner->AddMovementInput(Slide.GetDirection());
	UE_LOG(LogLooter, Verbose, TEXT("Slide at %.0f cm/s along %s"), Slide.GetStartSpeed(), *Slide.GetDirection().ToCompactString());
	return true;
}

void UPlayerLocomotionComponent::UpdateSlide(float DeltaTime)
{
	if (!Slide.IsActive())
	{
		return;
	}
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	// Read the keys every frame: the ease toward the slide's end follows what the player will do once it's over.
	bSlideExitsToSprint = SlideEndsInSprint();
	if (!Slide.Advance(DeltaTime, static_cast<float>(Move->Velocity.Size2D()), Move->IsMovingOnGround(), GetSlideExitSpeed()))
	{
		const bool bToSprint = bSlideExitsToSprint;
		EndSlide();
		if (bToSprint)
		{
			SprintOutOfSlide();
		}
		return;
	}

	// Hold the slide's speed and line for the next move. Its input stands in for the movement keys, which the character
	// ignores while it slides (ALooterCharacter::Move).
	const float Speed = Slide.GetSpeed();
	if (!FMath::IsNearlyEqual(Move->MaxWalkSpeedCrouched, Speed))
	{
		Move->MaxWalkSpeedCrouched = Speed;
	}
	Owner->AddMovementInput(Slide.GetDirection());
}

void UPlayerLocomotionComponent::EndSlide()
{
	if (!Slide.IsActive() && !bSlideHoldsSpeed)
	{
		return;
	}
	Slide.Stop();
	bSlideHoldsSpeed = false;
	bSlideExitsToSprint = false;
	// The crouched walk's own speed back; the crouch keys decide from here whether the player stays down.
	if (UCharacterMovementComponent* Move = Movement.Get())
	{
		Move->MaxWalkSpeedCrouched = CrouchSpeed;
	}
	UE_LOG(LogLooter, Verbose, TEXT("Slide over %s after %.2f s"), SlideEndName(Slide.GetLastEnd()), Slide.GetElapsed());
}

bool UPlayerLocomotionComponent::SlideEndsInSprint() const
{
	// The user's rule: still running forward at the end goes back into the sprint, unless the crouch key is held down
	// (hold mode), which keeps the crouch. Only the keys the character passes on count: through a slide the last move's
	// input is the slide's own line.
	const bool bCrouchHeld = Intent.IsHeldMode(FStanceIntent::EStance::Crouch) && Intent.IsActive(FStanceIntent::EStance::Crouch);
	return bHasMoveInput && IsMovingForward() && !bCrouchHeld;
}

float UPlayerLocomotionComponent::GetSlideExitSpeed() const
{
	if (SlideEndsInSprint())
	{
		return BaseWalkSpeed * SprintSpeedMultiplier;
	}
	// On into the crouched walk with the keys down. With none, the slide runs out to a stop on its own ease: ending it at
	// the crouched walk left the ground's braking to stop it hard just after (the old abrupt stop).
	return GetHeldMoveInput().IsNearlyZero() ? 0.f : CrouchSpeed;
}

void UPlayerLocomotionComponent::SprintOutOfSlide()
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	if (!Owner || !Move)
	{
		return;
	}
	// Stand at once, through the engine's headroom test. Under something low the crouch stays, at the crouched walk, and
	// the keys decide from there as after any slide.
	if (Owner->bIsCrouched)
	{
		Move->UnCrouch(false);
		if (Owner->bIsCrouched)
		{
			UE_LOG(LogLooter, Verbose, TEXT("Slide over under something low: stays crouched."));
			return;
		}
	}
	// Back into the sprint as if its key were down (a toggled crouch cleared), the movement's crouch wish too, and the
	// sprint's speed for this frame's move already: the slide eased to it, so there's no dip.
	Intent.ResumeSprint();
	Owner->UnCrouch();
	bSprinting = true;
	Move->MaxWalkSpeed = BaseWalkSpeed * SprintSpeedMultiplier;
	UE_LOG(LogLooter, Verbose, TEXT("Slide over: back into the sprint."));
}
