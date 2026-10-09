#include "UI/Inventory/MapView.h"

void FMapView::Clamp()
{
	Zoom = FMath::Clamp(Zoom, MinZoom, MaxZoom);
	// How much of the map (0-1) half the frame shows along each side: past half the map, the map can't fill that side
	// and stays centred on it; otherwise its edges never come inside the frame.
	const double S = Side();
	const FVector2D Half = Frame * 0.5 / S;
	Center.X = Half.X >= 0.5 ? 0.5 : FMath::Clamp(Center.X, Half.X, 1.0 - Half.X);
	Center.Y = Half.Y >= 0.5 ? 0.5 : FMath::Clamp(Center.Y, Half.Y, 1.0 - Half.Y);
}

void FMapView::ZoomAt(const FVector2D& FramePoint, double Factor)
{
	const FVector2D Under = FrameToUV(FramePoint);
	Zoom = FMath::Clamp(Zoom * Factor, MinZoom, MaxZoom);
	// The point under the pointer stays put: the centre moves to wherever puts Under back there at the new size.
	Center = Under - (FramePoint - Frame * 0.5) / Side();
	Clamp();
}

void FMapView::Pan(const FVector2D& Delta)
{
	Center -= Delta / Side();
	Clamp();
}

void FMapView::Fit(const FBox2D& Box, double Margin)
{
	if (!Box.bIsValid)
	{
		Zoom = MinZoom;
		Center = FVector2D(0.5, 0.5);
		Clamp();
		return;
	}
	const FVector2D Size = Box.GetSize() * (1.0 + 2.0 * Margin);
	// The box's longer side (in frame proportions) fills the frame.
	const double ZoomX = Size.X > 0.0 ? Frame.X / (Size.X * FitSide()) : MaxZoom;
	const double ZoomY = Size.Y > 0.0 ? Frame.Y / (Size.Y * FitSide()) : MaxZoom;
	Zoom = FMath::Min(ZoomX, ZoomY);
	Center = Box.GetCenter();
	Clamp();
}

double FMapView::GridStep(double MapSize, double MinSpacing) const
{
	static const double Steps[] = { 2500.0, 5000.0, 10000.0, 25000.0, 50000.0, 100000.0 };
	const double PagePerCm = MapSize > 0.0 ? Side() / MapSize : 0.0;
	for (const double Step : Steps)
	{
		if (Step * PagePerCm >= MinSpacing)
		{
			return Step;
		}
	}
	return Steps[UE_ARRAY_COUNT(Steps) - 1];
}

int32 FMapView::PickInDirection(TConstArrayView<FVector2D> Points, const FVector2D& From, const FVector2D& Direction, int32 Exclude)
{
	const FVector2D Way = Direction.GetSafeNormal();
	if (Way.IsZero())
	{
		return INDEX_NONE;
	}
	int32 Best = INDEX_NONE;
	double BestScore = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const FVector2D Offset = Points[Index] - From;
		const double Distance = Offset.Size();
		if (Index == Exclude || Distance < 1e-3)
		{
			continue;
		}
		// Ahead within 60 degrees (cos 0.5); one straight ahead counts as half as far as one off to the side.
		const double Ahead = FVector2D::DotProduct(Offset / Distance, Way);
		if (Ahead < 0.5)
		{
			continue;
		}
		const double Score = Distance * (2.0 - Ahead);
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Index;
		}
	}
	return Best;
}
