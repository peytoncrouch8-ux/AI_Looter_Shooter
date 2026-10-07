#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InstancedProps.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * Level dressing drawn as one mesh's instances: the fences, walls, graves, cairns and yard props along a level's
 * obstacles (Ransom's Rest: one actor per mesh, set by Tools/Unreal/build_area_dressing.py from layout.json's
 * obstacles), so a thousand pieces cost a few draws per mesh instead of a draw each. Unlike AInstancedScenery it stands
 * inside the playable area: every instance is solid like a placed static mesh actor (BlockAll, with its mesh's own
 * hulls: pawns, the Weapon trace channel, projectiles), casts shadows, and is culled on its own past CullDistance.
 *
 * In a game world it carries the Obstacle tag, so the minimap draws it as an obstacle and creatures don't take it for
 * ground. In the editor it doesn't: the scatter (PCG) keeps every layer out of the whole bounds of each actor tagged
 * Obstacle, and these bounds span the valley, so the scatter would leave it bare. The scatter takes the pieces' own
 * footprints from the layout instead (build_area_dressing.footprints).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AInstancedProps : public AActor
{
	GENERATED_BODY()

public:
	AInstancedProps();

	virtual void PostInitializeComponents() override;

	/**
	 * Replaces the instances with Mesh at every transform (world space; the actor stays where it was placed). Applies the
	 * props' settings again too, CullDistance included, so an actor saved before a settings change catches up when it's
	 * rebuilt.
	 */
	UFUNCTION(BlueprintCallable, Category = "Props")
	void SetInstances(UStaticMesh* Mesh, const TArray<FTransform>& Transforms);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Instances;

	/**
	 * How far (cm) each instance is drawn, measured to that instance alone, so a fence's near sections stay while its far
	 * end goes (0: never culled). Set before SetInstances, which applies it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props", meta = (ClampMin = "0"))
	float CullDistance = 12000.f;

private:
	/** Static, solid, shadowed, each instance culled past CullDistance. */
	void ApplyPropFlags();
};
