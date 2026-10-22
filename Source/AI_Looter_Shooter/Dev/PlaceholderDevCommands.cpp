// Developer console command for the placeholder creatures (not in shipping builds): Looter.SpawnPlaceholder.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/PlaceholderCreature.h"
#include "World/WorldQueries.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** Where each kind's class lives (Tools/Unreal/create_placeholder_creatures.py makes it). */
	const TCHAR* PlaceholderFolder = TEXT("/Game/Data/Placeholders");

	UWorld* FindPlaceholderWorld(UWorld* World)
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

	bool FindPlaceholderGround(UWorld* World, const FVector& Spot, FVector& OutGround)
	{
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SpawnPlaceholder"));
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Spot + FVector(0.f, 0.f, 800.f), Spot - FVector(0.f, 0.f, 3000.f),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			OutGround = Hit.ImpactPoint;
			return true;
		}
		return false;
	}

	/**
	 * Looter.SpawnPlaceholder <Name> [rank] [count] [chase]: count placeholders of a kind (BP_Placeholder_<Name>: CoffinRat,
	 * BoneHound, Knocker, Gravepicker, GravepickerLookout, ...) about 8 m in front of the player, in a row, facing them;
	 * "chase" sets them on the player. Spawned in play: gone for good once killed.
	 */
	void SpawnPlaceholder(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindPlaceholderWorld(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn || Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnPlaceholder <Name> [rank] [count] [chase]: start the game first, and name a kind."));
			return;
		}
		const FString Name = Args[0];
		const FString ClassPath = FString::Printf(TEXT("%s/BP_Placeholder_%s.BP_Placeholder_%s_C"), PlaceholderFolder, *Name, *Name);
		UClass* Class = LoadClass<APlaceholderCreature>(nullptr, *ClassPath);
		if (!Class)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnPlaceholder: no %s (run create_placeholder_creatures.py)."), *ClassPath);
			return;
		}
		ECreatureRank Rank = ECreatureRank::Basic;
		if (Args.Num() > 1 && !UCreatureRankSettings::ParseRank(Args[1], Rank))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnPlaceholder: no rank called '%s'."), *Args[1]);
			return;
		}
		const int32 Count = Args.Num() > 2 ? FMath::Clamp(FCString::Atoi(*Args[2]), 1, 20) : 1;
		const bool bChase = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("chase"), ESearchCase::IgnoreCase); });

		const FVector Ahead = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Ahead);
		ACreatureBase::FRuntimeSpawn Spawn;
		Spawn.Rank = Rank;
		int32 Spawned = 0;
		const APlaceholderCreature* Last = nullptr;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Spot = Pawn->GetActorLocation() + Ahead * 800.f + Side * ((Index - (Count - 1) * 0.5f) * 220.f);
			FVector Ground;
			if (!FindPlaceholderGround(GameWorld, Spot, Ground))
			{
				continue;
			}
			const float Yaw = static_cast<float>((Pawn->GetActorLocation() - Ground).Rotation().Yaw);
			APlaceholderCreature* Creature = Cast<APlaceholderCreature>(ACreatureBase::SpawnAtRuntime(GameWorld, Class, Ground, Yaw, Spawn));
			if (!Creature)
			{
				continue;
			}
			++Spawned;
			Last = Creature;
			if (bChase)
			{
				Creature->AlertTo(Pawn);
			}
		}
		if (Last)
		{
			const UHealthComponent* Health = Last->FindComponentByClass<UHealthComponent>();
			UE_LOG(LogLooter, Log, TEXT("Looter.SpawnPlaceholder: %d of %d %s (%s), level %d, %.0f health, a %.1f attack, %d XP at level 1%s."),
				Spawned, Count, *Last->DisplayName.ToString(), *UCreatureRankSettings::GetRankName(Rank), Last->Level,
				Health ? Health->GetMaxHealth() : 0.f, Last->AttackDamage, Last->XPReward, bChase ? TEXT(", hunting the player") : TEXT(""));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs SpawnPlaceholderCommand(
		TEXT("Looter.SpawnPlaceholder"),
		TEXT("Looter.SpawnPlaceholder <Name> [rank] [count] [chase]: placeholder creatures (BP_Placeholder_<Name>) in front of the player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnPlaceholder));
}

#endif
