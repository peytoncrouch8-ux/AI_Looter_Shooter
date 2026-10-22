#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shields/ShieldTypes.h"
#include "ShieldLabelWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * The floating label over a ward lying as loot (AShieldPickup), as a gun's (UWeaponLabelWidget): from afar its name in its
 * rarity's colour over its Inked icon and its trait's name; looked at, a plate opens with its numbers (capacity, wait,
 * refill), green or red against the ward worn, its trait's line and the keys.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UShieldLabelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The ward shown and the one worn to compare it with (null: none worn). Safe before the widget is built. */
	void SetWard(const FShieldInstanceData& InWard, const FShieldInstanceData* InWorn);
	void SetFocused(bool bFocused);

	/** What it shows once built (tests): the name, and the numbers line. */
	FText GetNameText() const;
	FText GetStatsText() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyContent();

	UPROPERTY(Transient) TObjectPtr<UImage> PlateFill;
	UPROPERTY(Transient) TObjectPtr<UImage> PlateLine;
	UPROPERTY(Transient) TObjectPtr<UImage> Icon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TraitText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatsText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CompareText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PromptText;

	UPROPERTY(Transient)
	FShieldInstanceData Ward;

	/** The worn ward's numbers to compare with, if any. */
	bool bHasWorn = false;
	FShieldStats Worn;
	bool bIsFocused = false;
	/** What was last shown, so a ward that hasn't changed isn't rebuilt every look. */
	uint32 ShownIdentity = 0;
	uint32 ShownWorn = 0;
	bool bShownFocus = false;
};
