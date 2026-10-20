// ASpiderCreature's leg poses beyond the walking gait (SpiderCreature.cpp walks them): the two-bone solve every leg is
// posed by, a walking knee's spread as its foot comes in, and the attack's raised front legs on a spring. (The death's
// throe and curl are SpiderCreatureDeath.cpp's.)
//
// Why (Looter.CastShots, 2026-10-09): the front legs leapt off the ground at the start of a bite (a plain chase toward
// the raised spot started at full speed, 30-70 cm a frame), and a back leg folding in swung its femur up into the abdomen
// it sits under.

#include "Creatures/SpiderCreature.h"

namespace
{
	/** Each foot's springs step at most this long (s): short enough for the stiffest to stay steady. */
	constexpr float HeldSpringStep = 1.f / 120.f;

	/**
	 * A walking knee spreads out by this much of the body's outward way (on top of KneePole's 0.4) per share of the leg's
	 * resting stretch its foot has come in, at most MaxKneeSpread: a back foot stepped in 35 cm keeps its femur at its
	 * standing slope instead of rising 11 degrees into the abdomen.
	 */
	constexpr float KneeSpreadPerFold = 3.5f;
	constexpr float MaxKneeSpread = 1.f;
}

void ASpiderCreature::SolveTwoBone(const FVector& Hip, const FVector& Target, float Upper, float Lower, const FVector& Pole, FVector& OutKnee,
	FVector& OutFoot)
{
	const FVector ToTarget = Target - Hip;
	float Distance = ToTarget.Size();
	const FVector Direction = Distance > KINDA_SMALL_NUMBER ? ToTarget / Distance : FVector::DownVector;
	Distance = FMath::Clamp(Distance, FMath::Abs(Upper - Lower) + 1.f, (Upper + Lower) * 0.999f);
	OutFoot = Hip + Direction * Distance;

	const float Along = (Upper * Upper - Lower * Lower + Distance * Distance) / (2.f * Distance);
	const float Height = FMath::Sqrt(FMath::Max(Upper * Upper - Along * Along, 0.f));
	FVector Bend = Pole - Direction * FVector::DotProduct(Pole, Direction);
	if (!Bend.Normalize())
	{
		Bend = FVector::UpVector;
	}
	OutKnee = Hip + Direction * Along + Bend * Height;
}

float ASpiderCreature::WantedKneeSpread(const FLeg& Leg, float Stretch)
{
	const float In = FMath::Clamp((Leg.RestStretch - Stretch) / FMath::Max(Leg.RestStretch, 1.f), 0.f, 1.f);
	return FMath::Min(In * KneeSpreadPerFold, MaxKneeSpread);
}

FVector ASpiderCreature::OverrideFootTarget(const FLeg& Leg, const FTransform& Body) const
{
	// In the air in the body's frame (the clod lifted over her head), or on the ground ahead of the actor and dug into it
	// (her front legs tearing at the floor), or between the two as she lifts.
	const FSpiderPoseOverride& Pose = PoseOverride;
	const float Scale = GetSizeScale();
	const FVector Air = Body.TransformPosition(FVector(Pose.FrontAir.X, Pose.FrontAir.Y * Leg.Side, Pose.FrontAir.Z));
	if (Pose.GroundShare <= 0.f)
	{
		return Air;
	}
	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const FVector Spot = GetActorLocation() + Yaw.RotateVector(FVector(Pose.FrontGround.X, Pose.FrontGround.Y * Leg.Side, 0.f) * Scale);
	const FVector Ground = GroundUnder(Spot) - FVector(0.f, 0.f, Pose.DigDepth * Scale);
	return FMath::Lerp(Air, Ground, FMath::Clamp(Pose.GroundShare, 0.f, 1.f));
}

FVector ASpiderCreature::GetPosedFoot(int32 Pair, float Side) const
{
	for (const FLeg& Leg : Legs)
	{
		if (Leg.Pair == Pair && (Leg.Side > 0.f) == (Side > 0.f))
		{
			return Leg.PosedFoot;
		}
	}
	return GetActorLocation();
}

void ASpiderCreature::StepHeldFoot(FLeg& Leg, const FVector& Target, float Omega, float DeltaSeconds)
{
	// Critically damped: no overshoot, and from rest it speeds up over a few frames rather than in one.
	const float Seconds = FMath::Min(DeltaSeconds, 0.5f);
	if (Seconds <= 0.f)
	{
		return;
	}
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / HeldSpringStep - 0.01f));
	const float Step = Seconds / Steps;
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		Leg.HeldVelocity += (Omega * Omega * (Target - Leg.Foot) - 2.f * Omega * Leg.HeldVelocity) * Step;
		Leg.Foot += Leg.HeldVelocity * Step;
	}
}
