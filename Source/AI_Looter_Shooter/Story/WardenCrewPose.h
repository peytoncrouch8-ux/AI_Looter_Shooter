#pragma once

#include "CoreMinimal.h"
#include "BonePose.h"

// The story people's pose on the mannequin (the Lamplighter's crew, AWardenCrewMember; Delia Ransom, ADeliaRansom): the
// mannequin's own unarmed idle (MM_Idle, real motion: breath, weight shifts) with a layer made in code on top, so a body
// stands at its station and still lives. The layer, in order: the hips lowered and turned with the feet kept planted (a
// brace, a turn to someone), the spine leaned, swayed and twisted, the head turned to what it looks at, then each hand
// reached to where it holds (a ship's wheel, the Great Lantern, the engine, a shotgun's grip and forend). Everything is
// in the full-size mannequin's units (component space); world targets are turned into it with the mesh's transform, so a
// scaled body (the player's 0.85 world) holds the same world spots. Plain data in, a pose out: tests run it on the
// mannequin's reference pose (Tests/WardenCrewTests.cpp).

/** One hand reaching for a place in the world, blended in by Weight (0: the idle's own hand). */
struct FWardenHand
{
	float Weight = 0.f;
	/** Where the wrist goes (the hand bone's head), world space. */
	FVector Wrist = FVector::ZeroVector;
	/** Which way the elbow points: a spot it bends toward, world space. */
	FVector Elbow = FVector::ZeroVector;
};

/**
 * A long gun held in both hands (Delia's shotgun): its grip in the right hand's HandGrip_R socket (the gun hangs there,
 * its +X its barrel, as the player's third-person guns do), the left hand on its forend. Carried low across the body at
 * rest, shouldered toward AimAt, broken open for a reload, kicked by a shot.
 */
struct FWardenGunHold
{
	/** 0: no gun (the hands are FWardenPoseInput::Hands'); 1: both hands on it. */
	float Weight = 0.f;
	/** Where a shouldered gun points (world). */
	FVector AimAt = FVector::ZeroVector;
	/** 0 carried low across the body (the muzzle down and to the left), 1 shouldered at AimAt. */
	float Shoulder = 0.f;
	/** 0 closed; 1 broken open, the barrels dropped for the shells (a reload). */
	float Break = 0.f;
	/** 0 the left hand on the forend; 1 at the apron pocket for shells (a reload's middle). */
	float LeftToPocket = 0.f;
	/** A shot's kick, 1 as it fires, easing to 0: the muzzle rises and the gun comes back into the shoulder. */
	float Kick = 0.f;
	/** The right hand's HandGrip_R socket from its hand bone (the skeleton's: read on the game thread). */
	FTransform SocketInHand = FTransform::Identity;
	/** Where the left hand holds, from the grip, in the gun's own space (cm). */
	FVector Forend = FVector(30.f, 0.f, -3.f);
};

/** Everything the layer needs for a frame, filled on the game thread (FWardenCrewAnimInstanceProxy copies it). */
struct FWardenPoseInput
{
	/** The idle's own clock (s): where in MM_Idle the body is. Each body starts at its own place, so two never breathe together. */
	float IdleTime = 0.f;

	/** The hips lowered this far (cm), the knees taking it: a brace, a crouch to work. */
	float Crouch = 0.f;
	/** The spine leaned forward (degrees; back below 0): leaning into the engine, looking up at the lantern. */
	float Lean = 0.f;
	/** The body swayed to its right (degrees): the ship's lurch. */
	float Roll = 0.f;
	/** The hips and chest turned to their right (degrees) with the feet kept where they stand: a turn to someone. */
	float BodyYaw = 0.f;
	/** The chest alone turned further (degrees, right): a glance over the shoulder, a twist onto the aim. */
	float ChestYaw = 0.f;
	/** The chest rocked back (degrees): a shot's kick, a flinch. */
	float ChestBack = 0.f;

	/** What the head turns to (world) and how much (0-1). */
	FVector LookAt = FVector::ZeroVector;
	float LookWeight = 0.f;

	/** The hands: 0 left, 1 right (unused while a gun takes them). */
	FWardenHand Hands[2];

	FWardenGunHold Gun;
};

namespace WardenCrewPose
{
	/** The head turns at most this far from the chest's way either side (degrees), up and down at most these. */
	inline constexpr float MaxLookYaw = 75.f;
	inline constexpr float MaxLookUp = 35.f;
	inline constexpr float MaxLookDown = 40.f;
	/** The body turns at most this far on planted feet (degrees). */
	inline constexpr float MaxBodyYaw = 60.f;

	/** The bone the right hand's held gun hangs from (the player's third-person hold's: UWeaponManagerComponent). */
	inline const TCHAR* const GunSocket = TEXT("HandGrip_R");

	/**
	 * Puts Input's layer on Pose (a whole mannequin pose in local space, the idle's): ComponentToWorld is the mesh's
	 * transform, which world targets are taken into. The mannequin faces its +Y, so its forward, right and up in its own
	 * space are found from the mesh's transform too (the actor's +X is the mesh's front).
	 */
	void Apply(FCompactPose& Pose, const FWardenPoseInput& Input, const FTransform& ComponentToWorld);

	/**
	 * The gun's grip frame (the socket's place and turn the gun hangs from), in component space, for a shoulder at
	 * Shoulder: the rules Apply follows, apart so tests can check where a shouldered or carried gun points.
	 */
	FTransform GunFrame(const FWardenGunHold& Gun, const FVector& Shoulder, const FVector& AimAtLocal, const FVector& Forward,
		const FVector& Right, const FVector& Up);
}
