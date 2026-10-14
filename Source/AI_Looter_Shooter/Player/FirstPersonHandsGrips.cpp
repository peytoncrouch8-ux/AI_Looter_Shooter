#include "Player/FirstPersonHandsGrips.h"

// The hands' holds on each gun kind, read off the gun models (Art/Models/Weapons/<Gun>.py; their u, v, w are X, Z and
// -Y here) and the approved look sheet (Tools/Blender/fp_arms_sheet.py: each hand put on a handle, turned to suit its
// forearm and palm), and the placements' own math. The tables are the tuning knobs: everything the hands do on a gun
// starts here.

namespace
{
	/**
	 * The Bullpup (DA_AssaultRifle): the trigger hand round the thumbhole loop's rear post (raked 8 degrees forward at its
	 * foot), held a touch to the right so the wrist rides beside the magazine just behind it, the forearm leaving back,
	 * right and down; the support hand under the handguard, its thumb along the left side, the fingers up the right side
	 * and over the top, its palm on the shell's underside (the socket is a centimetre under it).
	 */
	FGunHands MakeRifle()
	{
		FGunHands Hands;
		Hands.GripHandle = FVector(-0.14f, 0.f, 0.99f);
		Hands.GripPalm = FVector(0.15f, -1.f, 0.f);
		Hands.GripTilt = 12.f;
		Hands.GripForearm = FVector(-0.8f, 0.35f, -0.45f);
		Hands.GripHold = FVector(0.f, 0.5f, 0.f);
		// The trigger is 4 cm ahead of the post: a short reach, the finger well bent.
		Hands.TriggerCurl[0] = 30.f;
		Hands.TriggerCurl[1] = 50.f;
		Hands.TriggerCurl[2] = 25.f;
		Hands.ForeHandle = FVector(1.f, 0.f, 0.f);
		Hands.ForePalm = FVector(0.f, 0.3f, 1.f);
		Hands.ForeForearm = FVector(-0.45f, -0.5f, -0.75f);
		Hands.ForeHold = FVector(0.f, 0.6f, 4.5f);
		Hands.ForeGrip = EHandGrip::Forend;
		// The charging handle on the shell's left, ahead of the sight rail (Bullpup.py's fittings).
		Hands.Handle = FVector(41.5f, -1.9f, 3.2f);
		// A 30's middle: its seat is the middle of its top, it runs 7 cm front to back and 18 down; the palm on its left face.
		Hands.MagazineGrab = FVector(1.5f, 1.2f, -8.f);
		return Hands;
	}

	/**
	 * The Ranchhand (DA_PumpShotgun): the trigger hand on the stock's wrist, which is tall (9 cm), so the hand lies on its
	 * right side with the fingers curled under its lower edge (that edge runs up toward the trigger: the index finger's
	 * way); the support hand under the pump, which SOCKET_Foregrip marks the middle of, as under the rifle's handguard.
	 */
	FGunHands MakeShotgun()
	{
		FGunHands Hands;
		Hands.GripHandle = FVector(0.87f, 0.f, 0.48f);
		Hands.GripPalm = FVector(0.f, -1.f, 0.f);
		Hands.GripForearm = FVector(-0.8f, 0.35f, -0.5f);
		Hands.GripHold = FVector(0.f, 1.f, 0.f);
		// The trigger is 12 cm ahead of the grip: the finger reaches out nearly straight.
		Hands.TriggerCurl[0] = 5.f;
		Hands.TriggerCurl[1] = 20.f;
		Hands.TriggerCurl[2] = 10.f;
		Hands.ForeHandle = FVector(1.f, 0.f, 0.f);
		Hands.ForePalm = FVector(0.f, 0.3f, 1.f);
		Hands.ForeForearm = FVector(-0.45f, -0.5f, -0.75f);
		// The palm on the pump's underside (it's 3 cm deep).
		Hands.ForeHold = FVector(0.f, 0.5f, 2.f);
		Hands.ForeGrip = EHandGrip::Forend;
		// Under the receiver, between the trigger guard and the magazine tube.
		Hands.LoadingPort = FVector(15.5f, 0.f, -3.2f);
		return Hands;
	}

	/**
	 * The Drover (DA_Revolver): the trigger hand round the raked grip (it leans back 17 degrees), palm wrapping a little
	 * round its back; the support hand cupped round the shooting hand from the left, thumb up beside the frame, palm facing
	 * in and forward, closing over the shooting hand's fingers (the support socket is where it presses on the grip).
	 */
	FGunHands MakeRevolver()
	{
		FGunHands Hands;
		Hands.GripHandle = FVector(0.3f, 0.f, 0.95f);
		Hands.GripPalm = FVector(0.2f, -1.f, 0.f);
		Hands.GripTilt = 10.f;
		Hands.GripForearm = FVector(-0.8f, 0.35f, -0.45f);
		Hands.GripHold = FVector(0.f, 0.3f, 0.f);
		Hands.TriggerCurl[0] = 25.f;
		Hands.TriggerCurl[1] = 50.f;
		Hands.TriggerCurl[2] = 25.f;
		Hands.ForeHandle = FVector(0.3f, 0.f, 0.95f);
		Hands.ForePalm = FVector(0.35f, 0.94f, 0.f);
		Hands.ForeForearm = FVector(-0.65f, -0.55f, -0.5f);
		Hands.ForeHold = FVector(0.5f, 0.5f, -1.f);
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

void FirstPersonHandsGrips::OnHandle(EFirstPersonHand Hand, const FVector& Handle, const FVector& Palm, float Tilt, FVector& OutFingers,
	FVector& OutPalm)
{
	// The handle runs the index-and-thumb way across the palm. A hand's knuckles are square to that and to its palm: for a
	// right hand index-way cross palm, for a left hand palm cross index-way (they're mirror images).
	FVector IndexWay = Handle.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	OutPalm = (Palm - IndexWay * FVector::DotProduct(IndexWay, Palm)).GetSafeNormal(UE_SMALL_NUMBER, FVector(0.f, -1.f, 0.f));
	OutFingers = Hand == EFirstPersonHand::Right ? FVector::CrossProduct(IndexWay, OutPalm) : FVector::CrossProduct(OutPalm, IndexWay);
	if (!FMath::IsNearlyZero(Tilt))
	{
		// Turned about the palm's facing: the knuckles tip toward the handle's foot.
		const float Radians = FMath::DegreesToRadians(Tilt);
		OutFingers = (OutFingers * FMath::Cos(Radians) - IndexWay * FMath::Sin(Radians)).GetSafeNormal();
	}
}

FHandPlacement FirstPersonHandsGrips::InGun(const FHandsGun& Gun, const FVector& Hold, const FVector& FingersDir, const FVector& PalmDir,
	const FHandFingers& Fingers, const FVector& Forearm)
{
	return FHandPlacement::Make(Gun.ToCameraPoint(Hold), Gun.ToCameraDir(FingersDir), Gun.ToCameraDir(PalmDir), Fingers,
		Forearm.IsNearlyZero() ? FVector::ZeroVector : Gun.ToCameraDir(Forearm.GetSafeNormal()));
}

FHandPlacement FirstPersonHandsGrips::OnGrip(const FHandsGun& Gun)
{
	const FGunHands& Hands = For(Gun.Kind);
	FVector Fingers;
	FVector Palm;
	OnHandle(EFirstPersonHand::Right, Hands.GripHandle, Hands.GripPalm, Hands.GripTilt, Fingers, Palm);
	FHandFingers Curls = FHandFingers::Of(EHandGrip::Trigger);
	for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
	{
		Curls.Curl[FHandFingers::IndexFinger][Joint] = Hands.TriggerCurl[Joint];
	}
	return InGun(Gun, Gun.Grip + Hands.GripHold * Gun.Scale, Fingers, Palm, Curls, Hands.GripForearm);
}

FHandPlacement FirstPersonHandsGrips::OnForegrip(const FHandsGun& Gun, float PumpBack)
{
	const FGunHands& Hands = For(Gun.Kind);
	FVector Fingers;
	FVector Palm;
	OnHandle(EFirstPersonHand::Left, Hands.ForeHandle, Hands.ForePalm, Hands.ForeTilt, Fingers, Palm);
	// A racked pump slides back along the gun's -X, and the hand round it goes with it.
	const FVector Hold = Gun.Foregrip + (Hands.ForeHold - FVector(PumpBack, 0.f, 0.f)) * Gun.Scale;
	return InGun(Gun, Hold, Fingers, Palm, FHandFingers::Of(Hands.ForeGrip), Hands.ForeForearm);
}

// ---------------------------------------------------------------------------
// FHandPlacement
// ---------------------------------------------------------------------------

FHandPlacement FHandPlacement::Make(const FVector& InHold, const FVector& FingersDir, const FVector& PalmDir, const FHandFingers& InFingers,
	const FVector& InForearm)
{
	FHandPlacement Placement;
	Placement.Hold = InHold;
	Placement.Turn = FirstPersonHands::Frame(FingersDir, PalmDir);
	Placement.Fingers = InFingers;
	Placement.Forearm = InForearm.GetSafeNormal();
	return Placement;
}

FHandPlacement FHandPlacement::Blend(const FHandPlacement& From, const FHandPlacement& To, float Alpha)
{
	const float Share = FMath::Clamp(Alpha, 0.f, 1.f);
	FHandPlacement Placement;
	Placement.Hold = FMath::Lerp(From.Hold, To.Hold, Share);
	Placement.Turn = FQuat::Slerp(From.Turn, To.Turn, Share).GetNormalized();
	Placement.Fingers = FHandFingers::Blend(From.Fingers, To.Fingers, Share);
	// A way only one end has is kept whole (the other's "the hand's usual way" is only known to the rig).
	if (From.Forearm.IsNearlyZero() || To.Forearm.IsNearlyZero())
	{
		Placement.Forearm = Share < 0.5f && !From.Forearm.IsNearlyZero() ? From.Forearm : To.Forearm;
		if (Placement.Forearm.IsNearlyZero())
		{
			Placement.Forearm = From.Forearm;
		}
	}
	else
	{
		Placement.Forearm = FMath::Lerp(From.Forearm, To.Forearm, Share).GetSafeNormal(UE_SMALL_NUMBER, To.Forearm);
	}
	return Placement;
}

FHandPlacement FHandPlacement::Moved(const FTransform& Space) const
{
	FHandPlacement Placement = *this;
	Placement.Hold = Space.TransformPosition(Hold);
	Placement.Turn = (Space.GetRotation() * Turn).GetNormalized();
	Placement.Forearm = Space.GetRotation().RotateVector(Forearm);
	return Placement;
}
