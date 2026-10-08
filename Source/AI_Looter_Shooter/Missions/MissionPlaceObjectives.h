#pragma once

#include "CoreMinimal.h"
#include "Missions/MissionObjective.h"
#include "MissionPlaceObjectives.generated.h"

// The objectives about where the player is: reaching a place, walking a distance, holding out somewhere.

/** Go somewhere: done once the player is inside the place (near its actor, or its spot). */
UCLASS(BlueprintType, meta = (DisplayName = "Reach a place"))
class AI_LOOTER_SHOOTER_API UMissionReachObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Objective")
	FMissionPlace Place;

	virtual void Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const override;
	virtual FMissionActorFilter GetTargets() const override { return Place.Actor; }
	virtual TOptional<FVector> GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const override;
	virtual FString DescribeRule() const override;
};

/** Walk a distance from where the player stood as it began (the tutorial's first step: moving at all). */
UCLASS(BlueprintType, meta = (DisplayName = "Travel a distance"))
class AI_LOOTER_SHOOTER_API UMissionTravelObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	/** How far, in cm, measured on the map. */
	UPROPERTY(EditAnywhere, Category = "Objective", meta = (ClampMin = "0"))
	float Distance = 600.f;

	virtual void Begin(const FMissionContext& Context, FMissionObjectiveState& State) const override;
	virtual void Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const override;
	virtual FString DescribeRule() const override;
};

/**
 * Hold out for a time: the clock runs while the player is inside the place (or anywhere, without bStayInside) and stops
 * while they're out of it, keeping what was held. Missions can't be failed, so leaving only pauses it.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Defend for a time"))
class AI_LOOTER_SHOOTER_API UMissionDefendObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Objective")
	FMissionPlace Place;

	/** How long, in seconds. */
	UPROPERTY(EditAnywhere, Category = "Objective", meta = (ClampMin = "1"))
	float HoldSeconds = 30.f;

	/** The clock runs only while the player is inside the place. */
	UPROPERTY(EditAnywhere, Category = "Objective")
	bool bStayInside = true;

	virtual int32 GetRequired() const override;
	virtual void Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const override;
	virtual FMissionActorFilter GetTargets() const override { return Place.Actor; }
	virtual TOptional<FVector> GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const override;
	virtual FString FormatProgress(const FMissionObjectiveState& State) const override;
	virtual bool CountsSeconds() const override { return true; }
	virtual FString DescribeRule() const override;
};
