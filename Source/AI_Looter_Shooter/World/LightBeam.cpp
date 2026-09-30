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

	// Both live for the whole session; keep them so every beam doesn't hit the loader.
	static TWeakObjectPtr<UStaticMesh> CylinderMesh;
	static TWeakObjectPtr<UMaterialInterface> GlowMaterial;
	if (!CylinderMesh.IsValid())
	{
		CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	}
	if (!GlowMaterial.IsValid())
	{
		GlowMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow"));
	}
	Beam->SetStaticMesh(CylinderMesh.Get());
	Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam->SetCastShadow(false);
	Beam->SetGenerateOverlapEvents(false);
	Beam->bReceivesDecals = false;

	// The engine cylinder is 100 x 100 with its pivot in the middle.
	Beam->SetRelativeScale3D(FVector(Radius / 50.f, Radius / 50.f, Height / 100.f));
	Beam->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f));

	if (UMaterialInterface* Material = GlowMaterial.Get())
	{
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Material, Beam);
		Instance->SetVectorParameterValue(TEXT("Color"), Color);
		Instance->SetScalarParameterValue(TEXT("Glow"), Glow);
		Instance->SetScalarParameterValue(TEXT("GradHeight"), Height);
		Beam->SetMaterial(0, Instance);
	}
}
