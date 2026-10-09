#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Progression/XPCurve.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLine.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "UI/HUD/HudMissionTrackerWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Missions turned in, Borderlands' way: a mission whose objectives are all done is ready to turn in (not finished), the
// tracker says who to and points at them, and only a talk with that giver finishes it, with its rewards. The state survives
// a save; an automatic mission still finishes by itself; nothing gives experience along the way.

using namespace MissionTestWorld;

namespace
{
	const FName GiverTag(TEXT("Speaker_Wren"));
	const FName OtherTag(TEXT("Speaker_Stranger"));
	const FName FirstEvent(TEXT("Test.First"));
	const FName SecondEvent(TEXT("Test.Second"));

	/** A mission of two steps (an event each), turned in to Wren for 25 experience, played anywhere. */
	UMissionDefinition* MakeErrand(UObject* Outer, const TCHAR* Id, EMissionKind Kind = EMissionKind::Main)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, Kind, EMissionStart::Automatic, NAME_None);
		AddObjective<UMissionEventObjective>(Mission, 0)->Event = FirstEvent;
		AddObjective<UMissionEventObjective>(Mission, 1)->Event = SecondEvent;
		Mission->TurnIn.SpeakerTag = GiverTag;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("Wren"));
		Mission->Rewards.Experience = 25;
		return Mission;
	}

	/** What a runner's missions gave as they finished, as the HUD's banner hears it. */
	struct FHeard
	{
		int32 Finished = 0;
		int32 Rewarded = 0;
		FMissionRewardsGiven Last;
	};

	void Listen(UMissionRunner& Runner, FHeard& Heard)
	{
		Runner.OnMissionFinished.AddLambda([&Heard](const UMissionDefinition&, bool bRewarded)
		{
			++Heard.Finished;
			Heard.Rewarded += bRewarded ? 1 : 0;
		});
		Runner.OnMissionCompleted.AddLambda([&Heard](const UMissionDefinition&, const FMissionRewardsGiven& Given) { Heard.Last = Given; });
	}

	/** A speaker point as the build scripts place one: its point at the actor's spot, tagged Tag, saying Words. */
	ASpeakerPoint* PlaceSpeaker(UWorld* World, const FVector& Where, FName Tag, const TCHAR* Name, const TCHAR* Words)
	{
		ASpeakerPoint* Point = World->SpawnActor<ASpeakerPoint>(Where, FRotator(0.0, 180.0, 0.0));
		if (!Point)
		{
			return nullptr;
		}
		Point->SpeakerPoint->SetRelativeLocation(FVector::ZeroVector);
		Point->SpeakerPoint->SpeakerName = FText::FromString(Name);
		Point->SpeakerPoint->Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(Words), 3.f) };
		Point->Tags.Add(Tag);
		Point->DispatchBeginPlay();
		return Point;
	}

	FString OnScreen(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT("|") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInReadyTest, "Looter.Missions.TurnIn.Ready",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInReadyTest::RunTest(const FString& Parameters)
{
	// After its last objective a mission with a giver isn't finished: it's ready to turn in, one step past its last, kept
	// in the campaign, and the tracker says "Turn in to Wren" with the arrow on her.
	FCampaignRecord Campaign;
	FHeard Heard;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(2000.0, 500.0, 0.0), GiverTag);
	if (!Runner || !Display || !Player || !Wren)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	const FName Id(TEXT("TestErrand"));
	UMissionDefinition* Errand = MakeErrand(CreatePackage(nullptr), TEXT("TestErrand"));
	TestTrue(TEXT("A mission with a giver waits for its turn-in"), Errand->NeedsTurnIn());
	Runner->BeginForTesting({ Errand }, Campaign, Player);
	Listen(*Runner, Heard);
	Runner->Update(0.f);
	TestEqual(TEXT("It starts at its first step"), Runner->GetStep(Id), 0);

	Runner->NotifyEvent(FMissionEvent::Named(FirstEvent));
	TestTrue(TEXT("The first done: on to the second, not ready"), Runner->GetStep(Id) == 1 && !Runner->IsReadyToTurnIn(Id));
	const FMission* Before = Display->GetTracked();
	const FMissionTrackerParts LastObjective = Before ? Before->Tracker : FMissionTrackerParts();

	Runner->NotifyEvent(FMissionEvent::Named(SecondEvent));
	TestTrue(TEXT("Every objective done: ready to turn in, still running"), Runner->IsReadyToTurnIn(Id) && Runner->IsRunning(Id));
	TestTrue(TEXT("...not finished, nothing given"), !Runner->IsCompleted(Id) && !Campaign.HasCompleted(Id) && Heard.Finished == 0);
	TestEqual(TEXT("...its step one past its last"), Runner->GetStep(Id), 2);
	TestTrue(TEXT("...kept in the campaign: waiting, and the main mission at that step"), Campaign.IsReadyToTurnIn(Id)
		&& Campaign.ActiveMission == Id && Campaign.ActiveMissionStep == 2);
	TestTrue(TEXT("...under way on the Missions page"), Runner->GetStatus(*Errand) == EMissionStatus::Active);
	TestTrue(TEXT("...with no objectives left"), Runner->GetObjectiveViews(Id).IsEmpty());
	TestTrue(TEXT("Wren is who it's turned in to"), Runner->FindTurnInAt(Wren) == Errand && !Runner->FindTurnInAt(Player));
	TestEqual(TEXT("The Missions page's line"), Errand->GetTurnInText(), FString(TEXT("Ready to turn in: talk to Wren")));

	const FMission* Shown = Display->GetTracked();
	if (TestNotNull(TEXT("The tracked mission"), Shown))
	{
		TestTrue(TEXT("The tracker: \"Turn in to Wren\", every step done"), Shown->Tracker.bTurnIn && Shown->Tracker.Line == TEXT("Turn in to Wren")
			&& Shown->Tracker.Step == 2 && Shown->Tracker.StepCount == 2 && Shown->Tracker.bAnnouncedEnd);
		TestEqual(TEXT("...the full line"), Shown->Objective.ToString(), FString(TEXT("Ready to turn in: talk to Wren")));
		FVector Waypoint = FVector::ZeroVector;
		TestTrue(TEXT("...and the arrow on Wren"), Display->GetTrackedWaypoint(Waypoint) && Waypoint.Equals(Wren->GetActorLocation(), 1.0));
		// The HUD's tracker ticks the last objective (its step done), then slides the turn-in in.
		TestTrue(TEXT("The tracker takes it as the last step done"), UHudMissionTrackerWidget::DecideChange(LastObjective, Shown->Tracker, false)
			== UHudMissionTrackerWidget::EChange::StepDone);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInGiverTest, "Looter.Missions.TurnIn.Giver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInGiverTest::RunTest(const FString& Parameters)
{
	// Only talking to the giver turns it in: not someone else, not using the giver, not the console's talk with another
	// tag. Then it's finished with its rewards, and the mission it was the last prerequisite of starts.
	FCampaignRecord Campaign;
	FHeard Heard;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(500.0, 0.0, 0.0), GiverTag);
	AActor* Stranger = SpawnMarker(World, FVector(-500.0, 0.0, 0.0), OtherTag);
	if (!Runner || !Player || !Wren || !Stranger)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	const FName Id(TEXT("TestErrand"));
	const FName NextId(TEXT("TestNext"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Next = NewMission(Scratch, TEXT("TestNext"), EMissionKind::Main, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionEventObjective>(Next, 0)->Event = TEXT("Test.Later");
	Next->Prerequisites = { Id };
	Runner->BeginForTesting({ MakeErrand(Scratch, TEXT("TestErrand")), Next }, Campaign, Player);
	Listen(*Runner, Heard);
	Runner->Update(0.f);
	Runner->CompleteStep(Id);
	Runner->CompleteStep(Id);
	TestTrue(TEXT("Ready to turn in"), Runner->IsReadyToTurnIn(Id));
	TestFalse(TEXT("...and what comes after waits for the turn-in"), Runner->IsRunning(NextId));

	Runner->NotifyEvent(FMissionEvent::Talked(Stranger));
	TestTrue(TEXT("Talking to someone else: still waiting"), Runner->IsReadyToTurnIn(Id) && Heard.Finished == 0);
	Runner->NotifyEvent(FMissionEvent::Interaction(Wren, /*bHeld*/ false));
	TestTrue(TEXT("Using Wren rather than talking: still waiting"), Runner->IsReadyToTurnIn(Id) && Heard.Finished == 0);
	Runner->NotifyEvent(FMissionEvent::Named(FMissionEvent::Talk, nullptr, OtherTag));
	TestTrue(TEXT("The console's talk with another tag: still waiting"), Runner->IsReadyToTurnIn(Id) && Heard.Finished == 0);
	TestFalse(TEXT("Turning in one that isn't ready does nothing"), Runner->TurnIn(NextId));

	Runner->NotifyEvent(FMissionEvent::Talked(Wren));
	TestTrue(TEXT("Talked to Wren: turned in, finished"), Campaign.HasCompleted(Id) && !Runner->IsRunning(Id) && !Runner->IsReadyToTurnIn(Id));
	TestTrue(TEXT("...off the campaign's waiting list, and no longer its mission"), !Campaign.IsReadyToTurnIn(Id) && Campaign.ActiveMission != Id);
	TestTrue(TEXT("...its rewards given once: its 25 experience"), Heard.Finished == 1 && Heard.Rewarded == 1 && Heard.Last.Experience == 25);
	TestTrue(TEXT("...and the next mission starts, not taking the same talk"), Runner->IsRunning(NextId) && Runner->GetStep(NextId) == 0);

	// The console's Looter.Mission.Event Talk <tag> stands in for a talk: the tag alone turns one in too.
	FCampaignRecord Second;
	FTestWorldWrapper SecondLevel;
	if (!TestTrue(TEXT("Second test level made"), SecondLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UMissionRunner* SecondRunner = SecondLevel.GetTestWorld()->GetSubsystem<UMissionRunner>();
	AActor* SecondPlayer = SpawnMarker(SecondLevel.GetTestWorld(), FVector::ZeroVector);
	if (!SecondRunner || !SecondPlayer)
	{
		return false;
	}
	SecondRunner->BeginForTesting({ MakeErrand(Scratch, TEXT("TestSideErrand"), EMissionKind::Side) }, Second, SecondPlayer);
	SecondRunner->Update(0.f);
	SecondRunner->CompleteStep(TEXT("TestSideErrand"));
	SecondRunner->CompleteStep(TEXT("TestSideErrand"));
	TestTrue(TEXT("A side mission waits too, kept by the campaign though it keeps no side mission's step"),
		SecondRunner->IsReadyToTurnIn(TEXT("TestSideErrand")) && Second.IsReadyToTurnIn(TEXT("TestSideErrand")) && Second.ActiveMission.IsNone());
	SecondRunner->NotifyEvent(FMissionEvent::Named(FMissionEvent::Talk, nullptr, GiverTag));
	TestTrue(TEXT("The giver's tag alone turns it in"), Second.HasCompleted(TEXT("TestSideErrand")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInRewardsTest, "Looter.Missions.TurnIn.Rewards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInRewardsTest::RunTest(const FString& Parameters)
{
	// Rewards only at the turn-in: nothing as each objective and step is done (no experience for steps, the user's call),
	// its fixed experience as it's turned in, and nothing more when it's finished again.
	FCampaignRecord Campaign;
	FHeard Heard;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(500.0, 0.0, 0.0), GiverTag);
	if (!Runner || !Player || !Wren)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	const FName Id(TEXT("TestErrand"));
	UMissionDefinition* Errand = MakeErrand(CreatePackage(nullptr), TEXT("TestErrand"));
	Errand->Rewards.UnlockAreas = { FName(TEXT("SomewhereNew")) };
	Runner->BeginForTesting({ Errand }, Campaign, Player);
	Listen(*Runner, Heard);
	Runner->Update(0.f);
	Runner->NotifyEvent(FMissionEvent::Named(FirstEvent));
	TestTrue(TEXT("A step done: nothing given"), Heard.Finished == 0 && Heard.Last.IsEmpty() && !Campaign.IsAreaOpen(TEXT("SomewhereNew")));
	Runner->NotifyEvent(FMissionEvent::Named(SecondEvent));
	TestTrue(TEXT("Every objective done: still nothing given"), Heard.Finished == 0 && Heard.Last.IsEmpty() && !Campaign.IsAreaOpen(TEXT("SomewhereNew")));
	Runner->Update(1.f);
	TestTrue(TEXT("...however long it waits"), Heard.Finished == 0 && Runner->IsReadyToTurnIn(Id));

	Runner->NotifyEvent(FMissionEvent::Talked(Wren));
	TestTrue(TEXT("Turned in: its experience, as a fixed amount"), Heard.Rewarded == 1 && Heard.Last.Experience == 25);
	TestTrue(TEXT("...and the area it opens, by name"), Campaign.IsAreaOpen(TEXT("SomewhereNew")) && Heard.Last.AreasOpened.Num() == 1
		&& Heard.Last.AreasOpened[0].ToString() == TEXT("SomewhereNew"));
	TestEqual(TEXT("A fixed amount is the same at any level"), MissionRewards::ExperienceOf(Errand->Rewards, 9, FXPCurve()), static_cast<int64>(25));
	TestEqual(TEXT("...and nothing at the last level"), MissionRewards::ExperienceOf(Errand->Rewards, FXPCurve().MaxLevel, FXPCurve()), static_cast<int64>(0));

	// Played again from the console: finished again, nothing more.
	Runner->StartMission(Id, 0, /*bForce*/ true);
	Runner->CompleteMission(Id);
	TestTrue(TEXT("Finished again: no second reward"), Heard.Finished == 2 && Heard.Rewarded == 1 && Heard.Last.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInSaveTest, "Looter.Missions.TurnIn.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInSaveTest::RunTest(const FString& Parameters)
{
	// Ready to turn in survives a save: through the session save's format and back, a new level's runner brings the main
	// mission and a side mission back waiting for their giver, not asking their objectives again; one talk turns both in.
	const FName MainId(TEXT("TestErrand"));
	const FName SideId(TEXT("TestSideErrand"));
	UPackage* Scratch = CreatePackage(nullptr);
	FCampaignRecord Played;
	{
		FTestWorldWrapper TestLevel;
		if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
		{
			return false;
		}
		UMissionRunner* Runner = TestLevel.GetTestWorld()->GetSubsystem<UMissionRunner>();
		AActor* Player = SpawnMarker(TestLevel.GetTestWorld(), FVector::ZeroVector);
		if (!Runner || !Player)
		{
			return false;
		}
		Runner->BeginForTesting({ MakeErrand(Scratch, TEXT("TestErrand")), MakeErrand(Scratch, TEXT("TestSideErrand"), EMissionKind::Side) }, Played, Player);
		Runner->Update(0.f);
		Runner->NotifyEvent(FMissionEvent::Named(FirstEvent));
		Runner->NotifyEvent(FMissionEvent::Named(SecondEvent));
		TestTrue(TEXT("Both ready to turn in"), Runner->IsReadyToTurnIn(MainId) && Runner->IsReadyToTurnIn(SideId));
	}

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->Campaign = Played;
	TArray<uint8> Bytes;
	const ULooterSessionSave* Read = UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	if (!TestNotNull(TEXT("Saved and read back"), Read))
	{
		return false;
	}
	TestTrue(TEXT("The save keeps both waiting, and the main one at its step past the last"), Read->Campaign.IsReadyToTurnIn(MainId)
		&& Read->Campaign.IsReadyToTurnIn(SideId) && Read->Campaign.ActiveMission == MainId && Read->Campaign.ActiveMissionStep == 2
		&& !Read->Campaign.HasCompleted(MainId) && !Read->Campaign.HasCompleted(SideId));
	TestTrue(TEXT("A save from before turn-ins reads as none waiting"), FCampaignRecord().ReadyMissions.IsEmpty());

	// The record and the counters outlive the level, which the runner keeps them for.
	FCampaignRecord Loaded = Read->Campaign;
	FHeard Heard;
	FTestWorldWrapper NextLevel;
	if (!TestTrue(TEXT("Next test level made"), NextLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = NextLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(500.0, 0.0, 0.0), GiverTag);
	if (!Runner || !Display || !Player || !Wren)
	{
		return false;
	}
	// The same missions again, made afresh (in a package of their own: the first level's are gone with it).
	UPackage* NextScratch = CreatePackage(nullptr);
	Runner->BeginForTesting({ MakeErrand(NextScratch, TEXT("TestErrand")), MakeErrand(NextScratch, TEXT("TestSideErrand"), EMissionKind::Side) },
		Loaded, Player);
	Listen(*Runner, Heard);
	Runner->Update(0.f);
	TestTrue(TEXT("Loaded: the main mission is back, waiting for Wren"), Runner->IsRunning(MainId) && Runner->IsReadyToTurnIn(MainId)
		&& Runner->GetStep(MainId) == 2);
	TestTrue(TEXT("...and so is the side mission, not started over"), Runner->IsRunning(SideId) && Runner->IsReadyToTurnIn(SideId)
		&& Runner->GetStep(SideId) == 2);
	const FMission* Shown = Display->GetTracked();
	TestTrue(TEXT("The tracker shows the turn-in at once"), Shown && Shown->Tracker.bTurnIn);

	Runner->NotifyEvent(FMissionEvent::Talked(Wren));
	TestTrue(TEXT("One talk with Wren turns both in"), Loaded.HasCompleted(MainId) && Loaded.HasCompleted(SideId) && Heard.Rewarded == 2
		&& Loaded.ReadyMissions.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInAutomaticTest, "Looter.Missions.TurnIn.Automatic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInAutomaticTest::RunTest(const FString& Parameters)
{
	// A mission with no one to turn it in to (Skyreach's tutorial and skiff, a boss fight ending in its own scene) finishes
	// by itself after its last objective, as one with no giver named does (a test's mission made in code).
	FCampaignRecord Campaign;
	FHeard Heard;
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
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Flagged = MakeErrand(Scratch, TEXT("TestAutomatic"), EMissionKind::Side);
	Flagged->TurnIn.bAutomatic = true;
	UMissionDefinition* NoGiver = NewMission(Scratch, TEXT("TestNoGiver"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionEventObjective>(NoGiver, 0)->Event = SecondEvent;
	TestTrue(TEXT("Neither waits for a turn-in"), !Flagged->NeedsTurnIn() && !NoGiver->NeedsTurnIn());
	Runner->BeginForTesting({ Flagged, NoGiver }, Campaign, Player);
	Listen(*Runner, Heard);
	Runner->Update(0.f);
	Runner->NotifyEvent(FMissionEvent::Named(FirstEvent));
	Runner->NotifyEvent(FMissionEvent::Named(SecondEvent));
	TestTrue(TEXT("Flagged automatic: finished by its last objective, its giver's tag or not"), Campaign.HasCompleted(TEXT("TestAutomatic"))
		&& !Runner->IsReadyToTurnIn(TEXT("TestAutomatic")) && !Campaign.IsReadyToTurnIn(TEXT("TestAutomatic")));
	TestTrue(TEXT("No giver named: finished too"), Campaign.HasCompleted(TEXT("TestNoGiver")));
	TestTrue(TEXT("...each rewarded as it finished"), Heard.Rewarded == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInTalkTest, "Looter.Missions.TurnIn.Talk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInTalkTest::RunTest(const FString& Parameters)
{
	// At the giver's speaker point: the key says "Turn in" while a mission waits for them, the talk says the mission's own
	// turn-in line (in the giver's name) instead of their usual words, and turns it in. A mission whose turn-in has no
	// line of its own is turned in with the giver's own words (the talk the story already wrote).
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	ASpeakerPoint* Wren = PlaceSpeaker(World, InteractionTestWorld::Ahead(200.0), GiverTag, TEXT("Wren"), TEXT("Morning."));
	if (!Runner || !Captions || !Player || !Interaction || !Wren)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Errand = MakeErrand(Scratch, TEXT("TestErrand"));
	Errand->TurnIn.Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("That's the lot. Here's your due.")), 3.f) };
	UMissionDefinition* Merged = MakeErrand(Scratch, TEXT("TestMerged"), EMissionKind::Side);
	Merged->Prerequisites = { FName(TEXT("TestErrand")) };
	Runner->BeginForTesting({ Errand, Merged }, Campaign, Player);
	Runner->Update(0.f);

	TestEqual(TEXT("Nothing waiting: the key says Talk"), Wren->SpeakerPoint->GetInteractionOptions(*Interaction).TapPrompt.ToString(), FString(TEXT("Talk")));
	TestTrue(TEXT("Talked to before it's ready: her own words, nothing turned in"), Wren->SpeakerPoint->Talk(Player)
		&& OnScreen(*Captions) == TEXT("Wren|Morning.") && Runner->IsRunning(TEXT("TestErrand")) && !Campaign.HasCompleted(TEXT("TestErrand")));

	Runner->CompleteStep(TEXT("TestErrand"));
	Runner->CompleteStep(TEXT("TestErrand"));
	TestEqual(TEXT("Ready: the key says Turn in"), Wren->SpeakerPoint->GetInteractionOptions(*Interaction).TapPrompt.ToString(), FString(TEXT("Turn in")));
	Captions->Update(20.f);
	TestTrue(TEXT("Talked to: the mission's line, in her name"), Wren->SpeakerPoint->Talk(Player)
		&& OnScreen(*Captions) == TEXT("Wren|That's the lot. Here's your due."));
	TestTrue(TEXT("...and it's turned in"), Campaign.HasCompleted(TEXT("TestErrand")));
	TestTrue(TEXT("...the next one started, not by that talk"), Runner->IsRunning(TEXT("TestMerged")) && Runner->GetStep(TEXT("TestMerged")) == 0);

	Runner->CompleteStep(TEXT("TestMerged"));
	Runner->CompleteStep(TEXT("TestMerged"));
	Captions->Update(20.f);
	TestTrue(TEXT("A turn-in with no line of its own: her own words, and it's turned in"), Wren->SpeakerPoint->Talk(Player)
		&& OnScreen(*Captions) == TEXT("Wren|Morning.") && Campaign.HasCompleted(TEXT("TestMerged")));
	TestEqual(TEXT("Nothing waiting any more: Talk again"), Wren->SpeakerPoint->GetInteractionOptions(*Interaction).TapPrompt.ToString(), FString(TEXT("Talk")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInOldSaveTest, "Looter.Missions.TurnIn.OldSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInOldSaveTest::RunTest(const FString& Parameters)
{
	// A session saved before turn-ins, on a step its mission no longer has (the talk with its giver, now its turn-in: Main 1,
	// 3 and 4; or past Main 7's depot), comes back waiting for its turn-in: nothing done is asked again.
	FCampaignRecord Campaign;
	Campaign.ActiveMission = TEXT("TestErrand");
	Campaign.ActiveMissionStep = 2;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UMissionRunner* Runner = TestLevel.GetTestWorld()->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(TestLevel.GetTestWorld(), FVector::ZeroVector);
	if (!Runner || !Player)
	{
		return false;
	}
	Runner->BeginForTesting({ MakeErrand(CreatePackage(nullptr), TEXT("TestErrand")) }, Campaign, Player);
	Runner->Update(0.f);
	TestTrue(TEXT("Saved on its old last step: back waiting for Wren"), Runner->IsReadyToTurnIn(TEXT("TestErrand"))
		&& Runner->GetStep(TEXT("TestErrand")) == 2 && Campaign.IsReadyToTurnIn(TEXT("TestErrand")));
	TestFalse(TEXT("...not finished"), Campaign.HasCompleted(TEXT("TestErrand")));
	return true;
}

#endif
