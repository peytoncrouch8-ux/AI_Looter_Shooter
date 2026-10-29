#include "Player/FirstPersonHandsGrips.h"
#include "Player/FirstPersonHandsFraming.h"
#include "Player/FirstPersonHandsMarks.h"
#include "HAL/IConsoleManager.h"

// The support hand's cup (the user, 2026-10-27: "The hand needs to cup beneath the trigger housing thingy on the AR and on
// the magazine of the LMG, the left hand does not need to be towards the front of the gun. Keep this in mind for all
// weapons."): palm up under each gun's middle, fingers curled up round its far side, the thumb along its near side, the
// forearm up from under the frame. Spots read off the models (Art/Models/Weapons/<Gun>.py: u, v, w are X, Z and -Y here),
// from each gun's SOCKET_Grip, cm of the modeled gun (the game draws it bigger and scales these with it). These are each
// kind's spot along the gun and its fallback: how high and wide the underside is comes from the gun's own parts when they
// say (FitCup, FirstPersonHandsCupFit.cpp: another body or magazine than the one read here). Tuned live with
// the support hand's Looter.Hands.Support* console variables (they move the cup too) and Looter.FPHands.<Kind>.Support*.

namespace
{
	TAutoConsoleVariable<int32> CVarCup(TEXT("Looter.Hands.Cup"), 1,
		TEXT("1: the first-person support hand rests cupped under the gun's middle (the trigger housing, the magazine). 0: on the fore-end, as before."));
	TAutoConsoleVariable<float> CVarCupWristBend(TEXT("Looter.Hands.CupWristBend"), 50.f,
		TEXT("Degrees the cupping support hand's wrist bends back (palm up, the forearm rising from below behind the knuckles)."));
	TAutoConsoleVariable<float> CVarPumpReach(TEXT("Looter.Hands.PumpReachSeconds"), 0.08f,
		TEXT("Seconds a pump gun's support hand takes from its cup to the pump (to rack it or reload) and back."));

	/** A box under the hand: its middle Offset from the grip, its long way along the gun, half its height and width, rounding. */
	FHandleSpec CupBox(const FVector& Offset, float HalfHeight, float HalfWidth, float Corner, float HalfLength)
	{
		FHandleSpec Spec;
		Spec.Offset = Offset;
		Spec.Axis = FVector::ForwardVector;
		Spec.Across = FVector::UpVector;
		Spec.HalfAcross = HalfHeight;
		Spec.HalfThrough = HalfWidth;
		Spec.Corner = Corner;
		Spec.HalfLength = HalfLength;
		Spec.bFromGrip = true;
		Spec.bValid = true;
		return Spec;
	}

	/**
	 * The hand's grip point for a cup under a box whose underside is Bottom cm under the grip socket's level (relative Z):
	 * the palm's skin on the underside, the grip point FirstPersonHands::GripOffPalmSkin inside it (the palm is seated on it).
	 */
	float OverBottom(float Bottom)
	{
		return Bottom + FirstPersonHands::GripOffPalmSkin;
	}
}

bool FirstPersonHandsGrips::UseCup()
{
	return CVarCup.GetValueOnAnyThread() != 0;
}

FHandPlacement FirstPersonHandsGrips::OnForegrip(const FHandsGun& Gun, float PumpBack)
{
	// At rest cupped under the gun's middle; a pump gun's hand on the pump as far as it's gone there. The spots the gun's
	// parts mark come first (Hold_L at rest, Rack_L the pump: FirstPersonHandsMarks.cpp), the kind's tables for the rest.
	const FGunHands& Hands = For(Gun.Kind);
	FHandPlacement Rest;
	const bool bMarkedRest = FirstPersonHandsMarks::SupportRest(Gun, Rest);
	FHandPlacement Rack;
	// The pump's mark rests forward on the gun (WeaponModelHandMarks.cpp): racked, it goes back with the pump.
	const bool bMarkedRack = FirstPersonHandsMarks::Place(Gun, EHandMarkRole::Rack, EFirstPersonHand::Left, Rack, FVector(-PumpBack, 0.f, 0.f));
	if (!bMarkedRest && (!Hands.bCup || !UseCup()))
	{
		return bMarkedRack ? Rack : OnForeEnd(Gun, PumpBack);
	}
	if (!bMarkedRest)
	{
		Rest = OnCup(Gun);
	}
	const float Pump = bMarkedRack || Hands.bPumpFromCup ? FMath::Clamp(Gun.PumpShare, 0.f, 1.f) : 0.f;
	if (Pump <= 0.f)
	{
		return Rest;
	}
	return FHandPlacement::Blend(Rest, bMarkedRack ? Rack : OnForeEnd(Gun, PumpBack), FMath::SmoothStep(0.f, 1.f, Pump));
}

float FirstPersonHandsGrips::NextPumpShare(const FHandsGun& Gun, float Current, float DeltaTime)
{
	// A pump marked on the parts (Rack_L) or the kind's table says the hand leaves its rest hold for the pump.
	const FGunHands& Hands = For(Gun.Kind);
	if (!Hands.bPumpFromCup && !FirstPersonHandsMarks::Find(Gun, EHandMarkRole::Rack, EFirstPersonHand::Left))
	{
		return 0.f;
	}
	// On the pump for its rack between shots and the whole reload (the shells go in from the port and the pump racks last).
	const bool bWanted = Gun.PumpCycle >= 0.f || Gun.ReloadProgress >= 0.f;
	const float Seconds = FMath::Max(CVarPumpReach.GetValueOnAnyThread(), 0.01f);
	return FMath::FInterpConstantTo(Current, bWanted ? 1.f : 0.f, FMath::Max(DeltaTime, 0.f), 1.f / Seconds);
}

void FirstPersonHandsGrips::AddCup(EWeaponKind Kind, FGunHands& Hands)
{
	Hands.bCup = true;
	switch (Kind)
	{
	case EWeaponKind::Revolver:
		// One-handed: the support hand stays parked (FPHandsFraming's OneHand), and on the shooting hand for its work.
		Hands.bCup = false;
		break;
	case EWeaponKind::Shotgun:
		// Ranchhand: under the receiver's bare floor just ahead of the trigger group, grip at (-4.5, -6). The group's housing
		// and guard run u -0.6 to 8 (moved 5 cm back on 2026-10-28; on 10-27, when this was set, they ran 3 to 13 and the box
		// sat under them); the floor is v -3 from there to u 20, the loading port at 15.5, -3.2. The box: its middle u 9, 8 long
		// (its back 3 cm now over the housing's front), its floor v -3.5 (the cup fit keeps a table within 1 cm of the faces
		// it finds). The pump only to rack it and to reload (bPumpFromCup).
		// (The hold 3 cm further ahead than the box's middle: the framing's -3 slide, kept for the pump, takes it back.)
		Hands.CupShape = CupBox(FVector(13.5f, 0.f, 5.f), 2.5f, 2.4f, 1.f, 4.f);
		Hands.CupHold = FVector(16.5f, 0.3f, OverBottom(2.5f));
		Hands.bPumpFromCup = true;
		break;
	case EWeaponKind::SMG:
		// Rustler: under the helix's back end, just ahead of the grip (its axis v -5.7, 1.2 to the left, radius 2.75, from u 15),
		// grip at (5.4, -8.6): a round tube.
		Hands.CupShape = CupBox(FVector(12.6f, -1.2f, 2.9f), 2.75f, 2.75f, 2.75f, 3.f);
		Hands.CupHold = FVector(12.6f, -0.9f, OverBottom(0.15f));
		break;
	case EWeaponKind::Sniper:
		// Longhorn: under the box magazine ahead of the grip (u 33.6-37.6, its floor at v -7.2, 3.8 wide), grip at (19.8, -9).
		Hands.CupShape = CupBox(FVector(15.8f, 0.f, 2.95f), 1.15f, 1.9f, 0.5f, 2.f);
		Hands.CupHold = FVector(15.8f, 0.3f, OverBottom(1.8f));
		break;
	case EWeaponKind::LMG:
		// Thresher: under the belt box (u 15.4-30.6, 6.8 wide, 2.4 left to 4.4 right of the bore: its middle 1 to the left; its
		// floor at v -14.6), grip at (5.8, -9.6).
		Hands.CupShape = CupBox(FVector(17.2f, -1.f, -0.25f), 4.75f, 3.4f, 0.6f, 7.6f);
		Hands.CupHold = FVector(17.2f, -0.6f, OverBottom(-5.f));
		break;
	case EWeaponKind::Launcher:
		// Powderkeg: under the tube (radius 4.2 on the bore, its sand sleeves 4.8) ahead of the pistol grip at (48.6, -10).
		Hands.CupShape = CupBox(FVector(9.4f, 0.f, 10.f), 4.5f, 4.5f, 4.5f, 5.f);
		Hands.CupHold = FVector(9.4f, 0.3f, OverBottom(5.5f));
		break;
	case EWeaponKind::Rifle:
	case EWeaponKind::None:
	default:
		// Bullpup: beneath the trigger housing, the thumbhole loop's front-bottom (the loop's floor at v -15 from u 25 to 34,
		// 3.6 wide), just ahead of and under the trigger hand on the loop's rear post, grip at (22.5, -10).
		Hands.CupShape = CupBox(FVector(11.f, 0.f, -3.f), 2.f, 1.8f, 1.f, 3.f);
		Hands.CupHold = FVector(11.5f, 0.3f, OverBottom(-5.f));
		break;
	}
}

FHandPlacement FirstPersonHandsGrips::OnCup(const FHandsGun& Gun)
{
	// The rest hold the gun's parts mark (Hold_L), when they do.
	FHandPlacement Marked;
	if (FirstPersonHandsMarks::SupportRest(Gun, Marked))
	{
		return Marked;
	}
	FGunHands Hands = Tuned(Gun.Kind);
	if (!Hands.bCup)
	{
		return OnForeEnd(Gun);
	}
	// On the underside found on this gun's own parts (another body or magazine than the table's: FirstPersonHandsCupFit.cpp).
	FitCupTo(Hands, Gun.Cup);
	// The hand first, as a real cupping hold has it: the palm up onto the underside, the knuckles forward along the gun (a
	// little to its right, so the fingers curl up its far side), the thumb along its near side.
	const FVector Palm = Hands.CupPalm.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const FVector Fingers = (Hands.CupKnuckles - Palm * FVector::DotProduct(Hands.CupKnuckles, Palm)).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	// Then the forearm fitted to it: straight back behind the knuckles, the wrist bent back (toward the back of the hand)
	// and no other way, so it rises from under the frame in line with the hand (no sideways bend, no roll).
	const float Bend = FMath::DegreesToRadians(FMath::Clamp(CVarCupWristBend.GetValueOnAnyThread(), 0.f, 75.f));
	const FVector Forearm = -Fingers * FMath::Cos(Bend) - Palm * FMath::Sin(Bend);
	FHandFingers Curls = FHandFingers::Of(EHandGrip::Forend);
	Curls.Curl[FHandFingers::Thumb][0] = Hands.CupThumbSwing;
	Curls.Spread[FHandFingers::Thumb] = Hands.CupThumbSpread;
	// The framing's support slides (Looter.FPHands.<Kind>.Support*) move the cup as they moved the fore-end hold.
	const FVector Hold = Gun.Grip + (Hands.CupHold + FPHandsFraming::SupportNudge(Gun.Kind)) * Gun.Scale;
	FHandPlacement Cup = InGun(Gun, Hold, Fingers, Palm, Curls, Forearm);
	Cup.Contact = ContactOnGun(Gun, Gun.Grip, Hands.CupShape, 1.f, FVector::ZeroVector, 0.f);
	return Cup;
}
