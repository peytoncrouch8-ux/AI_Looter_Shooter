#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "XpPopupWidget.generated.h"

class UTextBlock;

/**
 * A floating "+38 XP" (AXpPopupActor carries it): the HUD's floating text (Chakra Petch, outlined, no plate) in the
 * experience's cyan, as the player frame's own "+XP" and the mission banner's reward. A kill's is small, beside its
 * damage numbers; a turn-in's big. Builds its own tree, so no Widget Blueprint is needed.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UXpPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Safe before the widget is built; shown once it is. */
	void SetAmount(int64 Amount, bool bBig);

	/** What it says for Amount: "+38 XP", "+1,240 XP". */
	static FString Words(int64 Amount);

	/** Type sizes: a kill's under a body shot's damage number (22), a turn-in's well over a crit's (36 is the crit's). */
	static constexpr int32 SmallSize = 18;
	static constexpr int32 BigSize = 32;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyText();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AmountText;

	int64 Amount = 0;
	bool bBig = false;
};
