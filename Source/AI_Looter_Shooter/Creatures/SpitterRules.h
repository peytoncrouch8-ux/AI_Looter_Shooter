#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"

/** What the Spitter does about the distance to its target while it hunts (ASpitterSpider; SpitterRules::ChooseKeep). */
enum class ESpitterKeep : uint8
{
	/** Not hunting, or too far off: its brain's own chase brings it in. */
	Close,
	/** In its band: it stands and watches its target, between strafes. */
	Hold,
	/** In its band: it sidesteps round its target, still facing it. */
	Strafe,
	/** Too close: it backs away, still facing its target. */
	BackOff,
	/** Too close with no way back: it stands its ground and bites whatever comes within reach. */
	Cornered,
	/** In its band: it walks round its target to its fight spot (UFightDirectorSubsystem), still facing it. */
	Reposition,
};

/**
 * The Spitter's rules as plain functions (ASpitterSpider, ASpitterGlob, ASpitterPuddle), so the tests check them without a
 * level: the lob's arc, where it aims, what it does about its distance, when it spits or bites, its rank's traits, and
 * whom a puddle burns.
 */
namespace SpitterRules
{
	/**
	 * The pull on a glob in flight (cm/s^2, downward): half again the world's, so a lob arcs high enough to read against the
	 * sky and comes down steep, onto the player rather than skimming the ground at them.
	 */
	inline constexpr float GlobGravity = 1800.f;

	/** A lob's flight (s) for a throw Distance (cm, flat): a short toss is quick, a long one hangs, never under or over these. */
	inline constexpr float ShortestFlight = 0.7f;
	inline constexpr float LongestFlight = 1.45f;
	AI_LOOTER_SHOOTER_API float FlightSeconds(float Distance);

	/** The launch velocity that carries a lob from Start to Target in Seconds, under Gravity (cm/s^2, downward). */
	AI_LOOTER_SHOOTER_API FVector LobVelocity(const FVector& Start, const FVector& Target, float Seconds, float Gravity = GlobGravity);

	/** Where a lob launched from Start at Velocity is After seconds. */
	AI_LOOTER_SHOOTER_API FVector LobPoint(const FVector& Start, const FVector& Velocity, float After, float Gravity = GlobGravity);

	/**
	 * Where it aims a lob that flies Seconds: its target's Feet led along their flat Velocity for LeadShare of the flight. Half
	 * a lead punishes standing still or running straight, and a player who turns or keeps strafing sees it land behind them.
	 */
	AI_LOOTER_SHOOTER_API FVector AimPoint(const FVector& Feet, const FVector& Velocity, float Seconds, float LeadShare);

	/**
	 * What it does at Distance (cm, flat) from its target: past CloseBeyond its brain brings it closer; within BackOffWithin it
	 * backs away while a way back is clear (bWayBack), else it's cornered; between the two it holds, or strafes while
	 * bStrafing (its own rhythm of sidesteps and pauses).
	 */
	AI_LOOTER_SHOOTER_API ESpitterKeep ChooseKeep(float Distance, float BackOffWithin, float CloseBeyond, bool bWayBack, bool bStrafing);

	/** Whether a spit may start: ready, and its target between MinDistance and MaxDistance (cm, flat). */
	AI_LOOTER_SHOOTER_API bool CanSpit(float Distance, float MinDistance, float MaxDistance, bool bReady);

	/**
	 * Whether it bites instead: only up close (within BiteFrom) with the bite ready, and only when it's caught (its target
	 * within CaughtWithin, too near to back away from) or cornered. Otherwise it keeps its distance.
	 */
	AI_LOOTER_SHOOTER_API bool WantsBite(float Distance, float BiteFrom, float CaughtWithin, bool bCornered, bool bReady);

	/** Globs one spit throws at a rank: a Gravebound one (or better) throws three, side by side. */
	AI_LOOTER_SHOOTER_API int32 GlobsFor(ECreatureRank Rank);

	/** Its time between spits at a rank, against a Basic one's: a Restless one (or better) spits a fifth sooner. */
	AI_LOOTER_SHOOTER_API float IntervalShareFor(ECreatureRank Rank);

	/**
	 * Whether a puddle Radius round Center burns a body standing with its Feet there: its feet within the puddle's edge plus
	 * half the body's radius (a toe in it counts, brushing past doesn't), and on the puddle's level (a player on a ledge over
	 * it, or jumping over it, isn't burned).
	 */
	AI_LOOTER_SHOOTER_API bool PuddleTouches(const FVector& Center, float Radius, const FVector& Feet, float BodyRadius);

	/** A puddle reaches this far above and below its middle (cm): the ground's own bumps, and a step up out of it. */
	inline constexpr float PuddleBelow = 40.f;
	inline constexpr float PuddleAbove = 60.f;

	/**
	 * How a puddle Seconds long shows Age seconds in (0 to 1): it spreads out in its first fifth of a second, holds, and
	 * dries up over its last 0.8 s. Its burn lasts as long as it shows at all.
	 */
	AI_LOOTER_SHOOTER_API float PuddleSpread(float Age, float Seconds);
}
