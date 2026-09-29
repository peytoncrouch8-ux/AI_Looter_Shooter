#pragma once

#include "CoreMinimal.h"
#include "StylizedProp.h"

class AStaticMeshActor;
class UMaterialInterface;
class UPackage;
class UStaticMesh;
class UWorld;

/**
 * Turns the procedural props placed in a level into ordinary assets and placed actors, so nothing is generated while the
 * game runs. Each prop's mesh becomes a Nanite static mesh under /Game/Environment/Props (props with the same settings
 * share one), its painted surfaces become material instances, and a static mesh actor takes its place with the same
 * collision, shadows, outline and draw distance, plus its light pillar and glow light if it has them.
 */
class FPropBaker
{
public:
	explicit FPropBaker(UWorld* InWorld);

	/** Converts every StylizedProp in the world. Returns the number of props converted. */
	int32 ConvertLevel();

	/** Saves the new assets and the level. */
	bool SaveAll();

private:
	struct FBaked
	{
		UStaticMesh* Mesh = nullptr;
		FStylizedPropLook Look;
	};

	bool ConvertProp(AStylizedProp* Prop);

	/**
	 * The baked mesh for this prop's settings, made the first time it's asked for (or found from an earlier bake).
	 * The pointer is only good until the next call.
	 */
	const FBaked* FindOrBake(const AStylizedProp& Prop);

	/** Places one baked prop: collision, shadows, outline and draw distance as the runtime prop had them, plus its beam and light. */
	AStaticMeshActor* PlaceProp(const FBaked& Baked, EStylizedPropShape Shape, const FTransform& Transform, const FString& Label, const FString& Folder);

	UMaterialInterface* FindOrCreateSurfaceMaterial(const FStylizedSurface& Surface);
	UMaterialInterface* FindOrCreateBeamMaterial(const FLinearColor& Color, float Height);

	UWorld* World = nullptr;
	TMap<FString, FBaked> BakedMeshes;
	TMap<FString, UMaterialInterface*> Materials;
	TArray<UPackage*> NewPackages;
};
