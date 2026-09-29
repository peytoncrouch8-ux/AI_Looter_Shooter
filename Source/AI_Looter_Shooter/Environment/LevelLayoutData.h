#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelLayoutData.generated.h"

USTRUCT(BlueprintType)
struct FPlacedObjectRecord
{
	GENERATED_BODY()

	/** FEnvironmentPaletteEntry::Id this object was placed from. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	FName EntryId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	FTransform Transform;

	/** Variation seed for procedural props. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 Seed = 0;
};

/**
 * Every object one level's EnvironmentLayout actor spawns when play starts. Made by the retired in-game Build Mode;
 * it goes away once the level is converted to placed meshes (Docs/Plan.md, phase 2).
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API ULevelLayoutData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	TArray<FPlacedObjectRecord> Objects;
};
