// The kit's progress ring: its geometry (LooterRing) and the Slate widget that strokes it (SLooterProgressRing).

#include "UI/Style/LooterProgressRing.h"
#include "UI/Style/LooterUIStyle.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Styling/WidgetStyle.h"

namespace
{
	/**
	 * Room kept between the line's centre and the box's edge beyond half its width: the dark edge (a pixel each side), the
	 * flash's glow (two and a half) and the anti-aliasing's soft pixel.
	 */
	constexpr float EdgeRoom = 3.5f;

	/** The dark edge is this much wider than the line, and the flash's glow this much. */
	constexpr float EdgeExtra = 2.f;
	constexpr float GlowExtra = 5.f;

	/** The glow's strength at a full flash. */
	constexpr float GlowAlpha = 0.4f;

	/** Arcs shorter than this don't draw: they would only be a speck. */
	constexpr float MinSweep = 0.05f;
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

float LooterRing::FillDegrees(float Progress)
{
	return FMath::Clamp(Progress, 0.f, 1.f) * 360.f;
}

LooterRing::FArc LooterRing::SegmentArc(int32 Index, int32 Segments, float GapDegrees)
{
	const float Span = 360.f / FMath::Max(Segments, 1);
	// A gap never eats a whole part, however wide it is asked for.
	const float Half = FMath::Min(GapDegrees, Span * 0.8f) * 0.5f;
	FArc Arc;
	Arc.Start = Index * Span + Half;
	Arc.End = (Index + 1) * Span - Half;
	return Arc;
}

float LooterRing::SegmentFill(int32 Index, int32 Segments, float Progress)
{
	return FMath::Clamp(FMath::Clamp(Progress, 0.f, 1.f) * FMath::Max(Segments, 1) - Index, 0.f, 1.f);
}

float LooterRing::GapDegrees(float GapPixels, float Radius)
{
	return Radius > UE_KINDA_SMALL_NUMBER ? FMath::RadiansToDegrees(GapPixels / Radius) : 0.f;
}

float LooterRing::CenterRadius(float Diameter, float Thickness)
{
	return FMath::Max(0.f, Diameter * 0.5f - Thickness * 0.5f - EdgeRoom);
}

void LooterRing::AppendArcPoints(TArray<FVector2f>& OutPoints, const FVector2f& Center, float Radius, float StartDegrees, float EndDegrees,
	float MaxStepDegrees)
{
	const float Sweep = EndDegrees - StartDegrees;
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(FMath::Abs(Sweep) / FMath::Max(MaxStepDegrees, 0.1f)));
	OutPoints.Reserve(OutPoints.Num() + Steps + 1);
	for (int32 Step = 0; Step <= Steps; ++Step)
	{
		const float Angle = FMath::DegreesToRadians(StartDegrees + Sweep * Step / Steps);
		OutPoints.Add(FVector2f(Center.X + Radius * FMath::Sin(Angle), Center.Y - Radius * FMath::Cos(Angle)));
	}
}

float LooterRing::PopScale(float Elapsed)
{
	const float Share = FMath::Clamp(Elapsed, 0.f, 1.f);
	return 1.f + PopGrow * FMath::Sin(UE_PI * Share);
}

float LooterRing::PopFlash(float Elapsed)
{
	// Brightest at once and gone by the end, easing out so the light lingers a moment as the ring settles.
	const float Left = 1.f - FMath::Clamp(Elapsed, 0.f, 1.f);
	return Left * Left;
}

// ---------------------------------------------------------------------------
// The widget
// ---------------------------------------------------------------------------

void SLooterProgressRing::Construct(const FArguments& InArgs)
{
	Progress = InArgs._Progress;
	Flash = InArgs._Flash;
	FillColor = InArgs._FillColor.IsSet() ? InArgs._FillColor : TAttribute<FLinearColor>(LooterUI::Color::Accent());
	if (InArgs._TrackColor.IsSet())
	{
		TrackColor = InArgs._TrackColor;
	}
	else
	{
		// The cyan hairline, faint: the part still to fill.
		FLinearColor Track = LooterUI::Color::Hairline();
		Track.A = 0.28f;
		TrackColor = Track;
	}
	Diameter = InArgs._Diameter;
	Thickness = InArgs._Thickness;
	Segments = InArgs._Segments;
	SegmentGap = InArgs._SegmentGap;
}

void SLooterProgressRing::SetProgress(const TAttribute<float>& InProgress)
{
	if (!Progress.IdenticalTo(InProgress))
	{
		Progress = InProgress;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SLooterProgressRing::SetFlash(const TAttribute<float>& InFlash)
{
	if (!Flash.IdenticalTo(InFlash))
	{
		Flash = InFlash;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SLooterProgressRing::SetColors(const TAttribute<FLinearColor>& InFill, const TAttribute<FLinearColor>& InTrack)
{
	FillColor = InFill;
	TrackColor = InTrack;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SLooterProgressRing::Configure(float InDiameter, float InThickness, int32 InSegments, float InSegmentGap)
{
	Diameter = InDiameter;
	Thickness = InThickness;
	Segments = InSegments;
	SegmentGap = InSegmentGap;
	// The diameter is the size it asks for.
	Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
}

FVector2D SLooterProgressRing::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(Diameter, Diameter);
}

int32 SLooterProgressRing::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Size(AllottedGeometry.GetLocalSize());
	const float Box = FMath::Min(Size.X, Size.Y);
	// Its parents' opacity (a prompt fading) arrives in the style, as it does for an image.
	const FLinearColor Style = InWidgetStyle.GetColorAndOpacityTint();
	if (Box < 4.f || Style.A <= 0.f)
	{
		return LayerId;
	}

	const FVector2f Center = Size * 0.5f;
	const float Radius = LooterRing::CenterRadius(Box, Thickness);
	const float Fraction = FMath::Clamp(Progress.Get(), 0.f, 1.f);
	const float Light = FMath::Clamp(Flash.Get(), 0.f, 1.f);

	// The track's parts, and the share of each that is filled.
	TArray<LooterRing::FArc> TrackArcs;
	TArray<LooterRing::FArc> FillArcs;
	if (Segments > 1)
	{
		const float Gap = LooterRing::GapDegrees(SegmentGap, Radius);
		for (int32 Index = 0; Index < Segments; ++Index)
		{
			const LooterRing::FArc Arc = LooterRing::SegmentArc(Index, Segments, Gap);
			TrackArcs.Add(Arc);
			const float Lit = LooterRing::SegmentFill(Index, Segments, Fraction);
			if (Lit * Arc.Sweep() > MinSweep)
			{
				FillArcs.Add({ Arc.Start, Arc.Start + Lit * Arc.Sweep() });
			}
		}
	}
	else
	{
		TrackArcs.Add({ 0.f, 360.f });
		const float Reach = LooterRing::FillDegrees(Fraction);
		if (Reach > MinSweep)
		{
			FillArcs.Add({ 0.f, Reach });
		}
	}

	auto Stroke = [&](int32 Layer, const TArray<LooterRing::FArc>& Arcs, const FLinearColor& Tint, float Width)
	{
		if (Tint.A <= 0.f)
		{
			return;
		}
		for (const LooterRing::FArc& Arc : Arcs)
		{
			TArray<FVector2f> Points;
			LooterRing::AppendArcPoints(Points, Center, Radius, Arc.Start, Arc.End);
			FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None,
				Tint, true, Width);
		}
	};

	// Bottom to top, each on its own layer so Slate keeps the order: the glow of a flash, the dark edge, the track, the fill.
	const FLinearColor Base = FillColor.Get();
	FLinearColor Filled = FMath::Lerp(Base, FLinearColor::White, Light * 0.85f);
	Filled.A = Base.A;
	FLinearColor Glow = Filled;
	Glow.A = Base.A * GlowAlpha * Light;
	if (Light > 0.f)
	{
		Stroke(LayerId, FillArcs, Glow * Style, Thickness + GlowExtra);
	}
	Stroke(LayerId + 1, TrackArcs, LooterUI::Color::Outline() * Style, Thickness + EdgeExtra);
	Stroke(LayerId + 2, TrackArcs, TrackColor.Get() * Style, Thickness);
	Stroke(LayerId + 3, FillArcs, Filled * Style, Thickness);
	return LayerId + 3;
}
