#include "Weapons/ReloadMotion.h"

namespace
{
	// Magazine reload, as fractions of the reload time.
	constexpr float MagOutStart = 0.16f;   // release pressed, the old magazine starts to slide
	constexpr float MagOutEnd = 0.28f;     // clear of the well
	constexpr float MagGoneEnd = 0.40f;    // dropped out of sight
	constexpr float MagInStart = 0.46f;    // the fresh one comes up
	constexpr float MagSeated = 0.62f;     // slapped home
	constexpr float BoltStart = LooterReload::HandleFrom;     // charging handle
	constexpr float BoltEnd = LooterReload::HandleTo;
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

	// The four frontier-tech kinds (Phase 12.3). The SMG's helix and the LMG's box come out and go in on the magazine's
	// beats; the SMG's knob is racked where the rifle's handle is, the LMG's belt laid in its feed there instead.
	constexpr FReloadStepAt HelixSteps[] = {
		{ MagOutStart + 0.01f, EReloadStep::HelixOut },
		{ MagSeated, EReloadStep::HelixIn },
		{ BoltStart + 0.02f, EReloadStep::Bolt },
	};
	constexpr FReloadStepAt BoxSteps[] = {
		{ MagOutStart + 0.01f, EReloadStep::BoxOut },
		{ MagSeated, EReloadStep::BoxIn },
		{ BoltStart + 0.02f, EReloadStep::BeltLay },
	};

	// The sniper's reload: the bolt turned up and drawn back, the box out and in (the magazine's beats, a little later),
	// the bolt run home.
	constexpr float BoltOpenStart = 0.04f;
	constexpr float BoltOpenEnd = 0.16f;
	constexpr float BoltCloseStart = 0.76f;
	constexpr float BoltCloseEnd = 0.88f;
	constexpr FReloadStepAt BoltSteps[] = {
		{ BoltOpenStart + 0.02f, EReloadStep::BoltOpen },
		{ MagOutStart + LooterReload::BoltMagazineLag + 0.01f, EReloadStep::MagOut },
		{ MagSeated + LooterReload::BoltMagazineLag, EReloadStep::MagIn },
		{ BoltCloseStart + 0.02f, EReloadStep::BoltClose },
	};
	static_assert(MagSeated + LooterReload::BoltMagazineLag < BoltCloseStart, "The box seats before the bolt runs home");

	// The launcher's reload: the next rocket drawn from the back, brought up in front of the mouth, slid in until it seats.
	constexpr float RocketDrawAt = 0.14f;
	constexpr float RocketShownAt = 0.34f;
	constexpr float RocketSeatedAt = 0.66f;
	constexpr FReloadStepAt RocketSteps[] = {
		{ RocketDrawAt, EReloadStep::RocketOut },
		{ RocketSeatedAt, EReloadStep::RocketIn },
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
	case EWeaponReloadPart::Bolt:
		return MakeArrayView(BoltSteps);
	case EWeaponReloadPart::Box:
		return MakeArrayView(BoxSteps);
	case EWeaponReloadPart::Rocket:
		return MakeArrayView(RocketSteps);
	case EWeaponReloadPart::Helix:
		return MakeArrayView(HelixSteps);
	case EWeaponReloadPart::None:
	default:
		return {};
	}
}

float LooterReload::WellTravel(EWeaponReloadPart Part, float Progress, bool& bOutVisible)
{
	bOutVisible = true;
	switch (Part)
	{
	case EWeaponReloadPart::Magazine:
	case EWeaponReloadPart::Helix:
	case EWeaponReloadPart::Box:
		return MagazineTravel(Progress, bOutVisible);
	case EWeaponReloadPart::Bolt:
		// Before the lag the box is still seated (MagazineTravel is 0 there too).
		return MagazineTravel(Progress - BoltMagazineLag, bOutVisible);
	default:
		return 0.f;
	}
}

float LooterReload::BoltOpenInReload(float Progress)
{
	return Rise(Progress, BoltOpenStart, BoltOpenEnd) * (1.f - Rise(Progress, BoltCloseStart, BoltCloseEnd));
}

float LooterReload::BoltHandInReload(float Progress)
{
	// Off the grip just before the bolt opens and back once it's drawn; again for running it home.
	return Window(Progress, 0.f, BoltOpenStart, BoltOpenEnd + 0.02f, BoltOpenEnd + 0.08f)
		+ Window(Progress, BoltCloseStart - 0.07f, BoltCloseStart, BoltCloseEnd, BoltCloseEnd + 0.06f);
}

float LooterReload::BoltHandInCycle(float CycleProgress)
{
	if (CycleProgress < 0.f || CycleProgress >= 1.f)
	{
		return 0.f;
	}
	// The hand leaves the grip as the cycle starts, works the knob, and is back on the grip as the bolt turns down.
	return Window(CycleProgress, 0.f, 0.12f, 0.88f, 1.f);
}

float LooterReload::BoltCycleOpen(float CycleProgress)
{
	if (CycleProgress < 0.f || CycleProgress >= 1.f)
	{
		return 0.f;
	}
	// Snapped open fast (lifted, yanked back), held a beat as the case flies, run home a touch slower.
	return Rise(CycleProgress, 0.f, BoltCaseOutAt) * (1.f - Rise(CycleProgress, 0.56f, 0.96f));
}

void LooterReload::BoltPose(float Open, float& OutLiftDegrees, float& OutBack)
{
	const float A = FMath::Clamp(Open, 0.f, 1.f);
	// The first half turns the handle up out of its notch; only then can it be drawn back.
	OutLiftDegrees = BoltLiftDegrees * FMath::Clamp(A * 2.f, 0.f, 1.f);
	OutBack = BoltBack * FMath::Clamp(A * 2.f - 1.f, 0.f, 1.f);
}

float LooterReload::RocketTravel(float Progress, bool& bOutShown, bool& bOutSeated)
{
	bOutShown = Progress >= RocketShownAt && Progress < RocketSeatedAt;
	bOutSeated = Progress >= RocketSeatedAt;
	if (!bOutShown)
	{
		return bOutSeated ? 0.f : RocketReach;
	}
	// Brought up in front of the mouth, then pushed in, slowing as it seats.
	const float T = (Progress - RocketShownAt) / (RocketSeatedAt - RocketShownAt);
	return RocketReach * FMath::Square(1.f - T);
}

void LooterReload::BoltCycleViewPose(float CycleProgress, FVector& OutOffset, FRotator& OutRotation)
{
	OutOffset = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;
	if (CycleProgress < 0.f || CycleProgress >= 1.f)
	{
		return;
	}
	// The gun cants toward the bolt hand and dips a little as the bolt is yanked, and settles back as it's run home.
	const float Work = Window(CycleProgress, 0.f, 0.25f, 0.6f, 1.f);
	const float Yank = Pulse(CycleProgress, BoltCaseOutAt * 0.8f, 0.12f);
	const float Home = Pulse(CycleProgress, 0.9f, 0.08f);
	OutOffset = FVector(-0.5f, 0.5f, -0.8f) * Work + FVector(-1.2f, 0.f, 0.f) * Yank + FVector(0.6f, 0.f, 0.f) * Home;
	OutRotation = FRotator(-1.5f, 1.f, 9.f) * Work + FRotator(1.f, 0.f, 2.f) * Yank + FRotator(-0.8f, 0.f, -1.5f) * Home;
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
	// The whole choreography, from empty (a pump gun's the old four shells).
	FReloadPlan FromEmpty;
	FromEmpty.bEmpty = true;
	ViewModelPose(Part, Progress, FromEmpty, OutOffset, OutRotation);
}

void LooterReload::ViewModelPose(EWeaponReloadPart Part, float Progress, const FReloadPlan& Plan, FVector& OutOffset, FRotator& OutRotation)
{
	OutOffset = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;
	if (Progress < 0.f || Part == EWeaponReloadPart::None)
	{
		return;
	}
	// A planned shell reload has its own beats (ReloadMotionPlan.cpp); no shells planned is the old four-shell one below.
	if (Part == EWeaponReloadPart::Pump && Plan.Shells > 0)
	{
		PumpViewModelPose(Progress, Plan, OutOffset, OutRotation);
		return;
	}

	// Bring the gun up toward the middle of the view and roll it so the side being worked on faces the player (low and
	// flat, the magazine swap would happen below the bottom of the screen).
	const float Pose = Window(Progress, 0.f, PoseIn, PoseOut, 1.f);
	if (Part == EWeaponReloadPart::Helix || Part == EWeaponReloadPart::Box)
	{
		// The helix runs under the barrel, the belt box hangs under the receiver's front: the gun rolls its left side and
		// underside up to the player, nose a touch up, so the swap under it shows. The LMG is heavier: less of a turn, a
		// deeper dip as the box is hung on, and the belt laid in its feed where the rifle's handle is yanked.
		const bool bHelix = Part == EWeaponReloadPart::Helix;
		OutOffset = (bHelix ? FVector(-2.f, -6.f, 7.f) : FVector(-2.f, -5.f, 5.f)) * Pose;
		OutRotation = (bHelix ? FRotator(6.f, -10.f, 32.f) : FRotator(4.f, -10.f, 24.f)) * Pose;
		const float Tug = Pulse(Progress, (MagOutStart + MagOutEnd) * 0.5f, 0.05f);
		const float Slap = Pulse(Progress, MagSeated, 0.04f);
		const float Last = Window(Progress, BoltStart, BoltStart + 0.03f, BoltEnd - 0.05f, BoltEnd);
		OutOffset += FVector(0.f, 0.f, 0.8f) * Tug + FVector(0.f, 0.f, bHelix ? 1.6f : 2.4f) * Slap
			+ (bHelix ? FVector(-2.f, 0.f, 0.3f) : FVector(0.f, 0.f, -1.2f)) * Last;
		OutRotation += FRotator(3.f, 0.f, -2.f) * Slap + (bHelix ? FRotator(0.f, 4.f, -8.f) : FRotator(-2.f, 0.f, 6.f)) * Last;
		return;
	}

	if (Part == EWeaponReloadPart::Bolt)
	{
		// The sniper: rolled to its right (the bolt) for the bolt, over to show the box under it, back for the bolt home.
		// A tactical reload leaves the bolt home: the box only.
		const float Worked = Plan.bEmpty ? 1.f : 0.f;
		const float BoltWork = Worked * Window(Progress, 0.f, BoltOpenStart, BoltOpenEnd, BoltOpenEnd + 0.08f)
			+ Worked * Window(Progress, BoltCloseStart - 0.08f, BoltCloseStart, BoltCloseEnd, BoltCloseEnd + 0.06f);
		OutOffset = FVector(-2.f, -6.f, 6.f) * Pose + FVector(0.f, 1.f, -1.f) * BoltWork;
		OutRotation = FRotator(3.f, -10.f, 24.f) * Pose + FRotator(0.f, 2.f, -30.f) * BoltWork;
		const float Lag = BoltMagazineLag;
		const float Slap = Pulse(Progress, MagSeated + Lag, 0.04f);
		const float Yank = Worked * Pulse(Progress, BoltOpenEnd - 0.03f, 0.04f);
		const float Home = Worked * Pulse(Progress, BoltCloseEnd - 0.04f, 0.04f);
		OutOffset += FVector(0.f, 0.f, 1.6f) * Slap + FVector(-1.5f, 0.f, 0.f) * Yank + FVector(1.f, 0.f, 0.f) * Home;
		OutRotation += FRotator(3.f, 0.f, -2.f) * Slap + FRotator(1.f, 0.f, 3.f) * Yank + FRotator(-1.f, 0.f, -2.f) * Home;
		return;
	}

	if (Part == EWeaponReloadPart::Rocket)
	{
		// The launcher is loaded at its mouth: lowered off the shoulder, muzzle tipped down and swung in toward the middle so
		// the mouth comes into view, the rocket pushed in, a shove as it seats.
		OutOffset = FVector(-14.f, -10.f, -9.f) * Pose;
		OutRotation = FRotator(-16.f, -28.f, 12.f) * Pose;
		const float Seat = Pulse(Progress, RocketSeatedAt, 0.05f);
		OutOffset += FVector(-2.f, 0.f, 0.5f) * Seat;
		OutRotation += FRotator(2.f, 0.f, -2.f) * Seat;
		return;
	}

	if (Part == EWeaponReloadPart::Magazine)
	{
		// The Bullpup's magazine is behind and under its grip, under the hip hold's view: the gun comes well up and in, its
		// muzzle level (tipped up, the magazine end went down out of the frame), so the support hand's work on it shows.
		OutOffset = FVector(-1.f, -7.f, 8.f) * Pose;
		OutRotation = FRotator(2.f, -14.f, 30.f) * Pose;

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
