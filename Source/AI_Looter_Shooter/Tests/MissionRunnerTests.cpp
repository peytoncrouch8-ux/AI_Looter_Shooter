#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Progression/XPCurve.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

using namespace MissionTestWorld;

namespace
{
	/** The first objective's progress of a running mission ("1 / 2"), or "not running". */
	FString ProgressOf(const UMissionRunner& Runner, FName MissionId)
	{
		const TArray<FMissionObjectiveView> Views = Runner.GetObjectiveViews(MissionId);
		return Views.IsEmpty() ? FString(TEXT("not running")) : Views[0].Progress;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionObjectivesTest, "Looter.Missions.Objectives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionObjectivesTest::RunTest(const FString& Parameters)
{
	// A main mission of three steps, played by a test level's mission runner: kill two nest dummies (one placed before the
	// runner began, one spawned after), reach a spot, use two posters. Its progress, its steps, the display the minimap
	// reads, the campaign record and its rewards (once).
	// The record and the counters outlive the level, which the runner keeps them for.
	FCampaignRecord Campaign;
	int32 Finished = 0;
	int32 Rewarded = 0;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	if (!TestNotNull(TEXT("The level has a mission runner"), Runner) || !TestNotNull(TEXT("and the missions' display"), Display))
	{
		return false;
	}
	TestFalse(TEXT("A preview level plays nothing until told to"), Runner->IsActive());

	const FName HuntId(TEXT("TestHunt"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Hunt = NewMission(Scratch, TEXT("TestHunt"), EMissionKind::Main, EMissionStart::Automatic, TEXT("TestValley"));
	UMissionKillObjective* Kill = AddObjective<UMissionKillObjective>(Hunt, 0);
	Kill->Text = FText::FromString(TEXT("Clear the nest"));
	Kill->Target.ActorClass = ATargetDummy::StaticClass();
	Kill->Target.ActorTag = TEXT("Nest");
	Kill->Count = 2;
	UMissionReachObjective* Reach = AddObjective<UMissionReachObjective>(Hunt, 1);
	Reach->Place.Location = FVector(5000.0, 0.0, 0.0);
	Reach->Place.Radius = 300.f;
	UMissionInteractObjective* Posters = AddObjective<UMissionInteractObjective>(Hunt, 2);
	Posters->Target.ActorTag = TEXT("Poster");
	Posters->Count = 2;
	Hunt->Rewards.ExperienceShare = 0.3f;
	Hunt->Rewards.UnlockAreas = { FName(TEXT("TestPass")) };

	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	APlayerController* Shooter = World->SpawnActor<APlayerController>();
	ATargetDummy* Placed = SpawnDummy(World, FVector(1000.0, 0.0, 0.0), TEXT("Nest"));
	if (!TestTrue(TEXT("Stand-in, shooter and a dummy placed"), Player && Shooter && Placed))
	{
		return false;
	}

	Runner->OnMissionFinished.AddLambda([&Finished, &Rewarded](const UMissionDefinition&, bool bRewarded)
	{
		++Finished;
		Rewarded += bRewarded ? 1 : 0;
	});
	Runner->BeginForTesting({ Hunt }, Campaign, Player, TEXT("TestValley"));
	TestTrue(TEXT("Now it plays"), Runner->IsActive());
	TestTrue(TEXT("An actor with health placed before it began is watched"), Runner->IsWatching(Placed));
	TestFalse(TEXT("Nothing starts before its first update (the level is still being put back)"), Runner->IsRunning(HuntId));

	// Due: no prerequisites, in its area. It starts on its own, as the campaign's main mission, on the display.
	Runner->Update(0.f);
	TestTrue(TEXT("It started on its own"), Runner->IsRunning(HuntId) && Runner->GetStep(HuntId) == 0);
	TestTrue(TEXT("The campaign plays it as its main mission"), Campaign.ActiveMission == HuntId && Campaign.ActiveMissionStep == 0);
	TestTrue(TEXT("It's the tracked mission"), Runner->GetTrackedMission() == HuntId);
	if (const FMission* Shown = Display->GetTracked())
	{
		TestEqual(TEXT("The display has its title"), Shown->Title.ToString(), FString(TEXT("Test TestHunt")));
		TestEqual(TEXT("and its objective, counted"), Shown->Objective.ToString(), FString(TEXT("Clear the nest (0/2)")));
		TestTrue(TEXT("Its waypoint is the nest's dummy"), Shown->Waypoint.IsSet() && Shown->Waypoint->Equals(Placed->GetActorLocation(), 1.0));
	}
	else
	{
		AddError(TEXT("The running mission isn't on the display"));
	}

	// Kills: ones spawned after it began count too; a kill that isn't the player's, or of something else, doesn't.
	ATargetDummy* Spawned = SpawnDummy(World, FVector(1500.0, 300.0, 0.0), TEXT("Nest"));
	ATargetDummy* Stray = SpawnDummy(World, FVector(1500.0, -300.0, 0.0), TEXT("Nest"));
	ATargetDummy* Other = SpawnDummy(World, FVector(1200.0, 0.0, 0.0));
	if (!TestTrue(TEXT("More dummies spawned"), Spawned && Stray && Other))
	{
		return false;
	}
	TestTrue(TEXT("An actor spawned later is watched"), Runner->IsWatching(Spawned));
	Hurt(Stray, 1000.f, nullptr);
	Hurt(Other, 1000.f, Shooter);
	TestEqual(TEXT("Nobody's kill, and a kill of something else, don't count"), ProgressOf(*Runner, HuntId), FString(TEXT("0 / 2")));
	Hurt(Placed, 1000.f, Shooter);
	TestEqual(TEXT("The player's kill counts"), ProgressOf(*Runner, HuntId), FString(TEXT("1 / 2")));
	TestTrue(TEXT("Still the first step"), Runner->GetStep(HuntId) == 0);
	Hurt(Spawned, 1000.f, Shooter);
	TestTrue(TEXT("Two kills: the next step"), Runner->GetStep(HuntId) == 1);
	TestTrue(TEXT("The campaign keeps the step"), Campaign.ActiveMissionStep == 1);

	// Reach by location: the arrow points to the spot; standing within its radius (height aside) does it.
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("Far from the spot: not there yet"), Runner->GetStep(HuntId) == 1);
	const FMission* Shown = Display->GetTracked();
	TestTrue(TEXT("The arrow points to the spot"), Shown && Shown->Waypoint.IsSet() && Shown->Waypoint->Equals(FVector(5000.0, 0.0, 0.0), 1.0));
	Player->SetActorLocation(FVector(4800.0, 100.0, 900.0));
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("Standing there (a roof above it counts): the next step"), Runner->GetStep(HuntId) == 2);

	// Interact by event: each poster once, a held use too, other events not; an event with only a tag (the console's).
	AActor* PosterA = SpawnMarker(World, FVector(6000.0, 0.0, 0.0), TEXT("Poster"));
	Runner->NotifyEvent(FMissionEvent::Interaction(PosterA, /*bHeld*/ false));
	TestEqual(TEXT("A poster used"), ProgressOf(*Runner, HuntId), FString(TEXT("1 / 2")));
	Runner->NotifyEvent(FMissionEvent::Interaction(PosterA, /*bHeld*/ true));
	TestEqual(TEXT("The same poster again counts once"), ProgressOf(*Runner, HuntId), FString(TEXT("1 / 2")));
	Runner->NotifyEvent(FMissionEvent::Talked(PosterA));
	TestEqual(TEXT("Other events don't count"), ProgressOf(*Runner, HuntId), FString(TEXT("1 / 2")));
	TestEqual(TEXT("Not finished yet"), Finished, 0);
	Runner->NotifyEvent(FMissionEvent::Named(FMissionEvent::Interact, nullptr, TEXT("Poster")));

	// Finished: recorded, rewarded, off the display; the area it opens is open.
	TestFalse(TEXT("Finished: no longer running"), Runner->IsRunning(HuntId));
	TestTrue(TEXT("Finished once, rewarded"), Finished == 1 && Rewarded == 1);
	TestTrue(TEXT("The campaign has it finished"), Campaign.HasCompleted(HuntId));
	TestTrue(TEXT("It's no longer the campaign's mission"), Campaign.ActiveMission.IsNone() && Campaign.ActiveMissionStep == 0);
	TestTrue(TEXT("The area it opens is open"), Campaign.IsAreaOpen(TEXT("TestPass")));
	TestEqual(TEXT("Gone from the display"), Display->GetMissions().Num(), 0);
	TestTrue(TEXT("Its status"), Runner->GetStatus(*Hunt) == EMissionStatus::Completed);

	// Rewards once: finished again from the console, or played again, it gives nothing more.
	TestTrue(TEXT("Finished again from the console"), Runner->CompleteMission(HuntId));
	TestTrue(TEXT("No second reward"), Finished == 2 && Rewarded == 1);
	TestFalse(TEXT("An automatic mission finished doesn't start again"), Runner->IsRunning(HuntId));
	TestTrue(TEXT("Played again"), Runner->StartMission(HuntId, 0, /*bForce*/ true));
	TestTrue(TEXT("Active while it runs again"), Runner->GetStatus(*Hunt) == EMissionStatus::Active);
	TestTrue(TEXT("A step finished from the console"), Runner->CompleteStep(HuntId));
	TestTrue(TEXT("The reach step passes at once: the player stands there"), Runner->GetStep(HuntId) == 2);
	TestTrue(TEXT("Finished again"), Runner->CompleteMission(HuntId) && !Runner->IsRunning(HuntId));
	TestTrue(TEXT("Still rewarded once"), Finished == 3 && Rewarded == 1);
	TestEqual(TEXT("The area opened once"), Campaign.OpenedAreas.Num(), 1);
	TestEqual(TEXT("Finished once in the record"), Campaign.CompletedMissions.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionPrerequisitesTest, "Looter.Missions.Prerequisites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionPrerequisitesTest::RunTest(const FString& Parameters)
{
	// What starts a mission: its prerequisites finished, its area's level, an event for one that waits for it, code for a
	// manual one. Side missions stay out of the campaign's main mission. Tracking moves between running missions.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	if (!TestNotNull(TEXT("The level has a mission runner"), Runner))
	{
		return false;
	}
	const FName Valley(TEXT("TestValley"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Bell = NewMission(Scratch, TEXT("TestBell"), EMissionKind::Side, EMissionStart::Automatic, Valley);
	AddObjective<UMissionEventObjective>(Bell, 0)->Event = TEXT("Bell.Rung");
	UMissionDefinition* Toll = NewMission(Scratch, TEXT("TestToll"), EMissionKind::Side, EMissionStart::Automatic, Valley);
	Toll->Prerequisites = { FName(TEXT("TestBell")) };
	AddObjective<UMissionEventObjective>(Toll, 0)->Event = TEXT("Toll.Paid");
	UMissionDefinition* Notice = NewMission(Scratch, TEXT("TestNotice"), EMissionKind::Side, EMissionStart::OnEvent, Valley);
	Notice->StartEvent = TEXT("Board.Read");
	Notice->Prerequisites = { FName(TEXT("TestBell")) };
	AddObjective<UMissionTalkObjective>(Notice, 0)->SpeakerTag = TEXT("Speaker_Sexton");
	UMissionDefinition* Far = NewMission(Scratch, TEXT("TestFar"), EMissionKind::Side, EMissionStart::Automatic, TEXT("TestElsewhere"));
	AddObjective<UMissionEventObjective>(Far, 0)->Event = TEXT("Far.Away");
	UMissionDefinition* ByHand = NewMission(Scratch, TEXT("TestByHand"), EMissionKind::Side, EMissionStart::Manual, NAME_None);
	AddObjective<UMissionEventObjective>(ByHand, 0)->Event = TEXT("Hand.Done");

	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	Runner->BeginForTesting({ Bell, Toll, Notice, Far, ByHand }, Campaign, Player, Valley);
	Runner->Update(0.f);
	TestTrue(TEXT("A mission without prerequisites starts in its area"), Runner->IsRunning(TEXT("TestBell")));
	TestFalse(TEXT("One waiting on it doesn't"), Runner->IsRunning(TEXT("TestToll")));
	TestFalse(TEXT("Nor can code start it before its prerequisites"), Runner->StartMission(TEXT("TestToll")));
	TestTrue(TEXT("It's locked"), Runner->GetStatus(*Toll) == EMissionStatus::Locked);
	TestTrue(TEXT("So is the notice"), Runner->GetStatus(*Notice) == EMissionStatus::Locked);
	TestFalse(TEXT("Another area's mission doesn't start here"), Runner->IsRunning(TEXT("TestFar")));
	TestTrue(TEXT("It's available, in its own area"), Runner->GetStatus(*Far) == EMissionStatus::Available);
	TestTrue(TEXT("A manual mission waits for code, unlisted"), !Runner->IsRunning(TEXT("TestByHand")) && Runner->GetStatus(*ByHand) == EMissionStatus::Locked);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Board.Read")));
	TestFalse(TEXT("Its start event before its prerequisites does nothing"), Runner->IsRunning(TEXT("TestNotice")));
	TestTrue(TEXT("Side missions stay out of the campaign's main mission"), Campaign.ActiveMission.IsNone());

	// The first finished: the one waiting on it starts by itself, and the notice waits for its event.
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Bell.Rung")));
	TestTrue(TEXT("Its event finished the first"), Campaign.HasCompleted(TEXT("TestBell")) && !Runner->IsRunning(TEXT("TestBell")));
	TestTrue(TEXT("The one waiting on it started by itself"), Runner->IsRunning(TEXT("TestToll")));
	TestTrue(TEXT("The notice is available, waiting for its event"),
		!Runner->IsRunning(TEXT("TestNotice")) && Runner->GetStatus(*Notice) == EMissionStatus::Available);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Board.Read")));
	TestTrue(TEXT("Its start event starts it"), Runner->IsRunning(TEXT("TestNotice")));
	TestTrue(TEXT("A manual mission starts from code"), Runner->StartMission(TEXT("TestByHand")) && Runner->IsRunning(TEXT("TestByHand")));

	// Talking at a speaker point: an event about the actor carrying its tag.
	AActor* Lookout = SpawnMarker(World, FVector(0.0, 500.0, 0.0), TEXT("Speaker_Sexton"));
	AActor* Elsewhere = SpawnMarker(World, FVector(0.0, -500.0, 0.0), TEXT("Speaker_Delia"));
	Runner->NotifyEvent(FMissionEvent::Talked(Elsewhere));
	TestTrue(TEXT("Talking to someone else doesn't do it"), Runner->IsRunning(TEXT("TestNotice")));
	Runner->NotifyEvent(FMissionEvent::Talked(Lookout));
	TestTrue(TEXT("Talking to the speaker does"), Campaign.HasCompleted(TEXT("TestNotice")));

	// Tracking moves between running missions, or to none.
	Runner->TrackMission(TEXT("TestToll"));
	TestTrue(TEXT("Track one"), Runner->GetTrackedMission() == FName(TEXT("TestToll")));
	Runner->TrackMission(TEXT("TestByHand"));
	TestTrue(TEXT("Track another"), Runner->GetTrackedMission() == FName(TEXT("TestByHand")));
	Runner->TrackMission(NAME_None);
	TestTrue(TEXT("Track none"), Runner->GetTrackedMission().IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTrackerPartsTest, "Looter.Missions.TrackerParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTrackerPartsTest::RunTest(const FString& Parameters)
{
	// What the runner gives the HUD's mission tracker, step by step: the short line (else the full words, keys resolved),
	// the count only when it counts more than one and shows it, the step and the steps, the key hint (no player in a test
	// level: the binding's name), and whether it's done in the inventory. The full line stays for the mission list.
	// The record and the counter outlive the level.
	FCampaignRecord Campaign;
	int32 Changes = 0;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	if (!TestTrue(TEXT("The level has a mission runner and the missions' display"), Runner && Display))
	{
		return false;
	}
	const FName RoundsId(TEXT("TestRounds"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Rounds = NewMission(Scratch, TEXT("TestRounds"), EMissionKind::Side, EMissionStart::Automatic, TEXT("TestValley"));
	UMissionKillObjective* Nest = AddObjective<UMissionKillObjective>(Rounds, 0);
	Nest->Text = FText::FromString(TEXT("Clear the nest under the old mill"));
	Nest->ShortText = FText::FromString(TEXT("Clear the nest"));
	Nest->HintAction = TEXT("Reload");
	Nest->HintText = FText::FromString(TEXT("Reload"));
	Nest->Target.ActorClass = ATargetDummy::StaticClass();
	Nest->Target.ActorTag = TEXT("Nest");
	Nest->Count = 2;
	UMissionInteractObjective* Posters = AddObjective<UMissionInteractObjective>(Rounds, 1);
	Posters->Text = FText::FromString(TEXT("Tear down the posters"));
	Posters->Target.ActorTag = TEXT("Poster");
	Posters->Count = 3;
	Posters->bShowCount = false;
	UMissionReachObjective* Spot = AddObjective<UMissionReachObjective>(Rounds, 2);
	Spot->Text = FText::FromString(TEXT("Press {Interact} at the spot"));
	Spot->Place.Location = FVector(5000.0, 0.0, 0.0);
	Spot->Place.Radius = 300.f;
	UMissionOpenPageObjective* OpenBook = AddObjective<UMissionOpenPageObjective>(Rounds, 3);
	OpenBook->Text = FText::FromString(TEXT("Open the Ledger"));
	OpenBook->HintAction = TEXT("Move");
	OpenBook->HintText = FText::FromString(TEXT("Walk"));

	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	APlayerController* Shooter = World->SpawnActor<APlayerController>();
	ATargetDummy* Dummy = SpawnDummy(World, FVector(1000.0, 0.0, 0.0), TEXT("Nest"));
	if (!TestTrue(TEXT("Stand-in, shooter and a nest dummy"), Player && Shooter && Dummy))
	{
		return false;
	}
	Display->OnMissionsChanged.AddLambda([&Changes]() { ++Changes; });
	Runner->BeginForTesting({ Rounds }, Campaign, Player, TEXT("TestValley"));
	Runner->Update(0.f);
	auto Parts = [Display]() { const FMission* Shown = Display->GetTracked(); return Shown ? Shown->Tracker : FMissionTrackerParts(); };

	// Step 1: the short line, a count of two, the hint's key by its binding's name.
	FMissionTrackerParts Now = Parts();
	TestEqual(TEXT("The short line"), Now.Line, FString(TEXT("Clear the nest")));
	TestTrue(TEXT("Counted: 0 / 2, 2 / 2 once done"), Now.Count == TEXT("0 / 2") && Now.CountDone == TEXT("2 / 2") && Now.Progress == 0 && Now.Required == 2);
	TestTrue(TEXT("Step 1 of 4"), Now.Step == 0 && Now.StepCount == 4);
	TestTrue(TEXT("The hint: the key and its words"), Now.HintKey == TEXT("Reload") && Now.HintText == TEXT("Reload"));
	TestFalse(TEXT("Not done in the inventory"), Now.bOverInventory);
	const FMission* Shown = Display->GetTracked();
	TestEqual(TEXT("The mission list keeps the full words, counted"), Shown ? Shown->Objective.ToString() : FString(),
		FString(TEXT("Clear the nest under the old mill (0/2)")));
	const int32 ChangesBefore = Changes;
	Hurt(Dummy, 1000.f, Shooter);
	Now = Parts();
	TestTrue(TEXT("A kill: 1 / 2"), Now.Count == TEXT("1 / 2") && Now.Progress == 1);
	TestTrue(TEXT("and the tracker hears of it"), Changes > ChangesBefore);

	// Step 2: no short line (its words), a count it hides, no hint.
	TestTrue(TEXT("On to the posters"), Runner->CompleteStep(RoundsId) && Runner->GetStep(RoundsId) == 1);
	Now = Parts();
	TestEqual(TEXT("No short line: its words"), Now.Line, FString(TEXT("Tear down the posters")));
	TestTrue(TEXT("A hidden count isn't shown"), Now.Count.IsEmpty() && Now.CountDone.IsEmpty() && Now.Required == 0);
	TestTrue(TEXT("No hint"), Now.HintKey.IsEmpty() && Now.HintText.IsEmpty());
	TestTrue(TEXT("Step 2 of 4"), Now.Step == 1 && Now.StepCount == 4);

	// Step 3: one thing to do shows no count; its words' key resolved.
	TestTrue(TEXT("On to the spot"), Runner->CompleteStep(RoundsId) && Runner->GetStep(RoundsId) == 2);
	Now = Parts();
	TestEqual(TEXT("Its words, the key resolved"), Now.Line, FString(TEXT("Press [Interact] at the spot")));
	TestTrue(TEXT("A single thing: no count"), Now.Count.IsEmpty());

	// Step 4: done in the inventory, so the tracker stays over it; Move's keys together.
	TestTrue(TEXT("On to the Ledger"), Runner->CompleteStep(RoundsId) && Runner->GetStep(RoundsId) == 3);
	Now = Parts();
	TestTrue(TEXT("Done in the inventory"), Now.bOverInventory);
	TestEqual(TEXT("The movement keys on one keycap"), Now.HintKey, FString(TEXT("MoveForward MoveLeft MoveBackward MoveRight")));
	TestTrue(TEXT("Step 4 of 4"), Now.Step == 3 && Now.StepCount == 4);
	TestTrue(TEXT("Finished"), Runner->CompleteStep(RoundsId) && !Runner->IsRunning(RoundsId));
	TestTrue(TEXT("Off the tracker"), Display->GetTracked() == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTrackerObjectiveTest, "Looter.Missions.TrackerObjective",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTrackerObjectiveTest::RunTest(const FString& Parameters)
{
	// Which objective of its step the tracker's line is: it moves on only as the one before it is done, which is how the HUD
	// tracker knows one is done (the words alone change with a rebound key). A count of seconds says so, so the tracker
	// doesn't pop it every second. The record outlives the level.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	if (!TestTrue(TEXT("The level has a mission runner and the missions' display"), Runner && Display))
	{
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Chores = NewMission(Scratch, TEXT("TestChores"), EMissionKind::Side, EMissionStart::Automatic, TEXT("TestValley"));
	UMissionEventObjective* Bell = AddObjective<UMissionEventObjective>(Chores, 0);
	Bell->Text = FText::FromString(TEXT("Ring the bell"));
	Bell->Event = TEXT("Chores.Bell");
	UMissionEventObjective* Lamp = AddObjective<UMissionEventObjective>(Chores, 0);
	Lamp->Text = FText::FromString(TEXT("Light the lamp"));
	Lamp->Event = TEXT("Chores.Lamp");
	UMissionEventObjective* Home = AddObjective<UMissionEventObjective>(Chores, 1);
	Home->Text = FText::FromString(TEXT("Go home"));
	Home->Event = TEXT("Chores.Home");

	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("A stand-in for the player"), Player))
	{
		return false;
	}
	Runner->BeginForTesting({ Chores }, Campaign, Player, TEXT("TestValley"));
	Runner->Update(0.f);
	auto Parts = [Display]() { const FMission* Shown = Display->GetTracked(); return Shown ? Shown->Tracker : FMissionTrackerParts(); };

	FMissionTrackerParts Now = Parts();
	TestTrue(TEXT("The first objective of the first step"), Now.Line == TEXT("Ring the bell") && Now.Step == 0 && Now.ObjectiveIndex == 0);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Chores.Bell")));
	Now = Parts();
	TestTrue(TEXT("The bell rung: the second objective of the same step"), Now.Line == TEXT("Light the lamp") && Now.Step == 0 && Now.ObjectiveIndex == 1);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Chores.Lamp")));
	Now = Parts();
	TestTrue(TEXT("The lamp lit: the next step's first objective"), Now.Line == TEXT("Go home") && Now.Step == 1 && Now.ObjectiveIndex == 0);

	// Holding out counts seconds; a kill count counts things.
	UMissionDefendObjective* Hold = NewObject<UMissionDefendObjective>(GetTransientPackage());
	Hold->HoldSeconds = 30.f;
	FMissionTrackerParts Timed;
	Hold->FillTrackerParts(nullptr, FMissionObjectiveState(), Timed);
	TestTrue(TEXT("Holding out counts seconds"), Timed.bCountIsTime && Timed.Count == TEXT("0 s / 30 s") && Timed.Required == 30);
	UMissionKillObjective* Killing = NewObject<UMissionKillObjective>(GetTransientPackage());
	Killing->Count = 4;
	FMissionTrackerParts Counted;
	Killing->FillTrackerParts(nullptr, FMissionObjectiveState(), Counted);
	TestTrue(TEXT("Kills count things"), !Counted.bCountIsTime && Counted.Count == TEXT("0 / 4") && Counted.Required == 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionRewardsTest, "Looter.Missions.Rewards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionRewardsTest::RunTest(const FString& Parameters)
{
	// A share of a level's experience is worth the same part of a level at any level; a gun's rarity floor raises only.
	FXPCurve Curve;
	TestEqual(TEXT("30% of the first level"), MissionRewards::ExperienceFor(0.3f, 1, Curve), static_cast<int64>(30));
	TestEqual(TEXT("30% of the second level (112)"), MissionRewards::ExperienceFor(0.3f, 2, Curve), static_cast<int64>(34));
	TestEqual(TEXT("20% of level 10"), MissionRewards::ExperienceFor(0.2f, 10, Curve),
		static_cast<int64>(FMath::RoundToDouble(static_cast<double>(Curve.XPToNextLevel(10)) * static_cast<double>(0.2f))));
	TestEqual(TEXT("No share, no experience"), MissionRewards::ExperienceFor(0.f, 5, Curve), static_cast<int64>(0));
	TestEqual(TEXT("Nothing at the maximum level"), MissionRewards::ExperienceFor(0.3f, Curve.MaxLevel, Curve), static_cast<int64>(0));
	TestTrue(TEXT("A floor raises a lower rarity"), MissionRewards::ApplyFloor(EWeaponRarity::Common, EWeaponRarity::Rare) == EWeaponRarity::Rare);
	TestTrue(TEXT("and leaves a higher one"), MissionRewards::ApplyFloor(EWeaponRarity::Epic, EWeaponRarity::Rare) == EWeaponRarity::Epic);

	// In words, for the Missions page.
	FMissionRewards Rewards;
	TestTrue(TEXT("Nothing to give"), Rewards.IsEmpty() && MissionRewards::Describe(Rewards).IsEmpty());
	Rewards.ExperienceShare = 0.3f;
	Rewards.bGun = true;
	Rewards.GunRarityFloor = EWeaponRarity::Uncommon;
	Rewards.NamedGun = TEXT("Heirloom");
	Rewards.UnlockAreas = { FName(TEXT("SomewhereNew")) };
	const TArray<FString> Lines = MissionRewards::Describe(Rewards);
	if (TestEqual(TEXT("A line each"), Lines.Num(), 4))
	{
		TestEqual(TEXT("Experience"), Lines[0], FString(TEXT("+30% of a level's experience")));
		TestEqual(TEXT("A gun"), Lines[1], FString(TEXT("Gun: Uncommon or better")));
		TestEqual(TEXT("A named gun"), Lines[2], FString(TEXT("Named gun: Heirloom")));
		TestEqual(TEXT("An area, by its id when it has no asset"), Lines[3], FString(TEXT("Opens SomewhereNew")));
	}
	return true;
}

#endif
