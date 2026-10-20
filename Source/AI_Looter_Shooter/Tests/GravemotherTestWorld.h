#pragma once
// What the Gravemother's tests build in their test levels: her (as her lair spawns her), damage dealt as the game deals it,
// a floor to land on, her pieces moved on (a test level never ticks).

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherMound.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "Templates/Function.h"

namespace GravemotherTestWorld
{
	inline constexpr float Step = 0.05f;

	/** The Gravemother as her lair spawns her (Legendary), at OwnLevel before her rank's, begun, dropping no loot. */
	inline AGravemotherCreature* SpawnMother(UWorld* World, const FVector& Feet, int32 OwnLevel = 1)
	{
		ACreatureBase::FRuntimeSpawn Spawn;
		Spawn.Rank = ECreatureRank::Legendary;
		Spawn.Level = OwnLevel;
		AGravemotherCreature* Mother = Cast<AGravemotherCreature>(
			ACreatureBase::SpawnAtRuntime(World, AGravemotherCreature::StaticClass(), Feet, 0.f, Spawn));
		if (Mother && !Mother->HasActorBegunPlay())
		{
			Mother->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Mother);
		return Mother;
	}

	/** Hurts a creature down to Share of its health, as the game's damage arrives, from By (or nobody). */
	inline void HurtTo(AActor* Actor, float Share, AController* By = nullptr)
	{
		const UHealthComponent* Health = Actor ? Actor->FindComponentByClass<UHealthComponent>() : nullptr;
		if (Health)
		{
			BossTestWorld::Hurt(Actor, Health->GetHealth() - Health->GetMaxHealth() * Share, By);
		}
	}

	/** A hit on one of her bones at Where (world; none: no point), as a bullet reports it. */
	inline FHitResult HitOn(ACreatureBase& Creature, FName Bone, const FVector& Where = FVector::ZeroVector)
	{
		FHitResult Hit;
		Hit.Component = Creature.GetMesh();
		Hit.BoneName = Bone;
		Hit.ImpactPoint = Where;
		Hit.Location = Where;
		return Hit;
	}

	/** Her feet: the ground under her middle. */
	inline FVector FeetOf(const ACreatureBase& Creature)
	{
		return Creature.GetActorLocation() - FVector(0.0, 0.0, Creature.GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	}

	/** A flat floor round Center (world-static, its top at Center's height), HalfSize across each way. */
	inline AActor* SpawnFloor(UWorld* World, const FVector& Center, double HalfSize)
	{
		return EncounterTestWorld::SpawnBlocks(World, { FBox(Center - FVector(HalfSize, HalfSize, 40.0), Center + FVector(HalfSize, HalfSize, 0.0)) });
	}

	/** Moves her on a frame at a time until Done holds (at most Limit seconds); the seconds it took, or negative. */
	inline float TickUntil(AGravemotherCreature* Mother, float Limit, TFunctionRef<bool()> Done)
	{
		for (float Time = 0.f; Mother && Time <= Limit; Time += Step)
		{
			if (Done())
			{
				return Time;
			}
			Mother->Tick(Step);
		}
		return -1.f;
	}

	inline void TickFor(AGravemotherCreature* Mother, float Seconds)
	{
		for (float Time = 0.f; Mother && Time < Seconds - KINDA_SMALL_NUMBER; Time += Step)
		{
			Mother->Tick(Step);
		}
	}

	/** Moves her mounds on (they tick themselves in a game). */
	inline void AdvanceMounds(AGravemotherCreature& Mother, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds - KINDA_SMALL_NUMBER; Time += Step)
		{
			for (AGravemotherMound* Mound : Mother.GetMounds())
			{
				Mound->Advance(Step);
			}
		}
	}
}

#endif
