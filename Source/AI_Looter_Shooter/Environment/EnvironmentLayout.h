#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Environment/LevelLayoutData.h"
#include "EnvironmentLayout.generated.h"

class UEnvironmentPalette;
struct FEnvironmentPaletteEntry;

/**
 * Place one per level. Spawns the level's saved layout (LayoutData, built from Palette entries) on BeginPlay.
 * Temporary: levels move to placed and instanced meshes in the Unreal Editor (Docs/Plan.md, phase 2).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AEnvironmentLayout : public AActor
{
	GENERATED_BODY()

public:
	AEnvironmentLayout();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	TObjectPtr<UEnvironmentPalette> Palette;

	/** Where this level's layout is stored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	TObjectPtr<ULevelLayoutData> LayoutData;

protected:
	virtual void BeginPlay() override;

private:
	AActor* SpawnFromRecord(const FPlacedObjectRecord& Record) const;
};
