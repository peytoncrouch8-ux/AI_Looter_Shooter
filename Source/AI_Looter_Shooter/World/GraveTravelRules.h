#pragma once

#include "CoreMinimal.h"

/** Why fast travel can't go now (GraveTravelRules), in the order the rules ask. None: it can. */
enum class EGraveTravelBlock : uint8
{
	None,
	/** No living character to move (dead and gone, a spectator). */
	NoPlayer,
	/** The player is down: the death's carry is running. */
	Dying,
	/** A scene is playing or holding the player (a ride, the cold open, a boss's entrance). */
	Scene,
	/** A boss's fight is on. */
	BossFight,
	/** Something is coming at the player (chasing or attacking; a creature only searching for them doesn't count). */
	Hunted,
	/** A trip to another level, or a fast travel, is already under way. */
	Travelling,
	/** The roost isn't open in this story (not found yet). */
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
	/** Creatures in a fight with the player: chasing or attacking (ACreatureBase::IsFightingWith), not searching for them. */
	int32 Hunters = 0;
	bool bTravelling = false;
};

/**
 * Fast travel between Hob's open roosts as plain rules, apart from the world so the tests drive them: when it may go
 * (never in a fight, a scene or a boss's fight), what the map says when it can't, how long the crows' carry takes, and where
 * the player stands at the other end (as a death's wake-up puts them: on the roost's spot, facing its arrow). The roosts
 * replaced the respawn graves and kept their rules, so the names still say grave.
 */
namespace GraveTravelRules
{
	/** Closer than this to a roost (cm, across the ground) the player is there already. */
	inline constexpr double AlreadyThereDistance = 800.0;

	/**
	 * The carry (seconds): the crows pour across the screen until it's black, the black held while the player is moved, and
	 * the crows bursting away as the world comes back (URoostCarrySubsystem draws them; the camera fades under them).
	 * Under two seconds in all: a ride, not a wait.
	 */
	inline constexpr float FadeOutSeconds = 0.75f;
	inline constexpr float HoldSeconds = 0.3f;
	inline constexpr float FadeInSeconds = 0.85f;

	/** Whether the player may travel at all now, before a roost is chosen: the first block that holds, or None. */
	AI_LOOTER_SHOOTER_API EGraveTravelBlock CheckNow(const FGraveTravelSenses& Senses);

	/** Whether the player may travel to a roost: CheckNow first, then the roost itself (open, and not where they stand). */
	AI_LOOTER_SHOOTER_API EGraveTravelBlock CheckGrave(const FGraveTravelSenses& Senses, bool bGraveOpen, double DistanceToGrave);

	/** The map's words for a block ("Something is hunting you..."); empty for None. */
	AI_LOOTER_SHOOTER_API FText Reason(EGraveTravelBlock Block);

	/** Where the player's middle goes for a roost whose spot is on the ground: half their height above it. */
	AI_LOOTER_SHOOTER_API FVector ArrivalLocation(const FVector& GraveSpot, float HalfHeight);
}
