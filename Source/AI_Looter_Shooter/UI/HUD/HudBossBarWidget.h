#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudBossBarWidget.generated.h"

class UCanvasPanel;
class UImage;
class USizeBox;
class UTextBlock;
class UWidget;

/**
 * A boss's health at the top of the screen, in the gameplay HUD's style (no backing panel): its level and name in floating
 * outlined text over a slim slanted bar, and the current phase's name under it.
 *  - the bar leans like the HUD's other bars; a dark tick cuts it where each later phase starts (light once passed)
 *  - a hit drops it at once, and the lost part lingers as a pale chip, then drains (as the vitals bar does)
 *  - while the boss can't be hurt the bar greys, the name dims, and the line under it says why (its spell's hint)
 *  - a new phase's name flashes in the accent color, then settles
 * UBossComponent adds it to the viewport for its fight and drives it; it fades in and out, and steps aside while a menu
 * is open. Only the bar's dark rim and track are backgrounds (they fade with the UI transparency setting).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudBossBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The bar's length on screen, whatever the boss's health. */
	static constexpr float BarWidth = 620.f;

	/** Over the gameplay HUD (0), under the tutorial's prompt (5), the inventory (20) and the pause menu (40). */
	static constexpr int32 ViewportZOrder = 3;

	/**
	 * Shows a boss: its name (in NameColor, its rank's) and level, and a tick at each phase's share of health after the
	 * first (PhaseShares as UBossComponent's phases have them, highest first). The bar starts full.
	 */
	void SetBoss(const FText& InName, int32 InLevel, const FLinearColor& InNameColor, const TArray<float>& PhaseShares);

	/** Cheap to call every frame: the bar only moves when the share of health left changes. */
	void SetHealth(float Health, float MaxHealth);

	/** Cheap to call every frame: the phase's name under the bar, flashing when a later phase starts. */
	void SetPhase(int32 Index, const FText& InPhaseName);

	/** Cheap to call every frame: greys the bar while the boss can't be hurt, with what to do about it (may be empty). */
	void SetUntargetable(bool bInUntargetable, const FText& InHint);

	/** Fades it in, or out (it stays in the viewport, collapsed). */
	void SetShown(bool bInShown);
	bool IsShown() const { return bWanted; }

	// What it shows, for the tests (the text blocks' own text once it's built).
	FText GetNameText() const;
	FText GetLevelText() const;
	FText GetPhaseText() const;
	const TArray<float>& GetTickShares() const { return TickShares; }
	int32 GetTickCount() const { return TickImages.Num(); }
	bool IsGreyed() const { return bGreyed; }
	float GetFraction() const { return Fraction; }
	float GetChipFraction() const { return ChipFraction; }

	/** Where the ticks go along the bar (0-1 from its empty end): every phase's share but the first, inside the bar, highest first. */
	static TArray<float> MakeTickShares(const TArray<float>& PhaseShares);

	/** "LV 9". */
	static FText MakeLevelText(int32 Level);

	/** The line under the bar: the hint while untargetable (when there is one), otherwise the phase's name; in capitals. */
	static FText MakePhaseLine(const FText& PhaseName, bool bUntargetable, const FText& Hint);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyTexts();
	/** Makes the tick marks for TickShares anew. */
	void BuildTicks();
	/** Sizes the fill and the chip, and colors each tick by which part of the bar it sits on. */
	void ApplyFill();
	/** The fill's, name's and phase line's colors for how grey it is and the phase flash. */
	void ApplyColors();
	/** The inventory or the pause menu is up, or the game is paused: the bar steps aside. */
	bool IsMenuOpen() const;

	UPROPERTY(Transient) TObjectPtr<UWidget> Cluster;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PhaseLabel;
	/** The lit part of the bar and the chip of lost health trailing it, sized by width. */
	UPROPERTY(Transient) TObjectPtr<USizeBox> FillBox;
	UPROPERTY(Transient) TObjectPtr<USizeBox> ChipBox;
	UPROPERTY(Transient) TObjectPtr<UImage> FillImage;
	UPROPERTY(Transient) TObjectPtr<UImage> HighlightImage;
	/** Over the whole bar, rim included, so the ticks notch its edge too. */
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> TickLayer;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> TickImages;

	FText BossName;
	int32 BossLevel = 1;
	FLinearColor NameColor = FLinearColor::White;
	TArray<float> TickShares;
	int32 PhaseIndex = INDEX_NONE;
	FText PhaseName;
	FText Hint;
	bool bGreyed = false;

	/** The share of health shown, and the chip's end: it holds a moment after a hit, then drains down to it. */
	float Fraction = 1.f;
	float ChipFraction = 1.f;
	float ChipHold = 0.f;
	/** How grey the bar is drawn, easing between 0 and 1. */
	float Grey = 0.f;
	/** Seconds left of the phase line's flash. */
	float PhaseFlash = 0.f;

	bool bWanted = false;
	float Opacity = 0.f;
	/** Drawn widths, and how many ticks sit on the lit part, so it only repaints what moved. */
	float ShownFillWidth = -1.f;
	float ShownChipWidth = -1.f;
	int32 ShownLitTicks = INDEX_NONE;
};
