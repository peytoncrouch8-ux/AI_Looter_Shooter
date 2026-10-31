#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shields/ShieldTypes.h"
#include "ShieldLabelWidget.generated.h"

class AActor;
class UImage;
class UInteractionComponent;
class UPlayerShieldComponent;
class UTextBlock;
class UWardCardWidget;
class UWidget;

/**
 * The floating label over a ward lying as loot (AShieldPickup), as a gun's (UWeaponLabelWidget; the user, 2026-10-31:
 * "Wards should have a similar UI to the weapons"). From afar: the ward box's Inked icon and its name in its rarity's
 * colour, its trait's name under it. Looked at: the ward card (UWardCardWidget, World) rises over the box, compared with
 * the ward worn and with the take keys under it, its foot on a tether down to a hollow diamond in the rarity's colour
 * where the name hung. ShieldLabelWidgetCard.cpp runs the card.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UShieldLabelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The ward shown and the one worn to compare it with (null: none worn). Safe before the widget is built. */
	void SetWard(const FShieldInstanceData& InWard, const FShieldInstanceData* InWorn);
	void SetFocused(bool bFocused);

	/** The pickup it hangs over: the label watches the player's look at it every frame, so the card opens at once. */
	void SetSource(const AActor* InSource);

	/**
	 * Fills the looked-at card: compared with Shield's ward worn, its keys from Shield's room (the backpack's shared count)
	 * and Interaction's hold on the source. Either may be null. NativeTick calls it every frame while looked at; tests call it.
	 */
	void UpdateCard(const UPlayerShieldComponent* Shield, const UInteractionComponent* Interaction);

	/** The looked-at card (null before the label is built), and whether it is on screen (tests). */
	UWardCardWidget* GetCard() const { return Card; }
	bool IsCardShown() const;
	bool IsFocusedLabel() const { return bIsFocused; }

	/** What it shows once built (tests): the name, and the trait's name from afar. */
	FText GetNameText() const;
	FText GetTraitText() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyContent();

	/** Builds the looked-at stack (the card, the tether, the diamond) into the tree (ShieldLabelWidgetCard.cpp). */
	UWidget* BuildFocusStack();

	/** UpdateCard from the owning player's pawn (the first player's when the label has no owner); its look at the source too. */
	void UpdateFromViewer();

	UPROPERTY(Transient) TObjectPtr<UWidget> Lines;
	UPROPERTY(Transient) TObjectPtr<UImage> Icon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TraitText;
	UPROPERTY(Transient) TObjectPtr<UWidget> FocusStack;
	UPROPERTY(Transient) TObjectPtr<UWardCardWidget> Card;
	UPROPERTY(Transient) TObjectPtr<UImage> TetherDiamond;

	UPROPERTY(Transient)
	FShieldInstanceData Ward;

	/** The pickup the label hangs over (its focus is watched every frame). */
	TWeakObjectPtr<const AActor> Source;
	bool bIsFocused = false;
	/** The card's intro plays at its next fill: the ward was just looked at. */
	bool bIntroPending = false;
	/** When it last stopped being looked at (real seconds): a look back within a blink plays no intro again. */
	double LostFocusAt = -1.0e9;
	/** What was last shown, so a ward that hasn't changed isn't rebuilt every look. */
	uint32 ShownIdentity = 0;
};
