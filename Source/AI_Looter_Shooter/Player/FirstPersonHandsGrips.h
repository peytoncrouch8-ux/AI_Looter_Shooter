#pragma once

#include "CoreMinimal.h"
#include "Player/FirstPersonHandsTypes.h"

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
