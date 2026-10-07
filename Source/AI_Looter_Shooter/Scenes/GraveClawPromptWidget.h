#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GraveClawPromptWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;

/**
 * The grave wake-up's prompt (Docs/Areas/RansomsRest.md, Main 1, step 2): "PRESS [SPACE BAR] TO CLAW OUT" low in the
 * middle of the screen, over three slim slanted pips that light one by one as the player claws, as floating outlined text
 * with no panel (the gameplay HUD's rule; the HUD itself is put away while a scene holds the player). The wake-up drives
 * it (GraveWake): shown while the player is still under the dirt, faded out once they're through.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGraveClawPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The prompt shown (fading in) or not (fading out): Key in brackets in the accent color, Lit of its pips lit. */
	void Update(bool bShown, const FString& Key, int32 Lit, float DeltaSeconds);

	/** One pip a claw. */
	static constexpr int32 Pips = 3;

	/** Over the HUD's widgets, beside the scene's skip prompt, under the menus. */
	static constexpr int32 ViewportZOrder = 6;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient) TObjectPtr<UWidget> Prompt;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyText;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> PipImages;

	float Opacity = 0.f;
	int32 ShownLit = -1;
};
