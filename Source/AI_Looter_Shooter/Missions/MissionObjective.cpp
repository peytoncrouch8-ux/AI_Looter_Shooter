#include "Missions/MissionObjective.h"
#include "Missions/MissionSubsystem.h"
#include "Missions/MissionText.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"

// --- FMissionContext ---

TOptional<FVector> FMissionContext::GetPlayerLocation() const
{
	return Player ? TOptional<FVector>(Player->GetActorLocation()) : TOptional<FVector>();
}

bool FMissionContext::IsPlayer(const AController* Instigator)
{
	return Instigator && Instigator->IsPlayerController();
}

// --- FMissionEvent ---

const FName FMissionEvent::Interact(TEXT("Interact"));
const FName FMissionEvent::Talk(TEXT("Talk"));
const FName FMissionEvent::Collect(TEXT("Collect"));

FMissionEvent FMissionEvent::Named(FName InName, AActor* InActor, FName InTag)
{
	FMissionEvent Event;
	Event.Name = InName;
	Event.Actor = InActor;
	Event.Tag = InTag;
	return Event;
}

FMissionEvent FMissionEvent::Interaction(AActor* InActor, bool bInHeld)
{
	FMissionEvent Event = Named(Interact, InActor);
	Event.bHeld = bInHeld;
	return Event;
}

FMissionEvent FMissionEvent::Talked(AActor* Speaker)
{
	return Named(Talk, Speaker);
}

FMissionEvent FMissionEvent::Collected(AActor* Item)
{
	return Named(Collect, Item);
}

FName FMissionEvent::SceneEvent(FName Scene)
{
	return FName(*(TEXT("Scene.") + Scene.ToString()));
}

FName FMissionEvent::BoardEvent(FName Vehicle)
{
	return FName(*(TEXT("Board.") + Vehicle.ToString()));
}

bool FMissionEvent::Matches(const FMissionActorFilter& Filter) const
{
	if (const AActor* Thing = Actor.Get())
	{
		return Filter.Matches(Thing);
	}
	// Nothing to look at: only a tag can say what it was, and only a filter that asks for no more than a tag can agree.
	return !Tag.IsNone() && !Filter.ActorClass && Filter.ActorTag == Tag;
}

FName FMissionEvent::ThingKey() const
{
	const AActor* Thing = Actor.Get();
	return Thing ? Thing->GetFName() : NAME_None;
}

// --- UMissionObjective ---

bool UMissionObjective::HasTargets(const FMissionContext& Context) const
{
	const FMissionActorFilter Targets = GetTargets();
	return !Targets.IsSet() || MissionTargets::Any(Context.World, Targets);
}

TOptional<FVector> UMissionObjective::GetAutoWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const
{
	const FMissionActorFilter Targets = GetTargets();
	const AActor* Nearest = Targets.IsSet() ? MissionTargets::FindNearest(Context.World, Targets, Context.GetPlayerLocation(), /*bLivingOnly*/ true) : nullptr;
	return Nearest ? TOptional<FVector>(GetWaypointOf(*Nearest)) : TOptional<FVector>();
}

FVector UMissionObjective::GetWaypointOf(const AActor& Found) const
{
	return Found.GetActorLocation();
}

FString UMissionObjective::FormatProgress(const FMissionObjectiveState& State) const
{
	const int32 Required = GetRequired();
	return FString::Printf(TEXT("%d / %d"), FMath::Min(State.Count, Required), Required);
}

TOptional<FVector> UMissionObjective::FindWaypoint(const FMissionContext& Context, const FMissionObjectiveState& State) const
{
	switch (Waypoint)
	{
	case EMissionWaypoint::None:
		return TOptional<FVector>();
	case EMissionWaypoint::Location:
		return WaypointLocation;
	case EMissionWaypoint::Actor:
		if (const AActor* Found = MissionTargets::FindNearest(Context.World, WaypointActor, Context.GetPlayerLocation(), /*bLivingOnly*/ true))
		{
			return GetWaypointOf(*Found);
		}
		break;
	case EMissionWaypoint::TargetsCenter:
	{
		const TOptional<FVector> Center = MissionTargets::FindCenter(Context.World, GetTargets());
		if (Center.IsSet())
		{
			return Center;
		}
		break;
	}
	case EMissionWaypoint::Auto:
		break;
	}
	// Auto, or the level has none of what the setting asked for: the objective's own.
	return GetAutoWaypoint(Context, State);
}

FString UMissionObjective::GetDisplayText(const UWorld* World) const
{
	return MissionText::ResolveKeys(World, Text.IsEmpty() ? DescribeRule() : Text.ToString());
}

FString UMissionObjective::GetTrackerText(const UWorld* World, const FMissionObjectiveState& State) const
{
	FString Line = GetDisplayText(World);
	if (bShowCount && GetRequired() > 1 && !State.bDone)
	{
		Line += FString::Printf(TEXT(" (%s)"), *FormatProgress(State).Replace(TEXT(" / "), TEXT("/")));
	}
	return Line;
}

FString UMissionObjective::GetShortText(const UWorld* World) const
{
	return ShortText.IsEmpty() ? GetDisplayText(World) : MissionText::ResolveKeys(World, ShortText.ToString());
}

void UMissionObjective::FillTrackerParts(const UWorld* World, const FMissionObjectiveState& State, FMissionTrackerParts& Out) const
{
	Out.Line = GetShortText(World);
	// The count only where it says something: a single thing to do is done or not.
	const int32 Required = GetRequired();
	if (bShowCount && Required > 1)
	{
		FMissionObjectiveState Full;
		Full.Count = Required;
		Full.bDone = true;
		Out.Count = FormatProgress(State);
		Out.CountDone = FormatProgress(Full);
		Out.Progress = FMath::Min(State.Count, Required);
		Out.Required = Required;
		Out.bCountIsTime = CountsSeconds();
	}
	else
	{
		Out.Count.Reset();
		Out.CountDone.Reset();
		Out.Progress = 0;
		Out.Required = 0;
		Out.bCountIsTime = false;
	}
	if (HintAction.IsNone())
	{
		Out.HintKey.Reset();
		Out.HintText.Reset();
	}
	else
	{
		Out.HintKey = MissionText::KeyName(World, HintAction);
		Out.HintText = MissionText::ResolveKeys(World, HintText.ToString());
	}
	Out.bOverInventory = IsDoneInInventory();
	Out.bDone = State.bDone;
}

float UMissionObjective::GetFraction(const FMissionObjectiveState& State) const
{
	if (State.bDone)
	{
		return 1.f;
	}
	const int32 Required = GetRequired();
	return Required > 0 ? FMath::Clamp(static_cast<float>(State.Count) / static_cast<float>(Required), 0.f, 1.f) : 0.f;
}
