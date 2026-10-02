#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/Function.h"
#include "MissionTargets.generated.h"

class UWorld;

/**
 * Which actors in the level an objective is about: actors of a class (and its children), actors carrying a tag, or both.
 * Missions are data, so they never point at a placed actor directly: level actors are named by tags (Speaker_Delia,
 * Poster, Boss_Abel), which survive rebuilding a level, and kinds of things by class (any spider, the weapon rack).
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FMissionActorFilter
{
	GENERATED_BODY()

	/** Actors of this class or a child of it; none: any class. */
	UPROPERTY(EditAnywhere, Category = "Mission")
	TSubclassOf<AActor> ActorClass;

	/** Actors carrying this tag; None: any. */
	UPROPERTY(EditAnywhere, Category = "Mission")
	FName ActorTag;

	/** A class or a tag is set: there is something to look for (an empty filter matches nothing). */
	bool IsSet() const { return ActorClass.Get() != nullptr || !ActorTag.IsNone(); }

	bool Matches(const AActor* Actor) const;

	/** What a line says about them when its objective has no words of its own: "Spider Creature", "Poster". */
	FString Describe() const;
};

/** A place to be: the nearest actor of a filter (a door, the weapon rack), or a spot, and how close counts as there. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FMissionPlace
{
	GENERATED_BODY()

	/** An actor to go to: the nearest one this filter matches. Not set: Location. */
	UPROPERTY(EditAnywhere, Category = "Mission")
	FMissionActorFilter Actor;

	/** Where the place is (world, cm), when no actor is named. */
	UPROPERTY(EditAnywhere, Category = "Mission")
	FVector Location = FVector::ZeroVector;

	/** How close counts as there (cm). */
	UPROPERTY(EditAnywhere, Category = "Mission", meta = (ClampMin = "0"))
	float Radius = 500.f;

	/** Measured on the map, height left out (as the tutorial measures): a roof over the spot counts as there. */
	UPROPERTY(EditAnywhere, Category = "Mission")
	bool bIgnoreHeight = true;

	/** The place's middle as seen from From: the nearest matching actor, or Location. Unset when its actor isn't in the level. */
	TOptional<FVector> Resolve(const UWorld* World, const TOptional<FVector>& From) const;

	/** Where is inside the place whose middle is Middle. */
	bool Contains(const FVector& Middle, const FVector& Where) const;

	/** Where is inside the place (the nearest of its actors to it, or its spot). */
	bool ContainsInWorld(const UWorld* World, const FVector& Where) const;
};

/** Finding an objective's actors in the level. */
namespace MissionTargets
{
	/** Calls Visit for every actor in World the filter matches (none for an empty filter), skipping ones being destroyed. */
	void ForEach(const UWorld* World, const FMissionActorFilter& Filter, TFunctionRef<void(AActor&)> Visit);

	/** The level has at least one actor the filter matches. */
	bool Any(const UWorld* World, const FMissionActorFilter& Filter);

	/**
	 * The matching actor nearest From, measured on the map (height doesn't count), or the first one found without From.
	 * With bLivingOnly, dead ones are skipped (a creature waiting to come back). Null when none.
	 */
	AActor* FindNearest(const UWorld* World, const FMissionActorFilter& Filter, const TOptional<FVector>& From, bool bLivingOnly);

	/** The middle of every matching actor (a training ground's dummies); unset when there are none. */
	TOptional<FVector> FindCenter(const UWorld* World, const FMissionActorFilter& Filter);

	/** It isn't dead: a creature by its state, anything else with health by its health; anything without health always. */
	bool IsAlive(const AActor& Actor);
}
