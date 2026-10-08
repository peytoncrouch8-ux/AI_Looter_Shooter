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

float LightBeams::GutterStrength(float Time)
{
	// Burning low: a slow breath under a quicker waver, never as bright as a healthy beam...
	const float Breath = 0.5f + 0.5f * FMath::PerlinNoise1D(Time * 0.7f);
	const float Waver = FMath::PerlinNoise1D(Time * 6.3f + 17.f);
	float Strength = 0.5f + 0.32f * Breath + 0.14f * Waver;

	// ...and once in each few seconds, at a moment of its own, it all but goes out: a quick sputter that catches again.
	constexpr float Cycle = 3.7f;
	constexpr float Sputter = 0.5f;
	const float CycleIndex = FMath::FloorToFloat(Time / Cycle);
	const float InCycle = Time - CycleIndex * Cycle;
	const float DipAt = 0.4f + FMath::Frac(FMath::Sin(CycleIndex * 12.9898f) * 43758.5453f) * (Cycle - Sputter - 0.8f);
	const float IntoDip = (InCycle - DipAt) / Sputter;
	if (IntoDip > 0.f && IntoDip < 1.f)
	{
		// Down fast, back up slower, flickering all the while.
		const float Depth = IntoDip < 0.3f ? IntoDip / 0.3f : 1.f - (IntoDip - 0.3f) / 0.7f;
		const float Flicker = 0.12f + 0.2f * FMath::Abs(FMath::Sin(Time * 47.f));
		Strength *= FMath::Lerp(1.f, Flicker, FMath::SmoothStep(0.f, 1.f, Depth));
	}
	return FMath::Clamp(Strength, 0.08f, 1.f);
}

void LightBeams::Gutter(UStaticMeshComponent* Beam, float Time, float Glow, float Height, float Radius)
{
	if (!Beam)
	{
		return;
	}
	const float Strength = GutterStrength(Time);
	// A flame low on fuel stands shorter and thinner as well as dimmer.
	const float Tall = Height * (0.55f + 0.45f * Strength);
	const float Wide = Radius * (0.7f + 0.3f * Strength);
	Beam->SetRelativeScale3D(FVector(Wide / 50.f, Wide / 50.f, Tall / 100.f));
	Beam->SetRelativeLocation(FVector(0.f, 0.f, Tall * 0.5f));
	if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Beam->GetMaterial(0)))
	{
		Material->SetScalarParameterValue(TEXT("Glow"), Glow * Strength);
		// The glow fades over the beam's height: keep the fade on the beam as it shrinks.
		Material->SetScalarParameterValue(TEXT("GradHeight"), Tall);
	}
}
