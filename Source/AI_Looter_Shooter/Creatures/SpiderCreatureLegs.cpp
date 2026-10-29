// ASpiderCreature's leg poses beyond the walking gait (SpiderCreature.cpp walks them): the two-bone solve every leg is
// posed by, a walking knee's spread as its foot comes in, and the attack's raised front legs on a spring. (The death's
// throe and curl are SpiderCreatureDeath.cpp's.)
//
// Why (Looter.CastShots, 2026-10-09): the front legs leapt off the ground at the start of a bite (a plain chase toward
// the raised spot started at full speed, 30-70 cm a frame), and a back leg folding in swung its femur up into the abdomen
// it sits under.
//
// The hit feel pass (HITS, 2026-10-28; looks only, Main's call for the user: no slow): a shot leg buckles (its knee drops
// on its arc, its foot lifts, in 0.04 s and back over 0.14), a stagger splays the feet out, a Big hit skids the far feet
// along the shot, and a badly hurt spider limps on its most-shot leg (a lower, slower step). The buckle swings the knee
// down round the hip-to-foot line rather than moving it: both bones keep their length, and the knee stays on the arc the
// solve allows, so nothing stretches or snaps.

#include "Creatures/SpiderCreature.h"
#include "Combat/HitPose.h"

namespace
{
	/** A shot leg's buckle at a flinch strength of 1 (cm at size 1): its knee drops this far, its foot lifts this far. */
	constexpr float BuckleKneeDrop = 12.f;
	constexpr float BuckleFootLift = 8.f;
	/** The buckle's envelope: in over this long, back over this long (s). */
	constexpr float BuckleIn = 0.04f;
	constexpr float BuckleOut = 0.14f;
	/** The knee swings down round the hip-to-foot line at most this far (degrees): past it a knee would fold under the leg. */
	constexpr float BuckleMostTurn = 75.f;
	/** A stagger splays the feet out this share of their resting spot's reach from the body. */
	constexpr float StaggerSplay = 0.15f;
	/** A limping leg steps this share as high, and its step takes this many times as long. */
	constexpr float LimpLift = 0.5f;
	constexpr float LimpTime = 1.3f;
	/** Wounded, it leans this far onto the leg it limps on; worst hurt, its breathing heaves this many times as deep. */
	constexpr float LimpRollBias = 3.f;
	constexpr float WoundBreathDeep = 1.6f;
	/** A skid slides its foot this share of what's left of it a second (most of it in about 0.15 s). */
	constexpr float SkidRate = 18.f;

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

// ---------------------------------------------------------------------------
// The hit feel: a shot leg's buckle, the stagger's splay, a Big hit's skid, the limp
// ---------------------------------------------------------------------------

float ASpiderCreature::BuckleNow(const FLeg& Leg) const
{
	return Leg.BuckleAge >= 0.f ? Leg.BuckleStrength * HitEnvelope(Leg.BuckleAge, BuckleIn, BuckleOut) : 0.f;
}

void ASpiderCreature::PoseGaitLeg(const FLeg& Leg, const FVector& Hip, const FVector& FootTarget, const FVector& Outward, bool bHeld,
	FVector& Pole, FVector& OutKnee, FVector& OutFoot) const
{
	const float Scale = GetSizeScale();
	const float Upper = Leg.FemurLength * Scale;
	const float Lower = Leg.TibiaLength * Scale;
	// Staggered, its feet are pushed out from under it as the body sags (the stagger pose: SpiderCreatureHurt.cpp).
	FVector Aim = FootTarget;
	if (!bHeld && StaggerShown > 0.f)
	{
		Aim += Outward * (Leg.Rest.Size2D() * Scale * StaggerSplay * StaggerShown);
	}
	SolveTwoBone(Hip, Aim, Upper, Lower, Pole, OutKnee, OutFoot);
	const float Buckle = bHeld ? 0.f : BuckleNow(Leg);
	if (Buckle <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	// Buckled: the foot lifts off its spot, and the knee drops BuckleKneeDrop below where it stood unbuckled. The lifted
	// foot's solve gives the knee's circle round the hip-to-foot line (both bones' lengths kept anywhere on it); the knee
	// swings down it, outward, to that height.
	const float WantedZ = static_cast<float>(OutKnee.Z) - BuckleKneeDrop * Buckle * Scale;
	const FVector Lifted = Aim + FVector::UpVector * (BuckleFootLift * Buckle * Scale);
	FVector Knee;
	FVector Foot;
	SolveTwoBone(Hip, Lifted, Upper, Lower, Pole, Knee, Foot);
	const FVector Line = Foot - Hip;
	const float Distance = static_cast<float>(Line.Size());
	if (Distance < 1.f)
	{
		return;
	}
	const FVector Way = Line / Distance;
	const float Along = (Upper * Upper - Lower * Lower + Distance * Distance) / (2.f * Distance);
	const float Height = FMath::Sqrt(FMath::Max(Upper * Upper - Along * Along, 0.f));
	const FVector Center = Hip + Way * Along;
	FVector Bend = Knee - Center;
	if (Height < 1.f || !Bend.Normalize())
	{
		OutKnee = Knee;
		OutFoot = Foot;
		return;
	}
	// The swing's way: outward, square to the line and to the bend.
	FVector Swing = Outward - Way * FVector::DotProduct(Outward, Way) - Bend * FVector::DotProduct(Outward, Bend);
	if (!Swing.Normalize())
	{
		Swing = FVector::CrossProduct(Way, Bend).GetSafeNormal();
	}
	// The knee's height off the circle's middle at a swing of Turn is Height x (A cos Turn + B sin Turn) = Height x R cos(Turn
	// - Phase): the smallest swing that brings it down to WantedZ, none if it's there already.
	const float A = static_cast<float>(Bend.Z);
	const float B = static_cast<float>(Swing.Z);
	const float R = FMath::Sqrt(A * A + B * B);
	const float Want = (WantedZ - static_cast<float>(Center.Z)) / Height;
	float Turn = 0.f;
	if (R > KINDA_SMALL_NUMBER && Want < A)
	{
		Turn = FMath::Atan2(B, A) + FMath::Acos(FMath::Clamp(Want / R, -1.f, 1.f));
	}
	Turn = FMath::Clamp(Turn, 0.f, FMath::DegreesToRadians(BuckleMostTurn));
	const FVector NewBend = Bend * FMath::Cos(Turn) + Swing * FMath::Sin(Turn);
	// The pole turns with the knee round the line, so the bones roll with it (as the knee's spread rolls them).
	Pole = FQuat::FindBetweenNormals(Bend, NewBend.GetSafeNormal()).RotateVector(Pole);
	SolveTwoBone(Hip, Lifted, Upper, Lower, Pole, OutKnee, OutFoot);
}

void ASpiderCreature::SlideSkids(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	const float Share = FMath::Min(SkidRate * DeltaSeconds, 1.f);
	for (FLeg& Leg : Legs)
	{
		if (Leg.Skid.IsNearlyZero(0.1))
		{
			Leg.Skid = FVector::ZeroVector;
			continue;
		}
		// A foot lifted to step (or held) lets go of its skid: its step lands it where it's going.
		if (Leg.bStepping)
		{
			Leg.Skid = FVector::ZeroVector;
			continue;
		}
		const FVector Move = Leg.Skid * Share;
		Leg.Foot += Move;
		Leg.Skid -= Move;
	}
}

float ASpiderCreature::LimpLiftShare(const FLeg& Leg)
{
	return Leg.bLimp ? LimpLift : 1.f;
}

float ASpiderCreature::LimpTimeScale(const FLeg& Leg)
{
	return Leg.bLimp ? LimpTime : 1.f;
}

int32 ASpiderCreature::GetLimpLeg() const
{
	if (WoundStageNow < 1 || Legs.IsEmpty())
	{
		return INDEX_NONE;
	}
	// The leg most shot; none shot, the front left (index 0: SetupRig lays them out front to back, left then right).
	int32 Worst = 0;
	float Most = 0.f;
	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		const float* Femur = HurtByBone.Find(Legs[Index].Femur);
		const float* Tibia = HurtByBone.Find(Legs[Index].Tibia);
		const float Taken = (Femur ? *Femur : 0.f) + (Tibia ? *Tibia : 0.f);
		if (Taken > Most)
		{
			Most = Taken;
			Worst = Index;
		}
	}
	return Worst;
}

float ASpiderCreature::LimpRoll() const
{
	const int32 Limp = GetLimpLeg();
	return Limp != INDEX_NONE && !IsDead() ? LimpRollBias * Legs[Limp].Side : 0.f;
}

float ASpiderCreature::WoundBreath() const
{
	return WoundStageNow >= 2 && !IsDead() ? WoundBreathDeep : 1.f;
}

