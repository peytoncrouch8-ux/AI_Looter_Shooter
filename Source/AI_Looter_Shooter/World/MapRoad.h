#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapRoad.generated.h"

/** What a road on the map is drawn as. */
UENUM(BlueprintType)
enum class EMapRoadKind : uint8
{
	/** A wagon road: a sand-coloured band at its width with a dark edge. */
	Road,
	/** A footpath: a thinner, paler line with no edge. */
	Path,
	/** A rail bed or the ore tramway: a dark steel line. */
	Rail,
};

/**
 * One of a level's roads, as the map draws it (the user's note 16, 2026-10-27: "the map doesn't really prompt the player
 * to use roads"). Nothing but data: its line (world XY, cm, as the layout's road path, smoothed), its width and its kind.
 * Tools/Unreal/build_area_dressing.py places one per layout road (MapRoad_<id>, in the Dressing/Roadside folder);
 * UMinimapSubsystem paints them all into its bake, so the HUD minimap and the map page both show them. Hidden, no
 * collision, never ticks.
 */
UCLASS(NotBlueprintable)
class AI_LOOTER_SHOOTER_API AMapRoad : public AActor
{
	GENERATED_BODY()

public:
	AMapRoad();

	/** The road's middle line, world X (north) and Y (east) in cm, in order along it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	TArray<FVector2D> Points;

	/** Its full width (cm), as the terrain cuts it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ClampMin = "0"))
	float Width = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	EMapRoadKind Kind = EMapRoadKind::Road;
};
