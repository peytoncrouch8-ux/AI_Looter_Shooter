// AEggSac: the fall to the ground under it, the burst, and the spiders it lets out.

#include "World/EggSac.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "World/WorldQueries.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	/**
	 * Where the burst sac's spiders come out without its model's sockets: Sink.py's SOCKET_Spawn_1 and _2 (Blender
	 * (0.62, -1.55, 0) and (-0.7, -1.45, 0), turned -26 and 30 degrees), in the actor's frame (cm, degrees).
	 */
	const FTransform FallbackSpawns[] = {
		FTransform(FRotator(0.0, 26.0, 0.0), FVector(155.0, -62.0, 0.0)),
		FTransform(FRotator(0.0, -30.0, 0.0), FVector(145.0, 70.0, 0.0)),
	};

	/** The ground under a sac is looked for this far below its pivot (cm): the Sink is 12 m deep. */
	constexpr double GroundSearch = 3000.0;

	/** A spider's spot is looked for on the ground this far above and below the burst sac's socket (cm). */
	constexpr double SpotSearch = 150.0;
}

// ---------------------------------------------------------------------------
// The fall
// ---------------------------------------------------------------------------

TOptional<double> AEggSac::FindGroundUnder(const FVector& Point, double Above, double Below) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return TOptional<double>();
	}
	// World-static only, as creatures look for the ground: the floor and the blocks on it, never itself or a volume.
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("EggSacGround"), this);
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Point + FVector(0.0, 0.0, Above), Point - FVector(0.0, 0.0, Below),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return TOptional<double>(Hit.ImpactPoint.Z);
	}
	return TOptional<double>();
}

FVector AEggSac::GetLandingSpot() const
{
	// Under the sac where it hangs (its pivot is its bottom), out along its front by BurstOut.
	const FTransform Hanging = SacRest * GetActorTransform();
	const FVector Pivot = bRestCaptured ? Hanging.GetLocation() : Sac->GetComponentLocation();
	const FVector Out = FRotator(0.0, GetActorRotation().Yaw, 0.0).Vector() * BurstOut;
	const TOptional<double> Ground = FindGroundUnder(Pivot + Out, 10.0, GroundSearch);
	return FVector(Pivot.X + Out.X, Pivot.Y + Out.Y, Ground.IsSet() ? Ground.GetValue() : Pivot.Z - DropHeight);
}

void AEggSac::StartFall()
{
	CaptureRest();
	State = EEggSacState::Falling;
	FallStart = Sac->GetComponentLocation();
	LandZ = GetLandingSpot().Z;
	Fallen = 0.0;
	FallSpeed = 0.0;
	// It drops through nothing: no shots or bumps on the way down.
	SetSolid(*Sac, false);
	SetActorTickEnabled(true);
	if (FallStart.Z - LandZ <= 1.0)
	{
		Land(/*bHatch*/ true);
	}
}

void AEggSac::Advance(float DeltaSeconds)
{
	if (State != EEggSacState::Falling)
	{
		return;
	}
	const double Drop = FMath::Max(FallStart.Z - LandZ, 0.0);
	FallSpeed += FallGravity * FMath::Max(DeltaSeconds, 0.f);
	Fallen = FMath::Min(Fallen + FallSpeed * FMath::Max(DeltaSeconds, 0.f), Drop);
	const double Share = Drop > 0.0 ? Fallen / Drop : 1.0;
	// Straight down, tipping forward out of its sling or off its silk as it goes.
	const FRotator Tip(-FallTip * static_cast<float>(Share), 0.f, 0.f);
	Sac->SetWorldLocationAndRotation(FallStart - FVector(0.0, 0.0, Fallen),
		(GetActorTransform().GetRotation() * Tip.Quaternion()) * SacRest.GetRotation());
	if (Fallen >= Drop)
	{
		Land(/*bHatch*/ true);
	}
}

void AEggSac::Land(bool bHatch)
{
	SetActorTickEnabled(false);
	State = EEggSacState::Burst;
	Sac->SetVisibility(false);
	SetSolid(*Sac, false);
	// The hatched sac on the ground under it, lying along the actor's front, its torn mouth away from the wall.
	Burst->SetWorldLocationAndRotation(GetLandingSpot(), FRotator(0.0, GetActorRotation().Yaw, 0.0));
	Burst->SetVisibility(true);
	SetSolid(*Burst, true);
	if (!bHatch)
	{
		return;
	}
	Hatch(FindTarget());
	UE_LOG(LogLooter, Log, TEXT("%s: the sac bursts on the ground and lets out %d spiders."), *GetActorNameOrLabel(), GetSpiders().Num());
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(BurstEvent, this));
	}
}

// ---------------------------------------------------------------------------
// Its spiders
// ---------------------------------------------------------------------------

TArray<FTransform> AEggSac::GetSpawnSpots() const
{
	TArray<FTransform> Spots;
	const FTransform Lying = Burst->GetComponentTransform();
	const bool bSockets = Burst->GetStaticMesh() != nullptr;
	for (int32 Index = 0; Index < SpawnSockets.Num(); ++Index)
	{
		if (bSockets && Burst->DoesSocketExist(SpawnSockets[Index]))
		{
			Spots.Add(Burst->GetSocketTransform(SpawnSockets[Index]));
		}
		else if (Index < static_cast<int32>(UE_ARRAY_COUNT(FallbackSpawns)))
		{
			Spots.Add(FallbackSpawns[Index] * Lying);
		}
	}
	return Spots;
}

APawn* AEggSac::FindTarget() const
{
	if (const AController* By = ShotBy.Get())
	{
		if (APawn* Pawn = By->GetPawn())
		{
			return Pawn;
		}
	}
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	return Encounters ? Encounters->GetPlayer() : nullptr;
}

void AEggSac::Hatch(APawn* Target)
{
	UWorld* World = GetWorld();
	if (!World || !SpiderClass)
	{
		return;
	}
	ACreatureBase::FRuntimeSpawn Setup;
	Setup.Rank = SpiderRank;
	Setup.BodyScale = SpiderScale;
	for (const FTransform& Spot : GetSpawnSpots())
	{
		// On the ground at the socket (the floor may dip or rise a little under the burst sac's front).
		FVector Feet = Spot.GetLocation();
		if (const TOptional<double> Ground = FindGroundUnder(Feet, SpotSearch, SpotSearch); Ground.IsSet())
		{
			Feet.Z = Ground.GetValue();
		}
		ACreatureBase* Spider = ACreatureBase::SpawnAtRuntime(World, SpiderClass, Feet, static_cast<float>(Spot.Rotator().Yaw), Setup);
		if (!Spider)
		{
			continue;
		}
		// In a level that isn't playing yet (a test level), it starts as play would start it.
		if (!Spider->HasActorBegunPlay())
		{
			Spider->DispatchBeginPlay();
		}
		if (SpiderGround.IsSet())
		{
			Spider->HuntingGround = SpiderGround;
		}
		for (const FName& Tag : SpiderTags)
		{
			if (!Tag.IsNone())
			{
				Spider->Tags.AddUnique(Tag);
			}
		}
		if (Target)
		{
			// Refused, as any call is, when the player isn't on their ground (up the ramp, on the rim) or is in a safe zone.
			Spider->AlertTo(Target);
		}
		Spiders.Add(Spider);
	}
}

TArray<ACreatureBase*> AEggSac::GetSpiders() const
{
	TArray<ACreatureBase*> Out;
	for (const TWeakObjectPtr<ACreatureBase>& Each : Spiders)
	{
		if (ACreatureBase* Spider = Each.Get())
		{
			Out.Add(Spider);
		}
	}
	return Out;
}
