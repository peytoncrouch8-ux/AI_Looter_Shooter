#pragma once

#include "CoreMinimal.h"
#include "Missions/MissionObjective.h"
#include "MissionEventObjectives.generated.h"

// The objectives done by something the game's systems tell the mission runner (UMissionRunner::NotifyEvent): an
// interaction, words at a speaker point, a scene that played, boarding the skiff, or any named event. The interaction
// component, the speaker points and the scenes (as each ends) send theirs. Looter.Mission.Event sends any of them from
// the console.

/** A named event happening a number of times ("Bell.Rung"). */
UCLASS(BlueprintType, meta = (DisplayName = "Event"))
class AI_LOOTER_SHOOTER_API UMissionEventObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Objective")
	FName Event;

	UPROPERTY(EditAnywhere, Category = "Objective", meta = (ClampMin = "1"))
	int32 Count = 1;

	virtual int32 GetRequired() const override { return FMath::Max(Count, 1); }
	virtual bool HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const override;
	virtual FString DescribeRule() const override;
};

/**
 * Use things: interact with (or hold Interact on) a number of different actors the filter matches, each counted once
 * (6 posters torn down, 6 hay bales loaded). An event without an actor (the console's) counts every time.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Interact"))
class AI_LOOTER_SHOOTER_API UMissionInteractObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Objective")
	FMissionActorFilter Target;

	/** How many different ones. */
	UPROPERTY(EditAnywhere, Category = "Objective", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** Only a held interaction counts (hold Interact). */
	UPROPERTY(EditAnywhere, Category = "Objective")
	bool bHold = false;

	virtual int32 GetRequired() const override { return FMath::Max(Count, 1); }
	virtual bool HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const override;
	virtual FMissionActorFilter GetTargets() const override { return Target; }
	virtual TOptional<FVector> GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const override;
	virtual FString DescribeRule() const override;
};

/**
 * Talk to someone at a speaker point: the actor carrying SpeakerTag (Delia's screen door, Tilly's shop window, a story
 * character). A speaker point (USpeakerPointComponent) sends a Talk event about the actor it's on when the player talks
 * there; Looter.Mission.Event Talk <tag> stands in for one from the console.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Talk"))
class AI_LOOTER_SHOOTER_API UMissionTalkObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Objective")
	FName SpeakerTag;

	virtual bool HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const override;
	virtual FMissionActorFilter GetTargets() const override;
	virtual FString DescribeRule() const override;
};

/**
 * Watch a scene: done when the scene named Scene has played (it sends Scene.<Scene> as it ends), or had played in this
 * level before the step began (USceneSubsystem::HasPlayed: one that played as the level began, or one the story is past),
 * since its event went out before this listened.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Play a scene"))
class AI_LOOTER_SHOOTER_API UMissionSceneObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Objective")
	FName Scene;

	virtual void Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const override;
	virtual bool HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const override;
	virtual FString DescribeRule() const override;
};

/** Board a vehicle: done when it sends Board.<Vehicle> (the tutorial's skiff, step 10). */
UCLASS(BlueprintType, meta = (DisplayName = "Board"))
class AI_LOOTER_SHOOTER_API UMissionBoardObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Objective")
	FName Vehicle = TEXT("Skiff");

	virtual bool HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const override;
	virtual FString DescribeRule() const override;
};
