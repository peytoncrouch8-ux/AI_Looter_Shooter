// The crosshair per gun kind, painted from HudCrosshairRules' shapes: dark edges on one layer, the coloured lines over them
// on the next, so one line's edge never covers another's colour where they meet.

#include "UI/HUD/HudCrosshairWidget.h"
#include "UI/HUD/HudCrosshairRules.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateLayoutTransform.h"
#include "Styling/WidgetStyle.h"

using namespace HudCrosshair;

namespace
{
	/** The ring's dashes are drawn as short polylines, a point every this many radians: smooth at the ring's sizes. */
	constexpr float ArcStep = 0.08f;

	FLinearColor Faded(const FLinearColor& Colour, float Alpha)
	{
		return FLinearColor(Colour.R, Colour.G, Colour.B, Colour.A * Alpha);
	}

	/**
	 * An open polyline's ends pushed out by Cap px along its first and last segments: Slate's lines end flat at their points,
	 * where the mockup's have round caps half their width past them, and the edge must reach past the line's end to frame it.
	 */
	TArray<FVector2f> Capped(const TArray<FVector2f>& Points, float Cap)
	{
		TArray<FVector2f> Out = Points;
		if (Out.Num() >= 2 && Cap > 0.f)
		{
			Out[0] += (Out[0] - Out[1]).GetSafeNormal() * Cap;
			const int32 Last = Out.Num() - 1;
			Out[Last] += (Out[Last] - Out[Last - 1]).GetSafeNormal() * Cap;
		}
		return Out;
	}

	/**
	 * A closed polyline started and ended halfway along its last side, so every corner is a join inside the line (Slate joins
	 * the points between, never the first with the last).
	 */
	TArray<FVector2f> Closed(const TArray<FVector2f>& Points)
	{
		if (Points.Num() < 3)
		{
			return Points;
		}
		const FVector2f Start = (Points.Last() + Points[0]) * 0.5f;
		TArray<FVector2f> Out;
		Out.Add(Start);
		Out.Append(Points);
		Out.Add(Start);
		return Out;
	}

	TArray<FVector2f> Arc(const FVector2f& Middle, float Radius, float From, float To)
	{
		TArray<FVector2f> Points;
		const int32 Steps = FMath::Max(2, FMath::CeilToInt32((To - From) / ArcStep));
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Angle = FMath::Lerp(From, To, static_cast<float>(Step) / Steps);
			Points.Add(Middle + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Points;
	}

	void Lines(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, TArray<FVector2f> Points,
		const FLinearColor& Tint, float Width)
	{
		if (Points.Num() >= 2)
		{
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Tint, true, Width);
		}
	}

	void Disc(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, const FVector2f& Centre, float Radius,
		const FLinearColor& Tint)
	{
		const FSlateRoundedBoxBrush Brush(Tint, Radius);
		FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(FVector2f(Radius * 2.f, Radius * 2.f),
			FSlateLayoutTransform(Centre - FVector2f(Radius, Radius))), &Brush, ESlateDrawEffect::None, FLinearColor::White);
	}
}

void SHudCrosshair::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
}

void SHudCrosshair::SetKind(EWeaponKind InKind)
{
	if (Kind != InKind)
	{
		Kind = InKind;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SHudCrosshair::SetPart(EHudCrosshairPart InPart)
{
	if (Part != InPart)
	{
		Part = InPart;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SHudCrosshair::SetOpen(float InOpen)
{
	const float Clamped = FMath::Clamp(InOpen, 0.f, MaxOpen);
	if (!FMath::IsNearlyEqual(Open, Clamped, 0.01f))
	{
		Open = Clamped;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SHudCrosshair::SetColour(const FLinearColor& InColour)
{
	if (!Colour.Equals(InColour))
	{
		Colour = InColour;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

FVector2D SHudCrosshair::ComputeDesiredSize(float) const
{
	return FVector2D(CanvasSize, CanvasSize);
}

int32 SHudCrosshair::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const float Alpha = InWidgetStyle.GetColorAndOpacityTint().A;
	if (Alpha <= 0.001f)
	{
		return LayerId;
	}
	const FShape& Shape = ShapeFor(Kind);
	const FVector2f Middle = FVector2f(AllottedGeometry.GetLocalSize()) * 0.5f;
	const FLinearColor Edge = Faded(EdgeColour(), Alpha);
	const FLinearColor Ink = Faded(Colour, Alpha);
	const int32 EdgeLayer = LayerId;
	const int32 InkLayer = LayerId + 1;

	if (Part == EHudCrosshairPart::Dot)
	{
		const FVector2f At = Middle + Shape.Dot.At * UnitScale;
		Disc(OutDrawElements, EdgeLayer, AllottedGeometry, At, (Shape.Dot.Radius + DotOutlineExtra) * UnitScale, Edge);
		Disc(OutDrawElements, InkLayer, AllottedGeometry, At, Shape.Dot.Radius * UnitScale, Ink);
		return InkLayer;
	}

	for (const FStroke& Stroke : Shape.Strokes)
	{
		TArray<FVector2f> Points;
		Points.Reserve(Stroke.Points.Num());
		for (const FVector2f& Point : Stroke.Points)
		{
			Points.Add(Middle + Point * UnitScale + Stroke.Push * Open);
		}
		const float Width = Stroke.Width * UnitScale;
		const float EdgeWidth = (Stroke.Width + OutlineExtra) * UnitScale;
		if (Stroke.bClosed)
		{
			const TArray<FVector2f> Loop = Closed(Points);
			Lines(OutDrawElements, EdgeLayer, AllottedGeometry, Loop, Edge, EdgeWidth);
			Lines(OutDrawElements, InkLayer, AllottedGeometry, Loop, Ink, Width);
		}
		else
		{
			Lines(OutDrawElements, EdgeLayer, AllottedGeometry, Capped(Points, EdgeWidth * 0.5f), Edge, EdgeWidth);
			Lines(OutDrawElements, InkLayer, AllottedGeometry, Capped(Points, Width * 0.5f), Ink, Width);
		}
	}

	for (const FRing& Ring : Shape.Rings)
	{
		const float Radius = Ring.Radius * UnitScale + Open;
		const int32 Dashes = FMath::Max(1, Ring.Dashes);
		const float Stretch = UE_TWO_PI / Dashes;
		const float Half = Stretch * FMath::Clamp(Ring.DashShare, 0.05f, 1.f) * 0.5f;
		const float Width = Ring.Width * UnitScale;
		const float EdgeWidth = (Ring.Width + OutlineExtra) * UnitScale;
		// Each dash's edge reaches a little past its ends, as the mockup's edge does round its dashes.
		const float EdgeReach = Radius > 1.f ? (OutlineExtra * 0.5f * UnitScale) / Radius : 0.f;
		for (int32 Dash = 0; Dash < Dashes; ++Dash)
		{
			// Dashes centred on the axes and the diagonals, so the ring reads level.
			const float Centre = Dash * Stretch;
			Lines(OutDrawElements, EdgeLayer, AllottedGeometry, Arc(Middle, Radius, Centre - Half - EdgeReach, Centre + Half + EdgeReach), Edge, EdgeWidth);
			Lines(OutDrawElements, InkLayer, AllottedGeometry, Arc(Middle, Radius, Centre - Half, Centre + Half), Ink, Width);
		}
	}
	return InkLayer;
}

TSharedRef<SWidget> UHudCrosshairWidget::RebuildWidget()
{
	Crosshair = SNew(SHudCrosshair);
	Crosshair->SetKind(Kind);
	Crosshair->SetPart(Part);
	Crosshair->SetOpen(Open);
	Crosshair->SetColour(Colour);
	return Crosshair.ToSharedRef();
}

void UHudCrosshairWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Crosshair.Reset();
}

void UHudCrosshairWidget::SetKind(EWeaponKind InKind)
{
	Kind = InKind;
	if (Crosshair.IsValid())
	{
		Crosshair->SetKind(Kind);
	}
}

void UHudCrosshairWidget::SetPart(EHudCrosshairPart InPart)
{
	Part = InPart;
	if (Crosshair.IsValid())
	{
		Crosshair->SetPart(Part);
	}
}

void UHudCrosshairWidget::SetOpen(float InOpen)
{
	Open = FMath::Clamp(InOpen, 0.f, MaxOpen);
	if (Crosshair.IsValid())
	{
		Crosshair->SetOpen(Open);
	}
}

void UHudCrosshairWidget::SetColour(const FLinearColor& InColour)
{
	Colour = InColour;
	if (Crosshair.IsValid())
	{
		Crosshair->SetColour(Colour);
	}
}
