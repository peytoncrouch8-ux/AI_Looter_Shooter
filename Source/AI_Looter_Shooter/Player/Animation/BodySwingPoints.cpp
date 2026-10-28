#include "Player/Animation/BodySwing.h"
#include "HAL/IConsoleManager.h"

// FBodySwingPoints: one swing chain's points and the step that moves them (FBodySwing, BodySwing.cpp, drives a chain's
// worth from the pose).

namespace
{
	TAutoConsoleVariable<float> CVarBodySwingTurnCarry(TEXT("Looter.Body.Swing.TurnCarry"), 1.f,
		TEXT("Scales how much of the body's turn the swinging cloth turns with (each kind's TurnCarry; 0: none, the springs alone)."));

	/** World gravity (cm/s^2); each kind takes a share (FBodySwingSettings::Gravity). */
	constexpr float GravityZ = -980.f;
	/** The anchor jumping this far in one frame (cm) is a teleport (a respawn, a lift's end): the chain starts over. */
	constexpr float TeleportDistance = 150.f;
	/** Every step's length (s), whatever the frame rate (FBodySwingPoints::Step). */
	constexpr float FixedStep = 1.f / 120.f;
	/** A frame longer than this (a hitch, a load) counts as this long. */
	constexpr float MaxFrame = 0.1f;

	/**
	 * A bone's nearest place to a limb this close to its head (a share of its length) is left to the point above: turning
	 * the bone there would swing its tip by many times the overlap (at 0.2, a crouch's thigh across the top of the coat's
	 * side swung a flap out level with the hip).
	 */
	constexpr double MinBoneAlong = 0.35;

	/** The most a limb turns a bone's tip in one step, a share of its length (about 30 degrees). */
	constexpr double MaxBonePushShare = 0.5;
}

void FBodySwingPoints::Reset(TConstArrayView<FVector> Targets)
{
	Points.Reset();
	Points.Append(Targets.GetData(), Targets.Num());
	Previous = Points;
	Shown = Points;
	Rises.Init(0.f, Points.Num());
	Accumulator = 0.f;
	bStarted = true;
}

FVector FBodySwingPoints::PushOut(const FVector& P, const FBodySwingCapsule& Capsule)
{
	const FVector Closest = FMath::ClosestPointOnSegment(P, Capsule.A, Capsule.B);
	const FVector Away = P - Closest;
	const double Distance = Away.Size();
	if (Distance >= Capsule.Radius)
	{
		return P;
	}
	const FVector Out = Distance > UE_KINDA_SMALL_NUMBER ? Away / Distance : FVector::ForwardVector;
	return Closest + Out * Capsule.Radius;
}

FVector FBodySwingPoints::PushBoneOut(const FVector& Head, const FVector& Tip, const FBodySwingCapsule& Capsule,
	const FVector& Outward)
{
	if (Capsule.BoneRadius <= 0.f)
	{
		return Tip;
	}
	FVector OnBone;
	FVector OnLimb;
	FMath::SegmentDistToSegmentSafe(Head, Tip, Capsule.A, Capsule.B, OnBone, OnLimb);
	const FVector Away = OnBone - OnLimb;
	const double Distance = Away.Size();
	const double Length = FVector::Dist(Head, Tip);
	if (Distance >= Capsule.BoneRadius || Length <= UE_KINDA_SMALL_NUMBER)
	{
		return Tip;
	}
	const double Along = FVector::Dist(Head, OnBone) / Length;
	if (Along < MinBoneAlong)
	{
		return Tip;
	}
	// Out the body's side of the limb, square to the limb and the bone (turning the bone about its head): the shortest
	// way apart pointed sideways when the two lay in one front-to-back plane, and a stride's back shin swung the coat's
	// back panel out round its side.
	const FVector Axis = (Capsule.B - Capsule.A).GetSafeNormal();
	const FVector Side = FVector::VectorPlaneProject(FVector::VectorPlaneProject(Outward, Axis), (Tip - Head) / Length).GetSafeNormal();
	FVector Out;
	double Overlap = Capsule.BoneRadius - Distance;
	if (!Side.IsZero())
	{
		Out = Side;
		if (Distance > UE_KINDA_SMALL_NUMBER && FVector::DotProduct(Away / Distance, Side) < -0.5)
		{
			// The limb has come through to the cloth's outer side (clearly: near a limb's end the way apart can point
			// anywhere): back across it.
			Overlap = Capsule.BoneRadius + Distance;
		}
	}
	else
	{
		Out = Distance > UE_KINDA_SMALL_NUMBER ? Away / Distance :
			FVector::VectorPlaneProject(Tip - OnLimb, Axis).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	}
	// The tip moves by the overlap over how far down the bone it is, so the bone's nearest place clears by the overlap; a
	// little at a time (the steps after finish it), or a limb coming through flung the cloth round.
	return Tip + Out * FMath::Min(Overlap / Along, Length * MaxBonePushShare);
}

FVector FBodySwingPoints::PushPlaceOut(const FVector& S, const FBodySwingCapsule& Capsule, const FVector& Outward)
{
	const FVector Closest = FMath::ClosestPointOnSegment(S, Capsule.A, Capsule.B);
	const FVector Away = S - Closest;
	const double Distance = Away.Size();
	if (Distance >= Capsule.Radius)
	{
		return FVector::ZeroVector;
	}
	const FVector Axis = (Capsule.B - Capsule.A).GetSafeNormal();
	const FVector Side = FVector::VectorPlaneProject(Outward, Axis).GetSafeNormal();
	if (Distance > UE_KINDA_SMALL_NUMBER && (Side.IsZero() || FVector::DotProduct(Away / Distance, Side) >= -0.5))
	{
		return Away / Distance * (Capsule.Radius - Distance);
	}
	// On the limb's line, or the limb come through to the cloth's outer side: out the body's side, across it.
	return Side.IsZero() ? FVector::ZeroVector : Side * (Capsule.Radius + Distance);
}

FVector FBodySwingPoints::PushBandOut(const FVector& S, const FBodySwingCapsule& Capsule, const FVector& Outward)
{
	const FVector Way = Capsule.Side.IsZero() ? Outward : Capsule.Side;
	FVector Push = PushPlaceOut(S, Capsule, Way);
	if (Push.Z < 0.0)
	{
		// Pushed down, though the cloth's side of the limb is up: a leg come up through it from below (a run's front
		// thigh rising under the cloth between two chains). Across it, over the leg: the place ends at least its
		// clearance off, whatever way it lay under it.
		const FVector Closest = FMath::ClosestPointOnSegment(S, Capsule.A, Capsule.B);
		const FVector Away = S - Closest;
		const FVector Side = FVector::VectorPlaneProject(Way, (Capsule.B - Capsule.A).GetSafeNormal()).GetSafeNormal();
		if (!Side.IsZero() && Side.Z > 0.0 && FVector::DotProduct(Away, Side) < 0.0)
		{
			Push = Side * (Capsule.Radius + Away.Size());
		}
	}
	// Lifted over a leg come up under it, never pulled down (the hem isn't dragged toward the ground).
	Push.Z = FMath::Max(Push.Z, 0.0);
	return Push;
}

FVector FBodySwingPoints::KeepUnder(const FVector& Head, const FVector& Tip, double Length, double CeilingZ)
{
	if (Tip.Z <= CeilingZ)
	{
		return Tip;
	}
	// At the ceiling, out the way it went (straight up has no way: then it hangs as it is).
	const FVector Flat = FVector(Tip.X - Head.X, Tip.Y - Head.Y, 0.0).GetSafeNormal();
	if (Flat.IsZero())
	{
		return Tip;
	}
	const double Rise = FMath::Clamp(CeilingZ - Head.Z, 0.0, Length);
	return Head + Flat * FMath::Sqrt(FMath::Max(Length * Length - Rise * Rise, 0.0)) + FVector(0.0, 0.0, Rise);
}

FVector FBodySwingPoints::KeepAbove(const FVector& Head, const FVector& Tip, double Length, double FloorZ, const FVector& Lean,
	double MaxReach)
{
	if (Tip.Z >= FloorZ)
	{
		return Tip;
	}
	FVector Flat(Tip.X - Head.X, Tip.Y - Head.Y, 0.0);
	if (!Flat.Normalize())
	{
		Flat = FVector(Lean.X, Lean.Y, 0.0);
		if (!Flat.Normalize())
		{
			Flat = FVector::ForwardVector;
		}
	}
	// As low as it can hang, on the floor, out the way it leans (it lies on the ground, not through it), no further out
	// than MaxReach.
	const double Drop = FMath::Clamp(Head.Z - FloorZ, 0.0, Length);
	const double Reach = FMath::Min(FMath::Sqrt(Length * Length - Drop * Drop), FMath::Max(MaxReach, 0.0));
	return Head + Flat * Reach - FVector(0.0, 0.0, Drop);
}

void FBodySwingPoints::Step(const FVector& Anchor, TConstArrayView<FVector> Targets, TConstArrayView<float> Lengths,
	TConstArrayView<FBodySwingCapsule> Capsules, const FBodySwingSettings& Settings, float DeltaSeconds,
	const FBodySwingFloor& Floor)
{
	// How far out each point may lie on the floor: its length, but the last its share (the hem gathers).
	auto ReachOf = [&](int32 Index) {
		return static_cast<double>(Lengths[Index]) * (Index == Targets.Num() - 1 ? Floor.LastReach : 1.f);
	};
	const int32 Count = Targets.Num();
	if (!bStarted || Points.Num() != Count || FVector::Dist(Anchor, LastAnchor) > TeleportDistance)
	{
		Reset(Targets);
		LastAnchor = Anchor;
		LastStepAnchor = Anchor;
		LastBodyRotation = BodyRotation;
		LastStepRotation = BodyRotation;
		LastBodyPivot = BodyPivot;
		LastStepPivot = BodyPivot;
		return;
	}
	// A fixed step, the leftover carried to the next frame: chains joined by length constraints swing differently at
	// different step sizes (a stretch of 1/60 s steps and one of 1/240 s ended 20 cm apart), so every frame rate takes the
	// same steps. A spring toward the animated pose (an acceleration: a nudge of the position each step was a stronger
	// spring the shorter the step), damped, under a share of gravity; Verlet.
	const float Frame = FMath::Clamp(DeltaSeconds, 0.f, MaxFrame);
	const float Carried = Accumulator;
	Accumulator += Frame;
	const float Keep = FMath::Exp(-Settings.Damping * FixedStep);
	const float Spring = Settings.Stiffness * Settings.Stiffness;
	const FVector Gravity(0.0, 0.0, GravityZ * Settings.Gravity);
	const int32 PerPoint = Capsules.Num() / FMath::Max(Count, 1);
	const float TurnShare = FMath::Clamp(Settings.TurnCarry * CVarBodySwingTurnCarry.GetValueOnAnyThread(), 0.f, 1.f);
	TArray<FVector, TInlineAllocator<8>> StepTargets;
	StepTargets.SetNum(Count);
	if (Rises.Num() != Count)
	{
		Rises.Init(0.f, Count);
	}
	int32 Stepped = 0;
	while (Accumulator >= FixedStep - UE_KINDA_SMALL_NUMBER)
	{
		// Where the anchor (and the animated pose with it) was when this step ends, between last frame's and this one's,
		// and how the body was turned then.
		const float Along = Frame > 0.f ? FMath::Min(1.f, (FixedStep * static_cast<float>(Stepped + 1) - Carried) / Frame) : 1.f;
		const FQuat StepRotation = FQuat::Slerp(LastBodyRotation, BodyRotation, Along).GetNormalized();
		const FVector StepPivot = FMath::Lerp(LastBodyPivot, BodyPivot, static_cast<double>(Along));
		// The anchor goes round with the turn, not across its chord (cut across, a quick turn's middle steps pulled it in
		// toward the pivot and back, and that flung the cloth even turned all the way with the body); with no turn, the
		// straight line between last frame's and this one's.
		const FVector OnBodyBefore = LastBodyRotation.UnrotateVector(LastAnchor - LastBodyPivot);
		const FVector OnBodyNow = BodyRotation.UnrotateVector(Anchor - BodyPivot);
		const FVector Head0 = StepPivot + StepRotation.RotateVector(FMath::Lerp(OnBodyBefore, OnBodyNow, static_cast<double>(Along)));
		// The animated pose then: this frame's, moved back to the step's anchor and turned back by the turn still to come
		// this frame (none when the body doesn't turn: the frame's pose moved, as it always was).
		const FQuat Unturn = (BodyRotation * StepRotation.Inverse()).GetNormalized().Inverse();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			StepTargets[Index] = Head0 + Unturn.RotateVector(Targets[Index] - Anchor);
		}
		// Where the last step's anchor goes with the body's turn as far as it's carried (the rest of its move is Carry's).
		FVector Turned = LastStepAnchor;
		if (TurnShare > 0.f)
		{
			// Its share of the body's turn since the last step, round the body's pivot: where and how fast (Previous too),
			// so the cloth's own swing turns with the body and only the rest of the turn sweeps it out.
			const FQuat Turn = (StepRotation * LastStepRotation.Inverse()).GetNormalized();
			const FQuat Share = FQuat::Slerp(FQuat::Identity, Turn, TurnShare).GetNormalized();
			if (!Share.Equals(FQuat::Identity, UE_SMALL_NUMBER))
			{
				for (int32 Index = 0; Index < Count; ++Index)
				{
					Points[Index] = LastStepPivot + Share.RotateVector(Points[Index] - LastStepPivot);
					Previous[Index] = LastStepPivot + Share.RotateVector(Previous[Index] - LastStepPivot);
				}
				Turned = LastStepPivot + Share.RotateVector(LastStepAnchor - LastStepPivot);
			}
		}
		if (Settings.Carry > 0.f)
		{
			// Its share of the anchor's move since the last step (past what the turn carried), handed on as it is (where and
			// how fast: Previous too).
			const FVector Handed = (Head0 - Turned) * static_cast<double>(Settings.Carry);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				Points[Index] += Handed;
				Previous[Index] += Handed;
			}
		}
		LastStepAnchor = Head0;
		LastStepRotation = StepRotation;
		LastStepPivot = StepPivot;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Velocity = (Points[Index] - Previous[Index]) * Keep;
			const FVector Accel = (StepTargets[Index] - Points[Index]) * Spring + Gravity;
			Previous[Index] = Points[Index];
			Points[Index] += Velocity + Accel * (FixedStep * FixedStep);
		}
		// Each point its length from the one before, and out of the limbs, from the anchor down.
		FVector Head = Head0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Rest = (StepTargets[Index] - Head).GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);
			FVector P = Head + (Points[Index] - Head).GetSafeNormal(UE_SMALL_NUMBER, Rest) * Lengths[Index];
			const FVector Free = P;
			bool bLifted = false;
			for (int32 Limb = 0; Limb < PerPoint; ++Limb)
			{
				const FBodySwingCapsule& Capsule = Capsules[Index * PerPoint + Limb];
				const FVector Before = P;
				P = PushOut(P, Capsule);
				// The cloth's side of the limb: a leg's own (turned with it), else the chain's way out of the body.
				const FVector Way = Capsule.bShortestWay ? FVector::ZeroVector : (Capsule.Side.IsZero() ? Outward : Capsule.Side);
				P = PushBoneOut(Head, P, Capsule, Way);
				bLifted |= Capsule.bLifts && !P.Equals(Before, UE_KINDA_SMALL_NUMBER);
			}
			P = Head + (P - Head).GetSafeNormal(UE_SMALL_NUMBER, Rest) * Lengths[Index];
			// Nothing rises over what it hangs from, but a leg come up under it lifts it some (a sprint's back heel).
			Rises[Index] = bLifted ? Lengths[Index] * LegLift : 0.f;
			P = KeepUnder(Head, P, Lengths[Index], (Settings.bRisesToTarget ? FMath::Max(Head.Z, StepTargets[Index].Z) : Head.Z) + Rises[Index]);
			const bool bGrounded = P.Z < Floor.Z;
			P = KeepAbove(Head, P, Lengths[Index], Floor.Z, Rest, ReachOf(Index));
			// A limb moves the cloth aside without throwing it: the push isn't carried on as speed (it flung a chain over
			// the thigh it was pushed off). Nor does it keep its speed into what pushed it: kept, a point held against a
			// limb stored a fall into it step on step, and slid round the limb on it, back and forth (Ellis's and Crane's
			// capes' backs wagged 7 to 14 cm side to side at a standstill, 2026-10-20). On the ground it rests (its speed
			// into the ground, kept, flipped a gathered hem up over its head).
			const FVector Push = P - Free;
			FVector Velocity = Free - Previous[Index];
			if (Push.SizeSquared() > UE_KINDA_SMALL_NUMBER)
			{
				const FVector Out = Push.GetUnsafeNormal();
				const double Into = FVector::DotProduct(Velocity, Out);
				if (Into < 0.0)
				{
					Velocity -= Out * Into;
				}
			}
			Previous[Index] = P - Velocity;
			if (bGrounded)
			{
				Previous[Index] = P;
			}
			Points[Index] = P;
			Head = P;
		}
		Accumulator -= FixedStep;
		++Stepped;
	}
	LastAnchor = Anchor;
	LastBodyRotation = BodyRotation;
	LastBodyPivot = BodyPivot;

	// Shown: between the last two steps by the leftover (no stutter when a frame takes no step or two), hung from where
	// the anchor is now, each point its length from the one before.
	const float Blend = FMath::Clamp(Accumulator / FixedStep, 0.f, 1.f);
	Shown.SetNum(Count);
	FVector Head = Anchor;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Between = FMath::Lerp(Previous[Index], Points[Index], static_cast<double>(Blend));
		const FVector Rest = (Targets[Index] - Head).GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);
		Shown[Index] = Head + (Between - Head).GetSafeNormal(UE_SMALL_NUMBER, Rest) * Lengths[Index];
		Shown[Index] = KeepUnder(Head, Shown[Index], Lengths[Index],
			(Settings.bRisesToTarget ? FMath::Max(Head.Z, Targets[Index].Z) : Head.Z) + Rises[Index]);
		Shown[Index] = KeepAbove(Head, Shown[Index], Lengths[Index], Floor.Z, Rest, ReachOf(Index));
		Head = Shown[Index];
	}
}
