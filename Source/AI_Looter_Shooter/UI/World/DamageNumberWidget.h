#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Elements/ElementCallout.h"
#include "DamageNumberWidget.generated.h"

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

	/** Type sizes: a crit's twice as loud as a body shot's; the tag is small beside either. */
	static constexpr int32 NormalSize = 22;
	static constexpr int32 CriticalSize = 36;
	static constexpr int32 TagSize = 13;

	/** The size the number is set in for a hit (a weak hit's a little bigger). */
	static int32 NumberSize(bool bCritical, const FElementNumberLook& Look);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyDamageText();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DamageText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TagText;

	float DisplayedDamage = 0.f;
	bool bDisplayedCritical = false;
	float DisplayedHeat = -1.f;
	FElementNumberLook DisplayedLook;
	float DisplayedTagScale = 1.f;
};
