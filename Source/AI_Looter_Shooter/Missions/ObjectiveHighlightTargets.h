#pragma once

#include "CoreMinimal.h"
#include "Missions/MissionTargets.h"

class AActor;
class UMissionObjective;
class UMissionRunner;

/**
 * What the tracked objective's orange highlight (UObjectiveHighlightSubsystem) is about: the things a mission's current
 * objectives send the player to use, take, read, shoot or talk to, and its giver while it waits to be turned in. Never the
 * creatures a fight is against: they have their tags, bars and the minimap, and a glow through the brush would give them
 * away. Rules only, apart from the highlight's look, so the tests read them without drawing anything.
 */
namespace ObjectiveHighlightTargets
{
	/** The most actors one look gathers for a mission (a filter matching every poster in town stays cheap to range). */
	constexpr int32 MaxTargets = 48;

	/**
	 * The actors an objective is about, for the highlight: its own targets (the thing to use, the items to collect, the
	 * speaker, a reach-place's actor, the dummies to hit), else its arrow's actor when the arrow names one
	 * (EMissionWaypoint::Actor: the egg sacs, a readable, the gun rack, the notice board, the bell). Unset for a fight (a
	 * kill, a named kill, an encounter or a zone to clear), a place to hold, and what's done anywhere (a scene, a walk, a
	 * page of the inventory).
	 */
	FMissionActorFilter FilterFor(const UMissionObjective& Objective);

	/**
	 * A running mission's objectives still to do now, in their step's order: the step's (optional ones too), or, while it
	 * waits for its turn-in, the last step's optional ones not done yet. Empty when it isn't running here.
	 */
	TArray<const UMissionObjective*> LiveObjectives(const UMissionRunner& Runner, FName MissionId);

	/**
	 * Every actor the highlight may light for a mission now, each once (at most MaxTargets): its live objectives' targets
	 * that CanLight, the gun lying on a rack rather than the rack, a mission item still lying about, and its giver while it
	 * waits to be turned in.
	 */
	void Gather(const UMissionRunner& Runner, FName MissionId, TArray<AActor*>& Out);

	/**
	 * It can wear the highlight: shown, not on its way out, still standing (not dead, not used up for good, a mission item
	 * not taken) and never a pawn (the creatures, the player).
	 */
	bool CanLight(const AActor& Actor);
}
