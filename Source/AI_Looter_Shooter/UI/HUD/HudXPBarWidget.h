#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudXPBarWidget.generated.h"

class UHorizontalBoxSlot;
class UImage;
class UTextBlock;
class UPlayerProgressionSubsystem;
enum class EXPSource : uint8;

DECLARE_DELEGATE_OneParam(FOnHudAnnouncement, const FText& /*Message*/);

/**
 * The HUD's experience bar, bottom center, the only place the level shows:
 *  - the level number in a circle in the bar's middle, ringed in the accent color (the HUD's circles, like the round
 *    weapon slots), at the screen's center line
 *  - the bar in two halves passing behind the circle, filling left to right through it: slim, leaning like the HUD's
 *    other bars, a faint track inside a thin dark edge, the earned part in the accent color with a lighter leading
 *    edge, and quiet notches at every tenth of the level (the circle marks the half)
 *  - "40 / 100 XP" ("MAX" at the top level), small and dim over the bar's right end
 * No backing panel, like the rest of the gameplay HUD: only the bar's dark edge and the circle's inside fade with the UI
 * transparency setting. On a gain the newly earned stretch lights up at once, holds a moment, and the bar catches up to
 * it with its leading edge glowing, while "+10 XP" rises beside the numbers. On a level-up the bar fills and wraps, the
 * ring sends out a pulse, the ring and number flash, and the HUD announces the new level. Driven by
 * UPlayerProgressionSubsystem's events: it builds strings only when experience changes and does nothing per frame unless
 * something is moving.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudXPBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The bar's length on screen, both halves together (the level circle sits between them). A multiple of ten, so the
	 *  tenths fall on whole pixels. */
	static constexpr float BarWidth = 780.f;

	/** A level-up to announce ("LEVEL UP! LEVEL 2"); the HUD shows it in its message plate. */
	FOnHudAnnouncement OnAnnouncement;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void HandleXPChanged(int64 Gained, EXPSource Source);
	void HandleLevelUp(int32 NewLevel);

	/** Aims the bar at the player's current progress; bSnap jumps there instead of easing. */
	void Retarget(bool bSnap);

	/** Draws ShownProgress: the level number and the bar, with the stretch still to catch up to. */
	void ShowProgress();
	/**
	 * The earned part up to Fraction and the just-earned stretch from there to GainEnd, both 0-1 of the whole bar: the
	 * left half shows the first half of it, the right half the rest.
	 */
	void PaintBar(float Fraction, float GainEnd);
	/** Colors each tenth's notch by whether it sits on the lit part of the bar, which runs up to LitEnd (0-1). */
	void PaintTicks(float LitEnd);
	void SetShownLevel(int32 Level);
	void UpdateXPText();
	/** The ring's and number's colors; Flash runs from 1 (just leveled up) to 0 (resting). */
	void PaintBadge(float Flash);
	/** The leading edge's color; Glow runs from 1 (just earned) to 0 (resting). */
	void PaintEdge(float Glow);

	UPROPERTY(Transient) TObjectPtr<UWidget> Cluster;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelValue;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> XPText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GainText;
	/** The level circle's ring, and the ring that spreads out of it on a level-up. */
	UPROPERTY(Transient) TObjectPtr<UImage> BadgeRing;
	UPROPERTY(Transient) TObjectPtr<UImage> BadgePulse;
	/** The light line at the earned part's leading edge, one per half, left then right; only the half it ends in shows it. */
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> FillEdges;
	/** The notches between the bar's tenths, left to right (none in the middle, where the circle is). */
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> TickMarks;
	/** Per half, left then right, the slots that share its width: the earned part, the just-earned stretch and the rest. */
	UPROPERTY(Transient) TArray<TObjectPtr<UHorizontalBoxSlot>> FilledSlots;
	UPROPERTY(Transient) TArray<TObjectPtr<UHorizontalBoxSlot>> GainedSlots;
	UPROPERTY(Transient) TArray<TObjectPtr<UHorizontalBoxSlot>> RestSlots;

	TWeakObjectPtr<UPlayerProgressionSubsystem> Progression;

	/** Level plus the fraction through it: what the bar shows now, and what it eases toward. */
	double ShownProgress = -1.0;
	double TargetProgress = 0.0;
	bool bMaxLevel = false;

	int32 ShownLevel = INDEX_NONE;
	/** The XP text waits for the bar to reach the new level, so the numbers and the bar agree. */
	bool bXPTextPending = false;
	/** The highest level gained but not yet announced; announced when the bar gets there. */
	int32 PendingAnnouncement = 0;

	/** How far the earned part and the just-earned stretch are drawn (0-1), so the bar only repaints when they change. */
	float ShownFill = -1.f;
	float ShownGainEnd = -1.f;
	/** How many notches are drawn dark (on the lit part), so they only repaint when one crosses the lit end. */
	int32 ShownLitTicks = INDEX_NONE;

	int64 ShownGain = 0;
	float GainTime = 0.f;
	/** After a gain the bar waits this long before catching up, so the just-earned stretch reads on its own. */
	float GainHoldTime = 0.f;
	/** How brightly the leading edge glows: 1 just after a gain, fading once the bar has caught up. */
	float EdgeGlow = 0.f;
	float FlashTime = 0.f;
	float PulseTime = 0.f;
	float Activity = 0.f;
};
