// Developer console commands for loot chests (AChest: Ransom's Rest's Ranger caches and the gang's Strongbox, step 26):
// open the nearest, shut them all again, list them (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Loot/Chest.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
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

	bool HasWord(const TArray<FString>& Args, const TCHAR* Word)
	{
		return Args.ContainsByPredicate([Word](const FString& Arg) { return Arg.Equals(Word, ESearchCase::IgnoreCase); });
	}

	APawn* FindPlayer(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/** Where distances are measured from: the player, else the world's origin. */
	FVector PlayerSpot(UWorld* World)
	{
		const APawn* Player = FindPlayer(World);
		return Player ? Player->GetActorLocation() : FVector::ZeroVector;
	}

	TArray<AChest*> FindChests(UWorld* World)
	{
		TArray<AChest*> Chests;
		for (TActorIterator<AChest> It(World); It; ++It)
		{
			Chests.Add(*It);
		}
		return Chests;
	}

	/** Nearest to From first. */
	void SortByDistance(TArray<AChest*>& Chests, const FVector& From)
	{
		Chests.Sort([&From](const AChest& A, const AChest& B)
		{
			return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From);
		});
	}

	const TCHAR* StateWord(EChestState State)
	{
		switch (State)
		{
		case EChestState::Unlocking: return TEXT("unlocking");
		case EChestState::Opening: return TEXT("opening");
		case EChestState::Open: return TEXT("open");
		default: return TEXT("closed");
		}
	}

	FString Describe(const AChest& Chest, const FVector& From)
	{
		const FChestKindInfo Info = Chest.GetKindInfo();
		const FVector At = Chest.GetActorLocation();
		return FString::Printf(TEXT("%s (%s, %s): %s%s, %d gun(s) at Luck %.1f and %d ammo pickup(s); at (%.0f, %.0f, %.0f), %.0f m away%s"),
			*Chest.GetSaveKey().ToString(), *Chest.GetActorNameOrLabel(), *UEnum::GetDisplayValueAsText(Chest.Kind).ToString(),
			StateWord(Chest.GetState()), Chest.HasGivenLoot() ? TEXT(", its loot given") : TEXT(""), Info.Guns, Info.Luck,
			Info.AmmoPickups, At.X, At.Y, At.Z, FVector::Dist(At, From) / 100.0,
			Chest.OpenWhen.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("; opens %s"), *Chest.OpenWhen.Describe()));
	}

	/**
	 * Looter.Chest.Open [all]: opens the nearest closed chest as a tap of Interact does (the missions told), whatever its
	 * story says; "all" opens every closed chest in the level.
	 */
	void OpenChests(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Chest.Open");
		UWorld* GameWorld = FindGameWorld(World);
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no level is being played."), Command);
			return;
		}
		const FVector From = PlayerSpot(GameWorld);
		TArray<AChest*> Closed = FindChests(GameWorld).FilterByPredicate([](const AChest* Chest)
		{
			return Chest->GetState() == EChestState::Closed;
		});
		if (Closed.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no closed chest in this level (Looter.Chest.Reset shuts them again)."), Command);
			return;
		}
		SortByDistance(Closed, From);
		if (!HasWord(Args, TEXT("all")))
		{
			Closed.SetNum(1);
		}
		APawn* Player = FindPlayer(GameWorld);
		UMissionRunner* Runner = UMissionRunner::Get(GameWorld);
		for (AChest* Chest : Closed)
		{
			if (Chest->Open(Player, /*bForce*/ true) && Runner)
			{
				// The missions hear of it as they would from the player's tap.
				Runner->NotifyEvent(FMissionEvent::Interaction(Chest, /*bHeld*/ false));
			}
			UE_LOG(LogLooter, Log, TEXT("%s: %s"), Command, *Describe(*Chest, From));
		}
	}

	/** Looter.Chest.Reset: shuts every chest in the level and fills it again (what they gave stays where it lies). */
	void ResetChests(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Chest.Reset");
		UWorld* GameWorld = FindGameWorld(World);
		const TArray<AChest*> Chests = GameWorld ? FindChests(GameWorld) : TArray<AChest*>();
		if (Chests.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no chests in the level being played."), Command);
			return;
		}
		for (AChest* Chest : Chests)
		{
			Chest->CloseAgain();
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %d chest(s) shut and full again (a save keeps them so)."), Command, Chests.Num());
	}

	/** Looter.Chest.List: every chest in the level, nearest first: its id, kind, state, what it gives and where it is. */
	void ListChests(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Chest.List");
		UWorld* GameWorld = FindGameWorld(World);
		TArray<AChest*> Chests = GameWorld ? FindChests(GameWorld) : TArray<AChest*>();
		if (Chests.IsEmpty())
		{
			UE_LOG(LogLooter, Log, TEXT("%s: no chests in the level being played."), Command);
			return;
		}
		const FVector From = PlayerSpot(GameWorld);
		SortByDistance(Chests, From);
		int32 Opened = 0;
		for (const AChest* Chest : Chests)
		{
			Opened += Chest->HasGivenLoot() ? 1 : 0;
			UE_LOG(LogLooter, Log, TEXT("%s: %s"), Command, *Describe(*Chest, From));
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %d chest(s), %d opened."), Command, Chests.Num(), Opened);
	}

	FAutoConsoleCommandWithWorldAndArgs OpenCommand(
		TEXT("Looter.Chest.Open"),
		TEXT("Looter.Chest.Open [all]: opens the nearest closed chest as a tap of Interact does (the missions told), whatever its ")
		TEXT("story says; 'all' opens every closed chest in the level."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&OpenChests));

	FAutoConsoleCommandWithWorldAndArgs ResetCommand(
		TEXT("Looter.Chest.Reset"),
		TEXT("Looter.Chest.Reset: shuts every chest in the level and fills it again (what they gave stays where it lies)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ResetChests));

	FAutoConsoleCommandWithWorldAndArgs ListCommand(
		TEXT("Looter.Chest.List"),
		TEXT("Looter.Chest.List: every chest in the level, nearest first: its id, kind, state, what it gives and where it is."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListChests));
}

#endif
