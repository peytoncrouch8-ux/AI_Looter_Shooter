#pragma once

#include "CoreMinimal.h"
#include "Story/StoryLine.h"

class AActor;
class UMissionDefinition;
class UObject;

/**
 * What a mission's giver says around its taking (FMissionOffer::Lines and AcceptLines), Borderlands' way (the user's call,
 * 2026-10-22): a main mission that starts by itself says both, queued, wherever the player is (UMissionRunner); the offer
 * card's Accept cuts the offer's words short and goes straight to the accept lines, and its Not now stops the giver talking
 * (UMissionOfferWidget). A line naming no speaker is said in the giver's name: their speaker point's, else the offer's.
 */
namespace MissionTalk
{
	/** Who says a giver's unnamed lines: Giver's speaker point's name, else the mission's offering giver's name. */
	AI_LOOTER_SHOOTER_API FText GiverSpeaker(const UMissionDefinition& Mission, const AActor* Giver);

	/** Lines in the giver's name where they name no speaker. */
	AI_LOOTER_SHOOTER_API TArray<FStoryLine> InGiverName(TArray<FStoryLine> Lines, const UMissionDefinition& Mission, const AActor* Giver);

	/**
	 * A main mission started by itself: its offer's lines, then its accept lines, after whatever is being said. Their
	 * conversation, or 0 (none to say, or no captions).
	 */
	AI_LOOTER_SHOOTER_API int32 SayOnTheMove(const UObject* WorldContext, const UMissionDefinition& Mission, const AActor* Giver);

	/**
	 * Taken on the card: someone at a speaker point stops what they were saying and says the accept lines instead (none:
	 * they only stop); a board or a found thing's accept lines (a remark) come after whatever is being said. Their
	 * conversation, or 0.
	 */
	AI_LOOTER_SHOOTER_API int32 SayAccepted(const UObject* WorldContext, const UMissionDefinition& Mission, AActor* Giver);

	/** Not now on the card: someone at a speaker point stops talking, if they still are. */
	AI_LOOTER_SHOOTER_API void StopGiver(AActor* Giver);
}
