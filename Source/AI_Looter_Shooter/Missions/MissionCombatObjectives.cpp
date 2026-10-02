#include "Missions/MissionCombatObjectives.h"
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
