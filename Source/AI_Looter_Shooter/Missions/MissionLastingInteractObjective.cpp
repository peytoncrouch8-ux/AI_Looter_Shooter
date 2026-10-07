#include "Missions/MissionLastingInteractObjective.h"
#include "Interaction/Interactable.h"
#include "Missions/MissionTargets.h"
#include "GameFramework/Actor.h"

void UMissionLastingInteractObjective::Begin(const FMissionContext& Context, FMissionObjectiveState& State) const
{
	Super::Begin(Context, State);
	CountUsedUp(Context, State);
}

void UMissionLastingInteractObjective::Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const
{
	Super::Update(Context, State, DeltaSeconds);
	CountUsedUp(Context, State);
}

bool UMissionLastingInteractObjective::CountUsedUp(const FMissionContext& Context, FMissionObjectiveState& State) const
{
	bool bCounted = false;
	MissionTargets::ForEach(Context.World, Target, [&State, &bCounted](AActor& Candidate)
	{
		const IInteractable* Interactable = Cast<IInteractable>(&Candidate);
		// Keyed by the actor's name, as an interaction is (FMissionEvent::ThingKey): a poster the player tore down, heard
		// as it happened and seen down here afterwards, counts once.
		const FName Key = Candidate.GetFName();
		if (Interactable && Interactable->IsUsedUp() && !State.Used.Contains(Key))
		{
			State.Used.Add(Key);
			++State.Count;
			bCounted = true;
		}
	});
	return bCounted;
}
