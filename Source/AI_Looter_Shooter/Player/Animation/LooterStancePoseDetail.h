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

	/**
	 * The hero worn over the body holds the gun at their own shoulder and eye (Stance.HoldOffset): HoldOffsetVector in
	 * component space (the body's facing, pitched as the torso pitches with the aim), and both hands moved by it, keeping
	 * their turn (the gun, in the right hand's socket, moves with them; the left hand finds the foregrip after).
	 */
	FVector HoldOffsetVector(const FLooterStanceInput& Stance, const FVector& Forward, const FVector& Right);
	void ApplyHoldOffset(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Forward,
		const FVector& Right);

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
	 * The left hand solved onto the held gun's foregrip and turned to grip it (LeftHandOnGun; PalmTurn is the carry's turn of
	 * the pose's own hand, ApplyCarry, which the elbow follows); let go of by ForegripHold.
	 */
	void ApplyLeftHandOnForegrip(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FVector& Right,
		const FQuat& PalmTurn);

	/** How firmly the left hand holds the foregrip (0-1): let go of during a reload and for a six-gun carried one-handed. */
	float ForegripHold(const FLooterStanceInput& Stance);

	/** How the left hand holds the gun (LooterStancePoseGrip.cpp): under a long gun's handguard, round a pump, round a six-gun's shooting hand. */
	enum class ELooterGrip : uint8
	{
		Handguard,
		Pump,
		SixGun
	};

	ELooterGrip GripOf(const FLooterStanceInput& Stance);

	/**
	 * How much of the left hand's grip shows (0-1), beside the foregrip's own let-go (ForegripHold): none without a gun, eased
	 * off as a grenade throw or a climb takes the hand, so they turn it from the pose's own hand as they always have.
	 */
	float GripShare(const FLooterStanceInput& Stance);

	/**
	 * The left hand's turn (component space) gripping the gun turned Gun: its knuckles, palm and index-and-thumb way set on the
	 * gun by the hold kind, so it turns with the gun (kick and carry and all), blended from AnimHand (the pose's own hand,
	 * turned with a carried gun) by GripShare. ApplyCarry reckons the left arm's reach with it too.
	 */
	FQuat LeftHandOnGun(const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FQuat& Gun, const FQuat& AnimHand);

	/** The left palm's spot moved off the foregrip by the hold kind (the gun's own space, cm; zero unless tuned), by GripShare. */
	FVector LeftGripShift(const FLooterStanceInput& Stance);

	/**
	 * The left fingers and thumb closed round the gun by the hold kind's curls, blended in by Weight (ForegripHold times
	 * GripShare), on the local pose once the component-space work is done: only these joints' own turns change.
	 */
	void ApplyLeftGripFingers(FCompactPose& Pose, const FLooterStanceInput& Stance, float Weight);

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

	/**
	 * The weary idle (Stance.IdleAlpha, LooterStancePoseIdle.cpp): the posture, the weight shift with the legs solved back
	 * onto the planted feet, the breath, a glance, the fingers and the thumb in the belt. Before the crouch (it fades out as
	 * a crouch comes in).
	 */
	void ApplyIdle(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FLegBones (&Legs)[2],
		FCompactPoseBoneIndex Pelvis, const FVector& Forward, const FVector& Right);

	/** A revolver's reload, the left hand's part: on the cylinder, the rod, the belt; blended in by the reload's weight. */
	void ApplyCylinderReloadHand(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
		const FLooterCylinderReload& Reload, const FVector& Forward, const FVector& Right);
}
