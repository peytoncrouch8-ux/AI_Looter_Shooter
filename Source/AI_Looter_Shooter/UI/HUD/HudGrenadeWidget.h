#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Combat/GrenadeKinds.h"
#include "HudGrenadeWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;

namespace LooterUI { struct FInkedIcon; }

/** How the grenade counter flashes when the count changes. */
enum class EHudGrenadeFlash : uint8
{
	/** One found or given: a bright white flash and a pop. */
	Gained,
	/** One thrown: a quick orange tick down. */
	Thrown,
	/** The key pressed with none left: red, and a shake. */
	Denied,
	/** A new kind of jar carried: its word pops in its colour and the pips flash it. */
	Swapped
};

/**
 * The grave-salt grenades carried, in the weapon column above the slots (bottom-right; PlayerHUDWidget places it): the
 * Grenade key's tab, the tin's Inked icon and the count, with a pip per grenade the player can carry under the count (lit
 * for each one carried). Floating outlined type and metal-edged shapes, no backing panel: only the key tab's glass fades
 * with the UI transparency setting. It shows once the player has been given grenades.
 *  - a change flashes it (Flash): a pop and a white flash for one gained, an orange tick for one thrown, red and a shake
 *    for a press with none left
 *  - none left: the count and the icon go dim, the count red
 * The tin's icon is made by Art/Icons/InkedIcons.py (InkedIconData::GraveSaltGrenade), in the ammo icons' box.
 *
 * The kind carried (round 4, GrenadeKinds): a pitch, scatter or cinder jar's word ("PITCH") in floating outlined type
 * in its colour, left of the key tab, and the lit pips in its colour; plain salt shows as it always has (no word, the
 * pips pale blue). A new kind flashes it (Swapped). It reads the kind off the owning player's pawn itself, so the HUD
 * that calls Update needn't know of kinds.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudGrenadeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Shows Count of MaxCount (only once bUnlocked) with KeyName on the tab; call every frame (it only repaints what
	 * changed, and runs the flash).
	 */
	void Update(int32 Count, int32 MaxCount, bool bUnlocked, const FString& KeyName, float DeltaTime);

	/** The same with the kind carried given (Update reads it off the owning pawn's throw component). */
	void UpdateKind(int32 Count, int32 MaxCount, bool bUnlocked, const FString& KeyName, EGrenadeKind Kind, float DeltaTime);

	/** The kind shown now. */
	EGrenadeKind GetShownKind() const { return ShownKind; }

	/** The kind's word's type size. */
	static constexpr int32 KindSize = 12;

	/** The count changed (or the key was pressed with none left): flash it. */
	void Flash(EHudGrenadeFlash Kind);

	/** The tin's Inked icon (InkedIconData::GraveSaltGrenade), in the ammo icons' box. */
	static const LooterUI::FInkedIcon& Icon();

	/** The icon's size on screen, the count's type size, the key tab's size (the slots' own) and the most pips shown. */
	static constexpr float IconSize = 30.f;
	static constexpr int32 CountSize = 20;
	static constexpr float TabWidth = 22.f;
	static constexpr float TabHeight = 16.f;
	static constexpr int32 MaxPips = 5;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** Colours for the count as it stands, before any flash. */
	void PaintState();
	/** The pips lit for the count, in the kind's colour. */
	void PaintPips();
	/** The kind's word and colour (none for salt). */
	void PaintKind();

	UPROPERTY(Transient) TObjectPtr<UWidget> Row;
	UPROPERTY(Transient) TObjectPtr<UImage> IconImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KindText;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> Pips;

	int32 ShownCount = INDEX_NONE;
	int32 ShownMax = INDEX_NONE;
	FString ShownKey;
	EGrenadeKind ShownKind = EGrenadeKind::Salt;
	/** A kind has been shown since the row came up (the first one doesn't flash: it was there). */
	bool bKindKnown = false;
	bool bShown = false;

	/** The flash running: its kind and seconds left. */
	EHudGrenadeFlash FlashKind = EHudGrenadeFlash::Gained;
	float FlashLeft = 0.f;
};
