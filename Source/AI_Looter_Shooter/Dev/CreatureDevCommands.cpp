// Developer console commands for creatures (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

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
}

#endif
