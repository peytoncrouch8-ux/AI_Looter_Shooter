#include "Missions/MissionPlaceObjectives.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** The player is inside Place (false without a player, or when the place's actor isn't in the level). */
	bool IsPlayerInside(const FMissionContext& Context, const FMissionPlace& Where)
	{
		const TOptional<FVector> PlayerSpot = Context.GetPlayerLocation();
		return PlayerSpot.IsSet() && Where.ContainsInWorld(Context.World, *PlayerSpot);
	}

	/** "the Weapon Rack", "the spot": what a place is called when the objective has no words of its own. */
	FString PlaceName(const FMissionPlace& Where)
	{
		const FString Described = Where.Actor.Describe();
		return Described.IsEmpty() ? FString(TEXT("the spot")) : TEXT("the ") + Described;
	}
}

// --- Reach ---

void UMissionReachObjective::Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const
{
	if (IsPlayerInside(Context, Place))
	{
		State.Count = 1;
	}
}

TOptional<FVector> UMissionReachObjective::GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const
{
	return Place.Resolve(Context.World, Context.GetPlayerLocation());
}

FString UMissionReachObjective::DescribeRule() const
{
	return TEXT("Go to ") + PlaceName(Place);
}

// --- Travel ---

void UMissionTravelObjective::Begin(const FMissionContext& Context, FMissionObjectiveState& State) const
{
	State.Start = Context.GetPlayerLocation();
}

void UMissionTravelObjective::Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const
{
	const TOptional<FVector> PlayerSpot = Context.GetPlayerLocation();
	if (!PlayerSpot.IsSet())
	{
		return;
	}
	// No player when it began (the level was still starting): it counts from where they first stand.
	if (!State.Start.IsSet())
	{
		State.Start = PlayerSpot;
		return;
	}
	if (FVector::Dist2D(*PlayerSpot, *State.Start) >= Distance)
	{
		State.Count = 1;
	}
}

FString UMissionTravelObjective::DescribeRule() const
{
	return FString::Printf(TEXT("Walk %d m"), FMath::RoundToInt32(Distance / 100.f));
}

// --- Defend ---

int32 UMissionDefendObjective::GetRequired() const
{
	return FMath::Max(1, FMath::CeilToInt32(HoldSeconds));
}

void UMissionDefendObjective::Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const
{
	if (bStayInside && !IsPlayerInside(Context, Place))
	{
		return;
	}
	State.Seconds += DeltaSeconds;
	State.Count = State.Seconds >= HoldSeconds ? GetRequired() : FMath::Min(FMath::FloorToInt32(State.Seconds), GetRequired() - 1);
}

TOptional<FVector> UMissionDefendObjective::GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const
{
	return Place.Resolve(Context.World, Context.GetPlayerLocation());
}

FString UMissionDefendObjective::FormatProgress(const FMissionObjectiveState& State) const
{
	return FString::Printf(TEXT("%d s / %d s"), FMath::Min(State.Count, GetRequired()), GetRequired());
}

FString UMissionDefendObjective::DescribeRule() const
{
	return FString::Printf(TEXT("Hold %s for %d s"), *PlaceName(Place), GetRequired());
}
