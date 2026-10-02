#include "World/MinimapPaint.h"
#include "World/PlayableBoundary.h"
#include "UI/Style/LooterUIStyle.h"

namespace
{
	/** Height step (cm) between neighboring texels that reads as a cliff edge. */
	constexpr float CliffStep = 70.f;
	/** Height between contour lines (cm). */
	constexpr float ContourInterval = 200.f;

	// The map's palette: the HUD's dark glass and cyan lines. Land is kept low-contrast so markers stand out.
	const FColor LandLow(32, 82, 96, 225);
	const FColor LandHigh(66, 124, 138, 230);
	const FColor ContourColor(92, 156, 170, 235);
	const FColor ObstacleColor(176, 218, 232, 235);
	const FColor CoastColor(108, 212, 255, 255);
	const FColor CliffColor(150, 206, 224, 240);

	/** What a texel outside the playable area keeps of its brightness and of its opacity (the dark glass shows through). */
	constexpr float OutsideBrightness = 0.45f;
	constexpr float OutsideOpacity = 0.6f;

	FColor Shade(const FColor& Color, float Amount)
	{
		auto Channel = [Amount](uint8 Value) { return static_cast<uint8>(FMath::Clamp(Value * (1.f + Amount), 0.f, 255.f)); };
		return FColor(Channel(Color.R), Channel(Color.G), Channel(Color.B), Color.A);
	}
}

bool MinimapPaint::HeightRange(TConstArrayView<EMinimapTexel> Kinds, TConstArrayView<float> Heights, TConstArrayView<uint8> Inside,
	float& OutMin, float& OutMax)
{
	OutMin = TNumericLimits<float>::Max();
	OutMax = TNumericLimits<float>::Lowest();
	const bool bMasked = Inside.Num() > 0;
	const int32 Count = FMath::Min(Kinds.Num(), Heights.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (Kinds[Index] == EMinimapTexel::Ground && (!bMasked || (Inside.IsValidIndex(Index) && Inside[Index] != 0)))
		{
			OutMin = FMath::Min(OutMin, Heights[Index]);
			OutMax = FMath::Max(OutMax, Heights[Index]);
		}
	}
	if (OutMin > OutMax)
	{
		OutMin = 0.f;
		OutMax = 0.f;
		return false;
	}
	return true;
}

void MinimapPaint::Terrain(int32 N, TConstArrayView<EMinimapTexel> Kinds, TConstArrayView<float> Heights, float MinHeight,
	float MaxHeight, TArray<FColor>& OutColors)
{
	OutColors.Reset();
	if (N <= 0 || Kinds.Num() < N * N || Heights.Num() < N * N)
	{
		return;
	}
	OutColors.Init(FColor(0, 0, 0, 0), N * N);
	const float Range = FMath::Max(MaxHeight - MinHeight, 1.f);

	auto KindAt = [&Kinds, N](int32 U, int32 V) { return (U < 0 || V < 0 || U >= N || V >= N) ? EMinimapTexel::Void : Kinds[V * N + U]; };
	auto HeightAt = [&Kinds, &Heights, N](int32 U, int32 V, float Fallback)
	{
		return (U < 0 || V < 0 || U >= N || V >= N || Kinds[V * N + U] == EMinimapTexel::Void) ? Fallback : Heights[V * N + U];
	};

	for (int32 V = 0; V < N; ++V)
	{
		for (int32 U = 0; U < N; ++U)
		{
			const int32 Index = V * N + U;
			const EMinimapTexel Kind = Kinds[Index];
			if (Kind == EMinimapTexel::Void)
			{
				continue;
			}
			const float Height = Heights[Index];
			const bool bCoast = KindAt(U - 1, V) == EMinimapTexel::Void || KindAt(U + 1, V) == EMinimapTexel::Void
				|| KindAt(U, V - 1) == EMinimapTexel::Void || KindAt(U, V + 1) == EMinimapTexel::Void;
			if (bCoast)
			{
				OutColors[Index] = CoastColor;
				continue;
			}
			if (Kind == EMinimapTexel::Obstacle)
			{
				OutColors[Index] = ObstacleColor;
				continue;
			}
			float Steepest = 0.f;
			bool bContour = false;
			const int32 Band = FMath::FloorToInt32(Height / ContourInterval);
			for (const FIntPoint Step : { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) })
			{
				if (KindAt(U + Step.X, V + Step.Y) == EMinimapTexel::Ground)
				{
					const float Neighbor = HeightAt(U + Step.X, V + Step.Y, Height);
					Steepest = FMath::Max(Steepest, FMath::Abs(Neighbor - Height));
					// Draw each contour once, on its uphill side.
					bContour |= FMath::FloorToInt32(Neighbor / ContourInterval) < Band;
				}
			}
			if (Steepest > CliffStep)
			{
				OutColors[Index] = CliffColor;
				continue;
			}
			if (bContour)
			{
				OutColors[Index] = ContourColor;
				continue;
			}
			// Height tint, plus a touch of hillshade lit from the north-west so slopes read.
			const FLinearColor Low(LandLow), High(LandHigh);
			const float Tint = FMath::Clamp((Height - MinHeight) / Range, 0.f, 1.f);
			const FColor Base = FLinearColor::LerpUsingHSV(Low, High, Tint).ToFColor(true);
			const float Slope = (Height - HeightAt(U - 1, V - 1, Height)) / 80.f;
			OutColors[Index] = Shade(Base, FMath::Clamp(Slope, -0.1f, 0.1f));
		}
	}
}

void MinimapPaint::RasterizeBoundary(const FPlayableBoundary& Outline, const FBox2D& MapBounds, int32 N, TArray<uint8>& OutInside,
	TArray<uint8>& OutLine)
{
	OutInside.Reset();
	OutLine.Reset();
	const FVector2D Size = MapBounds.GetSize();
	if (N <= 0 || !Outline.IsValid() || Size.X <= 0.0 || Size.Y <= 0.0)
	{
		return;
	}
	OutInside.Init(0, N * N);
	OutLine.Init(0, N * N);

	// Inside: along each row of texel centers (a line of constant world X), the boundary's crossings. They pair up, so
	// a texel with an odd number of them at or below its center has an odd number above it, which is what Contains
	// counts: the two always agree.
	TArray<double> Crossings;
	for (int32 V = 0; V < N; ++V)
	{
		const double X = MapBounds.Max.X - (V + 0.5) / N * Size.X;
		Outline.CrossingsAtX(X, Crossings);
		int32 Below = 0;
		for (int32 U = 0; U < N; ++U)
		{
			const double Y = MapBounds.Min.Y + (U + 0.5) / N * Size.Y;
			while (Below < Crossings.Num() && Crossings[Below] <= Y)
			{
				++Below;
			}
			OutInside[V * N + U] = (Below % 2) == 1 ? 1 : 0;
		}
	}

	// The line: texels whose centers lie within LineHalfWidth of a closed edge, measured in texels (where texel (U, V)'s
	// center is the point (U, V)).
	auto ToTexels = [&MapBounds, Size, N](const FVector2D& Point)
	{
		return FVector2D((Point.Y - MapBounds.Min.Y) / Size.Y * N - 0.5, (MapBounds.Max.X - Point.X) / Size.X * N - 0.5);
	};
	for (int32 Edge = 0; Edge < Outline.NumEdges(); ++Edge)
	{
		if (Outline.IsOpen(Edge))
		{
			continue;
		}
		const FVector2D A = ToTexels(Outline.EdgeStart(Edge));
		const FVector2D B = ToTexels(Outline.EdgeEnd(Edge));
		const int32 UMin = FMath::Max(0, FMath::FloorToInt32(FMath::Min(A.X, B.X) - LineHalfWidth));
		const int32 UMax = FMath::Min(N - 1, FMath::CeilToInt32(FMath::Max(A.X, B.X) + LineHalfWidth));
		const int32 VMin = FMath::Max(0, FMath::FloorToInt32(FMath::Min(A.Y, B.Y) - LineHalfWidth));
		const int32 VMax = FMath::Min(N - 1, FMath::CeilToInt32(FMath::Max(A.Y, B.Y) + LineHalfWidth));
		for (int32 V = VMin; V <= VMax; ++V)
		{
			for (int32 U = UMin; U <= UMax; ++U)
			{
				const FVector2D Center(static_cast<double>(U), static_cast<double>(V));
				if (FPlayableBoundary::DistanceToSegment(Center, A, B) <= LineHalfWidth)
				{
					OutLine[V * N + U] = 1;
				}
			}
		}
	}
}

FColor MinimapPaint::BoundaryTexel(const FColor& Painted, bool bInside, bool bOnLine)
{
	if (Painted.A == 0)
	{
		// Nothing there to stand on (the void, a canyon with no collision): it stays clear.
		return Painted;
	}
	if (bOnLine)
	{
		return BoundaryColor();
	}
	if (!bInside)
	{
		auto Scale = [](uint8 Value, float Share) { return static_cast<uint8>(FMath::RoundToInt32(Value * Share)); };
		return FColor(Scale(Painted.R, OutsideBrightness), Scale(Painted.G, OutsideBrightness), Scale(Painted.B, OutsideBrightness),
			Scale(Painted.A, OutsideOpacity));
	}
	return Painted;
}

void MinimapPaint::ApplyBoundary(TConstArrayView<uint8> Inside, TConstArrayView<uint8> Line, TArray<FColor>& Colors)
{
	const int32 Count = FMath::Min3(Colors.Num(), Inside.Num(), Line.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Colors[Index] = BoundaryTexel(Colors[Index], Inside[Index] != 0, Line[Index] != 0);
	}
}

FColor MinimapPaint::BoundaryColor()
{
	return LooterUI::Color::Accent().ToFColor(true);
}
