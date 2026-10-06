#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudCaptionWidget.generated.h"

class UTextBlock;
class UWidget;
struct FStoryLine;

/**
 * What's said aloud (UCaptionSubsystem's lines), low on the screen over the experience bar: the speaker's name in the
 * accent color over their words in the HUD's white, both floating outlined text with no panel, like the rest of the
 * gameplay HUD. Each line fades in and out as the caption queue times it. While a menu covers the game (or it's paused)
 * the captions step aside and the queue waits, so no line plays on unseen. ALooterHUD adds it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudCaptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Over the gameplay HUD (0), under the boss bar (3), the tutorial's prompt (5), the inventory (20) and the pause menu (40). */
	static constexpr int32 ViewportZOrder = 2;

	/** Long lines wrap at this width (reference pixels). */
	static constexpr float WrapWidth = 900.f;

	/** The words' bottom sits this far over the screen's bottom: clear of the experience bar and the gain over it. */
	static constexpr float BottomGap = 118.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

private:
	/** Puts a new line's speaker and words up. */
	void ShowLine(const FStoryLine& Line);

	/** The inventory or the pause menu is up, or the game is paused: nobody can read captions. */
	bool IsMenuOpen() const;

	UPROPERTY(Transient) TObjectPtr<UWidget> CaptionBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SpeakerText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WordsText;

	/** The queue's number for the line shown, so its words are set once. */
	int32 ShownSerial = 0;

	/** Eases to 0 while a menu covers the game and back to 1 after, on top of the line's own fade. */
	float Presence = 1.f;

	/** The opacity last drawn, so nothing is touched while it holds. */
	float ShownOpacity = -1.f;
};
