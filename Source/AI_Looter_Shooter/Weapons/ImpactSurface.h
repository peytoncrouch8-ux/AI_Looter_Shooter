#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UObject;
struct FHitResult;

/**
 * What a bullet struck in the world, for its sound, its puff and the mark it leaves (creatures and the training dummies
 * have their own: EImpactSurface). A terrain's surface map stores these per cell (UImpactSurfaceMapData), so never
 * reorder or reuse a value: only add before Count.
 */
enum class EImpactMaterial : uint8
{
	/** Nothing says what it is: the old thud and a plain hole. */
	Unknown,
	/** Packed earth: roads, paths, bare ground, the ground under trees. */
	Dirt,
	/** Grass over soil, green or dry. */
	Grass,
	/** Gravel, scree, pebbles, rail ballast. */
	Gravel,
	/** Dry sand and dust. */
	Sand,
	/** Rock and masonry: cliffs, outcrops, boulders, stone walls, chimneys, plaster. */
	Stone,
	/** Timber: buildings, fences, crates, the depot, tree trunks, coffins. */
	Wood,
	/** Heavy iron and steel: rails, the lift, the windmill, straps, lanterns, bells. */
	Metal,
	/** Thin sheet metal: tin roofs, drums, mailboxes, buckets, wire screens. */
	SheetMetal,
	/** Window glass. */
	Glass,
	Water,
	/** Cloth, canvas, rope, leather. */
	Cloth,
	/** Hay and straw. */
	Hay,
	/** Bone and skulls. */
	Bone,
	Count
};

/** Where ImpactSurface::Resolve found a hit's surface (Looter.Impact.Probe says it). */
enum class EImpactSurfaceSource : uint8
{
	/** Nothing did: Unknown. */
	None,
	/** A "Surface.<Name>" tag on the component or its actor. */
	Tag,
	/** The physical material's name (PM_<Name>), when one is set. */
	PhysicalMaterial,
	/** The house trim sheet's strip under the hit (its UV, where the mesh keeps them). */
	TrimSheet,
	/** The terrain's surface map, baked from its macro map. */
	SurfaceMap,
	/** A terrain face steeper than its material's steep start: the rock it shows. */
	Steep,
	/** The words in the material's (or a parent's) name. */
	MaterialName,
	/** Terrain without a map: grass on the flat, dirt on a slope. */
	TerrainSlope
};

/**
 * Telling what a bullet hit, cheaply and from what the game already has (no physical surface types to keep in step):
 *   1. a "Surface.<Name>" tag on the component, then on its actor, says it outright (Surface.Gravel on a ballast bed);
 *   2. a physical material named for a surface (PM_Wood) says it next;
 *   3. the material at the face that was hit (the bullet traces complex): the house trim sheet (one material for a whole
 *      building's siding, logs, stone, tin, iron straps and windows) by the strip under the hit's UV, on meshes that keep
 *      their collision UVs (Tools/Unreal/build_impact_surfaces.py turns that on); a terrain by its surface map
 *      (UImpactSurfaceMapData, baked from its macro map: grass, dirt roads, scree, rock) and the rock on its steep faces;
 *      else the PascalCase words in the material's name and its parents' (MI_WoodPlanks, MI_RockCliff_Ransom, M_Glass);
 *   4. terrain without a map (an actor tagged Ground, or a Macro/Terrain material): grass on the flat, dirt on a slope.
 * Each material's facts are worked out once and kept (by object key), so a hit costs a tag scan and a map lookup.
 */
namespace ImpactSurface
{
	/** The surface a bullet's hit (or a melee blow's) struck. */
	AI_LOOTER_SHOOTER_API EImpactMaterial Of(const FHitResult& Hit);

	/** The same, and where it was found. */
	AI_LOOTER_SHOOTER_API EImpactMaterial Resolve(const FHitResult& Hit, EImpactSurfaceSource& OutSource);

	/** The surface a name's PascalCase words say (MI_MailboxTin is sheet metal, MI_Tinted isn't), or false. */
	AI_LOOTER_SHOOTER_API bool FromName(const FString& Name, EImpactMaterial& OutMaterial);

	/** The surface a "Surface.<Name>" tag names (any case: Surface.wood), or false for any other tag. */
	AI_LOOTER_SHOOTER_API bool FromTag(FName Tag, EImpactMaterial& OutMaterial);

	/** Whether Word is in Name as a PascalCase word: as written, and not running on into a lower-case letter. */
	AI_LOOTER_SHOOTER_API bool HasWord(const FString& Name, const TCHAR* Word);

	/**
	 * The house trim sheet's surface at V, measured from the bottom of the sheet as Blender lays it out
	 * (Tools/Blender/looter_textures.py TRIM_STRIPS; Unreal's UV.y is 1 - V): siding, logs, beams, shingles, painted trim
	 * and the cream strip (paint and whitewash on boards, plaster on timber walls) are wood, the fieldstone stone, the
	 * corrugated tin sheet metal, the iron strap metal and the bottom strip window glass.
	 */
	AI_LOOTER_SHOOTER_API EImpactMaterial FromHouseTrim(float V);

	/** Whether the material (or a parent) is the house trim sheet (MI_HouseTrim and an area's instance of it). */
	AI_LOOTER_SHOOTER_API bool IsTrimSheet(const UMaterialInterface* Material);

	/**
	 * The UV (channel 0) at a complex trace's hit, from the collision's own copy of the mesh's UVs. Only meshes that keep
	 * them have them (UStaticMesh::bSupportPhysicalMaterialMasks, which build_impact_surfaces.py sets on the meshes
	 * wearing a trim sheet); an instanced mesh's instance is found by the hit's Item.
	 */
	AI_LOOTER_SHOOTER_API bool UVAtHit(const FHitResult& Hit, FVector2D& OutUV);

	/**
	 * How upright a terrain face must be to show its ground rather than its rock: the cosine of the middle of the
	 * material's SteepStart..SteepFull (M_Terrain's own 50-65 degrees, or an area's instance's).
	 */
	AI_LOOTER_SHOOTER_API float SteepCosine(const UMaterialInterface* Material);

	/** The impact's own cue (LooterSoundCue::Impact). */
	AI_LOOTER_SHOOTER_API FName Cue(EImpactMaterial Material);

	/** The old four (Impact.World, .Wood, .Metal, .Water), played while a surface's own cue has no sounds yet. */
	AI_LOOTER_SHOOTER_API FName FallbackCue(EImpactMaterial Material);

	/** The cue to play now: the surface's own once the bank has its sounds, else its fallback. */
	AI_LOOTER_SHOOTER_API FName PlayableCue(const UObject* WorldContext, EImpactMaterial Material);

	/** Hard enough for a glancing round to whine off (a ricochet's layer: stone, gravel, metal). */
	AI_LOOTER_SHOOTER_API bool Ricochets(EImpactMaterial Material);

	/** The surface's name, for logs and the console. */
	AI_LOOTER_SHOOTER_API const TCHAR* Name(EImpactMaterial Material);
	AI_LOOTER_SHOOTER_API const TCHAR* SourceName(EImpactSurfaceSource Source);

	/** Forgets every material's facts (tests; a rebuilt surface map is picked up on the next play anyway). */
	AI_LOOTER_SHOOTER_API void ResetCache();
}
