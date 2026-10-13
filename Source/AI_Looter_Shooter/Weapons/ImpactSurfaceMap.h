#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "Weapons/ImpactSurface.h"
#include "ImpactSurfaceMap.generated.h"

class UMaterialInterface;

/** One cell of a terrain's macro map, as the surface map's bake reads it. */
struct FImpactGroundSample
{
	/** The macro map's color, linear (the material reads it through sRGB). */
	FLinearColor Macro = FLinearColor::White;
	/** The macro map's alpha: about 0 grass and soil, 0.25 footpaths, 0.4 dirt roads, 0.65 scree, 1 rock. */
	float Select = 0.f;
	/** The scatter map's pebbles and small rocks (A, 0..1: road edges, cliff feet, the rim, creek banks); 0 with none. */
	float Pebbles = 0.f;
};

/**
 * What the ground is made of across a terrain material's macro map, so a shot into the ground sounds and marks as what
 * it shows: grass, dirt roads and bare soil, scree and gravel, rock. M_Terrain picks its ground layers per pixel from the
 * macro map's color and alpha; this is the same choice made once per half meter, baked in the editor
 * (UImpactSurfaceTools::BakeTerrainMap, from Tools/Unreal/build_impact_surfaces.py) and kept on the terrain's material
 * instance as asset user data, so it ships with the material and costs one array read a hit. A split macro map's windows
 * each carry their own; an area's child instances (MI_<Area>Macro_Steep) find their parent's.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UImpactSurfaceMapData : public UAssetUserData
{
	GENERATED_BODY()

public:
	/** The world square it covers (cm; X north, Y east), as the macro map's window (layout_computed.json's covers). */
	UPROPERTY(VisibleAnywhere, Category = "Impact")
	FVector2D WorldMin = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Impact")
	FVector2D WorldMax = FVector2D::ZeroVector;

	/** Cells along each side. */
	UPROPERTY(VisibleAnywhere, Category = "Impact")
	int32 Size = 0;

	/** An EImpactMaterial per cell: rows run north along X from WorldMin.X, columns east along Y from WorldMin.Y. */
	UPROPERTY()
	TArray<uint8> Kinds;

	/** The surface at a world point, or false outside the map (or on a cell that knows nothing). */
	bool KindAt(const FVector2D& WorldXY, EImpactMaterial& OutMaterial) const;

	/** The map the material carries, or one of its parents (a few steps up), or null. */
	static const UImpactSurfaceMapData* FindOn(const UMaterialInterface* Material);

	/**
	 * The ground a macro cell shows, by M_Terrain's own rules (Tools/Unreal/build_world_materials.py GROUND_WEIGHTS, its
	 * noise taken at its middle): rock where the material's rock share passes half (alpha about 0.72), scree and the
	 * scatter's pebble bands gravel; then the stony layer (the roads' and paths' packed earth, warm bare soil, the dark
	 * ground under trees) dirt where it's most of the cell, and the rest (green, and the dry thatch between) grass. Cues and
	 * Cues2 are the material's GroundCues and GroundCues2.
	 */
	static EImpactMaterial ClassifyGround(const FImpactGroundSample& Sample, const FLinearColor& Cues, const FLinearColor& Cues2);

	/** M_Terrain's defaults (GROUND_DEFAULTS), for a material that doesn't say. */
	static const FLinearColor DefaultCues;
	static const FLinearColor DefaultCues2;
};
