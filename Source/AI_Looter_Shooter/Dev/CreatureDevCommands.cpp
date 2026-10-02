// Developer console commands for creatures (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
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

	/** The creature class a command names ("Spider", "slime"), or none. */
	TSubclassOf<ACreatureBase> FindCreatureKind(const FString& Kind)
	{
		if (Kind.Equals(TEXT("Spider"), ESearchCase::IgnoreCase))
		{
			return ASpiderCreature::StaticClass();
		}
		if (Kind.Equals(TEXT("Slime"), ESearchCase::IgnoreCase))
		{
			return ASlimeCreature::StaticClass();
		}
		return nullptr;
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
		const TSubclassOf<ACreatureBase> Kind = FindCreatureKind(Args.Num() > 0 ? Args[0] : FString(TEXT("Spider")));
		if (!Kind)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnCreature: no creature called '%s' (Spider or Slime)."), *Args[0]);
			return;
		}
		ECreatureRank Rank = ECreatureRank::Basic;
		if (Args.Num() > 1 && !UCreatureRankSettings::ParseRank(Args[1], Rank))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnCreature: no rank called '%s' (Basic, Rare, Epic, Legendary, Boss, or ")
				TEXT("Restless, Gravebound, Soulfed)."), *Args[1]);
			return;
		}
		const int32 Count = Args.Num() > 2 ? FMath::Clamp(FCString::Atoi(*Args[2]), 1, 40) : 1;

		// In rows of up to five, from about 8 m in front of the player, spaced by the creature's size, each facing the player.
		ACreatureBase::FRuntimeSpawn Spawn;
		Spawn.Rank = Rank;
		Spawn.Level = SpawnLevel;
		Spawn.BodyScale = Size;
		const ACreatureBase* Defaults = Kind->GetDefaultObject<ACreatureBase>();
		const float Grown = (Size > 0.f ? Size : Defaults->BodyScale) * UCreatureRankSettings::Get(Rank).Size;
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
			ACreatureBase* Creature = ACreatureBase::SpawnAtRuntime(GameWorld, Kind, Ground, Yaw, Spawn);
			if (!Creature)
			{
				continue;
			}
			++Spawned;
			Name = Creature->DisplayName.ToString();
			if (bChase)
			{
				Creature->AlertTo(Pawn);
			}
		}

		const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Rank);
		const FString Shown = Info.Word.IsEmpty() ? Name : FString::Printf(TEXT("%s %s"), *Info.Word.ToString(), *Name);
		UE_LOG(LogLooter, Log, TEXT("Looter.SpawnCreature: %d of %d %s (%s, %.2fx size, level %d) in front of the player%s."), Spawned, Count,
			*Shown, *UCreatureRankSettings::GetRankName(Rank), Grown,
			(SpawnLevel > 0 ? SpawnLevel : Defaults->Level) + Info.LevelOffset, bChase ? TEXT(", hunting them") : TEXT(""));
		if (Spawned < Count)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnCreature: %d found no ground in front of the player."), Count - Spawned);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs SpawnCreatureCommand(
		TEXT("Looter.SpawnCreature"),
		TEXT("Looter.SpawnCreature <Spider|Slime> [Basic|Rare|Epic|Legendary|Boss] [count] [chase] [size=<scale>] [level=<n>]: ")
		TEXT("spawns creatures of a rank on the ground in front of the player (rows of five, from about 8 m out). They never ")
		TEXT("come back once killed. chase sets them on the player at once; size sets their BodyScale (0.45 a spiderling, 1.8 a ")
		TEXT("giant) before the rank's own; level is before the rank's offset. Ranks also take their words (Restless, Gravebound, Soulfed)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnCreature));
}

#endif
