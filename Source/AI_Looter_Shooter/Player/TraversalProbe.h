#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "Player/TraversalRules.h"

class ACharacter;
class APawn;

/** Why no traversal was found (the log, the debug view and the tests say). */
enum class ETraversalRefusal : uint8
{
	None,
	/** Nothing in front within reach, in the band a ledge's face can be in. */
	NoWall,
	/** It isn't in front: the player goes or looks too far across it. */
	NotFacing,
	/** It's a slope the player can walk up. */
	Walkable,
	/** Its top is under a step (the movement steps up it) or, in the air, about at the feet (the landing takes it). */
	TooLow,
	/** Its top is out of reach, or it goes on up past the probe (a tall wall, the playable area's walls). */
	TooHigh,
	/** It's something never climbed: a creature, loot, a moving or physics-driven thing, a NoClimb actor. */
	NotClimbable,
	/** No top where the body would go. */
	NoTop,
	/** No room for the standing body on top, or overhead on the way up. */
	NoRoom,
	/** Something in the way across the edge. */
	Blocked,
	/** No floor to come down on beyond it (a vault). */
	NoLanding,
	/** It would end outside the playable area. */
	OutsideArea,
};

/** What the locomotion asks the probe: the player, which way they go, and how they move. */
struct FTraversalAsk
{
	ACharacter* Character = nullptr;
	/** The way the player looks (flat, unit). */
	FVector Facing = FVector::ForwardVector;
	/** The way they're going (flat, unit): the keys' way, else the run's, else the look. */
	FVector Heading = FVector::ForwardVector;
	/** In the air (a catch) rather than on the ground (the jump key). */
	bool bInAir = false;
	/** At a run (a sprint): a low wall that can be vaulted is vaulted rather than climbed onto. */
	bool bRunning = false;
	/** The feet's height when last on the ground: in the air a ledge is reached for from there. */
	float LastFloorZ = 0.f;
	/** How much higher a jump still rising will carry the feet on its own: a ledge under that the jump clears itself. */
	float RiseLeft = 0.f;
	/** The speed along the move it ends at: after a vault (the run kept), after a mantle (a step on, or none). */
	float VaultExitSpeed = 0.f;
	float MantleExitSpeed = 0.f;
};

/** What the probe found: a move and its obstacle, or why there's none. */
struct FTraversalFind
{
	ETraversalKind Kind = ETraversalKind::None;
	ETraversalRefusal Refusal = ETraversalRefusal::NoWall;
	/** The way the move goes (flat, unit), and the obstacle's near and far faces along it from the player's axis. */
	FVector Direction = FVector::ForwardVector;
	float NearFace = 0.f;
	float FarFace = 1.e6f;
	/** The obstacle's top (world Z). */
	float TopZ = 0.f;
	/** Where the move ends: the capsule's centre, and the feet's height. */
	FVector End = FVector::ZeroVector;
	float EndFeetZ = 0.f;
	float ExitSpeed = 0.f;
	/** The move's length by the rules (the plan may stretch it). */
	float Duration = 0.f;
	/** The wall met, the top where the hands go and the floor the move ends on (their surfaces say the sounds). */
	FHitResult WallHit;
	FHitResult TopHit;
	FHitResult FloorHit;
};

/**
 * Looks for something to climb or vault in front of the player, with the queries the player's own movement makes (its
 * capsule's channel and responses, the player left out), so it sees exactly what stops them: placed and instanced props,
 * terrain ledges, the playable area's walls (too tall to have a reachable top, so never crossed). A body-wide probe swept
 * ahead finds the face; small balls dropped along the way find the top, how deep it runs and whether there's floor past
 * it; capsule sweeps then check the body's room on top or on the floor beyond and its way there, overhead included.
 */
struct AI_LOOTER_SHOOTER_API FTraversalProbe
{
	/** The mantle or vault in front of the player, or why there's none. */
	static FTraversalFind Find(const FTraversalAsk& Ask);

	/**
	 * Whether something the probe met can be climbed or vaulted: never a pawn (creatures, people), loot, a volume or the
	 * playable area's walls, anything simulating physics or moving, or anything tagged NoClimb (actor or component), and
	 * nothing the movement wouldn't step up onto (CanCharacterStepUp).
	 */
	static bool IsClimbable(const FHitResult& Hit, APawn* Climber);

	/**
	 * A free spot near the character with ground under it it can stand on, reached without passing through anything:
	 * where a wedged player is nudged to. Tries close first, the Preferred way first. False when nothing near is free.
	 */
	static bool FindFreeSpot(ACharacter& Character, const FVector& Preferred, FVector& OutCenter);

	static const TCHAR* RefusalName(ETraversalRefusal Refusal);
};
