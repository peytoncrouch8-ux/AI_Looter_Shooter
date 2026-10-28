#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponLabelWidget.generated.h"

class AWeaponBase;
class UGunCardWidget;
class UImage;
class UInteractionComponent;
class UTextBlock;
class UWeaponManagerComponent;
class UWidget;

/**
 * Floating label over a weapon lying in the world. From afar: its rarity-coloured name, its element and effect line, and a
 * cursed iron's cracked coin and curse (the name keeps its rarity colour). Looked at (the user's call, 2026-10-28: the
 * card over the gun, not on the screen's right): the gun card (UGunCardWidget, World) rises over the gun, compared with
 * the gun in hand and with the take keys, its foot on a 2 px cyan tether down to a hollow diamond in the rarity's colour
 * where the name hung; a named gun's line floats over the card. One of many guns lying together may be folded to a dot in
 * its rarity's colour (SetFolded). WeaponLabelWidgetCard.cpp runs the card.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UWeaponLabelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Safe to call before the widget has been built. */
	void SetWeapon(const AWeaponBase* InWeapon);
	void SetFocused(bool bFocused);

	/**
	 * Fills the looked-at card: compared with the gun in Manager's hand (none: nothing to compare), its keys from Manager's
	 * slots and backpack and Interaction's hold on this gun. Either may be null. NativeTick calls it every frame from the
	 * owning player's pawn while the label is looked at; tests call it.
	 */
	void UpdateCard(const UWeaponManagerComponent* Manager, const UInteractionComponent* Interaction);

	/** The looked-at card (null before the label is built), and whether it is on screen (tests). */
	UGunCardWidget* GetCard() const { return Card; }
	bool IsCardShown() const;

	/**
	 * Folds it to a dot in its rarity's colour (one of many guns lying together: ULootLabelSubsystem shows a few names in
	 * full and folds the rest, so they never stack into a tower), or opens it again. Looking at the gun opens it.
	 */
	void SetFolded(bool bInFolded);
	bool IsFolded() const { return bFolded; }

	/** The dot is on screen in place of the name (tests). */
	bool IsDotShown() const;

	/** The size the dot is drawn at (its halo's, widget units); zero while no dot shows (tests: what the player sees). */
	FVector2D GetDotSize() const { return ShownDotSize; }

	/** The label is the looked-at one (its card shows). */
	bool IsFocusedLabel() const { return bIsFocused; }

	/** What the label shows once built (for the tests): the name, and a named gun's line (empty for any other gun). */
	FText GetNameText() const;
	FText GetFlavorText() const;

	/** The flavor line is on screen: a named gun's label, looked at (over its card). */
	bool IsFlavorShown() const;

	/** A cursed iron's curse line ("HUNGRY", "HUNGRY · LIFTED"; empty for any other gun), and whether it's on screen. */
	FText GetCurseText() const;
	bool IsCurseShown() const;

	/** Its element and effect line ("EMBER · RICOCHET"; empty for a gun with neither), shown from afar in its colour. */
	FText GetTraitText() const;

	/** Its maker ("MERIDIAN"; empty for a named gun or one without a maker): on the card's sweep while looked at. */
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

	/** The tether from the card's foot down to the diamond (widget units), and the diamond's size. */
	static constexpr float TetherLength = 40.f;
	static constexpr float TetherWidth = 2.f;
	static constexpr float DiamondSize = 12.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyContent();

	/** Builds the looked-at stack (the line, the card, the tether, the diamond) into the tree (WeaponLabelWidgetCard.cpp). */
	UWidget* BuildFocusStack();

	/** UpdateCard from the owning player's pawn (the first player's when the label has no owner). */
	void UpdateCardFromViewer();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	/** A named gun's line, over its card while looked at. */
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

	/** The name and its lines, shown from afar (not looked at, not folded). */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> Lines;

	/** Looked at: the flavor line, the card, the tether and the diamond, bottom to top from where the name hung. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> FocusStack;

	UPROPERTY(Transient)
	TObjectPtr<UGunCardWidget> Card;

	/** The hollow diamond at the tether's foot, in the rarity's colour. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> TetherDiamond;

	/** The folded label: a dot in the rarity's colour where the name would hang (a dark halo under a lit core, so it reads over any ground and in a beam's glow). */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> FoldedDot;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FoldedHalo;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FoldedCore;

	FText Name;
	FText Flavor;
	FText Curse;
	FText Trait;
	FText Maker;
	FLinearColor NameColor = FLinearColor::White;
	FLinearColor TraitColor = FLinearColor::White;

	/** The gun shown (SetWeapon), for the card; and the gun itself, whose hold the card's HOLD cap fills with. */
	UPROPERTY(Transient)
	FWeaponInstanceData Gun;
	TWeakObjectPtr<const AWeaponBase> Weapon;
	bool bHasGun = false;
	/** The card needs the gun again (SetWeapon since it last took it), or the gun in hand changed. */
	bool bCardStale = true;
	/** The gun in hand the card compares with now, and whether it compares at all (the gun may be gone since). */
	TWeakObjectPtr<const AWeaponBase> ComparedWith;
	bool bCompared = false;
	/** The card's intro plays at its next fill: the gun was just looked at. */
	bool bIntroPending = false;
	/** When the label last stopped being looked at (real seconds): a look back within a blink plays no intro again. */
	double LostFocusAt = -1.0e9;

	bool bIsFocused = false;
	/** A drop's name pop (SetPop): name and trait line only, bigger. */
	bool bPop = false;
	/** Folded to its dot (SetFolded); never while looked at or popping. */
	bool bFolded = false;
	/** The dot's drawn size while it shows (GetDotSize), else zero. */
	FVector2D ShownDotSize = FVector2D::ZeroVector;
};
