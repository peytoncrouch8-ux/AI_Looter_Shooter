#pragma once

#include "CoreMinimal.h"

/** A named place written on the map page: a town, a farm, a landmark. */
struct AI_LOOTER_SHOOTER_API FMapPlace
{
	const TCHAR* Name = TEXT("");
	/** Where its name is centred (world cm: X north, Y east). */
	FVector2D At = FVector2D::ZeroVector;
	/** A settlement or a big place, named large at every zoom; a minor one (a rock, a gully) only once zoomed in. */
	bool bMajor = false;
};

/**
 * The names the map page writes over each level, from the level layouts' plan labels (Art/Levels/<Level>/layout.json,
 * "preview.labels", kept in step by hand; Skyreach's as Crossroads Town renamed its places). A level with none listed is
 * mapped without names.
 */
namespace MapPlaces
{
	/** The places of a level, by its package's short name ("Lvl_RansomsRest"); empty when it has none. */
	AI_LOOTER_SHOOTER_API TConstArrayView<FMapPlace> For(const FString& LevelName);

	/** The zoom from which the minor places are named too. */
	inline constexpr double MinorZoom = 1.8;
}
