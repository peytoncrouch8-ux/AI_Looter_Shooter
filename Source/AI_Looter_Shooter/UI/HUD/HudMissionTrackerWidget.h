#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Missions/MissionSubsystem.h"
#include "HudMissionTrackerWidget.generated.h"

class UHorizontalBox;
class UImage;
class UMissionSubsystem;
class UTextBlock;
class UWidget;

/**
 * The tracked mission (UMissionSubsystem::GetTracked), on the left of the screen from (36, 286) at 1080p, drawn in the
 * HUD's own metalwork: a ranger's star medal, the mission's name in caps, a slanted bar with a section per step (done
 * cyan, the current one orange) and "4 / 6", a cyan route line down to the objective row (an orange chevron, the short
 * line and its count) and a key hint under it (the bound key as a keycap and what it does). It serves every mission,
 * stays up through boss fights and takes no part in the idle fade; it steps aside while a menu is open, except over the
 * inventory for an objective done there (the tutorial's last step).
 *
 * A rising count of things pops (a count of seconds held doesn't); a done objective's chevron turns into a cyan tick and
 * its words dim (an objective is done when the objective or the step in the mission's parts moves on, never when only its
 * words change: a rebound key rewrites them in place), its step's section turns
 * cyan, and after DoneHoldSeconds the next objective slides in and its section turns orange. The tutorial's closing
 * line (ShowClosingLine, from ATutorialDirector through ALooterHUD) shows as a last, ticked objective for its seconds of
 * play; then the next tracked mission slides in, or the tracker fades out. ALooterHUD makes it as a viewport widget of
 * its own, over the inventory's pages. Floating outlined text and solid shapes, no panels: only the step bar's track,
 * the medal's glass and the keycap's plate are backgrounds (the UI transparency setting fades them). It repaints only
 * when the mission changes, and does nothing per frame unless something is moving.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudMissionTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Over the inventory's pages (20), so it can stay up on a step done there; under the station board (25) and the pause menu (40). */
	static constexpr int32 ViewportZOrder = 22;

	/** Its block's top-left corner, the medal's box, in 1080p pixels from the screen's top-left. */
	static constexpr float Left = 36.f;
	static constexpr float Top = 286.f;

	/** How long a done objective shows ticked before the next one slides in. */
	static constexpr float DoneHoldSeconds = 1.4f;

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
	 * show is still sliding in, so hardly seen.
	 */
	static EChange DecideChange(const FMissionTrackerParts& Shown, const FMissionTrackerParts& Now, bool bJustShown);

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

	void HandleMissionsChanged();
	/** Brings the tracker in line with the tracked mission (only while it shows one, or nothing). */
	void Refresh();
	/** Shows a mission's objective, sliding in. */
	void Present(const FMission& Mission);
	/** The same objective, moved on: its count (popping as it rises), its hint, its step count. */
	void UpdateInPlace(const FMission& Mission);
	/** Ticks the objective shown and holds it for DoneHoldSeconds; bStepDone turns its step's section cyan too. */
	void BeginFinish(bool bStepDone);
	/** The tick's sound (HudMissionTrackerWidgetSound.cpp): a step's, or the mission's when its last step is done. */
	void PlayFinishSound(bool bStepDone) const;
	void ShowClosing();
	/** A done objective or the closing line has shown long enough: what's next. */
	void EndHold();
	/** A menu covers the game (but the inventory for an objective done there), a scene holds the player, or it's paused. */
	bool IsCovered() const;
	/** The block's fade, the objective's slide-in and the count's pop. */
	void Animate(float DeltaTime, bool bVisible);
	void StartSlide();
	void StartPop();
	/** The count's size and color DoneShare of the way from its pop (0) to rest (1). */
	void ApplyPop(float DoneShare);
	/** The objective's offset and opacity DoneShare of the way through its slide-in. */
	void ApplySlide(float DoneShare);

	// --- Painting what's shown (HudMissionTrackerWidget.cpp) ---

	void PaintTitle(const FText& Title);
	/** The step bar: DoneCount sections cyan, Current orange (INDEX_NONE: none), the rest dark; "Number / StepCount". */
	void PaintSteps(int32 DoneCount, int32 Current, int32 StepCount, int32 Number);
	/** The objective row from Shown: the chevron or the tick, the line (dim once done) and the count. */
	void PaintObjective();
	void PaintHint();

	// --- The widgets (HudMissionTrackerWidgetLayout.cpp) ---

	void BuildTree();
	/** Makes sure the step bar has at least Count sections (made once, kept). */
	void EnsureSections(int32 Count);

	/** Everything, faded in and out as one. */
	UPROPERTY(Transient) TObjectPtr<UWidget> Block;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UWidget> StepBar;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> SectionRow;
	/** Per section, left to right: the section, its upper and its lower half. */
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Sections;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SectionUppers;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SectionLowers;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StepText;
	/** The objective row and the hint under it, sliding in together. */
	UPROPERTY(Transient) TObjectPtr<UWidget> Objective;
	UPROPERTY(Transient) TObjectPtr<UImage> Chevron;
	UPROPERTY(Transient) TObjectPtr<UImage> TickMark;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LineText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(Transient) TObjectPtr<UWidget> Hint;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HintText;

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

	/** The block's opacity, eased toward shown or hidden. */
	float BlockOpacity = 0.f;
	/** Seconds into the objective's slide-in and the count's pop; at or past their lengths they're at rest. */
	float SlideAge = 1000.f;
	float PopAge = 1000.f;
};
