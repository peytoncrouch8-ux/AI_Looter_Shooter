// AUnpaidCreature's phase-step: stuck, or fallen far behind while it chases, it fades out and comes back 3 to 5 m nearer
// its target. UnpaidRules has the rules (when, how far, which spot first); this is where they meet the level: ground it
// can stand on, room for its body, its target in sight from there, no safe zone and nothing past the playable area. Its
// material does the fading (Phase, in the mesh's custom primitive data): the solid body goes first and the shroud's
// ends last, and coming back the ends come first.

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/EncounterSubsystem.h"
#include "World/PlayableArea.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Ground steeper than about 50 degrees (the creatures' walkable floor angle) is no place to come back on. */
	constexpr double WalkableGroundNormalZ = 0.64;

	/** What stands on the ground (rocks, wagons, buildings) carries this tag for the minimap: it never lands on top of one. */
	const FName StandingObstacleTag(TEXT("Obstacle"));

	/** The room a spot needs is checked this far up from the ground (cm, at size 1), so a walkable slope doesn't touch it. */
	constexpr float RoomLift = 25.f;

	/** Where it can see its target from: about its eyes (a share of its height over the ground). */
	constexpr float EyeShare = 1.6f;

	/** Nowhere to come back on: it looks again after this long (s), not every frame. */
	constexpr float RetrySeconds = 0.75f;
}

bool AUnpaidCreature::IsPhasing() const
{
	return PhaseState != EPhaseState::None;
}

void AUnpaidCreature::TickPhaseStep(float DeltaSeconds)
{
	if (PhaseState == EPhaseState::None)
	{
		SinceLastStep = FMath::Min(SinceLastStep + DeltaSeconds, 1000.f);
		RetryIn = FMath::Max(RetryIn - DeltaSeconds, 0.f);
		WatchProgress(DeltaSeconds);
		if (WantsToStep())
		{
			StartPhaseStep();
		}
		return;
	}

	PhaseTime += DeltaSeconds;
	if (PhaseState == EPhaseState::Out)
	{
		PhaseAmount = FMath::Clamp(PhaseTime / PhaseFadeOutSeconds, 0.f, 1.f);
		if (PhaseAmount >= 1.f)
		{
			Arrive();
		}
		return;
	}
	PhaseAmount = 1.f - FMath::Clamp(PhaseTime / PhaseFadeInSeconds, 0.f, 1.f);
	if (PhaseAmount <= 0.f)
	{
		PhaseState = EPhaseState::None;
		SinceLastStep = 0.f;
		ResetProgress();
	}
}

void AUnpaidCreature::WatchProgress(float DeltaSeconds)
{
	const APawn* Victim = GetTarget();
	if (GetCreatureState() != ECreatureState::Chase || !Victim)
	{
		ResetProgress();
		return;
	}
	// Getting nearer than it has been is progress; a player who keeps their distance (outrunning it) is as good as a wall.
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	if (Distance < BestDistance - PhaseStep.ProgressNeeded)
	{
		BestDistance = Distance;
		StallTime = 0.f;
	}
	else
	{
		StallTime += DeltaSeconds;
	}
}

void AUnpaidCreature::ResetProgress()
{
	StallTime = 0.f;
	BestDistance = TNumericLimits<float>::Max();
}

bool AUnpaidCreature::WantsToStep() const
{
	const APawn* Victim = GetTarget();
	if (!Victim || bLunging || RetryIn > 0.f || GetCreatureState() != ECreatureState::Chase)
	{
		return false;
	}
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	return UnpaidRules::WantsPhaseStep(PhaseStep, Distance, StallTime, SinceLastStep);
}

void AUnpaidCreature::StartPhaseStep()
{
	FVector Feet;
	if (!FindPhaseSpot(Feet))
	{
		RetryIn = RetrySeconds;
		return;
	}
	PhaseDestination = Feet;
	PhaseState = EPhaseState::Out;
	PhaseTime = 0.f;
	GetCharacterMovement()->StopMovementImmediately();
	UE_LOG(LogLooter, Verbose, TEXT("%s phase-steps %.1f m nearer its target (stuck %.1f s)."), *GetName(),
		(FVector::Dist2D(GetActorLocation(), GetTarget()->GetActorLocation()) - FVector::Dist2D(Feet, GetTarget()->GetActorLocation())) / 100.0,
		StallTime);
}

bool AUnpaidCreature::FindPhaseSpot(FVector& OutFeet) const
{
	UWorld* World = GetWorld();
	const APawn* Victim = GetTarget();
	if (!World || !Victim)
	{
		return false;
	}
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float Scale = GetSizeScale();
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, HalfHeight);
	const FVector TargetEye = Victim->GetActorLocation() + FVector(0.f, 0.f, 50.f);

	// World-static ground only (terrain and solid props, never volumes); the room and sight checks see the trees and rocks
	// too, and never itself or its target.
	const FCollisionQueryParams GroundParams = LooterWorld::StaticGeometryParams(World, TEXT("UnpaidPhaseGround"), this);
	FCollisionQueryParams OpenParams(SCENE_QUERY_STAT(UnpaidPhaseRoom), false, this);
	OpenParams.AddIgnoredActor(Victim);
	const FCollisionShape Room = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight);
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	const APlayableArea* Playable = APlayableArea::Find(World);
	const float Reach = PhaseStep.MaxRise + 50.f;

	auto CanStand = [&](const FVector& Candidate, FVector& OutGround)
	{
		// Ground under it near its own level, walkable, and not the top of a rock, a wagon or a roof.
		FHitResult Hit;
		if (!World->LineTraceSingleByObjectType(Hit, Candidate + FVector(0.f, 0.f, Reach), Candidate - FVector(0.f, 0.f, Reach),
			FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams))
		{
			return false;
		}
		const AActor* Under = Hit.GetActor();
		if (Hit.ImpactNormal.Z < WalkableGroundNormalZ || (Under && Under->ActorHasTag(StandingObstacleTag)))
		{
			return false;
		}
		const FVector Ground = Hit.ImpactPoint;
		// Room for its body: never inside a wall, a rock, a tree or another creature.
		if (World->OverlapBlockingTestByChannel(Ground + FVector(0.f, 0.f, HalfHeight + RoomLift * Scale), FQuat::Identity, ECC_Pawn, Room,
			OpenParams))
		{
			return false;
		}
		// In the open, with its target in sight: never shut in a room or behind a wall from them.
		if (World->LineTraceTestByObjectType(Ground + FVector(0.f, 0.f, HalfHeight * EyeShare), TargetEye,
			FCollisionObjectQueryParams(ECC_WorldStatic), OpenParams))
		{
			return false;
		}
		// Not into a safe zone (Delia's salt line), nor past the playable area's edge.
		if ((Encounters && Encounters->IsInSafeZone(Ground)) || (Playable && !Playable->Contains(Ground)))
		{
			return false;
		}
		OutGround = Ground;
		return true;
	};
	return UnpaidRules::ChoosePhaseSpot(PhaseStep, Feet, Victim->GetActorLocation(), HuntingGround, GetHome().GetLocation(), CanStand,
		OutFeet);
}

void AUnpaidCreature::Arrive()
{
	// Gone from sight: it moves while unseen, and comes back facing its target.
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	double Yaw = GetActorRotation().Yaw;
	if (const APawn* Victim = GetTarget())
	{
		Yaw = (Victim->GetActorLocation() - PhaseDestination).Rotation().Yaw;
	}
	SetActorLocationAndRotation(PhaseDestination + FVector(0.f, 0.f, HalfHeight + 2.f), FRotator(0.0, Yaw, 0.0), false, nullptr,
		ETeleportType::TeleportPhysics);
	GetCharacterMovement()->StopMovementImmediately();
	PhaseState = EPhaseState::In;
	PhaseTime = 0.f;
	ResetProgress();
	// Its shroud comes back hanging still, not swinging in from where it was; and the turn isn't a spin.
	for (FShroudChain& Chain : Chains)
	{
		Chain.Settle();
	}
	LastYaw = static_cast<float>(Yaw);
}
