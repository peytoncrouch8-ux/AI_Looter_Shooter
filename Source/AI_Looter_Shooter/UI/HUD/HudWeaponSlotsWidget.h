#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudWeaponSlotsWidget.generated.h"

class UImage;
class UTextBlock;
class UWeaponManagerComponent;
class UWidget;

/**
 * The HUD's weapon slots (bottom-right), a column of circles with slot 1 on top, beside the magazine cartridge. Each row
 * is the ammo icon, the key tab and the circle, left to right:
 *  - the circle, in layers: a dark rim, a gunmetal ring lit from the top, the dark inner disc, the rarity arc along its
 *    bottom and a cyan hairline, with the gun's Inked icon inside, tilted up so its ends reach the ring
 *  - the key tab, a small chamfered plate: orange with a dark number for the gun in hand, dark glass otherwise
 *  - the Inked icon of the ammo the gun takes, so each slot shows which ammo pool it draws from
 *  - the gun in hand: the whole row moves left toward the screen's middle and grows, the circle gets an accent ring and
 *    a soft orange glow, and its icons show at full strength (the others are dimmed); switching eases it over
 *  - an empty slot: a faint rim round a dashed ring, its tab dimmed, and no ammo icon
 * No backing panels: the inner discs and the tabs' glass fade with the UI transparency setting; the rings, icons and
 * numbers stay. The shapes are vector pictures drawn once into shared textures; per frame it only checks whether
 * anything changed and eases the slot in hand.
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
	/** A slot's circle, and how far apart the circles stand, centre to centre (the column's rows). */
	static constexpr float SlotDiameter = 64.f;
	static constexpr float SlotSpacing = 76.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	struct FSlotWidgets
	{
		UWidget* Root = nullptr;
		/** The row (ammo icon, tab and circle), which moves left and grows when the slot is in hand. */
		UWidget* Lifted = nullptr;
		UImage* Glow = nullptr;
		UImage* Disc = nullptr;
		UImage* Rim = nullptr;
		UImage* Metal = nullptr;
		UImage* Arc = nullptr;
		UImage* Hairline = nullptr;
		UImage* AccentRing = nullptr;
		UImage* Dashed = nullptr;
		UImage* Gun = nullptr;
		UImage* TabGlass = nullptr;
		UImage* TabLit = nullptr;
		UImage* TabEdge = nullptr;
		UTextBlock* TabNumber = nullptr;
		/** The Inked icon of the gun's ammo type, left of the tab. */
		UImage* Ammo = nullptr;
		/** 0 resting, 1 in hand (moved left and grown), eased. */
		float Lift = 0.f;
		/** What it shows, to repaint only on change (nothing yet: the first update always paints). */
		uint32 Shown = MAX_uint32;
	};

	void Paint(FSlotWidgets& Cell, int32 Index, const UWeaponManagerComponent* Manager, bool bInHand);

	TArray<FSlotWidgets> Slots;
};
