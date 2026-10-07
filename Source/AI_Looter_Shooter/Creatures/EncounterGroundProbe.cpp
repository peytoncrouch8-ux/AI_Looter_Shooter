#include "Creatures/EncounterGroundProbe.h"
#include "World/WorldQueries.h"
#include "CollisionShape.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** Ground steeper than about 50 degrees (the creatures' walkable floor angle) is no place to stand. */
	constexpr double WalkableGroundNormalZ = 0.64;

	/** What stands on the ground (rocks, wagons, buildings) carries this tag for the minimap: nothing appears on top of one. */
	const FName StandingObstacleTag(TEXT("Obstacle"));

	/**
	 * The room's capsule is lifted off its ground by this share of its radius, so a walkable slope (up to 50 degrees) under
	 * it doesn't read as no room: 30 cm under the 40 cm body every spot once needed.
	 */
	constexpr float RoomLiftShare = 0.75f;

	/**
	 * A body that doesn't fit whole is measured smaller, in RoomSizes even steps down to this width (cm, a man's: the room
	 * every spot once needed), or not at all when it's no wider: a small creature needs the whole of its own.
	 */
	constexpr float SmallestRoomRadius = 40.f;
	constexpr int32 RoomSizes = 3;

	/** A roof of home's own obstacle this far (cm) over a spot puts the spot inside it (a den in a rock), not on top of it. */
	constexpr double InsideRoofReach = 1000.0;

	/** The roof is looked for from this far over the ground (cm): clear of the floor's own collision. */
	constexpr double RoofLookFrom = 10.0;
}

FEncounterGroundProbe::FEncounterGroundProbe(const UWorld* InWorld, const AActor* Ignored, FName TraceTag, const FEncounterBody& InBody,
	float MaxStep)
	: World(InWorld)
	// The ground's traces skip volumes and what the PCG volume scattered; the room check sees everything that stops a
	// walking body, the scattered trees and rocks too.
	, GroundParams(LooterWorld::StaticGeometryParams(InWorld, TraceTag, Ignored))
	, RoomParams(SCENE_QUERY_STAT(EncounterRoom), false, Ignored)
	, Body(InBody)
	, Reach(static_cast<double>(MaxStep) + 100.0)
{
	Body.Radius = FMath::Max(Body.Radius, 1.f);
	Body.HalfHeight = FMath::Max(Body.HalfHeight, Body.Radius);
}

bool FEncounterGroundProbe::FindHome(const FVector& From, double Above, double Below)
{
	HomeZ = From.Z;
	bHasGround = false;
	HomeObstacle = nullptr;
	FHitResult Hit;
	if (World && World->LineTraceSingleByObjectType(Hit, From + FVector(0.0, 0.0, Above), From - FVector(0.0, 0.0, Below),
		FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams))
	{
		bHasGround = true;
		HomeZ = Hit.ImpactPoint.Z;
		const AActor* Under = Hit.GetActor();
		// What home stands on, or else what roofs it: at the den's lip the Sink's floor and the den's (Den Rock's) lie within
		// a few centimetres of each other, so the lair may find either under it, but the rock's arch is over it either way.
		HomeObstacle = Under && Under->ActorHasTag(StandingObstacleTag) ? Under : FindObstacleOver(Hit.ImpactPoint);
	}
	return bHasGround;
}

const AActor* FEncounterGroundProbe::FindObstacleOver(const FVector& Point) const
{
	// Every surface over it, nearest first: a cocoon hung under the den's arch is no obstacle, and doesn't hide the arch.
	TArray<FHitResult> Over;
	const FVector From = Point + FVector(0.0, 0.0, RoofLookFrom);
	World->LineTraceMultiByObjectType(Over, From, From + FVector(0.0, 0.0, InsideRoofReach), FCollisionObjectQueryParams(ECC_WorldStatic),
		GroundParams);
	for (const FHitResult& Each : Over)
	{
		const AActor* Above = Each.GetActor();
		if (!Each.bStartPenetrating && Above && Above->ActorHasTag(StandingObstacleTag))
		{
			return Above;
		}
	}
	return nullptr;
}

FEncounterGroundHit FEncounterGroundProbe::Look(const FVector& Spot) const
{
	FEncounterGroundHit Found;
	if (!bHasGround || !World)
	{
		// No ground anywhere near home (a test level): every spot stands level with it.
		Found.bFound = true;
		Found.Point = FVector(Spot.X, Spot.Y, HomeZ);
		return Found;
	}
	// The ground within reach of home's level: one more than a step up or down isn't found at all.
	FHitResult Hit;
	if (!World->LineTraceSingleByObjectType(Hit, FVector(Spot.X, Spot.Y, HomeZ + Reach), FVector(Spot.X, Spot.Y, HomeZ - Reach),
		FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams))
	{
		return Found;
	}
	const AActor* Under = Hit.GetActor();
	const bool bOnObstacle = Under && Under->ActorHasTag(StandingObstacleTag) && !IsInsideHome(Hit);
	Found.bFound = true;
	Found.Point = Hit.ImpactPoint;
	Found.bStandable = Hit.ImpactNormal.Z >= WalkableGroundNormalZ && !bOnObstacle;
	// Room is measured only where a creature could stand at all.
	Found.Room = Found.bStandable ? MeasureRoom(Hit.ImpactPoint) : 0.f;
	return Found;
}

bool FEncounterGroundProbe::IsInsideHome(const FHitResult& Ground) const
{
	// Only home's own obstacle, and only under its roof: the den's floor in the rock her lair stands in, never the top of a
	// boulder of the same rock out in the open, nor another rock or a roof.
	if (!HomeObstacle || Ground.GetActor() != HomeObstacle)
	{
		return false;
	}
	// Against the obstacle's own collision only: a cocoon or a lantern hung under its roof doesn't hide the roof.
	FHitResult Roof;
	const FVector From = Ground.ImpactPoint + FVector(0.0, 0.0, RoofLookFrom);
	return HomeObstacle->ActorLineTraceSingle(Roof, From, From + FVector(0.0, 0.0, InsideRoofReach), ECC_WorldStatic, GroundParams)
		&& !Roof.bStartPenetrating;
}

float FEncounterGroundProbe::MeasureRoom(const FVector& Ground) const
{
	// The whole body first, then smaller ones evenly down to a man's width: the first that fits is the room there. A spot
	// a man fits beside a den's jamb isn't one for the Gravemother, nearly three times as wide.
	const float Smallest = FMath::Min(SmallestRoomRadius, Body.Radius);
	const int32 Sizes = Smallest < Body.Radius ? RoomSizes : 1;
	for (int32 Size = 0; Size < Sizes; ++Size)
	{
		const float Radius = Sizes > 1
			? FMath::Lerp(Body.Radius, Smallest, static_cast<float>(Size) / static_cast<float>(Sizes - 1)) : Body.Radius;
		const float Share = Radius / Body.Radius;
		const float HalfHeight = FMath::Max(Body.HalfHeight * Share, Radius);
		const FVector Middle = Ground + FVector(0.0, 0.0, RoomLiftShare * Radius + HalfHeight);
		if (!World->OverlapBlockingTestByChannel(Middle, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight),
			RoomParams))
		{
			return Share;
		}
	}
	return 0.f;
}
