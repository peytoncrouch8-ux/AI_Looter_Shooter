#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Creatures/EncounterRules.h"

class AActor;
class UWorld;
struct FHitResult;

/**
 * The ground under candidate spots in a level, for EncounterRules::ChooseSpots: where an encounter spawner's creatures
 * and the Gravemother's brood come up. A spot's ground is the first world-static surface within a step (and a little
 * more) of home's level. A creature can stand on it when it's walkable and not on top of something standing on the
 * ground (tagged Obstacle: a rock, a wagon, a roof); the one exception is the floor inside the very obstacle home stands
 * on, under its own roof: the Gravemother's den is carved in Den Rock, and its floor is the rock's. The room over a spot
 * is measured for Body: the whole of it, or the largest smaller body that fits there, down to a man's width.
 */
struct AI_LOOTER_SHOOTER_API FEncounterGroundProbe
{
	/** Probes for Body's room, ignoring Ignored (the spawner, or her), with spots no more than MaxStep above or below home. */
	FEncounterGroundProbe(const UWorld* InWorld, const AActor* Ignored, FName TraceTag, const FEncounterBody& InBody, float MaxStep);

	/**
	 * Finds home's ground under From (looked for from Above over it to Below under it): its height, and the obstacle home
	 * stands in (what it stands on, or else what roofs it: the den's rock). Without any ground there (a test level), home
	 * is level with From and every spot stands level with it.
	 */
	bool FindHome(const FVector& From, double Above, double Below);

	bool HasGround() const { return bHasGround; }
	double GetHomeZ() const { return HomeZ; }

	/** What the ground under Spot (seen from above) is like: found, standable, and the room over it. */
	FEncounterGroundHit Look(const FVector& Spot) const;

private:
	/** The nearest obstacle over Point (a rock's arch over a lair at its lip), or null. */
	const AActor* FindObstacleOver(const FVector& Point) const;

	/** The ground Hit found is inside home's own obstacle: a floor of it with its roof over it, not a top of it. */
	bool IsInsideHome(const FHitResult& Ground) const;

	/** The share of Body that fits over Ground: 1 for the whole of it, less for a smaller one, 0 for none. */
	float MeasureRoom(const FVector& Ground) const;

	const UWorld* World = nullptr;
	FCollisionQueryParams GroundParams;
	FCollisionQueryParams RoomParams;
	FEncounterBody Body;
	/** How far above and below home's level a spot's ground is looked for: a step and a metre more. */
	double Reach = 0.0;
	double HomeZ = 0.0;
	bool bHasGround = false;
	/** What home stands on, when it's an obstacle (Den Rock under the Gravemother's lair). */
	const AActor* HomeObstacle = nullptr;
};
