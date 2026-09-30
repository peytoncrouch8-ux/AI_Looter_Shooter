#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UWorld;

/**
 * Seats the level's props on the terrain: every static mesh actor standing on Ground-tagged terrain moves up or down
 * until the bottom of its mesh is just under the ground everywhere, so no edge hovers and nothing is swallowed. Low,
 * wide props (walls, rocks, stepping stones) lean with the slope under them; tall ones (trees, pillars, crystals)
 * stay upright. Anything with no ground close under it (sky islands, clouds) is left alone. Editor-only: run
 * Looter.SettleProps after changing the terrain or placing props.
 */
namespace PropSettler
{
	/** How a prop sits: how far it may lean with the slope, and how far the top of its base band goes under the ground. */
	struct FRules
	{
		float MaxTiltDegrees = 0.f;
		float Embed = 0.f;
	};

	/** The bottom of a mesh: the vertices in its lowest band (what touches the ground), and its bounds. Mesh space. */
	struct FFootprint
	{
		TArray<FVector> Base;
		FBox Bounds = FBox(ForceInit);
	};

	/** Leaning and embedding for a prop this tall whose footprint's narrow side is this wide (both in cm, placed). */
	LOOTEREDITOR_API FRules RulesFor(double Height, double Width);

	LOOTEREDITOR_API FFootprint FootprintOf(UStaticMesh& Mesh);

	/**
	 * Where a prop stands once seated, given its base (mesh space) and the ground height under a point (unset where
	 * there's none). Keeps the prop's X/Y, yaw and scale, so settling again changes nothing. Unset when the prop isn't
	 * standing on the ground: part of its base has no ground under it.
	 */
	LOOTEREDITOR_API TOptional<FTransform> Settle(const FTransform& Placement, TConstArrayView<FVector> Base, const FRules& Rules,
		TFunctionRef<TOptional<double>(const FVector&)> GroundHeight);

	/** Settles every prop standing on the ground in the world, or only the selected ones. Undoable. Returns how many moved. */
	LOOTEREDITOR_API int32 SettleWorld(UWorld* World, bool bSelectedOnly);
}
