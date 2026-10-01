// LooterUI's Inked icons (FInkedIcon): drawing one into a texture, the same way Art/Icons/InkedIcons.py draws its
// reference pictures (its draw(); keep the two the same).

#include "UI/Style/LooterUIStyle.h"
#include "Engine/Texture2D.h"

namespace
{
	/** Each texture pixel averages Supersample x Supersample points. */
	constexpr int32 Supersample = 4;

	/** The ink line stays at least this many texture pixels wide when an icon is drawn small (about 1.3 px on screen). */
	constexpr float MinOutlinePixels = 2.6f;

	/** A polygon or polyline with its bounds, so most points can skip it at a glance. */
	struct FPath
	{
		const TArray<FVector2D>* Points = nullptr;
		FBox2D Bounds = FBox2D(ForceInit);
		bool bClosed = true;
	};

	TArray<FPath> MakePaths(const TArray<TArray<FVector2D>>& Lines, bool bClosed)
	{
		TArray<FPath> Paths;
		for (const TArray<FVector2D>& Points : Lines)
		{
			if (Points.Num() < 2)
			{
				continue;
			}
			FPath& Path = Paths.AddDefaulted_GetRef();
			Path.Points = &Points;
			Path.bClosed = bClosed;
			for (const FVector2D& Point : Points)
			{
				Path.Bounds += Point;
			}
		}
		return Paths;
	}

	bool Near(const FBox2D& Bounds, const FVector2D& P, double Reach)
	{
		return P.X >= Bounds.Min.X - Reach && P.X <= Bounds.Max.X + Reach && P.Y >= Bounds.Min.Y - Reach && P.Y <= Bounds.Max.Y + Reach;
	}

	/** Even-odd point in polygon, as the reference's inside(). */
	bool Inside(const FPath& Path, const FVector2D& P)
	{
		if (!Near(Path.Bounds, P, 0.0))
		{
			return false;
		}
		const TArray<FVector2D>& Points = *Path.Points;
		bool bInside = false;
		for (int32 I = 0; I < Points.Num(); ++I)
		{
			const FVector2D& A = Points[I];
			const FVector2D& B = Points[(I + 1) % Points.Num()];
			if (A.Y == B.Y)
			{
				continue;
			}
			if ((A.Y > P.Y) != (B.Y > P.Y) && P.X < A.X + (P.Y - A.Y) * (B.X - A.X) / (B.Y - A.Y))
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}

	double SegmentDistance(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const double T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1e-12), 0.0, 1.0);
		return FVector2D::Distance(P, A + AB * T);
	}

	/** Whether P lies within Reach of the path's edges (closing edge included for a polygon). */
	bool Within(const FPath& Path, const FVector2D& P, double Reach)
	{
		if (!Near(Path.Bounds, P, Reach))
		{
			return false;
		}
		const TArray<FVector2D>& Points = *Path.Points;
		const int32 Count = Path.bClosed ? Points.Num() : Points.Num() - 1;
		for (int32 I = 0; I < Count; ++I)
		{
			if (SegmentDistance(P, Points[I], Points[(I + 1) % Points.Num()]) <= Reach)
			{
				return true;
			}
		}
		return false;
	}

	/** The icon at PixelsPerUnit, in its own colors (sRGB, straight alpha). */
	UTexture2D* DrawInkedIcon(const LooterUI::FInkedIcon& Icon, float PixelsPerUnit, const FString& TextureName)
	{
		const int32 Width = FMath::Max(FMath::CeilToInt(Icon.ViewBox.X * PixelsPerUnit), 1);
		const int32 Height = FMath::Max(FMath::CeilToInt(Icon.ViewBox.Y * PixelsPerUnit), 1);
		const double Outline = FMath::Min(FMath::Max(static_cast<double>(Icon.OutlineWidth), MinOutlinePixels / PixelsPerUnit),
			static_cast<double>(Icon.MaxOutlineWidth));
		const double HalfStroke = Icon.StrokeWidth * 0.5;
		const double HalfCut = Icon.CutWidth * 0.5;
		const TArray<FPath> Shapes = MakePaths(Icon.Shapes, true);
		const TArray<FPath> Holes = MakePaths(Icon.Holes, true);
		const TArray<FPath> Strokes = MakePaths(Icon.Strokes, false);
		const TArray<FPath> Cuts = MakePaths(Icon.Cuts, false);

		// The colors are averaged as sRGB bytes, premultiplied by coverage, like the reference pictures.
		const FColor Ink = LooterUI::Color::IconInk().ToFColor(true);
		const FColor Light = LooterUI::Color::IconLight().ToFColor(true);
		const FColor Shade = LooterUI::Color::IconShade().ToFColor(true);

		TArray<uint8> Pixels;
		Pixels.SetNumZeroed(Width * Height * 4);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				double R = 0.0, G = 0.0, B = 0.0;
				int32 Covered = 0;
				for (int32 SY = 0; SY < Supersample; ++SY)
				{
					for (int32 SX = 0; SX < Supersample; ++SX)
					{
						const FVector2D P((X + (SX + 0.5) / Supersample) / PixelsPerUnit, (Y + (SY + 0.5) / Supersample) / PixelsPerUnit);
						bool bShape = Shapes.ContainsByPredicate([&P](const FPath& Path) { return Inside(Path, P); })
							|| Strokes.ContainsByPredicate([&P, HalfStroke](const FPath& Path) { return Within(Path, P, HalfStroke); });
						bool bHole = false;
						for (const FPath& Hole : Holes)
						{
							bHole |= Inside(Hole, P);
						}

						const FColor* Paint = nullptr;
						if (bShape && !bHole)
						{
							const bool bCut = Cuts.ContainsByPredicate([&P, HalfCut](const FPath& Path) { return Within(Path, P, HalfCut); });
							Paint = bCut ? &Ink : (P.Y < Icon.ShadeY ? &Light : &Shade);
						}
						else
						{
							// The ink line: near the shapes' edges, a stroke, or the edge of a hole the point is in.
							bool bInk = Shapes.ContainsByPredicate([&P, Outline](const FPath& Path) { return Within(Path, P, Outline); })
								|| Strokes.ContainsByPredicate([&P, Outline, HalfStroke](const FPath& Path) { return Within(Path, P, Outline + HalfStroke); });
							for (int32 Index = 0; !bInk && Index < Holes.Num(); ++Index)
							{
								bInk = Inside(Holes[Index], P) && Within(Holes[Index], P, Outline);
							}
							Paint = bInk ? &Ink : nullptr;
						}
						if (Paint)
						{
							R += Paint->R;
							G += Paint->G;
							B += Paint->B;
							++Covered;
						}
					}
				}
				if (Covered > 0)
				{
					// Straight alpha: the covered points' average color, and how many of them there were.
					const int32 Index = (Y * Width + X) * 4;
					Pixels[Index + 0] = static_cast<uint8>(FMath::Clamp(B / Covered + 0.5, 0.0, 255.0));
					Pixels[Index + 1] = static_cast<uint8>(FMath::Clamp(G / Covered + 0.5, 0.0, 255.0));
					Pixels[Index + 2] = static_cast<uint8>(FMath::Clamp(R / Covered + 0.5, 0.0, 255.0));
					Pixels[Index + 3] = static_cast<uint8>(FMath::Clamp(255.0 * Covered / (Supersample * Supersample) + 0.5, 0.0, 255.0));
				}
			}
		}

		UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, FName(*TextureName), Pixels);
		Texture->SRGB = true;
		Texture->Filter = TF_Bilinear;
		Texture->NeverStream = true;
		Texture->UpdateResource();
		Texture->AddToRoot(); // Shared for the whole session, like the other generated UI textures.
		return Texture;
	}
}

FSlateBrush LooterUI::InkedIconBrush(FName Name, const FInkedIcon& Icon, float PixelsPerUnit, const FVector2D& Size, const FLinearColor& Tint)
{
	static TMap<TPair<FName, int32>, UTexture2D*> Cache;
	const int32 Resolution = FMath::RoundToInt(PixelsPerUnit * 100.f);
	UTexture2D*& Texture = Cache.FindOrAdd({ Name, Resolution });
	if (!Texture)
	{
		Texture = DrawInkedIcon(Icon, PixelsPerUnit, FString::Printf(TEXT("UI_Inked_%s_%d"), *Name.ToString(), Resolution));
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = Size;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(Tint);
	return Brush;
}

FSlateBrush LooterUI::InkedIconBrush(FName Name, const FInkedIcon& Icon, const FVector2D& Box, const FLinearColor& Tint)
{
	// As large as fits, keeping the icon's shape; the texture at twice that, so it stays crisp.
	const float Scale = static_cast<float>(FMath::Min(Box.X / FMath::Max(Icon.ViewBox.X, 1.0), Box.Y / FMath::Max(Icon.ViewBox.Y, 1.0)));
	const FVector2D Size = Icon.ViewBox * Scale;
	// Rounded to hundredths, so close sizes share a texture.
	const float PixelsPerUnit = FMath::RoundToFloat(Scale * 2.f * 100.f) / 100.f;
	return InkedIconBrush(Name, Icon, PixelsPerUnit, Size, Tint);
}
