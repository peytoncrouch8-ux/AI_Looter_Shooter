// The mission tracker's pictures: the medal, the keycap and the marks (rings that turn into checked discs once done),
// each drawn once into a shared texture through the kit's painted pictures. View-box units are the pixels they show at.

#include "UI/HUD/HudMissionTrackerShapes.h"
#include "UI/Style/LooterUIStyle.h"

using namespace LooterUI;

namespace
{
	/** The pictures are drawn at twice the size they show at, so they stay crisp (the keycap's are the exception, below). */
	constexpr float ShapesPixelsPerUnit = 2.f;

	/** The keycap: its top-right and bottom-left corners cut this far, inside a cyan edge this wide. */
	constexpr float KeycapCut = 6.f;
	constexpr float KeycapEdge = 1.4f;
	/**
	 * Its ends stay as drawn while its middle stretches to the key's width: each end is this many pixels wide (the cut
	 * corner and the round joins fit inside, the middle is a plain run of edge). Slate draws a Box brush's end at
	 * TextureWidth x Margin pixels showing the same share of the texture, so an end is only the size it was drawn at when
	 * one texel is one pixel: the keycap's pictures are made at 1x, not at the 2x of the others.
	 */
	constexpr float KeycapEndWidth = 8.f;
	constexpr float KeycapPixelsPerUnit = 1.f;

	// --- The marks: boxes, and the rings' and discs' radii in them ---

	/**
	 * The objective's ring, and its checked disc (a bigger box, room for the pop): as tall as its line's capitals and more
	 * (the user, 2026-10-31: "should be bigger"; it was a 5.8 ring in a 16 box).
	 */
	constexpr float ObjectiveBox = 28.f;
	constexpr float ObjectiveRadius = 10.f;
	constexpr float ObjectiveCheckBox = 32.f;
	constexpr float ObjectiveCheckRadius = 12.5f;
	/** The optional objectives' dashed ring, and their checked disc: the objective's size (the user, 2026-10-31; was 12/16). */
	constexpr float OptionalBox = ObjectiveBox;
	constexpr float OptionalRadius = ObjectiveRadius;
	constexpr float OptionalCheckBox = ObjectiveCheckBox;
	constexpr float OptionalCheckRadius = ObjectiveCheckRadius;
	/** The dashed ring's dashes (12 on the big ring) and how much of each one's share they fill. */
	constexpr int32 OptionalDashes = 12;
	constexpr float OptionalDashShare = 0.62f;

	/** A circle as a polygon, clockwise on screen from the right. */
	TArray<FVector2D> ShapesCircle(const FVector2D& Center, float Radius, int32 Points = 72)
	{
		TArray<FVector2D> Circle;
		Circle.Reserve(Points + 1);
		for (int32 Index = 0; Index < Points; ++Index)
		{
			const float Angle = 2.f * UE_PI * static_cast<float>(Index) / static_cast<float>(Points);
			Circle.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Circle;
	}

	/** The polyline closed on itself, for a stroke round a shape. */
	TArray<FVector2D> ShapesClosed(TArray<FVector2D> Points)
	{
		// A copy first: adding an element of the array to itself could read it after the array has moved.
		const FVector2D First = Points[0];
		Points.Add(First);
		return Points;
	}

	/** The upper half of a circle, from its left end over the top to its right end (the light half of a disc). */
	TArray<FVector2D> ShapesUpperHalf(const FVector2D& Center, float Radius)
	{
		constexpr int32 Points = 36;
		TArray<FVector2D> Half;
		Half.Reserve(Points + 1);
		for (int32 Index = 0; Index <= Points; ++Index)
		{
			const float Angle = UE_PI + UE_PI * static_cast<float>(Index) / static_cast<float>(Points);
			Half.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Half;
	}

	FPaintLayer ShapesFill(TArray<TArray<FVector2D>> Fills, const FLinearColor& LayerColor, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Fills = MoveTemp(Fills);
		Layer.Color = LayerColor;
		Layer.Opacity = Opacity;
		return Layer;
	}

	FPaintLayer ShapesStroke(TArray<TArray<FVector2D>> Lines, float Width, const FLinearColor& LayerColor, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Strokes = MoveTemp(Lines);
		Layer.StrokeWidth = Width;
		Layer.Color = LayerColor;
		Layer.Opacity = Opacity;
		return Layer;
	}

	/**
	 * Done: a disc in the experience bar's two cyan halves (the light one over its top), an ink rim and an ink check across
	 * it, as the level gem shows its number in ink. Radius sets the size; the check scales with it.
	 */
	FPaintedIcon MakeCheckDisc(float Box, float Radius)
	{
		const FVector2D Center(Box * 0.5f);
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(Box);
		// A soft dark ring outside the rim keeps it readable over sky.
		Icon.Layers.Add(ShapesStroke({ ShapesClosed(ShapesCircle(Center, Radius + 0.9f)) }, 1.6f, Color::Ink(), 0.45f));
		Icon.Layers.Add(ShapesFill({ ShapesCircle(Center, Radius) }, Color::XPDark()));
		Icon.Layers.Add(ShapesFill({ ShapesUpperHalf(Center, Radius) }, Color::XPLight()));
		Icon.Layers.Add(ShapesStroke({ ShapesClosed(ShapesCircle(Center, Radius)) }, FMath::Max(1.2f, Radius * 0.21f), Color::Ink()));
		const float Unit = Radius / 7.6f;
		const TArray<FVector2D> Check = { Center + FVector2D(-3.6f, 0.2f) * Unit, Center + FVector2D(-1.f, 2.8f) * Unit,
			Center + FVector2D(3.7f, -2.6f) * Unit };
		Icon.Layers.Add(ShapesStroke({ Check }, FMath::Max(1.5f, 2.4f * Unit), Color::Ink()));
		return Icon;
	}

	/** The objective to do: an orange ring over a thick dark one, as the kit outlines a shape with no panel behind it. */
	FPaintedIcon MakeObjectiveRing()
	{
		const FVector2D Center(ObjectiveBox * 0.5f);
		const TArray<FVector2D> Ring = ShapesClosed(ShapesCircle(Center, ObjectiveRadius));
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(ObjectiveBox);
		Icon.Layers.Add(ShapesStroke({ Ring }, 5.2f, Color::Ink()));
		Icon.Layers.Add(ShapesStroke({ Ring }, 2.8f, Color::Accent()));
		return Icon;
	}

	/** An optional objective: a dashed cyan ring over dark dashes (the old hollow diamond's colours). */
	FPaintedIcon MakeOptionalRing()
	{
		const FVector2D Center(OptionalBox * 0.5f);
		TArray<TArray<FVector2D>> Dashes;
		for (int32 Dash = 0; Dash < OptionalDashes; ++Dash)
		{
			const float Start = 2.f * UE_PI * static_cast<float>(Dash) / static_cast<float>(OptionalDashes);
			const float Length = 2.f * UE_PI / static_cast<float>(OptionalDashes) * OptionalDashShare;
			TArray<FVector2D>& Arc = Dashes.AddDefaulted_GetRef();
			for (int32 Point = 0; Point <= 4; ++Point)
			{
				const float Angle = Start + Length * static_cast<float>(Point) / 4.f;
				Arc.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * OptionalRadius);
			}
		}
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(OptionalBox);
		Icon.Layers.Add(ShapesStroke(Dashes, 5.f, Color::Ink(), 0.7f));
		Icon.Layers.Add(ShapesStroke(Dashes, 2.4f, Color::Hairline(), 0.9f));
		return Icon;
	}

	FVector2D ShapesMedalCenter()
	{
		return FVector2D(HudTrackerShapes::MedalBox * 0.5f);
	}

	/**
	 * The medal's metal: an ink rim and a gunmetal ring lit from the top. Both are rings, open where the glass is, so the
	 * glass alone fades with the UI transparency setting.
	 */
	FPaintedIcon MakeMedalMetal()
	{
		const FVector2D Center = ShapesMedalCenter();
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(HudTrackerShapes::MedalBox);
		Icon.Layers.Add(ShapesFill({ ShapesCircle(Center, 16.f), ShapesCircle(Center, 12.f) }, Color::Ink()));
		FPaintLayer Ring = ShapesFill({ ShapesCircle(Center, 14.6f), ShapesCircle(Center, 12.f) }, Color::BarMetalHi());
		Ring.GradientTo = Color::BarMetalLow();
		Ring.GradientStart = Center - FVector2D(0.f, 14.6f);
		Ring.GradientEnd = Center + FVector2D(0.f, 14.6f);
		Icon.Layers.Add(Ring);
		return Icon;
	}

	/** The dark glass inside the ring: a background. */
	FPaintedIcon MakeMedalGlass()
	{
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(HudTrackerShapes::MedalBox);
		Icon.Layers.Add(ShapesFill({ ShapesCircle(ShapesMedalCenter(), 12.f) }, Color::ScreenBg().CopyWithNewOpacity(0.9f)));
		return Icon;
	}

	/**
	 * The cyan hairline inside the ring and the ranger's star: orange, five points 10.4 px out with inner corners 4.5 px
	 * in, a 1.5 px ink edge and a light line along its upper-left edges.
	 */
	FPaintedIcon MakeMedalStar()
	{
		const FVector2D Center = ShapesMedalCenter();
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(HudTrackerShapes::MedalBox);
		Icon.Layers.Add(ShapesStroke({ ShapesClosed(ShapesCircle(Center, 12.4f)) }, 1.f, Color::Hairline(), 0.6f));
		TArray<FVector2D> Star;
		for (int32 Index = 0; Index < 10; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(-90.f + 36.f * static_cast<float>(Index));
			const float Radius = Index % 2 == 0 ? 10.4f : 4.5f;
			Star.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		Icon.Layers.Add(ShapesFill({ Star }, Color::Accent()));
		Icon.Layers.Add(ShapesStroke({ ShapesClosed(Star) }, 1.5f, Color::Ink()));
		Icon.Layers.Add(ShapesStroke({ { Center + FVector2D(-7.6f, -2.6f), Center + FVector2D(-2.1f, -2.9f), Center + FVector2D(0.f, -8.1f) } },
			1.f, FLinearColor::White, 0.55f));
		return Icon;
	}

	/** The keycap's outline with its top-right and bottom-left corners cut, Inset in from its box. */
	TArray<FVector2D> KeycapOutline(float Inset)
	{
		// Each cut moves in along its diagonal, so its ends sit where the inset edges meet it, a little further along.
		constexpr float Size = HudTrackerShapes::KeycapSize;
		const float Low = Inset;
		const float High = Size - Inset;
		const float CutInset = KeycapCut + Inset * (UE_SQRT_2 - 1.f);
		return { FVector2D(Low, Low), FVector2D(Size - CutInset, Low), FVector2D(High, CutInset), FVector2D(High, High),
			FVector2D(CutInset, High), FVector2D(Low, Size - CutInset) };
	}

	/** The keycap's plate, dark blue from the top down: a background. */
	FPaintedIcon MakeKeycapPlate()
	{
		constexpr float Size = HudTrackerShapes::KeycapSize;
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(Size);
		FPaintLayer Plate = ShapesFill({ KeycapOutline(0.f) }, Color::KeycapTop());
		Plate.GradientTo = Color::KeycapBottom();
		Plate.GradientStart = FVector2D(Size * 0.5f, 0.f);
		Plate.GradientEnd = FVector2D(Size * 0.5f, Size);
		Icon.Layers.Add(Plate);
		return Icon;
	}

	/** The keycap's cyan edge, just inside its outline. */
	FPaintedIcon MakeKeycapEdge()
	{
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(HudTrackerShapes::KeycapSize);
		Icon.Layers.Add(ShapesStroke({ ShapesClosed(KeycapOutline(KeycapEdge * 0.5f)) }, KeycapEdge, Color::Hairline()));
		return Icon;
	}

	/**
	 * A keycap picture drawn as a box: its ends as drawn, its middle stretched to the key's width. The picture's texture is
	 * KeycapSize texels across (1x), so the margin share below is KeycapEndWidth texels and KeycapEndWidth pixels at once.
	 */
	FSlateBrush KeycapBrush(FName Name, const FPaintedIcon& Icon)
	{
		constexpr float Size = HudTrackerShapes::KeycapSize;
		FSlateBrush Brush = PaintedIconBrush(Name, Icon, KeycapPixelsPerUnit, FVector2D(Size));
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(KeycapEndWidth / Size, 0.f);
		return Brush;
	}
}

FVector2D HudTrackerShapes::MarkSize(EHudTrackerMark Mark)
{
	switch (Mark)
	{
	case EHudTrackerMark::Objective: return FVector2D(ObjectiveBox);
	case EHudTrackerMark::ObjectiveDone: return FVector2D(ObjectiveCheckBox);
	case EHudTrackerMark::Optional: return FVector2D(OptionalBox);
	case EHudTrackerMark::OptionalDone: return FVector2D(OptionalCheckBox);
	default: return FVector2D(ObjectiveBox);
	}
}

FSlateBrush HudTrackerShapes::MarkBrush(EHudTrackerMark Mark)
{
	static const FPaintedIcon ObjectiveRing = MakeObjectiveRing();
	static const FPaintedIcon ObjectiveCheck = MakeCheckDisc(ObjectiveCheckBox, ObjectiveCheckRadius);
	static const FPaintedIcon OptionalRing = MakeOptionalRing();
	static const FPaintedIcon OptionalCheck = MakeCheckDisc(OptionalCheckBox, OptionalCheckRadius);
	const FVector2D Size = MarkSize(Mark);
	switch (Mark)
	{
	case EHudTrackerMark::ObjectiveDone: return PaintedIconBrush(TEXT("HudTrackerCheck"), ObjectiveCheck, ShapesPixelsPerUnit, Size);
	case EHudTrackerMark::Optional: return PaintedIconBrush(TEXT("HudTrackerOptionalRing"), OptionalRing, ShapesPixelsPerUnit, Size);
	case EHudTrackerMark::OptionalDone: return PaintedIconBrush(TEXT("HudTrackerOptionalCheck"), OptionalCheck, ShapesPixelsPerUnit, Size);
	default: return PaintedIconBrush(TEXT("HudTrackerRing"), ObjectiveRing, ShapesPixelsPerUnit, Size);
	}
}

FSlateBrush HudTrackerShapes::MedalMetalBrush()
{
	static const FPaintedIcon Icon = MakeMedalMetal();
	return PaintedIconBrush(TEXT("HudTrackerMedalMetal"), Icon, ShapesPixelsPerUnit, FVector2D(MedalBox));
}

FSlateBrush HudTrackerShapes::MedalGlassBrush()
{
	static const FPaintedIcon Icon = MakeMedalGlass();
	return PaintedIconBrush(TEXT("HudTrackerMedalGlass"), Icon, ShapesPixelsPerUnit, FVector2D(MedalBox));
}

FSlateBrush HudTrackerShapes::MedalStarBrush()
{
	static const FPaintedIcon Icon = MakeMedalStar();
	return PaintedIconBrush(TEXT("HudTrackerMedalStar"), Icon, ShapesPixelsPerUnit, FVector2D(MedalBox));
}

FSlateBrush HudTrackerShapes::KeycapPlateBrush()
{
	static const FPaintedIcon Icon = MakeKeycapPlate();
	return KeycapBrush(TEXT("HudTrackerKeycapPlate"), Icon);
}

FSlateBrush HudTrackerShapes::KeycapEdgeBrush()
{
	static const FPaintedIcon Icon = MakeKeycapEdge();
	return KeycapBrush(TEXT("HudTrackerKeycapEdge"), Icon);
}
