#pragma once

#include "CoreMinimal.h"
#include "Animation/BoneReference.h"
#include "BoneContainer.h"
#include "BonePose.h"

struct FReferenceSkeleton;

/**
 * Ellis's forearms roll with his hands (the user, 2026-10-15: "His wrists are either invisible or don't match the
 * gloves"): the retarget turns his hand about the forearm (the left about 100 degrees round onto a foregrip or the
 * revolver) but nothing turned the forearm, so the gauntlet stayed put while the glove turned and the wrist tore open.
 * Each `lowerarm_twist*` bone under a lowerarm (Art/Models/Characters/Ellis.py) takes the hand's twist about the forearm
 * by how far down the forearm it sits, the way the mannequin's twist bones carry it. A mesh without them twists nothing.
 */
struct AI_LOOTER_SHOOTER_API FBodyForearmTwist
{
	/** Finds each side's forearm, hand and twist bones and their places down the forearm in Skeleton (once per mesh). */
	void Bind(const FReferenceSkeleton& Skeleton);
	bool IsBoundTo(const FReferenceSkeleton& Skeleton) const;
	int32 NumTwistBones() const;

	/** Turns the twist bones of Pose (component space) by their shares of each hand's twist from rest. */
	void Apply(FCSPose<FCompactPose>& Pose);

	/** Rotation's turn about Axis (unit), in radians, -PI..PI: the part of a hand's turn that rolls the forearm. */
	static float TwistAbout(const FQuat& Rotation, const FVector& Axis);

private:
	struct FTwistBone
	{
		FBoneReference Bone;
		/** How far down the forearm it sits (0 the elbow, 1 the wrist): its share of the hand's twist. */
		float Share = 0.f;
		/** Its rest transform in the forearm's space. */
		FTransform InForearm;
	};

	struct FSide
	{
		FBoneReference Forearm;
		FBoneReference Hand;
		/** The hand's rest rotation in the forearm's space, and the forearm's axis (elbow to wrist) in its own space. */
		FQuat RestHand = FQuat::Identity;
		FVector Axis = FVector::ForwardVector;
		TArray<FTwistBone> Twists;
	};

	void InitBoneReferences(const FBoneContainer& Bones);

	TArray<FSide> Sides;
	int32 BoundBoneCount = 0;
	FName BoundFirstBone;
	uint16 BoundSerial = 0;
	bool bReferencesValid = false;
};
