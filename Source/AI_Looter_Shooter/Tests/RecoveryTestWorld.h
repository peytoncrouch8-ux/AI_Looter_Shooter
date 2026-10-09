#pragma once

// What the recovery tests build in their test levels: a player stand-in with health, damage dealt as the game deals it,
// a spider of a chosen rank, and a count of the soul-motes in the level.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/LootDropComponent.h"
#include "Loot/SoulMotePickup.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "UObject/Script.h"

namespace RecoveryTestWorld
{
	/** A player stand-in: a plain character with health (a test level has no game mode), started as play starts it. */
	inline ACharacter* SpawnPlayer(UWorld* World, const FVector& Where, UHealthComponent*& OutHealth)
	{
		ACharacter* Player = World->SpawnActor<ACharacter>(Where, FRotator::ZeroRotator);
		OutHealth = nullptr;
		if (!Player)
		{
			return nullptr;
		}
		OutHealth = NewObject<UHealthComponent>(Player, TEXT("Health"));
		OutHealth->bShowDamageNumbers = false;
		Player->AddInstanceComponent(OutHealth);
		OutHealth->RegisterComponent();
		Player->DispatchBeginPlay();
		return Player;
	}

	/**
	 * Damage as AActor::TakeDamage hands it to its listeners: a test level has no game mode, so a pawn would ignore
	 * ApplyDamage, and the engine drops actors' own events until play begins.
	 */
	inline void Hurt(AActor* Victim, float Damage)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		Victim->OnTakeAnyDamage.Broadcast(Victim, Damage, GetDefault<UDamageType>(), nullptr, nullptr);
	}

	/** A spider in a test level, started as play starts it, dropping nothing from its table when it dies (unless bDropsLoot). */
	inline ASpiderCreature* SpawnSpider(UWorld* World, const FVector& Where, ECreatureRank Rank, bool bDropsLoot = false)
	{
		ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(Where, FRotator::ZeroRotator);
		if (Spider)
		{
			Spider->StartingRank = Rank;
			if (ULootDropComponent* Loot = Spider->FindComponentByClass<ULootDropComponent>())
			{
				Loot->bDropOnDeath = bDropsLoot;
			}
			Spider->DispatchBeginPlay();
		}
		return Spider;
	}

	/** The soul-motes in the level that are not on their way out. */
	inline int32 CountMotes(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<ASoulMotePickup> It(World); It; ++It)
		{
			Count += It->IsActorBeingDestroyed() ? 0 : 1;
		}
		return Count;
	}

	/** Whether a mote is still in the level (not destroyed, not on its way out). */
	inline bool IsHere(const AActor* Actor)
	{
		return IsValid(Actor) && !Actor->IsActorBeingDestroyed();
	}
}

#endif
