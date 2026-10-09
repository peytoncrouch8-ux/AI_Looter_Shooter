#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"

class ACharacter;
class UWorld;

/**
 * The queries the player's own movement makes, for the traversal probe (FTraversalProbe): the capsule's channel and
 * responses, with the player and whatever it carries left out, so every probe sees exactly what stops the body.
 */
struct FTraversalBodyQuery
{
	explicit FTraversalBodyQuery(ACharacter& Character);

	bool Sweep(FHitResult& Hit, const FVector& From, const FVector& To, const FCollisionShape& Shape) const;

	/** Whether anything is in the way from one point to another (no way at all: whether the shape fits where it is). */
	bool Blocked(const FVector& From, const FVector& To, const FCollisionShape& Shape) const;

	/** Whether a straight line between two points meets anything. */
	bool LineBlocked(const FVector& From, const FVector& To) const;

	/** The player's own capsule, a little narrower so grazing what it touches now doesn't count. */
	FCollisionShape Body(float Shrink) const;

	/** Inside the level's playable area (any place, on a level that has none). */
	bool Inside(const FVector& Point) const;

	/** How far over the floor the movement keeps a walking capsule (the middle of its band), so a move ends where it would. */
	static float FloorGap();

	UWorld* World = nullptr;
	FCollisionQueryParams Params;
	FCollisionResponseParams Response;
	ECollisionChannel Channel = ECC_Pawn;
	float Radius = 0.f;
	float HalfHeight = 0.f;
};
