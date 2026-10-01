#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CreatureHealthBarWidget.generated.h"

class UImage;
class UTextBlock;

/** Small floating plate over a creature in combat: its level, name and a segmented health bar (LooterUI kit). */
UCLASS()
class AI_LOOTER_SHOOTER_API UCreatureHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Cheap to call every frame: the texts only change when the name or level does. */
	void SetCreature(const FText& InName, int32 InLevel);
	void SetHealthFraction(float InFraction);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyHealth();
	void ApplyLabel();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> Segments;

	FText CreatureName;
	int32 CreatureLevel = 1;
	float Fraction = 1.f;
	int32 ShownLit = INDEX_NONE;
};
