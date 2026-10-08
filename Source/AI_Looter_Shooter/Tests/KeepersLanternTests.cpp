#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/HuntingGround.h"
#include "Creatures/SpiderCreature.h"
#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/HobBird.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/KeepersLanternTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/EggSac.h"
#include "World/KeepersLantern.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Main 5, "The Keeper's Lantern" (Docs/Areas/RansomsRest.md): down into the Sink, the spiders on its floor, three egg sacs
// shot down (two spiders each), the Keeper's Lantern taken dark from the webbing, and out at the ramp head; Side 3 after it.
// The sacs' and the lantern's own rules are EggSacTests.cpp's.

using namespace KeepersLanternTestWorld;

namespace
{
	/** Where the test level's Sink has its pieces (cm, round its middle at the origin; the floor is z = 0). */
	const FVector RimHead(0.0, -2200.0, 1200.0);
	const FVector OnFloor(-150.0, 600.0, 90.0);

	/** At most this many web cards in the Sink (Docs/Areas/RansomsRest.md: "at most 20 per view"; a few are never in one view). */
	constexpr int32 MostCards = 24;

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

	FString MeshName(const UStaticMeshComponent* Mesh)
	{
		return Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetName() : FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKeepersLanternMissionTest, "Looter.Story.KeepersLantern.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKeepersLanternMissionTest::RunTest(const FString& Parameters)
{
	// Main 5 as its asset has it: id exactly Main5 (Side 3 opens after it), after Main 4 on Ransom's Rest; down to the
	// Sink's floor (height counted), three egg sacs down (their event, the arrow on the nearest still up), the lantern
	// taken (a tap), out at the ramp head (height counted); 30% of a level.
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main5")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main5.DA_Mission_Main5"));
		if (TestNotNull(TEXT("DA_Mission_Main5 loads"), Asset))
		{
			TestTrue(TEXT("Main5, a main mission starting by itself on Ransom's Rest, after Main4"), Asset->GetMissionId() == MainFive
				&& Asset->Kind == EMissionKind::Main && Asset->Start == EMissionStart::Automatic && Asset->Area == FName(TEXT("RansomsRest"))
				&& Asset->Prerequisites == TArray<FName>({ MainFour }));
			TestEqual(TEXT("Four steps"), Asset->Steps.Num(), MainFiveSteps);
			const UMissionReachObjective* Down = Cast<UMissionReachObjective>(Asset->GetObjective(0, 0));
			const UMissionEventObjective* Sacs = Cast<UMissionEventObjective>(Asset->GetObjective(1, 0));
			const UMissionInteractObjective* Take = Cast<UMissionInteractObjective>(Asset->GetObjective(2, 0));
			const UMissionReachObjective* Out = Cast<UMissionReachObjective>(Asset->GetObjective(3, 0));
			TestTrue(TEXT("1: down into the Sink, on its floor (height counted)"), Down && Down->Place.Actor.ActorTag == FloorPlace
				&& !Down->Place.bIgnoreHeight && Down->Place.Radius < 1200.f);
			TestTrue(TEXT("2: the three egg sacs down, the arrow on the nearest still up"), Sacs && Sacs->Event == AEggSac::BurstEvent
				&& Sacs->Count == 3 && Sacs->Waypoint == EMissionWaypoint::Actor && Sacs->WaypointActor.ActorTag == AEggSac::EggSacTag);
			TestTrue(TEXT("3: the Keeper's Lantern taken, a tap"), Take && Take->Target.ActorTag == AKeepersLantern::LanternTag && !Take->bHold);
			TestTrue(TEXT("4: out at the ramp head (height counted)"), Out && Out->Place.Actor.ActorTag == RimPlace && !Out->Place.bIgnoreHeight);
			TestEqual(TEXT("Its reward: 30% of a level"), Asset->Rewards.ExperienceShare, 0.3f);
			TestEqual(TEXT("The lantern's step is the one AKeepersLantern takes it on"), AKeepersLantern::TakeStep, 2);
		}
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Main5 isn't made yet: run Tools/Unreal/create_mission_assets.py. The flow below runs on a copy."));
	}
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Side3")))
	{
		const UMissionDefinition* Side = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Side3.DA_Mission_Side3"));
		TestTrue(TEXT("Side 3 opens after Main 5"), Side && Side->Prerequisites.Contains(MainFive));
	}
	else
	{
		AddInfo(TEXT("DA_Mission_Side3 isn't made yet: its prerequisite (Main5) isn't checked."));
	}
	// Hob's words through Main 5 and Aldana's after it, as the story script makes them (create_story_lines.py).
	const TCHAR* const LineSets[] = { TEXT("DA_Lines_HobMain5Way"), TEXT("DA_Lines_HobMain5Sacs"), TEXT("DA_Lines_HobMain5Lantern"),
		TEXT("DA_Lines_HobMain5Out"), TEXT("DA_Lines_HobMain5"), TEXT("DA_Lines_AldanaAfterMain5") };
	int32 LinesMade = 0;
	for (const TCHAR* Name : LineSets)
	{
		LinesMade += FPackageName::DoesPackageExist(FString(TEXT("/Game/Data/Story/")) + Name) ? 1 : 0;
	}
	if (LinesMade == 0)
	{
		AddWarning(TEXT("Main 5's line sets aren't made yet: run Tools/Unreal/create_story_lines.py."));
	}
	else
	{
		TestEqual(TEXT("Main 5's line sets are made, every one"), LinesMade, static_cast<int32>(UE_ARRAY_COUNT(LineSets)));
	}

	// Played through in a test level: Main 5 after Main 4; the floor's spiders out as the player looks down from the rim, not
	// hunting them up there; on the floor the sacs, two spiders each; the lantern tapped; out at the ramp head.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, RimHead);
	UInteractionComponent* Interaction = Player ? NewObject<UInteractionComponent>(Player, TEXT("Interaction")) : nullptr;
	if (Interaction)
	{
		Player->AddInstanceComponent(Interaction);
		Interaction->RegisterComponent();
	}
	AActor* Floor = MissionTestWorld::SpawnMarker(World, FVector(0.0, 0.0, 90.0), FloorPlace);
	AActor* Rim = MissionTestWorld::SpawnMarker(World, RimHead, RimPlace);
	AEncounterSpawner* Spiders = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, 0.0, 50.0), SetUpFloor);
	// Hanging round the floor's north side, facing its middle (build_area_sink.py's SACS): A and C off their silk, B slung
	// against the wall 40 cm behind where it lands.
	AEggSac* SacA = SpawnSac(World, FVector(1034.0, -868.0, 300.0), 140.0);
	AEggSac* SacB = SpawnSac(World, FVector(437.0, -1632.0, 280.0), 105.0, 40.f);
	AEggSac* SacC = SpawnSac(World, FVector(1102.0, 689.0, 320.0), -148.0);
	AKeepersLantern* Lantern = PlaceLantern(World, FVector(1644.0, -231.0, 175.0), 172.0);
	if (!Runner || !Player || !Interaction || !Floor || !Rim || !Spiders || !SacA || !SacB || !SacC || !Lantern)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* MainFiveMission = MakeMainFive(Scratch);
	Runner->BeginForTesting({ MakeMainFour(Scratch), MainFiveMission }, Campaign, Player, Valley);
	Lantern->DispatchBeginPlay();
	const TArray<AEggSac*> Sacs = { SacA, SacB, SacC };

	Runner->Update(0.f);
	TestTrue(TEXT("Main 4 first; Main 5 waits for it"), Runner->IsRunning(MainFour) && Runner->GetStatus(*MainFiveMission) == EMissionStatus::Locked);
	Spiders->UpdateEncounter(0.5f);
	TestTrue(TEXT("...the Sink is quiet: no spiders on its floor"), Spiders->GetState() == EEncounterState::Off && Spiders->NumAlive() == 0);
	TestEqual(TEXT("...and a shot takes nothing from a sac"), Shoot(SacA, 500.f), 0.f);

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainFourDone")));
	TestTrue(TEXT("Main 4 done: Main 5 starts by itself, down into the Sink"), Campaign.HasCompleted(MainFour) && Runner->GetStep(MainFive) == 0);
	TestTrue(TEXT("...the floor's spiders are on"), Spiders->IsStoryActive());
	Spiders->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Resident = Spiders->GetAliveCreatures();
	TestEqual(TEXT("Seen from the rim: four spiders among the blocks"), Resident.Num(), FloorSpiders);
	TestFalse(TEXT("...each tagged for the floor"), Resident.ContainsByPredicate([](const ACreatureBase* Spider) { return !Spider->ActorHasTag(FloorTag); }));
	TestFalse(TEXT("...none hunting the player up on the rim"), Resident.ContainsByPredicate([Player](const ACreatureBase* Spider)
	{
		return Spider->GetTarget() == Player;
	}));
	const FHuntingGround Ground = Spiders->MakeHuntingGround();
	TestTrue(TEXT("Their ground: the floor"), Ground.Contains(OnFloor, Spiders->GetActorLocation()));
	TestFalse(TEXT("...not the rim: nothing lives there"), Ground.Contains(RimHead, Spiders->GetActorLocation()));
	TestFalse(TEXT("...nor 4 m up the ramp: they give up at its foot"), Ground.Contains(FVector(-900.0, 1300.0, 450.0), Spiders->GetActorLocation()));
	TestTrue(TEXT("The sacs still can't be shot"), !SacA->CanBeShot() && !SacB->CanBeShot() && !SacC->CanBeShot());

	// Down on the floor: shoot the sacs.
	Player->SetActorLocation(OnFloor);
	Runner->Update(0.2f);
	TestEqual(TEXT("On the floor: shoot down the egg sacs"), Runner->GetStep(MainFive), 1);
	TestTrue(TEXT("...now they can be shot"), SacA->CanBeShot() && SacB->CanBeShot() && SacC->CanBeShot());
	int32 Down = 0;
	for (AEggSac* Sac : Sacs)
	{
		Shoot(Sac, 1000.f);
		FallAll(Sac);
		++Down;
		TestTrue(*FString::Printf(TEXT("Sac %d shot down: burst on the floor, two spiders out"), Down), Sac->GetState() == EEggSacState::Burst
			&& Sac->GetSpiders().Num() == 2);
		if (Down < 3)
		{
			const TArray<FMissionObjectiveView> Views = Runner->GetObjectiveViews(MainFive);
			TestTrue(*FString::Printf(TEXT("...the step counts it: %d / 3"), Down), Runner->GetStep(MainFive) == 1 && Views.Num() == 1
				&& Views[0].Progress == FString::Printf(TEXT("%d / 3"), Down));
		}
	}
	int32 Hatched = 0;
	for (const AEggSac* Sac : Sacs)
	{
		for (const ACreatureBase* Spider : Sac->GetSpiders())
		{
			Hatched += Spider->IsA<ASpiderCreature>() && Spider->GetRank() == ECreatureRank::Basic && Spider->GetTarget() == Player
				&& !Spider->WillRespawn() ? 1 : 0;
		}
	}
	TestEqual(TEXT("Six Basic spiders out of the sacs, every one coming for the player, none to come back"), Hatched, 6);
	TestEqual(TEXT("Three down: take the Keeper's Lantern"), Runner->GetStep(MainFive), 2);

	// The lantern in the webbing: a tap takes it.
	const TOptional<FVector> LanternAt = Lantern->GetInteractionLocation();
	if (TestTrue(TEXT("The lantern can be found"), LanternAt.IsSet()))
	{
		Player->SetActorLocationAndRotation(LanternAt.GetValue() - FVector(150.0, 0.0, Player->BaseEyeHeight), FRotator::ZeroRotator);
	}
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Looked at: \"Take the Keeper's Lantern\", a tap"), Interaction->GetFocusedActor() == Lantern
		&& Interaction->GetFocusedOptions().bTap && Interaction->GetFocusedOptions().TapPrompt.ToString() == TEXT("Take the Keeper's Lantern"));
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Taken: the snare is empty"), !Lantern->IsHanging() && !Lantern->Lantern->IsVisible());
	TestEqual(TEXT("...climb out"), Runner->GetStep(MainFive), 3);
	TestTrue(TEXT("...Ellis has it, by the story"), AKeepersLantern::IsTaken(Campaign, Runner));
	TestFalse(TEXT("...and the floor's spiders' story is over"), Spiders->IsStoryActive());

	// Out at the ramp head: Main 5 is finished.
	Player->SetActorLocation(RimHead + FVector(150.0, 0.0, 0.0));
	Runner->Update(0.2f);
	TestTrue(TEXT("At the ramp head: Main 5 is finished"), Campaign.HasCompleted(MainFive) && !Runner->IsRunning(MainFive));
	TestTrue(TEXT("...the lantern stays Ellis's"), AKeepersLantern::IsTaken(Campaign, Runner) && !Lantern->IsHanging());
	TestFalse(TEXT("...and the sacs stay burst"), Sacs.ContainsByPredicate([](const AEggSac* Sac) { return Sac->GetState() != EEggSacState::Burst; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKeepersLanternPlacedTest, "Looter.Story.KeepersLantern.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKeepersLanternPlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (build_area_sink.py): the floor's and the rim's places, the floor's spiders, three egg sacs (A
	// and C hanging, B in its sling), the lantern in its snare, the web cards by the art's rules (no collision, no shadow, no
	// Nanite, about 20 of them), and Hob's perches in Main 5.
	const ULevel* Level = LoadRansomsRest(*this);
	if (!Level)
	{
		return true;
	}
	TArray<const AEggSac*> Sacs;
	const AKeepersLantern* Lantern = nullptr;
	const AEncounterSpawner* Spiders = nullptr;
	const AHobBird* Hob = nullptr;
	bool bFloor = false;
	bool bRim = false;
	bool bSling = false;
	TArray<const UStaticMeshComponent*> Cards;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (const AEggSac* Sac = Cast<AEggSac>(Actor))
		{
			Sacs.Add(Sac);
		}
		Lantern = Lantern ? Lantern : Cast<AKeepersLantern>(Actor);
		if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor); Spawner && Spawner->SpawnerId == FloorId)
		{
			Spiders = Spawner;
		}
		Hob = Hob ? Hob : Cast<AHobBird>(Actor);
		bFloor |= Actor->ActorHasTag(FloorPlace);
		bRim |= Actor->ActorHasTag(RimPlace);
		if (const AStaticMeshActor* Placed = Cast<AStaticMeshActor>(Actor))
		{
			const FString Name = MeshName(Placed->GetStaticMeshComponent());
			bSling |= Name == TEXT("SM_Web_Sling");
			// The Webwood's crowns stand outside the Sink, on the dead trees north of it.
			if (Name.StartsWith(TEXT("SM_Web_")) && Name != TEXT("SM_Web_Crown"))
			{
				Cards.Add(Placed->GetStaticMeshComponent());
			}
		}
	}
	if (Sacs.IsEmpty() && !Lantern && !Spiders && !bFloor)
	{
		AddWarning(TEXT("Main 5's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	TestTrue(TEXT("The Sink's floor is marked for the way down"), bFloor);
	TestTrue(TEXT("...and the ramp head for the way out"), bRim);

	TestEqual(TEXT("Three egg sacs"), Sacs.Num(), 3);
	TSet<FString> Models;
	for (const AEggSac* Sac : Sacs)
	{
		Models.Add(MeshName(Sac->Sac));
		TestTrue(*FString::Printf(TEXT("%s: tagged for the arrow"), *Sac->GetName()), Sac->ActorHasTag(AEggSac::EggSacTag));
		TestTrue(*FString::Printf(TEXT("%s: shot only in Main 5's second step"), *Sac->GetName()), Sac->ShootableWhen.DuringMission == MainFive
			&& Sac->ShootableWhen.FromStep == 1 && Sac->ShootableWhen.BeforeStep == 2);
		TestTrue(*FString::Printf(TEXT("%s: down from the start after its step and after Main 5"), *Sac->GetName()),
			Sac->DownWhen.ContainsByPredicate([](const FStoryCondition& When) { return When.DuringMission == MainFive && When.FromStep == 2; })
			&& Sac->DownWhen.ContainsByPredicate([](const FStoryCondition& When) { return When.AfterMissions.Contains(MainFive); }));
		TestTrue(*FString::Printf(TEXT("%s: its spiders fight on the floor"), *Sac->GetName()), Sac->SpiderGround.IsSet()
			&& Sac->SpiderGround.MaxRise > 0.f && Sac->SpiderRank == ECreatureRank::Basic && Sac->SpiderClass
			&& Sac->SpiderClass->IsChildOf(ASpiderCreature::StaticClass()));
		TestTrue(*FString::Printf(TEXT("%s: it falls to the floor"), *Sac->GetName()), Sac->DropHeight > 100.f && MeshName(Sac->Burst) == TEXT("SM_EggSac_Burst"));
	}
	TestTrue(TEXT("...one of each: A and C hanging, B in its sling"), Models.Contains(TEXT("SM_EggSac_A")) && Models.Contains(TEXT("SM_EggSac_B"))
		&& Models.Contains(TEXT("SM_EggSac_C")));
	TestTrue(TEXT("...B's sling"), bSling);

	if (TestNotNull(TEXT("The Keeper's Lantern in its snare"), Lantern))
	{
		TestTrue(TEXT("...tagged for its step"), Lantern->ActorHasTag(AKeepersLantern::LanternTag));
		TestTrue(TEXT("...taken in Main 5's third step"), Lantern->TakeWhen.DuringMission == MainFive && Lantern->TakeWhen.FromStep == AKeepersLantern::TakeStep);
		TestTrue(TEXT("...the snare and the lantern, dark"), MeshName(Lantern->Snare) == TEXT("SM_Web_Snare")
			&& MeshName(Lantern->Lantern) == TEXT("SM_KeepersLantern") && !Lantern->bLit);
	}
	if (TestNotNull(TEXT("The floor's spiders"), Spiders))
	{
		TestTrue(TEXT("...four spiders, of the area's ranks"), Spiders->Groups.Num() == 1 && Spiders->Groups[0].Count == FloorSpiders
			&& Spiders->Groups[0].RankRoll == EEncounterRankRoll::Area && Spiders->Groups[0].CreatureClass
			&& Spiders->Groups[0].CreatureClass->IsChildOf(ASpiderCreature::StaticClass()));
		TestTrue(TEXT("...during Main 5, up to the lantern"), Spiders->ActiveWhen.DuringMission == MainFive && Spiders->ActiveWhen.BeforeStep == 3);
		TestTrue(TEXT("...on the floor, giving up at the ramp's foot"), Spiders->GroundCorners.Num() >= 3 && Spiders->GroundMaxRise > 0.f
			&& Spiders->GroundMaxRise < 400.f && Spiders->SpawnPoints.Num() >= FloorSpiders);
	}

	// The web cards, by the art session's rules.
	TestTrue(TEXT("Web cards dress the Sink"), Cards.Num() >= 8);
	if (Cards.Num() > MostCards)
	{
		AddWarning(FString::Printf(TEXT("%d web cards in the Sink: the budget is about 20 in any view."), Cards.Num()));
	}
	for (const UStaticMeshComponent* Card : Cards)
	{
		const UStaticMesh* Mesh = Card->GetStaticMesh();
		TestTrue(*FString::Printf(TEXT("%s (%s): no shadow, no collision"), *Card->GetOwner()->GetName(), *Mesh->GetName()),
			!Card->CastShadow && Card->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
#if WITH_EDITOR
		// The mesh's Nanite setting is editor-only data (a packaged Development build compiles the tests too).
		TestFalse(*FString::Printf(TEXT("%s (%s): no Nanite"), *Card->GetOwner()->GetName(), *Mesh->GetName()), Mesh->IsNaniteEnabled());
#endif
	}
	if (TestNotNull(TEXT("Hob"), Hob))
	{
		TestTrue(TEXT("Hob has perches in Main 5"), Hob->Perches.ContainsByPredicate([](const FHobPerch& Perch)
		{
			return Perch.When.DuringMission == MainFive;
		}));
	}
	return true;
}

#endif
