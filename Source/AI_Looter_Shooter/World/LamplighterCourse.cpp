// FLamplighterCourse: the Lamplighter's course in, walked by distance (a Catmull-Rom curve through WORLD's points).

#include "World/LamplighterCourse.h"
#include "Algo/BinarySearch.h"

namespace
{
	/** A uniform Catmull-Rom point between P1 and P2 at T (0-1). */
	FVector CatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, double T)
	{
		const double T2 = T * T;
		const double T3 = T2 * T;
		return 0.5 * ((2.0 * P1) + (-P0 + P2) * T + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * T2 + (-P0 + 3.0 * P1 - 3.0 * P2 + P3) * T3);
	}

	/** How far either side the heading's turn and the climb are measured (cm): a hull's length, so a sample's kink never shows. */
	constexpr float MeasureSpan = 1200.f;

	/** How much of the turn's sideways pull goes into the bank: a little more than the physics, so a turn reads. */
	constexpr float BankGain = 1.6f;
	constexpr float Gravity = 980.f;
}

void FLamplighterCourse::Reset()
{
	Samples.Reset();
	Lengths.Reset();
	PointSamples.Reset();
}

bool FLamplighterCourse::Build(const TArray<FVector>& Points)
{
	Reset();
	TArray<FVector> Clean;
	Clean.Reserve(Points.Num());
	for (const FVector& Point : Points)
	{
		if (Clean.IsEmpty() || !Clean.Last().Equals(Point, 1.0))
		{
			Clean.Add(Point);
		}
	}
	if (Clean.Num() < 2)
	{
		return false;
	}
	const int32 Spans = Clean.Num() - 1;
	Samples.Reserve(Spans * SamplesPerSpan + 1);
	PointSamples.Reserve(Clean.Num());
	for (int32 Span = 0; Span < Spans; ++Span)
	{
		// The ends repeat themselves, so the curve starts and ends on its first and last points heading along them.
		const FVector& P0 = Clean[FMath::Max(Span - 1, 0)];
		const FVector& P1 = Clean[Span];
		const FVector& P2 = Clean[Span + 1];
		const FVector& P3 = Clean[FMath::Min(Span + 2, Clean.Num() - 1)];
		PointSamples.Add(Samples.Num());
		for (int32 Step = 0; Step < SamplesPerSpan; ++Step)
		{
			Samples.Add(CatmullRom(P0, P1, P2, P3, static_cast<double>(Step) / SamplesPerSpan));
		}
	}
	PointSamples.Add(Samples.Num());
	Samples.Add(Clean.Last());

	Lengths.Reserve(Samples.Num());
	Lengths.Add(0.f);
	for (int32 Index = 1; Index < Samples.Num(); ++Index)
	{
		Lengths.Add(Lengths.Last() + static_cast<float>(FVector::Distance(Samples[Index - 1], Samples[Index])));
	}
	return true;
}

float FLamplighterCourse::DistanceAtPoint(int32 PointIndex) const
{
	if (PointSamples.IsEmpty())
	{
		return 0.f;
	}
	return Lengths[PointSamples[FMath::Clamp(PointIndex, 0, PointSamples.Num() - 1)]];
}

void FLamplighterCourse::Locate(float Distance, int32& OutIndex, float& OutAlpha) const
{
	OutIndex = 0;
	OutAlpha = 0.f;
	if (Lengths.Num() < 2)
	{
		return;
	}
	const float Clamped = FMath::Clamp(Distance, 0.f, GetLength());
	// The first sample past it, by halves: a handful of steps on a table of a few hundred.
	int32 Upper = Algo::UpperBound(Lengths, Clamped);
	Upper = FMath::Clamp(Upper, 1, Lengths.Num() - 1);
	OutIndex = Upper - 1;
	const float Span = Lengths[Upper] - Lengths[OutIndex];
	OutAlpha = Span > KINDA_SMALL_NUMBER ? (Clamped - Lengths[OutIndex]) / Span : 0.f;
}

FVector FLamplighterCourse::LocationAt(float Distance) const
{
	if (Samples.IsEmpty())
	{
		return FVector::ZeroVector;
	}
	int32 Index = 0;
	float Alpha = 0.f;
	Locate(Distance, Index, Alpha);
	return FMath::Lerp(Samples[Index], Samples[FMath::Min(Index + 1, Samples.Num() - 1)], static_cast<double>(Alpha));
}

FVector FLamplighterCourse::HeadingAt(float Distance) const
{
	if (Samples.Num() < 2)
	{
		return FVector::ForwardVector;
	}
	const float Length = GetLength();
	const float Half = FMath::Min(MeasureSpan * 0.25f, Length * 0.5f);
	const float Back = FMath::Clamp(Distance - Half, 0.f, Length - 2.f * Half);
	FVector Along = LocationAt(Back + 2.f * Half) - LocationAt(Back);
	Along.Z = 0.0;
	return Along.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
}

float FLamplighterCourse::TurnRateAt(float Distance) const
{
	const float Length = GetLength();
	if (Length < 2.f * MeasureSpan)
	{
		return 0.f;
	}
	const float From = FMath::Clamp(Distance - MeasureSpan, 0.f, Length);
	const float To = FMath::Clamp(Distance + MeasureSpan, 0.f, Length);
	if (To - From < KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	const double YawFrom = HeadingAt(From).Rotation().Yaw;
	const double YawTo = HeadingAt(To).Rotation().Yaw;
	return static_cast<float>(FMath::DegreesToRadians(FMath::FindDeltaAngleDegrees(YawFrom, YawTo)) / (To - From));
}

float FLamplighterCourse::SlopeAt(float Distance) const
{
	const float Length = GetLength();
	const float From = FMath::Clamp(Distance - MeasureSpan * 0.5f, 0.f, Length);
	const float To = FMath::Clamp(Distance + MeasureSpan * 0.5f, 0.f, Length);
	if (To - From < KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	const FVector Rise = LocationAt(To) - LocationAt(From);
	const double Run = FVector(Rise.X, Rise.Y, 0.0).Size();
	return Run > KINDA_SMALL_NUMBER ? static_cast<float>(Rise.Z / Run) : 0.f;
}

void FLamplighterCourse::BobAt(float Clock, float Sway, float& OutHeave, float& OutRoll, float& OutPitch)
{
	const float Amount = FMath::Clamp(Sway, 0.f, 1.f);
	OutHeave = Amount * HeaveCm * FMath::Sin(UE_TWO_PI * HeaveHz * Clock);
	OutRoll = Amount * RollDegrees * FMath::Sin(UE_TWO_PI * RollHz * Clock + 1.3f);
	OutPitch = Amount * PitchDegrees * FMath::Sin(UE_TWO_PI * PitchHz * Clock + 0.4f);
}

FTransform FLamplighterCourse::CalmPoseAt(float Distance, const FVector& Scale) const
{
	return FTransform(HeadingAt(Distance).Rotation(), LocationAt(Distance), Scale);
}

FTransform FLamplighterCourse::PoseAt(float Distance, float Speed, float Clock, float Sway, const FVector& Scale) const
{
	float Heave = 0.f;
	float BobRoll = 0.f;
	float BobPitch = 0.f;
	BobAt(Clock, Sway, Heave, BobRoll, BobPitch);
	// Leaning into the turn by its sideways pull (speed² × turn rate against gravity), and nosing up a climb.
	const float Pull = Speed * Speed * TurnRateAt(Distance);
	const float Bank = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan(Pull / Gravity)) * BankGain, -MaxBankDegrees, MaxBankDegrees);
	const float Climb = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan(SlopeAt(Distance))) * 0.5f, -MaxPitchDegrees, MaxPitchDegrees);

	const FQuat Heading = HeadingAt(Distance).ToOrientationQuat();
	// Rocked in her own frame: rolling about her length (local X; a turn toward +Y dips her +Y side, the negative turn about
	// X), pitching about her beam (local Y; a turn about +Y dips the nose, so nose up is the negative one).
	const FQuat Rock = FQuat(FVector::XAxisVector, FMath::DegreesToRadians(-Bank + BobRoll))
		* FQuat(FVector::YAxisVector, FMath::DegreesToRadians(-(Climb + BobPitch)));
	return FTransform(Heading * Rock, LocationAt(Distance) + FVector(0.0, 0.0, Heave), Scale);
}

float FLamplighterCourse::TargetSpeed(float Distance, float StopDistance) const
{
	const float Hold = HoldDistance(StopDistance);
	if (Distance >= Hold - 1.f)
	{
		return 0.f;
	}
	// Braking so she comes down to the creep right at the stop: the speed from which this much braking ends at the creep
	// there (v² = creep² + 2·a·d), never more than the cruise.
	const float ToStop = FMath::Max(StopDistance - Distance, 0.f);
	const float Braking = FMath::Sqrt(CreepSpeed * CreepSpeed + 2.f * Acceleration * ToStop);
	// Past the stop she creeps, and eases to a hold at its end the same way.
	const float ToHold = FMath::Max(Hold - Distance, 0.f);
	const float Holding = FMath::Sqrt(2.f * Acceleration * ToHold);
	return FMath::Min3(CruiseSpeed, Braking, Holding);
}

float FLamplighterCourse::SpeedToward(float Speed, float Target, float DeltaSeconds)
{
	const float Step = Acceleration * FMath::Max(DeltaSeconds, 0.f);
	return Speed < Target ? FMath::Min(Speed + Step, Target) : FMath::Max(Speed - Step, Target);
}
