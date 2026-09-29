#include "Procedural/StylizedSurface.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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

void StylizedSurfaces::SetupBeam(UStaticMeshComponent* Beam, const FLinearColor& Color, float Glow, float Height, float Radius)
{
	if (!Beam)
	{
		return;
	}

	static TWeakObjectPtr<UStaticMesh> CylinderMesh;
	if (!CylinderMesh.IsValid())
	{
		CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	}
	Beam->SetStaticMesh(CylinderMesh.Get());
	Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam->SetCastShadow(false);
	Beam->SetGenerateOverlapEvents(false);
	Beam->bReceivesDecals = false;

	// The engine cylinder is 100 x 100 with its pivot in the middle.
	Beam->SetRelativeScale3D(FVector(Radius / 50.f, Radius / 50.f, Height / 100.f));
	Beam->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f));

	if (UMaterialInterface* GlowMaterial = LoadMaterial(TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow")))
	{
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(GlowMaterial, Beam);
		Instance->SetVectorParameterValue(TEXT("Color"), Color);
		Instance->SetScalarParameterValue(TEXT("Glow"), Glow);
		Instance->SetScalarParameterValue(TEXT("GradHeight"), Height);
		Beam->SetMaterial(0, Instance);
	}
}
