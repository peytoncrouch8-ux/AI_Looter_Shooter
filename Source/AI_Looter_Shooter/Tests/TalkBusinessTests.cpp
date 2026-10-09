#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bestiary/Ledger.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/SpiderCreature.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Progression/XPCurve.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/HobBird.h"
#include "Story/MisterSexton.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "Story/StoryLineSet.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Main 2, "Shall We Talk Business?" (Docs/Areas/RansomsRest.md): the climb, the spider nest on Ransom's Point, Mister
// Sexton's deal on the lookout's rail and the Ledger. The Ledger's own rules and pages are LedgerTests.cpp's.

namespace
{
	// The ids and tags Main 2 finds its pieces by (Tools/Unreal/create_mission_assets.py, build_area_story.py).
	const FName MainOne(TEXT("Main1"));
	const FName MainTwo(TEXT("Main2"));
	const FName PointPlace(TEXT("Place_RansomsPoint"));
	const FName SextonTag(TEXT("Speaker_Sexton"));
	const FName NestId(TEXT("BluffNest"));
	const FName NestTag(TEXT("Spider_BluffNest"));
	const FName Valley(TEXT("TestValley"));

	/** The deal's words the docs give (Docs/Story.md: Antagonist, Signature moments; Docs/Areas/RansomsRest.md: Main 2), in order. */
	const TCHAR* const DealWords[] = {
		TEXT("Shall we talk business?"),
		TEXT("I attend every death, friend. It's my trade."),
		TEXT("And your father crosses. I'll see to his fare myself."),
		TEXT("They scattered. Where do I even start?"),
		TEXT("Ask a keeper. Their lanterns lean toward a saint's light."),
		TEXT("The keeper's dead."),
		TEXT("So are you, friend."),
	};
	const TCHAR* const WaitingWords = TEXT("The spiders first, friend. I'll keep.");
	const TCHAR* const HobClimb = TEXT("Someone's waiting on you. Up top, where it happened.");
	const TCHAR* const HobBlue = TEXT("See the blue on that one? Fed longer on the dark. Hits harder, and its iron's better.");

	FStoryLine Said(const TCHAR* Speaker, const TCHAR* Words, float Seconds = 3.f)
	{
		return FStoryLine::Make(FText::FromString(Speaker), FText::FromString(Words), Seconds);
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

	/** A stand-in for Main 1: one event finishes it. */
	UMissionDefinition* MakeMainOne(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main1"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.MainOneDone");
		return Mission;
	}

	/** Sexton's words as Main 2 is turned in to him (create_mission_assets.py's). */
	const TCHAR* const SextonsTurnIn[] = {
		TEXT("You've read them, then. Good."),
		TEXT("Seven names, friend. The new moon won't wait."),
	};

	/** Main 2 as the mission script makes it, in code: after Main 1, four steps, turned in to Sexton, 30 experience. */
	UMissionDefinition* MakeTalkBusiness(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main2"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		Mission->Prerequisites = { MainOne };
		UMissionReachObjective* Climb = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 0);
		Climb->Place.Actor.ActorTag = PointPlace;
		Climb->Place.Radius = 1800.f;
		Climb->Place.bIgnoreHeight = false;
		UMissionClearObjective* Nest = MissionTestWorld::AddObjective<UMissionClearObjective>(Mission, 1);
		Nest->SpawnerId = NestId;
		Nest->Count = 5;
		MissionTestWorld::AddObjective<UMissionTalkObjective>(Mission, 2)->SpeakerTag = SextonTag;
		MissionTestWorld::AddObjective<UMissionOpenPageObjective>(Mission, 3)->Page = EMissionPage::Bestiary;
		Mission->TurnIn.SpeakerTag = SextonTag;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("Mister Sexton"));
		for (const TCHAR* Words : SextonsTurnIn)
		{
			Mission->TurnIn.Lines.Add(FStoryLine::Make(FText::GetEmpty(), FText::FromString(Words)));
		}
		Mission->Rewards.Experience = 30;
		return Mission;
	}

	/** The nest as the build script sets it up: four spiders and a Restless one, during Main 2's climb and fight. */
	void SetUpNest(AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = NestId;
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 4),
			EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1, ECreatureRank::Rare) };
		Setup.CreatureTags = { NestTag };
		Setup.ActiveWhen.DuringMission = MainTwo;
		Setup.ActiveWhen.BeforeStep = 2;
		Setup.ActivationRadius = 3000.f;
		Setup.SpawnRadius = 900.f;
	}

	/** Sexton as the build script sets him up, his topics in code: the Ledger's step, the deal, the spiders first. */
	AMisterSexton* PlaceSexton(UWorld* World, const FVector& Where)
	{
		AMisterSexton* Sexton = World->SpawnActor<AMisterSexton>(Where, FRotator::ZeroRotator);
		if (!Sexton)
		{
			return nullptr;
		}
		Sexton->Tags.Add(SextonTag);
		Sexton->ShownWhen.DuringMission = MainTwo;
		FSpeakerTopic Reading;
		Reading.When.DuringMission = MainTwo;
		Reading.When.FromStep = 3;
		Reading.Lines = { Said(TEXT(""), TEXT("The names are in the ledger, friend. Do read them.")) };
		FSpeakerTopic Deal;
		Deal.When.DuringMission = MainTwo;
		Deal.When.FromStep = 2;
		for (const TCHAR* Words : DealWords)
		{
			Deal.Lines.Add(Said(TEXT(""), Words));
		}
		FSpeakerTopic Waiting;
		Waiting.When.DuringMission = MainTwo;
		Waiting.Lines = { Said(TEXT(""), WaitingWords) };
		Sexton->SpeakerPoint->Topics = { Reading, Deal, Waiting };
		return Sexton;
	}

	/** Ransom's Rest as built, when it is: its persistent level. */
	const ULevel* LoadRansomsRest(FAutomationTestBase& Test)
	{
		if (!FPackageName::DoesPackageExist(TEXT("/Game/Maps/Lvl_RansomsRest")))
		{
			Test.AddInfo(TEXT("Lvl_RansomsRest isn't built: skipped."));
			return nullptr;
		}
		const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
		return Test.TestNotNull(TEXT("Lvl_RansomsRest loads"), Map) ? Map->PersistentLevel.Get() : nullptr;
	}

	/** How many creatures of Class a spawner's groups bring in all, at Rank (fixed ranks, as the story's fights have). */
	int32 CountBrought(const AEncounterSpawner& Spawner, const UClass* Class, ECreatureRank Rank)
	{
		int32 Brought = 0;
		for (const FEncounterGroup& Group : Spawner.Groups)
		{
			const bool bOfClass = Group.CreatureClass && Group.CreatureClass->IsChildOf(Class);
			Brought += bOfClass && Group.RankRoll == EEncounterRankRoll::Fixed && Group.Rank == Rank ? Group.Count : 0;
		}
		return Brought;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTalkBusinessMissionTest, "Looter.Story.TalkBusiness.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTalkBusinessMissionTest::RunTest(const FString& Parameters)
{
	// Main 2 as its asset has it, once the mission script has made it: after Main 1, on Ransom's Rest, the climb (height
	// counted), the nest cleared (five), Sexton, the Ledger (the step the Ledger's rule names); turned in to Sexton, for 30
	// experience.
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main2")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main2.DA_Mission_Main2"));
		if (TestNotNull(TEXT("DA_Mission_Main2 loads"), Asset))
		{
			TestTrue(TEXT("Main2, a main mission starting by itself on Ransom's Rest, after Main1"), Asset->GetMissionId() == MainTwo
				&& Asset->Kind == EMissionKind::Main && Asset->Start == EMissionStart::Automatic && Asset->Area == FName(TEXT("RansomsRest"))
				&& Asset->Prerequisites == TArray<FName>({ MainOne }));
			TestEqual(TEXT("Four steps"), Asset->Steps.Num(), 4);
			const UMissionReachObjective* Climb = Cast<UMissionReachObjective>(Asset->GetObjective(0, 0));
			const UMissionClearObjective* Nest = Cast<UMissionClearObjective>(Asset->GetObjective(1, 0));
			const UMissionTalkObjective* Talk = Cast<UMissionTalkObjective>(Asset->GetObjective(2, 0));
			const UMissionOpenPageObjective* Open = Cast<UMissionOpenPageObjective>(Asset->GetObjective(Ledger::HandedOverStep, 0));
			TestTrue(TEXT("1: up the bluff path onto Ransom's Point, height counted"), Climb && Climb->Place.Actor.ActorTag == PointPlace
				&& !Climb->Place.bIgnoreHeight && Climb->Place.Radius >= 1000.f);
			TestTrue(TEXT("2: the nest cleared, all five"), Nest && Nest->SpawnerId == NestId && Nest->Count == 5);
			TestTrue(TEXT("3: talk to Mister Sexton"), Talk && Talk->SpeakerTag == SextonTag);
			TestTrue(TEXT("4: open the Ledger, the step the Ledger is handed over on"), Open && Open->Page == EMissionPage::Bestiary
				&& Ledger::HandedOverStep == 3);
			TestTrue(TEXT("Turned in to Sexton, with words of its own"), Asset->NeedsTurnIn() && Asset->TurnIn.SpeakerTag == SextonTag
				&& Asset->TurnIn.Lines.Num() == static_cast<int32>(UE_ARRAY_COUNT(SextonsTurnIn)) && Asset->TurnIn.Lines[0].Text.ToString() == SextonsTurnIn[0]);
			TestEqual(TEXT("Its reward: 30 experience"), Asset->Rewards.Experience, 30);
			TestFalse(TEXT("...and no gun (the Ledger is the other)"), Asset->Rewards.bGun);
		}
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Main2 isn't made yet: run Tools/Unreal/create_mission_assets.py. The flow below runs on a copy."));
	}

	// Played through in a test level: Main 2 waits for Main 1; Sexton waits on the rail; the nest comes out as the player
	// climbs, two are shot from the path, the rest on the top; the deal; the Ledger.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-6000.0, 0.0, 0.0));
	AActor* Top = MissionTestWorld::SpawnMarker(World, FVector(-1500.0, 0.0, 0.0), PointPlace);
	AEncounterSpawner* Nest = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, SetUpNest);
	AMisterSexton* Sexton = PlaceSexton(World, FVector(-2500.0, 0.0, 750.0));
	if (!Runner || !Captions || !Player || !Top || !Nest || !Sexton)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* TalkBusiness = MakeTalkBusiness(Scratch);
	Runner->BeginForTesting({ MakeMainOne(Scratch), TalkBusiness }, Campaign, Player, Valley);
	// After the runner: his story is read from the test's record.
	Sexton->DispatchBeginPlay();
	Runner->Update(0.f);
	TestTrue(TEXT("Main 1 first; Main 2 waits for it"), Runner->IsRunning(MainOne) && !Runner->IsRunning(MainTwo)
		&& Runner->GetStatus(*TalkBusiness) == EMissionStatus::Locked);
	TestFalse(TEXT("No Sexton before Main 2"), Sexton->IsShown());
	Nest->UpdateEncounter(0.5f);
	TestTrue(TEXT("No nest before Main 2"), Nest->GetState() == EEncounterState::Off && Nest->NumAlive() == 0);

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainOneDone")));
	TestTrue(TEXT("Main 1 done: Main 2 starts by itself, at the climb"), Campaign.HasCompleted(MainOne) && Runner->GetStep(MainTwo) == 0);
	TestTrue(TEXT("Sexton waits on the rail"), Sexton->IsShown() && Sexton->SpeakerPoint->CanTalk());
	TestFalse(TEXT("The bestiary isn't his Ledger yet"), Ledger::IsOpen(Campaign, Runner));
	Nest->UpdateEncounter(0.5f);
	TestTrue(TEXT("The nest is on, waiting for the player, who is far"), Nest->IsStoryActive() && Nest->GetState() == EEncounterState::Waiting
		&& Nest->NumAlive() == 0);

	// Up the path under the top's edge: within 30 m of the nest, 12 m below the top, not on it yet.
	Player->SetActorLocation(FVector(-2500.0, -1000.0, -1200.0));
	Nest->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Out = Nest->GetAliveCreatures();
	TestEqual(TEXT("The nest comes out as the player climbs: five"), Out.Num(), 5);
	TestEqual(TEXT("...four Basic spiders"), EncounterTestWorld::CountOf(Out, ASpiderCreature::StaticClass(), ECreatureRank::Basic), 4);
	TestEqual(TEXT("...and a Restless one (the blue one)"), EncounterTestWorld::CountOf(Out, ASpiderCreature::StaticClass(), ECreatureRank::Rare), 1);
	TestFalse(TEXT("...each tagged for the nest"), Out.ContainsByPredicate([](const ACreatureBase* Spider) { return !Spider->ActorHasTag(NestTag); }));
	Runner->Update(0.2f);
	TestEqual(TEXT("Under the edge, the top's middle 18.5 m off with the height: still climbing"), Runner->GetStep(MainTwo), 0);
	if (Out.Num() < 5)
	{
		return false;
	}

	// Two shot from the path, before the top: they count for the fight.
	BossTestWorld::Kill(Out[0]);
	BossTestWorld::Kill(Out[1]);
	Nest->UpdateEncounter(0.5f);
	Player->SetActorLocation(FVector(-1500.0, 600.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("On the top: the fight"), Runner->GetStep(MainTwo), 1);
	TArray<FMissionObjectiveView> Views = Runner->GetObjectiveViews(MainTwo);
	TestTrue(TEXT("The two shot from the path count: 2 / 5"), Views.Num() == 1 && Views[0].Progress == TEXT("2 / 5"));

	// Talked to mid-fight, Sexton puts the spiders first; the fight goes on.
	TestTrue(TEXT("Sexton talked to mid-fight"), Sexton->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...the spiders first"), OnScreen(*Captions), FString(TEXT("Mister Sexton|")) + WaitingWords);
	TestEqual(TEXT("...and the fight goes on"), Runner->GetStep(MainTwo), 1);

	EncounterTestWorld::KillAll(*Nest);
	Runner->Update(0.2f);
	Views = Runner->GetObjectiveViews(MainTwo);
	TestTrue(TEXT("All down, the nest not looked at again: one short, 4 / 5"), Views.Num() == 1 && Views[0].Progress == TEXT("4 / 5"));
	Nest->UpdateEncounter(0.5f);
	Runner->Update(0.2f);
	TestEqual(TEXT("The nest cleared: on to Sexton"), Runner->GetStep(MainTwo), 2);
	TestTrue(TEXT("...and the nest stays cleared, its story over"), Nest->GetState() == EEncounterState::Cleared && !Nest->IsStoryActive());

	// The deal: "Shall we talk business?", and the Ledger is his from the next step.
	Captions->Update(30.f);
	TestTrue(TEXT("Sexton talked to"), Sexton->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("Shall we talk business?"), OnScreen(*Captions), FString(TEXT("Mister Sexton|Shall we talk business?")));
	TestEqual(TEXT("The deal struck: open the Ledger"), Runner->GetStep(MainTwo), Ledger::HandedOverStep);
	TestTrue(TEXT("The bestiary is his Ledger now"), Ledger::IsOpen(Campaign, Runner));

	// Opening the inventory's second page does its last objective (a test level has no HUD: the step is passed by hand):
	// it's ready to turn in to Sexton, still on the rail.
	Runner->CompleteStep(MainTwo);
	TestTrue(TEXT("The Ledger read: ready to turn in to Sexton, not finished"), Runner->IsReadyToTurnIn(MainTwo) && !Campaign.HasCompleted(MainTwo));
	TestTrue(TEXT("The Ledger stays his"), Ledger::IsOpen(Campaign, Runner));
	TestTrue(TEXT("Sexton still on the rail, to be talked to"), Sexton->IsShown() && !Sexton->IsLeaving());
	Captions->Update(600.f);
	TestTrue(TEXT("Sexton talked to"), Sexton->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...his words for it"), OnScreen(*Captions), FString(TEXT("Mister Sexton|")) + SextonsTurnIn[0]);
	TestTrue(TEXT("Main 2 is turned in: finished"), Campaign.HasCompleted(MainTwo) && !Runner->IsRunning(MainTwo));
	TestTrue(TEXT("The Ledger stays his"), Ledger::IsOpen(Campaign, Runner));
	TestTrue(TEXT("Sexton, his words still being said, stays till they're done"), Sexton->IsShown() && Sexton->IsLeaving());
	Captions->Update(600.f);
	Sexton->Tick(0.25f);
	TestTrue(TEXT("Said, and nobody looking: he's gone"), !Sexton->IsShown() && !Sexton->IsLeaving() && !Sexton->SpeakerPoint->CanTalk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTalkBusinessSextonTest, "Looter.Story.TalkBusiness.Sexton",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTalkBusinessSextonTest::RunTest(const FString& Parameters)
{
	// Mister Sexton as a story character: his seated model with the ledger on its socket and his voice at his chin, a
	// posed idle (he never turns or breathes), a hull for the player's body and the Interact key but none for shots.
	const AMisterSexton* Defaults = GetDefault<AMisterSexton>();
	TestEqual(TEXT("He never turns to whoever talks"), Defaults->TurnSpeed, 0.f);
	TestEqual(TEXT("...nor breathes"), Defaults->BreathHeight, 0.f);
	TestEqual(TEXT("His name on his captions"), Defaults->SpeakerPoint->SpeakerName.ToString(), FString(TEXT("Mister Sexton")));
	const UStaticMeshComponent* Body = Defaults->Body;
	if (TestNotNull(TEXT("His body"), Body))
	{
		TestTrue(TEXT("Shots pass through him"), Body->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Ignore);
		TestTrue(TEXT("...and the creatures' pellets"), Body->GetCollisionResponseToChannel(ECC_GameTraceChannel1) == ECR_Ignore);
		TestTrue(TEXT("The player's body stops at him"), Body->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
		TestTrue(TEXT("...and the Interact key's line finds him"), Body->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block);
	}
	TestTrue(TEXT("His ledger has no collision"), Defaults->Ledger && !Defaults->Ledger->IsCollisionEnabled());
	if (FPackageName::DoesPackageExist(TEXT("/Game/Art/Characters/SM_MisterSexton")))
	{
		TestTrue(TEXT("His seated model, unscaled on the seat"), Body && Body->GetStaticMesh() && Body->GetStaticMesh()->GetName() == TEXT("SM_MisterSexton")
			&& Body->GetRelativeScale3D().Equals(FVector::OneVector) && Body->GetRelativeLocation().IsNearlyZero());
		TestTrue(TEXT("His ledger on his knee (the Ledger socket)"), Defaults->Ledger->GetAttachSocketName() == AMisterSexton::LedgerSocket);
		TestTrue(TEXT("His voice at his chin (the Speaker socket)"), Defaults->SpeakerPoint->GetAttachSocketName() == AMisterSexton::SpeakerSocket);
	}
	else
	{
		AddWarning(TEXT("SM_MisterSexton isn't imported in this checkout: the placeholder stands in."));
	}

	// Shown by his story: not before Main 2, there during it, gone after once nobody's looking; a level loaded after Main 2
	// never shows him.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Listener = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	AMisterSexton* Sexton = PlaceSexton(World, FVector(400.0, 0.0, 100.0));
	if (!Runner || !Listener || !Sexton)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Campaign.ActiveMission = MainOne;
	Runner->BeginForTesting({}, Campaign, Listener, Valley);
	Sexton->DispatchBeginPlay();
	TestTrue(TEXT("During Main 1: not there, not solid, nobody to talk to"), !Sexton->IsShown() && Sexton->IsHidden()
		&& !Sexton->GetActorEnableCollision() && !Sexton->SpeakerPoint->CanTalk());
	Campaign.Complete(MainOne);
	Campaign.ActiveMission = MainTwo;
	Sexton->RefreshShown();
	TestTrue(TEXT("During Main 2: on the rail, solid, to be talked to"), Sexton->IsShown() && !Sexton->IsHidden()
		&& Sexton->GetActorEnableCollision() && Sexton->SpeakerPoint->CanTalk());
	TestTrue(TEXT("Talked to during the climb: the spiders first"), Sexton->SpeakerPoint->Talk(Listener));
	Campaign.Complete(MainTwo);
	Sexton->RefreshShown();
	TestTrue(TEXT("Main 2 done mid-sentence: he stays till he's said it"), Sexton->IsShown() && Sexton->IsLeaving());
	Sexton->Tick(0.25f);
	TestTrue(TEXT("...still talking: still there"), Sexton->IsShown());
	UCaptionSubsystem::Get(World)->Update(30.f);
	Sexton->Tick(0.25f);
	TestTrue(TEXT("Said, and unseen: gone"), !Sexton->IsShown() && !Sexton->IsLeaving());

	AMisterSexton* Later = PlaceSexton(World, FVector(-400.0, 0.0, 100.0));
	if (TestNotNull(TEXT("A level loaded after Main 2"), Later))
	{
		Later->DispatchBeginPlay();
		TestFalse(TEXT("...has no Sexton on the rail"), Later->IsShown());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTalkBusinessLinesTest, "Looter.Story.TalkBusiness.Lines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTalkBusinessLinesTest::RunTest(const FString& Parameters)
{
	// The words as the story script writes them (when it has): the deal holds the docs' lines in their order, Sexton's
	// named by his speaker point and Ellis's by name; Hob's two lines are the doc's.
	const UStoryLineSet* Deal = LoadLines(TEXT("DA_Lines_SextonDeal"));
	if (!Deal)
	{
		AddWarning(TEXT("The story's line sets aren't made yet: run Tools/Unreal/create_story_lines.py."));
		return true;
	}
	int32 Next = 0;
	for (const FStoryLine& Line : Deal->Lines)
	{
		if (Next < UE_ARRAY_COUNT(DealWords) && Line.Text.ToString() == DealWords[Next])
		{
			++Next;
		}
	}
	TestEqual(TEXT("The deal holds the docs' lines, in order"), Next, static_cast<int32>(UE_ARRAY_COUNT(DealWords)));
	TestTrue(TEXT("It opens with \"Shall we talk business?\", in his voice"), !Deal->Lines.IsEmpty()
		&& Deal->Lines[0].Text.ToString() == DealWords[0] && Deal->Lines[0].Speaker.IsEmpty());
	TestTrue(TEXT("Ellis asks where to start, by name"), Deal->Lines.ContainsByPredicate([](const FStoryLine& Line)
	{
		return Line.Speaker.ToString() == TEXT("Ellis") && Line.Text.ToString() == TEXT("They scattered. Where do I even start?");
	}));
	TestTrue(TEXT("\"So are you, friend.\" is his"), Deal->Lines.ContainsByPredicate([](const FStoryLine& Line)
	{
		return Line.Speaker.IsEmpty() && Line.Text.ToString() == TEXT("So are you, friend.");
	}));
	if (const UStoryLineSet* Climb = LoadLines(TEXT("DA_Lines_HobMain2Climb")))
	{
		TestTrue(TEXT("Hob sends Ellis up top"), !Climb->Lines.IsEmpty() && Climb->Lines[0].Text.ToString() == HobClimb);
	}
	if (const UStoryLineSet* Blue = LoadLines(TEXT("DA_Lines_HobMain2Blue")))
	{
		TestTrue(TEXT("Hob on the blue one"), !Blue->Lines.IsEmpty() && Blue->Lines[0].Text.ToString() == HobBlue);
	}
	TestNotNull(TEXT("Sexton's word before the spiders are cleared"), LoadLines(TEXT("DA_Lines_SextonWaiting")));
	TestNotNull(TEXT("...and while the Ledger waits"), LoadLines(TEXT("DA_Lines_SextonLedger")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTalkBusinessPlacedTest, "Looter.Story.TalkBusiness.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTalkBusinessPlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (build_area_story.py): Sexton on the lookout's rail during Main 2, the bluff top's place, and
	// the nest's four spiders and a Restless one on the bluff top's ground during the climb and the fight.
	const ULevel* Level = LoadRansomsRest(*this);
	if (!Level)
	{
		return true;
	}
	int32 Sextons = 0;
	const AMisterSexton* Sexton = nullptr;
	const AEncounterSpawner* Nest = nullptr;
	const AHobBird* Hob = nullptr;
	bool bPlace = false;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (const AMisterSexton* Found = Cast<AMisterSexton>(Actor))
		{
			++Sextons;
			Sexton = Found;
		}
		if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor); Spawner && Spawner->SpawnerId == NestId)
		{
			Nest = Spawner;
		}
		Hob = Hob ? Hob : Cast<AHobBird>(Actor);
		bPlace |= Actor->ActorHasTag(PointPlace);
	}
	if (!Sexton && !Nest && !bPlace)
	{
		AddWarning(TEXT("Main 2's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	TestEqual(TEXT("One Sexton"), Sextons, 1);
	if (TestNotNull(TEXT("Sexton on the rail"), Sexton))
	{
		TestTrue(TEXT("...tagged for Main 2's talk"), Sexton->ActorHasTag(SextonTag));
		TestTrue(TEXT("...there during Main 2"), Sexton->ShownWhen.DuringMission == MainTwo && Sexton->ShownWhen.AfterMissions.IsEmpty());
		TestTrue(TEXT("...with the deal among his topics"), Sexton->SpeakerPoint->Topics.ContainsByPredicate([](const FSpeakerTopic& Topic)
		{
			return Topic.When.DuringMission == MainTwo && Topic.When.FromStep == 2 && Topic.LineSet != nullptr;
		}));
	}
	TestTrue(TEXT("Ransom's Point's middle is marked for the climb"), bPlace);
	if (TestNotNull(TEXT("The spider nest"), Nest))
	{
		TestEqual(TEXT("...four Basic spiders"), CountBrought(*Nest, ASpiderCreature::StaticClass(), ECreatureRank::Basic), 4);
		TestEqual(TEXT("...and one Restless Meadow Wolf"), CountBrought(*Nest, ASpiderCreature::StaticClass(), ECreatureRank::Rare), 1);
		TestTrue(TEXT("...during Main 2's climb and fight only"), Nest->ActiveWhen.DuringMission == MainTwo && Nest->ActiveWhen.FromStep == 0
			&& Nest->ActiveWhen.BeforeStep == 2);
		TestTrue(TEXT("...tagged for the fight"), Nest->CreatureTags.Contains(NestTag));
		TestTrue(TEXT("...on the bluff top's ground"), Nest->GroundCorners.Num() >= 3);
	}
	if (TestNotNull(TEXT("Hob"), Hob))
	{
		TestTrue(TEXT("Hob has perches in Main 2"), Hob->Perches.ContainsByPredicate([](const FHobPerch& Perch)
		{
			return Perch.When.DuringMission == MainTwo;
		}));
	}
	return true;
}

#endif
