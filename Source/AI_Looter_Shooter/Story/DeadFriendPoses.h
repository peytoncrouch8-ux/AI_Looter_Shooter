#pragma once

#include "CoreMinimal.h"

struct FReferenceSkeleton;

/** One arm laid by where it points (component space, under the chest's turn: X forward, Y right, Z up). */
struct FDeadFriendArm
{
	bool bSet = false;
	FVector Upper = FVector::DownVector;
	FVector Fore = FVector::DownVector;
	/** The wrist's bend (degrees, + down). */
	float Wrist = 0.f;
};

/**
 * A pose of a friendly dead man on the Unpaid's rig (SK_Unpaid's bones, or a model built on its skeleton: Hiram, Thaddeus),
 * authored in plain angles rather than read from a model's table, since their models aren't made yet: each bone's own turn
 * about its joint under its parent's (the rig's rule: Turned = Above * Own), its shift, and the arms laid by direction as
 * the Bell-Ringer's are. Turns by the rest pose's axes: Forward nods it forward (a hanging limb swings back), TurnRight
 * turns it to its right, LeanRight tips its top to its right. A bone left out stays at rest under its parent.
 */
struct AI_LOOTER_SHOOTER_API FDeadFriendPose
{
	TMap<FName, FQuat> Turns;
	TMap<FName, FVector> Shifts;
	/** 0 its left arm, 1 its right. */
	FDeadFriendArm Arms[2];

	/** Turns Bone (on top of what this pose gave it already). */
	FDeadFriendPose& Bend(FName Bone, float Forward, float TurnRight = 0.f, float LeanRight = 0.f);

	/** Shifts Bone's joint off where its parent carries it (cm, component space). */
	FDeadFriendPose& Move(FName Bone, const FVector& Shift);

	/** Lays an arm (0 left, 1 right): its upper arm and forearm along these (normalized here), the wrist bent. */
	FDeadFriendPose& Arm(int32 Side, const FVector& Upper, const FVector& Fore, float Wrist = 0.f);

	/** Alpha of the way from A to B: turns slerped, shifts, arm directions and wrists eased (a bone in one only, from rest). */
	static FDeadFriendPose Blend(const FDeadFriendPose& A, const FDeadFriendPose& B, float Alpha);
};

/** What's laid on top of a pose while he talks: his head turned to the listener (degrees about up, + right), his jaw open (degrees). */
struct FDeadFriendLayer
{
	float HeadYaw = 0.f;
	float JawOpen = 0.f;
};

/** The Unpaid's rig as these poses name it (FUnpaidRigBones' defaults), and the solver. */
namespace DeadFriendPoses
{
	inline const FName Pelvis(TEXT("pelvis"));
	inline const FName Spine(TEXT("spine_01"));
	inline const FName Chest(TEXT("spine_02"));
	inline const FName Neck(TEXT("neck"));
	inline const FName Head(TEXT("head"));
	inline const FName Jaw(TEXT("jaw"));
	inline const FName Hat(TEXT("hat"));
	inline const FName Tail(int32 Link)
	{
		return FName(*FString::Printf(TEXT("tail_%02d"), Link));
	}
	/** An arm's bones: Side 0 left, 1 right; Part 0 the upper arm, 1 the forearm, 2 the hand. */
	AI_LOOTER_SHOOTER_API FName ArmBone(int32 Side, int32 Part);

	/** A turn in the pose's plain angles (degrees). */
	AI_LOOTER_SHOOTER_API FQuat Turn(float Forward, float TurnRight, float LeanRight);

	/**
	 * Every bone of Skeleton in component space in Pose with Layer on top (indexed as the skeleton is), posed as the
	 * Unpaid's rig poses itself: a bone the pose leaves out keeps its rest place under its parent.
	 */
	AI_LOOTER_SHOOTER_API TArray<FTransform> Solve(const FReferenceSkeleton& Skeleton, const FDeadFriendPose& Pose,
		const FDeadFriendLayer& Layer = FDeadFriendLayer());

	/** Where Bone is in a solved pose (component space), or ZeroVector without it. */
	AI_LOOTER_SHOOTER_API FVector BoneLocation(const FReferenceSkeleton& Skeleton, const TArray<FTransform>& Solved, FName Bone);

	/** How far Bone's forward (+X) turns from the body's front in a solved pose (degrees about up, + right). */
	AI_LOOTER_SHOOTER_API float BoneYaw(const FReferenceSkeleton& Skeleton, const TArray<FTransform>& Solved, FName Bone);
}
