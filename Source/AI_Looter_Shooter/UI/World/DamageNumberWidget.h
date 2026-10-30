#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Elements/ElementCallout.h"
#include "DamageNumberWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * A single floating damage number. Builds its own widget tree, so no Widget Blueprint is needed: the number, and beside it
 * (collapsed unless the hit has one) a small WEAK or RESIST tag in the element's colour (greyed for a resist). An element's
 * look (FElementNumberLook) colours the number and sizes it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Safe to call before the widget has been built; the value is applied once it is. */
	void SetDamage(float Damage, bool bCritical, const FElementNumberLook& Look = FElementNumberLook());

	/** A crit's number white-hot (1) cooling into the crit colour (0), as it slams in. */
	void SetHeat(float Heat);

	/** The tag's size while it pops in (1: settled); nothing without a tag. */
	void SetTagScale(float Scale);

	/** A stream's number rolling up: only the number changes (its look stays the first piece's). */
	void SetValue(float Damage);

	/**
	 * A status's first-time word instead of a number: its glyph (GunCardIcons::ElementBrush) then its word
	 * (ElementRules::StatusWord, "BURNING") in the element's colour, spaced as a label.
	 */
	void SetCallout(ELooterElement Element);

	/** Type sizes: a crit's twice as loud as a body shot's; the tag is small beside either; a callout's word between. */
	static constexpr int32 NormalSize = 22;
	static constexpr int32 CriticalSize = 36;
	static constexpr int32 TagSize = 13;
	static constexpr int32 CalloutSize = 17;
	static constexpr int32 CalloutLetterSpacing = 120;
	static constexpr float CalloutGlyphSize = 18.f;

	/** The size the number is set in for a hit (a weak hit's a little bigger). */
	static int32 NumberSize(bool bCritical, const FElementNumberLook& Look);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyDamageText();
	void ApplyCallout();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DamageText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TagText;

	/** A callout's glyph before its word (collapsed for a number). */
	UPROPERTY(Transient)
	TObjectPtr<UImage> GlyphImage;

	/** The callout's element (None: a number). */
	ELooterElement CalloutElement = ELooterElement::None;
	float DisplayedDamage = 0.f;
	bool bDisplayedCritical = false;
	float DisplayedHeat = -1.f;
	FElementNumberLook DisplayedLook;
	float DisplayedTagScale = 1.f;
};
