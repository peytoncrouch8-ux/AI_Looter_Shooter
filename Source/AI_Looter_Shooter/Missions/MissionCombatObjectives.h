#pragma once

#include "CoreMinimal.h"
#include "Missions/MissionObjective.h"
#include "MissionCombatObjectives.generated.h"

// The objectives about fighting: kills counted by kind or place, one named enemy, hits on targets. The runner hears every
// death and hit from the health components of the level's actors, ones spawned later too.

/** Kill a number of something: by class, by tag, both, and optionally only inside a zone (the chapel yard). */
UCLASS(BlueprintType, meta = (DisplayName = "Kill"))
class AI_LOOTER_SHOOTER_API UMissionKillObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	/** What counts: actors of this class and/or with this tag. */
	UPROPERTY(EditAnywhere, Category = "Objective")
	FMissionActorFilter Target;

	UPROPERTY(EditAnywhere, Category = "Objective", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** Only kills inside Zone count. */
	UPROPERTY(EditAnywhere, Category = "Objective")
	bool bInZone = false;

	/** Where kills count (around its actor, or its spot), when bInZone. */
	UPROPERTY(EditAnywhere, Category = "Objective", meta = (EditCondition = "bInZone"))
	FMissionPlace Zone;

	/** Only the player's kills count (not a fall, not another creature's). */
	UPROPERTY(EditAnywhere, Category = "Objective")
	bool bPlayerKillsOnly = true;

	virtual int32 GetRequired() const override { return FMath::Max(Count, 1); }
	virtual bool HandleKill(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Killer) const override;
	virtual FMissionActorFilter GetTargets() const override { return Target; }
	virtual FString DescribeRule() const override;
};

/** Kill one particular enemy: the actor carrying ActorTag (a boss, a wanted man), spawned now or later. */
UCLASS(BlueprintType, meta = (DisplayName = "Kill a named actor"))
class AI_LOOTER_SHOOTER_API UMissionKillNamedObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	/** The tag only that actor carries (Boss_Abel). */
	UPROPERTY(EditAnywhere, Category = "Objective")
	FName ActorTag;

	/** Only the player's kill counts. Off by default: a boss falling off the deck is still beaten. */
	UPROPERTY(EditAnywhere, Category = "Objective")
	bool bPlayerKillsOnly = false;

	virtual bool HandleKill(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Killer) const override;
	virtual FMissionActorFilter GetTargets() const override;
	virtual FString DescribeRule() const override;
};

/** Land a number of hits on something (the tutorial's target dummies, which never stay dead). */
UCLASS(BlueprintType, meta = (DisplayName = "Hit"))
class AI_LOOTER_SHOOTER_API UMissionHitObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Objective")
	FMissionActorFilter Target;

	UPROPERTY(EditAnywhere, Category = "Objective", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** Only the player's hits count. */
	UPROPERTY(EditAnywhere, Category = "Objective")
	bool bPlayerHitsOnly = true;

	virtual int32 GetRequired() const override { return FMath::Max(Count, 1); }
	virtual bool HandleHit(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Attacker) const override;
	virtual FMissionActorFilter GetTargets() const override { return Target; }
	virtual FString DescribeRule() const override;
};
