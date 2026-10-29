#include "UI/Style/LooterReticleArt.h"

// Concept 3's sight pictures, drawn from the mockup's SVG helpers (make_a.js: ticks, reticle, rowBrackets, ruler) as the
// kit's painted pictures. Sizes follow the mockup at the sizes it draws them, and scale gently for bigger rings.

using namespace LooterUI;

namespace
{
	/** The dark edge under a line is the line plus this much (the mockup's P(): stroke width + 3). */
	constexpr float EdgeExtra = 3.f;
	/** How strong the dark edge is under the main lines, and under the thin ticks (fainter, or the ticks read as a band). */
	constexpr float EdgeStrong = 0.7f;
	constexpr float EdgeSoft = 0.4f;

	/**
	 * A picture's lines collected as they're drawn, then painted all edges first and all lines over them, so one line's
	 * dark edge never cuts across another line where they meet (a tick over the ring).
	 */
	struct FInk
	{
		TArray<FPaintLayer> Edges;
		TArray<FPaintLayer> Lines;

		void Line(TArray<TArray<FVector2D>> Strokes, float Width, const FLinearColor& Tint, float Opacity, float EdgeOpacity)
		{
			if (Strokes.IsEmpty())
			{
				return;
			}
			if (EdgeOpacity > 0.f)
			{
				FPaintLayer Edge;
				Edge.Strokes = Strokes;
				Edge.StrokeWidth = Width + EdgeExtra;
				Edge.Color = Color::Outline();
				Edge.Opacity = EdgeOpacity;
				Edges.Add(MoveTemp(Edge));
			}
			FPaintLayer Layer;
			Layer.Strokes = MoveTemp(Strokes);
			Layer.StrokeWidth = Width;
			Layer.Color = Tint;
			Layer.Opacity = Opacity;
			Lines.Add(MoveTemp(Layer));
		}

		FPaintedIcon Finish(const FVector2D& ViewBox)
		{
			FPaintedIcon Icon;
			Icon.ViewBox = ViewBox;
			Icon.Layers = MoveTemp(Edges);
			Icon.Layers.Append(MoveTemp(Lines));
			return Icon;
		}
	};

	FVector2D OnCircle(const FVector2D& Centre, float Radius, float Degrees)
	{
		const float Angle = FMath::DegreesToRadians(Degrees);
		return Centre + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
	}

	/** Points along a circle from one angle to the other (either way round), close enough to read as a curve. */
	TArray<FVector2D> ArcPoints(const FVector2D& Centre, float Radius, float FromDegrees, float ToDegrees)
	{
		const float StepDegrees = FMath::Min(5.f, FMath::RadiansToDegrees(3.f / FMath::Max(Radius, 1.f)));
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(FMath::Abs(ToDegrees - FromDegrees) / StepDegrees));
		TArray<FVector2D> Points;
		Points.Reserve(Steps + 1);
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			Points.Add(OnCircle(Centre, Radius, FMath::Lerp(FromDegrees, ToDegrees, float(Index) / Steps)));
		}
		return Points;
	}

	/** A radial line at Degrees from one radius to another. */
	TArray<FVector2D> Spoke(const FVector2D& Centre, float Degrees, float From, float To)
	{
		return { OnCircle(Centre, From, Degrees), OnCircle(Centre, To, Degrees) };
	}

	// --- The halo's proportions: the mockup's main menu ring (r 34: ticks 4 out, 6 long, 7 more for the long ones) and
	// its pause ring (r 112: 6, 8, 10), between which they scale. ---

	float HaloTickGap(float Radius) { return FMath::Max(4.f, Radius * 0.05f); }
	float HaloTickLength(float Radius) { return FMath::Max(6.f, Radius * 0.07f); }
	float HaloLongExtra(float Radius) { return FMath::Max(7.f, Radius * 0.09f); }
	float HaloRingWidth(float Radius) { return Radius >= 80.f ? 3.f : 2.5f; }

	void AddHaloTicks(FInk& Ink, const FVector2D& Centre, float Radius, float TickStep, int32 LongEvery, float FromDeg, float ToDeg)
	{
		if (TickStep <= 0.f)
		{
			return;
		}
		const float Inner = Radius + HaloTickGap(Radius);
		const float Outer = Inner + HaloTickLength(Radius);
		TArray<TArray<FVector2D>> Shorts;
		TArray<TArray<FVector2D>> Longs;
		for (int32 Index = 0; Index * TickStep < 360.f - 1e-3f; ++Index)
		{
			const float Angle = Index * TickStep;
			if (Angle < FromDeg - 1e-3f || Angle >= ToDeg - 1e-3f)
			{
				continue;
			}
			if (LongEvery > 0 && Index % LongEvery == 0)
			{
				Longs.Add(Spoke(Centre, Angle, Inner, Outer + HaloLongExtra(Radius)));
			}
			else
			{
				Shorts.Add(Spoke(Centre, Angle, Inner, Outer));
			}
		}
		Ink.Line(MoveTemp(Shorts), 1.4f, Color::SaltLine(), 0.6f, EdgeSoft);
		Ink.Line(MoveTemp(Longs), 2.4f, Color::SaltLine(), 1.f, EdgeStrong);
	}

	// --- The bracket's proportions (rowBrackets: r = 0.75 of the row, the tick from a quarter of r - 2 outside the arc
	// to 14 inside it) ---

	float BracketRadius(float Height) { return 0.75f * Height; }
	float BracketOut(float Height) { return FMath::Max(BracketRadius(Height) * 0.25f - 2.f, 4.f); }
	constexpr float BracketTickIn = 14.f;
	constexpr float BracketHalfAngle = 40.f;

	// --- The range arc's ticks (between the mockup's main menu arc, 5, and its pause corner, 8 and 16) ---

	constexpr float ArcTickShort = 6.f;
	constexpr float ArcTickLong = 14.f;
	/** Past Pad, room for a line's half width and its edge. */
	constexpr float ArcMargin = LooterReticle::Pad + 2.5f;

	/** The range arc's lines round a circle centred at the origin; the caller moves them into the view box. */
	struct FArcLines
	{
		TArray<FVector2D> Arc;
		TArray<TArray<FVector2D>> Shorts;
		TArray<TArray<FVector2D>> Longs;
		FBox2D Bounds = FBox2D(ForceInit);
	};

	FArcLines BuildArc(float Radius, float FromDeg, float ToDeg, float TickStep, int32 LongEvery, bool bTicksIn)
	{
		FArcLines Lines;
		const FVector2D Origin = FVector2D::ZeroVector;
		Lines.Arc = ArcPoints(Origin, Radius, FromDeg, ToDeg);
		if (TickStep > 0.f)
		{
			const float Sign = ToDeg >= FromDeg ? 1.f : -1.f;
			const float Span = FMath::Abs(ToDeg - FromDeg);
			const float Out = bTicksIn ? -1.f : 1.f;
			for (int32 Index = 0; Index * TickStep <= Span + 1e-3f; ++Index)
			{
				const float Angle = FromDeg + Sign * Index * TickStep;
				const bool bLong = LongEvery > 0 && Index % LongEvery == 0;
				TArray<FVector2D> Tick = Spoke(Origin, Angle, Radius, Radius + Out * (bLong ? ArcTickLong : ArcTickShort));
				(bLong ? Lines.Longs : Lines.Shorts).Add(MoveTemp(Tick));
			}
		}
		for (const FVector2D& Point : Lines.Arc)
		{
			Lines.Bounds += Point;
		}
		for (const TArray<TArray<FVector2D>>* Set : { &Lines.Shorts, &Lines.Longs })
		{
			for (const TArray<FVector2D>& Tick : *Set)
			{
				for (const FVector2D& Point : Tick)
				{
					Lines.Bounds += Point;
				}
			}
		}
		return Lines;
	}

	void Shift(TArray<FVector2D>& Points, const FVector2D& By)
	{
		for (FVector2D& Point : Points)
		{
			Point += By;
		}
	}
}

// ---------------------------------------------------------------------------
// The halo
// ---------------------------------------------------------------------------

float LooterReticle::HaloExtent(float Radius)
{
	// The long ticks' tips, their half width and edge, then the pad.
	return Radius + HaloTickGap(Radius) + HaloTickLength(Radius) + HaloLongExtra(Radius) + 2.5f + Pad;
}

FPaintedIcon LooterReticle::Halo(float Radius, float TickStep, int32 LongEvery, bool bCross, float GoldFrom, float GoldTo)
{
	const float Extent = HaloExtent(Radius);
	const FVector2D Centre(Extent, Extent);
	FInk Ink;

	Ink.Line({ ArcPoints(Centre, Radius, 0.f, 360.f) }, HaloRingWidth(Radius), Color::SaltLine(), 1.f, EdgeStrong);
	AddHaloTicks(Ink, Centre, Radius, TickStep, LongEvery, 0.f, 360.f);

	if (bCross)
	{
		// The sight's cross hairs, from inside the ring toward the middle, the middle left open (the pause mockup: 44 to 86
		// of 112).
		TArray<TArray<FVector2D>> Cross;
		for (int32 Quarter = 0; Quarter < 4; ++Quarter)
		{
			Cross.Add(Spoke(Centre, Quarter * 90.f, Radius * 0.39f, Radius * 0.77f));
		}
		Ink.Line(MoveTemp(Cross), 1.5f, Color::SaltLine(), 0.6f, EdgeSoft);
	}
	if (!FMath::IsNearlyEqual(GoldFrom, GoldTo))
	{
		Ink.Line({ ArcPoints(Centre, Radius * 0.875f, GoldFrom, GoldTo) }, FMath::Max(3.f, Radius * 0.04f), Color::SoulGold(), 1.f, EdgeStrong);
	}
	return Ink.Finish(FVector2D(Extent * 2.f, Extent * 2.f));
}

FPaintedIcon LooterReticle::HaloTicks(float Radius, float TickStep, int32 LongEvery, float FromDeg, float ToDeg)
{
	const float Extent = HaloExtent(Radius);
	FInk Ink;
	AddHaloTicks(Ink, FVector2D(Extent, Extent), Radius, TickStep, LongEvery, FromDeg, ToDeg);
	return Ink.Finish(FVector2D(Extent * 2.f, Extent * 2.f));
}

// ---------------------------------------------------------------------------
// The ruler
// ---------------------------------------------------------------------------

float LooterReticle::RulerLineY(bool bDots)
{
	// The dotted rule's ticks cross the line, 8 each way; the plain one's hang 12 down from it.
	return bDots ? Pad + 8.f : Pad;
}

float LooterReticle::RulerHeight(bool bDots)
{
	return bDots ? (Pad + 8.f) * 2.f : Pad + 12.f + Pad;
}

FPaintedIcon LooterReticle::Ruler(float Length, float Step, int32 LongEvery, bool bDots, const FLinearColor& Line)
{
	const float Y = RulerLineY(bDots);
	FInk Ink;
	Ink.Line({ { FVector2D(Pad, Y), FVector2D(Pad + Length, Y) } }, 1.6f, Line, bDots ? 0.5f : 0.55f, EdgeSoft);

	TArray<TArray<FVector2D>> Shorts;
	TArray<TArray<FVector2D>> Longs;
	TArray<TArray<FVector2D>> Dots;
	if (Step > 0.f)
	{
		for (int32 Index = 0; Index * Step <= Length + 0.1f; ++Index)
		{
			const float X = Pad + Index * Step;
			const bool bLong = LongEvery > 0 && Index % LongEvery == 0;
			if (bDots)
			{
				// A single point strokes as a dot.
				(bLong ? Longs : Dots).Add(bLong ? TArray<FVector2D>{ FVector2D(X, Y - 8.f), FVector2D(X, Y + 8.f) } : TArray<FVector2D>{ FVector2D(X, Y) });
			}
			else
			{
				(bLong ? Longs : Shorts).Add({ FVector2D(X, Y), FVector2D(X, Y + (bLong ? 12.f : 5.f)) });
			}
		}
	}
	Ink.Line(MoveTemp(Shorts), 1.2f, Line, 0.45f, 0.f);
	Ink.Line(MoveTemp(Dots), 6.f, Line, 0.7f, EdgeSoft);
	Ink.Line(MoveTemp(Longs), 2.f, Line, bDots ? 0.8f : 0.85f, EdgeSoft);
	return Ink.Finish(FVector2D(Length + Pad * 2.f, RulerHeight(bDots)));
}

// ---------------------------------------------------------------------------
// The bracket
// ---------------------------------------------------------------------------

float LooterReticle::BracketEdgeX(float Height)
{
	return Pad + BracketOut(Height);
}

float LooterReticle::BracketWidth(float Height)
{
	const float Bow = BracketRadius(Height) * (1.f - FMath::Cos(FMath::DegreesToRadians(BracketHalfAngle)));
	return BracketEdgeX(Height) + FMath::Max(BracketTickIn, Bow + 2.f) + Pad;
}

FPaintedIcon LooterReticle::Bracket(float Height, bool bRight, const FLinearColor& Line)
{
	const float Radius = BracketRadius(Height);
	const float Edge = BracketEdgeX(Height);
	const float Width = BracketWidth(Height);
	const float MiddleY = Pad + Height * 0.5f;
	TArray<FVector2D> Bow = ArcPoints(FVector2D(Edge + Radius, MiddleY), Radius, 180.f - BracketHalfAngle, 180.f + BracketHalfAngle);
	TArray<FVector2D> Tick = { FVector2D(Edge - BracketOut(Height), MiddleY), FVector2D(Edge + BracketTickIn, MiddleY) };
	if (bRight)
	{
		for (TArray<FVector2D>* Points : { &Bow, &Tick })
		{
			for (FVector2D& Point : *Points)
			{
				Point.X = Width - Point.X;
			}
		}
	}
	FInk Ink;
	Ink.Line({ MoveTemp(Bow) }, 4.f, Line, 1.f, EdgeStrong);
	Ink.Line({ MoveTemp(Tick) }, 2.5f, Line, 1.f, EdgeStrong);
	return Ink.Finish(FVector2D(Width, Height + Pad * 2.f));
}

// ---------------------------------------------------------------------------
// The small reticle
// ---------------------------------------------------------------------------

float LooterReticle::SmallReticleExtent(float Radius)
{
	// The ticks reach 5 past the circle; their half width and edge, then the pad.
	return Radius + 5.f + 2.75f + Pad;
}

FPaintedIcon LooterReticle::SmallReticle(float Radius, const FLinearColor& Line)
{
	const float Extent = SmallReticleExtent(Radius);
	const FVector2D Centre(Extent, Extent);
	TArray<TArray<FVector2D>> Arcs;
	TArray<TArray<FVector2D>> Ticks;
	for (int32 Quarter = 0; Quarter < 4; ++Quarter)
	{
		Arcs.Add(ArcPoints(Centre, Radius, Quarter * 90.f + 14.f, Quarter * 90.f + 76.f));
		Ticks.Add(Spoke(Centre, Quarter * 90.f, Radius + 5.f, FMath::Max(Radius - 7.f, 2.f)));
	}
	FInk Ink;
	Ink.Line(MoveTemp(Arcs), 3.f, Line, 1.f, EdgeStrong);
	Ink.Line(MoveTemp(Ticks), 2.5f, Line, 1.f, EdgeStrong);
	// The middle dot: a single point strokes as a dot.
	Ink.Line(TArray<TArray<FVector2D>>{ TArray<FVector2D>{ Centre } }, 7.f, Line, 1.f, EdgeStrong);
	return Ink.Finish(FVector2D(Extent * 2.f, Extent * 2.f));
}

// ---------------------------------------------------------------------------
// The range arc
// ---------------------------------------------------------------------------

FVector2D LooterReticle::ArcScaleCentre(float Radius, float FromDeg, float ToDeg, float TickStep, int32 LongEvery, bool bTicksIn)
{
	const FArcLines Lines = BuildArc(Radius, FromDeg, ToDeg, TickStep, LongEvery, bTicksIn);
	// The circle's centre is the lines' origin: it moves with them into the view box.
	return FVector2D(ArcMargin, ArcMargin) - Lines.Bounds.Min;
}

FPaintedIcon LooterReticle::ArcScale(float Radius, float FromDeg, float ToDeg, float TickStep, int32 LongEvery, bool bTicksIn,
	const FLinearColor& Line)
{
	FArcLines Lines = BuildArc(Radius, FromDeg, ToDeg, TickStep, LongEvery, bTicksIn);
	const FVector2D By = FVector2D(ArcMargin, ArcMargin) - Lines.Bounds.Min;
	Shift(Lines.Arc, By);
	for (TArray<TArray<FVector2D>>* Set : { &Lines.Shorts, &Lines.Longs })
	{
		for (TArray<FVector2D>& Tick : *Set)
		{
			Shift(Tick, By);
		}
	}
	FInk Ink;
	Ink.Line({ MoveTemp(Lines.Arc) }, 2.f, Line, 0.55f, EdgeStrong);
	Ink.Line(MoveTemp(Lines.Shorts), 1.2f, Line, 0.45f, 0.f);
	Ink.Line(MoveTemp(Lines.Longs), 2.f, Line, 0.85f, EdgeSoft);
	return Ink.Finish(Lines.Bounds.GetSize() + FVector2D(ArcMargin, ArcMargin) * 2.f);
}
