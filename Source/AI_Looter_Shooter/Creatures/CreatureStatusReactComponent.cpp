#include "Creatures/CreatureStatusReactComponent.h"
#include "Creatures/CreatureBase.h"
#include "Elements/ElementStatus.h"
#include "Elements/ElementStatusComponent.h"
#include "Elements/ElementTuning.h"
#include "GameFramework/Actor.h"

UCreatureStatusReactComponent::UCreatureStatusReactComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	// Before the creature poses its body this frame (its actor tick), so the pose it lays is this frame's.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

UCreatureStatusReactComponent* UCreatureStatusReactComponent::Find(const AActor* Creature)
{
	return Creature ? Creature->FindComponentByClass<UCreatureStatusReactComponent>() : nullptr;
}

UCreatureStatusReactComponent* UCreatureStatusReactComponent::FindOrAdd(AActor& Creature)
{
	if (UCreatureStatusReactComponent* Existing = Find(&Creature))
	{
		return Existing;
	}
	UCreatureStatusReactComponent* Added = NewObject<UCreatureStatusReactComponent>(&Creature, TEXT("StatusReact"));
	Added->RegisterComponent();
	return Added;
}

void UCreatureStatusReactComponent::BeginPlay()
{
	Super::BeginPlay();
	// Its own offset into the flails and tremors, and its own dice, from its owner: a pack burning doesn't flail in step,
	// and the same level rolls the same way each load.
	const uint32 Seed = GetOwner() ? GetTypeHash(GetOwner()->GetFName()) : 0u;
	Rolls.Initialize(static_cast<int32>(Seed ^ 0x5eedu));
	ResetReactions();
}

void UCreatureStatusReactComponent::ResetReactions()
{
	const uint32 Seed = GetOwner() ? GetTypeHash(GetOwner()->GetFName()) : 0u;
	Tracker.Reset(static_cast<float>(Seed % 1000u) * 0.01f);
	Panic = FBurnPanic();
}

void UCreatureStatusReactComponent::Wake()
{
	if (!IsComponentTickEnabled())
	{
		SetComponentTickEnabled(true);
	}
}

void UCreatureStatusReactComponent::Advance(float DeltaSeconds)
{
	const AActor* Owner = GetOwner();
	const ACreatureBase* Creature = Cast<ACreatureBase>(Owner);
	const UElementStatusComponent* Statuses = UElementStatusComponent::Find(Owner);
	const bool bAlive = Creature && !Creature->IsDead();
	static const FElementStatus None = FElementStatus();
	Tracker.Advance(Statuses ? Statuses->GetStatus() : None, FElementTuning::Live(), bAlive, DeltaSeconds);
}

void UCreatureStatusReactComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
	// Nothing on it and nothing left to ease: it sleeps until a status takes it again (UpdateStatusBrain wakes it).
	const UElementStatusComponent* Statuses = UElementStatusComponent::Find(GetOwner());
	const bool bStatuses = Statuses && Statuses->GetStatus().IsActive();
	if (!bStatuses && Tracker.IsAtRest() && !IsPanicking())
	{
		SetComponentTickEnabled(false);
	}
}
