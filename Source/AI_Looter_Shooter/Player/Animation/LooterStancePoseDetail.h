#pragma once

#include "CoreMinimal.h"
#include "Animation/BoneReference.h"
#include "BoneContainer.h"
#include "BonePose.h"

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
	 * toward the middle, the elbow straightening; eased off through a sprint and a reload. The left hand follows it onto
	 * the gun after.
	 */
	void ApplyHoldReach(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right);

	/** Each shot rocks the chest back and lets the right arm give. */
	void ApplyRecoil(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right);

	/** A melee strike's jab (Stance.MeleeJab): the chest twists into the blow and the right hand drives forward. */
	void ApplyMeleeJab(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Forward,
		const FVector& Right);

	/** The left hand solved onto the held gun's foregrip, and let go of during a reload. */
	void ApplyLeftHandOnForegrip(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right);
}
