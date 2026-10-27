#include "Player/PlayerMeleeMotion.h"
#include "Player/PlayerMeleeRules.h"
#include "HAL/IConsoleManager.h"

// MeleeMotion's gun and view: the gun dropped into the left hand while the tool swings, the view's kicks by swing style.
// The tool's arcs are PlayerMeleeToolMotion.cpp.

namespace
{
	TAutoConsoleVariable<float> CVarGunLower(TEXT("Looter.Melee.GunLower"), 1.f,
		TEXT("How far the gun drops out of view during a melee swing (a share of MeleeMotion::AwayPose)."));
	TAutoConsoleVariable<float> CVarGunLowerSeconds(TEXT("Looter.Melee.GunLowerSeconds"), 0.08f,
		TEXT("Seconds the gun takes to drop out of view as a melee swing starts."));
	TAutoConsoleVariable<float> CVarGunRaiseSeconds(TEXT("Looter.Melee.GunRaiseSeconds"), 0.08f,
		TEXT("Seconds the gun takes to come back up at a melee swing's end."));
	TAutoConsoleVariable<float> CVarHeftQuick(TEXT("Looter.Melee.Heft.Quick"), 0.8f,
		TEXT("A quick tool's wind-up size and view jolts against a steady one's (the razor fan, the rock pick, the piston fist)."));
	TAutoConsoleVariable<float> CVarHeftHeavy(TEXT("Looter.Melee.Heft.Heavy"), 1.25f,
		TEXT("A heavy tool's wind-up size and view jolts against a steady one's (the ferry gaff)."));

	FMeleeSwingPose Scaled(const FMeleeSwingPose& Pose, float Share)
	{
		FMeleeSwingPose Out;
		Out.Offset = Pose.Offset * Share;
		Out.Rotation = Pose.Rotation * Share;
		return Out;
	}
}

float MeleeMotion::HeftScale(EMeleeHeft Heft)
{
	switch (Heft)
	{
	case EMeleeHeft::Quick: return FMath::Clamp(CVarHeftQuick.GetValueOnAnyThread(), 0.3f, 2.f);
	case EMeleeHeft::Heavy: return FMath::Clamp(CVarHeftHeavy.GetValueOnAnyThread(), 0.3f, 2.f);
	default: return 1.f;
	}
}

FMeleeSwingPose MeleeMotion::AwayPose()
{
	// Dropped down out of the frame, muzzle tipped down and swung a little right, Borderlands' way: the swing has the view to
	// itself (the user's playtest, 2026-10-27: "Gun stays visible while using melee animation"; round 3 held it low in the
	// left hand, in view).
	FMeleeSwingPose Pose;
	Pose.Offset = FVector(-8.f, 6.f, -42.f);
	Pose.Rotation = FRotator(-35.f, 10.f, 15.f);
	return Pose;
}

float MeleeMotion::GunAwayShare(float SwingTime)
{
	if (SwingTime <= 0.f || SwingTime >= FMeleeRules::SwingSeconds)
	{
		return 0.f;
	}
	// Down over the first LowerSeconds of the wind-up, back up over the swing's last RaiseSeconds.
	const float Lower = FMath::Max(CVarGunLowerSeconds.GetValueOnAnyThread(), 0.01f);
	const float Raise = FMath::Max(CVarGunRaiseSeconds.GetValueOnAnyThread(), 0.01f);
	const float Down = FMath::SmoothStep(0.f, Lower, SwingTime);
	const float Up = FMath::SmoothStep(FMeleeRules::SwingSeconds - Raise, FMeleeRules::SwingSeconds, SwingTime);
	return Down * (1.f - Up);
}

bool MeleeMotion::IsSmearing(float SwingTime)
{
	// The drive starts at CockSeconds (a frame early so the band begins at the wind-up's top); the follow-through is the
	// last of the head's speed.
	return SwingTime >= CockSeconds * 0.9f && SwingTime <= FMeleeRules::ContactSeconds + FollowThroughSeconds;
}

bool MeleeMotion::IsGunAway(float SwingTime)
{
	// Fully down out of the frame: the gun (its attachments, laser and label with it) is hidden till it comes back up.
	return GunAwayShare(SwingTime) >= 0.98f;
}

FMeleeSwingPose MeleeMotion::GunPose(float SwingTime, bool bLanded)
{
	return Scaled(AwayPose(), GunAwayShare(SwingTime) * FMath::Clamp(CVarGunLower.GetValueOnAnyThread(), 0.f, 1.5f));
}

FViewKick MeleeMotion::WindUp(EMeleeSwingStyle Style, EMeleeHeft Heft)
{
	FViewKick Kick;
	switch (Style)
	{
	case EMeleeSwingStyle::Flick:
		// Coiled low to the left for the backhand: the view drawn down a touch and turned left.
		Kick.Pitch = -0.2f;
		Kick.Yaw = -0.5f;
		Kick.Roll = -0.5f;
		Kick.FieldOfView = 0.006f;
		break;
	case EMeleeSwingStyle::Chop:
	case EMeleeSwingStyle::Gaff:
	case EMeleeSwingStyle::Peck:
		// Raised high on the right: the view lifts with it.
		Kick.Pitch = 0.6f;
		Kick.Yaw = 0.3f;
		Kick.Roll = 0.5f;
		Kick.FieldOfView = 0.006f;
		break;
	case EMeleeSwingStyle::Jab:
		// Drawn back by the shoulder: the view eases back and widens a hair.
		Kick.Pitch = 0.25f;
		Kick.Yaw = 0.35f;
		Kick.Roll = 0.4f;
		Kick.FieldOfView = 0.01f;
		break;
	default:
		// The swat's coil, up and back to the right.
		Kick.Pitch = 0.35f;
		Kick.Yaw = 0.7f;
		Kick.Roll = 0.8f;
		Kick.FieldOfView = 0.006f;
		break;
	}
	// Slow enough to peak with the coil (about 90 ms in) and well damped, so it hands over cleanly to the strike's lean.
	Kick.Frequency = 3.f;
	Kick.Damping = 0.7f;
	const float Coil = HeftScale(Heft);
	Kick.Pitch *= Coil;
	Kick.Yaw *= Coil;
	Kick.Roll *= Coil;
	return Kick;
}

FViewKick MeleeMotion::Strike(EMeleeSwingStyle Style, EMeleeHeft Heft)
{
	FViewKick Kick;
	switch (Style)
	{
	case EMeleeSwingStyle::Flick:
		// Up and across to the right with the flick.
		Kick.Pitch = -0.4f;
		Kick.Yaw = 0.8f;
		Kick.Roll = 1.f;
		Kick.FieldOfView = -0.008f;
		Kick.Frequency = 8.f;
		break;
	case EMeleeSwingStyle::Chop:
	case EMeleeSwingStyle::Gaff:
	case EMeleeSwingStyle::Peck:
		// Down onto the blow, a little left.
		Kick.Pitch = -1.1f;
		Kick.Yaw = -0.35f;
		Kick.Roll = -0.7f;
		Kick.FieldOfView = -0.01f;
		Kick.Frequency = 8.f;
		break;
	case EMeleeSwingStyle::Jab:
		// The punch: a dip toward the blow and the view pushed in, as if the head went with the fist.
		Kick.Pitch = -0.9f;
		Kick.Yaw = -0.4f;
		Kick.Roll = -0.5f;
		// As much as the stack allows with a killing blow's punch landing on top of it.
		Kick.FieldOfView = -0.022f;
		Kick.Frequency = 8.f;
		break;
	default:
		// Leaning into the swat's way across, right to left and down.
		Kick.Pitch = -0.6f;
		Kick.Yaw = -1.f;
		Kick.Roll = -1.4f;
		Kick.FieldOfView = -0.008f;
		Kick.Frequency = 7.f;
		break;
	}
	Kick.Damping = 0.55f;
	// A heavy tool leans the view harder (held to 15% more, inside the stack's limits); the punch in stays as it is, at
	// the stack's edge with a killing blow on top.
	const float Lean = FMath::Min(HeftScale(Heft), 1.15f);
	Kick.Pitch *= Lean;
	Kick.Yaw *= Lean;
	Kick.Roll *= Lean;
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
