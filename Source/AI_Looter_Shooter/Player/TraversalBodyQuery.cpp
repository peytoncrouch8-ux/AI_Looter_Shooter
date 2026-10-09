#include "Player/TraversalBodyQuery.h"
#include "World/PlayableArea.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

FTraversalBodyQuery::FTraversalBodyQuery(ACharacter& Character)
	: World(Character.GetWorld())
	, Params(SCENE_QUERY_STAT(PlayerTraversal), false, &Character)
{
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	Capsule->InitSweepCollisionParams(Params, Response);
	Channel = Capsule->GetCollisionObjectType();
	Radius = Capsule->GetScaledCapsuleRadius();
	HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	TArray<AActor*> Carried;
	Character.GetAttachedActors(Carried, true, true);
	Params.AddIgnoredActors(Carried);
}

bool FTraversalBodyQuery::Sweep(FHitResult& Hit, const FVector& From, const FVector& To, const FCollisionShape& Shape) const
{
	return World->SweepSingleByChannel(Hit, From, To, FQuat::Identity, Channel, Shape, Params, Response);
}

bool FTraversalBodyQuery::Blocked(const FVector& From, const FVector& To, const FCollisionShape& Shape) const
{
	if (FVector::DistSquared(From, To) < 1.0)
	{
		return World->OverlapBlockingTestByChannel(From, FQuat::Identity, Channel, Shape, Params, Response);
	}
	FHitResult Hit;
	return Sweep(Hit, From, To, Shape);
}

bool FTraversalBodyQuery::LineBlocked(const FVector& From, const FVector& To) const
{
	FHitResult Hit;
	return World->LineTraceSingleByChannel(Hit, From, To, Channel, Params, Response);
}

FCollisionShape FTraversalBodyQuery::Body(float Shrink) const
{
	return FCollisionShape::MakeCapsule(Radius - Shrink, HalfHeight);
}

bool FTraversalBodyQuery::Inside(const FVector& Point) const
{
	const APlayableArea* Area = APlayableArea::Find(World);
	return !Area || Area->Contains(Point);
}

float FTraversalBodyQuery::FloorGap()
{
	return 0.5f * (UCharacterMovementComponent::MIN_FLOOR_DIST + UCharacterMovementComponent::MAX_FLOOR_DIST);
}
