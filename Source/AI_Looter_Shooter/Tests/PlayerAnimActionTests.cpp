#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/PlayerThrowMotion.h"
#include "Player/PlayerThrowRules.h"
#include "Tests/PlayerAnimTestKit.h"
#include "Weapons/ReloadMotion.h"
#include "TwoBoneIK.h"

// The player's body for the actions with no animation of their own, without a world: the stance layer's grenade throw
// and the Drover's reload, run on the mannequin's reference pose bone by bone (PlayerAnimTests.cpp has the stance itself,
// the climb and the melee jab).

using namespace PlayerAnimTestKit;

namespace
{
	/** Every bone of two poses the same, within a millimetre. */
	bool SamePose(const FCompactPose& A, const FCompactPose& B)
	{
		for (const FCompactPoseBoneIndex Index : A.ForEachBoneIndex())
		{
			if (!A[Index].Equals(B[Index], 0.01))
			{
				return false;
			}
		}
		return true;
	}

	/** A bone's whole transform in component space (the rig's Where gives only its place). */
	FTransform BoneTransform(const FPoseRig& Rig, FCSPose<FCompactPose>& Space, const TCHAR* Name)
	{
		FBoneReference Bone(Name);
		Bone.Initialize(Rig.Bones);
		const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(Rig.Bones);
		return Index.IsValid() ? Space.GetComponentSpaceTransform(Index) : FTransform::Identity;
	}

	/** The left palm (the mannequin's HandGrip_L point) off the hand_l bone: where the stance layer puts what the hand holds. */
	const FVector LeftPalmFromWrist(7.5, -2.5, 0.0);

	/**
	 * A six-gun in the right hand of the reference pose, its grip held in front of the chest (the pose's hands hang at its
	 * sides, out of the other hand's reach): the Drover's support palm on the grip's left, pointing the way the mannequin
	 * faces (+Y), held out by its reach.
	 */
	FLooterStanceInput SixGunInHand(const FPoseRig& Rig, const FCompactPose& Rest)
	{
		FCSPose<FCompactPose> Space;
		Space.InitPose(Rest);
		FLooterStanceInput Held;
		Held.bHandOnForegrip = true;
		Held.HoldBone = TEXT("hand_r");
		Held.HoldSocketLocal = FTransform(FVector(-4.0, 24.0, 118.0)).GetRelativeTransform(BoneTransform(Rig, Space, TEXT("hand_r")));
		Held.WeaponRotation = FRotator(0.f, 90.f, 0.f).Quaternion();
		Held.WeaponForegrip = FVector(0.7, -3.2, -0.8);
		Held.HoldReach = 14.f;
		return Held;
	}

	/** Where the held gun's grip is in a posed body, and the left palm. */
	FVector GripIn(const FPoseRig& Rig, FCSPose<FCompactPose>& Space, const FLooterStanceInput& Held)
	{
		return (Held.HoldSocketLocal * BoneTransform(Rig, Space, TEXT("hand_r"))).GetLocation();
	}

	FVector PalmIn(const FPoseRig& Rig, FCSPose<FCompactPose>& Space)
	{
		return BoneTransform(Rig, Space, TEXT("hand_l")).TransformPosition(LeftPalmFromWrist);
	}

	/**
	 * The reference pose with its right hand brought up to Wrist (component space), the elbow bent down and out: a gun held
	 * in front of the chest, as the armed upper-body pose holds one. The reference pose's arm hangs nearly straight (52 of
	 * its 55 cm), where no IK can drop the hand further.
	 */
	FCompactPose WithRightHandAt(const FPoseRig& Rig, const FCompactPose& Rest, const FVector& Wrist)
	{
		const auto Index = [&Rig](const TCHAR* Name)
		{
			FBoneReference Bone(Name);
			Bone.Initialize(Rig.Bones);
			return Bone.GetCompactPoseIndex(Rig.Bones);
		};
		const FCompactPoseBoneIndex UpperArm = Index(TEXT("upperarm_r"));
		const FCompactPoseBoneIndex LowerArm = Index(TEXT("lowerarm_r"));
		const FCompactPoseBoneIndex Hand = Index(TEXT("hand_r"));
		FCompactPose Posed = Rest;
		if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
		{
			return Posed;
		}
		FCSPose<FCompactPose> Space;
		Space.InitPose(Rest);
		FTransform Shoulder = Space.GetComponentSpaceTransform(UpperArm);
		FTransform Elbow = Space.GetComponentSpaceTransform(LowerArm);
		FTransform Grip = Space.GetComponentSpaceTransform(Hand);
		const FQuat HandRotation = Grip.GetRotation();
		// The mannequin's right is -X in its component space.
		AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Grip, Elbow.GetLocation() - FVector(10.0, 0.0, 15.0), Wrist,
			/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
		Grip.SetRotation(HandRotation);
		const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Grip) };
		Space.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
		FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Space), Posed);
		return Posed;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerAnimThrowTest, "Looter.PlayerAnim.Throw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerAnimThrowTest::RunTest(const FString& Parameters)
{
	// Third person had no grenade throw: the body stood still while the jar flew out of its chest. The stance layer tosses
	// it overarm with the left hand now, on the throw's clock: cocked up by the ear, slung out ahead at the release, the gun
	// hand dipped out of the way, and nothing before or after.
	const float Release = FThrowRules::ReleaseSeconds;
	TestFalse(TEXT("No throw: no pose"), LooterStancePose::DescribeThrow(-1.f).IsActive());
	TestFalse(TEXT("At the press: nothing yet"), LooterStancePose::DescribeThrow(0.f).IsActive());
	TestFalse(TEXT("Over by the throw's end"), LooterStancePose::DescribeThrow(FThrowRules::ThrowSeconds).IsActive());
	TestTrue(TEXT("The arm is on the throw by the draw"), FMath::IsNearlyEqual(LooterStancePose::DescribeThrow(ThrowMotion::DrawSeconds).Arm, 1.f, 0.01f));
	TestTrue(TEXT("The jar is in the hand while it's drawn, gone once it's let go"), ThrowMotion::IsJarInHand(ThrowMotion::DrawSeconds)
		&& !ThrowMotion::IsJarInHand(0.f) && !ThrowMotion::IsJarInHand(Release));
	TestTrue(TEXT("The chest winds up, then turns into the toss"), LooterStancePose::DescribeThrow(ThrowMotion::DrawSeconds).Twist < 0.f
		&& LooterStancePose::DescribeThrow(Release).Twist > 0.f);

	USkeleton* Skeleton = MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();
	const FVector RestHandL = Rig.Where(Rest, TEXT("hand_l"));

	// Zero input: a throw at its first instant or its end leaves the pose exactly as it was.
	for (const float Time : { 0.f, FThrowRules::ThrowSeconds })
	{
		FLooterStanceInput Input;
		Input.ThrowTime = Time;
		FCompactPose Pose = Rest;
		LooterStancePose::Apply(Pose, Input);
		TestTrue(FString::Printf(TEXT("A throw %.2f s in leaves the pose untouched"), Time), SamePose(Rest, Pose));
	}

	// Cocked: the left hand up by the head. Let go: out ahead of the body. (The mannequin faces +Y in its component space.)
	FLooterStanceInput Drawn;
	Drawn.ThrowTime = ThrowMotion::DrawSeconds;
	FCompactPose DrawnPose = Rest;
	LooterStancePose::Apply(DrawnPose, Drawn);
	FLooterStanceInput Thrown;
	Thrown.ThrowTime = Release;
	FCompactPose ThrownPose = Rest;
	LooterStancePose::Apply(ThrownPose, Thrown);
	TestTrue(TEXT("Throw: sound"), FPoseRig::IsSound(DrawnPose) && FPoseRig::IsSound(ThrownPose));
	const FVector DrawnHand = Rig.Where(DrawnPose, TEXT("hand_l"));
	const FVector ThrownHand = Rig.Where(ThrownPose, TEXT("hand_l"));
	TestTrue(FString::Printf(TEXT("Cocked: the left hand up by the head (%.1f cm, was %.1f)"), DrawnHand.Z, RestHandL.Z), DrawnHand.Z > RestHandL.Z + 40.0);
	TestTrue(FString::Printf(TEXT("Let go: the left hand forward of where it was (%.1f cm, was %.1f)"), ThrownHand.Y, RestHandL.Y), ThrownHand.Y > RestHandL.Y + 15.0);
	TestTrue(FString::Printf(TEXT("...slung forward from the draw (%.1f cm)"), ThrownHand.Y - DrawnHand.Y), ThrownHand.Y - DrawnHand.Y > 30.0);
	TestTrue(TEXT("The right hand isn't thrown"), Rig.Where(ThrownPose, TEXT("hand_r")).Equals(Rig.Where(Rest, TEXT("hand_r")), 15.0));
	TestTrue(TEXT("The feet stay put"), Rig.Where(ThrownPose, TEXT("foot_l")).Equals(Rig.Where(Rest, TEXT("foot_l")), 0.5));

	// With a gun in hand (held out in front of the chest), the gun hand comes in from arm's length and dips out of the
	// throw's way.
	{
		const FCompactPose GunUp = WithRightHandAt(Rig, Rest, FVector(-20.0, 30.0, 115.0));
		FLooterStanceInput Armed = SixGunInHand(Rig, GunUp);
		FCompactPose Holding = GunUp;
		LooterStancePose::Apply(Holding, Armed);
		Armed.ThrowTime = Release;
		FCompactPose Throwing = GunUp;
		LooterStancePose::Apply(Throwing, Armed);
		TestTrue(TEXT("Armed throw: sound"), FPoseRig::IsSound(Throwing));
		const double Dip = Rig.Where(Holding, TEXT("hand_r")).Z - Rig.Where(Throwing, TEXT("hand_r")).Z;
		TestTrue(FString::Printf(TEXT("Armed: the gun hand dips (%.1f cm)"), Dip), Dip > 8.0);
		TestTrue(FString::Printf(TEXT("Armed: the left hand leaves the gun to throw as a bare hand does (%.1f cm apart)"),
			FVector::Dist(Rig.Where(Throwing, TEXT("hand_l")), ThrownHand)), FVector::Dist(Rig.Where(Throwing, TEXT("hand_l")), ThrownHand) < 10.0);
	}

	// Followed through the whole throw at 120 frames a second: every pose sound, nothing popping. A throwing hand is quick
	// (13 cm a frame as the jar leaves it); a pop would be far past the limit.
	const TCHAR* Watched[] = { TEXT("spine_05"), TEXT("head"), TEXT("hand_l"), TEXT("lowerarm_l"), TEXT("hand_r") };
	TArray<FVector> Last;
	bool bSound = true;
	float MaxStep = 0.f;
	const TCHAR* WorstBone = TEXT("");
	for (float Time = 0.f; Time <= FThrowRules::ThrowSeconds + 0.02f; Time += 1.f / 120.f)
	{
		FLooterStanceInput Frame;
		Frame.ThrowTime = Time;
		FCompactPose Pose = Rest;
		LooterStancePose::Apply(Pose, Frame);
		bSound &= FPoseRig::IsSound(Pose);
		FCSPose<FCompactPose> Space;
		Space.InitPose(Pose);
		TArray<FVector> Now;
		for (const TCHAR* Bone : Watched)
		{
			Now.Add(Rig.Where(Space, Bone));
		}
		for (int32 Index = 0; Index < Now.Num() && Last.Num() == Now.Num(); ++Index)
		{
			const float Step = static_cast<float>(FVector::Dist(Now[Index], Last[Index]));
			if (Step > MaxStep)
			{
				MaxStep = Step;
				WorstBone = Watched[Index];
			}
		}
		Last = MoveTemp(Now);
	}
	TestTrue(TEXT("Through the throw: every pose sound"), bSound);
	TestTrue(FString::Printf(TEXT("Through the throw: no bone jumps (%.1f cm in a 120th of a second at most, the %s)"), MaxStep, WorstBone), MaxStep <= 25.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerAnimRevolverReloadTest, "Looter.PlayerAnim.RevolverReload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerAnimRevolverReloadTest::RunTest(const FString& Parameters)
{
	// The Drover's reload had the rifle's magazine swap on the body, its left hand off to a well the six-gun hasn't got.
	// Its own now, on the gun's reload steps: the left hand rides the cylinder out, punches the rod, fetches a speedloader
	// from the belt and pushes it in, and bats the cylinder home as the gun hand flicks.
	const float Open = LooterReload::CylinderOpenDegrees;
	const FVector Closed = LooterStancePose::CylinderCenter(0.f);
	const FVector Swung = LooterStancePose::CylinderCenter(Open);
	TestTrue(FString::Printf(TEXT("The cylinder swings out to the gun's left (%s from %s)"), *Swung.ToCompactString(), *Closed.ToCompactString()),
		Swung.Y < Closed.Y - 3.0 && FMath::IsNearlyEqual(Swung.X, Closed.X));

	const float OutAt = 0.2f;
	const FLooterCylinderReload RidingOut = LooterStancePose::DescribeCylinderReload(OutAt);
	const FVector CylinderThen = LooterStancePose::CylinderCenter(LooterReload::CylinderSwing(OutAt));
	TestTrue(FString::Printf(TEXT("The left hand rides the cylinder out (%.1f cm from it)"), FVector::Dist(RidingOut.OnGun, CylinderThen)),
		FVector::Dist(RidingOut.OnGun, CylinderThen) < 2.5);
	TestTrue(TEXT("...on the gun, not off at the belt"), RidingOut.Pouch == 0.f && RidingOut.Flick == 0.f);
	float EjectAt = 0.3f;
	float CloseAt = 0.72f;
	for (const FReloadStepAt& Step : LooterReload::Steps(EWeaponReloadPart::Cylinder))
	{
		EjectAt = Step.Step == EReloadStep::Eject ? Step.Progress : EjectAt;
		CloseAt = Step.Step == EReloadStep::CylinderIn ? Step.Progress : CloseAt;
	}
	TestTrue(TEXT("The palm punches the rod ahead of the cylinder as the empties go"), LooterStancePose::DescribeCylinderReload(EjectAt).OnGun.X > Swung.X + 3.0);
	TestTrue(TEXT("It goes to the belt for the speedloader"), LooterStancePose::DescribeCylinderReload(0.43f).Pouch > 0.9f);
	TestTrue(TEXT("The gun hand flicks as the cylinder latches"), LooterStancePose::DescribeCylinderReload(CloseAt).Flick > 0.9f);
	TestTrue(TEXT("...and the hand rests on the shut cylinder after"),
		FVector::Dist(LooterStancePose::DescribeCylinderReload(0.8f).OnGun, Closed) < 2.5);

	// Never a jump: through the whole reload, a step of 0.2% (under a third of a frame at its 2.3 s) moves the hand under 1 cm.
	float WorstMove = 0.f;
	float WorstPouch = 0.f;
	FLooterCylinderReload Previous = LooterStancePose::DescribeCylinderReload(0.f);
	for (float Progress = 0.002f; Progress <= 1.f; Progress += 0.002f)
	{
		const FLooterCylinderReload Now = LooterStancePose::DescribeCylinderReload(Progress);
		WorstMove = FMath::Max(WorstMove, static_cast<float>(FVector::Dist(Now.OnGun, Previous.OnGun)));
		WorstPouch = FMath::Max(WorstPouch, FMath::Abs(Now.Pouch - Previous.Pouch));
		Previous = Now;
	}
	TestTrue(FString::Printf(TEXT("The hand's spot never jumps (%.2f cm a step at most)"), WorstMove), WorstMove < 1.f);
	TestTrue(FString::Printf(TEXT("Nor its trip to the belt (%.3f of the way a step)"), WorstPouch), WorstPouch < 0.08f);

	USkeleton* Skeleton = MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();
	const FLooterStanceInput Held = SixGunInHand(Rig, Rest);

	// Held: the support palm sits out past the shooting hand's fingers on the grip's left, not in them.
	FCompactPose HeldPose = Rest;
	LooterStancePose::Apply(HeldPose, Held);
	FCSPose<FCompactPose> HeldSpace;
	HeldSpace.InitPose(HeldPose);
	const FVector Support = Held.WeaponRotation.UnrotateVector(PalmIn(Rig, HeldSpace) - GripIn(Rig, HeldSpace, Held));
	TestTrue(FString::Printf(TEXT("Held: the support palm clears the shooting hand (%.1f cm left of the grip)"), -Support.Y), Support.Y < -4.5);

	// Zero input: a reload that doesn't show yet (no weight) leaves the hold as it is.
	FLooterStanceInput Unweighted = Held;
	Unweighted.CylinderReload = 0.4f;
	FCompactPose UnweightedPose = Rest;
	LooterStancePose::Apply(UnweightedPose, Unweighted);
	TestTrue(TEXT("A reload with no weight changes nothing"), SamePose(HeldPose, UnweightedPose));

	// Under way: the left hand goes from the grip to the cylinder swinging out.
	FLooterStanceInput Reloading = Held;
	Reloading.CylinderReload = OutAt;
	Reloading.ReloadWeight = 1.f;
	FCompactPose ReloadPose = Rest;
	LooterStancePose::Apply(ReloadPose, Reloading);
	TestTrue(TEXT("Reload: sound"), FPoseRig::IsSound(ReloadPose));
	FCSPose<FCompactPose> ReloadSpace;
	ReloadSpace.InitPose(ReloadPose);
	const FVector Cylinder = GripIn(Rig, ReloadSpace, Reloading) + Reloading.WeaponRotation.RotateVector(RidingOut.OnGun);
	const float AtCylinder = static_cast<float>(FVector::Dist(PalmIn(Rig, ReloadSpace), Cylinder));
	const float WasFrom = static_cast<float>(FVector::Dist(PalmIn(Rig, HeldSpace), Cylinder));
	TestTrue(FString::Printf(TEXT("Reload: the left hand is at the cylinder (%.1f cm from it, %.1f on the grip)"), AtCylinder, WasFrom),
		AtCylinder < 3.f && WasFrom > AtCylinder + 4.f);

	// At the belt for the speedloader the hand is down by the hips, well under the gun.
	Reloading.CylinderReload = 0.43f;
	FCompactPose PouchPose = Rest;
	LooterStancePose::Apply(PouchPose, Reloading);
	TestTrue(FString::Printf(TEXT("Reload: down at the belt (%.1f cm under the cylinder)"), Cylinder.Z - Rig.Where(PouchPose, TEXT("hand_l")).Z),
		Rig.Where(PouchPose, TEXT("hand_l")).Z < Cylinder.Z - 15.0);
	return true;
}

#endif
