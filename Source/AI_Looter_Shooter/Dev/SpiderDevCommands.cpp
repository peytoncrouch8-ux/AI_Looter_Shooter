// Developer console commands for the spiders' ambush (2026-10-31; not in shipping builds), for PIE checks:
//   Looter.Spider.LurkHere [count]  brown spiders lurking on the wall, rock or tree the player looks at (within 40 m), spread
//                               2.5 m apart along it (a floor or a rock's top: crouched on it); they spring as the player
//                               passes, stares, lingers or shoots, as a lurk ambush's do (no ambush of their own: no mates).
//   Looter.Spider.Spring        every lurking spider springs at the player now.
//   Looter.Spider.Lurkers       logs every spider in an ambush: where it is in it, and where it hangs.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
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
	/** How far the player's look finds a surface (cm), and how far apart the spiders hang along it. */
	constexpr float LookReach = 4000.f;
	constexpr float SpiderSpacing = 250.f;

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

	const TCHAR* PhaseName(ESpiderLurkPhase Phase)
	{
		switch (Phase)
		{
		case ESpiderLurkPhase::Perched: return TEXT("perched");
		case ESpiderLurkPhase::Coiling: return TEXT("coiling");
		case ESpiderLurkPhase::Leaping: return TEXT("leaping");
		case ESpiderLurkPhase::Climbing: return TEXT("climbing");
		case ESpiderLurkPhase::Landing: return TEXT("landing");
		case ESpiderLurkPhase::DenHidden: return TEXT("in its den");
		case ESpiderLurkPhase::DenCrawl: return TEXT("crawling out of its den");
		case ESpiderLurkPhase::DenTurn: return TEXT("tipping out of its den");
		default: return TEXT("none");
		}
	}

	void LurkHere(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = GameWorldOf(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Player = Controller ? Controller->GetPawn() : nullptr;
		if (!Player)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Spider.LurkHere: no player (play in editor first)."));
			return;
		}
		const int32 Count = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1, 8) : 1;
		FVector EyeAt;
		FRotator EyeTurn;
		Controller->GetPlayerViewPoint(EyeAt, EyeTurn);
		FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(GameWorld, TEXT("SpiderLurkHere"), Player);
		FHitResult Hit;
		if (!GameWorld->LineTraceSingleByObjectType(Hit, EyeAt, EyeAt + EyeTurn.Vector() * LookReach, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Spider.LurkHere: look at a wall, a rock or a tree within %.0f m."), LookReach / 100.f);
			return;
		}
		const FVector Normal = Hit.ImpactNormal;
		const ESpiderSurface Surface = SpiderAmbushRules::SurfaceOf(Normal);
		const bool bLedge = Surface == ESpiderSurface::Ledge || Surface == ESpiderSurface::None;
		const FVector Out = FVector(Normal.X, Normal.Y, 0.0).GetSafeNormal(UE_SMALL_NUMBER, -EyeTurn.Vector().GetSafeNormal2D());
		const FVector Along = FVector::CrossProduct(FVector::UpVector, Out).GetSafeNormal();
		int32 Made = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector At = FVector(Hit.ImpactPoint) + Along * ((Index - (Count - 1) * 0.5f) * SpiderSpacing);
			// Spawned on the ground before it (as a lurk ambush brings its spiders out), then up onto its perch.
			FHitResult Floor;
			const FVector Probe = At + Out * 200.f;
			if (!GameWorld->LineTraceSingleByObjectType(Floor, Probe + FVector(0.0, 0.0, 300.0), Probe - FVector(0.0, 0.0, 3000.0),
				FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				continue;
			}
			ACreatureBase::FRuntimeSpawn Spawn;
			Spawn.bMayBeTouched = false;
			ASpiderCreature* Spider = Cast<ASpiderCreature>(ACreatureBase::SpawnAtRuntime(GameWorld, ASpiderCreature::StaticClass(), Floor.ImpactPoint,
				static_cast<float>(Out.Rotation().Yaw), Spawn));
			if (!Spider)
			{
				continue;
			}
			FSpiderPerchSpot Perch;
			Perch.Kind = bLedge ? ESpiderPerchKind::Ledge : ESpiderPerchKind::Wall;
			Perch.Spot = bLedge ? At : At + Out * 30.f;
			Perch.FaceYaw = static_cast<float>(Out.Rotation().Yaw);
			Spider->BeginLurk(Perch, nullptr);
			++Made;
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Spider.LurkHere: %d spider(s) lurking on the %s at (%.0f, %.0f, %.0f)."), Made, bLedge ? TEXT("ledge") : TEXT("wall"),
			Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z);
	}

	void SpringAll(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = GameWorldOf(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Player = Controller ? Controller->GetPawn() : nullptr;
		if (!Player)
		{
			return;
		}
		int32 Sprung = 0;
		for (TActorIterator<ASpiderCreature> It(GameWorld); It; ++It)
		{
			if (It->GetLurkPhase() == ESpiderLurkPhase::Perched)
			{
				It->DevSpring(Player);
				++Sprung;
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Spider.Spring: %d lurker(s) sprang."), Sprung);
	}

	void ListLurkers(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = GameWorldOf(World);
		if (!GameWorld)
		{
			return;
		}
		int32 Listed = 0;
		for (TActorIterator<ASpiderCreature> It(GameWorld); It; ++It)
		{
			const FSpiderLurkState& Lurk = It->GetLurk();
			if (Lurk.Phase == ESpiderLurkPhase::None)
			{
				continue;
			}
			UE_LOG(LogLooter, Log, TEXT("%s: %s on (%.0f, %.0f, %.0f), its face %s, %.1f m over the ground under it."), *It->GetName(), PhaseName(Lurk.Phase),
				Lurk.Point.X, Lurk.Point.Y, Lurk.Point.Z, *Lurk.Normal.ToCompactString(), (Lurk.Point.Z - Lurk.Foot.Z) / 100.0);
			++Listed;
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Spider.Lurkers: %d in an ambush."), Listed);
	}

	FAutoConsoleCommandWithWorldAndArgs LurkHereCommand(
		TEXT("Looter.Spider.LurkHere"),
		TEXT("Looter.Spider.LurkHere [count]: brown spiders lurking on the wall, rock or tree you look at (within 40 m); they spring as you pass."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LurkHere));

	FAutoConsoleCommandWithWorldAndArgs SpringAllCommand(
		TEXT("Looter.Spider.Spring"),
		TEXT("Looter.Spider.Spring: every lurking spider springs at the player now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpringAll));

	FAutoConsoleCommandWithWorldAndArgs ListLurkersCommand(
		TEXT("Looter.Spider.Lurkers"),
		TEXT("Looter.Spider.Lurkers: logs every spider in an ambush, where it is in it and where it hangs."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListLurkers));
}

#endif
