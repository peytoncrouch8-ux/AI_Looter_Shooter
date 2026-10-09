#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "Player/PlayerMeleeMotion.h"
#include "Player/PlayerMeleeRules.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "TwoBoneIK.h"

using namespace LooterStancePoseDetail;

// The stance pose's gun work: each shot's recoil in the body, a melee strike's jab, and the left hand on the foregrip.

namespace
{
	/** The jab cocks back to this share of its reach before it drives out, and holds at the blow this long (seconds). */
	constexpr float JabCock = -0.3f;
	constexpr float JabHold = 0.05f;
	/** At full jab: the chest turns this far into the blow and leans this far over it (degrees), the hand drives this far (cm). */
	constexpr float JabTwist = 22.f;
	constexpr float JabLean = 10.f;
	constexpr float JabReach = 28.f;
}

float LooterStancePose::MeleeJab(float SwingTime)
{
	const float Cock = MeleeMotion::CockSeconds;
	const float Contact = FMeleeRules::ContactSeconds;
	const float End = FMeleeRules::SwingSeconds;
	if (SwingTime <= 0.f || SwingTime >= End)
	{
		return 0.f;
	}
	if (SwingTime < Cock)
	{
		return JabCock * FMath::InterpEaseInOut(0.f, 1.f, SwingTime / Cock, 2.f);
	}
	if (SwingTime < Contact)
	{
		// Speeding up into the blow, as the first-person stock does.
		return FMath::Lerp(JabCock, 1.f, FMath::InterpEaseIn(0.f, 1.f, (SwingTime - Cock) / (Contact - Cock), 2.f));
	}
	if (SwingTime < Contact + JabHold)
	{
		return 1.f;
	}
	return 1.f - FMath::InterpEaseInOut(0.f, 1.f, (SwingTime - Contact - JabHold) / (End - Contact - JabHold), 2.f);
}

void LooterStancePoseDetail::ApplyMeleeJab(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FVector& Forward, const FVector& Right)
{
	// The chest turns to its left into the blow (the stock, in the right hand, leads across) and leans over it, mostly high up.
	const float Jab = Stance.MeleeJab;
	const FCompactPoseBoneIndex Chest[] = { FindBone(Bones, TEXT("spine_03")), FindBone(Bones, TEXT("spine_05")) };
	const float ChestShare[] = { 0.45f, 0.55f };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Chest); ++Index)
	{
		RotateBone(Pose, Chest[Index], FQuat(FVector::UpVector, FMath::DegreesToRadians(-JabTwist * Jab * ChestShare[Index]))
			* FQuat(Right, FMath::DegreesToRadians(JabLean * Jab * ChestShare[Index])));
	}

	// The right hand drives forward and a little in and up (the gun's stock with it, or the fist); the elbow stays down and out.
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_r"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_r"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_r"));
	if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
	{
		return;
	}
	FTransform Shoulder = Pose.GetComponentSpaceTransform(UpperArm);
	FTransform Elbow = Pose.GetComponentSpaceTransform(LowerArm);
	FTransform Wrist = Pose.GetComponentSpaceTransform(Hand);
	const FQuat HandRotation = Wrist.GetRotation();
	const FVector WristTarget = Wrist.GetLocation() + (Forward - Right * 0.25f + FVector::UpVector * 0.15f) * (JabReach * Jab);
	const FVector ElbowHint = Elbow.GetLocation() - FVector::UpVector * 15.f + Right * 10.f;

	AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, ElbowHint, WristTarget,
		/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
	Wrist.SetRotation(HandRotation);

	const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
}

void LooterStancePoseDetail::ApplyRecoil(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right)
{
	// The chest rocks back with the kick, mostly high up. Seen from behind the character a gun's few cm of kick would be
	// lost, so the body sells it bigger than the first-person view does.
	const FCompactPoseBoneIndex Chest[] = { FindBone(Bones, TEXT("spine_03")), FindBone(Bones, TEXT("spine_05")) };
	const float ChestShare[] = { 0.4f, 0.6f };
	const float ChestPitch = FMath::Min(Stance.RecoilPitch * 0.8f + Stance.RecoilBack * 1.5f, 14.f);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Chest); ++Index)
	{
		RotateBone(Pose, Chest[Index], FQuat(Right, FMath::DegreesToRadians(-ChestPitch * ChestShare[Index])));
	}

	// The right arm gives: the hand is pushed back along the barrel, the elbow bending down and out.
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_r"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_r"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_r"));
	if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid() || Stance.RecoilBack <= 0.f)
	{
		return;
	}
	FTransform Shoulder = Pose.GetComponentSpaceTransform(UpperArm);
	FTransform Elbow = Pose.GetComponentSpaceTransform(LowerArm);
	FTransform Wrist = Pose.GetComponentSpaceTransform(Hand);
	const FQuat HandRotation = Wrist.GetRotation();
	const FVector Barrel = Stance.WeaponRotation.GetForwardVector();
	const FVector WristTarget = Wrist.GetLocation() - Barrel * FMath::Min(Stance.RecoilBack * 2.5f, 14.f);
	const FVector ElbowHint = Elbow.GetLocation() - FVector::UpVector * 15.f + Right * 10.f;

	AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, ElbowHint, WristTarget,
		/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
	Wrist.SetRotation(HandRotation);

	const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
}

void LooterStancePoseDetail::ApplyLeftHandOnForegrip(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right)
{
	const FCompactPoseBoneIndex HoldBone = FindBone(Bones, *Stance.HoldBone.ToString());
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_l"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_l"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_l"));
	if (!HoldBone.IsValid() || !UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
	{
		return;
	}

	// The gun's grip sits in the right-hand socket and the gun points along the aim, so its foregrip is here:
	const FVector GripLocation = (Stance.HoldSocketLocal * Pose.GetComponentSpaceTransform(HoldBone)).GetLocation();
	const FVector Foregrip = GripLocation + Stance.WeaponRotation.RotateVector(Stance.WeaponForegrip - Stance.WeaponGrip);

	FTransform Shoulder = Pose.GetComponentSpaceTransform(UpperArm);
	FTransform Elbow = Pose.GetComponentSpaceTransform(LowerArm);
	FTransform Wrist = Pose.GetComponentSpaceTransform(Hand);

	// Aim the palm (the mannequin's HandGrip_L point) at the foregrip, keeping the animation's hand orientation.
	static const FVector PalmFromWrist(7.5f, -2.5f, 0.f);
	const FQuat HandRotation = Wrist.GetRotation();
	const FVector WristTarget = Foregrip - HandRotation.RotateVector(PalmFromWrist);
	// Keep the elbow bending down and out, the way it already is.
	const FVector ElbowHint = Elbow.GetLocation() - FVector::UpVector * 20.f + Right * -10.f;

	AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, ElbowHint, WristTarget,
		/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
	Wrist.SetRotation(HandRotation);

	// During a reload the left hand is busy with the magazine, so it lets go of the foregrip.
	const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f - Stance.ReloadWeight);
}
