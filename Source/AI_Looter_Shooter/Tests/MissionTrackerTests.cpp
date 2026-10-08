#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HUD/HudMissionTrackerWidget.h"

namespace
{
	/** The tracker's parts for the objective at (Step, ObjectiveIndex) with these words. */
	FMissionTrackerParts PartsAt(int32 Step, int32 ObjectiveIndex, const TCHAR* Line)
	{
		FMissionTrackerParts Parts;
		Parts.Step = Step;
		Parts.StepCount = 4;
		Parts.ObjectiveIndex = ObjectiveIndex;
		Parts.Line = Line;
		return Parts;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudMissionTrackerChangeTest, "Looter.UI.MissionTracker.Change",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudMissionTrackerChangeTest::RunTest(const FString& Parameters)
{
	// When the HUD's mission tracker ticks an objective: when its step or its place in the step moves on, never when only the
	// words (or the count, or the hint) change. Decided from the parts alone.
	using EChange = UHudMissionTrackerWidget::EChange;
	auto Decide = [](const FMissionTrackerParts& Shown, const FMissionTrackerParts& Now, bool bJustShown = false)
	{
		return UHudMissionTrackerWidget::DecideChange(Shown, Now, bJustShown);
	};
	const FMissionTrackerParts Shown = PartsAt(1, 0, TEXT("Press [E] to open the door"));

	// The same objective.
	TestTrue(TEXT("Nothing changed: nothing to do"), Decide(Shown, Shown) == EChange::InPlace);
	TestTrue(TEXT("A key rebound rewrites the words in place: no tick"),
		Decide(Shown, PartsAt(1, 0, TEXT("Press [F] to open the door"))) == EChange::InPlace);
	FMissionTrackerParts Counted = Shown;
	Counted.Count = TEXT("2 / 5");
	Counted.Progress = 2;
	Counted.Required = 5;
	TestTrue(TEXT("A count that rose: in place"), Decide(Shown, Counted) == EChange::InPlace);
	FMissionTrackerParts Hinted = Shown;
	Hinted.HintKey = TEXT("F");
	TestTrue(TEXT("A hint rebound: in place"), Decide(Shown, Hinted) == EChange::InPlace);
	TestTrue(TEXT("Words changed while it's still sliding in: still in place"),
		Decide(Shown, PartsAt(1, 0, TEXT("Press [F] to open the door")), /*bJustShown*/ true) == EChange::InPlace);

	// The objective done: the next of its step, or the next step (several at once too).
	TestTrue(TEXT("The step's next objective: the one shown is done"), Decide(Shown, PartsAt(1, 1, TEXT("Light the lamp"))) == EChange::ObjectiveDone);
	TestTrue(TEXT("Even with the same words, a later objective means the one shown is done"),
		Decide(Shown, PartsAt(1, 1, TEXT("Press [E] to open the door"))) == EChange::ObjectiveDone);
	TestTrue(TEXT("The next step: it's done with its step"), Decide(Shown, PartsAt(2, 0, TEXT("Go home"))) == EChange::StepDone);
	TestTrue(TEXT("Steps done at once: the same"), Decide(Shown, PartsAt(3, 1, TEXT("Go home"))) == EChange::StepDone);
	TestTrue(TEXT("The step moves on even with the words the same"), Decide(Shown, PartsAt(2, 0, *Shown.Line)) == EChange::StepDone);

	// Hardly seen yet, or started over: shown anew, with no tick.
	TestTrue(TEXT("Moved on while it slides in: no tick"), Decide(Shown, PartsAt(1, 1, TEXT("Light the lamp")), true) == EChange::Present);
	TestTrue(TEXT("A step moved on while it slides in: no tick"), Decide(Shown, PartsAt(2, 0, TEXT("Go home")), true) == EChange::Present);
	TestTrue(TEXT("An earlier step: started over"), Decide(Shown, PartsAt(0, 0, TEXT("Walk"))) == EChange::Present);
	TestTrue(TEXT("An earlier objective of the step: started over"),
		Decide(PartsAt(1, 1, TEXT("Light the lamp")), PartsAt(1, 0, TEXT("Ring the bell"))) == EChange::Present);

	// Nothing on show to tick: what comes is shown, but empty on both sides isn't shown again at every look.
	const FMissionTrackerParts Blank = PartsAt(1, 0, TEXT(""));
	TestTrue(TEXT("Words after an empty line: shown anew"), Decide(Blank, Shown) == EChange::Present);
	TestTrue(TEXT("Empty again, the same objective: nothing to show"), Decide(Blank, Blank) == EChange::InPlace);
	TestTrue(TEXT("An empty line is never a tick"), Decide(Shown, PartsAt(1, 0, TEXT(""))) == EChange::InPlace);
	return true;
}

#endif
