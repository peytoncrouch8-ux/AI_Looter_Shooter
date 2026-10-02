#pragma once

#include "CoreMinimal.h"

struct FPlayableBoundary;

/** Why fall recovery brought a player back. */
enum class EFallRecoveryReason : uint8
{
	/** They left the playable area over an open edge (the Rim, a ledge, the deck, a falls) and dropped below it. */
	OffOpenEdge,
	/** They fell a long way below their last safe spot: the floating islands' rule, and the backstop on the ground. */
	LongFall,
};

/**
 * One player's fall recovery bookkeeping: their last safe spot, and how they left the playable area. Plain data in and
 * a decision out, with no world, so the rules can be tested (Looter.World.FallRecovery); UFallRecoverySubsystem feeds
 * it every frame and does the bringing back.
 *
 * Safe spots are where the player stood on walkable ground, a few times a second, and with a playable area only
 * inside it. Without one (the tutorial island) the only rule is a long fall. With one, a player who left it over an
 * open edge comes back as soon as they have dropped a little below their last safe spot (there's nothing below an open
 * edge to land on), and the long fall stays as the backstop everywhere else.
 */
struct AI_LOOTER_SHOOTER_API FFallRecoveryTracker
{
	/** The rules' numbers (cm, seconds). */
	struct FRules
	{
		/** A fall this far below the last safe spot brings the player back anywhere. */
		float LongDrop = 3000.f;
		/** Past an open edge of the playable area, this drop is enough. */
		float OpenEdgeDrop = 500.f;
		/** How often the safe spot is taken while standing on safe ground. */
		float SampleInterval = 0.5f;
	};

	/**
	 * Feeds one frame: where the player is, which way they look, and whether they stand on walkable ground. Boundary
	 * is the level's playable area, or null without one. Returns why to bring them back to the safe spot now, or unset.
	 */
	TOptional<EFallRecoveryReason> Update(float DeltaTime, const FVector& Location, const FRotator& ViewRotation,
		bool bOnWalkableGround, const FPlayableBoundary* Boundary, const FRules& Rules);

	/** The player was just put back on the safe spot (inside the area). */
	void MarkRecovered();

	bool HasSafeSpot() const { return bHasSafeSpot; }
	const FVector& GetSafeLocation() const { return SafeLocation; }
	const FRotator& GetSafeRotation() const { return SafeRotation; }

	/** Whether the player is outside the playable area (never without one). */
	bool IsOutside() const { return bOutside; }

	/** The playable area's edge the player left it over; INDEX_NONE while inside, or if they were never inside. */
	int32 GetExitEdge() const { return ExitEdge; }

private:
	/** Whether the way out was an open edge: the edge they crossed, or (leaving at a corner, drifting while falling) the one they're nearest now. */
	bool LeftOverOpenEdge(const FPlayableBoundary& Boundary, const FVector2D& Where) const;

	FVector SafeLocation = FVector::ZeroVector;
	FRotator SafeRotation = FRotator::ZeroRotator;
	bool bHasSafeSpot = false;
	float SampleTimer = 0.f;

	/** Where the player last was inside the playable area (XY). */
	FVector2D LastInside = FVector2D::ZeroVector;
	bool bHasLastInside = false;
	bool bOutside = false;
	int32 ExitEdge = INDEX_NONE;
};
