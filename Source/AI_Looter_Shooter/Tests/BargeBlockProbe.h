#pragma once

// What stops a body standing somewhere on the barge, traced in C++ (the user's call, 2026-11-05: "Trace it in C++"; the
// stern push's starboard walk stopped "in Hull" at (1590, 750) though no UCX hull in REPACK's tables reaches there). A failing
// walk's report prints it: the barge tests' own world, capsule and queries, and her Hull's collision as cooked.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/BargeTestWorld.h"
#include "World/BargeBoarding.h"
#include "Chaos/Convex.h"
#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/ConvexElem.h"

namespace BargeTestWorld
{
	/** A world point or way on her, for people: "(x, y, z)" in her frame. */
	inline FString PointOnHer(const ABargeBoarding& Barge, const FVector& World)
	{
		const FVector Local = Barge.GetActorTransform().InverseTransformPosition(World);
		return FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), Local.X, Local.Y, Local.Z);
	}

	inline FString WayOnHer(const ABargeBoarding& Barge, const FVector& World)
	{
		const FVector Local = Barge.GetActorTransform().InverseTransformVector(World);
		return FString::Printf(TEXT("(%.2f, %.2f, %.2f)"), Local.X, Local.Y, Local.Z);
	}

	inline FString BoxText(const FBox& Box)
	{
		return Box.IsValid ? FString::Printf(TEXT("x %.0f..%.0f, y %.0f..%.0f, z %.0f..%.0f"), Box.Min.X, Box.Max.X, Box.Min.Y, Box.Max.Y, Box.Min.Z, Box.Max.Z)
			: FString(TEXT("none"));
	}

	/**
	 * What stops a body of Radius standing at At on her, as RouteOpen looks (her floor under it within reach of Z, then the
	 * capsule from OverFeet over it to Tall), line by line:
	 *  - every overlap that blocks a pawn: its component, owner and body item;
	 *  - the same capsule swept to At from From at that height, and started at At, simple and complex: each hit's component,
	 *    item, shape index in its body (ElementIndex: her Hull's convexes are its shapes 0..N-1, her complex mesh after them),
	 *    face, impact point and normal (her frame);
	 *  - each of her Hull's convex elements the capsule's axis comes within Radius of by its cooked planes (the shapes the
	 *    queries use): its index, the imported points' count and box, the cooked convex's points, planes and box, and how far
	 *    out of it the axis comes (negative: inside).
	 */
	inline TArray<FString> DescribeBlock(const UWorld* World, const ABargeBoarding& Barge, const FVector2D& From, const FVector2D& At, double Z,
		double Radius, double Tall)
	{
		TArray<FString> Lines;
		const FTransform Her = Barge.GetActorTransform();
		double Floor = Z;
		const bool bFloor = FloorUnder(World, Barge, FVector(At, Z + 120.0), Floor);
		Lines.Add(FString::Printf(TEXT("at (%.0f, %.0f): floor %s"), At.X, At.Y,
			bFloor ? *FString::Printf(TEXT("%.1f on %s"), Floor, *FloorWhat(World, Barge, FVector(At, Z + 120.0))) : TEXT("none (the capsule at Z)")));
		const float Half = FMath::Max(static_cast<float>((Tall - OverFeet) * 0.5), static_cast<float>(Radius));
		const FCollisionShape Body = FCollisionShape::MakeCapsule(static_cast<float>(Radius), Half);
		const double MiddleZ = Floor + OverFeet + Half;
		const FVector Middle = Her.TransformPosition(FVector(At, MiddleZ));
		Lines.Add(FString::Printf(TEXT("  capsule radius %.0f, half height %.0f, middle %s (z %.0f..%.0f on her)"), Radius, Half, *PointOnHer(Barge, Middle),
			MiddleZ - Half, MiddleZ + Half));

		// The test's own look (CapsuleClear): every blocking overlap.
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByChannel(Overlaps, Middle, FQuat::Identity, ECC_Pawn, Body);
		int32 Blocking = 0;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			const UPrimitiveComponent* Part = Overlap.GetComponent();
			if (!Overlap.bBlockingHit || !Part)
			{
				continue;
			}
			++Blocking;
			Lines.Add(FString::Printf(TEXT("  overlap (simple, ECC_Pawn): %s.%s, item %d"), *GetNameSafe(Part->GetOwner()), *Part->GetName(), Overlap.ItemIndex));
		}
		if (Blocking == 0)
		{
			Lines.Add(TEXT("  overlap (simple, ECC_Pawn): nothing blocks"));
		}

		// The same body swept in from the last cell reached, and started where it stands: the shapes the hits name.
		const FVector Before = Her.TransformPosition(FVector(From, MiddleZ));
		for (const bool bComplex : { false, true })
		{
			const FCollisionQueryParams Params(SCENE_QUERY_STAT(BargeBlockProbe), bComplex);
			for (const bool bInPlace : { false, true })
			{
				TArray<FHitResult> Hits;
				const FVector Start = bInPlace ? Middle + FVector(0.0, 0.0, 1.0) : Before;
				World->SweepMultiByChannel(Hits, Start, Middle, FQuat::Identity, ECC_Pawn, Body, Params);
				const FString How = FString::Printf(TEXT("%s, %s"), bComplex ? TEXT("complex") : TEXT("simple"),
					bInPlace ? TEXT("started in place") : *FString::Printf(TEXT("swept from (%.0f, %.0f)"), From.X, From.Y));
				if (Hits.IsEmpty())
				{
					Lines.Add(FString::Printf(TEXT("  sweep (%s): no hit"), *How));
				}
				for (const FHitResult& Hit : Hits)
				{
					const UPrimitiveComponent* Part = Hit.GetComponent();
					Lines.Add(FString::Printf(TEXT("  sweep (%s): %s.%s%s%s, item %d, shape %d, face %d, impact %s, normal %s"), *How,
						*GetNameSafe(Part ? Part->GetOwner() : nullptr), *GetNameSafe(Part), Hit.bStartPenetrating ? TEXT(", started in it") : TEXT(""),
						Hit.bBlockingHit ? TEXT(", blocking") : TEXT(""), Hit.Item, static_cast<int32>(Hit.ElementIndex), Hit.FaceIndex,
						*PointOnHer(Barge, Hit.ImpactPoint), *WayOnHer(Barge, Hit.ImpactNormal)));
				}
			}
		}

		// Her Hull's convexes, as cooked: which the capsule meets.
		const UStaticMeshComponent* Hull = Barge.Hull;
		const UStaticMesh* Mesh = Hull ? Hull->GetStaticMesh() : nullptr;
		const UBodySetup* Setup = Mesh ? Mesh->GetBodySetup() : nullptr;
		if (!Setup)
		{
			Lines.Add(TEXT("  her Hull has no body setup"));
			return Lines;
		}
		const FKAggregateGeom& Geometry = Setup->AggGeom;
		Lines.Add(FString::Printf(TEXT("  her Hull (%s): %d convexes, %d boxes, %d spheres, %d capsules; trace flag %d (0 project default, 1 simple and complex, 2 simple as complex, 3 complex as simple)"),
			*Mesh->GetName(), Geometry.ConvexElems.Num(), Geometry.BoxElems.Num(), Geometry.SphereElems.Num(), Geometry.SphylElems.Num(),
			static_cast<int32>(Setup->CollisionTraceFlag.GetValue())));
		TArray<FVector> Axis;
		const double Low = MiddleZ - Half + Radius;
		const double High = MiddleZ + Half - Radius;
		for (int32 Step = 0; Step <= 10; ++Step)
		{
			Axis.Add(Her.TransformPosition(FVector(At, FMath::Lerp(Low, High, Step / 10.0))));
		}
		const FTransform HullToWorld = Hull->GetComponentTransform();
		int32 Met = 0;
		for (int32 Index = 0; Index < Geometry.ConvexElems.Num(); ++Index)
		{
			const FKConvexElem& Element = Geometry.ConvexElems[Index];
			const FTransform ElementToWorld = Element.GetTransform() * HullToWorld;
			const Chaos::FConvexPtr& Cooked = Element.GetChaosConvexMesh();
			if (!Cooked.IsValid() || Cooked->NumPlanes() == 0)
			{
				continue;
			}
			double Nearest = TNumericLimits<double>::Max();
			double NearestZ = 0.0;
			for (const FVector& Point : Axis)
			{
				const FVector Local = ElementToWorld.InverseTransformPosition(Point);
				double Phi = TNumericLimits<double>::Lowest();
				for (int32 Plane = 0; Plane < Cooked->NumPlanes(); ++Plane)
				{
					Chaos::FVec3 Normal;
					Chaos::FVec3 OnPlane;
					Cooked->GetPlaneNX(Plane, Normal, OnPlane);
					Phi = FMath::Max(Phi, FVector::DotProduct(Local - FVector(OnPlane), FVector(Normal)));
				}
				if (Phi < Nearest)
				{
					Nearest = Phi;
					NearestZ = Her.InverseTransformPosition(Point).Z;
				}
			}
			// The planes' distance is no more than the true one: a corner may show near when it's a little farther.
			if (Nearest >= Radius)
			{
				continue;
			}
			++Met;
			FBox Imported(ForceInit);
			for (const FVector& Point : Element.VertexData)
			{
				Imported += Her.InverseTransformPosition(ElementToWorld.TransformPosition(Point));
			}
			FBox CookedBox(ForceInit);
			for (int32 Point = 0; Point < Cooked->NumVertices(); ++Point)
			{
				CookedBox += Her.InverseTransformPosition(ElementToWorld.TransformPosition(FVector(Cooked->GetVertex(Point))));
			}
			Lines.Add(FString::Printf(TEXT("  convex %d%s: axis %.1f cm out of it at z %.0f; imported %d points, %d triangles, %s; cooked %d points, %d planes, %s"),
				Index, Element.GetName().IsNone() ? TEXT("") : *FString::Printf(TEXT(" \"%s\""), *Element.GetName().ToString()), Nearest, NearestZ,
				Element.VertexData.Num(), Element.IndexData.Num() / 3, *BoxText(Imported), Cooked->NumVertices(), Cooked->NumPlanes(), *BoxText(CookedBox)));
		}
		if (Met == 0)
		{
			Lines.Add(FString::Printf(TEXT("  no convex of hers within %.0f cm of the capsule's axis by its cooked planes"), Radius));
		}
		return Lines;
	}
}

#endif
