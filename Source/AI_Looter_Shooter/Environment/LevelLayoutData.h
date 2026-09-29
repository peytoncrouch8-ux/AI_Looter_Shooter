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
 * Everything placed in Build Mode for one level. Saved from Build Mode while playing in the editor,
 * and loaded by the level's EnvironmentLayout actor in every build (so players see it too).
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API ULevelLayoutData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	TArray<FPlacedObjectRecord> Objects;
};
