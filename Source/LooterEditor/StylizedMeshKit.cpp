#include "StylizedMeshKit.h"
#include "UDynamicMesh.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"

using namespace UE::Geometry;
using FPrim = UGeometryScriptLibrary_MeshPrimitiveFunctions;

FGeometryScriptPrimitiveOptions StylizedMesh::Slot(int32 MaterialID)
{
	FGeometryScriptPrimitiveOptions Options;
	Options.MaterialID = MaterialID;
	return Options;
}

int32 StylizedMesh::VertexCount(const UDynamicMesh* Mesh)
{
	int32 Count = 0;
	Mesh->ProcessMesh([&Count](const FDynamicMesh3& ReadMesh) { Count = ReadMesh.MaxVertexID(); });
	return Count;
}

void StylizedMesh::Displace(UDynamicMesh* Mesh, int32 FirstVertex, float Magnitude, float Frequency, float SeedOffset)
{
	if (Magnitude <= 0.f)
	{
		return;
	}
	const FVector Offset(SeedOffset * 1.37f, SeedOffset * 2.11f, SeedOffset * 0.73f);
	Mesh->EditMesh([&](FDynamicMesh3& EditMesh)
	{
		for (int32 Vertex : EditMesh.VertexIndicesItr())
		{
			if (Vertex < FirstVertex)
			{
				continue;
			}
			const FVector3d Position = EditMesh.GetVertex(Vertex);
			const FVector Sample = FVector(Position) * Frequency + Offset;
			// Two octaves; raw Perlin rarely leaves +/-0.5, so scale it up to use the full magnitude.
			auto Octave = [](const FVector& P)
			{
				return FVector(FMath::PerlinNoise3D(P), FMath::PerlinNoise3D(P + FVector(31.7f, 11.3f, 5.1f)),
					FMath::PerlinNoise3D(P + FVector(7.9f, 57.3f, 23.9f)));
			};
			const FVector Delta = (Octave(Sample) + Octave(Sample * 2.3f + FVector(3.3f)) * 0.5f) * 1.5f;
			EditMesh.SetVertex(Vertex, Position + FVector3d(Delta * Magnitude));
		}
	}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, true);
}

void StylizedMesh::FlattenBelow(UDynamicMesh* Mesh, int32 FirstVertex, float LocalZ)
{
	Mesh->EditMesh([&](FDynamicMesh3& EditMesh)
	{
		for (int32 Vertex : EditMesh.VertexIndicesItr())
		{
			FVector3d Position = EditMesh.GetVertex(Vertex);
			if (Vertex >= FirstVertex && Position.Z < LocalZ)
			{
				Position.Z = LocalZ;
				EditMesh.SetVertex(Vertex, Position);
			}
		}
	}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, true);
}

void StylizedMesh::Box(UDynamicMesh* Mesh, int32 MaterialID, const FVector& Center, const FVector& Size, const FRotator& Rotation, int32 Steps)
{
	FPrim::AppendBox(Mesh, Slot(MaterialID), FTransform(Rotation, Center), Size.X, Size.Y, Size.Z, Steps, Steps, Steps,
		EGeometryScriptPrimitiveOriginMode::Center);
}

void StylizedMesh::Cylinder(UDynamicMesh* Mesh, int32 MaterialID, const FTransform& Transform, float Radius, float Height, int32 Sides, bool bCapped)
{
	FPrim::AppendCylinder(Mesh, Slot(MaterialID), Transform, Radius, Height, Sides, 0, bCapped);
}

void StylizedMesh::Cone(UDynamicMesh* Mesh, int32 MaterialID, const FTransform& Transform, float BaseRadius, float TopRadius, float Height, int32 Sides, int32 HeightSteps)
{
	FPrim::AppendCone(Mesh, Slot(MaterialID), Transform, BaseRadius, TopRadius, Height, Sides, HeightSteps, true);
}

void StylizedMesh::Blob(UDynamicMesh* Mesh, int32 MaterialID, const FTransform& Transform, float Radius, int32 Steps)
{
	// Box-spheres stay boxy at low step counts; a coarse lat-long sphere gives the chunky, faceted round we want.
	FPrim::AppendSphereLatLong(Mesh, Slot(MaterialID), Transform, Radius, 4 + Steps, 6 + Steps * 2);
}

void StylizedMesh::Ball(UDynamicMesh* Mesh, int32 MaterialID, const FTransform& Transform, float Radius, int32 StepsPhi, int32 StepsTheta)
{
	FPrim::AppendSphereLatLong(Mesh, Slot(MaterialID), Transform, Radius, StepsPhi, StepsTheta);
}

StylizedMesh::FRawBuilder::FRawBuilder(FDynamicMesh3& InMesh)
	: Mesh(InMesh)
{
	if (!Mesh.HasAttributes())
	{
		Mesh.EnableAttributes();
	}
	if (!Mesh.Attributes()->HasMaterialID())
	{
		Mesh.Attributes()->EnableMaterialID();
	}
}

int32 StylizedMesh::FRawBuilder::Vertex(const FVector& Location)
{
	return Mesh.AppendVertex(FVector3d(Location));
}

FVector StylizedMesh::FRawBuilder::Position(int32 Index) const
{
	return FVector(Mesh.GetVertex(Index));
}

void StylizedMesh::FRawBuilder::Triangle(int32 A, int32 B, int32 C, int32 MaterialID)
{
	// Unreal is left-handed: a face points toward the side it appears clockwise from.
	const int32 NewTriangle = Mesh.AppendTriangle(A, C, B);
	if (NewTriangle >= 0)
	{
		Mesh.Attributes()->GetMaterialID()->SetValue(NewTriangle, MaterialID);
	}
}

void StylizedMesh::EditRaw(UDynamicMesh* Mesh, TFunctionRef<void(FRawBuilder&)> Build)
{
	Mesh->EditMesh([&](FDynamicMesh3& EditMesh)
	{
		FRawBuilder Builder(EditMesh);
		Build(Builder);
	}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, true);
}

void StylizedMesh::FinishNormals(UDynamicMesh* Mesh, float SmoothAngle)
{
	if (SmoothAngle <= 0.f)
	{
		UGeometryScriptLibrary_MeshNormalsFunctions::SetPerFaceNormals(Mesh);
	}
	else if (SmoothAngle >= 180.f)
	{
		UGeometryScriptLibrary_MeshNormalsFunctions::SetPerVertexNormals(Mesh);
	}
	else
	{
		FGeometryScriptSplitNormalsOptions SplitOptions;
		SplitOptions.bSplitByOpeningAngle = true;
		SplitOptions.OpeningAngleDeg = SmoothAngle;
		UGeometryScriptLibrary_MeshNormalsFunctions::ComputeSplitNormals(Mesh, SplitOptions, FGeometryScriptCalculateNormalsOptions());
	}
}
