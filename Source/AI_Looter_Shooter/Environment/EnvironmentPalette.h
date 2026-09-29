#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Environment/StylizedProp.h"
#include "EnvironmentPalette.generated.h"

class UStaticMesh;

/**
 * One placeable item in Build Mode. Source priority: ActorClass, then StaticMesh (e.g. Fab/Megascans
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

	/** Tab this entry shows under in Build Mode (e.g. Terrain, Rocks, Plants, Props). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FName Category;

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

	/** Random scale range used when "random scale" is on and by the scatter brush. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.01"))
	float MinScale = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.01"))
	float MaxScale = 1.2f;

	/** Tilt to match the ground slope (good for rocks, bad for trees). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	bool bAlignToSurface = false;

	/** Rough footprint in cm; the scatter brush spaces placements by about this much. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "10"))
	float Footprint = 200.f;

	/**
	 * Sky pieces (islands, clouds): placed in mid-air in front of the camera instead of on a surface.
	 * The placement point is AirDistance along the view, or the surface under the cursor if that's closer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	bool bPlaceInAir = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (EditCondition = "bPlaceInAir", ClampMin = "100"))
	float AirDistance = 8000.f;
};

/** Everything that can be placed in Build Mode. New entries appear in the palette automatically. */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UEnvironmentPalette : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UEnvironmentPalette();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette", meta = (TitleProperty = "DisplayName"))
	TArray<FEnvironmentPaletteEntry> Entries;

	const FEnvironmentPaletteEntry* FindEntry(FName Id) const;

	/** Categories in first-seen order. */
	TArray<FName> GetCategories() const;
};
