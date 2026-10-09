// UMissionRunner: starting missions, moving them step by step, the wait for the turn-in after the last, finishing them,
// their rewards and the campaign record. The turn-in itself (a talk to the giver) is MissionRunnerTurnIn.cpp's.

#include "Missions/MissionRunner.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRewards.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Steps passed in one go at most: a mission whose steps are all done already still ends. */
	constexpr int32 MaxStepsAtOnce = 64;
}

// ---------------------------------------------------------------------------
// Starting
// ---------------------------------------------------------------------------

bool UMissionRunner::StartMission(FName MissionId, int32 Step, bool bForce)
{
	UMissionDefinition* Mission = FindDefinition(MissionId);
	if (!bActive || !Mission)
	{
		return false;
	}
	if (Mission->Steps.IsEmpty())
	{
		UE_LOG(LogLooter, Warning, TEXT("Missions: %s has no steps, so it can't be played."), *MissionId.ToString());
		return false;
	}
	if (!bForce && (IsRunning(MissionId) || !CanStart(*Mission)))
	{
		return false;
	}
	if (bForce && GetCampaign().ClearReady(MissionId))
	{
		// Started over on purpose (the console): it's played from Step, not waiting for its turn-in any more.
		AskToSave();
	}
	StartRun(*Mission, Step);
	AfterChange();
	return true;
}

void UMissionRunner::StartRun(UMissionDefinition& Mission, int32 Step)
{
	const FName MissionId = Mission.GetMissionId();
	FRun* Run = FindRun(MissionId);
	if (!Run)
	{
		Run = &Runs.AddDefaulted_GetRef();
		Run->Id = MissionId;
		Run->Mission = &Mission;
	}
	Run->bReady = false;
	bChanged = true;
	// Its objectives were done in an earlier level or session: it comes back waiting for its turn-in, wherever it was
	// left, side missions too (the campaign keeps only the main one's step). So does one saved on a step past its last: a
	// save from before turn-ins, on the talk with its giver that is its turn-in now (Main 1, 3 and 4's last step), or past
	// a step that went (Main 7's depot and board, Heirloom already handed out).
	if (GetCampaign().IsReadyToTurnIn(MissionId) || (Mission.NeedsTurnIn() && Step >= Mission.Steps.Num()))
	{
		EnterReady(*Run);
		return;
	}
	Run->Step = FMath::Clamp(Step, 0, Mission.Steps.Num() - 1);
	BeginStep(*Run);
	RecordStep(*Run);
	UE_LOG(LogLooter, Log, TEXT("Mission %s started at step %d/%d"), *MissionId.ToString(), Run->Step + 1, Mission.Steps.Num());
}

void UMissionRunner::StartDue(FName StartedBy)
{
	// Automatic missions whenever they're due; OnEvent ones only on their event.
	for (UMissionDefinition* Mission : Definitions)
	{
		const bool bDue = Mission && (StartedBy.IsNone()
			? Mission->Start == EMissionStart::Automatic
			: Mission->Start == EMissionStart::OnEvent && Mission->StartEvent == StartedBy);
		if (bDue && !Mission->Steps.IsEmpty() && !IsRunning(Mission->GetMissionId()) && CanStart(*Mission))
		{
			StartRun(*Mission, 0);
		}
	}
}

void UMissionRunner::ResumeRecorded()
{
	// The main mission the session was playing goes on from its step, when this is its area. Elsewhere it waits, still
	// the campaign's mission, for the player to come back.
	const FCampaignRecord& Campaign = GetCampaign();
	UMissionDefinition* Mission = FindDefinition(Campaign.ActiveMission);
	if (Mission && Mission->Start != EMissionStart::Manual && !Mission->Steps.IsEmpty() && !IsRunning(Campaign.ActiveMission)
		&& !Campaign.HasCompleted(Campaign.ActiveMission) && IsInArea(*Mission))
	{
		StartRun(*Mission, Campaign.ActiveMissionStep);
	}

	// Every mission waiting for its turn-in in this area comes back waiting, whatever starts it: a side mission's objectives
	// done in an earlier session aren't asked again. (A copy: starting one may write the record.)
	const TArray<FName> Waiting = Campaign.ReadyMissions;
	for (const FName ReadyId : Waiting)
	{
		UMissionDefinition* Ready = FindDefinition(ReadyId);
		if (Ready && !Ready->Steps.IsEmpty() && !IsRunning(ReadyId) && !Campaign.HasCompleted(ReadyId) && IsInArea(*Ready))
		{
			StartRun(*Ready, Ready->Steps.Num());
		}
	}
}

// ---------------------------------------------------------------------------
// Steps
// ---------------------------------------------------------------------------

void UMissionRunner::BeginStep(FRun& Run)
{
	Run.States.Reset();
	const UMissionDefinition* Mission = Run.Mission.Get();
	if (!Mission || !Mission->Steps.IsValidIndex(Run.Step))
	{
		return;
	}
	const FMissionContext Context = MakeContext();
	const int32 NumObjectives = Mission->Steps[Run.Step].Objectives.Num();
	Run.States.SetNum(NumObjectives);
	for (int32 Index = 0; Index < NumObjectives; ++Index)
	{
		const UMissionObjective* Objective = Mission->GetObjective(Run.Step, Index);
		FMissionObjectiveState& State = Run.States[Index];
		if (!Objective)
		{
			// An empty entry in the data asks for nothing.
			State.bDone = true;
			continue;
		}
		// A look at once, as the tutorial did: a gun already carried, a place already stood in, pass straight away.
		Objective->Begin(Context, State);
		Objective->Update(Context, State, 0.f);
		RefreshDone(*Objective, Context, State);
	}
}

bool UMissionRunner::IsStepDone(const FRun& Run) const
{
	for (const FMissionObjectiveState& State : Run.States)
	{
		if (!State.bDone)
		{
			return false;
		}
	}
	return true;
}

bool UMissionRunner::AdvanceStep(FRun& Run)
{
	const UMissionDefinition* Mission = Run.Mission.Get();
	if (!Mission || Run.Step + 1 >= Mission->Steps.Num())
	{
		return false;
	}
	++Run.Step;
	BeginStep(Run);
	RecordStep(Run);
	bChanged = true;
	UE_LOG(LogLooter, Log, TEXT("Mission %s: step %d/%d"), *Run.Id.ToString(), Run.Step + 1, Mission->Steps.Num());
	return true;
}

void UMissionRunner::EnterReady(FRun& Run)
{
	const UMissionDefinition* Mission = Run.Mission.Get();
	Run.bReady = true;
	// One past its last step, so the story's "from the last step" conditions still hold while it waits (a giver's lines for
	// that talk, Hob's perch for it), and "before the last step" ones don't.
	Run.Step = Mission ? Mission->Steps.Num() : Run.Step;
	Run.States.Reset();
	if (GetCampaign().MarkReady(Run.Id))
	{
		AskToSave();
	}
	RecordStep(Run);
	bChanged = true;
	UE_LOG(LogLooter, Log, TEXT("Mission %s: objectives done, ready to turn in to %s"), *Run.Id.ToString(),
		Mission ? *Mission->GetGiverName().ToString() : TEXT("its giver"));
}

bool UMissionRunner::SetStep(FName MissionId, int32 Step)
{
	FRun* Run = FindRun(MissionId);
	const UMissionDefinition* Mission = Run ? Run->Mission.Get() : nullptr;
	if (!Mission || !Mission->Steps.IsValidIndex(Step))
	{
		return false;
	}
	// Back to playing a step: no longer waiting for its turn-in.
	Run->bReady = false;
	if (GetCampaign().ClearReady(MissionId))
	{
		AskToSave();
	}
	Run->Step = Step;
	BeginStep(*Run);
	RecordStep(*Run);
	bChanged = true;
	AfterChange();
	return true;
}

bool UMissionRunner::CompleteStep(FName MissionId)
{
	FRun* Run = FindRun(MissionId);
	const UMissionDefinition* Mission = Run ? Run->Mission.Get() : nullptr;
	if (!Mission)
	{
		return false;
	}
	// Nothing left but the turn-in: that's the step to finish.
	if (Run->bReady)
	{
		return TurnIn(MissionId);
	}
	for (int32 Index = 0; Index < Run->States.Num(); ++Index)
	{
		const UMissionObjective* Objective = Mission->GetObjective(Run->Step, Index);
		FMissionObjectiveState& State = Run->States[Index];
		State.Count = Objective ? FMath::Max(State.Count, Objective->GetRequired()) : State.Count;
		State.bDone = true;
	}
	bChanged = true;
	AfterChange();
	return true;
}

void UMissionRunner::Settle()
{
	// A step done as it begins passes at once, and so on; a mission past its last step waits for its turn-in, or ends
	// (an automatic one), and the missions it was the last prerequisite of start (their first steps may be done already
	// too). Waiting for a turn-in unlocks nothing: only the turn-in finishes it.
	for (int32 Pass = 0; Pass < MaxStepsAtOnce; ++Pass)
	{
		TArray<FName> Finished;
		for (FRun& Run : Runs)
		{
			const UMissionDefinition* Mission = Run.Mission.Get();
			if (Run.bReady)
			{
				// Kept ready by the campaign, but its data asks for no turn-in now: it's done.
				if (!Mission || !Mission->NeedsTurnIn())
				{
					Finished.Add(Run.Id);
				}
				continue;
			}
			for (int32 Guard = 0; Guard < MaxStepsAtOnce && IsStepDone(Run); ++Guard)
			{
				if (!AdvanceStep(Run))
				{
					if (Mission && Mission->NeedsTurnIn())
					{
						EnterReady(Run);
					}
					else
					{
						Finished.Add(Run.Id);
					}
					break;
				}
			}
		}
		if (Finished.IsEmpty())
		{
			return;
		}
		for (const FName MissionId : Finished)
		{
			FinishRun(MissionId);
		}
		StartDue(NAME_None);
	}
}

void UMissionRunner::RecordStep(const FRun& Run)
{
	// The campaign record keeps one mission and its step: the main one (the story plays one at a time), its step count
	// once it waits for its turn-in. Side missions start again from their first step in the next level or session (unless
	// they wait for their turn-in: ReadyMissions); tutorial missions' owners keep their own steps.
	const UMissionDefinition* Mission = Run.Mission.Get();
	if (!Mission || Mission->Kind != EMissionKind::Main)
	{
		return;
	}
	FCampaignRecord& Campaign = GetCampaign();
	if (Campaign.ActiveMission != Run.Id || Campaign.ActiveMissionStep != Run.Step)
	{
		Campaign.ActiveMission = Run.Id;
		Campaign.ActiveMissionStep = Run.Step;
		AskToSave();
	}
}

// ---------------------------------------------------------------------------
// Finishing
// ---------------------------------------------------------------------------

bool UMissionRunner::CompleteMission(FName MissionId)
{
	const UMissionDefinition* Mission = FindDefinition(MissionId);
	if (!bActive || !Mission)
	{
		return false;
	}
	if (IsRunning(MissionId))
	{
		FinishRun(MissionId);
	}
	else
	{
		FMissionRewardsGiven Given;
		const bool bRewarded = RecordCompletion(*Mission, /*bGiveRewards*/ true, &Given);
		BroadcastFinished(*Mission, bRewarded, Given);
		bChanged = true;
	}
	StartDue(NAME_None);
	AfterChange();
	return true;
}

void UMissionRunner::RecordCompleted(FName MissionId)
{
	const UMissionDefinition* Mission = FindDefinition(MissionId);
	if (!Mission || IsCompleted(MissionId))
	{
		return;
	}
	RecordCompletion(*Mission, /*bGiveRewards*/ false);
	bChanged = true;
	// As the level begins (the tutorial director's BeginPlay) the first update starts what this unlocks.
	if (bActive && !bStartPending)
	{
		StartDue(NAME_None);
		AfterChange();
	}
}

void UMissionRunner::FinishRun(FName MissionId)
{
	const int32 Index = Runs.IndexOfByPredicate([MissionId](const FRun& Run) { return Run.Id == MissionId; });
	if (Index == INDEX_NONE)
	{
		return;
	}
	FRun Finished = MoveTemp(Runs[Index]);
	Runs.RemoveAt(Index);
	RemoveFromDisplay(Finished);
	bChanged = true;
	if (const UMissionDefinition* Mission = Finished.Mission.Get())
	{
		FMissionRewardsGiven Given;
		const bool bRewarded = RecordCompletion(*Mission, /*bGiveRewards*/ true, &Given);
		BroadcastFinished(*Mission, bRewarded, Given);
	}
}

bool UMissionRunner::RecordCompletion(const UMissionDefinition& Mission, bool bGiveRewards, FMissionRewardsGiven* OutGiven)
{
	const FName MissionId = Mission.GetMissionId();
	FCampaignRecord& Campaign = GetCampaign();
	// Rewards come once per session: a mission finished before (played again, or from the console) gives nothing more.
	const bool bFirstTime = !Campaign.HasCompleted(MissionId);
	Campaign.Complete(MissionId);
	if (bFirstTime && bGiveRewards)
	{
		const FMissionRewardsGiven Given = GrantRewards(Mission);
		if (OutGiven)
		{
			*OutGiven = Given;
		}
	}
	AskToSave();
	UE_LOG(LogLooter, Log, TEXT("Mission %s finished%s"), *MissionId.ToString(),
		!bFirstTime ? TEXT(" again (no rewards)") : bGiveRewards ? TEXT(", rewards given") : TEXT(" (recorded)"));
	return bFirstTime && bGiveRewards;
}

FMissionRewardsGiven UMissionRunner::GrantRewards(const UMissionDefinition& Mission)
{
	const FMissionRewards& Rewards = Mission.Rewards;
	const FMissionContext Context = MakeContext();
	FMissionRewardsGiven Given;

	// Experience: the mission's fixed amount (or its share of the level the player is on). This and a kill are the only
	// ways experience is earned: never for an objective or a step on the way. Without a player (a test level) it's still
	// worked out, at level 1, for whoever hears what the mission gave.
	int32 PlayerLevel = 1;
	const ULocalPlayer* LocalPlayer = Context.Controller ? Context.Controller->GetLocalPlayer() : nullptr;
	UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	if (Progression)
	{
		PlayerLevel = Progression->GetLevel();
	}
	Given.Experience = MissionRewards::ExperienceOf(Rewards, PlayerLevel, UPlayerProgressionSubsystem::GetCurve());
	if (Progression && Given.Experience > 0)
	{
		Progression->AddXP(Given.Experience, EXPSource::Mission);
	}

	// A gun at the player's feet, at their level, and a named gun the same way, unless the story hands that one over itself
	// (Heirloom through Delia's door).
	if (Rewards.bGun)
	{
		if (const AWeaponBase* Gun = MissionRewards::DropGun(Context.World, Rewards, PlayerLevel, Context.Player))
		{
			const FWeaponInstanceData& Instance = Gun->GetInstance();
			Given.bGun = true;
			Given.GunRarity = Instance.Rarity;
			Given.GunColor = UWeaponRollLibrary::GetRarityColor(Instance.Definition, Instance.Rarity);
		}
	}
	if (!Rewards.NamedGun.IsNone())
	{
		if (!Rewards.bNamedGunByHand)
		{
			MissionRewards::DropNamedGun(Context.World, Rewards, PlayerLevel, Context.Player);
		}
		Given.NamedGun = MissionRewards::NamedGunName(Rewards);
	}

	// Areas opened to travel.
	FCampaignRecord& Campaign = GetCampaign();
	for (const FName AreaId : Rewards.UnlockAreas)
	{
		if (Campaign.OpenArea(AreaId))
		{
			Given.AreasOpened.Add(MissionRewards::AreaName(AreaId));
			UE_LOG(LogLooter, Log, TEXT("Missions: %s opens %s"), *Mission.GetMissionId().ToString(), *AreaId.ToString());
		}
	}
	return Given;
}

void UMissionRunner::AskToSave() const
{
	// A few seconds from now, so a fight's worth of progress makes one write; nothing without a session.
	if (!TestCampaign)
	{
		if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
		{
			Sessions->SaveSoon();
		}
	}
}
