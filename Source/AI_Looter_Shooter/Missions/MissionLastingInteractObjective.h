#pragma once

#include "CoreMinimal.h"
#include "Missions/MissionEventObjectives.h"
#include "MissionLastingInteractObjective.generated.h"

/**
 * Use things that stay used: Interact's rule (each target the filter matches counted once, by a held use when it asks for
 * one), and every target the world keeps used up (IInteractable::IsUsedUp: a wanted poster torn down, a hay bale loaded)
 * counts whenever it looks: as its step begins and a few times a second. So its count lives in the world, which the
 * session keeps with each map. A side mission starts again from its first step in the next session or level (the
 * campaign record keeps only the main mission's step), and the posters torn down before still count; so do ones torn
 * before the step began, or by the console (Looter.Poster.TearAll).
 */
UCLASS(BlueprintType, meta = (DisplayName = "Interact (lasting)"))
class AI_LOOTER_SHOOTER_API UMissionLastingInteractObjective : public UMissionInteractObjective
{
	GENERATED_BODY()

public:
	virtual void Begin(const FMissionContext& Context, FMissionObjectiveState& State) const override;
	virtual void Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const override;

	/** Counts the targets used up that aren't counted yet. True when one was. */
	bool CountUsedUp(const FMissionContext& Context, FMissionObjectiveState& State) const;
};
