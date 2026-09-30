#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StylizedSurface.h"
#include "StylizedProp.generated.h"

class UDynamicMesh;
class UDynamicMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/** Every procedural prop the generator can make. Saved in levels by name, so only append or rename carefully. */
UENUM(BlueprintType)
enum class EStylizedPropShape : uint8
{
	// Ground
	Terrain,
	IslandTerrain,
	Hill,
	Cliff,
	// Rocks
	Rock,
	Boulder,
	RockPillar,
	Crystal,
	SteppingStones,
	// Plants
	GrassPatch,
	TallGrass,
	FlowerPatch,
	CanopyTree,
	BlossomTree,
	PineTree,
	Bush,
	// Props
	Crate,
	Barrel,
	StoneWall,
	RuinPillar,
	// Sky
	FloatingIsland,
	FloatingDebris,
	Cloud,
	Beacon
};

/** Everything about a generated prop besides its triangles: how its material slots are painted and how it renders. */
struct FStylizedPropLook
{
	/** One per material slot, in slot order. */
	TArray<FStylizedSurface> Surfaces;

	/** Distance (cm) past which it isn't drawn. 0 = always drawn. */
	float CullDistance = 0.f;
	bool bCastShadow = true;

	/** Optional light pillar standing on the prop's origin (sky beacons). */
	float BeamHeight = 0.f;
	float BeamRadius = 0.f;
	FLinearColor BeamColor = FLinearColor::White;

	/** Optional soft glow light (crystals, beacons). */
	float LightIntensity = 0.f;
	float LightRadius = 0.f;
	FVector LightOffset = FVector::ZeroVector;
	FLinearColor LightColor = FLinearColor::White;
};

/**
 * Hand-crafted-looking, low-poly environment prop generated entirely from code, for building levels in the editor.
 * Shape + Seed fully determine the mesh; the colors are yours, and everything else about the look (moss, gradients,
 * wind, glow) is painted by M_StylizedSurface. It rebuilds whenever a property changes. Editor-only: run
 * Looter.BakeLevelProps before shipping and each one becomes a static mesh actor with baked assets. The class doesn't
 * exist in a packaged game, so an unbaked prop simply isn't there.
 */
UCLASS(Blueprintable)
class LOOTEREDITOR_API AStylizedProp : public AActor
{
	GENERATED_BODY()

public:
	AStylizedProp();

	/** Regenerates the mesh from the current settings. */
	UFUNCTION(BlueprintCallable, Category = "Stylized Prop")
	void Rebuild();

	UFUNCTION(BlueprintCallable, Category = "Stylized Prop")
	void Configure(EStylizedPropShape InShape, int32 InSeed, FLinearColor InPrimary, FLinearColor InSecondary);

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Ground cover and clouds: you walk and shoot through them, and the ink outline skips them. */
	static bool IsSoftShape(EStylizedPropShape InShape);
	bool IsSoft() const { return IsSoftShape(Shape); }

	/** Terrain the player walks on (and the minimap maps): the island tops, terrain tiles, hills and cliffs. */
	static bool IsGroundShape(EStylizedPropShape InShape);

	/** The minimap tag it carries, before and after baking: ground, something standing on it, or none (soft cover). */
	static FName MinimapTag(EStylizedPropShape InShape);

	/**
	 * Builds a prop's mesh into OutMesh and returns how to paint and render it. The result depends only on the shape,
	 * seed and colors, except for hills: they fit the ground under Placement, tracing the world ProbeActor is in (and
	 * ignoring ProbeActor itself).
	 */
	static FStylizedPropLook Generate(EStylizedPropShape InShape, int32 InSeed, const FLinearColor& Primary, const FLinearColor& Secondary,
		const FTransform& Placement, const AActor* ProbeActor, UDynamicMesh* OutMesh);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Prop")
	EStylizedPropShape Shape = EStylizedPropShape::Rock;

	/** Change for a different variation of the same shape. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Prop")
	int32 Seed = 1;

	/** Main color (rock, wood, grass, crate paint...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Prop")
	FLinearColor PrimaryColor = FLinearColor(0.45f, 0.42f, 0.4f);

	/** Accent color (moss, leaves, flowers, trim...) depending on the shape. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Prop")
	FLinearColor SecondaryColor = FLinearColor(0.2f, 0.35f, 0.1f);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDynamicMeshComponent> MeshComponent;

	/** Light pillar for beacons; hidden on everything else. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BeamComponent;

	/** Soft glow for crystals and beacons; hidden on everything else. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> GlowLight;
};
