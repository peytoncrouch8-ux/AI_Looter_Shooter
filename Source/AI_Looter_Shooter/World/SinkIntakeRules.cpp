// SinkIntakeRules: the intake's draw and its hose's shapes, as plain math.

#include "World/SinkIntakeRules.h"

namespace
{
	/** The swing: kicked out this far (degrees), a swing there and back this long (s), dying away this fast (s). */
	constexpr float SwingKick = 72.f;
	constexpr float SwingPeriod = 2.3f;
	constexpr float SwingDecay = 1.7f;
	/** The pendulum's pivot: this far down the hose from its top. */
	constexpr float PivotAlong = 0.48f;
	/** The swing settles into the limp shape over its last share. */
	constexpr float SettleShare = 0.35f;
	/** The buck thrashes this far (cm) at the hose's end, this fast. */
	constexpr float BuckWidth = 160.f;
	constexpr float BuckSpeed = 19.f;

	float Smooth(float A)
	{
		const float T = FMath::Clamp(A, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

FVector FIntakeHoseShape::At(float T) const
{
	const float A = FMath::Clamp(T, 0.f, 1.f);
	const float U = 1.f - A;
	return P0 * (U * U * U) + P1 * (3.f * U * U * A) + P2 * (3.f * U * A * A) + P3 * (A * A * A);
}

float SinkIntakeRules::DrawInput(float Distance, float DrawRadius, float DrainRadius, float Scale)
{
	if (Distance >= DrawRadius || DrawRadius <= 0.f)
	{
		return 0.f;
	}
	const float Inner = FMath::Clamp(DrainRadius, 0.f, DrawRadius - 1.f);
	const float Toward = Distance <= Inner ? 1.f : 1.f - (Distance - Inner) / FMath::Max(DrawRadius - Inner, 1.f);
	return FMath::Min(FMath::Lerp(EdgeDraw, MouthDraw, Toward) * FMath::Max(Scale, 0.f), MostDraw);
}

FIntakeHoseShape SinkIntakeRules::RestShape(const FVector& Top, const FVector& C1, const FVector& C2, const FVector& Bottom)
{
	FIntakeHoseShape Shape;
	Shape.P0 = Top;
	Shape.P1 = C1;
	Shape.P2 = C2;
	Shape.P3 = Bottom;
	return Shape;
}

FIntakeHoseShape SinkIntakeRules::LimpShape(const FVector& Top, const FVector& Rest)
{
	// Straight down from the drum most of the way, then bending over onto the floor and lying along it to its end.
	FIntakeHoseShape Shape;
	const double Drop = FMath::Max(Top.Z - Rest.Z, 100.0);
	const FVector Along = (FVector(Rest.X, Rest.Y, 0.0) - FVector(Top.X, Top.Y, 0.0)).GetSafeNormal();
	Shape.P0 = Top;
	Shape.P1 = Top - FVector(0.0, 0.0, Drop * 0.75);
	Shape.P2 = FVector(Top.X, Top.Y, Rest.Z) + Along * 120.0 + FVector(0.0, 0.0, 40.0);
	Shape.P3 = Rest;
	return Shape;
}

FIntakeHoseShape SinkIntakeRules::BuckShape(const FIntakeHoseShape& Rest, const FVector& Side, float Time, float BuckSeconds)
{
	// Its lower half whips side to side, hardest at the end, dying as the buck ends.
	FIntakeHoseShape Shape = Rest;
	const float Fade = 1.f - FMath::Clamp(Time / FMath::Max(BuckSeconds, 0.05f), 0.f, 1.f);
	Shape.P2 += Side * (BuckWidth * 0.5f * Fade * FMath::Sin(Time * BuckSpeed));
	Shape.P3 += Side * (BuckWidth * 0.35f * Fade * FMath::Sin(Time * BuckSpeed * 1.3f + 1.f))
		+ FVector(0.0, 0.0, 60.0 * Fade * FMath::Abs(FMath::Sin(Time * BuckSpeed * 0.5f)));
	return Shape;
}

float SinkIntakeRules::SwingAngle(float Time)
{
	const float T = FMath::Max(Time, 0.f);
	return SwingKick * FMath::Exp(-T / SwingDecay) * FMath::Sin(T * static_cast<float>(UE_TWO_PI) / SwingPeriod);
}

FIntakeHoseShape SinkIntakeRules::SwingShape(const FIntakeHoseShape& Rest, const FIntakeHoseShape& Limp, const FVector& Side,
	float FloorZ, float Time, float Seconds)
{
	// The end swings about a pivot halfway down the hose, in the plane of the hanging line and Side.
	const FVector Pivot = Rest.At(PivotAlong);
	const FVector Hang = Rest.P3 - Pivot;
	const double Reach = FMath::Max(Hang.Size(), 100.0);
	const FVector Across = Side.GetSafeNormal2D();
	const float Angle = FMath::DegreesToRadians(SwingAngle(Time));
	FVector End = Pivot + (FVector(0.0, 0.0, -1.0) * FMath::Cos(Angle) + Across * FMath::Sin(Angle)) * Reach;
	End.Z = FMath::Max(End.Z, static_cast<double>(FloorZ) + 40.0);
	// Settling into the limp shape over the swing's last share.
	const float Settle = Smooth((Time / FMath::Max(Seconds, 0.1f) - (1.f - SettleShare)) / SettleShare);
	FIntakeHoseShape Shape;
	Shape.P0 = Rest.P0;
	Shape.P1 = FMath::Lerp(Rest.P1 + (End - Rest.P3) * 0.2, Limp.P1, Settle);
	Shape.P2 = FMath::Lerp(Pivot + (End - Pivot) * 0.55, Limp.P2, Settle);
	Shape.P3 = FMath::Lerp(End, Limp.P3, Settle);
	return Shape;
}

FIntakeHoseShape SinkIntakeRules::SecondShape(const FVector& From, const FVector& To)
{
	// A long run sagging under its own weight, deepest a little past its middle, rising to the shaft head's cowl.
	const FVector Span = To - From;
	const double Flat = FVector(Span.X, Span.Y, 0.0).Size();
	FIntakeHoseShape Shape;
	Shape.P0 = From;
	Shape.P1 = From + Span * 0.3 - FVector(0.0, 0.0, Flat * 0.12);
	Shape.P2 = From + Span * 0.72 - FVector(0.0, 0.0, Flat * 0.08);
	Shape.P3 = To;
	return Shape;
}
