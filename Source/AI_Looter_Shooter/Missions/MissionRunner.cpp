// UMissionRunner: its life in the level, the missions it knows, and what lists ask of it.

#include "Missions/MissionRunner.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Core/LooterMenuGameMode.h"
#include "Missions/MissionActorWatch.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UMissionRunner* UMissionRunner::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UMissionRunner>() : nullptr;
}

bool UMissionRunner::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests: there it waits for BeginForTesting.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UMissionRunner::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Played worlds only, and never behind the main menu: no one plays there. The session was loaded as the game mode
	// began (its InitGame), so the campaign record is ready; the actors begin play after this.
	if (bActive || !InWorld.IsGameWorld() || ALooterMenuGameMode::IsMenuWorld(&InWorld))
	{
		return;
	}
	for (UMissionDefinition* Mission : UMissionDefinition::LoadAll())
	{
		RegisterDefinition(Mission);
	}
	const UAreaDefinition* Area = UAreaDefinition::FindByMap(USessionSubsystem::MapOf(&InWorld));
	AreaHere = Area ? Area->GetAreaId() : NAME_None;
	Begin();
}

void UMissionRunner::BeginForTesting(const TArray<UMissionDefinition*>& Missions, FCampaignRecord& Campaign, AActor* Player, FName InArea)
{
	Definitions.Reset();
	for (UMissionDefinition* Mission : Missions)
	{
		RegisterDefinition(Mission);
	}
	TestCampaign = &Campaign;
	TestPlayer = Player;
	AreaHere = InArea;
	Begin();
}

void UMissionRunner::Begin()
{
	UWorld* World = GetWorld();
	if (bActive || !World)
	{
		return;
	}
	bActive = true;
	// What the session left running and what's due start on the first update, once the level is up: the player is
	// back where the session left them, so a place they stood in when the level began doesn't count as reached.
	bStartPending = true;
	WatchExisting();
	SpawnHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UMissionRunner::HandleActorSpawned));
	UE_LOG(LogLooter, Log, TEXT("Missions: %d known, %s"), Definitions.Num(),
		AreaHere.IsNone() ? TEXT("in a level that is no area's") : *FString::Printf(TEXT("in %s"), *AreaHere.ToString()));
}

void UMissionRunner::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(SpawnHandle);
	}
	SpawnHandle.Reset();
	bActive = false;
	Runs.Reset();
	Watches.Reset();
	WatchedActors.Reset();
	TestCampaign = nullptr;
	Super::Deinitialize();
}

void UMissionRunner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SinceUpdate += DeltaTime;
	if (SinceUpdate >= UpdateInterval)
	{
		const float Elapsed = SinceUpdate;
		SinceUpdate = 0.f;
		Update(Elapsed);
	}
}

bool UMissionRunner::IsTickable() const
{
	return bActive;
}

TStatId UMissionRunner::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMissionRunner, STATGROUP_Tickables);
}

// ---------------------------------------------------------------------------
// The missions it knows
// ---------------------------------------------------------------------------

UMissionDefinition* UMissionRunner::FindDefinition(FName MissionId) const
{
	if (MissionId.IsNone())
	{
		return nullptr;
	}
	const TObjectPtr<UMissionDefinition>* Found = Definitions.FindByPredicate([MissionId](const TObjectPtr<UMissionDefinition>& Mission)
	{
		return Mission && Mission->GetMissionId() == MissionId;
	});
	return Found ? Found->Get() : nullptr;
}

void UMissionRunner::RegisterDefinition(UMissionDefinition* Mission)
{
	if (!Mission)
	{
		return;
	}
	if (FindDefinition(Mission->GetMissionId()))
	{
		UE_LOG(LogLooter, Warning, TEXT("Missions: %s has the id %s, which another mission has already: it's left out."), *Mission->GetName(),
			*Mission->GetMissionId().ToString());
		return;
	}
	Definitions.Add(Mission);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

FMissionContext UMissionRunner::MakeContext() const
{
	FMissionContext Context;
	Context.World = GetWorld();
	if (TestCampaign)
	{
		// A test's world has no player controller: its stand-in is the player.
		Context.Player = TestPlayer.Get();
		return Context;
	}
	Context.Controller = Context.World ? Context.World->GetFirstPlayerController() : nullptr;
	Context.Player = Context.Controller ? Context.Controller->GetPawn() : nullptr;
	return Context;
}

UMissionRunner::FRun* UMissionRunner::FindRun(FName MissionId)
{
	return Runs.FindByPredicate([MissionId](const FRun& Run) { return Run.Id == MissionId; });
}

const UMissionRunner::FRun* UMissionRunner::FindRun(FName MissionId) const
{
	return Runs.FindByPredicate([MissionId](const FRun& Run) { return Run.Id == MissionId; });
}

bool UMissionRunner::IsRunning(FName MissionId) const
{
	return !MissionId.IsNone() && FindRun(MissionId) != nullptr;
}

int32 UMissionRunner::GetStep(FName MissionId) const
{
	const FRun* Run = FindRun(MissionId);
	return Run ? Run->Step : INDEX_NONE;
}

bool UMissionRunner::IsCompleted(FName MissionId) const
{
	return GetCampaign().HasCompleted(MissionId);
}

bool UMissionRunner::ArePrerequisitesMet(const UMissionDefinition& Mission) const
{
	const FCampaignRecord& Campaign = GetCampaign();
	for (const FName Needed : Mission.Prerequisites)
	{
		if (!Needed.IsNone() && !Campaign.HasCompleted(Needed))
		{
			return false;
		}
	}
	return true;
}

bool UMissionRunner::IsInArea(const UMissionDefinition& Mission) const
{
	return Mission.Area.IsNone() || Mission.Area == AreaHere;
}

bool UMissionRunner::CanStart(const UMissionDefinition& Mission) const
{
	return !IsCompleted(Mission.GetMissionId()) && ArePrerequisitesMet(Mission) && IsInArea(Mission);
}

EMissionStatus UMissionRunner::GetStatus(const UMissionDefinition& Mission) const
{
	const FName MissionId = Mission.GetMissionId();
	const FCampaignRecord& Campaign = GetCampaign();
	// Running beats finished: a mission played again is active while it runs.
	if (IsRunning(MissionId) || Campaign.ActiveMission == MissionId)
	{
		return EMissionStatus::Active;
	}
	if (Campaign.HasCompleted(MissionId))
	{
		return EMissionStatus::Completed;
	}
	if (Mission.Start != EMissionStart::Manual && ArePrerequisitesMet(Mission))
	{
		return EMissionStatus::Available;
	}
	return EMissionStatus::Locked;
}

TArray<FMissionObjectiveView> UMissionRunner::GetObjectiveViews(FName MissionId) const
{
	TArray<FMissionObjectiveView> Views;
	const FRun* Run = FindRun(MissionId);
	const UMissionDefinition* Mission = Run ? Run->Mission.Get() : nullptr;
	if (!Mission)
	{
		return Views;
	}
	for (int32 Index = 0; Index < Run->States.Num(); ++Index)
	{
		const UMissionObjective* Objective = Mission->GetObjective(Run->Step, Index);
		if (!Objective)
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
	}
	return Views;
}

FCampaignRecord& UMissionRunner::GetCampaign() const
{
	if (TestCampaign)
	{
		return *TestCampaign;
	}
	// The session's: saved with it, or kept in memory for a play without one (USessionSubsystem::GetCampaign).
	USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	FCampaignRecord* Campaign = Sessions ? Sessions->GetCampaign() : nullptr;
	return Campaign ? *Campaign : LocalCampaign;
}

// ---------------------------------------------------------------------------
// Tracking
// ---------------------------------------------------------------------------

FName UMissionRunner::GetTrackedMission() const
{
	const UMissionSubsystem* Display = GetWorld() ? GetWorld()->GetSubsystem<UMissionSubsystem>() : nullptr;
	const int32 Tracked = Display ? Display->GetTrackedMission() : INDEX_NONE;
	const FRun* Run = Tracked == INDEX_NONE ? nullptr : Runs.FindByPredicate([Tracked](const FRun& Each) { return Each.BookId == Tracked; });
	return Run ? Run->Id : NAME_None;
}

void UMissionRunner::TrackMission(FName MissionId)
{
	UMissionSubsystem* Display = GetWorld() ? GetWorld()->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (!Display)
	{
		return;
	}
	const FRun* Run = FindRun(MissionId);
	const int32 Before = Display->GetTrackedMission();
	Display->TrackMission(Run ? Run->BookId : INDEX_NONE);
	if (Display->GetTrackedMission() != Before)
	{
		OnChanged.Broadcast();
	}
}

bool UMissionRunner::IsWatching(const AActor* Actor) const
{
	return Actor && WatchedActors.Contains(FObjectKey(Actor));
}
