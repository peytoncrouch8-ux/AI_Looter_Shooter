#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/EconomyRules.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "UI/HUD/HudMissionTrackerWidget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Optional objectives (wave 2a): they never hold a step, stay doable until it ends (the last step's until the turn-in), are
// kept done by the campaign so a step played again starts with them done, show under the tracker's line, and each pays its
// mission's bonus at the turn-in, never experience.

using namespace MissionTestWorld;

namespace
{
	const FName WrenTag(TEXT("Speaker_Wren"));
	const FName Asked(TEXT("Test.Asked"));
	const FName ExtraStep(TEXT("Test.ExtraStep"));
	const FName Last(TEXT("Test.Last"));
	const FName LastExtra(TEXT("Test.LastExtra"));

	UMissionEventObjective* AddEvent(UMissionDefinition* Mission, int32 Step, FName Event, bool bOptional)
	{
		UMissionEventObjective* Objective = AddObjective<UMissionEventObjective>(Mission, Step);
		Objective->Event = Event;
		Objective->bOptional = bOptional;
		Objective->Text = FText::FromString(Event.ToString());
		return Objective;
	}

	/** Step 1: Asked, and ExtraStep optional; step 2: Last, and LastExtra optional. Turned in to Wren for 25 XP, 10 silver each optional. */
	UMissionDefinition* MakeErrand(UObject* Outer, const TCHAR* Id, EMissionKind Kind = EMissionKind::Side)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, Kind, EMissionStart::Automatic, NAME_None);
		AddEvent(Mission, 0, Asked, false);
		AddEvent(Mission, 0, ExtraStep, true);
		AddEvent(Mission, 1, Last, false);
		AddEvent(Mission, 1, LastExtra, true);
		Mission->TurnIn.SpeakerTag = WrenTag;
		Mission->Rewards.Experience = 25;
		Mission->Rewards.OptionalSilver = 10;
		return Mission;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionOptionalNeverHoldsTest, "Looter.Missions.Optional.NeverHolds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionOptionalNeverHoldsTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(500.0, 0.0, 0.0), WrenTag);
	if (!Runner || !Display || !Player || !Wren)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	const FName Id(TEXT("TestExtras"));
	UMissionDefinition* Errand = MakeErrand(CreatePackage(nullptr), TEXT("TestExtras"));
	TestEqual(TEXT("It counts its optional objectives"), Errand->CountOptional(), 2);
	FMissionRewardsGiven Given;
	Runner->OnMissionCompleted.AddLambda([&Given](const UMissionDefinition&, const FMissionRewardsGiven& Heard) { Given = Heard; });
	Runner->BeginForTesting({ Errand }, Campaign, Player);
	Runner->Update(0.f);
	TestEqual(TEXT("It starts at its first step"), Runner->GetStep(Id), 0);

	// The tracker shows what the step asks for, the optional one under it.
	const FMission* Shown = Display->GetTracked();
	TestTrue(TEXT("The tracker's line is the objective asked for"), Shown && Shown->Tracker.Line == Asked.ToString());
	TestTrue(TEXT("...the optional one under it, not done"), Shown && Shown->Tracker.Optionals.Num() == 1
		&& Shown->Tracker.Optionals[0].Line == ExtraStep.ToString() && !Shown->Tracker.Optionals[0].bDone);
	const TArray<FMissionObjectiveView> Views = Runner->GetObjectiveViews(Id);
	TestTrue(TEXT("The Missions page sees which is optional"), Views.Num() == 2 && !Views[0].bOptional && Views[1].bOptional);

	// Done first, the optional one doesn't move the step; it's kept by the campaign.
	Runner->NotifyEvent(FMissionEvent::Named(ExtraStep));
	TestEqual(TEXT("An optional objective done: still the first step"), Runner->GetStep(Id), 0);
	TestTrue(TEXT("...kept by the campaign"), Campaign.HasDoneOptional(Id, 0, 1) && Runner->CountOptionalDone(Id) == 1);
	Shown = Display->GetTracked();
	TestTrue(TEXT("...and ticked on the tracker, the line unchanged"), Shown && Shown->Tracker.Optionals.Num() == 1
		&& Shown->Tracker.Optionals[0].bDone && Shown->Tracker.Line == Asked.ToString());

	// What's asked for moves it on; the second step's optional is skipped and the mission waits for its turn-in.
	Runner->NotifyEvent(FMissionEvent::Named(Asked));
	TestEqual(TEXT("What the step asked for done: the next step"), Runner->GetStep(Id), 1);
	Runner->NotifyEvent(FMissionEvent::Named(Last));
	TestTrue(TEXT("The last step's asked-for objective done: ready, its optional one not done"), Runner->IsReadyToTurnIn(Id)
		&& !Campaign.HasDoneOptional(Id, 1, 1));

	// The last step's optional one stays doable until the turn-in.
	const TArray<FMissionObjectiveView> Open = Runner->GetOptionalViews(Id);
	TestTrue(TEXT("While it waits, the last step's optional objective is still there"), Open.Num() == 1 && !Open[0].bDone);
	Shown = Display->GetTracked();
	TestTrue(TEXT("...on the tracker under the turn-in"), Shown && Shown->Tracker.bTurnIn && Shown->Tracker.Optionals.Num() == 1);
	Runner->NotifyEvent(FMissionEvent::Named(LastExtra));
	TestTrue(TEXT("Done before the turn-in: it counts"), Campaign.HasDoneOptional(Id, 1, 1) && Runner->CountOptionalDone(Id) == 2);
	TestTrue(TEXT("...and the mission still waits for its giver"), Runner->IsReadyToTurnIn(Id) && !Campaign.HasCompleted(Id));

	// The turn-in pays the bonus (two at level 1: 20 silver), no extra experience, and forgets the records.
	Runner->NotifyEvent(FMissionEvent::Talked(Wren));
	TestTrue(TEXT("Turned in"), Campaign.HasCompleted(Id));
	TestEqual(TEXT("Two optional objectives done"), Given.OptionalDone, 2);
	TestEqual(TEXT("...paid 10 silver each at level 1"), Given.OptionalSilver, 20);
	TestEqual(TEXT("...in the silver given"), Given.Silver, 20);
	TestEqual(TEXT("The experience is only the mission's"), static_cast<int32>(Given.Experience), 25);
	TestTrue(TEXT("The campaign forgets them once paid"), Campaign.OptionalDone.IsEmpty() && Runner->CountOptionalDone(Id) == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionOptionalSavedTest, "Looter.Missions.Optional.Saved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionOptionalSavedTest::RunTest(const FString& Parameters)
{
	// A side mission starts again from its first step in the next level or session; an optional objective done before
	// starts done, and isn't counted twice.
	FCampaignRecord Campaign;
	const FName Id(TEXT("TestKept"));
	Campaign.MarkOptionalDone(Id, 0, 1);
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	if (!Runner || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UMissionDefinition* Errand = MakeErrand(CreatePackage(nullptr), TEXT("TestKept"));
	Runner->BeginForTesting({ Errand }, Campaign, Player);
	Runner->Update(0.f);
	const TArray<FMissionObjectiveView> Views = Runner->GetObjectiveViews(Id);
	TestTrue(TEXT("Played again, the optional one done before starts done"), Views.Num() == 2 && Views[1].bOptional && Views[1].bDone);
	TestTrue(TEXT("...and what's asked for doesn't"), Views.Num() == 2 && !Views[0].bDone);
	Runner->NotifyEvent(FMissionEvent::Named(ExtraStep));
	TestEqual(TEXT("Done again, it's still one"), Runner->CountOptionalDone(Id), 1);

	// The console's CompleteStep finishes what a step asks for, never its optional objectives.
	Runner->NotifyEvent(FMissionEvent::Named(Asked));
	Runner->CompleteStep(Id);
	TestTrue(TEXT("CompleteStep: ready to turn in"), Runner->IsReadyToTurnIn(Id));
	TestFalse(TEXT("...without the last step's optional one done"), Campaign.HasDoneOptional(Id, 1, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionOptionalRulesTest, "Looter.Missions.Optional.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionOptionalRulesTest::RunTest(const FString& Parameters)
{
	// The campaign's keys: one mission's never counted as another's whose id starts the same.
	FCampaignRecord Campaign;
	TestEqual(TEXT("A key reads mission.step.objective"), FCampaignRecord::OptionalKey(TEXT("Main1"), 2, 1), FName(TEXT("Main1.2.1")));
	Campaign.MarkOptionalDone(TEXT("Main1"), 0, 0);
	Campaign.MarkOptionalDone(TEXT("Main10"), 0, 0);
	Campaign.MarkOptionalDone(TEXT("Main10"), 1, 2);
	TestEqual(TEXT("Main1 has one"), Campaign.CountOptionalDone(TEXT("Main1")), 1);
	TestEqual(TEXT("Main10 has two"), Campaign.CountOptionalDone(TEXT("Main10")), 2);
	TestFalse(TEXT("Marked twice: once"), Campaign.MarkOptionalDone(TEXT("Main1"), 0, 0));
	TestEqual(TEXT("Clearing Main1 leaves Main10's"), Campaign.ClearOptionals(TEXT("Main1")), 1);
	TestEqual(TEXT("...two left"), Campaign.OptionalDone.Num(), 2);
	Campaign.Complete(TEXT("Main10"));
	TestFalse(TEXT("A finished mission's aren't recorded any more"), Campaign.MarkOptionalDone(TEXT("Main10"), 2, 0));

	// The bonus grows with the player's level as the economy does, and nothing for none done.
	FMissionRewards Rewards;
	Rewards.OptionalSilver = 10;
	TestEqual(TEXT("Two at level 1"), MissionRewards::OptionalBonus(Rewards, 2, 1), 20);
	TestEqual(TEXT("One at level 7"), MissionRewards::OptionalBonus(Rewards, 1, 7), LooterEconomy::AtLevel(10, 7));
	TestEqual(TEXT("None done"), MissionRewards::OptionalBonus(Rewards, 0, 5), 0);

	// The tracker ticks an optional objective done in place: the same list with one turned done, never a new list.
	FMissionTrackerParts Before;
	Before.Optionals.Add({ FString(TEXT("Read the roll")), FString(), false });
	Before.Optionals.Add({ FString(TEXT("Look at the hat")), FString(), false });
	FMissionTrackerParts After = Before;
	After.Optionals[1].bDone = true;
	TestFalse(TEXT("A change to an optional objective is a change to the parts"), Before.SameAs(After));
	TestEqual(TEXT("...one optional done: the tracker's line doesn't move"), UHudMissionTrackerWidget::DecideChange(Before, After, false),
		UHudMissionTrackerWidget::EChange::InPlace);
	const TArray<int32> Done = UHudMissionTrackerWidget::NewlyDoneOptionals(Before, After);
	TestTrue(TEXT("...and it's the one that ticks"), Done.Num() == 1 && Done[0] == 1);
	FMissionTrackerParts Other = After;
	Other.Optionals[1].Line = TEXT("Something else");
	TestTrue(TEXT("A different line is a new list, not a tick"), UHudMissionTrackerWidget::NewlyDoneOptionals(Before, Other).IsEmpty());
	return true;
}

#endif
