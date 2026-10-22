#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "Player/PlayerThrowMotion.h"
#include "BoneContainer.h"
#include "BonePose.h"

using namespace LooterStancePoseDetail;

// Aiming down the sights in third person (the user, 2026-10-22: "when they aim down sights don't have an aim-in animation
// in 3rd person"). The hip hold keeps the gun at the chest; aimed, it comes up to the eye: the head drops onto it (canted
// to a long gun's stock), and both hands move until the gun's sight line runs through the eye with its grip a set reach
// ahead of it (the stock in the shoulder; a six-gun out in both hands), the elbows tucked in under it. The eye is the
// hidden mannequin's moved by the worn hero's hold and eye offset, so the sight comes up to the hero's own eye; it blends
// in by the view's eased aim, which runs on each gun's own aim time, and out the same way.

namespace
{
	/** The elbows' hint from where they are: down this far (cm), and out this far, at the hip hold and tucked in fully. */
	constexpr float ElbowDownHip = 20.f;
	constexpr float ElbowDownTucked = 35.f;
	constexpr float ElbowOutHip = 10.f;
	constexpr float ElbowOutTucked = 2.f;
	/** The neck's share of the head's drop onto the gun (the head takes the rest), and a six-gun's share of the cant. */
	constexpr float NeckShare = 0.4f;
	constexpr float SixGunCant = 0.3f;
}

float LooterStancePoseDetail::AimShare(const FLooterStanceInput& Stance)
{
	if (!Stance.bHandOnForegrip)
	{
		return 0.f;
	}
	return FMath::Clamp(Stance.AimDownSights, 0.f, 1.f) * FMath::Clamp(Stance.Aim.Raise, 0.f, 1.f) * (1.f - FMath::Clamp(Stance.ReloadWeight, 0.f, 1.f))
		* (1.f - Stance.Carry.Weight()) * (1.f - FMath::Clamp(Stance.Climb.Hands, 0.f, 1.f)) * (1.f - ThrowMotion::GunDipShare(Stance.ThrowTime));
}

FVector LooterStancePoseDetail::AimEye(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Forward,
	const FVector& Right)
{
	const FCompactPoseBoneIndex Head = FindBone(Bones, TEXT("head"));
	if (!Head.IsValid())
	{
		return FVector::ZeroVector;
	}
	// In the body's frame pitched with the aim, as the chest (and the head on it) pitches with it; the hero's hold is their
	// head and shoulders against the mannequin's (HoldOffsetVector pitches it the same way).
	const FVector Local = Stance.EyeFromHead + Stance.Aim.EyeOffset;
	const FVector Level = Forward * Local.X + Right * Local.Y + FVector::UpVector * Local.Z;
	const FVector Pitched = FQuat(Right, FMath::DegreesToRadians(-Stance.AimPitch)).RotateVector(Level);
	return Pose.GetComponentSpaceTransform(Head).GetLocation() + Pitched + HoldOffsetVector(Stance, Forward, Right);
}

void LooterStancePoseDetail::ApplyAimDownSights(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FVector& Forward, const FVector& Right)
{
	const float Share = AimShare(Stance);
	const FCompactPoseBoneIndex HoldBone = FindBone(Bones, *Stance.HoldBone.ToString());
	if (Share <= UE_KINDA_SMALL_NUMBER || !HoldBone.IsValid())
	{
		return;
	}
	const FVector Up = FVector::UpVector;
	const FVector AimWay = Stance.AimRotation.GetForwardVector().GetSafeNormal(UE_SMALL_NUMBER, Forward);
	const bool bSixGun = Stance.HoldReach > 0.f || Stance.WeaponKind == EWeaponKind::Revolver;

	// The head down onto the gun (about Right tips forward), canted toward its shoulder: about the aim, the right side goes
	// down the negative way. A six-gun held out in both hands takes little cant.
	const float Down = FMath::DegreesToRadians(Stance.Aim.HeadDown * Share);
	const float Cant = FMath::DegreesToRadians(-Stance.Aim.HeadCant * Share * (bSixGun ? SixGunCant : 1.f));
	RotateBone(Pose, FindBone(Bones, TEXT("neck_01")), FQuat(Right, Down * NeckShare));
	RotateBone(Pose, FindBone(Bones, TEXT("head")), FQuat(AimWay, Cant) * FQuat(Right, Down * (1.f - NeckShare)));

	// Where the gun's sight is now, and how far the hands must move for its line along the aim to run through the eye with the
	// grip its reach ahead of the eye.
	const FVector Eye = AimEye(Pose, Bones, Stance, Forward, Right);
	const FVector Grip = (Stance.HoldSocketLocal * Pose.GetComponentSpaceTransform(HoldBone)).GetLocation();
	const FVector Sight = Grip + Stance.WeaponRotation.RotateVector(Stance.WeaponSight);
	const FVector Across = FVector::VectorPlaneProject(Sight - Eye, AimWay);
	const float Along = Stance.SightGripAhead - static_cast<float>(FVector::DotProduct(Grip - Eye, AimWay));
	const FVector Shift = (AimWay * Along - Across) * Share;

	// Both hands by it, keeping their turn (the gun hangs in the right hand's socket and moves with it; the left hand finds
	// the foregrip after), the elbows down and in under the gun as it comes up.
	const float Tuck = FMath::Clamp(Stance.Aim.ElbowIn, 0.f, 1.5f) * Share;
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
		const FVector Elbow = Pose.GetComponentSpaceTransform(LowerArm).GetLocation();
		const FVector Wrist = Pose.GetComponentSpaceTransform(Hand).GetLocation();
		const FVector Hint = Elbow + Shift - Up * FMath::Lerp(ElbowDownHip, ElbowDownTucked, Tuck)
			+ Right * (Side * FMath::Lerp(ElbowOutHip, ElbowOutTucked, FMath::Min(Tuck, 1.f)));
		SolveArm(Pose, UpperArm, LowerArm, Hand, Wrist + Shift, Hint, 1.f, /*bTurnHand*/ false);
	}
}
