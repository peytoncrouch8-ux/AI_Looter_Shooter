#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HudMagazineWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * The HUD's magazine gauge (bottom-right, beside the weapon slots): a cartridge standing upright, its base (rim and
 * groove) at the bottom and the bullet's pointed tip at the top, so the box itself reads as ammo. Its inside is filled
 * with the rounds left and drains from the tip down as the gun fires, with faint marks across it at a half and three
 * quarters. The fill has two tones (light on the left), a fine hatch and a light leading edge; the outline is doubled,
 * dark under light. Inside, by the base, the counts stack, centred: the rounds in the magazine over the reserve ("/120").
 *  - low (a quarter or less): the fill and the count turn orange
 *  - empty: the count turns red and the outline beats between orange and red
 *  - reloading: the fill is the reload's progress, in orange, rising from the base; the count still shows the magazine
 *  - no reserve: the reserve turns red
 * No backing panel: the outline, fill and counts stay solid, the empty inside fades with the UI transparency setting.
 * The shapes are vector pictures (drawn in the mockup's lying-down frame and turned a quarter) made once into shared
 * textures; per frame it eases the level and repaints only what changed.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudMagazineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The cartridge's size on screen, standing; the bullet's nose is about the top 50 px of it. */
	static constexpr float Width = 51.f;
	static constexpr float Height = 196.f;

	/** At or under this share of the magazine left, the gauge reads as low. */
	static constexpr float LowFraction = 0.25f;

	/**
	 * Shows the magazine and the reserve; call every frame (it only repaints what changed).
	 * @param Reserve         The rounds left to reload from, shown under the magazine's count.
	 * @param ReloadProgress  0-1 through the reload, shown by the fill instead of the rounds while bReloading.
	 * @param bSnap           Jump to the new level instead of easing to it (another gun just came into hand).
	 * @param Pulse           0-1, the HUD's alarm beat, so an empty gun's outline beats with the reload prompt.
	 */
	void SetMagazine(int32 Rounds, int32 Capacity, int32 Reserve, bool bReloading, float ReloadProgress, bool bSnap, float Pulse,
		float DeltaTime);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** Shows the fill (and its leading edge) from the base up to Fraction of the fill's run. */
	void PaintFill(float Fraction);

	UPROPERTY(Transient) TObjectPtr<UImage> FillImage;
	UPROPERTY(Transient) TObjectPtr<UImage> EdgeImage;
	UPROPERTY(Transient) TObjectPtr<UImage> OutlineImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ReserveText;

	/** The whole fill in each colour, which each level crops from (their textures are shared and kept for the session). */
	FSlateBrush CyanFillBrush;
	FSlateBrush OrangeFillBrush;
	/** The inside's silhouette, white: the leading edge is a thin band cropped from it at the level. */
	FSlateBrush EdgeBrush;
	/** Whether the fill shows orange now (low or reloading), so a colour change crops the other picture. */
	bool bOrangeFill = false;

	/** The level the fill shows (0-1, eased toward the target) and its drawn length, to repaint only when that moves. */
	float ShownFraction = -1.f;
	float ShownLength = -1.f;
	int32 ShownRounds = INDEX_NONE;
	int32 ShownReserve = INDEX_NONE;
	/** The low / empty / reloading state the colours were last set for. */
	int32 ShownState = INDEX_NONE;
	bool bWasReloading = false;
};
