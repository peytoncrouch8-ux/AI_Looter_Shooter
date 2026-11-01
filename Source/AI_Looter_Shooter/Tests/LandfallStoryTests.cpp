#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Landfall, Main 1 since 2026-10-31 (the Wardens story; was Seven Days, whose tests this file held): its mission asset as
// Tools/Unreal/create_mission_assets.py makes it, played through on a copy in a test level, and its words as
// Tools/Unreal/create_story_lines.py writes them. The step table is Docs/Work/2026-10-31-Wardens/CONTRACT.md's.
// Looter.Story.Landfall.Flight.* (FLIGHT) covers the ship, the grapples, the landing and the old-save move.

#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "Story/StoryLineSet.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	// What Landfall finds its things by (the contract's step table, exact; the mission script's constants).
	const FName LandfallId(TEXT("Main1"));
	const FName ProwTag(TEXT("Lamplighter_Prow"));
	const FName DeckClearEvent(TEXT("Landfall.DeckClear"));
	const FName GrappleTag(TEXT("Landfall_Grapple"));
	const FName HardLandingScene(TEXT("HardLanding"));
	const FName WinchTag(TEXT("Landfall_AnchorWinch"));
	const FName LedgeClimbersId(TEXT("LedgeClimbers"));
	const FName RansomsPointTag(TEXT("Place_RansomsPoint"));
	const FName SpyglassTag(TEXT("Landfall_Spyglass"));
	const FName DeliaTag(TEXT("Speaker_Delia"));
	const FName HatTag(TEXT("Landfall_AbelsHat"));
	const FName FarmPorchId(TEXT("FarmPorch"));
	/** Abel's hat picked up (its readable's SetsFlag, story_texts.ABELS_HAT_FLAG): Delia's hat lines at the turn-in. */
	const FName HatFlag(TEXT("Landfall.AbelsHat"));
	constexpr int32 LandfallSteps = 9;
	/** Ready to turn in: the step past the last (Delia's turn-in topics' from_step). */
	constexpr int32 ReadyStep = LandfallSteps;

	/** The first words of Delia's three topics (story_texts.TOPICS['Speaker_Delia']; their sets' first lines). */
	const TCHAR* DeliaHatFirst = TEXT("...That's his hat. Give it here.");
	const TCHAR* DeliaPlainFirst = TEXT("So the Wardens came. A week late, but you came.");
	const TCHAR* DeliaWaitFirst = TEXT("Talk after! Shoot first!");

	/** Captions are short (the talk cut's rule: said on the move), so a line reads at a glance. */
	constexpr int32 LongestCaption = 90;

	FStoryLine Said(const TCHAR* Speaker, const TCHAR* Words, float Seconds = 3.f)
	{
		return FStoryLine::Make(FText::FromString(Speaker), FText::FromString(Words), Seconds);
	}

	/** An objective of type T in step Step, with its tracker words. */
	template <typename T>
	T* AddShort(UMissionDefinition* Mission, int32 Step, const TCHAR* Short)
	{
		T* Objective = MissionTestWorld::AddObjective<T>(Mission, Step);
		Objective->ShortText = FText::FromString(Short);
		return Objective;
	}

	/** Landfall as the mission script makes it (create_mission_assets.py main1_steps), in code: nine steps, Delia's turn-in. */
	UMissionDefinition* MakeLandfall(UObject* Outer, const TCHAR* Id, FName Area)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, Id, EMissionKind::Main, EMissionStart::Automatic, Area);
		UMissionReachObjective* Prow = AddShort<UMissionReachObjective>(Mission, 0, TEXT("Go to the Great Lantern at the prow"));
		Prow->Place.Actor.ActorTag = ProwTag;
		Prow->Place.Radius = 350.f;
		AddShort<UMissionEventObjective>(Mission, 1, TEXT("Fight off Vane's boarders"))->Event = DeckClearEvent;
		UMissionInteractObjective* Grapples = AddShort<UMissionInteractObjective>(Mission, 2, TEXT("Cut the grapple lines"));
		Grapples->Target.ActorTag = GrappleTag;
		Grapples->Count = 2;
		Grapples->bHold = true;
		AddShort<UMissionSceneObjective>(Mission, 3, TEXT("Hold on"))->Scene = HardLandingScene;
		UMissionInteractObjective* Winch = AddShort<UMissionInteractObjective>(Mission, 4, TEXT("Drop the anchor"));
		Winch->Target.ActorTag = WinchTag;
		Winch->bHold = true;
		UMissionClearObjective* Climbers = AddShort<UMissionClearObjective>(Mission, 4, TEXT("Fight off the dead"));
		Climbers->SpawnerId = LedgeClimbersId;
		Climbers->Count = 4;
		UMissionReachObjective* Point = AddShort<UMissionReachObjective>(Mission, 5, TEXT("Climb to Ransom's Point"));
		Point->Place.Actor.ActorTag = RansomsPointTag;
		Point->Place.Radius = 1800.f;
		Point->Place.bIgnoreHeight = false;
		AddShort<UMissionInteractObjective>(Mission, 6, TEXT("Look over the Rest"))->Target.ActorTag = SpyglassTag;
		UMissionReachObjective* Trail = AddShort<UMissionReachObjective>(Mission, 7, TEXT("Follow Abel's trail"));
		Trail->Place.Actor.ActorTag = DeliaTag;
		Trail->Place.Radius = 1800.f;
		UMissionInteractObjective* Hat = AddShort<UMissionInteractObjective>(Mission, 7, TEXT("Pick up Abel's hat"));
		Hat->Target.ActorTag = HatTag;
		Hat->bOptional = true;
		UMissionClearObjective* Porch = AddShort<UMissionClearObjective>(Mission, 8, TEXT("Help Delia hold the porch"));
		Porch->SpawnerId = FarmPorchId;
		Porch->Count = 8;
		Mission->TurnIn.SpeakerTag = DeliaTag;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("Delia Ransom"));
		Mission->Rewards.Experience = 450;
		return Mission;
	}

	/** Delia's topics as story_texts.TOPICS['Speaker_Delia'] lists them, for MissionId: the hat's turn-in, the plain one, mid-fight. */
	TArray<FSpeakerTopic> DeliasTopics(FName MissionId)
	{
		FSpeakerTopic Hat;
		Hat.When.DuringMission = MissionId;
		Hat.When.FromStep = ReadyStep;
		Hat.When.WithFlags = { HatFlag };
		Hat.Lines = { Said(TEXT(""), DeliaHatFirst) };
		FSpeakerTopic Plain;
		Plain.When.DuringMission = MissionId;
		Plain.When.FromStep = ReadyStep;
		Plain.Lines = { Said(TEXT(""), DeliaPlainFirst) };
		FSpeakerTopic Wait;
		Wait.When.DuringMission = MissionId;
		Wait.Lines = { Said(TEXT(""), DeliaWaitFirst) };
		return { Hat, Plain, Wait };
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

	/** How far the first (optional, or not) objective of the step a mission is at has come: 0 to 1, -1 with none such. */
	float ShareDone(const UMissionRunner& Runner, FName MissionId, bool bOptional)
	{
		const TArray<FMissionObjectiveView> Views = Runner.GetObjectiveViews(MissionId);
		const FMissionObjectiveView* View = Views.FindByPredicate([bOptional](const FMissionObjectiveView& Each) { return Each.bOptional == bOptional; });
		return View ? (View->bDone ? 1.f : View->Fraction) : -1.f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandfallMissionTest, "Looter.Story.Landfall.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLandfallMissionTest::RunTest(const FString& Parameters)
{
	// Main 1 as its asset has it, once the mission script has made it: the contract's nine steps, each objective of its kind
	// with its exact tag, the hat optional, turned in to Delia Ransom with no lines of its own (her topics say them, so the
	// hat can choose), a main mission that starts by itself on Ransom's Rest (the one no giver offers), 450 and her 20.
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main1")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main1.DA_Mission_Main1"));
		if (TestNotNull(TEXT("DA_Mission_Main1 loads"), Asset))
		{
			TestTrue(TEXT("Main1, a main mission starting by itself on Ransom's Rest, after nothing"), Asset->GetMissionId() == LandfallId
				&& Asset->Kind == EMissionKind::Main && Asset->Start == EMissionStart::Automatic && Asset->Area == FName(TEXT("RansomsRest"))
				&& Asset->Prerequisites.IsEmpty());
			TestEqual(TEXT("Titled Landfall"), Asset->Title.ToString(), FString(TEXT("Landfall")));
			if (TestEqual(TEXT("Nine steps"), Asset->Steps.Num(), LandfallSteps))
			{
				const UMissionReachObjective* Prow = Cast<UMissionReachObjective>(Asset->GetObjective(0, 0));
				const UMissionEventObjective* Deck = Cast<UMissionEventObjective>(Asset->GetObjective(1, 0));
				const UMissionInteractObjective* Grapples = Cast<UMissionInteractObjective>(Asset->GetObjective(2, 0));
				const UMissionSceneObjective* Landing = Cast<UMissionSceneObjective>(Asset->GetObjective(3, 0));
				const UMissionInteractObjective* Winch = Cast<UMissionInteractObjective>(Asset->GetObjective(4, 0));
				const UMissionClearObjective* Climbers = Cast<UMissionClearObjective>(Asset->GetObjective(4, 1));
				const UMissionReachObjective* Point = Cast<UMissionReachObjective>(Asset->GetObjective(5, 0));
				const UMissionInteractObjective* Spyglass = Cast<UMissionInteractObjective>(Asset->GetObjective(6, 0));
				const UMissionReachObjective* Trail = Cast<UMissionReachObjective>(Asset->GetObjective(7, 0));
				const UMissionInteractObjective* Hat = Cast<UMissionInteractObjective>(Asset->GetObjective(7, 1));
				const UMissionClearObjective* Porch = Cast<UMissionClearObjective>(Asset->GetObjective(8, 0));
				TestTrue(TEXT("0: the Great Lantern at the prow, within 3.5 m"), Prow && Prow->Place.Actor.ActorTag == ProwTag
					&& FMath::IsNearlyEqual(Prow->Place.Radius, 350.f));
				TestTrue(TEXT("1: Vane's boarders (Landfall.DeckClear, once)"), Deck && Deck->Event == DeckClearEvent && Deck->Count == 1);
				TestTrue(TEXT("2: two grapple lines, each held"), Grapples && Grapples->Target.ActorTag == GrappleTag && Grapples->Count == 2
					&& Grapples->bHold);
				TestTrue(TEXT("3: the hard landing"), Landing && Landing->Scene == HardLandingScene);
				TestTrue(TEXT("4: the anchor winch, held"), Winch && Winch->Target.ActorTag == WinchTag && Winch->bHold && !Winch->bOptional);
				TestTrue(TEXT("...and the dead off the ledge (LedgeClimbers, 4)"), Climbers && Climbers->SpawnerId == LedgeClimbersId
					&& Climbers->Count == 4 && !Climbers->bOptional);
				TestTrue(TEXT("5: Ransom's Point, its height counted"), Point && Point->Place.Actor.ActorTag == RansomsPointTag
					&& !Point->Place.bIgnoreHeight);
				TestTrue(TEXT("6: the spyglass"), Spyglass && Spyglass->Target.ActorTag == SpyglassTag && Spyglass->Count == 1);
				TestTrue(TEXT("7: Abel's trail to Delia, within 18 m"), Trail && Trail->Place.Actor.ActorTag == DeliaTag
					&& FMath::IsNearlyEqual(Trail->Place.Radius, 1800.f) && !Trail->bOptional);
				TestTrue(TEXT("...and, optional, Abel's hat"), Hat && Hat->bOptional && Hat->Target.ActorTag == HatTag);
				TestTrue(TEXT("8: Delia's porch held (FarmPorch, 8)"), Porch && Porch->SpawnerId == FarmPorchId && Porch->Count == 8);
				// Every objective has the tracker's words (the contract's), and every one says why in its full sentence.
				for (int32 Step = 0; Step < Asset->Steps.Num(); ++Step)
				{
					for (const TObjectPtr<UMissionObjective>& Objective : Asset->Steps[Step].Objectives)
					{
						TestTrue(FString::Printf(TEXT("Step %d: its tracker line and its sentence"), Step), Objective
							&& !Objective->ShortText.IsEmpty() && Objective->Text.ToString().Len() > Objective->ShortText.ToString().Len());
					}
				}
			}
			TestTrue(TEXT("Turned in to Delia Ransom on her porch"), Asset->NeedsTurnIn() && Asset->TurnIn.SpeakerTag == DeliaTag
				&& Asset->TurnIn.GiverName.ToString() == TEXT("Delia Ransom"));
			TestTrue(TEXT("...no lines of its own: her topics say them (the hat's or not)"), Asset->TurnIn.Lines.IsEmpty()
				&& Asset->TurnIn.Choices.IsEmpty());
			TestEqual(TEXT("Its reward: 450 experience"), Asset->Rewards.Experience, 450);
			TestEqual(TEXT("...and Delia's purse, 20 silver"), MissionRewards::PurseOf(Asset->Rewards, LandfallId), 20);
		}
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Main1 isn't made yet: run Tools/Unreal/create_mission_assets.py. The flow below runs on a copy."));
	}

	// Played through on a copy in a test level: the walk to the prow, the deck cleared, the grapples cut only by holding,
	// the landing, the anchor held (the fights' clears stood in for: their spawners are WORLD's), the Point, the spyglass,
	// the trail with the hat picked up, the porch; then the talk on the porch turns it in with the hat's words.
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
	AActor* Prow = MissionTestWorld::SpawnMarker(World, FVector(2000.0, 0.0, 0.0), ProwTag);
	AActor* GrappleA = MissionTestWorld::SpawnMarker(World, FVector(1000.0, 400.0, 0.0), GrappleTag);
	AActor* GrappleB = MissionTestWorld::SpawnMarker(World, FVector(1000.0, -400.0, 0.0), GrappleTag);
	AActor* Winch = MissionTestWorld::SpawnMarker(World, FVector(0.0, 600.0, 0.0), WinchTag);
	AActor* Point = MissionTestWorld::SpawnMarker(World, FVector(0.0, -8000.0, 3000.0), RansomsPointTag);
	AActor* Spyglass = MissionTestWorld::SpawnMarker(World, FVector(100.0, -8000.0, 3000.0), SpyglassTag);
	AActor* Hat = MissionTestWorld::SpawnMarker(World, FVector(3000.0, -6000.0, 1500.0), HatTag);
	ASpeakerPoint* Delia = PlacePoint(World, FVector(6000.0, -4000.0, InteractionTestWorld::EyeHeight), DeliaTag, TEXT("Delia Ransom"),
		{ Said(TEXT(""), TEXT("Go on, Warden. I'll keep the porch.")) });
	if (!Runner || !Scenes || !Captions || !Player || !Interaction || !Prow || !GrappleA || !GrappleB || !Winch || !Point || !Spyglass
		|| !Hat || !Delia)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	const FName CopyId(TEXT("TestMain1"));
	Delia->SpeakerPoint->Topics = DeliasTopics(CopyId);
	const FName Valley(TEXT("TestValley"));
	Runner->BeginForTesting({ MakeLandfall(CreatePackage(nullptr), TEXT("TestMain1"), Valley) }, Campaign, Player, Valley);
	Runner->Update(0.f);
	TestEqual(TEXT("It starts by itself, on deck, at the prow's step"), Runner->GetStep(CopyId), 0);

	Player->SetActorLocation(FVector(1600.0, 0.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("4 m from the Lantern: not there yet"), Runner->GetStep(CopyId), 0);
	Player->SetActorLocation(FVector(1700.0, 0.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("At the prow: on to the boarders"), Runner->GetStep(CopyId), 1);

	Runner->NotifyEvent(FMissionEvent::Named(DeckClearEvent));
	TestEqual(TEXT("The deck clear: on to the grapples"), Runner->GetStep(CopyId), 2);

	Runner->NotifyEvent(FMissionEvent::Interaction(GrappleA, /*bHeld*/ false));
	TestTrue(TEXT("A tap on a grapple cuts nothing"), Runner->GetStep(CopyId) == 2 && ShareDone(*Runner, CopyId, false) == 0.f);
	Runner->NotifyEvent(FMissionEvent::Interaction(GrappleA, /*bHeld*/ true));
	Runner->NotifyEvent(FMissionEvent::Interaction(GrappleA, /*bHeld*/ true));
	TestTrue(TEXT("One held cut, counted once (one of two)"), Runner->GetStep(CopyId) == 2
		&& FMath::IsNearlyEqual(ShareDone(*Runner, CopyId, false), 0.5f));
	Runner->NotifyEvent(FMissionEvent::Interaction(GrappleB, /*bHeld*/ true));
	TestEqual(TEXT("Both cut: the hard landing"), Runner->GetStep(CopyId), 3);

	Scenes->MarkPlayed(HardLandingScene);
	TestEqual(TEXT("Landed: on to the anchor"), Runner->GetStep(CopyId), 4);
	Runner->NotifyEvent(FMissionEvent::Interaction(Winch, /*bHeld*/ true));
	TestTrue(TEXT("The anchor down, the dead not yet: still the ledge"), Runner->GetStep(CopyId) == 4
		&& Runner->GetObjectiveViews(CopyId).ContainsByPredicate([](const FMissionObjectiveView& View) { return View.bDone; }));
	// The ledge's fight cleared (its spawner is WORLD's; Looter.Missions covers the clear objective).
	Runner->CompleteStep(CopyId);
	TestEqual(TEXT("Moored and the ledge clear: the climb"), Runner->GetStep(CopyId), 5);

	Player->SetActorLocation(FVector(0.0, -8000.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("Under the Point, 30 m below its top: the height counts"), Runner->GetStep(CopyId), 5);
	Player->SetActorLocation(FVector(0.0, -8000.0, 3000.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("On the Point: the spyglass"), Runner->GetStep(CopyId), 6);
	Runner->NotifyEvent(FMissionEvent::Interaction(Spyglass, /*bHeld*/ false));
	TestEqual(TEXT("Looked through it (a tap does): the trail"), Runner->GetStep(CopyId), 7);

	// The hat, on the way down: its readable sets the flag as it's read, and the interaction counts the optional.
	Runner->SetFlag(HatFlag);
	Runner->NotifyEvent(FMissionEvent::Interaction(Hat, /*bHeld*/ true));
	TestTrue(TEXT("Abel's hat picked up: the optional done, the trail still to follow"), Runner->GetStep(CopyId) == 7
		&& ShareDone(*Runner, CopyId, true) == 1.f);
	Player->SetActorLocation(FVector(6000.0, -2400.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("16 m from Delia: the porch's fight"), Runner->GetStep(CopyId), 8);

	Captions->Update(10.f);
	TestTrue(TEXT("Talked to mid-fight, she says shoot first"), Delia->SpeakerPoint->Talk(Player)
		&& OnScreen(*Captions) == FString(TEXT("Delia Ransom|")) + DeliaWaitFirst);
	TestFalse(TEXT("...and nothing is turned in"), Campaign.HasCompleted(CopyId));
	Runner->CompleteStep(CopyId);
	TestTrue(TEXT("The porch held: ready to turn in to her"), Runner->IsReadyToTurnIn(CopyId) && Runner->GetStep(CopyId) == ReadyStep
		&& !Campaign.HasCompleted(CopyId));

	Captions->Update(10.f);
	TestTrue(TEXT("Talked to on her porch"), Delia->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...the hat's words (her best line first)"), OnScreen(*Captions), FString(TEXT("Delia Ransom|")) + DeliaHatFirst);
	TestTrue(TEXT("Landfall is turned in: finished"), Campaign.HasCompleted(CopyId));
	TestTrue(TEXT("...and the hat's flag stays for later"), Campaign.HasFlag(HatFlag));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandfallLinesTest, "Looter.Story.Landfall.Lines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLandfallLinesTest::RunTest(const FString& Parameters)
{
	// Every line set the contract lists, as the story script writes it (when it has): there, short, each line with words,
	// the right voices (Vane's all his; Delia's barks and turn-ins hers), the counts asked for.
	struct FSetRule
	{
		const TCHAR* Name;
		int32 Fewest;
		int32 Most;
		/** Every line names this speaker (empty: any; "-": none, said in the speaker point's name). */
		const TCHAR* Speaker;
	};
	const FSetRule Rules[] = {
		{ TEXT("DA_Lines_LandfallDeck"), 4, 6, TEXT("") },
		{ TEXT("DA_Lines_LandfallVaneHorn"), 2, 4, TEXT("Cornelius Vane") },
		{ TEXT("DA_Lines_LandfallBoarders"), 2, 4, TEXT("") },
		{ TEXT("DA_Lines_LandfallBoardersWave2"), 1, 3, TEXT("") },
		{ TEXT("DA_Lines_LandfallGrapples"), 2, 4, TEXT("") },
		{ TEXT("DA_Lines_LandfallLanding"), 1, 3, TEXT("") },
		{ TEXT("DA_Lines_LandfallAnchor"), 2, 4, TEXT("") },
		{ TEXT("DA_Lines_LandfallClimb"), 2, 4, TEXT("") },
		{ TEXT("DA_Lines_LandfallPoint"), 1, 3, TEXT("") },
		{ TEXT("DA_Lines_LandfallVaneValley"), 2, 5, TEXT("Cornelius Vane") },
		{ TEXT("DA_Lines_LandfallTrail"), 2, 4, TEXT("") },
		{ TEXT("DA_Lines_HobLandfallPorch"), 1, 2, TEXT("Hob") },
		{ TEXT("DA_Lines_DeliaFight"), 6, 8, TEXT("Delia Ransom") },
		{ TEXT("DA_Lines_DeliaLandfallWait"), 1, 2, TEXT("-") },
		{ TEXT("DA_Lines_DeliaLandfall"), 3, 5, TEXT("-") },
		{ TEXT("DA_Lines_DeliaLandfallHat"), 3, 5, TEXT("-") },
	};
	int32 Found = 0;
	for (const FSetRule& Rule : Rules)
	{
		const UStoryLineSet* Set = LoadLines(Rule.Name);
		if (!Set)
		{
			continue;
		}
		++Found;
		TestTrue(FString::Printf(TEXT("%s: %d to %d lines (%d)"), Rule.Name, Rule.Fewest, Rule.Most, Set->Lines.Num()),
			Set->Lines.Num() >= Rule.Fewest && Set->Lines.Num() <= Rule.Most);
		for (const FStoryLine& Line : Set->Lines)
		{
			const FString Words = Line.Text.ToString();
			TestTrue(FString::Printf(TEXT("%s: \"%s\" has words, a caption's length (%d of %d)"), Rule.Name, *Words.Left(40), Words.Len(),
				LongestCaption), !Words.IsEmpty() && Words.Len() <= LongestCaption);
			const FString Who = Line.Speaker.ToString();
			if (FCString::Strcmp(Rule.Speaker, TEXT("-")) == 0)
			{
				TestTrue(FString::Printf(TEXT("%s: said in her point's name"), Rule.Name), Who.IsEmpty());
			}
			else if (*Rule.Speaker)
			{
				TestEqual(FString::Printf(TEXT("%s: its voice"), Rule.Name), Who, FString(Rule.Speaker));
			}
			else
			{
				TestFalse(FString::Printf(TEXT("%s: every line names who says it"), Rule.Name), Who.IsEmpty());
			}
		}
	}
	if (Found == 0)
	{
		AddWarning(TEXT("Landfall's line sets aren't made yet: run Tools/Unreal/create_story_lines.py. The rules below still run."));
	}
	else
	{
		TestEqual(TEXT("Every Landfall set is made"), Found, static_cast<int32>(UE_ARRAY_COUNT(Rules)));
		// The topics' first words here are the sets' (story_texts.TOPICS['Speaker_Delia'] names these sets).
		const UStoryLineSet* HatSet = LoadLines(TEXT("DA_Lines_DeliaLandfallHat"));
		const UStoryLineSet* PlainSet = LoadLines(TEXT("DA_Lines_DeliaLandfall"));
		const UStoryLineSet* WaitSet = LoadLines(TEXT("DA_Lines_DeliaLandfallWait"));
		TestTrue(TEXT("Her hat lines open on the hat"), HatSet && !HatSet->Lines.IsEmpty() && HatSet->Lines[0].Text.ToString() == DeliaHatFirst);
		TestTrue(TEXT("Her plain turn-in opens as written here"), PlainSet && !PlainSet->Lines.IsEmpty()
			&& PlainSet->Lines[0].Text.ToString() == DeliaPlainFirst);
		TestTrue(TEXT("Her mid-fight word as written here"), WaitSet && !WaitSet->Lines.IsEmpty() && WaitSet->Lines[0].Text.ToString() == DeliaWaitFirst);
		// Both turn-ins tell what the gang did with the hose, and send the player to the barge for Saint Ada.
		for (const UStoryLineSet* Set : { HatSet, PlainSet })
		{
			bool bHose = false;
			bool bSaint = false;
			for (const FStoryLine& Line : Set ? Set->Lines : TArray<FStoryLine>())
			{
				bHose |= Line.Text.ToString().Contains(TEXT("hose"));
				bSaint |= Line.Text.ToString().Contains(TEXT("Saint Ada"));
			}
			TestTrue(TEXT("A turn-in tells of the hose and asks for Saint Ada"), bHose && bSaint);
		}
	}

	// Which of her words holds when (her topics, first that holds): mid-fight before the porch is held; once ready to turn
	// in, the plain words, or the hat's once its flag is set; after Landfall, none of them (her porch line).
	const FName CopyId(TEXT("TestMain1"));
	const TArray<FSpeakerTopic> Topics = DeliasTopics(CopyId);
	auto First = [&Topics](const FCampaignRecord& Campaign)
	{
		return Topics.IndexOfByPredicate([&Campaign](const FSpeakerTopic& Topic) { return Topic.When.IsMet(Campaign); });
	};
	FCampaignRecord Campaign;
	Campaign.ActiveMission = CopyId;
	Campaign.ActiveMissionStep = 8;
	TestEqual(TEXT("At the porch's fight: shoot first"), First(Campaign), 2);
	Campaign.ActiveMissionStep = ReadyStep;
	TestEqual(TEXT("Ready, no hat: the plain turn-in"), First(Campaign), 1);
	Campaign.SetFlag(HatFlag);
	TestEqual(TEXT("Ready, with Abel's hat: the hat's"), First(Campaign), 0);
	Campaign.Complete(CopyId);
	TestEqual(TEXT("After Landfall: none of them"), First(Campaign), static_cast<int32>(INDEX_NONE));
	return true;
}

#endif
