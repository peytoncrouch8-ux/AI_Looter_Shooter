#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

struct FPlayableBoundary;

/** What the minimap's bake found under a texel. */
enum class EMinimapTexel : uint8
{
	/** Nothing to stand on: the void around a floating island, or a canyon with no collision. Left clear. */
	Void,
	/** Walkable terrain (anything solid the bake hits that isn't tagged Obstacle). */
	Ground,
	/** Something solid standing on the ground: rocks, walls, buildings (actors tagged Obstacle). */
	Obstacle,
};

/**
 * The minimap picture's rules texel by texel, apart from the world so they can be tested (Looter.World.Minimap.Bounds):
 * land shaded by height with coasts, cliffs, contours and obstacles, and with a playable area the outside dimmed and
 * its closed edges drawn as the boundary line. Texels run row by row from the map's north-west corner: V runs south
 * (world -X) and U east (world +Y), and a texel stands for what lies under its center, as UMinimapSubsystem traces it.
 */
namespace MinimapPaint
{
	/**
	 * The heights (cm) the land's tint spans: every ground texel's, or when Inside isn't empty (one entry per texel)
	 * only those it marks, so the ridges around a playable area don't squash the tint inside. False if none count.
	 */
	AI_LOOTER_SHOOTER_API bool HeightRange(TConstArrayView<EMinimapTexel> Kinds, TConstArrayView<float> Heights,
		TConstArrayView<uint8> Inside, float& OutMin, float& OutMax);

	/**
	 * Paints N x N texels: coasts where ground meets the void, obstacles, cliff edges, contour lines, and the land
	 * tinted by height from MinHeight to MaxHeight (heights past them take the tint's ends) with a touch of hillshade.
	 * Void stays clear.
	 */
	AI_LOOTER_SHOOTER_API void Terrain(int32 N, TConstArrayView<EMinimapTexel> Kinds, TConstArrayView<float> Heights,
		float MinHeight, float MaxHeight, TArray<FColor>& OutColors);

	/** Half the boundary line's width, in texels. */
	constexpr double LineHalfWidth = 0.9;

	/**
	 * For an N x N map over MapBounds: which texels' centers lie inside the boundary (OutInside, 1 or 0, the same
	 * answer as FPlayableBoundary::Contains), and which carry the boundary line (OutLine): within LineHalfWidth texels
	 * of a closed edge. Open edges get no line; past them is a drop, whose coast the terrain already draws.
	 */
	AI_LOOTER_SHOOTER_API void RasterizeBoundary(const FPlayableBoundary& Outline, const FBox2D& MapBounds, int32 N,
		TArray<uint8>& OutInside, TArray<uint8>& OutLine);

	/**
	 * One painted texel under the playable area's rules: void stays clear (line or not), the boundary line covers
	 * anything else, and outside the area the texel is dimmed.
	 */
	AI_LOOTER_SHOOTER_API FColor BoundaryTexel(const FColor& Painted, bool bInside, bool bOnLine);

	/** BoundaryTexel over a whole painted map (Inside and Line from RasterizeBoundary). */
	AI_LOOTER_SHOOTER_API void ApplyBoundary(TConstArrayView<uint8> Inside, TConstArrayView<uint8> Line, TArray<FColor>& Colors);

	/** The boundary line's color: the HUD's orange accent. */
	AI_LOOTER_SHOOTER_API FColor BoundaryColor();
}
