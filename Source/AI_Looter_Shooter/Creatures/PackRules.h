#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Templates/Function.h"

/**
 * How a pack behaves between the first sighting and the last creature down (Docs/Polish/BorderlandsComparison.md, item 8),
 * and how a patrol walks and an ambush springs, as plain functions the tests check without a level
 * (UCreaturePackComponent, APatrolSpawner and AAmbushSpawner play them):
 *  - flanking: a pack chasing one player spreads its approaches over angles, each spiraling in on its own side, instead
 *    of running at them in a line;
 *  - breaking off: a wounded Basic creature whose pack is gone may run for a moment before it turns again;
 *  - the "high rank appears" sting, once a life for a Restless or better creature;
 *  - a patrol's walk along its route and its file behind the leader;
 *  - an ambush's trigger: the player walking onto its ground, not set down there (a respawn grave, a load).
 */
namespace PackRules
{
	// --- Flanking ---

	/** A creature's bearing round a target, seen from above (degrees, -180 to 180): the angle of Me less Target. */
	AI_LOOTER_SHOOTER_API float BearingAround(const FVector& Target, const FVector& Me);

	/** How far either side of the middle a pack of PackSize spreads its approaches (degrees): StepDegrees a member, at most MaxDegrees. */
	AI_LOOTER_SHOOTER_API float FlankHalfSpread(int32 PackSize, float StepDegrees, float MaxDegrees);

	/**
	 * Each chaser's flank (degrees), in the order of BearingsDegrees: their bearings round the target ranked about their
	 * mean (TieBreaks, one each, order chasers on the same bearing, so every chaser ranks the pack alike), the one at the
	 * low end gets -HalfSpread, the one at the high end +HalfSpread, the rest evenly between. A pack of one gets 0.
	 * Positive turns a chaser so its bearing round the target grows (FlankDirection): each swings out on its own side.
	 */
	AI_LOOTER_SHOOTER_API TArray<float> FlankOffsets(const TArray<float>& BearingsDegrees, const TArray<uint32>& TieBreaks, float StepDegrees,
		float MaxDegrees);

	/**
	 * The way (flat, unit) a chaser at Me runs at Target with a flank of FlankDegrees: straight at it within
	 * ReleaseDistance (or with no flank), else turned that far off the straight line, so its bearing round the target grows
	 * (a negative flank: shrinks). Followed every moment, it's a spiral that closes in from its own side.
	 */
	AI_LOOTER_SHOOTER_API FVector FlankDirection(const FVector& Me, const FVector& Target, float FlankDegrees, float ReleaseDistance);

	// --- Breaking off ---

	/** What the decision to break off looks at. */
	struct FRetreatAsk
	{
		ECreatureRank Rank = ECreatureRank::Basic;
		/** Its kind may break off at all (UEncounterSettings::RetreatKinds: the spiders and slimes; the dead never do). */
		bool bAllowedKind = true;
		/** Its health against its most (0-1). */
		float HealthShare = 1.f;
		/** It had packmates near it in this life, and how many are near and alive now. */
		bool bHadPack = false;
		int32 PackmatesLeft = 0;
		/** It rolled for it already this life: one roll, whatever came of it. */
		bool bAlreadyRolled = false;
		/** At or under this share of its health it may break off, with Chance (0-1). */
		float HealthBelow = 0.35f;
		float Chance = 0.5f;
	};

	/** Whether everything but the dice says it may break off now: Basic, of a kind that does, hurt enough, its pack gone, no roll yet. */
	AI_LOOTER_SHOOTER_API bool WantsRetreatRoll(const FRetreatAsk& Ask);

	/** Whether it breaks off now, with Roll (uniform 0-1) its one roll for this life. */
	AI_LOOTER_SHOOTER_API bool ShouldRetreat(const FRetreatAsk& Ask, float Roll);

	/**
	 * Where it runs while it breaks off: Distance straight away from Target; when that leaves its hunting ground
	 * (IsOnGround), bent toward Home, so it never runs off the ground it fights on.
	 */
	AI_LOOTER_SHOOTER_API FVector RetreatGoal(const FVector& Me, const FVector& Target, const FVector& Home, float Distance,
		TFunctionRef<bool(const FVector&)> IsOnGround);

	// --- The rank sting ---

	/** Whether a creature turning on a player sounds the "high rank appears" sting: Restless or better, not a boss (its own bar and show), its tag shown, not yet this life. */
	AI_LOOTER_SHOOTER_API bool ShouldRankSting(ECreatureRank Rank, bool bShowsTag, bool bAlreadyStung);

	// --- Patrols ---

	/** Where a patrol is on its route: how far along it (cm from the first point), which way it walks, and the rest left at an end. */
	struct FPatrolWalk
	{
		float Along = 0.f;
		float Direction = 1.f;
		float PauseLeft = 0.f;
	};

	/** The route's length (cm, seen from above): its legs, and the leg back to its start when it's a loop. */
	AI_LOOTER_SHOOTER_API float RouteLength(const TArray<FVector>& Route, bool bLoop);

	/** The point Along cm down the route, and the way that leg runs (OutLegDirection, flat unit, from the first point onward). */
	AI_LOOTER_SHOOTER_API FVector PointAlong(const TArray<FVector>& Route, bool bLoop, float Along, FVector* OutLegDirection = nullptr);

	/**
	 * Walks Walk on by Speed for DeltaSeconds: a loop goes round and rests PauseSeconds at its first point each time; an
	 * open route walks there and back, resting at each end. True when it moved (not resting, not a route of one point).
	 */
	AI_LOOTER_SHOOTER_API bool AdvancePatrol(const TArray<FVector>& Route, bool bLoop, float Speed, float DeltaSeconds, float PauseSeconds,
		FPatrolWalk& Walk);

	/** Which way the patrol faces now (flat unit): along its leg, the way it walks. */
	AI_LOOTER_SHOOTER_API FVector PatrolHeading(const TArray<FVector>& Route, bool bLoop, const FPatrolWalk& Walk);

	/**
	 * Where the Index-th of a patrol walks (0 the leader, on Point): the rest in a staggered file behind it, Spacing apart,
	 * alternately right and left of the leader's line, turned with Heading.
	 */
	AI_LOOTER_SHOOTER_API FVector FormationSpot(const FVector& Point, const FVector& Heading, int32 Index, float Spacing);

	// --- Ambushes ---

	/**
	 * Whether the player walked onto an ambush's ground since the last look: inside now, outside then, and no farther
	 * from where they were than MaxStep (a respawn or a load sets them down farther). The first look (bSeenBefore off)
	 * never springs it: a player standing inside as it comes on (Main 4 turned in at the vestry door) walked in before.
	 */
	AI_LOOTER_SHOOTER_API bool IsWalkIn(bool bSeenBefore, bool bWasInside, bool bInside, const FVector& Before, const FVector& Now, float MaxStep);
}
