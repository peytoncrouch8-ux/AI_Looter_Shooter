#include "PropSettler.h"
#include "World/MinimapSubsystem.h"
#include "World/WorldQueries.h"
#include "Components/StaticMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GeometryScript/MeshAssetFunctions.h"
#include "ScopedTransaction.h"
#include "UDynamicMesh.h"

namespace
{
	/** How far above or below its base the ground may be for a prop to count as standing on it (cm). */
	constexpr double Reach = 600.0;

	/** The share of a prop's base that needs ground under it; less, and it's hanging over an edge or up in the sky. */
	constexpr float GroundedShare = 0.75f;

	/**
	 * The tilt of the plane that best fits the ground under the base (least squares, z = A x + B y + C). The small ridge
	 * term keeps a base that is nearly a line from reading a tilt across itself out of noise.
	 */
	FVector FitGroundNormal(TConstArrayView<FVector> Points, TConstArrayView<double> Heights)
	{
		FVector2D Center = FVector2D::ZeroVector;
		double MeanHeight = 0.0;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			Center += FVector2D(Points[Index]);
			MeanHeight += Heights[Index];
		}
		Center /= Points.Num();
		MeanHeight /= Points.Num();

		double Sxx = 0.0, Sxy = 0.0, Syy = 0.0, Sxz = 0.0, Syz = 0.0;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const double X = Points[Index].X - Center.X;
			const double Y = Points[Index].Y - Center.Y;
			const double Z = Heights[Index] - MeanHeight;
			Sxx += X * X;
			Sxy += X * Y;
			Syy += Y * Y;
			Sxz += X * Z;
			Syz += Y * Z;
		}
		const double Ridge = 1e-3 * (Sxx + Syy) + 1.0;
		Sxx += Ridge;
		Syy += Ridge;
		const double Determinant = Sxx * Syy - Sxy * Sxy;
		if (Determinant <= UE_DOUBLE_SMALL_NUMBER)
		{
			return FVector::UpVector;
		}
		const double A = (Sxz * Syy - Syz * Sxy) / Determinant;
		const double B = (Syz * Sxx - Sxz * Sxy) / Determinant;
		return FVector(-A, -B, 1.0).GetSafeNormal();
	}
}

PropSettler::FRules PropSettler::RulesFor(double Height, double Width)
{
	FRules Rules;
	// A tree or a pillar looks wrong the moment it leans; a wall, a rock or a row of stones looks wrong when it doesn't.
	Rules.MaxTiltDegrees = Height >= Width * 1.2 ? 0.f : 15.f;
	// A little under the ground, deeper for big things (a tree's roots, a wall's footing).
	Rules.Embed = static_cast<float>(FMath::Clamp(Height * 0.04, 2.0, 12.0));
	return Rules;
}

PropSettler::FFootprint PropSettler::FootprintOf(UStaticMesh& Mesh)
{
	FFootprint Footprint;
	UDynamicMesh* Scratch = NewObject<UDynamicMesh>(GetTransientPackage(), NAME_None, RF_Transient);
	FGeometryScriptMeshReadLOD Source;
	Source.LODType = EGeometryScriptLODType::SourceModel;
	EGeometryScriptOutcomePins Outcome = EGeometryScriptOutcomePins::Failure;
	UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshFromStaticMesh(&Mesh, Scratch, FGeometryScriptCopyMeshFromAssetOptions(), Source, Outcome);
	if (Outcome != EGeometryScriptOutcomePins::Success)
	{
		return Footprint;
	}

	TArray<FVector> Vertices;
	Scratch->ProcessMesh([&Vertices](const UE::Geometry::FDynamicMesh3& Geometry)
	{
		for (const int32 Vertex : Geometry.VertexIndicesItr())
		{
			Vertices.Add(Geometry.GetVertex(Vertex));
		}
	});
	for (const FVector& Vertex : Vertices)
	{
		Footprint.Bounds += Vertex;
	}
	if (!Footprint.Bounds.IsValid)
	{
		return Footprint;
	}

	// The lowest band is what meets the ground: a few centimeters, more on big props (a boulder's rounded bottom).
	const double Band = FMath::Max(10.0, Footprint.Bounds.GetSize().Z * 0.08);
	for (const FVector& Vertex : Vertices)
	{
		if (Vertex.Z <= Footprint.Bounds.Min.Z + Band)
		{
			Footprint.Base.Add(Vertex);
		}
	}
	return Footprint;
}

TOptional<FTransform> PropSettler::Settle(const FTransform& Placement, TConstArrayView<FVector> Base, const FRules& Rules,
	TFunctionRef<TOptional<double>(const FVector&)> GroundHeight)
{
	if (Base.IsEmpty())
	{
		return {};
	}

	// Start from the prop standing upright on its own yaw (undoing a lean from an earlier settle exactly), so settling a
	// settled prop gives the same answer.
	const FQuat Rotation = Placement.GetRotation();
	const FQuat Upright = FQuat::FindBetweenNormals(Rotation.GetUpVector(), FVector::UpVector) * Rotation;
	FTransform Seated(Upright, Placement.GetLocation(), Placement.GetScale3D());

	TArray<FVector> Points;
	TArray<double> Heights;
	auto SampleGround = [&]()
	{
		Points.Reset();
		Heights.Reset();
		for (const FVector& Local : Base)
		{
			const FVector Point = Seated.TransformPosition(Local);
			if (const TOptional<double> Height = GroundHeight(Point))
			{
				Points.Add(Point);
				Heights.Add(*Height);
			}
		}
		return Points.Num() >= FMath::CeilToInt(Base.Num() * GroundedShare);
	};
	if (!SampleGround())
	{
		return {};
	}

	if (Rules.MaxTiltDegrees > 0.f)
	{
		FVector Normal = FitGroundNormal(Points, Heights);
		const double MaxTilt = FMath::DegreesToRadians(Rules.MaxTiltDegrees);
		if (FMath::Acos(FMath::Clamp(Normal.Z, -1.0, 1.0)) > MaxTilt)
		{
			const FVector Axis = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
			Normal = FQuat(Axis, MaxTilt).RotateVector(FVector::UpVector);
		}
		Seated.SetRotation(FQuat::FindBetweenNormals(FVector::UpVector, Normal) * Upright);
		if (!SampleGround())
		{
			return {};
		}
	}

	// Down (or up) until the base point highest above the ground is Embed under it.
	double Highest = -UE_BIG_NUMBER;
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		Highest = FMath::Max(Highest, Points[Index].Z - Heights[Index]);
	}
	Seated.AddToTranslation(FVector(0.0, 0.0, -(Highest + Rules.Embed)));
	return Seated;
}

int32 PropSettler::SettleWorld(UWorld* World, bool bSelectedOnly)
{
	if (!World)
	{
		return 0;
	}

	// Only terrain counts as ground, never another prop (or the meadow's volume, which StaticGeometryParams skips).
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("PropSettle"));
	auto GroundHeight = [World, &Params](const FVector& Point) -> TOptional<double>
	{
		TArray<FHitResult> Hits;
		World->LineTraceMultiByObjectType(Hits, Point + FVector(0.0, 0.0, Reach), Point - FVector(0.0, 0.0, Reach),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		for (const FHitResult& Hit : Hits)
		{
			const AActor* Actor = Hit.GetActor();
			if (Actor && Actor->ActorHasTag(MinimapTags::Ground))
			{
				return Hit.ImpactPoint.Z;
			}
		}
		return {};
	};

	const FScopedTransaction Transaction(NSLOCTEXT("LooterEditor", "SettleProps", "Settle Props on the Ground"));
	TMap<UStaticMesh*, FFootprint> Footprints;
	int32 Moved = 0;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		AStaticMeshActor* Actor = *It;
		const UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
		UStaticMesh* Mesh = Component ? Component->GetStaticMesh() : nullptr;
		// The terrain is what props stand on, and a beam or a light rides along with the prop it's attached to.
		if (!Mesh || Actor->ActorHasTag(MinimapTags::Ground) || Actor->GetAttachParentActor() || (bSelectedOnly && !Actor->IsSelected()))
		{
			continue;
		}
		if (!Footprints.Contains(Mesh))
		{
			Footprints.Add(Mesh, FootprintOf(*Mesh));
		}
		const FFootprint& Footprint = Footprints[Mesh];
		const FVector Size = Footprint.Bounds.GetSize() * Actor->GetActorScale3D().GetAbs();
		const FTransform Placement = Actor->GetActorTransform();
		const TOptional<FTransform> Seated = Settle(Placement, Footprint.Base, RulesFor(Size.Z, FMath::Min(Size.X, Size.Y)), GroundHeight);
		if (!Seated || (Seated->GetTranslation().Equals(Placement.GetTranslation(), 0.05) && Seated->GetRotation().Equals(Placement.GetRotation(), 1e-5)))
		{
			continue;
		}
		Actor->Modify();
		Actor->SetActorTransform(*Seated);
		++Moved;
	}
	return Moved;
}
