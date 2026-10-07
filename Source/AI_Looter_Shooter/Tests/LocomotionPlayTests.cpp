#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerSlide.h"
#include "Tests/LocomotionTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The player's stances played out in a test level, on the real character (BP_LooterCharacter): the jump key out of a
// crouch, and the slide from its start to each of its ends. The rules alone are tested in LocomotionTests.cpp.

using namespace LocomotionTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJumpWhileCrouchedTest, "Looter.Locomotion.JumpWhileCrouched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FJumpWhileCrouchedTest::RunTest(const FString& Parameters)
{
	// The user's rule: crouched, the jump key stands the player up; a second press jumps. Under something low there's no
	// room to stand, and the press does nothing at all.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	// A level never readied for play drops the actors' own events (AActor::ProcessEvent), and whether a character can
	// jump is one (ACharacter::CanJumpInternal): let them run, as in the game.
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	UPlayerLocomotionComponent* Open = SpawnPlayer(World, FVector::ZeroVector);
	UPlayerLocomotionComponent* Low = SpawnPlayer(World, FVector(800.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("A walker in the open"), Open) || !TestNotNull(TEXT("A walker under a ledge"), Low))
	{
		return false;
	}

	ACharacter* Walker = BodyOf(Open);
	UCharacterMovementComponent* Movement = Walker->GetCharacterMovement();
	CrouchDown(Open);
	TestTrue(TEXT("Crouched"), Walker->bIsCrouched);
	Open->HandleJumpPressed();
	TestFalse(TEXT("Jump while crouched stands up"), Walker->bIsCrouched);
	TestFalse(TEXT("...without jumping"), Walker->bPressedJump || Movement->IsFalling());
	Step(Open);
	TestFalse(TEXT("...and stays up with the crouch key still held"), Movement->bWantsToCrouch);
	Open->HandleJumpPressed();
	Walker->CheckJumpInput(Frame);
	TestTrue(TEXT("The next press jumps"), Movement->IsFalling() && Movement->Velocity.Z > 0.0);

	ACharacter* Under = BodyOf(Low);
	UCharacterMovementComponent* UnderMovement = Under->GetCharacterMovement();
	CrouchDown(Low);
	// Its underside 150 cm up: over the player's crouched capsule (115.6 cm at 0.85), under the standing one (163.2).
	const AStaticMeshActor* Ledge = SpawnBlock(World, FVector(800.0, 0.0, 200.0), FVector(300.0, 300.0, 100.0));
	if (!TestNotNull(TEXT("A ledge"), Ledge) || !TestTrue(TEXT("Crouched under it"), Under->bIsCrouched))
	{
		return false;
	}
	Low->HandleJumpPressed();
	TestTrue(TEXT("No room to stand: still crouched"), Under->bIsCrouched && UnderMovement->bWantsToCrouch);
	TestFalse(TEXT("...and no jump"), Under->bPressedJump || UnderMovement->IsFalling());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerSlideTest, "Looter.Locomotion.Slide.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerSlideTest::RunTest(const FString& Parameters)
{
	// The user's calls: crouch while sprinting slides, 10% faster than the sprint, for a moment. Still running forward at
	// its end goes straight back into the sprint, without a dip; otherwise it ends in the crouch, easing out smoothly.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	// As in the game, the actors' own events run (a level never readied for play drops them: AActor::ProcessEvent).
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	UPlayerLocomotionComponent* Slider = SpawnPlayer(World, FVector::ZeroVector);
	UPlayerLocomotionComponent* Walker = SpawnPlayer(World, FVector(0.0, 1000.0, 0.0));
	UPlayerLocomotionComponent* Jumper = SpawnPlayer(World, FVector(0.0, 2000.0, 0.0));
	UPlayerLocomotionComponent* Leaper = SpawnPlayer(World, FVector(0.0, 3000.0, 0.0));
	UPlayerLocomotionComponent* Runner = SpawnPlayer(World, FVector(0.0, -1000.0, 0.0));
	UPlayerLocomotionComponent* Croucher = SpawnPlayer(World, FVector(0.0, -2000.0, 0.0));
	UPlayerLocomotionComponent* Stopper = SpawnPlayer(World, FVector(0.0, -3000.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !Slider || !Walker || !Jumper || !Leaper || !Runner || !Croucher || !Stopper)
	{
		AddError(TEXT("The players weren't made."));
		return false;
	}

	// Sprint, then crouch: a slide at 1.1x the sprint, along the run, in the crouched capsule. All at the player's speeds:
	// walk 510, sprint 790.5, slide 869.55 cm/s.
	ACharacter* SliderBody = BodyOf(Slider);
	UCharacterMovementComponent* Movement = SliderBody->GetCharacterMovement();
	const float Walk = Movement->MaxWalkSpeed;
	TestEqual(TEXT("The player walks at 85% of 600"), Walk, LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale, 0.01f);
	StartSprint(Slider);
	const float Sprint = static_cast<float>(Movement->Velocity.Size2D());
	const float Top = Sprint * FPlayerSlide::SpeedMultiplier;
	TestEqual(TEXT("Sprinting"), Movement->MaxWalkSpeed, Sprint, 0.5f);
	TestEqual(TEXT("...at 85% of 930"), Movement->MaxWalkSpeed, 930.f * LooterPlayerSize::SpeedScale, 0.5f);
	Slider->HandleCrouchPressed();
	TestTrue(TEXT("Sprint and crouch: a slide"), Slider->IsSliding());
	TestEqual(TEXT("...at 1.1x the sprint"), static_cast<float>(Movement->Velocity.Size2D()), Top, 0.5f);
	TestTrue(TEXT("...along the run"), Movement->Velocity.GetSafeNormal2D().Equals(FVector::ForwardVector, 1.e-3));
	TestTrue(TEXT("...crouched, to fit under things"), Movement->bWantsToCrouch);
	TestTrue(TEXT("...with room to hold that speed"), Movement->MaxWalkSpeedCrouched >= Top - 0.5f);
	// What the next move does with that wish: the crouched capsule, at the player's size.
	Movement->Crouch(false);
	TestTrue(FString::Printf(TEXT("...in the crouched capsule (%.1f cm tall)"), SliderBody->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f),
		SliderBody->bIsCrouched && FMath::IsNearlyEqual(SliderBody->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Slider->CrouchedHalfHeight * LooterPlayerSize::Scale, 0.01f));

	// No movement keys and the crouch key held: it runs out to a stop on its own ease (rather than braking hard after it)
	// and ends in the crouch.
	float Lowest = 0.f;
	float Last = 0.f;
	float Time = RunSlideOut(Slider, Lowest, Last);
	TestTrue(FString::Printf(TEXT("The slide ends on time (%.2f s)"), Time), Slider->GetSlide().GetLastEnd() == FPlayerSlide::EEnd::Time
		&& FMath::IsNearlyEqual(Time, FPlayerSlide::Duration, Frame + 0.001f));
	TestTrue(FString::Printf(TEXT("...no keys: run out to a stop (%.0f cm/s on its last frame)"), Last), Last <= Top * 0.05f);
	TestTrue(TEXT("...in a crouch (the key is still held)"), SliderBody->bIsCrouched && Movement->bWantsToCrouch);
	TestEqual(TEXT("...at the crouched walk"), Movement->MaxWalkSpeedCrouched, Slider->CrouchSpeed, 0.01f);
	Slider->HandleCrouchReleased();
	Step(Slider);
	TestFalse(TEXT("Letting go of crouch afterwards stands up"), Movement->bWantsToCrouch);

	// Still running forward at the end (crouch tapped, Shift let go during the slide): it eases to the sprint, never under
	// it, stands, and the sprint stays on until the player stops running forward.
	ACharacter* RunnerBody = BodyOf(Runner);
	UCharacterMovementComponent* RunnerMovement = RunnerBody->GetCharacterMovement();
	Runner->HandleMoveInput(FVector2D(0.0, 1.0));
	StartSlide(Runner);
	Runner->HandleCrouchReleased();
	Runner->HandleSprintReleased();
	Time = RunSlideOut(Runner, Lowest, Last);
	TestTrue(FString::Printf(TEXT("Forward held: the slide ends on time (%.2f s)"), Time), Runner->GetSlide().GetLastEnd() == FPlayerSlide::EEnd::Time);
	TestTrue(FString::Printf(TEXT("...easing to the sprint, never under it (%.1f cm/s at least)"), Lowest), Lowest >= Sprint - 0.5f);
	TestFalse(TEXT("...standing"), RunnerBody->bIsCrouched || RunnerMovement->bWantsToCrouch);
	TestEqual(TEXT("...straight back into the sprint"), RunnerMovement->MaxWalkSpeed, Sprint, 0.5f);
	for (int32 Frames = 0; Frames < 30; ++Frames)
	{
		Step(Runner);
	}
	TestEqual(TEXT("...which stays on with Shift let go"), RunnerMovement->MaxWalkSpeed, Sprint, 0.5f);
	Runner->HandleMoveInput(FVector2D::ZeroVector);
	for (int32 Frames = 0; Frames < 30; ++Frames)
	{
		Step(Runner);
	}
	TestEqual(TEXT("...until the player stops running forward"), RunnerMovement->MaxWalkSpeed, Walk, 0.5f);

	// Forward held with the crouch key held down (hold mode): it stays in the crouch, easing to the crouched walk.
	ACharacter* CroucherBody = BodyOf(Croucher);
	UCharacterMovementComponent* CroucherMovement = CroucherBody->GetCharacterMovement();
	Croucher->HandleMoveInput(FVector2D(0.0, 1.0));
	StartSlide(Croucher);
	RunSlideOut(Croucher, Lowest, Last);
	TestTrue(TEXT("Forward and the crouch key held: the slide ends in the crouch"), CroucherBody->bIsCrouched && CroucherMovement->bWantsToCrouch);
	TestTrue(FString::Printf(TEXT("...easing to the crouched walk (%.0f cm/s on its last frame)"), Last),
		FMath::IsNearlyEqual(Last, Croucher->CrouchSpeed, (Top - Croucher->CrouchSpeed) * 0.02f));
	TestEqual(TEXT("...and walking on at it"), CroucherMovement->MaxWalkSpeedCrouched, Croucher->CrouchSpeed, 0.01f);
	Step(Croucher);
	TestTrue(TEXT("...not sprinting"), CroucherBody->bIsCrouched && CroucherMovement->MaxWalkSpeed <= Walk + 0.5f);

	// No movement keys, crouch tapped and Shift let go: it runs out to a stop, and with the crouch key up (hold mode) the
	// player stands, as the key says; no sprint.
	ACharacter* StopperBody = BodyOf(Stopper);
	UCharacterMovementComponent* StopperMovement = StopperBody->GetCharacterMovement();
	StartSlide(Stopper);
	Stopper->HandleCrouchReleased();
	Stopper->HandleSprintReleased();
	RunSlideOut(Stopper, Lowest, Last);
	TestTrue(FString::Printf(TEXT("Nothing held: run out to a stop (%.0f cm/s on its last frame)"), Last), Last <= Top * 0.05f);
	Step(Stopper);
	TestFalse(TEXT("...the crouch key up: standing, as it says"), StopperMovement->bWantsToCrouch);
	TestTrue(TEXT("...not sprinting"), StopperMovement->MaxWalkSpeed <= Walk + 0.5f);

	// Walking, then crouch: just a crouch.
	UCharacterMovementComponent* WalkerMovement = BodyOf(Walker)->GetCharacterMovement();
	MoveForward(Walker, Walk);
	Step(Walker);
	Walker->HandleCrouchPressed();
	Step(Walker);
	TestFalse(TEXT("Crouch without a sprint doesn't slide"), Walker->IsSliding());
	TestTrue(TEXT("...it crouches"), WalkerMovement->bWantsToCrouch);
	TestEqual(TEXT("...at the walk it had"), static_cast<float>(WalkerMovement->Velocity.Size2D()), Walk, 0.5f);

	// Jump mid-slide: the slide ends and the player stands, no jump (the crouch-jump rule).
	ACharacter* JumperBody = BodyOf(Jumper);
	StartSlide(Jumper);
	Step(Jumper);
	TestTrue(TEXT("Sliding"), Jumper->IsSliding() && JumperBody->bIsCrouched);
	Jumper->HandleJumpPressed();
	TestFalse(TEXT("Jump mid-slide ends it"), Jumper->IsSliding());
	TestFalse(TEXT("...stands up"), JumperBody->bIsCrouched || JumperBody->GetCharacterMovement()->bWantsToCrouch);
	TestFalse(TEXT("...without jumping"), JumperBody->bPressedJump || JumperBody->GetCharacterMovement()->IsFalling());

	// Sprinting off a ledge, then crouch: no slide in the air.
	StartSprint(Leaper);
	BodyOf(Leaper)->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	Leaper->HandleCrouchPressed();
	TestFalse(TEXT("No slide in the air"), Leaper->IsSliding());
	return true;
}

#endif
