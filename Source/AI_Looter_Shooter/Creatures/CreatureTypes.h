#pragma once

// The small types ACreatureBase hands out and takes (its state, steering sizes, footprint, runtime spawn setup, the hunting
// and viewer checks), kept apart so CreatureBase.h stays about the class. The class names each of them again as a nested alias
// (ACreatureBase::FFootprint, ::FRuntimeSpawn, ::FSteerProbes), so code reads them as it always did.

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "CreatureTypes.generated.h"

UENUM(BlueprintType)
enum class ECreatureState : uint8
{
	Idle,     // standing around near home
	Wander,   // strolling to a random spot near home
	Chase,    // closing in on its target
	Attack,   // wind-up, strike, recovery
	Return,   // gave up its hunt, walking home
	Search,   // still after its target but not chasing: tracking where it lost them, sniffing there, or watching from its edge
	Dead
};

/** What its steering looks ahead with (FCreatureSteerProbe), at its size (ACreatureBase::GetSteerProbes). */
struct FCreatureProbeSizes
{
	/** A capsule this wide (radius) swept this far ahead finds what's in its way. */
	float SweepLength = 0.f;
	float SweepRadius = 0.f;
	/** It looks for ground this far ahead, at most this far below its middle: it never walks off a higher drop. */
	float LedgeDistance = 0.f;
	float LedgeDrop = 0.f;
	/** The swept capsule's half height, and its middle against the body's: from a little off the ground to under its top. */
	float SweepHalfHeight = 0.f;
	float SweepLift = 0.f;
	/** The highest step it walks up: anything lower isn't in its way. */
	float StepHeight = 0.f;
};

/**
 * The ground its body covers from above, at size 1: a flat capsule along its facing from Front to Back (cm from its middle)
 * and Radius wide, which other creatures keep clear of (ACreatureBase::GetFootprint). The capsule's circle by default.
 */
struct FCreatureFootprint
{
	float Front = 0.f;
	float Back = 0.f;
	float Radius = 0.f;
};

/** How a creature spawned in play (by a spawner, an egg sac or a command) starts (ACreatureBase::SpawnAtRuntime). */
struct FCreatureRuntimeSpawn
{
	ECreatureRank Rank = ECreatureRank::Basic;
	/**
	 * Its level before its rank's, kept whatever the area's band. 0: as a placed creature's, from the levels where it stands
	 * in its area (UAreaDefinition), or the class's where the area has none.
	 */
	int32 Level = 0;
	/** Its BodyScale; 0 keeps the class's. */
	float BodyScale = 0.f;
	/** Its health against its class's, before its level and rank multiply it (the test boss is twelve spiders tough). */
	float HealthScale = 1.f;
};

/** Whether a player is prey for a creature (CreatureBaseHunting.cpp). */
enum class ECreatureHuntCheck : uint8
{
	Huntable,
	/** Not a living player character (dead, a creature, nothing). */
	NotPrey,
	/** Alive and out of every safe zone, but off its hunting ground. */
	OffGround,
	/** In a safe zone that's on. */
	Sheltered,
};

/** How the local players see a creature, for picking its update rate (CreatureBaseUpdateRate.cpp). */
struct FCreatureViewerMeasure
{
	/**
	 * Distance (cm) to the nearest local player's pawn or camera, divided by the camera's zoom when it's in that camera's
	 * zoomed view (a scope shows it that much nearer); the largest float when there is no player.
	 */
	float Distance = TNumericLimits<float>::Max();
	/** In a local player's view, or drawn on screen lately. Behind a wall or a rock still counts. */
	bool bOnScreen = false;
};
