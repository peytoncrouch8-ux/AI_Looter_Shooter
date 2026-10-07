#include "World/StoryLighting.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "World/LightingStateSubsystem.h"

AStoryLighting::AStoryLighting()
{
	PrimaryActorTick.bCanEverTick = false;
}

FName AStoryLighting::PickState(const TArray<FStoryLightingRule>& InRules, const FCampaignRecord& Campaign, const UMissionRunner* Runner)
{
	for (const FStoryLightingRule& Rule : InRules)
	{
		// An empty condition always holds: it would light the level so from the start, which the level's own light does.
		if (!Rule.State.IsNone() && !Rule.When.IsEmpty() && Rule.When.IsMet(Campaign, Runner))
		{
			return Rule.State;
		}
	}
	return NAME_None;
}

void AStoryLighting::BeginPlay()
{
	Super::BeginPlay();
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AStoryLighting::HandleMissionsChanged);
	}
	// As the level begins (a session loaded mid-mission), at once: the level's load covers it.
	Refresh(/*bInstant*/ true);
}

void AStoryLighting::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	Super::EndPlay(EndPlayReason);
}

void AStoryLighting::HandleMissionsChanged()
{
	Refresh(/*bInstant*/ false);
}

void AStoryLighting::Refresh(bool bInstant)
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner)
	{
		return;
	}
	const FName Wanted = PickState(Rules, Runner->GetCampaign(), Runner);
	// None holds: the light stays as the last rule left it.
	if (Wanted.IsNone() || Wanted == Asked)
	{
		return;
	}
	Asked = Wanted;
	ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(this);
	if (!Lighting)
	{
		return;
	}
	if (Lighting->GetState() == Wanted && !Lighting->IsSwitching())
	{
		return;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the story asks for %s light%s."), *GetActorNameOrLabel(), *Wanted.ToString(), bInstant ? TEXT(" at once") : TEXT(""));
	Lighting->SetState(Wanted, bInstant ? ELightingSwitch::Instant : ELightingSwitch::Fade);
}
