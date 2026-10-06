#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/SkiffJetty.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	const FName BoardSkiffId(TEXT("BoardSkiff"));
	const FName SkyreachId(TEXT("Skyreach"));

	/**
	 * "Board the skiff" as the asset holds it, made in code: one step, boarding the skiff. Each in a scratch package of its
	 * own, so two never share a name (a second would be made over the first).
	 */
	UMissionDefinition* NewBoardingMission()
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(CreatePackage(nullptr), TEXT("BoardSkiff"), EMissionKind::Tutorial,
			EMissionStart::Manual, SkyreachId);
		MissionTestWorld::AddObjective<UMissionBoardObjective>(Mission, 0);
		return Mission;
	}

	/** Moves the jetty on by Seconds in short steps, as frames would. */
	void Run(ASkiffJetty& Jetty, float Seconds)
	{
		for (float Done = 0.f; Done < Seconds; Done += 0.1f)
		{
			Jetty.Advance(0.1f);
		}
	}

	/** The plank stands up from its hinge: the middle of it well over the hinge. */
	bool IsPlankRaised(const UStaticMeshComponent& Plank)
	{
		return Plank.Bounds.Origin.Z > Plank.GetComponentLocation().Z + 100.0;
	}

	/** The plank lies level: the middle of it about as high as the hinge. */
	bool IsPlankLevel(const UStaticMeshComponent& Plank)
	{
		return FMath::Abs(Plank.Bounds.Origin.Z - Plank.GetComponentLocation().Z) < 15.0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationGangplankTest, "Looter.Station.Gangplank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStationGangplankTest::RunTest(const FString& Parameters)
{
	// The rule: up until the tutorial is done (or skipped), and always down once the first cast-off is recorded.
	TestFalse(TEXT("Tutorial not done, not cast off: up"), ASkiffJetty::IsGangplankDownFor(false, false));
	TestTrue(TEXT("Tutorial done: down"), ASkiffJetty::IsGangplankDownFor(true, false));
	TestTrue(TEXT("Cast off (the skip): down, whatever the tutorial"), ASkiffJetty::IsGangplankDownFor(false, true));
	TestTrue(TEXT("Both: down"), ASkiffJetty::IsGangplankDownFor(true, true));

	// The jetty in a level, reading the story from the level's mission runner (a test record).
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	if (!TestNotNull(TEXT("The level's mission runner"), Runner) || !TestTrue(TEXT("The player's stand-in"), Player && Interaction))
	{
		return false;
	}
	Runner->BeginForTesting({ NewBoardingMission() }, Campaign, Player, SkyreachId);

	ASkiffJetty* Jetty = World->SpawnActor<ASkiffJetty>(FVector(0.0, 3000.0, 0.0), FRotator(0.0, 30.0, 0.0));
	if (!TestNotNull(TEXT("Jetty placed"), Jetty))
	{
		return false;
	}
	Jetty->DispatchBeginPlay();
	const AStaticMeshActor* Skiff = Jetty->GetSkiff();
	const UStaticMeshComponent* Plank = Jetty->GetGangplank();
	if (!TestNotNull(TEXT("The skiff, moored"), Skiff) || !TestNotNull(TEXT("Its gangplank"), Plank))
	{
		return false;
	}
	TestTrue(TEXT("The skiff's hull is its root, with the deck the ride stands on"), Skiff->GetStaticMeshComponent()->DoesSocketExist(TEXT("Deck")));
	TestTrue(TEXT("Fixed to the jetty while moored (the crosshair on it finds the jetty)"), Skiff->IsAttachedTo(Jetty));

	// As the art was made: the hinge level with the deck's edge, 2.75 m out, so the 3 m plank rests 25 cm onto the deck;
	// the keel 45 cm under the deck's top.
	const UStaticMeshComponent* Deck = Jetty->Jetty;
	if (TestTrue(TEXT("The jetty has the gangplank's landing socket"), Deck->DoesSocketExist(Jetty->GangplankLandSocket)))
	{
		const FVector Land = Deck->GetSocketLocation(Jetty->GangplankLandSocket);
		const FVector Hinge = Plank->GetComponentLocation();
		TestTrue(FString::Printf(TEXT("The hinge 2.75 m from the deck's edge (it's %.1f cm)"), FVector::Dist2D(Hinge, Land)),
			FMath::IsNearlyEqual(FVector::Dist2D(Hinge, Land), 275.0, 4.0));
		TestTrue(FString::Printf(TEXT("The hinge at the deck's height (%.1f cm off)"), Hinge.Z - Land.Z), FMath::IsNearlyEqual(Hinge.Z, Land.Z, 4.0));
		TestTrue(FString::Printf(TEXT("The keel 45 cm under the deck's top (%.1f)"), Land.Z - Skiff->GetActorLocation().Z),
			FMath::IsNearlyEqual(Land.Z - Skiff->GetActorLocation().Z, 45.0, 4.0));
		TestTrue(TEXT("The skiff turned as the jetty is"), Skiff->GetActorRotation().Equals(Jetty->GetActorRotation(), 0.5));
	}

	// Before the tutorial is done: the plank up, nothing to use, no boarding mission.
	TestFalse(TEXT("Up as the tutorial starts"), Jetty->IsGangplankDown());
	TestTrue(TEXT("Raised: its free end up"), FMath::IsNearlyEqual(Jetty->GetGangplankRaise(), 1.f) && IsPlankRaised(*Plank));
	TestFalse(TEXT("Nothing to use"), Jetty->GetInteractionOptions(*Interaction).bUsable);
	Runner->Update(0.f);
	TestFalse(TEXT("No boarding mission yet"), Runner->IsRunning(BoardSkiffId));

	// The tutorial finishes: the plank comes down, the bell rings, and "Board the skiff" shows.
	Campaign.Complete(TEXT("Tutorial"));
	Jetty->Advance(0.3f);
	TestTrue(TEXT("Coming down"), Jetty->IsGangplankDown() && Jetty->GetGangplankRaise() < 1.f);
	TestTrue(TEXT("The bell rings"), Jetty->IsBellRinging());
	TestTrue(TEXT("Board the skiff shows"), Runner->IsRunning(BoardSkiffId) && Runner->GetTrackedMission() == BoardSkiffId);
	TestFalse(TEXT("Still swinging: nothing to use"), Jetty->GetInteractionOptions(*Interaction).bUsable);
	Run(*Jetty, Jetty->GangplankSeconds + 0.2f);
	TestTrue(TEXT("Down on the deck"), FMath::IsNearlyZero(Jetty->GetGangplankRaise()) && IsPlankLevel(*Plank));
	const FInteractionOptions Options = Jetty->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("Down: held a second to open the board"), Options.bUsable && Options.bHold && !Options.bTap
		&& FMath::IsNearlyEqual(Options.HoldSeconds, Jetty->HoldSeconds) && !Options.HoldPrompt.IsEmpty());
	Run(*Jetty, Jetty->BellRingSeconds);
	TestFalse(TEXT("The bell settles"), Jetty->IsBellRinging());

	// Casting off sends Board.Skiff: the mission is done, and not offered again.
	Runner->NotifyEvent(FMissionEvent::Named(FMissionEvent::BoardEvent(Jetty->Vehicle), Jetty));
	TestTrue(TEXT("Boarding finishes it"), !Runner->IsRunning(BoardSkiffId) && Campaign.HasCompleted(BoardSkiffId));
	TestFalse(TEXT("Finished, it isn't offered again"), ASkiffJetty::OfferBoarding(*Runner, BoardSkiffId, true));

	// The dev command: up whatever the story says, and the story no longer moves it.
	Jetty->ForceGangplank(false);
	Run(*Jetty, Jetty->GangplankSeconds + 1.f);
	TestTrue(TEXT("Forced up, and it stays up"), !Jetty->IsGangplankDown() && IsPlankRaised(*Plank));

	// A practice visit after the first cast-off (the tutorial skipped from the main menu): down at once, no bell, no mission.
	FCampaignRecord Visit;
	Visit.bFirstCastOff = true;
	FTestWorldWrapper PracticeLevel;
	if (!TestTrue(TEXT("Second test level made"), PracticeLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* PracticeWorld = PracticeLevel.GetTestWorld();
	UMissionRunner* PracticeRunner = PracticeWorld->GetSubsystem<UMissionRunner>();
	AActor* Visitor = MissionTestWorld::SpawnMarker(PracticeWorld, FVector::ZeroVector);
	if (!TestTrue(TEXT("Its runner and the visitor"), PracticeRunner && Visitor))
	{
		return false;
	}
	PracticeRunner->BeginForTesting({ NewBoardingMission() }, Visit, Visitor, SkyreachId);
	ASkiffJetty* Again = PracticeWorld->SpawnActor<ASkiffJetty>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Jetty placed again"), Again))
	{
		return false;
	}
	Again->DispatchBeginPlay();
	TestTrue(TEXT("Down from the start"), Again->IsGangplankDown() && FMath::IsNearlyZero(Again->GetGangplankRaise()));
	TestFalse(TEXT("Without a bell"), Again->IsBellRinging());
	PracticeRunner->Update(0.f);
	TestFalse(TEXT("And no boarding mission"), PracticeRunner->IsRunning(BoardSkiffId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationBoardSkiffTest, "Looter.Station.BoardSkiffMission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStationBoardSkiffTest::RunTest(const FString& Parameters)
{
	// "Board the skiff" shows whenever the tutorial is done and the player hasn't cast off for the first time: after the
	// tutorial, after skipping it on Skyreach, and for sessions that finished it before the skiff came. Not before, and
	// never after the first cast-off (the main menu's skip counts as one).
	TestFalse(TEXT("The tutorial under way: no"), ASkiffJetty::ShowsBoardingFor(false, false));
	TestTrue(TEXT("The tutorial done (or skipped on Skyreach), not cast off: yes"), ASkiffJetty::ShowsBoardingFor(true, false));
	TestFalse(TEXT("Cast off: no"), ASkiffJetty::ShowsBoardingFor(true, true));
	TestFalse(TEXT("Cast off by the main menu's skip: no"), ASkiffJetty::ShowsBoardingFor(false, true));

	// An older session that finished the tutorial before the skiff came: it shows, on Skyreach only.
	FCampaignRecord Older;
	Older.Complete(TEXT("Tutorial"));
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	if (!TestTrue(TEXT("The runner and the player's stand-in"), Runner && Player))
	{
		return false;
	}
	Runner->BeginForTesting({ NewBoardingMission() }, Older, Player, TEXT("RansomsRest"));
	TestFalse(TEXT("Elsewhere than Skyreach it doesn't start"), ASkiffJetty::OfferBoarding(*Runner, BoardSkiffId, true));

	FTestWorldWrapper SkyLevel;
	if (!TestTrue(TEXT("Skyreach's test level made"), SkyLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UMissionRunner* SkyRunner = SkyLevel.GetTestWorld()->GetSubsystem<UMissionRunner>();
	AActor* SkyPlayer = MissionTestWorld::SpawnMarker(SkyLevel.GetTestWorld(), FVector::ZeroVector);
	if (!TestTrue(TEXT("Skyreach's runner"), SkyRunner && SkyPlayer))
	{
		return false;
	}
	SkyRunner->BeginForTesting({ NewBoardingMission() }, Older, SkyPlayer, SkyreachId);
	TestFalse(TEXT("Not before the tutorial's done"), ASkiffJetty::OfferBoarding(*SkyRunner, BoardSkiffId, false));
	TestTrue(TEXT("On Skyreach, the tutorial done: it shows"), ASkiffJetty::OfferBoarding(*SkyRunner, BoardSkiffId, true)
		&& SkyRunner->IsRunning(BoardSkiffId));
	TestTrue(TEXT("Asked again: still the one running"), ASkiffJetty::OfferBoarding(*SkyRunner, BoardSkiffId, true));
	SkyRunner->NotifyEvent(FMissionEvent::Named(FMissionEvent::BoardEvent(TEXT("Skiff"))));
	TestTrue(TEXT("Board.Skiff finishes it"), !SkyRunner->IsRunning(BoardSkiffId) && Older.HasCompleted(BoardSkiffId));

	// The project's asset (Tools/Unreal/create_mission_assets.py), as the jetty offers it.
	const UMissionDefinition* Asset = nullptr;
	for (const UMissionDefinition* Mission : UMissionDefinition::LoadAll())
	{
		Asset = Mission && Mission->GetMissionId() == BoardSkiffId ? Mission : Asset;
	}
	if (!Asset)
	{
		AddError(TEXT("DA_Mission_BoardSkiff belongs in /Game/Data/Missions: run Tools/Unreal/create_mission_assets.py in the editor."));
		return false;
	}
	TestTrue(TEXT("Skyreach's own (not a story mission), started from code, on Skyreach"), Asset->Kind == EMissionKind::Tutorial
		&& Asset->Start == EMissionStart::Manual && Asset->Area == SkyreachId);
	TestTrue(TEXT("Nothing to finish first: the jetty's rule decides"), Asset->Prerequisites.IsEmpty());
	TestTrue(TEXT("No reward: Skyreach gives no experience"), Asset->Rewards.IsEmpty());
	const UMissionBoardObjective* Board = Cast<UMissionBoardObjective>(Asset->GetObjective(0, 0));
	if (TestTrue(TEXT("One step: board the skiff"), Asset->Steps.Num() == 1 && Board != nullptr))
	{
		TestTrue(TEXT("The skiff"), Board->Vehicle == FName(TEXT("Skiff")));
		TestEqual(TEXT("Its words"), Board->Text.ToString(),
			FString(TEXT("Your skiff is in. Board it at the jetty past the lookout when you're ready to leave Skyreach.")));
		TestTrue(TEXT("Its arrow on the jetty"), Board->Waypoint == EMissionWaypoint::Actor
			&& Board->WaypointActor.ActorClass.Get() == ASkiffJetty::StaticClass());
	}
	return true;
}

#endif
