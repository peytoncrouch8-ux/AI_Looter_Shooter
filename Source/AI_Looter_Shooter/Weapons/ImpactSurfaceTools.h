#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ImpactSurfaceTools.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UTexture2D;

/**
 * The editor's side of the impact surfaces, for Tools/Unreal/build_impact_surfaces.py (in Python:
 * unreal.ImpactSurfaceTools.bake_terrain_map and the rest). Each does nothing outside the editor: the game only reads what
 * they leave on the assets (ImpactSurface).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UImpactSurfaceTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Bakes Terrain's surface map (UImpactSurfaceMapData) from its MacroMap texture's source (the PNG as imported, at full
	 * size) over the world square WorldMin..WorldMax, Size cells a side, with the scatter map's pebble bands where
	 * Scatter is given (it covers ScatterMin..ScatterMax), reading the material's own GroundCues; keeps it on the material
	 * as asset user data in place of an older one. The caller saves the material. False, with a warning saying why, when
	 * the material has no macro map that can be read.
	 */
	UFUNCTION(BlueprintCallable, Category = "Looter|Impact")
	static bool BakeTerrainMap(UMaterialInterface* Terrain, FVector2D WorldMin, FVector2D WorldMax, UTexture2D* Scatter,
		FVector2D ScatterMin, FVector2D ScatterMax, int32 Size);

	/** Each surface's share of the map Terrain carries ("Grass 61%, Dirt 22%, ..."), or why there's none. */
	UFUNCTION(BlueprintCallable, Category = "Looter|Impact")
	static FString DescribeTerrainMap(UMaterialInterface* Terrain);

	/**
	 * Makes a mesh that wears the house trim sheet keep its collision's UVs (UStaticMesh::bSupportPhysicalMaterialMasks,
	 * with the face remap), so a bullet tells its siding from its stone, tin, iron and glass. True when it changed the
	 * mesh (the caller saves it); false for a mesh without the trim sheet, or one already set.
	 */
	UFUNCTION(BlueprintCallable, Category = "Looter|Impact")
	static bool EnableTrimLookups(UStaticMesh* Mesh);

	/** Whether the mesh wears the trim sheet and keeps its collision's UVs. */
	UFUNCTION(BlueprintCallable, Category = "Looter|Impact")
	static bool HasTrimLookups(UStaticMesh* Mesh);
};
