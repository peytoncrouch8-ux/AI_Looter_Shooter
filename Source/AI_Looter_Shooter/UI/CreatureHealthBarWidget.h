#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CreatureHealthBarWidget.generated.h"

class UImage;
class UTextBlock;

/** Small floating plate over a creature in combat: name and a segmented health bar (LooterUI kit). */
UCLASS()
class AI_LOOTER_SHOOTER_API UCreatureHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetCreatureName(const FText& InName);
	void SetHealthFraction(float InFraction);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyHealth();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> Segments;

	FText CreatureName;
	float Fraction = 1.f;
	int32 ShownLit = INDEX_NONE;
};
