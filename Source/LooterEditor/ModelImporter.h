#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogModelImporter, Log, All);

struct FStylizedSurface;
class UFbxAssetImportData;
class UMaterialInterface;
class UPackage;
class UPhysicsAsset;
class USkeletalMesh;
class UStaticMesh;

/**
 * Imports the models Tools/models.ps1 exported from Blender. Each manifest (*.json) in the export folder lists FBX files
 * and the look of every material they use. Meshes land in /Game/Art/<Category> with fixed settings: Blender's Front view
 * faces the actor's forward (+X), 1 m is 100 cm, the model's own hard and soft edges are kept, Nanite is on, UCX_ hulls
 * (or else the mesh itself) are the collision, and SOCKET_ empties become sockets. Each material slot gets the stylized
 * material instance /Game/Art/Materials/MI_<slot name>, made or updated from the Blender material. Importing again
 * updates the assets in place, so placed actors keep their meshes.
 *
 * A rigged model becomes a skeletal mesh (SK_) with its own skeleton, and its hit zones become the bodies of a physics
 * asset (PA_): what shots hit, each telling which bone it belongs to.
 */
class FModelImporter
{
public:
	/** Meshes go to <ContentRoot>/<Category> and materials to <ContentRoot>/Materials (tests import under /Temp). */
	explicit FModelImporter(const FString& InContentRoot = TEXT("/Game/Art"));

	/** Imports everything the manifests in Folder list. Returns the number of models imported. */
	int32 ImportFolder(const FString& Folder);

	/** Saves the new and changed assets. */
	bool SaveAll();

private:
	struct FModelSocket
	{
		FName Name;
		FVector Location = FVector::ZeroVector;
		FVector Forward = FVector::ForwardVector;
		FVector Up = FVector::UpVector;
	};

	/**
	 * A rig's hit zone, in the model's space: a sphere (Center, Radius), a capsule whose round ends center on Start and
	 * End (Radius), or the convex hull of Points.
	 */
	struct FModelHitShape
	{
		enum class EKind : uint8 { Sphere, Capsule, Convex };

		FName Bone;
		EKind Kind = EKind::Sphere;
		FVector Center = FVector::ZeroVector;
		FVector Start = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
		float Radius = 0.f;
		TArray<FVector> Points;
	};

	struct FModel
	{
		FString Name;
		FString FbxPath;
		FString Category;
		bool bNanite = true;
		/** UCX_ hulls came with the model; without them the mesh is its own collision. */
		bool bHulls = false;
		/** In the model's space. They come through the manifest: FBX sockets arrive with the wrong rotation. */
		TArray<FModelSocket> Sockets;
		/** A rigged model: a skeletal mesh whose hit zones make its physics asset. */
		bool bSkeletal = false;
		TArray<FModelHitShape> HitShapes;
	};

	/** Reads one manifest: updates its materials and returns its models. */
	bool ReadManifest(const FString& Path, TArray<FModel>& OutModels);

	UStaticMesh* ImportModel(const FModel& Model);
	static void SetSockets(UStaticMesh* Mesh, const TArray<FModelSocket>& Sockets);

	/**
	 * Importing onto an existing asset re-imports it, and a re-import uses the settings stored on the asset (from its
	 * last import, or whatever the editor last used): store ours there first.
	 */
	static void KeepSettings(UObject* Existing, const UFbxAssetImportData* Settings, const FString& FbxPath);
	UMaterialInterface* UpdateMaterial(const FString& Name, const FStylizedSurface& Surface);

	// ModelImporterRig.cpp
	USkeletalMesh* ImportRig(const FModel& Model);
	UPhysicsAsset* MakeHitZones(USkeletalMesh* Mesh, const FModel& Model);

	FString ContentRoot;
	TMap<FString, UMaterialInterface*> Materials;
	TArray<UPackage*> ChangedPackages;
};
