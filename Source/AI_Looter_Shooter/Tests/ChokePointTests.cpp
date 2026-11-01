#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/HuntingGround.h"
#include "Creatures/RappelSpawner.h"
#include "Creatures/SpiderCreature.h"
#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionPlayerObjectives.h"
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

// Main 5, "Choke Point" (was "The Keeper's Lantern"; MAINS_2_7.md section 7, lanes/MAIN45.md): Junie's charges and down into
// the Sink, the spiders on its floor, three egg sacs shot down (two spiders each), Abel's lantern taken dark from the
// webbing, the barge's intake choked with three salt charges, and out at the ramp head; turned in to Finch; Side 3 after it.
// The sacs' and the lantern's own rules are EggSacTests.cpp's; the intake's, the charges' and the ropes' Main45WorldTests.cpp's.

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChokePointMissionTest, "Looter.Story.ChokePoint.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChokePointMissionTest::RunTest(const FString& Parameters)
{
	// Main 5 as its asset has it (Choke Point since 2026-11-01, MAINS_2_7.md section 7): id exactly Main5 (Side 3 opens after
	// it), offered by Aldana after Main 4; Junie's charges (its item) and down to the Sink's floor (height counted), three egg
	// sacs down, Abel's lantern taken (a tap), three charges on the intake's collar (held) and the blast, out at the ramp head
	// (height counted; the sheriff's hat optional); turned in to Brother Finch at the Reliquary with words of its own; 1,400
	// (3,000 until 2026-11-01: trimmed for the Pacing bands, Docs/Progression.md).
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main5")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main5.DA_Mission_Main5"));
		if (TestNotNull(TEXT("DA_Mission_Main5 loads"), Asset))
		{
			TestTrue(TEXT("Main5 \"Choke Point\", a main mission Aldana offers on Ransom's Rest, after Main4"), Asset->GetMissionId() == MainFive
				&& Asset->Title.ToString() == TEXT("Choke Point") && Asset->Kind == EMissionKind::Main
				&& Asset->Start == EMissionStart::Offered && Asset->Offer.GiverTag == AldanaTag
				&& Asset->Area == FName(TEXT("RansomsRest")) && Asset->Prerequisites == TArray<FName>({ MainFour }));
			TestEqual(TEXT("Five steps"), Asset->Steps.Num(), MainFiveSteps);
			TestTrue(TEXT("Its item: Junie's salt charges"), Asset->Items.ContainsByPredicate([](const FMissionItemInfo& Item)
			{
				return Item.Id == FName(TEXT("SaltCharges"));
			}));
			const UMissionCollectItemObjective* Charges = Cast<UMissionCollectItemObjective>(Asset->GetObjective(0, 0));
			const UMissionReachObjective* Down = Cast<UMissionReachObjective>(Asset->GetObjective(0, 1));
			const UMissionEventObjective* Sacs = Cast<UMissionEventObjective>(Asset->GetObjective(1, 0));
			const UMissionInteractObjective* Take = Cast<UMissionInteractObjective>(Asset->GetObjective(2, 0));
			const UMissionInteractObjective* Set = Cast<UMissionInteractObjective>(Asset->GetObjective(IntakeStep, 0));
			const UMissionEventObjective* Blown = Cast<UMissionEventObjective>(Asset->GetObjective(IntakeStep, 1));
			const UMissionReachObjective* Out = Cast<UMissionReachObjective>(Asset->GetObjective(OutStep, 0));
			TestTrue(TEXT("1: Junie's charges carried"), Charges != nullptr);
			TestTrue(TEXT("...and down into the Sink, on its floor (height counted)"), Down && Down->Place.Actor.ActorTag == FloorPlace
				&& !Down->Place.bIgnoreHeight && Down->Place.Radius < 1200.f);
			TestTrue(TEXT("2: the three egg sacs down, the arrow on the nearest still up"), Sacs && Sacs->Event == AEggSac::BurstEvent
				&& Sacs->Count == 3 && Sacs->Waypoint == EMissionWaypoint::Actor && Sacs->WaypointActor.ActorTag == AEggSac::EggSacTag);
			TestTrue(TEXT("3: Abel's lantern taken, a tap"), Take && Take->Target.ActorTag == AKeepersLantern::LanternTag && !Take->bHold);
			TestTrue(TEXT("4: three charges on the intake's collar, each held"), Set && Set->Target.ActorTag == AIntakeCharge::ChargeTag
				&& Set->Count == 3 && Set->bHold);
			TestTrue(TEXT("...and the blast"), Blown && Blown->Event == ASinkIntake::ChokedEvent);
			TestTrue(TEXT("5: out at the ramp head (height counted)"), Out && Out->Place.Actor.ActorTag == RimPlace && !Out->Place.bIgnoreHeight);
			const UMissionObjective* Hat = Asset->GetObjective(OutStep, 1);
			TestTrue(TEXT("...and, optional, the sheriff's hat by the den"), Hat && Hat->bOptional);
			TestTrue(TEXT("Turned in to Brother Finch, with words of its own"), Asset->NeedsTurnIn() && Asset->TurnIn.SpeakerTag == FinchTag
				&& !Asset->TurnIn.Lines.IsEmpty());
			TestEqual(TEXT("Its reward: 1,400 experience"), Asset->Rewards.Experience, 1400);
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
	// (The sacs' and the lantern's perches went silent 2026-10-21, and Hob's Main 5 tap went.)
	const TCHAR* const LineSets[] = { TEXT("DA_Lines_HobMain5Satchel"), TEXT("DA_Lines_HobMain5Out"), TEXT("DA_Lines_HobMain5Fuse"),
		TEXT("DA_Lines_HobMain5Growl"), TEXT("DA_Lines_HobMain5Mine"), TEXT("DA_Lines_Main5VaneSink"), TEXT("DA_Lines_Main5VaneCharge"),
		TEXT("DA_Lines_Main5VaneChoked"), TEXT("DA_Lines_FinchAfterMain5"), TEXT("DA_Lines_AldanaAfterMain5") };
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
	TestEqual(TEXT("Seen from the rim: six spiders among the blocks"), Resident.Num(), FloorSpiders);
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
	TestEqual(TEXT("...choke the intake"), Runner->GetStep(MainFive), IntakeStep);
	TestTrue(TEXT("...Sully has it, by the story"), AKeepersLantern::IsTaken(Campaign, Runner));
	TestFalse(TEXT("...and the floor's spiders' story is over"), Spiders->IsStoryActive());

	// The intake: three charges on its collar (each a held Interact, as the player's interaction component tells the
	// missions), the fuse, the blast.
	ASinkIntake* Intake = World->SpawnActor<ASinkIntake>(FVector(-450.0, 0.0, 0.0), FRotator::ZeroRotator);
	TArray<AIntakeCharge*> Clamps;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Clamps.Add(World->SpawnActor<AIntakeCharge>(FVector(-450.0, 0.0, 0.0) + FRotator(0.0, Index * 120.0, 0.0).Vector() * 185.0,
			FRotator(0.0, Index * 120.0, 0.0)));
	}
	if (!TestNotNull(TEXT("The intake"), Intake) || Clamps.Contains(nullptr))
	{
		return false;
	}
	Intake->DispatchBeginPlay();
	for (AIntakeCharge* Clamp : Clamps)
	{
		Clamp->DispatchBeginPlay();
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestTrue(*FString::Printf(TEXT("Charge %d set on the collar"), Index + 1), Clamps[Index]->SetCharge(Player));
		Runner->NotifyEvent(FMissionEvent::Interaction(Clamps[Index], /*bHeld*/ true));
		TestFalse(TEXT("...and it can't be set again"), Clamps[Index]->CanSet());
	}
	TestTrue(TEXT("Three on: the fuse burns, the step waits for the blast"), Intake->GetIntakeState() == EIntakeState::Fused
		&& Clamps[0]->IsLit() && Runner->GetStep(MainFive) == IntakeStep);
	Intake->Advance(Intake->FuseSeconds + 0.1f);
	TestTrue(TEXT("The fuse burnt down: choked, its hose swinging loose"), Intake->GetIntakeState() == EIntakeState::Swinging
		&& Intake->DrawsHose());
	TestEqual(TEXT("...climb out"), Runner->GetStep(MainFive), OutStep);

	// Out at the ramp head: Main 5's objectives are done, and it waits for Finch.
	Player->SetActorLocation(RimHead + FVector(150.0, 0.0, 0.0));
	Runner->Update(0.2f);
	TestTrue(TEXT("At the ramp head: ready to turn in to Finch, not finished"), Runner->IsReadyToTurnIn(MainFive) && !Campaign.HasCompleted(MainFive));
	TestTrue(TEXT("...the lantern Sully's meanwhile"), AKeepersLantern::IsTaken(Campaign, Runner) && !Lantern->IsHanging());
	TestFalse(TEXT("...and the sacs stay burst"), Sacs.ContainsByPredicate([](const AEggSac* Sac) { return Sac->GetState() != EEggSacState::Burst; }));

	// Turned in at the Reliquary (the talk's event, as his speaker point sends it): finished.
	Runner->NotifyEvent(FMissionEvent::Named(FMissionEvent::Talk, nullptr, FinchTag));
	TestTrue(TEXT("Turned in to Finch: Main 5 is finished"), Campaign.HasCompleted(MainFive) && !Runner->IsRunning(MainFive));
	TestTrue(TEXT("...the lantern stays Sully's"), AKeepersLantern::IsTaken(Campaign, Runner) && !Lantern->IsHanging());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChokePointPlacedTest, "Looter.Story.ChokePoint.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChokePointPlacedTest::RunTest(const FString& Parameters)
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
		TestTrue(TEXT("...six spiders, of the area's ranks"), Spiders->Groups.Num() == 1 && Spiders->Groups[0].Count == FloorSpiders
			&& Spiders->Groups[0].RankRoll == EEncounterRankRoll::Area && Spiders->Groups[0].CreatureClass
			&& Spiders->Groups[0].CreatureClass->IsChildOf(ASpiderCreature::StaticClass()));
		TestTrue(TEXT("...during Main 5, up to the lantern"), Spiders->ActiveWhen.DuringMission == MainFive && Spiders->ActiveWhen.BeforeStep == 3);
		TestTrue(TEXT("...on the floor, giving up at the ramp's foot"), Spiders->GroundCorners.Num() >= 3 && Spiders->GroundMaxRise > 0.f
			&& Spiders->GroundMaxRise < 400.f && Spiders->SpawnPoints.Num() >= 4);
	}

	// Choke Point's own (build_area_main5.py): the intake and its three clamp spots, the drops down the hose, the ramp's
	// spiders, the satchel.
	const ASinkIntake* Intake = nullptr;
	const ARappelSpawner* Drop = nullptr;
	const AEncounterSpawner* RampSpiders = nullptr;
	int32 Clamps = 0;
	bool bSatchel = false;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		Intake = Intake ? Intake : Cast<ASinkIntake>(Actor);
		Clamps += Actor->IsA<AIntakeCharge>() ? 1 : 0;
		if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor))
		{
			Drop = Spawner->SpawnerId == FName(TEXT("IntakeDrop")) ? Cast<ARappelSpawner>(Spawner) : Drop;
			RampSpiders = Spawner->SpawnerId == FName(TEXT("IntakeSpiders")) ? Spawner : RampSpiders;
		}
		bSatchel |= Actor->ActorHasTag(FName(TEXT("Item_SaltCharges")));
	}
	if (TestNotNull(TEXT("The barge's intake"), Intake))
	{
		TestTrue(TEXT("...drawing in its step, limp after"), Intake->DrawWhen.DuringMission == MainFive && Intake->DrawWhen.FromStep == IntakeStep
			&& Intake->ChokedWhen.Num() >= 2 && !Intake->HoseTop.IsNearlyZero() && Intake->HoseMesh != nullptr);
		TestTrue(TEXT("...its second hose onto Larkspur High"), !Intake->SecondHoseTo.IsNearlyZero() && Intake->SecondHoseWhen.Num() >= 1);
	}
	TestEqual(TEXT("Three clamp spots on its collar"), Clamps, 3);
	if (TestNotNull(TEXT("The drops down the hose (IntakeDrop)"), Drop))
	{
		TestTrue(TEXT("...two ropes, two drops"), Drop->RopeTops.Num() >= 2 && Drop->NumWaves == 2 && Drop->WaveInterval == 0.f);
	}
	TestNotNull(TEXT("The ramp's spiders (IntakeSpiders)"), RampSpiders);
	TestTrue(TEXT("Hob's satchel with Junie's charges at the ramp head"), bSatchel);

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
