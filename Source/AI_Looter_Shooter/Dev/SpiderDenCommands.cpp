// Developer console command for the den entrance (2026-10-31; not in shipping builds), for PIE checks without the mission:
//   Looter.Spider.Den.Test [count]  brown spiders crawling out of dens: the nearest den ambush's dens (within 60 m, nearest
//                               the player first) if there is one, else dens in the wall the player looks at (within 40 m,
//                               2.5 m apart along it, the mouth where the look meets it). Each crawls out on its turn
//                               (Looter.Spider.Den.Stagger), climbs down, drops and hunts the player; one whose den has no
//                               wall stands where it was spawned. They belong to no spawner (nothing counts them).
// To spring a placed den ambush itself: Looter.Encounter.Wave <id> force.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Creatures/AmbushSpawner.h"
#include "Creatures/SpiderAmbushRules.h"
#include "Creatures/SpiderCreature.h"
#include "World/WorldQueries.h"
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
	/** A placed den ambush this near (cm) is the one tried; else the player's look finds a wall this far (cm). */
	constexpr float AmbushNear = 6000.f;
	constexpr float LookReach = 4000.f;
	/** Dens made in a looked-at wall are this far apart along it (cm). */
	constexpr float DenSpacing = 250.f;
	/** Each spider is spawned on the floor this far out from its den's wall (cm), as a den ambush's floor spots are. */
	constexpr float FloorOut = 150.f;

	UWorld* GameWorldOf(UWorld* World)
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

	/** The nearest den ambush's dens, nearest the player first; none within AmbushNear: empty. */
	TArray<FSpiderPerchSpot> PlacedDens(UWorld& World, const FVector& PlayerAt)
	{
		const AAmbushSpawner* Nearest = nullptr;
		double Best = FMath::Square(static_cast<double>(AmbushNear));
		for (TActorIterator<AAmbushSpawner> It(&World); It; ++It)
		{
			if (It->Entrance != EAmbushEntrance::Den)
			{
				continue;
			}
			for (const FSpiderPerchSpot& Den : It->Dens)
			{
				const double Distance = FVector::DistSquared(Den.Spot, PlayerAt);
				if (Distance < Best)
				{
					Best = Distance;
					Nearest = *It;
				}
			}
		}
		TArray<FSpiderPerchSpot> Dens = Nearest ? Nearest->Dens : TArray<FSpiderPerchSpot>();
		Dens.Sort([&PlayerAt](const FSpiderPerchSpot& A, const FSpiderPerchSpot& B)
		{
			return FVector::DistSquared(A.Spot, PlayerAt) < FVector::DistSquared(B.Spot, PlayerAt);
		});
		if (Nearest)
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Spider.Den.Test: the dens of %s."), *Nearest->GetName());
		}
		return Dens;
	}

	/** Dens in the wall the player looks at: the mouth where the look meets it, Count of them along it. */
	TArray<FSpiderPerchSpot> LookedAtDens(UWorld& World, APlayerController& Controller, int32 Count)
	{
		TArray<FSpiderPerchSpot> Dens;
		FVector EyeAt;
		FRotator EyeTurn;
		Controller.GetPlayerViewPoint(EyeAt, EyeTurn);
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("SpiderDenTest"), Controller.GetPawn());
		FHitResult Hit;
		if (!World.LineTraceSingleByObjectType(Hit, EyeAt, EyeAt + EyeTurn.Vector() * LookReach, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			return Dens;
		}
		const ESpiderSurface Surface = SpiderAmbushRules::SurfaceOf(Hit.ImpactNormal);
		if (Surface != ESpiderSurface::Wall && Surface != ESpiderSurface::Overhang)
		{
			return Dens;
		}
		const FVector Out = FVector(Hit.ImpactNormal.X, Hit.ImpactNormal.Y, 0.0).GetSafeNormal(UE_SMALL_NUMBER, -EyeTurn.Vector().GetSafeNormal2D());
		const FVector Along = FVector::CrossProduct(FVector::UpVector, Out).GetSafeNormal();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FSpiderPerchSpot& Den = Dens.AddDefaulted_GetRef();
			Den.Kind = ESpiderPerchKind::Wall;
			Den.Spot = FVector(Hit.ImpactPoint) + Along * ((Index - (Count - 1) * 0.5f) * DenSpacing);
			Den.FaceYaw = static_cast<float>(Out.Rotation().Yaw);
		}
		return Dens;
	}

	void DenTest(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = GameWorldOf(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Player = Controller ? Controller->GetPawn() : nullptr;
		if (!Player)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Spider.Den.Test: no player (play in editor first)."));
			return;
		}
		const int32 Count = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1, 8) : 1;
		TArray<FSpiderPerchSpot> Dens = PlacedDens(*GameWorld, Player->GetActorLocation());
		if (Dens.IsEmpty())
		{
			Dens = LookedAtDens(*GameWorld, *Controller, Count);
		}
		if (Dens.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Spider.Den.Test: no den ambush within %.0f m; look at a rock wall within %.0f m."),
				AmbushNear / 100.f, LookReach / 100.f);
			return;
		}
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(GameWorld, TEXT("SpiderDenTest"), Player);
		int32 Out = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FSpiderPerchSpot& Den = Dens[SpiderAmbushRules::DenFor(Index, Dens.Num())];
			const FVector Probe = Den.Spot + SpiderAmbushRules::FaceWay(Den.FaceYaw) * FloorOut;
			FHitResult Floor;
			if (!GameWorld->LineTraceSingleByObjectType(Floor, Probe + FVector(0.0, 0.0, 100.0), Probe - FVector(0.0, 0.0, 3000.0),
				FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				continue;
			}
			ACreatureBase::FRuntimeSpawn Spawn;
			Spawn.bMayBeTouched = false;
			ASpiderCreature* Spider = Cast<ASpiderCreature>(ACreatureBase::SpawnAtRuntime(GameWorld, ASpiderCreature::StaticClass(), Floor.ImpactPoint,
				Den.FaceYaw, Spawn));
			if (!Spider)
			{
				continue;
			}
			if (Spider->BeginDenEmerge(Den, SpiderAmbushRules::DenDelay(Index, FMath::FRand()), nullptr))
			{
				++Out;
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.Spider.Den.Test: no wall or floor at den (%.0f, %.0f, %.0f): %s stands on its floor spot."),
					Den.Spot.X, Den.Spot.Y, Den.Spot.Z, *Spider->GetName());
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Spider.Den.Test: %d of %d spider(s) coming out of %d den(s)."), Out, Count, FMath::Min(Count, Dens.Num()));
	}

	FAutoConsoleCommandWithWorldAndArgs DenTestCommand(
		TEXT("Looter.Spider.Den.Test"),
		TEXT("Looter.Spider.Den.Test [count]: spiders crawl out of the nearest den ambush's dens (within 60 m), else out of the wall you look at."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DenTest));
}

#endif
