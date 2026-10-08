#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponLabelWidget.generated.h"

class AWeaponBase;
class UImage;
class UTextBlock;
class UWidget;

/**
 * Floating label over a weapon lying in the world: rarity-colored name, key stats, and a pickup prompt. A named gun
 * (Heirloom) shows its own name, and its flavor line under it while it's looked at. A cursed iron adds the cracked coin
 * and the curse's name under its name, from afar too (the name keeps its rarity color).
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

	/** A cursed iron's curse line ("HUNGRY", "HUNGRY · LIFTED"; empty for any other gun), and whether it's on screen. */
	FText GetCurseText() const;
	bool IsCurseShown() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyContent();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	/** A named gun's line, under its name. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FlavorText;

	/** A cursed iron's coin and the curse's name, under the gun's name; collapsed for any other gun. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> CurseRow;

	UPROPERTY(Transient)
	TObjectPtr<UImage> CurseGlyph;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CurseText;

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
	FText Curse;
	FText Stats;
	FLinearColor NameColor = FLinearColor::White;
	bool bIsFocused = false;
};
