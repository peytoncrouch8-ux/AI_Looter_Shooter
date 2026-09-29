#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

/** A single floating damage number. Builds its own widget tree, so no Widget Blueprint is needed. */
UCLASS()
class AI_LOOTER_SHOOTER_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Safe to call before the widget has been built; the value is applied once it is. */
	void SetDamage(float Damage, bool bCritical);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyDamageText();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DamageText;

	float DisplayedDamage = 0.f;
	bool bDisplayedCritical = false;
};
