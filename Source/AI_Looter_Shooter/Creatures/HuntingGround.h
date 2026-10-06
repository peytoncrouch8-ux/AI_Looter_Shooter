#pragma once

#include "CoreMinimal.h"
#include "HuntingGround.generated.h"

/**
 * Where a creature fights: its group's own level of ground (Docs/Areas/RansomsRest.md, "Encounters stay on one level of
 * ground"). It hunts only players standing on it, and a target who leaves it ends the chase: the creature lets go and
 * walks home (ACreatureBase, CreatureBaseHunting.cpp). Seen from above it is a radius round a middle or a polygon (a
 * fence's line, a pit's floor), and a height band round the middle keeps a pit's floor from reaching up to its rim. A
 * spawner gives the creatures it spawns its own (AEncounterSpawner); a placed creature can be given one in the level (the
 * bluff spiders, on the top of Ransom's Point). An unset hunting ground holds everywhere: a creature without one hunts
 * as creatures always have, as far as its LoseInterestRadius.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FHuntingGround
{
	GENERATED_BODY()

	/** How far it reaches round its middle (cm, seen from above). 0, with fewer than three Corners: no hunting ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunting Ground", meta = (ClampMin = "0", Units = "cm"))
	float Radius = 0.f;

	/**
	 * It as a polygon instead of the radius (world X and Y in cm, corners in order either way round; Z is ignored): the
	 * churchyard's iron fence, the barn yard's. Three corners or more replace the radius.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunting Ground")
	TArray<FVector> Corners;

	/** This far past the polygon's edge (cm) still counts as on it, so a fight along a fence doesn't stop and start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunting Ground", meta = (ClampMin = "0", Units = "cm"))
	float Margin = 200.f;

	/**
	 * A player more than this (cm) above or below its middle isn't on it: the rim over the Sink's floor, a ledge over a
	 * yard. 0: any height.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunting Ground", meta = (ClampMin = "0", Units = "cm"))
	float MaxRise = 0.f;

	/** Its middle is the creature's home (where it stood as play began). Off: Center, a spawner's spot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunting Ground")
	bool bAroundHome = true;

	/** Its middle (world, cm) when it isn't round the creature's home. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunting Ground", meta = (EditCondition = "!bAroundHome"))
	FVector Center = FVector::ZeroVector;

	/** It has a radius or a polygon: the creature keeps to it. */
	bool IsSet() const;

	/** Its middle: Home (the creature's) or Center. */
	FVector GetMiddle(const FVector& Home) const;

	/**
	 * Whether Point stands on it: inside the polygon or its margin (or within the radius of its middle), and no more than
	 * MaxRise above or below the middle. Home is the creature's home. An unset hunting ground holds everywhere.
	 */
	bool Contains(const FVector& Point, const FVector& Home) const;

	/** Whether a spot is well on it, seen from above: inside the polygon itself, or the radius. Where a spawner may put a creature. */
	bool ContainsSpot(const FVector& Point, const FVector& Home) const;

	/** A spawner's: InRadius round InCenter, or the polygon when InCorners has three corners or more. */
	static FHuntingGround MakeAround(const FVector& InCenter, float InRadius, const TArray<FVector>& InCorners, float InMargin,
		float InMaxRise);

	/** Whether Point lies inside a polygon's corners seen from above (a point exactly on an edge may go either way). */
	static bool IsInsidePolygon(const TArray<FVector>& Polygon, const FVector2D& Point);

	/** How far Point lies from a polygon's nearest edge seen from above (cm); the largest double without an edge. */
	static double DistanceToPolygonEdge(const TArray<FVector>& Polygon, const FVector2D& Point);
};
