#include "Scenes/ColdOpenCourse.h"

namespace
{
	/** Samples per segment of the curve for its length table: a centimeter or two off over 100 m. */
	constexpr int32 SamplesPerSegment = 48;

	/** How far ahead and behind the course looks to see it heading, and turning (cm). */
	constexpr float HeadingProbe = 60.f;
	constexpr float TurnProbe = 400.f;

	/** An airship sways into a turn far less than an aircraft banks: this share of a banked turn's lean. */
	constexpr float BankShare = 0.5f;

	/** The gas-bag's slow heave (cm, Hz), as the packet skiff's (SkiffRide). */
	constexpr float HeaveCm = 12.f;
	constexpr float HeaveHz = 0.21f;

	constexpr float Gravity = 980.f;

	/** Easing from 0 to 1 (a smooth step). */
	float Ease(float U)
	{
		U = FMath::Clamp(U, 0.f, 1.f);
		return U * U * (3.f - 2.f * U);
	}

	/** The smooth step's area from 0 to U: how far a speed easing from 0 to 1 over a span carries, in spans (1/2 at 1). */
	float EasedArea(float U)
	{
		U = FMath::Clamp(U, 0.f, 1.f);
		return U * U * U - 0.5f * U * U * U * U;
	}
}

bool FColdOpenCourse::Build(const TArray<FVector>& InPoints, float Seconds)
{
	Points = InPoints;
	Table.Reset();
	Duration = FMath::Max(Seconds, 1.f);
	if (Points.Num() < 2)
	{
		return false;
	}

	// The curve's length, sample by sample, so a distance along it finds its point.
	FVector Last = Points[0];
	float Along = 0.f;
	Table.Add({ Last, 0.f });
	for (int32 Segment = 0; Segment + 1 < Points.Num(); ++Segment)
	{
		for (int32 Step = 1; Step <= SamplesPerSegment; ++Step)
		{
			const FVector Here = Evaluate(Segment, static_cast<float>(Step) / SamplesPerSegment);
			Along += FVector::Dist(Here, Last);
			Last = Here;
			Table.Add({ Here, Along });
		}
	}

	// The phases fit inside a short course; a long one holds, ramps and slows for their full time.
	Hold = FMath::Min(HoldSeconds, Duration * 0.25f);
	Ramp = FMath::Min(RampSeconds, Duration * 0.25f);
	Slow = FMath::Min(SlowSeconds, Duration * 0.3f);

	// The distance covered is linear in the cruise: the held drift and the ends' easing carry a fixed part, the cruise
	// the rest, so the cruise that covers the course in its time comes straight out.
	const float Fixed = StartSpeed * (Hold + 0.5f * Ramp) + EndSpeed * 0.5f * Slow;
	const float PerCruise = Duration - Hold - 0.5f * Ramp - 0.5f * Slow;
	Cruise = FMath::Max((GetLength() - Fixed) / FMath::Max(PerCruise, UE_KINDA_SMALL_NUMBER), 1.f);
	return true;
}

float FColdOpenCourse::GetLength() const
{
	return Table.IsEmpty() ? 0.f : Table.Last().Distance;
}

FVector FColdOpenCourse::Evaluate(int32 Segment, float Alpha) const
{
	// A uniform Catmull-Rom curve through every point, the ends' neighbours standing in for the missing ones.
	const int32 Last = Points.Num() - 1;
	const FVector& P0 = Points[FMath::Clamp(Segment - 1, 0, Last)];
	const FVector& P1 = Points[FMath::Clamp(Segment, 0, Last)];
	const FVector& P2 = Points[FMath::Clamp(Segment + 1, 0, Last)];
	const FVector& P3 = Points[FMath::Clamp(Segment + 2, 0, Last)];
	const float T = Alpha;
	const float T2 = T * T;
	const float T3 = T2 * T;
	return 0.5f * ((2.f * P1) + (P2 - P0) * T + (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2 + (3.f * P1 - P0 - 3.f * P2 + P3) * T3);
}

float FColdOpenCourse::DistanceAt(float Seconds) const
{
	if (!IsBuilt())
	{
		return 0.f;
	}
	const float T = FMath::Clamp(Seconds, 0.f, Duration);
	const float SlowStart = Duration - Slow;
	const float Cruising = StartSpeed * (Hold + Ramp) + (Cruise - StartSpeed) * Ramp * 0.5f;
	float Covered = 0.f;
	if (T <= Hold)
	{
		Covered = StartSpeed * T;
	}
	else if (T <= Hold + Ramp)
	{
		Covered = StartSpeed * T + (Cruise - StartSpeed) * Ramp * EasedArea((T - Hold) / Ramp);
	}
	else if (T <= SlowStart)
	{
		Covered = Cruising + Cruise * (T - Hold - Ramp);
	}
	else
	{
		const float AtSlow = Cruising + Cruise * (SlowStart - Hold - Ramp);
		Covered = AtSlow + Cruise * (T - SlowStart) + (EndSpeed - Cruise) * Slow * EasedArea((T - SlowStart) / Slow);
	}
	// Never past the end, should a short course have had its cruise held up at the least speed.
	return FMath::Min(Covered, GetLength());
}

float FColdOpenCourse::SpeedAt(float Seconds) const
{
	const float T = FMath::Clamp(Seconds, 0.f, Duration);
	if (T <= Hold)
	{
		return StartSpeed;
	}
	if (T <= Hold + Ramp)
	{
		return StartSpeed + (Cruise - StartSpeed) * Ease((T - Hold) / Ramp);
	}
	const float SlowStart = Duration - Slow;
	if (T <= SlowStart)
	{
		return Cruise;
	}
	return Cruise + (EndSpeed - Cruise) * Ease((T - SlowStart) / Slow);
}

FVector FColdOpenCourse::PointAt(float Distance) const
{
	if (!IsBuilt())
	{
		return Points.IsEmpty() ? FVector::ZeroVector : Points[0];
	}
	const float Along = FMath::Clamp(Distance, 0.f, GetLength());
	// The samples either side of it (a binary search: they're in order), and straight between them.
	int32 Low = 0;
	int32 High = Table.Num() - 1;
	while (High - Low > 1)
	{
		const int32 Middle = (Low + High) / 2;
		if (Table[Middle].Distance < Along)
		{
			Low = Middle;
		}
		else
		{
			High = Middle;
		}
	}
	const float Span = Table[High].Distance - Table[Low].Distance;
	const float Alpha = Span > UE_KINDA_SMALL_NUMBER ? (Along - Table[Low].Distance) / Span : 0.f;
	return FMath::Lerp(Table[Low].Location, Table[High].Location, Alpha);
}

FVector FColdOpenCourse::HeadingAt(float Distance) const
{
	const float Along = FMath::Clamp(Distance, 0.f, GetLength());
	const FVector Ahead = PointAt(FMath::Min(Along + HeadingProbe, GetLength()));
	const FVector Behind = PointAt(FMath::Max(Along - HeadingProbe, 0.f));
	return (Ahead - Behind).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
}

FTransform FColdOpenCourse::PoseAt(float Seconds) const
{
	const float Along = DistanceAt(Seconds);
	const FVector Heading = HeadingAt(Along);
	const double Yaw = Heading.Rotation().Yaw;

	// The nose lifts a little on a climb and dips on a descent: an airship stays nearly level.
	const double Climb = FMath::RadiansToDegrees(FMath::Atan2(Heading.Z, FVector2D(Heading.X, Heading.Y).Size()));
	const double Pitch = FMath::Clamp(Climb * 0.5, -static_cast<double>(MaxPitchDegrees), static_cast<double>(MaxPitchDegrees));

	// Leaning into a turn: how fast the heading swings round, as a sideways pull against gravity. A turn to the right
	// (yaw growing) rolls the right side down, which is a positive roll.
	const double Before = HeadingAt(Along - TurnProbe).Rotation().Yaw;
	const double After = HeadingAt(Along + TurnProbe).Rotation().Yaw;
	const double TurnPerCm = FMath::DegreesToRadians(FRotator::NormalizeAxis(After - Before)) / (2.0 * TurnProbe);
	const double Speed = SpeedAt(Seconds);
	const double Pull = Speed * Speed * TurnPerCm;
	const double Bank = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Pull, static_cast<double>(Gravity))) * BankShare,
		-static_cast<double>(MaxBankDegrees), static_cast<double>(MaxBankDegrees));

	const double Heave = HeaveCm * FMath::Sin(UE_TWO_PI * HeaveHz * Seconds);
	return FTransform(FRotator(Pitch, Yaw, Bank), PointAt(Along) + FVector(0.0, 0.0, Heave));
}
