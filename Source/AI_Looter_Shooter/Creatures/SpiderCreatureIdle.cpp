// ASpiderCreature's idle life and its notice beat's pose (moved out of SpiderCreatureReact.cpp, 2026-10-31, with the weight
// shift added): standing about with nobody to hunt, now and then it taps a front leg, grooms its front legs at its fangs, or
// shifts its weight (two of its middle or back legs lifted and set down a hand's width off, the body leaning off them a
// moment), each spider on its own dice; the notice beat rears it with its front legs up and its fangs flared. Anything else
// (a hunt, a hit, a perch) takes its legs back at once.

#include "Creatures/SpiderCreature.h"
#include "Creatures/CreatureReactComponent.h"

namespace
{
	/** The notice beat: it rears this far (degrees, and cm at size 1), its fangs spread this wide, its front legs this share of the way to raised. */
	constexpr float NoticeRear = 16.f;
	constexpr float NoticeRise = 10.f;
	constexpr float NoticeFang = 30.f;
	constexpr float NoticeLegRaise = 0.55f;

	/** Idle life: an act every this many seconds or so (the first no sooner than FirstIdleAct), a tap this long (lifted this high, cm at size 1) or a groom this long. */
	constexpr float IdleEvery[2] = { 3.5f, 8.f };
	constexpr float FirstIdleAct = 2.5f;
	/** Its acts' shares: a tap, a weight shift, else a groom. */
	constexpr float TapShare = 0.4f;
	constexpr float ShiftShare = 0.35f;
	/**
	 * A weight shift (2026-10-31): two legs lifted and set down a hand's width off their spots, ShiftDue seconds in, the body
	 * leaning ShiftLean degrees off them; it takes ShiftSeconds. Each foot moves at most ShiftReach (cm at size 1): less than
	 * the settle rule's 12, so the gait leaves it there.
	 */
	constexpr float ShiftSeconds = 1.f;
	constexpr float ShiftDue[2] = { 0.1f, 0.45f };
	constexpr float ShiftLean = 1.5f;
	constexpr float ShiftReach = 9.f;
	constexpr float TapSeconds = 0.9f;
	constexpr float TapLift = 13.f;
	constexpr float GroomSeconds = 1.8f;
	constexpr float GroomRubHz = 3.2f;
	/** Where a groom draws the front feet: before the fangs (the body's frame at size 1: ahead, out to each side, below the thorax). */
	const FVector GroomSpot(60.f, 14.f, -18.f);

	/** Up over In seconds, held, and down over the last Out seconds of Length. */
	float SpiderEnvelope(float Time, float Length, float In, float Out)
	{
		return FMath::SmoothStep(0.f, In, Time) * (1.f - FMath::SmoothStep(Length - Out, Length, Time));
	}
}

void ASpiderCreature::TickIdleLife(float DeltaSeconds)
{
	UCreatureReactComponent* Reacts = GetReact();
	if (!Reacts)
	{
		return;
	}
	FRandomStream& Dice = Reacts->GetRandom();
	if (!bIdleClockSet)
	{
		bIdleClockSet = true;
		NextIdleAct = Dice.FRandRange(FirstIdleAct, IdleEvery[1]);
	}
	// Standing about with nobody to hunt, now and then it taps a leg or grooms; anything else takes its legs back at once.
	const bool bCalm = GetCreatureState() == ECreatureState::Idle && GetVelocity().SizeSquared2D() < 100.0 && !IsNoticing()
		&& EmergeTime < 0.f && !IsPoseFrozen() && Lurk.Phase == ESpiderLurkPhase::None;
	IdleWeight = FMath::FInterpTo(IdleWeight, bCalm ? 1.f : 0.f, DeltaSeconds, bCalm ? 4.f : 12.f);
	if (IdleAct != EIdleAct::None)
	{
		IdleActTime += DeltaSeconds;
		const float Length = IdleAct == EIdleAct::Tap ? TapSeconds : (IdleAct == EIdleAct::Shift ? ShiftSeconds : GroomSeconds);
		if (IdleActTime >= Length || (!bCalm && IdleWeight < 0.01f))
		{
			IdleAct = EIdleAct::None;
			IdleActTime = 0.f;
		}
		return;
	}
	if (!bCalm)
	{
		return;
	}
	NextIdleAct -= DeltaSeconds;
	if (NextIdleAct <= 0.f)
	{
		const float Roll = Dice.FRand();
		IdleAct = Roll < TapShare ? EIdleAct::Tap : (Roll < TapShare + ShiftShare ? EIdleAct::Shift : EIdleAct::Groom);
		IdleActTime = 0.f;
		IdleSide = Dice.FRand() < 0.5f ? -1.f : 1.f;
		if (IdleAct == EIdleAct::Shift && Legs.Num() >= 8)
		{
			// Two of its middle or back legs (never a front one: those tap and groom), each set down a little off its spot,
			// outward or along: its weight moved over.
			ShiftLegs[0] = Dice.RandRange(2, 7);
			ShiftLegs[1] = 2 + (ShiftLegs[0] - 2 + Dice.RandRange(2, 4)) % 6;
			for (int32 Each = 0; Each < 2; ++Each)
			{
				const float Side = Legs[ShiftLegs[Each]].Side;
				ShiftOffsets[Each] = FVector2D(Dice.FRandRange(-1.f, 1.f), Side * Dice.FRandRange(0.3f, 1.f)).GetSafeNormal() * Dice.FRandRange(4.f, ShiftReach);
			}
			ShiftDone = 0;
		}
		NextIdleAct = Dice.FRandRange(IdleEvery[0], IdleEvery[1]);
	}
}

int32 ASpiderCreature::IdleShiftLeg(FVector& OutOffset) const
{
	// The weight shift's next leg, once it's due (AnimateLegs steps it and counts it done).
	if (IdleAct != EIdleAct::Shift || IdleWeight < 0.5f || ShiftDone >= 2 || IdleActTime < ShiftDue[ShiftDone])
	{
		return INDEX_NONE;
	}
	const FVector2D Off = ShiftOffsets[ShiftDone];
	OutOffset = FRotator(0.f, GetActorRotation().Yaw, 0.f).RotateVector(FVector(Off.X, Off.Y, 0.f) * GetSizeScale());
	return ShiftLegs[ShiftDone];
}

void ASpiderCreature::ReactBody(float& TargetZ, float& TargetPitch, float& TargetRoll, float& TargetFang) const
{
	const float Notice = NoticePose;
	if (Notice > 0.f)
	{
		// Seen: reared, nose up, fangs spread wide.
		TargetPitch += NoticeRear * Notice;
		TargetZ += NoticeRise * GetSizeScale() * Notice;
		TargetFang = FMath::Max(TargetFang, NoticeFang * Notice);
	}
	if (IdleAct == EIdleAct::Groom && IdleWeight > 0.f)
	{
		// Its front lifted a little to the legs it draws up, its fangs working over them.
		const float Groom = IdleWeight * SpiderEnvelope(IdleActTime, GroomSeconds, 0.35f, 0.4f);
		TargetPitch += 3.f * Groom;
		TargetFang += Groom * (6.f + 6.f * FMath::Sin(2.f * UE_PI * 4.f * IdleActTime));
	}
	if (IdleAct == EIdleAct::Shift && IdleWeight > 0.f && Legs.IsValidIndex(ShiftLegs[0]))
	{
		// Its weight eased off the legs it lifts: a slow lean away from them and back.
		const float Shift = IdleWeight * SpiderEnvelope(IdleActTime, ShiftSeconds, 0.3f, 0.4f);
		TargetRoll -= ShiftLean * Shift * Legs[ShiftLegs[0]].Side;
	}
}

FVector ASpiderCreature::ReactFootOffset(const FLeg& Leg, const FTransform& Body) const
{
	if (Leg.Pair != 0)
	{
		return FVector::ZeroVector;
	}
	FVector Offset = FVector::ZeroVector;
	const float Notice = NoticePose;
	if (Notice > 0.001f)
	{
		// The front legs come up and reach as it rears (the bite's raise, lower).
		const FVector Raised = Body.TransformPosition(FVector(Leg.Hip.X + 55.f, Leg.Hip.Y * 1.4f, 25.f));
		Offset += (Raised - Leg.Foot) * (NoticeLegRaise * Notice);
	}
	if (IdleAct == EIdleAct::None || IdleWeight <= 0.f || Leg.bStepping)
	{
		return Offset;
	}
	const FQuat Turn = Body.GetRotation();
	if (IdleAct == EIdleAct::Tap && Leg.Side == IdleSide)
	{
		// Lifted, two quick taps on the ground, set down again.
		const float Lift = SpiderEnvelope(IdleActTime, TapSeconds, 0.18f, 0.2f);
		const float TapClock = FMath::Clamp((IdleActTime - 0.2f) / 0.5f, 0.f, 1.f);
		const float Dip = 0.5f * (1.f - FMath::Cos(2.f * UE_PI * 2.f * TapClock));
		Offset += (Turn.GetUpVector() * (TapLift * (1.f - 0.75f * Dip)) + Turn.GetForwardVector() * 6.f) * (GetSizeScale() * Lift * IdleWeight);
	}
	else if (IdleAct == EIdleAct::Groom)
	{
		// Both front legs drawn up to the fangs and rubbed over them, one then the other.
		const float Groom = SpiderEnvelope(IdleActTime, GroomSeconds, 0.35f, 0.4f) * IdleWeight;
		const float Rub = 6.f * Leg.Side * FMath::Sin(2.f * UE_PI * GroomRubHz * IdleActTime);
		const FVector AtFangs = Body.TransformPosition(FVector(GroomSpot.X + Rub, GroomSpot.Y * Leg.Side, GroomSpot.Z));
		Offset += (AtFangs - Leg.Foot) * Groom;
	}
	return Offset;
}

