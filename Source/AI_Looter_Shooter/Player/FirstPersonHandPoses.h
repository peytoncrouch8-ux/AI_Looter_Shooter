#pragma once

#include "CoreMinimal.h"
#include "Player/FirstPersonHandsTypes.h"

/**
 * The first-person hands' pose library (the grip spots job, 2026-10-29): how one hand holds one kind of spot on a gun
 * part. A marker socket on the part (SOCKET_<Role>_<L|R>_<Pose>) names one; the hands' code reaches the arm to the marker
 * and dresses the hand with the pose. The numbers are generated from Art/Models/Characters/FPHandPoses.py
 * (FirstPersonHandPoseData.inl): change them there, not here. Pure data, no component, so the tests read it too.
 */
namespace FirstPersonHandPoses
{
	struct FHandPose
	{
		FName Name;
		/** The fingers' curls from straight and spreads, the thumb's swing and spread already set on them (FHandFingers). */
		FHandFingers Fingers;
		/** The thumb's first curl (its swing in across the palm) and spread, as also set in Fingers. */
		float ThumbSwing = 0.f;
		float ThumbSpread = 0.f;
		/** 0..1: how much touch-closing on the handle may still bend the fingers (1: closed till they touch, 0: the pose's own curls). */
		float TouchShare = 1.f;
		/** The default handle the touch-closing closes on (cm of the modeled gun, as FHandleSpec): half depth, half width, edge rounding. */
		float HandleHalfAcross = 2.f;
		float HandleHalfThrough = 2.f;
		float HandleCorner = 1.f;
		/** The cup's rule: the forearm leaves straight back under the palm instead of where the arm's framing puts it. */
		bool bForearmBehindKnuckles = false;
	};

	/** The pose named Pose (Pistol, Cup, Pump, Forend, Wrap, Revolver), or null when there is none. */
	AI_LOOTER_SHOOTER_API const FHandPose* Find(FName Pose);

	/** Every pose, in the order the library lists them. */
	AI_LOOTER_SHOOTER_API TConstArrayView<FHandPose> All();
}
