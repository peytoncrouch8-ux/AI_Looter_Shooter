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
 * The HUD's experience bar, bottom center: the player's level, a slim slanted bar in ten sections (each a tenth of the
 * level, the one being filled filling along its length) of the way through it, and
 * "XP 40 / 100" ("MAX" at the top level). No backing panel, like the rest of the gameplay HUD. On a gain the bar eases
 * up to the new amount and "+10 XP" rises beside it; on a level-up it fills, wraps, the level number flashes and the HUD
 * announces the new level. Driven by UPlayerProgressionSubsystem's events: it builds strings only when experience
 * changes and does nothing per frame unless something is moving.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudXPBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The bar's length on screen. */
	static constexpr float BarWidth = 440.f;

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

	/** Draws ShownProgress: the level number and the bar. */
	void ShowProgress();
	void PaintBar(float Fraction);
	void SetShownLevel(int32 Level);
	void UpdateXPText();

	UPROPERTY(Transient) TObjectPtr<UWidget> Cluster;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelValue;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> XPText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GainText;
	/** Per section: its filled part, and the slots that share its width between the filled part and the rest. */
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SectionFills;
	UPROPERTY(Transient) TArray<TObjectPtr<UHorizontalBoxSlot>> FilledSlots;
	UPROPERTY(Transient) TArray<TObjectPtr<UHorizontalBoxSlot>> EmptySlots;

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

	/** Per section, how full it is (0-1), so only the ones that change get repainted. */
	TArray<float> SectionFill;

	int64 ShownGain = 0;
	float GainTime = 0.f;
	float FlashTime = 0.f;
	float Activity = 0.f;
};
