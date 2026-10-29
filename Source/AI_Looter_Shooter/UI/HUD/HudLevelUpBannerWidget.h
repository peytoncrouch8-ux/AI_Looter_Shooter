#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudLevelUpBannerWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * The level-up popup near the top of the screen, concept 5 "Signal Glass" (Docs/Work/2026-10-29-Morning/ui3/c5.png, the
 * user's pick 2026-10-29): no glass, only floating outlined type and salt hairlines. "LEVEL UP" wide-spaced, a short gold
 * bar under it, the new level as one big number between two salt hairlines (small gold arrowheads by the number), and
 * the level's reward ("MAX HEALTH +8%", from the progression settings) under it.
 * The show: the hairlines wipe out from the middle while the old level stands dim, the number counts old -> new with one
 * soft gold flash as the UI.LevelUp cue's chord strikes, the gold bar draws last, it holds about 2 s and fades; a new
 * level-up restarts it. As it starts, the world shows it too: a ring of light bursts out along the ground round the
 * player and embers rise (ULevelUpBurstSubsystem).
 * Its root is collapsed while it waits (the other banners read that to take turns with it); it costs nothing hidden.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudLevelUpBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Its box, centred where the HUD places it: the title to the reward, and the hairlines either side of a two-digit
	 *  level. Kept under 768 wide so the mission tracker clears it (Looter.UI.HUD). */
	static constexpr float Width = 760.f;
	static constexpr float Height = 420.f;

	/** Plays the show for a new level (counting up from the one before); a new call restarts it. */
	void Show(int32 NewLevel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Puts the level's reward into its text and spaces the hairlines round the widest number the count shows. */
	void ApplyTexts();

	/** Poses every piece for Time seconds into its show (wipe, count, flash, bar, fades). */
	void PaintShow();

	UPROPERTY(Transient) TObjectPtr<UWidget> Banner;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RewardText;
	UPROPERTY(Transient) TObjectPtr<UWidget> TitleText;
	UPROPERTY(Transient) TObjectPtr<UWidget> GoldBar;
	UPROPERTY(Transient) TObjectPtr<UWidget> Flash;
	UPROPERTY(Transient) TObjectPtr<UWidget> LeftLine;
	UPROPERTY(Transient) TObjectPtr<UWidget> RightLine;
	UPROPERTY(Transient) TObjectPtr<UWidget> LeftArrow;
	UPROPERTY(Transient) TObjectPtr<UWidget> RightArrow;

	/** The level shown, the one the count starts from, and seconds into the show (below zero while hidden). */
	int32 Level = 0;
	int32 FromLevel = 0;
	float Time = -1.f;
	/** The number the big text holds now (none: set it on the next paint). */
	int32 ShownNumber = INDEX_NONE;
};
