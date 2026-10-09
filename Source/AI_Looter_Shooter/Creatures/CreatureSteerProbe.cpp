#include "Creatures/CreatureSteerProbe.h"
#include "Creatures/CreatureBase.h"
#include "World/PlayableArea.h"
#include "World/WorldQueries.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	FVector FlatUnit(const FVector& Vector)
	{
		return FVector(Vector.X, Vector.Y, 0.0).GetSafeNormal();
	}

	/**
	 * The face a look met, flat and pointing back at the creature: the sweep's own normal (smooth round posts and corners),
	 * else the face's, else straight back.
	 */
	FVector WallNormal(const FHitResult& Hit, const FVector& Way)
	{
		const FVector Candidates[] = { Hit.Normal, Hit.ImpactNormal };
		for (const FVector& Candidate : Candidates)
		{
			const FVector Flat = FlatUnit(Candidate);
			if (!Flat.IsNearlyZero() && FVector::DotProduct(Flat, Way) < -0.05)
			{
				return Flat;
			}
		}
		return -Way;
	}

	/** How far past a touch (cm, at size 1) it looks for the surface its feet would meet there. */
	constexpr float PastTouch = 4.f;
	/** A rise from the ground just before a touch to the surface past it of more than this share of its step is a ledge's face. */
	constexpr float StepShare = 0.9f;
	/** A sweep already touching something going along or away from it looks again this much thinner. */
	constexpr float ThinShare = 0.5f;

	/** Free spots: how far out it tries (body radii), which ways (degrees from the way it wants), how high it lifts the body to look (a share of its half height), and how far under it looks for ground past its step (cm, at size 1). */
	constexpr float SpotDistances[] = { 0.75f, 1.25f, 2.f };
	constexpr float SpotTurns[] = { 0.f, 45.f, -45.f, 90.f, -90.f, 135.f, -135.f, 180.f };
	constexpr float SpotLift = 0.15f;
	constexpr float SpotGroundSearch = 150.f;
	/** The low line it checks a slide along, as a share of its step over its feet: a rail under its middle. */
	constexpr float KneeShare = 0.6f;
	/** Set down this far over the ground (cm, at size 1), as the walking movement keeps a body. */
	constexpr float SpotGap = 2.f;
}

FCreatureSteerProbe::FCreatureSteerProbe(const ACreatureBase& Creature)
	: World(Creature.GetWorld())
	, Movement(Creature.GetCharacterMovement())
	, BodyParams(SCENE_QUERY_STAT(CreatureSteer), false, &Creature)
	, RoomParams(SCENE_QUERY_STAT(CreatureSteerRoom), false, &Creature)
	, GroundParams(LooterWorld::StaticGeometryParams(Creature.GetWorld(), TEXT("CreatureSteerGround"), &Creature))
{
	const UCapsuleComponent* Capsule = Creature.GetCapsuleComponent();
	Middle = Creature.GetActorLocation();
	Radius = Capsule->GetScaledCapsuleRadius();
	HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	SizeScale = Creature.GetSizeScale();
	if (Movement)
	{
		StepHeight = Movement->MaxStepHeight;
		WalkableFloorZ = Movement->GetWalkableFloorZ();
	}
	const ACreatureBase::FSteerProbes Probes = Creature.GetSteerProbes();
	SweepRadius = Probes.SweepRadius;
	SweepHalfHeight = Probes.SweepHalfHeight;
	SweepLift = Probes.SweepLift;
	LedgeDistance = Probes.LedgeDistance;
	LedgeDrop = Probes.LedgeDrop;

	// What stops the body is what stops its capsule; but not other creatures (its pack's spacing keeps them apart) nor the
	// player it's after (its brain stops short of them). Room at a free spot counts both.
	Capsule->InitSweepCollisionParams(BodyParams, BodyResponse);
	Capsule->InitSweepCollisionParams(RoomParams, RoomResponse);
	Channel = Capsule->GetCollisionObjectType();
	BodyResponse.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
	TArray<AActor*> Carried;
	Creature.GetAttachedActors(Carried, true, true);
	BodyParams.AddIgnoredActors(Carried);
	RoomParams.AddIgnoredActors(Carried);
	if (const APawn* Victim = Creature.GetTarget())
	{
		BodyParams.AddIgnoredActor(Victim);
	}

	// A body that reaches well past its capsule (a spider's abdomen a meter behind it) needs room for that end too.
	FVector Front;
	FVector Back;
	float FootRadius = 0.f;
	Creature.GetFootprintInWorld(Front, Back, FootRadius);
	const FVector BackReach(Back.X - Middle.X, Back.Y - Middle.Y, 0.0);
	if (BackReach.Size() + FootRadius > Radius + 5.f * SizeScale)
	{
		FootprintReach = BackReach;
		FootprintRadius = FootRadius;
	}
}

FCreatureSteerPlanner::FLookResult FCreatureSteerProbe::Look(const FVector& Direction, float Distance) const
{
	using FLookResult = FCreatureSteerPlanner::FLookResult;
	const FVector Way = FlatUnit(Direction);
	if (!World || Way.IsNearlyZero() || Distance <= 0.f)
	{
		return FLookResult::Clear(Distance);
	}

	// The body, swept along the way.
	const FVector Start = Middle + FVector(0.0, 0.0, SweepLift);
	const FVector End = Start + Way * Distance;
	FHitResult Hit;
	bool bHit = SweepBody(Start, End, SweepRadius, Hit);
	if (bHit && Hit.bStartPenetrating)
	{
		// Touching something already: going into it is blocked; going along or away from it, a thinner sweep sees past the touch.
		const FVector Out = FlatUnit(Hit.Normal);
		if (FVector::DotProduct(Out, Way) < -0.1)
		{
			return FLookResult::Blocked(0.f, Out);
		}
		bHit = SweepBody(Start, End, SweepRadius * ThinShare, Hit);
		if (bHit && Hit.bStartPenetrating)
		{
			const FVector ThinOut = FlatUnit(Hit.Normal);
			if (FVector::DotProduct(ThinOut, Way) < -0.1)
			{
				return FLookResult::Blocked(0.f, ThinOut);
			}
			bHit = false;
		}
	}
	if (bHit)
	{
		FVector Surface = FVector::UpVector;
		double SurfaceZ = Middle.Z;
		if (!IsWalkableContact(Hit, Way, &Surface, &SurfaceZ))
		{
			return FLookResult::Blocked(static_cast<float>(Hit.Distance), WallNormal(Hit, Way));
		}
		// A slope or a step it walks up: it looks on past it, along the surface, its foot lifted onto it (once; a second
		// walkable touch is let be).
		const float Left = Distance - static_cast<float>(Hit.Distance);
		const FVector Along = (Way - Surface * FVector::DotProduct(Way, Surface)).GetSafeNormal();
		if (Left > 1.f && !Along.IsNearlyZero())
		{
			const double Foot = Hit.Location.Z - FMath::Max(SweepHalfHeight, SweepRadius);
			const FVector From = FVector(Hit.Location) + Surface * SizeScale + FVector(0.0, 0.0, FMath::Max(0.0, SurfaceZ + SizeScale - Foot));
			FHitResult Next;
			// Judged from where its feet would be there, up the slope.
			if (SweepBody(From, From + Along * Left, SweepRadius, Next) && !Next.bStartPenetrating
				&& !IsWalkableContactFrom(Next, Way, Next.Location.Z - SweepLift - HalfHeight, nullptr, nullptr))
			{
				return FLookResult::Blocked(static_cast<float>(Hit.Distance + Next.Distance), WallNormal(Next, Way));
			}
		}
	}

	// Ground ahead: it never walks off a drop (an island's edge, a deck's), looking no farther than it was asked to.
	const float GroundAt = FMath::Min(LedgeDistance, Distance);
	FVector Ground;
	if (!FindGround(Middle + Way * GroundAt, LedgeDrop, Ground))
	{
		return FLookResult::Blocked(GroundAt * 0.5f, FVector::ZeroVector, true);
	}
	return FLookResult::Clear(Distance);
}

bool FCreatureSteerProbe::IsWalkableContact(const FHitResult& Hit, const FVector& Direction, FVector* OutSurfaceNormal, double* OutSurfaceZ) const
{
	return IsWalkableContactFrom(Hit, Direction, Middle.Z - HalfHeight, OutSurfaceNormal, OutSurfaceZ);
}

bool FCreatureSteerProbe::IsWalkableContactFrom(const FHitResult& Hit, const FVector& Direction, double FeetZ, FVector* OutSurfaceNormal,
	double* OutSurfaceZ) const
{
	const FVector Way = FlatUnit(Direction);
	// Touched higher over its feet than a step: a wall's face. Not by the sweep's normal: grazing a wall's top edge, a mesh
	// reports the top's slope, and a box's edge its upright face even at a kerb it steps up.
	if (!World || !Hit.bBlockingHit || Way.IsNearlyZero() || Hit.ImpactPoint.Z - FeetZ > StepHeight * StepShare)
	{
		return false;
	}
	// Lower: judge by the surface its feet would meet just past the touch...
	const FVector Up = FVector::UpVector;
	const FVector Past = FVector(Hit.ImpactPoint) + Way * (PastTouch * SizeScale);
	FHitResult Top;
	if (!Trace(Past + Up * StepHeight, Past - Up * StepHeight, Top) || Top.bStartPenetrating || Top.ImpactNormal.Z < WalkableFloorZ)
	{
		return false;
	}
	// ...and by how far that is over the ground just before it: a slope rises bit by bit, a wall's top all at once.
	const FVector Before = FVector(Hit.ImpactPoint) - Way * FMath::Max(0.5f * Radius, 10.f * SizeScale);
	double BeforeZ = FeetZ;
	FHitResult Below;
	if (Trace(FVector(Before.X, Before.Y, Top.ImpactPoint.Z + 1.0), FVector(Before.X, Before.Y, FeetZ - StepHeight), Below))
	{
		if (Below.bStartPenetrating)
		{
			return false;
		}
		BeforeZ = Below.ImpactPoint.Z;
	}
	if (Top.ImpactPoint.Z - BeforeZ > StepHeight * StepShare)
	{
		return false;
	}
	if (OutSurfaceNormal)
	{
		*OutSurfaceNormal = Top.ImpactNormal;
	}
	if (OutSurfaceZ)
	{
		*OutSurfaceZ = Top.ImpactPoint.Z;
	}
	return true;
}

bool FCreatureSteerProbe::FindFreeSpot(const FVector& Preferred, FVector& OutMiddle) const
{
	if (!World || !Movement)
	{
		return false;
	}
	FVector Forward = FlatUnit(Preferred);
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::ForwardVector;
	}
	const FVector Up = FVector::UpVector;
	// Its own capsule, a little narrower so grazing what it touches now doesn't count.
	const FCollisionShape Body = FCollisionShape::MakeCapsule(FMath::Max(Radius - SizeScale, 1.f), HalfHeight);
	const FVector Knee = Up * (StepHeight * KneeShare - HalfHeight);
	const float GroundSearch = SpotLift * HalfHeight + StepHeight + SpotGroundSearch * SizeScale;
	const APlayableArea* Area = APlayableArea::Find(World);
	for (const float Out : SpotDistances)
	{
		for (const float TurnDegrees : SpotTurns)
		{
			const FVector Spot = Middle + Forward.RotateAngleAxis(TurnDegrees, Up) * (Out * Radius) + Up * (SpotLift * HalfHeight);
			// Reached without passing through anything: never out through a fence, at its middle or its knees...
			FHitResult Line;
			if (Trace(Middle, Spot, Line) || Trace(Middle + Knee, Spot + Knee, Line))
			{
				continue;
			}
			// ...with room for its body there, and for what reaches past it...
			if (World->OverlapBlockingTestByChannel(Spot, FQuat::Identity, Channel, Body, RoomParams, RoomResponse))
			{
				continue;
			}
			if (FootprintRadius > 0.f && World->OverlapBlockingTestByChannel(Spot + FootprintReach, FQuat::Identity, Channel,
				FCollisionShape::MakeSphere(FootprintRadius * 0.8f), BodyParams, BodyResponse))
			{
				continue;
			}
			// ...and ground under it that it walks on (never a creature's back), in the playable area.
			FHitResult Floor;
			if (!World->SweepSingleByChannel(Floor, Spot, Spot - Up * GroundSearch, FQuat::Identity, Channel, Body, RoomParams, RoomResponse)
				|| Floor.bStartPenetrating || !Movement->IsWalkable(Floor) || (Floor.GetActor() && Floor.GetActor()->IsA<APawn>())
				|| (Area && !Area->Contains(Floor.Location)))
			{
				continue;
			}
			OutMiddle = FVector(Floor.Location) + Up * (SpotGap * SizeScale);
			return true;
		}
	}
	return false;
}

bool FCreatureSteerProbe::FindGround(const FVector& Point, float Below, FVector& OutGround) const
{
	FHitResult Hit;
	if (World && World->LineTraceSingleByObjectType(Hit, Point, Point - FVector(0.0, 0.0, Below), FCollisionObjectQueryParams(ECC_WorldStatic),
		GroundParams))
	{
		OutGround = Hit.ImpactPoint;
		return true;
	}
	return false;
}

bool FCreatureSteerProbe::SweepBody(const FVector& From, const FVector& To, float InRadius, FHitResult& OutHit) const
{
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(InRadius, FMath::Max(SweepHalfHeight, InRadius));
	return World->SweepSingleByChannel(OutHit, From, To, FQuat::Identity, Channel, Shape, BodyParams, BodyResponse);
}

bool FCreatureSteerProbe::Trace(const FVector& From, const FVector& To, FHitResult& OutHit) const
{
	return World->LineTraceSingleByChannel(OutHit, From, To, Channel, BodyParams, BodyResponse);
}
