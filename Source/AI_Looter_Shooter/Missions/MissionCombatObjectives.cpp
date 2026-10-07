#include "Missions/MissionCombatObjectives.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"

// --- Kill ---

bool UMissionKillObjective::HandleKill(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Killer) const
{
	if (!Target.Matches(&Victim) || (bPlayerKillsOnly && !FMissionContext::IsPlayer(Killer)))
	{
		return false;
	}
	if (bInZone && !Zone.ContainsInWorld(Context.World, Victim.GetActorLocation()))
	{
		return false;
	}
	++State.Count;
	return true;
}

FString UMissionKillObjective::DescribeRule() const
{
	return FString::Printf(TEXT("Kill %d %s"), GetRequired(), *Target.Describe());
}

// --- Kill a named actor ---

bool UMissionKillNamedObjective::HandleKill(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Killer) const
{
	if (ActorTag.IsNone() || !Victim.ActorHasTag(ActorTag) || (bPlayerKillsOnly && !FMissionContext::IsPlayer(Killer)))
	{
		return false;
	}
	State.Count = 1;
	return true;
}

FMissionActorFilter UMissionKillNamedObjective::GetTargets() const
{
	FMissionActorFilter Named;
	Named.ActorTag = ActorTag;
	return Named;
}

FString UMissionKillNamedObjective::DescribeRule() const
{
	return TEXT("Kill ") + FName::NameToDisplayString(ActorTag.ToString(), /*bIsBool*/ false);
}

// --- Hit ---

bool UMissionHitObjective::HandleHit(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Attacker) const
{
	if (!Target.Matches(&Victim) || (bPlayerHitsOnly && !FMissionContext::IsPlayer(Attacker)))
	{
		return false;
	}
	++State.Count;
	return true;
}

FString UMissionHitObjective::DescribeRule() const
{
	return FString::Printf(TEXT("Hit %s %d times"), *Target.Describe(), GetRequired());
}

// --- Clear an encounter ---

AEncounterSpawner* UMissionClearObjective::FindSpawner(const FMissionContext& Context) const
{
	const UEncounterSubsystem* Encounters = Context.World ? Context.World->GetSubsystem<UEncounterSubsystem>() : nullptr;
	return Encounters && !SpawnerId.IsNone() ? Encounters->FindSpawner(SpawnerId) : nullptr;
}

bool UMissionClearObjective::ReadSpawner(const FMissionContext& Context, FMissionObjectiveState& State) const
{
	const AEncounterSpawner* Spawner = FindSpawner(Context);
	if (!Spawner)
	{
		return false;
	}
	const int32 Required = GetRequired();
	int32 Down = Required;
	if (Spawner->GetState() != EEncounterState::Cleared)
	{
		// Out of the fight for good: brought by its waves, and neither alive nor still owed (held back by a cap, or taken
		// away while the player was far). Only a cleared encounter is done, so the count stops one short till then.
		Down = FMath::Clamp(Spawner->GetTotalSpawned() - Spawner->NumAlive() - Spawner->NumOwed(), 0, Required - 1);
	}
	if (Down == State.Count)
	{
		return false;
	}
	State.Count = Down;
	return true;
}

void UMissionClearObjective::Begin(const FMissionContext& Context, FMissionObjectiveState& State) const
{
	ReadSpawner(Context, State);
}

void UMissionClearObjective::Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const
{
	ReadSpawner(Context, State);
}

bool UMissionClearObjective::HandleKill(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Killer) const
{
	// One of its own down: the tracker counts it at once (the spawner calls itself cleared at its next look).
	return Cast<ACreatureBase>(&Victim) && ReadSpawner(Context, State);
}

bool UMissionClearObjective::HasTargets(const FMissionContext& Context) const
{
	return FindSpawner(Context) != nullptr;
}

TOptional<FVector> UMissionClearObjective::GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const
{
	const AEncounterSpawner* Spawner = FindSpawner(Context);
	if (!Spawner)
	{
		return TOptional<FVector>();
	}
	const TOptional<FVector> From = Context.GetPlayerLocation();
	const ACreatureBase* Nearest = nullptr;
	double NearestSquared = TNumericLimits<double>::Max();
	for (const ACreatureBase* Creature : Spawner->GetAliveCreatures())
	{
		const double DistanceSquared = From.IsSet() ? FVector::DistSquared2D(From.GetValue(), Creature->GetActorLocation()) : 0.0;
		if (!Nearest || DistanceSquared < NearestSquared)
		{
			Nearest = Creature;
			NearestSquared = DistanceSquared;
		}
	}
	return Nearest ? Nearest->GetActorLocation() : Spawner->GetActorLocation();
}

FString UMissionClearObjective::DescribeRule() const
{
	return FString::Printf(TEXT("Clear the encounter %s"), *SpawnerId.ToString());
}
