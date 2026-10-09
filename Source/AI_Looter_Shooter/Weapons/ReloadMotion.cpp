#include "Weapons/ReloadMotion.h"

namespace
{
	// Magazine reload, as fractions of the reload time.
	constexpr float MagOutStart = 0.16f;   // release pressed, the old magazine starts to slide
	constexpr float MagOutEnd = 0.28f;     // clear of the well
	constexpr float MagGoneEnd = 0.40f;    // dropped out of sight
	constexpr float MagInStart = 0.46f;    // the fresh one comes up
	constexpr float MagSeated = 0.62f;     // slapped home
	constexpr float BoltStart = 0.70f;     // charging handle
	constexpr float BoltEnd = 0.82f;
	constexpr float MagSlideOut = 12.f;    // cm out of the well before it falls
	constexpr float MagDrop = 60.f;        // cm it falls, well out of view
	constexpr float MagInFrom = 26.f;      // cm below the well the fresh one starts

	// Shotgun reload.
	constexpr int32 ShellPushes = 4;
	constexpr float ShellStart = 0.18f;
	constexpr float ShellEnd = 0.76f;
	constexpr float PumpStart = 0.80f;
	constexpr float PumpBack = 0.86f;
	constexpr float PumpEnd = 0.93f;
	constexpr float PumpStroke = 7.f;

	// Revolver reload: the cylinder flicks out, the gun tips up and the rod is punched, it tips down for the speedloader,
	// and the cylinder is slapped home.
	constexpr float CylOutStart = 0.12f;
	constexpr float CylOutEnd = 0.20f;
	constexpr float EjectAt = 0.30f;
	constexpr float RoundsInAt = 0.56f;
	constexpr float CylInStart = 0.66f;
	constexpr float CylInEnd = 0.72f;

	// Getting the gun into and out of the reload pose.
	constexpr float PoseIn = 0.14f;
	constexpr float PoseOut = 0.86f;

	/** 0 before A, eases up to 1 at B. */
	float Rise(float P, float A, float B)
	{
		return FMath::SmoothStep(A, B, P);
	}

	/** Eases up from A to B, holds, eases back down from C to D. */
	float Window(float P, float A, float B, float C, float D)
	{
		return Rise(P, A, B) * (1.f - Rise(P, C, D));
	}

	/** A quick bump that peaks at T. */
	float Pulse(float P, float T, float HalfWidth)
	{
		return Window(P, T - HalfWidth, T, T, T + HalfWidth);
	}

	/** When shell Shell (from 0) is pushed home: its nudge's peak. */
	constexpr float ShellAt(int32 Shell)
	{
		return ShellStart + (ShellEnd - ShellStart) * (Shell + 0.5f) / ShellPushes;
	}

	// The sounds land on the motion: the release as the magazine starts to slide, the slap as the fresh one seats, the
	// handle as it's yanked; each shell at its push, the pump as it starts back.
	constexpr FReloadStepAt MagazineSteps[] = {
		{ MagOutStart + 0.01f, EReloadStep::MagOut },
		{ MagSeated, EReloadStep::MagIn },
		{ BoltStart + 0.02f, EReloadStep::Bolt },
	};
	constexpr FReloadStepAt PumpSteps[] = {
		{ ShellAt(0), EReloadStep::ShellIn },
		{ ShellAt(1), EReloadStep::ShellIn },
		{ ShellAt(2), EReloadStep::ShellIn },
		{ ShellAt(3), EReloadStep::ShellIn },
		{ PumpStart, EReloadStep::Pump },
	};
	static_assert(ShellPushes == 4, "PumpSteps lists one ShellIn per shell pushed");
	// The latch's click as the cylinder starts out, the rod's punch, the rounds dropping in, the cylinder hitting home.
	constexpr FReloadStepAt CylinderSteps[] = {
		{ CylOutStart + 0.01f, EReloadStep::CylinderOut },
		{ EjectAt, EReloadStep::Eject },
		{ RoundsInAt, EReloadStep::RoundsIn },
		{ CylInEnd, EReloadStep::CylinderIn },
	};
}

TConstArrayView<FReloadStepAt> LooterReload::Steps(EWeaponReloadPart Part)
{
	switch (Part)
	{
	case EWeaponReloadPart::Magazine:
		return MakeArrayView(MagazineSteps);
	case EWeaponReloadPart::Pump:
		return MakeArrayView(PumpSteps);
	case EWeaponReloadPart::Cylinder:
		return MakeArrayView(CylinderSteps);
	default:
		return {};
	}
}

float LooterReload::CylinderSwing(float Progress)
{
	if (Progress < CylOutStart || Progress >= CylInEnd)
	{
		return 0.f;
	}
	if (Progress < CylOutEnd)
	{
		// Flicked out: fast at first, a touch past full as it reaches the stop, then settling on it.
		const float T = (Progress - CylOutStart) / (CylOutEnd - CylOutStart);
		return CylinderOpenDegrees * (1.f - FMath::Pow(1.f - T, 3.f) + 0.06f * FMath::Sin(UE_PI * T));
	}
	if (Progress < CylInStart)
	{
		return CylinderOpenDegrees;
	}
	// Pushed home, speeding up until it latches.
	const float T = (Progress - CylInStart) / (CylInEnd - CylInStart);
	return CylinderOpenDegrees * (1.f - T * T);
}

float LooterReload::MagazineTravel(float Progress, bool& bOutVisible)
{
	bOutVisible = true;
	if (Progress < MagOutStart || Progress >= MagSeated)
	{
		return 0.f;
	}
	if (Progress < MagOutEnd)
	{
		const float T = (Progress - MagOutStart) / (MagOutEnd - MagOutStart);
		return MagSlideOut * T * T;
	}
	if (Progress < MagGoneEnd)
	{
		// Falls away, speeding up.
		const float T = (Progress - MagOutEnd) / (MagGoneEnd - MagOutEnd);
		return MagSlideOut + MagDrop * T * T;
	}
	if (Progress < MagInStart)
	{
		bOutVisible = false;
		return MagSlideOut + MagDrop;
	}
	// The fresh one rises into the well and slows as it seats.
	const float T = (Progress - MagInStart) / (MagSeated - MagInStart);
	return MagInFrom * FMath::Square(1.f - T);
}

float LooterReload::PumpTravel(float Progress)
{
	return PumpStroke * (Rise(Progress, PumpStart, PumpBack) - Rise(Progress, PumpBack, PumpEnd));
}

void LooterReload::ViewModelPose(EWeaponReloadPart Part, float Progress, FVector& OutOffset, FRotator& OutRotation)
{
	OutOffset = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;
	if (Progress < 0.f || Part == EWeaponReloadPart::None)
	{
		return;
	}

	// Bring the gun up toward the middle of the view and roll it so the side being worked on faces the player (low and
	// flat, the magazine swap would happen below the bottom of the screen).
	const float Pose = Window(Progress, 0.f, PoseIn, PoseOut, 1.f);
	if (Part == EWeaponReloadPart::Magazine)
	{
		OutOffset = FVector(-1.f, -6.f, 3.5f) * Pose;
		OutRotation = FRotator(10.f, -14.f, 34.f) * Pose;

		// A tug as the old magazine comes free, a slap as the new one seats, then a yank on the charging handle.
		const float Tug = Pulse(Progress, (MagOutStart + MagOutEnd) * 0.5f, 0.05f);
		const float Slap = Pulse(Progress, MagSeated, 0.04f);
		const float Bolt = Window(Progress, BoltStart, BoltStart + 0.03f, BoltEnd - 0.05f, BoltEnd);
		OutOffset += FVector(0.f, 0.f, 0.8f) * Tug + FVector(0.f, 0.f, 1.8f) * Slap + FVector(-2.5f, 0.f, 0.3f) * Bolt;
		OutRotation += FRotator(3.f, 0.f, -2.f) * Slap + FRotator(0.f, 5.f, -10.f) * Bolt;
		return;
	}

	if (Part == EWeaponReloadPart::Cylinder)
	{
		// Rolled so its left side (where the cylinder swings out) faces the player, a little higher and nearer than a
		// long gun, since a six-gun is worked in front of the chest.
		OutOffset = FVector(-1.5f, -4.5f, 3.f) * Pose;
		OutRotation = FRotator(8.f, -12.f, 26.f) * Pose;

		// The wrist flicks as the cylinder swings out; the muzzle tips up and the palm punches the rod; it tips down for the
		// speedloader, which pushes in; the cylinder is slapped home from the left.
		const float Flick = Pulse(Progress, CylOutEnd, 0.04f);
		const float TipUp = Window(Progress, 0.22f, 0.29f, 0.34f, 0.44f);
		const float Punch = Pulse(Progress, EjectAt, 0.03f);
		const float TipDown = Window(Progress, 0.44f, 0.50f, 0.60f, CylInStart);
		const float Push = Pulse(Progress, RoundsInAt, 0.035f);
		const float Slap = Pulse(Progress, CylInEnd, 0.035f);
		OutOffset += FVector(0.f, -1.f, 0.f) * Flick + FVector(-2.f, 0.f, 3.f) * TipUp + FVector(0.f, 0.f, -1.5f) * Punch
			+ FVector(0.f, 0.f, -0.5f) * TipDown + FVector(1.f, 0.f, 0.f) * Push + FVector(0.f, 0.8f, 0.4f) * Slap;
		OutRotation += FRotator(0.f, 0.f, 6.f) * Flick + FRotator(40.f, 0.f, 0.f) * TipUp + FRotator(4.f, 0.f, 0.f) * Punch
			+ FRotator(-12.f, 0.f, 0.f) * TipDown + FRotator(-2.f, 0.f, 0.f) * Push + FRotator(2.f, 0.f, -8.f) * Slap;
		return;
	}

	OutOffset = FVector(-1.f, -5.f, 2.f) * Pose;
	OutRotation = FRotator(10.f, -12.f, 30.f) * Pose;

	// Each shell pushed in nudges the gun; racking the pump rocks it back.
	for (int32 Shell = 0; Shell < ShellPushes; ++Shell)
	{
		const float Push = Pulse(Progress, ShellStart + (ShellEnd - ShellStart) * (Shell + 0.5f) / ShellPushes, 0.035f);
		OutOffset.Z += 1.f * Push;
		OutRotation.Pitch += 2.f * Push;
	}
	const float Rack = PumpTravel(Progress) / PumpStroke;
	OutOffset.X -= 1.5f * Rack;
	OutRotation.Pitch -= 2.f * Rack;
}
