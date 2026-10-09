#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/HuntingGround.h"
#include "Creatures/UnpaidCreature.h"
#include "Interaction/InteractionComponent.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/GraveSightSubsystem.h"
#include "Story/HobBird.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/HallowedGroundTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/ChapelBell.h"
#include "World/ChapelReliquary.h"
#include "World/RespawnMarker.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Main 4, "Hallowed Ground" (Docs/Areas/RansomsRest.md): up to the Chapel of Saint Ada, the Unpaid in the chapel yard, the
// bell that won't call them to rest, the smashed Reliquary seen in a Grave Sight flash, Father Aldana at the vestry door;
// the chapel yard's grave after it, and the Unpaid on boot hill and the north road from then on. The bell's and the
// Reliquary's own rules, and Aldana's topics, are ChapelTests.cpp's.

using namespace HallowedGroundTestWorld;

namespace
{
	// The rest of the ids Main 4's pieces go by (build_area_chapel.py): the chapel yard's grave, the roaming stretches.
	const FName YardGrave(TEXT("ChapelYard"));
	const FName BootHillId(TEXT("BootHill"));
	const FName NorthRoadId(TEXT("NorthRoad"));

	FString OnScreen(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT("|") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}

	/** A roaming stretch as the build script sets it up: three Unpaid of the area's ranks after Main 4, standing about. */
	void SetUpRoaming(AEncounterSpawner& Setup, FName Id)
	{
		Setup.SpawnerId = Id;
		FEncounterGroup Roamers;
		Roamers.CreatureClass = AUnpaidCreature::StaticClass();
		Roamers.Count = 3;
		Roamers.RankRoll = EEncounterRankRoll::Area;
		Setup.Groups = { Roamers };
		Setup.CreatureTags = { FName(*(TEXT("Unpaid_") + Id.ToString())) };
		Setup.ActiveWhen.AfterMissions = { MainFour };
		Setup.bHuntOnSpawn = false;
		Setup.SpawnRadius = 600.f;
		Setup.GiveUpRadius = 2500.f;
	}

	/** How many creatures of Class at Rank a spawner's waves bring in all: each group's count, for every wave it joins. */
	int32 CountBrought(const AEncounterSpawner& Spawner, const UClass* Class, ECreatureRank Rank)
	{
		const int32 Waves = FMath::Max(Spawner.NumWaves, 1);
		int32 Brought = 0;
		for (const FEncounterGroup& Group : Spawner.Groups)
		{
			if (!Group.CreatureClass || !Group.CreatureClass->IsChildOf(Class) || Group.RankRoll != EEncounterRankRoll::Fixed
				|| Group.Rank != Rank)
			{
				continue;
			}
			for (int32 Wave = 1; Wave <= Waves; ++Wave)
			{
				Brought += EncounterRules::JoinsWave(Group, Wave) ? Group.Count : 0;
			}
		}
		return Brought;
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallowedGroundMissionTest, "Looter.Story.HallowedGround.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHallowedGroundMissionTest::RunTest(const FString& Parameters)
{
	// Main 4 as its asset has it: id exactly Main4 (Side 2 opens after it, Aldana's Ledger page is known after it), after
	// Main 3 on Ransom's Rest; up to the chapel, the yard cleared (13), the bell rung (held), the Reliquary seen (its flash's
	// event, the arrow on it); turned in to Aldana at the vestry door; 40 experience.
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main4")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main4.DA_Mission_Main4"));
		if (TestNotNull(TEXT("DA_Mission_Main4 loads"), Asset))
		{
			TestTrue(TEXT("Main4, a main mission starting by itself on Ransom's Rest, after Main3"), Asset->GetMissionId() == MainFour
				&& Asset->Kind == EMissionKind::Main && Asset->Start == EMissionStart::Automatic && Asset->Area == FName(TEXT("RansomsRest"))
				&& Asset->Prerequisites == TArray<FName>({ MainThree }));
			TestEqual(TEXT("Four steps"), Asset->Steps.Num(), MainFourSteps);
			const UMissionReachObjective* Up = Cast<UMissionReachObjective>(Asset->GetObjective(0, 0));
			const UMissionClearObjective* Yard = Cast<UMissionClearObjective>(Asset->GetObjective(1, 0));
			const UMissionInteractObjective* Bell = Cast<UMissionInteractObjective>(Asset->GetObjective(2, 0));
			const UMissionEventObjective* Sight = Cast<UMissionEventObjective>(Asset->GetObjective(3, 0));
			TestTrue(TEXT("1: up to the Chapel of Saint Ada"), Up && Up->Place.Actor.ActorTag == ChapelPlace && Up->Place.Radius >= 1000.f);
			TestTrue(TEXT("2: the chapel yard cleared, all thirteen"), Yard && Yard->SpawnerId == YardId && Yard->Count == 13);
			TestTrue(TEXT("3: the chapel bell rung, held"), Bell && Bell->Target.ActorTag == AChapelBell::BellTag && Bell->bHold);
			TestTrue(TEXT("4: the Reliquary seen: done at its flash's end, the arrow on it"), Sight && Sight->Event == AChapelReliquary::SightEvent
				&& Sight->Waypoint == EMissionWaypoint::Actor && Sight->WaypointActor.ActorTag == AChapelReliquary::ReliquaryTag);
			TestTrue(TEXT("Turned in to Father Aldana at the vestry door"), Asset->NeedsTurnIn() && Asset->TurnIn.SpeakerTag == AldanaTag);
			TestEqual(TEXT("Its reward: 40 experience"), Asset->Rewards.Experience, 40);
		}
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Main4 isn't made yet: run Tools/Unreal/create_mission_assets.py. The flow below runs on a copy."));
	}
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Side2")))
	{
		const UMissionDefinition* Side = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Side2.DA_Mission_Side2"));
		TestTrue(TEXT("Side 2 opens after Main 4"), Side && Side->Prerequisites.Contains(MainFour));
	}

	// Played through in a test level: Main 4 waits for Main 3; the yard's fight waits for the chapel; six Unpaid, then six
	// and a Restless one; the bell held; the Reliquary's flash; Aldana's words; the chapel yard's grave opens.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	UGraveSightSubsystem* Sight = UGraveSightSubsystem::Get(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-4000.0, 0.0, 0.0));
	UInteractionComponent* Interaction = Player ? NewObject<UInteractionComponent>(Player, TEXT("Interaction")) : nullptr;
	if (Interaction)
	{
		Player->AddInstanceComponent(Interaction);
		Interaction->RegisterComponent();
	}
	AActor* Chapel = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector, ChapelPlace);
	AEncounterSpawner* Yard = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, SetUpYard);
	// Far enough apart that only the one in front of the player is ever in reach.
	AChapelBell* Bell = SpawnBell(World, FVector(0.0, 3000.0, 64.0));
	AChapelReliquary* Reliquary = SpawnReliquary(World, FVector(0.0, 6000.0, 0.0));
	ASpeakerPoint* Aldana = PlaceAldana(World, FVector(0.0, 9000.0, 170.0));
	if (!Runner || !Captions || !Sight || !Player || !Interaction || !Chapel || !Yard || !Bell || !Reliquary || !Aldana)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Hallowed = MakeMainFour(Scratch);
	Runner->BeginForTesting({ MakeMainThree(Scratch), Hallowed }, Campaign, Player, Valley);
	Aldana->DispatchBeginPlay();
	ARespawnMarker* Grave = World->SpawnActor<ARespawnMarker>(FVector(1500.0, 1500.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The chapel yard's grave"), Grave))
	{
		return false;
	}
	Grave->MarkerId = YardGrave;
	Grave->ActiveAfterMission = MainFour;
	Grave->DispatchBeginPlay();

	Runner->Update(0.f);
	TestTrue(TEXT("Main 3 first; Main 4 waits for it"), Runner->IsRunning(MainThree) && Runner->GetStatus(*Hallowed) == EMissionStatus::Locked);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainThreeDone")));
	TestTrue(TEXT("Main 3 done: Main 4 starts by itself, on the way up"), Campaign.HasCompleted(MainThree) && Runner->GetStep(MainFour) == 0);
	Yard->UpdateEncounter(0.5f);
	TestTrue(TEXT("On the way up the yard is quiet: its fight waits for the chapel"), Yard->GetState() == EEncounterState::Off && Yard->NumAlive() == 0);
	TestFalse(TEXT("The Reliquary can't be looked at yet"), Reliquary->CanLook() || Reliquary->Look(Player));
	TestFalse(TEXT("...nor is the chapel yard's grave open"), Grave->IsActive(Campaign));

	// At the chapel, a step outside its fence: the Unpaid rise in the yard.
	Player->SetActorLocation(FVector(1100.0, 0.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("At the chapel: clear the yard"), Runner->GetStep(MainFour), 1);
	TestTrue(TEXT("...the yard's fight is on"), Yard->IsStoryActive());
	Yard->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> First = Yard->GetAliveCreatures();
	TestEqual(TEXT("The first wave: six Unpaid"), First.Num(), 6);
	TestEqual(TEXT("...all of them Basic"), EncounterTestWorld::CountOf(First, AUnpaidCreature::StaticClass(), ECreatureRank::Basic), 6);
	TestFalse(TEXT("...each tagged for the yard"), First.ContainsByPredicate([](const ACreatureBase* Unpaid) { return !Unpaid->ActorHasTag(YardTag); }));
	const TArray<FVector> FenceLine(Fence, UE_ARRAY_COUNT(Fence));
	TestFalse(TEXT("...each inside the churchyard fence"), First.ContainsByPredicate([&FenceLine](const ACreatureBase* Unpaid)
	{
		return !FHuntingGround::IsInsidePolygon(FenceLine, FVector2D(Unpaid->GetActorLocation()));
	}));
	TestFalse(TEXT("...none within 8 m of the player"), First.ContainsByPredicate([Player](const ACreatureBase* Unpaid)
	{
		return FVector::Dist2D(Unpaid->GetActorLocation(), Player->GetActorLocation()) < 800.0;
	}));
	TestFalse(TEXT("...coming for the player at the fence"), First.ContainsByPredicate([Player](const ACreatureBase* Unpaid)
	{
		return Unpaid->GetTarget() != Player;
	}));
	const FHuntingGround Ground = Yard->MakeHuntingGround();
	TestTrue(TEXT("Their ground: inside the fence, and just past it"), Ground.Contains(FVector(0.0, 1000.0, 0.0), FVector::ZeroVector)
		&& Ground.Contains(FVector(1100.0, 0.0, 0.0), FVector::ZeroVector));
	TestFalse(TEXT("...not 5 m past it: they give up there"), Ground.Contains(FVector(1500.0, 0.0, 0.0), FVector::ZeroVector));

	EncounterTestWorld::KillAll(*Yard);
	Yard->UpdateEncounter(0.5f);
	Runner->Update(0.2f);
	TestTrue(TEXT("The first wave down: the yard isn't clear yet"), Runner->GetStep(MainFour) == 1 && Yard->GetState() != EEncounterState::Cleared);
	Yard->UpdateEncounter(3.f);
	TestEqual(TEXT("...the second wave waits a moment"), Yard->GetWavesStarted(), 1);
	Yard->UpdateEncounter(1.f);
	const TArray<ACreatureBase*> Second = Yard->GetAliveCreatures();
	TestEqual(TEXT("The second wave: seven"), Second.Num(), 7);
	TestEqual(TEXT("...six Basic"), EncounterTestWorld::CountOf(Second, AUnpaidCreature::StaticClass(), ECreatureRank::Basic), 6);
	TestEqual(TEXT("...and one Restless"), EncounterTestWorld::CountOf(Second, AUnpaidCreature::StaticClass(), ECreatureRank::Rare), 1);
	EncounterTestWorld::KillAll(*Yard);
	Yard->UpdateEncounter(0.5f);
	Runner->Update(0.2f);
	TestTrue(TEXT("Thirteen down: the yard is clear"), Yard->GetState() == EEncounterState::Cleared && Yard->GetKilled() == 13);
	TestEqual(TEXT("...ring the bell"), Runner->GetStep(MainFour), 2);
	TestFalse(TEXT("...and the yard's fight is over"), Yard->IsStoryActive());

	// The bell: the rope held in the vestibule.
	Player->SetActorLocationAndRotation(FVector(-150.0, 3000.0, 0.0), FRotator::ZeroRotator);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The bell's rope is what's looked at, to hold"), Interaction->GetFocusedActor() == Bell
		&& Interaction->GetFocusedOptions().bHold && !Interaction->GetFocusedOptions().bTap
		&& Interaction->GetFocusedOptions().HoldPrompt.ToString() == TEXT("Ring the chapel bell"));
	Interaction->PressInteract();
	Interaction->UpdateInteraction(Bell->HoldSeconds * 0.5f);
	TestFalse(TEXT("Half a hold: not yet"), Bell->IsRinging());
	Interaction->UpdateInteraction(Bell->HoldSeconds);
	TestTrue(TEXT("Held: the bell rings"), Bell->IsRinging());
	TestEqual(TEXT("...and nothing answers: look at the Reliquary"), Runner->GetStep(MainFour), 3);

	// The Reliquary: Grave Sight's flash, the step done as it ends.
	TestTrue(TEXT("Now the Reliquary can be looked at"), Reliquary->CanLook());
	TestTrue(TEXT("Looked at"), Reliquary->Look(Player));
	TestTrue(TEXT("...Grave Sight flashes on the screen"), Sight->IsFlashing());
	Reliquary->Advance(1.f);
	Sight->Advance(1.f);
	TestTrue(TEXT("A second in: the ember rising off the lid, the step still waiting"), Reliquary->IsFlashing()
		&& Reliquary->GetEmberLocation().Z > Reliquary->GetEmberStart().Z + 50.0 && Runner->GetStep(MainFour) == 3);
	Reliquary->Advance(1.1f);
	Sight->Advance(1.1f);
	TestTrue(TEXT("The flash over: talk to Father Aldana (its step past the last)"), !Reliquary->IsFlashing() && !Sight->IsFlashing()
		&& Runner->GetStep(MainFour) == 4);
	TestTrue(TEXT("...ready to turn in to him, the yard's grave still closed"), Runner->IsReadyToTurnIn(MainFour) && !Campaign.HasCompleted(MainFour)
		&& !Grave->IsActive(Campaign));

	// Aldana at the vestry door: the doc's words, and Main 4 is turned in.
	Captions->Update(30.f);
	TestTrue(TEXT("Aldana talked to"), Aldana->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("Main 4 is turned in: finished"), Campaign.HasCompleted(MainFour) && !Runner->IsRunning(MainFour));
	TestTrue(TEXT("...and the chapel yard is a respawn grave"), Grave->IsActive(Campaign) && Campaign.IsRespawnActive(YardGrave));
	bool bSink = false;
	for (int32 Line = 0; Line < 16 && !bSink && Captions->GetCurrent(); ++Line)
	{
		bSink = OnScreen(*Captions) == FString(TEXT("Father Aldana|")) + AldanaWords[AldanaWordCount - 1];
		Captions->Update(Captions->GetCurrent()->Seconds + 0.01f);
	}
	TestTrue(TEXT("...his words end \"Look in the Sink.\""), bSink);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallowedGroundRoamingTest, "Looter.Story.HallowedGround.Roaming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHallowedGroundRoamingTest::RunTest(const FString& Parameters)
{
	// "From now on the Unpaid walk the north road and boot hill": their spawners are off until Main 4 is finished and on for
	// good after it; three each, of the area's ranks, standing about rather than coming at the player as they appear.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1200.0, 0.0, 0.0));
	AEncounterSpawner* Hill = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector,
		[](AEncounterSpawner& Setup) { SetUpRoaming(Setup, BootHillId); });
	AEncounterSpawner* Road = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, 12000.0, 0.0),
		[](AEncounterSpawner& Setup) { SetUpRoaming(Setup, NorthRoadId); });
	if (!Runner || !Player || !Hill || !Road)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* MainFourStandIn = MissionTestWorld::NewMission(Scratch, TEXT("Main4"), EMissionKind::Main, EMissionStart::Automatic, Valley);
	MissionTestWorld::AddObjective<UMissionEventObjective>(MainFourStandIn, 0)->Event = TEXT("Test.MainFourDone");
	Runner->BeginForTesting({ MainFourStandIn }, Campaign, Player, Valley);
	Runner->Update(0.f);
	Hill->UpdateEncounter(0.5f);
	TestTrue(TEXT("During Main 4: boot hill is quiet"), Runner->IsRunning(MainFour) && Hill->GetState() == EEncounterState::Off
		&& Hill->NumAlive() == 0);

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainFourDone")));
	TestTrue(TEXT("Main 4 done: both stretches are on"), Campaign.HasCompleted(MainFour) && Hill->IsStoryActive() && Road->IsStoryActive());
	Hill->UpdateEncounter(0.5f);
	Road->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Out = Hill->GetAliveCreatures();
	TestEqual(TEXT("Boot hill, the player near: three Unpaid"), EncounterTestWorld::CountOf(Out, AUnpaidCreature::StaticClass(),
		ECreatureRank::Basic), 3);
	TestFalse(TEXT("...each tagged for its stretch"), Out.ContainsByPredicate([](const ACreatureBase* Unpaid)
	{
		return !Unpaid->ActorHasTag(FName(TEXT("Unpaid_BootHill")));
	}));
	TestFalse(TEXT("...walking about, not coming at the player as they appear"), Out.ContainsByPredicate([](const ACreatureBase* Unpaid)
	{
		return Unpaid->GetTarget() != nullptr;
	}));
	TestTrue(TEXT("The north road, the player far: waiting for them"), Road->GetState() == EEncounterState::Waiting && Road->NumAlive() == 0);
	TestTrue(TEXT("Their ranks are the area's, Restless and Gravebound among them now and then"),
		Hill->Groups.Num() == 1 && Hill->Groups[0].RankRoll == EEncounterRankRoll::Area);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallowedGroundPlacedTest, "Looter.Story.HallowedGround.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHallowedGroundPlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (build_area_chapel.py): the chapel's place, the yard's fight, the bell with its bell up in the
	// belfry, the Reliquary on its plinth, Aldana's door, the chapel yard's grave after Main 4, the Unpaid on boot hill and
	// the north road after it, and Hob's perches at the chapel.
	const ULevel* Level = LoadRansomsRest(*this);
	if (!Level)
	{
		return true;
	}
	const AEncounterSpawner* Yard = nullptr;
	const AEncounterSpawner* Hill = nullptr;
	const AEncounterSpawner* Road = nullptr;
	const AChapelBell* Bell = nullptr;
	const AChapelReliquary* Reliquary = nullptr;
	const ASpeakerPoint* Aldana = nullptr;
	const ARespawnMarker* Grave = nullptr;
	const AHobBird* Hob = nullptr;
	bool bPlace = false;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor))
		{
			Yard = Spawner->SpawnerId == YardId ? Spawner : Yard;
			Hill = Spawner->SpawnerId == BootHillId ? Spawner : Hill;
			Road = Spawner->SpawnerId == NorthRoadId ? Spawner : Road;
		}
		Bell = Bell ? Bell : Cast<AChapelBell>(Actor);
		Reliquary = Reliquary ? Reliquary : Cast<AChapelReliquary>(Actor);
		if (const ASpeakerPoint* Point = Cast<ASpeakerPoint>(Actor); Point && Point->ActorHasTag(AldanaTag))
		{
			Aldana = Point;
		}
		if (const ARespawnMarker* Marker = Cast<ARespawnMarker>(Actor); Marker && Marker->MarkerId == YardGrave)
		{
			Grave = Marker;
		}
		Hob = Hob ? Hob : Cast<AHobBird>(Actor);
		bPlace |= Actor->ActorHasTag(ChapelPlace);
	}
	if (!Yard && !Bell && !Reliquary && !Aldana && !bPlace)
	{
		AddWarning(TEXT("Main 4's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	TestTrue(TEXT("The chapel's middle is marked for the way up"), bPlace);
	if (TestNotNull(TEXT("The chapel yard's fight"), Yard))
	{
		TestEqual(TEXT("...twelve Basic Unpaid over its waves"), CountBrought(*Yard, AUnpaidCreature::StaticClass(), ECreatureRank::Basic), 12);
		TestEqual(TEXT("...and one Restless"), CountBrought(*Yard, AUnpaidCreature::StaticClass(), ECreatureRank::Rare), 1);
		TestTrue(TEXT("...in two waves, the second once the first is down"), Yard->NumWaves == 2 && Yard->bWaitForClear);
		TestTrue(TEXT("...on Main 4's second step only"), Yard->ActiveWhen.DuringMission == MainFour && Yard->ActiveWhen.FromStep == 1
			&& Yard->ActiveWhen.BeforeStep == 2);
		TestTrue(TEXT("...coming for the player as they appear"), Yard->bHuntOnSpawn);
		TestTrue(TEXT("...inside the churchyard fence"), Yard->GroundCorners.Num() >= 3 && Yard->SpawnPoints.Num() >= 7);
		TestTrue(TEXT("...tagged for the fight"), Yard->CreatureTags.Contains(YardTag));
	}
	if (TestNotNull(TEXT("The chapel bell"), Bell))
	{
		TestTrue(TEXT("...tagged for its step"), Bell->ActorHasTag(AChapelBell::BellTag));
		// Its place as the level keeps it (relative to the rope): a level loaded only to look at it has no world transforms.
		TestTrue(TEXT("...its bell hung up in the belfry, over the rope"), Bell->Bell && Bell->Bell->GetStaticMesh()
			&& Bell->Bell->GetRelativeLocation().Z > 500.0);
	}
	if (TestNotNull(TEXT("The Reliquary"), Reliquary))
	{
		TestTrue(TEXT("...tagged for the arrow"), Reliquary->ActorHasTag(AChapelReliquary::ReliquaryTag));
		TestTrue(TEXT("...looked at from Main 4's fourth step"), Reliquary->LookWhen.DuringMission == MainFour && Reliquary->LookWhen.FromStep == 3);
		if (!Reliquary->HasModel())
		{
			AddInfo(TEXT("The smashed Reliquary's model isn't placed: its stand-in shows until Reliquary.py is imported and the build runs again."));
		}
	}
	if (TestNotNull(TEXT("Father Aldana's vestry door"), Aldana))
	{
		TestEqual(TEXT("...his name"), Aldana->SpeakerPoint->SpeakerName.ToString(), FString(TEXT("Father Aldana")));
		TestTrue(TEXT("...his words at Main 4's last step"), Aldana->SpeakerPoint->Topics.ContainsByPredicate([](const FSpeakerTopic& Topic)
		{
			return Topic.When.DuringMission == MainFour && Topic.When.FromStep == 4 && Topic.LineSet != nullptr;
		}));
	}
	if (TestNotNull(TEXT("The chapel yard's respawn grave"), Grave))
	{
		TestTrue(TEXT("...open after Main 4"), Grave->ActiveAfterMission == MainFour && !Grave->bStartActive);
	}
	for (const AEncounterSpawner* Stretch : { Hill, Road })
	{
		if (TestNotNull(TEXT("A roaming stretch (boot hill, the north road)"), Stretch))
		{
			TestTrue(*FString::Printf(TEXT("%s: Unpaid of the area's ranks, after Main 4, standing about"), *Stretch->SpawnerId.ToString()),
				Stretch->ActiveWhen.AfterMissions.Contains(MainFour) && !Stretch->bHuntOnSpawn && !Stretch->Groups.IsEmpty()
				&& Stretch->Groups[0].RankRoll == EEncounterRankRoll::Area && Stretch->Groups[0].CreatureClass
				&& Stretch->Groups[0].CreatureClass->IsChildOf(AUnpaidCreature::StaticClass()));
		}
	}
	if (TestNotNull(TEXT("Hob"), Hob))
	{
		TestTrue(TEXT("Hob has perches in Main 4"), Hob->Perches.ContainsByPredicate([](const FHobPerch& Perch)
		{
			return Perch.When.DuringMission == MainFour;
		}));
	}
	return true;
}

#endif
