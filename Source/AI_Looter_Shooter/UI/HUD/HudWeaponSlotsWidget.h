#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudWeaponSlotsWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UWeaponManagerComponent;
class UWidget;

/**
 * The HUD's weapon slots, a row of hexagons over the ammo (bottom-right), one per slot, numbered by their keys:
 *  - a carried gun: outlined in its rarity color, the slot's lower edge striped in it, the gun's silhouette inside, and its
 *    ammo class under the hex ("AR")
 *  - the gun in hand: raised and a little larger, outlined in the accent color with a soft glow, tinted with its rarity,
 *    the silhouette white; switching eases the new slot up
 *  - an empty slot: a dashed outline
 * No backing panels; the hexes' fills fade with the UI transparency setting, the outlines and text stay. The hexes are
 * vector icons drawn once into shared textures; per frame it only checks whether anything changed and eases the lift.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudWeaponSlotsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows Manager's slots; call every frame (it only repaints when something changed). */
	void Update(const UWeaponManagerComponent* Manager, float DeltaTime);

	/** The most slots it shows. */
	static constexpr int32 MaxSlots = 4;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	struct FSlotWidgets
	{
		UWidget* Root = nullptr;
		/** The hex and its tab, which rise and grow when the slot is in hand. */
		UWidget* Lifted = nullptr;
		UImage* Glow = nullptr;
		UImage* Fill = nullptr;
		UImage* Stripe = nullptr;
		UImage* Outline = nullptr;
		UImage* BoldOutline = nullptr;
		UImage* Dashed = nullptr;
		UImage* GunShadow = nullptr;
		UImage* Gun = nullptr;
		UBorder* Tab = nullptr;
		UTextBlock* TabNumber = nullptr;
		UTextBlock* AmmoClass = nullptr;
		/** 0 resting, 1 raised (in hand), eased. */
		float Lift = 0.f;
		/** What it shows, to repaint only on change (nothing yet: the first update always paints). */
		uint32 Shown = MAX_uint32;
	};

	void Paint(FSlotWidgets& Cell, int32 Index, const UWeaponManagerComponent* Manager, bool bInHand);

	TArray<FSlotWidgets> Slots;
};
