// Developer console commands for Vane's Dredgers (not in shipping builds):
//   Looter.Dredger.Spawn <Rifleman|HoseMan> [count] [rank] [level=<n>] [hold]: Dredgers 10-14 m ahead of the player, on them.
//   Looter.Dredger.State: the nearest one's job, tactic, spot, rounds, hose and tank, logged.
//   Looter.Dredger.Latch: the nearest hose-man throws his line at the player now.
//   Looter.Dredger.Tank: the nearest hose-man's tank bursts.
//   Looter.Dredger.Reload: the nearest rifleman reloads.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/DredgerCreature.h"
#include "Creatures/DredgerHoseComponent.h"
#include "Creatures/DredgerHoseMan.h"
#include "Creatures/DredgerRifleman.h"
#include "World/WorldQueries.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world a command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* DredgerGameWorld(UWorld* World)
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

	APawn* DredgerPlayer(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/** The living Dredger of Class nearest the player. */
	template <typename TDredger>
	TDredger* NearestDredger(UWorld* World)
	{
		const APawn* Player = DredgerPlayer(World);
		TDredger* Nearest = nullptr;
		double Best = TNumericLimits<double>::Max();
		for (TActorIterator<TDredger> It(World); Player && It; ++It)
		{
			const double Distance = FVector::DistSquared(It->GetActorLocation(), Player->GetActorLocation());
			if (!It->IsDead() && Distance < Best)
			{
				Best = Distance;
				Nearest = *It;
			}
		}
		return Nearest;
	}

	void SpawnDredgers(const TArray<FString>& AllArgs, UWorld* World)
	{
		UWorld* GameWorld = DredgerGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Player = Controller ? Controller->GetPawn() : nullptr;
		if (!Player)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Dredger.Spawn: no player (start the game first)."));
			return;
		}
		TArray<FString> Args;
		int32 SpawnLevel = 0;
		bool bHold = false;
		for (const FString& Arg : AllArgs)
		{
			FString Key;
			FString Value;
			if (Arg.Split(TEXT("="), &Key, &Value) && Key.Equals(TEXT("level"), ESearchCase::IgnoreCase))
			{
				SpawnLevel = FMath::Max(FCString::Atoi(*Value), 1);
			}
			else if (Arg.Equals(TEXT("hold"), ESearchCase::IgnoreCase))
			{
				bHold = true;
			}
			else
			{
				Args.Add(Arg);
			}
		}
		const FString Job = Args.Num() > 0 ? Args[0] : FString(TEXT("Rifleman"));
		const bool bHoseMan = Job.StartsWith(TEXT("Hose"), ESearchCase::IgnoreCase);
		if (!bHoseMan && !Job.StartsWith(TEXT("Rifle"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Dredger.Spawn: no job called '%s' (Rifleman or HoseMan)."), *Job);
			return;
		}
		const int32 Count = Args.Num() > 1 ? FMath::Clamp(FCString::Atoi(*Args[1]), 1, 12) : 1;
		ECreatureRank Rank = ECreatureRank::Basic;
		if (Args.Num() > 2 && !UCreatureRankSettings::ParseRank(Args[2], Rank))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Dredger.Spawn: no rank called '%s'."), *Args[2]);
			return;
		}
		const TSubclassOf<ADredgerCreature> Class = ADredgerCreature::ClassFor(bHoseMan ? EDredgerRole::HoseMan : EDredgerRole::Rifleman);
		ACreatureBase::FRuntimeSpawn Spawn;
		Spawn.Rank = Rank;
		Spawn.Level = SpawnLevel;
		Spawn.bMayBeTouched = false;

		// Spread across where the player looks, 10-14 m out, each facing them.
		const FVector Ahead = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Ahead);
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(GameWorld, TEXT("DredgerSpawn"));
		int32 Spawned = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Across = (static_cast<float>(Index) - (Count - 1) * 0.5f) * 350.f;
			const FVector Spot = Player->GetActorLocation() + Ahead * (1000.f + 400.f * (Index % 2)) + Side * Across;
			FHitResult Hit;
			if (!GameWorld->LineTraceSingleByObjectType(Hit, Spot + FVector(0.f, 0.f, 800.f), Spot - FVector(0.f, 0.f, 3000.f),
				FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				continue;
			}
			const float Yaw = static_cast<float>((Player->GetActorLocation() - Hit.ImpactPoint).Rotation().Yaw);
			ACreatureBase* Made = ACreatureBase::SpawnAtRuntime(GameWorld, Class, Hit.ImpactPoint, Yaw, Spawn);
			if (ADredgerCreature* Dredger = Cast<ADredgerCreature>(Made))
			{
				Dredger->SetHoldsGround(bHold);
				Dredger->AlertTo(Player);
				++Spawned;
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Dredger.Spawn: %d %s%s (rank %s)."), Spawned, bHoseMan ? TEXT("hose-men") : TEXT("riflemen"),
			bHold ? TEXT(", holding their ground") : TEXT(""), *UEnum::GetValueAsString(Rank));
	}

	void LogState(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = DredgerGameWorld(World);
		const ADredgerCreature* Dredger = GameWorld ? NearestDredger<ADredgerCreature>(GameWorld) : nullptr;
		if (!Dredger)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Dredger.State: no Dredger alive."));
			return;
		}
		const UHealthComponent* Health = Dredger->FindComponentByClass<UHealthComponent>();
		FString Job;
		if (const ADredgerRifleman* Rifleman = Cast<ADredgerRifleman>(Dredger))
		{
			FVector Spot;
			bool bCovered = false;
			const bool bSpot = Rifleman->GetSpot(Spot, bCovered);
			Job = FString::Printf(TEXT("rifleman: tactic %d, spot %s%s, rounds %d%s, %d fired"), static_cast<int32>(Rifleman->GetTactic()),
				bSpot ? *Spot.ToCompactString() : TEXT("none"), bSpot && bCovered ? TEXT(" (cover)") : TEXT(""), Rifleman->GetRoundsLeft(),
				Rifleman->IsReloading() ? TEXT(" reloading") : TEXT(""), Rifleman->GetShotsFired());
		}
		else if (const ADredgerHoseMan* HoseMan = Cast<ADredgerHoseMan>(Dredger))
		{
			const UDredgerHoseComponent* Hose = HoseMan->GetHose();
			Job = FString::Printf(TEXT("hose-man: hose %d (last break %d), %.1f drained, %.1f/s; tank %.0f%s; rest %.1f s"),
				static_cast<int32>(Hose->GetState()), static_cast<int32>(Hose->GetLastBreak()), Hose->GetDrainDealt(), HoseMan->GetDrainPerSecond(),
				HoseMan->GetTankDamage(), HoseMan->IsTankBurst() ? TEXT(" (burst)") : TEXT(""), HoseMan->GetLatchRestLeft());
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Dredger.State: %s level %d, %.0f/%.0f health, state %s, lenses broken %d; %s"), *Dredger->GetName(),
			Dredger->Level, Health ? Health->GetHealth() : 0.f, Health ? Health->GetMaxHealth() : 0.f,
			*UEnum::GetValueAsString(Dredger->GetCreatureState()), Dredger->GetBrokenLenses(), *Job);
	}

	void ForceLatch(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = DredgerGameWorld(World);
		ADredgerHoseMan* HoseMan = GameWorld ? NearestDredger<ADredgerHoseMan>(GameWorld) : nullptr;
		APawn* Player = DredgerPlayer(GameWorld);
		const bool bThrown = HoseMan && Player && HoseMan->GetHose()->Fire(Player);
		UE_LOG(LogLooter, Log, TEXT("Looter.Dredger.Latch: %s"), bThrown ? TEXT("thrown") : TEXT("no hose-man near, or his line is out or torn"));
	}

	void ForceTank(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = DredgerGameWorld(World);
		ADredgerHoseMan* HoseMan = GameWorld ? NearestDredger<ADredgerHoseMan>(GameWorld) : nullptr;
		if (!HoseMan)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Dredger.Tank: no hose-man alive."));
			return;
		}
		HoseMan->BurstTank(GameWorld->GetFirstPlayerController());
		UE_LOG(LogLooter, Log, TEXT("Looter.Dredger.Tank: %s's tank burst."), *HoseMan->GetName());
	}

	void ForceReload(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = DredgerGameWorld(World);
		ADredgerRifleman* Rifleman = GameWorld ? NearestDredger<ADredgerRifleman>(GameWorld) : nullptr;
		if (Rifleman)
		{
			Rifleman->DevStartReload();
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Dredger.Reload: %s"), Rifleman ? *Rifleman->GetName() : TEXT("no rifleman alive"));
	}

	FAutoConsoleCommandWithWorldAndArgs DredgerSpawnCommand(
		TEXT("Looter.Dredger.Spawn"),
		TEXT("Looter.Dredger.Spawn <Rifleman|HoseMan> [count] [Regular|Restless|Gravebound|Hallowed|Ethereal] [level=<n>] [hold]: ")
		TEXT("Vane's Dredgers 10-14 m ahead of the player, set on them (hold: they keep their ground, as on the tender)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnDredgers));
	FAutoConsoleCommandWithWorldAndArgs DredgerStateCommand(
		TEXT("Looter.Dredger.State"),
		TEXT("Logs the nearest Dredger's job, tactic, spot, rounds, hose and tank."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LogState));
	FAutoConsoleCommandWithWorldAndArgs DredgerLatchCommand(
		TEXT("Looter.Dredger.Latch"),
		TEXT("The nearest hose-man throws his line at the player now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ForceLatch));
	FAutoConsoleCommandWithWorldAndArgs DredgerTankCommand(
		TEXT("Looter.Dredger.Tank"),
		TEXT("The nearest hose-man's tank bursts."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ForceTank));
	FAutoConsoleCommandWithWorldAndArgs DredgerReloadCommand(
		TEXT("Looter.Dredger.Reload"),
		TEXT("The nearest rifleman reloads."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ForceReload));
}

#endif
