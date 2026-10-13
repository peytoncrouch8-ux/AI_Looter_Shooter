#pragma once

#include "CoreMinimal.h"

struct FReferenceSkeleton;

/** One bone of a generated pose table (HiramPoseData.inl, ThaddeusPoseData.inl): its name, its parent among the table's bones (-1: under the armature's root), its head at rest (cm). */
struct FDeadFriendPoseBoneInfo
{
	const TCHAR* Name;
	int32 Parent;
	float Rest[3];
};

/** One bone in one pose of a table: its own turn under its parent's (FQuat X, Y, Z, W) and its shift (cm). */
struct FDeadFriendPoseBone
{
	float Own[4];
	float Shift[3];
};

/**
 * A generated pose table as its .inl lays it out (Tools/dead_friend_poses.py, from the model script's table): its bones,
 * parents first, and NumPoses rows of NumBones each, pose by pose. The model's rest pose is its idle.
 */
struct FDeadFriendPoseTable
{
	const FDeadFriendPoseBoneInfo* Bones = nullptr;
	int32 NumBones = 0;
	const FDeadFriendPoseBone* Rows = nullptr;
	int32 NumPoses = 0;
};

/**
 * A pose of a friendly dead man (Hiram, Thaddeus) on his model's rig (the Unpaid's bones and Abel's front skirt chain), as
 * the figure eases from one to the next: each bone's own turn about its joint under its parent's (the rig's rule: Turned =
 * Above * Own) and its shift off where its parent carries it, by name. A bone left out stays at rest under its parent. Made
 * from a row of the character's pose table (DeadFriendPoses::FromTable), blended between rows, and a small turn (a breath)
 * laid on top by the character's own code (Bend).
 */
struct AI_LOOTER_SHOOTER_API FDeadFriendPose
{
	TMap<FName, FQuat> Turns;
	TMap<FName, FVector> Shifts;

	/** Turns Bone on top of what this pose gave it: Forward nods it forward, TurnRight to its right, LeanRight tips its top right (degrees). */
	FDeadFriendPose& Bend(FName Bone, float Forward, float TurnRight = 0.f, float LeanRight = 0.f);

	/** Shifts Bone's joint off where its parent carries it (cm, component space), on top of this pose's. */
	FDeadFriendPose& Move(FName Bone, const FVector& Shift);

	/** Alpha of the way from A to B: turns slerped, shifts eased (a bone in one only, from or to rest). */
	static FDeadFriendPose Blend(const FDeadFriendPose& A, const FDeadFriendPose& B, float Alpha);
};

/** What's laid on top of a pose while he talks: his head turned to the listener (degrees about up, + right), his jaw open (degrees). */
struct FDeadFriendLayer
{
	float HeadYaw = 0.f;
	float JawOpen = 0.f;
};

/** The rig's bones as these poses name them, the tables, and the solver. */
namespace DeadFriendPoses
{
	inline const FName Pelvis(TEXT("pelvis"));
	inline const FName Chest(TEXT("spine_02"));
	inline const FName Head(TEXT("head"));
	inline const FName Jaw(TEXT("jaw"));
	inline const FName Hat(TEXT("hat"));

	/** An arm's bones: Side 0 left, 1 right; Part 0 the upper arm, 1 the forearm, 2 the hand. */
	AI_LOOTER_SHOOTER_API FName ArmBone(int32 Side, int32 Part);

	/** A turn in plain angles (degrees). */
	AI_LOOTER_SHOOTER_API FQuat Turn(float Forward, float TurnRight, float LeanRight);

	/** Row Pose of Table as a pose (every bone's own turn and shift by name); an empty pose past its rows. */
	AI_LOOTER_SHOOTER_API FDeadFriendPose FromTable(const FDeadFriendPoseTable& Table, int32 Pose);

	/** A bone's index in Table, or INDEX_NONE. */
	AI_LOOTER_SHOOTER_API int32 FindTableBone(const FDeadFriendPoseTable& Table, FName Bone);

	/**
	 * Every bone of Table in Pose with Layer on top, from the table's own rest heads, no model needed (the tests' measure,
	 * and his head and mouth without his model): each bone's head (cm) and whole turn from rest, indexed as the table lists them.
	 */
	struct FTableHeads
	{
		TArray<FVector> Heads;
		TArray<FQuat> Turns;
	};
	AI_LOOTER_SHOOTER_API FTableHeads SolveTable(const FDeadFriendPoseTable& Table, const FDeadFriendPose& Pose,
		const FDeadFriendLayer& Layer = FDeadFriendLayer());

	/** Bone's head in a table solve, or ZeroVector without it; how far its forward turns from the body's front (degrees, + right). */
	AI_LOOTER_SHOOTER_API FVector TableHead(const FDeadFriendPoseTable& Table, const FTableHeads& Solved, FName Bone);
	AI_LOOTER_SHOOTER_API float TableYaw(const FDeadFriendPoseTable& Table, const FTableHeads& Solved, FName Bone);

	/**
	 * Every bone of Skeleton in component space in Pose with Layer on top (indexed as the skeleton is), posed as the
	 * Unpaid's rig poses itself: a bone the pose leaves out keeps its rest place under its parent.
	 */
	AI_LOOTER_SHOOTER_API TArray<FTransform> Solve(const FReferenceSkeleton& Skeleton, const FDeadFriendPose& Pose,
		const FDeadFriendLayer& Layer = FDeadFriendLayer());

	/** Where Bone is in a skeleton solve (component space), or ZeroVector without it. */
	AI_LOOTER_SHOOTER_API FVector BoneLocation(const FReferenceSkeleton& Skeleton, const TArray<FTransform>& Solved, FName Bone);

	/** How far Bone's forward (+X) turns from the body's front in a skeleton solve (degrees about up, + right). */
	AI_LOOTER_SHOOTER_API float BoneYaw(const FReferenceSkeleton& Skeleton, const TArray<FTransform>& Solved, FName Bone);
}
