#include "Weapons/ReloadMotion.h"
#include "Weapons/ReloadTuning.h"

// The reload's spring curves (ReloadCurves, ReloadMotion.h; Phase 3 change 12) and the first-person gun's jolt as a part
// seats or a handle slams (LooterReload::JoltPose), as plain math. A Snap is three pieces joined at rest, so nothing ever
// jumps: an eased wind-up the other way, half a swing of an underdamped spring past the end (quick, then slowing into its
// far point), and a critically damped settle back onto the end that is trimmed to land on it exactly at the window's end.

namespace
{
	/** The settle's stiffness over its own window: a critically damped return all but home (1.7%) by its end, then trimmed onto it. */
	constexpr float SnapTailStiffness = 6.f;
	/** A knock's kick takes this share of the time between its hit and its settle; the rebound past rest takes the rest. */
	constexpr float SnapKickShare = 0.4f;
	/** An overshoot under this is none (the spring's half swing needs one to have a shape). */
	constexpr float SnapLeastOvershoot = 0.001f;
	/** The jolt's way in the gun's space: back toward the shooter and down (a unit vector, so its size is the jolt's cm). */
	const FVector JoltWay(-0.8f, 0.f, -0.6f);
	/** The jolt is over once its swing can't reach this share of its peak again. */
	constexpr float JoltRungOut = 0.01f;

	/** A shape kept to one that starts and ends at rest with room for the move (the cvars are clamped too; a test's may not be). */
	struct FSnapParts
	{
		float Anticipate = 0.f;
		float Overshoot = 0.f;
		float Lead = 0.f;
		float Settle = 0.f;
	};

	FSnapParts PartsOf(const ReloadTuning::FSnapShape& Shape)
	{
		FSnapParts Parts;
		Parts.Lead = FMath::Clamp(Shape.Lead, 0.f, 0.45f);
		Parts.Anticipate = Parts.Lead > 0.f ? FMath::Clamp(Shape.Anticipate, 0.f, 0.6f) : 0.f;
		Parts.Settle = FMath::Clamp(Shape.Settle, 0.f, 0.9f - Parts.Lead);
		// With no settle the swing past the end would have to jump back: no overshoot then.
		Parts.Overshoot = Parts.Settle > 0.f ? FMath::Clamp(Shape.Overshoot, 0.f, 0.5f) : 0.f;
		return Parts;
	}

	float Ease(float T)
	{
		return FMath::SmoothStep(0.f, 1.f, T);
	}

	/**
	 * Half a swing of an underdamped spring let go at rest, from 0 to 1 and at rest again: R is its overshoot ratio
	 * (exp(-zeta pi / sqrt(1 - zeta^2))). Small R is quick at first and slow into its far point, as a sprung part moves; R
	 * near 1 is the cosine ease.
	 */
	float SpringHalf(float V, float R)
	{
		const float Beta = -FMath::Loge(R) / UE_PI;
		const float Swing = FMath::Pow(R, V) * (FMath::Cos(UE_PI * V) + Beta * FMath::Sin(UE_PI * V));
		return (1.f - Swing) / (1.f + R);
	}

	/** A critically damped return from 1 to 0 over W (0 to 1): leaving at rest, never past 0, trimmed to land on it at W 1. */
	float Tail(float W)
	{
		const float K = SnapTailStiffness;
		const float Left = (1.f + K) * FMath::Exp(-K);
		return (1.f + K * W) * FMath::Exp(-K * W) - Left * Ease(W);
	}

	/** The swing from -Anticipate to 1 + Overshoot of a spring let go toward 1 (its overshoot is (1 + Anticipate) R). */
	float SwingPast(float V, const FSnapParts& Parts)
	{
		if (Parts.Overshoot < SnapLeastOvershoot)
		{
			return -Parts.Anticipate + (1.f + Parts.Anticipate) * Ease(V);
		}
		const float R = Parts.Overshoot / (1.f + Parts.Anticipate);
		return -Parts.Anticipate + (1.f + Parts.Anticipate + Parts.Overshoot) * SpringHalf(V, R);
	}

	/** The old bump, symmetric round At over At +- HalfWidth (the reload's beats before the snap). */
	float EasedBump(float P, float At, float HalfWidth)
	{
		return FMath::SmoothStep(At - HalfWidth, At, P) * (1.f - FMath::SmoothStep(At, At + HalfWidth, P));
	}
}

float ReloadCurves::Snap(float T, const ReloadTuning::FSnapShape& Shape)
{
	if (T <= 0.f)
	{
		return 0.f;
	}
	if (T >= 1.f)
	{
		return 1.f;
	}
	const FSnapParts Parts = PartsOf(Shape);
	if (T < Parts.Lead)
	{
		return -Parts.Anticipate * Ease(T / Parts.Lead);
	}
	const float SwingEnd = 1.f - Parts.Settle;
	if (T < SwingEnd)
	{
		return SwingPast((T - Parts.Lead) / (SwingEnd - Parts.Lead), Parts);
	}
	return 1.f + Parts.Overshoot * Tail((T - SwingEnd) / FMath::Max(Parts.Settle, UE_SMALL_NUMBER));
}

float ReloadCurves::SnapInto(float T, const ReloadTuning::FSnapShape& Shape)
{
	if (T <= 0.f)
	{
		return 0.f;
	}
	if (T >= 1.f)
	{
		return 1.f;
	}
	const FSnapParts Parts = PartsOf(Shape);
	if (T < Parts.Lead)
	{
		return -Parts.Anticipate * Ease(T / Parts.Lead);
	}
	// Pushed with a steady hand from the wind-up's rest: speeding up all the way into the stop, which it hits at T 1.
	const float V = (T - Parts.Lead) / (1.f - Parts.Lead);
	return -Parts.Anticipate + (1.f + Parts.Anticipate) * V * V;
}

float ReloadCurves::BeatPeakAt(const ReloadTuning::FSnapShape& Shape)
{
	const FSnapParts Parts = PartsOf(Shape);
	return Parts.Lead + (1.f - Parts.Lead - Parts.Settle) * SnapKickShare;
}

float ReloadCurves::SnapBeat(float T, const ReloadTuning::FSnapShape& Shape)
{
	if (T <= 0.f || T >= 1.f)
	{
		return 0.f;
	}
	const FSnapParts Parts = PartsOf(Shape);
	if (T < Parts.Lead)
	{
		return -Parts.Anticipate * Ease(T / Parts.Lead);
	}
	const float Peak = BeatPeakAt(Shape);
	const bool bSprung = Parts.Overshoot >= SnapLeastOvershoot;
	if (T < Peak)
	{
		// The hit: kicked from the wind-up up to its peak, quick at first (the spring's own swing, arriving at rest).
		const float V = (T - Parts.Lead) / (Peak - Parts.Lead);
		const float Up = bSprung ? SpringHalf(V, Parts.Overshoot / (1.f + Parts.Anticipate)) : Ease(V);
		return -Parts.Anticipate + (1.f + Parts.Anticipate) * Up;
	}
	const float ReboundEnd = 1.f - Parts.Settle;
	if (T < ReboundEnd)
	{
		// Let go at the peak, the spring swings back past rest by Overshoot.
		const float V = (T - Peak) / (ReboundEnd - Peak);
		return 1.f - (1.f + Parts.Overshoot) * (bSprung ? SpringHalf(V, Parts.Overshoot) : Ease(V));
	}
	return -Parts.Overshoot * Tail((T - ReboundEnd) / FMath::Max(Parts.Settle, UE_SMALL_NUMBER));
}

float ReloadCurves::JoltSeconds(float Hz, float Damping)
{
	const float Zeta = FMath::Clamp(Damping, 0.05f, 0.95f);
	const float Omega = 2.f * UE_PI * FMath::Max(Hz, 0.01f);
	const float Decay = Zeta * Omega;
	const float Ring = Omega * FMath::Sqrt(1.f - Zeta * Zeta);
	const float PeakAt = FMath::Atan2(Ring, Decay) / Ring;
	const float Peak = FMath::Exp(-Decay * PeakAt) * FMath::Sin(Ring * PeakAt);
	// The swing's envelope over its peak falls under JoltRungOut here and never rises again.
	return FMath::Loge(1.f / (JoltRungOut * Peak)) / Decay;
}

float ReloadCurves::Jolt(float Seconds, float Hz, float Damping)
{
	if (Seconds <= 0.f || Seconds >= JoltSeconds(Hz, Damping))
	{
		return 0.f;
	}
	// A spring at rest given a knock of velocity (FWeaponRecoil::FSpring after AddShot): x = e^(-zeta w t) sin(w_d t), scaled
	// so its peak is 1.
	const float Zeta = FMath::Clamp(Damping, 0.05f, 0.95f);
	const float Omega = 2.f * UE_PI * FMath::Max(Hz, 0.01f);
	const float Decay = Zeta * Omega;
	const float Ring = Omega * FMath::Sqrt(1.f - Zeta * Zeta);
	const float PeakAt = FMath::Atan2(Ring, Decay) / Ring;
	const float Peak = FMath::Exp(-Decay * PeakAt) * FMath::Sin(Ring * PeakAt);
	return FMath::Exp(-Decay * Seconds) * FMath::Sin(Ring * Seconds) / Peak;
}

// ---------------------------------------------------------------------------
// Over a reload's progress
// ---------------------------------------------------------------------------

bool ReloadCurves::On()
{
	return ReloadTuning::SnapOn();
}

float ReloadCurves::Rise(float P, float A, float B)
{
	if (!On() || B <= A)
	{
		return FMath::SmoothStep(A, B, P);
	}
	return Snap((P - A) / (B - A), ReloadTuning::SnapShape());
}

float ReloadCurves::Window(float P, float A, float B, float C, float D)
{
	return Rise(P, A, B) * (1.f - Rise(P, C, D));
}

float ReloadCurves::Into(float P, float A, float B, bool bWindUp)
{
	if (!On() || B <= A)
	{
		return FMath::SmoothStep(A, B, P);
	}
	const ReloadTuning::FSnapShape Shape = ReloadTuning::SnapShape();
	return SnapInto((P - A) / (B - A), bWindUp ? Shape : Shape.NoWindUp());
}

float ReloadCurves::Beat(float P, float At, float HalfWidth)
{
	if (!On() || HalfWidth <= 0.f)
	{
		return EasedBump(P, At, HalfWidth);
	}
	// The same width as the old bump, moved so the hit is At: wound up before it, rung out after it.
	const ReloadTuning::FSnapShape Shape = ReloadTuning::SnapShape();
	const float Width = 2.f * HalfWidth;
	const float Start = At - PartsOf(Shape).Lead * Width;
	return SnapBeat((P - Start) / Width, Shape);
}

// ---------------------------------------------------------------------------
// The first-person gun's jolt
// ---------------------------------------------------------------------------

void LooterReload::JoltPose(EWeaponReloadPart Part, const FReloadPlan& Plan, float RawProgress, float MotionSeconds, float Snappiness,
	const ReloadTuning::FJoltRules& Rules, FVector& OutOffset, FRotator& OutRotation)
{
	OutOffset = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;
	if (RawProgress < 0.f || MotionSeconds <= 0.f || Rules.Scale <= 0.f || !ReloadCurves::On())
	{
		return;
	}
	// The gun recoil's own spring: its frequency times the gun's Snappiness (FWeaponRecoil::AddShot's, a heavy gun's slower).
	const float Hz = Rules.Hz * FMath::Max(Snappiness, 0.2f);
	const float RingsFor = ReloadCurves::JoltSeconds(Hz, Rules.Damping);
	TArray<FReloadJoltAt> Jolts;
	JoltsFor(Part, Plan, Jolts);
	float Knock = 0.f;
	for (const FReloadJoltAt& At : Jolts)
	{
		const float Since = (RawProgress - RawProgressOf(Part, Plan, At.Progress)) * MotionSeconds;
		if (Since > 0.f && Since < RingsFor)
		{
			Knock += (At.bSlam ? Rules.Slam : Rules.Seat) * ReloadCurves::Jolt(Since, Hz, Rules.Damping);
		}
	}
	Knock *= Rules.Scale;
	OutOffset = JoltWay * Knock;
	OutRotation = FRotator(-Rules.Pitch * Knock, 0.f, 0.f);
}
