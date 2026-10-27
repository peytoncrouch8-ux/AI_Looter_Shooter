#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Missions/MissionTalk.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The user's mission rules of 2026-10-22: main missions are never offered (they start by themselves the moment they're
// due, tracked, their giver's words said on the move; one whose giver must be met first is listed as "Talk to <giver>"
// and the talk starts it); side missions are offered only after their story gate; the offer card never waits for the
// giver's words, and Accept goes straight to what the giver says once it's taken.

using namespace MissionTestWorld;

namespace
{
	const FName VoiceTag(TEXT("Speaker_Wren"));
	const FName BoardTag(TEXT("NoticeBoard_TestStart"));
	const FName HereArea(TEXT("TestHere"));
	const FName FirstEvent(TEXT("Test.First"));
	const FName LaterEvent(TEXT("Test.Later"));
	const FName Rest(TEXT("RansomsRest"));

	FStoryLine LineOf(const TCHAR* Words, const TCHAR* Speaker = TEXT(""))
	{
		return FStoryLine::Make(FText::FromString(Speaker), FText::FromString(Words), 3.f);
	}

	/** A mission picked up from the giver tagged Giver: two steps, an event each, finishing by itself. */
	UMissionDefinition* MakePickedUp(UObject* Outer, const TCHAR* Id, EMissionKind Kind, FName Giver)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, Kind, EMissionStart::Offered, NAME_None);
		AddObjective<UMissionEventObjective>(Mission, 0)->Event = LaterEvent;
		AddObjective<UMissionEventObjective>(Mission, 1)->Event = LaterEvent;
		Mission->Offer.GiverTag = Giver;
		Mission->Offer.GiverName = FText::FromString(TEXT("Wren"));
		Mission->TurnIn.bAutomatic = true;
		return Mission;
	}

	/** The story's next mission at Wren's, with her words for it and what she says once it's taken. */
	UMissionDefinition* MakeStory(UObject* Outer, const TCHAR* Id)
	{
		UMissionDefinition* Mission = MakePickedUp(Outer, Id, EMissionKind::Main, VoiceTag);
		Mission->Offer.Lines = { LineOf(TEXT("Someone's waiting up top.")) };
		Mission->Offer.AcceptLines = { LineOf(TEXT("Mind the spiders.")) };
		return Mission;
	}

	/** A mission that starts by itself and finishes on FirstEvent: what comes before the story's next one. */
	UMissionDefinition* MakeFirst(UObject* Outer)
	{
		UMissionDefinition* Mission = NewMission(Outer, TEXT("TestFirst"), EMissionKind::Main, EMissionStart::Automatic, NAME_None);
		AddObjective<UMissionEventObjective>(Mission, 0)->Event = FirstEvent;
		Mission->TurnIn.bAutomatic = true;
		return Mission;
	}

	/** Wren at a speaker point, as the build scripts place one. */
	ASpeakerPoint* PlaceVoice(UWorld* World, const FVector& Where)
	{
		ASpeakerPoint* Point = World->SpawnActor<ASpeakerPoint>(Where, FRotator(0.0, 180.0, 0.0));
		if (!Point)
		{
			return nullptr;
		}
		Point->SpeakerPoint->SetRelativeLocation(FVector::ZeroVector);
		Point->SpeakerPoint->SpeakerName = FText::FromString(TEXT("Wren"));
		Point->SpeakerPoint->Lines = { LineOf(TEXT("Morning.")) };
		Point->Tags.Add(VoiceTag);
		Point->DispatchBeginPlay();
		return Point;
	}

	FString CaptionNow(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT("|") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}

	UMissionDefinition* LoadMission(const TCHAR* Id)
	{
		const FString Name = FString(UMissionDefinition::AssetPrefix) + Id;
		const FString Path = FString(UMissionDefinition::AssetFolder) / Name;
		return FPackageName::DoesPackageExist(Path) ? LoadObject<UMissionDefinition>(nullptr, *(Path + TEXT(".") + Name)) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionMainByItselfTest, "Looter.Missions.MainStart.ByItself",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionMainByItselfTest::RunTest(const FString& Parameters)
{
	// Due, a main mission starts by itself: taken, tracked, its giver's words then her accept lines said on the move; nobody
	// offers it. Before what comes first it's locked; another area's waits for its own.
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
	ASpeakerPoint* Wren = PlaceVoice(World, InteractionTestWorld::Ahead(300.0));
	if (!Runner || !Captions || !Player || !Wren)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* First = MakeFirst(Scratch);
	UMissionDefinition* Story = MakeStory(Scratch, TEXT("TestStory"));
	Story->Prerequisites = { FName(TEXT("TestFirst")) };
	UMissionDefinition* Far = MakeStory(Scratch, TEXT("TestFarStory"));
	Far->Prerequisites = { FName(TEXT("TestFirst")) };
	Far->Area = TEXT("Elsewhere");
	Runner->BeginForTesting({ First, Story, Far }, Campaign, Player, HereArea);
	Runner->Update(0.f);

	TestTrue(TEXT("Before what comes first: locked, not under way, not offered"), !Runner->IsRunning(TEXT("TestStory"))
		&& Runner->GetStatus(*Story) == EMissionStatus::Locked && !Runner->IsAvailable(TEXT("TestStory")));

	Runner->NotifyEvent(FMissionEvent::Named(FirstEvent));
	TestTrue(TEXT("What comes first finished"), Campaign.HasCompleted(TEXT("TestFirst")));
	TestTrue(TEXT("The story's next started by itself, at its first step, taken"), Runner->IsRunning(TEXT("TestStory"))
		&& Runner->GetStep(TEXT("TestStory")) == 0 && Campaign.IsAccepted(TEXT("TestStory")));
	TestTrue(TEXT("...under way on the Missions page"), Runner->GetStatus(*Story) == EMissionStatus::Active);
	TestTrue(TEXT("...never offered: nobody has it"), !Runner->IsAvailable(TEXT("TestStory")) && Runner->GetOffersAt(Wren).IsEmpty());
	TestEqual(TEXT("...tracked"), Runner->GetTrackedMission(), FName(TEXT("TestStory")));
	TestEqual(TEXT("...her words for it on the move, in her name"), CaptionNow(*Captions), FString(TEXT("Wren|Someone's waiting up top.")));
	Captions->Update(3.1f);
	TestEqual(TEXT("...then what she says once it's taken"), CaptionNow(*Captions), FString(TEXT("Wren|Mind the spiders.")));
	TestTrue(TEXT("Another area's waits for its own, not taken"), !Runner->IsRunning(TEXT("TestFarStory"))
		&& Runner->GetStatus(*Far) == EMissionStatus::Available && !Campaign.IsAccepted(TEXT("TestFarStory")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionMainMeetFirstTest, "Looter.Missions.MainStart.MeetFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionMainMeetFirstTest::RunTest(const FString& Parameters)
{
	// A main mission met first: listed and tracked as "Talk to Wren" with the arrow on her, under way though its story isn't
	// yet ("during" conditions); the talk says her words and starts it, no card, her accept lines after.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	ASpeakerPoint* Wren = PlaceVoice(World, InteractionTestWorld::Ahead(300.0));
	if (!Runner || !Display || !Captions || !Player || !Wren)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UMissionDefinition* Story = MakeStory(CreatePackage(nullptr), TEXT("TestVisit"));
	Story->Offer.bMeetFirst = true;
	Runner->BeginForTesting({ Story }, Campaign, Player);
	Runner->Update(0.f);
	const FName VisitId(TEXT("TestVisit"));

	TestTrue(TEXT("Due: waiting for Wren, under way, not running, not taken yet"), Runner->IsMeeting(VisitId) && !Runner->IsRunning(VisitId)
		&& Runner->GetStatus(*Story) == EMissionStatus::Active && !Campaign.IsAccepted(VisitId));
	const TArray<FMissionObjectiveView> Views = Runner->GetObjectiveViews(VisitId);
	TestTrue(TEXT("...its one objective: talk to her"), Views.Num() == 1 && Views[0].Text == TEXT("Talk to Wren."));
	TestEqual(TEXT("...tracked"), Runner->GetTrackedMission(), VisitId);
	const FMission* Tracked = Display->GetTracked();
	TestTrue(TEXT("...the tracker's line and the arrow on her"), Tracked && Tracked->Tracker.Line == TEXT("Talk to Wren")
		&& Tracked->Waypoint.IsSet() && Tracked->Waypoint->Equals(Wren->GetActorLocation(), 1.0));
	FStoryCondition During;
	During.DuringMission = VisitId;
	TestFalse(TEXT("...its story isn't under way yet (no 'during' world changes)"), During.IsMet(Campaign, Runner));
	TestTrue(TEXT("...she has it"), Runner->GetOffersAt(Wren).Contains(Story) && Wren->SpeakerPoint->HasMissionBusiness());

	TestTrue(TEXT("Talked to"), Wren->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...her words for it"), CaptionNow(*Captions), FString(TEXT("Wren|Someone's waiting up top.")));
	TestTrue(TEXT("...the talk started it, no card"), Runner->IsRunning(VisitId) && Runner->GetStep(VisitId) == 0
		&& Campaign.IsAccepted(VisitId) && !Runner->IsMeeting(VisitId) && Wren->SpeakerPoint->GetPendingOffer().IsNone());
	TestEqual(TEXT("...still tracked, its run's line now"), Runner->GetTrackedMission(), VisitId);
	TestTrue(TEXT("...its story under way"), During.IsMet(Campaign, Runner));
	Captions->Update(3.1f);
	TestEqual(TEXT("...then her accept lines"), CaptionNow(*Captions), FString(TEXT("Wren|Mind the spiders.")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionMainOldSaveTest, "Looter.Missions.MainStart.OldSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionMainOldSaveTest::RunTest(const FString& Parameters)
{
	// A session saved before 2026-10-22: the story's next mission was offered and never taken. It starts as the level
	// begins (one met first waits listed for its giver); a side mission taken goes on; one behind its gate stays shut.
	FCampaignRecord Campaign;
	Campaign.CompletedMissions = { FName(TEXT("TestFirst")) };
	Campaign.AcceptedMissions = { FName(TEXT("TestChore")) };
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(600.0, 0.0, 0.0), VoiceTag);
	if (!Runner || !Player || !Wren)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* First = MakeFirst(Scratch);
	UMissionDefinition* Story = MakeStory(Scratch, TEXT("TestStory"));
	Story->Prerequisites = { FName(TEXT("TestFirst")) };
	UMissionDefinition* Visit = MakeStory(Scratch, TEXT("TestVisit"));
	Visit->Prerequisites = { FName(TEXT("TestFirst")) };
	Visit->Offer.bMeetFirst = true;
	UMissionDefinition* Chore = MakePickedUp(Scratch, TEXT("TestChore"), EMissionKind::Side, VoiceTag);
	Chore->Prerequisites = { FName(TEXT("TestFirst")) };
	UMissionDefinition* Later = MakePickedUp(Scratch, TEXT("TestLater"), EMissionKind::Side, VoiceTag);
	Later->Prerequisites = { FName(TEXT("TestNever")) };
	Runner->BeginForTesting({ First, Story, Visit, Chore, Later }, Campaign, Player);
	Runner->Update(0.f);

	TestTrue(TEXT("The story mission left at its giver starts as the level begins, taken"), Runner->IsRunning(TEXT("TestStory"))
		&& Campaign.IsAccepted(TEXT("TestStory")));
	TestTrue(TEXT("One met first waits for its giver, listed"), Runner->IsMeeting(TEXT("TestVisit"))
		&& Runner->GetStatus(*Visit) == EMissionStatus::Active && !Runner->IsRunning(TEXT("TestVisit")));
	TestTrue(TEXT("A side mission taken goes on"), Runner->IsRunning(TEXT("TestChore")));
	TestTrue(TEXT("One behind its gate isn't offered"), !Runner->IsAvailable(TEXT("TestLater"))
		&& Runner->GetStatus(*Later) == EMissionStatus::Locked && !Runner->GetOffersAt(Wren).Contains(Later));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionOfferAcceptTalkTest, "Looter.Missions.MainStart.AcceptTalk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionOfferAcceptTalkTest::RunTest(const FString& Parameters)
{
	// The card's two buttons and the giver's words: Accept cuts the offer short and goes straight to her accept lines (she
	// keeps the floor); Not now stops her; a board's accept remark comes after whatever is being said.
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
	ASpeakerPoint* Wren = PlaceVoice(World, InteractionTestWorld::Ahead(300.0));
	AActor* Board = SpawnMarker(World, FVector(-600.0, 0.0, 0.0), BoardTag);
	if (!Runner || !Captions || !Player || !Wren || !Board)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Job = MakePickedUp(Scratch, TEXT("TestJob"), EMissionKind::Side, VoiceTag);
	Job->Offer.Lines = { LineOf(TEXT("The well's gone foul.")), LineOf(TEXT("Nobody's been down since.")) };
	Job->Offer.AcceptLines = { LineOf(TEXT("Mind the rope.")) };
	Job->SortOrder = 0;
	UMissionDefinition* Chore = MakePickedUp(Scratch, TEXT("TestChore"), EMissionKind::Side, VoiceTag);
	Chore->Offer.Lines = { LineOf(TEXT("And the fence is down.")) };
	Chore->SortOrder = 1;
	UMissionDefinition* Bounty = MakePickedUp(Scratch, TEXT("TestBounty"), EMissionKind::Side, BoardTag);
	Bounty->Offer.AcceptLines = { LineOf(TEXT("Easy money, sunshine."), TEXT("Hob")) };
	Runner->BeginForTesting({ Job, Chore, Bounty }, Campaign, Player);
	Runner->Update(0.f);

	TestTrue(TEXT("Talked to"), Wren->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...the job's first words"), CaptionNow(*Captions), FString(TEXT("Wren|The well's gone foul.")));
	TestEqual(TEXT("...the job offered at once"), Wren->SpeakerPoint->GetPendingOffer(), FName(TEXT("TestJob")));
	TestTrue(TEXT("Accepted (as the card does)"), Runner->AcceptMission(TEXT("TestJob")));
	TestTrue(TEXT("...her accept lines said"), MissionTalk::SayAccepted(World, *Job, Wren) != 0);
	Captions->Update(FCaptionQueue::CutSeconds + 0.05f);
	TestEqual(TEXT("...her offer cut short, straight to them"), CaptionNow(*Captions), FString(TEXT("Wren|Mind the rope.")));
	TestTrue(TEXT("...and she keeps the floor"), Wren->SpeakerPoint->IsTalking() && !Wren->SpeakerPoint->CanTalk());
	TestEqual(TEXT("...the rest of her offer never comes"), Captions->GetQueue().Num(), 1);
	Captions->Update(30.f);

	TestTrue(TEXT("Talked to again: the chore"), Wren->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...its words"), CaptionNow(*Captions), FString(TEXT("Wren|And the fence is down.")));
	MissionTalk::StopGiver(Wren);
	TestTrue(TEXT("Not now: she stops, the chore still offered"), !Wren->SpeakerPoint->IsTalking() && Runner->IsAvailable(TEXT("TestChore")));
	TestTrue(TEXT("...and she can be talked to again"), Wren->SpeakerPoint->CanTalk());

	TestTrue(TEXT("A board's job taken"), Runner->AcceptMission(TEXT("TestBounty")));
	Captions->Update(30.f);
	TestTrue(TEXT("...its accept remark said"), MissionTalk::SayAccepted(World, *Bounty, Board) != 0);
	TestEqual(TEXT("...in its own speaker's name"), CaptionNow(*Captions), FString(TEXT("Hob|Easy money, sunshine.")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionSideGatesTest, "Looter.Missions.MainStart.SideGates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionSideGatesTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest's side missions as their assets have them, each behind its story gate (Docs/Areas/
	// RansomsRest_Expansion.md section 2): nothing offered on a new game; after Main 5, every one whose gate is open (not
	// the Last Watch, behind Dead Lift, nor Lost and Found, behind the Old Fare). Dead Lift and Back Pay are main missions
	// since 2026-10-27 (the user's note 32: Cyrus Teague the second boss): Looter.Missions.MinePromoted.
	struct FGate
	{
		const TCHAR* Id;
		const TCHAR* After;
	};
	const FGate Gates[] = {
		{ TEXT("Side1"), TEXT("Main3") }, { TEXT("CaldersCaches"), TEXT("Main3") }, { TEXT("OldFare"), TEXT("Main3") },
		{ TEXT("LostAndFound"), TEXT("OldFare") },
		{ TEXT("Side2"), TEXT("Main4") }, { TEXT("KnockingBelow"), TEXT("Main4") },
		{ TEXT("Side3"), TEXT("Main5") }, { TEXT("LastWatch"), TEXT("DeadLift") },
	};
	TArray<UMissionDefinition*> Sides;
	for (const FGate& Gate : Gates)
	{
		UMissionDefinition* Asset = LoadMission(Gate.Id);
		if (!TestNotNull(FString::Printf(TEXT("DA_Mission_%s loads"), Gate.Id), Asset))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: a side mission offered on Ransom's Rest after %s"), Gate.Id, Gate.After),
			Asset->Kind == EMissionKind::Side && Asset->IsOffered() && Asset->Area == Rest
			&& Asset->Prerequisites.Contains(FName(Gate.After)));
		Sides.Add(Asset);
	}

	// A new game: nothing offered.
	FCampaignRecord Fresh;
	FTestWorldWrapper FreshLevel;
	if (!TestTrue(TEXT("Test level made"), FreshLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UMissionRunner* FreshRunner = FreshLevel.GetTestWorld()->GetSubsystem<UMissionRunner>();
	AActor* FreshPlayer = SpawnMarker(FreshLevel.GetTestWorld(), FVector::ZeroVector);
	if (!FreshRunner || !FreshPlayer)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	FreshRunner->BeginForTesting(Sides, Fresh, FreshPlayer, Rest);
	FreshRunner->Update(0.f);
	for (const UMissionDefinition* Side : Sides)
	{
		TestTrue(FString::Printf(TEXT("New game: %s not offered, locked"), *Side->GetMissionId().ToString()),
			!FreshRunner->IsAvailable(Side->GetMissionId()) && FreshRunner->GetStatus(*Side) == EMissionStatus::Locked);
	}

	// Main 5 turned in (and nothing else): every gate open but Dead Lift's and the Old Fare's.
	FCampaignRecord Later;
	Later.CompletedMissions = { FName(TEXT("Main1")), FName(TEXT("Main2")), FName(TEXT("Main3")), FName(TEXT("Main4")), FName(TEXT("Main5")) };
	FTestWorldWrapper LaterLevel;
	if (!TestTrue(TEXT("Second level made"), LaterLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UMissionRunner* LaterRunner = LaterLevel.GetTestWorld()->GetSubsystem<UMissionRunner>();
	AActor* LaterPlayer = SpawnMarker(LaterLevel.GetTestWorld(), FVector::ZeroVector);
	if (!LaterRunner || !LaterPlayer)
	{
		AddError(TEXT("The second level isn't whole."));
		return false;
	}
	LaterRunner->BeginForTesting(Sides, Later, LaterPlayer, Rest);
	LaterRunner->Update(0.f);
	for (const UMissionDefinition* Side : Sides)
	{
		const bool bShut = Side->Prerequisites.ContainsByPredicate([&Later](FName Each) { return !Later.CompletedMissions.Contains(Each); });
		TestEqual(FString::Printf(TEXT("After Main 5: %s %s"), *Side->GetMissionId().ToString(), bShut ? TEXT("still shut") : TEXT("offered")),
			LaterRunner->IsAvailable(Side->GetMissionId()), !bShut);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionMainStartAssetsTest, "Looter.Missions.MainStart.Assets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionMainStartAssetsTest::RunTest(const FString& Parameters)
{
	// The data as create_mission_assets.py and create_side_mission_assets.py write it (2026-10-22): Main 3 and Main 6 met
	// first at Delia's door, Dead Lift at Hiram's stool and Back Pay at the lift head's speaking tube (main since
	// 2026-10-27), the other main missions with a giver starting by themselves; the speakers' jobs with words for once
	// they're taken.
	struct FMeet
	{
		const TCHAR* Id;
		const TCHAR* Giver;
	};
	for (const FMeet& Meet : { FMeet{ TEXT("Main3"), TEXT("Speaker_Delia") }, FMeet{ TEXT("Main6"), TEXT("Speaker_Delia") },
		FMeet{ TEXT("DeadLift"), TEXT("Speaker_Hiram") }, FMeet{ TEXT("BackPay"), TEXT("Speaker_LiftTube") } })
	{
		const UMissionDefinition* Asset = LoadMission(Meet.Id);
		TestTrue(FString::Printf(TEXT("%s: met first at %s"), Meet.Id, Meet.Giver), Asset && Asset->IsMetFirst()
			&& Asset->Offer.GiverTag == FName(Meet.Giver));
	}
	for (const TCHAR* Id : { TEXT("Main2"), TEXT("Main4"), TEXT("Main5"), TEXT("Main7") })
	{
		const UMissionDefinition* Asset = LoadMission(Id);
		TestTrue(FString::Printf(TEXT("%s: starts by itself"), Id), Asset && Asset->IsMainByItself());
	}
	for (const TCHAR* Id : { TEXT("OldFare"), TEXT("Side2"), TEXT("KnockingBelow"), TEXT("DeadLift"), TEXT("BackPay"), TEXT("LostAndFound") })
	{
		const UMissionDefinition* Asset = LoadMission(Id);
		TestTrue(FString::Printf(TEXT("%s: offer words and accept words"), Id), Asset && !Asset->Offer.Lines.IsEmpty()
			&& !Asset->Offer.AcceptLines.IsEmpty());
	}
	return true;
}

#endif
