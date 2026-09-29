#pragma once

#include "CoreMinimal.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"

class UDynamicMesh;
namespace UE::Geometry { class FDynamicMesh3; }

/**
 * Thin helpers over GeometryScript for building chunky, hand-made-looking props from code.
 * All sizes are in cm. "Slot" is the material slot a part is painted with.
 */
namespace StylizedMesh
{
	AI_LOOTER_SHOOTER_API FGeometryScriptPrimitiveOptions Slot(int32 MaterialID);

	AI_LOOTER_SHOOTER_API int32 VertexCount(const UDynamicMesh* Mesh);

	/**
	 * Smooth 3D noise offset for every vertex added since FirstVertex. Uses positions, so welded and
	 * unwelded seams move together and the mesh never tears.
	 */
	AI_LOOTER_SHOOTER_API void Displace(UDynamicMesh* Mesh, int32 FirstVertex, float Magnitude, float Frequency, float SeedOffset);

	/** Squashes vertices added since FirstVertex below LocalZ flat onto it (sinks rocks into the ground). */
	AI_LOOTER_SHOOTER_API void FlattenBelow(UDynamicMesh* Mesh, int32 FirstVertex, float LocalZ);

	/** Center-origin box. */
	AI_LOOTER_SHOOTER_API void Box(UDynamicMesh* Mesh, int32 MaterialID, const FVector& Center, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator, int32 Steps = 0);

	/** Base-origin cylinder along the transform's Z. */
	AI_LOOTER_SHOOTER_API void Cylinder(UDynamicMesh* Mesh, int32 MaterialID, const FTransform& Transform, float Radius, float Height, int32 Sides, bool bCapped = true);

	/** Base-origin cone/frustum along the transform's Z. */
	AI_LOOTER_SHOOTER_API void Cone(UDynamicMesh* Mesh, int32 MaterialID, const FTransform& Transform, float BaseRadius, float TopRadius, float Height, int32 Sides, int32 HeightSteps = 0);

	/** Faceted round blob. Steps 0-2 keep it low-poly. */
	AI_LOOTER_SHOOTER_API void Blob(UDynamicMesh* Mesh, int32 MaterialID, const FTransform& Transform, float Radius, int32 Steps);

	/** Round blob (lat-long sphere), for clouds and flower heads. */
	AI_LOOTER_SHOOTER_API void Ball(UDynamicMesh* Mesh, int32 MaterialID, const FTransform& Transform, float Radius, int32 StepsPhi, int32 StepsTheta);

	/** Rotation that points a primitive's +Z along +X (barrels, beams lying down). */
	inline FRotator AlongX() { return FRotator(-90.f, 0.f, 0.f); }

	/**
	 * Hand-built geometry. Triangles are given counter-clockwise as seen from the side they should face
	 * (the usual math convention); the builder converts to Unreal's left-handed winding, so callers can't
	 * accidentally make inside-out, one-sided-collision surfaces.
	 */
	class AI_LOOTER_SHOOTER_API FRawBuilder
	{
	public:
		explicit FRawBuilder(UE::Geometry::FDynamicMesh3& InMesh);
		int32 Vertex(const FVector& Location);
		FVector Position(int32 Index) const;
		void Triangle(int32 A, int32 B, int32 C, int32 MaterialID);
		void Quad(int32 A, int32 B, int32 C, int32 D, int32 MaterialID) { Triangle(A, B, C, MaterialID); Triangle(A, C, D, MaterialID); }

	private:
		UE::Geometry::FDynamicMesh3& Mesh;
	};

	AI_LOOTER_SHOOTER_API void EditRaw(UDynamicMesh* Mesh, TFunctionRef<void(FRawBuilder&)> Build);

	/**
	 * Normals for the whole mesh: 0 = faceted low-poly, 180 = fully soft, anything between keeps
	 * hard edges only where neighboring faces bend more than that many degrees.
	 */
	AI_LOOTER_SHOOTER_API void FinishNormals(UDynamicMesh* Mesh, float SmoothAngle);
}
