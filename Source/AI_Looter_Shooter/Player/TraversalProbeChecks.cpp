#include "Player/TraversalProbe.h"
#include "Player/TraversalBodyQuery.h"
#include "Loot/AmmoPickup.h"
#include "Weapons/WeaponBase.h"
#include "World/PlayableArea.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Volume.h"

// FTraversalProbe's checks: what can be climbed or vaulted, the free spot a wedged player is nudged to, and the reasons'
// names.

bool FTraversalProbe::IsClimbable(const FHitResult& Hit, APawn* Climber)
{
	if (const AActor* Actor = Hit.GetActor())
	{
		if (Actor->IsA<APawn>() || Actor->IsA<AVolume>() || Actor->IsA<APlayableArea>() || Actor->IsA<AWeaponBase>() || Actor->IsA<AAmmoPickup>())
		{
			return false;
		}
		if (Actor->ActorHasTag(LooterTraversal::NoClimbTag()) || (Climber && Actor->IsAttachedTo(Climber)))
		{
			return false;
		}
	}
	if (const UPrimitiveComponent* Component = Hit.GetComponent())
	{
		if (Component->ComponentHasTag(LooterTraversal::NoClimbTag()) || Component->IsSimulatingPhysics())
		{
			return false;
		}
		// Whatever the movement wouldn't step up onto isn't climbed onto either; nor is anything on the move (a train).
		if (Climber && !Component->CanCharacterStepUp(Climber))
		{
			return false;
		}
		if (Component->Mobility == EComponentMobility::Movable && Component->GetComponentVelocity().SizeSquared() > 1.0)
		{
			return false;
		}
	}
	return true;
}

bool FTraversalProbe::FindFreeSpot(ACharacter& Character, const FVector& Preferred, FVector& OutCenter)
{
	const UCharacterMovementComponent* Move = Character.GetCharacterMovement();
	if (!Move || !Character.GetCapsuleComponent() || !Character.GetWorld())
	{
		return false;
	}
	const FTraversalBodyQuery Query(Character);
	const FVector Center = Character.GetActorLocation();
	FVector Forward = FVector(Preferred.X, Preferred.Y, 0.0).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = Character.GetActorForwardVector().GetSafeNormal2D();
	}
	static constexpr float Distances[] = { 30.f, 55.f, 85.f };
	static constexpr float Lifts[] = { 10.f, 40.f };
	static constexpr float Turns[] = { 0.f, 45.f, -45.f, 90.f, -90.f, 135.f, -135.f, 180.f };
	for (const float Distance : Distances)
	{
		for (const float Lift : Lifts)
		{
			for (const float Turn : Turns)
			{
				const FVector Spot = Center + Forward.RotateAngleAxis(Turn, FVector::UpVector) * Distance + FVector(0.0, 0.0, Lift);
				// Room for the whole body, reached without passing through anything (never out through a fence)...
				if (Query.Blocked(Spot, Spot, Query.Body(0.5f)) || Query.LineBlocked(Center, Spot))
				{
					continue;
				}
				// ...with ground under it the body can stand on, inside the playable area.
				FHitResult Floor;
				if (!Query.Sweep(Floor, Spot, Spot - FVector(0.0, 0.0, 250.0), Query.Body(0.5f)) || Floor.bStartPenetrating
					|| !Move->IsWalkable(Floor) || !IsClimbable(Floor, &Character) || !Query.Inside(Spot))
				{
					continue;
				}
				OutCenter = Spot;
				return true;
			}
		}
	}
	return false;
}

const TCHAR* FTraversalProbe::RefusalName(ETraversalRefusal Refusal)
{
	switch (Refusal)
	{
	case ETraversalRefusal::None: return TEXT("none");
	case ETraversalRefusal::NoWall: return TEXT("nothing in front");
	case ETraversalRefusal::NotFacing: return TEXT("not facing it");
	case ETraversalRefusal::Walkable: return TEXT("a walkable slope");
	case ETraversalRefusal::TooLow: return TEXT("too low");
	case ETraversalRefusal::TooHigh: return TEXT("too high");
	case ETraversalRefusal::NotClimbable: return TEXT("not climbable");
	case ETraversalRefusal::NoTop: return TEXT("no top to stand on");
	case ETraversalRefusal::NoRoom: return TEXT("no room");
	case ETraversalRefusal::Blocked: return TEXT("blocked across the edge");
	case ETraversalRefusal::NoLanding: return TEXT("no floor beyond");
	case ETraversalRefusal::OutsideArea: return TEXT("outside the playable area");
	default: return TEXT("-");
	}
}
