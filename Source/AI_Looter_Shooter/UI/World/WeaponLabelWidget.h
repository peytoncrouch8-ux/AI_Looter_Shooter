#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponLabelWidget.generated.h"

class AWeaponBase;
class UImage;
class UTextBlock;

/** Floating label over a weapon lying in the world: rarity-colored name, key stats, and a pickup prompt. */
UCLASS()
class AI_LOOTER_SHOOTER_API UWeaponLabelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Safe to call before the widget has been built. */
	void SetWeapon(const AWeaponBase* Weapon);
	void SetFocused(bool bFocused);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyContent();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;

	/** Plate background, only shown while the player is looking at this weapon. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> PlateFill;

	UPROPERTY(Transient)
	TObjectPtr<UImage> PlateLine;

	FText Name;
	FText Stats;
	FLinearColor NameColor = FLinearColor::White;
	bool bIsFocused = false;
};
