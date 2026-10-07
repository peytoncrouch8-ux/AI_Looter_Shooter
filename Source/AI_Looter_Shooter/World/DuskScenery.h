#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DuskScenery.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
struct FLightingStateChange;

/**
 * Scenery seen only in one lighting state: the Gravewind's wisps spilling off the Rim and the fog welling up out of the
 * canyon at Gravewind Point, which are Main 6's dusk (Docs/Areas/RansomsRest.md: "fog rising out of the canyon at dusk";
 * the art session's SM_GravewindWisp_A-D and SM_CanyonFog_A-C). One mesh's instances per actor, placed by the area's build
 * script (Tools/Unreal/build_area_deck.py), drawn with the mesh's own materials: no collision, no shadows, each instance
 * gone past CullDistance.
 *
 * Hidden by day and shown while the level is in ShownState (Dusk): it listens for ULightingStateSubsystem's switches, as
 * AHouseLights does, and never ticks. A level without lighting states shows it never.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ADuskScenery : public AActor
{
	GENERATED_BODY()

public:
	ADuskScenery();

	/** Replaces the instances with Mesh at every transform (world space), and applies the scenery's settings again. */
	UFUNCTION(BlueprintCallable, Category = "Scenery")
	void SetInstances(UStaticMesh* Mesh, const TArray<FTransform>& Transforms);

	/** Shown or hidden as the named lighting state asks (shown only in ShownState). */
	void ApplyState(FName State);

	bool IsShownNow() const { return bShownNow; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Instances;

	/** The lighting state it's seen in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenery")
	FName ShownState = FName(TEXT("Dusk"));

	/** Each instance fades from this far and is gone at CullDistance (cm): the art's 70 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenery", meta = (ClampMin = "0", Units = "cm"))
	float CullDistance = 7000.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnLightingChanged(const FLightingStateChange& Change);
	/** No collision, navigation or shadows; its instances culled at CullDistance. */
	void ApplySceneryFlags();

	bool bShownNow = false;
	FDelegateHandle Listening;
};
