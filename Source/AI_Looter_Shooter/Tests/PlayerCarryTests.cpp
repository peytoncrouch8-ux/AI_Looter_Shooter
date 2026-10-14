#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseCarry.h"
#include "Tests/PlayerAnimTestKit.h"
#include "TwoBoneIK.h"

// The third-person carry (Player/Animation/LooterStancePoseCarry.h), without a world. The user's report (2026-10-14):
// "When running in F5, the weapon looks goofy": the gun stayed shouldered at the crosshair through every walk and run, and a
// sprint spun it in a fixed palm, its stock swinging into the arm. Now: a low-ready on the move at ease, back to the shoulder
// at once on an aim or a shot; across the chest in a sprint (a six-gun low at the side in one hand), the hands turning with
// the gun; less of the run's bounce in the upper body. Run on the mannequin's reference pose, bone by bone.

namespace
{
	constexpr float CarryFrame = 1.f / 60.f;

	/** The mannequin's front and its right in its component space (it faces +Y; its right is -X), and the facing's yaw there. */
	const FVector CarryAhead(0.0, 1.0, 0.0);
	const FVector CarryRight(-1.0, 0.0, 0.0);
	constexpr float CarryFacingYaw = 90.f;

	/** The left palm (the mannequin's HandGrip_L point) off the hand_l bone, where the stance layer puts what that hand holds. */
	const FVector CarryPalmFromWrist(7.5, -2.5, 0.0);

	/**
	 * The shouldered right wrist from the right shoulder: up in front of the chest, in toward its middle (the reference
	 * pose's own arms hang at its sides, out of the other hand's reach of a gun).
	 */
	const FVector CarryShoulderedWrist(14.0, 16.0, -18.0);

	/** A bullpup's foregrip from its grip (Bullpup.py: 21.5 cm ahead, 5.5 up), a shotgun's pump (Ranchhand.py), a six-gun's support (Drover.py). */
	const FVector CarryBullpupForegrip(21.5, 0.0, 5.5);
	const FVector CarryShotgunForegrip(41.5, 0.0, 3.0);
	const FVector CarrySixGunForegrip(0.7, -3.2, -0.8);

	/** Runs Clock Seconds at 60 frames a second; the biggest change of its alpha in a frame goes into MaxStep. */
	float CarryRunClock(FLooterReadyClock& Clock, float Seconds, bool bAlert, bool bMoving, float& MaxStep)
	{
		float Alpha = Clock.GetAlpha();
		const int32 Frames = FMath::RoundToInt(Seconds / CarryFrame);
		for (int32 Index = 0; Index < Frames; ++Index)
		{
			Clock.Update(CarryFrame, bAlert, bMoving);
			MaxStep = FMath::Max(MaxStep, FMath::Abs(Clock.GetAlpha() - Alpha));
			Alpha = Clock.GetAlpha();
		}
		return Alpha;
	}

	float CarryDegrees(const FQuat& A, const FQuat& B)
	{
		return FMath::RadiansToDegrees(static_cast<float>(A.AngularDistance(B)));
	}

	/** A bone's whole transform in component space once Pose is posed; identity for a bone the skeleton lacks. */
	FTransform CarryBone(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Pose, const TCHAR* Name)
	{
		FCSPose<FCompactPose> Space;
		Space.InitPose(Pose);
		FBoneReference Bone(Name);
		Bone.Initialize(Rig.Bones);
		const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(Rig.Bones);
		return Index.IsValid() ? Space.GetComponentSpaceTransform(Index) : FTransform::Identity;
	}

	/**
	 * The reference pose with the right wrist brought to Wrist (component space), the elbow bent down and out, the hand as it
	 * was: a gun held up in front of the right chest, as the armed upper body holds it shouldered. (The reference pose's arm
	 * hangs nearly straight, where no carry could lower it.)
	 */
	FCompactPose CarryRightHandAt(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Rest, const FVector& Wrist)
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
		AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Grip, Elbow.GetLocation() + CarryRight * 10.0 - FVector(0.0, 0.0, 15.0), Wrist,
			/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
		Grip.SetRotation(HandRotation);
		const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Grip) };
		Space.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
		FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Space), Posed);
		return Posed;
	}

	/**
	 * A gun in the right hand of Shouldered, as the anim instance hands it to the stance layer: its grip 6 cm ahead of the
	 * wrist and 2 under, pointing the way the mannequin faces, its foregrip at Foregrip from the grip (the gun's own space),
	 * the sprint's lean as the game has it.
	 */
	FLooterStanceInput CarryGunIn(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Shouldered, const FVector& Foregrip, float HoldReach)
	{
		const FTransform Hand = CarryBone(Rig, Shouldered, TEXT("hand_r"));
		FLooterStanceInput Held;
		Held.bAimWithTorso = true;
		Held.bHandOnForegrip = true;
		Held.HoldBone = TEXT("hand_r");
		Held.HoldSocketLocal = FTransform(Hand.GetLocation() + CarryAhead * 6.0 - FVector(0.0, 0.0, 2.0)).GetRelativeTransform(Hand);
		Held.WeaponForegrip = Foregrip;
		Held.HoldReach = HoldReach;
		Held.SprintTorsoLean = 9.f;
		Held.AimRotation = FRotator(0.f, CarryFacingYaw, 0.f).Quaternion();
		Held.CarryRotation = Held.AimRotation;
		Held.WeaponRotation = Held.AimRotation;
		return Held;
	}

	/** One carried pose and where its gun and hands are. */
	struct FCarryPosed
	{
		FCompactPose Pose;
		FQuat Gun = FQuat::Identity;
		FTransform HandR;
		FTransform HandL;
		FVector Grip = FVector::ZeroVector;
		FVector ShoulderR = FVector::ZeroVector;

		/** The gun as the right hand holds it (its turn seen from the hand), and as the left palm holds it. */
		FQuat InRightHand() const { return HandR.GetRotation().Inverse() * Gun; }
		FQuat InLeftHand() const { return HandL.GetRotation().Inverse() * Gun; }
		FVector Palm() const { return HandL.TransformPosition(CarryPalmFromWrist); }
		FVector OnGun(const FVector& Spot) const { return Grip + Gun.RotateVector(Spot); }
	};

	/** Held carried as Carry says, the gun turned as the view turns it and the sprint's lean with it, posed on Shouldered. */
	FCarryPosed CarryPoseOf(const PlayerAnimTestKit::FPoseRig& Rig, const FCompactPose& Shouldered, FLooterStanceInput Held, const FLooterCarry& Carry)
	{
		Held.Carry = Carry;
		Held.SprintAlpha = Carry.Sprint;
		Held.CarryRotation = LooterCarry::CarriedGun(Held.AimRotation, CarryFacingYaw, Carry);
		Held.WeaponRotation = Held.CarryRotation;
		FCarryPosed Posed;
		Posed.Pose = Shouldered;
		LooterStancePose::Apply(Posed.Pose, Held);
		Posed.Gun = Held.WeaponRotation;
		Posed.HandR = CarryBone(Rig, Posed.Pose, TEXT("hand_r"));
		Posed.HandL = CarryBone(Rig, Posed.Pose, TEXT("hand_l"));
		Posed.Grip = (Held.HoldSocketLocal * Posed.HandR).GetLocation();
		Posed.ShoulderR = CarryBone(Rig, Posed.Pose, TEXT("upperarm_r")).GetLocation();
		return Posed;
	}

	FLooterCarry CarryOfShares(float LowReady, float Sprint, bool bShortGun)
	{
		FLooterCarry Carry;
		Carry.LowReady = LowReady;
		Carry.Sprint = Sprint;
		Carry.bShortGun = bShortGun;
		return Carry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerCarryReadyClockTest, "Looter.Player.Carry.ReadyClock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerCarryReadyClockTest::RunTest(const FString& Parameters)
{
	// On the move at ease the gun goes to the low-ready after the delay, eased; an aim or a shot snaps it back to the shoulder
	// in RaiseSeconds (the one quick move), and the delay starts over; standing still keeps it up; a stop lets it settle up.
	const float Delay = LooterCarry::ReadyDelay;
	float Easing = 0.f;
	FLooterReadyClock Clock;
	TestEqual(TEXT("Shouldered through the delay"), CarryRunClock(Clock, Delay - 0.05f, false, true, Easing), 0.f);
	const float Lowered = CarryRunClock(Clock, 0.05f + LooterCarry::LowerSeconds + 0.05f, false, true, Easing);
	TestTrue(FString::Printf(TEXT("Lowered once the delay and the ease are over (%.3f)"), Lowered), Lowered >= 0.99f);
	TestTrue(FString::Printf(TEXT("...eased down, never in a jump (%.3f of the way in a frame at most)"), Easing), Easing <= 0.08f);

	// An aim or a shot: one frame of it is enough, and the gun is up by the raise's time.
	float Snap = 0.f;
	CarryRunClock(Clock, CarryFrame, true, true, Snap);
	const float Raised = CarryRunClock(Clock, LooterCarry::RaiseSeconds, false, true, Snap);
	TestTrue(FString::Printf(TEXT("An aim or a shot snaps it back to the shoulder in %.2f s (%.3f left)"), LooterCarry::RaiseSeconds, Raised), Raised <= 0.01f);
	TestTrue(FString::Printf(TEXT("...quick, but on a curve (%.2f of the way in a frame at most)"), Snap), Snap > 0.1f && Snap <= 0.35f);
	TestEqual(TEXT("...and stays up for the delay again"), CarryRunClock(Clock, Delay - 0.15f, false, true, Easing), 0.f);
	TestTrue(TEXT("...then goes low again"), CarryRunClock(Clock, 0.15f + LooterCarry::LowerSeconds + 0.05f, false, true, Easing) >= 0.99f);

	// A stop: a strafe turning round (shorter than the grace) changes nothing; standing still lets it settle up, unhurried.
	TestTrue(TEXT("A short stop keeps it low"), CarryRunClock(Clock, LooterCarry::StillGrace - 0.05f, false, false, Easing) >= 0.99f);
	float Settling = 0.f;
	const float Settled = CarryRunClock(Clock, 0.05f + LooterCarry::SettleSeconds + 0.05f, false, false, Settling);
	TestTrue(FString::Printf(TEXT("Standing still it settles back up (%.3f)"), Settled), Settled <= 0.01f);
	TestTrue(FString::Printf(TEXT("...unhurried (%.3f of the way in a frame at most)"), Settling), Settling <= 0.1f);

	// An aim while it settles up slowly still snaps it the rest of the way.
	CarryRunClock(Clock, 0.1f, false, true, Easing);
	CarryRunClock(Clock, Delay + LooterCarry::LowerSeconds + 0.1f, false, true, Easing);
	CarryRunClock(Clock, LooterCarry::StillGrace + 0.05f, false, false, Easing);
	const float MidSettle = Clock.GetAlpha();
	const float AimedUp = CarryRunClock(Clock, LooterCarry::RaiseSeconds + CarryFrame, true, false, Easing);
	TestTrue(FString::Printf(TEXT("An aim mid-settle (at %.2f) is up in the raise's time (%.3f left)"), MidSettle, AimedUp), MidSettle > 0.5f && AimedUp <= 0.01f);

	// Standing still from the start never lowers it; a fresh draw waits the whole delay.
	FLooterReadyClock Still;
	TestEqual(TEXT("Standing still it stays shouldered"), CarryRunClock(Still, 3.f, false, false, Easing), 0.f);
	Clock.Reset();
	TestEqual(TEXT("Reset: shouldered at once"), Clock.GetAlpha(), 0.f);
	TestEqual(TEXT("...and the delay starts over"), CarryRunClock(Clock, Delay - 0.05f, false, true, Easing), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerCarryGunTurnTest, "Looter.Player.Carry.GunTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerCarryGunTurnTest::RunTest(const FString& Parameters)
{
	// The gun as the view turns it: on the aim shouldered; 15-20 degrees under it at the low-ready (still following the aim);
	// across the chest in a sprint, muzzle down and to the left whatever the aim's pitch; a six-gun's at the ground.
	const float Yaw = 30.f;
	const FQuat Level = FRotator(0.f, Yaw, 0.f).Quaternion();
	TestTrue(TEXT("Shouldered: on the aim"), LooterCarry::CarriedGun(Level, Yaw, FLooterCarry()).Equals(Level, 1.e-4f));

	const FLooterCarry Low = CarryOfShares(1.f, 0.f, false);
	const FVector LowMuzzle = LooterCarry::CarriedGun(Level, Yaw, Low).GetForwardVector();
	const float Under = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(FVector::DotProduct(LowMuzzle, Level.GetForwardVector())), -1.f, 1.f)));
	TestTrue(FString::Printf(TEXT("Low-ready: the muzzle %.1f degrees off the aim, under it (%.2f)"), Under, LowMuzzle.Z), Under >= 15.f && Under <= 20.f && LowMuzzle.Z < -0.2);
	const FRotator LookingUp = LooterCarry::CarriedGun(FRotator(30.f, Yaw, 0.f).Quaternion(), Yaw, Low).Rotator();
	const float UpPitch = static_cast<float>(LookingUp.Pitch);
	TestTrue(FString::Printf(TEXT("...and it follows the aim up (%.1f degrees at a 30 degree aim)"), UpPitch), FMath::IsNearlyEqual(UpPitch, 12.f, 2.f));

	const FLooterCarry Sprint = CarryOfShares(0.f, 1.f, false);
	for (const float Pitch : { -30.f, 0.f, 40.f })
	{
		const FRotator Carried = LooterCarry::CarriedGun(FRotator(Pitch, Yaw, 0.f).Quaternion(), Yaw, Sprint).Rotator();
		const float CarriedPitch = static_cast<float>(Carried.Pitch);
		const float CarriedYaw = static_cast<float>(FRotator::NormalizeAxis(Carried.Yaw - Yaw));
		const float CarriedRoll = static_cast<float>(Carried.Roll);
		TestTrue(FString::Printf(TEXT("Sprint, aiming %.0f: across the chest at (%.1f, %.1f, %.1f) from the facing"), Pitch, CarriedPitch, CarriedYaw, CarriedRoll),
			FMath::IsNearlyEqual(CarriedPitch, -20.f, 0.5f) && FMath::IsNearlyEqual(CarriedYaw, -35.f, 0.5f) && FMath::IsNearlyEqual(CarriedRoll, -10.f, 0.5f));
	}
	const FRotator SixGun = LooterCarry::CarriedGun(Level, Yaw, CarryOfShares(0.f, 1.f, true)).Rotator();
	TestTrue(FString::Printf(TEXT("A six-gun in a sprint points at the ground (%.1f degrees)"), SixGun.Pitch), SixGun.Pitch <= -60.f);

	// Blends turn it evenly: no step of a twentieth of a share turns the muzzle more than a twentieth of the whole turn (and a bit).
	for (const FLooterCarry& Full : { Low, Sprint, CarryOfShares(0.f, 1.f, true) })
	{
		const float Whole = CarryDegrees(Level, LooterCarry::CarriedGun(Level, Yaw, Full));
		float Worst = 0.f;
		FQuat Last = Level;
		for (int32 Step = 1; Step <= 20; ++Step)
		{
			const float Share = Step / 20.f;
			const FQuat Now = LooterCarry::CarriedGun(Level, Yaw, CarryOfShares(Full.LowReady * Share, Full.Sprint * Share, Full.bShortGun));
			Worst = FMath::Max(Worst, CarryDegrees(Last, Now));
			Last = Now;
		}
		TestTrue(FString::Printf(TEXT("The turn to a %.0f degree carry is even (%.2f degrees a step at most)"), Whole, Worst), Worst <= Whole / 20.f * 1.1f + 0.01f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerCarryChestHoldTest, "Looter.Player.Carry.ChestHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerCarryChestHoldTest::RunTest(const FString& Parameters)
{
	// A long gun in both hands on the mannequin: at the low-ready the hands come lower; in a sprint the right hand goes low on
	// the chest; both hands turn with the gun, so it never turns in a palm; the left palm stays on the foregrip, a shotgun's
	// far-out pump too; the blend in moves smoothly.
	USkeleton* Skeleton = PlayerAnimTestKit::MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const PlayerAnimTestKit::FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();
	const FVector RestShoulder = Rig.Where(Rest, TEXT("upperarm_r"));
	// Shouldered: the right wrist up in front of the chest, the gun's grip in it.
	const FCompactPose Shouldered = CarryRightHandAt(Rig, Rest, RestShoulder + CarryShoulderedWrist);
	const FLooterStanceInput Bullpup = CarryGunIn(Rig, Shouldered, CarryBullpupForegrip, 0.f);
	const FCarryPosed Held = CarryPoseOf(Rig, Shouldered, Bullpup, FLooterCarry());
	TestTrue(TEXT("Shouldered: sound"), PlayerAnimTestKit::FPoseRig::IsSound(Held.Pose));

	// The low-ready: lower, the gun in both hands as it was.
	const FCarryPosed Low = CarryPoseOf(Rig, Shouldered, Bullpup, CarryOfShares(1.f, 0.f, false));
	TestTrue(TEXT("Low-ready: sound"), PlayerAnimTestKit::FPoseRig::IsSound(Low.Pose));
	// (The gun may be drawn in a little toward the left shoulder, which takes some of the drop back.)
	TestTrue(FString::Printf(TEXT("Low-ready: the right hand comes down (%.1f cm)"), Held.HandR.GetLocation().Z - Low.HandR.GetLocation().Z),
		Held.HandR.GetLocation().Z - Low.HandR.GetLocation().Z >= LooterCarry::LowDrop * 0.6f);
	TestTrue(FString::Printf(TEXT("Low-ready: the gun turns with the right hand (%.2f degrees in the palm)"), CarryDegrees(Held.InRightHand(), Low.InRightHand())),
		CarryDegrees(Held.InRightHand(), Low.InRightHand()) <= 1.f);
	TestTrue(FString::Printf(TEXT("Low-ready: the left palm on the foregrip (%.2f cm off)"), FVector::Dist(Low.Palm(), Low.OnGun(CarryBullpupForegrip))),
		FVector::Dist(Low.Palm(), Low.OnGun(CarryBullpupForegrip)) <= 1.0);

	// The sprint: low on the chest, in toward the middle, turned with the gun; the left palm on it, turned with it too.
	const FCarryPosed Sprint = CarryPoseOf(Rig, Shouldered, Bullpup, CarryOfShares(0.f, 1.f, false));
	TestTrue(TEXT("Sprint: sound"), PlayerAnimTestKit::FPoseRig::IsSound(Sprint.Pose));
	const FVector ChestSpot = Sprint.ShoulderR + CarryAhead * LooterCarry::ChestAhead - CarryRight * LooterCarry::ChestIn
		- FVector(0.0, 0.0, LooterCarry::ChestDown);
	TestTrue(FString::Printf(TEXT("Sprint: the right hand low on the chest (%.1f cm from the spot, the gun maybe drawn in for the left hand; %.1f under the shoulder)"),
		FVector::Dist(Sprint.HandR.GetLocation(), ChestSpot), Sprint.ShoulderR.Z - Sprint.HandR.GetLocation().Z),
		FVector::Dist(Sprint.HandR.GetLocation(), ChestSpot) <= 10.0 && Sprint.ShoulderR.Z - Sprint.HandR.GetLocation().Z >= LooterCarry::ChestDown - 8.f);
	TestTrue(FString::Printf(TEXT("Sprint: the gun turns with the right hand (%.2f degrees in the palm)"), CarryDegrees(Held.InRightHand(), Sprint.InRightHand())),
		CarryDegrees(Held.InRightHand(), Sprint.InRightHand()) <= 1.f);
	TestTrue(FString::Printf(TEXT("Sprint: the left palm on the foregrip (%.2f cm off)"), FVector::Dist(Sprint.Palm(), Sprint.OnGun(CarryBullpupForegrip))),
		FVector::Dist(Sprint.Palm(), Sprint.OnGun(CarryBullpupForegrip)) <= 1.0);
	TestTrue(FString::Printf(TEXT("Sprint: ...turned with the gun (%.2f degrees off how it held it shouldered)"), CarryDegrees(Held.InLeftHand(), Sprint.InLeftHand())),
		CarryDegrees(Held.InLeftHand(), Sprint.InLeftHand()) <= 1.f);
	const FVector Muzzle = Sprint.Gun.GetForwardVector();
	TestTrue(FString::Printf(TEXT("Sprint: the muzzle down and to the left (%s)"), *Muzzle.ToCompactString()),
		Muzzle.Z < -0.2 && FVector::DotProduct(Muzzle, CarryRight) < -0.4);

	// A shotgun's pump, twice as far out: the gun is drawn in until the left hand has it.
	const FLooterStanceInput Shotgun = CarryGunIn(Rig, Shouldered, CarryShotgunForegrip, 0.f);
	const FCarryPosed Pumped = CarryPoseOf(Rig, Shouldered, Shotgun, CarryOfShares(0.f, 1.f, false));
	TestTrue(FString::Printf(TEXT("Shotgun sprint: the left palm on the pump (%.2f cm off)"), FVector::Dist(Pumped.Palm(), Pumped.OnGun(CarryShotgunForegrip))),
		FVector::Dist(Pumped.Palm(), Pumped.OnGun(CarryShotgunForegrip)) <= 1.0);
	TestTrue(FString::Printf(TEXT("Shotgun sprint: the gun turns with the right hand (%.2f degrees)"), CarryDegrees(Held.InRightHand(), Pumped.InRightHand())),
		CarryDegrees(Held.InRightHand(), Pumped.InRightHand()) <= 1.f);

	// The sprint coming in a twentieth at a time (the lean with it): every pose sound, the hand never jumping, the gun only
	// ever off its place in the palm by the lean that fades as the carry comes in.
	FVector LastWrist = Held.HandR.GetLocation();
	float WorstStep = 0.f;
	float WorstTurn = 0.f;
	float WorstPalm = 0.f;
	bool bSound = true;
	for (int32 Step = 1; Step <= 20; ++Step)
	{
		const FCarryPosed Frame = CarryPoseOf(Rig, Shouldered, Bullpup, CarryOfShares(0.f, Step / 20.f, false));
		bSound &= PlayerAnimTestKit::FPoseRig::IsSound(Frame.Pose);
		WorstStep = FMath::Max(WorstStep, static_cast<float>(FVector::Dist(Frame.HandR.GetLocation(), LastWrist)));
		WorstTurn = FMath::Max(WorstTurn, CarryDegrees(Held.InRightHand(), Frame.InRightHand()));
		WorstPalm = FMath::Max(WorstPalm, static_cast<float>(FVector::Dist(Frame.Palm(), Frame.OnGun(CarryBullpupForegrip))));
		LastWrist = Frame.HandR.GetLocation();
	}
	TestTrue(TEXT("Into the sprint: every pose sound"), bSound);
	TestTrue(FString::Printf(TEXT("Into the sprint: the hand moves smoothly (%.1f cm a twentieth at most)"), WorstStep), WorstStep <= 5.f);
	TestTrue(FString::Printf(TEXT("Into the sprint: the gun stays put in the palm (%.2f degrees at most, the lean's)"), WorstTurn),
		WorstTurn <= Bullpup.SprintTorsoLean * 0.3f);
	TestTrue(FString::Printf(TEXT("Into the sprint: the left palm stays on (%.2f cm off at most)"), WorstPalm), WorstPalm <= 1.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerCarrySixGunTest, "Looter.Player.Carry.SixGun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerCarrySixGunTest::RunTest(const FString& Parameters)
{
	// The Drover, held out at arm's length in both hands: in a sprint it hangs low at the side in the right hand, muzzle to
	// the ground, turned with the hand; the left hand lets go (its arm swings with the run). At the low-ready both hands stay.
	USkeleton* Skeleton = PlayerAnimTestKit::MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const PlayerAnimTestKit::FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();
	const FCompactPose Shouldered = CarryRightHandAt(Rig, Rest, Rig.Where(Rest, TEXT("upperarm_r")) + CarryShoulderedWrist);
	const FLooterStanceInput SixGun = CarryGunIn(Rig, Shouldered, CarrySixGunForegrip, 14.f);
	// The support palm's spot on a six-gun: out past the shooting hand's fingers on the grip's left, a little under them.
	const FVector Support = CarrySixGunForegrip + FVector(0.0, -2.0, -1.0);
	const FCarryPosed Held = CarryPoseOf(Rig, Shouldered, SixGun, FLooterCarry());

	const FCarryPosed Sprint = CarryPoseOf(Rig, Shouldered, SixGun, CarryOfShares(0.f, 1.f, true));
	TestTrue(TEXT("Sprint: sound"), PlayerAnimTestKit::FPoseRig::IsSound(Sprint.Pose));
	const FVector SideSpot = Sprint.ShoulderR + CarryAhead * LooterCarry::SideAhead + CarryRight * LooterCarry::SideOut
		- FVector(0.0, 0.0, LooterCarry::SideDown);
	TestTrue(FString::Printf(TEXT("Sprint: the right hand low at the side (%.1f cm from the spot, %.1f under the shoulder)"),
		FVector::Dist(Sprint.HandR.GetLocation(), SideSpot), Sprint.ShoulderR.Z - Sprint.HandR.GetLocation().Z),
		FVector::Dist(Sprint.HandR.GetLocation(), SideSpot) <= 1.5 && Sprint.ShoulderR.Z - Sprint.HandR.GetLocation().Z >= 40.0);
	TestTrue(FString::Printf(TEXT("Sprint: the gun turns with the hand (%.2f degrees in the palm)"), CarryDegrees(Held.InRightHand(), Sprint.InRightHand())),
		CarryDegrees(Held.InRightHand(), Sprint.InRightHand()) <= 1.f);
	TestTrue(FString::Printf(TEXT("Sprint: the muzzle at the ground (%.2f down)"), -Sprint.Gun.GetForwardVector().Z), Sprint.Gun.GetForwardVector().Z <= -0.85);
	TestTrue(FString::Printf(TEXT("Sprint: one-handed, the left palm off the gun (%.1f cm from it)"), FVector::Dist(Sprint.Palm(), Sprint.OnGun(Support))),
		FVector::Dist(Sprint.Palm(), Sprint.OnGun(Support)) >= 15.0);

	// Half way into the sprint the left hand is half way off: it lets go smoothly.
	const FCarryPosed Half = CarryPoseOf(Rig, Shouldered, SixGun, CarryOfShares(0.f, 0.5f, true));
	const double HalfOff = FVector::Dist(Half.Palm(), Half.OnGun(Support));
	TestTrue(FString::Printf(TEXT("Half way: the left hand letting go (%.1f cm off, %.1f in full)"), HalfOff, FVector::Dist(Sprint.Palm(), Sprint.OnGun(Support))),
		HalfOff > 1.0 && HalfOff < FVector::Dist(Sprint.Palm(), Sprint.OnGun(Support)));

	const FCarryPosed Low = CarryPoseOf(Rig, Shouldered, SixGun, CarryOfShares(1.f, 0.f, true));
	TestTrue(TEXT("Low-ready: sound"), PlayerAnimTestKit::FPoseRig::IsSound(Low.Pose));
	TestTrue(FString::Printf(TEXT("Low-ready: both hands stay on (%.2f cm off the support spot)"), FVector::Dist(Low.Palm(), Low.OnGun(Support))),
		FVector::Dist(Low.Palm(), Low.OnGun(Support)) <= 1.5);
	TestTrue(FString::Printf(TEXT("Low-ready: the gun turns with the hand (%.2f degrees)"), CarryDegrees(Held.InRightHand(), Low.InRightHand())),
		CarryDegrees(Held.InRightHand(), Low.InRightHand()) <= 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerCarryBobTest, "Looter.Player.Carry.Bob",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerCarryBobTest::RunTest(const FString& Parameters)
{
	// The armed upper body over the run's hips: most of the quick bounce smoothed out (a share kept, so it looks alive), slow
	// changes passed through, never trailing the hips far, the same at any frame rate.
	// The share of a 6 cm (peak to peak) bounce at Hertz left, over its last whole cycle once the filter has settled.
	const auto Bounce = [](float Hertz, float FramesPerSecond, float Seconds)
	{
		FLooterBobFilter Filter;
		const float Step = 1.f / FramesPerSecond;
		const float Cycle = 1.f / Hertz + 0.05f;
		double Low = UE_BIG_NUMBER;
		double High = -UE_BIG_NUMBER;
		for (float Time = 0.f; Time < Seconds; Time += Step)
		{
			const FVector Raw(0.0, 0.0, 100.0 + 3.0 * FMath::Sin(2.f * UE_PI * Hertz * Time));
			const double Z = Filter.Update(Raw, Step).Z;
			if (Time > Seconds - Cycle)
			{
				Low = FMath::Min(Low, Z);
				High = FMath::Max(High, Z);
			}
		}
		return static_cast<float>((High - Low) / 6.0);
	};

	FLooterBobFilter Steady;
	FVector Out = FVector::ZeroVector;
	for (int32 Frame = 0; Frame < 120; ++Frame)
	{
		Out = Steady.Update(FVector(5.0, 2.0, 96.0), CarryFrame);
	}
	TestTrue(TEXT("Standing still: right where the hips put it"), Out.Equals(FVector(5.0, 2.0, 96.0), 0.01));

	const float Run = Bounce(2.6f, 60.f, 4.f);
	TestTrue(FString::Printf(TEXT("The jog's bounce: %.0f%% of it kept"), Run * 100.f), Run >= 0.3f && Run <= 0.65f);
	const float Slow = Bounce(0.3f, 60.f, 8.f);
	TestTrue(FString::Printf(TEXT("A slow change passes through (%.0f%%)"), Slow * 100.f), Slow >= 0.85f);
	const float Fast = Bounce(2.6f, 144.f, 4.f);
	const float Choppy = Bounce(2.6f, 30.f, 4.f);
	TestTrue(FString::Printf(TEXT("The same at 30 and 144 frames a second (%.0f%%, %.0f%%)"), Choppy * 100.f, Fast * 100.f), FMath::Abs(Fast - Choppy) <= 0.06f);

	// A landing's sudden dip: the body never trails the hips by more than the limit's share.
	FLooterBobFilter Landing;
	float WorstLag = 0.f;
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		const FVector Raw(0.0, 0.0, Frame < 10 ? 100.0 : 80.0);
		WorstLag = FMath::Max(WorstLag, static_cast<float>(FVector::Dist(Landing.Update(Raw, CarryFrame), Raw)));
	}
	TestTrue(FString::Printf(TEXT("A sudden dip: trailing %.2f cm at most"), WorstLag), WorstLag <= FLooterBobFilter::MaxLag * FLooterBobFilter::SmoothShare + 0.01f);
	return true;
}

#endif
