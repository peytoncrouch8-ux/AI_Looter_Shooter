#pragma once

#include "CoreMinimal.h"
#include "Player/FirstPersonHandsTypes.h"

class UWeaponModelComponent;

/**
 * A handle's solid on a gun (FHandleShape in the modeled gun's space): its middle Offset from the hand's socket
 * (SOCKET_Foregrip, or SOCKET_Grip with bFromGrip: a six-gun's support hand closes round the shooting hand there), its
 * long way, the way its HalfAcross runs, its half width, the edges' rounding, cm of the modeled gun.
 */
struct FHandleSpec
{
	FVector Offset = FVector::ZeroVector;
	FVector Axis = FVector::UpVector;
	FVector Across = FVector::ForwardVector;
	float HalfAcross = 2.f;
	float HalfThrough = 1.7f;
	float Corner = 0.9f;
	float HalfLength = 0.f;
	bool bFromGrip = false;
	bool bValid = false;
};

/**
 * One gun kind's hold, in the modeled gun's own space (X to the muzzle, Y right, Z up; cm; the gun is drawn bigger,
 * FHandsGun::Scale, and these offsets grow with it), read off the models in Art/Models/Weapons/<Gun>.py and the approved
 * look sheet's poses (Tools/Blender/fp_arms_sheet.py). A hand is put on a handle: the way its index and thumb lie along it,
 * the way its palm faces, a tilt of the handle across the palm, where its forearm leaves the wrist, and its grip point's
 * offset from the gun's socket.
 */
struct FGunHands
{
	/** The trigger hand on SOCKET_Grip. */
	FVector GripHandle = FVector::UpVector;
	FVector GripPalm = FVector(0.f, -1.f, 0.f);
	/** Degrees the knuckles tip down toward the handle's foot (the handle then crosses the palm on a slant; less: up). */
	float GripTilt = 0.f;
	/** A last turn of the hand in its own frame (roll about the knuckles' line, pitch about the palm's facing, yaw about the third). */
	FRotator GripTurn = FRotator::ZeroRotator;
	FVector GripForearm = FVector(-0.8f, 0.35f, -0.45f);
	FVector GripHold = FVector::ZeroVector;
	/** The trigger finger's three joints (degrees): how far this gun's trigger sits from its grip sets how bent it is. */
	float TriggerCurl[FHandFingers::Joints] = { 30.f, 50.f, 25.f };

	/** The support hand on SOCKET_Foregrip. */
	FVector ForeHandle = FVector::ForwardVector;
	FVector ForePalm = FVector(0.f, 0.3f, 1.f);
	float ForeTilt = 0.f;
	FRotator ForeTurn = FRotator::ZeroRotator;
	FVector ForeForearm = FVector(-0.45f, -0.5f, -0.75f);
	FVector ForeHold = FVector::ZeroVector;
	EHandGrip ForeGrip = EHandGrip::Forend;

	/** A rifle's charging handle (the bolt's yank after a fresh magazine). */
	FVector Handle = FVector::ZeroVector;
	/** A pump gun's loading port, under the receiver, where shells are thumbed in. */
	FVector LoadingPort = FVector::ZeroVector;
	/** Where on a magazine the hand takes it, from its seat (it slides out along the gun's -Z). */
	FVector MagazineGrab = FVector::ZeroVector;

	/**
	 * A bolt gun's bolt (the sniper's): the point on the bore it turns about (its part's origin, the body's SOCKET_Bolt) and
	 * its knob at rest, which the trigger hand takes to work it (Handle is the knob for such a gun).
	 */
	FVector BoltPivot = FVector::ZeroVector;

	/** The launcher's next rocket: where the support hand holds it, ahead of its seat (it goes in tail first, nose out). */
	FVector RocketGrab = FVector::ZeroVector;

	// How the hands touch the gun (FPGRIP, 2026-10-22; FirstPersonHandsGripShapes.cpp fills these from the models).

	/** The trigger hand's handle (the pistol grip) and the support hand's (handguard, pump, front grip, helix). */
	FHandleSpec GripShape;
	FHandleSpec ForeShape;
	/** On the trigger's face, where the trigger finger's middle line rests (the modeled gun's own space; zero: no search). */
	FVector TriggerAt = FVector::ZeroVector;
	/** The trigger finger off the trigger: nearly straight along the frame above the guard, lifted toward the thumb. */
	float IndexedCurl[FHandFingers::Joints] = { 4.f, 6.f, 4.f };
	float IndexedSpread = 6.f;
	/**
	 * The thumbs' base swing in across the palm and spread toward the fingers' way (degrees; their outer joints close by
	 * touch): on the grip swung round its back onto its far side (68: tuned in PIE on the rifle, revolver, SMG, sniper,
	 * LMG, 2026-10-22), on the foregrip laid along the handguard toward the muzzle.
	 */
	float GripThumbSwing = 68.f;
	float GripThumbSpread = 0.f;
	float ForeThumbSwing = 10.f;
	float ForeThumbSpread = 0.f;
	/** 0-1: how much the support hand's fingers close by touch (the rest: its grip's own curls). */
	float ForeContact = 1.f;

	/**
	 * The support hand's rest hold since 2026-10-27 (the user: "the hand needs to cup beneath the trigger housing ... on the
	 * magazine of the LMG, the left hand does not need to be towards the front of the gun. Keep this in mind for all
	 * weapons"): palm up under the gun's middle, the fingers curled up round its far side, the thumb along its near side,
	 * the forearm up from under the frame. From SOCKET_Grip (cm of the modeled gun, drawn to scale with it): CupHold the
	 * hand's grip point, CupShape what it closes on (the trigger housing, the magazine or the box). bCup off (the revolver,
	 * one-handed; or Looter.Hands.Cup 0): the fore-end hold above. The reloads start and end on it (OnForegrip gives it).
	 */
	bool bCup = false;
	FVector CupHold = FVector::ZeroVector;
	/**
	 * The cup's frame (the gun's space): the knuckles' way (forward along the gun, a little to its right, where the fingers
	 * curl up its far side) and the palm's (up onto the underside). The forearm is fitted to them, not the other way round
	 * (the user, 2026-10-27: "the hand is twisted to the right, straighten it out"): straight behind the knuckles, the wrist
	 * bent back only (Looter.Hands.CupWristBend, no sideways bend or roll), so it rises from the frame's bottom left.
	 */
	FVector CupKnuckles = FVector(0.8f, 0.5f, 0.15f);
	FVector CupPalm = FVector(0.f, 0.15f, 1.f);
	FHandleSpec CupShape;
	float CupThumbSwing = 0.f;
	float CupThumbSpread = -20.f;
	/** A pump gun: the hand leaves the cup for the pump to rack it or reload (FHandsGun::PumpShare of the way). */
	bool bPumpFromCup = false;
};

/** The hands on the gun: the trigger hand on its grip, the support hand on its foregrip (FirstPersonHandsGrips.cpp). */
namespace FirstPersonHandsGrips
{
	/** The hold for a gun kind (None: the rifle's). */
	AI_LOOTER_SHOOTER_API const FGunHands& For(EWeaponKind Kind);

	/**
	 * The hold as the hands use it: For's, with (development builds only) the Looter.Hands.* tuning console variables on
	 * it for the kind Looter.Hands.Kind picks (0 every gun, 1 rifle, 2 shotgun, 3 revolver): Trigger/Support PalmRoll,
	 * PalmPitch, PalmYaw (degrees, the hand's own turn), ForearmYaw, ForearmPitch (degrees, the forearm's way swung in the
	 * gun's frame), HoldAhead, HoldRight, HoldUp (cm of the modeled gun). Looter.Hands.Dump prints the numbers to bake.
	 */
	AI_LOOTER_SHOOTER_API FGunHands Tuned(EWeaponKind Kind);

	/**
	 * The trigger hand round the grip, index finger at the trigger (camera space); a bolt gun's goes to the bolt's knob and
	 * rides it as the bolt is worked (Gun.BoltHand of the way there, the knob where Gun.BoltOpen has it).
	 */
	AI_LOOTER_SHOOTER_API FHandPlacement OnGrip(const FHandsGun& Gun);

	/** The trigger hand pinching a bolt gun's knob, the bolt Open (0-1) of the way (camera space). */
	AI_LOOTER_SHOOTER_API FHandPlacement OnBoltKnob(const FHandsGun& Gun, float Open);

	/** Where a bolt gun's knob is with the bolt Open (0-1) of the way, in the modeled gun's space (LooterReload::BoltPose). */
	AI_LOOTER_SHOOTER_API FVector BoltKnobAt(const FGunHands& Hands, float Open);

	/**
	 * The support hand on the foregrip (camera space): under a rifle's handguard, round a shotgun's pump (PumpBack: how
	 * far the pump is racked, cm of the modeled gun), or round a six-gun's shooting hand.
	 */
	AI_LOOTER_SHOOTER_API FHandPlacement OnForegrip(const FHandsGun& Gun, float PumpBack = 0.f);

	/** The support hand's cup under the gun's middle (FGunHands::CupHold; camera space); the fore-end hold for a kind without one. */
	AI_LOOTER_SHOOTER_API FHandPlacement OnCup(const FHandsGun& Gun);

	/** The fore-end (or pump) hold itself, whatever the cup (the pump gun's rack, the tests). */
	AI_LOOTER_SHOOTER_API FHandPlacement OnForeEnd(const FHandsGun& Gun, float PumpBack = 0.f);

	/** Looter.Hands.Cup (1): the support hand rests cupped under the gun's middle (0: on the fore-end, as before 2026-10-27). */
	AI_LOOTER_SHOOTER_API bool UseCup();

	/**
	 * A pump gun's support hand from its cup toward the pump: Current eased toward the pump while it's racked or reloaded
	 * (Gun.PumpCycle or Gun.ReloadProgress under way), back to the cup after, over Looter.Hands.PumpReachSeconds (0.08).
	 */
	AI_LOOTER_SHOOTER_API float NextPumpShare(const FHandsGun& Gun, float Current, float DeltaTime);

	/** A placement given in the gun's own space, in camera space. */
	AI_LOOTER_SHOOTER_API FHandPlacement InGun(const FHandsGun& Gun, const FVector& Hold, const FVector& FingersDir, const FVector& PalmDir,
		const FHandFingers& Fingers, const FVector& Forearm);

	/**
	 * A hand on a handle (gun space): its index-and-thumb side along Handle, its palm facing Palm (made square to the
	 * handle), the handle slanted across the palm by Tilt degrees. Gives the knuckles' way and the palm's.
	 */
	AI_LOOTER_SHOOTER_API void OnHandle(EFirstPersonHand Hand, const FVector& Handle, const FVector& Palm, float Tilt, FVector& OutFingers,
		FVector& OutPalm);

	/** A kind's handles, trigger and thumbs, read off its model (FirstPersonHandsGripShapes.cpp; For's tables get them). */
	AI_LOOTER_SHOOTER_API void AddContact(EWeaponKind Kind, FGunHands& Hands);

	/** A kind's cup under the gun's middle (FirstPersonHandsCup.cpp; For's tables get it). */
	AI_LOOTER_SHOOTER_API void AddCup(EWeaponKind Kind, FGunHands& Hands);

	/**
	 * The cup found on the gun's own parts (FirstPersonHandsCupFit.cpp): rays up from under the gun at the kind's cup spot
	 * through every part's collision mesh find its underside there (any body or magazine, scripted or Tripo), rays in from
	 * each side its width. Not fitted (the table holds) for a kind without a cup, a gun not built from parts, or nothing
	 * within reach of the table's spot. Run on the gun at rest (not mid reload or rack: the parts move).
	 */
	AI_LOOTER_SHOOTER_API FHandsCupFit FitCup(const UWeaponModelComponent& Model, EWeaponKind Kind);

	/** The kind's cup moved onto a fit (Looter.Hands.CupFit 1): its box's bottom on the underside, the hand with it. */
	AI_LOOTER_SHOOTER_API void FitCupTo(FGunHands& Hands, const FHandsCupFit& Fit);

	/** The fit's tuning now (Looter.Hands.CupFit.MinWidth), so a fit kept for a gun is worked out again when it changes. */
	AI_LOOTER_SHOOTER_API uint32 CupFitSettings();

	/**
	 * The touch tuning on a kind's hold (development builds; Looter.Hands.Kind picks the kind as for the rest):
	 * Looter.Hands.GripShape* / ForeShape* (Ahead, Right, Up, HalfAcross, HalfThrough, Corner: cm of the modeled gun),
	 * TriggerAtAhead / TriggerAtUp, TriggerThumbSwing / TriggerThumbSpread, SupportThumbSwing / SupportThumbSpread,
	 * IndexedSpread (degrees). Looter.Hands.ContactDump prints the numbers to bake.
	 */
	AI_LOOTER_SHOOTER_API void TuneContact(EWeaponKind Kind, FGunHands& Hands);

	/**
	 * A hand's contact on the gun (camera space): Spec's handle on Socket (the modeled gun's: Gun.Grip or Gun.Foregrip,
	 * already drawn to scale; Slide moves the handle with a racked pump, cm of the modeled gun), Share of the fingers closing
	 * on it, the palm seated, and with a trigger point (TriggerAt non-zero, bTrigger) the trigger finger on it TriggerShare of
	 * the way. Looter.Hands.Contact, .Seat, .SeatGap, .FingerRadius set the shares and sizes.
	 */
	AI_LOOTER_SHOOTER_API FHandContact ContactOnGun(const FHandsGun& Gun, const FVector& Socket, const FHandleSpec& Spec, float Share,
		const FVector& TriggerAt, float TriggerShare, const FVector& Slide = FVector::ZeroVector);

	/** Looter.Hands.TriggerDiscipline (default 1): the trigger finger lies along the frame when it isn't wanted on the trigger. */
	AI_LOOTER_SHOOTER_API bool UseTriggerDiscipline();

	/** Seconds the trigger finger stays on the trigger after the last pull (Looter.Hands.TriggerHoldSeconds). */
	AI_LOOTER_SHOOTER_API float TriggerHoldSeconds();
}
