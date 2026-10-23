#include "Weapons/ReloadMotion.h"
#include "Weapons/ReloadTuning.h"

// A reload as planned (the gun feel pass; ReloadTuning has the numbers): tactical reloads that skip the handle or leave
// the bolt home, the Ranchhand's shell-by-shell reload (each shell where ReloadTuning's seconds put it, the pump only from
// empty), and its pump racked between shots. The magazine's, bolt's and cylinder's own beats stay in ReloadMotion.cpp.

namespace
{
	// The pump at the end of a shell reload from empty, as shares of its own section (ReloadTuning's Pump seconds): the
	// hand on it, snapped back, shoved home; the rest of the section brings the gun back to its hold.
	constexpr float PumpRackFrom = 0.15f;
	constexpr float PumpRackBack = 0.35f;
	constexpr float PumpRackHome = 0.55f;
	/** With no pump, the gun goes back to its hold this share of a shell's turn after the last one seats. */
	constexpr float PoseOutAfterSeat = 0.1f;
	/** A shell's nudge on the gun as it seats, either side of the seat (base seconds). */
	constexpr float ShellNudge = 0.08f;
	/** The pose-out never starts later than this (a window that ends where it starts would never let the gun go). */
	constexpr float LatestPoseOut = 0.98f;

	// The rack between shots, as shares of its cycle: back by PumpCycleBackAt (the hull flies), held a beat, home.
	constexpr float RackHoldUntil = 0.55f;
	constexpr float RackHomeAt = 0.95f;

	float Rise(float P, float A, float B)
	{
		return FMath::SmoothStep(A, B, P);
	}

	float Window(float P, float A, float B, float C, float D)
	{
		return Rise(P, A, B) * (1.f - Rise(P, C, D));
	}

	float Pulse(float P, float T, float HalfWidth)
	{
		return Window(P, T - HalfWidth, T, T, T + HalfWidth);
	}

	/** A shell reload's beats as shares of its whole length (its seconds cancel out: the gun's reload scale doesn't move them). */
	struct FShellBeats
	{
		FReloadPlan Plan;
		ReloadTuning::FShellTiming Timing;
		float Total = 1.f;

		float At(float Seconds) const { return Total > 0.f ? Seconds / Total : 0.f; }
		float Seat(int32 Shell) const { return At(ReloadTuning::ShellSeatSeconds(Shell, Timing)); }
		float PumpAt(float Share) const { return At(Timing.Start + FMath::Max(Plan.Shells, 0) * Timing.Shell + Share * Timing.Pump); }
		float PoseIn() const { return At(Timing.Start); }
		float PoseOut() const
		{
			const float Out = Plan.bEmpty ? PumpAt(PumpRackHome)
				: At(ReloadTuning::ShellSeatSeconds(FMath::Max(Plan.Shells - 1, 0), Timing) + PoseOutAfterSeat * Timing.Shell);
			return FMath::Min(Out, LatestPoseOut);
		}
	};

	FShellBeats BeatsFor(const FReloadPlan& Plan)
	{
		FShellBeats Beats;
		Beats.Plan = Plan;
		Beats.Timing = ReloadTuning::ShellTiming();
		Beats.Total = FMath::Max(ReloadTuning::ShellReloadSeconds(Plan, Beats.Timing), 0.05f);
		return Beats;
	}

	/** A tactical reload of a magazine or helix skips its handle (there's a round chambered already). */
	bool SkipsHandle(EWeaponReloadPart Part, const FReloadPlan& Plan)
	{
		return !Plan.bEmpty && (Part == EWeaponReloadPart::Magazine || Part == EWeaponReloadPart::Helix);
	}
}

float LooterReload::ChoreographyProgress(EWeaponReloadPart Part, const FReloadPlan& Plan, float RawProgress)
{
	if (RawProgress < 0.f || !SkipsHandle(Part, Plan))
	{
		return RawProgress;
	}
	// The motion runs through everything but the handle at one pace, then jumps it: at both of its ends the gun is in its
	// reload pose, the magazine home and the handle forward, so the view and the hands carry on without a jerk.
	const float Skipped = HandleTo - HandleFrom;
	const float Choreography = FMath::Clamp(RawProgress, 0.f, 1.f) * (1.f - Skipped);
	return Choreography < HandleFrom ? Choreography : Choreography + Skipped;
}

void LooterReload::StepsFor(EWeaponReloadPart Part, const FReloadPlan& Plan, TArray<FReloadStepAt>& OutSteps)
{
	OutSteps.Reset();
	if (Part == EWeaponReloadPart::Pump && Plan.Shells > 0)
	{
		const FShellBeats Beats = BeatsFor(Plan);
		for (int32 Shell = 0; Shell < Plan.Shells; ++Shell)
		{
			OutSteps.Add({ Beats.Seat(Shell), EReloadStep::ShellIn });
		}
		// From empty the pump chambers the first shell (and throws the hull the last shot left).
		if (Plan.bEmpty)
		{
			OutSteps.Add({ Beats.PumpAt(PumpRackFrom), EReloadStep::Pump });
		}
		return;
	}
	for (const FReloadStepAt& Step : Steps(Part))
	{
		if (!Plan.bEmpty)
		{
			// A round's chambered: no handle to work, and the sniper's bolt stays home (the box only).
			if (SkipsHandle(Part, Plan) && Step.Step == EReloadStep::Bolt)
			{
				continue;
			}
			if (Part == EWeaponReloadPart::Bolt && (Step.Step == EReloadStep::BoltOpen || Step.Step == EReloadStep::BoltClose))
			{
				continue;
			}
		}
		OutSteps.Add(Step);
	}
}

bool LooterReload::CommitsAt(EWeaponReloadPart Part, EReloadStep Step)
{
	switch (Part)
	{
	case EWeaponReloadPart::Magazine:
	case EWeaponReloadPart::Bolt:
		return Step == EReloadStep::MagIn;
	case EWeaponReloadPart::Helix:
		return Step == EReloadStep::HelixIn;
	case EWeaponReloadPart::Box:
		return Step == EReloadStep::BoxIn;
	case EWeaponReloadPart::Rocket:
		return Step == EReloadStep::RocketIn;
	case EWeaponReloadPart::Cylinder:
		return Step == EReloadStep::RoundsIn;
	case EWeaponReloadPart::Pump:
		return Step == EReloadStep::ShellIn;
	case EWeaponReloadPart::None:
	default:
		return false;
	}
}

float LooterReload::PumpTravel(float Progress, const FReloadPlan& Plan)
{
	if (Plan.Shells <= 0)
	{
		return PumpTravel(Progress);
	}
	if (!Plan.bEmpty || Progress < 0.f)
	{
		return 0.f;
	}
	const FShellBeats Beats = BeatsFor(Plan);
	const float Back = Beats.PumpAt(PumpRackBack);
	return PumpStroke * (Rise(Progress, Beats.PumpAt(PumpRackFrom), Back) - Rise(Progress, Back, Beats.PumpAt(PumpRackHome)));
}

float LooterReload::BoltOpenInReload(float Progress, const FReloadPlan& Plan)
{
	return Plan.bEmpty ? BoltOpenInReload(Progress) : 0.f;
}

float LooterReload::BoltHandInReload(float Progress, const FReloadPlan& Plan)
{
	return Plan.bEmpty ? BoltHandInReload(Progress) : 0.f;
}

void LooterReload::PumpViewModelPose(float Progress, const FReloadPlan& Plan, FVector& OutOffset, FRotator& OutRotation)
{
	OutOffset = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;
	if (Progress < 0.f)
	{
		return;
	}
	// The old reload's pose (its loading port turned up to the player), in and out on the plan's own beats.
	const FShellBeats Beats = BeatsFor(Plan);
	const float Pose = Window(Progress, 0.f, Beats.PoseIn(), Beats.PoseOut(), 1.f);
	OutOffset = FVector(-1.f, -5.f, 2.f) * Pose;
	OutRotation = FRotator(10.f, -12.f, 30.f) * Pose;

	// Each shell nudges the gun as it's thumbed home; racking the pump rocks it back.
	const float Half = Beats.At(ShellNudge);
	for (int32 Shell = 0; Shell < Plan.Shells; ++Shell)
	{
		const float Push = Pulse(Progress, Beats.Seat(Shell), Half);
		OutOffset.Z += 1.f * Push;
		OutRotation.Pitch += 2.f * Push;
	}
	const float Rack = PumpTravel(Progress, Plan) / PumpStroke;
	OutOffset.X -= 1.5f * Rack;
	OutRotation.Pitch -= 2.f * Rack;
}

float LooterReload::PumpCycleTravel(float CycleProgress)
{
	if (CycleProgress < 0.f || CycleProgress >= 1.f)
	{
		return 0.f;
	}
	// Snapped back hard (the hull flies as it bottoms), held a beat, shoved home.
	return PumpStroke * Rise(CycleProgress, 0.f, PumpCycleBackAt) * (1.f - Rise(CycleProgress, RackHoldUntil, RackHomeAt));
}

void LooterReload::PumpCycleViewPose(float CycleProgress, FVector& OutOffset, FRotator& OutRotation)
{
	OutOffset = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;
	if (CycleProgress < 0.f || CycleProgress >= 1.f)
	{
		return;
	}
	// The support hand's pull rocks the gun back and cants it toward that hand; the shove home bumps it forward.
	const float Rack = PumpCycleTravel(CycleProgress) / PumpStroke;
	const float Shove = Pulse(CycleProgress, RackHomeAt - 0.03f, 0.06f);
	OutOffset = FVector(-1.5f, -0.3f, -0.6f) * Rack + FVector(0.8f, 0.f, 0.f) * Shove;
	OutRotation = FRotator(-2.f, 0.f, -3.f) * Rack + FRotator(0.8f, 0.f, 1.f) * Shove;
}
