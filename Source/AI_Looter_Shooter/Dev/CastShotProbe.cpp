// Looter.CastShots' numbers (CastShotProbe.h): ground gaps, limbs in bodies, bodies in each other and in the level, feet
// sliding and pops, measured on what the game shows. Developer builds only.

#include "Dev/CastShotProbe.h"
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Creatures/CreatureBodyHulls.h"
#include "World/WorldQueries.h"
#include "AnimationRuntime.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "ReferenceSkeleton.h"

namespace
{
	using FShape = FCreatureBodyHulls::FShape;

	/** A point is inside the level when every way out within this far (cm) meets a surface. */
	constexpr float WayOutLength = 30.f;
	/** At most this many points of a body are tried against the level (its hulls' corners, thinned evenly). */
	constexpr int32 MaxLevelPoints = 600;
	/** Against the part a limb hangs from, its hull's points nearer its joint than this share of its reach are left out. */
	constexpr float JointShare = 0.3f;
	/** A body that moves farther than this in one frame (cm) was put there (a phase-step, a respawn): the pop watch starts over. */
	constexpr float TeleportDistance = 150.f;

	const FReferenceSkeleton* SkeletonOf(const USkinnedMeshComponent& Mesh)
	{
		const USkinnedAsset* Asset = Mesh.GetSkinnedAsset();
		return Asset ? &Asset->GetRefSkeleton() : nullptr;
	}

	FTransform PosedInComponent(const USkinnedMeshComponent& Mesh, int32 Bone)
	{
		return Mesh.GetBoneTransform(Bone, FTransform::Identity);
	}

	FString PartName(const FShape& Shape)
	{
		return Shape.Bone.IsNone() ? FString(TEXT("body")) : Shape.Bone.ToString();
	}

	/** The fourteen ways out of a point: along the axes and the cube's diagonals. */
	const TArray<FVector>& WaysOut()
	{
		static const TArray<FVector> Ways = []()
		{
			TArray<FVector> Made = { FVector::ForwardVector, -FVector::ForwardVector, FVector::RightVector, -FVector::RightVector,
				FVector::UpVector, -FVector::UpVector };
			for (int32 Corner = 0; Corner < 8; ++Corner)
			{
				Made.Add(FVector((Corner & 1) ? 1.0 : -1.0, (Corner & 2) ? 1.0 : -1.0, (Corner & 4) ? 1.0 : -1.0).GetSafeNormal());
			}
			return Made;
		}();
		return Ways;
	}
}

bool CastShotProbe::GroundUnder(const UWorld& World, const FVector& Point, float Above, float Below, const AActor* Ignored, FVector& OutGround)
{
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("CastShotGround"), Ignored);
	const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	FHitResult Hit;
	if (!World.LineTraceSingleByObjectType(Hit, Point + FVector(0.0, 0.0, Above), Point - FVector(0.0, 0.0, Below), Objects, Params))
	{
		return false;
	}
	OutGround = Hit.ImpactPoint;
	if (OutGround.Z <= Point.Z + 0.5)
	{
		return true;
	}
	// A surface over the point: it's sunk in the ground, or it hangs under something in open air. Looking up from it meets an
	// underside (a face turned down) only in the open: inside the solid a look up starts in it or meets nothing. (Sitting on
	// a fence, Amos's coat tail between the rails read as 36 cm under the ground: the ground the trace found was the top rail.)
	FHitResult Over;
	const bool bUnderside = World.LineTraceSingleByObjectType(Over, Point, OutGround + FVector(0.0, 0.0, 1.0), Objects, Params)
		&& !Over.bStartPenetrating && Over.ImpactNormal.Z < -0.3;
	if (!bUnderside)
	{
		return true;
	}
	FHitResult Floor;
	if (World.LineTraceSingleByObjectType(Floor, Point, Point - FVector(0.0, 0.0, Below), Objects, Params) && !Floor.bStartPenetrating)
	{
		OutGround = Floor.ImpactPoint;
		return true;
	}
	return false;
}

CastShotProbe::FGroundGap CastShotProbe::PointsAgainstGround(const UWorld& World, TConstArrayView<FVector> Points,
	TConstArrayView<FString> Names, const AActor* Ignored)
{
	FGroundGap Gap;
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		FVector Ground;
		if (!GroundUnder(World, Points[Index], 150.f, 300.f, Ignored, Ground))
		{
			continue;
		}
		++Gap.Measured;
		const float Height = static_cast<float>(Points[Index].Z - Ground.Z);
		const FString& Name = Names.IsValidIndex(Index) ? Names[Index] : FString();
		if (-Height > Gap.Under)
		{
			Gap.Under = -Height;
			Gap.UnderPart = Name;
		}
		if (Height > Gap.Over)
		{
			Gap.Over = Height;
			Gap.OverPart = Name;
		}
	}
	return Gap;
}

CastShotProbe::FGroundGap CastShotProbe::BonesAgainstGround(const UWorld& World, const USkinnedMeshComponent& Mesh,
	TConstArrayView<FName> Bones, const AActor* Ignored)
{
	TArray<FVector> Points;
	TArray<FString> Names;
	for (const FName& Bone : Bones)
	{
		const int32 Index = Mesh.GetBoneIndex(Bone);
		if (Index != INDEX_NONE)
		{
			Points.Add(Mesh.GetBoneTransform(Index).GetLocation());
			Names.Add(Bone.ToString());
		}
	}
	return PointsAgainstGround(World, Points, Names, Ignored);
}

CastShotProbe::FIntrusion CastShotProbe::LimbsIntoBody(const USkinnedMeshComponent& Mesh, TConstArrayView<FName> Limbs,
	TConstArrayView<FName> BodyParts)
{
	FIntrusion Result;
	const FReferenceSkeleton* Skeleton = SkeletonOf(Mesh);
	const UPhysicsAsset* Asset = Mesh.GetPhysicsAsset();
	if (!Skeleton || !Asset)
	{
		return Result;
	}
	FCreatureBodyHulls LimbHulls;
	FCreatureBodyHulls BodyHulls;
	LimbHulls.AddFromPhysicsAsset(Asset, Skeleton, Limbs);
	BodyHulls.AddFromPhysicsAsset(Asset, Skeleton, BodyParts);
	TSet<int32> BodyBones;
	for (const FShape& Body : BodyHulls.Shapes)
	{
		BodyBones.Add(Body.BoneIndex);
	}
	for (const FShape& Limb : LimbHulls.Shapes)
	{
		const FTransform LimbPosed = PosedInComponent(Mesh, Limb.BoneIndex);
		const FTransform LimbRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(*Skeleton, Limb.BoneIndex);
		// The body part it hangs from (its nearest one up the skeleton), and how far its hull reaches from its joint.
		int32 HangsFrom = Skeleton->GetParentIndex(Limb.BoneIndex);
		while (HangsFrom != INDEX_NONE && !BodyBones.Contains(HangsFrom))
		{
			HangsFrom = Skeleton->GetParentIndex(HangsFrom);
		}
		float LimbReach = 0.f;
		for (const FVector& Point : Limb.Points)
		{
			LimbReach = FMath::Max(LimbReach, static_cast<float>(Point.Size()));
		}
		for (const FShape& Body : BodyHulls.Shapes)
		{
			if (Body.BoneIndex == Limb.BoneIndex)
			{
				continue;
			}
			const FTransform BodyPosed = PosedInComponent(Mesh, Body.BoneIndex);
			const FTransform BodyRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(*Skeleton, Body.BoneIndex);
			const float JointZone = Body.BoneIndex == HangsFrom ? JointShare * LimbReach : 0.f;
			for (const FVector& Point : Limb.Points)
			{
				if (Point.Size() < JointZone)
				{
					continue;
				}
				const float Now = FCreatureBodyHulls::DepthWithin(Body, BodyPosed.InverseTransformPosition(LimbPosed.TransformPosition(Point)), 0.f);
				if (Now <= Result.Depth)
				{
					continue;
				}
				// Only what goes deeper than the rest pose counts: neighbouring parts' hulls overlap at their joints by design.
				const float AtRest = FCreatureBodyHulls::Depth(Body, BodyRest.InverseTransformPosition(LimbRest.TransformPosition(Point)));
				const float Past = Now - FMath::Max(AtRest, 0.f);
				if (Past > Result.Depth)
				{
					Result.Depth = Past;
					Result.Part = PartName(Limb);
					Result.Into = PartName(Body);
				}
			}
		}
	}
	// In world centimetres: a creature's size scales its component.
	Result.Depth *= static_cast<float>(Mesh.GetComponentTransform().GetMaximumAxisScale());
	return Result;
}

CastShotProbe::FIntrusion CastShotProbe::BodyIntoBody(const USkinnedMeshComponent& Mesh, const USkinnedMeshComponent& Other)
{
	FIntrusion Result;
	if (!Mesh.Bounds.GetBox().Intersect(Other.Bounds.GetBox()))
	{
		return Result;
	}
	const FReferenceSkeleton* MeshSkeleton = SkeletonOf(Mesh);
	const FReferenceSkeleton* OtherSkeleton = SkeletonOf(Other);
	FCreatureBodyHulls MeshHulls;
	FCreatureBodyHulls OtherHulls;
	MeshHulls.AddFromPhysicsAsset(Mesh.GetPhysicsAsset(), MeshSkeleton);
	OtherHulls.AddFromPhysicsAsset(Other.GetPhysicsAsset(), OtherSkeleton);
	for (const FShape& Mine : MeshHulls.Shapes)
	{
		const FTransform MineInWorld = Mesh.GetBoneTransform(Mine.BoneIndex);
		for (const FShape& Theirs : OtherHulls.Shapes)
		{
			const FTransform TheirsInWorld = Other.GetBoneTransform(Theirs.BoneIndex);
			const float Scale = static_cast<float>(TheirsInWorld.GetMaximumAxisScale());
			for (const FVector& Point : Mine.Points)
			{
				const float Depth = Scale * FCreatureBodyHulls::DepthWithin(Theirs, TheirsInWorld.InverseTransformPosition(MineInWorld.TransformPosition(Point)), 0.f);
				if (Depth > Result.Depth)
				{
					Result.Depth = Depth;
					Result.Part = PartName(Mine);
					Result.Into = PartName(Theirs);
				}
			}
		}
	}
	return Result;
}

CastShotProbe::FLevelHit CastShotProbe::AgainstLevel(const UWorld& World, const UPrimitiveComponent& Body, const AActor* Ignored)
{
	// Its points: the hit hulls' corners as posed (a static mesh's simple collision), or its bones without hulls.
	TArray<FVector> Points;
	TArray<FString> Names;
	FCreatureBodyHulls Hulls;
	TArray<FTransform> HullToWorld;
	if (const USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(&Body))
	{
		Hulls.AddFromPhysicsAsset(Skinned->GetPhysicsAsset(), SkeletonOf(*Skinned));
		for (const FShape& Shape : Hulls.Shapes)
		{
			HullToWorld.Add(Skinned->GetBoneTransform(Shape.BoneIndex));
		}
		if (Hulls.Shapes.IsEmpty())
		{
			for (int32 Bone = 0; Bone < Skinned->GetNumBones(); ++Bone)
			{
				Points.Add(Skinned->GetBoneTransform(Bone).GetLocation());
				Names.Add(Skinned->GetBoneName(Bone).ToString());
			}
		}
	}
	else if (const UStaticMeshComponent* Static = Cast<UStaticMeshComponent>(&Body))
	{
		const UStaticMesh* Model = Static->GetStaticMesh();
		Hulls.AddFromBodySetup(Model ? Model->GetBodySetup() : nullptr);
		for (int32 Shape = 0; Shape < Hulls.Shapes.Num(); ++Shape)
		{
			HullToWorld.Add(Static->GetComponentTransform());
		}
	}
	int32 Corners = 0;
	for (const FShape& Shape : Hulls.Shapes)
	{
		Corners += Shape.Points.Num();
	}
	const int32 Stride = FMath::Max(1, FMath::DivideAndRoundUp(Corners, MaxLevelPoints));
	int32 Counter = 0;
	for (int32 Index = 0; Index < Hulls.Shapes.Num(); ++Index)
	{
		for (const FVector& Point : Hulls.Shapes[Index].Points)
		{
			if (Counter++ % Stride == 0)
			{
				Points.Add(HullToWorld[Index].TransformPosition(Point));
				Names.Add(PartName(Hulls.Shapes[Index]));
			}
		}
	}

	FLevelHit Result;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("CastShotLevel"), Ignored);
	const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		++Result.Measured;
		const FVector& Point = Points[Index];
		float Shortest = TNumericLimits<float>::Max();
		FString Into;
		bool bWayOut = false;
		for (const FVector& Way : WaysOut())
		{
			FHitResult Hit;
			if (!World.LineTraceSingleByObjectType(Hit, Point + Way * WayOutLength, Point, Objects, Params))
			{
				bWayOut = true;
				break;
			}
			const float Depth = Hit.bStartPenetrating ? WayOutLength : WayOutLength - static_cast<float>(Hit.Distance);
			if (Depth < Shortest)
			{
				Shortest = Depth;
				Into = Hit.GetActor() ? Hit.GetActor()->GetActorNameOrLabel() : GetNameSafe(Hit.GetComponent());
			}
		}
		if (bWayOut || Shortest < 1.f)
		{
			continue;
		}
		++Result.Inside;
		if (Shortest > Result.Depth)
		{
			Result.Depth = Shortest;
			Result.Part = Names[Index];
			Result.Into = Into;
		}
	}
	return Result;
}

void CastShotProbe::FFootSlide::Sample(const UWorld& World, const USkinnedMeshComponent& Mesh, TConstArrayView<FName> Feet,
	float PlantedHeight, const AActor* Ignored, float DeltaSeconds)
{
	for (const FName& Foot : Feet)
	{
		const int32 Index = Mesh.GetBoneIndex(Foot);
		if (Index == INDEX_NONE)
		{
			continue;
		}
		const FVector Here = Mesh.GetBoneTransform(Index).GetLocation();
		FVector Ground;
		const bool bDown = GroundUnder(World, Here, 60.f, 120.f, Ignored, Ground) && Here.Z - Ground.Z < PlantedHeight;
		// Planted once it has been down a moment: a landing step's last frames brush the ground still moving.
		float& DownFor = PlantedFor.FindOrAdd(Foot);
		const bool bPlanted = bDown && DownFor >= MinPlantedSeconds;
		DownFor = bDown ? DownFor + DeltaSeconds : 0.f;
		const FVector* Before = Last.Find(Foot);
		const bool* bWasDown = WasPlanted.Find(Foot);
		if (Before && bWasDown && *bWasDown && bPlanted && DeltaSeconds > 0.001f)
		{
			const float Moved = static_cast<float>(FVector::Dist2D(Here, *Before));
			SlideDistance += Moved;
			PlantedSeconds += DeltaSeconds;
			if (Moved / DeltaSeconds > Worst)
			{
				Worst = Moved / DeltaSeconds;
				WorstName = Foot.ToString();
			}
		}
		Last.Add(Foot, Here);
		WasPlanted.Add(Foot, bPlanted);
	}
}

void CastShotProbe::FPopWatch::Sample(const USkinnedMeshComponent& Mesh, float DeltaSeconds)
{
	const TArray<FTransform>& Pose = Mesh.GetComponentSpaceTransforms();
	const FTransform& ToWorld = Mesh.GetComponentTransform();
	const FVector Body = ToWorld.GetLocation();
	// Frozen frames and hitches say nothing about pops, and a body put somewhere new starts over.
	const bool bStep = DeltaSeconds > 0.001f && DeltaSeconds < 0.1f && Last.Num() == Pose.Num()
		&& FVector::Dist(Body, LastBody) < TeleportDistance;
	const bool bMeasure = bStep && LastVelocity.Num() == Pose.Num();
	TArray<FVector> Velocity;
	Velocity.SetNumZeroed(bStep ? Pose.Num() : 0);
	Last.SetNum(Pose.Num());
	for (int32 Bone = 0; Bone < Pose.Num(); ++Bone)
	{
		const FVector Here = ToWorld.TransformPosition(Pose[Bone].GetLocation());
		if (bStep)
		{
			Velocity[Bone] = (Here - Last[Bone]) / DeltaSeconds;
			if (bMeasure)
			{
				// A one-frame jump of D cm changes the velocity by D / DeltaSeconds, read as cm per frame at 60 a second (the
				// same scale the old measure of jumps against the body had).
				const float Change = static_cast<float>(FVector::Dist(Velocity[Bone], LastVelocity[Bone])) / 60.f;
				if (Change > Worst)
				{
					Worst = Change;
					WorstBone = Mesh.GetBoneName(Bone).ToString();
				}
			}
		}
		Last[Bone] = Here;
	}
	LastVelocity = MoveTemp(Velocity);
	LastBody = Body;
}

#endif
