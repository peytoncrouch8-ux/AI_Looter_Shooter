#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/MissionTestWorld.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRespawnNearestGraveTest, "Looter.Respawn.NearestGrave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRespawnNearestGraveTest::RunTest(const FString& Parameters)
{
	// Where a death wakes the player: the open grave nearest where they fell, standing on it and facing its way; with none
	// open, the level's own start. Never a trip's landing: not a grave tagged as one, nor a landing's player start. A grave
	// opens from the start, with its mission's finish, or by the record; one whose mission finishes in play is recorded.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();

	// The family plot opens with Main 1 and the chapel yard with Main 4; boot hill is open from the start; a grave on the
	// depot's platform is open but tagged a trip's landing. The level's own start is far off; a jetty's start is nearer.
	ARespawnMarker* FamilyPlot = PlaceGrave(World, TEXT("FamilyPlot"), FVector(2000.0, 0.0, 0.0), 90.0, false, TEXT("TestMain1"));
	ARespawnMarker* ChapelYard = PlaceGrave(World, TEXT("ChapelYard"), FVector(500.0, 0.0, 0.0), 0.0, false, TEXT("TestMain4"));
	ARespawnMarker* BootHill = PlaceGrave(World, TEXT("BootHill"), FVector(-3000.0, 0.0, 0.0), 0.0, true);
	ARespawnMarker* Platform = PlaceGrave(World, TEXT("Platform"), FVector(300.0, 0.0, 0.0), 0.0, true);
	APlayerStart* LevelStart = PlaceStart(World, FVector(0.0, 5000.0, 100.0), 45.0);
	APlayerStart* Jetty = PlaceStart(World, FVector(100.0, 0.0, 100.0), 0.0, TEXT("Landing_Jetty"));
	if (!TestTrue(TEXT("Graves and starts placed"), FamilyPlot && ChapelYard && BootHill && Platform && LevelStart && Jetty))
	{
		return false;
	}
	Platform->Tags.Add(FName(TEXT("Landing_Depot")));
	auto WakeAt = [World](const FVector& FellAt, const FCampaignRecord* Record)
	{
		return ARespawnMarker::ChooseWakeSpot(World, FellAt, Record);
	};
	const FVector Fell = FVector::ZeroVector;

	FRespawnWakeSpot Spot = WakeAt(Fell, &Campaign);
	TestTrue(TEXT("Only boot hill is open (the platform is a landing): there"), Spot.Grave == BootHill && !Spot.Start);
	TestTrue(TEXT("Standing on it"), Spot.Location.Equals(BootHill->GetActorLocation()));

	Campaign.Complete(TEXT("TestMain1"));
	Spot = WakeAt(Fell, &Campaign);
	TestTrue(TEXT("Main 1 finished: the family plot, nearer"), Spot.Grave == FamilyPlot);
	TestTrue(TEXT("Facing its way, level"), FMath::IsNearlyEqual(Spot.Facing.Yaw, 90.0, 0.01) && Spot.Facing.Pitch == 0.0 && Spot.Facing.Roll == 0.0);

	TestTrue(TEXT("Recorded open (the console): the chapel yard, nearer still"), Campaign.ActivateRespawn(TEXT("ChapelYard"))
		&& WakeAt(Fell, &Campaign).Grave == ChapelYard);
	TestTrue(TEXT("Fallen far to the west: boot hill"), WakeAt(FVector(-2600.0, 0.0, 0.0), &Campaign).Grave == BootHill);
	TestTrue(TEXT("Fallen on the platform: never there, the nearest other"), WakeAt(FVector(300.0, 0.0, 0.0), &Campaign).Grave == ChapelYard);
	// Measured through the air: a grave up on a bluff overhead is nearer on the map but farther than one along the flat.
	ARespawnMarker* Bluff = PlaceGrave(World, TEXT("Bluff"), FVector(-300.0, 0.0, 2000.0), 0.0, true);
	TestTrue(TEXT("The bluff's grave overhead is farther than the chapel yard"), Bluff && WakeAt(Fell, &Campaign).Grave == ChapelYard);
	if (Bluff)
	{
		Bluff->Destroy();
	}

	// Nothing open: the level's own start (where its middle goes, facing its way), not the jetty's nearer one.
	FCampaignRecord NewGame;
	BootHill->bStartActive = false;
	Spot = WakeAt(Fell, &NewGame);
	TestTrue(TEXT("No grave open: the level's start"), !Spot.Grave && Spot.Start == LevelStart);
	TestTrue(TEXT("...there, facing its way"), Spot.Location.Equals(LevelStart->GetActorLocation()) && FMath::IsNearlyEqual(Spot.Facing.Yaw, 45.0, 0.01));
	TestTrue(TEXT("No story at all: the level's start too"), WakeAt(Fell, nullptr).Start == LevelStart);

	// In play, a grave opens as its mission finishes and the record keeps it; one whose mission was finished before the
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
	TestTrue(TEXT("...and is where a death nearby wakes"), WakeAt(Fell, &Played).Grave == ChapelYard);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRespawnSavedActivationTest, "Looter.Respawn.SavedActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRespawnSavedActivationTest::RunTest(const FString& Parameters)
{
	// Graves opened stay open: through the session's save format and back, through a trip's save to its destination, and
	// in a level played after loading, where a grave the record opened is where a death wakes.
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->Map = TEXT("/Game/Maps/Lvl_TutorialIsland");
	FCampaignRecord& Story = Save->Campaign;
	TestTrue(TEXT("A grave opens once"), Story.ActivateRespawn(TEXT("FamilyPlot")) && !Story.ActivateRespawn(TEXT("FamilyPlot")));
	TestFalse(TEXT("No name: nothing opens"), Story.ActivateRespawn(NAME_None));
	Story.ActivateRespawn(TEXT("ChapelYard"));
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

	// Played after loading: the chapel yard (opened only by the record) is where a death wakes, past a nearer closed grave.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ARespawnMarker* ChapelYard = PlaceGrave(World, TEXT("ChapelYard"), FVector(4000.0, 0.0, 0.0), 0.0, false, TEXT("TestMain4"));
	ARespawnMarker* BootHill = PlaceGrave(World, TEXT("BootHill"), FVector(1000.0, 0.0, 0.0), 0.0, false, TEXT("TestMain9"));
	if (!TestTrue(TEXT("Graves placed"), ChapelYard && BootHill))
	{
		return false;
	}
	TestTrue(TEXT("The loaded record's grave is where a death wakes"),
		ARespawnMarker::ChooseWakeSpot(World, FVector::ZeroVector, &Arrived->Campaign).Grave == ChapelYard);
	return true;
}

#endif
