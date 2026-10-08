#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "UI/Style/LooterUIStyle.h"
#include "LooterButton.generated.h"

class UImage;
class UTextBlock;
class ULooterButton;

DECLARE_DELEGATE_OneParam(FOnLooterButtonClicked, ULooterButton* /*Button*/);

/**
 * Code-built button in the shared UI style (chamfered, outlined). Carries an action id + index so one
 * handler can serve a whole list. Never takes keyboard focus, so WASD keeps reaching the game after clicking.
 * It sounds as it's pointed at (UI.Hover) and clicked (ClickCue, or by its action and kind: ClickCueFor).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULooterButton : public UButton
{
	GENERATED_BODY()

public:
	ULooterButton();

	/** Builds the label and outline and wires the click. Call once right after constructing the button. */
	void Setup(UTextBlock* InLabel, FName InAction, int32 InIndex, const FText& Text, int32 FontSize = 12,
		LooterUI::EButtonKind InKind = LooterUI::EButtonKind::Normal);

	/** Like Setup, but the button shows any content instead of a text label (and no outline of its own). */
	void SetupContent(UWidget* Content, FName InAction, int32 InIndex, LooterUI::EButtonKind InKind = LooterUI::EButtonKind::Bare);

	void SetLabel(const FText& Text);
	void SetHighlighted(bool bHighlighted);

	/**
	 * A click's sound: UI.Back for a button that closes or goes back (its action Close or Back, or one starting Cancel),
	 * UI.Tab for a tab or a switch's segment, UI.Click for everything else.
	 */
	static FName ClickCueFor(FName InAction, LooterUI::EButtonKind InKind);

	FName Action;
	int32 Index = INDEX_NONE;
	/** The sound its click makes; none: ClickCueFor its action and kind. */
	FName ClickCue;
	/** Off: it makes no sound of its own (a screen that plays its own clicks and hovers, so they don't double). */
	bool bPlaysSounds = true;
	FOnLooterButtonClicked OnButtonClicked;
	/** The mouse moved onto the button. */
	FOnLooterButtonClicked OnButtonHovered;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	void ApplyStyle();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Outline;

	LooterUI::EButtonKind Kind = LooterUI::EButtonKind::Normal;
	bool bIsHighlighted = false;
	bool bUppercase = true;
	FText RawText;
};
