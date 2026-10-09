#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudBossBarWidget.generated.h"

class UCanvasPanel;
class UImage;
class USizeBox;
class UTextBlock;
class UWidget;

/**
 * A boss's health at the top of the screen, in the gameplay HUD's style (no backing panel), built like the player's
 * health bar: its name in floating outlined text over a slanted bar, its level in a gem of its rank's color at the bar's
 * left, and the current phase's name under it.
 *  - the bar: an ink rim, a gunmetal bezel lit along its top, a dark track, the red fill in three bands under a 45-degree
 *    hatch with a light leading edge, and an orange clamp over the far end; it leans like every HUD bar, and the fill is
 *    cut to length, never squeezed (HudBossBarWidgetLayout.cpp builds it)
 *  - a cut crosses the bar where each later phase starts: dark on the red, it lights up once that phase is reached
 *  - a hit drops it at once, and the lost part lingers as a pale chip, then drains (as the player's bar does)
 *  - while the boss can't be hurt the fill greys, the name dims, and the line under it says why (its spell's hint)
 *  - a new phase's name flashes orange and pops, then settles; the fill and the passed cut flash orange with it
 *  - the fight's start (PlayIntro): the bar drops in, its fill runs up from empty, and the boss's title is spelled out in
 *    cyan on the line under it before the first phase's name pops up
 *  - every hit flashes the fill toward white a moment; a big one (2% and more) jolts the bar sideways
 *  - a callout takes the line a moment in orange ("STAGGERED"); the boss's death leaves it there ("DEFEATED")
 * UBossComponent adds it to the viewport for its fight and drives it; it fades in and out, and steps aside while a menu
 * is open. Only the track is a background (it fades with the UI transparency setting); the metal, fill and text stay.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudBossBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The bar's length on screen, whatever the boss's health. */
	static constexpr float BarWidth = 640.f;

	/** Over the gameplay HUD (0), under the inventory (20) and the pause menu (40). */
	static constexpr int32 ViewportZOrder = 3;

	/**
	 * Shows a boss: its name (in NameColor, its rank's) and level (in a gem of that color), and a cut at each phase's share
	 * of health after the first (PhaseShares as UBossComponent's phases have them, highest first). The bar starts full.
	 */
	void SetBoss(const FText& InName, int32 InLevel, const FLinearColor& InNameColor, const TArray<float>& PhaseShares);

	/** Cheap to call every frame: the bar only moves when the share of health left changes. */
	void SetHealth(float Health, float MaxHealth);

	/** Cheap to call every frame: the phase's name under the bar, flashing when a later phase starts. */
	void SetPhase(int32 Index, const FText& InPhaseName);

	/** Cheap to call every frame: greys the bar while the boss can't be hurt, with what to do about it (may be empty). */
	void SetUntargetable(bool bInUntargetable, const FText& InHint);

	/** Fades it in, or out (it stays in the viewport, collapsed). */
	void SetShown(bool bInShown);
	bool IsShown() const { return bWanted; }

	/** The boss's title, spelled out under the bar as it sweeps in ("Brood of the Sink"); empty: none. */
	void SetTitle(const FText& InTitle);

	/** The fight starts: the bar drops in, its fill runs up from empty, the title is spelled out, then the phase's name. */
	void PlayIntro();

	/** Takes the line under the bar for Seconds in orange, a little bigger ("STAGGERED"). */
	void ShowCallout(const FText& InCallout, float Seconds);

	/** The boss died: the bar flashes and the line says so for good (InLine, or "DEFEATED" when empty). */
	void PlayDefeated(const FText& InLine);

	// What it shows, for the tests (the text blocks' own text once it's built).
	FText GetNameText() const;
	/** The level as words ("LV 9"); the gem shows its number. */
	FText GetLevelText() const;
	FText GetPhaseText() const;
	const TArray<float>& GetTickShares() const { return TickShares; }
	int32 GetTickCount() const { return TickImages.Num(); }
	bool IsGreyed() const { return bGreyed; }
	float GetFraction() const { return Fraction; }
	float GetChipFraction() const { return ChipFraction; }
	/** The intro is playing (the fill running up, the title on the line). */
	bool IsIntroPlaying() const { return IntroTime >= 0.f; }
	/** How far the intro's fill has run up (1 outside the intro). */
	float GetIntroSweep() const;
	/** A hit's flash on the fill (0 none, 1 full). */
	float GetHitFlash() const { return HitFlash; }
	bool IsDefeated() const { return bDefeated; }

	/** How long the intro lasts: the drop, the fill's run, the title on the line (s). */
	static constexpr float IntroSeconds = 3.2f;

	/** The title as the intro has spelled it out Time seconds in (capitals, a letter at a time), empty before it starts. */
	static FText MakeIntroTitle(const FText& InTitle, float Time);

	/** Where the cuts go along the bar (0-1 from its empty end): every phase's share but the first, inside the bar, highest first. */
	static TArray<float> MakeTickShares(const TArray<float>& PhaseShares);

	/** "LV 9". */
	static FText MakeLevelText(int32 Level);

	/** The line under the bar: the hint while untargetable (when there is one), otherwise the phase's name; in capitals. */
	static FText MakePhaseLine(const FText& PhaseName, bool bUntargetable, const FText& Hint);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyTexts();
	/** Makes the phase cuts for TickShares anew (HudBossBarWidgetLayout.cpp). */
	void BuildTicks();
	/** Cuts the fill and the chip to length, and lights each cut once its phase is reached (HudBossBarWidgetLayout.cpp). */
	void ApplyFill();
	/** The fill's, gem's, name's and phase line's colors for how grey it is, and the phase line's flash. */
	void ApplyColors();
	/** The cuts' colors: dark above the health left, lit once passed, orange while a new phase flashes. */
	void ColorTicks();
	/** The line under the bar now: a callout, the boss's death, the intro's title, or the phase line. */
	FText CurrentLine() const;
	/** The title has the line (the intro, before the phase's name). */
	bool IsTitleShowing() const;
	/** Moves the intro, the flashes, the jolt and the callout on (HudBossBarWidget.cpp's tick). */
	void TickMotion(float DeltaSeconds);
	/** The inventory or the pause menu is up, or the game is paused: the bar steps aside. */
	bool IsMenuOpen() const;

	UPROPERTY(Transient) TObjectPtr<UWidget> Cluster;
	/** The gem's face, tinted the rank's color, and the level's number on it. */
	UPROPERTY(Transient) TObjectPtr<UImage> GemFace;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PhaseLabel;
	/** The health and the chip of lost health trailing it: boxes whose width cuts them to length. */
	UPROPERTY(Transient) TObjectPtr<USizeBox> FillBox;
	UPROPERTY(Transient) TObjectPtr<USizeBox> ChipBox;
	/** The fill's three bands (top, middle, bottom), its lit top line and its leading edge: they grey while untargetable. */
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> FillBands;
	UPROPERTY(Transient) TObjectPtr<UImage> FillTopLine;
	UPROPERTY(Transient) TObjectPtr<UImage> FillEdge;
	/** Over the track: the phase cuts. */
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> TickLayer;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> TickImages;

	FText BossName;
	int32 BossLevel = 1;
	FLinearColor NameColor = FLinearColor::White;
	TArray<float> TickShares;
	int32 PhaseIndex = INDEX_NONE;
	FText PhaseName;
	FText Hint;
	bool bGreyed = false;

	/** The share of health shown, and the chip's end: it holds a moment after a hit, then drains down to it. */
	float Fraction = 1.f;
	float ChipFraction = 1.f;
	float ChipHold = 0.f;
	/** How grey the bar is drawn, easing between 0 and 1. */
	float Grey = 0.f;
	/** Seconds left of the phase line's flash. */
	float PhaseFlash = 0.f;

	/** The intro's clock (negative: not playing), the title it spells out and how much of it is shown. */
	float IntroTime = -1.f;
	FText Title;
	int32 ShownTitleChars = INDEX_NONE;
	/** A callout's words and seconds left; the boss's death holds one for good. */
	FText Callout;
	float CalloutLeft = 0.f;
	float CalloutSeconds = 1.f;
	bool bDefeated = false;
	/** A hit's flash, a big hit's jolt, a new phase's flash on the fill (each 1 at its start, easing to 0). */
	float HitFlash = 0.f;
	float Jolt = 0.f;
	float BarFlash = 0.f;
	float MotionClock = 0.f;
	FVector2D ShownOffset = FVector2D::ZeroVector;

	bool bWanted = false;
	float Opacity = 0.f;
	/** Drawn widths, and how many cuts sit on the lit part, so it only repaints what moved. */
	float ShownFillWidth = -1.f;
	float ShownChipWidth = -1.f;
	int32 ShownLitTicks = INDEX_NONE;
};
