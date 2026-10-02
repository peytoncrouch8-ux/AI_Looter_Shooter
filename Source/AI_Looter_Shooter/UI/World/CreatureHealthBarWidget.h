#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CreatureHealthBarWidget.generated.h"

class UCanvasPanel;
class USizeBox;
class UTextBlock;

/**
 * The tag over a creature in combat, in the gameplay HUD's style: no backing plate, just floating outlined text ("LV 1
 * Brown Spider") over a slim slanted health bar with a dark rim and a lit top edge. A ranked creature's word comes before
 * its name in its rank's color ("LV 2 Restless Brown Spider", the word blue), and a rank with no word (a boss) colors the
 * name instead. The bar is the same over every creature whatever its name or health: solid dark lines cut it into
 * quarters, so a glance says how far gone it is without a tough creature's bar turning into noise. A hit leaves a pale
 * chip of the health it took that drains away a moment later.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UCreatureHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Cheap to call every frame: the texts only change when one of these does. InRankWord is empty for Basic; InRankColor
	 * colors the word, or the name when there's no word (Basic's is the plain text color).
	 */
	void SetCreature(const FText& InName, int32 InLevel, const FText& InRankWord, const FLinearColor& InRankColor);

	/** Cheap to call every frame: the bar only moves when the share of health left changes. */
	void SetHealth(float Health, float MaxHealth);

	/** The bar's length on screen, whatever the creature's health. */
	static constexpr float BarWidth = 120.f;

	/** How many parts the dividers cut the bar into, at any health (4 = a line at every quarter). */
	static constexpr int32 Parts = 4;

	/** Where the dividers go across the bar (0-1): at every 1 / Parts of it, the same for every creature. */
	static TArray<float> DividerPositions();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyLabel();
	void ApplyBar();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelText;

	/** The rank's word ("Restless"), hidden for Basic. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RankText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	/** The lit part of the bar, and the chip of lost health trailing it. */
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> HealthFill;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ChipFill;

	FText CreatureName;
	int32 CreatureLevel = 1;
	FText RankWord;
	FLinearColor RankColor = FLinearColor::White;
	float Fraction = 1.f;
	/** The chip's end: snaps up with healing, holds a moment after a hit, then drains down to the health. */
	float GhostFraction = 1.f;
	float GhostHold = 0.f;
};
