// The finds' console commands (not in shipping builds; FINDS.md §3): Looter.Finds.List, .Mark <id> [silent], .Forget <id|all>,
// .Goto <id>, .Tells 0/1.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Loot/Chest.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/FindRules.h"
#include "World/FindSpot.h"
#include "World/FindSubsystem.h"
#include "World/ReadableProp.h"
#include "World/WorldQueries.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** How far off a find Goto sets the player (cm): outside an ambush's 3.5 m ground, near enough to see it. */
	constexpr double GotoDistance = 600.0;

	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindsWorld(UWorld* World)
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
		return World;
	}

	UFindSubsystem* FindsIn(UWorld* World, const TCHAR* Command)
	{
		UFindSubsystem* Finds = UFindSubsystem::Get(FindsWorld(World));
		if (!Finds)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no finds here (play a level first)."), Command);
		}
		return Finds;
	}

	const TCHAR* KindWord(EFindKind Kind)
	{
		switch (Kind)
		{
		case EFindKind::Cache: return TEXT("Cache");
		case EFindKind::Hidden: return TEXT("Hidden");
		case EFindKind::Grave: return TEXT("Grave");
		case EFindKind::Elite: return TEXT("Elite");
		case EFindKind::Lore: return TEXT("Lore");
		}
		return TEXT("?");
	}

	/** What its source is doing: a chest's state, a readable read or not, an elite's tag, or missing. */
	FString SourceState(const AFindSpot& Spot, const FCampaignRecord* Campaign)
	{
		const AActor* Source = Spot.GetSourceActor();
		if (const AChest* Chest = Cast<AChest>(Source))
		{
			return FString::Printf(TEXT("chest %s %s"), *Chest->GetSaveKey().ToString(),
				Chest->HasGivenLoot() ? TEXT("looted") : (Chest->IsUsedUp() ? TEXT("opening") : TEXT("closed")));
		}
		if (const AReadableProp* Readable = Cast<AReadableProp>(Source))
		{
			const bool bRead = Campaign && !Readable->SetsFlag.IsNone() && Campaign->HasFlag(Readable->SetsFlag);
			return FString::Printf(TEXT("readable %s%s"), *Readable->GetReadEvent().ToString(), bRead ? TEXT(" (its flag set)") : TEXT(""));
		}
		if (Source)
		{
			return FString::Printf(TEXT("actor %s"), *Source->GetName());
		}
		if (!Spot.SourceTag.IsNone())
		{
			return FString::Printf(TEXT("creature tagged %s"), *Spot.SourceTag.ToString());
		}
		return Spot.SourceEvent.IsNone() ? TEXT("NO SOURCE") : FString::Printf(TEXT("event %s"), *Spot.SourceEvent.ToString());
	}

	/** Looter.Finds.List: every find here, its region, kind, found or not and its source's state; then the tallies. */
	void ListCommand(const TArray<FString>& Args, UWorld* World)
	{
		const UFindSubsystem* Finds = FindsIn(World, TEXT("Looter.Finds.List"));
		if (!Finds)
		{
			return;
		}
		const UMissionRunner* Runner = UMissionRunner::Get(FindsWorld(World));
		const FCampaignRecord* Campaign = Runner ? &Runner->GetCampaign() : nullptr;
		for (const AFindSpot* Spot : Finds->GetSpots())
		{
			UE_LOG(LogLooter, Display, TEXT("  %-14s %-17s %-6s %-5s %s"), *Spot->FindId.ToString(), *Spot->Region.ToString(), KindWord(Spot->Kind),
				Finds->IsFound(Spot->FindId) ? TEXT("FOUND") : TEXT("-"), *SourceState(*Spot, Campaign));
		}
		const FFindTally Tally = Finds->GetTally();
		const FFindTally Lore = Finds->GetLoreTally();
		UE_LOG(LogLooter, Display, TEXT("Looter.Finds.List: finds %d / %d, ledger %d / %d, tells %s."), Tally.Found, Tally.Total, Lore.Found,
			Lore.Total, Finds->AreTellsEnabled() ? TEXT("on") : TEXT("off"));
		for (const FFindRegionCount& Row : Tally.ByRegion)
		{
			UE_LOG(LogLooter, Display, TEXT("  %s %d / %d%s"), *Row.Region.ToString(), Row.Found, Row.Total, Row.IsDone() ? TEXT(" (done)") : TEXT(""));
		}
	}

	/** Looter.Finds.Mark <id> [silent]: found now, with its moment (banner, rewards, XP), or silently. */
	void MarkCommand(const TArray<FString>& Args, UWorld* World)
	{
		UFindSubsystem* Finds = FindsIn(World, TEXT("Looter.Finds.Mark"));
		if (!Finds || Args.IsEmpty())
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Finds.Mark <id> [silent]"));
			return;
		}
		const bool bSilent = Args.Num() > 1 && Args[1].Equals(TEXT("silent"), ESearchCase::IgnoreCase);
		const bool bDone = Finds->MarkFound(FName(*Args[0]), !bSilent);
		UE_LOG(LogLooter, Display, TEXT("Looter.Finds.Mark: %s %s."), *Args[0], bDone ? TEXT("found") : TEXT("is no find here, or found already"));
	}

	/** Looter.Finds.Forget <id|all>: unfound again (its tells back; its chest stays as it is). */
	void ForgetCommand(const TArray<FString>& Args, UWorld* World)
	{
		UFindSubsystem* Finds = FindsIn(World, TEXT("Looter.Finds.Forget"));
		if (!Finds || Args.IsEmpty())
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Finds.Forget <id|all>"));
			return;
		}
		if (Args[0].Equals(TEXT("all"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Finds.Forget: %d forgotten."), Finds->ForgetAll());
			return;
		}
		const bool bDone = Finds->Forget(FName(*Args[0]));
		UE_LOG(LogLooter, Display, TEXT("Looter.Finds.Forget: %s %s."), *Args[0], bDone ? TEXT("forgotten") : TEXT("wasn't found"));
	}

	/**
	 * Looter.Finds.Goto <id>: the player set down GotoDistance off the find on the side they come from (the first heading round
	 * it with ground under it), facing it. Never onto it: 6 m is outside an ambush's ground (the miser's plot springs at 3.5 m).
	 */
	void GotoCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = FindsWorld(World);
		UFindSubsystem* Finds = FindsIn(World, TEXT("Looter.Finds.Goto"));
		const AFindSpot* Spot = Finds && !Args.IsEmpty() ? Finds->FindSpot(FName(*Args[0])) : nullptr;
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Spot || !Pawn)
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Finds.Goto <id>: %s."), Spot ? TEXT("no player") : TEXT("no such find here"));
			return;
		}
		const FVector Target = Spot->GetActorLocation();
		FVector Toward = Pawn->GetActorLocation() - Target;
		Toward.Z = 0.0;
		Toward = Toward.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("FindGoto"), Pawn);
		for (int32 Turn = 0; Turn < 8; ++Turn)
		{
			const FVector Heading = Toward.RotateAngleAxis(45.0 * Turn, FVector::UpVector);
			const FVector Spot2D = Target + Heading * GotoDistance;
			FHitResult Hit;
			if (!World->LineTraceSingleByObjectType(Hit, Spot2D + FVector(0.0, 0.0, 800.0), Spot2D - FVector(0.0, 0.0, 2000.0),
				FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				continue;
			}
			const float HalfHeight = static_cast<float>(Pawn->GetRootComponent()->Bounds.BoxExtent.Z);
			Pawn->SetActorLocation(Hit.ImpactPoint + FVector(0.0, 0.0, HalfHeight + 5.0), false, nullptr, ETeleportType::TeleportPhysics);
			const FVector Look = Target - Pawn->GetActorLocation();
			Controller->SetControlRotation(FRotator(static_cast<float>(FMath::Clamp(Look.Rotation().Pitch, -30.0, 20.0)),
				static_cast<float>(Look.Rotation().Yaw), 0.f));
			UE_LOG(LogLooter, Display, TEXT("Looter.Finds.Goto: by %s (%s)."), *Spot->FindId.ToString(), *Spot->ToEntry().Name.ToString());
			return;
		}
		UE_LOG(LogLooter, Warning, TEXT("Looter.Finds.Goto: no ground round %s."), *Spot->FindId.ToString());
	}

	/** Looter.Finds.Tells 0/1: the crows and glints over unfound finds off or on (no argument: says which). */
	void TellsCommand(const TArray<FString>& Args, UWorld* World)
	{
		UFindSubsystem* Finds = FindsIn(World, TEXT("Looter.Finds.Tells"));
		if (!Finds)
		{
			return;
		}
		if (!Args.IsEmpty())
		{
			Finds->SetTellsEnabled(FCString::Atoi(*Args[0]) != 0);
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Finds.Tells %d"), Finds->AreTellsEnabled() ? 1 : 0);
	}

	FAutoConsoleCommandWithWorldAndArgs FindsListCommand(TEXT("Looter.Finds.List"),
		TEXT("Every find here: id, region, kind, found, its source's state; the finds' and the ledger's counts."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListCommand));
	FAutoConsoleCommandWithWorldAndArgs FindsMarkCommand(TEXT("Looter.Finds.Mark"),
		TEXT("<id> [silent]: the find found now, with its banner, rewards and XP (silent: only counted)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MarkCommand));
	FAutoConsoleCommandWithWorldAndArgs FindsForgetCommand(TEXT("Looter.Finds.Forget"),
		TEXT("<id|all>: unfound again, its tells back (an opened chest stays open)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ForgetCommand));
	FAutoConsoleCommandWithWorldAndArgs FindsGotoCommand(TEXT("Looter.Finds.Goto"),
		TEXT("<id>: the player set down 6 m off the find, facing it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GotoCommand));
	FAutoConsoleCommandWithWorldAndArgs FindsTellsCommand(TEXT("Looter.Finds.Tells"),
		TEXT("0/1: the circling crows and glints over unfound finds off or on."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TellsCommand));
}

#endif
