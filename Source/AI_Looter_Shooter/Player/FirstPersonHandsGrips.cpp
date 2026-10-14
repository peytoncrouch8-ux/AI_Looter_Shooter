#include "Player/FirstPersonHandsGrips.h"

// The hands' holds on each gun kind, read off the gun models (Art/Models/Weapons/<Gun>.py; their u, v, w are X, Z and
// -Y here), and the placements' own math. The tables are the tuning knobs: everything the hands do on a gun starts here.

namespace
{
	/**
	 * The Bullpup (DA_AssaultRifle): the trigger hand round the thumbhole loop's rear post (raked 8 degrees forward at its
	 * foot), held a touch to the right so the wrist clears the magazine right behind it; the support hand under the
	 * shell's nose, its palm on the shell's underside (the socket is a centimetre under it), fingers curling up its right.
	 */
	FGunHands MakeRifle()
	{
		FGunHands Hands;
		Hands.GripIndexUp = FVector(-0.146f, 0.f, 0.989f);
		Hands.GripPalm = FVector(0.f, -1.f, 0.f);
		Hands.GripHold = FVector(0.f, 1.f, 0.f);
		// The trigger is 4 cm ahead of the post: a short reach, the finger well bent.
		Hands.TriggerCurl[0] = 30.f;
		Hands.TriggerCurl[1] = 50.f;
		Hands.TriggerCurl[2] = 25.f;
		Hands.ForeFingers = FVector(0.5f, 0.85f, 0.1f);
		Hands.ForePalm = FVector(0.f, 0.25f, 1.f);
		Hands.ForeHold = FVector(0.f, 0.f, 3.f);
		Hands.ForeGrip = EHandGrip::Forend;
		// The charging handle on the shell's left, ahead of the sight rail (Bullpup.py's fittings).
		Hands.Handle = FVector(41.5f, -1.9f, 3.2f);
		// A 30's middle: its seat is the middle of its top, it runs 7 cm front to back and 18 down.
		Hands.MagazineGrab = FVector(0.5f, 0.f, -8.f);
		return Hands;
	}

	/**
	 * The Ranchhand (DA_PumpShotgun): the trigger hand on the stock's wrist, which is tall (9 cm), so the hand lies on its
	 * right side with the fingers curled under its lower edge (that edge runs up toward the trigger: the index finger's
	 * way); the support hand round the pump, which SOCKET_Foregrip marks the middle of.
	 */
	FGunHands MakeShotgun()
	{
		FGunHands Hands;
		Hands.GripIndexUp = FVector(0.87f, 0.f, 0.48f);
		Hands.GripPalm = FVector(0.f, -1.f, 0.f);
		Hands.GripHold = FVector(0.f, 1.f, 0.f);
		// The trigger is 12 cm ahead of the grip: the finger reaches out nearly straight.
		Hands.TriggerCurl[0] = 5.f;
		Hands.TriggerCurl[1] = 20.f;
		Hands.TriggerCurl[2] = 10.f;
		Hands.ForeFingers = FVector(0.4f, 0.9f, 0.15f);
		Hands.ForePalm = FVector(0.f, 0.35f, 1.f);
		Hands.ForeHold = FVector::ZeroVector;
		Hands.ForeGrip = EHandGrip::Forend;
		// Under the receiver, between the trigger guard and the magazine tube.
		Hands.LoadingPort = FVector(15.5f, 0.f, -3.2f);
		return Hands;
	}

	/**
	 * The Drover (DA_Revolver): the trigger hand round the raked grip (it leans back 17 degrees), palm wrapping a little
	 * round its back; the support hand cupped round the shooting hand from the left, out past its fingers and a little under
	 * (the support socket is where it presses on the grip, where the shooting hand's fingers already are: the third-person
	 * stance moves it the same way, LooterStancePoseWeapon.cpp).
	 */
	FGunHands MakeRevolver()
	{
		FGunHands Hands;
		Hands.GripIndexUp = FVector(0.3f, 0.f, 0.95f);
		Hands.GripPalm = FVector(0.2f, -1.f, 0.f);
		Hands.GripHold = FVector(0.f, 0.3f, 0.f);
		Hands.TriggerCurl[0] = 25.f;
		Hands.TriggerCurl[1] = 50.f;
		Hands.TriggerCurl[2] = 25.f;
		Hands.ForeFingers = FVector(0.55f, 0.45f, -0.7f);
		Hands.ForePalm = FVector(0.f, 1.f, 0.45f);
		Hands.ForeHold = FVector(0.f, -2.f, -1.f);
		Hands.ForeGrip = EHandGrip::Support;
		return Hands;
	}
}

const FGunHands& FirstPersonHandsGrips::For(EWeaponKind Kind)
{
	static const FGunHands Rifle = MakeRifle();
	static const FGunHands Shotgun = MakeShotgun();
	static const FGunHands Revolver = MakeRevolver();
	switch (Kind)
	{
	case EWeaponKind::Shotgun:
		return Shotgun;
	case EWeaponKind::Revolver:
		return Revolver;
	default:
		return Rifle;
	}
}

FHandPlacement FirstPersonHandsGrips::InGun(const FHandsGun& Gun, const FVector& Hold, const FVector& FingersDir, const FVector& PalmDir,
	const FHandFingers& Fingers)
{
	return FHandPlacement::Make(Gun.ToCameraPoint(Hold), Gun.ToCameraDir(FingersDir), Gun.ToCameraDir(PalmDir), Fingers);
}

FHandPlacement FirstPersonHandsGrips::OnGrip(const FHandsGun& Gun)
{
	const FGunHands& Hands = For(Gun.Kind);
	// The right hand's frame: up the grip runs pinky to index, the palm faces into the grip, and its knuckles run square to
	// both (for a right hand, fingers = index-ward cross palm).
	const FVector IndexUp = Hands.GripIndexUp.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	FVector Palm = Hands.GripPalm - IndexUp * FVector::DotProduct(IndexUp, Hands.GripPalm);
	Palm = Palm.GetSafeNormal(UE_SMALL_NUMBER, FVector(0.f, -1.f, 0.f));
	const FVector Fingers = FVector::CrossProduct(IndexUp, Palm);

	FHandFingers Curls = FHandFingers::Of(EHandGrip::Trigger);
	for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
	{
		Curls.Curl[FHandFingers::IndexFinger][Joint] = Hands.TriggerCurl[Joint];
	}
	return InGun(Gun, Gun.Grip + Hands.GripHold, Fingers, Palm, Curls);
}

FHandPlacement FirstPersonHandsGrips::OnForegrip(const FHandsGun& Gun, float PumpBack)
{
	const FGunHands& Hands = For(Gun.Kind);
	// A racked pump slides back along the gun's -X, and the hand round it goes with it.
	const FVector Hold = Gun.Foregrip + Hands.ForeHold - FVector(PumpBack, 0.f, 0.f);
	return InGun(Gun, Hold, Hands.ForeFingers, Hands.ForePalm, FHandFingers::Of(Hands.ForeGrip));
}

// ---------------------------------------------------------------------------
// FHandPlacement
// ---------------------------------------------------------------------------

FHandPlacement FHandPlacement::Make(const FVector& InHold, const FVector& FingersDir, const FVector& PalmDir, const FHandFingers& InFingers)
{
	FHandPlacement Placement;
	Placement.Hold = InHold;
	Placement.Turn = FirstPersonHands::Frame(FingersDir, PalmDir);
	Placement.Fingers = InFingers;
	return Placement;
}

FHandPlacement FHandPlacement::Blend(const FHandPlacement& From, const FHandPlacement& To, float Alpha)
{
	const float Share = FMath::Clamp(Alpha, 0.f, 1.f);
	FHandPlacement Placement;
	Placement.Hold = FMath::Lerp(From.Hold, To.Hold, Share);
	Placement.Turn = FQuat::Slerp(From.Turn, To.Turn, Share).GetNormalized();
	Placement.Fingers = FHandFingers::Blend(From.Fingers, To.Fingers, Share);
	return Placement;
}

FHandPlacement FHandPlacement::Moved(const FTransform& Space) const
{
	FHandPlacement Placement = *this;
	Placement.Hold = Space.TransformPosition(Hold);
	Placement.Turn = (Space.GetRotation() * Turn).GetNormalized();
	return Placement;
}
