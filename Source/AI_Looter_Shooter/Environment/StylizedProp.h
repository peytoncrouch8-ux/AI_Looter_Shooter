#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StylizedProp.generated.h"

class UDynamicMesh;
class UDynamicMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/** Every procedural prop the game can generate. Saved in palettes by name, so only append or rename carefully. */
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

/**
 * Hand-crafted-looking, low-poly environment prop generated entirely from code (no art assets).
 * Shape + Seed fully determine the mesh, so layouts only store those. Colors come from the palette,
 * everything else about the look (moss, gradients, wind, glow) is painted by M_StylizedSurface.
 * Rebuilds in the editor whenever a property changes, so it can also be placed by hand.
 */
UCLASS(Blueprintable)
class AI_LOOTER_SHOOTER_API AStylizedProp : public AActor
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
