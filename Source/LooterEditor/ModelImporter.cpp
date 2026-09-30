#include "ModelImporter.h"
#include "SurfaceMaterials.h"
#include "Procedural/StylizedSurface.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Factories/FbxAssetImportData.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY(LogModelImporter);

namespace
{
	FVector ReadVector(const FJsonObject& Json, const TCHAR* Field, const FVector& Default)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Json.TryGetArrayField(Field, Values) || Values->Num() != 3)
		{
			return Default;
		}
		return FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
	}

	FLinearColor ReadColor(const FJsonObject& Json, const TCHAR* Field, const FLinearColor& Default)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Json.TryGetArrayField(Field, Values) || Values->Num() < 3)
		{
			return Default;
		}
		const float Alpha = Values->Num() > 3 ? static_cast<float>((*Values)[3]->AsNumber()) : 1.f;
		return FLinearColor(static_cast<float>((*Values)[0]->AsNumber()), static_cast<float>((*Values)[1]->AsNumber()),
			static_cast<float>((*Values)[2]->AsNumber()), Alpha);
	}

	/** A Blender material's look. The exporter already converted its distances to centimeters. */
	FStylizedSurface ReadSurface(const FJsonObject& Json)
	{
		FStylizedSurface Surface;
		Surface.Color = ReadColor(Json, TEXT("Color"), Surface.Color);
		Surface.TopColor = ReadColor(Json, TEXT("TopColor"), Surface.TopColor);
		auto Scalar = [&Json](const TCHAR* Field, float& Value)
		{
			double Number = 0.0;
			if (Json.TryGetNumberField(Field, Number))
			{
				Value = static_cast<float>(Number);
			}
		};
		Scalar(TEXT("TopBlend"), Surface.TopBlend);
		Scalar(TEXT("TopThreshold"), Surface.TopThreshold);
		Scalar(TEXT("GradHeight"), Surface.GradHeight);
		Scalar(TEXT("GradDark"), Surface.GradDark);
		Scalar(TEXT("Strata"), Surface.Strata);
		Scalar(TEXT("Wind"), Surface.Wind);
		Scalar(TEXT("Glow"), Surface.Glow);
		Scalar(TEXT("Variation"), Surface.Variation);
		Scalar(TEXT("UpNormal"), Surface.UpNormal);
		FString Kind;
		Json.TryGetStringField(TEXT("Kind"), Kind);
		Surface.bAdditive = Kind == TEXT("Glow");
		Surface.bTwoSided = Kind == TEXT("Foliage");
		return Surface;
	}

	/** The project's fixed FBX settings (see the class comment). */
	UFbxImportUI* MakeImportOptions(bool bNanite)
	{
		UFbxImportUI* Options = NewObject<UFbxImportUI>();
		Options->bIsObjImport = false;
		Options->MeshTypeToImport = FBXIT_StaticMesh;
		Options->OriginalImportType = FBXIT_StaticMesh;
		Options->bAutomatedImportShouldDetectType = false;
		Options->bImportAsSkeletal = false;
		Options->bImportMesh = true;
		Options->bImportAnimations = false;
		// Slots get the stylized material instances instead.
		Options->bImportMaterials = false;
		Options->bImportTextures = false;

		UFbxStaticMeshImportData* Data = Options->StaticMeshImportData;
		Data->bConvertScene = true;
		Data->bForceFrontXAxis = true;
		Data->bConvertSceneUnit = true;
		Data->ImportUniformScale = 1.f;
		Data->ImportTranslation = FVector::ZeroVector;
		Data->ImportRotation = FRotator::ZeroRotator;
		// The exporter moves each model to the world origin, so its Blender origin becomes the pivot.
		Data->bTransformVertexToAbsolute = true;
		Data->bBakePivotInVertex = false;
		Data->bCombineMeshes = true;
		Data->bImportMeshLODs = false;
		Data->NormalImportMethod = FBXNIM_ImportNormals;
		Data->NormalGenerationMethod = EFBXNormalGenerationMethod::MikkTSpace;
		Data->VertexColorImportOption = EVertexColorImportOption::Replace;
		Data->bRemoveDegenerates = true;
		// All lighting is dynamic.
		Data->bGenerateLightmapUVs = false;
		Data->bAutoGenerateCollision = false;
		Data->bOneConvexHullPerUCX = true;
		Data->bBuildNanite = bNanite;
		Data->bReorderMaterialToFbxOrder = true;
		return Options;
	}
}

FModelImporter::FModelImporter(const FString& InContentRoot)
	: ContentRoot(InContentRoot)
{
}

int32 FModelImporter::ImportFolder(const FString& Folder)
{
	TArray<FString> Manifests;
	IFileManager::Get().FindFiles(Manifests, *(Folder / TEXT("*.json")), true, false);
	if (Manifests.IsEmpty())
	{
		UE_LOG(LogModelImporter, Warning, TEXT("No model manifests in %s. Export the models with Tools/models.ps1 first."), *Folder);
		return 0;
	}
	for (UPackage* Package : SurfaceMaterials::PrepareParents())
	{
		ChangedPackages.AddUnique(Package);
	}

	int32 Imported = 0;
	for (const FString& Manifest : Manifests)
	{
		TArray<FModel> Models;
		if (!ReadManifest(Folder / Manifest, Models))
		{
			continue;
		}
		for (const FModel& Model : Models)
		{
			const bool bImported = Model.bSkeletal ? ImportRig(Model) != nullptr : ImportModel(Model) != nullptr;
			Imported += bImported ? 1 : 0;
		}
	}
	return Imported;
}

bool FModelImporter::ReadManifest(const FString& Path, TArray<FModel>& OutModels)
{
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogModelImporter, Error, TEXT("Couldn't read the manifest %s."), *Path);
		return false;
	}

	// Materials first: the models' slots are painted with them.
	TSet<FString> AdditiveMaterials;
	const TSharedPtr<FJsonObject>* MaterialLooks = nullptr;
	if (Root->TryGetObjectField(TEXT("materials"), MaterialLooks))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : (*MaterialLooks)->Values)
		{
			const TSharedPtr<FJsonObject>* Look = nullptr;
			if (Entry.Value->TryGetObject(Look))
			{
				const FStylizedSurface Surface = ReadSurface(**Look);
				UpdateMaterial(Entry.Key, Surface);
				if (Surface.bAdditive)
				{
					AdditiveMaterials.Add(Entry.Key);
				}
			}
		}
	}

	FString Category;
	Root->TryGetStringField(TEXT("category"), Category);
	const TArray<TSharedPtr<FJsonValue>>* ModelEntries = nullptr;
	if (!Root->TryGetArrayField(TEXT("models"), ModelEntries))
	{
		return true;
	}
	for (const TSharedPtr<FJsonValue>& Entry : *ModelEntries)
	{
		const TSharedPtr<FJsonObject>* Json = nullptr;
		FString Fbx;
		FModel Model;
		if (!Entry->TryGetObject(Json) || !(*Json)->TryGetStringField(TEXT("name"), Model.Name) || !(*Json)->TryGetStringField(TEXT("fbx"), Fbx))
		{
			UE_LOG(LogModelImporter, Warning, TEXT("%s has a model entry without a name or file; skipped."), *Path);
			continue;
		}
		Model.FbxPath = FPaths::GetPath(Path) / Fbx;
		Model.Category = Category;
		(*Json)->TryGetBoolField(TEXT("nanite"), Model.bNanite);
		TArray<FString> ModelMaterials;
		(*Json)->TryGetStringArrayField(TEXT("materials"), ModelMaterials);
		for (const FString& Material : ModelMaterials)
		{
			if (Model.bNanite && AdditiveMaterials.Contains(Material))
			{
				// Nanite draws only opaque and masked materials; the glow would come out as the default material.
				UE_LOG(LogModelImporter, Display, TEXT("%s: Nanite off, because the additive glow %s can't be drawn by Nanite."), *Model.Name, *Material);
				Model.bNanite = false;
			}
		}
		FString Collision;
		(*Json)->TryGetStringField(TEXT("collision"), Collision);
		Model.bHulls = Collision == TEXT("hulls");
		const TArray<TSharedPtr<FJsonValue>>* Sockets = nullptr;
		if ((*Json)->TryGetArrayField(TEXT("sockets"), Sockets))
		{
			for (const TSharedPtr<FJsonValue>& SocketValue : *Sockets)
			{
				const TSharedPtr<FJsonObject>* SocketJson = nullptr;
				FString SocketName;
				if (SocketValue->TryGetObject(SocketJson) && (*SocketJson)->TryGetStringField(TEXT("name"), SocketName))
				{
					FModelSocket& Socket = Model.Sockets.AddDefaulted_GetRef();
					Socket.Name = *SocketName;
					Socket.Location = ReadVector(**SocketJson, TEXT("location"), Socket.Location);
					Socket.Forward = ReadVector(**SocketJson, TEXT("forward"), Socket.Forward);
					Socket.Up = ReadVector(**SocketJson, TEXT("up"), Socket.Up);
				}
			}
		}
		(*Json)->TryGetBoolField(TEXT("skeletal"), Model.bSkeletal);
		const TArray<TSharedPtr<FJsonValue>>* HitShapes = nullptr;
		if ((*Json)->TryGetArrayField(TEXT("hitShapes"), HitShapes))
		{
			for (const TSharedPtr<FJsonValue>& ShapeValue : *HitShapes)
			{
				const TSharedPtr<FJsonObject>* ShapeJson = nullptr;
				FString Bone;
				FString Kind;
				double Radius = 0.0;
				const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
				const bool bValid = ShapeValue->TryGetObject(ShapeJson) && (*ShapeJson)->TryGetStringField(TEXT("bone"), Bone)
					&& (*ShapeJson)->TryGetStringField(TEXT("shape"), Kind)
					&& (Kind == TEXT("convex") ? (*ShapeJson)->TryGetArrayField(TEXT("points"), Points) : (*ShapeJson)->TryGetNumberField(TEXT("radius"), Radius));
				if (!bValid)
				{
					UE_LOG(LogModelImporter, Warning, TEXT("%s: a hit zone without a bone, shape or size; skipped."), *Model.Name);
					continue;
				}
				FModelHitShape& Shape = Model.HitShapes.AddDefaulted_GetRef();
				Shape.Bone = *Bone;
				Shape.Kind = Kind == TEXT("convex") ? FModelHitShape::EKind::Convex
					: Kind == TEXT("capsule") ? FModelHitShape::EKind::Capsule : FModelHitShape::EKind::Sphere;
				Shape.Radius = static_cast<float>(Radius);
				Shape.Center = ReadVector(**ShapeJson, TEXT("center"), Shape.Center);
				Shape.Start = ReadVector(**ShapeJson, TEXT("start"), Shape.Start);
				Shape.End = ReadVector(**ShapeJson, TEXT("end"), Shape.End);
				for (int32 Index = 0; Points && Index < Points->Num(); ++Index)
				{
					const TArray<TSharedPtr<FJsonValue>>* Point = nullptr;
					if ((*Points)[Index]->TryGetArray(Point) && Point->Num() == 3)
					{
						Shape.Points.Emplace((*Point)[0]->AsNumber(), (*Point)[1]->AsNumber(), (*Point)[2]->AsNumber());
					}
				}
			}
		}
		OutModels.Add(Model);
	}
	return true;
}

UMaterialInterface* FModelImporter::UpdateMaterial(const FString& Name, const FStylizedSurface& Surface)
{
	UMaterialInterface* Parent = SurfaceMaterials::ParentFor(Surface);
	if (!Parent)
	{
		UE_LOG(LogModelImporter, Error, TEXT("The stylized materials are missing; can't paint %s."), *Name);
		return nullptr;
	}

	// Blender is the source of truth: an existing instance takes the material's current look.
	const FString AssetName = TEXT("MI_") + Name;
	const FString MaterialFolder = ContentRoot / TEXT("Materials");
	UMaterialInstanceConstant* Instance = SurfaceMaterials::LoadExisting<UMaterialInstanceConstant>(MaterialFolder / AssetName);
	if (!Instance)
	{
		Instance = SurfaceMaterials::Create(MaterialFolder, AssetName, Parent);
	}
	else
	{
		Instance->Modify();
		if (Instance->Parent != Parent)
		{
			Instance->SetParentEditorOnly(Parent);
		}
	}
	if (!Instance)
	{
		UE_LOG(LogModelImporter, Error, TEXT("Couldn't create %s."), *AssetName);
		return nullptr;
	}
	SurfaceMaterials::Apply(Instance, Surface);
	Instance->MarkPackageDirty();
	ChangedPackages.AddUnique(Instance->GetPackage());
	Materials.Add(Name, Instance);
	return Instance;
}

UStaticMesh* FModelImporter::ImportModel(const FModel& Model)
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
	UFbxImportUI* Options = MakeImportOptions(Model.bNanite);
	Task->Options = Options;
	KeepSettings(SurfaceMaterials::LoadExisting<UStaticMesh>(Task->DestinationPath / Model.Name), Options->StaticMeshImportData, Model.FbxPath);
	FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({ Task });

	UStaticMesh* Mesh = nullptr;
	for (UObject* Object : Task->GetObjects())
	{
		Mesh = Mesh ? Mesh : Cast<UStaticMesh>(Object);
	}
	if (!Mesh)
	{
		UE_LOG(LogModelImporter, Error, TEXT("Couldn't import %s."), *Model.FbxPath);
		return nullptr;
	}

	Mesh->Modify();
	for (FStaticMaterial& Slot : Mesh->GetStaticMaterials())
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
	if (UBodySetup* Body = Mesh->GetBodySetup())
	{
		Body->Modify();
		if (!Model.bHulls)
		{
			// No hulls: collide with the mesh itself (reimporting keeps old hulls otherwise).
			Body->RemoveSimpleCollision();
		}
		Body->CollisionTraceFlag = Model.bHulls ? CTF_UseDefault : CTF_UseComplexAsSimple;
		Body->InvalidatePhysicsData();
		Body->CreatePhysicsMeshes();
	}
	SetSockets(Mesh, Model.Sockets);
	Mesh->PostEditChange();
	Mesh->MarkPackageDirty();
	ChangedPackages.AddUnique(Mesh->GetPackage());

	const FBox Bounds = Mesh->GetBoundingBox();
	UE_LOG(LogModelImporter, Display, TEXT("Imported %s: %d slots, %d sockets, %s, bounds min %s max %s."), *Mesh->GetPathName(),
		Mesh->GetStaticMaterials().Num(), Mesh->Sockets.Num(), Model.bHulls ? TEXT("hull collision") : TEXT("mesh collision"),
		*Bounds.Min.ToCompactString(), *Bounds.Max.ToCompactString());
	return Mesh;
}

void FModelImporter::SetSockets(UStaticMesh* Mesh, const TArray<FModelSocket>& Sockets)
{
	// The manifest has the whole list: sockets are updated by name, and ones deleted in Blender go away.
	TArray<UStaticMeshSocket*> Unused;
	for (UStaticMeshSocket* Socket : Mesh->Sockets)
	{
		Unused.Add(Socket);
	}
	for (const FModelSocket& Wanted : Sockets)
	{
		UStaticMeshSocket* Socket = Mesh->FindSocket(Wanted.Name);
		if (!Socket)
		{
			Socket = NewObject<UStaticMeshSocket>(Mesh);
			Socket->SocketName = Wanted.Name;
			Mesh->AddSocket(Socket);
		}
		Unused.Remove(Socket);
		Socket->RelativeLocation = Wanted.Location;
		Socket->RelativeRotation = FRotationMatrix::MakeFromXZ(Wanted.Forward, Wanted.Up).Rotator();
		Socket->RelativeScale = FVector::OneVector;
	}
	for (UStaticMeshSocket* Socket : Unused)
	{
		// RemoveSocket only takes it off the list; move it out of the package so it isn't saved with the mesh.
		Mesh->RemoveSocket(Socket);
		Socket->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
	}
}

void FModelImporter::KeepSettings(UObject* Existing, const UFbxAssetImportData* Settings, const FString& FbxPath)
{
	if (!Existing || !Settings)
	{
		return;
	}
	UFbxAssetImportData* Kept = DuplicateObject(Settings, Existing);
	Kept->UpdateFilenameOnly(FbxPath);
	if (UStaticMesh* Mesh = Cast<UStaticMesh>(Existing))
	{
		Mesh->SetAssetImportData(Kept);
	}
	else if (USkeletalMesh* Rig = Cast<USkeletalMesh>(Existing))
	{
		Rig->SetAssetImportData(Kept);
	}
}

bool FModelImporter::SaveAll()
{
	return ChangedPackages.IsEmpty() || UEditorLoadingAndSavingUtils::SavePackages(ChangedPackages, false);
}
