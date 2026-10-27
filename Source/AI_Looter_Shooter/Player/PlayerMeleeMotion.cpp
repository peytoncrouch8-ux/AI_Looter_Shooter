#include "Player/PlayerMeleeMotion.h"
#include "Player/PlayerMeleeRules.h"
#include "HAL/IConsoleManager.h"

// MeleeMotion's gun and view: the gun dropped into the left hand while the tool swings, the view's kicks by swing style.
// The tool's arcs are PlayerMeleeToolMotion.cpp.

namespace
{
	TAutoConsoleVariable<float> CVarGunLower(TEXT("Looter.Melee.GunLower"), 1.f,
		TEXT("How far the gun drops into the left hand during a melee swing (a share of MeleeMotion::LowReadyPose; 0 keeps it up)."));
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

FMeleeSwingPose MeleeMotion::LowReadyPose()
{
	// Dropped low and to the left, muzzle down and canted over, held by the support hand alone: the right side of the view is
	// the tool's, and the gun still reads as the gun in hand (the throw dips it the other way, ThrowMotion::GunDipPose).
	FMeleeSwingPose Pose;
	Pose.Offset = FVector(-5.f, -9.f, -11.f);
	Pose.Rotation = FRotator(-14.f, -10.f, -26.f);
	return Pose;
}

FMeleeSwingPose MeleeMotion::GunPose(float SwingTime, bool bLanded)
{
	if (SwingTime <= 0.f || SwingTime >= FMeleeRules::SwingSeconds)
	{
		return FMeleeSwingPose();
	}
	// Down with the wind-up, held through the blow and its follow-through, back up as the tool goes away under the frame.
	const float Down = FMath::SmoothStep(0.f, CockSeconds, SwingTime);
	const float Up = FMath::SmoothStep(ReturnFrom, FMeleeRules::SwingSeconds, SwingTime);
	float Share = Down * (1.f - Up);
	// A landed blow jars the whole body: the gun gives a little more for an instant.
	if (bLanded && SwingTime > FMeleeRules::ContactSeconds)
	{
		const float Since = (SwingTime - FMeleeRules::ContactSeconds) / RecoilSeconds;
		Share *= 1.f + 0.12f * FMath::Sin(UE_PI * FMath::Clamp(Since, 0.f, 1.f));
	}
	return Scaled(LowReadyPose(), Share * FMath::Clamp(CVarGunLower.GetValueOnAnyThread(), 0.f, 1.5f));
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
