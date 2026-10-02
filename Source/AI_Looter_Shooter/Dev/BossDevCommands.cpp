// Developer console commands for boss fights (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossTestSpider.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "World/WorldQueries.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	/** Every boss in the world (a creature with a boss component). */
	TArray<UBossComponent*> FindBosses(UWorld* World)
	{
		TArray<UBossComponent*> Bosses;
		if (!World)
		{
			return Bosses;
		}
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			if (UBossComponent* Boss = It->FindComponentByClass<UBossComponent>())
			{
				Bosses.Add(Boss);
			}
		}
		return Bosses;
	}

	void BossTest(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Boss.Test: no player (start the game first)."));
			return;
		}
		const int32 PhaseCount = Args.Num() > 0
			? FMath::Clamp(FCString::Atoi(*Args[0]), BossTestSpider::MinPhases, BossTestSpider::MaxPhases) : BossTestSpider::DefaultPhases;

		// One test boss at a time: an earlier one goes, with its adds and its wall.
		TArray<ACreatureBase*> Earlier;
		for (TActorIterator<ACreatureBase> It(GameWorld); It; ++It)
		{
			if (It->Tags.Contains(BossTestSpider::Tag()))
			{
				Earlier.Add(*It);
			}
		}
		for (ACreatureBase* Old : Earlier)
		{
			Old->Destroy();
		}

		// A few metres ahead of where the player looks, on the ground, facing them: well inside its 15 m ring.
		const FVector Ahead = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector();
		const FVector Spot = Pawn->GetActorLocation() + Ahead * 700.f;
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(GameWorld, TEXT("BossTest"));
		FHitResult Hit;
		if (!GameWorld->LineTraceSingleByObjectType(Hit, Spot + FVector(0.0, 0.0, 800.0), Spot - FVector(0.0, 0.0, 3000.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Boss.Test: no ground in front of the player."));
			return;
		}
		const float Yaw = static_cast<float>((Pawn->GetActorLocation() - Hit.ImpactPoint).Rotation().Yaw);
		UBossComponent* Boss = BossTestSpider::Spawn(GameWorld, Hit.ImpactPoint, Yaw, PhaseCount);
		if (!Boss)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Boss.Test: the test boss couldn't be spawned."));
			return;
		}
		Boss->StartFight(Pawn);
		const UHealthComponent* Health = Boss->GetBossHealth();
		UE_LOG(LogLooter, Log, TEXT("Looter.Boss.Test: %s, %d phases, %.0f health; its fog wall closes a %.0f m ring round it."),
			*Boss->BossName.ToString(), Boss->NumPhases(), Health ? Health->GetMaxHealth() : 0.f, BossTestSpider::SealRadius / 100.f);
	}

	void BossReset(const TArray<FString>& Args, UWorld* World)
	{
		int32 Reset = 0;
		for (UBossComponent* Boss : FindBosses(FindGameWorld(World)))
		{
			if (Boss->IsFighting())
			{
				Boss->ResetFight();
				++Reset;
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Boss.Reset: %d boss fight%s started over."), Reset, Reset == 1 ? TEXT("") : TEXT("s"));
	}

	void BossKill(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		int32 Killed = 0;
		for (UBossComponent* Boss : FindBosses(GameWorld))
		{
			const ACreatureBase* Creature = Boss->GetCreature();
			const UHealthComponent* Health = Boss->GetBossHealth();
			if (!Creature || !Health || Creature->IsDead())
			{
				continue;
			}
			// As the player's kill, so it counts as one (experience, loot, a mission's objective). A spell can't save it.
			Boss->EndUntargetable();
			UGameplayStatics::ApplyDamage(Boss->GetOwner(), Health->GetMaxHealth() * 10.f, Controller, Pawn, UDamageType::StaticClass());
			++Killed;
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Boss.Kill: %d boss%s killed."), Killed, Killed == 1 ? TEXT("") : TEXT("es"));
	}

	FAutoConsoleCommandWithWorldAndArgs BossTestCommand(
		TEXT("Looter.Boss.Test"),
		TEXT("Looter.Boss.Test [phases 1-5, 3 by default]: spawns the test boss a few metres ahead and starts its fight: a big ")
		TEXT("Boss-rank spider, untargetable at 50% while three spiderlings live, spitting pellet volleys from 25%, inside a 15 m ")
		TEXT("fog-wall ring. A second one replaces the first; it never comes back once killed."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BossTest));

	FAutoConsoleCommandWithWorldAndArgs BossResetCommand(
		TEXT("Looter.Boss.Reset"),
		TEXT("Looter.Boss.Reset: starts every boss fight over, as the player's death does (the boss heals and goes home, its adds ")
		TEXT("go, its wall drops)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BossReset));

	FAutoConsoleCommandWithWorldAndArgs BossKillCommand(
		TEXT("Looter.Boss.Kill"),
		TEXT("Looter.Boss.Kill: kills every living boss as the player's kill (its loot drops, the fight is won, its wall drops)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BossKill));
}

#endif
