// UHudMissionTrackerWidget: following the tracked mission, the closing line, painting what it shows, and its motion.
// Its widgets and pictures are built in HudMissionTrackerWidgetLayout.cpp.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "Scenes/SceneSubsystem.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** Seconds for the whole tracker to fade in or out. */
	constexpr float TrackerFadeSeconds = 0.35f;
	/** The next objective slides in from this far left, fading in over SlideSeconds (the mockup's). */
	constexpr float SlideDistance = 14.f;
	constexpr float SlideSeconds = 0.45f;
	/** A new count pops to this size, white, and settles over PopSeconds. */
	constexpr float PopScale = 1.35f;
	constexpr float PopSeconds = 0.3f;

	/** Fast at first, settling gently, like the mockup's CSS ease-out. */
	float TrackerEaseOut(float Share)
	{
		const float Remaining = 1.f - FMath::Clamp(Share, 0.f, 1.f);
		return 1.f - Remaining * Remaining;
	}

	/** A step bar section's two flat halves, light over dark. */
	struct FTrackerSectionColors
	{
		FLinearColor Upper;
		FLinearColor Lower;
	};

	/** A step done: the experience bar's cyan; the current one: orange; one still to come: a faint cyan on the dark track. */
	FTrackerSectionColors SectionColorsFor(int32 Index, int32 DoneCount, int32 Current)
	{
		if (Index < DoneCount)
		{
			return { Color::XPLight(), Color::XPDark() };
		}
		if (Index == Current)
		{
			return { Color::AccentLight(), Color::Accent() };
		}
		const FLinearColor Faint = Color::Hairline().CopyWithNewOpacity(0.17f);
		return { Faint, Faint };
	}
}

void UHudMissionTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// It never takes the mouse: it sits over the inventory on the step done there.
	SetVisibility(ESlateVisibility::HitTestInvisible);
	UWorld* World = GetWorld();
	UMissionSubsystem* Subsystem = World ? World->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (Subsystem && !ChangedHandle.IsValid())
	{
		Missions = Subsystem;
		ChangedHandle = Subsystem->OnMissionsChanged.AddUObject(this, &UHudMissionTrackerWidget::HandleMissionsChanged);
	}
	bRefreshPending = true;
}

void UHudMissionTrackerWidget::NativeDestruct()
{
	if (UMissionSubsystem* Subsystem = Missions.Get())
	{
		Subsystem->OnMissionsChanged.Remove(ChangedHandle);
	}
	ChangedHandle.Reset();
	Missions.Reset();
	Super::NativeDestruct();
}

void UHudMissionTrackerWidget::HandleMissionsChanged()
{
	// A runner update can add, change and remove missions in one go: look once, on the next tick, at how they ended up.
	bRefreshPending = true;
}

void UHudMissionTrackerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Block)
	{
		return;
	}
	// A done objective or the closing line holds the tracker until it has shown its time; changes wait for it.
	if (bRefreshPending && (Phase == EPhase::Idle || Phase == EPhase::Showing))
	{
		Refresh();
	}
	// Time under a menu doesn't count, so the tick or the closing line still shows once the menu closes.
	if ((Phase == EPhase::Finishing || Phase == EPhase::Closing) && !IsCovered())
	{
		HoldLeft -= InDeltaTime;
		if (HoldLeft <= 0.f)
		{
			EndHold();
		}
	}
	Animate(InDeltaTime, Phase != EPhase::Idle && !IsCovered());
}

// ---------------------------------------------------------------------------
// Following the tracked mission
// ---------------------------------------------------------------------------

UHudMissionTrackerWidget::EChange UHudMissionTrackerWidget::DecideChange(const FMissionTrackerParts& Shown, const FMissionTrackerParts& Now,
	bool bJustShown)
{
	// Started over: an earlier step, or an earlier objective of the same step (the step played again).
	if (Now.Step < Shown.Step || (Now.Step == Shown.Step && Now.ObjectiveIndex < Shown.ObjectiveIndex))
	{
		return EChange::Present;
	}
	const bool bStepMoved = Now.Step > Shown.Step;
	const bool bObjectiveMoved = Now.Step == Shown.Step && Now.ObjectiveIndex > Shown.ObjectiveIndex;
	// Nothing was on show to tick: what comes is shown anew. (Empty on both sides and still the same objective: nothing to
	// show, so nothing to slide in again at every look.)
	if (Shown.Line.IsEmpty())
	{
		return (bStepMoved || bObjectiveMoved || !Now.Line.IsEmpty()) ? EChange::Present : EChange::InPlace;
	}
	if (!bStepMoved && !bObjectiveMoved)
	{
		// The same objective: its words may differ (a key rebound is resolved into the line), which is no tick.
		return EChange::InPlace;
	}
	if (bJustShown)
	{
		return EChange::Present;
	}
	return bStepMoved ? EChange::StepDone : EChange::ObjectiveDone;
}

void UHudMissionTrackerWidget::Refresh()
{
	bRefreshPending = false;
	const UMissionSubsystem* Subsystem = Missions.Get();
	const FMission* Tracked = Subsystem ? Subsystem->GetTracked() : nullptr;
	if (Phase == EPhase::Showing)
	{
		if (Tracked && Tracked->Id == ShownMission)
		{
			// An objective still sliding in was hardly seen (a saved session picking up its step as the level starts):
			// the next one takes its place without a tick.
			const bool bJustShown = SlideAge < SlideSeconds;
			switch (DecideChange(Shown, Tracked->Tracker, bJustShown))
			{
			case EChange::Present:
				// Started over, nothing was on show to tick, or it was only just shown.
				Present(*Tracked);
				break;
			case EChange::StepDone:
				// The step is done (several at once, when the next ones were done already): tick it, then the next.
				BeginFinish(/*bStepDone*/ true);
				break;
			case EChange::ObjectiveDone:
				// One objective of the step done, the next one up.
				BeginFinish(/*bStepDone*/ false);
				break;
			case EChange::InPlace:
				UpdateInPlace(*Tracked);
				break;
			}
			return;
		}
		// The mission shown is over (gone from the list): its last objective ticks before whatever comes next. One the
		// player stopped tracking just makes way.
		if (!Subsystem || !Subsystem->FindMission(ShownMission))
		{
			BeginFinish(/*bStepDone*/ true);
			return;
		}
	}
	if (Tracked)
	{
		Present(*Tracked);
	}
	else
	{
		// Nothing to follow: the tracker fades out with what it showed last.
		Phase = EPhase::Idle;
		ShownMission = INDEX_NONE;
	}
}

void UHudMissionTrackerWidget::Present(const FMission& Mission)
{
	Phase = EPhase::Showing;
	ShownMission = Mission.Id;
	ShownTitle = Mission.Title;
	Shown = Mission.Tracker;
	Shown.bDone = false;
	PaintTitle(ShownTitle);
	PaintSteps(Shown.Step, Shown.Step, Shown.StepCount, Shown.Step + 1);
	PaintObjective();
	PaintHint();
	PopAge = PopSeconds;
	ApplyPop(1.f);
	StartSlide();
}

void UHudMissionTrackerWidget::UpdateInPlace(const FMission& Mission)
{
	const FMissionTrackerParts& Now = Mission.Tracker;
	const bool bNewTitle = !Mission.Title.EqualTo(ShownTitle);
	// The same objective with other words (a rebound key) is repainted, with no tick and no slide.
	const bool bNewLine = !Now.Line.Equals(Shown.Line, ESearchCase::CaseSensitive);
	const bool bNewCount = !Now.Count.Equals(Shown.Count, ESearchCase::CaseSensitive);
	// Only a count of things pops: one of seconds held would pop every second.
	const bool bCountPops = Now.Progress > Shown.Progress && !Now.bCountIsTime;
	const bool bNewHint = !Now.HintKey.Equals(Shown.HintKey, ESearchCase::CaseSensitive)
		|| !Now.HintText.Equals(Shown.HintText, ESearchCase::CaseSensitive);
	const bool bNewSteps = Now.StepCount != Shown.StepCount;
	Shown = Now;
	if (bNewTitle)
	{
		ShownTitle = Mission.Title;
		PaintTitle(ShownTitle);
	}
	if (bNewLine || bNewCount)
	{
		PaintObjective();
		if (bNewCount && bCountPops)
		{
			StartPop();
		}
	}
	if (bNewHint)
	{
		PaintHint();
	}
	if (bNewSteps)
	{
		PaintSteps(Shown.Step, Shown.Step, Shown.StepCount, Shown.Step + 1);
	}
}

void UHudMissionTrackerWidget::BeginFinish(bool bStepDone)
{
	Phase = EPhase::Finishing;
	HoldLeft = DoneHoldSeconds;
	Shown.bDone = true;
	// The last count lands as it ticks ("5 / 5"): the runner has moved on before the tracker saw it.
	const bool bCountLands = !Shown.CountDone.IsEmpty() && !Shown.Count.Equals(Shown.CountDone, ESearchCase::CaseSensitive);
	if (bCountLands)
	{
		Shown.Count = Shown.CountDone;
		Shown.Progress = Shown.Required;
	}
	PaintObjective();
	if (bCountLands && !Shown.bCountIsTime)
	{
		StartPop();
	}
	if (bStepDone)
	{
		PaintSteps(Shown.Step + 1, INDEX_NONE, Shown.StepCount, Shown.Step + 1);
	}
}

void UHudMissionTrackerWidget::ShowClosing()
{
	bClosingPending = false;
	Phase = EPhase::Closing;
	HoldLeft = ClosingSeconds;
	// It belongs to no mission now: the next tracked one (the skiff) follows it as a mission of its own.
	ShownMission = INDEX_NONE;
	ShownTitle = ClosingTitle;
	Shown = FMissionTrackerParts();
	Shown.Line = ClosingLine;
	Shown.StepCount = ClosingSteps;
	Shown.Step = ClosingSteps - 1;
	Shown.bDone = true;
	PaintTitle(ShownTitle);
	PaintSteps(ClosingSteps, INDEX_NONE, ClosingSteps, ClosingSteps);
	PaintObjective();
	PaintHint();
	PopAge = PopSeconds;
	ApplyPop(1.f);
	StartSlide();
}

void UHudMissionTrackerWidget::EndHold()
{
	if (bClosingPending)
	{
		ShowClosing();
		return;
	}
	// The next objective of the same mission, the next tracked mission, or nothing (the tracker fades out).
	Phase = EPhase::Idle;
	ShownMission = INDEX_NONE;
	Refresh();
}

void UHudMissionTrackerWidget::ShowClosingLine(const FText& Title, int32 StepCount, const FString& Line, float Seconds)
{
	bClosingPending = true;
	ClosingTitle = Title;
	ClosingSteps = FMath::Max(1, StepCount);
	ClosingLine = Line;
	ClosingSeconds = Seconds;
	switch (Phase)
	{
	case EPhase::Showing:
		// Its mission's last objective, still on show (the runner's news comes on the next tick): it ticks first.
		if (ShownTitle.EqualTo(Title))
		{
			BeginFinish(/*bStepDone*/ true);
			return;
		}
		ShowClosing();
		return;
	case EPhase::Finishing:
		// It follows the tick (EndHold).
		return;
	case EPhase::Idle:
	case EPhase::Closing:
		ShowClosing();
		return;
	}
}

void UHudMissionTrackerWidget::ClearClosingLine()
{
	bClosingPending = false;
	if (Phase == EPhase::Closing)
	{
		Phase = EPhase::Idle;
		bRefreshPending = true;
	}
}

bool UHudMissionTrackerWidget::IsCovered() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	if (LooterHUD && LooterHUD->IsMenuOpen())
	{
		// An objective done in the inventory (the tutorial's last step) keeps the tracker up over it, so the player sees it
		// tick; every other menu covers it, the closing line's too.
		const bool bOverInventory = LooterHUD->IsInventoryOpen() && !LooterHUD->IsStationBoardOpen() && Shown.bOverInventory
			&& (Phase == EPhase::Showing || Phase == EPhase::Finishing);
		if (!bOverInventory)
		{
			return true;
		}
	}
	// A scene that holds the player (the skiff ride) puts the gameplay HUD away, and the tracker with it.
	if (USceneSubsystem::HidesGameplayHUD(this))
	{
		return true;
	}
	// The pause menu pauses the game.
	const UWorld* World = GetWorld();
	return World && World->IsPaused();
}

// ---------------------------------------------------------------------------
// Painting what's shown (only when it changes)
// ---------------------------------------------------------------------------

void UHudMissionTrackerWidget::PaintTitle(const FText& Title)
{
	if (TitleText)
	{
		TitleText->SetText(Title.ToUpper());
	}
}

void UHudMissionTrackerWidget::PaintSteps(int32 DoneCount, int32 Current, int32 StepCount, int32 Number)
{
	if (!StepBar || !StepText)
	{
		return;
	}
	// A mission of one step has no bar.
	const bool bBar = StepCount > 1;
	const ESlateVisibility BarVisibility = bBar ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	StepBar->SetVisibility(BarVisibility);
	StepText->SetVisibility(BarVisibility);
	if (!bBar)
	{
		return;
	}
	EnsureSections(StepCount);
	for (int32 Index = 0; Index < Sections.Num(); ++Index)
	{
		if (Index >= StepCount)
		{
			Sections[Index]->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		Sections[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
		const FTrackerSectionColors Colors = SectionColorsFor(Index, DoneCount, Current);
		SectionUppers[Index]->SetColorAndOpacity(Colors.Upper);
		SectionLowers[Index]->SetColorAndOpacity(Colors.Lower);
	}
	StepText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::Clamp(Number, 1, StepCount), StepCount)));
}

void UHudMissionTrackerWidget::PaintObjective()
{
	if (!LineText || !CountText || !Chevron || !TickMark)
	{
		return;
	}
	// Done: the chevron turns into the tick and the words dim; the count keeps its cyan.
	LineText->SetText(FText::FromString(Shown.Line));
	LineText->SetColorAndOpacity(FSlateColor(Shown.bDone ? Color::TextDim() : FLinearColor::White));
	Chevron->SetVisibility(Shown.bDone ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	TickMark->SetVisibility(Shown.bDone ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	CountText->SetText(FText::FromString(Shown.Count));
	CountText->SetVisibility(Shown.Count.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UHudMissionTrackerWidget::PaintHint()
{
	if (!Hint || !KeyText || !HintText)
	{
		return;
	}
	// Only for objectives that teach a key.
	const bool bHint = !Shown.HintKey.IsEmpty();
	Hint->SetVisibility(bHint ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bHint)
	{
		KeyText->SetText(FText::FromString(Shown.HintKey));
		HintText->SetText(FText::FromString(Shown.HintText));
	}
}

// ---------------------------------------------------------------------------
// Motion
// ---------------------------------------------------------------------------

void UHudMissionTrackerWidget::Animate(float DeltaTime, bool bVisible)
{
	const float Target = bVisible ? 1.f : 0.f;
	if (BlockOpacity != Target)
	{
		BlockOpacity = FMath::Clamp(BlockOpacity + (bVisible ? 1.f : -1.f) * DeltaTime / TrackerFadeSeconds, 0.f, 1.f);
		Block->SetRenderOpacity(BlockOpacity);
		Block->SetVisibility(BlockOpacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (SlideAge < SlideSeconds)
	{
		SlideAge = FMath::Min(SlideAge + DeltaTime, SlideSeconds);
		ApplySlide(SlideAge / SlideSeconds);
	}
	if (PopAge < PopSeconds)
	{
		PopAge = FMath::Min(PopAge + DeltaTime, PopSeconds);
		ApplyPop(PopAge / PopSeconds);
	}
}

void UHudMissionTrackerWidget::StartSlide()
{
	// The first frame already shows it at the start of its slide, so the new words never flash in place.
	SlideAge = 0.f;
	ApplySlide(0.f);
}

void UHudMissionTrackerWidget::StartPop()
{
	PopAge = 0.f;
	ApplyPop(0.f);
}

void UHudMissionTrackerWidget::ApplySlide(float DoneShare)
{
	if (Objective)
	{
		const float Eased = TrackerEaseOut(DoneShare);
		Objective->SetRenderTranslation(FVector2D(-SlideDistance * (1.f - Eased), 0.f));
		Objective->SetRenderOpacity(Eased);
	}
}

void UHudMissionTrackerWidget::ApplyPop(float DoneShare)
{
	if (CountText)
	{
		const float Eased = TrackerEaseOut(DoneShare);
		CountText->SetRenderScale(FVector2D(FMath::Lerp(PopScale, 1.f, Eased)));
		CountText->SetColorAndOpacity(FSlateColor(FMath::Lerp(FLinearColor::White, Color::CyanText(), Eased)));
	}
}
