#pragma once

#include "CoreMinimal.h"
#include "Player/FirstPersonHandsTypes.h"

/**
 * One gun kind's hold, in the gun's own space (X to the muzzle, Y right, Z up; cm), read off the models in
 * Art/Models/Weapons/<Gun>.py: where each hand sits on the gun, which way its palm and fingers go, and the spots a
 * reload works. The sockets carry positions only, so the turns live here (no gun is re-exported for them).
 */
struct FGunHands
{
	/** The trigger hand on SOCKET_Grip: up the grip (pinky to index), out of its palm into the grip, its grip point off the socket. */
	FVector GripIndexUp = FVector::UpVector;
	FVector GripPalm = FVector(0.f, -1.f, 0.f);
	FVector GripHold = FVector::ZeroVector;
	/** The trigger finger's three joints (degrees): how far this gun's trigger sits from its grip sets how bent it is. */
	float TriggerCurl[FHandFingers::Joints] = { 30.f, 50.f, 25.f };

	/** The support hand on SOCKET_Foregrip: wrist to knuckles, out of its palm, its grip point off the socket, its fingers. */
	FVector ForeFingers = FVector(0.5f, 0.85f, 0.1f);
	FVector ForePalm = FVector(0.f, 0.25f, 1.f);
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

	/** The trigger hand round the grip, index finger at the trigger (camera space). */
	AI_LOOTER_SHOOTER_API FHandPlacement OnGrip(const FHandsGun& Gun);

	/**
	 * The support hand on the foregrip (camera space): under a rifle's handguard, round a shotgun's pump (PumpBack: how
	 * far the pump is racked, cm), or round a six-gun's shooting hand.
	 */
	AI_LOOTER_SHOOTER_API FHandPlacement OnForegrip(const FHandsGun& Gun, float PumpBack = 0.f);

	/** A placement given in the gun's own space, in camera space. */
	AI_LOOTER_SHOOTER_API FHandPlacement InGun(const FHandsGun& Gun, const FVector& Hold, const FVector& FingersDir, const FVector& PalmDir,
		const FHandFingers& Fingers);
}
