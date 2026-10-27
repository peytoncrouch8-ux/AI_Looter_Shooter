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

	/** A name wanted on the page: where its centre would sit, how big it draws (page units), and whether it is a major one. */
	struct FNameBox
	{
		FVector2D Center = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
		bool bMajor = false;
		/** Wanted on the page at all (in the frame, and a minor one only once zoomed in); the rest are left alone. */
		bool bWanted = true;
		/** Out: drawn (true) or left off because no free spot was near. */
		bool bShown = false;
	};

	/** How big a place's name draws: its letters at the kit's floating-text size (11 major, 8 minor), spaced as they are. */
	AI_LOOTER_SHOOTER_API FVector2D NameSize(const TCHAR* Name, bool bMajor);

	/**
	 * Keeps the names from drawing over each other (Widow's Pines and the Chapel of Saint Ada did, playtest 2026-10-27).
	 * Majors are placed first, then minors, each in list order; a name that would overlap one already placed slides up or
	 * down by its own height (a few steps) to the first free spot, and if none is free it is left off (bShown false).
	 * Center is rewritten for a shifted name. Gap: the room kept between two names (page units).
	 */
	AI_LOOTER_SHOOTER_API void Declutter(TArray<FNameBox>& Boxes, double Gap);
}
