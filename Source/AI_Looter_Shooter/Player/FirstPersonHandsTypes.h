#pragma once

#include "CoreMinimal.h"
#include "Math/RotationMatrix.h"
#include "Weapons/WeaponTypes.h"

// What the first-person hands' files share (UFirstPersonHandsComponent, FFirstPersonHandsRig, FirstPersonHandsGrips,
// FirstPersonHandsMotion): which hand, how its fingers close, where it goes, and the gun in hand as the hands see it.

/** Which hand: the left (the support hand: foregrip, reloads, the grenade) or the right (the trigger hand). */
enum class EFirstPersonHand : uint8
{
	Left,
	Right
};

/** What the support hand is doing, so a job cut short (a strike stops a reload) eases over instead of snapping. */
enum class EHandsSupportJob : uint8
{
	Foregrip,
	Reload,
	Throw
};

/** How a hand's fingers close round what it holds (FHandFingers::Of has each one's curls). */
enum class EHandGrip : uint8
{
	/** Flat, fingers out: a palm on the ejector rod, batting a cylinder shut, letting go after a throw. */
	Open,
	/** Round a gun's grip, the index finger reaching for the trigger (the gun kind sets that finger: FGunHands). */
	Trigger,
	/** Cupping a foregrip or a pump from below. */
	Forend,
	/** Round something thick: a magazine, the grenade jar. */
	Wrap,
	/** Thumb and index together (a shell, a charging handle), the rest curled. */
	Pinch,
	/** A six-gun's support hand, closed round the shooting hand's fingers. */
	Support
};

namespace FirstPersonHands
{
	/** A hand's slot in the hands' two-element arrays. */
	inline int32 Index(EFirstPersonHand Hand)
	{
		return Hand == EFirstPersonHand::Left ? 0 : 1;
	}

	/** The frame with X along XAxis and Y as near YAxis as square to it (Z = X cross Y). */
	inline FQuat Frame(const FVector& XAxis, const FVector& YAxis)
	{
		return FRotationMatrix::MakeFromXY(XAxis, YAxis).ToQuat();
	}
}

/** Each finger's three joints, bent toward the palm (degrees): thumb, index, middle, ring, pinky; from the knuckle out. */
struct AI_LOOTER_SHOOTER_API FHandFingers
{
	static constexpr int32 Count = 5;
	static constexpr int32 Joints = 3;
	static constexpr int32 Thumb = 0;
	static constexpr int32 IndexFinger = 1;

	float Curl[Count][Joints] = {};

	/** A grip's curls (FirstPersonHandsFingers.cpp). */
	static FHandFingers Of(EHandGrip Grip);

	static FHandFingers Blend(const FHandFingers& From, const FHandFingers& To, float Alpha);
};

/**
 * Where one hand goes, in whatever space its maker works in: the middle of what it holds (the hand's grip point, the
 * mannequin's HandGrip socket), how it's turned (its frame: X from the wrist to the knuckles, Y out of the palm toward
 * what it holds) and how its fingers close.
 */
struct AI_LOOTER_SHOOTER_API FHandPlacement
{
	FVector Hold = FVector::ZeroVector;
	FQuat Turn = FQuat::Identity;
	FHandFingers Fingers;

	static FHandPlacement Make(const FVector& InHold, const FVector& FingersDir, const FVector& PalmDir, const FHandFingers& InFingers);

	/** Alpha of the way from From to To: the grip point along a line, the turn on the shortest arc, the fingers by their curls. */
	static FHandPlacement Blend(const FHandPlacement& From, const FHandPlacement& To, float Alpha);

	/** The same placement seen from another space: Space takes this one's space into it. */
	FHandPlacement Moved(const FTransform& Space) const;

	FVector GetFingersDir() const { return Turn.GetAxisX(); }
	FVector GetPalmDir() const { return Turn.GetAxisY(); }
};

/**
 * The gun in hand as the hands see it: its visible model's own space (origin at the back of the receiver, X to the
 * muzzle, Y right, Z up; cm) in the first-person camera's space (X ahead, Y right, Z up), so a strike's or a throw's
 * pose on the model moves the hands with it, and the points on it the hands work. UFirstPersonHandsComponent fills it
 * each frame, after the gun is placed; the tests make their own.
 */
struct AI_LOOTER_SHOOTER_API FHandsGun
{
	FTransform ToCamera = FTransform::Identity;
	EWeaponKind Kind = EWeaponKind::None;
	EWeaponReloadPart ReloadPart = EWeaponReloadPart::None;
	/** 0..1 through a reload, or -1 when it isn't reloading (AWeaponBase::GetReloadProgress). */
	float ReloadProgress = -1.f;

	/** SOCKET_Grip and SOCKET_Foregrip, in the gun's space. */
	FVector Grip = FVector::ZeroVector;
	FVector Foregrip = FVector::ZeroVector;

	/** Where the magazine seats (the body's SOCKET_Magazine), when a reload needs it. */
	FVector MagazineSeat = FVector::ZeroVector;
	bool bHasMagazineSeat = false;

	/** A revolver's cylinder as the model has it now (swung out or home): the middle of its rear face and its axis forward. */
	FVector CylinderRear = FVector::ZeroVector;
	FVector CylinderAxis = FVector::ForwardVector;
	bool bHasCylinder = false;

	FVector ToCameraPoint(const FVector& InGun) const { return ToCamera.TransformPosition(InGun); }
	FVector ToCameraDir(const FVector& InGun) const { return ToCamera.TransformVectorNoScale(InGun); }
};
