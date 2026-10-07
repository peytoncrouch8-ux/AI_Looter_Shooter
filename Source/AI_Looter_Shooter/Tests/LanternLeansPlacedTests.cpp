#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/StationBoard.h"
#include "Session/CampaignRecord.h"
#include "Story/DoorHandoff.h"
#include "Story/HobBird.h"
#include "Tests/LanternLeansTestWorld.h"
#include "World/LanternFlame.h"
#include "World/Train.h"
#include "World/TrainStation.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

// Main 7, "The Lantern Leans", as the scripts make it: the Gilded Lily's area (create_area_assets.py), an area of the story
// with no level yet that the depot's board lists once Main 7 is done ("The line to the Lily isn't open yet."), and Main
// 7's pieces as Ransom's Rest is built (build_area_depot.py): the train, the depot's place, Delia's hand-off, the lantern's
// leaning flame and Hob's perches. The mission played through is LanternLeansTests.cpp's.

using namespace LanternLeansTestWorld;

namespace
{
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanternLeansBoardTest, "Looter.Story.LanternLeans.Board",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLanternLeansBoardTest::RunTest(const FString& Parameters)
{
	// The Lily as the area script makes it: an area of the story with no level yet, "the Lily" in a sentence, so the board
	// lists it once Main 7 is done and choosing it says "The line to the Lily isn't open yet."; and the event the board
	// sends as it opens, which Main 7's last step waits for.
	TestTrue(TEXT("The board's event"), StationBoard::ReadEvent() == FName(TEXT("StationBoard.Read")));
	UPackage* Scratch = CreatePackage(nullptr);
	const UAreaDefinition* Stand = NewLily(Scratch);
	TestEqual(TEXT("A sentence calls it the Lily"), StationBoard::SpokenName(*Stand).ToString(), FString(TEXT("the Lily")));
	UAreaDefinition* Plain = NewArea(Scratch, TEXT("DA_Area_TestPlain"), TEXT("Dustwater"), TEXT("/Game/Maps/Lvl_NotBuiltYet"), TEXT("Landing_Platform"), false);
	TestEqual(TEXT("...an area without a spoken name by its board name"), StationBoard::SpokenName(*Plain).ToString(), FString(TEXT("Dustwater")));
	FStationBoardLine Practice;
	Practice.Kind = EStationLine::Practice;
	Practice.Name = FText::FromString(TEXT("Skyreach (practice)"));
	TestEqual(TEXT("A line with no spoken name says its own"), StationBoard::NotOpenText(FStationBoardWords::Jetty(), Practice).ToString(),
		FString(TEXT("The skiff can't reach Skyreach (practice) yet.")));

	const UAreaDefinition* Lily = FPackageName::DoesPackageExist(TEXT("/Game/Data/Areas/DA_Area_GildedLily"))
		? LoadObject<UAreaDefinition>(nullptr, TEXT("/Game/Data/Areas/DA_Area_GildedLily.DA_Area_GildedLily")) : nullptr;
	if (!Lily)
	{
		AddWarning(TEXT("DA_Area_GildedLily isn't made yet: run Tools/Unreal/create_area_assets.py."));
		return true;
	}
	TestTrue(TEXT("The Gilded Lily, an area of the story"), Lily->GetAreaId() == LilyId && !Lily->bPractice
		&& Lily->DisplayName.ToString() == TEXT("The Gilded Lily"));
	TestFalse(TEXT("...its level not in the game yet"), Lily->HasMap());
	TestTrue(TEXT("...trips arriving at its station"), !Lily->GetDefaultLanding().IsNone());
	FCampaignRecord Opened;
	StationBoard::RecordFirstCastOff(Opened);
	Opened.Complete(MainSeven);
	Opened.OpenArea(LilyId);
	const TArray<FStationBoardLine> Lines = StationBoard::BuildLines(Opened, UAreaDefinition::LoadAll(), {}, StationBoard::FirstAreaId());
	const FStationBoardLine* LilyLine = Lines.FindByPredicate([](const FStationBoardLine& Line) { return Line.AreaId == LilyId; });
	if (TestNotNull(TEXT("Once Main 7 is done the depot's board lists it"), LilyLine))
	{
		TestTrue(TEXT("...a destination nobody can go to yet"), LilyLine->IsDestination() && !LilyLine->bLevelBuilt);
		TestEqual(TEXT("...and choosing it says so"), StationBoard::NotOpenText(FStationBoardWords::Station(), *LilyLine).ToString(),
			FString(TEXT("The line to the Lily isn't open yet.")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanternLeansPlacedTest, "Looter.Story.LanternLeans.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLanternLeansPlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (build_area_depot.py): the train in place of the plain bodies, cold until Main 7, by the
	// depot; the depot's place at the hearse car's door; Delia's hand-off on the farmhouse with her screen door to open;
	// the lantern's leaning flame; Hob's perches in Main 7.
	const ULevel* Level = LoadRansomsRest(*this);
	if (!Level)
	{
		return true;
	}
	const ATrain* Train = nullptr;
	const ATrainStation* Station = nullptr;
	const ADoorHandoff* Handoff = nullptr;
	const ALanternFlame* Flame = nullptr;
	const AHobBird* Hob = nullptr;
	const AActor* Depot = nullptr;
	const AActor* Delia = nullptr;
	int32 PlainBodies = 0;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		Train = Train ? Train : Cast<ATrain>(Actor);
		Station = Station ? Station : Cast<ATrainStation>(Actor);
		Handoff = Handoff ? Handoff : Cast<ADoorHandoff>(Actor);
		Flame = Flame ? Flame : Cast<ALanternFlame>(Actor);
		Hob = Hob ? Hob : Cast<AHobBird>(Actor);
		Depot = Depot || !Actor->ActorHasTag(DepotPlace) ? Depot : Actor;
		Delia = Delia || !Actor->ActorHasTag(DeliasDoor) ? Delia : Actor;
		if (const AStaticMeshActor* Placed = Cast<AStaticMeshActor>(Actor))
		{
			const FString Name = MeshName(Placed->GetStaticMeshComponent());
			PlainBodies += Name == TEXT("SM_HearseCar") || Name == TEXT("SM_PassengerCar") || Name == TEXT("SM_Locomotive_B") ? 1 : 0;
		}
	}
	if (!Train && !Handoff && !Flame && !Depot)
	{
		AddWarning(TEXT("Main 7's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	if (TestNotNull(TEXT("The train at the platform"), Train))
	{
		TestEqual(TEXT("...in place of the plain bodies"), PlainBodies, 0);
		TestTrue(TEXT("...Locomotive B, the passenger car and Tilly's hearse car"), MeshName(Train->HearseCar) == TEXT("SM_HearseCar")
			&& MeshName(Train->PassengerCar) == TEXT("SM_PassengerCar") && MeshName(Train->Locomotive) == TEXT("SM_Locomotive_B"));
		TestTrue(TEXT("...on its wheels: thirteen axles, three drivers, two rods"), Train->DriverWheels->GetInstanceCount() == 3
			&& Train->CarriageWheels->GetInstanceCount() == 10 && Train->Rods->GetInstanceCount() == 2);
		TestTrue(TEXT("...cold and shut until Main 7 (after Main 6)"), !Train->bWarm && Train->WarmWhen.AfterMissions == TArray<FName>({ MainSix }));
		TestTrue(TEXT("...an obstacle for the minimap"), Train->ActorHasTag(TEXT("Obstacle")));
		if (TestNotNull(TEXT("The depot's station"), Station))
		{
			TestTrue(TEXT("...its train the one at its platform"), ATrain::FindNear(Station) == Train);
			const FVector Landing = (Station->LandingTransform * Station->GetActorTransform()).GetLocation();
			TestTrue(TEXT("...trips landing by the hearse car's door"), FVector::Dist2D(Landing, Train->GetArrivalSpot().GetLocation()) < 300.0);
		}
		if (TestNotNull(TEXT("The depot's place"), Depot))
		{
			TestTrue(TEXT("...at the hearse car's door on the platform"), FVector::Dist2D(Depot->GetActorLocation(), Train->GetArrivalSpot().GetLocation()) < 200.0);
		}
	}
	if (TestNotNull(TEXT("Delia's hand-off"), Handoff))
	{
		TestTrue(TEXT("...as Main 7's first step ends"), Handoff->Mission == MainSeven && Handoff->Step == 0);
		const AStaticMeshActor* Screen = Cast<AStaticMeshActor>(Handoff->Door.Get());
		TestTrue(TEXT("...opening her screen door"), Screen && MeshName(Screen->GetStaticMeshComponent()) == TEXT("SM_ScreenDoor"));
		TestTrue(TEXT("...at her door"), Delia && FVector::Dist(Delia->GetActorLocation(), Handoff->GetActorLocation()) < 200.0);
	}
	if (TestNotNull(TEXT("The lantern's flame"), Flame))
	{
		TestTrue(TEXT("...leaning north-east after Main 6"), FMath::IsNearlyEqual(Flame->Bearing, 45.f) && Flame->ShownWhen.AfterMissions == TArray<FName>({ MainSix }));
		TestNotNull(TEXT("...hung on the lantern"), Flame->GetAttachParentActor());
	}
	if (TestNotNull(TEXT("Hob"), Hob))
	{
		TestTrue(TEXT("Hob has perches in Main 7"), Hob->Perches.ContainsByPredicate([](const FHobPerch& Perch) { return Perch.When.DuringMission == MainSeven; }));
	}
	return true;
}

#endif
