#include "Player/FirstPersonHandsMotion.h"
#include "Player/PlayerThrowMotion.h"
#include "Player/PlayerThrowRules.h"

// The support hand's grenade throw: the first-person jar rides the camera on ThrowMotion's poses
// (UPlayerThrowComponent), and the left hand is put round it on the same clock, so it carries the jar up from under the
// view, draws it back by the cheek and slings it, then opens and follows through and comes back to the gun.

namespace
{
	/** The tin's middle above its foot, its pivot (PlayerThrowMotion.cpp's), and its radius (SaltGrenade.py's R_BODY). */
	constexpr float JarMiddleUp = 5.f;
	constexpr float JarRadius = 3.75f;
	/** The throwing arm's forearm, leaving the wrist down, back and out to the left (camera space), as the jar comes up. */
	const FVector JarForearm(-0.35f, -0.55f, -0.75f);
	/** Off the gun and round the jar in this long (seconds), while it's still rising out of sight under the view. */
	constexpr float GrabSeconds = 0.05f;
	/** After the release: the hand carries on along the throw, opening (camera space, cm), over this long. */
	const FVector FollowThrough(8.f, 6.f, -12.f);
	constexpr float FollowSeconds = 0.1f;
	/** Back to the gun from here to here (seconds into the throw), as the gun comes up out of its dip. */
	constexpr float BackFrom = 0.28f;
	constexpr float BackBy = 0.46f;

	float Rise(float P, float A, float B)
	{
		return FMath::SmoothStep(A, B, P);
	}
}

FHandPlacement FirstPersonHandsMotion::OnJar(float ThrowTime)
{
	bool bShown = false;
	const FThrowPose Pose = ThrowMotion::JarPose(ThrowTime, bShown);
	const FQuat Turn = Pose.Rotation.Quaternion();
	const FVector JarUp = Turn.GetUpVector();
	const FVector Middle = Pose.Offset + JarUp * JarMiddleUp;
	// The palm on the jar's outer side (its left, a little behind), so the jar shows in front of the fingers; the jar's
	// axis runs pinky to index, the fuse standing up out of the fist. A left hand's knuckles are then palm cross that.
	const FVector Palm = Turn.RotateVector(FVector(0.35f, 1.f, 0.f)).GetSafeNormal();
	const FVector Fingers = FVector::CrossProduct(Palm, JarUp);
	// The palm on the tin's side: the grip point sits GripOffPalm off it, nearly on the tin's axis.
	return FHandPlacement::Make(Middle - Palm * (JarRadius - FirstPersonHands::GripOffPalm), Fingers, Palm, FHandFingers::Of(EHandGrip::Wrap),
		JarForearm);
}

FHandPlacement FirstPersonHandsMotion::SupportInThrow(const FHandPlacement& OnGun, float ThrowTime)
{
	const float Release = FThrowRules::ReleaseSeconds;
	if (ThrowTime <= 0.f || ThrowTime >= FThrowRules::ThrowSeconds)
	{
		return OnGun;
	}
	if (ThrowTime < GrabSeconds)
	{
		return FHandPlacement::Blend(OnGun, OnJar(ThrowTime), Rise(ThrowTime, 0.f, GrabSeconds));
	}
	if (ThrowTime < Release)
	{
		return OnJar(ThrowTime);
	}
	// The jar is gone where the release pose had it; the open hand follows through, then goes back to the gun.
	const FHandPlacement LetGo = OnJar(Release);
	FHandPlacement Thrown = LetGo;
	Thrown.Hold += FollowThrough;
	Thrown.Fingers = FHandFingers::Of(EHandGrip::Open);
	if (ThrowTime < Release + FollowSeconds)
	{
		return FHandPlacement::Blend(LetGo, Thrown, Rise(ThrowTime, Release, Release + FollowSeconds));
	}
	return FHandPlacement::Blend(Thrown, OnGun, Rise(ThrowTime, BackFrom, BackBy));
}
