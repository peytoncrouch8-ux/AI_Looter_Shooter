#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponLabelWidget.generated.h"

class AWeaponBase;
class UImage;
class UTextBlock;

/**
 * Floating label over a weapon lying in the world: rarity-colored name, key stats, and a pickup prompt. A named gun
 * (Heirloom) shows its own name, and its flavor line under it while it's looked at.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UWeaponLabelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Safe to call before the widget has been built. */
	void SetWeapon(const AWeaponBase* Weapon);
	void SetFocused(bool bFocused);

	/** What the label shows once built (for the tests): the name, and a named gun's line (empty for any other gun). */
	FText GetNameText() const;
	FText GetFlavorText() const;

	/** The flavor line is on screen: a named gun's label, looked at. */
	bool IsFlavorShown() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyContent();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	/** A named gun's line, under its name. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FlavorText;

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
	FText Flavor;
	FText Stats;
	FLinearColor NameColor = FLinearColor::White;
	bool bIsFocused = false;
};
