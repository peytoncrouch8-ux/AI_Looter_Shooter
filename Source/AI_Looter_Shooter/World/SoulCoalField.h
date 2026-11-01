#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "SoulCoalField.generated.h"

class UInstancedStaticMeshComponent;
class UMissionRunner;

/**
 * The soul coal the dying barge rained over the Rest (Main 6's BargeFall; MAINS_2_7.md section 10: "soul coal strewn across
 * the valley and the canyon rim after Main 6 (instanced glowing lumps)"), lying smoking along the west road, the keeper's
 * path, the valley under the barge's last line and the canyon's rim through Main 7, gone once Saint Ada wakes (her light
 * burns the Rest clean). Scenery only (the twelve with souls to free are ASoulCoalLump): black lumps (the Tripo soul coal,
 * else a small boulder in charcoal) with a cyan crust of soul-light, one instanced mesh each (two draws a field), no
 * collision, no shadow, cut off at DrawDistance; a thin thread of smoke off a lump near the player now and then (the game's
 * puffs). Tools/Unreal/build_area_main7.py places one field per stretch with its lumps (SetLumps). Hidden, it ticks nothing.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASoulCoalField : public AActor
{
	GENERATED_BODY()

public:
	ASoulCoalField();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Its lumps (world transforms, each a lump's foot and size), made into instances now. */
	UFUNCTION(BlueprintCallable, Category = "Soul Coal")
	void SetLumps(const TArray<FTransform>& InLumps);

	int32 NumLumps() const { return Lumps.Num(); }
	bool IsShownNow() const { return bShown; }

	/** Reads the story again: shown or not. */
	void RefreshStory();

	/** There while this holds (after Main 6, until Saint Ada wakes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Coal")
	FStoryCondition ShownWhen;

	/** The lumps, in the world's frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Coal")
	TArray<FTransform> Lumps;

	/** The cyan crust of soul-light on each lump (a second instanced mesh). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Coal")
	bool bGlints = true;

	/** Drawn no farther than this (cm): lumps of coal read only near. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Coal", meta = (ClampMin = "0", Units = "cm"))
	float DrawDistance = 9000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Coal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Crust;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BuildInstances();
	void HandleMissionsChanged();

	bool bShown = true;
	float SmokeWait = 0.f;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
};
