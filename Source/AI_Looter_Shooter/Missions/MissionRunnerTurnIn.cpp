// UMissionRunner: turning missions in. A mission whose objectives are all done waits for its giver (Borderlands' way): a
// talk at their speaker point turns it in, and only then is it finished, its rewards given and its end announced. Its
// line in the display meanwhile says who, with the arrow on them.

#include "Missions/MissionRunner.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionSubsystem.h"
#include "Missions/MissionTargets.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** The giver's speaker tag is on Speaker, or (no actor: the console's Talk event) the event names that tag. */
	bool IsGiver(const UMissionDefinition& Mission, const AActor* Speaker, FName EventTag)
	{
		const FName Tag = Mission.TurnIn.SpeakerTag;
		if (Tag.IsNone())
		{
			return false;
		}
		return Speaker ? Speaker->ActorHasTag(Tag) : EventTag == Tag;
	}
}

// ---------------------------------------------------------------------------
// Turning in
// ---------------------------------------------------------------------------

bool UMissionRunner::TurnIn(FName MissionId)
{
	const FRun* Run = FindRun(MissionId);
	if (!bActive || !Run || !Run->bReady)
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("Mission %s turned in"), *MissionId.ToString());
	FinishRun(MissionId);
	// What it was the last prerequisite of starts now (Main N+1 after Main N is turned in).
	StartDue(NAME_None);
	AfterChange();
	return true;
}

bool UMissionRunner::TurnInAt(const FMissionEvent& Event)
{
	if (Event.Name != FMissionEvent::Talk)
	{
		return false;
	}
	const AActor* Speaker = Event.Actor.Get();
	TArray<FName> Due;
	for (const FRun& Run : Runs)
	{
		const UMissionDefinition* Mission = Run.Mission.Get();
		if (Run.bReady && Mission && Mission->NeedsTurnIn() && IsGiver(*Mission, Speaker, Event.Tag))
		{
			Due.Add(Run.Id);
		}
	}
	// Everything ready for this giver at once: two missions handed to the same person are both done by one talk.
	for (const FName MissionId : Due)
	{
		UE_LOG(LogLooter, Log, TEXT("Mission %s turned in to %s"), *MissionId.ToString(), Speaker ? *Speaker->GetActorNameOrLabel() : *Event.Tag.ToString());
		FinishRun(MissionId);
	}
	return !Due.IsEmpty();
}

void UMissionRunner::BroadcastFinished(const UMissionDefinition& Mission, bool bRewarded, const FMissionRewardsGiven& Given)
{
	OnMissionFinished.Broadcast(Mission, bRewarded);
	// Nothing given when it was finished before: the announcement still says it's done, with no rewards.
	OnMissionCompleted.Broadcast(Mission, bRewarded ? Given : FMissionRewardsGiven());
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool UMissionRunner::IsReadyToTurnIn(FName MissionId) const
{
	if (MissionId.IsNone())
	{
		return false;
	}
	const FRun* Run = FindRun(MissionId);
	return Run ? Run->bReady : GetCampaign().IsReadyToTurnIn(MissionId);
}

const UMissionDefinition* UMissionRunner::FindTurnInAt(const AActor* Speaker) const
{
	if (!Speaker)
	{
		return nullptr;
	}
	for (const FRun& Run : Runs)
	{
		const UMissionDefinition* Mission = Run.Mission.Get();
		if (Run.bReady && Mission && Mission->NeedsTurnIn() && IsGiver(*Mission, Speaker, NAME_None))
		{
			return Mission;
		}
	}
	return nullptr;
}

TOptional<FVector> UMissionRunner::FindGiver(const UMissionDefinition& Mission) const
{
	if (Mission.TurnIn.SpeakerTag.IsNone())
	{
		return TOptional<FVector>();
	}
	FMissionActorFilter Giver;
	Giver.ActorTag = Mission.TurnIn.SpeakerTag;
	const FMissionContext Context = MakeContext();
	// Anyone carrying the tag, living or not: a door, a window, a ghost on a fence.
	const AActor* Found = MissionTargets::FindNearest(Context.World, Giver, Context.GetPlayerLocation(), /*bLivingOnly*/ false);
	return Found ? TOptional<FVector>(Found->GetActorLocation()) : TOptional<FVector>();
}

// ---------------------------------------------------------------------------
// The display
// ---------------------------------------------------------------------------

void UMissionRunner::SyncTurnIn(const FRun& Run, const UMissionDefinition& Mission, UMissionSubsystem& Display) const
{
	// Every step done (the step bar full), and the turn-in as the objective: the step one past the last, so the HUD's
	// tracker ticks the last objective, then slides this in.
	FMissionTrackerParts Tracker;
	Tracker.Line = Mission.GetTurnInShortText();
	Tracker.Step = Run.Step;
	Tracker.StepCount = FMath::Max(Mission.Steps.Num(), 1);
	Tracker.ObjectiveIndex = 0;
	Tracker.bTurnIn = true;
	Tracker.bAnnouncedEnd = Mission.Kind != EMissionKind::Tutorial;
	Display.SetObjective(Run.BookId, FText::FromString(Mission.GetTurnInText()), FindGiver(Mission), Tracker);
}
