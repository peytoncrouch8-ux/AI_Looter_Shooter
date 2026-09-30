#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ModelImporter.h"
#include "AnimationRuntime.h"
#include "Engine/SkeletalMesh.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRigImportTest, "Looter.Editor.RigImport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRigImportTest::RunTest(const FString& Parameters)
{
	// Tests/RigImport holds RigTest.py exported by Tools/Blender/looter_export.py: bones body at (0, 0, 0.5) m, side at
	// (0.5, 0, 0.5) m and top at (0, 0, 1) m, a box skinned to each and a hit zone per bone. It's imported under /Temp,
	// twice: the second time re-imports the rig in place, with the same settings.
	for (const TCHAR* Pass : { TEXT("Import"), TEXT("Re-import") })
	{
		FModelImporter Importer(TEXT("/Temp/LooterTests"));
		TestEqual(TEXT("Models imported"), Importer.ImportFolder(FPaths::ProjectDir() / TEXT("Source/LooterEditor/Tests/RigImport")), 1);
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Temp/LooterTests/RigImport/SK_RigTest.SK_RigTest"));
		if (!TestNotNull(TEXT("Imported mesh"), Mesh))
		{
			return false;
		}
		TestNotNull(FString::Printf(TEXT("%s: the classic FBX importer did it"), Pass), Cast<UFbxSkeletalMeshImportData>(Mesh->GetAssetImportData()));

		// The armature is the root bone at the origin. The bones sit where Blender put them, turned and scaled like static
		// models (Blender's +X is Unreal's -Y, 1 m is 100 cm), and carry no scale of their own.
		const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
		TestEqual(TEXT("Bones"), Skeleton.GetNum(), 4);
		TestEqual(TEXT("Root bone"), Skeleton.GetBoneName(0), FName(TEXT("root")));
		TestTrue(FString::Printf(TEXT("Root unturned (is %s)"), *Skeleton.GetRefBonePose()[0].Rotator().ToString()),
			Skeleton.GetRefBonePose()[0].GetRotation().Equals(FQuat::Identity, 1.e-4));
		const TPair<const TCHAR*, FVector> ExpectedBones[] = {
			{ TEXT("root"), FVector(0.0, 0.0, 0.0) }, { TEXT("body"), FVector(0.0, 0.0, 50.0) },
			{ TEXT("side"), FVector(0.0, -50.0, 50.0) }, { TEXT("top"), FVector(0.0, 0.0, 100.0) } };
		for (const TPair<const TCHAR*, FVector>& Expected : ExpectedBones)
		{
			const int32 Index = Skeleton.FindBoneIndex(Expected.Key);
			if (TestTrue(FString::Printf(TEXT("%s is a bone"), Expected.Key), Index != INDEX_NONE))
			{
				const FTransform Bone = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index);
				TestTrue(FString::Printf(TEXT("%s at %s (is %s)"), Expected.Key, *Expected.Value.ToString(), *Bone.GetLocation().ToString()),
					Bone.GetLocation().Equals(Expected.Value, 0.1));
				TestTrue(FString::Printf(TEXT("%s unscaled (is %s)"), Expected.Key, *Bone.GetScale3D().ToString()), Bone.GetScale3D().Equals(FVector::OneVector, 0.001));
			}
		}

		const FBox Bounds = Mesh->GetImportedBounds().GetBox();
		TestTrue(FString::Printf(TEXT("Bounds (are %s)"), *Bounds.ToString()),
			Bounds.Min.Equals(FVector(-30.0, -90.0, 30.0), 0.1) && Bounds.Max.Equals(FVector(30.0, 30.0, 130.0), 0.1));

		// Every vertex moves rigidly with the bone of its part: the side box lies past Y -45, the top box above Z 95.
		int32 Vertices = 0;
		int32 WrongBone = 0;
		for (const FSkelMeshSection& Section : Mesh->GetImportedModel()->LODModels[0].Sections)
		{
			for (const FSoftSkinVertex& Vertex : Section.SoftVertices)
			{
				const FName Wanted = Vertex.Position.Z > 95.f ? FName(TEXT("top")) : Vertex.Position.Y < -45.f ? FName(TEXT("side")) : FName(TEXT("body"));
				uint16 Bone = 0;
				const bool bRigid = Vertex.GetRigidWeightBone(Bone);
				WrongBone += bRigid && Skeleton.GetBoneName(Section.BoneMap[Bone]) == Wanted ? 0 : 1;
				++Vertices;
			}
		}
		TestTrue(TEXT("Has vertices"), Vertices > 0);
		TestEqual(TEXT("Vertices not rigidly on their part's bone"), WrongBone, 0);

		// The hit zones are the physics asset's bodies, one per bone, where they were in Blender.
		const UPhysicsAsset* Physics = Mesh->GetPhysicsAsset();
		if (TestNotNull(TEXT("Physics asset"), Physics))
		{
			TestEqual(TEXT("Physics asset path"), Physics->GetPathName(), FString(TEXT("/Temp/LooterTests/RigImport/PA_RigTest.PA_RigTest")));
			TestEqual(TEXT("Bodies"), Physics->SkeletalBodySetups.Num(), 3);
			auto BodyOf = [&](const TCHAR* Bone) -> const USkeletalBodySetup*
			{
				const int32 Index = Physics->FindBodyIndex(Bone);
				return Index == INDEX_NONE ? nullptr : Physics->SkeletalBodySetups[Index].Get();
			};
			auto BoneTransform = [&](const TCHAR* Bone) { return FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Skeleton.FindBoneIndex(Bone)); };

			const USkeletalBodySetup* Body = BodyOf(TEXT("body"));
			if (TestNotNull(TEXT("body's hit zone"), Body) && TestEqual(TEXT("body spheres"), Body->AggGeom.SphereElems.Num(), 1))
			{
				const FKSphereElem& Sphere = Body->AggGeom.SphereElems[0];
				TestTrue(TEXT("body sphere center"), BoneTransform(TEXT("body")).TransformPosition(Sphere.Center).Equals(FVector(0.0, 0.0, 50.0), 0.1));
				TestEqual(TEXT("body sphere radius"), Sphere.Radius, 35.f, 0.01f);
				TestTrue(TEXT("Kinematic"), Body->PhysicsType == PhysType_Kinematic);
			}
			const USkeletalBodySetup* Top = BodyOf(TEXT("top"));
			if (TestNotNull(TEXT("top's hit zone"), Top) && TestEqual(TEXT("top hulls"), Top->AggGeom.ConvexElems.Num(), 1))
			{
				// The hull around the top box: its eight corners.
				const FKConvexElem& Hull = Top->AggGeom.ConvexElems[0];
				FBox Box(ForceInit);
				for (const FVector& Point : Hull.VertexData)
				{
					Box += BoneTransform(TEXT("top")).TransformPosition(Point);
				}
				TestEqual(TEXT("top hull points"), Hull.VertexData.Num(), 8);
				TestTrue(FString::Printf(TEXT("top hull around the box (is %s)"), *Box.ToString()),
					Box.Min.Equals(FVector(-10.0, -10.0, 100.0), 0.1) && Box.Max.Equals(FVector(10.0, 10.0, 130.0), 0.1));
				TestTrue(TEXT("top hull cooked"), Hull.GetChaosConvexMesh().IsValid());
			}
			const USkeletalBodySetup* Side = BodyOf(TEXT("side"));
			if (TestNotNull(TEXT("side's hit zone"), Side) && TestEqual(TEXT("side capsules"), Side->AggGeom.SphylElems.Num(), 1))
			{
				// From (0.5, 0, 0.5) to (0.9, 0, 0.5) m in Blender: along Unreal's -Y.
				const FKSphylElem& Capsule = Side->AggGeom.SphylElems[0];
				const FTransform Bone = BoneTransform(TEXT("side"));
				const FVector Axis = Bone.TransformVectorNoScale(Capsule.Rotation.RotateVector(FVector::UpVector));
				TestTrue(TEXT("side capsule center"), Bone.TransformPosition(Capsule.Center).Equals(FVector(0.0, -70.0, 50.0), 0.1));
				TestTrue(FString::Printf(TEXT("side capsule along Y (is %s)"), *Axis.ToString()), FMath::Abs(Axis.Y) > 0.999);
				TestEqual(TEXT("side capsule radius"), Capsule.Radius, 12.f, 0.01f);
				TestEqual(TEXT("side capsule length"), Capsule.Length, 40.f, 0.01f);
			}
		}

		// Each slot is painted with the stylized instance named after its Blender material.
		TestEqual(TEXT("Slots"), Mesh->GetMaterials().Num(), 2);
		for (const FSkeletalMaterial& Slot : Mesh->GetMaterials())
		{
			const FString Expected = FString::Printf(TEXT("/Temp/LooterTests/Materials/MI_%s"), *Slot.MaterialSlotName.ToString());
			TestEqual(TEXT("Slot material"), Slot.MaterialInterface ? Slot.MaterialInterface->GetPackage()->GetName() : FString(), Expected);
		}
	}

	// The test's assets are throwaway: keep them out of the editor's save prompt.
	for (TObjectIterator<UPackage> It; It; ++It)
	{
		if (It->GetName().StartsWith(TEXT("/Temp/LooterTests/")))
		{
			It->SetDirtyFlag(false);
		}
	}
	return true;
}

#endif
