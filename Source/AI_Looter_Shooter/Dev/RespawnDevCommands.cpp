// Developer console commands for respawn graves (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Areas/AreaLandings.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "World/RespawnMarker.h"
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

	APawn* FindPlayerPawn(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/** The story being played here (the session's campaign record); logged when there's none. */
	FCampaignRecord* FindCampaign(UWorld* GameWorld, const TCHAR* Command)
	{
		UMissionRunner* Runner = GameWorld ? GameWorld->GetSubsystem<UMissionRunner>() : nullptr;
		if (!Runner || !Runner->IsActive())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no story here (start the game first; the main menu has none)."), Command);
			return nullptr;
		}
		return &Runner->GetCampaign();
	}

	/** Where a wake spot is, for people. */
	FString DescribeWake(const FRespawnWakeSpot& Spot)
	{
		if (Spot.Grave)
		{
			return FString::Printf(TEXT("at the grave %s"), *Spot.Grave->GetMarkerId().ToString());
		}
		return Spot.Start ? FString(TEXT("at the level's start")) : FString(TEXT("where the player fell (no grave open, no start)"));
	}

	/** Looter.Respawn.List: every grave in the level, open or not and why, how far, and where a death here wakes. */
	void ListGraves(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Respawn.List");
		UWorld* GameWorld = FindGameWorld(World);
		const FCampaignRecord* Campaign = FindCampaign(GameWorld, Command);
		if (!Campaign)
		{
			return;
		}
		const APawn* Pawn = FindPlayerPawn(GameWorld);
		const FVector Here = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
		const FRespawnWakeSpot Spot = ARespawnMarker::ChooseWakeSpot(GameWorld, Here, Campaign);
		int32 Count = 0;
		for (TActorIterator<ARespawnMarker> It(GameWorld); It; ++It)
		{
			const ARespawnMarker* Grave = *It;
			UE_LOG(LogLooter, Log, TEXT("  %-16s %-36s %6.0f m%s%s"), *Grave->GetMarkerId().ToString(), *Grave->DescribeState(*Campaign),
				FVector::Dist(Grave->GetActorLocation(), Here) / 100.0,
				AreaLandings::IsLanding(Grave) ? TEXT("  (a trip's landing: never woken at)") : TEXT(""),
				Spot.Grave == Grave ? TEXT("  <- a death here wakes here") : TEXT(""));
			++Count;
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %d graves in this level; opened in the record [%s]; a death here wakes %s."), Command, Count,
			*FString::JoinBy(Campaign->ActiveRespawns, TEXT(", "), [](const FName& Id) { return Id.ToString(); }), *DescribeWake(Spot));
	}

	/**
	 * Looter.Respawn.Activate <id | all>: opens graves in the session's campaign record, saved with it. An id that no grave
	 * in this level has is recorded all the same: that grave is open when its level is played.
	 */
	void ActivateGraves(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Respawn.Activate");
		UWorld* GameWorld = FindGameWorld(World);
		FCampaignRecord* Campaign = FindCampaign(GameWorld, Command);
		if (!Campaign)
		{
			return;
		}
		if (Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: %s <grave id | all> (Looter.Respawn.List shows the ids)"), Command);
			return;
		}
		const bool bAll = Args[0].Equals(TEXT("all"), ESearchCase::IgnoreCase);
		int32 Opened = 0;
		int32 Matched = 0;
		for (TActorIterator<ARespawnMarker> It(GameWorld); It; ++It)
		{
			const ARespawnMarker* Grave = *It;
			if (!bAll && !Grave->GetMarkerId().ToString().Equals(Args[0], ESearchCase::IgnoreCase))
			{
				continue;
			}
			++Matched;
			const bool bNew = Grave->Activate(*Campaign);
			Opened += bNew ? 1 : 0;
			UE_LOG(LogLooter, Log, TEXT("%s: %s %s."), Command, *Grave->GetMarkerId().ToString(), bNew ? TEXT("opened") : TEXT("was recorded open already"));
		}
		if (!bAll && Matched == 0)
		{
			const bool bNew = Campaign->ActivateRespawn(FName(*Args[0]));
			Opened += bNew ? 1 : 0;
			UE_LOG(LogLooter, Log, TEXT("%s: no grave here is called %s; %s for whichever level has it."), Command, *Args[0],
				bNew ? TEXT("recorded open") : TEXT("it was recorded open already"));
		}
		if (Opened > 0)
		{
			if (USessionSubsystem* Sessions = USessionSubsystem::Get(GameWorld))
			{
				Sessions->SaveSoon();
			}
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %d opened."), Command, Opened);
	}

	/**
	 * Looter.Respawn.Place [closed]: a test grave (not saved in the level) where the player stands, facing the way they
	 * look; open unless 'closed'.
	 */
	void PlaceGrave(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Respawn.Place");
		UWorld* GameWorld = FindGameWorld(World);
		const APawn* Pawn = FindPlayerPawn(GameWorld);
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no player (start the game first)."), Command);
			return;
		}
		// Each its own id, so the record never mixes two of them up.
		static int32 PlacedCount = 0;
		const FVector Feet = Pawn->GetActorLocation() - FVector(0.0, 0.0, Pawn->GetSimpleCollisionHalfHeight());
		const FTransform Where(FRotator(0.0, Pawn->GetViewRotation().Yaw, 0.0), Feet);
		ARespawnMarker* Grave = GameWorld->SpawnActorDeferred<ARespawnMarker>(ARespawnMarker::StaticClass(), Where);
		if (!Grave)
		{
			return;
		}
		Grave->MarkerId = FName(*FString::Printf(TEXT("DevGrave%d"), ++PlacedCount));
		Grave->bStartActive = !(Args.Num() > 0 && Args[0].Equals(TEXT("closed"), ESearchCase::IgnoreCase));
		Grave->FinishSpawning(Where);
		UE_LOG(LogLooter, Log, TEXT("%s: %s placed where the player stands (%s)."), Command, *Grave->GetMarkerId().ToString(),
			Grave->bStartActive ? TEXT("open") : TEXT("closed: Looter.Respawn.Activate opens it"));
	}

	/** Looter.Respawn.Die: the player dies where they stand, to try waking at the nearest open grave. */
	void KillPlayer(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Respawn.Die");
		UWorld* GameWorld = FindGameWorld(World);
		APawn* Pawn = FindPlayerPawn(GameWorld);
		const UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
		if (!Health || Health->IsDead())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no living player (start the game first)."), Command);
			return;
		}
		const FCampaignRecord* Campaign = FindCampaign(GameWorld, Command);
		const FRespawnWakeSpot Spot = ARespawnMarker::ChooseWakeSpot(GameWorld, Pawn->GetActorLocation(), Campaign);
		// Through the engine's damage, as a hit kills: everything that hears of a death does (the fade, a boss's reset).
		UGameplayStatics::ApplyDamage(Pawn, Health->GetHealth() + Health->GetMaxHealth(), nullptr, nullptr, UDamageType::StaticClass());
		if (Health->IsDead())
		{
			UE_LOG(LogLooter, Log, TEXT("%s: the player died; they wake %s."), Command, *DescribeWake(Spot));
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: the player can't be hurt now (invulnerable, or no game mode lets damage through)."), Command);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ListGravesCommand(
		TEXT("Looter.Respawn.List"),
		TEXT("Looter.Respawn.List: every respawn grave in the level, open or closed and why, how far away, and where a death here wakes."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListGraves));

	FAutoConsoleCommandWithWorldAndArgs ActivateGravesCommand(
		TEXT("Looter.Respawn.Activate"),
		TEXT("Looter.Respawn.Activate <grave id | all>: opens respawn graves in the session's campaign record (saved with it)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ActivateGraves));

	FAutoConsoleCommandWithWorldAndArgs PlaceGraveCommand(
		TEXT("Looter.Respawn.Place"),
		TEXT("Looter.Respawn.Place [closed]: a test respawn grave (not saved) where the player stands, facing the way they look; open ")
		TEXT("unless 'closed'."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PlaceGrave));

	FAutoConsoleCommandWithWorldAndArgs KillPlayerCommand(
		TEXT("Looter.Respawn.Die"),
		TEXT("Looter.Respawn.Die: the player dies where they stand, to try waking at the nearest open grave."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&KillPlayer));
}

#endif
