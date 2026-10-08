// UHudWeaponSlotsWidget's pictures: the slot circle's rings, rarity arc and dashes, and the key tab's plate and edge, drawn
// once from vector shapes into shared textures (the mockup's measures). How the widget lays them out and paints them per
// slot is in HudWeaponSlotsWidget.cpp.

#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/Style/LooterUIStyle.h"

using namespace LooterUI;

namespace
{
	constexpr float SlotDiameter = UHudWeaponSlotsWidget::SlotDiameter;
	constexpr float SlotRadius = SlotDiameter * 0.5f;
	/** Points round a whole circle: at the drawn size (twice that in the texture) the facets stay well under a pixel. */
	constexpr int32 CircleSegments = 64;
	/** The textures are drawn at twice the size they show at, so they stay crisp. */
	constexpr float SlotPixelsPerUnit = 2.f;

	// The circle's layers, as radii from its centre (1080p pixels, the mockup's). The rim reaches under the gunmetal ring,
	// and the disc a little under the ring's inner edge, so no seam shows between them.
	constexpr float RimOuter = 31.8f;
	constexpr float RimInner = 27.5f;
	constexpr float MetalOuter = 30.f;
	constexpr float MetalInner = 27.5f;
	constexpr float DiscRadius = 28.f;
	constexpr float HairlineRadius = 27.9f;
	constexpr float HairlineWidth = 1.f;
	/** The hairline's opacity: faint over the gunmetal ring's inner edge. */
	constexpr float HairlineOpacity = 0.55f;
	/** The rarity arc: a thick band along the circle's bottom, this many degrees of it (centred on the bottom). */
	constexpr float ArcRadius = 27.25f;
	constexpr float ArcWidth = 4.5f;
	constexpr float ArcSweep = 100.f;
	/** The ring round the slot in hand, over the rim and a little past it. */
	constexpr float AccentRadius = 31.6f;
	constexpr float AccentWidth = 3.f;
	/** That ring reaches past the circle, so its picture has this much room on every side (the widget scales it to match). */
	constexpr float AccentMargin = UHudWeaponSlotsWidget::AccentMargin;
	/** An empty slot's dashed ring: about 4.4 px of dash to 4.4 of gap. */
	constexpr float DashRadius = 28.f;
	constexpr float DashWidth = 2.f;
	constexpr int32 DashCount = 20;

	// The key tab: its size, the cut of its bottom-right corner and its edge.
	constexpr float TabWidth = UHudWeaponSlotsWidget::TabWidth;
	constexpr float TabHeight = UHudWeaponSlotsWidget::TabHeight;
	/** The tab's bottom-right corner is cut from this far down its right side to this far along its bottom. */
	constexpr float TabCutRight = 11.2f;
	constexpr float TabCutBottom = 17.16f;
	constexpr float TabEdgeWidth = 1.4f;
	/** The tab is small, so its pictures are drawn finer than the circle's. */
	constexpr float TabPixelsPerUnit = 3.f;

	/**
	 * Points round a circle of Radius about Center, from angle From to To in degrees (clockwise on screen from the right,
	 * so 90 is the bottom: the order fills and strokes take), both ends included.
	 */
	TArray<FVector2D> SlotArc(const FVector2D& Center, float Radius, float From, float To, int32 Segments)
	{
		TArray<FVector2D> Points;
		Points.Reserve(Segments + 1);
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(From, To, static_cast<float>(Index) / static_cast<float>(Segments)));
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Points;
	}

	/**
	 * The whole circle as a polygon. The last point (back at the start) is dropped: a fill's edge of almost no length has
	 * no direction, which would spoil the polygon's coverage.
	 */
	TArray<FVector2D> SlotCircle(const FVector2D& Center, float Radius)
	{
		TArray<FVector2D> Points = SlotArc(Center, Radius, 0.f, 360.f, CircleSegments);
		Points.Pop();
		return Points;
	}

	TArray<FVector2D> SlotClosed(TArray<FVector2D> Points)
	{
		// A copy first: adding an element of the array to itself could read it after the array has moved.
		const FVector2D First = Points[0];
		Points.Add(First);
		return Points;
	}

	/** The circle's centre in its own picture. */
	const FVector2D SlotCenter(SlotRadius, SlotRadius);

	/** A picture the size of the slot: one ring, Radius to its middle and Width wide (or the disc of Radius, filled). */
	FVectorIcon SlotRing(float Radius, float Width, bool bFilled = false)
	{
		FVectorIcon Icon;
		Icon.ViewBox = FVector2D(SlotDiameter, SlotDiameter);
		Icon.StrokeWidth = Width;
		if (bFilled)
		{
			Icon.Fills.Add(SlotCircle(SlotCenter, Radius));
		}
		else
		{
			Icon.Strokes.Add(SlotClosed(SlotCircle(SlotCenter, Radius)));
		}
		return Icon;
	}

	const FVectorIcon& SlotDiscIcon() { static const FVectorIcon Icon = SlotRing(DiscRadius, 0.f, true); return Icon; }
	const FVectorIcon& SlotRimIcon() { static const FVectorIcon Icon = SlotRing((RimOuter + RimInner) * 0.5f, RimOuter - RimInner); return Icon; }
	const FVectorIcon& SlotHairlineIcon() { static const FVectorIcon Icon = SlotRing(HairlineRadius, HairlineWidth); return Icon; }

	/**
	 * The gunmetal ring, lit from the top like the player frame's medallion: the upper half runs from MetalHi to MetalMid,
	 * the lower from MetalLow to MetalDeep. The lower tones go under the whole ring first (flat MetalLow above the middle,
	 * nearly MetalMid), so the upper half's edge leaves no seam.
	 */
	const FPaintedIcon& SlotMetalIcon()
	{
		static const FPaintedIcon Icon = []()
		{
			const FVector2D& Center = SlotCenter;
			FPaintedIcon Ring;
			Ring.ViewBox = FVector2D(SlotDiameter, SlotDiameter);

			FPaintLayer Lower;
			// Even-odd: the inner circle cuts the hole.
			Lower.Fills.Add(SlotCircle(Center, MetalOuter));
			Lower.Fills.Add(SlotCircle(Center, MetalInner));
			Lower.Color = Color::MetalLow();
			Lower.GradientTo = Color::MetalDeep();
			Lower.GradientStart = Center;
			Lower.GradientEnd = Center + FVector2D(0.f, MetalOuter);
			Ring.Layers.Add(Lower);

			FPaintLayer Upper;
			TArray<FVector2D> HalfRing = SlotArc(Center, MetalOuter, 180.f, 360.f, CircleSegments / 2);
			HalfRing.Append(SlotArc(Center, MetalInner, 360.f, 180.f, CircleSegments / 2));
			Upper.Fills.Add(HalfRing);
			Upper.Color = Color::MetalHi();
			Upper.GradientTo = Color::MetalMid();
			Upper.GradientStart = Center - FVector2D(0.f, MetalOuter);
			Upper.GradientEnd = Center;
			Ring.Layers.Add(Upper);
			return Ring;
		}();
		return Icon;
	}

	/** The rarity arc along the circle's bottom. Its round ends are pulled in so the band, ends included, spans ArcSweep. */
	const FVectorIcon& SlotArcIcon()
	{
		static const FVectorIcon Icon = []()
		{
			const float CapDegrees = FMath::RadiansToDegrees(ArcWidth * 0.5f / ArcRadius);
			const float Half = ArcSweep * 0.5f - CapDegrees;
			FVectorIcon Band;
			Band.ViewBox = FVector2D(SlotDiameter, SlotDiameter);
			Band.StrokeWidth = ArcWidth;
			Band.Strokes.Add(SlotArc(SlotCenter, ArcRadius, 90.f - Half, 90.f + Half, 32));
			return Band;
		}();
		return Icon;
	}

	/** The accent ring round the slot in hand, in a picture AccentMargin bigger on every side than the slot. */
	const FVectorIcon& SlotAccentIcon()
	{
		static const FVectorIcon Icon = []()
		{
			const float Size = SlotDiameter + AccentMargin * 2.f;
			FVectorIcon Ring;
			Ring.ViewBox = FVector2D(Size, Size);
			Ring.StrokeWidth = AccentWidth;
			Ring.Strokes.Add(SlotClosed(SlotCircle(FVector2D(Size * 0.5f, Size * 0.5f), AccentRadius)));
			return Ring;
		}();
		return Icon;
	}

	/** An empty slot's ring: short dashes evenly round the circle, one centred on the bottom (so it's symmetric). */
	const FVectorIcon& SlotDashedIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Dashes;
			Dashes.ViewBox = FVector2D(SlotDiameter, SlotDiameter);
			Dashes.StrokeWidth = DashWidth;
			// Half of each step is dash, round ends included: the line between the ends is shorter by the stroke's width.
			constexpr float Step = 360.f / DashCount;
			const float CapDegrees = FMath::RadiansToDegrees(DashWidth * 0.5f / DashRadius);
			const float Sweep = FMath::Max(Step * 0.5f - CapDegrees * 2.f, 0.5f);
			for (int32 Index = 0; Index < DashCount; ++Index)
			{
				const float Middle = 90.f + Step * Index;
				Dashes.Strokes.Add(SlotArc(SlotCenter, DashRadius, Middle - Sweep * 0.5f, Middle + Sweep * 0.5f, 3));
			}
			return Dashes;
		}();
		return Icon;
	}

	/** The key tab's plate: a rectangle with its bottom-right corner cut, clockwise on screen from the top-left. */
	TArray<FVector2D> TabPlatePoints()
	{
		return { FVector2D(0.f, 0.f), FVector2D(TabWidth, 0.f), FVector2D(TabWidth, TabCutRight), FVector2D(TabCutBottom, TabHeight),
			FVector2D(0.f, TabHeight) };
	}

	/** A convex polygon (clockwise on screen) with every edge moved Distance inward. */
	TArray<FVector2D> SlotInset(const TArray<FVector2D>& Points, float Distance)
	{
		TArray<FVector2D> Result;
		Result.Reserve(Points.Num());
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FVector2D& Prev = Points[(Index + Points.Num() - 1) % Points.Num()];
			const FVector2D& Here = Points[Index];
			const FVector2D& Next = Points[(Index + 1) % Points.Num()];
			// Clockwise on screen (y down), the inside lies to the right of travel.
			const FVector2D In = (Here - Prev).GetSafeNormal();
			const FVector2D Out = (Next - Here).GetSafeNormal();
			const FVector2D NormalIn(-In.Y, In.X);
			const FVector2D NormalOut(-Out.Y, Out.X);
			Result.Add(Here + (NormalIn + NormalOut) * (Distance / (1.f + FVector2D::DotProduct(NormalIn, NormalOut))));
		}
		return Result;
	}

	const FVectorIcon& TabPlateIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Plate;
			Plate.ViewBox = FVector2D(TabWidth, TabHeight);
			Plate.Fills.Add(TabPlatePoints());
			return Plate;
		}();
		return Icon;
	}

	/** The tab's edge, drawn just inside the plate. */
	const FVectorIcon& TabEdgeIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Edge;
			Edge.ViewBox = FVector2D(TabWidth, TabHeight);
			Edge.StrokeWidth = TabEdgeWidth;
			Edge.Strokes.Add(SlotClosed(SlotInset(TabPlatePoints(), TabEdgeWidth * 0.5f)));
			return Edge;
		}();
		return Icon;
	}
}

FSlateBrush UHudWeaponSlotsWidget::PictureBrush(EHudSlotPicture Picture)
{
	const FVector2D CircleSize(SlotDiameter, SlotDiameter);
	const FVector2D TabSize(TabWidth, TabHeight);
	switch (Picture)
	{
	case EHudSlotPicture::Disc: return IconBrush(TEXT("HudSlotDisc"), SlotDiscIcon(), SlotPixelsPerUnit, CircleSize, FLinearColor::White);
	case EHudSlotPicture::Rim: return IconBrush(TEXT("HudSlotRim"), SlotRimIcon(), SlotPixelsPerUnit, CircleSize, FLinearColor::White);
	case EHudSlotPicture::Metal: return PaintedIconBrush(TEXT("HudSlotMetal"), SlotMetalIcon(), SlotPixelsPerUnit, CircleSize);
	case EHudSlotPicture::Arc: return IconBrush(TEXT("HudSlotArc"), SlotArcIcon(), SlotPixelsPerUnit, CircleSize, FLinearColor::White);
	case EHudSlotPicture::Hairline:
		return IconBrush(TEXT("HudSlotHairline"), SlotHairlineIcon(), SlotPixelsPerUnit, CircleSize, Color::Hairline().CopyWithNewOpacity(HairlineOpacity));
	case EHudSlotPicture::Dashed: return IconBrush(TEXT("HudSlotDashed"), SlotDashedIcon(), SlotPixelsPerUnit, CircleSize, FLinearColor::White);
	case EHudSlotPicture::AccentRing: return IconBrush(TEXT("HudSlotAccent"), SlotAccentIcon(), SlotPixelsPerUnit, CircleSize, FLinearColor::White);
	case EHudSlotPicture::TabPlate: return IconBrush(TEXT("HudSlotTab"), TabPlateIcon(), TabPixelsPerUnit, TabSize, FLinearColor::White);
	case EHudSlotPicture::TabEdge: return IconBrush(TEXT("HudSlotTabEdge"), TabEdgeIcon(), TabPixelsPerUnit, TabSize, FLinearColor::White);
	}
	return FSlateBrush();
}
