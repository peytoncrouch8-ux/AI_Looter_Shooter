// LooterUI's painted pictures (FPaintedIcon: the portrait, bezels, gems) and its soft round glow (GlowBrush): each drawn
// into a texture once and shared for the session, like the Inked icons (LooterUIInkedIcons.cpp).

#include "UI/Style/LooterUIStyle.h"
#include "Engine/Texture2D.h"

namespace
{
	/**
	 * A fill is sampled at this many heights in each texture row, and across each of those lines the share of a pixel it
	 * covers is exact: every edge gets at least this many steps of anti-aliasing, near-upright ones a smooth ramp.
	 */
	constexpr int32 PaintSubRows = 8;

	/** The soft glow's texture, wide and high: big enough that a large glow (the banner's) stays smooth. */
	constexpr int32 GlowTextureSize = 128;

	/** One edge of a fill in texture pixels, its ends ordered top to bottom (Y0 < Y1). */
	struct FPaintEdge
	{
		double X0 = 0.0;
		double Y0 = 0.0;
		double X1 = 0.0;
		double Y1 = 0.0;
	};

	/** A color in display (sRGB) space, 0 to 1, with its (linear) alpha. */
	FVector4f DisplayColor(const FLinearColor& Color)
	{
		const FColor Bytes = Color.ToFColor(true);
		return FVector4f(Bytes.R / 255.f, Bytes.G / 255.f, Bytes.B / 255.f, Color.A);
	}

	double PaintSegmentDistance(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const double T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1e-12), 0.0, 1.0);
		return FVector2D::Distance(P, A + AB * T);
	}

	/** The texture pixels a layer can touch: its points' bounds, grown by its strokes and the anti-aliased edge. */
	FIntRect LayerArea(const LooterUI::FPaintLayer& Layer, double Scale, int32 Width, int32 Height)
	{
		FBox2D Bounds(ForceInit);
		for (const TArray<FVector2D>& Polygon : Layer.Fills)
		{
			for (const FVector2D& Point : Polygon)
			{
				Bounds += Point * Scale;
			}
		}
		if (!Layer.Strokes.IsEmpty())
		{
			const double Reach = Layer.StrokeWidth * 0.5 * Scale + 1.0;
			for (const TArray<FVector2D>& Line : Layer.Strokes)
			{
				for (const FVector2D& Point : Line)
				{
					Bounds += Point * Scale - FVector2D(Reach);
					Bounds += Point * Scale + FVector2D(Reach);
				}
			}
		}
		if (!Bounds.bIsValid)
		{
			return FIntRect();
		}
		return FIntRect(
			FMath::Clamp(FMath::FloorToInt32(Bounds.Min.X) - 1, 0, Width), FMath::Clamp(FMath::FloorToInt32(Bounds.Min.Y) - 1, 0, Height),
			FMath::Clamp(FMath::CeilToInt32(Bounds.Max.X) + 1, 0, Width), FMath::Clamp(FMath::CeilToInt32(Bounds.Max.Y) + 1, 0, Height));
	}

	/** Adds the share of each pixel that [XA, XB) covers on one line of a row, Weight being the line's share of the row. */
	void AddPaintSpan(float* Row, int32 Width, double XA, double XB, float Weight)
	{
		XA = FMath::Max(XA, 0.0);
		XB = FMath::Min(XB, static_cast<double>(Width));
		if (XB <= XA)
		{
			return;
		}
		const int32 First = FMath::FloorToInt32(XA);
		const int32 Last = FMath::Min(FMath::FloorToInt32(XB), Width - 1);
		if (First >= Last)
		{
			Row[First] += static_cast<float>(XB - XA) * Weight;
			return;
		}
		Row[First] += static_cast<float>(First + 1 - XA) * Weight;
		for (int32 X = First + 1; X < Last; ++X)
		{
			Row[X] += Weight;
		}
		Row[Last] += static_cast<float>(XB - Last) * Weight;
	}

	/** Adds the layer's fills to Cover inside Area: all its polygons together, even-odd. */
	void CoverFills(const TArray<TArray<FVector2D>>& Fills, double Scale, const FIntRect& Area, int32 Width, TArray<float>& Cover)
	{
		TArray<FPaintEdge> Edges;
		for (const TArray<FVector2D>& Polygon : Fills)
		{
			for (int32 Index = 0; Index < Polygon.Num(); ++Index)
			{
				const FVector2D A = Polygon[Index] * Scale;
				const FVector2D B = Polygon[(Index + 1) % Polygon.Num()] * Scale;
				// A level edge never crosses a sampling line (each line sits between pixel edges).
				if (A.Y != B.Y)
				{
					Edges.Add(A.Y < B.Y ? FPaintEdge{ A.X, A.Y, B.X, B.Y } : FPaintEdge{ B.X, B.Y, A.X, A.Y });
				}
			}
		}
		if (Edges.IsEmpty())
		{
			return;
		}

		TArray<double> Crossings;
		const float Weight = 1.f / PaintSubRows;
		for (int32 Y = Area.Min.Y; Y < Area.Max.Y; ++Y)
		{
			float* Row = Cover.GetData() + Y * Width;
			for (int32 Line = 0; Line < PaintSubRows; ++Line)
			{
				// Where the edges cross this line, left to right; even-odd, every other gap between them is inside.
				const double LineY = Y + (Line + 0.5) / PaintSubRows;
				Crossings.Reset();
				for (const FPaintEdge& Edge : Edges)
				{
					if (LineY >= Edge.Y0 && LineY < Edge.Y1)
					{
						Crossings.Add(Edge.X0 + (LineY - Edge.Y0) * (Edge.X1 - Edge.X0) / (Edge.Y1 - Edge.Y0));
					}
				}
				Crossings.Sort();
				for (int32 Index = 0; Index + 1 < Crossings.Num(); Index += 2)
				{
					AddPaintSpan(Row, Width, Crossings[Index], Crossings[Index + 1], Weight);
				}
			}
		}
	}

	/**
	 * Adds the layer's strokes to Cover: each pixel takes its coverage from its distance to the nearest segment, which
	 * makes every join and cap round, and keeps the larger of that and the fills' coverage.
	 */
	void CoverStrokes(const TArray<TArray<FVector2D>>& Strokes, double Scale, double HalfWidth, int32 Width, int32 Height,
		TArray<float>& Cover)
	{
		const double Reach = HalfWidth + 1.0;
		for (const TArray<FVector2D>& Line : Strokes)
		{
			// A single point is a dot: one segment of no length.
			const int32 Segments = FMath::Max(Line.Num() - 1, Line.IsEmpty() ? 0 : 1);
			for (int32 Index = 0; Index < Segments; ++Index)
			{
				const FVector2D A = Line[Index] * Scale;
				const FVector2D B = Line[FMath::Min(Index + 1, Line.Num() - 1)] * Scale;
				const int32 X0 = FMath::Max(FMath::FloorToInt32(FMath::Min(A.X, B.X) - Reach), 0);
				const int32 Y0 = FMath::Max(FMath::FloorToInt32(FMath::Min(A.Y, B.Y) - Reach), 0);
				const int32 X1 = FMath::Min(FMath::CeilToInt32(FMath::Max(A.X, B.X) + Reach), Width);
				const int32 Y1 = FMath::Min(FMath::CeilToInt32(FMath::Max(A.Y, B.Y) + Reach), Height);
				for (int32 Y = Y0; Y < Y1; ++Y)
				{
					for (int32 X = X0; X < X1; ++X)
					{
						const double Distance = PaintSegmentDistance(FVector2D(X + 0.5, Y + 0.5), A, B);
						const float Covered = static_cast<float>(FMath::Clamp(HalfWidth + 0.5 - Distance, 0.0, 1.0));
						float& Pixel = Cover[Y * Width + X];
						Pixel = FMath::Max(Pixel, Covered);
					}
				}
			}
		}
	}

	/**
	 * Paints the layer's coverage over the picture in its color or gradient, and clears the coverage for the next layer.
	 * Layers go "over" each other in display (sRGB) space, premultiplied: Slate blends the HUD's layers that way too (its
	 * shader writes gamma-corrected color, blended by source alpha), so a painted picture looks like the same layers
	 * stacked as widgets, and like the mockups.
	 */
	void PaintLayer(const LooterUI::FPaintLayer& Layer, double Scale, const FIntRect& Area, int32 Width, TArray<float>& Cover,
		TArray<FVector4f>& Paint)
	{
		const FVector4f From = DisplayColor(Layer.Color);
		const FVector4f To = Layer.GradientTo.IsSet() ? DisplayColor(Layer.GradientTo.GetValue()) : From;
		const bool bGradient = Layer.GradientTo.IsSet();
		const FVector2D Start = Layer.GradientStart * Scale;
		const FVector2D Axis = (Layer.GradientEnd - Layer.GradientStart) * Scale;
		const double AxisLengthSquared = Axis.SizeSquared();
		const float Opacity = FMath::Clamp(Layer.Opacity, 0.f, 1.f);

		for (int32 Y = Area.Min.Y; Y < Area.Max.Y; ++Y)
		{
			for (int32 X = Area.Min.X; X < Area.Max.X; ++X)
			{
				const int32 Index = Y * Width + X;
				const float Covered = FMath::Min(Cover[Index], 1.f);
				Cover[Index] = 0.f;
				if (Covered <= 0.f)
				{
					continue;
				}
				FVector4f Color = From;
				if (bGradient && AxisLengthSquared > 1e-12)
				{
					const FVector2D Offset = FVector2D(X + 0.5, Y + 0.5) - Start;
					const double T = Layer.bRadialGradient
						? FMath::Clamp(FMath::Sqrt(Offset.SizeSquared() / AxisLengthSquared), 0.0, 1.0)
						: FMath::Clamp(FVector2D::DotProduct(Offset, Axis) / AxisLengthSquared, 0.0, 1.0);
					Color = From + (To - From) * static_cast<float>(T);
				}
				const float Alpha = Covered * Color.W * Opacity;
				FVector4f& Under = Paint[Index];
				Under.X = Color.X * Alpha + Under.X * (1.f - Alpha);
				Under.Y = Color.Y * Alpha + Under.Y * (1.f - Alpha);
				Under.Z = Color.Z * Alpha + Under.Z * (1.f - Alpha);
				Under.W = Alpha + Under.W * (1.f - Alpha);
			}
		}
	}

	uint8 PaintByte(float Value)
	{
		return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt32(Value * 255.f), 0, 255));
	}

	/**
	 * The painted picture as BGRA bytes with straight alpha, as Slate draws textures. A clear pixel takes the color of its
	 * covered neighbors: the texture filter blends toward it at an edge, and with black there the edge would darken.
	 */
	TArray<uint8> PaintedPixels(const TArray<FVector4f>& Paint, int32 Width, int32 Height)
	{
		TArray<uint8> Pixels;
		Pixels.SetNumZeroed(Width * Height * 4);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const int32 Index = Y * Width + X;
				const FVector4f& Here = Paint[Index];
				const uint8 Alpha = PaintByte(Here.W);
				FVector4f Sum = Here;
				if (Alpha == 0)
				{
					Sum = FVector4f(0.f, 0.f, 0.f, 0.f);
					for (int32 NY = FMath::Max(Y - 1, 0); NY <= FMath::Min(Y + 1, Height - 1); ++NY)
					{
						for (int32 NX = FMath::Max(X - 1, 0); NX <= FMath::Min(X + 1, Width - 1); ++NX)
						{
							Sum += Paint[NY * Width + NX];
						}
					}
				}
				if (Sum.W <= 1e-6f)
				{
					continue;
				}
				// Premultiplied sums over their alpha: the covered pixels' color, weighted by how covered they are.
				uint8* Pixel = Pixels.GetData() + Index * 4;
				Pixel[0] = PaintByte(Sum.Z / Sum.W);
				Pixel[1] = PaintByte(Sum.Y / Sum.W);
				Pixel[2] = PaintByte(Sum.X / Sum.W);
				Pixel[3] = Alpha;
			}
		}
		return Pixels;
	}

	UTexture2D* MakePaintedTexture(int32 Width, int32 Height, const TArray<uint8>& Pixels, const FString& TextureName)
	{
		UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, FName(*TextureName), Pixels);
		Texture->SRGB = true;
		Texture->Filter = TF_Bilinear;
		Texture->NeverStream = true;
		Texture->UpdateResource();
		Texture->AddToRoot(); // Shared for the whole session, like the other generated UI textures.
		return Texture;
	}

	/** The picture at PixelsPerUnit, its layers painted back to front, anti-aliased. */
	UTexture2D* DrawPaintedIcon(const LooterUI::FPaintedIcon& Icon, float PixelsPerUnit, const FString& TextureName)
	{
		// A view box meant to come out a whole number of pixels mustn't gain a column from rounding.
		const int32 Width = FMath::Max(FMath::CeilToInt32(Icon.ViewBox.X * PixelsPerUnit - 1e-3), 1);
		const int32 Height = FMath::Max(FMath::CeilToInt32(Icon.ViewBox.Y * PixelsPerUnit - 1e-3), 1);
		const double Scale = PixelsPerUnit;

		TArray<FVector4f> Paint;
		Paint.SetNumZeroed(Width * Height);
		TArray<float> Cover;
		Cover.SetNumZeroed(Width * Height);
		for (const LooterUI::FPaintLayer& Layer : Icon.Layers)
		{
			if (Layer.Opacity <= 0.f)
			{
				continue;
			}
			const FIntRect Area = LayerArea(Layer, Scale, Width, Height);
			if (Area.Width() <= 0 || Area.Height() <= 0)
			{
				continue;
			}
			CoverFills(Layer.Fills, Scale, Area, Width, Cover);
			CoverStrokes(Layer.Strokes, Scale, Layer.StrokeWidth * 0.5 * Scale, Width, Height, Cover);
			PaintLayer(Layer, Scale, Area, Width, Cover, Paint);
		}
		return MakePaintedTexture(Width, Height, PaintedPixels(Paint, Width, Height), TextureName);
	}

	/** White, its alpha falling from the middle as (1 - r^2)^2: flat at the heart, no edge at the rim. */
	UTexture2D* GetGlowTexture()
	{
		static UTexture2D* Texture = nullptr;
		if (!Texture)
		{
			const float Half = GlowTextureSize * 0.5f;
			TArray<uint8> Pixels;
			Pixels.SetNumUninitialized(GlowTextureSize * GlowTextureSize * 4);
			for (int32 Y = 0; Y < GlowTextureSize; ++Y)
			{
				for (int32 X = 0; X < GlowTextureSize; ++X)
				{
					const float R2 = (FMath::Square(X + 0.5f - Half) + FMath::Square(Y + 0.5f - Half)) / FMath::Square(Half);
					uint8* Pixel = Pixels.GetData() + (Y * GlowTextureSize + X) * 4;
					Pixel[0] = 255;
					Pixel[1] = 255;
					Pixel[2] = 255;
					Pixel[3] = PaintByte(R2 < 1.f ? FMath::Square(1.f - R2) : 0.f);
				}
			}
			Texture = MakePaintedTexture(GlowTextureSize, GlowTextureSize, Pixels, TEXT("UI_Glow"));
		}
		return Texture;
	}
}

FSlateBrush LooterUI::PaintedIconBrush(FName Name, const FPaintedIcon& Icon, float PixelsPerUnit, const FVector2D& Size, const FLinearColor& Tint)
{
	static TMap<TPair<FName, int32>, UTexture2D*> Cache;
	const int32 Resolution = FMath::RoundToInt(PixelsPerUnit * 100.f);
	UTexture2D*& Texture = Cache.FindOrAdd({ Name, Resolution });
	if (!Texture)
	{
		Texture = DrawPaintedIcon(Icon, PixelsPerUnit, FString::Printf(TEXT("UI_Painted_%s_%d"), *Name.ToString(), Resolution));
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = Size;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(Tint);
	return Brush;
}

FSlateBrush LooterUI::GlowBrush(const FVector2D& Size, const FLinearColor& Tint)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(GetGlowTexture());
	Brush.ImageSize = Size;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(Tint);
	return Brush;
}
