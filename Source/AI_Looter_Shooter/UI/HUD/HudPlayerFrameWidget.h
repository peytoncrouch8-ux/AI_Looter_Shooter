#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HudPlayerFrameWidget.generated.h"

class UCanvasPanel;
class UHudPortraitWidget;
class UImage;
class UTextBlock;
class UWidget;
class UWorld;
class UPlayerProgressionSubsystem;
enum class EXPSource : uint8;

DECLARE_DELEGATE_OneParam(FOnHudLevelUp, int32 /*NewLevel*/);

/** The player frame's pictures, each drawn once into a shared texture (HudPlayerFrameWidgetPictures.cpp draws and places them). */
enum class EHudFramePicture : uint8
{
	MedallionBack,    // the horn, the gunmetal bezel and its lines, behind the portrait
	MedallionFront,   // the window's ink edge, its cyan hairline and the two orange clamps, over the portrait
	Gem,              // the level gem
	GemFlash,         // the gem's face in white, flashing it on a level-up
	GemRing,          // the ring that spreads out of the gem on a level-up
	HealthBack,       // the health bar's rim and gunmetal bezel (its track cut out)
	HealthTrack,      // the health bar's dark track (a background)
	HealthFront,      // the quarter cuts and the orange "]" clamp over the far end
	HealthShine,      // the pale green shine that sweeps along the fill on a heal
	HealthFillBody,   // the fill: three red bands, the hatch, the lit top line and (at the far end) the light edge
	HealthFillCap,    // the fill's slanted leading end with its light edge
	HealthWhiteBody,  // the track's shape in white, tinted: the chip, the low-health beat
	HealthWhiteCap,
	XPEdge,           // the experience bar's ink edge
	XPTrack,          // its dark track and the faint empty sections (a background)
	XPFillBody,       // the ten sections in cyan halves
	XPFillCap,
	XPWhiteBody,      // the sections in white, tinted: the just-earned stretch
	XPWhiteCap,
	Count
};

/**
 * The frame's bars as slanted stretches, each from its bar's start up to a level: its bar's picture cropped just short of
 * the level, and an end piece that finishes it along the HUD's 16 degree lean, so the leading edge leans like the bar's
 * ends and stays smooth (only edges inside a texture are anti-aliased). Back to front within each bar.
 */
enum class EHudFrameStretch : uint8
{
	HealthChip,   // the part the last hit took, lingering
	HealthFill,
	HealthBeat,   // the low-health beat, brightening the fill
	XPGain,       // the just-earned stretch ahead of the fill, white
	XPFill,
	XPGlow,       // the just-earned stretch the fill has covered, white and fading
	XPBefore,     // the fill as it was before the gain, laid back over the glow so the glow starts where the gain did
	Count
};

/**
 * The player frame, bottom-left (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "The player frame"): the portrait in a
 * gunmetal medallion (orange clamps, a horn on its left tip), the health bar running right from behind it, the level in a
 * cyan gem on the medallion's lower-right edge and the experience bar in ten sections under the health bar. No name, no
 * backing panel: only the tracks and the portrait's glass fade with the UI transparency setting.
 *  - hit: the bar drops at once, the lost part lingers as a pale chip, then drains; the portrait flinches
 *  - low health (30% or less): the fill brightens on a 0.9 s beat, the number turns pale, the portrait squints
 *  - heal: the fill rises with a pale green shine sweeping along it, "+30" rises at the bar's end
 *  - regeneration (the wounds that close): the fill rises smoothly as health comes back, a softer shine sweeps along
 *    it every so often, and one "+N" rises when the run has filled the bar
 *  - experience: the stretch just earned shows white, holds and fades as the bar catches up; "+160 XP" rises
 *  - level-up: once the bar gets there the gem flashes, a ring spreads out of it, the portrait's eyes flare, and
 *    OnLevelUp tells the HUD to show the banner
 *  - full health and nothing happening: the whole frame steps back to the HUD's idle opacity
 * Its shapes are painted pictures drawn once into shared textures; per frame it only repaints what changed. Health comes
 * from the HUD (SetHealth), experience from UPlayerProgressionSubsystem's events.
 * Split by topic: HudPlayerFrameWidget.cpp (health), HudPlayerFrameWidgetRegen.cpp (regeneration),
 * HudPlayerFrameWidgetXP.cpp (experience and level-ups),
 * HudPlayerFrameWidgetLayout.cpp (the widget tree), HudPlayerFrameWidgetPictures.cpp (the pictures) and
 * HudPlayerFrameWidgetStretches.cpp (the bars' stretches), with the bars' measurements in HudPlayerFrameShapes.h.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudPlayerFrameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Its box: from the screen's left edge to past the XP numbers, and from above the medallion down to the screen's bottom. */
	static constexpr float Width = 700.f;
	static constexpr float Height = 210.f;

	/** At or under this share of the maximum, health reads as low; the low-health beat's length (the screen edges share both). */
	static constexpr float LowFraction = 0.3f;
	static constexpr float LowBeatSeconds = 0.9f;

	/** Shows the player's health; call every frame (it only repaints what changed). */
	void SetHealth(float Health, float MaxHealth, float DeltaTime);

	/** There is no health to show now (a pawn without any): the next SetHealth shows its health as it is, with no hit or heal. */
	void ForgetHealth() { LastHealth = -1.f; }

	/** A level-up, once the experience bar gets there: the HUD shows the banner. */
	FOnHudLevelUp OnLevelUp;

	/** The low-health beat at World's time, 0 to 1 and back every LowBeatSeconds, so everything that beats beats together. */
	static float LowBeat(const UWorld* World);

	// Where the parts sit: the spec's numbers, in pixels on a 1080p screen from its top-left (the box ends at its bottom).
	static constexpr float ScreenBottom = 1080.f;
	/** The medallion's centre and the level gem's. */
	static constexpr float MedallionX = 113.f;
	static constexpr float MedallionY = 974.f;
	static constexpr float GemX = 161.f;
	static constexpr float GemY = 999.f;
	/** The health and experience bars' boxes start here (their top-left before the lean). */
	static constexpr float HealthBarX = 130.f;
	static constexpr float HealthBarY = 940.f;
	static constexpr float XPBarX = 185.f;
	static constexpr float XPBarY = 994.f;

	/** A spot on the 1080p screen in the frame's box. */
	static FVector2D InFrame(float ScreenX, float ScreenY) { return FVector2D(ScreenX, ScreenY - (ScreenBottom - Height)); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// The frame's timing, shared by its health and experience halves.
	/** How long it stays at full strength after something happens, and its opacity when nothing is (as the HUD's corners). */
	static constexpr float ActivityHoldSeconds = 3.f;
	static constexpr float IdleOpacity = 0.6f;

	/** A heal's fill rises over this long; its shine sweeps along the fill over this long; the "+30" floats this long. */
	static constexpr float HealRiseSeconds = 0.5f;
	static constexpr float ShineSeconds = 0.75f;
	static constexpr float HealFloatSeconds = 1.2f;

	/** Floats Text (a "+30" or "+160 XP") Age into its life of Duration: in quickly, rising, fading out. */
	static void PaintFloat(UTextBlock* Text, float Age, float Duration);

	// --- Pictures (HudPlayerFrameWidgetPictures.cpp) and stretches (HudPlayerFrameWidgetStretches.cpp) ---

	/** A picture as a brush its own size, and where its top-left sits in the frame's box. */
	static FSlateBrush PictureBrush(EHudFramePicture Picture);
	static FVector2D PicturePosition(EHudFramePicture Picture);
	/** Adds a stretch's body and end piece to Canvas, tinted Tint (white shows the pictures' own colors). */
	void AddStretch(UCanvasPanel* Canvas, EHudFrameStretch Stretch, const FLinearColor& Tint);
	/** Shows a stretch from its bar's start up to Amount (0-1 of the bar); repaints only when its end moves a pixel. */
	void PaintStretch(EHudFrameStretch Stretch, float Amount);
	void TintStretch(EHudFrameStretch Stretch, const FLinearColor& Tint);
	/** The heal's shine Progress (0-1) of the way along a fill of Amount, cut off at the fill's ends; hidden at 1. */
	void PaintShine(float Progress, float Amount);

	// --- Health (HudPlayerFrameWidget.cpp) ---

	/** Shows Fraction at once, with nothing lingering or rising (first sight, a respawn). */
	void SnapHealth(float Fraction);
	void UpdateHealthNumbers(float Health, float MaxHealth);
	/** The low-health look: the number's color and the portrait's squint, set when it changes. */
	void ShowLow(bool bLow);

	// --- Regeneration, the wounds that close (HudPlayerFrameWidgetRegen.cpp) ---

	/** The player's wounds are closing now (UPlayerVitalsSubsystem knows: past the wait, below full health). */
	bool IsRegenerating() const;
	/**
	 * Shows Gained points healed this frame as part of a regeneration: the fill rises with it frame by frame, a gentle
	 * shine sweeps along now and then, and the gains add up for the "+N" at the run's end. False when the gain is too big
	 * to be regeneration (a soul-mote's heal on top): the heal's own show takes it.
	 */
	bool ShowRegenGain(float Gained, float Fraction, float DeltaTime);
	/** Regeneration stopped (full health, a hit, a scene): once it filled the bar from a real wound, "+N" rises at the bar's end. */
	void EndRegenRun(float Fraction, float MaxHealth);
	/** Raises "+Amount" at the health bar's end (a regeneration run's total). */
	void RaiseHealFloat(float Amount);

	// --- Experience (HudPlayerFrameWidgetXP.cpp) ---

	void HandleXPChanged(int64 Gained, EXPSource Source);
	void HandleLevelUp(int32 NewLevel);
	/** Finds the player's progression subsystem and listens to it (once; the subsystem outlives the HUD). False while there is none. */
	bool BindProgression();
	/** Aims the bar at the player's current progress; bSnap jumps there instead of easing. */
	void Retarget(bool bSnap);
	/**
	 * Draws ShownProgress: the level in the gem and the bar, with the stretch still to catch up to. A level the bar reached
	 * by easing across it is celebrated (bCelebrate); one it jumped to (a snap, the skip over levels of a huge gain) is shown quietly.
	 */
	void ShowProgress(bool bCelebrate = true);
	void SetShownLevel(int32 Level, bool bCelebrate);
	void UpdateXPText();
	/** Moves the floating "+160 XP", the gem's flash and its ring; false once none of them is running. */
	bool TickXPEffects(float DeltaTime);

	/** The root canvas: the idle fade. */
	UPROPERTY(Transient) TObjectPtr<UWidget> Cluster;
	UPROPERTY(Transient) TObjectPtr<UHudPortraitWidget> Portrait;
	UPROPERTY(Transient) TObjectPtr<UImage> GemFlash;
	UPROPERTY(Transient) TObjectPtr<UImage> GemRing;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelText;
	UPROPERTY(Transient) TObjectPtr<UImage> Shine;
	/** "78" and "/ 100" in the middle of the health bar, and the "+30" that rises at its end on a heal. */
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HealthValue;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HealthMax;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HealFloat;
	/** "1,240" and "/ 2,000 XP" after the experience bar ("MAX" alone at the top level), and the "+160 XP" over them. */
	UPROPERTY(Transient) TObjectPtr<UTextBlock> XPValue;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> XPMax;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> XPFloat;
	/** Per stretch (EHudFrameStretch): its cropped body, and its end piece moved along to the level. */
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> StretchBodies;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> StretchCaps;

	/** Per stretch, its body's whole brush (cropped from), and where its end is drawn (in its bar's pixels; -1 hidden). */
	TArray<FSlateBrush> StretchBrushes;
	TArray<float> StretchShown;
	/** The shine's whole brush, and whether it's showing. */
	FSlateBrush ShineBrush;
	bool bShineShown = false;

	TWeakObjectPtr<UPlayerProgressionSubsystem> Progression;

	/** Seconds left of the full-strength hold after something happened (a hit, a heal, experience). */
	float Activity = 0.f;

	// Health: what the bar shows (0-1 of it), and how it got there.
	/** The health last seen in points (-1 before the first), and as a share of the maximum. */
	float LastHealth = -1.f;
	float LastFraction = -1.f;
	float FillShown = 0.f;
	/** A heal's rise: from FillFrom to FillTo over the rise's length, RiseTime into it. */
	float FillFrom = 0.f;
	float FillTo = 0.f;
	float RiseTime = 0.f;
	/** The chip: it holds at ChipFrom for ChipHold seconds after a hit, then drains to the fill, ChipDrain seconds into that. */
	float ChipShown = 0.f;
	float ChipFrom = 0.f;
	float ChipHold = 0.f;
	float ChipDrain = 0.f;
	/** Seconds into the heal's shine (it runs while under its length), and the "+30" still floating, with its amount. */
	float ShineTime = 1000.f;
	float HealFloatTime = 0.f;
	float HealAmount = 0.f;
	/**
	 * Regeneration: the points the current run has healed, whether the last frame was part of one, how fast the shine
	 * runs (1 for a heal's; slower for a regeneration's gentle sweeps) and the pause left before the next gentle sweep.
	 */
	float RegenGained = 0.f;
	bool bRegenShown = false;
	float ShineSpeed = 1.f;
	float RegenShineWait = 0.f;
	int32 ShownPoints = INDEX_NONE;
	int32 ShownMaxPoints = INDEX_NONE;
	bool bShownLow = false;
	bool bBeatShown = false;

	// Experience, as HudXPBarWidget had it: level plus the fraction through it, shown and aimed at.
	double ShownProgress = -1.0;
	double TargetProgress = 0.0;
	bool bMaxLevel = false;
	int32 ShownLevel = INDEX_NONE;
	/** The numbers wait for the bar to reach the new level, so they and the bar agree. */
	bool bXPTextPending = false;
	/** The highest level gained but not yet announced; announced when the bar gets there. */
	int32 PendingAnnouncement = 0;
	/** After a gain the bar waits this long before catching up, so the new stretch reads on its own. */
	float GainHoldTime = 0.f;
	/**
	 * The just-earned stretch: where in the shown level it starts (-1 when there's none), its whiteness (1 to 0), the time
	 * since the last gain, and how far its fade has got once the bar has caught up.
	 */
	float GainStart = -1.f;
	float GainAlpha = 0.f;
	float GainAge = 0.f;
	float GainFade = 0.f;
	/** The "+160 XP" still floating, and what it says. */
	float XPFloatTime = 0.f;
	int64 ShownGain = 0;
	/** Seconds left of the gem's level-up flash and of its spreading ring. */
	float GemFlashTime = 0.f;
	float RingTime = 0.f;
};
