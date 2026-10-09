#pragma once

#include "CoreMinimal.h"
#include "Missions/MissionObjective.h"
#include "MissionClearZoneObjective.generated.h"

/**
 * Clear a place of what lives there: done once none of the creatures (or anything else with health) the filter matches and
 * whose home is inside the zone is left alive. Skyreach's Web Hollow (six spiders) and the Wallow (five slimes), whose
 * creatures never come back once killed.
 *
 * Unlike a kill count, it reads the world whenever it looks: a spider shot before the posting went up, or before a session
 * was reloaded, counts as down, so a player can never be left owing kills of creatures that are already dead. A creature
 * belongs to the zone by its home (where it stood as play began), so one that chased the player out of the hollow still
 * counts, and one from elsewhere wandering through doesn't. Count is how many the zone holds in all, for the tracker's
 * "2 / 6"; until the last is down the count stops one short, whatever Count says.
 *
 * It lives with the tutorial because Skyreach's postings are its first users; any mission can use it.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Clear a zone"))
class AI_LOOTER_SHOOTER_API UMissionClearZoneObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	/** What lives there: actors of a class and/or with a tag (SpiderCreature). */
	UPROPERTY(EditAnywhere, Category = "Objective")
	FMissionActorFilter Target;

	/** The zone: around its actor, or a spot, and its radius (on the map unless height counts). */
	UPROPERTY(EditAnywhere, Category = "Objective")
	FMissionPlace Zone;

	/** How many it holds in all, for the tracker's count. */
	UPROPERTY(EditAnywhere, Category = "Objective", meta = (ClampMin = "1"))
	int32 Count = 1;

	virtual int32 GetRequired() const override { return FMath::Max(Count, 1); }
	virtual void Begin(const FMissionContext& Context, FMissionObjectiveState& State) const override;
	virtual void Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const override;
	virtual bool HandleKill(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Killer) const override;
	virtual FMissionActorFilter GetTargets() const override { return Target; }
	/** The zone counts as having targets even once empty: an empty zone is a cleared one, not a level missing its creatures. */
	virtual bool HasTargets(const FMissionContext& Context) const override { return true; }
	/** The living one homed in the zone nearest the player; none left (or none yet), the zone's middle. */
	virtual TOptional<FVector> GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const override;
	virtual FString DescribeRule() const override;

	/** How many of its creatures are alive in the zone now. */
	int32 CountAlive(const FMissionContext& Context) const;

	/** Where an actor belongs: a creature's home, anything else where it stands. */
	static FVector HomeOf(const AActor& Actor);

private:
	/** Reads the zone into State: how many are down (Count less those alive), all of them once none is. True when it changed. */
	bool ReadZone(const FMissionContext& Context, FMissionObjectiveState& State) const;
};
