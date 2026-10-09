#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/AmbushSpawner.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/PackRules.h"
#include "Creatures/PatrolSpawner.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Templates/Function.h"
#include "Tests/AutomationCommon.h"

// A roaming pack on its road (APatrolSpawner) and a landmark ambush (AAmbushSpawner): the patrol's walk and file, held by
// a fight; the ambush sprung by the player walking onto its ground (not set down there), its dead rising and its spiders
// dropping. Rules first, then in a test level (its traces find no ground, and nothing moves unless the test moves it).

namespace
{
	/** An encounter of kind T at Where, set up by Setup before its play begins, begun, with a fixed seed. */
	template <typename T>
	T* SpawnEncounter(UWorld* World, const FVector& Where, TFunctionRef<void(T&)> Setup)
	{
		T* Made = World->SpawnActor<T>(Where, FRotator::ZeroRotator);
		if (!Made)
		{
			return nullptr;
		}
		Setup(*Made);
		Made->DispatchBeginPlay();
		Made->SetRandomSeed(20261008);
		return Made;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterPatrolTest, "Looter.Encounters.Patrol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterPatrolTest::RunTest(const FString& Parameters)
{
	// The walk: there and back along an open route, resting at each end; round a loop, resting at its first point.
	const TArray<FVector> Road = { FVector::ZeroVector, FVector(1000.0, 0.0, 0.0) };
	TestEqual(TEXT("The road's length"), PackRules::RouteLength(Road, false), 1000.f);
	PackRules::FPatrolWalk Walk;
	TestTrue(TEXT("It walks"), PackRules::AdvancePatrol(Road, false, 100.f, 5.f, 2.f, Walk) && FMath::IsNearlyEqual(Walk.Along, 500.f));
	PackRules::AdvancePatrol(Road, false, 100.f, 6.f, 2.f, Walk);
	TestTrue(TEXT("At the end it turns and rests"), FMath::IsNearlyEqual(Walk.Along, 1000.f) && Walk.Direction < 0.f && Walk.PauseLeft > 0.f);
	TestFalse(TEXT("...not moving while it rests"), PackRules::AdvancePatrol(Road, false, 100.f, 1.f, 2.f, Walk));
	PackRules::AdvancePatrol(Road, false, 100.f, 1.f, 2.f, Walk);
	PackRules::AdvancePatrol(Road, false, 100.f, 3.f, 2.f, Walk);
	TestTrue(TEXT("...then walks back"), FMath::IsNearlyEqual(Walk.Along, 700.f)
		&& PackRules::PatrolHeading(Road, false, Walk).Equals(FVector(-1.0, 0.0, 0.0), 0.001));
	const TArray<FVector> Square = { FVector::ZeroVector, FVector(1000.0, 0.0, 0.0), FVector(1000.0, 1000.0, 0.0), FVector(0.0, 1000.0, 0.0) };
	TestEqual(TEXT("A loop's length comes back to its start"), PackRules::RouteLength(Square, true), 4000.f);
	PackRules::FPatrolWalk Round;
	PackRules::AdvancePatrol(Square, true, 100.f, 25.f, 2.f, Round);
	TestTrue(TEXT("Round past its third corner"), PackRules::PointAlong(Square, true, Round.Along).Equals(FVector(500.0, 1000.0, 0.0), 1.0));
	PackRules::AdvancePatrol(Square, true, 100.f, 16.f, 2.f, Round);
	TestTrue(TEXT("Back at its first point, resting"), FMath::IsNearlyEqual(Round.Along, 100.f, 1.f) && Round.PauseLeft > 0.f);

	// The file: the leader on the point, the rest behind it, right and left of its line.
	const FVector North(1.0, 0.0, 0.0);
	TestTrue(TEXT("The leader on the point"), PackRules::FormationSpot(FVector::ZeroVector, North, 0, 260.f).Equals(FVector::ZeroVector));
	const FVector Second = PackRules::FormationSpot(FVector::ZeroVector, North, 1, 260.f);
	const FVector Third = PackRules::FormationSpot(FVector::ZeroVector, North, 2, 260.f);
	TestTrue(TEXT("The second behind and to the right, the third behind and to the left"), Second.X < 0.0 && Second.Y > 0.0 && Third.X < 0.0
		&& Third.Y < 0.0);

	// In a level: three spiders (a Restless one leads) on a 30 m road; the player 30 m off its side.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 3000.0, 0.0));
	APatrolSpawner* Patrol = SpawnEncounter<APatrolSpawner>(World, FVector::ZeroVector, [](APatrolSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("TestRoad");
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 2),
			EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1, ECreatureRank::Rare) };
		Setup.PatrolRoute = { FVector(-1500.0, 0.0, 0.0), FVector(1500.0, 0.0, 0.0) };
		// Its ground reaches the player, so one of them can turn on them.
		Setup.GiveUpRadius = 4000.f;
	});
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Patrol"), Patrol))
	{
		return false;
	}
	TestTrue(TEXT("Its point starts at its route's first point"), Patrol->GetPatrolPoint().Equals(FVector(-1500.0, 0.0, 0.0), 1.0));
	Patrol->UpdateEncounter(0.5f);
	TArray<ACreatureBase*> Pack = Patrol->GetAliveCreatures();
	if (!TestEqual(TEXT("The player near: its pack is out"), Pack.Num(), 3))
	{
		return false;
	}
	// They came out round the route's first point (the pack's point then); it may have walked a step since.
	const FVector Start = Patrol->GetWorldRoute()[0];
	const FVector Point = Patrol->GetPatrolPoint();
	for (const ACreatureBase* Creature : Pack)
	{
		TestTrue(TEXT("...each out round the pack's point"), FVector::Dist2D(Creature->GetActorLocation(), Start) <= Patrol->SpawnRadius + 1.0);
		TestTrue(TEXT("...with its place in the file"), Creature->GetPack()->HasRoamAnchor());
		if (Creature->GetRank() == ECreatureRank::Rare)
		{
			TestTrue(TEXT("The Restless one leads, on the point"), FVector::Dist2D(Creature->GetPack()->GetRoamAnchor(), Point) < 1.0);
		}
	}

	// Each in its place: the pack walks on at its pace (a test level moves no one, so the test keeps them in their places).
	auto KeepPlaces = [&Pack]()
	{
		for (ACreatureBase* Creature : Pack)
		{
			Creature->SetActorLocation(Creature->GetPack()->GetRoamAnchor());
		}
	};
	KeepPlaces();
	const float Along = Patrol->GetWalk().Along;
	Patrol->WalkFor(2.f);
	TestTrue(TEXT("Everyone in their place: it walks on"), Patrol->IsWalking());
	TestTrue(FString::Printf(TEXT("...at its pace (%.0f cm in 2 s)"), Patrol->GetWalk().Along - Along),
		FMath::IsNearlyEqual(Patrol->GetWalk().Along - Along, Patrol->GetPace() * 2.f, 1.f));
	TestTrue(TEXT("...the stroll its slowest member keeps up with"), FMath::IsNearlyEqual(Patrol->GetPace(),
		GetDefault<ASpiderCreature>()->WalkSpeed * Patrol->PaceShare, 0.5f));

	// One of them turns on the player: the walk holds until it's back in its place.
	KeepPlaces();
	Pack[0]->AlertTo(Player);
	const float Held = Patrol->GetWalk().Along;
	Patrol->WalkFor(2.f);
	TestTrue(TEXT("A fight holds the walk"), !Patrol->IsWalking() && FMath::IsNearlyEqual(Patrol->GetWalk().Along, Held));
	Pack[0]->DevPutInState(ECreatureState::Return);
	Pack[0]->SetActorLocation(Pack[0]->GetPack()->GetRoamAnchor() + FVector(0.0, 900.0, 0.0));
	Patrol->WalkFor(2.f);
	TestFalse(TEXT("...and a straggler, walking back"), Patrol->IsWalking());
	KeepPlaces();
	Patrol->WalkFor(2.f);
	TestTrue(TEXT("All back in the file: it walks on"), Patrol->IsWalking());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterAmbushTest, "Looter.Encounters.Ambush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterAmbushTest::RunTest(const FString& Parameters)
{
	// The trigger: walked in since the last look, not set down inside, and never on the first look.
	const FVector Out(1500.0, 0.0, 0.0);
	const FVector In(800.0, 0.0, 0.0);
	TestTrue(TEXT("Walked in"), PackRules::IsWalkIn(true, false, true, Out, In, 1200.f));
	TestFalse(TEXT("Set down inside from far off (a respawn)"), PackRules::IsWalkIn(true, false, true, FVector(9000.0, 0.0, 0.0), In, 1200.f));
	TestFalse(TEXT("Inside at the first look"), PackRules::IsWalkIn(false, false, true, Out, In, 1200.f));
	TestFalse(TEXT("Inside both looks"), PackRules::IsWalkIn(true, true, true, In, In, 1200.f));

	// In a level: the churchyard's kind of ambush, a square 20 m across, two of the dead rising at its far side.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(5000.0, 0.0, 0.0));
	// Its ground (world corners, 10 m round Middle) and its spots (relative to it) on the far side from the player's way in.
	auto Square = [](AAmbushSpawner& Setup, const FVector& Middle)
	{
		Setup.AmbushCorners = { Middle + FVector(-1000.0, -1000.0, 0.0), Middle + FVector(1000.0, -1000.0, 0.0),
			Middle + FVector(1000.0, 1000.0, 0.0), Middle + FVector(-1000.0, 1000.0, 0.0) };
		Setup.SpawnPoints = { FVector(-700.0, 0.0, 0.0), FVector(-700.0, 400.0, 0.0) };
		Setup.GiveUpRadius = 3000.f;
	};
	AAmbushSpawner* Yard = SpawnEncounter<AAmbushSpawner>(World, FVector::ZeroVector, [&Square](AAmbushSpawner& Setup)
	{
		Square(Setup, FVector::ZeroVector);
		Setup.SpawnerId = TEXT("TestYard");
		Setup.Groups = { EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 2) };
		Setup.Entrance = EAmbushEntrance::Rise;
	});
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Ambush"), Yard))
	{
		return false;
	}
	const double PlayerZ = Player->GetActorLocation().Z;
	auto LookAt = [Player, PlayerZ](AAmbushSpawner& Ambush, const FVector& Where)
	{
		Player->SetActorLocation(FVector(Where.X, Where.Y, PlayerZ));
		Ambush.UpdateEncounter(0.5f);
	};
	LookAt(*Yard, FVector(5000.0, 0.0, 0.0));
	TestTrue(TEXT("The player far off: it waits"), Yard->GetState() == EEncounterState::Waiting && Yard->NumAlive() == 0);
	LookAt(*Yard, FVector(0.0, 500.0, 0.0));
	TestTrue(TEXT("Set down inside (a respawn grave): it doesn't spring"), Yard->GetState() == EEncounterState::Waiting && Yard->NumAlive() == 0);
	LookAt(*Yard, FVector(1500.0, 0.0, 0.0));
	TestEqual(TEXT("Walked out: still waiting"), Yard->NumAlive(), 0);
	LookAt(*Yard, FVector(800.0, 0.0, 0.0));
	const TArray<ACreatureBase*> Risen = Yard->GetAliveCreatures();
	TestTrue(TEXT("Walked back in through its fence: it springs"), Yard->GetState() == EEncounterState::Engaged && Risen.Num() == 2);
	for (const ACreatureBase* Creature : Risen)
	{
		const AUnpaidCreature* Dead = Cast<AUnpaidCreature>(Creature);
		TestTrue(TEXT("...the dead rising where they stand (fading in)"), Dead && Dead->IsPhasing() && Dead->GetPhase() > 0.5f);
		TestTrue(TEXT("...and coming for the player"), Creature->GetCreatureState() == ECreatureState::Chase && Creature->GetTarget() == Player);
		TestTrue(TEXT("...from its far side, not in the player's face"), FVector::Dist2D(Creature->GetActorLocation(), Player->GetActorLocation()) >= 800.0);
	}

	// A player inside as it comes on (the story turning it on at the vestry door) has walked in before: no spring.
	AAmbushSpawner* Standing = SpawnEncounter<AAmbushSpawner>(World, FVector(0.0, 20000.0, 0.0), [&Square](AAmbushSpawner& Setup)
	{
		Square(Setup, FVector(0.0, 20000.0, 0.0));
		Setup.SpawnerId = TEXT("TestStanding");
		Setup.Groups = { EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 1) };
	});
	if (!TestNotNull(TEXT("Second ambush"), Standing))
	{
		return false;
	}
	LookAt(*Standing, FVector(0.0, 20300.0, 0.0));
	LookAt(*Standing, FVector(200.0, 20300.0, 0.0));
	TestEqual(TEXT("Inside from its first look: it stays quiet"), Standing->NumAlive(), 0);

	// The Webwood's kind: spiders dropping from the trees, falling from over their feet.
	AAmbushSpawner* Wood = SpawnEncounter<AAmbushSpawner>(World, FVector(0.0, -20000.0, 0.0), [&Square](AAmbushSpawner& Setup)
	{
		Square(Setup, FVector(0.0, -20000.0, 0.0));
		Setup.SpawnerId = TEXT("TestWood");
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1) };
		Setup.Entrance = EAmbushEntrance::Drop;
		Setup.DropHeight = 350.f;
	});
	if (!TestNotNull(TEXT("Third ambush"), Wood))
	{
		return false;
	}
	LookAt(*Wood, FVector(1500.0, -20000.0, 0.0));
	LookAt(*Wood, FVector(800.0, -20000.0, 0.0));
	const TArray<ACreatureBase*> Dropped = Wood->GetAliveCreatures();
	if (TestEqual(TEXT("Walked into the wood: a spider drops"), Dropped.Num(), 1))
	{
		const ACreatureBase* Spider = Dropped[0];
		const double Lift = Spider->GetActorLocation().Z - Spider->GetHome().GetLocation().Z;
		TestTrue(FString::Printf(TEXT("...from %.0f cm over where it lands"), Lift), FMath::IsNearlyEqual(Lift, 350.0, 5.0));
		TestTrue(TEXT("...falling"), Spider->GetCharacterMovement()->MovementMode == MOVE_Falling);
	}
	return true;
}

#endif
