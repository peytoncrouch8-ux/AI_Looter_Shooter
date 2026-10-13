// UMissionRunner: optional objectives (UMissionObjective::bOptional), Borderlands' way. They never hold a step, stay doable
// until it ends (the last step's until the mission is turned in), are kept done by the campaign so a step played again
// starts with them done, and each pays its mission's bonus at the turn-in (MissionRunnerRewards.cpp).

#include "Missions/MissionRunner.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionSubsystem.h"
#include "Engine/World.h"

int32 UMissionRunner::StatesStep(const FRun& Run) const
{
	const UMissionDefinition* Mission = Run.Mission.Get();
	// Ready to turn in, its step is one past its last; its States are still the last step's.
	return Run.bReady && Mission ? Mission->Steps.Num() - 1 : Run.Step;
}

void UMissionRunner::NoteOptionalDone(const FRun& Run, int32 Index)
{
	const int32 Step = StatesStep(Run);
	if (GetCampaign().MarkOptionalDone(Run.Id, Step, Index))
	{
		UE_LOG(LogLooter, Log, TEXT("Mission %s: optional objective %d of step %d done."), *Run.Id.ToString(), Index + 1, Step + 1);
		AskToSave();
	}
}

void UMissionRunner::PrepareReadyStates(FRun& Run)
{
	Run.States.Reset();
	const UMissionDefinition* Mission = Run.Mission.Get();
	const int32 Last = Mission ? Mission->Steps.Num() - 1 : INDEX_NONE;
	if (!Mission || !Mission->Steps.IsValidIndex(Last))
	{
		return;
	}
	// What the last step asks for is done (that's why it waits); its optional objectives as the campaign kept them, the
	// ones not done yet looked at afresh, so they can still be done before the turn-in.
	const FMissionContext Context = MakeContext();
	const int32 NumObjectives = Mission->Steps[Last].Objectives.Num();
	Run.States.SetNum(NumObjectives);
	for (int32 Index = 0; Index < NumObjectives; ++Index)
	{
		const UMissionObjective* Objective = Mission->GetObjective(Last, Index);
		FMissionObjectiveState& State = Run.States[Index];
		if (!Objective || !Objective->bOptional || GetCampaign().HasDoneOptional(Run.Id, Last, Index))
		{
			State.Count = Objective ? Objective->GetRequired() : 0;
			State.bDone = true;
			continue;
		}
		Objective->Begin(Context, State);
		Objective->Update(Context, State, 0.f);
		if (RefreshDone(*Objective, Context, State))
		{
			// Run.bReady is set: the key is the last step's.
			NoteOptionalDone(Run, Index);
		}
	}
}

TArray<FMissionObjectiveView> UMissionRunner::GetOptionalViews(FName MissionId) const
{
	TArray<FMissionObjectiveView> Views;
	const FRun* Run = FindRun(MissionId);
	const UMissionDefinition* Mission = Run ? Run->Mission.Get() : nullptr;
	if (!Mission)
	{
		return Views;
	}
	const int32 Step = StatesStep(*Run);
	for (int32 Index = 0; Index < Run->States.Num(); ++Index)
	{
		const UMissionObjective* Objective = Mission->GetObjective(Step, Index);
		if (!Objective || !Objective->bOptional)
		{
			continue;
		}
		const FMissionObjectiveState& State = Run->States[Index];
		FMissionObjectiveView& View = Views.AddDefaulted_GetRef();
		View.Text = Objective->GetDisplayText(GetWorld());
		View.Required = Objective->GetRequired();
		View.Progress = View.Required > 1 ? Objective->FormatProgress(State) : FString();
		View.Fraction = Objective->GetFraction(State);
		View.bDone = State.bDone;
		View.bOptional = true;
	}
	return Views;
}

int32 UMissionRunner::CountOptionalDone(FName MissionId) const
{
	return GetCampaign().CountOptionalDone(MissionId);
}

void UMissionRunner::FillOptionalParts(const FRun& Run, FMissionTrackerParts& Tracker) const
{
	Tracker.Optionals.Reset();
	const UMissionDefinition* Mission = Run.Mission.Get();
	if (!Mission)
	{
		return;
	}
	const UWorld* World = GetWorld();
	const int32 Step = StatesStep(Run);
	for (int32 Index = 0; Index < Run.States.Num(); ++Index)
	{
		const UMissionObjective* Objective = Mission->GetObjective(Step, Index);
		if (!Objective || !Objective->bOptional)
		{
			continue;
		}
		const FMissionObjectiveState& State = Run.States[Index];
		FMissionTrackerOptional& Optional = Tracker.Optionals.AddDefaulted_GetRef();
		Optional.Line = Objective->GetShortText(World);
		Optional.bDone = State.bDone;
		// A count only while there's more than one thing to it and it isn't done (the tick says the rest).
		if (Objective->bShowCount && Objective->GetRequired() > 1 && !State.bDone)
		{
			Optional.Count = Objective->FormatProgress(State);
		}
	}
}
