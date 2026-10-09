#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/BlendSpace.h"
#include "Player/PlayerSize.h"
#include "Tests/AutomationCommon.h"
#include "Tests/PlayerAnimPlayKit.h"
#include "UObject/Script.h"

// The player's feet against the ground, measured on the real character with the real Anim Blueprints and the real movement:
// a planted foot has to travel backward under the body as fast as the body goes forward, or it slides. The climbing pose
// played out the same way: PlayerAnimClimbPlayTests.cpp. The pose without a world: PlayerAnimTests.cpp.

using namespace LocomotionTestWorld;
using namespace PlayerAnimPlayKit;

namespace
{
	/** How far a planted foot's travel may be from the body's before the feet are called sliding (a share of the ground speed). */
	constexpr float PaceTolerance = 0.2f;
	/** A foot is planted while it is this close (mesh cm) to the lowest it gets. */
	constexpr double PlantedBand = 3.0;

	struct FPace
	{
		/** The body's speed over the ground, and the planted feet's travel under it (both world cm/s). */
		float Ground = 0.f;
		float Foot = 0.f;
		/** How far the capsule went (cm), and the animation rate the locomotion component ended on. */
		float Travelled = 0.f;
		float Rate = 0.f;
		int32 PlantedFrames = 0;
	};

	/** The middle (median) speed a foot travels backward along Forward in the frames it is planted in; 0 if it never was. */
	float PlantedSpeed(const TArray<FVector>& Foot, const FVector& Forward, double Floor, int32& OutFrames)
	{
		TArray<float> Speeds;
		for (int32 Index = 1; Index < Foot.Num(); ++Index)
		{
			if (Foot[Index].Z <= Floor + PlantedBand && Foot[Index - 1].Z <= Floor + PlantedBand)
			{
				Speeds.Add(static_cast<float>(-FVector::DotProduct(Foot[Index] - Foot[Index - 1], Forward) / Frame));
			}
		}
		OutFrames += Speeds.Num();
		if (Speeds.IsEmpty())
		{
			return 0.f;
		}
		Speeds.Sort();
		return Speeds[Speeds.Num() / 2];
	}

	/**
	 * Runs the player forward with the movement ticking as the game's does (the keys' push, the movement, the locomotion,
	 * the animation), Warm frames to get up to speed, then Frames while the feet are followed.
	 */
	FPace MeasurePace(UPlayerLocomotionComponent* Locomotion, int32 Warm, int32 Frames)
	{
		ACharacter* Body = BodyOf(Locomotion);
		UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
		USkeletalMeshComponent* Mesh = Body->GetMesh();
		const FVector Start = Body->GetActorLocation();
		TArray<FVector> Left;
		TArray<FVector> Right;
		double GroundSum = 0.0;
		for (int32 Index = 0; Index < Warm + Frames; ++Index)
		{
			Locomotion->HandleMoveInput(FVector2D(0.0, 1.0));
			Body->AddMovementInput(Body->GetActorForwardVector(), 1.f);
			static_cast<UActorComponent*>(Movement)->TickComponent(Frame, LEVELTICK_All, nullptr);
			Step(Locomotion);
			PlayAnimation(Locomotion);
			if (Index >= Warm)
			{
				GroundSum += Movement->Velocity.Size2D();
				Left.Add(InMesh(Locomotion, TEXT("foot_l")));
				Right.Add(InMesh(Locomotion, TEXT("foot_r")));
			}
		}

		FPace Pace;
		Pace.Ground = static_cast<float>(GroundSum / FMath::Max(Frames, 1));
		Pace.Travelled = static_cast<float>(FVector::Dist2D(Body->GetActorLocation(), Start));
		Pace.Rate = Mesh->GlobalAnimRateScale;
		double Floor = UE_BIG_NUMBER;
		for (const FVector& Foot : Left)
		{
			Floor = FMath::Min(Floor, Foot.Z);
		}
		for (const FVector& Foot : Right)
		{
			Floor = FMath::Min(Floor, Foot.Z);
		}
		const FVector Forward = Mesh->GetComponentTransform().InverseTransformVectorNoScale(Body->GetActorForwardVector()).GetSafeNormal();
		const float Scale = static_cast<float>(Mesh->GetComponentScale().Z);
		int32 LeftFrames = 0;
		int32 RightFrames = 0;
		const float LeftSpeed = PlantedSpeed(Left, Forward, Floor, LeftFrames);
		const float RightSpeed = PlantedSpeed(Right, Forward, Floor, RightFrames);
		Pace.PlantedFrames = LeftFrames + RightFrames;
		const int32 Sides = (LeftFrames > 0 ? 1 : 0) + (RightFrames > 0 ? 1 : 0);
		Pace.Foot = Sides > 0 ? (LeftSpeed * (LeftFrames > 0 ? 1.f : 0.f) + RightSpeed * (RightFrames > 0 ? 1.f : 0.f)) / Sides * Scale : 0.f;
		return Pace;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerAnimFootPaceTest, "Looter.PlayerAnim.FootPace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerAnimFootPaceTest::RunTest(const FString& Parameters)
{
	// The user's report (2026-10-08): feet that slide. The locomotion blend spaces top out at the jog (600 cm/s, the full-size
	// walk); the player walks at that, scaled to its size, and sprints 1.55 times faster than any clip there, so the body's
	// animation is played faster to match (UPlayerLocomotionComponent::UpdateAlphas). Walking, crouch-walking and sprinting
	// are followed here with the real Anim Blueprints: the planted feet must travel as fast as the ground goes by.
	const UBlendSpace* Space = LoadObject<UBlendSpace>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run.BS_Idle_Walk_Run"));
	if (TestNotNull(TEXT("The locomotion blend space"), Space))
	{
		float TopRow = 0.f;
		for (const FBlendSample& Sample : Space->GetBlendSamples())
		{
			TopRow = FMath::Max(TopRow, static_cast<float>(Sample.SampleValue.Y));
		}
		TestTrue(FString::Printf(TEXT("The blend space's fastest clip is the full-size walk (%.0f cm/s)"), TopRow),
			FMath::IsNearlyEqual(TopRow, LooterPlayerSize::FullSizeWalkSpeed, 1.f));
	}

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(12000.0, 12000.0, 100.0));
	if (!TestNotNull(TEXT("A floor"), Floor))
	{
		return false;
	}
	const UClass* ArmedClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Rifle/ABP_Rifle.ABP_Rifle_C"));

	struct FScenario
	{
		const TCHAR* Name;
		bool bSprint;
		bool bCrouch;
		bool bArmed;
	};
	const FScenario Scenarios[] = {
		{ TEXT("Walking"), false, false, false },
		{ TEXT("Crouch-walking"), false, true, false },
		{ TEXT("Sprinting"), true, false, false },
		{ TEXT("Sprinting, rifle in hand"), true, false, true },
	};
	int32 Row = 0;
	for (const FScenario& Scenario : Scenarios)
	{
		UPlayerLocomotionComponent* Locomotion = SpawnPlayer(World, FVector(0.0, -4500.0 + 1500.0 * Row++, 0.0));
		if (!TestNotNull(FString::Printf(TEXT("%s: a player"), Scenario.Name), Locomotion))
		{
			continue;
		}
		ACharacter* Body = BodyOf(Locomotion);
		UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
		// The test level never ticks, so the movement is ticked here; with no controller it only runs when told to.
		Movement->bRunPhysicsWithNoController = true;
		if (Scenario.bArmed)
		{
			if (!ArmedClass)
			{
				AddWarning(TEXT("The rifle Anim Blueprint isn't there: the armed sprint isn't measured."));
				continue;
			}
			Body->GetMesh()->SetAnimInstanceClass(const_cast<UClass*>(ArmedClass));
		}
		if (Scenario.bSprint)
		{
			Locomotion->HandleSprintPressed();
		}
		if (Scenario.bCrouch)
		{
			Locomotion->HandleCrouchPressed();
		}

		const float Walk = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale;
		const float Wanted = Scenario.bCrouch ? LooterPlayerSize::FullSizeCrouchSpeed * LooterPlayerSize::SpeedScale
			: (Scenario.bSprint ? Walk * Locomotion->SprintSpeedMultiplier : Walk);
		const FPace Pace = MeasurePace(Locomotion, 60, 120);
		AddInfo(FString::Printf(TEXT("%s: the body goes %.0f cm/s (wanted %.0f), the planted feet travel %.0f cm/s under it over %d planted frames, animation rate %.2f"),
			Scenario.Name, Pace.Ground, Wanted, Pace.Foot, Pace.PlantedFrames, Pace.Rate));
		if (Pace.Travelled < Wanted * 0.5f || FMath::Abs(Pace.Ground - Wanted) > Wanted * 0.05f)
		{
			AddWarning(FString::Printf(TEXT("%s: the test level didn't run the player at %.0f cm/s (%.0f cm/s over %.0f cm): the feet aren't measured."),
				Scenario.Name, Wanted, Pace.Ground, Pace.Travelled));
			continue;
		}
		if (Pace.PlantedFrames < 8)
		{
			AddWarning(FString::Printf(TEXT("%s: no planted foot seen (%d frames): the Anim Blueprint didn't run in the test level, the feet aren't measured."),
				Scenario.Name, Pace.PlantedFrames));
			continue;
		}

		TestTrue(FString::Printf(TEXT("%s: the feet don't slide (they travel %.0f cm/s under a body going %.0f)"), Scenario.Name, Pace.Foot, Pace.Ground),
			FMath::Abs(Pace.Foot - Pace.Ground) <= Pace.Ground * PaceTolerance);
		// Up to the full-size walk the blend space plays the right clips at their own pace; past it the whole animation is sped up.
		const float WantedRate = FMath::Max(1.f, Pace.Ground / Walk);
		TestTrue(FString::Printf(TEXT("%s: the animation rate is %.2f (wanted %.2f)"), Scenario.Name, Pace.Rate, WantedRate),
			FMath::IsNearlyEqual(Pace.Rate, WantedRate, 0.08f));
	}

	// A slide holds the legs in their pose, so the run cycle underneath mustn't pump them: the animation nearly stops.
	UPlayerLocomotionComponent* Slider = SpawnPlayer(World, FVector(0.0, -4500.0 + 1500.0 * Row, 0.0));
	if (TestNotNull(TEXT("A sliding player"), Slider))
	{
		StartSlide(Slider);
		for (int32 Frames = 0; Frames < 30; ++Frames)
		{
			Step(Slider);
		}
		if (TestTrue(TEXT("Sliding"), Slider->IsSliding()))
		{
			const float Rate = BodyMesh(Slider)->GlobalAnimRateScale;
			TestTrue(FString::Printf(TEXT("A slide holds the animation nearly still (rate %.2f at slide pose %.2f)"), Rate, Slider->GetSlideAlpha()),
				Slider->GetSlideAlpha() >= 0.9f && Rate <= 0.5f);
		}
	}
	return true;
}

#endif
