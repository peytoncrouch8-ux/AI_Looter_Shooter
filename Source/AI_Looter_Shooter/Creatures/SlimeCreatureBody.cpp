// ASlimeCreature's body: the squash and core springs, and its fit to the ground. SlimeCreature.cpp works out the shape
// each frame (AnimateBody) and hops.
//
// The fit: the slime stands upright on its capsule, whose foot touches a slope uphill of its middle, so its flat, wide foot
// (78 cm across at rest, more squashed) cut into the hill on one side and hung over it on the other, and a squash pressed
// the gel through the ground (the user's "models clip"). On the ground it now measures the ground under its foot (a plane
// through four points round it, only after it has moved: it stands still between hops), lays the body along it, turned
// about the foot's middle, and sets it down onto the ground under its middle. In the air it eases back upright.

#include "Creatures/SlimeCreature.h"
#include "Creatures/CreatureUpdateRate.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	/**
	 * The squash spring: about 3.2 wobbles a second, lightly damped, so a landing at 0.6 overshoots to about 1.1, dips
	 * to 0.95 and settles in about a third of a second.
	 */
	constexpr float SquashStiffness = 404.f;
	constexpr float SquashDamping = 14.f;
	/** The core's spring: a little quicker and looser, so it jiggles inside the gel. */
	constexpr float CoreStiffness = 632.f;
	constexpr float CoreDamping = 12.6f;
	/** The springs' longest step (s): small enough for both to stay steady. */
	constexpr float SpringStep = 1.f / 120.f;

	/** The ground is measured again once it has moved this far (cm), at this share of its foot's radius round its middle. */
	constexpr float RefitDistance = 4.f;
	constexpr float ProbeShare = 0.7f;
	/** The farthest it sets itself down (or up) onto the ground under its middle (cm at size 1): no reaching into a hole. */
	constexpr float MaxDrop = 30.f;
	constexpr float MaxRise = 10.f;
	/** How quickly it settles onto the ground's tilt and seat, and rights itself in the air (1/s). */
	constexpr float FitRate = 14.f;
	constexpr float UprightRate = 8.f;
}

void ASlimeCreature::FSquashSpring::Ramp(float NewTo, float Seconds)
{
	From = Value;
	To = NewTo;
	RampTime = 0.f;
	RampLength = FMath::Max(Seconds, KINDA_SMALL_NUMBER);
	Velocity = 0.f;
}

void ASlimeCreature::FSquashSpring::Tick(float DeltaSeconds)
{
	if (IsRamping())
	{
		RampTime += DeltaSeconds;
		Value = FMath::Lerp(From, To, FMath::InterpEaseOut(0.f, 1.f, FMath::Min(RampTime / RampLength, 1.f), 2.f));
		// It then springs from rest at the ramp's end, which gives the overshoot after a landing.
		return;
	}
	Velocity += (SquashStiffness * (Target - Value) - SquashDamping * Velocity) * DeltaSeconds;
	Value += Velocity * DeltaSeconds;
}

void ASlimeCreature::StepSprings(FSquashSpring& SquashSpring, FVector& CoreLag, FVector& CoreLagSpeed, float DeltaSeconds)
{
	const float Seconds = FMath::Min(DeltaSeconds, FCreatureUpdateRate::MaxInterval);
	if (Seconds <= 0.f)
	{
		return;
	}
	// Even steps no longer than SpringStep: short enough for both springs to stay steady, and a long update takes the
	// same steps as the short ones it stands in for (a slow-ticking slime wobbles like a near one).
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / SpringStep - 0.01f));
	const float Step = Seconds / Steps;
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		SquashSpring.Tick(Step);
		CoreLagSpeed += (-CoreStiffness * CoreLag - CoreDamping * CoreLagSpeed) * Step;
		CoreLag += CoreLagSpeed * Step;
	}
}

ACreatureBase::FFootprint ASlimeCreature::GetFootprint() const
{
	FFootprint Footprint;
	Footprint.Radius = FootRadius - 4.f;
	return Footprint;
}

FQuat ASlimeCreature::TiltForGround(const FVector& GroundNormal)
{
	const FVector Normal = GroundNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	if (Normal.Z <= 0.0)
	{
		return FQuat::Identity;
	}
	const FQuat Full = FQuat::FindBetweenNormals(FVector::UpVector, Normal);
	const float Degrees = FMath::RadiansToDegrees(Full.GetAngle());
	return Degrees <= MaxGroundTilt ? Full : FQuat::Slerp(FQuat::Identity, Full, MaxGroundTilt / Degrees).GetNormalized();
}

void ASlimeCreature::FitToGround(float DeltaSeconds)
{
	const bool bOnGround = IsDead() || Hop != EHop::Air;
	const float Scale = GetSizeScale();
	if (!bOnGround)
	{
		// Airborne: upright over the capsule's foot, as it hops.
		GroundTiltTarget = FQuat::Identity;
		GroundDropTarget = 0.f;
		GroundMeasuredAt = FVector(1.0e9);
	}
	else if (FVector::DistSquared(GetActorLocation(), GroundMeasuredAt) > FMath::Square(RefitDistance * Scale))
	{
		// The ground under its foot: four points round its middle (a plane through them), and the point under its middle.
		GroundMeasuredAt = GetActorLocation();
		const USkeletalMeshComponent* Body = GetMesh();
		const FTransform& MeshXf = Body->GetComponentTransform();
		const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const float Reach = FootRadius * ProbeShare * Scale;
		const FVector Middle = GetActorLocation();
		FVector Points[4];
		bool bFound = true;
		for (int32 Index = 0; Index < 4 && bFound; ++Index)
		{
			const FVector Offset = MeshXf.TransformVectorNoScale(FVector(Index == 0 ? Reach : (Index == 1 ? -Reach : 0.f),
				Index == 2 ? Reach : (Index == 3 ? -Reach : 0.f), 0.f));
			bFound = FindGround(Middle + Offset, HalfHeight + Reach, HalfHeight + 2.f * Reach, Points[Index]);
		}
		FVector Under;
		if (bFound && FindGround(Middle, HalfHeight, HalfHeight + MaxDrop * Scale, Under))
		{
			FVector Normal = FVector::CrossProduct(Points[0] - Points[1], Points[2] - Points[3]);
			Normal *= Normal.Z < 0.0 ? -1.0 : 1.0;
			GroundTiltTarget = TiltForGround(MeshXf.InverseTransformVectorNoScale(Normal));
			// The mesh's origin is the capsule's foot; the ground under its middle is a little below it on a slope (the capsule
			// touches uphill), or under the floor's float. In the mesh's space (the actor's size scales it).
			const float FootZ = static_cast<float>(Middle.Z) - HalfHeight;
			GroundDropTarget = FMath::Clamp((FootZ - static_cast<float>(Under.Z)) / Scale, -MaxRise, MaxDrop);
		}
		else
		{
			GroundTiltTarget = FQuat::Identity;
			GroundDropTarget = 0.f;
		}
	}
	if (!bGroundFitStarted || DeltaSeconds <= 0.f)
	{
		GroundTilt = GroundTiltTarget;
		GroundDrop = GroundDropTarget;
		bGroundFitStarted = true;
		return;
	}
	const float Rate = bOnGround ? FitRate : UprightRate;
	GroundTilt = FQuat::Slerp(GroundTilt, GroundTiltTarget, FMath::Min(DeltaSeconds * Rate, 1.f)).GetNormalized();
	GroundDrop = FMath::FInterpTo(GroundDrop, GroundDropTarget, DeltaSeconds, Rate);
}
