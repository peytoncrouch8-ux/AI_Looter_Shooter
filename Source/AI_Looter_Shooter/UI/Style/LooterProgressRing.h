#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * A ring's geometry, apart from the widget that draws it so it can be tested: angles are degrees clockwise from 12 o'clock,
 * the way a clock fills.
 */
namespace LooterRing
{
	/** A stretch of the ring between two angles. */
	struct FArc
	{
		float Start = 0.f;
		float End = 0.f;
		float Sweep() const { return End - Start; }
	};

	/** How far round the fill reaches for a Progress (clamped to 0-1): 0 to 360 degrees. */
	float FillDegrees(float Progress);

	/**
	 * The Index-th of Segments equal parts of the ring, with GapDegrees cut between neighbours (half off each end), so the
	 * first gap is centred on 12 o'clock.
	 */
	FArc SegmentArc(int32 Index, int32 Segments, float GapDegrees);

	/** How much of segment Index is lit (0-1) at Progress: they fill one after another, each over its own share of it. */
	float SegmentFill(int32 Index, int32 Segments, float Progress);

	/** The angle a gap of GapPixels spans on a ring of Radius. */
	float GapDegrees(float GapPixels, float Radius);

	/**
	 * The radius of the ring's centre line in a box Diameter across: it leaves room outside for the dark edge, the flash's
	 * glow and the line's anti-aliasing, so nothing is cut at the box.
	 */
	float CenterRadius(float Diameter, float Thickness);

	/** Appends points along the arc (a point at the start, one at the end, steps of at most MaxStepDegrees between). */
	void AppendArcPoints(TArray<FVector2f>& OutPoints, const FVector2f& Center, float Radius, float StartDegrees, float EndDegrees,
		float MaxStepDegrees = 4.f);

	/** The pop a ring gives when a hold completes: it swells by PopGrow and settles back over PopSeconds. */
	inline constexpr float PopSeconds = 0.16f;
	inline constexpr float PopGrow = 0.10f;

	/** The ring's scale a share (0-1) of the way through the pop: 1, up to 1 + PopGrow midway, and 1 again. */
	float PopScale(float Elapsed);

	/** The bright flash a share of the way through the pop: full as it starts, gone by the end. */
	float PopFlash(float Elapsed);
}

/**
 * The kit's progress ring: a circle that fills clockwise from 12 o'clock, anti-aliased and crisp at any size, on a dim
 * track with a dark edge so it reads over sky and snow. Smooth, or in Segments parts with small gaps (the HUD's segmented
 * look). Colors come from LooterUI::Color; it paints bars (solid), never a background. Flash (0-1) lights the fill toward
 * white and adds a soft glow: the pop when a hold completes. In UMG it is ULooterProgressRing.
 */
class AI_LOOTER_SHOOTER_API SLooterProgressRing : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SLooterProgressRing)
		: _Progress(0.f)
		, _Flash(0.f)
		, _Diameter(32.f)
		, _Thickness(3.f)
		, _Segments(0)
		, _SegmentGap(3.f)
	{
	}
		/** How full it is, 0-1. */
		SLATE_ATTRIBUTE(float, Progress)
		/** The pop's light, 0-1. */
		SLATE_ATTRIBUTE(float, Flash)
		/** The filled part (the kit's accent when not set) and the track under it (a dim cyan when not set). */
		SLATE_ATTRIBUTE(FLinearColor, FillColor)
		SLATE_ATTRIBUTE(FLinearColor, TrackColor)
		/** The box it asks for, and the line's width, in Slate units. */
		SLATE_ARGUMENT(float, Diameter)
		SLATE_ARGUMENT(float, Thickness)
		/** 0 (or 1): one smooth arc. More: that many parts, SegmentGap pixels apart. */
		SLATE_ARGUMENT(int32, Segments)
		SLATE_ARGUMENT(float, SegmentGap)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetProgress(const TAttribute<float>& InProgress);
	void SetFlash(const TAttribute<float>& InFlash);
	void SetColors(const TAttribute<FLinearColor>& InFill, const TAttribute<FLinearColor>& InTrack);
	void Configure(float InDiameter, float InThickness, int32 InSegments, float InSegmentGap);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	TAttribute<float> Progress;
	TAttribute<float> Flash;
	TAttribute<FLinearColor> FillColor;
	TAttribute<FLinearColor> TrackColor;
	float Diameter = 32.f;
	float Thickness = 3.f;
	int32 Segments = 0;
	float SegmentGap = 3.f;
};
