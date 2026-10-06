#pragma once

// What the encounter tests build in their test levels: spawners and safe zones begun as play begins them, and the
// player's stand-in, made the level's player for its encounters (a test level has no player controller). A test level's
// traces find no ground, so a spawner's creatures stand level with it, and its timer never runs: the tests move it on
// with UpdateEncounter.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Tests/BossTestWorld.h"
#include "World/SafeGround.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Templates/Function.h"

namespace EncounterTestWorld
{
	/** A group of Count of Class, all of one rank. */
	inline FEncounterGroup MakeGroup(TSubclassOf<ACreatureBase> Class, int32 Count, ECreatureRank Rank = ECreatureRank::Basic)
	{
		FEncounterGroup Made;
		Made.CreatureClass = Class;
		Made.Count = Count;
		Made.RankRoll = EEncounterRankRoll::Fixed;
		Made.Rank = Rank;
		return Made;
	}

	/** The player's stand-in (a character with health), the level's player for its encounters. */
	inline ACharacter* SpawnPlayer(UWorld* World, const FVector& Where)
	{
		ACharacter* Player = BossTestWorld::SpawnPlayer(World, Where);
		UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(World);
		if (Player && Encounters)
		{
			Encounters->SetTestPlayer(Player);
		}
		return Player;
	}

	/** A spawner at Where, set up by Setup before its play begins (as a level's is set up before), begun, with a fixed seed. */
	inline AEncounterSpawner* SpawnSpawner(UWorld* World, const FVector& Where, TFunctionRef<void(AEncounterSpawner&)> Setup)
	{
		AEncounterSpawner* Spawner = World->SpawnActor<AEncounterSpawner>(Where, FRotator::ZeroRotator);
		if (!Spawner)
		{
			return nullptr;
		}
		Setup(*Spawner);
		Spawner->DispatchBeginPlay();
		Spawner->SetRandomSeed(20261006);
		return Spawner;
	}

	/** A square safe zone HalfSize (cm) round Center, set up by Setup (its condition), begun. */
	inline ASafeGround* SpawnSquareZone(UWorld* World, const FVector& Center, double HalfSize, TFunctionRef<void(ASafeGround&)> Setup)
	{
		ASafeGround* Zone = World->SpawnActor<ASafeGround>(Center, FRotator::ZeroRotator);
		if (!Zone)
		{
			return nullptr;
		}
		Zone->Corners = { Center + FVector(-HalfSize, -HalfSize, 0.0), Center + FVector(HalfSize, -HalfSize, 0.0),
			Center + FVector(HalfSize, HalfSize, 0.0), Center + FVector(-HalfSize, HalfSize, 0.0) };
		Setup(*Zone);
		Zone->DispatchBeginPlay();
		return Zone;
	}

	/** A creature spawned in play and started as play would start it, dropping no loot (a test level has none to see). */
	inline ACreatureBase* SpawnCreature(UWorld* World, TSubclassOf<ACreatureBase> Class, const FVector& Feet)
	{
		ACreatureBase* Creature = ACreatureBase::SpawnAtRuntime(World, Class, Feet, 0.f, ACreatureBase::FRuntimeSpawn());
		if (Creature && !Creature->HasActorBegunPlay())
		{
			Creature->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Creature);
		return Creature;
	}

	/** Kills every creature the spawner has out, as the game's damage kills them, dropping no loot. */
	inline void KillAll(AEncounterSpawner& Spawner)
	{
		for (ACreatureBase* Creature : Spawner.GetAliveCreatures())
		{
			BossTestWorld::Kill(Creature);
		}
	}

	/** How many of Creatures are of Class at Rank. */
	inline int32 CountOf(const TArray<ACreatureBase*>& Creatures, const UClass* Class, ECreatureRank Rank)
	{
		int32 Found = 0;
		for (const ACreatureBase* Creature : Creatures)
		{
			Found += Creature && Creature->IsA(Class) && Creature->GetRank() == Rank ? 1 : 0;
		}
		return Found;
	}

	/** Every pair of spots at least Apart (cm, seen from above) from each other. */
	inline bool AllApart(const TArray<FVector>& Spots, double Apart)
	{
		for (int32 First = 0; First < Spots.Num(); ++First)
		{
			for (int32 Second = First + 1; Second < Spots.Num(); ++Second)
			{
				if (FVector::Dist2D(Spots[First], Spots[Second]) < Apart)
				{
					return false;
				}
			}
		}
		return true;
	}

	/** Where each creature stands. */
	inline TArray<FVector> PlacesOf(const TArray<ACreatureBase*>& Creatures)
	{
		TArray<FVector> Places;
		for (const ACreatureBase* Creature : Creatures)
		{
			Places.Add(Creature->GetActorLocation());
		}
		return Places;
	}
}

#endif
