// Developer console commands for Abel's fight, Main 6 "The Gravewind" (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Scenes/ColdOpen.h"
#include "Scenes/SitWithPa.h"
#include "World/KeeperLanternPost.h"
#include "World/LightingStateSubsystem.h"
#include "World/WorldQueries.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
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

	/** The level's Abel, or null (with a warning naming the command). */
	AAbelKeeper* FindAbel(UWorld* World, const TCHAR* Command)
	{
		if (World)
		{
			for (TActorIterator<AAbelKeeper> It(World); It; ++It)
			{
				return *It;
			}
		}
		UE_LOG(LogLooter, Warning, TEXT("%s: no Abel in this level (Ransom's Rest's deck: build_area_deck.py places him; start the game first)."), Command);
		return nullptr;
	}

	APawn* PlayerPawn(UWorld* World, APlayerController** OutController = nullptr)
	{
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		if (OutController)
		{
			*OutController = Controller;
		}
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/** Puts the player on the deck Back cm from his spot toward the gate, facing him. */
	void PutOnDeck(UWorld* World, AAbelKeeper& Abel, float Back)
	{
		APlayerController* Controller = nullptr;
		APawn* Pawn = PlayerPawn(World, &Controller);
		if (!Pawn)
		{
			return;
		}
		const FVector Spot = Abel.GetHome().GetLocation() - Abel.GetSunsetDirection() * Back;
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("AbelDeck"), Pawn);
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Spot + FVector(0.0, 0.0, 600.0), Spot - FVector(0.0, 0.0, 1500.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			const float HalfHeight = static_cast<float>(Pawn->GetRootComponent()->Bounds.BoxExtent.Z);
			Pawn->SetActorLocation(Hit.ImpactPoint + FVector(0.0, 0.0, HalfHeight + 5.0), false, nullptr, ETeleportType::TeleportPhysics);
			Controller->SetControlRotation(FRotator(-4.f, static_cast<float>(Abel.GetSunsetDirection().Rotation().Yaw), 0.f));
		}
	}

	/** Dusk at once, if the level has it. */
	void DuskNow(UWorld* World)
	{
		ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(World);
		if (Lighting && Lighting->GetStateNames().Contains(ColdOpen::DuskState()))
		{
			Lighting->SetState(ColdOpen::DuskState(), ELightingSwitch::Instant);
		}
	}

	/** Present, at dusk, the player on the deck, and the fight on; false without him or a player. */
	bool StartFightNow(UWorld* World, AAbelKeeper& Abel, bool bPutPlayer)
	{
		APawn* Pawn = PlayerPawn(World);
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Abel: no player (start the game first)."));
			return false;
		}
		Abel.ForcePresent(true);
		DuskNow(World);
		if (bPutPlayer)
		{
			PutOnDeck(World, Abel, 800.f);
		}
		if (!Abel.GetBoss()->IsFighting())
		{
			Abel.GetBoss()->StartFight(Pawn);
		}
		return Abel.GetBoss()->IsFighting();
	}

	void AbelFight(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		AAbelKeeper* Abel = FindAbel(GameWorld, TEXT("Looter.Abel.Fight"));
		if (Abel && StartFightNow(GameWorld, *Abel, !(Args.Num() > 0 && Args[0].Equals(TEXT("here"), ESearchCase::IgnoreCase))))
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Abel.Fight: the fight is on (%.0f health, level %d)."), Abel->GetBoss()->GetBossHealth()->GetMaxHealth(), Abel->Level);
		}
	}

	void AbelPhase(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		AAbelKeeper* Abel = FindAbel(GameWorld, TEXT("Looter.Abel.Phase"));
		const int32 Phase = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1, 3) : 2;
		if (!Abel || !StartFightNow(GameWorld, *Abel, false))
		{
			return;
		}
		const float Share = Phase == 1 ? 1.f : Phase == 2 ? AbelRules::BellShare : AbelRules::WindShare;
		UHealthComponent* Health = Abel->GetBoss()->GetBossHealth();
		// A little under the phase's line: its events start at once.
		Health->SetHealth(FMath::Max(1.f, Health->GetMaxHealth() * Share - (Phase == 1 ? 0.f : 1.f)));
		if (Phase == 3)
		{
			// Out of the fog first: the lanterns relit, as the player would.
			for (AKeeperLanternPost* Post : Abel->GetLanternPosts())
			{
				if (Post)
				{
					Post->Relight(nullptr);
				}
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Abel.Phase %d: at %.0f%% health."), Phase, Health->GetHealthPercent() * 100.f);
	}

	void AbelKneel(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		AAbelKeeper* Abel = FindAbel(GameWorld, TEXT("Looter.Abel.Kneel"));
		APlayerController* Controller = nullptr;
		APawn* Pawn = PlayerPawn(GameWorld, &Controller);
		if (!Abel || !StartFightNow(GameWorld, *Abel, false))
		{
			return;
		}
		// As the player's kill, so it counts (experience, his loot, Main 6's step); no spell saves him.
		Abel->GetBoss()->EndUntargetable();
		UHealthComponent* Health = Abel->GetBoss()->GetBossHealth();
		UGameplayStatics::ApplyDamage(Abel, Health->GetMaxHealth() * 10.f, Controller, Pawn, UDamageType::StaticClass());
		UE_LOG(LogLooter, Log, TEXT("Looter.Abel.Kneel: he kneels; the scene follows."));
	}

	void AbelScene(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		AAbelKeeper* Abel = FindAbel(GameWorld, TEXT("Looter.Abel.Scene"));
		if (!Abel)
		{
			return;
		}
		Abel->ForcePresent(true);
		Abel->SetPose(EAbelPose::Kneel, 0.f);
		const bool bPlays = SitWithPa::Play(*Abel);
		UE_LOG(LogLooter, Log, TEXT("Looter.Abel.Scene: %s (Looter.Abel.Back puts him on the boards again)."),
			bPlays ? TEXT("the scene plays with him where he stands") : TEXT("no scene could play (scenes off, or one is playing)"));
	}

	void AbelBack(const TArray<FString>& Args, UWorld* World)
	{
		AAbelKeeper* Abel = FindAbel(FindGameWorld(World), TEXT("Looter.Abel.Back"));
		if (Abel)
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Abel.Back: %s"), Abel->ComeBack() ? TEXT("he walks the boards again.")
				: TEXT("he's been beaten in this level; load the session again to fight him."));
		}
	}

	void AbelReset(const TArray<FString>& Args, UWorld* World)
	{
		AAbelKeeper* Abel = FindAbel(FindGameWorld(World), TEXT("Looter.Abel.Reset"));
		if (Abel)
		{
			Abel->GetBoss()->ResetFight();
			UE_LOG(LogLooter, Log, TEXT("Looter.Abel.Reset: the fight starts over, as the player's death starts it."));
		}
	}

	void AbelLanterns(const TArray<FString>& Args, UWorld* World)
	{
		AAbelKeeper* Abel = FindAbel(FindGameWorld(World), TEXT("Looter.Abel.Lanterns"));
		if (!Abel)
		{
			return;
		}
		const bool bDark = Args.Num() > 0 && Args[0].Equals(TEXT("dark"), ESearchCase::IgnoreCase);
		for (AKeeperLanternPost* Post : Abel->GetLanternPosts())
		{
			if (Post && bDark)
			{
				Post->SetDark(true);
			}
			else if (Post)
			{
				Post->Relight(nullptr);
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Abel.Lanterns: %d of %d lit."), Abel->NumLitLanterns(), Abel->GetLanternPosts().Num());
	}

	void AbelMove(const TArray<FString>& Args, UWorld* World)
	{
		AAbelKeeper* Abel = FindAbel(FindGameWorld(World), TEXT("Looter.Abel.Move"));
		const FString What = Args.Num() > 0 ? Args[0].ToLower() : FString();
		if (!Abel)
		{
			return;
		}
		bool bDone = true;
		if (What == TEXT("grieve"))
		{
			bDone = Abel->Grieve();
		}
		else if (What == TEXT("flare") || What == TEXT("buckshot"))
		{
			bDone = Abel->StartFlare();
		}
		else if (What == TEXT("drift") || What == TEXT("fog"))
		{
			bDone = Abel->DriftOut();
		}
		else if (What == TEXT("walkoff"))
		{
			bDone = Abel->WalkOff();
		}
		else if (What == TEXT("pull"))
		{
			Abel->Pull();
		}
		else if (What == TEXT("gust"))
		{
			Abel->ForceGust();
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Abel.Move <grieve|flare|drift|walkoff|pull|gust>"));
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Abel.Move %s: %s."), *What, bDone ? TEXT("done") : TEXT("not now (his fight must be on, and him free)"));
	}

	void AbelState(const TArray<FString>& Args, UWorld* World)
	{
		AAbelKeeper* Abel = FindAbel(FindGameWorld(World), TEXT("Looter.Abel.State"));
		if (!Abel)
		{
			return;
		}
		const UBossComponent* Boss = Abel->GetBoss();
		UE_LOG(LogLooter, Display, TEXT("Abel: %s, fight %s, phase %d, %.0f%% health, move %s, pose %s, coal %s, lanterns %d/%d, wind %s, %s."),
			Abel->IsPresent() ? TEXT("present") : TEXT("away"), Boss->IsFighting() ? TEXT("on") : (Boss->IsWon() ? TEXT("won") : TEXT("off")),
			Boss->GetPhase() + 1, Boss->GetBossHealth()->GetHealthPercent() * 100.f, *UEnum::GetValueAsString(Abel->GetMove()),
			AbelPoses::Name(Abel->GetPoseNow()), Abel->IsCoalOpen() ? TEXT("open") : TEXT("guarded"), Abel->NumLitLanterns(),
			Abel->GetLanternPosts().Num(), Abel->IsWindBlowing() ? TEXT("blowing") : TEXT("still"), Abel->IsHastened() ? TEXT("hastened") : TEXT("at his pace"));
	}

	/**
	 * Looter.Abel.Perf [adds]: the tour's view 13 measured where it happens (perf.ps1 -Exec): dusk at once, the player on
	 * the deck facing Abel and unhurtable, his fight on, and adds (10 by default, the fight's most) rising round the deck's
	 * middle and hunting the player.
	 */
	void AbelPerf(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		AAbelKeeper* Abel = FindAbel(GameWorld, TEXT("Looter.Abel.Perf"));
		APawn* Pawn = PlayerPawn(GameWorld);
		if (!Abel || !Pawn)
		{
			return;
		}
		Pawn->SetCanBeDamaged(false);
		if (!StartFightNow(GameWorld, *Abel, true))
		{
			return;
		}
		const int32 Adds = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 0, 10) : 10;
		const int32 Risen = Adds > 0 ? Abel->GetBoss()->SpawnWave(AbelRules::RisingUnpaid(Adds, 0)) : 0;
		UE_LOG(LogLooter, Display, TEXT("Looter.Abel.Perf: Abel and %d adds at dusk on the deck; the player can't be hurt now."), Risen);
	}

	FAutoConsoleCommandWithWorldAndArgs AbelFightCommand(TEXT("Looter.Abel.Fight"),
		TEXT("Looter.Abel.Fight [here]: Abel's fight now, whatever the story: dusk at once, the player put on the deck 8 m from him (not with ")
		TEXT("'here'), the fog wall closing as they're inside it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AbelFight));
	FAutoConsoleCommandWithWorldAndArgs AbelPhaseCommand(TEXT("Looter.Abel.Phase"),
		TEXT("Looter.Abel.Phase <1|2|3>: his fight on (started if needed) and his health set just under the phase's line: 2 the bell ")
		TEXT("and the fog, 3 the Gravewind (the lanterns relit first)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AbelPhase));
	FAutoConsoleCommandWithWorldAndArgs AbelKneelCommand(TEXT("Looter.Abel.Kneel"),
		TEXT("Looter.Abel.Kneel: brings him to zero as the player's kill: he kneels and the scene (Sit with Pa) follows."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AbelKneel));
	FAutoConsoleCommandWithWorldAndArgs AbelSceneCommand(TEXT("Looter.Abel.Scene"),
		TEXT("Looter.Abel.Scene: plays Sit with Pa alone with him where he stands, recording nothing; Looter.Abel.Back after."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AbelScene));
	FAutoConsoleCommandWithWorldAndArgs AbelBackCommand(TEXT("Looter.Abel.Back"),
		TEXT("Looter.Abel.Back: puts him back on the boards from his board after Looter.Abel.Scene (not once he's been beaten)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AbelBack));
	FAutoConsoleCommandWithWorldAndArgs AbelResetCommand(TEXT("Looter.Abel.Reset"),
		TEXT("Looter.Abel.Reset: starts his fight over, as the player's death does (he goes home healed, the adds go, the lanterns burn, the wall drops)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AbelReset));
	FAutoConsoleCommandWithWorldAndArgs AbelLanternsCommand(TEXT("Looter.Abel.Lanterns"),
		TEXT("Looter.Abel.Lanterns [dark|lit]: puts the deck's three lanterns out, or relights them as the player would (dragging him back)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AbelLanterns));
	FAutoConsoleCommandWithWorldAndArgs AbelMoveCommand(TEXT("Looter.Abel.Move"),
		TEXT("Looter.Abel.Move <grieve|flare|drift|walkoff|pull|gust>: one of his moments now (his fight must be on)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AbelMove));
	FAutoConsoleCommandWithWorldAndArgs AbelStateCommand(TEXT("Looter.Abel.State"),
		TEXT("Looter.Abel.State: where he and his fight stand (present, phase, health, his moment and pose, the coal, the lanterns, the wind)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AbelState));
	FAutoConsoleCommandWithWorldAndArgs AbelPerfCommand(TEXT("Looter.Abel.Perf"),
		TEXT("Looter.Abel.Perf [adds 0-10]: the tour's view 13 to measure: dusk, the player on the deck unhurtable, his fight on and adds hunting: ")
		TEXT("perf.ps1 -Map /Game/Maps/Lvl_RansomsRest -Exec \"Looter.Quality Medium,Looter.Abel.Perf 10\"."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AbelPerf));
}

#endif
