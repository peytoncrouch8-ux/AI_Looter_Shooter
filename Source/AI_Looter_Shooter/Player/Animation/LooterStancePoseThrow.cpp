#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "Player/PlayerThrowMotion.h"
#include "Player/PlayerThrowRules.h"
#include "BoneContainer.h"
#include "BonePose.h"

using namespace LooterStancePoseDetail;

// The stance pose's grenade throw: a code pose, like the melee jab, since the mannequin set has no throw. It shows from
// behind (third person) and in the body's shadow in first person, on the same clock as the first-person jar and gun
// (ThrowMotion): an overarm toss with the left hand, cocked up by the ear as the first-person jar comes up to the cheek,
// slung out to where the jar leaves the hand (FThrowRules::ReleasePoint, from the eye), following through across the
// body. The chest winds up (the left shoulder back) and turns into the toss; the gun in the right hand dips down and out
// of its way. Offsets are the full-size mannequin's, from the left shoulder: X ahead, Y out to the body's left, Z up.

namespace
{
	/** One moment of the throw: the left wrist and where its elbow points, and the chest's turn and lean (degrees). */
	struct FThrowKey
	{
		FVector Wrist;
		FVector Elbow;
		float Twist;
		float Lean;
	};

	/** Cocked: the hand up by the ear and a little behind it, the elbow out at the shoulder's height, the shoulder drawn back. */
	FThrowKey DrawnKey()
	{
		return { FVector(-6.f, 5.f, 20.f), FVector(-2.f, 24.f, 4.f), -16.f, -4.f };
	}

	/**
	 * Let go: the arm out ahead at the eyes' height, the palm (8 cm past the wrist) where the jar starts its flight
	 * (FThrowRules::ReleasePoint: 30 cm ahead of the eye, 14 left and 12 under it, at the player's size), the chest turned
	 * into it and leaning over.
	 */
	FThrowKey ReleaseKey()
	{
		return { FVector(38.f, -2.f, 7.f), FVector(12.f, 16.f, -10.f), 10.f, 6.f };
	}

	/** Followed through: the hand carried on down and across the body. */
	FThrowKey FollowKey()
	{
		return { FVector(32.f, -14.f, -16.f), FVector(8.f, 12.f, -20.f), 14.f, 8.f };
	}

	/** The arm has followed through by now (seconds), and is back to what it was doing by the throw's end. */
	constexpr float FollowSeconds = 0.28f;

	/** The gun hand's deepest dip (cm, the body's frame): down, out to the right and a little back, out of the toss's way. */
	constexpr float DipDown = 12.f;
	constexpr float DipOut = 6.f;
	constexpr float DipBack = 3.f;

	FThrowKey Blend(const FThrowKey& From, const FThrowKey& To, float Alpha)
	{
		return { FMath::Lerp(From.Wrist, To.Wrist, Alpha), FMath::Lerp(From.Elbow, To.Elbow, Alpha),
			FMath::Lerp(From.Twist, To.Twist, Alpha), FMath::Lerp(From.Lean, To.Lean, Alpha) };
	}
}

FLooterThrowStance LooterStancePose::DescribeThrow(float ThrowTime)
{
	FLooterThrowStance Throw;
	const float Draw = ThrowMotion::DrawSeconds;
	const float Release = FThrowRules::ReleaseSeconds;
	const float End = FThrowRules::ThrowSeconds;
	if (ThrowTime <= 0.f || ThrowTime >= End)
	{
		return Throw;
	}
	Throw.GunDip = ThrowMotion::GunDipShare(ThrowTime);

	FThrowKey Key = DrawnKey();
	if (ThrowTime < Draw)
	{
		// Up to the ear off whatever the arm was doing (the foregrip, or the walk's swing), slowing as it gets there.
		Throw.Arm = FMath::InterpEaseOut(0.f, 1.f, ThrowTime / Draw, 2.f);
	}
	else if (ThrowTime < Release)
	{
		// Slung: speeding up out of the draw, fastest as the jar leaves the hand, as the first-person jar is.
		const float Sling = (ThrowTime - Draw) / (Release - Draw);
		Throw.Arm = 1.f;
		Key = Blend(DrawnKey(), ReleaseKey(), Sling * Sling);
	}
	else if (ThrowTime < FollowSeconds)
	{
		Throw.Arm = 1.f;
		Key = Blend(ReleaseKey(), FollowKey(), FMath::InterpEaseOut(0.f, 1.f, (ThrowTime - Release) / (FollowSeconds - Release), 2.f));
	}
	else
	{
		// Back to the foregrip (or the walk) by the end, the gun coming up to meet it.
		Throw.Arm = 1.f - FMath::SmoothStep(0.f, 1.f, (ThrowTime - FollowSeconds) / (End - FollowSeconds));
		Key = FollowKey();
	}
	Throw.Hand = Key.Wrist;
	Throw.Elbow = Key.Elbow;
	// The chest comes into the wind-up with the arm and goes back with it.
	Throw.Twist = Key.Twist * Throw.Arm;
	Throw.Lean = Key.Lean * Throw.Arm;
	return Throw;
}

void LooterStancePoseDetail::ApplyThrowBody(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FLooterThrowStance& Throw, const FVector& Forward, const FVector& Right)
{
	// The chest turns, mostly high up: back for the wind-up, then into the toss (a positive turn about up brings the left
	// shoulder forward), leaning back a touch and then over it.
	const FCompactPoseBoneIndex Chest[] = { FindBone(Bones, TEXT("spine_03")), FindBone(Bones, TEXT("spine_05")) };
	const float ChestShare[] = { 0.45f, 0.55f };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Chest); ++Index)
	{
		RotateBone(Pose, Chest[Index], FQuat(FVector::UpVector, FMath::DegreesToRadians(Throw.Twist * ChestShare[Index]))
			* FQuat(Right, FMath::DegreesToRadians(Throw.Lean * ChestShare[Index])));
	}

	// The gun hand drops down and out to the right, the gun in it still pointing where the player aims (the view keeps it
	// on the aim in third person), so the left arm's toss has the middle to itself. A bare right hand stays as it is.
	if (!Stance.bHandOnForegrip || Throw.GunDip <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_r"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_r"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_r"));
	if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
	{
		return;
	}
	const FVector Wrist = Pose.GetComponentSpaceTransform(Hand).GetLocation();
	const FVector Elbow = Pose.GetComponentSpaceTransform(LowerArm).GetLocation();
	const FVector WristTarget = Wrist + (-FVector::UpVector * DipDown + Right * DipOut - Forward * DipBack) * Throw.GunDip;
	const FVector ElbowHint = Elbow - FVector::UpVector * 15.f + Right * 10.f;
	SolveArm(Pose, UpperArm, LowerArm, Hand, WristTarget, ElbowHint, 1.f, /*bTurnHand*/ false);
}

void LooterStancePoseDetail::ApplyThrowArm(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FLooterThrowStance& Throw, const FVector& Forward, const FVector& Right)
{
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_l"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_l"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_l"));
	if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
	{
		return;
	}
	// In the body's facing, pitched with the aim as the chest already is (armed), so a throw looking up goes up.
	const float Pitch = Stance.bAimWithTorso ? Stance.AimPitch : 0.f;
	const FQuat Aim(Right, FMath::DegreesToRadians(-Pitch));
	const FVector Out = -Right;
	const auto InBody = [&](const FVector& Offset)
	{
		return Aim.RotateVector(Forward * Offset.X + Out * Offset.Y + FVector::UpVector * Offset.Z);
	};
	const FVector Shoulder = Pose.GetComponentSpaceTransform(UpperArm).GetLocation();
	// The hand turns with the forearm as it swings up by the ear and over: kept as the pose had it, the wrist would bend
	// back on itself.
	SolveArm(Pose, UpperArm, LowerArm, Hand, Shoulder + InBody(Throw.Hand), Shoulder + InBody(Throw.Elbow), Throw.Arm, /*bTurnHand*/ true);
}
