#include "World/FindGlints.h"
#include "World/FindRules.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"

const TCHAR* AFindGlints::ShapeMaterialPath = TEXT("/Game/Art/Materials/Masters/M_FX_Shape.M_FX_Shape");

namespace
{
	/** M_FX_Shape's custom data (build_vfx_materials.py): colour 0-2, strength 3, shape 4, its parameter 5, seed 6. */
	constexpr int32 ShapeFloats = 7;
	/** EVfxShape::Starburst, with this many spikes: a four-point catch of light, the classic glint. */
	constexpr float StarShape = 2.f;
	constexpr float StarSpikes = 4.f;
	/** Warm white: sunlight off old brass (linear). */
	const FLinearColor GlintColor(1.f, 0.92f, 0.72f);
	/** Under water a paler, greener white: brass seen through the tarn, bright against its teal fog without looking lit. */
	const FLinearColor UnderGlowColor(0.85f, 1.f, 0.88f);
}

AFindGlints::AFindGlints()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	Cards = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cards"));
	SetRootComponent(Cards);
	Cards->SetMobility(EComponentMobility::Movable);
	Cards->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cards->SetCastShadow(false);
	Cards->SetCanEverAffectNavigation(false);
	Cards->bReceivesDecals = false;
	Cards->bAffectDistanceFieldLighting = false;
	Cards->bAffectDynamicIndirectLighting = false;
	Cards->bVisibleInRayTracing = false;
	Cards->SetVisibility(false);
}

bool AFindGlints::MakePool()
{
	bTriedPool = true;
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, ShapeMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!Material || !Plane || !Cards)
	{
		return false;
	}
	// The actor sits at the world's origin, so the cards' places are the world's.
	SetActorTransform(FTransform::Identity);
	Cards->SetStaticMesh(Plane);
	Cards->SetMaterial(0, Material);
	Cards->SetNumCustomDataFloats(ShapeFloats);
	TArray<FTransform> Hidden;
	Hidden.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), FindRules::MaxGlints);
	Cards->AddInstances(Hidden, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
	bPool = true;
	return true;
}

void AFindGlints::SetStars(const TArray<FFindGlintStar>& Stars, const FVector& ViewLocation)
{
	Drawn = FMath::Min(Stars.Num(), FindRules::MaxGlints);
	if (Drawn == 0)
	{
		if (Cards && Cards->IsVisible())
		{
			Cards->SetVisibility(false);
		}
		return;
	}
	if (!bPool && (bTriedPool || !MakePool()))
	{
		return;
	}
	TArray<FTransform> Transforms;
	Transforms.Reserve(FindRules::MaxGlints);
	for (int32 Index = 0; Index < FindRules::MaxGlints; ++Index)
	{
		if (Index >= Drawn)
		{
			Transforms.Add(FTransform(FQuat::Identity, ViewLocation, FVector::ZeroVector));
			const float Off[ShapeFloats] = { 0.f, 0.f, 0.f, 0.f, StarShape, StarSpikes, 0.f };
			Cards->SetCustomData(Index, TArrayView<const float>(Off, ShapeFloats), false);
			continue;
		}
		const FFindGlintStar& Star = Stars[Index];
		FVector Facing = ViewLocation - Star.Location;
		const double Distance = Facing.Size();
		if (!Facing.Normalize())
		{
			Facing = FVector::UpVector;
		}
		// The engine's plane is 100 cm across. A far glint grows a little so it still reads as a point of light, not a speck.
		const float Size = FindRules::GlintSize * static_cast<float>(FMath::GetMappedRangeValueClamped(FVector2D(1000.0, 4500.0), FVector2D(1.0, 1.8), Distance))
			* FMath::Max(Star.Scale, 0.f);
		Transforms.Add(FTransform(FRotationMatrix::MakeFromZ(Facing).ToQuat(), Star.Location, FVector(Size / 100.f, Size / 100.f, 1.f)));
		const FLinearColor& Color = Star.bUnderwater ? UnderGlowColor : GlintColor;
		const float Data[ShapeFloats] = { Color.R, Color.G, Color.B, Star.Strength, StarShape, StarSpikes, Star.Seed };
		Cards->SetCustomData(Index, TArrayView<const float>(Data, ShapeFloats), false);
	}
	Cards->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	if (!Cards->IsVisible())
	{
		Cards->SetVisibility(true);
	}
}
