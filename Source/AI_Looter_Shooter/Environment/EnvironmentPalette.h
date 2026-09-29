#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Environment/StylizedProp.h"
#include "EnvironmentPalette.generated.h"

class UStaticMesh;

/**
 * One kind of object a level layout can spawn. Source priority: ActorClass, then StaticMesh (e.g. Fab/Megascans
 * assets), then the procedural Shape.
 */
USTRUCT(BlueprintType)
struct FEnvironmentPaletteEntry
{
	GENERATED_BODY()

	/** Unique, stable id. Saved layouts reference entries by this, so don't rename it once used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	TSoftClassPtr<AActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	EStylizedPropShape Shape = EStylizedPropShape::Rock;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	FLinearColor PrimaryColor = FLinearColor(0.55f, 0.36f, 0.22f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	FLinearColor SecondaryColor = FLinearColor(0.35f, 0.45f, 0.15f);
};

/** Everything a level layout can spawn, looked up by id. */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UEnvironmentPalette : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UEnvironmentPalette();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette", meta = (TitleProperty = "DisplayName"))
	TArray<FEnvironmentPaletteEntry> Entries;

	const FEnvironmentPaletteEntry* FindEntry(FName Id) const;
};
