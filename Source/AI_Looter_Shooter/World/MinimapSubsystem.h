#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/MinimapPaint.h"
#include "MinimapSubsystem.generated.h"

class APlayableArea;
class UTexture2D;

/**
 * Tags the minimap reads: walkable terrain and solid things standing on it (actor tags; untagged static geometry
 * counts as ground), and trees (an actor tag, or a component tag on instanced trees such as the scatter's), which are
 * drawn as crowns since only their trunks are solid.
 */
namespace MinimapTags
{
	inline const FName Ground(TEXT("Ground"));
	inline const FName Obstacle(TEXT("Obstacle"));
	inline const FName Tree(TEXT("Tree"));
}

/**
 * A top-down picture of the playable world for the HUD minimap, baked at runtime by tracing straight down over the
 * ground (a few milliseconds per frame, so it never hitches), about a meter per texel: land shaded by height,
 * coastlines and cliff edges drawn bright, rocks, walls and trees marked, the void left clear (MinimapPaint has the
 * rules). With a playable area in the level (APlayableArea), what lies outside it is dimmed, its closed edges are drawn
 * as the boundary line, and the land's tint spans only the heights inside it.
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

	/** Texels per side of the baked map: ResolutionFor the ground's size, set as a bake starts. */
	int32 GetResolution() const { return Resolution; }

	/**
	 * Texels per side for a map this wide (cm): about a meter each, in steps of 64, never fewer than 256 (so the
	 * tutorial island's 209 m keeps its 256) and never more than 1024 (memory and bake time).
	 */
	static int32 ResolutionFor(double MapSize);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMinimapSubsystem, STATGROUP_Tickables); }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void StartBake();
	void TraceRows(double TimeBudgetSeconds);
	void FinishBake();
	/** Paints a crown for every tree onto the baked picture. */
	void PaintTrees(TArray<FColor>& Colors) const;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Texture;

	FBox2D Bounds = FBox2D(FVector2D(-10000.f), FVector2D(10000.f));
	/** Texels per side of the map being baked, or last baked. */
	int32 Resolution = 256;
	bool bBaked = false;
	bool bBaking = false;

	// Bake in progress.
	int32 NextRow = 0;
	float TraceTop = 0.f;
	float TraceBottom = 0.f;
	/** Skips the level's volumes and its playable area (see LooterWorld::StaticGeometryParams), found once per bake. */
	FCollisionQueryParams TraceParams;
	double BakeStartTime = 0.0;
	/** The level's playable area, if it has one, found as the bake starts. */
	TWeakObjectPtr<APlayableArea> PlayableArea;
	/** Ground height per texel (lowest float = nothing there). */
	TArray<float> Heights;
	TArray<EMinimapTexel> Kinds;
};
