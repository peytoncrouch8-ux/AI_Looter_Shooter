#pragma once

#include "CoreMinimal.h"

class UPoseableMeshComponent;
class USkeletalMesh;

/**
 * One bone of a generated animal pose table (World/HenPoseData.inl, RoosterPoseData.inl, CatPoseData.inl, written by
 * Tools/animal_poses.py from the model scripts' tables): its name, its parent and its mirror among the table's bones
 * (-1: under the armature's root; the mirror is the bone of the other side, itself for a middle one), its head at rest (cm).
 */
struct FAnimalPoseBoneInfo
{
	const TCHAR* Name;
	int32 Parent;
	int32 Mirror;
	float Rest[3];
};

/** One bone in one pose of a table: its own turn under its parent's (FQuat X, Y, Z, W) and its shift (cm). */
struct FAnimalPoseBone
{
	float Own[4];
	float Shift[3];
};

/**
 * A generated pose table as its .inl lays it out: the bones, parents first, and NumPoses rows of NumBones each, pose by
 * pose, in the order of the animal's pose enum (World/AnimalPoseTables.h). Component space at size 1 (cm; X forward, Y
 * right, Z up; the origin on the ground under the animal).
 */
struct AI_LOOTER_SHOOTER_API FAnimalPoseTable
{
	const FAnimalPoseBoneInfo* Bones = nullptr;
	int32 NumBones = 0;
	const FAnimalPoseBone* Rows = nullptr;
	const TCHAR* const* PoseNames = nullptr;
	int32 NumPoses = 0;

	bool IsValidBone(int32 Bone) const { return Bones && Bone >= 0 && Bone < NumBones; }
	/** A bone's index, or INDEX_NONE. */
	int32 FindBone(FName Name) const;
	/** A pose's index by its table name ("peck"), or INDEX_NONE. */
	int32 FindPose(const TCHAR* Name) const;
	FName BoneName(int32 Bone) const;
	/** Its parent's index, or INDEX_NONE under the root. */
	int32 ParentOf(int32 Bone) const;
	/** The bone of the other side (_l for _r), itself for a middle one. */
	int32 MirrorOf(int32 Bone) const;
	FVector RestHead(int32 Bone) const;
	FQuat Own(int32 Pose, int32 Bone) const;
	FVector Shift(int32 Pose, int32 Bone) const;
};

/**
 * A pose of an animal on its table's bones, as its code eases from one to the next: per bone (indexed as the table lists
 * them) its own turn about its joint under its parent's, its shift off where its parent carries it, and a lead: a turn
 * in the animal's own frame laid on at the joint before the parent's (a look: the head turned to the player). A bone's
 * whole turn is Lead * Above * Own, and its children ride it.
 */
struct AI_LOOTER_SHOOTER_API FAnimalPose
{
	TArray<FQuat, TInlineAllocator<32>> Own;
	TArray<FVector, TInlineAllocator<32>> Shift;
	TArray<FQuat, TInlineAllocator<32>> Lead;

	int32 Num() const { return Own.Num(); }
	bool IsEmpty() const { return Own.IsEmpty(); }
	/** Every bone at rest under its parent (the model's bind pose), NumBones of them. */
	void Reset(int32 NumBones);
};

/** Where a pose puts each bone of its table, from the table's own rest heads (no model needed). */
struct FAnimalPoseSolve
{
	/** Each bone's head (cm, component space) and whole turn from rest, indexed as the table lists them. */
	TArray<FVector, TInlineAllocator<32>> Heads;
	TArray<FQuat, TInlineAllocator<32>> Turns;
};

/** A damped spring swinging a hanging part (a hen's wattles, her tail's tip) in two angles (degrees). */
struct AI_LOOTER_SHOOTER_API FAnimalSpring
{
	FVector2D Angle = FVector2D::ZeroVector;
	FVector2D Speed = FVector2D::ZeroVector;

	/**
	 * Moves it on by DeltaSeconds: pulled back at Frequency (Hz) with Damping (share of critical), pushed by Push
	 * (degrees a second squared: what the joint's acceleration throws it by), never past Limit degrees.
	 */
	void Update(float DeltaSeconds, const FVector2D& Push, float Frequency, float Damping, float Limit);
};

/** Building, blending and solving poses (plain functions over the tables, so the tests use them without a model). */
namespace AnimalPose
{
	/** Row Pose of Table (an empty pose past its rows). */
	AI_LOOTER_SHOOTER_API FAnimalPose FromTable(const FAnimalPoseTable& Table, int32 Pose);

	/** The same pose on the other side: _l and _r swapped, every turn and shift reflected across the animal's middle. */
	AI_LOOTER_SHOOTER_API FAnimalPose Mirrored(const FAnimalPoseTable& Table, const FAnimalPose& Pose);

	/** Alpha of the way from A to B into Out (turns slerped, shifts eased); Out may be A or B. */
	AI_LOOTER_SHOOTER_API void Blend(const FAnimalPose& A, const FAnimalPose& B, float Alpha, FAnimalPose& Out);

	/** Only Bones of InOut, Alpha of the way to B's (a run's legs under a flutter's wings). */
	AI_LOOTER_SHOOTER_API void BlendBones(FAnimalPose& InOut, const FAnimalPose& B, float Alpha, TConstArrayView<int32> Bones);

	/**
	 * A gait's pose Phase (0-1) through a stride of two steps: First eased into Second over the first step and back over
	 * the second (a walk's two contacts, or a one-sided pose and its mirror). Returns how far into its step it is (0-1).
	 */
	AI_LOOTER_SHOOTER_API float StrideBlend(const FAnimalPose& First, const FAnimalPose& Second, float Phase, FAnimalPose& Out);

	/** Turns Bone on top of its own turn (degrees: Pitch lifts its front, Yaw turns it right, Roll drops its right side). */
	AI_LOOTER_SHOOTER_API void Bend(FAnimalPose& Pose, int32 Bone, float Pitch, float Yaw, float Roll = 0.f);

	/** Turns Bone, and what rides it, in the animal's own frame at its joint (degrees: Yaw to its right, Pitch up). */
	AI_LOOTER_SHOOTER_API void Look(FAnimalPose& Pose, int32 Bone, float Yaw, float Pitch);

	/** Moves Bone's joint (and what rides it) by Offset (cm, the animal's frame), on top of its shift. */
	AI_LOOTER_SHOOTER_API void Nudge(FAnimalPose& Pose, int32 Bone, const FVector& Offset);

	/** Every bone's head and whole turn in Pose, from Table's own rest heads. */
	AI_LOOTER_SHOOTER_API void Solve(const FAnimalPoseTable& Table, const FAnimalPose& Pose, FAnimalPoseSolve& Out);

	/** How far a bone's front (+X) is turned from the animal's front in a solve (degrees about up; + right). */
	AI_LOOTER_SHOOTER_API float YawOf(const FAnimalPoseSolve& Solved, int32 Bone);

	/** How far a turn is from none (degrees, the short way). */
	AI_LOOTER_SHOOTER_API float Degrees(const FQuat& Turn);

	/** 0 to 1 eased in and out (smoothstep). */
	AI_LOOTER_SHOOTER_API float Ease(float Share);
}

/**
 * A pose table bound to a model's skeleton: which table bone each of its bones is, and its rest in component space, so a
 * pose goes straight into a poseable mesh's local transforms once a frame (no animation runs; bones the table lacks keep
 * their rest under their parent). Bind it to the mesh's asset once; Apply poses it.
 */
struct AI_LOOTER_SHOOTER_API FAnimalRig
{
	/** Binds Table to Mesh's skeleton: false (and unbound) without a mesh or with none of the table's bones in it. */
	bool Bind(const USkeletalMesh* Mesh, const FAnimalPoseTable& InTable);

	bool IsBound() const { return Table != nullptr && TableBone.Num() > 0; }

	/** Poses the component's bones (its asset the one bound) and refreshes them for this frame's drawing. */
	void Apply(UPoseableMeshComponent& Component, const FAnimalPose& Pose);

	/** Where a table bone's joint was put by the last Apply (cm, component space), or ZeroVector. */
	FVector JointOf(int32 TableBoneIndex) const;

	const FAnimalPoseTable* Table = nullptr;

private:
	/** Per skeleton bone: its table bone (INDEX_NONE: none), its parent, its rest in component space. */
	TArray<int32> TableBone;
	TArray<int32> Parent;
	TArray<FTransform> RestPose;
	/** Per table bone: its skeleton bone (INDEX_NONE: not in the skeleton). */
	TArray<int32> SkeletonBone;
	/** Scratch, reused every Apply. */
	TArray<FTransform> Posed;
	TArray<FQuat> Turned;
};
