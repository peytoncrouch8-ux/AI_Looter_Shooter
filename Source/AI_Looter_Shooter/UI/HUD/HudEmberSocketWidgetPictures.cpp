// UHudEmberSocketWidget's pictures: the ring's disc, rim, gunmetal ring, hairline and ready ring (the weapon slots'
// measures, so the socket reads as one of them) and the key tab's plate and edge at any width, drawn once from vector
// shapes into shared textures. How the widget lays them out and paints them is in HudEmberSocketWidget.cpp.

#include "UI/HUD/HudEmberSocketWidget.h"
#include "UI/Style/LooterUIStyle.h"

using namespace LooterUI;

namespace
{
	constexpr float RingDiameter = UHudEmberSocketWidget::RingDiameter;
	constexpr float RingRadius = RingDiameter * 0.5f;
	constexpr int32 CircleSegments = 64;
	/** The textures are drawn at twice the size they show at (the tab's, finer, at three times). */
	constexpr float PixelsPerUnit = 2.f;
	constexpr float TabPixelsPerUnit = 3.f;

	// The ring's layers, as radii (the weapon slots' own).
	constexpr float RimOuter = 31.8f;
	constexpr float RimInner = 27.5f;
	constexpr float MetalOuter = UHudEmberSocketWidget::MetalOuter;
	constexpr float MetalInner = UHudEmberSocketWidget::MetalInner;
	constexpr float DiscRadius = 28.f;
	constexpr float HairlineRadius = 27.9f;
	constexpr float HairlineOpacity = 0.55f;
	/** The ready ring: over the rim and a little past it, in a picture AccentMargin bigger on every side. */
	constexpr float AccentRadius = 31.6f;
	constexpr float AccentWidth = 3.f;
	constexpr float AccentMargin = UHudEmberSocketWidget::AccentMargin;

	/** Points round a circle (degrees clockwise on screen from the right, both ends included). */
	TArray<FVector2D> SocketArc(const FVector2D& Center, float Radius, float From, float To, int32 Segments)
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

	/** The whole circle as a polygon, its closing point dropped (an edge of no length spoils a fill). */
	TArray<FVector2D> SocketCircle(const FVector2D& Center, float Radius)
	{
		TArray<FVector2D> Points = SocketArc(Center, Radius, 0.f, 360.f, CircleSegments);
		Points.Pop();
		return Points;
	}

	TArray<FVector2D> SocketClosed(TArray<FVector2D> Points)
	{
		const FVector2D First = Points[0];
		Points.Add(First);
		return Points;
	}

	const FVector2D SocketCenter(RingRadius, RingRadius);

	FVectorIcon SocketRing(float Radius, float Width, bool bFilled)
	{
		FVectorIcon Icon;
		Icon.ViewBox = FVector2D(RingDiameter, RingDiameter);
		Icon.StrokeWidth = Width;
		if (bFilled)
		{
			Icon.Fills.Add(SocketCircle(SocketCenter, Radius));
		}
		else
		{
			Icon.Strokes.Add(SocketClosed(SocketCircle(SocketCenter, Radius)));
		}
		return Icon;
	}

	const FVectorIcon& DiscIcon() { static const FVectorIcon Icon = SocketRing(DiscRadius, 0.f, true); return Icon; }
	const FVectorIcon& RimIcon() { static const FVectorIcon Icon = SocketRing((RimOuter + RimInner) * 0.5f, RimOuter - RimInner, false); return Icon; }
	const FVectorIcon& HairlineIcon() { static const FVectorIcon Icon = SocketRing(HairlineRadius, 1.f, false); return Icon; }

	/** The gunmetal ring lit from the top, as the weapon slots' (MetalHi to MetalMid above, MetalLow to MetalDeep below). */
	const FPaintedIcon& MetalIcon()
	{
		static const FPaintedIcon Icon = []()
		{
			const FVector2D& Center = SocketCenter;
			FPaintedIcon Ring;
			Ring.ViewBox = FVector2D(RingDiameter, RingDiameter);

			FPaintLayer Lower;
			// Even-odd: the inner circle cuts the hole.
			Lower.Fills.Add(SocketCircle(Center, MetalOuter));
			Lower.Fills.Add(SocketCircle(Center, MetalInner));
			Lower.Color = Color::MetalLow();
			Lower.GradientTo = Color::MetalDeep();
			Lower.GradientStart = Center;
			Lower.GradientEnd = Center + FVector2D(0.f, MetalOuter);
			Ring.Layers.Add(Lower);

			FPaintLayer Upper;
			TArray<FVector2D> HalfRing = SocketArc(Center, MetalOuter, 180.f, 360.f, CircleSegments / 2);
			HalfRing.Append(SocketArc(Center, MetalInner, 360.f, 180.f, CircleSegments / 2));
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

	const FVectorIcon& AccentIcon()
	{
		static const FVectorIcon Icon = []()
		{
			const float Size = RingDiameter + AccentMargin * 2.f;
			FVectorIcon Ring;
			Ring.ViewBox = FVector2D(Size, Size);
			Ring.StrokeWidth = AccentWidth;
			Ring.Strokes.Add(SocketClosed(SocketCircle(FVector2D(Size * 0.5f, Size * 0.5f), AccentRadius)));
			return Ring;
		}();
		return Icon;
	}

	/** The key tab Width wide: a small plate with its top-left and bottom-right corners cut, as the grenade's tab. */
	TArray<FVector2D> TabOutline(float Width)
	{
		const float H = UHudEmberSocketWidget::TabHeight;
		return { FVector2D(3.f, 0.f), FVector2D(Width, 0.f), FVector2D(Width, H - 3.f), FVector2D(Width - 3.f, H), FVector2D(0.f, H), FVector2D(0.f, 3.f) };
	}

	FSlateBrush TabPlateBrush(float Width)
	{
		FVectorIcon Plate;
		Plate.ViewBox = FVector2D(Width, UHudEmberSocketWidget::TabHeight);
		Plate.Fills.Add(TabOutline(Width));
		const FName Name(*FString::Printf(TEXT("HudEmberTabPlate_%d"), FMath::RoundToInt(Width)));
		return IconBrush(Name, Plate, TabPixelsPerUnit, Plate.ViewBox, FLinearColor::White);
	}

	FSlateBrush TabEdgeBrush(float Width)
	{
		FVectorIcon Edge;
		Edge.ViewBox = FVector2D(Width, UHudEmberSocketWidget::TabHeight);
		Edge.StrokeWidth = 1.f;
		TArray<FVector2D> Line = TabOutline(Width);
		// Pulled in half the line's width, so the stroke stays inside the picture.
		for (FVector2D& Point : Line)
		{
			Point = FVector2D(FMath::Clamp(Point.X, 0.5f, Width - 0.5f), FMath::Clamp(Point.Y, 0.5f, UHudEmberSocketWidget::TabHeight - 0.5f));
		}
		Edge.Strokes.Add(SocketClosed(Line));
		const FName Name(*FString::Printf(TEXT("HudEmberTabEdge_%d"), FMath::RoundToInt(Width)));
		return IconBrush(Name, Edge, TabPixelsPerUnit, Edge.ViewBox, FLinearColor::White);
	}
}

FSlateBrush UHudEmberSocketWidget::PictureBrush(EHudEmberPicture Picture)
{
	const FVector2D CircleSize(RingDiameter, RingDiameter);
	switch (Picture)
	{
	case EHudEmberPicture::Disc: return IconBrush(TEXT("HudEmberDisc"), DiscIcon(), PixelsPerUnit, CircleSize, FLinearColor::White);
	case EHudEmberPicture::Rim: return IconBrush(TEXT("HudEmberRim"), RimIcon(), PixelsPerUnit, CircleSize, FLinearColor::White);
	case EHudEmberPicture::Metal: return PaintedIconBrush(TEXT("HudEmberMetal"), MetalIcon(), PixelsPerUnit, CircleSize);
	case EHudEmberPicture::Hairline:
		return IconBrush(TEXT("HudEmberHairline"), HairlineIcon(), PixelsPerUnit, CircleSize, Color::Hairline().CopyWithNewOpacity(HairlineOpacity));
	case EHudEmberPicture::Accent: return IconBrush(TEXT("HudEmberAccent"), AccentIcon(), PixelsPerUnit, CircleSize, FLinearColor::White);
	}
	return FSlateBrush();
}

FSlateBrush UHudEmberSocketWidget::TabBrush(float Width, bool bEdge)
{
	return bEdge ? TabEdgeBrush(Width) : TabPlateBrush(Width);
}
