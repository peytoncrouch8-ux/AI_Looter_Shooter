#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SceneSkipPromptWidget.generated.h"

class ULooterProgressRing;
class UTextBlock;
class UWidget;

/**
 * How to skip the scene playing, in the bottom right corner as floating outlined text with no panel (the gameplay HUD's
 * rule): "HOLD (E) TO SKIP" with the key inside a round progress ring that fills while the key is held (and pops, with the
 * Hold::Complete cue, as it fills), or "PRESS [ESC] AGAIN TO SKIP" after a first Escape. It shows only once the player
 * presses one of them, and fades out a few seconds after (USceneSubsystem drives it).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API USceneSkipPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * The prompt shown (fading in) or not (fading out): Lead ("HOLD", "PRESS"), then Key, then Words. A Lead of "HOLD" puts the
	 * key inside the progress ring, filled to Hold (0-1; a Hold of 1 is the skip: the ring pops once); any other lead shows
	 * Key in brackets in the accent color.
	 */
	void Update(bool bShown, const FString& Lead, const FString& Key, const FString& Words, float Hold, float DeltaSeconds);

	/** Over the HUD's widgets and the tutorial's prompt, under the menus. */
	static constexpr int32 ViewportZOrder = 6;

	/** The lead word of the hold variant: the one that wears the ring. */
	static bool IsHoldLead(const FString& Lead);

	/** What the prompt shows now, for the tests. */
	struct FShown
	{
		bool bVisible = false;
		/** The ring is showing round the key (the hold variant). */
		bool bRing = false;
		float RingProgress = 0.f;
		bool bRingPopping = false;
		/** The key as it shows: inside the ring, or in brackets beside the lead. */
		FString Key;
	};
	FShown GetShown() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient) TObjectPtr<UWidget> Prompt;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LeadText;
	/** The key in brackets, for a press. */
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyText;
	/** The key inside its ring, for a hold: the whole cap, the ring and the key. */
	UPROPERTY(Transient) TObjectPtr<UWidget> HoldCap;
	UPROPERTY(Transient) TObjectPtr<ULooterProgressRing> HoldRing;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HoldKey;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WordsText;

	/** The hold reached its end and popped: not again until the key is let go. */
	bool bHoldDone = false;

	/** Shown opacity, eased toward 1 while the prompt is wanted. */
	float Opacity = 0.f;
};
