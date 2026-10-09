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
	// also taken as its first objective. A talk then turns in what's ready for that giver (missions it unlocks start, and
	// don't take the talk either), so a talk that finished an objective doesn't also turn its mission in.
	const FMissionContext Context = MakeContext();
	ForEachRunningObjective([&Context, &Event](const UMissionObjective& Objective, FMissionObjectiveState& State)
	{
		return Objective.HandleEvent(Context, State, Event);
	});
	if (TurnInAt(Event))
	{
		StartDue(NAME_None);
	}
	StartDue(Event.Name);
	AfterChange();
	// Passed on last, so a listener whose story hangs on the step this event just finished sees it finished.
	OnEvent.Broadcast(Event);
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
		if (Run.bReady)
		{
			// Nothing left but its turn-in: who to, and the arrow on them.
			SyncTurnIn(Run, *Mission, *Display);
			continue;
		}
		// The first objective not done yet is what the tracker says (in full, and in the HUD tracker's parts: its short
		// line, count and hint, with the step bar's step); the arrow points to the first one that is somewhere.
		FString Line;
		FMissionTrackerParts Tracker;
		Tracker.Step = Run.Step;
		Tracker.StepCount = Mission->Steps.Num();
		// The HUD's mission-complete banner announces a story mission's end with the fanfare; the tracker's own tick stays a
		// chime then (the tutorial's end keeps the tracker's fanfare).
		Tracker.bAnnouncedEnd = Mission->Kind != EMissionKind::Tutorial;
		bool bLineFound = false;
		TOptional<FVector> Target;
		for (int32 Index = 0; Index < Run.States.Num(); ++Index)
		{
			const UMissionObjective* Objective = Mission->GetObjective(Run.Step, Index);
			const FMissionObjectiveState& State = Run.States[Index];
			if (!Objective || State.bDone)
			{
				continue;
			}
			if (!bLineFound)
			{
				Line = Objective->GetTrackerText(Context.World, State);
				Objective->FillTrackerParts(Context.World, State, Tracker);
				// Which one of the step it is: the tracker takes this moving on as the one before it done (a rebound key
				// only changes the words).
				Tracker.ObjectiveIndex = Index;
				bLineFound = true;
			}
			if (!Target.IsSet())
			{
				Target = Objective->FindWaypoint(Context, State);
			}
		}
		if (!bLineFound)
		{
			// Nothing left to ask for (every objective of the step done and the step not moved on yet, as when a run of steps
			// all done at once outlasts Settle's guard): the display keeps what it shows rather than an empty line, which the
			// tracker would take for a done objective.
			continue;
		}
		// Unchanged words and parts, and a waypoint that only moved, don't wake the display's listeners, so this is cheap
		// to repeat.
		Display->SetObjective(Run.BookId, FText::FromString(Line), Target, Tracker);
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
