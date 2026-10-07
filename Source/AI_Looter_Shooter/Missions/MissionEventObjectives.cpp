#include "Missions/MissionEventObjectives.h"
#include "Scenes/SceneSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// --- Event ---

bool UMissionEventObjective::HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const
{
	if (Event.IsNone() || Happened.Name != Event)
	{
		return false;
	}
	++State.Count;
	return true;
}

FString UMissionEventObjective::DescribeRule() const
{
	return FName::NameToDisplayString(Event.ToString(), /*bIsBool*/ false);
}

// --- Interact ---

bool UMissionInteractObjective::HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const
{
	if (Happened.Name != FMissionEvent::Interact || (bHold && !Happened.bHeld) || !Happened.Matches(Target))
	{
		return false;
	}
	// Each thing once: tearing the same poster down twice is still one poster.
	const FName Key = Happened.ThingKey();
	if (!Key.IsNone())
	{
		if (State.Used.Contains(Key))
		{
			return false;
		}
		State.Used.Add(Key);
	}
	++State.Count;
	return true;
}

TOptional<FVector> UMissionInteractObjective::GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const
{
	// The nearest one not used yet, so the arrow moves on to the next poster.
	const TOptional<FVector> From = Context.GetPlayerLocation();
	const AActor* Nearest = nullptr;
	double NearestDistance = TNumericLimits<double>::Max();
	MissionTargets::ForEach(Context.World, Target, [&](AActor& Candidate)
	{
		if (State.Used.Contains(Candidate.GetFName()))
		{
			return;
		}
		const double Distance = From.IsSet() ? FVector::DistSquared2D(Candidate.GetActorLocation(), *From) : 0.0;
		if (!Nearest || Distance < NearestDistance)
		{
			Nearest = &Candidate;
			NearestDistance = Distance;
		}
	});
	return Nearest ? TOptional<FVector>(GetWaypointOf(*Nearest)) : TOptional<FVector>();
}

FString UMissionInteractObjective::DescribeRule() const
{
	const FString Thing = Target.Describe();
	return GetRequired() > 1 ? FString::Printf(TEXT("Use %d %s"), GetRequired(), *Thing) : TEXT("Use the ") + Thing;
}

// --- Talk ---

bool UMissionTalkObjective::HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const
{
	if (SpeakerTag.IsNone() || Happened.Name != FMissionEvent::Talk || !Happened.Matches(GetTargets()))
	{
		return false;
	}
	State.Count = 1;
	return true;
}

FMissionActorFilter UMissionTalkObjective::GetTargets() const
{
	FMissionActorFilter Speaker;
	Speaker.ActorTag = SpeakerTag;
	return Speaker;
}

FString UMissionTalkObjective::DescribeRule() const
{
	// Speakers are tagged Speaker_<Name>; the tracker says the name ("Talk to Delia").
	FString Name = SpeakerTag.ToString();
	Name.RemoveFromStart(TEXT("Speaker_"));
	return TEXT("Talk to ") + FName::NameToDisplayString(Name, /*bIsBool*/ false);
}

// --- Scene ---

void UMissionSceneObjective::Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const
{
	// Played before this step began listening (as the level began, or passed over because the story is past it).
	if (!Scene.IsNone() && USceneSubsystem::HasPlayed(Context.World, Scene))
	{
		State.Count = 1;
	}
}

bool UMissionSceneObjective::HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const
{
	if (Scene.IsNone() || Happened.Name != FMissionEvent::SceneEvent(Scene))
	{
		return false;
	}
	State.Count = 1;
	return true;
}

FString UMissionSceneObjective::DescribeRule() const
{
	return TEXT("Watch ") + FName::NameToDisplayString(Scene.ToString(), /*bIsBool*/ false);
}

// --- Board ---

bool UMissionBoardObjective::HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const
{
	if (Vehicle.IsNone() || Happened.Name != FMissionEvent::BoardEvent(Vehicle))
	{
		return false;
	}
	State.Count = 1;
	return true;
}

FString UMissionBoardObjective::DescribeRule() const
{
	return TEXT("Board the ") + FName::NameToDisplayString(Vehicle.ToString(), /*bIsBool*/ false);
}
