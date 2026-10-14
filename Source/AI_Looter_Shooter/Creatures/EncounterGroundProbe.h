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
 * more) of home's level. Under a roof (something over home's ground within that reach: Tilly's cellar's vault) it's looked
 * for from under the roof, never from over it: the workshop's floor and the yard over the cellar are less than a step up,
 * but they're another storey. A creature can stand on it when it's walkable and not on top of something standing on the
 * ground (tagged Obstacle: a rock, a wagon, a roof); the one exception is the floor inside the very obstacle home stands
 * on, under its own roof: the Gravemother's den is carved in Den Rock, and its floor is the rock's. The room over a spot
 * is measured for Body: the whole of it, or the largest smaller body that fits there, down to a man's width.
 */
struct AI_LOOTER_SHOOTER_API FEncounterGroundProbe
{
	/** Probes for Body's room, ignoring Ignored (the spawner, or her), with spots no more than MaxStep above or below home. */
	FEncounterGroundProbe(const UWorld* InWorld, const AActor* Ignored, FName TraceTag, const FEncounterBody& InBody, float MaxStep);

	/**
	 * Finds home's ground under From (looked for from Above over it to Below under it, but never from over a roof over From:
	 * a spawner on a cellar's floor finds that floor, not the top of the vault over it), the obstacle home stands in (what
	 * it stands on, or else what roofs it: the den's rock), and home's roof (HasRoof). Without any ground there (a test
	 * level), home is level with From and every spot stands level with it.
	 */
	bool FindHome(const FVector& From, double Above, double Below);

	bool HasGround() const { return bHasGround; }
	double GetHomeZ() const { return HomeZ; }

	/**
	 * Something stands over home's ground lower than the highest a spot's ground is looked for (a cellar's vault, a drift's
	 * timber): every spot's ground is looked for from under it.
	 */
	bool HasRoof() const { return bHasRoof; }

	/** The underside of home's roof (world cm), when it has one. */
	double GetRoofZ() const { return RoofZ; }

	/** What the ground under Spot (seen from above) is like: found, standable, and the room over it. */
	FEncounterGroundHit Look(const FVector& Spot) const;

private:
	/**
	 * The nearest world-static surface over Point (from just over it) no higher than TopZ, skipping any the look starts
	 * inside (the floor's own slab under a point sunk into it): its height in OutZ. False when there's none.
	 */
	bool FindRoofOver(const FVector& Point, double TopZ, double& OutZ) const;

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
	/** Home's roof: the underside of the nearest surface over home's ground within Reach. */
	double RoofZ = 0.0;
	bool bHasRoof = false;
	/** What home stands on, when it's an obstacle (Den Rock under the Gravemother's lair). */
	const AActor* HomeObstacle = nullptr;
};
