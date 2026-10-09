// BossSealFog: the fog wall's look as plain functions (BossSeal.h), apart from the world so the tests can check them.

#include "Bosses/BossSeal.h"

float BossSealFog::Density(float Z, float Height)
{
	const float Top = FMath::Max(Height, 1.f);
	return 1.f - 0.75f * FMath::SmoothStep(0.1f * Top, 0.9f * Top, Z);
}

float BossSealFog::Flare(float Distance)
{
	return 1.f - FMath::SmoothStep(60.f, 380.f, Distance);
}

float BossSealFog::NearestOnPath(const TArray<FVector>& Path, bool bClosed, const FVector& Point, FVector& OutNearest)
{
	OutNearest = Point;
	if (Path.IsEmpty())
	{
		return TNumericLimits<float>::Max();
	}
	const int32 Count = Path.Num();
	const int32 Spans = Count == 1 ? 1 : (bClosed ? Count : Count - 1);
	double Best = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Spans; ++Index)
	{
		const FVector& From = Path[Index];
		const FVector& To = Path[(Index + 1) % Count];
		const double AlongX = To.X - From.X;
		const double AlongY = To.Y - From.Y;
		const double Length2 = AlongX * AlongX + AlongY * AlongY;
		const double Share = Length2 > 1e-4 ? FMath::Clamp(((Point.X - From.X) * AlongX + (Point.Y - From.Y) * AlongY) / Length2, 0.0, 1.0) : 0.0;
		const FVector Near = From + (To - From) * Share;
		const double Away = FMath::Square(Point.X - Near.X) + FMath::Square(Point.Y - Near.Y);
		if (Away < Best)
		{
			Best = Away;
			OutNearest = Near;
		}
	}
	return static_cast<float>(FMath::Sqrt(Best));
}
