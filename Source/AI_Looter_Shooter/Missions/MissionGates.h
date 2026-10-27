#pragma once

#include "CoreMinimal.h"

struct FInteractionOptions;
struct FStoryCondition;
class UObject;

/**
 * Mission gates (the user's playtest, 2026-10-27: "you can speed run quests and just spam click the use button"): a
 * mission's steps open in order. A step's things appear or work only from that step (the build scripts give each its
 * FStoryCondition: AMissionItem::ShownWhen/TakeWhen, AReadableProp::ShownWhen/UsableWhen, AMissionFitting::UsableWhen),
 * and its key actions (taking a mission item, a step's reading) take a short hold of Interact instead of a tap, so a
 * stray press never does them. Lore stays a tap, any time.
 */
namespace MissionGates
{
	/** The short hold's length (s) for a mission's key actions: Looter.Missions.HoldSeconds. 0 or less: they're taps. */
	float HoldSeconds();

	/** Whether any of Conditions holds now, as WorldContext's level's mission runner sees the story. Empty: true; no runner: false. */
	bool AnyMet(const TArray<FStoryCondition>& Conditions, const UObject* WorldContext);

	/** Whether Condition holds now (empty: true; no runner: false), as AnyMet. */
	bool IsMet(const FStoryCondition& Condition, const UObject* WorldContext);

	/**
	 * A mission action's use on the Interact key: a hold of HoldSeconds() with Words as its prompt when bKeyAction (and the
	 * hold isn't turned off), else a tap. Leaves bUsable and Reach alone.
	 */
	void SetUse(FInteractionOptions& Options, const FText& Words, bool bKeyAction);
}
