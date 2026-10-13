// Console commands for checking what bullets hit and the marks they leave (not in shipping builds):
//   Looter.Impact.Probe          what's under the crosshair (100 m): its surface, how it was found, the cue and the mark
//   Looter.BulletMarks.Stats     the marks showing and the decals in the pool
//   Looter.BulletMarks.Clear     hides every mark
// The marks' settings are console variables: Looter.BulletMarks 0/1, Looter.BulletMarks.Max, .Hold and .Fade (seconds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/BulletMarks.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/ImpactSurfaceMap.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

namespace
{
	/** The bullets' trace channel (Weapon, DefaultEngine.ini; AWeaponBase::TraceChannel). */
	constexpr ECollisionChannel ImpactProbeChannel = ECC_GameTraceChannel2;
	constexpr float ImpactProbeReach = 10000.f;

	/** The game world a command is for: the one it was typed in, or the running play-in-editor session. */
	UWorld* ImpactCommandWorld(UWorld* World)
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

	void ProbeImpact(const TArray<FString>& Args, UWorld* InWorld)
	{
		UWorld* World = ImpactCommandWorld(InWorld);
		APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		if (!Player || !Player->PlayerCameraManager)
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Impact.Probe: play the game (or a session in the editor) first."));
			return;
		}
		const FVector Eye = Player->PlayerCameraManager->GetCameraLocation();
		const FVector End = Eye + Player->PlayerCameraManager->GetCameraRotation().Vector() * ImpactProbeReach;
		// As a bullet traces: complex, with the face and the physical material.
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ImpactProbe), /*bTraceComplex*/ true);
		Params.bReturnPhysicalMaterial = true;
		Params.bReturnFaceIndex = true;
		Params.AddIgnoredActor(Player->GetPawn());
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, Eye, End, ImpactProbeChannel, Params))
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Impact.Probe: nothing within %.0f m."), ImpactProbeReach / 100.f);
			return;
		}
		EImpactSurfaceSource Source = EImpactSurfaceSource::None;
		const EImpactMaterial Material = ImpactSurface::Resolve(Hit, Source);
		const UPrimitiveComponent* Component = Hit.GetComponent();
		int32 Section = 0;
		const UMaterialInterface* Face = Component && Hit.FaceIndex != INDEX_NONE ? Component->GetMaterialFromCollisionFaceIndex(Hit.FaceIndex, Section) : nullptr;
		FVector2D UV = FVector2D::ZeroVector;
		const bool bUV = ImpactSurface::UVAtHit(Hit, UV);
		const UImpactSurfaceMapData* Map = UImpactSurfaceMapData::FindOn(Face);
		const FBulletMarkLook Look = FBulletMarks::LookOf(Material);
		UE_LOG(LogLooter, Display, TEXT("Looter.Impact.Probe: %s (%s, %s) %.1f m away, normal %s; material %s (face %d), physical %s; UV %s; surface map %s"),
			*GetNameSafe(Hit.GetActor()), *GetNameSafe(Component),
			Component ? (Component->Mobility == EComponentMobility::Movable ? TEXT("movable") : TEXT("static")) : TEXT("-"),
			Hit.Distance / 100.f, *Hit.ImpactNormal.ToCompactString(), *GetNameSafe(Face), Hit.FaceIndex, *GetNameSafe(Hit.PhysMaterial.Get()),
			bUV ? *UV.ToString() : TEXT("not kept"), Map ? TEXT("yes") : TEXT("no"));
		UE_LOG(LogLooter, Display, TEXT("    %s, from %s; plays %s (its own: %s); %s"), ImpactSurface::Name(Material), ImpactSurface::SourceName(Source),
			*ImpactSurface::PlayableCue(World, Material).ToString(), *ImpactSurface::Cue(Material).ToString(),
			FBulletMarks::CanMark(Hit, Material) ? *FString::Printf(TEXT("a mark of style %d, %.1f cm across for a rifle"), Look.Style, Look.Radius * 2.f)
				: TEXT("no mark (water, or a thing that moves, or one that takes no decals)"));
		DrawDebugPoint(World, Hit.ImpactPoint, 12.f, FColor::Orange, false, 5.f);
	}

	UBulletSubsystem* ImpactBullets(UWorld* World)
	{
		UWorld* Game = ImpactCommandWorld(World);
		UBulletSubsystem* Bullets = Game ? Game->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			UE_LOG(LogLooter, Display, TEXT("No bullets: play the game (or a session in the editor) first."));
		}
		return Bullets;
	}

	void ShowMarks(const TArray<FString>& Args, UWorld* World)
	{
		if (UBulletSubsystem* Bullets = ImpactBullets(World))
		{
			UE_LOG(LogLooter, Display, TEXT("Bullet marks: %d showing, %d decals in the pool."),
				Bullets->GetMarks().NumLive(Bullets->GetWorld()->GetTimeSeconds()), Bullets->GetMarks().NumDecals());
		}
	}

	void ClearMarks(const TArray<FString>& Args, UWorld* World)
	{
		if (UBulletSubsystem* Bullets = ImpactBullets(World))
		{
			Bullets->GetMarks().Clear();
			UE_LOG(LogLooter, Display, TEXT("Bullet marks cleared."));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ImpactProbeCommand(TEXT("Looter.Impact.Probe"),
		TEXT("What's under the crosshair: its surface, how it was found, the impact's cue and the mark it would take."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ProbeImpact));
	FAutoConsoleCommandWithWorldAndArgs MarksStatsCommand(TEXT("Looter.BulletMarks.Stats"), TEXT("The bullet marks showing and the pool's decals."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowMarks));
	FAutoConsoleCommandWithWorldAndArgs MarksClearCommand(TEXT("Looter.BulletMarks.Clear"), TEXT("Hides every bullet mark."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ClearMarks));
}

#endif
