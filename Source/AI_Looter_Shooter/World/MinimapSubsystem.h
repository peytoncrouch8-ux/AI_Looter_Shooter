#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/MapRoad.h"
#include "World/MinimapPaint.h"
#include "MinimapSubsystem.generated.h"

class APlayableArea;
class UTexture2D;

/** One road as the map's bake paints it (an AMapRoad's data): its middle line (world XY, cm), width (cm) and kind. */
struct FMinimapRoadLine
{
	TArray<FVector2D> Points;
	float Width = 400.f;
	EMapRoadKind Kind = EMapRoadKind::Road;
};

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
 * The level's roads (AMapRoad) are painted over the land, under the trees, so the map shows the way between places.
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

	/**
	 * Paints roads onto an N x N picture over MapBounds (MinimapSubsystemRoads.cpp; the user's note 16, 2026-10-27): a
	 * road a sand band at its width with a dark edge, a path a thinner pale line, a rail a dark steel line, never thinner
	 * than MinRoadTexels so a path still shows on the minimap. Where roads meet they merge into one shape (the edge only
	 * round the outside). Only ground texels take paint (Kinds, one per texel; empty: any texel that isn't clear), so
	 * buildings, fences and the void keep theirs.
	 */
	static void PaintRoadLines(TArray<FColor>& Colors, int32 N, const FBox2D& MapBounds,
		TConstArrayView<FMinimapRoadLine> Lines, TConstArrayView<EMinimapTexel> Kinds);

	/** The colour a kind of road is painted in (its fill), and a road's edge. */
	static FColor RoadFillColor(EMapRoadKind Kind);
	static FColor RoadEdgeColor();

	/** A road's (or rail's) fill is never thinner than this across, in texels; a path's never thinner than MinPathTexels. */
	static constexpr float MinRoadTexels = 2.5f;
	static constexpr float MinPathTexels = 2.f;
	/** A road's dark edge, in texels past its fill. */
	static constexpr float RoadEdgeTexels = 1.f;

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
	/** Paints every AMapRoad in the level onto the baked picture (PaintRoadLines), on its ground texels (Kinds). */
	void PaintRoads(TArray<FColor>& Colors) const;

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
