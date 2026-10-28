#pragma once

#include "CoreMinimal.h"

/**
 * The Unpaid's body parts as its rig and arms track them (AUnpaidCreature uses them by these names: UnpaidCreatureRig.cpp,
 * UnpaidCreatureArms.cpp, UnpaidCreatureArmGuard.cpp). Split from UnpaidCreature.h (2026-10-28, the ~500-line rule).
 */
namespace UnpaidBody
{
	struct FArm
	{
		float Side = 1.f;
		int32 UpperArm = INDEX_NONE;
		int32 LowerArm = INDEX_NONE;
		int32 Hand = INDEX_NONE;
		TArray<int32> Fingers;
		/** The axis each finger curls about, in the rest pose's frame, and how far it fans from the middle one (in fingers). */
		TArray<FVector> CurlAxes;
		TArray<float> FanOffsets;
		/** The idle curl and fan each finger rests in (the model bakes them in), taken back off as it's posed. */
		TArray<FQuat> IdleTurns;
	};

	/** The pose's smoothed channels (degrees, or 0 to 1). */
	struct FPoseChannels
	{
		float Lean = 0.f;
		float Rear = 0.f;
		float Reach = 0.f;
		float Spread = 0.f;
		float Jaw = 0.f;
		float Curl = 0.f;
		float Splay = 0.f;
		float Snap = 0.f;
		float LookYaw = 0.f;
		float LookPitch = 0.f;
		float Death = 0.f;
		/** Hunting (0 to 1): its arms come up and claw at the air in turn while it chases. */
		float Hunt = 0.f;
		/** Dying, crumpling to the ground (0 to 1; not a boss); calm, its slow sway where it hangs (degrees of roll). */
		float Crumple = 0.f;
		float Sway = 0.f;
		/** Each arm's rake (0 left, 1 right): -1 drawn back and out, 0 as it is, 1 swept forward across its front. */
		float Rake[2] = { 0.f, 0.f };
	};

	/** One arm's lag behind the body's moves, on a spring: degrees swung back and out from the body, and their speeds. */
	struct FArmSway
	{
		float Swing = 0.f;
		float Out = 0.f;
		float SwingSpeed = 0.f;
		float OutSpeed = 0.f;
		/** How far the keep-out has turned it away from the body and swung it back (degrees), easing off once it needn't. */
		float KeepOut = 0.f;
		float KeepSwing = 0.f;
	};

	/** A point on an arm's hit hull (in its arm bone's space) that must keep out of the torso, and how deep it sits in each torso shape at rest. */
	struct FArmGuardPoint
	{
		/** Its arm bone (an index into Bones). */
		int32 Bone = INDEX_NONE;
		FVector InBone = FVector::ZeroVector;
		/** Per TorsoGuards entry: the rest pose's depth (a shoulder's sleeve sits in the chest's hull by design). */
		TArray<float> RestDepth;
		/** On the upper arm by the shoulder: the chest it hangs from may take it in (the armpit's fold), nothing else. */
		bool bNearShoulder = false;
	};

	/** A torso shape the arms keep out of: its bone (an index into Bones) and its shape in TorsoHulls. */
	struct FTorsoGuard
	{
		int32 Bone = INDEX_NONE;
		int32 Shape = INDEX_NONE;
	};
}
