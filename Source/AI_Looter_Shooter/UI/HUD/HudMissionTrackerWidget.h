#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Missions/MissionSubsystem.h"
#include "UI/HUD/HudMissionTrackerShapes.h"
#include "HudMissionTrackerWidget.generated.h"

class UCanvasPanelSlot;
class UHorizontalBox;
class UImage;
class UMissionSubsystem;
class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * The tracked mission (UMissionSubsystem::GetTracked), top-left a little under the corner (the user's call, 2026-10-31;
 * it was under the minimap), its lines left-aligned, drawn in the HUD's own metalwork: a ranger's star medal, then the
 * mission's name in caps; under it the objective row (its big circle under the medal, then the short line and its count)
 * and a key hint under the line (the bound key as a keycap and what it does). No step count and no step circles (the
 * user, 2026-10-31: "it should just tell you what your next objective is"). It serves every mission, stays up through boss fights and takes no part in the idle fade; it steps
 * aside while any menu is open (the inventory too, even for an objective done there: its check waits for the menu to close),
 * and while the loot card would cover it (the player looks at a gun on the ground).
 *
 * A rising count of things pops (a count of seconds held doesn't); a done objective's ring turns into a checked cyan
 * disc that pops in and its words dim (an objective is done when the objective or the step in the mission's parts moves
 * on, never when only its words change: a rebound key rewrites them in place), and after DoneHoldSeconds the next
 * objective slides in from the left. The
 * step's optional objectives sit under it, smaller and dimmed, a dashed ring for a mark, "Optional:" after it: one done
 * is checked where it stands (the check pops, the line flashes cyan and settles, its own small sound) and stays checked
 * until the step ends. Once every step is done a mission waiting for its turn-in shows "Turn in to <who>", checked as
 * it's turned in. The tutorial's closing line (ShowClosingLine, from ATutorialDirector through
 * ALooterHUD) shows as a last, checked objective for its seconds of play; then the next tracked mission slides in, or the
 * tracker fades out. ALooterHUD makes it as a viewport widget of its own, under every menu. Floating outlined
 * text and solid shapes, no panels: only the medal's glass and the keycap's plate are backgrounds (the UI transparency
 * setting fades them). It repaints only when the mission changes, and does nothing per frame unless something is moving.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudMissionTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Under every menu (the inventory's pages 20, the boards 25, the offer card 30, the pause menu 40), so even its fade-out
	 * as one opens is behind it; over the captions (2) and the boss bar (3).
	 */
	static constexpr int32 ViewportZOrder = 4;

	/**
	 * Its block's top-left corner in 1080p pixels: in from the left edge as far as the minimap is from the right
	 * (UHudMinimapWidget::Margin, the frame rate counter's and the control hint's edge too), a little under the corner
	 * (the frame rate counter, when it's on, sits in the corner over it).
	 */
	static constexpr float Left = 36.f;
	static constexpr float Top = 96.f;

	/**
	 * The widest the block gets: its long lines wrap inside it, so it stays clear of what's at the top's middle (the boss
	 * bar's cluster reaches 380 px either side of it, the level-up banner 350).
	 */
	static constexpr float MaxWidth = 540.f;

	/** The mark an objective shows: its ring while it's to do, its check once done (an optional one's dashed ring and check). */
	static EHudTrackerMark ObjectiveMark(bool bDone, bool bOptional);

	/**
	 * Whether it steps aside: any menu open (the inventory too), a scene holding the player, the game paused, or the loot
	 * card where it stands. Decided from those alone, so a test can run it without a widget.
	 */
	static bool CoveredBy(bool bMenuOpen, bool bSceneHidesHUD, bool bPaused, bool bMakingWay);

	/** How long a done objective shows checked before the next one slides in. */
	static constexpr float DoneHoldSeconds = 1.4f;

	/** Moves it on by DeltaSeconds as a frame does (NativeTick calls it; the tests call it directly). */
	void Advance(float DeltaSeconds);

	/** What it shows, for the tests: the objective's mark and each optional objective's mark. */
	EHudTrackerMark GetShownObjectiveMark() const;
	TArray<EHudTrackerMark> GetShownOptionalMarks() const;

	/** Where its block's top-left corner is placed (1080p pixels from the screen's top-left). */
	FVector2D GetPlacedCorner() const;

	/**
	 * A mission's closing line (the tutorial's "You're ready..."): a last, ticked objective under Title with all
	 * StepCount steps done, for Seconds of play (time under a menu doesn't count). A mission with that title on show ticks
	 * its objective first.
	 */
	void ShowClosingLine(const FText& Title, int32 StepCount, const FString& Line, float Seconds);

	/** Takes the closing line away, shown or waiting (the tutorial restarted or skipped). */
	void ClearClosingLine();

	/** What the tracker does when the tracked mission's parts (Now) differ from the ones on show (Shown). */
	enum class EChange : uint8
	{
		/** The same objective: its words, count, hint or step count are repainted where they stand, with no tick. */
		InPlace,
		/** Shown anew, sliding in, with no tick: started over, nothing was on show, or it was hardly seen yet. */
		Present,
		/** The objective on show is done (the step's next one is up): it ticks, then the next slides in. */
		ObjectiveDone,
		/** The objective on show is done and so is its step (the next step is up, or several at once). */
		StepDone,
	};

	/**
	 * Decides from the parts alone, so a test can run it without a widget. An objective is done when the step or the
	 * objective's index moves on; the words changing on their own (a key rebound) is no tick. bJustShown: the objective on
	 * show is still sliding in, so hardly seen. Optional objectives never move it: they change in place.
	 */
	static EChange DecideChange(const FMissionTrackerParts& Shown, const FMissionTrackerParts& Now, bool bJustShown);

	/** The optional objectives (by index) done in Now that weren't in Shown: each ticks where it stands, with its sound. */
	static TArray<int32> NewlyDoneOptionals(const FMissionTrackerParts& Shown, const FMissionTrackerParts& Now);

	/** How long an optional objective's tick pops and its line flashes. */
	static constexpr float OptionalTickSeconds = 0.7f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** What the tracker is doing: nothing, a mission's objective, a done objective held ticked, or the closing line. */
	enum class EPhase : uint8
	{
		Idle,
		Showing,
		Finishing,
		Closing,
	};

	// --- Following the tracked mission (HudMissionTrackerWidget.cpp) ---

	/** Listens to the world's missions (once; again if the world's subsystem changed). */
	void BindMissions();
	void HandleMissionsChanged();
	/** Brings the tracker in line with the tracked mission (only while it shows one, or nothing). */
	void Refresh();
	/** Shows a mission's objective, sliding in. */
	void Present(const FMission& Mission);
	/** The same objective, moved on: its count (popping as it rises), its hint, its step count. */
	void UpdateInPlace(const FMission& Mission);
	/**
	 * Checks the objective shown and holds it for DoneHoldSeconds; bStepDone: its step is done too (its chime). bMissionEnded:
	 * the mission is over (gone from the list, or the tutorial's closing line follows), not just on to its next step or its
	 * turn-in.
	 */
	void BeginFinish(bool bStepDone, bool bMissionEnded = false);
	/**
	 * The tick's sound (HudMissionTrackerWidgetSound.cpp): a step's chime (the last objective done before a turn-in too),
	 * or the mission's fanfare as it ends, unless the mission-complete banner sounds it.
	 */
	void PlayFinishSound(bool bStepDone, bool bMissionEnded) const;
	void ShowClosing();
	/** A done objective or the closing line has shown long enough: what's next. */
	void EndHold();
	/**
	 * CoveredBy, as things stand: a menu covers the game, a scene holds the player, it's paused, or the loot card would
	 * cover it (IsMakingWay). Time covered doesn't count toward a hold.
	 */
	bool IsCovered() const;
	/**
	 * The loot comparison card is up where the tracker is (a long mission under a big minimap reaches the screen's middle,
	 * where the card stands): the tracker steps aside until it goes, rather than draw over it.
	 */
	bool IsMakingWay() const;
	/** The block's fade, the objective's slide-in, the count's pop and the checks' pops. */
	void Animate(float DeltaTime, bool bVisible);
	void StartSlide();
	void StartPop();
	/** The count's size and color DoneShare of the way from its pop (0) to rest (1). */
	void ApplyPop(float DoneShare);
	/** The objective's offset and opacity DoneShare of the way through its slide-in. */
	void ApplySlide(float DoneShare);

	// --- Painting what's shown (HudMissionTrackerWidgetPaint.cpp) ---

	void PaintTitle(const FText& Title);
	/** The objective row from Shown: the ring or the check, the line (dim once done) and the count. */
	void PaintObjective();
	void PaintHint();
	/** The objective's check pops in (it was just done). */
	void StartCheckPop();
	/** The objective's check pop, moved on. */
	void AnimateChecks(float DeltaTime);

	// --- The widgets (HudMissionTrackerWidgetLayout.cpp) ---

	void BuildTree();

	// --- Optional objectives under the objective (HudMissionTrackerWidgetOptional.cpp) ---

	/** The box their rows go in, under the objective and its hint (BuildTree places it). */
	UWidget* BuildOptionals();
	/** Makes sure there are at least Count rows (made once, kept). */
	void EnsureOptionalRows(int32 Count);
	/** The rows from Shown.Optionals: a dashed ring (the check once done), "Optional:" and the line, dimmed. */
	void PaintOptionals();
	/** An optional objective just done: its check pops, its line flashes cyan and settles dim. */
	void StartOptionalTick(int32 Index);
	void AnimateOptionals(float DeltaTime);
	/** An optional objective's tick sound (HudMissionTrackerWidgetSound.cpp). */
	void PlayOptionalSound() const;

	/** Everything, faded in and out as one; its slot on the root canvas places it (Left, Top). */
	UPROPERTY(Transient) TObjectPtr<UWidget> Block;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> BlockSlot;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	/** The objective row and the hint under it, sliding in together. */
	UPROPERTY(Transient) TObjectPtr<UWidget> Objective;
	UPROPERTY(Transient) TObjectPtr<UImage> ObjectiveRing;
	UPROPERTY(Transient) TObjectPtr<UImage> ObjectiveCheck;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LineText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountText;
	/** Where the objective line wraps: with a count beside it, and alone (it takes the count's room). */
	float LineWrapCounted = 0.f;
	float LineWrapAlone = 0.f;
	UPROPERTY(Transient) TObjectPtr<UWidget> Hint;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HintText;
	/** The optional objectives' rows, each its mark (a dashed ring, the check over it once done), "Optional:", the line and a count. */
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> OptionalBox;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> OptionalRows;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> OptionalMarks;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> OptionalChecks;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptionalLines;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptionalCounts;
	/** Seconds into each row's tick (at or past OptionalTickSeconds: at rest). */
	TArray<float> OptionalTickAges;

	/** Seconds into the objective's check popping in. */
	float CheckPopAge = 1000.f;

	TWeakObjectPtr<UMissionSubsystem> Missions;
	FDelegateHandle ChangedHandle;

	EPhase Phase = EPhase::Idle;
	/** The mission shown (its id in UMissionSubsystem), INDEX_NONE for none or the closing line; its title and parts. */
	int32 ShownMission = INDEX_NONE;
	FText ShownTitle;
	FMissionTrackerParts Shown;
	/** The missions changed since the tracker last looked; it looks on its next tick (several changes come together). */
	bool bRefreshPending = true;
	/** Seconds left of the done objective's hold or of the closing line (counted only while it's on screen). */
	float HoldLeft = 0.f;

	/** A closing line waiting for the tick before it. */
	bool bClosingPending = false;
	FText ClosingTitle;
	FString ClosingLine;
	int32 ClosingSteps = 1;
	float ClosingSeconds = 0.f;

	/** The next objective slides in over this long (the mockup's); a new count pops and settles over this long. */
	static constexpr float SlideSeconds = 0.45f;
	static constexpr float PopSeconds = 0.3f;

	/** The block's opacity, eased toward shown or hidden. */
	float BlockOpacity = 0.f;
	/** Seconds into the objective's slide-in and the count's pop; at or past their lengths they're at rest. */
	float SlideAge = 1000.f;
	float PopAge = 1000.f;
};
