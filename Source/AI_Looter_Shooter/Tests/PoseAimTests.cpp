#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Embers/HeroDefinition.h"
#include "Player/Animation/HeroPoseTuning.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseCarry.h"
#include "Tests/PlayerAnimTestKit.h"
#include "TwoBoneIK.h"

// Aiming down the sights in third person (the user, 2026-10-22; LooterStancePoseAim.cpp): on the mannequin's reference
// pose with a gun held at the chest as the hip hold has it, the aim raises it until the gun's sight line runs through the eye
// with the grip its kind's reach ahead, for all seven kinds, pitched up and down, with a hero's hold; half aimed is half way;
// a reload or a carry keeps it down; the head drops onto it, the elbow stays under it and the left palm stays on the foregrip.

namespace
{
	const FVector AimAhead(0.0, 1.0, 0.0);
	const FVector AimRight(-1.0, 0.0, 0.0);
	constexpr float AimFacingYaw = 90.f;
	const FVector AimPalmFromWrist(7.5, -2.5, 0.0);

	FTransform AimBone(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Pose, const TCHAR* Name)
	{
		FCSPose<FCompactPose> Space;
		Space.InitPose(Pose);
		FBoneReference Bone(Name);
		Bone.Initialize(Rig.Bones);
		const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(Rig.Bones);
		return Index.IsValid() ? Space.GetComponentSpaceTransform(Index) : FTransform::Identity;
	}

	/** The reference pose with the right wrist up in front of the right chest, as the armed upper body holds a gun at the hip. */
	FCompactPose AimHipHold(const PlayerAnimTestKit::FPoseRig& Rig)
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
		AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Grip, Elbow.GetLocation() + AimRight * 10.0 - FVector(0.0, 0.0, 15.0), Wrist,
			/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
		Grip.SetRotation(HandRotation);
		const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Grip) };
		Space.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
		FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Space), Posed);
		return Posed;
	}

	/** A gun of Kind held in the hip pose, aimed Pitch degrees up, Aimed of the way to the sights. */
	struct FAimCase
	{
		EWeaponKind Kind = EWeaponKind::Rifle;
		float Pitch = 0.f;
		float Aimed = 1.f;
		FVector Hold = FVector::ZeroVector;
		float Reload = 0.f;
		FLooterCarry Carry;
	};

	struct FAimed
	{
		FCompactPose Pose;
		FLooterStanceInput Input;
		FVector Grip = FVector::ZeroVector;
		FVector Sight = FVector::ZeroVector;
		FVector Eye = FVector::ZeroVector;
		FVector Palm = FVector::ZeroVector;
		FVector Foregrip = FVector::ZeroVector;
		FTransform Head;
		FVector ElbowR = FVector::ZeroVector;
		FVector WristR = FVector::ZeroVector;
	};

	FAimed AimPose(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& HipHold, const FAimCase& Case)
	{
		const bool bSixGun = Case.Kind == EWeaponKind::Revolver;
		const FTransform Hand = AimBone(Rig, HipHold, TEXT("hand_r"));
		FLooterStanceInput Held;
		Held.bAimWithTorso = true;
		Held.AimPitch = Case.Pitch;
		Held.bHandOnForegrip = true;
		Held.HoldBone = TEXT("hand_r");
		Held.HoldSocketLocal = FTransform(Hand.GetLocation() + AimAhead * 6.0 - FVector(0.0, 0.0, 2.0)).GetRelativeTransform(Hand);
		Held.WeaponKind = Case.Kind;
		Held.HoldReach = bSixGun ? 18.f : 0.f;
		Held.WeaponForegrip = bSixGun ? FVector(1.0, -3.2, -2.0) : FVector(21.5, 0.0, 5.5);
		Held.WeaponSight = bSixGun ? FVector(12.0, 0.0, 9.0) : FVector(8.0, 0.0, 10.0);
		Held.SightGripAhead = HeroPoseTuning::DefaultGripAhead(Case.Kind);
		Held.EyeFromHead = HeroPoseTuning::DefaultEyeFromHead();
		Held.HoldOffset = Case.Hold;
		Held.ReloadWeight = Case.Reload;
		Held.Carry = Case.Carry;
		Held.AimRotation = FRotator(Case.Pitch, AimFacingYaw, 0.f).Quaternion();
		Held.CarryRotation = LooterCarry::CarriedGun(Held.AimRotation, AimFacingYaw, Case.Carry);
		Held.WeaponRotation = Held.CarryRotation;
		Held.AimDownSights = Case.Aimed;

		FAimed Out;
		Out.Input = Held;
		Out.Pose = HipHold;
		LooterStancePose::Apply(Out.Pose, Held);
		const FTransform HandR = AimBone(Rig, Out.Pose, TEXT("hand_r"));
		Out.Grip = (Held.HoldSocketLocal * HandR).GetLocation();
		Out.Sight = Out.Grip + Held.WeaponRotation.RotateVector(Held.WeaponSight);
		Out.Head = AimBone(Rig, Out.Pose, TEXT("head"));
		// The eye: the head and the eye's offset in the body's frame pitched with the aim, and the hero's hold pitched the same.
		const FQuat Pitch(FVector::CrossProduct(FVector::UpVector, AimAhead), FMath::DegreesToRadians(-Case.Pitch));
		const FVector Eye = Held.EyeFromHead;
		Out.Eye = Out.Head.GetLocation() + Pitch.RotateVector(AimAhead * Eye.X + AimRight * Eye.Y + FVector::UpVector * Eye.Z)
			+ Pitch.RotateVector(AimAhead * Case.Hold.X + AimRight * Case.Hold.Y + FVector::UpVector * Case.Hold.Z);
		Out.Palm = AimBone(Rig, Out.Pose, TEXT("hand_l")).TransformPosition(AimPalmFromWrist);
		FVector OnGun = Held.WeaponForegrip;
		if (bSixGun)
		{
			OnGun += FVector(0.0, -2.0, -1.0);
		}
		Out.Foregrip = Out.Grip + Held.WeaponRotation.RotateVector(OnGun);
		Out.ElbowR = AimBone(Rig, Out.Pose, TEXT("lowerarm_r")).GetLocation();
		Out.WristR = HandR.GetLocation();
		return Out;
	}

	/** How far the sight is off the line from the eye along the aim (cm), and the grip's reach ahead of the eye along it. */
	void SightLine(const FAimed& Aimed, float Pitch, double& OutOff, double& OutReach)
	{
		const FVector Way = FRotator(Pitch, AimFacingYaw, 0.f).Vector();
		OutOff = FVector::VectorPlaneProject(Aimed.Sight - Aimed.Eye, Way).Size();
		OutReach = FVector::DotProduct(Aimed.Grip - Aimed.Eye, Way);
	}

	FString KindName(EWeaponKind Kind)
	{
		return StaticEnum<EWeaponKind>()->GetNameStringByValue(static_cast<int64>(Kind));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoseAimRaiseTest, "Looter.Pose.Aim.Raise",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPoseAimRaiseTest::RunTest(const FString& Parameters)
{
	USkeleton* Skeleton = PlayerAnimTestKit::MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const PlayerAnimTestKit::FPoseRig Rig(*Skeleton);
	const FCompactPose HipHold = AimHipHold(Rig);

	// Every kind, level and pitched: the sight on the eye's line, the grip its reach ahead, the left palm on the gun.
	for (const EWeaponKind Kind : AllGunKinds)
	{
		const double Tolerance = Kind == EWeaponKind::Revolver ? 3.0 : 1.5;
		for (const float Pitch : { 0.f, 30.f, -30.f })
		{
			FAimCase Case;
			Case.Kind = Kind;
			Case.Pitch = Pitch;
			const FAimed Aimed = AimPose(Rig, HipHold, Case);
			double Off = 0.0;
			double Reach = 0.0;
			SightLine(Aimed, Pitch, Off, Reach);
			const FString Name = FString::Printf(TEXT("%s aimed %+.0f"), *KindName(Kind), Pitch);
			TestTrue(FString::Printf(TEXT("%s: sound"), *Name), PlayerAnimTestKit::FPoseRig::IsSound(Aimed.Pose));
			TestTrue(FString::Printf(TEXT("%s: the sight on the eye's line (%.2f cm off)"), *Name, Off), Off <= Tolerance);
			TestTrue(FString::Printf(TEXT("%s: the grip %.1f cm ahead of the eye (want %.1f)"), *Name, Reach, Aimed.Input.SightGripAhead),
				FMath::Abs(Reach - Aimed.Input.SightGripAhead) <= Tolerance);
			TestTrue(FString::Printf(TEXT("%s: the left palm on the gun (%.2f cm off)"), *Name, FVector::Dist(Aimed.Palm, Aimed.Foregrip)),
				FVector::Dist(Aimed.Palm, Aimed.Foregrip) <= 2.0);
		}
	}

	// From the hip: nothing moves; half aimed: about half way; up at the eye the head comes down onto it, the elbow under it.
	FAimCase Hip;
	Hip.Aimed = 0.f;
	FAimCase Half;
	Half.Aimed = 0.5f;
	FAimCase Full;
	const FAimed AtHip = AimPose(Rig, HipHold, Hip);
	const FAimed Halfway = AimPose(Rig, HipHold, Half);
	const FAimed Sighted = AimPose(Rig, HipHold, Full);
	const double Moved = FVector::Dist(Sighted.Grip, AtHip.Grip);
	const double HalfMoved = FVector::Dist(Halfway.Grip, AtHip.Grip);
	TestTrue(FString::Printf(TEXT("The gun comes up from the hip (%.1f cm)"), Moved), Moved > 8.0 && Sighted.Grip.Z > AtHip.Grip.Z + 5.0);
	TestTrue(FString::Printf(TEXT("Half aimed is about half way (%.1f of %.1f cm)"), HalfMoved, Moved), HalfMoved > Moved * 0.3 && HalfMoved < Moved * 0.7);
	const FVector HeadWay = (Sighted.Head.GetRotation() * AtHip.Head.GetRotation().Inverse()).RotateVector(AimAhead);
	TestTrue(FString::Printf(TEXT("The head comes down onto the gun (%.1f degrees)"), FMath::RadiansToDegrees(FMath::Asin(-HeadWay.Z))), HeadWay.Z < -0.05);
	TestTrue(FString::Printf(TEXT("The right elbow under the gun (%.1f cm below the wrist)"), Sighted.WristR.Z - Sighted.ElbowR.Z),
		Sighted.ElbowR.Z < Sighted.WristR.Z - 5.0);

	// A reload or a sprint's carry keeps the gun down.
	FAimCase Reloading;
	Reloading.Reload = 1.f;
	FAimCase ReloadingHip = Reloading;
	ReloadingHip.Aimed = 0.f;
	TestTrue(TEXT("Reloading, aiming raises nothing"),
		FVector::Dist(AimPose(Rig, HipHold, Reloading).Grip, AimPose(Rig, HipHold, ReloadingHip).Grip) <= 0.1);
	FAimCase Sprinting;
	Sprinting.Carry.Sprint = 1.f;
	FAimCase SprintingHip = Sprinting;
	SprintingHip.Aimed = 0.f;
	TestTrue(TEXT("Carried in a sprint, aiming raises nothing"),
		FVector::Dist(AimPose(Rig, HipHold, Sprinting).Grip, AimPose(Rig, HipHold, SprintingHip).Grip) <= 0.1);

	// A hero's hold (Crane's head 16.5 cm higher) lifts the eye the sight comes up to.
	FAimCase Tall;
	Tall.Hold = FVector(0.0, 0.0, 16.5);
	const FAimed TallAimed = AimPose(Rig, HipHold, Tall);
	double TallOff = 0.0;
	double TallReach = 0.0;
	SightLine(TallAimed, 0.f, TallOff, TallReach);
	TestTrue(FString::Printf(TEXT("A tall hero: the sight on their eye's line (%.2f cm off), above the mannequin's (%.1f cm)"), TallOff,
		TallAimed.Sight.Z - Sighted.Sight.Z), TallOff <= 2.0 && TallAimed.Sight.Z > Sighted.Sight.Z + 10.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoseAimDataTest, "Looter.Pose.Aim.Data",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPoseAimDataTest::RunTest(const FString& Parameters)
{
	// Every kind has a reach ahead of the eye; a six-gun's out at arm's length, longer than any shouldered gun's.
	for (const EWeaponKind Kind : AllGunKinds)
	{
		const float Reach = HeroPoseTuning::DefaultGripAhead(Kind);
		TestTrue(FString::Printf(TEXT("%s: the grip %.0f cm ahead of the eye"), *KindName(Kind), Reach), Reach >= 10.f && Reach <= 50.f);
		if (Kind != EWeaponKind::Revolver)
		{
			TestTrue(FString::Printf(TEXT("%s: shouldered, nearer than a six-gun"), *KindName(Kind)), Reach < HeroPoseTuning::DefaultGripAhead(EWeaponKind::Revolver));
		}
	}
	// Each hero raises the sights (the user wants it for every hero), with their head bowed onto the gun, elbows in.
	for (const FName Id : UHeroDefinition::RosterIds())
	{
		const UHeroDefinition* Hero = UHeroDefinition::Load(Id);
		if (TestNotNull(*FString::Printf(TEXT("%s's definition"), *Id.ToString()), Hero))
		{
			TestTrue(FString::Printf(TEXT("%s raises the sights to the eye (%.2f)"), *Id.ToString(), Hero->Aim.Raise), Hero->Aim.Raise >= 0.99f);
			TestTrue(FString::Printf(TEXT("%s bows onto the gun, elbows in"), *Id.ToString()), Hero->Aim.HeadDown > 0.f && Hero->Aim.ElbowIn > 0.f);
		}
	}
	return true;
}

#endif
