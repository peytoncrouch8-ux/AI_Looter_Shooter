// AStoryProp: a model the story shows and hides, read again whenever the missions change.

#include "World/StoryProp.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

AStoryProp::AStoryProp()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Seen, never touched: tags on a hook, a plume, a book on a shelf. The Interact key's line passes it too.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
}

void AStoryProp::BeginPlay()
{
	Super::BeginPlay();
	bShownNow = true;
	// What's there follows the story: it looks again whenever the missions change (a step, a turn-in's choice, a flag).
	if (ShownWhen.Num() > 0 || HiddenWhen.Num() > 0)
	{
		if (UMissionRunner* Runner = UMissionRunner::Get(this))
		{
			BoundRunner = Runner;
			ChangedHandle = Runner->OnChanged.AddUObject(this, &AStoryProp::HandleRunnerChanged);
		}
	}
	Refresh();
}

void AStoryProp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

bool AStoryProp::Applies(const TArray<FStoryCondition>& Shown, const TArray<FStoryCondition>& Hidden, const FCampaignRecord* Campaign,
	const UMissionRunner* Runner)
{
	if (!Campaign)
	{
		return Shown.Num() == 0;
	}
	auto Holds = [Campaign, Runner](const FStoryCondition& When) { return When.IsMet(*Campaign, Runner); };
	const bool bShown = Shown.Num() == 0 || Shown.ContainsByPredicate(Holds);
	return bShown && !Hidden.ContainsByPredicate(Holds);
}

bool AStoryProp::ShouldShow() const
{
	if (ShownWhen.Num() == 0 && HiddenWhen.Num() == 0)
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Applies(ShownWhen, HiddenWhen, Runner ? &Runner->GetCampaign() : nullptr, Runner);
}

void AStoryProp::Refresh()
{
	SetShown(ShouldShow());
}

void AStoryProp::HandleRunnerChanged()
{
	Refresh();
}

void AStoryProp::SetShown(bool bShown)
{
	if (bShown == bShownNow)
	{
		return;
	}
	bShownNow = bShown;
	SetActorHiddenInGame(!bShown);
	UE_LOG(LogLooter, Verbose, TEXT("%s: %s by the story."), *GetActorNameOrLabel(), bShown ? TEXT("shown") : TEXT("hidden"));
}
