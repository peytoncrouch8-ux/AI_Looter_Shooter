#pragma once

#include "CoreMinimal.h"
#include "StoryCondition.generated.h"

class UMissionRunner;
struct FCampaignRecord;

/**
 * When something of the story applies, read from the campaign record: after some missions are finished, before others
 * are, while one is being played. An empty condition always applies. Story characters use it to be in the world or not
 * (Abel on his board after his fight), and speaker points to pick what they say (Delia's door after each mission).
 * Missions are named by id, as the campaign record keeps them.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FStoryCondition
{
	GENERATED_BODY()

	/** Every one of these missions is finished. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	TArray<FName> AfterMissions;

	/** None of these missions is finished yet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	TArray<FName> BeforeMissions;

	/**
	 * This mission is being played: the campaign's main mission (even while the player is in another area), or a mission
	 * running in this level (a side mission). None: whatever is being played.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FName DuringMission;

	/** It asks nothing, so it always applies. */
	bool IsEmpty() const;

	/** It applies in Campaign. Runner adds the missions running in this level; without it only the campaign's main mission counts as being played. */
	bool IsMet(const FCampaignRecord& Campaign, const UMissionRunner* Runner = nullptr) const;

	/** "after Main1; before Main6; during Main2", or "always", for logs and the console. */
	FString Describe() const;
};
