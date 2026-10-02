#include "Creatures/CreatureUpdateRate.h"

namespace
{
	/** How much farther off the middle a 16:9 screen's corners are than its side edges, in tangents: sqrt(1 + (9/16)^2). */
	constexpr float ScreenCornerStretch = 1.1474f;

	/** Fields of view are kept within these (degrees), so their tangents stay finite. */
	constexpr float NarrowestViewDegrees = 1.f;
	constexpr float WidestViewDegrees = 170.f;

	/** The tangent of half a field of view (degrees): how wide the view is at a distance of one. */
	float HalfViewTangent(float FieldOfView)
	{
		return FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(FieldOfView, NarrowestViewDegrees, WidestViewDegrees) * 0.5f));
	}
}

float FCreatureUpdateRate::IntervalFor(float Distance, bool bEngaged, bool bOnScreen) const
{
	if (!bEnabled || bEngaged || Distance < NearDistance || (bOnScreen && Distance < OnScreenFullRateDistance))
	{
		return 0.f;
	}
	// Clamped so a mistyped interval can't outrun what the bodies' animation takes in one step.
	const float Interval = Distance < MidDistance ? MidInterval : (Distance < FarDistance ? FarInterval : VeryFarInterval);
	return FMath::Clamp(Interval, 0.f, MaxInterval);
}

bool FCreatureUpdateRate::ShouldFreezePose(float Distance, bool bEngaged, bool bOnScreen) const
{
	return bEnabled && !bEngaged && !bOnScreen && Distance >= FreezePoseDistance;
}

float FCreatureUpdateRate::ZoomFor(float FieldOfView) const
{
	// Tangents, as a sight narrows the view (UPlayerViewComponent): through 2x, the view is half as wide.
	return FMath::Max(1.f, HalfViewTangent(ReferenceFieldOfView) / HalfViewTangent(FieldOfView));
}

bool FCreatureUpdateRate::IsInView(const FVector& ViewLocation, const FVector& ViewDirection, float FieldOfView, const FVector& Point,
	float Radius, float MarginDegrees)
{
	const FVector ToPoint = Point - ViewLocation;
	const float Distance = static_cast<float>(ToPoint.Size());
	const FVector Forward = ViewDirection.GetSafeNormal();
	// A camera inside the body sees it whichever way it looks (and one looking nowhere can't rule it out).
	if (Distance <= Radius || Forward.IsZero())
	{
		return true;
	}
	// The cone through the screen's corners, widened by the body's own size and the margin.
	const float Corners = FMath::Atan(HalfViewTangent(FieldOfView) * ScreenCornerStretch);
	const float BodyAngle = FMath::Asin(FMath::Clamp(Radius / Distance, 0.f, 1.f));
	const float HalfAngle = Corners + BodyAngle + FMath::DegreesToRadians(FMath::Max(MarginDegrees, 0.f));
	if (HalfAngle >= UE_PI)
	{
		return true;
	}
	return static_cast<float>(FVector::DotProduct(ToPoint / Distance, Forward)) >= FMath::Cos(HalfAngle);
}
