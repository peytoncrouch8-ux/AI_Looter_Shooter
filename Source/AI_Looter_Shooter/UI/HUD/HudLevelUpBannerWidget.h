#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudLevelUpBannerWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * The level-up banner near the top of the screen (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "Level up"): the new
 * level in a 92 px cyan gem with a soft glow and eight orange rays behind it, "LEVEL UP" in orange between fading cyan
 * rules under it, and the level's reward ("MAX HEALTH +8%", from the progression settings). It pops in, holds and fades
 * over 2.8 s; a new level-up restarts it. Its box is centred on the gem, so placing the box's middle places the gem.
 * Nothing in it is a background; it costs nothing while hidden.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudLevelUpBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Its box: wide enough for the title and its rules, the gem in its middle. */
	static constexpr float Width = 700.f;
	static constexpr float Height = 340.f;

	/** Pops in, holds and fades over 2.8 s; a new call restarts it. */
	void Show(int32 NewLevel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Puts Level and the level's reward into the texts. */
	void ApplyTexts();

	/** Scales and fades the whole banner for Time seconds into its show. */
	void PaintShow();

	UPROPERTY(Transient) TObjectPtr<UWidget> Banner;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RewardText;

	/** The level shown, and seconds into the show (below zero while hidden). */
	int32 Level = 0;
	float Time = -1.f;
};
