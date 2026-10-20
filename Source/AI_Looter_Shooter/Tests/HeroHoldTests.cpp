#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Embers/HeroDefinition.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseCarry.h"
#include "Tests/PlayerAnimTestKit.h"
#include "TwoBoneIK.h"

// A hero's own gun hold in third person (UHeroDefinition::ThirdPersonHold; LooterStancePoseDetail::ApplyHoldOffset): the
// guns hang from the hidden mannequin's hand and every hero's hands are pinned to its hands, so a taller hero (Crane) held
// them at the chest and a shorter one (Pike) had the sight over his eyes (2026-10-20). The mannequin's hands now move by
// the worn hero's hold, both of them, keeping their turn, the left palm on the foregrip, in a sprint's carry too.

namespace
{
	/** The mannequin's front and right in its component space (it faces +Y; its right is -X), and that facing's yaw. */
	const FVector HoldAhead(0.0, 1.0, 0.0);
	const FVector HoldRight(-1.0, 0.0, 0.0);
	constexpr float HoldFacingYaw = 90.f;
	/** The left palm off hand_l (the mannequin's HandGrip_L point), as the stance layer has it. */
	const FVector HoldPalmFromWrist(7.5, -2.5, 0.0);
	/** A bullpup's foregrip from its grip (Bullpup.py). */
	const FVector HoldForegrip(21.5, 0.0, 5.5);

	FTransform HoldBone(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Pose, const TCHAR* Name)
	{
		FCSPose<FCompactPose> Space;
		Space.InitPose(Pose);
		FBoneReference Bone(Name);
		Bone.Initialize(Rig.Bones);
		const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(Rig.Bones);
		return Index.IsValid() ? Space.GetComponentSpaceTransform(Index) : FTransform::Identity;
	}

	/** The reference pose with the right wrist up in front of the right chest, as the armed upper body holds a gun. */
	FCompactPose HoldShouldered(const PlayerAnimTestKit::FPoseRig& Rig)
	{
		FCompactPose Posed = Rig.RefPose();
		FCSPose<FCompactPose> Space;
		Space.InitPose(Posed);
		const auto Index = [&Rig](const TCHAR* Name)
		{
			FBoneReference Bone(Name);
			Bone.Initialize(Rig.Bones);
			return Bone.GetCompactPoseIndex(Rig.Bones);
		};
		const FCompactPoseBoneIndex UpperArm = Index(TEXT("upperarm_r"));
		const FCompactPoseBoneIndex LowerArm = Index(TEXT("lowerarm_r"));
		const FCompactPoseBoneIndex Hand = Index(TEXT("hand_r"));
		if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
		{
			return Posed;
		}
		FTransform Shoulder = Space.GetComponentSpaceTransform(UpperArm);
		FTransform Elbow = Space.GetComponentSpaceTransform(LowerArm);
		FTransform Grip = Space.GetComponentSpaceTransform(Hand);
		const FQuat HandRotation = Grip.GetRotation();
		const FVector Wrist = Shoulder.GetLocation() + FVector(14.0, 16.0, -18.0);
		AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Grip, Elbow.GetLocation() + HoldRight * 10.0 - FVector(0.0, 0.0, 15.0), Wrist,
			/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
		Grip.SetRotation(HandRotation);
		const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Grip) };
		Space.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
		FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Space), Posed);
		return Posed;
	}

	/** One held pose: the hands, the grip and the gun. */
	struct FHeldPose
	{
		FCompactPose Pose;
		FTransform HandR;
		FTransform HandL;
		FVector Grip = FVector::ZeroVector;
		FQuat Gun = FQuat::Identity;
		FVector Palm() const { return HandL.TransformPosition(HoldPalmFromWrist); }
		FVector Foregrip() const { return Grip + Gun.RotateVector(HoldForegrip); }
	};

	/** A bullpup in the right hand of Shouldered, held with Offset (the hero's hold), aimed Pitch degrees up, Carry as given. */
	FHeldPose HoldPose(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Shouldered, const FVector& Offset, float Pitch, const FLooterCarry& Carry)
	{
		const FTransform Hand = HoldBone(Rig, Shouldered, TEXT("hand_r"));
		FLooterStanceInput Held;
		Held.bAimWithTorso = true;
		Held.AimPitch = Pitch;
		Held.bHandOnForegrip = true;
		Held.HoldBone = TEXT("hand_r");
		Held.HoldSocketLocal = FTransform(Hand.GetLocation() + HoldAhead * 6.0 - FVector(0.0, 0.0, 2.0)).GetRelativeTransform(Hand);
		Held.WeaponForegrip = HoldForegrip;
		Held.HoldOffset = Offset;
		Held.SprintTorsoLean = 9.f;
		Held.Carry = Carry;
		Held.SprintAlpha = Carry.Sprint;
		Held.AimRotation = FRotator(Pitch, HoldFacingYaw, 0.f).Quaternion();
		Held.CarryRotation = LooterCarry::CarriedGun(Held.AimRotation, HoldFacingYaw, Carry);
		Held.WeaponRotation = Held.CarryRotation;
		FHeldPose Out;
		Out.Pose = Shouldered;
		LooterStancePose::Apply(Out.Pose, Held);
		Out.HandR = HoldBone(Rig, Out.Pose, TEXT("hand_r"));
		Out.HandL = HoldBone(Rig, Out.Pose, TEXT("hand_l"));
		Out.Grip = (Held.HoldSocketLocal * Out.HandR).GetLocation();
		Out.Gun = Held.WeaponRotation;
		return Out;
	}

	float HoldDegrees(const FQuat& A, const FQuat& B)
	{
		return FMath::RadiansToDegrees(static_cast<float>(A.AngularDistance(B)));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroHoldTest, "Looter.Heroes.Hold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroHoldTest::RunTest(const FString& Parameters)
{
	USkeleton* Skeleton = PlayerAnimTestKit::MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const PlayerAnimTestKit::FPoseRig Rig(*Skeleton);
	const FCompactPose Shouldered = HoldShouldered(Rig);
	const FHeldPose Plain = HoldPose(Rig, Shouldered, FVector::ZeroVector, 0.f, FLooterCarry());

	// Level: the right hand (and the grip in it) moves by the hold along the facing, its right and up; turned as it was; the
	// left palm still on the foregrip.
	struct FCase { FVector Offset; const TCHAR* Who; };
	for (const FCase& Case : { FCase{ FVector(0.0, 0.0, 14.0), TEXT("a tall hero's") }, FCase{ FVector(0.0, 0.0, -6.0), TEXT("a short hero's") },
		FCase{ FVector(5.0, 3.0, -7.0), TEXT("a broad hero's") } })
	{
		const FHeldPose Moved = HoldPose(Rig, Shouldered, Case.Offset, 0.f, FLooterCarry());
		const FVector Want = HoldAhead * Case.Offset.X + HoldRight * Case.Offset.Y + FVector::UpVector * Case.Offset.Z;
		const FVector Got = Moved.Grip - Plain.Grip;
		TestTrue(FString::Printf(TEXT("%s hold: sound"), Case.Who), PlayerAnimTestKit::FPoseRig::IsSound(Moved.Pose));
		TestTrue(FString::Printf(TEXT("%s hold %s: the grip moves by it (%s, %.2f cm off)"), Case.Who, *Case.Offset.ToCompactString(), *Got.ToCompactString(),
			FVector::Dist(Got, Want)), FVector::Dist(Got, Want) <= 0.5);
		TestTrue(FString::Printf(TEXT("...the hand keeps its turn (%.2f degrees)"), HoldDegrees(Plain.HandR.GetRotation(), Moved.HandR.GetRotation())),
			HoldDegrees(Plain.HandR.GetRotation(), Moved.HandR.GetRotation()) <= 0.5f);
		TestTrue(FString::Printf(TEXT("...the left palm on the foregrip (%.2f cm off)"), FVector::Dist(Moved.Palm(), Moved.Foregrip())),
			FVector::Dist(Moved.Palm(), Moved.Foregrip()) <= 1.0);
	}

	// Aimed up: the hold pitches with the torso (a hero's shoulders are off the mannequin's along the chest).
	const FHeldPose UpPlain = HoldPose(Rig, Shouldered, FVector::ZeroVector, 30.f, FLooterCarry());
	const FHeldPose UpMoved = HoldPose(Rig, Shouldered, FVector(0.0, 0.0, 12.0), 30.f, FLooterCarry());
	const FVector UpWant = FQuat(FVector::CrossProduct(FVector::UpVector, HoldAhead), FMath::DegreesToRadians(-30.f)).RotateVector(FVector(0.0, 0.0, 12.0));
	const FVector UpGot = UpMoved.Grip - UpPlain.Grip;
	TestTrue(FString::Printf(TEXT("Aimed 30 up: the hold tips back with the chest (%s, want %s)"), *UpGot.ToCompactString(), *UpWant.ToCompactString()),
		FVector::Dist(UpGot, UpWant) <= 0.75 && FVector::DotProduct(UpGot, HoldAhead) < -3.0);

	// A sprint's carry low across the chest: at the hero's chest, as far off the mannequin's as the hold; the palm on.
	FLooterCarry Sprint;
	Sprint.Sprint = 1.f;
	const FHeldPose RunPlain = HoldPose(Rig, Shouldered, FVector::ZeroVector, 0.f, Sprint);
	const FHeldPose RunMoved = HoldPose(Rig, Shouldered, FVector(0.0, 0.0, 14.0), 0.f, Sprint);
	const float RunRise = static_cast<float>(RunMoved.HandR.GetLocation().Z - RunPlain.HandR.GetLocation().Z);
	TestTrue(FString::Printf(TEXT("Sprint: a tall hero carries it higher (%.1f cm of 14)"), RunRise), RunRise >= 10.f && RunRise <= 15.f);
	TestTrue(FString::Printf(TEXT("Sprint: the left palm on the foregrip (%.2f cm off)"), FVector::Dist(RunMoved.Palm(), RunMoved.Foregrip())),
		FVector::Dist(RunMoved.Palm(), RunMoved.Foregrip()) <= 1.5);

	// The data: Ellis is built to the mannequin's size; Crane, the tallest, holds higher; Pike and Odessa, shorter, lower.
	const UHeroDefinition* Ellis = UHeroDefinition::Load(TEXT("Ellis"));
	const UHeroDefinition* Crane = UHeroDefinition::Load(TEXT("Crane"));
	const UHeroDefinition* Pike = UHeroDefinition::Load(TEXT("Pike"));
	const UHeroDefinition* Odessa = UHeroDefinition::Load(TEXT("Odessa"));
	if (TestTrue(TEXT("The heroes' definitions load"), Ellis && Crane && Pike && Odessa))
	{
		TestTrue(TEXT("Ellis holds where the mannequin does"), Ellis->ThirdPersonHold.IsNearlyZero());
		TestTrue(FString::Printf(TEXT("Crane holds higher (%.1f cm)"), Crane->ThirdPersonHold.Z), Crane->ThirdPersonHold.Z >= 8.f);
		TestTrue(FString::Printf(TEXT("Pike and Odessa hold lower (%.1f, %.1f cm)"), Pike->ThirdPersonHold.Z, Odessa->ThirdPersonHold.Z),
			Pike->ThirdPersonHold.Z < 0.f && Odessa->ThirdPersonHold.Z < 0.f);
	}
	return true;
}

#endif
