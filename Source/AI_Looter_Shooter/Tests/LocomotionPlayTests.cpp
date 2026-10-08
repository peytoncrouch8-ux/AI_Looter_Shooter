#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerSlide.h"
#include "Player/SlideDust.h"
#include "Tests/LocomotionTestWorld.h"
#include "Combat/BulletSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The player's stances played out in a test level, on the real character (BP_LooterCharacter): the jump key out of a
// crouch, the slide from its start to each of its ends, the first-person view through each (no pop, no jolt), and the
// slide's dust on different ground. The rules alone are tested in LocomotionTests.cpp.

using namespace LocomotionTestWorld;

namespace
{
	/**
	 * The view's limits at 60 fps. The camera moves at most 8 cm in a frame (480 cm/s): the slide's 57 cm drop on its
	 * quarter-second curve tops out near 6.4, while the pops this test was written for moved 24 cm in one frame. That move
	 * changes by at most 1.5 cm from one frame to the next (5400 cm/s^2): the eased curves stay under 1.25, while the old
	 * blends started a slide with a 3.5 cm change and a stance turning back mid-way flipped it by about 10. The roll moves
	 * at most 0.6 degrees a frame (its 5 degrees ease in over 0.3 s, topping out near 0.5) and changes by 0.15 at most.
	 */
	constexpr float ViewStepLimit = 8.f;
	constexpr float ViewStepChangeLimit = 1.5f;
	constexpr float RollStepLimit = 0.6f;
	constexpr float RollStepChangeLimit = 0.15f;
	/** Frames each ending is followed for after the slide starts: the slide's 0.8 s and the eye settling after it. */
	constexpr int32 FramesToFollow = 110;

	void CheckView(FAutomationTestBase& Test, const TCHAR* Name, const FViewTrack& Track, const UPlayerLocomotionComponent* Locomotion,
		float WantedEye)
	{
		Test.TestTrue(FString::Printf(TEXT("%s: %d frames of the view followed"), Name, Track.Frames), Track.Frames >= FramesToFollow);
		Test.TestTrue(FString::Printf(TEXT("%s: no pop (at most %.2f cm in a frame, frame %d)"), Name, Track.MaxStep, Track.WorstStepFrame),
			Track.MaxStep <= ViewStepLimit);
		Test.TestTrue(FString::Printf(TEXT("%s: no jolt (a frame's move changed by at most %.2f cm, frame %d)"), Name, Track.MaxStepChange,
			Track.WorstChangeFrame), Track.MaxStepChange <= ViewStepChangeLimit);
		Test.TestTrue(FString::Printf(TEXT("%s: the roll eases (at most %.3f degrees in a frame, changing by %.3f)"), Name, Track.MaxRollStep,
			Track.MaxRollStepChange), Track.MaxRollStep <= RollStepLimit && Track.MaxRollStepChange <= RollStepChangeLimit);
		const float Eye = EyeAboveFeet(Locomotion);
		Test.TestTrue(FString::Printf(TEXT("%s: the eye ends where it belongs (%.2f cm above the feet, wants %.2f)"), Name, Eye, WantedEye),
			FMath::IsNearlyEqual(Eye, WantedEye, 0.5f));
		Test.TestTrue(FString::Printf(TEXT("%s: ...level again"), Name), FMath::IsNearlyZero(Locomotion->GetViewRoll(), 0.02f));
	}

	/** Frames standing still, so the eye has its standing height; returns it. */
	float Settle(UPlayerLocomotionComponent* Locomotion)
	{
		for (int32 Frames = 0; Frames < 10; ++Frames)
		{
			PlayFrame(Locomotion);
		}
		return EyeAboveFeet(Locomotion);
	}
}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlideViewTest, "Looter.Locomotion.Slide.View",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlideViewTest::RunTest(const FString& Parameters)
{
	// The user's report (2026-10-08): through a slide "the view jerks or snaps". The first-person camera's height in the
	// world and its roll, sampled every frame at 60 fps from standing through the slide's start, its run and each way it
	// ends, never pop or jolt (the limits above), and the eye ends where a stand or a crouch puts it. Each frame plays as
	// the game does: the movement's crouch or stand first, then the locomotion (PlayFrame).
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	// Two ledges of their own, away from the floor: taken away mid-slide, they leave the slider in the air.
	AStaticMeshActor* Ledges[] = { SpawnBlock(World, FVector(8000.0, 0.0, -50.0), FVector(600.0, 600.0, 100.0)),
		SpawnBlock(World, FVector(8000.0, 1500.0, -50.0), FVector(600.0, 600.0, 100.0)) };
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("A ledge"), Ledges[0]) || !TestNotNull(TEXT("Another ledge"), Ledges[1]))
	{
		return false;
	}

	enum class EEnding : uint8 { IntoSprint, IntoCrouch, ToStop, JumpOut, Wall, OffLedge, OffLedgeRunning };
	struct FCase
	{
		const TCHAR* Name;
		EEnding Ending;
		FVector Feet;
		/** It ends standing (else crouched), and how the slide itself ended. */
		bool bEndsStanding;
		FPlayerSlide::EEnd End;
	};
	const FCase Cases[] = {
		{ TEXT("Into the sprint (forward held)"), EEnding::IntoSprint, FVector(0.0, -2000.0, 0.0), true, FPlayerSlide::EEnd::Time },
		{ TEXT("Into the crouch (forward and crouch held)"), EEnding::IntoCrouch, FVector(0.0, -1000.0, 0.0), false, FPlayerSlide::EEnd::Time },
		{ TEXT("To a stop, then up (nothing held)"), EEnding::ToStop, FVector::ZeroVector, true, FPlayerSlide::EEnd::Time },
		{ TEXT("Jumped out while still dropping"), EEnding::JumpOut, FVector(0.0, 1000.0, 0.0), true, FPlayerSlide::EEnd::Cancelled },
		{ TEXT("Into a wall (crouch held)"), EEnding::Wall, FVector(0.0, 2000.0, 0.0), false, FPlayerSlide::EEnd::Stalled },
		{ TEXT("Off a ledge (nothing held)"), EEnding::OffLedge, FVector(8000.0, 0.0, 0.0), true, FPlayerSlide::EEnd::Airborne },
		{ TEXT("Off a ledge running (forward held)"), EEnding::OffLedgeRunning, FVector(8000.0, 1500.0, 0.0), true, FPlayerSlide::EEnd::Airborne },
	};
	for (const FCase& Case : Cases)
	{
		UPlayerLocomotionComponent* Slider = SpawnPlayer(World, Case.Feet);
		if (!TestNotNull(FString::Printf(TEXT("%s: a player"), Case.Name), Slider)
			|| !TestNotNull(FString::Printf(TEXT("%s: its first-person camera"), Case.Name), CameraOf(Slider)))
		{
			continue;
		}
		ACharacter* Body = BodyOf(Slider);
		UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
		const float Standing = Settle(Slider);
		const float Crouched = Slider->GetCrouchedHeadHeight() * static_cast<float>(Body->GetActorScale3D().Z);
		const bool bForward = Case.Ending == EEnding::IntoSprint || Case.Ending == EEnding::IntoCrouch || Case.Ending == EEnding::OffLedgeRunning;
		const bool bCrouchHeld = Case.Ending == EEnding::IntoCrouch || Case.Ending == EEnding::Wall;
		AStaticMeshActor* Ledge = Case.Ending == EEnding::OffLedge ? Ledges[0] : (Case.Ending == EEnding::OffLedgeRunning ? Ledges[1] : nullptr);

		FViewTrack Track;
		Track.Sample(Slider);
		if (bForward)
		{
			Slider->HandleMoveInput(FVector2D(0.0, 1.0));
		}
		StartSprint(Slider);
		Track.Sample(Slider);
		Slider->HandleCrouchPressed();
		TestTrue(FString::Printf(TEXT("%s: sliding"), Case.Name), Slider->IsSliding());
		if (!bCrouchHeld)
		{
			Slider->HandleCrouchReleased();
		}
		Slider->HandleSprintReleased();
		for (int32 FrameIndex = 0; FrameIndex < FramesToFollow; ++FrameIndex)
		{
			if (bForward)
			{
				Slider->HandleMoveInput(FVector2D(0.0, 1.0));
			}
			if (FrameIndex == 6 && Case.Ending == EEnding::JumpOut)
			{
				Slider->HandleJumpPressed();
			}
			if (FrameIndex == 20 && Case.Ending == EEnding::Wall)
			{
				// Ran into something: the slide moved nowhere this frame.
				Movement->Velocity = FVector::ZeroVector;
			}
			if (FrameIndex == 20 && Ledge)
			{
				// Slid off the edge: nothing under the feet, falling.
				Ledge->Destroy();
				Movement->SetMovementMode(MOVE_Falling);
			}
			PlayFrame(Slider);
			Track.Sample(Slider);
		}

		TestTrue(FString::Printf(TEXT("%s: the slide ended as it should"), Case.Name), !Slider->IsSliding() && Slider->GetSlide().GetLastEnd() == Case.End);
		TestTrue(FString::Printf(TEXT("%s: %s"), Case.Name, Case.bEndsStanding ? TEXT("standing") : TEXT("crouched")),
			static_cast<bool>(Body->bIsCrouched) != Case.bEndsStanding);
		CheckView(*this, Case.Name, Track, Slider, Case.bEndsStanding ? Standing : Crouched);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlideDustTest, "Looter.Locomotion.Slide.Dust",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlideDustTest::RunTest(const FString& Parameters)
{
	// The user's call (2026-10-08): a slide kicks up dust and grit from the feet, thinning as it slows; none on water, a
	// little on wood. Ground nothing names (the engine cube's own material) counts as dirt.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();

	struct FGround
	{
		const TCHAR* Name;
		const TCHAR* Material;
		ESlideGround Expected;
		FVector Feet;
	};
	const FGround Grounds[] = {
		{ TEXT("Dirt"), nullptr, ESlideGround::Dirt, FVector::ZeroVector },
		{ TEXT("Water"), TEXT("/Game/Art/Materials/MI_Water.MI_Water"), ESlideGround::Water, FVector(0.0, 2000.0, 0.0) },
		{ TEXT("Wood"), TEXT("/Game/Art/Materials/MI_WoodPlanks.MI_WoodPlanks"), ESlideGround::Wood, FVector(0.0, 4000.0, 0.0) },
	};
	int32 Puffs[UE_ARRAY_COUNT(Grounds)] = {};
	int32 Grit[UE_ARRAY_COUNT(Grounds)] = {};
	int32 FirstPuffs = 0;
	int32 LastPuffs = 0;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Grounds); ++Index)
	{
		const FGround& Ground = Grounds[Index];
		AStaticMeshActor* Block = SpawnBlock(World, Ground.Feet - FVector(0.0, 0.0, 50.0), FVector(1500.0, 1500.0, 100.0));
		if (!TestNotNull(FString::Printf(TEXT("%s: the ground"), Ground.Name), Block))
		{
			return false;
		}
		if (Ground.Material)
		{
			UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, Ground.Material);
			if (!TestNotNull(FString::Printf(TEXT("%s: its material %s"), Ground.Name, Ground.Material), Material))
			{
				continue;
			}
			Block->GetStaticMeshComponent()->SetMaterial(0, Material);
		}
		UPlayerLocomotionComponent* Slider = SpawnPlayer(World, Ground.Feet);
		if (!TestNotNull(FString::Printf(TEXT("%s: a player"), Ground.Name), Slider))
		{
			continue;
		}
		// No keys: the slide runs its full time and eases out to a stop.
		StartSprint(Slider);
		Slider->HandleCrouchPressed();
		Slider->HandleCrouchReleased();
		Slider->HandleSprintReleased();
		const int32 EaseStartFrame = FMath::FloorToInt32((FPlayerSlide::Duration - FPlayerSlide::EaseOutTime) / Frame);
		const int32 FirstFrames = FMath::FloorToInt32(FPlayerSlide::EaseOutTime / Frame);
		int32 AtEaseStart = 0;
		for (int32 FrameIndex = 0; Slider->IsSliding() && FrameIndex < 120; ++FrameIndex)
		{
			PlayFrame(Slider);
			if (FrameIndex + 1 == FirstFrames && Index == 0)
			{
				FirstPuffs = Slider->GetDust().GetPuffsThrown();
			}
			if (FrameIndex + 1 == EaseStartFrame)
			{
				AtEaseStart = Slider->GetDust().GetPuffsThrown();
			}
		}
		Puffs[Index] = Slider->GetDust().GetPuffsThrown();
		Grit[Index] = Slider->GetDust().GetGritThrown();
		if (Index == 0)
		{
			LastPuffs = Puffs[Index] - AtEaseStart;
		}
		TestTrue(FString::Printf(TEXT("%s: the slide knows its ground"), Ground.Name), Slider->GetDust().GetGround() == Ground.Expected);
		AddInfo(FString::Printf(TEXT("%s: %d puffs, %d grains"), Ground.Name, Puffs[Index], Grit[Index]));
	}

	TestTrue(FString::Printf(TEXT("Dirt: dust kicked up (%d puffs)"), Puffs[0]), Puffs[0] >= 15);
	TestTrue(FString::Printf(TEXT("Dirt: grit thrown (%d grains)"), Grit[0]), Grit[0] >= 10);
	TestTrue(FString::Printf(TEXT("Dirt: it thins as the slide slows (%d puffs in its first %.1f s, %d in its last)"), FirstPuffs,
		FPlayerSlide::EaseOutTime, LastPuffs), LastPuffs * 2 < FirstPuffs);
	TestTrue(FString::Printf(TEXT("Water: nothing (%d puffs, %d grains)"), Puffs[1], Grit[1]), Puffs[1] == 0 && Grit[1] == 0);
	TestTrue(FString::Printf(TEXT("Wood: a little dust (%d puffs against dirt's %d), no grit"), Puffs[2], Puffs[0]),
		Puffs[2] > 0 && Puffs[2] * 2 < Puffs[0] && Grit[2] == 0);
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	TestTrue(TEXT("Drawn by the pooled effects"), Bullets && Bullets->GetEffects().NumParticles() > 0);
	return true;
}

#endif
