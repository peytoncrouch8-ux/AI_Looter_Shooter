#include "Creatures/CreatureBase.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr float SteerInterval = 0.1f;
}

// ---------------------------------------------------------------------------
// Movement: local steering, no navmesh. Every distance is at size 1 and multiplied by the creature's size.
// ---------------------------------------------------------------------------

void ACreatureBase::MoveToward(const FVector& Goal, float Speed, float DeltaSeconds)
{
	FVector ToGoal = Goal - GetActorLocation();
	ToGoal.Z = 0.f;
	if (ToGoal.SizeSquared() < 1.f)
	{
		return;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed;

	if (EscapeTime > 0.f)
	{
		EscapeTime -= DeltaSeconds;
		AddMovementInput(EscapeDirection);
		return;
	}

	SteerTimer -= DeltaSeconds;
	if (SteerTimer <= 0.f)
	{
		SteerTimer = SteerInterval;
		SteerDirection = ChooseDirection(ToGoal.GetSafeNormal());
	}
	if (!SteerDirection.IsNearlyZero())
	{
		AddMovementInput(SteerDirection);
	}

	// Wedged against something the probes didn't see: take a short detour in a random clear direction.
	StuckTime = IsStuck(Speed) ? StuckTime + DeltaSeconds : 0.f;
	if (StuckTime > 0.8f)
	{
		StuckTime = 0.f;
		for (int32 Attempt = 0; Attempt < 8; ++Attempt)
		{
			const FVector Candidate = ToGoal.GetSafeNormal().RotateAngleAxis(FMath::FRandRange(60.f, 150.f) * (FMath::RandBool() ? 1.f : -1.f), FVector::UpVector);
			if (IsDirectionClear(Candidate))
			{
				EscapeDirection = Candidate;
				EscapeTime = 0.9f;
				break;
			}
		}
	}
}

bool ACreatureBase::IsStuck(float Speed) const
{
	return GetVelocity().Size2D() < Speed * 0.15f;
}

FVector ACreatureBase::ChooseDirection(const FVector& Desired)
{
	// Straight at the goal if possible, otherwise fan out, favoring the side that worked last time so it
	// commits to going around an obstacle instead of dithering.
	static const float Offsets[] = { 0.f, 30.f, 60.f, 90.f, 125.f };
	for (const float Offset : Offsets)
	{
		for (const float Side : { PreferredSide, -PreferredSide })
		{
			const FVector Candidate = Desired.RotateAngleAxis(Offset * Side, FVector::UpVector);
			if (IsDirectionClear(Candidate))
			{
				if (Offset > 0.f)
				{
					PreferredSide = Side;
				}
				return Candidate;
			}
			if (Offset == 0.f)
			{
				break;
			}
		}
	}
	return FVector::ZeroVector;
}

ACreatureBase::FSteerProbes ACreatureBase::GetSteerProbes() const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	FSteerProbes Probes;
	// The capsule is scaled with the actor already; the distances ahead and the drop it takes grow with it.
	Probes.SweepRadius = Capsule->GetScaledCapsuleRadius() * 0.8f;
	Probes.SweepLength = 220.f * SizeScale;
	Probes.LedgeDistance = 160.f * SizeScale;
	Probes.LedgeDrop = Capsule->GetScaledCapsuleHalfHeight() + 350.f * SizeScale;
	return Probes;
}

bool ACreatureBase::IsDirectionClear(const FVector& Direction) const
{
	const FVector Start = GetActorLocation();
	const FSteerProbes Probes = GetSteerProbes();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CreatureSteer), false, this);
	if (const APawn* Victim = Target.Get())
	{
		Params.AddIgnoredActor(Victim);
	}

	// Obstacles: anything that blocks a walking pawn and is too steep to walk up.
	FHitResult Hit;
	const FCollisionShape Probe = FCollisionShape::MakeSphere(Probes.SweepRadius);
	if (GetWorld()->SweepSingleByChannel(Hit, Start, Start + Direction * Probes.SweepLength, FQuat::Identity, ECC_Pawn, Probe, Params)
		&& Hit.ImpactNormal.Z < 0.65f)
	{
		return false;
	}

	// Ledges: never walk off the edge of an island.
	FVector Ground;
	return FindGround(Start + Direction * Probes.LedgeDistance, 0.f, Probes.LedgeDrop, Ground);
}

void ACreatureBase::FaceToward(const FVector& Point, float DeltaSeconds)
{
	const FVector To = Point - GetActorLocation();
	if (To.SizeSquared2D() < 1.f)
	{
		return;
	}
	const FRotator Wanted(0.f, To.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Wanted, DeltaSeconds, 540.f));
}

bool ACreatureBase::PickWanderGoal()
{
	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		const FVector2D Offset = FMath::RandPointInCircle(WanderRadius);
		FVector Ground;
		if (Offset.Size() > 150.f * SizeScale && FindGround(Home.GetLocation() + FVector(Offset, 0.f), 300.f, 600.f, Ground))
		{
			WanderGoal = Ground;
			return true;
		}
	}
	return false;
}

bool ACreatureBase::FindGround(const FVector& Point, float Above, float Below, FVector& OutGround) const
{
	// World-static only: terrain and solid props, never grass, pawns, loot or volumes.
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(GetWorld(), TEXT("CreatureGround"), this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Point + FVector(0.f, 0.f, Above), Point - FVector(0.f, 0.f, Below),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		OutGround = Hit.ImpactPoint;
		return true;
	}
	return false;
}

void ACreatureBase::SnapToGround()
{
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector Ground;
	if (FindGround(GetActorLocation(), HalfHeight + 150.f, 1500.f, Ground))
	{
		SetActorLocation(Ground + FVector(0.f, 0.f, HalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
}
