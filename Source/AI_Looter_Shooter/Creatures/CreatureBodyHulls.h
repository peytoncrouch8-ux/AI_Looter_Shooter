#pragma once

#include "CoreMinimal.h"

class UBodySetup;
class UPhysicsAsset;
struct FReferenceSkeleton;

/**
 * A body's hit zones (its physics asset's shapes, or a static mesh's simple collision) as plain geometry on their bones,
 * to measure how deep one part sits in another: an Unpaid's arm in its own torso (its pose keeps them apart,
 * UnpaidCreatureArms.cpp), a spider's leg in its abdomen, one creature in the next, a story character in the rail he sits
 * on (Looter.CastShots' numbers, the creature animation tests). The art scripts wrap every part of a creature in a hull, so
 * the measure follows the model itself rather than numbers copied out of it.
 *
 * Everything is in a shape's bone space, as the physics asset holds it: give a point in the space you work in through the
 * bone's transform in that space (component space for a pose, the world for the level).
 */
struct AI_LOOTER_SHOOTER_API FCreatureBodyHulls
{
	/** One shape on a bone, in the bone's own space. */
	struct FShape
	{
		FName Bone;
		/** The bone's index in the reference skeleton; INDEX_NONE for a static mesh's own shape. */
		int32 BoneIndex = INDEX_NONE;
		/** A convex's or a box's faces, facing out (a point's depth is minus its largest PlaneDot). Empty for a round shape. */
		TArray<FPlane> Planes;
		/** A capsule's axis and radius (a sphere's ends are one point). Unused when it has planes. */
		FVector SegmentStart = FVector::ZeroVector;
		FVector SegmentEnd = FVector::ZeroVector;
		float Radius = 0.f;
		/** Points on its surface: a convex's or a box's corners, rings round a capsule, a sphere's poles. */
		TArray<FVector> Points;
		/** A sphere round all of it, for a quick miss. */
		FVector BoundCenter = FVector::ZeroVector;
		float BoundRadius = 0.f;

		bool IsRound() const { return Planes.IsEmpty(); }
	};

	TArray<FShape> Shapes;

	/**
	 * Adds the shapes on Bones in Asset (every body's when Bones is empty), each with its bone's index in Skeleton; a body on
	 * a bone the skeleton lacks is skipped (no skeleton: indices stay INDEX_NONE). Returns how many shapes it added.
	 */
	int32 AddFromPhysicsAsset(const UPhysicsAsset* Asset, const FReferenceSkeleton* Skeleton, TConstArrayView<FName> Bones = {});

	/** Adds a body setup's simple shapes as on Bone (a static mesh's collision has none). Returns how many it added. */
	int32 AddFromBodySetup(const UBodySetup* Setup, FName Bone = NAME_None, int32 BoneIndex = INDEX_NONE);

	/** The indices of the shapes on Bone. */
	TArray<int32> FindShapes(FName Bone) const;

	/**
	 * How deep a point (in the shape's bone space) is inside it: positive inside (cm), negative outside. Outside a convex's
	 * corner the distance is its nearest face's, a little short of the true one; inside, it's exact.
	 */
	static float Depth(const FShape& Shape, const FVector& PointInBone);

	/** Depth, or -Margin at once when the point lies farther than Margin outside the shape's bounding sphere (a quick miss). */
	static float DepthWithin(const FShape& Shape, const FVector& PointInBone, float Margin);
};
