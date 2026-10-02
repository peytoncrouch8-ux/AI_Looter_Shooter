#pragma once

#include "CoreMinimal.h"
#include "Missions/MissionTargets.h"
#include "UObject/Object.h"
#include "MissionObjective.generated.h"

class AActor;
class AController;
class APlayerController;
class UWorld;

/** Where the minimap's arrow points while an objective is the one to do. */
UENUM(BlueprintType)
enum class EMissionWaypoint : uint8
{
	/** The objective's own: its place, the nearest of its targets still standing, the speaker. */
	Auto,
	/** Nowhere: the arrow goes away (opening the inventory happens anywhere). */
	None,
	/** The middle of all its targets (a training ground, rather than one dummy). */
	TargetsCenter,
	/** The nearest actor WaypointActor matches (the gun rack while the road leads there); the objective's own when none. */
	Actor,
	/** A fixed spot, WaypointLocation. */
	Location,
};

/** What objectives look at while they run: the world and the player. */
struct AI_LOOTER_SHOOTER_API FMissionContext
{
	UWorld* World = nullptr;

	/** The player's pawn (or a test's stand-in); null while there is none. */
	AActor* Player = nullptr;

	/** The player's controller: their HUD, their keys. Null in tests. */
	APlayerController* Controller = nullptr;

	TOptional<FVector> GetPlayerLocation() const;

	/** Kills and hits count as the player's when a player's controller made them (one player in this game). */
	static bool IsPlayer(const AController* Instigator);
};

/**
 * Something that happened that objectives may be waiting for. The game's systems send them to the mission runner
 * (UMissionRunner::NotifyEvent): the interaction component (Interact, tap or hold), the speaker points (Talk), pickups
 * (Collect), scenes when they end (Scene.<Name>) and the skiff (Board.<Vehicle>). Any other name works for a generic
 * Event objective ("Bell.Rung").
 */
struct AI_LOOTER_SHOOTER_API FMissionEvent
{
	static const FName Interact;
	static const FName Talk;
	static const FName Collect;

	FName Name;

	/** What it happened with (the poster, the speaker point, the pickup), when there is something. */
	TWeakObjectPtr<AActor> Actor;

	/** A tag standing for the actor when there's none (the console: Looter.Mission.Event Interact Poster). */
	FName Tag;

	/** An interaction that was held rather than tapped. */
	bool bHeld = false;

	static FMissionEvent Named(FName InName, AActor* InActor = nullptr, FName InTag = NAME_None);
	static FMissionEvent Interaction(AActor* InActor, bool bInHeld);
	static FMissionEvent Talked(AActor* Speaker);
	static FMissionEvent Collected(AActor* Item);

	/** "Scene.ColdOpen": the event a scene sends when it has played. */
	static FName SceneEvent(FName Scene);

	/** "Board.Skiff": the event boarding a vehicle sends. */
	static FName BoardEvent(FName Vehicle);

	/** Its actor matches Filter; with no actor, its tag is Filter's tag (and Filter asks for no class). */
	bool Matches(const FMissionActorFilter& Filter) const;

	/** Tells this one's thing apart from others of its kind (the third poster): its actor's name. None without an actor. */
	FName ThingKey() const;
};

/** One objective's progress in a running mission. The runner keeps it; the mission's data never changes while it runs. */
struct FMissionObjectiveState
{
	/** How far along: kills, hits, things used, whole seconds held. Done at the objective's GetRequired(). */
	int32 Count = 0;

	/** Done, for good (stepping out of a place reached doesn't undo it). */
	bool bDone = false;

	/** Where the player stood as it began (a distance to walk). */
	TOptional<FVector> Start;

	/** Seconds counted so far (holding out). */
	float Seconds = 0.f;

	/** The things already counted, so each counts once (the posters torn down). */
	TArray<FName> Used;
};

/**
 * One thing to do in a mission's step: an instanced object inside a UMissionDefinition, one class per kind of objective
 * (reach a place, kill, interact, talk, ...; Missions/Mission*Objectives.h). An objective is a rule: it holds only its
 * settings, and the runner keeps each running copy's progress (FMissionObjectiveState) and calls the rule's hooks as the
 * player plays. A new kind of objective is a new subclass overriding the hooks it needs.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories)
class AI_LOOTER_SHOOTER_API UMissionObjective : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * What the tracker and the Missions page say ("Clear the spiders nesting at the top"). {Action} shows the key bound to
	 * that action ({Interact}, {Inventory}), {Move} the movement keys. Empty: a plain description of the rule.
	 */
	UPROPERTY(EditAnywhere, Category = "Objective", meta = (MultiLine = true))
	FText Text;

	UPROPERTY(EditAnywhere, Category = "Objective|Waypoint")
	EMissionWaypoint Waypoint = EMissionWaypoint::Auto;

	UPROPERTY(EditAnywhere, Category = "Objective|Waypoint", meta = (EditCondition = "Waypoint == EMissionWaypoint::Actor", EditConditionHides))
	FMissionActorFilter WaypointActor;

	UPROPERTY(EditAnywhere, Category = "Objective|Waypoint", meta = (EditCondition = "Waypoint == EMissionWaypoint::Location", EditConditionHides))
	FVector WaypointLocation = FVector::ZeroVector;

	/**
	 * Done at once when the level has none of its targets (no dummies, no weapon rack), so a level without them can't
	 * leave the mission stuck. The tutorial's way.
	 */
	UPROPERTY(EditAnywhere, Category = "Objective")
	bool bPassWithoutTargets = false;

	/** The tracker shows how far along it is, "(2/4)", when it counts more than one. */
	UPROPERTY(EditAnywhere, Category = "Objective")
	bool bShowCount = true;

	// --- The rule (the runner calls these; State is this objective's progress in the running mission) ---

	/** The count that finishes it (kills, hits, things; whole seconds for holding out). */
	virtual int32 GetRequired() const { return 1; }

	/** It starts: its step has begun. */
	virtual void Begin(const FMissionContext& Context, FMissionObjectiveState& State) const {}

	/** A few times a second while it isn't done: whatever it watches for itself (the player's spot, what they carry). */
	virtual void Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const {}

	/** Something with health died. True when that counted. */
	virtual bool HandleKill(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Killer) const { return false; }

	/** Something with health was hurt. True when that counted. */
	virtual bool HandleHit(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Attacker) const { return false; }

	/** Something happened (an interaction, words spoken, a scene played). True when that counted. */
	virtual bool HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const { return false; }

	/** The actors it's about, for its waypoint and for bPassWithoutTargets; an empty filter when it has none. */
	virtual FMissionActorFilter GetTargets() const { return FMissionActorFilter(); }

	/** The level has something for it to be done on (always, for objectives without targets). */
	virtual bool HasTargets(const FMissionContext& Context) const;

	/** Its own waypoint (EMissionWaypoint::Auto): by default the nearest of its targets still standing. */
	virtual TOptional<FVector> GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const;

	/** Where the arrow points for one of its actors: the actor (a gun rack's gun while collecting guns). */
	virtual FVector GetWaypointOf(const AActor& Found) const;

	/** Its progress for people: "2 / 4" (holding out: "12 s / 30 s"). */
	virtual FString FormatProgress(const FMissionObjectiveState& State) const;

	/** What it asks when Text is empty: "Kill 4 Spider Creature". */
	virtual FString DescribeRule() const { return FString(); }

	// --- For the runner and the Missions page ---

	/** Where the minimap's arrow points now, by the Waypoint setting; unset: nowhere. */
	TOptional<FVector> FindWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const;

	/** Its words with the player's keys in them, no count. */
	FString GetDisplayText(const UWorld* World) const;

	/** What the tracker says: its words, and "(2/4)" while a count of more than one is under way. */
	FString GetTrackerText(const UWorld* World, const FMissionObjectiveState& State) const;

	/** How far along, 0 to 1. */
	float GetFraction(const FMissionObjectiveState& State) const;
};
