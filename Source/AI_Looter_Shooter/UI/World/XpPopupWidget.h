#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "XpPopupWidget.generated.h"

class UTextBlock;

/**
 * A floating "+38 XP" (AXpPopupActor carries it): the HUD's floating text (Chakra Petch, outlined, no plate) in the
 * experience's cyan, as the player frame's own "+XP" and the mission banner's reward. A kill's is small, beside its
 * damage numbers; a turn-in's big. A Hallowed or Ethereal kill's (the enemy tiers, 2026-10-20) is in the tier's colour
 * and a size up (TierSize): the kill's reward reads as the tier's. Builds its own tree, so no Widget Blueprint is needed.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UXpPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Safe before the widget is built; shown once it is. bTier: a top tier's kill, in TierColor at TierSize. */
	void SetAmount(int64 Amount, bool bBig, bool bTier = false, const FLinearColor& TierColor = FLinearColor::White);

	/** What it says for Amount: "+38 XP", "+1,240 XP". */
	static FString Words(int64 Amount);

	/** Type sizes: a kill's under a body shot's damage number (22), a turn-in's well over a crit's (36 is the crit's). */
	static constexpr int32 SmallSize = 18;
	static constexpr int32 BigSize = 32;
	/** A top tier's kill: as big as a body shot's number, so its reward stands out of a fight's numbers. */
	static constexpr int32 TierSize = 22;

	bool IsTier() const { return bTier; }
	const FLinearColor& GetTextColor() const { return TextColor; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyText();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AmountText;

	int64 Amount = 0;
	bool bBig = false;
	bool bTier = false;
	/** The colour it shows in: the experience's cyan, or a top tier's own. */
	FLinearColor TextColor = FLinearColor::White;
};
