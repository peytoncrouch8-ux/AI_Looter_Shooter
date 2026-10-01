#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CreatureHealthBarWidget.generated.h"

class UCanvasPanel;
class USizeBox;
class UTextBlock;

/**
 * The tag over a creature in combat, in the gameplay HUD's style: no backing plate, just floating outlined text ("LV 1
 * Brown Spider") over a slim slanted health bar. The bar is the same width over every creature, so tags look alike
 * whatever the name or health: thin lines divide it every 100 health, so a tougher creature shows more, closer lines
 * rather than a longer bar. A hit leaves a pale chip of the health it took that drains away a moment later.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UCreatureHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Cheap to call every frame: the texts only change when the name or level does. */
	void SetCreature(const FText& InName, int32 InLevel);

	/** Cheap to call every frame: the dividers are only rebuilt when the maximum changes. */
	void SetHealth(float Health, float MaxHealth);

	/** The bar's length on screen, whatever the creature's health. */
	static constexpr float BarWidth = 120.f;

	/** Health between two dividers. */
	static constexpr float HealthPerDivider = 100.f;

	/**
	 * Where the dividers go across the bar (0-1), one every HealthPerDivider. A creature with so much health that they'd
	 * crowd closer than a few pixels gets one every ten times as much instead, and so on.
	 */
	static TArray<float> DividerPositions(float MaxHealth);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyLabel();
	void ApplyBar();
	void RebuildDividers();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	/** The lit part of the bar, and the chip of lost health trailing it. */
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> HealthFill;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ChipFill;

	/** The dividers, laid over the bar. */
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Dividers;

	FText CreatureName;
	float ShownMaxHealth = 0.f;
	int32 CreatureLevel = 1;
	float Fraction = 1.f;
	/** The chip's end: snaps up with healing, holds a moment after a hit, then drains down to the health. */
	float GhostFraction = 1.f;
	float GhostHold = 0.f;
};
