#include "Player/FirstPersonHandsMotion.h"
#include "Player/FirstPersonHandsFraming.h"
#include "Player/FirstPersonHandsGrips.h"
#include "Player/PlayerMeleeMotion.h"
#include "HAL/IConsoleManager.h"

// The first-person hands' casts and the melee tool's hand: an ember power's cast by the support hand (the user's note 27,
// 2026-10-27) and the trigger hand on the hero's tool (note 5), in camera space (X ahead, Y right, Z up; cm).

namespace
{
	TAutoConsoleVariable<float> CVarCastFlare(TEXT("Looter.Hands.Cast.Flare"), 0.07f,
		TEXT("Seconds the cast's hand takes from gathered to flung open as the ember power fires."));
	TAutoConsoleVariable<float> CVarCastHold(TEXT("Looter.Hands.Cast.Hold"), 0.12f,
		TEXT("Seconds the cast's hand stays out, open, after the power fires."));
	TAutoConsoleVariable<float> CVarCastBack(TEXT("Looter.Hands.Cast.Back"), 0.18f,
		TEXT("Seconds the cast's hand takes back to the gun."));
	TAutoConsoleVariable<float> CVarCastSplay(TEXT("Looter.Hands.Cast.Splay"), 12.f,
		TEXT("Degrees the fingers fan apart as the cast's hand flares open."));
	TAutoConsoleVariable<float> CVarCastDrift(TEXT("Looter.Hands.Cast.Drift"), 3.f,
		TEXT("Centimetres the open hand drifts on along its release while it's held out."));

	float Flare() { return FMath::Max(CVarCastFlare.GetValueOnAnyThread(), 0.01f); }
	float Hold() { return FMath::Max(CVarCastHold.GetValueOnAnyThread(), 0.f); }
	float Back() { return FMath::Max(CVarCastBack.GetValueOnAnyThread(), 0.01f); }

	FHandPlacement Spot(const FVector& At, const FVector& Fingers, const FVector& Palm, EHandGrip Grip, const FVector& Forearm)
	{
		return FHandPlacement::Make(At, Fingers.GetSafeNormal(), Palm.GetSafeNormal(), FHandFingers::Of(Grip), Forearm);
	}

	/** The fingers fanned apart (the index toward its side, the ring and little finger the other, the thumb out). */
	FHandFingers Splayed(FHandFingers Fingers, float Degrees)
	{
		const float Fan[FHandFingers::Count] = { -1.f, 1.f, 0.f, -1.f, -2.f };
		for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
		{
			Fingers.Spread[Finger] += Fan[Finger] * Degrees;
		}
		return Fingers;
	}

	/**
	 * A fist closed round a slim handle (a 3-3.5 cm shaft): a gun grip's curls with the index finger closed like the rest.
	 * The jar's loose Wrap left Crane's fingers open beside the gaff where touch found no shaft to close on (DEMO, round 3);
	 * from these the contact only eases each finger onto the shaft's surface.
	 */
	FHandFingers ClosedRound()
	{
		FHandFingers Fist = FHandFingers::Of(EHandGrip::Trigger);
		const int32 Middle = FHandFingers::IndexFinger + 1;
		for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
		{
			Fist.Curl[FHandFingers::IndexFinger][Joint] = Fist.Curl[Middle][Joint];
		}
		Fist.Spread[FHandFingers::IndexFinger] = 0.f;
		return Fist;
	}

	/** The same placement for the other hand: mirrored across the view's middle. */
	FHandPlacement Mirrored(const FHandPlacement& Placement)
	{
		const FVector Flip(1.f, -1.f, 1.f);
		return FHandPlacement::Make(Placement.Hold * Flip, Placement.GetFingersDir() * Flip, Placement.GetPalmDir() * Flip, Placement.Fingers,
			Placement.Forearm * Flip);
	}
}

EHandsCast FirstPersonHandsMotion::CastKindOf(FName PowerId)
{
	// By what each power does with the hand (Docs/Pipeline/EmberPowers.md): flung things, pushed marks, worn powers, set-downs.
	static const TMap<FName, EHandsCast> Kinds = {
		{ TEXT("FiftyTwoPickup"), EHandsCast::Fling },
		{ TEXT("SlagBomb"), EHandsCast::Fling },
		{ TEXT("TollChain"), EHandsCast::Fling },
		{ TEXT("LuckyStreak"), EHandsCast::Clench },
		{ TEXT("DoubleOrNothing"), EHandsCast::Clench },
		{ TEXT("IronHide"), EHandsCast::Clench },
		{ TEXT("BorrowedTime"), EHandsCast::Clench },
		{ TEXT("Sundown"), EHandsCast::Clench },
		{ TEXT("GraveWard"), EHandsCast::Plant },
		{ TEXT("Tripwire"), EHandsCast::Plant },
		{ TEXT("DustDevil"), EHandsCast::Push },
		{ TEXT("LastRites"), EHandsCast::Push },
		{ TEXT("RailSpike"), EHandsCast::Push },
		{ TEXT("Spyglass"), EHandsCast::Push },
		{ TEXT("LongShot"), EHandsCast::Push },
	};
	const EHandsCast* Found = Kinds.Find(PowerId);
	return Found ? *Found : EHandsCast::Push;
}

float FirstPersonHandsMotion::CastSeconds(float Lead)
{
	return FMath::Max(Lead, 0.f) + Flare() + Hold() + Back();
}

FHandPlacement FirstPersonHandsMotion::CastRaised(EHandsCast Kind)
{
	// Gathered low on the left, in view but clear of the crosshair, while the power's lead runs.
	switch (Kind)
	{
	case EHandsCast::Fling:
		// Drawn back by the cheek, the fingers up, pinching what it's about to fling.
		return Spot(FVector(20.f, -20.f, -12.f), FVector(0.2f, 0.3f, 1.f), FVector(0.5f, 0.85f, 0.f), EHandGrip::Pinch, FVector(-0.3f, -0.45f, -0.85f));
	case EHandsCast::Clench:
		// Opened palm up, gathering.
		return Spot(FVector(28.f, -12.f, -14.f), FVector(0.9f, 0.4f, 0.15f), FVector(0.f, 0.1f, 1.f), EHandGrip::Open, FVector(-0.6f, -0.4f, -0.7f));
	case EHandsCast::Plant:
		// Raised palm down over the ground ahead.
		return Spot(FVector(26.f, -16.f, -8.f), FVector(0.85f, 0.45f, 0.1f), FVector(0.f, 0.15f, -1.f), EHandGrip::Open, FVector(-0.6f, -0.4f, -0.6f));
	default:
		// Drawn in to the chest, a loose fist.
		return Spot(FVector(24.f, -14.f, -16.f), FVector(0.6f, 0.3f, 0.7f), FVector(0.1f, 1.f, 0.f), EHandGrip::Wrap, FVector(-0.4f, -0.4f, -0.8f));
	}
}

FHandPlacement FirstPersonHandsMotion::CastReleased(EHandsCast Kind)
{
	switch (Kind)
	{
	case EHandsCast::Fling:
		// Flung out across toward the middle, palm down, fingers open after what left them.
		return Spot(FVector(44.f, -4.f, -8.f), FVector(1.f, 0.35f, 0.15f), FVector(0.f, 0.2f, -1.f), EHandGrip::Open, FVector(-0.75f, -0.4f, -0.5f));
	case EHandsCast::Clench:
		// Clenched to a fist before the face, the palm toward the eye.
		return Spot(FVector(30.f, -9.f, -4.f), FVector(0.1f, 0.35f, 1.f), FVector(-1.f, 0.2f, 0.f), EHandGrip::Wrap, FVector(-0.3f, -0.4f, -0.85f));
	case EHandsCast::Plant:
		// Pressed down toward the ground, palm flat.
		return Spot(FVector(36.f, -14.f, -30.f), FVector(0.9f, 0.35f, -0.3f), FVector(0.2f, 0.1f, -1.f), EHandGrip::Open, FVector(-0.5f, -0.4f, -0.75f));
	default:
		// Pushed straight out, palm first, the fingers up.
		return Spot(FVector(48.f, -8.f, -6.f), FVector(0.15f, 0.2f, 1.f), FVector(1.f, 0.1f, 0.f), EHandGrip::Open, FVector(-0.7f, -0.3f, -0.6f));
	}
}

FHandPlacement FirstPersonHandsMotion::SupportInCast(const FHandPlacement& OnGun, EHandsCast Kind, float CastTime, float Lead)
{
	const float L = FMath::Max(Lead, 0.f);
	if (CastTime < 0.f || CastTime >= CastSeconds(L))
	{
		return OnGun;
	}
	const FHandPlacement Raised = CastRaised(Kind);
	if (CastTime < L)
	{
		// Up off the gun and gathered, slowing as it gets there: the power is about to go.
		return FHandPlacement::Blend(OnGun, Raised, FMath::InterpEaseOut(0.f, 1.f, CastTime / L, 2.f));
	}
	FHandPlacement Released = CastReleased(Kind);
	// A clench closes to a fist; the rest flare open, the fingers fanned.
	if (Kind != EHandsCast::Clench)
	{
		Released.Fingers = Splayed(Released.Fingers, CVarCastSplay.GetValueOnAnyThread());
	}
	// No lead (a console use, or no hands showing at the press): the flare starts from the gun.
	const FHandPlacement& From = L > 0.f ? Raised : OnGun;
	const float Since = CastTime - L;
	if (Since < Flare())
	{
		// Snapped out, fastest as it opens: the release reads with the power.
		return FHandPlacement::Blend(From, Released, FMath::InterpEaseOut(0.f, 1.f, Since / Flare(), 3.f));
	}
	// Held out a moment, drifting on along its way.
	FHandPlacement Out = Released;
	const FVector Way = (Released.Hold - Raised.Hold).GetSafeNormal();
	const float Held = FMath::Clamp((Since - Flare()) / FMath::Max(Hold(), 0.01f), 0.f, 1.f);
	Out.Hold += Way * (CVarCastDrift.GetValueOnAnyThread() * Held);
	if (Since < Flare() + Hold())
	{
		return Out;
	}
	return FHandPlacement::Blend(Out, OnGun, FMath::SmoothStep(0.f, 1.f, (Since - Flare() - Hold()) / Back()));
}

FHandPlacement FirstPersonHandsMotion::OnMeleeTool(const FTransform& Grip, float HandleRadius, bool bForearm, bool bRoundHandle)
{
	const FQuat Turn = Grip.GetRotation();
	const FVector Along = Turn.GetAxisX();
	const FVector Strike = Turn.GetAxisZ();
	if (bForearm)
	{
		// A fist inside the forearm piece: the knuckles along the punch, the palm down, the forearm straight back behind it.
		return FHandPlacement::Make(Grip.GetLocation(), Along, -Strike, ClosedRound(), -Along);
	}
	// Round the handle: the handle runs the index-and-thumb way across the palm, the palm onto it from the tool's -Y side,
	// so the knuckles lead the striking side (a hammer grip).
	FVector Fingers;
	FVector Palm;
	FirstPersonHandsGrips::OnHandle(EFirstPersonHand::Right, Along, Turn.GetAxisY(), 0.f, Fingers, Palm);
	if (bRoundHandle)
	{
		// A pole is the same all the way round: the fist turns on it so the wrist lies toward the shoulder and the forearm
		// runs back to it (a real wrist barely bends sideways). Kept to the tool's palm side, the gaff's chop and yank hung
		// the hand under the pole, the wrist above it and the forearm bent back up over it, so from the eye the glove sat
		// beside the shaft with its fingers out of sight behind it (playtest 2026-10-28). Where the arm runs along the pole
		// (the shoulder nearly straight down it) the tool's own palm side stays.
		const FVector ToShoulder = MeleeMotion::ShoulderPoint() - Grip.GetLocation();
		const FVector Across = ToShoulder - Along * FVector::DotProduct(ToShoulder, Along);
		const float Share = FMath::Clamp(static_cast<float>(Across.Size() / FMath::Max(ToShoulder.Size(), 1.0)) / 0.3f, 0.f, 1.f);
		if (Share > 0.f)
		{
			const FVector Knuckles = (-Across.GetSafeNormal() * Share + Fingers * (1.f - Share)).GetSafeNormal(UE_SMALL_NUMBER, Fingers);
			// The right hand's palm for those knuckles on this handle (OnHandle's Fingers = Along x Palm, turned round).
			FirstPersonHandsGrips::OnHandle(EFirstPersonHand::Right, Along, FVector::CrossProduct(Knuckles, Along), 0.f, Fingers, Palm);
		}
	}
	// The forearm behind the knuckles and down, out toward the right shoulder (the framing slants it out of view after).
	const FVector Forearm = (-Fingers + FVector(-0.4f, 0.35f, -0.5f)).GetSafeNormal();
	FHandPlacement Placement = FHandPlacement::Make(Grip.GetLocation(), Fingers, Palm, ClosedRound(), Forearm);
	// The fingers close till they touch the handle, the palm seated on it.
	FHandleShape Handle;
	Handle.bValid = true;
	Handle.Center = Grip.GetLocation();
	Handle.Axis = Along;
	Handle.Across = Strike;
	Handle.HalfAcross = HandleRadius;
	Handle.HalfThrough = HandleRadius;
	Handle.Corner = HandleRadius;
	Placement.Contact.Handle = Handle;
	Placement.Contact.Share = 1.f;
	Placement.Contact.Seat = 1.f;
	return Placement;
}

FHandPlacement FirstPersonHandsMotion::BareRest(EFirstPersonHand Hand)
{
	// The parked support hand's spot (a one-handed shooter's free hand, under the frame), and its mirror for the right.
	const FHandPlacement Left = FPHandsFraming::ParkedSupport();
	return Hand == EFirstPersonHand::Left ? Left : Mirrored(Left);
}
