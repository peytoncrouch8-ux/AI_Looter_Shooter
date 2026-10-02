#include "World/LightBeam.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

void LightBeams::Setup(UStaticMeshComponent* Beam, const FLinearColor& Color, float Glow, float Height, float Radius)
{
	if (!Beam)
	{
		return;
	}
	Setup(Beam, CreateMaterial(Beam, Color, Glow, Height), Height, Radius);
}

UMaterialInstanceDynamic* LightBeams::CreateMaterial(UObject* Outer, const FLinearColor& Color, float Glow, float Height)
{
	// Lives for the whole session; keep it so every beam doesn't hit the loader.
	static TWeakObjectPtr<UMaterialInterface> GlowMaterial;
	if (!GlowMaterial.IsValid())
	{
		GlowMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow"));
	}
	UMaterialInterface* Material = GlowMaterial.Get();
	if (!Material)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Material, Outer);
	Instance->SetVectorParameterValue(TEXT("Color"), Color);
	Instance->SetScalarParameterValue(TEXT("Glow"), Glow);
	Instance->SetScalarParameterValue(TEXT("GradHeight"), Height);
	return Instance;
}

void LightBeams::Setup(UStaticMeshComponent* Beam, UMaterialInterface* Material, float Height, float Radius)
{
	if (!Beam)
	{
		return;
	}

	// Lives for the whole session; keep it so every beam doesn't hit the loader.
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

	if (Material)
	{
		Beam->SetMaterial(0, Material);
	}
}
