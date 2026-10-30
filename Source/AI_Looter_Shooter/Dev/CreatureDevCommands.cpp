// Developer console commands for creatures: health, kill, Unpaid step check (not in shipping builds). Spawning is in CreatureDevCommandsSpawn.cpp.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/UnpaidCreature.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
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

	void SetCreatureHealth(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		TArray<float> Values;
		for (const FString& Arg : Args)
		{
			Values.Add(FMath::Max(FCString::Atof(*Arg), 1.f));
		}
		if (!Pawn || Values.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.CreatureHealth: give at least one health value, with the game running."));
			return;
		}

		// Nearest first, so the creatures in view get the values in the order they were typed.
		TArray<ACreatureBase*> Creatures;
		for (TActorIterator<ACreatureBase> It(GameWorld); It; ++It)
		{
			Creatures.Add(*It);
		}
		const FVector From = Pawn->GetActorLocation();
		Creatures.Sort([&From](const ACreatureBase& A, const ACreatureBase& B)
		{
			return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From);
		});
		for (int32 Index = 0; Index < Creatures.Num(); ++Index)
		{
			if (UHealthComponent* Health = Creatures[Index]->FindComponentByClass<UHealthComponent>())
			{
				Health->MaxHealth = Values[Index % Values.Num()];
				Health->ResetHealth();
				UE_LOG(LogLooter, Log, TEXT("Looter.CreatureHealth: %s now has %.0f health (%.0f m away)."), *Creatures[Index]->GetName(),
					Health->MaxHealth, FVector::Dist(Creatures[Index]->GetActorLocation(), From) / 100.f);
			}
		}
	}

	FAutoConsoleCommandWithWorldAndArgs CreatureHealthCommand(
		TEXT("Looter.CreatureHealth"),
		TEXT("Looter.CreatureHealth <health> [<health> ...]: sets every creature's maximum health and heals it, nearest first, ")
		TEXT("going through the values in turn (to compare their health bars). Until the level reloads."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetCreatureHealth));

	/**
	 * Looter.KillNearest [count]: kills the nearest living creatures as the player's kill, to watch deaths on cue (a shot
	 * at a charging creature often misses; Looter.Boss.Kill is only for bosses). Pairs with `slomo` for a slow look.
	 */
	void KillNearest(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.KillNearest: start the game first."));
			return;
		}
		TArray<ACreatureBase*> Living;
		for (TActorIterator<ACreatureBase> It(GameWorld); It; ++It)
		{
			if (!It->IsDead())
			{
				Living.Add(*It);
			}
		}
		const FVector From = Pawn->GetActorLocation();
		Living.Sort([&From](const ACreatureBase& A, const ACreatureBase& B)
		{
			return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From);
		});
		const int32 Count = FMath::Min(Args.Num() > 0 ? FMath::Max(FCString::Atoi(*Args[0]), 1) : 1, Living.Num());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const UHealthComponent* Health = Living[Index]->FindComponentByClass<UHealthComponent>();
			UGameplayStatics::ApplyDamage(Living[Index], (Health ? Health->GetMaxHealth() : 1000.f) * 10.f, Controller, Pawn,
				UDamageType::StaticClass());
			UE_LOG(LogLooter, Log, TEXT("Looter.KillNearest: killed %s (%.0f m away)."), *Living[Index]->GetName(),
				FVector::Dist(Living[Index]->GetActorLocation(), From) / 100.f);
		}
	}

	/** Looter.Unpaid.TryStep: the nearest Unpaid with a target says where a phase-step toward them could land, and why not. */
	void TryUnpaidStep(const TArray<FString>& Args, UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Unpaid.TryStep: no player (start the game first)."));
			return;
		}
		const AUnpaidCreature* Nearest = nullptr;
		double Best = TNumericLimits<double>::Max();
		for (TActorIterator<AUnpaidCreature> It(World); It; ++It)
		{
			const double Distance = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
			if (!It->IsDead() && It->GetTarget() && Distance < Best)
			{
				Best = Distance;
				Nearest = *It;
			}
		}
		if (!Nearest)
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Unpaid.TryStep: no Unpaid after anyone."));
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Unpaid.TryStep: %s (%s): %s"), *Nearest->GetName(),
			*UEnum::GetValueAsString(Nearest->GetCreatureState()), *Nearest->DescribePhaseRefusals());
	}

	FAutoConsoleCommandWithWorldAndArgs TryUnpaidStepCommand(
		TEXT("Looter.Unpaid.TryStep"),
		TEXT("Looter.Unpaid.TryStep: the nearest Unpaid after someone logs where a phase-step toward them could land, and why each spot is refused."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TryUnpaidStep));

	FAutoConsoleCommandWithWorldAndArgs KillNearestCommand(
		TEXT("Looter.KillNearest"),
		TEXT("Looter.KillNearest [count]: kills the nearest living creatures (1 by default) as the player's kill, to watch deaths on cue."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&KillNearest));
}

#endif
