#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IntroWidget.generated.h"

class APlayerController;
class UOverlaySlot;
class USceneSkipPromptWidget;
class USizeBox;
class UTextBlock;
class UWidget;

/** How a caption fades: in over FadeIn, held, out over FadeOut, all inside Seconds (0 outside it). */
namespace IntroFade
{
	AI_LOOTER_SHOOTER_API float Alpha(float Elapsed, float Seconds, float FadeIn, float FadeOut);
}

/**
 * The intro's skip keys where no scene holds the keys (the menu world's planet part): the scene's own rules
 * (USceneSubsystem): hold Interact's key (E), Enter, Space or the pad's bottom face button for SkipHoldSeconds, or
 * Escape (or the pad's Start) twice within SkipPromptSeconds; none of them in the first SkipKeysAfter, so the key that
 * confirmed the hero doesn't skip. The prompt shows once one is pressed.
 */
struct AI_LOOTER_SHOOTER_API FIntroSkipKeys
{
	/** A hold key went down (Label: how the prompt names it). */
	void HoldDown(const FString& Label);
	void HoldUp();
	void Escape();
	void Tick(float DeltaSeconds);

	bool WantsSkip() const { return bSkip; }
	/** The hold's progress (0-1). */
	float GetHold() const;
	bool IsPromptShown() const { return PromptLeft > 0.f; }
	/** The prompt's three parts, as USceneSkipPromptWidget::Update takes them. */
	FString GetLead() const;
	FString GetKey() const;
	FString GetWords() const;

private:
	bool CanSkip() const;

	float Age = 0.f;
	float Held = 0.f;
	float EscapeLeft = 0.f;
	float PromptLeft = 0.f;
	bool bDown = false;
	bool bEscapePrompt = false;
	bool bSkip = false;
	FString KeyLabel;
};

/**
 * The intro's screen (Story/NewGameIntro.h): letterbox bars, a place card (its name in the display type over a cyan rule
 * with an orange tick, a dim line under it; the hero's card with its role in orange) and narration lines low in the
 * frame, all floating outlined text with no panel (the gameplay HUD's rule; the bars are the only backgrounds, marked for
 * the UI transparency). Driven by whoever plays the intro; fades itself. In the menu world it also takes the skip keys
 * and shows the scene's skip prompt.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Over the HUD and the menus, under the white (UTransitionScreenSubsystem::ViewportZOrder) and the scene's skip prompt. */
	static constexpr int32 ViewportZOrder = 5;
	/** In the menu world, over the menu's own widgets. */
	static constexpr int32 MenuZOrder = 900;

	/** A card for Seconds: Title in the display type, Sub under it (orange with bAccentSub: the hero's role). */
	void ShowCard(const FText& Title, const FText& Sub, float Seconds, bool bAccentSub = false);
	/** A narration line for Seconds. */
	void ShowLine(const FText& Words, float Seconds);
	/** Whatever is on screen fades out quickly. */
	void ClearWords();
	/** The bars in (eased) or out; bInstant: at once (under the white). */
	void SetLetterbox(bool bShown, bool bInstant = false);

	/** Takes the keyboard and pad for the skip keys (the menu world), shows the skip prompt when one is pressed. */
	void SetTakesKeys(bool bTakes);

	/**
	 * Where a scene holds the keys (the Rest's part): reads the hold keys the scene doesn't (Enter, Space, the pad's
	 * bottom face button, unless one is Interact, the scene's own) from Controller's key states, so the same keys skip in
	 * both parts; its prompt shows as the menu's does. Null stops it.
	 */
	void SetPollsKeys(APlayerController* Controller);

	/** The skip keys' hold keys besides the player's Interact key (Enter, Space, the pad's A), and how the prompt names one. */
	static const TArray<FKey>& ExtraHoldKeys();
	static FString HoldKeyLabel(const FKey& Key);
	/** A skip was asked for (once). */
	bool ConsumeSkip();

	/** What shows now, for the tests. */
	struct FShown
	{
		FString CardTitle;
		float CardAlpha = 0.f;
		FString Line;
		float LineAlpha = 0.f;
		float Letterbox = 0.f;
	};
	FShown GetShown() const;

	/** Seconds a card and a line take to fade in and out. */
	static constexpr float CardFade = 0.7f;
	static constexpr float LineFade = 0.4f;
	/** Seconds the bars take to come in or go. */
	static constexpr float LetterboxSeconds = 0.8f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void UpdateLetterbox(const FGeometry& MyGeometry, float DeltaSeconds);
	void UpdateWords(float DeltaSeconds);

	UPROPERTY(Transient) TObjectPtr<USizeBox> TopBar;
	UPROPERTY(Transient) TObjectPtr<USizeBox> BottomBar;
	UPROPERTY(Transient) TObjectPtr<UWidget> Card;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CardTitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CardSubText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LineText;
	/** The line's slot: it rides above the bottom bar as the bar grows. */
	UPROPERTY(Transient) TObjectPtr<UOverlaySlot> LineSlot;
	UPROPERTY(Transient) TObjectPtr<USceneSkipPromptWidget> SkipPrompt;

	float CardElapsed = 0.f;
	float CardSeconds = 0.f;
	float LineElapsed = 0.f;
	float LineSeconds = 0.f;
	/** After a clear: the seconds left of the quick fade out (negative: no clear under way). */
	float ClearLeft = -1.f;

	float Letterbox = 0.f;
	float LetterboxTarget = 0.f;
	float LastBarHeight = -1.f;

	/** Reads Controller's held keys each frame (SetPollsKeys). */
	void PollKeys();

	bool bTakesKeys = false;
	bool bSkipTaken = false;
	TWeakObjectPtr<APlayerController> PolledController;
	/** The player's Interact key: the menu's hold key, and in a scene the scene's own (left to it). */
	FKey InteractKey;
	/** The polled key being held (none: none). */
	FKey PolledDown;
	FIntroSkipKeys Skip;
};
