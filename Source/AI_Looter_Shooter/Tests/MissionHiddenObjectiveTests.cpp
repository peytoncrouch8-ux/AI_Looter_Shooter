#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/EncounterSpawner.h"
#include "Creatures/SpiderCreature.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

using namespace MissionTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionHiddenUntilStartedTest, "Looter.Missions.HiddenUntilStarted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionHiddenUntilStartedTest::RunTest(const FString& Parameters)
{
	// Main 6's deck in small (the user's call, 2026-11-04: "Hide until it starts"): a step of a fight shown from the start, a
	// later push hidden until its spawner first spawns, and a push whose spawner isn't in the level (it passes unseen), then
	// a step after it. The later push is off the tracker and the Missions page until its spawner spawns, on after; the step
	// ends only once both fights are cleared.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	// Far from both spawners, so neither wakes for the player: only their triggers start them.
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(60000.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A mission runner"), Runner) || !TestNotNull(TEXT("...the missions' display"), Display)
		|| !TestNotNull(TEXT("...the player's stand-in"), Player))
	{
		return false;
	}
	const auto MakeSpawner = [World](const TCHAR* Id, const FVector& Where)
	{
		return EncounterTestWorld::SpawnSpawner(World, Where, [Id](AEncounterSpawner& Spawner)
		{
			Spawner.SpawnerId = FName(Id);
			Spawner.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1) };
		});
	};
	AEncounterSpawner* Deck = MakeSpawner(TEXT("TestDeck"), FVector::ZeroVector);
	AEncounterSpawner* Roof = MakeSpawner(TEXT("TestRoof"), FVector(3000.0, 0.0, 0.0));
	if (!TestTrue(TEXT("Two spawners"), Deck && Roof))
	{
		return false;
	}

	const FName MissionId(TEXT("TestBoarding"));
	const FName Area(TEXT("TestBarge"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Mission = NewMission(Scratch, TEXT("TestBoarding"), EMissionKind::Main, EMissionStart::Automatic, Area);
	const auto AddClear = [Mission](const TCHAR* Words, const TCHAR* Spawner, bool bLater)
	{
		UMissionClearObjective* Clear = AddObjective<UMissionClearObjective>(Mission, 0);
		Clear->Text = FText::FromString(Words);
		Clear->SpawnerId = FName(Spawner);
		// The later pushes as create_mission_assets.py makes them: hidden until they start, passing without their spawner.
		Clear->bHiddenUntilStarted = bLater;
		Clear->bPassWithoutTargets = bLater;
	};
	const FString DeckWords(TEXT("Fight across the deck"));
	const FString RoofWords(TEXT("Put down the roof droppers"));
	AddClear(*DeckWords, TEXT("TestDeck"), false);
	AddClear(*RoofWords, TEXT("TestRoof"), true);
	AddClear(TEXT("Put down the stern push"), TEXT("TestNoSuchSpawner"), true);
	UMissionReachObjective* After = AddObjective<UMissionReachObjective>(Mission, 1);
	After->Place.Location = FVector(-90000.0, 0.0, 0.0);
	After->Place.Radius = 100.f;

	Runner->BeginForTesting({ Mission }, Campaign, Player, Area);
	Runner->Update(0.f);
	const auto Line = [Display]() -> FString
	{
		const FMission* Shown = Display->GetTracked();
		return Shown ? Shown->Tracker.Line : FString();
	};
	TArray<FMissionObjectiveView> Views = Runner->GetObjectiveViews(MissionId);
	if (!TestTrue(TEXT("It runs, its step's three objectives listed"), Runner->IsRunning(MissionId) && Runner->GetStep(MissionId) == 0 && Views.Num() == 3))
	{
		return false;
	}
	TestTrue(TEXT("The deck's fight shown from the start; the roof's push hidden; the push with no spawner passed, hidden"),
		!Views[0].bHidden && Views[1].bHidden && !Views[1].bDone && Views[2].bHidden && Views[2].bDone);
	TestEqual(TEXT("The tracker: the deck's fight"), Line(), DeckWords);

	// The deck's fight begins and is cleared: the roof's push, not begun, stays off the tracker and holds the step.
	TestTrue(TEXT("The deck's fight begins"), Deck->TriggerWave(true) && Deck->NumAlive() == 1);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("...the roof's push still hidden"), Runner->GetObjectiveViews(MissionId)[1].bHidden);
	EncounterTestWorld::KillAll(*Deck);
	Deck->UpdateEncounter(Deck->CheckSeconds);
	Runner->Update(UMissionRunner::UpdateInterval);
	Views = Runner->GetObjectiveViews(MissionId);
	TestTrue(TEXT("The deck's fight cleared"), Deck->GetState() == EEncounterState::Cleared && Views[0].bDone);
	TestTrue(TEXT("...the roof's push not begun: hidden, off the tracker"), Views[1].bHidden && Line() != RoofWords);
	TestEqual(TEXT("...and the step held for it"), Runner->GetStep(MissionId), 0);

	// Its spawner spawns: on the tracker and the Missions page, counted as any other.
	TestTrue(TEXT("The roof's push begins"), Roof->TriggerWave(true) && Roof->NumAlive() == 1);
	Runner->Update(UMissionRunner::UpdateInterval);
	Views = Runner->GetObjectiveViews(MissionId);
	TestTrue(TEXT("...shown now, not done"), !Views[1].bHidden && !Views[1].bDone);
	TestEqual(TEXT("...the tracker's line"), Line(), RoofWords);
	TestEqual(TEXT("...still the step"), Runner->GetStep(MissionId), 0);

	// Cleared: every fight of the step done, the next step.
	EncounterTestWorld::KillAll(*Roof);
	Roof->UpdateEncounter(Roof->CheckSeconds);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("The roof's push cleared: the next step"), Roof->GetState() == EEncounterState::Cleared && Runner->GetStep(MissionId) == 1);
	return true;
}

#endif
