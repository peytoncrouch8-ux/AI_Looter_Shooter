#include "Weapons/ImpactSurface.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Weapons/ImpactSurfaceMap.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/ObjectKey.h"

namespace
{
	/** Words of a terrain's own material (M_Terrain and its macro instances): its ground comes from a macro map. */
	const TCHAR* const ImpactTerrainWords[] = { TEXT("Macro"), TEXT("Terrain") };

	/**
	 * The house trim sheet's strips from the sheet's bottom up, each running to Top (looter_textures.py TRIM_STRIPS). The
	 * cream strip G is lime plaster on the cottage's and the chapel's walls, but far more often cream paint on boards (the
	 * homes kit's band_G clapboards, casings and sashes, the family plot's whitewashed pickets): wood, as a plastered
	 * timber wall knocks too.
	 */
	struct FTrimStrip
	{
		float Top;
		EImpactMaterial Material;
	};
	const FTrimStrip ImpactTrimStrips[] = {
		{ 0.03125f, EImpactMaterial::Glass },      // H4: dark window glass
		{ 0.0625f, EImpactMaterial::Metal },       // H3: iron strap with rivets
		{ 0.09375f, EImpactMaterial::Wood },       // H2: oxide-red painted trim board
		{ 0.125f, EImpactMaterial::Wood },         // H1: teal painted trim board
		{ 0.25f, EImpactMaterial::Wood },          // G: cream paint and whitewash on boards, lime plaster on timber
		{ 0.375f, EImpactMaterial::SheetMetal },   // F: corrugated tin
		{ 0.5f, EImpactMaterial::Wood },           // E: wooden shingles
		{ 0.625f, EImpactMaterial::Stone },        // D: fieldstone masonry
		{ 0.75f, EImpactMaterial::Wood },          // C: hewn beams
		{ 0.875f, EImpactMaterial::Wood },         // B: logs
		{ 1.f, EImpactMaterial::Wood },            // A: weathered siding
	};
	const TCHAR* const ImpactTrimWord = TEXT("HouseTrim");

	const FName ImpactGroundTag(TEXT("Ground"));

	/** M_Terrain's own steep faces (SteepStart, SteepFull), for a terrain material that doesn't say. */
	constexpr float ImpactSteepStart = 50.f;
	constexpr float ImpactSteepFull = 65.f;
	/** Terrain without a map is grass up to this slope (its normal's Z: about 30 degrees), dirt past it. */
	constexpr float ImpactGrassFloorZ = 0.866f;
	/** How far up the parents a material's name is read. */
	constexpr int32 ImpactParentSteps = 4;
	/** Past this many materials (dynamic instances come and go) the facts start over. */
	constexpr int32 ImpactMaxCachedMaterials = 4096;

	/** What a material says about the surfaces it shows, worked out once. */
	struct FMaterialFacts
	{
		/** The surface its (or its nearest parent's) name says. */
		EImpactMaterial Named = EImpactMaterial::Unknown;
		bool bTerrain = false;
		bool bTrim = false;
		float SteepCos = 0.5f;
		TWeakObjectPtr<const UImpactSurfaceMapData> Map;
	};

	/** Game thread only: bullets and blows resolve there. */
	TMap<FObjectKey, FMaterialFacts>& FactsCache()
	{
		static TMap<FObjectKey, FMaterialFacts> Cache;
		return Cache;
	}

	bool IsTerrainName(const FString& Name)
	{
		for (const TCHAR* Word : ImpactTerrainWords)
		{
			if (ImpactSurface::HasWord(Name, Word))
			{
				return true;
			}
		}
		return false;
	}

	FMaterialFacts FactsOf(const UMaterialInterface& Material)
	{
		TMap<FObjectKey, FMaterialFacts>& Cache = FactsCache();
		const FObjectKey Key(&Material);
		if (const FMaterialFacts* Known = Cache.Find(Key))
		{
			// A map seen before but gone since (rebuilt in the editor) is looked for again.
			if (Known->Map.IsValid() || Known->Map.IsExplicitlyNull())
			{
				return *Known;
			}
		}
		FMaterialFacts Facts;
		const UMaterialInterface* Step = &Material;
		for (int32 Depth = 0; Step && Depth < ImpactParentSteps; ++Depth)
		{
			const FString Name = Step->GetName();
			if (Facts.Named == EImpactMaterial::Unknown)
			{
				// The nearest name that says anything: an instance's own (MI_WindowGlow_Dark) before its parent's.
				ImpactSurface::FromName(Name, Facts.Named);
			}
			Facts.bTerrain = Facts.bTerrain || IsTerrainName(Name);
			Facts.bTrim = Facts.bTrim || ImpactSurface::HasWord(Name, ImpactTrimWord);
			const UMaterialInstance* Instance = Cast<UMaterialInstance>(Step);
			Step = Instance ? Instance->Parent.Get() : nullptr;
		}
		Facts.Map = UImpactSurfaceMapData::FindOn(&Material);
		Facts.SteepCos = ImpactSurface::SteepCosine(&Material);
		if (Cache.Num() >= ImpactMaxCachedMaterials)
		{
			Cache.Reset();
		}
		Cache.Add(Key, Facts);
		return Facts;
	}

	/** The material at the face that was hit (a complex trace's), else the component's first. */
	const UMaterialInterface* MaterialAt(const UPrimitiveComponent& Component, int32 FaceIndex)
	{
		int32 Section = 0;
		const UMaterialInterface* Material = FaceIndex != INDEX_NONE ? Component.GetMaterialFromCollisionFaceIndex(FaceIndex, Section) : nullptr;
		if (!Material && Component.GetNumMaterials() > 0)
		{
			Material = Component.GetMaterial(0);
		}
		return Material;
	}

	/** Terrain with nothing better to go on: the rock on its steep faces, grass on the flat, the soil showing on a slope. */
	EImpactMaterial BySlope(const FVector& Normal, float SteepCos)
	{
		if (Normal.Z < SteepCos)
		{
			return EImpactMaterial::Stone;
		}
		return Normal.Z >= ImpactGrassFloorZ ? EImpactMaterial::Grass : EImpactMaterial::Dirt;
	}
}

EImpactMaterial ImpactSurface::FromHouseTrim(float V)
{
	const float Clamped = FMath::Clamp(V, 0.f, 1.f);
	for (const FTrimStrip& Strip : ImpactTrimStrips)
	{
		if (Clamped < Strip.Top)
		{
			return Strip.Material;
		}
	}
	return ImpactTrimStrips[UE_ARRAY_COUNT(ImpactTrimStrips) - 1].Material;
}

bool ImpactSurface::IsTrimSheet(const UMaterialInterface* Material)
{
	for (int32 Depth = 0; Material && Depth < ImpactParentSteps; ++Depth)
	{
		if (HasWord(Material->GetName(), ImpactTrimWord))
		{
			return true;
		}
		const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material);
		Material = Instance ? Instance->Parent.Get() : nullptr;
	}
	return false;
}

bool ImpactSurface::UVAtHit(const FHitResult& Hit, FVector2D& OutUV)
{
	UPrimitiveComponent* Component = Hit.GetComponent();
	if (!Component || Hit.FaceIndex < 0)
	{
		return false;
	}
	const UBodySetup* Body = Component->GetBodySetup();
	if (!Body)
	{
		return false;
	}
	// The collision's copy of the mesh's UVs, kept only by meshes that ask for it (UBodySetup::CalcUVAtLocation does the
	// same, but isn't exported to game modules).
	const FBodySetupUVInfo& Info = Body->UVInfo;
	const int32 First = Hit.FaceIndex * 3;
	if (!Info.VertUVs.IsValidIndex(0) || !Info.IndexBuffer.IsValidIndex(First + 2))
	{
		return false;
	}
	FTransform ToWorld = Component->GetComponentTransform();
	if (const UInstancedStaticMeshComponent* Instances = Cast<UInstancedStaticMeshComponent>(Component))
	{
		if (!Instances->GetInstanceTransform(Hit.Item, ToWorld, /*bWorldSpace*/ true))
		{
			return false;
		}
	}
	const int32 A = Info.IndexBuffer[First];
	const int32 B = Info.IndexBuffer[First + 1];
	const int32 C = Info.IndexBuffer[First + 2];
	const TArray<FVector2D>& UVs = Info.VertUVs[0];
	if (!Info.VertPositions.IsValidIndex(FMath::Max3(A, B, C)) || !UVs.IsValidIndex(FMath::Max3(A, B, C)) || FMath::Min3(A, B, C) < 0)
	{
		return false;
	}
	const FVector Local = ToWorld.InverseTransformPosition(Hit.Location);
	const FVector Weights = FMath::ComputeBaryCentric2D(Local, Info.VertPositions[A], Info.VertPositions[B], Info.VertPositions[C]);
	OutUV = UVs[A] * Weights.X + UVs[B] * Weights.Y + UVs[C] * Weights.Z;
	return true;
}

float ImpactSurface::SteepCosine(const UMaterialInterface* Material)
{
	float Start = ImpactSteepStart;
	float Full = ImpactSteepFull;
	if (Material)
	{
		Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("SteepStart")), Start);
		Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("SteepFull")), Full);
	}
	// Half way through the blend: past it the face shows more rock than ground.
	return FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(0.5f * (Start + Full), 5.f, 89.f)));
}

EImpactMaterial ImpactSurface::Resolve(const FHitResult& Hit, EImpactSurfaceSource& OutSource)
{
	OutSource = EImpactSurfaceSource::None;
	const UPrimitiveComponent* Component = Hit.GetComponent();
	const AActor* Actor = Hit.GetActor();
	EImpactMaterial Material = EImpactMaterial::Unknown;

	// A tag says it outright.
	if (Component)
	{
		for (const FName& Tag : Component->ComponentTags)
		{
			if (FromTag(Tag, Material))
			{
				OutSource = EImpactSurfaceSource::Tag;
				return Material;
			}
		}
	}
	if (Actor)
	{
		for (const FName& Tag : Actor->Tags)
		{
			if (FromTag(Tag, Material))
			{
				OutSource = EImpactSurfaceSource::Tag;
				return Material;
			}
		}
	}
	// A physical material named for a surface (none is set today; the engine's default names none).
	if (const UPhysicalMaterial* Physical = Hit.PhysMaterial.Get())
	{
		if (FromName(Physical->GetName(), Material))
		{
			OutSource = EImpactSurfaceSource::PhysicalMaterial;
			return Material;
		}
	}

	const bool bGround = Actor && Actor->ActorHasTag(ImpactGroundTag);
	const UMaterialInterface* Surface = Component ? MaterialAt(*Component, Hit.FaceIndex) : nullptr;
	if (!Surface)
	{
		if (bGround)
		{
			OutSource = EImpactSurfaceSource::TerrainSlope;
			return BySlope(Hit.ImpactNormal, SteepCosine(nullptr));
		}
		return EImpactMaterial::Unknown;
	}

	const FMaterialFacts Facts = FactsOf(*Surface);
	// One material for a whole building: the strip under the hit says which part of it.
	FVector2D UV;
	if (Facts.bTrim && UVAtHit(Hit, UV))
	{
		OutSource = EImpactSurfaceSource::TrimSheet;
		return FromHouseTrim(1.f - FMath::Frac(UV.Y));
	}
	// Terrain: its rock faces, then what its macro map shows there.
	if (const UImpactSurfaceMapData* Map = Facts.Map.Get())
	{
		if (Hit.ImpactNormal.Z < Facts.SteepCos)
		{
			OutSource = EImpactSurfaceSource::Steep;
			return EImpactMaterial::Stone;
		}
		if (Map->KindAt(FVector2D(Hit.ImpactPoint.X, Hit.ImpactPoint.Y), Material))
		{
			OutSource = EImpactSurfaceSource::SurfaceMap;
			return Material;
		}
	}
	if (Facts.Named != EImpactMaterial::Unknown)
	{
		OutSource = EImpactSurfaceSource::MaterialName;
		return Facts.Named;
	}
	if (Facts.bTerrain || bGround)
	{
		OutSource = EImpactSurfaceSource::TerrainSlope;
		return BySlope(Hit.ImpactNormal, Facts.SteepCos);
	}
	return EImpactMaterial::Unknown;
}

EImpactMaterial ImpactSurface::Of(const FHitResult& Hit)
{
	EImpactSurfaceSource Source = EImpactSurfaceSource::None;
	return Resolve(Hit, Source);
}

FName ImpactSurface::Cue(EImpactMaterial Material)
{
	namespace Cues = LooterSoundCue::Impact;
	switch (Material)
	{
	case EImpactMaterial::Dirt: return Cues::Dirt;
	case EImpactMaterial::Grass: return Cues::Grass;
	case EImpactMaterial::Gravel: return Cues::Gravel;
	case EImpactMaterial::Sand: return Cues::Sand;
	case EImpactMaterial::Stone: return Cues::Stone;
	case EImpactMaterial::Wood: return Cues::Wood;
	case EImpactMaterial::Metal: return Cues::Metal;
	case EImpactMaterial::SheetMetal: return Cues::SheetMetal;
	case EImpactMaterial::Glass: return Cues::Glass;
	case EImpactMaterial::Water: return Cues::Water;
	case EImpactMaterial::Cloth: return Cues::Cloth;
	case EImpactMaterial::Hay: return Cues::Hay;
	case EImpactMaterial::Bone: return Cues::Bone;
	case EImpactMaterial::Unknown:
	default:
		return LooterSoundCue::ImpactWorld;
	}
}

FName ImpactSurface::FallbackCue(EImpactMaterial Material)
{
	switch (Material)
	{
	case EImpactMaterial::Wood:
	case EImpactMaterial::Bone:
		return LooterSoundCue::ImpactWood;
	case EImpactMaterial::Metal:
	case EImpactMaterial::SheetMetal:
		return LooterSoundCue::ImpactMetal;
	case EImpactMaterial::Water:
		return LooterSoundCue::ImpactWater;
	default:
		return LooterSoundCue::ImpactWorld;
	}
}

FName ImpactSurface::PlayableCue(const UObject* WorldContext, EImpactMaterial Material)
{
	const FName Own = Cue(Material);
	const ULooterSoundSubsystem* Sounds = WorldContext ? ULooterSoundSubsystem::Find(WorldContext) : nullptr;
	return Sounds && !Sounds->HasSounds(Own) ? FallbackCue(Material) : Own;
}

bool ImpactSurface::Ricochets(EImpactMaterial Material)
{
	return Material == EImpactMaterial::Stone || Material == EImpactMaterial::Gravel || Material == EImpactMaterial::Metal
		|| Material == EImpactMaterial::SheetMetal;
}

const TCHAR* ImpactSurface::SourceName(EImpactSurfaceSource Source)
{
	switch (Source)
	{
	case EImpactSurfaceSource::Tag: return TEXT("a Surface tag");
	case EImpactSurfaceSource::PhysicalMaterial: return TEXT("the physical material");
	case EImpactSurfaceSource::TrimSheet: return TEXT("the trim sheet's strip");
	case EImpactSurfaceSource::SurfaceMap: return TEXT("the terrain's surface map");
	case EImpactSurfaceSource::Steep: return TEXT("a steep terrain face");
	case EImpactSurfaceSource::MaterialName: return TEXT("the material's name");
	case EImpactSurfaceSource::TerrainSlope: return TEXT("the terrain's slope");
	case EImpactSurfaceSource::None:
	default:
		return TEXT("nothing");
	}
}

void ImpactSurface::ResetCache()
{
	FactsCache().Reset();
}
