#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudFlaskWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;
class UPlayerFlaskComponent;
enum class EFlaskChange : uint8;

namespace LooterUI { struct FInkedIcon; }

/** How the flask row flashes when something happens to the flask (FlashFor maps the flask's changes to these). */
enum class EHudFlaskFlash : uint8
{
	/** A drink: the pip it came from pours out, the row dips a little. */
	Drunk,
	/** Soul filled a charge: that pip pops in white and settles into the soul-light, the icon's glow flares. */
	Filled,
	/** A respawn's top-up: every pip flashes softly, no pop (the flask's own sound is quiet too). */
	Refilled,
	/** The key pressed with no charge: the pips go red and the row shakes. */
	DeniedEmpty,
	/** The key pressed at full health: "Health full" by the pips, once per press. */
	DeniedFull
};

/**
 * The soul flask on the HUD (Docs/Work/2026-10-30-SoulFlask), under the player frame's experience bar, bottom-left (healing
 * belongs with health): the Flask key's tab (the grenade counter's tab), the flask's Inked icon (InkedIconData::SoulFlask)
 * and three pips, one per charge, slim slanted bars leaning with the HUD's others. A lit pip is a full charge, in the
 * soul-light's green (LooterUI::Color::Heal); the next one fills from its foot as a liquid level as soul comes in
 * (UPlayerFlaskComponent::GetSoulShare). Floating, no backing panel: only the key tab's glass and the pips' empty tracks
 * fade with the UI transparency setting.
 *  - drinking: the icon tips toward the drinker and back over the drink, its glow swells while the heal flows
 *  - flashes (Flash): a drink pours its pip out, a filled charge pops its pip in white, a respawn's top-up flashes them
 *    all softly, a press with none left turns the pips red and shakes the row, a press at full health says "Health full"
 *  - none left: the icon and the key go dim
 *  - nothing happening with a full flask: the row steps back to the HUD's idle opacity, as the player frame does
 * PlayerHUDWidget places it and calls Update every frame; it reads the flask itself.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudFlaskWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Where it sits, in 1080p pixels from the screen's top-left (PlayerHUDWidget places it by these) ---

	/**
	 * The row's left end: level with the experience bar's start (UHudPlayerFrameWidget::XPBarX), right of the level gem's
	 * right tip (x 180) and clear of the medallion's lower-right edge.
	 */
	static constexpr float Left = 185.f;
	/**
	 * The row's middle: its icon's box (IconSize) starts just under the experience bar's pictures (they end at y 1016) and
	 * ends 17 px above the screen's bottom; the flask drawn in it leaves about 6 px of the box clear above and below.
	 */
	static constexpr float Middle = 1040.f;
	static constexpr float ScreenBottom = 1080.f;

	// --- Its pieces ---

	/**
	 * Read at a glance beside the player frame (the play-check of 2026-10-30: at the grenade counter's sizes the flask was
	 * a letter's height): the key tab and its key's type size; the icon's box (the flask drawn in it about 34 px tall, the
	 * level gem's height); and the pips, each this big, this far apart, leaning.
	 */
	static constexpr float TabWidth = 26.f;
	static constexpr float TabHeight = 20.f;
	static constexpr int32 KeySize = 14;
	static constexpr float IconSize = 46.f;
	static constexpr float PipWidth = 16.f;
	static constexpr float PipHeight = 28.f;
	static constexpr float PipGap = 6.f;
	static constexpr float PipSlant = 16.f;
	static constexpr int32 MaxPips = 3;
	/** A pip's level this close to its target is there. */
	static constexpr float LevelSnap = 0.002f;
	/** "Health full"'s type size. */
	static constexpr int32 WordsSize = 15;

	/** How long each flash runs, and how long "Health full" stays. */
	static constexpr float DrunkSeconds = 0.35f;
	static constexpr float FilledSeconds = 0.6f;
	static constexpr float RefilledSeconds = 0.8f;
	static constexpr float DeniedSeconds = 0.4f;
	static constexpr float WordsSeconds = 1.4f;

	/** The icon tips this far (degrees, toward the left: the drinker's mouth) at the drink's middle. */
	static constexpr float TipDegrees = 24.f;

	/**
	 * Reads the flask (null: the row hides) and the bound key; call every frame. It only repaints what changed, and runs
	 * the flashes and the pips' pours.
	 */
	void Update(const UPlayerFlaskComponent* Flask, const FString& KeyName, float DeltaTime);

	/** Update's work with the flask's state given (tests; Update reads it off the component). */
	void UpdateState(bool bHasFlask, int32 Charges, float SoulShare, float DrinkAlpha, bool bHealing, const FString& KeyName,
		float DeltaTime);

	/**
	 * Something happened to the flask: flash it. Charges: the flask's count after the change (OnFlaskChanged gives it),
	 * so a drink's or a filled charge's pip is the right one; INDEX_NONE guesses from the count shown.
	 */
	void Flash(EHudFlaskFlash Kind, int32 Charges = INDEX_NONE);

	/** The next state shows as it is, with no pour or fill (a load, a new pawn). */
	void SnapNext() { bSnapNext = true; }

	// --- The plain rules (Looter.UI.Flask.Rules) ---

	/** How full pip Index should be: 1 for a full charge, the soul toward the next for the next one, 0 past it. */
	static float PipTarget(int32 Index, int32 Charges, float SoulShare);

	/**
	 * The flash a flask change earns, if any: a drink, a filled charge, a respawn's top-up and both refusals flash; soul
	 * pouring in (the pip's own rise shows it) and a load (shown as it is) don't.
	 */
	static bool FlashFor(EFlaskChange Why, EHudFlaskFlash& OutFlash);

	/** The icon's tip (degrees) at DrinkAlpha through the drink: up and back, 0 at both ends. */
	static float TipAngle(float DrinkAlpha);

	/** A pip's level Shown moved toward Target over DeltaTime: a pour drains in DrunkSeconds, soul rises quickly but visibly. */
	static float StepLevel(float Shown, float Target, float DeltaTime);

	/** The shake's sideways offset (px) Elapsed seconds into a refusal, dying away by DeniedSeconds. */
	static float ShakeOffset(float Elapsed);

	/** The flask's Inked icon. */
	static const LooterUI::FInkedIcon& Icon();

	// --- What it shows (tests) ---

	bool IsShowing() const { return bShown; }
	float GetShownLevel(int32 Pip) const { return ShownLevels.IsValidIndex(Pip) ? ShownLevels[Pip] : 0.f; }
	bool IsSayingHealthFull() const { return WordsLeft > 0.f; }
	bool IsFlashing() const { return FlashLeft > 0.f; }
	EHudFlaskFlash GetFlashKind() const { return FlashKind; }
	bool IsDim() const { return ShownCharges == 0; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** The pips' fills, tracks and flashes for the levels and the flash running. */
	void PaintPips();
	/** The icon's and key's dimness, the icon's tip and its glow. */
	void PaintIcon(float DrinkAlpha, bool bHealing, float DeltaTime);
	/** The flash's movement of the row (pop, shake) and "Health full"'s fade. */
	void RunFlash(float DeltaTime);

	UPROPERTY(Transient) TObjectPtr<UWidget> Row;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyText;
	UPROPERTY(Transient) TObjectPtr<UImage> IconImage;
	UPROPERTY(Transient) TObjectPtr<UImage> IconGlow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WordsText;
	/**
	 * Per pip: its leaning box (it pops), its empty track, its fill (scaled up from its foot), the soul's bright surface
	 * riding on the fill, and its white flash.
	 */
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> PipBoxes;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> PipTracks;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> PipFills;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> PipSurfaces;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> PipFlashes;

	TArray<float> ShownLevels;
	int32 ShownCharges = INDEX_NONE;
	FString ShownKey;
	bool bShown = false;
	bool bSnapNext = true;

	/** The flash running: its kind, seconds left, and the pip it's about (a drink's, a filled charge's). */
	EHudFlaskFlash FlashKind = EHudFlaskFlash::Filled;
	float FlashLeft = 0.f;
	int32 FlashPip = INDEX_NONE;
	/** Seconds left of "Health full". */
	float WordsLeft = 0.f;
	/** The icon's glow as shown (eased: up while the heal flows or a charge fills, down after). */
	float GlowShown = 0.f;
	/** Seconds the row stays at full strength after something happened; its opacity as shown. */
	float Activity = 0.f;
	float OpacityShown = 1.f;

	/** What was last set on the widgets, so a frame sets only what changed (-1: never set). */
	TArray<float> PaintedLevels;
	TArray<float> PaintedWhite;
	TArray<float> PaintedPop;
	float PaintedRed = -1.f;
	float PaintedAngle = -1000.f;
	float PaintedGlow = -1.f;
	bool bPaintedEmpty = false;
	/** The row was popped or shaken last frame (it's set back to rest once). */
	bool bRowMoved = false;
};
