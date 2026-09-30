#include "ModelImporter.h"
#include "SurfaceMaterials.h"
#include "AnimationRuntime.h"
#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/SkeletalMesh.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Animation/Skeleton.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "UObject/Package.h"

namespace
{
	/**
	 * A rig's FBX arrives already in centimeters and facing +X once Unreal flips it to left-handed (looter_export.py's
	 * bake_for_unreal), because the importer would put any unit or axis conversion on the root bone. These settings
	 * make it convert nothing: the file's axes are the ones the importer expects without "force front X".
	 */
	UFbxImportUI* MakeRigImportOptions()
	{
		UFbxImportUI* Options = NewObject<UFbxImportUI>();
		Options->bIsObjImport = false;
		Options->MeshTypeToImport = FBXIT_SkeletalMesh;
		Options->OriginalImportType = FBXIT_SkeletalMesh;
		Options->bAutomatedImportShouldDetectType = false;
		Options->bImportAsSkeletal = true;
		Options->bImportMesh = true;
		Options->bImportAnimations = false;
		// The hit zones from Blender make the physics asset instead of guessed shapes.
		Options->bCreatePhysicsAsset = false;
		Options->bImportMaterials = false;
		Options->bImportTextures = false;

		UFbxSkeletalMeshImportData* Data = Options->SkeletalMeshImportData;
		Data->bConvertScene = true;
		Data->bForceFrontXAxis = false;
		Data->bConvertSceneUnit = true;
		Data->ImportUniformScale = 1.f;
		Data->ImportTranslation = FVector::ZeroVector;
		Data->ImportRotation = FRotator::ZeroRotator;
		Data->ImportContentType = EFBXImportContentType::FBXICT_All;
		Data->NormalImportMethod = FBXNIM_ImportNormals;
		Data->NormalGenerationMethod = EFBXNormalGenerationMethod::MikkTSpace;
		Data->VertexColorImportOption = EVertexColorImportOption::Replace;
		Data->bImportMorphTargets = false;
		Data->bPreserveSmoothingGroups = true;
		Data->bImportMeshesInBoneHierarchy = true;
		// Importing again takes Blender's current bind pose.
		Data->bUpdateSkeletonReferencePose = true;
		Data->bUseT0AsRefPose = false;
		Data->bReorderMaterialToFbxOrder = true;
		return Options;
	}
}

USkeletalMesh* FModelImporter::ImportRig(const FModel& Model)
{
	if (!FPaths::FileExists(Model.FbxPath))
	{
		UE_LOG(LogModelImporter, Error, TEXT("%s: %s doesn't exist."), *Model.Name, *Model.FbxPath);
		return nullptr;
	}

	UAssetImportTask* Task = NewObject<UAssetImportTask>();
	Task->Filename = Model.FbxPath;
	Task->DestinationPath = ContentRoot / Model.Category;
	Task->DestinationName = Model.Name;
	Task->bReplaceExisting = true;
	Task->bReplaceExistingSettings = true;
	Task->bAutomated = true;
	Task->bSave = false;
	// An explicit factory keeps the import on these fixed settings instead of the Interchange defaults.
	Task->Factory = NewObject<UFbxFactory>();
	UFbxImportUI* Options = MakeRigImportOptions();
	Task->Options = Options;
	KeepSettings(SurfaceMaterials::LoadExisting<USkeletalMesh>(Task->DestinationPath / Model.Name), Options->SkeletalMeshImportData, Model.FbxPath);
	FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({ Task });

	USkeletalMesh* Mesh = nullptr;
	for (UObject* Object : Task->GetObjects())
	{
		Mesh = Mesh ? Mesh : Cast<USkeletalMesh>(Object);
	}
	if (!Mesh)
	{
		UE_LOG(LogModelImporter, Error, TEXT("Couldn't import the rig %s."), *Model.FbxPath);
		return nullptr;
	}

	Mesh->Modify();
	for (FSkeletalMaterial& Slot : Mesh->GetMaterials())
	{
		if (UMaterialInterface** Material = Materials.Find(Slot.MaterialSlotName.ToString()))
		{
			Slot.MaterialInterface = *Material;
		}
		else
		{
			UE_LOG(LogModelImporter, Warning, TEXT("%s: slot %s has no material in the manifest."), *Model.Name, *Slot.MaterialSlotName.ToString());
		}
	}
	UPhysicsAsset* Physics = MakeHitZones(Mesh, Model);
	Mesh->PostEditChange();
	Mesh->MarkPackageDirty();
	ChangedPackages.AddUnique(Mesh->GetPackage());
	if (USkeleton* Skeleton = Mesh->GetSkeleton())
	{
		ChangedPackages.AddUnique(Skeleton->GetPackage());
	}

	const FBox Bounds = Mesh->GetImportedBounds().GetBox();
	UE_LOG(LogModelImporter, Display, TEXT("Imported %s: %d bones, %d slots, %d hit zones, bounds min %s max %s."), *Mesh->GetPathName(),
		Mesh->GetRefSkeleton().GetNum(), Mesh->GetMaterials().Num(), Physics ? Physics->SkeletalBodySetups.Num() : 0,
		*Bounds.Min.ToCompactString(), *Bounds.Max.ToCompactString());
	return Mesh;
}

UPhysicsAsset* FModelImporter::MakeHitZones(USkeletalMesh* Mesh, const FModel& Model)
{
	if (Model.HitShapes.IsEmpty())
	{
		return nullptr;
	}

	// PA_<model>, beside the mesh. Rebuilt from the manifest every import: Blender is the source of truth.
	const FString AssetName = TEXT("PA_") + Model.Name.RightChop(Model.Name.StartsWith(TEXT("SK_")) ? 3 : 0);
	const FString PackagePath = ContentRoot / Model.Category / AssetName;
	UPhysicsAsset* Physics = SurfaceMaterials::LoadExisting<UPhysicsAsset>(PackagePath);
	if (!Physics)
	{
		Physics = NewObject<UPhysicsAsset>(CreatePackage(*PackagePath), *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Physics);
	}
	Physics->Modify();
	Physics->SkeletalBodySetups.Empty();
	Physics->ConstraintSetup.Empty();

	// Shapes arrive in the model's space; each body is in its bone's space, taken from the bind pose.
	const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
	for (const FModelHitShape& Shape : Model.HitShapes)
	{
		const int32 BoneIndex = Skeleton.FindBoneIndex(Shape.Bone);
		if (BoneIndex == INDEX_NONE)
		{
			UE_LOG(LogModelImporter, Warning, TEXT("%s: hit zone on %s, which isn't a bone of the rig; skipped."), *Model.Name, *Shape.Bone.ToString());
			continue;
		}
		const FTransform Bone = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, BoneIndex);

		USkeletalBodySetup* Body = nullptr;
		for (USkeletalBodySetup* Existing : Physics->SkeletalBodySetups)
		{
			Body = Body ? Body : (Existing && Existing->BoneName == Shape.Bone ? Existing : nullptr);
		}
		if (!Body)
		{
			Body = NewObject<USkeletalBodySetup>(Physics, NAME_None, RF_Transactional);
			Body->BoneName = Shape.Bone;
			// Moved by the animation, never simulated.
			Body->PhysicsType = PhysType_Kinematic;
			Physics->SkeletalBodySetups.Add(Body);
		}
		if (Shape.Kind == FModelHitShape::EKind::Convex)
		{
			FKConvexElem Hull;
			for (const FVector& Point : Shape.Points)
			{
				Hull.VertexData.Add(Bone.InverseTransformPosition(Point));
			}
			Hull.UpdateElemBox();
			Body->AggGeom.ConvexElems.Add(Hull);
		}
		else if (Shape.Kind == FModelHitShape::EKind::Capsule)
		{
			const FVector Start = Bone.InverseTransformPosition(Shape.Start);
			const FVector End = Bone.InverseTransformPosition(Shape.End);
			FKSphylElem Capsule(Shape.Radius, static_cast<float>(FVector::Dist(Start, End)));
			Capsule.Center = (Start + End) * 0.5;
			Capsule.Rotation = FRotationMatrix::MakeFromZ(End - Start).Rotator();
			Body->AggGeom.SphylElems.Add(Capsule);
		}
		else
		{
			FKSphereElem Sphere(Shape.Radius);
			Sphere.Center = Bone.InverseTransformPosition(Shape.Center);
			Body->AggGeom.SphereElems.Add(Sphere);
		}
	}

	// Hulls become physics shapes when cooked.
	for (USkeletalBodySetup* Body : Physics->SkeletalBodySetups)
	{
		Body->InvalidatePhysicsData();
		Body->CreatePhysicsMeshes();
	}
	Physics->UpdateBodySetupIndexMap();
	Physics->UpdateBoundsBodiesArray();
	Physics->SetPreviewMesh(Mesh);
	Physics->PostEditChange();
	Physics->MarkPackageDirty();
	Mesh->SetPhysicsAsset(Physics);
	ChangedPackages.AddUnique(Physics->GetPackage());
	return Physics;
}
