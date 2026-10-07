#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/UnpaidCreature.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "Story/StoryLineSet.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Weapons/WeaponTypes.h"
#include "World/SafeGround.h"
#include "World/WindowShutter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Main 3, "Cold Welcome" (Docs/Areas/RansomsRest.md): the farm road into town, the Unpaid at the gate, Bright & Daughter,
// Tilly at her window; the shutters that slam, and Main Street safe after it.

namespace
{
	// The ids and tags Main 3 finds its pieces by (Tools/Unreal/create_mission_assets.py, build_area_story.py).
	const FName MainTwo(TEXT("Main2"));
	const FName MainThree(TEXT("Main3"));
	const FName GatePlace(TEXT("Place_TownGate"));
	const FName GateId(TEXT("TownGate"));
	const FName GateTag(TEXT("Unpaid_TownGate"));
	const FName TillyTag(TEXT("Speaker_Tilly"));
	const FName Valley(TEXT("TestValley"));

	const TCHAR* const Collar = TEXT("You've ruined my collar, by the way.");
	const TCHAR* const Closed = TEXT("We're closed. Back after the funeral.");
	const TCHAR* const Later = TEXT("Still closed. Mind that collar.");

	/** Where the gate's Unpaid stand, from the gate (build_area_story.py's GATE_SPOTS: north-east of it). */
	const FVector GateSpots[] = { FVector(600.0, 900.0, 0.0), FVector(1100.0, 400.0, 0.0), FVector(300.0, 1500.0, 0.0),
		FVector(1300.0, 1300.0, 0.0) };

	FStoryLine Said(const TCHAR* Speaker, const TCHAR* Words, float Seconds = 3.f)
	{
		return FStoryLine::Make(FText::FromString(Speaker), FText::FromString(Words), Seconds);
	}

	FString OnScreen(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT("|") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}

	const UStoryLineSet* LoadLines(const TCHAR* Name)
	{
		const FString Package = FString(TEXT("/Game/Data/Story/")) + Name;
		return FPackageName::DoesPackageExist(Package) ? LoadObject<UStoryLineSet>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
	}

	UStaticMesh* LoadMesh(const TCHAR* Path)
	{
		return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path))) ? LoadObject<UStaticMesh>(nullptr, Path) : nullptr;
	}

	/** A stand-in for Main 2: one event finishes it. */
	UMissionDefinition* MakeMainTwo(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main2"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.MainTwoDone");
		return Mission;
	}

	/** Main 3 as the mission script makes it, in code (its gun left out: a test level has no loot to drop it in). */
	UMissionDefinition* MakeColdWelcome(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main3"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		Mission->Prerequisites = { MainTwo };
		UMissionReachObjective* Road = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 0);
		Road->Place.Actor.ActorTag = GatePlace;
		Road->Place.Radius = 1000.f;
		UMissionClearObjective* Gate = MissionTestWorld::AddObjective<UMissionClearObjective>(Mission, 1);
		Gate->SpawnerId = GateId;
		Gate->Count = 4;
		UMissionReachObjective* Shop = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 2);
		Shop->Place.Actor.ActorTag = TillyTag;
		Shop->Place.Radius = 800.f;
		MissionTestWorld::AddObjective<UMissionTalkObjective>(Mission, 3)->SpeakerTag = TillyTag;
		Mission->Rewards.ExperienceShare = 0.3f;
		return Mission;
	}

	/** The gate's fight as the build script sets it up: three Unpaid and a Restless one, on Main 3's second step only. */
	void SetUpGate(AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = GateId;
		Setup.Groups = { EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 3),
			EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 1, ECreatureRank::Rare) };
		Setup.CreatureTags = { GateTag };
		Setup.ActiveWhen.DuringMission = MainThree;
		Setup.ActiveWhen.FromStep = 1;
		Setup.ActiveWhen.BeforeStep = 2;
		Setup.SpawnPoints = TArray<FVector>(GateSpots, UE_ARRAY_COUNT(GateSpots));
		Setup.bHuntOnSpawn = true;
		Setup.GiveUpRadius = 3000.f;
	}

	/** Tilly's window as the build script sets it up: closed, then her Main 3 lines from its last step, then a word after. */
	ASpeakerPoint* PlaceTilly(UWorld* World, const FVector& Where)
	{
		ASpeakerPoint* Window = World->SpawnActor<ASpeakerPoint>(Where, FRotator(0.0, 180.0, 0.0));
		if (!Window)
		{
			return nullptr;
		}
		USpeakerPointComponent* Talk = Window->SpeakerPoint;
		Talk->SetRelativeLocation(FVector::ZeroVector);
		Talk->SpeakerName = FText::FromString(TEXT("Tilly Bright"));
		const UStoryLineSet* ClosedSet = LoadLines(TEXT("DA_Lines_TillyClosed"));
		const UStoryLineSet* MainSet = LoadLines(TEXT("DA_Lines_TillyMain3"));
		const UStoryLineSet* AfterSet = LoadLines(TEXT("DA_Lines_TillyAfterMain3"));
		Talk->Lines = ClosedSet ? ClosedSet->Lines : TArray<FStoryLine>({ Said(TEXT(""), Closed) });
		FSpeakerTopic During;
		During.When.DuringMission = MainThree;
		During.When.FromStep = 3;
		During.Lines = MainSet ? MainSet->Lines : TArray<FStoryLine>({ Said(TEXT(""), TEXT("I laid you out, you know.")),
			Said(TEXT(""), Collar), Said(TEXT("Ellis"), TEXT("Pa's lantern. Was it with him?")) });
		FSpeakerTopic After;
		After.When.AfterMissions = { MainThree };
		After.Lines = AfterSet ? AfterSet->Lines : TArray<FStoryLine>({ Said(TEXT(""), Later) });
		Talk->Topics = { During, After };
		Window->Tags.Add(TillyTag);
		return Window;
	}

	/** A shutter on a hinge at Where, closed as its socket hangs it, its leaf Mesh; begun. */
	AWindowShutter* SpawnShutter(UWorld* World, const FVector& Where, UStaticMesh* Mesh)
	{
		AWindowShutter* Shutter = World->SpawnActor<AWindowShutter>(Where, FRotator::ZeroRotator);
		if (!Shutter)
		{
			return nullptr;
		}
		Shutter->Leaf->SetStaticMesh(Mesh);
		Shutter->ShutWhen.AfterMissions = { MainThree };
		Shutter->DispatchBeginPlay();
		return Shutter;
	}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FColdWelcomeMissionTest, "Looter.Story.ColdWelcome.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FColdWelcomeMissionTest::RunTest(const FString& Parameters)
{
	// Main 3 as its asset has it: id exactly Main3 (Side 1 opens after it), after Main 2 on Ransom's Rest; the farm road
	// to the gate, the gate's fight cleared (four), Bright & Daughter, Tilly; 30% of a level and an Uncommon gun or better.
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main3")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main3.DA_Mission_Main3"));
		if (TestNotNull(TEXT("DA_Mission_Main3 loads"), Asset))
		{
			TestTrue(TEXT("Main3, a main mission starting by itself on Ransom's Rest, after Main2"), Asset->GetMissionId() == MainThree
				&& Asset->Kind == EMissionKind::Main && Asset->Start == EMissionStart::Automatic && Asset->Area == FName(TEXT("RansomsRest"))
				&& Asset->Prerequisites == TArray<FName>({ MainTwo }));
			TestEqual(TEXT("Four steps"), Asset->Steps.Num(), 4);
			const UMissionReachObjective* Road = Cast<UMissionReachObjective>(Asset->GetObjective(0, 0));
			const UMissionClearObjective* Gate = Cast<UMissionClearObjective>(Asset->GetObjective(1, 0));
			const UMissionReachObjective* Shop = Cast<UMissionReachObjective>(Asset->GetObjective(2, 0));
			const UMissionTalkObjective* Talk = Cast<UMissionTalkObjective>(Asset->GetObjective(3, 0));
			TestTrue(TEXT("1: the farm road into town, to the gate"), Road && Road->Place.Actor.ActorTag == GatePlace);
			TestTrue(TEXT("2: the Unpaid at the gate fought off, all four"), Gate && Gate->SpawnerId == GateId && Gate->Count == 4);
			TestTrue(TEXT("3: Bright & Daughter found (Tilly's window)"), Shop && Shop->Place.Actor.ActorTag == TillyTag);
			TestTrue(TEXT("4: Tilly talked to at the window"), Talk && Talk->SpeakerTag == TillyTag);
			TestEqual(TEXT("Its reward: 30% of a level"), Asset->Rewards.ExperienceShare, 0.3f);
			TestTrue(TEXT("...and a gun, Uncommon or better"), Asset->Rewards.bGun && Asset->Rewards.GunRarityFloor == EWeaponRarity::Uncommon);
			TestTrue(TEXT("...said so on the Missions page"), MissionRewards::Describe(Asset->Rewards).Contains(TEXT("Gun: Uncommon or better")));
		}
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Main3 isn't made yet: run Tools/Unreal/create_mission_assets.py. The flow below runs on a copy."));
	}
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Side1")))
	{
		const UMissionDefinition* Side = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Side1.DA_Mission_Side1"));
		TestTrue(TEXT("Side 1 opens after Main 3"), Side && Side->Prerequisites.Contains(MainThree));
	}
	TestTrue(TEXT("An Uncommon floor raises a Common gun"), MissionRewards::ApplyFloor(EWeaponRarity::Common, EWeaponRarity::Uncommon) == EWeaponRarity::Uncommon);

	// Played through in a test level: Main 3 waits for Main 2; the gate's fight waits for the gate; four Unpaid come, one
	// Restless; once they're down, the shop; Tilly closed until then, her lines at the last step.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-3000.0, -3000.0, 0.0));
	AActor* GateMark = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector, GatePlace);
	AEncounterSpawner* Gate = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, SetUpGate);
	ASpeakerPoint* Tilly = PlaceTilly(World, FVector(0.0, 6000.0, 170.0));
	if (!Runner || !Captions || !Player || !GateMark || !Gate || !Tilly)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* ColdWelcome = MakeColdWelcome(Scratch);
	Runner->BeginForTesting({ MakeMainTwo(Scratch), ColdWelcome }, Campaign, Player, Valley);
	Tilly->DispatchBeginPlay();
	Runner->Update(0.f);
	TestTrue(TEXT("Main 2 first; Main 3 waits for it"), Runner->IsRunning(MainTwo) && Runner->GetStatus(*ColdWelcome) == EMissionStatus::Locked);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainTwoDone")));
	TestTrue(TEXT("Main 2 done: Main 3 starts by itself, on the farm road"), Campaign.HasCompleted(MainTwo) && Runner->GetStep(MainThree) == 0);
	Gate->UpdateEncounter(0.5f);
	TestTrue(TEXT("On the road the gate is quiet: its fight waits for the gate"), Gate->GetState() == EEncounterState::Off && Gate->NumAlive() == 0);

	TestTrue(TEXT("Tilly talked to early"), Tilly->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...the shop's closed"), OnScreen(*Captions), FString(TEXT("Tilly Bright|")) + Closed);

	// At the gate: they come for the corpse.
	Player->SetActorLocation(FVector(-500.0, -500.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("At the gate: the fight"), Runner->GetStep(MainThree), 1);
	TestTrue(TEXT("...the gate's fight is on"), Gate->IsStoryActive());
	Gate->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Out = Gate->GetAliveCreatures();
	TestEqual(TEXT("Four Unpaid at the gate"), Out.Num(), 4);
	TestEqual(TEXT("...three of them Basic"), EncounterTestWorld::CountOf(Out, AUnpaidCreature::StaticClass(), ECreatureRank::Basic), 3);
	TestEqual(TEXT("...one of them Restless"), EncounterTestWorld::CountOf(Out, AUnpaidCreature::StaticClass(), ECreatureRank::Rare), 1);
	TestFalse(TEXT("...each tagged for the gate"), Out.ContainsByPredicate([](const ACreatureBase* Unpaid) { return !Unpaid->ActorHasTag(GateTag); }));
	TestFalse(TEXT("...none of them in the farm's corner of the map (south-west of the gate)"), Out.ContainsByPredicate([](const ACreatureBase* Unpaid)
	{
		return Unpaid->GetActorLocation().X < 0.0 && Unpaid->GetActorLocation().Y < 0.0;
	}));

	EncounterTestWorld::KillAll(*Gate);
	Gate->UpdateEncounter(0.5f);
	Runner->Update(0.2f);
	TestEqual(TEXT("The Unpaid fought off: find Bright & Daughter"), Runner->GetStep(MainThree), 2);
	TestTrue(TEXT("...and the gate's fight is over, cleared"), Gate->GetState() == EEncounterState::Cleared && !Gate->IsStoryActive());

	// The shop, then Tilly.
	Captions->Update(30.f);
	TestTrue(TEXT("Tilly talked to before the shop's found"), Tilly->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...still closed"), OnScreen(*Captions), FString(TEXT("Tilly Bright|")) + Closed);
	TestEqual(TEXT("...and the step stays"), Runner->GetStep(MainThree), 2);
	Player->SetActorLocation(FVector(0.0, 5400.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("At Bright & Daughter: talk to Tilly"), Runner->GetStep(MainThree), 3);
	Captions->Update(30.f);
	TestTrue(TEXT("Tilly talked to"), Tilly->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("Main 3 is finished"), Campaign.HasCompleted(MainThree) && !Runner->IsRunning(MainThree));
	// Her lines one by one, as they come on screen.
	bool bCollar = false;
	for (int32 Line = 0; Line < 12 && !bCollar && Captions->GetCurrent(); ++Line)
	{
		bCollar = OnScreen(*Captions) == FString(TEXT("Tilly Bright|")) + Collar;
		Captions->Update(Captions->GetCurrent()->Seconds + 0.01f);
	}
	TestTrue(TEXT("...her lines: \"You've ruined my collar, by the way.\""), bCollar);
	Captions->Update(600.f);
	TestTrue(TEXT("After Main 3, her word through the window"), Tilly->SpeakerPoint->Talk(Player));
	const TArray<FStoryLine> Now = Tilly->SpeakerPoint->GetLinesNow();
	TestTrue(TEXT("...not her Main 3 lines again"), !Now.ContainsByPredicate([](const FStoryLine& Line) { return Line.Text.ToString() == Collar; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FColdWelcomeTillyTest, "Looter.Story.ColdWelcome.Tilly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FColdWelcomeTillyTest::RunTest(const FString& Parameters)
{
	// Tilly's topics by the story: the shop closed before Main 3's last step, her lines at it (she laid out both Ransoms,
	// Abel's lantern wasn't on him, the collar), a word after Main 3.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Listener = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	ASpeakerPoint* Tilly = PlaceTilly(World, FVector(300.0, 0.0, 170.0));
	if (!Runner || !Listener || !Tilly)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Listener, Valley);
	Tilly->DispatchBeginPlay();

	auto Says = [Tilly](int32& OutTopic) { return Tilly->SpeakerPoint->GetLinesNow(&OutTopic); };
	int32 Topic = INDEX_NONE;
	Campaign.ActiveMission = MainTwo;
	TArray<FStoryLine> Lines = Says(Topic);
	TestTrue(TEXT("During Main 2: the shop's closed"), Topic == INDEX_NONE && !Lines.IsEmpty() && Lines[0].Speaker.ToString() == TEXT("Tilly Bright"));
	Campaign.Complete(MainTwo);
	Campaign.ActiveMission = MainThree;
	Campaign.ActiveMissionStep = 1;
	Says(Topic);
	TestEqual(TEXT("During the gate's fight: still closed"), Topic, static_cast<int32>(INDEX_NONE));
	Campaign.ActiveMissionStep = 3;
	Lines = Says(Topic);
	TestEqual(TEXT("At Main 3's last step: her Main 3 lines"), Topic, 0);
	TestTrue(TEXT("...\"You've ruined my collar, by the way.\", in her name"), Lines.ContainsByPredicate([](const FStoryLine& Line)
	{
		return Line.Text.ToString() == Collar && Line.Speaker.ToString() == TEXT("Tilly Bright");
	}));
	TestTrue(TEXT("...and Ellis asks after Pa's lantern, by name"), Lines.ContainsByPredicate([](const FStoryLine& Line)
	{
		return Line.Speaker.ToString() == TEXT("Ellis") && Line.Text.ToString().Contains(TEXT("lantern"));
	}));
	Campaign.Complete(MainThree);
	Says(Topic);
	TestEqual(TEXT("After Main 3: a word, not the lines again"), Topic, 1);

	// The words as the story script writes them, when it has.
	if (const UStoryLineSet* Main = LoadLines(TEXT("DA_Lines_TillyMain3")))
	{
		TestTrue(TEXT("Her Main 3 set holds the doc's line"), Main->Lines.ContainsByPredicate([](const FStoryLine& Line)
		{
			return Line.Text.ToString() == Collar && Line.Speaker.IsEmpty();
		}));
		TestTrue(TEXT("...and that Abel's lantern wasn't on him"), Main->Lines.ContainsByPredicate([](const FStoryLine& Line)
		{
			return Line.Text.ToString().Contains(TEXT("wasn't on him"));
		}));
	}
	else
	{
		AddWarning(TEXT("The story's line sets aren't made yet: run Tools/Unreal/create_story_lines.py."));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FColdWelcomeShuttersTest, "Looter.Story.ColdWelcome.Shutters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FColdWelcomeShuttersTest::RunTest(const FString& Parameters)
{
	// The living shutter their windows: a shutter hangs open flat against the wall, its free edge swung out toward the
	// street; the player coming near slams it (after its moment), and it stays shut; after Main 3 it's shut from the start,
	// and one still open when Main 3 ends slams.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Listener = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	if (!Runner || !Listener)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Campaign.ActiveMission = MainThree;
	Runner->BeginForTesting({}, Campaign, Listener, Valley);
	UStaticMesh* LeftMesh = LoadMesh(TEXT("/Game/Art/Buildings/SM_Shutter_Left.SM_Shutter_Left"));
	UStaticMesh* RightMesh = LoadMesh(TEXT("/Game/Art/Buildings/SM_Shutter_Right.SM_Shutter_Right"));
	if (!LeftMesh || !RightMesh)
	{
		AddWarning(TEXT("The shutters aren't imported in this checkout (FalseFronts.py): their swing's side is checked without them."));
	}
	AWindowShutter* Left = SpawnShutter(World, FVector(0.0, 0.0, 0.0), LeftMesh);
	AWindowShutter* Right = SpawnShutter(World, FVector(0.0, 200.0, 0.0), RightMesh);
	if (!TestNotNull(TEXT("A left shutter"), Left) || !TestNotNull(TEXT("A right shutter"), Right))
	{
		return false;
	}
	TestTrue(TEXT("Before Main 3 is done, open as the level begins"), Left->GetState() == EWindowShutterState::Open
		&& FMath::IsNearlyEqual(Left->GetSwing(), Left->GetOpenSwing()));
	TestEqual(TEXT("...through 178 degrees, flat against the wall"), FMath::Abs(Left->GetOpenSwing()), AWindowShutter::OpenDegrees);
	for (const TPair<AWindowShutter*, UStaticMesh*>& Each : { TPair<AWindowShutter*, UStaticMesh*>(Left, LeftMesh), TPair<AWindowShutter*, UStaticMesh*>(Right, RightMesh) })
	{
		if (Each.Value)
		{
			// Half open, its leaf stands out in front of the wall (the socket's front is the street): it swings out, not in.
			const FVector Middle = Each.Value->GetBoundingBox().GetCenter();
			TestTrue(FString::Printf(TEXT("%s swings out toward the street"), *Each.Value->GetName()),
				FRotator(0.f, Each.Key->GetOpenSwing() * 0.5f, 0.f).RotateVector(Middle).X > 1.0);
		}
	}
	if (LeftMesh && RightMesh)
	{
		TestTrue(TEXT("Left and right swing opposite ways"), Left->GetOpenSwing() * Right->GetOpenSwing() < 0.f);
	}

	TestFalse(TEXT("The player far off: nothing"), Left->NoticePlayer(FVector(5000.0, 0.0, 0.0)));
	TestTrue(TEXT("The player near: it means to slam"), Left->NoticePlayer(FVector(1000.0, 0.0, 0.0))
		&& Left->GetState() == EWindowShutterState::Noticed && FMath::IsNearlyEqual(Left->GetSwing(), Left->GetOpenSwing()));
	Left->AdvanceSlam(Left->MaxDelay + Left->SlamSeconds + 1.f);
	TestTrue(TEXT("...and slams shut, for good"), Left->IsShut() && FMath::IsNearlyZero(Left->GetSwing()));
	TestFalse(TEXT("Shut stays shut: no second slam"), Left->NoticePlayer(FVector(100.0, 0.0, 0.0)));

	// The swing itself: speeding up all the way, a bounce off the casing, shut.
	Right->Slam(0.f);
	const float Open = Right->GetSwing();
	Right->AdvanceSlam(Right->SlamSeconds * 0.5f);
	TestTrue(TEXT("Halfway through the slam's time it's three quarters open still (it speeds up)"),
		FMath::IsNearlyEqual(Right->GetSwing(), Open * 0.75f, 0.5f));
	Right->AdvanceSlam(Right->SlamSeconds * 0.5f + 0.05f);
	TestTrue(TEXT("At the casing it bounces back out a little, the way it came"), Right->GetState() == EWindowShutterState::Slamming
		&& FMath::Abs(Right->GetSwing()) > 0.f && FMath::Abs(Right->GetSwing()) < 10.f && FMath::Sign(Right->GetSwing()) == FMath::Sign(Open));
	Right->AdvanceSlam(1.f);
	TestTrue(TEXT("...and settles shut"), Right->IsShut() && FMath::IsNearlyZero(Right->GetSwing()));

	// Main 3 done: an open one slams; a level loaded after it has them shut from the start.
	Right->OpenNow();
	Campaign.Complete(MainThree);
	Right->RefreshStory();
	TestEqual(TEXT("Open when Main 3 ends: it slams"), static_cast<int32>(Right->GetState()), static_cast<int32>(EWindowShutterState::Noticed));
	Right->AdvanceSlam(5.f);
	TestTrue(TEXT("...shut"), Right->IsShut());
	AWindowShutter* Loaded = SpawnShutter(World, FVector(0.0, -200.0, 0.0), LeftMesh);
	if (TestNotNull(TEXT("A shutter as a level loaded after Main 3 begins"), Loaded))
	{
		TestTrue(TEXT("...is shut from the start"), Loaded->IsShut() && FMath::IsNearlyZero(Loaded->GetSwing()));
		TestFalse(TEXT("...and never slams"), Loaded->NoticePlayer(FVector(100.0, -200.0, 0.0)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FColdWelcomePlacedTest, "Looter.Story.ColdWelcome.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FColdWelcomePlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (build_area_story.py): the gate's place and its fight (three Unpaid and a Restless one, on
	// Main 3's second step only, coming as they appear, giving up 30 m out), Tilly's window, the store's shutters, and the
	// safe zones (the farm always, Main Street after Main 3).
	const ULevel* Level = LoadRansomsRest(*this);
	if (!Level)
	{
		return true;
	}
	const AEncounterSpawner* Gate = nullptr;
	const ASpeakerPoint* Tilly = nullptr;
	const ASafeGround* Farm = nullptr;
	const ASafeGround* Street = nullptr;
	int32 Shutters = 0;
	int32 ShuttersAfterMain3 = 0;
	bool bPlace = false;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor); Spawner && Spawner->SpawnerId == GateId)
		{
			Gate = Spawner;
		}
		if (const ASpeakerPoint* Point = Cast<ASpeakerPoint>(Actor); Point && Point->ActorHasTag(TillyTag))
		{
			Tilly = Point;
		}
		if (const ASafeGround* Zone = Cast<ASafeGround>(Actor))
		{
			Farm = Zone->ZoneId == FName(TEXT("Farm")) ? Zone : Farm;
			Street = Zone->ZoneId == FName(TEXT("MainStreet")) ? Zone : Street;
		}
		if (const AWindowShutter* Shutter = Cast<AWindowShutter>(Actor))
		{
			++Shutters;
			ShuttersAfterMain3 += Shutter->ShutWhen.AfterMissions.Contains(MainThree) && Shutter->Leaf->GetStaticMesh() ? 1 : 0;
		}
		bPlace |= Actor->ActorHasTag(GatePlace);
	}
	if (!Gate && !Tilly && !bPlace && Shutters == 0)
	{
		AddWarning(TEXT("Main 3's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	TestTrue(TEXT("The town gate is marked for the farm road's end"), bPlace);
	if (TestNotNull(TEXT("The gate's fight"), Gate))
	{
		TestEqual(TEXT("...three Basic Unpaid"), CountBrought(*Gate, AUnpaidCreature::StaticClass(), ECreatureRank::Basic), 3);
		TestEqual(TEXT("...and one Restless"), CountBrought(*Gate, AUnpaidCreature::StaticClass(), ECreatureRank::Rare), 1);
		TestTrue(TEXT("...on Main 3's second step only"), Gate->ActiveWhen.DuringMission == MainThree && Gate->ActiveWhen.FromStep == 1
			&& Gate->ActiveWhen.BeforeStep == 2);
		TestTrue(TEXT("...coming for the player as they appear"), Gate->bHuntOnSpawn);
		TestTrue(TEXT("...giving up 30 m from the gate"), FMath::IsNearlyEqual(Gate->GiveUpRadius, 3000.f) && Gate->GroundCorners.IsEmpty());
		TestTrue(TEXT("...tagged for the fight"), Gate->CreatureTags.Contains(GateTag));
		TestTrue(TEXT("...standing north-east of the gate"), !Gate->SpawnPoints.IsEmpty() && !Gate->SpawnPoints.ContainsByPredicate([](const FVector& Spot)
		{
			return Spot.X < 0.0 || Spot.Y < 0.0;
		}));
	}
	if (TestNotNull(TEXT("Tilly's window"), Tilly))
	{
		TestEqual(TEXT("...her name"), Tilly->SpeakerPoint->SpeakerName.ToString(), FString(TEXT("Tilly Bright")));
		TestTrue(TEXT("...her Main 3 lines at its last step"), Tilly->SpeakerPoint->Topics.ContainsByPredicate([](const FSpeakerTopic& Topic)
		{
			return Topic.When.DuringMission == MainThree && Topic.When.FromStep == 3 && Topic.LineSet != nullptr;
		}));
	}
	TestTrue(TEXT("Shutters on the store (eight sockets)"), Shutters >= 8);
	TestEqual(TEXT("...each hung, and shut after Main 3"), ShuttersAfterMain3, Shutters);
	TestTrue(TEXT("Delia's salt line keeps the farm safe from the start"), Farm && Farm->ActiveWhen.IsEmpty() && Farm->Corners.Num() >= 3);
	TestTrue(TEXT("Main Street is safe after Main 3"), Street && Street->ActiveWhen.AfterMissions.Contains(MainThree) && Street->Corners.Num() >= 3);
	return true;
}

#endif
