#include "Player/PlayerMeleeMotion.h"
#include "Player/PlayerMeleeRules.h"

namespace
{
	FMeleeSwingPose Blend(const FMeleeSwingPose& From, const FMeleeSwingPose& To, float Alpha)
	{
		FMeleeSwingPose Pose;
		Pose.Offset = FMath::Lerp(From.Offset, To.Offset, Alpha);
		// Each angle on its own: the swing's angles stay well inside a half turn, so no wrapping to mind.
		Pose.Rotation = From.Rotation + (To.Rotation - From.Rotation) * Alpha;
		return Pose;
	}

	FMeleeSwingPose Scaled(const FMeleeSwingPose& Pose, float Share)
	{
		FMeleeSwingPose Out;
		Out.Offset = Pose.Offset * Share;
		Out.Rotation = Pose.Rotation * Share;
		return Out;
	}
}

FMeleeSwingPose MeleeMotion::CockedPose()
{
	// The coil: drawn back toward the shoulder, the muzzle turned in a touch and the stock out to the right.
	FMeleeSwingPose Pose;
	Pose.Offset = FVector(-5.f, 3.f, -2.f);
	Pose.Rotation = FRotator(8.f, -12.f, -10.f);
	return Pose;
}

FMeleeSwingPose MeleeMotion::StrikePose()
{
	// Driven across: the muzzle swung out right and canted, so the stock behind the hands leads forward and left into the
	// middle of the view, where the blow lands.
	FMeleeSwingPose Pose;
	Pose.Offset = FVector(12.f, -8.f, 2.f);
	Pose.Rotation = FRotator(-8.f, 50.f, 30.f);
	return Pose;
}

FMeleeSwingPose MeleeMotion::GunPose(float SwingTime, bool bLanded)
{
	const float Contact = FMeleeRules::ContactSeconds;
	if (SwingTime <= 0.f || SwingTime >= FMeleeRules::SwingSeconds)
	{
		return FMeleeSwingPose();
	}
	if (SwingTime < CockSeconds)
	{
		return Blend(FMeleeSwingPose(), CockedPose(), FMath::SmoothStep(0.f, 1.f, SwingTime / CockSeconds));
	}
	if (SwingTime < Contact)
	{
		// Accelerating into the blow, so the stock is at its fastest as it lands: that speed is what reads as weight.
		const float Drive = (SwingTime - CockSeconds) / (Contact - CockSeconds);
		return Blend(CockedPose(), StrikePose(), Drive * Drive);
	}
	// Back to the hold: slow off the blow, quick through the middle, settling in.
	const float Back = (SwingTime - Contact) / (FMeleeRules::SwingSeconds - Contact);
	float Share = 1.f - FMath::SmoothStep(0.f, 1.f, Back);
	if (!bLanded)
	{
		// Nothing stopped it: it carries on past the strike pose, peaking FollowThroughSeconds after, then comes back.
		const float Through = FMath::Min((SwingTime - Contact) / (2.f * FollowThroughSeconds), 1.f);
		Share += FollowThrough * FMath::Sin(UE_PI * Through);
	}
	return Scaled(StrikePose(), Share);
}

FViewKick MeleeMotion::WindUp(bool bArmed)
{
	FViewKick Kick;
	if (bArmed)
	{
		// With the stock's coil: the view drawn up and back to the right.
		Kick.Pitch = 0.35f;
		Kick.Yaw = 0.7f;
		Kick.Roll = 0.8f;
		Kick.FieldOfView = 0.006f;
	}
	else
	{
		// The fist pulled back: the view eases back and widens a hair.
		Kick.Pitch = 0.25f;
		Kick.Yaw = 0.35f;
		Kick.Roll = 0.4f;
		Kick.FieldOfView = 0.01f;
	}
	// Quick (it peaks with the coil, about 45 ms in) and well damped, so it hands over cleanly to the strike's lean.
	Kick.Frequency = 4.f;
	Kick.Damping = 0.7f;
	return Kick;
}

FViewKick MeleeMotion::Strike(bool bArmed)
{
	FViewKick Kick;
	if (bArmed)
	{
		// Leaning into the swing's way across, right to left.
		Kick.Pitch = -0.6f;
		Kick.Yaw = -1.f;
		Kick.Roll = -1.4f;
		Kick.FieldOfView = -0.008f;
		Kick.Frequency = 7.f;
	}
	else
	{
		// The punch: a dip toward the blow and the view pushed in, as if the head went with the fist.
		Kick.Pitch = -0.9f;
		Kick.Yaw = -0.4f;
		Kick.Roll = -0.5f;
		// As much as the stack allows with a killing blow's punch landing on top of it.
		Kick.FieldOfView = -0.022f;
		Kick.Frequency = 8.f;
	}
	Kick.Damping = 0.55f;
	return Kick;
}

FViewKick MeleeMotion::Impact(bool bKill, float Lean)
{
	// The blow meets the body and the body pushes back: a sharp jolt up, a punch in, rolled a little to one side.
	FViewKick Kick;
	Kick.Pitch = 0.8f;
	Kick.Roll = 0.5f * (Lean < 0.f ? -1.f : 1.f);
	Kick.FieldOfView = -0.02f;
	Kick.Frequency = 13.f;
	Kick.Damping = 0.45f;
	return bKill ? Kick * 1.3f : Kick;
}

FViewKick MeleeMotion::WallKnock()
{
	FViewKick Kick;
	Kick.Pitch = 0.5f;
	Kick.FieldOfView = -0.01f;
	Kick.Frequency = 12.f;
	Kick.Damping = 0.5f;
	return Kick;
}
