#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/CreatureSteerProbe.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	constexpr float SteerInterval = 0.1f;
}

// ---------------------------------------------------------------------------
// Movement: local steering, no navmesh. Every distance is at size 1 and multiplied by the creature's size.
//
// Each update the planner (FCreatureSteerPlanner) picks its way from looks along a few directions (FCreatureSteerProbe):
// straight at its goal while that's clear, else along the wall it met, round its end. Bumping into something the looks
// missed (MoveBlockedBy) or pressing on without getting anywhere (CreatureBaseUnstick.cpp) is remembered as a felt wall.
// Cost: one look (a capsule sweep and a ground trace) per update going straight, two or three following a wall, at most
// ten meeting one head-on up close; four looks at most far away. Updates come every 0.1 s, or at its slower update rate.
// ---------------------------------------------------------------------------

void ACreatureBase::MoveToward(const FVector& Goal, float Speed, float DeltaSeconds)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = Speed;
	// A fresh start (SetState zeroes StuckTime): the wall it was following and any glide free are over.
	if (StuckTime <= 0.f)
	{
		if (!SteerPlanner.HasPlanned())
		{
			// Creatures of a pack split round what's in their way, rather than all taking one side.
			SteerPlanner.SeedSide(GetUniqueID() % 2 == 0 ? 1.f : -1.f);
		}
		SteerPlanner.Forget(false);
		Unstick.Reset(GetActorLocation());
		SteerDirection = FVector::ZeroVector;
	}
	StuckTime += DeltaSeconds;
	SteerPlanner.Advance(DeltaSeconds);
	// Being slid free of something it was stuck on: nothing else until it's there.
	if (TickNudge(DeltaSeconds))
	{
		return;
	}

	FVector ToGoal = Goal - GetActorLocation();
	ToGoal.Z = 0.f;
	if (ToGoal.SizeSquared() < 1.f)
	{
		return;
	}

	SteerTimer -= DeltaSeconds;
	if (SteerTimer <= 0.f)
	{
		SteerTimer = SteerInterval;
		// Bent away from any creature crowding it, so a pack fans out instead of running into each other's backs.
		SteerDirection = ChooseDirection(SpacedDirection(ToGoal.GetSafeNormal()), Goal, Speed);
	}
	if (!SteerDirection.IsNearlyZero())
	{
		AddMovementInput(SteerDirection);
	}
	TickUnstick(DeltaSeconds, Goal, Speed);
}

void ACreatureBase::MoveBlockedBy(const FHitResult& Impact)
{
	Super::MoveBlockedBy(Impact);
	// Only while it makes its own way, and only something solid: bumping a creature or the player is its pack's spacing's
	// business, and the floor or a ceiling isn't in its way.
	const bool bMakingWay = State == ECreatureState::Wander || State == ECreatureState::Chase || State == ECreatureState::Return;
	const AActor* Other = Impact.GetActor();
	if (!bMakingWay || Unstick.IsGliding() || (Other && Other->IsA<APawn>()) || FMath::Abs(Impact.Normal.Z) > 0.7)
	{
		return;
	}
	Unstick.NoteBlockedMove();
	// Pressing into it, not brushing along it.
	FVector Normal(Impact.Normal.X, Impact.Normal.Y, 0.0);
	if (SteerDirection.IsNearlyZero() || !Normal.Normalize() || FVector::DotProduct(SteerDirection, Normal) > -0.5
		|| !SteerPlanner.TakeContactCheck())
	{
		return;
	}
	// What its looks see, they go round already; only what they miss (a rail under them, a prop's odd collision) is kept.
	const float BodyRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	if (!LookAlong(-Normal, BodyRadius * 1.5f).bClear)
	{
		return;
	}
	if (SteerPlanner.NoteContact(Impact.ImpactPoint, Normal, SteerDirection, BodyRadius, SizeScale))
	{
		UE_LOG(LogLooter, VeryVerbose, TEXT("%s bumped into %s, which its looks missed: going round it."), *GetName(), *GetNameSafe(Other));
		SteerTimer = 0.f;
	}
}

FCreatureSteerPlanner::FLookResult ACreatureBase::LookAlong(const FVector& Direction, float Distance) const
{
	return FCreatureSteerProbe(*this).Look(Direction, Distance);
}

FCreatureSteerPlanner::FRequest ACreatureBase::MakeSteerRequest(const FVector& Goal, const FVector& Desired, float Speed) const
{
	const FSteerProbes Probes = GetSteerProbes();
	FCreatureSteerPlanner::FRequest Request;
	Request.Here = GetActorLocation();
	Request.Goal = Goal;
	Request.Desired = Desired;
	Request.GoalDistance = static_cast<float>(FVector::Dist2D(Request.Here, Goal));
	Request.LookLength = Probes.SweepLength;
	Request.LookRadius = Probes.SweepRadius;
	Request.Speed = Speed;
	Request.SizeScale = SizeScale;
	// Up close it takes its whole look round; far away (updating slowly, or out of view) fewer looks do.
	Request.bNear = UpdateInterval <= 0.f && !bPoseFrozen;
	return Request;
}

bool ACreatureBase::IsStuck(float Speed) const
{
	return GetVelocity().Size2D() < Speed * 0.15f;
}

FVector ACreatureBase::ChooseDirection(const FVector& Desired, const FVector& Goal, float Speed)
{
	// One set of queries for the update (the ground's ignore list gathered once), shared by every look the planner takes.
	const FCreatureSteerProbe Probe(*this);
	return SteerPlanner.Plan(MakeSteerRequest(Goal, Desired, Speed), [&Probe](const FVector& Direction, float Distance)
	{
		return Probe.Look(Direction, Distance);
	});
}

ACreatureBase::FSteerProbes ACreatureBase::GetSteerProbes() const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	FSteerProbes Probes;
	// The capsule and the step are scaled with it already; the distances ahead and the drop it takes grow with it.
	Probes.StepHeight = GetCharacterMovement()->MaxStepHeight;
	// The look sweeps the body nearly as wide as it is, from a little off the ground (the ground and kerbs under its step
	// aren't in its way, and whether a slope or a step is, the look judges by the surface) to just under its top: a rail
	// under its middle is seen (the old sphere at the middle missed an Unpaid's knee-high rails), and a den's low roof
	// over it isn't.
	const float FootLift = FMath::Min(Probes.StepHeight * 0.4f, HalfHeight * 0.3f);
	const float TopGap = HalfHeight * 0.03f;
	Probes.SweepHalfHeight = (2.f * HalfHeight - FootLift - TopGap) * 0.5f;
	Probes.SweepRadius = FMath::Min(Radius * 0.9f, Probes.SweepHalfHeight);
	Probes.SweepLift = FootLift + Probes.SweepHalfHeight - HalfHeight;
	Probes.SweepLength = 220.f * SizeScale;
	Probes.LedgeDistance = 160.f * SizeScale;
	Probes.LedgeDrop = HalfHeight + 350.f * SizeScale;
	return Probes;
}

bool ACreatureBase::IsDirectionClear(const FVector& Direction) const
{
	return LookAlong(Direction, GetSteerProbes().SweepLength).bClear;
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
	// A patrol's creature strolls nowhere of its own: it goes after its anchor (Wander follows it as it moves).
	if (Pack->HasRoamAnchor())
	{
		WanderGoal = Pack->GetRoamAnchor();
		return true;
	}
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
	const FVector Middle = GetActorLocation();
	// It looks down from over its head, so one placed a little into the ground still finds the surface; but never from
	// above a roof over it. In the Gravemother's den the ceiling is lower than that: a look from inside a rock's hull
	// finds the rock right there (a trace starting inside a convex reports a hit at its start), one from over the roof its
	// top, and either put her in the ceiling.
	float Above = HalfHeight + 150.f;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(GetWorld(), TEXT("CreatureRoof"), this);
	FHitResult Roof;
	if (GetWorld()->LineTraceSingleByObjectType(Roof, Middle, Middle + FVector(0.f, 0.f, Above), FCollisionObjectQueryParams(ECC_WorldStatic),
		Params) && !Roof.bStartPenetrating)
	{
		Above = FMath::Max(Roof.Distance - 1.f, 0.f);
	}
	FVector Ground;
	if (FindGround(Middle, Above, 1500.f, Ground))
	{
		SetActorLocation(Ground + FVector(0.f, 0.f, HalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
}
