#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Widgets/SLeafWidget.h"
#include "LooterSegmentBar.generated.h"

/**
 * The gun card's stat bar ("Wide Sweep", 2026-10-28): ten skewed segments, lit up to the gun's rating, with an orange tick
 * where the compared gun's rating is. Fill (0-1) is the rating; Shown (0-1) how much of it is drawn yet (the card's intro
 * fills it). In UMG it is ULooterSegmentBar. A bar, so never a background.
 */
class AI_LOOTER_SHOOTER_API SLooterSegmentBar : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SLooterSegmentBar) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetFill(float InFill);
	void SetShown(float InShown);
	/** The compared gun's rating (0-1), or <0 for none. */
	void SetTick(float InTick);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;

private:
	float Fill = 0.f;
	float Shown = 1.f;
	float Tick = -1.f;
};

namespace LooterSegmentBarRules
{
	inline constexpr int32 Segments = 10;
	inline constexpr float Height = 7.f;
	inline constexpr float Gap = 2.f;
	/** How many segments are lit for a fill (rounded; a fill above zero lights at least one). */
	AI_LOOTER_SHOOTER_API int32 LitSegments(float Fill);
}

UCLASS()
class AI_LOOTER_SHOOTER_API ULooterSegmentBar : public UWidget
{
	GENERATED_BODY()

public:
	void SetFill(float InFill);
	float GetFill() const { return Fill; }
	void SetShown(float InShown);
	void SetTick(float InTick);
	float GetTick() const { return Tick; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<SLooterSegmentBar> Bar;
	float Fill = 0.f;
	float Shown = 1.f;
	float Tick = -1.f;
};
