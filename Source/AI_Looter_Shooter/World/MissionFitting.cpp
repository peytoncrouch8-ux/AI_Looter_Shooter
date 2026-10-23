// AMissionFitting: where a mission has the player put something to work (a crank, coal, a pin), opened and closed by the
// story, its model shown once it's done.

#include "World/MissionFitting.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionTypes.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

AMissionFitting::AMissionFitting()
{
	// A held use, once: the hand on the crank for a moment and it's fitted for good.
	bHold = true;
	HoldSeconds = 1.5f;
	UseMode = EInteractablePropUse::TurnOn;
	Effect = EInteractablePropEffect::None;
	Prompt = FText::FromString(TEXT("Fit it"));

	// A small thing in a hand's reach: the Interact key's line finds it, nothing bumps into it (the build script makes a
	// gate solid again).
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
}

void AMissionFitting::BeginPlay()
{
	Super::BeginPlay();
	bUsedHere = false;
	UsedHandle = OnUsedNative.AddUObject(this, &AMissionFitting::HandleUsed);
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &AMissionFitting::HandleRunnerChanged);
	}
	Refresh();
}

void AMissionFitting::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	OnUsedNative.Remove(UsedHandle);
	UsedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Its story
// ---------------------------------------------------------------------------

bool AMissionFitting::IsDoneByStory() const
{
	if (DoneWhen.Num() == 0)
	{
		return false;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner)
	{
		return false;
	}
	const FCampaignRecord& Campaign = Runner->GetCampaign();
	return DoneWhen.ContainsByPredicate([&Campaign, Runner](const FStoryCondition& When) { return When.IsMet(Campaign, Runner); });
}

bool AMissionFitting::IsDone() const
{
	return bUsedHere || IsOn() || IsDoneByStory();
}

bool AMissionFitting::CanFit() const
{
	if (IsDone())
	{
		return false;
	}
	if (UsableWhen.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && UsableWhen.IsMet(Runner->GetCampaign(), Runner);
}

void AMissionFitting::Refresh()
{
	const bool bDone = IsDone();
	if (Mesh)
	{
		Mesh->SetVisibility(!bHideUntilDone || bDone);
	}
	// Done by the story (a later step, a load after the mission): on for good, so it can't be used again.
	if (bDone && !IsOn())
	{
		SetOn(true, /*bInstant*/ true);
	}
}

void AMissionFitting::HandleRunnerChanged()
{
	Refresh();
}

// ---------------------------------------------------------------------------
// Being used
// ---------------------------------------------------------------------------

FInteractionOptions AMissionFitting::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options = Super::GetInteractionOptions(User);
	// Never focused while the story keeps it shut, or once done: no prompt waits before its step or after the mission.
	Options.bUsable = Options.bUsable && CanFit();
	return Options;
}

bool AMissionFitting::Interact(UInteractionComponent& User, bool bHeld)
{
	return CanFit() && Super::Interact(User, bHeld);
}

TOptional<FVector> AMissionFitting::GetInteractionLocation() const
{
	// A model hidden until done (a crank, a pin) is fitted at the actor's own spot (the build script's socket), wherever the
	// model lies; one always seen (a gate) is used where it is, its middle.
	return bHideUntilDone ? TOptional<FVector>(GetActorLocation()) : Super::GetInteractionLocation();
}

TOptional<FInteractionBox> AMissionFitting::GetInteractionBox() const
{
	// A model always seen (a gate): anywhere on it. One hidden until done: the place the hand goes, a hand's room round it.
	if (!bHideUntilDone && Mesh && Mesh->GetStaticMesh() && Mesh->IsVisible())
	{
		return InteractionFocus::GrownBox(Mesh->Bounds.GetBox(), LookHalfSize.X, LookHalfSize.Z);
	}
	return InteractionFocus::BoxAround(GetActorLocation(), LookHalfSize.GetAbs(), GetActorQuat());
}

void AMissionFitting::HandleUsed(AInteractableProp& /*Prop*/, AActor* /*User*/, bool /*bHeld*/)
{
	bUsedHere = true;
	if (!UseCue.IsNone())
	{
		LooterSound::PlayAt(this, UseCue, GetActorLocation());
	}
	Refresh();
	UE_LOG(LogLooter, Log, TEXT("%s: done%s."), *GetActorNameOrLabel(), bHideUntilDone ? TEXT(", its model in place") : TEXT(""));
}
