#include "Combat/EnemyProjectileSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// UEnemyProjectileSubsystem's drawing: every pellet, tell and pop as an emissive sphere on one instanced mesh per color.
// Their flight is in EnemyProjectileSubsystem.cpp.

namespace
{
	const TCHAR* PelletMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	const TCHAR* PelletMeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");

	/** How brightly pellets glow (M_StylizedSurface's emissive multiplier of the color): bright enough to read at dusk. */
	constexpr float PelletGlow = 8.f;
	/** Pellets are drawn this much longer along their flight than across, so the way they move reads at a glance. */
	constexpr float PelletStretch = 1.6f;
	/** A pellet that hits the world swells to this much its size before it's gone. */
	constexpr float PopScale = 2.2f;
	/** A volley's tell starts this small (share of its full size) and swells, with a quick flicker so it reads as alive. */
	constexpr float ChargeStart = 0.25f;
	constexpr float ChargeFlicker = 0.12f;

	/** Replaces a component's instances with these (moved in place when the count is unchanged). */
	void DrawInstances(UInstancedStaticMeshComponent* Component, const TArray<FTransform>& Transforms)
	{
		if (!Component)
		{
			return;
		}
		if (Component->GetInstanceCount() != Transforms.Num())
		{
			Component->ClearInstances();
			if (Transforms.Num() > 0)
			{
				Component->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			}
		}
		else if (Transforms.Num() > 0)
		{
			Component->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
		Component->MarkRenderStateDirty();
	}

	/** A color as bytes, so pellets of the same look share one instanced mesh. */
	uint32 ColorKey(const FLinearColor& Color)
	{
		return Color.ToFColor(/*bSRGB*/ false).DWColor();
	}

	/** The engine sphere is 100 cm across: the scale that makes it Radius round. */
	float SphereScale(float Radius)
	{
		return Radius * 2.f / 100.f;
	}
}

UInstancedStaticMeshComponent* UEnemyProjectileSubsystem::InstancesFor(const FLinearColor& Color)
{
	const uint32 Key = ColorKey(Color);
	if (UInstancedStaticMeshComponent* Existing = Instances.FindRef(Key).Get())
	{
		return Existing;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	AActor* Holder = DrawActor.Get();
	if (!Holder)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Holder = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
		if (!Holder)
		{
			return nullptr;
		}
#if WITH_EDITOR
		Holder->SetActorLabel(TEXT("EnemyPellets"));
#endif
		USceneComponent* Root = NewObject<USceneComponent>(Holder, TEXT("Root"), RF_Transient);
		Root->SetMobility(EComponentMobility::Movable);
		Holder->SetRootComponent(Root);
		Root->RegisterComponent();
		DrawActor = Holder;
	}

	UInstancedStaticMeshComponent* Component = NewObject<UInstancedStaticMeshComponent>(Holder,
		MakeUniqueObjectName(Holder, UInstancedStaticMeshComponent::StaticClass(), TEXT("Pellets")), RF_Transient);
	Component->SetupAttachment(Holder->GetRootComponent());
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, PelletMeshPath));
	// Opaque and emissive (M_StylizedSurface's glow): it reads as light without the cost and sorting of translucency.
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, PelletMaterialPath))
	{
		UMaterialInstanceDynamic* Glow = UMaterialInstanceDynamic::Create(Base, Component);
		Glow->SetVectorParameterValue(TEXT("Color"), Color);
		Glow->SetScalarParameterValue(TEXT("Glow"), PelletGlow);
		Glow->SetScalarParameterValue(TEXT("Variation"), 0.f);
		Component->SetMaterial(0, Glow);
	}
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCastShadow(false);
	Component->SetCanEverAffectNavigation(false);
	Component->bReceivesDecals = false;
	Component->RegisterComponent();
	Instances.Add(Key, Component);
	return Component;
}

void UEnemyProjectileSubsystem::Draw()
{
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	TMap<uint32, TArray<FTransform>> ByColor;
	TMap<uint32, FLinearColor> Colors;
	auto Add = [&ByColor, &Colors](const FLinearColor& Color, const FTransform& Transform)
	{
		const uint32 Key = ColorKey(Color);
		ByColor.FindOrAdd(Key).Add(Transform);
		Colors.Add(Key, Color);
	};

	for (const FPellet& Pellet : Pellets)
	{
		if (!Pellet.bSpent)
		{
			const float Size = SphereScale(Pellet.Shot.Radius);
			Add(Pellet.Shot.Color, FTransform(FRotationMatrix::MakeFromX(Pellet.Shot.Direction).ToQuat(), Pellet.Position,
				FVector(Size * PelletStretch, Size, Size)));
		}
	}
	for (const FCharge& Charge : Charges)
	{
		if (const AActor* Shooter = Charge.Shooter.Get())
		{
			const float Grown = FMath::Lerp(ChargeStart, 1.f, FMath::Clamp(Charge.Age / Charge.Life, 0.f, 1.f));
			const float Flicker = 1.f + ChargeFlicker * FMath::Sin(Charge.Age * 40.f);
			Add(Charge.Color, FTransform(FQuat::Identity, Shooter->GetActorTransform().TransformPosition(Charge.Offset),
				FVector(SphereScale(Charge.Radius) * Grown * Flicker)));
		}
	}
	for (const FPop& Pop : Pops)
	{
		const float Swell = FMath::Lerp(1.f, PopScale, FMath::Clamp(Pop.Age / PopSeconds, 0.f, 1.f));
		Add(Pop.Color, FTransform(FQuat::Identity, Pop.Location, FVector(SphereScale(Pop.Radius) * Swell)));
	}

	// Colors that had something last update but have nothing now are cleared once; then they're left alone.
	TSet<uint32> Drawn;
	for (const TPair<uint32, TArray<FTransform>>& Group : ByColor)
	{
		DrawInstances(InstancesFor(Colors.FindChecked(Group.Key)), Group.Value);
		Drawn.Add(Group.Key);
	}
	for (const uint32 Key : DrawnColors)
	{
		if (!Drawn.Contains(Key))
		{
			DrawInstances(Instances.FindRef(Key).Get(), TArray<FTransform>());
		}
	}
	DrawnColors = MoveTemp(Drawn);
}
