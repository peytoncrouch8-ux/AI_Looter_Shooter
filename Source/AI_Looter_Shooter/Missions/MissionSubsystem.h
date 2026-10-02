#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MissionSubsystem.generated.h"

/** Missions changed: one was added or removed, another is tracked, or an objective's text or waypoint came or went. */
DECLARE_MULTICAST_DELEGATE(FOnMissionsChanged);

/** One mission the player has going: what it's called, what to do next, and where (if it's somewhere). */
struct FMission
{
	/** Given when it's added; never reused in the same world. */
	int32 Id = INDEX_NONE;
	FText Title;
	/** What to do next ("Shoot the target dummies..."). */
	FText Objective;
	/** Where the objective is, for the minimap's compass arrow; unset when it isn't anywhere in particular. */
	TOptional<FVector> Waypoint;
};

/**
 * The bookkeeping behind UMissionSubsystem, kept apart from the world so tests can drive it: the missions in the order
 * they were added and which one is tracked. Rules: the first mission added while none is tracked becomes tracked, and
 * removing the tracked one tracks the one that took its place in the list (else the one before it).
 */
class AI_LOOTER_SHOOTER_API FMissionBook
{
public:
	/** Adds a mission with no objective yet and returns its id. */
	int32 Add(const FText& Title);

	/** False when there was no such mission. */
	bool Remove(int32 Id);

	/**
	 * Sets a mission's objective and waypoint. Returns true when a mission list would show the change (new text, or a
	 * waypoint appearing or going away); false when nothing changed or the waypoint only moved, which happens all the time
	 * (a hunted creature walks) and only the map, which reads it every frame, cares about.
	 */
	bool SetObjective(int32 Id, const FText& Text, const TOptional<FVector>& Waypoint);

	/** Tracks a mission, or none with INDEX_NONE. False when nothing changed (already tracked, or no such mission). */
	bool Track(int32 Id);

	int32 GetTracked() const { return Tracked; }
	const FMission* Find(int32 Id) const;
	const TArray<FMission>& GetMissions() const { return Missions; }

private:
	TArray<FMission> Missions;
	int32 Tracked = INDEX_NONE;
	int32 NextId = 1;
};

/**
 * The missions active in this world, and which one is tracked (the player's "equipped" mission): the minimap's compass
 * arrow points to the tracked mission's waypoint. Whoever runs a mission adds it, keeps its objective current and
 * removes it when it's over: the mission runner (UMissionRunner) does it for every mission that is data, the tutorial's
 * among them. A mission log can list GetMissions() and pick one with TrackMission, and hear about changes from
 * OnMissionsChanged. Nothing here is saved: the runner keeps the campaign record, and adds its missions again in the
 * next level.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMissionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Adds a mission (tracked if none was) and returns its id. */
	int32 AddMission(const FText& Title);

	/** Removes a mission; if it was tracked, the next one is. Unknown ids are ignored. */
	void RemoveMission(int32 Id);

	/** The mission's next step and where it is (unset: nowhere in particular). Call it as often as the target moves. */
	void SetObjective(int32 Id, const FText& Text, const TOptional<FVector>& Waypoint);

	/** Tracks a mission (the map guides to it); INDEX_NONE tracks none. Unknown ids are ignored. */
	void TrackMission(int32 Id);

	/** The tracked mission's id, or INDEX_NONE. */
	int32 GetTrackedMission() const { return Book.GetTracked(); }

	/** The tracked mission, or null. */
	const FMission* GetTracked() const { return Book.Find(Book.GetTracked()); }

	/** A mission by id, or null. */
	const FMission* FindMission(int32 Id) const { return Book.Find(Id); }

	/** Where the tracked mission's objective is; false when no mission is tracked or its objective has no place. */
	bool GetTrackedWaypoint(FVector& OutLocation) const;

	/** Every active mission, in the order they were added. */
	const TArray<FMission>& GetMissions() const { return Book.GetMissions(); }

	/** Missions were added or removed, tracking moved, or an objective's text or waypoint came or went (not mere moves). */
	FOnMissionsChanged OnMissionsChanged;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	FMissionBook Book;
};
