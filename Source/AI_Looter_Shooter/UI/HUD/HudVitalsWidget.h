#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HudVitalsWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;

/**
 * The HUD's health readout (bottom-left): a ring with the health number inside and a small "HP" under it, and a solid bar
 * running right out of the ring's lower side, so ring and bar read as one outlined shape. The bar's near end follows the
 * ring, its far end leans like the HUD's other bars.
 *  - damage: the bar drops at once, the lost part lingers as a light chip and then drains, and the ring flashes red
 *  - low health (30% or less): the ring, the fill and the number beat between red and a lighter red, the disc throbs red
 *  - full health and nothing happening: the whole readout steps back to the HUD's idle opacity
 * No backing panel: the outline, fill and number stay solid, the disc and the bar's empty part fade with the UI
 * transparency setting. The shapes are vector icons drawn once into shared textures; per frame it only repaints what
 * changed (the bar is cropped, never squeezed, so its slanted ends keep their shape).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudVitalsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The ring's size on screen, and how far the bar reaches past it. */
	static constexpr float RingDiameter = 76.f;
	static constexpr float BarLength = 320.f;

	/** The whole readout's box: the ring on the left, the bar's far end (plus its outline) on the right. */
	static constexpr float Width = RingDiameter + BarLength + 2.f;
	static constexpr float Height = RingDiameter;

	/** At or under this share of the maximum, health reads as low. */
	static constexpr float LowFraction = 0.3f;

	/** Shows the player's health; call every frame (it only repaints what changed). */
	void SetHealth(float Health, float MaxHealth, float DeltaTime);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** Shows Image's bar from the near end up to Fraction of its length; ShownLength remembers the drawn length. */
	void PaintBar(UImage* Image, float Fraction, float& ShownLength) const;

	/** Sets the ring, fill, disc and number colors for the current health and beat. */
	void PaintColors(bool bLow, float Pulse);

	UPROPERTY(Transient) TObjectPtr<UWidget> Cluster;
	UPROPERTY(Transient) TObjectPtr<UImage> DiscImage;
	UPROPERTY(Transient) TObjectPtr<UImage> ChipImage;
	UPROPERTY(Transient) TObjectPtr<UImage> FillImage;
	UPROPERTY(Transient) TObjectPtr<UImage> OutlineImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HealthText;

	/** The whole bar inside, which each level crops from (its texture is shared and kept for the session). */
	FSlateBrush BarBrush;

	/** What the bar shows (snaps down on damage), and the trailing chip that drains after a hit. */
	float ShownFraction = -1.f;
	float ChipFraction = -1.f;
	float ChipHoldTime = 0.f;
	/** Drawn lengths of the fill and the chip, to repaint only when they move. */
	float ShownFillLength = -1.f;
	float ShownChipLength = -1.f;
	int32 ShownPoints = INDEX_NONE;

	/** Seconds left of the full-strength hold after something happened, and of the ring's red flash after a hit. */
	float Activity = 0.f;
	float HitFlash = 0.f;
	float PulseTime = 0.f;
	/** The colors show the resting state (nothing beating or flashing), so they needn't be set again. */
	bool bColorsAtRest = false;
};
