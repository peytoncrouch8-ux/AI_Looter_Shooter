// UHudMissionTrackerWidget: following the tracked mission and the closing line. Its widgets are built in
// HudMissionTrackerWidgetLayout.cpp, where it stands and how it moves is HudMissionTrackerWidgetMotion.cpp's.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "Engine/World.h"

void UHudMissionTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// It never takes the mouse: it sits over the inventory on the step done there.
	SetVisibility(ESlateVisibility::HitTestInvisible);
	BindMissions();
	bRefreshPending = true;
}

void UHudMissionTrackerWidget::BindMissions()
{
	UWorld* World = GetWorld();
	UMissionSubsystem* Subsystem = World ? World->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (!Subsystem || Missions.Get() == Subsystem)
	{
		return;
	}
	if (UMissionSubsystem* Old = Missions.Get())
	{
		Old->OnMissionsChanged.Remove(ChangedHandle);
	}
	Missions = Subsystem;
	ChangedHandle = Subsystem->OnMissionsChanged.AddUObject(this, &UHudMissionTrackerWidget::HandleMissionsChanged);
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
	Advance(InDeltaTime);
}

void UHudMissionTrackerWidget::Advance(float DeltaSeconds)
{
	if (!Block)
	{
		return;
	}
	// Made before the world's missions were (or in a test): it listens as soon as they're there.
	if (!Missions.IsValid())
	{
		BindMissions();
	}
	UpdatePlacement();
	// A done objective or the closing line holds the tracker until it has shown its time; changes wait for it.
	if (bRefreshPending && (Phase == EPhase::Idle || Phase == EPhase::Showing))
	{
		Refresh();
	}
	// Time under a menu doesn't count, so the check or the closing line still shows once the menu closes.
	const bool bCovered = IsCovered();
	if ((Phase == EPhase::Finishing || Phase == EPhase::Closing) && !bCovered)
	{
		HoldLeft -= DeltaSeconds;
		if (HoldLeft <= 0.f)
		{
			EndHold();
		}
	}
	Animate(DeltaSeconds, Phase != EPhase::Idle && !bCovered);
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
				// The step is done (several at once, when the next ones were done already): tick it, then the next (the
				// turn-in, after the last).
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
		// The mission shown is over (gone from the list: turned in, or done): its last line ticks before whatever comes
		// next. One the player stopped tracking just makes way.
		if (!Subsystem || !Subsystem->FindMission(ShownMission))
		{
			BeginFinish(/*bStepDone*/ true, /*bMissionEnded*/ true);
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
	OptionalTickAges.Reset();
	PaintOptionals();
	PopAge = PopSeconds;
	ApplyPop(1.f);
	StartSlide();
}

TArray<int32> UHudMissionTrackerWidget::NewlyDoneOptionals(const FMissionTrackerParts& Shown, const FMissionTrackerParts& Now)
{
	// The same optional objectives (as many, the same words), one done that wasn't: never a whole new list.
	TArray<int32> Done;
	if (Now.Optionals.Num() != Shown.Optionals.Num())
	{
		return Done;
	}
	for (int32 Index = 0; Index < Now.Optionals.Num(); ++Index)
	{
		const FMissionTrackerOptional& Was = Shown.Optionals[Index];
		const FMissionTrackerOptional& Is = Now.Optionals[Index];
		if (Is.bDone && !Was.bDone && Is.Line.Equals(Was.Line, ESearchCase::CaseSensitive))
		{
			Done.Add(Index);
		}
	}
	return Done;
}

void UHudMissionTrackerWidget::UpdateInPlace(const FMission& Mission)
{
	const FMissionTrackerParts& Now = Mission.Tracker;
	// Optional objectives done since the last look tick where they stand; any other change to them is repainted.
	const TArray<int32> OptionalsDone = NewlyDoneOptionals(Shown, Now);
	bool bNewOptionals = Now.Optionals.Num() != Shown.Optionals.Num();
	for (int32 Index = 0; !bNewOptionals && Index < Now.Optionals.Num(); ++Index)
	{
		bNewOptionals = !Now.Optionals[Index].SameAs(Shown.Optionals[Index]);
	}
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
	if (bNewOptionals)
	{
		PaintOptionals();
		for (const int32 Index : OptionalsDone)
		{
			StartOptionalTick(Index);
		}
		if (!OptionalsDone.IsEmpty())
		{
			PlayOptionalSound();
		}
	}
}

void UHudMissionTrackerWidget::BeginFinish(bool bStepDone, bool bMissionEnded)
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
	// The check pops in over where the ring was.
	StartCheckPop();
	if (bCountLands && !Shown.bCountIsTime)
	{
		StartPop();
	}
	if (bStepDone)
	{
		PaintSteps(Shown.Step + 1, INDEX_NONE, Shown.StepCount, Shown.Step + 1, /*bPopChecks*/ true);
	}
	PlayFinishSound(bStepDone, bMissionEnded);
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
	PaintOptionals();
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
			BeginFinish(/*bStepDone*/ true, /*bMissionEnded*/ true);
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

// Where it stands and how it moves (IsCovered, the placement, the fade, the slide, the pop): HudMissionTrackerWidgetMotion.cpp.
// Painting what's shown (the title, the step circles, the objective, the hint): HudMissionTrackerWidgetPaint.cpp; the
// optional objectives under them: HudMissionTrackerWidgetOptional.cpp.
