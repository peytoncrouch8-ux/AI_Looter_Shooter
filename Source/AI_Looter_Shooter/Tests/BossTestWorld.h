#pragma once

// What the boss tests build in their test levels: a player stand-in, the test boss's fight, damage dealt as the game deals it.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/BossComponent.h"
#include "Bosses/BossTestSpider.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Loot/LootDropComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "Misc/AutomationTest.h"
#include "UObject/Script.h"

namespace BossTestWorld
{
	/**
	 * Damage as AActor::TakeDamage hands it to its listeners. A test level has no game mode, and without one a pawn ignores
	 * ApplyDamage (APawn::ShouldTakeDamage), so the tests deal damage where the game's damage arrives.
	 */
	inline void Hurt(AActor* Victim, float Damage, AController* By = nullptr)
	{
		if (Victim)
		{
			// A test level never readies its actors for play, and until a level has, the engine drops every event bound to
			// an actor's own functions (AActor::ProcessEvent): a creature would never hear of its own death. Let them run,
			// as they do in the game.
			FEditorScriptExecutionGuard RunActorEvents;
			Victim->OnTakeAnyDamage.Broadcast(Victim, Damage, GetDefault<UDamageType>(), By, nullptr);
		}
	}

	/** Hurts the boss to half a point under a share of its health (or a little over it, with bOver). */
	inline void HurtTo(UBossComponent& Boss, float Share, bool bOver = false)
	{
		const UHealthComponent* Health = Boss.GetBossHealth();
		if (Health)
		{
			const float Wanted = Health->GetMaxHealth() * Share + (bOver ? 2.f : -0.5f);
			Hurt(Boss.GetOwner(), Health->GetHealth() - Wanted);
		}
	}

	/** Nothing a test kills drops loot: a test level has none to see (a boss's loot shower neither). */
	inline void NoLoot(AActor* Actor)
	{
		if (ULootDropComponent* Loot = Actor ? Actor->FindComponentByClass<ULootDropComponent>() : nullptr)
		{
			Loot->bDropOnDeath = false;
		}
		if (UBossComponent* Boss = Actor ? Actor->FindComponentByClass<UBossComponent>() : nullptr)
		{
			Boss->LootShower.bEnabled = false;
		}
	}

	inline void Kill(AActor* Actor)
	{
		NoLoot(Actor);
		Hurt(Actor, 1.0e7f);
	}

	inline void KillAdds(UBossComponent& Boss)
	{
		for (ACreatureBase* Add : Boss.GetAliveAdds())
		{
			Kill(Add);
		}
	}

	/** The player's stand-in: a plain character with health (no creature), started as play starts it. */
	inline ACharacter* SpawnPlayer(UWorld* World, const FVector& Where)
	{
		ACharacter* Player = World->SpawnActor<ACharacter>(Where, FRotator::ZeroRotator);
		if (!Player)
		{
			return nullptr;
		}
		UHealthComponent* Health = NewObject<UHealthComponent>(Player, TEXT("Health"));
		Health->bShowDamageNumbers = false;
		Player->AddInstanceComponent(Health);
		Health->RegisterComponent();
		Player->DispatchBeginPlay();
		return Player;
	}

	/** The test boss standing at the origin, its fight started against a player stand-in 6 m in front of it. */
	inline UBossComponent* StartTestFight(FAutomationTestBase& Test, UWorld* World, ACharacter*& OutPlayer,
		int32 PhaseCount = BossTestSpider::DefaultPhases)
	{
		OutPlayer = SpawnPlayer(World, FVector(600.0, 0.0, 0.0));
		UBossComponent* Boss = BossTestSpider::Spawn(World, FVector::ZeroVector, 0.f, PhaseCount);
		if (!Test.TestNotNull(TEXT("Player stand-in"), OutPlayer) || !Test.TestNotNull(TEXT("Test boss"), Boss))
		{
			return nullptr;
		}
		NoLoot(Boss->GetOwner());
		Test.TestTrue(TEXT("Before its fight it waits, hunting nobody"), Boss->GetCreature()->IsPassive());
		Boss->StartFight(OutPlayer);
		Test.TestTrue(TEXT("The fight is on"), Boss->IsFighting());
		return Boss;
	}
}

#endif
