#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HudWeaponSlotsWidget.generated.h"

class UImage;
class UTextBlock;
class UWeaponManagerComponent;
class UWidget;
enum class EGunSlotLock : uint8;

/** The pictures a slot's layers are drawn from (HudWeaponSlotsWidgetPictures.cpp). */
enum class EHudSlotPicture : uint8
{
	Disc,
	Rim,
	Metal,
	Arc,
	Hairline,
	Dashed,
	AccentRing,
	TabPlate,
	TabEdge,
	/** A shut slot's padlock (gunmetal body, salt shackle, ink keyhole), in its own colours. */
	Lock,
};

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
 *  - a shut slot (the player's slots open with their progress: GunSlotRules): a dimmed gunmetal ring round a dark disc with
 *    a small padlock in it, no dashes, its tab and number faint; the moment it opens a soul-gold ring flashes out from it
 * No backing panels: the inner discs and the tabs' glass fade with the UI transparency setting; the rings, icons and
 * numbers stay. The shapes are vector pictures drawn once into shared textures (HudWeaponSlotsWidgetPictures.cpp); per
 * frame it only checks whether anything changed and eases the slot in hand.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudWeaponSlotsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows Manager's slots; call every frame (it only repaints when something changed). */
	void Update(const UWeaponManagerComponent* Manager, float DeltaTime);

	/** The most slots it shows (GunSlotRules::NumSlots: every slot shows, a shut one dim with its lock). */
	static constexpr int32 MaxSlots = 4;
	/** The column's height for Slots rows: their circles and the gaps between (the cluster stands on it; Looter.UI.GunSlots). */
	static constexpr float ColumnHeight(int32 Slots) { return Slots > 0 ? Slots * SlotSpacing - (SlotSpacing - SlotDiameter) : 0.f; }
	/** A shut slot's padlock, and how long the ring flashes out as one opens. */
	static constexpr float LockWidth = 22.f;
	static constexpr float LockHeight = 26.f;
	static constexpr float UnlockSeconds = 0.9f;

	/** What slot Index shows now, for the tests: shown at all, and what keeps it shut (Open: a gun or an empty open slot). */
	bool IsSlotShown(int32 Index) const;
	EGunSlotLock GetShownLock(int32 Index) const;
	/** A slot's opening flash is playing. */
	bool IsUnlockFlashing(int32 Index) const;

	/** A slot picture as a brush, its texture made once and shared (HudWeaponSlotsWidgetPictures.cpp; the gun-slot banner and the loadout's shut rows draw them too). */
	static FSlateBrush PictureBrush(EHudSlotPicture Picture);

	/** A slot's circle, and how far apart the circles stand, centre to centre (the column's rows). */
	static constexpr float SlotDiameter = 64.f;
	static constexpr float SlotSpacing = 76.f;
	/** The key tab's size, and the room the accent ring's picture has round the circle on every side (the pictures are drawn to these). */
	static constexpr float TabWidth = 22.f;
	static constexpr float TabHeight = 16.f;
	static constexpr float AccentMargin = 4.f;

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
		/** A shut slot's padlock, and the soul-gold ring that flashes out as it opens. */
		UImage* Lock = nullptr;
		UImage* UnlockRing = nullptr;
		/** It was shut on the last update (an opening flashes), what keeps it shut now, and seconds into its opening flash. */
		bool bWasLocked = false;
		EGunSlotLock ShownLock{};
		float UnlockAge = 100.f;
		/** 0 resting, 1 in hand (moved left and grown), eased. */
		float Lift = 0.f;
		/** What it shows, to repaint only on change (nothing yet: the first update always paints). */
		uint32 Shown = MAX_uint32;
	};

	void Paint(FSlotWidgets& Cell, int32 Index, const UWeaponManagerComponent* Manager, bool bInHand, EGunSlotLock Lock);
	/** The opening flash's ring, DeltaTime on. */
	void TickUnlockFlash(FSlotWidgets& Cell, float DeltaTime);

	TArray<FSlotWidgets> Slots;
};
