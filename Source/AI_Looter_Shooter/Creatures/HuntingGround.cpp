#include "Creatures/HuntingGround.h"
#include "World/PlayableBoundary.h"

bool FHuntingGround::IsSet() const
{
	return Corners.Num() >= 3 || Radius > 0.f;
}

FVector FHuntingGround::GetMiddle(const FVector& Home) const
{
	return bAroundHome ? Home : Center;
}

bool FHuntingGround::Contains(const FVector& Point, const FVector& Home) const
{
	if (!IsSet())
	{
		return true;
	}
	const FVector Middle = GetMiddle(Home);
	if (MaxRise > 0.f && FMath::Abs(Point.Z - Middle.Z) > MaxRise)
	{
		return false;
	}
	const FVector2D Flat(Point.X, Point.Y);
	if (Corners.Num() >= 3)
	{
		return IsInsidePolygon(Corners, Flat) || DistanceToPolygonEdge(Corners, Flat) <= Margin;
	}
	return FVector2D::DistSquared(Flat, FVector2D(Middle.X, Middle.Y)) <= FMath::Square(static_cast<double>(Radius));
}

bool FHuntingGround::ContainsSpot(const FVector& Point, const FVector& Home) const
{
	if (!IsSet())
	{
		return true;
	}
	const FVector2D Flat(Point.X, Point.Y);
	if (Corners.Num() >= 3)
	{
		return IsInsidePolygon(Corners, Flat);
	}
	const FVector Middle = GetMiddle(Home);
	return FVector2D::DistSquared(Flat, FVector2D(Middle.X, Middle.Y)) <= FMath::Square(static_cast<double>(Radius));
}

FHuntingGround FHuntingGround::MakeAround(const FVector& InCenter, float InRadius, const TArray<FVector>& InCorners, float InMargin,
	float InMaxRise)
{
	FHuntingGround Made;
	Made.Radius = FMath::Max(InRadius, 0.f);
	if (InCorners.Num() >= 3)
	{
		Made.Corners = InCorners;
	}
	Made.Margin = FMath::Max(InMargin, 0.f);
	Made.MaxRise = FMath::Max(InMaxRise, 0.f);
	Made.bAroundHome = false;
	Made.Center = InCenter;
	return Made;
}

bool FHuntingGround::IsInsidePolygon(const TArray<FVector>& Polygon, const FVector2D& Point)
{
	const int32 Count = Polygon.Num();
	if (Count < 3)
	{
		return false;
	}
	// Count the edges a ray from the point toward +Y crosses, as FPlayableBoundary::Contains does: an odd count is inside.
	bool bInside = false;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector& A = Polygon[Index];
		const FVector& B = Polygon[(Index + 1) % Count];
		if ((A.X > Point.X) != (B.X > Point.X))
		{
			const double CrossingY = A.Y + (Point.X - A.X) / (B.X - A.X) * (B.Y - A.Y);
			if (CrossingY > Point.Y)
			{
				bInside = !bInside;
			}
		}
	}
	return bInside;
}

double FHuntingGround::DistanceToPolygonEdge(const TArray<FVector>& Polygon, const FVector2D& Point)
{
	const int32 Count = Polygon.Num();
	double Nearest = TNumericLimits<double>::Max();
	if (Count < 2)
	{
		return Nearest;
	}
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector& A = Polygon[Index];
		const FVector& B = Polygon[(Index + 1) % Count];
		Nearest = FMath::Min(Nearest, FPlayableBoundary::DistanceToSegment(Point, FVector2D(A.X, A.Y), FVector2D(B.X, B.Y)));
	}
	return Nearest;
}
