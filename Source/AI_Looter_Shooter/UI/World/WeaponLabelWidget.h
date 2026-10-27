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
 * and the curse's name under its name, from afar too (the name keeps its rarity color). Looked at, its maker's mark and
 * name sit over its name (Affixes/WeaponMakers). One of many guns lying together may be folded to a dot in its rarity's
 * colour (SetFolded).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UWeaponLabelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Safe to call before the widget has been built. */
	void SetWeapon(const AWeaponBase* Weapon);
	void SetFocused(bool bFocused);

	/**
	 * Folds it to a dot in its rarity's colour (one of many guns lying together: ULootLabelSubsystem shows a few names in
	 * full and folds the rest, so they never stack into a tower), or opens it again. Looking at the gun opens it.
	 */
	void SetFolded(bool bInFolded);
	bool IsFolded() const { return bFolded; }

	/** The dot is on screen in place of the name (tests). */
	bool IsDotShown() const;

	/** What the label shows once built (for the tests): the name, and a named gun's line (empty for any other gun). */
	FText GetNameText() const;
	FText GetFlavorText() const;

	/** The flavor line is on screen: a named gun's label, looked at. */
	bool IsFlavorShown() const;

	/** A cursed iron's curse line ("HUNGRY", "HUNGRY · LIFTED"; empty for any other gun), and whether it's on screen. */
	FText GetCurseText() const;
	bool IsCurseShown() const;

	/** Its element and effect line ("EMBER · RICOCHET"; empty for a gun with neither), shown from afar in its colour. */
	FText GetTraitText() const;

	/** Its maker line ("MERIDIAN"; empty for a named gun or one without a maker), shown over the name while looked at. */
	FText GetMakerText() const;
	bool IsMakerShown() const;

	/**
	 * Makes it a drop's name pop (a Legendary's, ULootFanfareSubsystem): the name alone, big, with its element and effect
	 * line, popping up (overshooting, settling) and fading out over its last moment. Age and Seconds: how long it has
	 * shown, of how long it shows.
	 */
	void SetPop(float Age, float Seconds);

	/** The pop's size and opacity at Age of Seconds (tests). */
	static float PopScaleAt(float Age);
	static float PopOpacityAt(float Age, float Seconds);

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

	/** Its element and effect, under its name (and its curse); collapsed for a gun with neither. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TraitText;

	/** Its maker's mark and name, over its name while looked at; collapsed for a gun without a maker. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> MakerRow;

	UPROPERTY(Transient)
	TObjectPtr<UImage> MakerGlyph;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MakerText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;

	/** Plate background, only shown while the player is looking at this weapon. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> PlateFill;

	UPROPERTY(Transient)
	TObjectPtr<UImage> PlateLine;

	/** Everything a label shows but the dot: its lines and the plate's. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> Lines;

	/** The folded label: a dot in the rarity's colour where the name would hang. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> FoldedDot;

	FText Name;
	FText Flavor;
	FText Curse;
	FText Stats;
	FText Trait;
	FText Maker;
	/** The maker's Inked mark's name (InkedIconData), none without a maker. */
	FName MakerIcon;
	FLinearColor NameColor = FLinearColor::White;
	FLinearColor TraitColor = FLinearColor::White;
	bool bIsFocused = false;
	/** A drop's name pop (SetPop): name and trait line only, bigger. */
	bool bPop = false;
	/** Folded to its dot (SetFolded); never while looked at or popping. */
	bool bFolded = false;
};
