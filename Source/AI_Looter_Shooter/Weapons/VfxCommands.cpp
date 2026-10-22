// Developer console commands for the code-drawn effects (VFX-2, 2026-10-22; not in shipping builds): a rocket's blast or a
// rifle round's hit where the player's view meets the world, once or on a repeat, so a look can be grabbed the same way
// twice and a performance capture (Tools\perf.ps1 -Exec runs once, before the capture) always has a blast going.
//   Looter.Vfx.Blast [seconds | off]   a rocket's blast (WeaponRocketBlast::SpawnEffects) at the aim; again every <seconds>
//   Looter.Vfx.Impact [scale] [surface]  a round's hit on what the aim meets, in its own look (1 a rifle's, 0.6 a pellet's) or
//                                       the named surface's (Stone, Dirt, Metal...)

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WeaponFX.h"
#include "Weapons/WeaponRocketBlast.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
	/** How far the aim is followed (cm). */
	constexpr float VfxAimReach = 20000.f;

	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* VfxGameWorld(UWorld* World)
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

	/** Where the first player's view meets the world (false: nothing in reach, or no player). */
	bool VfxAim(UWorld& World, FHitResult& OutHit, FVector& OutDirection)
	{
		APlayerController* Controller = World.GetFirstPlayerController();
		if (!Controller)
		{
			return false;
		}
		FVector Eye;
		FRotator View;
		Controller->GetPlayerViewPoint(Eye, View);
		OutDirection = View.Vector();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(VfxCommandAim), true, Controller->GetPawn());
		return World.LineTraceSingleByChannel(OutHit, Eye, Eye + OutDirection * VfxAimReach, ECC_Visibility, Params);
	}

	void VfxBlastNow(UWorld* World)
	{
		FHitResult Hit;
		FVector Direction;
		if (!World || !VfxAim(*World, Hit, Direction))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Vfx.Blast: the aim meets nothing within %.0f m."), VfxAimReach / 100.f);
			return;
		}
		const FVector Up = FVector(Hit.ImpactNormal).GetSafeNormal();
		WeaponRocketBlast::SpawnEffects(*World, Hit.ImpactPoint + Up * 5.f, Up, &Hit);
		UE_LOG(LogLooter, Log, TEXT("Looter.Vfx.Blast: at %s (surface %d)."), *Hit.ImpactPoint.ToCompactString(),
			static_cast<int32>(ImpactSurface::Of(Hit)));
	}

	FTimerHandle VfxBlastTimer;
	TWeakObjectPtr<UWorld> VfxBlastWorld;

	void VfxBlast(const TArray<FString>& Args, UWorld* InWorld)
	{
		if (UWorld* Old = VfxBlastWorld.Get())
		{
			Old->GetTimerManager().ClearTimer(VfxBlastTimer);
		}
		VfxBlastWorld.Reset();
		if (Args.Num() > 0 && Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Vfx.Blast: off."));
			return;
		}
		UWorld* World = VfxGameWorld(InWorld);
		if (!World)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Vfx.Blast: no level being played."));
			return;
		}
		const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.f;
		if (Seconds <= 0.f)
		{
			VfxBlastNow(World);
			return;
		}
		VfxBlastWorld = World;
		World->GetTimerManager().SetTimer(VfxBlastTimer, FTimerDelegate::CreateLambda([World]() { VfxBlastNow(World); }), FMath::Max(Seconds, 0.25f),
			true, 0.5f);
		UE_LOG(LogLooter, Log, TEXT("Looter.Vfx.Blast: every %.1f s at the aim (Looter.Vfx.Blast off stops it)."), Seconds);
	}

	void VfxImpact(const TArray<FString>& Args, UWorld* InWorld)
	{
		UWorld* World = VfxGameWorld(InWorld);
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		FHitResult Hit;
		FVector Direction;
		if (!Bullets || !VfxAim(*World, Hit, Direction))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Vfx.Impact: no level being played, or the aim meets nothing."));
			return;
		}
		const float Scale = Args.Num() > 0 ? FMath::Clamp(FCString::Atof(*Args[0]), 0.2f, 2.f) : 1.f;
		EImpactMaterial Material = ImpactSurface::Of(Hit);
		// A surface named second (Stone, Dirt...) is shown instead of what the aim meets: its look on any ground.
		EImpactMaterial Named;
		if (Args.Num() > 1 && ImpactSurface::FromTag(FName(*(FString(TEXT("Surface.")) + Args[1])), Named))
		{
			Material = Named;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		// What it throws lands on the surface it struck (a wall's chunks would want the ground under it: near enough here).
		const FPlane Floor(Hit.ImpactPoint, FVector(Hit.ImpactNormal));
		Effects.SpawnSurfaceImpact(Hit.ImpactPoint, Hit.ImpactNormal, Direction, Material, Scale, false, Hit.ImpactNormal.Z > 0.6 ? &Floor : nullptr);
		UE_LOG(LogLooter, Log, TEXT("Looter.Vfx.Impact: surface %d at %s, %.0f m away."), static_cast<int32>(Material), *Hit.ImpactPoint.ToCompactString(),
			Hit.Distance / 100.f);
	}

	FAutoConsoleCommandWithWorldAndArgs VfxBlastCommand(TEXT("Looter.Vfx.Blast"),
		TEXT("A rocket's blast where the view's aim meets the world; Looter.Vfx.Blast <seconds> repeats it, Looter.Vfx.Blast off stops."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&VfxBlast));
	FAutoConsoleCommandWithWorldAndArgs VfxImpactCommand(TEXT("Looter.Vfx.Impact"),
		TEXT("A round's hit where the view's aim meets the world, in that surface's look: [scale, 1 a rifle's] [surface, e.g. Stone]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&VfxImpact));
}

#endif
