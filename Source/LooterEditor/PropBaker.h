#pragma once

#include "CoreMinimal.h"
#include "Environment/StylizedProp.h"

class AActor;
class AEnvironmentLayout;
class AStaticMeshActor;
class UMaterialInterface;
class UPackage;
class UStaticMesh;
class UWorld;

/**
 * Turns a level's runtime-generated props into ordinary assets and placed actors, so nothing is generated while the game
 * runs. Each prop's mesh becomes a Nanite static mesh under /Game/Environment/Props (kinds placed more than a few times
 * share a handful of variants; the rest keep their exact shape), its painted surfaces become material instances, and the
 * level gets one static mesh actor per prop in place of the layout that spawned them. Creatures and dummies from the layout
 * become placed actors too.
 */
class FPropBaker
{
public:
	explicit FPropBaker(UWorld* InWorld);

	/** Converts every hand-placed StylizedProp and every EnvironmentLayout in the world. Returns the number of actors placed. */
	int32 ConvertLevel();

	/** Saves the new assets and the level. */
	bool SaveAll();

private:
	struct FBaked
	{
		UStaticMesh* Mesh = nullptr;
		FStylizedPropLook Look;
	};

	int32 ConvertHandPlaced(class AStylizedProp* Prop);
	int32 ConvertLayout(AEnvironmentLayout* Layout);

	/** The baked mesh called Name, made from these settings the first time it's asked for (or found from an earlier bake). */
	const FBaked* FindOrBake(const FString& Folder, const FString& Name, EStylizedPropShape Shape, int32 Seed, const FLinearColor& Primary,
		const FLinearColor& Secondary, const FTransform& Placement, const AActor* ProbeActor);

	/** Places one baked prop: collision, shadows, outline and draw distance as the runtime prop had them, plus its beam and light. */
	AStaticMeshActor* PlaceProp(const FBaked& Baked, EStylizedPropShape Shape, const FTransform& Transform, const FString& Label, const FString& Folder);

	UMaterialInterface* FindOrCreateSurfaceMaterial(const FStylizedSurface& Surface);
	UMaterialInterface* FindOrCreateBeamMaterial(const FLinearColor& Color, float Height);

	UWorld* World = nullptr;
	TMap<FString, FBaked> BakedMeshes;
	/** Hand-placed props converted so far, per kind (for their asset names). */
	TMap<FString, int32> HandPlacedCounts;
	TMap<FString, UMaterialInterface*> Materials;
	TArray<UPackage*> NewPackages;
};
