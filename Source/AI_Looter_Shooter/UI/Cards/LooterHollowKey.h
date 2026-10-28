#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Widgets/SLeafWidget.h"
#include "LooterHollowKey.generated.h"

/**
 * A hollow key cap (the user's call, 2026-10-28: "the Hold E buttons should be hollow instead of a solid bubble"): a 2 px
 * orange outline chamfered 5 px at two corners, the key's name in orange inside, no fill. A hold's cap fills orange left to
 * right as the hold runs (its letters turn dark over the fill). In UMG it is ULooterHollowKey.
 */
class AI_LOOTER_SHOOTER_API SLooterHollowKey : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SLooterHollowKey) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetLabel(const FString& InLabel);
	/** How far the hold has run (0-1); 0 draws it hollow. */
	void SetFill(float InFill);
	/** Dimmed (a second choice): the outline and letters at 60%. */
	void SetDim(bool bInDim);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;

private:
	FString Label;
	float Fill = 0.f;
	bool bDim = false;
};

UCLASS()
class AI_LOOTER_SHOOTER_API ULooterHollowKey : public UWidget
{
	GENERATED_BODY()

public:
	void SetLabel(const FString& InLabel);
	const FString& GetLabel() const { return Label; }
	void SetFill(float InFill);
	float GetFill() const { return Fill; }
	void SetDim(bool bInDim);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<SLooterHollowKey> Key;
	FString Label = TEXT("E");
	float Fill = 0.f;
	bool bDim = false;
};
