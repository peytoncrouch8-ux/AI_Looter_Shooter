// UMissionRunner: what moves objectives on (the player's spot, deaths, hits, events) and the display it feeds.

#include "Missions/MissionRunner.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionActorWatch.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"

// ---------------------------------------------------------------------------
// Watching the level's actors
// ---------------------------------------------------------------------------

void UMissionRunner::WatchActor(AActor* Actor)
{
	if (!Actor || Actor->IsActorBeingDestroyed() || WatchedActors.Contains(FObjectKey(Actor)))
	{
		return;
	}
	UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>();
	if (!Health)
	{
		return;
	}
	UMissionActorWatch* NewWatch = NewObject<UMissionActorWatch>(this);
	NewWatch->Watch(this, Actor, Health);
	Watches.Add(NewWatch);
	WatchedActors.Add(FObjectKey(Actor));
}

void UMissionRunner::WatchExisting()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		WatchActor(*It);
	}
}

void UMissionRunner::HandleActorSpawned(AActor* Actor)
{
	// Spawned creatures (spawners, egg sacs, the console) count as much as placed ones. In play the spawn is announced
	// once the actor is fully made, its health component included.
	WatchActor(Actor);
}

void UMissionRunner::PruneWatches()
{
	for (int32 Index = Watches.Num() - 1; Index >= 0; --Index)
	{
		if (!Watches[Index] || !Watches[Index]->GetActor())
		{
			Watches.RemoveAtSwap(Index);
		}
	}
	// The keys of actors gone are dropped with them (a destroyed actor's key never comes back for another actor).
	if (WatchedActors.Num() > Watches.Num())
	{
		WatchedActors.Reset();
		for (const TObjectPtr<UMissionActorWatch>& Kept : Watches)
		{
			WatchedActors.Add(FObjectKey(Kept->GetActor()));
		}
	}
}

// ---------------------------------------------------------------------------
// What moves objectives on
// ---------------------------------------------------------------------------

bool UMissionRunner::RefreshDone(const UMissionObjective& Objective, const FMissionContext& Context, FMissionObjectiveState& State) const
{
	if (State.bDone)
	{
		return false;
	}
	const int32 Required = Objective.GetRequired();
	// A level without its targets (no dummies, no weapon rack) can't leave the mission stuck, when the objective says so.
	const bool bNothingThere = Objective.bPassWithoutTargets && !Objective.HasTargets(Context);
	if (State.Count >= Required || bNothingThere)
	{
		State.Count = FMath::Max(State.Count, Required);
		State.bDone = true;
		return true;
	}
	return false;
}

void UMissionRunner::ForEachRunningObjective(TFunctionRef<bool(const UMissionObjective&, FMissionObjectiveState&)> Visit)
{
	const FMissionContext Context = MakeContext();
	for (FRun& Run : Runs)
	{
		const UMissionDefinition* Mission = Run.Mission.Get();
		for (int32 Index = 0; Mission && Index < Run.States.Num(); ++Index)
		{
			const UMissionObjective* Objective = Mission->GetObjective(Run.Step, Index);
			FMissionObjectiveState& State = Run.States[Index];
			if (!Objective || State.bDone)
			{
				continue;
			}
			if (Visit(*Objective, State))
			{
				bChanged = true;
			}
			bChanged |= RefreshDone(*Objective, Context, State);
		}
	}
}

void UMissionRunner::Update(float DeltaSeconds)
{
	if (!bActive)
	{
		return;
	}
	if (bStartPending)
	{
		bStartPending = false;
		ResumeRecorded();
		StartDue(NAME_None);
	}
	const FMissionContext Context = MakeContext();
	ForEachRunningObjective([&Context, DeltaSeconds](const UMissionObjective& Objective, FMissionObjectiveState& State)
	{
		const int32 CountBefore = State.Count;
		Objective.Update(Context, State, DeltaSeconds);
		return State.Count != CountBefore;
	});
	PruneWatches();
	AfterChange();
}

void UMissionRunner::HandleKill(AActor& Victim, AController* Killer)
{
	if (!bActive)
	{
		return;
	}
	const FMissionContext Context = MakeContext();
	ForEachRunningObjective([&Context, &Victim, Killer](const UMissionObjective& Objective, FMissionObjectiveState& State)
	{
		return Objective.HandleKill(Context, State, Victim, Killer);
	});
	AfterChange();
}

void UMissionRunner::HandleHit(AActor& Victim, AController* Attacker)
{
	if (!bActive)
	{
		return;
	}
	const FMissionContext Context = MakeContext();
	ForEachRunningObjective([&Context, &Victim, Attacker](const UMissionObjective& Objective, FMissionObjectiveState& State)
	{
		return Objective.HandleHit(Context, State, Victim, Attacker);
	});
	AfterChange();
}

void UMissionRunner::NotifyEvent(const FMissionEvent& Event)
{
	if (!bActive || Event.Name.IsNone())
	{
		return;
	}
	// The running missions hear it first; a mission it starts begins after, so the words that start a mission aren't
	// also taken as its first objective.
	const FMissionContext Context = MakeContext();
	ForEachRunningObjective([&Context, &Event](const UMissionObjective& Objective, FMissionObjectiveState& State)
	{
		return Objective.HandleEvent(Context, State, Event);
	});
	StartDue(Event.Name);
	AfterChange();
}

// ---------------------------------------------------------------------------
// The display (UMissionSubsystem) and listeners
// ---------------------------------------------------------------------------

void UMissionRunner::AfterChange()
{
	Settle();
	SyncDisplay();
	if (bChanged)
	{
		bChanged = false;
		OnChanged.Broadcast();
	}
}

void UMissionRunner::SyncDisplay()
{
	UMissionSubsystem* Display = GetWorld() ? GetWorld()->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (!Display)
	{
		return;
	}
	const FMissionContext Context = MakeContext();
	for (FRun& Run : Runs)
	{
		const UMissionDefinition* Mission = Run.Mission.Get();
		if (!Mission)
		{
			continue;
		}
		if (Run.BookId == INDEX_NONE)
		{
			Run.BookId = Display->AddMission(Mission->Title);
		}
		// The first objective not done yet is what the tracker says; the arrow points to the first one that is somewhere.
		FString Line;
		TOptional<FVector> Target;
		for (int32 Index = 0; Index < Run.States.Num(); ++Index)
		{
			const UMissionObjective* Objective = Mission->GetObjective(Run.Step, Index);
			const FMissionObjectiveState& State = Run.States[Index];
			if (!Objective || State.bDone)
			{
				continue;
			}
			if (Line.IsEmpty())
			{
				Line = Objective->GetTrackerText(Context.World, State);
			}
			if (!Target.IsSet())
			{
				Target = Objective->FindWaypoint(Context, State);
			}
		}
		// Unchanged words and a waypoint that only moved don't wake the display's listeners, so this is cheap to repeat.
		Display->SetObjective(Run.BookId, FText::FromString(Line), Target);
	}
}

void UMissionRunner::RemoveFromDisplay(FRun& Run)
{
	UMissionSubsystem* Display = GetWorld() ? GetWorld()->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (Display && Run.BookId != INDEX_NONE)
	{
		Display->RemoveMission(Run.BookId);
	}
	Run.BookId = INDEX_NONE;
}
