#include "Creatures/DeadMinerShiftSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"

void ADeadMinerShiftSpawner::BeginPlay()
{
	// The shift's rule goes into its condition before the encounter reads it as play begins (an arrival).
	if (!bPlacedCaptured)
	{
		PlacedWhen = ActiveWhen;
		bPlacedCaptured = true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	bShiftWorks = !Runner || DeadMinerRules::ShiftStillWorks(Runner->GetCampaign(), ShiftMission, KeptFlag);
	ActiveWhen = PlacedWhen;
	if (!bShiftWorks)
	{
		ActiveWhen.BeforeMissions.AddUnique(ShiftMission);
	}
	Super::BeginPlay();
	if (UMissionRunner* Listened = UMissionRunner::Get(this))
	{
		BoundRunner = Listened;
		MissionsChangedHandle = Listened->OnChanged.AddUObject(this, &ADeadMinerShiftSpawner::HandleMissionsChanged);
	}
}

void ADeadMinerShiftSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void ADeadMinerShiftSpawner::HandleMissionsChanged()
{
	RefreshShift();
}

void ADeadMinerShiftSpawner::RefreshShift()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	ApplyShift(!Runner || DeadMinerRules::ShiftStillWorks(Runner->GetCampaign(), ShiftMission, KeptFlag));
}

void ADeadMinerShiftSpawner::ApplyShift(bool bWorks)
{
	if (!bPlacedCaptured)
	{
		PlacedWhen = ActiveWhen;
		bPlacedCaptured = true;
	}
	if (bWorks != bShiftWorks)
	{
		UE_LOG(LogLooter, Log, TEXT("Encounter %s: the shift %s."), *GetSpawnerId().ToString(),
			bWorks ? TEXT("works on (the payroll kept)") : TEXT("is over: the eleven are paid"));
	}
	bShiftWorks = bWorks;
	ActiveWhen = PlacedWhen;
	if (!bWorks)
	{
		// "Before Back Pay" no longer holds once it's finished: the encounter goes off as any does when its story ends.
		ActiveWhen.BeforeMissions.AddUnique(ShiftMission);
	}
	// Whichever heard the missions change first (the encounters' subsystem or this), the condition read last is this one.
	if (HasActorBegunPlay())
	{
		RefreshStory();
	}
}
