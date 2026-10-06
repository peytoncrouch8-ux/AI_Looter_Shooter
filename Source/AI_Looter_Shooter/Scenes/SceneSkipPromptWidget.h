#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SceneSkipPromptWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;

/**
 * How to skip the scene playing, in the bottom right corner as floating outlined text with no panel (the gameplay HUD's
 * rule): "HOLD [E] TO SKIP" over a slim slanted bar that fills while the key is held, or "PRESS [ESC] AGAIN TO SKIP" after
 * a first Escape. It shows only once the player presses one of them, and fades out a few seconds after (USceneSubsystem
 * drives it).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API USceneSkipPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * The prompt shown (fading in) or not (fading out): Lead ("HOLD", "PRESS"), then Key in brackets in the accent color, then
	 * Words. Hold fills the bar under it (0 hides the bar).
	 */
	void Update(bool bShown, const FString& Lead, const FString& Key, const FString& Words, float Hold, float DeltaSeconds);

	/** Over the HUD's widgets and the tutorial's prompt, under the menus. */
	static constexpr int32 ViewportZOrder = 6;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient) TObjectPtr<UWidget> Prompt;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LeadText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WordsText;
	/** Fills while the key is held; hidden (keeping its room) otherwise. */
	UPROPERTY(Transient) TObjectPtr<UWidget> HoldBar;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> HoldSegments;

	/** Shown opacity, eased toward 1 while the prompt is wanted. */
	float Opacity = 0.f;
};
