// ASpiderCreature's leg poses beyond the walking gait (SpiderCreature.cpp walks them): the two-bone solve every leg is
// posed by, a walking knee's spread as its foot comes in, the attack's raised front legs on a spring, and the death curl.
//
// Why (Looter.CastShots, 2026-10-09): the front legs leapt off the ground at the start of a bite (a plain chase toward
// the raised spot started at full speed, 30-70 cm a frame), the dead legs snapped to a new bend and stood straight up in
// a cage through the abdomen, and a back leg folding in swung its femur up into the abdomen it sits under.

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

	/**
	 * The dead pose, in the body's frame at size 1 (the frame carries the size): each femur lies out to its side (the hip's
	 * own way out, turned this much more square to the body) raised this far from level, and its tibia comes back down to
	 * a spot under the body (the hip's place pulled in by these shares, a little ahead) this high over the ground.
	 */
	constexpr float DeadKneeRise = 35.f;
	constexpr float DeadSideways = 0.45f;
	constexpr float DeadTuckShareX = 0.5f;
	constexpr float DeadTuckAhead = 10.f;
	constexpr float DeadTuckShareY = 0.6f;
	constexpr float DeadTuckHeight = 4.f;
	/** Mid-curl, how widely a foot's way is rounded onto the ground where it would dip under it (cm at size 1). */
	constexpr float FloorRounding = 20.f;

	float Ease(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	/** From turned Alpha of the way toward To (both unit length), the shortest way round. */
	FVector TurnToward(const FVector& From, const FVector& To, float Alpha)
	{
		return FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(From, To), Alpha).RotateVector(From);
	}
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

void ASpiderCreature::RecordDeathPose()
{
	for (FLeg& Leg : Legs)
	{
		// Where the solve last put it (a foot past its reach was drawn short of Leg.Foot), or its spot if it was never posed.
		Leg.DeathFoot = Leg.PosedFoot.IsNearlyZero() ? Leg.Foot : Leg.PosedFoot;
		Leg.HeldVelocity = FVector::ZeroVector;
	}
}

void ASpiderCreature::CurlDeadLeg(const FLeg& Leg, float Time, FVector& OutKnee, FVector& OutFoot, FVector& OutPole) const
{
	const float Scale = GetSizeScale();

	// The dead pose in the body's frame. The femur lies out to the side, knee raised; the tibia comes back down toward its
	// tuck spot under the body, as far as its length takes it, ending DeadTuckHeight over the ground. (A straight tibia
	// longer than the femur can't end beside the body; tucked under it is how a dead spider holds its legs.)
	const FVector& Hip = Leg.Hip;
	const FVector Out = FVector(Hip.X, Hip.Y, 0.f).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FVector Aside = (Out + FVector(0.f, FMath::Sign(Hip.Y) * DeadSideways, 0.f)).GetSafeNormal(UE_SMALL_NUMBER, Out);
	const float Rise = FMath::DegreesToRadians(DeadKneeRise);
	const FVector KneeDead = Hip + (Aside * FMath::Cos(Rise) + FVector::UpVector * FMath::Sin(Rise)) * Leg.FemurLength;
	const FVector Tuck(Hip.X * DeadTuckShareX + DeadTuckAhead, Hip.Y * DeadTuckShareY, DeadTuckHeight - DeadRide);
	const float Down = FMath::Clamp(static_cast<float>(Tuck.Z - KneeDead.Z) / FMath::Max(Leg.TibiaLength, 1.f), -1.f, 0.f);
	const FVector Across = FVector(Tuck.X - KneeDead.X, Tuck.Y - KneeDead.Y, 0.f).GetSafeNormal(UE_SMALL_NUMBER, -Aside);
	const FVector FootDead = KneeDead + (Across * FMath::Sqrt(1.f - Down * Down) + FVector::UpVector * Down) * Leg.TibiaLength;
	// How the dead knee bends out of its hip-to-foot line: the solve below puts it back exactly there.
	const FVector Line = (FootDead - Hip).GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);
	const FVector BendDead = (KneeDead - Hip - Line * FVector::DotProduct(KneeDead - Hip, Line)).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);

	// From where the foot was as it died to the dead pose. The foot starts planted (the body's drop doesn't push it into the
	// ground) and goes round the hip, not across it: its way from the hip turns from where it stood to where it lies while
	// its distance shrinks, never nearer than either end, and it keeps over the ground. (On a straight line it passed under
	// the hip, nearer than the leg can fold, and the knee swung over the body and back in a few frames: the snap, and the
	// back femurs through the abdomen.) The knee's bend turns from the walking one to the dead one; both lean out, and the
	// foot's way runs down between them, so the knee stays up and out the whole way.
	const FVector HipWorld = BodyFrame.TransformPosition(Hip);
	const FVector DeadFootWorld = BodyFrame.TransformPosition(FootDead);
	const FVector From = Leg.DeathFoot.IsNearlyZero() ? DeadFootWorld : Leg.DeathFoot;
	const FVector Up = BodyFrame.GetRotation().GetUpVector();
	const FVector Outward = (HipWorld - BodyFrame.GetLocation()).GetSafeNormal2D();
	// The bones keep the walking legs' roll (their pole) throughout, as the model was rigged: the dead bend only places the
	// knee. (Laid by the dead bend, the femurs rolled a quarter turn and their hulls' far sides went into the abdomen.)
	OutPole = KneePole(Up, Outward) + Outward * Leg.KneeSpread;
	const float Alpha = Ease(Time / CurlSeconds);
	const FVector ToFrom = From - HipWorld;
	const FVector ToDead = DeadFootWorld - HipWorld;
	const FVector Way = TurnToward(ToFrom.GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector), ToDead.GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector), Alpha);
	FVector CurlTarget = HipWorld + Way * FMath::Lerp(ToFrom.Size(), ToDead.Size(), static_cast<double>(Alpha));
	// The ground between: from under where it stood to under where it lies (DeadTuckHeight under the dead foot). A way that
	// dips under it is lifted onto it with the corner rounded off: a hard floor kinked the foot's motion where it met it, and
	// the knee jolted (6 cm a frame against the body). The rounding is none at either end, so it starts planted and ends
	// exactly in the dead pose.
	const double Floor = FMath::Lerp(From.Z, DeadFootWorld.Z - DeadTuckHeight * Scale, static_cast<double>(Alpha));
	const double Under = Floor - CurlTarget.Z;
	const double Round = FloorRounding * Scale * FMath::Sin(UE_PI * Alpha);
	CurlTarget.Z += 0.5 * (Under + FMath::Sqrt(Under * Under + Round * Round));
	const FVector Bend = FMath::Lerp(OutPole.GetSafeNormal(), BodyFrame.TransformVectorNoScale(BendDead), Alpha);
	SolveTwoBone(HipWorld, CurlTarget, Leg.FemurLength * Scale, Leg.TibiaLength * Scale, Bend, OutKnee, OutFoot);
}
