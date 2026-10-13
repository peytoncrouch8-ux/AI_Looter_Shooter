#pragma once

#include "CoreMinimal.h"

/**
 * The compass letters N, E, S and W: where each rides on the minimap's bezel (a heading-up map, so they circle as the view
 * turns) and where each sits on the map page (north up). Pure geometry, apart from the widgets that place them. The world
 * is the minimap's: north is +X and east is +Y.
 */
namespace MinimapCompass
{
	enum class EPoint : uint8
	{
		North,
		East,
		South,
		West
	};

	/** All four, in the order the compass panel holds them. */
	inline constexpr EPoint All[] = { EPoint::North, EPoint::East, EPoint::South, EPoint::West };

	/** The letter drawn for a point ("N"). */
	const TCHAR* Letter(EPoint Point);

	/** The world yaw (degrees) of the point's direction: north 0, east 90, south 180, west 270. */
	float WorldYaw(EPoint Point);

	/** Degrees clockwise from straight up (0 to under 360) that the point lies at on a map that turns to show ViewYaw up. */
	float BezelDegrees(EPoint Point, float ViewYaw);

	/** The point's offset from the map's centre, Radius out along that angle (+X right, +Y down, as widgets are placed). */
	FVector2D BezelOffset(EPoint Point, float ViewYaw, float Radius);

	/** The middle of the frame's edge the point faces on the north-up map page: N top, E right, S bottom, W left. */
	FVector2D EdgeMidpoint(EPoint Point, const FVector2D& FramePosition, const FVector2D& FrameSize);
}
