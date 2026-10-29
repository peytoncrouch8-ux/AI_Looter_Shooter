#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/BossSeal.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/SetPieceRules.h"
#include "Creatures/SetPieceSpawner.h"
#include "Creatures/SpiderCreature.h"
#include "Tests/EncounterTestWorld.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// FIGHTS lane 6 (2026-10-28): a set piece's arena (fog gates across the chapel yard's opening: SetPieceSpawnerArena.cpp) and an
// encounter group's leader mark (EncounterSpawnerWaves.cpp). The gates' rules without a level, then one played in a test level:
// they rise only with the player inside, drop on the player's death and come again, drop when the fight is won.

namespace
{
	/** A set piece at Where, set up by Setup before its play begins, begun, with a fixed seed. */
	ASetPieceSpawner* SpawnArena(UWorld* World, const FVector& Where, TFunctionRef<void(ASetPieceSpawner&)> Setup)
	{
		ASetPieceSpawner* Made = World->SpawnActor<ASetPieceSpawner>(Where, FRotator::ZeroRotator);
		if (!Made)
		{
			return nullptr;
		}
		Setup(*Made);
		Made->DispatchBeginPlay();
		Made->SetRandomSeed(20261029);
		return Made;
	}

	/** A fog gate across X = Line, 12 m wide, the arena on the +X side (the spawner stands there). */
	FSetPieceGate GateAt(double Line)
	{
		FSetPieceGate Gate;
		Gate.Start = FVector(Line, -600.0, 0.0);
		Gate.End = FVector(Line, 600.0, 0.0);
		return Gate;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSetPieceArenaRulesTest, "Looter.SetPiece.Arena.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSetPieceArenaRulesTest::RunTest(const FString& Parameters)
{
	using namespace SetPieceRules;
	// A wall faces the arena whichever way round its ends are given; the seal's +Y runs along it.
	const FVector Arena(2000.0, 0.0, 0.0);
	const FGateFrame Straight = GateFrame(FVector(1000.0, -600.0, 0.0), FVector(1000.0, 600.0, 0.0), Arena);
	TestTrue(TEXT("A wall across X = 1000 with the arena at +X: its +X (yaw 0) faces it"), FMath::IsNearlyZero(Straight.Yaw, 0.01)
		&& Straight.Ahead.Equals(FVector(1.0, 0.0, 0.0), 0.001));
	TestNearlyEqual(TEXT("...12 m long"), static_cast<float>(Straight.Length), 1200.f, 0.01f);
	TestTrue(TEXT("...starting at the first end"), Straight.Origin.Equals(FVector(1000.0, -600.0, 0.0), 0.01));
	const FGateFrame Reversed = GateFrame(FVector(1000.0, 600.0, 0.0), FVector(1000.0, -600.0, 120.0), Arena);
	TestTrue(TEXT("Its ends given the other way round swap, so it still faces the arena"), Reversed.Ahead.Equals(FVector(1.0, 0.0, 0.0), 0.001)
		&& Reversed.Origin.Equals(FVector(1000.0, -600.0, 120.0), 0.01));
	TestNearlyEqual(TEXT("...and its far end is as high over the origin as it was (here under it)"), static_cast<float>(Reversed.Rise), -120.f, 0.01f);
	const FGateFrame Sideways = GateFrame(FVector(-600.0, 3000.0, 0.0), FVector(600.0, 3000.0, 0.0), FVector(0.0, 1000.0, 0.0));
	TestTrue(TEXT("A wall along X with the arena at lower Y faces -Y (yaw 270 or -90)"), Sideways.Ahead.Equals(FVector(0.0, -1.0, 0.0), 0.001));

	TestTrue(TEXT("2 m inside, margin 1.5 m: ahead"), IsAheadOfGate(Straight, FVector(1200.0, 0.0, 0.0), 150.0));
	TestFalse(TEXT("1 m inside: not yet"), IsAheadOfGate(Straight, FVector(1100.0, 0.0, 0.0), 150.0));
	TestFalse(TEXT("Outside the wall: never"), IsAheadOfGate(Straight, FVector(800.0, 0.0, 0.0), 0.0));

	// They rise only fighting, with a push begun, not won, the arena on, and the living player inside.
	TestTrue(TEXT("Fighting, begun, the player inside: rise"), ShouldRaiseGates(false, true, EEncounterState::Engaged, 1, false, true, true));
	TestFalse(TEXT("The player still outside: no (nobody is shut out)"), ShouldRaiseGates(false, true, EEncounterState::Engaged, 1, false, true, false));
	TestFalse(TEXT("The player dead: no"), ShouldRaiseGates(false, true, EEncounterState::Engaged, 1, false, false, true));
	TestFalse(TEXT("Not begun: no"), ShouldRaiseGates(false, true, EEncounterState::Engaged, 0, false, true, true));
	TestFalse(TEXT("Won: no"), ShouldRaiseGates(false, true, EEncounterState::Cleared, 3, true, true, true));
	TestFalse(TEXT("Taken away while the player is far: no"), ShouldRaiseGates(false, true, EEncounterState::Waiting, 1, false, true, true));
	TestFalse(TEXT("The arena switched off: no"), ShouldRaiseGates(false, false, EEncounterState::Engaged, 1, false, true, true));
	TestFalse(TEXT("Already up: nothing to do"), ShouldRaiseGates(true, true, EEncounterState::Engaged, 1, false, true, true));

	// ...and open when it is won (Cleared, so a Bell-Ringer left tolling traps no one), taken away, off, or the player dies.
	TestFalse(TEXT("Up and fighting with the player alive: stay up"), ShouldDropGates(true, true, EEncounterState::Engaged, false, true));
	TestTrue(TEXT("Its own creatures all down (Cleared): open"), ShouldDropGates(true, true, EEncounterState::Cleared, false, true));
	TestTrue(TEXT("Done: open"), ShouldDropGates(true, true, EEncounterState::Cleared, true, true));
	TestTrue(TEXT("Taken away: open"), ShouldDropGates(true, true, EEncounterState::Waiting, false, true));
	TestTrue(TEXT("Its story ended: open"), ShouldDropGates(true, true, EEncounterState::Off, false, true));
	TestTrue(TEXT("The player died: open"), ShouldDropGates(true, true, EEncounterState::Engaged, false, false));
	TestTrue(TEXT("The arena switched off while up: open"), ShouldDropGates(true, false, EEncounterState::Engaged, false, true));
	TestFalse(TEXT("Never up: nothing to open"), ShouldDropGates(false, true, EEncounterState::Cleared, true, false));

	const FSetPieceTuning Defaults;
	TestTrue(TEXT("The arena is on by default (Looter.Fight.SetPiece.Arena 1)"), Defaults.bArena && FSetPieceTuning::Current().bArena);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSetPieceArenaPlayTest, "Looter.SetPiece.Arena.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSetPieceArenaPlayTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	// The player stands 5 m inside the first arena's wall (X = 1000).
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1500.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	UHealthComponent* Health = Player->FindComponentByClass<UHealthComponent>();
	if (!TestNotNull(TEXT("...with health"), Health))
	{
		return false;
	}

	ASetPieceSpawner* Yard = SpawnArena(World, FVector(2000.0, 0.0, 0.0), [](ASetPieceSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("ArenaTest");
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 3) };
		Setup.SpringRadius = 3000.f;
		Setup.GiveUpRadius = 5000.f;
		Setup.bReward = false;
		Setup.ArenaGates = { GateAt(1000.0) };
	});
	if (!TestNotNull(TEXT("The arena set piece"), Yard))
	{
		return false;
	}
	TestFalse(TEXT("Before the fight the gates stand open"), Yard->AreGatesUp());

	// Sprung with the player inside: the gate rises with the first push and shuts the way they came.
	Yard->UpdateEncounter(0.5f);
	TestEqual(TEXT("Sprung: the three of its first push"), Yard->NumAlive(), 3);
	TestTrue(TEXT("...and the player inside: the gate is up"), Yard->AreGatesUp());
	ABossSeal* Seal = Yard->GetGateSeal(0);
	if (!TestNotNull(TEXT("...a fog wall made for it"), Seal))
	{
		return false;
	}
	TestTrue(TEXT("...raised"), Seal->IsRaised() && Seal->Shape == EBossSealShape::Gate);
	TestTrue(TEXT("...its walls stop pawns"), Seal->GetWalls().Num() > 0 && Seal->GetWalls()[0]
		&& Seal->GetWalls()[0]->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	TestTrue(TEXT("...the player on its arena's side, the way out behind the wall"), Seal->IsInside(Player->GetActorLocation(), 0.f)
		&& !Seal->IsInside(FVector(500.0, 0.0, 0.0), 0.f));
	TestTrue(TEXT("...across 12 m of the opening"), FMath::IsNearlyEqual(FVector::Dist2D(Seal->GetPath()[0], Seal->GetPath()[1]), 1200.0, 1.0));

	// The player's death opens it at once; alive again and inside, with the fight still on, it closes at the next look.
	BossTestWorld::Hurt(Player, 1.0e7f);
	TestTrue(TEXT("The player's death: the gate is open"), Health->IsDead() && !Yard->AreGatesUp() && !Seal->IsRaised());
	Yard->UpdateEncounter(0.5f);
	TestFalse(TEXT("...and stays open while they are dead"), Yard->AreGatesUp());
	Health->ResetHealth();
	Yard->UpdateEncounter(0.5f);
	TestTrue(TEXT("Back on their feet inside, the fight still on: it closes again"), Yard->AreGatesUp() && Seal->IsRaised());

	// Won: its own creatures all down, the gate opens.
	EncounterTestWorld::KillAll(*Yard);
	Yard->UpdateEncounter(0.5f);
	TestTrue(TEXT("Its creatures down: cleared and done"), Yard->GetState() == EEncounterState::Cleared && Yard->IsDone());
	TestTrue(TEXT("...the gate open"), !Yard->AreGatesUp() && !Seal->IsRaised());

	// A player outside when it springs is not shut out: the creatures come, the gate waits for them to walk in.
	ASetPieceSpawner* Far = SpawnArena(World, FVector(12000.0, 0.0, 0.0), [](ASetPieceSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("ArenaTestFar");
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 2) };
		Setup.SpringRadius = 20000.f;
		Setup.GiveUpRadius = 8000.f;
		Setup.bReward = false;
		Setup.ArenaGates = { GateAt(11000.0) };
	});
	if (TestNotNull(TEXT("A second arena, 11 m from the player's side of its wall"), Far))
	{
		Far->UpdateEncounter(0.5f);
		TestTrue(TEXT("Sprung with the player outside: its creatures are out"), Far->NumAlive() == 2);
		TestTrue(TEXT("...and its gate has not risen (no wall was even made)"), !Far->AreGatesUp() && Far->GetGateSeal(0) == nullptr);
		Player->SetActorLocation(FVector(11500.0, 0.0, 0.0));
		Far->UpdateEncounter(0.5f);
		TestTrue(TEXT("The player walks in past the margin: it closes"), Far->AreGatesUp());
		// Taken away while the player is far: the wall must not stand on its own.
		Player->SetActorLocation(FVector(-60000.0, 0.0, 0.0));
		EncounterTestWorld::KillAll(*Far);
		Far->UpdateEncounter(0.5f);
		TestFalse(TEXT("Won (or off): the gate is open"), Far->AreGatesUp());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterLeaderMarkTest, "Looter.SetPiece.Arena.Leader",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterLeaderMarkTest::RunTest(const FString& Parameters)
{
	// A group marked bLeader makes its first creature its pack's leader (once while it is out), and its DisplayName names them.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	AEncounterSpawner* Camp = EncounterTestWorld::SpawnSpawner(World, FVector(1800.0, 0.0, 0.0), [](AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("LeaderTest");
		FEncounterGroup Leader = EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 2, ECreatureRank::Rare);
		Leader.bLeader = true;
		Leader.DisplayName = FText::FromString(TEXT("The Test Elder"));
		Leader.Tags = { TEXT("Find_Test") };
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 3), Leader };
	});
	if (!TestNotNull(TEXT("A camp with a leader group"), Camp))
	{
		return false;
	}
	Camp->UpdateEncounter(0.5f);
	TestEqual(TEXT("Five out"), Camp->NumAlive(), 5);
	int32 Leaders = 0;
	int32 Named = 0;
	int32 Tagged = 0;
	bool bLeaderIsRare = true;
	for (const ACreatureBase* Each : Camp->GetAliveCreatures())
	{
		const bool bLeads = Each->GetPack() && Each->GetPack()->IsLeader();
		Leaders += bLeads ? 1 : 0;
		bLeaderIsRare &= !bLeads || Each->GetRank() == ECreatureRank::Rare;
		Named += Each->DisplayName.ToString() == TEXT("The Test Elder") ? 1 : 0;
		Tagged += Each->ActorHasTag(TEXT("Find_Test")) ? 1 : 0;
	}
	TestEqual(TEXT("One leader among them: its group's first"), Leaders, 1);
	TestTrue(TEXT("...of the marked group's rank"), bLeaderIsRare);
	TestEqual(TEXT("The group's name sits on both its creatures"), Named, 2);
	TestEqual(TEXT("...and so does its own tag, on those two alone"), Tagged, 2);
	return true;
}

#endif
