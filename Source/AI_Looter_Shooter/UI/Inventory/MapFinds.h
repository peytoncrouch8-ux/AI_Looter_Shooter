#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * The map page's count of things found off the path (FINDS.md section 3; the user's call, 2026-10-28: the map shows the total
 * and each region's count, no pins): plain numbers and words, so the page and its tests share them. Gather reads them from the
 * level's UFindSubsystem; UMapWidget::ApplyFinds shows them (MapWidgetFinds.cpp).
 */
namespace MapFinds
{
	/** One region's row ("Farm & Point  2 / 4"). */
	struct FRegionCount
	{
		FText Name;
		int32 Found = 0;
		int32 Total = 0;

		/** Every find in it found: its row turns to the done colour with a tick. */
		bool IsDone() const { return Total > 0 && Found >= Total; }
	};

	/** The finds, the ledger pages (counted on their own line) and the regions in their map order. */
	struct FCounts
	{
		int32 Found = 0;
		int32 Total = 0;
		int32 LoreFound = 0;
		int32 LoreTotal = 0;
		TArray<FRegionCount> Regions;

		/** A level without finds (the tutorial island, Skyreach) shows nothing. */
		bool HasFinds() const { return Total > 0; }
	};

	/** The most region rows the page has room for. */
	inline constexpr int32 MaxRegions = 6;

	/** "4 / 15" and "2 / 6". */
	FString FindsWords(const FCounts& Counts);
	FString LedgerWords(const FCounts& Counts);

	/** The counts as the level's find subsystem has them (empty in a world without one). */
	FCounts Gather(const UWorld* World);
}
