#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Subsystems/WorldSubsystem.h"
#include "MinimapSubsystem.generated.h"

class UTexture2D;

/** Actor tags the minimap reads: walkable terrain, and solid things standing on it. Untagged static geometry counts as ground. */
namespace MinimapTags
{
	inline const FName Ground(TEXT("Ground"));
	inline const FName Obstacle(TEXT("Obstacle"));
}

/**
 * A top-down picture of the playable world for the HUD minimap, baked at runtime by tracing straight down over the
 * islands (a few milliseconds per frame, so it never hitches): land shaded by height, coastlines and cliff edges drawn
 * bright, rocks, walls and trees marked, the void left clear.
 *
 * Map space: U runs east (world +Y) and V runs south (world -X), so north (world +X) is up.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMinimapSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The baked map. Starts baking on first use; null until the first bake finishes (or if there's no ground). */
	UTexture2D* GetMapTexture();

	/** The world rectangle (XY, cm) the map covers. Square. */
	const FBox2D& GetMapBounds() const { return Bounds; }

	/** 0-1 map coordinates of a world position. */
	FVector2D WorldToMapUV(const FVector& World) const { return WorldToMapUV(Bounds, World); }
	static FVector2D WorldToMapUV(const FBox2D& MapBounds, const FVector& World);

	/**
	 * Where something at WorldDelta from the player appears on a minimap that turns with the view (the view direction,
	 * yaw in degrees, points up), in pixels from the center (+X right, +Y down).
	 */
	static FVector2D ViewOffset(const FVector& WorldDelta, float ViewYaw, float PixelsPerCm);

	/** Texels per side of the baked map. */
	static constexpr int32 Resolution = 256;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMinimapSubsystem, STATGROUP_Tickables); }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void StartBake();
	void TraceRows(double TimeBudgetSeconds);
	void FinishBake();

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Texture;

	FBox2D Bounds = FBox2D(FVector2D(-10000.f), FVector2D(10000.f));
	bool bBaked = false;
	bool bBaking = false;

	// Bake in progress.
	int32 NextRow = 0;
	float TraceTop = 0.f;
	float TraceBottom = 0.f;
	/** Skips the level's volumes (see LooterWorld::StaticGeometryParams), found once per bake. */
	FCollisionQueryParams TraceParams;
	double BakeStartTime = 0.0;
	/** Ground height per texel (lowest float = nothing there). */
	TArray<float> Heights;
	/** 0 = void, 1 = ground, 2 = obstacle (rocks, walls, trees, props). */
	TArray<uint8> Kinds;
};
