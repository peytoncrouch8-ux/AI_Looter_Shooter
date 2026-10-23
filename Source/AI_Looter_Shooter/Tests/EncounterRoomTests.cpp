#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/AutomationCommon.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

// A fight never spawns nothing while its ground has room (the play-check of 2026-10-23: the depot's waiting room lies within
// 8 m of Mr. Sloane's door upstairs and of Fenn's coffin, and its two fights logged "nowhere to stand"; the baggage room's
// rats, on a 1.4 m ring, came two at a time). Its own rules first: 8 m from the player, its spacing apart. Then nearer the
// player, the farthest spots first, then closer together (AEncounterSpawner::SpawnSpotTries); never off its ground.

namespace
{
	/** Every spot of Creatures inside the square HalfSize round Center (seen from above). */
	bool AllInside(const TArray<ACreatureBase*>& Creatures, const FVector& Center, double HalfSize)
	{
		for (const ACreatureBase* Creature : Creatures)
		{
			const FVector At = Creature->GetActorLocation() - Center;
			if (FMath::Abs(At.X) > HalfSize + 1.0 || FMath::Abs(At.Y) > HalfSize + 1.0)
			{
				return false;
			}
		}
		return true;
	}

	/** The nearest any of Creatures stands to Spot (seen from above). */
	double Nearest(const TArray<ACreatureBase*>& Creatures, const FVector& Spot)
	{
		double Best = TNumericLimits<double>::Max();
		for (const ACreatureBase* Creature : Creatures)
		{
			Best = FMath::Min(Best, FVector::Dist2D(Creature->GetActorLocation(), Spot));
		}
		return Best;
	}

	/** A square ground HalfSize round Center as a spawner's corners. */
	TArray<FVector> SquareCorners(const FVector& Center, double HalfSize)
	{
		return { Center + FVector(-HalfSize, -HalfSize, 0.0), Center + FVector(HalfSize, -HalfSize, 0.0),
			Center + FVector(HalfSize, HalfSize, 0.0), Center + FVector(-HalfSize, HalfSize, 0.0) };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterRoomTest, "Looter.Encounters.Room",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterRoomTest::RunTest(const FString& Parameters)
{
	// The looks it makes: its rules', then 3.5 m from the player, then as close as its bodies allow, then 2 m; no repeats.
	const TArray<FEncounterSpotTry> Tries = AEncounterSpawner::SpawnSpotTries(800.f, 250.f, 62.f);
	TestTrue(TEXT("Four looks for a spider's fight"), Tries.Num() == 4);
	if (Tries.Num() == 4)
	{
		TestTrue(TEXT("...its own rules first"), FMath::IsNearlyEqual(Tries[0].ClearOfPlayer, 800.f) && FMath::IsNearlyEqual(Tries[0].Spacing, 250.f));
		TestTrue(TEXT("...then 3.5 m from the player"), FMath::IsNearlyEqual(Tries[1].ClearOfPlayer, 350.f) && FMath::IsNearlyEqual(Tries[1].Spacing, 250.f));
		TestTrue(TEXT("...then its bodies apart (2 x 62 + 30)"), FMath::IsNearlyEqual(Tries[2].Spacing, 154.f));
		TestTrue(TEXT("...then 2 m from the player"), FMath::IsNearlyEqual(Tries[3].ClearOfPlayer, 200.f) && FMath::IsNearlyEqual(Tries[3].Spacing, 154.f));
	}
	TestEqual(TEXT("Rules already that close: one look"), AEncounterSpawner::SpawnSpotTries(150.f, 100.f, 30.f).Num(), 1);

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const float Clear = UEncounterSettings::Get().MinSpawnDistanceFromPlayer;

	// A room 10 m square (the waiting room's size) with the player in its middle: every spot is within 8 m of them. Its six
	// all come, nearer than 8 m but none within 2 m, all in the room.
	const FVector Room(0.0, 0.0, 0.0);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, Room + FVector(0.0, 0.0, 100.0));
	AEncounterSpawner* Waiting = EncounterTestWorld::SpawnSpawner(World, Room, [&Room](AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("TestWaitingRoom");
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 6) };
		Setup.SpawnPoints = { FVector(-350.0, -250.0, 0.0), FVector(350.0, -250.0, 0.0), FVector(-350.0, 250.0, 0.0), FVector(350.0, 250.0, 0.0) };
		Setup.GroundCorners = SquareCorners(Room, 500.0);
		Setup.ActivationRadius = 2600.f;
		Setup.bHuntOnSpawn = true;
	});
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The room's spawner"), Waiting))
	{
		return false;
	}
	for (int32 Look = 0; Look < 4 && Waiting->NumAlive() < 6; ++Look)
	{
		Waiting->UpdateEncounter(0.5f);
	}
	const TArray<ACreatureBase*> InRoom = Waiting->GetAliveCreatures();
	TestEqual(TEXT("The player in the middle of a small room: all six come"), InRoom.Num(), 6);
	TestTrue(FString::Printf(TEXT("...none within 2 m of the player (nearest %.0f cm)"), Nearest(InRoom, Player->GetActorLocation())),
		Nearest(InRoom, Player->GetActorLocation()) >= 199.0);
	TestTrue(TEXT("...all in the room"), AllInside(InRoom, Room, 500.0));

	// Open ground with room 8 m out: its own rule holds, none nearer than 8 m.
	const FVector Field(20000.0, 0.0, 0.0);
	Player->SetActorLocation(Field + FVector(0.0, 0.0, 100.0));
	AEncounterSpawner* Open = EncounterTestWorld::SpawnSpawner(World, Field, [](AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("TestField");
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 4) };
		Setup.SpawnRadius = 1500.f;
		Setup.ActivationRadius = 2600.f;
	});
	if (!TestNotNull(TEXT("The field's spawner"), Open))
	{
		return false;
	}
	Open->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> InField = Open->GetAliveCreatures();
	TestEqual(TEXT("Open ground: its four come"), InField.Num(), 4);
	TestTrue(FString::Printf(TEXT("...none nearer the player than its rule (nearest %.0f cm)"), Nearest(InField, Player->GetActorLocation())),
		Nearest(InField, Player->GetActorLocation()) >= Clear - 1.0);

	// The baggage room's rats: four spots on a 1.4 m ring (2 m apart, under the 2.5 m spacing), the player 15 m off. All four come, as close together as their bodies allow, none on another.
	const FVector Baggage(-20000.0, 0.0, 0.0);
	Player->SetActorLocation(Baggage + FVector(1500.0, 0.0, 100.0));
	AEncounterSpawner* Rats = EncounterTestWorld::SpawnSpawner(World, Baggage, [&Baggage](AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("TestBaggage");
		Setup.Groups = { EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 4) };
		Setup.SpawnPoints = { FVector(140.0, 0.0, 0.0), FVector(0.0, 140.0, 0.0), FVector(-140.0, 0.0, 0.0), FVector(0.0, -140.0, 0.0) };
		Setup.GroundCorners = SquareCorners(Baggage, 230.0);
		Setup.ActivationRadius = 2600.f;
	});
	if (!TestNotNull(TEXT("The baggage room's spawner"), Rats))
	{
		return false;
	}
	for (int32 Look = 0; Look < 4 && Rats->NumAlive() < 4; ++Look)
	{
		Rats->UpdateEncounter(0.5f);
	}
	const TArray<ACreatureBase*> InBaggage = Rats->GetAliveCreatures();
	TestEqual(TEXT("Four on a ring closer than its spacing: all four come"), InBaggage.Num(), 4);
	TestTrue(TEXT("...all in the room"), AllInside(InBaggage, Baggage, 230.0));
	double Closest = TNumericLimits<double>::Max();
	for (int32 A = 0; A < InBaggage.Num(); ++A)
	{
		for (int32 B = A + 1; B < InBaggage.Num(); ++B)
		{
			Closest = FMath::Min(Closest, FVector::Dist2D(InBaggage[A]->GetActorLocation(), InBaggage[B]->GetActorLocation()));
		}
	}
	TestTrue(FString::Printf(TEXT("...a body apart at the least (closest %.0f cm)"), Closest), Closest >= 99.0);
	return true;
}

#endif
