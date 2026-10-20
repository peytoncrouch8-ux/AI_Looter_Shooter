#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseCarry.h"
#include "Tests/PlayerAnimTestKit.h"
#include "TwoBoneIK.h"

// The left hand's grip in third person (Player/Animation/LooterStancePoseGrip.cpp). Every character's left hand rested open,
// palm up, under the foregrip (the rifle animation's hand; 2026-10-20). Now it's turned onto the gun by the hold (under a
// handguard, round a pump, round a six-gun's shooting hand) and its fingers close round it, letting go smoothly in a
// reload, a one-handed carry and a throw. Run on the mannequin's reference pose, bone by bone; the heroes' hands copy it.

namespace
{
	/** The mannequin's front and right in its component space (it faces +Y; its right is -X), and that facing's yaw. */
	const FVector GripAhead(0.0, 1.0, 0.0);
	const FVector GripRight(-1.0, 0.0, 0.0);
	constexpr float GripFacingYaw = 90.f;
	/** The shouldered right wrist from the right shoulder: up in front of the chest, in toward its middle. */
	const FVector GripShoulderedWrist(14.0, 16.0, -18.0);
	/** The left palm off hand_l (the mannequin's HandGrip_L point), as the stance layer has it. */
	const FVector GripPalmFromWrist(7.5, -2.5, 0.0);
	/** The guns' foregrips from their grips (Bullpup.py, Ranchhand.py's pump, Drover.py's support). */
	const FVector GripBullpupForegrip(21.5, 0.0, 5.5);
	const FVector GripShotgunForegrip(41.5, 0.0, 3.0);
	const FVector GripSixGunForegrip(0.7, -3.2, -0.8);
	/** The six-gun's support palm: past its foregrip on the grip's left, a little under (the stance layer's SupportClear, SupportDrop). */
	const FVector GripSixGunSupport(0.7, -5.2, -1.8);
	/** The four fingers, index to pinky (the thumb is posed too, but it lies along the gun rather than closing on it). */
	const TCHAR* const GripFingers[] = { TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") };

	FCompactPoseBoneIndex GripIndexOf(const PlayerAnimTestKit::FPoseRig& Rig, const TCHAR* Name)
	{
		FBoneReference Bone(Name);
		Bone.Initialize(Rig.Bones);
		return Bone.GetCompactPoseIndex(Rig.Bones);
	}

	FTransform GripBoneIn(const PlayerAnimTestKit::FPoseRig& Rig, FCSPose<FCompactPose>& Space, const TCHAR* Name)
	{
		const FCompactPoseBoneIndex Index = GripIndexOf(Rig, Name);
		return Index.IsValid() ? Space.GetComponentSpaceTransform(Index) : FTransform::Identity;
	}

	FTransform GripBoneIn(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Pose, const TCHAR* Name)
	{
		FCSPose<FCompactPose> Space;
		Space.InitPose(Pose);
		return GripBoneIn(Rig, Space, Name);
	}

	/** The reference pose with the right wrist up in front of the right chest, as the armed upper body holds a gun. */
	FCompactPose GripShouldered(const PlayerAnimTestKit::FPoseRig& Rig)
	{
		FCompactPose Posed = Rig.RefPose();
		FCSPose<FCompactPose> Space;
		Space.InitPose(Posed);
		const FCompactPoseBoneIndex UpperArm = GripIndexOf(Rig, TEXT("upperarm_r"));
		const FCompactPoseBoneIndex LowerArm = GripIndexOf(Rig, TEXT("lowerarm_r"));
		const FCompactPoseBoneIndex Hand = GripIndexOf(Rig, TEXT("hand_r"));
		if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
		{
			return Posed;
		}
		FTransform Shoulder = Space.GetComponentSpaceTransform(UpperArm);
		FTransform Elbow = Space.GetComponentSpaceTransform(LowerArm);
		FTransform Wrist = Space.GetComponentSpaceTransform(Hand);
		const FQuat HandRotation = Wrist.GetRotation();
		AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, Elbow.GetLocation() + GripRight * 10.0 - FVector(0.0, 0.0, 15.0),
			Shoulder.GetLocation() + GripShoulderedWrist, /*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
		Wrist.SetRotation(HandRotation);
		const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
		Space.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
		FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Space), Posed);
		return Posed;
	}

	/**
	 * A gun of Kind in the right hand of Shouldered, as the anim instance hands it over: its grip 6 cm ahead of the wrist and
	 * 2 under, pointing the way the mannequin faces (aimed Pitch degrees up, Kick more in the gun's own pitch, carried as
	 * Carry says), its foregrip at Foregrip from the grip.
	 */
	FLooterStanceInput GripGunIn(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Shouldered, const FVector& Foregrip, float HoldReach,
		EWeaponKind Kind, float Pitch = 0.f, float Kick = 0.f, const FLooterCarry& Carry = FLooterCarry())
	{
		const FTransform Hand = GripBoneIn(Rig, Shouldered, TEXT("hand_r"));
		FLooterStanceInput Held;
		Held.bAimWithTorso = true;
		Held.AimPitch = Pitch;
		Held.bHandOnForegrip = true;
		Held.HoldBone = TEXT("hand_r");
		Held.HoldSocketLocal = FTransform(Hand.GetLocation() + GripAhead * 6.0 - FVector(0.0, 0.0, 2.0)).GetRelativeTransform(Hand);
		Held.WeaponForegrip = Foregrip;
		Held.HoldReach = HoldReach;
		Held.WeaponKind = Kind;
		Held.SprintTorsoLean = 9.f;
		Held.Carry = Carry;
		Held.SprintAlpha = Carry.Sprint;
		Held.AimRotation = FRotator(Pitch, GripFacingYaw, 0.f).Quaternion();
		Held.CarryRotation = LooterCarry::CarriedGun(Held.AimRotation, GripFacingYaw, Carry);
		Held.WeaponRotation = (Held.CarryRotation * FRotator(Kick, 0.f, 0.f).Quaternion()).GetNormalized();
		return Held;
	}

	/** A posed left hand and the gun it holds, in component space. */
	struct FGripPosed
	{
		FCompactPose Pose;
		FTransform HandL;
		FVector Grip = FVector::ZeroVector;
		FQuat Gun = FQuat::Identity;
		/** The hand's frame off its bones: the wrist to the middle knuckle, the pinky's knuckle to the index's, the palm's facing. */
		FVector Knuckles = FVector::ZeroVector;
		FVector Across = FVector::ZeroVector;
		FVector PalmFacing = FVector::ZeroVector;
		/**
		 * Each finger (index to pinky): its knuckle's bend (the metacarpal's way to the first bone's) and its middle joint's
		 * (the first bone's to the middle bone's), degrees, and its last joint, where the fingertip's bone starts. (The
		 * mannequin's reference fingers, and the rifle animation's, already rest bent 40-60 degrees: what tells a grip from an
		 * open hand is where the bent fingers end up on the gun.)
		 */
		float KnuckleBend[UE_ARRAY_COUNT(GripFingers)] = {};
		float MiddleJointBend[UE_ARRAY_COUNT(GripFingers)] = {};
		FVector Tip[UE_ARRAY_COUNT(GripFingers)];

		FVector Palm() const { return HandL.TransformPosition(GripPalmFromWrist); }
		FVector OnGun(const FVector& Spot) const { return Grip + Gun.RotateVector(Spot); }
		FVector InGun(const FVector& Point) const { return Gun.UnrotateVector(Point - Grip); }
		FQuat InLeftHand() const { return HandL.GetRotation().Inverse() * Gun; }
	};

	float GripAngle(const FVector& A, const FVector& B)
	{
		return FMath::RadiansToDegrees(static_cast<float>(FMath::Acos(FMath::Clamp(FVector::DotProduct(A.GetSafeNormal(), B.GetSafeNormal()), -1.0, 1.0))));
	}

	FGripPosed GripMeasure(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Pose, const FLooterStanceInput& Held)
	{
		FGripPosed Out;
		Out.Pose = Pose;
		FCSPose<FCompactPose> Space;
		Space.InitPose(Pose);
		Out.HandL = GripBoneIn(Rig, Space, TEXT("hand_l"));
		Out.Grip = (Held.HoldSocketLocal * GripBoneIn(Rig, Space, TEXT("hand_r"))).GetLocation();
		Out.Gun = Held.WeaponRotation;
		const FVector Middle1 = Rig.Where(Space, TEXT("middle_01_l"));
		Out.Knuckles = (Middle1 - Out.HandL.GetLocation()).GetSafeNormal();
		Out.Across = (Rig.Where(Space, TEXT("index_01_l")) - Rig.Where(Space, TEXT("pinky_01_l"))).GetSafeNormal();
		// A left hand's palm faces its knuckles' line cross its fingers' way (a right hand's is the other way round).
		Out.PalmFacing = FVector::VectorPlaneProject(FVector::CrossProduct(Out.Across, Out.Knuckles), Out.Knuckles).GetSafeNormal();
		for (int32 Finger = 0; Finger < UE_ARRAY_COUNT(GripFingers); ++Finger)
		{
			const FVector Hand = Rig.Where(Space, *FString::Printf(TEXT("%s_metacarpal_l"), GripFingers[Finger]));
			const FVector Base = Rig.Where(Space, *FString::Printf(TEXT("%s_01_l"), GripFingers[Finger]));
			const FVector Second = Rig.Where(Space, *FString::Printf(TEXT("%s_02_l"), GripFingers[Finger]));
			Out.Tip[Finger] = Rig.Where(Space, *FString::Printf(TEXT("%s_03_l"), GripFingers[Finger]));
			Out.KnuckleBend[Finger] = GripAngle(Base - Hand, Second - Base);
			Out.MiddleJointBend[Finger] = GripAngle(Second - Base, Out.Tip[Finger] - Second);
		}
		return Out;
	}

	FGripPosed GripPoseOf(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Shouldered, const FLooterStanceInput& Held)
	{
		FCompactPose Pose = Shouldered;
		LooterStancePose::Apply(Pose, Held);
		return GripMeasure(Rig, Pose, Held);
	}

	/** The most any left finger or thumb joint's own turn differs between two poses (degrees). */
	float GripFingersApart(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& A, const FCompactPose& B)
	{
		float Worst = 0.f;
		for (const TCHAR* Finger : { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") })
		{
			for (int32 Joint = 1; Joint <= 3; ++Joint)
			{
				const FCompactPoseBoneIndex Index = GripIndexOf(Rig, *FString::Printf(TEXT("%s_%02d_l"), Finger, Joint));
				if (Index.IsValid())
				{
					Worst = FMath::Max(Worst, FMath::RadiansToDegrees(static_cast<float>(A[Index].GetRotation().AngularDistance(B[Index].GetRotation()))));
				}
			}
		}
		return Worst;
	}

	float GripDegrees(const FQuat& A, const FQuat& B)
	{
		return FMath::RadiansToDegrees(static_cast<float>(A.AngularDistance(B)));
	}

	/**
	 * Every finger closed round what the hand holds: both its joints bent 40 degrees or more, and its last joint round to the
	 * gun's right of the palm's spot by MinRight cm or more (a long gun's far side, a six-gun's shooting hand), and with bUp,
	 * 1 cm or more above it too. (The hidden mannequin's hand is smaller than the heroes' it's copied onto: under a handguard
	 * its outer fingers close back toward the middle, where the heroes' longer ones lie on the far side.)
	 */
	void GripTestClosed(FAutomationTestBase& Test, const FGripPosed& Posed, const TCHAR* Round, float MinRight, bool bUp)
	{
		const FVector Palm = Posed.InGun(Posed.Palm());
		for (int32 Finger = 0; Finger < UE_ARRAY_COUNT(GripFingers); ++Finger)
		{
			const FVector Tip = Posed.InGun(Posed.Tip[Finger]) - Palm;
			Test.TestTrue(FString::Printf(TEXT("The %s finger closed (%.0f and %.0f degrees)"), GripFingers[Finger], Posed.KnuckleBend[Finger],
				Posed.MiddleJointBend[Finger]), Posed.KnuckleBend[Finger] >= 40.f && Posed.MiddleJointBend[Finger] >= 40.f);
			Test.TestTrue(FString::Printf(TEXT("...round %s (its last joint %.1f cm right of the palm, %.1f up)"), Round, Tip.Y, Tip.Z),
				Tip.Y >= MinRight && (!bUp || Tip.Z >= 1.0));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceGripRifleTest, "Looter.Anim.Grip.Rifle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceGripRifleTest::RunTest(const FString& Parameters)
{
	// Under the Bullpup's handguard: the palm on the foregrip facing up into it, the knuckles across under it to its far
	// side, index and thumb along the barrel, every finger closed up round it; aimed up with a kick, held the same.
	USkeleton* Skeleton = PlayerAnimTestKit::MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const PlayerAnimTestKit::FPoseRig Rig(*Skeleton);
	const FCompactPose Shouldered = GripShouldered(Rig);
	const FLooterStanceInput Bullpup = GripGunIn(Rig, Shouldered, GripBullpupForegrip, 0.f, EWeaponKind::Rifle);
	const FGripPosed Held = GripPoseOf(Rig, Shouldered, Bullpup);
	const FVector Up = Held.Gun.GetUpVector();

	TestTrue(TEXT("Held: sound"), PlayerAnimTestKit::FPoseRig::IsSound(Held.Pose));
	TestTrue(FString::Printf(TEXT("The left palm on the foregrip (%.2f cm off)"), FVector::Dist(Held.Palm(), Held.OnGun(GripBullpupForegrip))),
		FVector::Dist(Held.Palm(), Held.OnGun(GripBullpupForegrip)) <= 1.0);
	TestTrue(FString::Printf(TEXT("The palm faces up into the handguard (%.2f)"), FVector::DotProduct(Held.PalmFacing, Up)),
		FVector::DotProduct(Held.PalmFacing, Up) >= 0.8);
	// HandGrip_L sits off hand_l on the palm's side: the stance layer's left-hand rule and the socket agree.
	const FVector GripSide = Held.HandL.GetRotation().RotateVector(FVector(0.0, GripPalmFromWrist.Y, 0.0)).GetSafeNormal();
	TestTrue(FString::Printf(TEXT("...the HandGrip_L side of the hand toward the gun (%.2f)"), FVector::DotProduct(GripSide, Up)),
		FVector::DotProduct(GripSide, Up) >= 0.5);
	TestTrue(FString::Printf(TEXT("The knuckles across under it to its far side (%.2f)"), FVector::DotProduct(Held.Knuckles, Held.Gun.GetRightVector())),
		FVector::DotProduct(Held.Knuckles, Held.Gun.GetRightVector()) >= 0.5);
	TestTrue(FString::Printf(TEXT("Index and thumb along the barrel (%.2f)"), FVector::DotProduct(Held.Across, Held.Gun.GetForwardVector())),
		FVector::DotProduct(Held.Across, Held.Gun.GetForwardVector()) >= 0.6);
	GripTestClosed(*this, Held, TEXT("the handguard's far side, up it"), 0.f, /*bUp*/ true);

	// The hand follows the gun, not the animation: aimed 15 up with 5 degrees of kick in it, it holds the gun as it does level.
	const FLooterStanceInput Kicked = GripGunIn(Rig, Shouldered, GripBullpupForegrip, 0.f, EWeaponKind::Rifle, 15.f, 5.f);
	const FGripPosed Raised = GripPoseOf(Rig, Shouldered, Kicked);
	TestTrue(TEXT("Aimed up, kicked: sound"), PlayerAnimTestKit::FPoseRig::IsSound(Raised.Pose));
	TestTrue(FString::Printf(TEXT("Aimed up, kicked: the palm on the foregrip (%.2f cm off)"), FVector::Dist(Raised.Palm(), Raised.OnGun(GripBullpupForegrip))),
		FVector::Dist(Raised.Palm(), Raised.OnGun(GripBullpupForegrip)) <= 1.5);
	TestTrue(FString::Printf(TEXT("...the hand turned with the gun (%.2f degrees off the level hold)"), GripDegrees(Held.InLeftHand(), Raised.InLeftHand())),
		GripDegrees(Held.InLeftHand(), Raised.InLeftHand()) <= 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceGripLetGoTest, "Looter.Anim.Grip.LetGo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceGripLetGoTest::RunTest(const FString& Parameters)
{
	// The fingers let go as the hand does, blended, never snapped: a reload opens them to the animation's, a six-gun carried
	// one-handed and a grenade throw too.
	USkeleton* Skeleton = PlayerAnimTestKit::MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const PlayerAnimTestKit::FPoseRig Rig(*Skeleton);
	const FCompactPose Shouldered = GripShouldered(Rig);
	const FLooterStanceInput Bullpup = GripGunIn(Rig, Shouldered, GripBullpupForegrip, 0.f, EWeaponKind::Rifle);
	const FGripPosed Held = GripPoseOf(Rig, Shouldered, Bullpup);
	// How far the grip has the fingers from the animation's (here the reference pose's), at the joint it moves most.
	const float Gripped = GripFingersApart(Rig, Held.Pose, Shouldered);
	if (!TestTrue(FString::Printf(TEXT("Held, the grip poses the fingers (%.1f degrees off the animation's at most)"), Gripped), Gripped >= 5.f))
	{
		return false;
	}

	// A reload coming in a tenth at a time: the fingers go back to the animation's steadily, never in a jump, half way at half.
	float Last = Gripped;
	float WorstStep = 0.f;
	float HalfWay = 0.f;
	bool bOpening = true;
	bool bSound = true;
	FCompactPose Reloaded = Shouldered;
	for (int32 Step = 1; Step <= 10; ++Step)
	{
		FLooterStanceInput Reloading = Bullpup;
		Reloading.ReloadWeight = Step / 10.f;
		FCompactPose Frame = Shouldered;
		LooterStancePose::Apply(Frame, Reloading);
		bSound &= PlayerAnimTestKit::FPoseRig::IsSound(Frame);
		const float Now = GripFingersApart(Rig, Frame, Shouldered);
		WorstStep = FMath::Max(WorstStep, FMath::Abs(Now - Last));
		bOpening &= Now <= Last + 0.05f;
		Last = Now;
		if (Step == 5)
		{
			HalfWay = Now;
		}
		if (Step == 10)
		{
			Reloaded = Frame;
		}
	}
	TestTrue(TEXT("Reloading: every pose sound"), bSound);
	TestTrue(TEXT("Reloading: the fingers let go steadily"), bOpening);
	TestTrue(FString::Printf(TEXT("...never in a jump (%.2f degrees a tenth at most, of %.1f)"), WorstStep, Gripped), WorstStep <= 0.3f * Gripped);
	TestTrue(FString::Printf(TEXT("...half way, half let go (%.1f degrees of %.1f left)"), HalfWay, Gripped), HalfWay > 0.3f * Gripped && HalfWay < 0.7f * Gripped);
	TestTrue(FString::Printf(TEXT("Reloading in full: the animation's fingers (%.3f degrees off)"), GripFingersApart(Rig, Reloaded, Shouldered)),
		GripFingersApart(Rig, Reloaded, Shouldered) <= 0.05f);

	// A six-gun carried one-handed in a sprint: the left hand let go, fingers and all.
	FLooterCarry Sprint;
	Sprint.Sprint = 1.f;
	Sprint.bShortGun = true;
	FCompactPose Running = Shouldered;
	LooterStancePose::Apply(Running, GripGunIn(Rig, Shouldered, GripSixGunForegrip, 14.f, EWeaponKind::Revolver, 0.f, 0.f, Sprint));
	TestTrue(FString::Printf(TEXT("One-handed sprint: the animation's fingers (%.3f degrees off)"), GripFingersApart(Rig, Running, Shouldered)),
		GripFingersApart(Rig, Running, Shouldered) <= 0.05f);

	// A grenade throw takes the hand off the gun: at the throw's fullest the grip has let go as far as the arm has come in.
	float FullAt = -1.f;
	float FullArm = 0.f;
	for (float Time = 0.f; Time <= 3.f; Time += 0.01f)
	{
		const float Arm = LooterStancePose::DescribeThrow(Time).Arm;
		if (Arm > FullArm)
		{
			FullArm = Arm;
			FullAt = Time;
		}
	}
	if (TestTrue(FString::Printf(TEXT("A throw's arm comes in (%.2f at %.2f s)"), FullArm, FullAt), FullArm > 0.5f))
	{
		FLooterStanceInput Throwing = Bullpup;
		Throwing.ThrowTime = FullAt;
		FCompactPose Thrown = Shouldered;
		LooterStancePose::Apply(Thrown, Throwing);
		TestTrue(TEXT("Throwing: sound"), PlayerAnimTestKit::FPoseRig::IsSound(Thrown));
		const float Left = GripFingersApart(Rig, Thrown, Shouldered);
		TestTrue(FString::Printf(TEXT("Throwing: the grip let go (%.2f of its %.1f degrees left, the arm %.2f in)"), Left, Gripped, FullArm),
			Left <= Gripped * (1.f - FullArm) + 0.5f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceGripPumpTest, "Looter.Anim.Grip.Pump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceGripPumpTest::RunTest(const FString& Parameters)
{
	// Round the Ranchhand's pump (at the low-ready, which draws the long gun in until the left hand has its pump): cupped
	// from the lower left, every finger closed round it and up its far side.
	USkeleton* Skeleton = PlayerAnimTestKit::MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const PlayerAnimTestKit::FPoseRig Rig(*Skeleton);
	const FCompactPose Shouldered = GripShouldered(Rig);
	FLooterCarry Low;
	Low.LowReady = 1.f;
	const FLooterStanceInput Shotgun = GripGunIn(Rig, Shouldered, GripShotgunForegrip, 0.f, EWeaponKind::Shotgun, 0.f, 0.f, Low);
	const FGripPosed Pumped = GripPoseOf(Rig, Shouldered, Shotgun);

	TestTrue(TEXT("Pump: sound"), PlayerAnimTestKit::FPoseRig::IsSound(Pumped.Pose));
	TestTrue(FString::Printf(TEXT("The left palm on the pump (%.2f cm off)"), FVector::Dist(Pumped.Palm(), Pumped.OnGun(GripShotgunForegrip))),
		FVector::Dist(Pumped.Palm(), Pumped.OnGun(GripShotgunForegrip)) <= 1.0);
	const float Upward = static_cast<float>(FVector::DotProduct(Pumped.PalmFacing, Pumped.Gun.GetUpVector()));
	const float Inward = static_cast<float>(FVector::DotProduct(Pumped.PalmFacing, Pumped.Gun.GetRightVector()));
	TestTrue(FString::Printf(TEXT("The palm cups it from the lower left (%.2f up, %.2f in)"), Upward, Inward), Upward >= 0.5f && Inward >= 0.2f);
	GripTestClosed(*this, Pumped, TEXT("the pump's far side, up it"), 0.f, /*bUp*/ true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceGripSixGunTest, "Looter.Anim.Grip.SixGun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceGripSixGunTest::RunTest(const FString& Parameters)
{
	// The Drover held out in both hands: the support hand under the shooting hand and left of it, its palm toward it and a
	// little up, its fingers closed round the front of the shooting hand's.
	USkeleton* Skeleton = PlayerAnimTestKit::MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const PlayerAnimTestKit::FPoseRig Rig(*Skeleton);
	const FCompactPose Shouldered = GripShouldered(Rig);
	const FLooterStanceInput SixGun = GripGunIn(Rig, Shouldered, GripSixGunForegrip, 14.f, EWeaponKind::Revolver);
	const FGripPosed Held = GripPoseOf(Rig, Shouldered, SixGun);

	TestTrue(TEXT("Six-gun: sound"), PlayerAnimTestKit::FPoseRig::IsSound(Held.Pose));
	TestTrue(FString::Printf(TEXT("The support palm on its spot (%.2f cm off)"), FVector::Dist(Held.Palm(), Held.OnGun(GripSixGunSupport))),
		FVector::Dist(Held.Palm(), Held.OnGun(GripSixGunSupport)) <= 1.5);
	const FVector Palm = Held.InGun(Held.Palm());
	TestTrue(FString::Printf(TEXT("The support palm under the grip and left of it (%s cm)"), *Palm.ToCompactString()), Palm.Z < -0.5 && Palm.Y < -3.5);
	const FVector Knuckle = Held.InGun(Rig.Where(Held.Pose, TEXT("middle_01_l")));
	TestTrue(FString::Printf(TEXT("Its knuckles under the shooting hand's grip (%.1f cm)"), Knuckle.Z), Knuckle.Z < 0.0);
	const float Toward = static_cast<float>(FVector::DotProduct(Held.PalmFacing, Held.Gun.GetRightVector()));
	const float Upward = static_cast<float>(FVector::DotProduct(Held.PalmFacing, Held.Gun.GetUpVector()));
	TestTrue(FString::Printf(TEXT("Its palm toward the shooting hand and a little up (%.2f, %.2f)"), Toward, Upward), Toward >= 0.5f && Upward >= 0.1f);
	GripTestClosed(*this, Held, TEXT("the front of the shooting hand"), 1.f, /*bUp*/ false);
	return true;
}

#endif
