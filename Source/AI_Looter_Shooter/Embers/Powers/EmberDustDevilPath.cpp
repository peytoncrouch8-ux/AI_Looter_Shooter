#include "Embers/Powers/EmberDustDevil.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Embers/EmberDamageType.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberVfx.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerViewComponent.h"
#include "Player/SlideDust.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "World/PlayableArea.h"
#include "World/WorldQueries.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// UEmberDustDevil (Ellis's Dust Devil): the dash laid out before it runs (PlanDash: ground-following, swept, never off a drop or out of bounds).

// ---------------------------------------------------------------------------
// The path
// ---------------------------------------------------------------------------

FVector FDustDevilPath::At(float Distance) const
{
	if (Points.IsEmpty())
	{
		return FVector::ZeroVector;
	}
	float Left = FMath::Max(Distance, 0.f);
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const float Piece = static_cast<float>(FVector::Dist(Points[Index - 1], Points[Index]));
		if (Left <= Piece && Piece > 0.f)
		{
			return FMath::Lerp(Points[Index - 1], Points[Index], Left / Piece);
		}
		Left -= Piece;
	}
	return Points.Last();
}

FVector UEmberDustDevil::DashDirection(const ACharacter& Character)
{
	// The keys held, else the way the body is already going, else the way the player looks.
	FVector Way = Character.GetLastMovementInputVector();
	Way.Z = 0.0;
	if (Way.SizeSquared() < 0.01)
	{
		const UCharacterMovementComponent* Movement = Character.GetCharacterMovement();
		FVector Going = Movement ? Movement->Velocity : FVector::ZeroVector;
		Going.Z = 0.0;
		if (Going.SizeSquared() > FMath::Square(150.0))
		{
			Way = Going;
		}
	}
	if (Way.SizeSquared() < 0.01)
	{
		const AController* Controller = Character.GetController();
		const FRotator Look = Controller ? Controller->GetControlRotation() : Character.GetActorRotation();
		Way = FRotator(0.f, Look.Yaw, 0.f).Vector();
	}
	return Way.GetSafeNormal2D();
}

FDustDevilPath UEmberDustDevil::PlanDash(const ACharacter& Character, const FVector& Direction, float Range)
{
	FDustDevilPath Plan;
	const FVector Start = Character.GetActorLocation();
	Plan.Points.Add(Start);
	Plan.Direction = FVector(Direction.X, Direction.Y, 0.0).GetSafeNormal();
	if (Plan.Direction.IsNearlyZero())
	{
		Plan.Direction = FRotator(0.f, Character.GetActorRotation().Yaw, 0.f).Vector();
	}
	const UWorld* World = Character.GetWorld();
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	const UCharacterMovementComponent* Movement = Character.GetCharacterMovement();
	if (!World || !Capsule || Range <= 0.f)
	{
		return Plan;
	}
	const float BodyRadius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float StepUp = Movement ? Movement->MaxStepHeight : 45.f;
	const float WalkableZ = Movement ? Movement->GetWalkableFloorZ() : 0.7f;
	const APlayableArea* Area = APlayableArea::Find(World);

	// The ground: world-static (terrain and props), never a volume or the playable area's walls.
	const FCollisionQueryParams GroundParams = LooterWorld::StaticGeometryParams(World, TEXT("DustDevilGround"), &Character);
	// The way: whatever stops the player walking stops the dash (walls, rocks, trees, the playable area's walls), but not
	// creatures (the dash goes through them, flinging them aside) or loose bodies.
	FCollisionQueryParams SweepParams(SCENE_QUERY_STAT(DustDevilSweep), false, &Character);
	TArray<AActor*> Carried;
	Character.GetAttachedActors(Carried, true, true);
	SweepParams.AddIgnoredActors(Carried);
	FCollisionResponseParams Response(Capsule->GetCollisionResponseToChannels());
	Response.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
	Response.CollisionResponse.SetResponse(ECC_PhysicsBody, ECR_Ignore);
	Response.CollisionResponse.SetResponse(ECC_Destructible, ECR_Ignore);
	// Lifted by half a step, so the ground's small bumps don't stop it; a step's rise is the walk's to climb.
	const FVector Lift(0.0, 0.0, StepUp * 0.5f);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(BodyRadius * 0.9f, FMath::Max(HalfHeight - StepUp * 0.5f, BodyRadius));

	// On the ground (or just over it): it follows the ground. High in the air: it keeps its height.
	const float StartFeet = static_cast<float>(Start.Z) - HalfHeight;
	FHitResult Under;
	Plan.bOnGround = (Movement && Movement->IsMovingOnGround())
		|| World->LineTraceSingleByObjectType(Under, FVector(Start.X, Start.Y, StartFeet + StepUp), FVector(Start.X, Start.Y, StartFeet - 60.f),
			FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams);

	FVector Last = Start;
	float Covered = 0.f;
	const int32 Steps = FMath::CeilToInt(Range / StepLength);
	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		const float Along = FMath::Min(Step * StepLength, Range);
		FVector Next = Start + Plan.Direction * Along;
		Next.Z = Last.Z;
		if (Area && !Area->Contains(Next))
		{
			break;
		}
		if (Plan.bOnGround)
		{
			// Up or down what a walk could: a step's rise, or a walkable slope over this step's length. Past it (a ledge,
			// a drop, a cliff) the dash stops at the edge.
			const float Rise = StepUp + (Along - Covered);
			const float LastFeet = static_cast<float>(Last.Z) - HalfHeight;
			FHitResult Ground;
			if (!World->LineTraceSingleByObjectType(Ground, FVector(Next.X, Next.Y, LastFeet + Rise), FVector(Next.X, Next.Y, LastFeet - Rise),
					FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams)
				|| Ground.bStartPenetrating || Ground.ImpactNormal.Z < WalkableZ)
			{
				break;
			}
			Next.Z = Ground.ImpactPoint.Z + HalfHeight + 2.0;
		}
		FHitResult Block;
		if (World->SweepSingleByChannel(Block, Last + Lift, Next + Lift, FQuat::Identity, Capsule->GetCollisionObjectType(), Shape, SweepParams,
				Response)
			&& !Block.bStartPenetrating)
		{
			// Stopped short of a wall: as far as the sweep got, if that's anywhere.
			const FVector Reached = FMath::Lerp(Last, Next, FMath::Max(Block.Time - 0.05f, 0.f));
			if (FVector::Dist(Reached, Last) > 5.0)
			{
				Plan.Points.Add(Reached);
			}
			break;
		}
		Plan.Points.Add(Next);
		Covered = Along;
		Last = Next;
	}
	for (int32 Index = 1; Index < Plan.Points.Num(); ++Index)
	{
		Plan.Length += static_cast<float>(FVector::Dist(Plan.Points[Index - 1], Plan.Points[Index]));
	}
	return Plan;
}
