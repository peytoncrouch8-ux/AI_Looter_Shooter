#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Creatures/CreatureSteerPlanner.h"

class ACreatureBase;
class UCharacterMovementComponent;
class UWorld;
struct FHitResult;

/**
 * The world queries a creature's steering makes, set up once per update and shared by its looks (CreatureBaseSteering.cpp)
 * and its unstick (CreatureBaseUnstick.cpp).
 *
 * A look sweeps the body along a direction: a capsule nearly as wide as the creature (ACreatureBase::FSteerProbes) from a
 * little off the ground to just under its top, so a rail under its middle is seen and a low roof over it isn't. What it
 * meets is judged by the surface its feet would meet there (IsWalkableContact), not by the sweep's own normal, which reads
 * a wall's top edge as a slope; a slope or a step it can walk up is looked on past. Then the ground ahead: it never walks
 * off a drop. Other creatures and the player don't count (its pack's spacing and its brain see to those).
 */
struct AI_LOOTER_SHOOTER_API FCreatureSteerProbe
{
	explicit FCreatureSteerProbe(const ACreatureBase& Creature);

	/** Looks along Direction (flat) for up to Distance (cm). */
	FCreatureSteerPlanner::FLookResult Look(const FVector& Direction, float Distance) const;

	/**
	 * Whether the body's sweep along Direction, meeting Hit, met ground it walks on (a slope, a step no higher than its
	 * step), not a wall or a wall's top edge: touched no higher over its feet than a step, and by the surface just past the
	 * touch (walkable, and no higher above the ground just before it than a step). OutSurfaceNormal and OutSurfaceZ: that
	 * surface's normal and height.
	 */
	bool IsWalkableContact(const FHitResult& Hit, const FVector& Direction, FVector* OutSurfaceNormal = nullptr, double* OutSurfaceZ = nullptr) const;

	/**
	 * The nearest spot (the capsule's middle) it can be slid to from where it stands: reached without passing through
	 * anything (never out through a fence, high or low), with room for its body (among creatures and players too) and what
	 * reaches past it (a spider's abdomen), and ground under it that it walks on, in the playable area. It tries the way it
	 * wants to go (Preferred, flat) first.
	 */
	bool FindFreeSpot(const FVector& Preferred, FVector& OutMiddle) const;

	/** The ground under Point, at most Below under it (world-static: terrain and solid props), as ACreatureBase::FindGround. */
	bool FindGround(const FVector& Point, float Below, FVector& OutGround) const;

	UWorld* World = nullptr;
	const UCharacterMovementComponent* Movement = nullptr;
	/** The capsule's middle now, its size, the highest step it walks up and the steepest floor it stands on (normal Z). */
	FVector Middle = FVector::ZeroVector;
	float Radius = 0.f;
	float HalfHeight = 0.f;
	float StepHeight = 0.f;
	float WalkableFloorZ = 0.64f;
	float SizeScale = 1.f;
	/** The look's sweep and the ground it needs ahead (ACreatureBase::FSteerProbes). */
	float SweepRadius = 0.f;
	float SweepHalfHeight = 0.f;
	float SweepLift = 0.f;
	float LedgeDistance = 0.f;
	float LedgeDrop = 0.f;
	/** Where its body reaches past the capsule (a spider's abdomen): its far end from the middle, flat, and its radius; zero radius when it doesn't. */
	FVector FootprintReach = FVector::ZeroVector;
	float FootprintRadius = 0.f;

private:
	/** IsWalkableContact for feet at FeetZ (where they'd be up a slope it looks on along). */
	bool IsWalkableContactFrom(const FHitResult& Hit, const FVector& Direction, double FeetZ, FVector* OutSurfaceNormal, double* OutSurfaceZ) const;
	bool SweepBody(const FVector& From, const FVector& To, float InRadius, FHitResult& OutHit) const;
	bool Trace(const FVector& From, const FVector& To, FHitResult& OutHit) const;

	/** What stops the body (the capsule's own responses), with other creatures and the player left out, and itself and what it carries. */
	FCollisionQueryParams BodyParams;
	FCollisionResponseParams BodyResponse;
	/** Room for its body at a free spot: creatures and players count there. */
	FCollisionQueryParams RoomParams;
	FCollisionResponseParams RoomResponse;
	/** The ground: world-static, volumes and the playable area's walls left out (LooterWorld::StaticGeometryParams). */
	FCollisionQueryParams GroundParams;
	ECollisionChannel Channel = ECC_Pawn;
};
