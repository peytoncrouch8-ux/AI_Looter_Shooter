#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "UI/LooterUIStyle.h"
#include "LooterButton.generated.h"

class UImage;
class UTextBlock;
class ULooterButton;

DECLARE_DELEGATE_OneParam(FOnLooterButtonClicked, ULooterButton* /*Button*/);

/**
 * Code-built button in the shared UI style (chamfered, outlined). Carries an action id + index so one
 * handler can serve a whole list. Never takes keyboard focus, so WASD keeps reaching the game after clicking.
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

	FName Action;
	int32 Index = INDEX_NONE;
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
