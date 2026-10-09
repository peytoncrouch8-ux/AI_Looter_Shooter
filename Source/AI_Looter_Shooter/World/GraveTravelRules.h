#pragma once

#include "CoreMinimal.h"

/** Why fast travel can't go now (GraveTravelRules), in the order the rules ask. None: it can. */
enum class EGraveTravelBlock : uint8
{
	None,
	/** No living character to move (dead and gone, a spectator). */
	NoPlayer,
	/** The player is down: the death's fade is running. */
	Dying,
	/** A scene is playing or holding the player (a ride, the cold open, a boss's entrance). */
	Scene,
	/** A boss's fight is on. */
	BossFight,
	/** Something is hunting the player. */
	Hunted,
	/** A trip to another level, or a fast travel, is already under way. */
	Travelling,
	/** The grave isn't open in this story. */
	GraveClosed,
	/** The player is standing at it already. */
	AlreadyThere,
};

/** What the rules look at, gathered from the world (UGraveTravelSubsystem::Sense) or made up by a test. */
struct AI_LOOTER_SHOOTER_API FGraveTravelSenses
{
	bool bHasPlayer = false;
	bool bDying = false;
	bool bInScene = false;
	bool bBossFight = false;
	/** Creatures alive with the player as their target. */
	int32 Hunters = 0;
	bool bTravelling = false;
};

/**
 * Fast travel between open respawn graves as plain rules, apart from the world so the tests drive them: when it may go
 * (never in a fight, a scene or a boss's fight), what the map says when it can't, how long its fade takes, and where the
 * player stands at the other end (as a death's wake-up puts them: on the grave's spot, facing its arrow).
 */
namespace GraveTravelRules
{
	/** Closer than this to a grave (cm, across the ground) the player is there already. */
	inline constexpr double AlreadyThereDistance = 800.0;

	/** The fade to black, the black held while the player is moved, and the fade back in (seconds). */
	inline constexpr float FadeOutSeconds = 0.45f;
	inline constexpr float HoldSeconds = 0.2f;
	inline constexpr float FadeInSeconds = 0.7f;

	/** Whether the player may travel at all now, before a grave is chosen: the first block that holds, or None. */
	AI_LOOTER_SHOOTER_API EGraveTravelBlock CheckNow(const FGraveTravelSenses& Senses);

	/** Whether the player may travel to a grave: CheckNow first, then the grave itself (open, and not where they stand). */
	AI_LOOTER_SHOOTER_API EGraveTravelBlock CheckGrave(const FGraveTravelSenses& Senses, bool bGraveOpen, double DistanceToGrave);

	/** The map's words for a block ("Something is hunting you..."); empty for None. */
	AI_LOOTER_SHOOTER_API FText Reason(EGraveTravelBlock Block);

	/** Where the player's middle goes for a grave whose spot is on the ground: half their height above it. */
	AI_LOOTER_SHOOTER_API FVector ArrivalLocation(const FVector& GraveSpot, float HalfHeight);
}
