#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/WeaponManagerComponent.h"
#include "Player/Animation/LooterClimbPose.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerViewComponent.h"
#include "Player/TraversalProbe.h"
#include "Tests/AutomationCommon.h"
#include "Tests/PlayerAnimPlayKit.h"
#include "UObject/Script.h"

// The climbing pose and the body's clearances played out on the real character, with the real Anim Blueprints, in a test
// level: a mantle and a vault followed bone by bone (the pose follows the locomotion's alpha, nothing pops), and the gun in the
// right hand against the legs through every stance. The pose and its rules without a world: PlayerAnimTests.cpp.

using namespace LocomotionTestWorld;
using namespace PlayerAnimPlayKit;

namespace
{
	/**
	 * The most a bone of the body may move in one frame (mesh cm). The legs of a sprint swing at up to 30 cm a frame on their
	 * own at the sprint's animation rate, so this catches a teleport, not the run.
	 */
	constexpr float BoneStepLimit = 45.f;
	/** How near (world cm) the left hand gets to the ledge's top edge in a mantle. */
	constexpr float HandReachLimit = 30.f;

	struct FClimbPlay
	{
		bool bAnim = false;
		bool bFinite = true;
		float PeakAlpha = 0.f;
		float PeakLegs = 0.f;
		float PeakHands = 0.f;
		/** How far the body's lean was from the locomotion's alpha at its worst (it should be exactly it). */
		float MaxLeanGap = 0.f;
		float LastAlpha = 1.f;
		float LastLean = 1.f;
		float MaxBoneStep = 0.f;
		const TCHAR* WorstBone = TEXT("");
		/** The left hand's nearest approach to the ledge point, and the least the hips-to-left-foot height got (mesh cm). */
		float ClosestHand = UE_BIG_NUMBER;
		float LeastDrop = UE_BIG_NUMBER;
	};

	/** The move under way (just started by the jump key), followed until it is over and its pose has gone out. */
	FClimbPlay FollowClimb(UPlayerLocomotionComponent* Climber, bool bVault, const FVector& Ledge)
	{
		FClimbPlay Play;
		const ULooterCharacterAnimInstance* Anim = AnimOf(Climber);
		Play.bAnim = Anim != nullptr;
		const TCHAR* Watched[] = { TEXT("pelvis"), TEXT("spine_05"), TEXT("head"), TEXT("hand_l"), TEXT("hand_r"), TEXT("calf_l"), TEXT("calf_r"), TEXT("foot_l"), TEXT("foot_r") };
		TArray<FVector> Last;
		int32 After = 0;
		for (int32 Frames = 0; Frames < 150 && After < 40; ++Frames)
		{
			PlayFrame(Climber);
			PlayAnimation(Climber);
			const bool bMoving = Climber->IsTraversing();
			After += bMoving ? 0 : 1;
			const float Alpha = Climber->GetTraversalAlpha();
			Play.PeakAlpha = FMath::Max(Play.PeakAlpha, Alpha);
			Play.LastAlpha = Alpha;
			Play.bFinite &= PoseIsFinite(Climber);
			if (Anim)
			{
				const FLooterClimbInput& Climb = Anim->GetStanceInput().Climb;
				Play.PeakLegs = FMath::Max(Play.PeakLegs, Climb.Legs);
				Play.PeakHands = FMath::Max(Play.PeakHands, Climb.Hands);
				// A mantle's lean is the alpha; a vault's tops out at 0.6 and the body counts that as a full pose.
				const float Expected = bVault ? FMath::Min(1.f, Alpha / LooterClimbPose::VaultAlphaShare) : Alpha;
				Play.MaxLeanGap = FMath::Max(Play.MaxLeanGap, FMath::Abs(Climb.Lean - Expected));
				Play.LastLean = Climb.Lean;
			}
			if (bMoving)
			{
				const FVector Hand = InWorld(Climber, TEXT("hand_l"));
				Play.ClosestHand = FMath::Min(Play.ClosestHand, static_cast<float>(FVector2D(Hand.X - Ledge.X, Hand.Z - Ledge.Z).Size()));
				Play.LeastDrop = FMath::Min(Play.LeastDrop, static_cast<float>(InMesh(Climber, TEXT("pelvis")).Z - InMesh(Climber, TEXT("foot_l")).Z));
			}
			TArray<FVector> Now;
			for (const TCHAR* Bone : Watched)
			{
				Now.Add(InMesh(Climber, Bone));
			}
			if (Last.Num() == Now.Num())
			{
				for (int32 Index = 0; Index < Now.Num(); ++Index)
				{
					const float Step = static_cast<float>(FVector::Dist(Now[Index], Last[Index]));
					if (Step > Play.MaxBoneStep)
					{
						Play.MaxBoneStep = Step;
						Play.WorstBone = Watched[Index];
					}
				}
			}
			Last = MoveTemp(Now);
		}
		return Play;
	}

	void CheckClimb(FAutomationTestBase& Test, const TCHAR* Name, const FClimbPlay& Play, bool bVault, float StandingDrop)
	{
		Test.TestTrue(FString::Printf(TEXT("%s: every posed frame is finite"), Name), Play.bFinite);
		Test.TestTrue(FString::Printf(TEXT("%s: no bone jumps on the way in, through it or on the way out (%.1f cm in a frame at most, the %s)"), Name, Play.MaxBoneStep, Play.WorstBone),
			Play.MaxBoneStep <= BoneStepLimit);
		if (bVault)
		{
			Test.TestTrue(FString::Printf(TEXT("%s: the locomotion's pose alpha tops out at the body's vault share (%.2f of %.2f)"), Name, Play.PeakAlpha, LooterClimbPose::VaultAlphaShare),
				FMath::IsNearlyEqual(Play.PeakAlpha, LooterClimbPose::VaultAlphaShare, 0.03f));
		}
		else
		{
			Test.TestTrue(FString::Printf(TEXT("%s: the pose alpha comes in (%.2f)"), Name, Play.PeakAlpha), Play.PeakAlpha >= 0.8f);
		}
		Test.TestTrue(FString::Printf(TEXT("%s: ...and goes out after (%.3f)"), Name, Play.LastAlpha), Play.LastAlpha <= 0.01f);
		if (!Play.bAnim)
		{
			Test.AddWarning(FString::Printf(TEXT("%s: the body has no Anim Blueprint of ours in the test level, so its pose wasn't followed."), Name));
			return;
		}
		Test.TestTrue(FString::Printf(TEXT("%s: the body's lean follows the locomotion's alpha (%.3f off at worst)"), Name, Play.MaxLeanGap), Play.MaxLeanGap <= 0.011f);
		Test.TestTrue(FString::Printf(TEXT("%s: the legs tuck (%.2f at most) and the hands reach (%.2f)"), Name, Play.PeakLegs, Play.PeakHands),
			Play.PeakLegs >= 0.5f && Play.PeakHands >= 0.5f);
		Test.TestTrue(FString::Printf(TEXT("%s: the pose is gone once the move is over (lean %.3f)"), Name, Play.LastLean), Play.LastLean <= 0.01f);
		Test.TestTrue(FString::Printf(TEXT("%s: the leading foot comes up (hips to foot %.1f of %.1f cm standing)"), Name, Play.LeastDrop, StandingDrop),
			Play.LeastDrop <= StandingDrop * 0.85f);
	}

	/** The body standing and animated a few frames, so a baseline pose exists; returns the hips-to-left-foot height (mesh cm). */
	float StandingDropOf(UPlayerLocomotionComponent* Locomotion)
	{
		SettleStanding(Locomotion);
		for (int32 Frames = 0; Frames < 4; ++Frames)
		{
			PlayAnimation(Locomotion);
		}
		return static_cast<float>(InMesh(Locomotion, TEXT("pelvis")).Z - InMesh(Locomotion, TEXT("foot_l")).Z);
	}

	/**
	 * How far the gun's axis is from the legs' surface right now (world cm; under 0 it runs through a leg): the gun in the
	 * right hand's socket, along the way the view points it, from its stock to a rifle's muzzle, against the thighs and calves.
	 */
	float GunToLegs(const UPlayerLocomotionComponent* Locomotion, const UPlayerViewComponent* View, const TCHAR* Socket)
	{
		const FVector Grip = InWorld(Locomotion, Socket);
		const FVector Axis = View->GetHeldWeaponRotation().GetForwardVector();
		const FVector Stock = Grip - Axis * 18.0;
		const FVector Muzzle = Grip + Axis * 60.0;
		float Closest = UE_BIG_NUMBER;
		auto Clearance = [&](const FVector& From, const FVector& To, float Radius)
		{
			FVector OnGun;
			FVector OnLeg;
			FMath::SegmentDistToSegmentSafe(Stock, Muzzle, From, To, OnGun, OnLeg);
			Closest = FMath::Min(Closest, static_cast<float>(FVector::Dist(OnGun, OnLeg)) - Radius * LooterPlayerSize::Scale);
		};
		const TCHAR* Sides[] = { TEXT("l"), TEXT("r") };
		for (const TCHAR* Side : Sides)
		{
			const FVector Thigh = InWorld(Locomotion, *FString::Printf(TEXT("thigh_%s"), Side));
			const FVector Calf = InWorld(Locomotion, *FString::Printf(TEXT("calf_%s"), Side));
			const FVector Foot = InWorld(Locomotion, *FString::Printf(TEXT("foot_%s"), Side));
			Clearance(Thigh, Calf, 9.f);
			Clearance(Calf, Foot, 6.f);
		}
		return Closest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerAnimClimbPlayTest, "Looter.PlayerAnim.ClimbPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerAnimClimbPlayTest::RunTest(const FString& Parameters)
{
	// The user's ask (2026-10-08): the third-person body shouldn't run on the spot through a mantle or a vault. Its legs tuck
	// and its hands reach for the obstacle, driven by the locomotion's pose alpha, and nothing pops on the way in or out.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	UPlayerLocomotionComponent* Climber = SpawnPlayer(World, FVector::ZeroVector);
	UPlayerLocomotionComponent* Runner = SpawnPlayer(World, FVector(0.0, 1500.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !Climber || !Runner)
	{
		AddError(TEXT("The players weren't made."));
		return false;
	}
	const float Radius = RadiusOf(Climber);
	// A 1.2 m box 10 cm in front of the climber; a 1 m rail 60 cm in front of the runner.
	const double Face = Radius + 10.0;
	SpawnBlock(World, FVector(Face + 100.0, 0.0, 60.0), FVector(200.0, 300.0, 120.0));
	const double RailFace = Radius + 60.0;
	SpawnBlock(World, FVector(RailFace + 5.0, 1500.0, 50.0), FVector(10.0, 400.0, 100.0));

	const float ClimberDrop = StandingDropOf(Climber);
	Climber->HandleJumpPressed();
	if (TestTrue(FString::Printf(TEXT("Jump at a 1.2 m box: a mantle (refused: %s)"), FTraversalProbe::RefusalName(Climber->GetLastTraversalRefusal())),
		Climber->IsTraversing() && Climber->GetTraversalKind() == ETraversalKind::Mantle))
	{
		const FClimbPlay Play = FollowClimb(Climber, false, FVector(Face + LooterClimbPose::MantleHandDepth, 0.0, 120.0));
		CheckClimb(*this, TEXT("Mantle"), Play, false, ClimberDrop);
		if (Play.bAnim)
		{
			TestTrue(FString::Printf(TEXT("Mantle: the left hand gets onto the ledge (%.1f cm from its edge at the nearest)"), Play.ClosestHand), Play.ClosestHand <= HandReachLimit);
		}
	}

	const float RunnerDrop = StandingDropOf(Runner);
	Runner->HandleMoveInput(FVector2D(0.0, 1.0));
	StartSprint(Runner);
	Runner->HandleJumpPressed();
	if (TestTrue(FString::Printf(TEXT("Jump at a 1 m rail at a sprint: a vault (refused: %s)"), FTraversalProbe::RefusalName(Runner->GetLastTraversalRefusal())),
		Runner->IsTraversing() && Runner->GetTraversalKind() == ETraversalKind::Vault))
	{
		const FClimbPlay Play = FollowClimb(Runner, true, FVector(RailFace + 5.0, 0.0, 100.0));
		CheckClimb(*this, TEXT("Vault"), Play, true, RunnerDrop);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerAnimClearanceTest, "Looter.PlayerAnim.Clearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerAnimClearanceTest::RunTest(const FString& Parameters)
{
	// Clipping (the user, 2026-10-08: models that clip). In first person the body is never drawn to its owner, so the camera
	// can't see inside the head or the body in any stance; in third person the gun in the right hand never runs through the
	// legs, in the stances that fold them (crouch, slide, sprint, mantle, vault).
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	const UClass* ArmedClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Rifle/ABP_Rifle.ABP_Rifle_C"));
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("The rifle Anim Blueprint"), ArmedClass))
	{
		return false;
	}

	UPlayerLocomotionComponent* Players[6] = {};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Players); ++Index)
	{
		Players[Index] = SpawnPlayer(World, FVector(0.0, 1500.0 * Index - 3750.0, 0.0));
		if (!TestNotNull(TEXT("A player"), Players[Index]))
		{
			return false;
		}
		BodyMesh(Players[Index])->SetAnimInstanceClass(const_cast<UClass*>(ArmedClass));
	}

	// First person: the body is the world's representation of the player (its shadow), not drawn to its owner. In third
	// person it is, and back again.
	{
		UPlayerViewComponent* View = BodyOf(Players[0])->FindComponentByClass<UPlayerViewComponent>();
		if (TestNotNull(TEXT("The view component"), View))
		{
			const USkeletalMeshComponent* Body = BodyMesh(Players[0]);
			TestTrue(TEXT("First person: the body isn't drawn to its owner"), View->IsFirstPerson() && Body->bOwnerNoSee);
			View->SetViewMode(EPlayerViewMode::ThirdPerson);
			TestFalse(TEXT("Third person: it is"), Body->bOwnerNoSee);
			View->SetViewMode(EPlayerViewMode::FirstPerson);
			TestTrue(TEXT("...and back in first person it isn't"), Body->bOwnerNoSee);
		}
	}

	const UWeaponManagerComponent* Manager = BodyOf(Players[0])->FindComponentByClass<UWeaponManagerComponent>();
	const FName Socket = Manager ? Manager->ThirdPersonAttachSocket : FName(TEXT("HandGrip_R"));
	if (!TestTrue(TEXT("The hand socket the gun is held in"), BodyMesh(Players[0])->DoesSocketExist(Socket)))
	{
		return false;
	}
	const FString GripBone = Socket.ToString();

	// The stances, each on its own player: how near the gun's axis comes to a leg, at the worst of the frames that count.
	const TCHAR* Names[] = { TEXT("Standing"), TEXT("Crouched"), TEXT("Sliding"), TEXT("Sprinting"), TEXT("Mantling"), TEXT("Vaulting") };
	float Worst[UE_ARRAY_COUNT(Names)];
	for (float& Value : Worst)
	{
		Value = UE_BIG_NUMBER;
	}
	auto Measure = [&](int32 Index)
	{
		const UPlayerViewComponent* View = BodyOf(Players[Index])->FindComponentByClass<UPlayerViewComponent>();
		if (View)
		{
			Worst[Index] = FMath::Min(Worst[Index], GunToLegs(Players[Index], View, *GripBone));
		}
	};

	SettleStanding(Players[0]);
	for (int32 Frames = 0; Frames < 6; ++Frames)
	{
		PlayAnimation(Players[0]);
		Measure(0);
	}

	CrouchDown(Players[1]);
	for (int32 Frames = 0; Frames < 40; ++Frames)
	{
		PlayFrame(Players[1]);
		PlayAnimation(Players[1]);
		if (Frames >= 30)
		{
			Measure(1);
		}
	}

	StartSlide(Players[2]);
	for (int32 Frames = 0; Frames < 40; ++Frames)
	{
		Step(Players[2]);
		PlayAnimation(Players[2]);
		if (Frames >= 8 && Players[2]->IsSliding())
		{
			Measure(2);
		}
	}

	Players[3]->HandleMoveInput(FVector2D(0.0, 1.0));
	StartSprint(Players[3]);
	for (int32 Frames = 0; Frames < 40; ++Frames)
	{
		Players[3]->HandleMoveInput(FVector2D(0.0, 1.0));
		Step(Players[3]);
		PlayAnimation(Players[3]);
		if (Frames >= 30)
		{
			Measure(3);
		}
	}

	// A mantle onto a 1.2 m box and a vault of a 1 m rail, the gun held through them.
	const double Face = RadiusOf(Players[4]) + 10.0;
	SpawnBlock(World, FVector(Face + 100.0, 1500.0 * 4 - 3750.0, 60.0), FVector(200.0, 300.0, 120.0));
	SettleStanding(Players[4]);
	Players[4]->HandleJumpPressed();
	const bool bMantled = Players[4]->IsTraversing() && Players[4]->GetTraversalKind() == ETraversalKind::Mantle;
	TestTrue(TEXT("The gun-holding player mantles"), bMantled);
	for (int32 Frames = 0; bMantled && Frames < 60 && Players[4]->IsTraversing(); ++Frames)
	{
		PlayFrame(Players[4]);
		PlayAnimation(Players[4]);
		Measure(4);
	}

	const double RailFace = RadiusOf(Players[5]) + 60.0;
	SpawnBlock(World, FVector(RailFace + 5.0, 1500.0 * 5 - 3750.0, 50.0), FVector(10.0, 400.0, 100.0));
	SettleStanding(Players[5]);
	Players[5]->HandleMoveInput(FVector2D(0.0, 1.0));
	StartSprint(Players[5]);
	Players[5]->HandleJumpPressed();
	const bool bVaulted = Players[5]->IsTraversing() && Players[5]->GetTraversalKind() == ETraversalKind::Vault;
	TestTrue(TEXT("...and vaults"), bVaulted);
	for (int32 Frames = 0; bVaulted && Frames < 60 && Players[5]->IsTraversing(); ++Frames)
	{
		PlayFrame(Players[5]);
		PlayAnimation(Players[5]);
		Measure(5);
	}

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		if (Worst[Index] >= UE_BIG_NUMBER)
		{
			AddWarning(FString::Printf(TEXT("%s: the gun wasn't measured."), Names[Index]));
			continue;
		}
		AddInfo(FString::Printf(TEXT("%s: the gun's axis comes %.1f cm from the nearest leg's surface"), Names[Index], Worst[Index]));
		TestTrue(FString::Printf(TEXT("%s: the gun doesn't run through a leg (%.1f cm clear)"), Names[Index], Worst[Index]), Worst[Index] > 0.f);
	}
	return true;
}

#endif
