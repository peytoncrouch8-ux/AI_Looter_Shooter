#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudFrameRateWidget.generated.h"

class UTextBlock;

/**
 * The HUD's frame rate counter, top left: "FPS 120" with the frame time under it, in floating outlined text like the
 * rest of the gameplay HUD (no backing panel). It averages over half a second, so the number can be read instead of
 * flickering, and only rewrites its text that often. The number turns orange below 60 fps and red below 30. Settings ›
 * Interface › FPS counter hides it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudFrameRateWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Shows or hides the counter as the setting says; true when it's shown. */
	bool ApplySetting();

	UPROPERTY(Transient) TObjectPtr<UWidget> Cluster;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RateValue;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FrameTimeText;

	/** Frames and seconds counted toward the next update. */
	int32 Frames = 0;
	float Elapsed = 0.f;
};
