#include "Loot/LootTossComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

ULootTossComponent::ULootTossComponent()
{
	bAutoActivate = false;
	bShouldBounce = true;
	Bounciness = 0.3f;
	Friction = 0.6f;
	BounceVelocityStopSimulatingThreshold = 40.f;
}

void ULootTossComponent::OnRegister()
{
	Super::OnRegister();
	if (!Body)
	{
		Body = UpdatedComponent;
	}
}

void ULootTossComponent::Throw(const FVector& InVelocity)
{
	if (!Body)
	{
		Body = GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
	}
	SetUpdatedComponent(Body);
	Velocity = InVelocity;
	GroundImpacts = 0;
	Activate(true);
}

void ULootTossComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// On walkable ground: one hop at most, and none when the landing is already soft. Without this, friction alone
	// can't hold loot on a slope and it slides until the ground flattens out, which can be far from the kill.
	// Only static world geometry counts as ground; anything else may move or vanish and leave the loot floating.
	const UPrimitiveComponent* Surface = Hit.GetComponent();
	if (!Hit.bStartPenetrating && Hit.ImpactNormal.Z >= WalkableFloorZ && Surface && Surface->GetCollisionObjectType() == ECC_WorldStatic)
	{
		++GroundImpacts;
		if (GroundImpacts > 1 || ComputeBounceResult(Hit, TimeSlice, MoveDelta).Size() < SettleSpeed)
		{
			StopSimulating(Hit);
			return;
		}
	}
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
}
