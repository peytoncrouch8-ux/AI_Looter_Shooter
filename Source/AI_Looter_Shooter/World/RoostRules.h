#pragma once

#include "CoreMinimal.h"
#include "Story/StoryLine.h"

/** How Hob's crows brought the player to a roost (ARespawnMarker::Welcome). */
enum class ERoostArrival : uint8
{
	/** Fast travel: carried from another roost. */
	Travel,
	/** A death: dragged back to the roost last found. */
	Death,
};

/**
 * Hob's roosts as plain rules, apart from the world so the tests drive them: the beacon's look over time (its fade near the
 * camera, the column rising out of the post as a roost is found, the pulse of light climbing it), and what Hob says when
 * his crows bring Ellis home (after a death, every time; the first death, the first roost found and the first ride their
 * own once in a story).
 */
namespace RoostRules
{
	/** The beam's share of its strength with the camera Distance (cm) away: 0 within Gone, 1 from Full, smooth between. */
	AI_LOOTER_SHOOTER_API float NearFade(float Distance, float Gone, float Full);

	/** A first find: the column shoots up out of the post and eases to its full height over this long (seconds). */
	inline constexpr float RiseSeconds = 1.4f;

	/** How much of its height the column stands Age seconds into its rise: 0 at its start, 1 by RiseSeconds; 1 at rest (Age below zero). */
	AI_LOOTER_SHOOTER_API float BeamRise(float Age);

	/** A pulse of light climbing the column (a find, a mark again, a crow's homecoming), from the post to the top, this long. */
	inline constexpr float PulseSeconds = 1.3f;

	/** Where the pulse is Age seconds in (0 the post, 1 the top) and how bright (0 to 1); false before it and after it. */
	AI_LOOTER_SHOOTER_API bool PulseAt(float Age, float& OutHeight, float& OutStrength);

	// --- What Hob says (captions; his lines are his alone: he speaks only where the story is played) ---

	/** Hob's name as his captions give it. */
	AI_LOOTER_SHOOTER_API FText HobName();

	/** His lines after a death, said in a shuffled round (NextLine). */
	AI_LOOTER_SHOOTER_API int32 NumDeathLines();
	AI_LOOTER_SHOOTER_API FStoryLine DeathLine(int32 Index);

	/** Once in a story: the first death, the first roost found, the first ride between roosts. */
	AI_LOOTER_SHOOTER_API FStoryLine FirstDeathLine();
	AI_LOOTER_SHOOTER_API FStoryLine FirstRoostLine();
	AI_LOOTER_SHOOTER_API FStoryLine FirstTravelLine();

	/** The story flags (FCampaignRecord::StoryFlags) that keep the once-only lines said once in a story. */
	inline const FName FirstDeathFlag(TEXT("Roost.FirstDeath"));
	inline const FName FirstRoostFlag(TEXT("Roost.FirstFound"));
	inline const FName FirstTravelFlag(TEXT("Roost.FirstTravel"));

	/**
	 * The next of Count lines to say: each once, in a shuffled round, before any is said again, and never the one said last
	 * twice running (across rounds too). Order and Last carry the round from one call to the next; Random shuffles it.
	 */
	AI_LOOTER_SHOOTER_API int32 NextLine(TArray<int32>& Order, int32& Last, int32 Count, FRandomStream& Random);
}
