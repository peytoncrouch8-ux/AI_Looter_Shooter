#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialPromptWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/**
 * The tutorial's current instruction, near the top of the screen: a small "TUTORIAL 2 / 6" line and the instruction
 * under it, as floating outlined text (the gameplay HUD has no panels). Fades out and back in when the step changes,
 * and hides while a menu is open. ATutorialDirector drives it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UTutorialPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows step Index (0-based) of Count. */
	void ShowStep(int32 Index, int32 Count, const FString& Text);

	/** The closing line; it fades away after Seconds. */
	void ShowDone(const FString& Text, float Seconds);

	void HideNow();

	/** While a menu is open the prompt steps aside. */
	void SetSuppressed(bool bSuppressed);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Present(const FString& Header, const FString& Text);

	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Box;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BodyText;

	/** Shown opacity, eased toward the target; a new text waits for the old one to fade out. */
	float Opacity = 0.f;
	bool bWanted = false;
	bool bSuppressed = false;
	FString PendingHeader;
	FString PendingText;
	bool bPending = false;
	/** Counts down while the closing line shows; 0 = no limit. */
	float DoneTimer = 0.f;
};
