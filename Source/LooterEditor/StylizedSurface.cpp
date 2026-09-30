#include "StylizedSurface.h"
#include "Components/DynamicMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	UMaterialInterface* LoadMaterial(const TCHAR* Path)
	{
		// Materials live for the whole session; cache them so hundreds of props don't hit the loader.
		static TMap<FString, TWeakObjectPtr<UMaterialInterface>> Cache;
		TWeakObjectPtr<UMaterialInterface>& Cached = Cache.FindOrAdd(Path);
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, Path);
		}
		return Cached.Get();
	}
}

TArray<UMaterialInterface*> StylizedSurfaces::CreateMaterials(UObject* Outer, const TArray<FStylizedSurface>& Surfaces)
{
	UMaterialInterface* SurfaceMaterial = LoadMaterial(TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface"));
	UMaterialInterface* FoliageMaterial = LoadMaterial(TEXT("/Game/Environment/Materials/M_StylizedFoliage.M_StylizedFoliage"));
	UMaterialInterface* GlowMaterial = LoadMaterial(TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow"));

	TArray<UMaterialInterface*> Materials;
	for (const FStylizedSurface& Surface : Surfaces)
	{
		UMaterialInterface* Base = Surface.bAdditive ? GlowMaterial
			: (Surface.bTwoSided && FoliageMaterial ? FoliageMaterial : SurfaceMaterial);
		UMaterialInstanceDynamic* Instance = Base ? UMaterialInstanceDynamic::Create(Base, Outer) : nullptr;
		if (Instance)
		{
			Instance->SetVectorParameterValue(TEXT("Color"), Surface.Color);
			Instance->SetVectorParameterValue(TEXT("TopColor"), Surface.TopColor);
			Instance->SetScalarParameterValue(TEXT("TopBlend"), Surface.TopBlend);
			Instance->SetScalarParameterValue(TEXT("TopThreshold"), Surface.TopThreshold);
			Instance->SetScalarParameterValue(TEXT("GradHeight"), Surface.GradHeight);
			Instance->SetScalarParameterValue(TEXT("GradDark"), Surface.GradDark);
			Instance->SetScalarParameterValue(TEXT("Strata"), Surface.Strata);
			Instance->SetScalarParameterValue(TEXT("Wind"), Surface.Wind);
			Instance->SetScalarParameterValue(TEXT("Glow"), Surface.Glow);
			Instance->SetScalarParameterValue(TEXT("Variation"), Surface.Variation);
			Instance->SetScalarParameterValue(TEXT("UpNormal"), Surface.UpNormal);
		}
		Materials.Add(Instance ? static_cast<UMaterialInterface*>(Instance) : Base);
	}
	return Materials;
}

void StylizedSurfaces::Apply(UDynamicMeshComponent* Component, const TArray<FStylizedSurface>& Surfaces)
{
	if (Component)
	{
		Component->ConfigureMaterialSet(CreateMaterials(Component, Surfaces));
	}
}
