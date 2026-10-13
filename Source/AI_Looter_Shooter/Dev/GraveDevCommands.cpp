// Developer console commands for the respawn graves as save points (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "World/GraveMarkRules.h"
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
	UMissionRunner* FindRunner(UWorld* GameWorld, const TCHAR* Command)
	{
		UMissionRunner* Runner = GameWorld ? GameWorld->GetSubsystem<UMissionRunner>() : nullptr;
		if (!Runner || !Runner->IsActive())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no story here (start the game first; the main menu has none)."), Command);
			return nullptr;
		}
		return Runner;
	}

	/** Looter.Grave.List: every grave in the level, open or not, marked or not, the save point, how far, and what a walk up to it does now. */
	void ListGraves(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Grave.List");
		UWorld* GameWorld = FindGameWorld(World);
		const UMissionRunner* Runner = FindRunner(GameWorld, Command);
		if (!Runner)
		{
			return;
		}
		const FCampaignRecord& Campaign = Runner->GetCampaign();
		const FName Area = ARespawnMarker::FindAreaId(GameWorld);
		const APawn* Pawn = FindPlayerPawn(GameWorld);
		int32 Count = 0;
		for (TActorIterator<ARespawnMarker> It(GameWorld); It; ++It)
		{
			const ARespawnMarker* Grave = *It;
			const FVector Here = Pawn ? Pawn->GetActorLocation() : Grave->GetActorLocation();
			const EGraveMarkBlock Block = Pawn ? Grave->CheckMark(Pawn) : EGraveMarkBlock::NoPlayer;
			UE_LOG(LogLooter, Log, TEXT("  %-16s %-34s %-9s %s%6.0f m  walking up: %s"), *Grave->GetMarkerId().ToString(), *Grave->DescribeState(Campaign),
				Grave->IsMarked(Campaign) ? TEXT("marked") : TEXT("unmarked"), Grave->IsSavePoint(Campaign, Area) ? TEXT("SAVE POINT  ") : TEXT(""),
				FVector::Dist(Grave->GetActorLocation(), Here) / 100.0,
				Block == EGraveMarkBlock::None ? TEXT("marks it") : *GraveMarkRules::Describe(Block));
			++Count;
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %d graves in this level (area %s); the save point is %s."), Command, Count, *Area.ToString(),
			Campaign.GetLastGrave(Area).IsNone() ? TEXT("none yet") : *Campaign.GetLastGrave(Area).ToString());
	}

	/**
	 * Looter.Grave.Mark <id>: marks the grave as if the player had walked up to it, from where they stand, past its story and
	 * any wall: the lantern, the sound, the banner, the save point. 'Looter.Grave.Mark forget <id | all>' takes marks back.
	 */
	void MarkGrave(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Grave.Mark");
		UWorld* GameWorld = FindGameWorld(World);
		UMissionRunner* Runner = FindRunner(GameWorld, Command);
		if (!Runner)
		{
			return;
		}
		if (Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: %s <grave id> | %s forget <grave id | all> (Looter.Grave.List shows the ids)"), Command, Command);
			return;
		}
		const bool bForget = Args[0].Equals(TEXT("forget"), ESearchCase::IgnoreCase);
		const FString Wanted = bForget ? (Args.Num() > 1 ? Args[1] : FString(TEXT("all"))) : Args[0];
		const bool bAll = bForget && Wanted.Equals(TEXT("all"), ESearchCase::IgnoreCase);
		APawn* Pawn = FindPlayerPawn(GameWorld);
		if (!bForget && !Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no player (start the game first)."), Command);
			return;
		}
		FCampaignRecord& Campaign = Runner->GetCampaign();
		const FName Area = ARespawnMarker::FindAreaId(GameWorld);
		int32 Matched = 0;
		for (TActorIterator<ARespawnMarker> It(GameWorld); It; ++It)
		{
			ARespawnMarker* Grave = *It;
			if (!bAll && !Grave->GetMarkerId().ToString().Equals(Wanted, ESearchCase::IgnoreCase))
			{
				continue;
			}
			++Matched;
			if (bForget)
			{
				const bool bWas = GraveMarkRules::Forget(Campaign, Area, Grave->GetMarkerId());
				Grave->SetLit(false);
				UE_LOG(LogLooter, Log, TEXT("%s forget: %s %s."), Command, *Grave->GetMarkerId().ToString(), bWas ? TEXT("is unmarked") : TEXT("wasn't marked"));
				continue;
			}
			const EGraveMark Result = Grave->TryMark(Pawn, /*bForce*/ true);
			UE_LOG(LogLooter, Log, TEXT("%s: %s %s."), Command, *Grave->GetMarkerId().ToString(),
				Result == EGraveMark::First ? TEXT("marked for the first time (lantern, banner, save point)")
				: Result == EGraveMark::Again ? TEXT("marked again (the flame lifts, the save point moves back)")
				: TEXT("not marked: it is the save point already, or the player is down, or it is a trip's landing"));
		}
		if (Matched == 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no grave here is called %s (Looter.Grave.List shows the ids)."), Command, *Wanted);
		}
		else if (bForget)
		{
			if (USessionSubsystem* Sessions = USessionSubsystem::Get(GameWorld))
			{
				Sessions->SaveSoon();
			}
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ListGravesCommand(
		TEXT("Looter.Grave.List"),
		TEXT("Looter.Grave.List: every respawn grave in the level: open or closed, marked or not, the save point (where Continue wakes), how far ")
		TEXT("away, and what walking up to it does now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListGraves));

	FAutoConsoleCommandWithWorldAndArgs MarkGraveCommand(
		TEXT("Looter.Grave.Mark"),
		TEXT("Looter.Grave.Mark <grave id>: marks the grave as the save point as if the player had walked up to it (lantern, sound, banner). ")
		TEXT("Looter.Grave.Mark forget <grave id | all> takes marks back."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MarkGrave));
}

#endif
