#include "Missions/MissionSubsystem.h"
#include "Engine/World.h"

// --- FMissionBook ---

int32 FMissionBook::Add(const FText& Title)
{
	FMission& Mission = Missions.AddDefaulted_GetRef();
	Mission.Id = NextId++;
	Mission.Title = Title;
	// With nothing tracked, the new mission is what the player is doing now.
	if (Tracked == INDEX_NONE)
	{
		Tracked = Mission.Id;
	}
	return Mission.Id;
}

bool FMissionBook::Remove(int32 Id)
{
	const int32 Index = Missions.IndexOfByPredicate([Id](const FMission& Mission) { return Mission.Id == Id; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	Missions.RemoveAt(Index);
	// The arrow shouldn't just vanish while other missions are going: the next one in the list takes over.
	if (Tracked == Id)
	{
		Tracked = Missions.IsEmpty() ? INDEX_NONE : Missions[FMath::Min(Index, Missions.Num() - 1)].Id;
	}
	return true;
}

bool FMissionBook::SetObjective(int32 Id, const FText& Text, const TOptional<FVector>& Waypoint)
{
	FMission* Mission = Missions.FindByPredicate([Id](const FMission& Each) { return Each.Id == Id; });
	if (!Mission)
	{
		return false;
	}
	const bool bNewText = !Mission->Objective.EqualTo(Text);
	const bool bWaypointCameOrWent = Mission->Waypoint.IsSet() != Waypoint.IsSet();
	if (bNewText)
	{
		Mission->Objective = Text;
	}
	Mission->Waypoint = Waypoint;
	return bNewText || bWaypointCameOrWent;
}

bool FMissionBook::Track(int32 Id)
{
	if (Id == Tracked || (Id != INDEX_NONE && !Find(Id)))
	{
		return false;
	}
	Tracked = Id;
	return true;
}

const FMission* FMissionBook::Find(int32 Id) const
{
	return Id == INDEX_NONE ? nullptr : Missions.FindByPredicate([Id](const FMission& Mission) { return Mission.Id == Id; });
}

// --- UMissionSubsystem ---

bool UMissionSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds have missions, and editor preview worlds for the automated tests (the mission runner's test feeds
	// it); the editor's own world never does.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

int32 UMissionSubsystem::AddMission(const FText& Title)
{
	const int32 Id = Book.Add(Title);
	OnMissionsChanged.Broadcast();
	return Id;
}

void UMissionSubsystem::RemoveMission(int32 Id)
{
	if (Book.Remove(Id))
	{
		OnMissionsChanged.Broadcast();
	}
}

void UMissionSubsystem::SetObjective(int32 Id, const FText& Text, const TOptional<FVector>& Waypoint)
{
	if (Book.SetObjective(Id, Text, Waypoint))
	{
		OnMissionsChanged.Broadcast();
	}
}

void UMissionSubsystem::TrackMission(int32 Id)
{
	if (Book.Track(Id))
	{
		OnMissionsChanged.Broadcast();
	}
}

bool UMissionSubsystem::GetTrackedWaypoint(FVector& OutLocation) const
{
	const FMission* Mission = GetTracked();
	if (!Mission || !Mission->Waypoint.IsSet())
	{
		return false;
	}
	OutLocation = *Mission->Waypoint;
	return true;
}
