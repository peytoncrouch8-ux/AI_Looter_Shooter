#pragma once

#include "CoreMinimal.h"
#include "Animation/BoneReference.h"
#include "BoneContainer.h"
#include "BonePose.h"

struct FReferenceSkeleton;

/**
 * Ellis tips his hat back as he raises a sight (the user, 2026-10-15: in third-person aiming the brim covered the scope;
 * his eyes need a clear line to the reticle): the `hat` bone (Art/Models/Characters/Ellis.py, its head at the back of
 * the band) turns brim-up about the head's left-right axis, by the aim's share of MaxDegrees. A mesh without a hat bone
 * tips nothing.
 */
struct AI_LOOTER_SHOOTER_API FBodyHatTip
{
	/** How far the brim tips up at full aim (degrees). */
	static constexpr float MaxDegrees = 18.f;

	/** Finds the hat and head bones and the head's rest frame in Skeleton (once per mesh). */
	void Bind(const FReferenceSkeleton& Skeleton);
	bool IsBoundTo(const FReferenceSkeleton& Skeleton) const;
	bool HasHat() const { return bHasHat; }

	/** Tips the hat by Aim (0 lowered to 1 sighted, eased here) in Pose (component space). */
	void Apply(FCSPose<FCompactPose>& Pose, float Aim);

	/** The tip for Aim: none at 0, MaxDegrees at 1, easing in and out between (degrees). */
	static float DegreesFor(float Aim);

private:
	FBoneReference Hat;
	FBoneReference Head;
	/** The face's forward and the head's left-right axis, in the head bone's own space (from the rest pose). */
	FVector ForwardInHead = FVector::ForwardVector;
	FVector AcrossInHead = FVector::RightVector;
	bool bHasHat = false;
	int32 BoundBoneCount = 0;
	uint16 BoundSerial = 0;
	bool bReferencesValid = false;
};
