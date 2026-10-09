// Developer console commands for a boss fight's show (not in shipping builds): a stagger now, the next phase now, and a
// camera shake to feel. The fights themselves: Looter.Boss.Test, Looter.Abel.Fight, Looter.SpawnCreature Gravemother.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Engine/Engine.h"
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
	UWorld* FindShowWorld(UWorld* World)
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

	/** Every boss whose fight is on. */
	TArray<UBossComponent*> FightingBosses(UWorld* World)
	{
		TArray<UBossComponent*> Bosses;
		if (!World)
		{
			return Bosses;
		}
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			UBossComponent* Boss = It->FindComponentByClass<UBossComponent>();
			if (Boss && Boss->IsFighting())
			{
				Bosses.Add(Boss);
			}
		}
		return Bosses;
	}

	void BossStagger(const TArray<FString>& Args, UWorld* World)
	{
		int32 Staggered = 0;
		for (UBossComponent* Boss : FightingBosses(FindShowWorld(World)))
		{
			Staggered += Boss->BeginStagger() ? 1 : 0;
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Boss.Stagger: %d boss%s staggered (none: not fighting, or it can't be now)."), Staggered,
			Staggered == 1 ? TEXT("") : TEXT("es"));
	}

	void BossNext(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindShowWorld(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		for (UBossComponent* Boss : FightingBosses(GameWorld))
		{
			const UHealthComponent* Health = Boss->GetBossHealth();
			const TArray<float> Shares = Boss->GetPhaseShares();
			const int32 Next = Boss->GetPhase() + 1;
			if (!Health || !Shares.IsValidIndex(Next))
			{
				UE_LOG(LogLooter, Log, TEXT("Looter.Boss.Next: %s is in its last phase."), *GetNameSafe(Boss->GetOwner()));
				continue;
			}
			// As the player's damage, so its own code hears the hit (the Gravemother's brood); a spell it's under stops it.
			const float Wanted = Health->GetMaxHealth() * Shares[Next] - 1.f;
			UGameplayStatics::ApplyDamage(Boss->GetOwner(), FMath::Max(Health->GetHealth() - Wanted, 0.f), Controller, Pawn, UDamageType::StaticClass());
			UE_LOG(LogLooter, Log, TEXT("Looter.Boss.Next: %s at %.0f%% (phase %d of %d)."), *GetNameSafe(Boss->GetOwner()),
				Health->GetHealthPercent() * 100.f, Boss->GetPhase() + 1, Boss->NumPhases());
		}
	}

	void CameraShake(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindShowWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		const float Strength = Args.Num() > 0 ? FMath::Clamp(FCString::Atof(*Args[0]), 0.f, 1.f) : 0.6f;
		if (Pawn)
		{
			BossCameraShake::Kick(GameWorld, Pawn->GetActorLocation(), Strength, 0.8f);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs BossStaggerCommand(
		TEXT("Looter.Boss.Stagger"),
		TEXT("Looter.Boss.Stagger: staggers every boss whose fight is on, as its weak spot's crits would (the Gravemother sinks, ")
		TEXT("Abel drops to a knee with his coal open)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BossStagger));

	FAutoConsoleCommandWithWorldAndArgs BossNextCommand(
		TEXT("Looter.Boss.Next"),
		TEXT("Looter.Boss.Next: hurts every boss whose fight is on to just under its next phase's line, as the player's damage ")
		TEXT("(its phase's tell plays; nothing while it can't be hurt: Looter.Abel.Phase for Abel's)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BossNext));

	FAutoConsoleCommandWithWorldAndArgs CameraShakeCommand(
		TEXT("Looter.Boss.Shake"),
		TEXT("Looter.Boss.Shake [strength 0-1, 0.6 by default]: the boss fights' camera shake, on the player now. ")
		TEXT("Looter.CameraShake scales every shake (0 turns them off)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CameraShake));
}

#endif
