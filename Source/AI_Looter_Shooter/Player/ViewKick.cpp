#include "Player/ViewKick.h"

namespace
{
	/** Every spring is at least this quick and this damped, so a bad number can't make one ring on and on. */
	constexpr float MinFrequency = 1.f;
	constexpr float MinDamping = 0.2f;
	constexpr float MaxDamping = 0.95f;

	float ClampedDamping(float Damping)
	{
		return FMath::Clamp(Damping, MinDamping, MaxDamping);
	}

	/** Seconds after which a spring's swing stays under SettledShare of its peak. */
	float SettleSeconds(float Frequency, float Damping)
	{
		const float Decay = ClampedDamping(Damping) * 2.f * UE_PI * FMath::Max(Frequency, MinFrequency);
		return -FMath::Loge(FViewKickStack::SettledShare) / Decay;
	}
}

FViewKick FViewKick::operator*(float Scale) const
{
	FViewKick Scaled = *this;
	Scaled.Pitch *= Scale;
	Scaled.Yaw *= Scale;
	Scaled.Roll *= Scale;
	Scaled.FieldOfView *= Scale;
	return Scaled;
}

float FViewKickStack::Response(float Time, float Frequency, float Damping)
{
	if (Time <= 0.f)
	{
		return 0.f;
	}
	// x(t) = e^(-z w t) sin(wd t), the motion of a spring at rest given a push, divided by its first peak (at
	// t* = atan(wd / (z w)) / wd), so a kick's numbers are what the view reaches.
	const float Zeta = ClampedDamping(Damping);
	const float Omega = 2.f * UE_PI * FMath::Max(Frequency, MinFrequency);
	const float Decay = Zeta * Omega;
	const float Ringing = Omega * FMath::Sqrt(1.f - Zeta * Zeta);
	const float PeakTime = FMath::Atan2(Ringing, Decay) / Ringing;
	const float Peak = FMath::Exp(-Decay * PeakTime) * FMath::Sin(Ringing * PeakTime);
	return FMath::Exp(-Decay * Time) * FMath::Sin(Ringing * Time) / Peak;
}

void FViewKickStack::Add(const FViewKick& Kick)
{
	if (Active.Num() >= MaxKicks)
	{
		// The oldest has nearly settled anyway.
		Active.RemoveAt(0, 1, EAllowShrinking::No);
	}
	Active.Add({ Kick, 0.f });
}

void FViewKickStack::Tick(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	for (int32 Index = Active.Num() - 1; Index >= 0; --Index)
	{
		FActive& Each = Active[Index];
		Each.Age += Dt;
		if (Each.Age >= SettleSeconds(Each.Kick.Frequency, Each.Kick.Damping))
		{
			Active.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}
}

FRotator FViewKickStack::GetRotation() const
{
	FRotator Sum = FRotator::ZeroRotator;
	for (const FActive& Each : Active)
	{
		const float Swing = Response(Each.Age, Each.Kick.Frequency, Each.Kick.Damping);
		Sum += FRotator(Each.Kick.Pitch, Each.Kick.Yaw, Each.Kick.Roll) * Swing;
	}
	return FRotator(FMath::Clamp(Sum.Pitch, -MaxTurn, MaxTurn), FMath::Clamp(Sum.Yaw, -MaxTurn * 0.67f, MaxTurn * 0.67f),
		FMath::Clamp(Sum.Roll, -MaxTurn, MaxTurn));
}

float FViewKickStack::GetFieldOfView() const
{
	float Sum = 0.f;
	for (const FActive& Each : Active)
	{
		Sum += Each.Kick.FieldOfView * Response(Each.Age, Each.Kick.Frequency, Each.Kick.Damping);
	}
	return FMath::Clamp(Sum, -MaxFieldOfView, MaxFieldOfView);
}

FViewKick ViewKicks::ForShot(EWeaponKind Kind, float RecoilStat, float AimAlpha, FRandomStream& Random)
{
	FViewKick Kick;
	const float Side = Random.FRandRange(-1.f, 1.f);
	const float Lean = Random.RandRange(0, 1) == 0 ? -1.f : 1.f;
	if (Kind == EWeaponKind::Shotgun)
	{
		// A heavy shove: slower, bigger, wider, so the blast is felt in the shoulders.
		Kick.Pitch = 1.15f;
		Kick.Roll = 0.55f * Lean;
		Kick.Yaw = 0.15f * Side;
		Kick.FieldOfView = 0.022f;
		Kick.Frequency = 6.5f;
		Kick.Damping = 0.5f;
	}
	else if (Kind == EWeaponKind::Revolver)
	{
		// A sharp snap up and straight back down: nearly the shotgun's lift, but quick and well damped, so the sights are
		// back on target before the next pull of the trigger.
		Kick.Pitch = 0.9f;
		Kick.Roll = 0.3f * Lean;
		Kick.Yaw = 0.08f * Side;
		Kick.FieldOfView = 0.012f;
		Kick.Frequency = 11.f;
		Kick.Damping = 0.65f;
	}
	else
	{
		// A crisp tick per round: quick enough that a full-auto burst reads as a rattle, not a sway.
		Kick.Pitch = 0.32f;
		Kick.Roll = 0.22f * Lean;
		Kick.Yaw = 0.06f * Side;
		Kick.FieldOfView = 0.005f;
		Kick.Frequency = 14.f;
		Kick.Damping = 0.6f;
	}
	const float Strength = FMath::Clamp(RecoilStat, 0.6f, 1.5f);
	const float Aim = FMath::Clamp(AimAlpha, 0.f, 1.f);
	FViewKick Scaled = Kick * Strength;
	// Through the sight: half the turn and less of the widening, so the reticle stays readable on target.
	const float TurnScale = FMath::Lerp(1.f, 0.5f, Aim);
	Scaled.Pitch *= TurnScale;
	Scaled.Yaw *= TurnScale;
	Scaled.Roll *= TurnScale;
	Scaled.FieldOfView *= FMath::Lerp(1.f, 0.6f, Aim);
	return Scaled;
}

FViewKick ViewKicks::ForHurt(float DamageShare, float SourceSide, float SourceAhead)
{
	// A quarter of the bar or more in one hit is the full jolt; anything that hurts at all gets a fifth of it.
	const float Strength = FMath::Clamp(DamageShare / 0.25f, 0.2f, 1.f);
	const float Side = FMath::Clamp(SourceSide, -1.f, 1.f);
	const float Ahead = FMath::Clamp(SourceAhead, -1.f, 1.f);
	FViewKick Kick;
	// Knocked away from the hit: a blow from the right tips the head left (negative roll), one ahead tips it back.
	Kick.Roll = -Side * 2.5f * Strength;
	Kick.Yaw = -Side * 0.6f * Strength;
	Kick.Pitch = Ahead * 1.6f * Strength;
	Kick.FieldOfView = -0.015f * Strength;
	Kick.Frequency = 8.f;
	Kick.Damping = 0.45f;
	return Kick;
}

FViewKick ViewKicks::ForKill(bool bHeavy)
{
	FViewKick Kick;
	Kick.Pitch = 0.25f;
	Kick.FieldOfView = -0.016f;
	Kick.Frequency = 9.f;
	Kick.Damping = 0.5f;
	return bHeavy ? Kick * 1.4f : Kick;
}
