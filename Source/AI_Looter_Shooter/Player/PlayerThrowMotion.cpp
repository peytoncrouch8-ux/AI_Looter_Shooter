#include "Player/PlayerThrowMotion.h"
#include "Player/PlayerThrowRules.h"

namespace
{
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
	// Drawn back by the cheek on the left, tipped back as the wrist cocks.
	FThrowPose Pose;
	Pose.Offset = FVector(20.f, -17.f, -11.f);
	Pose.Rotation = FRotator(35.f, 10.f, -12.f);
	return Pose;
}

FThrowPose ThrowMotion::JarReleasePose()
{
	// Slung forward and up toward the middle of the view, the wrist snapping it over.
	FThrowPose Pose;
	Pose.Offset = FVector(40.f, -9.f, -5.f);
	Pose.Rotation = FRotator(-25.f, -5.f, 5.f);
	return Pose;
}

FThrowPose ThrowMotion::JarPose(float ThrowTime, bool& bOutShown)
{
	bOutShown = ThrowTime > 0.f && ThrowTime < FThrowRules::ReleaseSeconds;
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

FThrowPose ThrowMotion::GunPose(float ThrowTime)
{
	if (ThrowTime <= 0.f || ThrowTime >= FThrowRules::ThrowSeconds)
	{
		return FThrowPose();
	}
	float Share = 1.f;
	if (ThrowTime < DipInSeconds)
	{
		Share = FMath::SmoothStep(0.f, 1.f, ThrowTime / DipInSeconds);
	}
	else if (ThrowTime > DipHoldSeconds)
	{
		Share = 1.f - FMath::SmoothStep(0.f, 1.f, (ThrowTime - DipHoldSeconds) / (FThrowRules::ThrowSeconds - DipHoldSeconds));
	}
	return Scaled(GunDipPose(), Share);
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
