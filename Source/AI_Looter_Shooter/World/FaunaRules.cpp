#include "World/FaunaRules.h"

namespace
{
	/** Gravity for banking (cm/s/s). */
	constexpr float Gravity = 980.f;

	/** How much height counts in a threat's distance: a bird above the player lets them a little nearer. */
	constexpr float ThreatHeightWeight = 0.6f;

	/** The fear of a player standing still, and of one sprinting; crouched, a share of either. */
	constexpr float StillFear = 0.8f;
	constexpr float SprintFear = 1.35f;
	constexpr float CrouchedFear = 0.6f;
	constexpr float CreatureFear = 0.7f;

	/** Off screen, how often an actor still updates (s): busy, and at rest. */
	constexpr float OffScreenBusyInterval = 0.1f;
	constexpr float OffScreenRestInterval = 0.25f;

	/** On screen past the full-rate distances: 30 and 15 updates a second. */
	constexpr float MidInterval = 1.f / 30.f;
	constexpr float FarInterval = 1.f / 15.f;

	/** A small integer hash (lowbias32): every bit of the input reaches every bit of the output. */
	uint32 Mix(uint32 X)
	{
		X ^= X >> 16;
		X *= 0x7feb352du;
		X ^= X >> 15;
		X *= 0x846ca68bu;
		X ^= X >> 16;
		return X;
	}
}

float FaunaRules::FearOf(bool bPlayer, bool bCrouched, float Speed)
{
	if (!bPlayer)
	{
		return CreatureFear;
	}
	float Fear = 1.f;
	if (Speed < WalkSpeed)
	{
		Fear = FMath::Lerp(StillFear, 1.f, FMath::Clamp(Speed / WalkSpeed, 0.f, 1.f));
	}
	else
	{
		Fear = FMath::Lerp(1.f, SprintFear, FMath::Clamp((Speed - WalkSpeed) / (SprintSpeed - WalkSpeed), 0.f, 1.f));
	}
	return bCrouched ? Fear * CrouchedFear : Fear;
}

bool FaunaRules::IsThreatened(const FVector& Where, const FFaunaThreat& Threat, float FleeRadius)
{
	const FVector Offset = Where - Threat.Location;
	const double Reach = static_cast<double>(FleeRadius) * FMath::Max(Threat.Fear, 0.f);
	const double Weighted = FMath::Square(Offset.X) + FMath::Square(Offset.Y) + FMath::Square(Offset.Z * ThreatHeightWeight);
	return Weighted < FMath::Square(Reach);
}

bool FaunaRules::IsThreatenedByAny(const FVector& Where, TConstArrayView<FFaunaThreat> Threats, float FleeRadius)
{
	for (const FFaunaThreat& Threat : Threats)
	{
		if (IsThreatened(Where, Threat, FleeRadius))
		{
			return true;
		}
	}
	return false;
}

int32 FaunaRules::NearestThreat(const FVector& Where, TConstArrayView<FFaunaThreat> Threats, float& OutDistance)
{
	int32 Nearest = INDEX_NONE;
	OutDistance = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Threats.Num(); ++Index)
	{
		const float Distance = static_cast<float>(FVector::Dist(Where, Threats[Index].Location));
		if (Distance < OutDistance)
		{
			OutDistance = Distance;
			Nearest = Index;
		}
	}
	return Nearest;
}

bool FaunaRules::Hears(const FVector& Where, const FFaunaNoise& Noise, double Since, float GunfireRadius, float ImpactRadius)
{
	if (Noise.Time <= Since)
	{
		return false;
	}
	float Reach = Noise.Radius;
	switch (Noise.Kind)
	{
	case EFaunaNoise::Gunshot:
		Reach = GunfireRadius;
		break;
	case EFaunaNoise::Impact:
		Reach = ImpactRadius;
		break;
	default:
		break;
	}
	return FVector::DistSquared(Where, Noise.Location) <= FMath::Square(static_cast<double>(Reach));
}

int32 FaunaRules::FirstHeard(const FVector& Where, TConstArrayView<FFaunaNoise> Noises, double Since, float GunfireRadius, float ImpactRadius)
{
	for (int32 Index = 0; Index < Noises.Num(); ++Index)
	{
		if (Hears(Where, Noises[Index], Since, GunfireRadius, ImpactRadius))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

float FaunaRules::UpdateInterval(float Distance, bool bOnScreen, bool bBusy)
{
	if (!bOnScreen)
	{
		return bBusy ? OffScreenBusyInterval : OffScreenRestInterval;
	}
	if (Distance < FullRateDistance || (bBusy && Distance < BusyFullRateDistance))
	{
		return 0.f;
	}
	// A busy one far off, or one at rest in the middle distance, moves little on screen between two updates.
	return (bBusy || Distance < BusyFullRateDistance) ? MidInterval : FarInterval;
}

bool FaunaRules::ShouldShow(float Distance, float CullDistance, bool bShownNow)
{
	if (CullDistance <= 0.f)
	{
		return true;
	}
	return Distance <= (bShownNow ? CullDistance * CullHysteresis : CullDistance);
}

int32 FaunaRules::SwarmRoom(int32 Cap, int32 Active, int32 Wanted)
{
	return FMath::Max(0, FMath::Min(Wanted, Cap - Active));
}

float FaunaRules::Random01(uint32 Seed, uint32 Index)
{
	const uint32 Hashed = Mix(Seed * 0x9E3779B9u + Mix(Index + 0x632BE5ABu));
	// 24 bits: exactly representable in a float, so the result is always under 1.
	return static_cast<float>(Hashed >> 8) / 16777216.f;
}

float FaunaRules::RandomRange(uint32 Seed, uint32 Index, float Min, float Max)
{
	return Min + (Max - Min) * Random01(Seed, Index);
}

FQuat FaunaRules::WingTurn(int32 Side, float UpDegrees, float BackDegrees)
{
	// About the bird's forward axis a positive angle lowers the left wing (-Y) and raises the right (+Y), and about its up
	// axis it swings the left wing forward and the right one back: the side's sign makes both mean the same on each wing.
	const float Sign = Side < 0 ? -1.f : 1.f;
	const FQuat Flap(FVector::ForwardVector, FMath::DegreesToRadians(UpDegrees) * Sign);
	const FQuat Sweep(FVector::UpVector, FMath::DegreesToRadians(BackDegrees) * Sign);
	// The sweep first, in the wing's own frame, then the beat about the shoulder.
	return Flap * Sweep;
}

FVector FaunaRules::Hermite(const FVector& P0, const FVector& V0, const FVector& P1, const FVector& V1, float Duration, float Alpha,
	FVector* OutVelocity)
{
	const double T = FMath::Clamp(static_cast<double>(Alpha), 0.0, 1.0);
	const double D = FMath::Max(static_cast<double>(Duration), 1e-3);
	const double T2 = T * T;
	const double T3 = T2 * T;
	const FVector Point = P0 * (2.0 * T3 - 3.0 * T2 + 1.0) + V0 * (D * (T3 - 2.0 * T2 + T)) + P1 * (3.0 * T2 - 2.0 * T3)
		+ V1 * (D * (T3 - T2));
	if (OutVelocity)
	{
		// The curve's slope per unit of Alpha, over the seconds one unit takes.
		*OutVelocity = (P0 * (6.0 * T2 - 6.0 * T) + V0 * (D * (3.0 * T2 - 4.0 * T + 1.0)) + P1 * (6.0 * T - 6.0 * T2)
			+ V1 * (D * (3.0 * T2 - 2.0 * T))) / D;
	}
	return Point;
}

float FaunaRules::FlightSeconds(float Distance, float Speed, float Min, float Max)
{
	return FMath::Clamp(Distance / FMath::Max(Speed, 1.f), Min, FMath::Max(Min, Max));
}

FVector FaunaRules::Bounce(const FVector& Velocity, const FVector& Normal, float Restitution)
{
	const FVector Unit = Normal.GetSafeNormal();
	const double Into = FVector::DotProduct(Velocity, Unit);
	if (Into >= 0.0 || Unit.IsZero())
	{
		return Velocity;
	}
	return Velocity - Unit * (Into * (1.0 + FMath::Clamp(static_cast<double>(Restitution), 0.0, 1.0)));
}

float FaunaRules::BankDegrees(float Speed, float TurnRate, float MaxDegrees)
{
	// A coordinated turn: the lift tilted so its sideways share holds the bird on its curve.
	const float Degrees = FMath::RadiansToDegrees(FMath::Atan(Speed * TurnRate / Gravity));
	return FMath::Clamp(Degrees, -MaxDegrees, MaxDegrees);
}

FVector FaunaRules::GroundNear(const FFaunaPerch& Perch, const FVector2D& Offset)
{
	const FVector Up = Perch.Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	// On the plane through the perch with the ground's normal: steep normals are kept from throwing it far up or down.
	const double Rise = -(Up.X * Offset.X + Up.Y * Offset.Y) / FMath::Max(Up.Z, 0.3);
	return Perch.Location + FVector(Offset.X, Offset.Y, Rise);
}

float FaunaRules::StepAngle(float From, float To, float MaxStep)
{
	const float Delta = FMath::FindDeltaAngleDegrees(From, To);
	return From + FMath::Clamp(Delta, -FMath::Abs(MaxStep), FMath::Abs(MaxStep));
}
