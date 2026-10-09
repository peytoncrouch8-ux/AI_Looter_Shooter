#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InstancedProps.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * Level dressing drawn as one mesh's instances: the fences, walls, graves, cairns and yard props along a level's
 * obstacles, and a town's placed props, trees, bushes and webs (one actor per mesh, or per mesh and solidity, set by
 * Tools/Unreal/build_area_dressing.py from layout.json's obstacles and the town's town.json), so a thousand pieces cost a
 * few draws per mesh instead of a draw each. Unlike AInstancedScenery it stands inside the playable area: every
 * instance is solid like a placed static mesh actor (BlockAll, with its mesh's own hulls: pawns, the Weapon trace
 * channel, projectiles), casts shadows, and is culled on its own past CullDistance. A passable set (bSolid off: reeds,
 * lily pads, lone bushes, webs, cocoons) has no collision at all, and a set of cards (bCastShadows off: webs, reeds)
 * casts no shadow.
 *
 * In a game world a solid set carries the Obstacle tag, so the minimap draws it as an obstacle and creatures don't take
 * it for ground; a passable one never does (nothing stops there). In the editor neither does: the scatter (PCG) keeps
 * every layer out of the whole bounds of each actor tagged Obstacle, and these bounds span the valley, so the scatter
 * would leave it bare. The scatter takes the pieces' own footprints from the layout instead
 * (build_area_dressing.footprints).
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
	 * props' settings again too, CullDistance, bSolid and bCastShadows included, so an actor saved before a settings
	 * change catches up when it's rebuilt.
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

	/**
	 * Whether the instances are solid (BlockAll, an Obstacle in play). Off: no collision at all and no Obstacle tag, for
	 * what the player and the creatures walk and shoot through (reeds, lily pads, lone bushes, webs, cocoons). Set before
	 * SetInstances, which applies it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	bool bSolid = true;

	/** Whether the instances cast shadows. Off for cards (webs, reeds), as the art's rules ask. Set before SetInstances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	bool bCastShadows = true;

private:
	/** Static, solid or passable (bSolid), shadowed or not (bCastShadows), each instance culled past CullDistance. */
	void ApplyPropFlags();
};
