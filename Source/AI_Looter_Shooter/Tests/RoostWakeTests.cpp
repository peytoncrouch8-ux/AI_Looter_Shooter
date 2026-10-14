#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaLandings.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/MissionTestWorld.h"
#include "World/GraveMarkRules.h"
#include "World/GraveTravelRules.h"
#include "World/GraveTravelSubsystem.h"
#include "World/RespawnMarker.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"

// Where a death wakes the player since the user's call of 2026-10-14 ("only 2 fast travel crow stations per map"; "deaths
// should respawn you at the travel station you last activated"): the roost last found on the map (walked up to, or arrived
// at by fast travel), wherever they fell, never the nearest; before any is found, the level's own start; a save from the
// days of eleven roosts loads onto today's two; and the built maps keep two at most.

using namespace MissionTestWorld;

namespace
{
	const FName WakeValley(TEXT("TestValley"));

	/** A roost in a test level, named and opened (or not) before its play begins. */
	ARespawnMarker* PlaceWakeRoost(UWorld* World, FName Id, const FVector& Where, double Yaw, bool bOpenFromStart = false,
		FName OpenedBy = NAME_None)
	{
		ARespawnMarker* Roost = World->SpawnActor<ARespawnMarker>(Where, FRotator(0.0, Yaw, 0.0));
		if (Roost)
		{
			Roost->MarkerId = Id;
			Roost->bStartActive = bOpenFromStart;
			Roost->ActiveAfterMission = OpenedBy;
		}
		return Roost;
	}

	/** A player start: the level's own, or a trip's landing with a landing's tag. */
	APlayerStart* PlaceWakeStart(UWorld* World, const FVector& Where, double Yaw, FName StartTag = NAME_None)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		APlayerStart* Start = World->SpawnActor<APlayerStart>(Where, FRotator(0.0, Yaw, 0.0), Params);
		if (Start)
		{
			Start->PlayerStartTag = StartTag;
		}
		return Start;
	}

	/** A roost found as a walk up to it records it (opened, found, the area's save point), without the walk. */
	void WakeRecordFound(FCampaignRecord& Campaign, FName Id)
	{
		Campaign.ActivateRespawn(Id);
		GraveMarkRules::Record(Campaign, WakeValley, Id);
	}

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WakeWriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostWakeLastFoundTest, "Looter.Roost.Wake.LastFound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostWakeLastFoundTest::RunTest(const FString& Parameters)
{
	// A death wakes the player at the roost they last found here, wherever they fell: never the nearest, never one only
	// opened (by the story or from the start), never a trip's landing or a closed one even when the record names it.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector(-9000.0, 9000.0, 0.0));
	ARespawnMarker* Depot = PlaceWakeRoost(World, TEXT("Depot"), FVector(0.0, 0.0, 0.0), 135.0);
	ARespawnMarker* Calders = PlaceWakeRoost(World, TEXT("CaldersPost"), FVector(20000.0, 0.0, 0.0), -100.0);
	ARespawnMarker* Opened = PlaceWakeRoost(World, TEXT("Opened"), FVector(1500.0, 0.0, 0.0), 0.0, /*bOpenFromStart*/ true);
	ARespawnMarker* Platform = PlaceWakeRoost(World, TEXT("Platform"), FVector(600.0, 0.0, 0.0), 0.0, true);
	ARespawnMarker* Shut = PlaceWakeRoost(World, TEXT("Shut"), FVector(3000.0, 0.0, 0.0), 0.0, false, TEXT("TestNeverDone"));
	APlayerStart* LevelStart = PlaceWakeStart(World, FVector(-5000.0, 0.0, 100.0), 30.0);
	if (!TestTrue(TEXT("The runner, the player, the roosts and the level's start"), Runner && Player && Depot && Calders && Opened && Platform
		&& Shut && LevelStart))
	{
		return false;
	}
	Platform->Tags.Add(FName(TEXT("Landing_Depot")));
	Runner->BeginForTesting({}, Campaign, Player, WakeValley);
	auto WakeAt = [World, &Campaign]() { return ARespawnMarker::ChooseWakeSpot(World, &Campaign); };

	TestTrue(TEXT("Nothing found: the level's start, though open roosts stand nearer"), WakeAt().Start == LevelStart && !WakeAt().Grave);

	Player->SetActorLocation(Depot->GetActorLocation() + FVector(300.0, 0.0, 0.0));
	TestTrue(TEXT("The Depot found by walking up"), Depot->TryMark(Player) == EGraveMark::First);
	FRespawnWakeSpot Spot = WakeAt();
	TestTrue(TEXT("A death wakes at the Depot, carried by Hob's crows"), Spot.Grave == Depot && !Spot.Start && Spot.bByCrows);
	TestTrue(TEXT("...standing on its spot, facing its way"), Spot.Location.Equals(Depot->GetActorLocation())
		&& FMath::IsNearlyEqual(Spot.Facing.Yaw, 135.0, 0.01));

	Player->SetActorLocation(Calders->GetActorLocation() + FVector(0.0, 300.0, 0.0));
	TestTrue(TEXT("Calder's Post found"), Calders->TryMark(Player) == EGraveMark::First);
	TestTrue(TEXT("A death wakes at Calder's Post, the last found"), WakeAt().Grave == Calders);
	// The poll never runs in a test level: standing by the Depot finds nothing again, and where they fall doesn't count.
	Player->SetActorLocation(Depot->GetActorLocation() + FVector(200.0, 0.0, 0.0));
	TestTrue(TEXT("Fallen beside the Depot: still Calder's Post (never the nearest)"), WakeAt().Grave == Calders);
	TestTrue(TEXT("...the save point the map shows"), Calders->IsSavePoint(Campaign, WakeValley) && !Depot->IsSavePoint(Campaign, WakeValley));
	TestTrue(TEXT("Back at the Depot: found again"), Depot->TryMark(Player) == EGraveMark::Again);
	TestTrue(TEXT("...a death wakes there now"), WakeAt().Grave == Depot);
	TestFalse(TEXT("An open roost never walked up to is never woken at"), WakeAt().Grave == Opened);

	// The record naming somewhere no death may wake: the newest find still standing and open takes its place.
	FCampaignRecord OnlyDepot;
	WakeRecordFound(OnlyDepot, TEXT("Depot"));
	OnlyDepot.SetLastGrave(WakeValley, TEXT("Platform"));
	TestTrue(TEXT("The record naming a trip's landing: never; the Depot"), ARespawnMarker::ChooseWakeSpot(World, &OnlyDepot).Grave == Depot);
	OnlyDepot.SetLastGrave(WakeValley, TEXT("Shut"));
	TestTrue(TEXT("The record naming a closed roost: the Depot"), ARespawnMarker::ChooseWakeSpot(World, &OnlyDepot).Grave == Depot);

	// Another area's save point is not this one's; without a story, nothing was found.
	FCampaignRecord Elsewhere;
	Elsewhere.ActivateRespawn(TEXT("CaldersPost"));
	Elsewhere.SetLastGrave(TEXT("TestElsewhere"), TEXT("CaldersPost"));
	TestTrue(TEXT("Another area's save point: the level's start"), ARespawnMarker::ChooseWakeSpot(World, &Elsewhere).Start == LevelStart);
	Spot = ARespawnMarker::ChooseWakeSpot(World, nullptr);
	TestTrue(TEXT("No story: the level's start, the plain fade"), Spot.Start == LevelStart && !Spot.Grave && !Spot.bByCrows);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostWakeBeforeAnyFoundTest, "Looter.Roost.Wake.BeforeAnyFound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostWakeBeforeAnyFoundTest::RunTest(const FString& Parameters)
{
	// A new game: Ellis climbs out at the family plot with the Depot and Calder's Post still dark. A death then wakes at the
	// level's own start (where its middle goes, facing its way), never at a trip's landing, carried by Hob's crows on a map
	// with roosts; a map without (Skyreach) keeps the plain fade, and a level with no start of its own has nowhere.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector(9000.0, 500.0, 0.0));
	ARespawnMarker* Depot = PlaceWakeRoost(World, TEXT("Depot"), FVector(10000.0, 0.0, 0.0), 135.0);
	ARespawnMarker* Calders = PlaceWakeRoost(World, TEXT("CaldersPost"), FVector(20000.0, 0.0, 0.0), -100.0);
	APlayerStart* FamilyPlot = PlaceWakeStart(World, FVector(-3660.0, -8650.0, 100.0), 34.9);
	APlayerStart* Train = PlaceWakeStart(World, FVector(9500.0, 0.0, 100.0), 0.0, TEXT("Landing_Depot"));
	if (!TestTrue(TEXT("The runner, the player, two roosts and two starts"), Runner && Player && Depot && Calders && FamilyPlot && Train))
	{
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player, WakeValley);

	FRespawnWakeSpot Spot = ARespawnMarker::ChooseWakeSpot(World, &Campaign);
	TestTrue(TEXT("Nothing found: the level's start (the family plot), not the train's nearer landing"), !Spot.Grave && Spot.Start == FamilyPlot);
	TestTrue(TEXT("...where its middle goes, facing its way"), Spot.Location.Equals(FamilyPlot->GetActorLocation())
		&& FMath::IsNearlyEqual(Spot.Facing.Yaw, 34.9, 0.01));
	TestTrue(TEXT("...carried there by Hob's crows (a map with roosts, the story played)"), Spot.bByCrows);

	// Open is not found: the story opening the Depot, or a roost open from the start, wakes no one there.
	Campaign.ActivateRespawn(TEXT("Depot"));
	Calders->bStartActive = true;
	TestTrue(TEXT("Both open, neither found: still the family plot"), Depot->IsActive(Campaign) && Calders->IsActive(Campaign)
		&& ARespawnMarker::ChooseWakeSpot(World, &Campaign).Start == FamilyPlot);
	TestNull(TEXT("...and no save point for Continue (they stand where they were saved)"),
		ARespawnMarker::FindLastFound(World, Campaign, WakeValley));

	// Found at last: there from now on.
	Player->SetActorLocation(Depot->GetActorLocation() + FVector(300.0, 0.0, 0.0));
	TestTrue(TEXT("The Depot found"), Depot->TryMark(Player) == EGraveMark::First && ARespawnMarker::ChooseWakeSpot(World, &Campaign).Grave == Depot);

	// No story, and a map without roosts: the start with the plain fade.
	FCampaignRecord Fresh;
	TestFalse(TEXT("No story: the start, no crows"), ARespawnMarker::ChooseWakeSpot(World, nullptr).bByCrows);
	ARespawnMarker* Platform = PlaceWakeRoost(World, TEXT("Platform"), FVector(9800.0, 0.0, 0.0), 0.0, true);
	if (Platform)
	{
		Platform->Tags.Add(FName(TEXT("Landing_Depot")));
	}
	Depot->Destroy();
	Calders->Destroy();
	TestFalse(TEXT("Only a trip's landing left: no roosts of Hob's"), ARespawnMarker::HasRoosts(World));
	Spot = ARespawnMarker::ChooseWakeSpot(World, &Fresh);
	TestTrue(TEXT("A map without roosts (Skyreach): the start, the plain fade"), Spot.Start == FamilyPlot && !Spot.bByCrows);
	FamilyPlot->Destroy();
	TestFalse(TEXT("No start of its own and nothing found: nowhere (they wake where they fell)"), ARespawnMarker::ChooseWakeSpot(World, &Fresh).IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostWakeTravelFindsTest, "Looter.Roost.Wake.TravelFinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostWakeTravelFindsTest::RunTest(const FString& Parameters)
{
	// Arriving by fast travel touches the roost as walking up does: it becomes the save point, where a death and Continue
	// wake the player; quietly for one found before (the crows' welcome is the show), with its first find for one the story
	// opened but the player never walked up to.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UGraveTravelSubsystem* Travel = World->GetSubsystem<UGraveTravelSubsystem>();
	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(-9000.0, 0.0, 100.0), FRotator::ZeroRotator);
	ARespawnMarker* Depot = PlaceWakeRoost(World, TEXT("Depot"), FVector(0.0, 0.0, 0.0), 135.0);
	ARespawnMarker* Calders = PlaceWakeRoost(World, TEXT("CaldersPost"), FVector(20000.0, 0.0, 0.0), -100.0);
	ARespawnMarker* StoryOpened = PlaceWakeRoost(World, TEXT("StoryOpened"), FVector(0.0, 20000.0, 0.0), 90.0, /*bOpenFromStart*/ true);
	if (!TestTrue(TEXT("The runner, the travel, the player and three roosts"), Runner && Travel && Player && Depot && Calders && StoryOpened))
	{
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player, WakeValley);
	auto WakeAt = [World, &Campaign]() { return ARespawnMarker::ChooseWakeSpot(World, &Campaign).Grave; };
	auto Ride = [Travel, Player](const ARespawnMarker& To)
	{
		const bool bOff = Travel->TravelTo(Player, To);
		Travel->Advance(GraveTravelRules::FadeOutSeconds + GraveTravelRules::HoldSeconds);
		return bOff && !Travel->IsTravelling()
			&& Player->GetActorLocation().Equals(GraveTravelRules::ArrivalLocation(To.GetActorLocation(), Player->GetDefaultHalfHeight()), 1.0);
	};

	// Both found on foot, Calder's Post last; the find's flare let run out.
	Player->SetActorLocation(Depot->GetActorLocation() + FVector(300.0, 0.0, 100.0));
	TestTrue(TEXT("The Depot found on foot"), Depot->TryMark(Player) == EGraveMark::First);
	Player->SetActorLocation(Calders->GetActorLocation() + FVector(300.0, 0.0, 100.0));
	TestTrue(TEXT("Calder's Post found on foot, last"), Calders->TryMark(Player) == EGraveMark::First && WakeAt() == Calders);
	Depot->AdvanceLook(5.f);

	// A ride to the Depot: the save point moves with it.
	TestTrue(TEXT("Fast travel to the Depot, set down on its spot"), Ride(*Depot));
	TestTrue(TEXT("...the Depot is the save point now"), Campaign.GetLastGrave(WakeValley) == FName(TEXT("Depot")));
	TestTrue(TEXT("...a death wakes there, not at Calder's Post"), WakeAt() == Depot);
	TestTrue(TEXT("...and so does Continue"), ARespawnMarker::FindLastFound(World, Campaign, WakeValley) == Depot);
	TestFalse(TEXT("...quietly: no find's flare (the crows' welcome is the show)"), Depot->IsFlaring());

	// A ride to one the story opened but nobody walked up to: arriving finds it.
	TestTrue(TEXT("Open, never found"), StoryOpened->IsActive(Campaign) && !StoryOpened->IsMarked(Campaign));
	TestTrue(TEXT("Fast travel to it"), Ride(*StoryOpened));
	TestTrue(TEXT("...found as it's arrived at: lit, the save point"), StoryOpened->IsMarked(Campaign) && StoryOpened->IsLit()
		&& Campaign.GetLastGrave(WakeValley) == FName(TEXT("StoryOpened")));
	TestTrue(TEXT("...where a death wakes now"), WakeAt() == StoryOpened);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostWakeRemovedRoostTest, "Looter.Roost.Wake.RemovedRoost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostWakeRemovedRoostTest::RunTest(const FString& Parameters)
{
	// A session saved when Ransom's Rest had eleven roosts (the family plot, the Depot, Main Street and the Sink found, the
	// Sink last) loads cleanly onto today's two: the newest find still standing (the Depot) takes the gone save point's place
	// for a death, Continue and the map; with none of its finds standing, a death wakes at the level's start and Continue
	// where they were saved; the next roost found is written as the save point.
	ULooterSessionSave* Old = NewObject<ULooterSessionSave>();
	Old->Version = ULooterSessionSave::CurrentVersion;
	for (const FName Id : { FName(TEXT("FamilyPlot")), FName(TEXT("Depot")), FName(TEXT("MainStreet")), FName(TEXT("SinkRim")) })
	{
		WakeRecordFound(Old->Campaign, Id);
	}
	ULooterSessionSave* Loaded = WakeWriteAndRead(Old);
	if (!TestNotNull(TEXT("The old session saved and read"), Loaded))
	{
		return false;
	}
	TestTrue(TEXT("...its save point the Sink, gone from today's level"), Loaded->Campaign.GetLastGrave(WakeValley) == FName(TEXT("SinkRim")));

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Stand = SpawnMarker(World, FVector(5000.0, 5000.0, 0.0));
	ARespawnMarker* Depot = PlaceWakeRoost(World, TEXT("Depot"), FVector(0.0, 0.0, 0.0), 135.0);
	ARespawnMarker* Calders = PlaceWakeRoost(World, TEXT("CaldersPost"), FVector(20000.0, 0.0, 0.0), -100.0);
	APlayerStart* LevelStart = PlaceWakeStart(World, FVector(-5000.0, 0.0, 100.0), 30.0);
	ACharacter* Pawn = World->SpawnActor<ACharacter>(FVector(111.0, 222.0, 100.0), FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!TestTrue(TEXT("The runner, the roosts, the start and a player"), Runner && Stand && Depot && Calders && LevelStart && Pawn && Controller))
	{
		return false;
	}
	// A test world never readies its actors: the controller joins the world's list, as play would add it, and holds the pawn.
	World->AddController(Controller);
	Controller->SetPawn(Pawn);
	FCampaignRecord& Read = Loaded->Campaign;
	Runner->BeginForTesting({}, Read, Stand, WakeValley);

	TestTrue(TEXT("The save point gone: the newest find still standing, the Depot"), ARespawnMarker::FindLastFound(World, Read, WakeValley) == Depot);
	TestTrue(TEXT("...where a death wakes"), ARespawnMarker::ChooseWakeSpot(World, &Read).Grave == Depot);
	TestTrue(TEXT("...and the map's save point"), Depot->IsSavePoint(Read, WakeValley) && !Calders->IsSavePoint(Read, WakeValley));
	FCampaignRecord BothFound = Read;
	BothFound.ActivateRespawn(TEXT("CaldersPost"));
	BothFound.MarkedGraves.Insert(FName(TEXT("CaldersPost")), 2);
	TestTrue(TEXT("Calder's Post found after the Depot (before the Sink): Calder's Post"),
		ARespawnMarker::FindLastFound(World, BothFound, WakeValley) == Calders);

	// Continue: the player is put at the Depot, facing its arrow.
	Loaded->Map = USessionSubsystem::MapOf(World);
	Loaded->bHasPlayerSpot = true;
	Loaded->PlayerLocation = FVector(5000.0, 5000.0, 100.0);
	Loaded->PlayerView = FRotator(10.0, 45.0, 0.0);
	USessionSubsystem::RestoreWorld(World, *Loaded);
	TestTrue(TEXT("Continue: on the Depot's spot"), Pawn->GetActorLocation().Equals(
		GraveTravelRules::ArrivalLocation(Depot->GetActorLocation(), Pawn->GetDefaultHalfHeight()), 1.0));
	TestTrue(TEXT("...facing its arrow"), FMath::IsNearlyEqual(FRotator::NormalizeAxis(Pawn->GetActorRotation().Yaw), 135.0, 0.5));

	// The next walk up writes the record right again.
	Stand->SetActorLocation(Depot->GetActorLocation() + FVector(300.0, 0.0, 0.0));
	TestTrue(TEXT("The Depot walked up to: found again"), Depot->TryMark(Stand) == EGraveMark::Again);
	TestTrue(TEXT("...the record's save point now"), Read.GetLastGrave(WakeValley) == FName(TEXT("Depot")));

	// None of its finds standing: a death at the level's start (the crows still carry Ellis), Continue where they stood.
	ULooterSessionSave* Lost = NewObject<ULooterSessionSave>();
	Lost->Version = ULooterSessionSave::CurrentVersion;
	WakeRecordFound(Lost->Campaign, TEXT("FamilyPlot"));
	WakeRecordFound(Lost->Campaign, TEXT("SinkRim"));
	TestNull(TEXT("Only gone roosts found: no save point here"), ARespawnMarker::FindLastFound(World, Lost->Campaign, WakeValley));
	const FRespawnWakeSpot Spot = ARespawnMarker::ChooseWakeSpot(World, &Lost->Campaign);
	TestTrue(TEXT("...a death wakes at the level's start, carried by the crows"), Spot.Start == LevelStart && !Spot.Grave && Spot.bByCrows);
	Lost->Map = USessionSubsystem::MapOf(World);
	Lost->bHasPlayerSpot = true;
	Lost->PlayerLocation = FVector(7000.0, -3000.0, 100.0);
	Lost->PlayerView = FRotator(5.0, -60.0, 0.0);
	Pawn->SetActorLocation(FVector(111.0, 222.0, 100.0));
	USessionSubsystem::RestoreWorld(World, *Lost);
	TestTrue(TEXT("...Continue: where they were saved"), Pawn->GetActorLocation().Equals(Lost->PlayerLocation, 1.0));
	TestTrue(TEXT("...with the view they had"), Controller->GetControlRotation().Equals(Lost->PlayerView, 0.5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostPlacedTwoAMapTest, "Looter.Roost.Placed.TwoAMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostPlacedTwoAMapTest::RunTest(const FString& Parameters)
{
	// The built maps (Tools/Unreal/build_area_roosts.py from each layout's gameplay.roosts): two of Hob's roosts a map at
	// most; Ransom's Rest's are the Depot (by the train station) and Calder's Post (up the mountain). Each map has a start of
	// its own (a player start that is no trip's landing), where a death wakes before any roost is found.
	struct FWakeMap
	{
		const TCHAR* Package;
		const TCHAR* Object;
		TArray<FName> Expected;
	};
	const FWakeMap Maps[] = {
		{ TEXT("/Game/Maps/Lvl_RansomsRest"), TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"), { FName(TEXT("Depot")), FName(TEXT("CaldersPost")) } },
		{ TEXT("/Game/Maps/Lvl_TutorialIsland"), TEXT("/Game/Maps/Lvl_TutorialIsland.Lvl_TutorialIsland"), {} },
	};
	for (const FWakeMap& Map : Maps)
	{
		if (!FPackageName::DoesPackageExist(Map.Package))
		{
			AddInfo(FString::Printf(TEXT("%s isn't built: skipped."), Map.Package));
			continue;
		}
		const UWorld* Loaded = LoadObject<UWorld>(nullptr, Map.Object);
		const ULevel* Placed = Loaded ? Loaded->PersistentLevel.Get() : nullptr;
		if (!TestNotNull(FString::Printf(TEXT("%s loads"), Map.Package), Placed))
		{
			continue;
		}
		TArray<FName> Roosts;
		int32 Starts = 0;
		for (const AActor* Actor : Placed->Actors)
		{
			if (const ARespawnMarker* Roost = Cast<ARespawnMarker>(Actor); Roost && !AreaLandings::IsLanding(Roost))
			{
				Roosts.Add(Roost->GetMarkerId());
			}
			else if (const APlayerStart* Start = Cast<APlayerStart>(Actor); Start && !AreaLandings::IsLanding(Start))
			{
				++Starts;
			}
		}
		const FString Names = FString::JoinBy(Roosts, TEXT(", "), [](const FName& Id) { return Id.ToString(); });
		TestTrue(FString::Printf(TEXT("%s: two roosts at most (%s; run build_area_roosts.py after the layout changes)"), Map.Package, *Names),
			Roosts.Num() <= 2);
		for (const FName& Wanted : Map.Expected)
		{
			TestTrue(FString::Printf(TEXT("%s: the %s roost stands (%s)"), Map.Package, *Wanted.ToString(), *Names), Roosts.Contains(Wanted));
		}
		TestTrue(FString::Printf(TEXT("%s: a start of its own, where a death wakes before any roost is found"), Map.Package), Starts > 0);
	}
	return true;
}

#endif
