#pragma once

#include "CoreMinimal.h"

struct FCampaignRecord;

/**
 * A campaign record written against the missions before the Ransom's Rest expansion (2026-10-13), brought onto today's
 * (ULooterSessionSave::Upgrade, version 2 to 3). Two things changed under old saves that day:
 *  - the step lists (wave 4, Tools/Unreal/create_mission_assets.py and create_side_mission_assets.py): Main 4 gained the
 *    knock at the vestry door as its step 2, so an old Main 4 save on the bell or the Reliquary landed one step early;
 *    Side 2's talk with Amos became his offer and the road talk went in before the hands. Main 3's list didn't move (the
 *    same three steps since 2026-10-09), so it maps to itself.
 *  - the givers (wave 1): Side 1, 2 and 3 started by themselves once Main 3, 4 or 5 was done; now they're offered, and an
 *    old save never recorded them as taken (a side mission's step isn't kept: it starts again from its first). One whose
 *    main mission was done in the old save was running there, so it counts as taken, as a running main mission does.
 * Which changes a save predates is told by when it was last written (ULooterSessionSave::Saved, local time) against when
 * each reached the game: a version 2 save written after them already has today's lists and is left alone.
 */
namespace CampaignStepMigration
{
	/** When the missions picked up from givers came in (wave 1's commit, 2026-10-13 09:30, local time). */
	AI_LOOTER_SHOOTER_API FDateTime GiversSince();

	/** When the step lists changed (wave 4's commit, 2026-10-13 17:28, local time). */
	AI_LOOTER_SHOOTER_API FDateTime StepListsSince();

	/**
	 * The step OldStep of Mission (counted from 0, as a record keeps it) in a list from before 2026-10-13, in today's list.
	 * A mission whose steps didn't move maps to itself; one past the old last step (waiting for the turn-in) maps to one past
	 * today's.
	 */
	AI_LOOTER_SHOOTER_API int32 MapOldStep(FName Mission, int32 OldStep);

	/**
	 * Moves a record last written at Saved onto today's missions: the story mission's step and the optional objectives done
	 * (kept by their step) when Saved predates the step lists; the side missions that ran by themselves taken when it
	 * predates the givers. Returns what it moved, for the log (empty: nothing).
	 */
	AI_LOOTER_SHOOTER_API FString Upgrade(FCampaignRecord& Campaign, const FDateTime& Saved);
}
