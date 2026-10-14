#pragma once

#include "CoreMinimal.h"
#include "Animation/BoneReference.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Player/Animation/LooterStancePose.h"

struct FLooterStanceInput;

// What the stance layer's files share (LooterStancePose*.cpp): finding and moving bones, and the parts of the pose that
// have their own file. Not for use outside Player/Animation.
namespace LooterStancePoseDetail
{
	/** The compact-pose index of a bone by name; invalid when the skeleton (or this LOD) lacks it. */
	inline FCompactPoseBoneIndex FindBone(const FBoneContainer& Bones, const TCHAR* Name)
	{
		FBoneReference Reference(Name);
		Reference.Initialize(Bones);
		return Reference.GetCompactPoseIndex(Bones);
	}

	inline void SetBone(FCSPose<FCompactPose>& Pose, FCompactPoseBoneIndex Bone, const FTransform& Transform)
	{
		const FBoneTransform Single[] = { FBoneTransform(Bone, Transform) };
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Single), 1.f);
	}

	/** Rotates a bone about its own pivot, in component space; its children follow. */
	inline void RotateBone(FCSPose<FCompactPose>& Pose, FCompactPoseBoneIndex Bone, const FQuat& Rotation)
	{
		if (Bone.IsValid())
		{
			FTransform Transform = Pose.GetComponentSpaceTransform(Bone);
			Transform.SetRotation(Rotation * Transform.GetRotation());
			SetBone(Pose, Bone, Transform);
		}
	}

	/** One leg's bones, and which side it is. */
	struct FLegBones
	{
		FCompactPoseBoneIndex Thigh, Calf, Foot;
		float Side; // -1 left, +1 right
	};

	/**
	 * A mantle or vault: legs tucked up (the left one, away from the gun in the right hand, leads), blended in by the climb
	 * input's weight. Pelvis is the hips bone; Forward and Right are the body's facing and its right in component space.
	 */
	void ApplyClimbLegs(FCSPose<FCompactPose>& Pose, const FLegBones (&Legs)[2], FCompactPoseBoneIndex Pelvis, const FLooterStanceInput& Stance,
		const FVector& Forward, const FVector& Right);

	/**
	 * A mantle's hands reaching onto the ledge (the right one only if no gun is in it), or a vault's free hand planted on the
	 * obstacle, blended in by the climb input's weight. After the foregrip, so the hand leaves the gun by blending away.
	 */
	void ApplyClimbHands(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Forward,
		const FVector& Right);

	/**
	 * A short gun held out (Stance.HoldReach): the right hand, the gun in it, pushed out along the barrel and a little in
	 * toward the middle, the elbow straightening; eased off through a sprint, a reload and a throw's dip. The left hand
	 * follows it onto the gun after.
	 */
	void ApplyHoldReach(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right);

	/** Each shot rocks the chest back and lets the right arm give. */
	void ApplyRecoil(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right);

	/** A melee strike's jab (Stance.MeleeJab): the chest twists into the blow and the right hand drives forward. */
	void ApplyMeleeJab(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Forward,
		const FVector& Right);

	/**
	 * The gun carried rather than shouldered (Stance.Carry; LooterStancePoseCarry.cpp): the right hand lowered to the
	 * low-ready, or moved to the sprint's carry (across the chest; a six-gun low at the side), turned with the gun so the gun
	 * keeps its place in the palm, and drawn in until the gun's foregrip is in the left arm's reach. ChestTurn is how far the
	 * stance has turned the chest from the graph's pose (its lean and aim). Returns the hands' turn, which the left hand
	 * takes too (its palm turns with the gun); identity when the gun is shouldered.
	 */
	FQuat ApplyCarry(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FQuat& ChestTurn,
		const FVector& Forward, const FVector& Right);

	/**
	 * The left hand solved onto the held gun's foregrip, its palm turned by PalmTurn (the carry's, ApplyCarry); let go of
	 * during a reload and for a six-gun carried one-handed in a sprint.
	 */
	void ApplyLeftHandOnForegrip(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right,
		const FQuat& PalmTurn);

	/**
	 * The left palm (the mannequin's HandGrip_L point, where a held thing sits) from the hand_l bone, in that bone's space:
	 * the hand's IK aims this point, not the wrist, at what it holds.
	 */
	inline FVector LeftPalmFromWrist()
	{
		return FVector(7.5f, -2.5f, 0.f);
	}

	/**
	 * Two-bone IK on one arm, blended in by Weight: the wrist to WristTarget, the elbow toward ElbowHint. bTurnHand turns
	 * the hand with the forearm (a big swing, so the wrist doesn't wrench); otherwise it keeps the pose's orientation.
	 */
	void SolveArm(FCSPose<FCompactPose>& Pose, FCompactPoseBoneIndex UpperArm, FCompactPoseBoneIndex LowerArm, FCompactPoseBoneIndex Hand,
		const FVector& WristTarget, const FVector& ElbowHint, float Weight, bool bTurnHand);

	/**
	 * A grenade throw's chest (its wind-up and turn into the toss) and, with a gun in hand, the right hand dipping out of the
	 * throw's way (LooterStancePoseThrow.cpp). Before the left hand finds the foregrip, so it follows the dipped gun.
	 */
	void ApplyThrowBody(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FLooterThrowStance& Throw,
		const FVector& Forward, const FVector& Right);

	/** A grenade throw's left arm, cocked and slung, blended in by Throw.Arm; after the foregrip, so it lets go of the gun. */
	void ApplyThrowArm(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FLooterThrowStance& Throw,
		const FVector& Forward, const FVector& Right);

	/**
	 * A revolver's reload, the gun hand's part (LooterStancePoseReload.cpp): the gun brought in front of the chest and the
	 * flick that shuts the cylinder. Before the left hand, which works on the gun where this puts it.
	 */
	void ApplyCylinderReloadGun(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
		const FLooterCylinderReload& Reload, const FVector& Forward, const FVector& Right);

	/** A revolver's reload, the left hand's part: on the cylinder, the rod, the belt; blended in by the reload's weight. */
	void ApplyCylinderReloadHand(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
		const FLooterCylinderReload& Reload, const FVector& Forward, const FVector& Right);
}
