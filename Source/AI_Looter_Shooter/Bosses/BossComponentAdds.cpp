#include "Bosses/BossComponent.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossRules.h"
#include "Bosses/BossSeal.h"
#include "Creatures/CreatureBase.h"
#include "World/WorldQueries.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

// UBossComponent's adds: waves rising round the boss, kept under their caps, gone with the fight. The fight's course is in
// BossComponent.cpp.

int32 UBossComponent::SpawnWave(const FBossAddWave& Wave)
{
	ACreatureBase* Creature = GetCreature();
	UWorld* World = GetWorld();
	if (!Creature || !World || Creature->IsDead())
	{
		return 0;
	}
	PruneAdds();
	const int32 Count = BossRules::AddsToSpawn(Wave.Count, NumAliveAdds(), Wave.MaxAlive, MaxAliveAdds);
	if (Count <= 0)
	{
		return 0;
	}
	const TSubclassOf<ACreatureBase> Kind = Wave.CreatureClass ? Wave.CreatureClass : TSubclassOf<ACreatureBase>(Creature->GetClass());
	// Spawned in play, so they never come back once killed (ACreatureBase::SpawnAtRuntime).
	ACreatureBase::FRuntimeSpawn Spawn;
	Spawn.Rank = Wave.Rank;
	Spawn.Level = Wave.Level;
	Spawn.BodyScale = Wave.BodyScale;

	// They rise round the boss on its own level of ground (creatures never step down a drop, and every fight keeps to one
	// level), inside its wall: one ring of spots evenly round it, then a second between them for any that didn't fit. A wave
	// that rises round the fight's spot (its home, the arena's middle) does so wherever the boss has gone.
	const FVector Middle = Wave.bAroundSpot && bFighting ? Spot : Creature->GetActorLocation();
	const float FeetZ = static_cast<float>(Middle.Z) - Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Feet(Middle.X, Middle.Y, FeetZ);
	FVector BossGround;
	const bool bGroundUnderBoss = TraceGround(Feet, BossGround);
	const float Start = FMath::FRandRange(0.f, 360.f);
	TArray<FVector> Spots = BossRules::RingPoints(Feet, Wave.Radius, Count, Start);
	Spots.Append(BossRules::RingPoints(Feet, Wave.Radius, Count, Start + 180.f / static_cast<float>(Count)));
	const ABossSeal* ActiveSeal = GetActiveSeal();
	APawn* Fighter = FightPlayer.Get();

	int32 Spawned = 0;
	for (const FVector& Candidate : Spots)
	{
		if (Spawned >= Count)
		{
			break;
		}
		if (ActiveSeal && ActiveSeal->IsRaised() && !ActiveSeal->IsInside(Candidate, 100.f))
		{
			continue;
		}
		FVector AddFeet;
		if (!FindAddSpot(Candidate, FeetZ, bGroundUnderBoss, AddFeet))
		{
			continue;
		}
		const float Yaw = static_cast<float>(Fighter ? (Fighter->GetActorLocation() - AddFeet).Rotation().Yaw : Creature->GetActorRotation().Yaw);
		ACreatureBase* Risen = ACreatureBase::SpawnAtRuntime(World, Kind, AddFeet, Yaw, Spawn);
		if (!Risen)
		{
			continue;
		}
		// In a level that isn't playing yet (a test level), it starts as play would start it.
		if (!Risen->HasActorBegunPlay())
		{
			Risen->DispatchBeginPlay();
		}
		Risen->AlertTo(Fighter);
		Adds.Add(Risen);
		++Spawned;
	}
	UE_LOG(LogLooter, Log, TEXT("Boss %s: %d of %d adds rose (%d alive)."), *GetLabel(), Spawned, Wave.Count, NumAliveAdds());
	return Spawned;
}

bool UBossComponent::TraceGround(const FVector& Point, FVector& OutGround) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	// World-static only, skipping volumes and the playable area's walls: the terrain or the deck under it.
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("BossAddGround"), GetOwner());
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Point + FVector(0.0, 0.0, MaxAddRise), Point - FVector(0.0, 0.0, MaxAddRise * 2.f),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		OutGround = Hit.ImpactPoint;
		return true;
	}
	return false;
}

bool UBossComponent::FindAddSpot(const FVector& Point, float FeetZ, bool bGroundUnderBoss, FVector& OutFeet) const
{
	const FVector Level(Point.X, Point.Y, FeetZ);
	if (!bGroundUnderBoss)
	{
		// No ground anywhere near the boss (a test level): level with its feet.
		OutFeet = Level;
		return true;
	}
	FVector Ground;
	// Off an edge, or up on a rock or down a drop: not this spot.
	if (!TraceGround(Level, Ground) || FMath::Abs(Ground.Z - FeetZ) > MaxAddRise)
	{
		return false;
	}
	OutFeet = Ground;
	return true;
}

void UBossComponent::PruneAdds()
{
	Adds.RemoveAll([](const TWeakObjectPtr<ACreatureBase>& Each) { return !Each.IsValid() || Each->IsDead(); });
}

int32 UBossComponent::NumAliveAdds() const
{
	int32 Alive = 0;
	for (const TWeakObjectPtr<ACreatureBase>& Each : Adds)
	{
		Alive += Each.IsValid() && !Each->IsDead() ? 1 : 0;
	}
	return Alive;
}

TArray<ACreatureBase*> UBossComponent::GetAliveAdds() const
{
	TArray<ACreatureBase*> Alive;
	for (const TWeakObjectPtr<ACreatureBase>& Each : Adds)
	{
		if (Each.IsValid() && !Each->IsDead())
		{
			Alive.Add(Each.Get());
		}
	}
	return Alive;
}

void UBossComponent::DespawnAdds()
{
	// They fade with the fight: no loot and no experience, nothing left to come back.
	for (const TWeakObjectPtr<ACreatureBase>& Each : Adds)
	{
		if (ACreatureBase* Risen = Each.Get())
		{
			if (!Risen->IsDead())
			{
				Risen->Destroy();
			}
		}
	}
	Adds.Reset();
}
