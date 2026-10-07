#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InstancedScenery.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * Scenery far past a level's playable boundary drawn as one mesh's instances (Ransom's Rest's far ridge trees: one actor
 * per tree mesh, set by the area's build script, Tools/Unreal/build_area.py, from layout_computed.json's farTrees). Only
 * to be looked at: no collision, no navigation, no shadows, and never distance-culled, since its bounds are all its
 * instances' together and a CullDistanceVolume would otherwise take the whole lot at once.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AInstancedScenery : public AActor
{
	GENERATED_BODY()

public:
	AInstancedScenery();

	/**
	 * Replaces the instances with Mesh at every transform (world space; the actor stays where it was placed). Applies
	 * the scenery's settings again too, so an actor saved before a settings change catches up when it's rebuilt.
	 */
	UFUNCTION(BlueprintCallable, Category = "Scenery")
	void SetInstances(UStaticMesh* Mesh, const TArray<FTransform>& Transforms);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Instances;

private:
	/** No collision, navigation or shadows, static, never distance-culled. */
	void ApplySceneryFlags();
};
