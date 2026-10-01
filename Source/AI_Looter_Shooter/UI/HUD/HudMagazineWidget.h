#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HudMagazineWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * The HUD's magazine gauge (bottom-right): a cartridge lying on its side, its base (rim and groove) on the left and the
 * bullet's pointed nose on the right, so the box itself reads as ammo. Its inside is filled with the rounds left and
 * drains from the nose toward the base as the gun fires (the nose holds about the first quarter fired), with faint marks
 * at a quarter and a half; the count sits inside, by the base.
 *  - low (a quarter or less): the fill and the count turn orange
 *  - empty: the count turns red and the outline beats between orange and red
 *  - reloading: the fill is the reload's progress, in orange; the count still shows the magazine
 * No backing panel: the outline, fill and count stay solid, the empty inside fades with the UI transparency setting. The
 * shapes are vector icons drawn once into shared textures; per frame it eases the level and repaints only what changed.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudMagazineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The cartridge's size on screen; the bullet's nose is about the last 50 px of it. */
	static constexpr float Width = 230.f;
	static constexpr float Height = 60.f;

	/** At or under this share of the magazine left, the gauge reads as low. */
	static constexpr float LowFraction = 0.25f;

	/**
	 * Shows the magazine; call every frame (it only repaints what changed).
	 * @param ReloadProgress  0-1 through the reload, shown by the fill instead of the rounds while bReloading.
	 * @param bSnap           Jump to the new level instead of easing to it (another gun just came into hand).
	 * @param Pulse           0-1, the HUD's alarm beat, so an empty gun's outline beats with the reload prompt.
	 */
	void SetMagazine(int32 Rounds, int32 Capacity, bool bReloading, float ReloadProgress, bool bSnap, float Pulse, float DeltaTime);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** Shows the fill from the base up to Fraction of the inside's length. */
	void PaintFill(float Fraction);

	UPROPERTY(Transient) TObjectPtr<UImage> FillImage;
	UPROPERTY(Transient) TObjectPtr<UImage> OutlineImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountText;

	/** The whole fill, which each level crops from (its texture is shared and kept for the session). */
	FSlateBrush FillBrush;

	/** The level the fill shows (0-1, eased toward the target) and its drawn length, to repaint only when that moves. */
	float ShownFraction = -1.f;
	float ShownLength = -1.f;
	int32 ShownRounds = INDEX_NONE;
	/** The low / empty / reloading state the colors were last set for. */
	int32 ShownState = INDEX_NONE;
	bool bWasReloading = false;
};
