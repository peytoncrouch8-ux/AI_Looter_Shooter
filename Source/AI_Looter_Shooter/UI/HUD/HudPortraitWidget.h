#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudPortraitWidget.generated.h"

class UImage;
class UWidget;

/**
 * The player's portrait in the player frame's diamond window (UHudPlayerFrameWidget draws the bezel, edges and clamps
 * round and over it): a masked gunslinger in the Inked style on dark glass with faint scanlines, eyes glowing cyan
 * under the hat brim (Art/Icons/HudPortrait.svg, the user's pick). It draws what's inside the window, clipped to the
 * diamond, and reacts:
 *  - calm, by itself: it blinks every 5.2 s and breathes (the bust bobs 1.4 px over 4.2 s)
 *  - a hit: the content shakes, the window flashes red and the eyes squint
 *  - low health: the squint holds and the window pulses red
 *  - a level-up: the eyes flare, then fade
 * The art is vector data (UI/Style/HudPortraitData.inl, generated from the SVG by Art/Icons/HudPortrait.py) drawn into
 * shared textures once. Only the glass is a background (it fades with the UI transparency setting); the bust, eyes and
 * glows stay solid. Each frame it moves or fades only what changed.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudPortraitWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The diamond window it fills, tip to tip; its box is this square, the diamond inscribed in it. */
	static constexpr float WindowSize = 128.f;

	/** A hit: the shake, the window's red flash, the squint (UHudPlayerFrameWidget calls it). */
	void PlayHit();

	/** Low health: the squint holds and the window pulses red on the frame's 0.9 s beat (LowBeat) until it's set false. */
	void SetLowHealth(bool bLow);

	/** A level-up: the eyes flare, then fade over 1.8 s. */
	void PlayLevelUp();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Everything inside the window: it shakes on a hit (the window itself stays put). */
	UPROPERTY(Transient) TObjectPtr<UWidget> Face;
	/** The bust and the eyes: they breathe. */
	UPROPERTY(Transient) TObjectPtr<UWidget> Body;
	/** The open eyes with their brows, the squint, and the flare over them: each fades on its own. */
	UPROPERTY(Transient) TObjectPtr<UWidget> CalmEyes;
	UPROPERTY(Transient) TObjectPtr<UWidget> HurtEyes;
	UPROPERTY(Transient) TObjectPtr<UWidget> FlareEyes;
	/** The open eyes and their glows, without the brows: they squash for the blink. */
	UPROPERTY(Transient) TObjectPtr<UWidget> BlinkingEyes;
	/** The window's red, over the content (a hit's flash and the low-health pulse). */
	UPROPERTY(Transient) TObjectPtr<UImage> Flash;

	/** Seconds into each reaction, or below zero when it isn't playing. */
	float ShakeTime = -1.f;
	float FlashTime = -1.f;
	float SquintTime = -1.f;
	float FlareTime = -1.f;
	/** The calm loops. The low-health beat has no clock of its own: it follows UHudPlayerFrameWidget::LowBeat. */
	float BlinkClock = 0.f;
	float BreathClock = 0.f;
	bool bLowHealth = false;

	/** What's on screen now, so each frame sets only what changed. */
	FVector2D ShownShake = FVector2D::ZeroVector;
	float ShownBreath = 0.f;
	float ShownBlink = 1.f;
	float ShownCalm = 1.f;
	float ShownHurt = 0.f;
	float ShownFlare = 0.f;
	float ShownFlash = 0.f;
};
