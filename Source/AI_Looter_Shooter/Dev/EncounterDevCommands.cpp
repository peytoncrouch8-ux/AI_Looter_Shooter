// Developer console commands for encounters: the spawners, their waves and the safe zones (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "World/SafeGround.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/LineBatchComponent.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
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

	/** The level's encounters; logged when there are none to be had (the game isn't running). */
	UEncounterSubsystem* FindEncounters(UWorld* GameWorld, const TCHAR* Command)
	{
		UEncounterSubsystem* Encounters = GameWorld ? GameWorld->GetSubsystem<UEncounterSubsystem>() : nullptr;
		if (!Encounters)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no game running."), Command);
		}
		return Encounters;
	}

	/** How far a spot is from the player (metres), for the logs; 0 without a player. */
	double MetresFromPlayer(const UEncounterSubsystem& Encounters, const FVector& Spot)
	{
		const APawn* Player = Encounters.GetPlayer();
		return Player ? FVector::Dist(Player->GetActorLocation(), Spot) / 100.0 : 0.0;
	}

	/**
	 * Looter.Encounter.List: every spawner (its state and story, waves, alive, owed, killed, how far), every safe zone (on
	 * or off, and whether the player stands in it), and the caps as they stand: creatures near the player, each capped kind.
	 */
	void ListEncounters(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Encounter.List");
		UWorld* GameWorld = FindGameWorld(World);
		const UEncounterSubsystem* Encounters = FindEncounters(GameWorld, Command);
		if (!Encounters)
		{
			return;
		}
		const UEncounterSettings& Settings = UEncounterSettings::Get();
		const APawn* Player = Encounters->GetPlayer();
		const TArray<AEncounterSpawner*> Spawners = Encounters->GetSpawners();
		for (const AEncounterSpawner* Spawner : Spawners)
		{
			UE_LOG(LogLooter, Display, TEXT("  %-22s %s; %.0f m away"), *Spawner->GetSpawnerId().ToString(), *Spawner->Describe(),
				MetresFromPlayer(*Encounters, Spawner->GetActorLocation()));
		}
		const TArray<ASafeGround*> Zones = Encounters->GetSafeZones();
		for (const ASafeGround* Zone : Zones)
		{
			UE_LOG(LogLooter, Display, TEXT("  safe zone %-14s %s (%s), %.0f square meters%s"), *Zone->GetZoneId().ToString(),
				Zone->IsActive() ? TEXT("on") : TEXT("off"), *Zone->ActiveWhen.Describe(), Zone->GetSurfaceArea() / 10000.0,
				Player && Zone->Contains(Player->GetActorLocation()) ? TEXT("; the player is in it") : TEXT(""));
		}
		const int32 Near = Player ? Encounters->CountAliveNear(Player->GetActorLocation(), Settings.NearPlayerRadius) : 0;
		UE_LOG(LogLooter, Display, TEXT("%s: %d spawners and %d safe zones in %s; %d creatures alive, %d of them within %.0f m of the player (at most %d)."),
			Command, Spawners.Num(), Zones.Num(), *GameWorld->GetName(), Encounters->CountAlive(), Near, Settings.NearPlayerRadius / 100.f,
			Settings.MaxCreaturesNearPlayer);
		for (const FEncounterClassCap& Cap : Settings.ClassCaps)
		{
			const UClass* Kind = Cap.CreatureClass.Get();
			UE_LOG(LogLooter, Display, TEXT("  cap on %s: %s"), *Cap.CreatureClass.ToString(),
				Kind ? *FString::Printf(TEXT("%d alive of at most %d"), Encounters->CountAlive(Kind), Cap.MaxAlive) : TEXT("no such class yet"));
		}
	}

	/**
	 * Looter.Encounter.Wave <spawner id | event | nearest> [force]: starts a spawner's next wave now, or sends an encounter
	 * event to every spawner listening for it. force turns a spawner on whatever its story says, and starts a cleared one
	 * over, until the level is loaded again.
	 */
	void StartWaveNow(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Encounter.Wave");
		UWorld* GameWorld = FindGameWorld(World);
		UEncounterSubsystem* Encounters = FindEncounters(GameWorld, Command);
		if (!Encounters)
		{
			return;
		}
		if (Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: %s <spawner id | event | nearest> [force] (Looter.Encounter.List shows the ids)"), Command);
			return;
		}
		const bool bForce = Args.Num() > 1 && Args[1].Equals(TEXT("force"), ESearchCase::IgnoreCase);
		AEncounterSpawner* Chosen = nullptr;
		if (Args[0].Equals(TEXT("nearest"), ESearchCase::IgnoreCase))
		{
			double Nearest = TNumericLimits<double>::Max();
			for (AEncounterSpawner* Spawner : Encounters->GetSpawners())
			{
				const double Metres = MetresFromPlayer(*Encounters, Spawner->GetActorLocation());
				if (Metres < Nearest)
				{
					Nearest = Metres;
					Chosen = Spawner;
				}
			}
		}
		else
		{
			Chosen = Encounters->FindSpawner(FName(*Args[0]));
		}
		if (Chosen)
		{
			const bool bStarted = Chosen->TriggerWave(bForce);
			UE_LOG(LogLooter, Log, TEXT("%s: %s %s; %s."), Command, *Chosen->GetSpawnerId().ToString(),
				bStarted ? TEXT("starts a wave") : TEXT("starts none (see above)"), *Chosen->Describe());
			return;
		}
		// Not a spawner's id: an encounter event, as a boss's call or an egg sac sends it.
		const int32 Started = Encounters->SendEvent(FName(*Args[0]));
		UE_LOG(LogLooter, Log, TEXT("%s: no spawner is called %s; as an event it started %d waves."), Command, *Args[0], Started);
	}

	/** The line batch Looter.Encounter.Zones draws into (lines in it stay until it's cleared), and the world it's on in. */
	constexpr uint32 EncounterBatchId = 0x456E6343;
	TWeakObjectPtr<UWorld> EncountersShownIn;

	/**
	 * Looter.Encounter.Zones [1|0]: draws every safe zone (on: green; off: dim) and every spawner's hunting ground (orange),
	 * where its creatures appear (cyan) and how near the player comes before they do (grey); again, or 0, clears it. Logs
	 * each zone and whether the player stands in it.
	 */
	void ShowZones(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Encounter.Zones");
		UWorld* GameWorld = FindGameWorld(World);
		ULineBatchComponent* Lines = GameWorld ? GameWorld->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent) : nullptr;
		const UEncounterSubsystem* Encounters = FindEncounters(GameWorld, Command);
		if (!Lines || !Encounters)
		{
			return;
		}
		const bool bShow = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : EncountersShownIn.Get() != GameWorld;
		Lines->ClearBatch(EncounterBatchId);
		EncountersShownIn = bShow ? GameWorld : nullptr;
		if (!bShow)
		{
			UE_LOG(LogLooter, Display, TEXT("%s: off."), Command);
			return;
		}
		const APawn* Player = Encounters->GetPlayer();
		const TArray<ASafeGround*> Zones = Encounters->GetSafeZones();
		for (const ASafeGround* Zone : Zones)
		{
			Zone->DrawZone(*Lines, EncounterBatchId);
			UE_LOG(LogLooter, Display, TEXT("  %-14s %s (%s), %d corners%s"), *Zone->GetZoneId().ToString(), Zone->IsActive() ? TEXT("on") : TEXT("off"),
				*Zone->ActiveWhen.Describe(), Zone->Corners.Num(), Player && Zone->Contains(Player->GetActorLocation()) ? TEXT("; the player is in it") : TEXT(""));
		}
		const TArray<AEncounterSpawner*> Spawners = Encounters->GetSpawners();
		for (const AEncounterSpawner* Spawner : Spawners)
		{
			Spawner->DrawEncounter(*Lines, EncounterBatchId);
		}
		UE_LOG(LogLooter, Display, TEXT("%s: on, %d safe zones and %d spawners in %s."), Command, Zones.Num(), Spawners.Num(), *GameWorld->GetName());
	}

	/**
	 * Looter.Encounter.Test [count] [Spider|Slime] [waves=<n>] [every=<seconds>] [alive=<n>] [hunt]: a test spawner (not
	 * saved) on the ground where the player looks, with count creatures a wave (4 spiders by default) at the area's ranks,
	 * and its first wave out at once. waves= brings more, every= seconds apart (without it, each once the last is dead);
	 * alive= caps how many are out at once; hunt sets each wave on the player as it appears.
	 */
	void PlaceTestSpawner(const TArray<FString>& AllArgs, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Encounter.Test");
		int32 Count = 4;
		TSubclassOf<ACreatureBase> Kind = ASpiderCreature::StaticClass();
		int32 Waves = 1;
		float Every = 0.f;
		int32 MostAlive = 0;
		bool bHunt = false;
		for (const FString& Arg : AllArgs)
		{
			FString Key;
			FString Value;
			if (Arg.Split(TEXT("="), &Key, &Value))
			{
				if (Key.Equals(TEXT("waves"), ESearchCase::IgnoreCase))
				{
					Waves = FMath::Clamp(FCString::Atoi(*Value), 1, 20);
				}
				else if (Key.Equals(TEXT("every"), ESearchCase::IgnoreCase))
				{
					Every = FMath::Max(FCString::Atof(*Value), 0.f);
				}
				else if (Key.Equals(TEXT("alive"), ESearchCase::IgnoreCase))
				{
					MostAlive = FMath::Max(FCString::Atoi(*Value), 0);
				}
			}
			else if (Arg.Equals(TEXT("hunt"), ESearchCase::IgnoreCase))
			{
				bHunt = true;
			}
			else if (Arg.IsNumeric())
			{
				Count = FMath::Clamp(FCString::Atoi(*Arg), 1, 40);
			}
			else if (Arg.Equals(TEXT("Slime"), ESearchCase::IgnoreCase))
			{
				Kind = ASlimeCreature::StaticClass();
			}
			else if (!Arg.Equals(TEXT("Spider"), ESearchCase::IgnoreCase))
			{
				UE_LOG(LogLooter, Warning, TEXT("%s: '%s' isn't a count, Spider or Slime, waves=, every=, alive= or hunt; left out."), Command, *Arg);
			}
		}

		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no player (start the game first)."), Command);
			return;
		}
		// Where the player looks: the first thing their view meets within 100 m (15 m ahead if nothing), then the ground under it.
		FVector Eye;
		FRotator View;
		Controller->GetPlayerViewPoint(Eye, View);
		const FCollisionQueryParams Sight(SCENE_QUERY_STAT(EncounterTestSight), false, Pawn);
		FHitResult Hit;
		FVector Spot = Eye + View.Vector() * 1500.0;
		if (GameWorld->LineTraceSingleByChannel(Hit, Eye, Eye + View.Vector() * 10000.0, ECC_Visibility, Sight))
		{
			Spot = Hit.ImpactPoint;
		}
		const FCollisionQueryParams GroundQuery = LooterWorld::StaticGeometryParams(GameWorld, TEXT("EncounterTestGround"));
		if (!GameWorld->LineTraceSingleByObjectType(Hit, Spot + FVector(0.0, 0.0, 500.0), Spot - FVector(0.0, 0.0, 3000.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), GroundQuery))
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no ground where the player looks."), Command);
			return;
		}
		const FVector Feet = Hit.ImpactPoint;

		// Each its own id, so the console can name it.
		static int32 PlacedCount = 0;
		const FTransform Where(FRotator(0.0, View.Yaw, 0.0), Feet);
		AEncounterSpawner* Spawner = GameWorld->SpawnActorDeferred<AEncounterSpawner>(AEncounterSpawner::StaticClass(), Where);
		if (!Spawner)
		{
			return;
		}
		Spawner->SpawnerId = FName(*FString::Printf(TEXT("DevEncounter%d"), ++PlacedCount));
		FEncounterGroup Group;
		Group.CreatureClass = Kind;
		Group.Count = Count;
		Group.RankRoll = EEncounterRankRoll::Area;
		Spawner->Groups = { Group };
		Spawner->SpawnRadius = FMath::Clamp(150.f * Count, 400.f, 1500.f);
		Spawner->GiveUpRadius = Spawner->SpawnRadius + 2000.f;
		Spawner->NumWaves = Waves;
		Spawner->WaveInterval = Every;
		// Without an interval, each wave comes once the last is dead.
		Spawner->bWaitForClear = Every <= 0.f && Waves > 1;
		Spawner->MaxAlive = MostAlive;
		Spawner->bHuntOnSpawn = bHunt;
		const float FromPlayer = static_cast<float>(FVector::Dist(Pawn->GetActorLocation(), Feet));
		Spawner->ActivationRadius = FMath::Max(4500.f, FromPlayer + 1000.f);
		Spawner->DespawnRadius = Spawner->ActivationRadius * 2.f;
		Spawner->FinishSpawning(Where);
		const bool bStarted = Spawner->TriggerWave();
		UE_LOG(LogLooter, Log, TEXT("%s: %s placed %.0f m away where the player looks (%d %s a wave, %d waves%s%s%s): %s."), Command,
			*Spawner->GetSpawnerId().ToString(), FromPlayer / 100.f, Count, *GetNameSafe(Kind.Get()), Waves,
			Every > 0.f ? *FString::Printf(TEXT(", %.0f s apart"), Every) : TEXT(""),
			MostAlive > 0 ? *FString::Printf(TEXT(", %d alive at most"), MostAlive) : TEXT(""), bHunt ? TEXT(", hunting") : TEXT(""),
			bStarted ? *Spawner->Describe() : TEXT("no wave came"));
	}

	FAutoConsoleCommandWithWorldAndArgs ListEncountersCommand(
		TEXT("Looter.Encounter.List"),
		TEXT("Looter.Encounter.List: every encounter spawner (state, story, waves, alive, owed, killed, distance), every safe zone, ")
		TEXT("and the caps as they stand (creatures near the player, each capped kind)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListEncounters));

	FAutoConsoleCommandWithWorldAndArgs StartWaveCommand(
		TEXT("Looter.Encounter.Wave"),
		TEXT("Looter.Encounter.Wave <spawner id | event | nearest> [force]: starts a spawner's next wave now, or sends an encounter ")
		TEXT("event to the spawners listening for it. force turns a spawner on whatever its story says and starts a cleared one over."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StartWaveNow));

	FAutoConsoleCommandWithWorldAndArgs ShowZonesCommand(
		TEXT("Looter.Encounter.Zones"),
		TEXT("Looter.Encounter.Zones [1|0]: draws the safe zones (on green, off dim) and every spawner's hunting ground, spawn spots ")
		TEXT("and approach ring; again, or 0, clears it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowZones));

	FAutoConsoleCommandWithWorldAndArgs PlaceTestSpawnerCommand(
		TEXT("Looter.Encounter.Test"),
		TEXT("Looter.Encounter.Test [count] [Spider|Slime] [waves=<n>] [every=<seconds>] [alive=<n>] [hunt]: a test spawner (not ")
		TEXT("saved) where the player looks, its first wave out at once (4 spiders by default, at the area's ranks)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PlaceTestSpawner));
}

#endif
