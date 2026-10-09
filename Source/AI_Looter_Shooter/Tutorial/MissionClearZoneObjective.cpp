#include "Tutorial/MissionClearZoneObjective.h"
#include "Creatures/CreatureBase.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"

FVector UMissionClearZoneObjective::HomeOf(const AActor& Actor)
{
	// A creature wanders and chases; the spot it stood on as play began says where it lives.
	if (const ACreatureBase* Creature = Cast<ACreatureBase>(&Actor))
	{
		return Creature->GetHome().GetLocation();
	}
	return Actor.GetActorLocation();
}

int32 UMissionClearZoneObjective::CountAlive(const FMissionContext& Context) const
{
	int32 Alive = 0;
	const UWorld* World = Context.World;
	MissionTargets::ForEach(World, Target, [this, World, &Alive](AActor& Candidate)
	{
		if (MissionTargets::IsAlive(Candidate) && Zone.ContainsInWorld(World, HomeOf(Candidate)))
		{
			++Alive;
		}
	});
	return Alive;
}

bool UMissionClearZoneObjective::ReadZone(const FMissionContext& Context, FMissionObjectiveState& State) const
{
	const int32 Required = GetRequired();
	const int32 Alive = CountAlive(Context);
	// Only an empty zone is done, so the count stops one short until then (a zone holding more than Count reads 0 a while).
	const int32 Down = Alive == 0 ? Required : FMath::Clamp(Required - Alive, 0, Required - 1);
	if (Down == State.Count)
	{
		return false;
	}
	State.Count = Down;
	return true;
}

void UMissionClearZoneObjective::Begin(const FMissionContext& Context, FMissionObjectiveState& State) const
{
	ReadZone(Context, State);
}

void UMissionClearZoneObjective::Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const
{
	ReadZone(Context, State);
}

bool UMissionClearZoneObjective::HandleKill(const FMissionContext& Context, FMissionObjectiveState& State, const AActor& Victim, const AController* Killer) const
{
	// One of them down: the tracker counts it at once rather than at the next look.
	return Target.Matches(&Victim) && ReadZone(Context, State);
}

TOptional<FVector> UMissionClearZoneObjective::GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const
{
	const TOptional<FVector> From = Context.GetPlayerLocation();
	const AActor* Nearest = nullptr;
	double NearestSquared = TNumericLimits<double>::Max();
	const UWorld* World = Context.World;
	MissionTargets::ForEach(World, Target, [this, World, &From, &Nearest, &NearestSquared](AActor& Candidate)
	{
		if (!MissionTargets::IsAlive(Candidate) || !Zone.ContainsInWorld(World, HomeOf(Candidate)))
		{
			return;
		}
		const double DistanceSquared = From.IsSet() ? FVector::DistSquared2D(*From, Candidate.GetActorLocation()) : 0.0;
		if (!Nearest || DistanceSquared < NearestSquared)
		{
			Nearest = &Candidate;
			NearestSquared = DistanceSquared;
		}
	});
	return Nearest ? TOptional<FVector>(Nearest->GetActorLocation()) : Zone.Resolve(World, From);
}

FString UMissionClearZoneObjective::DescribeRule() const
{
	return FString::Printf(TEXT("Clear the %s out of the zone"), *Target.Describe());
}
