#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/MissionTestWorld.h"
#include "World/GraveMarkRules.h"
#include "World/RespawnMarker.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	/** A grave in a test level, its play not begun (choosing where a death wakes needs none). */
	ARespawnMarker* PlaceGrave(UWorld* World, FName Id, const FVector& Where, double Yaw, bool bOpenFromStart, FName OpenedBy = NAME_None)
	{
		ARespawnMarker* Grave = World->SpawnActor<ARespawnMarker>(Where, FRotator(0.0, Yaw, 0.0));
		if (Grave)
		{
			Grave->MarkerId = Id;
			Grave->bStartActive = bOpenFromStart;
			Grave->ActiveAfterMission = OpenedBy;
		}
		return Grave;
	}

	/** A player start: the level's own, or a trip's landing with a landing's tag. */
	APlayerStart* PlaceStart(UWorld* World, const FVector& Where, double Yaw, FName StartTag = NAME_None)
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

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRespawnOpeningTest, "Looter.Respawn.Opening",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRespawnOpeningTest::RunTest(const FString& Parameters)
{
	// What opens a roost (fast travel goes to open ones): the start, its mission's finish, or the record; one whose mission
	// finishes in play is recorded then, one finished before the level began as play begins. Opening is not finding: a death
	// wakes only at the roost last found (the user's rule, 2026-10-14; RoostWakeTests.cpp), never at one only opened, and
	// with none found at the level's own start, not a jetty's nearer landing.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();

	// The family plot opens with Main 1 and the chapel yard with Main 4; boot hill is open from the start; a roost on the
	// depot's platform is open but tagged a trip's landing. The level's own start is far off; a jetty's start is nearer.
	ARespawnMarker* FamilyPlot = PlaceGrave(World, TEXT("FamilyPlot"), FVector(2000.0, 0.0, 0.0), 90.0, false, TEXT("TestMain1"));
	ARespawnMarker* ChapelYard = PlaceGrave(World, TEXT("ChapelYard"), FVector(500.0, 0.0, 0.0), 0.0, false, TEXT("TestMain4"));
	ARespawnMarker* BootHill = PlaceGrave(World, TEXT("BootHill"), FVector(-3000.0, 0.0, 0.0), 0.0, true);
	ARespawnMarker* Platform = PlaceGrave(World, TEXT("Platform"), FVector(300.0, 0.0, 0.0), 0.0, true);
	APlayerStart* LevelStart = PlaceStart(World, FVector(0.0, 5000.0, 100.0), 45.0);
	APlayerStart* Jetty = PlaceStart(World, FVector(100.0, 0.0, 100.0), 0.0, TEXT("Landing_Jetty"));
	if (!TestTrue(TEXT("Roosts and starts placed"), FamilyPlot && ChapelYard && BootHill && Platform && LevelStart && Jetty))
	{
		return false;
	}
	Platform->Tags.Add(FName(TEXT("Landing_Depot")));

	TestTrue(TEXT("Boot hill: open from the start"), BootHill->IsActive(Campaign) && BootHill->DescribeState(Campaign) == TEXT("open from the start"));
	TestFalse(TEXT("The family plot waits for Main 1"), FamilyPlot->IsActive(Campaign));
	Campaign.Complete(TEXT("TestMain1"));
	TestTrue(TEXT("Main 1 finished: the family plot is open"), FamilyPlot->IsActive(Campaign));
	TestTrue(TEXT("Recorded open (the console): the chapel yard"), Campaign.ActivateRespawn(TEXT("ChapelYard")) && ChapelYard->IsActive(Campaign));
	FRespawnWakeSpot Spot = ARespawnMarker::ChooseWakeSpot(World, &Campaign);
	TestTrue(TEXT("Three open, none found: a death wakes at the level's start"), !Spot.Grave && Spot.Start == LevelStart);
	TestTrue(TEXT("...not the jetty's nearer landing, facing the start's way"), Spot.Location.Equals(LevelStart->GetActorLocation())
		&& FMath::IsNearlyEqual(Spot.Facing.Yaw, 45.0, 0.01));
	TestTrue(TEXT("No story at all: the level's start too"), ARespawnMarker::ChooseWakeSpot(World, nullptr).Start == LevelStart);

	// In play, a roost opens as its mission finishes and the record keeps it; one whose mission was finished before the
	// level began is recorded as play begins.
	FCampaignRecord Played;
	Played.Complete(TEXT("TestMain1"));
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	if (!TestNotNull(TEXT("The level has a mission runner"), Runner))
	{
		return false;
	}
	const FName Valley(TEXT("TestValley"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Main4 = MissionTestWorld::NewMission(Scratch, TEXT("TestMain4"), EMissionKind::Main, EMissionStart::Automatic, Valley);
	MissionTestWorld::AddObjective<UMissionEventObjective>(Main4, 0)->Event = TEXT("Bell.Rung");
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	ARespawnMarker* KeepersGrave = PlaceGrave(World, TEXT("KeepersGrave"), FVector(-1000.0, 2000.0, 0.0), 0.0, false, TEXT("TestMain1"));
	if (!TestTrue(TEXT("Stand-in and the keeper's grave placed"), Player && KeepersGrave))
	{
		return false;
	}
	Runner->BeginForTesting({ Main4 }, Played, Player, Valley);
	ChapelYard->DispatchBeginPlay();
	KeepersGrave->DispatchBeginPlay();
	TestTrue(TEXT("Its mission finished before the level began: recorded open as play begins"), Played.IsRespawnActive(TEXT("KeepersGrave")));
	TestFalse(TEXT("The chapel yard waits for Main 4"), Played.IsRespawnActive(TEXT("ChapelYard")) || ChapelYard->IsActive(Played));
	Runner->Update(0.f);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Bell.Rung")));
	TestTrue(TEXT("Main 4 finished: the chapel yard is recorded open"), Played.HasCompleted(TEXT("TestMain4")) && Played.IsRespawnActive(TEXT("ChapelYard")));
	TestFalse(TEXT("...open, not found: no death wakes there"), ARespawnMarker::ChooseWakeSpot(World, &Played).Grave == ChapelYard);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRespawnSavedActivationTest, "Looter.Respawn.SavedActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRespawnSavedActivationTest::RunTest(const FString& Parameters)
{
	// Roosts opened and found stay so: through the session's save format and back, through a trip's save to its
	// destination, and in a level played after loading, where the roost the record found is where a death wakes.
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->Map = TEXT("/Game/Maps/Lvl_TutorialIsland");
	FCampaignRecord& Story = Save->Campaign;
	TestTrue(TEXT("A grave opens once"), Story.ActivateRespawn(TEXT("FamilyPlot")) && !Story.ActivateRespawn(TEXT("FamilyPlot")));
	TestFalse(TEXT("No name: nothing opens"), Story.ActivateRespawn(NAME_None));
	Story.ActivateRespawn(TEXT("ChapelYard"));
	// The chapel yard found too (walked up to): its area's save point. A test level is no area's, so it's kept under none.
	GraveMarkRules::Record(Story, NAME_None, TEXT("ChapelYard"));
	TestTrue(TEXT("A new game has none open"), GetDefault<ULooterSessionSave>()->Campaign.ActiveRespawns.IsEmpty());

	ULooterSessionSave* Loaded = WriteAndRead(Save);
	if (!TestNotNull(TEXT("Saved and read"), Loaded))
	{
		return false;
	}
	const FCampaignRecord& Read = Loaded->Campaign;
	TestTrue(TEXT("Both graves still open, in the order they opened"), Read.ActiveRespawns.Num() == 2
		&& Read.ActiveRespawns[0] == FName(TEXT("FamilyPlot")) && Read.ActiveRespawns[1] == FName(TEXT("ChapelYard")));
	TestFalse(TEXT("One never opened stays closed"), Read.IsRespawnActive(TEXT("BootHill")));

	// A trip: the session is saved pointing at the destination, which reads it as it begins.
	Loaded->PrepareTrip(TEXT("/Game/Maps/Lvl_Skyreach"), TEXT("Landing_Jetty"));
	const ULooterSessionSave* Arrived = WriteAndRead(Loaded);
	if (!TestNotNull(TEXT("The trip's save reads"), Arrived))
	{
		return false;
	}
	TestTrue(TEXT("Arrived with the graves open"), Arrived->Campaign.IsRespawnActive(TEXT("FamilyPlot")) && Arrived->Campaign.IsRespawnActive(TEXT("ChapelYard")));

	TestTrue(TEXT("...and the chapel yard found, the save point"), Arrived->Campaign.MarkedGraves.Contains(FName(TEXT("ChapelYard")))
		&& Arrived->Campaign.GetLastGrave(NAME_None) == FName(TEXT("ChapelYard")));

	// Played after loading: the chapel yard (opened by the record, found) is where a death wakes, past a nearer closed roost.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ARespawnMarker* ChapelYard = PlaceGrave(World, TEXT("ChapelYard"), FVector(4000.0, 0.0, 0.0), 0.0, false, TEXT("TestMain4"));
	ARespawnMarker* BootHill = PlaceGrave(World, TEXT("BootHill"), FVector(1000.0, 0.0, 0.0), 0.0, false, TEXT("TestMain9"));
	if (!TestTrue(TEXT("Roosts placed"), ChapelYard && BootHill))
	{
		return false;
	}
	TestTrue(TEXT("The loaded record's roost is where a death wakes"),
		ARespawnMarker::ChooseWakeSpot(World, &Arrived->Campaign).Grave == ChapelYard);
	return true;
}

#endif
