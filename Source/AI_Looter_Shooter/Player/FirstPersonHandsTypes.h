#pragma once

#include "CoreMinimal.h"
#include "Math/RotationMatrix.h"
#include "Weapons/ReloadTuning.h"
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
	Throw,
	/** An ember power's cast (FirstPersonHandsMotion::SupportInCast). */
	Cast
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
	Support,
	/** Wrapped round a shotgun's pump: tighter than a foregrip's cup, the thumb laid along its near side. */
	Pump
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

	/**
	 * The hand's grip point (the middle of a 4 cm handle in the closed fist) off its palm, and past the palm's middle toward
	 * the knuckles (cm, SK_FP_Arms's: Intermediate/FPArms/FPArms_rig.json's GripPoint). Hand sizes, never the gun's.
	 */
	inline constexpr float GripOffPalm = 3.6f;
	inline constexpr float GripPastPalm = 2.2f;
	/**
	 * The grip point off the palm's skin (cm): GripOffPalm is measured from the hand's bones, inside the hand; the glove's
	 * palm lies about 1.6 nearer, on the 4 cm handle's surface (its radius). A palm is seated on a handle by this point.
	 */
	inline constexpr float GripOffPalmSkin = 2.0f;
}

/**
 * Each finger's three joints bent toward the palm from straight (degrees): thumb, index, middle, ring, pinky; from the
 * knuckle out. Absolute: the rig takes off the bend the mesh is made with (SK_FP_Arms rests a little curled).
 * Spread turns a finger's first joint about the palm's facing (degrees): a finger toward the index side (an indexed
 * trigger finger lifted along the frame), the thumb toward the fingers' way (laid along a handguard) or, negative, out.
 */
struct AI_LOOTER_SHOOTER_API FHandFingers
{
	static constexpr int32 Count = 5;
	static constexpr int32 Joints = 3;
	static constexpr int32 Thumb = 0;
	static constexpr int32 IndexFinger = 1;

	float Curl[Count][Joints] = {};
	float Spread[Count] = {};

	/** A grip's curls (FirstPersonHandsFingers.cpp). */
	static FHandFingers Of(EHandGrip Grip);

	static FHandFingers Blend(const FHandFingers& From, const FHandFingers& To, float Alpha);
};

/**
 * A handle's solid as a hand closes on it (FPGRIP, 2026-10-22; the user: "the fingers and hand need to grip the gun
 * better"): a box with rounded edges running along Axis through Center, HalfAcross along Across (square to Axis: a pistol
 * grip's front to back, a handguard's height), HalfThrough along the third way (its width), the edges rounded by Corner
 * (a corner as big as both halves: a round tube), endless along Axis unless HalfLength. Read off the gun models
 * (Art/Models/Weapons/<Gun>.py); the hands' fingers close till they touch it, the palm is seated on it.
 */
struct AI_LOOTER_SHOOTER_API FHandleShape
{
	FVector Center = FVector::ZeroVector;
	FVector Axis = FVector::UpVector;
	FVector Across = FVector::ForwardVector;
	float HalfAcross = 2.f;
	float HalfThrough = 2.f;
	float Corner = 1.f;
	float HalfLength = 0.f;
	bool bValid = false;

	/** Signed distance from the surface (cm): negative inside. An invalid shape is far from everything. */
	float Distance(const FVector& Point) const;

	/** The same shape seen from another space (sizes scaled with it). */
	FHandleShape Moved(const FTransform& Space) const;

	/** Alpha of the way from From to To (one without a shape gives way to the one with). */
	static FHandleShape Blend(const FHandleShape& From, const FHandleShape& To, float Alpha);
};

/**
 * How a hand touches what it holds (FirstPersonHandsContact.cpp; the rig does it, after placing the hand): Share of the
 * way its fingers close till they touch Handle instead of the placement's own curls; Seat of the way its palm slides along
 * its facing till it rests on the handle SeatGap off; FingerRadius, a gloved finger's half thickness. A trigger hand's
 * index finger instead goes to TriggerPoint (the trigger's face, where the pad of the finger rests) TriggerShare of the
 * way, else it lies where its curls put it (indexed along the frame). Lengths in the placement's space (Moved scales them).
 */
struct AI_LOOTER_SHOOTER_API FHandContact
{
	FHandleShape Handle;
	float Share = 0.f;
	float Seat = 0.f;
	float SeatGap = 0.15f;
	float FingerRadius = 0.8f;
	FVector TriggerPoint = FVector::ZeroVector;
	float TriggerShare = 0.f;
	bool bTrigger = false;

	FHandContact Moved(const FTransform& Space) const;
	static FHandContact Blend(const FHandContact& From, const FHandContact& To, float Alpha);
};

/**
 * Where one hand goes, in whatever space its maker works in: the middle of what it holds (the hand's grip point), how
 * it's turned (its frame: X from the wrist to the knuckles, Y out of the palm toward what it holds), which way its forearm
 * leaves the wrist (the elbow is put that way, so the wrist stays nearly straight; zero: the hand's usual way), how
 * its fingers close, and what they close on (Contact; none: the curls as they are).
 */
struct AI_LOOTER_SHOOTER_API FHandPlacement
{
	FVector Hold = FVector::ZeroVector;
	FQuat Turn = FQuat::Identity;
	FVector Forearm = FVector::ZeroVector;
	FHandFingers Fingers;
	FHandContact Contact;

	static FHandPlacement Make(const FVector& InHold, const FVector& FingersDir, const FVector& PalmDir, const FHandFingers& InFingers,
		const FVector& InForearm = FVector::ZeroVector);

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
	/**
	 * The reload as planned (AWeaponBase::GetReloadPlan): from empty or not, a pump gun's shells. Unset (a test's own gun):
	 * the full reload from empty, its part's own choreography (LooterReload::Steps).
	 */
	FReloadPlan ReloadPlan = { true, 0 };
	/** 0..1 through a pump gun's rack between shots, or -1 while the fore-end is still (AWeaponBase::GetPumpCycle). */
	float PumpCycle = -1.f;
	/**
	 * The sniper's bolt: how open it is (0 home, 1 turned up and drawn back; LooterReload::BoltPose) and how much of the
	 * trigger hand is on its knob (0 on the grip, 1 on the knob) between shots and through its reload.
	 */
	float BoltOpen = 0.f;
	float BoltHand = 0.f;
	/**
	 * 0-1: the trigger finger on the trigger (1: firing, aiming, or just fired) or laid straight along the frame above the
	 * guard (0), as a trained shooter keeps it (UFirstPersonHandsComponent eases it; Looter.Hands.TriggerDiscipline 0: always 1).
	 */
	float TriggerReady = 1.f;
	/**
	 * How much bigger than modeled the gun is drawn (UWeaponModelComponent::ScaleFor). The sockets below already are; the
	 * hands' own numbers off the gun (FGunHands, the reload spots) are the modeled gun's and are scaled by it.
	 */
	float Scale = 1.f;

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
