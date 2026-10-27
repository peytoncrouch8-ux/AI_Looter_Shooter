#include "Player/PlayerThrowMotion.h"
#include "Combat/GraveSaltRules.h"
#include "Player/PlayerThrowRules.h"

namespace
{
	/**
	 * The tin's middle above its foot (cm; Art/Models/Throwables/SaltGrenade.py): the poses place its foot, its pivot,
	 * while the flying jar (AGraveSaltGrenade) is held about its middle.
	 */
	constexpr float JarMiddleUp = FGraveSaltRules::JarMiddle;

	FThrowPose Blend(const FThrowPose& From, const FThrowPose& To, float Alpha)
	{
		FThrowPose Pose;
		Pose.Offset = FMath::Lerp(From.Offset, To.Offset, Alpha);
		// Each angle on its own: the throw's angles stay well inside a half turn, so no wrapping to mind.
		Pose.Rotation = From.Rotation + (To.Rotation - From.Rotation) * Alpha;
		return Pose;
	}

	FThrowPose Scaled(const FThrowPose& Pose, float Share)
	{
		FThrowPose Out;
		Out.Offset = Pose.Offset * Share;
		Out.Rotation = Pose.Rotation * Share;
		return Out;
	}
}

FThrowPose ThrowMotion::JarStartPose()
{
	// Out of sight under the view's lower left, the jar upright in the fist.
	FThrowPose Pose;
	Pose.Offset = FVector(26.f, -20.f, -40.f);
	Pose.Rotation = FRotator(-10.f, 0.f, 25.f);
	return Pose;
}

FThrowPose ThrowMotion::JarDrawnPose()
{
	// Drawn back low on the left, tipped back as the wrist cocks. Held by the cheek (20, -17, -11) the jar, twice its first
	// size since 2026-10-27, covered a third of the view through the wind-up (DEMO's play-check): low in the corner its
	// top and fuse show the throw coming without blocking the aim.
	FThrowPose Pose;
	Pose.Offset = FVector(22.f, -19.f, -19.f);
	Pose.Rotation = FRotator(35.f, 10.f, -12.f);
	return Pose;
}

FThrowPose ThrowMotion::JarReleasePose()
{
	// Slung forward and in from the cheek, the wrist snapping it over, to just where the real jar starts its flight
	// (FThrowRules::ReleasePoint, low on the view's left), which then carries it on up toward the middle. Released at
	// 40 cm ahead and 5 under the eye, it left the hand near the crosshair and the flying one appeared 17 cm back and
	// down from it, a jump of a fifth of the view. (The jar here rides the camera at the player's size, the flying one
	// is full size: both scale about the eye, so on screen they meet.)
	FThrowPose Pose;
	Pose.Rotation = FRotator(-25.f, -5.f, 5.f);
	const FVector Middle(FThrowRules::ReleaseForward, FThrowRules::ReleaseRight, FThrowRules::ReleaseUp);
	Pose.Offset = Middle - Pose.Rotation.RotateVector(FVector(0.f, 0.f, JarMiddleUp));
	return Pose;
}

bool ThrowMotion::IsJarInHand(float ThrowTime)
{
	return ThrowTime > 0.f && ThrowTime < FThrowRules::ReleaseSeconds;
}

FThrowPose ThrowMotion::JarPose(float ThrowTime, bool& bOutShown)
{
	bOutShown = IsJarInHand(ThrowTime);
	if (ThrowTime <= 0.f)
	{
		return JarStartPose();
	}
	if (ThrowTime < DrawSeconds)
	{
		// Up into view, slowing as it reaches the cheek.
		return Blend(JarStartPose(), JarDrawnPose(), FMath::InterpEaseOut(0.f, 1.f, ThrowTime / DrawSeconds, 2.f));
	}
	// Slung: accelerating out of the draw, so it's at its fastest as it leaves the hand.
	const float Sling = FMath::Clamp((ThrowTime - DrawSeconds) / (FThrowRules::ReleaseSeconds - DrawSeconds), 0.f, 1.f);
	return Blend(JarDrawnPose(), JarReleasePose(), Sling * Sling);
}

FThrowPose ThrowMotion::GunDipPose()
{
	// Lowered and swung right, muzzle down and canted, so the left hand's throw has the middle of the view.
	FThrowPose Pose;
	Pose.Offset = FVector(-4.f, 4.f, -10.f);
	Pose.Rotation = FRotator(-12.f, 8.f, 16.f);
	return Pose;
}

float ThrowMotion::GunDipShare(float ThrowTime)
{
	if (ThrowTime <= 0.f || ThrowTime >= FThrowRules::ThrowSeconds)
	{
		return 0.f;
	}
	if (ThrowTime < DipInSeconds)
	{
		return FMath::SmoothStep(0.f, 1.f, ThrowTime / DipInSeconds);
	}
	if (ThrowTime > DipHoldSeconds)
	{
		return 1.f - FMath::SmoothStep(0.f, 1.f, (ThrowTime - DipHoldSeconds) / (FThrowRules::ThrowSeconds - DipHoldSeconds));
	}
	return 1.f;
}

FThrowPose ThrowMotion::GunPose(float ThrowTime)
{
	const float Share = GunDipShare(ThrowTime);
	return Share > 0.f ? Scaled(GunDipPose(), Share) : FThrowPose();
}

FViewKick ThrowMotion::WindUp()
{
	FViewKick Kick;
	Kick.Pitch = 0.3f;
	Kick.Yaw = -0.35f;
	Kick.Roll = -0.5f;
	Kick.FieldOfView = 0.005f;
	// Peaks with the draw and hands over to the release's lean.
	Kick.Frequency = 4.5f;
	Kick.Damping = 0.7f;
	return Kick;
}

FViewKick ThrowMotion::Release()
{
	FViewKick Kick;
	Kick.Pitch = -0.45f;
	Kick.Yaw = 0.4f;
	Kick.Roll = 0.7f;
	Kick.FieldOfView = -0.006f;
	Kick.Frequency = 7.f;
	Kick.Damping = 0.55f;
	return Kick;
}

FViewKick ThrowMotion::Burst(float Share, float Lean)
{
	const float Weight = FMath::Clamp(Share, 0.f, 1.f);
	FViewKick Kick;
	Kick.Pitch = 1.5f;
	Kick.Roll = 1.f * (Lean < 0.f ? -1.f : 1.f);
	Kick.FieldOfView = -0.03f;
	Kick.Frequency = 10.f;
	Kick.Damping = 0.4f;
	return Kick * Weight;
}
