// Developer console commands for creatures (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "World/WorldQueries.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
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

	void SetCreatureHealth(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		TArray<float> Values;
		for (const FString& Arg : Args)
		{
			Values.Add(FMath::Max(FCString::Atof(*Arg), 1.f));
		}
		if (!Pawn || Values.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.CreatureHealth: give at least one health value, with the game running."));
			return;
		}

		// Nearest first, so the creatures in view get the values in the order they were typed.
		TArray<ACreatureBase*> Creatures;
		for (TActorIterator<ACreatureBase> It(GameWorld); It; ++It)
		{
			Creatures.Add(*It);
		}
		const FVector From = Pawn->GetActorLocation();
		Creatures.Sort([&From](const ACreatureBase& A, const ACreatureBase& B)
		{
			return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From);
		});
		for (int32 Index = 0; Index < Creatures.Num(); ++Index)
		{
			if (UHealthComponent* Health = Creatures[Index]->FindComponentByClass<UHealthComponent>())
			{
				Health->MaxHealth = Values[Index % Values.Num()];
				Health->ResetHealth();
				UE_LOG(LogLooter, Log, TEXT("Looter.CreatureHealth: %s now has %.0f health (%.0f m away)."), *Creatures[Index]->GetName(),
					Health->MaxHealth, FVector::Dist(Creatures[Index]->GetActorLocation(), From) / 100.f);
			}
		}
	}

	FAutoConsoleCommandWithWorldAndArgs CreatureHealthCommand(
		TEXT("Looter.CreatureHealth"),
		TEXT("Looter.CreatureHealth <health> [<health> ...]: sets every creature's maximum health and heals it, nearest first, ")
		TEXT("going through the values in turn (to compare their health bars). Until the level reloads."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetCreatureHealth));

	/** A kind of creature a command names: its class, and whether it's a spiderling (the brown spider as the Gravemother's brood). */
	struct FCreatureKind
	{
		TSubclassOf<ACreatureBase> Class;
		bool bSpiderling = false;
	};

	/** The creature kind a command names ("Spider", "slime", "Unpaid", "Gravemother", "Spiderling"), or none. */
	FCreatureKind FindCreatureKind(const FString& Kind)
	{
		FCreatureKind Found;
		if (Kind.Equals(TEXT("Spider"), ESearchCase::IgnoreCase))
		{
			Found.Class = ASpiderCreature::StaticClass();
		}
		else if (Kind.Equals(TEXT("Spiderling"), ESearchCase::IgnoreCase))
		{
			Found.Class = ASpiderCreature::StaticClass();
			Found.bSpiderling = true;
		}
		else if (Kind.Equals(TEXT("Gravemother"), ESearchCase::IgnoreCase))
		{
			Found.Class = AGravemotherCreature::StaticClass();
		}
		else if (Kind.Equals(TEXT("Slime"), ESearchCase::IgnoreCase))
		{
			Found.Class = ASlimeCreature::StaticClass();
		}
		else if (Kind.Equals(TEXT("Unpaid"), ESearchCase::IgnoreCase))
		{
			Found.Class = AUnpaidCreature::StaticClass();
		}
		return Found;
	}

	/** How a creature of Kind starts when a command spawns it: a spiderling as the Gravemother's brood come, else its class's. */
	ACreatureBase::FRuntimeSpawn MakeSpawn(const FCreatureKind& Kind)
	{
		return Kind.bSpiderling ? AGravemotherCreature::MakeSpiderlingSpawn() : ACreatureBase::FRuntimeSpawn();
	}

	/** The ground under a spot (terrain and solid props, never volumes), from well above it to well below. */
	bool FindSpawnGround(UWorld* World, const FVector& Spot, FVector& OutGround)
	{
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SpawnCreature"));
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Spot + FVector(0.f, 0.f, 800.f), Spot - FVector(0.f, 0.f, 3000.f),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			OutGround = Hit.ImpactPoint;
			return true;
		}
		return false;
	}

	void SpawnCreature(const TArray<FString>& AllArgs, UWorld* World)
	{
		// key=value arguments (size=0.45, level=5) set the optional extras; the rest are in order.
		TArray<FString> Args;
		float Size = 0.f;
		int32 SpawnLevel = 0;
		bool bChase = false;
		for (const FString& Arg : AllArgs)
		{
			FString Key;
			FString Value;
			if (Arg.Split(TEXT("="), &Key, &Value))
			{
				if (Key.Equals(TEXT("size"), ESearchCase::IgnoreCase))
				{
					Size = FMath::Clamp(FCString::Atof(*Value), 0.1f, 5.f);
				}
				else if (Key.Equals(TEXT("level"), ESearchCase::IgnoreCase))
				{
					SpawnLevel = FMath::Max(FCString::Atoi(*Value), 1);
				}
			}
			else if (Arg.Equals(TEXT("chase"), ESearchCase::IgnoreCase))
			{
				bChase = true;
			}
			else
			{
				Args.Add(Arg);
			}
		}

		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnCreature: no player (start the game first)."));
			return;
		}
		const FCreatureKind Kind = FindCreatureKind(Args.Num() > 0 ? Args[0] : FString(TEXT("Spider")));
		if (!Kind.Class)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnCreature: no creature called '%s' (Spider, Spiderling, Gravemother, Slime or Unpaid)."),
				*Args[0]);
			return;
		}
		// With no rank named, its kind's own: the Gravemother is Legendary, everything else Basic.
		const ACreatureBase* Defaults = Kind.Class->GetDefaultObject<ACreatureBase>();
		ECreatureRank Rank = Defaults->StartingRank;
		if (Args.Num() > 1 && !UCreatureRankSettings::ParseRank(Args[1], Rank))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnCreature: no rank called '%s' (Basic, Rare, Epic, Legendary, Boss, or ")
				TEXT("Restless, Gravebound, Soulfed)."), *Args[1]);
			return;
		}
		const int32 Count = Args.Num() > 2 ? FMath::Clamp(FCString::Atoi(*Args[2]), 1, 40) : 1;

		// In rows of up to five, from about 8 m in front of the player, spaced by the creature's size, each facing the player.
		ACreatureBase::FRuntimeSpawn Spawn = MakeSpawn(Kind);
		Spawn.Rank = Rank;
		Spawn.Level = SpawnLevel;
		if (Size > 0.f)
		{
			Spawn.BodyScale = Size;
		}
		const float Grown = (Spawn.BodyScale > 0.f ? Spawn.BodyScale : Defaults->BodyScale) * UCreatureRankSettings::Get(Rank).Size;
		const float Spacing = FMath::Max(Defaults->GetCapsuleComponent()->GetUnscaledCapsuleRadius() * Grown * 3.f, 120.f);
		// Where the player is looking, level (the body can face elsewhere in third person).
		const FVector Ahead = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Ahead);
		constexpr int32 PerRow = 5;
		int32 Spawned = 0;
		FString Name;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const int32 Row = Index / PerRow;
			const int32 InRow = FMath::Min(Count - Row * PerRow, PerRow);
			const float Across = (static_cast<float>(Index % PerRow) - (InRow - 1) * 0.5f) * Spacing;
			const FVector Spot = Pawn->GetActorLocation() + Ahead * (800.f + Spacing + Row * Spacing) + Side * Across;
			FVector Ground;
			if (!FindSpawnGround(GameWorld, Spot, Ground))
			{
				continue;
			}
			const float Yaw = static_cast<float>((Pawn->GetActorLocation() - Ground).Rotation().Yaw);
			ACreatureBase* Creature = ACreatureBase::SpawnAtRuntime(GameWorld, Kind.Class, Ground, Yaw, Spawn);
			if (!Creature)
			{
				continue;
			}
			if (Kind.bSpiderling)
			{
				Creature->DisplayName = AGravemotherCreature::SpiderlingName();
			}
			++Spawned;
			Name = Creature->DisplayName.ToString();
			if (bChase)
			{
				Creature->AlertTo(Pawn);
			}
		}

		const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Rank);
		const bool bWord = !Info.Word.IsEmpty() && !Defaults->bNameIsRankWord;
		const FString Shown = bWord ? FString::Printf(TEXT("%s %s"), *Info.Word.ToString(), *Name) : Name;
		UE_LOG(LogLooter, Log, TEXT("Looter.SpawnCreature: %d of %d %s (%s, %.2fx size, level %d) in front of the player%s."), Spawned, Count,
			*Shown, *UCreatureRankSettings::GetRankName(Rank), Grown,
			(SpawnLevel > 0 ? SpawnLevel : Defaults->Level) + Info.LevelOffset, bChase ? TEXT(", hunting them") : TEXT(""));
		if (Spawned < Count)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnCreature: %d found no ground in front of the player."), Count - Spawned);
		}
	}

	void SpawnHordeNow(const TArray<FString>& Args, UWorld* GameWorld, int32 TriesLeft);

	/**
	 * Looter.Perf.Horde <kind> <count> [rank] [x y yaw]: a fight's cost, measured where it happens (perf.ps1 -Exec). The
	 * tour looks through a camera of its own while the player stays at the spawn, and creatures far from the player slow
	 * down (FCreatureUpdateRate), so a fight has to come to the player: it's put at (x, y) facing yaw when given, can't be
	 * hurt until the level loads again, and count creatures come at it from an arc 8 to 14 m ahead, in view.
	 */
	void SpawnHorde(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.Horde: no game (start the game first)."));
			return;
		}
		SpawnHordeNow(Args, GameWorld, 20);
	}

	void SpawnHordeNow(const TArray<FString>& Args, UWorld* GameWorld, int32 TriesLeft)
	{
		APlayerController* Controller = GameWorld->GetFirstPlayerController();
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			// From the command line (-ExecCmds) the game may not have its player yet: look again shortly, for up to 10 s.
			if (TriesLeft > 0)
			{
				FTimerHandle Retry;
				const TWeakObjectPtr<UWorld> WeakWorld(GameWorld);
				GameWorld->GetTimerManager().SetTimer(Retry, FTimerDelegate::CreateLambda([Args, WeakWorld, TriesLeft]()
				{
					if (UWorld* Again = WeakWorld.Get())
					{
						SpawnHordeNow(Args, Again, TriesLeft - 1);
					}
				}), 0.5f, false);
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.Horde: the game never had a player."));
			}
			return;
		}
		const FCreatureKind Kind = FindCreatureKind(Args.Num() > 0 ? Args[0] : FString(TEXT("Unpaid")));
		ECreatureRank Rank = Kind.Class ? Kind.Class->GetDefaultObject<ACreatureBase>()->StartingRank : ECreatureRank::Basic;
		if (!Kind.Class || (Args.Num() > 2 && !UCreatureRankSettings::ParseRank(Args[2], Rank)))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.Horde <Spider|Spiderling|Gravemother|Slime|Unpaid> <count> [rank] [x y yaw]: no such ")
				TEXT("creature or rank."));
			return;
		}
		const int32 Count = Args.Num() > 1 ? FMath::Clamp(FCString::Atoi(*Args[1]), 1, 40) : 12;

		if (Args.Num() > 5)
		{
			FVector Ground;
			const FVector Spot(FCString::Atof(*Args[3]), FCString::Atof(*Args[4]), Pawn->GetActorLocation().Z);
			if (FindSpawnGround(GameWorld, Spot, Ground))
			{
				const float HalfHeight = Pawn->GetRootComponent()->Bounds.BoxExtent.Z;
				Pawn->SetActorLocation(Ground + FVector(0.f, 0.f, HalfHeight + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
				Controller->SetControlRotation(FRotator(-5.f, FCString::Atof(*Args[5]), 0.f));
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.Horde: no ground at (%s, %s); the fight comes to the player where they stand."),
					*Args[3], *Args[4]);
			}
		}
		Pawn->SetCanBeDamaged(false);

		// An arc across the view, alternately nearer and farther, each one facing the player and hunting them.
		ACreatureBase::FRuntimeSpawn Spawn = MakeSpawn(Kind);
		Spawn.Rank = Rank;
		const FVector Ahead = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector();
		int32 Spawned = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Across = Count > 1 ? FMath::Lerp(-70.f, 70.f, static_cast<float>(Index) / (Count - 1)) : 0.f;
			const float Distance = 800.f + static_cast<float>(Index % 3) * 300.f;
			FVector Ground;
			if (!FindSpawnGround(GameWorld, Pawn->GetActorLocation() + Ahead.RotateAngleAxis(Across, FVector::UpVector) * Distance, Ground))
			{
				continue;
			}
			const float Yaw = static_cast<float>((Pawn->GetActorLocation() - Ground).Rotation().Yaw);
			if (ACreatureBase* Creature = ACreatureBase::SpawnAtRuntime(GameWorld, Kind.Class, Ground, Yaw, Spawn))
			{
				if (Kind.bSpiderling)
				{
					Creature->DisplayName = AGravemotherCreature::SpiderlingName();
				}
				Creature->AlertTo(Pawn);
				++Spawned;
			}
		}
		const FVector Where = Pawn->GetActorLocation();
		UE_LOG(LogLooter, Display, TEXT("Looter.Perf.Horde: %d of %d %s (%s) hunting the player at (%.0f, %.0f, %.0f), who can't be hurt now."),
			Spawned, Count, Kind.bSpiderling ? TEXT("spiderlings") : *Kind.Class->GetName(), *UCreatureRankSettings::GetRankName(Rank),
			Where.X, Where.Y, Where.Z);
	}

	FAutoConsoleCommandWithWorldAndArgs HordeCommand(
		TEXT("Looter.Perf.Horde"),
		TEXT("Looter.Perf.Horde <Spider|Spiderling|Gravemother|Slime|Unpaid> <count> [rank] [x y yaw]: to measure a fight, puts ")
		TEXT("the player at (x, y) facing yaw (when given) where they can't be hurt, and sends count creatures of a rank at them from ")
		TEXT("an arc ahead: ")
		TEXT("perf.ps1 -Map /Game/Maps/Lvl_RansomsRest -Exec \"Looter.Quality Medium,Looter.Perf.Horde Unpaid 12 Basic 0 -1400 90\"."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnHorde));

	FAutoConsoleCommandWithWorldAndArgs SpawnCreatureCommand(
		TEXT("Looter.SpawnCreature"),
		TEXT("Looter.SpawnCreature <Spider|Spiderling|Gravemother|Slime|Unpaid> [Basic|Rare|Epic|Legendary|Boss] [count] [chase] ")
		TEXT("[size=<scale>] [level=<n>]: spawns creatures of a rank (with none named, their kind's own: the Gravemother is ")
		TEXT("Legendary) on the ground in front of the player (rows of five, from about 8 m out). They never come back once ")
		TEXT("killed. chase sets them on the player at once; size sets their BodyScale (0.45 a spiderling, 1.8 a giant) before the ")
		TEXT("rank's own; level is before the rank's offset. A Spiderling is the Gravemother's brood: a brown spider at 0.45 with a ")
		TEXT("fifth of its health. Ranks also take their words (Restless, Gravebound, Soulfed): Looter.SpawnCreature Unpaid ")
		TEXT("Gravebound 1 chase. The Unpaid's cap (12 at once) holds for the encounters, not here."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnCreature));
}

#endif
