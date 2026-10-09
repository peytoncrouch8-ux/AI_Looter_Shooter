#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Progression/XPCurve.h"
#include "Scenes/ColdOpen.h"
#include "Scenes/GraveWake.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/HobBird.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "Story/StoryLineSet.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/RespawnMarker.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	// The tags Main 1 finds its places by (Tools/Unreal/build_area_story.py tags them so).
	const FName AbelsHeadboard(TEXT("Headboard_Abel"));
	const FName DeliasDoor(TEXT("Speaker_Delia"));
	const FName MainOne(TEXT("Main1"));

	/** The words on Abel's headboard, and Delia's at the door (Docs/Story.md: Cold open, 3; Docs/Areas/RansomsRest.md: Main 1). */
	const TCHAR* AbelsBoard = TEXT("ABEL RANSOM. KEEPER OF SAINT ADA. HE HELD THE DOOR.");
	const TCHAR* const DeliasWords[] = {
		TEXT("I was to lay you on the boards tonight."),
		TEXT("Your Pa got up Wednesday night, came up through the dirt like it was fog."),
		TEXT("A keeper doesn't lie still while his saint is dark. He walks the boards at dusk."),
	};
	const TCHAR* HobsMorning = TEXT("Morning, sunshine. Most folks who get up do it after dark.");

	FStoryLine Said(const TCHAR* Speaker, const TCHAR* Words, float Seconds = 3.f)
	{
		return FStoryLine::Make(FText::FromString(Speaker), FText::FromString(Words), Seconds);
	}

	/**
	 * Main 1 as the mission script makes it (Tools/Unreal/create_mission_assets.py), in code: four steps, turned in at
	 * Delia's door (the talk that was its fifth step), 20 experience.
	 */
	UMissionDefinition* MakeSevenDays(UObject* Outer, const TCHAR* Id, FName Area)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, Id, EMissionKind::Main, EMissionStart::Automatic, Area);
		MissionTestWorld::AddObjective<UMissionSceneObjective>(Mission, 0)->Scene = ColdOpen::SceneName();
		MissionTestWorld::AddObjective<UMissionSceneObjective>(Mission, 1)->Scene = GraveWake::SceneName();
		MissionTestWorld::AddObjective<UMissionInteractObjective>(Mission, 2)->Target.ActorTag = AbelsHeadboard;
		UMissionReachObjective* Farmhouse = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 3);
		Farmhouse->Place.Actor.ActorTag = DeliasDoor;
		Farmhouse->Place.Radius = 900.f;
		Mission->TurnIn.SpeakerTag = DeliasDoor;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("Delia"));
		Mission->Rewards.Experience = 20;
		return Mission;
	}

	/** A speaker point placed as the build script places one: its point at the actor's spot, facing the origin. */
	ASpeakerPoint* PlacePoint(UWorld* World, const FVector& Where, FName Tag, const TCHAR* Name, const TArray<FStoryLine>& Lines)
	{
		ASpeakerPoint* Point = World->SpawnActor<ASpeakerPoint>(Where, FRotator(0.0, (-Where).Rotation().Yaw, 0.0));
		if (!Point)
		{
			return nullptr;
		}
		Point->SpeakerPoint->SetRelativeLocation(FVector::ZeroVector);
		Point->SpeakerPoint->SpeakerName = FText::FromString(Name);
		Point->SpeakerPoint->Lines = Lines;
		Point->Tags.Add(Tag);
		Point->DispatchBeginPlay();
		return Point;
	}

	FString OnScreen(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT("|") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}

	/** A line set the story script makes, when it's in this checkout. */
	const UStoryLineSet* LoadLines(const TCHAR* Name)
	{
		const FString Package = FString(TEXT("/Game/Data/Story/")) + Name;
		return FPackageName::DoesPackageExist(Package) ? LoadObject<UStoryLineSet>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSevenDaysMissionTest, "Looter.Story.SevenDays.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSevenDaysMissionTest::RunTest(const FString& Parameters)
{
	// Main 1 as its asset has it, once the mission script has made it: the cold open, the claw-out, Abel's headboard, the
	// farmhouse; turned in at Delia's door (her talk, which was its fifth step); a main mission that starts by itself on
	// Ransom's Rest, worth 20 experience.
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main1")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main1.DA_Mission_Main1"));
		if (TestNotNull(TEXT("DA_Mission_Main1 loads"), Asset))
		{
			TestTrue(TEXT("Main1, a main mission starting by itself on Ransom's Rest, after nothing"), Asset->GetMissionId() == MainOne
				&& Asset->Kind == EMissionKind::Main && Asset->Start == EMissionStart::Automatic && Asset->Area == FName(TEXT("RansomsRest"))
				&& Asset->Prerequisites.IsEmpty());
			TestEqual(TEXT("Four steps"), Asset->Steps.Num(), 4);
			const UMissionSceneObjective* Opening = Cast<UMissionSceneObjective>(Asset->GetObjective(0, 0));
			const UMissionSceneObjective* Claw = Cast<UMissionSceneObjective>(Asset->GetObjective(1, 0));
			const UMissionInteractObjective* Read = Cast<UMissionInteractObjective>(Asset->GetObjective(2, 0));
			const UMissionReachObjective* Go = Cast<UMissionReachObjective>(Asset->GetObjective(3, 0));
			TestTrue(TEXT("1: the cold open"), Opening && Opening->Scene == ColdOpen::SceneName());
			TestTrue(TEXT("2: the claw-out"), Claw && Claw->Scene == GraveWake::SceneName());
			TestTrue(TEXT("3: read Abel's headboard"), Read && Read->Target.ActorTag == AbelsHeadboard);
			TestTrue(TEXT("4: go up to the farmhouse (Delia's door)"), Go && Go->Place.Actor.ActorTag == DeliasDoor && Go->Place.Radius >= 300.f);
			TestTrue(TEXT("Turned in to Delia at her door"), Asset->NeedsTurnIn() && Asset->TurnIn.SpeakerTag == DeliasDoor);
			TestEqual(TEXT("Its reward: 20 experience"), Asset->Rewards.Experience, 20);
		}
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Main1 isn't made yet: run Tools/Unreal/create_mission_assets.py. The flow below runs on a copy."));
	}

	// Played through in a test level: the scenes, the headboard read with the Interact key, the walk up to the farmhouse,
	// the talk at the door; then the family plot's grave is open.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	USceneSubsystem* Scenes = World->GetSubsystem<USceneSubsystem>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	ASpeakerPoint* Headboard = PlacePoint(World, InteractionTestWorld::Ahead(150.0), AbelsHeadboard, TEXT(""), { Said(TEXT(""), AbelsBoard) });
	ASpeakerPoint* Door = PlacePoint(World, FVector(0.0, 3000.0, InteractionTestWorld::EyeHeight), DeliasDoor, TEXT("Grandma Delia"),
		{ Said(TEXT(""), DeliasWords[0]) });
	if (!Runner || !Scenes || !Captions || !Player || !Interaction || !Headboard || !Door)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Headboard->SpeakerPoint->Prompt = FText::FromString(TEXT("Read"));
	const FName Valley(TEXT("TestValley"));
	Runner->BeginForTesting({ MakeSevenDays(CreatePackage(nullptr), TEXT("TestMain1"), Valley) }, Campaign, Player, Valley);
	ARespawnMarker* Grave = World->SpawnActor<ARespawnMarker>(FVector(-200.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The family plot's grave"), Grave))
	{
		return false;
	}
	Grave->MarkerId = TEXT("FamilyPlot");
	Grave->ActiveAfterMission = TEXT("TestMain1");
	Grave->DispatchBeginPlay();
	Runner->Update(0.f);
	TestEqual(TEXT("It starts by itself, at the cold open"), Runner->GetStep(TEXT("TestMain1")), 0);
	TestFalse(TEXT("The family plot is closed"), Grave->IsActive(Campaign));

	Scenes->MarkPlayed(ColdOpen::SceneName());
	Scenes->MarkPlayed(GraveWake::SceneName());
	TestEqual(TEXT("Both scenes played: on to the headboard"), Runner->GetStep(TEXT("TestMain1")), 2);

	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Abel's headboard is what's looked at, to read"), Interaction->GetFocusedActor() == Headboard
		&& Interaction->GetFocusedOptions().TapPrompt.ToString() == TEXT("Read"));
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestEqual(TEXT("Read: its words on screen, no one's"), OnScreen(*Captions), FString(TEXT("|")) + AbelsBoard);
	TestEqual(TEXT("...and on to the farmhouse"), Runner->GetStep(TEXT("TestMain1")), 3);

	Runner->Update(0.2f);
	TestEqual(TEXT("Far from the door, still on the way"), Runner->GetStep(TEXT("TestMain1")), 3);
	Player->SetActorLocation(FVector(0.0, 2400.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("At the farmhouse: on to Delia (its step past the last)"), Runner->GetStep(TEXT("TestMain1")), 4);
	TestTrue(TEXT("...ready to turn in to her, not finished"), Runner->IsReadyToTurnIn(TEXT("TestMain1")) && !Campaign.HasCompleted(TEXT("TestMain1")));
	TestFalse(TEXT("...the family plot still closed"), Grave->IsActive(Campaign));

	Captions->Update(10.f);
	TestTrue(TEXT("Talked to at her door"), Door->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("Main 1 is turned in: finished"), Campaign.HasCompleted(TEXT("TestMain1")));
	TestTrue(TEXT("...and the family plot is a respawn grave"), Grave->IsActive(Campaign) && Campaign.IsRespawnActive(TEXT("FamilyPlot")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSevenDaysLinesTest, "Looter.Story.SevenDays.Lines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSevenDaysLinesTest::RunTest(const FString& Parameters)
{
	// The words, as the story script writes them into their line sets (when it has): Delia's at the door during Main 1,
	// the two headboards, Hob's landing.
	if (const UStoryLineSet* Delia = LoadLines(TEXT("DA_Lines_DeliaMain1")))
	{
		bool bSame = Delia->Lines.Num() == static_cast<int32>(UE_ARRAY_COUNT(DeliasWords));
		for (int32 Index = 0; bSame && Index < Delia->Lines.Num(); ++Index)
		{
			bSame = Delia->Lines[Index].Text.ToString() == DeliasWords[Index] && Delia->Lines[Index].Speaker.IsEmpty();
		}
		TestTrue(TEXT("Delia's Main 1 lines, hers by her door's name"), bSame);
	}
	else
	{
		AddWarning(TEXT("The story's line sets aren't made yet: run Tools/Unreal/create_story_lines.py."));
	}
	if (const UStoryLineSet* Board = LoadLines(TEXT("DA_Lines_HeadboardAbel")))
	{
		TestTrue(TEXT("Abel's headboard reads as carved, said by no one"), !Board->Lines.IsEmpty() && Board->Lines[0].Text.ToString() == AbelsBoard
			&& Board->Lines[0].Speaker.IsEmpty());
	}
	if (const UStoryLineSet* Mine = LoadLines(TEXT("DA_Lines_HeadboardEllis")))
	{
		TestTrue(TEXT("Ellis's own reads CAME HOME AT THE LAST"), !Mine->Lines.IsEmpty() && Mine->Lines[0].Text.ToString().Contains(TEXT("CAME HOME AT THE LAST")));
	}
	if (const UStoryLineSet* Hob = LoadLines(TEXT("DA_Lines_HobWakes")))
	{
		TestTrue(TEXT("Hob's landing line"), !Hob->Lines.IsEmpty() && Hob->Lines[0].Text.ToString() == HobsMorning);
	}

	// A step of a mission as a story condition: Hob's perch is his from Main 1's third step (counted from 0: 2).
	FCampaignRecord Campaign;
	FStoryCondition FromHeadboard;
	FromHeadboard.DuringMission = TEXT("TestMain1");
	FromHeadboard.FromStep = 2;
	Campaign.ActiveMission = TEXT("TestMain1");
	Campaign.ActiveMissionStep = 1;
	TestFalse(TEXT("At the claw-out: not yet"), FromHeadboard.IsMet(Campaign));
	Campaign.ActiveMissionStep = 2;
	TestTrue(TEXT("At the headboard: his"), FromHeadboard.IsMet(Campaign));
	TestTrue(TEXT("Said for people from 1"), FromHeadboard.Describe().Contains(TEXT("from step 3")));

	// In a test level: Delia says her Main 1 lines while it lasts and the porch line after; Hob is away until the claw-out
	// is done, then flies in to Ellis's headboard and says his piece, and stays once Main 1 is finished.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	AActor* Listener = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	ASpeakerPoint* Door = PlacePoint(World, FVector(300.0, 0.0, InteractionTestWorld::EyeHeight), DeliasDoor, TEXT("Grandma Delia"),
		{ Said(TEXT(""), TEXT("There's a plate on the porch.")) });
	AHobBird* Hob = World->SpawnActor<AHobBird>(FVector(0.0, 500.0, 0.0), FRotator::ZeroRotator);
	if (!Runner || !Captions || !Listener || !Door || !Hob)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	FSpeakerTopic DuringMainOne;
	DuringMainOne.When.BeforeMissions = { FName(TEXT("TestMain1")) };
	for (const TCHAR* Words : DeliasWords)
	{
		DuringMainOne.Lines.Add(Said(TEXT(""), Words));
	}
	Door->SpeakerPoint->Topics = { DuringMainOne };

	const FVector Board(-3510.0, -8945.0, 420.0);
	FHobPerch OnTheBoard;
	OnTheBoard.When = FromHeadboard;
	OnTheBoard.Location = Board;
	OnTheBoard.Yaw = 90.f;
	OnTheBoard.Arrival = { Said(TEXT(""), HobsMorning) };
	FHobPerch AfterMainOne;
	AfterMainOne.When.AfterMissions = { FName(TEXT("TestMain1")) };
	AfterMainOne.Location = Board;
	AfterMainOne.Yaw = 90.f;
	Hob->Perches = { OnTheBoard, AfterMainOne };

	const FName Valley(TEXT("TestValley"));
	FCampaignRecord Story;
	Runner->BeginForTesting({ MakeSevenDays(CreatePackage(nullptr), TEXT("TestMain1"), Valley) }, Story, Listener, Valley);
	Runner->Update(0.f);
	Hob->DispatchBeginPlay();
	TestTrue(TEXT("During the cold open and the claw-out Hob is away"), !Hob->IsShown() && Hob->GetPerch() == INDEX_NONE);

	int32 Topic = INDEX_NONE;
	const TArray<FStoryLine> Now = Door->SpeakerPoint->GetLinesNow(&Topic);
	TestTrue(TEXT("Delia's Main 1 lines, three, in her name"), Topic == 0 && Now.Num() == 3 && Now[0].Speaker.ToString() == TEXT("Grandma Delia")
		&& Now[2].Text.ToString() == DeliasWords[2]);

	Runner->SetStep(TEXT("TestMain1"), 2);
	TestTrue(TEXT("Past the claw-out: Hob flies in"), Hob->IsShown() && Hob->GetPerch() == 0 && Hob->IsFlying());
	// A flight lasts as long as its way needs at his speed, within the shortest and longest: dropping in from just above
	// and behind the board is a hop.
	TestEqual(TEXT("...a short drop onto the board, the shortest flight"), Hob->GetFlightSeconds(), Hob->MinFlightSeconds, 1e-3f);
	TestEqual(TEXT("A flight three seconds long at his speed takes three"), Hob->GetFlightSecondsFor(Hob->FlightSpeed * 3.f), 3.f, 1e-3f);
	TestEqual(TEXT("Across the valley he flies no longer than the longest"), Hob->GetFlightSecondsFor(1.0e6f), Hob->MaxFlightSeconds, 1e-3f);
	Hob->FinishFlight();
	TestTrue(TEXT("...and lands on Ellis's headboard"), !Hob->IsFlying() && Hob->GetActorLocation().Equals(Board, 0.5));
	TestEqual(TEXT("...saying his piece"), OnScreen(*Captions), FString(TEXT("Hob|")) + HobsMorning);

	Runner->CompleteMission(TEXT("TestMain1"));
	TestTrue(TEXT("Main 1 done: Hob stays where he is, no flight"), Hob->IsShown() && Hob->GetPerch() == 1 && !Hob->IsFlying()
		&& Hob->GetActorLocation().Equals(Board, 0.5));
	Captions->Update(20.f);
	TestTrue(TEXT("Delia, after Main 1: the porch"), Door->SpeakerPoint->Talk(Listener)
		&& OnScreen(*Captions) == TEXT("Grandma Delia|There's a plate on the porch."));
	return true;
}

#endif
