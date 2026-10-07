// Developer console commands for Ransom's Rest's wanted posters (Side 1; not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "World/WantedPoster.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** How far ahead of the eyes a wall is looked for (cm). */
	constexpr double WallSearch = 600.0;

	/** A surface steeper than this (its normal's height) is no wall to nail paper to. */
	constexpr double WallSteepness = 0.5;

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

	/** Looter.Poster.Spawn [note]: a wanted poster (or Calder's note) on the wall the player looks at. */
	void SpawnPoster(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Poster.Spawn: no player (start the game first)."));
			return;
		}
		const bool bNote = Args.Num() > 0 && Args[0].Equals(TEXT("note"), ESearchCase::IgnoreCase);

		// The wall the crosshair is on: the eyes' line, past the player and what they carry.
		FVector Eyes;
		FRotator View;
		Controller->GetPlayerViewPoint(Eyes, View);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PosterSpawn), /*bTraceComplex*/ true, Pawn);
		TArray<AActor*> Carried;
		Pawn->GetAttachedActors(Carried, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);
		Params.AddIgnoredActors(Carried);
		FHitResult Hit;
		const bool bWall = GameWorld->LineTraceSingleByChannel(Hit, Eyes, Eyes + View.Vector() * WallSearch, ECC_Visibility, Params)
			&& FMath::Abs(Hit.ImpactNormal.Z) < WallSteepness;

		FTransform Where;
		if (bWall)
		{
			// On the surface, upright, facing out of it.
			Where = FTransform(FRotator(0.0, Hit.ImpactNormal.Rotation().Yaw, 0.0), Hit.ImpactPoint);
		}
		else
		{
			const FVector Ahead = FRotator(0.0, View.Yaw, 0.0).Vector();
			Where = FTransform((-Ahead).Rotation(), Eyes + Ahead * 200.0);
			UE_LOG(LogLooter, Warning, TEXT("Looter.Poster.Spawn: no wall within %.0f m in front; it hangs in the air two meters ahead, ")
				TEXT("where a decal has nothing to show on. Look at a wall and try again."), WallSearch / 100.0);
		}
		// Its variant before it's made, so it's made as that (its tag, cell and box).
		AWantedPoster* Poster = GameWorld->SpawnActorDeferred<AWantedPoster>(AWantedPoster::StaticClass(), Where, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Poster)
		{
			return;
		}
		Poster->Variant = bNote ? EWantedPosterVariant::CalderNote : EWantedPosterVariant::Wanted;
		Poster->FinishSpawning(Where);
		UE_LOG(LogLooter, Log, TEXT("Looter.Poster.Spawn: %s placed %s (%s; not saved)."), bNote ? TEXT("Calder's note") : TEXT("a wanted poster"),
			bWall ? *FString::Printf(TEXT("on %s"), Hit.GetActor() ? *Hit.GetActor()->GetActorNameOrLabel() : TEXT("the wall")) : TEXT("in the air"),
			*Poster->GetName());
	}

	/** Looter.Poster.TearAll [count]: tears down the level's wanted posters still up (at most count), as a held Interact would. */
	void TearAll(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Poster.TearAll: no game running (start the game first)."));
			return;
		}
		if (Args.Num() > 0 && !Args[0].IsNumeric())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: Looter.Poster.TearAll [count]"));
			return;
		}
		const int32 Most = Args.Num() > 0 ? FMath::Max(FCString::Atoi(*Args[0]), 0) : TNumericLimits<int32>::Max();
		const APlayerController* Controller = GameWorld->GetFirstPlayerController();
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		UMissionRunner* Runner = UMissionRunner::Get(GameWorld);
		int32 Torn = 0;
		for (TActorIterator<AWantedPoster> It(GameWorld); It && Torn < Most; ++It)
		{
			if (It->Tear(Pawn))
			{
				++Torn;
				// The missions hear of it as they would from the player's held Interact.
				if (Runner)
				{
					Runner->NotifyEvent(FMissionEvent::Interaction(*It, /*bHeld*/ true));
				}
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Poster.TearAll: %d torn down; %d of the level's wanted posters are down."), Torn,
			AWantedPoster::CountTorn(GameWorld));
	}

	FAutoConsoleCommandWithWorldAndArgs SpawnPosterCommand(
		TEXT("Looter.Poster.Spawn"),
		TEXT("Looter.Poster.Spawn [note]: a wanted poster (or with 'note', Calder's note) on the wall the player looks at, ")
		TEXT("to try the tear (hold Interact) and the reading (tap). Not saved."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnPoster));

	FAutoConsoleCommandWithWorldAndArgs TearAllCommand(
		TEXT("Looter.Poster.TearAll"),
		TEXT("Looter.Poster.TearAll [count]: tears down the level's wanted posters still up (at most count), as a held Interact ")
		TEXT("would: the scraps fall, Hob remarks, and the missions hear of each."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TearAll));
}

#endif
