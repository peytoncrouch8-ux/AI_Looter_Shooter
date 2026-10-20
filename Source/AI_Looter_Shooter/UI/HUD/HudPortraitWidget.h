#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudPortraitWidget.generated.h"

class UCanvasPanel;
class UImage;
class UWidget;

/**
 * The player's portrait in the player frame's diamond window (UHudPlayerFrameWidget draws the bezel, edges and clamps
 * round and over it): the played hero's inked bust (Art/Icons/HudPortrait[_<Hero>].svg; Ellis's, a masked gunslinger
 * with cyan eyes under the hat brim, is the user's pick) on dark glass with faint scanlines. It draws what's inside the
 * window, clipped to the diamond, and reacts:
 *  - calm, by itself: it blinks every 5.2 s and breathes (the bust bobs 1.4 px over 4.2 s)
 *  - a hit: the content shakes, the window flashes red and the eyes squint
 *  - low health: the squint holds and the window pulses red
 *  - a level-up: the eyes flare, then fade
 * The art is vector data (UI/Style/HudPortraitData[_<Hero>].inl, generated from the SVGs by Art/Icons/HudPortrait.py)
 * drawn into shared textures once. Only the glass is a background (it fades with the UI transparency setting); the
 * bust, eyes and glows stay solid. Each frame it moves or fades only what changed.
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

	/**
	 * Whose portrait the window shows (UHeroDefinition::Id: "Ellis", "Odessa", "Crane", "Gauge", "Pike"; any other id
	 * shows Ellis's). Unset, the window asks for the hero played as it builds; a change once built redraws the window.
	 */
	void SetHero(FName InHeroId);

	/**
	 * What a hero's portrait is made of, for tests: the layers of its glass and bust and the glows of its eye groups
	 * (open, squint, flare). An id with no portrait of its own counts as Ellis's. Zero anywhere is a broken set.
	 */
	struct FPortraitCounts
	{
		int32 GlassLayers = 0;
		int32 BustLayers = 0;
		int32 EyesOpenGlows = 0;
		int32 EyesHurtGlows = 0;
		int32 EyesFlareGlows = 0;
		/** Starts the pictures' texture names (Ellis's "HudPortrait", Crane's "HudPortraitCrane"). */
		FString TexturePrefix;
	};
	static FPortraitCounts CountPortrait(FName HeroId);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Fills Content for HeroId: glass, bust, the eye groups and the red flash (again on a hero change). */
	void BuildPortrait();

	/** The upright content inside the diamond clip; BuildPortrait fills it. */
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Content;
	/** The hero shown (None until set or found: Ellis's portrait). */
	FName HeroId;

	/** Everything inside the window: it shakes on a hit (the window itself stays put). */
	UPROPERTY(Transient) TObjectPtr<UWidget> Face;
	/** The bust and the eyes: they breathe. */
	UPROPERTY(Transient) TObjectPtr<UWidget> Body;
	/** The open eyes with their brows, the squint, and the flare over them: each fades on its own. */
	UPROPERTY(Transient) TObjectPtr<UWidget> CalmEyes;
	UPROPERTY(Transient) TObjectPtr<UWidget> HurtEyes;
	UPROPERTY(Transient) TObjectPtr<UWidget> FlareEyes;
	/** The squint's glows alone: Gauge's lamp flickers while hurt (bHurtFlickers), so they fade apart from the picture. */
	UPROPERTY(Transient) TObjectPtr<UWidget> HurtGlows;
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
	/** The shown hero's squint flickers its glows (a failing lamp: Gauge's); set by BuildPortrait from the portrait set. */
	bool bHurtFlickers = false;

	/** What's on screen now, so each frame sets only what changed. */
	FVector2D ShownShake = FVector2D::ZeroVector;
	float ShownBreath = 0.f;
	float ShownBlink = 1.f;
	float ShownCalm = 1.f;
	float ShownHurt = 0.f;
	float ShownFlare = 0.f;
	float ShownHurtGlow = 1.f;
	float ShownFlash = 0.f;
};
