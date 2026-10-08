#include "Loot/LootTossComponent.h"
#include "Audio/LooterSound.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	/** A touch slower than this (cm/s, into the surface) makes no sound; from it to LoudLandSpeed it grows to full. */
	constexpr float QuietLandSpeed = 60.f;
	constexpr float LoudLandSpeed = 600.f;
}

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
	// Every knock is heard by how hard it came in: the first landing the loudest, a hop's after it softer.
	const float IntoSurface = static_cast<float>(-FVector::DotProduct(Velocity, Hit.ImpactNormal));
	if (!Hit.bStartPenetrating && IntoSurface > QuietLandSpeed)
	{
		const float Loudness = FMath::GetMappedRangeValueClamped(FVector2f(QuietLandSpeed, LoudLandSpeed), FVector2f(0.3f, 1.f), IntoSurface);
		LooterSound::PlayAt(this, LooterSoundCue::LootLand, Hit.ImpactPoint, Loudness);
	}
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
