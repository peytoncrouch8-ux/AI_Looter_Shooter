// Console commands for checking the Stone Sentinels in play (not in shipping builds; the attack cvar Looter.Sentinel.Move is
// StoneSentinelCreatureMoves.cpp's):
//   Looter.Sentinel            each sentinel: its life, its attack, its moves' readiness, open or not, its health
//   Looter.Sentinel.Ready      every sentinel's shockwave and boulder ready now
//   Looter.Sentinel.Wake       every statue wakes for the player

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/StoneSentinelCreature.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world a command is for: the one it was typed in, or the running play-in-editor session. */
	UWorld* GameWorldOf(UWorld* World)
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

	const TCHAR* PhaseName(EStoneSentinelPhase Phase)
	{
		switch (Phase)
		{
		case EStoneSentinelPhase::Dormant: return TEXT("statue");
		case EStoneSentinelPhase::Waking: return TEXT("waking");
		case EStoneSentinelPhase::Awake: return TEXT("awake");
		case EStoneSentinelPhase::Settling: return TEXT("settling");
		case EStoneSentinelPhase::Crumbling: return TEXT("crumbling");
		default: return TEXT("?");
		}
	}

	void ShowSentinels(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* Game = GameWorldOf(World);
		if (!Game)
		{
			return;
		}
		int32 Count = 0;
		for (TActorIterator<AStoneSentinelCreature> It(Game); It; ++It)
		{
			const AStoneSentinelCreature* Sentinel = *It;
			const UHealthComponent* Health = Sentinel->FindComponentByClass<UHealthComponent>();
			UE_LOG(LogLooter, Display, TEXT("%s: %s, attack %s (planned %s), shockwave in %.1f s, boulder in %.1f s, %s, health %.0f / %.0f"),
				*Sentinel->GetName(), PhaseName(Sentinel->GetPhase()), *UEnum::GetValueAsString(Sentinel->GetMove()),
				*UEnum::GetValueAsString(Sentinel->PlanMove()), Sentinel->GetShockReadyIn(), Sentinel->GetBoulderReadyIn(),
				Sentinel->IsExposed() ? *FString::Printf(TEXT("open %.1f s"), Sentinel->GetExposedLeft()) : TEXT("closed"),
				Health ? Health->GetHealth() : 0.f, Health ? Health->GetMaxHealth() : 0.f);
			++Count;
		}
		UE_LOG(LogLooter, Display, TEXT("%d Stone Sentinel(s)."), Count);
	}

	void ReadySentinels(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* Game = GameWorldOf(World);
		if (!Game)
		{
			return;
		}
		for (TActorIterator<AStoneSentinelCreature> It(Game); It; ++It)
		{
			It->ReadyMoves();
		}
		UE_LOG(LogLooter, Display, TEXT("Every Stone Sentinel's shockwave and boulder are ready."));
	}

	void WakeSentinels(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* Game = GameWorldOf(World);
		const APlayerController* Player = Game ? Game->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		if (!Pawn)
		{
			return;
		}
		for (TActorIterator<AStoneSentinelCreature> It(Game); It; ++It)
		{
			It->Wake(Pawn);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs SentinelCommand(TEXT("Looter.Sentinel"),
		TEXT("Each Stone Sentinel: its life, its attack, its moves' readiness, open or not, its health."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowSentinels));
	FAutoConsoleCommandWithWorldAndArgs SentinelReadyCommand(TEXT("Looter.Sentinel.Ready"),
		TEXT("Every Stone Sentinel's shockwave and boulder ready now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ReadySentinels));
	FAutoConsoleCommandWithWorldAndArgs SentinelWakeCommand(TEXT("Looter.Sentinel.Wake"),
		TEXT("Every Stone Sentinel statue wakes for the player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WakeSentinels));
}

#endif
