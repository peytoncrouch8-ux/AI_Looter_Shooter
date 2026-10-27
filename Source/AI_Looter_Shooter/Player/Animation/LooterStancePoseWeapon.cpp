#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "Player/PlayerMeleeMotion.h"
#include "Player/PlayerMeleeRules.h"
#include "Player/PlayerThrowMotion.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "HAL/IConsoleManager.h"
#include "TwoBoneIK.h"

using namespace LooterStancePoseDetail;

// The stance pose's gun work: a short gun held out at arm's length, each shot's recoil in the body, a melee strike's jab,
// and the left hand on the foregrip (on a six-gun's grip: the support hand wrapped round the shooting hand; how it grips is
// LooterStancePoseGrip.cpp's); and SolveArm, the arm IK the throw and the revolver's reload share.

namespace
{
	/** The jab cocks back to this share of its reach before it drives out, and holds at the blow this long (seconds). */
	constexpr float JabCock = -0.3f;
	constexpr float JabHold = 0.05f;
	/** At full jab: the chest turns this far into the blow and leans this far over it (degrees), the hand drives this far (cm). */
	constexpr float JabTwist = 22.f;
	constexpr float JabLean = 10.f;
	constexpr float JabReach = 28.f;

	TAutoConsoleVariable<float> CVarJabWristBack(TEXT("Looter.Melee.ThirdPerson.LayBack"), 40.f,
		TEXT("Third person: degrees the wrist lays the melee tool's head back at the swing's coil."));
	TAutoConsoleVariable<float> CVarJabWristOver(TEXT("Looter.Melee.ThirdPerson.SnapOver"), 25.f,
		TEXT("Third person: degrees the wrist snaps the melee tool's head over onto the blow."));

	/**
	 * A six-gun's support palm, past the spot its foregrip marks (cm): out to the grip's side and a little under. The
	 * foregrip (Drover.py's SUPPORT, 3.2 cm out on the grip's left) is where the support hand presses on the grip, but the
	 * shooting hand's fingers already wrap round there, so a palm put right on it sank into them.
	 */
	constexpr float SupportClear = 2.f;
	constexpr float SupportDrop = 1.f;
}

void LooterStancePoseDetail::SolveArm(FCSPose<FCompactPose>& Pose, FCompactPoseBoneIndex UpperArm, FCompactPoseBoneIndex LowerArm,
	FCompactPoseBoneIndex Hand, const FVector& WristTarget, const FVector& ElbowHint, float Weight, bool bTurnHand)
{
	if (Weight <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	FTransform Shoulder = Pose.GetComponentSpaceTransform(UpperArm);
	FTransform Elbow = Pose.GetComponentSpaceTransform(LowerArm);
	FTransform Wrist = Pose.GetComponentSpaceTransform(Hand);
	const FQuat HandRotation = Wrist.GetRotation();
	const FVector Forearm = (Wrist.GetLocation() - Elbow.GetLocation()).GetSafeNormal();

	AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, ElbowHint, WristTarget,
		/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
	const FVector Swung = (Wrist.GetLocation() - Elbow.GetLocation()).GetSafeNormal();
	Wrist.SetRotation(bTurnHand && !Forearm.IsNearlyZero() && !Swung.IsNearlyZero()
		? (FQuat::FindBetweenNormals(Forearm, Swung) * HandRotation).GetNormalized() : HandRotation);

	const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), FMath::Min(Weight, 1.f));
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
	// The hero's tool rides this hand (round 4, 2026-10-27): the wrist lays its head back with the coil and snaps it over
	// onto the blow (MeleeWristDegrees), so it swings rather than being pushed.
	Wrist.SetRotation(FQuat(Right, FMath::DegreesToRadians(LooterStancePose::MeleeWristDegrees(Jab))) * HandRotation);

	const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
}

float LooterStancePose::MeleeWristDegrees(float Jab)
{
	// Laid back (the head up and behind) by the full LayBack at the coil's JabCock, over by SnapOver at the blow's 1,
	// through nothing at 0: so no pop as the swing starts or ends. About the body's right: positive tips the head forward
	// and down onto the blow.
	const float Back = CVarJabWristBack.GetValueOnAnyThread();
	const float Over = CVarJabWristOver.GetValueOnAnyThread();
	return Jab < 0.f ? -Back * FMath::Min(Jab / JabCock, 1.f) : Over * FMath::Min(Jab, 1.f);
}

void LooterStancePoseDetail::ApplyHoldReach(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right)
{
	// The rifle poses tuck the hand in at the shoulder, where a six-gun would sit under the chin: out along the barrel it
	// goes instead, a little in toward the body's middle and up, the way a pistol is held out in both hands. A sprint
	// carries the gun low, a reload brings it in and a grenade throw dips it out of the way, so the reach eases off through
	// each: at arm's length the arm is straight, and a hand that must also drop has no slack left to do it (a throw's dip
	// came out at under half its depth).
	const float Reach = Stance.HoldReach * (1.f - Stance.SprintAlpha) * (1.f - Stance.ReloadWeight) * (1.f - Stance.Climb.Hands)
		* (1.f - ThrowMotion::GunDipShare(Stance.ThrowTime));
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_r"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_r"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_r"));
	if (Reach <= 0.01f || !UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
	{
		return;
	}
	FTransform Shoulder = Pose.GetComponentSpaceTransform(UpperArm);
	FTransform Elbow = Pose.GetComponentSpaceTransform(LowerArm);
	FTransform Wrist = Pose.GetComponentSpaceTransform(Hand);
	const FQuat HandRotation = Wrist.GetRotation();
	const FVector Barrel = Stance.WeaponRotation.GetForwardVector();
	const FVector WristTarget = Wrist.GetLocation() + Barrel * Reach - Right * (Reach * 0.25f) + FVector::UpVector * (Reach * 0.12f);
	const FVector ElbowHint = Elbow.GetLocation() - FVector::UpVector * 15.f + Right * 10.f;

	AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, ElbowHint, WristTarget,
		/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
	Wrist.SetRotation(HandRotation);

	const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
}

FVector LooterStancePoseDetail::HoldOffsetVector(const FLooterStanceInput& Stance, const FVector& Forward, const FVector& Right)
{
	// The torso pitches with the aim (LooterStancePose::Apply), and a hero's shoulders sit off the mannequin's along the
	// chest, so the offset pitches with it: X along the facing, Y to the right, Z up, tipped up by the torso's aim pitch.
	const FVector Level = Forward * Stance.HoldOffset.X + Right * Stance.HoldOffset.Y + FVector::UpVector * Stance.HoldOffset.Z;
	return FQuat(Right, FMath::DegreesToRadians(-Stance.AimPitch)).RotateVector(Level);
}

void LooterStancePoseDetail::ApplyHoldOffset(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FVector& Forward, const FVector& Right)
{
	// Both hands, so a reload's or a one-handed carry's left hand keeps its place on the gun too; the foregrip's solve
	// below takes the left hand from here. The mannequin is hidden under the hero: only where its hands end up shows.
	const FVector Shift = HoldOffsetVector(Stance, Forward, Right);
	for (const float Side : { 1.f, -1.f })
	{
		const TCHAR* const Suffix = Side > 0.f ? TEXT("_r") : TEXT("_l");
		const FCompactPoseBoneIndex UpperArm = FindBone(Bones, *(FString(TEXT("upperarm")) + Suffix));
		const FCompactPoseBoneIndex LowerArm = FindBone(Bones, *(FString(TEXT("lowerarm")) + Suffix));
		const FCompactPoseBoneIndex Hand = FindBone(Bones, *(FString(TEXT("hand")) + Suffix));
		if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
		{
			continue;
		}
		// The elbow keeps bending down and out, the way it already does.
		const FVector Elbow = Pose.GetComponentSpaceTransform(LowerArm).GetLocation();
		const FVector Wrist = Pose.GetComponentSpaceTransform(Hand).GetLocation();
		SolveArm(Pose, UpperArm, LowerArm, Hand, Wrist + Shift, Elbow + Shift - FVector::UpVector * 15.f + Right * (Side * 10.f), 1.f,
			/*bTurnHand*/ false);
	}
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

void LooterStancePoseDetail::ApplyLeftHandOnForegrip(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right,
	const FQuat& PalmTurn)
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
	FVector OnGun = Stance.WeaponForegrip - Stance.WeaponGrip;
	if (Stance.HoldReach > 0.f)
	{
		// A six-gun: the support palm out past the shooting hand's fingers, on the foregrip's side of the grip (its left
		// when the foregrip sits square on it), and a little under them.
		const float Side = OnGun.Y > 0.1 ? 1.f : -1.f;
		OnGun += FVector(0.f, Side * SupportClear, -SupportDrop);
	}
	const FVector GripLocation = (Stance.HoldSocketLocal * Pose.GetComponentSpaceTransform(HoldBone)).GetLocation();
	const FVector Foregrip = GripLocation + Stance.WeaponRotation.RotateVector(OnGun + LeftGripShift(Stance));

	FTransform Shoulder = Pose.GetComponentSpaceTransform(UpperArm);
	FTransform Elbow = Pose.GetComponentSpaceTransform(LowerArm);
	FTransform Wrist = Pose.GetComponentSpaceTransform(Hand);

	// Aim the palm (the mannequin's HandGrip_L point) at the foregrip, the hand turned to grip the gun (LeftHandOnGun: set on
	// the gun by the hold, so it turns with the gun's kick and carry), not left as the rifle animation's open palm up under it.
	const FQuat HandRotation = LeftHandOnGun(Bones, Stance, Stance.WeaponRotation, (PalmTurn * Wrist.GetRotation()).GetNormalized());
	const FVector WristTarget = Foregrip - HandRotation.RotateVector(LeftPalmFromWrist());
	// Keep the elbow bending down and out, the way it already is. Carried, it goes where the forearm turned with the palm
	// would put it, so the wrist doesn't bend away from the gun. Aimed down the sights it tucks down and in under the gun
	// (LooterStancePoseAim.cpp).
	const float Tuck = FMath::Clamp(Stance.Aim.ElbowIn, 0.f, 1.f) * AimShare(Stance);
	const FVector UsualHint = Elbow.GetLocation() - FVector::UpVector * FMath::Lerp(20.f, 35.f, Tuck) + Right * -FMath::Lerp(10.f, 2.f, Tuck);
	const FVector TurnedHint = WristTarget - PalmTurn.RotateVector(Wrist.GetLocation() - Elbow.GetLocation());
	const FVector ElbowHint = FMath::Lerp(UsualHint, TurnedHint, Stance.Carry.Weight());

	AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, ElbowHint, WristTarget,
		/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
	Wrist.SetRotation(HandRotation);

	const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), ForegripHold(Stance));
}

float LooterStancePoseDetail::ForegripHold(const FLooterStanceInput& Stance)
{
	// During a reload the left hand is busy (the magazine, or a revolver's cylinder), so it lets go of the foregrip; a
	// six-gun carried in one hand in a sprint lets go too, the arm swinging with the run (ULooterCharacterAnimInstance).
	return (1.f - Stance.ReloadWeight) * (1.f - Stance.Carry.OneHanded());
}
