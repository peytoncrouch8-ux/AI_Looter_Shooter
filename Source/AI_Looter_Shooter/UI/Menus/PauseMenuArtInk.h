#pragma once

#include "CoreMinimal.h"
#include "UI/Style/LooterUIStyle.h"

/**
 * The drawing helpers PauseMenuArt's two files share (PauseMenuArt.cpp: the dial and the backdrop; PauseMenuArtShapes.cpp:
 * the tags, cards, gems and small pieces), in LooterReticle's conventions: degrees from three o'clock, clockwise on screen.
 * A line "edged" has the kit's dark edge under it, the mockup's edged(): the line 3 px wider in Outline at 0.7 of the line's
 * opacity (e.js).
 */
namespace PauseMenuArtInk
{
	using LooterUI::FPaintLayer;
	using LooterUI::FPaintedIcon;

	/** The dark edge is the line plus this much, at this share of the line's opacity (e.js edged()). */
	inline constexpr float EdgeExtra = 3.f;
	inline constexpr float EdgeShare = 0.7f;

	inline FVector2D OnCircle(const FVector2D& Centre, float Radius, float Degrees)
	{
		const float Angle = FMath::DegreesToRadians(Degrees);
		return Centre + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
	}

	/** Points along a circle from one angle to the other (either way round), close enough to read as a curve. */
	inline TArray<FVector2D> ArcPoints(const FVector2D& Centre, float Radius, float FromDegrees, float ToDegrees)
	{
		const float StepDegrees = FMath::Min(4.f, FMath::RadiansToDegrees(2.5f / FMath::Max(Radius, 1.f)));
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(FMath::Abs(ToDegrees - FromDegrees) / StepDegrees));
		TArray<FVector2D> Points;
		Points.Reserve(Steps + 1);
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			Points.Add(OnCircle(Centre, Radius, FMath::Lerp(FromDegrees, ToDegrees, float(Index) / Steps)));
		}
		return Points;
	}

	/** A whole circle as a polygon (a fill) or a closed line (a stroke: its last point the first again). */
	inline TArray<FVector2D> CirclePoints(const FVector2D& Centre, float Radius, bool bClosedLine)
	{
		TArray<FVector2D> Points = ArcPoints(Centre, Radius, 0.f, 360.f);
		if (!bClosedLine)
		{
			Points.Pop();
		}
		return Points;
	}

	inline TArray<FVector2D> Spoke(const FVector2D& Centre, float Degrees, float From, float To)
	{
		return { OnCircle(Centre, From, Degrees), OnCircle(Centre, To, Degrees) };
	}

	/** A polygon as a closed line: its first point again at the end (copied first: adding an element of the array being
	 *  grown asserts, Docs/Pipeline/Bugs.md). */
	inline TArray<FVector2D> ClosedLine(const TArray<FVector2D>& Polygon)
	{
		TArray<FVector2D> Line = Polygon;
		if (Polygon.Num() > 0)
		{
			const FVector2D First = Polygon[0];
			Line.Add(First);
		}
		return Line;
	}

	inline TArray<FVector2D> Moved(const TArray<FVector2D>& Points, const FVector2D& By)
	{
		TArray<FVector2D> Result = Points;
		for (FVector2D& Point : Result)
		{
			Point += By;
		}
		return Result;
	}

	inline FPaintLayer FillLayer(TArray<TArray<FVector2D>> Polygons, const FLinearColor& Tint, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Fills = MoveTemp(Polygons);
		Layer.Color = Tint;
		Layer.Opacity = Opacity;
		return Layer;
	}

	inline FPaintLayer StrokeLayer(TArray<TArray<FVector2D>> Strokes, float Width, const FLinearColor& Tint, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Strokes = MoveTemp(Strokes);
		Layer.StrokeWidth = Width;
		Layer.Color = Tint;
		Layer.Opacity = Opacity;
		return Layer;
	}

	/**
	 * A picture's lines collected as they're drawn, then painted all edges first and everything else over them, so one
	 * line's dark edge never cuts across another line where they meet (a tick over the ring).
	 */
	struct FInk
	{
		TArray<FPaintLayer> Edges;
		TArray<FPaintLayer> Lines;

		/** Strokes in Tint, with the dark edge under them when bEdged. */
		void Line(TArray<TArray<FVector2D>> Strokes, float Width, const FLinearColor& Tint, float Opacity, bool bEdged)
		{
			if (Strokes.IsEmpty())
			{
				return;
			}
			if (bEdged)
			{
				Edges.Add(StrokeLayer(Strokes, Width + EdgeExtra, LooterUI::Color::Outline(), EdgeShare * Opacity));
			}
			Lines.Add(StrokeLayer(MoveTemp(Strokes), Width, Tint, Opacity));
		}

		/** A filled shape over the lines so far, with a thin dark edge round it (the mockup's paint-order stroke). */
		void Shape(const TArray<FVector2D>& Polygon, const FLinearColor& Tint, float EdgeWidth)
		{
			if (EdgeWidth > 0.f)
			{
				Lines.Add(StrokeLayer({ ClosedLine(Polygon) }, EdgeWidth * 2.f, LooterUI::Color::Outline(), 0.95f));
			}
			Lines.Add(FillLayer({ Polygon }, Tint));
		}

		void Add(FPaintLayer Layer)
		{
			Lines.Add(MoveTemp(Layer));
		}

		/** The layers in painting order. */
		TArray<FPaintLayer> Layers() const
		{
			TArray<FPaintLayer> Result = Edges;
			Result.Append(Lines);
			return Result;
		}

		FPaintedIcon Finish(const FVector2D& ViewBox) const
		{
			FPaintedIcon Icon;
			Icon.ViewBox = ViewBox;
			Icon.Layers = Layers();
			return Icon;
		}
	};

	/** Every point of a picture moved by By (fills and strokes; a gradient's ends too). */
	inline void MoveLayers(TArray<FPaintLayer>& Layers, const FVector2D& By)
	{
		for (FPaintLayer& Layer : Layers)
		{
			for (TArray<FVector2D>& Polygon : Layer.Fills)
			{
				for (FVector2D& Point : Polygon)
				{
					Point += By;
				}
			}
			for (TArray<FVector2D>& Line : Layer.Strokes)
			{
				for (FVector2D& Point : Line)
				{
					Point += By;
				}
			}
			Layer.GradientStart += By;
			Layer.GradientEnd += By;
		}
	}

	/** The box every layer's paint reaches (strokes by half their width). */
	inline FBox2D LayersBounds(const TArray<FPaintLayer>& Layers)
	{
		FBox2D Bounds(ForceInit);
		for (const FPaintLayer& Layer : Layers)
		{
			for (const TArray<FVector2D>& Polygon : Layer.Fills)
			{
				for (const FVector2D& Point : Polygon)
				{
					Bounds += Point;
				}
			}
			const FVector2D Half(Layer.StrokeWidth * 0.5f);
			for (const TArray<FVector2D>& Line : Layer.Strokes)
			{
				for (const FVector2D& Point : Line)
				{
					Bounds += Point - Half;
					Bounds += Point + Half;
				}
			}
		}
		return Bounds;
	}
}
