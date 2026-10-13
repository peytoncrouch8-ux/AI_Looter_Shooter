#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLine.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/MissionArtifact.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Missions picked up from their givers, Borderlands' way (2026-10-13): offered, not started, once what comes first is done;
// at their giver only; accepted, recorded and started; an old save's mission taken already; a talk turning in before it
// offers; an artifact gone once its mission is taken.

using namespace MissionTestWorld;

namespace
{
	const FName WrenTag(TEXT("Speaker_Wren"));
	const FName OtherTag(TEXT("Speaker_Stranger"));
	const FName ArtifactTag(TEXT("Artifact_TestTag"));
	const FName NoteTag(TEXT("Artifact_TestNote"));
	const FName HereArea(TEXT("TestHere"));
	const FName FirstEvent(TEXT("Test.First"));
	const FName SecondEvent(TEXT("Test.Second"));

	/** A mission of two steps (an event each), offered by the giver tagged Giver, played anywhere. */
	UMissionDefinition* MakeOffered(UObject* Outer, const TCHAR* Id, EMissionKind Kind, FName Giver, FName Area = NAME_None)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, Kind, EMissionStart::Offered, Area);
		AddObjective<UMissionEventObjective>(Mission, 0)->Event = FirstEvent;
		AddObjective<UMissionEventObjective>(Mission, 1)->Event = SecondEvent;
		Mission->Offer.GiverTag = Giver;
		Mission->TurnIn.bAutomatic = true;
		return Mission;
	}

	/** A mission that starts by itself and is turned in to Wren. */
	UMissionDefinition* MakeErrand(UObject* Outer, const TCHAR* Id)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, EMissionKind::Main, EMissionStart::Automatic, NAME_None);
		AddObjective<UMissionEventObjective>(Mission, 0)->Event = FirstEvent;
		Mission->TurnIn.SpeakerTag = WrenTag;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("Wren"));
		return Mission;
	}

	FStoryLine Said(const TCHAR* Words)
	{
		return FStoryLine::Make(FText::GetEmpty(), FText::FromString(Words), 3.f);
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
		Point->SpeakerPoint->Lines = { Said(Words) };
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGiverAvailabilityTest, "Looter.Missions.Givers.Availability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGiverAvailabilityTest::RunTest(const FString& Parameters)
{
	// Offered once what comes first is finished, in its area, until taken; never started by itself; listed as available
	// (Missions page) whether it's offered here or in its own area.
	FCampaignRecord Campaign;
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
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Job = MakeOffered(Scratch, TEXT("TestJob"), EMissionKind::Side, WrenTag);
	UMissionDefinition* Later = MakeOffered(Scratch, TEXT("TestLater"), EMissionKind::Side, WrenTag);
	Later->Prerequisites = { FName(TEXT("TestJob")) };
	UMissionDefinition* Far = MakeOffered(Scratch, TEXT("TestFar"), EMissionKind::Side, WrenTag, TEXT("Elsewhere"));
	UMissionDefinition* Nobody = MakeOffered(Scratch, TEXT("TestNobody"), EMissionKind::Side, NAME_None);
	UMissionDefinition* ByItself = NewMission(Scratch, TEXT("TestByItself"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionEventObjective>(ByItself, 0)->Event = FirstEvent;
	// Nobody (offered with no giver) is never registered: the runner warns of such data, and the test reads its definition.
	Runner->BeginForTesting({ Job, Later, Far, ByItself }, Campaign, Player, HereArea);
	Runner->Update(0.f);

	TestTrue(TEXT("Offered, not started: it waits at its giver"), Runner->IsAvailable(TEXT("TestJob")) && !Runner->IsRunning(TEXT("TestJob")));
	TestTrue(TEXT("...listed as available"), Runner->GetStatus(*Job) == EMissionStatus::Available);
	TestFalse(TEXT("...not taken yet"), Runner->IsAccepted(TEXT("TestJob")));
	TestTrue(TEXT("What comes first unfinished: not offered, locked"), !Runner->IsAvailable(TEXT("TestLater"))
		&& Runner->GetStatus(*Later) == EMissionStatus::Locked);
	TestTrue(TEXT("Another area's: not offered here, still listed (its own area offers it)"), !Runner->IsAvailable(TEXT("TestFar"))
		&& Runner->GetStatus(*Far) == EMissionStatus::Available);
	TestTrue(TEXT("Offered by nobody: never offered"), !Nobody->IsOffered() && !Runner->IsAvailable(TEXT("TestNobody")));
	TestTrue(TEXT("One that starts by itself isn't offered: it runs"), !Runner->IsAvailable(TEXT("TestByItself")) && Runner->IsRunning(TEXT("TestByItself")));

	// Taken: no longer offered, under way.
	TestTrue(TEXT("Accepted"), Runner->AcceptMission(TEXT("TestJob")));
	TestTrue(TEXT("...not offered any more, under way"), !Runner->IsAvailable(TEXT("TestJob")) && Runner->GetStatus(*Job) == EMissionStatus::Active);
	TestFalse(TEXT("...and what comes after it still waits for it to finish"), Runner->IsAvailable(TEXT("TestLater")));

	// Finished: what comes after it is offered, and it isn't offered again.
	Runner->CompleteMission(TEXT("TestJob"));
	TestTrue(TEXT("Finished: the next one is offered"), Runner->IsAvailable(TEXT("TestLater")) && !Runner->IsAvailable(TEXT("TestJob")));
	TestTrue(TEXT("...and the finished one is listed done"), Runner->GetStatus(*Job) == EMissionStatus::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGiverOffersAtTest, "Looter.Missions.Givers.OffersAt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGiverOffersAtTest::RunTest(const FString& Parameters)
{
	// What a giver offers now: their own missions, offered and not taken, the story's first; nobody else's.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(600.0, 0.0, 0.0), WrenTag);
	AActor* Stranger = SpawnMarker(World, FVector(-600.0, 0.0, 0.0), OtherTag);
	if (!Runner || !Player || !Wren || !Stranger)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Side = MakeOffered(Scratch, TEXT("TestSide"), EMissionKind::Side, WrenTag);
	UMissionDefinition* Main = MakeOffered(Scratch, TEXT("TestMain"), EMissionKind::Main, WrenTag);
	UMissionDefinition* Locked = MakeOffered(Scratch, TEXT("TestLocked"), EMissionKind::Main, WrenTag);
	Locked->Prerequisites = { FName(TEXT("TestNever")) };
	UMissionDefinition* Theirs = MakeOffered(Scratch, TEXT("TestTheirs"), EMissionKind::Side, OtherTag);
	Runner->BeginForTesting({ Side, Main, Locked, Theirs }, Campaign, Player);
	Runner->Update(0.f);

	TArray<const UMissionDefinition*> AtWren = Runner->GetOffersAt(Wren);
	TestEqual(TEXT("Wren offers her two"), AtWren.Num(), 2);
	TestTrue(TEXT("...the story's first"), AtWren.Num() == 2 && AtWren[0] == Main && AtWren[1] == Side);
	TestTrue(TEXT("...not the locked one, nor the stranger's"), !AtWren.Contains(Locked) && !AtWren.Contains(Theirs));
	TestTrue(TEXT("The stranger offers his"), Runner->GetOffersAt(Stranger).Num() == 1 && Runner->GetOffersAt(Stranger)[0] == Theirs);
	TestTrue(TEXT("Nobody offers nothing"), Runner->GetOffersAt(nullptr).IsEmpty() && Runner->GetOffersAt(Player).IsEmpty());
	const TOptional<FVector> WrenAt = Runner->FindOfferGiver(*Main);
	TestTrue(TEXT("Its giver is found where she stands"), WrenAt.IsSet() && WrenAt->Equals(Wren->GetActorLocation(), 1.0));

	Runner->AcceptMission(TEXT("TestMain"));
	TestTrue(TEXT("Taken: she offers only the side job now"), Runner->GetOffersAt(Wren).Num() == 1 && Runner->GetOffersAt(Wren)[0] == Side);

	// A giver hidden by the story offers nothing from where she isn't.
	Wren->SetActorHiddenInGame(true);
	TestFalse(TEXT("A hidden giver isn't where an offer is found"), Runner->FindOfferGiver(*Side).IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGiverAcceptTest, "Looter.Missions.Givers.Accept",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGiverAcceptTest::RunTest(const FString& Parameters)
{
	// Accepting records it in the campaign, starts it and has the tracker follow it when nothing else was tracked; the
	// listeners hear of it. Taken, it starts by itself in its area's next level; one never taken never does.
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
	if (!Runner || !Display || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* First = MakeOffered(Scratch, TEXT("TestFirst"), EMissionKind::Side, WrenTag);
	UMissionDefinition* Second = MakeOffered(Scratch, TEXT("TestSecond"), EMissionKind::Side, WrenTag);
	UMissionDefinition* Locked = MakeOffered(Scratch, TEXT("TestLocked"), EMissionKind::Side, WrenTag);
	Locked->Prerequisites = { FName(TEXT("TestNever")) };
	Runner->BeginForTesting({ First, Second, Locked }, Campaign, Player);
	int32 Changes = 0;
	Runner->OnChanged.AddLambda([&Changes]() { ++Changes; });
	Runner->Update(0.f);
	const int32 ChangesBefore = Changes;

	TestTrue(TEXT("Accepted"), Runner->AcceptMission(TEXT("TestFirst")));
	TestTrue(TEXT("...recorded in the campaign"), Campaign.IsAccepted(TEXT("TestFirst")) && Runner->IsAccepted(TEXT("TestFirst")));
	TestTrue(TEXT("...started at its first step"), Runner->IsRunning(TEXT("TestFirst")) && Runner->GetStep(TEXT("TestFirst")) == 0);
	TestEqual(TEXT("...tracked, with nothing else tracked"), Runner->GetTrackedMission(), FName(TEXT("TestFirst")));
	TestTrue(TEXT("...and the listeners heard"), Changes > ChangesBefore);
	TestTrue(TEXT("Accepted again: nothing new"), Runner->AcceptMission(TEXT("TestFirst")) && Campaign.AcceptedMissions.Num() == 1);

	TestTrue(TEXT("A second one taken"), Runner->AcceptMission(TEXT("TestSecond")) && Runner->IsRunning(TEXT("TestSecond")));
	TestEqual(TEXT("...the tracked one stays tracked"), Runner->GetTrackedMission(), FName(TEXT("TestFirst")));
	TestFalse(TEXT("One whose prerequisites aren't done can't be taken"), Runner->AcceptMission(TEXT("TestLocked")));
	TestFalse(TEXT("...nor recorded"), Campaign.IsAccepted(TEXT("TestLocked")));
	Runner->CompleteMission(TEXT("TestFirst"));
	TestFalse(TEXT("A finished one can't be taken again"), Runner->AcceptMission(TEXT("TestFirst")));

	// The next level of the session: the one taken comes back by itself, one never taken waits at its giver.
	FCampaignRecord NextCampaign = Campaign;
	FTestWorldWrapper NextLevel;
	if (!TestTrue(TEXT("Next level made"), NextLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* NextWorld = NextLevel.GetTestWorld();
	UMissionRunner* NextRunner = NextWorld->GetSubsystem<UMissionRunner>();
	AActor* NextPlayer = SpawnMarker(NextWorld, FVector::ZeroVector);
	UMissionDefinition* NotTaken = MakeOffered(Scratch, TEXT("TestNotTaken"), EMissionKind::Side, WrenTag);
	NextRunner->BeginForTesting({ First, Second, NotTaken }, NextCampaign, NextPlayer);
	NextRunner->Update(0.f);
	TestTrue(TEXT("Next level: the one taken runs again by itself"), NextRunner->IsRunning(TEXT("TestSecond")));
	TestTrue(TEXT("...the finished one doesn't"), !NextRunner->IsRunning(TEXT("TestFirst")));
	TestTrue(TEXT("...and one never taken waits at its giver"), !NextRunner->IsRunning(TEXT("TestNotTaken")) && NextRunner->IsAvailable(TEXT("TestNotTaken")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGiverOldSaveTest, "Looter.Missions.Givers.OldSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGiverOldSaveTest::RunTest(const FString& Parameters)
{
	// A session saved before missions were offered: the main mission it was playing, and a side one waiting for its
	// turn-in, were taken already. They go on, recorded as taken, and nobody offers them again.
	FCampaignRecord Campaign;
	Campaign.ActiveMission = TEXT("TestStory");
	Campaign.ActiveMissionStep = 1;
	Campaign.ReadyMissions = { FName(TEXT("TestChore")) };
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(600.0, 0.0, 0.0), WrenTag);
	if (!Runner || !Player || !Wren)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Story = MakeOffered(Scratch, TEXT("TestStory"), EMissionKind::Main, WrenTag);
	UMissionDefinition* Chore = MakeOffered(Scratch, TEXT("TestChore"), EMissionKind::Side, WrenTag);
	Chore->TurnIn.bAutomatic = false;
	Chore->TurnIn.SpeakerTag = WrenTag;
	TestTrue(TEXT("The old save records nothing as taken"), Campaign.AcceptedMissions.IsEmpty());
	Runner->BeginForTesting({ Story, Chore }, Campaign, Player);
	TestTrue(TEXT("...already, as the runner reads them"), Runner->IsAccepted(TEXT("TestStory")) && Runner->IsAccepted(TEXT("TestChore")));
	Runner->Update(0.f);

	TestTrue(TEXT("The main one goes on from its step"), Runner->IsRunning(TEXT("TestStory")) && Runner->GetStep(TEXT("TestStory")) == 1);
	TestTrue(TEXT("...recorded as taken"), Campaign.IsAccepted(TEXT("TestStory")));
	TestTrue(TEXT("The side one waits for its turn-in"), Runner->IsReadyToTurnIn(TEXT("TestChore")) && Campaign.IsAccepted(TEXT("TestChore")));
	TestTrue(TEXT("Neither is offered again"), !Runner->IsAvailable(TEXT("TestStory")) && !Runner->IsAvailable(TEXT("TestChore"))
		&& Runner->GetOffersAt(Wren).IsEmpty());
	TestTrue(TEXT("Both under way on the Missions page"), Runner->GetStatus(*Story) == EMissionStatus::Active
		&& Runner->GetStatus(*Chore) == EMissionStatus::Active);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGiverTalkTest, "Looter.Missions.Givers.Talk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGiverTalkTest::RunTest(const FString& Parameters)
{
	// Someone with a job offers it as they talk: the offer's lines, then the card once they're said. A mission waiting to
	// be turned in to them comes first: that talk turns it in, and the job is offered after its words.
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
	ASpeakerPoint* Wren = PlaceSpeaker(World, InteractionTestWorld::Ahead(200.0), WrenTag, TEXT("Wren"), TEXT("Morning."));
	if (!Runner || !Captions || !Player || !Wren)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Errand = MakeErrand(Scratch, TEXT("TestErrand"));
	Errand->TurnIn.Lines = { Said(TEXT("That's the lot. Here's your due.")) };
	UMissionDefinition* Job = MakeOffered(Scratch, TEXT("TestJob"), EMissionKind::Side, WrenTag);
	Job->Offer.Lines = { Said(TEXT("While you're here: the well's gone foul.")) };
	UMissionDefinition* Next = MakeOffered(Scratch, TEXT("TestNext"), EMissionKind::Main, WrenTag);
	Next->Prerequisites = { FName(TEXT("TestErrand")) };
	Runner->BeginForTesting({ Errand, Job, Next }, Campaign, Player);
	Runner->Update(0.f);

	// A turn-in waits and a job is offered: the talk turns in first, with the turn-in's words, and offers after.
	Runner->CompleteStep(TEXT("TestErrand"));
	TestTrue(TEXT("Ready to turn in to Wren, and a job of hers offered"), Runner->IsReadyToTurnIn(TEXT("TestErrand")) && Runner->IsAvailable(TEXT("TestJob")));
	TestTrue(TEXT("Talked to"), Wren->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...the turn-in's words first"), OnScreen(*Captions), FString(TEXT("Wren|That's the lot. Here's your due.")));
	TestTrue(TEXT("...turned in"), Campaign.HasCompleted(TEXT("TestErrand")));
	TestTrue(TEXT("...what it opened is offered after its words, the story first"), Wren->SpeakerPoint->GetPendingOffer() == FName(TEXT("TestNext")));
	TestTrue(TEXT("...offered, not started"), !Runner->IsRunning(TEXT("TestNext")) && Runner->IsAvailable(TEXT("TestNext")));
	TestFalse(TEXT("...nor talked to again before the words are said"), Wren->SpeakerPoint->CanTalk());

	// The words said and nobody's HUD to show the card (a test level): it's left for the next talk.
	Captions->Update(30.f);
	TestFalse(TEXT("No card without a HUD"), Wren->SpeakerPoint->UpdatePendingOffer());
	TestTrue(TEXT("...left for the next talk"), Wren->SpeakerPoint->GetPendingOffer().IsNone() && Runner->IsAvailable(TEXT("TestNext")));

	// Taken (as the card would), the side job is next: offered with its own lines.
	Runner->AcceptMission(TEXT("TestNext"));
	TestTrue(TEXT("Talked to again"), Wren->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...the job's own lines"), OnScreen(*Captions), FString(TEXT("Wren|While you're here: the well's gone foul.")));
	TestEqual(TEXT("...and the job offered once they're said"), Wren->SpeakerPoint->GetPendingOffer(), FName(TEXT("TestJob")));
	TestTrue(TEXT("...still waiting for its words"), !Wren->SpeakerPoint->UpdatePendingOffer() && !Wren->SpeakerPoint->GetPendingOffer().IsNone());

	// Nothing to offer, nothing to turn in: just her words.
	Runner->AcceptMission(TEXT("TestJob"));
	Captions->Update(30.f);
	TestTrue(TEXT("Talked to with nothing left"), Wren->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("...her own words, nothing offered"), OnScreen(*Captions) == TEXT("Wren|Morning.") && Wren->SpeakerPoint->GetPendingOffer().IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGiverArtifactTest, "Looter.Missions.Givers.Artifact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGiverArtifactTest::RunTest(const FString& Parameters)
{
	// A found thing offers its mission (picked up with Interact); taken with its mission it's gone for good, and stays gone
	// in the session's next levels. One that isn't taken on accept (a notice nailed up) stays where it is.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AMissionArtifact* Tag = World->SpawnActor<AMissionArtifact>(InteractionTestWorld::Ahead(150.0), FRotator::ZeroRotator);
	AMissionArtifact* Note = World->SpawnActor<AMissionArtifact>(InteractionTestWorld::Ahead(-150.0), FRotator::ZeroRotator);
	if (!Runner || !Player || !Interaction || !Tag || !Note)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* TagJob = MakeOffered(Scratch, TEXT("TestTagJob"), EMissionKind::Side, ArtifactTag);
	UMissionDefinition* NoteJob = MakeOffered(Scratch, TEXT("TestNoteJob"), EMissionKind::Side, NoteTag);
	UMissionDefinition* Later = MakeOffered(Scratch, TEXT("TestLaterJob"), EMissionKind::Side, NoteTag);
	Later->Prerequisites = { FName(TEXT("TestNever")) };
	Runner->BeginForTesting({ TagJob, NoteJob, Later }, Campaign, Player);
	Tag->Tags.Add(ArtifactTag);
	Tag->Prompt = FText::FromString(TEXT("Pick up the tag"));
	Note->Tags.Add(NoteTag);
	Note->bTakeOnAccept = false;
	Tag->DispatchBeginPlay();
	Note->DispatchBeginPlay();
	Runner->Update(0.f);

	TestTrue(TEXT("The tag offers its job"), Tag->GetOffer() == TagJob && !Tag->IsTaken() && !Tag->IsHidden());
	const FInteractionOptions Options = Tag->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("...picked up with a tap"), Options.CanUse() && Options.bTap && Options.TapPrompt.ToString() == TEXT("Pick up the tag"));
	TestFalse(TEXT("No HUD to show the card: it isn't picked up"), Tag->Use(Player));

	Runner->AcceptMission(TEXT("TestTagJob"));
	TestTrue(TEXT("Its mission taken: the tag is gone"), Tag->IsTaken() && Tag->IsHidden());
	TestFalse(TEXT("...and nothing to pick up"), Tag->GetInteractionOptions(*Interaction).CanUse());

	Runner->AcceptMission(TEXT("TestNoteJob"));
	TestTrue(TEXT("A notice nailed up stays where it is"), !Note->IsTaken() && !Note->IsHidden());
	TestFalse(TEXT("...with nothing more to offer for now"), Note->GetInteractionOptions(*Interaction).CanUse());

	// The session's next level: the tag's mission was taken, so it's gone from the start.
	FCampaignRecord NextCampaign = Campaign;
	FTestWorldWrapper NextLevel;
	if (!TestTrue(TEXT("Next level made"), NextLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* NextWorld = NextLevel.GetTestWorld();
	UMissionRunner* NextRunner = NextWorld->GetSubsystem<UMissionRunner>();
	AActor* NextPlayer = SpawnMarker(NextWorld, FVector::ZeroVector);
	AMissionArtifact* TagAgain = NextWorld->SpawnActor<AMissionArtifact>(FVector(150.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!NextRunner || !TagAgain)
	{
		AddError(TEXT("The next level isn't whole."));
		return false;
	}
	NextRunner->BeginForTesting({ TagJob, NoteJob, Later }, NextCampaign, NextPlayer);
	TagAgain->Tags.Add(ArtifactTag);
	TagAgain->DispatchBeginPlay();
	TestTrue(TEXT("Next level: the tag is gone from the start"), TagAgain->IsTaken() && TagAgain->IsHidden());
	return true;
}

#endif
