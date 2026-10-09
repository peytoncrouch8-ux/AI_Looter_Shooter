// Developer console commands for the map page and fast travel between graves (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "UI/HUD/LooterHUD.h"
#include "World/GraveTravelSubsystem.h"
#include "World/RespawnMarker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* MapCommandWorld(UWorld* World)
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

	APawn* MapCommandPawn(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/** Looter.Map: opens the inventory on the map page. */
	void OpenMapCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = MapCommandWorld(World);
		ALooterHUD* HUD = ALooterHUD::FindFor(GameWorld ? GameWorld->GetFirstPlayerController() : nullptr);
		if (!HUD)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Map: no player with a HUD (start a level, not the menu)."));
			return;
		}
		HUD->ShowInventoryPage(EInventoryPage::Map);
	}

	/** Looter.Map.Graves: every grave in the level, open or not, and whether fast travel could go there now (and why not). */
	void ListGravesCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = MapCommandWorld(World);
		const UGraveTravelSubsystem* Travel = UGraveTravelSubsystem::Get(GameWorld);
		const APawn* Pawn = MapCommandPawn(GameWorld);
		if (!Travel)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Map.Graves: no game world."));
			return;
		}
		int32 Count = 0;
		for (TActorIterator<ARespawnMarker> It(GameWorld); It; ++It)
		{
			const ARespawnMarker& Grave = **It;
			const EGraveTravelBlock Block = Travel->CheckGrave(Pawn, Grave);
			const double Metres = Pawn ? FVector::Dist2D(Pawn->GetActorLocation(), Grave.GetActorLocation()) / 100.0 : 0.0;
			UE_LOG(LogLooter, Display, TEXT("Looter.Map.Graves: %s (\"%s\"), %s, %.0f m: %s"), *Grave.GetMarkerId().ToString(),
				*Grave.GetGraveName().ToString(), Travel->IsOpen(Grave) ? TEXT("open") : TEXT("closed"), Metres,
				Block == EGraveTravelBlock::None ? TEXT("can travel") : *GraveTravelRules::Reason(Block).ToString());
			++Count;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Map.Graves: %d graves; travel now: %s"), Count,
			Travel->CheckNow(Pawn) == EGraveTravelBlock::None ? TEXT("allowed") : *GraveTravelRules::Reason(Travel->CheckNow(Pawn)).ToString());
	}

	/** Looter.Map.Travel <grave id>: fast travel to a grave as the map's Travel does, by the same rules. */
	void TravelCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = MapCommandWorld(World);
		UGraveTravelSubsystem* Travel = UGraveTravelSubsystem::Get(GameWorld);
		APawn* Pawn = MapCommandPawn(GameWorld);
		if (!Travel || !Pawn || Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Map.Travel <grave id>: needs a player in a level and a grave (Looter.Map.Graves lists them)."));
			return;
		}
		for (TActorIterator<ARespawnMarker> It(GameWorld); It; ++It)
		{
			if (It->GetMarkerId().ToString().Equals(Args[0], ESearchCase::IgnoreCase))
			{
				EGraveTravelBlock Block = EGraveTravelBlock::None;
				if (!Travel->TravelTo(Pawn, **It, &Block))
				{
					UE_LOG(LogLooter, Warning, TEXT("Looter.Map.Travel: not now: %s"), *GraveTravelRules::Reason(Block).ToString());
				}
				return;
			}
		}
		UE_LOG(LogLooter, Warning, TEXT("Looter.Map.Travel: no grave %s here (Looter.Map.Graves lists them)."), *Args[0]);
	}

	FAutoConsoleCommandWithWorldAndArgs OpenMapRegistration(
		TEXT("Looter.Map"),
		TEXT("Looter.Map: opens the inventory on the map page."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&OpenMapCommand));

	FAutoConsoleCommandWithWorldAndArgs MapGravesRegistration(
		TEXT("Looter.Map.Graves"),
		TEXT("Looter.Map.Graves: every respawn grave here, open or not, and whether fast travel could go there now (or why not)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListGravesCommand));

	FAutoConsoleCommandWithWorldAndArgs MapTravelRegistration(
		TEXT("Looter.Map.Travel"),
		TEXT("Looter.Map.Travel <grave id>: fast travel to a grave as the map's Travel does (never in a fight, a scene or a boss fight)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TravelCommand));
}

#endif
