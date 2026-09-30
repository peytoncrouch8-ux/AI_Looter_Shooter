#include "SurfaceMaterials.h"
#include "StylizedSurface.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialShared.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* SurfaceMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	const TCHAR* FoliageMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedFoliage.M_StylizedFoliage");
	const TCHAR* GlowMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow");
}

UMaterialInterface* SurfaceMaterials::ParentFor(const FStylizedSurface& Surface)
{
	const TCHAR* Path = Surface.bAdditive ? GlowMaterialPath : (Surface.bTwoSided ? FoliageMaterialPath : SurfaceMaterialPath);
	return LoadObject<UMaterialInterface>(nullptr, Path);
}

bool SurfaceMaterials::IsStylized(const UMaterialInterface* Material)
{
	const UMaterial* Base = Material ? Material->GetMaterial_Concurrent() : nullptr;
	if (!Base)
	{
		return false;
	}
	const FString Path = Base->GetPathName();
	return Path == SurfaceMaterialPath || Path == FoliageMaterialPath || Path == GlowMaterialPath;
}

TArray<UPackage*> SurfaceMaterials::PrepareParents()
{
	TArray<UPackage*> Changed;
	for (const TCHAR* Path : { SurfaceMaterialPath, FoliageMaterialPath, GlowMaterialPath })
	{
		UMaterial* Material = LoadObject<UMaterial>(nullptr, Path);
		if (!Material)
		{
			continue;
		}
		bool bChanged = false;
		for (const EMaterialUsage Usage : { MATUSAGE_Nanite, MATUSAGE_InstancedStaticMeshes, MATUSAGE_SkeletalMesh })
		{
			const bool bAllowed = Usage != MATUSAGE_Nanite || IsOpaqueOrMaskedBlendMode(*Material);
			if (bAllowed && !Material->GetUsageByFlag(Usage))
			{
				Material->Modify();
				Material->SetUsageByFlag(Usage, true);
				bChanged = true;
			}
		}
		if (bChanged)
		{
			// Recompiles its shaders with the new uses.
			Material->PostEditChange();
			Material->MarkPackageDirty();
			Changed.Add(Material->GetPackage());
		}
	}
	return Changed;
}

UMaterialInstanceConstant* SurfaceMaterials::Create(const FString& Folder, const FString& Name, UMaterialInterface* Parent)
{
	// Made directly (as the asset factory would) because AssetTools only takes content-browser folders, and the tests
	// import under /Temp.
	UPackage* Package = CreatePackage(*(Folder / Name));
	UMaterialInstanceConstant* Instance = NewObject<UMaterialInstanceConstant>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
	Instance->SetParentEditorOnly(Parent);
	FAssetRegistryModule::AssetCreated(Instance);
	Instance->MarkPackageDirty();
	return Instance;
}

void SurfaceMaterials::Apply(UMaterialInstanceConstant* Instance, const FStylizedSurface& Surface)
{
	auto Vector = [Instance](const TCHAR* Param, const FLinearColor& Value)
	{
		Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(Param), Value);
	};
	auto Scalar = [Instance](const TCHAR* Param, float Value)
	{
		Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(Param), Value);
	};
	Vector(TEXT("Color"), Surface.Color);
	Vector(TEXT("TopColor"), Surface.TopColor);
	Scalar(TEXT("TopBlend"), Surface.TopBlend);
	Scalar(TEXT("TopThreshold"), Surface.TopThreshold);
	Scalar(TEXT("GradHeight"), Surface.GradHeight);
	Scalar(TEXT("GradDark"), Surface.GradDark);
	Scalar(TEXT("Strata"), Surface.Strata);
	Scalar(TEXT("Wind"), Surface.Wind);
	Scalar(TEXT("Glow"), Surface.Glow);
	Scalar(TEXT("Variation"), Surface.Variation);
	Scalar(TEXT("UpNormal"), Surface.UpNormal);
	Instance->BasePropertyOverrides.bOverride_UsageFlags = 0;
	// Also rebuilds the static permutation from the overrides: none, so it uses the parent's shaders.
	Instance->PostEditChange();
}

bool SurfaceMaterials::ClearUsageOverrides(UMaterialInstanceConstant* Instance)
{
	if (Instance->BasePropertyOverrides.bOverride_UsageFlags == 0)
	{
		return false;
	}
	Instance->Modify();
	Instance->BasePropertyOverrides.bOverride_UsageFlags = 0;
	Instance->PostEditChange();
	Instance->MarkPackageDirty();
	return true;
}
