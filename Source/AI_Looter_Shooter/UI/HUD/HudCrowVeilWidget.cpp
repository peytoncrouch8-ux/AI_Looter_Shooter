// UHudCrowVeilWidget: Hob's crows over the screen, the carry's look. FRoostVeil says where each crow is; this paints them.

#include "UI/HUD/HudCrowVeilWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateLayoutTransform.h"
#include "Rendering/SlateRenderTransform.h"
#include "Styling/WidgetStyle.h"

using namespace LooterUI;

namespace
{
	/** The crow's picture: a 64 x 64 view box, the bird seen from above flying toward +x, wings spread across it. */
	constexpr float CrowBox = 64.f;
	constexpr float CrowMiddle = 32.f;
	/** Texture texels per view-box unit: a 256-texel picture, sharp enough for the nearest crows' size on a 1080p screen. */
	constexpr float CrowPixelsPerUnit = 4.f;

	/** How much the dark lightens the crows (toward the kit's low metal), so their wings still read against black. */
	constexpr float DarkLift = 0.75f;

	/** A circle as a polygon. */
	TArray<FVector2D> CrowRing(const FVector2D& Center, double Radius, int32 Sides = 16)
	{
		TArray<FVector2D> Points;
		for (int32 Step = 0; Step < Sides; ++Step)
		{
			const double Angle = UE_TWO_PI * Step / Sides;
			Points.Add(Center + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
		}
		return Points;
	}

	/**
	 * A wing (the upper one in the picture, y under the middle): its leading edge from the shoulder out to the tip, then
	 * back along the trailing edge with the primaries spread like fingers. Span folds it toward the body (a raised or
	 * lowered wing seen from above is shorter) and Sweep moves its tip back (raised) or forward (lowered).
	 */
	TArray<FVector2D> CrowWing(double Span, double Sweep, bool bLower)
	{
		static const FVector2D Shape[] = {
			FVector2D(39.0, 29.5), FVector2D(38.0, 23.0), FVector2D(35.5, 15.0), FVector2D(31.5, 8.0), FVector2D(27.0, 3.0),
			FVector2D(23.5, 1.5), FVector2D(21.5, 3.5), FVector2D(22.8, 6.6), FVector2D(19.8, 6.4), FVector2D(20.6, 9.8),
			FVector2D(17.6, 10.2), FVector2D(19.4, 13.8), FVector2D(16.6, 15.2), FVector2D(19.6, 18.4), FVector2D(18.4, 21.6),
			FVector2D(22.6, 24.4), FVector2D(26.0, 28.0), FVector2D(29.0, 29.5) };
		TArray<FVector2D> Points;
		for (const FVector2D& Point : Shape)
		{
			const double Out = FMath::Abs(Point.Y - CrowMiddle) / (CrowMiddle - 1.5);
			const double Y = CrowMiddle + (Point.Y - CrowMiddle) * Span;
			Points.Add(FVector2D(Point.X + Sweep * Out, bLower ? CrowBox - Y : Y));
		}
		return Points;
	}

	FPaintLayer CrowPart(TArray<FVector2D> Shape)
	{
		FPaintLayer Layer;
		Layer.Fills.Add(MoveTemp(Shape));
		Layer.Color = FLinearColor::White;
		return Layer;
	}

	/**
	 * A crow from above in white (the draw tints it): Frame 0 its wings raised, 1 level, 2 lowered. Each part a layer of its
	 * own, as a layer's polygons fill even-odd together and the head, body and wings overlap.
	 */
	FPaintedIcon CrowPicture(int32 Frame)
	{
		FPaintedIcon Picture;
		Picture.ViewBox = FVector2D(CrowBox, CrowBox);
		const double Span = Frame == 0 ? 0.58 : Frame == 2 ? 0.82 : 1.0;
		const double Sweep = Frame == 0 ? -3.5 : Frame == 2 ? 2.5 : 0.0;
		Picture.Layers.Add(CrowPart(CrowWing(Span, Sweep, false)));
		Picture.Layers.Add(CrowPart(CrowWing(Span, Sweep, true)));
		Picture.Layers.Add(CrowPart({ FVector2D(20.0, 29.4), FVector2D(12.0, 27.4), FVector2D(7.0, 27.8), FVector2D(5.0, 30.4), FVector2D(4.6, 32.0),
			FVector2D(5.0, 33.6), FVector2D(7.0, 36.2), FVector2D(12.0, 36.6), FVector2D(20.0, 34.6) }));
		Picture.Layers.Add(CrowPart({ FVector2D(19.0, 30.0), FVector2D(27.0, 28.2), FVector2D(37.0, 28.3), FVector2D(44.0, 29.5), FVector2D(47.5, 31.2),
			FVector2D(47.5, 32.8), FVector2D(44.0, 34.5), FVector2D(37.0, 35.7), FVector2D(27.0, 35.8), FVector2D(19.0, 34.0) }));
		Picture.Layers.Add(CrowPart(CrowRing(FVector2D(48.5, 32.0), 4.3)));
		Picture.Layers.Add(CrowPart({ FVector2D(51.8, 30.4), FVector2D(58.5, 32.0), FVector2D(51.8, 33.6) }));
		return Picture;
	}
}

TSharedRef<SWidget> UHudCrowVeilWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// Nothing of its own but the whole screen to paint on.
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;
	}
	if (!bBrushesMade)
	{
		bBrushesMade = true;
		ShadeBrush = RectBrush(FLinearColor::White);
		static const TCHAR* const Names[] = { TEXT("RoostCrowRaised"), TEXT("RoostCrowLevel"), TEXT("RoostCrowLowered") };
		for (int32 Frame = 0; Frame < 3; ++Frame)
		{
			CrowBrushes[Frame] = PaintedIconBrush(Names[Frame], CrowPicture(Frame), CrowPixelsPerUnit, FVector2D(CrowBox, CrowBox));
		}
	}
	TSharedRef<SWidget> Built = Super::RebuildWidget();
	SetVisibility(Veil.IsActive() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	return Built;
}

void UHudCrowVeilWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Every crow moves every frame of a carry (and it's collapsed between carries, when that costs nothing).
	ForceVolatile(true);
}

void UHudCrowVeilWidget::ShowVeil(const FRoostVeil& InVeil)
{
	Veil = InVeil;
	const ESlateVisibility Wanted = Veil.IsActive() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (GetVisibility() != Wanted)
	{
		SetVisibility(Wanted);
	}
}

int32 UHudCrowVeilWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Under = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FVector2f Screen(AllottedGeometry.GetLocalSize());
	if (!Veil.IsActive() || Screen.X < 2.f || Screen.Y < 2.f)
	{
		return Under;
	}
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
	const float Shade = Veil.GetShade();
	const FLinearColor Ink = Color::Ink();

	// The dark under the crows: the screen going black as the flock thickens.
	int32 Layer = Under + 1;
	if (Shade > 0.f)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), &ShadeBrush, ESlateDrawEffect::None,
			FLinearColor(Ink.R, Ink.G, Ink.B, Shade * Opacity));
	}

	// The crows over it, the far ones first, each turned along its flight; lighter as the dark deepens, so they still read.
	++Layer;
	const FLinearColor Body = Ink + (Color::MetalLow() - Ink) * (Shade * DarkLift);
	Veil.GetCrows(Screen.X / Screen.Y, Crows);
	for (const FRoostVeilCrow& Crow : Crows)
	{
		const float Span = Crow.Size * Screen.Y;
		if (Span < 1.f || Crow.Opacity <= 0.f)
		{
			continue;
		}
		const FVector2f Box(Span, Span);
		const FVector2f Centre = Screen * 0.5f + Crow.Position * Screen.Y;
		const FPaintGeometry Paint = AllottedGeometry.ToPaintGeometry(Box, FSlateLayoutTransform(Centre - Box * 0.5f),
			FSlateRenderTransform(FQuat2f(Crow.Angle)), FVector2f(0.5f, 0.5f));
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, Paint, &CrowBrushes[FMath::Clamp(Crow.Frame, 0, 2)], ESlateDrawEffect::None,
			FLinearColor(Body.R, Body.G, Body.B, Crow.Opacity * Opacity));
	}
	return Layer;
}
