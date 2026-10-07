#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Templates/Function.h"
#include "Templates/SubclassOf.h"

class ACreatureBase;
class UAreaDefinition;
struct FEncounterGroup;

/** What the ground under a candidate spot is like (EncounterRules::ChooseSpots). */
struct FEncounterGroundHit
{
	/** Ground was found under the spot, within reach. */
	bool bFound = false;

	/** Where feet go on it. */
	FVector Point = FVector::ZeroVector;

	/**
	 * A creature can stand there: walkable, and not on top of something standing on the ground (a rock, a wagon, a roof).
	 * The floor inside the obstacle its spawner stands in (a den in a rock) is ground (FEncounterGroundProbe).
	 */
	bool bStandable = true;

	/**
	 * The room over it for the body its creatures need (FEncounterBody): 1 when the whole body fits, a share of its size
	 * when only a smaller body does, 0 when none does (inside rock or a wall).
	 */
	float Room = 1.f;
};

/** A creature's body, for the room a spot needs: its capsule at the size it will be (cm). */
struct FEncounterBody
{
	/** A man-sized walking body: the room every spot needs when there's no creature class to measure. */
	float Radius = 40.f;
	float HalfHeight = 80.f;
};

/**
 * An encounter spawner's rules as plain functions, so the tests check them without a level (AEncounterSpawner plays
 * them): where its creatures may stand, how many may spawn under the caps, their ranks, and when its waves come.
 */
namespace EncounterRules
{
	/**
	 * Candidate spots for Wanted creatures, level with Center: each of Points (world) in turn, then rings round them a
	 * Spacing out, then two; without points, an even spread over a disc of Radius round Center (a sunflower's, from the
	 * middle out), turned StartDegrees. More than Wanted, so the ones ChooseSpots turns down leave enough.
	 */
	AI_LOOTER_SHOOTER_API TArray<FVector> CandidateSpots(const FVector& Center, float Radius, const TArray<FVector>& Points,
		int32 Wanted, float Spacing, float StartDegrees);

	/**
	 * Up to Wanted spots from Candidates, in order: ground under it (GroundAt) that a creature can stand on, no more than
	 * MaxStep above or below HomeZ (the spawner's ground: no group is split by a change of level), not Blocked (a safe zone,
	 * off the playable area or the group's hunting ground, too near the player), and at least Spacing from each other and
	 * from Occupied (creatures standing there already). Spots with room for the whole body come first; if too few have it,
	 * the roomiest of the rest fill in (in candidate order among equals, so a spawner's own points first), never one with
	 * no room at all. The spots returned are on the ground.
	 */
	AI_LOOTER_SHOOTER_API TArray<FVector> ChooseSpots(const TArray<FVector>& Candidates, int32 Wanted, double HomeZ, float MaxStep,
		float Spacing, const TArray<FVector>& Occupied, TFunctionRef<FEncounterGroundHit(const FVector&)> GroundAt,
		TFunctionRef<bool(const FVector&)> IsBlocked);

	/**
	 * The largest of the rank sizes (UCreatureRankSettings) Group's creatures can roll: its fixed rank's, or the biggest
	 * that Basic, its own chances or Area's promotions give.
	 */
	AI_LOOTER_SHOOTER_API float LargestRankSize(const FEncounterGroup& Group, const UAreaDefinition* Area);

	/**
	 * Class's body as SpawnAtRuntime sizes it: its class default's capsule at BodyScale (0: the class's own) times
	 * RankSize. Without a class, a man-sized body.
	 */
	AI_LOOTER_SHOOTER_API FEncounterBody CreatureBody(TSubclassOf<ACreatureBase> Class, float BodyScale, float RankSize);

	/**
	 * The room a spawner's spots need: the widest and the tallest of the bodies its Groups bring, each at its group's size
	 * and the largest rank it can roll (the Gravemother's, nearly three times a man's width). Without any creature class, a
	 * man-sized body.
	 */
	AI_LOOTER_SHOOTER_API FEncounterBody LargestBody(const TArray<FEncounterGroup>& Groups, const UAreaDefinition* Area);

	/** Room under a cap: how many more may be alive (Cap less Alive, never below 0); without a cap (0), as many as asked. */
	AI_LOOTER_SHOOTER_API int32 Room(int32 Alive, int32 Cap);

	/**
	 * A rank for one of Group's creatures from Roll (uniform in 0 to 1): its fixed one, its own chances (Gravebound first,
	 * then Restless), or Area's promotions (Basic without an area).
	 */
	AI_LOOTER_SHOOTER_API ECreatureRank PickRank(const FEncounterGroup& Group, const UAreaDefinition* Area, float Roll);

	/** Whether Group joins wave Wave (counted from 1). */
	AI_LOOTER_SHOOTER_API bool JoinsWave(const FEncounterGroup& Group, int32 Wave);

	/** How many waves a spawner plays: NumWaves; with none, as many as its total allows (MaxTotal); with neither, one. */
	AI_LOOTER_SHOOTER_API int32 PlannedWaves(int32 NumWaves, int32 MaxTotal);

	/** Whether another wave may still come: waves left, and room left under the total (TotalQueued of MaxTotal so far). */
	AI_LOOTER_SHOOTER_API bool HasWavesLeft(int32 WavesStarted, int32 NumWaves, int32 TotalQueued, int32 MaxTotal);

	/**
	 * Whether the next wave comes now on its own: waves left, and WaveClock at WaveInterval or past it. With bWaitForClear
	 * only once none are left (Remaining: alive or still to come), the clock counting from then. With neither an interval
	 * nor a wait for the clear, later waves come only when something triggers them.
	 */
	AI_LOOTER_SHOOTER_API bool IsWaveDue(bool bWavesLeft, float WaveClock, float WaveInterval, bool bWaitForClear, int32 Remaining);
}
