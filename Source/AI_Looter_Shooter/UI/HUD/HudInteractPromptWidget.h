#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudInteractPromptWidget.generated.h"

class UImage;
class UInteractionComponent;
class UTextBlock;
class UWidget;

/**
 * What the Interact key does to the thing in front of the player (a door, the bell, a lantern post; loot has its own
 * card), just under the crosshair as floating outlined text: "[E] OPEN THE DOOR", or "HOLD [E] RING THE BELL" over a slim
 * slanted bar that fills while the key is held. No panel behind it, as the gameplay HUD has none. It fades in and out
 * quickly as things come into and out of reach.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Shows what Interaction's focused thing offers, Key being the Interact key's name ("E"). Fades out when there's
	 * nothing to say: nothing focused, no words, or no Interaction (the loot card has it).
	 */
	void Update(const UInteractionComponent* Interaction, const FString& Key, float DeltaSeconds);

	/** How far under the middle of the screen the prompt's top sits: clear of the widest crosshair and the hit marker. */
	static constexpr float Gap = 52.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** One line of the prompt: an optional lead word ("HOLD"), the key in brackets in the accent color, then the words. */
	UWidget* MakeLine(const TCHAR* Lead, TObjectPtr<UTextBlock>& OutKey, TObjectPtr<UTextBlock>& OutWords);

	UPROPERTY(Transient) TObjectPtr<UWidget> Lines;
	UPROPERTY(Transient) TObjectPtr<UWidget> TapLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TapKey;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TapWords;
	UPROPERTY(Transient) TObjectPtr<UWidget> HoldLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HoldKey;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HoldWords;
	/** Fills while the key is held; hidden (keeping its room, so the words don't jump) otherwise. */
	UPROPERTY(Transient) TObjectPtr<UWidget> HoldBar;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> HoldSegments;

	/** Shown opacity, eased toward 1 while there's something to say. */
	float Opacity = 0.f;
};
