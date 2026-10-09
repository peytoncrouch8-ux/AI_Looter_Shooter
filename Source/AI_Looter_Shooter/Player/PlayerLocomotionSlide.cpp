#include "Player/PlayerLocomotionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Weapons/WeaponCurses.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// UPlayerLocomotionComponent's slide (FPlayerSlide holds its rules), how it ends (back into the sprint with forward
// held, else in the crouch), its dust and sounds, and the jump key, which stands a crouched or sliding player up before
// it jumps (standing, it climbs, vaults or jumps: PlayerLocomotionTraversal.cpp).

namespace
{
	/** Seconds the slide's scrape takes to fade once it's over. */
	constexpr float SlideLoopFadeSeconds = 0.25f;

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
	if (!Owner || !Move || Traversal.IsActive())
	{
		// Mid-climb the key does nothing: a press mashed through it would hop the player off the top as it lands.
		return;
	}
	if (!Owner->bIsCrouched && !Slide.IsActive())
	{
		// Standing: a ledge or fence in front is climbed or vaulted, else it's a jump (PlayerLocomotionTraversal.cpp).
		JumpOrTraverse();
		return;
	}

	// Crouched or sliding (the user's rule): the press stands the player up instead of jumping, in either crouch mode, and
	// the next press jumps. Stand at once, through the engine's own headroom test: under something low it can't, and the
	// press does nothing at all (a slide carries on).
	if (!StandUpNow())
	{
		UE_LOG(LogLooter, Verbose, TEXT("Jump while crouched: no room to stand."));
		return;
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
	bSlideJustStarted = true;

	// The rush of cloth and grit as the body drops, then the scrape along the ground for as long as it runs.
	LooterSound::PlayAttached(LooterSoundCue::Slide, Owner->GetRootComponent());
	LooterSound::Stop(SlideLoop.Get());
	SlideLoop = LooterSound::Start(this, LooterSoundCue::SlideLoop, Owner->GetRootComponent());
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
	LooterSound::Stop(SlideLoop.Get(), SlideLoopFadeSeconds);
	SlideLoop.Reset();
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
	// input is the slide's own line. A Cold iron taken in hand mid-slide allows no sprint to go back to.
	const bool bCrouchHeld = Intent.IsHeldMode(FStanceIntent::EStance::Crouch) && Intent.IsActive(FStanceIntent::EStance::Crouch);
	return bHasMoveInput && IsMovingForward() && !bCrouchHeld && !WeaponCurses::BlocksSprint(GetOwner());
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
	// the keys decide from there as after any slide. (The view doesn't stand at once: the eye rises on its curve.)
	if (!StandUpNow())
	{
		UE_LOG(LogLooter, Verbose, TEXT("Slide over under something low: stays crouched."));
		return;
	}
	// Back into the sprint as if its key were down (a toggled crouch cleared), the movement's crouch wish too, and the
	// sprint's speed for this frame's move already: the slide eased to it, so there's no dip.
	Intent.ResumeSprint();
	Owner->UnCrouch();
	bSprinting = true;
	Move->MaxWalkSpeed = BaseWalkSpeed * SprintSpeedMultiplier;
	UE_LOG(LogLooter, Verbose, TEXT("Slide over: back into the sprint."));
}

bool UPlayerLocomotionComponent::StandUpNow()
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	if (!Owner || !Move || !Owner->bIsCrouched)
	{
		return Owner != nullptr;
	}
	const double FeetBefore = GetFeetHeight();
	Move->UnCrouch(false);
	if (Owner->bIsCrouched)
	{
		return false;
	}
	// On the ground the engine keeps the feet where they were; in the air it stands up about the capsule's middle, so the
	// feet drop under the eye. The eye takes that back (UpdateCamera), and it's noted here, where it happened, since this
	// can run after the movement has (the end of a slide) or before it (the jump key).
	PendingFeetJump += static_cast<float>(GetFeetHeight() - FeetBefore);
	LastHalfHeight = Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	RefreshBodyTransform();
	return true;
}

double UPlayerLocomotionComponent::GetFeetHeight() const
{
	const ACharacter* Owner = Character.Get();
	return Owner ? Owner->GetActorLocation().Z - Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.0;
}

void UPlayerLocomotionComponent::UpdateDust(float DeltaTime)
{
	// Thrown at the speed the slide holds (it eases to the exit speed at its end), or what the ground lets it make if less.
	const ACharacter* Owner = Character.Get();
	const UCharacterMovementComponent* Move = Movement.Get();
	const bool bSliding = Slide.IsActive() && Move->IsMovingOnGround();
	const float Speed = FMath::Min(Slide.GetSpeed(), static_cast<float>(Move->Velocity.Size2D()));
	Dust.Update(*Owner, bSliding, bSlideJustStarted, Slide.GetDirection(), Speed, Slide.GetTopSpeed(), DeltaTime);
	bSlideJustStarted = false;
}
