#include "ModelImporter.h"
#include "SurfaceMaterials.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "Dom/JsonObject.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

namespace
{
	/** The textured style's master materials (Tools/Unreal/build_world_materials.py makes them). */
	FString MasterPath(const FString& Master)
	{
		const FString Name = TEXT("M_") + Master;
		return FString::Printf(TEXT("/Game/Art/Materials/Masters/%s.%s"), *Name, *Name);
	}

	enum class ETextureRole : uint8 { Color, Normal, Masks };

	/** A texture's role comes from its file name: T_<Set>_BC, _N or _ORM. */
	ETextureRole RoleOf(const FString& Stem)
	{
		if (Stem.EndsWith(TEXT("_N")))
		{
			return ETextureRole::Normal;
		}
		return Stem.EndsWith(TEXT("_ORM")) ? ETextureRole::Masks : ETextureRole::Color;
	}

	/** Color maps are sRGB; normal maps are already in Unreal's (DirectX) convention; masks hold AO, roughness, metallic. */
	bool ApplyRole(UTexture2D* Texture, ETextureRole Role)
	{
		const bool bSRGB = Role == ETextureRole::Color;
		const TextureCompressionSettings Compression = Role == ETextureRole::Normal ? TC_Normalmap : (Role == ETextureRole::Masks ? TC_Masks : TC_Default);
		const TextureGroup Group = Role == ETextureRole::Normal ? TEXTUREGROUP_WorldNormalMap : (Role == ETextureRole::Masks ? TEXTUREGROUP_WorldSpecular : TEXTUREGROUP_World);
		if (Texture->SRGB == bSRGB && Texture->CompressionSettings == Compression && Texture->LODGroup == Group && !Texture->bFlipGreenChannel)
		{
			return false;
		}
		Texture->Modify();
		Texture->SRGB = bSRGB;
		Texture->CompressionSettings = Compression;
		Texture->LODGroup = Group;
		Texture->bFlipGreenChannel = false;
		Texture->PostEditChange();
		return true;
	}

	/** Whether the texture was imported from this exact file content. */
	bool IsCurrent(const UTexture2D* Texture, const FString& AbsoluteFile)
	{
		const UAssetImportData* ImportData = Texture ? Texture->AssetImportData.Get() : nullptr;
		if (!ImportData || ImportData->SourceData.SourceFiles.IsEmpty())
		{
			return false;
		}
		const FMD5Hash Hash = FMD5Hash::HashFile(*AbsoluteFile);
		return Hash.IsValid() && ImportData->SourceData.SourceFiles[0].FileHash == Hash;
	}
}

bool FModelImporter::ReadTexturedLook(const FJsonObject& Json, FTexturedLook& OutLook)
{
	if (!Json.TryGetStringField(TEXT("Master"), OutLook.Master))
	{
		return false;
	}
	const TSharedPtr<FJsonObject>* TextureFiles = nullptr;
	if (Json.TryGetObjectField(TEXT("Textures"), TextureFiles))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : (*TextureFiles)->Values)
		{
			OutLook.Textures.Add(Entry.Key, Entry.Value->AsString());
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* Tint = nullptr;
	if (Json.TryGetArrayField(TEXT("Tint"), Tint) && Tint->Num() >= 3)
	{
		OutLook.Tint = FLinearColor(static_cast<float>((*Tint)[0]->AsNumber()), static_cast<float>((*Tint)[1]->AsNumber()),
			static_cast<float>((*Tint)[2]->AsNumber()), Tint->Num() > 3 ? static_cast<float>((*Tint)[3]->AsNumber()) : 1.f);
	}
	double UVScale = 1.0;
	if (Json.TryGetNumberField(TEXT("UVScale"), UVScale))
	{
		OutLook.UVScale = static_cast<float>(UVScale);
	}
	const TSharedPtr<FJsonObject>* Scalars = nullptr;
	if (Json.TryGetObjectField(TEXT("Scalars"), Scalars))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : (*Scalars)->Values)
		{
			OutLook.Scalars.Add(Entry.Key, static_cast<float>(Entry.Value->AsNumber()));
		}
	}
	const TSharedPtr<FJsonObject>* Colors = nullptr;
	if (Json.TryGetObjectField(TEXT("Colors"), Colors))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : (*Colors)->Values)
		{
			const TArray<TSharedPtr<FJsonValue>>& Channels = Entry.Value->AsArray();
			if (Channels.Num() >= 3)
			{
				OutLook.Colors.Add(Entry.Key, FLinearColor(static_cast<float>(Channels[0]->AsNumber()), static_cast<float>(Channels[1]->AsNumber()),
					static_cast<float>(Channels[2]->AsNumber()), Channels.Num() > 3 ? static_cast<float>(Channels[3]->AsNumber()) : 1.f));
			}
		}
	}
	return true;
}

UTexture2D* FModelImporter::ImportTexture(const FString& ProjectRelativeFile)
{
	if (UTexture2D** Found = Textures.Find(ProjectRelativeFile))
	{
		return *Found;
	}
	const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ProjectRelativeFile);
	if (!FPaths::FileExists(File))
	{
		UE_LOG(LogModelImporter, Error, TEXT("The texture %s doesn't exist."), *File);
		return nullptr;
	}

	// Textures/<the file's folder (its set)>/<the file's name>.
	const FString Stem = FPaths::GetBaseFilename(File);
	const FString Folder = ContentRoot / TEXT("Textures") / FPaths::GetCleanFilename(FPaths::GetPath(File));
	UTexture2D* Texture = SurfaceMaterials::LoadExisting<UTexture2D>(Folder / Stem);
	const ETextureRole Role = RoleOf(Stem);
	if (!IsCurrent(Texture, File))
	{
		UAssetImportTask* Task = NewObject<UAssetImportTask>();
		Task->Filename = File;
		Task->DestinationPath = Folder;
		Task->DestinationName = Stem;
		Task->bReplaceExisting = true;
		Task->bReplaceExistingSettings = false;
		Task->bAutomated = true;
		Task->bSave = false;
		UTextureFactory* Factory = NewObject<UTextureFactory>();
		// Compressed once, with the right settings, instead of twice.
		Factory->CompressionSettings = Role == ETextureRole::Normal ? TC_Normalmap : (Role == ETextureRole::Masks ? TC_Masks : TC_Default);
		Factory->LODGroup = Role == ETextureRole::Normal ? TEXTUREGROUP_WorldNormalMap : (Role == ETextureRole::Masks ? TEXTUREGROUP_WorldSpecular : TEXTUREGROUP_World);
		Factory->bFlipNormalMapGreenChannel = false;
		Task->Factory = Factory;
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({ Task });
		Texture = nullptr;
		for (UObject* Object : Task->GetObjects())
		{
			Texture = Texture ? Texture : Cast<UTexture2D>(Object);
		}
		if (!Texture)
		{
			UE_LOG(LogModelImporter, Error, TEXT("Couldn't import the texture %s."), *File);
			return nullptr;
		}
		ApplyRole(Texture, Role);
		Texture->MarkPackageDirty();
		ChangedPackages.AddUnique(Texture->GetPackage());
		// The source size: the built texture is still compiling here and reports a placeholder.
		UE_LOG(LogModelImporter, Display, TEXT("Imported %s (%dx%d)."), *Texture->GetPathName(), static_cast<int32>(Texture->Source.GetSizeX()), static_cast<int32>(Texture->Source.GetSizeY()));
	}
	else if (ApplyRole(Texture, Role))
	{
		Texture->MarkPackageDirty();
		ChangedPackages.AddUnique(Texture->GetPackage());
	}
	Textures.Add(ProjectRelativeFile, Texture);
	return Texture;
}

UMaterialInterface* FModelImporter::UpdateTexturedMaterial(const FString& Name, const FTexturedLook& Look)
{
	UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, *MasterPath(Look.Master));
	if (!Parent)
	{
		UE_LOG(LogModelImporter, Error, TEXT("%s: the master material %s is missing (run Tools/Unreal/build_world_materials.py)."), *Name, *MasterPath(Look.Master));
		return nullptr;
	}

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

	// Blender is the source of truth: start from the master's defaults, then set what the material names.
	Instance->ClearParameterValuesEditorOnly();
	for (const TPair<FString, FString>& Map : Look.Textures)
	{
		if (UTexture2D* Texture = ImportTexture(Map.Value))
		{
			Instance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(*Map.Key), Texture);
		}
	}
	Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Tint")), Look.Tint);
	Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("UVScale")), Look.UVScale);
	// The rest by name. A name the master doesn't have is most likely a typo in Blender, so say so.
	for (const TPair<FString, float>& Scalar : Look.Scalars)
	{
		float Default = 0.f;
		if (!Parent->GetScalarParameterDefaultValue(FHashedMaterialParameterInfo(*Scalar.Key), Default))
		{
			UE_LOG(LogModelImporter, Warning, TEXT("%s: %s has no scalar parameter %s."), *Name, *Parent->GetName(), *Scalar.Key);
			continue;
		}
		Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(*Scalar.Key), Scalar.Value);
	}
	for (const TPair<FString, FLinearColor>& Color : Look.Colors)
	{
		FLinearColor Default;
		if (!Parent->GetVectorParameterDefaultValue(FHashedMaterialParameterInfo(*Color.Key), Default))
		{
			UE_LOG(LogModelImporter, Warning, TEXT("%s: %s has no color parameter %s."), *Name, *Parent->GetName(), *Color.Key);
			continue;
		}
		Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(*Color.Key), Color.Value);
	}
	Instance->BasePropertyOverrides.bOverride_UsageFlags = 0;
	Instance->PostEditChange();
	Instance->MarkPackageDirty();
	ChangedPackages.AddUnique(Instance->GetPackage());
	Materials.Add(Name, Instance);
	return Instance;
}
