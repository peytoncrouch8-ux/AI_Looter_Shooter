#include "Creatures/CreatureBodyHulls.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "ReferenceSkeleton.h"

namespace
{
	using FShape = FCreatureBodyHulls::FShape;

	/** The bounding sphere: round the points' middle, out to the farthest one (a round shape's radius on top). */
	void FinishBounds(FShape& Shape)
	{
		FVector Sum = FVector::ZeroVector;
		for (const FVector& Point : Shape.Points)
		{
			Sum += Point;
		}
		Shape.BoundCenter = Shape.Points.IsEmpty() ? FVector::ZeroVector : Sum / Shape.Points.Num();
		float Farthest = 0.f;
		for (const FVector& Point : Shape.Points)
		{
			Farthest = FMath::Max(Farthest, static_cast<float>(FVector::Dist(Point, Shape.BoundCenter)));
		}
		Shape.BoundRadius = Farthest + 1.f;
	}

	/** A face of a shape given in its element's frame, moved into the bone's space. */
	FPlane PlaneInBone(const FTransform& Element, const FVector& PointOnFace, const FVector& Normal)
	{
		return FPlane(Element.TransformPosition(PointOnFace), Element.TransformVectorNoScale(Normal).GetSafeNormal());
	}

	/** A box of half-extents Half in its element's frame: six faces and eight corners. */
	FShape MakeBox(const FTransform& Element, const FVector& Half)
	{
		FShape Shape;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			for (const float Sign : { -1.f, 1.f })
			{
				FVector Normal = FVector::ZeroVector;
				Normal[Axis] = Sign;
				Shape.Planes.Add(PlaneInBone(Element, Normal * Half[Axis], Normal));
			}
		}
		for (int32 Corner = 0; Corner < 8; ++Corner)
		{
			const FVector Local((Corner & 1) ? Half.X : -Half.X, (Corner & 2) ? Half.Y : -Half.Y, (Corner & 4) ? Half.Z : -Half.Z);
			Shape.Points.Add(Element.TransformPosition(Local));
		}
		FinishBounds(Shape);
		return Shape;
	}

	/** A capsule along its element's Z, HalfLength each way from its middle: the axis, and rings at its ends and middle. */
	FShape MakeCapsule(const FTransform& Element, float HalfLength, float Radius)
	{
		FShape Shape;
		Shape.SegmentStart = Element.TransformPosition(FVector(0.0, 0.0, -HalfLength));
		Shape.SegmentEnd = Element.TransformPosition(FVector(0.0, 0.0, HalfLength));
		Shape.Radius = Radius;
		constexpr int32 RingPoints = 8;
		for (const float Along : { -HalfLength, 0.f, HalfLength })
		{
			for (int32 Step = 0; Step < RingPoints; ++Step)
			{
				const float Angle = 2.f * UE_PI * Step / RingPoints;
				Shape.Points.Add(Element.TransformPosition(FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Along)));
			}
		}
		Shape.Points.Add(Element.TransformPosition(FVector(0.0, 0.0, -HalfLength - Radius)));
		Shape.Points.Add(Element.TransformPosition(FVector(0.0, 0.0, HalfLength + Radius)));
		FinishBounds(Shape);
		return Shape;
	}

	FShape MakeConvex(const FKConvexElem& Convex)
	{
		const FTransform Element = Convex.GetTransform();
		TArray<FPlane> LocalPlanes;
		Convex.GetPlanes(LocalPlanes);
		if (LocalPlanes.IsEmpty())
		{
			// No cooked hull to read its faces from: its corners' box stands in for it.
			const FBox Box = Convex.ElemBox.IsValid ? Convex.ElemBox : FBox(Convex.VertexData);
			return MakeBox(FTransform(Box.GetCenter()) * Element, Box.GetExtent());
		}
		FShape Shape;
		for (const FPlane& Plane : LocalPlanes)
		{
			const FVector Normal = Plane.GetSafeNormal();
			Shape.Planes.Add(PlaneInBone(Element, Normal * Plane.W, Normal));
		}
		for (const FVector& Vertex : Convex.VertexData)
		{
			Shape.Points.Add(Element.TransformPosition(Vertex));
		}
		FinishBounds(Shape);
		return Shape;
	}
}

int32 FCreatureBodyHulls::AddFromPhysicsAsset(const UPhysicsAsset* Asset, const FReferenceSkeleton* Skeleton, TConstArrayView<FName> Bones)
{
	if (!Asset)
	{
		return 0;
	}
	int32 Added = 0;
	for (const USkeletalBodySetup* Setup : Asset->SkeletalBodySetups)
	{
		if (!Setup || (!Bones.IsEmpty() && !Bones.Contains(Setup->BoneName)))
		{
			continue;
		}
		const int32 Index = Skeleton ? Skeleton->FindBoneIndex(Setup->BoneName) : INDEX_NONE;
		if (Skeleton && Index == INDEX_NONE)
		{
			continue;
		}
		Added += AddFromBodySetup(Setup, Setup->BoneName, Index);
	}
	return Added;
}

int32 FCreatureBodyHulls::AddFromBodySetup(const UBodySetup* Setup, FName Bone, int32 BoneIndex)
{
	if (!Setup)
	{
		return 0;
	}
	const int32 Before = Shapes.Num();
	const FKAggregateGeom& Geometry = Setup->AggGeom;
	// A hull's faces come from its physics mesh; a body whose meshes aren't made yet (its component's collision is off)
	// makes them now, or its hulls would be measured as their corners' boxes.
	if (Geometry.ConvexElems.ContainsByPredicate([](const FKConvexElem& Each) { return !Each.GetChaosConvexMesh().IsValid(); }))
	{
		const_cast<UBodySetup*>(Setup)->CreatePhysicsMeshes();
	}
	for (const FKConvexElem& Convex : Geometry.ConvexElems)
	{
		Shapes.Add(MakeConvex(Convex));
	}
	for (const FKBoxElem& Box : Geometry.BoxElems)
	{
		Shapes.Add(MakeBox(Box.GetTransform(), FVector(Box.X, Box.Y, Box.Z) * 0.5));
	}
	for (const FKSphylElem& Capsule : Geometry.SphylElems)
	{
		Shapes.Add(MakeCapsule(Capsule.GetTransform(), Capsule.Length * 0.5f, Capsule.Radius));
	}
	for (const FKTaperedCapsuleElem& Tapered : Geometry.TaperedCapsuleElems)
	{
		// Measured as its wider end all along: a little generous at the narrow end.
		Shapes.Add(MakeCapsule(Tapered.GetTransform(), Tapered.Length * 0.5f, FMath::Max(Tapered.Radius0, Tapered.Radius1)));
	}
	for (const FKSphereElem& Sphere : Geometry.SphereElems)
	{
		Shapes.Add(MakeCapsule(Sphere.GetTransform(), 0.f, Sphere.Radius));
	}
	for (int32 Index = Before; Index < Shapes.Num(); ++Index)
	{
		Shapes[Index].Bone = Bone;
		Shapes[Index].BoneIndex = BoneIndex;
	}
	return Shapes.Num() - Before;
}

TArray<int32> FCreatureBodyHulls::FindShapes(FName Bone) const
{
	TArray<int32> Found;
	for (int32 Index = 0; Index < Shapes.Num(); ++Index)
	{
		if (Shapes[Index].Bone == Bone)
		{
			Found.Add(Index);
		}
	}
	return Found;
}

float FCreatureBodyHulls::Depth(const FShape& Shape, const FVector& PointInBone)
{
	if (Shape.IsRound())
	{
		return Shape.Radius - static_cast<float>(FMath::PointDistToSegment(PointInBone, Shape.SegmentStart, Shape.SegmentEnd));
	}
	float Outside = -TNumericLimits<float>::Max();
	for (const FPlane& Plane : Shape.Planes)
	{
		Outside = FMath::Max(Outside, static_cast<float>(Plane.PlaneDot(PointInBone)));
	}
	return -Outside;
}

float FCreatureBodyHulls::DepthWithin(const FShape& Shape, const FVector& PointInBone, float Margin)
{
	if (FVector::DistSquared(PointInBone, Shape.BoundCenter) > FMath::Square(Shape.BoundRadius + Margin))
	{
		return -Margin;
	}
	return Depth(Shape, PointInBone);
}
