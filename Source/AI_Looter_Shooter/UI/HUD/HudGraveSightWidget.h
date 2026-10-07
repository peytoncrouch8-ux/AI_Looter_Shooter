#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudGraveSightWidget.generated.h"

class UImage;

/**
 * Grave Sight's flash on the screen (UGraveSightSubsystem; Docs/Story.md: "The cyan lines are Grave Sight"): the HUD's cyan
 * laid over the world for a moment, in the LooterUI kit's palette. The screen's edges darken to the kit's dark glass, a
 * soft vignette (a background, so the UI transparency setting fades it), and over it, solid as the HUD's lines are: a thin
 * cyan frame inset from the edges with the kit's cut corners and its corner brackets, faint scan lines, a brighter line
 * sweeping down the screen once, and a ring opening out from the middle of the view, where the player is looking. No panel
 * and no words. The subsystem sets how strong it shows and how far through the flash it is, every frame of a flash;
 * between flashes it's collapsed.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudGraveSightWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Over the gameplay HUD (0), under the captions (2), the boss bar (3), the prompts (5, 6) and every menu (20 up). */
	static constexpr int32 ViewportZOrder = 1;

	/** Shows the flash at Alpha (0 to 1; 0 collapses it) and Progress (0 to 1: where the sweep and the ring have got to). */
	void SetFlash(float InAlpha, float InProgress);

	float GetAlpha() const { return Alpha; }
	float GetProgress() const { return Progress; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	/** The darkened edges: the only background it paints. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> Vignette;

	float Alpha = 0.f;
	float Progress = 0.f;
};
