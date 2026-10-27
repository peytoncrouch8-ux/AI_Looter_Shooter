#include "Misc/AutomationTest.h"
#include "World/GraveTravelRules.h"
#include "World/GraveTravelSubsystem.h"
#include "World/RespawnMarker.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// The user's call (2026-10-27, note 24): fast travel stations are the Depot and Calder's Post only. A roost with
// bFastTravelStation off (the town roost by the sheriff's office) stays a save point and a death's wake-up but is no
// destination. The map lists only what passes these rules (MapPins skips the others), so the rules are what is tested.

namespace
{
	ARespawnMarker* PlaceStationTestRoost(UWorld* World, FName Id, const FVector& Where, bool bStation)
	{
		const FTransform At(FRotator::ZeroRotator, Where);
		ARespawnMarker* Roost = World->SpawnActorDeferred<ARespawnMarker>(ARespawnMarker::StaticClass(), At);
		if (Roost)
		{
			Roost->MarkerId = Id;
			Roost->bStartActive = true;
			Roost->bFastTravelStation = bStation;
			Roost->FinishSpawning(At);
		}
		return Roost;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFastTravelStationRulesTest, "Looter.Map.Station.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFastTravelStationRulesTest::RunTest(const FString& Parameters)
{
	using namespace GraveTravelRules;
	FGraveTravelSenses Clear;
	Clear.bHasPlayer = true;
	TestTrue(TEXT("A station, open and far: it may go"), CheckGrave(Clear, true, 5000.0, true) == EGraveTravelBlock::None);
	TestTrue(TEXT("The default is a station"), CheckGrave(Clear, true, 5000.0) == EGraveTravelBlock::None);
	TestTrue(TEXT("Not a station: no, however open and far"), CheckGrave(Clear, true, 5000.0, false) == EGraveTravelBlock::NotAStation);
	TestTrue(TEXT("...closed or near, still not a station"), CheckGrave(Clear, false, 0.0, false) == EGraveTravelBlock::NotAStation);

	FGraveTravelSenses Hunted = Clear;
	Hunted.Hunters = 1;
	TestTrue(TEXT("The fight is named before the station"), CheckGrave(Hunted, true, 5000.0, false) == EGraveTravelBlock::Hunted);

	const FString Words = Reason(EGraveTravelBlock::NotAStation).ToString();
	TestTrue(TEXT("It says why"), !Words.IsEmpty() && Words != Reason(EGraveTravelBlock::GraveClosed).ToString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFastTravelStationWorldTest, "Looter.Map.Station.World",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFastTravelStationWorldTest::RunTest(const FString& Parameters)
{
	// A town roost that is no station beside a real one: the subsystem refuses the first and sends to the second; the
	// refused travel starts nothing. The save point rules (found by walking up, where a death wakes) are the roost's own and
	// untouched by the flag.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UGraveTravelSubsystem* Travel = World->GetSubsystem<UGraveTravelSubsystem>();
	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator);
	ARespawnMarker* Town = PlaceStationTestRoost(World, TEXT("TownTest"), FVector(5000.0, 0.0, 0.0), false);
	ARespawnMarker* Depot = PlaceStationTestRoost(World, TEXT("DepotTest"), FVector(0.0, 6000.0, 0.0), true);
	if (!TestTrue(TEXT("The travel, the player and the roosts"), Travel && Player && Town && Depot))
	{
		return false;
	}

	TestTrue(TEXT("A new roost is a station unless the level says otherwise"), GetDefault<ARespawnMarker>()->bFastTravelStation);
	TestTrue(TEXT("The town roost is open"), Travel->IsOpen(*Town));
	TestTrue(TEXT("...but no destination"), Travel->CheckGrave(Player, *Town) == EGraveTravelBlock::NotAStation);
	TestTrue(TEXT("The station can be travelled to"), Travel->CheckGrave(Player, *Depot) == EGraveTravelBlock::None);

	EGraveTravelBlock Block = EGraveTravelBlock::None;
	TestFalse(TEXT("A travel to the town roost doesn't start"), Travel->TravelTo(Player, *Town, &Block));
	TestTrue(TEXT("...and says it is no station"), Block == EGraveTravelBlock::NotAStation && !Travel->IsTravelling());
	TestTrue(TEXT("The station: on the way"), Travel->TravelTo(Player, *Depot, &Block) && Travel->IsTravelling());
	return true;
}
