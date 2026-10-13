#include "UI/Style/MissionMarkIcons.h"

using namespace LooterUI;

namespace
{
	/** How far the dark ink line reaches past the glyph's edge, in view-box units (a 24-unit glyph). */
	constexpr double InkReach = 1.25;

	/** A circle's outline as a polygon; closed (its first point again at the end) for a stroke. */
	TArray<FVector2D> Circle(const FVector2D& Center, double Radius, bool bClosed, int32 Sides = 24)
	{
		TArray<FVector2D> Points;
		for (int32 Step = 0; Step < Sides; ++Step)
		{
			const double Angle = UE_TWO_PI * Step / Sides;
			Points.Add(Center + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
		}
		if (bClosed)
		{
			Points.Add(Points[0]);
		}
		return Points;
	}

	/** A point on a circle, at Degrees (0 right, 90 down: the screen's y runs down). */
	FVector2D OnCircle(const FVector2D& Center, double Radius, double Degrees)
	{
		const double Radians = FMath::DegreesToRadians(Degrees);
		return Center + Radius * FVector2D(FMath::Cos(Radians), FMath::Sin(Radians));
	}

	/** The glyph's top colour and its underside's: gold, or white, lit from above. */
	FLinearColor TopColor(bool bMain)
	{
		return bMain ? Color::CurseLight() : Color::Cloud();
	}

	FLinearColor BottomColor(bool bMain)
	{
		return bMain ? Color::Focus() : Color::NumberDim();
	}

	/** The ink line under a glyph: its shapes filled and their edges stroked wide, so the fill sits inside a dark rim. */
	FPaintLayer InkLayer(const TArray<TArray<FVector2D>>& Fills, const TArray<TArray<FVector2D>>& Strokes, float GlyphStroke)
	{
		FPaintLayer Ink;
		Ink.Color = Color::IconInk();
		Ink.Fills = Fills;
		for (const TArray<FVector2D>& Shape : Fills)
		{
			TArray<FVector2D> Edge = Shape;
			Edge.Add(Shape[0]);
			Ink.Strokes.Add(MoveTemp(Edge));
		}
		Ink.Strokes.Append(Strokes);
		// A fill's edge stroked twice the reach wide pokes out by the reach; a stroke's own width grows by the same.
		Ink.StrokeWidth = Strokes.IsEmpty() ? static_cast<float>(InkReach * 2.0) : GlyphStroke + static_cast<float>(InkReach * 2.0);
		return Ink;
	}

	/** The glyph's body: its light top running to its deeper underside. */
	FPaintLayer BodyLayer(bool bMain, const TArray<TArray<FVector2D>>& Fills, const TArray<TArray<FVector2D>>& Strokes, float GlyphStroke)
	{
		FPaintLayer Body;
		Body.Fills = Fills;
		Body.Strokes = Strokes;
		Body.StrokeWidth = GlyphStroke;
		Body.Color = TopColor(bMain);
		Body.GradientTo = BottomColor(bMain);
		Body.GradientStart = FVector2D(12.0, 3.0);
		Body.GradientEnd = FVector2D(12.0, 21.5);
		return Body;
	}

	/** A glint down its upper left, as the inked icons are lit. */
	FPaintLayer GlintLayer(TArray<FVector2D> Line, float Width)
	{
		FPaintLayer Glint;
		Glint.Strokes.Add(MoveTemp(Line));
		Glint.StrokeWidth = Width;
		Glint.Color = FLinearColor::White;
		Glint.Opacity = 0.55f;
		return Glint;
	}

	/** !: a bar rounded at its top, narrowing to its foot, and the dot under it. */
	FPaintedIcon MakeBang(bool bMain)
	{
		TArray<FVector2D> Bar;
		const FVector2D Crown(12.0, 5.0);
		constexpr double CrownRadius = 2.8;
		for (int32 Step = 0; Step <= 8; ++Step)
		{
			// Over the top from the left (180 degrees) to the right (360): up is -y.
			Bar.Add(OnCircle(Crown, CrownRadius, 180.0 + 180.0 * Step / 8));
		}
		Bar.Add(FVector2D(13.15, 14.4));
		Bar.Add(FVector2D(12.7, 15.3));
		Bar.Add(FVector2D(11.3, 15.3));
		Bar.Add(FVector2D(10.85, 14.4));
		const TArray<TArray<FVector2D>> Fills = { Bar, Circle(FVector2D(12.0, 19.4), 2.35, false) };

		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(24.f, 24.f);
		Icon.Layers.Add(InkLayer(Fills, {}, 0.f));
		Icon.Layers.Add(BodyLayer(bMain, Fills, {}, 0.f));
		Icon.Layers.Add(GlintLayer({ FVector2D(10.7, 5.2), FVector2D(11.25, 11.6) }, 0.9f));
		return Icon;
	}

	/** ?: a hook drawn as one bold stroke, round at both ends, and the dot under it. */
	FPaintedIcon MakeQuery(bool bMain)
	{
		constexpr float HookWidth = 3.3f;
		const FVector2D Bowl(12.0, 8.1);
		constexpr double BowlRadius = 4.3;
		TArray<FVector2D> Hook;
		for (int32 Step = 0; Step <= 11; ++Step)
		{
			// From the left of the bowl, a little under its middle, over the top and round to the right.
			Hook.Add(OnCircle(Bowl, BowlRadius, 160.0 + 220.0 * Step / 11));
		}
		Hook.Add(FVector2D(15.2, 11.1));
		Hook.Add(FVector2D(13.7, 12.2));
		Hook.Add(FVector2D(12.5, 13.2));
		Hook.Add(FVector2D(12.1, 14.4));
		Hook.Add(FVector2D(12.1, 15.0));
		const TArray<TArray<FVector2D>> Strokes = { Hook };
		const TArray<TArray<FVector2D>> Fills = { Circle(FVector2D(12.1, 19.4), 2.35, false) };

		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(24.f, 24.f);
		// The hook's ink and the dot's ink go down as two layers: the hook's stroke is wider than the dot's rim.
		Icon.Layers.Add(InkLayer({}, Strokes, HookWidth));
		Icon.Layers.Add(InkLayer(Fills, {}, 0.f));
		Icon.Layers.Add(BodyLayer(bMain, {}, Strokes, HookWidth));
		Icon.Layers.Add(BodyLayer(bMain, Fills, {}, 0.f));
		TArray<FVector2D> Glint;
		for (int32 Step = 0; Step <= 4; ++Step)
		{
			Glint.Add(OnCircle(Bowl, BowlRadius - 0.55, 205.0 + 55.0 * Step / 4));
		}
		Icon.Layers.Add(GlintLayer(MoveTemp(Glint), 0.8f));
		return Icon;
	}

	/** The texture's resolution for a mark drawn at Size: about twice its pixels, in few steps so the textures are shared. */
	float PixelsPerUnitFor(const FVector2D& Size)
	{
		return FMath::Max(2.f, FMath::CeilToFloat(static_cast<float>(Size.GetMax()) * 2.f / 24.f));
	}
}

namespace MissionMarkIcons
{
	FLinearColor Color(bool bMain)
	{
		return bMain ? LooterUI::Color::Focus() : LooterUI::Color::Cloud();
	}

	FLinearColor ShadeColor(bool bMain)
	{
		return bMain ? LooterUI::Color::Accent() : LooterUI::Color::IconShade();
	}

	const FPaintedIcon& Icon(EMark Mark, bool bMain)
	{
		static const FPaintedIcon BangMain = MakeBang(true);
		static const FPaintedIcon BangSide = MakeBang(false);
		static const FPaintedIcon QueryMain = MakeQuery(true);
		static const FPaintedIcon QuerySide = MakeQuery(false);
		// The user's call (2026-10-13): a ? marks a mission to pick up, a ! one to turn in (Borderlands has them the other way).
		if (Mark == EMark::TurnIn)
		{
			return bMain ? BangMain : BangSide;
		}
		return bMain ? QueryMain : QuerySide;
	}

	FSlateBrush MarkBrush(EMark Mark, bool bMain, const FVector2D& Size, const FLinearColor& Tint)
	{
		const TCHAR* Name = Mark == EMark::TurnIn ? (bMain ? TEXT("MissionMark_TurnInMain") : TEXT("MissionMark_TurnInSide"))
			: (bMain ? TEXT("MissionMark_OfferMain") : TEXT("MissionMark_OfferSide"));
		return PaintedIconBrush(FName(Name), Icon(Mark, bMain), PixelsPerUnitFor(Size), Size, Tint);
	}

	FSlateBrush HaloBrush(bool bMain, const FVector2D& Size, float Alpha)
	{
		const FLinearColor Glow = Color(bMain);
		return GlowBrush(Size, FLinearColor(Glow.R, Glow.G, Glow.B, Alpha));
	}

	const TCHAR* Glyph(EMark Mark)
	{
		return Mark == EMark::TurnIn ? TEXT("!") : TEXT("?");
	}
}
