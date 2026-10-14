#pragma once

#include "CoreMinimal.h"
#include "Player/FirstPersonHandsTypes.h"

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

	/** The trigger hand round the grip, index finger at the trigger (camera space). */
	AI_LOOTER_SHOOTER_API FHandPlacement OnGrip(const FHandsGun& Gun);

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
}
